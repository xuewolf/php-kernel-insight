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
#include "zend_ptr_stack.h"
#include <stdarg.h>

// 这是个最简单的指针堆栈，语法解析时用到

// ing3, 初始化 堆栈
ZEND_API void zend_ptr_stack_init_ex(zend_ptr_stack *stack, bool persistent)
{
	stack->top_element = stack->elements = NULL;
	stack->top = stack->max = 0;
	stack->persistent = persistent;
}

// ing4, 初始化 堆栈，非持久
ZEND_API void zend_ptr_stack_init(zend_ptr_stack *stack)
{
	zend_ptr_stack_init_ex(stack, 0);
}

// ing4, 压入n个元素
ZEND_API void zend_ptr_stack_n_push(zend_ptr_stack *stack, int count, ...)
{
	va_list ptr;
	void *elem;
	// 如果需要，增加内存
	ZEND_PTR_STACK_RESIZE_IF_NEEDED(stack, count)

	va_start(ptr, count);
	// 遍历每一个
	while (count>0) {
		// 取得指针
		elem = va_arg(ptr, void *);
		// 高度 +1
		stack->top++;
		// 把指针放进去
		*(stack->top_element++) = elem;
		// 未处理数量 -1
		count--;
	}
	va_end(ptr);
}

// ing4, 弹出末尾n个元素
ZEND_API void zend_ptr_stack_n_pop(zend_ptr_stack *stack, int count, ...)
{
	va_list ptr;
	void **elem;
	// 按指定数量 获取接收列表
	va_start(ptr, count);
	// 按序号遍历每个接收变量
	while (count>0) {
		// 取得接收变量
		elem = va_arg(ptr, void **);
		// 接收弹出的元素
		*elem = *(--stack->top_element);
		// 高度-1
		stack->top--;
		// 未处理数量 -1
		count--;
	}
	va_end(ptr);
}


// ing4, 销毁 堆栈 元素列表
ZEND_API void zend_ptr_stack_destroy(zend_ptr_stack *stack)
{
	// 如果有元素列表
	if (stack->elements) {
		// 释放元素列表
		pefree(stack->elements, stack->persistent);
	}
}

// ing4, 从上到下对每个元素调用function 
ZEND_API void zend_ptr_stack_apply(zend_ptr_stack *stack, void (*func)(void *))
{
	int i = stack->top;
	// 遍历每个元素
	while (--i >= 0) {
		// 调用function
		func(stack->elements[i]);
	}
}

// ing4, 从下到上对每个元素调用function 
ZEND_API void zend_ptr_stack_reverse_apply(zend_ptr_stack *stack, void (*func)(void *))
{
	int i = 0;
	// 倒序遍历每个元素
	while (i < stack->top) {
		// 调用function
		func(stack->elements[i++]);
	}
}

// ing4, 清理哈希表
ZEND_API void zend_ptr_stack_clean(zend_ptr_stack *stack, void (*func)(void *), bool free_elements)
{
	// 对每个元素调用 func
	zend_ptr_stack_apply(stack, func);
	// 如果需要释放
	if (free_elements) {
		int i = stack->top;
		// 倒序
		while (--i >= 0) {
			pefree(stack->elements[i], stack->persistent);
		}
	}
	// 高度为0
	stack->top = 0;
	// 顶指针批到列表开头
	stack->top_element = stack->elements;
}


// ing4, 获取 堆栈高度
ZEND_API int zend_ptr_stack_num_elements(zend_ptr_stack *stack)
{
	return stack->top;
}
