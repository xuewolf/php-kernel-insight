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
   | Author: Wez Furlong <wez@thebrainroom.com>                           |
   |         Marcus Boerger <helly@php.net>                               |
   +----------------------------------------------------------------------+
*/

#include "zend.h"
#include "zend_API.h"

static zend_class_entry zend_iterator_class_entry;

static void iter_wrapper_free(zend_object *object);
static void iter_wrapper_dtor(zend_object *object);
static HashTable *iter_wrapper_get_gc(zend_object *object, zval **table, int *n);

// 【迭代器封装】类 一共只有3个有效方法
static const zend_object_handlers iterator_object_handlers = {
	// offset
	0,
	/* free_obj */
	iter_wrapper_free,
	/* dtor_obj */
	iter_wrapper_dtor,
	NULL, /* clone_obj */
	NULL, /* prop read */
	NULL, /* prop write */
	NULL, /* read dim */
	NULL, /* write dim */
	NULL, /* get_property_ptr_ptr */
	NULL, /* has prop */
	NULL, /* unset prop */
	NULL, /* has dim */
	NULL, /* unset dim */
	NULL, /* props get */
	NULL, /* method get */
	NULL, /* get ctor */
	NULL, /* get class name */
	NULL, /* cast */
	NULL, /* count */
	NULL, /* get_debug_info */
	NULL, /* get_closure */
	/* get_gc */
	iter_wrapper_get_gc,
	NULL, /* do_operation */
	NULL, /* compare */
	NULL  /* get_properties_for */
};

// ing3, 创建类 __iterator_wrapper 迭代器封装
ZEND_API void zend_register_iterator_wrapper(void)
{
	// 用给定的名字创建新类，并添加成员方法列表(NULL)。这里没有把方法列表加在类里，而是加在对象上。
	INIT_CLASS_ENTRY(zend_iterator_class_entry, "__iterator_wrapper", NULL);
}

// ing3, 释放 迭代器封装
static void iter_wrapper_free(zend_object *object)
{
	// 对象转成 对象迭代器
	zend_object_iterator *iter = (zend_object_iterator*)object;
	// 调用自己的 销毁方法
	iter->funcs->dtor(iter);
}

// ing3, 销毁 迭代器封装，空方法
static void iter_wrapper_dtor(zend_object *object)
{
}

// ing3, 返回 迭代器封装 的 gc计数器
static HashTable *iter_wrapper_get_gc(zend_object *object, zval **table, int *n) {
	// 转成对象迭代器
	zend_object_iterator *iter = (zend_object_iterator*)object;
	// 如果有 get_gc 方法，调用它
	if (iter->funcs->get_gc) {
		// table 和 n是，属性表和 属性数量
		return iter->funcs->get_gc(iter, table, n);
	}
	// 如果没有 get_gc方法
	
	// 返回属性表指针 置空
	*table = NULL;
	// 返回 属性数量 清0
	*n = 0;
	return NULL;
}

// ing3, 初始化 迭代器封装
ZEND_API void zend_iterator_init(zend_object_iterator *iter)
{
	// 给 迭代器封装对象 创建一个 迭代器实例
	zend_object_std_init(&iter->std, &zend_iterator_class_entry);
	// 给这个实例添加处理器 iterator_object_handlers
	iter->std.handlers = &iterator_object_handlers;
}

// ing3, 迭代器销毁器
ZEND_API void zend_iterator_dtor(zend_object_iterator *iter)
{
	// 如果引用次数-1后仍有引用，中断操作
	if (GC_DELREF(&iter->std) > 0) {
		return;
	}
	// 在对象容器中找到并删除迭代器，但并没删除 iter(迭代器封装)
	zend_objects_store_del(&iter->std);
}

// ing3, 获取 zval中的迭代器，加了个判断，类型是否是迭代器
ZEND_API zend_object_iterator* zend_iterator_unwrap(zval *array_ptr)
{
	// zval 类型必须是对象
	ZEND_ASSERT(Z_TYPE_P(array_ptr) == IS_OBJECT);
	// 通过指针访问 zval中的对象指针的操作方法集
	// 如果方法集是 iterator_object_handlers
	if (Z_OBJ_HT_P(array_ptr) == &iterator_object_handlers) {
		// 返回迭代器
		return (zend_object_iterator *)Z_OBJ_P(array_ptr);
	}
	// 不符合，返回null
	return NULL;
}
