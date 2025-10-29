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
   | Authors: Sascha Schumann <sascha@schumann.cx>                        |
   |          Ard Biesheuvel <ard.biesheuvel@linaro.org>                  |
   +----------------------------------------------------------------------+
*/

#include "zend_portability.h"

#ifndef ZEND_MULTIPLY_H
#define ZEND_MULTIPLY_H

// ZEND_SIGNED_MULTIPLY_LONG:两个整数相乘  方法在不同平台上的描述。
// VS 测试时，走6.1分支
// 情况1
#if PHP_HAVE_BUILTIN_SMULL_OVERFLOW && SIZEOF_LONG == SIZEOF_ZEND_LONG

#define ZEND_SIGNED_MULTIPLY_LONG(a, b, lval, dval, usedval) do {	\
	long __tmpvar;		 											\
	if (((usedval) = __builtin_smull_overflow((a), (b), &__tmpvar))) {	\
		(dval) = (double) (a) * (double) (b);						\
	}																\
	else (lval) = __tmpvar;											\
} while (0)

// 情况2
#elif PHP_HAVE_BUILTIN_SMULLL_OVERFLOW && SIZEOF_LONG_LONG == SIZEOF_ZEND_LONG

#define ZEND_SIGNED_MULTIPLY_LONG(a, b, lval, dval, usedval) do {	\
	long long __tmpvar; 											\
	if (((usedval) = __builtin_smulll_overflow((a), (b), &__tmpvar))) {	\
		(dval) = (double) (a) * (double) (b);						\
	}																\
	else (lval) = __tmpvar;											\
} while (0)

// 情况3
#elif (defined(__i386__) || defined(__x86_64__)) && defined(__GNUC__)

// suspend, 有汇编
#define ZEND_SIGNED_MULTIPLY_LONG(a, b, lval, dval, usedval) do {	\
	zend_long __tmpvar; 													\
	__asm__ ("imul %3,%0\n"											\
		"adc $0,%1" 												\
			: "=r"(__tmpvar),"=r"(usedval) 							\
			: "0"(a), "r"(b), "1"(0));								\
	if (usedval) (dval) = (double) (a) * (double) (b);				\
	else (lval) = __tmpvar;											\
} while (0)

// 情况4
#elif defined(__arm__) && defined(__GNUC__)

// suspend, 有汇编
#define ZEND_SIGNED_MULTIPLY_LONG(a, b, lval, dval, usedval) do {	\
	zend_long __tmpvar; 													\
	__asm__("smull %0, %1, %2, %3\n"								\
		"sub %1, %1, %0, asr #31"									\
			: "=r"(__tmpvar), "=r"(usedval)							\
			: "r"(a), "r"(b));										\
	if (usedval) (dval) = (double) (a) * (double) (b);				\
	else (lval) = __tmpvar;											\
} while (0)

// 情况5
#elif defined(__aarch64__) && defined(__GNUC__)

// suspend, 有汇编
#define ZEND_SIGNED_MULTIPLY_LONG(a, b, lval, dval, usedval) do {	\
	zend_long __tmpvar; 													\
	__asm__("mul %0, %2, %3\n"										\
		"smulh %1, %2, %3\n"										\
		"sub %1, %1, %0, asr #63\n"									\
			: "=&r"(__tmpvar), "=&r"(usedval)						\
			: "r"(a), "r"(b));										\
	if (usedval) (dval) = (double) (a) * (double) (b);				\
	else (lval) = __tmpvar;											\
} while (0)

// 情况6, windows操作系统 
#elif defined(ZEND_WIN32)

// 情况6.1
# ifdef _M_X64
#  pragma intrinsic(_mul128)
// ing2, 计算整数乘法，如果溢出，转成双精度乘法。_mul128 需要单独学习。
// 传入参数 前3个是整数，第4个 double，第五个overflow
#  define ZEND_SIGNED_MULTIPLY_LONG(a, b, lval, dval, usedval) do {       \
	__int64 __high; \
	__int64 __low = _mul128((a), (b), &__high); \
	/* 右移63位，得到高位，如果和__high相同说明没有溢出。 */ \
	/* 两个无符号大整数相乘会溢出，比如 0xFFFFFFFF * 0xFFFFFFFF。 */ \
	if ((__low >> 63I64) == __high) { \
		(usedval) = 0; \
		(lval) = __low; \
	/* 否则，说明溢出了 */ \
	} else { \
		(usedval) = 1; \
		/* 转为双精度乘法 */
		(dval) = (double)(a) * (double)(b); \
	} \
} while (0)
// 情况6.2
# elif defined(_M_ARM64)
#  pragma intrinsic(__mulh)
#  define ZEND_SIGNED_MULTIPLY_LONG(a, b, lval, dval, usedval) do {       \
	__int64 __high = __mulh((a), (b)); \
	__int64 __low  = (a) * (b); \
	if ((__low >> 63I64) == __high) { \
		(usedval) = 0; \
		(lval) = __low; \
	} else { \
		(usedval) = 1; \
		(dval) = (double)(a) * (double)(b); \
	} \
} while (0)
// 情况6.3
# else
#  define ZEND_SIGNED_MULTIPLY_LONG(a, b, lval, dval, usedval) do {	\
	zend_long   __lres  = (a) * (b);										\
	long double __dres  = (long double)(a) * (long double)(b);		\
	long double __delta = (long double) __lres - __dres;			\
	if ( ((usedval) = (( __dres + __delta ) != __dres))) {			\
		(dval) = __dres;											\
	} else {														\
		(lval) = __lres;											\
	}																\
} while (0)
# endif

// 情况7
#elif defined(__powerpc64__) && defined(__GNUC__)

#define ZEND_SIGNED_MULTIPLY_LONG(a, b, lval, dval, usedval) do {	\
	long __low, __high;						\
	__asm__("mulld %0,%2,%3\n\t"					\
		"mulhd %1,%2,%3\n"					\
		: "=&r"(__low), "=&r"(__high)				\
		: "r"(a), "r"(b));					\
	if ((__low >> 63) != __high) {					\
		(dval) = (double) (a) * (double) (b);			\
		(usedval) = 1;						\
	} else {							\
		(lval) = __low;						\
		(usedval) = 0;						\
	}								\
} while (0)

// 情况8, 32位系统
#elif SIZEOF_ZEND_LONG == 4

#define ZEND_SIGNED_MULTIPLY_LONG(a, b, lval, dval, usedval) do {	\
	int64_t __result = (int64_t) (a) * (int64_t) (b);				\
	if (__result > ZEND_LONG_MAX || __result < ZEND_LONG_MIN) {		\
		(dval) = (double) __result;									\
		(usedval) = 1;												\
	} else {														\
		(lval) = (long) __result;									\
		(usedval) = 0;												\
	}																\
} while (0)

// 情况9
#else

#define ZEND_SIGNED_MULTIPLY_LONG(a, b, lval, dval, usedval) do {	\
	long   __lres  = (a) * (b);										\
	long double __dres  = (long double)(a) * (long double)(b);		\
	long double __delta = (long double) __lres - __dres;			\
	if ( ((usedval) = (( __dres + __delta ) != __dres))) {			\
		(dval) = __dres;											\
	} else {														\
		(lval) = __lres;											\
	}																\
} while (0)

#endif
// ZEND_SIGNED_MULTIPLY_LONG 所有情况完毕


// zend_safe_address 方法在不同平台上的描述。其实只是为了检查内存是否溢出而已。
// VS 测试时，走7 分支
// 情况1，如果使用gunc 并且 ...
#if defined(__GNUC__) && (defined(__native_client__) || defined(i386))

static zend_always_inline size_t zend_safe_address(size_t nmemb, size_t size, size_t offset, bool *overflow)
{
	size_t res = nmemb;
	size_t m_overflow = 0;

	if (ZEND_CONST_COND(offset == 0, 0)) {
		__asm__ ("mull %3\n\tadcl $0,%1"
	     : "=&a"(res), "=&d" (m_overflow)
	     : "%0"(res),
	       "rm"(size));
	} else {
		__asm__ ("mull %3\n\taddl %4,%0\n\tadcl $0,%1"
	     : "=&a"(res), "=&d" (m_overflow)
	     : "%0"(res),
	       "rm"(size),
	       "rm"(offset));
	}

	if (UNEXPECTED(m_overflow)) {
		*overflow = 1;
		return 0;
	}
	*overflow = 0;
	return res;
}
// 情况2，如果使用gunc 并且 是64位
#elif defined(__GNUC__) && defined(__x86_64__)

static zend_always_inline size_t zend_safe_address(size_t nmemb, size_t size, size_t offset, bool *overflow)
{
	size_t res = nmemb;
	zend_ulong m_overflow = 0;

#ifdef __ILP32__ /* x32 */
# define LP_SUFF "l"
#else /* amd64 */
# define LP_SUFF "q"
#endif

	if (ZEND_CONST_COND(offset == 0, 0)) {
		__asm__ ("mul" LP_SUFF  " %3\n\t"
			"adc $0,%1"
			: "=&a"(res), "=&d" (m_overflow)
			: "%0"(res),
			  "rm"(size));
	} else {
		__asm__ ("mul" LP_SUFF  " %3\n\t"
			"add %4,%0\n\t"
			"adc $0,%1"
			: "=&a"(res), "=&d" (m_overflow)
			: "%0"(res),
			  "rm"(size),
			  "rm"(offset));
	}
#undef LP_SUFF
	if (UNEXPECTED(m_overflow)) {
		*overflow = 1;
		return 0;
	}
	*overflow = 0;
	return res;
}

// 情况3，如果使用gunc 并且 ...
#elif defined(__GNUC__) && defined(__arm__)

static zend_always_inline size_t zend_safe_address(size_t nmemb, size_t size, size_t offset, bool *overflow)
{
	size_t res;
	zend_ulong m_overflow;

	__asm__ ("umlal %0,%1,%2,%3"
		: "=r"(res), "=r"(m_overflow)
		: "r"(nmemb),
		  "r"(size),
		  "0"(offset),
		  "1"(0));

	if (UNEXPECTED(m_overflow)) {
		*overflow = 1;
		return 0;
	}
	*overflow = 0;
	return res;
}
// 情况4，如果使用gunc 并且 ...
#elif defined(__GNUC__) && defined(__aarch64__)

static zend_always_inline size_t zend_safe_address(size_t nmemb, size_t size, size_t offset, bool *overflow)
{
	size_t res;
	zend_ulong m_overflow;

	__asm__ ("mul %0,%2,%3\n\tumulh %1,%2,%3\n\tadds %0,%0,%4\n\tadc %1,%1,xzr"
		: "=&r"(res), "=&r"(m_overflow)
		: "r"(nmemb),
		  "r"(size),
		  "r"(offset));

	if (UNEXPECTED(m_overflow)) {
		*overflow = 1;
		return 0;
	}
	*overflow = 0;
	return res;
}
// 情况5，如果使用gunc 并且 ...
#elif defined(__GNUC__) && defined(__powerpc64__)

static zend_always_inline size_t zend_safe_address(size_t nmemb, size_t size, size_t offset, bool *overflow)
{
        size_t res;
        unsigned long m_overflow;

        __asm__ ("mulld %0,%2,%3\n\t"
                 "mulhdu %1,%2,%3\n\t"
                 "addc %0,%0,%4\n\t"
                 "addze %1,%1\n"
             : "=&r"(res), "=&r"(m_overflow)
             : "r"(nmemb),
               "r"(size),
               "r"(offset));

        if (UNEXPECTED(m_overflow)) {
                *overflow = 1;
                return 0;
        }
        *overflow = 0;
        return res;
}

// windows系统会走这两个分支
// 情况6，如果不使用gunc 并且 是32位系统 
#elif SIZEOF_SIZE_T == 4

static zend_always_inline size_t zend_safe_address(size_t nmemb, size_t size, size_t offset, bool *overflow)
{
	uint64_t res = (uint64_t) nmemb * (uint64_t) size + (uint64_t) offset;

	if (UNEXPECTED(res > UINT64_C(0xFFFFFFFF))) {
		*overflow = 1;
		return 0;
	}
	*overflow = 0;
	return (size_t) res;
}

// 情况7，其他情况（windows64位系统会走这个分支，测试过）
#else

// ing3, 检查溢出，只是加加减减看会不会溢出。
// 大小为 nmemb*size+offset
static zend_always_inline size_t zend_safe_address(size_t nmemb, size_t size, size_t offset, bool *overflow)
{
	size_t res = nmemb * size + offset;
	double _d  = (double)nmemb * (double)size + (double)offset;
	// nmemb * size + offset - ((double)nmemb * (double)size + (double)offset)
	// nmemb * size + offset - nmemb * size - offset
	// 0
	double _delta = (double)res - _d;

	if (UNEXPECTED((_d + _delta ) != _d)) {
		*overflow = 1;
		return 0;
	}
	*overflow = 0;
	return res;
}
// 
#endif
// zend_safe_address 所有情况结束 

// ing4, 检查地址溢出
static zend_always_inline size_t zend_safe_address_guarded(size_t nmemb, size_t size, size_t offset)
{
	bool overflow;
	// 检查地址溢出
	size_t ret = zend_safe_address(nmemb, size, offset, &overflow);
	// 如果有溢出，抛错
	if (UNEXPECTED(overflow)) {
		zend_error_noreturn(E_ERROR, "Possible integer overflow in memory allocation (%zu * %zu + %zu)", nmemb, size, offset);
		return 0;
	}
	return ret;
}

// ing4, 检查地址溢出，加了个报错信息
/* A bit more generic version of the same */
static zend_always_inline size_t zend_safe_addmult(size_t nmemb, size_t size, size_t offset, const char *message)
{
	bool overflow;
	// 检查地址溢出
	size_t ret = zend_safe_address(nmemb, size, offset, &overflow);
	// 如果有溢出，抛错
	if (UNEXPECTED(overflow)) {
		zend_error_noreturn(E_ERROR, "Possible integer overflow in %s (%zu * %zu + %zu)", message, nmemb, size, offset);
		return 0;
	}
	return ret;
}

// 对应到开头
#endif /* ZEND_MULTIPLY_H */
