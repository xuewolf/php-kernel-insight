/*
   +----------------------------------------------------------------------+
   | Zend Engine                                                          |
   +----------------------------------------------------------------------+
   | Copyright (c) Zend Technologies Ltd. (http://www.zend.com)           |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.00 of the Zend license,     |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.zend.com/license/2_00.txt.                                |
   | If you did not receive a copy of the Zend license and are unable to  |
   | obtain it through the world-wide-web, please send a note to          |
   | license@zend.com so we can mail you a copy immediately.              |
   +----------------------------------------------------------------------+
   | Authors: Andi Gutmans <andi@php.net>                                 |
   |          Marcus Boerger <helly@php.net>                              |
   |          Sterling Hughes <sterling@php.net>                          |
   |          Zeev Suraski <zeev@php.net>                                 |
   +----------------------------------------------------------------------+
*/

#include "zend.h"
#include "zend_API.h"
#include "zend_builtin_functions.h"
#include "zend_interfaces.h"
#include "zend_exceptions.h"
#include "zend_vm.h"
#include "zend_dtrace.h"
#include "zend_smart_str.h"
#include "zend_exceptions_arginfo.h"
#include "zend_observer.h"

// 异常类都是最普通的 zend_class_entry
ZEND_API zend_class_entry *zend_ce_throwable;
ZEND_API zend_class_entry *zend_ce_exception;
ZEND_API zend_class_entry *zend_ce_error_exception;
ZEND_API zend_class_entry *zend_ce_error;
ZEND_API zend_class_entry *zend_ce_compile_error;
ZEND_API zend_class_entry *zend_ce_parse_error;
ZEND_API zend_class_entry *zend_ce_type_error;
ZEND_API zend_class_entry *zend_ce_argument_count_error;
ZEND_API zend_class_entry *zend_ce_value_error;
ZEND_API zend_class_entry *zend_ce_arithmetic_error;
ZEND_API zend_class_entry *zend_ce_division_by_zero_error;
ZEND_API zend_class_entry *zend_ce_unhandled_match_error;

/* Internal pseudo-exception that is not exposed to userland. Throwing this exception *does not* execute finally blocks. */
static zend_class_entry zend_ce_unwind_exit;

/* Internal pseudo-exception that is not exposed to userland. Throwing this exception *does* execute finally blocks. */
static zend_class_entry zend_ce_graceful_exit;

ZEND_API void (*zend_throw_exception_hook)(zend_object *ex);

// 
static zend_object_handlers default_exception_handlers;

// ing3, 实现 throwable 接口前的检测：class_type 是否继承自  Exception 或 Error 。不是则报错
/* {{{ zend_implement_throwable */
static int zend_implement_throwable(zend_class_entry *interface, zend_class_entry *class_type)
{
	// zend_ce_exception 和 zend_ce_error 在被调用时，可能还未载入（例如为 Exception类实现 Throwable 接口）
	// 执行一个多重继承检查。
	/* zend_ce_exception and zend_ce_error may not be initialized yet when this is called (e.g when
	 * implementing Throwable for Exception itself). Perform a manual inheritance check. */
	zend_class_entry *root = class_type;
	// 找到最上层类
	while (root->parent) {
		root = root->parent;
	}
	// 如果 class_type 继承自  Exception 或 Error
	if (zend_string_equals_literal(root->name, "Exception")
			|| zend_string_equals_literal(root->name, "Error")) {
		// 返回成功
		return SUCCESS;
	}
	// enum不可以被继承
	bool can_extend = (class_type->ce_flags & ZEND_ACC_ENUM) == 0;

	// 抛错：不可以实现***接口，可以继承 Exception 或 Error 来替代它。
	zend_error_noreturn(E_ERROR,
		can_extend
			? "%s %s cannot implement interface %s, extend Exception or Error instead"
			: "%s %s cannot implement interface %s",
		zend_get_object_type_uc(class_type),
		ZSTR_VAL(class_type->name),
		ZSTR_VAL(interface->name));
	// 返回：失败
	return FAILURE;
}
/* }}} */

// ing4, object所属类继承自 zend_ce_exception ？使用 zend_ce_exception ： zend_ce_error
static inline zend_class_entry *i_get_exception_base(zend_object *object) /* {{{ */
{
	// object所属类继承自 zend_ce_exception ？使用 zend_ce_exception ： zend_ce_error
	return instanceof_function(object->ce, zend_ce_exception) ? zend_ce_exception : zend_ce_error;
}
/* }}} */

// ing4, object所属类继承自 zend_ce_exception ？使用 zend_ce_exception ： zend_ce_error
ZEND_API zend_class_entry *zend_get_exception_base(zend_object *object) /* {{{ */
{
	// object所属类继承自 zend_ce_exception ？使用 zend_ce_exception ： zend_ce_error
	return i_get_exception_base(object);
}
/* }}} */

// ing3, 如果ex和add_previous都不在对方的previous链上，把add_previous添加到exprevious链的源头上。
void zend_exception_set_previous(zend_object *exception, zend_object *add_previous) /* {{{ */
{
	zval *previous, *ancestor, *ex;
	// rv只是为了调用必须传递的参数，没有用
	zval  pv, zv, rv;
	zend_class_entry *base_ce;
	// 步骤1，处理两个特殊情况，这两个情况什么都不做
	// 如果两个里有一个为空，直接返回
	if (!exception || !add_previous) {
		return;
	}
	// 如果两个相同，或 add_previous 类型是 zend_ce_unwind_exit 或 zend_ce_graceful_exit
	if (exception == add_previous || zend_is_unwind_exit(add_previous) || zend_is_graceful_exit(add_previous)) {
		// 释放 add_previous 并返回
		OBJ_RELEASE(add_previous);
		return;
	}
	
	// 步骤2，一路往回遍历
	// add_previous 必须继承自 zend_ce_throwable
	ZEND_ASSERT(instanceof_function(add_previous->ce, zend_ce_throwable)
		&& "Previous exception must implement Throwable");

	// 前一个异常 zval
	ZVAL_OBJ(&pv, add_previous);
	// 当前异常 zval
	ZVAL_OBJ(&zv, exception);
	// ex先用当前异常, 一路往回遍历
	ex = &zv;
	do {
		// 步骤3， 先从 add_previous 一路往回遍历，看能不能找到 ex
		// object所属类继承自 zend_ce_exception ？使用 zend_ce_exception ： zend_ce_error
		// ancestor : 获取 add_previous 的 previous属性
		ancestor = zend_read_property_ex(i_get_exception_base(add_previous), add_previous, ZSTR_KNOWN(ZEND_STR_PREVIOUS), 1, &rv);
		// 如果 ancestor 是对象
		while (Z_TYPE_P(ancestor) == IS_OBJECT) {
			// ancestor 和 当前对象是同一个。说明 ex 在 add_previous 前面
			if (Z_OBJ_P(ancestor) == Z_OBJ_P(ex)) {
				// 释放 前一个 异常
				OBJ_RELEASE(add_previous);
				return;
			}
			// object所属类继承自 zend_ce_exception ？使用 zend_ce_exception ： zend_ce_error
			// 获取 ancestor 的 previous属性。 一路向回找
			ancestor = zend_read_property_ex(i_get_exception_base(Z_OBJ_P(ancestor)), Z_OBJ_P(ancestor), ZSTR_KNOWN(ZEND_STR_PREVIOUS), 1, &rv);
		}
		
		// 步骤4，从 ex 往回找
		// object所属类继承自 zend_ce_exception ？使用 zend_ce_exception ： zend_ce_error
		base_ce = i_get_exception_base(Z_OBJ_P(ex));
		// 读取 ex下属 对象 的 previous 属性。
		previous = zend_read_property_ex(base_ce, Z_OBJ_P(ex), ZSTR_KNOWN(ZEND_STR_PREVIOUS), 1, &rv);
		// 如果 previous 的值为null
		if (Z_TYPE_P(previous) == IS_NULL) {
			// 更新 它，写入 前一个异常
			zend_update_property_ex(base_ce, Z_OBJ_P(ex), ZSTR_KNOWN(ZEND_STR_PREVIOUS), &pv);
			// 引用数 -1 
			GC_DELREF(add_previous);
			return;
		}
		// ex 转到前一个异常
		ex = previous;
	// 直到 找到 add_previous 为止
	} while (Z_OBJ_P(ex) != add_previous);
}
/* }}} */

// ing4, 把异常保存到 previous 链里 ,清空当前异常
void zend_exception_save(void) /* {{{ */
{
	// 如果有前一个异常
	if (EG(prev_exception)) {
		// 如果ex和add_previous都不在对方的previous链上，把add_previous添加到exprevious链的源头上。
		zend_exception_set_previous(EG(exception), EG(prev_exception));
	}
	// 如果有当前异常
	if (EG(exception)) {
		// 存到前一个异常里
		EG(prev_exception) = EG(exception);
	}
	// 清空当前异常
	EG(exception) = NULL;
}
/* }}} */

// ing3, 如果有当前异常，把上一个异常存储起来，如果没有，把上一个作为当前异常
void zend_exception_restore(void) /* {{{ */
{
	// 如果有前一个exception
	if (EG(prev_exception)) {
		// 并且有当前 异常
		if (EG(exception)) {
			// 如果ex和add_previous都不在对方的previous链上，把add_previous添加到exprevious链的源头上。
			zend_exception_set_previous(EG(exception), EG(prev_exception));
		// 如果没有当前 异常
		} else {
			// 上一个异常作为当前异常
			EG(exception) = EG(prev_exception);
		}
		// 清空上一个异常
		EG(prev_exception) = NULL;
	}
}
/* }}} */

// ing3, 检验是否：上下文不存在 或 或函数名不存在 或 或不是内置函数 或 操作码是 ZEND_HANDLE_EXCEPTION
static zend_always_inline bool is_handle_exception_set(void) {
	// 当前执行上下文
	zend_execute_data *execute_data = EG(current_execute_data);
	// 如果上下文不存在
	return !execute_data
		// 或函数名不存在
		|| !execute_data->func
		// #define ZEND_USER_CODE(type) ((type) != ZEND_INTERNAL_FUNCTION)
		// 或不是内置函数
		|| !ZEND_USER_CODE(execute_data->func->common.type)
		// 或操作码是 ZEND_HANDLE_EXCEPTION
		|| execute_data->opline->opcode == ZEND_HANDLE_EXCEPTION;
		
}

// ing3, 抛出内部异常。EG里面异常相关的这几个东西比较绕
ZEND_API ZEND_COLD void zend_throw_exception_internal(zend_object *exception) /* {{{ */
{
// 默认没有开启 dtrace
#ifdef HAVE_DTRACE
	if (DTRACE_EXCEPTION_THROWN_ENABLED()) {
		if (exception != NULL) {
			DTRACE_EXCEPTION_THROWN(ZSTR_VAL(exception->ce->name));
		} else {
			DTRACE_EXCEPTION_THROWN(NULL);
		}
	}
#endif /* HAVE_DTRACE */
	
	// 情况1，如果 有传入exception
	if (exception != NULL) {
		// 前一个exception
		zend_object *previous = EG(exception);
		// 如果前一个 exception 存在， 并且它是 zend_ce_unwind_exit
		if (previous && zend_is_unwind_exit(previous)) {
			// 不要用其他exception 来代替正在展开的 exception
			/* Don't replace unwinding exception with different exception. */
			// 释放 exception
			OBJ_RELEASE(exception);
			return;
		}
		// 如果ex和add_previous都不在对方的previous链上，把add_previous添加到exprevious链的源头上。
		zend_exception_set_previous(exception, EG(exception));
		// 更新当前异常
		EG(exception) = exception;
		// 如果有原来的当前异常
		if (previous) {
			// 检验是否：上下文不存在 或 或函数名不存在 或 或不是内置函数 或 操作码是 ZEND_HANDLE_EXCEPTION
			ZEND_ASSERT(is_handle_exception_set() && "HANDLE_EXCEPTION not set?");
			// 返回
			return;
		}
	}
	
	// 情况2，如果exception不存在，且没有当前执行上下文
	if (!EG(current_execute_data)) {
		// 如果有传入异常 并且 传入异常类型是 zend_ce_parse_error 或 zend_ce_compile_error
		if (exception && (exception->ce == zend_ce_parse_error || exception->ce == zend_ce_compile_error)) {
			// 返回
			return;
		}
		// 如果有当前异常
		if (EG(exception)) {
			// 报错
			zend_exception_error(EG(exception), E_ERROR);
			// 跳伞
			zend_bailout();
		}
		// 如果没有当前异常， 报错：缺少堆栈信息0
		zend_error_noreturn(E_CORE_ERROR, "Exception thrown without a stack frame");
	}
	
	// 如果有 EG(current_execute_data)

	// 情况3， 如果有异常钩子
	if (zend_throw_exception_hook) {
		// 调用钩子
		zend_throw_exception_hook(exception);
	}
	
	// 情况4， 检验是否：上下文不存在 或 或函数名不存在 或 或不是内置函数 或 操作码是 ZEND_HANDLE_EXCEPTION
	if (is_handle_exception_set()) {
		/* no need to rethrow the exception */
		return;
	}
	// 情况5，其他情况
	// 当前操作码 存储成 异常前操作码
	EG(opline_before_exception) = EG(current_execute_data)->opline;
	// 当前操作码为异常操作码
	EG(current_execute_data)->opline = EG(exception_op);
}
/* }}} */

// ing3, 清除异常
ZEND_API void zend_clear_exception(void) /* {{{ */
{
	zend_object *exception;
	// 如果有前一个异常
	if (EG(prev_exception)) {
		// 释放前一个异常
		OBJ_RELEASE(EG(prev_exception));
		// 指针置空
		EG(prev_exception) = NULL;
	}
	// 如果没有异常了
	if (!EG(exception)) {
		// 返回
		return;
	}
	// 异常有可以自带销毁器
	/* exception may have destructor */
	//
	exception = EG(exception);
	// 指针置空
	EG(exception) = NULL;
	// 释放 exception
	OBJ_RELEASE(exception);
	// 如果当前有执行上下文
	if (EG(current_execute_data)) {
		// 使用异常前一个操作码（这是干嘛）
		EG(current_execute_data)->opline = EG(opline_before_exception);
	}
#if ZEND_DEBUG
	EG(opline_before_exception) = NULL;
#endif
}
/* }}} */

// ing3, 创建新的默认exception
static zend_object *zend_default_exception_new_ex(zend_class_entry *class_type, bool skip_top_traces) /* {{{ */
{
	zval tmp;
	zval trace;
	zend_class_entry *base_ce;
	zend_string *filename;
	// 创建新对象
	zend_object *object = zend_objects_new(class_type);
	// 使用默认异常处理方法
	object->handlers = &default_exception_handlers;
	// 初始化对象属性
	object_properties_init(object, class_type);
	// 如果有当前执行上下文
	if (EG(current_execute_data)) {
		// 取出trace信息 
		zend_fetch_debug_backtrace(&trace,
			skip_top_traces,
			EG(exception_ignore_args) ? DEBUG_BACKTRACE_IGNORE_ARGS : 0, 0);
	// 如果没有
	} else {
		// 初始化 trace 数组
		array_init(&trace);
	}
	// trace 引用数是0
	Z_SET_REFCOUNT(trace, 0);
	// object所属类继承自 zend_ce_exception ？使用 zend_ce_exception ： zend_ce_error
	base_ce = i_get_exception_base(object);

	// 如果 （类型不是 zend_ce_parse_error 也不是 zend_ce_compile_error）
	if (EXPECTED((class_type != zend_ce_parse_error && class_type != zend_ce_compile_error)
			// 或获取当前正在编译的文件名失败
			|| !(filename = zend_get_compiled_filename()))) {
		// 取得正在执行的文件名
		ZVAL_STRING(&tmp, zend_get_executed_filename());
		// 更新object的file属性，把文件名写进去
		zend_update_property_ex(base_ce, object, ZSTR_KNOWN(ZEND_STR_FILE), &tmp);
		// 销毁临时变量
		zval_ptr_dtor(&tmp);
		// 取得正在执行的行号
		ZVAL_LONG(&tmp, zend_get_executed_lineno());
		// 更新object的line属性，把行号写进去
		zend_update_property_ex(base_ce, object, ZSTR_KNOWN(ZEND_STR_LINE), &tmp);
	// 其他情况
	} else {
		// 文件名写进临时变量
		ZVAL_STR(&tmp, filename);
		// 更新object的file属性，把文件名写进去
		zend_update_property_ex(base_ce, object, ZSTR_KNOWN(ZEND_STR_FILE), &tmp);
		// 行号写进临时变量
		ZVAL_LONG(&tmp, zend_get_compiled_lineno());
		// 更新object的line属性，把行号关进去
		zend_update_property_ex(base_ce, object, ZSTR_KNOWN(ZEND_STR_LINE), &tmp);
	}
	// 更新object的trace属性，把 trace 写进去
	zend_update_property_ex(base_ce, object, ZSTR_KNOWN(ZEND_STR_TRACE), &trace);
	// 返回object
	return object;
}
/* }}} */

// ing4, 创建新的默认exception, 不跳过top trace
static zend_object *zend_default_exception_new(zend_class_entry *class_type) /* {{{ */
{
	// 创建新的默认exception, 不跳过top trace
	return zend_default_exception_new_ex(class_type, 0);
}
/* }}} */

// ing4, 和上面一样
static zend_object *zend_error_exception_new(zend_class_entry *class_type) /* {{{ */
{
	// 创建新的默认exception
	return zend_default_exception_new_ex(class_type, 0);
}
/* }}} */

// ing4, Exception类 的 __clone方法
/* {{{ Clone the exception object */
ZEND_COLD ZEND_METHOD(Exception, __clone)
{
	// 此方法不可调用
	/* Should never be executable */
	zend_throw_exception(NULL, "Cannot clone object using __clone()", 0);
}
/* }}} */

// ing3, Exception 的 __construct方法
/* {{{ Exception constructor */
ZEND_METHOD(Exception, __construct)
{
	zend_string *message = NULL;
	zend_long   code = 0;
	zval  tmp, *object, *previous = NULL;
	zend_class_entry *base_ce;

	object = ZEND_THIS;
	// object所属类继承自 zend_ce_exception ？使用 zend_ce_exception ： zend_ce_error
	base_ce = i_get_exception_base(Z_OBJ_P(object));
	
	// 解析3个参数: 消息，错误码，previous。 为什么还要传个类名？
	// 解析传入的参数（包含可变参数）p1:参数数量，p2:格式列表，p+:参数列表
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "|SlO!", &message, &code, &previous, zend_ce_throwable) == FAILURE) {
		// 失败则报错
		RETURN_THROWS();
	}

	// 如果有 消息
	if (message) {
		// 放入临时变量
		ZVAL_STR(&tmp, message);
		// 更新 message 属性
		zend_update_property_ex(base_ce, Z_OBJ_P(object), ZSTR_KNOWN(ZEND_STR_MESSAGE), &tmp);
	}

	// 如果有 错误码
	if (code) {
		// 放入临时变量
		ZVAL_LONG(&tmp, code);
		// 更新 code 属性
		zend_update_property_ex(base_ce, Z_OBJ_P(object), ZSTR_KNOWN(ZEND_STR_CODE), &tmp);
	}

	// 如果有 previous
	if (previous) {
		// 更新 previous 属性
		zend_update_property_ex(base_ce, Z_OBJ_P(object), ZSTR_KNOWN(ZEND_STR_PREVIOUS), previous);
	}
}
/* }}} */

// ing3, 检查 反序列化时的异常。类型不匹配则删除属性。p1:属性名，p2:要求类型
/* {{{ Exception unserialize checks */
#define CHECK_EXC_TYPE(id, type) \
	/* object所属类继承自 zend_ce_exception ？使用 zend_ce_exception ： zend_ce_error */ \
	pvalue = zend_read_property_ex(i_get_exception_base(Z_OBJ_P(object)), Z_OBJ_P(object), ZSTR_KNOWN(id), 1, &value); \
	/* pvalue类型不是null，和要求的不同 */ \
	if (Z_TYPE_P(pvalue) != IS_NULL && Z_TYPE_P(pvalue) != type) { \
		/* 删除此属性值  */ \
		/* object所属类继承自 zend_ce_exception ？使用 zend_ce_exception ： zend_ce_error */ \
		zend_unset_property(i_get_exception_base(Z_OBJ_P(object)), Z_OBJ_P(object), ZSTR_VAL(ZSTR_KNOWN(id)), ZSTR_LEN(ZSTR_KNOWN(id))); \
	}

// ing3, 唤醒。这时候要做 返序列化检查
ZEND_METHOD(Exception, __wakeup)
{
	// 不可以有参数
	ZEND_PARSE_PARAMETERS_NONE();

	zval value, *pvalue;
	//
	zval *object = ZEND_THIS;
	// 检查 反序列化时的异常。类型不匹配则删除属性。p1:属性名，p2:要求类型
	CHECK_EXC_TYPE(ZEND_STR_MESSAGE, IS_STRING);
	CHECK_EXC_TYPE(ZEND_STR_CODE,    IS_LONG);
	/* The type of all other properties is enforced through typed properties. */
}
/* }}} */

// ing4, ErrorException 类的构造方法
/* {{{ ErrorException constructor */
ZEND_METHOD(ErrorException, __construct)
{
	zend_string *message = NULL, *filename = NULL;
	zend_long   code = 0, severity = E_ERROR, lineno;
	bool lineno_is_null = 1;
	zval   tmp, *object, *previous = NULL;

	// 解析参数 message,code,severity,filename,lineno,lineno_is_null,previous
	// 解析传入的参数（包含可变参数）p1:参数数量，p2:格式列表，p+:参数列表
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "|SllS!l!O!", &message, &code, &severity, &filename, &lineno, &lineno_is_null, &previous, zend_ce_throwable) == FAILURE) {
		// 解析错误则抛错
		RETURN_THROWS();
	}

	// this
	object = ZEND_THIS;

	// 如果有 message 参数
	if (message) {
		// 先复制到临时变量
		ZVAL_STR_COPY(&tmp, message);
		// 更新 message 属性
		zend_update_property_ex(zend_ce_exception, Z_OBJ_P(object), ZSTR_KNOWN(ZEND_STR_MESSAGE), &tmp);
		// 清空临时变量
		zval_ptr_dtor(&tmp);
	}

	// 如果有 code 参数
	if (code) {
		// 取出整数
		ZVAL_LONG(&tmp, code);
		// 更新 code 属性
		zend_update_property_ex(zend_ce_exception, Z_OBJ_P(object), ZSTR_KNOWN(ZEND_STR_CODE), &tmp);
	}

	// 如果有 previous 参数
	if (previous) {
		// 更新 previous 属性
		zend_update_property_ex(zend_ce_exception, Z_OBJ_P(object), ZSTR_KNOWN(ZEND_STR_PREVIOUS), previous);
	}

	// 这个有默认值，一定要写
	// 取出整数
	ZVAL_LONG(&tmp, severity);
	// 更新 severity 属性
	zend_update_property_ex(zend_ce_exception, Z_OBJ_P(object), ZSTR_KNOWN(ZEND_STR_SEVERITY), &tmp);

	// 如果有 filename 参数
	if (filename) {
		// 先复制到临时变量
		ZVAL_STR_COPY(&tmp, filename);
		// 更新 filename 属性
		zend_update_property_ex(zend_ce_exception, Z_OBJ_P(object), ZSTR_KNOWN(ZEND_STR_FILE), &tmp);
		// 清空临时变量
		zval_ptr_dtor(&tmp);
	}

	// 如果有行号
	if (!lineno_is_null) {
		// 取出整数
		ZVAL_LONG(&tmp, lineno);
		// 更新 lineno 属性
		zend_update_property_ex(zend_ce_exception, Z_OBJ_P(object), ZSTR_KNOWN(ZEND_STR_LINE), &tmp);
	// 如果没有行号，有文件名
	} else if (filename) {
		// 值为0
		ZVAL_LONG(&tmp, 0);
		// 更新 lineno 属性
		zend_update_property_ex(zend_ce_exception, Z_OBJ_P(object), ZSTR_KNOWN(ZEND_STR_LINE), &tmp);
	}
}
/* }}} */

// ing4, 取得object对象的 指定 属性，非静默，需要rv
#define GET_PROPERTY(object, id) \
	/* object所属类继承自 zend_ce_exception ？使用 zend_ce_exception ： zend_ce_error */ \
	zend_read_property_ex(i_get_exception_base(Z_OBJ_P(object)), Z_OBJ_P(object), ZSTR_KNOWN(id), 0, &rv)

// ing4, 取得object对象的 指定 属性，静默，需要rv	
#define GET_PROPERTY_SILENT(object, id) \
	/* object所属类继承自 zend_ce_exception ？使用 zend_ce_exception ： zend_ce_error */ \
	zend_read_property_ex(i_get_exception_base(Z_OBJ_P(object)), Z_OBJ_P(object), ZSTR_KNOWN(id), 1, &rv)

// ing3, 取得file属性
/* {{{ Get the file in which the exception occurred */
ZEND_METHOD(Exception, getFile)
{
	zval *prop, rv;
	// 不可以传入参数
	ZEND_PARSE_PARAMETERS_NONE();
	// 从this对象从获取 file属性
	prop = GET_PROPERTY(ZEND_THIS, ZEND_STR_FILE);
	// 返回字串
	RETURN_STR(zval_get_string(prop));
}
/* }}} */

// ing3, 取得line属性
/* {{{ Get the line in which the exception occurred */
ZEND_METHOD(Exception, getLine)
{
	zval *prop, rv;
	// 不可以传入参数
	ZEND_PARSE_PARAMETERS_NONE();
	// 从this对象从获取 line属性
	prop = GET_PROPERTY(ZEND_THIS, ZEND_STR_LINE);
	// 返回整数
	RETURN_LONG(zval_get_long(prop));
}
/* }}} */

// ing3, 取得message属性
/* {{{ Get the exception message */
ZEND_METHOD(Exception, getMessage)
{
	zval *prop, rv;
	// 不可以传入参数
	ZEND_PARSE_PARAMETERS_NONE();
	// 从this对象从获取 message属性
	prop = GET_PROPERTY(ZEND_THIS, ZEND_STR_MESSAGE);
	// 返回字串
	RETURN_STR(zval_get_string(prop));
}
/* }}} */

// ing3, 取得code属性
/* {{{ Get the exception code */
ZEND_METHOD(Exception, getCode)
{
	zval *prop, rv;
	// 不可以传入参数
	ZEND_PARSE_PARAMETERS_NONE();
	// 从this对象从获取 code属性
	prop = GET_PROPERTY(ZEND_THIS, ZEND_STR_CODE);
	// 减少引用次数
	ZVAL_DEREF(prop);
	// 返回 prop 的副本
	ZVAL_COPY(return_value, prop);
}
/* }}} */

// ing3, 取得trace属性
/* {{{ Get the stack trace for the location in which the exception occurred */
ZEND_METHOD(Exception, getTrace)
{
	zval *prop, rv;
	// 不可以传入参数
	ZEND_PARSE_PARAMETERS_NONE();
	// 从this对象从获取 trace属性
	prop = GET_PROPERTY(ZEND_THIS, ZEND_STR_TRACE);
	// 减少引用次数
	ZVAL_DEREF(prop);
	// 返回 prop 的副本
	ZVAL_COPY(return_value, prop);
}
/* }}} */

// ing3, 取得Severity属性
/* {{{ Get the exception severity */
ZEND_METHOD(ErrorException, getSeverity)
{
	zval *prop, rv;
	// 不可以传入参数
	ZEND_PARSE_PARAMETERS_NONE();
	// 从this对象从获取 severity属性
	prop = GET_PROPERTY(ZEND_THIS, ZEND_STR_SEVERITY);
	// 减少引用次数
	ZVAL_DEREF(prop);
	// 返回 prop 的副本
	ZVAL_COPY(return_value, prop);
}
/* }}} */

// ing4, 从哈希表里取出这个key对应的值，并追加到smart_str后面
#define TRACE_APPEND_KEY(key) do {                                          \
		/* 如果哈希表里有对应的值 */ \
		tmp = zend_hash_find(ht, key);                                      \
		if (tmp) {                                                          \
			/* 如果值不是string */ \
			if (Z_TYPE_P(tmp) != IS_STRING) {                               \
				/* 报错，这个值的类型不是字串 */ \
				zend_error(E_WARNING, "Value for %s is not a string",       \
					ZSTR_VAL(key));                                         \
				/* 追加[unknown] */ \
				smart_str_appends(str, "[unknown]");                        \
			/* 如果值是string */ \
			} else {                                                        \
				/* 追加这个string */ \
				smart_str_appends(str, Z_STRVAL_P(tmp));                    \
			}                                                               \
		} \
	} while (0)

// ing4, 把参数转成字串追加到 智能字串后面。 p1:参数, p2:智能字串
static void _build_trace_args(zval *arg, smart_str *str) /* {{{ */
{
	// 实现 convert_to_string 的繁琐方法
	// 追加它并 停止当前 临时参数，但可能引起一些提示，并对长期有影响。
	/* the trivial way would be to do
	 * convert_to_string(arg);
	 * append it and kill the now tmp arg.
	 * but that could cause some E_NOTICE and also damn long lines.
	 */

	// 减少引用次数
	ZVAL_DEREF(arg);

	// 类型：string,double,long,true,false,null,undef
	if (Z_TYPE_P(arg) <= IS_STRING) {
		// 把传入参数追加到字串后面
		smart_str_append_scalar(str, arg, EG(exception_string_param_max_len));
		// 追加： ,
		smart_str_appends(str, ", ");
	// 其他类型
	} else {
		// 根据参数类型
		switch (Z_TYPE_P(arg)) {
			// 资源 
			case IS_RESOURCE:
				// 追加：Resource id #
				smart_str_appends(str, "Resource id #");
				// 追加：资源 ID
				smart_str_append_long(str, Z_RES_HANDLE_P(arg));
				// 追加：,
				smart_str_appends(str, ", ");
				// 跳出
				break;
			// 数组
			case IS_ARRAY:
				// 追加：Array
				smart_str_appends(str, "Array, ");
				// 跳出
				break;
			// 对象
			case IS_OBJECT: {
				// 类名
				zend_string *class_name = Z_OBJ_HANDLER_P(arg, get_class_name)(Z_OBJ_P(arg));
				// 追加：Object
				smart_str_appends(str, "Object(");
				// 追加：类名
				smart_str_appends(str, ZSTR_VAL(class_name));
				// 追加：), 
				smart_str_appends(str, "), ");
				// 释放类名
				zend_string_release_ex(class_name, 0);
				// 跳出
				break;
			}
		}
	}
}
/* }}} */

// ing4, 把追踪信息构造成 字串
static void _build_trace_string(smart_str *str, HashTable *ht, uint32_t num) /* {{{ */
{
	zval *file, *tmp;

	// 追加：#
	smart_str_appendc(str, '#');
	// 追加：数字
	smart_str_append_long(str, num);
	// 追加：空格
	smart_str_appendc(str, ' ');

	// 从哈希表中获取 文件名
	file = zend_hash_find_known_hash(ht, ZSTR_KNOWN(ZEND_STR_FILE));
	// 如果获取到文件名
	if (file) {
		// 如果文件名不是字串
		if (Z_TYPE_P(file) != IS_STRING) {
			// 报错：文件名
			zend_error(E_WARNING, "File name is not a string");
			// 追加：[unknown file]: 
			smart_str_appends(str, "[unknown file]: ");
		// 其他情况
		} else{
			// 行号
			zend_long line = 0;
			// 哈希表中取得行号
			tmp = zend_hash_find_known_hash(ht, ZSTR_KNOWN(ZEND_STR_LINE));
			// 如果获取成功
			if (tmp) {
				// 如果获取到整数
				if (Z_TYPE_P(tmp) == IS_LONG) {
					// 取出整数
					line = Z_LVAL_P(tmp);
				// 其他情况
				} else {
					// 行号必须是整数
					zend_error(E_WARNING, "Line is not an int");
				}
			}
			// 追加：文件名
			smart_str_append(str, Z_STR_P(file));
			// 追加：（
			smart_str_appendc(str, '(');
			// 追加：行号
			smart_str_append_long(str, line);
			// 追加：）：
			smart_str_appends(str, "): ");
		}
	// 如果没有获取到文件名
	} else {
		// 追加：内置函数
		smart_str_appends(str, "[internal function]: ");
	}
	// 从哈希表里取出这个key对应的值，并追加到smart_str后面
	// 追加：类名
	TRACE_APPEND_KEY(ZSTR_KNOWN(ZEND_STR_CLASS));
	// 追加：类型名
	TRACE_APPEND_KEY(ZSTR_KNOWN(ZEND_STR_TYPE));
	// 追加：函数名
	TRACE_APPEND_KEY(ZSTR_KNOWN(ZEND_STR_FUNCTION));
	// 追加：（
	smart_str_appendc(str, '(');
	// 哈希表中获取参数
	tmp = zend_hash_find_known_hash(ht, ZSTR_KNOWN(ZEND_STR_ARGS));
	// 如果获取到参数
	if (tmp) {
		// 如果获取到数组
		if (Z_TYPE_P(tmp) == IS_ARRAY) {
			// 字串长度
			size_t last_len = ZSTR_LEN(str->s);
			// 名称
			zend_string *name;
			// 参数
			zval *arg;
			// 遍历参数表
			ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL_P(tmp), name, arg) {
				// 如果有名称
				if (name) {
					// 追加：名称
					smart_str_append(str, name);
					// 追加：冒号
					smart_str_appends(str, ": ");
				}
				// 把参数转成字串追加到 智能字串后面。 p1:参数, p2:智能字串
				_build_trace_args(arg, str);
			} ZEND_HASH_FOREACH_END();
			
			// 如果长度有变化 
			if (last_len != ZSTR_LEN(str->s)) {
				// 删除最后一个，
				ZSTR_LEN(str->s) -= 2; /* remove last ', ' */
			}
		// 其他情况
		} else {
			// 报错：参数元素不是数组
			zend_error(E_WARNING, "args element is not an array");
		}
	}
	// 追加：）\n
	smart_str_appends(str, ")\n");
}
/* }}} */

// ing4, 获取 trace 跟踪信息
ZEND_API zend_string *zend_trace_to_string(HashTable *trace, bool include_main) {
	zend_ulong index;
	zval *frame;
	uint32_t num = 0;
	smart_str str = {0};
	// 遍历 trace
	ZEND_HASH_FOREACH_NUM_KEY_VAL(trace, index, frame) {
		// 如果frame不是array
		if (Z_TYPE_P(frame) != IS_ARRAY) {
			// 抛错：需要array类型的 frame
			zend_error(E_WARNING, "Expected array for frame " ZEND_ULONG_FMT, index);
			continue;
		}
		// 构造 trace字串
		_build_trace_string(&str, Z_ARRVAL_P(frame), num++);
	} ZEND_HASH_FOREACH_END();
	// 如果需要 main
	if (include_main) {
		// 添加 # ，num (trace元素序号), {main}
		smart_str_appendc(&str, '#');
		smart_str_append_long(&str, num);
		smart_str_appends(&str, " {main}");
	}
	// \0
	smart_str_0(&str);
	// 如果字串有效（从trace转化的信息），使用它，如果没有，返回空字串。
	return str.s ? str.s : ZSTR_EMPTY_ALLOC();
}

// ing4, 获取 trace 跟踪信息，转成字串
/* {{{ Obtain the backtrace for the exception as a string (instead of an array) */
ZEND_METHOD(Exception, getTraceAsString)
{
	// 不可以传入参数
	ZEND_PARSE_PARAMETERS_NONE();

	zval *object = ZEND_THIS;
	// object所属类继承自 zend_ce_exception ？使用 zend_ce_exception ： zend_ce_error
	zend_class_entry *base_ce = i_get_exception_base(Z_OBJ_P(object));
	zval rv;
	// 读取对象的 trace属性
	zval *trace = zend_read_property_ex(base_ce, Z_OBJ_P(object), ZSTR_KNOWN(ZEND_STR_TRACE), 1, &rv);
	// 如果有异常，抛出异常并并返回
	if (EG(exception)) {
		RETURN_THROWS();
	}
	// 类型由 属性类型来保证
	/* Type should be guaranteed by property type. */
	ZEND_ASSERT(Z_TYPE_P(trace) == IS_ARRAY);
	// 把trace转成字串返回
	RETURN_NEW_STR(zend_trace_to_string(Z_ARRVAL_P(trace), /* include_main */ true));
}
/* }}} */

// ing4, 获取 previous 属性并返回
/* {{{ Return previous Throwable or NULL. */
ZEND_METHOD(Exception, getPrevious)
{
	zval rv;
	// 不可以传入参数
	ZEND_PARSE_PARAMETERS_NONE();
	// 获取 previous 属性并返回
	ZVAL_COPY(return_value, GET_PROPERTY_SILENT(ZEND_THIS, ZEND_STR_PREVIOUS));
} /* }}} */

// ing3, 获取代表异常（串）的字串
/* {{{ Obtain the string representation of the Exception object */
ZEND_METHOD(Exception, __toString)
{
	zval trace, *exception;
	zend_class_entry *base_ce;
	zend_string *str;
	zend_fcall_info fci;
	zval rv, tmp;
	zend_string *fname;

	ZEND_PARSE_PARAMETERS_NONE();

	str = ZSTR_EMPTY_ALLOC();

	exception = ZEND_THIS;
	// 字串 gettraceasstring
	fname = zend_string_init("gettraceasstring", sizeof("gettraceasstring")-1, 0);

	// 有异常 并且 异常是对象 并且 从属于 zend_ce_throwable
	while (exception && Z_TYPE_P(exception) == IS_OBJECT && instanceof_function(Z_OBJCE_P(exception), zend_ce_throwable)) {
		zend_string *prev_str = str;
		// 取出异常消息
		zend_string *message = zval_get_string(GET_PROPERTY(exception, ZEND_STR_MESSAGE));
		// 取出文件名
		zend_string *file = zval_get_string(GET_PROPERTY(exception, ZEND_STR_FILE));
		// 取出行号
		zend_long line = zval_get_long(GET_PROPERTY(exception, ZEND_STR_LINE));

		// 调用信息大小
		fci.size = sizeof(fci);
		// 写入文件名
		ZVAL_STR(&fci.function_name, fname);
		// 对象
		fci.object = Z_OBJ_P(exception);
		// 追踪信息
		fci.retval = &trace;
		// 参数数量 
		fci.param_count = 0;
		// 参数表
		fci.params = NULL;
		// 命名参数
		fci.named_params = NULL;

		// 调用 gettraceasstring 函数。（没找到这个函数）
		zend_call_function(&fci, NULL);

		// 如果追踪信息不是字串
		if (Z_TYPE(trace) != IS_STRING) {
			// 清空追踪信息
			zval_ptr_dtor(&trace);
			// 设置成未定义
			ZVAL_UNDEF(&trace);
		}

		// （如果异常所属类型是 zend_ce_type_error 或 zend_ce_argument_count_error）并且消息中有 ", called in "
		if ((Z_OBJCE_P(exception) == zend_ce_type_error || Z_OBJCE_P(exception) == zend_ce_argument_count_error) && strstr(ZSTR_VAL(message), ", called in ")) {
			zval message_zv;
			// 消息放进临时变量
			ZVAL_STR(&message_zv, message);
			// 重新组织消息
			zend_string *real_message = zend_strpprintf_unchecked(0, "%Z and defined", &message_zv);
			// 释放原消息字串
			zend_string_release_ex(message, 0);
			// 使用新组织的消息
			message = real_message;
		}

		// 追踪信息是字串 并且不是空 ？创建副本 ：使用 "#0 {main}\n"
		zend_string *tmp_trace = (Z_TYPE(trace) == IS_STRING && Z_STRLEN(trace))
			? zend_string_copy(Z_STR(trace))
			: ZSTR_INIT_LITERAL("#0 {main}\n", false);

		zval name_zv, trace_zv, file_zv, prev_str_zv;
		// 异常类名
		ZVAL_STR(&name_zv, Z_OBJCE_P(exception)->name);
		// 追踪信息
		ZVAL_STR(&trace_zv, tmp_trace);
		// 文件名
		ZVAL_STR(&file_zv, file);
		// 前文创建的空字串
		ZVAL_STR(&prev_str_zv, prev_str);

		// 如果消息不是空
		if (ZSTR_LEN(message) > 0) {
			zval message_zv;
			// 消息写进临时变量
			ZVAL_STR(&message_zv, message);

			// 重新组织字串
			str = zend_strpprintf_unchecked(0, "%Z: %Z in %Z:" ZEND_LONG_FMT "\nStack trace:\n%Z%s%Z",
				&name_zv, &message_zv, &file_zv, line,
				&trace_zv, ZSTR_LEN(prev_str) ? "\n\nNext " : "", &prev_str_zv);
		// 消息是空
		} else {
			// 重新组织字串
			str = zend_strpprintf_unchecked(0, "%Z in %Z:" ZEND_LONG_FMT "\nStack trace:\n%Z%s%Z",
				&name_zv, &file_zv, line,
				&trace_zv, ZSTR_LEN(prev_str) ? "\n\nNext " : "", &prev_str_zv);
		}
		// 释放 字串：tmp_trace，prev_str，message，file，
		zend_string_release_ex(tmp_trace, false);
		
		zend_string_release_ex(prev_str, 0);
		zend_string_release_ex(message, 0);
		zend_string_release_ex(file, 0);

		// 销毁追踪信息
		zval_ptr_dtor(&trace);

		// 递归保护exception
		Z_PROTECT_RECURSION_P(exception);
		// 从属性中，获取前一个 异常
		exception = GET_PROPERTY(exception, ZEND_STR_PREVIOUS);
		// 如果有前一个异常 并且它是对象 并且 有递归调用
		if (exception && Z_TYPE_P(exception) == IS_OBJECT && Z_IS_RECURSIVE_P(exception)) {
			// 跳出
			break;
		}
		// 继续循环，把异常串起来
	}
	// 释放文件名
	zend_string_release_ex(fname, 0);

	// 
	exception = ZEND_THIS;
	// 重置 应用次数
	/* Reset apply counts */
	
	// 有异常 并且 
	// 获取到基类 -> object所属类继承自 zend_ce_exception ？使用 zend_ce_exception ： zend_ce_error
	// 并且 异常属于此基类
	while (exception && Z_TYPE_P(exception) == IS_OBJECT && (base_ce = i_get_exception_base(Z_OBJ_P(exception))) && instanceof_function(Z_OBJCE_P(exception), base_ce)) {
		// 如果有 递归保护 exception
		if (Z_IS_RECURSIVE_P(exception)) {
			// 解除 递归保护
			Z_UNPROTECT_RECURSION_P(exception);
		// 如果没有递归保护
		} else {
			// 跳出
			break;
		}
		// 从属性中，取得前一个异常
		exception = GET_PROPERTY(exception, ZEND_STR_PREVIOUS);
		// 循环解除所有异常的递归保护
	}

	// 
	exception = ZEND_THIS;
	// object所属类继承自 zend_ce_exception ？使用 zend_ce_exception ： zend_ce_error
	base_ce = i_get_exception_base(Z_OBJ_P(exception));

	// 把私有属性 中的结果 保存起来，这样可以在未捕获异常处理器中 访问结果 ，并且没有内存泄露
	/* We store the result in the private property string so we can access
	 * the result in uncaught exception handlers without memleaks. */
	// 字串放进临时变量
	ZVAL_STR(&tmp, str);
	// 更新异常的 string 属性
	zend_update_property_ex(base_ce, Z_OBJ_P(exception), ZSTR_KNOWN(ZEND_STR_STRING), &tmp);
	// 返回字串 str
	RETURN_STR(str);
}
/* }}} */

// ing3, 注册默认的异常类型
void zend_register_default_exception(void) /* {{{ */
{
	// 注册接口 Throwable，实现 zend_ce_stringable接口
	zend_ce_throwable = register_class_Throwable(zend_ce_stringable);
	// 实现 throwable 接口时的检测
	zend_ce_throwable->interface_gets_implemented = zend_implement_throwable;

	// 异常的 处理方法列表 提制标准对象的 处理方法列表
	memcpy(&default_exception_handlers, &std_object_handlers, sizeof(zend_object_handlers));
	// 没有克隆方法
	default_exception_handlers.clone_obj = NULL;

	// 注册类 Exception, 实现 zend_ce_throwable 接口
	zend_ce_exception = register_class_Exception(zend_ce_throwable);
	zend_ce_exception->create_object = zend_default_exception_new;

	// 注册类 ErrorException, 继承类 zend_ce_exception
	zend_ce_error_exception = register_class_ErrorException(zend_ce_exception);
	zend_ce_error_exception->create_object = zend_error_exception_new;

	// 注册类 Error, 实现 zend_ce_throwable 接口
	zend_ce_error = register_class_Error(zend_ce_throwable);
	zend_ce_error->create_object = zend_default_exception_new;

	// 注册类 CompileError, 继承类 zend_ce_error
	zend_ce_compile_error = register_class_CompileError(zend_ce_error);
	zend_ce_compile_error->create_object = zend_default_exception_new;

	// 注册类 ParseError, 继承类 zend_ce_compile_error
	zend_ce_parse_error = register_class_ParseError(zend_ce_compile_error);
	zend_ce_parse_error->create_object = zend_default_exception_new;

	// 注册类 TypeError, 继承类 zend_ce_error
	zend_ce_type_error = register_class_TypeError(zend_ce_error);
	zend_ce_type_error->create_object = zend_default_exception_new;

	// 注册类 ArgumentCountError, 继承类 zend_ce_error
	zend_ce_argument_count_error = register_class_ArgumentCountError(zend_ce_type_error);
	zend_ce_argument_count_error->create_object = zend_default_exception_new;

	// 注册类 ValueError, 继承类 zend_ce_error
	zend_ce_value_error = register_class_ValueError(zend_ce_error);
	zend_ce_value_error->create_object = zend_default_exception_new;

	// 注册类 ArithmeticError, 继承类 zend_ce_error
	zend_ce_arithmetic_error = register_class_ArithmeticError(zend_ce_error);
	zend_ce_arithmetic_error->create_object = zend_default_exception_new;

	// 注册类 DivisionByZeroError, 继承类 zend_ce_arithmetic_error
	zend_ce_division_by_zero_error = register_class_DivisionByZeroError(zend_ce_arithmetic_error);
	zend_ce_division_by_zero_error->create_object = zend_default_exception_new;

	// 注册类 UnhandledMatchError, 继承类 zend_ce_error
	zend_ce_unhandled_match_error = register_class_UnhandledMatchError(zend_ce_error);
	zend_ce_unhandled_match_error->create_object = zend_default_exception_new;

	// 用给定的名字创建新类（复制 zend_class_entry），并添加成员方法列表
	INIT_CLASS_ENTRY(zend_ce_unwind_exit, "UnwindExit", NULL);
	// 用给定的名字创建新类（复制 zend_class_entry），并添加成员方法列表
	INIT_CLASS_ENTRY(zend_ce_graceful_exit, "GracefulExit", NULL);
}
/* }}} */

// ing4, 弃用的，直接使用 zend_ce_exception 来替代
/* {{{ Deprecated - Use zend_ce_exception directly instead */
ZEND_API zend_class_entry *zend_exception_get_default(void)
{
	return zend_ce_exception;
}
/* }}} */

// ing4, 弃用的，直接使用 zend_ce_error_exception 来替代
/* {{{ Deprecated - Use zend_ce_error_exception directly instead */
ZEND_API zend_class_entry *zend_get_error_exception(void)
{
	// 返回类 zend_ce_error_exception
	return zend_ce_error_exception;
}
/* }}} */

// ing4, 使用给出的 类型、message和code, 创建临时异常实例并抛出。
static zend_object *zend_throw_exception_zstr(zend_class_entry *exception_ce, zend_string *message, zend_long code) /* {{{ */
{
	zval ex, tmp;
	// 如果没有指定类，使用默认类
	if (!exception_ce) {
		exception_ce = zend_ce_exception;
	}
	// exception_ce 必须 实现了 zend_ce_throwable 接口
	ZEND_ASSERT(instanceof_function(exception_ce, zend_ce_throwable)
		&& "Exceptions must implement Throwable");
	// 初始化对象
	object_init_ex(&ex, exception_ce);
	// 如果有消息
	if (message) {
		// 获取 message 字串变量
		ZVAL_STR(&tmp, message);
		// 更新 ex 对象的 message 属性
		zend_update_property_ex(exception_ce, Z_OBJ(ex), ZSTR_KNOWN(ZEND_STR_MESSAGE), &tmp);
	}
	if (code) {
		// 获取 code 整数变量
		ZVAL_LONG(&tmp, code);
		// 更新 ex 对象的 code 属性
		zend_update_property_ex(exception_ce, Z_OBJ(ex), ZSTR_KNOWN(ZEND_STR_CODE), &tmp);
	}
	// 抛异常 ex
	zend_throw_exception_internal(Z_OBJ(ex));
	// 返回异常对象
	return Z_OBJ(ex);
}
/* }}} */

// ing4, 使用给出的 类型、message（char*）和code, 创建临时异常实例并抛出。
ZEND_API ZEND_COLD zend_object *zend_throw_exception(zend_class_entry *exception_ce, const char *message, zend_long code) /* {{{ */
{
	// 创建错误信息字串
	zend_string *msg_str = message ? zend_string_init(message, strlen(message), 0) : NULL;
	// 使用给出的 类型、message和code, 创建临时异常实例并抛出。
	zend_object *ex = zend_throw_exception_zstr(exception_ce, msg_str, code);
	// 如果创建了临时变量
	if (msg_str) {
		// 销毁临时变量
		zend_string_release(msg_str);
	}
	return ex;
}
/* }}} */

// ing4, 使用给出的 类型、code、格式和参数列表, 创建临时异常实例并抛出。
ZEND_API ZEND_COLD zend_object *zend_throw_exception_ex(zend_class_entry *exception_ce, zend_long code, const char *format, ...) /* {{{ */
{
	va_list arg;
	char *message;
	zend_object *obj;
	//
	va_start(arg, format);
	// 组织异常信息
	zend_vspprintf(&message, 0, format, arg);
	va_end(arg);
	// 使用给出的 类型、message（char*）和code, 创建临时异常实例并抛出。
	obj = zend_throw_exception(exception_ce, message, code);
	efree(message);
	return obj;
}
/* }}} */

// ing3, 创建异常对象，如果有 severity 属性（p1决定），更新它。p1:所属类，p2:消息，p3:错误码，p4:severity
ZEND_API ZEND_COLD zend_object *zend_throw_error_exception(zend_class_entry *exception_ce, zend_string *message, zend_long code, int severity) /* {{{ */
{
	// 使用给出的 类型、message和code, 创建临时异常实例并抛出。
	zend_object *obj = zend_throw_exception_zstr(exception_ce, message, code);
	// 如果有异常类，并且异常类继承自 zend_ce_error_exception
	if (exception_ce && instanceof_function(exception_ce, zend_ce_error_exception)) {
		// 临时变量
		zval tmp;
		// 把值写进临时变量
		ZVAL_LONG(&tmp, severity);
		// 更新异常的 severity 属性值
		// 更新对象的属性，scope更新过程中用到的作用域，p1:类入口，p2:对象，p3:属性名，p4:属性值
		zend_update_property_ex(zend_ce_error_exception, obj, ZSTR_KNOWN(ZEND_STR_SEVERITY), &tmp);
	}
	return obj;
}
/* }}} */

// ing3, 通过传入参数报错
static void zend_error_va(int type, zend_string *file, uint32_t lineno, const char *format, ...) /* {{{ */
{
	va_list args;
	//
	va_start(args, format);
	// 格式化错误信息
	zend_string *message = zend_vstrpprintf(0, format, args);
	// 依次调用每个 出错回调 函数
	zend_observer_error_notify(type, file, lineno, message);
	// main.c : php_error_cb
	// 回调，显示错误
	zend_error_cb(type, file, lineno, message);
	// 释放错误信息
	zend_string_release(message);
	va_end(args);
}
/* }}} */

// 如果使用 E_ERROR，这个函数不会return
/* This function doesn't return if it uses E_ERROR */
// ing3, 报错
ZEND_API ZEND_COLD zend_result zend_exception_error(zend_object *ex, int severity) /* {{{ */
{
	zval exception, rv;
	zend_class_entry *ce_exception;
	zend_result result = FAILURE;
	// ex放进zval里
	ZVAL_OBJ(&exception, ex);
	// 异常的类型
	ce_exception = ex->ce;
	// 清空当前异常
	EG(exception) = NULL;
	// 如果异常类型是 zend_ce_parse_error 或 zend_ce_compile_error
	if (ce_exception == zend_ce_parse_error || ce_exception == zend_ce_compile_error) {
		// 取出 exception的 message 属性，file属性，line属性
		zend_string *message = zval_get_string(GET_PROPERTY(&exception, ZEND_STR_MESSAGE));
		zend_string *file = zval_get_string(GET_PROPERTY_SILENT(&exception, ZEND_STR_FILE));
		zend_long line = zval_get_long(GET_PROPERTY_SILENT(&exception, ZEND_STR_LINE));
		
		// 表明这个这个常见的致命异常不会在 bailout（魔术方法__get,__set） 里发生。
		// （E_PARSE 或 E_COMPILE_ERROR）| E_DONT_BAIL
		int type = (ce_exception == zend_ce_parse_error ? E_PARSE : E_COMPILE_ERROR) | E_DONT_BAIL;
		
		// 依次调用每个 出错回调 函数
		zend_observer_error_notify(type, file, line, message);
		// main.c : php_error_cb
		zend_error_cb(type, file, line, message);
		// 释放文件名，消息
		zend_string_release_ex(file, 0);
		zend_string_release_ex(message, 0);
	// 异常类型不是上面这两个，但它实现了 throwable 接口
	} else if (instanceof_function(ce_exception, zend_ce_throwable)) {
		zval tmp;
		zend_string *str, *file = NULL;
		zend_long line = 0;
		// 0个参数调用 __tostring 方法，返回值 tmp
		zend_call_known_instance_method_with_0_params(ex->ce->__tostring, ex, &tmp);
		// 如果没有异常
		if (!EG(exception)) {
			// 返回值不是string
			if (Z_TYPE(tmp) != IS_STRING) {
				// 报错：__toString 必须返回string
				zend_error(E_WARNING, "%s::__toString() must return a string", ZSTR_VAL(ce_exception->name));
			// 返回值是string
			} else {
				// object所属类继承自 zend_ce_exception ？使用 zend_ce_exception ： zend_ce_error
				zend_update_property_ex(i_get_exception_base(ex), ex, ZSTR_KNOWN(ZEND_STR_STRING), &tmp);
			}
		}
		// 销毁 tmp
		zval_ptr_dtor(&tmp);
		
		// 如果有异常
		if (EG(exception)) {
			zval zv;
			// 异常关联到 zval
			ZVAL_OBJ(&zv, EG(exception));
			// 尽量做好内部异常的通知
			/* do the best we can to inform about the inner exception */
			// 异常属于 zend_ce_exception 类或 zend_ce_error 类
			if (instanceof_function(ce_exception, zend_ce_exception) || instanceof_function(ce_exception, zend_ce_error)) {
				// 文件名，获取异常的 file 属性，静默
				file = zval_get_string(GET_PROPERTY_SILENT(&zv, ZEND_STR_FILE));
				// 行号，获取异常的 line 属性，静默
				line = zval_get_long(GET_PROPERTY_SILENT(&zv, ZEND_STR_LINE));
			}
			// 抛出警告：在调用 __toString 方法时出错 
			zend_error_va(E_WARNING, (file && ZSTR_LEN(file) > 0) ? file : NULL, line,
				"Uncaught %s in exception handling during call to %s::__toString()",
				ZSTR_VAL(Z_OBJCE(zv)->name), ZSTR_VAL(ce_exception->name));
			// 如果文件名有效
			if (file) {
				// 释放它
				zend_string_release_ex(file, 0);
			}
		}
		// 静默取出 exception 的 string 属性
		str = zval_get_string(GET_PROPERTY_SILENT(&exception, ZEND_STR_STRING));
		// 静默取出 exception 的 file 属性
		file = zval_get_string(GET_PROPERTY_SILENT(&exception, ZEND_STR_FILE));
		// 静默取出 exception 的 line 属性
		line = zval_get_long(GET_PROPERTY_SILENT(&exception, ZEND_STR_LINE));

		// 报错：未捕获的异常
		zend_error_va(severity | E_DONT_BAIL,
			(file && ZSTR_LEN(file) > 0) ? file : NULL, line,
			"Uncaught %s\n  thrown", ZSTR_VAL(str));
		// 释放信息和文件名
		zend_string_release_ex(str, 0);
		zend_string_release_ex(file, 0);
	// 如果异常属于 zend_ce_unwind_exit 或 zend_ce_graceful_exit 类
	} else if (ce_exception == &zend_ce_unwind_exit || ce_exception == &zend_ce_graceful_exit) {
		// 已经成功复位，没什么需要做的了
		// 这里还是要返回失败，并中断其他执行
		/* We successfully unwound, nothing more to do.
		 * We still return FAILURE in this case, as further execution should still be aborted. */
	// 其他情况
	} else {
		// 未捕获的error
		zend_error(severity, "Uncaught exception %s", ZSTR_VAL(ce_exception->name));
	}
	// 释放对象
	OBJ_RELEASE(ex);
	return result;
}
/* }}} */

// ing3, 抛出未捕获的异常
ZEND_NORETURN void zend_exception_uncaught_error(const char *format, ...) {
	va_list va;
	//
	va_start(va, format);
	// 前缀内容
	zend_string *prefix = zend_vstrpprintf(0, format, va);
	va_end(va);
	// exception要存在
	ZEND_ASSERT(EG(exception));
	zval exception_zv;
	// 复制异常对象
	ZVAL_OBJ_COPY(&exception_zv, EG(exception));
	// 清除异常
	zend_clear_exception();
	// 取得异常信息
	zend_string *exception_str = zval_get_string(&exception_zv);
	// 报错 ,未捕获的error
	zend_error_noreturn(E_ERROR,
		"%s: Uncaught %s", ZSTR_VAL(prefix), ZSTR_VAL(exception_str));
}

// ing3, 抛出异常实例
ZEND_API ZEND_COLD void zend_throw_exception_object(zval *exception) /* {{{ */
{
	// 如果没有传入exception 或 它不是对象
	if (exception == NULL || Z_TYPE_P(exception) != IS_OBJECT) {
		// 报错：抛出异常时必须是对象
		zend_error_noreturn(E_CORE_ERROR, "Need to supply an object when throwing an exception");
	}
	// 找到所属类
	zend_class_entry *exception_ce = Z_OBJCE_P(exception);
	// 如果没有所属类 或 没有实现 zend_ce_throwable
	if (!exception_ce || !instanceof_function(exception_ce, zend_ce_throwable)) {
		// 不能抛出没有实现 Throwable 接口的对象
		zend_throw_error(NULL, "Cannot throw objects that do not implement Throwable");
		// 删除对象
		zval_ptr_dtor(exception);
		return;
	}
	// 内部抛异常
	zend_throw_exception_internal(Z_OBJ_P(exception));
}
/* }}} */

// ing4, 创建并返回， zend_ce_unwind_exit 实例
ZEND_API ZEND_COLD zend_object *zend_create_unwind_exit(void)
{
	return zend_objects_new(&zend_ce_unwind_exit);
}

// ing4, 创建并返回， zend_ce_graceful_exit 实例
ZEND_API ZEND_COLD zend_object *zend_create_graceful_exit(void)
{
	return zend_objects_new(&zend_ce_graceful_exit);
}

// ing4, 创建 zend_ce_unwind_exit 实例作为异常
ZEND_API ZEND_COLD void zend_throw_unwind_exit(void)
{
	ZEND_ASSERT(!EG(exception));
	// 创建 zend_ce_unwind_exit 实例
	EG(exception) = zend_create_unwind_exit();
	// 异常前操作码
	EG(opline_before_exception) = EG(current_execute_data)->opline;
	// 异常行号
	EG(current_execute_data)->opline = EG(exception_op);
}

// ing4, 创建 zend_ce_graceful_exit 实例作为异常
ZEND_API ZEND_COLD void zend_throw_graceful_exit(void)
{
	ZEND_ASSERT(!EG(exception));
	// 创建 zend_ce_graceful_exit 实例
	EG(exception) = zend_create_graceful_exit();
	// 异常前操作码
	EG(opline_before_exception) = EG(current_execute_data)->opline;
	// 异常行号
	EG(current_execute_data)->opline = EG(exception_op);
}

// ing4, 验证 ex->ce 是否是 zend_ce_unwind_exit 类型
ZEND_API bool zend_is_unwind_exit(const zend_object *ex)
{
	return ex->ce == &zend_ce_unwind_exit;
}

// ing4, 验证 ex->ce 是否是 zend_ce_graceful_exit 类型
ZEND_API bool zend_is_graceful_exit(const zend_object *ex)
{
	return ex->ce == &zend_ce_graceful_exit;
}
