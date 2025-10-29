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
   | Authors: Levi Morrison <levim@php.net>                               |
   |          Sammy Kaye Powers <sammyk@php.net>                          |
   +----------------------------------------------------------------------+
*/

#ifndef ZEND_OBSERVER_H
#define ZEND_OBSERVER_H

#include "zend.h"
#include "zend_compile.h"
#include "zend_fibers.h"

// #define BEGIN_EXTERN_C() extern \"C\" {
BEGIN_EXTERN_C()

extern ZEND_API int zend_observer_fcall_op_array_extension;
extern ZEND_API bool zend_observer_errors_observed;
extern ZEND_API bool zend_observer_function_declared_observed;
extern ZEND_API bool zend_observer_class_linked_observed;

// ing3, 启用后值不是-1
#define ZEND_OBSERVER_ENABLED (zend_observer_fcall_op_array_extension != -1)

// ing3, 开始观察者调用
#define ZEND_OBSERVER_FCALL_BEGIN(execute_data) do { \
		/* 如果观察者已启用 */ \
		if (ZEND_OBSERVER_ENABLED) { \
			/* 开始 观察者调用 */\
			zend_observer_fcall_begin(execute_data); \
		} \
	} while (0)

// ing3, 结束观察者调用
#define ZEND_OBSERVER_FCALL_END(execute_data, return_value) do { \
		/* 如果观察者已启用 */ \
		if (ZEND_OBSERVER_ENABLED) { \
			/* 结束 观察者调用 */\
			zend_observer_fcall_end(execute_data, return_value); \
		} \
	} while (0)

// 开始观察者调用 ，函数结构
typedef void (*zend_observer_fcall_begin_handler)(zend_execute_data *execute_data);
// 结束观察者调用 ，函数结构
typedef void (*zend_observer_fcall_end_handler)(zend_execute_data *execute_data, zval *retval);

// 观察者(调用处理器)。包含一个开始函数指针 和 一个结束函数指针 ，一对方法指针，而不是两个方法。
// 针对每个函数 ,开始处理器 和 结束 处理器，在一个列表里的两个区域中
typedef struct _zend_observer_fcall_handlers {
	zend_observer_fcall_begin_handler begin;
	zend_observer_fcall_end_handler end;
} zend_observer_fcall_handlers;

// 如果fn不可被观察，要返回{null，null}
/* If the fn should not be observed then return {NULL, NULL} */

// 这个结构体是个函数类型， 返回值为 名称是 zend_observer_fcall_init 返回值是 zend_observer_fcall_handlers
typedef zend_observer_fcall_handlers (*zend_observer_fcall_init)(zend_execute_data *execute_data);

// 只在 minit/ 开启 中调用
// Call during minit/startup ONLY
ZEND_API void zend_observer_fcall_register(zend_observer_fcall_init);

// 运行时调用，但只有在 zend_observer_fcall_register 以后
// Call during runtime, but only if you have used zend_observer_fcall_register.
// 同时只能有一个begin和一个end处理器。如果已经有旧的，要先删除旧的。
// You must not have more than one begin and one end handler active at the same time. Remove the old one first, if there is an existing one.
ZEND_API void zend_observer_add_begin_handler(zend_function *function, zend_observer_fcall_begin_handler begin);
ZEND_API bool zend_observer_remove_begin_handler(zend_function *function, zend_observer_fcall_begin_handler begin);
ZEND_API void zend_observer_add_end_handler(zend_function *function, zend_observer_fcall_end_handler end);
ZEND_API bool zend_observer_remove_end_handler(zend_function *function, zend_observer_fcall_end_handler end);

ZEND_API void zend_observer_startup(void); // Called by engine before MINITs
ZEND_API void zend_observer_post_startup(void); // Called by engine after MINITs
ZEND_API void zend_observer_activate(void);
ZEND_API void zend_observer_shutdown(void);

// 观察者 开始调用
ZEND_API void ZEND_FASTCALL zend_observer_fcall_begin(
	zend_execute_data *execute_data);

// 观察者 生成器重新开始
ZEND_API void ZEND_FASTCALL zend_observer_generator_resume(
	zend_execute_data *execute_data);

// 观察者 结束调用
ZEND_API void ZEND_FASTCALL zend_observer_fcall_end(
	zend_execute_data *execute_data,
	zval *return_value);

// 观察者 结束所有调用
ZEND_API void zend_observer_fcall_end_all(void);

// 
typedef void (*zend_observer_function_declared_cb)(zend_op_array *op_array, zend_string *name);

//
ZEND_API void zend_observer_function_declared_register(zend_observer_function_declared_cb cb);
//
ZEND_API void ZEND_FASTCALL _zend_observer_function_declared_notify(zend_op_array *op_array, zend_string *name);

// ing4, 依次调用每个 声名回调 函数
static inline void zend_observer_function_declared_notify(zend_op_array *op_array, zend_string *name) {
	// 如果有 zend_observer_function_declared_observed
    if (UNEXPECTED(zend_observer_function_declared_observed)) {
		// 依次调用每个 声名回调 函数
		_zend_observer_function_declared_notify(op_array, name);
	}
}

typedef void (*zend_observer_class_linked_cb)(zend_class_entry *ce, zend_string *name);

ZEND_API void zend_observer_class_linked_register(zend_observer_class_linked_cb cb);
ZEND_API void ZEND_FASTCALL _zend_observer_class_linked_notify(zend_class_entry *ce, zend_string *name);

// ing4, 依次调用每个 链接回调 函数
static inline void zend_observer_class_linked_notify(zend_class_entry *ce, zend_string *name) {
	// 如果有 zend_observer_class_linked_observed
	if (UNEXPECTED(zend_observer_class_linked_observed)) {
		// 依次调用每个 链接回调 函数
		_zend_observer_class_linked_notify(ce, name);
	}
}

typedef void (*zend_observer_error_cb)(int type, zend_string *error_filename, uint32_t error_lineno, zend_string *message);

ZEND_API void zend_observer_error_register(zend_observer_error_cb callback);
ZEND_API void _zend_observer_error_notify(int type, zend_string *error_filename, uint32_t error_lineno, zend_string *message);

// ing4, 依次调用每个 出错回调 函数
static inline void zend_observer_error_notify(int type, zend_string *error_filename, uint32_t error_lineno, zend_string *message) {
	// 如果有 zend_observer_errors_observed
	if (UNEXPECTED(zend_observer_errors_observed)) {
		// 依次调用每个 出错回调 函数
		_zend_observer_error_notify(type, error_filename, error_lineno, message);
	}
}

typedef void (*zend_observer_fiber_init_handler)(zend_fiber_context *initializing);
typedef void (*zend_observer_fiber_switch_handler)(zend_fiber_context *from, zend_fiber_context *to);
typedef void (*zend_observer_fiber_destroy_handler)(zend_fiber_context *destroying);

ZEND_API void zend_observer_fiber_init_register(zend_observer_fiber_init_handler handler);
ZEND_API void zend_observer_fiber_switch_register(zend_observer_fiber_switch_handler handler);
ZEND_API void zend_observer_fiber_destroy_register(zend_observer_fiber_destroy_handler handler);

ZEND_API void ZEND_FASTCALL zend_observer_fiber_init_notify(zend_fiber_context *initializing);
ZEND_API void ZEND_FASTCALL zend_observer_fiber_switch_notify(zend_fiber_context *from, zend_fiber_context *to);
ZEND_API void ZEND_FASTCALL zend_observer_fiber_destroy_notify(zend_fiber_context *destroying);

// #define END_EXTERN_C() }
END_EXTERN_C()

#endif /* ZEND_OBSERVER_H */
