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
   +----------------------------------------------------------------------+
*/

#ifndef ZEND_LLIST_H
#define ZEND_LLIST_H

// 列表元素
typedef struct _zend_llist_element {
	// 后一个元素指针
	struct _zend_llist_element *next;
	// 前一个元素指针
	struct _zend_llist_element *prev;
	// 字串数据，有必要保持作为结构体的最后一个元素（因为长度不固定）
	char data[1]; /* Needs to always be last in the struct */
} zend_llist_element;

typedef void (*llist_dtor_func_t)(void *);
typedef int (*llist_compare_func_t)(const zend_llist_element **, const zend_llist_element **);
typedef void (*llist_apply_with_args_func_t)(void *data, int num_args, va_list args);
typedef void (*llist_apply_with_arg_func_t)(void *data, void *arg);
typedef void (*llist_apply_func_t)(void *);

// 列表
typedef struct _zend_llist {
	// 头原素指针
	zend_llist_element *head;
	// 尾元素指针
	zend_llist_element *tail;
	// 元素数量
	size_t count;
	// list每个元素的数据空间 大小（元素大小是：元素基本大小 + 数据大小 -1）
	size_t size;
	// 销毁器指针
	llist_dtor_func_t dtor;
	// 永久 （这个是开辟内存时带的标记，释放内存时需要用到它）
	unsigned char persistent;
	// 游标？
	zend_llist_element *traverse_ptr;
} zend_llist;

// zend_llist_position 是指向 zend_llist_element 的指针
typedef zend_llist_element* zend_llist_position;

BEGIN_EXTERN_C()
ZEND_API void zend_llist_init(zend_llist *l, size_t size, llist_dtor_func_t dtor, unsigned char persistent);
ZEND_API void zend_llist_add_element(zend_llist *l, const void *element);
ZEND_API void zend_llist_prepend_element(zend_llist *l, const void *element);
ZEND_API void zend_llist_del_element(zend_llist *l, void *element, int (*compare)(void *element1, void *element2));
ZEND_API void zend_llist_destroy(zend_llist *l);
ZEND_API void zend_llist_clean(zend_llist *l);
ZEND_API void zend_llist_remove_tail(zend_llist *l);
ZEND_API void zend_llist_copy(zend_llist *dst, zend_llist *src);
ZEND_API void zend_llist_apply(zend_llist *l, llist_apply_func_t func);
ZEND_API void zend_llist_apply_with_del(zend_llist *l, int (*func)(void *data));
ZEND_API void zend_llist_apply_with_argument(zend_llist *l, llist_apply_with_arg_func_t func, void *arg);
ZEND_API void zend_llist_apply_with_arguments(zend_llist *l, llist_apply_with_args_func_t func, int num_args, ...);
ZEND_API size_t zend_llist_count(zend_llist *l);
ZEND_API void zend_llist_sort(zend_llist *l, llist_compare_func_t comp_func);

/* traversal */
ZEND_API void *zend_llist_get_first_ex(zend_llist *l, zend_llist_position *pos);
ZEND_API void *zend_llist_get_last_ex(zend_llist *l, zend_llist_position *pos);
ZEND_API void *zend_llist_get_next_ex(zend_llist *l, zend_llist_position *pos);
ZEND_API void *zend_llist_get_prev_ex(zend_llist *l, zend_llist_position *pos);

static zend_always_inline void *zend_llist_get_first(zend_llist *l)
{
	return zend_llist_get_first_ex(l, NULL);
}

static zend_always_inline void *zend_llist_get_last(zend_llist *l)
{
	return zend_llist_get_last_ex(l, NULL);
}

static zend_always_inline void *zend_llist_get_next(zend_llist *l)
{
	return zend_llist_get_next_ex(l, NULL);
}

static zend_always_inline void *zend_llist_get_prev(zend_llist *l)
{
	return zend_llist_get_prev_ex(l, NULL);
}

END_EXTERN_C()

#endif /* ZEND_LLIST_H */
