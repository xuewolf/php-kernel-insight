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
   +----------------------------------------------------------------------+
*/

// 避免重复定义
#ifndef ZEND_CONFIG_W32_H
#define ZEND_CONFIG_W32_H

// ？
#include <../main/config.w32.h>

#define _CRTDBG_MAP_ALLOC

// 内存分配
#include <malloc.h>
// 标准类库
#include <stdlib.h>
// 调试工具
#include <crtdbg.h>

// string 工具
#include <string.h>

// main.c中定义，无条件
#ifndef ZEND_INCLUDE_FULL_WINDOWS_HEADERS
#define WIN32_LEAN_AND_MEAN
#endif
// socket 工具 ？ 
#include <winsock2.h>
// windows 工具
#include <windows.h>

// float 小数
#include <float.h>

#define HAVE_STDIOSTR_H 1
#define HAVE_CLASS_ISTDIOSTREAM
#define istdiostream stdiostream

// 低版本操作系统用 _snprintf
#if _MSC_VER < 1900
#define snprintf _snprintf
#endif

// ing4, 比较字串
#define strcasecmp(s1, s2) _stricmp(s1, s2)
// ing4, 忽略大小写比较字串
#define strncasecmp(s1, s2, n) _strnicmp(s1, s2, n)

// 环境变量
#ifdef LIBZEND_EXPORTS
#	define ZEND_API __declspec(dllexport)
#else
#	define ZEND_API __declspec(dllimport)
#endif

// ？
#define ZEND_DLEXPORT		__declspec(dllexport)
#define ZEND_DLIMPORT		__declspec(dllimport)


#endif /* ZEND_CONFIG_W32_H */
