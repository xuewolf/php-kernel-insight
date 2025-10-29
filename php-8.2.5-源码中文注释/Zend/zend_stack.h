// ing4
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

#ifndef ZEND_STACK_H
#define ZEND_STACK_H

// 堆栈
typedef struct _zend_stack {
	// size 元素尺寸, top 元素数, max最大容量
	int size, top, max;
	// 元素集合
	void *elements;
} zend_stack;

// 重新分配内存是消耗比较大的操作，所以常有这样的设置，一次多分配一些空间，
// 虽然有些浪费但可以避免频繁转移数据(分配新空间，复制，清空原空间）
#define STACK_BLOCK_SIZE 16

BEGIN_EXTERN_C()
ZEND_API void zend_stack_init(zend_stack *stack, int size);
ZEND_API int zend_stack_push(zend_stack *stack, const void *element);
ZEND_API void *zend_stack_top(const zend_stack *stack);
ZEND_API void zend_stack_del_top(zend_stack *stack);
ZEND_API int zend_stack_int_top(const zend_stack *stack);
ZEND_API bool zend_stack_is_empty(const zend_stack *stack);
ZEND_API void zend_stack_destroy(zend_stack *stack);
ZEND_API void *zend_stack_base(const zend_stack *stack);
ZEND_API int zend_stack_count(const zend_stack *stack);
ZEND_API void zend_stack_apply(zend_stack *stack, int type, int (*apply_function)(void *element));
ZEND_API void zend_stack_apply_with_argument(zend_stack *stack, int type, int (*apply_function)(void *element, void *arg), void *arg);
ZEND_API void zend_stack_clean(zend_stack *stack, void (*func)(void *), bool free_elements);
END_EXTERN_C()

#define ZEND_STACK_APPLY_TOPDOWN	1
#define ZEND_STACK_APPLY_BOTTOMUP	2

#endif /* ZEND_STACK_H */
