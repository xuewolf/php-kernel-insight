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
   | Author: Dmitry Stogov <dmitry@php.net>                               |
   +----------------------------------------------------------------------+
 */

#include <zend.h>
#include "zend_smart_str.h"
#include "zend_smart_string.h"

// ing4,zend_string 结构体中 val 的偏移量
// ZEND_MM_OVERHEAD 生产环境为 0, _ZSTR_HEADER_SIZE 是 zend_string 结构体中 val 的偏移量
#define SMART_STR_OVERHEAD   (ZEND_MM_OVERHEAD + _ZSTR_HEADER_SIZE + 1)
#define SMART_STR_START_SIZE 256
// ing4, 256个字节的zend_string中，实际用来存储char的长度
#define SMART_STR_START_LEN  (SMART_STR_START_SIZE - SMART_STR_OVERHEAD)
// 每个smart_str页的大小
#define SMART_STR_PAGE       4096

// clear, 计算smart_str长度，取得最小的的能存下 len 的 4096 的倍数，然后减掉 （zend_string中val前面的长度）
#define SMART_STR_NEW_LEN(len) \
	(ZEND_MM_ALIGNED_SIZE_EX(len + SMART_STR_OVERHEAD, SMART_STR_PAGE) - SMART_STR_OVERHEAD)

// ing4, 调整smart_str的大小
ZEND_API void ZEND_FASTCALL smart_str_erealloc(smart_str *str, size_t len)
{
	// 如果未创建 zend_string
	if (UNEXPECTED(!str->s)) {
		// 计算长度
		str->a = len <= SMART_STR_START_LEN
				? SMART_STR_START_LEN
				: SMART_STR_NEW_LEN(len);
		// 创建 zend_string
		str->s = zend_string_alloc(str->a, 0);
		// 长度设置成 0
		ZSTR_LEN(str->s) = 0;
	// 如果已有 zend_string
	} else {
		// 重新计算长度
		str->a = SMART_STR_NEW_LEN(len);
		// 调整zend_string 大小
		str->s = (zend_string *) erealloc2(str->s, str->a + _ZSTR_HEADER_SIZE + 1, _ZSTR_HEADER_SIZE + ZSTR_LEN(str->s));
	}
}

// ing4, 调整smart_str的大小
ZEND_API void ZEND_FASTCALL smart_str_realloc(smart_str *str, size_t len)
{
	// 如果没有 zend_string 实例
	if (UNEXPECTED(!str->s)) {
		// 计算可用来存储字串的长度
		str->a = len <= SMART_STR_START_LEN
				? SMART_STR_START_LEN
				: SMART_STR_NEW_LEN(len);
		str->s = zend_string_alloc(str->a, 1);
		ZSTR_LEN(str->s) = 0;
	// 如果已有 zend_string
	} else {
		// 重新计算长度
		str->a = SMART_STR_NEW_LEN(len);
		// 调整zend_string 大小。
		// perealloc 调整大小后，zend_string中记录的字串长度没变，\0的位置也没变，所以要在后面设置长度。
		str->s = (zend_string *) perealloc(str->s, str->a + _ZSTR_HEADER_SIZE + 1, 1);
	}
}

// windows 操作系统使用 VK_ESCAPE 代替 \e
/* Windows uses VK_ESCAPE instead of \e */
#ifndef VK_ESCAPE
#define VK_ESCAPE '\e'
#endif

// ing4, 计算转义后需要增加的长度
static size_t zend_compute_escaped_string_len(const char *s, size_t l) {
	size_t i, len = l;
	// 遍历每个字符
	for (i = 0; i < l; ++i) {
		char c = s[i];
		// 需要转义的字符，加一个字符 
		if (c == '\n' || c == '\r' || c == '\t' ||
			c == '\f' || c == '\v' || c == '\\' || c == VK_ESCAPE) {
			len += 1;
		// 不可见字符，加3个字符，（转成\x开头的16进制字串，共占4字节，所以要加3个字符 ）
		} else if (c < 32 || c > 126) {
			len += 3;
		}
	}
	// 返回长度
	return len;
}

// ing4, 把原字串转义后放进 smart_str 里
ZEND_API void ZEND_FASTCALL smart_str_append_escaped(smart_str *str, const char *s, size_t l) {
	char *res;
	// len ,转义后的长度
	size_t i, len = zend_compute_escaped_string_len(s, l);
	// 重新分配空间
	smart_str_alloc(str, len, 0);
	// 先找到最后一个字符
	res = &ZSTR_VAL(str->s)[ZSTR_LEN(str->s)];
	// 先增加长度
	ZSTR_LEN(str->s) += len;
	// 正序遍历 原字串中 每一个字符 
	for (i = 0; i < l; ++i) {
		// 从第一个字符开始
		unsigned char c = s[i];
		// 如果当前字符是控制字符 或 反斜线（\）
		if (c < 32 || c == '\\' || c > 126) {
			// res位置放一个 \，然后 smart_str 指针右移
			*res++ = '\\';
			// 根据当前字符进行操作
			switch (c) {
				// 如果是需要转义的字符，写入转义字符，然后 smart_str 指针右移
				case '\n': *res++ = 'n'; break;
				case '\r': *res++ = 'r'; break;
				case '\t': *res++ = 't'; break;
				case '\f': *res++ = 'f'; break;
				case '\v': *res++ = 'v'; break;
				case '\\': *res++ = '\\'; break;
				case VK_ESCAPE: *res++ = 'e'; break;
				// 如果是控制字符，转成\x开头的16进制字串，共占4字节，所以要加3个字符 
				default:
					// 先写一个x，凑成\x，指针右移
					*res++ = 'x';
					// c >> 4 得到左面4个bit, 如果 < 10
					if ((c >> 4) < 10) {
						// 写数字，然后 smart_str 指针右移
						*res++ = (c >> 4) + '0';
					// 如果 >= 10
					} else {
						// 写A-F，然后 smart_str 指针右移
						*res++ = (c >> 4) + 'A' - 10;
					}
					// 如果右面4个bit , 如果 < 10
					if ((c & 0xf) < 10) {
						// 写数字，然后 smart_str 指针右移
						*res++ = (c & 0xf) + '0';
					// 如果 >=0
					} else {
						// 写A-F，然后 smart_str 指针右移
						*res++ = (c & 0xf) + 'A' - 10;
					}
			}
		// 如果当前字符是普通字符
		} else {
			// 把它写入当前位置，然后 smart_str 指针右移
			*res++ = c;
		}
	}
}

// ing3, smart_str后面追加 浮点数
// precision,精度， zero_fraction是否把整数显示成小数，添加.0
ZEND_API void ZEND_FASTCALL smart_str_append_double(
		smart_str *str, double num, int precision, bool zero_fraction) {
	// 小数最大长度1080
	// define ZEND_DOUBLE_MAX_LENGTH 1080
	char buf[ZEND_DOUBLE_MAX_LENGTH];
	// 打印精确值
	/* Model snprintf precision behavior. */
	// 把小数转成 char 型（处理小数是非常复杂的事情）
	// 把不数打印打buffer里
	zend_gcvt(num, precision ? precision : 1, '.', 'E', buf);
	// 把buffer里的字串追加到smart_str后面
	smart_str_appends(str, buf);
	// 如果需要在整数后面补.0,并且当前是整数
	if (zero_fraction && zend_finite(num) && !strchr(buf, '.')) {
		// 补.0
		smart_str_appendl(str, ".0", 2);
	}
}

// ing3, smart_str 追加 printf
ZEND_API void smart_str_append_printf(smart_str *dest, const char *format, ...) {
	va_list arg;
	va_start(arg, format);
	// php_printf_to_smart_str 的别名
	zend_printf_to_smart_str(dest, format, arg);
	va_end(arg);
}

// ZEND_MM_OVERHEAD 生产环境为 0, SMART_STRING_OVERHEAD = 1，就是 \0
#define SMART_STRING_OVERHEAD   (ZEND_MM_OVERHEAD + 1)
// 初始长度256
#define SMART_STRING_START_SIZE 256
// SMART_STRING_START_LEN = 256-1 = 255
#define SMART_STRING_START_LEN  (SMART_STRING_START_SIZE - SMART_STRING_OVERHEAD)
// 每个页大小4096，4k
#define SMART_STRING_PAGE       4096

// ing4, 分配 smart_string， 注意 smart_string 不是 smart_str
ZEND_API void ZEND_FASTCALL _smart_string_alloc_persistent(smart_string *str, size_t len)
{
	// 如果没分配字串空间
	if (!str->c) {
		// 已用长度是0
		str->len = 0;
		// <= 255
		if (len <= SMART_STRING_START_LEN) {
			// 占用的长度是255
			str->a = SMART_STRING_START_LEN;
		// > 255
		} else {
			// 用户的长度是 4096*n - 1
			str->a = ZEND_MM_ALIGNED_SIZE_EX(len + SMART_STRING_OVERHEAD, SMART_STRING_PAGE) - SMART_STRING_OVERHEAD;
		}
		// 分配内存空间，多一个字节是 \0
		str->c = pemalloc(str->a + 1, 1);
	// 如果已经分配了内存空间，添加长度
	} else {
		// 如果要求大小大于 最大size，报错
		// #define SIZE_MAX	((size_t)~0)
		if (UNEXPECTED((size_t) len > SIZE_MAX - str->len)) {
			zend_error(E_ERROR, "String size overflow");
		}
		// 添加长度
		len += str->len;
		// 计算长度
		str->a = ZEND_MM_ALIGNED_SIZE_EX(len + SMART_STRING_OVERHEAD, SMART_STRING_PAGE) - SMART_STRING_OVERHEAD;
		// 重新分配内存
		str->c = perealloc(str->c, str->a + 1, 1);
	}
}

// ing4, 分配smart_string，注意 smart_string 不是 smart_str
ZEND_API void ZEND_FASTCALL _smart_string_alloc(smart_string *str, size_t len)
{
	// 如果还没有分配zend_string
	if (!str->c) {
		// 使用长度为0
		str->len = 0;
		// #define SMART_STRING_START_LEN (SMART_STRING_START_SIZE - SMART_STRING_OVERHEAD)
		// 如果要求的长度小于初始长度
		if (len <= SMART_STRING_START_LEN) {
			// 可用长度
			str->a = SMART_STRING_START_LEN;
			// 普通string 分配内存时要多占 1 字节,\0
			str->c = emalloc(SMART_STRING_START_LEN + 1);
		// 如果要求的长度小于初始长度
		} else {
			// 长度对齐到 4096 ，减掉头大小
			str->a = ZEND_MM_ALIGNED_SIZE_EX(len + SMART_STRING_OVERHEAD, SMART_STRING_PAGE) - SMART_STRING_OVERHEAD;
			// 小于2M - SMART_STRING_OVERHEAD
			if (EXPECTED(str->a < (ZEND_MM_CHUNK_SIZE - SMART_STRING_OVERHEAD))) {
				// 分配大块，多分配一个 \0
				str->c = emalloc_large(str->a + 1);
			// 大于2M
			} else {
				// 分配超大块
				/* allocate a huge chunk */
				// 多分配一个 \0
				str->c = emalloc(str->a + 1);
			}
		}
	// 如果已经分配了字串空间
	} else {
		// 如果长度超限
		if (UNEXPECTED((size_t) len > SIZE_MAX - str->len)) {
			// 报错：字串溢出
			zend_error(E_ERROR, "String size overflow");
		}
		// 更新长度
		len += str->len;
		// 计算大小
		str->a = ZEND_MM_ALIGNED_SIZE_EX(len + SMART_STRING_OVERHEAD, SMART_STRING_PAGE) - SMART_STRING_OVERHEAD;
		// 重新分配内存
		str->c = erealloc2(str->c, str->a + 1, str->len);
	}
}

// ing4, smart_str 添加转义后的字串，如果截断，添加截断标记
ZEND_API void ZEND_FASTCALL smart_str_append_escaped_truncated(smart_str *str, zend_string *value, size_t length)
{
	// 添加转义后的字串
	smart_str_append_escaped(str, ZSTR_VAL(value), MIN(length, ZSTR_LEN(value)));
	// 如果长度超限制 
	if (ZSTR_LEN(value) > length) {
		// 添加截断标记
		smart_str_appendl(str, "...", sizeof("...")-1);
	}
}

// ing4, smart_str 追加标量
ZEND_API void ZEND_FASTCALL smart_str_append_scalar(smart_str *dest, zval *value, size_t truncate) {
	// value 的类型必须在 IS_STRING 或以前
	ZEND_ASSERT(Z_TYPE_P(value) <= IS_STRING);

	// 根据类型处理
	switch (Z_TYPE_P(value)) {
		// undefined 或 null 都添加 NULL
		case IS_UNDEF:
		case IS_NULL:
			// -1 是减掉\0
			smart_str_appendl(dest, "NULL", sizeof("NULL")-1);
		break;
		// true 和 false 
		case IS_TRUE:
		case IS_FALSE:
			smart_str_appends(dest, Z_TYPE_P(value) == IS_TRUE ? "true" : "false");
		break;
		// double类型，追加小数转成的字串，精度用全局设置
		case IS_DOUBLE:
			smart_str_append_double(dest, Z_DVAL_P(value), (int) EG(precision), true);
		break;
		// 整形，追加整数转成的字串
		case IS_LONG:
			smart_str_append_long(dest, Z_LVAL_P(value));
		break;
		// 字串
		case IS_STRING:
			// 前后添加单引号'
			smart_str_appendc(dest, '\'');
			// 把转义过的字串放在引号中间
			smart_str_append_escaped_truncated(dest, Z_STR_P(value), truncate);
			smart_str_appendc(dest, '\'');
		break;

		EMPTY_SWITCH_DEFAULT_CASE();
	}
}
