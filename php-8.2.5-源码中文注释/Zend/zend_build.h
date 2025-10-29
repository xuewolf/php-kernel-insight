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
   | Authors: Stanislav Malyshev <stas@zend.com>                          |
   +----------------------------------------------------------------------+
*/

#ifndef ZEND_BUILD_H
#define ZEND_BUILD_H

#define ZEND_TOSTR_(x) #x
#define ZEND_TOSTR(x) ZEND_TOSTR_(x)
// 如果要求线程安全
#ifdef ZTS
// 线程安全标记
#define ZEND_BUILD_TS ",TS"
#else
// 非线程安全标记
#define ZEND_BUILD_TS ",NTS"
#endif

// 调试用
#if ZEND_DEBUG
// 调试标记
#define ZEND_BUILD_DEBUG ",debug"
// 正式环境
#else
// 正式环境为空
#define ZEND_BUILD_DEBUG
#endif

// 如果是windows 并且有定义 PHP_COMPILER_ID
#if defined(ZEND_WIN32) && defined(PHP_COMPILER_ID)
// 拼接 编译器ID
#define ZEND_BUILD_SYSTEM "," PHP_COMPILER_ID
// 不是windows
#else
// 编译器ID为空
#define ZEND_BUILD_SYSTEM
#endif


// 给私有应用使用
/* for private applications */
#define ZEND_BUILD_EXTRA

#endif
