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
   | Authors: Christian Seiler <chris_se@gmx.net>                         |
   |          Dmitry Stogov <dmitry@php.net>                              |
   |          Marcus Boerger <helly@php.net>                              |
   +----------------------------------------------------------------------+
*/

#include "zend.h"
#include "zend_API.h"
#include "zend_closures.h"
#include "zend_exceptions.h"
#include "zend_interfaces.h"
#include "zend_objects.h"
#include "zend_objects_API.h"
#include "zend_globals.h"
#include "zend_closures_arginfo.h"

// 闭包结构体
typedef struct _zend_closure {
	// 对象
	zend_object       std;
	// 函数
	zend_function     func;
	// this 指针
	zval              this_ptr;
	// 域指针
	zend_class_entry *called_scope;
	// 源处理方法
	zif_handler       orig_internal_handler;
} zend_closure;

// 需要引用时才有静态
/* non-static since it needs to be referenced */
ZEND_API zend_class_entry *zend_ce_closure;
// 
static zend_object_handlers closure_handlers;

// ing3, 调用闭包
ZEND_METHOD(Closure, __invoke) /* {{{ */
{
	// 正在执行的函数
	zend_function *func = EX(func);
	zval *args;
	uint32_t num_args;
	HashTable *named_args;

	// 任意多个参数
	ZEND_PARSE_PARAMETERS_START(0, -1)
		// 所有参数放到字典里
		Z_PARAM_VARIADIC_WITH_NAMED(args, num_args, named_args)
	ZEND_PARSE_PARAMETERS_END();

	// 调用用户函数。p1:没用到，p2:对象，p3:函数名，p4:返回值，p5:参数数量，p6:参数列表，p7:命名参数
	if (call_user_function_named(CG(function_table), NULL, ZEND_THIS, return_value, num_args, args, named_args) == FAILURE) {
		// 如果失败，返回 false
		RETVAL_FALSE;
	}

	// 销毁函数，会在 get_method 时分配它
	/* destruct the function also, then - we have allocated it in get_method */
	// 销毁函数名
	zend_string_release_ex(func->internal_function.function_name, 0);
	// 释放函数
	efree(func);
#if ZEND_DEBUG
	execute_data->func = NULL;
#endif
}
/* }}} */

// ing3, 验证闭包是否可以绑定到此域
static bool zend_valid_closure_binding(
		zend_closure *closure, zval *newthis, zend_class_entry *scope) /* {{{ */
{
	// 获取闭包下属函数 
	zend_function *func = &closure->func;
	// 如果有 ZEND_ACC_FAKE_CLOSURE 说明是伪闭包
	bool is_fake_closure = (func->common.fn_flags & ZEND_ACC_FAKE_CLOSURE) != 0;
	// 如果有 
	if (newthis) {
		// 并且 函数有 静态标记
		if (func->common.fn_flags & ZEND_ACC_STATIC) {
			// 报错，不能给静态闭包绑定实例
			zend_error(E_WARNING, "Cannot bind an instance to a static closure");
			return 0;
		}
		// 如果是伪闭包，并且有所属类，并且给出的类是 函数所属类的子类
		if (is_fake_closure && func->common.scope &&
				!instanceof_function(Z_OBJCE_P(newthis), func->common.scope)) {
			// 不支持 绑定不兼容的this到一个外部方法
			/* Binding incompatible $this to an internal method is not supported. */
			// 报错，不可以绑定此类的此方法到目标类
			zend_error(E_WARNING, "Cannot bind method %s::%s() to object of class %s",
					ZSTR_VAL(func->common.scope->name),
					ZSTR_VAL(func->common.function_name),
					ZSTR_VAL(Z_OBJCE_P(newthis)->name));
			// 返回失败
			return 0;
		}
	// 如果是伪闭包 并且有所属域 并且 函数不带 static 标记
	} else if (is_fake_closure && func->common.scope
			&& !(func->common.fn_flags & ZEND_ACC_STATIC)) {
		// 报错，不可以给这个方法绑定 $this
		zend_error(E_WARNING, "Cannot unbind $this of method");
		return 0;
	// 如果是伪装饰 并且 有 this 并且 函数有 this 标记
	} else if (!is_fake_closure && !Z_ISUNDEF(closure->this_ptr)
			&& (func->common.fn_flags & ZEND_ACC_USES_THIS)) {
		// 报错，有$this的装饰，不可以解绑 $this
		zend_error(E_WARNING, "Cannot unbind $this of closure using $this");
		return 0;
	}

	// 如果有域 并且 域不是函数所属类 并且 域是内部类
	if (scope && scope != func->common.scope && scope->type == ZEND_INTERNAL_CLASS) {
		// 不可以重复绑定内部类
		/* rebinding to internal class is not allowed */
		// 报错：不可以绑定闭包到 此内部类
		zend_error(E_WARNING, "Cannot bind closure to scope of internal class %s",
				ZSTR_VAL(scope->name));
		return 0;
	}
	
	// 如果是伪闭包 并且 所属类不是给出的类
	if (is_fake_closure && scope != func->common.scope) {
		// 如果函数没有属性类
		if (func->common.scope == NULL) {
			// 不可以给 函数中创建的闭包 重新绑定 重新绑定域
			zend_error(E_WARNING, "Cannot rebind scope of closure created from function");
		} else {
			// 不可以给 类方法中创建的闭包 重新绑定 重新绑定域
			zend_error(E_WARNING, "Cannot rebind scope of closure created from method");
		}
		return 0;
	}

	return 1;
}
/* }}} */

// 100 行
// ing3，调用闭包，绑定到给出的对象，用它的域作为调用域
/* {{{ Call closure, binding to a given object with its class as the scope */
ZEND_METHOD(Closure, call)
{
	// 函数调用中需要用到的临时变量
	zval *newthis, closure_result;
	zend_closure *closure;
	zend_fcall_info fci;
	zend_fcall_info_cache fci_cache;
	zend_object *newobj;
	zend_class_entry *newclass;

	fci.param_count = 0;
	fci.params = NULL;
	// 接收参数，最少1个
	ZEND_PARSE_PARAMETERS_START(1, -1)
		// 第一个参数是新的 this
		Z_PARAM_OBJECT(newthis)
		// 后面是带命名的字典参数
		Z_PARAM_VARIADIC_WITH_NAMED(fci.params, fci.param_count, fci.named_params)
	ZEND_PARSE_PARAMETERS_END();

	// 把 $this 转成闭包
	// #define ZEND_THIS (&EX(This))
	closure = (zend_closure *) Z_OBJ_P(ZEND_THIS);

	// 取出 新的 this中的对象
	newobj = Z_OBJ_P(newthis);
	// 取出所属类
	newclass = newobj->ce;

	// 验证闭包是否可以绑定到此域
	if (!zend_valid_closure_binding(closure, newthis, newclass)) {
		// 不可以，中断
		return;
	}

	// 更新调用域
	fci_cache.called_scope = newclass;
	// 更新调用对象
	fci_cache.object = fci.object = newobj;

	// 调用信息大小
	fci.size = sizeof(fci);
	// 把装闭包中的 函数对象取出 ，关联到调用信息
	ZVAL_OBJ(&fci.function_name, &closure->std);
	// 清空闭包结果
	ZVAL_UNDEF(&closure_result);
	// 把结果关联到调用信息
	fci.retval = &closure_result;

	// 如果函数是 生成器
	if (closure->func.common.fn_flags & ZEND_ACC_GENERATOR) {
		zval new_closure;
		// 创建闭包（或假闭包），p1:返回创建的闭包，p2:原函数，p3:域，p4:调用过的域，p5:this指针
		zend_create_closure(&new_closure, &closure->func, newclass, closure->called_scope, newthis);
		// 从新闭包对象中取出 闭包
		closure = (zend_closure *) Z_OBJ(new_closure);
		// 更新调用信息缓存中的处理函数
		fci_cache.function_handler = &closure->func;

		// 调用闭包
		zend_call_function(&fci, &fci_cache);

		// 在创建生成器时复制过了
		/* copied upon generator creation */
		// 闭包内置对象，减少引用次数
		GC_DELREF(&closure->std);
	// 函数不是生成器
	} else {
		zend_closure *fake_closure;
		zend_function *my_function;

		// 分配内存创建假闭包
		fake_closure = emalloc(sizeof(zend_closure));
		// 假闭包的内置对象，全部置成0
		memset(&fake_closure->std, 0, sizeof(fake_closure->std));
		// 闭包内置对象 引用次数为1
		fake_closure->std.gc.refcount = 1;
		// 闭包内置对象不使用垃圾回收
		fake_closure->std.gc.u.type_info = GC_NULL;
		// 清空 $this 指针
		ZVAL_UNDEF(&fake_closure->this_ptr);
		// 清空调用过的域
		fake_closure->called_scope = NULL;
		// 取出闭包中的函数
		my_function = &fake_closure->func;
		// 如果是用户定义函数
		if (ZEND_USER_CODE(closure->func.type)) {
			// 复制操作码组
			memcpy(my_function, &closure->func, sizeof(zend_op_array));
		} else {
			// 复制内置函数
			memcpy(my_function, &closure->func, sizeof(zend_internal_function));
		}
		// 使用传入对象的域
		/* use scope of passed object */
		my_function->common.scope = newclass;
		// 如果闭包是内置函数
		if (closure->func.type == ZEND_INTERNAL_FUNCTION) {
			// 使用原内置处理器
			my_function->internal_function.handler = closure->orig_internal_handler;
		}
		// 更新调用信息缓存中的 处理器
		fci_cache.function_handler = my_function;

		// 依赖反弹域的运行时缓存必须是不可更改的，所以 如果 更新了域 需要一个独立的缓存
		/* Runtime cache relies on bound scope to be immutable, hence we need a separate rt cache in case scope changed */
		// 如果是用户定义的函数 并且 （域不是 newclass 或 有 独享缓存 标记）
		if (ZEND_USER_CODE(my_function->type)
		 && (closure->func.common.scope != newclass
		  || (closure->func.common.fn_flags & ZEND_ACC_HEAP_RT_CACHE))) {
			// 临时指针
			void *ptr;
			// 添加独享缓存标记
			my_function->op_array.fn_flags |= ZEND_ACC_HEAP_RT_CACHE;
			// 分配缓存
			ptr = emalloc(my_function->op_array.cache_size);
			// 缓存关联到函数
			ZEND_MAP_PTR_INIT(my_function->op_array.run_time_cache, ptr);
			// 缓存全部设置成0
			memset(ptr, 0, my_function->op_array.cache_size);
		}

		// 调用函数
		zend_call_function(&fci, &fci_cache);

		// 如果是用户定义函数
		if (ZEND_USER_CODE(my_function->type)) {
			// 有 独享缓存
			if (fci_cache.function_handler->common.fn_flags & ZEND_ACC_HEAP_RT_CACHE) {
				// 释放运行时缓存
				efree(ZEND_MAP_PTR(my_function->op_array.run_time_cache));
			}
		}
		// 释放假闭包
		efree_size(fake_closure, sizeof(zend_closure));
	}

	// 如果有运行结果
	if (Z_TYPE(closure_result) != IS_UNDEF) {
		// 如果结果 是引用类型
		if (Z_ISREF(closure_result)) {
			// 解引用 解包引用对象：引用的值赋给当前变量
			zend_unwrap_reference(&closure_result);
		}
		// 把结果复制给返回结果
		ZVAL_COPY_VALUE(return_value, &closure_result);
	}
}
/* }}} */

// ing3, 创建闭包副本并绑定到指定域。p1:返回的闭包，p2:原闭包，p3:域，p4:域名字串
static void do_closure_bind(zval *return_value, zval *zclosure, zval *newthis, zend_object *scope_obj, zend_string *scope_str)
{
	zend_class_entry *ce, *called_scope;
	zend_closure *closure = (zend_closure *) Z_OBJ_P(zclosure);
	// 如果有域对象
	if (scope_obj) {
		// 获取类入口
		ce = scope_obj->ce;
	// 如果有域 名
	} else if (scope_str) {
		// 如果名称是 static
		if (zend_string_equals(scope_str, ZSTR_KNOWN(ZEND_STR_STATIC))) {
			// 取得函数中的域
			ce = closure->func.common.scope;
		// 如果找不到此类
		} else if ((ce = zend_lookup_class(scope_str)) == NULL) {
			// 报错 无法找到此类
			zend_error(E_WARNING, "Class \"%s\" not found", ZSTR_VAL(scope_str));
			RETURN_NULL();
		}
	// 没有所属类
	} else {
		ce = NULL;
	}
	// 验证闭包是否可以绑定到此域
	if (!zend_valid_closure_binding(closure, newthis, ce)) {
		return;
	}
	// 如果有新this
	if (newthis) {
		// 把它作为调用过的域
		called_scope = Z_OBJCE_P(newthis);
	// 没有新 this
	} else {
		// 上面获取到的域，作为调用过的域
		called_scope = ce;
	}
	// 创建闭包（或假闭包），p1:返回创建的闭包，p2:原函数，p3:域，p4:调用过的域，p5:this指针
	zend_create_closure(return_value, &closure->func, ce, called_scope, newthis);
}

// ing3, 从一个闭包创建闭包，并绑定到另一个对象和域
/* {{{ Create a closure from another one and bind to another object and scope */
ZEND_METHOD(Closure, bind)
{
	zval *zclosure, *newthis;
	zend_object *scope_obj = NULL;
	// 域名称为 static
	zend_string *scope_str = ZSTR_KNOWN(ZEND_STR_STATIC);

	// 参数必须是2-3个
	ZEND_PARSE_PARAMETERS_START(2, 3)
		// 闭包
		Z_PARAM_OBJECT_OF_CLASS(zclosure, zend_ce_closure)
		// 新的 $this
		Z_PARAM_OBJECT_OR_NULL(newthis)
		// 后面的可选
		Z_PARAM_OPTIONAL
		// 把传入的参数转换成对象（不限所属类）或字串，允null, p1:返回值，p2:返回字串
		Z_PARAM_OBJ_OR_STR_OR_NULL(scope_obj, scope_str)
	ZEND_PARSE_PARAMETERS_END();

	// 创建闭包副本并绑定到指定域。p1:返回的闭包，p2:原闭包，p3:域，p4:域名字串
	do_closure_bind(return_value, zclosure, newthis, scope_obj, scope_str);
}

// ing3, 从一个闭包创建闭包，并绑定到另一个对象和域. 不用传入闭包
/* {{{ Create a closure from another one and bind to another object and scope */
ZEND_METHOD(Closure, bindTo)
{
	zval *newthis;
	zend_object *scope_obj = NULL;
	// 域名称为 static
	zend_string *scope_str = ZSTR_KNOWN(ZEND_STR_STATIC);

	// 参数必须是1-2个
	ZEND_PARSE_PARAMETERS_START(1, 2)
		// 新的 $this
		Z_PARAM_OBJECT_OR_NULL(newthis)
		// 后面的可选
		Z_PARAM_OPTIONAL
		// 把传入的参数转换成对象（不限所属类）或字串，允null, p1:返回值，p2:返回字串
		Z_PARAM_OBJ_OR_STR_OR_NULL(scope_obj, scope_str)
	ZEND_PARSE_PARAMETERS_END();

	// 创建闭包副本并绑定到指定域。p1:返回的闭包，p2:原闭包，p3:域，p4:域名字串
	do_closure_bind(return_value, getThis(), newthis, scope_obj, scope_str);
}

// ing3, 调用魔术方法
static ZEND_NAMED_FUNCTION(zend_closure_call_magic) /* {{{ */ {
	zend_fcall_info fci;
	zend_fcall_info_cache fcc;
	zval params[2];

	// 清空调用信息
	memset(&fci, 0, sizeof(zend_fcall_info));
	// 清空调用信息缓存
	memset(&fcc, 0, sizeof(zend_fcall_info_cache));

	// 调用信息大小
	fci.size = sizeof(zend_fcall_info);
	// 绑定 调用信息返回值
	fci.retval = return_value;

	// 调用信息缓存中的处理函数 = 当前方法是静态方法 ？ __callstatic ： __call
	fcc.function_handler = (EX(func)->internal_function.fn_flags & ZEND_ACC_STATIC) ?
		EX(func)->internal_function.scope->__callstatic : EX(func)->internal_function.scope->__call;
	// 命名参数为空
	fci.named_params = NULL;
	// 参数表
	fci.params = params;
	// 参数数量 2个
	fci.param_count = 2;
	// 第一个参数是函数名
	ZVAL_STR(&fci.params[0], EX(func)->common.function_name);
	// 如果有传入参数
	if (ZEND_NUM_ARGS()) {
		// 把参数列表放到第二个参数里
		array_init_size(&fci.params[1], ZEND_NUM_ARGS());
		// 把形式参数添加到实际参数列表（哈希表）中去。p1:形式参数数量 ，p2:实际参数列表
		zend_copy_parameters_array(ZEND_NUM_ARGS(), &fci.params[1]);
	// 没有传入参数
	} else {
		// 二个参数 是空数组
		ZVAL_EMPTY_ARRAY(&fci.params[1]);
	}

	// 调用对象 使用 $this
	fcc.object = fci.object = Z_OBJ_P(ZEND_THIS);
	// 调用过的域，从执行数据中获取
	fcc.called_scope = zend_get_called_scope(EG(current_execute_data));

	// 调用函数
	zend_call_function(&fci, &fcc);

	// 释放第二个参数
	zval_ptr_dtor(&fci.params[1]);
}
/* }}} */

// ing3, 从一个闭包创建闭包。p1:返回闭包，p2:原闭包，p3:返回错误信息
static zend_result zend_create_closure_from_callable(zval *return_value, zval *callable, char **error) /* {{{ */ {
	zend_fcall_info_cache fcc;
	zend_function *mptr;
	zval instance;
	zend_internal_function call;

	// 检查此调用在 当前指定执行数据中 是否可行
	if (!zend_is_callable_ex(callable, NULL, 0, NULL, &fcc, error)) {
		// 不可行返回失败
		return FAILURE;
	}

	// 调用信息缓存中的函数
	mptr = fcc.function_handler;
	// 如果函数是通过弹跳创建的
	if (mptr->common.fn_flags & ZEND_ACC_CALL_VIA_TRAMPOLINE) {
		// 对于 Closure::fromCallable([$closure, "__invoke"]) 返回 $closure
		/* For Closure::fromCallable([$closure, "__invoke"]) return $closure. */
		// 如果对象所属类是  zend_ce_closure 并且方法是 __invoke
		if (fcc.object && fcc.object->ce == zend_ce_closure
				&& zend_string_equals_literal(mptr->common.function_name, "__invoke")) {
			// 复制并返回调用缓存中的对象
			RETVAL_OBJ_COPY(fcc.object);
			// 如果是执行时中的弹跳函数？清空函数名：否则释放此函数
			zend_free_trampoline(mptr);
			// 返回成功
			return SUCCESS;
		}

		// 如果函数没有域
		if (!mptr->common.scope) {
			// 返回失败
			return FAILURE;
		}
		// 如果是静态方法
		if (mptr->common.fn_flags & ZEND_ACC_STATIC) {
			// 如果没有 __callstatic 方法
			if (!mptr->common.scope->__callstatic) {
				// 返回失败
				return FAILURE;
			}
		// 不是静态方法
		} else {
			// 如果没有 __call 方法
			if (!mptr->common.scope->__call) {
				// 返回失败
				return FAILURE;
			}
		}

		// 清空内置函数
		memset(&call, 0, sizeof(zend_internal_function));
		// 类型为内置函数
		call.type = ZEND_INTERNAL_FUNCTION;
		// 复制 static标记
		call.fn_flags = mptr->common.fn_flags & ZEND_ACC_STATIC;
		// 处理函数为 zend_closure_call_magic
		call.handler = zend_closure_call_magic;
		// 复制函数名
		call.function_name = mptr->common.function_name;
		// 复制调用域
		call.scope = mptr->common.scope;

		// 如果是执行时中的弹跳函数？清空函数名：否则释放此函数
		zend_free_trampoline(mptr);
		// 指向指向临时函数
		mptr = (zend_function *) &call;
	}

	// 如果调用信息缓存中有对象
	if (fcc.object) {
		// 把对象放到实例里
		ZVAL_OBJ(&instance, fcc.object);
		// 创建假闭包 p1:返回创建的闭包，p2:原函数，p3:域，p4:调用过的域，p5:this指针
		zend_create_fake_closure(return_value, mptr, mptr->common.scope, fcc.called_scope, &instance);
	// 如果没有对象
	} else {
		// 创建假闭包 p1:返回创建的闭包，p2:原函数，p3:域，p4:调用过的域，p5:this指针
		zend_create_fake_closure(return_value, mptr, mptr->common.scope, fcc.called_scope, NULL);
	}

	// 如果内置函数已经指向临时函数（通过弹跳创建）
	if (&mptr->internal_function == &call) {
		// 释放函数名
		zend_string_release(mptr->common.function_name);
	}

	// 返回成功
	return SUCCESS;
}
/* }}} */

// ing3, 从一个闭包创建另一个闭包，使用当前域
/* {{{ Create a closure from a callable using the current scope. */
ZEND_METHOD(Closure, fromCallable)
{
	zval *callable;
	char *error = NULL;

	// 必须1个参数
	ZEND_PARSE_PARAMETERS_START(1, 1)
		// 传入闭包
		Z_PARAM_ZVAL(callable)
	ZEND_PARSE_PARAMETERS_END();

	// 如果闭包是 zend_ce_closure 类的对象
	if (Z_TYPE_P(callable) == IS_OBJECT && instanceof_function(Z_OBJCE_P(callable), zend_ce_closure)) {
		// 已经有闭包了
		/* It's already a closure */
		// 返回闭包副本
		RETURN_COPY(callable);
	}

	// 从一个闭包创建闭包。p1:返回闭包，p2:原闭包，p3:返回错误信息
	if (zend_create_closure_from_callable(return_value, callable, &error) == FAILURE) {
		// 如果有错误信息
		if (error) {
			// 报错：无法从此闭包创建闭包
			zend_type_error("Failed to create closure from callable: %s", error);
			// 释放错误
			efree(error);
		// 没有错误信息
		} else {
			// 报错：无法从此闭包创建闭包
			zend_type_error("Failed to create closure from callable");
		}
	}
}
/* }}} */

// ing4, 抛错 Closure 类不可以初始化
static ZEND_COLD zend_function *zend_closure_get_constructor(zend_object *object) /* {{{ */
{
	zend_throw_error(NULL, "Instantiation of class Closure is not allowed");
	return NULL;
}
/* }}} */

// 通过对象处理API返回int类型
/* int return due to Object Handler API */
// ing3, 比较两个闭包
static int zend_closure_compare(zval *o1, zval *o2) /* {{{ */
{
	// 如果不 （都是对象 并且 有同样的 compare() 函数）调用默认比较方法。
	ZEND_COMPARE_OBJECTS_FALLBACK(o1, o2);

	// 取出 左右两个闭包
	zend_closure *lhs = (zend_closure*) Z_OBJ_P(o1);
	zend_closure *rhs = (zend_closure*) Z_OBJ_P(o2);

	// 其中有一个不是假闭包
	if (!((lhs->func.common.fn_flags & ZEND_ACC_FAKE_CLOSURE) && (rhs->func.common.fn_flags & ZEND_ACC_FAKE_CLOSURE))) {
		// 返回无法比较（只能比较假闭包）
		return ZEND_UNCOMPARABLE;
	}

	// 如果两个的this指针类型不同
	if (Z_TYPE(lhs->this_ptr) != Z_TYPE(rhs->this_ptr)) {
		// 返回无法比较
		return ZEND_UNCOMPARABLE;
	}

	// 如果 有$this但不指向同一个目标
	if (Z_TYPE(lhs->this_ptr) == IS_OBJECT && Z_OBJ(lhs->this_ptr) != Z_OBJ(rhs->this_ptr)) {
		// 返回无法比较
		return ZEND_UNCOMPARABLE;
	}

	// 如果两个闭包调用过的域不同
	if (lhs->called_scope != rhs->called_scope) {
		// 返回无法比较
		return ZEND_UNCOMPARABLE;
	}

	// 如果两个闭包函数类型不同
	if (lhs->func.type != rhs->func.type) {
		// 返回无法比较
		return ZEND_UNCOMPARABLE;
	}

	// 如果两个闭包域不同
	if (lhs->func.common.scope != rhs->func.common.scope) {
		// 返回无法比较
		return ZEND_UNCOMPARABLE;
	}

	// 如果两个闭包 函数名 不同
	if (!zend_string_equals(lhs->func.common.function_name, rhs->func.common.function_name)) {
		// 返回无法比较
		return ZEND_UNCOMPARABLE;
	}

	// 返回相等
	return 0;
}
/* }}} */

// ing3, 给闭包创建_invoke方法，p1:闭包
ZEND_API zend_function *zend_get_closure_invoke_method(zend_object *object) /* {{{ */
{
	// 闭包对象
	zend_closure *closure = (zend_closure *)object;
	// 分配内存创建函数
	zend_function *invoke = (zend_function*)emalloc(sizeof(zend_function));
	// 可保留标记：引用返回，字典参数，包含返回类型
	const uint32_t keep_flags =
		ZEND_ACC_RETURN_REFERENCE | ZEND_ACC_VARIADIC | ZEND_ACC_HAS_RETURN_TYPE;

	invoke->common = closure->func.common;
	// 返回内置函数，但 参数信息和用户函数一样（使用 zend_string* 代替 char*）
	// 这样没问题，因为 ZEND_ACC_HAS_TYPE_HINTS 标记没有设置过，对内置函数不会检验参数信息
	// 再添加一个 ZEND_ACC_USER_ARG_INFO 标记来阻止通过反射的调用
	/* We return ZEND_INTERNAL_FUNCTION, but arg_info representation is the
	 * same as for ZEND_USER_FUNCTION (uses zend_string* instead of char*).
	 * This is not a problem, because ZEND_ACC_HAS_TYPE_HINTS is never set,
	 * and we won't check arguments on internal function. We also set
	 * ZEND_ACC_USER_ARG_INFO flag to prevent invalid usage by Reflection */
	// 类型是 内置函数 
	invoke->type = ZEND_INTERNAL_FUNCTION;
	// 添加标记 public，通过处理器调用 + 可保留标记
	invoke->internal_function.fn_flags =
		ZEND_ACC_PUBLIC | ZEND_ACC_CALL_VIA_HANDLER | (closure->func.common.fn_flags & keep_flags);
	// 如果不是内置函数 或 有【使用用户参数信息】 标记
	if (closure->func.type != ZEND_INTERNAL_FUNCTION || (closure->func.common.fn_flags & ZEND_ACC_USER_ARG_INFO)) {
		// 添加 【使用用户参数信息】 标记
		invoke->internal_function.fn_flags |=
			ZEND_ACC_USER_ARG_INFO;
	}
	// 处理器 Closure___invoke 方法
	invoke->internal_function.handler = ZEND_MN(Closure___invoke);
	// 所属模块：内置模块
	invoke->internal_function.module = 0;
	// 作用域 zend_ce_closure
	invoke->internal_function.scope = zend_ce_closure;
	// 函数名 __invoke
	invoke->internal_function.function_name = ZSTR_KNOWN(ZEND_STR_MAGIC_INVOKE);
	// 返回函数
	return invoke;
}
/* }}} */

// ing3, 返回闭包实例的 函数指针（解引用）
ZEND_API const zend_function *zend_get_closure_method_def(zend_object *obj) /* {{{ */
{
	zend_closure *closure = (zend_closure *) obj;
	return &closure->func;
}
/* }}} */

// ing3, 返回闭包实例的 this指针
ZEND_API zval* zend_get_closure_this_ptr(zval *obj) /* {{{ */
{
	zend_closure *closure = (zend_closure *)Z_OBJ_P(obj);
	return &closure->this_ptr;
}
/* }}} */

// ing3, 获取闭包对象的方法，p1:闭包指针，p2:方法名，p3:key（标准方法用）
// 闭包的 __invoke 方法需要特殊处理
static zend_function *zend_closure_get_method(zend_object **object, zend_string *method, const zval *key) /* {{{ */
{
	// 如果方法名是 __invoke
	if (zend_string_equals_literal_ci(method, ZEND_INVOKE_FUNC_NAME)) {
		// 给闭包创建_invoke方法，p1:闭包
		return zend_get_closure_invoke_method(*object);
	}
	// 返回其他方法
	return zend_std_get_method(object, method, key);
}
/* }}} */

// ing3, 释放闭包用到的数据, 没有释放闭包本身
static void zend_closure_free_storage(zend_object *object) /* {{{ */
{
	// 闭包
	zend_closure *closure = (zend_closure *)object;

	// 调用标准方法销毁 闭包中的对象
	zend_object_std_dtor(&closure->std);

	// 如果是用户定义函数
	if (closure->func.type == ZEND_USER_FUNCTION) {
		// 没有假闭包的静态变量
		/* We don't own the static variables of fake closures. */
		// 如果不是假闭包
		if (!(closure->func.op_array.fn_flags & ZEND_ACC_FAKE_CLOSURE)) {
			// 销毁静态变量表
			zend_destroy_static_vars(&closure->func.op_array);
			// 清空指针
			closure->func.op_array.static_variables = NULL;
		}
		// 销毁操作码组
		destroy_op_array(&closure->func.op_array);
	// 如果是内置函数
	} else if (closure->func.type == ZEND_INTERNAL_FUNCTION) {
		// 释放函数名
		zend_string_release(closure->func.common.function_name);
	}

	// 如果有 $this
	if (Z_TYPE(closure->this_ptr) != IS_UNDEF) {
		// 销毁  $this
		zval_ptr_dtor(&closure->this_ptr);
	}
}
/* }}} */

// ing3, 创建新闭包，添加对象的标准处理方法. p1:所属类
static zend_object *zend_closure_new(zend_class_entry *class_type) /* {{{ */
{
	zend_closure *closure;
	// 分配内存创建闭包
	closure = emalloc(sizeof(zend_closure));
	// 新闭包设置成0
	memset(closure, 0, sizeof(zend_closure));

	// 初始化闭包里的对象
	zend_object_std_init(&closure->std, class_type);
	// 给对象添加处理函数
	closure->std.handlers = &closure_handlers;

	// 返回新闭包
	return (zend_object*)closure;
}
/* }}} */

// ing3, 克隆闭包：就是用闭包的函数再创建一个闭包
static zend_object *zend_closure_clone(zend_object *zobject) /* {{{ */
{
	// 闭包
	zend_closure *closure = (zend_closure *)zobject;
	// 临时变量接收返回值
	zval result;
	// 创建闭包（或假闭包），p1:返回创建的闭包，p2:原函数，p3:域，p4:调用过的域，p5:this指针
	zend_create_closure(&result, &closure->func,
		closure->func.common.scope, closure->called_scope, &closure->this_ptr);
	// 返回创建的对象
	return Z_OBJ(result);
}
/* }}} */

// ing3, 把闭包拆分返回，p1:闭包，p2:返回调用过的域，p3:返回函数指针，p4:返回$this指针，p5:没用到
static zend_result zend_closure_get_closure(zend_object *obj, zend_class_entry **ce_ptr, zend_function **fptr_ptr, zend_object **obj_ptr, bool check_only) /* {{{ */
{
	// 闭包
	zend_closure *closure = (zend_closure*)obj;

	// 闭包的函数指针
	*fptr_ptr = &closure->func;
	// 调用过的域
	*ce_ptr = closure->called_scope;

	// 如果有$this
	if (Z_TYPE(closure->this_ptr) != IS_UNDEF) {
		// 返回 $this
		*obj_ptr = Z_OBJ(closure->this_ptr);
	} else {
		// 返回 空
		*obj_ptr = NULL;
	}

	// 返回成功
	return SUCCESS;
}
/* }}} */

// 近100行
// 在对象处理API中，is_temp 是整数
/* *is_temp is int due to Object Handler API */
// ing3, 获取调试信息，包含 函数名，静态变量表，$this，和参数表 4部分信息，p1:闭包对象，p2:是否临时
static HashTable *zend_closure_get_debug_info(zend_object *object, int *is_temp) /* {{{ */
{
	// 对象转成闭包
	zend_closure *closure = (zend_closure *)object;
	// 临时变量
	zval val;
	// 这是什么用法？
	struct _zend_arg_info *arg_info = closure->func.common.arg_info;
	// 调试信息哈希表
	HashTable *debug_info;
	// 是否是用户定义函数或 访问用户参数信息
	bool zstr_args = (closure->func.type == ZEND_USER_FUNCTION) || (closure->func.common.fn_flags & ZEND_ACC_USER_ARG_INFO);

	// 默认是1
	*is_temp = 1;

	// 创建数组，8个元素
	debug_info = zend_new_array(8);

	// 更新 function 元素
	// 如果是假闭包
	if (closure->func.op_array.fn_flags & ZEND_ACC_FAKE_CLOSURE) {
		// 如果有所属域
		if (closure->func.common.scope) {
			// 取出 类名
			zend_string *class_name = closure->func.common.scope->name;
			// 取出 函数名
			zend_string *func_name = closure->func.common.function_name;
			// 拼在一起
			zend_string *combined = zend_string_concat3(
				ZSTR_VAL(class_name), ZSTR_LEN(class_name),
				"::", strlen("::"),
				ZSTR_VAL(func_name), ZSTR_LEN(func_name)
			);
			// 值放在临时变量里
			ZVAL_STR(&val, combined);
		// 没有域
		} else {
			// 直接复制函数名
			ZVAL_STR_COPY(&val, closure->func.common.function_name);
		}
		// 更新 function 元素
		zend_hash_update(debug_info, ZSTR_KNOWN(ZEND_STR_FUNCTION), &val);
	}

	// 更新static元素
	// 如果是用户函数 并且有静态变量
	if (closure->func.type == ZEND_USER_FUNCTION && closure->func.op_array.static_variables) {
		zval *var;
		zend_string *key;
		// 取得静态变量表
		HashTable *static_variables = ZEND_MAP_PTR_GET(closure->func.op_array.static_variables_ptr);

		// 临时变量初始化成哈希表
		array_init(&val);

		// 遍历静态变量
		ZEND_HASH_MAP_FOREACH_STR_KEY_VAL(static_variables, key, var) {
			// 静态变量副本
			zval copy;
			// 如果是 表达式常量
			if (Z_TYPE_P(var) == IS_CONSTANT_AST) {
				// 转成字串 <constant ast>
				ZVAL_STRING(&copy, "<constant ast>");
			// 否则
			} else {
				// 如果是引用类型 并且引用数是1
				if (Z_ISREF_P(var) && Z_REFCOUNT_P(var) == 1) {
					// 解引用
					var = Z_REFVAL_P(var);
				}
				// 复制静态变量的值
				ZVAL_COPY(&copy, var);
			}
			// 把静态变量转存到 val里
			zend_hash_add_new(Z_ARRVAL(val), key, &copy);
		} ZEND_HASH_FOREACH_END();

		// 如果 val 里有存入的静态变量
		if (zend_hash_num_elements(Z_ARRVAL(val))) {
			// 更新 static 元素
			zend_hash_update(debug_info, ZSTR_KNOWN(ZEND_STR_STATIC), &val);
		// 否则
		} else {
			// 直接销毁 val
			zval_ptr_dtor(&val);
		}
	}

	// 更新 this 元素
	// 如果有 $this 指针
	if (Z_TYPE(closure->this_ptr) != IS_UNDEF) {
		// 增加引用次数
		Z_ADDREF(closure->this_ptr);
		// 更新 this 元素
		zend_hash_update(debug_info, ZSTR_KNOWN(ZEND_STR_THIS), &closure->this_ptr);
	}

	// 更新 parameter 元素
	// 如果有参数信息 并且 （参数数大于0 或 有字典参数）
	if (arg_info &&
		(closure->func.common.num_args ||
		 (closure->func.common.fn_flags & ZEND_ACC_VARIADIC))) {
		// 必须参数数量 
		uint32_t i, num_args, required = closure->func.common.required_num_args;
		// 临时变量初始化成哈希表
		array_init(&val);

		// 参数数量
		num_args = closure->func.common.num_args;
		// 如果有字典参数
		if (closure->func.common.fn_flags & ZEND_ACC_VARIADIC) {
			// 数量 +1
			num_args++;
		}
		// 遍历所有参数
		for (i = 0; i < num_args; i++) {
			zend_string *name;
			zval info;
			// 所有参数要有参数名
			ZEND_ASSERT(arg_info->name && "Argument should have name");
			// 如果是用户定义函数 ，或访问用户参数信息
			if (zstr_args) {
				// 参数名，如果引用类型前面加 &
				name = zend_strpprintf(0, "%s$%s",
						ZEND_ARG_SEND_MODE(arg_info) ? "&" : "",
						ZSTR_VAL(arg_info->name));
			// 其他情况
			} else {
				// 内置参数名，如果引用类型前面加 &
				name = zend_strpprintf(0, "%s$%s",
						ZEND_ARG_SEND_MODE(arg_info) ? "&" : "",
						((zend_internal_arg_info*)arg_info)->name);
			}
			// 必选参数以外是可选参数
			ZVAL_NEW_STR(&info, zend_strpprintf(0, "%s", i >= required ? "<optional>" : "<required>"));
			// 更新参数列表
			zend_hash_update(Z_ARRVAL(val), name, &info);
			// 释放函数名
			zend_string_release_ex(name, 0);
			// 下一个参数信息
			arg_info++;
		}
		// 更新 parameter 元素
		zend_hash_str_update(debug_info, "parameter", sizeof("parameter")-1, &val);
	}

	// 返回调试信息
	return debug_info;
}
/* }}} */

// ing3, 取得静态变量指针列表（直接返回），以及 this 和 this 的类型（引用返回）
static HashTable *zend_closure_get_gc(zend_object *obj, zval **table, int *n) /* {{{ */
{
	// 闭包对象
	zend_closure *closure = (zend_closure *)obj;
	// 如果有this,获取this
	*table = Z_TYPE(closure->this_ptr) != IS_NULL ? &closure->this_ptr : NULL;
	// 写入类型
	*n = Z_TYPE(closure->this_ptr) != IS_NULL ? 1 : 0;
	// 
	// 伪闭包没有静态变量
	/* Fake closures don't own the static variables they reference. */
	
	// 如果是用户函数 并且 不是伪闭包 ？ 返回操作码 的 静态变量指针列表 ：返回空
	return (closure->func.type == ZEND_USER_FUNCTION
			&& !(closure->func.op_array.fn_flags & ZEND_ACC_FAKE_CLOSURE)) ?
		ZEND_MAP_PTR_GET(closure->func.op_array.static_variables_ptr) : NULL;
}
/* }}} */

// ing3, 私有的构造方法，防止实例化
/* {{{ Private constructor preventing instantiation */
ZEND_COLD ZEND_METHOD(Closure, __construct)
{
	// 报错：Closure 类不可以实例化
	zend_throw_error(NULL, "Instantiation of class Closure is not allowed");
}
/* }}} */

// ing3, 注册闭包类
void zend_register_closure_ce(void) /* {{{ */
{
	// 注册类 Closure
	zend_ce_closure = register_class_Closure();
	// 创建对象方法
	zend_ce_closure->create_object = zend_closure_new;
	// 使用标准对象方法
	memcpy(&closure_handlers, &std_object_handlers, sizeof(zend_object_handlers));
	// 释放对象
	closure_handlers.free_obj = zend_closure_free_storage;
	// 获取构造器
	closure_handlers.get_constructor = zend_closure_get_constructor;
	// 获取方法
	closure_handlers.get_method = zend_closure_get_method;
	// 比较
	closure_handlers.compare = zend_closure_compare;
	// 克隆
	closure_handlers.clone_obj = zend_closure_clone;
	// 调试信息
	closure_handlers.get_debug_info = zend_closure_get_debug_info;
	// 获取闭包
	closure_handlers.get_closure = zend_closure_get_closure;
	// 获取gc对象
	closure_handlers.get_gc = zend_closure_get_gc;
}
/* }}} */

// ing3, 定义函数 zend_closure_internal_handler, 作为备用的闭包内置函数处理器
// #define ZEND_NAMED_FUNCTION(name) void ZEND_FASTCALL name(INTERNAL_FUNCTION_PARAMETERS)
static ZEND_NAMED_FUNCTION(zend_closure_internal_handler) /* {{{ */
{
	// 从 _zend_closure.zend_function 找到 _zend_closure 开头
	zend_closure *closure = (zend_closure*)ZEND_CLOSURE_OBJECT(EX(func));
	// 调用原处理函数
	// #define INTERNAL_FUNCTION_PARAM_PASSTHRU execute_data, return_value
	closure->orig_internal_handler(INTERNAL_FUNCTION_PARAM_PASSTHRU);
	// 赋值到 EX(this)，它在观察器检查等操作后会被释放
	// Assign to EX(this) so that it is released after observer checks etc.
	
	// Z_TYPE_INFO((p1)->This) |= p2
	ZEND_ADD_CALL_FLAG(execute_data, ZEND_CALL_RELEASE_THIS);
	// 用闭包的对象 当作 EX(This)
	Z_OBJ(EX(This)) = &closure->std;
}
/* }}} */

// 近100行
// ing3, 创建闭包，p1:返回创建的闭包，p2:原函数，p3:域，p4:调用过的域，p5:this指针，p6:是否假闭包
static void zend_create_closure_ex(zval *res, zend_function *func, zend_class_entry *scope, zend_class_entry *called_scope, zval *this_ptr, bool is_fake) /* {{{ */
{
	zend_closure *closure;
	void *ptr;
	// 创建闭包实例
	object_init_ex(res, zend_ce_closure);
	// 取得实例指针
	closure = (zend_closure *)Z_OBJ_P(res);
	// 如果没有作用域 但有 this 指针，并且 它有效
	if ((scope == NULL) && this_ptr && (Z_TYPE_P(this_ptr) != IS_UNDEF)) {
		/* use dummy scope if we're binding an object without specifying a scope */
		/* maybe it would be better to create one for this purpose */
		// 使用 zend_ce_closure 为作用域
		scope = zend_ce_closure;
	}

	// 如果函数是用户定义的
	if (func->type == ZEND_USER_FUNCTION) {
		// 复制操作码组
		memcpy(&closure->func, func, sizeof(zend_op_array));
		// 添加闭包标记
		closure->func.common.fn_flags |= ZEND_ACC_CLOSURE;
		// 删除 不可更改标记
		closure->func.common.fn_flags &= ~ZEND_ACC_IMMUTABLE;
		
		// 对于假闭包，需要利用源方法的静态变量
		/* For fake closures, we want to reuse the static variables of the original function. */
		// 如果不是假闭包
		if (!is_fake) {
			// 如果有静态变量表
			if (closure->func.op_array.static_variables) {
				// 复制静态变量表
				closure->func.op_array.static_variables =
					zend_array_dup(closure->func.op_array.static_variables);
			}
			// 初始化指针 static_variables_ptr 指向自己的列表
			ZEND_MAP_PTR_INIT(closure->func.op_array.static_variables_ptr,
				closure->func.op_array.static_variables);
		// 假闭包并且有静态变量表
		} else if (func->op_array.static_variables) {
			// 取得静态变量表指针
			HashTable *ht = ZEND_MAP_PTR_GET(func->op_array.static_variables_ptr);
			// 如果指针无效
			if (!ht) {
				// 复制静态变量表
				ht = zend_array_dup(func->op_array.static_variables);
				// 更新指针
				ZEND_MAP_PTR_SET(func->op_array.static_variables_ptr, ht);
			}
			// 更新闭包的 静态变量表指针
			ZEND_MAP_PTR_INIT(closure->func.op_array.static_variables_ptr, ht);
		}

		// 运行时缓存依赖域，如果域改变了，它就不能复用了
		/* Runtime cache is scope-dependent, so we cannot reuse it if the scope changed */
		// 取得运行时缓存 
		ptr = ZEND_MAP_PTR_GET(func->op_array.run_time_cache);
		// 如果不存在 或 函数不属于当前域 或 函数有 ZEND_ACC_HEAP_RT_CACHE 标记
		if (!ptr
			|| func->common.scope != scope
			|| (func->common.fn_flags & ZEND_ACC_HEAP_RT_CACHE)
		) {
			// 如果缓存不存在 并且 函数是闭包 并且 （函数属于当前域 或 没有 不可更改标记）
			if (!ptr
			 && (func->common.fn_flags & ZEND_ACC_CLOSURE)
			 && (func->common.scope == scope ||
			     !(func->common.fn_flags & ZEND_ACC_IMMUTABLE))) {
				// 真闭包第一次使用时，创建一个共享的运行时缓存，并记录它所属的域
				/* If a real closure is used for the first time, we create a shared runtime cache
				 * and remember which scope it is for. */
				// 如果函数没有域
				if (func->common.scope != scope) {
					// 使用指定的域
					func->common.scope = scope;
				}
				// 删除标记 ZEND_ACC_HEAP_RT_CACHE
				closure->func.op_array.fn_flags &= ~ZEND_ACC_HEAP_RT_CACHE;
				// 分配内存创建缓存
				ptr = zend_arena_alloc(&CG(arena), func->op_array.cache_size);
				// 把新创建的缓存关联到函数
				ZEND_MAP_PTR_SET(func->op_array.run_time_cache, ptr);
			// 其他情况
			} else {
				// 其他情况，使用非共享运行时缓存
				/* Otherwise, we use a non-shared runtime cache */
				// 添加标记 ZEND_ACC_HEAP_RT_CACHE
				closure->func.op_array.fn_flags |= ZEND_ACC_HEAP_RT_CACHE;
				// 分配内存创建缓存
				ptr = emalloc(func->op_array.cache_size);
			}
			// 新缓存全部写成0
			memset(ptr, 0, func->op_array.cache_size);
		}
		// 新缓存关联到闭包函数
		ZEND_MAP_PTR_INIT(closure->func.op_array.run_time_cache, ptr);

		// 函数名添加引用次数
		zend_string_addref(closure->func.op_array.function_name);
		// 如果函数操作码有引用次数
		if (closure->func.op_array.refcount) {
			// 增加引用次数
			(*closure->func.op_array.refcount)++;
		}
	// 如果函数不是用户定义的
	} else {
		// 复制内置函数
		memcpy(&closure->func, func, sizeof(zend_internal_function));
		// 添加闭包标记
		closure->func.common.fn_flags |= ZEND_ACC_CLOSURE;
		// 包装内置函数处理器，来避免内存泄露
		/* wrap internal function handler to avoid memory leak */
		// 如果闭包函数的处理器是 zend_closure_internal_handler
		if (UNEXPECTED(closure->func.internal_function.handler == zend_closure_internal_handler)) {
			//避免无限循环，使用嵌套闭包中的方法
			/* avoid infinity recursion, by taking handler from nested closure */
			// 取得嵌套闭包
			zend_closure *nested = (zend_closure*)((char*)func - XtOffsetOf(zend_closure, func));
			// 它必须是标准备类型
			ZEND_ASSERT(nested->std.ce == zend_ce_closure);
			// 使用它的原内置处理函数
			closure->orig_internal_handler = nested->orig_internal_handler;
		// 其他处理器
		} else {
			// 更新内置函处理器指针
			closure->orig_internal_handler = closure->func.internal_function.handler;
		}
		// 使用 zend_closure_internal_handler 
		closure->func.internal_function.handler = zend_closure_internal_handler;
		// 函数名增加引用次数
		zend_string_addref(closure->func.op_array.function_name);
		// 如果方法没有所属域
		if (!func->common.scope) {
			// 如果是个自由函数，不需要时 不设置 域和 $this
			/* if it's a free function, we won't set scope & this since they're meaningless */
			// $this 指针为空
			this_ptr = NULL;
			// 域为空
			scope = NULL;
		}
	}

	// this 指针清空
	ZVAL_UNDEF(&closure->this_ptr);
	/* Invariant:
	 * If the closure is unscoped or static, it has no bound object. */
	// 更新闭包作用域
	closure->func.common.scope = scope;
	
	// 复制调用过的域
	closure->called_scope = called_scope;
	// 如果作用域有效
	if (scope) {
		// 有public标记
		closure->func.common.fn_flags |= ZEND_ACC_PUBLIC;
		// this 有效并且是 对象 并且 闭包没有 static 标记
		if (this_ptr && Z_TYPE_P(this_ptr) == IS_OBJECT && (closure->func.common.fn_flags & ZEND_ACC_STATIC) == 0) {
			// 把 this 对象 复制给 闭包
			ZVAL_OBJ_COPY(&closure->this_ptr, Z_OBJ_P(this_ptr));
		}
	}
}
/* }}} */

// ing3, 创建闭包（或假闭包），p1:返回创建的闭包，p2:原函数，p3:域，p4:调用过的域，p5:this指针
ZEND_API void zend_create_closure(zval *res, zend_function *func, zend_class_entry *scope, zend_class_entry *called_scope, zval *this_ptr)
{
	// 创建闭包，p1:返回创建的闭包，p2:原函数，p3:域，p4:调用过的域，p5:this指针，p6:是否假闭包
	zend_create_closure_ex(res, func, scope, called_scope, this_ptr,
		/* is_fake */ (func->common.fn_flags & ZEND_ACC_FAKE_CLOSURE) != 0);
}

// ing3, 创建假闭包 p1:返回创建的闭包，p2:原函数，p3:域，p4:调用过的域，p5:this指针
ZEND_API void zend_create_fake_closure(zval *res, zend_function *func, zend_class_entry *scope, zend_class_entry *called_scope, zval *this_ptr) /* {{{ */
{
	zend_closure *closure;

	// 创建假闭包，p1:返回创建的闭包，p2:原函数，p3:域，p4:调用过的域，p5:this指针
	zend_create_closure_ex(res, func, scope, called_scope, this_ptr, /* is_fake */ true);

	// 创建的闭包
	closure = (zend_closure *)Z_OBJ_P(res);
	// 添加假闭包标记
	closure->func.common.fn_flags |= ZEND_ACC_FAKE_CLOSURE;
}
/* }}} */

// ing3, 从执行数据创建假闭包，p1:返回值，p2:执行数据
void zend_closure_from_frame(zval *return_value, zend_execute_data *call) { /* {{{ */
	zval instance;
	zend_internal_function trampoline;
	// 执行数据中的参数
	zend_function *mptr = call->func;

	// 如果执行数据本身是闭包
	if (ZEND_CALL_INFO(call) & ZEND_CALL_CLOSURE) {
		// 直接返回此闭包对象
		// 从 _zend_closure.zend_function 找到 _zend_closure 开头
		RETURN_OBJ(ZEND_CLOSURE_OBJECT(mptr));
	}

	// 如果函数是通过弹跳创建的
	if (mptr->common.fn_flags & ZEND_ACC_CALL_VIA_TRAMPOLINE) {
		// 如果 有$this 并且 $this是闭包 并且函数名是 __invoke
		if ((ZEND_CALL_INFO(call) & ZEND_CALL_HAS_THIS) &&
			(Z_OBJCE(call->This) == zend_ce_closure)
			&& zend_string_equals_literal(mptr->common.function_name, "__invoke")) {
			// 如果是执行时中的弹跳函数？清空函数名：否则释放此函数
	        zend_free_trampoline(mptr);
			// 返回 $this
	        RETURN_OBJ_COPY(Z_OBJ(call->This));
	    }

		// 清空弹跳函数
		memset(&trampoline, 0, sizeof(zend_internal_function));
		// 复制类型
		trampoline.type = ZEND_INTERNAL_FUNCTION;
		// 复制 static标记
		trampoline.fn_flags = mptr->common.fn_flags & ZEND_ACC_STATIC;
		// 绑定处理器
		trampoline.handler = zend_closure_call_magic;
		// 复制函数名
		trampoline.function_name = mptr->common.function_name;
		// 复制域
		trampoline.scope = mptr->common.scope;
		// 如果是执行时中的弹跳函数？清空函数名：否则释放此函数
		zend_free_trampoline(mptr);
		// 使用此跳弹函数
		mptr = (zend_function *) &trampoline;
	}

	// 如果 有 $this
	if (ZEND_CALL_INFO(call) & ZEND_CALL_HAS_THIS) {
		// 使用 执行数据中的 $this 作为实例
		ZVAL_OBJ(&instance, Z_OBJ(call->This));
		// 创建假闭包 p1:返回创建的闭包，p2:原函数，p3:域，p4:调用过的域，p5:this指针
		zend_create_fake_closure(return_value, mptr, mptr->common.scope, Z_OBJCE(instance), &instance);
	} else {
		// 创建假闭包 p1:返回创建的闭包，p2:原函数，p3:域，p4:调用过的域，p5:this指针
		zend_create_fake_closure(return_value, mptr, mptr->common.scope, Z_CE(call->This), NULL);
	}

	// 如果处理函数是弹跳函数
	if (&mptr->internal_function == &trampoline) {
		// 释放函数名
		zend_string_release(mptr->common.function_name);
	}
} /* }}} */

// ing3, 通过名称绑定外部变量（更新）。p1:闭包，p2:变量名，p3:新值
void zend_closure_bind_var(zval *closure_zv, zend_string *var_name, zval *var) /* {{{ */
{
	// 
	zend_closure *closure = (zend_closure *) Z_OBJ_P(closure_zv);
	// 通过操作码取得静态变量 哈希表指针
	HashTable *static_variables = ZEND_MAP_PTR_GET(closure->func.op_array.static_variables_ptr);
	// 更新哈希表，把外部zval写进去
	zend_hash_update(static_variables, var_name, var);
}
/* }}} */

// ing3, 通过偏移量绑定外部变量（更新）。p1:闭包，p2:偏移量，p3:新值
void zend_closure_bind_var_ex(zval *closure_zv, uint32_t offset, zval *val) /* {{{ */
{
	zend_closure *closure = (zend_closure *) Z_OBJ_P(closure_zv);
	// 通过操作码取得静态变量 哈希表指针
	HashTable *static_variables = ZEND_MAP_PTR_GET(closure->func.op_array.static_variables_ptr);
	// 取得数组中的 zval
	zval *var = (zval*)((char*)static_variables->arData + offset);
	// 清空它
	zval_ptr_dtor(var);
	// 新值创建副本并绑定到原位置
	ZVAL_COPY_VALUE(var, val);
}
/* }}} */
