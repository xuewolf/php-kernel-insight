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

#ifndef ZEND_COMPILE_H
#define ZEND_COMPILE_H

#include "zend.h"
#include "zend_ast.h"

#include <stdarg.h>

#include "zend_llist.h"

// ing3, 把运算对象设置为未使用, p1:运算对象
#define SET_UNUSED(op) do { \
	/* 修改上下文中的类型 */ \
	op ## _type = IS_UNUSED; \
	/* 修改绑定的变量编号 */ \
	op.num = (uint32_t) -1; \
} while (0)

// ing3，操作码设置成 ZEND_NOP，清空操作码的操作对象和结果
#define MAKE_NOP(opline) do { \
	(opline)->op1.num = 0; \
	(opline)->op2.num = 0; \
	(opline)->result.num = 0; \
	// ZEND_NOP = 0, 见 zend_vm_opcodes.h
	(opline)->opcode = ZEND_NOP; \

	// 清空3个 znode_op 对象
	SET_UNUSED((opline)->op1); \
	SET_UNUSED((opline)->op2); \
	SET_UNUSED((opline)->result); \
} while (0)

// ing3, 重置文档注释
#define RESET_DOC_COMMENT() do { \
	/* 如果编译时有文档注释 */ \
	if (CG(doc_comment)) { \
		/* 释放文档注释 */ \
		zend_string_release_ex(CG(doc_comment), 0); \
		/* 指针置空 */ \
		CG(doc_comment) = NULL; \
	} \
} while (0)

typedef struct _zend_op_array zend_op_array;
typedef struct _zend_op zend_op;

// 
/* On 64-bit systems less optimal, but more compact VM code leads to better
 * performance. So on 32-bit systems we use absolute addresses for jump
 * targets and constants, but on 64-bit systems relative 32-bit offsets */
#if SIZEOF_SIZE_T == 4
# define ZEND_USE_ABS_JMP_ADDR      1
# define ZEND_USE_ABS_CONST_ADDR    1
// 64位操作系统两个都是 0
#else
# define ZEND_USE_ABS_JMP_ADDR      0
# define ZEND_USE_ABS_CONST_ADDR    0
#endif

// znode_op, 指 operand，运算对象。 是最基础的结构体，没有引用其他结构体。
	// 只有几个外部文件直接引用它，但引用 zend_op 的非常多，都间接引用了它。
	// /Zend/zend_execute.c, /Zend/Optimizer/block_pass.c, /Zend/zend_execute.h, /Zend/Optimizer/zend_dump.c	
	// zval是内置类型吗？
typedef union _znode_op {
	// 在 _zend_execute_data 中 本node 的常量的偏移量
	uint32_t      constant;
	// 在 _zend_execute_data 中 本node 的变量的偏移量
	uint32_t      var;
	// 创建过程中的序号，例如，表示这是第几个参数
	uint32_t      num;
	// 操作码行号？
	uint32_t      opline_num; /*  Needs to be signed */
// 32位系统 	
#if ZEND_USE_ABS_JMP_ADDR
	// 跳转地址，只有这一个是指针
	zend_op       *jmp_addr;
// 64位系统
#else
	// 跳转偏移量
	uint32_t      jmp_offset;
#endif

// 只有32位系统有 zv ，64位用constant
#if ZEND_USE_ABS_CONST_ADDR
	// 常量
	zval          *zv;
#endif
} znode_op;

// 只有编译过程中用到，常用来接收表达式结果
typedef struct _znode { /* used only during compilation */
	zend_uchar op_type;
	zend_uchar flag;
	// 放 union 的好处是什么？只是为了结构清晰吗？
	union {
		// 这里不是指针 通过 SET_NODE 把 （zend_op 下属的 znode_op） 复制过来
		// 或者通过 GET_NODE 把它传给 （zend_op 下属的 znode_op）
		znode_op op;
		// 同上，通过 SET_NODE 和 GET_NODE 传递
		zval constant; /* replaced by literal/zv */
	} u;
} znode;

// 临时定义，避免头部顺序问题，znode如何和zend_ast里的指针通用呢？
/* Temporarily defined here, to avoid header ordering issues */
typedef struct _zend_ast_znode {
	zend_ast_kind kind;
	zend_ast_attr attr;
	uint32_t lineno;
	znode node;
} zend_ast_znode;

// 
ZEND_API zend_ast * ZEND_FASTCALL zend_ast_create_znode(znode *node);

// ing3, 直接返回语句的 node 属性, p1->node
static zend_always_inline znode *zend_ast_get_znode(zend_ast *ast) {
	return &((zend_ast_znode *) ast)->node;
}

// 
typedef struct _zend_declarables {
	zend_long ticks;
} zend_declarables;

// 编译上下文，每个文件一个，但是 可以通过 op arrays 共享
/* Compilation context that is different for each file, but shared between op arrays. */
typedef struct _zend_file_context {
	zend_declarables declarables;

	zend_string *current_namespace;
	bool in_namespace;
	bool has_bracketed_namespaces;

	HashTable *imports;
	HashTable *imports_function;
	HashTable *imports_const;

	HashTable seen_symbols;
} zend_file_context;

// 解析器堆栈元素
typedef union _zend_parser_stack_elem {
	zend_ast *ast;
	zend_string *str;
	zend_ulong num;
	unsigned char *ptr;
	unsigned char *ident;
} zend_parser_stack_elem;

void zend_compile_top_stmt(zend_ast *ast);
void zend_const_expr_to_zval(zval *result, zend_ast **ast_ptr, bool allow_dynamic);

typedef int (*user_opcode_handler_t) (zend_execute_data *execute_data);

// 这是一个操作，也是一个操作码.  程序里变量经常叫做op_line
struct _zend_op { // 操作码。程序里变量名经常是 op_line
	const void *handler; // 处理器，只有这一个元素是指针
	// 这三个都是znode_op实例，不是指针。op 是指 operand，运算对象
	znode_op op1;
	znode_op op2;
	znode_op result;
	uint32_t extended_value; // 扩展值
	uint32_t lineno; // 行号
	zend_uchar opcode; // 操作码编号
	// 上面3个运算对象的类型
	zend_uchar op1_type;
	zend_uchar op2_type;
	zend_uchar result_type;
};

// ? break,continue 跳转点 只有内部调用
typedef struct _zend_brk_cont_element {
	int start;
	int cont;
	int brk;
	int parent;
	bool is_switch;
} zend_brk_cont_element;

// 跳转标签
typedef struct _zend_label {
	//
	int brk_cont;
	// 操作码序号
	uint32_t opline_num;
} zend_label;

//
typedef struct _zend_try_catch_element {
	uint32_t try_op;
	uint32_t catch_op;  /* ketchup! */
	uint32_t finally_op;
	uint32_t finally_end;
} zend_try_catch_element;

// 临时变量 活动区域
#define ZEND_LIVE_TMPVAR  0
// 循环 活动区域
#define ZEND_LIVE_LOOP    1
// 静默 活动区域
#define ZEND_LIVE_SILENCE 2
// rope 活动区域
#define ZEND_LIVE_ROPE    3
// new 活动区域
#define ZEND_LIVE_NEW     4
// 这里又是把指针最后3位做成 附加标记用了 
#define ZEND_LIVE_MASK    7

// 操作码活跃区域
typedef struct _zend_live_range {
	// 低位用来来存放变量类型 （ZEND_LIVE_*宏）
	uint32_t var; /* low bits are used for variable type (ZEND_LIVE_* macros) */
	// 活动区域开始位置
	uint32_t start;
	// 活动区域结束位置
	uint32_t end;
} zend_live_range;

// 操作码上下文，给每个操作码列表一个独立的上下文
/* Compilation context that is different for each op array. */
typedef struct _zend_oparray_context {
	uint32_t   opcodes_size;
	int        vars_size;
	int        literals_size;
	uint32_t   fast_call_var;
	uint32_t   try_catch_offset;
	int        current_brk_cont;
	int        last_brk_cont;
	zend_brk_cont_element *brk_cont_array;
	// 跳转 label
	HashTable *labels;
} zend_oparray_context;

// 类，属性，方法，常量 使用的标记
/* Class, property and method flags                  class|meth.|prop.|const*/
/*                                                        |     |     |     */
/* Common flags                                           |     |     |     */
/* ============                                           |     |     |     */
/*                                                        |     |     |     */
// 可见性标记
/* Visibility flags (public < protected < private)        |     |     |     */
#define ZEND_ACC_PUBLIC                  (1 <<  0) /*     |  X  |  X  |  X  */
#define ZEND_ACC_PROTECTED               (1 <<  1) /*     |  X  |  X  |  X  */
#define ZEND_ACC_PRIVATE                 (1 <<  2) /*     |  X  |  X  |  X  */
/*                                                        |     |     |     */

// 覆盖私有属性或方法
/* Property or method overrides private one               |     |     |     */
#define ZEND_ACC_CHANGED                 (1 <<  3) /*     |  X  |  X  |     */
/*                                                        |     |     |     */

// 静态方法或属性
/* Static method or property                              |     |     |     */
#define ZEND_ACC_STATIC                  (1 <<  4) /*     |  X  |  X  |     */
/*                                                        |     |     |     */

// 提升的？属性或参数
/* Promoted property / parameter                          |     |     |     */
#define ZEND_ACC_PROMOTED                (1 <<  5) /*     |     |  X  |  X  */
/*                                                        |     |     |     */

// 带final标记的类或方法
/* Final class or method                                  |     |     |     */
#define ZEND_ACC_FINAL                   (1 <<  5) /*  X  |  X  |     |     */
/*                                                        |     |     |     */

// 抽象方法
/* Abstract method                                        |     |     |     */
#define ZEND_ACC_ABSTRACT                (1 <<  6) /*  X  |  X  |     |     */

// 显式抽象类
#define ZEND_ACC_EXPLICIT_ABSTRACT_CLASS (1 <<  6) /*  X  |     |     |     */
/*                                                        |     |     |     */

// 只读属性
/* Readonly property                                      |     |     |     */
#define ZEND_ACC_READONLY                (1 <<  7) /*     |     |  X  |     */
/*                                                        |     |     |     */


// 只有opcache扩展会不回 ext\opcache\zend_file_cache.c, ext\opcache\zend_persist.c
// 不可更改的 操作码和类入口
// 专为操作码的懒加载而实现
/* Immutable op_array and class_entries                   |     |     |     */
/* (implemented only for lazy loading of op_arrays)       |     |     |     */
#define ZEND_ACC_IMMUTABLE               (1 <<  7) /*  X  |  X  |     |     */
/*                                                        |     |     |     */

// 函数有指定类型的参数 或 类有指定类型的属性
/* Function has typed arguments / class has typed props   |     |     |     */
#define ZEND_ACC_HAS_TYPE_HINTS          (1 <<  8) /*  X  |  X  |     |     */
/*                                                        |     |     |     */

// 顶层的类或函数定义
/* Top-level class or function declaration                |     |     |     */
#define ZEND_ACC_TOP_LEVEL               (1 <<  9) /*  X  |  X  |     |     */
/*                                                        |     |     |     */

// 预先加载好的操作码或类
/* op_array or class is preloaded                         |     |     |     */
#define ZEND_ACC_PRELOADED               (1 << 10) /*  X  |  X  |     |     */
/*                                                        |     |     |     */

// 区分常量的标记， 不可以与 ZEND_ACC_* 可见标记 或 IS_CONSTANT_VISITED_MARK 冲突
/* Flag to differentiate cases from constants.            |     |     |     */
/* Must not conflict with ZEND_ACC_ visibility flags      |     |     |     */
/* or IS_CONSTANT_VISITED_MARK                            |     |     |     */
#define ZEND_CLASS_CONST_IS_CASE         (1 << 6)  /*     |     |     |  X  */
/*                                                        |     |     |     */
// 类标记，21,30,31 弃用
/* Class Flags (unused: 21,30,31)                         |     |     |     */
/* ===========                                            |     |     |     */
/*                                                        |     |     |     */
// 接口 或 TRAIT
/* Special class types                                    |     |     |     */
#define ZEND_ACC_INTERFACE               (1 <<  0) /*  X  |     |     |     */
#define ZEND_ACC_TRAIT                   (1 <<  1) /*  X  |     |     |     */

// 匿名类
#define ZEND_ACC_ANON_CLASS              (1 <<  2) /*  X  |     |     |     */

// 枚举
// enum
#define ZEND_ACC_ENUM                    (1 << 28) /*  X  |     |     |     */
/*                                                        |     |     |     */

// 链接了 父类，借口或trait
/* Class linked with parent, interfaces and traits        |     |     |     */
#define ZEND_ACC_LINKED                  (1 <<  3) /*  X  |     |     |     */
/*                                                        |     |     |     */
// 抽象类，包含任何抽象方法时设置
/* Class is abstract, since it is set by any              |     |     |     */
/* abstract method                                        |     |     |     */
#define ZEND_ACC_IMPLICIT_ABSTRACT_CLASS (1 <<  4) /*  X  |     |     |     */
/*                                                        |     |     |     */

// 有魔术方法 __get/__set/__unset/__isset 的类，使用 guards
/* Class has magic methods __get/__set/__unset/           |     |     |     */
/* __isset that use guards                                |     |     |     */
#define ZEND_ACC_USE_GUARDS              (1 << 11) /*  X  |     |     |     */
/*                                                        |     |     |     */

// 更新类常量
/* Class constants updated                                |     |     |     */
#define ZEND_ACC_CONSTANTS_UPDATED       (1 << 12) /*  X  |     |     |     */
/*                                                        |     |     |     */

// 这个类的对象不可以有动态属性
/* Objects of this class may not have dynamic properties  |     |     |     */
#define ZEND_ACC_NO_DYNAMIC_PROPERTIES   (1 << 13) /*  X  |     |     |     */
/*                                                        |     |     |     */

// 用户类有使用静态变量的成员方法
/* User class has methods with static variables           |     |     |     */
#define ZEND_HAS_STATIC_IN_METHODS       (1 << 14) /*  X  |     |     |     */
/*                                                        |     |     |     */

// 此类的实例可能有动态属性，不会触发弃用警告
/* Objects of this class may have dynamic properties      |     |     |     */
/* without triggering a deprecation warning               |     |     |     */
#define ZEND_ACC_ALLOW_DYNAMIC_PROPERTIES (1 << 15) /* X  |     |     |     */
/*                                                        |     |     |     */

// 只读类
/* Readonly class                                         |     |     |     */
#define ZEND_ACC_READONLY_CLASS          (1 << 16) /*  X  |     |     |     */
/*                                                        |     |     |     */
// 父类 ce 已经创建
/* Parent class is resolved (CE).                         |     |     |     */
#define ZEND_ACC_RESOLVED_PARENT         (1 << 17) /*  X  |     |     |     */
/*                                                        |     |     |     */
// 接口已经创建
/* Interfaces are resolved (CEs).                         |     |     |     */
#define ZEND_ACC_RESOLVED_INTERFACES     (1 << 18) /*  X  |     |     |     */
/*                                                        |     |     |     */

// ？variance obligations 是什么
/* Class has unresolved variance obligations.             |     |     |     */
#define ZEND_ACC_UNRESOLVED_VARIANCE     (1 << 19) /*  X  |     |     |     */
/*                                                        |     |     |     */
// ?
/* Class is linked apart from variance obligations.       |     |     |     */
#define ZEND_ACC_NEARLY_LINKED           (1 << 20) /*  X  |     |     |     */
/*                                                        |     |     |     */

// 储存在opcache里（可能只有部分）
/* stored in opcache (may be partially)                   |     |     |     */
#define ZEND_ACC_CACHED                  (1 << 22) /*  X  |     |     |     */
/*                                                        |     |     |     */

// ？
/* temporary flag used during delayed variance checks     |     |     |     */
#define ZEND_ACC_CACHEABLE               (1 << 23) /*  X  |     |     |     */

// 表达式常量，表达式属性，表达式静态变量
/*                                                        |     |     |     */
#define ZEND_ACC_HAS_AST_CONSTANTS       (1 << 24) /*  X  |     |     |     */
#define ZEND_ACC_HAS_AST_PROPERTIES      (1 << 25) /*  X  |     |     |     */
#define ZEND_ACC_HAS_AST_STATICS         (1 << 26) /*  X  |     |     |     */
/*                                                        |     |     |     */

// 从文件缓存 或 处理内存中加载
/* loaded from file cache to process memory               |     |     |     */
#define ZEND_ACC_FILE_CACHED             (1 << 27) /*  X  |     |     |     */
/*                                                        |     |     |     */

// 类不可序列化或反序列化
/* Class cannot be serialized or unserialized             |     |     |     */
#define ZEND_ACC_NOT_SERIALIZABLE        (1 << 29) /*  X  |     |     |     */
/*                                                        |     |     |     */
/* Function Flags (unused: 28-30)                         |     |     |     */
/* ==============                                         |     |     |     */
/*                                                        |     |     |     */

// 弃用标记
/* deprecation flag                                       |     |     |     */
#define ZEND_ACC_DEPRECATED              (1 << 11) /*     |  X  |     |     */
/*                                                        |     |     |     */

// 引用返回的函数
/* Function returning by reference                        |     |     |     */
#define ZEND_ACC_RETURN_REFERENCE        (1 << 12) /*     |  X  |     |     */
/*                                                        |     |     |     */

// 有返回类型的函数
/* Function has a return type                             |     |     |     */
#define ZEND_ACC_HAS_RETURN_TYPE         (1 << 13) /*     |  X  |     |     */
/*                                                        |     |     |     */

// 有参数数量的函数
/* Function with variable number of arguments             |     |     |     */
#define ZEND_ACC_VARIADIC                (1 << 14) /*     |  X  |     |     */
/*                                                        |     |     |     */

// 有最终块的 操作码（只有用户用到）
/* op_array has finally blocks (user only)                |     |     |     */
#define ZEND_ACC_HAS_FINALLY_BLOCK       (1 << 15) /*     |  X  |     |     */
/*                                                        |     |     |     */

// 有 ZEND_DECLARE_CLASS_DELAYED 操作码的 main 操作码
/* "main" op_array with                                   |     |     |     */
/* ZEND_DECLARE_CLASS_DELAYED opcodes                     |     |     |     */
#define ZEND_ACC_EARLY_BINDING           (1 << 16) /*     |  X  |     |     */
/*                                                        |     |     |     */

// 闭包中使用 $this
/* closure uses $this                                     |     |     |     */
#define ZEND_ACC_USES_THIS               (1 << 17) /*     |  X  |     |     */
/*                                                        |     |     |     */
// 通过用户函数弹跳调用，其实就是 __call, __callstatic 两个方法
/* call through user function trampoline. e.g.            |     |     |     */
/* __call, __callstatic                                   |     |     |     */
#define ZEND_ACC_CALL_VIA_TRAMPOLINE     (1 << 18) /*     |  X  |     |     */
/*                                                        |     |     |     */
// 禁用行内缓存
/* disable inline caching                                 |     |     |     */
#define ZEND_ACC_NEVER_CACHE             (1 << 19) /*     |  X  |     |     */
/*                                                        |     |     |     */
// 操作码是从trait中的方法复制来的
/* op_array is a clone of trait method                    |     |     |     */
#define ZEND_ACC_TRAIT_CLONE             (1 << 20) /*     |  X  |     |     */
/*                                                        |     |     |     */
// 构造方法
/* functions is a constructor                             |     |     |     */
#define ZEND_ACC_CTOR                    (1 << 21) /*     |  X  |     |     */
/*                                                        |     |     |     */

// 闭包相关
/* Closure related                                        |     |     |     */
#define ZEND_ACC_CLOSURE                 (1 << 22) /*     |  X  |     |     */

// 假闭包：从类方法或函数转成的闭包？
#define ZEND_ACC_FAKE_CLOSURE            (1 << 23) /*     |  X  |     |     */ /* Same as ZEND_CALL_FAKE_CLOSURE */
/*                                                        |     |     |     */

// 标记是在 yield 语句中
#define ZEND_ACC_GENERATOR               (1 << 24) /*     |  X  |     |     */
/*                                                        |     |     |     */

// 函数由 pass two 来处理
/* function was processed by pass two (user only)         |     |     |     */
#define ZEND_ACC_DONE_PASS_TWO           (1 << 25) /*     |  X  |     |     */
/*                                                        |     |     |     */

// arena 分配的内置函数
/* internal function is allocated at arena (int only)     |     |     |     */
#define ZEND_ACC_ARENA_ALLOCATED         (1 << 25) /*     |  X  |     |     */
/*                                                        |     |     |     */

// heap 上分配的 运行时缓存（用户用）
/* run_time_cache allocated on heap (user only)           |     |     |     */
#define ZEND_ACC_HEAP_RT_CACHE           (1 << 26) /*     |  X  |     |     */
/*                                                        |     |     |     */

// Closure::__invoke() 使用 的方法 flag，只能是 int 型
/* method flag used by Closure::__invoke() (int only)     |     |     |     */
#define ZEND_ACC_USER_ARG_INFO           (1 << 26) /*     |  X  |     |     */
/*                                                        |     |     |     */

// 支持 opcache 编译时 计算 （函数）
/* supports opcache compile-time evaluation (funcs)       |     |     |     */
#define ZEND_ACC_COMPILE_TIME_EVAL       (1 << 27) /*     |  X  |     |     */
/*                                                        |     |     |     */

// op_array 使用 严格模式的类型
/* op_array uses strict mode types                        |     |     |     */
#define ZEND_ACC_STRICT_TYPES            (1U << 31) /*    |  X  |     |     */

// public,protected,private 掩码
#define ZEND_ACC_PPP_MASK  (ZEND_ACC_PUBLIC | ZEND_ACC_PROTECTED | ZEND_ACC_PRIVATE)

// 通过内置函数处理器调用。例如 Closure::invoke()
/* call through internal function handler. e.g. Closure::invoke() */
#define ZEND_ACC_CALL_VIA_HANDLER     ZEND_ACC_CALL_VIA_TRAMPOLINE

// 短路标记掩码
#define ZEND_SHORT_CIRCUITING_CHAIN_MASK 0x3
// 用在表达式里的 短路
#define ZEND_SHORT_CIRCUITING_CHAIN_EXPR 0
// 用在 isset 里的 短路
#define ZEND_SHORT_CIRCUITING_CHAIN_ISSET 1
// 用在 emtpy 里的 短路
#define ZEND_SHORT_CIRCUITING_CHAIN_EMPTY 2

// 不可以与 ZEND_SHORT_CIRCUITING_CHAIN_MASK 冲突
// Must not clash with ZEND_SHORT_CIRCUITING_CHAIN_MASK
#define ZEND_JMP_NULL_BP_VAR_IS 4

// 
char *zend_visibility_string(uint32_t fn_flags);

// 类属性信息
typedef struct _zend_property_info {
	// 普通属性的 offset 或 静态属性的 index
	uint32_t offset; /* property offset for object properties or
	                      property index for static properties */
	// 标记
	uint32_t flags;
	// 名称
	zend_string *name;
	// 文档
	zend_string *doc_comment;
	// 修饰属性
	HashTable *attributes;
	// 类入口
	zend_class_entry *ce;
	// 类型
	zend_type type;
} zend_property_info;

// ing4, 通过偏移量获取指定对象的类属性值
#define OBJ_PROP(obj, offset) \
	((zval*)((char*)(obj) + offset))
// ing4, 通过索引号获取指定对象的属性值，和上面一样返回 *zval
#define OBJ_PROP_NUM(obj, num) \
	(&(obj)->properties_table[(num)])
// ing4, 计算类属性偏移量
#define OBJ_PROP_TO_OFFSET(num) \
	((uint32_t)(XtOffsetOf(zend_object, properties_table) + sizeof(zval) * (num)))
// ing4, 从偏移量获取 当前对象的(zval)序号
#define OBJ_PROP_TO_NUM(offset) \
	((offset - OBJ_PROP_TO_OFFSET(0)) / sizeof(zval))

// 类常量
typedef struct _zend_class_constant {
	// 值，标记存放在 zval 的 u2 里
	zval value; /* flags are stored in u2 */
	// 文档
	zend_string *doc_comment;
	// 修饰属性
	HashTable *attributes;
	// 类入口
	zend_class_entry *ce;
} zend_class_constant;

// #define Z_CONSTANT_FLAGS(zval) (zval).u2.constant_flags
// ing4, 获取常量的标记
#define ZEND_CLASS_CONST_FLAGS(c) Z_CONSTANT_FLAGS((c)->value)

// 内置函数的参数信息
/* arg_info for internal functions */
typedef struct _zend_internal_arg_info {
	// 参数名
	const char *name;
	// 类型
	zend_type type;
	// 默认值
	const char *default_value;
} zend_internal_arg_info;

// 用户定义函数的参数信息, 结构不同
/* arg_info for user functions */
typedef struct _zend_arg_info {
	// 参数名
	zend_string *name;
	// 类型
	zend_type type;
	// 默认值
	zend_string *default_value;
} zend_arg_info;

// 它重复了 zend_internal_arg_info 的结构，但意思不同。
// 它被用作 arg_info 数组的第一个元素，来定义内置函数的属性。也被用作返回类型。
/* the following structure repeats the layout of zend_internal_arg_info,
 * but its fields have different meaning. It's used as the first element of
 * arg_info array to define properties of internal functions.
 * It's also used for the return type.
 */
 // 内置函数信息
typedef struct _zend_internal_function_info {
	// 必须参数数量
	zend_uintptr_t required_num_args;
	// 类型
	zend_type type;
	// 默认值
	const char *default_value;
} zend_internal_function_info;

// 每个 _zend_op_array 对应一个function 或者goto标签
	// 参看编译过程中的 CG(active_op_array) = 作用域切换
struct _zend_op_array {
	/* Common elements */
	zend_uchar type;
	// 参数标记
	zend_uchar arg_flags[3]; /* bitset of arg_info.pass_by_reference */
	// 函数标记
	uint32_t fn_flags;
	// 所属函数名
	zend_string *function_name;
	// 所属类实体
	zend_class_entry *scope;
	// 函数原型。 函数是可以定义原型，抽象函数，这两个东西是不一样的。原型更像是一个协议，c语言里居然有这种东西，够高级的。
	zend_function *prototype;
	// 参数数量
	uint32_t num_args;
	// 需要的参数数量？
	uint32_t required_num_args;
	// 函数的形参列表 ~
	zend_arg_info *arg_info;
	// 修饰属性
	HashTable *attributes;
	// 临时变量数量，临时变量没有名称
	uint32_t T;         /* number of temporary variables */
	// run_time_cache 指针列表
	ZEND_MAP_PTR_DEF(void **, run_time_cache);
	/* END of common elements */

	// 缓存位置指针 数量 
	int cache_size;     /* number of run_time_cache_slots * sizeof(void*) */
	// 编译变量的数量，编译变量有名称
	int last_var;       /* number of CV variables */
	// 操作码数量
	uint32_t last;      /* number of opcodes */

	// 一串操作码的指针
	zend_op *opcodes;
	// 
	ZEND_MAP_PTR_DEF(HashTable *, static_variables_ptr);
	// 静态变量
	HashTable *static_variables;
	// 编译变量的变量名列表
	zend_string **vars; /* names of CV variables */

	// 引用数。_zend_op_array没有gc
	uint32_t *refcount;

	int last_live_range;
	int last_try_catch;
	zend_live_range *live_range;
	zend_try_catch_element *try_catch_array;

	// 文件名 
	zend_string *filename;
	// 在文件中的开始行数
	uint32_t line_start;
	// 结束行数
	uint32_t line_end;
	// 专属注释  /** 开头
	zend_string *doc_comment;

	// literal 数量
	int last_literal;
	// 闭包数量？
	uint32_t num_dynamic_func_defs;
	// ? 主要在 /Zend/Optimizer/compact_literals.c 和 /Zend/Optimizer/zend_optimizer.c 里面用到
	// 这个好像用来做opcache用的
	zval *literals;

	// 代码块中的闭包
	/* Functions that are declared dynamically are stored here and
	 * referenced by index from opcodes. */
	zend_op_array **dynamic_func_defs;

	void *reserved[ZEND_MAX_RESERVED_RESOURCES];
};


#define ZEND_RETURN_VALUE				0
#define ZEND_RETURN_REFERENCE			1

// 其实只有执行上下文和返回值，两个参数。
// #define INTERNAL_FUNCTION_PARAMETERS zend_execute_data *execute_data, zval *return_value

/* zend_internal_function_handler */
// zend 内部函数处理器
typedef void (ZEND_FASTCALL *zif_handler)(INTERNAL_FUNCTION_PARAMETERS);

// 内部函数
typedef struct _zend_internal_function {
	// 公共元素
	/* Common elements */
	// 类型
	zend_uchar type;
	// 参数标记
	zend_uchar arg_flags[3]; /* bitset of arg_info.pass_by_reference */
	// 函数标记
	uint32_t fn_flags;
	// 函数名
	zend_string* function_name;
	// 作用域
	zend_class_entry *scope;
	// php函数
	zend_function *prototype;
	// 参数数量
	uint32_t num_args;
	// 必须参数数量
	uint32_t required_num_args;
	// 参数信息列表
	zend_internal_arg_info *arg_info;
	// 修饰属性
	HashTable *attributes;
	// 临时变量数量
	uint32_t T;         /* number of temporary variables */
	// run_time_cache 指针列表
	ZEND_MAP_PTR_DEF(void **, run_time_cache);
	/* END of common elements */

	// 内置函数处理器
	zif_handler handler;
	// 
	struct _zend_module_entry *module;
	// 
	void *reserved[ZEND_MAX_RESERVED_RESOURCES];
} zend_internal_function;

// ing4, 返回函数所属类名，如果不存在，返回空字串
#define ZEND_FN_SCOPE_NAME(function)  ((function) && (function)->common.scope ? ZSTR_VAL((function)->common.scope->name) : "")

// php函数， 这两个结构体主体部分类似. union 和 struct 区别？为什么在 union 里定义 struct ?
union _zend_function {
	// 类型，必须是结构中第一个元素
	zend_uchar type;	/* MUST be the first element of this struct! */
	// 快速参数标记？
	uint32_t   quick_arg_flags;

	struct {
		// 不会用到
		zend_uchar type;  /* never used */
		// 参数标记
		zend_uchar arg_flags[3]; /* bitset of arg_info.pass_by_reference */
		// 函数标记
		uint32_t fn_flags;
		// 函数名
		zend_string *function_name;
		// 所属类
		zend_class_entry *scope;
		// 原型，父函数？
		zend_function *prototype;
		// 参数数量
		uint32_t num_args;
		// 必须参数数量
		uint32_t required_num_args;
		// 参数信息，如果任何参数都可以，返回-1
		zend_arg_info *arg_info;  /* index -1 represents the return value info, if any */
		// 修饰属性
		HashTable   *attributes;
		// 临时变量数量
		uint32_t T;         /* number of temporary variables */
		// run_time_cache 指针列表
		ZEND_MAP_PTR_DEF(void **, run_time_cache);
	} common;

	// 操作码
	zend_op_array op_array;
	// 内部函数
	zend_internal_function internal_function;
};

// 单个执行数据。最终被执行的不只是opcode而是这样打包好的一包数据 
struct _zend_execute_data {
	// 操作码执行过的操作码指针, （一个，而不是一堆）
	const zend_op       *opline;           /* executed opline                */
	// 执行数据，当前调用？
	zend_execute_data   *call;             /* current call                   */
	// 返回值
	zval                *return_value;
	// 所属函数
	zend_function       *func;             /* executed function              */
	// $this 对象 （ this + call_info + num_args）
	zval                 This;             /* this + call_info + num_args    */
	// 前一个执行数据对象 
	zend_execute_data   *prev_execute_data;
	// 符号表？
	zend_array          *symbol_table;
	// 运行时缓存 ？
	void               **run_time_cache;   /* cache op_array->run_time_cache */
	// 扩展命名参数 （函数的命名参数）
	zend_array          *extra_named_params;
};

// #define IS_OBJECT_EX (IS_OBJECT | (IS_TYPE_REFCOUNTED << Z_TYPE_FLAGS_SHIFT) | (IS_TYPE_COLLECTABLE << Z_TYPE_FLAGS_SHIFT))
// 包含this
#define ZEND_CALL_HAS_THIS           IS_OBJECT_EX

// Z_TYPE_INFO(EX(This)) 的前16个位作为 call_info 标记
/* Top 16 bits of Z_TYPE_INFO(EX(This)) are used as call_info flags */
// 调用函数 
#define ZEND_CALL_FUNCTION           (0 << 16)
// 调用代码
#define ZEND_CALL_CODE               (1 << 16)
// 嵌套调用
#define ZEND_CALL_NESTED             (0 << 17)
// 顶层调用
#define ZEND_CALL_TOP                (1 << 17)
// 分配的调用？
#define ZEND_CALL_ALLOCATED          (1 << 18)
// ？
#define ZEND_CALL_FREE_EXTRA_ARGS    (1 << 19)
// 包含符号表
#define ZEND_CALL_HAS_SYMBOL_TABLE   (1 << 20)
// 释放this
#define ZEND_CALL_RELEASE_THIS       (1 << 21)
// 调用闭包
#define ZEND_CALL_CLOSURE            (1 << 22)
// 调用假闭包
#define ZEND_CALL_FAKE_CLOSURE       (1 << 23) /* Same as ZEND_ACC_FAKE_CLOSURE */
// 调用生成器
#define ZEND_CALL_GENERATOR          (1 << 24)
// 动态调用
#define ZEND_CALL_DYNAMIC            (1 << 25)
// 调用中可能有未定义
#define ZEND_CALL_MAY_HAVE_UNDEF     (1 << 26)
// 调用中包含额外命名函数
#define ZEND_CALL_HAS_EXTRA_NAMED_PARAMS (1 << 27)
// 调用观察者
#define ZEND_CALL_OBSERVED           (1 << 28) /* "fcall_begin" observer handler may set this flag */
                                               /* to prevent optimization in RETURN handler and    */
                                               /* keep all local variables for "fcall_end" handler */
// 为追踪
#define ZEND_CALL_JIT_RESERVED       (1 << 29) /* reserved for tracing JIT */
// ？？
#define ZEND_CALL_NEEDS_REATTACH     (1 << 30)
// 调用时 引用传递参数
#define ZEND_CALL_SEND_ARG_BY_REF    (1u << 31)

// 函数嵌套调用
#define ZEND_CALL_NESTED_FUNCTION    (ZEND_CALL_FUNCTION | ZEND_CALL_NESTED)
// 代码嵌套调用
#define ZEND_CALL_NESTED_CODE        (ZEND_CALL_CODE | ZEND_CALL_NESTED)
// 顶层函数
#define ZEND_CALL_TOP_FUNCTION       (ZEND_CALL_TOP | ZEND_CALL_FUNCTION)
// 顶层代码
#define ZEND_CALL_TOP_CODE           (ZEND_CALL_CODE | ZEND_CALL_TOP)

// ing3, Z_TYPE_INFO((p1)->This))
#define ZEND_CALL_INFO(call) \
	Z_TYPE_INFO((call)->This)

// ing3, p1 & (ZEND_CALL_CODE | ZEND_CALL_TOP)
#define ZEND_CALL_KIND_EX(call_info) \
	(call_info & (ZEND_CALL_CODE | ZEND_CALL_TOP))

// ing3, p1->This & (ZEND_CALL_CODE | ZEND_CALL_TOP)
#define ZEND_CALL_KIND(call) \
	/* Z_TYPE_INFO((p1)->This)) */ \
	ZEND_CALL_KIND_EX(ZEND_CALL_INFO(call))

// ing3, p1 |= p2
#define ZEND_ADD_CALL_FLAG_EX(call_info, flag) do { \
		call_info |= (flag); \
	} while (0)

// ing3, p1里删除标记p2 。p1 &= ~(p2);
#define ZEND_DEL_CALL_FLAG_EX(call_info, flag) do { \
		call_info &= ~(flag); \
	} while (0)

// ing4, Z_TYPE_INFO((p1)->This) |= p2
#define ZEND_ADD_CALL_FLAG(call, flag) do { \
		/*  p1里添加标记p2 。p1 |= p2; */ \
		ZEND_ADD_CALL_FLAG_EX(Z_TYPE_INFO((call)->This), flag); \
	} while (0)

// ing4, Z_TYPE_INFO((p1)->This) &= ~(p2)
#define ZEND_DEL_CALL_FLAG(call, flag) do { \
		/*  p1里删除标记p2 。p1 &= ~(p2);*/ \
		ZEND_DEL_CALL_FLAG_EX(Z_TYPE_INFO((call)->This), flag); \
	} while (0)

// ing3, 从操作码中取出参数数量 
#define ZEND_CALL_NUM_ARGS(call) \
	(call)->This.u2.num_args

// ZEND_CALL_FRAME_SLOT:64位windows里是 =  2
#define ZEND_CALL_FRAME_SLOT \
	((int)((ZEND_MM_ALIGNED_SIZE(sizeof(zend_execute_data)) + ZEND_MM_ALIGNED_SIZE(sizeof(zval)) - 1) / ZEND_MM_ALIGNED_SIZE(sizeof(zval))))

// ing3, 把p1 右移p2字节，转成zval返回  ((zval*)((char*)(p1) + p2))
#define ZEND_CALL_VAR(call, n) \
	((zval*)(((char*)(call)) + ((int)(n))))

// ing3, 取得列表中第 ZEND_CALL_FRAME_SLOT + n 个 zval
#define ZEND_CALL_VAR_NUM(call, n) \
	(((zval*)(call)) + (ZEND_CALL_FRAME_SLOT + ((int)(n))))

// ing3, 取得列表中第 （ZEND_CALL_FRAME_SLOT +） n 个 zval
#define ZEND_CALL_ARG(call, n) \
	ZEND_CALL_VAR_NUM(call, ((int)(n)) - 1)

// ing3, 取得执行数据中的元素
#define EX(element) 			((execute_data)->element)

// ing3, Z_TYPE_INFO(execute_data->This)
#define EX_CALL_INFO()			ZEND_CALL_INFO(execute_data)
// ing3, execute_data->This & (ZEND_CALL_CODE | ZEND_CALL_TOP)
#define EX_CALL_KIND()			ZEND_CALL_KIND(execute_data)
// ing4, 取得执行数据中的参数数量
#define EX_NUM_ARGS()			ZEND_CALL_NUM_ARGS(execute_data)

// ing4, 此调用函数 的参数是否使用严格类型限制
#define ZEND_CALL_USES_STRICT_TYPES(call) \
	(((call)->func->common.fn_flags & ZEND_ACC_STRICT_TYPES) != 0)

// ing4, 某执行上下文所属函数 的参数是否使用严格类型限制
#define EX_USES_STRICT_TYPES() \
	/* 当前调用函数 的参数是否使用严格类型限制 */ \
	ZEND_CALL_USES_STRICT_TYPES(execute_data)

// ing3, 前一个执行数据所属函数 的参数使用严格类型限制
#define ZEND_ARG_USES_STRICT_TYPES() \
	/* 有前一个执行数据 */ \
	(EG(current_execute_data)->prev_execute_data && \
	/* 前一个执行数据有所属函数 */ \
	 EG(current_execute_data)->prev_execute_data->func && \
	/* 前一个执行数据调用函数 的参数使用严格类型限制 */ \
	 ZEND_CALL_USES_STRICT_TYPES(EG(current_execute_data)->prev_execute_data))

// ing4, 当前执行上下文函数 的参数是否使用严格类型限制
#define ZEND_RET_USES_STRICT_TYPES() \
	/* 当前执行上下文函数 的参数是否使用严格类型限制 */ \
	ZEND_CALL_USES_STRICT_TYPES(EG(current_execute_data))

// ing3, 通过偏移量获取 execute_data 中的 zval
// ZEND_CALL_VAR(call, n)  ((zval*)(((char*)(call)) + ((int)(n))))
#define EX_VAR(n)				ZEND_CALL_VAR(execute_data, n)

// ing3, 取得 p1 中第 p2 个 zval
#define EX_VAR_NUM(n)			ZEND_CALL_VAR_NUM(execute_data, n)

// ing4, 把偏移量转成 zval 序号 （减掉 ZEND_CALL_FRAME_SLOT）
#define EX_VAR_TO_NUM(n)		((uint32_t)((n) / sizeof(zval) - ZEND_CALL_FRAME_SLOT))
// ing4, 把 zval 序号 转成偏移量 （加上 ZEND_CALL_FRAME_SLOT）
#define EX_NUM_TO_VAR(n)		((uint32_t)(((n) + ZEND_CALL_FRAME_SLOT) * sizeof(zval)))

// ing4, 计算两个 指针之间的字节数
#define ZEND_OPLINE_TO_OFFSET(opline, target) \
	((char*)(target) - (char*)(opline))

// ing3, 计算 (p1)->opcodes[p3] 到p2 间的字节数
#define ZEND_OPLINE_NUM_TO_OFFSET(op_array, opline, opline_num) \
	((char*)&(op_array)->opcodes[opline_num] - (char*)(opline))

// ing3, 后移一段并返回操作码，(zend_op*)((char*)p1 + p2)
#define ZEND_OFFSET_TO_OPLINE(base, offset) \
	((zend_op*)(((char*)(base)) + (int)offset))
	
// ing3, 返回两个操作码间的距离 (zend_op*)((char*)p2 + p3) - p1->opcodes
#define ZEND_OFFSET_TO_OPLINE_NUM(op_array, base, offset) \
	(ZEND_OFFSET_TO_OPLINE(base, offset) - op_array->opcodes)


// 32位系统
#if ZEND_USE_ABS_JMP_ADDR

// 运行时跳转目标
/* run-time jump target */
// ing4, 访问 p2.jmp_addr
# define OP_JMP_ADDR(opline, node) \
	(node).jmp_addr

// ing3, p2.jmp_addr = p3
# define ZEND_SET_OP_JMP_ADDR(opline, node, val) do { \
		(node).jmp_addr = (val); \
	} while (0)

// ing3, 把跳转目标从编译时转到运行时
/* convert jump target from compile-time to run-time */
// (p3).jmp_addr = (p1)->opcodes + (p3).opline_num;
# define ZEND_PASS_TWO_UPDATE_JMP_TARGET(op_array, opline, node) do { \
		(node).jmp_addr = (op_array)->opcodes + (node).opline_num; \
	} while (0)

// ing3, 把跳转目标从运行时转回到编译时.
/* convert jump target back from run-time to compile-time */
// (p3).opline_num = (p3).jmp_addr - (p1)->opcodes;
# define ZEND_PASS_TWO_UNDO_JMP_TARGET(op_array, opline, node) do { \
		(node).opline_num = (node).jmp_addr - (op_array)->opcodes; \
	} while (0)

// 64位系统
#else

// 运行时跳转目标
/* run-time jump target */
// ing3, 后移一段并返回操作码，(zend_op*)((char*)p1 + p2.jmp_offset)
# define OP_JMP_ADDR(opline, node) \
	/* 后移一段并返回操作码，(zend_op*)((char*)p1 + p2) */ \
	ZEND_OFFSET_TO_OPLINE(opline, (node).jmp_offset)

// ing3, 计算 p1,p3 两个指针之间的字节数
# define ZEND_SET_OP_JMP_ADDR(opline, node, val) do { \
		/* 计算两个 指针之间的字节数 */ \
		(node).jmp_offset = ZEND_OPLINE_TO_OFFSET(opline, val); \
	} while (0)

// ing3, 把跳转目标从编译时转到运行时.
/* convert jump target from compile-time to run-time */
// 计算 (p1)->opcodes[p3.opline_num] 到p2 间的字节数
# define ZEND_PASS_TWO_UPDATE_JMP_TARGET(op_array, opline, node) do { \
		/* 计算 (p1)->opcodes[p3] 到p2 间的字节数 */ \
		(node).jmp_offset = ZEND_OPLINE_NUM_TO_OFFSET(op_array, opline, (node).opline_num); \
	} while (0)

// ing3, 把跳转目标从运行时转回到编译时.
/* convert jump target back from run-time to compile-time */
// 返回两个操作码间的距离 (zend_op*)((char*)p2 + p3.jmp_offset) - p1->opcodes
# define ZEND_PASS_TWO_UNDO_JMP_TARGET(op_array, opline, node) do { \
		/* 返回两个操作码间的距离 (zend_op*)((char*)p2 + p3) - p1->opcodes */ \
		(node).opline_num = ZEND_OFFSET_TO_OPLINE_NUM(op_array, opline, (node).jmp_offset); \
	} while (0)

#endif

// ing3, 获取操作码组中第 num 个 literal。p1->literals + p2
/* constant-time constant */
# define CT_CONSTANT_EX(op_array, num) \
	((op_array)->literals + (num))

// ing3, 获取当前操作码列表中 第 (p1).constant 个 literal
# define CT_CONSTANT(node) \
	CT_CONSTANT_EX(CG(active_op_array), (node).constant)


// 32位操作系统 
#if ZEND_USE_ABS_CONST_ADDR

// 运行时常量 
/* run-time constant */
// clear, 访问 (p2).zv。p1:opline,p2:node
# define RT_CONSTANT(opline, node) \
	(node).zv

// ing3, 把常量从编译时转到运行时: (p3).zv = p1->literals + p3.constant
/* convert constant from compile-time to run-time */
# define ZEND_PASS_TWO_UPDATE_CONSTANT(op_array, opline, node) do { \
		/* 获取操作码组中第 num 个 literal。p1->literals + p2 */ \
		(node).zv = CT_CONSTANT_EX(op_array, (node).constant); \
	} while (0)

// 64位操作系统 
#else

// 在运行时，常量和 op_array->opcodes 一起被分配 并且对应到相关的 操作码行
/* At run-time, constants are allocated together with op_array->opcodes
 * and addressed relatively to current opline.
 */

// 运行时常量
/* run-time constant */
// ing3, 在p1中通过偏移量p2.constant 获取 zval 指针。（VM中大量引用）
# define RT_CONSTANT(opline, node) \
	((zval*)(((char*)(opline)) + (int32_t)(node).constant))

// ing3, 把常量从编译时转到运行时: p3.constant = p1->literals + p3.constant - (char*)p2;
/* convert constant from compile-time to run-time */
# define ZEND_PASS_TWO_UPDATE_CONSTANT(op_array, opline, node) do { \
		/* 获取操作码组中第 num 个 literal。p1->literals + p2 */ \
		(node).constant = \
			(((char*)CT_CONSTANT_EX(op_array, (node).constant)) - \
			((char*)opline)); \
	} while (0)

#endif

// 把常量从运行时转回编译时
/* convert constant back from run-time to compile-time */
// ing3, 取得编译时的 常量zval序号 p1:操作码组，p2:操作码，p3:node
#define ZEND_PASS_TWO_UNDO_CONSTANT(op_array, opline, node) do { \
		/* 计算两个zval指针中间的差 */ \
		/* 在p1中通过偏移量p2.constant 获取 zval 指针。 */ \
		(node).constant = RT_CONSTANT(opline, node) - (op_array)->literals; \
	} while (0)

// ing3, 获取run_time_cache指针列表, p1->run_time_cache
#define RUN_TIME_CACHE(op_array) \
	ZEND_MAP_PTR_GET((op_array)->run_time_cache)

// 观察者用到，opcache用到。用于返回函数的观察者列表。
// ing3, 返回操作码组 p1 的运行时缓存的第 p1 个指针
#define ZEND_OP_ARRAY_EXTENSION(op_array, handle) \
	((void**)RUN_TIME_CACHE(op_array))[handle]

// znode 的 op_type， 5种类型
// 未使用的操作对象
#define IS_UNUSED	0		/* Unused operand */
// 常量
#define IS_CONST	(1<<0)
// 临时变量？
#define IS_TMP_VAR	(1<<1)
// 变量
#define IS_VAR		(1<<2)
// 编译过的变量。在编译过程中没有运算结果。
#define IS_CV		(1<<3)	/* Compiled variable */

/* Used for result.type of smart branch instructions */
#define IS_SMART_BRANCH_JMPZ  (1<<4)
#define IS_SMART_BRANCH_JMPNZ (1<<5)

#define ZEND_EXTRA_VALUE 1

#include "zend_globals.h"

typedef enum _zend_compile_position {
	ZEND_COMPILE_POSITION_AT_SHEBANG = 0,
	ZEND_COMPILE_POSITION_AT_OPEN_TAG,
	ZEND_COMPILE_POSITION_AFTER_OPEN_TAG
} zend_compile_position;

BEGIN_EXTERN_C()

void init_compiler(void);
void shutdown_compiler(void);
void zend_init_compiler_data_structures(void);

void zend_oparray_context_begin(zend_oparray_context *prev_context);
void zend_oparray_context_end(zend_oparray_context *prev_context);
void zend_file_context_begin(zend_file_context *prev_context);
void zend_file_context_end(zend_file_context *prev_context);

extern ZEND_API zend_op_array *(*zend_compile_file)(zend_file_handle *file_handle, int type);
extern ZEND_API zend_op_array *(*zend_compile_string)(zend_string *source_string, const char *filename, zend_compile_position position);

ZEND_API int ZEND_FASTCALL lex_scan(zval *zendlval, zend_parser_stack_elem *elem);
void startup_scanner(void);
void shutdown_scanner(void);

ZEND_API zend_string *zend_set_compiled_filename(zend_string *new_compiled_filename);
ZEND_API void zend_restore_compiled_filename(zend_string *original_compiled_filename);
ZEND_API zend_string *zend_get_compiled_filename(void);
ZEND_API int zend_get_compiled_lineno(void);
ZEND_API size_t zend_get_scanned_file_offset(void);

ZEND_API zend_string *zend_get_compiled_variable_name(const zend_op_array *op_array, uint32_t var);
// 如果要求线程安全
#ifdef ZTS
const char *zend_get_zendtext(void);
int zend_get_zendleng(void);
#endif

typedef zend_result (ZEND_FASTCALL *unary_op_type)(zval *, zval *);
typedef zend_result (ZEND_FASTCALL *binary_op_type)(zval *, zval *, zval *);

ZEND_API unary_op_type get_unary_op(int opcode);
ZEND_API binary_op_type get_binary_op(int opcode);

void zend_stop_lexing(void);
void zend_emit_final_return(bool return_one);

/* Used during AST construction */
zend_ast *zend_ast_append_str(zend_ast *left, zend_ast *right);
zend_ast *zend_negate_num_string(zend_ast *ast);
uint32_t zend_add_class_modifier(uint32_t flags, uint32_t new_flag);
uint32_t zend_add_member_modifier(uint32_t flags, uint32_t new_flag);
bool zend_handle_encoding_declaration(zend_ast *ast);

ZEND_API zend_class_entry *zend_bind_class_in_slot(
		zval *class_table_slot, zval *lcname, zend_string *lc_parent_name);
ZEND_API zend_result do_bind_function(zend_function *func, zval *lcname);
ZEND_API zend_result do_bind_class(zval *lcname, zend_string *lc_parent_name);

void zend_resolve_goto_label(zend_op_array *op_array, zend_op *opline);

ZEND_API void function_add_ref(zend_function *function);
zend_string *zval_make_interned_string(zval *zv);

#define INITIAL_OP_ARRAY_SIZE 64


/* helper functions in zend_language_scanner.l */
struct _zend_arena;

ZEND_API zend_op_array *compile_file(zend_file_handle *file_handle, int type);
ZEND_API zend_op_array *compile_string(zend_string *source_string, const char *filename, zend_compile_position position);
ZEND_API zend_op_array *compile_filename(int type, zend_string *filename);
ZEND_API zend_ast *zend_compile_string_to_ast(
		zend_string *code, struct _zend_arena **ast_arena, zend_string *filename);
ZEND_API int zend_execute_scripts(int type, zval *retval, int file_count, ...);
ZEND_API int open_file_for_scanning(zend_file_handle *file_handle);
ZEND_API void init_op_array(zend_op_array *op_array, zend_uchar type, int initial_ops_size);
ZEND_API void destroy_op_array(zend_op_array *op_array);
ZEND_API void zend_destroy_static_vars(zend_op_array *op_array);
ZEND_API void zend_destroy_file_handle(zend_file_handle *file_handle);
ZEND_API void zend_cleanup_mutable_class_data(zend_class_entry *ce);
ZEND_API void zend_cleanup_internal_class_data(zend_class_entry *ce);
ZEND_API void zend_type_release(zend_type type, bool persistent);
ZEND_API zend_string *zend_create_member_string(zend_string *class_name, zend_string *member_name);


ZEND_API ZEND_COLD void zend_user_exception_handler(void);

// ing4, 如果有异常 并且 有用户定义的异常处理器, 调用这个处理器
#define zend_try_exception_handler() do { \
		/* 如果有异常 */ \
		if (UNEXPECTED(EG(exception))) { \
			/* 如果有用户定义的异常处理器 */ \
			if (Z_TYPE(EG(user_exception_handler)) != IS_UNDEF) { \
				/* 调用这个处理器 */ \
				zend_user_exception_handler(); \
			} \
		} \
	} while (0)

void zend_free_internal_arg_info(zend_internal_function *function);
ZEND_API void destroy_zend_function(zend_function *function);
ZEND_API void zend_function_dtor(zval *zv);
ZEND_API void destroy_zend_class(zval *zv);
void zend_class_add_ref(zval *zv);

ZEND_API zend_string *zend_mangle_property_name(const char *src1, size_t src1_length, const char *src2, size_t src2_length, bool internal);

// ing3, 把完整属性名拆分成类名和属性名。p1:完整名称，p2:类名，p3:属性名
#define zend_unmangle_property_name(mangled_property, class_name, prop_name) \
        zend_unmangle_property_name_ex(mangled_property, class_name, prop_name, NULL)
ZEND_API zend_result zend_unmangle_property_name_ex(const zend_string *name, const char **class_name, const char **prop_name, size_t *prop_len);

// ing3, 把完整属性名拆分成类名和属性名
static zend_always_inline const char *zend_get_unmangled_property_name(const zend_string *mangled_prop) {
	const char *class_name, *prop_name;
	// 把完整属性名拆分成类名和属性名
	zend_unmangle_property_name(mangled_prop, &class_name, &prop_name);
	return prop_name;
}

#define ZEND_FUNCTION_DTOR zend_function_dtor
#define ZEND_CLASS_DTOR destroy_zend_class

// 回调函数，zend_opcode.c使用
typedef bool (*zend_needs_live_range_cb)(zend_op_array *op_array, zend_op *opline);

ZEND_API void zend_recalc_live_ranges(
	zend_op_array *op_array, zend_needs_live_range_cb needs_live_range);

ZEND_API void pass_two(zend_op_array *op_array);
ZEND_API bool zend_is_compiling(void);
ZEND_API char *zend_make_compiled_string_description(const char *name);
ZEND_API void zend_initialize_class_data(zend_class_entry *ce, bool nullify_handlers);
uint32_t zend_get_class_fetch_type(zend_string *name);
ZEND_API zend_uchar zend_get_call_op(const zend_op *init_op, zend_function *fbc);
ZEND_API bool zend_is_smart_branch(const zend_op *opline);

typedef bool (*zend_auto_global_callback)(zend_string *name);
typedef struct _zend_auto_global {
	zend_string *name;
	zend_auto_global_callback auto_global_callback;
	// ？
	bool jit;
	// ？
	bool armed;
} zend_auto_global;

ZEND_API zend_result zend_register_auto_global(zend_string *name, bool jit, zend_auto_global_callback auto_global_callback);
ZEND_API void zend_activate_auto_globals(void);
ZEND_API bool zend_is_auto_global(zend_string *name);
ZEND_API bool zend_is_auto_global_str(const char *name, size_t len);
ZEND_API size_t zend_dirname(char *path, size_t len);
ZEND_API void zend_set_function_arg_flags(zend_function *func);

int ZEND_FASTCALL zendlex(zend_parser_stack_elem *elem);

void zend_assert_valid_class_name(const zend_string *const_name);

zend_string *zend_type_to_string_resolved(zend_type type, zend_class_entry *scope);
ZEND_API zend_string *zend_type_to_string(zend_type type);

/* BEGIN: OPCODES */

#include "zend_vm_opcodes.h"

/* END: OPCODES */

// 获取类
/* class fetches */
// 默认
#define ZEND_FETCH_CLASS_DEFAULT	0
// self
#define ZEND_FETCH_CLASS_SELF		1
// parent
#define ZEND_FETCH_CLASS_PARENT		2
// static
#define ZEND_FETCH_CLASS_STATIC		3
// zend_fetch_ce_from_cache_slot：zend_execute.c 中，字串没有关联类时，带这个标记
#define ZEND_FETCH_CLASS_AUTO		4
// 接口
#define ZEND_FETCH_CLASS_INTERFACE	5
// trait
#define ZEND_FETCH_CLASS_TRAIT		6
// 以上全部（self,parent,static,接口,trait,auto?）
#define ZEND_FETCH_CLASS_MASK        0x0f
// 不可以 autoload
#define ZEND_FETCH_CLASS_NO_AUTOLOAD 0x80
// 不报错
#define ZEND_FETCH_CLASS_SILENT      0x0100
// 
#define ZEND_FETCH_CLASS_EXCEPTION   0x0200
//
#define ZEND_FETCH_CLASS_ALLOW_UNLINKED 0x0400
//
#define ZEND_FETCH_CLASS_ALLOW_NEARLY_LINKED 0x0800

// 防止与 ZEND_ACC_(PUBLIC|PROTECTED|PRIVATE) 冲突
/* These should not clash with ZEND_ACC_(PUBLIC|PROTECTED|PRIVATE) */
#define ZEND_PARAM_REF      (1<<3)
#define ZEND_PARAM_VARIADIC (1<<4)

// 绝对路径
#define ZEND_NAME_FQ       0
// 非绝对路径引用，可以带 \ 或不带 （参看parser）
#define ZEND_NAME_NOT_FQ   1
// namespace 开头
#define ZEND_NAME_RELATIVE 2

/* ZEND_FETCH_ flags in class name AST of new const expression must not clash with ZEND_NAME_ flags */
#define ZEND_CONST_EXPR_NEW_FETCH_TYPE_SHIFT 2

#define ZEND_TYPE_NULLABLE (1<<8)

#define ZEND_ARRAY_SYNTAX_LIST 1  /* list() */
#define ZEND_ARRAY_SYNTAX_LONG 2  /* array() */
#define ZEND_ARRAY_SYNTAX_SHORT 3 /* [] */

// 变量状态 backpatching
/* var status for backpatching */
// 读
#define BP_VAR_R			0
// 写
#define BP_VAR_W			1
// 读写。相当于 update
#define BP_VAR_RW			2
// isset, 检查存在
#define BP_VAR_IS			3
// 函数参数
#define BP_VAR_FUNC_ARG		4
// 删除变量
#define BP_VAR_UNSET		5

// 内部函数 
#define ZEND_INTERNAL_FUNCTION		1
// 用户函数 
#define ZEND_USER_FUNCTION			2
// eval代码
#define ZEND_EVAL_CODE				4

// clear, 判断：是用户定义的代码
#define ZEND_USER_CODE(type)		((type) != ZEND_INTERNAL_FUNCTION)

// 内置类
#define ZEND_INTERNAL_CLASS         1
// 用户类
#define ZEND_USER_CLASS             2

// eval
#define ZEND_EVAL				(1<<0)
// include
#define ZEND_INCLUDE			(1<<1)
// include_once
#define ZEND_INCLUDE_ONCE		(1<<2)
// require
#define ZEND_REQUIRE			(1<<3)
// require_one
#define ZEND_REQUIRE_ONCE		(1<<4)

// 全局或本地获取变量，全局，本地，独占全局？
/* global/local fetches */
#define ZEND_FETCH_GLOBAL		(1<<1)
#define ZEND_FETCH_LOCAL		(1<<2)
#define ZEND_FETCH_GLOBAL_LOCK	(1<<3)

// fetch type 掩码
#define ZEND_FETCH_TYPE_MASK	0xe

// 这几个里只能用一个
/* Only one of these can ever be in use */
#define ZEND_FETCH_REF			1
#define ZEND_FETCH_DIM_WRITE	2
#define ZEND_FETCH_OBJ_FLAGS	3

/* Used to mark what kind of operation a writing FETCH_DIM is used in,
 * to produce a more precise error on incorrect string offset use. */
#define ZEND_FETCH_DIM_REF 1
#define ZEND_FETCH_DIM_DIM 2
// 从数组中获取对象
#define ZEND_FETCH_DIM_OBJ 3
#define ZEND_FETCH_DIM_INCDEC 4

// empty
#define ZEND_ISEMPTY			(1<<0)

#define ZEND_LAST_CATCH			(1<<0)

#define ZEND_FREE_ON_RETURN     (1<<0)

#define ZEND_FREE_SWITCH        (1<<1)

// 通过普通变量传递
#define ZEND_SEND_BY_VAL     0u
// 通过引用传递
#define ZEND_SEND_BY_REF     1u
// 优先通过引用传递
#define ZEND_SEND_PREFER_REF 2u

#define ZEND_THROW_IS_EXPR 1u

#define ZEND_FCALL_MAY_HAVE_EXTRA_NAMED_PARAMS 1

// 发送模式，is_variadic（字典？），is_promoted（？），is_tentative（实验性的） 这几个标记存在 zend_type里
/* The send mode, the is_variadic, the is_promoted, and the is_tentative flags are stored as part of zend_type */

// #define _ZEND_TYPE_EXTRA_FLAGS_SHIFT 25
#define _ZEND_SEND_MODE_SHIFT _ZEND_TYPE_EXTRA_FLAGS_SHIFT

// ing3 参数是否是字典类型，这个在返回值里不能用
// is_variadic ，是三个里面的第一个, 1 << 27
#define _ZEND_IS_VARIADIC_BIT (1 << (_ZEND_TYPE_EXTRA_FLAGS_SHIFT + 2))

// 这个东东很少用到
// is_promoted ，是三个里面的第二个, 1 << 28
#define _ZEND_IS_PROMOTED_BIT (1 << (_ZEND_TYPE_EXTRA_FLAGS_SHIFT + 3))

// ing3, 这个东西是指返回类型是否允null，例如 function a(): ?string {} ,前面加个问号就是 is_tentative
// 它主要影 子类方法和父类的兼容，如果父类方法 f():?string {} 子类方法可以是 f():string {} 或 f():null {}
// is_tentative ，是三个里面的第三个, 1 << 29 
#define _ZEND_IS_TENTATIVE_BIT (1 << (_ZEND_TYPE_EXTRA_FLAGS_SHIFT + 4))

// ing3, 从arg_info中获取发送模式：0普通变量，1引用传递，2优先引用传递
#define ZEND_ARG_SEND_MODE(arg_info) \
	((ZEND_TYPE_FULL_MASK((arg_info)->type) >> _ZEND_SEND_MODE_SHIFT) & 3)
// ing3, 验证是否有 is_variadic
#define ZEND_ARG_IS_VARIADIC(arg_info) \
	((ZEND_TYPE_FULL_MASK((arg_info)->type) & _ZEND_IS_VARIADIC_BIT) != 0)
// ing3, 验证是否有 is_promoted
#define ZEND_ARG_IS_PROMOTED(arg_info) \
	((ZEND_TYPE_FULL_MASK((arg_info)->type) & _ZEND_IS_PROMOTED_BIT) != 0)
// ing3, 验证是否有 is_tentative
#define ZEND_ARG_TYPE_IS_TENTATIVE(arg_info) \
	((ZEND_TYPE_FULL_MASK((arg_info)->type) & _ZEND_IS_TENTATIVE_BIT) != 0)

// 短路调用需要 isset 方法。在 zend_compile.c 中定义，在 ZEND_AST_DIM 
#define ZEND_DIM_IS					(1 << 0) /* isset fetch needed for null coalesce. Set in zend_compile.c for ZEND_AST_DIM nested within ZEND_AST_COALESCE. */
// 弃用的圆括号用法
#define ZEND_DIM_ALTERNATIVE_SYNTAX	(1 << 1) /* deprecated curly brace usage */

/* Attributes for ${} encaps var in strings (ZEND_AST_DIM or ZEND_AST_VAR node) */
/* ZEND_AST_VAR nodes can have any of the ZEND_ENCAPS_VAR_* flags */
/* ZEND_AST_DIM flags can have ZEND_DIM_ALTERNATIVE_SYNTAX or ZEND_ENCAPS_VAR_DOLLAR_CURLY during the parse phase (ZEND_DIM_ALTERNATIVE_SYNTAX is a thrown fatal error). */
#define ZEND_ENCAPS_VAR_DOLLAR_CURLY (1 << 0)
#define ZEND_ENCAPS_VAR_DOLLAR_CURLY_VAR_VAR (1 << 1)

/* Make sure these don't clash with ZEND_FETCH_CLASS_* flags. */
#define IS_CONSTANT_CLASS                    0x400 /* __CLASS__ in trait */
#define IS_CONSTANT_UNQUALIFIED_IN_NAMESPACE 0x800

// ing3, 获取指定参数的发送模式：0普通变量，1引用传递，2优先引用传递
static zend_always_inline bool zend_check_arg_send_type(const zend_function *zf, uint32_t arg_num, uint32_t mask)
{
	arg_num--;
	// 如果给出参数大于需要的参数
	if (UNEXPECTED(arg_num >= zf->common.num_args)) {
		// 如果函数不允许字典参数
		if (EXPECTED((zf->common.fn_flags & ZEND_ACC_VARIADIC) == 0)) {
			// 返回失败
			return 0;
		}
		// 函数需要的参数数量
		arg_num = zf->common.num_args;
	}
	// 从arg_info中获取发送模式：0普通变量，1引用传递，2优先引用传递
	// 返回 第 arg_num 个参数的 发送模式
	return UNEXPECTED((ZEND_ARG_SEND_MODE(&zf->common.arg_info[arg_num]) & mask) != 0);
}

// ing4, 验证指定参数的发送模式，是否为引用传递
#define ARG_MUST_BE_SENT_BY_REF(zf, arg_num) \
	zend_check_arg_send_type(zf, arg_num, ZEND_SEND_BY_REF)

// ing4, 验证指定参数的发送模式，是否为引用传递 或 优先引用传递
#define ARG_SHOULD_BE_SENT_BY_REF(zf, arg_num) \
	zend_check_arg_send_type(zf, arg_num, ZEND_SEND_BY_REF|ZEND_SEND_PREFER_REF)

// ing4, 验证指定参数的发送模式，是否为 优先引用传递
#define ARG_MAY_BE_SENT_BY_REF(zf, arg_num) \
	zend_check_arg_send_type(zf, arg_num, ZEND_SEND_PREFER_REF)

// 检验前12个参数的快速api
/* Quick API to check first 12 arguments */
#define MAX_ARG_FLAG_NUM 12

// \build\php.m4 里的配置
#ifdef WORDS_BIGENDIAN
// ing3, 和下面的差不多，只是没有预留初始空间
# define ZEND_SET_ARG_FLAG(zf, arg_num, mask) do { \
		(zf)->quick_arg_flags |= ((mask) << ((arg_num) - 1) * 2); \
	} while (0)
# define ZEND_CHECK_ARG_FLAG(zf, arg_num, mask) \
	(((zf)->quick_arg_flags >> (((arg_num) - 1) * 2)) & (mask))
// windows走这里
#else
// ing3, 在函数 ->quick_arg_flags 中添加指定序号的参数的 标记，每个参数2个位的空间
# define ZEND_SET_ARG_FLAG(zf, arg_num, mask) do { \
		/* mask 先左移6位，再左移 参数序号*2这么多位，然后添加到标记中 */ \
		(zf)->quick_arg_flags |= (((mask) << 6) << (arg_num) * 2); \
	} while (0)
// ing3, 在函数 ->quick_arg_flags 中验证 指定序号的参数的 标记，每个标记2个位的空间
# define ZEND_CHECK_ARG_FLAG(zf, arg_num, mask) \
	(((zf)->quick_arg_flags >> (((arg_num) + 3) * 2)) & (mask))
#endif

// ing4, 检验是否必须通过引用传递
#define QUICK_ARG_MUST_BE_SENT_BY_REF(zf, arg_num) \
	ZEND_CHECK_ARG_FLAG(zf, arg_num, ZEND_SEND_BY_REF)

// ing4, 检验是否可以通过引用传递
#define QUICK_ARG_SHOULD_BE_SENT_BY_REF(zf, arg_num) \
	/* 检验 ZEND_SEND_BY_REF|ZEND_SEND_PREFER_REF 标记 */ \
	ZEND_CHECK_ARG_FLAG(zf, arg_num, ZEND_SEND_BY_REF|ZEND_SEND_PREFER_REF)

// ing3, 在函数 ->quick_arg_flags 中验证 ZEND_SEND_PREFER_REF 标记，每个标记2个位的空间
#define QUICK_ARG_MAY_BE_SENT_BY_REF(zf, arg_num) \
	/* 在函数 ->quick_arg_flags 中验证 指定序号的参数的 标记，每个标记2个位的空间 */ \
	ZEND_CHECK_ARG_FLAG(zf, arg_num, ZEND_SEND_PREFER_REF)

#define ZEND_RETURN_VAL 0
#define ZEND_RETURN_REF 1

// 绑定相关
#define ZEND_BIND_VAL      0
#define ZEND_BIND_REF      1
#define ZEND_BIND_IMPLICIT 2
#define ZEND_BIND_EXPLICIT 4

// 返回相关
#define ZEND_RETURNS_FUNCTION (1<<0)
#define ZEND_RETURNS_VALUE    (1<<1)

// 数组想装
#define ZEND_ARRAY_ELEMENT_REF		(1<<0)
#define ZEND_ARRAY_NOT_PACKED		(1<<1)
#define ZEND_ARRAY_SIZE_SHIFT		2

// 括号里的三元操作属性
/* Attribute for ternary inside parentheses */
#define ZEND_PARENTHESIZED_CONDITIONAL 1

// use 语句节点和 符号表
/* For "use" AST nodes and the seen symbol table */
#define ZEND_SYMBOL_CLASS    (1<<0)
#define ZEND_SYMBOL_FUNCTION (1<<1)
#define ZEND_SYMBOL_CONST    (1<<2)

// 所有算加法的操作码都是偶数，减法是奇数
/* All increment opcodes are even (decrement are odd) */
// ing3, 验证是否是加法操作码
#define ZEND_IS_INCREMENT(opcode) (((opcode) & 1) == 0)

// ing3, 判断是否是 二元 操作码
#define ZEND_IS_BINARY_ASSIGN_OP_OPCODE(opcode) \
	(((opcode) >= ZEND_ADD) && ((opcode) <= ZEND_POW))

// 在编译时临时用到的伪操作码
/* Pseudo-opcodes that are used only temporarily during compilation */
// goto语句
#define ZEND_GOTO  253
// break 语句
#define ZEND_BRK   254
// continue语句
#define ZEND_CONT  255


END_EXTERN_C()
// clear, 魔术方法名
#define ZEND_CLONE_FUNC_NAME		"__clone"
#define ZEND_CONSTRUCTOR_FUNC_NAME	"__construct"
#define ZEND_DESTRUCTOR_FUNC_NAME	"__destruct"
#define ZEND_GET_FUNC_NAME          "__get"
#define ZEND_SET_FUNC_NAME          "__set"
#define ZEND_UNSET_FUNC_NAME        "__unset"
#define ZEND_ISSET_FUNC_NAME        "__isset"
#define ZEND_CALL_FUNC_NAME         "__call"
#define ZEND_CALLSTATIC_FUNC_NAME   "__callstatic"
#define ZEND_TOSTRING_FUNC_NAME     "__tostring"
// 闭包用的
#define ZEND_INVOKE_FUNC_NAME       "__invoke"
#define ZEND_DEBUGINFO_FUNC_NAME    "__debuginfo"

/* The following constants may be combined in CG(compiler_options)
 * to change the default compiler behavior */

// 生成扩展的调试信息
/* generate extended debug information */
#define ZEND_COMPILE_EXTENDED_STMT              (1<<0)
#define ZEND_COMPILE_EXTENDED_FCALL             (1<<1)
#define ZEND_COMPILE_EXTENDED_INFO              (ZEND_COMPILE_EXTENDED_STMT|ZEND_COMPILE_EXTENDED_FCALL)

// 调用扩展的操作码处理器
/* call op_array handler of extensions */
#define ZEND_COMPILE_HANDLE_OP_ARRAY            (1<<2)

// 对内置函数调用创建 ZEND_INIT_FCALL_BY_NAME 而不是 ZEND_INIT_FCALL 操作码
/* generate ZEND_INIT_FCALL_BY_NAME for internal functions instead of ZEND_INIT_FCALL */
#define ZEND_COMPILE_IGNORE_INTERNAL_FUNCTIONS  (1<<3)

// 不要对继承内置类的类进行早期绑定。在命名空间中假定编译时没有的内置类，会在运行时存在
/* don't perform early binding for classes inherited form internal ones;
 * in namespaces assume that internal class that doesn't exist at compile-time
 * may appear in run-time */
#define ZEND_COMPILE_IGNORE_INTERNAL_CLASSES    (1<<4)

// 创建 ZEND_DECLARE_CLASS_DELAYED 操作码来延时早期绑定
/* generate ZEND_DECLARE_CLASS_DELAYED opcode to delay early binding */
#define ZEND_COMPILE_DELAYED_BINDING            (1<<5)

// 禁用编译时的 常量替换
/* disable constant substitution at compile-time */
#define ZEND_COMPILE_NO_CONSTANT_SUBSTITUTION   (1<<6)

// 禁用编译时的持久常量替换
/* disable substitution of persistent constants at compile-time */
#define ZEND_COMPILE_NO_PERSISTENT_CONSTANT_SUBSTITUTION	(1<<8)

// 对用户函数 创建 ZEND_INIT_FCALL_BY_NAME 而不是 ZEND_INIT_FCALL
/* generate ZEND_INIT_FCALL_BY_NAME for userland functions instead of ZEND_INIT_FCALL */
#define ZEND_COMPILE_IGNORE_USER_FUNCTIONS      (1<<9)

// 对所有类强制 ZEND_ACC_USE_GUARDS
/* force ZEND_ACC_USE_GUARDS for all classes */
#define ZEND_COMPILE_GUARDS						(1<<10)

// ？？
/* disable builtin special case function calls */
#define ZEND_COMPILE_NO_BUILTINS				(1<<11)

// 可能存储在文件缓存中的编译结果
/* result of compilation may be stored in file cache */
#define ZEND_COMPILE_WITH_FILE_CACHE			(1<<12)

// 忽略其他文件中声名的类和函数
/* ignore functions and classes declared in other files */
#define ZEND_COMPILE_IGNORE_OTHER_FILES			(1<<13)

// opcache_compile_file() 中引用 编译器
/* this flag is set when compiler invoked by opcache_compile_file() */
#define ZEND_COMPILE_WITHOUT_EXECUTION          (1<<14)

// 在预加载中引用 编译器
/* this flag is set when compiler invoked during preloading */
#define ZEND_COMPILE_PRELOAD                    (1<<15)

// 对switch语句禁用 跳转表优化
/* disable jumptable optimization for switch statements */
#define ZEND_COMPILE_NO_JUMPTABLES				(1<<16)

// 在分离的进程中进行预加载，并使用编译器时，会设置这个标记
/* this flag is set when compiler invoked during preloading in separate process */
#define ZEND_COMPILE_PRELOAD_IN_CHILD           (1<<17)

// 忽略观察者提示信息，例如 
/* ignore observer notifications, e.g. to manually notify afterwards in a post-processing step after compilation */
#define ZEND_COMPILE_IGNORE_OBSERVER			(1<<18)

// CG(compiler_options) 的默认值
/* The default value for CG(compiler_options) */
#define ZEND_COMPILE_DEFAULT					ZEND_COMPILE_HANDLE_OP_ARRAY

// eval() 过程中 CG(compiler_options) 的默认值
/* The default value for CG(compiler_options) during eval() */
#define ZEND_COMPILE_DEFAULT_FOR_EVAL			0

ZEND_API bool zend_is_op_long_compatible(zval *op);
ZEND_API bool zend_binary_op_produces_error(uint32_t opcode, zval *op1, zval *op2);
ZEND_API bool zend_unary_op_produces_error(uint32_t opcode, zval *op);

#endif /* ZEND_COMPILE_H */
