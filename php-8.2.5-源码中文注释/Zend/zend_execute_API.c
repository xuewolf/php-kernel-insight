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

// 平均每个方法40行，比较复杂

#include <stdio.h>
#include <signal.h>

#include "zend.h"
#include "zend_compile.h"
#include "zend_execute.h"
#include "zend_API.h"
#include "zend_stack.h"
#include "zend_constants.h"
#include "zend_extensions.h"
#include "zend_exceptions.h"
#include "zend_closures.h"
#include "zend_generators.h"
#include "zend_vm.h"
#include "zend_float.h"
#include "zend_fibers.h"
#include "zend_weakrefs.h"
#include "zend_inheritance.h"
#include "zend_observer.h"
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef ZEND_MAX_EXECUTION_TIMERS
#include <sys/syscall.h>
#endif

ZEND_API void (*zend_execute_ex)(zend_execute_data *execute_data);
ZEND_API void (*zend_execute_internal)(zend_execute_data *execute_data, zval *return_value);
ZEND_API zend_class_entry *(*zend_autoload)(zend_string *name, zend_string *lc_name);

/* true globals */
ZEND_API const zend_fcall_info empty_fcall_info = {0};
ZEND_API const zend_fcall_info_cache empty_fcall_info_cache = { NULL, NULL, NULL, NULL };

#ifdef ZEND_WIN32
ZEND_TLS HANDLE tq_timer = NULL;
#endif

// 失效代码
#if 0&&ZEND_DEBUG
static void (*original_sigsegv_handler)(int);
// 不会进到这里
static void zend_handle_sigsegv(void) /* {{{ */
{
	fflush(stdout);
	fflush(stderr);
	if (original_sigsegv_handler == zend_handle_sigsegv) {
		signal(SIGSEGV, original_sigsegv_handler);
	} else {
		signal(SIGSEGV, SIG_DFL);
	}
	{

		fprintf(stderr, "SIGSEGV caught on opcode %d on opline %d of %s() at %s:%d\n\n",
				active_opline->opcode,
				active_opline-EG(active_op_array)->opcodes,
				get_active_function_name(),
				zend_get_executed_filename(),
				zend_get_executed_lineno());
/* See http://support.microsoft.com/kb/190351 */

// windows走这里
#ifdef ZEND_WIN32
		fflush(stderr);
#endif
	}
	// 
	if (original_sigsegv_handler!=zend_handle_sigsegv) {
		original_sigsegv_handler(dummy);
	}
}
/* }}} */
#endif

// ing4, 调用扩展的激活方法
static void zend_extension_activator(zend_extension *extension) /* {{{ */
{
	if (extension->activate) {
		extension->activate();
	}
}
/* }}} */

// ing4, 调用扩展的反激活方法
static void zend_extension_deactivator(zend_extension *extension) /* {{{ */
{
	if (extension->deactivate) {
		extension->deactivate();
	}
}
/* }}} */

// ing3, 遍历哈希表删除非永久常量（辅助方法）
static int clean_non_persistent_constant_full(zval *zv) /* {{{ */
{
	zend_constant *c = Z_PTR_P(zv);
	return (ZEND_CONSTANT_FLAGS(c) & CONST_PERSISTENT) ? ZEND_HASH_APPLY_KEEP : ZEND_HASH_APPLY_REMOVE;
}
/* }}} */

// ing3, 遍历哈希表删除非内置函数（辅助方法）
static int clean_non_persistent_function_full(zval *zv) /* {{{ */
{
	zend_function *function = Z_PTR_P(zv);
	return (function->type == ZEND_INTERNAL_FUNCTION) ? ZEND_HASH_APPLY_KEEP : ZEND_HASH_APPLY_REMOVE;
}
/* }}} */

// ing3, 遍历哈希表删除非内置类（辅助方法）
static int clean_non_persistent_class_full(zval *zv) /* {{{ */
{
	// 取得 zval里的类入口
	zend_class_entry *ce = Z_PTR_P(zv);
	// 如果是内置类，保留，不是内置类，删除
	return (ce->type == ZEND_INTERNAL_CLASS) ? ZEND_HASH_APPLY_KEEP : ZEND_HASH_APPLY_REMOVE;
}
/* }}} */

// ing2, 初始化执行器
void init_executor(void) /* {{{ */
{
	// zend_float.c
	zend_init_fpu();

	// 全局未初始化zval 赋值为null
	ZVAL_NULL(&EG(uninitialized_zval));
	// 全局错误zval 赋值为错误
	ZVAL_ERROR(&EG(error_zval));

// 调试	
/* destroys stack frame, therefore makes core dumps worthless */
#if 0&&ZEND_DEBUG
	original_sigsegv_handler = signal(SIGSEGV, zend_handle_sigsegv);
#endif

	// 符号表缓存指针
	EG(symtable_cache_ptr) = EG(symtable_cache);
	// 符号表缓存限制 大小
	EG(symtable_cache_limit) = EG(symtable_cache) + SYMTABLE_CACHE_SIZE;
	// 不支持扩展信息
	EG(no_extensions) = 0;

	// 函数表
	EG(function_table) = CG(function_table);
	// 类表
	EG(class_table) = CG(class_table);

	// 禁用自动加载
	EG(in_autoload) = NULL;
	// 错误处理器
	EG(error_handling) = EH_NORMAL;
	// EG_FLAGS_INITIAL = 0
	EG(flags) = EG_FLAGS_INITIAL;

	// 初始化vm堆栈
	zend_vm_stack_init();

	// 初始化全局符号表
	zend_hash_init(&EG(symbol_table), 64, NULL, ZVAL_PTR_DTOR, 0);

	// 对所有扩展调用 zend_extension_activator
	zend_llist_apply(&zend_extensions, (llist_apply_func_t) zend_extension_activator);

	// 初始化包含文件哈希表
	zend_hash_init(&EG(included_files), 8, NULL, NULL, 0);

	// ？
	EG(ticks_count) = 0;

	// 用户错误处理器，初始化为未定义
	ZVAL_UNDEF(&EG(user_error_handler));
	// 用户异常处理器，初始化为未定义
	ZVAL_UNDEF(&EG(user_exception_handler));

	// 当前执行数据
	EG(current_execute_data) = NULL;

	// 初始化堆栈 ？
	zend_stack_init(&EG(user_error_handlers_error_reporting), sizeof(int));
	zend_stack_init(&EG(user_error_handlers), sizeof(zval));
	zend_stack_init(&EG(user_exception_handlers), sizeof(zval));

	// 初始化对象存储堆栈
	zend_objects_store_init(&EG(objects_store), 1024);

	// 默认不清理全表
	EG(full_tables_cleanup) = 0;
	// 两个原子 bool 变量
	ZEND_ATOMIC_BOOL_INIT(&EG(vm_interrupt), false);
	ZEND_ATOMIC_BOOL_INIT(&EG(timed_out), false);

	// 当前异常为 null
	EG(exception) = NULL;
	// 前一个异常为 null
	EG(prev_exception) = NULL;

	// 临时域 null
	EG(fake_scope) = NULL;
	// 弹跳函数名为 null
	EG(trampoline).common.function_name = NULL;

	// 哈希表迭代器数量 = 迭代器缓存大小 / 迭代器大小
	EG(ht_iterators_count) = sizeof(EG(ht_iterators_slots)) / sizeof(HashTableIterator);
	// 已使用迭代器数量为 0
	EG(ht_iterators_used) = 0;
	// ？
	EG(ht_iterators) = EG(ht_iterators_slots);
	//
	memset(EG(ht_iterators), 0, sizeof(EG(ht_iterators_slots)));

	// 持久常量数量 = 已有常量数量
	EG(persistent_constants_count) = EG(zend_constants)->nNumUsed;
	// 持久函数数量 = 已有函数数量
	EG(persistent_functions_count) = EG(function_table)->nNumUsed;
	// 持久类数量 = 已有类数量
	EG(persistent_classes_count)   = EG(class_table)->nNumUsed;

	// 初始化垃圾回收器，开始位置，结束位置，当前位置都是null
	EG(get_gc_buffer).start = EG(get_gc_buffer).end = EG(get_gc_buffer).cur = NULL;

	// 默认不记录错误
	EG(record_errors) = false;
	// 已有错误数 = 0
	EG(num_errors) = 0;
	// 已有错误 = null
	EG(errors) = NULL;

	//  ?
	EG(filename_override) = NULL;
	// ?
	EG(lineno_override) = -1;

	// 初始化超时计时器
	zend_max_execution_timer_init();
	// 初始化 fiber
	zend_fiber_init();
	// 初始化弱引用
	zend_weakrefs_init();

	// 执行器已激活
	EG(active) = 1;
}
/* }}} */

// ing4, 调用销毁器：遍历哈希表时用，删除 引用次数为1的对象
static int zval_call_destructor(zval *zv) /* {{{ */
{
	// 如果是间接引用
	if (Z_TYPE_P(zv) == IS_INDIRECT) {
		// 解引用
		zv = Z_INDIRECT_P(zv);
	}
	// 如果是对象 并且引用计数 = 1
	if (Z_TYPE_P(zv) == IS_OBJECT && Z_REFCOUNT_P(zv) == 1) {
		// 哈希表中删除
		return ZEND_HASH_APPLY_REMOVE;
	// 不是对象 或 引用!=1
	} else {
		// 哈希表中不保留
		return ZEND_HASH_APPLY_KEEP;
	}
}
/* }}} */

// ing4, 解间接引用 并销毁间接引用的 zval。p1:zval
static void zend_unclean_zval_ptr_dtor(zval *zv) /* {{{ */
{
	// 如果是间接引用
	if (Z_TYPE_P(zv) == IS_INDIRECT) {
		// 解除间接引用
		zv = Z_INDIRECT_P(zv);
	}
	// 销毁没有引用次数的对象，有引用次数的添加到gc回收周期里。并没有销毁 zval本身。
	i_zval_ptr_dtor(zv);
}
/* }}} */

// ing3, 抛错：p1:获取类型，p2:异常类，p3:格式，p4:参数
static ZEND_COLD void zend_throw_or_error(int fetch_type, zend_class_entry *exception_ce, const char *format, ...) /* {{{ */
{
	va_list va;
	char *message = NULL;

	// 开始参数列表
	va_start(va, format);
	// 组织错误信息
	zend_vspprintf(&message, 0, format, va);
	// 如果 在获取类 异常
	if (fetch_type & ZEND_FETCH_CLASS_EXCEPTION) {
		// 报错：带异常类名
		zend_throw_error(exception_ce, "%s", message);
	} else {
		// 报错
		zend_error(E_ERROR, "%s", message);
	}
	// 释放错误信息
	efree(message);
	// 结束参数调用
	va_end(va);
}
/* }}} */

// ing3, 关闭销毁器。销毁符号表和对象仓库
void shutdown_destructors(void) /* {{{ */
{
	// 如果有 unclean_shutdown 选项
	if (CG(unclean_shutdown)) {
		// 使用 zend_unclean_zval_ptr_dtor 销毁符号表
		EG(symbol_table).pDestructor = zend_unclean_zval_ptr_dtor;
	}
	zend_try {
		uint32_t symbols;
		do {
			// 取得符号表元 顺序素区
			symbols = zend_hash_num_elements(&EG(symbol_table));
			// 倒序遍历 ，调用 zval_call_destructor 销毁 zval
			zend_hash_reverse_apply(&EG(symbol_table), (apply_func_t) zval_call_destructor);
		// 如果一次遍历后有变化 ，就再来一次。直到没变化了为止。
		} while (symbols != zend_hash_num_elements(&EG(symbol_table)));
		// 销毁对象仓库里的所有对象
		zend_objects_store_call_destructors(&EG(objects_store));
	// 
	} zend_catch {
		// 如果没有清理干净，无论如何把所有对象标记成已销毁 
		/* if we couldn't destruct cleanly, mark all objects as destructed anyway */
		zend_objects_store_mark_destructed(&EG(objects_store));
	} zend_end_try();
}
/* }}} */

// ing3, 释放执行器里保存的值
/* Free values held by the executor. */
ZEND_API void zend_shutdown_executor_values(bool fast_shutdown)
{
	zend_string *key;
	zval *zv;
	// 添加标记
	EG(flags) |= EG_FLAGS_IN_RESOURCE_SHUTDOWN;
	zend_try {
		// 销毁一个列表里的所有资源元素 
		zend_close_rsrc_list(&EG(regular_list));
	} zend_end_try();

	// 在这个点以后，不会再调用php 回调函数
	/* No PHP callback functions should be called after this point. */
	EG(active) = 0;

	// 不快速关闭
	if (!fast_shutdown) {
		// 销毁全局符号表
		zend_hash_graceful_reverse_destroy(&EG(symbol_table));

		// 常量里可能包含对象，先销毁它们再销毁对象仓库
		/* Constants may contain objects, destroy them before the object store. */
		// 如果要清空所有表
		if (EG(full_tables_cleanup)) {
			// 使用 clean_non_persistent_constant_full 配合清空常量哈希表
			zend_hash_reverse_apply(EG(zend_constants), clean_non_persistent_constant_full);
		// 不要清空所有表
		} else {
			// 遍历常量表
			ZEND_HASH_MAP_REVERSE_FOREACH_STR_KEY_VAL(EG(zend_constants), key, zv) {
				// 取得一个常量
				zend_constant *c = Z_PTR_P(zv);
				// 如果是最后一个
				if (_idx == EG(persistent_constants_count)) {
					// 跳出
					break;
				}
				// 销毁没有引用次数的对象,并没有销毁 zval本身。nogc的意思是，不放入gc回收周期
				zval_ptr_dtor_nogc(&c->value);
				// 如果有常量名
				if (c->name) {
					// 释放常量名
					zend_string_release_ex(c->name, 0);
				}
				// 释放常量
				efree(c);
				// 释放key
				zend_string_release_ex(key, 0);
			} ZEND_HASH_MAP_FOREACH_END_DEL();
		}

		// 释放静态属性和静态变量 先于最后的 垃圾回收，因为它们可能占用gc root
		/* Release static properties and static variables prior to the final GC run,
		 * as they may hold GC roots. */
		// 遍历所有函数
		ZEND_HASH_MAP_REVERSE_FOREACH_VAL(EG(function_table), zv) {
			// 取出操作码
			zend_op_array *op_array = Z_PTR_P(zv);
			// 如果是内置函数
			if (op_array->type == ZEND_INTERNAL_FUNCTION) {
				// 跳出
				break;
			}
			// 如果有静态变量
			if (ZEND_MAP_PTR(op_array->static_variables_ptr)) {
				// 静态变量列表
				HashTable *ht = ZEND_MAP_PTR_GET(op_array->static_variables_ptr);
				// 如果列表有效
				if (ht) {
					// 销毁 列表
					zend_array_destroy(ht);
					// static_variables_ptr 指针清空
					ZEND_MAP_PTR_SET(op_array->static_variables_ptr, NULL);
				}
			}
		} ZEND_HASH_FOREACH_END();
		// 如果有类表
		ZEND_HASH_MAP_REVERSE_FOREACH_VAL(EG(class_table), zv) {
			// 取出类
			zend_class_entry *ce = Z_PTR_P(zv);
			// 默认静态成员数量
			if (ce->default_static_members_count) {
				// 找到并删除源指针列表中 指向属性信息的指针。p1:源列表，p2:属性信息指针
				zend_cleanup_internal_class_data(ce);
			}

			// 如果有替换数据
			if (ZEND_MAP_PTR(ce->mutable_data)) {
				// 如果数据有效
				if (ZEND_MAP_PTR_GET_IMM(ce->mutable_data)) {
					// 清空替换数据
					zend_cleanup_mutable_class_data(ce);
				}
			// 如果是用户类 并且没有不可更改标记
			} else if (ce->type == ZEND_USER_CLASS && !(ce->ce_flags & ZEND_ACC_IMMUTABLE)) {
				// 常量可能包含对象，要在销毁对象仓库前销毁它
				/* Constants may contain objects, destroy the values before the object store. */
				zend_class_constant *c;
				// 遍历常量表
				ZEND_HASH_MAP_FOREACH_PTR(&ce->constants_table, c) {
					// 如果常量属于当前 类
					if (c->ce == ce) {
						// 销毁没有引用次数的对象,并没有销毁 zval本身。nogc的意思是，不放入gc回收周期
						zval_ptr_dtor_nogc(&c->value);
						// 清空常量值
						ZVAL_UNDEF(&c->value);
					}
				} ZEND_HASH_FOREACH_END();

				// 属性中也可能包含对象
				/* properties may contain objects as well */
				// 遍历默认属性表
				if (ce->default_properties_table) {
					// 属性表开头
					zval *p = ce->default_properties_table;
					// 属性表结尾
					zval *end = p + ce->default_properties_count;
					// 遍历每个属性
					while (p != end) {
						// 销毁没有引用次数的对象，有引用次数的添加到gc回收周期里。并没有销毁 zval本身。
						i_zval_ptr_dtor(p);
						// 属性设置为 未定义
						ZVAL_UNDEF(p);
						// 下一个属性
						p++;
					}
				}
			}

			// 如果用户类有 备用枚举表
			if (ce->type == ZEND_USER_CLASS && ce->backed_enum_table) {
				// 不可以有不可更改标记
				ZEND_ASSERT(!(ce->ce_flags & ZEND_ACC_IMMUTABLE));
				// 释放备用 备用枚举表
				zend_hash_release(ce->backed_enum_table);
				// 指针改成 null
				ce->backed_enum_table = NULL;
			}

			// 如果有使用静态变量的方法
			if (ce->ce_flags & ZEND_HAS_STATIC_IN_METHODS) {
				zend_op_array *op_array;
				// 遍历方法表
				ZEND_HASH_MAP_FOREACH_PTR(&ce->function_table, op_array) {
					// 如果是用户定义方法
					if (op_array->type == ZEND_USER_FUNCTION) {
						// 如果有静态变量表
						if (ZEND_MAP_PTR(op_array->static_variables_ptr)) {
							// 取得 静态变量表
							HashTable *ht = ZEND_MAP_PTR_GET(op_array->static_variables_ptr);
							// 如果 静态变量表 有效
							if (ht) {
								// 销毁 哈希表
								zend_array_destroy(ht);
								// 静态变量表 指针为null
								ZEND_MAP_PTR_SET(op_array->static_variables_ptr, NULL);
							}
						}
					}
				} ZEND_HASH_FOREACH_END();
			}
		} ZEND_HASH_FOREACH_END();

		// 也要释放错误和异常处理器，里面可以包含对象
		/* Also release error and exception handlers, which may hold objects. */
		// 如果有错误处理器
		if (Z_TYPE(EG(user_error_handler)) != IS_UNDEF) {
			// 销毁没有引用次数的对象，有引用次数的添加到gc回收周期里。并没有销毁 zval本身
			zval_ptr_dtor(&EG(user_error_handler));
			// 设置成未定义
			ZVAL_UNDEF(&EG(user_error_handler));
		}

		if (Z_TYPE(EG(user_exception_handler)) != IS_UNDEF) {
			// 销毁没有引用次数的对象，有引用次数的添加到gc回收周期里。并没有销毁 zval本身
			zval_ptr_dtor(&EG(user_exception_handler));
			// 设置成未定义
			ZVAL_UNDEF(&EG(user_exception_handler));
		}

		// 清空几个堆栈
		zend_stack_clean(&EG(user_error_handlers_error_reporting), NULL, 1);
		// 使用 zval_ptr_dtor 清空每个元素（这里用了原型转换？）
		zend_stack_clean(&EG(user_error_handlers), (void (*)(void *))ZVAL_PTR_DTOR, 1);
		zend_stack_clean(&EG(user_exception_handlers), (void (*)(void *))ZVAL_PTR_DTOR, 1);

// 调试
#if ZEND_DEBUG
		if (!CG(unclean_shutdown)) {
			gc_collect_cycles();
		}
#endif
	// 快速关闭 
	} else {
		// 只清空常量哈希表
		zend_hash_discard(EG(zend_constants), EG(persistent_constants_count));
	}

	// 最后释放对象仓库
	zend_objects_store_free_object_storage(&EG(objects_store), fast_shutdown);
}

// ing3, 关闭执行器
void shutdown_executor(void) /* {{{ */
{
	zend_string *key;
	zval *zv;
// 调试
#if ZEND_DEBUG
	bool fast_shutdown = 0;
// 正式
#else
	// 可以快速关闭的条件：如果使用了 zend内存 并且 不需要清空全表
	bool fast_shutdown = is_zend_mm() && !EG(full_tables_cleanup);
#endif

	zend_try {
		// 关闭文件流
		zend_stream_shutdown();
	} zend_end_try();

	// 释放执行器里保存的值
	zend_shutdown_executor_values(fast_shutdown);

	// 关闭弱引用
	zend_weakrefs_shutdown();
	// 关闭超时计时器
	zend_max_execution_timer_shutdown();
	// 关闭 fiber
	zend_fiber_shutdown();

	zend_try {
		// 对所有扩展调用 zend_extension_deactivator
		zend_llist_apply(&zend_extensions, (llist_apply_func_t) zend_extension_deactivator);
	} zend_end_try();

	// 如果要快速关闭
	if (fast_shutdown) {
		// 快速请求关闭
		// zend内存管理器会自己释放每个分配的内存块。不用管它
		/* Fast Request Shutdown
		 * =====================
		 * Zend Memory Manager frees memory by its own. We don't have to free
		 * each allocated block separately.
		 */
		// 
		// 销毁函数表
		zend_hash_discard(EG(function_table), EG(persistent_functions_count));
		// 销毁类表
		zend_hash_discard(EG(class_table), EG(persistent_classes_count));
	// 不要快速关闭
	} else {
		// 先销毁虚拟机缓存
		zend_vm_stack_destroy();

		// 如果要清空全表
		if (EG(full_tables_cleanup)) {
			// 调用 clean_non_persistent_function_full 清空 函数表
			zend_hash_reverse_apply(EG(function_table), clean_non_persistent_function_full);
			// 调用 clean_non_persistent_class_full 清空 类表
			zend_hash_reverse_apply(EG(class_table), clean_non_persistent_class_full);
		// 不用清空全部表
		} else {
			// 遍历函数表
			ZEND_HASH_MAP_REVERSE_FOREACH_STR_KEY_VAL(EG(function_table), key, zv) {
				// 取得函数
				zend_function *func = Z_PTR_P(zv);
				// 如果是最后一个
				if (_idx == EG(persistent_functions_count)) {
					// 跳出
					break;
				}
				// 销毁操作码组
				destroy_op_array(&func->op_array);
				// 释放key
				zend_string_release_ex(key, 0);
			} ZEND_HASH_MAP_FOREACH_END_DEL();

			// 遍历函数表
			ZEND_HASH_MAP_REVERSE_FOREACH_STR_KEY_VAL(EG(class_table), key, zv) {
				// 如果是最后一个
				if (_idx == EG(persistent_classes_count)) {
					// 跳出
					break;
				}
				// 销毁类 zv
				destroy_zend_class(zv);
				// 释放key
				zend_string_release_ex(key, 0);
			} ZEND_HASH_MAP_FOREACH_END_DEL();
		}

		// 如果 符号表缓存 指针没有批向开头，一直遍历 
		while (EG(symtable_cache_ptr) > EG(symtable_cache)) {
			// 回退一个指针
			EG(symtable_cache_ptr)--;
			// 销毁 这个符号表缓存
			zend_hash_destroy(*EG(symtable_cache_ptr));
			// 释放这个符号表
			FREE_HASHTABLE(*EG(symtable_cache_ptr));
		}

		// 销毁 包含文件列表
		zend_hash_destroy(&EG(included_files));
		// 销毁 几个堆栈 
		zend_stack_destroy(&EG(user_error_handlers_error_reporting));
		zend_stack_destroy(&EG(user_error_handlers));
		zend_stack_destroy(&EG(user_exception_handlers));
		// 销毁对象仓库
		zend_objects_store_destroy(&EG(objects_store));
		// 如果有自动加载表
		if (EG(in_autoload)) {
			// 销毁自动加载表
			zend_hash_destroy(EG(in_autoload));
			// 释放自动加载表
			FREE_HASHTABLE(EG(in_autoload));
		}

		// 如果有迭代器列表 ？
		if (EG(ht_iterators) != EG(ht_iterators_slots)) {
			// 释放迭代器列表
			efree(EG(ht_iterators));
		}
	}

#if ZEND_DEBUG
	if (EG(ht_iterators_used) && !CG(unclean_shutdown)) {
		zend_error(E_WARNING, "Leaked %" PRIu32 " hashtable iterators", EG(ht_iterators_used));
	}
#endif

	// 检查是否 有 触发弹跳
	/* Check whether anyone is hogging the trampoline. */
	ZEND_ASSERT(EG(trampoline).common.function_name == NULL || CG(unclean_shutdown));

	// 使用迭代器数量清 0
	EG(ht_iterators_used) = 0;

	// 关闭fpu
	zend_shutdown_fpu();
}
/* }}} */

/* return class name and "::" or "". */
// ing4, 获取当前活动的类名 ，space是引用返回
ZEND_API const char *get_active_class_name(const char **space) /* {{{ */
{
	zend_function *func;

	// 如果不在执行中
	if (!zend_is_executing()) {
		// 如果有space
		if (space) {
			// 清空space
			*space = "";
		}
		// 返回空
		return "";
	}
	// 当前运行的函数
	func = EG(current_execute_data)->func;
	// 函数类型
	switch (func->type) {
		// 如果是用户定义或内置函数 
		case ZEND_USER_FUNCTION:
		case ZEND_INTERNAL_FUNCTION:
		{
			// 所属类
			zend_class_entry *ce = func->common.scope;
			// 如果想要 space
			if (space) {
				// 如果有类实体 添加 ::
				*space = ce ? "::" : "";
			}
			// 返回类名
			return ce ? ZSTR_VAL(ce->name) : "";
		}
		// 其他情况，返回空
		default:
			if (space) {
				*space = "";
			}
			return "";
	}
}
/* }}} */

// ing4, 获取当前活动的函数名 ，space是引用返回
ZEND_API const char *get_active_function_name(void) /* {{{ */
{
	zend_function *func;
	// 不在执行中返回null
	if (!zend_is_executing()) {
		return NULL;
	}
	// 当前运行的函数
	func = EG(current_execute_data)->func;
	// 类型
	switch (func->type) {
		// 用户函数 
		case ZEND_USER_FUNCTION: {
				zend_string *function_name = func->common.function_name;
				// 有函数名用函数名，没有用main（什么情况会用到？）
				if (function_name) {
					return ZSTR_VAL(function_name);
				} else {
					return "main";
				}
			}
			break;
		// 内置函数，用函数名
		case ZEND_INTERNAL_FUNCTION:
			return ZSTR_VAL(func->common.function_name);
			break;
		// 其他，返回null
		default:
			return NULL;
	}
}
/* }}} */

// ing4, 从正在执行的函数或方法实例中获取名称
ZEND_API zend_string *get_active_function_or_method_name(void) /* {{{ */
{
	// 必须正在执行
	ZEND_ASSERT(zend_is_executing());
	// 从函数或方法实例中获取名称
	return get_function_or_method_name(EG(current_execute_data)->func);
}
/* }}} */

// ing4, 从函数或方法实例中获取名称，p1:函数或方法
ZEND_API zend_string *get_function_or_method_name(const zend_function *func) /* {{{ */
{
	// 如果函数有所属类，并且有名称
	if (func->common.scope && func->common.function_name) {
		// 创建并返回方法名
		return zend_create_member_string(func->common.scope->name, func->common.function_name);
	}
	// 如果有函数名，返回函数名，否则返回 main
	return func->common.function_name ? zend_string_copy(func->common.function_name) : zend_string_init("main", sizeof("main") - 1, 0);
}
/* }}} */

// ing4, 取得当前执行的函数的第n个参数的名称
ZEND_API const char *get_active_function_arg_name(uint32_t arg_num) /* {{{ */
{
	zend_function *func;
	// 如果没在执行，返回null
	if (!zend_is_executing()) {
		return NULL;
	}
	// 找到上下文小的函数 
	func = EG(current_execute_data)->func;
	// 获取函数的指定顺序号的参数的名称
	return get_function_arg_name(func, arg_num);
}
/* }}} */

// ing4, 获取函数的指定顺序号的参数的名称
ZEND_API const char *get_function_arg_name(const zend_function *func, uint32_t arg_num) /* {{{ */
{
	// 如果没有函数实例 或 顺序号是0 或 顺序号溢出
	if (!func || arg_num == 0 || func->common.num_args < arg_num) {
		// 返回null
		return NULL;
	}
	// 如果是用户定义的函数 或有用户定义的参数信息
	if (func->type == ZEND_USER_FUNCTION || (func->common.fn_flags & ZEND_ACC_USER_ARG_INFO)) {
		// 返回参数信息中的名称
		return ZSTR_VAL(func->common.arg_info[arg_num - 1].name);
	} else {
		// 返回内置参数信息中的名称
		return ((zend_internal_arg_info*) func->common.arg_info)[arg_num - 1].name;
	}
}
/* }}} */

// ing4, 获取正在执行的文件名
ZEND_API const char *zend_get_executed_filename(void) /* {{{ */
{
	// 获取正在执行的文件名
	zend_string *filename = zend_get_executed_filename_ex();
	// 如果没有取到文件名，返回 [no active file]
	return filename != NULL ? ZSTR_VAL(filename) : "[no active file]";
}
/* }}} */

// ing3, 获取正在执行的文件名
ZEND_API zend_string *zend_get_executed_filename_ex(void) /* {{{ */
{
	// 取得覆盖文件
	zend_string *filename_override = EG(filename_override);
	// 如果有覆盖文件
	if (filename_override != NULL) {
		// 直接返回覆盖文件
		return filename_override;
	}

	// 当前执行数据
	zend_execute_data *ex = EG(current_execute_data);

	// 如果没有函数 或 不是用户定义函数
	while (ex && (!ex->func || !ZEND_USER_CODE(ex->func->type))) {
		// 一直向前取
		ex = ex->prev_execute_data;
	}
	// 如果有执行数据
	if (ex) {
		// 返回文件名
		return ex->func->op_array.filename;
	// 否则返回 null
	} else {
		return NULL;
	}
}
/* }}} */

// ing3, 获取正在执行的行号
ZEND_API uint32_t zend_get_executed_lineno(void) /* {{{ */
{
	// 取得覆盖的行号
	zend_long lineno_override = EG(lineno_override);
	// 如果覆盖的行号有效
	if (lineno_override != -1) {
		// 返回 覆盖的行号
		return lineno_override;
	}

	// 取得当前执行数量
	zend_execute_data *ex = EG(current_execute_data);

	// 如果没有函数 或 不是用户定义函数 
	while (ex && (!ex->func || !ZEND_USER_CODE(ex->func->type))) {
		// 一直向前找
		ex = ex->prev_execute_data;
	}
	// 如果找到执行数据
	if (ex) {
		// 如果里面没有操作码
		if (!ex->opline) {
			// SAVE_OPLINE() 丢失，找到函数第一行
			/* Missing SAVE_OPLINE()? Falling back to first line of function */
			// 返回函数第一个操作码行号
			return ex->func->op_array.opcodes[0].lineno;
		}
		// 如果有异常 并且 操作码是处理异常 并且 行号是0 并且有 记录异常前的行号
		if (EG(exception) && ex->opline->opcode == ZEND_HANDLE_EXCEPTION &&
		    ex->opline->lineno == 0 && EG(opline_before_exception)) {
			// 返回奶衫的行号
			return EG(opline_before_exception)->lineno;
		}
		// 返回当前操作码行号
		return ex->opline->lineno;
	// 没有执行数据
	} else {
		// 返回 0
		return 0;
	}
}
/* }}} */

// ing4, 从执行数据串中，获取执行时作用域
ZEND_API zend_class_entry *zend_get_executed_scope(void) /* {{{ */
{
	// 操作码上下文
	zend_execute_data *ex = EG(current_execute_data);

	while (1) {
		// 遍历到空，返回NULL
		if (!ex) {
			return NULL;
		// 如果操作码有所属函数，并且（函数是用户定义的，或函数有 作用域）
		} else if (ex->func && (ZEND_USER_CODE(ex->func->type) || ex->func->common.scope)) {
			// 返回函数作用域（指针）
			return ex->func->common.scope;
		}
		// 下一行操作码
		ex = ex->prev_execute_data;
	}
}
/* }}} */

// ing4, 是否正在执行
ZEND_API bool zend_is_executing(void) /* {{{ */
{
	// 只要有执行上下文就算在执行
	return EG(current_execute_data) != 0;
}
/* }}} */

// ing3, 更新常量，大量调用。p1:常量，p2:更新时用到的域
ZEND_API zend_result ZEND_FASTCALL zval_update_constant_ex(zval *p, zend_class_entry *scope) /* {{{ */
{
	// #define IS_CONSTANT_AST 11 /* Constant expressions */
	// 如果是常量表达式，更新方式是从常量表里获得取。
	if (Z_TYPE_P(p) == IS_CONSTANT_AST) {
		// 取出语句
		zend_ast *ast = Z_ASTVAL_P(p);
		// 如果语句类型是 常量
		if (ast->kind == ZEND_AST_CONSTANT) {
			// 常量名
			zend_string *name = zend_ast_get_constant_name(ast);
			// 取得常量 zval
			zval *zv = zend_get_constant_ex(name, scope, ast->attr);
			// 如果没取到
			if (UNEXPECTED(zv == NULL)) {
				// 返回错误
				return FAILURE;
			}
			// 删除 p 下属的对象实例
			zval_ptr_dtor_nogc(p);
			// 给zv创建复本，并关联到p
			ZVAL_COPY_OR_DUP(p, zv);
		// 如果语句类型不是常量表达式，更新方式是是调用 zend_ast_evaluate 计算常量的值
		} else {
			zval tmp;
			// 在计算语句的过程中增加引用次数，避免语句过早释放。
			// 这种情况可能在重复触发语句计算时 在自动加载的过程中发生 。 
			// Increase the refcount during zend_ast_evaluate to avoid releasing the ast too early
			// on nested calls to zval_update_constant_ex which can happen when retriggering ast
			// evaluation during autoloading.
			// 取得ref
			zend_ast_ref *ast_ref = Z_AST_P(p);
			// 不带有 GC_IMMUTABLE 标记
			bool ast_is_refcounted = !(GC_FLAGS(ast_ref) & GC_IMMUTABLE);
			// 如果不带有 GC_IMMUTABLE 标记
			if (ast_is_refcounted) {
				// 添加引用次数
				GC_ADDREF(ast_ref);
			}
			// 计算语句
			zend_result result = zend_ast_evaluate(&tmp, ast, scope);
			// 如果不带有 GC_IMMUTABLE 标记 并且 -1后次数是0
			if (ast_is_refcounted && !GC_DELREF(ast_ref)) {
				// 删除 ast_ref
				rc_dtor_func((zend_refcounted *)ast_ref);
			}
			// 如果没有成功
			if (UNEXPECTED(result != SUCCESS)) {
				// 返回：出错 
				return FAILURE;
			}
			// 销毁 p 的下属对象
			zval_ptr_dtor_nogc(p);
			// 把tmp 复制给 p
			ZVAL_COPY_VALUE(p, &tmp);
		}
	}
	// 返回成功
	return SUCCESS;
}
/* }}} */

// ing3, 更新常量，全局无调用
ZEND_API zend_result ZEND_FASTCALL zval_update_constant(zval *pp) /* {{{ */
{
	// 从当前操作码或全局变量中获取作用域
	return zval_update_constant_ex(pp, EG(current_execute_data) ? zend_get_executed_scope() : CG(active_class_entry));
}
/* }}} */

// ing3, 调用用户方法的实现。p1:对象，p2:方法名，p3:返回值指针，p4:参数数量，p5:参数表，p6:命名参数表
zend_result _call_user_function_impl(zval *object, zval *function_name, zval *retval_ptr, uint32_t param_count, zval params[], HashTable *named_params) /* {{{ */
{
	// 调用信息
	zend_fcall_info fci;
	// 调用信息的大小
	fci.size = sizeof(fci);
	// 如果有对象
	if (object) {
		// 类型必须是对象
		ZEND_ASSERT(Z_TYPE_P(object) == IS_OBJECT);
		// 对象关联到调用信息
		fci.object = Z_OBJ_P(object);
	// 如果没有对象
	} else {
		// 对象指针为 null
		fci.object = NULL;
	}
	// 把方法名复制给 调用信息
	ZVAL_COPY_VALUE(&fci.function_name, function_name);
	// 关联返回值 zval
	fci.retval = retval_ptr;
	// 参数数量
	fci.param_count = param_count;
	// 参数列表
	fci.params = params;
	// 命名参数
	fci.named_params = named_params;
	
	// 调用函数
	return zend_call_function(&fci, NULL);
}
/* }}} */

// 270行
// ing3, 调用函数。p1:调用信息，p2:调用信息缓存
zend_result zend_call_function(zend_fcall_info *fci, zend_fcall_info_cache *fci_cache) /* {{{ */
{
	uint32_t i;
	zend_execute_data *call;
	zend_fcall_info_cache fci_cache_local;
	zend_function *func;
	uint32_t call_info;
	void *object_or_called_scope;
	zend_class_entry *orig_fake_scope;

	ZVAL_UNDEF(fci->retval);

	// 执行器必须已经激活
	if (!EG(active)) {
		return FAILURE; /* executor is already inactive */
	}

	// 如果有exception
	if (EG(exception)) {
		// 如果有 调用信息缓存
		if (fci_cache) {
			// 释放缓存的 函数调用信息
			zend_release_fcall_info_cache(fci_cache);
		}
		// 返回成功
		return SUCCESS; /* we would result in an instable executor otherwise */
	}

	// 调用信息大小必须 正确
	ZEND_ASSERT(fci->size == sizeof(zend_fcall_info));

	// 如果没有调用信息缓存 或 调用信息缓存里没有 处理函数
	if (!fci_cache || !fci_cache->function_handler) {
		// 错误为 null
		char *error = NULL;
		// 如果没有调用信息缓存
		if (!fci_cache) {
			// 使用 临时变量
			fci_cache = &fci_cache_local;
		}

		// 检查此调用在 当前指定执行数据中 是否可行。如果失败：
		// p1:调用信息，p2:对象, p3:检验级别，p4:返回调用字串，p5:fcc,关键是它 ，p6:返回错误信息
		if (!zend_is_callable_ex(&fci->function_name, fci->object, 0, NULL, fci_cache, &error)) {
			// 必须要有错误
			ZEND_ASSERT(error && "Should have error if not callable");
			// 取得调用字串 class::method 或 函数名.无法解析的数组返回 “Array”
			zend_string *callable_name
				= zend_get_callable_name_ex(&fci->function_name, fci->object);
			// 报错：无效的回调
			zend_throw_error(NULL, "Invalid callback %s, %s", ZSTR_VAL(callable_name), error);
			// 释放错误
			efree(error);
			// 释放调用名称
			zend_string_release_ex(callable_name, 0);
			// 返回成功
			return SUCCESS;
		}
		// 到这里一定不能有错误
		ZEND_ASSERT(!error);
	}

	// 取得调用信息缓存中的 处理函数
	func = fci_cache->function_handler;
	// 如果是静态函数 或 调用信息没有关联对象
	if ((func->common.fn_flags & ZEND_ACC_STATIC) || !fci_cache->object) {
		// 被调用的对象或类
		object_or_called_scope = fci_cache->called_scope;
		// 调用信息：顶层函数 或 动态调用
		call_info = ZEND_CALL_TOP_FUNCTION | ZEND_CALL_DYNAMIC;
		
	// 有关联对象的非静态调用
	} else {
		// 被调用的对象
		object_or_called_scope = fci_cache->object;
		// 调用信息：顶层函数 或 动态调用 或 调用中有this
		call_info = ZEND_CALL_TOP_FUNCTION | ZEND_CALL_DYNAMIC | ZEND_CALL_HAS_THIS;
	}

	// 初始化并返回 堆栈最上一个执行数据，空间不够则扩展大小
	// p1:调用信息，p2:函数，p3:参数数量，p4:对象或调用域
	call = zend_vm_stack_push_call_frame(call_info,
		func, fci->param_count, object_or_called_scope);

	// 如果方法是受保护的
	if (UNEXPECTED(func->common.fn_flags & ZEND_ACC_DEPRECATED)) {
		// 报错：函数或方法已弃用
		zend_deprecated_function(func);
		// 如果有异常
		if (UNEXPECTED(EG(exception))) {
			// 释放调用框架（执行数据）。p1:执行数据
			zend_vm_stack_free_call_frame(call);
			// 返回成功
			return SUCCESS;
		}
	}

	// 遍历所有参数
	for (i=0; i<fci->param_count; i++) {
		// 执行数据中的参数
		zval *param = ZEND_CALL_ARG(call, i+1);
		// 调用信息中的参数
		zval *arg = &fci->params[i];
		// 不必须引用传递
		bool must_wrap = 0;
		// 如果调用参数中参数未定义
		if (UNEXPECTED(Z_ISUNDEF_P(arg))) {
			// 允许前面有未定义缓存位置。只有 Closure::__invoke() 使用
			/* Allow forwarding undef slots. This is only used by Closure::__invoke(). */
			// 执行数据中的参数设置为未定义
			ZVAL_UNDEF(param);
			// Z_TYPE_INFO((p1)->This) |= p2
			ZEND_ADD_CALL_FLAG(call, ZEND_CALL_MAY_HAVE_UNDEF);
			// 下一个参数
			continue;
		}

		// 验证指定参数的发送模式，是否为引用传递 或 优先引用传递。如果是：
		if (ARG_SHOULD_BE_SENT_BY_REF(func, i + 1)) {
			// 如果参数不是引用传递
			if (UNEXPECTED(!Z_ISREF_P(arg))) {
				// 验证指定参数的发送模式，是否为 优先引用传递，如果不是：
				if (!ARG_MAY_BE_SENT_BY_REF(func, i + 1)) {
					// 引用类型的传递是不允许的。发布一个警告，然后用包在引用中的值来调用函数 
					/* By-value send is not allowed -- emit a warning,
					 * and perform the call with the value wrapped in a reference. */
					// 报错，此参数必须引用传递
					zend_param_must_be_ref(func, i + 1);
					// 必须引用传递
					must_wrap = 1;
					// 如果有异常
					if (UNEXPECTED(EG(exception))) {
						// 设置参数数量 
						ZEND_CALL_NUM_ARGS(call) = i;
// 清理参数
cleanup_args:
						// 释放所有执行数据中的 函数参数
						zend_vm_stack_free_args(call);
						// 释放调用框架（执行数据）。p1:执行数据
						zend_vm_stack_free_call_frame(call);
						// 返回成功
						return SUCCESS;
					}
				}
			}
		// 如果不用引用传递
		} else {
			if (Z_ISREF_P(arg) &&
			    !(func->common.fn_flags & ZEND_ACC_CALL_VIA_TRAMPOLINE)) {
				/* don't separate references for __call */
				arg = Z_REFVAL_P(arg);
			}
		}

		// 如果不需要包装成引用
		if (EXPECTED(!must_wrap)) {
			// 直接复制参数
			ZVAL_COPY(param, arg);
		// 需要包装成引用
		} else {
			// 添加引用次数
			Z_TRY_ADDREF_P(arg);
			// 更新成引用类型
			ZVAL_NEW_REF(param, arg);
		}
	}

	// 如果有命名参数
	if (fci->named_params) {
		zend_string *name;
		zval *arg;
		// 参数数量 +1
		uint32_t arg_num = ZEND_CALL_NUM_ARGS(call) + 1;
		// 命名参数数量 
		bool have_named_params = 0;
		// 遍历参数哈希表
		ZEND_HASH_FOREACH_STR_KEY_VAL(fci->named_params, name, arg) {
			// 不用包装成引用
			bool must_wrap = 0;
			zval *target;
			// 如果有名称
			if (name) {
				// 两个 null 指针
				void *cache_slot[2] = {NULL, NULL};
				// 有命名参数
				have_named_params = 1;
				// 处理命名参数，p1:执行数据指针的指针，p2:参数名，p3:参数数量 ，p4:缓存位置
				target = zend_handle_named_arg(&call, name, &arg_num, cache_slot);
				// 如果返回无效
				if (!target) {
					// 清理参数
					goto cleanup_args;
				}
			// 没有名称
			} else {
				// 如果有命名参数
				if (have_named_params) {
					// 报错：不可以把顺序参数放在命名参数后面
					zend_throw_error(NULL,
						"Cannot use positional argument after named argument");
					// 清理参数
					goto cleanup_args;
				}
				// 扩展执行数据。如果当堆栈前空间够用，增加使用数。
				// 否则，新建堆栈并复制执行数据。p1:执行数据，p2:传入参数，p3:增加参数
				zend_vm_stack_extend_call_frame(&call, arg_num - 1, 1);
				// 取得列表中第 （ZEND_CALL_FRAME_SLOT +） n 个 zval
				target = ZEND_CALL_ARG(call, arg_num);
			}

			// 验证指定参数的发送模式，是否为引用传递 或 优先引用传递
			if (ARG_SHOULD_BE_SENT_BY_REF(func, arg_num)) {
				// 如果不是引用类型
				if (UNEXPECTED(!Z_ISREF_P(arg))) {
					// 如果不可以引用传递
					if (!ARG_MAY_BE_SENT_BY_REF(func, arg_num)) {
						// 不允许引用传递，发布警告，然后用包在引用中的值来调用函数
						/* By-value send is not allowed -- emit a warning,
						 * and perform the call with the value wrapped in a reference. */
						// 报错，此参数必须引用传递
						zend_param_must_be_ref(func, arg_num);
						// 必须包装成引用
						must_wrap = 1;
						// 如果有异常
						if (UNEXPECTED(EG(exception))) {
							// 清朝参数
							goto cleanup_args;
						}
					}
				}
			// 不可以引用传递
			} else {
				// 如果是引用类型 并且 不是通过弹跳创建的函数
				if (Z_ISREF_P(arg) &&
					!(func->common.fn_flags & ZEND_ACC_CALL_VIA_TRAMPOLINE)) {
					// 不要为 __call 创建引用副本
					/* don't separate references for __call */
					// 解引用
					arg = Z_REFVAL_P(arg);
				}
			}

			// 如果不需要包装成引用
			if (EXPECTED(!must_wrap)) {
				// arg 复制给 target
				ZVAL_COPY(target, arg);
			// 需要包装成引用
			} else {
				// arg增加引用数
				Z_TRY_ADDREF_P(arg);
				// 更新成引用哦打
				ZVAL_NEW_REF(target, arg);
			}
			// 如果没有名称
			if (!name) {
				// 传入参数数 +1
				ZEND_CALL_NUM_ARGS(call)++;
				// 参数数 +1
				arg_num++;
			}
		} ZEND_HASH_FOREACH_END();
	}

	// 如果调用信息里面可以有未定义
	if (UNEXPECTED(ZEND_CALL_INFO(call) & ZEND_CALL_MAY_HAVE_UNDEF)) {
		// 处理未定义的参数，给它们添加默认值，p1:执行数据
		if (zend_handle_undef_args(call) == FAILURE) {
			// 释放所有执行数据中的 函数参数
			zend_vm_stack_free_args(call);
			// 释放调用框架（执行数据）。p1:执行数据
			zend_vm_stack_free_call_frame(call);
			// 返回成功
			return SUCCESS;
		}
	}

	// 如果调用函数是闭包
	if (UNEXPECTED(func->op_array.fn_flags & ZEND_ACC_CLOSURE)) {
		uint32_t call_info;
		// 函数增加引用次数
		GC_ADDREF(ZEND_CLOSURE_OBJECT(func));
		// 调用信息为：调用闭包
		call_info = ZEND_CALL_CLOSURE;
		// 如果是假闭包
		if (func->common.fn_flags & ZEND_ACC_FAKE_CLOSURE) {
			// 调用信息添加：调用假闭包标记
			call_info |= ZEND_CALL_FAKE_CLOSURE;
		}
		// 执行数据添加 调用信息。
		// Z_TYPE_INFO((p1)->This) |= p2
		ZEND_ADD_CALL_FLAG(call, call_info);
	}

	// 如果是通过弹跳创建的方法
	if (func->common.fn_flags & ZEND_ACC_CALL_VIA_TRAMPOLINE) {
		// 处理函数为 null
		fci_cache->function_handler = NULL;
	}

	// 原临时域
	orig_fake_scope = EG(fake_scope);
	// 清空临时域
	EG(fake_scope) = NULL;
	// 如果是用户定义的函数
	if (func->type == ZEND_USER_FUNCTION) {
		// 原 jit_trace_num
		uint32_t orig_jit_trace_num = EG(jit_trace_num);
		// 初始化函数的执行数据，p1:执行数据，p2:操作码组，p3:返回值
		zend_init_func_execute_data(call, &func->op_array, fci->retval);
		// 开始观察者调用
		ZEND_OBSERVER_FCALL_BEGIN(call);
		// 调用虚拟机执行此数据 execute_ex : /Zend/zend_vm_execute.h
		zend_execute_ex(call);
		// 还原 orig_jit_trace_num
		EG(jit_trace_num) = orig_jit_trace_num;
	// 不是用户定义的函数
	} else {
		// 必须是内置方法
		ZEND_ASSERT(func->type == ZEND_INTERNAL_FUNCTION);
		// 返回值为 null
		ZVAL_NULL(fci->retval);
		// 执行数据放到前一个
		call->prev_execute_data = EG(current_execute_data);
		// 更新当前执行数据
		EG(current_execute_data) = call;
// 调试用
#if ZEND_DEBUG
		bool should_throw = zend_internal_call_should_throw(func, call);
#endif
		// 开始观察者调用
		ZEND_OBSERVER_FCALL_BEGIN(call);
		// 如果没有 zend_execute_internal 方法 dtrace_execute_internal 或其他
		if (EXPECTED(zend_execute_internal == NULL)) {
			// 如果 zend_execute_internal 没使用过保存一个 函数调用
			/* saves one function call if zend_execute_internal is not used */
			// 调用内置函数的处理函数
			func->internal_function.handler(call, fci->retval);
		// 如果有
		} else {
			// 调用这个方法
			zend_execute_internal(call, fci->retval);
		}

// 调试用
#if ZEND_DEBUG
		if (!EG(exception) && call->func) {
			if (should_throw) {
				zend_internal_call_arginfo_violation(call->func);
			}
			ZEND_ASSERT(!(call->func->common.fn_flags & ZEND_ACC_HAS_RETURN_TYPE) ||
				zend_verify_internal_return_type(call->func, fci->retval));
			ZEND_ASSERT((call->func->common.fn_flags & ZEND_ACC_RETURN_REFERENCE)
				? Z_ISREF_P(fci->retval) : !Z_ISREF_P(fci->retval));
		}
#endif
		// 结束观察者调用
		ZEND_OBSERVER_FCALL_END(call, fci->retval);
		// 当前执行数据切换到 前一个
		EG(current_execute_data) = call->prev_execute_data;
		// 释放所有执行数据中的 函数参数
		zend_vm_stack_free_args(call);
		// 如果有命名参数
		if (UNEXPECTED(ZEND_CALL_INFO(call) & ZEND_CALL_HAS_EXTRA_NAMED_PARAMS)) {
			// 释放命名参数 表
			zend_array_release(call->extra_named_params);
		}

		// 如果有异常
		if (EG(exception)) {
			// 清空调用信息中的返回结果
			zval_ptr_dtor(fci->retval);
			// 结果设置成 未定义
			ZVAL_UNDEF(fci->retval);
		}

		// 这个标记通常在运行用户函数时检查，但不在内置函数检查。
		// 所以当函数执行时，看 interrupt 标记是否已设置
		/* This flag is regularly checked while running user functions, but not internal
		 * So see whether interrupt flag was set while the function was running... */
		// vm_interrupt设置成false，如果 它原本是true
		if (zend_atomic_bool_exchange_ex(&EG(vm_interrupt), false)) {
			// timed_out 是 true
			if (zend_atomic_bool_load_ex(&EG(timed_out))) {
				// 超时
				zend_timeout();
			// php_win32_signal_ctrl_interrupt_function ：/win32/signal.c	
			} else if (zend_interrupt_function) {
				zend_interrupt_function(EG(current_execute_data));
			}
		}
		
		// 如果需要释放this
		if (UNEXPECTED(ZEND_CALL_INFO(call) & ZEND_CALL_RELEASE_THIS)) {
			// 释放 this
			OBJ_RELEASE(Z_OBJ(call->This));
		}
	}
	
	// 恢复临时域
	EG(fake_scope) = orig_fake_scope;
	
	// 释放调用框架（执行数据）。p1:执行数据
	zend_vm_stack_free_call_frame(call);

	// 如果有异常
	if (UNEXPECTED(EG(exception))) {
		// 如果没有当前执行数据
		if (UNEXPECTED(!EG(current_execute_data))) {
			// 抛出内部异常。zend_exception.c
			zend_throw_exception_internal(NULL);
		// 如果当前在函数中 并且 函数是用户定义的
		} else if (EG(current_execute_data)->func &&
		           ZEND_USER_CODE(EG(current_execute_data)->func->common.type)) {
			// 重新抛异常		   
			zend_rethrow_exception(EG(current_execute_data));
		}
	}
	// 返回成功
	return SUCCESS;
}
/* }}} */

// ing3, 调用对象的方法，p1:方法，p2:对象，p3:域，p4:返回值，p5:参数数量，p6:参数列表，p7:命名参数表
ZEND_API void zend_call_known_function(
		zend_function *fn, zend_object *object, zend_class_entry *called_scope, zval *retval_ptr,
		uint32_t param_count, zval *params, HashTable *named_params)
{
	zval retval;
	zend_fcall_info fci;
	zend_fcall_info_cache fcic;
	// 方法必须有效
	ZEND_ASSERT(fn && "zend_function must be passed!");

	// 调用信息大小
	fci.size = sizeof(fci);
	// 调用对象
	fci.object = object;
	// 如果不接收返回值，用临时变量
	fci.retval = retval_ptr ? retval_ptr : &retval;
	// 参数数量 
	fci.param_count = param_count;
	// 参数表
	fci.params = params;
	// 命名参数表
	fci.named_params = named_params;
	// 函数名设置成未定义
	ZVAL_UNDEF(&fci.function_name); /* Unused */

	// 调用信息缓存，绑定处理函数
	fcic.function_handler = fn;
	// 调用信息缓存，绑定对象
	fcic.object = object;
	// 调用信息缓存，绑定类（调用域）
	fcic.called_scope = called_scope;
	// 调用函数
	zend_result result = zend_call_function(&fci, &fcic);
	// 如果结果是失败
	if (UNEXPECTED(result == FAILURE)) {
		// 如果有异常
		if (!EG(exception)) {
			// 报错：无法执行方法
			zend_error_noreturn(E_CORE_ERROR, "Couldn't execute method %s%s%s",
				fn->common.scope ? ZSTR_VAL(fn->common.scope->name) : "",
				fn->common.scope ? "::" : "", ZSTR_VAL(fn->common.function_name));
		}
	}
	// 如果返回值无效
	if (!retval_ptr) {
		// 销毁返回值
		zval_ptr_dtor(&retval);
	}
}

// ing4, 调用已知对象的方法，2个附加参数。p1:方法，p2:所属对象:p3，返回结果，p4,p5:参数
ZEND_API void zend_call_known_instance_method_with_2_params(
		zend_function *fn, zend_object *object, zval *retval_ptr, zval *param1, zval *param2)
{
	// 临时变量
	zval params[2];
	// 把两个参数复制进来
	ZVAL_COPY_VALUE(&params[0], param1);
	ZVAL_COPY_VALUE(&params[1], param2);
	// 调用方法
	zend_call_known_instance_method(fn, object, retval_ptr, 2, params);
}

// ing3, 如果方法存在，调用它，不存在也不报错，返回错误状态，p1:对象，p2:方法名，p3:返回值，p4:参数数量，p5:参数列表
ZEND_API zend_result zend_call_method_if_exists(
		zend_object *object, zend_string *method_name, zval *retval,
		uint32_t param_count, zval *params)
{
	// 调用信息
	zend_fcall_info fci;
	// 大小
	fci.size = sizeof(zend_fcall_info);
	// 对象
	fci.object = object;
	// 方法名
	ZVAL_STR(&fci.function_name, method_name);
	// 返回值
	fci.retval = retval;
	// 参数数量
	fci.param_count = param_count;
	// 参数列表
	fci.params = params;
	// 没有命名参数
	fci.named_params = NULL;

	// 调用信息缓存
	zend_fcall_info_cache fcc;
	// 检查此调用在 当前指定执行数据中 是否可行。如果失败：
	// p1:调用信息，p2:对象, p3:检验级别，p4:返回调用字串，p5:fcc,关键是它 ，p6:返回错误信息
	if (!zend_is_callable_ex(&fci.function_name, fci.object, IS_CALLABLE_SUPPRESS_DEPRECATIONS, NULL, &fcc, NULL)) {
		// 返回值为未定义
		ZVAL_UNDEF(retval);
		// 反回失败
		return FAILURE;
	}

	// 调用方法
	return zend_call_function(&fci, &fcc);
}

/* 0-9 a-z A-Z _ \ 0x80-0xff */
static const uint32_t valid_chars[8] = {
	0x00000000,
	0x03ff0000,
	0x97fffffe,
	0x07fffffe,
	0xffffffff,
	0xffffffff,
	0xffffffff,
	0xffffffff,
};

// ing3, 验证类名有效 /* 0-9 a-z A-Z _ \ 0x80-0xff */
ZEND_API bool zend_is_valid_class_name(zend_string *name) {
	// 遍历每一个字符
	for (size_t i = 0; i < ZSTR_LEN(name); i++) {
		// 取出字符
		unsigned char c = ZSTR_VAL(name)[i];
		// ZEND_BIT_TEST ：/Zend/zend_portability.h
		// 这个不懂为什么这么验，但意思是 检验 /* 0-9 a-z A-Z _ \ 0x80-0xff */
		if (!ZEND_BIT_TEST(valid_chars, c)) {
			// 有一个字符不合法，就返回false
			return 0;
		}
	}
	// 全部合法，返回true
	return 1;
}

// ing3, 查找类。p1:类名，p2:key，p3:flags
ZEND_API zend_class_entry *zend_lookup_class_ex(zend_string *name, zend_string *key, uint32_t flags) /* {{{ */
{
	zend_class_entry *ce = NULL;
	zval *zv;
	zend_string *lc_name;
	zend_string *autoload_name;
	uint32_t ce_cache = 0;
	// 步骤1，检查ce缓存
	// ZSTR_HAS_CE_CACHE(s) : 查检是否是ce缓存：对象（字串）的type_info中指定位置有 IS_STR_CLASS_NAME_MAP_PTR 标记
	// ZSTR_VALID_CE_CACHE(s) : 校验偏移量（s的gc计数） ，看它是否在已用指针范围内
	// 如果有此名称的缓存 ，并且缓存有效
	if (ZSTR_HAS_CE_CACHE(name) && ZSTR_VALID_CE_CACHE(name)) {
		// 取得name的gc计数
		ce_cache = GC_REFCOUNT(name);
		// 获取 类入口
		ce = GET_CE_CACHE(ce_cache);
		// 如果获取成功
		if (EXPECTED(ce)) {
			// 返回类实体
			return ce;
		}
	}

	// 步骤2，生成小写类名
	// 如果有key
	if (key) {
		// 小写名字
		lc_name = key;
	// 如果没有key
	} else {
		// 如果没有name或name无效
		if (name == NULL || !ZSTR_LEN(name)) {
			// 返回null
			return NULL;
		}
		// 如果是完整路径
		if (ZSTR_VAL(name)[0] == '\\') {
			// 创建zend_string
			lc_name = zend_string_alloc(ZSTR_LEN(name) - 1, 0);
			// 把整个路径和名称转小写
			zend_str_tolower_copy(ZSTR_VAL(lc_name), ZSTR_VAL(name) + 1, ZSTR_LEN(name) - 1);
		// 如果不是完整路径
		} else {
			// 名称转小写
			lc_name = zend_string_tolower(name);
		}
	}

	// 步骤3
	// 从类列表中用小写名称查找类
	zv = zend_hash_find(EG(class_table), lc_name);
	// 如果找到
	if (zv) {
		// 如果没有key
		if (!key) {
			// 释放小写名
			zend_string_release_ex(lc_name, 0);
		}
		// 取出zval里的 类入口指针
		ce = (zend_class_entry*)Z_PTR_P(zv);
		// 如果类没有 ZEND_ACC_LINKED 标记
		if (UNEXPECTED(!(ce->ce_flags & ZEND_ACC_LINKED))) {
			// 如果有 ZEND_FETCH_CLASS_ALLOW_UNLINKED 标记 或 
			if ((flags & ZEND_FETCH_CLASS_ALLOW_UNLINKED) || 
				// ( 有ZEND_FETCH_CLASS_ALLOW_NEARLY_LINKED标记 并且有 ZEND_ACC_NEARLY_LINKED标记 )
				((flags & ZEND_FETCH_CLASS_ALLOW_NEARLY_LINKED) &&
					(ce->ce_flags & ZEND_ACC_NEARLY_LINKED))) {
				// 如果 CG(unlinked_uses) 无效
				if (!CG(unlinked_uses)) {
					// 分配内存创建哈希表 CG(unlinked_uses)
					ALLOC_HASHTABLE(CG(unlinked_uses));
					// 初始化哈希表
					zend_hash_init(CG(unlinked_uses), 0, NULL, NULL, 0);
				}
				// 添加空元素 ，键为 ce的指针转成int型
				zend_hash_index_add_empty_element(CG(unlinked_uses), (zend_long)(zend_uintptr_t)ce);
				// 反回类入口
				return ce;
			}
			// 否则返回null
			return NULL;
		}
		
		// 在编译过程中，不要为可转换的类添加 ce 缓存。这个类可能在 persisting 中被释放 ？
		/* Don't populate CE_CACHE for mutable classes during compilation.
		 * The class may be freed while persisting. */
		// 如果有类缓存 并且 （不在编译中 或 类有可转换标记）
		if (ce_cache &&
				(!CG(in_compilation) || (ce->ce_flags & ZEND_ACC_IMMUTABLE))) {
			// 在 CG(map_ptr_base) 指针列表通过偏移量写入 类入口指针
			SET_CE_CACHE(ce_cache, ce);
		}
		return ce;
	}
	
	// 步骤4， autoload
	// 编译器是 不可重复进入的。保证只在执行时自动加载类
	/* The compiler is not-reentrant. Make sure we autoload only during run-time. */
	// 如果不允许autoload 或 正在编译
	if ((flags & ZEND_FETCH_CLASS_NO_AUTOLOAD) || zend_is_compiling()) {
		// 如果没有key
		if (!key) {
			// 释放小写名
			zend_string_release_ex(lc_name, 0);
		}
		// 查找失败
		return NULL;
	}
	// 如果允许 autoload 并且不在编译中
	if (!zend_autoload) {
		// 如果没有key
		if (!key) {
			// 释放
			zend_string_release_ex(lc_name, 0);
		}
		// 查找失败
		return NULL;
	}

	// 传给 autoloader 之前先检验类名
	/* Verify class name before passing it to the autoloader. */
	// 如果没有key 并且 name没有缓存  并且 类名无效
	if (!key && !ZSTR_HAS_CE_CACHE(name) && !zend_is_valid_class_name(name)) {
		// 释放小写名
		zend_string_release_ex(lc_name, 0);
		// 查找失败
		return NULL;
	}

	// 如果 EG(in_autoload) 哈希表为空
	if (EG(in_autoload) == NULL) {
		// 分配哈希表
		ALLOC_HASHTABLE(EG(in_autoload));
		// 初始化哈希表
		zend_hash_init(EG(in_autoload), 8, NULL, NULL, 0);
	}

	// 把类名添加进 EG(in_autoload) 哈希表，值为空
	if (zend_hash_add_empty_element(EG(in_autoload), lc_name) == NULL) {
		// 如果没有key
		if (!key) {
			// 释放小写名
			zend_string_release_ex(lc_name, 0);
		}
		// 查找失败
		return NULL;
	}
	
	// 步骤5， 清理临时变量
	// 如果是绝对路径
	if (ZSTR_VAL(name)[0] == '\\') {
		// 名称为第一个 \ 后面的部分
		autoload_name = zend_string_init(ZSTR_VAL(name) + 1, ZSTR_LEN(name) - 1, 0);
	// 不是绝对路径
	} else {
		// 复制一份类名
		autoload_name = zend_string_copy(name);
	}

	// 把异常保存到 previous 链里 ,清空当前异常
	zend_exception_save();
	// spl_perform_autoload : ext\spl\php_spl.c
	ce = zend_autoload(autoload_name, lc_name);
	// 如果有当前异常，把上一个异常存储起来，如果没有，把上一个作为当前异常
	zend_exception_restore();
	
	// 释放autoload用的类名
	zend_string_release_ex(autoload_name, 0);
	// 在 EG(in_autoload) 哈希表里删除此名称
	zend_hash_del(EG(in_autoload), lc_name);
	// 如果没有传入key,肯定用了小写名
	if (!key) {
		// 释放小写名
		zend_string_release_ex(lc_name, 0);
	}
	// 如果 自动加载成功
	if (ce) {
		// 必须不在编译中
		ZEND_ASSERT(!CG(in_compilation));
		// 如果需要ce缓存
		if (ce_cache) {
			// 在 CG(map_ptr_base) 指针列表通过偏移量写入 类入口指针
			SET_CE_CACHE(ce_cache, ce);
		}
	}
	// 返回类入口
	return ce;
}
/* }}} */

// ing4, 查找类
ZEND_API zend_class_entry *zend_lookup_class(zend_string *name) /* {{{ */
{
	// 查找类。p1:类名，p2:key，p3:flags
	return zend_lookup_class_ex(name, NULL, 0);
}
/* }}} */

// ing2, 在执行数据链 中获取调用域
ZEND_API zend_class_entry *zend_get_called_scope(zend_execute_data *ex) /* {{{ */
{
	// 如果执行数据有效
	while (ex) {
		// 如果 this 是对象
		if (Z_TYPE(ex->This) == IS_OBJECT) {
			// 返回  zval中zend_object 的 类入口
			return Z_OBJCE(ex->This);
		// #define Z_CE(zval) (zval).value.ce
		// 如果 this 是类入口
		} else if (Z_CE(ex->This)) {
			// 返回类入口
			return Z_CE(ex->This);
		// 如果 ex有所属函数
		} else if (ex->func) {
			// 如果此函数不是内置函数（是用户定义） 或 此函数有所属类，这是什么情况？
			if (ex->func->type != ZEND_INTERNAL_FUNCTION || ex->func->common.scope) {
				// 返回null
				return NULL;
			}
		}
		// 前一个执行数据 
		ex = ex->prev_execute_data;
	}
	// 查找失败返回null
	return NULL;
}
/* }}} */

// ing3, 查找 $this 对象
ZEND_API zend_object *zend_get_this_object(zend_execute_data *ex) /* {{{ */
{
	while (ex) {
		// 如果 This 属性是对象
		if (Z_TYPE(ex->This) == IS_OBJECT) {
			// 返回 This 中的对象
			return Z_OBJ(ex->This);
		// 如果有函数 
		} else if (ex->func) {
			// 如果函数不是内置函数 或 函数有作用域（是类静态方法）
			if (ex->func->type != ZEND_INTERNAL_FUNCTION || ex->func->common.scope) {
				// 
				return NULL;
			}
		}
		// 找到前一个 zend_execute_data
		ex = ex->prev_execute_data;
	}
	// 
	return NULL;
}
/* }}} */

// ing3, 执行给出的代码。p1:代码字串，p2:长度，p3:返回值，p4:字串名
ZEND_API zend_result zend_eval_stringl(const char *str, size_t str_len, zval *retval_ptr, const char *string_name) /* {{{ */
{
	zend_op_array *new_op_array;
	uint32_t original_compiler_options;
	zend_result retval;
	zend_string *code_str;

	// 如果接收返回值
	if (retval_ptr) {
		// 拼接 return 语句
		code_str = zend_string_concat3(
			"return ", sizeof("return ")-1, str, str_len, ";", sizeof(";")-1);
	// 不接收不用加 return;
	} else {
		code_str = zend_string_init(str, str_len, 0);
	}

	/*printf("Evaluating '%s'\n", pv.value.str.val);*/

	// 源编译配置
	original_compiler_options = CG(compiler_options);
	// 临时改成 ZEND_COMPILE_DEFAULT_FOR_EVAL
	CG(compiler_options) = ZEND_COMPILE_DEFAULT_FOR_EVAL;
	// 编译字串 compile_string : zend_language_scanner.l
	new_op_array = zend_compile_string(code_str, string_name, ZEND_COMPILE_POSITION_AFTER_OPEN_TAG);
	// 恢复原编译配置
	CG(compiler_options) = original_compiler_options;

	// 如果有操作码组
	if (new_op_array) {
		zval local_retval;
		// 支持扩展信息
		EG(no_extensions)=1;
		// 从执行数据中，获取执行时作用域
		new_op_array->scope = zend_get_executed_scope();

		zend_try {
			// 临时变量设置为 未定义
			ZVAL_UNDEF(&local_retval);
			// 执行操作码
			// /Zend/zend_vm_execute.h
			zend_execute(new_op_array, &local_retval);
		} zend_catch {
			// 销毁操作码组
			destroy_op_array(new_op_array);
			// 释放操作码组
			efree_size(new_op_array, sizeof(zend_op_array));
			// 跳伞
			zend_bailout();
		} zend_end_try();

		// 如果临时结果 不是 未定义
		if (Z_TYPE(local_retval) != IS_UNDEF) {
			// 如接收结果
			if (retval_ptr) {
				// 把临时结果复制给返回值
				ZVAL_COPY_VALUE(retval_ptr, &local_retval);
			// 不接收结果
			} else {
				// 销毁临时结果
				zval_ptr_dtor(&local_retval);
			}
		// 结果是未定义
		} else {
			// 如接收结果
			if (retval_ptr) {
				// 结果为 null
				ZVAL_NULL(retval_ptr);
			}
		}

		// 不支持扩展信息
		EG(no_extensions)=0;
		// 销毁静态变量
		zend_destroy_static_vars(new_op_array);
		// 销毁操作码组
		destroy_op_array(new_op_array);
		// 释放操作码组
		efree_size(new_op_array, sizeof(zend_op_array));
		// 返回成功
		retval = SUCCESS;
	} else {
		// 返回失败
		retval = FAILURE;
	}
	// 释放字串，程序代码
	zend_string_release(code_str);
	// 返回
	return retval;
}
/* }}} */

// ing4, 执行给出的代码。p1:代码字串，p2:返回值，p4:字串名
ZEND_API zend_result zend_eval_string(const char *str, zval *retval_ptr, const char *string_name) /* {{{ */
{
	// 执行给出的代码。p1:代码字串，p2:长度，p3:返回值，p4:字串名
	return zend_eval_stringl(str, strlen(str), retval_ptr, string_name);
}
/* }}} */

// ing3, 执行给出的代码。p1:代码字串，p2:长度，p3:返回值，p4:字串名，p5:处理异常
ZEND_API zend_result zend_eval_stringl_ex(const char *str, size_t str_len, zval *retval_ptr, const char *string_name, bool handle_exceptions) /* {{{ */
{
	zend_result result;

	// 执行给出的代码。p1:代码字串，p2:长度，p3:返回值，p4:字串名
	result = zend_eval_stringl(str, str_len, retval_ptr, string_name);
	// 如果要处理异常 并且有异常
	if (handle_exceptions && EG(exception)) {
		// 按exception报错
		result = zend_exception_error(EG(exception), E_ERROR);
	}
	// 返回结果
	return result;
}
/* }}} */

// ing4, 执行给出的代码。p1:代码字串，p2:返回值，p3:字串名，p4:处理异常
ZEND_API zend_result zend_eval_string_ex(const char *str, zval *retval_ptr, const char *string_name, bool handle_exceptions) /* {{{ */
{
	return zend_eval_stringl_ex(str, strlen(str), retval_ptr, string_name, handle_exceptions);
}
/* }}} */

// abstract
static void zend_set_timeout_ex(zend_long seconds, bool reset_signals);

// ing3, 执行报错：超时
ZEND_API ZEND_NORETURN void ZEND_FASTCALL zend_timeout(void) /* {{{ */
{
// windows走这里
#if defined(PHP_WIN32)
// 线程安全
# ifndef ZTS
	// 如果0秒被忽略了，不需要执行操作。
	// 并且，hard_timeout 必须被遵守。
	// 如果计时器没有正确重启，它会在 关闭函数中被挂起
	/* No action is needed if we're timed out because zero seconds are
	   just ignored. Also, the hard timeout needs to be respected. If the
	   timer is not restarted properly, it could hang in the shutdown
	   function. */
	// 有运行时间
	if (EG(hard_timeout) > 0) {
		// 更新 timed_out 为 false
		zend_atomic_bool_store_ex(&EG(timed_out), false);
		// 设置超时时间, p1:超时时间，p2:windows 没用到
		zend_set_timeout_ex(EG(hard_timeout), 1);
		/* XXX Abused, introduce an additional flag if the value needs to be kept. */
		EG(hard_timeout) = 0;
	}
# endif

// windows不走这里
#else
	zend_atomic_bool_store_ex(&EG(timed_out), false);
	zend_set_timeout_ex(0, 1);
#endif

	// 没有线程安全的话只走这里。
	// 报错：超过了最大执行时间
	zend_error_noreturn(E_ERROR, "Maximum execution time of " ZEND_LONG_FMT " second%s exceeded", EG(timeout_seconds), EG(timeout_seconds) == 1 ? "" : "s");
}
/* }}} */

// 如果是windows
#ifndef ZEND_WIN32

// 默认没有这个
# ifdef ZEND_MAX_EXECUTION_TIMERS
static void zend_timeout_handler(int dummy, siginfo_t *si, void *uc) /* {{{ */
{
	if (si->si_value.sival_ptr != &EG(max_execution_timer_timer)) {
// 调试
#ifdef MAX_EXECUTION_TIMERS_DEBUG
		fprintf(stderr, "Executing previous handler (if set) for unexpected signal SIGRTMIN received on thread %d\n", (pid_t) syscall(SYS_gettid));
#endif

		if (EG(oldact).sa_sigaction) {
			EG(oldact).sa_sigaction(dummy, si, uc);

			return;
		}
		if (EG(oldact).sa_handler) EG(oldact).sa_handler(dummy);

		return;
	}
// 默认走这里
# else
// ing3, 超时处理器
static void zend_timeout_handler(int dummy) /* {{{ */
{
# endif
// 以上是定义函数名和参数列表

// 如果没有线程安全
#ifndef ZTS
	// 如果开启了 time_out
	if (zend_atomic_bool_load_ex(&EG(timed_out))) {
		// 在硬超时时 die
		/* Die on hard timeout */
		const char *error_filename = NULL;
		uint32_t error_lineno = 0;
		char log_buffer[2048];
		int output_len = 0;

		// 如果正在编译
		if (zend_is_compiling()) {
			// 编译文件名
			error_filename = ZSTR_VAL(zend_get_compiled_filename());
			// 编译行号
			error_lineno = zend_get_compiled_lineno();
		// 如果正在执行
		} else if (zend_is_executing()) {
			// 执行文件名
			error_filename = zend_get_executed_filename();
			// 如果文件名是 [ 开头 。 文件名是 [no active file]
			if (error_filename[0] == '[') { /* [no active file] */
				// 清空文件名
				error_filename = NULL;
				// 行号为0
				error_lineno = 0;
			// 否则 
			} else {
				// 取得正在执行的行号
				error_lineno = zend_get_executed_lineno();
			}
		}
		// 如果没有出错文件名
		if (!error_filename) {
			// 使用 Unknown
			error_filename = "Unknown";
		}

		// 组织log内容
		output_len = snprintf(log_buffer, sizeof(log_buffer), "\nFatal error: Maximum execution time of " ZEND_LONG_FMT "+" ZEND_LONG_FMT " seconds exceeded (terminated) in %s on line %d\n", EG(timeout_seconds), EG(hard_timeout), error_filename, error_lineno);
		// 如果有log内容
		if (output_len > 0) {
			// 静默写入缓存 ： /Zend/zend_portability.h
			// 只是调用系统函数 write
			zend_quiet_write(2, log_buffer, MIN(output_len, sizeof(log_buffer)));
		}
		// 系统函数
		_exit(124);
	}
#endif

	// 如果有这个 监听函数 
	// php_on_timeout ：/main/main.c
	if (zend_on_timeout) {
		// 调用它
		zend_on_timeout(EG(timeout_seconds));
	}

	// EG(timed_out) = true
	zend_atomic_bool_store_ex(&EG(timed_out), true);
	// EG(vm_interrupt) = true
	zend_atomic_bool_store_ex(&EG(vm_interrupt), true);
	
// 线程安全
#ifndef ZTS
	if (EG(hard_timeout) > 0) {
		/* Set hard timeout */
		zend_set_timeout_ex(EG(hard_timeout), 1);
	}
#endif
}
/* }}} */
#endif

// windows 走这里
#ifdef ZEND_WIN32
// ing3, 超时回调。p1:zend_executor_globals 指针，p2:超时时间
VOID CALLBACK tq_timer_cb(PVOID arg, BOOLEAN timed_out)
{
	// 
	zend_executor_globals *eg;

	// 
	/* The doc states it'll be always true, however it theoretically
		could be FALSE when the thread was signaled. */
	// 没传time_out
	if (!timed_out) {
		// 直接返回
		return;
	}

	// 第一个参数，转成 zend_executor_globals
	eg = (zend_executor_globals *)arg;
	// 更新参数中的 ->timed_out = true
	zend_atomic_bool_store_ex(&eg->timed_out, true);
	// 更新参数中的 ->vm_interrupt = true
	zend_atomic_bool_store_ex(&eg->vm_interrupt, true);
}
#endif

/* This one doesn't exists on QNX */
#ifndef SIGPROF
#define SIGPROF 27
#endif

// ing3, 设置超时时间, p1:超时时间，p2:windows 没用到
static void zend_set_timeout_ex(zend_long seconds, bool reset_signals) /* {{{ */
{
// windows 用
#ifdef ZEND_WIN32
	// 
	zend_executor_globals *eg;
	// 如果没有给定时间
	if (!seconds) {
		// 返回
		return;
	}

	// 不要使用 ChangeTimerQueueTimer() 它不会重启超时的 计时器。
	// 所以只需要通过 忽略超时时间来结束。替代 删除和重新创建操作。
	/* Don't use ChangeTimerQueueTimer() as it will not restart an expired
	 * timer, so we could end up with just an ignored timeout. Instead
	 * delete and recreate. */
	// 如果有 tq_timer
	if (NULL != tq_timer) {
		// windows原生方法，删除计时器。如果失败：
		if (!DeleteTimerQueueTimer(NULL, tq_timer, INVALID_HANDLE_VALUE)) {
			// 清空计时器
			tq_timer = NULL;
			// 报错：无法删除队列计时器
			zend_error_noreturn(E_ERROR, "Could not delete queued timer");
			// 
			return;
		}
		// 成功也清空 计时器
		tq_timer = NULL;
	}

	// 传入null表示 系统提供的默认计时列表 已经被使用了
	/* XXX passing NULL means the default timer queue provided by the system is used */
	// 返回此模块的 全局变量表结构体 的地址
	eg = ZEND_MODULE_GLOBALS_BULK(executor);
	// 如果创建队列计时器失败
	if (!CreateTimerQueueTimer(&tq_timer, NULL, (WAITORTIMERCALLBACK)tq_timer_cb, (VOID*)eg, seconds*1000, 0, WT_EXECUTEONLYONCE)) {
		// 计时器为空
		tq_timer = NULL;
		// 报错：无法创建新计时器
		zend_error_noreturn(E_ERROR, "Could not queue new timer");
		// 
		return;
	}
	
// 不是windows, 如果定义的最大执行时间
#elif defined(ZEND_MAX_EXECUTION_TIMERS)
	zend_max_execution_timer_settime(seconds);

	if (reset_signals) {
		sigset_t sigset;
		struct sigaction act;

		act.sa_sigaction = zend_timeout_handler;
		sigemptyset(&act.sa_mask);
		act.sa_flags = SA_ONSTACK | SA_SIGINFO;
		sigaction(SIGRTMIN, &act, NULL);
		sigemptyset(&sigset);
		sigaddset(&sigset, SIGRTMIN);
		sigprocmask(SIG_UNBLOCK, &sigset, NULL);
	}
// 不是windows, 有 HAVE_SETITIMER，到最后
#elif defined(HAVE_SETITIMER)
	{
		struct itimerval t_r;		/* timeout requested */
		int signo;

		if(seconds) {
			t_r.it_value.tv_sec = seconds;
			t_r.it_value.tv_usec = t_r.it_interval.tv_sec = t_r.it_interval.tv_usec = 0;

# if defined(__CYGWIN__) || defined(__PASE__)
			setitimer(ITIMER_REAL, &t_r, NULL);
		}
		signo = SIGALRM;
# else
			setitimer(ITIMER_PROF, &t_r, NULL);
		}
		signo = SIGPROF;
# endif

		if (reset_signals) {
# ifdef ZEND_SIGNALS
			zend_signal(signo, zend_timeout_handler);
# else
			sigset_t sigset;
#  ifdef HAVE_SIGACTION
			struct sigaction act;

			act.sa_handler = zend_timeout_handler;
			sigemptyset(&act.sa_mask);
			act.sa_flags = SA_RESETHAND | SA_NODEFER;
			sigaction(signo, &act, NULL);
#  else
			signal(signo, zend_timeout_handler);
#  endif /* HAVE_SIGACTION */
			sigemptyset(&sigset);
			sigaddset(&sigset, signo);
			sigprocmask(SIG_UNBLOCK, &sigset, NULL);
# endif /* ZEND_SIGNALS */
		}
	}
// 
#endif /* HAVE_SETITIMER */
}
/* }}} */

// ing3, 设置超时时间
void zend_set_timeout(zend_long seconds, bool reset_signals) /* {{{ */
{
	// 设置超时时间
	EG(timeout_seconds) = seconds;
	// 设置超时时间, p1:超时时间，p2:windows 没用到
	zend_set_timeout_ex(seconds, reset_signals);
	// EG(timed_out) = false
	zend_atomic_bool_store_ex(&EG(timed_out), false);
}
/* }}} */

// ing3, 删除超时计时器
void zend_unset_timeout(void) /* {{{ */
{
// windows走这里
#ifdef ZEND_WIN32
	// 如果计时器有效
	if (NULL != tq_timer) {
		// windows原生方法，删除计时器
		if (!DeleteTimerQueueTimer(NULL, tq_timer, INVALID_HANDLE_VALUE)) {
			// EG(timed_out) 更新为 false
			zend_atomic_bool_store_ex(&EG(timed_out), false);
			// 计时器为 null
			tq_timer = NULL;
			// 报错：无法删除队列计时器
			zend_error_noreturn(E_ERROR, "Could not delete queued timer");
			// 中断
			return;
		}
		// 删除成功，指针清空
		tq_timer = NULL;
	}
	// 还有最后面一行
// 不走这里	
#elif ZEND_MAX_EXECUTION_TIMERS
	zend_max_execution_timer_settime(0);
#elif defined(HAVE_SETITIMER)
	if (EG(timeout_seconds)) {
		struct itimerval no_timeout;

		no_timeout.it_value.tv_sec = no_timeout.it_value.tv_usec = no_timeout.it_interval.tv_sec = no_timeout.it_interval.tv_usec = 0;

// 
# if defined(__CYGWIN__) || defined(__PASE__)
		setitimer(ITIMER_REAL, &no_timeout, NULL);
// 64位windows走这里
# else
		setitimer(ITIMER_PROF, &no_timeout, NULL);
# endif
	}
#endif
	// 把 EG(timed_out) 更新成 false
	zend_atomic_bool_store_ex(&EG(timed_out), false);
}
/* }}} */

// ing3, 报错：无法找到此类（或接口，trait）
static ZEND_COLD void report_class_fetch_error(zend_string *class_name, uint32_t fetch_type)
{
	// 如果要求静默
	if (fetch_type & ZEND_FETCH_CLASS_SILENT) {
		// 直接返回
		return;
	}
	// 如果已经有异常
	if (EG(exception)) {
		// 如果 报错类型不是 ZEND_FETCH_CLASS_EXCEPTION
		if (!(fetch_type & ZEND_FETCH_CLASS_EXCEPTION)) {
			// 抛错：正在查找类
			zend_exception_uncaught_error("During class fetch");
		}
		return;
	}
	// 如果是查找接口
	if ((fetch_type & ZEND_FETCH_CLASS_MASK) == ZEND_FETCH_CLASS_INTERFACE) {
		// 抛错：未找到接口
		zend_throw_or_error(fetch_type, NULL, "Interface \"%s\" not found", ZSTR_VAL(class_name));
	// 如果是查找 trait
	} else if ((fetch_type & ZEND_FETCH_CLASS_MASK) == ZEND_FETCH_CLASS_TRAIT) {
		// 抛错：未找到trait
		zend_throw_or_error(fetch_type, NULL, "Trait \"%s\" not found", ZSTR_VAL(class_name));
	// 如果是在查找类
	} else {
		// 抛错：未找到类
		zend_throw_or_error(fetch_type, NULL, "Class \"%s\" not found", ZSTR_VAL(class_name));
	}
}

// ing3, 通过类名或类型查找类，p1:类名，p2:类型
zend_class_entry *zend_fetch_class(zend_string *class_name, uint32_t fetch_type) /* {{{ */
{
	zend_class_entry *ce, *scope;
	// 取得获取类型
	uint32_t fetch_sub_type = fetch_type & ZEND_FETCH_CLASS_MASK;

check_fetch_type:
	// 按获取类型操作
	switch (fetch_sub_type) {
		// self
		case ZEND_FETCH_CLASS_SELF:
			// 从执行数据中，获取执行时作用域
			scope = zend_get_executed_scope();
			// 如果作用域无效
			if (UNEXPECTED(!scope)) {
				// 报错：不在class作用域中无法使用 self
				zend_throw_or_error(fetch_type, NULL, "Cannot access \"self\" when no class scope is active");
			}
			// 返回找到的类
			return scope;
		// parent
		case ZEND_FETCH_CLASS_PARENT:
			// 从执行数据中，获取执行时作用域
			scope = zend_get_executed_scope();
			// 没有作用域
			if (UNEXPECTED(!scope)) {
				// 报错：不在类作用域中无法使用 parent
				zend_throw_or_error(fetch_type, NULL, "Cannot access \"parent\" when no class scope is active");
				return NULL;
			}
			// 作为域没有父类
			if (UNEXPECTED(!scope->parent)) {
				// 报错：类没有parent,无法使用 self
				zend_throw_or_error(fetch_type, NULL, "Cannot access \"parent\" when current class scope has no parent");
			}
			// 返回父类
			return scope->parent;
		// static：是会查找父类的
		case ZEND_FETCH_CLASS_STATIC:
			// 在执行数据链 中获取调用域
			ce = zend_get_called_scope(EG(current_execute_data));
			// 没有作用域
			if (UNEXPECTED(!ce)) {
				// 报错：不在类作用域中无法使用 static
				zend_throw_or_error(fetch_type, NULL, "Cannot access \"static\" when no class scope is active");
				// 返回null
				return NULL;
			}
			// 返回作用域
			return ce;
		// ?? zend_fetch_ce_from_cache_slot：zend_execute.c 中，字串没有关联类时，带这个标记
		// 支持使用 class_name 的类型 ?
		case ZEND_FETCH_CLASS_AUTO: {
				// 通过类名获取类的引用方式，self,parent,static 或其他。
				fetch_sub_type = zend_get_class_fetch_type(class_name);
				// 如果不是默认方法引用 ？
				if (UNEXPECTED(fetch_sub_type != ZEND_FETCH_CLASS_DEFAULT)) {
					// 从头再来
					goto check_fetch_type;
				}
			}
			break;
	}
	// 查找类。p1:类名，p2:key，p3:flags
	ce = zend_lookup_class_ex(class_name, NULL, fetch_type);
	// 如果找不到类
	if (!ce) {
		// 报错：无法找到此类（或接口，trait）
		report_class_fetch_error(class_name, fetch_type);
		// 返回null
		return NULL;
	}
	// 返回找到的类
	return ce;
}
/* }}} */

// ing3, 获取类。调用不合法时，报错。p1：类名（支持self,parent）, p2:类入口
zend_class_entry *zend_fetch_class_with_scope(
		zend_string *class_name, uint32_t fetch_type, zend_class_entry *scope)
{
	zend_class_entry *ce;
	switch (fetch_type & ZEND_FETCH_CLASS_MASK) {
		// 用 self 访问
		case ZEND_FETCH_CLASS_SELF:
			// 如果没有域
			if (UNEXPECTED(!scope)) {
				// 报错：不可以不在活动域中 访问 self
				zend_throw_or_error(fetch_type, NULL, "Cannot access \"self\" when no class scope is active");
			}
			// 返回传入域
			return scope;
		// 用 parent 访问
		case ZEND_FETCH_CLASS_PARENT:
			// 如果没有域
			if (UNEXPECTED(!scope)) {
				// 报错：不可以不在活动域中 访问 parent
				zend_throw_or_error(fetch_type, NULL, "Cannot access \"parent\" when no class scope is active");
				// 返回null
				return NULL;
			}
			// 如果没有父类
			if (UNEXPECTED(!scope->parent)) {
				// 报错：不可以在没有父类的域中 访问 parent
				zend_throw_or_error(fetch_type, NULL, "Cannot access \"parent\" when current class scope has no parent");
			}
			// 返回 父类
			return scope->parent;
		// 其他情况
		case 0:
			break;
		/* Other fetch types are not supported by this function. */
		EMPTY_SWITCH_DEFAULT_CASE()
	}
	// 查找类。p1:类名，p2:key，p3:flags
	ce = zend_lookup_class_ex(class_name, NULL, fetch_type);
	// 如果找不到
	if (!ce) {
		// 报错：无法找到此类（或接口，trait）
		report_class_fetch_error(class_name, fetch_type);
		return NULL;
	}
	return ce;
}

// ing3, 获取类，找不到会报错。p1:类名，p2:key，p3:flags
zend_class_entry *zend_fetch_class_by_name(zend_string *class_name, zend_string *key, uint32_t fetch_type) /* {{{ */
{
	// 查找类。p1:类名，p2:key，p3:flags
	zend_class_entry *ce = zend_lookup_class_ex(class_name, key, fetch_type);
	// 如果没找一
	if (!ce) {
		// 报错：无法找到此类（或接口，trait）
		report_class_fetch_error(class_name, fetch_type);
		return NULL;
	}
	return ce;
}
/* }}} */

// ing4, 在全局符号表中删除指定 名称的元素
ZEND_API zend_result zend_delete_global_variable(zend_string *name) /* {{{ */
{
	return zend_hash_del_ind(&EG(symbol_table), name);
}
/* }}} */

// ing3, 重建并返回当前执行数据的符号表
ZEND_API zend_array *zend_rebuild_symbol_table(void) /* {{{ */
{
	zend_execute_data *ex;
	zend_array *symbol_table;

	// 查找 最后调用的函数
	/* Search for last called user function */
	// 当前执行数据
	ex = EG(current_execute_data);
	// 执行数据有效，并且 （没有函数 或 函数不是用户定义的）
	while (ex && (!ex->func || !ZEND_USER_CODE(ex->func->common.type))) {
		// 使用前一个执行数据（都向后挪一个）
		ex = ex->prev_execute_data;
	}
	// 如果碰到空执行数据
	if (!ex) {
		// 返回null
		return NULL;
	}
	// 如果 this中有符号表
	// Z_TYPE_INFO((execute_data)->This))
	if (ZEND_CALL_INFO(ex) & ZEND_CALL_HAS_SYMBOL_TABLE) {
		// 返回符号表
		return ex->symbol_table;
	}
	// this中没有符号表

	// 添加 ZEND_CALL_HAS_SYMBOL_TABLE 标记
	ZEND_ADD_CALL_FLAG(ex, ZEND_CALL_HAS_SYMBOL_TABLE);
	// 如果指针不在开头位置，符号缓存有使用过
	if (EG(symtable_cache_ptr) > EG(symtable_cache)) {
		// EG(symtable_cache_ptr 指针倒回一个位置
		symbol_table = ex->symbol_table = *(--EG(symtable_cache_ptr));
		// 如果没有编译变量
		if (!ex->func->op_array.last_var) {
			// 直接返回符号表
			return symbol_table;
		}
		// 重新计算符号表大小，把编译变量算进去
		zend_hash_extend(symbol_table, ex->func->op_array.last_var, 0);
	// 如果指针 在开头位置
	} else {
		// 创建新哈希表， 编译变量的大小
		symbol_table = ex->symbol_table = zend_new_array(ex->func->op_array.last_var);
		// 如果没有用到编译变量
		if (!ex->func->op_array.last_var) {
			// 直接返回空 哈希表
			return symbol_table;
		}
		// 重新初始化混合哈希表
		zend_hash_real_init_mixed(symbol_table);
		/*printf("Cache miss!  Initialized %x\n", EG(active_symbol_table));*/
	}
	
	// 如果有编译变量
	if (EXPECTED(ex->func->op_array.last_var)) {
		// 编译变量名开头位置
		zend_string **str = ex->func->op_array.vars;
		// 编译变量名结束位置
		zend_string **end = str + ex->func->op_array.last_var;
		// 临时变量开头
		zval *var = ZEND_CALL_VAR_NUM(ex, 0);

		// 
		do {
			// 添加进符号表, 变量名称做key
			_zend_hash_append_ind(symbol_table, *str, var);
			// 下一个名称
			str++;
			// 下一个变量
			var++;
		// 遍历所有编译变量
		} while (str != end);
	}
	// 返回符号表
	return symbol_table;
}
/* }}} */

// ing3, 从附加符号表里读取所有临时变量，并更新到临时变量列表中
ZEND_API void zend_attach_symbol_table(zend_execute_data *execute_data) /* {{{ */
{
	// 执行数据中的函数操作码
	zend_op_array *op_array = &execute_data->func->op_array;
	// 执行数据中的符号表
	HashTable *ht = execute_data->symbol_table;

	// 把符号表中的真实值 复制到 编译变量位置 并且 在符号表中 创建 编译变量的 间接引用
	/* copy real values from symbol table into CV slots and create
	   INDIRECT references to CV in symbol table  */
	// 如果有临时变量
	if (EXPECTED(op_array->last_var)) {
		// 临时变量名 指针列表开头
		zend_string **str = op_array->vars;
		// 列表结尾
		zend_string **end = str + op_array->last_var;
		// 第一个临时变量
		zval *var = EX_VAR_NUM(0);

		// 遍历所有变量名
		do {
			// 在符号表中查找这个变量
			zval *zv = zend_hash_find_known_hash(ht, *str);
			// 如果找到
			if (zv) {
				// 如果 是间接引用
				if (Z_TYPE_P(zv) == IS_INDIRECT) {
					// 解引用
					zval *val = Z_INDIRECT_P(zv);
					// 值复制到临时变量里
					ZVAL_COPY_VALUE(var, val);
				// 不是间接引用
				} else {
					// 值复制到临时变量里 
					ZVAL_COPY_VALUE(var, zv);
				}
			// 如果没找到
			} else {
				// 变量值为 未定义
				ZVAL_UNDEF(var);
				// 添加 这个名称的临时变量
				zv = zend_hash_add_new(ht, *str, var);
			}
			// 把找到的值更新成间接引用
			ZVAL_INDIRECT(zv, var);
			// 下一个变量名
			str++;
			// 下一个变量
			var++;
		
		} while (str != end);
	}
}
/* }}} */

// ing3, 把临时变量中的编译变量 同步到符号表中，然后清空临时变量
ZEND_API void zend_detach_symbol_table(zend_execute_data *execute_data) /* {{{ */
{
	// 操作码组
	zend_op_array *op_array = &execute_data->func->op_array;
	// 符号表
	HashTable *ht = execute_data->symbol_table;

	// 把编译变量位置的 真正值 复制到符号表中
	/* copy real values from CV slots into symbol table */
	if (EXPECTED(op_array->last_var)) {
		// 编译变量名开头位置
		zend_string **str = op_array->vars;
		// 编译变量名结束位置
		zend_string **end = str + op_array->last_var;
		// 第一个临时变量位置
		zval *var = EX_VAR_NUM(0);

		// 
		do {
			// 如果变量类型 是未定义
			if (Z_TYPE_P(var) == IS_UNDEF) {
				// 从符号表里删除这个key对应的元素
				zend_hash_del(ht, *str);
			// 如果类型不是未定义
			} else {
				// 更新符号表
				zend_hash_update(ht, *str, var);
				// 当前变量设置成 未定义
				ZVAL_UNDEF(var);
			}
			// 下一个变量名
			str++;
			// 下一个临时变量
			var++;
		// 遍历编译变量名
		} while (str != end);
	}
}
/* }}} */

// ing3, 通过变量名 更新执行数据中的临时变量，p1:变量名（zend_string），p2:值，p3:是否重建符号表
ZEND_API zend_result zend_set_local_var(zend_string *name, zval *value, bool force) /* {{{ */
{
	// 当前执行数据
	zend_execute_data *execute_data = EG(current_execute_data);

	// 执行数据有效 并且 （没有函数 或 函数不是用户定义）
	while (execute_data && (!execute_data->func || !ZEND_USER_CODE(execute_data->func->common.type))) {
		// 取得前一个执行数据
		execute_data = execute_data->prev_execute_data;
	}
	// 取到的执行数据里 一定有 用户定义的函数

	// 如果执行数据有效
	if (execute_data) {
		// 如果执行数据中 没有 符号表
		if (!(EX_CALL_INFO() & ZEND_CALL_HAS_SYMBOL_TABLE)) {
			// 计算字串哈希值
			zend_ulong h = zend_string_hash_val(name);
			// 操作码组
			zend_op_array *op_array = &execute_data->func->op_array;

			// 如果有编译变量
			if (EXPECTED(op_array->last_var)) {
				// 编译变量列表开头
				zend_string **str = op_array->vars;
				// 编译变量列表结尾
				zend_string **end = str + op_array->last_var;

				//
				do {
					// 如果哈希值匹配 并且 名称相同
					if (ZSTR_H(*str) == h &&
					    zend_string_equal_content(*str, name)) {
						// 取出 这个变量
						zval *var = EX_VAR_NUM(str - op_array->vars);
						// 给这个变量赋值
						ZVAL_COPY_VALUE(var, value);
						// 返回成功
						return SUCCESS;
					}
					// 下一个变量名
					str++;
				// 遍历所有编译变量
				} while (str != end);
			}
			// 如果要求强制
			if (force) {
				// 先重建符号表
				zend_array *symbol_table = zend_rebuild_symbol_table();
				// 如果符号表有效
				if (symbol_table) {
					// 更新
					zend_hash_update(symbol_table, name, value);
					// 返回成功
					return SUCCESS;
				}
			}
		// 没有符号表
		} else {
			// 通过 键名 更新指定元素
			zend_hash_update_ind(execute_data->symbol_table, name, value);
			// 返回成功
			return SUCCESS;
		}
	}
	// 反回失败
	return FAILURE;
}
/* }}} */

// ing3, 通过变量名 更新执行数据中的临时变量，p1:变量名（char*），p2:长度，p3:值，p4:是否重建符号表
ZEND_API zend_result zend_set_local_var_str(const char *name, size_t len, zval *value, bool force) /* {{{ */
{
	// 当前执行数据
	zend_execute_data *execute_data = EG(current_execute_data);

	// 如果没有函数 或 不是用户定义，就向前找
	while (execute_data && (!execute_data->func || !ZEND_USER_CODE(execute_data->func->common.type))) {
		execute_data = execute_data->prev_execute_data;
	}

	// 如果有执行数据
	if (execute_data) {
		// 如果没有符号表
		if (!(EX_CALL_INFO() & ZEND_CALL_HAS_SYMBOL_TABLE)) {
			// 计算字串哈希值
			zend_ulong h = zend_hash_func(name, len);
			// 操作码组
			zend_op_array *op_array = &execute_data->func->op_array;
			// 遍历每一个操作码
			if (EXPECTED(op_array->last_var)) {
				// 编译变量列表开头
				zend_string **str = op_array->vars;
				// 编译变量列表结尾
				zend_string **end = str + op_array->last_var;

				do {
					// 如果哈希值匹配 并且 名称相同
					if (ZSTR_H(*str) == h &&
					    zend_string_equals_cstr(*str, name, len)) {
						// 取出 这个变量
						zval *var = EX_VAR_NUM(str - op_array->vars);
						// 清空此变量
						zval_ptr_dtor(var);
						// 给这个变量赋值
						ZVAL_COPY_VALUE(var, value);
						// 返回成功
						return SUCCESS;
					}
					// 下一个变量名
					str++;
				} while (str != end);
			}
			// 如果要求强制
			if (force) {
				// 先重建符号表
				zend_array *symbol_table = zend_rebuild_symbol_table();
				// 如果符号表有效
				if (symbol_table) {
					// 更新
					zend_hash_str_update(symbol_table, name, len, value);
					// 返回成功
					return SUCCESS;
				}
			}
		// 如果有符号表
		} else {
			// 通过 键名 更新指定元素
			zend_hash_str_update_ind(execute_data->symbol_table, name, len, value);
			// 返回成功
			return SUCCESS;
		}
	}
	// 返回失败
	return FAILURE;
}
/* }}} */
