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
   | Authors: Marcus Boerger <helly@php.net>                              |
   +----------------------------------------------------------------------+
*/

#include "zend.h"
#include "zend_API.h"
#include "zend_interfaces.h"
#include "zend_exceptions.h"
#include "zend_interfaces_arginfo.h"

ZEND_API zend_class_entry *zend_ce_traversable;
ZEND_API zend_class_entry *zend_ce_aggregate;
ZEND_API zend_class_entry *zend_ce_iterator;
ZEND_API zend_class_entry *zend_ce_arrayaccess;
ZEND_API zend_class_entry *zend_ce_serializable;
ZEND_API zend_class_entry *zend_ce_countable;
ZEND_API zend_class_entry *zend_ce_stringable;
ZEND_API zend_class_entry *zend_ce_internal_iterator;

static zend_object_handlers zend_internal_iterator_handlers;

// 如果 retval_ptr 不是空 zend_call_method 只返回返回的 zval
/* {{{ zend_call_method
 Only returns the returned zval if retval_ptr != NULL */
// ing4, 调用指定的参数或方法
// 9个参数。 对象，对象所属类，调用方法，方法名，方法名长度，返回值指针，参数数量，参数1，参数2
ZEND_API zval* zend_call_method(zend_object *object, zend_class_entry *obj_ce, zend_function **fn_proxy, const char *function_name, size_t function_name_len, zval *retval_ptr, uint32_t param_count, zval* arg1, zval* arg2)
{
	zend_function *fn;
	zend_class_entry *called_scope;
	zval params[2];
	// 如果有参数数量 
	if (param_count > 0) {
		// 复制第一个参数
		ZVAL_COPY_VALUE(&params[0], arg1);
	}
	// 如果参数大于1个
	if (param_count > 1) {
		// 复制第二个参数
		ZVAL_COPY_VALUE(&params[1], arg2);
	}
	// 如果没有传入类，使用对象的类
	if (!obj_ce) {
		obj_ce = object ? object->ce : NULL;
	}
	// 代理函数 或 它的指向目标不存在
	if (!fn_proxy || !*fn_proxy) {
		// 如果有所属类
		if (EXPECTED(obj_ce)) {
			// 从类的方法表里 用方法名，取回方法
			fn = zend_hash_str_find_ptr_lc(
				&obj_ce->function_table, function_name, function_name_len);
			// 如果没取到方法
			if (UNEXPECTED(fn == NULL)) {
				// c语言层面错误：找不到指定的方法
				/* error at c-level */
				zend_error_noreturn(E_CORE_ERROR, "Couldn't find implementation for method %s::%s", ZSTR_VAL(obj_ce->name), function_name);
			}
		// 如果没有所属类
		} else {
			// 通过函数名获取函数
			fn = zend_fetch_function_str(function_name, function_name_len);
			// 如果找不到
			if (UNEXPECTED(fn == NULL)) {
				// c语言层面错误：找不到指定的函数
				/* error at c-level */
				zend_error_noreturn(E_CORE_ERROR, "Couldn't find implementation for function %s", function_name);
			}
		}
		// 如果有代理函数指针
		if (fn_proxy) {
			// 找到的函数当成代理函数
			*fn_proxy = fn;
		}
	// 如果已经有了代理函数
	} else {
		// 追踪到函数指针
		fn = *fn_proxy;
	}
	// 如果有对象
	if (object) {
		// 使用对象的所属类为作用域
		called_scope = object->ce;
	// 否则
	} else {
		// 使用传入的作用域
		called_scope = obj_ce;
	}
	// 调用找到的方法。传入：函数指针，对象，对象作用域，反回参数指针，参数数量 ，参数列表。
	// 最后一个参数是命名参数列表
	zend_call_known_function(fn, object, called_scope, retval_ptr, param_count, params, NULL);
	// 返回对象指针
	return retval_ptr;
}
/* }}} */

/* iterator interface, c-level functions used by engine */

/* {{{ zend_user_it_new_iterator */
// ing3, 调用 new_iterator方法 创建迭代器并返回
ZEND_API void zend_user_it_new_iterator(zend_class_entry *ce, zval *object, zval *retval)
{
	zend_call_known_instance_method_with_0_params(
		ce->iterator_funcs_ptr->zf_new_iterator, Z_OBJ_P(object), retval);
}
/* }}} */

/* {{{ zend_user_it_invalidate_current */
// ing4, 销毁 迭代器的 value 元素 （zval）
ZEND_API void zend_user_it_invalidate_current(zend_object_iterator *_iter)
{
	// 指针转换成迭代器
	zend_user_iterator *iter = (zend_user_iterator*)_iter;
	// 如果当前值有效
	if (!Z_ISUNDEF(iter->value)) {
		// 销毁当前值（zval）
		zval_ptr_dtor(&iter->value);
		// 当前值为 未定义
		ZVAL_UNDEF(&iter->value);
	}
}
/* }}} */

/* {{{ zend_user_it_dtor */
// ing4, 销毁当前指向的元素
static void zend_user_it_dtor(zend_object_iterator *_iter)
{
	// 指针转成用户迭代器
	zend_user_iterator *iter = (zend_user_iterator*)_iter;
	// 找到zend迭代器中的数据
	zval *object = &iter->it.data;
	// 销毁 迭代器的 value 元素 （zval）
	zend_user_it_invalidate_current(_iter);
	// 销毁 zend迭代器中的数据
	zval_ptr_dtor(object);
}
/* }}} */


/* {{{ zend_user_it_valid */
// ing3, 迭代器的 valid 方法
ZEND_API int zend_user_it_valid(zend_object_iterator *_iter)
{
	// 如果迭代器存在
	if (_iter) {
		// 指针转成用户迭代器
		zend_user_iterator *iter = (zend_user_iterator*)_iter;
		// 找到zend迭代器中的数据
		zval *object = &iter->it.data;
		zval more;
		// 调用类的迭代方法列表-》zf_valid，返回值放在more里
		zend_call_known_instance_method_with_0_params(iter->ce->iterator_funcs_ptr->zf_valid, Z_OBJ_P(object), &more);
		// 返回值转换成 true, false
		bool result = i_zend_is_true(&more);
		// 销毁返回值
		zval_ptr_dtor(&more);
		// 返回 是否有效
		return result ? SUCCESS : FAILURE;
	}
	return FAILURE;
}
/* }}} */

/* {{{ zend_user_it_get_current_data */
// ing4, current函数， 获取当前指向元素的value。
ZEND_API zval *zend_user_it_get_current_data(zend_object_iterator *_iter)
{
	//
	zend_user_iterator *iter = (zend_user_iterator*)_iter;
	// zend迭代器中的zval
	zval *object = &iter->it.data;
	// 如果 value 未定义
	if (Z_ISUNDEF(iter->value)) {
		// 调用current方法
		zend_call_known_instance_method_with_0_params(iter->ce->iterator_funcs_ptr->zf_current, Z_OBJ_P(object), &iter->value);
	}
	// 返回 value
	return &iter->value;
}
/* }}} */

/* {{{ zend_user_it_get_current_key */
// ing4, key函数， 获取当前指向元素的key ，带引用追踪。
ZEND_API void zend_user_it_get_current_key(zend_object_iterator *_iter, zval *key)
{
	//
	zend_user_iterator *iter = (zend_user_iterator*)_iter;
	// zend 迭代器中的 zval
	zval *object = &iter->it.data;
	// 调用 key 函数，返回值是key
	zend_call_known_instance_method_with_0_params(iter->ce->iterator_funcs_ptr->zf_key, Z_OBJ_P(object), key);
	// 如果key是引用类型
	if (UNEXPECTED(Z_ISREF_P(key))) {
		// 追踪到引用目标
		zend_unwrap_reference(key);
	}
}
/* }}} */

/* {{{ zend_user_it_move_forward */
// ing3, next 方法
ZEND_API void zend_user_it_move_forward(zend_object_iterator *_iter)
{
	// 
	zend_user_iterator *iter = (zend_user_iterator*)_iter;
	// zend 迭代器中的 zval
	zval *object = &iter->it.data;
	// 销毁 迭代器的 value 元素 （zval）
	zend_user_it_invalidate_current(_iter);
	// 调用 迭代器的 zf_next 方法，无返回值
	zend_call_known_instance_method_with_0_params(iter->ce->iterator_funcs_ptr->zf_next, Z_OBJ_P(object), NULL);
}
/* }}} */

/* {{{ zend_user_it_rewind */
// ing3, 复位用户迭代器
ZEND_API void zend_user_it_rewind(zend_object_iterator *_iter)
{
	// 迭代器指针
	zend_user_iterator *iter = (zend_user_iterator*)_iter;
	// &iter->it.data 和 &iter->value 是怎么分工的，两个都是 zval
	// zend 迭代器里的数据
	zval *object = &iter->it.data;
	// 销毁 迭代器的 value 元素 （zval）
	zend_user_it_invalidate_current(_iter);
	// 调用 zf_rewind，无返回值
	zend_call_known_instance_method_with_0_params(iter->ce->iterator_funcs_ptr->zf_rewind, Z_OBJ_P(object), NULL);
}
/* }}} */

// ing3, 把当前指向元素添加进 EG(get_gc_buffer) 指针列表并返回顺序号
// 固定返回 NULL
ZEND_API HashTable *zend_user_it_get_gc(zend_object_iterator *_iter, zval **table, int *n)
{
	//
	zend_user_iterator *iter = (zend_user_iterator*)_iter;
	// 如果没有当前指向的元素
	if (Z_ISUNDEF(iter->value)) {
		// 取得zend迭代器中的 zval
		*table = &iter->it.data;
		// 序号为1
		*n = 1;
	} else {
		// 返回指针列表 EG(get_gc_buffer)
		zend_get_gc_buffer *gc_buffer = zend_get_gc_buffer_create();
		// 把 zend迭代器的 zval 添加进去
		zend_get_gc_buffer_add_zval(gc_buffer, &iter->it.data);
		// 用户迭代器的 zval 添加进去
		zend_get_gc_buffer_add_zval(gc_buffer, &iter->value);
		// 返回 zend_get_gc_buffer 的开始位置和 游标偏移量
		zend_get_gc_buffer_use(gc_buffer, table, n);
	}
	return NULL;
}

// ing3, zend迭代器用的方法列表
static const zend_object_iterator_funcs zend_interface_iterator_funcs_iterator = {
	// 销毁迭代器
	zend_user_it_dtor,
	// 验证有效
	zend_user_it_valid,
	// current
	zend_user_it_get_current_data,
	// key
	zend_user_it_get_current_key,
	// next
	zend_user_it_move_forward,
	// 重置
	zend_user_it_rewind,
	// 销毁 迭代器的 value 元素 （zval）
	zend_user_it_invalidate_current,
	// 把当前指向元素添加进 EG(get_gc_buffer) 指针列表并返回顺序号
	zend_user_it_get_gc,
};

/* {{{ zend_user_it_get_iterator */
/* by_ref is int due to Iterator API */
// by_ref 是 Iterator API 返回的int类型
// ing3, 给对象创建用户迭代器. 一开始没有指向的元素。
static zend_object_iterator *zend_user_it_get_iterator(zend_class_entry *ce, zval *object, int by_ref)
{
	zend_user_iterator *iterator;
	// 如果是引用传递
	if (by_ref) {
		// 迭代器不可以通过引用 迭代
		zend_throw_error(NULL, "An iterator cannot be used with foreach by reference");
		return NULL;
	}
	// 创建用户迭代器
	iterator = emalloc(sizeof(zend_user_iterator));
	// 初始化用户迭代器
	zend_iterator_init((zend_object_iterator*)iterator);
	// 把目标对象 copy 到 zend迭代器里
	ZVAL_OBJ_COPY(&iterator->it.data, Z_OBJ_P(object));
	// 给zend迭代器添加方法列表 zend_interface_iterator_funcs_iterator
	iterator->it.funcs = &zend_interface_iterator_funcs_iterator;
	// 通过指针访问 zval中zend_object 的 类入口
	iterator->ce = Z_OBJCE_P(object);
	// 迭代器的 zval 设置为未定义
	ZVAL_UNDEF(&iterator->value);
	// 返回迭代器
	return (zend_object_iterator*)iterator;
}
/* }}} */

/* {{{ zend_user_it_get_new_iterator */
/* by_ref is int due to Iterator API */
// ing3, getIterator 方法创建迭代器。
ZEND_API zend_object_iterator *zend_user_it_get_new_iterator(zend_class_entry *ce, zval *object, int by_ref)
{
	zval iterator;
	zend_object_iterator *new_iterator;
	zend_class_entry *ce_it;
	// 调用 new_iterator方法 创建迭代器并返回
	zend_user_it_new_iterator(ce, object, &iterator);
	// 如果创建的东东是对象，获取zval对象指针，否则 null
	ce_it = (Z_TYPE(iterator) == IS_OBJECT) ? Z_OBJCE(iterator) : NULL;
	// 如果没有创建有效迭代器 或 没有创建迭代器方法 
	// 或 创建方法是 zend_user_it_get_new_iterator  并且 迭代器和对象是同一个东东 ？
	if (!ce_it || !ce_it->get_iterator || (ce_it->get_iterator == zend_user_it_get_new_iterator && Z_OBJ(iterator) == Z_OBJ_P(object))) {
		// 如果还没有异常
		if (!EG(exception)) {
			// 抛异常：getIterator() 方法 返回的对象必须 实现 traversable 或 Iterator 接口
			zend_throw_exception_ex(NULL, 0, "Objects returned by %s::getIterator() must be traversable or implement interface Iterator", ce ? ZSTR_VAL(ce->name) : ZSTR_VAL(Z_OBJCE_P(object)->name));
		}
		// 销毁迭代器
		zval_ptr_dtor(&iterator);
		// 返回null
		return NULL;
	}
	// 获取迭代器 
	new_iterator = ce_it->get_iterator(ce_it, &iterator, by_ref);
	// 销毁蹭对象
	zval_ptr_dtor(&iterator);
	// 返回迭代器
	return new_iterator;
}
/* }}} */

/* {{{ zend_implement_traversable */
// ing3, 实现traversable接口
static int zend_implement_traversable(zend_class_entry *interface, zend_class_entry *class_type)
{
	// 只有抽象类能实现Traversable接口，它的子类必须实现 Iterator 或 IteratorAggregate 接口
	/* Abstract class can implement Traversable only, in which case the extending class must
	 * implement Iterator or IteratorAggregate. */
	// 如果是抽象类，返回成功
	if (class_type->ce_flags & ZEND_ACC_EXPLICIT_ABSTRACT_CLASS) {
		return SUCCESS;
	}
	// 非抽象类。检验类是否实现 IteratorAggregate 或 Iterator
	/* Check that class_type implements at least one of 'IteratorAggregate' or 'Iterator' */
	// 如果有实现接口
	if (class_type->num_interfaces) {
		// 必须有实现接口标记
		ZEND_ASSERT(class_type->ce_flags & ZEND_ACC_RESOLVED_INTERFACES);
		// 遍历每个接口
		for (uint32_t i = 0; i < class_type->num_interfaces; i++) {
			// 找到 IteratorAggregate 或 Iterator
			if (class_type->interfaces[i] == zend_ce_aggregate || class_type->interfaces[i] == zend_ce_iterator) {
				// 返回成功
				return SUCCESS;
			}
		}
	}
	// 报错：必须实现 traversable 作为 iterator 或 aggregate 的一部分
	zend_error_noreturn(E_CORE_ERROR, "%s %s must implement interface %s as part of either %s or %s",
		zend_get_object_type_uc(class_type),
		ZSTR_VAL(class_type->name),
		ZSTR_VAL(zend_ce_traversable->name),
		ZSTR_VAL(zend_ce_iterator->name),
		ZSTR_VAL(zend_ce_aggregate->name));
	return FAILURE;
}
/* }}} */

/* {{{ zend_implement_aggregate */
// ing3, 实现 aggregate 接口
static int zend_implement_aggregate(zend_class_entry *interface, zend_class_entry *class_type)
{
	// 如果实现失败
	if (zend_class_implements_interface(class_type, zend_ce_iterator)) {
		// 报错，不可以同时实现 Iterator 和 IteratorAggregate
		zend_error_noreturn(E_ERROR,
			"Class %s cannot implement both Iterator and IteratorAggregate at the same time",
			ZSTR_VAL(class_type->name));
	}

	/* Always initialize iterator_funcs_ptr. */
	// 总是初始化 iterator_funcs_ptr。一开始必须没有 iterator_funcs_ptr
	ZEND_ASSERT(!class_type->iterator_funcs_ptr && "Iterator funcs already set?");
	// 如果是内部类，分配持久内存。不是内部类，用arena分配内存。存放函数表。
	zend_class_iterator_funcs *funcs_ptr = class_type->type == ZEND_INTERNAL_CLASS
		? pemalloc(sizeof(zend_class_iterator_funcs), 1)
		: zend_arena_alloc(&CG(arena), sizeof(zend_class_iterator_funcs));
	// 新建的函数表关联到 此类
	class_type->iterator_funcs_ptr = funcs_ptr;
	// 函数表置空
	memset(funcs_ptr, 0, sizeof(zend_class_iterator_funcs));
	// 添加 getiterator 方法
	funcs_ptr->zf_new_iterator = zend_hash_str_find_ptr(
		&class_type->function_table, "getiterator", sizeof("getiterator") - 1);
	// 如果有 get_iterator 方法并且它不是 zend_user_it_get_new_iterator 函数
	if (class_type->get_iterator && class_type->get_iterator != zend_user_it_get_new_iterator) {
		// 对于内部类，get_iterator 已经被覆盖了
		/* get_iterator was explicitly assigned for an internal class. */
		// 如果没有父类 或者 父类的 get_iterator 与此类不同  
		if (!class_type->parent || class_type->parent->get_iterator != class_type->get_iterator) {
			// 必须是内部类
			ZEND_ASSERT(class_type->type == ZEND_INTERNAL_CLASS);
			// 返回成功
			return SUCCESS;
		}
		// getIterator() 方法没有被覆盖过，继承父类的 get_iterator()
		/* The getIterator() method has not been overwritten, use inherited get_iterator(). */
		// 如果 zf_new_iterator 不属于此类
		if (funcs_ptr->zf_new_iterator->common.scope != class_type) {
			// 返回成功
			return SUCCESS;
		}

		// 最后一种情况 getIterator() 被重写了，切换到 zend_user_it_get_new_iterator
		/* getIterator() has been overwritten, switch to zend_user_it_get_new_iterator. */
	}
	// 使用 zend_user_it_get_new_iterator
	class_type->get_iterator = zend_user_it_get_new_iterator;
	// 返回成功 
	return SUCCESS;
}
/* }}} */

/* {{{ zend_implement_iterator */
// ing3, 实现 iterator 接口
static int zend_implement_iterator(zend_class_entry *interface, zend_class_entry *class_type)
{
	// 实现 zend_ce_aggregate
	if (zend_class_implements_interface(class_type, zend_ce_aggregate)) {
		// 如果失败，报错，无法同时实现 Iterator 和 IteratorAggregate
		zend_error_noreturn(E_ERROR,
			"Class %s cannot implement both Iterator and IteratorAggregate at the same time",
			ZSTR_VAL(class_type->name));
	}
	// iterator_funcs_ptr 必须为空
	ZEND_ASSERT(!class_type->iterator_funcs_ptr && "Iterator funcs already set?");
	// 如果是内部类，分配持久内存。不是内部类，用arena分配内存。存放函数表。
	zend_class_iterator_funcs *funcs_ptr = class_type->type == ZEND_INTERNAL_CLASS
		? pemalloc(sizeof(zend_class_iterator_funcs), 1)
		: zend_arena_alloc(&CG(arena), sizeof(zend_class_iterator_funcs));
	// 函数表关联到此类
	class_type->iterator_funcs_ptr = funcs_ptr;
	// 函数表置空
	memset(funcs_ptr, 0, sizeof(zend_class_iterator_funcs));
	// 复制 rewind, valid, key, cuttent, next
	funcs_ptr->zf_rewind = zend_hash_str_find_ptr(
		&class_type->function_table, "rewind", sizeof("rewind") - 1);
	funcs_ptr->zf_valid = zend_hash_str_find_ptr(
		&class_type->function_table, "valid", sizeof("valid") - 1);
	funcs_ptr->zf_key = zend_hash_str_find_ptr(
		&class_type->function_table, "key", sizeof("key") - 1);
	funcs_ptr->zf_current = zend_hash_str_find_ptr(
		&class_type->function_table, "current", sizeof("current") - 1);
	funcs_ptr->zf_next = zend_hash_str_find_ptr(
		&class_type->function_table, "next", sizeof("next") - 1);

	// 如果有 get_iterator 并且不是 zend_user_it_get_iterator
	if (class_type->get_iterator && class_type->get_iterator != zend_user_it_get_iterator) {
		// 如果没有父类 或父类的 get_iterator与此类不同
		if (!class_type->parent || class_type->parent->get_iterator != class_type->get_iterator) {
			// get_iterator 被重写了
			/* get_iterator was explicitly assigned for an internal class. */
			// 必须是内置类
			ZEND_ASSERT(class_type->type == ZEND_INTERNAL_CLASS);
			// 返回成功
			return SUCCESS;
		}
		// 如果 Iterator 方法表没有被覆盖过，继承 get_iterator()
		/* None of the Iterator methods have been overwritten, use inherited get_iterator(). */
		// 如果每个方法都不是此类的（说明没有被覆盖过）
		if (funcs_ptr->zf_rewind->common.scope != class_type &&
				funcs_ptr->zf_valid->common.scope != class_type &&
				funcs_ptr->zf_key->common.scope != class_type &&
				funcs_ptr->zf_current->common.scope != class_type &&
				funcs_ptr->zf_next->common.scope != class_type) {
			// 返回成功
			return SUCCESS;
		}
		// 如果有方法被覆盖过，切换到 zend_user_it_get_iterator
		/* One of the Iterator methods has been overwritten,
		 * switch to zend_user_it_get_iterator. */
	}
	// 使用 zend_user_it_get_iterator
	class_type->get_iterator = zend_user_it_get_iterator;
	// 返回成功
	return SUCCESS;
}
/* }}} */

/* {{{ zend_implement_arrayaccess */
// ing4, 实现 arrayaccess 接口
static int zend_implement_arrayaccess(zend_class_entry *interface, zend_class_entry *class_type)
{
	// 数组接口指针必须为空
	ZEND_ASSERT(!class_type->arrayaccess_funcs_ptr && "ArrayAccess funcs already set?");
	// 如果是内部类，分配持久内存。不是内部类，用arena分配内存。存放函数表。
	zend_class_arrayaccess_funcs *funcs_ptr = class_type->type == ZEND_INTERNAL_CLASS
		? pemalloc(sizeof(zend_class_arrayaccess_funcs), 1)
		: zend_arena_alloc(&CG(arena), sizeof(zend_class_arrayaccess_funcs));
	// 数组访问接口的方法列表
	class_type->arrayaccess_funcs_ptr = funcs_ptr;
	// 像数组一样读取
	funcs_ptr->zf_offsetget = zend_hash_str_find_ptr(
		&class_type->function_table, "offsetget", sizeof("offsetget") - 1);
	// 像数组一样检查是否存在
	funcs_ptr->zf_offsetexists = zend_hash_str_find_ptr(
		&class_type->function_table, "offsetexists", sizeof("offsetexists") - 1);
	// 像数组一样写入
	funcs_ptr->zf_offsetset = zend_hash_str_find_ptr(
		&class_type->function_table, "offsetset", sizeof("offsetset") - 1);
	// 像数组一样删除
	funcs_ptr->zf_offsetunset = zend_hash_str_find_ptr(
		&class_type->function_table, "offsetunset", sizeof("offsetunset") - 1);

	return SUCCESS;
}
/* }}} */

/* {{{ zend_user_serialize */
// ing4, 序列化方法
ZEND_API int zend_user_serialize(zval *object, unsigned char **buffer, size_t *buf_len, zend_serialize_data *data)
{
	zend_class_entry * ce = Z_OBJCE_P(object);
	zval retval;
	zend_result result;
	// 调用序列化方法，传入object的对象指针 和 对象所属类
	zend_call_method_with_0_params(
		Z_OBJ_P(object), Z_OBJCE_P(object), NULL, "serialize", &retval);
	// 如果返回结果为未定义 或 抛错
	if (Z_TYPE(retval) == IS_UNDEF || EG(exception)) {
		// 返回失败
		result = FAILURE;
	// 如果返回正常
	} else {
		// 按结果类型操作
		switch(Z_TYPE(retval)) {
		// 返回null
		case IS_NULL:
			// ?
			/* we could also make this '*buf_len = 0' but this allows to skip variables */
			// 销毁返回值
			zval_ptr_dtor(&retval);
			// 返回失败
			return FAILURE;
		// 返回字串
		case IS_STRING:
			// 返回字串。(e-string-ndump)调用zend方法分配内存，复制字串，自定义长度
			*buffer = (unsigned char*)estrndup(Z_STRVAL(retval), Z_STRLEN(retval));
			// 返回长度
			*buf_len = Z_STRLEN(retval);
			// 结果为成功
			result = SUCCESS;
			break;
		// 默认情况
		default: /* failure */
			// 结果为失败
			result = FAILURE;
			break;
		}
		zval_ptr_dtor(&retval);
	}
	// 如果结果是失败 并且没有异常
	if (result == FAILURE && !EG(exception)) {
		// 抛异常：serialize() 必须返回字串或null
		zend_throw_exception_ex(NULL, 0, "%s::serialize() must return a string or NULL", ZSTR_VAL(ce->name));
	}
	return result;
}
/* }}} */

/* {{{ zend_user_unserialize */
// ing4, 调用用户的反序列化方法
ZEND_API int zend_user_unserialize(zval *object, zend_class_entry *ce, const unsigned char *buf, size_t buf_len, zend_unserialize_data *data)
{
	zval zdata;
	// 初始化对象
	if (UNEXPECTED(object_init_ex(object, ce) != SUCCESS)) {
		return FAILURE;
	}
	// 给zdata添加字串
	ZVAL_STRINGL(&zdata, (char*)buf, buf_len);
	// 调用对象的 unserialize方法，传入对象 和 所属类，并引用传入字串
	zend_call_method_with_1_params(
		Z_OBJ_P(object), Z_OBJCE_P(object), NULL, "unserialize", NULL, &zdata);
	// 销毁字串
	zval_ptr_dtor(&zdata);
	// 如果有异常
	if (EG(exception)) {
		// 返回失败
		return FAILURE;
	// 没有异常
	} else {
		// 返回成功
		return SUCCESS;
	}
}
/* }}} */

/* {{{ zend_implement_serializable */
// ing4, 实现可序列化接口
static int zend_implement_serializable(zend_class_entry *interface, zend_class_entry *class_type)
{
	// 有父类 并且 （父类有序列化方法 或 反序列化方法） 并且 父类没有继承 serializable 接口
	if (class_type->parent
		&& (class_type->parent->serialize || class_type->parent->unserialize)
		&& !zend_class_implements_interface(class_type->parent, zend_ce_serializable)) {
		// 返回失败
		return FAILURE;
	}
	// 如果没有序列化方法
	if (!class_type->serialize) {
		// 调用用户定义的序列化方法
		class_type->serialize = zend_user_serialize;
	}
	// 如果没有反序列化方法
	if (!class_type->unserialize) {
		// 调用用户定义的反序列化方法
		class_type->unserialize = zend_user_unserialize;
	}
	// 原来类是先解释，后实现接口的，实现时成员方法都已经解释完了。
	// 如果类不是抽象类
	if (!(class_type->ce_flags & ZEND_ACC_EXPLICIT_ABSTRACT_CLASS)
			// 并且没有 __serialize 或 __unserialize 方法
			&& (!class_type->__serialize || !class_type->__unserialize)) {
		// Serializable 是已弃用的接口。只要实现 __serialize() 或 __unserialize() 方法
		zend_error(E_DEPRECATED, "%s implements the Serializable interface, which is deprecated. Implement __serialize() and __unserialize() instead (or in addition, if support for old PHP versions is necessary)", ZSTR_VAL(class_type->name));
	}
	// 返回成功
	return SUCCESS;
}
/* }}}*/

// zend内置迭代器
typedef struct {
	zend_object std;
	zend_object_iterator *iter;
	bool rewind_called;
} zend_internal_iterator;

// ing4, 创建内置迭代器
static zend_object *zend_internal_iterator_create(zend_class_entry *ce) {
	// 分配内存创建 zend 内置迭代器
	zend_internal_iterator *intern = emalloc(sizeof(zend_internal_iterator));
	// 初始化迭代器自带的对象
	zend_object_std_init(&intern->std, ce);
	// 给它添加内置迭代器方法
	intern->std.handlers = &zend_internal_iterator_handlers;
	// 它的zend迭代器是空
	intern->iter = NULL;
	// 没有调用过复位
	intern->rewind_called = 0;
	// 返回下属对象的指针。std是第一个元素，所以返回的也是自己的指针
	return &intern->std;
}

// ing3, 创建内置迭代器，并关联到对象自己的迭代器
ZEND_API zend_result zend_create_internal_iterator_zval(zval *return_value, zval *obj) {
	// 从正在执行的上下文中取出作用域
	zend_class_entry *scope = EG(current_execute_data)->func->common.scope;
	// 获取迭代器方法不可以是 zend_user_it_get_new_iterator ？
	ZEND_ASSERT(scope->get_iterator != zend_user_it_get_new_iterator);
	// 获取 zend对象迭代器 
	zend_object_iterator *iter = scope->get_iterator(Z_OBJCE_P(obj), obj, /* by_ref */ 0);
	// 如果获取不到
	if (!iter) {
		// 返回失败
		return FAILURE;
	}
	// 创建内置迭代器
	zend_internal_iterator *intern =
		(zend_internal_iterator *) zend_internal_iterator_create(zend_ce_internal_iterator);
	// 关联下属zend迭代器
	intern->iter = iter;
	// 索引号为0
	intern->iter->index = 0;
	// 返回 内置迭代器下属对象的指针（也是内置迭代器自己的指针）
	ZVAL_OBJ(return_value, &intern->std);
	return SUCCESS;
}

// ing4, 销毁内置迭代器
static void zend_internal_iterator_free(zend_object *obj) {
	// 内置迭代器
	zend_internal_iterator *intern = (zend_internal_iterator *) obj;
	// 如果 有zend迭代器
	if (intern->iter) {
		// 先销毁zend迭代器
		zend_iterator_dtor(intern->iter);
	}
	// 再销毁迭代器带的 zend_object实例 
	zend_object_std_dtor(&intern->std);
}

// ing3, 获取This指向的内置迭代器。 传入的是 This（zval）
static zend_internal_iterator *zend_internal_iterator_fetch(zval *This) {
	// This 的下属对象指针是一个 内置迭代器
	zend_internal_iterator *intern = (zend_internal_iterator *) Z_OBJ_P(This);
	// 如果迭代器没有下属 zend迭代器
	if (!intern->iter) {
		// 报错：内置迭代器没有被 正确初始化
		zend_throw_error(NULL, "The InternalIterator object has not been properly initialized");
		return NULL;
	}
	// 返回内置迭代器
	return intern;
}

/* Many iterators will not behave correctly if rewind() is not called, make sure it happens. */
// ing4, 如果没有调用 rewind() 很多迭代器无法正常运作
static zend_result zend_internal_iterator_ensure_rewound(zend_internal_iterator *intern) {
	// 如果没有调用过 rewind
	if (!intern->rewind_called) {
		// 获取对象迭代器
		zend_object_iterator *iter = intern->iter;
		// 标记成调用过
		intern->rewind_called = 1;
		// 如果有 rewind 方法
		if (iter->funcs->rewind) {
			// 调用 rewind 方法
			iter->funcs->rewind(iter);
			// 如果有异常，抛出异常
			if (UNEXPECTED(EG(exception))) {
				// 返回失败
				return FAILURE;
			}
		}
	}
	// 返回成功
	return SUCCESS;
}


ZEND_METHOD(InternalIterator, __construct) {
	zend_throw_error(NULL, "Cannot manually construct InternalIterator");
}

ZEND_METHOD(InternalIterator, current) {
	ZEND_PARSE_PARAMETERS_NONE();

	zend_internal_iterator *intern = zend_internal_iterator_fetch(ZEND_THIS);
	if (!intern) {
		RETURN_THROWS();
	}

	if (zend_internal_iterator_ensure_rewound(intern) == FAILURE) {
		RETURN_THROWS();
	}

	zval *data = intern->iter->funcs->get_current_data(intern->iter);
	if (data) {
		RETURN_COPY_DEREF(data);
	}
}

ZEND_METHOD(InternalIterator, key) {
	ZEND_PARSE_PARAMETERS_NONE();

	zend_internal_iterator *intern = zend_internal_iterator_fetch(ZEND_THIS);
	if (!intern) {
		RETURN_THROWS();
	}

	if (zend_internal_iterator_ensure_rewound(intern) == FAILURE) {
		RETURN_THROWS();
	}

	if (intern->iter->funcs->get_current_key) {
		intern->iter->funcs->get_current_key(intern->iter, return_value);
	} else {
		RETURN_LONG(intern->iter->index);
	}
}

ZEND_METHOD(InternalIterator, next) {
	ZEND_PARSE_PARAMETERS_NONE();

	zend_internal_iterator *intern = zend_internal_iterator_fetch(ZEND_THIS);
	if (!intern) {
		RETURN_THROWS();
	}

	if (zend_internal_iterator_ensure_rewound(intern) == FAILURE) {
		RETURN_THROWS();
	}

	/* Advance index first to match foreach behavior. */
	intern->iter->index++;
	intern->iter->funcs->move_forward(intern->iter);
}

ZEND_METHOD(InternalIterator, valid) {
	ZEND_PARSE_PARAMETERS_NONE();

	zend_internal_iterator *intern = zend_internal_iterator_fetch(ZEND_THIS);
	if (!intern) {
		RETURN_THROWS();
	}

	if (zend_internal_iterator_ensure_rewound(intern) == FAILURE) {
		RETURN_THROWS();
	}

	RETURN_BOOL(intern->iter->funcs->valid(intern->iter) == SUCCESS);
}

ZEND_METHOD(InternalIterator, rewind) {
	ZEND_PARSE_PARAMETERS_NONE();

	zend_internal_iterator *intern = zend_internal_iterator_fetch(ZEND_THIS);
	if (!intern) {
		RETURN_THROWS();
	}

	if (!intern->iter->funcs->rewind) {
		/* Allow calling rewind() if no iteration has happened yet,
		 * even if the iterator does not support rewinding. */
		if (intern->iter->index != 0) {
			zend_throw_error(NULL, "Iterator does not support rewinding");
			RETURN_THROWS();
		}
		intern->iter->index = 0;
		return;
	}

	intern->iter->funcs->rewind(intern->iter);
	intern->iter->index = 0;
}

// ing3, 注册接口
/* {{{ zend_register_interfaces */
ZEND_API void zend_register_interfaces(void)
{
	// 可遍历 
	zend_ce_traversable = register_class_Traversable();
	zend_ce_traversable->interface_gets_implemented = zend_implement_traversable;
	// 可迭代
	zend_ce_aggregate = register_class_IteratorAggregate(zend_ce_traversable);
	zend_ce_aggregate->interface_gets_implemented = zend_implement_aggregate;
	// 迭代器
	zend_ce_iterator = register_class_Iterator(zend_ce_traversable);
	zend_ce_iterator->interface_gets_implemented = zend_implement_iterator;
	// 可序列化
	zend_ce_serializable = register_class_Serializable();
	zend_ce_serializable->interface_gets_implemented = zend_implement_serializable;
	// 像数组一样访问
	zend_ce_arrayaccess = register_class_ArrayAccess();
	zend_ce_arrayaccess->interface_gets_implemented = zend_implement_arrayaccess;
	// 可计数
	zend_ce_countable = register_class_Countable();
	// tostring
	zend_ce_stringable = register_class_Stringable();
	// 内部迭代器
	zend_ce_internal_iterator = register_class_InternalIterator(zend_ce_iterator);
	zend_ce_internal_iterator->create_object = zend_internal_iterator_create;
	// 内部迭代器 的处理器，使用标准对象处理器
	memcpy(&zend_internal_iterator_handlers, zend_get_std_object_handlers(),
		sizeof(zend_object_handlers));
	// 内部迭代器的释放方法
	zend_internal_iterator_handlers.free_obj = zend_internal_iterator_free;
}
/* }}} */
