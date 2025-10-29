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

#ifndef ZEND_RANGE_CHECK_H
#define ZEND_RANGE_CHECK_H

#include "zend_long.h"
// 基本范围识别的标记宏。 sizeof(signed) == sizeof(unsigned) 总是成立，所以不要想得太复杂。
/* Flag macros for basic range recognition. Notable is that
   always sizeof(signed) == sizeof(unsigned), so no need to
   overcomplicate things. */
// 4 < 8, long 会溢出 有符号和无符号的 int
#if SIZEOF_INT < SIZEOF_ZEND_LONG
# define ZEND_LONG_CAN_OVFL_INT 1
# define ZEND_LONG_CAN_OVFL_UINT 1
#endif

// 4 < 8, size_t 会溢出 有符号和无符号的 int
#if SIZEOF_INT < SIZEOF_SIZE_T
// 在同一平台， size_t 总会溢出有符号 int。 通过当前设计，size_t 可以溢出 zend_long。
/* size_t can always overflow signed int on the same platform.
   Furthermore, by the current design, size_t can always
   overflow zend_long. */
# define ZEND_SIZE_T_CAN_OVFL_UINT 1
#endif


/* zend_long vs. (unsigned) int checks. */
// 如果long会溢出int （64位系统）
#ifdef ZEND_LONG_CAN_OVFL_INT
// ing4, 检验当前 zlong 是否大于 INT_MAX（向上溢出）
# define ZEND_LONG_INT_OVFL(zlong) UNEXPECTED((zlong) > (zend_long)INT_MAX)
// ing4, 检验当前 zlong 是否小于 INT_MIN（向下溢出）
# define ZEND_LONG_INT_UDFL(zlong) UNEXPECTED((zlong) < (zend_long)INT_MIN)
// ing4, 检验当前 zlong 是否向上或向下溢出 有符号int
# define ZEND_LONG_EXCEEDS_INT(zlong) UNEXPECTED(ZEND_LONG_INT_OVFL(zlong) || ZEND_LONG_INT_UDFL(zlong))
// ing4, 检验当前 zlong 是否向上或向下溢出 无符号int
# define ZEND_LONG_UINT_OVFL(zlong) UNEXPECTED((zlong) < 0 || (zlong) > (zend_long)UINT_MAX)
#else
# define ZEND_LONG_INT_OVFL(zl) (0)
# define ZEND_LONG_INT_UDFL(zl) (0)
# define ZEND_LONG_EXCEEDS_INT(zlong) (0)
# define ZEND_LONG_UINT_OVFL(zl) (0)
#endif

/* size_t vs (unsigned) int checks. */
#define ZEND_SIZE_T_INT_OVFL(size) 	UNEXPECTED((size) > (size_t)INT_MAX)

// 如果可能溢出无符号 int
#ifdef ZEND_SIZE_T_CAN_OVFL_UINT
// ing4, 检验当前 zlong 是否大于 INT_MAX（UINT_MAX）
# define ZEND_SIZE_T_UINT_OVFL(size) UNEXPECTED((size) > (size_t)UINT_MAX)
#else
# define ZEND_SIZE_T_UINT_OVFL(size) (0)
#endif

// 比较 zend_long 和 size_t。 size_t 是 unsigned long (测试过，系统定义的)。
/* Comparison zend_long vs size_t */
// ing4, 检验 zlong小于0或 size > 转成无符号的 zlong 
#define ZEND_SIZE_T_GT_ZEND_LONG(size, zlong) ((zlong) < 0 || (size) > (size_t)(zlong))
// ing4, 检验 zlong小于0或 size >= 转成无符号的 zlong 
#define ZEND_SIZE_T_GTE_ZEND_LONG(size, zlong) ((zlong) < 0 || (size) >= (size_t)(zlong))
// ing4, 检验 zlong>=0或 size < 转成无符号的 zlong 
#define ZEND_SIZE_T_LT_ZEND_LONG(size, zlong) ((zlong) >= 0 && (size) < (size_t)(zlong))
// ing4, 检验 zlong>=0或 size <= 转成无符号的 zlong 
#define ZEND_SIZE_T_LTE_ZEND_LONG(size, zlong) ((zlong) >= 0 && (size) <= (size_t)(zlong))

#endif /* ZEND_RANGE_CHECK_H */
