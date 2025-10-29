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
   | Authors: Anatol Belski <ab@php.net>                                  |
   +----------------------------------------------------------------------+
*/

// ing3, 一些内置函数要自己测试一遍

#ifndef ZEND_LONG_H
#define ZEND_LONG_H

#include <inttypes.h>
#include <stdint.h>
// 这个是 zval里 所有 int64 实现的 核心
/* This is the heart of the whole int64 enablement in zval. */
// 如果是各种64位系统
#if defined(__x86_64__) || defined(__LP64__) || defined(_LP64) || defined(_WIN64)
# define ZEND_ENABLE_ZVAL_LONG64 1
#endif

// int 类型
/* Integer types. */
// 如果是64位（一般都是64位了，暂时忽略非64位）
#ifdef ZEND_ENABLE_ZVAL_LONG64
// 长整型
typedef int64_t zend_long;
// ulong 是无符号类型
typedef uint64_t zend_ulong;
// ?
typedef int64_t zend_off_t;
// 以下这几个是系统参数
# define ZEND_LONG_MAX INT64_MAX
# define ZEND_LONG_MIN INT64_MIN
# define ZEND_ULONG_MAX UINT64_MAX
// 转成64位 整数
# define Z_L(i) INT64_C(i)
// 转成64位 无符号整数
# define Z_UL(i) UINT64_C(i)
// 8字节
# define SIZEOF_ZEND_LONG 8

// 如果不是64位
#else
typedef int32_t zend_long;
typedef uint32_t zend_ulong;
typedef int32_t zend_off_t;
# define ZEND_LONG_MAX INT32_MAX
# define ZEND_LONG_MIN INT32_MIN
# define ZEND_ULONG_MAX UINT32_MAX
# define Z_L(i) INT32_C(i)
# define Z_UL(i) UINT32_C(i)
# define SIZEOF_ZEND_LONG 4
#endif

// 转换宏
/* Conversion macros. */
#define ZEND_LTOA_BUF_LEN 65

// 这部分的内置函数需要测试
// 允许64位
#ifdef ZEND_ENABLE_ZVAL_LONG64
# define ZEND_LONG_FMT "%" PRId64
# define ZEND_ULONG_FMT "%" PRIu64
# define ZEND_XLONG_FMT "%" PRIx64
# define ZEND_LONG_FMT_SPEC PRId64
# define ZEND_ULONG_FMT_SPEC PRIu64
// 如果是windows系统 
# ifdef ZEND_WIN32
// ？
#  define ZEND_LTOA(i, s, len) _i64toa_s((i), (s), (len), 10)
// ing4, 系统函数，字串转 10进制 整数。p1:字串
#  define ZEND_ATOL(s) _atoi64((s))
// ing4, 系统函数，字串转有符号整数。p1:字串，p2:返回，使用的结尾位置，p3:进制数（测试过）
#  define ZEND_STRTOL(s0, s1, base) _strtoi64((s0), (s1), (base))
// ing4, 系统函数，字串转无符号整数。p1:字串，p2:返回，使用的结尾位置，p3:进制数（测试过）
#  define ZEND_STRTOUL(s0, s1, base) _strtoui64((s0), (s1), (base))
// ing4, _strtoi64 函数别名
#  define ZEND_STRTOL_PTR _strtoi64
// ing4, _strtoui64 函数别名
#  define ZEND_STRTOUL_PTR _strtoui64
// ing4, _abs64 函数别名，取绝对值（测试过）
#  define ZEND_ABS _abs64
// 不是windows系统 
# else
#  define ZEND_LTOA(i, s, len) \
	do { \
		int st = snprintf((s), (len), ZEND_LONG_FMT, (i)); \
		(s)[st] = '\0'; \
 	} while (0)
#  define ZEND_ATOL(s) atoll((s))
#  define ZEND_STRTOL(s0, s1, base) strtoll((s0), (s1), (base))
#  define ZEND_STRTOUL(s0, s1, base) strtoull((s0), (s1), (base))
#  define ZEND_STRTOL_PTR strtoll
#  define ZEND_STRTOUL_PTR strtoull
#  define ZEND_ABS imaxabs
# endif
// 不允许 64位
#else
# define ZEND_STRTOL(s0, s1, base) strtol((s0), (s1), (base))
# define ZEND_STRTOUL(s0, s1, base) strtoul((s0), (s1), (base))
# define ZEND_LONG_FMT "%" PRId32
# define ZEND_ULONG_FMT "%" PRIu32
# define ZEND_XLONG_FMT "%" PRIx32
# define ZEND_LONG_FMT_SPEC PRId32
# define ZEND_ULONG_FMT_SPEC PRIu32
# ifdef ZEND_WIN32
#  define ZEND_LTOA(i, s, len) _ltoa_s((i), (s), (len), 10)
#  define ZEND_ATOL(s) atol((s))
# else
#  define ZEND_LTOA(i, s, len) \
	do { \
		int st = snprintf((s), (len), ZEND_LONG_FMT, (i)); \
		(s)[st] = '\0'; \
 	} while (0)
#  define ZEND_ATOL(s) atol((s))
# endif
# define ZEND_STRTOL_PTR strtol
# define ZEND_STRTOUL_PTR strtoul
# define ZEND_ABS abs
#endif
//

// 32位
#if SIZEOF_ZEND_LONG == 4
# define MAX_LENGTH_OF_LONG 11
# define LONG_MIN_DIGITS "2147483648"

// 64位
#elif SIZEOF_ZEND_LONG == 8
// 最大整数长度
# define MAX_LENGTH_OF_LONG 20
// 最大整数，字串
# define LONG_MIN_DIGITS "9223372036854775808"

// 其他长度，直接报错，不支持
#else
# error "Unknown SIZEOF_ZEND_LONG"
#endif

// 最大整数，字串
static const char long_min_digits[] = LONG_MIN_DIGITS;

// 32位
#if SIZEOF_SIZE_T == 4
// 地址格式8个字符
# define ZEND_ADDR_FMT "0x%08zx"
// 64位系统，地址长度16个字符
#elif SIZEOF_SIZE_T == 8
# define ZEND_ADDR_FMT "0x%016zx"
// 其他长度，直接报错
#else
# error "Unknown SIZEOF_SIZE_T"
#endif

#endif /* ZEND_LONG_H */
