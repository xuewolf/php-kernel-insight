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
   | Authors: David Wang <planetbeing@gmail.com>                          |
   |          Dmitry Stogov <dmitry@php.net>                              |
   +----------------------------------------------------------------------+
*/

#ifndef ZEND_GC_H
#define ZEND_GC_H

BEGIN_EXTERN_C()

// gc 运行状态
typedef struct _zend_gc_status {
	uint32_t runs;
	uint32_t collected;
	uint32_t threshold;
	uint32_t num_roots;
} zend_gc_status;

ZEND_API extern int (*gc_collect_cycles)(void);

ZEND_API void ZEND_FASTCALL gc_possible_root(zend_refcounted *ref);
ZEND_API void ZEND_FASTCALL gc_remove_from_buffer(zend_refcounted *ref);

/* enable/disable automatic start of GC collection */
ZEND_API bool gc_enable(bool enable);
ZEND_API bool gc_enabled(void);

/* enable/disable possible root additions */
ZEND_API bool gc_protect(bool protect);
ZEND_API bool gc_protected(void);

/* The default implementation of the gc_collect_cycles callback. */
ZEND_API int  zend_gc_collect_cycles(void);

ZEND_API void zend_gc_get_status(zend_gc_status *status);

void gc_globals_ctor(void);
void gc_globals_dtor(void);
void gc_reset(void);
// 如果要求线程安全
#ifdef ZTS
size_t zend_gc_globals_size(void);
#endif

// ing4, 把计数器和root解除关联
#define GC_REMOVE_FROM_BUFFER(p) do { \
		/* 计数器 */ \
		zend_refcounted *_p = (zend_refcounted*)(p); \
		/* 如果有类型 */ \
		if (GC_TYPE_INFO(_p) & GC_INFO_MASK) { \
			/* 把计数器和root解除关联 */ \
			gc_remove_from_buffer(_p); \
		} \
	} while (0)

// ing4, 检验是否可能泄露：是否带有 【不可回收】标记。不可回收的对象会长期占用内存，造成内存泄露
// 所谓泄露，其实就是回收不干净
#define GC_MAY_LEAK(ref) \
	((GC_TYPE_INFO(ref) & \
		(GC_INFO_MASK | (GC_NOT_COLLECTABLE << GC_FLAGS_SHIFT))) == 0)

// ing4, 检验目标是否可回收，可以则放进root里，外部少量引用
// 只有待回收的垃圾才会被添加到这里
static zend_always_inline void gc_check_possible_root(zend_refcounted *ref)
{
	// 如果计数器类型是 引用
	if (EXPECTED(GC_TYPE_INFO(ref) == GC_REFERENCE)) {
		// 取得引用目标
		zval *zv = &((zend_reference*)ref)->val;
		// 如果目标不可回收，返回
		if (!Z_COLLECTABLE_P(zv)) {
			return;
		}
		// 取得目标的gc 对象
		ref = Z_COUNTED_P(zv);
	}
	// 如果ref可能泄露
	if (UNEXPECTED(GC_MAY_LEAK(ref))) {
		// 找一个可用root把ref放进去
		gc_possible_root(ref);
	}
}

// 这些api可以用来简化对象 get_gc实现 通过 多态结构
// zend_generator_get_gc 是一个用例
/* These APIs can be used to simplify object get_gc implementations
 * over heterogeneous structures. See zend_generator_get_gc() for
 * a usage example. */
// 这组指针，指向一大串私有的 zval 列表。start指向开头，end指向结尾，cur是游标
typedef struct {
	zval *cur;
	zval *end;
	zval *start;
} zend_get_gc_buffer;

ZEND_API zend_get_gc_buffer *zend_get_gc_buffer_create(void);
ZEND_API void zend_get_gc_buffer_grow(zend_get_gc_buffer *gc_buffer);

// ing3, 给游标（p1->cur）指向的zval赋值， 完全是外部调用
static zend_always_inline void zend_get_gc_buffer_add_zval(
		zend_get_gc_buffer *gc_buffer, zval *zv) {
	// 如果是可计数类型
	if (Z_REFCOUNTED_P(zv)) {
		// 如果当前指针指向结尾
		if (UNEXPECTED(gc_buffer->cur == gc_buffer->end)) {
			// 增加buf
			zend_get_gc_buffer_grow(gc_buffer);
		}
		// 把 zv 复制给 游标指向的变量
		ZVAL_COPY_VALUE(gc_buffer->cur, zv);
		// 游标后移
		gc_buffer->cur++;
	}
}

// ing3, 给游标指向的zval赋值添加对象，完全是外部调用
static zend_always_inline void zend_get_gc_buffer_add_obj(
		zend_get_gc_buffer *gc_buffer, zend_object *obj) {
	// 
	if (UNEXPECTED(gc_buffer->cur == gc_buffer->end)) {
		zend_get_gc_buffer_grow(gc_buffer);
	}
	ZVAL_OBJ(gc_buffer->cur, obj);
	gc_buffer->cur++;
}

// ing3, 完全是外部调用,p1:gc_buffer,p2,p3:返回 gc_buffer 的开始位置 和 游标偏移量
static zend_always_inline void zend_get_gc_buffer_use(
		zend_get_gc_buffer *gc_buffer, zval **table, int *n) {
	*table = gc_buffer->start;
	*n = gc_buffer->cur - gc_buffer->start;
}

END_EXTERN_C()

#endif /* ZEND_GC_H */
