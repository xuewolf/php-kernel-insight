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
   |          Dmitry Stogov <dmitry@php.net>                              |
   +----------------------------------------------------------------------+
*/

#ifndef ZEND_VARIABLES_H
#define ZEND_VARIABLES_H

#include "zend_types.h"
#include "zend_gc.h"

BEGIN_EXTERN_C()

ZEND_API void ZEND_FASTCALL rc_dtor_func(zend_refcounted *p);
ZEND_API void ZEND_FASTCALL zval_copy_ctor_func(zval *zvalue);

// ing4, 销毁没有引用次数的对象,并没有销毁 zval本身，不放入gc回收周期。nogc的意思是，不放入gc回收周期。大量引用
static zend_always_inline void zval_ptr_dtor_nogc(zval *zval_ptr)
{
	// 如果是可计数变量，并且 -1 后引用次数为0
	if (Z_REFCOUNTED_P(zval_ptr) && !Z_DELREF_P(zval_ptr)) {
		// 删除对象，并没有销毁 zval本身
		rc_dtor_func(Z_COUNTED_P(zval_ptr));
	}
}

// ing4, 销毁没有引用次数的对象，有引用次数的添加到gc回收周期里。并没有销毁 zval本身。
// 它的别名是  zval_ptr_dtor
static zend_always_inline void i_zval_ptr_dtor(zval *zval_ptr)
{
	// 如果内置变量有引用次数
	if (Z_REFCOUNTED_P(zval_ptr)) {
		// 获取 counted 元素
		zend_refcounted *ref = Z_COUNTED_P(zval_ptr);
		// types.h : #define GC_DELREF(p) zend_gc_delref(&(p)->gc)
		// 引用次数-1 后如果为0
		if (!GC_DELREF(ref)) {
			// 删除计数器
			rc_dtor_func(ref);
		// 不为0，还有引用
		} else {
			// 添加到自动回收
			gc_check_possible_root(ref);
		}
	}
}

// suspend, 全局无调用
static zend_always_inline void zval_copy_ctor(zval *zvalue)
{
	// 如果是数组
	if (Z_TYPE_P(zvalue) == IS_ARRAY) {
		ZVAL_ARR(zvalue, zend_array_dup(Z_ARR_P(zvalue)));
	// 如果不是数组，并且可计数
	} else if (Z_REFCOUNTED_P(zvalue)) {
		// 添加引用次数
		Z_ADDREF_P(zvalue);
	}
}

// suspend, 全局无调用
static zend_always_inline void zval_opt_copy_ctor(zval *zvalue)
{
	// 如果是数组
	if (Z_OPT_TYPE_P(zvalue) == IS_ARRAY) {
		ZVAL_ARR(zvalue, zend_array_dup(Z_ARR_P(zvalue)));
	// 如果不是数组，并且可计数
	} else if (Z_OPT_REFCOUNTED_P(zvalue)) {
		// 添加引用次数
		Z_ADDREF_P(zvalue);
	}
}

// ing3, 通过指针销毁 zval（字串）
static zend_always_inline void zval_ptr_dtor_str(zval *zval_ptr)
{
	// 如果是可计数类型， 并且-1后为0 
	if (Z_REFCOUNTED_P(zval_ptr) && !Z_DELREF_P(zval_ptr)) {
		// 传入的 zval 必须是字串
		ZEND_ASSERT(Z_TYPE_P(zval_ptr) == IS_STRING);
		// 传入的字串不能是保留字
		ZEND_ASSERT(!ZSTR_IS_INTERNED(Z_STR_P(zval_ptr)));
		// 传入的字串不能是保留字
		ZEND_ASSERT(!(GC_FLAGS(Z_STR_P(zval_ptr)) & IS_STR_PERSISTENT));
		// 释放zval
		efree(Z_STR_P(zval_ptr));
	}
}

ZEND_API void zval_ptr_dtor(zval *zval_ptr);
ZEND_API void zval_internal_ptr_dtor(zval *zvalue);

// 保证兼容
/* Kept for compatibility */
#define zval_dtor(zvalue) zval_ptr_dtor_nogc(zvalue)

ZEND_API void zval_add_ref(zval *p);

END_EXTERN_C()

#define ZVAL_PTR_DTOR zval_ptr_dtor
#define ZVAL_INTERNAL_PTR_DTOR zval_internal_ptr_dtor

#endif
