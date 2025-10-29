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

#ifndef ZEND_SMART_STR_PUBLIC_H
#define ZEND_SMART_STR_PUBLIC_H

// smart_str 结构体，独立出来 方便外部引用
typedef struct {
	/** See smart_str_extract() */
	zend_string *s;
	// 多加了一个size，这个a是它的精确使用长度，不同于 zend_string中的长度是它的占用空间长度
	size_t a;
} smart_str;

#endif
