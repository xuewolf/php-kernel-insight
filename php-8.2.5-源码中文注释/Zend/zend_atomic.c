/*
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | https://www.php.net/license/3_01.txt                                 |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Levi Morrison <morrison.levi@gmail.com>                     |
   +----------------------------------------------------------------------+
 */

#include "zend_atomic.h"

// 这个文件包含不在行内的 原子方法副本。这对使用 Rust之类的语言写的扩展来说很有用。
// c和c++编译器可能链接这些方法。
/* This file contains the non-inline copy of atomic functions. This is useful
 * for extensions written in languages such as Rust. C and C++ compilers are
 * probably going to inline these functions, but in the case they don't, this
 * is also where the code will go.
 */

// 为FFI用户定义 其他用户使用 ZEND_ATOMIC_BOOL_INIT 方法。
// 当碰到初始化时，它不是原子的。
/* Defined for FFI users; everyone else use ZEND_ATOMIC_BOOL_INIT.
 * This is NOT ATOMIC as it is meant for initialization.
 */
// ing3, 调用头文件里的方法
ZEND_API void zend_atomic_bool_init(zend_atomic_bool *obj, bool desired) {
	ZEND_ATOMIC_BOOL_INIT(obj, desired);
}

// ing3, 调用头文件里的方法
ZEND_API bool zend_atomic_bool_exchange(zend_atomic_bool *obj, bool desired) {
	return zend_atomic_bool_exchange_ex(obj, desired);
}

// ing3, 调用头文件里的方法
ZEND_API void zend_atomic_bool_store(zend_atomic_bool *obj, bool desired) {
	zend_atomic_bool_store_ex(obj, desired);
}

// windows走这里
#if defined(ZEND_WIN32) || defined(HAVE_SYNC_ATOMICS)
// 少一个const修饰符
/* On these platforms it is non-const due to underlying APIs. */
// ing3, 调用头文件里的方法
ZEND_API bool zend_atomic_bool_load(zend_atomic_bool *obj) {
	return zend_atomic_bool_load_ex(obj);
}
// 
#else
// ing3, 调用头文件里的方法
ZEND_API bool zend_atomic_bool_load(const zend_atomic_bool *obj) {
	return zend_atomic_bool_load_ex(obj);
}
#endif
