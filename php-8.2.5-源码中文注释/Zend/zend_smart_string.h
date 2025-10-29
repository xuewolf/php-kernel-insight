/*
   +----------------------------------------------------------------------+
   | Copyright (c) The PHP Group                                          |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | https://www.php.net/license/3_01.txt                                 |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Author: Sascha Schumann <sascha@schumann.cx>                         |
   |         Xinchen Hui <laruence@php.net>                               |
   +----------------------------------------------------------------------+
 */

#ifndef PHP_SMART_STRING_H
#define PHP_SMART_STRING_H

#include "zend_smart_string_public.h"

#include <stdlib.h>
#include <zend.h>

// 这是个简单版本的 smart_string
// 封装
/* wrapper */

// ing4, 创建或追加 长度为len的字串， what是持久分配
#define smart_string_appends_ex(str, src, what) \
	/* 创建或追加一个 len 长度，并把给定的字串放进去 */ \
	smart_string_appendl_ex((str), (src), strlen(src), (what))
	
// ing4, 创建或追加字串，非持久分配
#define smart_string_appends(str, src) \
	smart_string_appendl((str), (src), strlen(src))
	
// ing4, 创建或追加已有的 smart_string ，what是持久分配
#define smart_string_append_ex(str, src, what) \
	/* 创建或追加一个 len 长度，并把给定的字串放进去 */ \
	smart_string_appendl_ex((str), ((smart_string *)(src))->c, \
		((smart_string *)(src))->len, (what));

// ing4, 手工分配字串给 smart_string		
#define smart_string_sets(str, src) \
	smart_string_setl((str), (src), strlen(src));

// ing4, 创建或追加 1 byte，并把char放进去，非持久分配
#define smart_string_appendc(str, c) \
	smart_string_appendc_ex((str), (c), 0)

// ing4, 释放 smart_string 中的字串，不销毁自身	
#define smart_string_free(s) \
	smart_string_free_ex((s), 0)

// ing4, 创建或追加 长度为len的字串，非持久分配
#define smart_string_appendl(str, src, len) \
	/* 创建或追加一个 len 长度，并把给定的字串放进去 */ \
	smart_string_appendl_ex((str), (src), (len), 0)
	
// ing4, 创建或追加已有的 smart_string，非持久分配
#define smart_string_append(str, src) \
	smart_string_append_ex((str), (src), 0)
	
// ing4, 追加整数 
#define smart_string_append_long(str, val) \
	smart_string_append_long_ex((str), (val), 0)
	
// ing4, 追加无符号整数 
#define smart_string_append_unsigned(str, val) \
	smart_string_append_unsigned_ex((str), (val), 0)

// zend_smart_str.c 中实现
ZEND_API void ZEND_FASTCALL _smart_string_alloc_persistent(smart_string *str, size_t len);
// zend_smart_str.c 中实现
ZEND_API void ZEND_FASTCALL _smart_string_alloc(smart_string *str, size_t len);

// ing4, 分配 smart_string，注意 smart_string 不是 smart_str
static zend_always_inline size_t smart_string_alloc(smart_string *str, size_t len, bool persistent) {
	// 如果没有分配字串 或 空间不够用
	if (UNEXPECTED(!str->c) || UNEXPECTED(len >= str->a - str->len)) {
		// 持久
		if (persistent) {
			// 持久分配 smart_string
			_smart_string_alloc_persistent(str, len);
		// 非持久分配
		} else {
			// 分配smart_string
			_smart_string_alloc(str, len);
		}
	}
	// 返回已使用长度，不是总长度
	return str->len + len;
}

// ing4, 释放 smart_string 中的字串，不销毁自身
static zend_always_inline void smart_string_free_ex(smart_string *str, bool persistent) {
	// 如果有字串
	if (str->c) {
		// 释放字串
		pefree(str->c, persistent);
		// 清空指针
		str->c = NULL;
	}
	// 长度为0
	str->a = str->len = 0;
}

// ing4, 添加结束字串 \0 
static zend_always_inline void smart_string_0(smart_string *str) {
	if (str->c) {
		str->c[str->len] = '\0';
	}
}

// ing4, 创建或追加 1 byte，并把char放进去
static zend_always_inline void smart_string_appendc_ex(smart_string *dest, char ch, bool persistent) {
	// 按需要创建或追加长度为 1 
	dest->len = smart_string_alloc(dest, 1, persistent);
	// 直接放进去
	dest->c[dest->len - 1] = ch;
}

// ing4, 创建或追加一个 len 长度，并把给定的字串放进去
static zend_always_inline void smart_string_appendl_ex(smart_string *dest, const char *str, size_t len, bool persistent) {
	// 按需要创建或追加长度为 len 
	size_t new_len = smart_string_alloc(dest, len, persistent);
	// 复制字串
	memcpy(dest->c + dest->len, str, len);
	// 更新已使用长度
	dest->len = new_len;

}

// ing4, 追加整型字串
static zend_always_inline void smart_string_append_long_ex(smart_string *dest, zend_long num, bool persistent) {
	char buf[32];
	// 整数打印进临时字串里
	char *result = zend_print_long_to_buf(buf + sizeof(buf) - 1, num);
	// 追加到 smart_string 后面
	smart_string_appendl_ex(dest, result, buf + sizeof(buf) - 1 - result, persistent);
}

// ing4, 追加无符号整型字串
static zend_always_inline void smart_string_append_unsigned_ex(smart_string *dest, zend_ulong num, bool persistent) {
	char buf[32];
	// 整数打印进临时字串里
	char *result = zend_print_ulong_to_buf(buf + sizeof(buf) - 1, num);
	// 追加到 smart_string 后面
	smart_string_appendl_ex(dest, result, buf + sizeof(buf) - 1 - result, persistent);
}

// ing4, 手工分配字串给 smart_string
static zend_always_inline void smart_string_setl(smart_string *dest, char *src, size_t len) {
	// 更新长度
	dest->len = len;
	// 更新总长度
	dest->a = len + 1;
	// 更新指针
	dest->c = src;
}

// ing4, 重置 smart_string
static zend_always_inline void smart_string_reset(smart_string *str) {
	str->len = 0;
}

#endif
