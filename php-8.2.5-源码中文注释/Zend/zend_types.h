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
   |          Xinchen Hui <laruence@php.net>                              |
   +----------------------------------------------------------------------+
*/

// zval 是最核心struct
// 外部调用的都是大写的函数（宏），宏的一个重要的特点是逻辑简单，代码行数少
// 绝大部分函数都有外部调用，很多函数有大量外部调用


#ifndef ZEND_TYPES_H
#define ZEND_TYPES_H

#include "zend_portability.h"
#include "zend_long.h"
#include <stdbool.h>

#ifdef __SSE2__
# include <mmintrin.h>
# include <emmintrin.h>
#endif

// ing3, 高位在前 
#ifdef WORDS_BIGENDIAN
# define ZEND_ENDIAN_LOHI(lo, hi)          hi; lo;
# define ZEND_ENDIAN_LOHI_3(lo, mi, hi)    hi; mi; lo;
# define ZEND_ENDIAN_LOHI_4(a, b, c, d)    d; c; b; a;
# define ZEND_ENDIAN_LOHI_C(lo, hi)        hi, lo
# define ZEND_ENDIAN_LOHI_C_3(lo, mi, hi)  hi, mi, lo,
# define ZEND_ENDIAN_LOHI_C_4(a, b, c, d)  d, c, b, a
// ing3, 低位在前 ，64位windows走这里
#else
# define ZEND_ENDIAN_LOHI(lo, hi)          lo; hi;
# define ZEND_ENDIAN_LOHI_3(lo, mi, hi)    lo; mi; hi;
# define ZEND_ENDIAN_LOHI_4(a, b, c, d)    a; b; c; d;
# define ZEND_ENDIAN_LOHI_C(lo, hi)        lo, hi
# define ZEND_ENDIAN_LOHI_C_3(lo, mi, hi)  lo, mi, hi,
# define ZEND_ENDIAN_LOHI_C_4(a, b, c, d)  a, b, c, d
#endif

// 普通bool型
typedef bool zend_bool;
// 无符号char型
typedef unsigned char zend_uchar;

// 函数执行结果
typedef enum {
  SUCCESS =  0,
  // 失败时必须返回负数，否则会影响函数运行
  FAILURE = -1,		/* this MUST stay a negative number, or it may affect functions! */
} ZEND_RESULT_CODE;

// zend_result 枚举
typedef ZEND_RESULT_CODE zend_result;

// 如果是64位系统
#ifdef ZEND_ENABLE_ZVAL_LONG64
	// windows 操作系统 
# ifdef ZEND_WIN32
#  define ZEND_SIZE_MAX  _UI64_MAX
	// 其他操作系统 
# else
#  define ZEND_SIZE_MAX  SIZE_MAX
# endif
// 其他情况 32位
#else
	// windows 操作系统 
# if defined(ZEND_WIN32)
#  define ZEND_SIZE_MAX  _UI32_MAX
	// 其他操作系统
# else
#  define ZEND_SIZE_MAX SIZE_MAX
# endif
#endif

// 转成int型的指针
typedef intptr_t zend_intptr_t;
// 转成 无符号 int型的指针
typedef uintptr_t zend_uintptr_t;
// 如果要求线程安全
#ifdef ZTS
// ?
#define ZEND_TLS static TSRM_TLS
#define ZEND_EXT_TLS TSRM_TLS
#else
#define ZEND_TLS static
#define ZEND_EXT_TLS
#endif

// 对象处理器
typedef struct _zend_object_handlers zend_object_handlers;
// 类入口
typedef struct _zend_class_entry     zend_class_entry;
// zend 函数 /Zend/zend_compile.h	定义
typedef union  _zend_function        zend_function;
// 执行上下文
typedef struct _zend_execute_data    zend_execute_data;

// zval 内置变量
typedef struct _zval_struct     zval;

// 这些都是很常用的结构体
typedef struct _zend_refcounted zend_refcounted;
// 常用数据类型，字串、数组、对象、资源、引用、语句计数器，语句
typedef struct _zend_string     zend_string;
typedef struct _zend_array      zend_array;
typedef struct _zend_object     zend_object;
typedef struct _zend_resource   zend_resource;
typedef struct _zend_reference  zend_reference;
typedef struct _zend_ast_ref    zend_ast_ref;
typedef struct _zend_ast        zend_ast;

// 比较函数的抽象形式
typedef int  (*compare_func_t)(const void *, const void *);
// 交换函数 
typedef void (*swap_func_t)(void *, void *);
// 排序函数
typedef void (*sort_func_t)(void *, size_t, size_t, compare_func_t, swap_func_t);
// 销毁器
typedef void (*dtor_func_t)(zval *pDest);
// 复制创建新对象，构造器
typedef void (*copy_ctor_func_t)(zval *pElement);

// zend_type是一个抽象层，代表关于type hint的信息
// 不要直接使用它，要使用 ZEND_TYPE_* 开头的宏操作
/*
   zend_type 是一个抽象层，来描述 type 的信息
 * zend_type - is an abstraction layer to represent information about type hint.

   不要直接引用它，通过 ZEND_TYPE_* 宏来引用
 * It shouldn't be used directly. Only through ZEND_TYPE_* macros.
 *
 // 检查是否有有效的type存在
 * ZEND_TYPE_IS_SET()        - checks if there is a type-hint

 // type存在，并且是标准type（不能是指针: (t).ptr == NULL)）
 * ZEND_TYPE_IS_ONLY_MASK()  - checks if type-hint refer to standard type only

 // 检验是type是否是一个列表，或者包含 一个作为ce或者名称的 类
 * ZEND_TYPE_IS_COMPLEX()    - checks if type is a type_list, or contains a class either as a CE or as a name


 * ZEND_TYPE_HAS_NAME()      - checks if type-hint contains some class as zend_string *
 * ZEND_TYPE_IS_INTERSECTION() - checks if the type_list represents an intersection type list
 * ZEND_TYPE_IS_UNION()      - checks if the type_list represents a union type list
 *
 * ZEND_TYPE_NAME()       - returns referenced class name
 * ZEND_TYPE_PURE_MASK()  - returns MAY_BE_* type mask
 * ZEND_TYPE_FULL_MASK()  - returns MAY_BE_* type mask together with other flags
 *
 * ZEND_TYPE_ALLOW_NULL() - checks if NULL is allowed
 *
 * ZEND_TYPE_INIT_*() should be used for construction.
 */

// 组合类型，这是个蛮巧妙的东西，可以方便地计算各种类型的交并集
typedef struct {
	// 这里不用union，因为找不到一个同时兼容c和c++的初始化方法。那种初始化只有在c++20中支持
	/* Not using a union here, because there's no good way to initialize them
	 * in a way that is supported in both C and C++ (designated initializers
	 * are only supported since C++20). */
	// 一个泛指针
	void *ptr;
	// 类型掩码，32位无符号整数 
	uint32_t type_mask;
	/* TODO: We could use the extra 32-bit of padding on 64-bit systems. */
} zend_type;

// 类型列表，主要在编译和继承时用到
typedef struct {
	// 数量
	uint32_t num_types;
	// 列表
	zend_type types[1];
} zend_type_list;

// 类型附加标识偏移量
#define _ZEND_TYPE_EXTRA_FLAGS_SHIFT 25
// 类型掩码，左侧25个1
#define _ZEND_TYPE_MASK ((1u << 25) - 1)
// 下面这几个单选一个
/* Only one of these bits may be set. */
// 名称类型。第24位。
#define _ZEND_TYPE_NAME_BIT (1u << 24)
// 列表类型，第22位。
#define _ZEND_TYPE_LIST_BIT (1u << 22)
// 名称或列表类型标记。 第22或24位
#define _ZEND_TYPE_KIND_MASK (_ZEND_TYPE_LIST_BIT|_ZEND_TYPE_NAME_BIT)
// 可迭代类型。第21位。
/* For BC behaviour with iterable type */
#define _ZEND_TYPE_ITERABLE_BIT (1u << 21)
// arena 分配的类型。第20位。
/* Whether the type list is arena allocated */
#define _ZEND_TYPE_ARENA_BIT (1u << 20)
// 交叉 类型。第19位。
/* Whether the type list is an intersection type */
#define _ZEND_TYPE_INTERSECTION_BIT (1u << 19)
// 是否是联合类型 。第18位。
/* Whether the type is a union type */
#define _ZEND_TYPE_UNION_BIT (1u << 18)

// 不可以包含上面定义的标识， 保留最右面18个1（左移18位到第19位，-1得到18个1）. 
// 前17个类型是指 MAY_BE_UNDEF:0, MAY_BE_NULL:1, MAY_BE_FALSE:2, MAY_BE_TRUE:3, MAY_BE_LONG:4, MAY_BE_DOUBLE:5, MAY_BE_STRING:6, MAY_BE_ARRAY:7, MAY_BE_OBJECT:8, MAY_BE_RESOURCE:9, MAY_BE_REF:10, MAY_BE_CALLABLE:12, MAY_BE_VOID:14, MAY_BE_NEVER:17, MAY_BE_STATIC:15。 MAY_BE_NEVER是最后一个（测试过：(MAY_BE_NEVER << 1) & _ZEND_TYPE_MAY_BE_MASK = 0）
/* Type mask excluding the flags above. */
#define _ZEND_TYPE_MAY_BE_MASK ((1u << 18) - 1)

// 必须和 MAY_BE_NULL 有相同的值，2
/* Must have same value as MAY_BE_NULL */
#define _ZEND_TYPE_NULLABLE_BIT 0x2u

// clear, 是否存在有效的 基础类型 ,_ZEND_TYPE_MASK 左侧25个1
#define ZEND_TYPE_IS_SET(t) \
	(((t).type_mask & _ZEND_TYPE_MASK) != 0)

// ing3, 检验是否是复杂类型, 复杂类型是指：传入的是（一个列表加一个(union 或 intersection)） 或者 （类名）
/* If a type is complex it means it's either a list with a union or intersection,
 * or the void pointer is a class name */
#define ZEND_TYPE_IS_COMPLEX(t) \
	((((t).type_mask) & _ZEND_TYPE_KIND_MASK) != 0)

// ing3, 检验是否名称类型
#define ZEND_TYPE_HAS_NAME(t) \
	((((t).type_mask) & _ZEND_TYPE_NAME_BIT) != 0)

// ing3, 检验是否包含列表类型标记
#define ZEND_TYPE_HAS_LIST(t) \
	((((t).type_mask) & _ZEND_TYPE_LIST_BIT) != 0)

// ing3, 检验是否是可迭代类型
#define ZEND_TYPE_IS_ITERABLE_FALLBACK(t) \
	((((t).type_mask) & _ZEND_TYPE_ITERABLE_BIT) != 0)

// ing3, 检验是交叉类型
#define ZEND_TYPE_IS_INTERSECTION(t) \
	((((t).type_mask) & _ZEND_TYPE_INTERSECTION_BIT) != 0)

// ing3, 检验是联合类型
#define ZEND_TYPE_IS_UNION(t) \
	((((t).type_mask) & _ZEND_TYPE_UNION_BIT) != 0)

// ing3, 检验是 ARENA
#define ZEND_TYPE_USES_ARENA(t) \
	((((t).type_mask) & _ZEND_TYPE_ARENA_BIT) != 0)

// ing4, 是有效的基础类型，并且指针为空
#define ZEND_TYPE_IS_ONLY_MASK(t) \
	(ZEND_TYPE_IS_SET(t) && (t).ptr == NULL)

// ing4, 取回附加指针，类型转换为字串指针
#define ZEND_TYPE_NAME(t) \
	((zend_string *) (t).ptr)

// ing4, 取回附加指针，类型转换为char指针
#define ZEND_TYPE_LITERAL_NAME(t) \
	((const char *) (t).ptr)

// ing4, 取回附加指针，类型转换为 type_list 指针
#define ZEND_TYPE_LIST(t) \
	((zend_type_list *) (t).ptr)

// ing4, 获取一个 包含 num_types 个元素的 zend_type列表 的大小
#define ZEND_TYPE_LIST_SIZE(num_types) \
	(sizeof(zend_type_list) + ((num_types) - 1) * sizeof(zend_type))

// ing4, 迭代一个 zend_type 列表
/* This iterates over a zend_type_list. */
#define ZEND_TYPE_LIST_FOREACH(list, type_ptr) do { \
	zend_type *_list = (list)->types; \
	zend_type *_end = _list + (list)->num_types; \
	for (; _list < _end; _list++) { \
		type_ptr = _list;

// clear, 循环语句的结尾大括号
#define ZEND_TYPE_LIST_FOREACH_END() \
	} \
} while (0)

// ing4, 这个迭代器用于任何 zend_type. 如果是 zend_type 列表，所有列表元素都会被访问到。
// 如果是个单独的 zend_type ,就只访问它一个
/* This iterates over any zend_type. If it's a type list, all list elements will
 * be visited. If it's a single type, only the single type is visited. */
#define ZEND_TYPE_FOREACH(type, type_ptr) do { \
	zend_type *_cur, *_end; \
	/* 如果包含列表 */ \
	if (ZEND_TYPE_HAS_LIST(type)) { \
		/* 取回 zend_type_list */ \
		zend_type_list *_list = ZEND_TYPE_LIST(type); \
		/* 设置起始位置 */ \
		_cur = _list->types; \
		/* 设置结束位置 */ \
		_end = _cur + _list->num_types; \
	/* 如果不包含列表 */ \
	} else { \
		/* 设置起始位置 */ \
		_cur = &(type); \
		/* 设置结束位置 */ \
		_end = _cur + 1; \
	} \
	do { \
		/* type_ptr 依次指向每个元素 */ \
		type_ptr = _cur;

// ing4, 迭代器后半段。
#define ZEND_TYPE_FOREACH_END() \
	/* 迭代条件 */ \
	} while (++_cur < _end); \
} while (0)

// ing4, 更新p1的指针 ((t).ptr = (_ptr))
#define ZEND_TYPE_SET_PTR(t, _ptr) \
	((t).ptr = (_ptr))

// ing3, 给类型设置类入口和type_mask
#define ZEND_TYPE_SET_PTR_AND_KIND(t, _ptr, kind_bit) do { \
	(t).ptr = (_ptr); \
	/* 删除：名称或列表类型标记 */ \
	(t).type_mask &= ~_ZEND_TYPE_KIND_MASK; \
	/* 添加新类型 */ \
	(t).type_mask |= (kind_bit); \
} while (0)

// ing3, 给列表类型设置类入口和 type_mask
#define ZEND_TYPE_SET_LIST(t, list) \
	ZEND_TYPE_SET_PTR_AND_KIND(t, list, _ZEND_TYPE_LIST_BIT)

// FULL_MASK() 包含 MAY_BE_* 格式的类型码，和附加的 元数据字节
// PURE_MASK() 只包含 MAY_BE_* 格式的类型码
/* FULL_MASK() includes the MAY_BE_* type mask, as well as additional metadata bits.
 * The PURE_MASK() only includes the MAY_BE_* type mask. */

// ing4, 返回zend_type 的类型码 
#define ZEND_TYPE_FULL_MASK(t) \
	((t).type_mask)

// ing4, 返回zend_type 的纯 类型码 
// _ZEND_TYPE_MAY_BE_MASK 前17个类型是指 MAY_BE_UNDEF:0, MAY_BE_NULL:1, MAY_BE_FALSE:2, MAY_BE_TRUE:3, MAY_BE_LONG:4, MAY_BE_DOUBLE:5, MAY_BE_STRING:6, MAY_BE_ARRAY:7, MAY_BE_OBJECT:8, MAY_BE_RESOURCE:9, MAY_BE_REF:10, MAY_BE_CALLABLE:12, MAY_BE_VOID:14, MAY_BE_NEVER:17, MAY_BE_STATIC:15
#define ZEND_TYPE_PURE_MASK(t) \
	((t).type_mask & _ZEND_TYPE_MAY_BE_MASK)

// ing4, 返回zend_type 的类型码，去掉NULL
#define ZEND_TYPE_FULL_MASK_WITHOUT_NULL(t) \
	((t).type_mask & ~_ZEND_TYPE_NULLABLE_BIT)

// ing4, 返回zend_type 的纯 类型码 ，去掉NULL
#define ZEND_TYPE_PURE_MASK_WITHOUT_NULL(t) \
	((t).type_mask & _ZEND_TYPE_MAY_BE_MASK & ~_ZEND_TYPE_NULLABLE_BIT)

// ing4, 检查 p1的类型中 是否包含需要的类型 p2
#define ZEND_TYPE_CONTAINS_CODE(t, code) \
	(((t).type_mask & (1u << (code))) != 0)

// ing3, 检查是否可以是 null
#define ZEND_TYPE_ALLOW_NULL(t) \
	(((t).type_mask & _ZEND_TYPE_NULLABLE_BIT) != 0)

// ing3, 创建一个无指针， 带 extra_flags 的 zend_type
#define ZEND_TYPE_INIT_NONE(extra_flags) \
	{ NULL, (extra_flags) }

// ing3, 创建一个无指针，带 _type_mask 的 zend_type
#define ZEND_TYPE_INIT_MASK(_type_mask) \
	{ NULL, (_type_mask) }

// ing3, 定义类型 type_mask ：p1:允许的类型，p2:允null，p3:额外标记
#define ZEND_TYPE_INIT_CODE(code, allow_null, extra_flags) \
	/* bool 或 迭代 或 any 或（1 << code）| allow_null | extra_flags */ \
	ZEND_TYPE_INIT_MASK(((code) == _IS_BOOL ? MAY_BE_BOOL : ( (code) == IS_ITERABLE ? _ZEND_TYPE_ITERABLE_BIT : ((code) == IS_MIXED ? MAY_BE_ANY : (1 << (code))))) \
		| ((allow_null) ? _ZEND_TYPE_NULLABLE_BIT : 0) | (extra_flags))

// ing3, 创建类型，带指针。自定义允null和type_mack。  只有下面两个宏用到 
#define ZEND_TYPE_INIT_PTR(ptr, type_kind, allow_null, extra_flags) \
	{ (void *) (ptr), \
		(type_kind) | ((allow_null) ? _ZEND_TYPE_NULLABLE_BIT : 0) | (extra_flags) }

// ing3, 创建类型，带指针，自定义整个 type_mask
#define ZEND_TYPE_INIT_PTR_MASK(ptr, type_mask) \
	{ (void *) (ptr), (type_mask) }

// ing3, 创建 联合类型，带指针，自定义 extra_flags
#define ZEND_TYPE_INIT_UNION(ptr, extra_flags) \
	{ (void *) (ptr), (_ZEND_TYPE_LIST_BIT|_ZEND_TYPE_UNION_BIT) | (extra_flags) }

// ing3, 创建 交叉类型，带指针，自定义 extra_flags
#define ZEND_TYPE_INIT_INTERSECTION(ptr, extra_flags) \
	{ (void *) (ptr), (_ZEND_TYPE_LIST_BIT|_ZEND_TYPE_INTERSECTION_BIT) | (extra_flags) }

// ing3, 创建 有类名类型，自定义允null, extra_flags
#define ZEND_TYPE_INIT_CLASS(class_name, allow_null, extra_flags) \
	ZEND_TYPE_INIT_PTR(class_name, _ZEND_TYPE_NAME_BIT, allow_null, extra_flags)

// ing3, 和上面一样
#define ZEND_TYPE_INIT_CLASS_CONST(class_name, allow_null, extra_flags) \
	ZEND_TYPE_INIT_PTR(class_name, _ZEND_TYPE_NAME_BIT, allow_null, extra_flags)

// ing3, 创建 有类名类型，自定义 type_mask
#define ZEND_TYPE_INIT_CLASS_CONST_MASK(class_name, type_mask) \
	ZEND_TYPE_INIT_PTR_MASK(class_name, _ZEND_TYPE_NAME_BIT | (type_mask))

// ing2, 所有基础类型都在里面了，所谓弱类型，主要是靠它支持咯？
typedef union _zend_value {
	// 长整型
	zend_long         lval;				/* long value */
	// 双精度
	double            dval;				/* double value */
	// 引用计数器
	zend_refcounted  *counted;
	// 字串
	zend_string      *str;
	// 数组
	zend_array       *arr;
	// 对象
	zend_object      *obj;
	// 资源
	zend_resource    *res;
	// 地址引用
	zend_reference   *ref;
	// 基本的引用计数器，做什么用的？上面不是已经有一个计数器了么
	zend_ast_ref     *ast;
	// 内置变量，php程序不可访问。用到这个的，类型会是间接引用 IS_INDIRECT
	zval             *zv;
	// 指针
	void             *ptr;
	// 类入口
	zend_class_entry *ce;
	// 函数
	zend_function    *func;
	// 
	struct {
		// 全局没有用到
		uint32_t w1;
		// ZVAL_COPY_VALUE 用到
		uint32_t w2;
	} ww;
} zend_value;
/*
IS_INDIRECT 是 _zend_value 里面的 zv 元素，它 指向一个内置变量，内置变量是实际应用的变量
IS_REFERENCE 是 _zend_value 里面的 ref 元素， 它指向一个 zend_reference 对象， zend_reference对象里面有一个 val 属性是 zval 变量（不是指针）
*/


// zval is the alias, 核心是 value ,这层包装主要是记录运行状态
struct _zval_struct {
	// 弱类型变量
	zend_value        value;			/* value */
	// 
	union {
		uint32_t type_info;
		// 这里放个struct是为了打包成和 uint32_t 相同的大小
		struct {
			// 高位或低位在前，共4字节。要兼容 uint32_t 所以要用 ZEND_ENDIAN_LOHI_3 打包。
			ZEND_ENDIAN_LOHI_3(
				// 1字节
				zend_uchar    type,			/* active type */
				// 1字节
				zend_uchar    type_flags,
				// 2字节，这里放个union其实木啥用了，里面只有一个类型
				union {
					uint16_t  extra;        /* not further specified */
				} u)
		} v;
	} u1;
	// 全是 int32。都是运行状态相关标记。
	union {
		// ing3, 哈希冲突链。哈希表里用它指向同样哈希值的下一个元素! 
		// 为了哈希表给每个内置变量加个指针，可见php对哈希表的重视。
		uint32_t     next;                 /* hash collision chain */
		//
		uint32_t     cache_slot;           /* cache slot (for RECV_INIT) */
		// 操作码编号，用于快速调用
		uint32_t     opline_num;           /* opline number (for FAST_CALL) */
		// 行号， 为了语句节点
		uint32_t     lineno;               /* line number (for ast nodes) */
		// 执行时用到的参数数量 ？
		uint32_t     num_args;             /* arguments number for EX(This) */
		// foreach 位置
		uint32_t     fe_pos;               /* foreach position */
		// foreach 迭代序号
		uint32_t     fe_iter_idx;          /* foreach iterator index */
		// 成员对象保护
		uint32_t     property_guard;       /* single property guard */
		// 常量标记
		uint32_t     constant_flags;       /* constant flags */
		// 额外数据，少量调用 （不再指定？）
		uint32_t     extra;                /* not further specified */
	} u2;
};

// ing4, gc计数器，由两个32位整数组成。这个结构体主要给内部引用，包装在其他 struct 里。所以这是一个头信息~
typedef struct _zend_refcounted_h {
	// 引用次数
	uint32_t         refcount;			/* reference counter 32-bit */
	// 类型信息存放在这里, 整型 gc.u.type_info( 参看 GC_TYPE_INFO(p) )
	// 为什么放个union ？
	union {
		// 用来做位标记，32位整数
			//分成4段使用：最左2位表示颜色，次左20位表示地址索引号，最右4位是type(类型),次右6位是flags
		uint32_t type_info;
	} u;
} zend_refcounted_h;

// 只封装一个 zend_refcounted_h 计数器，供外部调用，这个被引用的次数并不多
struct _zend_refcounted {
	zend_refcounted_h gc;
};

// 带引用计数器的 字串
struct _zend_string {
	// 引用计数器
	zend_refcounted_h gc;
	// 哈希值。为了用作数组key，给每个字串加个哈希值！
	zend_ulong        h;                /* hash value */
	// 长度,这是它的占用空间长度
	size_t            len;
	// 为什么不是 char *val? 因为如果是指针，就需要给指向位置再分配内存，那就不能包在一个 _zend_string 里面了，内存控制也更麻烦
	// 为了动态分配内存
	char              val[1];
};

// 哈希表的一个键值对
typedef struct _Bucket {
	// 值
	zval              val;
	// 哈希值 或 数字索引号
	zend_ulong        h;                /* hash value (or numeric index)   */
	// 键：字串型key，数字型保存 NULL
	zend_string      *key;              /* string key or NULL for numerics */
} Bucket;

// hashTable 是 zend_array 的别名
typedef struct _zend_array HashTable;

// array 和 hashtable. 哈希表可能在很多地方被创建
struct _zend_array {
	// 引用计数器
	zend_refcounted_h gc;
	// ？what is this?
	union {
		// 只有 struct 里面可以用 ZEND_ENDIAN_LOHI_4，所以这里定义 struct
		struct {
			// 唯一一处调用，只是改变一下顺序，四个元素同样类型 
			ZEND_ENDIAN_LOHI_4(
				zend_uchar    flags,
				zend_uchar    _unused,
				// 哈希表的迭代器数量
				zend_uchar    nIteratorsCount,
				zend_uchar    _unused2)
		} v;
		// int : 标记
		uint32_t flags;
	} u;
	// -(nTableSize*2)
	uint32_t          nTableMask;
	// 为什么要 union ?
	union {
		// #define HT_HASH(ht, idx) HT_HASH_EX((ht)->arHash, idx) // 全局只有这里用到
		// ( key 的哈希值 | mask) 得到在这各指针中的位置，里面存着对应的元素序号。
		// 哈希值的列表（分配在这个指针下）
		uint32_t     *arHash;   /* hash table (allocated above this pointer) */
		// 哈希表中的数据，bucket的列表，多个键值对。
		// 这部分比较复杂，顺序数组里没有 Bucket, 另外有一部分和 tablemask 相关
		Bucket       *arData;   /* array of hash buckets */
		// 顺序数组
		zval         *arPacked; /* packed array of zvals */
	};
	// 已使用数量
	uint32_t          nNumUsed;
	// 元素数量
	uint32_t          nNumOfElements;
	// 大小
	uint32_t          nTableSize;
	// 内部指针
	uint32_t          nInternalPointer;
	// 下一个空元素
	zend_long         nNextFreeElement;
	// 销毁函数
	dtor_func_t       pDestructor;
};

// 哈希表数据布局, 下面这一堆宏对哈希表的理解至关重要，尤其是 arHash 相关部分。 

/*
 * HashTable Data Layout
 * =====================
 *
			哈希表前面是哈希值，后面是bucket列表，数量相同。		索引数组前面是固定2个无效哈希值，后面是zval列表
 *                 +=============================+
 *                 | HT_HASH(ht, ht->nTableMask) |                   +=============================+
 *                 | ...                         |                   | HT_INVALID_IDX              |
 *                 | HT_HASH(ht, -1)             |                   | HT_INVALID_IDX              |
 *                 +-----------------------------+                   +-----------------------------+
 * ht->arData ---> | Bucket[0]                   | ht->arPacked ---> | ZVAL[0]                     |
 *                 | ...                         |                   | ...                         |
 *                 | Bucket[ht->nTableSize-1]    |                   | ZVAL[ht->nTableSize-1]      |
 *                 +=============================+                   +=============================+
 */

// 无效索引号：32个1 。4294967295 = ((uint32_t) -1) = 2147483648*2-1
#define HT_INVALID_IDX ((uint32_t) -1)

// 哈希表，最小掩码：31个1 + 1个0 ， 4294967294 = ((uint32_t) -2) = 2147483648*2-2 
#define HT_MIN_MASK ((uint32_t) -2)
// -2 对应 哈希->索引表 最少有2个元素。为什么是-2？

// 哈希表最少 8 个元素
#define HT_MIN_SIZE 8

// HT_MAX_SIZE 的选择必须满足下列约束：
/* HT_MAX_SIZE is chosen to satisfy the following constraints:
// 
 * - HT_SIZE_TO_MASK(HT_MAX_SIZE) != 0
//
 * - HT_SIZE_EX(HT_MAX_SIZE, HT_SIZE_TO_MASK(HT_MAX_SIZE)) does not overflow or
 *   wrapparound, and is <= the addressable space size
// 必须是2的幂
 * - HT_MAX_SIZE must be a power of two:
// 
 *   (nTableSize<HT_MAX_SIZE ? nTableSize+nTableSize : nTableSize) <= HT_MAX_SIZE
 */ 
// ------- 如果 size_t 长度是 4位，32位系统
#if SIZEOF_SIZE_T == 4
// 最大长度 2*16*16*16*16*16*16*16 =  32M 
# define HT_MAX_SIZE 0x02000000
 
// ing3, 通过指针和偏移量找到Bucket 指针。返回第 idx+1 个字符的指针并转成（Bucket类型）
# define HT_HASH_TO_BUCKET_EX(data, idx) \
	// 32位系统要先把 data 转成 char型指针，why ?
	((Bucket*)((char*)(data) + (idx)))
// ing3, 32位系统，计算存放 Bucket 用的内存大小。 序号 * Bucket大小。
# define HT_IDX_TO_HASH(idx) \
	((idx) * sizeof(Bucket))
// ing3, 计算 bucket 数量 , index / Bucket大小
# define HT_HASH_TO_IDX(idx) \
	((idx) / sizeof(Bucket))
	
	
// ------- 如果 size_t 长度是 8位，64位系统
#elif SIZEOF_SIZE_T == 8
// 最大长度 4*16*16*16*16*16*16*16*16 = 1G
# define HT_MAX_SIZE 0x40000000
// ing4, 通过指针和偏移量找到Bucket 指针
# define HT_HASH_TO_BUCKET_EX(data, idx) \
	((data) + (idx))
// ing4, 64位系统中 index 不用转换
# define HT_IDX_TO_HASH(idx) \
	(idx)
// ing4, 64位系统中 index 不用转换
# define HT_HASH_TO_IDX(idx) \
	(idx)
// 不支持其他类型系统
#else
# error "Unknown SIZEOF_SIZE_T"
#endif

// ing4, 找到哈希->索引号列表 中的指定索引号。p1:列表结尾位置，p2:顺序号（和 tableMask 做过或运算的哈希值, 负数）
#define HT_HASH_EX(data, idx) \
	/* 这里把idx转成有符号后是负数，所以是在data里倒着查找 */ \
	((uint32_t*)(data))[(int32_t)(idx)]
	
// ing4, 找到 (p1->arHash) 哈希->索引列表 中的指定索引号。p1:哈希表，p2:顺序号（和mask做过或运算, 负数）
#define HT_HASH(ht, idx) \
	/* 找到哈希->索引列表 中的指定索引号。p1:列表结尾位置，p2:顺序号（和mask做过或运算, 负数） */ \
	HT_HASH_EX((ht)->arHash, idx)

// ing4, 通过哈希表大小 计算 mask = (uint32_t)(-2 * nTableSize)
// 这里同样要保证 nTableSize 是2的倍数。这样mask是右侧连续的n个0，左侧全部是1。这样mask才有意义。
	// 例如 4 => -8 => 11111111 11111111 1111111 11111000
	// 例如 16 => -32 => 11111111 11111111 1111111 11100000
// 这样有什么好处呢？
#define HT_SIZE_TO_MASK(nTableSize) \
	((uint32_t)(-((nTableSize) + (nTableSize))))


// ing4, 把掩码换算成哈希值列表大小。例如，掩码HT_MIN_MASK=-2转成正数为2，计算结果为2*4 =8
// （哈希值类型是 uint32_t，32位，占4字节，参见 _zend_array 定义。 测试过）
// 只有当前页用调用。
#define HT_HASH_SIZE(nTableMask) \
	/* ms vistual studio 中不可以对无符号转负数，写成 int32_t 也能测试通过 */ \ 
	(((size_t)-(uint32_t)(nTableMask)) * sizeof(uint32_t))
	
// ing4, 哈希表 Bucket 列表 大小： 哈希表大小 * Bucket大小。 只有 HT_SIZE_EX用到 
#define HT_DATA_SIZE(nTableSize) \
	((size_t)(nTableSize) * sizeof(Bucket))
	
// ing4, 计算 哈希表 数据大小：Bucket 列表大小 + 哈希->索引 列表大小 。p1:哈希表元素数, p2:mask
#define HT_SIZE_EX(nTableSize, nTableMask) \
	/* 哈希表 Bucket 列表 大小： 哈希表大小 * Bucket大小 */ \
	/* 通过 nTableMask 计算 哈希->索引列表 大小 */ \
	(HT_DATA_SIZE((nTableSize)) + HT_HASH_SIZE((nTableMask)))

// ing4, 计算 哈希表 数据大小：Bucket 列表大小 + 哈希->索引 列表大小。p1:哈希表
// 分配内存时调用
#define HT_SIZE(ht) \
	/* 计算 哈希表 数据大小：Bucket 列表大小 + 哈希->索引 列表大小 */ \
	HT_SIZE_EX((ht)->nTableSize, (ht)->nTableMask)

// ing4, 已使用大小 nTableMask 部分算已用
#define HT_USED_SIZE(ht) \
	/* 通过 nTableMask 计算 哈希->索引列表 大小 */ \
	(HT_HASH_SIZE((ht)->nTableMask) + ((size_t)(ht)->nNumUsed * sizeof(Bucket)))
	
// ing4, 顺序数组的zval列表大小：元素数 * zval大小。p1:元素数
#define HT_PACKED_DATA_SIZE(nTableSize) \
	((size_t)(nTableSize) * sizeof(zval))
	
// ing4, 计算顺序数组的 数据大小: zval列表 大小 + 哈希->索引列表大小。p1:元素数，p2:mask
#define HT_PACKED_SIZE_EX(nTableSize, nTableMask) \
	/* 顺序数组的zval列表大小：元素数 * zval大小。p1:元素数 */ \
	/* 通过 nTableMask 计算 哈希->索引列表 大小 */ \
	(HT_PACKED_DATA_SIZE((nTableSize)) + HT_HASH_SIZE((nTableMask)))
	
// ing4, 计算顺序数组的 数据大小: 顺序数据大小 + 哈希->索引列表大小。p1:哈希表
#define HT_PACKED_SIZE(ht) \
	HT_PACKED_SIZE_EX((ht)->nTableSize, (ht)->nTableMask)
	
// ing4, 顺序数组的已使用部分大小，哈希->索引列表 部分算已使用。p1:哈希表
#define HT_PACKED_USED_SIZE(ht) \
	/* 通过 nTableMask 计算 哈希->索引列表 大小 */ \
	(HT_HASH_SIZE((ht)->nTableMask) + ((size_t)(ht)->nNumUsed * sizeof(zval)))

// windows没有这个
#ifdef __SSE2__
// 重置 arHash
# define HT_HASH_RESET(ht) do { \
		char *p = (char*)&HT_HASH(ht, (ht)->nTableMask); \
		/* 通过 nTableMask 计算 哈希->索引列表 大小 */ \
		size_t size = HT_HASH_SIZE((ht)->nTableMask); \
		__m128i xmm0 = _mm_setzero_si128(); \
		xmm0 = _mm_cmpeq_epi8(xmm0, xmm0); \
		ZEND_ASSERT(size >= 64 && ((size & 0x3f) == 0)); \
		do { \
			_mm_storeu_si128((__m128i*)p, xmm0); \
			_mm_storeu_si128((__m128i*)(p+16), xmm0); \
			_mm_storeu_si128((__m128i*)(p+32), xmm0); \
			_mm_storeu_si128((__m128i*)(p+48), xmm0); \
			p += 64; \
			size -= 64; \
		} while (size != 0); \
	} while (0)
// windows 走这里
#else
// ing4, 重置 (p1)->arHash 哈希->索引列表。 所有元素 都设置成 -1。p1:哈希表
# define HT_HASH_RESET(ht) \
	/* 把整个 哈希->索引列表 都设置成 HT_INVALID_IDX = -1 */ \
	/* 找到 (p1->arHash) 哈希->索引列表 中的指定索引号。p1:哈希表，p2:顺序号（和mask做过或运算, 负数） */ \
	/* 通过 nTableMask 计算 哈希->索引列表 大小 */ \
	/* memset 的 p3必须是 字节数，所以要计算 哈希->索引列表 大小 */ \
	memset(&HT_HASH(ht, (ht)->nTableMask), HT_INVALID_IDX, HT_HASH_SIZE((ht)->nTableMask))
#endif

// ing4, 顺序数组的 (p1)->arHash 哈希->索引列表 里有2个元素，都设置成 -1。p1:哈希表
#define HT_HASH_RESET_PACKED(ht) do { \
		/* 负数索引号，指针确实可以这样一直向左取，测试过 */ \
		/* 找到 (p1->arHash) 哈希->索引列表 中的指定索引号。p1:哈希表，p2:顺序号（和mask做过或运算, 负数） */ \
		/* HT_INVALID_IDX = -1 */ \
		HT_HASH(ht, -2) = HT_INVALID_IDX; \
		HT_HASH(ht, -1) = HT_INVALID_IDX; \
	} while (0)
		
// ing4, 找到哈希表中第 idx 个bucket。p1:哈希表，p2:bucket序号
#define HT_HASH_TO_BUCKET(ht, idx) \
	HT_HASH_TO_BUCKET_EX((ht)->arData, idx)

// ing4, 设置 p1->arData 到 Bucket 列表开头。p1:哈希表，p2: 内存块开头位置
#define HT_SET_DATA_ADDR(ht, ptr) do { \
		/* 跳过 哈希->索引列表 大小 */ \
		/* 通过 nTableMask 计算 哈希->索引列表 大小 */ \
		(ht)->arData = (Bucket*)(((char*)(ptr)) + HT_HASH_SIZE((ht)->nTableMask)); \
	} while (0)
		
// ing4, 从p1->arData找到哈希表数据块开头位置。p1:哈希表。数据块前半部分是 哈希->索引列表
#define HT_GET_DATA_ADDR(ht) \
	/* 先把指针转成(char *),数据块的大小是按byte算 */ \
	/* 通过 nTableMask 计算 哈希->索引列表 大小 */ \
	/* 指针向左移动 【哈希->索引列表 大小】的位置，找到数据块开头 */ \
	((char*)((ht)->arData) - HT_HASH_SIZE((ht)->nTableMask))

// 哈希表位置，int
typedef uint32_t HashPosition;

// 迭代器。一般都是运行时创建，放在 EG(ht_iterators) 里
typedef struct _HashTableIterator {
	// 哈希表指针
	HashTable    *ht;
	// 位置，int型
	HashPosition  pos;
} HashTableIterator;

// ing2， php 对象
// zend_object 都是在 execute执行阶段，在虚拟机里创建的，怪不得编译阶段完全没有这个东东
struct _zend_object {
	// 引用计数
	zend_refcounted_h gc;
	// handle 是对象在全局对象容器 EG(objects_store).object_buckets 中的【索引号】，通过它可以直接在全局容器中获取指针
	uint32_t          handle; // TODO: may be removed ???
	// 所属类入口
	zend_class_entry *ce;
	// 对象操作句柄，只读
	const zend_object_handlers *handlers;
	// 对象的【附加属性】是存在哈希表里的 ？
	HashTable        *properties;
	// 默认属性表，这里可以放任意多个属性（zval），对象被创建的时候它就创建好了
	zval              properties_table[1];
};

// ing4， resource 对象
struct _zend_resource {
	// 引用次数
	zend_refcounted_h gc;
	// 可删除，实际上这个是有有用到的
	zend_long         handle; // TODO: may be removed ???
	// 类型
	int               type;
	// 指针
	void             *ptr;
};

// 类属性信息（zend_property_info）列表 
// size_t 长度是8，所以由3个 64位整数 组成，共24byte（测试过）
// 它后面根了一大堆 _zend_property_info，通过 ptr来操作这些 _zend_property_info
typedef struct {
	size_t num;
	size_t num_allocated;
	struct _zend_property_info *ptr[1];
} zend_property_info_list;

// 执行时少量调用
// 这个 union 的长度是8，
// 如果把 union改成struct，长度是16。union相当于是一个指针，无论里面放多少东西，长度都是8
// union 和struct的区别到底是什么呢，struct 放在里面有什么用呢？
typedef union {
	struct _zend_property_info *ptr;
	uintptr_t list;
} zend_property_info_source_list;

// ing3, 把指针转成整数并把最后一位写成 1。 zend_property_info_list 大小是24，最后3位是0。
#define ZEND_PROPERTY_INFO_SOURCE_FROM_LIST(list) (0x1 | (uintptr_t) (list))
// ing3, 把 list 转成 zend_property_info_list 指针（去掉最后一位的1）
#define ZEND_PROPERTY_INFO_SOURCE_TO_LIST(list) ((zend_property_info_list *) ((list) & ~0x1))
// ing3, 检验 属性信息源 是否是 列表（最后 1 位是 1）
#define ZEND_PROPERTY_INFO_SOURCE_IS_LIST(list) ((list) & 0x1)

// 引用
struct _zend_reference {
	// 引用次数
	zend_refcounted_h              gc;
	// 变量，引用结构体中包含一个内置变量，而不是变量指针，通过这个变量指向目标
	zval                           val;
	// 成员变量信息？
	zend_property_info_source_list sources;
};

// 只封装一个gc计数器
struct _zend_ast_ref {
	// 两个32位整数 ，占8个字节
	zend_refcounted_h gc;
	// zend_ast 兼容 zend_ast_ref的结构。zend_ast的开头是两个16位整数+1个32位整数，共64位，和 _zend_ast_ref 大小一样。
	/*zend_ast        ast; zend_ast follows the zend_ast_ref structure */
};

// 普通数据类型，需要与 zend_variables.c 一致
/* Regular data types: Must be in sync with zend_variables.c. */
// undefine， 未定义
#define IS_UNDEF					0
// null
#define IS_NULL						1
// php 内置变量 true 和 false 是两个类型
#define IS_FALSE					2
#define IS_TRUE						3
// php 长整型
#define IS_LONG						4
// php 双精度
#define IS_DOUBLE					5
// php 字串
#define IS_STRING					6
// php 数组
#define IS_ARRAY					7
// php 对象
#define IS_OBJECT					8
// php 资源
#define IS_RESOURCE					9
// php 地址引用
#define IS_REFERENCE				10
// php 常量表达式
#define IS_CONSTANT_AST				11 /* Constant expressions */

// 假类型，用来提供指向
/* Fake types used only for type hinting.
// 可以和下面的类型叠加
 * These are allowed to overlap with the types below. */
// php 闭包
#define IS_CALLABLE					12
// php 可迭代对象
#define IS_ITERABLE					13
// return后面无内容 的 function 返回
#define IS_VOID						14
// static 标记
#define IS_STATIC					15
// php 混合类型标识
#define IS_MIXED					16
// 无 return语句 的 function 返回。
// function a():never {}, 编译不会出错 ，但只要调用就会出错。这个类型比较特殊。（测试过）
#define IS_NEVER					17

// 内部类型
/* internal types */
// 间接变量
#define IS_INDIRECT             	12
// 指针
#define IS_PTR						13
// 别名指针
#define IS_ALIAS_PTR				14
// 错误
#define _IS_ERROR					15

// 用来做类型转换
/* used for casts */
// 布尔型
#define _IS_BOOL					18
// 数字字串？
#define _IS_NUMBER					19

// ing4, 取得变量类型，通过宏大量调用
static zend_always_inline zend_uchar zval_get_type(const zval* pz) {
	// 类型本来就是 zend_uchar 类型， typedef unsigned char zend_uchar;
	return pz->u1.v.type;
}
// ing4 (全局无调用), 比较两个类型是否相同，如果 faketype 是bool型，realtype 可以是 IS_TRUE 或 IS_FALSE
#define ZEND_SAME_FAKE_TYPE(faketype, realtype) ( \
	(faketype) == (realtype) \
	|| ((faketype) == _IS_BOOL && ((realtype) == IS_TRUE || (realtype) == IS_FALSE)) \
)

/* we should never set just Z_TYPE, we should set Z_TYPE_INFO */
// z_开头的是 zval的操作方法，以下方法都是读取 zval 的 成员变量
// 宏是可以直接用来赋值的，所以这里没有set 方法
// ing4, 访问zval 的类型 . (u1.v.type, u1.v.type_flags, u1.type_info) 三个和类型有关，要仔细区分一下
#define Z_TYPE(zval)				zval_get_type(&(zval))
// ing4, 通过指针访问zval 的类型
#define Z_TYPE_P(zval_p)			Z_TYPE(*(zval_p))

// type_flags 和 type_info 区别是什么呢？
	// 主要用的是 type_info。 type_flags很少用到。

// ing4, 访问zval 的 类型标记，内部引用
#define Z_TYPE_FLAGS(zval)			(zval).u1.v.type_flags
// ing4, 通过指针访问zval 的 类型标记，zend_compile.c 一次引用。ext少量引用。
#define Z_TYPE_FLAGS_P(zval_p)		Z_TYPE_FLAGS(*(zval_p))

// ing4, 访问zval 的 类型信息，外部少量引用
#define Z_TYPE_INFO(zval)			(zval).u1.type_info
// ing4, 通过指针访问zval 的 类型信息，外部极大量引用
#define Z_TYPE_INFO_P(zval_p)		Z_TYPE_INFO(*(zval_p))

// ing4, 访问zval 哈希表里同样哈希值的下一个元素的指针 (哈希表里用它指向同样哈希值的下一个元素!)
#define Z_NEXT(zval)				(zval).u2.next
// ing4, 通过指针访问访问zval 哈希表里同样哈希值的下一个元素的指针
#define Z_NEXT_P(zval_p)			Z_NEXT(*(zval_p))

// ing3, 获取 (zval).u2.cache_slot
#define Z_CACHE_SLOT(zval)			(zval).u2.cache_slot
#define Z_CACHE_SLOT_P(zval_p)		Z_CACHE_SLOT(*(zval_p))

// ing4, 访问zval的 行号
#define Z_LINENO(zval)				(zval).u2.lineno
// ing4, 通过指针访问zval的 行号
#define Z_LINENO_P(zval_p)			Z_LINENO(*(zval_p))

// ing4, 访问zval的 操作码序号
#define Z_OPLINE_NUM(zval)			(zval).u2.opline_num
// ing4, 通过指针访问zval的 操作码序号
#define Z_OPLINE_NUM_P(zval_p)		Z_OPLINE_NUM(*(zval_p))

// ing4, 访问zval的 foreach位置 p1.u2.fe_pos
#define Z_FE_POS(zval)				(zval).u2.fe_pos
// ing4, 通过指针访问zval的 foreach位置。(*p1).u2.fe_pos
#define Z_FE_POS_P(zval_p)			Z_FE_POS(*(zval_p))

// ing4, 访问zval的 迭代索引号, (p1).u2.fe_iter_idx
#define Z_FE_ITER(zval)				(zval).u2.fe_iter_idx
// ing4, 通过指针访问zval的 迭代索引号 , (*p1).u2.fe_iter_idx
#define Z_FE_ITER_P(zval_p)			Z_FE_ITER(*(zval_p))

// ing3, 获取 (zval).u2.property_guard （uint32_t）
#define Z_PROPERTY_GUARD(zval)		(zval).u2.property_guard
#define Z_PROPERTY_GUARD_P(zval_p)	Z_PROPERTY_GUARD(*(zval_p))

// ing4, 访问zval的 常量标记
#define Z_CONSTANT_FLAGS(zval)		(zval).u2.constant_flags
// ing4, 通过指针访问zval的 常量标记
#define Z_CONSTANT_FLAGS_P(zval_p)	Z_CONSTANT_FLAGS(*(zval_p))

// ing4, 访问zval的 扩展数据
#define Z_EXTRA(zval)				(zval).u2.extra
// ing4, 通过指针访问zval的 扩展数据
#define Z_EXTRA_P(zval_p)			Z_EXTRA(*(zval_p))

// ing4, zval 的 引用计数器 
#define Z_COUNTED(zval)				(zval).value.counted
// ing4, （指针）zval 的 引用计数器 
#define Z_COUNTED_P(zval_p)			Z_COUNTED(*(zval_p))

// 类型掩码 8个1
#define Z_TYPE_MASK					0xff
// 类型标识掩码，8个1 + 8个0
#define Z_TYPE_FLAGS_MASK			0xff00

// 类型标记转换位数
#define Z_TYPE_FLAGS_SHIFT			8

// gc 开头的函数， 都是和垃圾回收有关吗？
// ing4, 取得指针的计数器 .gc
#define GC_REFCOUNT(p)				zend_gc_refcount(&(p)->gc)
// ing4, 给指针设置计数器
#define GC_SET_REFCOUNT(p, rc)		zend_gc_set_refcount(&(p)->gc, rc)
// ing4, 指针的引用计数 +1。 ++(p->refcount)
#define GC_ADDREF(p)				zend_gc_addref(&(p)->gc)
// ing4, 减少本对象的gc属性的引用次数。 --(p->refcount)
#define GC_DELREF(p)				zend_gc_delref(&(p)->gc)
// ing4, 增加引用次数 rc 次
#define GC_ADDREF_EX(p, rc)			zend_gc_addref_ex(&(p)->gc, rc)
// ing4, 减少引用次数 rc 次
#define GC_DELREF_EX(p, rc)			zend_gc_delref_ex(&(p)->gc, rc)
// ing4, 增加可变实例（没有GC_IMMUTABLE）的引用次数
#define GC_TRY_ADDREF(p)			zend_gc_try_addref(&(p)->gc)
// ing4, 减少可变实例（没有GC_IMMUTABLE）的引用次数
#define GC_TRY_DELREF(p)			zend_gc_try_delref(&(p)->gc)

// 0000 0000 0000 0000 0000 0000 0000 1111
#define GC_TYPE_MASK				0x0000000f
#define GC_FLAGS_MASK				0x000003f0
#define GC_INFO_MASK				0xfffffc00
#define GC_FLAGS_SHIFT				0
#define GC_INFO_SHIFT				10

// ing4, 获取 type_info 最右4位
static zend_always_inline zend_uchar zval_gc_type(uint32_t gc_type_info) {
	// 0000 0000 0000 0000 0000 0000 0000 1111
	return (gc_type_info & GC_TYPE_MASK);
}

// ing4, 取得 type_info 右侧 5-10位（共6位），右侧有4个空位
static zend_always_inline uint32_t zval_gc_flags(uint32_t gc_type_info) {
	// 0000 0000 0000 0000 0000 0011 1111 0000
	// gc_type_info & GC_FLAGS_MASK
	return (gc_type_info >> GC_FLAGS_SHIFT) & (GC_FLAGS_MASK >> GC_FLAGS_SHIFT);
}

// ing4, 取得 type_info 最左侧22位，通过右移清除右侧空位
static zend_always_inline uint32_t zval_gc_info(uint32_t gc_type_info) {
	return (gc_type_info >> GC_INFO_SHIFT);
}

// gc_type_info 不同于 zval的 type_info。
// ing4, 访问 gc属性（zend_refcounted_h） 中的 type_info
// 一共32位，分成下面3段。
#define GC_TYPE_INFO(p)				(p)->gc.u.type_info
// ing4, 获取 gc属性（zend_refcounted_h） 中的type_info 最右4位
#define GC_TYPE(p)					zval_gc_type(GC_TYPE_INFO(p))
// ing4, 取得 gc属性（zend_refcounted_h） 中的type_info 右侧 5-10位（共6位），右侧有4个空位
#define GC_FLAGS(p)					zval_gc_flags(GC_TYPE_INFO(p))
// ing4, 取得 gc属性（zend_refcounted_h） 中的type_info 最左侧22位，通过右移清除右侧空位。用法见zend_gc.c
#define GC_INFO(p)					zval_gc_info(GC_TYPE_INFO(p))


// ing3, 给 .gc.u.type_info 添加标记 
#define GC_ADD_FLAGS(p, flags) do { \
		GC_TYPE_INFO(p) |= (flags) << GC_FLAGS_SHIFT; \
	} while (0)
		
// ing3, 给 .gc.u.type_info 删除标记 
#define GC_DEL_FLAGS(p, flags) do { \
		GC_TYPE_INFO(p) &= ~((flags) << GC_FLAGS_SHIFT); \
	} while (0)

// #define Z_COUNTED(zval)				(zval).value.counted

// >>> Z_GC_* 是通过 Z_COUNTED(zval) 获取元素。这堆宏只有定义，完全没有用到
// ing3, 获取 (zval).value.counted.gc.u.type_info
#define Z_GC_TYPE(zval)				GC_TYPE(Z_COUNTED(zval))
// ing3, 通过指针获取 (zval).value.counted.gc.u.type_info
#define Z_GC_TYPE_P(zval_p)			Z_GC_TYPE(*(zval_p))

// ing3, 参见上面 GC_FLAGS
#define Z_GC_FLAGS(zval)			GC_FLAGS(Z_COUNTED(zval))
#define Z_GC_FLAGS_P(zval_p)		Z_GC_FLAGS(*(zval_p))
// ing3, 参见上面 GC_INFO
#define Z_GC_INFO(zval)				GC_INFO(Z_COUNTED(zval))
#define Z_GC_INFO_P(zval_p)			Z_GC_INFO(*(zval_p))
// ing3, 参见上面 GC_TYPE_INFO
#define Z_GC_TYPE_INFO(zval)		GC_TYPE_INFO(Z_COUNTED(zval))
#define Z_GC_TYPE_INFO_P(zval_p)	Z_GC_TYPE_INFO(*(zval_p))
// <<<


/* zval_gc_flags(zval.value->gc.u.type_info) (common flags) */
#define GC_NOT_COLLECTABLE			(1<<4)
// 用于检测递归
#define GC_PROTECTED                (1<<5) /* used for recursion detection */
// 不能修改
#define GC_IMMUTABLE                (1<<6) /* can't be changed in place */
// 使用 malloc 分配内存
#define GC_PERSISTENT               (1<<7) /* allocated using malloc */
// 本地持久:持久但本地线程？
#define GC_PERSISTENT_LOCAL         (1<<8) /* persistent, but thread-local */

// #define GC_FLAGS_SHIFT				0
// gc 是垃圾回收（garbage collect）
// 不可回收的null
#define GC_NULL						(IS_NULL         | (GC_NOT_COLLECTABLE << GC_FLAGS_SHIFT))
// 不可回收的STRING
#define GC_STRING					(IS_STRING       | (GC_NOT_COLLECTABLE << GC_FLAGS_SHIFT))
// 这两个和普通的类型一样
#define GC_ARRAY					IS_ARRAY
#define GC_OBJECT					IS_OBJECT
// 不可回收的 资源类型
#define GC_RESOURCE					(IS_RESOURCE     | (GC_NOT_COLLECTABLE << GC_FLAGS_SHIFT))
// 不可回收的 资引用型
#define GC_REFERENCE				(IS_REFERENCE    | (GC_NOT_COLLECTABLE << GC_FLAGS_SHIFT))
// 不可回收的 AST ？
#define GC_CONSTANT_AST				(IS_CONSTANT_AST | (GC_NOT_COLLECTABLE << GC_FLAGS_SHIFT))

/* zval.u1.v.type_flags */
#define IS_TYPE_REFCOUNTED			(1<<0)
// 可垃圾回收 标记
#define IS_TYPE_COLLECTABLE			(1<<1)

#if 1
// 这个优化版本 假定 我们有 独立的 type_flag
/* This optimized version assumes that we have a single "type_flag" */
// IS_TYPE_COLLECTABLE 只能和 IS_TYPE_REFCOUNTED 一起用
/* IS_TYPE_COLLECTABLE may be used only with IS_TYPE_REFCOUNTED */
// #define Z_TYPE_FLAGS_MASK			0xff00
// ing4, 通过 type_info 确定是否可记录引用数。 type_info 的左十六位只要有值，就是可计数
# define Z_TYPE_INFO_REFCOUNTED(t)	(((t) & Z_TYPE_FLAGS_MASK) != 0)
#else
// IS_TYPE_REFCOUNTED 标记不在这里用了
# define Z_TYPE_INFO_REFCOUNTED(t)	(((t) & (IS_TYPE_REFCOUNTED << Z_TYPE_FLAGS_SHIFT)) != 0)
#endif

/* extended types */
// 扩展类型
#define IS_INTERNED_STRING_EX		IS_STRING

#define IS_STRING_EX				(IS_STRING         | (IS_TYPE_REFCOUNTED << Z_TYPE_FLAGS_SHIFT))
#define IS_ARRAY_EX					(IS_ARRAY          | (IS_TYPE_REFCOUNTED << Z_TYPE_FLAGS_SHIFT) | (IS_TYPE_COLLECTABLE << Z_TYPE_FLAGS_SHIFT))
#define IS_OBJECT_EX				(IS_OBJECT         | (IS_TYPE_REFCOUNTED << Z_TYPE_FLAGS_SHIFT) | (IS_TYPE_COLLECTABLE << Z_TYPE_FLAGS_SHIFT))
#define IS_RESOURCE_EX				(IS_RESOURCE       | (IS_TYPE_REFCOUNTED << Z_TYPE_FLAGS_SHIFT))
#define IS_REFERENCE_EX				(IS_REFERENCE      | (IS_TYPE_REFCOUNTED << Z_TYPE_FLAGS_SHIFT))

#define IS_CONSTANT_AST_EX			(IS_CONSTANT_AST   | (IS_TYPE_REFCOUNTED << Z_TYPE_FLAGS_SHIFT))

/* string flags (zval.value->gc.u.flags) */
#define IS_STR_CLASS_NAME_MAP_PTR   GC_PROTECTED  /* refcount is a map_ptr offset of class_entry */
// 间接的字串 和 GC_IMMUTABLE 同义
#define IS_STR_INTERNED				GC_IMMUTABLE  /* interned string */
// 持久的字串，使用 malloc 分配 ？
#define IS_STR_PERSISTENT			GC_PERSISTENT /* allocated using malloc */
// 永久的字串
#define IS_STR_PERMANENT        	(1<<8)        /* relives request boundary */
// 支持 utf8
#define IS_STR_VALID_UTF8           (1<<9)        /* valid UTF-8 according to PCRE */

// array 标记，不可转换，持久保存
/* array flags */
// 不可变的数组
#define IS_ARRAY_IMMUTABLE			GC_IMMUTABLE
// 持久的数组
#define IS_ARRAY_PERSISTENT			GC_PERSISTENT

/* object flags (zval.value->gc.u.flags) */
// 弱持久引用？
#define IS_OBJ_WEAKLY_REFERENCED	GC_PERSISTENT
// 销毁过的对象
#define IS_OBJ_DESTRUCTOR_CALLED	(1<<8)
// 释放过内存的对象
#define IS_OBJ_FREE_CALLED			(1<<9)

// GC_FLAGS：取得 gc属性（zend_refcounted_h） 中的type_info 右侧 5-10位（共6位），右侧有4个空位
// ing4, 别名
#define OBJ_FLAGS(obj)              GC_FLAGS(obj)

// 快速类缓存 
/* Fast class cache */
// ing3, 查检是否是ce缓存位置：对象的type_info中指定位置有 IS_STR_CLASS_NAME_MAP_PTR 标记
#define ZSTR_HAS_CE_CACHE(s)		(GC_FLAGS(s) & IS_STR_CLASS_NAME_MAP_PTR)

// ing4, 在 CG(map_ptr_base) 通过偏移量（p1的引用计数）取回 类入口指针，带校验
#define ZSTR_GET_CE_CACHE(s)		ZSTR_GET_CE_CACHE_EX(s, 1)

// ing4, 在 CG(map_ptr_base) 通过偏移量（p1的引用计数）定入 类入口指针，带校验
#define ZSTR_SET_CE_CACHE(s, ce)	ZSTR_SET_CE_CACHE_EX(s, ce, 1)

// ing3, 校验偏移量（p1的引用计数） ，看它是否在已用指针范围内
#define ZSTR_VALID_CE_CACHE(s)		EXPECTED((GC_REFCOUNT(s)-1)/sizeof(void *) < CG(map_ptr_last))

// ing4, 在 CG(map_ptr_base) 通过偏移量（s的计数）取回 类入口指针，带校验
#define ZSTR_GET_CE_CACHE_EX(s, validate) \
	((!(validate) || ZSTR_VALID_CE_CACHE(s)) ? GET_CE_CACHE(GC_REFCOUNT(s)) : NULL)

// ing4, 在 CG(map_ptr_base) 通过偏移量（s的计数）设置 类入口指针，带校验
#define ZSTR_SET_CE_CACHE_EX(s, ce, validate) do { \
		if (!(validate) || ZSTR_VALID_CE_CACHE(s)) { \
			ZEND_ASSERT((validate) || ZSTR_VALID_CE_CACHE(s)); \
			SET_CE_CACHE(GC_REFCOUNT(s), ce); \
		} \
	} while (0)

// ing4, 在 CG(map_ptr_base) 通过偏移量取回 类入口指针
#define GET_CE_CACHE(ce_cache) \
	/* 根据偏移量（按char * 算）取回指定次序的指针	*/ \
	(*(zend_class_entry **)ZEND_MAP_PTR_OFFSET2PTR(ce_cache))

// ing4, 在 CG(map_ptr_base) 指针列表通过偏移量写入 类入口指针
#define SET_CE_CACHE(ce_cache, ce) do { \
		/* 根据偏移量（按char * 算）取回指定次序的指针	*/ \
		*((zend_class_entry **)ZEND_MAP_PTR_OFFSET2PTR(ce_cache)) = ce; \
	} while (0)

// 递归保护宏，只能用在 数组和哈希表上
/* Recursion protection macros must be used only for arrays and objects */
// ing4, 检查是否有递归（GC_PROTECTED）
#define GC_IS_RECURSIVE(p) \
	(GC_FLAGS(p) & GC_PROTECTED)

// ing4, 添加递归标记（GC_PROTECTED）
#define GC_PROTECT_RECURSION(p) do { \
		GC_ADD_FLAGS(p, GC_PROTECTED); \
	} while (0)

// ing4, 删除递归标记（GC_PROTECTED）
#define GC_UNPROTECT_RECURSION(p) do { \
		GC_DEL_FLAGS(p, GC_PROTECTED); \
	} while (0)

// ing4, 如果可更改，添加递归标记
#define GC_TRY_PROTECT_RECURSION(p) do { \
		if (!(GC_FLAGS(p) & GC_IMMUTABLE)) GC_PROTECT_RECURSION(p); \
	} while (0)

// ing4, 如果可更改，删除递归标记
#define GC_TRY_UNPROTECT_RECURSION(p) do { \
		if (!(GC_FLAGS(p) & GC_IMMUTABLE)) GC_UNPROTECT_RECURSION(p); \
	} while (0)

// ing4, zval.value.counted 是否有递归标记
#define Z_IS_RECURSIVE(zval)        GC_IS_RECURSIVE(Z_COUNTED(zval))
// ing4, zval.value.counted 添加递归标记
#define Z_PROTECT_RECURSION(zval)   GC_PROTECT_RECURSION(Z_COUNTED(zval))
// ing4, zval.value.counted 删除递归标记
#define Z_UNPROTECT_RECURSION(zval) GC_UNPROTECT_RECURSION(Z_COUNTED(zval))
// ing4, 通过指针检查 zval.value.counted 是否有递归标记
#define Z_IS_RECURSIVE_P(zv)        Z_IS_RECURSIVE(*(zv))
// ing4, 通过指针给 zval.value.counted 添加递归标记
#define Z_PROTECT_RECURSION_P(zv)   Z_PROTECT_RECURSION(*(zv))
// ing4, 通过指针给 zval.value.counted 删除递归标记
#define Z_UNPROTECT_RECURSION_P(zv) Z_UNPROTECT_RECURSION(*(zv))

// 所有 小于 IS_STRING 的数据类型 跳过 构造方法和析构方法
/* All data types < IS_STRING have their constructor/destructors skipped */
// ing4, 检验是否是常量表达式
#define Z_CONSTANT(zval)			(Z_TYPE(zval) == IS_CONSTANT_AST)
// ing4, 通过指针检验是否是常量表达式
#define Z_CONSTANT_P(zval_p)		Z_CONSTANT(*(zval_p))

#if 1
// 这个优化后的版本设想 有一个独立的type_flag， IS_TYPE_COLLECTABLE 可以和 IS_TYPE_REFCOUNTED 一起用
/* This optimized version assumes that we have a single "type_flag" */
/* IS_TYPE_COLLECTABLE may be used only with IS_TYPE_REFCOUNTED */

// ing3, 检验是否有有效的type_flags , (zval).u1.v.type_flags
#define Z_REFCOUNTED(zval)			(Z_TYPE_FLAGS(zval) != 0)
#else
// 无效逻辑
#define Z_REFCOUNTED(zval)			((Z_TYPE_FLAGS(zval) & IS_TYPE_REFCOUNTED) != 0)
#endif

// ing3, 通过指针检验是否有有效的type_flags
#define Z_REFCOUNTED_P(zval_p)		Z_REFCOUNTED(*(zval_p))

// ing3, 验证 IS_TYPE_COLLECTABLE, 是否可回收
#define Z_COLLECTABLE(zval)			((Z_TYPE_FLAGS(zval) & IS_TYPE_COLLECTABLE) != 0)
// ing3, 通过指针，验证目标的 IS_TYPE_COLLECTABLE
#define Z_COLLECTABLE_P(zval_p)		Z_COLLECTABLE(*(zval_p))

// 弃用 COPYABLE 与 IS_ARRAY 相同
/* deprecated: (COPYABLE is the same as IS_ARRAY) */
// ing4, 验证 zval->u1.v.type 是否是 IS_ARRAY
#define Z_COPYABLE(zval)			(Z_TYPE(zval) == IS_ARRAY)
// ing4, 验证指针目标 zval->u1.v.type 是否是 IS_ARRAY
#define Z_COPYABLE_P(zval_p)		Z_COPYABLE(*(zval_p))

// 弃用 IMMUTABLE 与 IS_ARRAY && !REFCOUNTED 相同
/* deprecated: (IMMUTABLE is the same as IS_ARRAY && !REFCOUNTED) */
// ing4, 检验zval 类型是否是数组 
#define Z_IMMUTABLE(zval)			(Z_TYPE_INFO(zval) == IS_ARRAY)
// ing4, 通过指针检验zval 类型是否是数组 
#define Z_IMMUTABLE_P(zval_p)		Z_IMMUTABLE(*(zval_p))
// ing4, 别名，检验zval 类型是否是数组 
#define Z_OPT_IMMUTABLE(zval)		Z_IMMUTABLE(zval_p)
// ing4, 通过指针检验zval 类型是否是数组 
#define Z_OPT_IMMUTABLE_P(zval_p)	Z_IMMUTABLE(*(zval_p))

// 如果 Z_TYPE_INFO 已经被访问过 ，Z_OPT_ 开头的宏 更好一些。（OPT_ 是 optimized 的缩写）
/* the following Z_OPT_* macros make better code when Z_TYPE_INFO accessed before */

// ing4, 获取 type_info,整数
#define Z_OPT_TYPE(zval)			(Z_TYPE_INFO(zval) & Z_TYPE_MASK)
#define Z_OPT_TYPE_P(zval_p)		Z_OPT_TYPE(*(zval_p))

// ing4, zval 类型是否是常量语句
#define Z_OPT_CONSTANT(zval)		(Z_OPT_TYPE(zval) == IS_CONSTANT_AST)
// ing4, zval 类型是否是常量语句
#define Z_OPT_CONSTANT_P(zval_p)	Z_OPT_CONSTANT(*(zval_p))

// ing4, 是否是可以计引用数的类型
#define Z_OPT_REFCOUNTED(zval)		Z_TYPE_INFO_REFCOUNTED(Z_TYPE_INFO(zval))
// ing4, 是否是可以计引用数的类型
#define Z_OPT_REFCOUNTED_P(zval_p)	Z_OPT_REFCOUNTED(*(zval_p))

// 弃用 COPYABLE 与 IS_ARRAY 相同（确实无调用）
/* deprecated: (COPYABLE is the same as IS_ARRAY) */
// ing4, 检验 zval的type_info 是否是 IS_ARRAY
#define Z_OPT_COPYABLE(zval)		(Z_OPT_TYPE(zval) == IS_ARRAY)
#define Z_OPT_COPYABLE_P(zval_p)	Z_OPT_COPYABLE(*(zval_p))

// ing4, 验证 zval的type 是否是 IS_REFERENCE
#define Z_OPT_ISREF(zval)			(Z_OPT_TYPE(zval) == IS_REFERENCE)
// ing4, 通过指针验证 zval的type 是否是 IS_REFERENCE
#define Z_OPT_ISREF_P(zval_p)		Z_OPT_ISREF(*(zval_p))

// ing4, 验证 zval的type 是否是 IS_REFERENCE
#define Z_ISREF(zval)				(Z_TYPE(zval) == IS_REFERENCE)
// ing4, 通过指针验证 zval的type 是否是 IS_REFERENCE
#define Z_ISREF_P(zval_p)			Z_ISREF(*(zval_p))

// ing4, 验证 zval的type 是否是 IS_UNDEF
#define Z_ISUNDEF(zval)				(Z_TYPE(zval) == IS_UNDEF)
// ing4, 通过指针验证 zval的type 是否是 IS_UNDEF
#define Z_ISUNDEF_P(zval_p)			Z_ISUNDEF(*(zval_p))

// ing4, 验证 zval的type 是否是 IS_NULL
#define Z_ISNULL(zval)				(Z_TYPE(zval) == IS_NULL)
// ing4, 通过指针验证 zval的type 是否是 IS_NULL
#define Z_ISNULL_P(zval_p)			Z_ISNULL(*(zval_p))

// ing4, 验证 zval的type 是否是 _IS_ERROR
#define Z_ISERROR(zval)				(Z_TYPE(zval) == _IS_ERROR)
// ing4, 通过指针验证 zval的type 是否是 _IS_ERROR
#define Z_ISERROR_P(zval_p)			Z_ISERROR(*(zval_p))

// clear, 访问 zval中的 long 型指针
#define Z_LVAL(zval)				(zval).value.lval
// clear, 通过指针访问 zval中的 long 型指针
#define Z_LVAL_P(zval_p)			Z_LVAL(*(zval_p))

// clear, 访问 zval中的  double 型指针
#define Z_DVAL(zval)				(zval).value.dval
// clear, 通过指针访问 zval中的 double 型指针
#define Z_DVAL_P(zval_p)			Z_DVAL(*(zval_p))

// clear, 访问 zval中的 字串指针
#define Z_STR(zval)					(zval).value.str
// clear, 通过指针访问 zval值 中的 字串指针
#define Z_STR_P(zval_p)				Z_STR(*(zval_p))

// ing4, 先从zval里取出str指针，再从str指针获取 val是char数组，不是指针
// strval是这里来的  #define ZSTR_VAL(zstr) (zstr)->val
#define Z_STRVAL(zval)				ZSTR_VAL(Z_STR(zval))
// ing4, 通过指针获取zval中字串的值
#define Z_STRVAL_P(zval_p)			Z_STRVAL(*(zval_p))

// ing4, 获取 zval中的字串长度
#define Z_STRLEN(zval)				ZSTR_LEN(Z_STR(zval))
// ing4, 通过指针获取 zval中的字串长度
#define Z_STRLEN_P(zval_p)			Z_STRLEN(*(zval_p))

// ing4, 访问 zval 中字串的哈希值
#define Z_STRHASH(zval)				ZSTR_HASH(Z_STR(zval))
// ing4, 通过指针访问 zval 中字串的哈希值
#define Z_STRHASH_P(zval_p)			Z_STRHASH(*(zval_p))

// clear, 访问 zval中的 数组指针
#define Z_ARR(zval)					(zval).value.arr
// clear, 通过指针访问 zval中的数组指针
#define Z_ARR_P(zval_p)				Z_ARR(*(zval_p))

// clear, 访问 zval中的 数组指针, 和 Z_ARR 一毛一样
#define Z_ARRVAL(zval)				Z_ARR(zval)
// clear, 通过指针访问 zval中的数组指针
#define Z_ARRVAL_P(zval_p)			Z_ARRVAL(*(zval_p))

// clear, 访问 zval中的对象指针
#define Z_OBJ(zval)					(zval).value.obj
// clear, 通过指针访问 zval中的对象指针
#define Z_OBJ_P(zval_p)				Z_OBJ(*(zval_p))

// ing4, 访问 zval中的对象指针的操作方法集(HT,看起来像HASHTABLE)	
#define Z_OBJ_HT(zval)				Z_OBJ(zval)->handlers
// ing4, 通过指针访问 zval中的对象指针的操作方法集
#define Z_OBJ_HT_P(zval_p)			Z_OBJ_HT(*(zval_p))

// ing4, 访问 zval中的对象指针的操作方法集，再访问集合里的指定方法
#define Z_OBJ_HANDLER(zval, hf)		Z_OBJ_HT((zval))->hf
// ing4, 通过指针访问 zval中的对象指针的操作方法集，再访问集合里的指定方法
#define Z_OBJ_HANDLER_P(zv_p, hf)	Z_OBJ_HANDLER(*(zv_p), hf)

// ing4, 访问 zval中zend_object 的 handle 属性
#define Z_OBJ_HANDLE(zval)          (Z_OBJ((zval)))->handle
// ing4, 通过指针访问 zval中zend_object 的 handle 属性
#define Z_OBJ_HANDLE_P(zval_p)      Z_OBJ_HANDLE(*(zval_p))

// ing4, 访问 zval中zend_object 的 类入口
#define Z_OBJCE(zval)				(Z_OBJ(zval)->ce)
// ing4, 通过指针访问 zval中zend_object 的 类入口
#define Z_OBJCE_P(zval_p)			Z_OBJCE(*(zval_p))

// ing4, 通过属性名 zval 获取 zend_object 的 属性值
#define Z_OBJPROP(zval)				Z_OBJ_HT((zval))->get_properties(Z_OBJ(zval))
// ing4, 通过属性名 zval指针 获取 zend_object 的 属性值
#define Z_OBJPROP_P(zval_p)			Z_OBJPROP(*(zval_p))

// ing4, 访问 zval中的资源指针
#define Z_RES(zval)					(zval).value.res
// ing4, 通过指针访问 zval中的资源指针
#define Z_RES_P(zval_p)				Z_RES(*zval_p)

// ing4, zval 泛类型 的 .res资源型 -> handle
#define Z_RES_HANDLE(zval)			Z_RES(zval)->handle
#define Z_RES_HANDLE_P(zval_p)		Z_RES_HANDLE(*zval_p)

// ing4, zval 泛类型 的 .res资源型 -> type
#define Z_RES_TYPE(zval)			Z_RES(zval)->type
#define Z_RES_TYPE_P(zval_p)		Z_RES_TYPE(*zval_p)

// ing4, zval 泛类型 的 .res资源型 -> ptr
#define Z_RES_VAL(zval)				Z_RES(zval)->ptr
#define Z_RES_VAL_P(zval_p)			Z_RES_VAL(*zval_p)

// ing4, zval 泛类型 的 .ref引用型(zend_reference)
#define Z_REF(zval)					(zval).value.ref
#define Z_REF_P(zval_p)				Z_REF(*(zval_p))

// ing4, zval 泛类型 的 .ref引用型(zend_reference) -> val (zval 型）
#define Z_REFVAL(zval)				&Z_REF(zval)->val
#define Z_REFVAL_P(zval_p)			Z_REFVAL(*(zval_p))

// ing4, zval 泛类型 的 常量表达式
#define Z_AST(zval)					(zval).value.ast
#define Z_AST_P(zval_p)				Z_AST(*(zval_p))

// 下面这3个宏，用得很少。zend_ast_ref：带引用计数器的 zend_ast
// ing3, 把指针p向右移动 sizeof(zend_ast_ref) 这么多，再转成 zend_ast指针，返回
#define GC_AST(p)					((zend_ast*)(((char*)p) + sizeof(zend_ast_ref)))

// ing3, 取回zval中的 表达式语句 （zend_ast 实例）
#define Z_ASTVAL(zval)				GC_AST(Z_AST(zval))
// ing3 通过指针 取回zval中的 表达式语句 （zend_ast 实例）
#define Z_ASTVAL_P(zval_p)			Z_ASTVAL(*(zval_p))

// 这个甚至不算引用类型，更像指针类型
// ing4, zval 泛类型 的 .zv内置变量指针，指向一个 zval 对象。 对应的类型是 IS_INDIRECT
#define Z_INDIRECT(zval)			(zval).value.zv
// ing4, 取得指针指向的zval变量的 zv 属性
#define Z_INDIRECT_P(zval_p)		Z_INDIRECT(*(zval_p))

// ing4, 访问 zval中的类入口
#define Z_CE(zval)					(zval).value.ce
// ing4, 通过指针访问 zval中的类入口
#define Z_CE_P(zval_p)				Z_CE(*(zval_p))

// ing4, 访问 zval中的函数
#define Z_FUNC(zval)				(zval).value.func
// ing4, 通过指针访问 zval中的指针
#define Z_FUNC_P(zval_p)			Z_FUNC(*(zval_p))

// ing4, 访问 zval中的指针
#define Z_PTR(zval)					(zval).value.ptr
// ing4, 通过访问 zval中的指针
#define Z_PTR_P(zval_p)				Z_PTR(*(zval_p))

// ing4, zval 的 type_info 设置成 IS_UNDEF
#define ZVAL_UNDEF(z) do {				\
		Z_TYPE_INFO_P(z) = IS_UNDEF;	\
	} while (0)

// ing4, zval 的 type_info 设置成 IS_NULL
#define ZVAL_NULL(z) do {				\
		Z_TYPE_INFO_P(z) = IS_NULL;		\
	} while (0)

// ing4, zval 的 type_info 设置成 IS_FALSE
#define ZVAL_FALSE(z) do {				\
		Z_TYPE_INFO_P(z) = IS_FALSE;	\
	} while (0)

// ing4, zval 的 type_info 设置成 IS_TRUE
#define ZVAL_TRUE(z) do {				\
		Z_TYPE_INFO_P(z) = IS_TRUE;		\
	} while (0)

// ing4, 把 type_info 设置成 IS_TRUE 或 IS_FALSE
#define ZVAL_BOOL(z, b) do {			\
		Z_TYPE_INFO_P(z) =				\
			(b) ? IS_TRUE : IS_FALSE;	\
	} while (0)

// ing4, 给zval.value.lval 赋值并把 type_info 设置成 IS_LONG
#define ZVAL_LONG(z, l) do {			\
		zval *__z = (z);				\
		Z_LVAL_P(__z) = l;				\
		Z_TYPE_INFO_P(__z) = IS_LONG;	\
	} while (0)

// ing4, 给zval.value.dval 赋值并把 type_info 设置成 IS_DOUBLE
#define ZVAL_DOUBLE(z, d) do {			\
		zval *__z = (z);				\
		Z_DVAL_P(__z) = d;				\
		Z_TYPE_INFO_P(__z) = IS_DOUBLE;	\
	} while (0)

// ing4, 给zval.value.str 赋值 ， 并设置type_info : IS_INTERNED_STRING_EX 或 IS_STRING_EX。支持保留字
#define ZVAL_STR(z, s) do {						\
		zval *__z = (z);						\
		zend_string *__s = (s);					\
		Z_STR_P(__z) = __s;						\
		/* interned strings support */			\
		Z_TYPE_INFO_P(__z) = ZSTR_IS_INTERNED(__s) ? \
			IS_INTERNED_STRING_EX : 			\
			IS_STRING_EX;						\
	} while (0)

// ing4, 给zval.value.str 赋值，并设置type_info
#define ZVAL_INTERNED_STR(z, s) do {				\
		zval *__z = (z);							\
		zend_string *__s = (s);						\
		Z_STR_P(__z) = __s;							\
		Z_TYPE_INFO_P(__z) = IS_INTERNED_STRING_EX;	\
	} while (0)

// ing4, 把zend_string 添加给 zval，不支持保留字
#define ZVAL_NEW_STR(z, s) do {					\
		zval *__z = (z);						\
		zend_string *__s = (s);					\
		Z_STR_P(__z) = __s;						\
		/* 类型为普通字串 */ \
		Z_TYPE_INFO_P(__z) = IS_STRING_EX;		\
	} while (0)

// ing4, 给 zval 添加 string, 并修改type_info（支持保留字），并增加引用次数（这就算copy了）
	// 带 copy字样的有 string, object, zval
#define ZVAL_STR_COPY(z, s) do {						\
		zval *__z = (z);								\
		zend_string *__s = (s);							\
		Z_STR_P(__z) = __s;								\
		/* 支持保留字 */					\
		/* interned strings support */					\
		if (ZSTR_IS_INTERNED(__s)) {					\
			Z_TYPE_INFO_P(__z) = IS_INTERNED_STRING_EX;	\
		} else {										\
			GC_ADDREF(__s);								\
			Z_TYPE_INFO_P(__z) = IS_STRING_EX;			\
		}												\
	} while (0)

// ing4, 给zval 添加 zend_array 实例指针, 并修改type_info
#define ZVAL_ARR(z, a) do {						\
		zend_array *__arr = (a);				\
		zval *__z = (z);						\
		Z_ARR_P(__z) = __arr;					\
		Z_TYPE_INFO_P(__z) = IS_ARRAY_EX;		\
	} while (0)

// ing4, 给zval.value.arr 创建新的永久数组 , 并修改type_info
#define ZVAL_NEW_PERSISTENT_ARR(z) do {							\
		zval *__z = (z);										\
		zend_array *_arr =										\
		(zend_array *) malloc(sizeof(zend_array));				\
		Z_ARR_P(__z) = _arr;									\
		Z_TYPE_INFO_P(__z) = IS_ARRAY_EX;						\
	} while (0)

// ing4, 给zval添加 zend_object 实例指针, 并修改type_info
#define ZVAL_OBJ(z, o) do {						\
		zval *__z = (z);						\
		Z_OBJ_P(__z) = (o);						\
		Z_TYPE_INFO_P(__z) = IS_OBJECT_EX;		\
	} while (0)
		
// ing4, 给zval 复制 zend_object 实例指针, 并修改type_info。与上面不同的是增加了引用次数
#define ZVAL_OBJ_COPY(z, o) do {				\
		zval *__z = (z);						\
		zend_object *__o = (o);					\
		/* 源对象添加引用次数 */ \
		GC_ADDREF(__o);							\
		/* zval.value.obj 指针赋值 */ \
		Z_OBJ_P(__z) = __o;						\
		Z_TYPE_INFO_P(__z) = IS_OBJECT_EX;		\
	} while (0)

// ing4, 给zval添加 zend_resource 实例指针, 并修改type_info
#define ZVAL_RES(z, r) do {						\
		zval *__z = (z);						\
		Z_RES_P(__z) = (r);						\
		Z_TYPE_INFO_P(__z) = IS_RESOURCE_EX;	\
	} while (0)

// ing4, 给 zval 创建新的 zend_reosource 
#define ZVAL_NEW_RES(z, h, p, t) do {							\
		zend_resource *_res =									\
		/* 创建 zend_reosource */ \
		(zend_resource *) emalloc(sizeof(zend_resource));		\
		zval *__z;												\
		/* 引用次数设置成1 */ \
		GC_SET_REFCOUNT(_res, 1);								\
		/* 设置新实例 type_info */ \
		GC_TYPE_INFO(_res) = GC_RESOURCE;						\
		/* 把 h,p,t 装进去 */ \
		_res->handle = (h);										\
		_res->type = (t);										\
		_res->ptr = (p);										\
		__z = (z);												\
		/* 新实例 关联到 (zval)z 里面 */ \
		Z_RES_P(__z) = _res;									\
		/* 设置 zval type_info */ \
		Z_TYPE_INFO_P(__z) = IS_RESOURCE_EX;					\
	} while (0)

// ing4, 创建新的永久 zend_reosource ，逻辑同上
#define ZVAL_NEW_PERSISTENT_RES(z, h, p, t) do {				\
		zend_resource *_res =									\
		(zend_resource *) malloc(sizeof(zend_resource));		\
		zval *__z;												\
		GC_SET_REFCOUNT(_res, 1);								\
		GC_TYPE_INFO(_res) = GC_RESOURCE |						\
			(GC_PERSISTENT << GC_FLAGS_SHIFT);					\
		_res->handle = (h);										\
		_res->type = (t);										\
		_res->ptr = (p);										\
		__z = (z);												\
		Z_RES_P(__z) = _res;									\
		Z_TYPE_INFO_P(__z) = IS_RESOURCE_EX;					\
	} while (0)

// ing4, 给zval.ref 添加 zend_reference 实例指针, 并修改type_info
#define ZVAL_REF(z, r) do {										\
		zval *__z = (z);										\
		Z_REF_P(__z) = (r);										\
		Z_TYPE_INFO_P(__z) = IS_REFERENCE_EX;					\
	} while (0)

// ing4, 给 zval 创建新的空的 zend_reference 
#define ZVAL_NEW_EMPTY_REF(z) do {								\
		zend_reference *_ref =									\
		/* 创建新的 zend_reference实例（自带zval） */ \
		(zend_reference *) emalloc(sizeof(zend_reference));		\
		/* 引用次数设置成1 */ \
		GC_SET_REFCOUNT(_ref, 1);								\
		/* 设置新实例 type_info */ \
		GC_TYPE_INFO(_ref) = GC_REFERENCE;						\
		/* 引用目标为NULL */ \
		_ref->sources.ptr = NULL;									\
		/* 新实例 关联到 (zval)z 里面 */ \
		Z_REF_P(z) = _ref;										\
		/* 设置 zval type_info */ \
		Z_TYPE_INFO_P(z) = IS_REFERENCE_EX;						\
	} while (0)

// ing4, 给 zval 创建新的 zend_reference ，从给出的 zval 中复制附加内容
#define ZVAL_NEW_REF(z, r) do {									\
		/* 创建新的引用实例（自带zval） */ \
		zend_reference *_ref =									\
		(zend_reference *) emalloc(sizeof(zend_reference));		\
		/* 引用数+1 */ \
		GC_SET_REFCOUNT(_ref, 1);								\
		/* 实例类型 */ \
		GC_TYPE_INFO(_ref) = GC_REFERENCE;						\
		/* 目标的值复制给引用实例 */ \
		ZVAL_COPY_VALUE(&_ref->val, r);							\
		/* 有值就用不到指针了 */ \
		_ref->sources.ptr = NULL;									\
		/* 引用实例关联到 zval */ \
		Z_REF_P(z) = _ref;										\
		/* zval 类型为引用  */ \
		Z_TYPE_INFO_P(z) = IS_REFERENCE_EX;						\
	} while (0)

// ing3, 把zval转为引用，p1:zval，p2:引用次数
#define ZVAL_MAKE_REF_EX(z, refcount) do {						\
		zval *_z = (z);											\
		/* 创建引用实例（自带zval） */ \
		zend_reference *_ref =									\
			(zend_reference *) emalloc(sizeof(zend_reference));	\
		/* 自定义引用次数 */ \
		GC_SET_REFCOUNT(_ref, (refcount));						\
		/* 类型是引用 */ \
		GC_TYPE_INFO(_ref) = GC_REFERENCE;						\
		/* 把zval指针复制给 引用实例 */ \
		ZVAL_COPY_VALUE(&_ref->val, _z);						\
		/* 引用实例指针为null */ \
		_ref->sources.ptr = NULL;									\
		/* 引用实例关联到原zval */ \
		Z_REF_P(_z) = _ref;										\
		/* 原zval 类型为引用 */ \
		Z_TYPE_INFO_P(_z) = IS_REFERENCE_EX;					\
	} while (0)

// ing4, 给zval转换成持久引用
#define ZVAL_NEW_PERSISTENT_REF(z, r) do {						\
		zend_reference *_ref =									\
		/* 创建引用实例（自带zval） */ \
		(zend_reference *) malloc(sizeof(zend_reference));		\
		/* 引用次数为1 */ \
		GC_SET_REFCOUNT(_ref, 1);								\
		/* 类型上加一个，持久 */ \
		GC_TYPE_INFO(_ref) = GC_REFERENCE |						\
			(GC_PERSISTENT << GC_FLAGS_SHIFT);					\
		/* 把zval的值复制给 引用实例 */ \
		ZVAL_COPY_VALUE(&_ref->val, r);							\
		/* 引用实例指针为null */ \
		_ref->sources.ptr = NULL;									\
		/* 引用实例关联到原zval */ \
		Z_REF_P(z) = _ref;										\
		/* 原zval转为引用类型 */ \
		Z_TYPE_INFO_P(z) = IS_REFERENCE_EX;						\
	} while (0)

// ing4, 给 zval.ast 添加指针, 并标记成 常量表达式 类型
#define ZVAL_AST(z, ast) do {									\
		zval *__z = (z);										\
		Z_AST_P(__z) = ast;										\
		Z_TYPE_INFO_P(__z) = IS_CONSTANT_AST_EX;				\
	} while (0)

// ing4, 给 zval.zv 添加指针, 并标记成 间接引用 类型
#define ZVAL_INDIRECT(z, v) do {								\
		Z_INDIRECT_P(z) = (v);									\
		Z_TYPE_INFO_P(z) = IS_INDIRECT;							\
	} while (0)

// ing4, 给 zval.ptr 添加指针, 并标记成 指针 类型
#define ZVAL_PTR(z, p) do {										\
		Z_PTR_P(z) = (p);										\
		Z_TYPE_INFO_P(z) = IS_PTR;								\
	} while (0)

// ing4, 给 zval.func 添加指针, 并标记成 指针 类型
#define ZVAL_FUNC(z, f) do {									\
		Z_FUNC_P(z) = (f);										\
		Z_TYPE_INFO_P(z) = IS_PTR;								\
	} while (0)
		
// ing4, 给 zval.ce 添加指针, 并标记成 指针 类型
#define ZVAL_CE(z, c) do {										\
		Z_CE_P(z) = (c);										\
		Z_TYPE_INFO_P(z) = IS_PTR;								\
	} while (0)

// ing4, 给 zval.ptr 添加指针, 并标记成 别名指针 类型
#define ZVAL_ALIAS_PTR(z, p) do {								\
		Z_PTR_P(z) = (p);										\
		Z_TYPE_INFO_P(z) = IS_ALIAS_PTR;						\
	} while (0)

// ing4, 把 zval 标记成 错误 类型
#define ZVAL_ERROR(z) do {				\
		Z_TYPE_INFO_P(z) = _IS_ERROR;	\
	} while (0)

// ing3, zval_refcount_p 的别名，获取引用次数
#define Z_REFCOUNT_P(pz)			zval_refcount_p(pz)
// ing3, zval_set_refcount_p 的别名，设置引用次数
#define Z_SET_REFCOUNT_P(pz, rc)	zval_set_refcount_p(pz, rc)
// ing4, 通过指针 添加 .counted.gc 引用次数
#define Z_ADDREF_P(pz)				zval_addref_p(pz)
// ing4, 通过指针 减少 .counted.gc 引用次数
#define Z_DELREF_P(pz)				zval_delref_p(pz)

// ing3, 转化成指针并获取引用次数
#define Z_REFCOUNT(z)				Z_REFCOUNT_P(&(z))
// ing3, 转化成指针并设置引用次数
#define Z_SET_REFCOUNT(z, rc)		Z_SET_REFCOUNT_P(&(z), rc)
// 指针操作是个大学问，处处都要用到，不是指针都要硬转成指针
// ing4, 添加 .counted.gc 引用次数
#define Z_ADDREF(z)					Z_ADDREF_P(&(z))
// ing4, 减少 .counted.gc 引用次数
#define Z_DELREF(z)					Z_DELREF_P(&(z))

// ing4, 尝试给 pz(指针) 增加引用次数
#define Z_TRY_ADDREF_P(pz) do {		\
	/* 是有计数的类型，才进行增加操作，不会报错 */ \
	if (Z_REFCOUNTED_P((pz))) {		\
		Z_ADDREF_P((pz));			\
	}								\
} while (0)

// ing4, 尝试给 pz(指针) 减少引用次数
#define Z_TRY_DELREF_P(pz) do {		\
	/* 是有计数的类型，才进行增加操作，不会报错 */ \
	if (Z_REFCOUNTED_P((pz))) {		\
		Z_DELREF_P((pz));			\
	}								\
} while (0)

// ing4, 尝试给 z 增加引用次数
#define Z_TRY_ADDREF(z)				Z_TRY_ADDREF_P(&(z))
// ing4, 尝试给 z 减少引用次数
#define Z_TRY_DELREF(z)				Z_TRY_DELREF_P(&(z))

#ifndef ZEND_RC_DEBUG
# define ZEND_RC_DEBUG 0
#endif

// RC 调试模式
#if ZEND_RC_DEBUG
extern ZEND_API bool zend_rc_debug;
/* The GC_PERSISTENT flag is reused for IS_OBJ_WEAKLY_REFERENCED on objects.
 * Skip checks for OBJECT/NULL type to avoid interpreting the flag incorrectly. */
# define ZEND_RC_MOD_CHECK(p) do { \
		if (zend_rc_debug) { \
			zend_uchar type = zval_gc_type((p)->u.type_info); \
			if (type != IS_OBJECT && type != IS_NULL) { \
				ZEND_ASSERT(!(zval_gc_flags((p)->u.type_info) & GC_IMMUTABLE)); \
				ZEND_ASSERT((zval_gc_flags((p)->u.type_info) & (GC_PERSISTENT|GC_PERSISTENT_LOCAL)) != GC_PERSISTENT); \
			} \
		} \
	} while (0)
// ing3, 调试时增加 GC_PERSISTENT_LOCAL 标记
# define GC_MAKE_PERSISTENT_LOCAL(p) do { \
		GC_ADD_FLAGS(p, GC_PERSISTENT_LOCAL); \
	} while (0)
// 没有 ZEND_RC_DEBUG ，ZEND_RC_MOD_CHECK，GC_MAKE_PERSISTENT_LOCAL  这两个无业务逻辑
#else
# define ZEND_RC_MOD_CHECK(p) \
	do { } while (0)
# define GC_MAKE_PERSISTENT_LOCAL(p) \
	do { } while (0)
#endif

// ing4, 获取引用次数
static zend_always_inline uint32_t zend_gc_refcount(const zend_refcounted_h *p) {
	return p->refcount;
}

// ing4, 设置引用次数
static zend_always_inline uint32_t zend_gc_set_refcount(zend_refcounted_h *p, uint32_t rc) {
	p->refcount = rc;
	return p->refcount;
}

// ing4, 增加引用计数
static zend_always_inline uint32_t zend_gc_addref(zend_refcounted_h *p) {
	// ZEND_RC_MOD_CHECK 调试用
	ZEND_RC_MOD_CHECK(p);
	return ++(p->refcount);
}

// ing3, 增加可变实例的引用数
static zend_always_inline void zend_gc_try_addref(zend_refcounted_h *p) {
	// 如果不是不可变的
	if (!(p->u.type_info & GC_IMMUTABLE)) {
		// 调试用
		ZEND_RC_MOD_CHECK(p);
		++p->refcount;
	}
}

// ing3,减少可变实例的引用数
static zend_always_inline void zend_gc_try_delref(zend_refcounted_h *p) {
	// 如果不是不可变的
	if (!(p->u.type_info & GC_IMMUTABLE)) {
		// 调试用
		ZEND_RC_MOD_CHECK(p);
		--p->refcount;
	}
}

// ing4, 减少 p 的 引用次数
static zend_always_inline uint32_t zend_gc_delref(zend_refcounted_h *p) {
	// 
	ZEND_ASSERT(p->refcount > 0);
	ZEND_RC_MOD_CHECK(p);
	// -1 次
	return --(p->refcount);
}

// ing4, 增加引用次数 rc 次
static zend_always_inline uint32_t zend_gc_addref_ex(zend_refcounted_h *p, uint32_t rc) {
	ZEND_RC_MOD_CHECK(p);
	p->refcount += rc;
	return p->refcount;
}

// ing4, 减少引用次数 rc 次
static zend_always_inline uint32_t zend_gc_delref_ex(zend_refcounted_h *p, uint32_t rc) {
	ZEND_RC_MOD_CHECK(p);
	p->refcount -= rc;
	return p->refcount;
}

// ing4, 获取指针所指对象的引用数
static zend_always_inline uint32_t zval_refcount_p(const zval* pz) {
#if ZEND_DEBUG
	ZEND_ASSERT(Z_REFCOUNTED_P(pz) || Z_TYPE_P(pz) == IS_ARRAY);
#endif
	return GC_REFCOUNT(Z_COUNTED_P(pz));
}

// ing3, 设置指针所指对象的引用数
static zend_always_inline uint32_t zval_set_refcount_p(zval* pz, uint32_t rc) {
	ZEND_ASSERT(Z_REFCOUNTED_P(pz));
	return GC_SET_REFCOUNT(Z_COUNTED_P(pz), rc);
}

// ing4，添加引用次数
static zend_always_inline uint32_t zval_addref_p(zval* pz) {
	// 必须已经有引用次数（不是0）
	ZEND_ASSERT(Z_REFCOUNTED_P(pz));
	// 添加并返回引用次数
	return GC_ADDREF(Z_COUNTED_P(pz));
}

// ing4，添加引用次数
static zend_always_inline uint32_t zval_delref_p(zval* pz) {
	// 必须已经有引用次数（不是0）
	ZEND_ASSERT(Z_REFCOUNTED_P(pz));
	// 减少并返回引用次数
	return GC_DELREF(Z_COUNTED_P(pz));
}

// 32位系统 
#if SIZEOF_SIZE_T == 4
// 要多复制一个 ww.w2, int型
# define ZVAL_COPY_VALUE_EX(z, v, gc, t)				\
	do {												\
		uint32_t _w2 = v->value.ww.w2;					\
		/* v的计数器指针给z */ \
		Z_COUNTED_P(z) = gc;							\
		/* v的 w2 给z */ \
		z->value.ww.w2 = _w2;							\
		/* v的 type_info 给z */ \
		Z_TYPE_INFO_P(z) = t;							\
	} while (0)

// 64位系统
#elif SIZEOF_SIZE_T == 8
// z, 被赋值变量指针， v, 取值变量（没用到），_gc 源zval中的指针，_t 源zval中的类型信息
// ing4, 这里是复制引用计数器和 typeinfo, typeinfo是int型，gc和原来的变量共用一个？
# define ZVAL_COPY_VALUE_EX(z, v, gc, t)				\
	do {												\
		/* 不算是什么类型，都是同一个指针，因为是union类型 */ \
		Z_COUNTED_P(z) = gc;							\
		/* type_info是整数 */ \
		Z_TYPE_INFO_P(z) = t;							\
	} while (0)
#else
# error "Unknown SIZEOF_SIZE_T"
#endif

// 使用量巨大
// z,_z1 被赋值变量指针， v,_z2 取值变量，_gc 引用计数器，_t type_info
// ing4, 把v的【附加信息】复制给z，不复制zval变量关键内容。 typeinfo是int型，gc是和原来的变量共用一个。
#define ZVAL_COPY_VALUE(z, v)							\
	do {												\
		/* 目标zval的指针 */ \
		zval *_z1 = (z);								\
		/* 源zval的指针 */ \
		const zval *_z2 = (v);							\
		/* 源zval中的指针 */ \
		zend_refcounted *_gc = Z_COUNTED_P(_z2);		\
		/* 源zval的type_info */ \
		uint32_t _t = Z_TYPE_INFO_P(_z2);				\
		/*  */ \
		ZVAL_COPY_VALUE_EX(_z1, _z2, _gc, _t);			\
	} while (0)

// ing4, 复制 zval 变量和附加信息,和上面唯一的不同是增加了一次引用计数
#define ZVAL_COPY(z, v)									\
	do {												\
		zval *_z1 = (z);								\
		const zval *_z2 = (v);							\
		/* v的引用计数器 */ \
		/* 其实复制了计数器指针，就等于复制了整个对象指针，因为计数器在对象的开头 */ \
		zend_refcounted *_gc = Z_COUNTED_P(_z2);		\
		/* v的type_info */ \
		uint32_t _t = Z_TYPE_INFO_P(_z2);				\
		/* v的附加信息复制给z */ \
		ZVAL_COPY_VALUE_EX(_z1, _z2, _gc, _t);			\
		/* 如果有引用计数，增加引用计数 */ \
		if (Z_TYPE_INFO_REFCOUNTED(_t)) {				\
			GC_ADDREF(_gc);								\
		}												\
	} while (0)

// ing4, 复制 zval 变量和附加信息。 如果类型是数组，给gc创建副本
#define ZVAL_DUP(z, v)									\
	do {												\
		zval *_z1 = (z);								\
		const zval *_z2 = (v);							\
		/* 其实复制了计数器指针，就等于复制了整个对象指针，因为计数器在对象的开头 */ \
		zend_refcounted *_gc = Z_COUNTED_P(_z2);		\
		uint32_t _t = Z_TYPE_INFO_P(_z2);				\
		/* 如果源实例类型为数组 */ \
		if ((_t & Z_TYPE_MASK) == IS_ARRAY) {			\
			/* 给gc创建副本 */ \
			ZVAL_ARR(_z1, zend_array_dup((zend_array*)_gc));\
		/* 如果源实例类型不是数组 */ \
		} else {										\
			/* 如果支持引用计数 */ \
			if (Z_TYPE_INFO_REFCOUNTED(_t)) {			\
				/* 引用数+1 */ \
				GC_ADDREF(_gc);							\
			}											\
			/* 复制附加信息 */ \
			ZVAL_COPY_VALUE_EX(_z1, _z2, _gc, _t);		\
		}												\
	} while (0)


// 在处理持久zval时， ZVAL_COPY_OR_DUP 可以替代 ZVAL_COPY 和 ZVAL_DUP
/* ZVAL_COPY_OR_DUP() should be used instead of ZVAL_COPY() and ZVAL_DUP()
 * in all places where the source may be a persistent zval.
 */
// ing3, 自动判断 ：持久对象增加引用次数，非持久对象创建副本
#define ZVAL_COPY_OR_DUP(z, v)											\
	do {																\
		zval *_z1 = (z);												\
		const zval *_z2 = (v);											\
		/* 其实复制了计数器指针，就等于复制了整个对象指针，因为计数器在对象的开头 */ \
		zend_refcounted *_gc = Z_COUNTED_P(_z2);						\
		/* type_info */			\
		uint32_t _t = Z_TYPE_INFO_P(_z2);								\
		/* 先复制zval */			\
		ZVAL_COPY_VALUE_EX(_z1, _z2, _gc, _t);							\
		/* 对象可计数 */			\
		if (Z_TYPE_INFO_REFCOUNTED(_t)) {								\
			/* 对象使用了持久弱引用 */			\
			/* Objects reuse PERSISTENT as WEAKLY_REFERENCED */			\
			/* 如果有 GC_PERSISTENT 标记 或 者是对象 */ \
			if (EXPECTED(!(GC_FLAGS(_gc) & GC_PERSISTENT)				\
					/* 增加引用次数 */			\
					|| GC_TYPE(_gc) == IS_OBJECT)) {					\
				/* 增加引用次数 */			\
				GC_ADDREF(_gc);											\
			/* 其他情况 */			\
			} else {													\
				/* 创建副本，并把zval关联到副本 */ \
				zval_copy_ctor_func(_z1);								\
			}															\
		}																\
	} while (0)

// ing4, 如果是引用类型，追踪到被引用的对象
#define ZVAL_DEREF(z) do {								\
		if (UNEXPECTED(Z_ISREF_P(z))) {					\
			(z) = Z_REFVAL_P(z);						\
		}												\
	} while (0)

// ing4, 如果是 IS_INDIRECT(.value.zv) 类型的引用，转到被引用的 zval
#define ZVAL_DEINDIRECT(z) do {							\
		if (Z_TYPE_P(z) == IS_INDIRECT) {				\
			(z) = Z_INDIRECT_P(z);						\
		}												\
	} while (0)

// ing4, 如果是 .value.ref 类型的引用，转到被引用的 zval
#define ZVAL_OPT_DEREF(z) do {							\
		if (UNEXPECTED(Z_OPT_ISREF_P(z))) {				\
			(z) = Z_REFVAL_P(z);						\
		}												\
	} while (0)
		
// ing4, 创建引用：创建一个引用对象，把值放在引用对象里，再把引用对象关联到本 zval
#define ZVAL_MAKE_REF(zv) do {							\
		zval *__zv = (zv);								\
		if (!Z_ISREF_P(__zv)) {							\
			ZVAL_NEW_REF(__zv, __zv);					\
		}												\
	} while (0)

// ing4, 解引用：追踪到引用目标，把目标中的变量复制过来，并销毁目标。
#define ZVAL_UNREF(z) do {								\
		zval *_z = (z);									\
		zend_reference *ref;							\
		/* 必须是引用类型 */ \
		ZEND_ASSERT(Z_ISREF_P(_z));						\
		/* 取得引用指针 */ \
		ref = Z_REF_P(_z);								\
		/* 把引用目标的值复制过来 */ \
		ZVAL_COPY_VALUE(_z, &ref->val);					\
		/* 释放引用目标 */ \
		efree_size(ref, sizeof(zend_reference));		\
	} while (0)

// ing4, 复制引用目标 和 附加信息，并增加引用计数
#define ZVAL_COPY_DEREF(z, v) do {						\
		zval *_z3 = (v);								\
		/* 如果是可计数的类型，先增加引用计数 */ \
		if (Z_OPT_REFCOUNTED_P(_z3)) {					\
			/* 如果v是 引用 类型 */ 				\
			if (UNEXPECTED(Z_OPT_ISREF_P(_z3))) {		\
				/* 追踪到引用目标 */ 				\
				_z3 = Z_REFVAL_P(_z3);					\
				/* 如果引用目标可计数 */ 			\
				if (Z_OPT_REFCOUNTED_P(_z3)) {			\
					/* 增加引用计数 */				\
					Z_ADDREF_P(_z3);					\
				}										\
			/* 如果v不是资源类型 */ 				\
			} else {									\
				/* 增加引用计数 */					\
				Z_ADDREF_P(_z3);						\
			}											\
		}												\
		/* 复制变量的附加信息 */ \
		ZVAL_COPY_VALUE(z, _z3);						\
	} while (0)

// ing4, 给 zval 指向的 zend_string 创建副本，并把指针指向新副本。
#define SEPARATE_STRING(zv) do {						\
		zval *_zv = (zv);								\
		/* 引用次数 >1 才有必要创建副本 */ \
		if (Z_REFCOUNT_P(_zv) > 1) {					\
			/* 取出zval中的zend_string */ \
			zend_string *_str = Z_STR_P(_zv);			\
			/* 必要要是可计数对 */ \
			ZEND_ASSERT(Z_REFCOUNTED_P(_zv));			\
			/* 必不是保留字 */ \
			ZEND_ASSERT(!ZSTR_IS_INTERNED(_str));		\
			/* 取出 string 并创建副本 */ \
			ZVAL_NEW_STR(_zv, zend_string_init(			\
				ZSTR_VAL(_str),	ZSTR_LEN(_str), 0));	\
			/* 原字串减少引用 */ \
			GC_DELREF(_str);							\
		}												\
	} while (0)

// ing4, 给 zval 指向的 数组 创建副本，并把指针指向新副本。
#define SEPARATE_ARRAY(zv) do {							\
		zval *__zv = (zv);								\
		zend_array *_arr = Z_ARR_P(__zv);				\
		/* 引用次数大于1才有必要创建分支 */ \
		if (UNEXPECTED(GC_REFCOUNT(_arr) > 1)) {		\
			ZVAL_ARR(__zv, zend_array_dup(_arr));		\
			/* 原实例减少引用次数 */ \
			GC_TRY_DELREF(_arr);						\
		}												\
	} while (0)

// ing4, 如果是数组，按需要创建副本，其他类型不操作。参见 SEPARATE_ARRAY。
#define SEPARATE_ZVAL_NOREF(zv) do {					\
		zval *_zv = (zv);								\
		/* 不可以是引用类型 */ \
		ZEND_ASSERT(Z_TYPE_P(_zv) != IS_REFERENCE);		\
		if (Z_TYPE_P(_zv) == IS_ARRAY) {				\
			SEPARATE_ARRAY(_zv);						\
		}												\
	} while (0)
		
// SEPARATE_*有4个方法，除了这个还有 SEPARATE_STRING，SEPARATE_ARRAY，SEPARATE_ZVAL_NOREF。
// 从逻辑上看，SEPARATE_ 只针对zend_string 和 哈希表
// ing3, zval创建新版本
#define SEPARATE_ZVAL(zv) do {							\
		zval *_zv = (zv);								\
		/* 如果是引用类型 */ \
		if (Z_ISREF_P(_zv)) {							\
			/* 引用实例 */ \
			zend_reference *_r = Z_REF_P(_zv);			\
			/* 解引用 */ \
			ZVAL_COPY_VALUE(_zv, &_r->val);				\
			/* 如果引用实例只有1次引用 */ \
			if (GC_DELREF(_r) == 0) {					\
				/* 释放引用实例 */ \
				efree_size(_r, sizeof(zend_reference));	\
			/* 否则（不可释放）：如果 zv 是数组 */ \
			} else if (Z_OPT_TYPE_P(_zv) == IS_ARRAY) {	\
				/* 创建副本 */ \
				ZVAL_ARR(_zv, zend_array_dup(Z_ARR_P(_zv)));\
				/* 完毕 */ \
				break;									\
			/* 否则（不可释放且不是数组）：如果 zv 是可计数 */ \
			} else if (Z_OPT_REFCOUNTED_P(_zv)) {		\
				/* 增加引用次数，不是数组不用真的separate */ \
				Z_ADDREF_P(_zv);						\
				/* 完毕 */ \
				break;									\
			}											\
		}												\
		/* 数组这里单独有一个处理 */ \
		if (Z_TYPE_P(_zv) == IS_ARRAY) {				\
			/* 数组创建副本 */ \
			SEPARATE_ARRAY(_zv);						\
		}												\
	} while (0)

// propertyies 存储一个标记，区分Z_EXTRA中，unset过的和未初始化的 properties
// (类型都是 IS_UNDEF) 。当复制propertyies的默认值时也需要复制 Z_EXTRA 。
// 所以定义不同的 宏 ，未来想删除这个工作区也比较容易。
/* Properties store a flag distinguishing unset and uninitialized properties
 * (both use IS_UNDEF type) in the Z_EXTRA space. As such we also need to copy
 * the Z_EXTRA space when copying property default values etc. We define separate
 * macros for this purpose, so this workaround is easier to remove in the future. */
#define IS_PROP_UNINIT 1

// ing4, 返回 (zval).u2.extra
#define Z_PROP_FLAG_P(z) Z_EXTRA_P(z)

// ing3, 把v完整复制到z
#define ZVAL_COPY_VALUE_PROP(z, v) \
	do { *(z) = *(v); } while (0)
// ing3, 两个zval, 先复制内容，再复制 (zval).u2.extra
#define ZVAL_COPY_PROP(z, v) \
	do { ZVAL_COPY(z, v); Z_PROP_FLAG_P(z) = Z_PROP_FLAG_P(v); } while (0)
		
// ing3, 自动判断 ：持久对象增加引用次数，非持久对象创建副本。并复制 .u2.extra。
#define ZVAL_COPY_OR_DUP_PROP(z, v) \
	do { ZVAL_COPY_OR_DUP(z, v); Z_PROP_FLAG_P(z) = Z_PROP_FLAG_P(v); } while (0)


#endif /* ZEND_TYPES_H */
