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
   | Authors: Bob Weinand <bwoebi@php.net>                                |
   |          Dmitry Stogov <dmitry@php.net>                              |
   |          Nikita Popov <nikic@php.net>                                |
   +----------------------------------------------------------------------+
*/

#ifndef ZEND_AST_H
#define ZEND_AST_H

#include "zend.h"

#ifndef ZEND_AST_SPEC
# define ZEND_AST_SPEC 1
#endif

#define ZEND_AST_SPECIAL_SHIFT      6
#define ZEND_AST_IS_LIST_SHIFT      7
#define ZEND_AST_NUM_CHILDREN_SHIFT 8

// 共118个值, 看ast. 对应的业务逻辑也很清晰
enum _zend_ast_kind {
	
// 特殊节点，不在parse时创建
	/* special nodes */
	// zend_ast_create_zval_int, zend_ast_tree_copy 两个方法赋值
	// /Zend/zend_enum.c 也用到，比较复杂
	// 只有解析和编译时用到
	ZEND_AST_ZVAL = 1 << ZEND_AST_SPECIAL_SHIFT,
	// compile 时创建的常量 
	// zend_ast_create_constant, zend_ast_tree_copy 两个方法赋值
	ZEND_AST_CONSTANT,
	// zend_ast_create_znode 赋值
	ZEND_AST_ZNODE,

// 声明型节点
	/* declaration nodes */
	// 定义函数
	ZEND_AST_FUNC_DECL,
	// 定义闭包和闭包的use参数，2处
	ZEND_AST_CLOSURE,
	// 定义类方法
	ZEND_AST_METHOD,
	// 定义class，trait，interface，enum，匿名类 5处
	ZEND_AST_CLASS,
	// fn()=> 语句
	ZEND_AST_ARROW_FUNC,

// 列表型节点
		// 列表的特点是：通常有一个标识用来创建列表，另一个标识往列表里添加元素
	/* list nodes */
	// argument_list， 调用方法或实例化类时传入的参数列表，多处
	ZEND_AST_ARG_LIST = 1 << ZEND_AST_IS_LIST_SHIFT,
	// 键值对，用于定义和遍历数组。 这个会直接创建成 list 
	ZEND_AST_ARRAY,
	// 引号或反引号中的变量列表, 2处
	ZEND_AST_ENCAPS_LIST,
	// 用在for语句和match语句里的单个公式（公式本身是递归结构），2处
	ZEND_AST_EXPR_LIST,
	// 最外层文档，内部代码块，unset语句，global语句,static 语句，echo语句，
	// 类，interface, trait , enum,中的业务逻辑，都用它包装
	ZEND_AST_STMT_LIST,
	// if语句 ，2处（有或没有else）
	ZEND_AST_IF,
	// 空switch语句
	ZEND_AST_SWITCH_LIST,
	// 空 catch语句
	ZEND_AST_CATCH_LIST,
	// 声明函数，匿名函数，fn()=> 或类方法时的单个参数，或空列表，2处
	ZEND_AST_PARAM_LIST,
	// 闭包Use里的 每个，基本变量或变量地址
	ZEND_AST_CLOSURE_USES,
	// 声明每个类属性
	ZEND_AST_PROP_DECL,
	// 用const 语句或者declare语句声明的每个常量
	ZEND_AST_CONST_DECL,
	// 用const 语句语句声明的每个 类常量
	ZEND_AST_CLASS_CONST_DECL,
	// 类名列表中的每一个类名，use trait，extends, implements 时用到的
	ZEND_AST_NAME_LIST,
	// 一条 use trait 语句后面{} 里的每一段内容
	ZEND_AST_TRAIT_ADAPTATIONS,
	// use的每个类名
	ZEND_AST_USE,
	// 形参类型明，多个之间 | 会触发这个 
	ZEND_AST_TYPE_UNION,
	// 形参类型明，多个之间 & 会触发这个 
	ZEND_AST_TYPE_INTERSECTION,
	// 每个修饰属性列表
		// 后面可以跟 
		//	attributed_statement: function , class, trait, interface, enum
		//	parameter,
		//	attributed_class_statement,
		//	anonymous_class,
		//	inline_function: function(), fn() => 
			// ?
		//	T_STATIC inline_function: static function(), fn() => 
	ZEND_AST_ATTRIBUTE_LIST,
	// 放在 #[] 里的修饰属性列表 前面的分组，
	// 例如  #[Foo, Bar(a: "foo", b: 1234), Baz("foo", 1234), X(NO_ERROR), Y(new stdClass)]
	ZEND_AST_ATTRIBUTE_GROUP,
	// match 语句中的条件列表
	ZEND_AST_MATCH_ARM_LIST,

// 0 个子节点
	/* 0 child nodes */
	// 魔术常量
	ZEND_AST_MAGIC_CONST = 0 << ZEND_AST_NUM_CHILDREN_SHIFT,
	// 变量类型或方法的返回类型，array,callable,static,mixed
	ZEND_AST_TYPE,
	// compile.c 创建, __CLASS__
	ZEND_AST_CONSTANT_CLASS,
	// (...) ，可伸缩形参
	ZEND_AST_CALLABLE_CONVERT,

// 1 个子节点
	/* 1 child node */
	// 最简单的变量
	ZEND_AST_VAR = 1 << ZEND_AST_NUM_CHILDREN_SHIFT,
	// 常量名
	ZEND_AST_CONST,
	// ...$arr 解压数组
	ZEND_AST_UNPACK,
	// 操作符 +
	ZEND_AST_UNARY_PLUS,
	// 操作符 -
	ZEND_AST_UNARY_MINUS,
	// zend_ast.h 中创建
	ZEND_AST_CAST,
	// empty 函数
	ZEND_AST_EMPTY,
	// isset 函数的每个参数
	ZEND_AST_ISSET,
	// 静默操作符 @
	ZEND_AST_SILENCE,
	// 反引号语句
	ZEND_AST_SHELL_EXEC,
	// clone 语句
	ZEND_AST_CLONE,
	// exit 语句
	ZEND_AST_EXIT,
	// print 语句
	ZEND_AST_PRINT,
	// include, require, eval 语句
	ZEND_AST_INCLUDE_OR_EVAL,
	// ！~ 两个操作符
	ZEND_AST_UNARY_OP,
	// 前面 ++
	ZEND_AST_PRE_INC,
	// 前面 --
	ZEND_AST_PRE_DEC,
	// 后面 ++
	ZEND_AST_POST_INC,
	// 后面 --
	ZEND_AST_POST_DEC,
	// 长yield 语句 , yield expr1 => expr2
	ZEND_AST_YIELD_FROM,
	// ast.c 创建，通过 ::class 常量获取类名
	ZEND_AST_CLASS_NAME,
	// 声明全局变量
	ZEND_AST_GLOBAL,
	// unset 语句
	ZEND_AST_UNSET,
	// return 语句
	ZEND_AST_RETURN,
	// goto 跳转点
	ZEND_AST_LABEL,
	// foreach($a as $k=>&$v)中的传址遍历
	ZEND_AST_REF,
	// __halt_compiler 语句
	ZEND_AST_HALT_COMPILER,
	// echo 语句或者内嵌html代码
	ZEND_AST_ECHO,
	// throw 语句
	ZEND_AST_THROW,
	// goto 语句
	ZEND_AST_GOTO,
	// break 语句
	ZEND_AST_BREAK,
	// continue 语句
	ZEND_AST_CONTINUE,

// 2 个子节点
	/* 2 child nodes */
	// ?
	ZEND_AST_DIM = 2 << ZEND_AST_NUM_CHILDREN_SHIFT,
	// 引用非null对象的属性
	ZEND_AST_PROP,
	// 引用允null对象的属性，
	ZEND_AST_NULLSAFE_PROP,
	// 类的静态成员变量，获取或赋值
	ZEND_AST_STATIC_PROP,
	// 完整的函数或匿名函数调用
	ZEND_AST_CALL,
	// ast.c	创建，zend_ast_create_class_const_or_name
	// 引用类中的常量
	ZEND_AST_CLASS_CONST,
	// 赋值语句，包含解压赋值 list($a,$b) = [1,2];
	ZEND_AST_ASSIGN,
	// 传址赋值
	ZEND_AST_ASSIGN_REF,
	// zend_ast.h 创建， zend_ast_create_assign_op 方法， parser.y调用
	// 二元赋值语句，变量后面加 (操作符和= ) 再加表达式
	ZEND_AST_ASSIGN_OP,
	// zend_ast.h 创建， zend_ast_create_binary_op 方法， parser.y调用
	// 普通二元操作符语句，expr 操作符 expr
	ZEND_AST_BINARY_OP,
	// >, 大于和小于都用这两个类型，在处理时逻辑反转
	ZEND_AST_GREATER,
	// >=
	ZEND_AST_GREATER_EQUAL,
	// 布尔型和逻辑型的 and
	ZEND_AST_AND,
	// 布尔型和逻辑型的 or
	ZEND_AST_OR,
	// 数组里的一组键值对 => ZEND_AST_ARRAY 是列表
	ZEND_AST_ARRAY_ELEM,
	// new 普通类或匿名类
	ZEND_AST_NEW,
	// instanceof 语句
	ZEND_AST_INSTANCEOF,
	// yield 语句
	ZEND_AST_YIELD,
	// ?? 连接的两个表达式
	ZEND_AST_COALESCE,
	// ??= 连接的两个表达式
	ZEND_AST_ASSIGN_COALESCE,
	// static 后面的每个变量
	ZEND_AST_STATIC,
	// 完整的 while 语句
	ZEND_AST_WHILE,
	// 完整的 do while 语句
	ZEND_AST_DO_WHILE,
	// 整套的 if elseif else 语句块
	ZEND_AST_IF_ELEM,
	// 整套的 switch 语句块
	ZEND_AST_SWITCH,
	// switch 语句中的每一个case 
	ZEND_AST_SWITCH_CASE,
	// declare() {}, 后面这部分代码块
	ZEND_AST_DECLARE,
	// 引用trait的一套完整代码 
	ZEND_AST_USE_TRAIT,
	// 引用一个trait ?
	ZEND_AST_TRAIT_PRECEDENCE,
	// 引用trait里的指定方法
	ZEND_AST_METHOD_REFERENCE,
	// namespace 语句
	ZEND_AST_NAMESPACE,
	// 被use 的每个命名空间
	ZEND_AST_USE_ELEM,
	// 给trait的方法设置别名
	ZEND_AST_TRAIT_ALIAS,
	// 自定义引用命名空间，use namespace\{}
	ZEND_AST_GROUP_USE,
	// 一组完整的类常量定义
	ZEND_AST_CLASS_CONST_GROUP,
	// 使用的每一个修饰属性
	ZEND_AST_ATTRIBUTE,
	// 一套完整的match语句
	ZEND_AST_MATCH,
	// match里的每个arm语句
	ZEND_AST_MATCH_ARM,
	// 参数列表中每一个规定了类型的参数（形参）
	ZEND_AST_NAMED_ARG,

// 3 个子节点
	/* 3 child nodes */
	// 调用非Null对象方法
	ZEND_AST_METHOD_CALL = 3 << ZEND_AST_NUM_CHILDREN_SHIFT,
	// 调用允Null对象的方法 $a?->fff()
	ZEND_AST_NULLSAFE_METHOD_CALL,
	// 调用类的静态方法
	ZEND_AST_STATIC_CALL,
	// 只是三元操作符
	ZEND_AST_CONDITIONAL,
	// try 语句
	ZEND_AST_TRY,
	// 单个catch 语句
	ZEND_AST_CATCH,
	// 类里，一条完整的属性声明语句
	ZEND_AST_PROP_GROUP,
	// 类里每个属性
	ZEND_AST_PROP_ELEM,
	// 定义每一个常量（一条语句里可以定义多个）
	ZEND_AST_CONST_ELEM,

	// Pseudo node for initializing enums
	// compile 时用到
	ZEND_AST_CONST_ENUM_INIT,

// 4 个子节点
	/* 4 child nodes */
	// 完整的for(;;){} 代码段
	ZEND_AST_FOR = 4 << ZEND_AST_NUM_CHILDREN_SHIFT,
	// 完整的foreach(as){} 代码段
	ZEND_AST_FOREACH,
	// enum里一条完整的 case语句
	ZEND_AST_ENUM_CASE,

// 5 个子节点
	/* 5 child nodes */
	// 参数列表中的单个参数，有很多修饰符， 2处
	ZEND_AST_PARAM = 5 << ZEND_AST_NUM_CHILDREN_SHIFT,
};

typedef uint16_t zend_ast_kind;
typedef uint16_t zend_ast_attr;

// 语句分3种（普通语句 zend_ast，列表型语句 zend_ast_list，声明型语句 zend_ast_decl）
// 扩展到 _zend_ast_znode，结构的前三个成员相同，可以通用，第四个不一样
// 普通语句，
struct _zend_ast {
	// 语句类型,16位整数  // typedef uint16_t zend_ast_kind;
	zend_ast_kind kind; /* Type of the node (ZEND_AST_* enum constant) */
	// 附加属性,16位整数 // typedef uint16_t zend_ast_attr;
	zend_ast_attr attr; /* Additional attribute, use depending on node type */
	// 行号
	uint32_t lineno;    /* Line number */
	// 前面一共8个字节, _zend_ast_ref 是两个32位整数，也是占8个字节，所以能兼容
	
	// 子元素，也是语句
	zend_ast *child[1]; /* Array of children (using struct hack) */
};

// 语句集合，比 zend_ast 增加了一个动态更新的children count
/* Same as zend_ast, but with children count, which is updated dynamically */
typedef struct _zend_ast_list {
	zend_ast_kind kind;
	zend_ast_attr attr;
	uint32_t lineno;
	// 最主要的，增加了子语句数量
	uint32_t children;
	zend_ast *child[1];
} zend_ast_list;

// 行号在 val变量里, 给变量添加了kind和类型，变成ast_zval
/* Lineno is stored in val.u2.lineno */
typedef struct _zend_ast_zval {
	zend_ast_kind kind;
	zend_ast_attr attr;
	zval val;
} zend_ast_zval;

// 声明型语句，当需要额外信息时，把function 和class的定义区分开，
// class 应该包含了 interface和enum?
/* Separate structure for function and class declaration, as they need extra information. */
typedef struct _zend_ast_decl {
	zend_ast_kind kind;
	// 没用到，只是为了结构兼容
	zend_ast_attr attr; /* Unused - for structure compatibility */
	// 起止行号
	uint32_t start_lineno;
	uint32_t end_lineno;
	// ?
	uint32_t flags;
	// 
	unsigned char *lex_pos;
	// 注释
	zend_string *doc_comment;
	// 类或function 的name
	zend_string *name;
	// 5个子元素
	zend_ast *child[5];
} zend_ast_decl;

typedef void (*zend_ast_process_t)(zend_ast *ast);
extern ZEND_API zend_ast_process_t zend_ast_process;

ZEND_API zend_ast * ZEND_FASTCALL zend_ast_create_zval_with_lineno(zval *zv, uint32_t lineno);
ZEND_API zend_ast * ZEND_FASTCALL zend_ast_create_zval_ex(zval *zv, zend_ast_attr attr);
ZEND_API zend_ast * ZEND_FASTCALL zend_ast_create_zval(zval *zv);
ZEND_API zend_ast * ZEND_FASTCALL zend_ast_create_zval_from_str(zend_string *str);
ZEND_API zend_ast * ZEND_FASTCALL zend_ast_create_zval_from_long(zend_long lval);

ZEND_API zend_ast * ZEND_FASTCALL zend_ast_create_constant(zend_string *name, zend_ast_attr attr);
ZEND_API zend_ast * ZEND_FASTCALL zend_ast_create_class_const_or_name(zend_ast *class_name, zend_ast *name);

#if ZEND_AST_SPEC
# define ZEND_AST_SPEC_CALL(name, ...) \
	ZEND_EXPAND_VA(ZEND_AST_SPEC_CALL_(name, __VA_ARGS__, _5, _4, _3, _2, _1, _0)(__VA_ARGS__))
# define ZEND_AST_SPEC_CALL_(name, _, _5, _4, _3, _2, _1, suffix, ...) \
	name ## suffix
# define ZEND_AST_SPEC_CALL_EX(name, ...) \
	ZEND_EXPAND_VA(ZEND_AST_SPEC_CALL_EX_(name, __VA_ARGS__, _5, _4, _3, _2, _1, _0)(__VA_ARGS__))
# define ZEND_AST_SPEC_CALL_EX_(name, _, _6, _5, _4, _3, _2, _1, suffix, ...) \
	name ## suffix

ZEND_API zend_ast * ZEND_FASTCALL zend_ast_create_0(zend_ast_kind kind);
ZEND_API zend_ast * ZEND_FASTCALL zend_ast_create_1(zend_ast_kind kind, zend_ast *child);
ZEND_API zend_ast * ZEND_FASTCALL zend_ast_create_2(zend_ast_kind kind, zend_ast *child1, zend_ast *child2);
ZEND_API zend_ast * ZEND_FASTCALL zend_ast_create_3(zend_ast_kind kind, zend_ast *child1, zend_ast *child2, zend_ast *child3);
ZEND_API zend_ast * ZEND_FASTCALL zend_ast_create_4(zend_ast_kind kind, zend_ast *child1, zend_ast *child2, zend_ast *child3, zend_ast *child4);
ZEND_API zend_ast * ZEND_FASTCALL zend_ast_create_5(zend_ast_kind kind, zend_ast *child1, zend_ast *child2, zend_ast *child3, zend_ast *child4, zend_ast *child5);

// clear, 创建n个子元素的语句，并设置附加属性
static zend_always_inline zend_ast * zend_ast_create_ex_0(zend_ast_kind kind, zend_ast_attr attr) {
	zend_ast *ast = zend_ast_create_0(kind);
	ast->attr = attr;
	return ast;
}
// clear,
static zend_always_inline zend_ast * zend_ast_create_ex_1(zend_ast_kind kind, zend_ast_attr attr, zend_ast *child) {
	zend_ast *ast = zend_ast_create_1(kind, child);
	ast->attr = attr;
	return ast;
}
// clear,
static zend_always_inline zend_ast * zend_ast_create_ex_2(zend_ast_kind kind, zend_ast_attr attr, zend_ast *child1, zend_ast *child2) {
	zend_ast *ast = zend_ast_create_2(kind, child1, child2);
	ast->attr = attr;
	return ast;
}
// clear,
static zend_always_inline zend_ast * zend_ast_create_ex_3(zend_ast_kind kind, zend_ast_attr attr, zend_ast *child1, zend_ast *child2, zend_ast *child3) {
	zend_ast *ast = zend_ast_create_3(kind, child1, child2, child3);
	ast->attr = attr;
	return ast;
}
// clear,
static zend_always_inline zend_ast * zend_ast_create_ex_4(zend_ast_kind kind, zend_ast_attr attr, zend_ast *child1, zend_ast *child2, zend_ast *child3, zend_ast *child4) {
	zend_ast *ast = zend_ast_create_4(kind, child1, child2, child3, child4);
	ast->attr = attr;
	return ast;
}
// clear,
static zend_always_inline zend_ast * zend_ast_create_ex_5(zend_ast_kind kind, zend_ast_attr attr, zend_ast *child1, zend_ast *child2, zend_ast *child3, zend_ast *child4, zend_ast *child5) {
	zend_ast *ast = zend_ast_create_5(kind, child1, child2, child3, child4, child5);
	ast->attr = attr;
	return ast;
}

ZEND_API zend_ast * ZEND_FASTCALL zend_ast_create_list_0(zend_ast_kind kind);
ZEND_API zend_ast * ZEND_FASTCALL zend_ast_create_list_1(zend_ast_kind kind, zend_ast *child);
ZEND_API zend_ast * ZEND_FASTCALL zend_ast_create_list_2(zend_ast_kind kind, zend_ast *child1, zend_ast *child2);

# define zend_ast_create(...) \
	ZEND_AST_SPEC_CALL(zend_ast_create, __VA_ARGS__)
	
// 默认调用这个方法，调用 zend_ast_create_ex_(n)
# define zend_ast_create_ex(...) \
	ZEND_AST_SPEC_CALL_EX(zend_ast_create_ex, __VA_ARGS__)
# define zend_ast_create_list(init_children, ...) \
	ZEND_AST_SPEC_CALL(zend_ast_create_list, __VA_ARGS__)

// 默认不走这里
#else
ZEND_API zend_ast *zend_ast_create(zend_ast_kind kind, ...);
ZEND_API zend_ast *zend_ast_create_ex(zend_ast_kind kind, zend_ast_attr attr, ...);
ZEND_API zend_ast *zend_ast_create_list(uint32_t init_children, zend_ast_kind kind, ...);
#endif

ZEND_API zend_ast * ZEND_FASTCALL zend_ast_list_add(zend_ast *list, zend_ast *op);

ZEND_API zend_ast *zend_ast_create_decl(
	zend_ast_kind kind, uint32_t flags, uint32_t start_lineno, zend_string *doc_comment,
	zend_string *name, zend_ast *child0, zend_ast *child1, zend_ast *child2, zend_ast *child3, zend_ast *child4
);

ZEND_API zend_result ZEND_FASTCALL zend_ast_evaluate(zval *result, zend_ast *ast, zend_class_entry *scope);
ZEND_API zend_string *zend_ast_export(const char *prefix, zend_ast *ast, const char *suffix);

ZEND_API zend_ast_ref * ZEND_FASTCALL zend_ast_copy(zend_ast *ast);
ZEND_API void ZEND_FASTCALL zend_ast_destroy(zend_ast *ast);
ZEND_API void ZEND_FASTCALL zend_ast_ref_destroy(zend_ast_ref *ast);

typedef void (*zend_ast_apply_func)(zend_ast **ast_ptr, void *context);
ZEND_API void zend_ast_apply(zend_ast *ast, zend_ast_apply_func fn, void *context);


// 主要内部调用，取得一个语句的总内存大小，因为结构体默认有一个子元素，所以先减掉一个指针
// 创建子语句时用到
static zend_always_inline size_t zend_ast_size(uint32_t children) {
	return sizeof(zend_ast) - sizeof(zend_ast *) + sizeof(zend_ast *) * children;
}

// 检查类型是否是 ZEND_AST_ZVAL或其他相关元素，编译调用一次
static zend_always_inline bool zend_ast_is_special(zend_ast *ast) {
	return (ast->kind >> ZEND_AST_SPECIAL_SHIFT) & 1;
}

// 检查 zend_ast.type 是否是 ZEND_AST_ARG_LIST 类型，内部，编译一次调用
// 并非所有的zend_ast都算zend_ast_list
static zend_always_inline bool zend_ast_is_list(zend_ast *ast) {
	return (ast->kind >> ZEND_AST_IS_LIST_SHIFT) & 1;
}

// 本身 type 必须是列表类型，语句强转成列表，内部和编译调用
static zend_always_inline zend_ast_list *zend_ast_get_list(zend_ast *ast) {
	ZEND_ASSERT(zend_ast_is_list(ast));
	return (zend_ast_list *) ast;
}

// ing3, 从语句中取得计算好的内置变量
// 类型必须是内置变量，强转成zend_ast_zval结构，后，获取val属性，内部和编译调用
static zend_always_inline zval *zend_ast_get_zval(zend_ast *ast) {
	ZEND_ASSERT(ast->kind == ZEND_AST_ZVAL);
	return &((zend_ast_zval *) ast)->val;
}

// 获取string值？，解析，编译，执行，都调用
static zend_always_inline zend_string *zend_ast_get_str(zend_ast *ast) {
	zval *zv = zend_ast_get_zval(ast);
	ZEND_ASSERT(Z_TYPE_P(zv) == IS_STRING);
	return Z_STR_P(zv);
}

// clear, 取得常量名，少量外部调用
static zend_always_inline zend_string *zend_ast_get_constant_name(zend_ast *ast) {
	// 语句类型必须是常量
	ZEND_ASSERT(ast->kind == ZEND_AST_CONSTANT);
	// 下属变量类型必须是 string
	ZEND_ASSERT(Z_TYPE(((zend_ast_zval *) ast)->val) == IS_STRING);
	// 把下属变量的值转成 string 并返回
	return Z_STR(((zend_ast_zval *) ast)->val);
}

// clear, 取得子节点数量，(定义是数量 << ZEND_AST_NUM_CHILDREN_SHIFT), 内部和编译调用
static zend_always_inline uint32_t zend_ast_get_num_children(zend_ast *ast) {
	ZEND_ASSERT(!zend_ast_is_list(ast));
	return ast->kind >> ZEND_AST_NUM_CHILDREN_SHIFT;
}

// clear, 获取语句行号, 内部和编译调用
static zend_always_inline uint32_t zend_ast_get_lineno(zend_ast *ast) {
	// 如果是zval，获取zval的行号
	if (ast->kind == ZEND_AST_ZVAL) {
		zval *zv = zend_ast_get_zval(ast);
		return Z_LINENO_P(zv);
	} else {
		return ast->lineno;
	}
}

// clear， 二进制操作： + - * 、 . 等 ，parser调用
static zend_always_inline zend_ast *zend_ast_create_binary_op(uint32_t opcode, zend_ast *op0, zend_ast *op1) {
	return zend_ast_create_ex(ZEND_AST_BINARY_OP, opcode, op0, op1);
}

zend_ast *zend_ast_create_concat_op(zend_ast *op0, zend_ast *op1);

//clear, 二元赋值操作， parser 用的
static zend_always_inline zend_ast *zend_ast_create_assign_op(uint32_t opcode, zend_ast *op0, zend_ast *op1) {
	return zend_ast_create_ex(ZEND_AST_ASSIGN_OP, opcode, op0, op1);
}

//clear, 强转类型语句(int),(float)等， parser 调用
static zend_always_inline zend_ast *zend_ast_create_cast(uint32_t type, zend_ast *op0) {
	return zend_ast_create_ex(ZEND_AST_CAST, type, op0);
}

//clear, zend_ast_list 清除最后一个空子元素， parser 调用
static zend_always_inline zend_ast *zend_ast_list_rtrim(zend_ast *ast) {
	zend_ast_list *list = zend_ast_get_list(ast);
	if (list->children && list->child[list->children - 1] == NULL) {
		list->children--;
	}
	return ast;
}

zend_ast * ZEND_FASTCALL zend_ast_with_attributes(zend_ast *ast, zend_ast *attr);

#endif
