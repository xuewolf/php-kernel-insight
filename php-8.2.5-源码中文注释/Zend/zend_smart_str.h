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
   +----------------------------------------------------------------------+
 */

#ifndef ZEND_SMART_STR_H
#define ZEND_SMART_STR_H

#include <zend.h>
#include "zend_globals.h"
#include "zend_smart_str_public.h"

BEGIN_EXTERN_C()

ZEND_API void ZEND_FASTCALL smart_str_erealloc(smart_str *str, size_t len);
ZEND_API void ZEND_FASTCALL smart_str_realloc(smart_str *str, size_t len);
ZEND_API void ZEND_FASTCALL smart_str_append_escaped(smart_str *str, const char *s, size_t l);
/* If zero_fraction is true, then a ".0" will be added to numbers that would not otherwise
 * have a fractional part and look like integers. */
ZEND_API void ZEND_FASTCALL smart_str_append_double(
		smart_str *str, double num, int precision, bool zero_fraction);
ZEND_API void smart_str_append_printf(smart_str *dest, const char *format, ...)
	ZEND_ATTRIBUTE_FORMAT(printf, 2, 3);
ZEND_API void ZEND_FASTCALL smart_str_append_escaped_truncated(smart_str *str, zend_string *value, size_t length);
ZEND_API void ZEND_FASTCALL smart_str_append_scalar(smart_str *str, zval *value, size_t truncate);
END_EXTERN_C()

// ing4, 给已有的 smart_str 增加空间（如果已经存在，会增加len这么多空间）
// smart_str_alloc 没改变 zend_string中记录的字串长度，所以需要自己修改字串长度
static zend_always_inline size_t smart_str_alloc(smart_str *str, size_t len, bool persistent) {
	// 如果没有分配zend_string
	if (UNEXPECTED(!str->s)) {
		// 跳到分配
		goto do_smart_str_realloc;
	// 如果已经有zend_string
	} else {
		// 把原有长度和要求长度加起来
		len += ZSTR_LEN(str->s);
		// 如果新长度大于原长度
		if (UNEXPECTED(len >= str->a)) {
// 重新分配 zend_string
do_smart_str_realloc:
			// 调整smart_str的大小，两个逻辑类型
			if (persistent) {
				smart_str_realloc(str, len);
			} else {
				smart_str_erealloc(str, len);
			}
		}
	}
	// 返回长度
	return len;
}

// ing4 , smart_str 增加长度，返回 char * 结尾位置
static zend_always_inline char* smart_str_extend_ex(smart_str *dest, size_t len, bool persistent) {
	// 增加长度
	size_t new_len = smart_str_alloc(dest, len, persistent);
	// 字串新加位置，这时串内容没变，\0的位置也没变，长度也没变？
	char *ret = ZSTR_VAL(dest->s) + ZSTR_LEN(dest->s);
	// 设置zend_string长度
	ZSTR_LEN(dest->s) = new_len;
	// 返回字串新加部分的开头位置
	return ret;
}

// ing4, smart_str 增加长度，返回 char * 结尾位置
static zend_always_inline char* smart_str_extend(smart_str *dest, size_t length)
{
	return smart_str_extend_ex(dest, length, false);
}

// ing4, 释放smart_str中的zend_string
static zend_always_inline void smart_str_free_ex(smart_str *str, bool persistent) {
	if (str->s) {
		zend_string_release_ex(str->s, persistent);
		str->s = NULL;
	}
	str->a = 0;
}

// ing4, 释放smart_str中的zend_string
static zend_always_inline void smart_str_free(smart_str *str)
{
	smart_str_free_ex(str, false);
}

// ing4, smart_str 添加结束标记
static zend_always_inline void smart_str_0(smart_str *str) {
	if (str->s) {
		ZSTR_VAL(str->s)[ZSTR_LEN(str->s)] = '\0';
	}
}

// ing4, 获取 smart_str 长度
static zend_always_inline size_t smart_str_get_len(smart_str *str) {
	return str->s ? ZSTR_LEN(str->s) : 0;
}

// ing3, 截取到 zend_string 已使用的长度
static zend_always_inline void smart_str_trim_to_size_ex(smart_str *str, bool persistent)
{
	// 如果zend_string存在，并且分配长度大于 zend_string长度
	if (str->s && str->a > ZSTR_LEN(str->s)) {
		// 调整 zend_string 大小
		str->s = zend_string_realloc(str->s, ZSTR_LEN(str->s), persistent);
		// 长度设置成 zend_string 长度
		str->a = ZSTR_LEN(str->s);
	}
}

// ing4, 截取到 zend_string允许的长度
static zend_always_inline void smart_str_trim_to_size(smart_str *dest)
{
	smart_str_trim_to_size_ex(dest, false);
}

// ing4, 返回并删除 smart_str 中的 zend_string
static zend_always_inline zend_string *smart_str_extract_ex(smart_str *str, bool persistent) {
	// 如果已分配 zend_string
	if (str->s) {
		zend_string *res;
		// 先把结尾标记好
		smart_str_0(str);
		// 
		smart_str_trim_to_size_ex(str, persistent);
		// 取出 zend_string
		res = str->s;
		// 清空 zend_string 指针
		str->s = NULL;
		// 返回zend_string
		return res;
	// 未分配，返回null
	} else {
		return ZSTR_EMPTY_ALLOC();
	}
}

// ing4, 返回并删除 smart_str 中的 zend_string
static zend_always_inline zend_string *smart_str_extract(smart_str *dest)
{
	return smart_str_extract_ex(dest, false);
}

// ing3, 向smarty_str中追加单个字符 
static zend_always_inline void smart_str_appendc_ex(smart_str *dest, char ch, bool persistent) {
	// 先增加长度
	size_t new_len = smart_str_alloc(dest, 1, persistent);
	// 然后直接写入
	ZSTR_VAL(dest->s)[new_len - 1] = ch;
	// 更新 zend_string 长度（smart_str 长度在 alloc里更新过了）
	ZSTR_LEN(dest->s) = new_len;
}

// ing4, smart_str 追加常量字串
static zend_always_inline void smart_str_appendl_ex(smart_str *dest, const char *str, size_t len, bool persistent) {
	// 先增加长度
	size_t new_len = smart_str_alloc(dest, len, persistent);
	// 直接使用内存复制，把字串复制过来
	memcpy(ZSTR_VAL(dest->s) + ZSTR_LEN(dest->s), str, len);
	// 更新长度
	ZSTR_LEN(dest->s) = new_len;
}

// ing4, smart_str 追加 zend_string
static zend_always_inline void smart_str_append_ex(smart_str *dest, const zend_string *src, bool persistent) {
	smart_str_appendl_ex(dest, ZSTR_VAL(src), ZSTR_LEN(src), persistent);
}

// ing4, smart_str追加 smart_str
static zend_always_inline void smart_str_append_smart_str_ex(smart_str *dest, const smart_str *src, bool persistent) {
	// 源smart_str必须已分配 zend_string
	if (src->s && ZSTR_LEN(src->s)) {
		// 目标实例追加 zend_string
		smart_str_append_ex(dest, src->s, persistent);
	}
}

// ing4, smart_str 追加 zend_long
static zend_always_inline void smart_str_append_long_ex(smart_str *dest, zend_long num, bool persistent) {
	// 先创建一个32位缓冲区
	char buf[32];
	// 把 zend_string 打印到缓冲区中
	char *result = zend_print_long_to_buf(buf + sizeof(buf) - 1, num);
	// 当成字串追加到smart_str里
	smart_str_appendl_ex(dest, result, buf + sizeof(buf) - 1 - result, persistent);
}

// ing4, smart_str 追加 zend_long
static zend_always_inline void smart_str_append_long(smart_str *dest, zend_long num)
{
	smart_str_append_long_ex(dest, num, false);
}

// ing4, smart_str 追加 无符号整数
static zend_always_inline void smart_str_append_unsigned_ex(smart_str *dest, zend_ulong num, bool persistent) {
	// 先创建一个32Bytes的缓冲区
	char buf[32];
	// 把 无符号整数 打印到缓冲区中
	char *result = zend_print_ulong_to_buf(buf + sizeof(buf) - 1, num);
	// 当成字串追加到smart_str里
	smart_str_appendl_ex(dest, result, buf + sizeof(buf) - 1 - result, persistent);
}

// ing4, smart_str 追加 无符号整数
static zend_always_inline void smart_str_append_unsigned(smart_str *dest, zend_ulong num)
{
	smart_str_append_unsigned_ex(dest, num, false);
}

// ing4, smart_str 追加 字串常量
static zend_always_inline void smart_str_appendl(smart_str *dest, const char *src, size_t length)
{
	smart_str_appendl_ex(dest, src, length, false);
}

// ing4, smart_str 追加 字串常量，带 persistent 标记
static zend_always_inline void smart_str_appends_ex(smart_str *dest, const char *src, bool persistent)
{
	smart_str_appendl_ex(dest, src, strlen(src), persistent);
}

// ing4, smart_str 追加 未知长度的 字串常量
static zend_always_inline void smart_str_appends(smart_str *dest, const char *src)
{
	smart_str_appendl_ex(dest, src, strlen(src), false);
}
// ing4, smart_str 追加 zend_string
static zend_always_inline void smart_str_append(smart_str *dest, const zend_string *src)
{
	smart_str_append_ex(dest, src, false);
}
// ing4, 向smarty_str中追加单个字符 
static zend_always_inline void smart_str_appendc(smart_str *dest, char ch)
{
	smart_str_appendc_ex(dest, ch, false);
}
// ing4, smart_str 追加 smart_str
static zend_always_inline void smart_str_append_smart_str(smart_str *dest, const smart_str *src)
{
	smart_str_append_smart_str_ex(dest, src, false);
}

// ing4, 释放 zend_string 后重新添加内容
static zend_always_inline void smart_str_setl(smart_str *dest, const char *src, size_t len) {
	// 释放 smart_str
	smart_str_free(dest);
	// 追加 src
	smart_str_appendl(dest, src, len);
}

// ing4, 释放 zend_string 后重新添加内容
static zend_always_inline void smart_str_sets(smart_str *dest, const char *src)
{
	smart_str_setl(dest, src, strlen(src));
}
#endif
