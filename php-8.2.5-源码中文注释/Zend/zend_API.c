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
   |          Zeev Suraski <zeev@php.net>                                 |
   |          Andrei Zmievski <andrei@php.net>                            |
   |          Dmitry Stogov <dmitry@php.net>                              |
   +----------------------------------------------------------------------+
*/


#include "zend.h"
#include "zend_execute.h"
#include "zend_API.h"
#include "zend_modules.h"
#include "zend_extensions.h"
#include "zend_constants.h"
#include "zend_interfaces.h"
#include "zend_exceptions.h"
#include "zend_closures.h"
#include "zend_inheritance.h"
#include "zend_ini.h"
#include "zend_enum.h"
#include "zend_observer.h"

#include <stdarg.h>

// 这些变量是 静态 或 全局的， 必须在每次访问中 互斥（mutex'ed）
/* these variables are true statics/globals, and have to be mutex'ed on every access */

// 注册的模块
ZEND_API HashTable module_registry;

// 4个都是模块指针列表。触发器列表
static zend_module_entry **module_request_startup_handlers;
static zend_module_entry **module_request_shutdown_handlers;
static zend_module_entry **module_post_deactivate_handlers;
static zend_class_entry  **class_cleanup_handlers;


// ing3, 把形式参数添加到实际参数列表（原生数组）中去。p1:形式参数数量 ，p2:实际参数列表
ZEND_API zend_result zend_get_parameters_array_ex(uint32_t param_count, zval *argument_array) /* {{{ */
{
	zval *param_ptr;
	uint32_t arg_count;

	// #define EX_VAR_NUM(n) ZEND_CALL_VAR_NUM(execute_data, n)
	// ZEND_CALL_VAR_NUM(call, ((int)(n)) - 1)
	// 形式参数列表
	param_ptr = ZEND_CALL_ARG(EG(current_execute_data), 1);
	// 实际参数数量
	arg_count = ZEND_CALL_NUM_ARGS(EG(current_execute_data));

	// 如果形式参数数量 大于 实际参数数量 
	if (param_count>arg_count) {
		// 返回错误
		return FAILURE;
	}
	// 遍历形式参数
	while (param_count-->0) {
		// 把形式参数复制到实际参数列表中
		ZVAL_COPY_VALUE(argument_array, param_ptr);
		// 下一个实际参数
		argument_array++;
		// 下一个形式参数
		param_ptr++;
	}

	return SUCCESS;
}
/* }}} */

// ing3, 把形式参数添加到实际参数列表（哈希表）中去。p1:形式参数数量 ，p2:实际参数列表
ZEND_API zend_result zend_copy_parameters_array(uint32_t param_count, zval *argument_array) /* {{{ */
{
	zval *param_ptr;
	uint32_t arg_count;

	// #define EX_VAR_NUM(n) ZEND_CALL_VAR_NUM(execute_data, n)
	// ZEND_CALL_VAR_NUM(call, ((int)(n)) - 1)
	// 形式参数列表
	param_ptr = ZEND_CALL_ARG(EG(current_execute_data), 1);
	// 实际参数数量
	arg_count = ZEND_CALL_NUM_ARGS(EG(current_execute_data));
	// 如果形式参数数量 大于 实际参数数量 
	if (param_count>arg_count) {
		// 返回错误
		return FAILURE;
	}

	// 遍历形式参数
	while (param_count-->0) {
		// 引用数+1
		Z_TRY_ADDREF_P(param_ptr);
		// 添加到实际参数列表中
		zend_hash_next_index_insert_new(Z_ARRVAL_P(argument_array), param_ptr);
		// 下一个形式参数
		param_ptr++;
	}

	return SUCCESS;
}
/* }}} */

// ing3, 报错参数数量错误
ZEND_API ZEND_COLD void zend_wrong_param_count(void) /* {{{ */
{
	const char *space;
	const char *class_name = get_active_class_name(&space);

	zend_argument_count_error("Wrong parameter count for %s%s%s()", class_name, space, get_active_function_name());
}
/* }}} */

// ing3, 抛错，读取对象的属性出错
ZEND_API ZEND_COLD void zend_wrong_property_read(zval *object, zval *property)
{
	zend_string *tmp_property_name;
	// 属性名临时字串
	zend_string *property_name = zval_get_tmp_string(property, &tmp_property_name);
	// 错误：尝试读取 A 上的属性 B
	zend_error(E_WARNING, "Attempt to read property \"%s\" on %s", ZSTR_VAL(property_name), zend_zval_type_name(object));
	// 释放临时属性名
	zend_tmp_string_release(tmp_property_name);
}

/* Argument parsing API -- andrei */
// clear, 把类型常量转成字串
ZEND_API const char *zend_get_type_by_const(int type) /* {{{ */
{
	switch(type) {
		case IS_FALSE:
		case IS_TRUE:
		case _IS_BOOL:
			return "bool";
		case IS_LONG:
			return "int";
		case IS_DOUBLE:
			return "float";
		case IS_STRING:
			return "string";
		case IS_OBJECT:
			return "object";
		case IS_RESOURCE:
			return "resource";
		case IS_NULL:
			return "null";
		case IS_CALLABLE:
			return "callable";
		case IS_ITERABLE:
			return "iterable";
		case IS_ARRAY:
			return "array";
		case IS_VOID:
			return "void";
		case IS_MIXED:
			return "mixed";
		case _IS_NUMBER:
			return "number";
		EMPTY_SWITCH_DEFAULT_CASE()
	}
}
/* }}} */

// ing4, 取得类型名字串
ZEND_API const char *zend_zval_type_name(const zval *arg) /* {{{ */
{
	ZVAL_DEREF(arg);

	if (Z_ISUNDEF_P(arg)) {
		return "null";
	}

	if (Z_TYPE_P(arg) == IS_OBJECT) {
		return ZSTR_VAL(Z_OBJCE_P(arg)->name);
	}

	return zend_get_type_by_const(Z_TYPE_P(arg));
}
/* }}} */

// 这个api只给 gettype使用。其他地方使用 zend_zval_type_name
/* This API exists *only* for use in gettype().
 * For anything else, you likely want zend_zval_type_name(). */
// ing4, 根据参数的类型返回类型字串
ZEND_API zend_string *zend_zval_get_legacy_type(const zval *arg) /* {{{ */
{
	switch (Z_TYPE_P(arg)) {
		case IS_NULL:
			return ZSTR_KNOWN(ZEND_STR_NULL);
		case IS_FALSE:
		case IS_TRUE:
			return ZSTR_KNOWN(ZEND_STR_BOOLEAN);
		case IS_LONG:
			return ZSTR_KNOWN(ZEND_STR_INTEGER);
		case IS_DOUBLE:
			return ZSTR_KNOWN(ZEND_STR_DOUBLE);
		case IS_STRING:
			return ZSTR_KNOWN(ZEND_STR_STRING);
		case IS_ARRAY:
			return ZSTR_KNOWN(ZEND_STR_ARRAY);
		case IS_OBJECT:
			return ZSTR_KNOWN(ZEND_STR_OBJECT);
		case IS_RESOURCE:
			// 取得资源列表类型，成功返回 “resource”
			if (zend_rsrc_list_get_rsrc_type(Z_RES_P(arg))) {
				return ZSTR_KNOWN(ZEND_STR_RESOURCE);
			// 失败返回 “resource (closed)”
			} else {
				return ZSTR_KNOWN(ZEND_STR_CLOSED_RESOURCE);
			}
		default:
			return NULL;
	}
}
/* }}} */

// ing4, 报错：此方法必须0个参数
ZEND_API ZEND_COLD void ZEND_FASTCALL zend_wrong_parameters_none_error(void) /* {{{ */
{
	int num_args = ZEND_CALL_NUM_ARGS(EG(current_execute_data));
	// 取得函数或方法名
	zend_string *func_name = get_active_function_or_method_name();
	// 此方法必须0个参数，但给了n个
	zend_argument_count_error("%s() expects exactly 0 arguments, %d given", ZSTR_VAL(func_name), num_args);
	// 释放方法名
	zend_string_release(func_name);
}
/* }}} */

// ing3, 报错： 参数数量不匹配
ZEND_API ZEND_COLD void ZEND_FASTCALL zend_wrong_parameters_count_error(uint32_t min_num_args, uint32_t max_num_args) /* {{{ */
{
	// 当前执行数据中的参数数量
	uint32_t num_args = ZEND_CALL_NUM_ARGS(EG(current_execute_data));
	// 取得函数或方法名
	zend_string *func_name = get_active_function_or_method_name();
	// 报错 ： 参数数量不匹配
	zend_argument_count_error(
		"%s() expects %s %d argument%s, %d given",
		ZSTR_VAL(func_name),
		min_num_args == max_num_args ? "exactly" : num_args < min_num_args ? "at least" : "at most",
		num_args < min_num_args ? min_num_args : max_num_args,
		(num_args < min_num_args ? min_num_args : max_num_args) == 1 ? "" : "s",
		num_args
	);

	zend_string_release(func_name);
}
/* }}} */

// ing3, 解析参数错误，根据错误码转发到处理方法
ZEND_API ZEND_COLD void ZEND_FASTCALL zend_wrong_parameter_error(int error_code, uint32_t num, char *name, zend_expected_type expected_type, zval *arg) /* {{{ */
{
	switch (error_code) {
		// 
		case ZPP_ERROR_WRONG_CALLBACK:
			// 报错：参数必须是有效闭包
			zend_wrong_callback_error(num, name);
			break;
		case ZPP_ERROR_WRONG_CALLBACK_OR_NULL:
			// 报错：参数必须是有效闭包 或 null
			zend_wrong_callback_or_null_error(num, name);
			break;
		case ZPP_ERROR_WRONG_CLASS:
			// 报错：参数必须是 此类型，p1:错误码，p2:类型名，p3:参数值
			zend_wrong_parameter_class_error(num, name, arg);
			break;
		case ZPP_ERROR_WRONG_CLASS_OR_NULL:
			// 报错：参数必须是 此类型 或 null，p1:错误码，p2:类型名，p3:参数值
			zend_wrong_parameter_class_or_null_error(num, name, arg);
			break;
		case ZPP_ERROR_WRONG_CLASS_OR_STRING:
			// 报错：参数必须是 此类型 或 字串 ，p1:错误码，p2:类型名，p3:参数值
			zend_wrong_parameter_class_or_string_error(num, name, arg);
			break;
		case ZPP_ERROR_WRONG_CLASS_OR_STRING_OR_NULL:
			// 报错：参数必须是 此类型 或 字串 或 null，p1:错误码，p2:类型名，p3:参数值
			zend_wrong_parameter_class_or_string_or_null_error(num, name, arg);
			break;
		case ZPP_ERROR_WRONG_CLASS_OR_LONG:
			// 报错：参数必须是 此类型 或 整数，p1:错误码，p2:类型名，p3:参数值
			zend_wrong_parameter_class_or_long_error(num, name, arg);
			break;
		case ZPP_ERROR_WRONG_CLASS_OR_LONG_OR_NULL:
			// 报错：参数必须是 此类型 或 整数 或 null，p1:错误码，p2:类型名，p3:参数值
			zend_wrong_parameter_class_or_long_or_null_error(num, name, arg);
			break;
		case ZPP_ERROR_WRONG_ARG:
			// 报错：不是需要的类型
			zend_wrong_parameter_type_error(num, expected_type, arg);
			break;
		case ZPP_ERROR_UNEXPECTED_EXTRA_NAMED:
			// 报错 不允许未知的命名参数
			zend_unexpected_extra_named_error();
			break;
		// 参数 解析错误
		case ZPP_ERROR_FAILURE:
			// 应该已经有了一个 异常
			ZEND_ASSERT(EG(exception) && "Should have produced an error already");
			break;
		EMPTY_SWITCH_DEFAULT_CASE()
	}
}
/* }}} */

// ing3, 报错：不是需要的类型
ZEND_API ZEND_COLD void ZEND_FASTCALL zend_wrong_parameter_type_error(uint32_t num, zend_expected_type expected_type, zval *arg) /* {{{ */
{
	// 所有类型字串，只保留类型的英文名
	static const char * const expected_error[] = {
		Z_EXPECTED_TYPES(Z_EXPECTED_TYPE_STR)
		NULL
	};

	// 如果有异常，返回
	if (EG(exception)) {
		return;
	}

	// 如果 预期类型是 Z_EXPECTED_PATH 或 Z_EXPECTED_PATH_OR_NULL （不包含null的字串）
	if ((expected_type == Z_EXPECTED_PATH || expected_type == Z_EXPECTED_PATH_OR_NULL)
			&& Z_TYPE_P(arg) == IS_STRING) {
		// 报错：不可以包含空字符
		zend_argument_value_error(num, "must not contain any null bytes");
		// 
		return;
	}
	// 报错（类型：zend_ce_type_error）
	zend_argument_type_error(num, "must be %s, %s given", expected_error[expected_type], zend_zval_type_name(arg));
}
/* }}} */

// ing4, 报错：参数必须是 此类型，p1:错误码，p2:类型名，p3:参数值
ZEND_API ZEND_COLD void ZEND_FASTCALL zend_wrong_parameter_class_error(uint32_t num, const char *name, zval *arg) /* {{{ */
{
	if (EG(exception)) {
		return;
	}
	// 报错（类型：zend_ce_type_error）
	zend_argument_type_error(num, "must be of type %s, %s given", name, zend_zval_type_name(arg));
}
/* }}} */

// ing4, 报错：参数必须是 此类型 或 null，p1:错误码，p2:类型名，p3:参数值
ZEND_API ZEND_COLD void ZEND_FASTCALL zend_wrong_parameter_class_or_null_error(uint32_t num, const char *name, zval *arg) /* {{{ */
{
	if (EG(exception)) {
		return;
	}
	// 报错（类型：zend_ce_type_error）
	zend_argument_type_error(num, "must be of type ?%s, %s given", name, zend_zval_type_name(arg));
}
/* }}} */

// ing4, 报错：参数必须是 此类型 或 整数，p1:错误码，p2:类型名，p3:参数值
ZEND_API ZEND_COLD void ZEND_FASTCALL zend_wrong_parameter_class_or_long_error(uint32_t num, const char *name, zval *arg) /* {{{ */
{
	if (EG(exception)) {
		return;
	}
	// 报错（类型：zend_ce_type_error）
	zend_argument_type_error(num, "must be of type %s|int, %s given", name, zend_zval_type_name(arg));
}
/* }}} */

// ing4, 报错：参数必须是 此类型 或 整数 或 null，p1:错误码，p2:类型名，p3:参数值
ZEND_API ZEND_COLD void ZEND_FASTCALL zend_wrong_parameter_class_or_long_or_null_error(uint32_t num, const char *name, zval *arg) /* {{{ */
{
	if (EG(exception)) {
		return;
	}
	// 报错（类型：zend_ce_type_error）
	zend_argument_type_error(num, "must be of type %s|int|null, %s given", name, zend_zval_type_name(arg));
}
/* }}} */

// ing4, 报错：参数必须是 此类型 或 字串 ，p1:错误码，p2:类型名，p3:参数值
ZEND_API ZEND_COLD void ZEND_FASTCALL zend_wrong_parameter_class_or_string_error(uint32_t num, const char *name, zval *arg) /* {{{ */
{
	if (EG(exception)) {
		return;
	}
	// 报错（类型：zend_ce_type_error）
	zend_argument_type_error(num, "must be of type %s|string, %s given", name, zend_zval_type_name(arg));
}
/* }}} */

// ing4, 报错：参数必须是 此类型 或 字串 或 null，p1:错误码，p2:类型名，p3:参数值
ZEND_API ZEND_COLD void ZEND_FASTCALL zend_wrong_parameter_class_or_string_or_null_error(uint32_t num, const char *name, zval *arg) /* {{{ */
{
	if (EG(exception)) {
		return;
	}
	// 报错（类型：zend_ce_type_error）
	zend_argument_type_error(num, "must be of type %s|string|null, %s given", name, zend_zval_type_name(arg));
}
/* }}} */

// ing4, 报错：参数必须是有效闭包
ZEND_API ZEND_COLD void ZEND_FASTCALL zend_wrong_callback_error(uint32_t num, char *error) /* {{{ */
{
	if (!EG(exception)) {
		// 报错（类型：zend_ce_type_error）
		zend_argument_type_error(num, "must be a valid callback, %s", error);
	}
	efree(error);
}
/* }}} */

// ing4, 报错：参数必须是有效闭包 或 null
ZEND_API ZEND_COLD void ZEND_FASTCALL zend_wrong_callback_or_null_error(uint32_t num, char *error) /* {{{ */
{
	if (!EG(exception)) {
		// 报错（类型：zend_ce_type_error）
		zend_argument_type_error(num, "must be a valid callback or null, %s", error);
	}
	efree(error);
}
/* }}} */

// ing3, 报错 不允许未知的命名参数
ZEND_API ZEND_COLD void ZEND_FASTCALL zend_unexpected_extra_named_error(void)
{
	const char *space;
	const char *class_name = get_active_class_name(&space);
	zend_argument_count_error("%s%s%s() does not accept unknown named parameters",
		class_name, space, get_active_function_name());
}

// ing3, 报错，字典参数出错 
ZEND_API ZEND_COLD void ZEND_FASTCALL zend_argument_error_variadic(zend_class_entry *error_ce, uint32_t arg_num, const char *format, va_list va) /* {{{ */
{
	zend_string *func_name;
	const char *arg_name;
	char *message = NULL;
	// 没有异常，直接返回
	if (EG(exception)) {
		return;
	}
	// 方法名
	func_name = get_active_function_or_method_name();
	// 参数名
	arg_name = get_active_function_arg_name(arg_num);
	// 组织报错信息
	zend_vspprintf(&message, 0, format, va);
	// 抛错
	zend_throw_error(error_ce, "%s(): Argument #%d%s%s%s %s",
		ZSTR_VAL(func_name), arg_num,
		arg_name ? " ($" : "", arg_name ? arg_name : "", arg_name ? ")" : "", message
	);
	// 释放错误信息
	efree(message);
	// 释放函数名
	zend_string_release(func_name);
}
/* }}} */

// ing3, 报错（类型：error_ce）
ZEND_API ZEND_COLD void zend_argument_error(zend_class_entry *error_ce, uint32_t arg_num, const char *format, ...) /* {{{ */
{
	va_list va;

	va_start(va, format);
	// 报错，字典参数出错 
	zend_argument_error_variadic(error_ce, arg_num, format, va);
	va_end(va);
}
/* }}} */

// ing3, 报错（类型：zend_ce_type_error）
ZEND_API ZEND_COLD void zend_argument_type_error(uint32_t arg_num, const char *format, ...) /* {{{ */
{
	va_list va;

	va_start(va, format);
	// 报错，字典参数出错 
	zend_argument_error_variadic(zend_ce_type_error, arg_num, format, va);
	va_end(va);
}
/* }}} */

// ing3, 报错（类型：zend_ce_value_error）
ZEND_API ZEND_COLD void zend_argument_value_error(uint32_t arg_num, const char *format, ...) /* {{{ */
{
	va_list va;

	va_start(va, format);
	// 报错，字典参数出错 
	zend_argument_error_variadic(zend_ce_value_error, arg_num, format, va);
	va_end(va);
}
/* }}} */

// ing3, 把参数（字串）解析成类入口，p2:要求的基类，也是返回值，p3:arg参数序号，报错用，p4:是否检查 null
ZEND_API bool ZEND_FASTCALL zend_parse_arg_class(zval *arg, zend_class_entry **pce, uint32_t num, bool check_null) /* {{{ */
{
	zend_class_entry *ce_base = *pce;
	// 允null，并且遇到null
	if (check_null && Z_TYPE_P(arg) == IS_NULL) {
		// 返回值为null
		*pce = NULL;
		// 返回真
		return 1;
	}
	// 转成字串失败
	if (!try_convert_to_string(arg)) {
		// 返回值为null
		*pce = NULL;
		// 返回 假
		return 0;
	}
	// 查找类
	*pce = zend_lookup_class(Z_STR_P(arg));
	// 如果有指定基类
	if (ce_base) {
		// 如果没找到目标类 或 目标类不是基类的子类
		if ((!*pce || !instanceof_function(*pce, ce_base))) {
			// 报错：必须要是从 ce_base 衍生的类
			zend_argument_type_error(num, "must be a class name derived from %s, %s given", ZSTR_VAL(ce_base->name), Z_STRVAL_P(arg));
			// 返回值null
			*pce = NULL;
			// 返回假
			return 0;
		}
	}
	// 如果没有找到目标类
	if (!*pce) {
		// 报错：目标类必须是有效类
		zend_argument_type_error(num, "must be a valid class name, %s given", Z_STRVAL_P(arg));
		return 0;
	}
	// 返回真
	return 1;
}
/* }}} */

// ing3, 报错：不可以给规定类型的参数赋值为null。返回 !EG(exception)
static ZEND_COLD bool zend_null_arg_deprecated(const char *fallback_type, uint32_t arg_num) {
	// 取得上下文中的函数
	zend_function *func = EG(current_execute_data)->func;
	// 必须要有参数
	ZEND_ASSERT(arg_num > 0);
	// 顺序号
	uint32_t arg_offset = arg_num - 1;
	// 如果顺序号大于等于函数的参数数量
	if (arg_offset >= func->common.num_args) {
		// 函数必须支持字典参数
		ZEND_ASSERT(func->common.fn_flags & ZEND_ACC_VARIADIC);
		// 顺序号：最后一个参数
		arg_offset = func->common.num_args;
	}
	// 参数信息
	zend_arg_info *arg_info = &func->common.arg_info[arg_offset];
	// 从函数或方法实例中获取名称
	zend_string *func_name = get_active_function_or_method_name();
	// 取得当前执行的函数的第n个参数的名称
	const char *arg_name = get_active_function_arg_name(arg_num);

	// 如果参数信息中没指定type, 使用指定的返回类型
	/* If no type is specified in arginfo, use the specified fallback_type determined through
	 * zend_parse_parameters instead. */
	 
	// 获取类型的名称
	zend_string *type_str = zend_type_to_string(arg_info->type);
	// 如果有名称 使用名称字串，否则使用返回类型
	const char *type = type_str ? ZSTR_VAL(type_str) : fallback_type;
	// 报错: 对此类型的参数 赋值为null ，已弃用
	zend_error(E_DEPRECATED,
		"%s(): Passing null to parameter #%" PRIu32 "%s%s%s of type %s is deprecated",
		ZSTR_VAL(func_name), arg_num,
		arg_name ? " ($" : "", arg_name ? arg_name : "", arg_name ? ")" : "",
		type);
	// 释放参数名
	zend_string_release(func_name);
	// 如果有类型字串
	if (type_str) {
		// 释放类型字串
		zend_string_release(type_str);
	}
	// 有exception 返回false,否则 true
	return !EG(exception);
}

// ing3, 获取第n个 bool 类型的值，p2:返回值，p3:arg参数序号，报错用
ZEND_API bool ZEND_FASTCALL zend_parse_arg_bool_weak(zval *arg, bool *dest, uint32_t arg_num) /* {{{ */
{
	// 如果类型是undef,null,true,false,long,double,string
	if (EXPECTED(Z_TYPE_P(arg) <= IS_STRING)) {
		// 如果值为null, 并且有exception
		// 报错：不可以给规定类型的参数赋值为null。返回 !EG(exception)
		if (UNEXPECTED(Z_TYPE_P(arg) == IS_NULL) && !zend_null_arg_deprecated("bool", arg_num)) {
			// false
			return 0;
		}
		// 返回转转换为bool型
		*dest = zend_is_true(arg);
	// 其他类型，返回false
	} else {
		return 0;
	}
	// 返回true
	return 1;
}
/* }}} */

// ing3, （严格类型返回0）获取第n个 bool 类型的值，p2:返回值，p3:arg参数序号，报错用
ZEND_API bool ZEND_FASTCALL zend_parse_arg_bool_slow(zval *arg, bool *dest, uint32_t arg_num) /* {{{ */
{
	if (UNEXPECTED(ZEND_ARG_USES_STRICT_TYPES())) {
		return 0;
	}
	return zend_parse_arg_bool_weak(arg, dest, arg_num);
}
/* }}} */

// ing3, 把传入的参数转换成整数，遇到null会报错。p2:返回值，p3:arg参数序号，报错用
ZEND_API bool ZEND_FASTCALL zend_parse_arg_long_weak(zval *arg, zend_long *dest, uint32_t arg_num) /* {{{ */
{
	// 如果参数类型是小数
	if (EXPECTED(Z_TYPE_P(arg) == IS_DOUBLE)) {
		// 如果未传入参数 ??? vs里没这个东西
		// # define zend_isnan std::isnan
		if (UNEXPECTED(zend_isnan(Z_DVAL_P(arg)))) {
			// 返回 false
			return 0;
		}
		
		// 如果小数不能转成整数 
		if (UNEXPECTED(!ZEND_DOUBLE_FITS_LONG(Z_DVAL_P(arg)))) {
			// 返回 false
			return 0;
		// 如果能转成整数 
		} else {
			// 转成整数 
			zend_long lval = zend_dval_to_lval(Z_DVAL_P(arg));
			// 如果转成整数后值未改变
			if (UNEXPECTED(!zend_is_long_compatible(Z_DVAL_P(arg), lval))) {
				/* Check arg_num is not (uint32_t)-1, as otherwise its called by
				 * zend_verify_weak_scalar_type_hint_no_sideeffect() */
				// arg_num 不是有效
				if (arg_num != (uint32_t)-1) {
					zend_incompatible_double_to_long_error(Z_DVAL_P(arg));
				}
				// arg_num 无效
				// 如果有exception 返回 false
				if (UNEXPECTED(EG(exception))) {
					return 0;
				}
			}
			// 返回目标是转成的整数 
			*dest = lval;
		}
	// 如果参数类型是字串
	} else if (EXPECTED(Z_TYPE_P(arg) == IS_STRING)) {
		double d;
		zend_uchar type;
		// 验证是否是 数字字串。接收转换后的值，
		// 如果结果不是整数
		if (UNEXPECTED((type = is_numeric_str_function(Z_STR_P(arg), dest, &d)) != IS_LONG)) {
			// 如果返回类型有效 ，当成小数处理
			if (EXPECTED(type != 0)) {
				zend_long lval;
				// 调用底层的 isnan 函数，如果是null返回 false
				if (UNEXPECTED(zend_isnan(d))) {
					return 0;
				}
				// 如果是小数并且转成整数值会变，返回 false
				if (UNEXPECTED(!ZEND_DOUBLE_FITS_LONG(d))) {
					return 0;
				}
				// 把小数转成整数 
				lval = zend_dval_to_lval(d);
				// 只检查分部
				/* This only checks for a fractional part as if doesn't fit it already throws a TypeError */
				// 如果小数和转换后的整数不兼容
				if (UNEXPECTED(!zend_is_long_compatible(d, lval))) {
					// 检测 arg_num 不是 -1（32个1），否则调用 zend_verify_weak_scalar_type_hint_no_sideeffect
					/* Check arg_num is not (uint32_t)-1, as otherwise its called by
					 * zend_verify_weak_scalar_type_hint_no_sideeffect() */
					 
					// 参数数量不是 -1
					if (arg_num != (uint32_t)-1) {
						// 报错：隐式转换，从小数字串转成整数会丢失精度
						zend_incompatible_string_to_long_error(Z_STR_P(arg));
					}
					// 如果有异常
					if (UNEXPECTED(EG(exception))) {
						// 返回 false
						return 0;
					}
				}
				// 转换成功，返回整数 
				*dest = lval;
			// 返回类型无效，返回 false
			} else {
				return 0;
			}
		}
		// 如果有异常，返回0
		if (UNEXPECTED(EG(exception))) {
			return 0;
		}
	// 如果参数类型是：null、false、undef
	} else if (EXPECTED(Z_TYPE_P(arg) < IS_TRUE)) {
		// 报错：不可以给规定类型的参数赋值为null。返回 !EG(exception)
		if (UNEXPECTED(Z_TYPE_P(arg) == IS_NULL) && !zend_null_arg_deprecated("int", arg_num)) {
			return 0;
		}
		// 返回目标是 0
		*dest = 0;
	// 如果参数类型null 或 false	
	} else if (EXPECTED(Z_TYPE_P(arg) == IS_TRUE)) {
		// 返回目标是 1
		*dest = 1;
	// 其他情况返回false
	} else {
		return 0;
	}
	// 返回true
	return 1;
}
/* }}} */

// ing4, (严格类型返回0)把传入的参数转换成整数，遇到null会报错。p2:返回值，p3:arg参数序号，报错用
ZEND_API bool ZEND_FASTCALL zend_parse_arg_long_slow(zval *arg, zend_long *dest, uint32_t arg_num) /* {{{ */
{
	if (UNEXPECTED(ZEND_ARG_USES_STRICT_TYPES())) {
		return 0;
	}
	// 把传入的参数转换成整数，遇到null会报错。p2:返回值，p3:arg参数序号，报错用
	return zend_parse_arg_long_weak(arg, dest, arg_num);
}
/* }}} */

// ing4, 把传入的参数转换成小数，遇到null会报错。p2:返回值，p3:arg参数序号，报错用
ZEND_API bool ZEND_FASTCALL zend_parse_arg_double_weak(zval *arg, double *dest, uint32_t arg_num) /* {{{ */
{
	// 如果传入值为整数
	if (EXPECTED(Z_TYPE_P(arg) == IS_LONG)) {
		// 直接转成小数
		*dest = (double)Z_LVAL_P(arg);
	// 如果传入值是字串
	} else if (EXPECTED(Z_TYPE_P(arg) == IS_STRING)) {
		zend_long l;
		zend_uchar type;
		// 如果转成数值后不是小数
		if (UNEXPECTED((type = is_numeric_str_function(Z_STR_P(arg), &l, dest)) != IS_DOUBLE)) {
			// 如果转换成功（转成整数）
			if (EXPECTED(type != 0)) {
				// 结果转成小数
				*dest = (double)(l);
			// 转换失败，返回失败
			} else {
				return 0;
			}
		}
		// 如果有异常，返回失败
		if (UNEXPECTED(EG(exception))) {
			return 0;
		}
	// 如果传入值是 false ,null ,undef
	} else if (EXPECTED(Z_TYPE_P(arg) < IS_TRUE)) {
		// 报错：不可以给规定类型的参数赋值为null。返回 !EG(exception)
		if (UNEXPECTED(Z_TYPE_P(arg) == IS_NULL) && !zend_null_arg_deprecated("float", arg_num)) {
			return 0;
		}
		// 返回结果 0
		*dest = 0.0;
	// 如果传入值是 true
	} else if (EXPECTED(Z_TYPE_P(arg) == IS_TRUE)) {
		// 返回结果 1
		*dest = 1.0;
	// 其他情况，返回0
	} else {
		return 0;
	}
	// 成功
	return 1;
}
/* }}} */

// ing4, (非整数，严格类型返回0)把传入的参数转换成小数，遇到null会报错。p2:返回值，p3:arg参数序号，报错用
ZEND_API bool ZEND_FASTCALL zend_parse_arg_double_slow(zval *arg, double *dest, uint32_t arg_num) /* {{{ */
{
	// 如果传入整数
	if (EXPECTED(Z_TYPE_P(arg) == IS_LONG)) {
		// ssth 异常：IS_LONG 可以替换成 IS_DOUBLE
		/* SSTH Exception: IS_LONG may be accepted instead as IS_DOUBLE */
		
		// 转成小数
		*dest = (double)Z_LVAL_P(arg);
	// 非整数，严格类型返回 false
	} else if (UNEXPECTED(ZEND_ARG_USES_STRICT_TYPES())) {
		return 0;
	}
	// 把传入的参数转换成小数，遇到null会报错。p2:返回值，p3:arg参数序号，报错用
	return zend_parse_arg_double_weak(arg, dest, arg_num);
}
/* }}} */

// ing4, (严格类型返回0)把传入的参数转换成数值，遇到null会报错。p2:返回值，p3:arg参数序号，报错用
ZEND_API bool ZEND_FASTCALL zend_parse_arg_number_slow(zval *arg, zval **dest, uint32_t arg_num) /* {{{ */
{
	// 严格类型返回0
	if (UNEXPECTED(ZEND_ARG_USES_STRICT_TYPES())) {
		return 0;
	}
	// 如果参数类型是字串
	if (Z_TYPE_P(arg) == IS_STRING) {
		// 参数中的字串
		zend_string *str = Z_STR_P(arg);
		zend_long lval;
		double dval;
		// 字串转成数值
		zend_uchar type = is_numeric_str_function(str, &lval, &dval);
		// 结果类型是 整数
		if (type == IS_LONG) {
			// 更新参数值
			ZVAL_LONG(arg, lval);
		// 结果类型是 小数
		} else if (type == IS_DOUBLE) {
			// 更新参数值
			ZVAL_DOUBLE(arg, dval);
		// 其他情况
		} else {
			// 返回false
			return 0;
		}
		zend_string_release(str);
	// 如果参数类型是 undef,null,false
	} else if (Z_TYPE_P(arg) < IS_TRUE) {
		// 如果参数是null
		// 报错：不可以给规定类型的参数赋值为null。返回 !EG(exception)
		if (UNEXPECTED(Z_TYPE_P(arg) == IS_NULL) && !zend_null_arg_deprecated("int|float", arg_num)) {
			return 0;
		}
		ZVAL_LONG(arg, 0);
	// 如果参数值是true
	} else if (Z_TYPE_P(arg) == IS_TRUE) {
		// 参数值修改为1
		ZVAL_LONG(arg, 1);
	// 其他情况
	} else {
		// 返回 false
		return 0;
	}
	// 返回结果为此参数
	*dest = arg;
	// 返回 true
	return 1;
}
/* }}} */

// ing3, 把传入的参数转换成字串，遇到null会报错。p2:返回值，p3:arg参数序号，报错用
ZEND_API bool ZEND_FASTCALL zend_parse_arg_str_weak(zval *arg, zend_string **dest, uint32_t arg_num) /* {{{ */
{
	// 如果 arg 是 double,long,true,false,null,undef
	if (EXPECTED(Z_TYPE_P(arg) < IS_STRING)) {
		// 如果遇到null。报错：不可以给规定类型的参数赋值为null。返回 !EG(exception)
		if (UNEXPECTED(Z_TYPE_P(arg) == IS_NULL) && !zend_null_arg_deprecated("string", arg_num)) {
			return 0;
		}
		// 如果不是null，转成字串
		convert_to_string(arg);
		// 返回给结果
		*dest = Z_STR_P(arg);
	// arg 是对象
	} else if (UNEXPECTED(Z_TYPE_P(arg) == IS_OBJECT)) {
		// 对象指针
		zend_object *zobj = Z_OBJ_P(arg);
		zval obj;
		// 调用对象自己的方法转成字串。如果成功
		if (zobj->handlers->cast_object(zobj, &obj, IS_STRING) == SUCCESS) {
			// 销毁释放掉 此对象
			OBJ_RELEASE(zobj);
			// 把转换结果放到arg里
			ZVAL_COPY_VALUE(arg, &obj);
			// 转换后的字串，返回
			*dest = Z_STR_P(arg);
			// 返回成功
			return 1;
		}
		// 转换不成功，返回失败
		return 0;
	// arg不是简单值，也不是对象。（可能是字串，数组，资源，引用等类型）
	} else {
		// 返回失败
		return 0;
	}
	// 成功
	return 1;
}
/* }}} */

// ing4, （严格类型返回失败）把传入的参数转换成字串，遇到null会报错。p2:返回值，p3:arg参数序号，报错用
ZEND_API bool ZEND_FASTCALL zend_parse_arg_str_slow(zval *arg, zend_string **dest, uint32_t arg_num) /* {{{ */
{
	// 严格类型返回失败
	if (UNEXPECTED(ZEND_ARG_USES_STRICT_TYPES())) {
		return 0;
	}
	return zend_parse_arg_str_weak(arg, dest, arg_num);
}
/* }}} */

// ing4, （严格类型返回失败）把传入的参数转换成整数或字串，遇到null会报错。p2,3:返回值，p4:arg参数序号，报错用
ZEND_API bool ZEND_FASTCALL zend_parse_arg_str_or_long_slow(zval *arg, zend_string **dest_str, zend_long *dest_long, uint32_t arg_num) /* {{{ */
{
	// 严格类型返回失败
	if (UNEXPECTED(ZEND_ARG_USES_STRICT_TYPES())) {
		return 0;
	}
	// 尝试转成整数，如果成功
	if (zend_parse_arg_long_weak(arg, dest_long, arg_num)) {
		// 清空字串返回值
		*dest_str = NULL;
		return 1;
	// 尝试转字串，如果成功
	} else if (zend_parse_arg_str_weak(arg, dest_str, arg_num)) {
		// 整数返回值为0
		*dest_long = 0;
		return 1;
	// 其他情况，返回失败
	} else {
		return 0;
	}
}
/* }}} */

// 近300行
// ing3, 解析参数，失败返回需要的类型, p1:参数，p2:函数参数列表，p3:修饰符，p4:返回的错误，p5:参数序号
static const char *zend_parse_arg_impl(zval *arg, va_list *va, const char **spec, char **error, uint32_t arg_num) /* {{{ */
{
	// spec
	const char *spec_walk = *spec;
	// 取得当前字符，然后指针后移
	char c = *spec_walk++;
	// 默认不检查null
	bool check_null = 0;
	// 默认不创建副本
	bool separate = 0;
	// 默认使用原 arg（不是副本）
	zval *real_arg = arg;

	// 通过修饰符扫描
	/* scan through modifiers */
	// 减少引用数
	ZVAL_DEREF(arg);
	// 检查 /! 两个前缀
	while (1) {
		// 碰到 '/'
		if (*spec_walk == '/') {
			// 给arg创建副本
			SEPARATE_ZVAL_NOREF(arg);
			// arg 副本
			real_arg = arg;
			// 需要创建副本
			separate = 1;
		// 碰到 ！
		} else if (*spec_walk == '!') {
			// 要检查null
			check_null = 1;
		// 其他情况直接跳出
		} else {
			break;
		}
		// 下一个字符
		spec_walk++;
	}

	switch (c) {
		// 整数(允null)
		case 'l':
			{
				// 从参数里获取 整数 指针
				zend_long *p = va_arg(*va, zend_long *);
				// 默认非null
				bool *is_null = NULL;
				// 如果检查null
				if (check_null) {
					// 接收is_null的指针
					is_null = va_arg(*va, bool *);
				}
				// 实际参数解析成整数 ,如果失败
				if (!zend_parse_arg_long(arg, p, is_null, check_null, arg_num)) {
					// 返回类型字串
					return check_null ? "?int" : "int";
				}
			}
			break;
		// 小数(允null)
		case 'd':
			{
				// 从参数里获取 小数 指针
				double *p = va_arg(*va, double *);
				bool *is_null = NULL;
				// 如果检查null
				if (check_null) {
					// 接收is_null的指针
					is_null = va_arg(*va, bool *);
				}
				// 实际参数解析成小数，p2:返回值，p3:是否为null，p4:是否允null，p5:arg参数序号，报错用
				if (!zend_parse_arg_double(arg, p, is_null, check_null, arg_num)) {
					return check_null ? "?float" : "float";
				}
			}
			break;
		// 整数或小数(允null)
		case 'n':
			{
				// 从参数里获取 zval 指针的指针
				zval **p = va_arg(*va, zval **);
				// 实际参数解析成数值，p2:返回值，p3:是否允null，p4:arg参数序号，报错用
				if (!zend_parse_arg_number(arg, p, check_null, arg_num)) {
					return check_null ? "int|float|null" : "int|float";
				}
			}
			break;
		// 字串(允null)
		case 's':
			{
				// 从参数里获取 字串 指针的指针
				char **p = va_arg(*va, char **);
				size_t *pl = va_arg(*va, size_t *);
				// 实际参数解析成字串和长度，p2,3:返回值，p4:是否允null，p5是arg参数序号，报错用
				if (!zend_parse_arg_string(arg, p, pl, check_null, arg_num)) {
					return check_null ? "?string" : "string";
				}
			}
			break;
		// 字串，不可包含无效字符(允null)
		case 'p':
			{
				// 从参数里获取 字串 指针的指针
				char **p = va_arg(*va, char **);
				size_t *pl = va_arg(*va, size_t *);
				// 参数解析成字串和长度，字串不可包含无效字符，p3,4:返回值，p4:是否允null，p5:arg参数序号，报错用
				if (!zend_parse_arg_path(arg, p, pl, check_null, arg_num)) {
					// 如果参数是字串
					if (Z_TYPE_P(arg) == IS_STRING) {
						// 报错：不可以包含任何 null 字节
						zend_spprintf(error, 0, "must not contain any null bytes");
						// 返回空字串
						return "";
					// 参数不是字串
					} else {
						return check_null ? "?string" : "string";
					}
				}
			}
			break;
		// 字串，zend_string，不可包含无效字符(允null)
		case 'P':
			{
				// 从参数里获取 zend_string 指针的指针
				zend_string **str = va_arg(*va, zend_string **);
				// 参数解析成 zend_string，字串不可包含无效字符，p3:是否允null，p4:arg参数序号，报错用
				if (!zend_parse_arg_path_str(arg, str, check_null, arg_num)) {
					// 如果参数是字串
					if (Z_TYPE_P(arg) == IS_STRING) {
						// 报错：不可以包含任何 null 字节
						zend_spprintf(error, 0, "must not contain any null bytes");
						// 返回空字串
						return "";
					// 参数不是字串
					} else {
						return check_null ? "?string" : "string";
					}
				}
			}
			break;
		// 字串，zend_string(允null)
		case 'S':
			{
				// 从参数里获取 zend_string 指针的指针
				zend_string **str = va_arg(*va, zend_string **);
				//  实际参数解析成 zend_string，p2:返回值，p3:是否允null，p4:arg参数序号，报错用
				if (!zend_parse_arg_str(arg, str, check_null, arg_num)) {
					return check_null ? "?string" : "string";
				}
			}
			break;
		// 布尔型(允null)
		case 'b':
			{
				// 从参数里获取 布尔型 指针
				bool *p = va_arg(*va, bool *);
				bool *is_null = NULL;
				// 如果检查null
				if (check_null) {
					// 接收is_null的指针
					is_null = va_arg(*va, bool *);
				}
				// 把参数解析成布尔值，p2:返回值，p3:是否允null，p4:是否检查null，p5:arg参数序号，报错用 
				if (!zend_parse_arg_bool(arg, p, is_null, check_null, arg_num)) {
					return check_null ? "?bool" : "bool";
				}
			}
			break;
		// 资源型(允null)
		case 'r':
			{
				// 从参数里获取 zval 指针的指针
				zval **p = va_arg(*va, zval **);
				// 实际参数解析成资源类型，p2:返回值，p3:是否允null
				if (!zend_parse_arg_resource(arg, p, check_null)) {
					return check_null ? "resource or null" : "resource";
				}
			}
			break;
		// 数组型，允许对象(允null)
		case 'A':
		// 数组型，不允许对象(允null)
		case 'a':
			{
				// 从参数里获取 zval 指针的指针
				zval **p = va_arg(*va, zval **);
				// 实际参数解析成数组或对象，p2:返回值，p3:是否允null，p4:是否允许对象
				if (!zend_parse_arg_array(arg, p, check_null, c == 'A')) {
					return check_null ? "?array" : "array";
				}
			}
			break;
		// 哈希表，允许对象(允null)
		case 'H':
		// 哈希表，不允许对象(允null)
		case 'h':
			{
				// 从参数里获取 哈希表 指针的指针
				HashTable **p = va_arg(*va, HashTable **);
				// 实际参数解析成哈希表，p2:返回值，p3:是否允null，p4:是否支持对象，p5:是否创建副本
				if (!zend_parse_arg_array_ht(arg, p, check_null, c == 'H', separate)) {
					return check_null ? "?array" : "array";
				}
			}
			break;
		// 对象(允null)
		case 'o':
			{
				// 从参数里获取 zval 指针的指针
				zval **p = va_arg(*va, zval **);
				// 实际参数解析成对象（zval），p2:返回值，p3:所属类，p4:是否允null
				if (!zend_parse_arg_object(arg, p, NULL, check_null)) {
					return check_null ? "?object" : "object";
				}
			}
			break;
		// 对象(允null)
		case 'O':
			{
				// 从参数里获取 zval 指针的指针
				zval **p = va_arg(*va, zval **);
				zend_class_entry *ce = va_arg(*va, zend_class_entry *);
				// 实际参数解析成对象（zval），p2:返回值，p3:所属类，p4:是否允null
				if (!zend_parse_arg_object(arg, p, ce, check_null)) {
					if (ce) {
						// 如果检查null
						if (check_null) {
							// 报错：必须是 A 类型，但给了 B 类型
							zend_spprintf(error, 0, "must be of type ?%s, %s given", ZSTR_VAL(ce->name), zend_zval_type_name(arg));
							// 返回空字串
							return "";
						// 不检查 null
						} else {
							// 返回类名
							return ZSTR_VAL(ce->name);
						}
					} else {
						return check_null ? "?object" : "object";
					}
				}
			}
			break;
		// 类名
		case 'C':
			{
				// 取得参数中的类名 pce: parent ce
				zend_class_entry *lookup, **pce = va_arg(*va, zend_class_entry **);
				// 基类名
				zend_class_entry *ce_base = *pce;

				// 允null，并且遇到null
				if (check_null && Z_TYPE_P(arg) == IS_NULL) {
					// 返回null
					*pce = NULL;
					break;
				}
				
				// 如果参数不能转成字串
				if (!try_convert_to_string(arg)) {
					// 返回null
					*pce = NULL;
					// 返回空字串
					return ""; /* try_convert_to_string() throws an exception */
				}

				// 参数是字串，获取类
				if ((lookup = zend_lookup_class(Z_STR_P(arg))) == NULL) {
					// 失败返回null
					*pce = NULL;
				} else {
					// 成功返回找到的类
					*pce = lookup;
				}
				// 有要求的基类
				if (ce_base) {
					// 如果 找到的类 无效 或  它不是 ce_base 的子类
					if ((!*pce || !instanceof_function(*pce, ce_base))) {
						// 报错：必须 是 ce_base的衍生类，但给出了 A 类
						zend_spprintf(error, 0, "must be a class name derived from %s%s, %s given",
							ZSTR_VAL(ce_base->name), check_null ? " or null" : "", Z_STRVAL_P(arg));
						// 返回NULL
						*pce = NULL;
						// 返回空字串
						return "";
					}
				}
				// （没有基类） 如果找到的类无效
				if (!*pce) {
					// 必须要有效的类名
					zend_spprintf(error, 0, "must be a valid class name%s, %s given",
						check_null ? " or null" : "", Z_STRVAL_P(arg));
					// 返回空字串	
					return "";
				}
				break;

			}
			break;
		// 
		case 'f':
			{
				// 从参数中获取 调用信息
				zend_fcall_info *fci = va_arg(*va, zend_fcall_info *);
				// 从参数中获取 调用信息缓存
				zend_fcall_info_cache *fcc = va_arg(*va, zend_fcall_info_cache *);
				// 
				char *is_callable_error = NULL;
				
				// 允null，并且遇到null
				if (check_null && Z_TYPE_P(arg) == IS_NULL) {
					// 大小 0
					fci->size = 0;
					// 调用函数 0
					fcc->function_handler = 0;
					break;
				}

				// 初始化调用信息，如果成功
				if (zend_fcall_info_init(arg, 0, fci, fcc, NULL, &is_callable_error) == SUCCESS) {
					// 必须不能有错误信息
					ZEND_ASSERT(!is_callable_error);
					// 释放调用弹跳：方法可能没有被调用，这种情况下弹跳会泄露。
					// 在 zend_call_function 中会强制 重新获取
					/* Release call trampolines: The function may not get called, in which case
					 * the trampoline will leak. Force it to be refetched during
					 * zend_call_function instead. */
					 
					// 释放缓存的 函数调用信息
					zend_release_fcall_info_cache(fcc);
					break;
				}

				// 如果有错误信息
				if (is_callable_error) {
					// 报错：必须要有效的回调
					zend_spprintf(error, 0, "must be a valid callback%s, %s", check_null ? " or null" : "", is_callable_error);
					// 释放错误信息
					efree(is_callable_error);
					// 返回空字串
					return "";
				// 没有错误信息
				} else {
					// 
					return check_null ? "a valid callback or null" : "a valid callback";
				}
			}
		// zval (允null)
		case 'z':
			{
				// 从参数里获取 zval 指针的指针
				zval **p = va_arg(*va, zval **);
				// 如果要检验null并且 arg 是null，返回null。否则返回 arg
				zend_parse_arg_zval_deref(real_arg, p, check_null);
			}
			break;
		// 用小写 z 代替
		case 'Z': /* replace with 'z' */
		// 用小写 l 代替
		case 'L': /* replace with 'l' */
			// ZPP：zend parse ...
			ZEND_ASSERT(0 && "ZPP modifier no longer supported");
			ZEND_FALLTHROUGH;
		// 其他情况，返回unknown
		default:
			return "unknown";
	}
	// 返回新的指针位置
	*spec = spec_walk;

	return NULL;
}
/* }}} */

// ing3, 解析参数：p1:参数序号，p2:参数，p3:函数参数列表，p4:修饰符，p5:附加配置信息
static zend_result zend_parse_arg(uint32_t arg_num, zval *arg, va_list *va, const char **spec, int flags) /* {{{ */
{
	const char *expected_type = NULL;
	char *error = NULL;
	// 解析参数，失败返回需要的类型, p1:参数，p2:函数参数列表，p3:修饰符，p4:返回的错误，p5:参数序号
	expected_type = zend_parse_arg_impl(arg, va, spec, &error, arg_num);
	// 如果有返回信息（解析失败）
	if (expected_type) {
		// 如果有异常，返回失败
		if (EG(exception)) {
			return FAILURE;
		}
		// 如果不要求静默 并且 （有需要的类型 或 有错误信息）
		if (!(flags & ZEND_PARSE_PARAMS_QUIET) && (*expected_type || error)) {
			// 如果有错误信息
			if (error) {
				// 如果信息是：不可以包含null字符
				if (strcmp(error, "must not contain any null bytes") == 0) {
					// 报错（类型：zend_ce_value_error）
					zend_argument_value_error(arg_num, "%s", error);
				// 否则 
				} else {
					// 报错（类型：zend_ce_type_error）
					zend_argument_type_error(arg_num, "%s", error);
				}
				// 释放错误信息
				efree(error);
			// 没有错误信息（有需要的类型）
			} else {
				// 报错：必须要指定类型
				zend_argument_type_error(arg_num, "must be of type %s, %s given", expected_type, zend_zval_type_name(arg));
			}
		// 如果有错误
		} else if (error) {
			// 释放错误
			efree(error);
		}
		// 返回失败
		return FAILURE;
	}
	// 返回成功
	return SUCCESS;
}
/* }}} */

// ing3, 解析参数（1个）：p1:附加配置信息，p2:参数序号，p3:参数，p4:修饰符，pN:多个参数
ZEND_API zend_result zend_parse_parameter(int flags, uint32_t arg_num, zval *arg, const char *spec, ...)
{
	va_list va;
	zend_result ret;

	va_start(va, spec);
	// 解析参数：p1:参数序号，p2:参数，p3:函数参数列表，p4:修饰符，p5:附加配置信息
	ret = zend_parse_arg(arg_num, arg, &va, &spec, flags);
	va_end(va);

	return ret;
}

// ing4, 报错：解析参数调试错误， 错误信息是传入的，前面添加了类名和方法名
static ZEND_COLD void zend_parse_parameters_debug_error(const char *msg) {
	// 当前执行数据中的方法
	zend_function *active_function = EG(current_execute_data)->func;
	// 类名
	const char *class_name = active_function->common.scope
		? ZSTR_VAL(active_function->common.scope->name) : "";
	// 报错：错误信息是传入的，前面添加了类名和方法名
	zend_error_noreturn(E_CORE_ERROR, "%s%s%s(): %s",
		class_name, class_name[0] ? "::" : "",
		ZSTR_VAL(active_function->common.function_name), msg);
}

// ing3, 解析传入的参数（包含可变参数）p1:参数数量，p2:参数格式列表，p3:可变参数列表，p4:标记
static zend_result zend_parse_va_args(uint32_t num_args, const char *type_spec, va_list *va, int flags) /* {{{ */
{
	const  char *spec_walk;
	char c;
	uint32_t i;
	uint32_t min_num_args = 0;
	uint32_t max_num_args = 0;
	uint32_t post_varargs = 0;
	zval *arg;
	bool have_varargs = 0;
	bool have_optional_args = 0;
	zval **varargs = NULL;
	uint32_t *n_varargs = NULL;
	
	// 步骤1，逐个字符处理
	// 遍历 type_spec 的每一个字符 
	for (spec_walk = type_spec; *spec_walk; spec_walk++) {
		c = *spec_walk;
		// 按字符处理
		switch (c) {
			// 类型修饰符，只计数
			case 'l': case 'd':
			case 's': case 'b':
			case 'r': case 'a':
			case 'o': case 'O':
			case 'z': case 'Z':
			case 'C': case 'h':
			case 'f': case 'A':
			case 'H': case 'p':
			case 'S': case 'P':
			case 'L': case 'n':
				max_num_args++;
				// 下个字符
				break;
			// 附加修饰符
			case '|':
				// 最少参数=最多参数 ？
				min_num_args = max_num_args;
				// 有可选参数
				have_optional_args = 1;
				// 下个字符
				break;
			// 这两个字符跳过
			case '/':
			case '!':
				/* Pass */
				// 下个字符
				break;
			//
			case '*':
			case '+':
				// 如果有可变参数
				if (have_varargs) {
					// 报错 只允许一个可变参数标识 （*或+）
					zend_parse_parameters_debug_error(
						"only one varargs specifier (* or +) is permitted");
					// 返回失败
					return FAILURE;
				}
				// 有可变参数（只有这里会标记成1）
				have_varargs = 1;
				// 可变参数里，至少应该有1个参数
				/* we expect at least one parameter in varargs */
				// 如果是 + 号
				if (c == '+') {
					// 数量 +1
					max_num_args++;
				}
				// *号不计数
				
				// 标记可变参数的开始位置
				/* mark the beginning of varargs */
				post_varargs = max_num_args;
				
				// 如果当前操作码有 额外命名参数标记
				if (ZEND_CALL_INFO(EG(current_execute_data)) & ZEND_CALL_HAS_EXTRA_NAMED_PARAMS) {
					// 报错 不允许未知的命名参数
					zend_unexpected_extra_named_error();
					// 返回失败
					return FAILURE;
				}
				// 下个字符
				break;
			// 其他情况
			default:
				// 报错 ：解析参数时遇到错误的类型字符
				zend_parse_parameters_debug_error("bad type specifier while parsing parameters");
				return FAILURE;
		}
	}

	// 步骤2，确定参数数量
	// 没有可选参数，最小参数数量 和 最大参数数量必须相等
	/* with no optional arguments the minimum number of arguments must be the same as the maximum */
	// 如果没有可选参数
	if (!have_optional_args) {
		// 最小参数数量 = 最大参数数量 
		min_num_args = max_num_args;
	}
	
	// 步骤3，确定可变参数后面有多少个参数，变更参数数量
	// 如果有可变参数
	if (have_varargs) {
		// 计算在修饰符列表结尾有多少个必须参数
		/* calculate how many required args are at the end of the specifier list */
		
		// 计算在可变参数后面有多少个参数
		post_varargs = max_num_args - post_varargs;
		
		// 有了可变参数，最大数量是 无限
		max_num_args = UINT32_MAX;
	}

	// 步骤4，参数数量是否有效
	// 如果参数数量不在有效范围内
	if (num_args < min_num_args || num_args > max_num_args) {
		// 如果不要求静默解析
		if (!(flags & ZEND_PARSE_PARAMS_QUIET)) {
			// 当前函数名
			zend_string *func_name = get_active_function_or_method_name();
			// 报错：函数需要的参数与给出的参数数量 不符
			zend_argument_count_error("%s() expects %s %d argument%s, %d given",
				ZSTR_VAL(func_name),
				min_num_args == max_num_args ? "exactly" : num_args < min_num_args ? "at least" : "at most",
				num_args < min_num_args ? min_num_args : max_num_args,
				(num_args < min_num_args ? min_num_args : max_num_args) == 1 ? "" : "s",
				num_args
			);
			// 释放函数名
			zend_string_release(func_name);
		}
		// 返回失败
		return FAILURE;
	}
	//  要求参数数量 大于 执行数据中的参数数量 
	if (num_args > ZEND_CALL_NUM_ARGS(EG(current_execute_data))) {
		// 无法获取用来解析的参数
		zend_parse_parameters_debug_error("could not obtain parameters for parsing");
		// 返回失败
		return FAILURE;
	}

	// 步骤5，解析参数
	// 计数
	i = 0;
	// 倒序遍历 
	while (num_args-- > 0) {
		// 碰到 |
		if (*type_spec == '|') {
			// 下一个字符
			type_spec++;
		}
		// 碰到*或+
		if (*type_spec == '*' || *type_spec == '+') {
			// 可变参数数量  = 总数 + 1 - 可变参数后面的数量
			uint32_t num_varargs = num_args + 1 - post_varargs;
			// 
			/* eat up the passed in storage even if it won't be filled in with varargs */
			// （返回值）从列表参数中 取出可变变量列表（zval **）
			varargs = va_arg(*va, zval **);
			// （返回值）从列表中取得 可变变量数量 （uint32_t *) 
			n_varargs = va_arg(*va, uint32_t *);
			// 下一个字符
			type_spec++;
			// 如果可变参数数量大于 0
			if (num_varargs > 0) {
				// 返回可变参数数量（这里就不再做解析了）
				*n_varargs = num_varargs;
				// 返回可变参数 列表指针： 取得列表中第 （ZEND_CALL_FRAME_SLOT +） n 个 zval
				*varargs = ZEND_CALL_ARG(EG(current_execute_data), i + 1);
				// 记录在重启循环前，一共处理了多少个参数
				/* adjust how many args we have left and restart loop */
				num_args += 1 - num_varargs;
				// 调整计数
				i += num_varargs;
				// 下一个
				continue;
			// 可变参数数量为0
			} else {
				// 可变参数为空
				*varargs = NULL;
				// 可变参数数量为0
				*n_varargs = 0;
			}
		}
		
		// 取得列表中第 （ZEND_CALL_FRAME_SLOT +） n 个 zval
		arg = ZEND_CALL_ARG(EG(current_execute_data), i + 1);
		// 解析参数：p1:参数序号，p2:参数，p3:函数参数列表，p4:修饰符，p5:附加配置信息
		if (zend_parse_arg(i+1, arg, va, &type_spec, flags) == FAILURE) {
			// 如果可变参数列表使用过了，清空它
			/* clean up varargs array if it was used */
			// 如果可变参数列表有效
			if (varargs && *varargs) {
				// 清空指针
				*varargs = NULL;
			}
			// 返回失败
			return FAILURE;
		}
		// 计数+1
		i++;
	}

	return SUCCESS;
}
/* }}} */

// ing4, 解析传入的参数（包含可变参数）p1:标记，p2:参数数量，p3:格式列表，p+:参数列表
ZEND_API zend_result zend_parse_parameters_ex(int flags, uint32_t num_args, const char *type_spec, ...) /* {{{ */
{
	va_list va;
	zend_result retval;

	va_start(va, type_spec);
	// 解析传入的参数（包含可变参数）p1:参数数量，p2:参数格式列表，p3:可变参数列表，p4:标记
	retval = zend_parse_va_args(num_args, type_spec, &va, flags);
	va_end(va);

	return retval;
}
/* }}} */

// ing4, 解析传入的参数（包含可变参数）p1:参数数量，p2:格式列表，p+:参数列表
ZEND_API zend_result zend_parse_parameters(uint32_t num_args, const char *type_spec, ...) /* {{{ */
{
	va_list va;
	zend_result retval;
	int flags = 0;

	va_start(va, type_spec);
	// 解析传入的参数（包含可变参数）p1:参数数量，p2:参数格式列表，p3:可变参数列表，p4:标记
	retval = zend_parse_va_args(num_args, type_spec, &va, flags);
	va_end(va);

	return retval;
}
/* }}} */

// ing3, 解析传入的参数（包含可变参数）p1:参数数量，p2:this指针，p3:参数格式列表，p+:参数列表，外部大量引用
// this有效时，还要传入p4:接收this的参数, p5:用来验证的类入口，后面才是p+:参数列表（严格检验this，不可静默解析）
ZEND_API zend_result zend_parse_method_parameters(uint32_t num_args, zval *this_ptr, const char *type_spec, ...) /* {{{ */
{
	va_list va;
	zend_result retval;
	int flags = 0;
	const char *p = type_spec;
	zval **object;
	zend_class_entry *ce;

	// 仅仅检测 this 指针是不够的，因为 调用 common.scope == NULL 的内部函数 时 
	// fcall_common_helper 没有把 Z_OBJ(EG(This)) 设置成null，
	// 这种情况下 EG(This) 还是调用代码的域，这里要修复这个错误
	/* Just checking this_ptr is not enough, because fcall_common_helper does not set
	 * Z_OBJ(EG(This)) to NULL when calling an internal function with common.scope == NULL.
	 * In that case EG(This) would still be the $this from the calling code and we'd take the
	 * wrong branch here. */
	 
	// 如果当前执行数据有域，说明是类方法
	bool is_method = EG(current_execute_data)->func->common.scope != NULL;
	
	// 如果不是类方法 或 没有 this指针 或 this不是对象
	if (!is_method || !this_ptr || Z_TYPE_P(this_ptr) != IS_OBJECT) {
		// 获取动态参数列表 开始
		va_start(va, type_spec);
		// 解析传入的参数（包含可变参数）p1:参数数量，p2:参数格式列表，p3:可变参数列表，p4:标记
		retval = zend_parse_va_args(num_args, type_spec, &va, flags);
		// 获取动态参数列表 结束
		va_end(va);
	// 是类方法并且this有效
	} else {
		p++;
		// 获取动态参数列表
		va_start(va, type_spec);
		// 第一个动态参数，对象
		object = va_arg(va, zval **);
		// 第二个动态参数，类入口
		ce = va_arg(va, zend_class_entry *);
		
		// 把this返回回去
		*object = this_ptr;
		// 如果有指定类 并且 this不是此类的实例
		if (ce && !instanceof_function(Z_OBJCE_P(this_ptr), ce)) {
			// 报错：this的所属类必须是指定类的衍生类
			zend_error_noreturn(E_CORE_ERROR, "%s::%s() must be derived from %s::%s()",
				ZSTR_VAL(Z_OBJCE_P(this_ptr)->name), get_active_function_name(), ZSTR_VAL(ce->name), get_active_function_name());
		}
		// 解析传入的参数（包含可变参数）p1:参数数量，p2:参数格式列表，p3:可变参数列表，p4:标记
		retval = zend_parse_va_args(num_args, p, &va, flags);
		// 获取动态参数列表 结束
		va_end(va);
	}
	// 结果
	return retval;
}
/* }}} */

// ing3, 解析传入的参数（包含可变参数）p1:参数数量，p2:this指针，p3:参数格式列表，p+:参数列表，外部大量引用
// this有效时，还要传入p4:接收this的参数, p5:用来验证的类入口，后面才是p+:参数列表（不严格检验this，可静默解析）
ZEND_API zend_result zend_parse_method_parameters_ex(int flags, uint32_t num_args, zval *this_ptr, const char *type_spec, ...) /* {{{ */
{
	va_list va;
	zend_result retval;
	const char *p = type_spec;
	zval **object;
	zend_class_entry *ce;

	// 没有传入this
	if (!this_ptr) {
		// 获取动态参数列表 开始
		va_start(va, type_spec);
		// 解析传入的参数（包含可变参数）p1:参数数量，p2:参数格式列表，p3:可变参数列表，p4:标记
		retval = zend_parse_va_args(num_args, type_spec, &va, flags);
		// 获取动态参数列表 结束
		va_end(va);
	} else {
		p++;
		// 获取动态参数列表
		va_start(va, type_spec);
		// 第一个动态参数，对象
		object = va_arg(va, zval **);
		// 第二个动态参数，类入口
		ce = va_arg(va, zend_class_entry *);
		// 把this返回回去
		*object = this_ptr;
		// 如果有指定类 并且 this不是此类的实例
		if (ce && !instanceof_function(Z_OBJCE_P(this_ptr), ce)) {
			// 如果没有要求静默解析
			if (!(flags & ZEND_PARSE_PARAMS_QUIET)) {
				// 报错：this的所属类必须是指定类的衍生类
				zend_error_noreturn(E_CORE_ERROR, "%s::%s() must be derived from %s::%s()",
					ZSTR_VAL(ce->name), get_active_function_name(), ZSTR_VAL(Z_OBJCE_P(this_ptr)->name), get_active_function_name());
			}
			// 获取动态参数列表 结束
			va_end(va);
			// 返回失败
			return FAILURE;
		}
		// 解析传入的参数（包含可变参数）p1:参数数量，p2:参数格式列表，p3:可变参数列表，p4:标记
		retval = zend_parse_va_args(num_args, p, &va, flags);
		// 获取动态参数列表 结束
		va_end(va);
	}
	return retval;
}
/* }}} */

// EG(fake_scope) 明显是获取成员属性等操作时用到的临时域

// 这个函数应该在构造方法调用后调用，因为它可以会在反序列化时调用 __set
/* This function should be called after the constructor has been called
 * because it may call __set from the uninitialized object otherwise. */
// ing4, 把属性表合并给对象
ZEND_API void zend_merge_properties(zval *obj, HashTable *properties) /* {{{ */
{
	// zval里的对象
	zend_object *zobj = Z_OBJ_P(obj);
	// 对象本身的更新属性方法
	zend_object_write_property_t write_property = zobj->handlers->write_property;
	// 旧的临时域
	zend_class_entry *old_scope = EG(fake_scope);
	zend_string *key;
	zval *value;
	// 如果 属性 是顺序数组，到这里就可以结束了
	if (HT_IS_PACKED(properties)) {
		return;
	}
	// 计算属性用的域，使用此对象的类
	EG(fake_scope) = Z_OBJCE_P(obj);
	// 遍历每个属性
	ZEND_HASH_MAP_FOREACH_STR_KEY_VAL(properties, key, value) {
		// 如果有属性名
		if (key) {
			// 把属性写到 对象中
			write_property(zobj, key, value, NULL);
		}
	} ZEND_HASH_FOREACH_END();
	// 恢复临时域
	EG(fake_scope) = old_scope;
}
/* }}} */

// ing4, 为类创建转换数据实例，并关联到类
static zend_class_mutable_data *zend_allocate_mutable_data(zend_class_entry *class_type) /* {{{ */
{
	zend_class_mutable_data *mutable_data;
	// 转换数据指针必须存在
	ZEND_ASSERT(ZEND_MAP_PTR(class_type->mutable_data) != NULL);
	// 转换数据实例必须不存在
	ZEND_ASSERT(ZEND_MAP_PTR_GET_IMM(class_type->mutable_data) == NULL);
	// 创建新的转换数据 
	mutable_data = zend_arena_alloc(&CG(arena), sizeof(zend_class_mutable_data));
	// 全部置0
	memset(mutable_data, 0, sizeof(zend_class_mutable_data));
	// 复制类的 标记
	mutable_data->ce_flags = class_type->ce_flags;
	// 设置转换数据实例偏移量
	ZEND_MAP_PTR_SET_IMM(class_type->mutable_data, mutable_data);

	return mutable_data;
}
/* }}} */

// ing3, 返回类的常量表副本
ZEND_API HashTable *zend_separate_class_constants_table(zend_class_entry *class_type) /* {{{ */
{
	zend_class_mutable_data *mutable_data;
	HashTable *constants_table;
	zend_string *key;
	zend_class_constant *new_c, *c;
	// 分配常量表
	constants_table = zend_arena_alloc(&CG(arena), sizeof(HashTable));
	// 初始化
	zend_hash_init(constants_table, zend_hash_num_elements(&class_type->constants_table), NULL, NULL, 0);
	// 扩展到 原常量表大小
	zend_hash_extend(constants_table, zend_hash_num_elements(&class_type->constants_table), 0);

	// 遍历原常量表
	ZEND_HASH_MAP_FOREACH_STR_KEY_PTR(&class_type->constants_table, key, c) {
		// 如果 常量属性当前类
		if (c->ce == class_type) {
			// 如果 是常量表达式
			if (Z_TYPE(c->value) == IS_CONSTANT_AST) {
				// 分配新常量
				new_c = zend_arena_alloc(&CG(arena), sizeof(zend_class_constant));
				// 把旧常量复制过来
				memcpy(new_c, c, sizeof(zend_class_constant));
				// 使用副本
				c = new_c;
			}
			// 副本增加引用次数
			Z_TRY_ADDREF(c->value);
		// 常量不属性当前类
		} else {
			// 如果 是常量表达式
			if (Z_TYPE(c->value) == IS_CONSTANT_AST) {
				// 从类常量表中获取常量
				c = zend_hash_find_ptr(CE_CONSTANTS_TABLE(c->ce), key);
				// 必须存在
				ZEND_ASSERT(c);
			}
		}
		// 添加表里添加新 常量指针
		_zend_hash_append_ptr(constants_table, key, c);
	} ZEND_HASH_FOREACH_END();

	// 类的 转换数据不能为空
	ZEND_ASSERT(ZEND_MAP_PTR(class_type->mutable_data) != NULL);
	// 根据偏移量取得转换数据
	mutable_data = ZEND_MAP_PTR_GET_IMM(class_type->mutable_data);
	// 如果没有
	if (!mutable_data) {
		// 为类创建转换数据实例，并关联到类
		mutable_data = zend_allocate_mutable_data(class_type);
	}
	// 关联常量列表
	mutable_data->constants_table = constants_table;
	// 返回常量列表
	return constants_table;
}

// ing3, 更新并返回 属性的值（更新常量表达式）
static zend_result update_property(zval *val, zend_property_info *prop_info) {
	// 如果有类型
	if (ZEND_TYPE_IS_SET(prop_info->type)) {
		zval tmp;
		// 值创建副本
		ZVAL_COPY(&tmp, val);
		// 更新常量，大量调用。p1:常量，p2:更新时用到的域
		if (UNEXPECTED(zval_update_constant_ex(&tmp, prop_info->ce) != SUCCESS)) {
			// 如果失败，销毁临时变量
			zval_ptr_dtor(&tmp);
			// 返回失败
			return FAILURE;
		}
		// 属性初始化器必须总是用严格类型进行计算
		/* property initializers must always be evaluated with strict types */;
		
		// 检查属性值类型 与 属性信息要求的类型是否匹配，如果失败
		if (UNEXPECTED(!zend_verify_property_type(prop_info, &tmp, /* strict */ 1))) {
			// 销毁 临时变量
			zval_ptr_dtor(&tmp);
			// 返回失败
			return FAILURE;
		}
		// 清空原始值
		zval_ptr_dtor(val);
		// 把计算好的值放进去
		ZVAL_COPY_VALUE(val, &tmp);
		// 返回成功
		return SUCCESS;
	}
	// 如果没有类型，更新常量
	// 更新常量，大量调用。p1:常量，p2:更新时用到的域
	return zval_update_constant_ex(val, prop_info->ce);
}

// ing3, 更新类常量（常量，静态属性，普通属性）
ZEND_API zend_result zend_update_class_constants(zend_class_entry *class_type) /* {{{ */
{
	zend_class_mutable_data *mutable_data = NULL;
	zval *default_properties_table = NULL;
	zval *static_members_table = NULL;
	zend_class_constant *c;
	zval *val;
	uint32_t ce_flags;

	ce_flags = class_type->ce_flags;
	// 步骤1：检查处理完成，完成就不用再处理了
	// 如果常量已更新过
	if (ce_flags & ZEND_ACC_CONSTANTS_UPDATED) {
		// 返回成功
		return SUCCESS;
	}
	
	// 步骤2：创建转换数据
	// 是否有转换数据
	bool uses_mutable_data = ZEND_MAP_PTR(class_type->mutable_data) != NULL;
	// 如果 有转换数据
	if (uses_mutable_data) {
		// 根据偏移量取回指定的指针
		mutable_data = ZEND_MAP_PTR_GET_IMM(class_type->mutable_data);
		// 如果转换数据有效
		if (mutable_data) {
			// 类标记
			ce_flags = mutable_data->ce_flags;
			// 如果转换数据中的常量更新过了
			if (ce_flags & ZEND_ACC_CONSTANTS_UPDATED) {
				// 返回成功
				return SUCCESS;
			}
		// 如果没有转换数据
		} else {
			// 为类创建转换数据实例，并关联到类
			mutable_data = zend_allocate_mutable_data(class_type);
		}
	}

	// 步骤3：处理常量
	// 如果有父类
	if (class_type->parent) {
		// 如果更新父类常量（常量，静态属性，普通属性）失败
		if (UNEXPECTED(zend_update_class_constants(class_type->parent) != SUCCESS)) {
			// 返回失败
			return FAILURE;
		}
	}

	// 如果有表达式常量
	if (ce_flags & ZEND_ACC_HAS_AST_CONSTANTS) {
		HashTable *constants_table;
		// 如果有转换数据
		if (uses_mutable_data) {
			// 取得转换数据中的常量表
			constants_table = mutable_data->constants_table;
			// 常量表不存在
			if (!constants_table) {
				// 返回类的常量表副本
				constants_table = zend_separate_class_constants_table(class_type);
			}
		// 没有转换数据
		} else {
			// 使用此类的常量表
			constants_table = &class_type->constants_table;
		}

		zend_string *name;
		// 遍历常量表
		ZEND_HASH_MAP_FOREACH_STR_KEY_VAL(constants_table, name, val) {
			// 常量
			c = Z_PTR_P(val);
			// 如果类型是表达式常量
			if (Z_TYPE(c->value) == IS_CONSTANT_AST) {
				// 如果所属类不是当前类
				if (c->ce != class_type) {
					// 从所属类的常量表里查找此常量
					Z_PTR_P(val) = c = zend_hash_find_ptr(CE_CONSTANTS_TABLE(c->ce), name);
					// 如果不是表达式常量
					if (Z_TYPE(c->value) != IS_CONSTANT_AST) {
						// 下一个
						continue;
					}
					// 如果是表达式常量，这时候先处理父类中的常量
				}
				//
				val = &c->value;
				// 更新常量，大量调用。p1:常量，p2:更新时用到的域
				if (UNEXPECTED(zval_update_constant_ex(val, c->ce) != SUCCESS)) {
					// 更新失败，返回：失败
					return FAILURE;
				}
			}
		} ZEND_HASH_FOREACH_END();
	}

	// 步骤4：初始化静态成员
	// 如果有静态成员
	if (class_type->default_static_members_count) {
		// 返回静态成员表【指针列表】
		static_members_table = CE_STATIC_MEMBERS(class_type);
		// 如果没有静态成员表
		if (!static_members_table) {
			// 初始化静态成员列表，并把父类的静态成员复制过来
			zend_class_init_statics(class_type);
			// 返回静态成员表【指针列表】
			static_members_table = CE_STATIC_MEMBERS(class_type);
		}
	}

	// 步骤5：初始化普通属性
	// 默认属性列表
	default_properties_table = class_type->default_properties_table;
	// 如果有转换数据 并且 此类有属性
	if (uses_mutable_data && (ce_flags & ZEND_ACC_HAS_AST_PROPERTIES)) {
		zval *src, *dst, *end;
		// 从转换数据中获取成员表
		default_properties_table = mutable_data->default_properties_table;
		// 如果没有默认成员表
		if (!default_properties_table) {
			// 创建 属性（zval） 列表
			default_properties_table = zend_arena_alloc(&CG(arena), sizeof(zval) * class_type->default_properties_count);
			// 获取此类的成员列表（非转换数据）
			src = class_type->default_properties_table;
			// 目标列表
			dst = default_properties_table;
			// 原列表的结束位置
			end = dst + class_type->default_properties_count;
			// 遍历原列表
			do {
				// 把无列表中的数据依次复制到新列表中
				ZVAL_COPY_PROP(dst, src);
				// 下一个元素
				src++;
				dst++;
			} while (dst != end);
			// 使用新列表作为转换数据 的 属性列表
			mutable_data->default_properties_table = default_properties_table;
		}
	}

	// 步骤6：更新 普通属性 和 静态成员
	// 如果有类属性 或 有静态成员
	if (ce_flags & (ZEND_ACC_HAS_AST_PROPERTIES|ZEND_ACC_HAS_AST_STATICS)) {
		zend_property_info *prop_info;
		// 使用默认属性表 来更新私有属性的初始化器，它在子类中被遮蔽了
		/* Use the default properties table to also update initializers of private properties
		 * that have been shadowed in a child class. */
		// 遍历默认属性
		for (uint32_t i = 0; i < class_type->default_properties_count; i++) {
			// 取出属性
			val = &default_properties_table[i];
			// 属性信息
			prop_info = class_type->properties_info_table[i];
			// 如果属性是 表达式 并且 更新属性
			if (Z_TYPE_P(val) == IS_CONSTANT_AST
					// 更新并返回 属性的值（更新常量表达式）
					&& UNEXPECTED(update_property(val, prop_info) != SUCCESS)) {
				// 更新失败，返回：失败
				return FAILURE;
			}
		}
		
		// 遍历默认静态属性
		if (class_type->default_static_members_count) {
			// 遍历属性信息
			ZEND_HASH_MAP_FOREACH_PTR(&class_type->properties_info, prop_info) {
				// 如果是静态属性
				if (prop_info->flags & ZEND_ACC_STATIC) {
					// 找到此静态属性
					val = static_members_table + prop_info->offset;
					// 如果是表达式 更新它
					if (Z_TYPE_P(val) == IS_CONSTANT_AST
							// 更新并返回 属性的值（更新常量表达式）
							&& UNEXPECTED(update_property(val, prop_info) != SUCCESS)) {
						// 更新失败，返回：失败
						return FAILURE;
					}
				}
			} ZEND_HASH_FOREACH_END();
		}
	}

	// 步骤7：如果是用户定义类 并且 是枚举 并且 有定义枚举类型
	if (class_type->type == ZEND_USER_CLASS && class_type->ce_flags & ZEND_ACC_ENUM && class_type->enum_backing_type != IS_UNDEF) {
		// 给类添加备用 enum 表（case值表）
		if (zend_enum_build_backed_enum_table(class_type) == FAILURE) {
			// 如果失败，返回失败
			return FAILURE;
		}
	}

	// 步骤8：处理标记
	// 添加标记：已更新常量
	ce_flags |= ZEND_ACC_CONSTANTS_UPDATED;
	// 删除标记：包含表达式常量
	ce_flags &= ~ZEND_ACC_HAS_AST_CONSTANTS;
	// 删除标记：包含表达式属性
	ce_flags &= ~ZEND_ACC_HAS_AST_PROPERTIES;
	// 删除标记：包含表达式 静态属性
	ce_flags &= ~ZEND_ACC_HAS_AST_STATICS;
	// 如果有转换数据
	if (uses_mutable_data) {
		// 更新抱团数据的标记
		mutable_data->ce_flags = ce_flags;
	// 没有转换数据 
	} else {
		// 更新当前类的标记
		class_type->ce_flags = ce_flags;
	}

	return SUCCESS;
}
/* }}} */

// ing3, 初始化对象的属性表：复制所属类属性表的每个元素
static zend_always_inline void _object_properties_init(zend_object *object, zend_class_entry *class_type) /* {{{ */
{
	// 如果有默认属性
	if (class_type->default_properties_count) {
		// 返回 ce->mutable_data 或者 ce本身的 默认属性列表，开头
		zval *src = CE_DEFAULT_PROPERTIES_TABLE(class_type);
		// 对象的属性表
		zval *dst = object->properties_table;
		// 类的属性表结尾
		zval *end = src + class_type->default_properties_count;

		// 如果类是内置类
		if (UNEXPECTED(class_type->type == ZEND_INTERNAL_CLASS)) {
			// 遍历类的每个属性
			do {
				// 复制每个属性
				ZVAL_COPY_OR_DUP_PROP(dst, src);
				src++;
				dst++;
			} while (src != end);
		// 如果类是用户类
		} else {
			// 遍历类的每个属性
			do {
				// 复制每个属性（追加引用次数）
				ZVAL_COPY_PROP(dst, src);
				src++;
				dst++;
			} while (src != end);
		}
	}
}
/* }}} */

// ing4, 初始化对象的属性表：复制所属类属性表的每个元素
ZEND_API void object_properties_init(zend_object *object, zend_class_entry *class_type) /* {{{ */
{
	// 附加属性为空
	object->properties = NULL;
	// ing3, 初始化对象的属性表：复制所属类属性表的每个元素
	_object_properties_init(object, class_type);
}
/* }}} */

// ing3, 把传入的属性表应用到此对象，验证不过的跳过
ZEND_API void object_properties_init_ex(zend_object *object, HashTable *properties) /* {{{ */
{
	// 应用附加属性表
	object->properties = properties;
	
	// 如果有默认属性
	if (object->ce->default_properties_count) {
		zval *prop;
		zend_string *key;
		zend_property_info *property_info;
		// 遍历属性表
		ZEND_HASH_MAP_FOREACH_STR_KEY_VAL(properties, key, prop) {
			// 获取属性信息 （zend_property_info，不是属性值）
			property_info = zend_get_property_info(object->ce, key, 1);
			// 如果没有出错 并且 属性信息有效  并且 没有 static 标记
			if (property_info != ZEND_WRONG_PROPERTY_INFO &&
			    property_info &&
			    (property_info->flags & ZEND_ACC_STATIC) == 0) {
				// 使用偏移量取得 对象中的属性
				zval *slot = OBJ_PROP(object, property_info->offset);
				// 如果属性信息有类型要求
				if (UNEXPECTED(ZEND_TYPE_IS_SET(property_info->type))) {
					zval tmp;
					// 先复制到临时变量里
					ZVAL_COPY_VALUE(&tmp, prop);
					// 检查属性值类型 与 属性信息要求的类型是否匹配，如果失败
					if (UNEXPECTED(!zend_verify_property_type(property_info, &tmp, 0))) {
						// 下一个
						continue;
					}
					// 通过验证，更新属性值
					ZVAL_COPY_VALUE(slot, &tmp);
				// 属性信息没有类型要求
				} else {
					// 直接更新属性值
					ZVAL_COPY_VALUE(slot, prop);
				}
				// prop 添加间接引用到 slot 位置
				ZVAL_INDIRECT(prop, slot);
			}
		} ZEND_HASH_FOREACH_END();
	}
}
/* }}} */

// ing3, 给对象加载属性表
ZEND_API void object_properties_load(zend_object *object, HashTable *properties) /* {{{ */
{
	zval *prop, tmp;
	zend_string *key;
	zend_long h;
	zend_property_info *property_info;

	// 遍历属性表
	ZEND_HASH_FOREACH_KEY_VAL(properties, h, key, prop) {
		// 如果有属性名
		if (key) {
			// 如果属性名为空
			if (ZSTR_VAL(key)[0] == '\0') {
				const char *class_name, *prop_name;
				size_t prop_name_len;
				// 属性名：拚接类名和属性名
				if (zend_unmangle_property_name_ex(key, &class_name, &prop_name, &prop_name_len) == SUCCESS) {
					// 创建 属性名 zend_string
					zend_string *pname = zend_string_init(prop_name, prop_name_len, 0);
					// 前一个临时域
					zend_class_entry *prev_scope = EG(fake_scope);
					// 如果有类名 并且不是 *
					if (class_name && class_name[0] != '*') {
						// 创建类名 zend_string
						zend_string *cname = zend_string_init(class_name, strlen(class_name), 0);
						// 切换临时域
						EG(fake_scope) = zend_lookup_class(cname);
						// 释放类名
						zend_string_release_ex(cname, 0);
					}
					// 从类中获取属性信息
					property_info = zend_get_property_info(object->ce, pname, 1);
					// 释放属性名
					zend_string_release_ex(pname, 0);
					// 还原临时域
					EG(fake_scope) = prev_scope;
				// 如果创建属性名失败
				} else {
					// 错误信息：属性信息错误
					property_info = ZEND_WRONG_PROPERTY_INFO;
				}
			// 如果属性名有效
			} else {
				// 从类里获取属性信息
				property_info = zend_get_property_info(object->ce, key, 1);
			}
			// 如果获取属性信息成功 并且 属性信息有效 并且 不是 static
			if (property_info != ZEND_WRONG_PROPERTY_INFO &&
				property_info &&
				(property_info->flags & ZEND_ACC_STATIC) == 0) {
				// 获取属性值
				zval *slot = OBJ_PROP(object, property_info->offset);
				// 清空此属性值
				zval_ptr_dtor(slot);
				// 更新此属性值
				ZVAL_COPY_VALUE(slot, prop);
				// 增加引用次数
				zval_add_ref(slot);
				// 如果对象有属性表
				if (object->properties) {
					// 把此属性添加间接引用到临时变量里
					ZVAL_INDIRECT(&tmp, slot);
					// 更新属性表
					zend_hash_update(object->properties, key, &tmp);
				}
			// 如果获取属性信息失败 或者 属性是 static
			} else {
				// 如果类不允许动态属性
				if (UNEXPECTED(object->ce->ce_flags & ZEND_ACC_NO_DYNAMIC_PROPERTIES)) {
					// 报错：无法创建动态属性
					zend_throw_error(NULL, "Cannot create dynamic property %s::$%s",
						ZSTR_VAL(object->ce->name), property_info != ZEND_WRONG_PROPERTY_INFO ? zend_get_unmangled_property_name(key): "");
					// 中断
					return;
				// 如果类 没有允许动态属性 标记
				} else if (!(object->ce->ce_flags & ZEND_ACC_ALLOW_DYNAMIC_PROPERTIES)) {
					// 弃用报错：无法创建动态属性
					zend_error(E_DEPRECATED, "Creation of dynamic property %s::$%s is deprecated",
						ZSTR_VAL(object->ce->name), property_info != ZEND_WRONG_PROPERTY_INFO ? zend_get_unmangled_property_name(key): "");
				}

				// 如果对象没有属性表
				if (!object->properties) {
					// 重新创建属性表
					rebuild_object_properties(object);
				}
				// 更新属性表
				prop = zend_hash_update(object->properties, key, prop);
				// 增加引用次数
				zval_add_ref(prop);
			}
		// 如果没有属性名
		} else {
			// 如果类不允许动态属性
			if (UNEXPECTED(object->ce->ce_flags & ZEND_ACC_NO_DYNAMIC_PROPERTIES)) {
				// 报错：无法创建动态属性
				zend_throw_error(NULL, "Cannot create dynamic property %s::$" ZEND_LONG_FMT, ZSTR_VAL(object->ce->name), h);
				// 中断
				return;
			// 如果类 没有允许动态属性 标记
			} else if (!(object->ce->ce_flags & ZEND_ACC_ALLOW_DYNAMIC_PROPERTIES)) {
				// 弃用报错：无法创建动态属性
				zend_error(E_DEPRECATED, "Creation of dynamic property %s::$" ZEND_LONG_FMT " is deprecated",
					ZSTR_VAL(object->ce->name), h);
			}
			// 如果对象没有属性表
			if (!object->properties) {
				// 重新创建属性表
				rebuild_object_properties(object);
			}
			// 用哈希值，更新属性表
			prop = zend_hash_index_update(object->properties, h, prop);
			// 增加引用次数
			zval_add_ref(prop);
		}
	} ZEND_HASH_FOREACH_END();
}
/* }}} */

// 这个函数需要 proerties 来存放所有在类中声名的 属性，所有属性都是public
// 如果只是给了一个子集或类有受保护的成员，需要分别 调用zend_merge_properties来 合并这些 属性
/* This function requires 'properties' to contain all props declared in the
 * class and all props being public. If only a subset is given or the class
 * has protected members then you need to merge the properties separately by
 * calling zend_merge_properties(). */
 
// ing3, 初始化对象和属性列表，把新创建的对象关系到 arg
static zend_always_inline zend_result _object_and_properties_init(zval *arg, zend_class_entry *class_type, HashTable *properties) /* {{{ */
{
	// 如果类入口是 接口、trait、显式抽象类、隐式抽象类 或 ENUM，都要报错
	if (UNEXPECTED(class_type->ce_flags & (ZEND_ACC_INTERFACE|ZEND_ACC_TRAIT|ZEND_ACC_IMPLICIT_ABSTRACT_CLASS|ZEND_ACC_EXPLICIT_ABSTRACT_CLASS|ZEND_ACC_ENUM))) {
		// 不可以实例化接口
		if (class_type->ce_flags & ZEND_ACC_INTERFACE) {
			zend_throw_error(NULL, "Cannot instantiate interface %s", ZSTR_VAL(class_type->name));
		// 不可以实例化 trait
		} else if (class_type->ce_flags & ZEND_ACC_TRAIT) {
			zend_throw_error(NULL, "Cannot instantiate trait %s", ZSTR_VAL(class_type->name));
		// 不可以实例化 enum
		} else if (class_type->ce_flags & ZEND_ACC_ENUM) {
			zend_throw_error(NULL, "Cannot instantiate enum %s", ZSTR_VAL(class_type->name));
		// 不可以实例化 抽象类
		} else {
			zend_throw_error(NULL, "Cannot instantiate abstract class %s", ZSTR_VAL(class_type->name));
		}
		// arg值为NULL
		ZVAL_NULL(arg);
		// 清空arg的对象指针
		Z_OBJ_P(arg) = NULL;
		// 返回：失败
		return FAILURE;
	}
	
	// 这个是 private 声名的属性或常量，参见: zend_inheritance.c
	if (UNEXPECTED(!(class_type->ce_flags & ZEND_ACC_CONSTANTS_UPDATED))) {
		// 如果更新类常量（常量，静态属性，普通属性）失败
		if (UNEXPECTED(zend_update_class_constants(class_type) != SUCCESS)) {
			// 值设置成null
			ZVAL_NULL(arg);
			// 下属对象元素也是null
			Z_OBJ_P(arg) = NULL;
			// 返回失败
			return FAILURE;
		}
	}
	
	// 如果此类 没有 create_object 方法
	if (class_type->create_object == NULL) {
		// 创建新对象
		zend_object *obj = zend_objects_new(class_type);
		// 对象加关联一 arg
		ZVAL_OBJ(arg, obj);
		// 如果有属性表
		if (properties) {
			// 给对象添加已有属性
			object_properties_init_ex(obj, properties);
		// 如果没有
		} else {
			// 按类结构添加属性
			_object_properties_init(obj, class_type);
		}
	// 如果有 create_object 方法
	} else {
		// 调用此方法创建对象并关联到 arg
		ZVAL_OBJ(arg, class_type->create_object(class_type));
	}
	return SUCCESS;
}
/* }}} */

// ing4, 初始化对象并添加属性，传入zval指针和类入口，属性哈希表
ZEND_API zend_result object_and_properties_init(zval *arg, zend_class_entry *class_type, HashTable *properties) /* {{{ */
{
	return _object_and_properties_init(arg, class_type, properties);
}
/* }}} */

// ing4, 初始化对象，传入zval指针和类入口
ZEND_API zend_result object_init_ex(zval *arg, zend_class_entry *class_type) /* {{{ */
{
	return _object_and_properties_init(arg, class_type, NULL);
}
/* }}} */

// ing4, 创建标准对象，放在zval里返回
ZEND_API void object_init(zval *arg) /* {{{ */
{
	// stdClass
	ZVAL_OBJ(arg, zend_objects_new(zend_standard_class_def));
}
/* }}} */


// ing4, 更新符号表，值为整数
ZEND_API void add_assoc_long_ex(zval *arg, const char *key, size_t key_len, zend_long n) /* {{{ */
{
	zval tmp;

	ZVAL_LONG(&tmp, n);
	// 更新符号表
	zend_symtable_str_update(Z_ARRVAL_P(arg), key, key_len, &tmp);
}
/* }}} */

// ing4, 更新符号表，值为null
ZEND_API void add_assoc_null_ex(zval *arg, const char *key, size_t key_len) /* {{{ */
{
	zval tmp;

	ZVAL_NULL(&tmp);
	// 更新符号表
	zend_symtable_str_update(Z_ARRVAL_P(arg), key, key_len, &tmp);
}
/* }}} */

// ing4, 更新符号表，值为布尔型
ZEND_API void add_assoc_bool_ex(zval *arg, const char *key, size_t key_len, bool b) /* {{{ */
{
	zval tmp;

	ZVAL_BOOL(&tmp, b);
	// 更新符号表
	zend_symtable_str_update(Z_ARRVAL_P(arg), key, key_len, &tmp);
}
/* }}} */

// ing4, 更新符号表，值为资源型
ZEND_API void add_assoc_resource_ex(zval *arg, const char *key, size_t key_len, zend_resource *r) /* {{{ */
{
	zval tmp;

	ZVAL_RES(&tmp, r);
	// 更新符号表
	zend_symtable_str_update(Z_ARRVAL_P(arg), key, key_len, &tmp);
}
/* }}} */

// ing4, 更新符号表，值为小数
ZEND_API void add_assoc_double_ex(zval *arg, const char *key, size_t key_len, double d) /* {{{ */
{
	zval tmp;

	ZVAL_DOUBLE(&tmp, d);
	// 更新符号表
	zend_symtable_str_update(Z_ARRVAL_P(arg), key, key_len, &tmp);
}
/* }}} */

// ing4, 更新符号表，值为 zend_string
ZEND_API void add_assoc_str_ex(zval *arg, const char *key, size_t key_len, zend_string *str) /* {{{ */
{
	zval tmp;

	ZVAL_STR(&tmp, str);
	// 更新符号表
	zend_symtable_str_update(Z_ARRVAL_P(arg), key, key_len, &tmp);
}
/* }}} */

// ing4, 更新符号表，值为 字串
ZEND_API void add_assoc_string_ex(zval *arg, const char *key, size_t key_len, const char *str) /* {{{ */
{
	zval tmp;

	ZVAL_STRING(&tmp, str);
	// 更新符号表
	zend_symtable_str_update(Z_ARRVAL_P(arg), key, key_len, &tmp);
}
/* }}} */

// ing4, 更新符号表，值为 字串和长度
ZEND_API void add_assoc_stringl_ex(zval *arg, const char *key, size_t key_len, const char *str, size_t length) /* {{{ */
{
	zval tmp;

	ZVAL_STRINGL(&tmp, str, length);
	// 更新符号表
	zend_symtable_str_update(Z_ARRVAL_P(arg), key, key_len, &tmp);
}
/* }}} */

// ing4, 更新符号表，值为数组
ZEND_API void add_assoc_array_ex(zval *arg, const char *key, size_t key_len, zend_array *arr) /* {{{ */
{
	zval tmp;

	ZVAL_ARR(&tmp, arr);
	// 更新符号表
	zend_symtable_str_update(Z_ARRVAL_P(arg), key, key_len, &tmp);
}
/* }}} */

// ing4, 更新符号表，值为对象
ZEND_API void add_assoc_object_ex(zval *arg, const char *key, size_t key_len, zend_object *obj) /* {{{ */
{
	zval tmp;

	ZVAL_OBJ(&tmp, obj);
	// 更新符号表
	zend_symtable_str_update(Z_ARRVAL_P(arg), key, key_len, &tmp);
}
/* }}} */

// ing4, 更新符号表，值为 引用
ZEND_API void add_assoc_reference_ex(zval *arg, const char *key, size_t key_len, zend_reference *ref) /* {{{ */
{
	zval tmp;

	ZVAL_REF(&tmp, ref);
	// 更新符号表
	zend_symtable_str_update(Z_ARRVAL_P(arg), key, key_len, &tmp);
}
/* }}} */

// ing4, 更新符号表，值为 zval
ZEND_API void add_assoc_zval_ex(zval *arg, const char *key, size_t key_len, zval *value) /* {{{ */
{
	// 更新符号表
	zend_symtable_str_update(Z_ARRVAL_P(arg), key, key_len, value);
}
/* }}} */

// ing4, 用哈希值更新zval中的 哈希表，值为整数
ZEND_API void add_index_long(zval *arg, zend_ulong index, zend_long n) /* {{{ */
{
	zval tmp;

	ZVAL_LONG(&tmp, n);	
	zend_hash_index_update(Z_ARRVAL_P(arg), index, &tmp);
}
/* }}} */

// ing4, 用哈希值更新zval中的 哈希表，值为null
ZEND_API void add_index_null(zval *arg, zend_ulong index) /* {{{ */
{
	zval tmp;

	ZVAL_NULL(&tmp);
	zend_hash_index_update(Z_ARRVAL_P(arg), index, &tmp);
}
/* }}} */

// ing4, 用哈希值更新zval中的 哈希表，值为布尔型
ZEND_API void add_index_bool(zval *arg, zend_ulong index, bool b) /* {{{ */
{
	zval tmp;

	ZVAL_BOOL(&tmp, b);
	zend_hash_index_update(Z_ARRVAL_P(arg), index, &tmp);
}
/* }}} */

// ing4, 用哈希值更新zval中的 哈希表，值为资源
ZEND_API void add_index_resource(zval *arg, zend_ulong index, zend_resource *r) /* {{{ */
{
	zval tmp;

	ZVAL_RES(&tmp, r);
	zend_hash_index_update(Z_ARRVAL_P(arg), index, &tmp);
}
/* }}} */

// ing4, 用哈希值更新zval中的 哈希表，值为小数
ZEND_API void add_index_double(zval *arg, zend_ulong index, double d) /* {{{ */
{
	zval tmp;

	ZVAL_DOUBLE(&tmp, d);
	zend_hash_index_update(Z_ARRVAL_P(arg), index, &tmp);
}
/* }}} */

// ing4, 用哈希值更新zval中的 哈希表，值为 zend_string
ZEND_API void add_index_str(zval *arg, zend_ulong index, zend_string *str) /* {{{ */
{
	zval tmp;

	ZVAL_STR(&tmp, str);
	zend_hash_index_update(Z_ARRVAL_P(arg), index, &tmp);
}
/* }}} */

// ing4, 用哈希值更新zval中的 哈希表，值为字串
ZEND_API void add_index_string(zval *arg, zend_ulong index, const char *str) /* {{{ */
{
	zval tmp;

	ZVAL_STRING(&tmp, str);
	zend_hash_index_update(Z_ARRVAL_P(arg), index, &tmp);
}
/* }}} */

// ing4, 用哈希值更新zval中的 哈希表，值为 字串和长度
ZEND_API void add_index_stringl(zval *arg, zend_ulong index, const char *str, size_t length) /* {{{ */
{
	zval tmp;

	ZVAL_STRINGL(&tmp, str, length);
	zend_hash_index_update(Z_ARRVAL_P(arg), index, &tmp);
}
/* }}} */

// ing4, 用哈希值更新zval中的 哈希表，值为数组
ZEND_API void add_index_array(zval *arg, zend_ulong index, zend_array *arr) /* {{{ */
{
	zval tmp;

	ZVAL_ARR(&tmp, arr);
	zend_hash_index_update(Z_ARRVAL_P(arg), index, &tmp);
}
/* }}} */

// ing4, 用哈希值更新zval中的 哈希表，值为对象
ZEND_API void add_index_object(zval *arg, zend_ulong index, zend_object *obj) /* {{{ */
{
	zval tmp;

	ZVAL_OBJ(&tmp, obj);
	zend_hash_index_update(Z_ARRVAL_P(arg), index, &tmp);
}
/* }}} */

// ing4, 用哈希值更新zval中的 哈希表，值为引用类型
ZEND_API void add_index_reference(zval *arg, zend_ulong index, zend_reference *ref) /* {{{ */
{
	zval tmp;

	ZVAL_REF(&tmp, ref);
	zend_hash_index_update(Z_ARRVAL_P(arg), index, &tmp);
}
/* }}} */

// ing4, 向顺序哈希表中添加元素，值为 整数
ZEND_API zend_result add_next_index_long(zval *arg, zend_long n) /* {{{ */
{
	zval tmp;

	ZVAL_LONG(&tmp, n);
	return zend_hash_next_index_insert(Z_ARRVAL_P(arg), &tmp) ? SUCCESS : FAILURE;
}
/* }}} */

// ing4, 向顺序哈希表中添加元素，值为 null
ZEND_API zend_result add_next_index_null(zval *arg) /* {{{ */
{
	zval tmp;

	ZVAL_NULL(&tmp);
	return zend_hash_next_index_insert(Z_ARRVAL_P(arg), &tmp) ? SUCCESS : FAILURE;
}
/* }}} */

// ing4, 向顺序哈希表中添加元素，值为 布尔型
ZEND_API zend_result add_next_index_bool(zval *arg, bool b) /* {{{ */
{
	zval tmp;

	ZVAL_BOOL(&tmp, b);
	return zend_hash_next_index_insert(Z_ARRVAL_P(arg), &tmp) ? SUCCESS : FAILURE;
}
/* }}} */

// ing4, 向顺序哈希表中添加元素，值为 资源型
ZEND_API zend_result add_next_index_resource(zval *arg, zend_resource *r) /* {{{ */
{
	zval tmp;

	ZVAL_RES(&tmp, r);
	return zend_hash_next_index_insert(Z_ARRVAL_P(arg), &tmp) ? SUCCESS : FAILURE;
}
/* }}} */

// ing4, 向顺序哈希表中添加元素，值为 小数
ZEND_API zend_result add_next_index_double(zval *arg, double d) /* {{{ */
{
	zval tmp;

	ZVAL_DOUBLE(&tmp, d);
	return zend_hash_next_index_insert(Z_ARRVAL_P(arg), &tmp) ? SUCCESS : FAILURE;
}
/* }}} */

// ing4, 向顺序哈希表中添加元素，值为 zend_string
ZEND_API zend_result add_next_index_str(zval *arg, zend_string *str) /* {{{ */
{
	zval tmp;

	ZVAL_STR(&tmp, str);
	return zend_hash_next_index_insert(Z_ARRVAL_P(arg), &tmp) ? SUCCESS : FAILURE;
}
/* }}} */

// ing4, 向顺序哈希表中添加元素，值为 字串
ZEND_API zend_result add_next_index_string(zval *arg, const char *str) /* {{{ */
{
	zval tmp;

	ZVAL_STRING(&tmp, str);
	return zend_hash_next_index_insert(Z_ARRVAL_P(arg), &tmp) ? SUCCESS : FAILURE;
}
/* }}} */

// ing4, 向顺序哈希表中添加元素，值为 字串和长度
ZEND_API zend_result add_next_index_stringl(zval *arg, const char *str, size_t length) /* {{{ */
{
	zval tmp;

	ZVAL_STRINGL(&tmp, str, length);
	return zend_hash_next_index_insert(Z_ARRVAL_P(arg), &tmp) ? SUCCESS : FAILURE;
}
/* }}} */

// ing4, 向顺序哈希表中添加元素，值为 数组
ZEND_API zend_result add_next_index_array(zval *arg, zend_array *arr) /* {{{ */
{
	zval tmp;

	ZVAL_ARR(&tmp, arr);
	return zend_hash_next_index_insert(Z_ARRVAL_P(arg), &tmp) ? SUCCESS : FAILURE;
}
/* }}} */

// ing4, 向顺序哈希表中添加元素，值为 对象
ZEND_API zend_result add_next_index_object(zval *arg, zend_object *obj) /* {{{ */
{
	zval tmp;

	ZVAL_OBJ(&tmp, obj);
	return zend_hash_next_index_insert(Z_ARRVAL_P(arg), &tmp) ? SUCCESS : FAILURE;
}
/* }}} */

// ing4, 向顺序哈希表中添加元素，值为 引用类型
ZEND_API zend_result add_next_index_reference(zval *arg, zend_reference *ref) /* {{{ */
{
	zval tmp;

	ZVAL_REF(&tmp, ref);
	return zend_hash_next_index_insert(Z_ARRVAL_P(arg), &tmp) ? SUCCESS : FAILURE;
}
/* }}} */

// ing4, 根据传入key的类型，更新符号表
ZEND_API zend_result array_set_zval_key(HashTable *ht, zval *key, zval *value) /* {{{ */
{
	zval *result;

	// 根据key的类型操作
	switch (Z_TYPE_P(key)) {
		// 字串
		case IS_STRING:
			// 用键名更新，键为传入的key
			result = zend_symtable_update(ht, Z_STR_P(key), value);
			break;
		// NULL
		case IS_NULL:
			// 用键名更新，键为空字串
			result = zend_hash_update(ht, ZSTR_EMPTY_ALLOC(), value);
			break;
		// 资源
		case IS_RESOURCE:
			//  抛出警告：资源作为偏移量，转成了数字（自己的ID）
			zend_use_resource_as_offset(key);
			// 用哈希值更新，哈希值为资源的ID
			result = zend_hash_index_update(ht, Z_RES_HANDLE_P(key), value);
			break;
		// false
		case IS_FALSE:
			// 用哈希值更新，哈希值为 0
			result = zend_hash_index_update(ht, 0, value);
			break;
		// true
		case IS_TRUE:
			// 用哈希值更新，哈希值为 1
			result = zend_hash_index_update(ht, 1, value);
			break;
		// 整数 
		case IS_LONG:
			// 用哈希值更新，哈希值为 key 转成的整数
			result = zend_hash_index_update(ht, Z_LVAL_P(key), value);
			break;
		// 小数
		case IS_DOUBLE:
			// 用哈希值更新，哈希值为 key 转成的整数
			result = zend_hash_index_update(ht, zend_dval_to_lval_safe(Z_DVAL_P(key)), value);
			break;
		// 其他情况
		default:
			zend_type_error("Illegal offset type");
			result = NULL;
	}
	// 如果成功
	if (result) {
		// 结果增加引用次数
		Z_TRY_ADDREF_P(result);
		// 成功
		return SUCCESS;
	// 失败
	} else {
		return FAILURE;
	}
}
/* }}} */

// ing4, 给的对象添加属性,传入属性名，值为 整数
ZEND_API void add_property_long_ex(zval *arg, const char *key, size_t key_len, zend_long n) /* {{{ */
{
	zval tmp;

	ZVAL_LONG(&tmp, n);
	add_property_zval_ex(arg, key, key_len, &tmp);
}
/* }}} */

// ing4, 给的对象添加属性,传入属性名，值为 布尔型
ZEND_API void add_property_bool_ex(zval *arg, const char *key, size_t key_len, zend_long b) /* {{{ */
{
	zval tmp;

	ZVAL_BOOL(&tmp, b);
	add_property_zval_ex(arg, key, key_len, &tmp);
}
/* }}} */

// ing4, 给的对象添加属性,传入属性名，值为 null
ZEND_API void add_property_null_ex(zval *arg, const char *key, size_t key_len) /* {{{ */
{
	zval tmp;

	ZVAL_NULL(&tmp);
	add_property_zval_ex(arg, key, key_len, &tmp);
}
/* }}} */

// ing4, 给的对象添加属性,传入属性名，值为 资源
ZEND_API void add_property_resource_ex(zval *arg, const char *key, size_t key_len, zend_resource *r) /* {{{ */
{
	zval tmp;

	ZVAL_RES(&tmp, r);
	add_property_zval_ex(arg, key, key_len, &tmp);
	zval_ptr_dtor(&tmp); /* write_property will add 1 to refcount */
}
/* }}} */

// ing4, 给的对象添加属性,传入属性名，值为 小数
ZEND_API void add_property_double_ex(zval *arg, const char *key, size_t key_len, double d) /* {{{ */
{
	zval tmp;

	ZVAL_DOUBLE(&tmp, d);
	add_property_zval_ex(arg, key, key_len, &tmp);
}
/* }}} */

// ing4, 给的对象添加属性,传入属性名，值为 zend_string
ZEND_API void add_property_str_ex(zval *arg, const char *key, size_t key_len, zend_string *str) /* {{{ */
{
	zval tmp;

	ZVAL_STR(&tmp, str);
	add_property_zval_ex(arg, key, key_len, &tmp);
	zval_ptr_dtor(&tmp); /* write_property will add 1 to refcount */
}
/* }}} */

// ing4, 给的对象添加属性,传入属性名，值为 字串
ZEND_API void add_property_string_ex(zval *arg, const char *key, size_t key_len, const char *str) /* {{{ */
{
	zval tmp;

	ZVAL_STRING(&tmp, str);
	add_property_zval_ex(arg, key, key_len, &tmp);
	zval_ptr_dtor(&tmp); /* write_property will add 1 to refcount */
}
/* }}} */

// ing4, 给的对象添加属性,传入属性名，值为 字串和长度
ZEND_API void add_property_stringl_ex(zval *arg, const char *key, size_t key_len, const char *str, size_t length) /* {{{ */
{
	zval tmp;

	ZVAL_STRINGL(&tmp, str, length);
	add_property_zval_ex(arg, key, key_len, &tmp);
	zval_ptr_dtor(&tmp); /* write_property will add 1 to refcount */
}
/* }}} */

// ing4, 给的对象添加属性,传入属性名，值为 数组
ZEND_API void add_property_array_ex(zval *arg, const char *key, size_t key_len, zend_array *arr) /* {{{ */
{
	zval tmp;

	ZVAL_ARR(&tmp, arr);
	add_property_zval_ex(arg, key, key_len, &tmp);
	zval_ptr_dtor(&tmp); /* write_property will add 1 to refcount */
}
/* }}} */

// ing4, 给的对象添加属性,传入属性名，值为 对象
ZEND_API void add_property_object_ex(zval *arg, const char *key, size_t key_len, zend_object *obj) /* {{{ */
{
	zval tmp;

	ZVAL_OBJ(&tmp, obj);
	add_property_zval_ex(arg, key, key_len, &tmp);
	zval_ptr_dtor(&tmp); /* write_property will add 1 to refcount */
}
/* }}} */

// ing4, 给的对象添加属性,传入属性名，值为 引用类型
ZEND_API void add_property_reference_ex(zval *arg, const char *key, size_t key_len, zend_reference *ref) /* {{{ */
{
	zval tmp;

	ZVAL_REF(&tmp, ref);
	add_property_zval_ex(arg, key, key_len, &tmp);
	zval_ptr_dtor(&tmp); /* write_property will add 1 to refcount */
}
/* }}} */

// ing4, 给的对象添加属性, p1:zval, p2:属性名, p3:属性名长度, p4: 属性值
ZEND_API void add_property_zval_ex(zval *arg, const char *key, size_t key_len, zval *value) /* {{{ */
{
	zend_string *str;
	// 初始化key
	str = zend_string_init(key, key_len, 0);
	// 调用对象的 write_property 方法，p1:对象本身，p2:属性名，p3，属性值，p4: void **cache_slot
	Z_OBJ_HANDLER_P(arg, write_property)(Z_OBJ_P(arg), str, value, NULL);
	// 释放属性名
	zend_string_release_ex(str, 0);
}
/* }}} */

// ing4, 启动模块，先检查依赖模块，再调用模块的启动方法
ZEND_API zend_result zend_startup_module_ex(zend_module_entry *module) /* {{{ */
{
	size_t name_len;
	zend_string *lcname;
	// 如果已经启动
	if (module->module_started) {
		// 直接返回成功
		return SUCCESS;
	}
	// 已经启动
	module->module_started = 1;

	// 检查模块依赖
	/* Check module dependencies */
	// 如果块有依赖
	if (module->deps) {
		// 依赖列表开头
		const zend_module_dep *dep = module->deps;
		// 如果依赖模块有名称
		while (dep->name) {
			// 如果有依赖需要标记
			if (dep->type == MODULE_DEP_REQUIRED) {
				zend_module_entry *req_mod;
				// 模块名称长度
				name_len = strlen(dep->name);
				// 分配字串
				lcname = zend_string_alloc(name_len, 0);
				// 小写模块名称
				zend_str_tolower_copy(ZSTR_VAL(lcname), dep->name, name_len);
				// 如果找不到模块 或 模块没有启动
				if ((req_mod = zend_hash_find_ptr(&module_registry, lcname)) == NULL || !req_mod->module_started) {
					// 清空模块名
					zend_string_efree(lcname);
					/* TODO: Check version relationship */
					// 无法加载模块，因为依赖的模块 A 未加载
					zend_error(E_CORE_WARNING, "Cannot load module \"%s\" because required module \"%s\" is not loaded", module->name, dep->name);
					// 未启动
					module->module_started = 0;
					// 返回失败
					return FAILURE;
				}
				// 释放模块名
				zend_string_efree(lcname);
			}
			// 下一个依赖的模块
			++dep;
		}
	}
	
	// 初始化模块 globals
	/* Initialize module globals */
	
	// 如果有全局大小
	if (module->globals_size) {
// 如果要求线程安全
#ifdef ZTS
		ts_allocate_id(module->globals_id_ptr, module->globals_size, (ts_allocate_ctor) module->globals_ctor, (ts_allocate_dtor) module->globals_dtor);
#else
		// 如果模块有全局构造器
		if (module->globals_ctor) {
			// 调用全局构造器 
			module->globals_ctor(module->globals_ptr);
		}
#endif
	}
	
	// 如果有启动方法
	if (module->module_startup_func) {
		// 记录 当前模块
		EG(current_module) = module;
		// 调用启动方法，如果失败
		if (module->module_startup_func(module->type, module->module_number)==FAILURE) {
			// 报错：无法启动 A 模块
			zend_error_noreturn(E_CORE_ERROR,"Unable to start %s module", module->name);
			// 清空当前模块
			EG(current_module) = NULL;
			// 启动失败
			return FAILURE;
		}
		// 清空当前模块
		EG(current_module) = NULL;
	}
	// 返回成功
	return SUCCESS;
}
/* }}} */

// ing4, 启动指定模块，成功则返回从哈希表中删除指令
static int zend_startup_module_zval(zval *zv) /* {{{ */
{
	// zv中的模块指针
	zend_module_entry *module = Z_PTR_P(zv);
	// 启动模块，先检查依赖模块，再调用模块的启动方法。成功则返回从哈希表中删除指令
	return (zend_startup_module_ex(module) == SUCCESS) ? ZEND_HASH_APPLY_KEEP : ZEND_HASH_APPLY_REMOVE;
}
/* }}} */

// ing3, 给模块列表中的模块排序，把被依赖的排到前面来
static void zend_sort_modules(void *base, size_t count, size_t siz, compare_func_t compare, swap_func_t swp) /* {{{ */
{
	// 指针列表开头
	Bucket *b1 = base;
	// 循环变量
	Bucket *b2;
	// 指针列表结束
	Bucket *end = b1 + count;
	// 临时变量
	Bucket tmp;
	// 模块入口
	zend_module_entry *m, *r;
	
	// 从头到尾遍历
	while (b1 < end) {
try_again:
		// 所属模块
		m = (zend_module_entry*)Z_PTR(b1->val);
		// 如果模块未启动 或 有依赖模块
		if (!m->module_started && m->deps) {
			// 依赖模块列表
			const zend_module_dep *dep = m->deps;
			// 遍历依赖模块
			while (dep->name) {
				// 如果类型是必须依赖 或 可选依赖
				if (dep->type == MODULE_DEP_REQUIRED || dep->type == MODULE_DEP_OPTIONAL) {
					// 下一个元素
					b2 = b1 + 1;
					// 一直追到结尾
					while (b2 < end) {
						// 取出模块
						r = (zend_module_entry*)Z_PTR(b2->val);
						// 找到依赖的模块
						if (strcasecmp(dep->name, r->name) == 0) {
							// 交换b1,b2。此模块和依赖的模块，调换位置
							tmp = *b1;
							*b1 = *b2;
							*b2 = tmp;
							// 同一个位置，再来一轮
							goto try_again;
						}
						// 名字不相同，b2 下一个
						b2++;
					}
					//
				}
				// 下一个依赖模块
				dep++;
			}
		}
		// 下一个模块
		b1++;
	}
}
/* }}} */

// ing4, 收集模块的触发器放入列表, 4种：启动时触发，关闭时触发，暂停时触发，清理内置内的静态变量
ZEND_API void zend_collect_module_handlers(void) /* {{{ */
{
	zend_module_entry *module;
	int startup_count = 0;
	int shutdown_count = 0;
	int post_deactivate_count = 0;
	zend_class_entry *ce;
	int class_count = 0;

	// 收集有 request_startup_func/request_shutdown_func/post_deactivate_func 的扩展
	/* Collect extensions with request startup/shutdown handlers */
	ZEND_HASH_MAP_FOREACH_PTR(&module_registry, module) {
		// 如果有 request_startup_func
		if (module->request_startup_func) {
			// 需要启动时调用的，计数增加
			startup_count++;
		}
		// 如果有 request_shutdown_func
		if (module->request_shutdown_func) {
			// 需要关闭时调用的，计数增加
			shutdown_count++;
		}
		// 如果有 post_deactivate_func
		if (module->post_deactivate_func) {
			// 需要暂停时调用的，计数增加
			post_deactivate_count++;
		}
	} ZEND_HASH_FOREACH_END();
	
	// 创建模块指针 触发器列表，上面3个列表放在同一个列表里。每个列表放一个空指针来标记结束位置
	module_request_startup_handlers = (zend_module_entry**)malloc(
	    sizeof(zend_module_entry*) *
		(startup_count + 1 +
		 shutdown_count + 1 +
		 post_deactivate_count + 1));
	// 启动列表的结尾标记
	module_request_startup_handlers[startup_count] = NULL;
	// 关闭列表的结尾
	module_request_shutdown_handlers = module_request_startup_handlers + startup_count + 1;
	// 关闭列表的结尾标记
	module_request_shutdown_handlers[shutdown_count] = NULL;
	// 暂停列表的结尾
	module_post_deactivate_handlers = module_request_shutdown_handlers + shutdown_count + 1;
	// 暂停列表的结尾标记
	module_post_deactivate_handlers[post_deactivate_count] = NULL;
	// 开始列表循环计数
	startup_count = 0;

	// 在3个列表里添加模块
	ZEND_HASH_MAP_FOREACH_PTR(&module_registry, module) {
		// 如果有 request_startup_func
		if (module->request_startup_func) {
			// 需要启动时调用的列表里添加指针（正序添加）
			module_request_startup_handlers[startup_count++] = module;
		}
		// 如果有 request_shutdown_func
		if (module->request_shutdown_func) {
			// 需要关闭时调用的列表里添加指针（倒着添加）
			module_request_shutdown_handlers[--shutdown_count] = module;
		}
		// 如果有 post_deactivate_func
		if (module->post_deactivate_func) {
			// 需要暂停时调用的列表里添加指针（倒着添加）
			module_post_deactivate_handlers[--post_deactivate_count] = module;
		}
	} ZEND_HASH_FOREACH_END();

	// 收集有静态成员的内部类
	/* Collect internal classes with static members */
	ZEND_HASH_MAP_FOREACH_PTR(CG(class_table), ce) {
		// 内部类 并且有 静态成员
		if (ce->type == ZEND_INTERNAL_CLASS &&
		    ce->default_static_members_count > 0) {
			// 只计数
		    class_count++;
		}
	} ZEND_HASH_FOREACH_END();

	// 需要清理的类列表, 放一个空指针来标记结束位置
	class_cleanup_handlers = (zend_class_entry**)malloc(
		sizeof(zend_class_entry*) *
		(class_count + 1));
	// 最后一个是null
	class_cleanup_handlers[class_count] = NULL;

	// 如果有包含静态成员的类
	if (class_count) {
		// 遍历编译时类表
		ZEND_HASH_MAP_FOREACH_PTR(CG(class_table), ce) {
			// 如果是内置类 并且 有静态成员
			if (ce->type == ZEND_INTERNAL_CLASS &&
			    ce->default_static_members_count > 0) {
				// 倒着添加进列表里
			    class_cleanup_handlers[--class_count] = ce;
			}
		} ZEND_HASH_FOREACH_END();
	}
}
/* }}} */

// ing4, 启动所有模块
ZEND_API void zend_startup_modules(void) /* {{{ */
{
	// 先给所有模块排序 
	zend_hash_sort_ex(&module_registry, zend_sort_modules, NULL, 0);
	//  启动指定模块，成功则返回从哈希表中删除指令
	zend_hash_apply(&module_registry, zend_startup_module_zval);
}
/* }}} */


// ing3, 销毁模块
ZEND_API void zend_destroy_modules(void) /* {{{ */
{
	// 释放，清理静态成员用的列表
	free(class_cleanup_handlers);
	// 释放，需要启动时调用的列表
	free(module_request_startup_handlers);
	// 优雅地逆序销毁
	zend_hash_graceful_reverse_destroy(&module_registry);
}
/* }}} */

// ing3, 注册模块
ZEND_API zend_module_entry* zend_register_module_ex(zend_module_entry *module) /* {{{ */
{
	size_t name_len;
	zend_string *lcname;
	zend_module_entry *module_ptr;

	// 模块无效，返回null
	if (!module) {
		return NULL;
	}

#if 0
	zend_printf("%s: Registering module %d\n", module->name, module->module_number);
#endif

	// 检查模块依赖
	/* Check module dependencies */
	// 如果有依赖模块
	if (module->deps) {
		const zend_module_dep *dep = module->deps;
		// 如果依赖模块有名称
		while (dep->name) {
			// 如果有依赖需要标记
			if (dep->type == MODULE_DEP_CONFLICTS) {
				// 名称长度
				name_len = strlen(dep->name);
				// 新字串
				lcname = zend_string_alloc(name_len, 0);
				// 小写模块名
				zend_str_tolower_copy(ZSTR_VAL(lcname), dep->name, name_len);

				// 如果已有此模块 或 扩展
				if (zend_hash_exists(&module_registry, lcname) || zend_get_extension(dep->name)) {
					// 释放小写名称
					zend_string_efree(lcname);
					/* TODO: Check version relationship */
					// 报错：模块 A 已经存在
					zend_error(E_CORE_WARNING, "Cannot load module \"%s\" because conflicting module \"%s\" is already loaded", module->name, dep->name);
					// 中断
					return NULL;
				}
				// 释放小写模块名
				zend_string_efree(lcname);
			}
			// 下一个依赖模块
			++dep;
		}
	}

	// 模块名长度
	name_len = strlen(module->name);
	// 新字串
	lcname = zend_string_alloc(name_len, module->type == MODULE_PERSISTENT);
	// 复制小写模块名
	zend_str_tolower_copy(ZSTR_VAL(lcname), module->name, name_len);

	// 新建成内置字串
	lcname = zend_new_interned_string(lcname);
	// 如果模块已存在
	if ((module_ptr = zend_hash_add_ptr(&module_registry, lcname, module)) == NULL) {
		// 报错：模块已存在
		zend_error(E_CORE_WARNING, "Module \"%s\" is already loaded", module->name);
		// 释放模块名
		zend_string_release(lcname);
		return NULL;
	}
	// 
	module = module_ptr;
	// 当前模块
	EG(current_module) = module;

	// 如果模块中有函数 并且 注册这些函数失败
	// 注册所有  *library_functions 里的函数到函数哈希表里
	if (module->functions && zend_register_functions(NULL, module->functions, NULL, module->type)==FAILURE) {
		// 删除此模块
		zend_hash_del(&module_registry, lcname);
		// 释放名称
		zend_string_release(lcname);
		// 清空当前模块
		EG(current_module) = NULL;
		// 报错：无法注册模块中的函数 
		zend_error(E_CORE_WARNING,"%s: Unable to register functions, unable to load", module->name);
		return NULL;
	}

	// 清空当前模块
	EG(current_module) = NULL;
	// 清空名称
	zend_string_release(lcname);
	// 返回模块
	return module;
}
/* }}} */

// ing4, 注册内置模块（类型是永久）
ZEND_API zend_module_entry* zend_register_internal_module(zend_module_entry *module) /* {{{ */
{
	// 返回下一个空模块编号
	module->module_number = zend_next_free_module();
	// 模块类型，默认是永久
	module->type = MODULE_PERSISTENT;
	// 注册模块
	return zend_register_module_ex(module);
}
/* }}} */

// ing4, 检验魔术方法的形参：（必须）数量一致，参数不支持引用传递。p1:要求参数数量, p2:类，p3:方法，p4:错误类型
static void zend_check_magic_method_args(
		uint32_t num_args, const zend_class_entry *ce, const zend_function *fptr, int error_type)
{
	// 如果要求的参数数量和传入的不符
	if (fptr->common.num_args != num_args) {
		// 参数数量0
		if (num_args == 0) {
			// 此方法不可以有参数
			zend_error(error_type, "Method %s::%s() cannot take arguments",
				ZSTR_VAL(ce->name), ZSTR_VAL(fptr->common.function_name));
		// 参数数量 1
		} else if (num_args == 1) {
			// 此方法必须有1个参数
			zend_error(error_type, "Method %s::%s() must take exactly 1 argument",
				ZSTR_VAL(ce->name), ZSTR_VAL(fptr->common.function_name));
		// 其他情况
		} else {
			// 此方法必须有n个参数
			zend_error(error_type, "Method %s::%s() must take exactly %" PRIu32 " arguments",
				ZSTR_VAL(ce->name), ZSTR_VAL(fptr->common.function_name), num_args);
		}
		return;
	}
	// 遍历每个参数
	for (uint32_t i = 0; i < num_args; i++) {
		// 检验是否可以通过引用传递, 如果可以
		if (QUICK_ARG_SHOULD_BE_SENT_BY_REF(fptr, i + 1)) {
			// 报错，此方法不可以使用引用类型的参数
			zend_error(error_type, "Method %s::%s() cannot take arguments by reference",
				ZSTR_VAL(ce->name), ZSTR_VAL(fptr->common.function_name));
			// 中断
			return;
		}
	}
}

// ing4, 检验魔术方法 参数的类型。p1:参数序号，p2:类，p3:方法，p4:错误类型，p5:要求的参数类型
static void zend_check_magic_method_arg_type(uint32_t arg_num, const zend_class_entry *ce, const zend_function *fptr, int error_type, int arg_type)
{
		// 如果此参数有类型 并且 与传入的类型不相符
		if (
			ZEND_TYPE_IS_SET(fptr->common.arg_info[arg_num].type)
			 && !(ZEND_TYPE_FULL_MASK(fptr->common.arg_info[arg_num].type) & arg_type)
		) {
			// 参数必须是 A 类型
			zend_error(error_type, "%s::%s(): Parameter #%d ($%s) must be of type %s when declared",
				ZSTR_VAL(ce->name), ZSTR_VAL(fptr->common.function_name),
				arg_num + 1, ZSTR_VAL(fptr->common.arg_info[arg_num].name),
				ZSTR_VAL(zend_type_to_string((zend_type) ZEND_TYPE_INIT_MASK(arg_type))));
		}
}

// ing3, 检验魔术方法 返回值的类型
static void zend_check_magic_method_return_type(const zend_class_entry *ce, const zend_function *fptr, int error_type, int return_type)
{
	// 如果方法没有指定返回类型
	if (!(fptr->common.fn_flags & ZEND_ACC_HAS_RETURN_TYPE)) {
		// 为了向后兼容，返回类型不存在时，不要强制添加
		/* For backwards compatibility reasons, do not enforce the return type if it is not set. */
		// 直接返回
		return;
	}

	// 如果返回类型是never
	if (ZEND_TYPE_PURE_MASK(fptr->common.arg_info[-1].type) & MAY_BE_NEVER) {
		// 指定 never 类型总是合法的
		/* It is always legal to specify the never type. */
		return;
	}

	// 如果是复杂类型
	bool is_complex_type = ZEND_TYPE_IS_COMPLEX(fptr->common.arg_info[-1].type);
	// 取得纯属类型，并mmbw 返回类型
	uint32_t extra_types = ZEND_TYPE_PURE_MASK(fptr->common.arg_info[-1].type) & ~return_type;
	// 如果要求 静态
	if (extra_types & MAY_BE_STATIC) {
		// 添加标记：可以静态
		extra_types &= ~MAY_BE_STATIC;
		// 是复杂类型
		is_complex_type = true;
	}

	// 如果有扩展类型 或 （是复杂类型 并且 类型不是 可以是对象）
	if (extra_types || (is_complex_type && return_type != MAY_BE_OBJECT)) {
		// 报错：返回类型是 return_type 类型
		zend_error(error_type, "%s::%s(): Return type must be %s when declared",
			ZSTR_VAL(ce->name), ZSTR_VAL(fptr->common.function_name),
			ZSTR_VAL(zend_type_to_string((zend_type) ZEND_TYPE_INIT_MASK(return_type))));
	}
}

// ing3, 检验魔术方法 必须不是static, p1:类，p2:方法，p3:错误类型
static void zend_check_magic_method_non_static(
		const zend_class_entry *ce, const zend_function *fptr, int error_type)
{
	if (fptr->common.fn_flags & ZEND_ACC_STATIC) {
		zend_error(error_type, "Method %s::%s() cannot be static",
			ZSTR_VAL(ce->name), ZSTR_VAL(fptr->common.function_name));
	}
}

// ing3, 检验：此魔术方法 必须是static
static void zend_check_magic_method_static(
		const zend_class_entry *ce, const zend_function *fptr, int error_type)
{
	// 如果不是static
	if (!(fptr->common.fn_flags & ZEND_ACC_STATIC)) {
		zend_error(error_type, "Method %s::%s() must be static",
			ZSTR_VAL(ce->name), ZSTR_VAL(fptr->common.function_name));
	}
}

// ing3, 检验：此魔术方法 必须是public
static void zend_check_magic_method_public(
		const zend_class_entry *ce, const zend_function *fptr, int error_type)
{
	// TODO: Remove this warning after adding proper visibility handling.
	// 如果不是public
	if (!(fptr->common.fn_flags & ZEND_ACC_PUBLIC)) {
		// 报错，此魔术方法必须是Public
		zend_error(E_WARNING, "The magic method %s::%s() must have public visibility",
			ZSTR_VAL(ce->name), ZSTR_VAL(fptr->common.function_name));
	}
}

// ing3, 检验：此魔术方法不可以定义返回类型
static void zend_check_magic_method_no_return_type(
		const zend_class_entry *ce, const zend_function *fptr, int error_type)
{
	// 如果有返回类型
	if (fptr->common.fn_flags & ZEND_ACC_HAS_RETURN_TYPE) {
		// 报错 此方法不可以定义返回类型
		zend_error_noreturn(error_type, "Method %s::%s() cannot declare a return type",
			ZSTR_VAL(ce->name), ZSTR_VAL(fptr->common.function_name));
	}
}

// ing3, 检验魔术方法的定义是否符合要求，p1:类，p2:方法，p3:方法名，p4:错误类型
ZEND_API void zend_check_magic_method_implementation(const zend_class_entry *ce, const zend_function *fptr, zend_string *lcname, int error_type) /* {{{ */
{
	// 如果开头不是两个 _ 直接返回
	if (ZSTR_VAL(fptr->common.function_name)[0] != '_'
	 || ZSTR_VAL(fptr->common.function_name)[1] != '_') {
		return;
	}
	// __construct
	if (zend_string_equals_literal(lcname, ZEND_CONSTRUCTOR_FUNC_NAME)) {
		// 检验魔术方法 必须不是static, p1:类，p2:方法，p3:错误类型
		zend_check_magic_method_non_static(ce, fptr, error_type);
		// 检验：此魔术方法不可以定义返回类型
		zend_check_magic_method_no_return_type(ce, fptr, error_type);
	// __destruct
	} else if (zend_string_equals_literal(lcname, ZEND_DESTRUCTOR_FUNC_NAME)) {
		// 检验魔术方法的形参：（必须）数量一致，参数不支持引用传递。p1:要求参数数量, p2:类，p3:方法，p4:错误类型
		zend_check_magic_method_args(0, ce, fptr, error_type);
		// 检验魔术方法 必须不是static, p1:类，p2:方法，p3:错误类型
		zend_check_magic_method_non_static(ce, fptr, error_type);
		// 检验：此魔术方法不可以定义返回类型
		zend_check_magic_method_no_return_type(ce, fptr, error_type);
	// 	__clone
	} else if (zend_string_equals_literal(lcname, ZEND_CLONE_FUNC_NAME)) {
		// 检验魔术方法的形参：（必须）数量一致，参数不支持引用传递。p1:要求参数数量, p2:类，p3:方法，p4:错误类型
		zend_check_magic_method_args(0, ce, fptr, error_type);
		// 检验魔术方法 必须不是static, p1:类，p2:方法，p3:错误类型
		zend_check_magic_method_non_static(ce, fptr, error_type);
		// 检验魔术方法 返回值的类型
		zend_check_magic_method_return_type(ce, fptr, error_type, MAY_BE_VOID);
	// 	__get
	} else if (zend_string_equals_literal(lcname, ZEND_GET_FUNC_NAME)) {
		// 检验魔术方法的形参：（必须）数量一致，参数不支持引用传递。p1:要求参数数量, p2:类，p3:方法，p4:错误类型
		zend_check_magic_method_args(1, ce, fptr, error_type);
		// 检验魔术方法 必须不是static, p1:类，p2:方法，p3:错误类型
		zend_check_magic_method_non_static(ce, fptr, error_type);
		// 检验：此魔术方法 必须是public
		zend_check_magic_method_public(ce, fptr, error_type);
		// 检验魔术方法 参数的类型。p1:参数序号，p2:类，p3:方法，p4:错误类型，p5:要求的参数类型
		zend_check_magic_method_arg_type(0, ce, fptr, error_type, MAY_BE_STRING);
	// 	__set
	} else if (zend_string_equals_literal(lcname, ZEND_SET_FUNC_NAME)) {
		// 检验魔术方法的形参：（必须）数量一致，参数不支持引用传递。p1:要求参数数量, p2:类，p3:方法，p4:错误类型
		zend_check_magic_method_args(2, ce, fptr, error_type);
		// 检验魔术方法 必须不是static, p1:类，p2:方法，p3:错误类型
		zend_check_magic_method_non_static(ce, fptr, error_type);
		// 检验：此魔术方法 必须是public
		zend_check_magic_method_public(ce, fptr, error_type);
		// 检验魔术方法 参数的类型。p1:参数序号，p2:类，p3:方法，p4:错误类型，p5:要求的参数类型
		zend_check_magic_method_arg_type(0, ce, fptr, error_type, MAY_BE_STRING);
		// 检验魔术方法 返回值的类型
		zend_check_magic_method_return_type(ce, fptr, error_type, MAY_BE_VOID);
	// 	__unset
	} else if (zend_string_equals_literal(lcname, ZEND_UNSET_FUNC_NAME)) {
		// 检验魔术方法的形参：（必须）数量一致，参数不支持引用传递。p1:要求参数数量, p2:类，p3:方法，p4:错误类型
		zend_check_magic_method_args(1, ce, fptr, error_type);
		// 检验魔术方法 必须不是static, p1:类，p2:方法，p3:错误类型
		zend_check_magic_method_non_static(ce, fptr, error_type);
		// 检验：此魔术方法 必须是public
		zend_check_magic_method_public(ce, fptr, error_type);
		// 检验魔术方法 参数的类型。p1:参数序号，p2:类，p3:方法，p4:错误类型，p5:要求的参数类型
		zend_check_magic_method_arg_type(0, ce, fptr, error_type, MAY_BE_STRING);
		// 检验魔术方法 返回值的类型
		zend_check_magic_method_return_type(ce, fptr, error_type, MAY_BE_VOID);
	// 	__isset
	} else if (zend_string_equals_literal(lcname, ZEND_ISSET_FUNC_NAME)) {
		// 检验魔术方法的形参：（必须）数量一致，参数不支持引用传递。p1:要求参数数量, p2:类，p3:方法，p4:错误类型
		zend_check_magic_method_args(1, ce, fptr, error_type);
		// 检验魔术方法 必须不是static, p1:类，p2:方法，p3:错误类型
		zend_check_magic_method_non_static(ce, fptr, error_type);
		// 检验：此魔术方法 必须是public
		zend_check_magic_method_public(ce, fptr, error_type);
		// 检验魔术方法 参数的类型。p1:参数序号，p2:类，p3:方法，p4:错误类型，p5:要求的参数类型
		zend_check_magic_method_arg_type(0, ce, fptr, error_type, MAY_BE_STRING);
		// 检验魔术方法 返回值的类型
		zend_check_magic_method_return_type(ce, fptr, error_type, MAY_BE_BOOL);
	// __call
	} else if (zend_string_equals_literal(lcname, ZEND_CALL_FUNC_NAME)) {
		// 检验魔术方法的形参：（必须）数量一致，参数不支持引用传递。p1:要求参数数量, p2:类，p3:方法，p4:错误类型
		zend_check_magic_method_args(2, ce, fptr, error_type);
		// 检验魔术方法 必须不是static, p1:类，p2:方法，p3:错误类型
		zend_check_magic_method_non_static(ce, fptr, error_type);
		// 检验：此魔术方法 必须是public
		zend_check_magic_method_public(ce, fptr, error_type);
		// 检验魔术方法 参数的类型。p1:参数序号，p2:类，p3:方法，p4:错误类型，p5:要求的参数类型
		zend_check_magic_method_arg_type(0, ce, fptr, error_type, MAY_BE_STRING);
		// 检验魔术方法 参数的类型。p1:参数序号，p2:类，p3:方法，p4:错误类型，p5:要求的参数类型
		zend_check_magic_method_arg_type(1, ce, fptr, error_type, MAY_BE_ARRAY);
	// __callstatic
	} else if (zend_string_equals_literal(lcname, ZEND_CALLSTATIC_FUNC_NAME)) {
		// 检验魔术方法的形参：（必须）数量一致，参数不支持引用传递。p1:要求参数数量, p2:类，p3:方法，p4:错误类型
		zend_check_magic_method_args(2, ce, fptr, error_type);
		// 检验：此魔术方法 必须是static
		zend_check_magic_method_static(ce, fptr, error_type);
		// 检验：此魔术方法 必须是public
		zend_check_magic_method_public(ce, fptr, error_type);
		// 检验魔术方法 参数的类型。p1:参数序号，p2:类，p3:方法，p4:错误类型，p5:要求的参数类型
		zend_check_magic_method_arg_type(0, ce, fptr, error_type, MAY_BE_STRING);
		// 检验魔术方法 参数的类型。p1:参数序号，p2:类，p3:方法，p4:错误类型，p5:要求的参数类型
		zend_check_magic_method_arg_type(1, ce, fptr, error_type, MAY_BE_ARRAY);
	// __tostring
	} else if (zend_string_equals_literal(lcname, ZEND_TOSTRING_FUNC_NAME)) {
		// 检验魔术方法的形参：（必须）数量一致，参数不支持引用传递。p1:要求参数数量, p2:类，p3:方法，p4:错误类型
		zend_check_magic_method_args(0, ce, fptr, error_type);
		// 检验魔术方法 必须不是static, p1:类，p2:方法，p3:错误类型
		zend_check_magic_method_non_static(ce, fptr, error_type);
		// 检验：此魔术方法 必须是public
		zend_check_magic_method_public(ce, fptr, error_type);
		// 检验魔术方法 返回值的类型
		zend_check_magic_method_return_type(ce, fptr, error_type, MAY_BE_STRING);
	// __debuginfo
	} else if (zend_string_equals_literal(lcname, ZEND_DEBUGINFO_FUNC_NAME)) {
		// 检验魔术方法的形参：（必须）数量一致，参数不支持引用传递。p1:要求参数数量, p2:类，p3:方法，p4:错误类型
		zend_check_magic_method_args(0, ce, fptr, error_type);
		// 检验魔术方法 必须不是static, p1:类，p2:方法，p3:错误类型
		zend_check_magic_method_non_static(ce, fptr, error_type);
		// 检验：此魔术方法 必须是public
		zend_check_magic_method_public(ce, fptr, error_type);
		// 检验魔术方法 返回值的类型
		zend_check_magic_method_return_type(ce, fptr, error_type, (MAY_BE_ARRAY | MAY_BE_NULL));
	// 序列化
	} else if (zend_string_equals_literal(lcname, "__serialize")) {
		// 检验魔术方法的形参：（必须）数量一致，参数不支持引用传递。p1:要求参数数量, p2:类，p3:方法，p4:错误类型
		zend_check_magic_method_args(0, ce, fptr, error_type);
		// 检验魔术方法 必须不是static, p1:类，p2:方法，p3:错误类型
		zend_check_magic_method_non_static(ce, fptr, error_type);
		// 检验：此魔术方法 必须是public
		zend_check_magic_method_public(ce, fptr, error_type);
		// 检验魔术方法 返回值的类型
		zend_check_magic_method_return_type(ce, fptr, error_type, MAY_BE_ARRAY);
	// 反序列化
	} else if (zend_string_equals_literal(lcname, "__unserialize")) {
		// 检验魔术方法的形参：（必须）数量一致，参数不支持引用传递。p1:要求参数数量, p2:类，p3:方法，p4:错误类型
		zend_check_magic_method_args(1, ce, fptr, error_type);
		// 检验魔术方法 必须不是static, p1:类，p2:方法，p3:错误类型
		zend_check_magic_method_non_static(ce, fptr, error_type);
		// 检验：此魔术方法 必须是public
		zend_check_magic_method_public(ce, fptr, error_type);
		// 检验魔术方法 参数的类型。p1:参数序号，p2:类，p3:方法，p4:错误类型，p5:要求的参数类型
		zend_check_magic_method_arg_type(0, ce, fptr, error_type, MAY_BE_ARRAY);
		// 检验魔术方法 返回值的类型
		zend_check_magic_method_return_type(ce, fptr, error_type, MAY_BE_VOID);
	// 获取状态
	} else if (zend_string_equals_literal(lcname, "__set_state")) {
		// 检验魔术方法的形参：（必须）数量一致，参数不支持引用传递。p1:要求参数数量, p2:类，p3:方法，p4:错误类型
		zend_check_magic_method_args(1, ce, fptr, error_type);
		// 检验：此魔术方法 必须是static
		zend_check_magic_method_static(ce, fptr, error_type);
		// 检验：此魔术方法 必须是public
		zend_check_magic_method_public(ce, fptr, error_type);
		// 检验魔术方法 参数的类型。p1:参数序号，p2:类，p3:方法，p4:错误类型，p5:要求的参数类型
		zend_check_magic_method_arg_type(0, ce, fptr, error_type, MAY_BE_ARRAY);
		// 检验魔术方法 返回值的类型
		zend_check_magic_method_return_type(ce, fptr, error_type, MAY_BE_OBJECT);
	// 引用
	} else if (zend_string_equals_literal(lcname, "__invoke")) {
		// 检验魔术方法 必须不是static, p1:类，p2:方法，p3:错误类型
		zend_check_magic_method_non_static(ce, fptr, error_type);
		// 检验：此魔术方法 必须是public
		zend_check_magic_method_public(ce, fptr, error_type);
	// 休眠
	} else if (zend_string_equals_literal(lcname, "__sleep")) {
		// 检验魔术方法的形参：（必须）数量一致，参数不支持引用传递。p1:要求参数数量, p2:类，p3:方法，p4:错误类型
		zend_check_magic_method_args(0, ce, fptr, error_type);
		// 检验魔术方法 必须不是static, p1:类，p2:方法，p3:错误类型
		zend_check_magic_method_non_static(ce, fptr, error_type);
		// 检验：此魔术方法 必须是public
		zend_check_magic_method_public(ce, fptr, error_type);
		// 检验魔术方法 返回值的类型
		zend_check_magic_method_return_type(ce, fptr, error_type, MAY_BE_ARRAY);
	// 唤醒
	} else if (zend_string_equals_literal(lcname, "__wakeup")) {
		// 检验魔术方法的形参：（必须）数量一致，参数不支持引用传递。p1:要求参数数量, p2:类，p3:方法，p4:错误类型
		zend_check_magic_method_args(0, ce, fptr, error_type);
		// 检验魔术方法 必须不是static, p1:类，p2:方法，p3:错误类型
		zend_check_magic_method_non_static(ce, fptr, error_type);
		// 检验：此魔术方法 必须是public
		zend_check_magic_method_public(ce, fptr, error_type);
		// 检验魔术方法 返回值的类型
		zend_check_magic_method_return_type(ce, fptr, error_type, MAY_BE_VOID);
	}
}
/* }}} */

// ing4, 添加魔术方法,13个，p1:类，p2:函数，p3:魔术方法名
ZEND_API void zend_add_magic_method(zend_class_entry *ce, zend_function *fptr, zend_string *lcname)
{
	// 如果开头不是 _ 直接跳过
	if (ZSTR_VAL(lcname)[0] != '_' || ZSTR_VAL(lcname)[1] != '_') {
		/* pass */
	// __clone
	} else if (zend_string_equals_literal(lcname, ZEND_CLONE_FUNC_NAME)) {
		ce->clone = fptr;
	// __construct
	} else if (zend_string_equals_literal(lcname, ZEND_CONSTRUCTOR_FUNC_NAME)) {
		ce->constructor = fptr;
		// 添加构造方法标记
		ce->constructor->common.fn_flags |= ZEND_ACC_CTOR;
	// __destruct
	} else if (zend_string_equals_literal(lcname, ZEND_DESTRUCTOR_FUNC_NAME)) {
		ce->destructor = fptr;
	// __get
	} else if (zend_string_equals_literal(lcname, ZEND_GET_FUNC_NAME)) {
		ce->__get = fptr;
		ce->ce_flags |= ZEND_ACC_USE_GUARDS;
	// __set
	} else if (zend_string_equals_literal(lcname, ZEND_SET_FUNC_NAME)) {
		ce->__set = fptr;
		ce->ce_flags |= ZEND_ACC_USE_GUARDS;
	// __call
	} else if (zend_string_equals_literal(lcname, ZEND_CALL_FUNC_NAME)) {
		ce->__call = fptr;
	// __unset
	} else if (zend_string_equals_literal(lcname, ZEND_UNSET_FUNC_NAME)) {
		ce->__unset = fptr;
		ce->ce_flags |= ZEND_ACC_USE_GUARDS;
	// __isset
	} else if (zend_string_equals_literal(lcname, ZEND_ISSET_FUNC_NAME)) {
		ce->__isset = fptr;
		ce->ce_flags |= ZEND_ACC_USE_GUARDS;
	// __callstatic
	} else if (zend_string_equals_literal(lcname, ZEND_CALLSTATIC_FUNC_NAME)) {
		ce->__callstatic = fptr;
	// __tostring
	} else if (zend_string_equals_literal(lcname, ZEND_TOSTRING_FUNC_NAME)) {
		ce->__tostring = fptr;
	// __debuginfo
	} else if (zend_string_equals_literal(lcname, ZEND_DEBUGINFO_FUNC_NAME)) {
		ce->__debugInfo = fptr;
	// __serialize
	} else if (zend_string_equals_literal(lcname, "__serialize")) {
		ce->__serialize = fptr;
	// __unserialize
	} else if (zend_string_equals_literal(lcname, "__unserialize")) {
		ce->__unserialize = fptr;
	}
}

// 创建 参数信息列表 ，增加第一个 返回类型不允null 的 参数（返回值）。
// 传入：变量名，是否引用返回，必要的参数数，类型，是否允null
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arg_info_toString, 0, 0, IS_STRING, 0)
ZEND_END_ARG_INFO()

// 370 行
// ing3, 注册所有  *library_functions 里的函数到函数哈希表里
/* registers all functions in *library_functions in the function hash */
ZEND_API zend_result zend_register_functions(zend_class_entry *scope, const zend_function_entry *functions, HashTable *function_table, int type) /* {{{ */
{
	// 函数列表指针
	const zend_function_entry *ptr = functions;
	// 临时变量
	zend_function function, *reg_function;
	// 内置函数（临时变量）
	zend_internal_function *internal_function = (zend_internal_function *)&function;
	//
	int count=0, unload=0;
	//
	HashTable *target_function_table = function_table;
	int error_type;
	zend_string *lowercase_name;
	size_t fname_len;
	// 如果是持久模块
	if (type==MODULE_PERSISTENT) {
		// 类型为 核心警告
		error_type = E_CORE_WARNING;
	} else {
		// 类型为 警告
		error_type = E_WARNING;
	}

	// 如果没有目标函数表
	if (!target_function_table) {
		// 使用 compile 函数表
		target_function_table = CG(function_table);
	}
	// （临时变量的）类型为 内置函数 
	internal_function->type = ZEND_INTERNAL_FUNCTION;
	// （临时变量的）模块入口
	internal_function->module = EG(current_module);
	// （临时变量的）临时变量数量
	internal_function->T = 0;
	// 资源指针列表（6个），这种指针列表一定要先置成0，要不然里面残留的任何内容都可能被误读
	memset(internal_function->reserved, 0, ZEND_MAX_RESERVED_RESOURCES * sizeof(void*));
	
	// 步骤1，处理所有函数
	// 遍历函数列表：只要有函数名
	while (ptr->fname) {
		// 函数名长度
		fname_len = strlen(ptr->fname);
		// 处理器 复制给临时变量
		internal_function->handler = ptr->handler;
		// 函数名 复制给临时变量
		internal_function->function_name = zend_string_init_interned(ptr->fname, fname_len, 1);
		// 类入口，作用域 复制给临时变量
		internal_function->scope = scope;
		// 原型？
		internal_function->prototype = NULL;
		// 修饰属性
		internal_function->attributes = NULL;
		// 步骤1.1，创建 run_time_cache
		// 如果正在执行
		// 这应该只有在使用 dl()注册 或 运行时的 某些临时情况时会发生 
		if (EG(active)) { // at run-time: this ought to only happen if registered with dl() or somehow temporarily at runtime
			// 初始化运行时缓存 
			// zend_internal_run_time_cache_reserved_size： 模块的处理器数量 * sizeof(void *)
			ZEND_MAP_PTR_INIT(internal_function->run_time_cache, zend_arena_calloc(&CG(arena), 1, zend_internal_run_time_cache_reserved_size()));
		// 如果没有在执行
		} else {
			// 创建 internal_function->run_time_cache__ptr
			ZEND_MAP_PTR_NEW(internal_function->run_time_cache);
		}
		
		// 步骤1.2，处理可见性标记
		// 如果有附加标记
		if (ptr->flags) {
			// 如果没有 public,protected,private
			if (!(ptr->flags & ZEND_ACC_PPP_MASK)) {
				// 如果没有 弃用标记，并且有指定域
				if (ptr->flags != ZEND_ACC_DEPRECATED && scope) {
					// 报错，无效的访问级别。访问级别必须是 public,protected 或private
					zend_error(error_type, "Invalid access level for %s%s%s() - access must be exactly one of public, protected or private", scope ? ZSTR_VAL(scope->name) : "", scope ? "::" : "", ptr->fname);
				}
				// 如果是弃用，添加 public
				internal_function->fn_flags = ZEND_ACC_PUBLIC | ptr->flags;
			// 如果有访问级别
			} else {
				// 使用已有的访问级别
				internal_function->fn_flags = ptr->flags;
			}
		// 没有附加标记
		} else {
			// 函数添加 public 标记
			internal_function->fn_flags = ZEND_ACC_PUBLIC;
		}

		// 步骤1.3，处理参数信息
		// 如果有参数信息
		if (ptr->arg_info) {
			// 返回类型？
			zend_internal_function_info *info = (zend_internal_function_info*)ptr->arg_info;
			// 第一个参数
			internal_function->arg_info = (zend_internal_arg_info*)ptr->arg_info+1;
			// 参数数量 
			internal_function->num_args = ptr->num_args;
			// 目前不表示 函数可以允许比 num_args 少的参数数量
			/* Currently you cannot denote that the function can accept less arguments than num_args */
			// 如果必要参数数量无效
			if (info->required_num_args == (zend_uintptr_t)-1) {
				// 必要参数数量等于参数数量 
				internal_function->required_num_args = ptr->num_args;
			// 如果必要参数数量有效
			} else {
				// 使用必要参数数量 
				internal_function->required_num_args = info->required_num_args;
			}
			// 如果有 send mode
			if (ZEND_ARG_SEND_MODE(info)) {
				// 添加 返回引用 标记
				internal_function->fn_flags |= ZEND_ACC_RETURN_REFERENCE;
			}
			// 如果是 IS_VARIADIC 模式
			if (ZEND_ARG_IS_VARIADIC(&ptr->arg_info[ptr->num_args])) {
				// 添加 ZEND_ACC_VARIADIC 标记
				internal_function->fn_flags |= ZEND_ACC_VARIADIC;
				// 不计算 字典参数
				/* Don't count the variadic argument */
				// 在参数数量中把字典参数减掉
				internal_function->num_args--;
			}
			// 如果类型有效
			if (ZEND_TYPE_IS_SET(info->type)) {
				// 如果类型有名字
				if (ZEND_TYPE_HAS_NAME(info->type)) {
					// 类型名
					const char *type_name = ZEND_TYPE_LITERAL_NAME(info->type);
					// 如果没有作用域 并且 类型是 self 或 parent 
					if (!scope && (!strcasecmp(type_name, "self") || !strcasecmp(type_name, "parent"))) {
						// 报错 不可以在类作用域外定义返回类型 type_name ?
						zend_error_noreturn(E_CORE_ERROR, "Cannot declare a return type of %s outside of a class scope", type_name);
					}
				}
				// 类型没有名字，添加 有返回类型，标记
				internal_function->fn_flags |= ZEND_ACC_HAS_RETURN_TYPE;
			}
		// 如果没有参数信息
		} else {
			// 核心警告，参数信息丢失
			zend_error(E_CORE_WARNING, "Missing arginfo for %s%s%s()",
				 scope ? ZSTR_VAL(scope->name) : "", scope ? "::" : "", ptr->fname);
			// 清空参数信息
			internal_function->arg_info = NULL;
			// 清空参数数量和需要的参数数量 0
			internal_function->num_args = 0;
			// 必须参数数量 0
			internal_function->required_num_args = 0;
		}
		
		// 步骤1.4，处理 __toString
		// 如果未指定，添加 __toString 的返回类型，来兼容 Stringable 接口
		/* If not specified, add __toString() return type for compatibility with Stringable
		 * interface. */
		// 如果方法名是 __tostring 并且未指定返回类型
		if (scope && zend_string_equals_literal_ci(internal_function->function_name, "__tostring") &&
				!(internal_function->fn_flags & ZEND_ACC_HAS_RETURN_TYPE)) {
			// 核心警告， __toString 必需添加返回类型为string
			zend_error(E_CORE_WARNING, "%s::__toString() implemented without string return type",
				ZSTR_VAL(scope->name));
			// 自动修复 __tostring 方法
			// 下一个 ？？
			internal_function->arg_info = (zend_internal_arg_info *) arg_info_toString + 1;
			// 函数有返回类型
			internal_function->fn_flags |= ZEND_ACC_HAS_RETURN_TYPE;
			// 参数数量和返回值数量都是0
			internal_function->num_args = internal_function->required_num_args = 0;
		}
		
		// 步骤1.5，设置函数的参数标识，是否通过引用传递（字典参数特殊处理）
		zend_set_function_arg_flags((zend_function*)internal_function);
		
		// 步骤1.6，给类添加抽象标记
		// 如果是抽象函数 
		if (ptr->flags & ZEND_ACC_ABSTRACT) {
			// 如果有作用域
			if (scope) {
				// 有了抽象方法，类也会被设置成抽象
				/* This is a class that must be abstract itself. Here we set the check info. */
				// 添加隐式抽象类标记（接口用的）
				scope->ce_flags |= ZEND_ACC_IMPLICIT_ABSTRACT_CLASS;
				// 如果作用域不是接口
				if (!(scope->ce_flags & ZEND_ACC_INTERFACE)) {
					// 如果类不是接口它需要被定义成抽象类
					// 处理可以添加标记的内置函数。（接口不可以）
					// 设置 abstract 关键字
					/* Since the class is not an interface it needs to be declared as a abstract class. */
					/* Since here we are handling internal functions only we can add the keyword flag. */
					/* This time we set the flag for the keyword 'abstract'. */
					// 添加隐式抽象类标记
					scope->ce_flags |= ZEND_ACC_EXPLICIT_ABSTRACT_CLASS;
				}
			}
			// 如果是静态函数  并且 没有作用域或 作用域是接口
			if ((ptr->flags & ZEND_ACC_STATIC) && (!scope || !(scope->ce_flags & ZEND_ACC_INTERFACE))) {
				// 报错，不可以注册抽象函数
				zend_error(error_type, "Static function %s%s%s() cannot be abstract", scope ? ZSTR_VAL(scope->name) : "", scope ? "::" : "", ptr->fname);
			}
		// 如果不是抽象函数 
		} else {
			// 如果是在接口中
			if (scope && (scope->ce_flags & ZEND_ACC_INTERFACE)) {
				// 报错：接口中不可以有非抽象方法
				zend_error(error_type, "Interface %s cannot contain non abstract method %s()", ZSTR_VAL(scope->name), ptr->fname);
				return FAILURE;
			}
			// 如果临时函数没有处理器
			if (!internal_function->handler) {
				// 报错：方法不可以是一个空函数
				zend_error(error_type, "Method %s%s%s() cannot be a NULL function", scope ? ZSTR_VAL(scope->name) : "", scope ? "::" : "", ptr->fname);
				// 删除（多个）函数，p1:删除这些函数, p2:删除数量（-1全部），p3:函数表
				zend_unregister_functions(functions, count, target_function_table);
				// 返回失败
				return FAILURE;
			}
		}
		
		// 步骤1.7，把临时函数添加到函数表中
		// 小写函数名
		lowercase_name = zend_string_tolower_ex(internal_function->function_name, type == MODULE_PERSISTENT);
		// 创建内置字串
		lowercase_name = zend_new_interned_string(lowercase_name);
		// 创建内置函数对象
		reg_function = malloc(sizeof(zend_internal_function));
		// 把 function（临时 函数实例） 复制给 reg_function
		memcpy(reg_function, &function, sizeof(zend_internal_function));
		// 在目标函数表中，添加这个内置函数 ，如果操作失败
		if (zend_hash_add_ptr(target_function_table, lowercase_name, reg_function) == NULL) {
			// 如果操作失败，需要卸载
			unload=1;
			// 删除内置函数对象
			free(reg_function);
			// 释放函数名
			zend_string_release(lowercase_name);
			// 跳出
			break;
		}

		// 步骤1.8，计算参数数量
		// 获取参数数量，包含字典参数
		/* Get parameter count including variadic parameter. */
		uint32_t num_args = reg_function->common.num_args;
		// 如果有字典参数，参数数量 +1
		if (reg_function->common.fn_flags & ZEND_ACC_VARIADIC) {
			num_args++;
		}
		
		// 步骤1.9，给有类型的参数添加 ZEND_ACC_HAS_TYPE_HINTS 标记
		// 如果需要检测参数类型
		/* If types of arguments have to be checked */
		// 如果有参数信息，参数个数>0
		if (reg_function->common.arg_info && num_args) {
			uint32_t i;
			// 遍历每一个参数
			for (i = 0; i < num_args; i++) {
				// 取得内置参数信息
				zend_internal_arg_info *arg_info = &reg_function->internal_function.arg_info[i];
				// 必需要有参数名
				ZEND_ASSERT(arg_info->name && "Parameter must have a name");
				// 如果有类型
				if (ZEND_TYPE_IS_SET(arg_info->type)) {
					// 添加【有类型信息】标记
				    reg_function->common.fn_flags |= ZEND_ACC_HAS_TYPE_HINTS;
				}
// 调试用
#if ZEND_DEBUG
				for (uint32_t j = 0; j < i; j++) {
					if (!strcmp(arg_info->name, reg_function->internal_function.arg_info[j].name)) {
						zend_error_noreturn(E_CORE_ERROR,
							"Duplicate parameter name $%s for function %s%s%s()", arg_info->name,
							scope ? ZSTR_VAL(scope->name) : "", scope ? "::" : "", ptr->fname);
					}
				}
#endif
			}
		}

		// 步骤1.10，如果参数有类型，返回值也有类型， 重建参数信息 
		/* Rebuild arginfos if parameter/property types and/or a return type are used */
		if (reg_function->common.arg_info &&
		    (reg_function->common.fn_flags & (ZEND_ACC_HAS_RETURN_TYPE|ZEND_ACC_HAS_TYPE_HINTS))) {
			// 把 const char* 类型转成 zend_string*
			/* convert "const char*" class type names into "zend_string*" */
			// 
			uint32_t i;
			// 返回值信息
			zend_arg_info *arg_info = reg_function->common.arg_info - 1;
			// 
			zend_arg_info *new_arg_info;

			// 把返回值处理成一个扩展参数
			/* Treat return type as an extra argument */
			// 参数数量 +1
			num_args++;
			// 创建参数信息列表
			new_arg_info = malloc(sizeof(zend_arg_info) * num_args);
			// 复制返回值信息
			memcpy(new_arg_info, arg_info, sizeof(zend_arg_info) * num_args);
			// 使用新的参数列表，把返回值信息放在 -1位置
			reg_function->common.arg_info = new_arg_info + 1;
			// 遍历 所有参数
			for (i = 0; i < num_args; i++) {
				// 检验是否是复杂类型
				if (ZEND_TYPE_IS_COMPLEX(new_arg_info[i].type)) {
					// 必需要有type名称
					ZEND_ASSERT(ZEND_TYPE_HAS_NAME(new_arg_info[i].type)
						&& "Should be stored as simple name");
					// 取得类型名称（可能多个）
					const char *class_name = ZEND_TYPE_LITERAL_NAME(new_arg_info[i].type);
					// 类型数量为1
					size_t num_types = 1;
					// 类型名
					const char *p = class_name;
					// 检查每个字符，如果是 | （多个类型的间隔符号）
					while ((p = strchr(p, '|'))) {
						// 类型数量+1
						num_types++;
						// 下一个字符 
						p++;
					}
					// 如果只有一种类型
					if (num_types == 1) {
						// 简单类 类型
						/* Simple class type */
						// 更新p1的指针 ((t).ptr = (_ptr))
						ZEND_TYPE_SET_PTR(new_arg_info[i].type,
							zend_string_init_interned(class_name, strlen(class_name), 1));
					// 如果有多种类型
					} else {
						// 联合类型
						/* Union type */
						// 创建类型列表
						zend_type_list *list = malloc(ZEND_TYPE_LIST_SIZE(num_types));
						// 记录类型数量 
						list->num_types = num_types;
						// 给列表类型设置类入口和 type_mask
						ZEND_TYPE_SET_LIST(new_arg_info[i].type, list);
						// 添加标记，联合类型
						ZEND_TYPE_FULL_MASK(new_arg_info[i].type) |= _ZEND_TYPE_UNION_BIT;
						// 循环变量：字串的开始位置
						const char *start = class_name;
						// 类型数量
						uint32_t j = 0;
						// 一个个截取类型字串
						while (true) {
							// 下一个 | 是结尾
							const char *end = strchr(start, '|');
							// 创建内置字串，开始位置到结束位置
							zend_string *str = zend_string_init_interned(
								start, end ? end - start : strlen(start), 1);
							// 创建 有类名类型，带类入口，自定义允null, extra_flags
							list->types[j] = (zend_type) ZEND_TYPE_INIT_CLASS(str, 0, 0);
							// 如果没有下一个类型
							if (!end) {
								// 跳出
								break;
							}
							// 有下一个，下一个开始位置
							start = end + 1;
							// 计数+1
							j++;
						}
					}
				}
				// 如果是可迭代类型
				if (ZEND_TYPE_IS_ITERABLE_FALLBACK(new_arg_info[i].type)) {
					// 以前，每次进入这里都会显示这个警告
					/* Warning generated an extension load warning which is emitted for every test
					zend_error(E_CORE_WARNING, "iterable type is now a compile time alias for array|Traversable,"
						" regenerate the argument info via the php-src gen_stub build script");
					*/
					
					// 创建类型 Traversable。
					// 创建 有类名类型，自定义 type_mask。每次都创建是为了把原有类型加进来
					zend_type legacy_iterable = ZEND_TYPE_INIT_CLASS_CONST_MASK(ZSTR_KNOWN(ZEND_STR_TRAVERSABLE),
						(new_arg_info[i].type.type_mask|MAY_BE_ARRAY));
					// 此参数的类型为 Traversable
					new_arg_info[i].type = legacy_iterable;
				}
			}
		}
		// 步骤1.11，处理魔术方法
		// 如果有作用域
		if (scope) {
			// 检验魔术方法的定义是否符合要求
			zend_check_magic_method_implementation(
				scope, reg_function, lowercase_name, E_CORE_ERROR);
			// 添加魔术方法
			zend_add_magic_method(scope, reg_function, lowercase_name);
		}
		// 下一个函数 
		ptr++;
		// 数量+1
		count++;
		// 释放，小写函数名
		zend_string_release(lowercase_name);
	}
	// while结束 
	
	// 步骤2，卸载
	// 如果需要卸载，卸载前，显示 所有模块中留存的坏函数 
	if (unload) { /* before unloading, display all remaining bad function in the module */
		// 如果有函数名
		while (ptr->fname) {
			// 名称长度
			fname_len = strlen(ptr->fname);
			// 创建小写函数名
			lowercase_name = zend_string_alloc(fname_len, 0);
			// 把函数名转成小写放进string
			zend_str_tolower_copy(ZSTR_VAL(lowercase_name), ptr->fname, fname_len);
			// 如果目标函数列表中有此函数 
			if (zend_hash_exists(target_function_table, lowercase_name)) {
				// 报错：注册失败，函数名存在
				zend_error(error_type, "Function registration failed - duplicate name - %s%s%s", scope ? ZSTR_VAL(scope->name) : "", scope ? "::" : "", ptr->fname);
			}
			// 释放小写函数名
			zend_string_efree(lowercase_name);
			// 下一个函数 
			ptr++;
		}
		// 删除（多个）函数，p1:删除这些函数, p2:删除数量（-1全部），p3:函数表
		zend_unregister_functions(functions, count, target_function_table);
		// 操作失败
		return FAILURE;
	}
	// 操作成功
	return SUCCESS;
}
/* }}} */

// 数量 -1 意思是抹除所有函数，否则，抹除最前 count 个函数
/* count=-1 means erase all functions, otherwise,
 * erase the first count functions
 */
// ing3, 删除（多个）函数，p1:删除这些函数, p2:删除数量（-1全部），p3:函数表
ZEND_API void zend_unregister_functions(const zend_function_entry *functions, int count, HashTable *function_table) /* {{{ */
{
	const zend_function_entry *ptr = functions;
	int i=0;
	// 目标函数表
	HashTable *target_function_table = function_table;
	zend_string *lowercase_name;
	size_t fname_len;

	// 如果没有目标函数表
	if (!target_function_table) {
		// 编译时函数表作为目标函数表
		target_function_table = CG(function_table);
	}
	// 遍历所有函数，碰到没有函数名的中断
	while (ptr->fname) {
		// 没有要求删除全部，序号大于删除数量
		if (count!=-1 && i>=count) {
			// 退出
			break;
		}
		// 函数名长度
		fname_len = strlen(ptr->fname);
		// 写名称字串
		lowercase_name = zend_string_alloc(fname_len, 0);
		// 复制小写函数名
		zend_str_tolower_copy(ZSTR_VAL(lowercase_name), ptr->fname, fname_len);
		// 从哈希表中删除函数
		zend_hash_del(target_function_table, lowercase_name);
		// 释放小写函数名
		zend_string_efree(lowercase_name);
		// 下一个
		ptr++;
		// 已经删除的数量 
		i++;
	}
}
/* }}} */

// ing4, 启动模块
ZEND_API zend_result zend_startup_module(zend_module_entry *module) /* {{{ */
{
	// 注册内置模块（类型是永久），如果 成功启动模块
	if ((module = zend_register_internal_module(module)) != NULL && zend_startup_module_ex(module) == SUCCESS) {
		// 启动成功，返回：成功
		return SUCCESS;
	}
	// 返回：失败
	return FAILURE;
}
/* }}} */

// clear, 检查模块是否已启动。全局无调用
ZEND_API zend_result zend_get_module_started(const char *module_name) /* {{{ */
{
	zend_module_entry *module;
	// 在已注册模块列表中查找
	module = zend_hash_str_find_ptr(&module_registry, module_name, strlen(module_name));
	// 按启动状态返回结果
	return (module && module->module_started) ? SUCCESS : FAILURE;
}
/* }}} */

// ing4, 清理模块的类
static int clean_module_class(zval *el, void *arg) /* {{{ */
{
	// 要清理的类
	zend_class_entry *ce = (zend_class_entry *)Z_PTR_P(el);
	// 模块编号 
	int module_number = *(int *)arg;
	// 如果是内置类 并且 编号匹配
	if (ce->type == ZEND_INTERNAL_CLASS && ce->info.internal.module->module_number == module_number) {
		// 从哈希表删除
		return ZEND_HASH_APPLY_REMOVE;
	// 没找到这个类
	} else {
		// 在哈希表保留
		return ZEND_HASH_APPLY_KEEP;
	}
}
/* }}} */

// ing4, 清理指定模块的类
static void clean_module_classes(int module_number) /* {{{ */
{
	// 执行时类表中，删除指定模块的类
	zend_hash_apply_with_argument(EG(class_table), clean_module_class, (void *) &module_number);
}
/* }}} */

// ing4, 清理函数表的辅助函数， p1:数组元素，p2:其他参数
static int clean_module_function(zval *el, void *arg) /* {{{ */
{
	// 元素 中的函数 指针
	zend_function *fe = (zend_function *) Z_PTR_P(el);
	// 附加参数，模块
	zend_module_entry *module = (zend_module_entry *) arg;
	// 如果是内置函数 并且 属于此模块
	if (fe->common.type == ZEND_INTERNAL_FUNCTION && fe->internal_function.module == module) {
		// 返回指令：从哈希表中删除此函数
		return ZEND_HASH_APPLY_REMOVE;
	} else {
		// 返回指令：在哈希表中保留此函数
		return ZEND_HASH_APPLY_KEEP;
	}
}
/* }}} */

// ing4, 清理此模块的函数
static void clean_module_functions(zend_module_entry *module) /* {{{ */
{
	// 在哈希表中根据回调清理元素, 对每个元素调用 clean_module_function
	zend_hash_apply_with_argument(CG(function_table), clean_module_function, module);
}
/* }}} */

// ing3, 销毁指定模块
void module_destructor(zend_module_entry *module) /* {{{ */
{
// 调试用
#if ZEND_RC_DEBUG
	bool orig_rc_debug = zend_rc_debug;
#endif

	// 如果是临时模块
	if (module->type == MODULE_TEMPORARY) {
// 调试用
#if ZEND_RC_DEBUG
		/* FIXME: Loading extensions during the request breaks some invariants.
		 * In particular, it will create persistent interned strings, which is
		 * not allowed at this stage. */
		zend_rc_debug = false;
#endif
		// 清理模块中的资源。/Zend/zend_list.c	
		zend_clean_module_rsrc_dtors(module->module_number);
		// 清理模块中的常量。/Zend/zend_constants.c	
		clean_module_constants(module->module_number);
		// 清理模块中的类
		clean_module_classes(module->module_number);
	}

	// 如果模块已启动并且有关闭方法
	if (module->module_started && module->module_shutdown_func) {
// 不会用到
#if 0
		zend_printf("%s: Module shutdown\n", module->name);
#endif
		// 调用关闭方法
		module->module_shutdown_func(module->type, module->module_number);
	}

	// 如果模块已启动，并且 没有关闭方法 并且 是临时模块
	if (module->module_started
	 && !module->module_shutdown_func
	 && module->type == MODULE_TEMPORARY) {
		// 删除ini中的实体 zend_ini.h
		zend_unregister_ini_entries_ex(module->module_number, module->type);
	}

	/* Deinitialize module globals */
	if (module->globals_size) {
// 线程安全
#ifdef ZTS
		// 如果模块有全局ID
		if (*module->globals_id_ptr) {
			// 释放模块
			ts_free_id(*module->globals_id_ptr);
		}
// 非线程安全
#else
		// 如果模块有全局销毁器
		if (module->globals_dtor) {
			// 销毁模块
			module->globals_dtor(module->globals_ptr);
		}
#endif
	}
	
	// 未启动
	module->module_started=0;
	// 如果是临时模块，并且有函数 
	if (module->type == MODULE_TEMPORARY && module->functions) {
		// 删除（多个）函数，p1:删除这些函数, p2:删除数量（-1全部），p3:函数表
		zend_unregister_functions(module->functions, -1, NULL);
		// 清理 module->functions 中 分开注册的函数
		/* Clean functions registered separately from module->functions */
		clean_module_functions(module);
	}

// windows有这个东东
#if HAVE_LIBDL
	// 如果环境变量没有 不让卸载模块
	if (module->handle && !getenv("ZEND_DONT_UNLOAD_MODULES")) {
		// 卸载模块
		DL_UNLOAD(module->handle);
	}
#endif

// 调试用
#if ZEND_RC_DEBUG
	zend_rc_debug = orig_rc_debug;
#endif
}
/* }}} */

// ing4, 激活所有模块
ZEND_API void zend_activate_modules(void) /* {{{ */
{
	// 模块指针列表
	zend_module_entry **p = module_request_startup_handlers;

	// 遍历
	while (*p) {
		// 取得模块
		zend_module_entry *module = *p;
		// 调用启动方法，如果失败
		if (module->request_startup_func(module->type, module->module_number)==FAILURE) {
			// 报错：A 模块请求启动失败
			zend_error(E_WARNING, "request_startup() for %s module failed", module->name);
			exit(1);
		}
		// 下一个模块
		p++;
	}
}
/* }}} */

// ing3, 暂停所有模块
ZEND_API void zend_deactivate_modules(void) /* {{{ */
{
	// 先清空当前执行数据。不再执行任何东东
	EG(current_execute_data) = NULL; /* we're no longer executing anything */

	// 如果要完全清空表
	if (EG(full_tables_cleanup)) {
		// 循环变量
		zend_module_entry *module;
		// 倒序遍历所有模块，依次调用结束方法
		ZEND_HASH_MAP_REVERSE_FOREACH_PTR(&module_registry, module) {
			// 如果有结束方法
			if (module->request_shutdown_func) {
				zend_try {
					// 调用结束方法
					module->request_shutdown_func(module->type, module->module_number);
				} zend_end_try();
			}
		} ZEND_HASH_FOREACH_END();
	// 不要完全清空表
	} else {
		// 关闭列表
		zend_module_entry **p = module_request_shutdown_handlers;
		// 遍历关闭列表
		while (*p) {
			zend_module_entry *module = *p;
			zend_try {
				// 调用结束方法
				module->request_shutdown_func(module->type, module->module_number);
			} zend_end_try();
			p++;
		}
	}
}
/* }}} */

// ing3, 暂停模块 的 后续处理
ZEND_API void zend_post_deactivate_modules(void) /* {{{ */
{
	// 如果要完全清空表
	if (EG(full_tables_cleanup)) {
		// 循环变量
		zend_module_entry *module;
		zval *zv;
		zend_string *key;
		// 遍历所有模块，依次调用 暂停后续处理方法
		ZEND_HASH_MAP_FOREACH_PTR(&module_registry, module) {
			// 如果有此方法
			if (module->post_deactivate_func) {
				// 调用它
				module->post_deactivate_func();
			}
		} ZEND_HASH_FOREACH_END();
		
		// 倒序遍历所有模块(带着key)，依次调用结束方法。
		ZEND_HASH_MAP_REVERSE_FOREACH_STR_KEY_VAL(&module_registry, key, zv) {
			// 取出 模块
			module = Z_PTR_P(zv);
			// 如果模块不是临时的
			if (module->type != MODULE_TEMPORARY) {
				// 跳过
				break;
			}
			// 销毁临时模块
			module_destructor(module);
			// 释放模块名
			zend_string_release_ex(key, 0);
		// 在遍历的过程中，依次删除每个元素
		} ZEND_HASH_MAP_FOREACH_END_DEL();
		
	// 如果不要完全清空表
	} else {
		// 需要后续处理的列表
		zend_module_entry **p = module_post_deactivate_handlers;
		// 遍历需要后续处理的列表
		while (*p) {
			zend_module_entry *module = *p;
			// 依次调用
			module->post_deactivate_func();
			p++;
		}
	}
}
/* }}} */

// clear, 返回下一个空模块编号
/* return the next free module number */
ZEND_API int zend_next_free_module(void) /* {{{ */
{
	return zend_hash_num_elements(&module_registry) + 1;
}
/* }}} */

// ing3, 注册内置类
static zend_class_entry *do_register_internal_class(zend_class_entry *orig_class_entry, uint32_t ce_flags) /* {{{ */
{
	// 创建一个标准zend类入口
	zend_class_entry *class_entry = malloc(sizeof(zend_class_entry));
	// 小写类名
	zend_string *lowercase_name;
	// 复制源类入口
	*class_entry = *orig_class_entry;
	// 新的类入口的类型是 内置类
	class_entry->type = ZEND_INTERNAL_CLASS;
	// 初始化类数据
	zend_initialize_class_data(class_entry, 0);
	// 分配类入口缓存。新建一个 map_ptr位置给type_name,把新map_ptr的偏移位置（序号）放进type_name的引用次数里。
	zend_alloc_ce_cache(class_entry->name);
	// 添加标记，在传入的标记外，还要添加 ZEND_ACC_CONSTANTS_UPDATED | ZEND_ACC_LINKED | ZEND_ACC_RESOLVED_PARENT | ZEND_ACC_RESOLVED_INTERFACES
	class_entry->ce_flags = orig_class_entry->ce_flags | ce_flags | ZEND_ACC_CONSTANTS_UPDATED | ZEND_ACC_LINKED | ZEND_ACC_RESOLVED_PARENT | ZEND_ACC_RESOLVED_INTERFACES;
	// 设置 _zend_module_entry
	class_entry->info.internal.module = EG(current_module);
	// 如果已经有了内置函数列表
	if (class_entry->info.internal.builtin_functions) {
		// 注册所有  *library_functions 里的函数到函数哈希表里
		zend_register_functions(class_entry, class_entry->info.internal.builtin_functions, &class_entry->function_table, EG(current_module)->type);
	}
	// 创建小写类名，是否持久看模块有没有要求持久
	lowercase_name = zend_string_tolower_ex(orig_class_entry->name, EG(current_module)->type == MODULE_PERSISTENT);
	// 类名创建内置字串
	lowercase_name = zend_new_interned_string(lowercase_name);
	// 把小写类名加到类列表里
	zend_hash_update_ptr(CG(class_table), lowercase_name, class_entry);
	// 释放小写类名
	zend_string_release_ex(lowercase_name, 1);
	// 如果有 __tostring 方法，并且类名不是 Stringable
	if (class_entry->__tostring && !zend_string_equals_literal(class_entry->name, "Stringable")
			// 并且 类实体不是 trait
			&& !(class_entry->ce_flags & ZEND_ACC_TRAIT)) {
		// Stringable 类必须已经存在
		// zend_ce_stringable = register_class_Stringable();
		ZEND_ASSERT(zend_ce_stringable
			&& "Should be registered before first class using __toString()");
		// 此类实现 zend_ce_stringable 接口
		zend_do_implement_interface(class_entry, zend_ce_stringable);
	}
	// 返回新创建的类入口
	return class_entry;
}
/* }}} */

// 如果有父类，从父类继承
// 如果没有父类但有父类名，查找父类并继承
// 如果都没有，进行普通的类注册
// 如果父类指定了，但找不到，返回NULL
/* If parent_ce is not NULL then it inherits from parent_ce
 * If parent_ce is NULL and parent_name isn't then it looks for the parent and inherits from it
 * If both parent_ce and parent_name are NULL it does a regular class registration
 * If parent_name is specified but not found NULL is returned
 */
// ing3, 注册内置类
ZEND_API zend_class_entry *zend_register_internal_class_ex(zend_class_entry *class_entry, zend_class_entry *parent_ce) /* {{{ */
{
	zend_class_entry *register_class;
	// 注册内置类
	register_class = zend_register_internal_class(class_entry);
	// 如果有父类
	if (parent_ce) {
		// 继承父类
		zend_do_inheritance(register_class, parent_ce);
		// 构造属性信息表
		zend_build_properties_info_table(register_class);
	}

	return register_class;
}
/* }}} */

// ing4, 类实现多个接口
ZEND_API void zend_class_implements(zend_class_entry *class_entry, int num_interfaces, ...) /* {{{ */
{
	zend_class_entry *interface_entry;
	// 获取多个参数（测试过）
	va_list interface_list;
	va_start(interface_list, num_interfaces);
	// 遍历每个接口
	while (num_interfaces--) {
		// 取出一个接口
		interface_entry = va_arg(interface_list, zend_class_entry *);
		// 如果是 zend_ce_stringable 接口，调用 zend_class_implements_interface 来实现它
		if (interface_entry == zend_ce_stringable
				&& zend_class_implements_interface(class_entry, zend_ce_stringable)) {
			
			// Stringable 接口自动实现，静默忽略显式的实现
			/* Stringable is implemented automatically,
			 * silently ignore an explicit implementation. */
			continue;
		}
		// 实现单个接口
		zend_do_implement_interface(class_entry, interface_entry);
	}

	va_end(interface_list);
}
/* }}} */

// 一个包含至少1个抽象方法的类会自动变成抽象类
/* A class that contains at least one abstract method automatically becomes an abstract class.
 */
// ing4, 注册内置类
ZEND_API zend_class_entry *zend_register_internal_class(zend_class_entry *orig_class_entry) /* {{{ */
{
	// 注册内置类
	return do_register_internal_class(orig_class_entry, 0);
}
/* }}} */

// ing4, 注册内置接口
ZEND_API zend_class_entry *zend_register_internal_interface(zend_class_entry *orig_class_entry) /* {{{ */
{
	// 注册内置类，附加接口标记
	return do_register_internal_class(orig_class_entry, ZEND_ACC_INTERFACE);
}
/* }}} */

// ing3, 注册类别名,p1:别名字串，p2:别名长度，p3:类，p4:是否永久
ZEND_API zend_result zend_register_class_alias_ex(const char *name, size_t name_len, zend_class_entry *ce, bool persistent) /* {{{ */
{
	zend_string *lcname;
	zval zv, *ret;

	/* TODO: Move this out of here in 7.4. */
	// 如果要求永久，但当前模块是临时的
	if (persistent && EG(current_module) && EG(current_module)->type == MODULE_TEMPORARY) {
		// 改为临时
		persistent = 0;
	}

	// 如果是 \ 命名空间开头
	if (name[0] == '\\') {
		// 创建字串
		lcname = zend_string_alloc(name_len-1, persistent);
		// 小写类名，不加开头 \ 
		zend_str_tolower_copy(ZSTR_VAL(lcname), name+1, name_len-1);
	// 不是命名空间开头
	} else {
		// 小写类名字串
		lcname = zend_string_alloc(name_len, persistent);
		// 小写类名
		zend_str_tolower_copy(ZSTR_VAL(lcname), name, name_len);
	}

	// 保证类名不属于保留字（测试过）
	zend_assert_valid_class_name(lcname);
	// 小写类名 创建内置字串
	lcname = zend_new_interned_string(lcname);

	// 给 zval.ptr 添加指针, 并标记成 别名指针 类型
	ZVAL_ALIAS_PTR(&zv, ce);
	// 小写类名，添加进类表里
	ret = zend_hash_add(CG(class_table), lcname, &zv);
	// 释放小写类名
	zend_string_release_ex(lcname, 0);
	// 添加成功
	if (ret) {
		// 如果不是不可更改
		if (!(ce->ce_flags & ZEND_ACC_IMMUTABLE)) {
			// 增加引用次数
			ce->refcount++;
		}
		// 避免 MINIT 时间的提示
		// avoid notifying at MINIT time
		if (ce->type == ZEND_USER_CLASS) {
			// 次调用每个 链接回调 函数
			zend_observer_class_linked_notify(ce, lcname);
		}
		return SUCCESS;
	}
	// 失败（别名已经存在）
	return FAILURE;
}
/* }}} */

// TODO num_symbol_tables as unsigned int?
// ing3, 在多个符号表中添加同一个元素
ZEND_API zend_result zend_set_hash_symbol(zval *symbol, const char *name, size_t name_length, bool is_ref, int num_symbol_tables, ...) /* {{{ */
{
	HashTable *symbol_table;
	va_list symbol_table_list;
	// 元素数<=0 返回:错误
	if (num_symbol_tables <= 0) return FAILURE;

	// 如果要求引用
	if (is_ref) {
		// 转成引用
		ZVAL_MAKE_REF(symbol);
	}
	// 参数列表
	va_start(symbol_table_list, num_symbol_tables);
	// 遍历每个参数
	while (num_symbol_tables-- > 0) {
		// 每个参数 HashTable *
		symbol_table = va_arg(symbol_table_list, HashTable *);
		// 更新元素
		zend_hash_str_update(symbol_table, name, name_length, symbol);
		// 给元素增加引用次数
		Z_TRY_ADDREF_P(symbol);
	}
	va_end(symbol_table_list);
	return SUCCESS;
}
/* }}} */

/* Disabled functions support */
// ing4, 从编译时 函数表 中删除一个函数
static void zend_disable_function(const char *function_name, size_t function_name_length)
{
	zend_hash_str_del(CG(function_table), function_name, function_name_length);
}

// ing4, 函数一串函数，传入函数名，多个用,或空格分隔
ZEND_API void zend_disable_functions(const char *function_list) /* {{{ */
{
	// 如果函数表无效
	if (!function_list || !*function_list) {
		// 中断
		return;
	}

	// 
	const char *s = NULL, *e = function_list;
	
	// 遍历每个字符（这法子厉害）
	while (*e) {
		switch (*e) {
			// 空或，
			case ' ':
			case ',':
				if (s) {
					// 删除前面这个函数（从上一个间隔符 到现在这个间隔符）
					zend_disable_function(s, e - s);
					// 函数名置空
					s = NULL;
				}
				break;
			// 默认（开头，或在间隔符后面）
			default:
				// 如果函数名为空
				if (!s) {
					// 从现在这个字符算开头
					s = e;
				}
				break;
		}
		// 下一个字符
		e++;
	}
	// 如果还有函数名
	if (s) {
		// 删除最后一个函数
		zend_disable_function(s, e - s);
	}
	
	// 删除完要 rehash 哈希表。保证所有内置函数都是连续的。
	// 这样在关闭时就不用清理整个表。
	/* Rehash the function table after deleting functions. This ensures that all internal
	 * functions are contiguous, which means we don't need to perform full table cleanup
	 * on shutdown. */
	// 整理哈希表
	zend_hash_rehash(CG(function_table));
}
/* }}} */

#ifdef ZEND_WIN32
#pragma optimize("", off)
#endif
// ing3, 显示禁用类错误信息，并返回此类的实例
static ZEND_COLD zend_object *display_disabled_class(zend_class_entry *class_type) /* {{{ */
{
	zend_object *intern;
	// 类入口创建新对象
	intern = zend_objects_new(class_type);

	// 初始化 属性
	/* Initialize default properties */
	// 如果默认属性数不是0
	if (EXPECTED(class_type->default_properties_count != 0)) {
		// 属性表
		zval *p = intern->properties_table;
		// 属性表结尾
		zval *end = p + class_type->default_properties_count;
		do {
			// 每个元素设置成未使用
			ZVAL_UNDEF(p);
			// 下一个元素
			p++;
		// 
		} while (p != end);
	}
	// 报错：此类型由于安全问题已经被禁用了
	zend_error(E_WARNING, "%s() has been disabled for security reasons", ZSTR_VAL(class_type->name));
	// 返回此实例
	return intern;
}
#ifdef ZEND_WIN32
#pragma optimize("", on)
#endif
/* }}} */

// 禁用类
static const zend_function_entry disabled_class_new[] = {
	ZEND_FE_END
};

// ing3, 禁用类
ZEND_API zend_result zend_disable_class(const char *class_name, size_t class_name_length) /* {{{ */
{
	zend_class_entry *disabled_class;
	zend_string *key;
	zend_function *fn;
	zend_property_info *prop;
	// 分配内存创建类名
	key = zend_string_alloc(class_name_length, 0);
	// 复制小写类名
	zend_str_tolower_copy(ZSTR_VAL(key), class_name, class_name_length);
	// 在 编译时 类表中查找此类
	disabled_class = zend_hash_find_ptr(CG(class_table), key);
	// 释放小写类名
	zend_string_release_ex(key, 0);
	// 没找到
	if (!disabled_class) {
		// 返回失败
		return FAILURE;
	}

	// 会在 INIT_CLASS_ENTRY 重置
	/* Will be reset by INIT_CLASS_ENTRY. */
	// 释放此类的所有接口
	free(disabled_class->interfaces);

	// 初始化类方法（构造方法，魔术方法等）
	INIT_CLASS_ENTRY_INIT_METHODS((*disabled_class), disabled_class_new);
	// 更改创建方法
	// 显示禁用类错误信息，并返回此类的实例
	disabled_class->create_object = display_disabled_class;

	// 遍历所有方法
	ZEND_HASH_MAP_FOREACH_PTR(&disabled_class->function_table, fn) {
		// 如果有返回类型 或 有参数类型 并且此方法已被禁用
		if ((fn->common.fn_flags & (ZEND_ACC_HAS_RETURN_TYPE|ZEND_ACC_HAS_TYPE_HINTS)) &&
			fn->common.scope == disabled_class) {
			// 释放内部参数信息（列表）
			zend_free_internal_arg_info(&fn->internal_function);
		}
	} ZEND_HASH_FOREACH_END();
	// 销毁每个元素并 重置哈希表
	zend_hash_clean(&disabled_class->function_table);
	// 遍历属性信息列表
	ZEND_HASH_MAP_FOREACH_PTR(&disabled_class->properties_info, prop) {
		// 如果所属类是此类
		if (prop->ce == disabled_class) {
			// 释放属性名
			zend_string_release(prop->name);
			// 释放此类型 
			zend_type_release(prop->type, /* persistent */ 1);
			// 释放此属性信息
			free(prop);
		}
	} ZEND_HASH_FOREACH_END();
	// 销毁每个元素并 重置哈希表
	zend_hash_clean(&disabled_class->properties_info);
	return SUCCESS;
}
/* }}} */

// ing4, 取得执行上下文中的域
static zend_always_inline zend_class_entry *get_scope(zend_execute_data *frame)
{
	// 如果有函数，有函数所属类，返回这个类
	return frame && frame->func ? frame->func->common.scope : NULL;
}

// ing3, 检验调用对象的所属类，并完善调用信息
// p1:类名，p2:域，p3:执行数据，p4:调用信息，p5:返回，是否严格类，p6:返回，错误信息，p7:是否弃用
static bool zend_is_callable_check_class(zend_string *name, zend_class_entry *scope, zend_execute_data *frame, zend_fcall_info_cache *fcc, bool *strict_class, char **error, bool suppress_deprecation) /* {{{ */
{
	bool ret = 0;
	zend_class_entry *ce;
	// 名称长度
	size_t name_len = ZSTR_LEN(name);
	zend_string *lcname;
	ALLOCA_FLAG(use_heap);
	// 新字串，小写名
	ZSTR_ALLOCA_ALLOC(lcname, name_len, use_heap);
	// 给名称复制小写副本
	zend_str_tolower_copy(ZSTR_VAL(lcname), ZSTR_VAL(name), name_len);
	// 默认不是严格类
	*strict_class = 0;
	// 如果名称是 self
	if (zend_string_equals_literal(lcname, "self")) {
		// 如果没有所属域
		if (!scope) {
			// 如果接收异常信息，返回错误信息：不可以在类外面使用 self
			if (error) *error = estrdup("cannot access \"self\" when no class scope is active");
		// 成功的情况1：使用 self ,并且有传入scope
		} else {
			// 如果没有抑制弃用信息
			if (!suppress_deprecation) {
				// 如果接收异常信息，返回错误信息：self已弃用
				zend_error(E_DEPRECATED, "Use of \"self\" in callables is deprecated");
			}
			// 获取调用域
			fcc->called_scope = zend_get_called_scope(frame);
			// 如果调用过的域无效 或 调用过的域 不是此类的子类
			if (!fcc->called_scope || !instanceof_function(fcc->called_scope, scope)) {
				// 更新调用过的域 成 此类
				fcc->called_scope = scope;
			}
			// 正在调用的域： 此类
			fcc->calling_scope = scope;
			// 如果没有调用对象
			if (!fcc->object) {
				// 从执行数据 中获取 This 对象
				fcc->object = zend_get_this_object(frame);
			}
			// 结果为1
			ret = 1;
		}
	// 如果名称是 parent
	} else if (zend_string_equals_literal(lcname, "parent")) {
		if (!scope) {
			// 如果接收异常信息，返回错误信息：不可以在类外面使用parent类
			if (error) *error = estrdup("cannot access \"parent\" when no class scope is active");
		} else if (!scope->parent) {
			// 如果接收异常信息，返回错误信息：此类没有父类
			if (error) *error = estrdup("cannot access \"parent\" when current class scope has no parent");
		// 成功的情况2：使用 parent 并且有传入域，并且域有parent
		} else {
			// 如果没有抑制弃用信息
			if (!suppress_deprecation) {
				// 错误信息：parent已弃用
				zend_error(E_DEPRECATED, "Use of \"parent\" in callables is deprecated");
			}
			// 更新，调用过的域
			fcc->called_scope = zend_get_called_scope(frame);
			// 如果调用过的域无效 或 调用过的域 和 此类不是同一个父类
			if (!fcc->called_scope || !instanceof_function(fcc->called_scope, scope->parent)) {
				// 更新调用过的域 ：类的父类
				fcc->called_scope = scope->parent;
			}
			// 正在调用的域，此类的父类
			fcc->calling_scope = scope->parent;
			// 如果没有调用对象
			if (!fcc->object) {
				// 从执行数据 中获取 This 对象
				fcc->object = zend_get_this_object(frame);
			}
			// 严格类
			*strict_class = 1;
			// 结果为1
			ret = 1;
		}
	// 如果名称是 static
	} else if (zend_string_equals_literal(lcname, "static")) {
		// 获取执行数据中的域
		zend_class_entry *called_scope = zend_get_called_scope(frame);
		// 如果域不存在
		if (!called_scope) {
			// 如果接收异常信息，返回错误信息：不可以在类外面使用 static 类
			if (error) *error = estrdup("cannot access \"static\" when no class scope is active");
		// 成功的情况3：用 static 并且没有 正在调用的域
		} else {
			// 如果没有抑制弃用信息
			if (!suppress_deprecation) {
				// 错误信息：parent已弃用 static已弃用
				zend_error(E_DEPRECATED, "Use of \"static\" in callables is deprecated");
			}
			// 正在调用的域 和调用过的域，都更新成 执行数据中的域
			fcc->called_scope = called_scope;
			fcc->calling_scope = called_scope;
			// 如果没有调用的对象 
			if (!fcc->object) {
				// 从执行数据 中获取 This 对象
				fcc->object = zend_get_this_object(frame);
			}
			// 严格类
			*strict_class = 1;
			// 结果为1
			ret = 1;
		}
	// 成功的情况4：如果用名称可以找到这个类
	} else if ((ce = zend_lookup_class(name)) != NULL) {
		// 取得执行上下文中的域
		zend_class_entry *scope = get_scope(frame);
		// 正在调用的域：找到的类
		fcc->calling_scope = ce;
		// 如果有域，并且 没有调用信息缓存中的对象
		if (scope && !fcc->object) {
			// 取得执行数据中的 This 对象
			zend_object *object = zend_get_this_object(frame);
			// 如果对象存在 并且 属于此域 并且 此域是 找到的类 的子域
			if (object &&
			    instanceof_function(object->ce, scope) &&
			    instanceof_function(scope, ce)) {
				// 使用 This 作为调用对象
				fcc->object = object;
				// This 的类 作为 调用过的域 
				fcc->called_scope = object->ce;
			// 没有This 或继承关联不对
			} else {
				// 找到的类 作为 调用过的域 
				fcc->called_scope = ce;
			}
		// 域不存在或者 已经有调用对象
		} else {
			// 如果调用信息缓存里有调用对象，用缓存对象的类；否则用找到的类
			fcc->called_scope = fcc->object ? fcc->object->ce : ce;
		}
		// 严格类
		*strict_class = 1;
		// 结果为1
		ret = 1;
	// 其他情况
	} else {
		// 如果接收异常信息，返回错误信息： 此类无法找到
		if (error) zend_spprintf(error, 0, "class \"%.*s\" not found", (int)name_len, ZSTR_VAL(name));
	}
	// 释放小写类名
	ZSTR_ALLOCA_FREE(lcname, use_heap);
	// 返回结果 
	return ret;
}
/* }}} */

// ing3, 释放缓存的 函数调用信息
// 只有 通过 __call 转发进来的方法调用才会走这里 ？
ZEND_API void zend_release_fcall_info_cache(zend_fcall_info_cache *fcc) {
	// 要有指定函数 
	if (fcc->function_handler &&
		// 指定函数必须有 ZEND_ACC_CALL_VIA_TRAMPOLINE 标记(/Zend/zend_object_handlers.c添加）
		// 这个是指，通过 __call() 方法转发进来的调用
		(fcc->function_handler->common.fn_flags & ZEND_ACC_CALL_VIA_TRAMPOLINE)) {
		// 如果有函数名
		if (fcc->function_handler->common.function_name) {
			// 把函数名释放掉
			zend_string_release_ex(fcc->function_handler->common.function_name, 0);
		}
		// ing4, 如果是执行时中的弹跳函数？清空函数名：否则释放此函数
		zend_free_trampoline(fcc->function_handler);
		// 清空 处理函数
		fcc->function_handler = NULL;
	}
}


// 260行
// ing3, 查找调用的函数实体，并检查此调用在 当前指定执行数据中 是否可行
// p1:只用来获取方法名字串，p2:上下文，p3:fcc,从头到尾一直在用 ，p4:是否严格类，p5:返回错误信息, p6:是否抑制弃用信息
static zend_always_inline bool zend_is_callable_check_func(zval *callable, zend_execute_data *frame, zend_fcall_info_cache *fcc, bool strict_class, char **error, bool suppress_deprecation) /* {{{ */
{
	// 原调用域
	zend_class_entry *ce_org = fcc->calling_scope;
	bool retval = 0;
	zend_string *mname, *cname;
	zend_string *lmname;
	const char *colon;
	size_t clen;
	HashTable *ftable;
	int call_via_handler = 0;
	zend_class_entry *scope;
	zval *zv;
	ALLOCA_FLAG(use_heap)
	// 先清空调用域
	fcc->calling_scope = NULL;

	// 步骤1：处理普通函数
	// 如果没有原调用域
	if (!ce_org) {
		zend_function *func;
		zend_string *lmname;
		// 检查此名称的函数是否存在。它可以是个包含命名空间名称的混合名称。
		/* Check if function with given name exists.
		 * This may be a compound name that includes namespace name */
		// 如果开头是 \ （带命名空间） 
		if (UNEXPECTED(Z_STRVAL_P(callable)[0] == '\\')) {
			/* Skip leading \ */
			// 创建指定长度（自动对齐）的字串
			ZSTR_ALLOCA_ALLOC(lmname, Z_STRLEN_P(callable) - 1, use_heap);
			// 复制小写名称，不带最前面 \ 
			zend_str_tolower_copy(ZSTR_VAL(lmname), Z_STRVAL_P(callable) + 1, Z_STRLEN_P(callable) - 1);
			// 用函数名 zend_string 获取函数
			func = zend_fetch_function(lmname);
			// 释放临时函数名
			ZSTR_ALLOCA_FREE(lmname, use_heap);
		// 开头不是 \ 
		} else {
			// 直接使用传入的函数名
			lmname = Z_STR_P(callable);
			// 查找函数
			func = zend_fetch_function(lmname);
			// 如果没找到
			if (!func) {
				// 创建指定长度（自动对齐）的字串
				ZSTR_ALLOCA_ALLOC(lmname, Z_STRLEN_P(callable), use_heap);
				// 复制小写名称，
				zend_str_tolower_copy(ZSTR_VAL(lmname), Z_STRVAL_P(callable), Z_STRLEN_P(callable));
				// 查找函数
				func = zend_fetch_function(lmname);
				// 释放临时函数名
				ZSTR_ALLOCA_FREE(lmname, use_heap);
			}
		}
		// 如果找到函数
		if (EXPECTED(func != NULL)) {
			// 链接到调用信息
			fcc->function_handler = func;
			// 结束
			return 1;
		}
	}

	// 步骤2：查找 调用域 和 方法名
	
	// 情况2.1， 用类名调用静态方法
	
	// 把名称拆分成 类/命名空间 和 方法名/函数名 
	/* Split name into class/namespace and method/function names */
	// 查字符 c 在 s 中最后出现的位置， 并且不在开头，并且前面一个字符也是:。（这样应该会匹配到开头::）
	if ((colon = zend_memrchr(Z_STRVAL_P(callable), ':', Z_STRLEN_P(callable))) != NULL &&
		colon > Z_STRVAL_P(callable) &&
		*(colon-1) == ':'
	) {
		size_t mlen;
		// 倒回到第一个 ：
		colon--;
		// 前面的是类名的长度
		clen = colon - Z_STRVAL_P(callable);
		// 后面的是方法名的长度
		mlen = Z_STRLEN_P(callable) - clen - 2;
		// 如果 :: 在开头
		if (colon == Z_STRVAL_P(callable)) {
			// 错误信息: 函数名无效
			if (error) *error = estrdup("invalid function name");
			return 0;
		}

		// 这是一个混合的类名的函数名
		// 尝试拆分成类和静态方法名
		/* This is a compound name.
		 * Try to fetch class and then find static method. */
		// 如果有 原调用域
		if (ce_org) {
			// 使用 原调用域
			scope = ce_org;
		// 如果没有正在调用的类入口
		} else {
			// 取得执行上下文中的域
			scope = get_scope(frame);
		}

		// 创建永久保留字，p1:字串，p2:长度，p3:是否永久
		cname = zend_string_init_interned(Z_STRVAL_P(callable), clen, 0);
		// 如果有类缓存 并且缓存有效
		if (ZSTR_HAS_CE_CACHE(cname) && ZSTR_GET_CE_CACHE(cname)) {
			// 调用中的域 更新
			fcc->calling_scope = ZSTR_GET_CE_CACHE(cname);
			// 如果有原来在调用的 域 并且没有对象
			if (scope && !fcc->object) {
				// 从执行数据中查找对象
				zend_object *object = zend_get_this_object(frame);
				// 如果有对象 并且 
				if (object &&
					// 对象是 scope的子类
				    instanceof_function(object->ce, scope) &&
					// scope 是当前调用域的子类
				    instanceof_function(scope, fcc->calling_scope)) {
					// 可用此object
					fcc->object = object;
					// object所属类记录 在调用过的域里
					fcc->called_scope = object->ce;
				// 否则
				} else {
					// 当前调用域记录在 调用过的域里
					fcc->called_scope = fcc->calling_scope;
				}
			// 否则
			} else {
				// 调用过的域 = 调用对象有效 ？ 调用对象的所属类 ： 当前调用域
				fcc->called_scope = fcc->object ? fcc->object->ce : fcc->calling_scope;
			}
			// 严格类
			strict_class = 1;
		// 检验调用对象的所属类，并完善调用信息
		// p1:类名，p2:域，p3:执行数据，p4:调用信息，p5:返回，是否严格类，p6:返回，错误信息，p7:是否弃用
		} else if (!zend_is_callable_check_class(cname, scope, frame, fcc, &strict_class, error, suppress_deprecation || ce_org != NULL)) {
			// 检验失败，释放类名
			zend_string_release_ex(cname, 0);
			// 返回：否
			return 0;
		}
		// 释放类名
		zend_string_release_ex(cname, 0);

		// 获取调用信息中的 调用域 的方法列表
		ftable = &fcc->calling_scope->function_table;
		// 如果有 原调用域 并且 它不是当前调用域的子类
		if (ce_org && !instanceof_function(ce_org, fcc->calling_scope)) {
			// 如果接收错误信息，返回错误信息：类A不是类B的子类
			if (error) zend_spprintf(error, 0, "class %s is not a subclass of %s", ZSTR_VAL(ce_org->name), ZSTR_VAL(fcc->calling_scope->name));
			// 返回：否
			return 0;
		}
		// 如果有 原调用域 并且 不抑制弃用信息
		if (ce_org && !suppress_deprecation) {
			// 弃用信息：[A,B] 形式的调用已经弃用
			zend_error(E_DEPRECATED,
				"Callables of the form [\"%s\", \"%s\"] are deprecated",
				ZSTR_VAL(ce_org->name), Z_STRVAL_P(callable));
		}
		// 方法名
		mname = zend_string_init(Z_STRVAL_P(callable) + clen + 2, mlen, 0);
		
	// 情况2.2， 如果有原调用域
	} else if (ce_org) {
		// 
		/* Try to fetch find static method of given class. */
		// 获取方法名
		mname = Z_STR_P(callable);
		// 方法名增加引用次数
		zend_string_addref(mname);
		// 原调用域的方法表
		ftable = &ce_org->function_table;
		// 调用域换成原调用域
		fcc->calling_scope = ce_org;
		
	// 情况2.3， 不是静态调用也没有原调用域
	} else {
		/* We already checked for plain function before. */
		// 如果接收错误
		if (error) {
			// 返回错误信息：函数没有找到有效的函数名
			zend_spprintf(error, 0, "function \"%s\" not found or invalid function name", Z_STRVAL_P(callable));
		}
		// 返回失败
		return 0;
	}

	// 步骤3：查找目标函数实例
	// 小写函数名
	lmname = zend_string_tolower(mname);
	// 情况3.1，如果是严格类 并且 有正在调用的域 并且 调用 __construct方法
	if (strict_class &&
	    fcc->calling_scope &&
		zend_string_equals_literal(lmname, ZEND_CONSTRUCTOR_FUNC_NAME)) {
		// 对应的c函数是 调用域的 构造 方法 
		fcc->function_handler = fcc->calling_scope->constructor;
		// 如果构造方法有效
		if (fcc->function_handler) {
			// 结果为：是
			retval = 1;
		}
	// 情况3.2，如果能在方法列表中找到要调用的方法
	} else if ((zv = zend_hash_find(ftable, lmname)) != NULL) {
		// 绑定此方法
		fcc->function_handler = Z_PTR_P(zv);
		// 结果为：是
		retval = 1;
		// 如果此方法是继承来的 并且 
		if ((fcc->function_handler->op_array.fn_flags & ZEND_ACC_CHANGED) &&
		    !strict_class) {
			// 取得执行上下文中的域
			scope = get_scope(frame);
			// 如果域有效 并且
			if (scope &&
				// 调用信息缓存中的函数所在域 是 此域的子类
			    instanceof_function(fcc->function_handler->common.scope, scope)) {
				// 在此域中查找此方法
				zv = zend_hash_find(&scope->function_table, lmname);
				// 如果找到此方法
				if (zv != NULL) {
					// 取出方法
					zend_function *priv_fbc = Z_PTR_P(zv);
					// 如果方法是私有的 并且 所属域是此域
					if ((priv_fbc->common.fn_flags & ZEND_ACC_PRIVATE)
					 && priv_fbc->common.scope == scope) {
						// 使用此方法
						fcc->function_handler = priv_fbc;
					}
				}
			}
		}
		// 如果方法不是是 public 并且 （有调用域 并且 （调用__call 或 不是调用 __callstatic））
		if (!(fcc->function_handler->common.fn_flags & ZEND_ACC_PUBLIC) &&
		    (fcc->calling_scope &&
		     ((fcc->object && fcc->calling_scope->__call) ||
		      (!fcc->object && fcc->calling_scope->__callstatic)))) {
			// 取得执行数据中的 域
			scope = get_scope(frame);
			// 如果执行数据中的 域 不是此方法的域
			if (fcc->function_handler->common.scope != scope) {
				// 如果此方法是 private 
				if ((fcc->function_handler->common.fn_flags & ZEND_ACC_PRIVATE)
				// 保证调用可用的 受保护方法（实际上还是检验继承关系）
				// 取得方法的根类：如果有原型（->common.prototype）返回原型的 作用域。否则使用本身的作用域
				 || !zend_check_protected(zend_get_function_root_class(fcc->function_handler), scope)) {
					// 如果失败，结果为：否
					retval = 0;
					// 清空处理函数
					fcc->function_handler = NULL;
					// 重新获取方法
					goto get_function_via_handler;
				}
			}
		}
	// 情况3.3，找不到方法，尝试通过弹跳调用 
	} else {
// 通过处理器获取函数
get_function_via_handler:
		// 如果调用信息缓存有 对象元素 并且 它的调用域  与原调用域相同
		if (fcc->object && fcc->calling_scope == ce_org) {
			// 如果是严格类 并且 原调用域有 __call 方法
			if (strict_class && ce_org->__call) {
				// 使用弹跳调用原域中的方法
				fcc->function_handler = zend_get_call_trampoline_func(ce_org, mname, 0);
				// 标记：是通过弹跳调用
				call_via_handler = 1;
				// 返回值为 1
				retval = 1;
			// 其他情况
			} else {
				// 在方法所属对象 中 用方法名查找方法
				fcc->function_handler = fcc->object->handlers->get_method(&fcc->object, mname, NULL);
				// 如果找到方法
				if (fcc->function_handler) {
					// 如果是严格类 并且 （调用的方法没有所属域 或 原调用域是 方法所属域的子类）
					if (strict_class &&
					    (!fcc->function_handler->common.scope ||
					     !instanceof_function(ce_org, fcc->function_handler->common.scope))) {
						// 释放缓存的 函数调用信息
						zend_release_fcall_info_cache(fcc);
					// 否则 
					} else {
						// 结果为：是
						retval = 1;
						// 标记：是否通过弹跳调用
						call_via_handler = (fcc->function_handler->common.fn_flags & ZEND_ACC_CALL_VIA_TRAMPOLINE) != 0;
					}
				}
			}
		// 
		} else if (fcc->calling_scope) {
			// 如果正在调用的域中 可获取静态方法
			if (fcc->calling_scope->get_static_method) {
				// 获取静态方法
				fcc->function_handler = fcc->calling_scope->get_static_method(fcc->calling_scope, mname);
			// 无法获取静态方法
			} else {
				// 使用标记方法获取静态方法
				fcc->function_handler = zend_std_get_static_method(fcc->calling_scope, mname, NULL);
			}
			// 如果找到了需要的方法
			if (fcc->function_handler) {
				// 结果为：是
				retval = 1;
				// 标记：是否通过弹跳调用
				call_via_handler = (fcc->function_handler->common.fn_flags & ZEND_ACC_CALL_VIA_TRAMPOLINE) != 0;
				// 如果是通过弹跳调用 并且 调用缓存没有关联对象
				if (call_via_handler && !fcc->object) {
					// 从执行数据中 获取 This
					zend_object *object = zend_get_this_object(frame);
					// 如果有this 并且 它是 函数调用域的子类
					if (object &&
					    instanceof_function(object->ce, fcc->calling_scope)) {
						// this作为调用时的对象
						fcc->object = object;
					}
				}
			}
		}
	}
	
	// 步骤4：进行最后的检查
	// 情况4.1， 如果结果为： 是，找到了可用的方法
	if (retval) {
		// 如果有调用域 并且
		if (fcc->calling_scope && !call_via_handler) {
			// 如果方法 是 抽象的
			if (fcc->function_handler->common.fn_flags & ZEND_ACC_ABSTRACT) {
				// 结果为：否
				retval = 0;
				// 如果接收异常信息
				if (error) {
					// 返回错误信息：不可以调用抽象方法
					zend_spprintf(error, 0, "cannot call abstract method %s::%s()", ZSTR_VAL(fcc->calling_scope->name), ZSTR_VAL(fcc->function_handler->common.function_name));
				}
			// 如果没有调用对象 并且 不是调用静态方法
			} else if (!fcc->object && !(fcc->function_handler->common.fn_flags & ZEND_ACC_STATIC)) {
				// 结果为：否
				retval = 0;
				// 如果接收异常信息
				if (error) {
					// 不可以用非静态方式调用静态方法
					zend_spprintf(error, 0, "non-static method %s::%s() cannot be called statically", ZSTR_VAL(fcc->calling_scope->name), ZSTR_VAL(fcc->function_handler->common.function_name));
				}
			}
			// 如果暂时通过检测 并且 方法不是public
			if (retval
			 && !(fcc->function_handler->common.fn_flags & ZEND_ACC_PUBLIC)) {
				// 取得执行上下文中的域
				scope = get_scope(frame);
				// 如果方法不属于当前域
				if (fcc->function_handler->common.scope != scope) {
					// 如果方法是 private 或 
					if ((fcc->function_handler->common.fn_flags & ZEND_ACC_PRIVATE)
					// 保证调用可用的 受保护方法（实际上还是检验继承关系）
					 || (!zend_check_protected(zend_get_function_root_class(fcc->function_handler), scope))) {
						// 如果接收异常信息
						if (error) {
							// 如果有异常信息
							if (*error) {
								// 释放它
								efree(*error);
							}
							// 重建异常信息：无法访问 （ppp） 方法
							zend_spprintf(error, 0, "cannot access %s method %s::%s()", zend_visibility_string(fcc->function_handler->common.fn_flags), ZSTR_VAL(fcc->calling_scope->name), ZSTR_VAL(fcc->function_handler->common.function_name));
						}
						// 返回值 0
						retval = 0;
					}
				}
			}
		}
	// 情况4.2，没有找到可用的方法，并且有接收错误信息
	} else if (error) {
		// 有正在调用的域
		if (fcc->calling_scope) {
			// 返回错误信息：此类没有 想要的方法
			if (error) zend_spprintf(error, 0, "class %s does not have a method \"%s\"", ZSTR_VAL(fcc->calling_scope->name), ZSTR_VAL(mname));
			
		// 没有正在调用的域
		} else {
			// 返回错误信息：函数不存在
			if (error) zend_spprintf(error, 0, "function %s() does not exist", ZSTR_VAL(mname));
		}
	}
	
	// 步骤5：清理无用的变量
	// 释放方法名和小写方法名
	zend_string_release_ex(lmname, 0);
	zend_string_release_ex(mname, 0);

	// 如调用信息中有对象
	if (fcc->object) {
		// 记录调用结束时的域
		fcc->called_scope = fcc->object->ce;
		// 如果有 函数处理器 并且 函数处理器是静态的
		if (fcc->function_handler
		 && (fcc->function_handler->common.fn_flags & ZEND_ACC_STATIC)) {
			// 清空调用信息中的对象信息
			fcc->object = NULL;
		}
	}
	return retval;
}
/* }}} */

// ing3, 取得调用字串 class::method 或 函数名.无法解析的数组返回 “Array”
ZEND_API zend_string *zend_get_callable_name_ex(zval *callable, zend_object *object) /* {{{ */
{
try_again:
	// 根据类型操作
	switch (Z_TYPE_P(callable)) {
		// 如果是字串
		case IS_STRING:
			// 如果对象有效
			if (object) {
				// 返回静态调用字串p1::p2，p1:类名，p2:成员名。
				return zend_create_member_string(object->ce->name, Z_STR_P(callable));
			}
			// 只是调用函数，返回函数名
			return zend_string_copy(Z_STR_P(callable));
			
		// 如果是数组
		case IS_ARRAY:
		{
			zval *method = NULL;
			zval *obj = NULL;
			// 如果有2个元素
			if (zend_hash_num_elements(Z_ARRVAL_P(callable)) == 2) {
				// 用哈希值查找 元素 并减少值的引用次数
				obj = zend_hash_index_find_deref(Z_ARRVAL_P(callable), 0);
				// 用哈希值查找 元素 并减少值的引用次数
				method = zend_hash_index_find_deref(Z_ARRVAL_P(callable), 1);
			}

			// 如果方法名不是字串
			if (obj == NULL || method == NULL || Z_TYPE_P(method) != IS_STRING) {
				// 返回 “Array”
				return ZSTR_KNOWN(ZEND_STR_ARRAY_CAPITALIZED);
			}

			// obj 是字串
			if (Z_TYPE_P(obj) == IS_STRING) {
				// 返回静态调用字串p1::p2，p1:类名，p2:成员名。
				return zend_create_member_string(Z_STR_P(obj), Z_STR_P(method));
			// obj 是对象
			} else if (Z_TYPE_P(obj) == IS_OBJECT) {
				// 返回静态调用字串p1::p2，p1:类名，p2:成员名。
				return zend_create_member_string(Z_OBJCE_P(obj)->name, Z_STR_P(method));
			// obj 是其他类型
			} else {
				// 返回 “Array”
				return ZSTR_KNOWN(ZEND_STR_ARRAY_CAPITALIZED);
			}
		}
		// 如果是对象
		case IS_OBJECT:
		{
			// 获取对象的所属类
			zend_class_entry *ce = Z_OBJCE_P(callable);
			// 返回 ： 类名::__invoke
			return zend_string_concat2(
				ZSTR_VAL(ce->name), ZSTR_LEN(ce->name),
				"::__invoke", sizeof("::__invoke") - 1);
		}
		// 如果是引用类型
		case IS_REFERENCE:
			// 追踪到引用目标
			callable = Z_REFVAL_P(callable);
			// 从头再来
			goto try_again;
		// 默认情况
		default:
			// zval 转成 字串, 没有try
			return zval_get_string_func(callable);
	}
}
/* }}} */

// ing4, 取得调用字串 class::method 或 函数名.无法解析的数组返回 “Array”
ZEND_API zend_string *zend_get_callable_name(zval *callable) /* {{{ */
{
	// 取得调用字串 class::method 或 函数名.无法解析的数组返回 “Array”
	return zend_get_callable_name_ex(callable, NULL);
}
/* }}} */

// ing3, 查找调用的函数实体，并检查此调用在 当前指定执行数据中 是否可行
// p1:调用信息，p2:对象, p3:执行数据，p4:检验级别，p5:fcc,关键是它 ，p6:返回错误信息
ZEND_API bool zend_is_callable_at_frame(
		zval *callable, zend_object *object, zend_execute_data *frame,
		uint32_t check_flags, zend_fcall_info_cache *fcc, char **error) /* {{{ */
{
	bool ret;
	zend_fcall_info_cache fcc_local;
	bool strict_class = 0;

	// zend_fcall_info_cache =>F Call info Cache
	// 调用信息缓存为空
	if (fcc == NULL) {
		// 引用返回临时变量
		fcc = &fcc_local;
	}
	// 如果有错误，清空它
	if (error) {
		*error = NULL;
	}
	// 正在调用的域
	fcc->calling_scope = NULL;
	// 调用过的域
	fcc->called_scope = NULL;
	// 处理函数
	fcc->function_handler = NULL;
	// 调用对象
	fcc->object = NULL;

again:
	// 根据调用对象的类型操作
	switch (Z_TYPE_P(callable)) {
		// 字串
		case IS_STRING:
			// 如果有传入对象
			if (object) {
				// 使用传入的对象
				fcc->object = object;
				// 更新 正在调用的域
				fcc->calling_scope = object->ce;
			}
			// 如果只检查语法
			if (check_flags & IS_CALLABLE_CHECK_SYNTAX_ONLY) {
				// 记录调用过的域
				fcc->called_scope = fcc->calling_scope;
				return 1;
			}
// 检查函数
check_func:
			// 查找调用的函数实体，并检查此调用在 当前指定执行数据中 是否可行
			// p1:只用来获取方法名字串，p2:上下文，p3:fcc,从头到尾一直在用 ，p4:是否严格类，p5:返回错误信息, p6:是否抑制弃用信息
			ret = zend_is_callable_check_func(callable, frame, fcc, strict_class, error, check_flags & IS_CALLABLE_SUPPRESS_DEPRECATIONS);
			// 如果使用新建的调用信息缓存
			if (fcc == &fcc_local) {
				// 释放缓存的 函数调用信息
				zend_release_fcall_info_cache(fcc);
			}
			// 返回检查结果
			return ret;
		// 数组
		case IS_ARRAY:
			{
				// 如果哈希表里不是2个元素
				if (zend_hash_num_elements(Z_ARRVAL_P(callable)) != 2) {
					// 返回失败，错误信息：数组回调必须包含2个元素
					if (error) *error = estrdup("array callback must have exactly two members");
					return 0;
				}
				// 第一个元素是 对象
				zval *obj = zend_hash_index_find(Z_ARRVAL_P(callable), 0);
				// 第二个元素是 方法名
				zval *method = zend_hash_index_find(Z_ARRVAL_P(callable), 1);
				// 如果两个里有一个缺失
				if (!obj || !method) {
					// 返回失败，错误信息：数组回调要有两个有效元素
					if (error) *error = estrdup("array callback has to contain indices 0 and 1");
					return 0;
				}
				// 对象引用数 -1
				ZVAL_DEREF(obj);
				// 如果 obj 不是字串 并且 不是对象
				if (Z_TYPE_P(obj) != IS_STRING && Z_TYPE_P(obj) != IS_OBJECT) {
					// 返回失败，错误信息：第一个元素必须是类名或对象
					if (error) *error = estrdup("first array member is not a valid class name or object");
					return 0;
				}
				// 方法名引用数 -1
				ZVAL_DEREF(method);
				// 如果方法名不是字串
				if (Z_TYPE_P(method) != IS_STRING) {
					// 返回失败，错误信息：第二个元素不是有效方法名
					if (error) *error = estrdup("second array member is not a valid method");
					return 0;
				}
				// 如果obj是字串
				if (Z_TYPE_P(obj) == IS_STRING) {
					// 如果只是做语法检查，
					if (check_flags & IS_CALLABLE_CHECK_SYNTAX_ONLY) {
						// 到这里就成功了
						return 1;
					}
					// 检验调用对象的所属类，并完善调用信息
					// p1:类名，p2:域，p3:执行数据，p4:调用信息，p5:返回，是否严格类，p6:返回，错误信息，p7:是否弃用
					if (!zend_is_callable_check_class(Z_STR_P(obj), get_scope(frame), frame, fcc, &strict_class, error, check_flags & IS_CALLABLE_SUPPRESS_DEPRECATIONS)) {
						return 0;
					}
				// 如果obj不是字串
				} else {
					// obj必须是对象
					ZEND_ASSERT(Z_TYPE_P(obj) == IS_OBJECT);
					// 更新正在调用的域 
					fcc->calling_scope = Z_OBJCE_P(obj); /* TBFixed: what if it's overloaded? */
					// 记录方法的所属对象
					fcc->object = Z_OBJ_P(obj);
					// 如果只是语法检查
					if (check_flags & IS_CALLABLE_CHECK_SYNTAX_ONLY) {
						// 记录调用过的域
						fcc->called_scope = fcc->calling_scope;
						// 返回成功
						return 1;
					}
				}
				// 记录被调用方法
				callable = method;
				// 检查此方法
				goto check_func;
			}
			return 0;
		// 对象
		case IS_OBJECT:
			// 如果有 get_closure 方法，调用它
			// 通过指针访问 zval中的对象指针的操作方法集，再访问集合里的指定方法
			if (Z_OBJ_HANDLER_P(callable, get_closure) && Z_OBJ_HANDLER_P(callable, get_closure)(Z_OBJ_P(callable), &fcc->calling_scope, &fcc->function_handler, &fcc->object, 1) == SUCCESS) {
				// 如果调用成功，记录调用过的域
				fcc->called_scope = fcc->calling_scope;
				// 如果使用新建的调用信息缓存
				if (fcc == &fcc_local) {
					// 释放缓存的 函数调用信息
					zend_release_fcall_info_cache(fcc);
				}
				return 1;
			}
			// 返回失败，错误信息：未传入数组或字串
			if (error) *error = estrdup("no array or string given");
			return 0;
		// 引用
		case IS_REFERENCE:
			// 追踪到引用目标
			callable = Z_REFVAL_P(callable);
			// 从头再来
			goto again;
		// 其他情况
		default:
			// 返回错误：没有传入数组或字串
			if (error) *error = estrdup("no array or string given");
			// 返回失败
			return 0;
	}
}
/* }}} */

// ing3, 检查此调用在 当前指定执行数据中 是否可行
// p1:调用信息，p2:对象, p3:检验级别，p4:返回调用字串，p5:fcc,关键是它 ，p6:返回错误信息
ZEND_API bool zend_is_callable_ex(zval *callable, zend_object *object, uint32_t check_flags, zend_string **callable_name, zend_fcall_info_cache *fcc, char **error) /* {{{ */
{
	// 在第一个parent user frame，决定可调用性
	/* Determine callability at the first parent user frame. */
	// 当前执行数据
	zend_execute_data *frame = EG(current_execute_data);
	// 如果执行数据存在 并且 （ 数据中没有函数 或 函数不是用户定义的）
	while (frame && (!frame->func || !ZEND_USER_CODE(frame->func->type))) {
		// 前一个执行数据
		frame = frame->prev_execute_data;
	}

	// 查找调用的函数实体，并检查此调用在 当前指定执行数据中 是否可行
	// p1:调用信息，p2:对象, p3:执行数据，p4:检验级别，p5:fcc,关键是它 ，p6:返回错误信息
	bool ret = zend_is_callable_at_frame(callable, object, frame, check_flags, fcc, error);
	
	// 如果接收调用字串
	if (callable_name) {
		// 取得调用字串 class::method 或 函数名.无法解析的数组返回 “Array”
		*callable_name = zend_get_callable_name_ex(callable, object);
	}
	// 
	return ret;
}

// ing4, 检查此调用在 当前指定执行数据中 是否可行。 p1:调用信息，p2:检验级别，p3:返回调用字串
ZEND_API bool zend_is_callable(zval *callable, uint32_t check_flags, zend_string **callable_name) /* {{{ */
{
	// 检查此调用在 当前指定执行数据中 是否可行
	// p1:调用信息，p2:对象, p3:检验级别，p4:返回调用字串，p5:fcc,关键是它 ，p6:返回错误信息
	return zend_is_callable_ex(callable, NULL, check_flags, callable_name, NULL, NULL);
}
/* }}} */

// ing4, 把callable转成调用的 【类名，方法名】数组
ZEND_API bool zend_make_callable(zval *callable, zend_string **callable_name) /* {{{ */
{
	zend_fcall_info_cache fcc;

	// 检查此调用在 当前指定执行数据中 是否可行
	// p1:调用信息，p2:对象, p3:检验级别，p4:返回调用字串，p5:fcc,关键是它 ，p6:返回错误信息
	if (zend_is_callable_ex(callable, NULL, IS_CALLABLE_SUPPRESS_DEPRECATIONS, callable_name, &fcc, NULL)) {
		// 如果 callable 是字串，并且调用信息中的指定域（函数调用没有这个）
		if (Z_TYPE_P(callable) == IS_STRING && fcc.calling_scope) {
			// 销毁 callable
			zval_ptr_dtor_str(callable);
			// callable 转成数组
			array_init(callable);
			// 先把类名插进去
			add_next_index_str(callable, zend_string_copy(fcc.calling_scope->name));
			// 再把调用的方法名插进去
			add_next_index_str(callable, zend_string_copy(fcc.function_handler->common.function_name));
		}
		// 释放缓存的 函数调用信息
		zend_release_fcall_info_cache(&fcc);
		return 1;
	}
	return 0;
}
/* }}} */

// ing3, 初始化调用信息。p1:方法名，p2:检验级别，p3:调用信息，p4:调用信息缓存，p5:返回调用字串，p6:返回错误信息
ZEND_API zend_result zend_fcall_info_init(zval *callable, uint32_t check_flags, zend_fcall_info *fci, zend_fcall_info_cache *fcc, zend_string **callable_name, char **error) /* {{{ */
{
	// 检查此调用在 当前指定执行数据中 是否可行。如果不可行：
	// p1:调用信息，p2:对象, p3:检验级别，p4:返回调用字串，p5:fcc,关键是它 ，p6:返回错误信息
	if (!zend_is_callable_ex(callable, NULL, check_flags, callable_name, fcc, error)) {
		// 返回失败
		return FAILURE;
	}

	// 调用信息大小
	fci->size = sizeof(*fci);
	// 调用的对象
	fci->object = fcc->object;
	// 复制函数名
	ZVAL_COPY_VALUE(&fci->function_name, callable);
	// 结果变量 null
	fci->retval = NULL;
	// 参数数量 0
	fci->param_count = 0;
	// 参数列表 null
	fci->params = NULL;
	// 命名参数 null
	fci->named_params = NULL;

	return SUCCESS;
}
/* }}} */

// ing4, 清空函数调用信息中的参数表
ZEND_API void zend_fcall_info_args_clear(zend_fcall_info *fci, bool free_mem) /* {{{ */
{
	// 如果调用信息中有参数表
	if (fci->params) {
		// 参数表开头
		zval *p = fci->params;
		// 参数表结尾
		zval *end = p + fci->param_count;

		// 遍历参数表
		while (p != end) {
			// 销毁参数
			i_zval_ptr_dtor(p);
			// 下一个
			p++;
		}
		// 如果需要释放内存
		if (free_mem) {
			// 释放参数表
			efree(fci->params);
			// 指针置空
			fci->params = NULL;
		}
	}
	// 参数数量0
	fci->param_count = 0;
}
/* }}} */

// ing3, 取出并清空参数数量和参数表。p1:调用信息，p2:返回参数数量，p3:返回参数表
ZEND_API void zend_fcall_info_args_save(zend_fcall_info *fci, uint32_t *param_count, zval **params) /* {{{ */
{
	
	*param_count = fci->param_count;
	*params = fci->params;
	fci->param_count = 0;
	fci->params = NULL;
}
/* }}} */

// ing4, 重新保存函数 调用信息的参数信息
ZEND_API void zend_fcall_info_args_restore(zend_fcall_info *fci, uint32_t param_count, zval *params) /* {{{ */
{
	// 清空函数调用信息中的参数表
	zend_fcall_info_args_clear(fci, 1);
	// 新参数数量
	fci->param_count = param_count;
	// 新参数表
	fci->params = params;
}
/* }}} */

// ing4, 创建参数表（zval串）并把给出的参数复制进去，这里处理了需要引用传递的参数。
ZEND_API zend_result zend_fcall_info_args_ex(zend_fcall_info *fci, zend_function *func, zval *args) /* {{{ */
{
	zval *arg, *params;
	uint32_t n = 1;
	// 清空函数调用信息中的参数表
	zend_fcall_info_args_clear(fci, !args);
	// 如果没有参数表
	if (!args) {
		// 直接返回成功
		return SUCCESS;
	}
	// 参数表不是数组
	if (Z_TYPE_P(args) != IS_ARRAY) {
		// 直接返回失败
		return FAILURE;
	}
	// 参数数量，数组元素数
	fci->param_count = zend_hash_num_elements(Z_ARRVAL_P(args));
	// 创建参数列表
	fci->params = params = (zval *) erealloc(fci->params, fci->param_count * sizeof(zval));
	// 遍历参数表
	ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(args), arg) {
		// 如果函数有效 并且 参数不是引用类型 并且 
		// 验证指定参数的发送模式，是否为引用传递 或 优先引用传递
		if (func && !Z_ISREF_P(arg) && ARG_SHOULD_BE_SENT_BY_REF(func, n)) {
			// 如果要求引用传递，创建 zend_reference ，把参数复制进去
			ZVAL_NEW_REF(params, arg);
			// 增加引用次数
			Z_TRY_ADDREF_P(arg);
		// 如果不用引用传递
		} else {
			// 直接复制参数
			ZVAL_COPY(params, arg);
		}
		// 下一个参数
		params++;
		// 计数+1
		n++;
	} ZEND_HASH_FOREACH_END();

	return SUCCESS;
}
/* }}} */

// ing4, 创建参数表（zval串）并把给出的参数复制进去，这里处理了需要引用传递的参数。
ZEND_API zend_result zend_fcall_info_args(zend_fcall_info *fci, zval *args) /* {{{ */
{
	return zend_fcall_info_args_ex(fci, NULL, args);
}
/* }}} */

// ing4, 创建参数列表，并把所有参数复制到新列表里
ZEND_API void zend_fcall_info_argp(zend_fcall_info *fci, uint32_t argc, zval *argv) /* {{{ */
{
	// 清空函数调用信息中的参数表
	zend_fcall_info_args_clear(fci, !argc);
	// 如果有参数
	if (argc) {
		// 更新参数数量 
		fci->param_count = argc;
		// 创建参数列表
		fci->params = (zval *) erealloc(fci->params, fci->param_count * sizeof(zval));
		// 把所有参数复制到新列表里
		for (uint32_t i = 0; i < argc; ++i) {
			ZVAL_COPY(&fci->params[i], &argv[i]);
		}
	}
}
/* }}} */

// ing4, 从函数参数表中获取参数，并添加到调用信息中去
ZEND_API void zend_fcall_info_argv(zend_fcall_info *fci, uint32_t argc, va_list *argv) /* {{{ */
{
	// 清空函数调用信息中的参数表
	zend_fcall_info_args_clear(fci, !argc);

	// 如果有参数表
	if (argc) {
		zval *arg;
		// 更新参数数量
		fci->param_count = argc;
		// 创建参数表
		fci->params = (zval *) erealloc(fci->params, fci->param_count * sizeof(zval));
		// 遍历每个参数
		for (uint32_t i = 0; i < argc; ++i) {
			// 从参数表中获取能数 （zval*）
			arg = va_arg(*argv, zval *);
			// 把添加添加到列表里
			ZVAL_COPY(&fci->params[i], arg);
		}
	}
}
/* }}} */

// ing4, 从函数参数表中获取(任意多个)参数，并添加到调用信息中去
ZEND_API void zend_fcall_info_argn(zend_fcall_info *fci, uint32_t argc, ...) /* {{{ */
{
	va_list argv;

	// 开始实用参数表
	va_start(argv, argc);
	// 从函数参数表中获取参数，并添加到调用信息中去
	zend_fcall_info_argv(fci, argc, &argv);
	// 结束使用参数表
	va_end(argv);
}
/* }}} */


// ing3, 调用函数，p1:调用信息, p2:调用信息缓存, p3:引用返回, p4:参数列表
ZEND_API zend_result zend_fcall_info_call(zend_fcall_info *fci, zend_fcall_info_cache *fcc, zval *retval_ptr, zval *args) /* {{{ */
{
	//
	zval retval, *org_params = NULL;
	uint32_t org_count = 0;
	zend_result result;
	// 绑定返回值变量
	fci->retval = retval_ptr ? retval_ptr : &retval;
	// 如果有参数表
	if (args) {
		// 取出并清空参数数量和参数表。p1:调用信息，p2:返回参数数量，p3:返回参数表
		zend_fcall_info_args_save(fci, &org_count, &org_params);
		// 创建参数表（zval串）并把给出的参数复制进去，这里处理了需要引用传递的参数。
		zend_fcall_info_args(fci, args);
	}
	// 调用函数，传入调用信息 和 调用信息缓存
	result = zend_call_function(fci, fcc);

	// 如果 不接收返回结果 并且 结果不是NULL
	if (!retval_ptr && Z_TYPE(retval) != IS_UNDEF) {
		// 销毁执行结果
		zval_ptr_dtor(&retval);
	}
	// 如果有参数表
	if (args) {
		// 重新保存函数 调用信息的参数信息
		zend_fcall_info_args_restore(fci, org_count, org_params);
	}
	// 返回函数执行结果
	return result;
}
/* }}} */

// ing4, 获取模块版本号
ZEND_API const char *zend_get_module_version(const char *module_name) /* {{{ */
{
	zend_string *lname;
	size_t name_len = strlen(module_name);
	zend_module_entry *module;
	// 新字串
	lname = zend_string_alloc(name_len, 0);
	// 小写模块名
	zend_str_tolower_copy(ZSTR_VAL(lname), module_name, name_len);
	// 模块表里查找
	module = zend_hash_find_ptr(&module_registry, lname);
	// 释放小写模块名
	zend_string_efree(lname);
	// 如果找到模块，返回版本号
	return module ? module->version : NULL;
}
/* }}} */

// ing4, 检验：是内置类 并且 所属模块是永久模块
static zend_always_inline bool is_persistent_class(zend_class_entry *ce) {
	// 检验：是内置类 并且 所属模块是永久模块
	return (ce->type & ZEND_INTERNAL_CLASS)
		&& ce->info.internal.module->type == MODULE_PERSISTENT;
}

// ing3, 声名 类（不是对象）的静态和动态属性。返回属性信息。传入：类入口、属性名、属性值、权限、注释、类型（zend_type）
ZEND_API zend_property_info *zend_declare_typed_property(zend_class_entry *ce, zend_string *name, zval *property, int access_type, zend_string *doc_comment, zend_type type) /* {{{ */
{
	zend_property_info *property_info, *property_info_ptr;
	// 步骤1，修改类标记
	// 如此类型存在
	if (ZEND_TYPE_IS_SET(type)) {
		// 添加标记 有类型引用
		ce->ce_flags |= ZEND_ACC_HAS_TYPE_HINTS;
	}
	// 如果是内置类
	if (ce->type == ZEND_INTERNAL_CLASS) {
		// 创建持久 属性信息
		property_info = pemalloc(sizeof(zend_property_info), 1);
	// 非内置类
	} else {
		// 通过 CG(arena) 创建 属性信息
		property_info = zend_arena_alloc(&CG(arena), sizeof(zend_property_info));
		// 如果 zval类型是 常量语句
		if (Z_TYPE_P(property) == IS_CONSTANT_AST) {
			// 添加标记 更新过的常量
			ce->ce_flags &= ~ZEND_ACC_CONSTANTS_UPDATED;
			// 如果是static属性
			if (access_type & ZEND_ACC_STATIC) {
				// 给类添加 包含 静态成员 标记
				ce->ce_flags |= ZEND_ACC_HAS_AST_STATICS;
			// 如果不是 static, 给类
			} else {
				// 给类添加 包含普通属性 标记
				ce->ce_flags |= ZEND_ACC_HAS_AST_PROPERTIES;
			}
		}
	}
	
	// 上面是属性信息，下面是处理属性值
	// 如果属性类型是 string  并且 值不是保留字
	if (Z_TYPE_P(property) == IS_STRING && !ZSTR_IS_INTERNED(Z_STR_P(property))) {
		// 把属性值创建成 保留字 ?
		zval_make_interned_string(property);
	}
	
	// 如果没有 ppp， 默认是public
	if (!(access_type & ZEND_ACC_PPP_MASK)) {
		access_type |= ZEND_ACC_PUBLIC;
	}
	// 步骤2（最麻烦的在这里），更新类入口 中的属性信息，静态或动态。获取属性信息的offset
	// 如果有 static 标记
	if (access_type & ZEND_ACC_STATIC) {
		// 如果此属性名已存在 并且 有 static 标记
		if ((property_info_ptr = zend_hash_find_ptr(&ce->properties_info, name)) != NULL &&
		    (property_info_ptr->flags & ZEND_ACC_STATIC) != 0) {
			// 使用原有的 序号
			property_info->offset = property_info_ptr->offset;
			// 销毁此 静态属性 原有的值（准备写入新值）
			zval_ptr_dtor(&ce->default_static_members_table[property_info->offset]);
			// 从 属性信息列表中删除此属性的信息（这个在最最后重建）
			zend_hash_del(&ce->properties_info, name);
		// 名称不存在或者没有 static 属性
		} else {
			// 偏移量 = 静态属性数 +1
			property_info->offset = ce->default_static_members_count++;
			// 分配空间扩大静态属性表。这里每次过来都要重新分配，因为一个类不会有太多属性，分配多了浪费。
			ce->default_static_members_table = perealloc(ce->default_static_members_table, sizeof(zval) * ce->default_static_members_count, ce->type == ZEND_INTERNAL_CLASS);
		}
		// 这时候属性已经存在了
		// 把属性的值 copy 给类的静态属性表 的对应 元素
		ZVAL_COPY_VALUE(&ce->default_static_members_table[property_info->offset], property);
		// zval *static_members_table__ptr;
		// 如果没有 ce->static_members_table__ptr 静态成员表
		if (!ZEND_MAP_PTR(ce->static_members_table)) {
			// 如果是内置类 或 属于持久模块
			if (ce->type == ZEND_INTERNAL_CLASS &&
					ce->info.internal.module->type == MODULE_PERSISTENT) {
				// 创建指针 ce->static_members_table__ptr = 
				//  CG(map_ptr_real_base) 指针列表中 创建新的指针，并返回新指针相对于 CG(map_ptr_base) 的偏移量
				ZEND_MAP_PTR_NEW(ce->static_members_table);
			}
		}
	// 如果没有 static 标记
	} else {
		zval *property_default_ptr;
		// 如果类的属性信息表中有此属性 并且 属性信息里没有 static 标记
		if ((property_info_ptr = zend_hash_find_ptr(&ce->properties_info, name)) != NULL &&
		    (property_info_ptr->flags & ZEND_ACC_STATIC) == 0) {
			// 复制 offset
			property_info->offset = property_info_ptr->offset;
			// 从 默认属性表中删除此 属性
			zval_ptr_dtor(&ce->default_properties_table[OBJ_PROP_TO_NUM(property_info->offset)]);
			// 从 属性信息列表中删除此属性信息
			zend_hash_del(&ce->properties_info, name);
			// 必须是内置类
			ZEND_ASSERT(ce->type == ZEND_INTERNAL_CLASS);
			// 属性信息列表不可以是空
			ZEND_ASSERT(ce->properties_info_table != NULL);
			// 更新属性信息列表
			ce->properties_info_table[OBJ_PROP_TO_NUM(property_info->offset)] = property_info;
			
		// 如果属性信息表中没有此属性 或者 有 static 标记（这是怎么回事？）
		} else {
			// 把序号转成偏移量
			property_info->offset = OBJ_PROP_TO_OFFSET(ce->default_properties_count);
			// 默认属性数量  +1
			ce->default_properties_count++;
			// 扩大默认属性表
			ce->default_properties_table = perealloc(ce->default_properties_table, sizeof(zval) * ce->default_properties_count, ce->type == ZEND_INTERNAL_CLASS);

			// 对于 用户定义类，这个在链接过程中处理
			/* For user classes this is handled during linking */
			// 如果类型为内置类
			if (ce->type == ZEND_INTERNAL_CLASS) {
				// 扩大属性信息表 
				ce->properties_info_table = perealloc(ce->properties_info_table, sizeof(zend_property_info *) * ce->default_properties_count, 1);
				// 把属性信息添加进去
				ce->properties_info_table[ce->default_properties_count - 1] = property_info;
			}
		}
		// 默认属性指针，指向当前属性
		property_default_ptr = &ce->default_properties_table[OBJ_PROP_TO_NUM(property_info->offset)];
		// 上面已经创建好了属性实例，现在把值添加进去
		ZVAL_COPY_VALUE(property_default_ptr, property);
		// 给此属性添加标记 
		Z_PROP_FLAG_P(property_default_ptr) = Z_ISUNDEF_P(property) ? IS_PROP_UNINIT : 0;
	}
	// 如果类型是内置类
	if (ce->type & ZEND_INTERNAL_CLASS) {
		// 
		/* Must be interned to avoid ZTS data races */
		// 如果是持久类
		if (is_persistent_class(ce)) {
			// 属性名创保留字
			name = zend_new_interned_string(zend_string_copy(name));
		}
		// 如果是可计数类型
		if (Z_REFCOUNTED_P(property)) {
			// 报错，内置zval不可计数
			zend_error_noreturn(E_CORE_ERROR, "Internal zvals cannot be refcounted");
		}
	}

	// 步骤3，填充信息，构造 属性信息
	// 如果 是public
	if (access_type & ZEND_ACC_PUBLIC) {
		// 名称
		property_info->name = zend_string_copy(name);
	// 如果 是private
	} else if (access_type & ZEND_ACC_PRIVATE) {
		// 拼接属性名 类名+属性名 持久
		property_info->name = zend_mangle_property_name(ZSTR_VAL(ce->name), ZSTR_LEN(ce->name), ZSTR_VAL(name), ZSTR_LEN(name), is_persistent_class(ce));
	// 必须 是protected
	} else {
		ZEND_ASSERT(access_type & ZEND_ACC_PROTECTED);
		// 拼接属性名 *+属性名 持久性跟类走
		property_info->name = zend_mangle_property_name("*", 1, ZSTR_VAL(name), ZSTR_LEN(name), is_persistent_class(ce));
	}
	// 名称 创建保留字
	property_info->name = zend_new_interned_string(property_info->name);
	// 标记
	property_info->flags = access_type;
	// 注释
	property_info->doc_comment = doc_comment;
	// 修饰属性
	property_info->attributes = NULL;
	// 所属类
	property_info->ce = ce;
	// 类型（zend_type）
	property_info->type = type;

	// 如果是持久类，处理类型
	if (is_persistent_class(ce)) {
		zend_type *single_type;
		// 遍历 zend_type
		ZEND_TYPE_FOREACH(property_info->type, single_type) {
			// TODO Add support and test cases when gen_stub support added
			// 单个type不可以包含列表
			ZEND_ASSERT(!ZEND_TYPE_HAS_LIST(*single_type));
			// 如果类型有名字
			if (ZEND_TYPE_HAS_NAME(*single_type)) {
				// 名字创建保留字
				zend_string *name = zend_new_interned_string(ZEND_TYPE_NAME(*single_type));
				// 关联到 single_type
				ZEND_TYPE_SET_PTR(*single_type, name);
				// 缓存 类入口
				zend_alloc_ce_cache(name);
			}
		} ZEND_TYPE_FOREACH_END();
	}
	// 步骤4，更新属性信息
	// 更新属性信息列表中的 相应条目
	zend_hash_update_ptr(&ce->properties_info, name, property_info);
	// 返回属性信息
	return property_info;
}
/* }}} */

// ing3, 如果通过检测，把值赋给 ref的附加zval
ZEND_API zend_result zend_try_assign_typed_ref_ex(zend_reference *ref, zval *val, bool strict) /* {{{ */
{
	// zend_execute.c
	// 检查值是否适配属性信息列表的每一条信息，有一条不适配就返回失败。 p1:属性信息列表的引用,p2:值
	if (UNEXPECTED(!zend_verify_ref_assignable_zval(ref, val, strict))) {
		// 如果失败，销毁 val
		zval_ptr_dtor(val);
		return FAILURE;
	// 如果通过检查
	} else {
		// 清空 ref的附加 zval
		zval_ptr_dtor(&ref->val);
		// 把值复制给 ref的附加 zval
		ZVAL_COPY_VALUE(&ref->val, val);
		return SUCCESS;
	}
}
/* }}} */

// ing3, 如果通过检测，把值赋给 ref的附加zval
ZEND_API zend_result zend_try_assign_typed_ref(zend_reference *ref, zval *val) /* {{{ */
{
	return zend_try_assign_typed_ref_ex(ref, val, ZEND_ARG_USES_STRICT_TYPES());
}
/* }}} */

// ing3, 给引用目标赋值，值为 null
ZEND_API zend_result zend_try_assign_typed_ref_null(zend_reference *ref) /* {{{ */
{
	zval tmp;

	ZVAL_NULL(&tmp);
	return zend_try_assign_typed_ref(ref, &tmp);
}
/* }}} */

// ing3, 给引用目标赋值，值为 布尔型
ZEND_API zend_result zend_try_assign_typed_ref_bool(zend_reference *ref, bool val) /* {{{ */
{
	zval tmp;

	ZVAL_BOOL(&tmp, val);
	return zend_try_assign_typed_ref(ref, &tmp);
}
/* }}} */

// ing3, 给引用目标赋值，值为 整数
ZEND_API zend_result zend_try_assign_typed_ref_long(zend_reference *ref, zend_long lval) /* {{{ */
{
	zval tmp;

	ZVAL_LONG(&tmp, lval);
	return zend_try_assign_typed_ref(ref, &tmp);
}
/* }}} */

// ing3, 给引用目标赋值，值为 小数
ZEND_API zend_result zend_try_assign_typed_ref_double(zend_reference *ref, double dval) /* {{{ */
{
	zval tmp;

	ZVAL_DOUBLE(&tmp, dval);
	return zend_try_assign_typed_ref(ref, &tmp);
}
/* }}} */

// ing3, 给引用目标赋值，值为 空字串
ZEND_API zend_result zend_try_assign_typed_ref_empty_string(zend_reference *ref) /* {{{ */
{
	zval tmp;

	ZVAL_EMPTY_STRING(&tmp);
	return zend_try_assign_typed_ref(ref, &tmp);
}
/* }}} */

// ing3, 给引用目标赋值，值为 zend_string
ZEND_API zend_result zend_try_assign_typed_ref_str(zend_reference *ref, zend_string *str) /* {{{ */
{
	zval tmp;

	ZVAL_STR(&tmp, str);
	return zend_try_assign_typed_ref(ref, &tmp);
}
/* }}} */

// ing3, 给引用目标赋值，值为 字串
ZEND_API zend_result zend_try_assign_typed_ref_string(zend_reference *ref, const char *string) /* {{{ */
{
	zval tmp;

	ZVAL_STRING(&tmp, string);
	return zend_try_assign_typed_ref(ref, &tmp);
}
/* }}} */

// ing3, 给引用目标赋值，值为 字串和长度
ZEND_API zend_result zend_try_assign_typed_ref_stringl(zend_reference *ref, const char *string, size_t len) /* {{{ */
{
	zval tmp;

	ZVAL_STRINGL(&tmp, string, len);
	return zend_try_assign_typed_ref(ref, &tmp);
}
/* }}} */

// ing3, 给引用目标赋值，值为 数组
ZEND_API zend_result zend_try_assign_typed_ref_arr(zend_reference *ref, zend_array *arr) /* {{{ */
{
	zval tmp;

	ZVAL_ARR(&tmp, arr);
	return zend_try_assign_typed_ref(ref, &tmp);
}
/* }}} */

// ing3, 给引用目标赋值，值为 资源
ZEND_API zend_result zend_try_assign_typed_ref_res(zend_reference *ref, zend_resource *res) /* {{{ */
{
	zval tmp;

	ZVAL_RES(&tmp, res);
	return zend_try_assign_typed_ref(ref, &tmp);
}
/* }}} */

// ing3, 给引用目标赋值，值为 zval
ZEND_API zend_result zend_try_assign_typed_ref_zval(zend_reference *ref, zval *zv) /* {{{ */
{
	zval tmp;

	ZVAL_COPY_VALUE(&tmp, zv);
	return zend_try_assign_typed_ref(ref, &tmp);
}
/* }}} */

// ing3, 给引用目标赋值，值为 zval, p3:是否严格控制类型
ZEND_API zend_result zend_try_assign_typed_ref_zval_ex(zend_reference *ref, zval *zv, bool strict) /* {{{ */
{
	zval tmp;

	ZVAL_COPY_VALUE(&tmp, zv);
	return zend_try_assign_typed_ref_ex(ref, &tmp, strict);
}
/* }}} */

// ing4, 声名 无类型限定的 类（不是对象）的静态和动态属性。zend_type 为0。 传入：类入口、属性名、属性值、权限、注释
ZEND_API void zend_declare_property_ex(zend_class_entry *ce, zend_string *name, zval *property, int access_type, zend_string *doc_comment) /* {{{ */
{
	// 声名 类（不是对象）的静态和动态属性。返回属性信息。
	// 传入：类入口、属性名、属性值、权限、注释、类型（zend_type）
	zend_declare_typed_property(ce, name, property, access_type, doc_comment, (zend_type) ZEND_TYPE_INIT_NONE(0));
}
/* }}} */

// ing4, 声名 无类型限定的 类（不是对象）的静态和动态属性。 传入：类入口、属性名、名称长度、属性值、权限
ZEND_API void zend_declare_property(zend_class_entry *ce, const char *name, size_t name_length, zval *property, int access_type) /* {{{ */
{
	// 属性名
	zend_string *key = zend_string_init(name, name_length, is_persistent_class(ce));
	// 声名 无类型限定的 类（不是对象）的静态和动态属性。zend_type 为0。 传入：类入口、属性名、属性值、权限、注释 为 NULL
	zend_declare_property_ex(ce, key, property, access_type, NULL);
	zend_string_release(key);
}
/* }}} */

// ing4, 声名类属性，值为 null
ZEND_API void zend_declare_property_null(zend_class_entry *ce, const char *name, size_t name_length, int access_type) /* {{{ */
{
	zval property;

	ZVAL_NULL(&property);
	// 声名 无类型限定的 类（不是对象）的静态和动态属性。 传入：类入口、属性名、名称长度、属性值、权限
	zend_declare_property(ce, name, name_length, &property, access_type);
}
/* }}} */

// ing4, 声名类属性，值为 布尔型
ZEND_API void zend_declare_property_bool(zend_class_entry *ce, const char *name, size_t name_length, zend_long value, int access_type) /* {{{ */
{
	zval property;

	ZVAL_BOOL(&property, value);
	// 声名 无类型限定的 类（不是对象）的静态和动态属性。 传入：类入口、属性名、名称长度、属性值、权限
	zend_declare_property(ce, name, name_length, &property, access_type);
}
/* }}} */

// ing4, 声名类属性，值为 整数
ZEND_API void zend_declare_property_long(zend_class_entry *ce, const char *name, size_t name_length, zend_long value, int access_type) /* {{{ */
{
	zval property;

	ZVAL_LONG(&property, value);
	// 声名 无类型限定的 类（不是对象）的静态和动态属性。 传入：类入口、属性名、名称长度、属性值、权限
	zend_declare_property(ce, name, name_length, &property, access_type);
}
/* }}} */

// ing4, 声名类属性，值为 小数
ZEND_API void zend_declare_property_double(zend_class_entry *ce, const char *name, size_t name_length, double value, int access_type) /* {{{ */
{
	zval property;

	ZVAL_DOUBLE(&property, value);
	// 声名 无类型限定的 类（不是对象）的静态和动态属性。 传入：类入口、属性名、名称长度、属性值、权限
	zend_declare_property(ce, name, name_length, &property, access_type);
}
/* }}} */

// ing4, 声名类属性，值为 字串
ZEND_API void zend_declare_property_string(zend_class_entry *ce, const char *name, size_t name_length, const char *value, int access_type) /* {{{ */
{
	zval property;

	ZVAL_NEW_STR(&property, zend_string_init(value, strlen(value), ce->type & ZEND_INTERNAL_CLASS));
	// 声名 无类型限定的 类（不是对象）的静态和动态属性。 传入：类入口、属性名、名称长度、属性值、权限
	zend_declare_property(ce, name, name_length, &property, access_type);
}
/* }}} */

// ing4, 声名类属性，值为 字串和长度
ZEND_API void zend_declare_property_stringl(zend_class_entry *ce, const char *name, size_t name_length, const char *value, size_t value_len, int access_type) /* {{{ */
{
	zval property;

	ZVAL_NEW_STR(&property, zend_string_init(value, value_len, ce->type & ZEND_INTERNAL_CLASS));
	// 声名 无类型限定的 类（不是对象）的静态和动态属性。 传入：类入口、属性名、名称长度、属性值、权限
	zend_declare_property(ce, name, name_length, &property, access_type);
}
/* }}} */

// ing4, 定义类常量 p1:类, p2:常量名, p3:常量值, p4:标记, p5:文档注释
ZEND_API zend_class_constant *zend_declare_class_constant_ex(zend_class_entry *ce, zend_string *name, zval *value, int flags, zend_string *doc_comment) /* {{{ */
{
	zend_class_constant *c;

	// 如果当前是接口
	if (ce->ce_flags & ZEND_ACC_INTERFACE) {
		// 如果不是public：报错，接口常量必须为public
		if (!(flags & ZEND_ACC_PUBLIC)) {
			zend_error_noreturn(E_COMPILE_ERROR, "Access type for interface constant %s::%s must be public", ZSTR_VAL(ce->name), ZSTR_VAL(name));
		}
	}
	// 如果常量名是 class
	if (zend_string_equals_literal_ci(name, "class")) {
		// 内置类报内核错误，用户类报 编译错误：类常量不可以叫做class，它被保留用来获取类名
		zend_error_noreturn(ce->type == ZEND_INTERNAL_CLASS ? E_CORE_ERROR : E_COMPILE_ERROR,
				"A class constant must not be called 'class'; it is reserved for class name fetching");
	}

	// 如果值类型是字串 并且不是 内置字串
	if (Z_TYPE_P(value) == IS_STRING && !ZSTR_IS_INTERNED(Z_STR_P(value))) {
		// 创建保留字串 
		zval_make_interned_string(value);
	}

	// 如果是内置类
	if (ce->type == ZEND_INTERNAL_CLASS) {
		// 分配永久内存，创建类常量
		c = pemalloc(sizeof(zend_class_constant), 1);
	// 用户类
	} else {
		// 分配内存创建类常量
		c = zend_arena_alloc(&CG(arena), sizeof(zend_class_constant));
	}
	// 复制常量值
	ZVAL_COPY_VALUE(&c->value, value);
	// 添加标记
	ZEND_CLASS_CONST_FLAGS(c) = flags;
	// 添加文档注释
	c->doc_comment = doc_comment;
	// 修饰属性为空
	c->attributes = NULL;
	// 关联所属类
	c->ce = ce;
	// 如果是表达式常量
	if (Z_TYPE_P(value) == IS_CONSTANT_AST) {
		// 删除 已更新常量 标记
		ce->ce_flags &= ~ZEND_ACC_CONSTANTS_UPDATED;
		// 添加 有表达式常量 标记
		ce->ce_flags |= ZEND_ACC_HAS_AST_CONSTANTS;
		// 如果是内置类 并且 没有 mutable_data 指针
		if (ce->type == ZEND_INTERNAL_CLASS && !ZEND_MAP_PTR(ce->mutable_data)) {
			// 创建转换数据指针
			ZEND_MAP_PTR_NEW(ce->mutable_data);
		}
	}

	// 常量添加到量表里
	if (!zend_hash_add_ptr(&ce->constants_table, name, c)) {
		// 如果出错：报错：不可以重复定义类常量
		zend_error_noreturn(ce->type == ZEND_INTERNAL_CLASS ? E_CORE_ERROR : E_COMPILE_ERROR,
			"Cannot redefine class constant %s::%s", ZSTR_VAL(ce->name), ZSTR_VAL(name));
	}

	return c;
}
/* }}} */

// ing3, 声名类常量，值为zval
ZEND_API void zend_declare_class_constant(zend_class_entry *ce, const char *name, size_t name_length, zval *value) /* {{{ */
{
	zend_string *key;
	// 如果是内置类
	if (ce->type == ZEND_INTERNAL_CLASS) {
		// 键名为内置字串
		key = zend_string_init_interned(name, name_length, 1);
	// 如果是用户类
	} else {
		// 键名为普通字串
		key = zend_string_init(name, name_length, 0);
	}
	// 定义类常量 p1:类, p2:常量名, p3:常量值, p4:标记, p5:文档注释
	zend_declare_class_constant_ex(ce, key, value, ZEND_ACC_PUBLIC, NULL);
	// 释放常量名
	zend_string_release(key);
}
/* }}} */

// ing4, 声名类常量，值为 null
ZEND_API void zend_declare_class_constant_null(zend_class_entry *ce, const char *name, size_t name_length) /* {{{ */
{
	zval constant;

	ZVAL_NULL(&constant);
	zend_declare_class_constant(ce, name, name_length, &constant);
}
/* }}} */

// ing4, 声名类常量，值为 整数
ZEND_API void zend_declare_class_constant_long(zend_class_entry *ce, const char *name, size_t name_length, zend_long value) /* {{{ */
{
	zval constant;

	ZVAL_LONG(&constant, value);
	zend_declare_class_constant(ce, name, name_length, &constant);
}
/* }}} */

// ing4, 声名类常量，值为 布尔型
ZEND_API void zend_declare_class_constant_bool(zend_class_entry *ce, const char *name, size_t name_length, bool value) /* {{{ */
{
	zval constant;

	ZVAL_BOOL(&constant, value);
	zend_declare_class_constant(ce, name, name_length, &constant);
}
/* }}} */

// ing4, 声名类常量，值为 小数
ZEND_API void zend_declare_class_constant_double(zend_class_entry *ce, const char *name, size_t name_length, double value) /* {{{ */
{
	zval constant;

	ZVAL_DOUBLE(&constant, value);
	zend_declare_class_constant(ce, name, name_length, &constant);
}
/* }}} */

// ing4, 声名类常量，值为 字串和长度
ZEND_API void zend_declare_class_constant_stringl(zend_class_entry *ce, const char *name, size_t name_length, const char *value, size_t value_length) /* {{{ */
{
	zval constant;

	ZVAL_NEW_STR(&constant, zend_string_init(value, value_length, ce->type & ZEND_INTERNAL_CLASS));
	zend_declare_class_constant(ce, name, name_length, &constant);
}
/* }}} */

// ing4, 声名类常量，值为 字串
ZEND_API void zend_declare_class_constant_string(zend_class_entry *ce, const char *name, size_t name_length, const char *value) /* {{{ */
{
	zend_declare_class_constant_stringl(ce, name, name_length, value, strlen(value));
}
/* }}} */

// ing3, 更新对象的属性，scope更新过程中用到的作用域，p1:类入口，p2:对象，p3:属性名，p4:属性值
ZEND_API void zend_update_property_ex(zend_class_entry *scope, zend_object *object, zend_string *name, zval *value) /* {{{ */
{
	// 旧作用域
	zend_class_entry *old_scope = EG(fake_scope);
	// 更新旧作用域
	EG(fake_scope) = scope;
	// 调用 write_property 方法， 更新指定名字的属性
	object->handlers->write_property(object, name, value, NULL);
	// 旧作用域更新回去
	EG(fake_scope) = old_scope;
}
/* }}} */

// ing3, 更新对象的属性，scope更新过程中用到的作用域
ZEND_API void zend_update_property(zend_class_entry *scope, zend_object *object, const char *name, size_t name_length, zval *value) /* {{{ */
{
	zend_string *property;
	// 旧作用域
	zend_class_entry *old_scope = EG(fake_scope);
	// 更新旧作用域
	EG(fake_scope) = scope;
	// 属性名
	property = zend_string_init(name, name_length, 0);
	// 调用 write_property 方法， 更新指定名字的属性
	object->handlers->write_property(object, property, value, NULL);
	// 释放属性名
	zend_string_release_ex(property, 0);
	// 旧作用域更新回去
	EG(fake_scope) = old_scope;
}
/* }}} */

// ing3, 更新对象的属性值为null，scope更新过程中用到的作用域
ZEND_API void zend_update_property_null(zend_class_entry *scope, zend_object *object, const char *name, size_t name_length) /* {{{ */
{
	zval tmp;

	ZVAL_NULL(&tmp);
	zend_update_property(scope, object, name, name_length, &tmp);
}
/* }}} */

// ing3, 删除指定属性，scope更新过程中用到的作用域
ZEND_API void zend_unset_property(zend_class_entry *scope, zend_object *object, const char *name, size_t name_length) /* {{{ */
{
	zend_string *property;
	// 旧作用域
	zend_class_entry *old_scope = EG(fake_scope);
	// 更新旧作用域
	EG(fake_scope) = scope;
	// 初始化属性名
	property = zend_string_init(name, name_length, 0);
	// 调用对象自带的删除方法
	object->handlers->unset_property(object, property, 0);
	// 释放属性名
	zend_string_release_ex(property, 0);
	// 旧作用域更新回去
	EG(fake_scope) = old_scope;
}
/* }}} */

// ing3, 更新对象的属性值为 bool型，scope更新过程中用到的作用域
ZEND_API void zend_update_property_bool(zend_class_entry *scope, zend_object *object, const char *name, size_t name_length, zend_long value) /* {{{ */
{
	zval tmp;

	ZVAL_BOOL(&tmp, value);
	zend_update_property(scope, object, name, name_length, &tmp);
}
/* }}} */

// ing3, 更新对象的属性值为 整数，scope更新过程中用到的作用域
ZEND_API void zend_update_property_long(zend_class_entry *scope, zend_object *object, const char *name, size_t name_length, zend_long value) /* {{{ */
{
	zval tmp;

	ZVAL_LONG(&tmp, value);
	zend_update_property(scope, object, name, name_length, &tmp);
}
/* }}} */

// ing3, 更新对象的属性值为 小数，scope更新过程中用到的作用域
ZEND_API void zend_update_property_double(zend_class_entry *scope, zend_object *object, const char *name, size_t name_length, double value) /* {{{ */
{
	zval tmp;

	ZVAL_DOUBLE(&tmp, value);
	zend_update_property(scope, object, name, name_length, &tmp);
}
/* }}} */

// ing3, 更新对象的属性值为 字串，scope更新过程中用到的作用域
ZEND_API void zend_update_property_str(zend_class_entry *scope, zend_object *object, const char *name, size_t name_length, zend_string *value) /* {{{ */
{
	zval tmp;

	ZVAL_STR(&tmp, value);
	zend_update_property(scope, object, name, name_length, &tmp);
}
/* }}} */

// ing3, 更新对象的属性值为 字串（引用次数为0），scope更新过程中用到的作用域
ZEND_API void zend_update_property_string(zend_class_entry *scope, zend_object *object, const char *name, size_t name_length, const char *value) /* {{{ */
{
	zval tmp;

	ZVAL_STRING(&tmp, value);
	Z_SET_REFCOUNT(tmp, 0);
	zend_update_property(scope, object, name, name_length, &tmp);
}
/* }}} */

// ing3, 更新对象的属性值为 字串（指定长度），scope更新过程中用到的作用域
ZEND_API void zend_update_property_stringl(zend_class_entry *scope, zend_object *object, const char *name, size_t name_length, const char *value, size_t value_len) /* {{{ */
{
	zval tmp;

	ZVAL_STRINGL(&tmp, value, value_len);
	Z_SET_REFCOUNT(tmp, 0);
	zend_update_property(scope, object, name, name_length, &tmp);
}
/* }}} */

// ing3, 更新静态属性
ZEND_API zend_result zend_update_static_property_ex(zend_class_entry *scope, zend_string *name, zval *value) /* {{{ */
{
	zval *property, tmp;
	zend_property_info *prop_info;
	// 旧作用域
	zend_class_entry *old_scope = EG(fake_scope);
	// 如果不是 已经更新过的常量 
	if (UNEXPECTED(!(scope->ce_flags & ZEND_ACC_CONSTANTS_UPDATED))) {
		// 如果更新常量（常量，静态属性，普通属性）失败
		if (UNEXPECTED(zend_update_class_constants(scope)) != SUCCESS) {
			// 返回失败
			return FAILURE;
		}
	}
	// 切换到新域
	EG(fake_scope) = scope;
	// 标准方法：获取静态成员 和 成员信息
	property = zend_std_get_static_property_with_info(scope, name, BP_VAR_W, &prop_info);
	// 切换到旧域
	EG(fake_scope) = old_scope;
	// 如果静态成员无效
	if (!property) {
		// 返回失败
		return FAILURE;
	}
	// 值不可以是引用类型
	ZEND_ASSERT(!Z_ISREF_P(value));
	// 增加引用次数
	Z_TRY_ADDREF_P(value);
	// 如果有类型
	if (ZEND_TYPE_IS_SET(prop_info->type)) {
		// 创建副本给 tmp
		ZVAL_COPY_VALUE(&tmp, value);
		// 检查属性值类型 与 属性信息要求的类型是否匹配
		if (!zend_verify_property_type(prop_info, &tmp, /* strict */ 0)) {
			// 减少引用次数
			Z_TRY_DELREF_P(value);
			// 返回失败
			return FAILURE;
		}
		// 切换到副本
		value = &tmp;
	}
	// 给静态成员变量赋值
	zend_assign_to_variable(property, value, IS_TMP_VAR, /* strict */ 0);
	return SUCCESS;
}
/* }}} */

// ing3, 更新静态成员变量 ，scope更新过程中用到的作用域
ZEND_API zend_result zend_update_static_property(zend_class_entry *scope, const char *name, size_t name_length, zval *value) /* {{{ */
{
	zend_string *key = zend_string_init(name, name_length, 0);
	zend_result retval = zend_update_static_property_ex(scope, key, value);
	zend_string_efree(key);
	return retval;
}
/* }}} */

// ing3, 更新静态成员变量的值为null ，scope更新过程中用到的作用域
ZEND_API zend_result zend_update_static_property_null(zend_class_entry *scope, const char *name, size_t name_length) /* {{{ */
{
	zval tmp;

	ZVAL_NULL(&tmp);
	return zend_update_static_property(scope, name, name_length, &tmp);
}
/* }}} */

// ing3, 更新静态成员变量的值为布尔型 ，scope更新过程中用到的作用域
ZEND_API zend_result zend_update_static_property_bool(zend_class_entry *scope, const char *name, size_t name_length, zend_long value) /* {{{ */
{
	zval tmp;

	ZVAL_BOOL(&tmp, value);
	return zend_update_static_property(scope, name, name_length, &tmp);
}
/* }}} */

// ing3, 更新静态成员变量的值为整数 ，scope更新过程中用到的作用域
ZEND_API zend_result zend_update_static_property_long(zend_class_entry *scope, const char *name, size_t name_length, zend_long value) /* {{{ */
{
	zval tmp;

	ZVAL_LONG(&tmp, value);
	return zend_update_static_property(scope, name, name_length, &tmp);
}
/* }}} */

// ing3, 更新静态成员变量的值为小数 ，scope更新过程中用到的作用域
ZEND_API zend_result zend_update_static_property_double(zend_class_entry *scope, const char *name, size_t name_length, double value) /* {{{ */
{
	zval tmp;

	ZVAL_DOUBLE(&tmp, value);
	return zend_update_static_property(scope, name, name_length, &tmp);
}
/* }}} */

// ing3, 更新静态成员变量的值为字串 ，scope更新过程中用到的作用域
ZEND_API zend_result zend_update_static_property_string(zend_class_entry *scope, const char *name, size_t name_length, const char *value) /* {{{ */
{
	zval tmp;

	ZVAL_STRING(&tmp, value);
	Z_SET_REFCOUNT(tmp, 0);
	return zend_update_static_property(scope, name, name_length, &tmp);
}
/* }}} */

// ing3, 更新静态成员变量的值为字串（值带长度） ，scope更新过程中用到的作用域
ZEND_API zend_result zend_update_static_property_stringl(zend_class_entry *scope, const char *name, size_t name_length, const char *value, size_t value_len) /* {{{ */
{
	zval tmp;

	ZVAL_STRINGL(&tmp, value, value_len);
	Z_SET_REFCOUNT(tmp, 0);
	return zend_update_static_property(scope, name, name_length, &tmp);
}
/* }}} */

// ing3, 调用对象的方法，读取普通成员属性（属性名是 zend_string）
ZEND_API zval *zend_read_property_ex(zend_class_entry *scope, zend_object *object, zend_string *name, bool silent, zval *rv) /* {{{ */
{
	zval *value;
	zend_class_entry *old_scope = EG(fake_scope);

	EG(fake_scope) = scope;
	// zend_std_read_property。第四个参数是 cache_slot
	value = object->handlers->read_property(object, name, silent?BP_VAR_IS:BP_VAR_R, NULL, rv);

	EG(fake_scope) = old_scope;
	return value;
}
/* }}} */

// ing4, 调用对象的方法，读取普通成员属性（属性名是 char*,带长度）
ZEND_API zval *zend_read_property(zend_class_entry *scope, zend_object *object, const char *name, size_t name_length, bool silent, zval *rv) /* {{{ */
{
	zval *value;
	zend_string *str;

	str = zend_string_init(name, name_length, 0);
	value = zend_read_property_ex(scope, object, str, silent, rv);
	zend_string_release_ex(str, 0);
	return value;
}
/* }}} */

// ing3, 读取静态成员变量（key 是 zend_string）
ZEND_API zval *zend_read_static_property_ex(zend_class_entry *scope, zend_string *name, bool silent) /* {{{ */
{
	zval *property;
	// 保存旧域
	zend_class_entry *old_scope = EG(fake_scope);
	// 切换到新旧
	EG(fake_scope) = scope;
	// 读取静态成员变量，不带成员信息
	property = zend_std_get_static_property(scope, name, silent ? BP_VAR_IS : BP_VAR_R);
	// 切换回旧域
	EG(fake_scope) = old_scope;
	// 返回结果
	return property;
}
/* }}} */

// ing3, 读取静态成员变量（key 是 char* ，带长度）
ZEND_API zval *zend_read_static_property(zend_class_entry *scope, const char *name, size_t name_length, bool silent) /* {{{ */
{
	zend_string *key = zend_string_init(name, name_length, 0);
	zval *property = zend_read_static_property_ex(scope, key, silent);
	zend_string_efree(key);
	return property;
}
/* }}} */

// ing3, 把运行时中的 错误处理器和 异常类 保存到 zend_error_handling 中
ZEND_API void zend_save_error_handling(zend_error_handling *current) /* {{{ */
{
	current->handling = EG(error_handling);
	current->exception = EG(exception_class);
}
/* }}} */

// ing3, 把运行时中的 错误处理器和 异常类 保存到 current 中， 传入的 异常处理方法 和 异常类应用到运行时中
ZEND_API void zend_replace_error_handling(zend_error_handling_t error_handling, zend_class_entry *exception_class, zend_error_handling *current) /* {{{ */
{
	// 把运行时中的 错误处理器和 异常类 保存到 current 中
	if (current) {
		zend_save_error_handling(current);
	}
	// 断言： 处理方法是 EH_THROW 或 没有异常类
	ZEND_ASSERT(error_handling == EH_THROW || exception_class == NULL);
	// 传入的 异常处理方法 和 异常类应用到运行时中
	EG(error_handling) = error_handling;
	EG(exception_class) = exception_class;
}
/* }}} */

// ing3, 把保存的处理器应用到运行时中
ZEND_API void zend_restore_error_handling(zend_error_handling *saved) /* {{{ */
{
	EG(error_handling) = saved->handling;
	EG(exception_class) = saved->exception;
}
/* }}} */

// ing4, 返回对象类型（trait、接口、枚举、类）
ZEND_API ZEND_COLD const char *zend_get_object_type_case(const zend_class_entry *ce, bool upper_case) /* {{{ */
{
	// trait
	if (ce->ce_flags & ZEND_ACC_TRAIT) {
		return upper_case ? "Trait" : "trait";
	// 接口
	} else if (ce->ce_flags & ZEND_ACC_INTERFACE) {
		return upper_case ? "Interface" : "interface";
	// 枚举
	} else if (ce->ce_flags & ZEND_ACC_ENUM) {
		return upper_case ? "Enum" : "enum";
	// 类
	} else {
		return upper_case ? "Class" : "class";
	}
}
/* }}} */

// ing4, 是否可迭代
ZEND_API bool zend_is_iterable(zval *iterable) /* {{{ */
{
	// 按类型区分
	switch (Z_TYPE_P(iterable)) {
		// 数据可迭代
		case IS_ARRAY:
			return 1;
		// 对象
		case IS_OBJECT:
			// 实现了 traversable 接口的可迭代
			return zend_class_implements_interface(Z_OBJCE_P(iterable), zend_ce_traversable);
		// 其他都不可迭代
		default:
			return 0;
	}
}
/* }}} */

// ing4, 是否可计数
ZEND_API bool zend_is_countable(zval *countable) /* {{{ */
{
	// 按所属类型操作
	switch (Z_TYPE_P(countable)) {
		// 数组可计数
		case IS_ARRAY:
			return 1;
		// 对象
		case IS_OBJECT:
			// 如果属性中有 count_elements 元素
			if (Z_OBJ_HT_P(countable)->count_elements) {
				// 返回可以
				return 1;
			}
			// 如果实现了 countable 接口，也可以
			return zend_class_implements_interface(Z_OBJCE_P(countable), zend_ce_countable);
		// 其他东东都不可以
		default:
			return 0;
	}
}
/* }}} */

// ing3, 通过语句获取默认值
static zend_result get_default_via_ast(zval *default_value_zval, const char *default_value) {
	zend_ast *ast;
	zend_arena *ast_arena;

	// 把默认值前面拚一个 "<?php " 后面加一个 ;
	zend_string *code = zend_string_concat3(
		"<?php ", sizeof("<?php ") - 1, default_value, strlen(default_value), ";", 1);

	// 字串转成语句对象 Zend/zend_language_scanner.l
	ast = zend_compile_string_to_ast(code, &ast_arena, ZSTR_EMPTY_ALLOC());
	// 释放字串
	zend_string_release(code);
	// 如果创建语句失败
	if (!ast) {
		// 返回失败
		return FAILURE;
	}

	// 语句列表
	zend_ast_list *statement_list = zend_ast_get_list(ast);
	// 语句中的常量表达式
	zend_ast **const_expr_ast_ptr = &statement_list->child[0];

	// 原 CG(ast_arena)
	zend_arena *original_ast_arena = CG(ast_arena);
	// 原编译配置
	uint32_t original_compiler_options = CG(compiler_options);
	// 原文件上下文
	zend_file_context original_file_context;
	// 使用 临时 ast_arena
	CG(ast_arena) = ast_arena;
	
	// 禁用常量替换，让 getDefaultValueConstant() 起作用
	/* Disable constant substitution, to make getDefaultValueConstant() work. */
	
	// 禁用编译时的 常量替换，禁用编译时的持久常量替换
	CG(compiler_options) |= ZEND_COMPILE_NO_CONSTANT_SUBSTITUTION | ZEND_COMPILE_NO_PERSISTENT_CONSTANT_SUBSTITUTION;
	// 文件上下文开始
	zend_file_context_begin(&original_file_context);
	// 常量表达式转成内置变量
	zend_const_expr_to_zval(default_value_zval, const_expr_ast_ptr, /* allow_dynamic */ true);
	// 恢复 CG(ast_arena)
	CG(ast_arena) = original_ast_arena;
	// 恢复 原编译配置
	CG(compiler_options) = original_compiler_options;
	// 文件上下文结束
	zend_file_context_end(&original_file_context);
	// 销毁毫语句对象
	zend_ast_destroy(ast);
	// 销毁临时 ast_arena
	zend_arena_destroy(ast_arena);
	// 成功
	return SUCCESS;
}

// ing4, 解析字串，不可以有 \\ 和 quote，内部1次调用
static zend_string *try_parse_string(const char *str, size_t len, char quote) {
	// 返回 null , 	ZEND_API zend_string *zend_empty_string = NULL;
	if (len == 0) {
		return ZSTR_EMPTY_ALLOC();
	}

	// 检查每个字符 
	for (size_t i = 0; i < len; i++) {
		// 如果是  \ 或 quote
		if (str[i] == '\\' || str[i] == quote) {
			// 返回null
			return NULL;
		}
	}
	// 返回字串
	return zend_string_init(str, len, 0);
}

// ing3, 解析参数的默认值。p1:默认值，p2:参数信息
ZEND_API zend_result zend_get_default_from_internal_arg_info(
		zval *default_value_zval, zend_internal_arg_info *arg_info)
{
	// 参数的默认值
	const char *default_value = arg_info->default_value;
	// 如果没有默认值
	if (!default_value) {
		// 返回失败
		return FAILURE;
	}

	// 对于一些简单的实例 避免使用完整的语句解析机制
	/* Avoid going through the full AST machinery for some simple and common cases. */
	// 默认值长度
	size_t default_value_len = strlen(default_value);
	// 临时变量
	zend_ulong lval;
	// 字串 null
	if (default_value_len == sizeof("null")-1
			&& !memcmp(default_value, "null", sizeof("null")-1)) {
		// 转成 null
		ZVAL_NULL(default_value_zval);
		// 返回成功
		return SUCCESS;
	// 字串 true
	} else if (default_value_len == sizeof("true")-1
			&& !memcmp(default_value, "true", sizeof("true")-1)) {
		// 转成 true
		ZVAL_TRUE(default_value_zval);
		// 返回成功
		return SUCCESS;
	// 字串 false
	} else if (default_value_len == sizeof("false")-1
			&& !memcmp(default_value, "false", sizeof("false")-1)) {
		// 转成 false
		ZVAL_FALSE(default_value_zval);
		// 返回成功
		return SUCCESS;
	// 开头和结尾是 字串 ' 或 "，并且成对出现
	} else if (default_value_len >= 2
			&& (default_value[0] == '\'' || default_value[0] == '"')
			&& default_value[default_value_len - 1] == default_value[0]) {
		// 解析字串，去掉开头和结尾的引号
		zend_string *str = try_parse_string(
			default_value + 1, default_value_len - 2, default_value[0]);
		// 如果解析成功
		if (str) {
			// 值为解析成的字串
			ZVAL_STR(default_value_zval, str);
			// 返回成功
			return SUCCESS;
		}
	// 字串 []
	} else if (default_value_len == sizeof("[]")-1
			&& !memcmp(default_value, "[]", sizeof("[]")-1)) {
		// 转成 空数组
		ZVAL_EMPTY_ARRAY(default_value_zval);
		// 返回成功
		return SUCCESS;
		
	// 其他情况，转成整数
	} else if (ZEND_HANDLE_NUMERIC_STR(default_value, default_value_len, lval)) {
		// 如果成功，值转成整数
		ZVAL_LONG(default_value_zval, lval);
		// 返回成功
		return SUCCESS;
	}

#if 0
	fprintf(stderr, "Evaluating %s via AST\n", default_value);
#endif
	// 通过语句获取默认值
	return get_default_via_ast(default_value_zval, default_value);
}
