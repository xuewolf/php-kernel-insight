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

#include <stdio.h>
#include "zend.h"
#include "zend_API.h"
#include "zend_ast.h"
#include "zend_globals.h"
#include "zend_constants.h"
#include "zend_list.h"

// 调试状态
#if ZEND_DEBUG
static void ZEND_FASTCALL zend_string_destroy(zend_string *str);
// 默认直接用 _efree 销毁 zend_string
#else
# define zend_string_destroy _efree
#endif

static void ZEND_FASTCALL zend_reference_destroy(zend_reference *ref);
static void ZEND_FASTCALL zend_empty_destroy(zend_reference *ref);

// 函数形式的强制转换，并没有太多奥秘。这里必须是指针，否则后面用到时无法转换。
// 用于把接收到的参数类型和返回值类型 转换到统一的格式
typedef void (ZEND_FASTCALL *zend_rc_dtor_func_t)(zend_refcounted *p);

// 针对每一种类型的销毁方法， dtor是 destructor 的缩写
// 类型定义在 zend_types.h 里
static const zend_rc_dtor_func_t zend_rc_dtor_func[] = {
	// 这些类型是不需要用销毁器销毁的， zend_empty_destroy 是空函数，无业务逻辑
	/* IS_UNDEF        */ (zend_rc_dtor_func_t)zend_empty_destroy,
	/* IS_NULL         */ (zend_rc_dtor_func_t)zend_empty_destroy,
	/* IS_FALSE        */ (zend_rc_dtor_func_t)zend_empty_destroy,
	/* IS_TRUE         */ (zend_rc_dtor_func_t)zend_empty_destroy,
	/* IS_LONG         */ (zend_rc_dtor_func_t)zend_empty_destroy,
	/* IS_DOUBLE       */ (zend_rc_dtor_func_t)zend_empty_destroy,
	// 以上类型不需要专门的销毁方法
	
	// 下面这些结构体的开头都是 zend_refcounted_h gc; 所以可以用计数器指针直接转成对象指针。所以这6个类型和gc关系最密切
	// 字串, 直接用 _efree 销毁
	/* IS_STRING       */ (zend_rc_dtor_func_t)zend_string_destroy,
	// 数组， zend_hash.c
	/* IS_ARRAY        */ (zend_rc_dtor_func_t)zend_array_destroy,
	// 对象
	/* IS_OBJECT       */ (zend_rc_dtor_func_t)zend_objects_store_del,
	// 资源
	/* IS_RESOURCE     */ (zend_rc_dtor_func_t)zend_list_free,
	// 引用
	/* IS_REFERENCE    */ (zend_rc_dtor_func_t)zend_reference_destroy,
	// 常量表达式
	/* IS_CONSTANT_AST */ (zend_rc_dtor_func_t)zend_ast_ref_destroy
};

// ing4, 调用每个type对应的销毁函数执行销毁
	// 为什么 zend_refcounted 可以转成任何类型？ 这和每一个的结构有关 
	// _zend_string 的开头是 zend_refcounted_h gc; （zend_string_destroy）
	// HashTable（_zend_array）的开头是 zend_refcounted_h gc; （zend_array_destroy）
	// _zend_object 的开头是 zend_refcounted_h gc; （zend_objects_store_del）
	// _zend_resource 的开头是 zend_refcounted_h gc; （zend_list_free）
	// _zend_reference 的开头是 zend_refcounted_h gc; （zend_reference_destroy）
	// _zend_ast_ref  的开头是 zend_refcounted_h gc; （zend_ast_ref_destroy）
ZEND_API void ZEND_FASTCALL rc_dtor_func(zend_refcounted *p)
{
	// 必须是有效类型
	ZEND_ASSERT(GC_TYPE(p) <= IS_CONSTANT_AST);
	zend_rc_dtor_func[GC_TYPE(p)](p);
}

// debug模式下才有的
#if ZEND_DEBUG
static void ZEND_FASTCALL zend_string_destroy(zend_string *str)
{
	CHECK_ZVAL_STRING(str);
	ZEND_ASSERT(!ZSTR_IS_INTERNED(str));
	ZEND_ASSERT(GC_REFCOUNT(str) == 0);
	ZEND_ASSERT(!(GC_FLAGS(str) & IS_STR_PERSISTENT));
	efree(str);
}
#endif

// ing3, 销毁 zend_reference，引用类型
static void ZEND_FASTCALL zend_reference_destroy(zend_reference *ref)
{
	// 引用目标(ref->sources)必须 是 NULL
	ZEND_ASSERT(!ZEND_REF_HAS_TYPE_SOURCES(ref));
	// 销毁没有引用次数的对象，有引用次数的添加到gc回收周期里。并没有销毁 zval本身。
	i_zval_ptr_dtor(&ref->val);
	// 释放ref
	efree_size(ref, sizeof(zend_reference));
}

// ing4, 不需要释放的
static void ZEND_FASTCALL zend_empty_destroy(zend_reference *ref)
{
}

// ing4, 销毁没有引用次数的对象，有引用次数的添加到gc回收周期里。并没有销毁 zval本身。（大量引用）
ZEND_API void zval_ptr_dtor(zval *zval_ptr) /* {{{ */
{
	// 销毁没有引用次数的对象，有引用次数的添加到gc回收周期里。并没有销毁 zval本身。
	i_zval_ptr_dtor(zval_ptr);
}
/* }}} */

// ing3, 销毁内部zval（其实只有内部string）
ZEND_API void zval_internal_ptr_dtor(zval *zval_ptr) /* {{{ */
{
	// 如果是可计数变量
	if (Z_REFCOUNTED_P(zval_ptr)) {
		// gc计数器
		zend_refcounted *ref = Z_COUNTED_P(zval_ptr);
		// 如果引用数是 0 
		if (GC_DELREF(ref) == 0) {
			// 如果是字串
			if (Z_TYPE_P(zval_ptr) == IS_STRING) {
				zend_string *str = (zend_string*)ref;
				// 调试用
				CHECK_ZVAL_STRING(str);
				// 不可以是内置字串
				ZEND_ASSERT(!ZSTR_IS_INTERNED(str));
				// 必须有持久标记
				ZEND_ASSERT((GC_FLAGS(str) & IS_STR_PERSISTENT));
				// 释放字串
				free(str);
			// 其他情况 抛错：内部zval不可以是 数组，对象
			} else {
				zend_error_noreturn(E_CORE_ERROR, "Internal zval's can't be arrays, objects, resources or reference");
			}
		}
	}
}
/* }}} */

// 这个函数只能当成 复制构造方法 来使用，例如 它只能在 zval 被通过 ZVAL_COPY_VALUE 复制到另一个位置后 调用。
// 不要在复制前调用它，否则引用会 泄露
/* This function should only be used as a copy constructor, i.e. it
 * should only be called AFTER a zval has been copied to another
 * location using ZVAL_COPY_VALUE. Do not call it before copying,
 * otherwise a reference may be leaked. */
 
// ing3, 增加引用次数
ZEND_API void zval_add_ref(zval *p)
{
	// 如果是可计数类型
	if (Z_REFCOUNTED_P(p)) {
		// 如果是引用类型 并且 引用数次是1
		if (Z_ISREF_P(p) && Z_REFCOUNT_P(p) == 1) {
			// 把引用目标复制到 p 
			ZVAL_COPY(p, Z_REFVAL_P(p));
		// 否则 
		} else {
			// 只增加引用次数
			Z_ADDREF_P(p);
		}
	}
}

// ing3, 创建副本，并把zval关联到副本
ZEND_API void ZEND_FASTCALL zval_copy_ctor_func(zval *zvalue)
{
	// 如果类型是数组 
	if (EXPECTED(Z_TYPE_P(zvalue) == IS_ARRAY)) {
		// 创建副本，并把zval关联到副本
		ZVAL_ARR(zvalue, zend_array_dup(Z_ARRVAL_P(zvalue)));
	// 如果类型是字串
	} else if (EXPECTED(Z_TYPE_P(zvalue) == IS_STRING)) {
		// 不可以是内置字串
		ZEND_ASSERT(!ZSTR_IS_INTERNED(Z_STR_P(zvalue)));
		// 调试用
		CHECK_ZVAL_STRING(Z_STR_P(zvalue));
		// 创建副本，并把zval关联到副本
		ZVAL_NEW_STR(zvalue, zend_string_dup(Z_STR_P(zvalue), 0));
	// 其他类型
	} else {
		ZEND_UNREACHABLE();
	}
}
