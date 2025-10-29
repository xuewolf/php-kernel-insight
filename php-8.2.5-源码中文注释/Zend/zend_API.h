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
   |          Andrei Zmievski <andrei@php.net>                            |
   |          Dmitry Stogov <dmitry@php.net>                              |
   +----------------------------------------------------------------------+
*/

// zend_api 是上层接口，外部调用非常多

#ifndef ZEND_API_H
#define ZEND_API_H

#include "zend_modules.h"
#include "zend_list.h"
#include "zend_operators.h"
#include "zend_variables.h"
#include "zend_execute.h"
#include "zend_type_info.h"


BEGIN_EXTERN_C()

// 函数实体，包含5个元素
typedef struct _zend_function_entry {
	// 文件名
	const char *fname;
	// 这是个抽象函数，接收一个操作码上下文和一个返回值，两个参数
	zif_handler handler;
	// 参数信息指针，可以指向一个参数信息列表
	const struct _zend_internal_arg_info *arg_info;
	// 参数个数 
	uint32_t num_args;
	// 标记
	uint32_t flags;
} zend_function_entry;

// 函数调用信息
typedef struct _zend_fcall_info {
	size_t size;
	// 函数名
	zval function_name;
	// 返回值指针
	zval *retval;
	// 参数列表指针
	zval *params;
	// 对象指针
	zend_object *object;
	// 参数数量
	uint32_t param_count;
	// 这个哈希表也可以用来存放位置序列参数，它们会被添加到普通的params[]j里。
	// 
	/* This hashtable can also contain positional arguments (with integer keys),
	 * which will be appended to the normal params[]. This makes it easier to
	 * integrate APIs like call_user_func_array(). The usual restriction that
	 * there may not be position arguments after named arguments applies. */
	 
	// 命名参数列表 指针
	HashTable *named_params;
} zend_fcall_info;

// 调用信息缓存 
typedef struct _zend_fcall_info_cache {
	// 函数指针
	zend_function *function_handler;
	// 所属类
	zend_class_entry *calling_scope;
	// 正在调用的域
	zend_class_entry *called_scope;
	// 
	zend_object *object;
} zend_fcall_info_cache;

// ing4, 命名空间名字，传入前缀和名字，2个参数
#define ZEND_NS_NAME(ns, name)			ns "\\" name

// 下面这堆和函数  和 类方法定义相关，在扩展里大量使用。
// ZEND_FN/ZEND_MN 在这里定义，防止预扫描宏膨胀。如果函数名也是宏名的话，会出错。
/* ZEND_FN/ZEND_MN are inlined below to prevent pre-scan macro expansion,
 * which causes issues if the function name is also a macro name. */
// ing3, 加前缀，得到内置函数名 
#define ZEND_FN(name) zif_##name
// ing3, 加前缀，得到内置方法名
#define ZEND_MN(name) zim_##name

// ing3, 定义 zif 函数 ，这种函数只有 执行上下文 和 返回值zval 2个参数
#define ZEND_NAMED_FUNCTION(name)		void ZEND_FASTCALL name(INTERNAL_FUNCTION_PARAMETERS)
// ing3, 定义内置函数 
#define ZEND_FUNCTION(name)				ZEND_NAMED_FUNCTION(zif_##name)
// ing3, 定义内置方法
#define ZEND_METHOD(classname, name)	ZEND_NAMED_FUNCTION(zim_##classname##_##name)

// ing3, 入口， 用来创建 _zend_function_entry 实例的，
//  与下面不同的地方在于 #zend_name 这个是变量名，写什么样就是什么样，而不是变量的值
#define ZEND_FENTRY(zend_name, name, arg_info, flags)	{ #zend_name, name, arg_info, (uint32_t) (sizeof(arg_info)/sizeof(struct _zend_internal_arg_info)-1), flags },

// ing3, 原生入口，这个是用来创建 _zend_function_entry 实例的，就是 函数或成员方法。
// 每个zend_function 有5个元素：【函数名 ，对内的内置函数名，参数信息表指针，参数数量，flags】
#define ZEND_RAW_FENTRY(zend_name, name, arg_info, flags)   { zend_name, name, arg_info, (uint32_t) (sizeof(arg_info)/sizeof(struct _zend_internal_arg_info)-1), flags },
// 问题是这个除法是怎么算的

// 与 ZEND_NAMED_FE 相同
/* Same as ZEND_NAMED_FE */
// ing3, 创建函数，传入函数名，对应的内置C函数， 参数信息 3个参数
#define ZEND_RAW_NAMED_FE(zend_name, name, arg_info) ZEND_RAW_FENTRY(#zend_name, name, arg_info, 0)
// ing3, 和上面相同
#define ZEND_NAMED_FE(zend_name, name, arg_info)	ZEND_RAW_FENTRY(#zend_name, name, arg_info, 0)
// ing3, 创建函数，传入函数名（自动生成内置C函数名）， 参数信息 2个参数
#define ZEND_FE(name, arg_info)						ZEND_RAW_FENTRY(#name, zif_##name, arg_info, 0)
// ing3, 创建弃用的函数，传入函数名（自动生成内置C函数名）， 参数信息 2个参数
#define ZEND_DEP_FE(name, arg_info)                 ZEND_RAW_FENTRY(#name, zif_##name, arg_info, ZEND_ACC_DEPRECATED)
// ing3, 创建函数，传入函数名，c函数别名， 参数信息 3个参数
#define ZEND_FALIAS(name, alias, arg_info)			ZEND_RAW_FENTRY(#name, zif_##alias, arg_info, 0)
// ing3, 创建弃用的函数，传入函数名，c函数别名， 参数信息 3个参数
#define ZEND_DEP_FALIAS(name, alias, arg_info)		ZEND_RAW_FENTRY(#name, zif_##alias, arg_info, ZEND_ACC_DEPRECATED)
// ing3, 创建函数，传入函数名，对应的内置C函数， 参数信息，标识 4个参数。 这里的zend_name 用变量名。
#define ZEND_NAMED_ME(zend_name, name, arg_info, flags)	ZEND_FENTRY(zend_name, name, arg_info, flags)

// 通过这个也能明白 类方法是如何定义的
// ing3, 创建类方法，传入类名，方法名（自动生成内置C函数名）， 参数信息，标识 4个参数
#define ZEND_ME(classname, name, arg_info, flags)	ZEND_RAW_FENTRY(#name, zim_##classname##_##name, arg_info, flags)
// ing3, 创建弃用的类方法，传入类名，方法名（自动生成内置C函数名）， 参数信息，标识 4个参数
#define ZEND_DEP_ME(classname, name, arg_info, flags) ZEND_RAW_FENTRY(#name, zim_##classname##_##name, arg_info, flags | ZEND_ACC_DEPRECATED)
// ing3, 创建抽象类方法，传入类名，方法名（对应的内置C函数名是NULL），参数信息 参数信息 3个参数
#define ZEND_ABSTRACT_ME(classname, name, arg_info)	ZEND_RAW_FENTRY(#name, NULL, arg_info, ZEND_ACC_PUBLIC|ZEND_ACC_ABSTRACT)

// ing3, 创建抽象类方法，传入类名，方法名（对应的内置C函数名是NULL），参数信息，标识 参数信息 4个参数
#define ZEND_ABSTRACT_ME_WITH_FLAGS(classname, name, arg_info, flags)	ZEND_RAW_FENTRY(#name, NULL, arg_info, flags)

// ing3, 创建类方法，传入类名，方法别名（自动生成方法名），参数信息，标识 参数信息 4个参数
#define ZEND_MALIAS(classname, name, alias, arg_info, flags) ZEND_RAW_FENTRY(#name, zim_##classname##_##alias, arg_info, flags)
// ing3, 创建函数，传入函数名，对应的内置C函数， 参数信息，标识 4个参数。 
#define ZEND_ME_MAPPING(name, func_name, arg_info, flags) ZEND_RAW_FENTRY(#name, zif_##func_name, arg_info, flags)
// ing3, 创建函数，传入命名空间，函数名（使用变量名），对应的内置C函数， 参数信息，标识 5个参数。 
#define ZEND_NS_FENTRY(ns, zend_name, name, arg_info, flags)		ZEND_RAW_FENTRY(ZEND_NS_NAME(ns, #zend_name), name, arg_info, flags)
// ing3, 创建函数。p1:命名空间，p2:函数名，p3:对应的内置C函数，p4:参数信息，p5:flags
#define ZEND_NS_RAW_FENTRY(ns, zend_name, name, arg_info, flags)	ZEND_RAW_FENTRY(ZEND_NS_NAME(ns, zend_name), name, arg_info, flags)


/**
 如果断言一个函数是编译时可计算的，需要断言：
 * Note that if you are asserting that a function is compile-time evaluable, you are asserting that
 *
 函数收到同样参数时必须返回同样结果 
 * 1. The function will always have the same result for the same arguments
 函数不依赖全局状态 例如 ini设置 或本地 (例如 mb_strtolower)，number_format()等
 * 2. The function does not depend on global state such as ini settings or locale (e.g. mb_strtolower), number_format(), etc.
 函数没有其他影响，如果参数不对，抛出错误和警告是可以的，当侦测到错误或警告，会抛弃运行结果 。
 * 3. The function does not have side effects. It is okay if they throw
 *    or warn on invalid arguments, as we detect this and will discard the evaluation result.
 函数运行时，不会使用不合理的时间或内存
 * 4. The function will not take an unreasonable amount of time or memory to compute on code that may be seen in practice.
 *    (e.g. str_repeat is special cased to check the length instead of using this)
 */
// ing3, 支持编译时运行的函数 ，传入函数名和参数信息，自动生成处理器名和 添加ZEND_ACC_COMPILE_TIME_EVAL标记。
#define ZEND_SUPPORTS_COMPILE_TIME_EVAL_FE(name, arg_info) ZEND_RAW_FENTRY(#name, zif_##name, arg_info, ZEND_ACC_COMPILE_TIME_EVAL)

// 和 ZEND_NS_NAMED_FE 一样
/* Same as ZEND_NS_NAMED_FE */

// ing3, 创建函数，无flags。p1:命名空间，p2:函数名，p3:对应的内置C函数，p4:参数信息
#define ZEND_NS_RAW_NAMED_FE(ns, zend_name, name, arg_info)			ZEND_NS_RAW_FENTRY(ns, #zend_name, name, arg_info, 0)

// ing3, 创建函数，无flags。p1:命名空间，p2:函数名，p3:对应的内置C函数，p4:参数信息
#define ZEND_NS_NAMED_FE(ns, zend_name, name, arg_info)	ZEND_NS_RAW_FENTRY(ns, #zend_name, name, arg_info, 0)

// ing3, 创建函数指向c函数zif_##函数名，无flags。p1:命名空间，p2:函数名，p3:参数信息
#define ZEND_NS_FE(ns, name, arg_info)					ZEND_NS_RAW_FENTRY(ns, #name, zif_##name, arg_info, 0)
// ing3, 创建函数指向c函数zif_##函数名，带 ZEND_ACC_DEPRECATED 标记。p1:命名空间，p2:函数名，p3:参数信息
#define ZEND_NS_DEP_FE(ns, name, arg_info)				ZEND_NS_RAW_FENTRY(ns, #name, zif_##name, arg_info, ZEND_ACC_DEPRECATED)

// ing3, 创建函数指向c函数zif_##别名，无flags。p1:命名空间，p2:函数名，p3:别名，p4:参数信息
#define ZEND_NS_FALIAS(ns, name, alias, arg_info)		ZEND_NS_RAW_FENTRY(ns, #name, zif_##alias, arg_info, 0)
// ing3, 创建函数指向c函数zif_##别名，带 ZEND_ACC_DEPRECATED 标记。p1:命名空间，p2:函数名，p3:别名，p4:参数信息
#define ZEND_NS_DEP_FALIAS(ns, name, alias, arg_info)	ZEND_NS_RAW_FENTRY(ns, #name, zif_##alias, arg_info, ZEND_ACC_DEPRECATED)

#define ZEND_FE_END            { NULL, NULL, NULL, 0, 0 }

// ing3,  返回形式参数标记，3个标记分别是：是否是引用返回，是否是字典，返回值类型是否兼容null
#define _ZEND_ARG_INFO_FLAGS(pass_by_ref, is_variadic, is_tentative) \
	(((pass_by_ref) << _ZEND_SEND_MODE_SHIFT) | ((is_variadic) ? _ZEND_IS_VARIADIC_BIT : 0) | ((is_tentative) ? _ZEND_IS_TENTATIVE_BIT : 0))

// 这里对应的结构体是 zend_internal_arg_info {	const char *name;	zend_type type;	const char *default_value; }
// 没有type信息的参数结构
/* Arginfo structures without type information */
// ing3, 参数信息，（不支持字典，返回类型不允null），无默认值。传入：可否引用传递，参数名
#define ZEND_ARG_INFO(pass_by_ref, name) \
	{ #name, ZEND_TYPE_INIT_NONE(_ZEND_ARG_INFO_FLAGS(pass_by_ref, 0, 0)), NULL },
	
// ing3, 参数信息，（不支持字典，返回类型不允null）。传入：可否引用传递，参数名，默认
#define ZEND_ARG_INFO_WITH_DEFAULT_VALUE(pass_by_ref, name, default_value) \
	{ #name, ZEND_TYPE_INIT_NONE(_ZEND_ARG_INFO_FLAGS(pass_by_ref, 0, 0)), default_value },
	
// ing3, 参数信息，（支持字典，返回类型不允null），无默认值。传入：可否引用传递，参数名
#define ZEND_ARG_VARIADIC_INFO(pass_by_ref, name) \
	{ #name, ZEND_TYPE_INIT_NONE(_ZEND_ARG_INFO_FLAGS(pass_by_ref, 1, 0)), NULL },

// 带有简单类型信息的 参数信息 结构 
/* Arginfo structures with simple type information */
// ing3, 无默认值的 参数信息：可引用传递，参数名称，类型，可以是null
#define ZEND_ARG_TYPE_INFO(pass_by_ref, name, type_hint, allow_null) \
	{ #name, ZEND_TYPE_INIT_CODE(type_hint, allow_null, _ZEND_ARG_INFO_FLAGS(pass_by_ref, 0, 0)), NULL },
// ing3, 有默认值的 参数信息：可引用传递，参数名称，类型，可以是null, 默认值
#define ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(pass_by_ref, name, type_hint, allow_null, default_value) \
	{ #name, ZEND_TYPE_INIT_CODE(type_hint, allow_null, _ZEND_ARG_INFO_FLAGS(pass_by_ref, 0, 0)), default_value },
// ing3, 无默认值的 字典参数信息：可引用传递，参数名称，类型，可以是null
#define ZEND_ARG_VARIADIC_TYPE_INFO(pass_by_ref, name, type_hint, allow_null) \
	{ #name, ZEND_TYPE_INIT_CODE(type_hint, allow_null, _ZEND_ARG_INFO_FLAGS(pass_by_ref, 1, 0)), NULL },

// 带有复杂类型信息的 参数信息 结构 
/* Arginfo structures with complex type information */
// ing3, 有默认值、需要type_mask的 参数信息，可引用传递，参数名称，type_mask，默认值
#define ZEND_ARG_TYPE_MASK(pass_by_ref, name, type_mask, default_value) \
	{ #name, ZEND_TYPE_INIT_MASK(type_mask | _ZEND_ARG_INFO_FLAGS(pass_by_ref, 0, 0)), default_value },

// ing3, 有默认值、需要type_mask的 参数信息，可引用传递，参数名称，类型名称，type_mask，默认值
#define ZEND_ARG_OBJ_TYPE_MASK(pass_by_ref, name, class_name, type_mask, default_value) \
	{ #name, ZEND_TYPE_INIT_CLASS_CONST_MASK(#class_name, type_mask | _ZEND_ARG_INFO_FLAGS(pass_by_ref, 0, 0)), default_value },

// ing3, 无默认值、需要type_mask的 参数信息，可引用传递，参数名称，类型名称，type_mask
#define ZEND_ARG_VARIADIC_OBJ_TYPE_MASK(pass_by_ref, name, class_name, type_mask) \
	{ #name, ZEND_TYPE_INIT_CLASS_CONST_MASK(#class_name, type_mask | _ZEND_ARG_INFO_FLAGS(pass_by_ref, 1, 0)), NULL },

// 带有类型信息的 参数信息 结构 
/* Arginfo structures with object type information */

// ing3, 定义对象返回值。参数：可否引用传递, 参数名，类名，是否允null
#define ZEND_ARG_OBJ_INFO(pass_by_ref, name, class_name, allow_null) \
	/* 创建 有类名类型，带类入口，自定义允null，pass_by_ref, extra_flags。*/ \
	{ #name, ZEND_TYPE_INIT_CLASS_CONST(#class_name, allow_null, _ZEND_ARG_INFO_FLAGS(pass_by_ref, 0, 0)), NULL },

// ing3, 定义有默认值的对象参数。参数：可否引用传递,参数名，类名，是否允null，默认值
#define ZEND_ARG_OBJ_INFO_WITH_DEFAULT_VALUE(pass_by_ref, name, class_name, allow_null, default_value) \
	/* 创建 有类名类型，带类入口，自定义允null，pass_by_ref, extra_flags */ \
	{ #name, ZEND_TYPE_INIT_CLASS_CONST(#class_name, allow_null, _ZEND_ARG_INFO_FLAGS(pass_by_ref, 0, 0)), default_value },

// ing3, 定义字典型对象参数。参数：可否引用传递,参数名，类名，是否允null
#define ZEND_ARG_VARIADIC_OBJ_INFO(pass_by_ref, name, class_name, allow_null) \
	/* 创建 有类名类型，带类入口，自定义允null，pass_by_ref, extra_flags */ \
	{ #name, ZEND_TYPE_INIT_CLASS_CONST(#class_name, allow_null, _ZEND_ARG_INFO_FLAGS(pass_by_ref, 1, 0)), NULL },

// 遗产参数信息结构
/* Legacy arginfo structures */

// ing3, 定义数组类型 type_mask ：p1:可否引用传递，p2:类型名，p3:允null
#define ZEND_ARG_ARRAY_INFO(pass_by_ref, name, allow_null) \
	/* 定义类型 type_mask ：p1:允许的类型，p2:允null，p3:额外标记 */ \
	{ #name, ZEND_TYPE_INIT_CODE(IS_ARRAY, allow_null, _ZEND_ARG_INFO_FLAGS(pass_by_ref, 0, 0)), NULL },
	
// ing3, 定义 IS_CALLABLE 类型 type_mask ：p1:可否引用传递，p2:类型名，p3:允null
#define ZEND_ARG_CALLABLE_INFO(pass_by_ref, name, allow_null) \
	/* 定义类型 type_mask ：p1:允许的类型，p2:允null，p3:额外标记 */ \
	{ #name, ZEND_TYPE_INIT_CODE(IS_CALLABLE, allow_null, _ZEND_ARG_INFO_FLAGS(pass_by_ref, 0, 0)), NULL },

// ing3, 创建参数信息列表 ，增加第一个参数（返回值）。
// 传入参数名，是否引用返回，必要参数数量 ，返回类型，是否允null, 返回类型是否允null
// allow_null 和 is_tentative_return_type 如何分工 ？
#define ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX2(name, return_reference, required_num_args, class_name, allow_null, is_tentative_return_type) \
	/* 参数列表 */ \
	static const zend_internal_arg_info name[] = { \
		{ (const char*)(zend_uintptr_t)(required_num_args), \
			/* 创建 有类名类型，带类入口，自定义允null, extra_flags */ \
			ZEND_TYPE_INIT_CLASS_CONST(#class_name, allow_null, _ZEND_ARG_INFO_FLAGS(return_reference, 0, is_tentative_return_type)), NULL },

// ing3, 创建参数信息列表 ，增加第一个参数（返回值）：返回类型不允null。
// 传入参数名，是否引用返回，必要参数数，返回类型，是否允null
#define ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(name, return_reference, required_num_args, class_name, allow_null) \
	ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX2(name, return_reference, required_num_args, class_name, allow_null, 0)

// ing3, 创建参数信息列表 ，增加第一个参数（返回值）：返回类型允null。
// 传入参数名，是否引用返回，必要参数数，返回类型，是否允null
#define ZEND_BEGIN_ARG_WITH_TENTATIVE_RETURN_OBJ_INFO_EX(name, return_reference, required_num_args, class_name, allow_null) \
	ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX2(name, return_reference, required_num_args, class_name, allow_null, 1)


// ing3, 创建参数信息列表 ，增加第一个参数（返回值）：不可引用返回，必要参数数量 -1，返回类型不允null。
// 传入参数名，返回类型，是否允null
#define ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO(name, class_name, allow_null) \
	ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX2(name, 0, -1, class_name, allow_null, 0)

// ing3, 创建参数信息列表 ，增加第一个参数（返回值）：不可以是字典参数，无默认值
// 传入参数名，是否引用返回，必要参数数量，type_mask(聚合类型)，返回类型是否允null
#define ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX2(name, return_reference, required_num_args, type, is_tentative_return_type) \
	static const zend_internal_arg_info name[] = { \
		{ (const char*)(zend_uintptr_t)(required_num_args), ZEND_TYPE_INIT_MASK(type | _ZEND_ARG_INFO_FLAGS(return_reference, 0, is_tentative_return_type)), NULL },

// ing3, 创建参数信息列表 ，增加第一个参数（返回值）：不可以是字典参数，无默认值，返回类型不允null
// 传入参数名，是否引用返回，必要参数数量，type_mask(聚合类型)
#define ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(name, return_reference, required_num_args, type) \
	ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX2(name, return_reference, required_num_args, type, 0)

// ing3, 创建参数信息列表 ，增加第一个参数（返回值）：不可以是字典参数，无默认值，返回类型允null
// 传入参数名，是否引用返回，必要参数数量，type_mask(聚合类型)
#define ZEND_BEGIN_ARG_WITH_TENTATIVE_RETURN_TYPE_MASK_EX(name, return_reference, required_num_args, type) \
	ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX2(name, return_reference, required_num_args, type, 1)

// ing3, 创建参数信息列表 ，增加第一个参数（返回值）：不可以是字典参数，无默认值
// 传入参数名，是否引用返回，必要参数数量，类名，type_mask(聚合类型)，返回类型是否允null
#define ZEND_BEGIN_ARG_WITH_RETURN_OBJ_TYPE_MASK_EX2(name, return_reference, required_num_args, class_name, type, is_tentative_return_type) \
	static const zend_internal_arg_info name[] = { \
		{ (const char*)(zend_uintptr_t)(required_num_args), ZEND_TYPE_INIT_CLASS_CONST_MASK(#class_name, type | _ZEND_ARG_INFO_FLAGS(return_reference, 0, is_tentative_return_type)), NULL },

// ing3, 创建参数信息列表 ，增加第一个参数（返回值）：不可以是字典参数，无默认值，返回类型不允null
// 传入参数名，是否引用返回，必要参数数量，类名，type_mask(聚合类型)
#define ZEND_BEGIN_ARG_WITH_RETURN_OBJ_TYPE_MASK_EX(name, return_reference, required_num_args, class_name, type) \
	ZEND_BEGIN_ARG_WITH_RETURN_OBJ_TYPE_MASK_EX2(name, return_reference, required_num_args, class_name, type, 0)

// ing3, 创建参数信息列表 ，增加第一个参数（返回值）：不可以是字典参数，无默认值，返回类型允null
// 传入参数名，是否引用返回，必要参数数量，类名，type_mask(聚合类型)
#define ZEND_BEGIN_ARG_WITH_TENTATIVE_RETURN_OBJ_TYPE_MASK_EX(name, return_reference, required_num_args, class_name, type) \
	ZEND_BEGIN_ARG_WITH_RETURN_OBJ_TYPE_MASK_EX2(name, return_reference, required_num_args, class_name, type, 1)

// ing3, 创建参数信息列表 ，增加第一个参数（返回值）。
// 传入：变量名，是否引用返回，必要的参数数，类型，是否允null，。。。
#define ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX2(name, return_reference, required_num_args, type, allow_null, is_tentative_return_type) \
	/* 声名内部参数信息列表 */ \
	static const zend_internal_arg_info name[] = { \
		/* 第一个参数信息： {参数名， ZEND_TYPE_INIT_CODE创建一个类型，默认值 } */ \
		{ (const char*)(zend_uintptr_t)(required_num_args), ZEND_TYPE_INIT_CODE(type, allow_null, _ZEND_ARG_INFO_FLAGS(return_reference, 0, is_tentative_return_type)), NULL },

// ing3, 创建 参数信息列表 ，增加第一个 返回类型不允null 的 参数（返回值）。（大量引用）
// 传入：变量名，是否引用返回，必要的参数数，类型，是否允null
#define ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(name, return_reference, required_num_args, type, allow_null) \
	ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX2(name, return_reference, required_num_args, type, allow_null, 0)

// ing3, 创建 参数信息列表 ，增加第一个 返回类型允null 的 参数（返回值）。（大量引用）
// 传入：变量名，是否引用返回，必要的参数数，类型，是否允null
#define ZEND_BEGIN_ARG_WITH_TENTATIVE_RETURN_TYPE_INFO_EX(name, return_reference, required_num_args, type, allow_null) \
	ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX2(name, return_reference, required_num_args, type, allow_null, 1)

// ing3, 创建 参数信息列表 ，增加第一个 返回类型不允null，非引用返回，必要参数数量为0 的 参数（返回值）。
// 传入：变量名，类型，是否允null
#define ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO(name, type, allow_null) \
	ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX2(name, 0, -1, type, allow_null, 0)

// ing2, 创建参数信息列表 ，增加第一个参数（返回值）。传入：变量名，_unused？，是否引用返回，必要的参数数
#define ZEND_BEGIN_ARG_INFO_EX(name, _unused, return_reference, required_num_args)	\
	/* 声名内部参数信息列表 */ \
	static const zend_internal_arg_info name[] = { \
		/* 第一个参数信息： {参数名， ZEND_TYPE_INIT_CODE创建一个类型，默认值 } */ \
		{ (const char*)(zend_uintptr_t)(required_num_args), ZEND_TYPE_INIT_NONE(_ZEND_ARG_INFO_FLAGS(return_reference, 0, 0)), NULL },
		
// ing3, 创建参数信息列表 ，增加第一个参数（返回值）。
// 传入：变量名，_unused？
#define ZEND_BEGIN_ARG_INFO(name, _unused)	\
	ZEND_BEGIN_ARG_INFO_EX(name, {}, ZEND_RETURN_VALUE, -1)

// ing4, 参数列表结尾大括号
#define ZEND_END_ARG_INFO()		};

// 名称宏
/* Name macros */
// ing3, 模块的【启动】方法名
#define ZEND_MODULE_STARTUP_N(module)       zm_startup_##module
// ing3, 模块的【关闭】方法名
#define ZEND_MODULE_SHUTDOWN_N(module)		zm_shutdown_##module
// ing3, 模块的【激活】方法名
#define ZEND_MODULE_ACTIVATE_N(module)		zm_activate_##module
// ing3, 模块的【反激活】方法名
#define ZEND_MODULE_DEACTIVATE_N(module)	zm_deactivate_##module
// ing3, 模块的【发送激活】方法名
#define ZEND_MODULE_POST_ZEND_DEACTIVATE_N(module)	zm_post_zend_deactivate_##module
// ing3, 模块的【获取信息】方法名
#define ZEND_MODULE_INFO_N(module)			zm_info_##module
// ing3, 模块的【全局构造器】方法名
#define ZEND_MODULE_GLOBALS_CTOR_N(module)  zm_globals_ctor_##module
// ing3, 模块的【全局销毁器】方法名
#define ZEND_MODULE_GLOBALS_DTOR_N(module)  zm_globals_dtor_##module

// 声名宏
/* Declaration macros */
// ing3, 模块的【启动】方法
#define ZEND_MODULE_STARTUP_D(module)		zend_result ZEND_MODULE_STARTUP_N(module)(INIT_FUNC_ARGS)
// ing3, 模块的【关闭】方法
#define ZEND_MODULE_SHUTDOWN_D(module)		zend_result ZEND_MODULE_SHUTDOWN_N(module)(SHUTDOWN_FUNC_ARGS)
// ing3, 模块的【激活】方法
#define ZEND_MODULE_ACTIVATE_D(module)		zend_result ZEND_MODULE_ACTIVATE_N(module)(INIT_FUNC_ARGS)
// ing3, 模块的【反激活】方法
#define ZEND_MODULE_DEACTIVATE_D(module)	zend_result ZEND_MODULE_DEACTIVATE_N(module)(SHUTDOWN_FUNC_ARGS)
// ing3, 模块的【发送激活】方法
#define ZEND_MODULE_POST_ZEND_DEACTIVATE_D(module)	zend_result ZEND_MODULE_POST_ZEND_DEACTIVATE_N(module)(void)
// ing3, 模块的【获取信息】方法
#define ZEND_MODULE_INFO_D(module)			ZEND_COLD void ZEND_MODULE_INFO_N(module)(ZEND_MODULE_INFO_FUNC_ARGS)
// ing3, 模块的【全局构造器】方法
#define ZEND_MODULE_GLOBALS_CTOR_D(module)  void ZEND_MODULE_GLOBALS_CTOR_N(module)(zend_##module##_globals *module##_globals)
// ing3, 模块的【全局销毁器】方法
#define ZEND_MODULE_GLOBALS_DTOR_D(module)  void ZEND_MODULE_GLOBALS_DTOR_N(module)(zend_##module##_globals *module##_globals)

// ing3, 声名 【返回 此名称的模块】 的函数
#define ZEND_GET_MODULE(name) \
    BEGIN_EXTERN_C()\
	ZEND_DLEXPORT zend_module_entry *get_module(void) { return &name##_module_entry; }\
    END_EXTERN_C()

// ing3, 声名此模块全局变量表结构体，并定义成类型
#define ZEND_BEGIN_MODULE_GLOBALS(module_name)		\
	typedef struct _zend_##module_name##_globals {

// ing3, 声名此模块全局变量表结构体，并定义成类型（结束）
#define ZEND_END_MODULE_GLOBALS(module_name)		\
	} zend_##module_name##_globals;
// 如果要求线程安全
#ifdef ZTS

#define ZEND_DECLARE_MODULE_GLOBALS(module_name)							\
	ts_rsrc_id module_name##_globals_id;
#define ZEND_EXTERN_MODULE_GLOBALS(module_name)								\
	extern ts_rsrc_id module_name##_globals_id;
#define ZEND_INIT_MODULE_GLOBALS(module_name, globals_ctor, globals_dtor)	\
	ts_allocate_id(&module_name##_globals_id, sizeof(zend_##module_name##_globals), (ts_allocate_ctor) globals_ctor, (ts_allocate_dtor) globals_dtor);
#define ZEND_MODULE_GLOBALS_ACCESSOR(module_name, v) ZEND_TSRMG(module_name##_globals_id, zend_##module_name##_globals *, v)
#ifdef ZEND_ENABLE_STATIC_TSRMLS_CACHE
#define ZEND_MODULE_GLOBALS_BULK(module_name) TSRMG_BULK_STATIC(module_name##_globals_id, zend_##module_name##_globals *)
#else
#define ZEND_MODULE_GLOBALS_BULK(module_name) TSRMG_BULK(module_name##_globals_id, zend_##module_name##_globals *)
#endif

// 不需要线程安全
#else

// ing3, 声名此模块的全局变量表 实例
#define ZEND_DECLARE_MODULE_GLOBALS(module_name)							\
	zend_##module_name##_globals module_name##_globals;
// ing1, 声名此模块的全局变量表 实例. 带 extern
#define ZEND_EXTERN_MODULE_GLOBALS(module_name)								\
	extern zend_##module_name##_globals module_name##_globals;

// ing3, 调用模块的全局变量表 构造器。p1:模块名，p2:全局变量表构造器，p3:全局变量表销毁器
#define ZEND_INIT_MODULE_GLOBALS(module_name, globals_ctor, globals_dtor)	\
	globals_ctor(&module_name##_globals);

// ing3, 访问此模块的全局变量表中的 变量	
#define ZEND_MODULE_GLOBALS_ACCESSOR(module_name, v) (module_name##_globals.v)

// ing3, 返回此模块的 全局变量表结构体 的地址
#define ZEND_MODULE_GLOBALS_BULK(module_name) (&module_name##_globals)

#endif

// ing4, 用给定的名字创建新类，并添加成员方法列表。p1:接收变量，p2:类名，p3:方法表
#define INIT_CLASS_ENTRY(class_container, class_name, functions) \
	INIT_CLASS_ENTRY_EX(class_container, class_name, sizeof(class_name)-1, functions)

// ing4, 用给定的名字创建新类，并添加成员方法列表。p1:接收变量，p2:类名，p3:类名长度，p4:方法表
#define INIT_CLASS_ENTRY_EX(class_container, class_name, class_name_len, functions) \
	{															\
		/* 先复制zend_class_entry结构体到  class_container */ \
		memset(&class_container, 0, sizeof(zend_class_entry)); \
		/* 用类名创建保留字串 */ \
		class_container.name = zend_string_init_interned(class_name, class_name_len, 1); \
		/* 给类添加成员方法列表 */ \
		class_container.info.internal.builtin_functions = functions;	\
	}

// ing3, 初始化类方法（构造方法，魔术方法等）
#define INIT_CLASS_ENTRY_INIT_METHODS(class_container, functions) \
	{															\
		/* 构造方法 */ \
		class_container.constructor = NULL;						\
		/* 析构方法 */ \
		class_container.destructor = NULL;						\
		/* 克隆方法 */ \
		class_container.clone = NULL;							\
		/* 序列化方法 */ \
		class_container.serialize = NULL;						\
		/* 反序列化方法 */ \
		class_container.unserialize = NULL;						\
		/* 实例化方法 */ \
		class_container.create_object = NULL;					\
		/* ？？ */ \
		class_container.get_static_method = NULL;				\
		/* 常用魔术方法 */ \
		class_container.__call = NULL;							\
		class_container.__callstatic = NULL;					\
		class_container.__tostring = NULL;						\
		class_container.__get = NULL;							\
		class_container.__set = NULL;							\
		class_container.__unset = NULL;							\
		class_container.__isset = NULL;							\
		class_container.__debugInfo = NULL;						\
		class_container.__serialize = NULL;						\
		class_container.__unserialize = NULL;					\
		/* 父类 */ \
		class_container.parent = NULL;							\
		/* 实现接口数量 */ \
		class_container.num_interfaces = 0;						\
		/* 引用trait名字列表 */ \
		class_container.trait_names = NULL;						\
		/* 引用trait数量 */ \
		class_container.num_traits = 0;							\
		/* trait别名 */ \
		class_container.trait_aliases = NULL;					\
		/* trait优先顺序 */ \
		class_container.trait_precedences = NULL;				\
		/* 接口名列表 */ \
		class_container.interfaces = NULL;						\
		/* 返回迭代器 ？ */ \
		class_container.get_iterator = NULL;					\
		/* 迭代函数指针 */ \
		class_container.iterator_funcs_ptr = NULL;				\
		/* 像数组一样访问的函数指针 */ \
		class_container.arrayaccess_funcs_ptr = NULL;			\
		/* 所属模块 */ \
		class_container.info.internal.module = NULL;			\
		/* 添加的内置函数列表 */ \
		class_container.info.internal.builtin_functions = functions;	\
	}


// ing4, 用给定的名字创建新类，并添加成员方法列表。p1:接收变量，p2:命名空间，p3:类名，p4:方法表
#define INIT_NS_CLASS_ENTRY(class_container, ns, class_name, functions) \
	/* 用给定的名字创建新类，并添加成员方法列表。p1:接收变量，p2:类名，p3:方法表 */ \
	INIT_CLASS_ENTRY(class_container, ZEND_NS_NAME(ns, class_name), functions)

// ing4, 返回静态成员表【指针列表】
#define CE_STATIC_MEMBERS(ce) \
	((zval*)ZEND_MAP_PTR_GET((ce)->static_members_table))

// ing4, 返回类的常量列表
#define CE_CONSTANTS_TABLE(ce) \
	zend_class_constants_table(ce)

// ing4, 返回 ce->mutable_data 或者 ce本身的 默认属性列表
#define CE_DEFAULT_PROPERTIES_TABLE(ce) \
	zend_class_default_properties_table(ce)

// ing4, 返回 ce->mutable_data 或者 ce本身的 enum备用表
#define CE_BACKED_ENUM_TABLE(ce) \
	zend_class_backed_enum_table(ce)

// ing3, 检查调用信息是否初始化过
#define ZEND_FCI_INITIALIZED(fci) ((fci).size != 0)

ZEND_API int zend_next_free_module(void);

BEGIN_EXTERN_C()
ZEND_API zend_result zend_get_parameters_array_ex(uint32_t param_count, zval *argument_array);

/* internal function to efficiently copy parameters when executing __call() */
ZEND_API zend_result zend_copy_parameters_array(uint32_t param_count, zval *argument_array);

// ing3, 把形式参数添加到实际参数列表（原生数组）中去。p1:没用到，p2:形式参数数量，p3:实际参数列表
#define zend_get_parameters_array(ht, param_count, argument_array) \
	zend_get_parameters_array_ex(param_count, argument_array)
	
// ing4, 不可以有参数，否则报错
#define zend_parse_parameters_none() \
	(EXPECTED(ZEND_NUM_ARGS() == 0) ? SUCCESS : (zend_wrong_parameters_none_error(), FAILURE))

// ing4, 不可以有参数，否则报错
#define zend_parse_parameters_none_throw() \
	zend_parse_parameters_none()

/* Parameter parsing API -- andrei */

#define ZEND_PARSE_PARAMS_THROW 0 /* No longer used, zpp always uses exceptions */
#define ZEND_PARSE_PARAMS_QUIET (1<<1)
// 
ZEND_API zend_result zend_parse_parameters(uint32_t num_args, const char *type_spec, ...);
ZEND_API zend_result zend_parse_parameters_ex(int flags, uint32_t num_args, const char *type_spec, ...);

// 提示：这里必须有至少1个值在 __VA_ARGS__ 否则 表达式会不可用
/* NOTE: This must have at least one value in __VA_ARGS__ for the expression to be valid */
// ing3, 解析传入的参数（包含可变参数），参数格式在__VA_ARGS__里。 p1:参数数量，p+:参数列表
#define zend_parse_parameters_throw(num_args, ...) \
	zend_parse_parameters(num_args, __VA_ARGS__)
	
ZEND_API const char *zend_zval_type_name(const zval *arg);
ZEND_API zend_string *zend_zval_get_legacy_type(const zval *arg);

ZEND_API zend_result zend_parse_method_parameters(uint32_t num_args, zval *this_ptr, const char *type_spec, ...);
ZEND_API zend_result zend_parse_method_parameters_ex(int flags, uint32_t num_args, zval *this_ptr, const char *type_spec, ...);

ZEND_API zend_result zend_parse_parameter(int flags, uint32_t arg_num, zval *arg, const char *spec, ...);

/* End of parameter parsing API -- andrei */

ZEND_API zend_result zend_register_functions(zend_class_entry *scope, const zend_function_entry *functions, HashTable *function_table, int type);
ZEND_API void zend_unregister_functions(const zend_function_entry *functions, int count, HashTable *function_table);
ZEND_API zend_result zend_startup_module(zend_module_entry *module_entry);
ZEND_API zend_module_entry* zend_register_internal_module(zend_module_entry *module_entry);
ZEND_API zend_module_entry* zend_register_module_ex(zend_module_entry *module);
ZEND_API zend_result zend_startup_module_ex(zend_module_entry *module);
ZEND_API void zend_startup_modules(void);
ZEND_API void zend_collect_module_handlers(void);
ZEND_API void zend_destroy_modules(void);
ZEND_API void zend_check_magic_method_implementation(
		const zend_class_entry *ce, const zend_function *fptr, zend_string *lcname, int error_type);
ZEND_API void zend_add_magic_method(zend_class_entry *ce, zend_function *fptr, zend_string *lcname);

ZEND_API zend_class_entry *zend_register_internal_class(zend_class_entry *class_entry);
ZEND_API zend_class_entry *zend_register_internal_class_ex(zend_class_entry *class_entry, zend_class_entry *parent_ce);
ZEND_API zend_class_entry *zend_register_internal_interface(zend_class_entry *orig_class_entry);
ZEND_API void zend_class_implements(zend_class_entry *class_entry, int num_interfaces, ...);

ZEND_API zend_result zend_register_class_alias_ex(const char *name, size_t name_len, zend_class_entry *ce, bool persistent);

// ing3, 注册永久类别名。p1:别名字串，p3:类
static zend_always_inline zend_result zend_register_class_alias(const char *name, zend_class_entry *ce) {
	// 注册类别名,p1:别名字串，p2:别名长度，p3:类，p4:是否永久
	return zend_register_class_alias_ex(name, strlen(name), ce, 1);
}

// ing3, 注册永久类别名。p1:命名空间，p2:别名字串，p3:类
#define zend_register_ns_class_alias(ns, name, ce) \
	zend_register_class_alias_ex(ZEND_NS_NAME(ns, name), sizeof(ZEND_NS_NAME(ns, name))-1, ce, 1)

ZEND_API void zend_disable_functions(const char *function_list);
ZEND_API zend_result zend_disable_class(const char *class_name, size_t class_name_length);

ZEND_API ZEND_COLD void zend_wrong_param_count(void);
ZEND_API ZEND_COLD void zend_wrong_property_read(zval *object, zval *property);

#define IS_CALLABLE_CHECK_SYNTAX_ONLY (1<<0)
#define IS_CALLABLE_SUPPRESS_DEPRECATIONS (1<<1)

ZEND_API void zend_release_fcall_info_cache(zend_fcall_info_cache *fcc);
ZEND_API zend_string *zend_get_callable_name_ex(zval *callable, zend_object *object);
ZEND_API zend_string *zend_get_callable_name(zval *callable);
ZEND_API bool zend_is_callable_at_frame(
		zval *callable, zend_object *object, zend_execute_data *frame,
		uint32_t check_flags, zend_fcall_info_cache *fcc, char **error);
ZEND_API bool zend_is_callable_ex(zval *callable, zend_object *object, uint32_t check_flags, zend_string **callable_name, zend_fcall_info_cache *fcc, char **error);
ZEND_API bool zend_is_callable(zval *callable, uint32_t check_flags, zend_string **callable_name);
ZEND_API bool zend_make_callable(zval *callable, zend_string **callable_name);
ZEND_API const char *zend_get_module_version(const char *module_name);
ZEND_API int zend_get_module_started(const char *module_name);

ZEND_API zend_property_info *zend_declare_typed_property(zend_class_entry *ce, zend_string *name, zval *property, int access_type, zend_string *doc_comment, zend_type type);

ZEND_API void zend_declare_property_ex(zend_class_entry *ce, zend_string *name, zval *property, int access_type, zend_string *doc_comment);
ZEND_API void zend_declare_property(zend_class_entry *ce, const char *name, size_t name_length, zval *property, int access_type);
ZEND_API void zend_declare_property_null(zend_class_entry *ce, const char *name, size_t name_length, int access_type);
ZEND_API void zend_declare_property_bool(zend_class_entry *ce, const char *name, size_t name_length, zend_long value, int access_type);
ZEND_API void zend_declare_property_long(zend_class_entry *ce, const char *name, size_t name_length, zend_long value, int access_type);
ZEND_API void zend_declare_property_double(zend_class_entry *ce, const char *name, size_t name_length, double value, int access_type);
ZEND_API void zend_declare_property_string(zend_class_entry *ce, const char *name, size_t name_length, const char *value, int access_type);
ZEND_API void zend_declare_property_stringl(zend_class_entry *ce, const char *name, size_t name_length, const char *value, size_t value_len, int access_type);

ZEND_API zend_class_constant *zend_declare_class_constant_ex(zend_class_entry *ce, zend_string *name, zval *value, int access_type, zend_string *doc_comment);
ZEND_API void zend_declare_class_constant(zend_class_entry *ce, const char *name, size_t name_length, zval *value);
ZEND_API void zend_declare_class_constant_null(zend_class_entry *ce, const char *name, size_t name_length);
ZEND_API void zend_declare_class_constant_long(zend_class_entry *ce, const char *name, size_t name_length, zend_long value);
ZEND_API void zend_declare_class_constant_bool(zend_class_entry *ce, const char *name, size_t name_length, bool value);
ZEND_API void zend_declare_class_constant_double(zend_class_entry *ce, const char *name, size_t name_length, double value);
ZEND_API void zend_declare_class_constant_stringl(zend_class_entry *ce, const char *name, size_t name_length, const char *value, size_t value_length);
ZEND_API void zend_declare_class_constant_string(zend_class_entry *ce, const char *name, size_t name_length, const char *value);

ZEND_API zend_result zend_update_class_constants(zend_class_entry *class_type);
ZEND_API HashTable *zend_separate_class_constants_table(zend_class_entry *class_type);

// ing3, 返回 ce->mutable_data 或者 ce本身的 常量列表
static zend_always_inline HashTable *zend_class_constants_table(zend_class_entry *ce) {
	// 如果 类包含常量 并且 有 转换数据
	if ((ce->ce_flags & ZEND_ACC_HAS_AST_CONSTANTS) && ZEND_MAP_PTR(ce->mutable_data)) {
		// 取得转换数据
		zend_class_mutable_data *mutable_data =
			(zend_class_mutable_data*)ZEND_MAP_PTR_GET_IMM(ce->mutable_data);
		// 如果转换数据里的常量表有效
		if (mutable_data && mutable_data->constants_table) {
			// 返回此常量表
			return mutable_data->constants_table;
		// 否则 
		} else {
			// 返回类的常量表副本
			return zend_separate_class_constants_table(ce);
		}
	// 否则 ，返回自身的 常量表
	} else {
		return &ce->constants_table;
	}
}
// ing3, 返回 ce->mutable_data 或者 ce本身的 默认属性列表
static zend_always_inline zval *zend_class_default_properties_table(zend_class_entry *ce) {
	// 如果 类包含属性 并且 有 转换数据
	if ((ce->ce_flags & ZEND_ACC_HAS_AST_PROPERTIES) && ZEND_MAP_PTR(ce->mutable_data)) {
		// 取得转换数据
		zend_class_mutable_data *mutable_data =
			(zend_class_mutable_data*)ZEND_MAP_PTR_GET_IMM(ce->mutable_data);
		// 返回转换数据 中的默认属性列表
		return mutable_data->default_properties_table;
	// 否则 ，返回自身的 属性列表
	} else {
		return ce->default_properties_table;
	}
}

// ing4, 如果类入口有可转换数据，把备用表关联到可转换数据上。否则，把备用表关联到类入口上。
static zend_always_inline void zend_class_set_backed_enum_table(zend_class_entry *ce, HashTable *backed_enum_table)
{
	// 如果有 ce->mutable_data__ptr 并且 类的类型是用户定义类
	if (ZEND_MAP_PTR(ce->mutable_data) && ce->type == ZEND_USER_CLASS) {
		// 通过偏移量 获取 可转换数据指针
		zend_class_mutable_data *mutable_data = (zend_class_mutable_data*)ZEND_MAP_PTR_GET_IMM(ce->mutable_data);
		// 把 备用表添加到可转换数据里
		mutable_data->backed_enum_table = backed_enum_table;
	// 如果没有可转换数据 指针
	} else {
		// 直接添加到类入口上
		ce->backed_enum_table = backed_enum_table;
	}
}

// ing4, 返回 ce->mutable_data 或者 ce本身的 enum备用表
static zend_always_inline HashTable *zend_class_backed_enum_table(zend_class_entry *ce)
{
	// 如果类有转换数据 并且 类的类型是用户定义
	if (ZEND_MAP_PTR(ce->mutable_data) && ce->type == ZEND_USER_CLASS) {
		// 从偏移量获取 转换数据表
		zend_class_mutable_data *mutable_data = (zend_class_mutable_data*)ZEND_MAP_PTR_GET_IMM(ce->mutable_data);
		// 返回转换数据表中的 enum备用表
		return mutable_data->backed_enum_table;
	// 否则 ，返回 enum备用表
	} else {
		return ce->backed_enum_table;
	}
}

ZEND_API void zend_update_property_ex(zend_class_entry *scope, zend_object *object, zend_string *name, zval *value);
ZEND_API void zend_update_property(zend_class_entry *scope, zend_object *object, const char *name, size_t name_length, zval *value);
ZEND_API void zend_update_property_null(zend_class_entry *scope, zend_object *object, const char *name, size_t name_length);
ZEND_API void zend_update_property_bool(zend_class_entry *scope, zend_object *object, const char *name, size_t name_length, zend_long value);
ZEND_API void zend_update_property_long(zend_class_entry *scope, zend_object *object, const char *name, size_t name_length, zend_long value);
ZEND_API void zend_update_property_double(zend_class_entry *scope, zend_object *object, const char *name, size_t name_length, double value);
ZEND_API void zend_update_property_str(zend_class_entry *scope, zend_object *object, const char *name, size_t name_length, zend_string *value);
ZEND_API void zend_update_property_string(zend_class_entry *scope, zend_object *object, const char *name, size_t name_length, const char *value);
ZEND_API void zend_update_property_stringl(zend_class_entry *scope, zend_object *object, const char *name, size_t name_length, const char *value, size_t value_length);
ZEND_API void zend_unset_property(zend_class_entry *scope, zend_object *object, const char *name, size_t name_length);

ZEND_API zend_result zend_update_static_property_ex(zend_class_entry *scope, zend_string *name, zval *value);
ZEND_API zend_result zend_update_static_property(zend_class_entry *scope, const char *name, size_t name_length, zval *value);
ZEND_API zend_result zend_update_static_property_null(zend_class_entry *scope, const char *name, size_t name_length);
ZEND_API zend_result zend_update_static_property_bool(zend_class_entry *scope, const char *name, size_t name_length, zend_long value);
ZEND_API zend_result zend_update_static_property_long(zend_class_entry *scope, const char *name, size_t name_length, zend_long value);
ZEND_API zend_result zend_update_static_property_double(zend_class_entry *scope, const char *name, size_t name_length, double value);
ZEND_API zend_result zend_update_static_property_string(zend_class_entry *scope, const char *name, size_t name_length, const char *value);
ZEND_API zend_result zend_update_static_property_stringl(zend_class_entry *scope, const char *name, size_t name_length, const char *value, size_t value_length);

ZEND_API zval *zend_read_property_ex(zend_class_entry *scope, zend_object *object, zend_string *name, bool silent, zval *rv);
ZEND_API zval *zend_read_property(zend_class_entry *scope, zend_object *object, const char *name, size_t name_length, bool silent, zval *rv);

ZEND_API zval *zend_read_static_property_ex(zend_class_entry *scope, zend_string *name, bool silent);
ZEND_API zval *zend_read_static_property(zend_class_entry *scope, const char *name, size_t name_length, bool silent);

ZEND_API const char *zend_get_type_by_const(int type);

// this 是执行全局变量中的一个元素
#define ZEND_THIS                           (&EX(This))
// ing4, 取得this 或 null
#define getThis()							((Z_TYPE_P(ZEND_THIS) == IS_OBJECT) ? ZEND_THIS : NULL)
// ing4, 是否在调用类方法
#define ZEND_IS_METHOD_CALL()				(EX(func)->common.scope != NULL)

// ing4, 报错参数数量错误
#define WRONG_PARAM_COUNT					ZEND_WRONG_PARAM_COUNT()
// #define EX_NUM_ARGS() ZEND_CALL_NUM_ARGS(execute_data)
// ing4, 从操作码中取出参数数量 
#define ZEND_NUM_ARGS()						EX_NUM_ARGS()

// ing4, 报错参数数量错误
#define ZEND_WRONG_PARAM_COUNT()					{ zend_wrong_param_count(); return; }

#ifndef ZEND_WIN32
#define DLEXPORT
#endif
// ing4, 创建新数组并关联到 arg
#define array_init(arg)				ZVAL_ARR((arg), zend_new_array(0))
// ing4, 创建指定元素数量的新数组并关联到 arg
#define array_init_size(arg, size)	ZVAL_ARR((arg), zend_new_array(size))
ZEND_API void object_init(zval *arg);
ZEND_API zend_result object_init_ex(zval *arg, zend_class_entry *ce);
ZEND_API zend_result object_and_properties_init(zval *arg, zend_class_entry *ce, HashTable *properties);
ZEND_API void object_properties_init(zend_object *object, zend_class_entry *class_type);
ZEND_API void object_properties_init_ex(zend_object *object, HashTable *properties);
ZEND_API void object_properties_load(zend_object *object, HashTable *properties);

ZEND_API void zend_merge_properties(zval *obj, HashTable *properties);

ZEND_API void add_assoc_long_ex(zval *arg, const char *key, size_t key_len, zend_long n);
ZEND_API void add_assoc_null_ex(zval *arg, const char *key, size_t key_len);
ZEND_API void add_assoc_bool_ex(zval *arg, const char *key, size_t key_len, bool b);
ZEND_API void add_assoc_resource_ex(zval *arg, const char *key, size_t key_len, zend_resource *r);
ZEND_API void add_assoc_double_ex(zval *arg, const char *key, size_t key_len, double d);
ZEND_API void add_assoc_str_ex(zval *arg, const char *key, size_t key_len, zend_string *str);
ZEND_API void add_assoc_string_ex(zval *arg, const char *key, size_t key_len, const char *str);
ZEND_API void add_assoc_stringl_ex(zval *arg, const char *key, size_t key_len, const char *str, size_t length);
ZEND_API void add_assoc_array_ex(zval *arg, const char *key, size_t key_len, zend_array *arr);
ZEND_API void add_assoc_object_ex(zval *arg, const char *key, size_t key_len, zend_object *obj);
ZEND_API void add_assoc_reference_ex(zval *arg, const char *key, size_t key_len, zend_reference *ref);
ZEND_API void add_assoc_zval_ex(zval *arg, const char *key, size_t key_len, zval *value);

// ing4, 向zval下属哈希表中添加，键为字串，值为 整数 的元素
static zend_always_inline void add_assoc_long(zval *arg, const char *key, zend_long n) {
	add_assoc_long_ex(arg, key, strlen(key), n);
}

// ing4, 向zval下属哈希表中添加，键为字串，值为 NULL 的元素
static zend_always_inline void add_assoc_null(zval *arg, const char *key) {
	add_assoc_null_ex(arg, key, strlen(key));
}

// ing4, 向zval下属哈希表中添加，键为字串，值为 BOOL值 的元素
static zend_always_inline void add_assoc_bool(zval *arg, const char *key, bool b) {
	add_assoc_bool_ex(arg, key, strlen(key), b);
}
// ing4, 向zval下属哈希表中添加，键为字串，值为 资源 的元素
static zend_always_inline void add_assoc_resource(zval *arg, const char *key, zend_resource *r) {
	add_assoc_resource_ex(arg, key, strlen(key), r);
}
// ing4, 向zval下属哈希表中添加，键为字串，值为 小数 的元素
static zend_always_inline void add_assoc_double(zval *arg, const char *key, double d) {
	add_assoc_double_ex(arg, key, strlen(key), d);
}
// ing4, 向zval下属哈希表中添加，键为字串，值为 zend_string 的元素
static zend_always_inline void add_assoc_str(zval *arg, const char *key, zend_string *str) {
	add_assoc_str_ex(arg, key, strlen(key), str);
}
// ing4, 向zval下属哈希表中添加，键为字串，值为 字串 的元素
static zend_always_inline void add_assoc_string(zval *arg, const char *key, const char *str) {
	add_assoc_string_ex(arg, key, strlen(key), str);
}
// ing4, 向zval下属哈希表中添加，键为字串（带长度），值为 字串 的元素
static zend_always_inline void add_assoc_stringl(zval *arg, const char *key, const char *str, size_t length) {
	add_assoc_stringl_ex(arg, key, strlen(key), str, length);
}
// ing4, 向zval下属哈希表中添加，键为字串，值为 数组 的元素
static zend_always_inline void add_assoc_array(zval *arg, const char *key, zend_array *arr) {
	add_assoc_array_ex(arg, key, strlen(key), arr);
}
// ing4, 向zval下属哈希表中添加，键为字串，值为 对象 的元素
static zend_always_inline void add_assoc_object(zval *arg, const char *key, zend_object *obj) {
	add_assoc_object_ex(arg, key, strlen(key), obj);
}
// ing4, 向zval下属哈希表中添加，键为字串，值为 引用 的元素
static zend_always_inline void add_assoc_reference(zval *arg, const char *key, zend_reference *ref) {
	add_assoc_reference_ex(arg, key, strlen(key), ref);
}
// ing4, 向zval下属哈希表中添加，键为字串，值为 zval 的元素
static zend_always_inline void add_assoc_zval(zval *arg, const char *key, zval *value) {
	add_assoc_zval_ex(arg, key, strlen(key), value);
}

ZEND_API void add_index_long(zval *arg, zend_ulong index, zend_long n);
ZEND_API void add_index_null(zval *arg, zend_ulong index);
ZEND_API void add_index_bool(zval *arg, zend_ulong index, bool b);
ZEND_API void add_index_resource(zval *arg, zend_ulong index, zend_resource *r);
ZEND_API void add_index_double(zval *arg, zend_ulong index, double d);
ZEND_API void add_index_str(zval *arg, zend_ulong index, zend_string *str);
ZEND_API void add_index_string(zval *arg, zend_ulong index, const char *str);
ZEND_API void add_index_stringl(zval *arg, zend_ulong index, const char *str, size_t length);
ZEND_API void add_index_array(zval *arg, zend_ulong index, zend_array *arr);
ZEND_API void add_index_object(zval *arg, zend_ulong index, zend_object *obj);
ZEND_API void add_index_reference(zval *arg, zend_ulong index, zend_reference *ref);

// ing3, 更新哈希表里的元素，p1:参数，哈希表，p2:索引号，p3:值
static zend_always_inline zend_result add_index_zval(zval *arg, zend_ulong index, zval *value)
{
	return zend_hash_index_update(Z_ARRVAL_P(arg), index, value) ? SUCCESS : FAILURE;
}

ZEND_API zend_result add_next_index_long(zval *arg, zend_long n);
ZEND_API zend_result add_next_index_null(zval *arg);
ZEND_API zend_result add_next_index_bool(zval *arg, bool b);
ZEND_API zend_result add_next_index_resource(zval *arg, zend_resource *r);
ZEND_API zend_result add_next_index_double(zval *arg, double d);
ZEND_API zend_result add_next_index_str(zval *arg, zend_string *str);
ZEND_API zend_result add_next_index_string(zval *arg, const char *str);
ZEND_API zend_result add_next_index_stringl(zval *arg, const char *str, size_t length);
ZEND_API zend_result add_next_index_array(zval *arg, zend_array *arr);
ZEND_API zend_result add_next_index_object(zval *arg, zend_object *obj);
ZEND_API zend_result add_next_index_reference(zval *arg, zend_reference *ref);

// ing4, 在哈希表中，下一个位置上插入元素。p1:哈希表，p2:新元素 zval
static zend_always_inline zend_result add_next_index_zval(zval *arg, zval *value)
{
	// 哈希表中，在下一个位置上插入
	return zend_hash_next_index_insert(Z_ARRVAL_P(arg), value) ? SUCCESS : FAILURE;
}

ZEND_API zend_result array_set_zval_key(HashTable *ht, zval *key, zval *value);

ZEND_API void add_property_long_ex(zval *arg, const char *key, size_t key_len, zend_long l);
ZEND_API void add_property_null_ex(zval *arg, const char *key, size_t key_len);
ZEND_API void add_property_bool_ex(zval *arg, const char *key, size_t key_len, zend_long b);
ZEND_API void add_property_resource_ex(zval *arg, const char *key, size_t key_len, zend_resource *r);
ZEND_API void add_property_double_ex(zval *arg, const char *key, size_t key_len, double d);
ZEND_API void add_property_str_ex(zval *arg, const char *key, size_t key_len, zend_string *str);
ZEND_API void add_property_string_ex(zval *arg, const char *key, size_t key_len, const char *str);
ZEND_API void add_property_stringl_ex(zval *arg, const char *key, size_t key_len,  const char *str, size_t length);
ZEND_API void add_property_array_ex(zval *arg, const char *key, size_t key_len, zend_array *arr);
ZEND_API void add_property_object_ex(zval *arg, const char *key, size_t key_len, zend_object *obj);
ZEND_API void add_property_reference_ex(zval *arg, const char *key, size_t key_len, zend_reference *ref);
ZEND_API void add_property_zval_ex(zval *arg, const char *key, size_t key_len, zval *value);

// ing4, 给的对象添加属性,传入属性名，值为 整数
static zend_always_inline void add_property_long(zval *arg, const char *key, zend_long n) {
	add_property_long_ex(arg, key, strlen(key), n);
}

// ing4, 给的对象添加属性,传入属性名，值为 null
static zend_always_inline void add_property_null(zval *arg, const char *key) {
	add_property_null_ex(arg, key, strlen(key));
}

// ing4, 给的对象添加属性,传入属性名，值为 布尔型
static zend_always_inline void add_property_bool(zval *arg, const char *key, bool b) {
	add_property_bool_ex(arg, key, strlen(key), b);
}

// ing4, 给的对象添加属性,传入属性名，值为 资源
static zend_always_inline void add_property_resource(zval *arg, const char *key, zend_resource *r) {
	add_property_resource_ex(arg, key, strlen(key), r);
}

// ing4, 给的对象添加属性,传入属性名，值为 小数
static zend_always_inline void add_property_double(zval *arg, const char *key, double d) {
	add_property_double_ex(arg, key, strlen(key), d);
}

// ing4, 给的对象添加属性,传入属性名，值为 zend_string
static zend_always_inline void add_property_str(zval *arg, const char *key, zend_string *str) {
	add_property_str_ex(arg, key, strlen(key), str);
}

// ing4, 给的对象添加属性,传入属性名，值为 字串
static zend_always_inline void add_property_string(zval *arg, const char *key, const char *str) {
	add_property_string_ex(arg, key, strlen(key), str);
}

// ing4, 给的对象添加属性,传入属性名，值为 字串和长度
static zend_always_inline void add_property_stringl(zval *arg, const char *key, const char *str, size_t length) {
	add_property_stringl_ex(arg, key, strlen(key), str, length);
}

// ing4, 给的对象添加属性,传入属性名，值为 数组
static zend_always_inline void add_property_array(zval *arg, const char *key, zend_array *arr) {
	add_property_array_ex(arg, key, strlen(key), arr);
}

// ing4, 给的对象添加属性,传入属性名，值为 对象
static zend_always_inline void add_property_object(zval *arg, const char *key, zend_object *obj) {
	add_property_object_ex(arg, key, strlen(key), obj);
}

// ing4, 给的对象添加属性,传入属性名，值为 引用类型
static zend_always_inline void add_property_reference(zval *arg, const char *key, zend_reference *ref) {
	add_property_reference_ex(arg, key, strlen(key), ref);
}

// ing4, 给的对象添加属性, p1:zval, p2:属性名, p3: 属性值
static zend_always_inline void add_property_zval(zval *arg, const char *key, zval *value) {
	add_property_zval_ex(arg, key, strlen(key), value);
}

ZEND_API zend_result _call_user_function_impl(zval *object, zval *function_name, zval *retval_ptr, uint32_t param_count, zval params[], HashTable *named_params);

// ing3, 调用用户函数，无命名参数。p1:没用到，p2:对象，p3:函数名，p4:返回值，p5:参数数量，p6:参数列表
#define call_user_function(function_table, object, function_name, retval_ptr, param_count, params) \
	_call_user_function_impl(object, function_name, retval_ptr, param_count, params, NULL)

// ing3, 调用用户函数。p1:没用到，p2:对象，p3:函数名，p4:返回值，p5:参数数量，p6:参数列表，p7:命名参数
#define call_user_function_named(function_table, object, function_name, retval_ptr, param_count, params, named_params) \
	_call_user_function_impl(object, function_name, retval_ptr, param_count, params, named_params)

ZEND_API extern const zend_fcall_info empty_fcall_info;
ZEND_API extern const zend_fcall_info_cache empty_fcall_info_cache;

/** Build zend_call_info/cache from a zval*
 *
 * Caller is responsible to provide a return value (fci->retval), otherwise the we will crash.
 * In order to pass parameters the following members need to be set:
 * fci->param_count = 0;
 * fci->params = NULL;
 * The callable_name argument may be NULL.
 */
ZEND_API zend_result zend_fcall_info_init(zval *callable, uint32_t check_flags, zend_fcall_info *fci, zend_fcall_info_cache *fcc, zend_string **callable_name, char **error);

/** Clear arguments connected with zend_fcall_info *fci
 * If free_mem is not zero then the params array gets free'd as well
 */
ZEND_API void zend_fcall_info_args_clear(zend_fcall_info *fci, bool free_mem);

/** Save current arguments from zend_fcall_info *fci
 * params array will be set to NULL
 */
ZEND_API void zend_fcall_info_args_save(zend_fcall_info *fci, uint32_t *param_count, zval **params);

/** Free arguments connected with zend_fcall_info *fci and set back saved ones.
 */
ZEND_API void zend_fcall_info_args_restore(zend_fcall_info *fci, uint32_t param_count, zval *params);

/** Set or clear the arguments in the zend_call_info struct taking care of
 * refcount. If args is NULL and arguments are set then those are cleared.
 */
ZEND_API zend_result zend_fcall_info_args(zend_fcall_info *fci, zval *args);
ZEND_API zend_result zend_fcall_info_args_ex(zend_fcall_info *fci, zend_function *func, zval *args);

/** Set arguments in the zend_fcall_info struct taking care of refcount.
 * If argc is 0 the arguments which are set will be cleared, else pass
 * a variable amount of zval** arguments.
 */
ZEND_API void zend_fcall_info_argp(zend_fcall_info *fci, uint32_t argc, zval *argv);

/** Set arguments in the zend_fcall_info struct taking care of refcount.
 * If argc is 0 the arguments which are set will be cleared, else pass
 * a variable amount of zval** arguments.
 */
ZEND_API void zend_fcall_info_argv(zend_fcall_info *fci, uint32_t argc, va_list *argv);

/** Set arguments in the zend_fcall_info struct taking care of refcount.
 * If argc is 0 the arguments which are set will be cleared, else pass
 * a variable amount of zval** arguments.
 */
ZEND_API void zend_fcall_info_argn(zend_fcall_info *fci, uint32_t argc, ...);

/** Call a function using information created by zend_fcall_info_init()/args().
 * If args is given then those replace the argument info in fci is temporarily.
 */
ZEND_API zend_result zend_fcall_info_call(zend_fcall_info *fci, zend_fcall_info_cache *fcc, zval *retval, zval *args);

/* Can only return FAILURE if EG(active) is false during late engine shutdown.
 * If the call or call setup throws, EG(exception) will be set and the retval
 * will be UNDEF. Otherwise, the retval will be a non-UNDEF value. */
ZEND_API zend_result zend_call_function(zend_fcall_info *fci, zend_fcall_info_cache *fci_cache);

/* Call the provided zend_function with the given params.
 * If retval_ptr is NULL, the return value is discarded.
 * If object is NULL, this must be a free function or static call.
 * called_scope must be provided for instance and static method calls. */
ZEND_API void zend_call_known_function(
		zend_function *fn, zend_object *object, zend_class_entry *called_scope, zval *retval_ptr,
		uint32_t param_count, zval *params, HashTable *named_params);

// 在对象上，调用提供的 成员方法
/* Call the provided zend_function instance method on an object. */
// ing4, 调用对象的方法，p1:方法，p2:对象，p3:返回值，p4:参数数量，p5:参数列表
static zend_always_inline void zend_call_known_instance_method(
		zend_function *fn, zend_object *object, zval *retval_ptr,
		uint32_t param_count, zval *params)
{
	// 调用对象的方法，p1:方法，p2:对象，p3:域，p4:返回值，p5:参数数量，p6:参数列表，p7:命名参数表
	zend_call_known_function(fn, object, object->ce, retval_ptr, param_count, params, NULL);
}

// ing3, 调用对象的方法，无参数。p1:方法，p2:对象，p3:返回值
static zend_always_inline void zend_call_known_instance_method_with_0_params(
		zend_function *fn, zend_object *object, zval *retval_ptr)
{
	// 调用对象的方法，p1:方法，p2:对象，p3:返回值，p4:参数数量，p5:参数列表
	zend_call_known_instance_method(fn, object, retval_ptr, 0, NULL);
}

// ing4, 调用已知对象的方法，1个附加参数。p1:方法，p2:所属对象:p3，返回结果，p4:参数
static zend_always_inline void zend_call_known_instance_method_with_1_params(
		zend_function *fn, zend_object *object, zval *retval_ptr, zval *param)
{
	// 调用对象的方法，p1:方法，p2:对象，p3:返回值，p4:参数数量，p5:参数列表
	zend_call_known_instance_method(fn, object, retval_ptr, 1, param);
}

ZEND_API void zend_call_known_instance_method_with_2_params(
		zend_function *fn, zend_object *object, zval *retval_ptr, zval *param1, zval *param2);

/* Call method if it exists. Return FAILURE if method does not exist or call failed.
 * If FAILURE is returned, retval will be UNDEF. As such, destroying retval unconditionally
 * is legal. */
ZEND_API zend_result zend_call_method_if_exists(
		zend_object *object, zend_string *method_name, zval *retval,
		uint32_t param_count, zval *params);

ZEND_API zend_result zend_set_hash_symbol(zval *symbol, const char *name, size_t name_length, bool is_ref, int num_symbol_tables, ...);

ZEND_API zend_result zend_delete_global_variable(zend_string *name);

ZEND_API zend_array *zend_rebuild_symbol_table(void);
ZEND_API void zend_attach_symbol_table(zend_execute_data *execute_data);
ZEND_API void zend_detach_symbol_table(zend_execute_data *execute_data);
ZEND_API zend_result zend_set_local_var(zend_string *name, zval *value, bool force);
ZEND_API zend_result zend_set_local_var_str(const char *name, size_t len, zval *value, bool force);

// ing4, 禁止动态调用，碰到动态调用就报错。
static zend_always_inline zend_result zend_forbid_dynamic_call(void)
{
	// 当前执行数据
	zend_execute_data *ex = EG(current_execute_data);
	// 有执行数据 并且 有函数
	ZEND_ASSERT(ex != NULL && ex->func != NULL);

	// 如果 是动态调用
	if (ZEND_CALL_INFO(ex) & ZEND_CALL_DYNAMIC) {
		// 取得函数名 或 方法名
		zend_string *function_or_method_name = get_active_function_or_method_name();
		// 报错：不可以动态调用此方法
		zend_throw_error(NULL, "Cannot call %.*s() dynamically",
			(int) ZSTR_LEN(function_or_method_name), ZSTR_VAL(function_or_method_name));
		// 释放方法名
		zend_string_release(function_or_method_name);
		// 返回失败
		return FAILURE;
	}

	// 返回成功
	return SUCCESS;
}

ZEND_API ZEND_COLD const char *zend_get_object_type_case(const zend_class_entry *ce, bool upper_case);

// ing3, 返回 对象类型（trait、接口、枚举、类）
static zend_always_inline const char *zend_get_object_type(const zend_class_entry *ce)
{
	return zend_get_object_type_case(ce, false);
}

// ing3, 返回 首字母大写 对象类型（trait、接口、枚举、类）
static zend_always_inline const char *zend_get_object_type_uc(const zend_class_entry *ce)
{
	return zend_get_object_type_case(ce, true);
}

ZEND_API bool zend_is_iterable(zval *iterable);

ZEND_API bool zend_is_countable(zval *countable);

ZEND_API zend_result zend_get_default_from_internal_arg_info(
		zval *default_value_zval, zend_internal_arg_info *arg_info);

END_EXTERN_C()

// 调试用到
#if ZEND_DEBUG
#define CHECK_ZVAL_STRING(str) \
	ZEND_ASSERT(ZSTR_VAL(str)[ZSTR_LEN(str)] == '\0' && "String is not null-terminated");
// 正式环境无业务
#else
#define CHECK_ZVAL_STRING(z)
#endif

// ing4, 检验zend_string是否包含空字符。它的长度是否与自己char * 相同
static zend_always_inline bool zend_str_has_nul_byte(const zend_string *str)
{
	return ZSTR_LEN(str) != strlen(ZSTR_VAL(str));
}

// clear, 检验：字串中是否包含无效字符，（已知长度是否与计算出的长度相符）
static zend_always_inline bool zend_char_has_nul_byte(const char *s, size_t known_length)
{
	return known_length != strlen(s);
}

/* Compatibility with PHP 8.1 and below */
// ing4, 检验zend_string是否包含空字符。它的长度是否与自己char * 相同
#define CHECK_ZVAL_NULL_PATH(p) zend_str_has_nul_byte(Z_STR_P(p))
// clear, 检验：字串中是否包含无效字符，（已知长度是否与计算出的长度相符）
#define CHECK_NULL_PATH(p, l) zend_char_has_nul_byte(p, l)

// ing4, 用给定字串 和长度 创建 zend_string，并关联到 zval 上
#define ZVAL_STRINGL(z, s, l) do {				\
		ZVAL_NEW_STR(z, zend_string_init(s, l, 0));		\
	} while (0)
// ing4, 用给定字串 创建 zend_string，并关联到 zval 上
#define ZVAL_STRING(z, s) do {					\
		const char *_s = (s);					\
		ZVAL_STRINGL(z, _s, strlen(_s));		\
	} while (0)
// ing4, 创建空保留字串并关联到 zval
#define ZVAL_EMPTY_STRING(z) do {				\
		ZVAL_INTERNED_STR(z, ZSTR_EMPTY_ALLOC());		\
	} while (0)
// ing4, 用给定字串 和长度 创建 永久zend_string，并关联到 zval 上
#define ZVAL_PSTRINGL(z, s, l) do {				\
		ZVAL_NEW_STR(z, zend_string_init(s, l, 1));		\
	} while (0)
// ing4, 用给定字串 创建 永久zend_string，并关联到 zval 上
#define ZVAL_PSTRING(z, s) do {					\
		const char *_s = (s);					\
		ZVAL_PSTRINGL(z, _s, strlen(_s));		\
	} while (0)
// ing4, 创建 永久空zend_string，并关联到 zval 上
#define ZVAL_EMPTY_PSTRING(z) do {				\
		ZVAL_PSTRINGL(z, "", 0);				\
	} while (0)
// ing4, 取出单个字符 保留字串并关联到 zval
#define ZVAL_CHAR(z, c)  do {		            \
		char _c = (c);                          \
		ZVAL_INTERNED_STR(z, ZSTR_CHAR((zend_uchar) _c));	\
	} while (0)
// ing4,  使用 char*, len, 快速创建zend_string(对单个字符和空值做了快速处理) 并关联到 zval
#define ZVAL_STRINGL_FAST(z, s, l) do {			\
		ZVAL_STR(z, zend_string_init_fast(s, l));	\
	} while (0)

// ing4,  使用 char*, 快速创建zend_string(对单个字符和空值做了快速处理)并关联到 zval
#define ZVAL_STRING_FAST(z, s) do {				\
		const char *_s = (s);					\
		ZVAL_STRINGL_FAST(z, _s, strlen(_s));	\
	} while (0)

// ing4, 从一个zval复制到另一个zval，如果要求销毁 或 没有要求复制，销毁原zval
#define ZVAL_ZVAL(z, zv, copy, dtor) do {		\
		zval *__z = (z);						\
		zval *__zv = (zv);						\
		/* 如果zv不是引用类型 */ \
		if (EXPECTED(!Z_ISREF_P(__zv))) {		\
			/* 如果要求复制 并且不销毁 */ \
			if (copy && !dtor) {				\
				/* 复制 zval 变量和附加信息,并增加了一次引用计数 */ \
				ZVAL_COPY(__z, __zv);			\
			} else {							\
				/* 复制 zval 变量和附加信息,不增加引用计数 */ \
				ZVAL_COPY_VALUE(__z, __zv);		\
			}									\
		/* 如果zv是引用类型 */ \
		} else {								\
			/* 复制 引用目标 变量的附加信息,并增加了一次引用计数 */ \
			ZVAL_COPY(__z, Z_REFVAL_P(__zv));	\
			/* 如果要求销毁 或 没有要求复制 */ \
			if (dtor || !copy) {				\
				/* 销毁原zval */ \
				zval_ptr_dtor(__zv);			\
			}									\
		}										\
	} while (0)

// ing4, 返回值(zval)里写入 bool值
#define RETVAL_BOOL(b)					ZVAL_BOOL(return_value, b)
// ing4, 返回值(zval)里写入 NULL
#define RETVAL_NULL()					ZVAL_NULL(return_value)
// ing4, 返回值(zval)里写入 整数
#define RETVAL_LONG(l)					ZVAL_LONG(return_value, l)
// ing4, 返回值(zval)里写入 小数
#define RETVAL_DOUBLE(d)				ZVAL_DOUBLE(return_value, d)
// ing4, 返回值(zval)里写入 zend_string
#define RETVAL_STR(s)					ZVAL_STR(return_value, s)
// ing4, 返回值(zval)里写入 保留字串 zend_string
#define RETVAL_INTERNED_STR(s)			ZVAL_INTERNED_STR(return_value, s)
// ing4, 把 zend_string 添加给 zval，不支持保留字
#define RETVAL_NEW_STR(s)				ZVAL_NEW_STR(return_value, s)
// ing4, 给 zval 添加 string, 并修改type_info（支持保留字），并增加引用次数（这就算copy了）
#define RETVAL_STR_COPY(s)				ZVAL_STR_COPY(return_value, s)
// ing4, 返回值(zval)里写入 char *
#define RETVAL_STRING(s)				ZVAL_STRING(return_value, s)
// ing4, 返回值(zval)里写入 char *, 带长度
#define RETVAL_STRINGL(s, l)			ZVAL_STRINGL(return_value, s, l)
// ing4, 使用 char*, 快速创建zend_string 并关联到 返回值(zval)
#define RETVAL_STRING_FAST(s)			ZVAL_STRING_FAST(return_value, s)
// ing4, 使用 char*, len, 快速创建zend_string(对单个字符和空值做了快速处理) 并关联到 返回值(zval)
#define RETVAL_STRINGL_FAST(s, l)		ZVAL_STRINGL_FAST(return_value, s, l)
// ing4, 创建空保留字串并关联到 返回值(zval)
#define RETVAL_EMPTY_STRING()			ZVAL_EMPTY_STRING(return_value)
// ing4, 返回值赋值为单个字符
#define RETVAL_CHAR(c)		            ZVAL_CHAR(return_value, c)
// ing4, 返回值赋值为资源
#define RETVAL_RES(r)					ZVAL_RES(return_value, r)
// ing4, 返回值赋值为数组
#define RETVAL_ARR(r)					ZVAL_ARR(return_value, r)
// ing4, 返回值赋值为空数组
#define RETVAL_EMPTY_ARRAY()			ZVAL_EMPTY_ARRAY(return_value)
// ing4, 返回值赋值为对象
#define RETVAL_OBJ(r)					ZVAL_OBJ(return_value, r)
// ing4, 返回值赋值为对象,并给对象增加引用次数
#define RETVAL_OBJ_COPY(r)				ZVAL_OBJ_COPY(return_value, r)
// ing4, 返回使用原值的type_info和计数器指针,并增加引用次数
#define RETVAL_COPY(zv)					ZVAL_COPY(return_value, zv)
// ing4, 返回使用原值的type_info和计数器指针,不加引用次数
#define RETVAL_COPY_VALUE(zv)			ZVAL_COPY_VALUE(return_value, zv)
// ing4, 复制引用目标 和 附加信息 到返回值，并增加引用计数
#define RETVAL_COPY_DEREF(zv)			ZVAL_COPY_DEREF(return_value, zv)
// ing4, 从一个zval复制到 返回结果，如果要求销毁 或 没有要求复制，销毁原zval
#define RETVAL_ZVAL(zv, copy, dtor)		ZVAL_ZVAL(return_value, zv, copy, dtor)
// ing4, 返回值为 false
#define RETVAL_FALSE					ZVAL_FALSE(return_value)
// ing4, 返回值为 true
#define RETVAL_TRUE						ZVAL_TRUE(return_value)

// ing4, 给返回值变量赋值后返回
#define RETURN_BOOL(b)					do { RETVAL_BOOL(b); return; } while (0)
#define RETURN_NULL()					do { RETVAL_NULL(); return;} while (0)
#define RETURN_LONG(l)					do { RETVAL_LONG(l); return; } while (0)
#define RETURN_DOUBLE(d)				do { RETVAL_DOUBLE(d); return; } while (0)
#define RETURN_STR(s) 					do { RETVAL_STR(s); return; } while (0)
#define RETURN_INTERNED_STR(s)			do { RETVAL_INTERNED_STR(s); return; } while (0)
#define RETURN_NEW_STR(s)				do { RETVAL_NEW_STR(s); return; } while (0)
#define RETURN_STR_COPY(s)				do { RETVAL_STR_COPY(s); return; } while (0)
#define RETURN_STRING(s) 				do { RETVAL_STRING(s); return; } while (0)
#define RETURN_STRINGL(s, l) 			do { RETVAL_STRINGL(s, l); return; } while (0)
#define RETURN_STRING_FAST(s) 			do { RETVAL_STRING_FAST(s); return; } while (0)
#define RETURN_STRINGL_FAST(s, l)		do { RETVAL_STRINGL_FAST(s, l); return; } while (0)
#define RETURN_EMPTY_STRING() 			do { RETVAL_EMPTY_STRING(); return; } while (0)
#define RETURN_CHAR(c)		            do { RETVAL_CHAR(c); return; } while (0)
#define RETURN_RES(r)					do { RETVAL_RES(r); return; } while (0)
#define RETURN_ARR(r)					do { RETVAL_ARR(r); return; } while (0)
#define RETURN_EMPTY_ARRAY()			do { RETVAL_EMPTY_ARRAY(); return; } while (0)
#define RETURN_OBJ(r)					do { RETVAL_OBJ(r); return; } while (0)
#define RETURN_OBJ_COPY(r)				do { RETVAL_OBJ_COPY(r); return; } while (0)
#define RETURN_COPY(zv)					do { RETVAL_COPY(zv); return; } while (0)
#define RETURN_COPY_VALUE(zv)			do { RETVAL_COPY_VALUE(zv); return; } while (0)
#define RETURN_COPY_DEREF(zv)			do { RETVAL_COPY_DEREF(zv); return; } while (0)
#define RETURN_ZVAL(zv, copy, dtor)		do { RETVAL_ZVAL(zv, copy, dtor); return; } while (0)
#define RETURN_FALSE					do { RETVAL_FALSE; return; } while (0)
#define RETURN_TRUE						do { RETVAL_TRUE; return; } while (0)
	
// ing1, 保证有异常的return 语句， (void) return_value; 是做什么?
#define RETURN_THROWS()					do { ZEND_ASSERT(EG(exception)); (void) return_value; return; } while (0)

// ing3, 如果是数组，返回哈希表。如果是对象，获取属性表并返回
#define HASH_OF(p) (Z_TYPE_P(p)==IS_ARRAY ? Z_ARRVAL_P(p) : ((Z_TYPE_P(p)==IS_OBJECT ? Z_OBJ_HT_P(p)->get_properties(Z_OBJ_P(p)) : NULL)))

// ing4，验证 zval 是否是 null
#define ZVAL_IS_NULL(z) (Z_TYPE_P(z) == IS_NULL)

/* For compatibility */
#define ZEND_MINIT			ZEND_MODULE_STARTUP_N
#define ZEND_MSHUTDOWN		ZEND_MODULE_SHUTDOWN_N
#define ZEND_RINIT			ZEND_MODULE_ACTIVATE_N
#define ZEND_RSHUTDOWN		ZEND_MODULE_DEACTIVATE_N
#define ZEND_MINFO			ZEND_MODULE_INFO_N

// ing3, 定义模块的全局构造方法
// ZEND_MODULE_GLOBALS_CTOR_N: 模块的【全局构造器】方法名
#define ZEND_GINIT(module)		((void (*)(void*))(ZEND_MODULE_GLOBALS_CTOR_N(module)))

// ing3, 定义模块的全局销毁方法
// ZEND_MODULE_GLOBALS_DTOR_N: 模块的【全局销毁器】方法名
#define ZEND_GSHUTDOWN(module)	((void (*)(void*))(ZEND_MODULE_GLOBALS_DTOR_N(module)))

#define ZEND_MINIT_FUNCTION			ZEND_MODULE_STARTUP_D
#define ZEND_MSHUTDOWN_FUNCTION		ZEND_MODULE_SHUTDOWN_D
#define ZEND_RINIT_FUNCTION			ZEND_MODULE_ACTIVATE_D
#define ZEND_RSHUTDOWN_FUNCTION		ZEND_MODULE_DEACTIVATE_D
#define ZEND_MINFO_FUNCTION			ZEND_MODULE_INFO_D
#define ZEND_GINIT_FUNCTION			ZEND_MODULE_GLOBALS_CTOR_D
#define ZEND_GSHUTDOWN_FUNCTION		ZEND_MODULE_GLOBALS_DTOR_D

/* May modify arg in-place. Will free arg in failure case (and take ownership in success case).
 * Prefer using the ZEND_TRY_ASSIGN_* macros over these APIs. */
ZEND_API zend_result zend_try_assign_typed_ref_ex(zend_reference *ref, zval *zv, bool strict);
ZEND_API zend_result zend_try_assign_typed_ref(zend_reference *ref, zval *zv);

ZEND_API zend_result zend_try_assign_typed_ref_null(zend_reference *ref);
ZEND_API zend_result zend_try_assign_typed_ref_bool(zend_reference *ref, bool val);
ZEND_API zend_result zend_try_assign_typed_ref_long(zend_reference *ref, zend_long lval);
ZEND_API zend_result zend_try_assign_typed_ref_double(zend_reference *ref, double dval);
ZEND_API zend_result zend_try_assign_typed_ref_empty_string(zend_reference *ref);
ZEND_API zend_result zend_try_assign_typed_ref_str(zend_reference *ref, zend_string *str);
ZEND_API zend_result zend_try_assign_typed_ref_string(zend_reference *ref, const char *string);
ZEND_API zend_result zend_try_assign_typed_ref_stringl(zend_reference *ref, const char *string, size_t len);
ZEND_API zend_result zend_try_assign_typed_ref_arr(zend_reference *ref, zend_array *arr);
ZEND_API zend_result zend_try_assign_typed_ref_res(zend_reference *ref, zend_resource *res);
ZEND_API zend_result zend_try_assign_typed_ref_zval(zend_reference *ref, zval *zv);
ZEND_API zend_result zend_try_assign_typed_ref_zval_ex(zend_reference *ref, zval *zv, bool strict);

// ing4, 给引用目标赋值，值为 null, p2:是否解引用
#define _ZEND_TRY_ASSIGN_NULL(zv, is_ref) do { \
	zval *_zv = zv; \
	/* 如果要引用赋值或 zv 是引用类型 */ \
	if (is_ref || UNEXPECTED(Z_ISREF_P(_zv))) { \
		/* 引用实例指针 */ \
		zend_reference *ref = Z_REF_P(_zv); \
		/* 引用目标有类型 */ \
		if (UNEXPECTED(ZEND_REF_HAS_TYPE_SOURCES(ref))) { \
			/* 给引用目标赋值，值为 null */ \
			zend_try_assign_typed_ref_null(ref); \
			break; \
		} \
		/* 没有类型，追踪到目标 */ \
		_zv = &ref->val; \
	} \
	/* 不用追踪引用，清空zv */ \
	zval_ptr_dtor(_zv); \
	/* 更新值为null */ \
	ZVAL_NULL(_zv); \
} while (0)

// ing4, 给引用目标赋值，值为 null, 不解引用
#define ZEND_TRY_ASSIGN_NULL(zv) \
	_ZEND_TRY_ASSIGN_NULL(zv, 0)

// ing4, 给引用目标赋值，值为 null, 解引用
#define ZEND_TRY_ASSIGN_REF_NULL(zv) do { \
	ZEND_ASSERT(Z_ISREF_P(zv)); \
	_ZEND_TRY_ASSIGN_NULL(zv, 1); \
} while (0)

// ing4, 给引用目标赋值，值为 false, p2:是否解引用
#define _ZEND_TRY_ASSIGN_FALSE(zv, is_ref) do { \
	zval *_zv = zv; \
	/* 如果要引用赋值或 zv 是引用类型 */ \
	if (is_ref || UNEXPECTED(Z_ISREF_P(_zv))) { \
		/* 引用实例指针 */ \
		zend_reference *ref = Z_REF_P(_zv); \
		/* 如果 检验引用目标是否有效 */ \
		if (UNEXPECTED(ZEND_REF_HAS_TYPE_SOURCES(ref))) { \
			/* 给引用目标赋值，值为 布尔型 */ \
			zend_try_assign_typed_ref_bool(ref, 0); \
			break; \
		} \
		/* 没有类型，追踪到目标 */ \
		_zv = &ref->val; \
	} \
	/* 不用追踪引用，清空zv */ \
	zval_ptr_dtor(_zv); \
	/* 更新值为 false */ \
	ZVAL_FALSE(_zv); \
} while (0)

// ing4, 给引用目标赋值，值为 false, 不解引用
#define ZEND_TRY_ASSIGN_FALSE(zv) \
	_ZEND_TRY_ASSIGN_FALSE(zv, 0)

// ing4, 给引用目标赋值，值为 false, 解引用
#define ZEND_TRY_ASSIGN_REF_FALSE(zv) do { \
	ZEND_ASSERT(Z_ISREF_P(zv)); \
	_ZEND_TRY_ASSIGN_FALSE(zv, 1); \
} while (0)

// ing4, 给引用目标赋值，值为 true, p2:是否解引用
#define _ZEND_TRY_ASSIGN_TRUE(zv, is_ref) do { \
	zval *_zv = zv; \
	/* 如果要引用赋值或 zv 是引用类型 */ \
	if (is_ref || UNEXPECTED(Z_ISREF_P(_zv))) { \
		/* 引用实例指针 */ \
		zend_reference *ref = Z_REF_P(_zv); \
		/* 如果 检验引用目标是否有效 */ \
		if (UNEXPECTED(ZEND_REF_HAS_TYPE_SOURCES(ref))) { \
			/* 给引用目标赋值，值为 布尔型 */ \
			zend_try_assign_typed_ref_bool(ref, 1); \
			break; \
		} \
		/* 没有类型，追踪到目标 */ \
		_zv = &ref->val; \
	} \
	/* 不用追踪引用，清空zv */ \
	zval_ptr_dtor(_zv); \
	/* 更新值为 true */ \
	ZVAL_TRUE(_zv); \
} while (0)

// ing4, 给引用目标赋值，值为 true, 不解引用
#define ZEND_TRY_ASSIGN_TRUE(zv) \
	_ZEND_TRY_ASSIGN_TRUE(zv, 0)

// ing4, 给引用目标赋值，值为 true, 解引用
#define ZEND_TRY_ASSIGN_REF_TRUE(zv) do { \
	ZEND_ASSERT(Z_ISREF_P(zv)); \
	_ZEND_TRY_ASSIGN_TRUE(zv, 1); \
} while (0)

// ing4, 给引用目标赋值，值为 布尔型, p2:传入值，p3:是否解引用
#define _ZEND_TRY_ASSIGN_BOOL(zv, bval, is_ref) do { \
	zval *_zv = zv; \
	/* 如果要引用赋值或 zv 是引用类型 */ \
	if (is_ref || UNEXPECTED(Z_ISREF_P(_zv))) { \
		/* 引用实例指针 */ \
		zend_reference *ref = Z_REF_P(_zv); \
		/* 如果 检验引用目标是否有效 */ \
		if (UNEXPECTED(ZEND_REF_HAS_TYPE_SOURCES(ref))) { \
			/* 给引用目标赋值，值为 布尔型 */ \
			zend_try_assign_typed_ref_bool(ref, 1); \
			break; \
		} \
		/* 没有类型，追踪到目标 */ \
		_zv = &ref->val; \
	} \
	/* 不用追踪引用，清空zv */ \
	zval_ptr_dtor(_zv); \
	/* 更新值为 布尔型 */ \
	ZVAL_BOOL(_zv, bval); \
} while (0)

// ing4, 给引用目标赋值，值为 布尔型, 不解引用, p2:传入值
#define ZEND_TRY_ASSIGN_BOOL(zv, bval) \
	_ZEND_TRY_ASSIGN_BOOL(zv, bval, 0)

// ing4, 给引用目标赋值，值为 布尔型, 解引用, p2:传入值
#define ZEND_TRY_ASSIGN_REF_BOOL(zv, bval) do { \
	ZEND_ASSERT(Z_ISREF_P(zv)); \
	_ZEND_TRY_ASSIGN_BOOL(zv, bval, 1); \
} while (0)

// ing4, 给引用目标赋值，值为 整数, p2:传入值，p3:是否解引用
#define _ZEND_TRY_ASSIGN_LONG(zv, lval, is_ref) do { \
	zval *_zv = zv; \
	/* 如果要引用赋值或 zv 是引用类型 */ \
	if (is_ref || UNEXPECTED(Z_ISREF_P(_zv))) { \
		/* 引用实例指针 */ \
		zend_reference *ref = Z_REF_P(_zv); \
		/* 如果 检验引用目标是否有效 */ \
		if (UNEXPECTED(ZEND_REF_HAS_TYPE_SOURCES(ref))) { \
			/* 给引用目标赋值，值为 整数 */ \
			zend_try_assign_typed_ref_long(ref, lval); \
			break; \
		} \
		/* 没有类型，追踪到目标 */ \
		_zv = &ref->val; \
	} \
	/* 不用追踪引用，清空zv */ \
	zval_ptr_dtor(_zv); \
	/* 更新值为 整数 */ \
	ZVAL_LONG(_zv, lval); \
} while (0)

// ing4, 给引用目标赋值，值为 整数，不解引用, p2:传入值
#define ZEND_TRY_ASSIGN_LONG(zv, lval) \
	_ZEND_TRY_ASSIGN_LONG(zv, lval, 0)

// ing4, 给引用目标赋值，值为 整数，解引用, p2:传入值
#define ZEND_TRY_ASSIGN_REF_LONG(zv, lval) do { \
	ZEND_ASSERT(Z_ISREF_P(zv)); \
	_ZEND_TRY_ASSIGN_LONG(zv, lval, 1); \
} while (0)

// ing4, 给引用目标赋值，值为 小数, p2:传入值，p3:是否解引用
#define _ZEND_TRY_ASSIGN_DOUBLE(zv, dval, is_ref) do { \
	zval *_zv = zv; \
	/* 如果要引用赋值或 zv 是引用类型 */ \
	if (is_ref || UNEXPECTED(Z_ISREF_P(_zv))) { \
		/* 引用实例指针 */ \
		zend_reference *ref = Z_REF_P(_zv); \
		/* 如果 检验引用目标是否有效 */ \
		if (UNEXPECTED(ZEND_REF_HAS_TYPE_SOURCES(ref))) { \
			/* 给引用目标赋值，值为 小数 */ \
			zend_try_assign_typed_ref_double(ref, dval); \
			break; \
		} \
		/* 没有类型，追踪到目标 */ \
		_zv = &ref->val; \
	} \
	/* 不用追踪引用，清空zv */ \
	zval_ptr_dtor(_zv); \
	/* 更新值为 小数 */ \
	ZVAL_DOUBLE(_zv, dval); \
} while (0)

// ing4, 给引用目标赋值，值为 小数，不解引用, p2:传入值
#define ZEND_TRY_ASSIGN_DOUBLE(zv, dval) \
	_ZEND_TRY_ASSIGN_DOUBLE(zv, dval, 0)

// ing4, 给引用目标赋值，值为 小数，解引用, p2:传入值
#define ZEND_TRY_ASSIGN_REF_DOUBLE(zv, dval) do { \
	ZEND_ASSERT(Z_ISREF_P(zv)); \
	_ZEND_TRY_ASSIGN_DOUBLE(zv, dval, 1); \
} while (0)

// ing4, 给引用目标赋值，值为 空字串，p2:是否解引用
#define _ZEND_TRY_ASSIGN_EMPTY_STRING(zv, is_ref) do { \
	zval *_zv = zv; \
	/* 如果要引用赋值或 zv 是引用类型 */ \
	if (is_ref || UNEXPECTED(Z_ISREF_P(_zv))) { \
		/* 引用实例指针 */ \
		zend_reference *ref = Z_REF_P(_zv); \
		/* 如果 检验引用目标是否有效 */ \
		if (UNEXPECTED(ZEND_REF_HAS_TYPE_SOURCES(ref))) { \
			/* 给引用目标赋值，值为 空字串 */ \
			zend_try_assign_typed_ref_empty_string(ref); \
			break; \
		} \
		/* 没有类型，追踪到目标 */ \
		_zv = &ref->val; \
	} \
	/* 不用追踪引用，清空zv */ \
	zval_ptr_dtor(_zv); \
	/* 更新值为 空字串 */ \
	ZVAL_EMPTY_STRING(_zv); \
} while (0)

// ing4, 给引用目标赋值，值为 空字串，不解引用
#define ZEND_TRY_ASSIGN_EMPTY_STRING(zv) \
	_ZEND_TRY_ASSIGN_EMPTY_STRING(zv, 0)

// ing4, 给引用目标赋值，值为 空字串，解引用
#define ZEND_TRY_ASSIGN_REF_EMPTY_STRING(zv) do { \
	ZEND_ASSERT(Z_ISREF_P(zv)); \
	_ZEND_TRY_ASSIGN_EMPTY_STRING(zv, 1); \
} while (0)

// ing4, 给引用目标赋值，值为 zend_string, 支持保留字, p2:传入值，p3:是否解引用
#define _ZEND_TRY_ASSIGN_STR(zv, str, is_ref) do { \
	zval *_zv = zv; \
	/* 如果要引用赋值或 zv 是引用类型 */ \
	if (is_ref || UNEXPECTED(Z_ISREF_P(_zv))) { \
		/* 引用实例指针 */ \
		zend_reference *ref = Z_REF_P(_zv); \
		/* 如果 检验引用目标是否有效 */ \
		if (UNEXPECTED(ZEND_REF_HAS_TYPE_SOURCES(ref))) { \
			/* 给引用目标赋值，值为 zend_string */ \
			zend_try_assign_typed_ref_str(ref, str); \
			break; \
		} \
		/* 没有类型，追踪到目标 */ \
		_zv = &ref->val; \
	} \
	/* 不用追踪引用，清空zv */ \
	zval_ptr_dtor(_zv); \
	/*  给zval.value.str 赋值 ， 并设置type_info : IS_INTERNED_STRING_EX 或 IS_STRING_EX。支持保留字 */ \
	ZVAL_STR(_zv, str); \
} while (0)

// ing4, 给引用目标赋值，值为 zend_string, 支持保留字，不解引用, p2:传入值
#define ZEND_TRY_ASSIGN_STR(zv, str) \
	_ZEND_TRY_ASSIGN_STR(zv, str, 0)

// ing4, 给引用目标赋值，值为 zend_string, 支持保留字，解引用, p2:传入值
#define ZEND_TRY_ASSIGN_REF_STR(zv, str) do { \
	ZEND_ASSERT(Z_ISREF_P(zv)); \
	_ZEND_TRY_ASSIGN_STR(zv, str, 1); \
} while (0)

// ing4, 给引用目标赋值，值为 zend_string, 不支持保留字, p2:传入值，p3:是否解引用
#define _ZEND_TRY_ASSIGN_NEW_STR(zv, str, is_str) do { \
	zval *_zv = zv; \
	/* 如果要引用赋值或 zv 是引用类型 */ \
	if (is_str || UNEXPECTED(Z_ISREF_P(_zv))) { \
		/* 引用实例指针 */ \
		zend_reference *ref = Z_REF_P(_zv); \
		/* 如果 检验引用目标是否有效 */ \
		if (UNEXPECTED(ZEND_REF_HAS_TYPE_SOURCES(ref))) { \
			/* 给引用目标赋值，值为 zend_string */ \
			zend_try_assign_typed_ref_str(ref, str); \
			break; \
		} \
		/* 没有类型，追踪到目标 */ \
		_zv = &ref->val; \
	} \
	/* 不用追踪引用，清空zv */ \
	zval_ptr_dtor(_zv); \
	/* 把zend_string 添加给 zval，不支持保留字 */ \
	ZVAL_NEW_STR(_zv, str); \
} while (0)

// ing4, 给引用目标赋值，值为 zend_string, 不支持保留字，不解引用, p2:传入值
#define ZEND_TRY_ASSIGN_NEW_STR(zv, str) \
	_ZEND_TRY_ASSIGN_NEW_STR(zv, str, 0)

// ing4, 给引用目标赋值，值为 zend_string, 不支持保留字，解引用, p2:传入值
#define ZEND_TRY_ASSIGN_REF_NEW_STR(zv, str) do { \
	ZEND_ASSERT(Z_ISREF_P(zv)); \
	_ZEND_TRY_ASSIGN_NEW_STR(zv, str, 1); \
} while (0)

// ing4, 给引用目标赋值，值为 字串, p2:传入值，p3:是否解引用
#define _ZEND_TRY_ASSIGN_STRING(zv, string, is_ref) do { \
	zval *_zv = zv; \
	/* 如果要引用赋值或 zv 是引用类型 */ \
	if (is_ref || UNEXPECTED(Z_ISREF_P(_zv))) { \
		/* 引用实例指针 */ \
		zend_reference *ref = Z_REF_P(_zv); \
		/* 如果 检验引用目标是否有效 */ \
		if (UNEXPECTED(ZEND_REF_HAS_TYPE_SOURCES(ref))) { \
			/* 给引用目标赋值，值为 字串 */ \
			zend_try_assign_typed_ref_string(ref, string); \
			break; \
		} \
		/* 没有类型，追踪到目标 */ \
		_zv = &ref->val; \
	} \
	/* 不用追踪引用，清空zv */ \
	zval_ptr_dtor(_zv); \
	/* 更新值为 字串 */ \
	ZVAL_STRING(_zv, string); \
} while (0)

// ing4, 给引用目标赋值，值为 字串，不解引用, p2:传入值
#define ZEND_TRY_ASSIGN_STRING(zv, string) \
	_ZEND_TRY_ASSIGN_STRING(zv, string, 0)

// ing4, 给引用目标赋值，值为 字串，解引用, p2:传入值
#define ZEND_TRY_ASSIGN_REF_STRING(zv, string) do { \
	ZEND_ASSERT(Z_ISREF_P(zv)); \
	_ZEND_TRY_ASSIGN_STRING(zv, string, 1); \
} while (0)

// ing4, 给引用目标赋值，值为 字串, p2,p3:传入值，p4:是否解引用
#define _ZEND_TRY_ASSIGN_STRINGL(zv, string, len, is_ref) do { \
	zval *_zv = zv; \
	/* 如果要引用赋值或 zv 是引用类型 */ \
	if (is_ref || UNEXPECTED(Z_ISREF_P(_zv))) { \
		/* 引用实例指针 */ \
		zend_reference *ref = Z_REF_P(_zv); \
		/* 如果 检验引用目标是否有效 */ \
		if (UNEXPECTED(ZEND_REF_HAS_TYPE_SOURCES(ref))) { \
			/* 给引用目标赋值，值为 字串和长度 */ \
			zend_try_assign_typed_ref_stringl(ref, string, len); \
			break; \
		} \
		/* 没有类型，追踪到目标 */ \
		_zv = &ref->val; \
	} \
	/* 不用追踪引用，清空zv */ \
	zval_ptr_dtor(_zv); \
	/* 更新值为 字串和长度 */ \
	ZVAL_STRINGL(_zv, string, len); \
} while (0)

// ing4, 给引用目标赋值，值为 字串，不解引用, p2,p3:传入值
#define ZEND_TRY_ASSIGN_STRINGL(zv, string, len) \
	_ZEND_TRY_ASSIGN_STRINGL(zv, string, len, 0)

// ing4, 给引用目标赋值，值为 字串，解引用, p2,p3:传入值
#define ZEND_TRY_ASSIGN_REF_STRINGL(zv, string, len) do { \
	ZEND_ASSERT(Z_ISREF_P(zv)); \
	_ZEND_TRY_ASSIGN_STRINGL(zv, string, len, 1); \
} while (0)

// ing4, 给引用目标赋值，值为 数组, p2:传入值，p3:是否解引用
#define _ZEND_TRY_ASSIGN_ARR(zv, arr, is_ref) do { \
	zval *_zv = zv; \
	/* 如果要引用赋值或 zv 是引用类型 */ \
	if (is_ref || UNEXPECTED(Z_ISREF_P(_zv))) { \
		/* 引用实例指针 */ \
		zend_reference *ref = Z_REF_P(_zv); \
		/* 如果 检验引用目标是否有效 */ \
		if (UNEXPECTED(ZEND_REF_HAS_TYPE_SOURCES(ref))) { \
			/* 给引用目标赋值，值为 数组 */ \
			zend_try_assign_typed_ref_arr(ref, arr); \
			break; \
		} \
		/* 没有类型，追踪到目标 */ \
		_zv = &ref->val; \
	} \
	/* 不用追踪引用，清空zv */ \
	zval_ptr_dtor(_zv); \
	/* 更新值为 数组 */ \
	ZVAL_ARR(_zv, arr); \
} while (0)

// ing4, 给引用目标赋值，值为 数组，不解引用, p2:传入值
#define ZEND_TRY_ASSIGN_ARR(zv, arr) \
	_ZEND_TRY_ASSIGN_ARR(zv, arr, 0)

// ing4, 给引用目标赋值，值为 数组，解引用, p2:传入值
#define ZEND_TRY_ASSIGN_REF_ARR(zv, arr) do { \
	ZEND_ASSERT(Z_ISREF_P(zv)); \
	_ZEND_TRY_ASSIGN_ARR(zv, arr, 1); \
} while (0)

// ing4, 给引用目标赋值，值为 资源, p2:传入值，p3:是否解引用
#define _ZEND_TRY_ASSIGN_RES(zv, res, is_ref) do { \
	zval *_zv = zv; \
	/* 如果要引用赋值或 zv 是引用类型 */ \
	if (is_ref || UNEXPECTED(Z_ISREF_P(_zv))) { \
		/* 引用实例指针 */ \
		zend_reference *ref = Z_REF_P(_zv); \
		/* 如果 检验引用目标是否有效 */ \
		if (UNEXPECTED(ZEND_REF_HAS_TYPE_SOURCES(ref))) { \
			/* 给引用目标赋值，值为 资源 */ \
			zend_try_assign_typed_ref_res(ref, res); \
			break; \
		} \
		/* 没有类型，追踪到目标 */ \
		_zv = &ref->val; \
	} \
	/* 不用追踪引用，清空zv */ \
	zval_ptr_dtor(_zv); \
	/* 更新值为 资源 */ \
	ZVAL_RES(_zv, res); \
} while (0)

// ing4, 给引用目标赋值，值为 资源，不解引用，p2:传入值 
#define ZEND_TRY_ASSIGN_RES(zv, res) \
	_ZEND_TRY_ASSIGN_RES(zv, res, 0)

// ing4, 给引用目标赋值，值为 资源，解引用，p2:传入值 
#define ZEND_TRY_ASSIGN_REF_RES(zv, res) do { \
	ZEND_ASSERT(Z_ISREF_P(zv)); \
	_ZEND_TRY_ASSIGN_RES(zv, res, 1); \
} while (0)

// ing4, 给引用目标赋值，值为 zval, p2:传入值，p3:是否解引用
#define _ZEND_TRY_ASSIGN_TMP(zv, other_zv, is_ref) do { \
	zval *_zv = zv; \
	/* 如果要引用赋值或 zv 是引用类型 */ \
	if (is_ref || UNEXPECTED(Z_ISREF_P(_zv))) { \
		/* 引用实例指针 */ \
		zend_reference *ref = Z_REF_P(_zv); \
		/* 如果 检验引用目标是否有效 */ \
		if (UNEXPECTED(ZEND_REF_HAS_TYPE_SOURCES(ref))) { \
			/*  如果通过检测，把值赋给 ref的附加zval */ \
			zend_try_assign_typed_ref(ref, other_zv); \
			break; \
		} \
		/* 没有类型，追踪到目标 */ \
		_zv = &ref->val; \
	} \
	/* 不用追踪引用，清空zv */ \
	zval_ptr_dtor(_zv); \
	/* 更新值为 zval */ \
	ZVAL_COPY_VALUE(_zv, other_zv); \
} while (0)

// ing4, 给引用目标赋值，值为 zval，不解引用, p2:传入值
#define ZEND_TRY_ASSIGN_TMP(zv, other_zv) \
	_ZEND_TRY_ASSIGN_TMP(zv, other_zv, 0)

// ing4, 给引用目标赋值，值为 zval，解引用, p2:传入值
#define ZEND_TRY_ASSIGN_REF_TMP(zv, other_zv) do { \
	ZEND_ASSERT(Z_ISREF_P(zv)); \
	_ZEND_TRY_ASSIGN_TMP(zv, other_zv, 1); \
} while (0)

// ing4, 给引用目标赋值，值为 zval, p2:传入值，p3:是否解引用
#define _ZEND_TRY_ASSIGN_VALUE(zv, other_zv, is_ref) do { \
	zval *_zv = zv; \
	/* 如果要引用赋值或 zv 是引用类型 */ \
	if (is_ref || UNEXPECTED(Z_ISREF_P(_zv))) { \
		/* 引用实例指针 */ \
		zend_reference *ref = Z_REF_P(_zv); \
		/* 如果 检验引用目标是否有效 */ \
		if (UNEXPECTED(ZEND_REF_HAS_TYPE_SOURCES(ref))) { \
			/* 给引用目标赋值，值为 zval */ \
			zend_try_assign_typed_ref_zval(ref, other_zv); \
			break; \
		} \
		/* 没有类型，追踪到目标 */ \
		_zv = &ref->val; \
	} \
	/* 不用追踪引用，清空zv */ \
	zval_ptr_dtor(_zv); \
	/* 更新值为 zval */ \
	ZVAL_COPY_VALUE(_zv, other_zv); \
} while (0)

// ing4, 给引用目标赋值，值为 zval，不解引用, p2:传入值
#define ZEND_TRY_ASSIGN_VALUE(zv, other_zv) \
	_ZEND_TRY_ASSIGN_VALUE(zv, other_zv, 0)

// ing4, 给引用目标赋值，值为 zval，解引用, p2:传入值
#define ZEND_TRY_ASSIGN_REF_VALUE(zv, other_zv) do { \
	ZEND_ASSERT(Z_ISREF_P(zv)); \
	_ZEND_TRY_ASSIGN_VALUE(zv, other_zv, 1); \
} while (0)

// ing4, 给引用目标 增加引用次数并赋值，值为 zval，不解引用, p2:传入值
#define ZEND_TRY_ASSIGN_COPY(zv, other_zv) do { \
	Z_TRY_ADDREF_P(other_zv); \
	ZEND_TRY_ASSIGN_VALUE(zv, other_zv); \
} while (0)

// ing4, 给引用目标 增加引用次数并赋值，值为 zval，解引用, p2:传入值
#define ZEND_TRY_ASSIGN_REF_COPY(zv, other_zv) do { \
	Z_TRY_ADDREF_P(other_zv); \
	ZEND_TRY_ASSIGN_REF_VALUE(zv, other_zv); \
} while (0)

// ing4, 给引用目标赋值，值为 zval, p2:传入值，p3:是否严格类型，p4:是否解引用
#define _ZEND_TRY_ASSIGN_VALUE_EX(zv, other_zv, strict, is_ref) do { \
	zval *_zv = zv; \
	/* 如果要引用赋值或 zv 是引用类型 */ \
	if (is_ref || UNEXPECTED(Z_ISREF_P(_zv))) { \
		/* 引用实例指针 */ \
		zend_reference *ref = Z_REF_P(_zv); \
		/* 如果 检验引用目标是否有效 */ \
		if (UNEXPECTED(ZEND_REF_HAS_TYPE_SOURCES(ref))) { \
			/* 给引用目标赋值，值为 zval, p3:是否严格控制类型 */ \
			zend_try_assign_typed_ref_zval_ex(ref, other_zv, strict); \
			break; \
		} \
		/* 没有类型，追踪到目标 */ \
		_zv = &ref->val; \
	} \
	/* 不用追踪引用，清空zv */ \
	zval_ptr_dtor(_zv); \
	/* 更新值为 zval */ \
	ZVAL_COPY_VALUE(_zv, other_zv); \
} while (0)

// ing4, 给引用目标赋值，值为 zval，不解引用, p2:传入值，p3:是否严格类型
#define ZEND_TRY_ASSIGN_VALUE_EX(zv, other_zv, strict) \
	_ZEND_TRY_ASSIGN_VALUE_EX(zv, other_zv, strict, 0)

// ing4, 给引用目标赋值，值为 zval，解引用, p2:传入值，p3:是否严格类型
#define ZEND_TRY_ASSIGN_REF_VALUE_EX(zv, other_zv, strict) do { \
	ZEND_ASSERT(Z_ISREF_P(zv)); \
	_ZEND_TRY_ASSIGN_VALUE_EX(zv, other_zv, strict, 1); \
} while (0)

// ing4, 给引用目标 增加引用次数并赋值，值为 zval，不解引用, p2:传入值，p3:是否严格类型
#define ZEND_TRY_ASSIGN_COPY_EX(zv, other_zv, strict) do { \
	Z_TRY_ADDREF_P(other_zv); \
	ZEND_TRY_ASSIGN_VALUE_EX(zv, other_zv, strict); \
} while (0)

// ing4, 给引用目标 增加引用次数并赋值，值为 zval，解引用, p2:传入值，p3:是否严格类型
#define ZEND_TRY_ASSIGN_REF_COPY_EX(zv, other_zv, strict) do { \
	Z_TRY_ADDREF_P(other_zv); \
	ZEND_TRY_ASSIGN_REF_VALUE_EX(zv, other_zv, strict); \
} while (0)

// ing3, 初始化一个指向空哈希表的引用 并且返回 解引用的 zval ，如果初始化失败，返回null。p1:接收变量，p2:大小
/* Initializes a reference to an empty array and returns dereferenced zval,
 * or NULL if the initialization failed. */
static zend_always_inline zval *zend_try_array_init_size(zval *zv, uint32_t size)
{
	// 创建新哈希表
	zend_array *arr = zend_new_array(size);

	// 如果zv是引用类型
	if (EXPECTED(Z_ISREF_P(zv))) {
		// 取出引用实例
		zend_reference *ref = Z_REF_P(zv);
		// 如果 检验引用目标是否有效 
		if (UNEXPECTED(ZEND_REF_HAS_TYPE_SOURCES(ref))) {
			// 给引用目标赋值，值为 数组
			if (zend_try_assign_typed_ref_arr(ref, arr) == FAILURE) {
				// 失败返回null
				return NULL;
			}
			// 返回引用实例中的 zval
			return &ref->val;
		}
		// 解引用
		zv = &ref->val;
	}
	// 清空zv
	zval_ptr_dtor(zv);
	// 哈希表放入 zv中
	ZVAL_ARR(zv, arr);
	// 返回zv
	return zv;
}

// ing3, 初始化一个指向 大小为0空哈希表的引用 并且返回 解引用的 zval ，如果初始化失败，返回null。p1:接收变量
static zend_always_inline zval *zend_try_array_init(zval *zv)
{
	// 初始化一个指向空哈希表的引用 并且返回 解引用的 zval ，如果初始化失败，返回null。p1:接收变量，p2:大小
	return zend_try_array_init_size(zv, 0);
}

/* Fast parameter parsing API */

/* Fast ZPP is always enabled now; this define is left in for compatibility
 * with any existing conditional compilation blocks.
 */
#define FAST_ZPP 1

// ing3, 常用类型对象的 英文名称
#define Z_EXPECTED_TYPES(_) \
	_(Z_EXPECTED_LONG,				"of type int") \
	_(Z_EXPECTED_LONG_OR_NULL,		"of type ?int") \
	_(Z_EXPECTED_BOOL,				"of type bool") \
	_(Z_EXPECTED_BOOL_OR_NULL,		"of type ?bool") \
	_(Z_EXPECTED_STRING,			"of type string") \
	_(Z_EXPECTED_STRING_OR_NULL,	"of type ?string") \
	_(Z_EXPECTED_ARRAY,				"of type array") \
	_(Z_EXPECTED_ARRAY_OR_NULL,		"of type ?array") \
	_(Z_EXPECTED_ARRAY_OR_LONG,		"of type array|int") \
	_(Z_EXPECTED_ARRAY_OR_LONG_OR_NULL, "of type array|int|null") \
	_(Z_EXPECTED_ITERABLE,				"of type Traversable|array") \
	_(Z_EXPECTED_ITERABLE_OR_NULL,		"of type Traversable|array|null") \
	_(Z_EXPECTED_FUNC,				"a valid callback") \
	_(Z_EXPECTED_FUNC_OR_NULL,		"a valid callback or null") \
	_(Z_EXPECTED_RESOURCE,			"of type resource") \
	_(Z_EXPECTED_RESOURCE_OR_NULL,	"of type resource or null") \
	_(Z_EXPECTED_PATH,				"of type string") \
	_(Z_EXPECTED_PATH_OR_NULL,		"of type ?string") \
	_(Z_EXPECTED_OBJECT,			"of type object") \
	_(Z_EXPECTED_OBJECT_OR_NULL,	"of type ?object") \
	_(Z_EXPECTED_DOUBLE,			"of type float") \
	_(Z_EXPECTED_DOUBLE_OR_NULL,	"of type ?float") \
	_(Z_EXPECTED_NUMBER,			"of type int|float") \
	_(Z_EXPECTED_NUMBER_OR_NULL,	"of type int|float|null") \
	_(Z_EXPECTED_ARRAY_OR_STRING,	"of type array|string") \
	_(Z_EXPECTED_ARRAY_OR_STRING_OR_NULL, "of type array|string|null") \
	_(Z_EXPECTED_STRING_OR_LONG,	"of type string|int") \
	_(Z_EXPECTED_STRING_OR_LONG_OR_NULL, "of type string|int|null") \
	_(Z_EXPECTED_OBJECT_OR_CLASS_NAME,	"an object or a valid class name") \
	_(Z_EXPECTED_OBJECT_OR_CLASS_NAME_OR_NULL, "an object, a valid class name, or null") \
	_(Z_EXPECTED_OBJECT_OR_STRING,	"of type object|string") \
	_(Z_EXPECTED_OBJECT_OR_STRING_OR_NULL, "of type object|string|null") \

#define Z_EXPECTED_TYPE

// ing4, 只保留类型的 id
#define Z_EXPECTED_TYPE_ENUM(id, str) id,
// ing4, 只保留类型的英文名
#define Z_EXPECTED_TYPE_STR(id, str)  str,

// ing4, 所有的常用类型列表（无英文名）
typedef enum _zend_expected_type {
	Z_EXPECTED_TYPES(Z_EXPECTED_TYPE_ENUM)
	Z_EXPECTED_LAST
} zend_expected_type;

ZEND_API ZEND_COLD void ZEND_FASTCALL zend_wrong_parameters_none_error(void);
ZEND_API ZEND_COLD void ZEND_FASTCALL zend_wrong_parameters_count_error(uint32_t min_num_args, uint32_t max_num_args);
ZEND_API ZEND_COLD void ZEND_FASTCALL zend_wrong_parameter_error(int error_code, uint32_t num, char *name, zend_expected_type expected_type, zval *arg);
ZEND_API ZEND_COLD void ZEND_FASTCALL zend_wrong_parameter_type_error(uint32_t num, zend_expected_type expected_type, zval *arg);
ZEND_API ZEND_COLD void ZEND_FASTCALL zend_wrong_parameter_class_error(uint32_t num, const char *name, zval *arg);
ZEND_API ZEND_COLD void ZEND_FASTCALL zend_wrong_parameter_class_or_null_error(uint32_t num, const char *name, zval *arg);
ZEND_API ZEND_COLD void ZEND_FASTCALL zend_wrong_parameter_class_or_long_error(uint32_t num, const char *name, zval *arg);
ZEND_API ZEND_COLD void ZEND_FASTCALL zend_wrong_parameter_class_or_long_or_null_error(uint32_t num, const char *name, zval *arg);
ZEND_API ZEND_COLD void ZEND_FASTCALL zend_wrong_parameter_class_or_string_error(uint32_t num, const char *name, zval *arg);
ZEND_API ZEND_COLD void ZEND_FASTCALL zend_wrong_parameter_class_or_string_or_null_error(uint32_t num, const char *name, zval *arg);
ZEND_API ZEND_COLD void ZEND_FASTCALL zend_wrong_callback_error(uint32_t num, char *error);
ZEND_API ZEND_COLD void ZEND_FASTCALL zend_wrong_callback_or_null_error(uint32_t num, char *error);
ZEND_API ZEND_COLD void ZEND_FASTCALL zend_unexpected_extra_named_error(void);
ZEND_API ZEND_COLD void ZEND_FASTCALL zend_argument_error_variadic(zend_class_entry *error_ce, uint32_t arg_num, const char *format, va_list va);
ZEND_API ZEND_COLD void zend_argument_error(zend_class_entry *error_ce, uint32_t arg_num, const char *format, ...);
ZEND_API ZEND_COLD void zend_argument_type_error(uint32_t arg_num, const char *format, ...);
ZEND_API ZEND_COLD void zend_argument_value_error(uint32_t arg_num, const char *format, ...);

#define ZPP_ERROR_OK                            0
#define ZPP_ERROR_FAILURE                       1
#define ZPP_ERROR_WRONG_CALLBACK                2
#define ZPP_ERROR_WRONG_CLASS                   3
#define ZPP_ERROR_WRONG_CLASS_OR_NULL           4
#define ZPP_ERROR_WRONG_CLASS_OR_STRING         5
#define ZPP_ERROR_WRONG_CLASS_OR_STRING_OR_NULL 6
#define ZPP_ERROR_WRONG_CLASS_OR_LONG           7
#define ZPP_ERROR_WRONG_CLASS_OR_LONG_OR_NULL   8
#define ZPP_ERROR_WRONG_ARG                     9
#define ZPP_ERROR_WRONG_COUNT                   10
#define ZPP_ERROR_UNEXPECTED_EXTRA_NAMED        11
#define ZPP_ERROR_WRONG_CALLBACK_OR_NULL        12

// ing2, 验证参数数量是否合法并取出参数列表, 传入：flag，最小数量，最大数量 
#define ZEND_PARSE_PARAMETERS_START_EX(flags, min_num_args, max_num_args) do { \
		/* 3个宏参数 */ \
		const int _flags = (flags); \
		uint32_t _min_num_args = (min_num_args); \
		uint32_t _max_num_args = (uint32_t) (max_num_args); \
		/* 参数数量 */ \
		uint32_t _num_args = EX_NUM_ARGS(); \
		/* 已解析参数数量 */ \
		uint32_t _i = 0; \
		/* 真实参数列表指针， ？ */ \
		zval *_real_arg, *_arg = NULL; \
		/* 期望类型，默认是整数 */ \
		zend_expected_type _expected_type = Z_EXPECTED_LONG; \
		/* 错误信息 */ \
		char *_error = NULL; \
		/* 临时变量 */ \
		bool _dummy = 0; \
		/* 是否可选参数 */ \
		bool _optional = 0; \
		int _error_code = ZPP_ERROR_OK; \
		/* 这是在干嘛 */ \
		((void)_i); \
		((void)_real_arg); \
		((void)_arg); \
		((void)_expected_type); \
		((void)_error); \
		((void)_optional); \
		((void)_dummy); \
		\
		do { \
			/* 如果小于最少参数数量 或 大于最大参数数量  */ \
			if (UNEXPECTED(_num_args < _min_num_args) || \
			    UNEXPECTED(_num_args > _max_num_args)) { \
				/* 如果需要退出 */ \
				if (!(_flags & ZEND_PARSE_PARAMS_QUIET)) { \
					/* 报错：参数数量错误 */ \
					zend_wrong_parameters_count_error(_min_num_args, _max_num_args); \
				} \
				/* 错误码 */ \
				_error_code = ZPP_ERROR_FAILURE; \
				break; \
			} \
			/* 取出参数列表 */ \
			_real_arg = ZEND_CALL_ARG(execute_data, 0);

// ing3, 验证参数数量是否合法并取出参数列表, 传入：最小数量，最大数量 
#define ZEND_PARSE_PARAMETERS_START(min_num_args, max_num_args) \
	ZEND_PARSE_PARAMETERS_START_EX(0, min_num_args, max_num_args)

// ing4, 不可以有传入参数，有就报错
#define ZEND_PARSE_PARAMETERS_NONE() do { \
		/* 如果有传入参数 */ \
		if (UNEXPECTED(ZEND_NUM_ARGS() != 0)) { \
			/* 报错 */ \
			zend_wrong_parameters_none_error(); \
			return; \
		} \
	} while (0)

// ing3, 获取参数的 最后验证，failure 出错时执行的语句。
#define ZEND_PARSE_PARAMETERS_END_EX(failure) \
			/* 如果有最大参数数，必须与最大参数数相等 */ \
			ZEND_ASSERT(_i == _max_num_args || _max_num_args == (uint32_t) -1); \
		} while (0); \
		/* 如果获取参数过程中出错 */ \
		if (UNEXPECTED(_error_code != ZPP_ERROR_OK)) { \
			/* 如果不静默 */ \
			if (!(_flags & ZEND_PARSE_PARAMS_QUIET)) { \
				/* 报错 */ \
				zend_wrong_parameter_error(_error_code, _i, _error, _expected_type, _arg); \
			} \
			failure; \
		} \
	} while (0)

// ing4, 获取参数的 最后验证，出错时 return返回。
#define ZEND_PARSE_PARAMETERS_END() \
	ZEND_PARSE_PARAMETERS_END_EX(return)

// ing3, 获取下一个实际参数，p1:是否解引用，p2:是否创建副本
#define Z_PARAM_PROLOGUE(deref, separate) \
	++_i; \
	/* 小于等于最小数量 或 可选 */ \
	ZEND_ASSERT(_i <= _min_num_args || _optional==1); \
	/* 大于最小数量 或 必选 */ \
	ZEND_ASSERT(_i >  _min_num_args || _optional==0); \
	/* 如果可选 */ \
	if (_optional) { \
		/* 并且数量大于参数数量，跳出 */ \
		if (UNEXPECTED(_i >_num_args)) break; \
	} \
	/* 实际参数指针后移 */ \
	_real_arg++; \
	/* 当前实际参数 */ \
	_arg = _real_arg; \
	/* 如果需要解引用 */ \
	if (deref) { \
		/* 确实是引用 */ \
		if (EXPECTED(Z_ISREF_P(_arg))) { \
			/* 追踪到目标 */ \
			_arg = Z_REFVAL_P(_arg); \
		} \
	} \
	/* 如果需要分割版本 */ \
	if (separate) { \
		/* 如果是数组，按需要创建副本，其他类型不操作。 */ \
		SEPARATE_ZVAL_NOREF(_arg); \
	}

// 取得 前面解析过的参数的 zval指针 
/* get the zval* for a previously parsed argument */
// ing3, 返回 _arg。p1:接收返回值
#define Z_PARAM_GET_PREV_ZVAL(dest) \
	/* 如果要检验null并且 p1 是null，返回null.否则返回 p1.p2:返回值，p3:允null */ \
	zend_parse_arg_zval_deref(_arg, &dest, 0);

/* old "|" */
// ing4, 可选标记
#define Z_PARAM_OPTIONAL \
	_optional = 1;

/* old "a" */
// ing3, 实际参数解析成数组（不允许对象），p1:返回值，p2:是否允null，p3:是否解引用，p4:是否创建副本
#define Z_PARAM_ARRAY_EX2(dest, check_null, deref, separate) \
		/* 获取下一个实际参数，p1:是否解引用，p2:是否创建副本 */ \
		Z_PARAM_PROLOGUE(deref, separate); \
		/* 实际参数解析成数组或对象，p2:返回值，p3:是否允null，p4:是否允许对象 */ \
		if (UNEXPECTED(!zend_parse_arg_array(_arg, &dest, check_null, 0))) { \
			_expected_type = check_null ? Z_EXPECTED_ARRAY_OR_NULL : Z_EXPECTED_ARRAY; \
			_error_code = ZPP_ERROR_WRONG_ARG; \
			break; \
		}

// ing3, 实际参数解析成数组（不允许对象），p1:返回值，p2:是否允null，p3:是否解引用 和 创建副本
#define Z_PARAM_ARRAY_EX(dest, check_null, separate) \
	Z_PARAM_ARRAY_EX2(dest, check_null, separate, separate)

// ing3, 实际参数解析成数组（不允许对象），不允null，不解引用和创建副本，p1:返回值
#define Z_PARAM_ARRAY(dest) \
	Z_PARAM_ARRAY_EX(dest, 0, 0)
// ing3, 实际参数解析成数组（不允许对象），允null，不解引用和创建副本，p1:返回值
#define Z_PARAM_ARRAY_OR_NULL(dest) \
	Z_PARAM_ARRAY_EX(dest, 1, 0)

/* old "A" */
// ing3, 实际参数解析成数组或对象，p1:返回值，p2:是否允null，p3:是否解引用，p4:是否创建副本
#define Z_PARAM_ARRAY_OR_OBJECT_EX2(dest, check_null, deref, separate) \
		/* 获取下一个实际参数，p1:是否解引用，p2:是否创建副本 */ \
		Z_PARAM_PROLOGUE(deref, separate); \
		/* 实际参数解析成数组或对象，p2:返回值，p3:是否允null，p4:是否允许对象 */ \
		if (UNEXPECTED(!zend_parse_arg_array(_arg, &dest, check_null, 1))) { \
			_expected_type = check_null ? Z_EXPECTED_ARRAY_OR_NULL : Z_EXPECTED_ARRAY; \
			_error_code = ZPP_ERROR_WRONG_ARG; \
			break; \
		}

// ing3, 实际参数解析成数组或对象，p1:返回值，p2:是否允null，p3:是否解引用 和 创建副本
#define Z_PARAM_ARRAY_OR_OBJECT_EX(dest, check_null, separate) \
	Z_PARAM_ARRAY_OR_OBJECT_EX2(dest, check_null, separate, separate)

// ing3, 实际参数解析成数组或对象，不允null，不解引用和创建副本，p1:返回值
#define Z_PARAM_ARRAY_OR_OBJECT(dest) \
	Z_PARAM_ARRAY_OR_OBJECT_EX(dest, 0, 0)

// ing3, 实际参数解析成迭代器，p1:返回值，p2:是否允null
#define Z_PARAM_ITERABLE_EX(dest, check_null) \
	/* 获取下一个实际参数，p1:是否解引用，p2:是否创建副本 */ \
	Z_PARAM_PROLOGUE(0, 0); \
	/* 实际参数解析成迭代器，p2:返回值，p3:是否允null */ \
	if (UNEXPECTED(!zend_parse_arg_iterable(_arg, &dest, check_null))) { \
		_expected_type = check_null ? Z_EXPECTED_ITERABLE_OR_NULL : Z_EXPECTED_ITERABLE; \
		_error_code = ZPP_ERROR_WRONG_ARG; \
		break; \
	}

// ing3, 实际参数解析成迭代器，不允null，p1:返回值
#define Z_PARAM_ITERABLE(dest) \
	Z_PARAM_ITERABLE_EX(dest, 0)

// ing3, 实际参数解析成迭代器，允null，p1:返回值
#define Z_PARAM_ITERABLE_OR_NULL(dest) \
	Z_PARAM_ITERABLE_EX(dest, 1)

/* old "b" */
// ing3, 把参数解析成布尔值，p1:返回值，p2:是否允null，p3:是否检查null，p4:是否解引用
#define Z_PARAM_BOOL_EX(dest, is_null, check_null, deref) \
		/* 获取下一个实际参数，p1:是否解引用，p2:是否创建副本 */ \
		Z_PARAM_PROLOGUE(deref, 0); \
		/* 把参数解析成布尔值，p2:返回值，p3:是否允null，p4:是否检查null，p5:arg参数序号，报错用 */ \
		if (UNEXPECTED(!zend_parse_arg_bool(_arg, &dest, &is_null, check_null, _i))) { \
			_expected_type = check_null ? Z_EXPECTED_BOOL_OR_NULL : Z_EXPECTED_BOOL; \
			_error_code = ZPP_ERROR_WRONG_ARG; \
			break; \
		}

// ing3, 把参数解析成布尔值，不检查null，不解引用，p1:返回值，_dummy:是否允null
#define Z_PARAM_BOOL(dest) \
	Z_PARAM_BOOL_EX(dest, _dummy, 0, 0)

// ing3, 把参数解析成布尔值，允null，不解引用，p1:返回值，p2:是否允null
#define Z_PARAM_BOOL_OR_NULL(dest, is_null) \
	Z_PARAM_BOOL_EX(dest, is_null, 1, 0)

/* old "C" */
// ing3, 把参数（字串）解析成类入口，p1:要求的基类，也是返回值，p2:是否检查 null，p3:是否解引用
#define Z_PARAM_CLASS_EX(dest, check_null, deref) \
		/* 获取下一个实际参数，p1:是否解引用，p2:是否创建副本 */ \
		Z_PARAM_PROLOGUE(deref, 0); \
		/* 把参数（字串）解析成类入口，p2:要求的基类，也是返回值，p3:arg参数序号，报错用，p4:是否检查 null */ \
		if (UNEXPECTED(!zend_parse_arg_class(_arg, &dest, _i, check_null))) { \
			_error_code = ZPP_ERROR_FAILURE; \
			break; \
		}

// ing3, 把参数（字串）解析成类入口，不检查null，不解引用，p1:要求的基类，也是返回值
#define Z_PARAM_CLASS(dest) \
	Z_PARAM_CLASS_EX(dest, 0, 0)

// ing3, 把参数（字串）解析成类入口，检查null，不解引用，p1:要求的基类，也是返回值
#define Z_PARAM_CLASS_OR_NULL(dest) \
	Z_PARAM_CLASS_EX(dest, 1, 0)

// ing3, 把传入的参数（类名或对象）转换成类入口, p1:返回值，p2:是否允null
#define Z_PARAM_OBJ_OR_CLASS_NAME_EX(dest, allow_null) \
	/* 获取下一个实际参数，p1:是否解引用，p2:是否创建副本 */ \
	Z_PARAM_PROLOGUE(0, 0); \
	/* 把传入的参数（类名或对象）转换成类入口, p2:返回值，p3:是否允null */ \
	if (UNEXPECTED(!zend_parse_arg_obj_or_class_name(_arg, &dest, allow_null))) { \
		_expected_type = allow_null ? Z_EXPECTED_OBJECT_OR_CLASS_NAME_OR_NULL : Z_EXPECTED_OBJECT_OR_CLASS_NAME; \
		_error_code = ZPP_ERROR_WRONG_ARG; \
		break; \
	}

// ing3, 把传入的参数（类名或对象）转换成类入口,不允null, p1:返回值
#define Z_PARAM_OBJ_OR_CLASS_NAME(dest) \
	Z_PARAM_OBJ_OR_CLASS_NAME_EX(dest, 0);

// ing3, 把传入的参数（类名或对象）转换成类入口,允null, p1:返回值
#define Z_PARAM_OBJ_OR_CLASS_NAME_OR_NULL(dest) \
	Z_PARAM_OBJ_OR_CLASS_NAME_EX(dest, 1);

// ing3, 把传入的参数转换成对象（不限所属类）或字串, p1:返回值，p2:返回字串，p3:是否允null
#define Z_PARAM_OBJ_OR_STR_EX(destination_object, destination_string, allow_null) \
	/* 获取下一个实际参数，p1:是否解引用，p2:是否创建副本 */ \
	Z_PARAM_PROLOGUE(0, 0); \
	/* 把传入的参数转换成对象或字串, p2:返回值，p3:要求的类，p4:返回字串，p5:是否允null，p6:arg参数序号，报错用 */ \
	if (UNEXPECTED(!zend_parse_arg_obj_or_str(_arg, &destination_object, NULL, &destination_string, allow_null, _i))) { \
		_expected_type = allow_null ? Z_EXPECTED_OBJECT_OR_STRING_OR_NULL : Z_EXPECTED_OBJECT_OR_STRING; \
		_error_code = ZPP_ERROR_WRONG_ARG; \
		break; \
	}

// ing3, 把传入的参数转换成对象（不限所属类）或字串，不允null, p1:返回值，p2:返回字串
#define Z_PARAM_OBJ_OR_STR(destination_object, destination_string) \
	Z_PARAM_OBJ_OR_STR_EX(destination_object, destination_string, 0);

// ing3, 把传入的参数转换成对象（不限所属类）或字串，允null, p1:返回值，p2:返回字串
#define Z_PARAM_OBJ_OR_STR_OR_NULL(destination_object, destination_string) \
	Z_PARAM_OBJ_OR_STR_EX(destination_object, destination_string, 1);

// ing3, 把传入的参数转换成对象或字串, p1:返回值，p2:要求的类，p3:返回字串，p4:是否允null
#define Z_PARAM_OBJ_OF_CLASS_OR_STR_EX(destination_object, base_ce, destination_string, allow_null) \
	/* 获取下一个实际参数，p1:是否解引用，p2:是否创建副本 */ \
	Z_PARAM_PROLOGUE(0, 0); \
	/* 把传入的参数转换成对象或字串, p2:返回值，p3:要求的类，p4:返回字串，p5:是否允null，p6:arg参数序号，报错用 */ \
	if (UNEXPECTED(!zend_parse_arg_obj_or_str(_arg, &destination_object, base_ce, &destination_string, allow_null, _i))) { \
		if (base_ce) { \
			_error = ZSTR_VAL((base_ce)->name); \
			_error_code = allow_null ? ZPP_ERROR_WRONG_CLASS_OR_STRING_OR_NULL : ZPP_ERROR_WRONG_CLASS_OR_STRING; \
			break; \
		} else { \
			_expected_type = allow_null ? Z_EXPECTED_OBJECT_OR_STRING_OR_NULL : Z_EXPECTED_OBJECT_OR_STRING; \
			_error_code = ZPP_ERROR_WRONG_ARG; \
			break; \
		} \
	}
	
// ing3, 把传入的参数转换成对象或字串,不允null, p1:返回值，p2:要求的类，p3:返回字串
#define Z_PARAM_OBJ_OF_CLASS_OR_STR(destination_object, base_ce, destination_string) \
	Z_PARAM_OBJ_OF_CLASS_OR_STR_EX(destination_object, base_ce, destination_string, 0);

// ing3, 把传入的参数转换成对象或字串,允null, p1:返回值，p2:要求的类，p3:返回字串
#define Z_PARAM_OBJ_OF_CLASS_OR_STR_OR_NULL(destination_object, base_ce, destination_string) \
	Z_PARAM_OBJ_OF_CLASS_OR_STR_EX(destination_object, base_ce, destination_string, 1);

/* old "d" */
// ing3, 实际参数解析成小数，p1:返回值，p2:是否为null，p3:是否允null，p4:是否解引用
#define Z_PARAM_DOUBLE_EX(dest, is_null, check_null, deref) \
		/* 获取下一个实际参数，p1:是否解引用，p2:是否创建副本 */ \
		Z_PARAM_PROLOGUE(deref, 0); \
		/* 实际参数解析成小数，p2:返回值，p3:是否为null，p4:是否允null，p5:arg参数序号，报错用 */ \
		if (UNEXPECTED(!zend_parse_arg_double(_arg, &dest, &is_null, check_null, _i))) { \
			_expected_type = check_null ? Z_EXPECTED_DOUBLE_OR_NULL : Z_EXPECTED_DOUBLE; \
			_error_code = ZPP_ERROR_WRONG_ARG; \
			break; \
		}

// ing3, 实际参数解析成小数，不允null，不解引用。p1:返回值，_dummy:是否为null
#define Z_PARAM_DOUBLE(dest) \
	Z_PARAM_DOUBLE_EX(dest, _dummy, 0, 0)

// ing3, 实际参数解析成小数，允null，不解引用。p1:返回值，p2:是否为null
#define Z_PARAM_DOUBLE_OR_NULL(dest, is_null) \
	Z_PARAM_DOUBLE_EX(dest, is_null, 1, 0)

/* old "f" */

// ing3, 从传入参数，初始化调用信息。_arg:方法名，p1:调用信息，p2:调用信息缓存，p3:允null，p4:参数是否解引用
#define Z_PARAM_FUNC_EX(dest_fci, dest_fcc, check_null, deref) \
		/* 获取下一个实际参数，p1:是否解引用，p2:是否创建副本 */ \
		Z_PARAM_PROLOGUE(deref, 0); \
		/* 初始化调用信息。p1:方法名，p2:调用信息，p3:调用信息缓存，p4:允null，p5:返回错误信息 */ \
		if (UNEXPECTED(!zend_parse_arg_func(_arg, &dest_fci, &dest_fcc, check_null, &_error))) { \
			/* 如果没有错误信息 */ \
			if (!_error) { \
				/* 是否充null */ \
				_expected_type = check_null ? Z_EXPECTED_FUNC_OR_NULL : Z_EXPECTED_FUNC; \
				/* 错误码 */ \
				_error_code = ZPP_ERROR_WRONG_ARG; \
			/* 有错误信息 */ \
			} else { \
				/* 错误码 */ \
				_error_code = check_null ? ZPP_ERROR_WRONG_CALLBACK_OR_NULL : ZPP_ERROR_WRONG_CALLBACK; \
			} \
			/* 跳出 */ \
			break; \
		} \

// ing3, 从传入参数，初始化调用信息，不允null，参数不解引用。p1:调用信息，p2:调用信息缓存
#define Z_PARAM_FUNC(dest_fci, dest_fcc) \
	Z_PARAM_FUNC_EX(dest_fci, dest_fcc, 0, 0)

// ing3, 从传入参数，初始化调用信息，允null，参数不解引用。p1:调用信息，p2:调用信息缓存
#define Z_PARAM_FUNC_OR_NULL(dest_fci, dest_fcc) \
	Z_PARAM_FUNC_EX(dest_fci, dest_fcc, 1, 0)

// ing3, 从传入参数，初始化调用信息，允null，参数不解引用。p1:调用信息，p2:调用信息缓存，p3:返回参数
#define Z_PARAM_FUNC_OR_NULL_WITH_ZVAL(dest_fci, dest_fcc, dest_zp) \
	Z_PARAM_FUNC_EX(dest_fci, dest_fcc, 1, 0) \
	/* 返回 _arg。p1:接收返回值 */ \
	Z_PARAM_GET_PREV_ZVAL(dest_zp)

/* old "h" */
// ing3, 实际参数解析成数组，p1:返回值，p2:是否允null，p3:是否解引用，p4:是否创建副本
#define Z_PARAM_ARRAY_HT_EX2(dest, check_null, deref, separate) \
		/* 获取下一个实际参数，p1:是否解引用，p2:是否创建副本 */ \
		Z_PARAM_PROLOGUE(deref, separate); \
		/* 实际参数解析成哈希表，p2:返回值，p3:是否允null，p4:是否支持对象，p5:是否创建副本 */ \
		if (UNEXPECTED(!zend_parse_arg_array_ht(_arg, &dest, check_null, 0, separate))) { \
			_expected_type = check_null ? Z_EXPECTED_ARRAY_OR_NULL : Z_EXPECTED_ARRAY; \
			_error_code = ZPP_ERROR_WRONG_ARG; \
			break; \
		}

// ing3, 实际参数解析成数组，p1:返回值，p2:是否允null，p3:是否解引用 和 创建副本
#define Z_PARAM_ARRAY_HT_EX(dest, check_null, separate) \
	Z_PARAM_ARRAY_HT_EX2(dest, check_null, separate, separate)

// ing3, 实际参数解析成数组，不允null，不解引用 和 创建副本，p1:返回值
#define Z_PARAM_ARRAY_HT(dest) \
	Z_PARAM_ARRAY_HT_EX(dest, 0, 0)

// ing3, 实际参数解析成数组，允null，不解引用 和 创建副本，p1:返回值
#define Z_PARAM_ARRAY_HT_OR_NULL(dest) \
	Z_PARAM_ARRAY_HT_EX(dest, 1, 0)

// ing3, 实际参数解析成数组或整数 ，p1,2:返回值，p3: 结果是否为null，p4:是否允null
#define Z_PARAM_ARRAY_HT_OR_LONG_EX(dest_ht, dest_long, is_null, allow_null) \
	/* 获取下一个实际参数，p1:是否解引用，p2:是否创建副本 */ \
	Z_PARAM_PROLOGUE(0, 0); \
	/* 实际参数解析成数组或整数 ，p2,3:返回值，p4: 结果是否为null，p5:是否允null，p6:arg参数序号，报错用 */ \
	if (UNEXPECTED(!zend_parse_arg_array_ht_or_long(_arg, &dest_ht, &dest_long, &is_null, allow_null, _i))) { \
		_expected_type = allow_null ? Z_EXPECTED_ARRAY_OR_LONG_OR_NULL : Z_EXPECTED_ARRAY_OR_LONG; \
		_error_code = ZPP_ERROR_WRONG_ARG; \
		break; \
	}

// ing3, 实际参数解析成数组或整数，不允null ，p1,2:返回值，_dummy: 结果是否为null
#define Z_PARAM_ARRAY_HT_OR_LONG(dest_ht, dest_long) \
	Z_PARAM_ARRAY_HT_OR_LONG_EX(dest_ht, dest_long, _dummy, 0)

// ing3, 实际参数解析成数组或整数，允null ，p1,2:返回值，p3: 结果是否为null
#define Z_PARAM_ARRAY_HT_OR_LONG_OR_NULL(dest_ht, dest_long, is_null) \
	Z_PARAM_ARRAY_HT_OR_LONG_EX(dest_ht, dest_long, is_null, 1)

/* old "H" */
// ing3, 实际参数解析成数组，p1:返回值，p2:是否允null，p3:是否解引用，p4:是否创建副本
#define Z_PARAM_ARRAY_OR_OBJECT_HT_EX2(dest, check_null, deref, separate) \
		/* 获取下一个实际参数，p1:是否解引用，p2:是否创建副本 */ \
		Z_PARAM_PROLOGUE(deref, separate); \
		/* 实际参数解析成哈希表，p2:返回值，p3:是否允null，p4:是否支持对象，p5:是否创建副本 */ \
		if (UNEXPECTED(!zend_parse_arg_array_ht(_arg, &dest, check_null, 1, separate))) { \
			_expected_type = check_null ? Z_EXPECTED_ARRAY_OR_NULL : Z_EXPECTED_ARRAY; \
			_error_code = ZPP_ERROR_WRONG_ARG; \
			break; \
		}
// ing3, 实际参数解析成数组，p1:返回值，p2:是否允null，p3:是否解引用 和 创建副本
#define Z_PARAM_ARRAY_OR_OBJECT_HT_EX(dest, check_null, separate) \
	Z_PARAM_ARRAY_OR_OBJECT_HT_EX2(dest, check_null, separate, separate)

// ing3, 实际参数解析成数组，p1:返回值，不允null，不解引用 和 创建副本
#define Z_PARAM_ARRAY_OR_OBJECT_HT(dest) \
	Z_PARAM_ARRAY_OR_OBJECT_HT_EX(dest, 0, 0)

/* old "l" */

// ing3, 实际参数解析成整数，p1:返回值，p2:是否允null，p3:是否检查null，p4:是否解引用
#define Z_PARAM_LONG_EX(dest, is_null, check_null, deref) \
		/* 获取下一个实际参数，p1:是否解引用，p2:是否创建副本 */ \
		Z_PARAM_PROLOGUE(deref, 0); \
		/* 实际参数解析成整数，p2:返回值，p3:是否允null，p4:是否检查null，p5:arg参数序号，报错用 */ \
		if (UNEXPECTED(!zend_parse_arg_long(_arg, &dest, &is_null, check_null, _i))) { \
			/* 预期类型 */ \
			_expected_type = check_null ? Z_EXPECTED_LONG_OR_NULL : Z_EXPECTED_LONG; \
			/* 错误码 */ \
			_error_code = ZPP_ERROR_WRONG_ARG; \
			break; \
		}

// ing3, 实际参数解析成整数，不允null，不检查null，p1:返回值，_dummy:是否允null
#define Z_PARAM_LONG(dest) \
	Z_PARAM_LONG_EX(dest, _dummy, 0, 0)

// ing3, 实际参数解析成整数，允null，不检查null，p1:返回值，p2:是否允null
#define Z_PARAM_LONG_OR_NULL(dest, is_null) \
	Z_PARAM_LONG_EX(dest, is_null, 1, 0)

/* old "n" */
// ing3, 实际参数解析成数值，p1:返回值，p2:是否允null
#define Z_PARAM_NUMBER_EX(dest, check_null) \
	/* 获取下一个实际参数，p1:是否解引用，p2:是否创建副本 */ \
	Z_PARAM_PROLOGUE(0, 0); \
	/* 实际参数解析成数值，p2:返回值，p3:是否允null，p4:arg参数序号，报错用 */ \
	if (UNEXPECTED(!zend_parse_arg_number(_arg, &dest, check_null, _i))) { \
		_expected_type = check_null ? Z_EXPECTED_NUMBER_OR_NULL : Z_EXPECTED_NUMBER; \
		_error_code = ZPP_ERROR_WRONG_ARG; \
		break; \
	}
// ing3, 实际参数解析成数值，允null，p1:返回值
#define Z_PARAM_NUMBER_OR_NULL(dest) \
	Z_PARAM_NUMBER_EX(dest, 1)

// ing3, 实际参数解析成数值，不允null，p1:返回值
#define Z_PARAM_NUMBER(dest) \
	Z_PARAM_NUMBER_EX(dest, 0)

/* old "o" */
// ing3, 实际参数解析成对象（zval），p1:返回值，p2:是否允null，p3:是否解引用
#define Z_PARAM_OBJECT_EX(dest, check_null, deref) \
		/* 获取下一个实际参数，p1:是否解引用，p2:是否创建副本 */ \
		Z_PARAM_PROLOGUE(deref, 0); \
		/* 实际参数解析成对象（zval），p2:返回值，p3:所属类，p4:是否允null */ \
		if (UNEXPECTED(!zend_parse_arg_object(_arg, &dest, NULL, check_null))) { \
			_expected_type = check_null ? Z_EXPECTED_OBJECT_OR_NULL : Z_EXPECTED_OBJECT; \
			_error_code = ZPP_ERROR_WRONG_ARG; \
			break; \
		}

// ing3, 实际参数解析成对象（zval），不允null，不解引用，p1:返回值
#define Z_PARAM_OBJECT(dest) \
	Z_PARAM_OBJECT_EX(dest, 0, 0)

// ing3, 实际参数解析成对象（zval），允null，不解引用，p1:返回值
#define Z_PARAM_OBJECT_OR_NULL(dest) \
	Z_PARAM_OBJECT_EX(dest, 1, 0)

// 与 Z_PARAM_OBJECT_EX 相同，预期 dest 是 zend_object 而不是 zval
/* The same as Z_PARAM_OBJECT_EX except that dest is a zend_object rather than a zval */

// ing3, 实际参数解析成对象（zend_object），p1:返回值，p2:是否允null，p3:是否解引用
#define Z_PARAM_OBJ_EX(dest, check_null, deref) \
		/* 获取下一个实际参数，p1:是否解引用，p2:是否创建副本 */ \
		Z_PARAM_PROLOGUE(deref, 0); \
		/* 实际参数解析成对象（zend_object），p2:返回值，p3:所属类，p4:是否允null */ \
		if (UNEXPECTED(!zend_parse_arg_obj(_arg, &dest, NULL, check_null))) { \
			/* 解析失败 ，预期类型是 object */ \
			_expected_type = check_null ? Z_EXPECTED_OBJECT_OR_NULL : Z_EXPECTED_OBJECT; \
			/* 错误码 */ \
			_error_code = ZPP_ERROR_WRONG_ARG; \
			break; \
		}

// ing3, 实际参数解析成对象（zend_object），不允null，不解引用，p1:返回值
#define Z_PARAM_OBJ(dest) \
	Z_PARAM_OBJ_EX(dest, 0, 0)

// ing3, 实际参数解析成对象（zend_object），允null，不解引用，p1:返回值
#define Z_PARAM_OBJ_OR_NULL(dest) \
	Z_PARAM_OBJ_EX(dest, 1, 0)

/* old "O" */
// ing3，实际参数解析成对象（zval），p1:返回值，p2:所属类，p3:是否允null，p4:是否解引用
#define Z_PARAM_OBJECT_OF_CLASS_EX(dest, _ce, check_null, deref) \
		/* 获取下一个实际参数，p1:是否解引用，p2:是否创建副本 */ \
		Z_PARAM_PROLOGUE(deref, 0); \
		/* 实际参数解析成对象（zval），p2:返回值，p3:所属类，p4:是否允null */ \
		if (UNEXPECTED(!zend_parse_arg_object(_arg, &dest, _ce, check_null))) { \
			if (_ce) { \
				_error = ZSTR_VAL((_ce)->name); \
				_error_code = check_null ? ZPP_ERROR_WRONG_CLASS_OR_NULL : ZPP_ERROR_WRONG_CLASS; \
				break; \
			} else { \
				_expected_type = check_null ? Z_EXPECTED_OBJECT_OR_NULL : Z_EXPECTED_OBJECT; \
				_error_code = ZPP_ERROR_WRONG_ARG; \
				break; \
			} \
		}

// ing3，实际参数解析成对象（zval），不允null，不解引用，p1:返回值，p2:所属类
#define Z_PARAM_OBJECT_OF_CLASS(dest, _ce) \
	Z_PARAM_OBJECT_OF_CLASS_EX(dest, _ce, 0, 0)

// ing3，实际参数解析成对象（zval），允null，不解引用，p1:返回值，p2:所属类
#define Z_PARAM_OBJECT_OF_CLASS_OR_NULL(dest, _ce) \
	Z_PARAM_OBJECT_OF_CLASS_EX(dest, _ce, 1, 0)

// 和 Z_PARAM_OBJECT_OF_CLASS_EX 一样，不过返回值是 zend_object 而不是 zval
/* The same as Z_PARAM_OBJECT_OF_CLASS_EX except that dest is a zend_object rather than a zval */

// ing3, 实际参数解析成对象（zend_object），p1:返回值，p2:所属类，p3:是否允null，p4:是否解引用
#define Z_PARAM_OBJ_OF_CLASS_EX(dest, _ce, check_null, deref) \
		/* 获取下一个实际参数，p1:是否解引用，p2:是否创建副本 */ \
		Z_PARAM_PROLOGUE(deref, 0); \
		if (UNEXPECTED(!zend_parse_arg_obj(_arg, &dest, _ce, check_null))) { \
			if (_ce) { \
				_error = ZSTR_VAL((_ce)->name); \
				_error_code = check_null ? ZPP_ERROR_WRONG_CLASS_OR_NULL : ZPP_ERROR_WRONG_CLASS; \
				break; \
			} else { \
				_expected_type = check_null ? Z_EXPECTED_OBJECT_OR_NULL : Z_EXPECTED_OBJECT; \
				_error_code = ZPP_ERROR_WRONG_ARG; \
				break; \
			} \
		}

// ing3, 实际参数解析成对象（zend_object），不允null，不解引用，p1:返回值，p2:所属类
#define Z_PARAM_OBJ_OF_CLASS(dest, _ce) \
	Z_PARAM_OBJ_OF_CLASS_EX(dest, _ce, 0, 0)

// ing3, 实际参数解析成对象（zend_object），允null，不解引用，p1:返回值，p2:所属类
#define Z_PARAM_OBJ_OF_CLASS_OR_NULL(dest, _ce) \
	Z_PARAM_OBJ_OF_CLASS_EX(dest, _ce, 1, 0)

// ing3, 参数解析成整数或对象，不解引用，p1:返回值对象，p2:要求的类，p3:返回值整数，p4: 结果是否为null，p5:是否允null
#define Z_PARAM_OBJ_OF_CLASS_OR_LONG_EX(dest_obj, _ce, dest_long, is_null, allow_null) \
		/* 获取下一个实际参数，p1:是否解引用，p2:是否创建副本 */ \
		Z_PARAM_PROLOGUE(0, 0); \
		/* 参数解析成整数或对象，p2:返回值对象，p3:要求的类，p4:返回值整数，p5: 结果是否为null，p6:是否允null，p7:arg参数序号，报错用 */ \
		if (UNEXPECTED(!zend_parse_arg_obj_or_long(_arg, &dest_obj, _ce, &dest_long, &is_null, allow_null, _i))) { \
			_error = ZSTR_VAL((_ce)->name); \
			_error_code = allow_null ? ZPP_ERROR_WRONG_CLASS_OR_LONG_OR_NULL : ZPP_ERROR_WRONG_CLASS_OR_LONG; \
			break; \
		}

// ing3, 参数解析成整数或对象，不解引用，p1:返回值对象，p2:要求的类，p3:返回值整数，_dummy:是否允null
#define Z_PARAM_OBJ_OF_CLASS_OR_LONG(dest_obj, _ce, dest_long) \
	Z_PARAM_OBJ_OF_CLASS_OR_LONG_EX(dest_obj, _ce, dest_long, _dummy, 0)

// ing3, 参数解析成整数或对象，解引用，p1:返回值对象，p2:要求的类，p3:返回值整数，p4: 是否允null
#define Z_PARAM_OBJ_OF_CLASS_OR_LONG_OR_NULL(dest_obj, _ce, dest_long, is_null) \
	Z_PARAM_OBJ_OF_CLASS_OR_LONG_EX(dest_obj, _ce, dest_long, is_null, 1)

/* old "p" */
// ing3, 参数解析成字串和长度，字串不可包含无效字符，p1,2:返回值，p3:是否允null，p4:是否解引用
#define Z_PARAM_PATH_EX(dest, dest_len, check_null, deref) \
		/* 获取下一个实际参数，p1:是否解引用，p2:是否创建副本 */ \
		Z_PARAM_PROLOGUE(deref, 0); \
		/* 参数解析成字串和长度，字串不可包含无效字符，p2,3:返回值，p4:是否允null，p5:arg参数序号，报错用 */ \
		if (UNEXPECTED(!zend_parse_arg_path(_arg, &dest, &dest_len, check_null, _i))) { \
			_expected_type = check_null ? Z_EXPECTED_PATH_OR_NULL : Z_EXPECTED_PATH; \
			_error_code = ZPP_ERROR_WRONG_ARG; \
			break; \
		}
// ing3, 参数解析成字串和长度，字串不可包含无效字符，不允null，不解引用，p1,2:返回值
#define Z_PARAM_PATH(dest, dest_len) \
	Z_PARAM_PATH_EX(dest, dest_len, 0, 0)

// ing3, 参数解析成字串和长度，字串不可包含无效字符，允null，不解引用，p1,2:返回值
#define Z_PARAM_PATH_OR_NULL(dest, dest_len) \
	Z_PARAM_PATH_EX(dest, dest_len, 1, 0)

/* old "P" */
// ing3, 参数解析成 zend_string，字串不包含无效字符，p2:是否允null，p3:是否解引用
#define Z_PARAM_PATH_STR_EX(dest, check_null, deref) \
		/* 获取下一个实际参数，p1:是否解引用，p2:是否创建副本 */ \
		Z_PARAM_PROLOGUE(deref, 0); \
		/* 参数解析成 zend_string，字串不包含无效字符，p3:是否允null，p4:arg参数序号，报错用 */ \
		if (UNEXPECTED(!zend_parse_arg_path_str(_arg, &dest, check_null, _i))) { \
			_expected_type = check_null ? Z_EXPECTED_PATH_OR_NULL : Z_EXPECTED_PATH; \
			_error_code = ZPP_ERROR_WRONG_ARG; \
			break; \
		}

// ing3, 参数解析成 zend_string，不允null，不解引用，字串不包含无效字符
#define Z_PARAM_PATH_STR(dest) \
	Z_PARAM_PATH_STR_EX(dest, 0, 0)

// ing3, 参数解析成 zend_string，允null，不解引用，字串不包含无效字符
#define Z_PARAM_PATH_STR_OR_NULL(dest) \
	Z_PARAM_PATH_STR_EX(dest, 1, 0)

/* old "r" */
// ing3, 获取下一个实际参数，类型：资源。 p1:返回值，p2:是否允null，p3:是否解引用
#define Z_PARAM_RESOURCE_EX(dest, check_null, deref) \
		/* 获取下一个实际参数，p1:是否解引用，p2:是否创建副本 */ \
		Z_PARAM_PROLOGUE(deref, 0); \
		/* 实际参数解析成资源类型，p2:返回值，p3:是否允null */ \
		if (UNEXPECTED(!zend_parse_arg_resource(_arg, &dest, check_null))) { \
			_expected_type = check_null ? Z_EXPECTED_RESOURCE_OR_NULL : Z_EXPECTED_RESOURCE; \
			_error_code = ZPP_ERROR_WRONG_ARG; \
			break; \
		}

// ing3, 获取下一个实际参数，类型：资源，不允null，不解引用。
#define Z_PARAM_RESOURCE(dest) \
	Z_PARAM_RESOURCE_EX(dest, 0, 0)

// ing3, 获取下一个实际参数，类型：资源，允null，不解引用。
#define Z_PARAM_RESOURCE_OR_NULL(dest) \
	Z_PARAM_RESOURCE_EX(dest, 1, 0)

/* old "s" */
// ing3, 获取下一个实际参数，类型：字串。 p1,2:返回值，p3:是否允null，p4:是否解引用
#define Z_PARAM_STRING_EX(dest, dest_len, check_null, deref) \
		/* 获取下一个实际参数，p1:是否解引用，p2:是否创建副本 */ \
		Z_PARAM_PROLOGUE(deref, 0); \
		/* 实际参数解析成字串和长度，p2,3:返回值，p4:是否允null，p5是arg参数序号，报错用 */ \
		if (UNEXPECTED(!zend_parse_arg_string(_arg, &dest, &dest_len, check_null, _i))) { \
			_expected_type = check_null ? Z_EXPECTED_STRING_OR_NULL : Z_EXPECTED_STRING; \
			_error_code = ZPP_ERROR_WRONG_ARG; \
			break; \
		}
// ing3, 获取下一个实际参数，类型：字串，不允null，不解引用。 p1,2:返回值
#define Z_PARAM_STRING(dest, dest_len) \
	Z_PARAM_STRING_EX(dest, dest_len, 0, 0)

// ing3, 获取下一个实际参数，类型：字串，允null，不解引用。 p1,2:返回值
#define Z_PARAM_STRING_OR_NULL(dest, dest_len) \
	Z_PARAM_STRING_EX(dest, dest_len, 1, 0)

/* old "S" */
// ing3, 获取下一个实际参数，类型：zend_string。 p1:返回值，p2:是否允null，p3:是否解引用
#define Z_PARAM_STR_EX(dest, check_null, deref) \
		/* 获取下一个实际参数，p1:是否解引用，p2:是否创建副本 */ \
		Z_PARAM_PROLOGUE(deref, 0); \
		/* 实际参数解析成 zend_string，p2:返回值，p3:是否允null，p4:arg参数序号，报错用 */ \
		if (UNEXPECTED(!zend_parse_arg_str(_arg, &dest, check_null, _i))) { \
			_expected_type = check_null ? Z_EXPECTED_STRING_OR_NULL : Z_EXPECTED_STRING; \
			_error_code = ZPP_ERROR_WRONG_ARG; \
			break; \
		}
// ing3, 获取下一个实际参数，类型：zend_string，不允null，不解引用。
#define Z_PARAM_STR(dest) \
	Z_PARAM_STR_EX(dest, 0, 0)
	
// ing3, 获取下一个实际参数，类型：zend_string，允null，不解引用。
#define Z_PARAM_STR_OR_NULL(dest) \
	Z_PARAM_STR_EX(dest, 1, 0)

/* old "z" */
// ing4, 取下一个参数，如果要检验null并且 arg 是null，返回null，否则返回 arg
#define Z_PARAM_ZVAL_EX2(dest, check_null, deref, separate) \
		/* 获取下一个实际参数，p1:是否解引用，p2:是否创建副本 */ \
		Z_PARAM_PROLOGUE(deref, separate); \
		/* 如果要检验null并且 p1 是null，返回null.否则返回 p1.p2:返回值，p3:允null */ \
		zend_parse_arg_zval_deref(_arg, &dest, check_null);

// ing4, 取下一个参数，如果要检验null并且 arg 是null，返回null，否则返回 arg
#define Z_PARAM_ZVAL_EX(dest, check_null, separate) \
	/* 这里 separate 和 deref 要求相同 */ \
	Z_PARAM_ZVAL_EX2(dest, check_null, separate, separate)

// ing4, 取下一个参数，不检测null，返回什么就是什么
#define Z_PARAM_ZVAL(dest) \
	Z_PARAM_ZVAL_EX(dest, 0, 0)

// ing4, 取下一个参数，null，但不创建副本
#define Z_PARAM_ZVAL_OR_NULL(dest) \
	Z_PARAM_ZVAL_EX(dest, 1, 0)

/* old "+" and "*" */
// 这里前面是 ZEND_PARSE_PARAMETERS_START -> ZEND_PARSE_PARAMETERS_START_EX，都是宏之前的连接
// ing3, 解析字典参数，p1:格式，p2:返回参数列表，p3:返回参数数量，p4:固定传0（全局只有 Z_PARAM_VARIADIC_EX 调用）
#define Z_PARAM_VARIADIC_EX(spec, dest, dest_num, post_varargs) do { \
		/* 字典参数数量 = 传入参数数量 - 已解析过的参数 - 0 */ \
		uint32_t _num_varargs = _num_args - _i - (post_varargs); \
		/* 如果 字典参数数量 > 0 */ \
		if (EXPECTED(_num_varargs > 0)) { \
			/* 返回真实参数的 后面一段（跳过已解析部分） */ \
			dest = _real_arg + 1; \
			/* 返回 字典参数数量 */ \
			dest_num = _num_varargs; \
			/* 更新 已解析参数数量 */ \
			_i += _num_varargs; \
			/* 真实参数列表指针后移 */ \
			_real_arg += _num_varargs; \
		} else { \
			/* 返回参数列表为空 */\
			dest = NULL; \
			/* 返回参数数量为0 */\
			dest_num = 0; \
		} \
		/* 如果有额外命名参数 */ \
		if (UNEXPECTED(ZEND_CALL_INFO(execute_data) & ZEND_CALL_HAS_EXTRA_NAMED_PARAMS)) { \
			/* 错误号：需要额外命名参数 */ \
			_error_code = ZPP_ERROR_UNEXPECTED_EXTRA_NAMED; \
			/* 跳出 */ \
			break; \
		} \
	} while (0);

// ing3, 解析字典参数，p1:格式，p2:返回参数列表，p3:返回参数数量。外部调用
#define Z_PARAM_VARIADIC(spec, dest, dest_num) \
	/* 解析字典参数，p1:格式，p2:返回参数列表，p3:返回参数数量，p4:固定传0（全局只有 Z_PARAM_VARIADIC_EX 调用） */ \
	Z_PARAM_VARIADIC_EX(spec, dest, dest_num, 0)

// ing3, 解析字典参数和命名参数
#define Z_PARAM_VARIADIC_WITH_NAMED(dest, dest_num, dest_named) do { \
		/* 命名参数数量 = 传入参数数量 - 已解析过的参数 */ \
		uint32_t _num_varargs = _num_args - _i; \
		/* 如果 命名参数数量 > 0 */ \
		if (EXPECTED(_num_varargs > 0)) { \
			/* 返回真实参数的 后面一段（跳过已解析部分） */ \
			dest = _real_arg + 1; \
			/* 返回 命名参数数量 */ \
			dest_num = _num_varargs; \
		} else { \
			/* 返回参数列表为空 */\
			dest = NULL; \
			/* 返回参数数量为0 */\
			dest_num = 0; \
		} \
		/* 如果有额外命名参数 */ \
		if (ZEND_CALL_INFO(execute_data) & ZEND_CALL_HAS_EXTRA_NAMED_PARAMS) { \
			/* 返回额外的命名参数 */\
			dest_named = execute_data->extra_named_params; \
		} else { \
			/* 额外的命名参数 为空 */\
			dest_named = NULL; \
		} \
	} while (0);

// ing3, 获取下一个实际参数，类型：哈希表或字串。 p1,2:返回值，p3:是否允null
#define Z_PARAM_ARRAY_HT_OR_STR_EX(dest_ht, dest_str, allow_null) \
	/* 获取下一个实际参数，p1:是否解引用，p2:是否创建副本 */ \
	Z_PARAM_PROLOGUE(0, 0); \
	/* 把传入的参数转换成字串或哈希表, p2,3:返回值，p4:是否允null，p5:arg参数序号，报错用 */ \
	if (UNEXPECTED(!zend_parse_arg_array_ht_or_str(_arg, &dest_ht, &dest_str, allow_null, _i))) { \
		/* 如果失败，准备报错数据 ，类型和错误码 */ \
		_expected_type = allow_null ? Z_EXPECTED_ARRAY_OR_STRING_OR_NULL : Z_EXPECTED_ARRAY_OR_STRING; \
		_error_code = ZPP_ERROR_WRONG_ARG; \
		break; \
	}
// ing3, 获取下一个实际参数，类型：哈希表或字串，不允null。 p1,2:返回值，p3:是否允null
#define Z_PARAM_ARRAY_HT_OR_STR(dest_ht, dest_str) \
	Z_PARAM_ARRAY_HT_OR_STR_EX(dest_ht, dest_str, 0);

// ing3, 获取下一个实际参数，类型：哈希表或字串，允null。 p1,2:返回值，p3:是否允null
#define Z_PARAM_ARRAY_HT_OR_STR_OR_NULL(dest_ht, dest_str) \
	Z_PARAM_ARRAY_HT_OR_STR_EX(dest_ht, dest_str, 1);


// ing3, 获取下一个实际参数，类型：整数。 p1,p2:返回值，p3: 结果是否为null，p4:是否允null
#define Z_PARAM_STR_OR_LONG_EX(dest_str, dest_long, is_null, allow_null) \
	/* 获取下一个实际参数，p1:是否解引用，p2:是否创建副本 */ \
	Z_PARAM_PROLOGUE(0, 0); \
	/* 参数解析成字串或整数，p2,p3:返回值，p4: 结果是否为null，p5:是否允null，p6:arg参数序号，报错用 */ \
	if (UNEXPECTED(!zend_parse_arg_str_or_long(_arg, &dest_str, &dest_long, &is_null, allow_null, _i))) { \
		_expected_type = allow_null ? Z_EXPECTED_STRING_OR_LONG_OR_NULL : Z_EXPECTED_STRING_OR_LONG; \
		_error_code = ZPP_ERROR_WRONG_ARG; \
		break; \
	}

// ing3, 获取下一个实际参数，类型：整数，不允null。 p1,p2:返回值
#define Z_PARAM_STR_OR_LONG(dest_str, dest_long) \
	Z_PARAM_STR_OR_LONG_EX(dest_str, dest_long, _dummy, 0);

// ing3, 获取下一个实际参数，类型：整数，允null。 p1,p2:返回值
#define Z_PARAM_STR_OR_LONG_OR_NULL(dest_str, dest_long, is_null) \
	Z_PARAM_STR_OR_LONG_EX(dest_str, dest_long, is_null, 1);

// 新参数解析API结束
/* End of new parameter parsing API */

/* Inlined implementations shared by new and old parameter parsing APIs */

ZEND_API bool ZEND_FASTCALL zend_parse_arg_class(zval *arg, zend_class_entry **pce, uint32_t num, bool check_null);
ZEND_API bool ZEND_FASTCALL zend_parse_arg_bool_slow(zval *arg, bool *dest, uint32_t arg_num);
ZEND_API bool ZEND_FASTCALL zend_parse_arg_bool_weak(zval *arg, bool *dest, uint32_t arg_num);
ZEND_API bool ZEND_FASTCALL zend_parse_arg_long_slow(zval *arg, zend_long *dest, uint32_t arg_num);
ZEND_API bool ZEND_FASTCALL zend_parse_arg_long_weak(zval *arg, zend_long *dest, uint32_t arg_num);
ZEND_API bool ZEND_FASTCALL zend_parse_arg_double_slow(zval *arg, double *dest, uint32_t arg_num);
ZEND_API bool ZEND_FASTCALL zend_parse_arg_double_weak(zval *arg, double *dest, uint32_t arg_num);
ZEND_API bool ZEND_FASTCALL zend_parse_arg_str_slow(zval *arg, zend_string **dest, uint32_t arg_num);
ZEND_API bool ZEND_FASTCALL zend_parse_arg_str_weak(zval *arg, zend_string **dest, uint32_t arg_num);
ZEND_API bool ZEND_FASTCALL zend_parse_arg_number_slow(zval *arg, zval **dest, uint32_t arg_num);
ZEND_API bool ZEND_FASTCALL zend_parse_arg_str_or_long_slow(zval *arg, zend_string **dest_str, zend_long *dest_long, uint32_t arg_num);

// ing4,把参数解析成布尔值，p2:返回值，p3:是否允null，p4:是否检查null，p5:arg参数序号，报错用 
static zend_always_inline bool zend_parse_arg_bool(zval *arg, bool *dest, bool *is_null, bool check_null, uint32_t arg_num)
{
	// 如果需要检查 null
	if (check_null) {
		*is_null = 0;
	}
	// 如果参数值为true
	if (EXPECTED(Z_TYPE_P(arg) == IS_TRUE)) {
		// 结果为1
		*dest = 1;
	// 如果参数值为false
	} else if (EXPECTED(Z_TYPE_P(arg) == IS_FALSE)) {
		// 结果为0
		*dest = 0;
	// 如果要求检查null，并且结果是null
	} else if (check_null && Z_TYPE_P(arg) == IS_NULL) {
		// 检查到null
		*is_null = 1;
		// 结果为0
		*dest = 0;
	// 其他类型
	} else {
		return zend_parse_arg_bool_slow(arg, dest, arg_num);
	}
	return 1;
}

// ing4, 实际参数解析成整数，p2:返回值，p3:是否允null，p4:是否检查null，p5:arg参数序号，报错用 
static zend_always_inline bool zend_parse_arg_long(zval *arg, zend_long *dest, bool *is_null, bool check_null, uint32_t arg_num)
{	// 如果需要检验空
	if (check_null) {
		// 默认不是null
		*is_null = 0;
	}
	// 如果参数是整数
	if (EXPECTED(Z_TYPE_P(arg) == IS_LONG)) {
		// 直接取出并返回
		*dest = Z_LVAL_P(arg);
	// 如果是null
	} else if (check_null && Z_TYPE_P(arg) == IS_NULL) {
		// 是null
		*is_null = 1;
		// 结果是0
		*dest = 0;
	// 其他情况
	} else {
		// 把参数转成整数 
		return zend_parse_arg_long_slow(arg, dest, arg_num);
	}
	return 1;
}

// ing3, 实际参数解析成小数，p2:返回值，p3:是否为null，p4:是否允null，p5:arg参数序号，报错用
static zend_always_inline bool zend_parse_arg_double(zval *arg, double *dest, bool *is_null, bool check_null, uint32_t arg_num)
{
	// 如果需要检验空
	if (check_null) {
		// 默认不是null
		*is_null = 0;
	}
	// 如果参数是小整数
	if (EXPECTED(Z_TYPE_P(arg) == IS_DOUBLE)) {
		*dest = Z_DVAL_P(arg);
	// 是null并且允null，结果是 0
	} else if (check_null && Z_TYPE_P(arg) == IS_NULL) {
		// 值是null
		*is_null = 1;
		*dest = 0.0;
	// 其他情况
	} else {
		// (非整数，严格类型返回0)把传入的参数转换成小数，遇到null会报错。p2:返回值，p3:arg参数序号，报错用
		return zend_parse_arg_double_slow(arg, dest, arg_num);
	}
	return 1;
}

// ing3, 实际参数解析成数值，p2:返回值，p3:是否允null，p4:arg参数序号，报错用
static zend_always_inline bool zend_parse_arg_number(zval *arg, zval **dest, bool check_null, uint32_t arg_num)
{
	// 如果类型是整数、小数，直接返回arg
	if (EXPECTED(Z_TYPE_P(arg) == IS_LONG || Z_TYPE_P(arg) == IS_DOUBLE)) {
		*dest = arg;
	// 是null并且允null，结果是null
	} else if (check_null && EXPECTED(Z_TYPE_P(arg) == IS_NULL)) {
		*dest = NULL;
	// 其他情况
	} else {
		// (严格类型返回0)把传入的参数转换成数值，遇到null会报错。p2:返回值，p3:arg参数序号，报错用
		return zend_parse_arg_number_slow(arg, dest, arg_num);
	}
	// 成功
	return 1;
}

// ing4, 实际参数解析成 zend_string，p2:返回值，p3:是否允null，p4:arg参数序号，报错用
static zend_always_inline bool zend_parse_arg_str(zval *arg, zend_string **dest, bool check_null, uint32_t arg_num)
{
	// 字串直接返回
	if (EXPECTED(Z_TYPE_P(arg) == IS_STRING)) {
		*dest = Z_STR_P(arg);
	// 如果要检测null 并且 值为null
	} else if (check_null && Z_TYPE_P(arg) == IS_NULL) {
		// 返回null
		*dest = NULL;
	// 如果不检测null，并且返回null，会报错
	} else {
		// （严格类型返回失败）把传入的参数转换成字串，遇到null会报错。结果从p2返回，p3是 arg参数序号，报错时用
		return zend_parse_arg_str_slow(arg, dest, arg_num);
	}
	// 返回成功
	return 1;
}

// ing4, 实际参数解析成字串和长度，p2,3:返回值，p4:是否允null，p5是arg参数序号，报错用
static zend_always_inline bool zend_parse_arg_string(zval *arg, char **dest, size_t *dest_len, bool check_null, uint32_t arg_num)
{
	zend_string *str;
	// 实际参数解析成字串，p2:返回值，p3:是否允null，p4:arg参数序号，报错用
	if (!zend_parse_arg_str(arg, &str, check_null, arg_num)) {
		return 0;
	}
	// 如果要检测null 并且str无效
	if (check_null && UNEXPECTED(!str)) {
		// 结果为null
		*dest = NULL;
		// 长度为0
		*dest_len = 0;
	// 不测试null或结果有效
	} else {
		// 返回结果
		*dest = ZSTR_VAL(str);
		// 返回长度
		*dest_len = ZSTR_LEN(str);
	}
	return 1;
}


// ing4, 参数解析成 zend_string，字串不包含无效字符，p3:是否允null，p4:arg参数序号，报错用
static zend_always_inline bool zend_parse_arg_path_str(zval *arg, zend_string **dest, bool check_null, uint32_t arg_num)
{
	// 实际参数解析成字串，p2:返回值，p3:是否允null，p4:arg参数序号，报错用
	// 检验：字串中是否包含无效字符，（已知长度是否与计算出的长度相符）
	// 如果有一个失败，返回失败
	if (!zend_parse_arg_str(arg, dest, check_null, arg_num) ||
	    (*dest && UNEXPECTED(CHECK_NULL_PATH(ZSTR_VAL(*dest), ZSTR_LEN(*dest))))) {
		return 0;
	}
	return 1;
}

// ing4, 参数解析成字串和长度，字串不可包含无效字符，p2,3:返回值，p4:是否允null，p5:arg参数序号，报错用
static zend_always_inline bool zend_parse_arg_path(zval *arg, char **dest, size_t *dest_len, bool check_null, uint32_t arg_num)
{
	zend_string *str;
	// 参数解析成 zend_string，字串不可包含无效字符，check_null也是允null的意思，p4是arg参数序号，报错用
	if (!zend_parse_arg_path_str(arg, &str, check_null, arg_num)) {
		return 0;
	}
	// 允null 并且 碰到null
	if (check_null && UNEXPECTED(!str)) {
		// 结果为null
		*dest = NULL;
		// 长度0
		*dest_len = 0;
	// 其他情况
	} else {
		// 结果
		*dest = ZSTR_VAL(str);
		// 长度
		*dest_len = ZSTR_LEN(str);
	}
	return 1;
}

// ing4, 实际参数解析成迭代器，p2:返回值，p3:是否允null
static zend_always_inline bool zend_parse_arg_iterable(zval *arg, zval **dest, bool check_null)
{
	// 如果参数本来是迭代器，返回true
	if (EXPECTED(zend_is_iterable(arg))) {
		// 结果是原参数
		*dest = arg;
		return 1;
	}
	// 允null，并且参数是null,返回true
	if (check_null && EXPECTED(Z_TYPE_P(arg) == IS_NULL)) {
		// 结果为null
		*dest = NULL;
		return 1;
	}
	// 返回false
	return 0;
}

// ing4, 实际参数解析成数组或对象，p2:返回值，p3:是否允null，p4:是否允许对象
static zend_always_inline bool zend_parse_arg_array(zval *arg, zval **dest, bool check_null, bool or_object)
{
	// 如果参数本身是数组 或 （or_object有效并且是对象）
	if (EXPECTED(Z_TYPE_P(arg) == IS_ARRAY) ||
		(or_object && EXPECTED(Z_TYPE_P(arg) == IS_OBJECT))) {
		// 直接返回arg
		*dest = arg;
	// 允null，并且参数是null
	} else if (check_null && EXPECTED(Z_TYPE_P(arg) == IS_NULL)) {
		*dest = NULL;
	// 其他情况
	} else {
		// 返回false
		return 0;
	}
	// true
	return 1;
}

// ing4, 实际参数解析成哈希表，p2:返回值，p3:是否允null，p4:是否支持对象，p5:是否创建副本
static zend_always_inline bool zend_parse_arg_array_ht(zval *arg, HashTable **dest, bool check_null, bool or_object, bool separate)
{
	// 如果参数是数组
	if (EXPECTED(Z_TYPE_P(arg) == IS_ARRAY)) {
		*dest = Z_ARRVAL_P(arg);
	// 如果 支持对象 并且 参数是对象
	} else if (or_object && EXPECTED(Z_TYPE_P(arg) == IS_OBJECT)) {
		// 获取参数中的对象
		zend_object *zobj = Z_OBJ_P(arg);
		// 如果要求创建副本
		if (separate
		 // 并且对象有属性表
		 && zobj->properties
		 // 并且属性表引用次数大于1
		 && UNEXPECTED(GC_REFCOUNT(zobj->properties) > 1)) {
			// 如果属性表不是 不可变的
			if (EXPECTED(!(GC_FLAGS(zobj->properties) & IS_ARRAY_IMMUTABLE))) {
				// 减少引用次数
				GC_DELREF(zobj->properties);
			}
			// 创建副本
			zobj->properties = zend_array_dup(zobj->properties);
		}
		// zend_std_get_properties ，获取属性哈希表
		*dest = zobj->handlers->get_properties(zobj);
	// 允null，并且参数是null
	} else if (check_null && EXPECTED(Z_TYPE_P(arg) == IS_NULL)) {
		*dest = NULL;
	// 其他情况
	} else {
		// 返回假
		return 0;
	}
	// 返回真
	return 1;
}

// ing4, 实际参数解析成数组或整数 ，p2,3:返回值，p4: 结果是否为null，p5:是否允null，p6:arg参数序号，报错用
static zend_always_inline bool zend_parse_arg_array_ht_or_long(
	zval *arg, HashTable **dest_ht, zend_long *dest_long, bool *is_null, bool allow_null, uint32_t arg_num
) {
	// 允null
	if (allow_null) {
		// 结果不为null
		*is_null = 0;
	}
	// 如果参数是数组
	if (EXPECTED(Z_TYPE_P(arg) == IS_ARRAY)) {
		// 返回参数中的哈希表
		*dest_ht = Z_ARRVAL_P(arg);
	// 如果参数是整数 
	} else if (EXPECTED(Z_TYPE_P(arg) == IS_LONG)) {
		// 返回哈希表为null
		*dest_ht = NULL;
		// 返回整数 
		*dest_long = Z_LVAL_P(arg);
	// 允null，并且参数是null
	} else if (allow_null && EXPECTED(Z_TYPE_P(arg) == IS_NULL)) {
		// 返回哈希表为null
		*dest_ht = NULL;
		// 结果为null
		*is_null = 1;
	// 其他情况
	} else {
		*dest_ht = NULL;
		// (严格类型返回0)把传入的参数转换成整数，遇到null会报错。p2:返回值，p3:arg参数序号，报错用
		return zend_parse_arg_long_slow(arg, dest_long, arg_num);
	}
	// 返回真
	return 1;
}

// ing4, 实际参数解析成对象（zval），p2:返回值，p3:所属类，p4:是否允null
static zend_always_inline bool zend_parse_arg_object(zval *arg, zval **dest, zend_class_entry *ce, bool check_null)
{
	// 如果参数是对象 并且 （没有指定类 或 参数对象属于 指定类）
	if (EXPECTED(Z_TYPE_P(arg) == IS_OBJECT) &&
	    (!ce || EXPECTED(instanceof_function(Z_OBJCE_P(arg), ce) != 0))) {
		// 返回结果为此参数
		*dest = arg;
	// 允null，并且参数是null
	} else if (check_null && EXPECTED(Z_TYPE_P(arg) == IS_NULL)) {
		// 返回结果为空
		*dest = NULL;
	// 其他情况
	} else {
		// 返回 false
		return 0;
	}
	// 返回结果为 true
	return 1;
}

// ing4, 实际参数解析成对象（zend_object），p2:返回值，p3:所属类，p4:是否允null
static zend_always_inline bool zend_parse_arg_obj(zval *arg, zend_object **dest, zend_class_entry *ce, bool check_null)
{
	// 如果参数是对象 并且 （没有指定类 或 参数对象属于 指定类）
	if (EXPECTED(Z_TYPE_P(arg) == IS_OBJECT) &&
	    (!ce || EXPECTED(instanceof_function(Z_OBJCE_P(arg), ce) != 0))) {
		// 返回结果为此参数 中的对象
		*dest = Z_OBJ_P(arg);
	// 允null，并且参数是null
	} else if (check_null && EXPECTED(Z_TYPE_P(arg) == IS_NULL)) {
		// 返回结果为空
		*dest = NULL;
	// 其他情况
	} else {
		// 返回 false
		return 0;
	}
	// 返回结果为 true
	return 1;
}

// ing4, 参数解析成整数或对象，p2:返回值对象，p3:要求的类，p4:返回值整数，p5: 结果是否为null，p6:是否允null，p7:arg参数序号，报错用
static zend_always_inline bool zend_parse_arg_obj_or_long(
	zval *arg, zend_object **dest_obj, zend_class_entry *ce, zend_long *dest_long, bool *is_null, bool allow_null, uint32_t arg_num
) {
	// 允null
	if (allow_null) {
		// 结果不为null
		*is_null = 0;
	}
	// 如果参数是对象 并且 对象属于要求的类
	if (EXPECTED(Z_TYPE_P(arg) == IS_OBJECT) && EXPECTED(instanceof_function(Z_OBJCE_P(arg), ce) != 0)) {
		// 结果为此对象
		*dest_obj = Z_OBJ_P(arg);
	// 如果参数是整数
	} else if (EXPECTED(Z_TYPE_P(arg) == IS_LONG)) {
		// 对象结果为空
		*dest_obj = NULL;
		// 整数结果
		*dest_long = Z_LVAL_P(arg);
	// 允null，并且参数是null
	} else if (allow_null && EXPECTED(Z_TYPE_P(arg) == IS_NULL)) {
		// 对象结果为空
		*dest_obj = NULL;
		// 返回结果为null
		*is_null = 1;
	// 其他情况
	} else {
		// 对象结果为空
		*dest_obj = NULL;
		//  (严格类型返回0)把传入的参数转换成整数，遇到null会报错。p2:返回值，p3:arg参数序号，报错用
		return zend_parse_arg_long_slow(arg, dest_long, arg_num);
	}
	// 返回true
	return 1;
}

// ing4, 实际参数解析成资源类型，p2:返回值，p3:是否允null
static zend_always_inline bool zend_parse_arg_resource(zval *arg, zval **dest, bool check_null)
{
	// 参数类型是资源
	if (EXPECTED(Z_TYPE_P(arg) == IS_RESOURCE)) {
		// 此参数为结果 
		*dest = arg;
	// 允null，并且参数是null
	} else if (check_null && EXPECTED(Z_TYPE_P(arg) == IS_NULL)) {
		// 结果为null
		*dest = NULL;
	// 其他情况	
	} else {
		// 对象结果为空
		return 0;
	}
	// 返回true
	return 1;
}

// ing3，初始化调用信息。p1:方法名，p2:调用信息，p3:调用信息缓存，p4:允null，p5:返回错误信息
static zend_always_inline bool zend_parse_arg_func(zval *arg, zend_fcall_info *dest_fci, zend_fcall_info_cache *dest_fcc, bool check_null, char **error)
{
	// 如果参数是null
	if (check_null && UNEXPECTED(Z_TYPE_P(arg) == IS_NULL)) {
		dest_fci->size = 0;
		dest_fcc->function_handler = NULL;
		*error = NULL;
	// 初始化调用信息。如果失败：
	// p1:方法名，p2:检验级别，p3:调用信息，p4:调用信息缓存，p5:返回调用字串，p6:返回错误信息
	} else if (UNEXPECTED(zend_fcall_info_init(arg, 0, dest_fci, dest_fcc, NULL, error) != SUCCESS)) {
		// 返回 否
		return 0;
	}
	// 释放调用弹跳：方法可能没有被调用，这各情况下弹跳会泄露。在 zend_call_function 里会重新获取它。
	/* Release call trampolines: The function may not get called, in which case
	 * the trampoline will leak. Force it to be refetched during
	 * zend_call_function instead. */
	 
	// 释放缓存的 函数调用信息
	zend_release_fcall_info_cache(dest_fcc);
	// 返回是
	return 1;
}

// ing4, 如果要检测null 并且 arg是null 或 （arg是引用 并且 引用目标是null），返回null，否则返回arg
static zend_always_inline void zend_parse_arg_zval(zval *arg, zval **dest, bool check_null)
{
	// 如果要检测null
	*dest = (check_null &&
		// 并且 arg是null 或 （arg是引用 并且 引用目标是null）
	    (UNEXPECTED(Z_TYPE_P(arg) == IS_NULL) ||
	     (UNEXPECTED(Z_ISREF_P(arg)) &&
	      UNEXPECTED(Z_TYPE_P(Z_REFVAL_P(arg)) == IS_NULL)))) ? NULL : arg;
}

// ing4, 如果要检验null并且 p1 是null，返回null.否则返回 p1.p2:返回值，p3:允null
static zend_always_inline void zend_parse_arg_zval_deref(zval *arg, zval **dest, bool check_null)
{
	// 如果要检验null并且 arg 是null，返回null.否则返回 arg
	*dest = (check_null && UNEXPECTED(Z_TYPE_P(arg) == IS_NULL)) ? NULL : arg;
}

// ing4, 把传入的参数转换成字串或哈希表, p2,3:返回值，p4:是否允null，p5:arg参数序号，报错用
static zend_always_inline bool zend_parse_arg_array_ht_or_str(
		zval *arg, HashTable **dest_ht, zend_string **dest_str, bool allow_null, uint32_t arg_num)
{
	// 类型是字串
	if (EXPECTED(Z_TYPE_P(arg) == IS_STRING)) {
		// 返回哈希表为空
		*dest_ht = NULL;
		// 返回字串
		*dest_str = Z_STR_P(arg);
	// 类型是数组
	} else if (EXPECTED(Z_TYPE_P(arg) == IS_ARRAY)) {
		// 返回哈希表
		*dest_ht = Z_ARRVAL_P(arg);
		// 返回为空
		*dest_str = NULL;
	// 允null，并且参数是null
	} else if (allow_null && EXPECTED(Z_TYPE_P(arg) == IS_NULL)) {
		// 两个返回值都是空
		*dest_ht = NULL;
		*dest_str = NULL;
	// 其他情况
	} else {
		// 返回哈希表为空
		*dest_ht = NULL;
		//（严格类型返回失败）把传入的参数转换成字串，遇到null会报错。p2:返回值，p3:arg参数序号，报错用
		return zend_parse_arg_str_slow(arg, dest_str, arg_num);
	}
	// 返回true
	return 1;
}

// ing4, 参数解析成字串或整数，p2,p3:返回值，p4: 结果是否为null，p5:是否允null，p6:arg参数序号，报错用
static zend_always_inline bool zend_parse_arg_str_or_long(zval *arg, zend_string **dest_str, zend_long *dest_long,
	bool *is_null, bool allow_null, uint32_t arg_num)
{
	// 允null
	if (allow_null) {
		// 结果不为null
		*is_null = 0;
	}
	// 如果参数是字串
	if (EXPECTED(Z_TYPE_P(arg) == IS_STRING)) {
		// 此参数为结果 
		*dest_str = Z_STR_P(arg);
	// 如果参数是 整数
	} else if (EXPECTED(Z_TYPE_P(arg) == IS_LONG)) {
		// 结果字串为null
		*dest_str = NULL;
		// 整数结果
		*dest_long = Z_LVAL_P(arg);
	// 允null，并且参数是null
	} else if (allow_null && EXPECTED(Z_TYPE_P(arg) == IS_NULL)) {
		// 字串结果为null
		*dest_str = NULL;
		// 结果为null
		*is_null = 1;
	// 其他情况
	} else {
		// （严格类型返回失败）把传入的参数转换成整数或字串，遇到null会报错。p2,3:返回值，p4:arg参数序号，报错用
		return zend_parse_arg_str_or_long_slow(arg, dest_str, dest_long, arg_num);
	}
	// 返回true
	return 1;
}

// ing4, 把传入的参数（类名或对象）转换成类入口, p2:返回值，p3:是否允null
static zend_always_inline bool zend_parse_arg_obj_or_class_name(
	zval *arg, zend_class_entry **destination, bool allow_null
) {
	// 如果传入字串
	if (EXPECTED(Z_TYPE_P(arg) == IS_STRING)) {
		// 查找并返回类入口
		*destination = zend_lookup_class(Z_STR_P(arg));
		// 返回查找是否成功
		return *destination != NULL;
	// 如果传入对象
	} else if (EXPECTED(Z_TYPE_P(arg) == IS_OBJECT)) {
		// 直接用它的所属类
		*destination = Z_OBJ_P(arg)->ce;
	// 允null，并且参数是null
	} else if (allow_null && EXPECTED(Z_TYPE_P(arg) == IS_NULL)) {
		// 返回类为空
		*destination = NULL;
	// 其他情况
	} else {
		// 返回false
		return 0;
	}
	// 返回true
	return 1;
}

// ing4, 把传入的参数转换成对象或字串, p2:返回值，p3:要求的类，p4:返回字串，p5:是否允null，p6:arg参数序号，报错用
static zend_always_inline bool zend_parse_arg_obj_or_str(
	zval *arg, zend_object **destination_object, zend_class_entry *base_ce, zend_string **destination_string, bool allow_null, uint32_t arg_num
) {
	// 如果传入对象
	if (EXPECTED(Z_TYPE_P(arg) == IS_OBJECT)) {
		// 没有要求所属类 或 与要求匹配
		if (!base_ce || EXPECTED(instanceof_function(Z_OBJCE_P(arg), base_ce))) {
			// 返回此对象
			*destination_object = Z_OBJ_P(arg);
			// 返回字串为空
			*destination_string = NULL;
			// 返回 true
			return 1;
		}
	}
	// 返回对象为空
	*destination_object = NULL;
	// 实际参数解析成 zend_string，p2:返回值，p3:是否允null，p4:arg参数序号，报错用
	return zend_parse_arg_str(arg, destination_string, allow_null, arg_num);
}

END_EXTERN_C()

#endif /* ZEND_API_H */
