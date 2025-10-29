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
#include "zend_llist.h"
#include "zend_sort.h"

// ing3, 初始化列表
ZEND_API void zend_llist_init(zend_llist *l, size_t size, llist_dtor_func_t dtor, unsigned char persistent)
{
	// 头尾指针
	l->head  = NULL;
	l->tail  = NULL;
	// 数量
	l->count = 0;
	// 大小
	l->size  = size;
	// 销毁器
	l->dtor  = dtor;
	// 永久？
	l->persistent = persistent;
}

// ing4, 向列表中添加元素，元素可以是任何类型
ZEND_API void zend_llist_add_element(zend_llist *l, const void *element)
{
	// 创建一个element，
	// 大小的计算：zend_llist_element 大小 + 需要的大小（zend_llist->size） -1（因为zend_llist_element里面已经有一个char）
	zend_llist_element *tmp = pemalloc(sizeof(zend_llist_element)+l->size-1, l->persistent);
	// 新元素的前一个指向列表结尾
	tmp->prev = l->tail;
	// 新元素的下一个，不存在
	tmp->next = NULL;
	// 如果结尾元素存在
	if (l->tail) {
		// 结尾元素的下一个，指向新元素
		l->tail->next = tmp;
	// 如果结尾元素不存在
	} else {
		// 头原素指针指向新元素
		l->head = tmp;
	}
	// 尾元素指针一定指向新元素
	l->tail = tmp;
	// 把传入的数据复制到新元素里
	memcpy(tmp->data, element, l->size);
	// 元素数量 +1
	++l->count;
}

// ing4, 在列表开头插入元素
ZEND_API void zend_llist_prepend_element(zend_llist *l, const void *element)
{
	zend_llist_element *tmp = pemalloc(sizeof(zend_llist_element)+l->size-1, l->persistent);

	tmp->next = l->head;
	tmp->prev = NULL;
	// 如果头原素存在
	if (l->head) {
		// 头原素的前一个指向新元素
		l->head->prev = tmp;
	// 如果头原素不存在
	} else {
		// 尾元素指针指向新元素
		l->tail = tmp;
	}
	// 头原素指针一定指向本元素
	l->head = tmp;
	// 把传入的数据复制到新元素里
	memcpy(tmp->data, element, l->size);
	// 元素数量 +1
	++l->count;
}

// ing4, 列表中删除一个元素
// 先把自己从列表里摘出来，
// 如果有有dtor方法，调用dtor方法清除 element->data, 
// 然后销毁 element
// 元素数 -1
#define DEL_LLIST_ELEMENT(current, l) \
			if ((current)->prev) {\
				(current)->prev->next = (current)->next;\
			} else {\
				(l)->head = (current)->next;\
			}\
			if ((current)->next) {\
				(current)->next->prev = (current)->prev;\
			} else {\
				(l)->tail = (current)->prev;\
			}\
			if ((l)->dtor) {\
				(l)->dtor((current)->data);\
			}\
			pefree((current), (l)->persistent);\
			--l->count;

// ing4, 删除元素，传入列表，元素指针，和一个比对函数的指针（此函数返回int型）
ZEND_API void zend_llist_del_element(zend_llist *l, void *element, int (*compare)(void *element1, void *element2))
{
	// 从头开始
	zend_llist_element *current=l->head;
	// 如果当前元素存在
	while (current) {
		// 如果比对结果是同一个元素
		if (compare(current->data, element)) {
			// 删除这个元素
			DEL_LLIST_ELEMENT(current, l);
			break;
		}
		// 下一个
		current = current->next;
	}
}

// ing4, 销毁整个列表（不释放内存）
ZEND_API void zend_llist_destroy(zend_llist *l)
{
	// 从头开始
	zend_llist_element *current=l->head, *next;

	// 遍历每一个，逐个删除
	while (current) {
		// 先把下一个的指针保存好
		next = current->next;
		// 如果有销毁器，销毁元素中的数据 
		if (l->dtor) {
			l->dtor(current->data);
		}
		// 释放元素
		pefree(current, l->persistent);
		// 当前指针切换到下一个元素
		current = next;
	}

	// 清空指针，元素数量 归零
	l->head  = NULL;
	l->tail  = NULL;
	l->count = 0;
}

// ing4, 清空整个列表，销毁所有元素
ZEND_API void zend_llist_clean(zend_llist *l)
{
	zend_llist_destroy(l);
	l->head = l->tail = NULL;
}

// ing4, 删除1个尾元素
ZEND_API void zend_llist_remove_tail(zend_llist *l)
{
	// 取得尾元素
	zend_llist_element *old_tail = l->tail;
	// 如果没有，返回
	if (!old_tail) {
		return;
	}

	// 如果尾元素不是头元素
	if (old_tail->prev) {
		// 清空指向尾元素的指针
		old_tail->prev->next = NULL;
	// 如果尾元素是头原素
	} else {
		// 清空头指针
		l->head = NULL;
	}

	// 修改尾指针
	l->tail = old_tail->prev;
	// 数量 -1
	--l->count;

	// 如果有销毁方法
	if (l->dtor) {
		// 调用销毁望海潮清空元素内容
		l->dtor(old_tail->data);
	}
	// 释放内存
	pefree(old_tail, l->persistent);
}

// ing4, 复制整个列表
ZEND_API void zend_llist_copy(zend_llist *dst, zend_llist *src)
{
	zend_llist_element *ptr;
	// 初始化新列表
	zend_llist_init(dst, src->size, src->dtor, src->persistent);
	// 从源列表头原素开始
	ptr = src->head;
	// 遍历每个元素
	while (ptr) {
		// 复制到新列表中
		zend_llist_add_element(dst, ptr->data);
		// 下一个元素
		ptr = ptr->next;
	}
}

// ing4, 这是个带删除功能的 array_walk，
// 对每个元素执行func，如果成功，删除这个元素
ZEND_API void zend_llist_apply_with_del(zend_llist *l, int (*func)(void *data))
{
	zend_llist_element *element, *next;
	// 从头开始遍历整个列表
	element=l->head;
	while (element) {
		next = element->next;
		// 对每个元素执行 func，如果返回true，删除这个元素
		if (func(element->data)) {
			DEL_LLIST_ELEMENT(element, l);
		}
		// 下一个
		element = next;
	}
}

// ing4, 这个是array_walk ，对每个元素执行 func
ZEND_API void zend_llist_apply(zend_llist *l, llist_apply_func_t func)
{
	zend_llist_element *element;
	
	for (element=l->head; element; element=element->next) {
		func(element->data);
	}
}

// ing4, 交换两个元素的内存位置。但两个元素本身没有变。 这个要小心，防止元素中的指针被错误应用。
static void zend_llist_swap(zend_llist_element **p, zend_llist_element **q)
{
	zend_llist_element *t;
	t = *p;
	*p = *q;
	*q = t;
}

// ing4, 用传入的方法进行排序，usort
ZEND_API void zend_llist_sort(zend_llist *l, llist_compare_func_t comp_func)
{
	size_t i;

	zend_llist_element **elements;
	zend_llist_element *element, **ptr;
	// 如果是空列表，直接返回
	if (l->count == 0) {
		return;
	}
	// 创建一组临时指针
	elements = (zend_llist_element **) emalloc(l->count * sizeof(zend_llist_element *));
	// 临时指针的开头
	ptr = &elements[0];

	// 遍历所有列表元素
	for (element=l->head; element; element=element->next) {
		// 把所有元素的指针放到指针列表中
		*ptr++ = element;
	}

	// 对临时指针列表进行排序，用 zend_llist_swap 函数做交换方法
	zend_sort(elements, l->count, sizeof(zend_llist_element *),
			(compare_func_t) comp_func, (swap_func_t) zend_llist_swap);

	// 排序后，更新头原素 指针
	l->head = elements[0];
	// 头原素的 prev 指针置空
	elements[0]->prev = NULL;
	// 把整个链表按新顺序 重新串进来
	for (i = 1; i < l->count; i++) {
		elements[i]->prev = elements[i-1];
		elements[i-1]->next = elements[i];
	}
	// 结尾元素的 next 指针置空
	elements[i-1]->next = NULL;
	// 更新尾元素指针
	l->tail = elements[i-1];
	// 销毁临时列表
	efree(elements);
}

// ing4, 一个附加参数的array_walk
ZEND_API void zend_llist_apply_with_argument(zend_llist *l, llist_apply_with_arg_func_t func, void *arg)
{
	zend_llist_element *element;
	// 遍历列表中每个元素
	for (element=l->head; element; element=element->next) {
		// 带附加参数的调用
		func(element->data, arg);
	}
}

// ing4, 带 任意多个附加参数的array_walk
ZEND_API void zend_llist_apply_with_arguments(zend_llist *l, llist_apply_with_args_func_t func, int num_args, ...)
{
	zend_llist_element *element;
	va_list args;
	// 取得任意多个参数
	va_start(args, num_args);
	// 遍历列表中每个元素
	for (element=l->head; element; element=element->next) {
		// 带附加参数的调用
		func(element->data, num_args, args);
	}
	va_end(args);
}

// clear, 取得列表的长度
ZEND_API size_t zend_llist_count(zend_llist *l)
{
	return l->count;
}

// ing3, 取得头原素的内容，并返回头原素的指针，pos的意义是什么？
ZEND_API void *zend_llist_get_first_ex(zend_llist *l, zend_llist_position *pos)
{
	// zend_llist_position 是指向 element的指针
	// current 是游标或返回位置
	zend_llist_position *current = pos ? pos : &l->traverse_ptr;
	// 游标或返回位置，指向头原素
	*current = l->head;
	// 如果有指向元素，返回元素的内容
	if (*current) {
		return (*current)->data;
	} else {
		return NULL;
	}
}

// ing3, 取得尾原素的内容，并返回尾原素的指针
ZEND_API void *zend_llist_get_last_ex(zend_llist *l, zend_llist_position *pos)
{
	zend_llist_position *current = pos ? pos : &l->traverse_ptr;

	*current = l->tail;
	if (*current) {
		return (*current)->data;
	} else {
		return NULL;
	}
}

// ing3, 取得下一个元素的内容，并返回下一个元素的指针
ZEND_API void *zend_llist_get_next_ex(zend_llist *l, zend_llist_position *pos)
{
	zend_llist_position *current = pos ? pos : &l->traverse_ptr;

	if (*current) {
		*current = (*current)->next;
		if (*current) {
			return (*current)->data;
		}
	}
	return NULL;
}

// ing3, 取得前一个元素的内容，并返回前一个元素的指针
ZEND_API void *zend_llist_get_prev_ex(zend_llist *l, zend_llist_position *pos)
{
	zend_llist_position *current = pos ? pos : &l->traverse_ptr;

	if (*current) {
		*current = (*current)->prev;
		if (*current) {
			return (*current)->data;
		}
	}
	return NULL;
}
