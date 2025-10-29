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

#include "zend.h"
#include "zend_stack.h"

// 获取指向第n个元素结尾位置的指针
#define ZEND_STACK_ELEMENT(stack, n) ((void *)((char *) (stack)->elements + (stack)->size * (n)))

// ing4, 初始化堆栈
ZEND_API void zend_stack_init(zend_stack *stack, int size)
{
	// size 是每个元素的大小
	stack->size = size;
	// 当前元素数量
	stack->top = 0;
	// 最大允许元素数量
	stack->max = 0;
	// 元素指针
	stack->elements = NULL;
}

// ing4, 入栈
ZEND_API int zend_stack_push(zend_stack *stack, const void *element)
{
	/* We need to allocate more memory */
	if (stack->top >= stack->max) {
		stack->max += STACK_BLOCK_SIZE;
		// 第一个参数，指针，第二个参数，元素数量，第三个参数，元素尺寸，第四个参数，返回的指针位置
		// 关键在于，重新申请的内存在其地方，原来的应该是被释放了？
		stack->elements = safe_erealloc(stack->elements, stack->size, stack->max, 0);
	}
	// top 后写入新 元素指针
	memcpy(ZEND_STACK_ELEMENT(stack, stack->top), element, stack->size);
	// 数量+1
	return stack->top++;
}

// ing4, 获取最后一个元素的指针
ZEND_API void *zend_stack_top(const zend_stack *stack)
{
	if (stack->top > 0) {
		return ZEND_STACK_ELEMENT(stack, stack->top - 1);
	} else {
		return NULL;
	}
}

// ing4, 降低高度，这时候不修改内存
ZEND_API void zend_stack_del_top(zend_stack *stack)
{
	--stack->top;
}

// ? 这个干嘛用的，为啥要Int类型？
ZEND_API int zend_stack_int_top(const zend_stack *stack)
{
	int *e = zend_stack_top(stack);
	if (e) {
		return *e;
	} else {
		return FAILURE;
	}
}

// ing4, 检测堆栈是否为空
ZEND_API bool zend_stack_is_empty(const zend_stack *stack)
{
	return stack->top == 0;
}

// ing4, 销毁堆栈，删除所有元素，但未销毁堆栈本身
ZEND_API void zend_stack_destroy(zend_stack *stack)
{
	// 如果有元素
	if (stack->elements) {
		// 释放所有元素
		efree(stack->elements);
		// 元素标记成null
		stack->elements = NULL;
	}
}

// ing4, 返回堆栈元素列表的指针。stack->elements 没有++或--，指针永远在开头
ZEND_API void *zend_stack_base(const zend_stack *stack)
{
	return stack->elements;
}

// ing4, 返回堆栈深度（元素数）
ZEND_API int zend_stack_count(const zend_stack *stack)
{
	return stack->top;
}

// ing4, 对所有元素调用函数
ZEND_API void zend_stack_apply(zend_stack *stack, int type, int (*apply_function)(void *element))
{
	int i;

	switch (type) {
		// 从上往下
		case ZEND_STACK_APPLY_TOPDOWN:
			for (i=stack->top-1; i>=0; i--) {
				if (apply_function(ZEND_STACK_ELEMENT(stack, i))) {
					break;
				}
			}
			break;
		// 从下往上
		case ZEND_STACK_APPLY_BOTTOMUP:
			for (i=0; i<stack->top; i++) {
				if (apply_function(ZEND_STACK_ELEMENT(stack, i))) {
					break;
				}
			}
			break;
	}
}

// ing4, 对所有元素调用带参数的函数（参数也需要在这里传入）
ZEND_API void zend_stack_apply_with_argument(zend_stack *stack, int type, int (*apply_function)(void *element, void *arg), void *arg)
{
	int i;

	switch (type) {
		// 从上往下
		case ZEND_STACK_APPLY_TOPDOWN:
			for (i=stack->top-1; i>=0; i--) {
				if (apply_function(ZEND_STACK_ELEMENT(stack, i), arg)) {
					break;
				}
			}
			break;
		// 从下往上
		case ZEND_STACK_APPLY_BOTTOMUP:
			for (i=0; i<stack->top; i++) {
				if (apply_function(ZEND_STACK_ELEMENT(stack, i), arg)) {
					break;
				}
			}
			break;
	}
}

// ing3, 对所有元素调用func, 形参本身就可以是函数！！
ZEND_API void zend_stack_clean(zend_stack *stack, void (*func)(void *), bool free_elements)
{
	int i;

	// 如果有调用的函数
	if (func) {
		// 从下向上一次调用
		for (i = 0; i < stack->top; i++) {
			func(ZEND_STACK_ELEMENT(stack, i));
		}
	}
	// 如果要求释放所有元素
	if (free_elements) {
		if (stack->elements) {
			efree(stack->elements);
			stack->elements = NULL;
		}
		stack->top = stack->max = 0;
	}
}
