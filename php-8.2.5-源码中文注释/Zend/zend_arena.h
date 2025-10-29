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
   | Authors: Dmitry Stogov <dmitry@php.net>                              |
   +----------------------------------------------------------------------+
*/

#ifndef _ZEND_ARENA_H_
#define _ZEND_ARENA_H_

#include "zend.h"

#ifndef ZEND_TRACK_ARENA_ALLOC

typedef struct _zend_arena zend_arena;

// 自定义内存链。相关业务逻辑不是很多。
struct _zend_arena {
	// 内存开头和结束位置
	char		*ptr;
	char		*end;
	// 链表，指向前一个arena，它是个单向链
	zend_arena  *prev;
};

// arena 其实是一块由 zend_alloc 分配的自定义内存。
// ing4, 创建 zend_arena 对象
static zend_always_inline zend_arena* zend_arena_create(size_t size)
{
	// zend 分配内存
	zend_arena *arena = (zend_arena*)emalloc(size);
	// ZEND_MM_ALIGNED_SIZE: (size+7)&-8
	// 起始位置, 前面留出 zend_arena 的大小（对齐到 8）作为头信息。
	arena->ptr = (char*) arena + ZEND_MM_ALIGNED_SIZE(sizeof(zend_arena));
	// 结束位置，emalloc 里面也有对齐分配，所以一如既往，后面会空出一丢丢。
	arena->end = (char*) arena + size;
	// 没有前一个 arena
	arena->prev = NULL;
	// 返回新创建的 arena
	return arena;
}

// ing4, 删除 zend_arena对象链，释放内存
static zend_always_inline void zend_arena_destroy(zend_arena *arena)
{
	do {
		// 前一个的位置
		zend_arena *prev = arena->prev;
		// 删除当前实例
		efree(arena);
		// 前一个作为当前实例
		arena = prev;
	} while (arena);
}

// 分配内存：每个 arena 的大小只会比前一个大，不会比前一个小
static zend_always_inline void* zend_arena_alloc(zend_arena **arena_ptr, size_t size)
{
	zend_arena *arena = *arena_ptr;
	char *ptr = arena->ptr;
	// 大小对齐到8
	size = ZEND_MM_ALIGNED_SIZE(size);
	// 如果尺寸够用
	if (EXPECTED(size <= (size_t)(arena->end - ptr))) {
		// 指针后移
		arena->ptr = ptr + size;
	// 如果尺寸不够用
	} else {
		// 重新计算大小
		size_t arena_size =
			// 大小 + 头大小 > 整个 arena 的大小
			UNEXPECTED((size + ZEND_MM_ALIGNED_SIZE(sizeof(zend_arena))) > (size_t)(arena->end - (char*) arena)) ?
				// 大小 + 头大小（整个arena都不够用，才重新计算大小）
				(size + ZEND_MM_ALIGNED_SIZE(sizeof(zend_arena))) :
				// 整个 arena 大小 
				(size_t)(arena->end - (char*) arena);
		// 这样算出的每个 arena 的大小只会比前一个大，不会比前一个小
		// 分配内存
		zend_arena *new_arena = (zend_arena*)emalloc(arena_size);
		// 指针跳过头位置
		ptr = (char*) new_arena + ZEND_MM_ALIGNED_SIZE(sizeof(zend_arena));
		// 起始位置，跳过头位置和当前分配出去的位置
		new_arena->ptr = (char*) new_arena + ZEND_MM_ALIGNED_SIZE(sizeof(zend_arena)) + size;
		// 结束位置
		new_arena->end = (char*) new_arena + arena_size;
		// 前一个 arena
		new_arena->prev = arena;
		// 指向前一个 arena
		*arena_ptr = new_arena;
	}
	
	// 返回的是修改前的内部指针的位置
	return (void*) ptr;
}

// ing4, 带安全检测地分配内存
static zend_always_inline void* zend_arena_calloc(zend_arena **arena_ptr, size_t count, size_t unit_size)
{
	bool overflow;
	size_t size;
	void *ret;
	// 检查地址溢出
	size = zend_safe_address(unit_size, count, 0, &overflow);
	// 如果有溢出，抛错
	if (UNEXPECTED(overflow)) {
		zend_error(E_ERROR, "Possible integer overflow in zend_arena_calloc() (%zu * %zu)", unit_size, count);
	}
	// 分配内存
	ret = zend_arena_alloc(arena_ptr, size);
	// 全部写成0
	memset(ret, 0, size);
	return ret;
}

// ing4, 获取内部指针位置
static zend_always_inline void* zend_arena_checkpoint(zend_arena *arena)
{
	return arena->ptr;
}

// ing4, 释放内存，释放到 checkpoint ，后面的arena全都释放掉
static zend_always_inline void zend_arena_release(zend_arena **arena_ptr, void *checkpoint)
{
	zend_arena *arena = *arena_ptr;
	// 如果检验位置不在 arena 内
	while (UNEXPECTED((char*)checkpoint > arena->end) ||
	       UNEXPECTED((char*)checkpoint <= (char*)arena)) {
		// 转到前一个 arena
		zend_arena *prev = arena->prev;
		// 释放掉当前 arena
		efree(arena);
		// 去检查前一个 arena
		*arena_ptr = arena = prev;
	}
	// 检验位置必须在当前 arena 中
	ZEND_ASSERT((char*)checkpoint > (char*)arena && (char*)checkpoint <= arena->end);
	// 使用位置切换到 checkpoint
	arena->ptr = (char*)checkpoint;
}

// ing4, 看指针位置是否包含在 arena 中
static zend_always_inline bool zend_arena_contains(zend_arena *arena, void *ptr)
{
	// 查找所有 arena
	while (arena) {
		// 如果指针位置在 当前 area的已分配位置中间
		if ((char*)ptr > (char*)arena && (char*)ptr <= arena->ptr) {
			// 返回 true
			return 1;
		}
		// 前一个 arena
		arena = arena->prev;
	}
	// 返回 false
	return 0;
}
// 这个不会用到
#else
// 使用普通分配
// 这个是为了和 asan/valgrind 一起用
/* Use normal allocations and keep track of them for mass-freeing.
 * This is intended for use with asan/valgrind. */

typedef struct _zend_arena zend_arena;

struct _zend_arena {
	void **ptr;
	void **end;
	struct _zend_arena *prev;
	void *ptrs[0];
};

#define ZEND_TRACKED_ARENA_SIZE 1000

static zend_always_inline zend_arena *zend_arena_create(size_t _size)
{
	zend_arena *arena = (zend_arena*) emalloc(
		sizeof(zend_arena) + sizeof(void *) * ZEND_TRACKED_ARENA_SIZE);
	arena->ptr = &arena->ptrs[0];
	arena->end = &arena->ptrs[ZEND_TRACKED_ARENA_SIZE];
	arena->prev = NULL;
	return arena;
}

static zend_always_inline void zend_arena_destroy(zend_arena *arena)
{
	do {
		zend_arena *prev = arena->prev;
		void **ptr;
		for (ptr = arena->ptrs; ptr < arena->ptr; ptr++) {
			efree(*ptr);
		}
		efree(arena);
		arena = prev;
	} while (arena);
}

static zend_always_inline void *zend_arena_alloc(zend_arena **arena_ptr, size_t size)
{
	zend_arena *arena = *arena_ptr;
	if (arena->ptr == arena->end) {
		*arena_ptr = zend_arena_create(0);
		(*arena_ptr)->prev = arena;
		arena = *arena_ptr;
	}

	return *arena->ptr++ = emalloc(size);
}

static zend_always_inline void* zend_arena_calloc(zend_arena **arena_ptr, size_t count, size_t unit_size)
{
	bool overflow;
	size_t size;
	void *ret;
	// 检查地址溢出
	size = zend_safe_address(unit_size, count, 0, &overflow);
	// 如果有溢出，抛错
	if (UNEXPECTED(overflow)) {
		zend_error(E_ERROR, "Possible integer overflow in zend_arena_calloc() (%zu * %zu)", unit_size, count);
	}
	ret = zend_arena_alloc(arena_ptr, size);
	memset(ret, 0, size);
	return ret;
}

static zend_always_inline void* zend_arena_checkpoint(zend_arena *arena)
{
	return arena->ptr;
}

// 
static zend_always_inline void zend_arena_release(zend_arena **arena_ptr, void *checkpoint)
{
	while (1) {
		zend_arena *arena = *arena_ptr;
		zend_arena *prev = arena->prev;
		while (1) {
			// 到检测点，返回
			if (arena->ptr == (void **) checkpoint) {
				return;
			}
			// 到达指定长度，跳出这一层
			if (arena->ptr == arena->ptrs) {
				break;
			}
			// ？
			arena->ptr--;
			efree(*arena->ptr);
		}
		efree(arena);
		*arena_ptr = prev;
		ZEND_ASSERT(*arena_ptr);
	}
}

// todu
static zend_always_inline bool zend_arena_contains(zend_arena *arena, void *ptr)
{
	/* TODO: Dummy */
	return 1;
}

#endif

#endif /* _ZEND_ARENA_H_ */
