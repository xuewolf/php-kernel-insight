// 只有一些是给 compile ，一些是给 excute 服务的模块比较多

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

#include <ctype.h>

#include "zend.h"
#include "zend_operators.h"
#include "zend_variables.h"
#include "zend_globals.h"
#include "zend_list.h"
#include "zend_API.h"
#include "zend_strtod.h"
#include "zend_exceptions.h"
#include "zend_closures.h"

#include <locale.h>
#ifdef HAVE_LANGINFO_H
# include <langinfo.h>
#endif

// window没有这个东西
#ifdef __SSE2__
#include <emmintrin.h>
#endif

// windows 满足这个条件
#if defined(ZEND_WIN32) && !defined(ZTS) && defined(_MSC_VER)
/* This performance improvement of tolower() on Windows gives 10-18% on bench.php */
// 这会让 windows 的 tolower() 性能提升 10-18%
#define ZEND_USE_TOLOWER_L 1
#endif

// windows有这个
#ifdef ZEND_USE_TOLOWER_L
static _locale_t current_locale = NULL;
/* this is true global! may lead to strange effects on ZTS, but so may setlocale() */
#define zend_tolower(c) _tolower_l(c, current_locale)
// 没有 __SSE2__
#else
// ing4, 转小写
#define zend_tolower(c) tolower(c)
#endif

// ing4, 把两个type放在一个整数里，用于组合比较。t1 左移动4 位 | t2
#define TYPE_PAIR(t1,t2) (((t1) << 4) | (t2))

// window没有这个东西
#if __SSE2__
// windows没有这个东西，大小小写转换用到
#define HAVE_BLOCKCONV

/* Common code for SSE2 accelerated character case conversion */

#define BLOCKCONV_INIT_RANGE(start, end) \
	const __m128i blconv_start_minus_1 = _mm_set1_epi8((start) - 1); \
	const __m128i blconv_end_plus_1 = _mm_set1_epi8((end) + 1);

#define BLOCKCONV_STRIDE sizeof(__m128i)

#define BLOCKCONV_INIT_DELTA(delta) \
	const __m128i blconv_delta = _mm_set1_epi8(delta);

#define BLOCKCONV_LOAD(input) \
	__m128i blconv_operand = _mm_loadu_si128((__m128i*)(input)); \
	__m128i blconv_gt = _mm_cmpgt_epi8(blconv_operand, blconv_start_minus_1); \
	__m128i blconv_lt = _mm_cmplt_epi8(blconv_operand, blconv_end_plus_1); \
	__m128i blconv_mingle = _mm_and_si128(blconv_gt, blconv_lt);

#define BLOCKCONV_FOUND() _mm_movemask_epi8(blconv_mingle)

#define BLOCKCONV_STORE(dest) \
	__m128i blconv_add = _mm_and_si128(blconv_mingle, blconv_delta); \
	__m128i blconv_result = _mm_add_epi8(blconv_operand, blconv_add); \
	_mm_storeu_si128((__m128i *)(dest), blconv_result);

#endif /* __SSE2__ */

// 字符转小写
ZEND_API const unsigned char zend_tolower_map[256] = {
0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
0x40,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x5b,0x5c,0x5d,0x5e,0x5f,
0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x7b,0x7c,0x7d,0x7e,0x7f,
0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,
0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,
0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf,
0xb0,0xb1,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xbb,0xbc,0xbd,0xbe,0xbf,
0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf,
0xd0,0xd1,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xdb,0xdc,0xdd,0xde,0xdf,
0xe0,0xe1,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xeb,0xec,0xed,0xee,0xef,
0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa,0xfb,0xfc,0xfd,0xfe,0xff
};

// 字符转大写
ZEND_API const unsigned char zend_toupper_map[256] = {
0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,
0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x5b,0x5c,0x5d,0x5e,0x5f,
0x60,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,
0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x7b,0x7c,0x7d,0x7e,0x7f,
0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,
0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,
0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf,
0xb0,0xb1,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xbb,0xbc,0xbd,0xbe,0xbf,
0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf,
0xd0,0xd1,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xdb,0xdc,0xdd,0xde,0xdf,
0xe0,0xe1,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xeb,0xec,0xed,0xee,0xef,
0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa,0xfb,0xfc,0xfd,0xfe,0xff
};


/**
// 使用本地小写字符的函数，有 _l 结尾
 * Functions using locale lowercase:
 	 	zend_binary_strncasecmp_l
 	 	zend_binary_strcasecmp_l

// 使用ascii小写字符的函数
 * Functions using ascii lowercase:
		string_compare_function_ex
		string_case_compare_function
  		zend_str_tolower_copy
		zend_str_tolower_dup
		zend_str_tolower
		zend_binary_strcasecmp
		zend_binary_strncasecmp
 */

// ing4，字串转整数，结尾是 g G m M k K 的 *1024
static zend_long ZEND_FASTCALL zend_atol_internal(const char *str, size_t str_len) /* {{{ */
{
	// 如果没传长度
	if (!str_len) {
		// 先计算长度
		str_len = strlen(str);
	}
	// 这样计算乘法 可避免 UB 的溢出。新的溢出静默并忽略，其他可能发生的情况未知
	// 特别是函数的最终结果用于 无符号上下文时
	// （例如 "memory_limit=3G" ，会在int32类型里溢出，但不会在size_t类型溢出）
	/* Perform following multiplications on unsigned to avoid overflow UB.
	 * For now overflow is silently ignored -- not clear what else can be
	 * done here, especially as the final result of this function may be
	 * used in an unsigned context (e.g. "memory_limit=3G", which overflows
	 * zend_long on 32-bit, but not size_t). */
	 
	// 先把 str转成 整数
	zend_ulong retval = (zend_ulong) ZEND_STRTOL(str, NULL, 0);
	// 如果有长度
	if (str_len>0) {
		// 获取最后一个字符
		switch (str[str_len-1]) {
			case 'g':
			case 'G':
				retval *= 1024;
				// # define ZEND_FALLTHROUGH ((void)0)
				ZEND_FALLTHROUGH;
			case 'm':
			case 'M':
				retval *= 1024;
				// # define ZEND_FALLTHROUGH ((void)0)
				ZEND_FALLTHROUGH;
			case 'k':
			case 'K':
				retval *= 1024;
				break;
		}
	}
	// 
	return (zend_long) retval;
}
/* }}} */

// ing4，字串转整数，结尾是 g G m M k K 的 *1024，返回类型 zend_long
ZEND_API zend_long ZEND_FASTCALL zend_atol(const char *str, size_t str_len)
{
	// ing4，字串转整数，结尾是 g G m M k K 的 *1024
	return zend_atol_internal(str, str_len);
}

// ing4，字串转整数，结尾是 g G m M k K 的 *1024，返回类型 int
ZEND_API int ZEND_FASTCALL zend_atoi(const char *str, size_t str_len)
{
	// ing4，字串转整数，结尾是 g G m M k K 的 *1024
	return (int) zend_atol_internal(str, str_len);
}

// ing4, 对象转换到指定类型，结果可能是 ctype 或 未定义
/* {{{ convert_object_to_type: dst will be either ctype or UNDEF */
#define convert_object_to_type(op, dst, ctype)									\
	/* 目标设置成未定义 */ \
	ZVAL_UNDEF(dst);																		\
	/* 调用 cast_object方法，如果失败 */ \
	if (Z_OBJ_HT_P(op)->cast_object(Z_OBJ_P(op), dst, ctype) == FAILURE) {					\
		/* 报错：类型转换失败 */ \
		zend_error(E_WARNING,																\
			"Object of class %s could not be converted to %s", ZSTR_VAL(Z_OBJCE_P(op)->name),\
		zend_get_type_by_const(ctype));														\
	} 																						\

/* }}} */

// ing4, 把op里的对象转成整数 ，清空op，整数返回到op里
ZEND_API void ZEND_FASTCALL convert_scalar_to_number(zval *op) /* {{{ */
{
try_again:
	switch (Z_TYPE_P(op)) {
		// 引用类型
		case IS_REFERENCE:
			// 追踪到引用目标
			zend_unwrap_reference(op);
			goto try_again;
		// 字串
		case IS_STRING:
			{
				zend_string *str;
				// 找到 zend_string 指针
				str = Z_STR_P(op);
				// 尝试把字串转成整数或小数，并把结果返回给op，同时更新op的类型
				if ((Z_TYPE_INFO_P(op)=is_numeric_string(ZSTR_VAL(str), ZSTR_LEN(str), &Z_LVAL_P(op), &Z_DVAL_P(op), 1)) == 0) {
					// 如果失败，值为为
					ZVAL_LONG(op, 0);
				}
				// 释放字串
				zend_string_release_ex(str, 0);
				break;
			}
		// null和false转成0
		case IS_NULL:
		case IS_FALSE:
			ZVAL_LONG(op, 0);
			break;
		// true 转成1
		case IS_TRUE:
			ZVAL_LONG(op, 1);
			break;
		// 资源类型
		case IS_RESOURCE:
			{
				// 取得handle，就是资源编号 
				zend_long l = Z_RES_HANDLE_P(op);
				// 销毁op
				zval_ptr_dtor(op);
				// 返回handle
				ZVAL_LONG(op, l);
			}
			break;
		// 对象类型
		case IS_OBJECT:
			{
				zval dst;
				// 把对象转成数字
				convert_object_to_type(op, &dst, _IS_NUMBER);
				// 销毁对象
				zval_ptr_dtor(op);
				// 如果结果是 整数 或小数
				if (Z_TYPE(dst) == IS_LONG || Z_TYPE(dst) == IS_DOUBLE) {
					// 结果给op
					ZVAL_COPY_VALUE(op, &dst);
				// 结果不是整数或小数
				} else {
					// op的值为1
					ZVAL_LONG(op, 1);
				}
			}
			break;
	}
}
/* }}} */

// ing3, 标量转换成数字，结果放在 holder 里，全程不报错，不转换数组，对象转换失败时返回1。
static zend_never_inline zval* ZEND_FASTCALL _zendi_convert_scalar_to_number_silent(zval *op, zval *holder) /* {{{ */
{
	// 按操作对象类型划分
	switch (Z_TYPE_P(op)) {
		// null, false 转成 0
		case IS_NULL:
		case IS_FALSE:
			ZVAL_LONG(holder, 0);
			return holder;
		// true 转成1
		case IS_TRUE:
			ZVAL_LONG(holder, 1);
			return holder;
		//  string
		case IS_STRING:
			// 转成数值，写入holder，转换类型也入hoder
			if ((Z_TYPE_INFO_P(holder) = is_numeric_string(Z_STRVAL_P(op), Z_STRLEN_P(op), &Z_LVAL_P(holder), &Z_DVAL_P(holder), 1)) == 0) {
				// 如果失败，返回值为0
				ZVAL_LONG(holder, 0);
			}
			// 都引用返回了，还又返回一次
			return holder;
		// 资源
		case IS_RESOURCE:
			// 找到溯源编号，返回给holder
			ZVAL_LONG(holder, Z_RES_HANDLE_P(op));
			return holder;
		// 对象
		case IS_OBJECT:
			// op转成数字
			convert_object_to_type(op, holder, _IS_NUMBER);
			// 如果有异常 或 ( 返回类型不是 整数 也不是 小数 )
			if (UNEXPECTED(EG(exception)) ||
			    UNEXPECTED(Z_TYPE_P(holder) != IS_LONG && Z_TYPE_P(holder) != IS_DOUBLE)) {
				// 值为1
				ZVAL_LONG(holder, 1);
			}
			return holder;
		// 整数，小数和其他类型，不转换
		case IS_LONG:
		case IS_DOUBLE:
		default:
			return op;
	}
}
/* }}} */

// ing3, 标量转成数字（结果可能是整数或小数）
static zend_never_inline zend_result ZEND_FASTCALL _zendi_try_convert_scalar_to_number(zval *op, zval *holder) /* {{{ */
{
	// 
	switch (Z_TYPE_P(op)) {
		// null 和 false 转成 整数 0
		case IS_NULL:
		case IS_FALSE:
			ZVAL_LONG(holder, 0);
			return SUCCESS;
		// true 转成整数 1
		case IS_TRUE:
			ZVAL_LONG(holder, 1);
			return SUCCESS;
		// 字串
		case IS_STRING:
		{
			// 没有尾数据
			bool trailing_data = false;
			// 因为BC原因，允许异常，可以用其他方式报错
			/* For BC reasons we allow errors so that we can warn on leading numeric string */
			
			// op转成数字，放在holder里，并更新holder类型
			if (0 == (Z_TYPE_INFO_P(holder) = is_numeric_string_ex(Z_STRVAL_P(op), Z_STRLEN_P(op),
					&Z_LVAL_P(holder), &Z_DVAL_P(holder),  /* allow errors */ true, NULL, &trailing_data))) {
				// 如果失败，会导致 无效op 类型 异常
				/* Will lead to invalid OP type error */
				return FAILURE;
			}
			
			// 如果有尾数据
			if (UNEXPECTED(trailing_data)) {
				// 报错：遇到了非数字的值
				zend_error(E_WARNING, "A non-numeric value encountered");
				// 如果有异常
				if (UNEXPECTED(EG(exception))) {
					// 返回失败
					return FAILURE;
				}
			}
			// 返回成功
			return SUCCESS;
		}
		// 对象
		case IS_OBJECT:
			// 对象转成数字，放在 holder 里
			if (Z_OBJ_HT_P(op)->cast_object(Z_OBJ_P(op), holder, _IS_NUMBER) == FAILURE
					|| EG(exception)) {
				// 如果转换失败或有异常，返回失败
				return FAILURE;
			}
			// 结果必须是 整数或小数
			ZEND_ASSERT(Z_TYPE_P(holder) == IS_LONG || Z_TYPE_P(holder) == IS_DOUBLE);
			// 返回成功
			return SUCCESS;
		// 资源或数组 
		case IS_RESOURCE:
		case IS_ARRAY:
			// 返回失败
			return FAILURE;
		EMPTY_SWITCH_DEFAULT_CASE()
	}
}
/* }}} */

// ing3, 标量转数字（结果可能是整数或小数）
static zend_always_inline zend_result zendi_try_convert_scalar_to_number(zval *op, zval *holder) /* {{{ */
{
	// 如果本身是整数或小数，直接copy给结果
	if (Z_TYPE_P(op) == IS_LONG || Z_TYPE_P(op) == IS_DOUBLE) {
		ZVAL_COPY_VALUE(holder, op);
		return SUCCESS;
	// 本身是其他类型
	} else {
		// 标量转成数字（可能是整数或小数）
		return _zendi_try_convert_scalar_to_number(op, holder);
	}
}
/* }}} */

// ing4, zval转成整数 ，不支持资源类型 和 数组
static zend_never_inline zend_long ZEND_FASTCALL zendi_try_get_long(zval *op, bool *failed) /* {{{ */
{
	*failed = 0;
	switch (Z_TYPE_P(op)) {
		// null,false 转成0
		case IS_NULL:
		case IS_FALSE:
			return 0;
		// true转成1
		case IS_TRUE:
			return 1;
		// 小数
		case IS_DOUBLE: {
			double dval = Z_DVAL_P(op);
			// 转成整数
			zend_long lval = zend_dval_to_lval(dval);
			if (!zend_is_long_compatible(dval, lval)) {
				// 报错：隐式转换，从小数转成整数会丢失精度
				zend_incompatible_double_to_long_error(dval);
				// 如果有exception
				if (UNEXPECTED(EG(exception))) {
					// 失败标记
					*failed = 1;
				}
			}
			// 返回整数 
			return lval;
		}
		// 字串
		case IS_STRING:
			{
				zend_uchar type;
				zend_long lval;
				double dval;
				bool trailing_data = false;
				// 由于BC原因，允许出错，所以可以自己控制报错
				/* For BC reasons we allow errors so that we can warn on leading numeric string */
				// 字串转成数值
				type = is_numeric_string_ex(Z_STRVAL_P(op), Z_STRLEN_P(op), &lval, &dval,
					/* allow errors */ true, NULL, &trailing_data);
				// 如果转换失败
				if (type == 0) {
					// 失败标记
					*failed = 1;
					// 返回0
					return 0;
				}
				// 如果有尾数据
				if (UNEXPECTED(trailing_data)) {
					// 报错，遇到非数字字串
					zend_error(E_WARNING, "A non-numeric value encountered");
					// 如果有 异常
					if (UNEXPECTED(EG(exception))) {
						// 失败标记
						*failed = 1;
					}
				}
				// 转成整数
				if (EXPECTED(type == IS_LONG)) {
					// 直接返回
					return lval;
				// 其他情况：转成了小数
				} else {
					// 之前这里使用 strtol 而不是 is_numeric_string，strtol在溢出时会给出  LONG_MAX/_MIN
					// 所以使用 饱和转换来 模拟 strtol 的行为
					/* Previously we used strtol here, not is_numeric_string,
					 * and strtol gives you LONG_MAX/_MIN on overflow.
					 * We use use saturating conversion to emulate strtol()'s
					 * behaviour.
					 */
					// 强转成整数，不兼容时，返回最小或最大整数
					lval = zend_dval_to_lval_cap(dval);
					// 如果整数和小数不兼容
					if (!zend_is_long_compatible(dval, lval)) {
						// 报错
						zend_incompatible_string_to_long_error(Z_STR_P(op));
						// 如果有exception
						if (UNEXPECTED(EG(exception))) {
							// 失败标记
							*failed = 1;
						}
					}
					// 返回整数
					return lval;
				}
			}
		// 对象
		case IS_OBJECT:
			{
				zval dst;
				// 调用自身的转换方法转成整数
				if (Z_OBJ_HT_P(op)->cast_object(Z_OBJ_P(op), &dst, IS_LONG) == FAILURE
						|| EG(exception)) {
					// 如果转换失败或有 exception
					// 失败标记
					*failed = 1;
					// 返回0
					return 0;
				}
				// 返回值是整数
				ZEND_ASSERT(Z_TYPE(dst) == IS_LONG);
				// 取出整数并返回
				return Z_LVAL(dst);
			}
		// 资源和数组不能转
		case IS_RESOURCE:
		case IS_ARRAY:
			// 转换失败
			*failed = 1;
			// 返回0
			return 0;
		EMPTY_SWITCH_DEFAULT_CASE()
	}
}
/* }}} */

// #define Z_OBJ_HANDLER(zval, hf)		Z_OBJ_HT((zval))->hf
// #define Z_OBJ_HT(zval)				Z_OBJ(zval)->handlers
// ing4, 尝试调用op2的 do_operation 方法，如果成功直接返回
#define ZEND_TRY_BINARY_OP1_OBJECT_OPERATION(opcode) \
	/* 如果op1类型是对象 并且  */ \
	if (UNEXPECTED(Z_TYPE_P(op1) == IS_OBJECT) \
		/* 访问 zval中的对象指针的操作方法集，再访问集合里的指定方法 */ \
		/* 如果要访问在的方法存在 */ \
		&& UNEXPECTED(Z_OBJ_HANDLER_P(op1, do_operation))) { \
		/* 调用指定方法，如果成功 */\
		if (EXPECTED(SUCCESS == Z_OBJ_HANDLER_P(op1, do_operation)(opcode, result, op1, op2))) { \
			/* 返回成功 */\
			return SUCCESS; \
		} \
	}

// ing4, 尝试调用op2的 do_operation 方法，如果成功直接返回
#define ZEND_TRY_BINARY_OP2_OBJECT_OPERATION(opcode) \
	if (UNEXPECTED(Z_TYPE_P(op2) == IS_OBJECT) \
		/* 访问 zval中的对象指针的操作方法集，再访问集合里的指定方法 */ \
		/* 如果要访问在的方法存在 */ \
		&& UNEXPECTED(Z_OBJ_HANDLER_P(op2, do_operation)) \
		/* 调用指定方法，如果成功 */\
		&& EXPECTED(SUCCESS == Z_OBJ_HANDLER_P(op2, do_operation)(opcode, result, op1, op2))) { \
		/* 返回成功 */\
		return SUCCESS; \
	}

// 加减乘除和连接时用到	
// ing4, 尝试调用op1 和 op2的 do_operation 方法，如果成功直接返回
#define ZEND_TRY_BINARY_OBJECT_OPERATION(opcode) \
	ZEND_TRY_BINARY_OP1_OBJECT_OPERATION(opcode) \
	else \
	ZEND_TRY_BINARY_OP2_OBJECT_OPERATION(opcode)

// ing4, 一元操作符
#define ZEND_TRY_UNARY_OBJECT_OPERATION(opcode) \
	/* 如果op1是对象 */ \
	if (UNEXPECTED(Z_TYPE_P(op1) == IS_OBJECT) \
		/* 访问 zval中的对象指针的操作方法集，再访问集合里的指定方法 */ \
		/* 如果要访问在的方法存在 */ \
		&& UNEXPECTED(Z_OBJ_HANDLER_P(op1, do_operation)) \
		/* 调用指定方法，如果成功 */\
		&& EXPECTED(SUCCESS == Z_OBJ_HANDLER_P(op1, do_operation)(opcode, result, op1, NULL))) { \
		/* 返回成功 */ \
		return SUCCESS; \
	}

// ing4, 把op1,op2都转成整数 ，并按要求进行计算（左右位移 和 取余。全局没有其他 文件 调用）
#define convert_op1_op2_long(op1, op1_lval, op2, op2_lval, result, opcode, sigil) \
	do {																\
		/* 如果op1不是整数 */ \
		if (UNEXPECTED(Z_TYPE_P(op1) != IS_LONG)) {						\
			bool failed;											\
			/* 如果op1是引用 */ \
			if (Z_ISREF_P(op1)) {										\
				/* 追踪到引用目标 */ \
				op1 = Z_REFVAL_P(op1);									\
				/* 如果op1是整数 */ \
				if (Z_TYPE_P(op1) == IS_LONG) {							\
					/* 取出整数值 */ \
					op1_lval = Z_LVAL_P(op1);							\
					/* 还会进入后面的 do-while */ \
					break;												\
				}														\
			}															\
			/* 调用op1的 do_operation */ \
			ZEND_TRY_BINARY_OP1_OBJECT_OPERATION(opcode);				\
			/* zval转成整数 ，不支持资源类型 和 数组 */ \
			op1_lval = zendi_try_get_long(op1, &failed);				\
			/* 如果转换失败 */ \
			if (UNEXPECTED(failed)) {									\
				/* 二进制操作报错,不支持此操作符 */ \
				zend_binop_error(sigil, op1, op2);						\
				/* 如果接收变量不是第一个操作对象 */ \
				if (result != op1) {									\
					/* 清空接收变量（中断操作） */ \
					ZVAL_UNDEF(result);									\
				}														\
				/* 返回失败 */ \
				return FAILURE;											\
			}															\
		/* 如果op1是整数 */ \
		} else {														\
			/* 取出并返回整数值 */ \
			op1_lval = Z_LVAL_P(op1);									\
		}																\
	} while (0);														\
	do {																\
		/* 如果op2不是整数 */ \
		if (UNEXPECTED(Z_TYPE_P(op2) != IS_LONG)) {						\
			bool failed;											\
			/* 如果op2是引用 */ \
			if (Z_ISREF_P(op2)) {										\
				/* 追踪到引用目标 */ \
				op2 = Z_REFVAL_P(op2);									\
				/* 如果op1是整数 */ \
				if (Z_TYPE_P(op2) == IS_LONG) {							\
					/* 取出整数值 */ \
					op2_lval = Z_LVAL_P(op2);							\
					/* 还会进入后面的 do-while */ \
					break;												\
				}														\
			}															\
			/* 调用op2的 do_operation */ \
			ZEND_TRY_BINARY_OP2_OBJECT_OPERATION(opcode);				\
			/* zval转成整数 ，不支持资源类型 和 数组 */ \
			op2_lval = zendi_try_get_long(op2, &failed);				\
			/* 如果转换失败 */ \
			if (UNEXPECTED(failed)) {									\
				/* 二进制操作报错,不支持此操作符 */ \
				zend_binop_error(sigil, op1, op2);						\
				/* 如果接收变量不是第一个操作对象 */ \
				if (result != op1) {									\
					/* 清空接收变量（中断操作） */ \
					ZVAL_UNDEF(result);									\
				}														\
				/* 返回失败 */ \
				return FAILURE;											\
			}															\
		/* 如果op1是整数 */ \
		} else {														\
			/* 取出并返回整数值 */ \
			op2_lval = Z_LVAL_P(op2);									\
		}																\
	} while (0);

// ing4, 把各种类型转换为整数
ZEND_API void ZEND_FASTCALL convert_to_long(zval *op) /* {{{ */
{
	zend_long tmp;

try_again:
	//
	switch (Z_TYPE_P(op)) {
		// false 和 null 转成 0
		case IS_NULL:
		case IS_FALSE:
			ZVAL_LONG(op, 0);
			break;
		// true 转成 1
		case IS_TRUE:
			ZVAL_LONG(op, 1);
			break;
		// 资源
		case IS_RESOURCE:
			// 找到资源的 ID
			tmp = Z_RES_HANDLE_P(op);
			// 销毁op的指针元素
			zval_ptr_dtor(op);
			// 把资源id 赋值给op
			ZVAL_LONG(op, tmp);
			break;
		// 整型直接跳过
		case IS_LONG:
			break;
		// 小数，转成整数 
		case IS_DOUBLE:
			ZVAL_LONG(op, zend_dval_to_lval(Z_DVAL_P(op)));
			break;
		// 字串
		case IS_STRING:
			{
				// 先取出 字串指针
				zend_string *str = Z_STR_P(op);
				// 尝试转成整数并放入op
				ZVAL_LONG(op, zval_get_long(op));
				// 释放字串
				zend_string_release_ex(str, 0);
			}
			break;
		// 数组
		case IS_ARRAY:
			// 数组是否为空
			tmp = (zend_hash_num_elements(Z_ARRVAL_P(op))?1:0);
			// 删除op里的指针
			zval_ptr_dtor(op);
			// 更新op
			ZVAL_LONG(op, tmp);
			break;
		// 对象
		case IS_OBJECT:
			{
				zval dst;
				// 把对象转成类型
				convert_object_to_type(op, &dst, IS_LONG);
				// 销毁 Op 中的指针
				zval_ptr_dtor(op);
				// 如果 转换结果是整数 
				if (Z_TYPE(dst) == IS_LONG) {
					// 把结果赋值给 op
					ZVAL_LONG(op, Z_LVAL(dst));
				} else {
					// 结果为 1
					ZVAL_LONG(op, 1);
				}
				return;
			}
		// 引用
		case IS_REFERENCE:
			// 解包引用对象：引用的值赋给当前变量
			zend_unwrap_reference(op);
			// 重来一次
			goto try_again;
		EMPTY_SWITCH_DEFAULT_CASE()
	}
}
/* }}} */

// ing3, 内置变量转成double类型
ZEND_API void ZEND_FASTCALL convert_to_double(zval *op) /* {{{ */
{
	double tmp;

try_again:
	// 按类型操作
	switch (Z_TYPE_P(op)) {
		// null或false
		case IS_NULL:
		case IS_FALSE:
			// 转成 0.0
			ZVAL_DOUBLE(op, 0.0);
			break;
		// true
		case IS_TRUE:
			// 转成 1
			ZVAL_DOUBLE(op, 1.0);
			break;
		// 资源类型
		case IS_RESOURCE: {
				// #define Z_RES_HANDLE_P(zval_p) Z_RES_HANDLE(*zval_p)
				double d = (double) Z_RES_HANDLE_P(op);
				zval_ptr_dtor(op);
				ZVAL_DOUBLE(op, d);
			}
			break;
		// 整数
		case IS_LONG:
			// 转成double
			ZVAL_DOUBLE(op, (double) Z_LVAL_P(op));
			break;
		// double 不用转
		case IS_DOUBLE:
			break;
		// 字串
		case IS_STRING:
			{
				// 先获取字串
				zend_string *str = Z_STR_P(op);
				// 字串转小数
				ZVAL_DOUBLE(op, zend_strtod(ZSTR_VAL(str), NULL));
				// 释放临时变量
				zend_string_release_ex(str, 0);
			}
			break;
		// 数组  
		case IS_ARRAY:
			tmp = (zend_hash_num_elements(Z_ARRVAL_P(op))?1:0);
			zval_ptr_dtor(op);
			ZVAL_DOUBLE(op, tmp);
			break;
		// 对象类型
		case IS_OBJECT:
			{
				zval dst;
				// 对象转小数
				convert_object_to_type(op, &dst, IS_DOUBLE);
				// 销毁 op
				zval_ptr_dtor(op);
				// 如果结果是小数
				if (Z_TYPE(dst) == IS_DOUBLE) {
					// 更新op，写入转换成的值
					ZVAL_DOUBLE(op, Z_DVAL(dst));
				// 结果不是小数
				} else {
					// 更新op的值为1
					ZVAL_DOUBLE(op, 1.0);
				}
				break;
			}
		// 引用对象
		case IS_REFERENCE:
			// 解包引用对象：引用的值赋给当前变量
			zend_unwrap_reference(op);
			goto try_again;
		EMPTY_SWITCH_DEFAULT_CASE()
	}
}
/* }}} */

// ing4, 转Null
ZEND_API void ZEND_FASTCALL convert_to_null(zval *op) /* {{{ */
{
	// 直接销毁op
	zval_ptr_dtor(op);
	// 写入null
	ZVAL_NULL(op);
}
/* }}} */

// ing3, 转bool型
ZEND_API void ZEND_FASTCALL convert_to_boolean(zval *op) /* {{{ */
{
	bool tmp;

try_again:
	switch (Z_TYPE_P(op)) {
		// false,true 不用转
		case IS_FALSE:
		case IS_TRUE:
			break;
		// null转成false
		case IS_NULL:
			ZVAL_FALSE(op);
			break;
		// 资源
		case IS_RESOURCE: {
				// 取得 有 id ？ 1 :0
				zend_long l = (Z_RES_HANDLE_P(op) ? 1 : 0);
				// 销毁 op
				zval_ptr_dtor(op);
				// 写入op
				ZVAL_BOOL(op, l);
			}
			break;
		// 整数
		case IS_LONG:
			// 0为false，其他true
			ZVAL_BOOL(op, Z_LVAL_P(op) ? 1 : 0);
			break;
		// 小数
		case IS_DOUBLE:
			// 0.0为false，其他true
			ZVAL_BOOL(op, Z_DVAL_P(op) ? 1 : 0);
			break;
		// 字串
		case IS_STRING:
			{
				// 取出 string
				zend_string *str = Z_STR_P(op);
				// 空串和 "0" 为false
				if (ZSTR_LEN(str) == 0
					|| (ZSTR_LEN(str) == 1 && ZSTR_VAL(str)[0] == '0')) {
					ZVAL_FALSE(op);
				// 其他为true
				} else {
					ZVAL_TRUE(op);
				}
				// 销毁字串
				zend_string_release_ex(str, 0);
			}
			break;
		// 数组 
		case IS_ARRAY:
			// 有元素1，无元素0
			tmp = (zend_hash_num_elements(Z_ARRVAL_P(op))?1:0);
			zval_ptr_dtor(op);
			ZVAL_BOOL(op, tmp);
			break;
		// 对象
		case IS_OBJECT:
			{
				zval dst;
				// 对象转bool
				convert_object_to_type(op, &dst, _IS_BOOL);
				// 销毁op
				zval_ptr_dtor(op);
				// 如果转成了true或false
				if (Z_TYPE_INFO(dst) == IS_FALSE || Z_TYPE_INFO(dst) == IS_TRUE) {
					// 复制type_info
					Z_TYPE_INFO_P(op) = Z_TYPE_INFO(dst);
				// 其他情况都算 true
				} else {
					ZVAL_TRUE(op);
				}
				break;
			}
		// 解包后从头再来
		case IS_REFERENCE:
			// 解包引用对象：引用的值赋给当前变量
			zend_unwrap_reference(op);
			goto try_again;
		EMPTY_SWITCH_DEFAULT_CASE()
	}
}
/* }}} */

// ing3, 内置变量转换成字串（这是php很核心的能力之一）
ZEND_API void ZEND_FASTCALL _convert_to_string(zval *op) /* {{{ */
{
try_again:
	switch (Z_TYPE_P(op)) {
		// 未定义，null,false
		case IS_UNDEF:
		case IS_NULL:
		case IS_FALSE: {
			// 转成空字串
			ZVAL_EMPTY_STRING(op);
			break;
		}
		// true 转成 1
		case IS_TRUE:
			ZVAL_CHAR(op, '1');
			break;
		// 是 string 就不用转了
		case IS_STRING:
			break;
		// 资源，转成文字描述，打印资源ID
		case IS_RESOURCE: {
			zend_string *str = zend_strpprintf(0, "Resource id #" ZEND_LONG_FMT, (zend_long)Z_RES_HANDLE_P(op));
			zval_ptr_dtor(op);
			ZVAL_NEW_STR(op, str);
			break;
		}
		// long 型
		case IS_LONG:
			ZVAL_STR(op, zend_long_to_str(Z_LVAL_P(op)));
			break;
		// double 型
		case IS_DOUBLE:
			ZVAL_NEW_STR(op, zend_double_to_str(Z_DVAL_P(op)));
			break;
		// array 报错，array不能直接转 string
		case IS_ARRAY:
			zend_error(E_WARNING, "Array to string conversion");
			// 销毁op
			zval_ptr_dtor(op);
			// op写入内置字串 "Array"
			ZVAL_INTERNED_STR(op, ZSTR_KNOWN(ZEND_STR_ARRAY_CAPITALIZED));
			break;
		// ing3, 对象
		case IS_OBJECT: {
			zval tmp;
			
			// 如果转换，成功. _zend_object_handlers 结构体的 cast_object 成员函数。由外部传入。
			if (Z_OBJ_HT_P(op)->cast_object(Z_OBJ_P(op), &tmp, IS_STRING) == SUCCESS) {
				// 销毁 op
				zval_ptr_dtor(op);
				// 
				ZVAL_COPY_VALUE(op, &tmp);
				return;
			}
			// 报错
			if (!EG(exception)) {
				zend_throw_error(NULL, "Object of class %s could not be converted to string", ZSTR_VAL(Z_OBJCE_P(op)->name));
			}
			// 销毁op
			zval_ptr_dtor(op);
			// 结果为空字串
			ZVAL_EMPTY_STRING(op);
			break;
		}
		// 地址引用。解包后从头再来
		case IS_REFERENCE:
			// 解包引用对象：引用的值赋给当前变量
			zend_unwrap_reference(op);
			goto try_again;
		//
		EMPTY_SWITCH_DEFAULT_CASE()
	}
}
/* }}} */

// ing4, 尝试转成字串
ZEND_API bool ZEND_FASTCALL _try_convert_to_string(zval *op) /* {{{ */
{
	zend_string *str;
	// 本身不可以是字串
	ZEND_ASSERT(Z_TYPE_P(op) != IS_STRING);
	// zval 转成 字串, 有try
	str = zval_try_get_string_func(op);
	// 如果转换失败
	if (UNEXPECTED(!str)) {
		// 返回 false
		return 0;
	}
	// 清空 op
	zval_ptr_dtor(op);
	// 值给op
	ZVAL_STR(op, str);
	// 返回true
	return 1;
}
/* }}} */

// ing4, 标量转数组：创建新数组，把标量作为第一个元素
static void convert_scalar_to_array(zval *op) /* {{{ */
{
	// 创建新哈希表
	HashTable *ht = zend_new_array(1);
	// 添加新元素op
	zend_hash_index_add_new(ht, 0, op);
	// 把新哈希表关联到 op
	ZVAL_ARR(op, ht);
}
/* }}} */

// ing3, 各种类型转成数组
ZEND_API void ZEND_FASTCALL convert_to_array(zval *op) /* {{{ */
{
try_again:
	switch (Z_TYPE_P(op)) {
		// 数组，不用转
		case IS_ARRAY:
			break;
/* OBJECTS_OPTIMIZE */
		// 对象
		case IS_OBJECT:
			// 闭包
			if (Z_OBJCE_P(op) == zend_ce_closure) {
				// 标量转数组：创建新数组，把标量作为第一个元素
				convert_scalar_to_array(op);
			// （没有属性和特殊的属性获取方法）
			// 如果对象没有属性
			} else if (Z_OBJ_P(op)->properties == NULL
			// #define Z_OBJ_HT(zval)				Z_OBJ(zval)->handlers
			// 并且 对象没有 get_properties_for 方法
			 && Z_OBJ_HT_P(op)->get_properties_for == NULL
			// 并且对象的  get_properties 是标准方法 
			 && Z_OBJ_HT_P(op)->get_properties == zend_std_get_properties) {
				// 不需要重建属性哈希表的 优化版本
				/* Optimized version without rebuilding properties HashTable */
				HashTable *ht = zend_std_build_object_properties_array(Z_OBJ_P(op));
				// 释放op里的对象
				OBJ_RELEASE(Z_OBJ_P(op));
				// ht 关联到op
				ZVAL_ARR(op, ht);
			// 其他情况，有属性或特殊的属性获取方法
			// ing3, 取出对象属性表，转成符号表，然后销毁原属性表
			} else {
				// 调用对象自带的方法，或标准方法，获取属性列表
				HashTable *obj_ht = zend_get_properties_for(op, ZEND_PROP_PURPOSE_ARRAY_CAST);
				// 如果列表有效
				if (obj_ht) {
					// 把属性表（只包含字串key），转成符号表 （包含数字和非数字key）
					HashTable *new_obj_ht = zend_proptable_to_symtable(obj_ht,
						// （如果有默认属性 或 不使用标准对象方法 或 gc里有递归。则需要创建副本）
						(Z_OBJCE_P(op)->default_properties_count ||
						 Z_OBJ_P(op)->handlers != &std_object_handlers ||
						 GC_IS_RECURSIVE(obj_ht)));
					// 清空op
					zval_ptr_dtor(op);
					// 放入新哈希表
					ZVAL_ARR(op, new_obj_ht);
					// 释放原属性表
					zend_release_properties(obj_ht);
				// 列表无效
				} else {
					// 销毁op
					zval_ptr_dtor(op);
					// op里插入新数组
					/*ZVAL_EMPTY_ARRAY(op);*/
					array_init(op);
				}
			}
			break;
		// null
		case IS_NULL:
			// 转成空数组 
			/*ZVAL_EMPTY_ARRAY(op);*/
			array_init(op);
			break;
		// 引用
		case IS_REFERENCE:
			// 解包引用对象：引用的值赋给当前变量
			zend_unwrap_reference(op);
			goto try_again;
		// 其他类型
		default:
			// 标量转数组：创建新数组，把标量作为第一个元素
			convert_scalar_to_array(op);
			break;
	}
}
/* }}} */

// ing4, zval转成对象
ZEND_API void ZEND_FASTCALL convert_to_object(zval *op) /* {{{ */
{
try_again:
	switch (Z_TYPE_P(op)) {
		// 数组
		case IS_ARRAY:
			{
				// 符号表转成属性表
				HashTable *ht = zend_symtable_to_proptable(Z_ARR_P(op));
				zend_object *obj;
				
				// 如果属性表不可转换
				if (GC_FLAGS(ht) & IS_ARRAY_IMMUTABLE) {
					/* TODO: try not to duplicate immutable arrays as well ??? */
					// 创建副本
					ht = zend_array_dup(ht);
				// 如果可转换，并且 如果哈希表是副本
				} else if (ht != Z_ARR_P(op)) {
					// 销毁原本
					zval_ptr_dtor(op);
				// 如果可转换，并且 如果哈希表不是副本
				} else {
					// 引用次数-1（转换操作要求把原对象替换掉，不是副本，前面一定增加过引用次数，这里要减掉）
					GC_DELREF(ht);
				}
				// zend_standard_class_def = register_class_stdClass();
				// 创建 stdclass实例
				obj = zend_objects_new(zend_standard_class_def);
				// 把属性表关联进来
				obj->properties = ht;
				// 实例关联到op1
				ZVAL_OBJ(op, obj);
				break;
			}
		// 对象不用转
		case IS_OBJECT:
			break;
		// null，转成空对象
		case IS_NULL:
			// 
			object_init(op);
			break;
		// 引用类型
		case IS_REFERENCE:
			// 解包引用对象：引用的值赋给当前变量
			zend_unwrap_reference(op);
			// 重来
			goto try_again;
		// 其他类型
		default: {
			zval tmp;
			// 值copy给临时变量
			ZVAL_COPY_VALUE(&tmp, op);
			// op 创建对象
			object_init(op);
			// 对象的属性表里添加新值 ，key是 scalar ，值是刚创建的op的副本
			zend_hash_add_new(Z_OBJPROP_P(op), ZSTR_KNOWN(ZEND_STR_SCALAR), &tmp);
			break;
		}
	}
}
/* }}} */

// ing4, 报错：隐式转换，从小数转成整数会丢失精度
ZEND_API void ZEND_COLD zend_incompatible_double_to_long_error(double d)
{
	zend_error_unchecked(E_DEPRECATED, "Implicit conversion from float %.*H to int loses precision", -1, d);
}

// ing4, 报错：隐式转换，从小数字串转成整数会丢失精度
ZEND_API void ZEND_COLD zend_incompatible_string_to_long_error(const zend_string *s)
{
	zend_error(E_DEPRECATED, "Implicit conversion from float-string \"%s\" to int loses precision", ZSTR_VAL(s));
}

// ing3, 从zval里取出整数，p1:zval，p2:是否严格类型
ZEND_API zend_long ZEND_FASTCALL zval_get_long_func(zval *op, bool is_strict) /* {{{ */
{

try_again:
	// 根据类型操作
	switch (Z_TYPE_P(op)) {
		// undef,null,false, 返回0
		case IS_UNDEF:
		case IS_NULL:
		case IS_FALSE:
			return 0;
		// true，返回1
		case IS_TRUE:
			return 1;
		// 资源返回序号
		case IS_RESOURCE:
			return Z_RES_HANDLE_P(op);
		// 整数指接取出并返回
		case IS_LONG:
			return Z_LVAL_P(op);
		// 小数
		case IS_DOUBLE: {
			// 取出小数并转成整数
			double dval = Z_DVAL_P(op);
			zend_long lval = zend_dval_to_lval(dval);
			// 如果有严格标记
			if (UNEXPECTED(is_strict)) {
				// 如果整数和小数不兼容
				if (!zend_is_long_compatible(dval, lval)) {
					// 报错：隐式转换，从小数转成整数会丢失精度
					zend_incompatible_double_to_long_error(dval);
				}
			}
			// 返回整数
			return lval;
		}
		// 字串
		case IS_STRING:
			{
				zend_uchar type;
				zend_long lval;
				double dval;
				// 尝试把字串转成数字
				if (0 == (type = is_numeric_string(Z_STRVAL_P(op), Z_STRLEN_P(op), &lval, &dval, true))) {
					// 失败返回0
					return 0;
				// 转成整数，返回整数 
				} else if (EXPECTED(type == IS_LONG)) {
					return lval;
				// 转成小数
				} else {
					// 之前这里使用 strtol 而不是 is_numeric_string，strtol在溢出时会给出  LONG_MAX/_MIN
					// 所以使用 饱和转换来 模拟 strtol 的行为
					/* Previously we used strtol here, not is_numeric_string,
					 * and strtol gives you LONG_MAX/_MIN on overflow.
					 * We use saturating conversion to emulate strtol()'s
					 * behaviour.
					 */
					// 大部分情况不转成整数 
					 /* Most usages are expected to not be (int) casts */
					// 强转成整数，不兼容时，返回最小或最大整数
					lval = zend_dval_to_lval_cap(dval);
					// 有严格标记
					if (UNEXPECTED(is_strict)) {
						// 如果不兼容
						if (!zend_is_long_compatible(dval, lval)) {
							// 报错
							zend_incompatible_string_to_long_error(Z_STR_P(op));
						}
					}
					// 返回整数 
					return lval;
				}
			}
		// 数组
		case IS_ARRAY:
			// 有元素返回1，否则 0
			return zend_hash_num_elements(Z_ARRVAL_P(op)) ? 1 : 0;
		// 对象
		case IS_OBJECT:
			{
				zval dst;
				// 对象转成 整数
				convert_object_to_type(op, &dst, IS_LONG);
				// 如果结果是整数
				if (Z_TYPE(dst) == IS_LONG) {
					// 返回转换后的整数
					return Z_LVAL(dst);
				// 结果不是整数 
				} else {
					// 返回1
					return 1;
				}
			}
		// 引用类型
		case IS_REFERENCE:
			// 追踪到目标
			op = Z_REFVAL_P(op);
			// 再来一次
			goto try_again;
		EMPTY_SWITCH_DEFAULT_CASE()
	}
	// 失败返回0
	return 0;
}
/* }}} */

// ing3, zval转成小数
ZEND_API double ZEND_FASTCALL zval_get_double_func(zval *op) /* {{{ */
{
try_again:
	//
	switch (Z_TYPE_P(op)) {
		// null，false 转成 0.0
		case IS_NULL:
		case IS_FALSE:
			return 0.0;
		// true 转成 1
		case IS_TRUE:
			return 1.0;
		// 资源，返回id （double型）
		case IS_RESOURCE:
			return (double) Z_RES_HANDLE_P(op);
		// 整数直接转成double返回
		case IS_LONG:
			return (double) Z_LVAL_P(op);
		// 小数，取出 小数返回
		case IS_DOUBLE:
			return Z_DVAL_P(op);
		// 字串，字串转小数
		case IS_STRING:
			return zend_strtod(Z_STRVAL_P(op), NULL);
		// 数组，有元素返回1.0，否则 0.0	
		case IS_ARRAY:
			return zend_hash_num_elements(Z_ARRVAL_P(op)) ? 1.0 : 0.0;
		// 对象
		case IS_OBJECT:
			{
				zval dst;
				// 调用对象的方法转换
				convert_object_to_type(op, &dst, IS_DOUBLE);
				// 结果是小数，取出 小数返回
				if (Z_TYPE(dst) == IS_DOUBLE) {
					return Z_DVAL(dst);
				// 结果不是小数，返回1.0
				} else {
					return 1.0;
				}
			}
		// 引用类型
		case IS_REFERENCE:
			// 追踪到引用目标
			op = Z_REFVAL_P(op);
			// 重来
			goto try_again;
		EMPTY_SWITCH_DEFAULT_CASE()
	}
	// 其他情况一律返回0
	return 0.0;
}
/* }}} */

// ing3, zval 转成 字串
static zend_always_inline zend_string* __zval_get_string_func(zval *op, bool try) /* {{{ */
{
try_again:
	// 根据类型操作
	switch (Z_TYPE_P(op)) {
		// undef, null, false 返回空字串
		case IS_UNDEF:
		case IS_NULL:
		case IS_FALSE:
			return ZSTR_EMPTY_ALLOC();
		// true 返回1
		case IS_TRUE:
			return ZSTR_CHAR('1');
		// 资源："Resource id #{资源ID}"
		case IS_RESOURCE:
			return zend_strpprintf(0, "Resource id #" ZEND_LONG_FMT, (zend_long)Z_RES_HANDLE_P(op));
		// 整数，转成字串
		case IS_LONG:
			return zend_long_to_str(Z_LVAL_P(op));
		// 小数，转成字串
		case IS_DOUBLE:
			return zend_double_to_str(Z_DVAL_P(op));
		// 数组
		case IS_ARRAY:
			// 警告：数组转字串操作
			zend_error(E_WARNING, "Array to string conversion");
			// 如果有try 并且有 exception, 返回null，否则返回 "Array"
			return (try && UNEXPECTED(EG(exception))) ?
				NULL : ZSTR_KNOWN(ZEND_STR_ARRAY_CAPITALIZED);
		// 对象
		case IS_OBJECT: {
			zval tmp;
			// 对象转成字串
			if (Z_OBJ_HT_P(op)->cast_object(Z_OBJ_P(op), &tmp, IS_STRING) == SUCCESS) {
				// 如果成功，返回临时变量里的zend_string 指针
				return Z_STR(tmp);
			}
			// 如果没有异常
			if (!EG(exception)) {
				// 抛异常：属于此类的对象无法转成字串
				zend_throw_error(NULL, "Object of class %s could not be converted to string", ZSTR_VAL(Z_OBJCE_P(op)->name));
			}
			// 有try , 返回null，否则返回 空字串
			return try ? NULL : ZSTR_EMPTY_ALLOC();
		}
		// 引用类型
		case IS_REFERENCE:
			// 追踪到目标
			op = Z_REFVAL_P(op);
			goto try_again;
		// 字串类型
		case IS_STRING:
			// 自己的下属字串 增加引用次数，并返回
			return zend_string_copy(Z_STR_P(op));
		EMPTY_SWITCH_DEFAULT_CASE()
	}
	// 其他情况返回null
	return NULL;
}
/* }}} */

// ing3, zval 转成 字串, 没有try
ZEND_API zend_string* ZEND_FASTCALL zval_get_string_func(zval *op) /* {{{ */
{
	return __zval_get_string_func(op, 0);
}
/* }}} */

// ing3, zval 转成 字串, 有try
ZEND_API zend_string* ZEND_FASTCALL zval_try_get_string_func(zval *op) /* {{{ */
{
	return __zval_get_string_func(op, 1);
}
/* }}} */

// ing4, 二进制操作报错,不支持此操作符
static ZEND_COLD zend_never_inline void ZEND_FASTCALL zend_binop_error(const char *operator, zval *op1, zval *op2) /* {{{ */ {
	// 如果有异常，直接返回
	if (EG(exception)) {
		return;
	}
	// 抛异常： 这两个类型的对象 不支持此操作
	zend_type_error("Unsupported operand types: %s %s %s",
		zend_zval_type_name(op1), operator, zend_zval_type_name(op2));
}
/* }}} */

// ing3, 两个数组相加
static zend_never_inline void ZEND_FASTCALL add_function_array(zval *result, zval *op1, zval *op2) /* {{{ */
{
	// 如果结果和op1相等，并且op1,op2 指向同一个数组
	if (result == op1 && Z_ARR_P(op1) == Z_ARR_P(op2)) {
		// 数组加自己
		/* $a += $a */
		// 无效，直接返回
		return;
	}
	
	// 1. 先处理result和op1的关系。
	
	// 结果不等于op1 ($c = $a + $b;)
	if (result != op1) {
		// 把op1 复制一份，返回给结果 
		ZVAL_ARR(result, zend_array_dup(Z_ARR_P(op1)));
	// 结果等于op1 ($a = $a + $b;)
	} else {
		// 给result 创建副本，返回副本
		SEPARATE_ARRAY(result);
	}
	// 2. 然后用merge, 把op2加进来。 merge方法是增加引用次数。
	// 这个merge碰到相同索引号会跳过，例如：[1]+[2,3] => [1,3]
	zend_hash_merge(Z_ARRVAL_P(result), Z_ARRVAL_P(op2), zval_add_ref, 0);
}
/* }}} */

// ing3, 特定类型快速做加法
static zend_always_inline zend_result add_function_fast(zval *result, zval *op1, zval *op2) /* {{{ */
{
	// 组合类型
	zend_uchar type_pair = TYPE_PAIR(Z_TYPE_P(op1), Z_TYPE_P(op2));
	// 两个长整型
	if (EXPECTED(type_pair == TYPE_PAIR(IS_LONG, IS_LONG))) {
		fast_long_add_function(result, op1, op2);
		return SUCCESS;
	// 两个双精度
	} else if (EXPECTED(type_pair == TYPE_PAIR(IS_DOUBLE, IS_DOUBLE))) {
		ZVAL_DOUBLE(result, Z_DVAL_P(op1) + Z_DVAL_P(op2));
		return SUCCESS;
	// 一个长整形和一个双精度
	} else if (EXPECTED(type_pair == TYPE_PAIR(IS_LONG, IS_DOUBLE))) {
		ZVAL_DOUBLE(result, ((double)Z_LVAL_P(op1)) + Z_DVAL_P(op2));
		return SUCCESS;
	// 同上
	} else if (EXPECTED(type_pair == TYPE_PAIR(IS_DOUBLE, IS_LONG))) {
		ZVAL_DOUBLE(result, Z_DVAL_P(op1) + ((double)Z_LVAL_P(op2)));
		return SUCCESS;
	// 两个数组
	} else if (EXPECTED(type_pair == TYPE_PAIR(IS_ARRAY, IS_ARRAY))) {
		add_function_array(result, op1, op2);
		return SUCCESS;
	// 其他类型全部 failure
	} else {
		return FAILURE;
	}
} /* }}} */

// ing4, 慢加
static zend_never_inline zend_result ZEND_FASTCALL add_function_slow(zval *result, zval *op1, zval *op2) /* {{{ */
{
	// 先追踪到引用目标
	ZVAL_DEREF(op1);
	ZVAL_DEREF(op2);
	// 步骤1
	// 先尝试快加
	if (add_function_fast(result, op1, op2) == SUCCESS) {
		return SUCCESS;
	}
	
	// 步骤2
	// 尝试调用op1 和 op2的 do_operation 方法，如果成功直接返回
	// ZEND_ADD 是传入的参数
	ZEND_TRY_BINARY_OBJECT_OPERATION(ZEND_ADD);

	// 步骤3
	zval op1_copy, op2_copy;
	if (UNEXPECTED(zendi_try_convert_scalar_to_number(op1, &op1_copy) == FAILURE)
			|| UNEXPECTED(zendi_try_convert_scalar_to_number(op2, &op2_copy) == FAILURE)) {
		// 二进制操作报错,不支持此操作符
		zend_binop_error("+", op1, op2);
		// 如果第一个操作对象不是接收变量
		if (result != op1) {
			// 清空接收变量，中断操作
			ZVAL_UNDEF(result);
		}
		// 返回失败
		return FAILURE;
	}
	// 如果没有出错
	
	// 如果 接收变量与第一个操作对象相同
	if (result == op1) {
		// 销毁 接收变量
		zval_ptr_dtor(result);
	}

	// 步骤4，再次尝试快加
	if (add_function_fast(result, &op1_copy, &op2_copy) == SUCCESS) {
		return SUCCESS;
	}

	ZEND_ASSERT(0 && "Operation must succeed");
	return FAILURE;
} /* }}} */

// ing4, 运算符 + 
ZEND_API zend_result ZEND_FASTCALL add_function(zval *result, zval *op1, zval *op2) /* {{{ */
{
	// 快速加法，只适合少数类型
	if (add_function_fast(result, op1, op2) == SUCCESS) {
		return SUCCESS;
	} else {
		return add_function_slow(result, op1, op2);
	}
}
/* }}} */

// ing4, 特定类型，快速减法
static zend_always_inline zend_result sub_function_fast(zval *result, zval *op1, zval *op2) /* {{{ */
{
	zend_uchar type_pair = TYPE_PAIR(Z_TYPE_P(op1), Z_TYPE_P(op2));

	// 两个 long 型
	if (EXPECTED(type_pair == TYPE_PAIR(IS_LONG, IS_LONG))) {
		fast_long_sub_function(result, op1, op2);
		return SUCCESS;
	// 两个 double 型
	} else if (EXPECTED(type_pair == TYPE_PAIR(IS_DOUBLE, IS_DOUBLE))) {
		ZVAL_DOUBLE(result, Z_DVAL_P(op1) - Z_DVAL_P(op2));
		return SUCCESS;
	// 左 long 右 double
	} else if (EXPECTED(type_pair == TYPE_PAIR(IS_LONG, IS_DOUBLE))) {
		ZVAL_DOUBLE(result, ((double)Z_LVAL_P(op1)) - Z_DVAL_P(op2));
		return SUCCESS;
	// 右 double 左 long
	} else if (EXPECTED(type_pair == TYPE_PAIR(IS_DOUBLE, IS_LONG))) {
		ZVAL_DOUBLE(result, Z_DVAL_P(op1) - ((double)Z_LVAL_P(op2)));
		return SUCCESS;
	// 其他情况不支持
	} else {
		return FAILURE;
	}
}
/* }}} */

// ing4, 慢减法
static zend_never_inline zend_result ZEND_FASTCALL sub_function_slow(zval *result, zval *op1, zval *op2) /* {{{ */
{
	// 先追踪到引用目标
	ZVAL_DEREF(op1);
	ZVAL_DEREF(op2);
	// 步骤1
	// 特定类型，快速减法
	if (sub_function_fast(result, op1, op2) == SUCCESS) {
		return SUCCESS;
	}
	// 步骤2
	// 尝试调用op1 和 op2的 do_operation 方法，如果成功直接返回
	// ZEND_SUB 是传入的参数
	ZEND_TRY_BINARY_OBJECT_OPERATION(ZEND_SUB);

	zval op1_copy, op2_copy;
	// 步骤3
	// op1，op2 转成数字
	if (UNEXPECTED(zendi_try_convert_scalar_to_number(op1, &op1_copy) == FAILURE)
			|| UNEXPECTED(zendi_try_convert_scalar_to_number(op2, &op2_copy) == FAILURE)) {
		// 二进制操作报错,不支持此操作符
		zend_binop_error("-", op1, op2);
		// 如果第一个操作对象不是接收变量
		if (result != op1) {
			// 清空接收变量，中断操作
			ZVAL_UNDEF(result);
		}
		return FAILURE;
	}
	// 如果没有出错

	// 如果 接收变量与第一个操作对象相同
	if (result == op1) {
		// 销毁 接收变量
		zval_ptr_dtor(result);
	}
	// 步骤4，再次尝试快速减法
	if (sub_function_fast(result, &op1_copy, &op2_copy) == SUCCESS) {
		return SUCCESS;
	}

	ZEND_ASSERT(0 && "Operation must succeed");
	return FAILURE;
}
/* }}} */

// ing4, 减法
ZEND_API zend_result ZEND_FASTCALL sub_function(zval *result, zval *op1, zval *op2) /* {{{ */
{
	// 特定类型，快速减法
	if (sub_function_fast(result, op1, op2) == SUCCESS) {
		return SUCCESS;
	} else {
		// 失败，使用慢减法
		return sub_function_slow(result, op1, op2);
	}
}
/* }}} */

// ing3, 快速乘法
static zend_always_inline zend_result mul_function_fast(zval *result, zval *op1, zval *op2) /* {{{ */
{
	zend_uchar type_pair = TYPE_PAIR(Z_TYPE_P(op1), Z_TYPE_P(op2));
	// 两个整数
	if (EXPECTED(type_pair == TYPE_PAIR(IS_LONG, IS_LONG))) {
		zend_long overflow;
		// 整数乘法
		ZEND_SIGNED_MULTIPLY_LONG(
			Z_LVAL_P(op1), Z_LVAL_P(op2),
			Z_LVAL_P(result), Z_DVAL_P(result), overflow);
		// 溢出按小数类型，否则按整数型
		Z_TYPE_INFO_P(result) = overflow ? IS_DOUBLE : IS_LONG;
		return SUCCESS;
	// 两个小数
	} else if (EXPECTED(type_pair == TYPE_PAIR(IS_DOUBLE, IS_DOUBLE))) {
		// 直接乘
		ZVAL_DOUBLE(result, Z_DVAL_P(op1) * Z_DVAL_P(op2));
		return SUCCESS;
	// 一个整数一个小数
	} else if (EXPECTED(type_pair == TYPE_PAIR(IS_LONG, IS_DOUBLE))) {
		// 转成小数算乘法
		ZVAL_DOUBLE(result, ((double)Z_LVAL_P(op1)) * Z_DVAL_P(op2));
		return SUCCESS;
	// 一个小数一个整数
	} else if (EXPECTED(type_pair == TYPE_PAIR(IS_DOUBLE, IS_LONG))) {
		// 转成小数算乘法
		ZVAL_DOUBLE(result, Z_DVAL_P(op1) * ((double)Z_LVAL_P(op2)));
		return SUCCESS;
	// 其他情况, 返回失败
	} else {
		return FAILURE;
	}
}
/* }}} */

// ing4, 慢乘法
static zend_never_inline zend_result ZEND_FASTCALL mul_function_slow(zval *result, zval *op1, zval *op2) /* {{{ */
{
	// 先追踪到引用目标
	ZVAL_DEREF(op1);
	ZVAL_DEREF(op2);
	// 步骤1
	// 先尝试快乘
	if (mul_function_fast(result, op1, op2) == SUCCESS) {
		return SUCCESS;
	}
	
	// 步骤2
	// 尝试调用op1 和 op2的 do_operation 方法，如果成功直接返回
	// ZEND_MUL 是传入的参数
	ZEND_TRY_BINARY_OBJECT_OPERATION(ZEND_MUL);

	zval op1_copy, op2_copy;
	// 步骤3
	// op1，op2 转成数字
	if (UNEXPECTED(zendi_try_convert_scalar_to_number(op1, &op1_copy) == FAILURE)
			|| UNEXPECTED(zendi_try_convert_scalar_to_number(op2, &op2_copy) == FAILURE)) {
		// 二进制操作报错,不支持此操作符
		zend_binop_error("*", op1, op2);
		// 如果第一个操作对象不是接收变量，清空接收变量
		if (result != op1) {
			// 清空接收变量，中断操作
			ZVAL_UNDEF(result);
		}
		return FAILURE;
	}

	// 如果 接收变量与第一个操作对象相同，销毁 接收变量
	if (result == op1) {
		zval_ptr_dtor(result);
	}

	// 步骤4，再次尝试快乘
	if (mul_function_fast(result, &op1_copy, &op2_copy) == SUCCESS) {
		return SUCCESS;
	}

	ZEND_ASSERT(0 && "Operation must succeed");
	return FAILURE;
}
/* }}} */

// ing4, 乘法
ZEND_API zend_result ZEND_FASTCALL mul_function(zval *result, zval *op1, zval *op2) /* {{{ */
{
	// 快速乘法
	if (mul_function_fast(result, op1, op2) == SUCCESS) {
		return SUCCESS;
	// 如果失败
	} else {
		// 慢乘
		return mul_function_slow(result, op1, op2);
	}
}
/* }}} */

// ing3, 幂运算，先用整数乘法，如果溢出转成小数幂运算
static zend_result ZEND_FASTCALL pow_function_base(zval *result, zval *op1, zval *op2) /* {{{ */
{
	zend_uchar type_pair = TYPE_PAIR(Z_TYPE_P(op1), Z_TYPE_P(op2));
	// 如果是两个整数 
	if (EXPECTED(type_pair == TYPE_PAIR(IS_LONG, IS_LONG))) {
		if (Z_LVAL_P(op2) >= 0) {
			// 
			zend_long l1 = 1, l2 = Z_LVAL_P(op1), i = Z_LVAL_P(op2);
			// 如果是0次幂
			if (i == 0) {
				// 结果为1
				ZVAL_LONG(result, 1L);
				return SUCCESS;
			// 如果是0的幂
			} else if (l2 == 0) {
				// 结果为0
				ZVAL_LONG(result, 0);
				return SUCCESS;
			}
			// 循环相乘，如果整数乘法一直不溢出，就一直用乘法算，只要有溢出，转成小数算
			while (i >= 1) {
				zend_long overflow;
				double dval = 0.0;
				// 如果i（幂次数）是奇数
				if (i % 2) {
					// i自减
					--i;
					// 算乘法 l1 = l1 * l2
					ZEND_SIGNED_MULTIPLY_LONG(l1, l2, l1, dval, overflow);
					// 如果有溢出
					if (overflow) {
						// 转成小数计数，其余部分用 pow计算
						ZVAL_DOUBLE(result, dval * pow(l2, i));
						return SUCCESS;
					}
				// 没有溢出，继续循环
				} else {
					// i减半
					i /= 2;
					// 算乘法 l2 = l2 * l2
					ZEND_SIGNED_MULTIPLY_LONG(l2, l2, l2, dval, overflow);
					// 如果有溢出
					if (overflow) {
						// 转成小数计数
						ZVAL_DOUBLE(result, (double)l1 * pow(dval, i));
						return SUCCESS;
					}
				}
			}
			// i==0 才走到这里，幂运算也用乘法运算计算完了
			/* i == 0 */
			ZVAL_LONG(result, l1);
		// op2 小于 0
		} else {
			// 调用底层函数计算（支持小数和负数的幂运算）
			ZVAL_DOUBLE(result, pow((double)Z_LVAL_P(op1), (double)Z_LVAL_P(op2)));
		}
		return SUCCESS;
	// 如果是两个小数
	} else if (EXPECTED(type_pair == TYPE_PAIR(IS_DOUBLE, IS_DOUBLE))) {
		// 调用操作系统 pow 方法，传入两个小数
		ZVAL_DOUBLE(result, pow(Z_DVAL_P(op1), Z_DVAL_P(op2)));
		return SUCCESS;
	// 如果是一个整数，一个小数
	} else if (EXPECTED(type_pair == TYPE_PAIR(IS_LONG, IS_DOUBLE))) {
		// 调用操作系统 pow 方法，传入两个小数
		ZVAL_DOUBLE(result, pow((double)Z_LVAL_P(op1), Z_DVAL_P(op2)));
		return SUCCESS;
	// 如果是一个小数，一个整数
	} else if (EXPECTED(type_pair == TYPE_PAIR(IS_DOUBLE, IS_LONG))) {
		// 调用操作系统 pow 方法，传入两个小数
		ZVAL_DOUBLE(result, pow(Z_DVAL_P(op1), (double)Z_LVAL_P(op2)));
		return SUCCESS;
	// 其他情况，返回失败
	} else {
		return FAILURE;
	}
}
/* }}} */

// ing3, 慢幂运算
ZEND_API zend_result ZEND_FASTCALL pow_function(zval *result, zval *op1, zval *op2) /* {{{ */
{
	// 先追踪到引用目标
	ZVAL_DEREF(op1);
	ZVAL_DEREF(op2);
	// 步骤1
	// 幂运算，先用整数，如果溢出转成小数计算
	if (pow_function_base(result, op1, op2) == SUCCESS) {
		return SUCCESS;
	}
	
	// 步骤2
	// 尝试调用op1 和 op2的 do_operation 方法，如果成功直接返回
	// ZEND_POW 是传入的参数
	ZEND_TRY_BINARY_OBJECT_OPERATION(ZEND_POW);

	zval op1_copy, op2_copy;
	// 步骤3
	// op1，op2 转成数字
	if (UNEXPECTED(zendi_try_convert_scalar_to_number(op1, &op1_copy) == FAILURE)
			|| UNEXPECTED(zendi_try_convert_scalar_to_number(op2, &op2_copy) == FAILURE)) {
		// 二进制操作报错,不支持此操作符
		zend_binop_error("**", op1, op2);
		// 如果第一个操作对象不是接收变量，清空接收变量
		if (result != op1) {
			// 清空接收变量，中断操作
			ZVAL_UNDEF(result);
		}
		return FAILURE;
	}
	// 如果没有出错

	// 如果 接收变量与第一个操作对象相同，销毁 接收变量
	if (result == op1) {
		zval_ptr_dtor(result);
	}

	// 步骤4，幂运算，先用整数，如果溢出转成小数计算
	if (pow_function_base(result, &op1_copy, &op2_copy) == SUCCESS) {
		return SUCCESS;
	}

	ZEND_ASSERT(0 && "Operation must succeed");
	return FAILURE;
}
/* }}} */

/* Returns SUCCESS/TYPES_NOT_HANDLED/DIV_BY_ZERO */
#define TYPES_NOT_HANDLED 1
#define DIV_BY_ZERO 2

// ing4, 除法
static int ZEND_FASTCALL div_function_base(zval *result, zval *op1, zval *op2) /* {{{ */
{
	zend_uchar type_pair = TYPE_PAIR(Z_TYPE_P(op1), Z_TYPE_P(op2));
	// 根据类型处理
	// 两个整数 
	if (EXPECTED(type_pair == TYPE_PAIR(IS_LONG, IS_LONG))) {
		// 如果除数是0，返回错误
		if (Z_LVAL_P(op2) == 0) {
			return DIV_BY_ZERO;
		// 如果第二个是 -1,并且第一个是最大数
		} else if (Z_LVAL_P(op2) == -1 && Z_LVAL_P(op1) == ZEND_LONG_MIN) {
			// 这样直接转会出错，转成小数再除
			/* Prevent overflow error/crash */
			ZVAL_DOUBLE(result, (double) ZEND_LONG_MIN / -1);
			return SUCCESS;
		}
		// 如果结果两个整数取余结果为0
		if (Z_LVAL_P(op1) % Z_LVAL_P(op2) == 0) { /* integer */
			// 直接按整数算除法，返回结果
			ZVAL_LONG(result, Z_LVAL_P(op1) / Z_LVAL_P(op2));
		// 如果有余数
		} else {
			// 转成小数算除法
			ZVAL_DOUBLE(result, ((double) Z_LVAL_P(op1)) / Z_LVAL_P(op2));
		}
		// 返回成功
		return SUCCESS;
	// 两个小数
	} else if (EXPECTED(type_pair == TYPE_PAIR(IS_DOUBLE, IS_DOUBLE))) {
		// 如果除数是0，返回错误
		if (Z_DVAL_P(op2) == 0) {
			return DIV_BY_ZERO;
		}
		// 直接除
		ZVAL_DOUBLE(result, Z_DVAL_P(op1) / Z_DVAL_P(op2));
		return SUCCESS;
	// 小数除以整数
	} else if (EXPECTED(type_pair == TYPE_PAIR(IS_DOUBLE, IS_LONG))) {
		// 如果除数是0，返回错误
		if (Z_LVAL_P(op2) == 0) {
			return DIV_BY_ZERO;
		}
		// 转成小数算除法
		ZVAL_DOUBLE(result, Z_DVAL_P(op1) / (double)Z_LVAL_P(op2));
		return SUCCESS;
	// 整数除以小数
	} else if (EXPECTED(type_pair == TYPE_PAIR(IS_LONG, IS_DOUBLE))) {
		// 如果除数是0，返回错误
		if (Z_DVAL_P(op2) == 0) {
			return DIV_BY_ZERO;
		}
		// 转成小数算除法
		ZVAL_DOUBLE(result, (double)Z_LVAL_P(op1) / Z_DVAL_P(op2));
		return SUCCESS;
	// 其他类型，不处理
	} else {
		return TYPES_NOT_HANDLED;
	}
}
/* }}} */

// ing3, 除法，有类型处理
ZEND_API zend_result ZEND_FASTCALL div_function(zval *result, zval *op1, zval *op2) /* {{{ */
{
	// 先追踪到引用目标
	ZVAL_DEREF(op1);
	ZVAL_DEREF(op2);

	// 步骤1
	// 先尝试普通除法
	int retval = div_function_base(result, op1, op2);
	// 成功直接返回
	if (EXPECTED(retval == SUCCESS)) {
		return SUCCESS;
	}
	// 除数是0
	if (UNEXPECTED(retval == DIV_BY_ZERO)) {
		goto div_by_zero;
	}
	// 步骤2
	// 尝试调用op1 和 op2的 do_operation 方法，如果成功直接返回
	// ZEND_DIV 是传入的参数
	ZEND_TRY_BINARY_OBJECT_OPERATION(ZEND_DIV);

	zval result_copy, op1_copy, op2_copy;
	// 步骤3
	// op1，op2 转成数字
	if (UNEXPECTED(zendi_try_convert_scalar_to_number(op1, &op1_copy) == FAILURE)
			|| UNEXPECTED(zendi_try_convert_scalar_to_number(op2, &op2_copy) == FAILURE)) {
		// 二进制操作报错,不支持此操作符
		zend_binop_error("/", op1, op2);
		// 如果第一个操作对象不是接收变量，清空接收变量
		if (result != op1) {
			// 清空接收变量，中断操作
			ZVAL_UNDEF(result);
		}
		// 返回失败
		return FAILURE;
	}
	// 如果没有出错

	// 步骤4，再次尝普通除法
	retval = div_function_base(&result_copy, &op1_copy, &op2_copy);
	// 如果成功
	if (retval == SUCCESS) {
		// 如果 接收变量与第一个操作对象相同，销毁 接收变量
		if (result == op1) {
			zval_ptr_dtor(result);
		}
		// 创建副本返回
		ZVAL_COPY_VALUE(result, &result_copy);
		return SUCCESS;
	}

div_by_zero:
	// 要有错误信息：除数是0
	ZEND_ASSERT(retval == DIV_BY_ZERO && "TYPES_NOT_HANDLED should not occur here");
	if (result != op1) {
		// 清空接收变量，中断操作
		ZVAL_UNDEF(result);
	}
	// 抛错：除数是0
	zend_throw_error(zend_ce_division_by_zero_error, "Division by zero");
	// 返回失败
	return FAILURE;
}
/* }}} */

// ing3, 取余运算。（c语言里的取余也是只针对整数 ，对负数取余相当于对它的绝对值取余。）
ZEND_API zend_result ZEND_FASTCALL mod_function(zval *result, zval *op1, zval *op2) /* {{{ */
{
	zend_long op1_lval, op2_lval;
	// 把op1,op2都转成整数  ，并按要求进行计算
	convert_op1_op2_long(op1, op1_lval, op2, op2_lval, result, ZEND_MOD, "%");
	// 第二个整数 是0
	if (op2_lval == 0) {
		// 对0取余
		/* modulus by zero */
		// 如果在执行时，不在编译时
		if (EG(current_execute_data) && !CG(in_compilation)) {
			// 报错，对0取余
			zend_throw_exception_ex(zend_ce_division_by_zero_error, 0, "Modulo by zero");
		// 否则 
		} else {
			// 不带返回的报错，对0取余
			zend_error_noreturn(E_ERROR, "Modulo by zero");
		}
		// 如果不是用op1接收
		if (op1 != result) {
			// 清空接收变量，中断操作
			ZVAL_UNDEF(result);
		}
		// 返回失败
		return FAILURE;
	}
	// 如果用op1接收
	if (op1 == result) {
		// 清空op1
		zval_ptr_dtor(result);
	}
	// 对 -1取余 ，其实和对 1 取余一样，值一定是0。
	if (op2_lval == -1) {
		// 防止 op1 是最大数时的溢出
		/* Prevent overflow error/crash if op1==LONG_MIN */
		// 结果是0
		ZVAL_LONG(result, 0);
		// 反回 成功
		return SUCCESS;
	}
	// 取余运算，返回结果 
	ZVAL_LONG(result, op1_lval % op2_lval);
	return SUCCESS;
}
/* }}} */

// ing4, 布尔抑或（两个值不同时返回 true）
ZEND_API zend_result ZEND_FASTCALL boolean_xor_function(zval *result, zval *op1, zval *op2) /* {{{ */
{
	int op1_val, op2_val;

	do {
		// 如果第一个是false
		if (Z_TYPE_P(op1) == IS_FALSE) {
			// 第一个值为0
			op1_val = 0;
		// 第一个是true
		} else if (EXPECTED(Z_TYPE_P(op1) == IS_TRUE)) {
			// 第一个值为1
			op1_val = 1;
		// 其他情况
		} else {
			// 第一个引用类型
			if (Z_ISREF_P(op1)) {
				// 追踪引用目标
				op1 = Z_REFVAL_P(op1);
				// 如果第一个是false
				if (Z_TYPE_P(op1) == IS_FALSE) {
					// 第一个值为0
					op1_val = 0;
					break;
				// 第一个是true
				} else if (EXPECTED(Z_TYPE_P(op1) == IS_TRUE)) {
					// 第一个值为1
					op1_val = 1;
					break;
				}
			}
			// 调用第一个的 do_operation方法，执行 ZEND_BOOL_XOR 操作
			ZEND_TRY_BINARY_OP1_OBJECT_OPERATION(ZEND_BOOL_XOR);
			// 第一个转成bool型
			op1_val = zval_is_true(op1);
		}
	} while (0);
	do {
		// 如果第二个是false
		if (Z_TYPE_P(op2) == IS_FALSE) {
			// 第二个值为0
			op2_val = 0;
		// 第二个是true
		} else if (EXPECTED(Z_TYPE_P(op2) == IS_TRUE)) {
			// 第二个值为1
			op2_val = 1;
		// 其他情况	
		} else {
			// 第一个引用类型
			if (Z_ISREF_P(op2)) {
				// 追踪引用目标
				op2 = Z_REFVAL_P(op2);
				// 如果第二个是false
				if (Z_TYPE_P(op2) == IS_FALSE) {
					// 第二个值为0
					op2_val = 0;
					break;
				// 第二个是true
				} else if (EXPECTED(Z_TYPE_P(op2) == IS_TRUE)) {
					// 第二个值为1
					op2_val = 1;
					break;
				}
			}
			// 调用第一个的 do_operation方法，执行 ZEND_BOOL_XOR 操作
			ZEND_TRY_BINARY_OP2_OBJECT_OPERATION(ZEND_BOOL_XOR);
			// 第一个转成bool型
			op2_val = zval_is_true(op2);
		}
	} while (0);
	// 两个都是1个位的整数， 进行 抑或 操作，返回布尔值
	ZVAL_BOOL(result, op1_val ^ op2_val);
	return SUCCESS;
}
/* }}} */

// ing4, ! 操作符，布尔取否（针对undef,false,null,true,object类型）
ZEND_API zend_result ZEND_FASTCALL boolean_not_function(zval *result, zval *op1) /* {{{ */
{
	// 如果小于true：false,null,undef
	if (Z_TYPE_P(op1) < IS_TRUE) {
		// 值为true
		ZVAL_TRUE(result);
	// 如果是true
	} else if (EXPECTED(Z_TYPE_P(op1) == IS_TRUE)) {
		// 值为false
		ZVAL_FALSE(result);
	// 其他情况
	} else {
		// 如果是引用类型
		if (Z_ISREF_P(op1)) {
			// 追踪引用目标
			op1 = Z_REFVAL_P(op1);
			// 如果小于true：false,null,undef
			if (Z_TYPE_P(op1) < IS_TRUE) {
				// 值为true
				ZVAL_TRUE(result);
				return SUCCESS;
			// 如果是true	
			} else if (EXPECTED(Z_TYPE_P(op1) == IS_TRUE)) {
				// 值为false
				ZVAL_FALSE(result);
				return SUCCESS;
			}
		}
		// 这里不用考虑其他类型吗？字串，数组？
		// 其他情况，调用do_operation方法
		ZEND_TRY_UNARY_OBJECT_OPERATION(ZEND_BOOL_NOT);
		// 值转成bool，取反
		ZVAL_BOOL(result, !zval_is_true(op1));
	}
	return SUCCESS;
}
/* }}} */

// ing4, ~ 操作符，一元操作符，二进制取反（测试过，可对字串操作）
ZEND_API zend_result ZEND_FASTCALL bitwise_not_function(zval *result, zval *op1) /* {{{ */
{
try_again:
	// 按类型操作
	switch (Z_TYPE_P(op1)) {
		// 整数 ，直接取反
		case IS_LONG:
			ZVAL_LONG(result, ~Z_LVAL_P(op1));
			return SUCCESS;
		// 小数
		case IS_DOUBLE: {
			// 转成整数 
			zend_long lval = zend_dval_to_lval(Z_DVAL_P(op1));
			// 如果不兼容整数 
			if (!zend_is_long_compatible(Z_DVAL_P(op1), lval)) {
				// 报错：隐式转换，从小数转成整数会丢失精度
				zend_incompatible_double_to_long_error(Z_DVAL_P(op1));
				// 如果有异常
				if (EG(exception)) {
					// 如果不用op1接收结果 
					if (result != op1) {
						// 清空接收变量，中断操作
						ZVAL_UNDEF(result);
					}
					// 返回失败
					return FAILURE;
				}
			}
			// 整数 取反
			ZVAL_LONG(result, ~lval);
			// 返回成功
			return SUCCESS;
		}
		// 字串
		case IS_STRING: {
			size_t i;
			// 如果长度为1
			if (Z_STRLEN_P(op1) == 1) {
				// 对目标 ascii 码取反并转成 uchar型
				zend_uchar not = (zend_uchar) ~*Z_STRVAL_P(op1);
				// 返回char 型结果 
				ZVAL_CHAR(result, not);
			// 长度不为1
			} else {
				// 先分配临时字串
				ZVAL_NEW_STR(result, zend_string_alloc(Z_STRLEN_P(op1), 0));
				// 遍历每个字符
				for (i = 0; i < Z_STRLEN_P(op1); i++) {
					// 逐个字符取反
					Z_STRVAL_P(result)[i] = ~Z_STRVAL_P(op1)[i];
				}
				// 最后再加个 0
				Z_STRVAL_P(result)[i] = 0;
			}
			// 返回成功
			return SUCCESS;
		}
		// 如果是引用类型
		case IS_REFERENCE:
			// 追踪到引用目标
			op1 = Z_REFVAL_P(op1);
			// 从头再来
			goto try_again;
		// 默认情况
		default:
			ZEND_TRY_UNARY_OBJECT_OPERATION(ZEND_BW_NOT);
			// 如果不用op1接收结果
			if (result != op1) {
				// 清空接收变量，中断操作
				ZVAL_UNDEF(result);
			}
			// 报错：不可以在此类型上进行位运算
			zend_type_error("Cannot perform bitwise not on %s", zend_zval_type_name(op1));
			return FAILURE;
	}
}
/* }}} */

// ing4, 整数或字串（1个或多个字符）的 按位 或 运算
ZEND_API zend_result ZEND_FASTCALL bitwise_or_function(zval *result, zval *op1, zval *op2) /* {{{ */
{
	zend_long op1_lval, op2_lval;
	// 如果两个都是整数 
	if (EXPECTED(Z_TYPE_P(op1) == IS_LONG) && EXPECTED(Z_TYPE_P(op2) == IS_LONG)) {
		// 直接或运算
		ZVAL_LONG(result, Z_LVAL_P(op1) | Z_LVAL_P(op2));
		return SUCCESS;
	}
	// 先追踪到引用目标
	ZVAL_DEREF(op1);
	ZVAL_DEREF(op2);
	// 两个都是字串
	if (Z_TYPE_P(op1) == IS_STRING && EXPECTED(Z_TYPE_P(op2) == IS_STRING)) {
		zval *longer, *shorter;
		zend_string *str;
		size_t i;
		// op1 更长
		if (EXPECTED(Z_STRLEN_P(op1) >= Z_STRLEN_P(op2))) {
			// 如果两个都是单字符
			if (EXPECTED(Z_STRLEN_P(op1) == Z_STRLEN_P(op2)) && Z_STRLEN_P(op1) == 1) {
				// 转成char型，进行或运算
				zend_uchar or = (zend_uchar) (*Z_STRVAL_P(op1) | *Z_STRVAL_P(op2));
				// 如果用op1接收结果 
				if (result==op1) {
					// 清空op1
					zval_ptr_dtor_str(result);
				}
				// 更新结果
				ZVAL_CHAR(result, or);
				return SUCCESS;
			}
			// 不都是单字符，按长短区分
			longer = op1;
			shorter = op2;
		} else {
			// 按长短区分
			longer = op2;
			shorter = op1;
		}
		// 再分配一个长的字串
		str = zend_string_alloc(Z_STRLEN_P(longer), 0);
		// 逐个字符做或运算，结果放在新串里
		for (i = 0; i < Z_STRLEN_P(shorter); i++) {
			ZSTR_VAL(str)[i] = Z_STRVAL_P(longer)[i] | Z_STRVAL_P(shorter)[i];
		}
		// 把多出的部分直接copy
		memcpy(ZSTR_VAL(str) + i, Z_STRVAL_P(longer) + i, Z_STRLEN_P(longer) - i + 1);
		// 如果用op1接收结果 
		if (result==op1) {
			// 清空op1
			zval_ptr_dtor_str(result);
		}
		// 更新结果
		ZVAL_NEW_STR(result, str);
		return SUCCESS;
	}

	// 如果op1不是整数
	if (UNEXPECTED(Z_TYPE_P(op1) != IS_LONG)) {
		bool failed;
		// op1 的 do_operation方法，操作符 ZEND_BW_OR
		ZEND_TRY_BINARY_OP1_OBJECT_OPERATION(ZEND_BW_OR);
		/* zval转成整数 ，不支持资源类型 和 数组 */ 
		op1_lval = zendi_try_get_long(op1, &failed);
		// 如果出错了
		if (UNEXPECTED(failed)) {
			// 二进制操作报错,不支持此操作符
			zend_binop_error("|", op1, op2);
			// 如果不用op1接收结果
			if (result != op1) {
				// 清空接收变量，中断操作
				ZVAL_UNDEF(result);
			}
			// 返回失败
			return FAILURE;
		}
	// op1是整数
	} else {
		// 取出整数
		op1_lval = Z_LVAL_P(op1);
	}	
	
	// 如果op2不是整数 
	if (UNEXPECTED(Z_TYPE_P(op2) != IS_LONG)) {
		bool failed;
		// op2 的 do_operation方法，操作符 ZEND_BW_OR
		ZEND_TRY_BINARY_OP2_OBJECT_OPERATION(ZEND_BW_OR);
		// zval转成整数 ，不支持资源类型 和 数组 
		op2_lval = zendi_try_get_long(op2, &failed);
		// 如果出错了
		if (UNEXPECTED(failed)) {
			// 二进制操作报错,不支持此操作符
			zend_binop_error("|", op1, op2);
			// 如果不用op1接收结果
			if (result != op1) {
				// 清空接收变量，中断操作
				ZVAL_UNDEF(result);
			}
			// 返回失败
			return FAILURE;
		}
	// op2是整数
	} else {
		// 取出整数 
		op2_lval = Z_LVAL_P(op2);
	}
	// 如果用op1接收结果
	if (op1 == result) {
		// 先清空op1
		zval_ptr_dtor(result);
	}
	// 两个整数作或运算，把值返回给p1
	ZVAL_LONG(result, op1_lval | op2_lval);
	return SUCCESS;
}
/* }}} */

// ing4 , 整数或字串（1个或多个字符）的 按位与 运算
ZEND_API zend_result ZEND_FASTCALL bitwise_and_function(zval *result, zval *op1, zval *op2) /* {{{ */
{
	zend_long op1_lval, op2_lval;
	// 如果两个都是整数 
	if (EXPECTED(Z_TYPE_P(op1) == IS_LONG) && EXPECTED(Z_TYPE_P(op2) == IS_LONG)) {
		// 直接与运算
		ZVAL_LONG(result, Z_LVAL_P(op1) & Z_LVAL_P(op2));
		return SUCCESS;
	}
	// 先追踪到引用目标
	ZVAL_DEREF(op1);
	ZVAL_DEREF(op2);
	// 两个都是字串
	if (Z_TYPE_P(op1) == IS_STRING && Z_TYPE_P(op2) == IS_STRING) {
		zval *longer, *shorter;
		zend_string *str;
		size_t i;
		// op1 更长
		if (EXPECTED(Z_STRLEN_P(op1) >= Z_STRLEN_P(op2))) {
			// 如果两个都是单字符
			if (EXPECTED(Z_STRLEN_P(op1) == Z_STRLEN_P(op2)) && Z_STRLEN_P(op1) == 1) {
				// 转成char型，进行与运算
				zend_uchar and = (zend_uchar) (*Z_STRVAL_P(op1) & *Z_STRVAL_P(op2));
				// 如果用op1接收结果 
				if (result==op1) {
					// 清空op1
					zval_ptr_dtor_str(result);
				}
				// 更新结果
				ZVAL_CHAR(result, and);
				return SUCCESS;
			}
			// 不都是单字符，按长短区分
			longer = op1;
			shorter = op2;
		} else {
			// 按长短区分
			longer = op2;
			shorter = op1;
		}
		// 再分配一个长的字串
		str = zend_string_alloc(Z_STRLEN_P(shorter), 0);
		// 按短的长度，逐个字符做与运算，结果放在新串里
		for (i = 0; i < Z_STRLEN_P(shorter); i++) {
			ZSTR_VAL(str)[i] = Z_STRVAL_P(shorter)[i] & Z_STRVAL_P(longer)[i];
		}
		// 把多出的部分直接copy
		ZSTR_VAL(str)[i] = 0;
		// 如果用op1接收结果 
		if (result==op1) {
			// 清空op1
			zval_ptr_dtor_str(result);
		}
		// 更新结果
		ZVAL_NEW_STR(result, str);
		return SUCCESS;
	}
	// 如果 op1 不是整数 
	if (UNEXPECTED(Z_TYPE_P(op1) != IS_LONG)) {
		bool failed;
		// op2 的 do_operation方法，操作符 ZEND_BW_AND
		ZEND_TRY_BINARY_OP1_OBJECT_OPERATION(ZEND_BW_AND);
		// zval转成整数 ，不支持资源类型 和 数组 
		op1_lval = zendi_try_get_long(op1, &failed);
		if (UNEXPECTED(failed)) {
			// 二进制操作报错,不支持此操作符
			zend_binop_error("&", op1, op2);
			// 如果不用op1接收结果
			if (result != op1) {
				// 清空接收变量，中断操作
				ZVAL_UNDEF(result);
			}
			return FAILURE;
		}
	// op1是整数
	} else {
		op1_lval = Z_LVAL_P(op1);
	}
	
	// 如果op2不是整数 
	if (UNEXPECTED(Z_TYPE_P(op2) != IS_LONG)) {
		bool failed;
		// op2 的 do_operation方法，操作符 ZEND_BW_AND
		ZEND_TRY_BINARY_OP2_OBJECT_OPERATION(ZEND_BW_AND);
		// zval转成整数 ，不支持资源类型 和 数组 
		op2_lval = zendi_try_get_long(op2, &failed);
		if (UNEXPECTED(failed)) {
			// 二进制操作报错,不支持此操作符
			zend_binop_error("&", op1, op2);
			// 如果不用op1接收结果
			if (result != op1) {
				// 清空接收变量，中断操作
				ZVAL_UNDEF(result);
			}
			// 返回失败
			return FAILURE;
		}
	// op2是整数
	} else {
		// 取出整数 
		op2_lval = Z_LVAL_P(op2);
	}
	// 如果用op1接收结果
	if (op1 == result) {
		// 先清空op1
		zval_ptr_dtor(result);
	}
	// 两个整数作与运算，把值返回给p1
	ZVAL_LONG(result, op1_lval & op2_lval);
	return SUCCESS;
}
/* }}} */

// ing4, 整数或字串（1个或多个字符）的 按位 抑或 运算
ZEND_API zend_result ZEND_FASTCALL bitwise_xor_function(zval *result, zval *op1, zval *op2) /* {{{ */
{
	zend_long op1_lval, op2_lval;
	// 如果两个都是整数 
	if (EXPECTED(Z_TYPE_P(op1) == IS_LONG) && EXPECTED(Z_TYPE_P(op2) == IS_LONG)) {
		// 直接或运算
		ZVAL_LONG(result, Z_LVAL_P(op1) ^ Z_LVAL_P(op2));
		return SUCCESS;
	}
	// 先追踪到引用目标
	ZVAL_DEREF(op1);
	ZVAL_DEREF(op2);
	// 两个都是字串
	if (Z_TYPE_P(op1) == IS_STRING && Z_TYPE_P(op2) == IS_STRING) {
		zval *longer, *shorter;
		zend_string *str;
		size_t i;
		// op1 更长
		if (EXPECTED(Z_STRLEN_P(op1) >= Z_STRLEN_P(op2))) {
			// 如果两个都是单字符
			if (EXPECTED(Z_STRLEN_P(op1) == Z_STRLEN_P(op2)) && Z_STRLEN_P(op1) == 1) {
				// 转成char型，进行抑或运算
				zend_uchar xor = (zend_uchar) (*Z_STRVAL_P(op1) ^ *Z_STRVAL_P(op2));
				// 如果用op1接收结果 
				if (result==op1) {
					// 清空op1
					zval_ptr_dtor_str(result);
				}
				// 更新结果
				ZVAL_CHAR(result, xor);
				return SUCCESS;
			}
			// 不都是单字符，按长短区分
			longer = op1;
			shorter = op2;
		} else {
			// 按长短区分
			longer = op2;
			shorter = op1;
		}
		// 再分配一个长的字串
		str = zend_string_alloc(Z_STRLEN_P(shorter), 0);
		// 按短的长度，逐个字符做抑或运算，结果放在新串里
		for (i = 0; i < Z_STRLEN_P(shorter); i++) {
			ZSTR_VAL(str)[i] = Z_STRVAL_P(shorter)[i] ^ Z_STRVAL_P(longer)[i];
		}
		// 最后放一个0
		ZSTR_VAL(str)[i] = 0;
		// 如果用op1接收结果 
		if (result==op1) {
			// 清空op1
			zval_ptr_dtor_str(result);
		}
		// 更新结果
		ZVAL_NEW_STR(result, str);
		return SUCCESS;
	}

	// 如果op1不是整数
	if (UNEXPECTED(Z_TYPE_P(op1) != IS_LONG)) {
		bool failed;
		// op1 的 do_operation方法，操作符 ZEND_BW_XOR
		ZEND_TRY_BINARY_OP1_OBJECT_OPERATION(ZEND_BW_XOR);
		// zval转成整数 ，不支持资源类型 和 数组 
		op1_lval = zendi_try_get_long(op1, &failed);
		// 如果出错了
		if (UNEXPECTED(failed)) {
			// 二进制操作报错,不支持此操作符
			zend_binop_error("^", op1, op2);
			// 如果不用op1接收结果
			if (result != op1) {
				// 清空接收变量，中断操作
				ZVAL_UNDEF(result);
			}
			// 返回失败
			return FAILURE;
		}
	// op1是整数
	} else {
		// 取出整数
		op1_lval = Z_LVAL_P(op1);
	}
	
	// 如果op2不是整数 
	if (UNEXPECTED(Z_TYPE_P(op2) != IS_LONG)) {
		bool failed;
		// op2 的 do_operation方法，操作符 ZEND_BW_OR
		ZEND_TRY_BINARY_OP2_OBJECT_OPERATION(ZEND_BW_XOR);
		// zval转成整数 ，不支持资源类型 和 数组 
		op2_lval = zendi_try_get_long(op2, &failed);
		// 如果出错了
		if (UNEXPECTED(failed)) {
			// 二进制操作报错,不支持此操作符
			zend_binop_error("^", op1, op2);
			if (result != op1) {
				// 清空接收变量，中断操作
				ZVAL_UNDEF(result);
			}
			// 返回失败
			return FAILURE;
		}
	// op2是整数
	} else {
		op2_lval = Z_LVAL_P(op2);
	}
	// 如果用op1接收结果
	if (op1 == result) {
		// 先清空op1
		zval_ptr_dtor(result);
	}
	// 两个整数作抑或运算，把值返回给p1
	ZVAL_LONG(result, op1_lval ^ op2_lval);
	return SUCCESS;
}
/* }}} */

// ing4, 向左位移
ZEND_API zend_result ZEND_FASTCALL shift_left_function(zval *result, zval *op1, zval *op2) /* {{{ */
{
	zend_long op1_lval, op2_lval;
	// 把op1,op2都转成整数  ，并按要求进行计算
	convert_op1_op2_long(op1, op1_lval, op2, op2_lval, result, ZEND_SL, "<<");

	// 防止某些怪癖的封装，<< 64+x = <<x
	/* prevent wrapping quirkiness on some processors where << 64 + x == << x */
	// 如果位移数 >= 整数位数
	if (UNEXPECTED((zend_ulong)op2_lval >= SIZEOF_ZEND_LONG * 8)) {
		// 如果位移数 > 0
		if (EXPECTED(op2_lval > 0)) {
			// 如果用op1接收结果
			if (op1 == result) {
				// 清空op1
				zval_ptr_dtor(result);
			}
			// 结果为0
			ZVAL_LONG(result, 0);
			return SUCCESS;
		// 位移数不大于0
		} else {
			// 如果在执行时，不在编译时
			if (EG(current_execute_data) && !CG(in_compilation)) {
				// 抛异常 进行负数次位操作 
				zend_throw_exception_ex(zend_ce_arithmetic_error, 0, "Bit shift by negative number");
			} else {
				// 抛异常 进行负数次位操作 
				zend_error_noreturn(E_ERROR, "Bit shift by negative number");
			}
			// 如果不用op1接收结果
			if (op1 != result) {
				// 清空接收变量，中断操作
				ZVAL_UNDEF(result);
			}
			// 返回失败
			return FAILURE;
		}
	}
	// 如果用op1接收结果 
	if (op1 == result) {
		// 清空接收变量
		zval_ptr_dtor(result);
	}
	// 在无符号成员上进行位操作，来取得 定义明确的 封装行为
	/* Perform shift on unsigned numbers to get well-defined wrap behavior. */
	// 转成无符号，进行位操作，再转回来
	ZVAL_LONG(result, (zend_long) ((zend_ulong) op1_lval << op2_lval));
	return SUCCESS;
}
/* }}} */

// ing4, 向右位移
ZEND_API zend_result ZEND_FASTCALL shift_right_function(zval *result, zval *op1, zval *op2) /* {{{ */
{
	zend_long op1_lval, op2_lval;
	// 把op1,op2都转成整数  ，并按要求进行计算
	convert_op1_op2_long(op1, op1_lval, op2, op2_lval, result, ZEND_SR, ">>");

	// 防止某些怪癖的封装，>> 64+x = >>x
	/* prevent wrapping quirkiness on some processors where >> 64 + x == >> x */
	// 如果位移数 >= 整数位数
	if (UNEXPECTED((zend_ulong)op2_lval >= SIZEOF_ZEND_LONG * 8)) {
		// 如果位移数 > 0
		if (EXPECTED(op2_lval > 0)) {
			// 如果用op1接收结果
			if (op1 == result) {
				// 清空op1
				zval_ptr_dtor(result);
			}
			// 位移导致所有位丢失时，最左一位要保留(why)
			ZVAL_LONG(result, (op1_lval < 0) ? -1 : 0);
			return SUCCESS;
		// 位移数不大于0
		} else {
			// 如果在执行时，不在编译时
			if (EG(current_execute_data) && !CG(in_compilation)) {
				// 抛异常 进行负数次位操作 
				zend_throw_exception_ex(zend_ce_arithmetic_error, 0, "Bit shift by negative number");
			} else {
				// 抛异常 进行负数次位操作 
				zend_error_noreturn(E_ERROR, "Bit shift by negative number");
			}
			// 如果不用op1接收结果
			if (op1 != result) {
				// 清空接收变量，中断操作
				ZVAL_UNDEF(result);
			}
			// 返回失败
			return FAILURE;
		}
	}
	// 如果用op1接收结果 
	if (op1 == result) {
		
		zval_ptr_dtor(result);
	}

	ZVAL_LONG(result, op1_lval >> op2_lval);
	return SUCCESS;
}
/* }}} */

// ing3, 连接函数
ZEND_API zend_result ZEND_FASTCALL concat_function(zval *result, zval *op1, zval *op2) /* {{{ */
{
    zval *orig_op1 = op1;
	zval op1_copy, op2_copy;
	// 准备两个临时变量
	ZVAL_UNDEF(&op1_copy);
	ZVAL_UNDEF(&op2_copy);

	do {
		// op1 不是字串
	 	if (UNEXPECTED(Z_TYPE_P(op1) != IS_STRING)) {
			// op1 是引用类型
	 		if (Z_ISREF_P(op1)) {
				// 解引用
	 			op1 = Z_REFVAL_P(op1);
				// 如果是字串，op1处理完毕
	 			if (Z_TYPE_P(op1) == IS_STRING) break;
	 		}
			// 尝试调用op1 和 op2的 do_operation 方法，如果成功直接返回
			// ZEND_CONCAT 是传入的参数
			ZEND_TRY_BINARY_OBJECT_OPERATION(ZEND_CONCAT);
			// 如果上面的操作没成功，获取op1里的字串
			// zval 转成 字串, 没有try
			ZVAL_STR(&op1_copy, zval_get_string_func(op1));
			// 如果有异常
			if (UNEXPECTED(EG(exception))) {
				// 销毁临时变量
				zval_ptr_dtor_str(&op1_copy);
				// 如果不是用 op1 接收结果
				if (orig_op1 != result) {
					// 清空接收变量，中断操作
					ZVAL_UNDEF(result);
				}
				// 返回失败
				return FAILURE;
			}
			// 如果没有异常，并且用op1接收结果
			if (result == op1) {
				// 如果两个操作对象相同
				if (UNEXPECTED(op1 == op2)) {
					// op2 指向op1的副本
					op2 = &op1_copy;
				}
			}
			// op1 指向副本
			op1 = &op1_copy;
		}
	} while (0);
	// 
	do {
		// op2不是字串
		if (UNEXPECTED(Z_TYPE_P(op2) != IS_STRING)) {
			// 如果op2是引用类型
	 		if (Z_ISREF_P(op2)) {
				// op2解引用
	 			op2 = Z_REFVAL_P(op2);
				// 如果是字串，op2处理完毕
	 			if (Z_TYPE_P(op2) == IS_STRING) break;
	 		}
			// 尝试调用 op2的 do_operation 方法，如果成功直接返回
			// ZEND_CONCAT 是传入的参数
			ZEND_TRY_BINARY_OP2_OBJECT_OPERATION(ZEND_CONCAT);
			// zval 转成 字串, 没有try
			ZVAL_STR(&op2_copy, zval_get_string_func(op2));
			// 如果有异常
			if (UNEXPECTED(EG(exception))) {
				// 销毁临时变量
				zval_ptr_dtor_str(&op1_copy);
				zval_ptr_dtor_str(&op2_copy);
				// 如果不是用 op1 接收结果
				if (orig_op1 != result) {
					// 清空接收变量，中断操作
					ZVAL_UNDEF(result);
				}
				// 返回失败
				return FAILURE;
			}
			// op2 指向副本
			op2 = &op2_copy;
		}
	} while (0);

	// op1 长度为0
	if (UNEXPECTED(Z_STRLEN_P(op1) == 0)) {
		// 不是用op2接收结果
		if (EXPECTED(result != op2)) {
			// 如果用原始的op1接收结果
			if (result == orig_op1) {
				// 清空结果 
				i_zval_ptr_dtor(result);
			}
			// 返回 处理后的op2
			ZVAL_COPY(result, op2);
		}
	// op2 长度为0
	} else if (UNEXPECTED(Z_STRLEN_P(op2) == 0)) {
		// 不是用op1接收结果
		if (EXPECTED(result != op1)) {
			// 如果用原始的op1接收结果
			if (result == orig_op1) {
				// 清空结果
				i_zval_ptr_dtor(result);
			}
			// 返回 处理后的op1
			ZVAL_COPY(result, op1);
		}
	// 两个都有长度
	} else {
		// 取得2个长度
		size_t op1_len = Z_STRLEN_P(op1);
		size_t op2_len = Z_STRLEN_P(op2);
		// 长度和
		size_t result_len = op1_len + op2_len;
		// 结果字串
		zend_string *result_str;
		// 如果结果超长
		if (UNEXPECTED(op1_len > ZSTR_MAX_LEN - op2_len)) {
			// 报错：字串溢出
			zend_throw_error(NULL, "String size overflow");
			// 销毁2个副本
			zval_ptr_dtor_str(&op1_copy);
			zval_ptr_dtor_str(&op2_copy);
			// 如果是用原始op1接收结果 
			if (orig_op1 != result) {
				// 清空接收变量，中断操作
				ZVAL_UNDEF(result);
			}
			// 返回错误
			return FAILURE;
		}

		// 如果用op1接收结果 并且 结果可计数
		if (result == op1 && Z_REFCOUNTED_P(result)) {
			// 特殊情况，在结果上计算
			/* special case, perform operations on result */
			// 调整结果字串大小
			result_str = zend_string_extend(Z_STR_P(result), result_len, 0);
		// 不是用op1接收或结果 或 结果不可计数
		} else {
			// 重新分配结果字串
			result_str = zend_string_alloc(result_len, 0);
			// 把op1复制进来
			memcpy(ZSTR_VAL(result_str), Z_STRVAL_P(op1), op1_len);
			// 如果用原op1接收结果
			if (result == orig_op1) {
				// 销毁费根
				i_zval_ptr_dtor(result);
			}
		}
		// 当 result == op1 == op2 并且调整大小已经完成的情况下，需要先做这一步操作。
		// 这种情况下 这一行会更新 Z_STRVAL_P(op2) 成指向新字串。
		// 第一个 op2_len 长度也一样。（主要是为了这个，后面还要用到）
		/* This has to happen first to account for the cases where result == op1 == op2 and
		 * the realloc is done. In this case this line will also update Z_STRVAL_P(op2) to
		 * point to the new string. The first op2_len bytes of result will still be the same. */
		 
		// 把zend_string 添加给 zval，不支持保留字
		ZVAL_NEW_STR(result, result_str);
		// 把op2复制进来
		memcpy(ZSTR_VAL(result_str) + op1_len, Z_STRVAL_P(op2), op2_len);
		// 最后加结束字符 
		ZSTR_VAL(result_str)[result_len] = '\0';
	}
	// 销毁临时变量
	zval_ptr_dtor_str(&op1_copy);
	zval_ptr_dtor_str(&op2_copy);
	// 返回成功
	return SUCCESS;
}
/* }}} */

// ing4, 比较两个字串
ZEND_API int ZEND_FASTCALL string_compare_function_ex(zval *op1, zval *op2, bool case_insensitive) /* {{{ */
{
	zend_string *tmp_str1, *tmp_str2;
	// 取得op1,op2的临时字串
	zend_string *str1 = zval_get_tmp_string(op1, &tmp_str1);
	zend_string *str2 = zval_get_tmp_string(op2, &tmp_str2);
	// 结果 
	int ret;
	// 大小写不敏感
	if (case_insensitive) {
		// 进制比较两个字串，大小写不敏感，长度用str2的长度
		ret = zend_binary_strcasecmp(ZSTR_VAL(str1), ZSTR_LEN(str1), ZSTR_VAL(str2), ZSTR_LEN(str2));
	} else {
		// 进制比较两个字串，长度用str2的长度
		ret = zend_binary_strcmp(ZSTR_VAL(str1), ZSTR_LEN(str1), ZSTR_VAL(str2), ZSTR_LEN(str2));
	}
	// 释放两个临时变量
	zend_tmp_string_release(tmp_str1);
	zend_tmp_string_release(tmp_str2);
	return ret;
}
/* }}} */

// ing4, 比较字串，不是字串的转成字串比较
ZEND_API int ZEND_FASTCALL string_compare_function(zval *op1, zval *op2) /* {{{ */
{
	// 如果是两个字串
	if (EXPECTED(Z_TYPE_P(op1) == IS_STRING) &&
	    EXPECTED(Z_TYPE_P(op2) == IS_STRING)) {
		// 如果指针相同
		if (Z_STR_P(op1) == Z_STR_P(op2)) {
			// 返回相同
			return 0;
		// 否则 ，二进制比较
		} else {
			return zend_binary_strcmp(Z_STRVAL_P(op1), Z_STRLEN_P(op1), Z_STRVAL_P(op2), Z_STRLEN_P(op2));
		}
	// 如果不都是字串
	} else {
		zend_string *tmp_str1, *tmp_str2;
		// 取得op1,op2的临时字串
		zend_string *str1 = zval_get_tmp_string(op1, &tmp_str1);
		zend_string *str2 = zval_get_tmp_string(op2, &tmp_str2);
		// 二进制比较两个临时字串
		int ret = zend_binary_strcmp(ZSTR_VAL(str1), ZSTR_LEN(str1), ZSTR_VAL(str2), ZSTR_LEN(str2));
		// 释放临时字串
		zend_tmp_string_release(tmp_str1);
		zend_tmp_string_release(tmp_str2);
		// 返回结果 
		return ret;
	}
}
/* }}} */

// ing4, 比较字串，不是字串的转成字串比较，大小写不敏感
ZEND_API int ZEND_FASTCALL string_case_compare_function(zval *op1, zval *op2) /* {{{ */
{
	// 两个都是字串
	if (EXPECTED(Z_TYPE_P(op1) == IS_STRING) &&
	    EXPECTED(Z_TYPE_P(op2) == IS_STRING)) {
		// 如果两个字串指针相同
		if (Z_STR_P(op1) == Z_STR_P(op2)) {
			// 返回0
			return 0;
		// 否则
		} else {
			// 二进制比较两个字串，大小写不敏感
			return zend_binary_strcasecmp(Z_STRVAL_P(op1), Z_STRLEN_P(op1), Z_STRVAL_P(op2), Z_STRLEN_P(op2));
		}
	// 两个不都是字串
	} else {
		zend_string *tmp_str1, *tmp_str2;
		// 取得op1,op2的临时字串
		zend_string *str1 = zval_get_tmp_string(op1, &tmp_str1);
		zend_string *str2 = zval_get_tmp_string(op2, &tmp_str2);
		// 二进制比较两个临时字串，大小写不敏感
		int ret = zend_binary_strcasecmp(ZSTR_VAL(str1), ZSTR_LEN(str1), ZSTR_VAL(str2), ZSTR_LEN(str2));
		// 释放临时字串
		zend_tmp_string_release(tmp_str1);
		zend_tmp_string_release(tmp_str2);
		return ret;
	}
}
/* }}} */

// ing3, 调用系统原生方法比较字串，不是字串的转成字串比较（和其他有什么不同）
ZEND_API int ZEND_FASTCALL string_locale_compare_function(zval *op1, zval *op2) /* {{{ */
{
	zend_string *tmp_str1, *tmp_str2;
	// 获取临时字串
	zend_string *str1 = zval_get_tmp_string(op1, &tmp_str1);
	zend_string *str2 = zval_get_tmp_string(op2, &tmp_str2);
	// 调用系统原生方法比较两个字串
	int ret = strcoll(ZSTR_VAL(str1), ZSTR_VAL(str2));
	// 释放临时字串
	zend_tmp_string_release(tmp_str1);
	zend_tmp_string_release(tmp_str2);
	return ret;
}
/* }}} */

// ing4, 比较两个数字
ZEND_API int ZEND_FASTCALL numeric_compare_function(zval *op1, zval *op2) /* {{{ */
{
	double d1, d2;
	// 取出两个小数
	d1 = zval_get_double(op1);
	d2 = zval_get_double(op2);
	// 结果转成 0，1，-1
	// ((n) ? (((n)<0) ? -1 : 1) : 0)
	return ZEND_NORMALIZE_BOOL(d1 - d2);
}
/* }}} */

// ing4, 比较两个变量。p1:返回结果, p2:变量1, p3:变量2
ZEND_API zend_result ZEND_FASTCALL compare_function(zval *result, zval *op1, zval *op2) /* {{{ */
{
	ZVAL_LONG(result, zend_compare(op1, op2));
	return SUCCESS;
}
/* }}} */

// ing4, 整数和字串比较
static int compare_long_to_string(zend_long lval, zend_string *str) /* {{{ */
{
	zend_long str_lval;
	double str_dval;
	// 字串转成数字
	zend_uchar type = is_numeric_string(ZSTR_VAL(str), ZSTR_LEN(str), &str_lval, &str_dval, 0);
	// 如果能转成整数 
	if (type == IS_LONG) {
		// 直接比较大小
		return lval > str_lval ? 1 : lval < str_lval ? -1 : 0;
	}
	// 如果能转成小数
	if (type == IS_DOUBLE) {
		// 转成小数算减法
		double diff = (double) lval - str_dval;
		// 结果转成 0，1，-1
		// ((n) ? (((n)<0) ? -1 : 1) : 0)
		return ZEND_NORMALIZE_BOOL(diff);
	}
	// 整数转成字串
	zend_string *lval_as_str = zend_long_to_str(lval);
	// 比较两个字串
	int cmp_result = zend_binary_strcmp(
		ZSTR_VAL(lval_as_str), ZSTR_LEN(lval_as_str), ZSTR_VAL(str), ZSTR_LEN(str));
	// 释放临时字串
	zend_string_release(lval_as_str);
	// 结果转成 0，1，-1
	// ((n) ? (((n)<0) ? -1 : 1) : 0)
	return ZEND_NORMALIZE_BOOL(cmp_result);
}
/* }}} */

// ing4, 小数和字串比较
static int compare_double_to_string(double dval, zend_string *str) /* {{{ */
{
	zend_long str_lval;
	double str_dval;
	// 字串转成数字
	zend_uchar type = is_numeric_string(ZSTR_VAL(str), ZSTR_LEN(str), &str_lval, &str_dval, 0);
	// 如果转成整数
	if (type == IS_LONG) {
		// 转成小数算减法
		double diff = dval - (double) str_lval;
		// 结果转成 0，1，-1
		// ((n) ? (((n)<0) ? -1 : 1) : 0)
		return ZEND_NORMALIZE_BOOL(diff);
	}
	// 如果转成小数
	if (type == IS_DOUBLE) {
		//相等直接返回
		if (dval == str_dval) {
			return 0;
		}
		// 结果转成 0，1，-1
		// ((n) ? (((n)<0) ? -1 : 1) : 0)
		return ZEND_NORMALIZE_BOOL(dval - str_dval);
	}
	// 小数转成字串
	zend_string *dval_as_str = zend_double_to_str(dval);
	// 比较两个字串
	int cmp_result = zend_binary_strcmp(
		ZSTR_VAL(dval_as_str), ZSTR_LEN(dval_as_str), ZSTR_VAL(str), ZSTR_LEN(str));
	// 释放临时字串
	zend_string_release(dval_as_str);
	// 结果转成 0，1，-1
	// ((n) ? (((n)<0) ? -1 : 1) : 0)
	return ZEND_NORMALIZE_BOOL(cmp_result);
}
/* }}} */

// ing3, 比较两个内置变量
ZEND_API int ZEND_FASTCALL zend_compare(zval *op1, zval *op2) /* {{{ */
{
	int converted = 0;
	zval op1_copy, op2_copy;

	while (1) {
		// 按两个变量的类型，适配比较方法
		switch (TYPE_PAIR(Z_TYPE_P(op1), Z_TYPE_P(op2))) {
			// ing4, 两个整型
			case TYPE_PAIR(IS_LONG, IS_LONG):
				// 取回zval 里的 long 型，直接比较
				return Z_LVAL_P(op1)>Z_LVAL_P(op2)?1:(Z_LVAL_P(op1)<Z_LVAL_P(op2)?-1:0);

			// ing4, 一个double 一个 long, 两个都转成 double 进行比较
			case TYPE_PAIR(IS_DOUBLE, IS_LONG):
				// 转成比较结果 0 1 -1
				return ZEND_NORMALIZE_BOOL(Z_DVAL_P(op1) - (double)Z_LVAL_P(op2));
			// ing4, 同上
			case TYPE_PAIR(IS_LONG, IS_DOUBLE):
				// 转成比较结果 0 1 -1
				return ZEND_NORMALIZE_BOOL((double)Z_LVAL_P(op1) - Z_DVAL_P(op2));
				
			// ing4, 两个 double
			case TYPE_PAIR(IS_DOUBLE, IS_DOUBLE):
				if (Z_DVAL_P(op1) == Z_DVAL_P(op2)) {
					return 0;
				} else {
					// 结果转成 0，1，-1
					// ((n) ? (((n)<0) ? -1 : 1) : 0)
					return ZEND_NORMALIZE_BOOL(Z_DVAL_P(op1) - Z_DVAL_P(op2));
				}
			
			// ing3, 两个 array
			case TYPE_PAIR(IS_ARRAY, IS_ARRAY):
				// 用 hash_zval_compare_function 函数比较两个哈希表
				return zend_compare_arrays(op1, op2);

			// ing4, null 和 false 相等。（测试过，但在javascript 里 null和false 不相等）
			case TYPE_PAIR(IS_NULL, IS_NULL):
			case TYPE_PAIR(IS_NULL, IS_FALSE):
			case TYPE_PAIR(IS_FALSE, IS_NULL):
			case TYPE_PAIR(IS_FALSE, IS_FALSE):
			case TYPE_PAIR(IS_TRUE, IS_TRUE):
				return 0;
			// ing4, true > null
			case TYPE_PAIR(IS_NULL, IS_TRUE):
				return -1;
			// ing4, 同上
			case TYPE_PAIR(IS_TRUE, IS_NULL):
				return 1;
				
			// ing4, 两个 string
			case TYPE_PAIR(IS_STRING, IS_STRING):
				// 指针目标相同
				if (Z_STR_P(op1) == Z_STR_P(op2)) {
					return 0;
				}
				// 先按数字比较，不能按数字再按字串比较
				return zendi_smart_strcmp(Z_STR_P(op1), Z_STR_P(op2));

			// ing4, string 和 null 比较。空string 和null 相等
			case TYPE_PAIR(IS_NULL, IS_STRING):
				return Z_STRLEN_P(op2) == 0 ? 0 : -1;
			// ing4, 同上
			case TYPE_PAIR(IS_STRING, IS_NULL):
				return Z_STRLEN_P(op1) == 0 ? 0 : 1;

			// ing4, string 和 整型比较
			case TYPE_PAIR(IS_LONG, IS_STRING):
				return compare_long_to_string(Z_LVAL_P(op1), Z_STR_P(op2));
			// ing4, 同上
			case TYPE_PAIR(IS_STRING, IS_LONG):
				return -compare_long_to_string(Z_LVAL_P(op2), Z_STR_P(op1));

			// ing4, string 和 双精度比较
			case TYPE_PAIR(IS_DOUBLE, IS_STRING):
				if (zend_isnan(Z_DVAL_P(op1))) {
					return 1;
				}

				return compare_double_to_string(Z_DVAL_P(op1), Z_STR_P(op2));
			// ing4, 同上
			case TYPE_PAIR(IS_STRING, IS_DOUBLE):
				if (zend_isnan(Z_DVAL_P(op2))) {
					return 1;
				}
			
				return -compare_double_to_string(Z_DVAL_P(op2), Z_STR_P(op1));

			// ing4, object > null
			case TYPE_PAIR(IS_OBJECT, IS_NULL):
				return 1;
			// ing4, 同上  null < object
			case TYPE_PAIR(IS_NULL, IS_OBJECT):
				return -1;
				
			// 其它情况
			default:
				// ing4, 如果有一个是引用，追踪引用对象，然后重来
				if (Z_ISREF_P(op1)) {
					op1 = Z_REFVAL_P(op1);
					continue;
				} else if (Z_ISREF_P(op2)) {
					op2 = Z_REFVAL_P(op2);
					continue;
				}
				
				// ing4, 如果两个都是对象切指针相同，返回相同
				if (Z_TYPE_P(op1) == IS_OBJECT
				 && Z_TYPE_P(op2) == IS_OBJECT
				 && Z_OBJ_P(op1) == Z_OBJ_P(op2)) {
					return 0;
					
				// ing4, 如果op1或op2是对象，调用它的 compare方法
				} else if (Z_TYPE_P(op1) == IS_OBJECT) {
					return Z_OBJ_HANDLER_P(op1, compare)(op1, op2);
				} else if (Z_TYPE_P(op2) == IS_OBJECT) {
					return Z_OBJ_HANDLER_P(op2, compare)(op1, op2);
				}
				// 如果没有转换过
				if (!converted) {
					// ing4, p1 或 p2 有一个是 true false null
					// p1是 false 或 null
					if (Z_TYPE_P(op1) < IS_TRUE) {
						// p2是ture，算p2大，其他情况算相等
						return zval_is_true(op2) ? -1 : 0;
					// p1 是 true
					} else if (Z_TYPE_P(op1) == IS_TRUE) {
						// p2 是 true 算相等，其他算 p1 大
						return zval_is_true(op2) ? 0 : 1;
					// p2 是 false 或 null, 
					} else if (Z_TYPE_P(op2) < IS_TRUE) {
						// p1是ture，算p1大，其他情况算相等
						return zval_is_true(op1) ? 1 : 0;
					// p2 是 true	
					} else if (Z_TYPE_P(op2) == IS_TRUE) {
						// p1 是 true 算相等，其他算 p2 大
						return zval_is_true(op1) ? 0 : -1;
					// ing3, 其他情况
					} else {
						// 标量转换成数字，结果放在 holder 里，全程不报错，不转换数组，对象转换失败时返回1。
						op1 = _zendi_convert_scalar_to_number_silent(op1, &op1_copy);
						// 标量转换成数字，结果放在 holder 里，全程不报错，不转换数组，对象转换失败时返回1。
						op2 = _zendi_convert_scalar_to_number_silent(op2, &op2_copy);
						// 如果有异常
						if (EG(exception)) {
							// 停止比较array（这个异常只能是数组比较造成的？）
							return 1; /* to stop comparison of arrays */
						}
						// 标记成转换过
						converted = 1;
						// 从头来
					}
				// ing4, 数组更大
				} else if (Z_TYPE_P(op1)==IS_ARRAY) {
					return 1;
				// ing4, 数组更大
				} else if (Z_TYPE_P(op2)==IS_ARRAY) {
					return -1;
				// ing4, 其他情况，报错
				} else {
					ZEND_UNREACHABLE();
					// 不支持的操作对象
					zend_throw_error(NULL, "Unsupported operand types");
					return 1;
				}
		}
	}
}
/* }}} */

// 返回结果与 compare_func_t 兼容
/* return int to be compatible with compare_func_t */
// ing4, 比较两个zval是否相同
static int hash_zval_identical_function(zval *z1, zval *z2) /* {{{ */
{
	// is_identical_function 相同时返回1，不同时返回0
	// 不同于比较函数 ，相同时返回0，其他情况不是0。
	/* is_identical_function() returns 1 in case of identity and 0 in case
	 * of a difference;
	 * whereas this comparison function is expected to return 0 on identity,
	 * and non zero otherwise.
	 */
	// 先追踪到引用目标
	ZVAL_DEREF(z1);
	ZVAL_DEREF(z2);
	// 比较两个zval
	return fast_is_not_identical_function(z1, z2);
}
/* }}} */

// ing4, ===
ZEND_API bool ZEND_FASTCALL zend_is_identical(zval *op1, zval *op2) /* {{{ */
{
	// 如果类型不一样，直接返回 false
	if (Z_TYPE_P(op1) != Z_TYPE_P(op2)) {
		return 0;
	}
	// 
	switch (Z_TYPE_P(op1)) {
		// null ,true,false 返回 true
		case IS_NULL:
		case IS_FALSE:
		case IS_TRUE:
			return 1;
		// 整数，转成 long 型比较
		case IS_LONG:
			return (Z_LVAL_P(op1) == Z_LVAL_P(op2));
		// 资源，比 .res 引用指针
		case IS_RESOURCE:
			return (Z_RES_P(op1) == Z_RES_P(op2));
		// 双精度
		case IS_DOUBLE:
			return (Z_DVAL_P(op1) == Z_DVAL_P(op2));
		// 字串
		case IS_STRING:
			return zend_string_equals(Z_STR_P(op1), Z_STR_P(op2));
		// 数组，先比指针。不同再比内容
		case IS_ARRAY:
			return (Z_ARRVAL_P(op1) == Z_ARRVAL_P(op2) ||
				zend_hash_compare(Z_ARRVAL_P(op1), Z_ARRVAL_P(op2), (compare_func_t) hash_zval_identical_function, 1) == 0);
		// 对象，必须指针相同
		case IS_OBJECT:
			return (Z_OBJ_P(op1) == Z_OBJ_P(op2));
		// 其他类型直接 false
		default:
			return 0;
	}
}
/* }}} */

// ing3, ===
ZEND_API zend_result ZEND_FASTCALL is_identical_function(zval *result, zval *op1, zval *op2) /* {{{ */
{
	ZVAL_BOOL(result, zend_is_identical(op1, op2));
	return SUCCESS;
}
/* }}} */

// ing3, !==
ZEND_API zend_result ZEND_FASTCALL is_not_identical_function(zval *result, zval *op1, zval *op2) /* {{{ */
{
	ZVAL_BOOL(result, !zend_is_identical(op1, op2));
	return SUCCESS;
}
/* }}} */

// ing3, ==
ZEND_API zend_result ZEND_FASTCALL is_equal_function(zval *result, zval *op1, zval *op2) /* {{{ */
{
	ZVAL_BOOL(result, zend_compare(op1, op2) == 0);
	return SUCCESS;
}
/* }}} */

// ing3, !=
ZEND_API zend_result ZEND_FASTCALL is_not_equal_function(zval *result, zval *op1, zval *op2) /* {{{ */
{
	ZVAL_BOOL(result, (zend_compare(op1, op2) != 0));
	return SUCCESS;
}
/* }}} */

// ing3, 操作符 <
ZEND_API zend_result ZEND_FASTCALL is_smaller_function(zval *result, zval *op1, zval *op2) /* {{{ */
{
	ZVAL_BOOL(result, (zend_compare(op1, op2) < 0));
	return SUCCESS;
}
/* }}} */

// ing3, <=
ZEND_API zend_result ZEND_FASTCALL is_smaller_or_equal_function(zval *result, zval *op1, zval *op2) /* {{{ */
{
	ZVAL_BOOL(result, (zend_compare(op1, op2) <= 0));
	return SUCCESS;
}
/* }}} */

// ing4, 检验类 是否实现 某接口
ZEND_API bool ZEND_FASTCALL zend_class_implements_interface(const zend_class_entry *class_ce, const zend_class_entry *interface_ce) /* {{{ */
{
	uint32_t i;
	// 接口的标记 必须接口
	ZEND_ASSERT(interface_ce->ce_flags & ZEND_ACC_INTERFACE);
	// 如果有实现接口数量 
	if (class_ce->num_interfaces) {
		// 类必须要有实现接口标记
		ZEND_ASSERT(class_ce->ce_flags & ZEND_ACC_RESOLVED_INTERFACES);
		// 遍历每个接口
		for (i = 0; i < class_ce->num_interfaces; i++) {
			// 如果找到了目标接口
			if (class_ce->interfaces[i] == interface_ce) {
				// 返回true
				return 1;
			}
		}
	}
	// 没有实现，直接返回 false
	return 0;
}
/* }}} */

// ing4, 检验继承或接口实现，单向
ZEND_API bool ZEND_FASTCALL instanceof_function_slow(const zend_class_entry *instance_ce, const zend_class_entry *ce) /* {{{ */
{
	// 两个类必须不同，否则应该已经验证过了
	ZEND_ASSERT(instance_ce != ce && "Should have been checked already");
	// 如果ce是接口
	if (ce->ce_flags & ZEND_ACC_INTERFACE) {
		uint32_t i;
		// 如果有实现接口数量 
		if (instance_ce->num_interfaces) {
			// 类必须有实现接口
			ZEND_ASSERT(instance_ce->ce_flags & ZEND_ACC_RESOLVED_INTERFACES);
			// 遍历每个接口
			for (i = 0; i < instance_ce->num_interfaces; i++) {
				// 如果相符
				if (instance_ce->interfaces[i] == ce) {
					// 返回true
					return 1;
				}
			}
		}
		return 0;
	// 如果ce不是接口
	} else {
		while (1) {
			// 追踪到父类
			instance_ce = instance_ce->parent;
			// 如果找到
			if (instance_ce == ce) {
				// 返回true
				return 1;
			}
			// 如果到最后也找不到
			if (instance_ce == NULL) {
				// 返回false
				return 0;
			}
		}
	}
}
/* }}} */

#define LOWER_CASE 1
#define UPPER_CASE 2
#define NUMERIC 3

// perl风格的字串自增
// Perl style string increment
// '0'++ => '1', 'a'++ => 'b'
// 'z'++ => 'aa', 'z9'++ => 'aa0'
// '{9'++ => '{0'
// ing4, 大小写和数字字串的自增。perl风格。
// 这个东西没什么问题，但还不够直观，应该很少用到吧
static void ZEND_FASTCALL increment_string(zval *str) /* {{{ */
{
	int carry=0;
	// 位置为 字串长度-1
	size_t pos=Z_STRLEN_P(str)-1;
	char *s;
	zend_string *t;
	// 关闭编译 警告
	int last=0; /* Shut up the compiler warning */
	int ch;
	// 如果长度为0
	if (Z_STRLEN_P(str) == 0) {
		// 销毁字串
		zval_ptr_dtor_str(str);
		// 值更新为 '1'
		ZVAL_CHAR(str, '1');
		// 返回
		return;
	}

	// 如果 zval 不可计数
	if (!Z_REFCOUNTED_P(str)) {
		// 取出里面的string，创建 非永久 zend_string 副本, 并关联到str
		Z_STR_P(str) = zend_string_init(Z_STRVAL_P(str), Z_STRLEN_P(str), 0);
		// 类型为 string
		Z_TYPE_INFO_P(str) = IS_STRING_EX;
	// 如果 zval 引用计数 > 1
	} else if (Z_REFCOUNT_P(str) > 1) {
		// 只有在分配成功后释放string
		/* Only release string after allocation succeeded. */
		// 取出 zend_string
		zend_string *orig_str = Z_STR_P(str);
		// 创建新的字串 副本
		Z_STR_P(str) = zend_string_init(Z_STRVAL_P(str), Z_STRLEN_P(str), 0);
		// 原字串减少引用次数
		GC_DELREF(orig_str);
	// 如果可计数，计数是1（或0）
	} else {
		// 删除 zend_string 的哈希值
		zend_string_forget_hash_val(Z_STR_P(str));
	}
	// 取出 字串（可能是副本或原本）
	s = Z_STRVAL_P(str);

	// 遍历
	do {
		// 倒序遍历
		ch = s[pos];
		// a-z
		if (ch >= 'a' && ch <= 'z') {
			// 如果字符是z（直接自增会变成 { ）
			if (ch == 'z') {
				// 转成 a
				s[pos] = 'a';
				// 需要进位
				carry=1;
			// 其他，直接自增（c语言里char型可以 按ascii码序自增，测试过）
			} else {
				// ascii 码自增
				s[pos]++;
				// 不需要进位
				carry=0;
			}
			// 最后一个是小写字符
			last=LOWER_CASE;
		// A-Z
		} else if (ch >= 'A' && ch <= 'Z') {
			// 如果字符是Z（直接自增会变成 [ ）
			if (ch == 'Z') {
				// 转成 Z
				s[pos] = 'A';
				// 需要进位
				carry=1;
			// 其他，直接自增（c语言里char型可以 按ascii码序自增，测试过）
			} else {
				// ascii 码自增
				s[pos]++;
				// 不需要进位
				carry=0;
			}
			// 最后一个是大写字符
			last=UPPER_CASE;
		// 0-9
		} else if (ch >= '0' && ch <= '9') {
			// 如果字符是Z（直接自增会变成 [ ）
			if (ch == '9') {
				// 转成 0
				s[pos] = '0';
				// 需要进位
				carry=1;
			// 其他，直接自增（c语言里char型可以 按ascii码序自增，测试过）
			} else {
				// ascii 码自增
				s[pos]++;
				// 不需要进位
				carry=0;
			}
			// 最后一个是数字
			last = NUMERIC;
		// 其他字串
		} else {
			// 无越界
			carry=0;
			// 自增运算已完成
			break;
		}
		// 如果没有 字符z/Z/9 自增带来的 进位。
		if (carry == 0) {
			// 自增运算已完成
			break;
		}
	// 一直到头
	} while (pos-- > 0);
	
	// 如果有 字符z/Z/9 自增带来的 进位
	if (carry) {
		// 分配新串，多要1位
		t = zend_string_alloc(Z_STRLEN_P(str)+1, 0);
		// 新串最前面空1位，复制全部
		memcpy(ZSTR_VAL(t) + 1, Z_STRVAL_P(str), Z_STRLEN_P(str));
		// 最后加个\0
		ZSTR_VAL(t)[Z_STRLEN_P(str) + 1] = '\0';
		// 按操作类型
		switch (last) {
			// 数字，最前面加1
			case NUMERIC:
				ZSTR_VAL(t)[0] = '1';
				break;
			// 大写字母，最前面加A
			case UPPER_CASE:
				ZSTR_VAL(t)[0] = 'A';
				break;
			// 小写字母，最前面加A
			case LOWER_CASE:
				ZSTR_VAL(t)[0] = 'a';
				break;
		}
		// 释放 str 中的字串
		zend_string_free(Z_STR_P(str));
		// 把新字串关联到str
		ZVAL_NEW_STR(str, t);
	}
}
/* }}} */

// ing3, 自增函数
ZEND_API zend_result ZEND_FASTCALL increment_function(zval *op1) /* {{{ */
{
try_again:
	// 根据类型操作
	switch (Z_TYPE_P(op1)) {
		// 整数 ，快速自增
		case IS_LONG:
			fast_long_increment_function(op1);
			break;
		// 小数 +1
		case IS_DOUBLE:
			Z_DVAL_P(op1) = Z_DVAL_P(op1) + 1;
			break;
		// null 变成1 （测试过）
		case IS_NULL:
			ZVAL_LONG(op1, 1);
			break;
		// 字串
		case IS_STRING: {
				zend_long lval;
				double dval;
				// 转成数值
				switch (is_numeric_str_function(Z_STR_P(op1), &lval, &dval)) {
					// 如果成整数
					case IS_LONG:
						// 清空 op1
						zval_ptr_dtor_str(op1);
						// 如果是最大整数
						if (lval == ZEND_LONG_MAX) {
							// 转成小数
							/* switch to double */
							double d = (double)lval;
							// +1后保存成小数
							ZVAL_DOUBLE(op1, d+1);
						// 普通整数
						} else {
							// +1
							ZVAL_LONG(op1, lval+1);
						}
						break;
					// 小数
					case IS_DOUBLE:
						// 清空 op1
						zval_ptr_dtor_str(op1);
						// +1后保存成小数
						ZVAL_DOUBLE(op1, dval+1);
						break;
					// 其他情况
					default:
						// perl 风格的字串自增
						/* Perl style string increment */
						increment_string(op1);
						break;
				}
			}
			break;
		// false,true，不能自增（测试过）
		case IS_FALSE:
		case IS_TRUE:
			/* Do nothing. */
			break;
		// 如果是引用
		case IS_REFERENCE:
			// 追踪到引用目标
			op1 = Z_REFVAL_P(op1);
			goto try_again;
		// 对象
		case IS_OBJECT:
			// 如果有 do_operation方法
			if (Z_OBJ_HANDLER_P(op1, do_operation)) {
				zval op2;
				// 临时数值1
				ZVAL_LONG(&op2, 1);
				// 计算加法
				if (Z_OBJ_HANDLER_P(op1, do_operation)(ZEND_ADD, op1, op1, &op2) == SUCCESS) {
					return SUCCESS;
				}
			}
			ZEND_FALLTHROUGH;
		// 资源或数字 
		case IS_RESOURCE:
		case IS_ARRAY:
			// 报错，不可自增
			zend_type_error("Cannot increment %s", zend_zval_type_name(op1));
			return FAILURE;
		EMPTY_SWITCH_DEFAULT_CASE()
	}
	return SUCCESS;
}
/* }}} */

// ing3, 自减
ZEND_API zend_result ZEND_FASTCALL decrement_function(zval *op1) /* {{{ */
{
	zend_long lval;
	double dval;

try_again:
	// 按类型操作
	switch (Z_TYPE_P(op1)) {
		// 整数
		case IS_LONG:
			// 快速自减
			fast_long_decrement_function(op1);
			break;
		// 小数
		case IS_DOUBLE:
			// 直接算 -1
			Z_DVAL_P(op1) = Z_DVAL_P(op1) - 1;
			break;
		// 字串。和perl一样 只支持字串自减
		case IS_STRING:		/* Like perl we only support string increment */
			// 如果长度为0
			if (Z_STRLEN_P(op1) == 0) { /* consider as 0 */
				// 清空
				zval_ptr_dtor_str(op1);
				// 结果为 -1
				ZVAL_LONG(op1, -1);
				break;
			}
			// 转成整数 
			switch (is_numeric_str_function(Z_STR_P(op1), &lval, &dval)) {
				// 整数
				case IS_LONG:
					// 清空op1
					zval_ptr_dtor_str(op1);
					// 等于最小整数
					if (lval == ZEND_LONG_MIN) {
						// 转成小数
						double d = (double)lval;
						// -1，返回值为小数
						ZVAL_DOUBLE(op1, d-1);
					// 可以算减法
					} else {
						// -1
						ZVAL_LONG(op1, lval-1);
					}
					break;
				// 小数
				case IS_DOUBLE:
					// 清空op1
					zval_ptr_dtor_str(op1);
					// -1
					ZVAL_DOUBLE(op1, dval - 1);
					break;
			}
			// 不是数字的字串，自减无效（测试过）
			break;
		// null,false,true. 自减无效.（测试过）
		case IS_NULL:
		case IS_FALSE:
		case IS_TRUE:
			/* Do nothing. */
			break;
		// 引用类型
		case IS_REFERENCE:
			// 追踪到引用对象
			op1 = Z_REFVAL_P(op1);
			// 从头再来
			goto try_again;
		// 对象
		case IS_OBJECT:
			// 如果有 do_operation
			if (Z_OBJ_HANDLER_P(op1, do_operation)) {
				zval op2;
				// 临时变量值为整数 1
				ZVAL_LONG(&op2, 1);
				// 按-1计算
				if (Z_OBJ_HANDLER_P(op1, do_operation)(ZEND_SUB, op1, op1, &op2) == SUCCESS) {
					// 成功
					return SUCCESS;
				}
				// 失败
			}
			// 这里什么也不做。
			// 经测试 stdclass 对象自减会报错 ？是因为没有 do_operation 方法，但怎么抛错的？
			ZEND_FALLTHROUGH;
		// 资源或数组
		case IS_RESOURCE:
		case IS_ARRAY:
			// 抛错，不可做自减
			zend_type_error("Cannot decrement %s", zend_zval_type_name(op1));
			return FAILURE;
		EMPTY_SWITCH_DEFAULT_CASE()
	}

	return SUCCESS;
}
/* }}} */

// ing4, 验证各种类型是否为 true
ZEND_API int ZEND_FASTCALL zend_is_true(zval *op) /* {{{ */
{
	return (int) i_zend_is_true(op);
}
/* }}} */

// ing4, 对象算bool值
ZEND_API bool ZEND_FASTCALL zend_object_is_true(zval *op) /* {{{ */
{
	// 取得zval里的对象
	zend_object *zobj = Z_OBJ_P(op);
	zval tmp;
	// 调用 cast_object
	if (zobj->handlers->cast_object(zobj, &tmp, _IS_BOOL) == SUCCESS) {
		return Z_TYPE(tmp) == IS_TRUE;
	}
	// 如果转换失败，报错：此对象无法转为bool型
	zend_error(E_RECOVERABLE_ERROR, "Object of class %s could not be converted to bool", ZSTR_VAL(zobj->ce->name));
	return false;
}
/* }}} */

// ing2, 设置字符集相关变量，CG(variable_width_locale) 和 CG(ascii_compatible_locale)
ZEND_API void zend_update_current_locale(void) /* {{{ */
{
//  windows有这个
#ifdef ZEND_USE_TOLOWER_L
//  windows走这里
# if defined(ZEND_WIN32) && defined(_MSC_VER)
	// 取得本地字符集（测试过）
	current_locale = _get_current_locale();
# else
	current_locale = uselocale(0);
# endif
#endif
// windows系统走这里（测试过）
#if defined(ZEND_WIN32) && defined(_MSC_VER)
	// 我的windows， MB_CUR_MAX = 1
	if (MB_CUR_MAX > 1) {
		unsigned int cp = ___lc_codepage_func();
		CG(variable_width_locale) = 1;
		// TODO: EUC-* are also ASCII compatible ???
		CG(ascii_compatible_locale) =
			cp == 65001; /* UTF-8 */
	// 走这里, 只走这面这两行。（测试过）
	} else {
		// 这东西似乎也只有这里用到
		CG(variable_width_locale) = 0;
		// ascii 本地兼容
		CG(ascii_compatible_locale) = 1;
	}
// 不需要再走这里了
#elif defined(MB_CUR_MAX)
	/* Check if current locale uses variable width encoding */
	if (MB_CUR_MAX > 1) {
// windows没有这个
#if HAVE_NL_LANGINFO
		const char *charmap = nl_langinfo(CODESET);
#else
		char buf[16];
		const char *charmap = NULL;
		const char *locale = setlocale(LC_CTYPE, NULL);

		if (locale) {
			const char *dot = strchr(locale, '.');
			const char *modifier;

			if (dot) {
				dot++;
				modifier = strchr(dot, '@');
				if (!modifier) {
					charmap = dot;
				} else if (modifier - dot < sizeof(buf)) {
					memcpy(buf, dot, modifier - dot);
                    buf[modifier - dot] = '\0';
                    charmap = buf;
				}
			}
		}
#endif
		CG(variable_width_locale) = 1;
		CG(ascii_compatible_locale) = 0;

		if (charmap) {
			size_t len = strlen(charmap);
			static const char *ascii_compatible_charmaps[] = {
				"utf-8",
				"utf8",
				// TODO: EUC-* are also ASCII compatible ???
				NULL
			};
			const char **p;
			/* Check if current locale is ASCII compatible */
			for (p = ascii_compatible_charmaps; *p; p++) {
				if (zend_binary_strcasecmp(charmap, len, *p, strlen(*p)) == 0) {
					CG(ascii_compatible_locale) = 1;
					break;
				}
			}
		}

	} else {
		CG(variable_width_locale) = 0;
		CG(ascii_compatible_locale) = 1;
	}
// 也不需要走这里
#else
	/* We can't determine current charset. Assume the worst case */
	CG(variable_width_locale) = 1;
	CG(ascii_compatible_locale) = 0;
#endif
}
/* }}} */


// ing3, 调用操作系统方法设置字符集
ZEND_API void zend_reset_lc_ctype_locale(void)
{
	// 使用 C.UTF-8 编码，让readline可以读取utf-8的输入，不会牵涉到 PHP的 单字节的 本地依赖函数
	/* Use the C.UTF-8 locale so that readline can process UTF-8 input, while not interfering
	 * with single-byte locale-dependent functions used by PHP. */
	// 如果使用 C.UTF8不成功
	if (!setlocale(LC_CTYPE, "C.UTF-8")) {
		// 如果使用 C
		setlocale(LC_CTYPE, "C");
	}
}

// ing2, 字符串 str 转小写后放在 dest 里
static zend_always_inline void zend_str_tolower_impl(char *dest, const char *str, size_t length) /* {{{ */ {
	unsigned char *p = (unsigned char*)str;
	unsigned char *q = (unsigned char*)dest;
	unsigned char *end = p + length;
// windows没有这个东西
#ifdef HAVE_BLOCKCONV
	if (length >= BLOCKCONV_STRIDE) {
		BLOCKCONV_INIT_RANGE('A', 'Z');
		BLOCKCONV_INIT_DELTA('a' - 'A');
		do {
			BLOCKCONV_LOAD(p);
			BLOCKCONV_STORE(q);
			p += BLOCKCONV_STRIDE;
			q += BLOCKCONV_STRIDE;
		} while (p + BLOCKCONV_STRIDE <= end);
	}
#endif
	// 逐个字符转小写
	while (p < end) {
		// 单个字符转小写
		*q++ = zend_tolower_ascii(*p++);
	}
}
/* }}} */

// ing4, 返回字串大写副本
static zend_always_inline void zend_str_toupper_impl(char *dest, const char *str, size_t length) /* {{{ */ {
	// 原字串
	unsigned char *p = (unsigned char*)str;
	// 目标字串
	unsigned char *q = (unsigned char*)dest;
	// 原字串结尾
	unsigned char *end = p + length;
// windows没有这个东西
#ifdef HAVE_BLOCKCONV
	if (length >= BLOCKCONV_STRIDE) {
		BLOCKCONV_INIT_RANGE('a', 'z');
		BLOCKCONV_INIT_DELTA('A' - 'a');
		do {
			BLOCKCONV_LOAD(p);
			BLOCKCONV_STORE(q);
			p += BLOCKCONV_STRIDE;
			q += BLOCKCONV_STRIDE;
		} while (p + BLOCKCONV_STRIDE <= end);
	}
#endif
	// 逐个字符操作
	while (p < end) {
		// 转成大写，指针指向下一个
		*q++ = zend_toupper_ascii(*p++);
	}
}
/* }}} */

// ing3 返回目标字串的小写副本（副本要自己创建并传入）,外部调用
ZEND_API char* ZEND_FASTCALL zend_str_tolower_copy(char *dest, const char *source, size_t length) /* {{{ */
{
	// source 转小写后放在 dest 里
	zend_str_tolower_impl(dest, source, length);
	dest[length] = '\0';
	return dest;
}
/* }}} */

// ing4, 复制大写副本
ZEND_API char* ZEND_FASTCALL zend_str_toupper_copy(char *dest, const char *source, size_t length) /* {{{ */
{
	zend_str_toupper_impl(dest, source, length);
	dest[length] = '\0';
	return dest;
}
/* }}} */

// ing4, 返回创建好的小写副本
ZEND_API char* ZEND_FASTCALL zend_str_tolower_dup(const char *source, size_t length) /* {{{ */
{
	return zend_str_tolower_copy((char *)emalloc(length+1), source, length);
}
/* }}} */

// ing4, 返回创建好的大写副本
ZEND_API char* ZEND_FASTCALL zend_str_toupper_dup(const char *source, size_t length) /* {{{ */
{
	return zend_str_toupper_copy((char *)emalloc(length+1), source, length);
}
/* }}} */

// ing4, 字串转小写
ZEND_API void ZEND_FASTCALL zend_str_tolower(char *str, size_t length) /* {{{ */
{
	zend_str_tolower_impl(str, (const char*)str, length);
}
/* }}} */

// ing4, 字串转大写
ZEND_API void ZEND_FASTCALL zend_str_toupper(char *str, size_t length) /* {{{ */
{
	zend_str_toupper_impl(str, (const char*)str, length);
}
/* }}} */

// ing2, 创建并返回小写副本，如果本来就是小写，返回null
ZEND_API char* ZEND_FASTCALL zend_str_tolower_dup_ex(const char *source, size_t length) /* {{{ */
{
	const unsigned char *p = (const unsigned char*)source;
	const unsigned char *end = p + length;
	// 遍历每个字符
	while (p < end) {
		// 如果转成小写后有变化。只要有一个字符有变化，就需要创建副本
		if (*p != zend_tolower_ascii(*p)) {
			// 分配内存创建新字串
			char *res = (char*)emalloc(length + 1);
			unsigned char *r;
			// 如果指针不在开头（已有跳过去的字串）
			if (p != (const unsigned char*)source) {
				// 把前面检查过的长度全复制进来
				memcpy(res, source, p - (const unsigned char*)source);
			}
			// 后面的部分的指针 ??
			r = (unsigned char*)p + (res - source);
			// 后面的部分全转小写
			zend_str_tolower_impl((char *)r, (const char*)p, end - p);
			// 添加结束字符
			res[length] = '\0';
			// 返回
			return res;
		}
		//
		p++;
	}
	// 走到这里说明原本就都是大写，什么也没做
	return NULL;
}
/* }}} */

// ing2, 创建并返回大写副本，如果本来就是大写，返回null
ZEND_API char* ZEND_FASTCALL zend_str_toupper_dup_ex(const char *source, size_t length) /* {{{ */
{
	const unsigned char *p = (const unsigned char*)source;
	const unsigned char *end = p + length;
	// 遍历每个字符
	while (p < end) {
		// 如果转成大写后有变化。只要有一个字符有变化，就需要创建副本
		if (*p != zend_toupper_ascii(*p)) {
			// 分配内存创建新字串
			char *res = (char*)emalloc(length + 1);
			unsigned char *r;
			// 如果指针不在开头（已有跳过去的字串）
			if (p != (const unsigned char*)source) {
				// 把前面检查过的长度全复制进来
				memcpy(res, source, p - (const unsigned char*)source);
			}
			// 后面的部分的指针 ??
			r = (unsigned char*)p + (res - source);
			// 后面的部分全转小写
			zend_str_toupper_impl((char *)r, (const char*)p, end - p);
			// 添加结束字符
			res[length] = '\0';
			// 返回
			return res;
		}
		//
		p++;
	}
	// 走到这里说明原本就都是大写，什么也没做
	return NULL;
}
/* }}} */

// ing3, 如果转换后有变化，创建并返回小写副本。如果原本就都是小写，增加引用次数并 返回原字串
ZEND_API zend_string* ZEND_FASTCALL zend_string_tolower_ex(zend_string *str, bool persistent) /* {{{ */
{
	size_t length = ZSTR_LEN(str);
	unsigned char *p = (unsigned char *) ZSTR_VAL(str);
	unsigned char *end = p + length;
// windows没有这个东西
#ifdef HAVE_BLOCKCONV
	BLOCKCONV_INIT_RANGE('A', 'Z');
	while (p + BLOCKCONV_STRIDE <= end) {
		BLOCKCONV_LOAD(p);
		if (BLOCKCONV_FOUND()) {
			zend_string *res = zend_string_alloc(length, persistent);
			memcpy(ZSTR_VAL(res), ZSTR_VAL(str), p - (unsigned char *) ZSTR_VAL(str));
			unsigned char *q = (unsigned char*) ZSTR_VAL(res) + (p - (unsigned char*) ZSTR_VAL(str));

			/* Lowercase the chunk we already compared. */
			BLOCKCONV_INIT_DELTA('a' - 'A');
			BLOCKCONV_STORE(q);

			/* Lowercase the rest of the string. */
			p += BLOCKCONV_STRIDE;
			q += BLOCKCONV_STRIDE;
			zend_str_tolower_impl((char *) q, (const char *) p, end - p);
			ZSTR_VAL(res)[length] = '\0';
			return res;
		}
		p += BLOCKCONV_STRIDE;
	}
#endif
	// 遍历每个字符
	while (p < end) {
		// 如果转成小写后有变化。只要有一个字符有变化，就需要创建副本
		if (*p != zend_tolower_ascii(*p)) {
			// 分配内存创建新字串
			zend_string *res = zend_string_alloc(length, persistent);
			// 把前面检查过的长度全复制进来
			memcpy(ZSTR_VAL(res), ZSTR_VAL(str), p - (unsigned char*) ZSTR_VAL(str));
			// q是新串的指针
			unsigned char *q = (unsigned char*) ZSTR_VAL(res) + (p - (unsigned char*) ZSTR_VAL(str));
			// 后面的字串全转小写
			while (p < end) {
				// 逐个转成小写
				*q++ = zend_tolower_ascii(*p++);
			}
			// 添加结束字符
			ZSTR_VAL(res)[length] = '\0';
			// 返回新创建的小写副本
			return res;
		}
		// 下一个字符
		p++;
	}
	// 走到这里说明原本就都是小写，增加引用次数并 返回原字串
	return zend_string_copy(str);
}
/* }}} */

// ing3, 如果转换后有变化，创建并返回大写副本。如果原本就都是大写，增加引用次数并 返回原字串
ZEND_API zend_string* ZEND_FASTCALL zend_string_toupper_ex(zend_string *str, bool persistent) /* {{{ */
{
	size_t length = ZSTR_LEN(str);
	unsigned char *p = (unsigned char *) ZSTR_VAL(str);
	unsigned char *end = p + length;
// windows没有这个东西
#ifdef HAVE_BLOCKCONV
	BLOCKCONV_INIT_RANGE('a', 'z');
	while (p + BLOCKCONV_STRIDE <= end) {
		BLOCKCONV_LOAD(p);
		if (BLOCKCONV_FOUND()) {
			zend_string *res = zend_string_alloc(length, persistent);
			memcpy(ZSTR_VAL(res), ZSTR_VAL(str), p - (unsigned char *) ZSTR_VAL(str));
			unsigned char *q = (unsigned char *) ZSTR_VAL(res) + (p - (unsigned char *) ZSTR_VAL(str));

			/* Uppercase the chunk we already compared. */
			BLOCKCONV_INIT_DELTA('A' - 'a');
			BLOCKCONV_STORE(q);

			/* Uppercase the rest of the string. */
			p += BLOCKCONV_STRIDE;
			q += BLOCKCONV_STRIDE;
			zend_str_toupper_impl((char *) q, (const char *) p, end - p);
			ZSTR_VAL(res)[length] = '\0';
			return res;
		}
		p += BLOCKCONV_STRIDE;
	}
#endif
	// 遍历每个字符
	while (p < end) {
		// 如果转成大写后有变化。只要有一个字符有变化，就需要创建副本
		if (*p != zend_toupper_ascii(*p)) {
			// 分配内存创建新字串
			zend_string *res = zend_string_alloc(length, persistent);
			// 把前面检查过的长度全复制进来
			memcpy(ZSTR_VAL(res), ZSTR_VAL(str), p - (unsigned char*) ZSTR_VAL(str));
			// q是新串的指针
			unsigned char *q = (unsigned char *) ZSTR_VAL(res) + (p - (unsigned char *) ZSTR_VAL(str));
			// 后面的字串全转大写
			while (p < end) {
				// 逐个转成大写
				*q++ = zend_toupper_ascii(*p++);
			}
			// 添加结束字符
			ZSTR_VAL(res)[length] = '\0';
			// 返回新创建的大写副本
			return res;
		}
		// 下一个字符
		p++;
	}
	// 走到这里说明原本就都是大写，增加引用次数并 返回原字串
	return zend_string_copy(str);
}
/* }}} */

// ing4, 进制比较两个字串
ZEND_API int ZEND_FASTCALL zend_binary_strcmp(const char *s1, size_t len1, const char *s2, size_t len2) /* {{{ */
{
	int retval;

	// 如果两个指针相同，返回0
	if (s1 == s2) {
		return 0;
	}
	// 在内存中按位比较，比较长度是两个字串中短的一个
	retval = memcmp(s1, s2, MIN(len1, len2));
	// 如果结果是0 （这时两个长度可能不同）
	if (!retval) {
		// #define ZEND_THREEWAY_COMPARE(a, b) ((a) == (b) ? 0 : ((a) < (b) ? -1 : 1))
		return ZEND_THREEWAY_COMPARE(len1, len2);
	// 结果不是0，直接返回
	} else {
		return retval;
	}
}
/* }}} */

// ing4, 进制比较两个字串，指定比较长度
ZEND_API int ZEND_FASTCALL zend_binary_strncmp(const char *s1, size_t len1, const char *s2, size_t len2, size_t length) /* {{{ */
{
	int retval;
	// 如果两个指针相同，返回0
	if (s1 == s2) {
		return 0;
	}
	// 在内存中按位比较，比较长度是3长度中最短的一个
	retval = memcmp(s1, s2, MIN(length, MIN(len1, len2)));
	// 如果结果是0 （这时两个长度可能不同）
	if (!retval) {
		// #define ZEND_THREEWAY_COMPARE(a, b) ((a) == (b) ? 0 : ((a) < (b) ? -1 : 1))
		return ZEND_THREEWAY_COMPARE(MIN(length, len1), MIN(length, len2));
	// 结果不是0，直接返回
	} else {
		return retval;
	}
}
/* }}} */

// ing4, 逐个字符二进制比较两个字符串，忽略大小写。s1>s2:正数, s1<s2:负数, s1==s2:0
ZEND_API int ZEND_FASTCALL zend_binary_strcasecmp(const char *s1, size_t len1, const char *s2, size_t len2) /* {{{ */
{
	size_t len;
	int c1, c2;
	// 如果是指向同一个地址，返回0
	if (s1 == s2) {
		return 0;
	}
	// 取得最小长度
	len = MIN(len1, len2);
	while (len--) {
		// 两个字串从头开始逐个字符比较，忽略大小写
		// 逐个取出字符
		c1 = zend_tolower_ascii(*(unsigned char *)s1++);
		c2 = zend_tolower_ascii(*(unsigned char *)s2++);
		if (c1 != c2) {
			return c1 - c2;
		}
	}
	// 如果前面一段全相等
	// #define ZEND_THREEWAY_COMPARE(a, b) ((a) == (b) ? 0 : ((a) < (b) ? -1 : 1))
	return ZEND_THREEWAY_COMPARE(len1, len2);
}
/* }}} */

// ing4, 逐个字符二进制比较两个字符串，忽略大小写。指定长度
ZEND_API int ZEND_FASTCALL zend_binary_strncasecmp(const char *s1, size_t len1, const char *s2, size_t len2, size_t length) /* {{{ */
{
	size_t len;
	int c1, c2;
	// 如果是指向同一个地址，返回0
	if (s1 == s2) {
		return 0;
	}
	// 取得最小长度
	len = MIN(length, MIN(len1, len2));
	while (len--) {
		/ 两个字串从头开始逐个字符比较，忽略大小写
		// 逐个取出字符
		c1 = zend_tolower_ascii(*(unsigned char *)s1++);
		c2 = zend_tolower_ascii(*(unsigned char *)s2++);
		if (c1 != c2) {
			return c1 - c2;
		}
	}
	// #define ZEND_THREEWAY_COMPARE(a, b) ((a) == (b) ? 0 : ((a) < (b) ? -1 : 1))
	return ZEND_THREEWAY_COMPARE(MIN(length, len1), MIN(length, len2));
}
/* }}} */

// ing4, 逐个字符二进制比较两个字符串，忽略大小写。两个都要传入长度
ZEND_API int ZEND_FASTCALL zend_binary_strcasecmp_l(const char *s1, size_t len1, const char *s2, size_t len2) /* {{{ */
{
	size_t len;
	int c1, c2;
	// 如果是指向同一个地址，返回0
	if (s1 == s2) {
		return 0;
	}
	// // 取得最小长度
	len = MIN(len1, len2);
	// 
	while (len--) {
		// 两个字串从头开始逐个字符比较，忽略大小写
		// 逐个取出字符
		c1 = zend_tolower((int)*(unsigned char *)s1++);
		c2 = zend_tolower((int)*(unsigned char *)s2++);
		if (c1 != c2) {
			return c1 - c2;
		}
	}
	// #define ZEND_THREEWAY_COMPARE(a, b) ((a) == (b) ? 0 : ((a) < (b) ? -1 : 1))
	return ZEND_THREEWAY_COMPARE(len1, len2);
}
/* }}} */

// ing4, 逐个字符二进制比较两个字符串，忽略大小写。指定长度 和 比较长度
ZEND_API int ZEND_FASTCALL zend_binary_strncasecmp_l(const char *s1, size_t len1, const char *s2, size_t len2, size_t length) /* {{{ */
{
	size_t len;
	int c1, c2;
	// 如果是指向同一个地址，返回0
	if (s1 == s2) {
		return 0;
	}
	len = MIN(length, MIN(len1, len2));
	while (len--) {
		// 两个字串从头开始逐个字符比较，忽略大小写
		// 逐个取出字符
		c1 = zend_tolower((int)*(unsigned char *)s1++);
		c2 = zend_tolower((int)*(unsigned char *)s2++);
		if (c1 != c2) {
			return c1 - c2;
		}
	}
	// #define ZEND_THREEWAY_COMPARE(a, b) ((a) == (b) ? 0 : ((a) < (b) ? -1 : 1))
	return ZEND_THREEWAY_COMPARE(MIN(length, len1), MIN(length, len2));
}
/* }}} */

// ing4, 比较两个zval里的string，不用传长度
ZEND_API int ZEND_FASTCALL zend_binary_zval_strcmp(zval *s1, zval *s2) /* {{{ */
{
	return zend_binary_strcmp(Z_STRVAL_P(s1), Z_STRLEN_P(s1), Z_STRVAL_P(s2), Z_STRLEN_P(s2));
}
/* }}} */

// ing4, 二进制比较字串，第三个参数是长度
ZEND_API int ZEND_FASTCALL zend_binary_zval_strncmp(zval *s1, zval *s2, zval *s3) /* {{{ */
{
	return zend_binary_strncmp(Z_STRVAL_P(s1), Z_STRLEN_P(s1), Z_STRVAL_P(s2), Z_STRLEN_P(s2), Z_LVAL_P(s3));
}
/* }}} */

// ing3，智能比较两个字串是否相等（常用的好功能）
ZEND_API bool ZEND_FASTCALL zendi_smart_streq(zend_string *s1, zend_string *s2) /* {{{ */
{
	zend_uchar ret1, ret2;
	int oflow1, oflow2;
	zend_long lval1 = 0, lval2 = 0;
	double dval1 = 0.0, dval2 = 0.0;
	// 两个数都转成数值
	if ((ret1 = is_numeric_string_ex(s1->val, s1->len, &lval1, &dval1, false, &oflow1, NULL)) &&
		(ret2 = is_numeric_string_ex(s2->val, s2->len, &lval2, &dval2, false, &oflow2, NULL))) {
//64位
#if ZEND_ULONG_MAX == 0xFFFFFFFF
		// 第一个有溢出 并且 第二个也有溢出，并且 两个小数值相同 并且 （第一个是正溢出且大于最大小数，或第一个是负溢出且小于最小小数）
		if (oflow1 != 0 && oflow1 == oflow2 && dval1 - dval2 == 0. &&
			((oflow1 == 1 && dval1 > 9007199254740991. /*0x1FFFFFFFFFFFFF*/)
			|| (oflow1 == -1 && dval1 < -9007199254740991.))) {
// 32位
#else
		if (oflow1 != 0 && oflow1 == oflow2 && dval1 - dval2 == 0.) {
#endif
			// 如果两个整数都溢出，并且向同一方向，并且 小数比较可能会丢失关键正确性。
			/* both values are integers overflown to the same side, and the
			 * double comparison may have resulted in crucial accuracy lost */
			// 转换到字串比较
			goto string_cmp;
		}
		// 如果两个里有一个是小数
		if ((ret1 == IS_DOUBLE) || (ret2 == IS_DOUBLE)) {
			// 第一个不是小数
			if (ret1 != IS_DOUBLE) {
				// 如果第二个是小数，并且溢出了
				if (oflow2) {
					// 第二个是 大于最大整数 或小于最小整数
					/* 2nd operand is integer > LONG_MAX (oflow2==1) or < LONG_MIN (-1) */
					// 返回0 ,不相等
					return 0;
				}
				// 第1个转成小数
				dval1 = (double) lval1;
			// 第二个不是小数
			} else if (ret2 != IS_DOUBLE) {
				// 如果第一个是小数，并且溢出了
				if (oflow1) {
					// 反回0 ,不相等
					return 0;
				}
				// 第2个转成小数
				dval2 = (double) lval2;
			// 都是小数且相等，但不是有限小数
			} else if (dval1 == dval2 && !zend_finite(dval1)) {
				/* Both values overflowed and have the same sign,
				 * so a numeric comparison would be inaccurate */
				// 转到字串比较
				goto string_cmp;
			}
			// 到这里两个都是小数了
			// 比较两个小数是否相等
			return dval1 == dval2;
		// 如果两个都是整数
		} else { /* they both have to be long's */
			// 比较两个整数是否相等
			return lval1 == lval2;
		}
	} else {
string_cmp:
		// 比较两个字串是否相等
		return zend_string_equal_content(s1, s2);
	}
}
/* }}} */

// ing3, 智能比较两个字串的大小。如果能按数字比较，先按数字比较，不能才按字串比较。
// 唯一的问题是下面两个数是怎么来的。
ZEND_API int ZEND_FASTCALL zendi_smart_strcmp(zend_string *s1, zend_string *s2) /* {{{ */
{
	zend_uchar ret1, ret2;
	int oflow1, oflow2;
	zend_long lval1 = 0, lval2 = 0;
	double dval1 = 0.0, dval2 = 0.0;
	// 如果 s1,s2都能转成数字
	if ((ret1 = is_numeric_string_ex(s1->val, s1->len, &lval1, &dval1, false, &oflow1, NULL)) &&
		(ret2 = is_numeric_string_ex(s2->val, s2->len, &lval2, &dval2, false, &oflow2, NULL))) {
// 如果是64位
#if ZEND_ULONG_MAX == 0xFFFFFFFF
		// 第一个有溢出 并且 第二个也有溢出，并且 两个小数值相同 并且 （第一个是正溢出且大于最大小数，或第一个是负溢出且小于最小小数）
		if (oflow1 != 0 && oflow1 == oflow2 && dval1 - dval2 == 0. &&
			((oflow1 == 1 && dval1 > 9007199254740991. /*0x1FFFFFFFFFFFFF*/)
			|| (oflow1 == -1 && dval1 < -9007199254740991.))) {
// 32位
#else
		if (oflow1 != 0 && oflow1 == oflow2 && dval1 - dval2 == 0.) {
#endif
			// 如果两个整数都溢出，并且向同一方向，并且 小数比较可能会丢失关键正确性。
			/* both values are integers overflowed to the same side, and the
			 * double comparison may have resulted in crucial accuracy lost */
			// 转换到字串比较
			goto string_cmp;
		}
		// 如果两个里有一个是小数
		if ((ret1 == IS_DOUBLE) || (ret2 == IS_DOUBLE)) {
			// 第一个不是小数
			if (ret1 != IS_DOUBLE) {
				// 如果第二个是小数，并且溢出了
				if (oflow2) {
					// 第二个是 大于最大整数 或小于最小整数
					/* 2nd operand is integer > LONG_MAX (oflow2==1) or < LONG_MIN (-1) */
					// 把第二个的溢出方向反过来，得到的是比较结果 。
					return -1 * oflow2;
				}
				// 第1个转成小数
				dval1 = (double) lval1;
			// 第二个不是小数
			} else if (ret2 != IS_DOUBLE) {
				// 如果第一个是小数，并且溢出了
				if (oflow1) {
					// 第一个是 大于最大整数 或小于最小整数
					// 第一个溢出方向直接就是比较结果 。
					return oflow1;
				}
				// 第2个转成小数
				dval2 = (double) lval2;
			// 都是小数且相等，但不是有限小数
			} else if (dval1 == dval2 && !zend_finite(dval1)) {
				/* Both values overflowed and have the same sign,
				 * so a numeric comparison would be inaccurate */
				// 转到字串比较
				goto string_cmp;
			}
			// 到这里两个都是小数了
			// 计算两个数的差
			dval1 = dval1 - dval2;
			// 结果转成 -1，0，1
			return ZEND_NORMALIZE_BOOL(dval1);
		// 如果两个都是整数
		} else { /* they both have to be long's */
			// 直接比，这个最简单
			return lval1 > lval2 ? 1 : (lval1 < lval2 ? -1 : 0);
		}
	// 如果s1,s2不是都能转成数字 
	} else {
		int strcmp_ret;
string_cmp:
		// 二进制比较两个字串
		strcmp_ret = zend_binary_strcmp(s1->val, s1->len, s2->val, s2->len);
		// 结果转成 -1，0，1
		return ZEND_NORMALIZE_BOOL(strcmp_ret);
	}
}
/* }}} */

// clear
static int hash_zval_compare_function(zval *z1, zval *z2) /* {{{ */
{
	return zend_compare(z1, z2);
}
/* }}} */

// ing3, 用 hash_zval_compare_function 函数比较两个哈希表
ZEND_API int ZEND_FASTCALL zend_compare_symbol_tables(HashTable *ht1, HashTable *ht2) /* {{{ */
{
	return ht1 == ht2 ? 0 : zend_hash_compare(ht1, ht2, (compare_func_t) hash_zval_compare_function, 0);
}
/* }}} */

// ing3, 比较两个哈希表，传入的是 zval
ZEND_API int ZEND_FASTCALL zend_compare_arrays(zval *a1, zval *a2) /* {{{ */
{
	return zend_compare_symbol_tables(Z_ARRVAL_P(a1), Z_ARRVAL_P(a2));
}
/* }}} */

// ing3, 比较两个 object
ZEND_API int ZEND_FASTCALL zend_compare_objects(zval *o1, zval *o2) /* {{{ */
{
	// 指针目标相同
	if (Z_OBJ_P(o1) == Z_OBJ_P(o2)) {
		return 0;
	}
	// 如果第一个对象没有比较方法，认为第一个大
	if (Z_OBJ_HT_P(o1)->compare == NULL) {
		return 1;
	// 否则调用比较方法
	} else {
		return Z_OBJ_HT_P(o1)->compare(o1, o2);
	}
}
/* }}} */

// ing3, zend_long 型转 string
ZEND_API zend_string* ZEND_FASTCALL zend_long_to_str(zend_long num) /* {{{ */
{
	// 10以内数字
	if ((zend_ulong)num <= 9) {
		// ?? 为什么要做加法 ？
		return ZSTR_CHAR((zend_uchar)'0' + (zend_uchar)num);
	} else {
		// 创建字串
		char buf[MAX_LENGTH_OF_LONG + 1];
		// num打印到字串里，关键是打印。 指针移到buffer最后一位
		char *res = zend_print_long_to_buf(buf + sizeof(buf) - 1, num);
		// 创建 zend_string 对象并返回。
		// ? buf 没有销毁
		return zend_string_init(res, buf + sizeof(buf) - 1 - res, 0);
	}
}
/* }}} */

// buf，指针在 buffer 结尾。
// ing3, zend_ulong 型转 string。逻辑与上面类似
ZEND_API zend_string* ZEND_FASTCALL zend_ulong_to_str(zend_ulong num)
{
	// 10 以内数字
	if (num <= 9) {
		return ZSTR_CHAR((zend_uchar)'0' + (zend_uchar)num);
	} else {
		char buf[MAX_LENGTH_OF_LONG + 1];
		// 
		char *res = zend_print_ulong_to_buf(buf + sizeof(buf) - 1, num);
		return zend_string_init(res, buf + sizeof(buf) - 1 - res, 0);
	}
}


/* buf points to the END of the buffer */
// ing3, 64 位整数转 string
static zend_always_inline char *zend_print_u64_to_buf(char *buf, uint64_t num64) {
// 如果默认 long长度 8字节（64位）
#if SIZEOF_ZEND_LONG == 8
	return zend_print_ulong_to_buf(buf, num64);
// 32位
#else
	*buf = '\0';
	// 如果数字大于 zend_ulong 最大值，先直接写入buffer
	while (num64 > ZEND_ULONG_MAX) {
		*--buf = (char) (num64 % 10) + '0';
		num64 /= 10;
	}

	// 再转成 zend_ulong，写入剩余部分。为啥这么麻烦？
	zend_ulong num = (zend_ulong) num64;
	do {
		*--buf = (char) (num % 10) + '0';
		num /= 10;
	} while (num > 0);
	return buf;
#endif
}

// buf，指针在 buffer 结尾
/* buf points to the END of the buffer */
// ing3, 有符号，64位整数转 字串
static zend_always_inline char *zend_print_i64_to_buf(char *buf, int64_t num) {
	// 负数，先转成正数
	if (num < 0) {
		// 写入buffer，再在前面加负号
	    char *result = zend_print_u64_to_buf(buf, ~((uint64_t) num) + 1);
	    *--result = '-';
		return result;
	// 正数直接转
	} else {
	    return zend_print_u64_to_buf(buf, num);
	}
}

// ing3, 无符号，64位整数转string
ZEND_API zend_string* ZEND_FASTCALL zend_u64_to_str(uint64_t num)
{
	if (num <= 9) {
		return ZSTR_CHAR((zend_uchar)'0' + (zend_uchar)num);
	} else {
		char buf[20 + 1];
		char *res = zend_print_u64_to_buf(buf + sizeof(buf) - 1, num);
		return zend_string_init(res, buf + sizeof(buf) - 1 - res, 0);
	}
}

// ing3, 有符号，64位整数转string
ZEND_API zend_string* ZEND_FASTCALL zend_i64_to_str(int64_t num)
{
	if ((uint64_t)num <= 9) {
		return ZSTR_CHAR((zend_uchar)'0' + (zend_uchar)num);
	} else {
		char buf[20 + 1];
		char *res = zend_print_i64_to_buf(buf + sizeof(buf) - 1, num);
		return zend_string_init(res, buf + sizeof(buf) - 1 - res, 0);
	}
}

// ing3, 双精度转string
ZEND_API zend_string* ZEND_FASTCALL zend_double_to_str(double num)
{
	// ZEND_DOUBLE_MAX_LENGTH 1080 ,够长的
	char buf[ZEND_DOUBLE_MAX_LENGTH];
	// 小数位数，应该是在Ini里设置的
	/* Model snprintf precision behavior. */
	// # define GLOBAL_CONSTANTS_TABLE		EG(zend_constants)
	int precision = (int) EG(precision);
	// 默认1位小数。 zend_strtod.c 中，逻辑复杂
	zend_gcvt(num, precision ? precision : 1, '.', 'E', buf);
	// buff 转成 zend_string 返回
	return zend_string_init(buf, strlen(buf), 0);
}

// ing4, 验证是否是 数字字串。接收转换后的值，
ZEND_API zend_uchar ZEND_FASTCALL is_numeric_str_function(const zend_string *str, zend_long *lval, double *dval) /* {{{ */
{
	// -> is_numeric_string_ex -> _is_numeric_string_ex
	// allow_errors = false, 有错时返回0
	return is_numeric_string(ZSTR_VAL(str), ZSTR_LEN(str), lval, dval, false);
}
/* }}} */

// 详细说明在 zend_operators.h里。数字处理想想似乎简单，实际处理起来非常麻烦。下面这些业务逻辑其实只处理了整数 ，小数在 zend_strtod
// ing3, 验证是否是 数字字串。传入 str字串，length长度，lval接收返回值用的整数，dval接收返回值用的小数，
	// allow_errors是否允许错误，oflow_info是否溢出，trailing_data 结尾是否有无效数据
ZEND_API zend_uchar ZEND_FASTCALL _is_numeric_string_ex(const char *str, size_t length, zend_long *lval,
	double *dval, bool allow_errors, int *oflow_info, bool *trailing_data) /* {{{ */
{
	const char *ptr;
	int digits = 0, dp_or_e = 0;
	double local_dval = 0.0;
	zend_uchar type;
	zend_ulong tmp_lval = 0;
	int neg = 0;

	// 空字串，返回否
	if (!length) {
		return 0;
	}
	// 初始化引用返回值，溢出信息。正数溢出，返回1，负数溢出返回-1，无溢出返回0。
	if (oflow_info != NULL) {
		*oflow_info = 0;
	}
	// 初始化引用返回值
	if (trailing_data != NULL) {
		*trailing_data = false;
	}

	// 跳过开头的空格，这个比 isspace() 快很多
	/* Skip any whitespace 
	 * This is much faster than the isspace() function */
	// 如果碰到空字符 
	while (*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r' || *str == '\v' || *str == '\f') {
		// 指针后移
		str++;
		// 长度-1
		length--;
	}
	// 这时候指针在有效数字的开头
	ptr = str;
	// 先处理正负号
	// 如果字符是 -
	if (*ptr == '-') {
		// 负数
		neg = 1;
		// 下一个字符 
		ptr++;
	// 如果字符是 +
	} else if (*ptr == '+') {
		// 下一个字符 
		ptr++;
	}
	// #define ZEND_IS_DIGIT(c) ((c) >= \'0\' && (c) <= \'9\')
	// 如果开头是字符 0-9
	if (ZEND_IS_DIGIT(*ptr)) {
		// 跳过所有开头的0
		/* Skip any leading 0s */
		while (*ptr == '0') {
			ptr++;
		}

		// 计算数字数量。 如果 如果找到小数点或 e(科学计数法)，当成小数处理。 
		// 其他情况，如果有小数或不需要匹配整个字串， 碰到够长的整数就停止。
		/* Count the number of digits. If a decimal point/exponent is found,
		 * it's a double. Otherwise, if there's a dval or no need to check for
		 * a full match, stop when there are too many digits for a long */
		// 类型是整数。逐个字符处理。
		for (type = IS_LONG; !(digits >= MAX_LENGTH_OF_LONG && (dval || allow_errors)); digits++, ptr++) {
// 检查数字
check_digits:
			// 如果字符是数字 
			if (ZEND_IS_DIGIT(*ptr)) {
				// 计算成整数（前面的*10，加这最后一位）
				tmp_lval = tmp_lval * 10 + (*ptr) - '0';
				// 下一个数字 
				continue;
			// 如果字符是 . 并且 没碰到过小数点和 e （e后面不能再有小数点）
			} else if (*ptr == '.' && dp_or_e < 1) {
				// 处理小数
				goto process_double;
			// 如果字符是 E或e 并且 没碰到过 e
			} else if ((*ptr == 'e' || *ptr == 'E') && dp_or_e < 2) {
				// e后面的字符
				const char *e = ptr + 1;
				// 如果是正负号
				if (*e == '-' || *e == '+') {
					// 再向右移1个字符
					ptr = e++;
				}
				// 如果碰到数字 
				if (ZEND_IS_DIGIT(*e)) {
					// 处理小数
					goto process_double;
				}
			}
			
			// 其他情况：碰到无效字符，到此为止，break;
			break;
		}
		// 如果大于最大整数长度（超长会转成小数处理）
		if (digits >= MAX_LENGTH_OF_LONG) {
			// 如果接收 溢出信息
			if (oflow_info != NULL) {
				// 溢出信息：负数 -1，正数 1
				*oflow_info = *str == '-' ? -1 : 1;
			}
			// 长度溢出，值为 -1
			dp_or_e = -1;
			// 处理小数
			goto process_double;
		}
	// 如果开头是 . 并且下一个字符是数字
	} else if (*ptr == '.' && ZEND_IS_DIGIT(ptr[1])) {
// 处理小数
process_double:
		// 类型为小数
		type = IS_DOUBLE;
		// 如果有接收用的小数变量，转换小数，否则 如果需要完全匹配的话，检查数字 
		/* If there's a dval, do the conversion; else continue checking
		 * the digits if we need to check for a full match */
		// 如果有接收用的小数变量
		if (dval) {
			// 转成小数
			local_dval = zend_strtod(str, &ptr);
		// 如果没有接收用的小数变量，并且不允许出错 并且长度没有溢出
		} else if (!allow_errors && dp_or_e != -1) {
			// 如果当前字符是. ,值为1，否则（就是碰到E跳过来的）值为2
			dp_or_e = (*ptr++ == '.') ? 1 : 2;
			// 检查小数
			goto check_digits;
		}
	// 开头字符不是数字或 . 直接返回 否
	} else {
		return 0;
	}
	// 如果，还没到结尾，简单检查一下后面的字符 
	if (ptr != str + length) {
		// 尾数据，指针指向当前字符 
		const char *endptr = ptr;
		// 跳过连续的空白
		while (*endptr == ' ' || *endptr == '\t' || *endptr == '\n' || *endptr == '\r' || *endptr == '\v' || *endptr == '\f') {
			// 指针右移
			endptr++;
			// 有效长度-1
			length--;
		}
		// 如果指针位置不在字串末尾（说明空格后面还有东西）
		if (ptr != str + length) {
			// 如果不允许错误
			if (!allow_errors) {
				// 返回0 （转换失败）
				return 0;
			}
			// 如果允许错误，并且有接收变量
			if (trailing_data != NULL) {
				// 返回：结尾有无效数据
				*trailing_data = true;
			}
		}
	}
	// 如果是整数 
	if (type == IS_LONG) {
		// # define MAX_LENGTH_OF_LONG 20 ,64位最大长度20
		// 如果已经到了最大长度，减掉 \0占一个字符 。
		if (digits == MAX_LENGTH_OF_LONG - 1) {
			// # define LONG_MIN_DIGITS \"9223372036854775808\" ,64位
			// 比较大小，检查是否溢出
			int cmp = strcmp(&ptr[-digits], long_min_digits);
			// ！（负数溢出 或 （无溢出 并且带 负号））
			if (!(cmp < 0 || (cmp == 0 && *str == '-'))) {
				// 如果接收小数
				if (dval) {
					// 字串转成小数
					*dval = zend_strtod(str, NULL);
				}
				// 如果没有溢出
				if (oflow_info != NULL) {
					// 溢出值为：负数 -1，正数 1
					*oflow_info = *str == '-' ? -1 : 1;
				}
				// 返回：小数类型
				return IS_DOUBLE;
			}
		}
		// 如果有接收变量
		if (lval) {
			// 如果是负数
			if (neg) {
				// 转成负数
				tmp_lval = -tmp_lval;
			}
			// 赋值返回
			*lval = (zend_long) tmp_lval;
		}
		// 返回：整数类型
		return IS_LONG;
	// 如果不是整数（是小数）
	} else {
		// 如果有接收变量
		if (dval) {
			// 赋值返回
			*dval = local_dval;
		}
		// 返回：小数类型
		return IS_DOUBLE;
	}
}
/* }}} */

// 字串匹配，Sunday 算法。用于查找。
/*
 * String matching - Sunday algorithm
 * http://www.iti.fh-flensburg.de/lang/algorithmen/pattern/sundayen.htm
 */
// ing3, 记录 p2 里每个 ascii 字符出现的最后（或最初）位置 到p1里，p3:p2的长度，p4:倒序
// 用于查找一个字串在另一个字串中的位置，的辅助方法。有趣的算法！（测试过）
static zend_always_inline void zend_memnstr_ex_pre(unsigned int td[], const char *needle, size_t needle_len, int reverse) /* {{{ */ {
	int i;
	// 256个
	for (i = 0; i < 256; i++) {
		// 初始值都是一样的，都是最大值
		td[i] = needle_len + 1;
	}
	// 如果要反转
	if (reverse) {
		// 倒序遍历 needle
		for (i = needle_len - 1; i >= 0; i--) {
			// needle 里的每个字符，出现的最后序号，放在 needle 里
			// 多个相同字符会覆盖
			td[(unsigned char)needle[i]] = i + 1;
		}
	// 不需要反转
	} else {
		size_t i;
		// 正序遍历
		for (i = 0; i < needle_len; i++) {
			// needle 里的每个字符，出现的最后序号，放在 needle 里
			// 多个相同字符会覆盖
			td[(unsigned char)needle[i]] = (int)needle_len - i;
		}
	}
}
/* }}} */

// ing3, 寻找一个字串在另一个字串中的位置，正序
ZEND_API const char* ZEND_FASTCALL zend_memnstr_ex(const char *haystack, const char *needle, size_t needle_len, const char *end) /* {{{ */
{
	unsigned int td[256];
	size_t i;
	const char *p;
	// 如果针的长度是0 或 haystack 没有针长
	if (needle_len == 0 || (end - haystack) < needle_len) {
		// 返回空
		return NULL;
	}
	// 记录 p2 里每个 ascii 字符出现的最后（或最初）位置 到p1里，p3:p2的长度，p4:倒序
	zend_memnstr_ex_pre(td, needle, needle_len, 0);

	// 开头位置
	p = haystack;
	// 最后查找位置
	end -= needle_len;

	// 如果没超过结尾
	while (p <= end) {
		// 遍历needle的长度
		for (i = 0; i < needle_len; i++) {
			// 只要有一个字符不匹配
			if (needle[i] != p[i]) {
				// 跳出此循环
				break;
			}
		}
		// 如果每个字符都匹配
		if (i == needle_len) {
			// 返回找到的位置
			return p;
		}
		// 如果找到结束位置
		if (UNEXPECTED(p == end)) {
			// 查找失败，返回空
			return NULL;
		}
		// 向后找n个字符 ，来计算跳跃距离
		p += td[(unsigned char)(p[needle_len])];
		// 这个地方是精华所在
		// 例如查找  abcde,如果碰到d，往后跳2个，先让d匹配上，再从头比较，这样绝对不会错。
		// 如果查找  abcdcecf，如果碰到c，还是往后跳2个（测试过）
	}

	// 查找失败，返回空
	return NULL;
}
/* }}} */

// ing3, 寻找一个字串在另一个字串中的位置，倒序
ZEND_API const char* ZEND_FASTCALL zend_memnrstr_ex(const char *haystack, const char *needle, size_t needle_len, const char *end) /* {{{ */
{
	unsigned int td[256];
	size_t i;
	const char *p;
	// 如果针的长度是0 或 haystack 没有针长
	if (needle_len == 0 || (end - haystack) < needle_len) {
		// 返回空
		return NULL;
	}
	// 记录 p2 里每个 ascii 字符出现的最后（或最初）位置 到p1里，p3:p2的长度，p4:倒序
	zend_memnstr_ex_pre(td, needle, needle_len, 1);

	// 结尾位置
	p = end;
	// 初始查找位置
	p -= needle_len;

	// 如果没超过开头
	while (p >= haystack) {
		// 遍历needle的长度
		for (i = 0; i < needle_len; i++) {
			// 只要有一个字符不匹配
			if (needle[i] != p[i]) {
				// 跳出此循环
				break;
			}
		}
		// 如果每个字符都匹配
		if (i == needle_len) {
			// 返回找到的位置
			return (const char *)p;
		}

		// 如果找到开头位置
		if (UNEXPECTED(p == haystack)) {
			// 查找失败，返回空
			return NULL;
		}

		// 向前找1个字符，来计算跳跃距离
		p -= td[(unsigned char)(p[-1])];
		// 这个地方是精华所在
		// 例如查找  abcde,如果碰到c，往回跳3个，先让c匹配上，再从头比较，这样绝对不会错。
		// 如果查找  abcdcecf，如果碰到c，还是往回跳3个（测试过）
	}
	
	// 查找失败，返回空
	return NULL;
}
/* }}} */

// ing3, double 转成 long 型。兼容32位，64位系统。
// 如果long型是4字节（32位系统）
#if SIZEOF_ZEND_LONG == 4
ZEND_API zend_long ZEND_FASTCALL zend_dval_to_lval_slow(double d) /* {{{ */
{
	// 4字节的最大值，符号呢？下面处理
	double	two_pow_32 = pow(2., 32.),
			dmod;
	// 取得此区间内余数
	dmod = fmod(d, two_pow_32);
	// 如果是负数
	if (dmod < 0) {
		// 需要把它变成正数。使用ceil向上取整，取得更接近0的数（测试过）
		/* we're going to make this number positive; call ceil()
		 * to simulate rounding towards 0 of the negative number */
		// 转成正数 
		dmod = ceil(dmod) + two_pow_32;
	}
	// 强转成zend_long并返回
	return (zend_long)(zend_ulong)dmod;
}
// 如果long型不是4字节（64位系统）
#else
ZEND_API zend_long ZEND_FASTCALL zend_dval_to_lval_slow(double d)
{
	double	two_pow_64 = pow(2., 64.),
			dmod;

	dmod = fmod(d, two_pow_64);
	// 不需要用ceil取整，原 double 数 d 不能包含小数部分。所以dmod 也不会包含小数。
	if (dmod < 0) {
		/* no need to call ceil; original double must have had no
		 * fractional part, hence dmod does not have one either */
		dmod += two_pow_64;
	}
	// 强转
	return (zend_long)(zend_ulong)dmod;
}
/* }}} */
#endif
