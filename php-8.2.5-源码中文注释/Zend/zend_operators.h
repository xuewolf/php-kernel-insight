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
   |          Dmitry Stogov <dmitry@php.net>                              |
   +----------------------------------------------------------------------+
*/

#ifndef ZEND_OPERATORS_H
#define ZEND_OPERATORS_H

#include <errno.h>
#include <math.h>
#include <assert.h>
#include <stddef.h>

#ifdef HAVE_IEEEFP_H
#include <ieeefp.h>
#endif

#include "zend_portability.h"
#include "zend_strtod.h"
#include "zend_multiply.h"
#include "zend_object_handlers.h"

#define LONG_SIGN_MASK ZEND_LONG_MIN

BEGIN_EXTERN_C()
ZEND_API zend_result ZEND_FASTCALL add_function(zval *result, zval *op1, zval *op2);
ZEND_API zend_result ZEND_FASTCALL sub_function(zval *result, zval *op1, zval *op2);
ZEND_API zend_result ZEND_FASTCALL mul_function(zval *result, zval *op1, zval *op2);
ZEND_API zend_result ZEND_FASTCALL pow_function(zval *result, zval *op1, zval *op2);
ZEND_API zend_result ZEND_FASTCALL div_function(zval *result, zval *op1, zval *op2);
ZEND_API zend_result ZEND_FASTCALL mod_function(zval *result, zval *op1, zval *op2);
ZEND_API zend_result ZEND_FASTCALL boolean_xor_function(zval *result, zval *op1, zval *op2);
ZEND_API zend_result ZEND_FASTCALL boolean_not_function(zval *result, zval *op1);
ZEND_API zend_result ZEND_FASTCALL bitwise_not_function(zval *result, zval *op1);
ZEND_API zend_result ZEND_FASTCALL bitwise_or_function(zval *result, zval *op1, zval *op2);
ZEND_API zend_result ZEND_FASTCALL bitwise_and_function(zval *result, zval *op1, zval *op2);
ZEND_API zend_result ZEND_FASTCALL bitwise_xor_function(zval *result, zval *op1, zval *op2);
ZEND_API zend_result ZEND_FASTCALL shift_left_function(zval *result, zval *op1, zval *op2);
ZEND_API zend_result ZEND_FASTCALL shift_right_function(zval *result, zval *op1, zval *op2);
ZEND_API zend_result ZEND_FASTCALL concat_function(zval *result, zval *op1, zval *op2);

ZEND_API bool ZEND_FASTCALL zend_is_identical(zval *op1, zval *op2);

ZEND_API zend_result ZEND_FASTCALL is_equal_function(zval *result, zval *op1, zval *op2);
ZEND_API zend_result ZEND_FASTCALL is_identical_function(zval *result, zval *op1, zval *op2);
ZEND_API zend_result ZEND_FASTCALL is_not_identical_function(zval *result, zval *op1, zval *op2);
ZEND_API zend_result ZEND_FASTCALL is_not_equal_function(zval *result, zval *op1, zval *op2);
ZEND_API zend_result ZEND_FASTCALL is_smaller_function(zval *result, zval *op1, zval *op2);
ZEND_API zend_result ZEND_FASTCALL is_smaller_or_equal_function(zval *result, zval *op1, zval *op2);

ZEND_API bool ZEND_FASTCALL zend_class_implements_interface(const zend_class_entry *class_ce, const zend_class_entry *interface_ce);
ZEND_API bool ZEND_FASTCALL instanceof_function_slow(const zend_class_entry *instance_ce, const zend_class_entry *ce);

// ing4, 检验继承或接口实现，单向
static zend_always_inline bool instanceof_function(
		const zend_class_entry *instance_ce, const zend_class_entry *ce) {
	return instance_ce == ce || instanceof_function_slow(instance_ce, ce);
}

/**
 检验带长度的字串是否是数字。allow_errors的值决定 需要完全是数字，或者只是前面一段是数字。
 字串最前面可以有空白。
 * Checks whether the string "str" with length "length" is numeric. The value
 * of allow_errors determines whether it's required to be entirely numeric, or
 * just its prefix. Leading whitespace is allowed.
 *
 如果字串不包含有效数字，返回0。 如果找到了一段整数，返回 IS_DOUBLE。
 如果有小数点或科学计数法e，返回 IS_DOUBLE。
 返整数从 lval，小数从 dval 返回，如果这两个指针有效。
 * The function returns 0 if the string did not contain a valid number; IS_LONG
 * if it contained a number that fits within the range of a long; or IS_DOUBLE
 * if the number was out of long range or contained a decimal point/exponent.
 * The number's value is returned into the respective pointer, *lval or *dval,
 * if that pointer is not NULL.
 *
 如果string转成数字后发生溢出，返回值也会有多种情况。
 如果比ZEND_LONG_MAX大，oflow_info 值为1，如果小于 ZEND_LONG_MIN 值为-1。
 * This variant also gives information if a string that represents an integer
 * could not be represented as such due to overflow. It writes 1 to oflow_info
 * if the integer is larger than ZEND_LONG_MAX and -1 if it's smaller than ZEND_LONG_MIN.
 */
ZEND_API zend_uchar ZEND_FASTCALL _is_numeric_string_ex(const char *str, size_t length, zend_long *lval,
	double *dval, bool allow_errors, int *oflow_info, bool *trailing_data);

ZEND_API const char* ZEND_FASTCALL zend_memnstr_ex(const char *haystack, const char *needle, size_t needle_len, const char *end);
ZEND_API const char* ZEND_FASTCALL zend_memnrstr_ex(const char *haystack, const char *needle, size_t needle_len, const char *end);

// ing3, 检测 d 是否在 long型的取值范围内
// 如果long型是4字节， (double d)
#if SIZEOF_ZEND_LONG == 4
#	define ZEND_DOUBLE_FITS_LONG(d) (!((d) > (double)ZEND_LONG_MAX || (d) < (double)ZEND_LONG_MIN))
#else
	// 当 (double)ZEND_LONG_MAX 溢出有符号区域，要用 >= 
	/* >= as (double)ZEND_LONG_MAX is outside signed range */
#	define ZEND_DOUBLE_FITS_LONG(d) (!((d) >= (double)ZEND_LONG_MAX || (d) < (double)ZEND_LONG_MIN))
#endif

//
ZEND_API zend_long ZEND_FASTCALL zend_dval_to_lval_slow(double d);

// ing3, double 型转成 zend_long 型
static zend_always_inline zend_long zend_dval_to_lval(double d)
{
	// 	# define zend_finite(a) isfinite(a)
	// 如果不是有限的，或者是null 
	if (UNEXPECTED(!zend_finite(d)) || UNEXPECTED(zend_isnan(d))) {
		// 返回 0
		return 0;
	// d 不在 long 型的取值范围内
	} else if (!ZEND_DOUBLE_FITS_LONG(d)) {
		// 兼容32位和64位系统的转换方法（负数取它和最大数的和）
		return zend_dval_to_lval_slow(d);
	}
	// 可以直接转成long，直接转
	return (zend_long)d;
}

/* Used to convert a string float to integer during an (int) cast */
// ing3, 强转成整数，不兼容时，返回最小或最大整数
static zend_always_inline zend_long zend_dval_to_lval_cap(double d)
{
	// 如果不是有限的，或者是null 
	if (UNEXPECTED(!zend_finite(d)) || UNEXPECTED(zend_isnan(d))) {
		// 返回0
		return 0;
	// 如果不兼容整数
	} else if (!ZEND_DOUBLE_FITS_LONG(d)) {
		// 返回最大或最小整数
		return (d > 0 ? ZEND_LONG_MAX : ZEND_LONG_MIN);
	}
	return (zend_long)d;
}
/* }}} */

// ing3, 一个double 和 一个zend_long 是否兼容
static zend_always_inline bool zend_is_long_compatible(double d, zend_long l) {
	return (double)l == d;
}

ZEND_API void zend_incompatible_double_to_long_error(double d);
ZEND_API void zend_incompatible_string_to_long_error(const zend_string *s);

// ing3, 安全的 double 转 long 方法，出现兼容问题会抛错。 主要给 /Zend/zend_vm_execute.h 调用
static zend_always_inline zend_long zend_dval_to_lval_safe(double d)
{
	// double 转 long ，兼容32位和64位
	zend_long l = zend_dval_to_lval(d);
	// 如果转换后发现不兼容
	if (!zend_is_long_compatible(d, l)) {
		// 抛错
		zend_incompatible_double_to_long_error(d);
	}
	return l;
}
// ing4, 检查字符是否0-9的数字
#define ZEND_IS_DIGIT(c) ((c) >= '0' && (c) <= '9')
// ing4, 检查字符是否a-f的 16进制数字
#define ZEND_IS_XDIGIT(c) (((c) >= 'A' && (c) <= 'F') || ((c) >= 'a' && (c) <= 'f'))

// ing4, 验证是否是 数字字串。传入 str字串，length长度，lval接收返回值用的整数，dval接收返回值用的小数，
static zend_always_inline zend_uchar is_numeric_string_ex(const char *str, size_t length, zend_long *lval,
	double *dval, bool allow_errors, int *oflow_info, bool *trailing_data)
{
	// 9 以后的字符不是数字，直接返回false
	if (*str > '9') {
		return 0;
	}
	// 验证是否是 数字字串。传入 str字串，length长度，lval接收返回值用的整数，dval接收返回值用的小数，
	return _is_numeric_string_ex(str, length, lval, dval, allow_errors, oflow_info, trailing_data);
}

// ing4, 检测是否是 数字字串 ^[0-9]+$
static zend_always_inline zend_uchar is_numeric_string(const char *str, size_t length, zend_long *lval, double *dval, bool allow_errors) {
	// int *oflow_info, bool *trailing_data 这两个参数是null
    return is_numeric_string_ex(str, length, lval, dval, allow_errors, NULL, NULL);
}

ZEND_API zend_uchar ZEND_FASTCALL is_numeric_str_function(const zend_string *str, zend_long *lval, double *dval);

static zend_always_inline const char *
zend_memnstr(const char *haystack, const char *needle, size_t needle_len, const char *end)
{
	const char *p = haystack;
	size_t off_s;

	ZEND_ASSERT(end >= p);

	if (needle_len == 1) {
		return (const char *)memchr(p, *needle, (end-p));
	} else if (UNEXPECTED(needle_len == 0)) {
		return p;
	}

	off_s = (size_t)(end - p);

	if (needle_len > off_s) {
		return NULL;
	}

	if (EXPECTED(off_s < 1024 || needle_len < 9)) {	/* glibc memchr is faster when needle is too short */
		const char ne = needle[needle_len-1];
		end -= needle_len;

		while (p <= end) {
			if ((p = (const char *)memchr(p, *needle, (end-p+1)))) {
				if (ne == p[needle_len-1] && !memcmp(needle+1, p+1, needle_len-2)) {
					return p;
				}
			} else {
				return NULL;
			}
			p++;
		}

		return NULL;
	} else {
		return zend_memnstr_ex(haystack, needle, needle_len, end);
	}
}

// ing3, 查字符 c 在 s 中最后出现的位置
static zend_always_inline const void *zend_memrchr(const void *s, int c, size_t n)
{
// 如果有内置函数，直接调用
#if defined(HAVE_MEMRCHR) && !defined(i386)
	/* On x86 memrchr() doesn't use SSE/AVX, so inlined version is faster */
	return (const void*)memrchr(s, c, n);
#else
	const unsigned char *e;
	if (0 == n) {
		return NULL;
	}
	// 从结尾倒着遍历回来,从 结尾位置指针，倒回到开头位置指针
	for (e = (const unsigned char *)s + n - 1; e >= (const unsigned char *)s; e--) {
		// 如果找到字符，返回位置指针
		if (*e == (unsigned char)c) {
			return (const void *)e;
		}
	}
	return NULL;
#endif
}


static zend_always_inline const char *
zend_memnrstr(const char *haystack, const char *needle, size_t needle_len, const char *end)
{
    const char *p = end;
    ptrdiff_t off_p;
    size_t off_s;

	if (needle_len == 0) {
		return p;
	}

    if (needle_len == 1) {
        return (const char *)zend_memrchr(haystack, *needle, (p - haystack));
    }

    off_p = end - haystack;
    off_s = (off_p > 0) ? (size_t)off_p : 0;

    if (needle_len > off_s) {
        return NULL;
    }

	if (EXPECTED(off_s < 1024 || needle_len < 3)) {
		const char ne = needle[needle_len-1];
		p -= needle_len;

		do {
			p = (const char *)zend_memrchr(haystack, *needle, (p - haystack) + 1);
			if (!p) {
				return NULL;
			}
			if (ne == p[needle_len-1] && !memcmp(needle + 1, p + 1, needle_len - 2)) {
				return p;
			}
		} while (p-- >= haystack);

		return NULL;
	} else {
		return zend_memnrstr_ex(haystack, needle, needle_len, end);
	}
}

ZEND_API zend_result ZEND_FASTCALL increment_function(zval *op1);
ZEND_API zend_result ZEND_FASTCALL decrement_function(zval *op2);

ZEND_API void ZEND_FASTCALL convert_scalar_to_number(zval *op);
ZEND_API void ZEND_FASTCALL _convert_to_string(zval *op);
ZEND_API void ZEND_FASTCALL convert_to_long(zval *op);
ZEND_API void ZEND_FASTCALL convert_to_double(zval *op);
ZEND_API void ZEND_FASTCALL convert_to_null(zval *op);
ZEND_API void ZEND_FASTCALL convert_to_boolean(zval *op);
ZEND_API void ZEND_FASTCALL convert_to_array(zval *op);
ZEND_API void ZEND_FASTCALL convert_to_object(zval *op);

ZEND_API zend_long    ZEND_FASTCALL zval_get_long_func(zval *op, bool is_strict);
ZEND_API double       ZEND_FASTCALL zval_get_double_func(zval *op);
ZEND_API zend_string* ZEND_FASTCALL zval_get_string_func(zval *op);
ZEND_API zend_string* ZEND_FASTCALL zval_try_get_string_func(zval *op);

// ing4, 整数直接返回，不是整数转成整数返回。非严格类型
static zend_always_inline zend_long zval_get_long(zval *op) {
	return EXPECTED(Z_TYPE_P(op) == IS_LONG) ? Z_LVAL_P(op) : zval_get_long_func(op, false);
}

// ing4, 整数直接返回，不是整数转成整数返回。
static zend_always_inline zend_long zval_get_long_ex(zval *op, bool is_strict) {
	return EXPECTED(Z_TYPE_P(op) == IS_LONG) ? Z_LVAL_P(op) : zval_get_long_func(op, is_strict);
}
// ing4, 小数直接返回，不是小数转成不数返回。
static zend_always_inline double zval_get_double(zval *op) {
	return EXPECTED(Z_TYPE_P(op) == IS_DOUBLE) ? Z_DVAL_P(op) : zval_get_double_func(op);
}
// ing4, 字串直接返回，不是字串转成不数返回。
static zend_always_inline zend_string *zval_get_string(zval *op) {
	return EXPECTED(Z_TYPE_P(op) == IS_STRING) ? zend_string_copy(Z_STR_P(op)) : zval_get_string_func(op);
}

// ing4, 运算对象转成临时字串（本身是字串则返回字串指针），p1:运算对象，p2:接收返回字串（p1字串时返回null）
static zend_always_inline zend_string *zval_get_tmp_string(zval *op, zend_string **tmp) {
	// 本身是string时
	if (EXPECTED(Z_TYPE_P(op) == IS_STRING)) {
		// tmp为空
		*tmp = NULL;
		// 返回自己的字串
		return Z_STR_P(op);
	// 本身不是string时
	} else {
		// tmp和结果都是转换成的新字串
		return *tmp = zval_get_string_func(op);
	}
}

// ing4, 释放zend_string
static zend_always_inline void zend_tmp_string_release(zend_string *tmp) {
	// 如果tmp有效
	if (UNEXPECTED(tmp)) {
		// 释放zend_string
		zend_string_release_ex(tmp, 0);
	}
}

// 和zval_get_string类似，但遇到错误时返回null，并附带exception
/* Like zval_get_string, but returns NULL if the conversion fails with an exception. */
// ing4, 转成字串（不抛错）返回
static zend_always_inline zend_string *zval_try_get_string(zval *op) {
	// 如果是字串
	if (EXPECTED(Z_TYPE_P(op) == IS_STRING)) {
		// 创建并返回副本
		zend_string *ret = zend_string_copy(Z_STR_P(op));
		ZEND_ASSUME(ret != NULL);
		return ret;
	// 不是字串
	} else {
		// 转成字串（不报错，可能附带异常）返回
		return zval_try_get_string_func(op);
	}
}

// zval_get_tmp_string，但遇到错误时返回null，并附带exception
/* Like zval_get_tmp_string, but returns NULL if the conversion fails with an exception. */
// ing4, 获取临时字串（不抛错）返回
static zend_always_inline zend_string *zval_try_get_tmp_string(zval *op, zend_string **tmp) {
	// 如果是string
	if (EXPECTED(Z_TYPE_P(op) == IS_STRING)) {
		// 取出字串，并返回
		zend_string *ret = Z_STR_P(op);
		// tmp为空
		*tmp = NULL;
		ZEND_ASSUME(ret != NULL);
		return ret;
	// 不是字串
	} else {
		// 转成字串（不报错，可能附带异常）返回
		return *tmp = zval_try_get_string_func(op);
	}
}

// convert_to_string，但返回转换是否成功，当出错时，不会修改 zval
/* Like convert_to_string(), but returns whether the conversion succeeded and does not modify the
 * zval in-place if it fails. */
// ing3, 检验zval是否可转为string
ZEND_API bool ZEND_FASTCALL _try_convert_to_string(zval *op);

// ing4, 检查是否可转成字串，string返回1，其他类型执行检查 
static zend_always_inline bool try_convert_to_string(zval *op) {
	// 是string直接返回1
	if (Z_TYPE_P(op) == IS_STRING) {
		return 1;
	}
	// 否则返回尝试结果
	return _try_convert_to_string(op);
}

// ing4, 兼容7.2和更低版本的宏
/* Compatibility macros for 7.2 and below */
#define _zval_get_long(op) zval_get_long(op)
#define _zval_get_double(op) zval_get_double(op)
#define _zval_get_string(op) zval_get_string(op)
#define _zval_get_long_func(op) zval_get_long_func(op)
#define _zval_get_double_func(op) zval_get_double_func(op)
#define _zval_get_string_func(op) zval_get_string_func(op)

// ing4, zval如果本身不是string，转换成string
#define convert_to_string(op) if (Z_TYPE_P(op) != IS_STRING) { _convert_to_string((op)); }


ZEND_API int ZEND_FASTCALL zend_is_true(zval *op);
ZEND_API bool ZEND_FASTCALL zend_object_is_true(zval *op);

// ing4, 别名 zval_is_true
#define zval_is_true(op) \
	zend_is_true(op)

// ing3, 验证是否为真
static zend_always_inline bool i_zend_is_true(zval *op)
{
	bool result = 0;

again:
	switch (Z_TYPE_P(op)) {
		// 如果类型是 is_true, 返回 1
		case IS_TRUE:
			result = 1;
			break;
		// long 型, 并且 > 0 , 返回 1
		case IS_LONG:
			if (Z_LVAL_P(op)) {
				result = 1;
			}
			break;
		// double 型, 并且 > 0 , 返回 1
		case IS_DOUBLE:
			if (Z_DVAL_P(op)) {
				result = 1;
			}
			break;
		// string 型， 长度>1 或者， (长度是1 并且内容不是 '0'), 返回 1
		case IS_STRING:
			if (Z_STRLEN_P(op) > 1 || (Z_STRLEN_P(op) && Z_STRVAL_P(op)[0] != '0')) {
				result = 1;
			}
			break;
		// array 型，不空，返回 1
		case IS_ARRAY:
			if (zend_hash_num_elements(Z_ARRVAL_P(op))) {
				result = 1;
			}
			break;
		// ？ 如果是对象 ，
		case IS_OBJECT:
			if (EXPECTED(Z_OBJ_HT_P(op)->cast_object == zend_std_cast_object_tostring)) {
				result = 1;
			} else {
				result = zend_object_is_true(op);
			}
			break;
		// 如果是资源
		case IS_RESOURCE:
			// 取得handle，就是资源编号 
			if (EXPECTED(Z_RES_HANDLE_P(op))) {
				result = 1;
			}
			break;
		// 如果是引用
		case IS_REFERENCE:
			// 找到被引用的变量
			op = Z_REFVAL_P(op);
			// 从头再来
			goto again;
			break;
		default:
			break;
	}
	// 返回 0
	return result;
}

/* Indicate that two values cannot be compared. This value should be returned for both orderings
 * of the operands. This implies that all of ==, <, <= and >, >= will return false, because we
 * canonicalize >/>= to </<= with swapped operands. */
// TODO: Use a different value to allow an actual distinction here.
#define ZEND_UNCOMPARABLE 1

ZEND_API int ZEND_FASTCALL zend_compare(zval *op1, zval *op2);

ZEND_API zend_result ZEND_FASTCALL compare_function(zval *result, zval *op1, zval *op2);

ZEND_API int ZEND_FASTCALL numeric_compare_function(zval *op1, zval *op2);
ZEND_API int ZEND_FASTCALL string_compare_function_ex(zval *op1, zval *op2, bool case_insensitive);
ZEND_API int ZEND_FASTCALL string_compare_function(zval *op1, zval *op2);
ZEND_API int ZEND_FASTCALL string_case_compare_function(zval *op1, zval *op2);
ZEND_API int ZEND_FASTCALL string_locale_compare_function(zval *op1, zval *op2);

ZEND_API extern const unsigned char zend_tolower_map[256];
ZEND_API extern const unsigned char zend_toupper_map[256];

// 大小写转换是通过map来实现的
// ing4, 单个字符转小写
#define zend_tolower_ascii(c) (zend_tolower_map[(unsigned char)(c)])
// ing4, 单个字符转大写
#define zend_toupper_ascii(c) (zend_toupper_map[(unsigned char)(c)])

ZEND_API void         ZEND_FASTCALL zend_str_tolower(char *str, size_t length);
ZEND_API void         ZEND_FASTCALL zend_str_toupper(char *str, size_t length);
ZEND_API char*        ZEND_FASTCALL zend_str_tolower_copy(char *dest, const char *source, size_t length);
ZEND_API char*        ZEND_FASTCALL zend_str_toupper_copy(char *dest, const char *source, size_t length);
ZEND_API char*        ZEND_FASTCALL zend_str_tolower_dup(const char *source, size_t length);
ZEND_API char*        ZEND_FASTCALL zend_str_toupper_dup(const char *source, size_t length);
ZEND_API char*        ZEND_FASTCALL zend_str_tolower_dup_ex(const char *source, size_t length);
ZEND_API char*        ZEND_FASTCALL zend_str_toupper_dup_ex(const char *source, size_t length);
ZEND_API zend_string* ZEND_FASTCALL zend_string_tolower_ex(zend_string *str, bool persistent);
ZEND_API zend_string* ZEND_FASTCALL zend_string_toupper_ex(zend_string *str, bool persistent);

// ing4, 字串转小写
static zend_always_inline zend_string* zend_string_tolower(zend_string *str) {
	return zend_string_tolower_ex(str, false);
}

// ing4, 字串转大写
static zend_always_inline zend_string* zend_string_toupper(zend_string *str) {
	return zend_string_toupper_ex(str, false);
}

ZEND_API int ZEND_FASTCALL zend_binary_zval_strcmp(zval *s1, zval *s2);
ZEND_API int ZEND_FASTCALL zend_binary_zval_strncmp(zval *s1, zval *s2, zval *s3);
ZEND_API int ZEND_FASTCALL zend_binary_strcmp(const char *s1, size_t len1, const char *s2, size_t len2);
ZEND_API int ZEND_FASTCALL zend_binary_strncmp(const char *s1, size_t len1, const char *s2, size_t len2, size_t length);
ZEND_API int ZEND_FASTCALL zend_binary_strcasecmp(const char *s1, size_t len1, const char *s2, size_t len2);
ZEND_API int ZEND_FASTCALL zend_binary_strncasecmp(const char *s1, size_t len1, const char *s2, size_t len2, size_t length);
ZEND_API int ZEND_FASTCALL zend_binary_strcasecmp_l(const char *s1, size_t len1, const char *s2, size_t len2);
ZEND_API int ZEND_FASTCALL zend_binary_strncasecmp_l(const char *s1, size_t len1, const char *s2, size_t len2, size_t length);

ZEND_API bool ZEND_FASTCALL zendi_smart_streq(zend_string *s1, zend_string *s2);
ZEND_API int ZEND_FASTCALL zendi_smart_strcmp(zend_string *s1, zend_string *s2);
ZEND_API int ZEND_FASTCALL zend_compare_symbol_tables(HashTable *ht1, HashTable *ht2);
ZEND_API int ZEND_FASTCALL zend_compare_arrays(zval *a1, zval *a2);
ZEND_API int ZEND_FASTCALL zend_compare_objects(zval *o1, zval *o2);

/** Deprecatd in favor of ZEND_STRTOL() */
ZEND_ATTRIBUTE_DEPRECATED ZEND_API int ZEND_FASTCALL zend_atoi(const char *str, size_t str_len);

/** Deprecatd in favor of ZEND_STRTOL() */
ZEND_ATTRIBUTE_DEPRECATED ZEND_API zend_long ZEND_FASTCALL zend_atol(const char *str, size_t str_len);

// ing4, 都是别名
#define convert_to_null_ex(zv) convert_to_null(zv)
#define convert_to_boolean_ex(zv) convert_to_boolean(zv)
#define convert_to_long_ex(zv) convert_to_long(zv)
#define convert_to_double_ex(zv) convert_to_double(zv)
#define convert_to_string_ex(zv) convert_to_string(zv)
#define convert_to_array_ex(zv) convert_to_array(zv)
#define convert_to_object_ex(zv) convert_to_object(zv)
#define convert_scalar_to_number_ex(zv) convert_scalar_to_number(zv)

ZEND_API void zend_update_current_locale(void);

ZEND_API void zend_reset_lc_ctype_locale(void);

/* The offset in bytes between the value and type fields of a zval */
#define ZVAL_OFFSETOF_TYPE	\
	(offsetof(zval, u1.type_info) - offsetof(zval, value))

#if defined(HAVE_ASM_GOTO) && !__has_feature(memory_sanitizer)
# define ZEND_USE_ASM_ARITHMETIC 1
#else
# define ZEND_USE_ASM_ARITHMETIC 0
#endif

// ing3, 整数自增
static zend_always_inline void fast_long_increment_function(zval *op1)
{
#if ZEND_USE_ASM_ARITHMETIC && defined(__i386__) && !(4 == __GNUC__ && 8 == __GNUC_MINOR__)
	__asm__ goto(
		"addl $1,(%0)\n\t"
		"jo  %l1\n"
		:
		: "r"(&op1->value)
		: "cc", "memory"
		: overflow);
	return;
overflow: ZEND_ATTRIBUTE_COLD_LABEL
	ZVAL_DOUBLE(op1, (double)ZEND_LONG_MAX + 1.0);
#elif ZEND_USE_ASM_ARITHMETIC && defined(__x86_64__)
	__asm__ goto(
		"addq $1,(%0)\n\t"
		"jo  %l1\n"
		:
		: "r"(&op1->value)
		: "cc", "memory"
		: overflow);
	return;
overflow: ZEND_ATTRIBUTE_COLD_LABEL
	ZVAL_DOUBLE(op1, (double)ZEND_LONG_MAX + 1.0);
#elif ZEND_USE_ASM_ARITHMETIC && defined(__aarch64__)
	__asm__ goto (
		"ldr x5, [%0]\n\t"
		"adds x5, x5, 1\n\t"
		"bvs %l1\n"
		"str x5, [%0]"
		:
		: "r"(&op1->value)
		: "x5", "cc", "memory"
		: overflow);
	return;
overflow: ZEND_ATTRIBUTE_COLD_LABEL
	ZVAL_DOUBLE(op1, (double)ZEND_LONG_MAX + 1.0);
#elif PHP_HAVE_BUILTIN_SADDL_OVERFLOW && SIZEOF_LONG == SIZEOF_ZEND_LONG
	long lresult;
	if (UNEXPECTED(__builtin_saddl_overflow(Z_LVAL_P(op1), 1, &lresult))) {
		/* switch to double */
		ZVAL_DOUBLE(op1, (double)ZEND_LONG_MAX + 1.0);
	} else {
		Z_LVAL_P(op1) = lresult;
	}
#elif PHP_HAVE_BUILTIN_SADDLL_OVERFLOW && SIZEOF_LONG_LONG == SIZEOF_ZEND_LONG
	long long llresult;
	if (UNEXPECTED(__builtin_saddll_overflow(Z_LVAL_P(op1), 1, &llresult))) {
		/* switch to double */
		ZVAL_DOUBLE(op1, (double)ZEND_LONG_MAX + 1.0);
	} else {
		Z_LVAL_P(op1) = llresult;
	}
// windows走这里（测试过）
#else
	// 如果是最大整数 
	if (UNEXPECTED(Z_LVAL_P(op1) == ZEND_LONG_MAX)) {
		// +1并转成小数
		/* switch to double */
		ZVAL_DOUBLE(op1, (double)ZEND_LONG_MAX + 1.0);
	// 不是最大整数 
	} else {
		// 自增
		Z_LVAL_P(op1)++;
	}
#endif
}

// ing2, 快速自减
static zend_always_inline void fast_long_decrement_function(zval *op1)
{
#if ZEND_USE_ASM_ARITHMETIC && defined(__i386__) && !(4 == __GNUC__ && 8 == __GNUC_MINOR__)
	__asm__ goto(
		"subl $1,(%0)\n\t"
		"jo  %l1\n"
		:
		: "r"(&op1->value)
		: "cc", "memory"
		: overflow);
	return;
overflow: ZEND_ATTRIBUTE_COLD_LABEL
	ZVAL_DOUBLE(op1, (double)ZEND_LONG_MIN - 1.0);
#elif ZEND_USE_ASM_ARITHMETIC && defined(__x86_64__)
	__asm__ goto(
		"subq $1,(%0)\n\t"
		"jo  %l1\n"
		:
		: "r"(&op1->value)
		: "cc", "memory"
		: overflow);
	return;
overflow: ZEND_ATTRIBUTE_COLD_LABEL
	ZVAL_DOUBLE(op1, (double)ZEND_LONG_MIN - 1.0);
#elif ZEND_USE_ASM_ARITHMETIC && defined(__aarch64__)
	__asm__ goto (
		"ldr x5, [%0]\n\t"
		"subs x5 ,x5, 1\n\t"
		"bvs %l1\n"
		"str x5, [%0]"
		:
		: "r"(&op1->value)
		: "x5", "cc", "memory"
		: overflow);
	return;
overflow: ZEND_ATTRIBUTE_COLD_LABEL
	ZVAL_DOUBLE(op1, (double)ZEND_LONG_MIN - 1.0);
#elif PHP_HAVE_BUILTIN_SSUBL_OVERFLOW && SIZEOF_LONG == SIZEOF_ZEND_LONG
	long lresult;
	if (UNEXPECTED(__builtin_ssubl_overflow(Z_LVAL_P(op1), 1, &lresult))) {
		/* switch to double */
		ZVAL_DOUBLE(op1, (double)ZEND_LONG_MIN - 1.0);
	} else {
		Z_LVAL_P(op1) = lresult;
	}
#elif PHP_HAVE_BUILTIN_SSUBLL_OVERFLOW && SIZEOF_LONG_LONG == SIZEOF_ZEND_LONG
	long long llresult;
	if (UNEXPECTED(__builtin_ssubll_overflow(Z_LVAL_P(op1), 1, &llresult))) {
		/* switch to double */
		ZVAL_DOUBLE(op1, (double)ZEND_LONG_MIN - 1.0);
	} else {
		Z_LVAL_P(op1) = llresult;
	}
// windows走这里（测试过）
#else
	// 如果是最小整数
	if (UNEXPECTED(Z_LVAL_P(op1) == ZEND_LONG_MIN)) {
		/* switch to double */
		// 转成小数再 -1
		ZVAL_DOUBLE(op1, (double)ZEND_LONG_MIN - 1.0);
	// 如果不是，直接-1
	} else {
		Z_LVAL_P(op1)--;
	}
#endif
}

// ing2, 两个整数相加
static zend_always_inline void fast_long_add_function(zval *result, zval *op1, zval *op2)
{
#if ZEND_USE_ASM_ARITHMETIC && defined(__i386__) && !(4 == __GNUC__ && 8 == __GNUC_MINOR__)
	__asm__ goto(
		"movl	(%1), %%eax\n\t"
		"addl   (%2), %%eax\n\t"
		"jo     %l5\n\t"
		"movl   %%eax, (%0)\n\t"
		"movl   %3, %c4(%0)\n"
		:
		: "r"(&result->value),
		  "r"(&op1->value),
		  "r"(&op2->value),
		  "n"(IS_LONG),
		  "n"(ZVAL_OFFSETOF_TYPE)
		: "eax","cc", "memory"
		: overflow);
	return;
overflow: ZEND_ATTRIBUTE_COLD_LABEL
	ZVAL_DOUBLE(result, (double) Z_LVAL_P(op1) + (double) Z_LVAL_P(op2));
#elif ZEND_USE_ASM_ARITHMETIC && defined(__x86_64__)
	__asm__ goto(
		"movq	(%1), %%rax\n\t"
		"addq   (%2), %%rax\n\t"
		"jo     %l5\n\t"
		"movq   %%rax, (%0)\n\t"
		"movl   %3, %c4(%0)\n"
		:
		: "r"(&result->value),
		  "r"(&op1->value),
		  "r"(&op2->value),
		  "n"(IS_LONG),
		  "n"(ZVAL_OFFSETOF_TYPE)
		: "rax","cc", "memory"
		: overflow);
	return;
overflow: ZEND_ATTRIBUTE_COLD_LABEL
	ZVAL_DOUBLE(result, (double) Z_LVAL_P(op1) + (double) Z_LVAL_P(op2));
#elif ZEND_USE_ASM_ARITHMETIC && defined(__aarch64__)
	__asm__ goto(
		"ldr    x5, [%1]\n\t"
		"ldr    x6, [%2]\n\t"
		"adds	x5, x5, x6\n\t"
		"bvs	%l5\n\t"
		"mov	w6, %3\n\t"
		"str	x5, [%0]\n\t"
		"str	w6, [%0, %c4]\n"
		:
		: "r"(&result->value),
		  "r"(&op1->value),
		  "r"(&op2->value),
		  "n"(IS_LONG),
		  "n"(ZVAL_OFFSETOF_TYPE)
		: "x5", "x6", "cc", "memory"
		: overflow);
	return;
overflow: ZEND_ATTRIBUTE_COLD_LABEL
	ZVAL_DOUBLE(result, (double) Z_LVAL_P(op1) + (double) Z_LVAL_P(op2));
#elif PHP_HAVE_BUILTIN_SADDL_OVERFLOW && SIZEOF_LONG == SIZEOF_ZEND_LONG
	long lresult;
	if (UNEXPECTED(__builtin_saddl_overflow(Z_LVAL_P(op1), Z_LVAL_P(op2), &lresult))) {
		ZVAL_DOUBLE(result, (double) Z_LVAL_P(op1) + (double) Z_LVAL_P(op2));
	} else {
		ZVAL_LONG(result, lresult);
	}
#elif PHP_HAVE_BUILTIN_SADDLL_OVERFLOW && SIZEOF_LONG_LONG == SIZEOF_ZEND_LONG
	long long llresult;
	if (UNEXPECTED(__builtin_saddll_overflow(Z_LVAL_P(op1), Z_LVAL_P(op2), &llresult))) {
		ZVAL_DOUBLE(result, (double) Z_LVAL_P(op1) + (double) Z_LVAL_P(op2));
	} else {
		ZVAL_LONG(result, llresult);
	}
// windows走这里（测试过）
#else
	// result可能是op1或op2的别名，所以需要保证读取op1和op2前，result没有变。
	/*
	 * 'result' may alias with op1 or op2, so we need to
	 * ensure that 'result' is not updated until after we
	 * have read the values of op1 and op2.
	 */
	// 如果 LONG_SIGN_MASK
	if (UNEXPECTED((Z_LVAL_P(op1) & LONG_SIGN_MASK) == (Z_LVAL_P(op2) & LONG_SIGN_MASK)
		&& (Z_LVAL_P(op1) & LONG_SIGN_MASK) != ((Z_LVAL_P(op1) + Z_LVAL_P(op2)) & LONG_SIGN_MASK))) {
		// 转成小数相加
		ZVAL_DOUBLE(result, (double) Z_LVAL_P(op1) + (double) Z_LVAL_P(op2));
	} else {
		// 整数相加
		ZVAL_LONG(result, Z_LVAL_P(op1) + Z_LVAL_P(op2));
	}
#endif
}
// ing4, 快速加法，任意类型相加
static zend_always_inline zend_result fast_add_function(zval *result, zval *op1, zval *op2)
{
	// op1是整数
	if (EXPECTED(Z_TYPE_P(op1) == IS_LONG)) {
		// op2也是整数
		if (EXPECTED(Z_TYPE_P(op2) == IS_LONG)) {
			// 快速整数加法
			fast_long_add_function(result, op1, op2);
			return SUCCESS;
		// op2是小数
		} else if (EXPECTED(Z_TYPE_P(op2) == IS_DOUBLE)) {
			// 转成小数相加
			ZVAL_DOUBLE(result, ((double)Z_LVAL_P(op1)) + Z_DVAL_P(op2));
			return SUCCESS;
		}
		// 其他情况，任意类型相加
	// op1是小数
	} else if (EXPECTED(Z_TYPE_P(op1) == IS_DOUBLE)) {
		// op2也是小数
		if (EXPECTED(Z_TYPE_P(op2) == IS_DOUBLE)) {
			// 小数相加，返回结果
			ZVAL_DOUBLE(result, Z_DVAL_P(op1) + Z_DVAL_P(op2));
			return SUCCESS;
		// op2是整数
		} else if (EXPECTED(Z_TYPE_P(op2) == IS_LONG)) {
			// 转成小数相加，返回结果 
			ZVAL_DOUBLE(result, Z_DVAL_P(op1) + ((double)Z_LVAL_P(op2)));
			return SUCCESS;
		}
		// 其他情况，任意类型相加
	}
	// 其他情况，任意类型相加
	return add_function(result, op1, op2);
}

// ing2, 快速减法。 减法只支持整数和小数，其他类型报错
static zend_always_inline void fast_long_sub_function(zval *result, zval *op1, zval *op2)
{
#if ZEND_USE_ASM_ARITHMETIC && defined(__i386__) && !(4 == __GNUC__ && 8 == __GNUC_MINOR__)
	__asm__ goto(
		"movl	(%1), %%eax\n\t"
		"subl   (%2), %%eax\n\t"
		"jo     %l5\n\t"
		"movl   %%eax, (%0)\n\t"
		"movl   %3, %c4(%0)\n"
		:
		: "r"(&result->value),
		  "r"(&op1->value),
		  "r"(&op2->value),
		  "n"(IS_LONG),
		  "n"(ZVAL_OFFSETOF_TYPE)
		: "eax","cc", "memory"
		: overflow);
	return;
overflow: ZEND_ATTRIBUTE_COLD_LABEL
	ZVAL_DOUBLE(result, (double) Z_LVAL_P(op1) - (double) Z_LVAL_P(op2));
#elif ZEND_USE_ASM_ARITHMETIC && defined(__x86_64__)
	__asm__ goto(
		"movq	(%1), %%rax\n\t"
		"subq   (%2), %%rax\n\t"
		"jo     %l5\n\t"
		"movq   %%rax, (%0)\n\t"
		"movl   %3, %c4(%0)\n"
		:
		: "r"(&result->value),
		  "r"(&op1->value),
		  "r"(&op2->value),
		  "n"(IS_LONG),
		  "n"(ZVAL_OFFSETOF_TYPE)
		: "rax","cc", "memory"
		: overflow);
	return;
overflow: ZEND_ATTRIBUTE_COLD_LABEL
	ZVAL_DOUBLE(result, (double) Z_LVAL_P(op1) - (double) Z_LVAL_P(op2));
#elif ZEND_USE_ASM_ARITHMETIC && defined(__aarch64__)
	__asm__ goto(
		"ldr    x5, [%1]\n\t"
		"ldr    x6, [%2]\n\t"
		"subs	x5, x5, x6\n\t"
		"bvs	%l5\n\t"
		"mov	w6, %3\n\t"
		"str	x5, [%0]\n\t"
		"str	w6, [%0, %c4]\n"
		:
		: "r"(&result->value),
		  "r"(&op1->value),
		  "r"(&op2->value),
		  "n"(IS_LONG),
		  "n"(ZVAL_OFFSETOF_TYPE)
		: "x5", "x6", "cc", "memory"
		: overflow);
	return;
overflow: ZEND_ATTRIBUTE_COLD_LABEL
	ZVAL_DOUBLE(result, (double) Z_LVAL_P(op1) - (double) Z_LVAL_P(op2));
#elif PHP_HAVE_BUILTIN_SSUBL_OVERFLOW && SIZEOF_LONG == SIZEOF_ZEND_LONG
	long lresult;
	if (UNEXPECTED(__builtin_ssubl_overflow(Z_LVAL_P(op1), Z_LVAL_P(op2), &lresult))) {
		ZVAL_DOUBLE(result, (double) Z_LVAL_P(op1) - (double) Z_LVAL_P(op2));
	} else {
		ZVAL_LONG(result, lresult);
	}
#elif PHP_HAVE_BUILTIN_SSUBLL_OVERFLOW && SIZEOF_LONG_LONG == SIZEOF_ZEND_LONG
	long long llresult;
	if (UNEXPECTED(__builtin_ssubll_overflow(Z_LVAL_P(op1), Z_LVAL_P(op2), &llresult))) {
		ZVAL_DOUBLE(result, (double) Z_LVAL_P(op1) - (double) Z_LVAL_P(op2));
	} else {
		ZVAL_LONG(result, llresult);
	}
// 默认逻辑,windows走这里（测试过）
#else
	// 直接按整数减
	ZVAL_LONG(result, Z_LVAL_P(op1) - Z_LVAL_P(op2));
	// ?? 如果取整后两个数不同 并且 取整后op1 结果也不同 
	if (UNEXPECTED((Z_LVAL_P(op1) & LONG_SIGN_MASK) != (Z_LVAL_P(op2) & LONG_SIGN_MASK)
		&& (Z_LVAL_P(op1) & LONG_SIGN_MASK) != (Z_LVAL_P(result) & LONG_SIGN_MASK))) {
		// 按小数算减法
		ZVAL_DOUBLE(result, (double) Z_LVAL_P(op1) - (double) Z_LVAL_P(op2));
	}
#endif
}

// ing4, 快速比较两个字串是否相等
static zend_always_inline bool zend_fast_equal_strings(zend_string *s1, zend_string *s2)
{
	// 先比较指针，相同返回true
	if (s1 == s2) {
		return 1;
	// 有一个不是数字
	} else if (ZSTR_VAL(s1)[0] > '9' || ZSTR_VAL(s2)[0] > '9') {
		// 按字串比较
		return zend_string_equal_content(s1, s2);
	// 都是数字开头
	} else {
		// 先按数字比较，无效再按字串比较
		return zendi_smart_streq(s1, s2);
	}
}

// ing4, 快速比较相等
static zend_always_inline bool fast_equal_check_function(zval *op1, zval *op2)
{
	// op1是整数
	if (EXPECTED(Z_TYPE_P(op1) == IS_LONG)) {
		// op2也是整数
		if (EXPECTED(Z_TYPE_P(op2) == IS_LONG)) {
			// 直接比较
			return Z_LVAL_P(op1) == Z_LVAL_P(op2);
		// op2是小数
		} else if (EXPECTED(Z_TYPE_P(op2) == IS_DOUBLE)) {
			// 转成小数比较
			return ((double)Z_LVAL_P(op1)) == Z_DVAL_P(op2);
		}
		// 其他情况，按任意类型比较
	// op1是小数
	} else if (EXPECTED(Z_TYPE_P(op1) == IS_DOUBLE)) {
		// op2是小数
		if (EXPECTED(Z_TYPE_P(op2) == IS_DOUBLE)) {
			// 直接比较
			return Z_DVAL_P(op1) == Z_DVAL_P(op2);
		// op2是整数
		} else if (EXPECTED(Z_TYPE_P(op2) == IS_LONG)) {
			// op2转成小数比较
			return Z_DVAL_P(op1) == ((double)Z_LVAL_P(op2));
		}
		// 其他情况，按任意类型比较
	// op1是字串
	} else if (EXPECTED(Z_TYPE_P(op1) == IS_STRING)) {
		// op2也是字串
		if (EXPECTED(Z_TYPE_P(op2) == IS_STRING)) {
			// 按字串比较
			return zend_fast_equal_strings(Z_STR_P(op1), Z_STR_P(op2));
		}
		// 其他情况，按任意类型比较
	}
	// 其他情况，按任意类型比较
	return zend_compare(op1, op2) == 0;
}

// ing3，检查整数是否相等
static zend_always_inline bool fast_equal_check_long(zval *op1, zval *op2)
{
	// 如果op2是整数 （why？）
	if (EXPECTED(Z_TYPE_P(op2) == IS_LONG)) {
		// 直接比较
		return Z_LVAL_P(op1) == Z_LVAL_P(op2);
	}
	// 任意类型比较
	return zend_compare(op1, op2) == 0;
}

// ing3，检查字串是否相等
static zend_always_inline bool fast_equal_check_string(zval *op1, zval *op2)
{
	// 如果op2是字串 （why？）
	if (EXPECTED(Z_TYPE_P(op2) == IS_STRING)) {
		return zend_fast_equal_strings(Z_STR_P(op1), Z_STR_P(op2));
	}
	// 任意类型比较
	return zend_compare(op1, op2) == 0;
}

// ing3，检查是否相同
static zend_always_inline bool fast_is_identical_function(zval *op1, zval *op2)
{
	// 类型不一样就不用比了
	if (Z_TYPE_P(op1) != Z_TYPE_P(op2)) {
		// 返回不同
		return 0;
	// 类型一样， op1：true,false,null,UNDEF（why ??）
	} else if (Z_TYPE_P(op1) <= IS_TRUE) {
		// 返回相同
		return 1;
	}
	// 任意类型比较相同
	return zend_is_identical(op1, op2);
}

// ing3，检查是否 不相同
static zend_always_inline bool fast_is_not_identical_function(zval *op1, zval *op2)
{
	// 类型不同，返回true
	if (Z_TYPE_P(op1) != Z_TYPE_P(op2)) {
		return 1;
	// op1：true,false,null,UNDEF（why ??）
	} else if (Z_TYPE_P(op1) <= IS_TRUE) {
		// 返回false
		return 0;
	}
	// 任意类型比较相同，取否
	return !zend_is_identical(op1, op2);
}

// buf，指针在 buffer 结尾。原来如此。
/* buf points to the END of the buffer */
// ing4, 无符号长整型， zend_ulong 转 string
static zend_always_inline char *zend_print_ulong_to_buf(char *buf, zend_ulong num) {
	// 先添加终止字符
	*buf = '\0';
	do {
		// 添加最后一位数字，buf指针前移
		*--buf = (char) (num % 10) + '0';
		num /= 10; // 下一位数字
	// 直到数字用完
	} while (num > 0);
	return buf;
}

// buf，指针在 buffer 结尾。
/* buf points to the END of the buffer */
// ing4, 有符号长整型， zend_ulong 转 string
static zend_always_inline char *zend_print_long_to_buf(char *buf, zend_long num) {
	// 负数
	if (num < 0) {
	    char *result = zend_print_ulong_to_buf(buf, ~((zend_ulong) num) + 1);
		// 最后在前面加个负号
	    *--result = '-';
		return result;
	// 正数
	} else {
	    return zend_print_ulong_to_buf(buf, num);
	}
}

ZEND_API zend_string* ZEND_FASTCALL zend_long_to_str(zend_long num);
ZEND_API zend_string* ZEND_FASTCALL zend_ulong_to_str(zend_ulong num);
ZEND_API zend_string* ZEND_FASTCALL zend_u64_to_str(uint64_t num);
ZEND_API zend_string* ZEND_FASTCALL zend_i64_to_str(int64_t num);
ZEND_API zend_string* ZEND_FASTCALL zend_double_to_str(double num);

// ing4, 解包引用：p1引用数为1？p1赋值为未定义：减少p1引用次数，p1引用目标复制给p1
static zend_always_inline void zend_unwrap_reference(zval *op) /* {{{ */
{
	// 如果只有一次引用，更新成未定义
	if (Z_REFCOUNT_P(op) == 1) {
		// 赋值为未定义
		ZVAL_UNREF(op);
	// 否则
	} else {
		// 减少引用次数（因为当前操作不再引用它了）
		Z_DELREF_P(op);
		// 引用目标复制给当前变量
		ZVAL_COPY(op, Z_REFVAL_P(op));
	}
}
/* }}} */

// ing4, 字串转成小写，比较不同
static zend_always_inline bool zend_strnieq(const char *ptr1, const char *ptr2, size_t num)
{
	const char *end = ptr1 + num;
	// 逐个比较
	while (ptr1 < end) {
		// 转成小写后有一个不同
		if (zend_tolower_ascii(*ptr1++) != zend_tolower_ascii(*ptr2++)) {
			// 返回否
			return 0;
		}
	}
	// 返回true
	return 1;
}

static zend_always_inline const char *
zend_memnistr(const char *haystack, const char *needle, size_t needle_len, const char *end)
{
	ZEND_ASSERT(end >= haystack);

	if (UNEXPECTED(needle_len == 0)) {
		return haystack;
	}

	if (UNEXPECTED(needle_len > (size_t)(end - haystack))) {
		return NULL;
	}
	
	const char first_lower = zend_tolower_ascii(*needle);
	const char first_upper = zend_toupper_ascii(*needle);
	const char *p_lower = (const char *)memchr(haystack, first_lower, end - haystack);
	const char *p_upper = NULL;
	if (first_lower != first_upper) {
		// If the needle length is 1 we don't need to look beyond p_lower as it is a guaranteed match
		size_t upper_search_length = end - (needle_len == 1 && p_lower != NULL ? p_lower : haystack);
		p_upper = (const char *)memchr(haystack, first_upper, upper_search_length);
	}
	const char *p = !p_upper || (p_lower && p_lower < p_upper) ? p_lower : p_upper;

	if (needle_len == 1) {
		return p;
	}

	const char needle_end_lower = zend_tolower_ascii(needle[needle_len - 1]);
	const char needle_end_upper = zend_toupper_ascii(needle[needle_len - 1]);
	end -= needle_len;

	while (p && p <= end) {
		if (needle_end_lower == p[needle_len - 1] || needle_end_upper == p[needle_len - 1]) {
			if (zend_strnieq(needle + 1, p + 1, needle_len - 2)) {
				return p;
			}
		}
		if (p_lower == p) {
			p_lower = (const char *)memchr(p_lower + 1, first_lower, end - p_lower);
		}
		if (p_upper == p) {
			p_upper = (const char *)memchr(p_upper + 1, first_upper, end - p_upper);
		}
		p = !p_upper || (p_lower && p_lower < p_upper) ? p_lower : p_upper;
	}

	return NULL;
}


END_EXTERN_C()

#endif
