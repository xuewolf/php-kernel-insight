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
   | Authors: Sterling Hughes <sterling@php.net>                          |
   |          Marcus Boerger <helly@php.net>                              |
   +----------------------------------------------------------------------+
*/

#include "zend.h"
#include "zend_API.h"
#include "zend_attributes.h"
#include "zend_builtin_functions.h"
#include "zend_interfaces.h"
#include "zend_exceptions.h"
#include "zend_closures.h"
#include "zend_generators.h"
#include "zend_weakrefs.h"
#include "zend_enum.h"
#include "zend_fibers.h"

// ing2, 注册默认的底层类
ZEND_API void zend_register_default_classes(void)
{
	// 接口
	zend_register_interfaces();
	// 默认异常类
	zend_register_default_exception();
	// 迭代器类
	zend_register_iterator_wrapper();
	// 闭包类
	zend_register_closure_ce();
	// 生成器类
	zend_register_generator_ce();
	// 弱引用类
	zend_register_weakref_ce();
	// 修饰属性类
	zend_register_attribute_ce();
	// menu类型
	zend_register_enum_ce();
	// fiber ?
	zend_register_fiber_ce();
}
