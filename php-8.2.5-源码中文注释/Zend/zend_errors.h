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

#ifndef ZEND_ERRORS_H
#define ZEND_ERRORS_H

// 普通错误
#define E_ERROR				(1<<0L)
// 普通警告
#define E_WARNING			(1<<1L)
// 语法解析错误
#define E_PARSE				(1<<2L)
// 普通提示
#define E_NOTICE			(1<<3L)
// 内核错误
#define E_CORE_ERROR		(1<<4L)
// 内核警告
#define E_CORE_WARNING		(1<<5L)
// 编译错误
#define E_COMPILE_ERROR		(1<<6L)
// 编译警告
#define E_COMPILE_WARNING	(1<<7L)
// 用户错误
#define E_USER_ERROR		(1<<8L)
// 用户警告
#define E_USER_WARNING		(1<<9L)
// 用户提示
#define E_USER_NOTICE		(1<<10L)
// 严格语法提示
#define E_STRICT			(1<<11L)
// 恢复错误 ？？？
#define E_RECOVERABLE_ERROR	(1<<12L)
// 弃用的语法或函数
#define E_DEPRECATED		(1<<13L)
// 用户弃用的函数 
#define E_USER_DEPRECATED	(1<<14L)

// 表明这个这个常见的致命异常不会在 bailout（魔术方法__get,__set） 里发生。
/* Indicates that this usually fatal error should not result in a bailout */
#define E_DONT_BAIL			(1<<15L)

// clear, 全部错误
#define E_ALL (E_ERROR | E_WARNING | E_PARSE | E_NOTICE | E_CORE_ERROR | E_CORE_WARNING | E_COMPILE_ERROR | E_COMPILE_WARNING | E_USER_ERROR | E_USER_WARNING | E_USER_NOTICE | E_RECOVERABLE_ERROR | E_DEPRECATED | E_USER_DEPRECATED | E_STRICT)
#define E_CORE (E_CORE_ERROR | E_CORE_WARNING)

// clear, 静默符号（@）有效的致命错误
/* Fatal errors that are ignored by the silence operator */
#define E_FATAL_ERRORS (E_ERROR | E_CORE_ERROR | E_COMPILE_ERROR | E_USER_ERROR | E_RECOVERABLE_ERROR | E_PARSE)

// clear, 是否仅包含 致命错误
#define E_HAS_ONLY_FATAL_ERRORS(mask) !((mask) & ~E_FATAL_ERRORS)

#endif /* ZEND_ERRORS_H */
