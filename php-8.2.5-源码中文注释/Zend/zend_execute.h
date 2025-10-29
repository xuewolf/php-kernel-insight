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

#ifndef ZEND_EXECUTE_H
#define ZEND_EXECUTE_H

#include "zend_compile.h"
#include "zend_hash.h"
#include "zend_operators.h"
#include "zend_variables.h"

BEGIN_EXTERN_C()
struct _zend_fcall_info;
ZEND_API extern void (*zend_execute_ex)(zend_execute_data *execute_data);
ZEND_API extern void (*zend_execute_internal)(zend_execute_data *execute_data, zval *return_value);

/* The lc_name may be stack allocated! */
ZEND_API extern zend_class_entry *(*zend_autoload)(zend_string *name, zend_string *lc_name);

void init_executor(void);
void shutdown_executor(void);
void shutdown_destructors(void);
ZEND_API void zend_shutdown_executor_values(bool fast_shutdown);

ZEND_API void zend_init_execute_data(zend_execute_data *execute_data, zend_op_array *op_array, zval *return_value);
ZEND_API void zend_init_func_execute_data(zend_execute_data *execute_data, zend_op_array *op_array, zval *return_value);
ZEND_API void zend_init_code_execute_data(zend_execute_data *execute_data, zend_op_array *op_array, zval *return_value);
ZEND_API void zend_execute(zend_op_array *op_array, zval *return_value);
ZEND_API void execute_ex(zend_execute_data *execute_data);
ZEND_API void execute_internal(zend_execute_data *execute_data, zval *return_value);
ZEND_API bool zend_is_valid_class_name(zend_string *name);
ZEND_API zend_class_entry *zend_lookup_class(zend_string *name);
ZEND_API zend_class_entry *zend_lookup_class_ex(zend_string *name, zend_string *lcname, uint32_t flags);
ZEND_API zend_class_entry *zend_get_called_scope(zend_execute_data *ex);
ZEND_API zend_object *zend_get_this_object(zend_execute_data *ex);
ZEND_API zend_result zend_eval_string(const char *str, zval *retval_ptr, const char *string_name);
ZEND_API zend_result zend_eval_stringl(const char *str, size_t str_len, zval *retval_ptr, const char *string_name);
ZEND_API zend_result zend_eval_string_ex(const char *str, zval *retval_ptr, const char *string_name, bool handle_exceptions);
ZEND_API zend_result zend_eval_stringl_ex(const char *str, size_t str_len, zval *retval_ptr, const char *string_name, bool handle_exceptions);

/* export zend_pass_function to allow comparisons against it */
extern ZEND_API const zend_internal_function zend_pass_function;

ZEND_API ZEND_COLD void ZEND_FASTCALL zend_missing_arg_error(zend_execute_data *execute_data);
ZEND_API ZEND_COLD void ZEND_FASTCALL zend_deprecated_function(const zend_function *fbc);
ZEND_API ZEND_COLD void ZEND_FASTCALL zend_false_to_array_deprecated(void);
ZEND_COLD void ZEND_FASTCALL zend_param_must_be_ref(const zend_function *func, uint32_t arg_num);
ZEND_API ZEND_COLD void ZEND_FASTCALL zend_use_resource_as_offset(const zval *dim);

ZEND_API bool ZEND_FASTCALL zend_verify_ref_assignable_zval(zend_reference *ref, zval *zv, bool strict);
ZEND_API bool ZEND_FASTCALL zend_verify_prop_assignable_by_ref(zend_property_info *prop_info, zval *orig_val, bool strict);

ZEND_API ZEND_COLD void zend_throw_ref_type_error_zval(zend_property_info *prop, zval *zv);
ZEND_API ZEND_COLD void zend_throw_ref_type_error_type(zend_property_info *prop1, zend_property_info *prop2, zval *zv);
ZEND_API ZEND_COLD zval* ZEND_FASTCALL zend_undefined_offset_write(HashTable *ht, zend_long lval);
ZEND_API ZEND_COLD zval* ZEND_FASTCALL zend_undefined_index_write(HashTable *ht, zend_string *offset);
ZEND_API ZEND_COLD void zend_wrong_string_offset_error(void);

ZEND_API ZEND_COLD void ZEND_FASTCALL zend_readonly_property_modification_error(zend_property_info *info);
ZEND_API ZEND_COLD void ZEND_FASTCALL zend_readonly_property_indirect_modification_error(zend_property_info *info);

ZEND_API bool zend_verify_scalar_type_hint(uint32_t type_mask, zval *arg, bool strict, bool is_internal_arg);
ZEND_API ZEND_COLD void zend_verify_arg_error(
		const zend_function *zf, const zend_arg_info *arg_info, uint32_t arg_num, zval *value);
ZEND_API ZEND_COLD void zend_verify_return_error(
		const zend_function *zf, zval *value);
ZEND_API ZEND_COLD void zend_verify_never_error(
		const zend_function *zf);
ZEND_API bool zend_verify_ref_array_assignable(zend_reference *ref);
ZEND_API bool zend_check_user_type_slow(
		zend_type *type, zval *arg, zend_reference *ref, void **cache_slot, bool is_return_type);

#if ZEND_DEBUG
ZEND_API bool zend_internal_call_should_throw(zend_function *fbc, zend_execute_data *call);
ZEND_API ZEND_COLD void zend_internal_call_arginfo_violation(zend_function *fbc);
ZEND_API bool zend_verify_internal_return_type(zend_function *zf, zval *ret);
#endif

// ing4, 返回引用源（引用目标） ->sources
#define ZEND_REF_TYPE_SOURCES(ref) \
	(ref)->sources

// ing4, 检验引用目标是否有效 ((p1)->sources).ptr != NULL
#define ZEND_REF_HAS_TYPE_SOURCES(ref) \
	(ZEND_REF_TYPE_SOURCES(ref).ptr != NULL)

// ing3, 如果ref引用的是列表 ？返回属性信息列表 ：p1的引用目标
#define ZEND_REF_FIRST_SOURCE(ref) \
	/* 检验 属性信息源 是否是 列表（最后 1 位是 1） */ \
	(ZEND_PROPERTY_INFO_SOURCE_IS_LIST((ref)->sources.list) \
		/* 把 list 转成 zend_property_info_list 指针（去掉最后一位的1） */ \
		? ZEND_PROPERTY_INFO_SOURCE_TO_LIST((ref)->sources.list)->ptr[0] \
		/* 使用ref引用目标 */ \
		: (ref)->sources.ptr)


ZEND_API void ZEND_FASTCALL zend_ref_add_type_source(zend_property_info_source_list *source_list, zend_property_info *prop);
ZEND_API void ZEND_FASTCALL zend_ref_del_type_source(zend_property_info_source_list *source_list, zend_property_info *prop);

ZEND_API zval* zend_assign_to_typed_ref(zval *variable_ptr, zval *value, zend_uchar value_type, bool strict);

// ing3, 把值复制给变量 p1:变量，p2:值，p3:值的变量类型（常量，变量，编译变量）
static zend_always_inline void zend_copy_to_variable(zval *variable_ptr, zval *value, zend_uchar value_type)
{
	zend_refcounted *ref = NULL;

	// 如果值为引用类型
	// ZEND_CONST_COND：windows直接取 p2
	if (ZEND_CONST_COND(value_type & (IS_VAR|IS_CV), 1) && Z_ISREF_P(value)) {
		// 计数器
		ref = Z_COUNTED_P(value);
		// 解引用
		value = Z_REFVAL_P(value);
	}

	// 把值复制给变量
	ZVAL_COPY_VALUE(variable_ptr, value);
	
	// window 不走这里
	// ZEND_CONST_COND：windows直接取 p2
	if (ZEND_CONST_COND(value_type  == IS_CONST, 0)) {
		// 变量 是否是可以计引用数的类型
		if (UNEXPECTED(Z_OPT_REFCOUNTED_P(variable_ptr))) {
			// 变量增加引用次数
			Z_ADDREF_P(variable_ptr);
		}
	// 如果值为 常量 或 编译变量
	} else if (value_type & (IS_CONST|IS_CV)) {
		// 变量 是否是可以计引用数的类型
		if (Z_OPT_REFCOUNTED_P(variable_ptr)) {
			// 变量增加引用次数
			Z_ADDREF_P(variable_ptr);
		}
	// ZEND_CONST_COND：windows直接取 p2
	} else if (ZEND_CONST_COND(value_type == IS_VAR, 1) && UNEXPECTED(ref)) {
		// 引用数 -1 后为0
		if (UNEXPECTED(GC_DELREF(ref) == 0)) {
			// 释放引用实例
			efree_size(ref, sizeof(zend_reference));
		// 变量 是否是可以计引用数的类型
		} else if (Z_OPT_REFCOUNTED_P(variable_ptr)) {
			// 变量增加引用次数
			Z_ADDREF_P(variable_ptr);
		}
	}
}

// ing3, 给变量赋值，p1:变量，p2:值，p3:值的变量类型（常量，变量，编译变量），p4:是否严格类型
static zend_always_inline zval* zend_assign_to_variable(zval *variable_ptr, zval *value, zend_uchar value_type, bool strict)
{
	do {
		// 如果是可计数类型
		if (UNEXPECTED(Z_REFCOUNTED_P(variable_ptr))) {
			// 临时变量
			zend_refcounted *garbage;
			// 如果是引用类型
			if (Z_ISREF_P(variable_ptr)) {
				// 检验引用目标是否有效，如果有效
				if (UNEXPECTED(ZEND_REF_HAS_TYPE_SOURCES(Z_REF_P(variable_ptr)))) {
					// ing3, 给有规定类型的引用 赋值
					// p1:被赋值的引用对象，p2:赋值（可能被强制转换），p3:值类型（变量或其他），p4:是否严格类型
					return zend_assign_to_typed_ref(variable_ptr, value, value_type, strict);
				}
				// 解引用
				variable_ptr = Z_REFVAL_P(variable_ptr);
				// 如果变量不可计数
				if (EXPECTED(!Z_REFCOUNTED_P(variable_ptr))) {
					// 跳出
					break;
				}
			}
			// 取得计数器，准备回收
			garbage = Z_COUNTED_P(variable_ptr);
			// 把值复制给变量 p1:变量，p2:值，p3:值的变量类型（常量，变量，编译变量）
			zend_copy_to_variable(variable_ptr, value, value_type);
			// 如果引用-1后为0
			if (GC_DELREF(garbage) == 0) {
				// 销毁旧变量值
				rc_dtor_func(garbage);
			// 否则，需要拆分
			} else { /* we need to split */
				// GC_ZVAL_CHECK_POSSIBLE_ROOT(variable_ptr) 的优化版本
				/* optimized version of GC_ZVAL_CHECK_POSSIBLE_ROOT(variable_ptr) */
				// 如果可能泄漏
				if (UNEXPECTED(GC_MAY_LEAK(garbage))) {
					// 放入垃圾回收 
					gc_possible_root(garbage);
				}
			}
			// 返回变量
			return variable_ptr;
		}
	} while (0);
	
	// 把值复制给变量 p1:变量，p2:值，p3:值的变量类型（常量，变量，编译变量）
	zend_copy_to_variable(variable_ptr, value, value_type);
	// 返回变量
	return variable_ptr;
}

ZEND_API zend_result ZEND_FASTCALL zval_update_constant(zval *pp);
ZEND_API zend_result ZEND_FASTCALL zval_update_constant_ex(zval *pp, zend_class_entry *scope);

// 专用的 ZEND 执行器方法，其他地方不要使用它！
/* dedicated Zend executor functions - do not use! */
// 虚拟机堆栈，它的大小是 24，3个指针（测试过）
struct _zend_vm_stack {
	// 头元素
	zval *top;
	// 尾元素
	zval *end;
	// typedef struct _zend_vm_stack *zend_vm_stack;
	// 前一个堆栈的指针
	zend_vm_stack prev;
};

// sizeof(struct _zend_vm_stack) = 24
// ing3, 64位：ZEND_MM_ALIGNED_SIZE((24+16-1)/16)= ZEND_MM_ALIGNED_SIZE(2)=8。8*16=128Bytes
#define ZEND_VM_STACK_HEADER_SLOTS \
	((ZEND_MM_ALIGNED_SIZE(sizeof(struct _zend_vm_stack)) + ZEND_MM_ALIGNED_SIZE(sizeof(zval)) - 1) / ZEND_MM_ALIGNED_SIZE(sizeof(zval)))

// 跳过stack头部（64位系统中8个zval,128Bytes）,找到元素列表开头
#define ZEND_VM_STACK_ELEMENTS(stack) \
	(((zval*)(stack)) + ZEND_VM_STACK_HEADER_SLOTS)

/*
 * In general in RELEASE build ZEND_ASSERT() must be zero-cost, but for some
 * reason, GCC generated worse code, performing CSE on assertion code and the
 * following "slow path" and moving memory read operations from slow path into
 * common header. This made a degradation for the fast path.
 * The following "#if ZEND_DEBUG" eliminates it.
 */
// 调试用
#if ZEND_DEBUG
# define ZEND_ASSERT_VM_STACK(stack) ZEND_ASSERT(stack->top > (zval *) stack && stack->end > (zval *) stack && stack->top <= stack->end)
# define ZEND_ASSERT_VM_STACK_GLOBAL ZEND_ASSERT(EG(vm_stack_top) > (zval *) EG(vm_stack) && EG(vm_stack_end) > (zval *) EG(vm_stack) && EG(vm_stack_top) <= EG(vm_stack_end))
// 正式环境这两个都是空
#else
// 正式环境为空
# define ZEND_ASSERT_VM_STACK(stack)
// 正式环境为空
# define ZEND_ASSERT_VM_STACK_GLOBAL
#endif

ZEND_API void zend_vm_stack_init(void);
ZEND_API void zend_vm_stack_init_ex(size_t page_size);
ZEND_API void zend_vm_stack_destroy(void);
ZEND_API void* zend_vm_stack_extend(size_t size);

// ing3, 创建新的虚拟机堆栈页。p1:大小，p2:前一个堆栈页
static zend_always_inline zend_vm_stack zend_vm_stack_new_page(size_t size, zend_vm_stack prev) {
	// 分配内存空间，创建新的堆栈页
	zend_vm_stack page = (zend_vm_stack)emalloc(size);

	// 初始化堆栈
	// stack 跳过头部分（2个zval） ，找到存数据的位置
	page->top = ZEND_VM_STACK_ELEMENTS(page);
	// 指向末尾，后面不再有空间
	page->end = (zval*)((char*)page + size);
	// 指向前一个堆栈页
	page->prev = prev;
	// 返回当前页指针
	return page;
}

// ing3, 虚拟机方法，初始化调用框架。为把被调用函数的相关信息附加到 执行数据对象上
// p1:执行数据，p2:调用信息，p3:函数，p4:参数数量，p5:对象或调用域
static zend_always_inline void zend_vm_init_call_frame(zend_execute_data *call, uint32_t call_info, zend_function *func, uint32_t num_args, void *object_or_called_scope)
{
	// 断言：函数不在类里，或有 object_or_called_scope
	ZEND_ASSERT(!func->common.scope || object_or_called_scope);
	// 更新执行数据的 func
	call->func = func;
	// 更新执行数据的  This 对象（作用域）
	Z_PTR(call->This) = object_or_called_scope;
	// 为执行数据 设置调用信息
	ZEND_CALL_INFO(call) = call_info;
	// 为执行数据 设置参数数量
	ZEND_CALL_NUM_ARGS(call) = num_args;
}

// ing3, 初始化并返回 堆栈最上一个执行数据，空间不够则扩展大小
// p1:使用大小，p2:调用信息，p3:函数，p4:参数数量，p5:对象或调用域
static zend_always_inline zend_execute_data *zend_vm_stack_push_call_frame_ex(uint32_t used_stack, uint32_t call_info, zend_function *func, uint32_t num_args, void *object_or_called_scope)
{
	// 取得虚拟机堆栈里最上一个执行数据
	zend_execute_data *call = (zend_execute_data*)EG(vm_stack_top);
	// 正式环境为空
	ZEND_ASSERT_VM_STACK_GLOBAL;

	// 如果使用大小大于 剩余大小
	if (UNEXPECTED(used_stack > (size_t)(((char*)EG(vm_stack_end)) - (char*)call))) {
		// 扩展堆栈空间
		call = (zend_execute_data*)zend_vm_stack_extend(used_stack);
		// 正式环境为空
		ZEND_ASSERT_VM_STACK_GLOBAL;
		// 虚拟机方法，初始化调用框架。为把被调用函数的相关信息附加到 执行数据对象上
		// p1:执行数据，p2:调用信息，p3:函数，p4:参数数量，p5:对象或调用域
		zend_vm_init_call_frame(call, call_info | ZEND_CALL_ALLOCATED, func, num_args, object_or_called_scope);
		// 返回最上一个执行数据
		return call;
	// 剩余空间够大
	} else {
		EG(vm_stack_top) = (zval*)((char*)call + used_stack);
		// 虚拟机方法，初始化调用框架。为把被调用函数的相关信息附加到 执行数据对象上
		// p1:执行数据，p2:调用信息，p3:函数，p4:参数数量，p5:对象或调用域
		zend_vm_init_call_frame(call, call_info, func, num_args, object_or_called_scope);
		// 返回最上一个执行数据
		return call;
	}
}

// ing3, 计算使用的变量大小。p1:参数数量，p2:函数
static zend_always_inline uint32_t zend_vm_calc_used_stack(uint32_t num_args, zend_function *func)
{
	// ZEND_CALL_FRAME_SLOT:64位windows里是 2 + 参数数量  + 临时变量数量
	uint32_t used_stack = ZEND_CALL_FRAME_SLOT + num_args + func->common.T;

	// 如果函数是用户定义的
	if (EXPECTED(ZEND_USER_CODE(func->type))) {
		// used_stack + 编译变量数量 - 形参和实参里小的一个
		used_stack += func->op_array.last_var - MIN(func->op_array.num_args, num_args);
	}
	// 使用变量数 * 变量大小
	return used_stack * sizeof(zval);
}

// ing3, 初始化并返回 堆栈最上一个执行数据，空间不够则扩展大小
// p1:调用信息，p2:函数，p3:参数数量，p4:对象或调用域
static zend_always_inline zend_execute_data *zend_vm_stack_push_call_frame(uint32_t call_info, zend_function *func, uint32_t num_args, void *object_or_called_scope)
{
	// 计算使用的变量大小。p1:参数数量，p2:函数
	uint32_t used_stack = zend_vm_calc_used_stack(num_args, func);

	// 初始化并返回 堆栈最上一个执行数据，空间不够则扩展大小
	// p1:使用大小，p2:调用信息，p3:函数，p4:参数数量，p5:对象或调用域
	return zend_vm_stack_push_call_frame_ex(used_stack, call_info,
		func, num_args, object_or_called_scope);
}

// ing3, 销毁 虚拟机堆栈里的额外参数，p1:调用信息，p2:执行数据
static zend_always_inline void zend_vm_stack_free_extra_args_ex(uint32_t call_info, zend_execute_data *call)
{
	// 如果要释放 附加参数
	if (UNEXPECTED(call_info & ZEND_CALL_FREE_EXTRA_ARGS)) {
		// 附加参数数量 = 实参数量 - 形参数量
		uint32_t count = ZEND_CALL_NUM_ARGS(call) - call->func->op_array.num_args;
		// 取得列表中第 ZEND_CALL_FRAME_SLOT + n 个 zval
		zval *p = ZEND_CALL_VAR_NUM(call, call->func->op_array.last_var + call->func->op_array.T);
		// 遍历每个参数
		do {
			// 销毁没有引用次数的对象，有引用次数的添加到gc回收周期里。并没有销毁 zval本身。
			i_zval_ptr_dtor(p);
			// 下一个参数
			p++;
		// 计数-1
		} while (--count);
 	}
}

// ing4, 销毁 虚拟机堆栈里的额外参数，p1:执行数据
static zend_always_inline void zend_vm_stack_free_extra_args(zend_execute_data *call)
{
	zend_vm_stack_free_extra_args_ex(ZEND_CALL_INFO(call), call);
}

// ing3, 释放所有执行数据中的 函数参数
static zend_always_inline void zend_vm_stack_free_args(zend_execute_data *call)
{
	// 参数数量
	uint32_t num_args = ZEND_CALL_NUM_ARGS(call);

	// 如果有参数
	if (EXPECTED(num_args > 0)) {
		// 第一个参数
		zval *p = ZEND_CALL_ARG(call, 1);

		// 
		do {
			// 销毁没有引用次数的对象,并没有销毁 zval本身。nogc的意思是，不放入gc回收周期。
			zval_ptr_dtor_nogc(p);
			// 下一个参数
			p++;
		// 遍历所有参数
		} while (--num_args);
	}
}

// ing3, 释放调用框架（执行数据），p1:调用信息，p2:执行数据
static zend_always_inline void zend_vm_stack_free_call_frame_ex(uint32_t call_info, zend_execute_data *call)
{
	// 正式环境为空
	ZEND_ASSERT_VM_STACK_GLOBAL;

	// 如果是分配过的调用 ZEND_CALL_ALLOCATED
	if (UNEXPECTED(call_info & ZEND_CALL_ALLOCATED)) {
		// 虚拟机堆栈
		zend_vm_stack p = EG(vm_stack);
		// 前一个堆栈页
		zend_vm_stack prev = p->prev;
		// stack 跳过头部分（2个zval） ，找到存数据的位置
		ZEND_ASSERT(call == (zend_execute_data*)ZEND_VM_STACK_ELEMENTS(EG(vm_stack)));
		// 前一个页的最上元素
		EG(vm_stack_top) = prev->top;
		// 前一个页的最后元素
		EG(vm_stack_end) = prev->end;
		// 使用前一个堆栈页
		EG(vm_stack) = prev;
		// 释放当前堆栈页
		efree(p);
	// 不是分配过的调用
	} else {
		// 只更新 EG(vm_stack_top)
		EG(vm_stack_top) = (zval*)call;
	}
	
	// 正式环境为空
	ZEND_ASSERT_VM_STACK_GLOBAL;
}

// ing4, 释放调用框架（执行数据）。p1:执行数据
static zend_always_inline void zend_vm_stack_free_call_frame(zend_execute_data *call)
{
	// 释放调用框架（执行数据），p1:调用信息，p2:执行数据
	zend_vm_stack_free_call_frame_ex(ZEND_CALL_INFO(call), call);
}

zend_execute_data *zend_vm_stack_copy_call_frame(
	zend_execute_data *call, uint32_t passed_args, uint32_t additional_args);

// ing3, 扩展执行数据。如果当堆栈前空间够用，增加使用数。
// 否则，新建堆栈并复制执行数据。p1:执行数据，p2:传入参数，p3:增加参数
static zend_always_inline void zend_vm_stack_extend_call_frame(
	zend_execute_data **call, uint32_t passed_args, uint32_t additional_args)
{
	// 如果堆栈还有空间
	if (EXPECTED((uint32_t)(EG(vm_stack_end) - EG(vm_stack_top)) > additional_args)) {
		// 附加参数指针放到堆栈里
		EG(vm_stack_top) += additional_args;
	// 堆栈没有空间
	} else {
		// 复制执行数据到新创建的执行数据中。 p1:执行数据，p2:传入参数，p3:附加参数
		*call = zend_vm_stack_copy_call_frame(*call, passed_args, additional_args);
	}
}

ZEND_API void ZEND_FASTCALL zend_free_extra_named_params(zend_array *extra_named_params);

/* services */
ZEND_API const char *get_active_class_name(const char **space);
ZEND_API const char *get_active_function_name(void);
ZEND_API const char *get_active_function_arg_name(uint32_t arg_num);
ZEND_API const char *get_function_arg_name(const zend_function *func, uint32_t arg_num);
ZEND_API zend_string *get_active_function_or_method_name(void);
ZEND_API zend_string *get_function_or_method_name(const zend_function *func);
ZEND_API const char *zend_get_executed_filename(void);
ZEND_API zend_string *zend_get_executed_filename_ex(void);
ZEND_API uint32_t zend_get_executed_lineno(void);
ZEND_API zend_class_entry *zend_get_executed_scope(void);
ZEND_API bool zend_is_executing(void);
ZEND_API ZEND_COLD void ZEND_FASTCALL zend_cannot_pass_by_reference(uint32_t arg_num);

ZEND_API void zend_set_timeout(zend_long seconds, bool reset_signals);
ZEND_API void zend_unset_timeout(void);
ZEND_API ZEND_NORETURN void ZEND_FASTCALL zend_timeout(void);
ZEND_API zend_class_entry *zend_fetch_class(zend_string *class_name, uint32_t fetch_type);
ZEND_API zend_class_entry *zend_fetch_class_with_scope(zend_string *class_name, uint32_t fetch_type, zend_class_entry *scope);
ZEND_API zend_class_entry *zend_fetch_class_by_name(zend_string *class_name, zend_string *lcname, uint32_t fetch_type);

ZEND_API zend_function * ZEND_FASTCALL zend_fetch_function(zend_string *name);
ZEND_API zend_function * ZEND_FASTCALL zend_fetch_function_str(const char *name, size_t len);
ZEND_API void ZEND_FASTCALL zend_init_func_run_time_cache(zend_op_array *op_array);

ZEND_API void zend_fetch_dimension_const(zval *result, zval *container, zval *dim, int type);

ZEND_API zval* zend_get_compiled_variable_value(const zend_execute_data *execute_data_ptr, uint32_t var);

ZEND_API bool zend_gcc_global_regs(void);

// 执行下一个操作码
#define ZEND_USER_OPCODE_CONTINUE   0 /* execute next opcode */
// 从执行器里退出
#define ZEND_USER_OPCODE_RETURN     1 /* exit from executor (return from function) */
// 调用原生操作码处理器
#define ZEND_USER_OPCODE_DISPATCH   2 /* call original opcode handler */
// 进入新操作码列表，不递归
#define ZEND_USER_OPCODE_ENTER      3 /* enter into new op_array without recursion */
// 在同一个执行器里返回调用的 操作码列表
#define ZEND_USER_OPCODE_LEAVE      4 /* return to calling op_array within the same executor */
// 调用返回操作码的原生处理器
#define ZEND_USER_OPCODE_DISPATCH_TO 0x100 /* call original handler of returned opcode */


ZEND_API int zend_set_user_opcode_handler(zend_uchar opcode, user_opcode_handler_t handler);
ZEND_API user_opcode_handler_t zend_get_user_opcode_handler(zend_uchar opcode);

ZEND_API zval *zend_get_zval_ptr(const zend_op *opline, int op_type, const znode_op *node, const zend_execute_data *execute_data);

ZEND_API void zend_clean_and_cache_symbol_table(zend_array *symbol_table);
ZEND_API void ZEND_FASTCALL zend_free_compiled_variables(zend_execute_data *execute_data);
ZEND_API void zend_unfinished_calls_gc(zend_execute_data *execute_data, zend_execute_data *call, uint32_t op_num, zend_get_gc_buffer *buf);
ZEND_API void zend_cleanup_unfinished_execution(zend_execute_data *execute_data, uint32_t op_num, uint32_t catch_op_num);
ZEND_API ZEND_ATTRIBUTE_DEPRECATED HashTable *zend_unfinished_execution_gc(zend_execute_data *execute_data, zend_execute_data *call, zend_get_gc_buffer *gc_buffer);
ZEND_API HashTable *zend_unfinished_execution_gc_ex(zend_execute_data *execute_data, zend_execute_data *call, zend_get_gc_buffer *gc_buffer, bool suspended_by_yield);

zval * ZEND_FASTCALL zend_handle_named_arg(
		zend_execute_data **call_ptr, zend_string *arg_name,
		uint32_t *arg_num_ptr, void **cache_slot);
ZEND_API int ZEND_FASTCALL zend_handle_undef_args(zend_execute_data *call);

// ing3, 通过偏移量 在运行时缓存中获取一个 (void**)
#define CACHE_ADDR(num) \
	((void**)((char*)EX(run_time_cache) + (num)))

// ing3, 通过偏移量 在运行时缓存中获取一个 (void**) 中的第一个元素
#define CACHED_PTR(num) \
	((void**)((char*)EX(run_time_cache) + (num)))[0]

// ing3, 通过偏移量 在 EX(run_time_cache) 中获取一个 (void**) 中的第一个元素，并给它赋值
#define CACHE_PTR(num, ptr) do { \
		((void**)((char*)EX(run_time_cache) + (num)))[0] = (ptr); \
	} while (0)

// ing3, 如果运行时缓存位置放的是这个类，取回下一个元素，否则null。p1:位置序号，p2:类
#define CACHED_POLYMORPHIC_PTR(num, ce) \
	/* 如果第 p1 个运行缓存数据是 p2 类 */ \
	(EXPECTED(((void**)((char*)EX(run_time_cache) + (num)))[0] == (void*)(ce)) ? \
		/* 返回下一个元素，否则null */\
		((void**)((char*)EX(run_time_cache) + (num)))[1] : \
		NULL)

// ing3, 找到运行时缓存的 p1 个位置，把p2 和p3依次保存进去
#define CACHE_POLYMORPHIC_PTR(num, ce, ptr) do { \
		/* 取得运行缓存中的位置 */ \
		void **slot = (void**)((char*)EX(run_time_cache) + (num)); \
		/* 数组第一个位置放类 */ \
		slot[0] = (ce); \
		/* 数组第二个位置放指针 */ \
		slot[1] = (ptr); \
	} while (0)

// ing4, 返回数组的第一个元素
#define CACHED_PTR_EX(slot) \
	(slot)[0]

// ing3, 把指针缓存到slot当前位置
#define CACHE_PTR_EX(slot, ptr) do { \
		(slot)[0] = (ptr); \
	} while (0)

// ing3, 如果缓存位置放的是这个类，取回下一个元素，否则null。p1:缓存位置，p2:类
#define CACHED_POLYMORPHIC_PTR_EX(slot, ce) \
	(EXPECTED((slot)[0] == (ce)) ? (slot)[1] : NULL)

// ing3, 缓存多态指针。(p1)[0] = (p2); (p1)[1] = (p3);
#define CACHE_POLYMORPHIC_PTR_EX(slot, ce, ptr) do { \
		(slot)[0] = (ce); \
		(slot)[1] = (ptr); \
	} while (0)

// 1
#define CACHE_SPECIAL (1<<0)

// ing3, 验证地址是否是特殊缓存地址（最后一位是1）
#define IS_SPECIAL_CACHE_VAL(ptr) \
	(((uintptr_t)(ptr)) & CACHE_SPECIAL)

// ing3, 创建特殊缓存序号，把序号 *2+1，再转成指针
#define ENCODE_SPECIAL_CACHE_NUM(num) \
	((void*)((((uintptr_t)(num)) << 1) | CACHE_SPECIAL))

// ing3, 解码特殊缓存序号，把序号 右移1位
#define DECODE_SPECIAL_CACHE_NUM(ptr) \
	(((uintptr_t)(ptr)) >> 1)

// ing3, 创建特殊缓存指针，把地址最后一位变成1
#define ENCODE_SPECIAL_CACHE_PTR(ptr) \
	((void*)(((uintptr_t)(ptr)) | CACHE_SPECIAL))

// ing3, 解码特殊缓存指针，把地址最后一位变成0
#define DECODE_SPECIAL_CACHE_PTR(ptr) \
	((void*)(((uintptr_t)(ptr)) & ~CACHE_SPECIAL))

// ing3, 跳过扩展操作码（ZEND_EXT_STMT 和 ZEND_TICKS 之间）
#define SKIP_EXT_OPLINE(opline) do { \
		/* 操作操作码在 ZEND_EXT_STMT 和 ZEND_TICKS 之间 */ \
		while (UNEXPECTED((opline)->opcode >= ZEND_EXT_STMT \
			&& (opline)->opcode <= ZEND_TICKS)) {     \
			/* 跳过它 */ \
			(opline)--;                                  \
		}                                                \
	} while (0)

// ing3, 验证类是否有 ZEND_ACC_HAS_TYPE_HINTS  (1 << 8) 标记
#define ZEND_CLASS_HAS_TYPE_HINTS(ce) ((ce->ce_flags & ZEND_ACC_HAS_TYPE_HINTS) == ZEND_ACC_HAS_TYPE_HINTS)

ZEND_API bool zend_verify_property_type(zend_property_info *info, zval *property, bool strict);
ZEND_COLD void zend_verify_property_type_error(zend_property_info *info, zval *property);


// ing3, 向zend_property_info_source_list指向的 zend_property_info 列表中添加 zend_property_info
// #define ZEND_REF_TYPE_SOURCES(ref) (ref)->sources
#define ZEND_REF_ADD_TYPE_SOURCE(ref, source) \
	zend_ref_add_type_source(&ZEND_REF_TYPE_SOURCES(ref), source)

// ing3, 找到并删除源指针列表中 指向属性信息的指针。p1:源列表，p2:属性信息指针
// #define ZEND_REF_TYPE_SOURCES(ref) (ref)->sources
#define ZEND_REF_DEL_TYPE_SOURCE(ref, source) \
	/* 找到并删除源指针列表中 指向属性信息的指针。p1:源列表，p2:属性信息指针 */ \
	zend_ref_del_type_source(&ZEND_REF_TYPE_SOURCES(ref), source)

// ing3, 追踪到到引用的 属性信息列表，并遍历它（开始段）
#define ZEND_REF_FOREACH_TYPE_SOURCES(ref, prop) do { \
		/* #define ZEND_REF_TYPE_SOURCES(ref) (ref)->sources */ \
		/* 追踪引用目标 */ \
		zend_property_info_source_list *_source_list = &ZEND_REF_TYPE_SOURCES(ref); \
		/* 遍历用的列表起点和终点 */ \
		zend_property_info **_prop, **_end; \
		zend_property_info_list *_list; \
		if (_source_list->ptr) { \
			/* 检验 属性信息源 是否是 列表（最后 1 位是 1） */ \
			if (ZEND_PROPERTY_INFO_SOURCE_IS_LIST(_source_list->list)) { \
				/*把 list 转成 zend_property_info_list 指针（去掉最后一位的1）*/ \
				_list = ZEND_PROPERTY_INFO_SOURCE_TO_LIST(_source_list->list); \
				/* 遍历用的列表起点和终点 */ \
				_prop = _list->ptr; \
				_end = _list->ptr + _list->num; \
			/* 不是列表也用遍历，只遍历1个元素 */ \
			} else { \
				/* 遍历用的列表起点和终点 */ \
				_prop = &_source_list->ptr; \
				_end = _prop + 1; \
			} \
			/* 遍历类型列表 */ \
			for (; _prop < _end; _prop++) { \
				/* 每个属性信息 */ \
				prop = *_prop; \


// ing4, 追踪到到引用的 属性信息列表，并遍历它（结束段）
#define ZEND_REF_FOREACH_TYPE_SOURCES_END() \
			} \
		} \
	} while (0)

ZEND_COLD void zend_match_unhandled_error(zval *value);

END_EXTERN_C()

#endif /* ZEND_EXECUTE_H */
