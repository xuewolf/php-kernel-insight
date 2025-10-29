// 共有 295 个 static 开头的语句，static void zend_compile 开头的有101次

// #define FC(member) (CG(file_context).member)
	// file_context 要通过 op_array 共享

// lcname: low char name ,转成小写的函数名或类名

// opcode 都是一行一行的，而且只有简单的几个指令， zend_ast 是树形结构而且关联性强。所以compile看似复杂，实际上并不比语法检查复杂多少，只是要校验的规则比较多，涉及到数据类型比较繁琐。

// 编译时被视为特殊的函数
		strlen,is_null,is_bool,is_long,is_int,is_integer,is_float,is_double,is_string,
		is_array,is_object,is_resource,is_scalar,boolval,intval,floatval,doubleval,
		strval,defined,chr,ord,call_user_func_array,call_user_func,in_array,count,sizeof,
		get_class,get_called_class,gettype,func_num_args,func_get_args,array_slice,array_key_exists

// 全局变量 FC(in_namespace) ：
	// 只有内部用到
	// zend_end_namespace，zend_file_context_begin 两个地方赋值 0
	// zend_compile_namespace 一个地方赋值 1
	
// 全局变量 FC(has_bracketed_namespaces) ：
	// 只有内部用到
	// zend_file_context_begin 赋值 0
	// zend_compile_namespace 赋值 1
	
// FC(current_namespace)
	// 只有内部用到
	// zend_file_context_begin, zend_end_namespace 赋值 NULL
	// zend_compile_namespace :	FC(current_namespace) = zend_string_copy(name); 或 null

// 以上三个变量的逻辑关系紧密
	
// CG(loop_var_stack)
	
// CG(active_class_entry), 类入口外部少量调用
	// zend_init_compiler_data_structures 赋值 null，
	// zend_compile_func_decl 赋值 null, orig_class_entry
	// zend_compile_class_decl 赋值 ce, original_ce
		// 主要是这个
		
// FC(imports)，FC(imports_function)，FC(imports_const) ，HashTable，导入列表, 内部引用
	// 
	
// lexical variable 专指 匿名function use () 里面的变量

// parameter 指形式参数， argument 指实际参数	

// 从 zend_compile_top_stmt 开始
	-> switch(ast->kind) 
	ZEND_AST_STMT_LIST -> zend_compile_top_stmt
	ZEND_AST_FUNC_DECL -> zend_compile_func_decl
	ZEND_AST_CLASS -> zend_compile_class_decl
	其他 -> zend_compile_stmt
	
# zend_compile_stmt 20 次调用
	-> switch(ast->kind) 
	ZEND_AST_STMT_LIST -> zend_compile_stmt_list(ast);
	ZEND_AST_GLOBAL -> zend_compile_global_var(ast);
	ZEND_AST_STATIC -> zend_compile_static_var(ast);
	ZEND_AST_UNSET -> zend_compile_unset(ast);
	ZEND_AST_RETURN -> zend_compile_return(ast);
	ZEND_AST_ECHO -> zend_compile_echo(ast);
	ZEND_AST_BREAK:
	ZEND_AST_CONTINUE -> zend_compile_break_continue(ast);
	ZEND_AST_GOTO -> zend_compile_goto(ast);
	ZEND_AST_LABEL -> zend_compile_label(ast);
	ZEND_AST_WHILE -> zend_compile_while(ast);
	ZEND_AST_DO_WHILE -> zend_compile_do_while(ast);
	ZEND_AST_FOR -> zend_compile_for(ast);
	ZEND_AST_FOREACH -> zend_compile_foreach(ast);
	ZEND_AST_IF -> zend_compile_if(ast);
	ZEND_AST_SWITCH -> zend_compile_switch(ast);
	ZEND_AST_TRY -> zend_compile_try(ast);
	ZEND_AST_DECLARE -> zend_compile_declare(ast);
	ZEND_AST_FUNC_DECL:
	ZEND_AST_METHOD -> zend_compile_func_decl(NULL, ast, 0);
	ZEND_AST_ENUM_CASE -> zend_compile_enum_case(ast);
	ZEND_AST_PROP_GROUP -> zend_compile_prop_group(ast);
	ZEND_AST_CLASS_CONST_GROUP -> zend_compile_class_const_group(ast);
	ZEND_AST_USE_TRAIT -> zend_compile_use_trait(ast);
	ZEND_AST_CLASS -> zend_compile_class_decl(NULL, ast, 0);
	ZEND_AST_GROUP_USE -> zend_compile_group_use(ast);
	ZEND_AST_USE -> zend_compile_use(ast);
	ZEND_AST_CONST_DECL -> zend_compile_const_decl(ast);
	ZEND_AST_NAMESPACE -> zend_compile_namespace(ast);
	ZEND_AST_HALT_COMPILER -> zend_compile_halt_compiler(ast);
	ZEND_AST_THROW:
	ZEND_AST_EXIT -> zend_compile_expr(NULL, ast);
	default -> zend_compile_expr(&result, ast);
	

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
   |          Nikita Popov <nikic@php.net>                                |
   +----------------------------------------------------------------------+
*/

#include <zend_language_parser.h>
#include "zend.h"
#include "zend_attributes.h"
#include "zend_compile.h"
#include "zend_constants.h"
#include "zend_llist.h"
#include "zend_API.h"
#include "zend_exceptions.h"
#include "zend_interfaces.h"
#include "zend_virtual_cwd.h"
#include "zend_multibyte.h"
#include "zend_language_scanner.h"
#include "zend_inheritance.h"
#include "zend_vm.h"
#include "zend_enum.h"
#include "zend_observer.h"

// target = (src)->u.op;

// 30多次内部调用
// ing2, 把znode的值 传给 znode_op，如果是常量，复制u.constant对象 否则复制 znode_op 对象，不是指针
// target 是opline的子元素 znode_op, src是znode的指针
#define SET_NODE(target, src) do { \
		target ## _type = (src)->op_type; \
		/* 如果src的类型是常量 */ \
		if ((src)->op_type == IS_CONST) { \
			target.constant = zend_add_literal(&(src)->u.constant); \
		} else { \
			target = (src)->u.op; \
		} \
	} while (0)

// 50次调用
// ing2, 
// target 是znode的指针, src是opline的子元素 znode_op
#define GET_NODE(target, src) do { \
		(target)->op_type = src ## _type; \
		if ((target)->op_type == IS_CONST) { \
			ZVAL_COPY_VALUE(&(target)->u.constant, CT_CONSTANT(src)); \
		} else { \
			(target)->u.op = src; \
		} \
	} while (0)

// ing4, 获取compile global ( file_context ) 里面的 成员变量，外部调用特别多
#define FC(member) (CG(file_context).member)

// 循环变量，用在循环堆栈里
typedef struct _zend_loop_var {
	// 操作码名
	zend_uchar opcode;
	// 变量类型
	zend_uchar var_type;
	// 变量数量
	uint32_t   var_num;
	// ？
	uint32_t   try_catch_offset;
} zend_loop_var;

// ing3, 给 CG(active_op_array) 添加缓存大小，添加 count 个void*指针的大小, 返回缓存大小
static inline uint32_t zend_alloc_cache_slots(unsigned count) {
	//
	if (count == 0) {
		// 即使不需要cache slots, VM handler 仍然会无条件 调用 CACHE_ADDR().
		// 返回0可以保证地址计算一直合法，ubsan 不会冲突
		/* Even if no cache slots are desired, the VM handler may still want to acquire
		 * CACHE_ADDR() unconditionally. Returning zero makes sure that the address
		 * calculation is still legal and ubsan does not complain. */
		return 0;
	}

	// 
	zend_op_array *op_array = CG(active_op_array);
	// 缓存大小
	uint32_t ret = op_array->cache_size;
	// 添加 count 个指针的大小。（void* 无类型指针，指针其实有无类型大小都一致）
	op_array->cache_size += count * sizeof(void*);
	return ret;
}

// ing3，给 CG(active_op_array) 添加缓存大小，添加 一个 void*指针的大小，返回添加后的缓存大小
static inline uint32_t zend_alloc_cache_slot(void) {
	return zend_alloc_cache_slots(1);
}

ZEND_API zend_op_array *(*zend_compile_file)(zend_file_handle *file_handle, int type);
ZEND_API zend_op_array *(*zend_compile_string)(zend_string *source_string, const char *filename, zend_compile_position position);

#ifndef ZTS
ZEND_API zend_compiler_globals compiler_globals;
ZEND_API zend_executor_globals executor_globals;
#endif

static zend_op *zend_emit_op(znode *result, zend_uchar opcode, znode *op1, znode *op2);
static bool zend_try_ct_eval_array(zval *result, zend_ast *ast);
static void zend_eval_const_expr(zend_ast **ast_ptr);

static zend_op *zend_compile_var(znode *result, zend_ast *ast, uint32_t type, bool by_ref);
static zend_op *zend_delayed_compile_var(znode *result, zend_ast *ast, uint32_t type, bool by_ref);
static void zend_compile_expr(znode *result, zend_ast *ast);
static void zend_compile_stmt(zend_ast *ast);
static void zend_compile_assign(znode *result, zend_ast *ast);

// ing3, 初始化一个操作码
static void init_op(zend_op *op)
{
	// 这个也是初始化
	MAKE_NOP(op);
	op->extended_value = 0;
	// 设置行号
	op->lineno = CG(zend_lineno);
}

// ing4, 返回最新操作码行号：前上下文中的操作码数量, 大量引用
static zend_always_inline uint32_t get_next_op_number(void)
{
	return CG(active_op_array)->last;
}

// ing3, 取得新操作码对象，这是个工场，只有内部调用
static zend_op *get_next_op(void)
{
	// 当前作用域
	zend_op_array *op_array = CG(active_op_array);
	// 新操作码的编号（从1开始？）
	uint32_t next_op_num = op_array->last++;
	// 新操作码
	zend_op *next_op;

	// 如果操作码编号大于 上下文中的操作码数量
	if (UNEXPECTED(next_op_num >= CG(context).opcodes_size)) {
		CG(context).opcodes_size *= 4;
		// 重新划分内存！这时候新建了操作码
		op_array->opcodes = erealloc(op_array->opcodes, CG(context).opcodes_size * sizeof(zend_op));
	}

	// 获取新操作码指针
	next_op = &(op_array->opcodes[next_op_num]);

	// 初始化新操作码（就算没重新划分内存，这里也要重新初始化）
	init_op(next_op);

	// 返回新操作码指针
	return next_op;
}

// ing3, 创建并返回空的 zend_brk_cont_element 实例
static zend_brk_cont_element *get_next_brk_cont_element(void)
{
	// 增加上下文中 break,continue 语句的数量 。这是唯一一个会让计数增加的地方
	CG(context).last_brk_cont++;
	// 重新分配 zend_brk_cont_element 列表的内存大小，不存在会自动创建
	CG(context).brk_cont_array = erealloc(CG(context).brk_cont_array, sizeof(zend_brk_cont_element) * CG(context).last_brk_cont);
	// 返回刚刚创建的 zend_brk_cont_element
	return &CG(context).brk_cont_array[CG(context).last_brk_cont-1];
}

// ing2 ,取得程序运行时定义的类名，PRIu32，PRIx32 是什么
static zend_string *zend_build_runtime_definition_key(zend_string *name, uint32_t start_lineno) /* {{{ */
{
	// 当前文件名
	zend_string *filename = CG(active_op_array)->filename;
	// 结果为 \0 + 类名 + 文件名 + 起始行号 + CG(rtd_key_counter)++
	zend_string *result = zend_strpprintf(0, "%c%s%s:%" PRIu32 "$%" PRIx32,
		'\0', ZSTR_VAL(name), ZSTR_VAL(filename), start_lineno, CG(rtd_key_counter)++);
	// 新类名 创建保留字串 
	return zend_new_interned_string(result);
}
/* }}} */

// ing2, 取得不带命名空间的名字（类，函数，常量等）
static bool zend_get_unqualified_name(const zend_string *name, const char **result, size_t *result_len) /* {{{ */
{
	// zend_memrchr(const void *s, int c, size_t n)
	const char *ns_separator = zend_memrchr(ZSTR_VAL(name), '\\', ZSTR_LEN(name));
	if (ns_separator != NULL) {
		*result = ns_separator + 1;
		*result_len = ZSTR_VAL(name) + ZSTR_LEN(name) - *result;
		return 1;
	}

	return 0;
}
/* }}} */

// clear, 类名保留字
struct reserved_class_name {
	const char *name;
	size_t len;
};

// #define ZEND_STRL(str) (str), (sizeof(str)-1) 返回本身和长度
// clear, 保留的类名
static const struct reserved_class_name reserved_class_names[] = {
	{ZEND_STRL("bool")},
	{ZEND_STRL("false")},
	{ZEND_STRL("float")},
	{ZEND_STRL("int")},
	{ZEND_STRL("null")},
	{ZEND_STRL("parent")},
	{ZEND_STRL("self")},
	{ZEND_STRL("static")},
	{ZEND_STRL("string")},
	{ZEND_STRL("true")},
	{ZEND_STRL("void")},
	{ZEND_STRL("never")},
	{ZEND_STRL("iterable")},
	{ZEND_STRL("object")},
	{ZEND_STRL("mixed")},
	{NULL, 0}
};

// ing3, 检查类名是否属于保留字
static bool zend_is_reserved_class_name(const zend_string *name) /* {{{ */
{
	// 保留字
	const struct reserved_class_name *reserved = reserved_class_names;

	// 获取字串
	const char *uqname = ZSTR_VAL(name);
	// 字串长度
	size_t uqname_len = ZSTR_LEN(name);
	// 获取不带命名空间的类名
	zend_get_unqualified_name(name, &uqname, &uqname_len);

	for (; reserved->name; ++reserved) {
		if (uqname_len == reserved->len
			// 二进制比对, 相等0 ,大0 ,小-1
			&& zend_binary_strcasecmp(uqname, uqname_len, reserved->name, reserved->len) == 0
		) {
			return 1;
		}
	}

	return 0;
}
/* }}} */

// ing4, 保证类名不属于保留字（测试过）
void zend_assert_valid_class_name(const zend_string *name) /* {{{ */
{
	if (zend_is_reserved_class_name(name)) {
		zend_error_noreturn(E_COMPILE_ERROR,
			"Cannot use '%s' as class name as it is reserved", ZSTR_VAL(name));
	}
}
/* }}} */

// 内置类型信息
typedef struct _builtin_type_info {
	const char* name;
	const size_t name_len;
	const zend_uchar type;
} builtin_type_info;

// ing4, 内置类型列表。20种里的13种。参看 zend_types.h
static const builtin_type_info builtin_types[] = {
	{ZEND_STRL("null"), IS_NULL},
	{ZEND_STRL("true"), IS_TRUE},
	{ZEND_STRL("false"), IS_FALSE},
	{ZEND_STRL("int"), IS_LONG},
	{ZEND_STRL("float"), IS_DOUBLE},
	{ZEND_STRL("string"), IS_STRING},
	{ZEND_STRL("bool"), _IS_BOOL},
	{ZEND_STRL("void"), IS_VOID},
	{ZEND_STRL("never"), IS_NEVER},
	{ZEND_STRL("iterable"), IS_ITERABLE},
	{ZEND_STRL("object"), IS_OBJECT},
	{ZEND_STRL("mixed"), IS_MIXED},
	{NULL, 0, IS_UNDEF}
};

// ing4, 易混淆类型
typedef struct {
	const char *name;
	size_t name_len;
	const char *correct_name;
} confusable_type_info;

// ing4, 5种，resource 类型没有别名。Null 为什么也算呢？
static const confusable_type_info confusable_types[] = {
	{ZEND_STRL("boolean"), "bool"},
	{ZEND_STRL("integer"), "int"},
	{ZEND_STRL("double"), "float"},
	{ZEND_STRL("resource"), NULL},
	{NULL, 0, NULL},
};

// ing4, 用名称查找内置类型，zend_compile_single_typename 一次调用
static zend_always_inline zend_uchar zend_lookup_builtin_type_by_name(const zend_string *name) /* {{{ */
{
	const builtin_type_info *info = &builtin_types[0];
	// 遍历上述13种元素，第13个null 当成哨兵了
	for (; info->name; ++info) {
		// 如果名称长度等于内置类型的名称长度
		if (ZSTR_LEN(name) == info->name_len
			// 并且 忽略大小写比较，名称字串相通
			&& zend_binary_strcasecmp(ZSTR_VAL(name), ZSTR_LEN(name), info->name, info->name_len) == 0
		) {
			// 返回类型（常量，如 IS_NULL）
			return info->type;
		}
	}

	return 0;
}
/* }}} */

// ing3, 是否是易混淆类型， correct_name 引用返回类型名,为什么要两层指针？ 外部引用
static zend_always_inline bool zend_is_confusable_type(const zend_string *name, const char **correct_name) /* {{{ */
{
	// boolean -> bool, integer -> int, double -> float, null -> null
	const confusable_type_info *info = confusable_types;

	// 故意用大小写敏感的比对，因为 integer 像是标量类型， Integer 像类型名
	/* Intentionally using case-sensitive comparison here, because "integer" is likely intended
	 * as a scalar type, while "Integer" is likely a class type. */
	for (; info->name; ++info) {
		if (zend_string_equals_cstr(name, info->name, info->name_len)) {
			*correct_name = info->correct_name;
			return 1;
		}
	}

	return 0;
}
/* }}} */

// ing4, 确认类名没有导入过，zend_is_not_imported 一次调用
static bool zend_is_not_imported(zend_string *name) {
	// 假设类名不是绝对地址
	/* Assuming "name" is unqualified here. */
	// 导入列表为空，或查询返回null 
	return !FC(imports) || zend_hash_find_ptr_lc(FC(imports), name) == NULL;
}

// ing3, 重置 操作码上下文，zend_compile_func_decl一处调用，,/Zend/zend_language_scanner.l 还有一处调用
void zend_oparray_context_begin(zend_oparray_context *prev_context) /* {{{ */
{
	*prev_context = CG(context); // 当前上下文
	CG(context).opcodes_size = INITIAL_OP_ARRAY_SIZE; // 初始操作码数量 64
	CG(context).vars_size = 0;// 变量数
	CG(context).literals_size = 0; // literal 变量数量 
	CG(context).fast_call_var = -1; // finally 语句里用到
	CG(context).try_catch_offset = -1; // try_catch 偏移量，初始无效
	CG(context).current_brk_cont = -1; // current_brk_cont 赋值1/3：初始值为 -1
	CG(context).last_brk_cont = 0; // break continue 数量
	CG(context).brk_cont_array = NULL; // zend_brk_cont_element 实例列表
	CG(context).labels = NULL; // 跳转标签	
}
/* }}} */

// ing4, 操作码上下文结束, 清空break/continue跳转点和goto跳转标签
void zend_oparray_context_end(zend_oparray_context *prev_context) /* {{{ */
{
	// 操作码上下文，如果有break continue列表
	if (CG(context).brk_cont_array) {
		// 释放这个列表
		efree(CG(context).brk_cont_array);
		// 指针置空
		CG(context).brk_cont_array = NULL;
	}
	// 如果有跳转 labels
	if (CG(context).labels) {
		// 销毁label哈希表
		zend_hash_destroy(CG(context).labels);
		// 释放哈希表
		FREE_HASHTABLE(CG(context).labels);
		// 指针置空
		CG(context).labels = NULL;
	}
	// 使用前一个上下文 作为当前 上下文
	CG(context) = *prev_context;
}
/* }}} */

// ing4, 重置导入列表
static void zend_reset_import_tables(void) /* {{{ */
{
	// 如果有导入类名，全部销毁
	if (FC(imports)) {
		zend_hash_destroy(FC(imports));
		efree(FC(imports));
		FC(imports) = NULL;
	}

	// 如果有导入函数名，全部销毁
	if (FC(imports_function)) {
		zend_hash_destroy(FC(imports_function));
		efree(FC(imports_function));
		FC(imports_function) = NULL;
	}

	// 如果有导入常量名，全部销毁
	if (FC(imports_const)) {
		zend_hash_destroy(FC(imports_const));
		efree(FC(imports_const));
		FC(imports_const) = NULL;
	}
}
/* }}} */

// ing3, 编译命名空间结束, 释放当前命名空间
static void zend_end_namespace(void) /* {{{ */ {
	// 清空状态
	FC(in_namespace) = 0;
	// 重置导入列表
	zend_reset_import_tables();
	// 如果有当前命名空间
	if (FC(current_namespace)) {
		// 释放当前命名空间，并标记成Null
		zend_string_release_ex(FC(current_namespace), 0);
		FC(current_namespace) = NULL;
	}
}
/* }}} */

// ing3, 文件上下文开始
void zend_file_context_begin(zend_file_context *prev_context) /* {{{ */
{
	// 关联前一个上下文
	*prev_context = CG(file_context);
	// import 列表，默认 null
	FC(imports) = NULL;
	// import 函数表，默认 null
	FC(imports_function) = NULL;
	// import 常量表，默认 null
	FC(imports_const) = NULL;
	// 当前命名空间，默认 null
	FC(current_namespace) = NULL;
	// 是否在命名空间中，默认否
	FC(in_namespace) = 0;
	// 是否有括号的命名空间，默认否
	FC(has_bracketed_namespaces) = 0;
	// declare(ticks=n) 语句定义的值
	FC(declarables).ticks = 0;
	// 已知符号表
	// #define FC(member) (CG(file_context).member)
	zend_hash_init(&FC(seen_symbols), 8, NULL, NULL, 0);
}
/* }}} */

// ing3, 文件上下文结束 
void zend_file_context_end(zend_file_context *prev_context) /* {{{ */
{
	// 编译命名空间结束, 释放当前命名空间
	zend_end_namespace();
	// 销毁已知符号列表
	zend_hash_destroy(&FC(seen_symbols));
	// 切换回前一个上下文
	CG(file_context) = *prev_context;
}
/* }}} */

// ing2, 初始化编译数据结构体
void zend_init_compiler_data_structures(void) /* {{{ */
{
	// 初始化 循环堆栈
	zend_stack_init(&CG(loop_var_stack), sizeof(zend_loop_var));
	// 初始化 延时操作码堆栈
	zend_stack_init(&CG(delayed_oplines_stack), sizeof(zend_op));
	// 初始化短路堆栈（short_circuited_operation_nums?)
	zend_stack_init(&CG(short_circuiting_opnums), sizeof(uint32_t));
	// 类入口
	CG(active_class_entry) = NULL;
	// ? goto 相关
	CG(in_compilation) = 0;
	// ? 刚进入php程序时设置成1，这是什么
	CG(skip_shebang) = 0;

	// 字符集
	CG(encoding_declared) = 0;
	// 记忆表达式列表
	CG(memoized_exprs) = NULL;
	// 编译记忆模式
	CG(memoize_mode) = 0;
}
/* }}} */

// ing3, 已知符号列表：包含类名，函数名，常量名，因为可能重复，所以添加kind
static void zend_register_seen_symbol(zend_string *name, uint32_t kind) {
	// 哈希表中查找 类名
	zval *zv = zend_hash_find(&FC(seen_symbols), name);
	// 存在，添加 kind
	if (zv) {
		Z_LVAL_P(zv) |= kind;
	// 不存在 ，添加进哈希表
	} else {
		zval tmp;
		ZVAL_LONG(&tmp, kind);
		zend_hash_add_new(&FC(seen_symbols), name, &tmp);
	}
}

// ing3, 检查是否存在于已知符号列表，已知符号列表：包含类名，函数名，常量名，因为可能重复，所以添加kind
static bool zend_have_seen_symbol(zend_string *name, uint32_t kind) {
	zval *zv = zend_hash_find(&FC(seen_symbols), name);
	return zv && (Z_LVAL_P(zv) & kind) != 0;
}

// ing3, 初始化编译器，这里是入口
void init_compiler(void) /* {{{ */
{
	// 创建arena
	CG(arena) = zend_arena_create(64 * 1024);
	// 关键变量，两个地方给它赋值，zend_resolve_goto_label() 和 zend_compile_func_decl()
	// 可见 导致作用域切换的最主要的逻辑是 声明goto标签 和声明 function
	CG(active_op_array) = NULL;
	// 初始化上下文
	memset(&CG(context), 0, sizeof(CG(context)));
	// 初始化编译数据结构体
	zend_init_compiler_data_structures();
	// 初始化资源列表， /Zend/zend_list.c 中定义，只有这里用到
	zend_init_rsrc_list();
	// 初始化文件流工具， /Zend/zend_stream.c 中定义，只有这里用到
	zend_stream_init();
	// 未清理的关闭，默认不允许 ？
	CG(unclean_shutdown) = 0;

	// 这几个东西主要用在类继承
	CG(delayed_variance_obligations) = NULL;
	// 延时自动加载
	CG(delayed_autoloads) = NULL;
	// 未链接的use
	CG(unlinked_uses) = NULL;
	// 当前正在链接的类
	CG(current_linking_class) = NULL;
}
/* }}} */

// ing3, 关闭编译器
void shutdown_compiler(void) /* {{{ */
{
	// 销毁 arena 前先重置 filename, 因为文件缓存会使用 arena 分配字串
	/* Reset filename before destroying the arena, as file cache may use arena allocated strings. */
	// 清空当前文件名 CG(compiled_filename)
	zend_restore_compiled_filename(NULL);
	// 销毁循环堆栈
	zend_stack_destroy(&CG(loop_var_stack));
	// 销毁延时操作码堆栈
	zend_stack_destroy(&CG(delayed_oplines_stack));
	// 销毁短路堆栈
	zend_stack_destroy(&CG(short_circuiting_opnums));

	// 销毁哈希表 CG(delayed_variance_obligations) ，用于类继承，不是编译阶段用的
	if (CG(delayed_variance_obligations)) {
		zend_hash_destroy(CG(delayed_variance_obligations));
		FREE_HASHTABLE(CG(delayed_variance_obligations));
		CG(delayed_variance_obligations) = NULL;
	}
	// 销毁哈希表 CG(delayed_autoloads) ，用于类继承，不是编译阶段用的
	if (CG(delayed_autoloads)) {
		zend_hash_destroy(CG(delayed_autoloads));
		FREE_HASHTABLE(CG(delayed_autoloads));
		CG(delayed_autoloads) = NULL;
	}
	// 销毁哈希表 CG(unlinked_uses) ，用于类继承和其他地方，不是编译阶段用的
	if (CG(unlinked_uses)) {
		zend_hash_destroy(CG(unlinked_uses));
		FREE_HASHTABLE(CG(unlinked_uses));
		CG(unlinked_uses) = NULL;
	}
	// 用于类继承和其他地方，不是编译阶段用的
	CG(current_linking_class) = NULL;
}
/* }}} */

// clear, 设置正在编译的文件名
ZEND_API zend_string *zend_set_compiled_filename(zend_string *new_compiled_filename) /* {{{ */
{
	CG(compiled_filename) = zend_string_copy(new_compiled_filename);
	return new_compiled_filename;
}
/* }}} */

// ing4, 重新保存编译文件名
ZEND_API void zend_restore_compiled_filename(zend_string *original_compiled_filename) /* {{{ */
{
	// 先销毁原来的文件名
	if (CG(compiled_filename)) {
		zend_string_release(CG(compiled_filename));
		CG(compiled_filename) = NULL;
	}
	// 设置新文件名
	CG(compiled_filename) = original_compiled_filename;
}
/* }}} */

// ing3,返回正在编译的文件名
ZEND_API zend_string *zend_get_compiled_filename(void) /* {{{ */
{
	return CG(compiled_filename);
}
/* }}} */

// ing3,返回正在编译的行号
ZEND_API int zend_get_compiled_lineno(void) /* {{{ */
{
	return CG(zend_lineno);
}
/* }}} */

// ing4, 是否正在编译
ZEND_API bool zend_is_compiling(void) /* {{{ */
{
	return CG(in_compilation);
}
/* }}} */

// ing4, 增加临时变量计数， IS_VAR 和 IS_TMP_VAR 都算临时变量，并返回数量，内部引用
static zend_always_inline uint32_t get_temporary_variable(void) /* {{{ */
{
	return (uint32_t)CG(active_op_array)->T++;
}
/* }}} */

// ing3, 查找【编译变量】，如果没有的话创建一个新的并返回序号
static int lookup_cv(zend_string *name) /* {{{ */{
	// 当前操作码列表
	zend_op_array *op_array = CG(active_op_array);
	// 变量序号 0
	int i = 0;
	// 取得name的哈希值，long型
	zend_ulong hash_value = zend_string_hash_val(name);
	// 遍历 op_array 中的 CV
	while (i < op_array->last_var) {
		// 如果当前变量名的哈希值和要查找的哈希值相同， 并且当前变量名和要查找的变量名相同，说明找到了
		if (ZSTR_H(op_array->vars[i]) == hash_value
		 && zend_string_equals(op_array->vars[i], name)) {
			// #define EX_NUM_TO_VAR(n) ((uint32_t)(((n) + ZEND_CALL_FRAME_SLOT) * sizeof(zval)))
			// 返回变量序号
			return EX_NUM_TO_VAR(i);
		}
		i++;
	}
	// 变量列表中的序号 = 当前变量数
	i = op_array->last_var;
	// 向右移1个
	op_array->last_var++;
	// 如果指针超出范围
	if (op_array->last_var > CG(context).vars_size) {
		// 再多分配16个变量的空间
		CG(context).vars_size += 16; /* FIXME */
		// 分配内存
		op_array->vars = erealloc(op_array->vars, CG(context).vars_size * sizeof(zend_string*));
	}
	// 更新CV name
	op_array->vars[i] = zend_string_copy(name);
	// 返回在变量列表中的序号
	return EX_NUM_TO_VAR(i);
}
/* }}} */

// ing2, 创建保留字，保留字是干嘛用的呢？
zend_string *zval_make_interned_string(zval *zv)
{
	// 必须是string
	ZEND_ASSERT(Z_TYPE_P(zv) == IS_STRING);
	// zend_new_interned_string 在 zend_string.c里，比较复杂
	Z_STR_P(zv) = zend_new_interned_string(Z_STR_P(zv));
	// 如果已经有 IS_STR_INTERNED 标记， #define ZSTR_IS_INTERNED(s)	 (GC_FLAGS(s) & IS_STR_INTERNED)
	if (ZSTR_IS_INTERNED(Z_STR_P(zv))) {
		// TYPE_FLAGS 标记成0
		Z_TYPE_FLAGS_P(zv) = 0;
	}
	return Z_STR_P(zv);
}

// zend_add_literal 和 zend_append_individual_literal（不存在） 公用
/* Common part of zend_add_literal and zend_append_individual_literal */
// ing2, 插入 给操作码添加 literal
static inline void zend_insert_literal(zend_op_array *op_array, zval *zv, int literal_position) /* {{{ */
{
	// define CT_CONSTANT_EX(op_array, num) ((op_array)->literals + (num))
	// literals指针向右移 literal_position 个zval
	zval *lit = CT_CONSTANT_EX(op_array, literal_position);
	// 如果是string 类型
	if (Z_TYPE_P(zv) == IS_STRING) {
		// 用cv创建保留字
		zval_make_interned_string(zv);
	}
	// 赋值 literals 中的元素赋值
	ZVAL_COPY_VALUE(lit, zv);
	// (zval).u2.extra 设置成0
	Z_EXTRA_P(lit) = 0;
}
/* }}} */

// 编译函数时调用，用上下文保存大概尺寸的追踪，避免频繁 重新检索
// 第二次编译pass_two()时，会把Literals截短到正确的尺寸。
/* Is used while compiling a function, using the context to keep track
   of an approximate size to avoid to relocate to often.
   Literals are truncated to actual size in the second compiler pass (pass_two()). */
// ing3， 添加 literal ，是在当前 CG(active_op_array)->literals 中添加一个 zval
static int zend_add_literal(zval *zv) /* {{{ */
{
	zend_op_array *op_array = CG(active_op_array);
	// literal数量
	int i = op_array->last_literal;
	// 数量 +1 
	op_array->last_literal++;
	if (i >= CG(context).literals_size) {
		// 每次增加16字节，直到大于 i
		while (i >= CG(context).literals_size) {
			// 
			CG(context).literals_size += 16; /* FIXME */
		}
		// 重新分配内存，并返回地址
		op_array->literals = (zval*)erealloc(op_array->literals, CG(context).literals_size * sizeof(zval));
	}
	// 给op_array 添 加literal
	zend_insert_literal(op_array, zv, i);
	// 返回添加的 literal 序号
	return i;
}
/* }}} */

// ing3, 给CG(active_op_array)添加 string类型的 literal
static inline int zend_add_literal_string(zend_string **str) /* {{{ */
{
	int ret;
	zval zv;
	// 复制zval 的string 部分
	ZVAL_STR(&zv, *str);
	// 添加 literal ，是在当前 CG(active_op_array)->literals 中添加一个 zval
	ret = zend_add_literal(&zv);
	*str = Z_STR(zv);
	return ret;
}
/* }}} */

// ing3, 把方法名添加到 当前 CG(active_op_array)-> literals 里面, 添加一个原名称一个小写名称
static int zend_add_func_name_literal(zend_string *name) /* {{{ */
{
	// 原始名
	/* Original name */
	int ret = zend_add_literal_string(&name);

	// 小写名
	/* Lowercased name */
	zend_string *lc_name = zend_string_tolower(name);
	zend_add_literal_string(&lc_name);

	return ret;
}
/* }}} */

// ing3, 把namespace名添加到 当前 CG(active_op_array)-> literals 里面,
static int zend_add_ns_func_name_literal(zend_string *name) /* {{{ */
{
	const char *unqualified_name;
	size_t unqualified_name_len;

	// 1. 先添加原始名称
	/* Original name */
	int ret = zend_add_literal_string(&name);

	// 2. 小写名
	/* Lowercased name */
	zend_string *lc_name = zend_string_tolower(name);
	zend_add_literal_string(&lc_name);

	// 3. 取得不带命名空间的名字，转成小写后，也添加到 literals 里面
	/* Lowercased unqualified name */
	if (zend_get_unqualified_name(name, &unqualified_name, &unqualified_name_len)) {
		// 取得最后一段
		lc_name = zend_string_alloc(unqualified_name_len, 0);
		// 转小写
		zend_str_tolower_copy(ZSTR_VAL(lc_name), unqualified_name, unqualified_name_len);
		zend_add_literal_string(&lc_name);
	}

	return ret;
}
/* }}} */

// ing3, 把类名添加到 当前 CG(active_op_array)-> literals 里面,
static int zend_add_class_name_literal(zend_string *name) /* {{{ */
{
	// 原始名称
	/* Original name */
	int ret = zend_add_literal_string(&name);

	// 小写名称
	/* Lowercased name */
	zend_string *lc_name = zend_string_tolower(name);
	zend_add_literal_string(&lc_name);

	return ret;
}
/* }}} */

// ing3, 把常量名添加到 当前 CG(active_op_array)-> literals 里面,
static int zend_add_const_name_literal(zend_string *name, bool unqualified) /* {{{ */
{
	zend_string *tmp_name;
	// 1.添加原始名字
	int ret = zend_add_literal_string(&name);

	size_t ns_len = 0, after_ns_len = ZSTR_LEN(name);
	const char *after_ns = zend_memrchr(ZSTR_VAL(name), '\\', ZSTR_LEN(name));
	// 如果名字带命名空间
	if (after_ns) {
		after_ns += 1;
		ns_len = after_ns - ZSTR_VAL(name) - 1;
		after_ns_len = ZSTR_LEN(name) - ns_len - 1;

		// 2. 添加小写的名字（常量为什么也要添加小写名字）
		/* lowercased namespace name & original constant name */
		tmp_name = zend_string_init(ZSTR_VAL(name), ZSTR_LEN(name), 0);
		zend_str_tolower(ZSTR_VAL(tmp_name), ns_len);
		zend_add_literal_string(&tmp_name);

		if (!unqualified) {
			return ret;
		}
	// 如果名字不带命名空间
	} else {
		after_ns = ZSTR_VAL(name);
	}

	// 3.原始的无命名空间的常量名，添加到 literals
	/* original unqualified constant name */
	tmp_name = zend_string_init(after_ns, after_ns_len, 0);
	zend_add_literal_string(&tmp_name);

	return ret;
}
/* }}} */

// ing3, 添加literal，并把返回的literal序号存放到 op.constant里
#define LITERAL_STR(op, str) do { \
		zval _c; \
		ZVAL_STR(&_c, str); \
		/* 添加 literal ，是在当前 CG(active_op_array)->literals 中添加一个 zval */ \
		op.constant = zend_add_literal(&_c); \
	} while (0)

// ing2, 触发全局stop事件, 
// 调用和修改 _zend_php_scanner_globals 结构体中的元素,  __halt_compiler() 函数用到
void zend_stop_lexing(void)
{
	// _zend_php_scanner_globals 结构体
	// # define LANG_SCNG(v) (language_scanner_globals.v)
	if (LANG_SCNG(on_event)) {
		LANG_SCNG(on_event)(ON_STOP, END, 0, NULL, 0, LANG_SCNG(on_event_context));
	}

	LANG_SCNG(yy_cursor) = LANG_SCNG(yy_limit);
}

// ing3, 开始循环， free_opcode 是清空循环变量用的操作码
static inline void zend_begin_loop(zend_uchar free_opcode, const znode *loop_var, bool is_switch) /* {{{ */
{
	// break 元素
	zend_brk_cont_element *brk_cont_element;
	// 当前上下文中的 zend_brk_cont_element 编号
	int parent = CG(context).current_brk_cont;
	// 循环变量
	zend_loop_var info = {0};
	// current_brk_cont 赋值2/3：更新 current_brk_cont
	// 这个地方是关键，last_brk_cont 是上下文中 zend_brk_cont_element 的数量
	CG(context).current_brk_cont = CG(context).last_brk_cont;
	
	// 创建并返回空的 zend_brk_cont_element
	brk_cont_element = get_next_brk_cont_element();
	// 当前上下文中的 元素序号，记录到新元素中
	brk_cont_element->parent = parent;
	// 是否是 switch 语句
	brk_cont_element->is_switch = is_switch;

	// 如果有循环变量，并且类型是变量或临时变量
	if (loop_var && (loop_var->op_type & (IS_VAR|IS_TMP_VAR))) {
		// 最新行号：前上下文中的操作码数量
		uint32_t start = get_next_op_number();
		// 用 free_opcode 创建操作码 ，释放临时变量
		info.opcode = free_opcode;
		// 循环变量中的类型
		info.var_type = loop_var->op_type;
		// 变量序号 ?
		info.var_num = loop_var->u.op.var;
		// 跳转的操作码行号
		brk_cont_element->start = start;
	// 其他情况
	} else {
		// 操作码为 ZEND_NOP
		info.opcode = ZEND_NOP;
		// 开始区域用来在发生 异常时 释放临时变量。如果没有循环变量就不用释放了。
		/* The start field is used to free temporary variables in case of exceptions.
		 * We won't try to free something of we don't have loop variable.  */
		// 开始位置无效
		brk_cont_element->start = -1;
	}
	// 把info 压到循环堆栈里
	zend_stack_push(&CG(loop_var_stack), &info);
}
/* }}} */

// ing2, 循环结束，参数意义
static inline void zend_end_loop(int cont_addr, const znode *var_node) /* {{{ */
{
	// 最新行号：前上下文中的操作码数量
	uint32_t end = get_next_op_number();
	// 当前的brk_cont_element
	zend_brk_cont_element *brk_cont_element
		= &CG(context).brk_cont_array[CG(context).current_brk_cont];
	// 更新 cont 元素 ？
	brk_cont_element->cont = cont_addr;
	// 更新 brk 为：前上下文中的操作码数量
	brk_cont_element->brk = end;
	// current_brk_cont 赋值3/3：回到上一层跳转点
	CG(context).current_brk_cont = brk_cont_element->parent;
	// 循环变量堆栈中删除最上一个
	zend_stack_del_top(&CG(loop_var_stack));
}
/* }}} */

// 100行
// ing2, 销毁语句或表达式的编译结果
static void zend_do_free(znode *op1) /* {{{ */
{
	// 如果是临时变量
	if (op1->op_type == IS_TMP_VAR) {
		// 
		zend_op *opline = &CG(active_op_array)->opcodes[CG(active_op_array)->last-1];
		// 如果碰到 ZEND_END_SILENCE 或 ZEND_OP_DATA，前一一个
		while (opline->opcode == ZEND_END_SILENCE ||
		       opline->opcode == ZEND_OP_DATA) {
			opline--;
		}

		// 如果结果类型是 临时变量 并且 操作码结果等于第一个运算对象的结果
		if (opline->result_type == IS_TMP_VAR && opline->result.var == op1->u.op.var) {
			// 根据操作码处理
			switch (opline->opcode) {
				// 布尔型结果不需要释放
				case ZEND_BOOL:
				case ZEND_BOOL_NOT:
					/* boolean results don't have to be freed */
					return;
				// 变量后面的 ++ 或 --
				case ZEND_POST_INC_STATIC_PROP:
				case ZEND_POST_DEC_STATIC_PROP:
				case ZEND_POST_INC_OBJ:
				case ZEND_POST_DEC_OBJ:
				case ZEND_POST_INC:
				case ZEND_POST_DEC:
					/* convert $i++ to ++$i */
					// 操作码左移两个
					opline->opcode -= 2;
					// 删除当前结果
					SET_UNUSED(opline->result);
					return;
				// 赋值
				case ZEND_ASSIGN:
				case ZEND_ASSIGN_DIM:
				case ZEND_ASSIGN_OBJ:
				case ZEND_ASSIGN_STATIC_PROP:
				// 自运算赋值, 例如 $a += 1;
				case ZEND_ASSIGN_OP:
				case ZEND_ASSIGN_DIM_OP:
				case ZEND_ASSIGN_OBJ_OP:
				case ZEND_ASSIGN_STATIC_PROP_OP:
				//  变量前面的 ++ 或 --
				case ZEND_PRE_INC_STATIC_PROP:
				case ZEND_PRE_DEC_STATIC_PROP:
				case ZEND_PRE_INC_OBJ:
				case ZEND_PRE_DEC_OBJ:
				case ZEND_PRE_INC:
				case ZEND_PRE_DEC:
					// 删除当前结果
					SET_UNUSED(opline->result);
					return;
			}
		}
		// 创建操作码: ZEND_FREE，销毁第一个运算对象
		zend_emit_op(NULL, ZEND_FREE, op1, NULL);
	// 如果是变量
	} else if (op1->op_type == IS_VAR) {
		// 取得最后一行操作码
		zend_op *opline = &CG(active_op_array)->opcodes[CG(active_op_array)->last-1];
		// 倒着遍历，跳过这3种操作码 
		while (opline->opcode == ZEND_END_SILENCE ||
				opline->opcode == ZEND_EXT_FCALL_END ||
				opline->opcode == ZEND_OP_DATA) {
			opline--;
		}
		// 如果结果类型是变量 并且 变量编号与 运算对象1 的变量编号相同
		if (opline->result_type == IS_VAR
			&& opline->result.var == op1->u.op.var) {
			// 如果是从 $this 里获取
			if (opline->opcode == ZEND_FETCH_THIS) {
				// 操作码转成  ZEND_NOP
				opline->opcode = ZEND_NOP;
			}
			// 结果标记成 unused, op.num = (uint32_t) -1;
			SET_UNUSED(opline->result);
		} else {
			// 倒着遍历
			while (opline >= CG(active_op_array)->opcodes) {
				// （ZEND_FETCH_LIST_R 或 ZEND_FETCH_LIST_W 操作码） 并且 
				if ((opline->opcode == ZEND_FETCH_LIST_R ||
				     opline->opcode == ZEND_FETCH_LIST_W) &&
					// 运算对象1类型是变量
				    opline->op1_type == IS_VAR &&
					// 操作码的操作对象1的变量 与当前 要释放的znode变量序号 相同 ？
					// u.op.var 只有此文件用到
				    opline->op1.var == op1->u.op.var) {
					// 创建 ZEND_FREE 操作码
					zend_emit_op(NULL, ZEND_FREE, op1, NULL);
					// 返回
					return;
				}
				// 如果结果类型是变量 并且 变量编号与 运算对象1 的变量编号相同
				if (opline->result_type == IS_VAR
					&& opline->result.var == op1->u.op.var) {
					// 如果是 new 操作
					if (opline->opcode == ZEND_NEW) {
						// 创建 ZEND_FREE 操作码
						zend_emit_op(NULL, ZEND_FREE, op1, NULL);
					}
					// 跳出
					break;
				}
				// 前一个
				opline--;
			}
		}
	// 如果是常量
	} else if (op1->op_type == IS_CONST) {
		// 不使用垃圾回收，直接销毁。当Opcache 把array 移动到 SHM中，他会销毁 zend_array结构体，
		// 所以对它的外部引用就变得无效了。 垃圾回收 会造成根缓冲区中存在这样的引用。
		/* Destroy value without using GC: When opcache moves arrays into SHM it will
		 * free the zend_array structure, so references to it from outside the op array
		 * become invalid. GC would cause such a reference in the root buffer. */
		 
		// 销毁没有引用次数的对象,并没有销毁 zval本身。nogc的意思是，不放入gc回收周期。大量引用 
		zval_ptr_dtor_nogc(&op1->u.constant);
	}
}
/* }}} */

// clear, 添加类修饰符，碰到异常组合返回 0
uint32_t zend_add_class_modifier(uint32_t flags, uint32_t new_flag) /* {{{ */
{
	// 组合标记
	uint32_t new_flags = flags | new_flag;
	// 新旧标记里都有 abstract
	if ((flags & ZEND_ACC_EXPLICIT_ABSTRACT_CLASS) && (new_flag & ZEND_ACC_EXPLICIT_ABSTRACT_CLASS)) {
		zend_throw_exception(zend_ce_compile_error,
			"Multiple abstract modifiers are not allowed", 0);
		return 0;
	}
	// 新旧标记里都有 final
	if ((flags & ZEND_ACC_FINAL) && (new_flag & ZEND_ACC_FINAL)) {
		zend_throw_exception(zend_ce_compile_error, "Multiple final modifiers are not allowed", 0);
		return 0;
	}
	// 新旧标记里都有 readonly
	if ((flags & ZEND_ACC_READONLY_CLASS) && (new_flag & ZEND_ACC_READONLY_CLASS)) {
		zend_throw_exception(zend_ce_compile_error, "Multiple readonly modifiers are not allowed", 0);
		return 0;
	}
	// 同时存在 final 和 abstract
	if ((new_flags & ZEND_ACC_EXPLICIT_ABSTRACT_CLASS) && (new_flags & ZEND_ACC_FINAL)) {
		zend_throw_exception(zend_ce_compile_error,
			"Cannot use the final modifier on an abstract class", 0);
		return 0;
	}
	return new_flags;
}
/* }}} */

// ing2, 验证并返回类成员修饰符，返回 int, （ parser 多个修饰符时 调用 ）
uint32_t zend_add_member_modifier(uint32_t flags, uint32_t new_flag) /* {{{ */
{
	
	uint32_t new_flags = flags | new_flag;
	// ?? 如果两个都包含 ZEND_ACC_PPP_MASK . what is ZEND_ACC_PPP_MASK
	if ((flags & ZEND_ACC_PPP_MASK) && (new_flag & ZEND_ACC_PPP_MASK)) {
		zend_throw_exception(zend_ce_compile_error,
			"Multiple access type modifiers are not allowed", 0);
		return 0;
	}
	// 如果两个都包含 abstract
	if ((flags & ZEND_ACC_ABSTRACT) && (new_flag & ZEND_ACC_ABSTRACT)) {
		zend_throw_exception(zend_ce_compile_error, "Multiple abstract modifiers are not allowed", 0);
		return 0;
	}
	// 如果两个都包含 static
	if ((flags & ZEND_ACC_STATIC) && (new_flag & ZEND_ACC_STATIC)) {
		zend_throw_exception(zend_ce_compile_error, "Multiple static modifiers are not allowed", 0);
		return 0;
	}
	// 如果两个都包含 final
	if ((flags & ZEND_ACC_FINAL) && (new_flag & ZEND_ACC_FINAL)) {
		zend_throw_exception(zend_ce_compile_error, "Multiple final modifiers are not allowed", 0);
		return 0;
	}
	// 如果两个都包含 readonly
	if ((flags & ZEND_ACC_READONLY) && (new_flag & ZEND_ACC_READONLY)) {
		zend_throw_exception(zend_ce_compile_error,
			"Multiple readonly modifiers are not allowed", 0);
		return 0;
	}
	// 如果一个abstract 一个 final
	if ((new_flags & ZEND_ACC_ABSTRACT) && (new_flags & ZEND_ACC_FINAL)) {
		zend_throw_exception(zend_ce_compile_error,
			"Cannot use the final modifier on an abstract class member", 0);
		return 0;
	}
	return new_flags;
}
/* }}} */

// ing3, 返回静态调用字串p1::p2，p1:类名，p2:成员名。
ZEND_API zend_string *zend_create_member_string(zend_string *class_name, zend_string *member_name) {
	return zend_string_concat3(
		ZSTR_VAL(class_name), ZSTR_LEN(class_name),
		"::", sizeof("::") - 1,
		ZSTR_VAL(member_name), ZSTR_LEN(member_name));
}

// ing4, 拼接命名空间引用路径
static zend_string *zend_concat_names(char *name1, size_t name1_len, char *name2, size_t name2_len) {
	// 拼接3个字符
	return zend_string_concat3(name1, name1_len, "\\", 1, name2, name2_len);
}

// ing3, 给函数名（或者类名？）添加命名空间
static zend_string *zend_prefix_with_ns(zend_string *name) {
	// 当前有命名空间
	if (FC(current_namespace)) {
		zend_string *ns = FC(current_namespace);
		// 返回拼接了当前命名空间的引用路径
		return zend_concat_names(ZSTR_VAL(ns), ZSTR_LEN(ns), ZSTR_VAL(name), ZSTR_LEN(name));
	} else {
		// ? 返回 name 的副本
		return zend_string_copy(name);
	}
}

// ing3, 获取不在类中的常量名或方法名
static zend_string *zend_resolve_non_class_name(
	zend_string *name, uint32_t type, bool *is_fully_qualified,
	bool case_sensitive, HashTable *current_import_sub
) {
	char *compound;
	*is_fully_qualified = 0;
	
	// 如果开头是反斜线
	if (ZSTR_VAL(name)[0] == '\\') {
		// 删除 \ 前缀（只有这是string 不是 label时）
		/* Remove \ prefix (only relevant if this is a string rather than a label) */
		*is_fully_qualified = 1;
		// 直接把名字复制一份返回, 不要开头的 \\ 
		return zend_string_init(ZSTR_VAL(name) + 1, ZSTR_LEN(name) - 1, 0);
	}

	// 如果类型是：完整路径
	if (type == ZEND_NAME_FQ) {
		*is_fully_qualified = 1;
		// 直接复制返回
		return zend_string_copy(name);
	}

	// 如果类型是：namespace 开头
	if (type == ZEND_NAME_RELATIVE) {
		*is_fully_qualified = 1;
		// 添加命名空间后返回
		return zend_prefix_with_ns(name);
	}

	// 当前引用集合 ?
	if (current_import_sub) {
		// 如果一个无\的名称 是一个 函数或常量的别名， 替换它
		/* If an unqualified name is a function/const alias, replace it. */
		zend_string *import_name;
		// 大小写敏感
		if (case_sensitive) {
			import_name = zend_hash_find_ptr(current_import_sub, name);
		// 不敏感
		} else {
			import_name = zend_hash_find_ptr_lc(current_import_sub, name);
		}
		// 如果在引用中找到了
		if (import_name) {
			*is_fully_qualified = 1;
			// 返回别名的原始名称
			return zend_string_copy(import_name);
		}
	}

	// 命名空间第一节, 它可能是其他命名空间的别名
	compound = memchr(ZSTR_VAL(name), '\\', ZSTR_LEN(name));
	if (compound) {
		*is_fully_qualified = 1;
	}

	// 在上下文中查找,FC(imports) 是全局上下文, 不同于 current_import_sub
	if (compound && FC(imports)) {
		// 如果是别名，替换它
		/* If the first part of a qualified name is an alias, substitute it. */
		size_t len = compound - ZSTR_VAL(name);
		zend_string *import_name = zend_hash_str_find_ptr_lc(FC(imports), ZSTR_VAL(name), len);
		// 如果有对应的原始名称
		if (import_name) {
			// 用原始名称替换最前面一段,然后返回
			return zend_concat_names(
				ZSTR_VAL(import_name), ZSTR_LEN(import_name), ZSTR_VAL(name) + len + 1, ZSTR_LEN(name) - len - 1);
		}
	}
	// 普通情况,无特殊逻辑, 添加命名空间后返回
	return zend_prefix_with_ns(name);
}
/* }}} */

// ing3, 把用户传入的函数名转成 运行时的函数名, 大小写不敏感
static zend_string *zend_resolve_function_name(zend_string *name, uint32_t type, bool *is_fully_qualified)
{
	return zend_resolve_non_class_name(
		name, type, is_fully_qualified, 0, FC(imports_function));
}

// ing3, 把用户传入的常量名转成 运行时的常量名， 常量名是大小写敏感的
static zend_string *zend_resolve_const_name(zend_string *name, uint32_t type, bool *is_fully_qualified)
{
	return zend_resolve_non_class_name(
		name, type, is_fully_qualified, 1, FC(imports_const));
}

// ing3, 获取类名，传入类名和类名类型: T_NAME_QUALIFIED,T_NAME_FULLY_QUALIFIED,T_NAME_RELATIVE
// 处理替换别名
static zend_string *zend_resolve_class_name(zend_string *name, uint32_t type) /* {{{ */
{
	char *compound;

	// 通过类名获取类的引用方式，self,parent,static 或其他(ZEND_FETCH_CLASS_DEFAULT)
	// 如果是用 self,parent,static 调用
	if (ZEND_FETCH_CLASS_DEFAULT != zend_get_class_fetch_type(name)) {
		// 规则是：中间有 \ ，报错
		if (type == ZEND_NAME_FQ) {
			zend_error_noreturn(E_COMPILE_ERROR,
				"'\\%s' is an invalid class name", ZSTR_VAL(name));
		}
		// 规则是： namespace 开头， 报错 
		if (type == ZEND_NAME_RELATIVE) {
			zend_error_noreturn(E_COMPILE_ERROR,
				"'namespace\\%s' is an invalid class name", ZSTR_VAL(name));
		}
		// 规则不可以是绝对路径调用
		ZEND_ASSERT(type == ZEND_NAME_NOT_FQ);
		// ？ 返回类名的副本
		return zend_string_copy(name);
	}

	// 如果是 namespace 开头
	if (type == ZEND_NAME_RELATIVE) {
		// 返回拼接完整命名空间的类名
		return zend_prefix_with_ns(name);
	}

	// 如果是绝对地址命名空间
	if (type == ZEND_NAME_FQ) {
		// 如果开头是 \ 
		if (ZSTR_VAL(name)[0] == '\\') {
			// 删除 \ 前缀（只有这是string 不是 label时）
			/* Remove \ prefix (only relevant if this is a string rather than a label) */
			name = zend_string_init(ZSTR_VAL(name) + 1, ZSTR_LEN(name) - 1, 0);
			// 通过类名获取类的引用方式，self,parent,static 或其他(ZEND_FETCH_CLASS_DEFAULT)
			// 如果类名是 \ + self,parent,static , 报错
			if (ZEND_FETCH_CLASS_DEFAULT != zend_get_class_fetch_type(name)) {
				zend_error_noreturn(E_COMPILE_ERROR,
					"'\\%s' is an invalid class name", ZSTR_VAL(name));
			}
			return name;
		}
		// 返回
		return zend_string_copy(name);
	}

	// 如果有导入列表，处理引用别名
	if (FC(imports)) {
		// 获取类名的第一节结尾位置指针,没有 \ 会返回false 。 (参考：main/php_variables.c : memchr(ZSTR_VAL(new_value), '\0', ZSTR_LEN(new_value)) 
		compound = memchr(ZSTR_VAL(name), '\\', ZSTR_LEN(name));
		// 如果有 \ 
		if (compound) {
			// 如果名字的第一部分是别名，替代它
			/* If the first part of a qualified name is an alias, substitute it. */
			// 结尾指针 - 开头指针
			size_t len = compound - ZSTR_VAL(name);
			// 在导入列表里查找
			zend_string *import_name =
				zend_hash_str_find_ptr_lc(FC(imports), ZSTR_VAL(name), len);
			// 如果找到了，把第一节别名替换掉，返回
			if (import_name) {
				// 
				return zend_concat_names(
					ZSTR_VAL(import_name), ZSTR_LEN(import_name), ZSTR_VAL(name) + len + 1, ZSTR_LEN(name) - len - 1);
			}
		// 没有 \ 
		} else {
			// 如果名字是别名，替换它
			/* If an unqualified name is an alias, replace it. */
			zend_string *import_name
				= zend_hash_find_ptr_lc(FC(imports), name);
			// 找到别名对应的引用，返回
			if (import_name) {
				return zend_string_copy(import_name);
			}
		}
	}
	// 如果不是绝对引用，也不是别名，在前面加上命名空间，然后返回
	/* If not fully qualified and not an alias, prepend the current namespace */
	return zend_prefix_with_ns(name);
}
/* }}} */

// ing3, 通过语句获取完整类名
zend_string *zend_resolve_class_name_ast(zend_ast *ast) /* {{{ */
{
	// 语句类型必须是 ZEND_AST_ZVAL
	zval *class_name = zend_ast_get_zval(ast);
	// 变量类型必须是string，否则抛错。这个错应该没法用php脚本触发。
	if (Z_TYPE_P(class_name) != IS_STRING) {
		zend_error_noreturn(E_COMPILE_ERROR, "Illegal class name");
	}
	return zend_resolve_class_name(Z_STR_P(class_name), ast->attr);
}
/* }}} */

// ing3, 销毁 label 指针
static void label_ptr_dtor(zval *zv) /* {{{ */
{
	efree_size(Z_PTR_P(zv), sizeof(zend_label));
}
/* }}} */

// ing3, 销毁字串
static void str_dtor(zval *zv)  /* {{{ */ {
	zend_string_release_ex(Z_STR_P(zv), 0);
}
/* }}} */

// ing4
static bool zend_is_call(zend_ast *ast);

// ing2, 添加try元素，只有 zend_compile_try 用到
static uint32_t zend_add_try_element(uint32_t try_op) /* {{{ */
{
	// 当前操作码列表
	zend_op_array *op_array = CG(active_op_array);
	// last_try_catch 计数 +1
	uint32_t try_catch_offset = op_array->last_try_catch++;
	//
	zend_try_catch_element *elem;

	// try_catch_array 创建 zend_try_catch_element 
	op_array->try_catch_array = safe_erealloc(
		op_array->try_catch_array, sizeof(zend_try_catch_element), op_array->last_try_catch, 0);

	// 初始化新的 zend_try_catch_element
	elem = &op_array->try_catch_array[try_catch_offset];
	// try 操作码
	elem->try_op = try_op;
	// catch操作码
	elem->catch_op = 0;
	// finally 操作码
	elem->finally_op = 0;
	// ?
	elem->finally_end = 0;
	// 返回 try_catch 数量
	return try_catch_offset;
}
/* }}} */

// ing1, function 添加引用
ZEND_API void function_add_ref(zend_function *function) /* {{{ */
{
	// 如果是用户定义函数
	if (function->type == ZEND_USER_FUNCTION) {
		// 操作码
		zend_op_array *op_array = &function->op_array;
		// ？
		if (op_array->refcount) {
			(*op_array->refcount)++;
		}
		// ？
		ZEND_MAP_PTR_INIT(op_array->run_time_cache, NULL);
		// 
		ZEND_MAP_PTR_INIT(op_array->static_variables_ptr, NULL);
	}

	// 如果有函数名，增加引用次数
	if (function->common.function_name) {
		zend_string_addref(function->common.function_name);
	}
}
/* }}} */

// ing3, 检查函数是否重复定义
static zend_never_inline ZEND_COLD ZEND_NORETURN void do_bind_function_error(zend_string *lcname, zend_op_array *op_array, bool compile_time) /* {{{ */
{
	// 全局变量中查找 lcname
	zval *zv = zend_hash_find_known_hash(compile_time ? CG(function_table) : EG(function_table), lcname);
	// 错误类型
	int error_level = compile_time ? E_COMPILE_ERROR : E_ERROR;
	// 
	zend_function *old_function;

	// 函数必须存在
	ZEND_ASSERT(zv != NULL);
	// 函数对象，zval 中的指针型变量
	old_function = (zend_function*)Z_PTR_P(zv);
	
	// 是用户函数并且 操作码数量>0
	if (old_function->type == ZEND_USER_FUNCTION
		&& old_function->op_array.last > 0) {
		// 报错，不可以重复定义函数
		zend_error_noreturn(error_level, "Cannot redeclare %s() (previously declared in %s:%d)",
					// 如果有传入 操作码，在传入的操作码里获取函数名，否则在函数定义里获取
					op_array ? ZSTR_VAL(op_array->function_name) : ZSTR_VAL(old_function->common.function_name),
					// 文件名，行号
					ZSTR_VAL(old_function->op_array.filename),
					old_function->op_array.opcodes[0].lineno);
	// 非用户函数 或 没有op_array的用户函数
	} else {
		// 报错，不可以重复定义函数
		zend_error_noreturn(error_level, "Cannot redeclare %s()",
			op_array ? ZSTR_VAL(op_array->function_name) : ZSTR_VAL(old_function->common.function_name));
	}
}

// ing2，把函数添加到函数列表中，p1:函数，p2:函数名
ZEND_API zend_result do_bind_function(zend_function *func, zval *lcname) /* {{{ */
{
	// EG(function_table) 中添加函数
	zend_function *added_func = zend_hash_add_ptr(EG(function_table), Z_STR_P(lcname), func);
	// 如果添加失败，说明重复，报错
	if (UNEXPECTED(!added_func)) {
		do_bind_function_error(Z_STR_P(lcname), &func->op_array, 0);
		return FAILURE;
	}

	// 函数增加引用次数
	if (func->op_array.refcount) {
		++*func->op_array.refcount;
	}
	// 如果有函数名
	if (func->common.function_name) {
		// 函数名字串增加引用次数
		zend_string_addref(func->common.function_name);
	}
	// ？ end_observer.h
	zend_observer_function_declared_notify(&func->op_array, Z_STR_P(lcname));
	return SUCCESS;
}
/* }}} */

// ing3, 构建类，并添加到类表中，p1:类 zval，p2:类名，p3:父类名
ZEND_API zend_class_entry *zend_bind_class_in_slot(
		zval *class_table_slot, zval *lcname, zend_string *lc_parent_name)
{
	// 类入口
	zend_class_entry *ce = Z_PTR_P(class_table_slot);
	// 如果类有 ZEND_ACC_PRELOADED 标记，并且编译选项没有 ZEND_COMPILE_PRELOAD 标记
	bool is_preloaded =
		(ce->ce_flags & ZEND_ACC_PRELOADED) && !(CG(compiler_options) & ZEND_COMPILE_PRELOAD);
	// 操作是否成功
	bool success;
	// 如果没有 preload
	if (EXPECTED(!is_preloaded)) {
		// 在哈希表中给类重新设置 key（类名）
		success = zend_hash_set_bucket_key(EG(class_table), (Bucket*) class_table_slot, Z_STR_P(lcname)) != NULL;
	} else {
		// 如果用到了预加载，不要替换已有的 bucket，添加一个新的
		/* If preloading is used, don't replace the existing bucket, add a new one. */
		
		// 添加类
		success = zend_hash_add_ptr(EG(class_table), Z_STR_P(lcname), ce) != NULL;
	}
	// 如果没有成功
	if (UNEXPECTED(!success)) {
		// 报错：无法定义类，因为同名类已存在
		zend_error_noreturn(E_COMPILE_ERROR, "Cannot declare %s %s, because the name is already in use", zend_get_object_type(ce), ZSTR_VAL(ce->name));
		// 返回null
		return NULL;
	}

	// 如果类已链接完成
	if (ce->ce_flags & ZEND_ACC_LINKED) {
		// 依次调用每个 链接回调 函数
		zend_observer_class_linked_notify(ce, Z_STR_P(lcname));
		// 返回类
		return ce;
	}

	// 链接类 : zend_inheritance.c
	ce = zend_do_link_class(ce, lc_parent_name, Z_STR_P(lcname));
	// 如果链接成功
	if (ce) {
		// 不能有异常
		ZEND_ASSERT(!EG(exception));
		// 依次调用每个 链接回调 函数
		zend_observer_class_linked_notify(ce, Z_STR_P(lcname));
		// 返回类
		return ce;
	}

	// 不是预加载
	if (!is_preloaded) {
		// 重载 bucket 指针，哈希表可能已经重新分配了
		/* Reload bucket pointer, the hash table may have been reallocated */
		// 类表中找到这个类
		zval *zv = zend_hash_find(EG(class_table), Z_STR_P(lcname));
		// 类表中给这个类 重新设置 key（类名）
		zend_hash_set_bucket_key(EG(class_table), (Bucket *) zv, Z_STR_P(lcname + 1));
	// 是预加载
	} else {
		// 类表中删除这个类
		zend_hash_del(EG(class_table), Z_STR_P(lcname));
	}
	// 返回null
	return NULL;
}

// ing2, 绑定父类，只有外部调用。p1:类型名，p2:父类名
ZEND_API zend_result do_bind_class(zval *lcname, zend_string *lc_parent_name) /* {{{ */
{
	zend_class_entry *ce;
	zval *rtd_key, *zv;

	// rtd_key是小写名字的下一个变量
	rtd_key = lcname + 1;

	// ？ 在类列表中查找 rtd_key
	zv = zend_hash_find_known_hash(EG(class_table), Z_STR_P(rtd_key));

	// 如果zv无效， 找不到 rtd_key
	if (UNEXPECTED(!zv)) {
		// 在类列表中查找 lcname
		ce = zend_hash_find_ptr(EG(class_table), Z_STR_P(lcname));
		// 这次必须找到
		ZEND_ASSERT(ce);
		// 报错 不可以重复定义 类或接口或trait （三种都测试过）
		zend_error_noreturn(E_COMPILE_ERROR, "Cannot declare %s %s, because the name is already in use", zend_get_object_type(ce), ZSTR_VAL(ce->name));
		return FAILURE;
	}

	// 注册派生类
	/* Register the derived class */
	// 构建类，并添加到类表中，p1:类 zval，p2:类名，p3:父类名
	return zend_bind_class_in_slot(zv, lcname, lc_parent_name) ? SUCCESS : FAILURE;
}
/* }}} */

// ing3, 添加类型字串
static zend_string *add_type_string(zend_string *type, zend_string *new_type, bool is_intersection) {
	zend_string *result;
	// 现有类型为空
	if (type == NULL) {
		return zend_string_copy(new_type);
	}
	// 现有类型不为空
	// 如果是交叉
	if (is_intersection) {
		// 用 & 字符拼接两个串，清空原type
		result = zend_string_concat3(ZSTR_VAL(type), ZSTR_LEN(type),
			"&", 1, ZSTR_VAL(new_type), ZSTR_LEN(new_type));
		// 释放type
		zend_string_release(type);
	// 如果是联合， 用 | 字符拼接两个串，清空原type
	} else {
		// type + | + new_type
		result = zend_string_concat3(
			ZSTR_VAL(type), ZSTR_LEN(type), "|", 1, ZSTR_VAL(new_type), ZSTR_LEN(new_type));
		// 释放type
		zend_string_release(type);
	}
	// 新字串
	return result;
}

// ing3, 转换类名，主要是转换 self 和 parent
static zend_string *resolve_class_name(zend_string *name, zend_class_entry *scope) {
	// 如果有指定作用域
	if (scope) {
		// 如果类名是 self
		if (zend_string_equals_literal_ci(name, "self")) {
			// 类名从作用域里获取
			name = scope->name;
		// 如果类名是 parent
		} else if (zend_string_equals_literal_ci(name, "parent") && scope->parent) {
			// 类名从父作用域里获取
			name = scope->parent->name;
		}
	}

	// 匿名类的完整名称是null。null后面的所有字节都剪掉，避免在后续操作中打印和执行
	/* The resolved name for anonymous classes contains null bytes. Cut off everything after the
	 * null byte here, to avoid larger parts of the type being omitted by printing code later. */
	// 类名长度
	size_t len = strlen(ZSTR_VAL(name));
	// 如果长度不相符
	if (len != ZSTR_LEN(name)) {
		// 断言 ？
		ZEND_ASSERT(scope && "This should only happen with resolved types");
		// 返回类名
		return zend_string_init(ZSTR_VAL(name), len, 0);
	}
	// 返回类名
	return zend_string_copy(name);
}

// ing3, 添加交叉引用类型，返回交集算式 字串
static zend_string *add_intersection_type(zend_string *str,
	zend_type_list *intersection_type_list, zend_class_entry *scope,
	bool is_bracketed)
{
	zend_type *single_type;
	zend_string *intersection_str = NULL;
	// 遍历所有的交叉类型
	ZEND_TYPE_LIST_FOREACH(intersection_type_list, single_type) {
		// 不可以包含列表类型标记 _ZEND_TYPE_LIST_BIT
		ZEND_ASSERT(!ZEND_TYPE_HAS_LIST(*single_type));
		// 必须是 type_name 类型 _ZEND_TYPE_NAME_BIT
		ZEND_ASSERT(ZEND_TYPE_HAS_NAME(*single_type));
		// 取回类型名字
		zend_string *name = ZEND_TYPE_NAME(*single_type);
		// 转换类名，主要是转换 self 和 parent
		zend_string *resolved = resolve_class_name(name, scope);
		// 取得现有的交叉类型（交集）
		intersection_str = add_type_string(intersection_str, resolved, /* is_intersection */ true);
		// 释放此类型
		zend_string_release(resolved);
	} ZEND_TYPE_LIST_FOREACH_END();

	// 交集必须存在
	ZEND_ASSERT(intersection_str);

	// 如果是纯粹的交叉类型，没有联合（没有并集）
	if (is_bracketed) {
		// 结果是 "( intersection_str )"
		zend_string *result = zend_string_concat3("(", 1, ZSTR_VAL(intersection_str), ZSTR_LEN(intersection_str), ")", 1);
		// 释放原交叉类型
		zend_string_release(intersection_str);
		// 让它指向现有结果 
		intersection_str = result;
	}
	// 添加类型字串
	str = add_type_string(str, intersection_str, /* is_intersection */ false);
	// 释放交叉类型变量
	zend_string_release(intersection_str);
	return str;
}

// ing3, 类型转成字串
zend_string *zend_type_to_string_resolved(zend_type type, zend_class_entry *scope) {
	zend_string *str = NULL;
	// 如果是 纯粹的交叉类型 _ZEND_TYPE_ARENA_BIT，没有联合
	/* Pure intersection type */
	if (ZEND_TYPE_IS_INTERSECTION(type)) {
		// 断言，不可以是联合类型（检查 _ZEND_TYPE_UNION_BIT 标记）
		ZEND_ASSERT(!ZEND_TYPE_IS_UNION(type));
		// 取得交集字串
		str = add_intersection_type(str, ZEND_TYPE_LIST(type), scope, /* is_bracketed */ false);
	// 列表类型标记 _ZEND_TYPE_LIST_BIT
	} else if (ZEND_TYPE_HAS_LIST(type)) {
		/* A union type might not be a list */
		zend_type *list_type;
		// 遍历类型
		ZEND_TYPE_LIST_FOREACH(ZEND_TYPE_LIST(type), list_type) {
			// 如果 在 compile_type 方法中添加过 _ZEND_TYPE_INTERSECTION_BIT 标记
			if (ZEND_TYPE_IS_INTERSECTION(*list_type)) {
				// 取得交集字串
				str = add_intersection_type(str, ZEND_TYPE_LIST(*list_type), scope, /* is_bracketed */ true);
				continue;
			}
			// 断言：不可以是列表
			ZEND_ASSERT(!ZEND_TYPE_HAS_LIST(*list_type));
			// 断言：有类型名
			ZEND_ASSERT(ZEND_TYPE_HAS_NAME(*list_type));
			// 获取类型名
			zend_string *name = ZEND_TYPE_NAME(*list_type);
			// 转换类名，主要是转换 self 和 parent
			zend_string *resolved = resolve_class_name(name, scope);
			// 添加类型字串
			str = add_type_string(str, resolved, /* is_intersection */ false);
			// 释放原类型
			zend_string_release(resolved);
		} ZEND_TYPE_LIST_FOREACH_END();
	// 名称类型标记 _ZEND_TYPE_NAME_BIT
	} else if (ZEND_TYPE_HAS_NAME(type)) {
		// 转换类名，主要是转换 self 和 parent
		str = resolve_class_name(ZEND_TYPE_NAME(type), scope);
	}

	// 类型为纯交叉（交集），没有联合（并集）
	uint32_t type_mask = ZEND_TYPE_PURE_MASK(type);

	// 可能是任何东东
	if (type_mask == MAY_BE_ANY) {
		// 添加 类型字串 “mixed” 并返回：_(ZEND_STR_MIXED, \"mixed\") /Zend/zend_string.h
		str = add_type_string(str, ZSTR_KNOWN(ZEND_STR_MIXED), /* is_intersection */ false);
		return str;
	}
	// 如果可能是 "static" ,用类名替代 "static" 字串
	if (type_mask & MAY_BE_STATIC) {
		// 字符 static 
		zend_string *name = ZSTR_KNOWN(ZEND_STR_STATIC);
		// 在计算好的代码的编译过程中 被调用的scope 引用到 调用的scppe
		// During compilation of eval'd code the called scope refers to the scope calling the eval
		// 如果有作用域 并且没有在编译
		if (scope && !zend_is_compiling()) {
			// 取得执行时的作用域
			zend_class_entry *called_scope = zend_get_called_scope(EG(current_execute_data));
			// 如果存在
			if (called_scope) {
				// 用它替换当前当前名称
				name = called_scope->name;
			}
		}
		// 添加类型字串
		str = add_type_string(str, name, /* is_intersection */ false);
	}
	// 闭包
	if (type_mask & MAY_BE_CALLABLE) {
		str = add_type_string(str, ZSTR_KNOWN(ZEND_STR_CALLABLE), /* is_intersection */ false);
	}
	// 对象
	if (type_mask & MAY_BE_OBJECT) {
		str = add_type_string(str, ZSTR_KNOWN(ZEND_STR_OBJECT), /* is_intersection */ false);
	}
	// 数组
	if (type_mask & MAY_BE_ARRAY) {
		str = add_type_string(str, ZSTR_KNOWN(ZEND_STR_ARRAY), /* is_intersection */ false);
	}
	// 字串
	if (type_mask & MAY_BE_STRING) {
		str = add_type_string(str, ZSTR_KNOWN(ZEND_STR_STRING), /* is_intersection */ false);
	}
	// 长整
	if (type_mask & MAY_BE_LONG) {
		str = add_type_string(str, ZSTR_KNOWN(ZEND_STR_INT), /* is_intersection */ false);
	}
	// 双精度
	if (type_mask & MAY_BE_DOUBLE) {
		str = add_type_string(str, ZSTR_KNOWN(ZEND_STR_FLOAT), /* is_intersection */ false);
	}
	// boolean
	if ((type_mask & MAY_BE_BOOL) == MAY_BE_BOOL) {
		str = add_type_string(str, ZSTR_KNOWN(ZEND_STR_BOOL), /* is_intersection */ false);
	// false
	} else if (type_mask & MAY_BE_FALSE) {
		str = add_type_string(str, ZSTR_KNOWN(ZEND_STR_FALSE), /* is_intersection */ false);
	// true
	} else if (type_mask & MAY_BE_TRUE) {
		str = add_type_string(str, ZSTR_KNOWN(ZEND_STR_TRUE), /* is_intersection */ false);
	}
	// 空, IS_VOID 这个type用得比较少
	if (type_mask & MAY_BE_VOID) {
		str = add_type_string(str, ZSTR_KNOWN(ZEND_STR_VOID), /* is_intersection */ false);
	}
	// IS_NEVER 指没有return 的function 的返回类型
	if (type_mask & MAY_BE_NEVER) {
		str = add_type_string(str, ZSTR_KNOWN(ZEND_STR_NEVER), /* is_intersection */ false);
	}

	// 如果可能是 NULL
	if (type_mask & MAY_BE_NULL) {
		// 如果名称不存在 或里面有字符 “|” ，表示是联合类型
		bool is_union = !str || memchr(ZSTR_VAL(str), '|', ZSTR_LEN(str)) != NULL;
		// 如果名称不存在或 里面有字符 “|”，表示包含交叉类型
		bool has_intersection = !str || memchr(ZSTR_VAL(str), '&', ZSTR_LEN(str)) != NULL;
		// 如果既没有交叉也没有联合
		if (!is_union && !has_intersection) {
			// 类型前面加个 问号 “?”
			zend_string *nullable_str = zend_string_concat2("?", 1, ZSTR_VAL(str), ZSTR_LEN(str));
			// 释放类型名
			zend_string_release(str);
			return nullable_str;
		}
		// 添加短路类型字串 "null"
		str = add_type_string(str, ZSTR_KNOWN(ZEND_STR_NULL_LOWERCASE), /* is_intersection */ false);
	}
	// 返回类型名
	return str;
}

// ing4, 返回string 类型的type
ZEND_API zend_string *zend_type_to_string(zend_type type) {
	return zend_type_to_string_resolved(type, NULL);
}

// ing4, 是否是兼容 generator 的类型，Traversable 或 Iterator 或 Generator
static bool is_generator_compatible_class_type(zend_string *name) {
	return zend_string_equals_literal_ci(name, "Traversable")
		|| zend_string_equals_literal_ci(name, "Iterator")
		|| zend_string_equals_literal_ci(name, "Generator");
}

// ing3, 把 function 标记生成生器( 方法里有yield时 ）
static void zend_mark_function_as_generator(void) /* {{{ */
{
	// 如果当前不在任何函数里，报错 yield表达式 只能在 function 里应用（测试过）
	if (!CG(active_op_array)->function_name) {
		zend_error_noreturn(E_COMPILE_ERROR,
			"The \"yield\" expression can only be used inside a function");
	}

	// 如果函数有返回类型
	if (CG(active_op_array)->fn_flags & ZEND_ACC_HAS_RETURN_TYPE) {
		// 返回类型的type
		zend_type return_type = CG(active_op_array)->arg_info[-1].type;
		// 是否是有效类型：返回类型可能是对象
		bool valid_type = (ZEND_TYPE_FULL_MASK(return_type) & MAY_BE_OBJECT) != 0;
		// 返回类型无效
		if (!valid_type) {
			zend_type *single_type;
			// 遍历所有可能的返回类型
			ZEND_TYPE_FOREACH(return_type, single_type) {
				// 如果 1.名称有 _ZEND_TYPE_NAME_BIT 标记
				if (ZEND_TYPE_HAS_NAME(*single_type)
						// 并且 2.是兼容 generator 的类型（Traversable 或 Iterator 或 Generator）
						&& is_generator_compatible_class_type(ZEND_TYPE_NAME(*single_type))) {
					// 返回类型有效
					valid_type = 1;
					break;
				}
			} ZEND_TYPE_FOREACH_END();
		}
		// 如果没有可以类型，报错
		if (!valid_type) {
			zend_string *str = zend_type_to_string(return_type);
			// 报错，此类型不可用（测试过 function a():float{yield $a=1;} ）
			zend_error_noreturn(E_COMPILE_ERROR,
				"Generator return type must be a supertype of Generator, %s given",
				ZSTR_VAL(str));
		}
	}
	// 直接添加标记，在 yield 语句中
	CG(active_op_array)->fn_flags |= ZEND_ACC_GENERATOR;
}
/* }}} */

// ing3, 拼接属性名，内部 zend_compile_halt_compiler调用，有外部蛮多调用
ZEND_API zend_string *zend_mangle_property_name(const char *src1, size_t src1_length, const char *src2, size_t src2_length, bool internal) /* {{{ */
{
	// 两个长度的和+2
	size_t prop_name_length = 1 + src1_length + 1 + src2_length;
	// 分配内存，创建字串
	zend_string *prop_name = zend_string_alloc(prop_name_length, internal);
	// 先置空
	ZSTR_VAL(prop_name)[0] = '\0';
	// 先复制第一个
	memcpy(ZSTR_VAL(prop_name) + 1, src1, src1_length+1);
	// 再复制第二个
	memcpy(ZSTR_VAL(prop_name) + 1 + src1_length + 1, src2, src2_length+1);
	return prop_name;
}
/* }}} */

// ing3, 计算 string 长度
static zend_always_inline size_t zend_strnlen(const char* s, size_t maxlen) /* {{{ */
{
	size_t len = 0;
	// 碰到 \0 会跳出？
	while (*s++ && maxlen--) len++;
	return len;
}
/* }}} */

// ？，（延后，与当前业务关系不大）
ZEND_API zend_result zend_unmangle_property_name_ex(const zend_string *name, const char **class_name, const char **prop_name, size_t *prop_len) /* {{{ */
{
	size_t class_name_len;
	size_t anonclass_src_len;

	*class_name = NULL;

	// 如果字串为空 或 开头是 \0
	if (!ZSTR_LEN(name) || ZSTR_VAL(name)[0] != '\0') {
		// 返回 属性名
		*prop_name = ZSTR_VAL(name);
		// 如果接收长度
		if (prop_len) {
			// 返回长度
			*prop_len = ZSTR_LEN(name);
		}
		// 返回成功
		return SUCCESS;
	}
	// 如果名称小于3个字符 或 第二个字符是 \0
	if (ZSTR_LEN(name) < 3 || ZSTR_VAL(name)[1] == '\0') {
		// 提示：非法变量名
		zend_error(E_NOTICE, "Illegal member variable name");
		// 返回 属性名
		*prop_name = ZSTR_VAL(name);
		// 如果接收长度
		if (prop_len) {
			// 返回长度
			*prop_len = ZSTR_LEN(name);
		}
		// 返回失败
		return FAILURE;
	}

	// 类名长度 前面去掉1个，后面去掉1个字符
	class_name_len = zend_strnlen(ZSTR_VAL(name) + 1, ZSTR_LEN(name) - 2);
	// 
	if (class_name_len >= ZSTR_LEN(name) - 2 || ZSTR_VAL(name)[class_name_len + 1] != '\0') {
		zend_error(E_NOTICE, "Corrupt member variable name");
		*prop_name = ZSTR_VAL(name);
		if (prop_len) {
			*prop_len = ZSTR_LEN(name);
		}
		return FAILURE;
	}

	*class_name = ZSTR_VAL(name) + 1;
	// 匿名类名长度
	anonclass_src_len = zend_strnlen(*class_name + class_name_len + 1, ZSTR_LEN(name) - class_name_len - 2);
	if (class_name_len + anonclass_src_len + 2 != ZSTR_LEN(name)) {
		class_name_len += anonclass_src_len + 1;
	}
	*prop_name = ZSTR_VAL(name) + class_name_len + 2;
	if (prop_len) {
		*prop_len = ZSTR_LEN(name) - class_name_len - 2;
	}
	return SUCCESS;
}
/* }}} */

// ing3，根据配置选项要求，是否可以编译常量
// typedef struct _zend_constant {	zval value;	zend_string *name; } zend_constant;
static bool can_ct_eval_const(zend_constant *c) {
	// 如果常量有弃用标记，返回false
	if (ZEND_CONSTANT_FLAGS(c) & CONST_DEPRECATED) {
		return 0;
	}
	// 如果有 1. 常量有【持久标记】
	if ((ZEND_CONSTANT_FLAGS(c) & CONST_PERSISTENT)
			// 并且 2. 配置选项中没有要求【不替换持久常量】
			&& !(CG(compiler_options) & ZEND_COMPILE_NO_PERSISTENT_CONSTANT_SUBSTITUTION)
			// 并且 3.不是：（常量有 【不使用文件缓存】 标记 并且 配置选项 开启了 【文件缓存编译】）, 返回 true
			&& !((ZEND_CONSTANT_FLAGS(c) & CONST_NO_FILE_CACHE)
				&& (CG(compiler_options) & ZEND_COMPILE_WITH_FILE_CACHE))) {
		return 1;
	}
	// 如果常量类型是 (null,true,false,整数，双精度，数组） 并且 配置选项 没有 【编译不替换常量】,返回 true
	if (Z_TYPE(c->value) < IS_OBJECT
			&& !(CG(compiler_options) & ZEND_COMPILE_NO_CONSTANT_SUBSTITUTION)) {
		return 1;
	}
	// 其他情况返回 false
	return 0;
}

// ing3, 尝试计算常量的值
static bool zend_try_ct_eval_const(zval *zv, zend_string *name, bool is_fully_qualified) /* {{{ */
{
	// 在查找可能是命名空间名称前，替换 true,false,null （包括命名空间里的相对路径引用）
	/* Substitute true, false and null (including unqualified usage in namespaces)
	 * before looking up the possibly namespaced name. */
	// 常量名
	const char *lookup_name = ZSTR_VAL(name);
	// 名称长度
	size_t lookup_len = ZSTR_LEN(name);

	// 如果不要完整路径
	if (!is_fully_qualified) {
		// 取得不带命名空间的类名
		zend_get_unqualified_name(name, &lookup_name, &lookup_len);
	}

	// 
	zend_constant *c;
	// 如果获取特殊常量成功（专指 true,false,null 三个常量 ）
	if ((c = zend_get_special_const(lookup_name, lookup_len))) {
		// 使用特殊常量的值
		ZVAL_COPY_VALUE(zv, &c->value);
		return 1;
	}
	// 使用名称获取常量的值
	c = zend_hash_find_ptr(EG(zend_constants), name);
	// 如果有值，并且 根据配置选项要求，可以编译常量
	if (c && can_ct_eval_const(c)) {
		// 复制常量的值
		ZVAL_COPY_OR_DUP(zv, &c->value);
		return 1;
	}
	return 0;
}
/* }}} */

// ing3, 如果在类方法中，或者在【被引用到类里的】trait 的方法中
static inline bool zend_is_scope_known(void) /* {{{ */
{
	// 如果还没有创建操作码列表，返回 false
	if (!CG(active_op_array)) {
		/* This can only happen when evaluating a default value string. */
		return 0;
	}

	// 如果当前操作码列表有闭包标记 返回 false
	if (CG(active_op_array)->fn_flags & ZEND_ACC_CLOSURE) {
		// 闭包可以重新保定到不同的作用域
		/* Closures can be rebound to a different scope */
		return 0;
	}

	// 如果当前有类作用域
	if (!CG(active_class_entry)) {
		// 如果在独立函数里，有作用域。如果在 include/eval 新文件时没有作用域（会继承 调用时的作用域）。
		/* The scope is known if we're in a free function (no scope), but not if we're in
		 * a file/eval (which inherits including/eval'ing scope). */
		// 如果所属类方法不为空，true, 否则false 
		return CG(active_op_array)->function_name != NULL;
	}

	// 这个判断用于traits 关联引用的类，不是trait 本身
	/* For traits self etc refers to the using class, not the trait itself */
	// 必须在类里，而不是trait 里
	return (CG(active_class_entry)->ce_flags & ZEND_ACC_TRAIT) == 0;
}
/* }}} */

// ing4, 类名是否与当前活动类相同
// ce: class entry
static inline bool class_name_refers_to_active_ce(zend_string *class_name, uint32_t fetch_type) /* {{{ */
{
	// 如果不在类中，返回0
	if (!CG(active_class_entry)) {
		return 0;
	}
	// 如果在类方法中，或者在【被引用到类里的】trait 的方法中
	if (fetch_type == ZEND_FETCH_CLASS_SELF && zend_is_scope_known()) {
		return 1;
	}
	// 如果是普通类名（非self,parent,static），返回类名与活动中的类名是否相等
	return fetch_type == ZEND_FETCH_CLASS_DEFAULT
		&& zend_string_equals_ci(class_name, CG(active_class_entry)->name);
}
/* }}} */

// ing4, 通过类名获取类的引用方式，self,parent,static 或其他。 绝大部分是内部调用
uint32_t zend_get_class_fetch_type(zend_string *name) /* {{{ */
{
	if (zend_string_equals_literal_ci(name, "self")) {
		return ZEND_FETCH_CLASS_SELF;
	} else if (zend_string_equals_literal_ci(name, "parent")) {
		return ZEND_FETCH_CLASS_PARENT;
	} else if (zend_string_equals_literal_ci(name, "static")) {
		return ZEND_FETCH_CLASS_STATIC;
	} else {
		return ZEND_FETCH_CLASS_DEFAULT;
	}
}
/* }}} */

// ing4, 通过语句 zend_ast 获取类引用方式 self,parent,static 或其他(ZEND_FETCH_CLASS_DEFAULT)
static uint32_t zend_get_class_fetch_type_ast(zend_ast *name_ast) /* {{{ */
{
	/* Fully qualified names are always default refs */
	if (name_ast->attr == ZEND_NAME_FQ) {
		return ZEND_FETCH_CLASS_DEFAULT;
	}
	// 通过类名获取类的引用方式，self,parent,static 或其他(ZEND_FETCH_CLASS_DEFAULT)
	return zend_get_class_fetch_type(zend_ast_get_str(name_ast));
}
/* }}} */

// ing4, 语句对象中获取完整类名
static zend_string *zend_resolve_const_class_name_reference(zend_ast *ast, const char *type)
{
	// 取得语句中的string
	zend_string *class_name = zend_ast_get_str(ast);
	// #define ZEND_FETCH_CLASS_DEFAULT 0
	// 如果获取到特殊类名（self,parent,static),抛错
	if (ZEND_FETCH_CLASS_DEFAULT != zend_get_class_fetch_type_ast(ast)) {
		zend_error_noreturn(E_COMPILE_ERROR,
			"Cannot use '%s' as %s, as it is reserved",
			ZSTR_VAL(class_name), type);
	}
	// 返回完整类名
	return zend_resolve_class_name(class_name, ast->attr);
}

// ing3, 保证调用类的方式有效, self,parent,static 没有用错
static void zend_ensure_valid_class_fetch_type(uint32_t fetch_type) /* {{{ */
{
	// 如果类型不是 default (是 self,parent 或 static) 
	// 并且 在类方法中，或者在【被引用到类里的】trait 的方法中
	if (fetch_type != ZEND_FETCH_CLASS_DEFAULT && zend_is_scope_known()) {
		// 当前所属类
		zend_class_entry *ce = CG(active_class_entry);
		// 如果米在类里，报错
		if (!ce) {
			// 不可以在类外面用 self,parent 或 static 做类名
			zend_error_noreturn(E_COMPILE_ERROR, "Cannot use \"%s\" when no class scope is active",
				fetch_type == ZEND_FETCH_CLASS_SELF ? "self" :
				fetch_type == ZEND_FETCH_CLASS_PARENT ? "parent" : "static");
		// 如果调用 parent 但 当前类 没有父类名
		} else if (fetch_type == ZEND_FETCH_CLASS_PARENT && !ce->parent_name) {
			zend_error_noreturn(E_COMPILE_ERROR,
				"Cannot use \"parent\" when current class scope has no parent");
		}
	}
}
/* }}} */

// ing4, 尝试编译常量表达式，计算类名
static bool zend_try_compile_const_expr_resolve_class_name(zval *zv, zend_ast *class_ast) /* {{{ */
{
	uint32_t fetch_type;
	zval *class_name;
	// 只处理内部变量
	if (class_ast->kind != ZEND_AST_ZVAL) {
		return 0;
	}
	// 获取内部变量
	class_name = zend_ast_get_zval(class_ast);

	// 类名必须是 string
	if (Z_TYPE_P(class_name) != IS_STRING) {
		zend_error_noreturn(E_COMPILE_ERROR, "Illegal class name");
	}
	// 通过类名获取类的引用方式，self,parent,static 或其他(ZEND_FETCH_CLASS_DEFAULT)
	fetch_type = zend_get_class_fetch_type(Z_STR_P(class_name));
	// 保证调用类的方式有效, self,parent,static 没有用错
	zend_ensure_valid_class_fetch_type(fetch_type);

	// 根据调用方式分别处理
	switch (fetch_type) {
		// self
		case ZEND_FETCH_CLASS_SELF:
			// 有类作用域 并且 在类方法中，或者在【被引用到类里的】trait 的方法中
			if (CG(active_class_entry) && zend_is_scope_known()) {
				// 返回当前类的名字 
				ZVAL_STR_COPY(zv, CG(active_class_entry)->name);
				return 1;
			}
			// 否则false
			return 0;
		// parent
		case ZEND_FETCH_CLASS_PARENT:
			// 有类作用域 并且 有父类名 
			if (CG(active_class_entry) && CG(active_class_entry)->parent_name
					// 并且 在类方法中，或者在【被引用到类里的】trait 的方法中
					&& zend_is_scope_known()) {
				// 返回父类的名字 
				ZVAL_STR_COPY(zv, CG(active_class_entry)->parent_name);
				return 1;
			}
			// 否则 false
			return 0;
		// static 直接返回 false
		case ZEND_FETCH_CLASS_STATIC:
			// 
			return 0;
		// 其它
		case ZEND_FETCH_CLASS_DEFAULT:
			// 从语句里 获取完整类名
			ZVAL_STR(zv, zend_resolve_class_name_ast(class_ast));
			return 1;
		// default: 
		EMPTY_SWITCH_DEFAULT_CASE()
	}
}
/* }}} */

// ing4, 验证类常量是否可访问，只有计算类常量时调用1次
// 不用 zend_verify_const_access 因为要处理未链接的类
/* We don't use zend_verify_const_access because we need to deal with unlinked classes. */
static bool zend_verify_ct_const_access(zend_class_constant *c, zend_class_entry *scope)
{
	// 如果是在trait里，返回false
	if (c->ce->ce_flags & ZEND_ACC_TRAIT) {
		// 只有直接访问trait常量时会遇到这种情况，因为在trait绑定到class时，ce 已经被组成类替代了
		/* This condition is only met on directly accessing trait constants,
		 * because the ce is replaced to the class entry of the composing class
		 * on binding. */
		return 0;
	// 如果是Public权限，返回true
	} else if (ZEND_CLASS_CONST_FLAGS(c) & ZEND_ACC_PUBLIC) {
		return 1;
	// private权限，看常量是否属于当前作用域
	} else if (ZEND_CLASS_CONST_FLAGS(c) & ZEND_ACC_PRIVATE) {
		return c->ce == scope;
	// 其他情况 protected ?
	} else {
		zend_class_entry *ce = c->ce;
		// 递归遍历父类，看常量是否属于某父类
		while (1) {
			// 如果属于当前类，返回true
			if (ce == scope) {
				return 1;
			}
			// 没有父类
			if (!ce->parent) {
				break;
			}
			// 如果父类已处理完，入口切换到父类
			if (ce->ce_flags & ZEND_ACC_RESOLVED_PARENT) {
				ce = ce->parent;
			// 引用列表中查找父类，入口切换到父类
			} else {
				ce = zend_hash_find_ptr_lc(CG(class_table), ce->parent_name);
				// 如果父类不在当前引用列表中
				if (!ce) {
					break;
				}
			}
		}
		// 在编译时，不允许相反的情况
		/* Reverse case cannot be true during compilation */
		return 0;
	}
}

// ing4, 计算并返回类常量，这里包含了访问权限控制
static bool zend_try_ct_eval_class_const(zval *zv, zend_string *class_name, zend_string *name) /* {{{ */
{
	// 通过类名获取类的引用方式，self,parent,static 或其他(ZEND_FETCH_CLASS_DEFAULT)
	uint32_t fetch_type = zend_get_class_fetch_type(class_name);
	zend_class_constant *cc;
	zval *c;
	// 如果类名与当前活动类相同
	if (class_name_refers_to_active_ce(class_name, fetch_type)) {
		// 直接在活动类中查找返回
		cc = zend_hash_find_ptr(&CG(active_class_entry)->constants_table, name);
	// 不相同： 如果是普通类名 并且 编译选项中没有禁用替换常量
	} else if (fetch_type == ZEND_FETCH_CLASS_DEFAULT && !(CG(compiler_options) & ZEND_COMPILE_NO_CONSTANT_SUBSTITUTION)) {
		// 取得类入口
		zend_class_entry *ce = zend_hash_find_ptr_lc(CG(class_table), class_name);
		// 如果存在，在类常量列表中查找返回
		if (ce) {
			cc = zend_hash_find_ptr(&ce->constants_table, name);
		// 如果类不存在，报错
		} else {
			return 0;
		}
	// 其他情况: 类不存在或 编译选项中禁用替换常量，报错
	} else {
		return 0;
	}

	// 如果禁止永久替换常量，报错
	if (CG(compiler_options) & ZEND_COMPILE_NO_PERSISTENT_CONSTANT_SUBSTITUTION) {
		return 0;
	}

	// 如果入口不存在 或 常量无权限访问
	if (!cc || !zend_verify_ct_const_access(cc, CG(active_class_entry))) {
		return 0;
	}

	// 取得常量的值
	c = &cc->value;

	// 如果c不是对象，替换大小写敏感的永久类常量
	/* Substitute case-sensitive (or lowercase) persistent class constants */
	if (Z_TYPE_P(c) < IS_OBJECT) {
		ZVAL_COPY_OR_DUP(zv, c);
		return 1;
	}

	return 0;
}
/* }}} */

// ing3, 添加到list 里，可以是任何类型的指针
static void zend_add_to_list(void *result, void *item) /* {{{ */
{
	void** list = *(void**)result;
	size_t n = 0;

	if (list) {
		while (list[n]) {
			n++;
		}
	}
	
	// 分配内存
	list = erealloc(list, sizeof(void*) * (n+2));

	list[n]   = item;
	list[n+1] = NULL;

	*(void**)result = list;
}
/* }}} */

// ing4, 创建操作码： ZEND_EXT_STMT，  extended statement 是什么
static void zend_do_extended_stmt(void) /* {{{ */
{
	zend_op *opline;
	
	// 如果 options 里没有 ZEND_COMPILE_EXTENDED_STMT（php启动时设置），直接返回
	if (!(CG(compiler_options) & ZEND_COMPILE_EXTENDED_STMT)) {
		return;
	}

	// 创建操作码 : ZEND_EXT_STMT
	opline = get_next_op();

	opline->opcode = ZEND_EXT_STMT;
}
/* }}} */

// ing3, 开始编译扩展信息（如果php配置里要求），创建 ZEND_EXT_FCALL_BEGIN 操作码
static void zend_do_extended_fcall_begin(void) /* {{{ */
{
	zend_op *opline;
	// 如果不包含 ZEND_COMPILE_EXTENDED_FCALL 标记，
		// 这个标记从属于 ZEND_COMPILE_EXTENDED_INFO ，是在php启动时设置的
	if (!(CG(compiler_options) & ZEND_COMPILE_EXTENDED_FCALL)) {
		return;
	}
	// 创建操作码 ZEND_EXT_FCALL_BEGIN
	opline = get_next_op();

	opline->opcode = ZEND_EXT_FCALL_BEGIN;
}
/* }}} */

// ing3, 结束编译扩展信息（如果php配置里要求），创建 ZEND_EXT_FCALL_END 操作码
static void zend_do_extended_fcall_end(void) /* {{{ */
{
	zend_op *opline;

	if (!(CG(compiler_options) & ZEND_COMPILE_EXTENDED_FCALL)) {
		return;
	}

	// 创建操作码 ZEND_EXT_FCALL_END
	opline = get_next_op();

	opline->opcode = ZEND_EXT_FCALL_END;
}
/* }}} */

// ing2, 检查是否是自动全部变量，就是在CG(auto_globals)里查找
ZEND_API bool zend_is_auto_global_str(const char *name, size_t len) /* {{{ */ {
	zend_auto_global *auto_global;
	// 如果能在全局变量列表中找到，并且不是null
	if ((auto_global = zend_hash_str_find_ptr(CG(auto_globals), name, len)) != NULL) {
		// ?
		if (auto_global->armed) {
			// 调用回调方法
			auto_global->armed = auto_global->auto_global_callback(auto_global->name);
		}
		// 返回 true
		return 1;
	}
	// 返回false
	return 0;
}
/* }}} */

// ing2, 是否是自动全局变量
ZEND_API bool zend_is_auto_global(zend_string *name) /* {{{ */
{
	zend_auto_global *auto_global;
	// 如果在compile auto_globals 列表中能获取到
	if ((auto_global = zend_hash_find_ptr(CG(auto_globals), name)) != NULL) {
		// ?
		if (auto_global->armed) {
			// 调用回调方法
			auto_global->armed = auto_global->auto_global_callback(auto_global->name);
		}
		return 1;
	}
	// 否则，false
	return 0;
}
/* }}} */

// ing3, 注册自动全局变量，用于注册 _GET,_POST,_COOKIE,_FILES jit参数是0 . GLOBALS jit参数是1.
// _SERVER,_ENV,_REQUEST jit参数是 PG(auto_globals_jit). 回调方法各自不同
ZEND_API zend_result zend_register_auto_global(zend_string *name, bool jit, zend_auto_global_callback auto_global_callback) /* {{{ */
{
	// 自动全局变量，未通过分配内存创建
	zend_auto_global auto_global;
	// 
	zend_result retval;

	// 名称
	auto_global.name = name;
	// 回调方法
	auto_global.auto_global_callback = auto_global_callback;
	// ？
	auto_global.jit = jit;

	// 在 CG(auto_globals) 中添加全局变量,
	// hash表中的元素是auto_global的副本内存指针，所以这里不用分配内存创建持久变量
	retval = zend_hash_add_mem(CG(auto_globals), auto_global.name, &auto_global, sizeof(zend_auto_global)) != NULL ? SUCCESS : FAILURE;

	// 返回添加是否成功
	return retval;
}
/* }}} */

// ing2, 激活自动全局变量
ZEND_API void zend_activate_auto_globals(void) /* {{{ */
{
	zend_auto_global *auto_global;

	// 遍历自动全局变量列表
	ZEND_HASH_MAP_FOREACH_PTR(CG(auto_globals), auto_global) {
		// 如果存在 .jit ，armed=1
		if (auto_global->jit) {
			auto_global->armed = 1;
		// 否则，如果有回调，调用它，把结果存放在 .armed里
		} else if (auto_global->auto_global_callback) {
			auto_global->armed = auto_global->auto_global_callback(auto_global->name);
		// 否则没有 .armed
		} else {
			auto_global->armed = 0;
		}
	} ZEND_HASH_FOREACH_END();
}
/* }}} */

// suspend， /Zend/zend_highlight.c 注释里有，这东东貌似并没有被用到
int ZEND_FASTCALL zendlex(zend_parser_stack_elem *elem) /* {{{ */
{
	zval zv;
	int ret;

	if (CG(increment_lineno)) {
		CG(zend_lineno)++;
		CG(increment_lineno) = 0;
	}

	ret = lex_scan(&zv, elem);
	ZEND_ASSERT(!EG(exception) || ret == T_ERROR);
	return ret;

}
/* }}} */

// ing1, 初始化类数据，是否把所有操作设置成null
ZEND_API void zend_initialize_class_data(zend_class_entry *ce, bool nullify_handlers) /* {{{ */
{
	// 持久哈希
	bool persistent_hashes = ce->type == ZEND_INTERNAL_CLASS;

	// 引用数 1
	ce->refcount = 1;
	// ？
	ce->ce_flags = ZEND_ACC_CONSTANTS_UPDATED;

	// 如果编译配置选项里 要求编译警卫
	if (CG(compiler_options) & ZEND_COMPILE_GUARDS) {
		// 添加警卫标记
		ce->ce_flags |= ZEND_ACC_USE_GUARDS;
	}

	// 默认属性列表
	ce->default_properties_table = NULL;
	// 默认静成员性列表
	ce->default_static_members_table = NULL;
	// 初始化属性信息 hash表
	zend_hash_init(&ce->properties_info, 8, NULL, NULL, persistent_hashes);
	// 初始化类常量 hash表
	zend_hash_init(&ce->constants_table, 8, NULL, NULL, persistent_hashes);
	// 初始化成员方法 hash表 ， ZEND_FUNCTION_DTOR？
	zend_hash_init(&ce->function_table, 8, NULL, ZEND_FUNCTION_DTOR, persistent_hashes);

	// 如果是用户类（通过 zend_compile_class_decl 方法定义的）
	if (ce->type == ZEND_USER_CLASS) {
		// 初始化类里的用户文档
		ce->info.user.doc_comment = NULL;
	}
	// 初始化静态成员列表
	ZEND_MAP_PTR_INIT(ce->static_members_table, NULL);
	// 初始化指针 ce->mutable_data__ptr=NULL
	ZEND_MAP_PTR_INIT(ce->mutable_data, NULL);

	// 默认成员变量数为0
	ce->default_properties_count = 0;
	// 默认静态成员变量数为0
	ce->default_static_members_count = 0;
	// 默认静态成员信息表，和 properties_info 有什么区别
	ce->properties_info_table = NULL;
	// 默认修饰属性
	ce->attributes = NULL;
	// ？
	ce->enum_backing_type = IS_UNDEF;
	// 默认枚举表
	ce->backed_enum_table = NULL;

	// 所有handler都是NULL
	if (nullify_handlers) {
		// 构造方法
		ce->constructor = NULL;
		// 析构方法
		ce->destructor = NULL;
		// 克隆方法
		ce->clone = NULL;
		// 读取成员变量
		ce->__get = NULL;
		// 写入成员变量
		ce->__set = NULL;
		// 销毁成员变量
		ce->__unset = NULL;
		// 成员变量是否存在
		ce->__isset = NULL;
		// 调用成员方法
		ce->__call = NULL;
		// 调用静态成员方法
		ce->__callstatic = NULL;
		// 转成字串
		ce->__tostring = NULL;
		// 序列化
		ce->__serialize = NULL;
		// 反序列化
		ce->__unserialize = NULL;
		// 调试信息
		ce->__debugInfo = NULL;
		// ?
		ce->create_object = NULL;
		// 迭代方法
		ce->get_iterator = NULL;
		// 代方法指针
		ce->iterator_funcs_ptr = NULL;
		// 当成array访问
		ce->arrayaccess_funcs_ptr = NULL;
		// ？
		ce->get_static_method = NULL;
		// 父类
		ce->parent = NULL;
		// 父类名
		ce->parent_name = NULL;
		// 实现接口数量 
		ce->num_interfaces = 0;
		// 实现的接口
		ce->interfaces = NULL;
		// 调用 trait 数量 
		ce->num_traits = 0;
		// 调用 trait 名称
		ce->trait_names = NULL;
		// trait 别名
		ce->trait_aliases = NULL;
		// trait 优先顺序 
		ce->trait_precedences = NULL;
		// 序列化方法
		ce->serialize = NULL;
		// 反序列化方法
		ce->unserialize = NULL;
		// 如果是内部类
		if (ce->type == ZEND_INTERNAL_CLASS) {
			// 内置 模块 和内置函数列表也要清空
			ce->info.internal.module = NULL;
			ce->info.internal.builtin_functions = NULL;
		}
	}
}
/* }}} */

// ing2, 编译变量的名称，CV：compiled_variable
ZEND_API zend_string *zend_get_compiled_variable_name(const zend_op_array *op_array, uint32_t var) /* {{{ */
{
	// #define EX_VAR_TO_NUM(n)		((uint32_t)((n) / sizeof(zval) - ZEND_CALL_FRAME_SLOT))
	return op_array->vars[EX_VAR_TO_NUM(var)];
}
/* }}} */

// ing3, 拼接字串
zend_ast *zend_ast_append_str(zend_ast *left_ast, zend_ast *right_ast) /* {{{ */
{
	// 左侧串中的常量：原字串
	zval *left_zv = zend_ast_get_zval(left_ast);
	// 字串值
	zend_string *left = Z_STR_P(left_zv);
	// 右侧字串值
	zend_string *right = zend_ast_get_str(right_ast);

	zend_string *result;
	// 左侧字串长度
	size_t left_len = ZSTR_LEN(left);
	// 拼接后长度
	size_t len = left_len + ZSTR_LEN(right) + 1; /* left\right */

	// 给left 增加长度
	result = zend_string_extend(left, len, 0);
	// 结尾本来是\0, 换成\\ 
	ZSTR_VAL(result)[left_len] = '\\';
	// 把right 拼接进来
	memcpy(&ZSTR_VAL(result)[left_len + 1], ZSTR_VAL(right), ZSTR_LEN(right));
	// 新字串结尾
	ZSTR_VAL(result)[len] = '\0';
	// 销毁右侧字串
	zend_string_release_ex(right, 0);

	ZVAL_STR(left_zv, result);
	return left_ast;
}
/* }}} */

// ing3, 转成负数字串, 语法解析时用。
// 被转的可以是整形或字串类型的【数值】,不能是其他字串（测试过）
zend_ast *zend_negate_num_string(zend_ast *ast) /* {{{ */
{
	// 取回语句里的内置变量
	zval *zv = zend_ast_get_zval(ast);
	// 如果是long型
	if (Z_TYPE_P(zv) == IS_LONG) {
		// 如果值是0
		if (Z_LVAL_P(zv) == 0) {
			// 把值更新成字串型 "0"
			ZVAL_NEW_STR(zv, zend_string_init("-0", sizeof("-0")-1, 0));
		// 其他情况 *-1
		} else {
			ZEND_ASSERT(Z_LVAL_P(zv) > 0);
			Z_LVAL_P(zv) *= -1;
		}
		
	// string型，字串向右移，在最前面加个0
	} else if (Z_TYPE_P(zv) == IS_STRING) {
		size_t orig_len = Z_STRLEN_P(zv);
		Z_STR_P(zv) = zend_string_extend(Z_STR_P(zv), orig_len + 1, 0);
		memmove(Z_STRVAL_P(zv) + 1, Z_STRVAL_P(zv), orig_len + 1);
		Z_STRVAL_P(zv)[0] = '-';
		
	// 其他类型
	} else {
		ZEND_UNREACHABLE();
	}
	return ast;
}
/* }}} */

// ing4, 验证命名空间是否合法，zend_compile_top_stmt一处调用
static void zend_verify_namespace(void) /* {{{ */
{
	// 如果有含代码块的命名空间，但当前不在命名空间中
	if (FC(has_bracketed_namespaces) && !FC(in_namespace)) {
		zend_error_noreturn(E_COMPILE_ERROR, "No code may exist outside of namespace {}");
	}
}
/* }}} */

// ing2, 取得目录
/* {{{ zend_dirname
   Returns directory name component of path */
ZEND_API size_t zend_dirname(char *path, size_t len)
{
	char *end = path + len - 1;
	unsigned int len_adjust = 0;

#ifdef ZEND_WIN32
	/* Note that on Win32 CWD is per drive (heritage from CP/M).
	 * This means dirname("c:foo") maps to "c:." or "c:" - which means CWD on C: drive.
	 */
	if ((2 <= len) && isalpha((int)((unsigned char *)path)[0]) && (':' == path[1])) {
		/* Skip over the drive spec (if any) so as not to change */
		path += 2;
		len_adjust += 2;
		if (2 == len) {
			/* Return "c:" on Win32 for dirname("c:").
			 * It would be more consistent to return "c:."
			 * but that would require making the string *longer*.
			 */
			return len;
		}
	}
#endif

	if (len == 0) {
		/* Illegal use of this function */
		return 0;
	}

	/* Strip trailing slashes */
	while (end >= path && IS_SLASH_P(end)) {
		end--;
	}
	if (end < path) {
		/* The path only contained slashes */
		path[0] = DEFAULT_SLASH;
		path[1] = '\0';
		return 1 + len_adjust;
	}

	/* Strip filename */
	while (end >= path && !IS_SLASH_P(end)) {
		end--;
	}
	if (end < path) {
		/* No slash found, therefore return '.' */
		path[0] = '.';
		path[1] = '\0';
		return 1 + len_adjust;
	}

	/* Strip slashes which came before the file name */
	while (end >= path && IS_SLASH_P(end)) {
		end--;
	}
	if (end < path) {
		path[0] = DEFAULT_SLASH;
		path[1] = '\0';
		return 1 + len_adjust;
	}
	*(end+1) = '\0';

	return (size_t)(end + 1 - path) + len_adjust;
}
/* }}} */

// ing3, 按变量的操作类型给opline适配操作码
static void zend_adjust_for_fetch_type(zend_op *opline, znode *result, uint32_t type) /* {{{ */
{
	// 如果 操作码是 ZEND_FETCH_STATIC_PROP_R 返回 1：ZEND_ADD 否则  3：ZEND_MUL
	zend_uchar factor = (opline->opcode == ZEND_FETCH_STATIC_PROP_R) ? 1 : 3;

	// 在操作码上加一个数字，做这个的目的是从opcode上区分出操作类型
	switch (type) {
		// 读 +0
		case BP_VAR_R:
			// 结果类型标记成 临时变量
			opline->result_type = IS_TMP_VAR;
			result->op_type = IS_TMP_VAR;
			return;
		// 写 *1，=1:ZEND_ADD 或 3:ZEND_MUL
		case BP_VAR_W:
			opline->opcode += 1 * factor;
			return;
		// 读写 *2，=2:ZEND_SUB 或 6:ZEND_SL 左移
		case BP_VAR_RW:
			opline->opcode += 2 * factor;
			return;
		// issue 发布 *3，=3 或 9
		case BP_VAR_IS:
			opline->result_type = IS_TMP_VAR;
			result->op_type = IS_TMP_VAR;
			opline->opcode += 3 * factor;
			return;
		// 函数参数 ？*4，=4 或 12
		case BP_VAR_FUNC_ARG:
			opline->opcode += 4 * factor;
			return;
		// Unset +5，*5 或 15
		case BP_VAR_UNSET:
			opline->opcode += 5 * factor;
			return;
		EMPTY_SWITCH_DEFAULT_CASE()
	}
}
/* }}} */

// ing3, 把操作码的运行结果返回给 result，操作码结果标记成 IS_VAR。 
// 应该是关联到result ? 因为这时候操作码还没有被执行，不过又用了 GET_NODE？
// result是指操作码的 result 元素，var 是指result 元素的类型， 只有内部调用
static inline void zend_make_var_result(znode *result, zend_op *opline) /* {{{ */
{
	// 变量类型
	opline->result_type = IS_VAR;
	// 增加变量计数
	opline->result.var = get_temporary_variable();
	// 把操作码的运行结果返回给 result
	GET_NODE(result, opline->result);
}
/* }}} */

// ing3, 把操作码的运行结果返回给 result，操作码结果标记成 IS_TMP_VAR
// result是指操作码的 result 元素，tmp 是指result 元素的类型， 只有内部调用
static inline void zend_make_tmp_result(znode *result, zend_op *opline) /* {{{ */
{
	opline->result_type = IS_TMP_VAR;
	// 增加变量计数
	opline->result.var = get_temporary_variable();
	GET_NODE(result, opline->result);
}
/* }}} */

// ing3, 返回一个新操作码，调用它的操作有90多个，有很多重复，并没有覆盖全部操作码
// 新操作码返回的 znode 应该是空的。
// 创建操作码主要是通过这两个方法
static zend_op *zend_emit_op(znode *result, zend_uchar opcode, znode *op1, znode *op2) /* {{{ */
{

// 在当前上下文中创建新操作码
	zend_op *opline = get_next_op();
// 设置 opcode 	
	opline->opcode = opcode;
// 设置 zend_op 的两个 op对象
	if (op1 != NULL) {
		SET_NODE(opline->op1, op1);
	}

	if (op2 != NULL) {
		SET_NODE(opline->op2, op2);
	}
// 如果要求返回结果
	if (result) {
		// 把操作码的运行结果返回给 result, 操作码标记成 IS_VAR
		zend_make_var_result(result, opline);
	}
	return opline;
}
/* }}} */

// ing3, 同上，只是创建的变量标记不同 ，60次内部引用
// 新操作码返回的 znode 应该是空的
static zend_op *zend_emit_op_tmp(znode *result, zend_uchar opcode, znode *op1, znode *op2) /* {{{ */
{
	// 创建新行
	zend_op *opline = get_next_op();
	opline->opcode = opcode;

	// 第一个op
	if (op1 != NULL) {
		SET_NODE(opline->op1, op1);
	}

	// 第二个op 
	if (op2 != NULL) {
		SET_NODE(opline->op2, op2);
	}

	if (result) {
		// 操作码结果类型 为临时变量
		zend_make_tmp_result(result, opline);
	}

	return opline;
}
/* }}} */

// ing3, 创建 ZEND_TICKS 操作码，只有 zend_compile_stmt 用到
static void zend_emit_tick(void) /* {{{ */
{
	zend_op *opline;

	// 阻止由解释器语句 declare() 造成的双重 tick
	/* This prevents a double TICK generated by the parser statement of "declare()" */
	// 
	if (CG(active_op_array)->last && CG(active_op_array)->opcodes[CG(active_op_array)->last - 1].opcode == ZEND_TICKS) {
		return;
	}
	// 创建 ZEND_TICKS 操作码
	opline = get_next_op();

	opline->opcode = ZEND_TICKS;
	// declare()里面定义的ticks
	opline->extended_value = FC(declarables).ticks;
}
/* }}} */

// ing3,创建 ZEND_OP_DATA 操作码
static inline zend_op *zend_emit_op_data(znode *value) /* {{{ */
{
	return zend_emit_op(NULL, ZEND_OP_DATA, value, NULL);
}
/* }}} */

// ing3, 创建 ZEND_JMP 操作码，跳转到指定编号的操作码，返回操作码顺序号
static inline uint32_t zend_emit_jump(uint32_t opnum_target) /* {{{ */
{
	// 最新行号：当前上下文中的操作码数量
	uint32_t opnum = get_next_op_number();
	// 创建操作码 ZEND_JMP 
	zend_op *opline = zend_emit_op(NULL, ZEND_JMP, NULL, NULL);
	// 跳转目标行号
	opline->op1.opline_num = opnum_target;
	return opnum;
}
/* }}} */

// ing3, 智能分支（一些用作判断的操作码）
ZEND_API bool zend_is_smart_branch(const zend_op *opline) /* {{{ */
{
	// 以下特定操作码，算 smart_branch
	switch (opline->opcode) {
		case ZEND_IS_IDENTICAL:
		case ZEND_IS_NOT_IDENTICAL:
		case ZEND_IS_EQUAL:
		case ZEND_IS_NOT_EQUAL:
		case ZEND_IS_SMALLER:
		case ZEND_IS_SMALLER_OR_EQUAL:
		case ZEND_CASE:
		case ZEND_CASE_STRICT:
		case ZEND_ISSET_ISEMPTY_CV:
		case ZEND_ISSET_ISEMPTY_VAR:
		case ZEND_ISSET_ISEMPTY_DIM_OBJ:
		case ZEND_ISSET_ISEMPTY_PROP_OBJ:
		case ZEND_ISSET_ISEMPTY_STATIC_PROP:
		case ZEND_INSTANCEOF:
		case ZEND_TYPE_CHECK:
		case ZEND_DEFINED:
		case ZEND_IN_ARRAY:
		case ZEND_ARRAY_KEY_EXISTS:
			return 1;
		default:
			return 0;
	}
}
/* }}} */

// ing3, 创建 按条件(znode) 跳转 操作码，返回新操作码顺序号
static inline uint32_t zend_emit_cond_jump(zend_uchar opcode, znode *cond, uint32_t opnum_target) /* {{{ */
{
	// 下一个操作码序号（当前上下文操作码数量，如果它为空，这里会返回0 )
	uint32_t opnum = get_next_op_number();
	// 操作码
	zend_op *opline;
	// 如果条件的 类型 是 IS_TMP_VAR 并且操作码序号 > 0
	if (cond->op_type == IS_TMP_VAR && opnum > 0) {
		// ？zend_op 指针 如何直接做加减法？
		opline = CG(active_op_array)->opcodes + opnum - 1;
		// 如果操作码结果类型是 临时变量，并且结果等于 cond 的值，并且 操作码属于 smart branch
		if (opline->result_type == IS_TMP_VAR
		 && opline->result.var == cond->u.op.var
		 && zend_is_smart_branch(opline)) {
			// 操作码 更换成 ZEND_JMPZ，结果类型添加 IS_SMART_BRANCH_JMPZ 标记
			if (opcode == ZEND_JMPZ) {
				opline->result_type = IS_TMP_VAR | IS_SMART_BRANCH_JMPZ;
			// 否则
			} else {
				// 操作码必须是 ZEND_JMPNZ （？两个操作码的区别）
				ZEND_ASSERT(opcode == ZEND_JMPNZ);
				// 结果类型添加 IS_SMART_BRANCH_JMPNZ 标记
				opline->result_type = IS_TMP_VAR | IS_SMART_BRANCH_JMPNZ;
			}
		}
	}
	
	// 创建操作码 opcode，op1 是 cond
	opline = zend_emit_op(NULL, opcode, cond, NULL);
	// op2 的行号设置成目标行号
	opline->op2.opline_num = opnum_target;
	return opnum;
}
/* }}} */

// ing3, 更新跳转型操作码的跳转目标。opnum_jump，要跳转的操作码顺序号, opnum_target 新顺序号
static inline void zend_update_jump_target(uint32_t opnum_jump, uint32_t opnum_target) /* {{{ */
{
	// 找到要更新的操作码
	zend_op *opline = &CG(active_op_array)->opcodes[opnum_jump];
	// 根据类型操作, 写入目标操作码。 写进op1和op2 有什么区别呢？
	switch (opline->opcode) {		
		// ZEND_JMP 操作码，跳转行号写在第一个操作对象里。有什么区别？
		case ZEND_JMP:
			opline->op1.opline_num = opnum_target;
			break;
		// 这些操作码，跳转行号写在第二个操作对象里
		case ZEND_JMPZ:
		case ZEND_JMPNZ:
		case ZEND_JMPZ_EX:
		case ZEND_JMPNZ_EX:
		case ZEND_JMP_SET:
		case ZEND_COALESCE:
		case ZEND_JMP_NULL:
			opline->op2.opline_num = opnum_target;
			break;
		//
		EMPTY_SWITCH_DEFAULT_CASE()
	}
}
/* }}} */

// ing4, 更新指定跳转型操作码，跳转到下一个创建的新操作码 。
// opnum_jump，要更新的跳转型操作码顺序号
static inline void zend_update_jump_target_to_next(uint32_t opnum_jump) /* {{{ */
{
	// get_next_op_number返回当前上下文中的操作码数量
	zend_update_jump_target(opnum_jump, get_next_op_number());
}
/* }}} */

// ing3, 创建延时操作码
static inline zend_op *zend_delayed_emit_op(znode *result, zend_uchar opcode, znode *op1, znode *op2) /* {{{ */
{
	// 为什么不分配内存创建
	zend_op tmp_opline;

	// 直接初始化操作码
	init_op(&tmp_opline);
	// 操作码
	tmp_opline.opcode = opcode;
	// 更新两个操作对象
	if (op1 != NULL) {
		SET_NODE(tmp_opline.op1, op1);
	}
	if (op2 != NULL) {
		SET_NODE(tmp_opline.op2, op2);
	}
	// 如果要求返回结果
	if (result) {
		// 把操作码的运行结果返回给 result
		zend_make_var_result(result, &tmp_opline);
	}
	// 把操作码添加到延时堆栈的顶端
	zend_stack_push(&CG(delayed_oplines_stack), &tmp_opline);
	// 返回刚创建的操作码
	return zend_stack_top(&CG(delayed_oplines_stack));
}
/* }}} */

// ing3, 延时编译 开始，返回堆栈高度
static inline uint32_t zend_delayed_compile_begin(void) /* {{{ */
{
	// 返回堆栈高度。延时操作码堆栈。
	return zend_stack_count(&CG(delayed_oplines_stack));
}
/* }}} */

// ing3, 延时编译 结尾
static zend_op *zend_delayed_compile_end(uint32_t offset) /* {{{ */
{
	// 获取操作码堆栈的，元素列表指针。 // 返回堆栈元素列表的指针
	zend_op *opline = NULL, *oplines = zend_stack_base(&CG(delayed_oplines_stack));
	// 堆栈高度
	uint32_t i, count = zend_stack_count(&CG(delayed_oplines_stack));

	// 高度必须大于偏移量
	ZEND_ASSERT(count >= offset);
	// 从偏移量开始，遍历堆栈中的元素
	for (i = offset; i < count; ++i) {
		// 如果不是空操作码
		if (EXPECTED(oplines[i].opcode != ZEND_NOP)) {
			// 下一个操作码序号
			opline = get_next_op();
			// 复制第 i 个操作码
			memcpy(opline, &oplines[i], sizeof(zend_op));
		// 空操作码
		} else {
			// 在当前操作码列表中，获取指定的操作码
			opline = CG(active_op_array)->opcodes + oplines[i].extended_value;
		}
	}

	// 设置堆栈高度
	CG(delayed_oplines_stack).top = offset;
	// 返回操作码
	return opline;
}
/* }}} */

// ing3, 是否可能短路
static bool zend_ast_kind_is_short_circuited(zend_ast_kind ast_kind)
{
	switch (ast_kind) {
		// 数组元素
		case ZEND_AST_DIM:
		// 类属性
		case ZEND_AST_PROP:
		// 允空的类属性
		case ZEND_AST_NULLSAFE_PROP:
		// 静态属性
		case ZEND_AST_STATIC_PROP:
		// 方法调用
		case ZEND_AST_METHOD_CALL:
		// 允空的方法调用
		case ZEND_AST_NULLSAFE_METHOD_CALL:
		// 静态方法调用
		case ZEND_AST_STATIC_CALL:
			return 1;
		default:
			return 0;
	}
}

// ing4, 知否短路（可能来自 null 对象调用）。 short_circuited 短路，测试过
static bool zend_ast_is_short_circuited(const zend_ast *ast)
{
	// 可能来自null 调用的情况
	switch (ast->kind) {
		// $name[],$name{}, 中的 $name 部分。获取数组元素
		case ZEND_AST_DIM:
		// $name->prop 中的 $name 部分。获取对象属性
		case ZEND_AST_PROP:
		// $name::prop 中的 $name 部分。获取对象静态属性
		case ZEND_AST_STATIC_PROP:
		// $name->method() 中的 $name 部分。调用对象成员方法
		case ZEND_AST_METHOD_CALL:
		// $name::method() 中的 $name 部分。调用对象静态成员方法
		case ZEND_AST_STATIC_CALL:
			// 以上情况递归检查 $name 部分
			return zend_ast_is_short_circuited(ast->child[0]);
		// 确认是 ?-> 调用属性或方法
		case ZEND_AST_NULLSAFE_PROP:
		case ZEND_AST_NULLSAFE_METHOD_CALL:
			return 1;
		// 其它情确定不是 short_circuited
		default:
			return 0;
	}
}

/* Mark nodes that are an inner part of a short-circuiting chain.
 * We should not perform a "commit" on them, as it will be performed by the outer-most node.
 * We do this to avoid passing down an argument in various compile functions. */

#define ZEND_SHORT_CIRCUITING_INNER 0x8000

// ing3, 如果是可以短路的类型，添加短路标记 (应用还蛮多的）
static void zend_short_circuiting_mark_inner(zend_ast *ast) {
	// 如果是可以短路的类型
	if (zend_ast_kind_is_short_circuited(ast->kind)) {
		// 添加 ZEND_SHORT_CIRCUITING_INNER 标记
		ast->attr |= ZEND_SHORT_CIRCUITING_INNER;
	}
}

// ing3, 短路监测点，返回短路操作码堆栈深度
static uint32_t zend_short_circuiting_checkpoint(void)
{
	return zend_stack_count(&CG(short_circuiting_opnums));
}

// ing3, 提交带短路的操作集合
static void zend_short_circuiting_commit(uint32_t checkpoint, znode *result, zend_ast *ast)
{
	// 短路或 isset 或 empty 函数
	bool is_short_circuited = zend_ast_kind_is_short_circuited(ast->kind)
		|| ast->kind == ZEND_AST_ISSET || ast->kind == ZEND_AST_EMPTY;
	// 如果没有短路	
	if (!is_short_circuited) {
		// 监测点必须等于短路堆栈深度
		ZEND_ASSERT(zend_stack_count(&CG(short_circuiting_opnums)) == checkpoint
			&& "Short circuiting stack should be empty");
		return;
	}
	// 如果有短路，当前语句要求 ZEND_SHORT_CIRCUITING_INNER
	if (ast->attr & ZEND_SHORT_CIRCUITING_INNER) {
		/* Outer-most node will commit. */
		return;
	}

	// 弹出所有元素，为了给对应的操作码排序，（只要监测点不等于短路堆栈深度）
	while (zend_stack_count(&CG(short_circuiting_opnums)) != checkpoint) {
		// 获取堆栈的最后一个元素，它是opcode 号码
		uint32_t opnum = *(uint32_t *) zend_stack_top(&CG(short_circuiting_opnums));
		// 取得对应的 操作码
		zend_op *opline = &CG(active_op_array)->opcodes[opnum];
		// 操作码序号，向后排。get_next_op_number返回当前上下文中的操作码数量
		opline->op2.opline_num = get_next_op_number();
		// 给操作码添加执行结果
		SET_NODE(opline->result, result);
		// 标记当前操作是 isset ，empty，或普通 expr
		opline->extended_value |=
			ast->kind == ZEND_AST_ISSET ? ZEND_SHORT_CIRCUITING_CHAIN_ISSET :
			ast->kind == ZEND_AST_EMPTY ? ZEND_SHORT_CIRCUITING_CHAIN_EMPTY :
			                              ZEND_SHORT_CIRCUITING_CHAIN_EXPR;
		// 降低堆栈深度
		zend_stack_del_top(&CG(short_circuiting_opnums));
	}
}

// ing3, 创建 ZEND_JMP_NULL 操作码
static void zend_emit_jmp_null(znode *obj_node, uint32_t bp_type)
{
	// 最新行号：前上下文中的操作码数量
	uint32_t jmp_null_opnum = get_next_op_number();
	// 创建操作码 ZEND_JMP_NULL
	zend_op *opline = zend_emit_op(NULL, ZEND_JMP_NULL, obj_node, NULL);
	// 如果 第一个操作对象的类型是常量
	if (opline->op1_type == IS_CONST) {
		// 第一个操作对象添加引用次数
		Z_TRY_ADDREF_P(CT_CONSTANT(opline->op1));
	}
	// 如果类型是创建变量
	if (bp_type == BP_VAR_IS) {
		// 添加标记 ZEND_JMP_NULL_BP_VAR_IS
		opline->extended_value |= ZEND_JMP_NULL_BP_VAR_IS;
	}
	// 入栈短路堆栈
	zend_stack_push(&CG(short_circuiting_opnums), &jmp_null_opnum);
}

// 不用内存
#define ZEND_MEMOIZE_NONE 0
// 内存编译
#define ZEND_MEMOIZE_COMPILE 1
// 内存获取
#define ZEND_MEMOIZE_FETCH 2

// ing4, 记忆编译，用到记忆表来缓存编译结果，提升速度，只有 zend_compile_expr_inner 用到
static void zend_compile_memoized_expr(znode *result, zend_ast *expr) /* {{{ */
{
	int memoize_mode = CG(memoize_mode);
	// ZEND_MEMOIZE_COMPILE 模式
	if (memoize_mode == ZEND_MEMOIZE_COMPILE) {
		znode memoized_result;

		// 先通过普通模式编译
		/* Go through normal compilation */
		CG(memoize_mode) = ZEND_MEMOIZE_NONE;
		// 编译表达式语句, p1:返回结果，p2:表达式语句
		zend_compile_expr(result, expr);
		// 切换回 ZEND_MEMOIZE_COMPILE 模式
		CG(memoize_mode) = ZEND_MEMOIZE_COMPILE;

		// 如果结果类型是变量
		if (result->op_type == IS_VAR) {
			// 创建操作码 ZEND_COPY_TMP
			zend_emit_op(&memoized_result, ZEND_COPY_TMP, result, NULL);
		// 如果结果类型是 临时变量
		} else if (result->op_type == IS_TMP_VAR) {
			// 创建临时操作码 ZEND_COPY_TMP
			zend_emit_op_tmp(&memoized_result, ZEND_COPY_TMP, result, NULL);
		// 其他情况
		} else {
			// 如果结果是常量
			if (result->op_type == IS_CONST) {
				// 增加引用次数
				Z_TRY_ADDREF(result->u.constant);
			}
			memoized_result = *result;
		}
		// 把结果记录到记忆表 中
		zend_hash_index_update_mem(
			CG(memoized_exprs), (uintptr_t) expr, &memoized_result, sizeof(znode));
	// ZEND_MEMOIZE_FETCH 模式
	} else if (memoize_mode == ZEND_MEMOIZE_FETCH) {
		// 在记忆表 中查找编译好的结果
		znode *memoized_result = zend_hash_index_find_ptr(CG(memoized_exprs), (uintptr_t) expr);
		*result = *memoized_result;
		//如果结果类型是常量
		if (result->op_type == IS_CONST) {
			// 增加引用次数
			Z_TRY_ADDREF(result->u.constant);
		}
	// 其他模式
	} else {
		ZEND_UNREACHABLE();
	}
}
/* }}} */

// 修改此方法时，记得更新 compact_literals.c 中的 type_num_classes() 方法
/* Remember to update type_num_classes() in compact_literals.c when changing this function */
// ing3: 计算运算结果是有多少个类型交叉计算成的
static size_t zend_type_get_num_classes(zend_type type) {
	// 如果不是复杂类型
	if (!ZEND_TYPE_IS_COMPLEX(type)) {
		return 0;
	}
	// 如果是列表类型
	if (ZEND_TYPE_HAS_LIST(type)) {
		// 交叉类型不可以嵌套类型列表
		/* Intersection types cannot have nested list types */
		// 如果是交叉类型
		if (ZEND_TYPE_IS_INTERSECTION(type)) {
			// 返回类型数量
			return ZEND_TYPE_LIST(type)->num_types;
		}
		// 不可以是联合类型 _ZEND_TYPE_UNION_BIT
		ZEND_ASSERT(ZEND_TYPE_IS_UNION(type));
		size_t count = 0;
		zend_type *list_type;
		
		// 遍历类型列表
		ZEND_TYPE_LIST_FOREACH(ZEND_TYPE_LIST(type), list_type) {
			// 如果是交叉类型
			if (ZEND_TYPE_IS_INTERSECTION(*list_type)) {
				// 数量与当前类型的数量相加
				count += ZEND_TYPE_LIST(*list_type)->num_types;
			// 如果不是交叉类型
			} else {
				// 不可以包含列表
				ZEND_ASSERT(!ZEND_TYPE_HAS_LIST(*list_type));
				// 数量+1
				count += 1;
			}
		} ZEND_TYPE_LIST_FOREACH_END();
		// 返回数量 
		return count;
	}
	return 1;
}

// ing3, 检查函数的返回类型（可能创建操作码），expr是表达式的编译结果 ，return_info 返回类型
static void zend_emit_return_type_check(
		znode *expr, zend_arg_info *return_info, bool implicit) /* {{{ */
{
	// 返回类型
	zend_type type = return_info->type;
	// 如果是有效的type，参见zend_types.h
	if (ZEND_TYPE_IS_SET(type)) {
		zend_op *opline;

		// 在一个反回类型为void的function里，return ...是非法的。return; 是合法的
		/* `return ...;` is illegal in a void function (but `return;` isn't) */
		// 如果返回类型是void
		if (ZEND_TYPE_CONTAINS_CODE(type, IS_VOID)) {
			// 如果有传入表达式结果
			if (expr) {
				// 如果类型是常量，值为null。（这个报错只是为了更友好一些）
				if (expr->op_type == IS_CONST && Z_TYPE(expr->u.constant) == IS_NULL) {
					// 返回值void类型的方法，不可以返回null （测试过）
					zend_error_noreturn(E_COMPILE_ERROR,
						"A void function must not return a value "
						"(did you mean \"return;\" instead of \"return null;\"?)");
				// 其他情况，报错，void类型不可以返回任何值
				} else {
					zend_error_noreturn(E_COMPILE_ERROR, "A void function must not return a value");
				}
			}
			/* we don't need run-time check */
			return;
		}

		/* `return` is illegal in a never-returning function */
		// 如果返回类型是 IS_NEVER,那么不可以有return 语句 （测试过）
		if (ZEND_TYPE_CONTAINS_CODE(type, IS_NEVER)) {
			// 这里不可以是间接类型，它在 VERIFY_NEVER_TYPE 类型中单独处理了
			/* Implicit case handled separately using VERIFY_NEVER_TYPE opcode. */
			ZEND_ASSERT(!implicit);
			// 报错
			zend_error_noreturn(E_COMPILE_ERROR, "A never-returning function must not return");
			return;
		}

		// 如果没有要求返回结果，也不是间接引用
		if (!expr && !implicit) {
			if (ZEND_TYPE_ALLOW_NULL(type)) {
				zend_error_noreturn(E_COMPILE_ERROR,
					"A function with return type must return a value "
					"(did you mean \"return null;\" instead of \"return;\"?)");
			} else {
				zend_error_noreturn(E_COMPILE_ERROR,
					"A function with return type must return a value");
			}
		}
		// 如果返回结果可以是任何类型，中断 （应该是不写类型就是any？）
		if (expr && ZEND_TYPE_PURE_MASK(type) == MAY_BE_ANY) {
			/* we don't need run-time check for mixed return type */
			return;
		}

		// 如果返回类型是常量，且类型与结果的类型一致（ 这里是有要求类型并且检查通过）
		if (expr && expr->op_type == IS_CONST && ZEND_TYPE_CONTAINS_CODE(type, Z_TYPE(expr->u.constant))) {
			// 但这里要求 表达式在编译阶段就运算完成了
			/* we don't need run-time check */
			return;
		}

		// 创建 ZEND_VERIFY_RETURN_TYPE 操作码
		opline = zend_emit_op(NULL, ZEND_VERIFY_RETURN_TYPE, expr, NULL);
		// 如果有返回类型，并且类型是常量
		if (expr && expr->op_type == IS_CONST) {
			// 返回类型改成 临时变量
			opline->result_type = expr->op_type = IS_TMP_VAR;
			// 增加变量计数
			opline->result.var = expr->u.op.var = get_temporary_variable();
		}
		// 给 CG(active_op_array) 添加缓存大小，添加 count 个void*指针的大小, 返回缓存大小
		opline->op2.num = zend_alloc_cache_slots(zend_type_get_num_classes(return_info->type));
	}
}
/* }}} */

// ing3, 创建 ZEND_RETURN (返回) 操作码，只有 zend_compile_func_decl和, /Zend/zend_language_scanner.l 用到
void zend_emit_final_return(bool return_one) /* {{{ */
{
	znode zn;
	zend_op *ret;
	// 返回引用： 是否包含 ZEND_ACC_RETURN_REFERENCE 标记
	bool returns_reference = (CG(active_op_array)->fn_flags & ZEND_ACC_RETURN_REFERENCE) != 0;

	// 如果有 ZEND_ACC_RETURN_REFERENCE 标记，并且有生成器标记
	if ((CG(active_op_array)->fn_flags & ZEND_ACC_HAS_RETURN_TYPE)
			// 是否在 yield 语句中
			&& !(CG(active_op_array)->fn_flags & ZEND_ACC_GENERATOR)) {
		// 返回类型信息
		zend_arg_info *return_info = CG(active_op_array)->arg_info - 1;
		// 如果返回类型包含 IS_NEVER（不返回）
		if (ZEND_TYPE_CONTAINS_CODE(return_info->type, IS_NEVER)) {
			// 创建操作码 ZEND_VERIFY_NEVER_TYPE
			zend_emit_op(NULL, ZEND_VERIFY_NEVER_TYPE, NULL, NULL);
			return;
		}

		// 检查函数的返回类型（可能创建操作码）
		zend_emit_return_type_check(NULL, return_info, 1);
	}

	// 返回结果标记成常量
	zn.op_type = IS_CONST;
	// 如果需要返回 1
	if (return_one) {
		// 结果结点的值为 1
		ZVAL_LONG(&zn.u.constant, 1);
	// 否则返回 null
	} else {
		// 结果结点的值为 null
		ZVAL_NULL(&zn.u.constant);
	}
	// 创建 RETURN 操作码 , 返回引用或返回副本
	ret = zend_emit_op(NULL, returns_reference ? ZEND_RETURN_BY_REF : ZEND_RETURN, &zn, NULL);
	// 扩展信息
	ret->extended_value = -1;
}
/* }}} */

// ing4, 是否是变量：php变量，数组元素，对象属性，允空对象属性，静态属性
static inline bool zend_is_variable(zend_ast *ast) /* {{{ */
{
	// 是否是一下情况之一：php变量，数组元素，对象属性，允空对象属性，静态属性
	return ast->kind == ZEND_AST_VAR
		|| ast->kind == ZEND_AST_DIM
		|| ast->kind == ZEND_AST_PROP
		|| ast->kind == ZEND_AST_NULLSAFE_PROP
		|| ast->kind == ZEND_AST_STATIC_PROP;
}
/* }}} */

// ing4, 语句是否是方法调用语句：调用函数，调用成员方法，调用允空成员方法，调用静态成员方法
static inline bool zend_is_call(zend_ast *ast) /* {{{ */
{
	// 是否是一下情况之一：调用函数，调用成员方法，调用允空成员方法，调用静态成员方法
	return ast->kind == ZEND_AST_CALL
		|| ast->kind == ZEND_AST_METHOD_CALL
		|| ast->kind == ZEND_AST_NULLSAFE_METHOD_CALL
		|| ast->kind == ZEND_AST_STATIC_CALL;
}
/* }}} */

// ing4, 是否是变量或函数调用
static inline bool zend_is_variable_or_call(zend_ast *ast) /* {{{ */
{
	// 如果是变量语句或方法调用语句。zend_is_variable：是否是变量：php变量，数组元素，对象属性，允空对象属性，静态属性
	return zend_is_variable(ast) || zend_is_call(ast);
}
/* }}} */

// ing3, 是否是未标记的语句：语句块，挑转标签，类属性定义，类常量定义，引用trait，类方法定义
static inline bool zend_is_unticked_stmt(zend_ast *ast) /* {{{ */
{
	return ast->kind == ZEND_AST_STMT_LIST || ast->kind == ZEND_AST_LABEL
		|| ast->kind == ZEND_AST_PROP_DECL || ast->kind == ZEND_AST_CLASS_CONST_GROUP
		|| ast->kind == ZEND_AST_USE_TRAIT || ast->kind == ZEND_AST_METHOD;
}
/* }}} */

// ing3, 是否可写入：是变量或方法调用 && 没有短路。 如果是调用方法,可能语句链中有写入语句。
static inline bool zend_can_write_to_variable(zend_ast *ast) /* {{{ */
{
	// 如果是调用数组元素或对象属性，转移到父对象
	while (
		ast->kind == ZEND_AST_DIM
		|| ast->kind == ZEND_AST_PROP
	) {
		ast = ast->child[0];
	}
	// 如果是通用变量或函数调用，并且不短路
	return zend_is_variable_or_call(ast) && !zend_ast_is_short_circuited(ast);
}
/* }}} */

// ing3, 是否正在引用 self,parent,static 以外的普通类名，只有 zend_compile_try 用到
static inline bool zend_is_const_default_class_ref(zend_ast *name_ast) /* {{{ */
{
	// 如果不是特殊语句，是在parse时创建的普通语句，直接返回 false
	if (name_ast->kind != ZEND_AST_ZVAL) {
		return 0;
	}
	
	// 是否是 self,parent,static以外的普通类名
	return ZEND_FETCH_CLASS_DEFAULT == zend_get_class_fetch_type_ast(name_ast);
}
/* }}} */

// ing4，把数字字串的数组元素名转成整数， zend_compile_array 一次调用
static inline void zend_handle_numeric_op(znode *node) /* {{{ */
{
	// 如果结果类型是 常量 并且是 字串
	if (node->op_type == IS_CONST && Z_TYPE(node->u.constant) == IS_STRING) {
		zend_ulong index;
		// #define ZEND_HANDLE_NUMERIC(key, idx) ZEND_HANDLE_NUMERIC_STR(ZSTR_VAL(key), ZSTR_LEN(key), idx)
		// 如果是数字字串
		if (ZEND_HANDLE_NUMERIC(Z_STR(node->u.constant), index)) {
			// 销毁结果中的常量
			zval_ptr_dtor(&node->u.constant);
			// index 存放到结果的常量中
			ZVAL_LONG(&node->u.constant, index);
		}
	}
}
/* }}} */

// ing3, 处理数字字串类型的数组索引
static inline void zend_handle_numeric_dim(zend_op *opline, znode *dim_node) /* {{{ */
{
	// 如果元素名是字串
	if (Z_TYPE(dim_node->u.constant) == IS_STRING) {
		zend_ulong index;
		// 元素名可以转成整数 
		if (ZEND_HANDLE_NUMERIC(Z_STR(dim_node->u.constant), index)) {
			// 保持原始值
			/* For numeric indexes we also keep the original value to use by ArrayAccess
			 * See bug #63217
			 */
			// 添加 literal
			int c = zend_add_literal(&dim_node->u.constant);
			// ？
			ZEND_ASSERT(opline->op2.constant + 1 == c);
			// 元素名转成整数
			ZVAL_LONG(CT_CONSTANT(opline->op2), index);
			// 添加扩展标记 ZEND_EXTRA_VALUE
			Z_EXTRA_P(CT_CONSTANT(opline->op2)) = ZEND_EXTRA_VALUE;
			return;
		}
	}
}
/* }}} */

// ing3, 把类名放到 opline->op1 里，根据 class_node 类型，分成两种情况
static inline void zend_set_class_name_op1(zend_op *opline, znode *class_node) /* {{{ */
{
	// 如果节点是常量
	if (class_node->op_type == IS_CONST) {
		// 操作码类型是常量
		opline->op1_type = IS_CONST;
		// 把类名添加到literals并且把返回的序号存到 opline->op1.constant 里面
		opline->op1.constant = zend_add_class_name_literal(
			Z_STR(class_node->u.constant));
	// 否则 
	} else {
		// 把znode的值 传给 znode_op，复制 znode_op 对象，不是指针
		// target 是opline的子元素 znode_op, src是znode的指针
		SET_NODE(opline->op1, class_node);
	}
}
/* }}} */

// ing3, 编译类引用，返回类名，5处调用
static void zend_compile_class_ref(znode *result, zend_ast *name_ast, uint32_t fetch_flags) /* {{{ */
{
	uint32_t fetch_type;
	// 如果类名不是内置变量
	if (name_ast->kind != ZEND_AST_ZVAL) {
		znode name_node;
		// 编译表达式语句, p1:返回结果，p2:表达式语句
		zend_compile_expr(&name_node, name_ast);
		// 类名编译结果是常量
		if (name_node.op_type == IS_CONST) {
			zend_string *name;
			// 如果编译结果不是 string, 抛错， 类名非法
			if (Z_TYPE(name_node.u.constant) != IS_STRING) {
				// 测试？
				zend_error_noreturn(E_COMPILE_ERROR, "Illegal class name");
			}

			// define Z_STR(zval) (zval).value.str
			// 获取类名
			name = Z_STR(name_node.u.constant);
			// 通过类名获取类的引用方式，self,parent,static 或其他(ZEND_FETCH_CLASS_DEFAULT)
			fetch_type = zend_get_class_fetch_type(name);

			// 通过类名引用
			if (fetch_type == ZEND_FETCH_CLASS_DEFAULT) {
				// 返回结果类型是常量，值为完整的类名
				result->op_type = IS_CONST;
				ZVAL_STR(&result->u.constant, zend_resolve_class_name(name, ZEND_NAME_FQ));
			// 通过 魔术常量 self,parent,static 引用
			} else {
				// 保证调用类的方式有效, self,parent,static 没有用错
				zend_ensure_valid_class_fetch_type(fetch_type);
				// 返回结果类型为 为null
				result->op_type = IS_UNUSED;
				// 添加标记  fetch_type | fetch_flags
				result->u.op.num = fetch_type | fetch_flags;
			}
			// 销毁类名
			zend_string_release_ex(name, 0);
		// 类名编译结果不是常量
		} else {
			// 创建操作码 ZEND_FETCH_CLASS，
			zend_op *opline = zend_emit_op(result, ZEND_FETCH_CLASS, NULL, &name_node);
			// 添加标记 fetch_flags
			opline->op1.num = ZEND_FETCH_CLASS_DEFAULT | fetch_flags;
		}
		// 
		return;
	}
	// 类名是内置变量才能到这里

	// 如果是完整路径引用
	/* Fully qualified names are always default refs */
	if (name_ast->attr == ZEND_NAME_FQ) {
		// 返回结果类型为常量，值为完整的类名
		result->op_type = IS_CONST;
		// 通过语句获取完整类名
		ZVAL_STR(&result->u.constant, zend_resolve_class_name_ast(name_ast));
		return;
	}
	
	// 通过类名获取类的引用方式，self,parent,static 或其他(ZEND_FETCH_CLASS_DEFAULT)
	fetch_type = zend_get_class_fetch_type(zend_ast_get_str(name_ast));
	// 按引用类型返回，与前面业务逻辑相同
	if (ZEND_FETCH_CLASS_DEFAULT == fetch_type) {
		result->op_type = IS_CONST;
		// 通过语句获取完整类名
		ZVAL_STR(&result->u.constant, zend_resolve_class_name_ast(name_ast));
	} else {
		// 保证调用类的方式有效, self,parent,static 没有用错
		zend_ensure_valid_class_fetch_type(fetch_type);
		// 结果类型是 IS_UNUSED
		result->op_type = IS_UNUSED;
		// 添加 fetch_flags
		result->u.op.num = fetch_type | fetch_flags;
	}
}
/* }}} */

// ing3, 编译变量，变量名必须是内置变量, 
// 关键步骤是1：用变量名创建保留字。2：lookup_cv 创建编译变量
static zend_result zend_try_compile_cv(znode *result, zend_ast *ast) /* {{{ */
{
	// 取得第一个子语句，变量名
	zend_ast *name_ast = ast->child[0];
	// 变量名必须是内置变量
	if (name_ast->kind == ZEND_AST_ZVAL) {
		// 直接获取语句里的变量
		zval *zv = zend_ast_get_zval(name_ast);
		zend_string *name;

		// 从变量 zv 获取 变量名 name
		// 如果变量类型是 string，创建保留字 zv
		if (EXPECTED(Z_TYPE_P(zv) == IS_STRING)) {
			// 创建内置变量
			name = zval_make_interned_string(zv);
		// 否则
		} else {
			// 从zv中获取字串，并创建保留字
			name = zend_new_interned_string(zval_get_string_func(zv));
		}

		// 如果变量名属于自动全局变量_GET,_POST等，报错
		if (zend_is_auto_global(name)) {
			return FAILURE;
		}

		// 结果类型标记成编译变量
		result->op_type = IS_CV;
		// 查找编译变量，如果没有的话创建一个新的并返回序号
		result->u.op.var = lookup_cv(name);

		// 如果类不型是 string
		if (UNEXPECTED(Z_TYPE_P(zv) != IS_STRING)) {
			// 释放变量名
			zend_string_release_ex(name, 0);
		}

		return SUCCESS;
	}

	return FAILURE;
}
/* }}} */

// ing3, 编译简单变量，不使用CV。大部分时候是在 zend_try_compile_cv 失败时调用 
static zend_op *zend_compile_simple_var_no_cv(znode *result, zend_ast *ast, uint32_t type, bool delayed) /* {{{ */
{
	// 第一个子语句，变量名
	zend_ast *name_ast = ast->child[0];
	znode name_node;
	zend_op *opline;

	// 编译表达式语句, p1:返回结果，p2:表达式语句
	zend_compile_expr(&name_node, name_ast);
	// 如果类型是 常量
	if (name_node.op_type == IS_CONST) {
		// 转成string
		convert_to_string(&name_node.u.constant);
	}

	// 如果要求延时，创建延时操作码
	if (delayed) {
		opline = zend_delayed_emit_op(result, ZEND_FETCH_R, &name_node, NULL);
	// 否则，创建普通操作码
	} else {
		opline = zend_emit_op(result, ZEND_FETCH_R, &name_node, NULL);
	}

	// 如果变量名类型是常量 并且 是自动全局变量 _GET,_POST 等
	if (name_node.op_type == IS_CONST &&
	    zend_is_auto_global(Z_STR(name_node.u.constant))) {
		// 操作码标记成 从全局变量里获取
		opline->extended_value = ZEND_FETCH_GLOBAL;
	} else {
		// 操作码标记成 从本地变量里获取
		opline->extended_value = ZEND_FETCH_LOCAL;
	}

	// 按变量的操作类型给opline适配操作码
	zend_adjust_for_fetch_type(opline, result, type);
	return opline;
}
/* }}} */

// ing3，检查变量名是否是 $this
static bool is_this_fetch(zend_ast *ast) /* {{{ */
{
	// 如果是php变量，并且第一个子元素是内部变量
	if (ast->kind == ZEND_AST_VAR && ast->child[0]->kind == ZEND_AST_ZVAL) {
		// 获取变量名
		zval *name = zend_ast_get_zval(ast->child[0]);
		// 如果变量名是string 并且值为 this
		return Z_TYPE_P(name) == IS_STRING && zend_string_equals_literal(Z_STR_P(name), "this");
	}

	return 0;
}
/* }}} */

// ing3，检查变量名是否是 $GLOBALS
static bool is_globals_fetch(const zend_ast *ast)
{
	// 逻辑同上
	if (ast->kind == ZEND_AST_VAR && ast->child[0]->kind == ZEND_AST_ZVAL) {
		zval *name = zend_ast_get_zval(ast->child[0]);
		return Z_TYPE_P(name) == IS_STRING && zend_string_equals_literal(Z_STR_P(name), "GLOBALS");
	}

	return 0;
}

// ing3, 是否从$GLOBALS中获取变量
static bool is_global_var_fetch(zend_ast *ast)
{
	// 必须用 $GLOBALS[] 格式
	return ast->kind == ZEND_AST_DIM && is_globals_fetch(ast->child[0]);
}

// ing3, 能否保证 $this 存在
static bool this_guaranteed_exists(void) /* {{{ */
{
	zend_op_array *op_array = CG(active_op_array);
	
	// 实例的成员方法里总是包含$this. 包括在类中定义的闭包。
	/* Instance methods always have a $this.
	 * This also includes closures that have a scope and use $this. */
	// 有作用域，并且 没有静态访问标记 ZEND_ACC_STATIC
	return op_array->scope != NULL
		&& (op_array->fn_flags & ZEND_ACC_STATIC) == 0;
}
/* }}} */

// ing2, 编译简单变量
static zend_op *zend_compile_simple_var(znode *result, zend_ast *ast, uint32_t type, bool delayed) /* {{{ */
{
	// 从 $this 里获取
	if (is_this_fetch(ast)) {
		// 创建操作码：ZEND_FETCH_THIS
		zend_op *opline = zend_emit_op(result, ZEND_FETCH_THIS, NULL, NULL);
		// ?? 操作码和结果都标记成 临时变量
		if ((type == BP_VAR_R) || (type == BP_VAR_IS)) {
			opline->result_type = IS_TMP_VAR;
			result->op_type = IS_TMP_VAR;
		}
		// 给当前操作码集合，添加 $this 标记
		CG(active_op_array)->fn_flags |= ZEND_ACC_USES_THIS;
		return opline;
	// 如果从 $GLOBALS 里获取元素
	} else if (is_globals_fetch(ast)) {
		// 创建操作码：ZEND_FETCH_THIS
		zend_op *opline = zend_emit_op(result, ZEND_FETCH_GLOBALS, NULL, NULL);
		// ?? 操作码和结果都标记成 临时变量
		if (type == BP_VAR_R || type == BP_VAR_IS) {
			opline->result_type = IS_TMP_VAR;
			result->op_type = IS_TMP_VAR;
		}
		return opline;
	// 其他情况，如果尝试获取编译变量失败
	} else if (zend_try_compile_cv(result, ast) == FAILURE) {
		// 编译简单变量
		return zend_compile_simple_var_no_cv(result, ast, type, delayed);
	}
	// 其他情况返回null
	return NULL;
}
/* }}} */

// ing3, 如果是写入或者调用方法的话，需要插入一个 ZEND_SEPARATE 操作码
static void zend_separate_if_call_and_write(znode *node, zend_ast *ast, uint32_t type) /* {{{ */
{
	// 如果不是写入变量，也不是创建变量 BP_VAR_IS，也不是函数调用语句。需要添加一个 ZEND_SEPARATE
	if (type != BP_VAR_R && type != BP_VAR_IS && zend_is_call(ast)) {
		// 如果操作码类型是变量
		if (node->op_type == IS_VAR) {
			// 创建 ZEND_SEPARATE 操作码
			zend_op *opline = zend_emit_op(NULL, ZEND_SEPARATE, node, NULL);
			// 返回结果类型是变量
			opline->result_type = IS_VAR;
			// 值为 opline->op1.var
			opline->result.var = opline->op1.var;
		// 否则报错
		} else {
			zend_error_noreturn(E_COMPILE_ERROR, "Cannot use result of built-in function in write context");
		}
	}
}
/* }}} */

// ing3, 执行赋值：创建临时赋值语句计算 value_node 的结果，把结果存入 var_ast 用
static inline void zend_emit_assign_znode(zend_ast *var_ast, znode *value_node) /* {{{ */
{
	// 虚拟节点
	znode dummy_node;
	// 创建 ZEND_AST_ASSIGN 语句， var_ast 是接收结果用的语句
	zend_ast *assign_ast = zend_ast_create(ZEND_AST_ASSIGN, var_ast,
		zend_ast_create_znode(value_node));
	// 编译表达式语句, p1:返回结果，p2:表达式语句
	zend_compile_expr(&dummy_node, assign_ast);
	// 销毁临时节点
	zend_do_free(&dummy_node);
}
/* }}} */

// ing3, 延时编译，获取数组元素。获取数组元素要在变量或表达式被彻底执行完，所以要延时编译
static zend_op *zend_delayed_compile_dim(znode *result, zend_ast *ast, uint32_t type, bool by_ref)
{
	// # {}变量风格 $a{} 已彻底弃用
	if (ast->attr == ZEND_DIM_ALTERNATIVE_SYNTAX) {
		zend_error(E_COMPILE_ERROR, "Array and string offset access syntax with curly braces is no longer supported");
	}
	// 变量名
	zend_ast *var_ast = ast->child[0];
	// 元素键名
	zend_ast *dim_ast = ast->child[1];
	// 操作码
	zend_op *opline;

	znode var_node, dim_node;

	// 如果是获取全局变量
	if (is_globals_fetch(var_ast)) {
		// $GLOBALS[] = 1; 不可以给 $GLOBALS 追加元素（测试过）
		if (dim_ast == NULL) {
			zend_error_noreturn(E_COMPILE_ERROR, "Cannot append to $GLOBALS");
		}
		// 编译表达式语句, p1:返回结果，p2:表达式语句
		zend_compile_expr(&dim_node, dim_ast);
		// 如果结果是常量，转成字串
		if (dim_node.op_type == IS_CONST) {
			convert_to_string(&dim_node.u.constant);
		}
		// 创建延时操作码 ZEND_FETCH_R
		opline = zend_delayed_emit_op(result, ZEND_FETCH_R, &dim_node, NULL);
		// 添加标记，从全局变量获取
		opline->extended_value = ZEND_FETCH_GLOBAL;
		// 按变量的操作类型给opline适配操作码
		zend_adjust_for_fetch_type(opline, result, type);
		return opline;
	// 不是全局变量
	} else {
		// 如果是可以短路的类型， 添加短路标记
		zend_short_circuiting_mark_inner(var_ast);
		// 延时编译变量 var_ast
		opline = zend_delayed_compile_var(&var_node, var_ast, type, 0);
		// 如果有返回操作码
		if (opline) {
			// ？如果是变量写入，并且 目标是 静态成员变量或写入成员变量
			if (type == BP_VAR_W && (opline->opcode == ZEND_FETCH_STATIC_PROP_W || opline->opcode == ZEND_FETCH_OBJ_W)) {
				// 添加操作码扩展信息 ZEND_FETCH_DIM_WRITE
				opline->extended_value |= ZEND_FETCH_DIM_WRITE;
			// 其他情况，如果以下4个操作码
			} else if (opline->opcode == ZEND_FETCH_DIM_W
					|| opline->opcode == ZEND_FETCH_DIM_RW
					|| opline->opcode == ZEND_FETCH_DIM_FUNC_ARG
					|| opline->opcode == ZEND_FETCH_DIM_UNSET) {
				// 添加扩展信息 ZEND_FETCH_DIM_DIM
				opline->extended_value = ZEND_FETCH_DIM_DIM;
			}
		}
	}
	// 如果是写入或者调用方法的话，需要插入一个 ZEND_SEPARATE 操作码
	zend_separate_if_call_and_write(&var_node, var_ast, type);

	// 如果没有元素名
	if (dim_ast == NULL) {
		// ？如果是读取或者创建元素？ BP_VAR_IS?
		if (type == BP_VAR_R || type == BP_VAR_IS) {
			zend_error_noreturn(E_COMPILE_ERROR, "Cannot use [] for reading");
		}
		// 如果是删除元素
		if (type == BP_VAR_UNSET) {
			zend_error_noreturn(E_COMPILE_ERROR, "Cannot use [] for unsetting");
		}
		// 元素名是null (添加 IS_UNUSED 标记)
		dim_node.op_type = IS_UNUSED;
	// 如有元素名
	} else {
		// 编译表达式语句, p1:返回结果，p2:表达式语句
		zend_compile_expr(&dim_node, dim_ast);
	}

	// 创建延时操作码 ZEND_FETCH_DIM_R
	opline = zend_delayed_emit_op(result, ZEND_FETCH_DIM_R, &var_node, &dim_node);
	// 按变量的操作类型给opline适配操作码
	zend_adjust_for_fetch_type(opline, result, type);
	// 如果要求返回地址
	if (by_ref) {
		// 操作码添加获取地址标记
		opline->extended_value = ZEND_FETCH_DIM_REF;
	}

	// 如果键名是常量
	if (dim_node.op_type == IS_CONST) {
		// 处理数字字串类型的数组索引
		zend_handle_numeric_dim(opline, &dim_node);
	}
	// 返回操作码
	return opline;
}

// ing3, 编译：获取数组元素
static zend_op *zend_compile_dim(znode *result, zend_ast *ast, uint32_t type, bool by_ref) /* {{{ */
{
	// 开始延时编译
	uint32_t offset = zend_delayed_compile_begin();
	// 创建获取数组元素的延时操作码
	zend_delayed_compile_dim(result, ast, type, by_ref);
	// 结束延时编译
	return zend_delayed_compile_end(offset);
}
/* }}} */

// ing1, 延时编译属性
static zend_op *zend_delayed_compile_prop(znode *result, zend_ast *ast, uint32_t type) /* {{{ */
{
	// 对象语句
	zend_ast *obj_ast = ast->child[0];
	// 属性名语句
	zend_ast *prop_ast = ast->child[1];

	znode obj_node, prop_node;
	zend_op *opline;
	// 短路标记
	bool nullsafe = ast->kind == ZEND_AST_NULLSAFE_PROP;

	// 如果通过 $this 操作
	if (is_this_fetch(obj_ast)) {
		// 如果能保证 $this 存在
		if (this_guaranteed_exists()) {
			// 结果类型是 IS_UNUSED
			obj_node.op_type = IS_UNUSED;
		} else {
			// 否则创建操作码 ZEND_FETCH_THIS
			zend_emit_op(&obj_node, ZEND_FETCH_THIS, NULL, NULL);
		}
		// 操作码列表添加标记 ZEND_ACC_USES_THIS（使用了this）
		CG(active_op_array)->fn_flags |= ZEND_ACC_USES_THIS;

		/* We will throw if $this doesn't exist, so there's no need to emit a JMP_NULL
		 * check for a nullsafe access. */
	// 其他情况
	} else {
		// 如果是可以短路的类型， 添加短路标记
		zend_short_circuiting_mark_inner(obj_ast);
		// 延时编译 对象子句
		opline = zend_delayed_compile_var(&obj_node, obj_ast, type, 0);
		// 如果操作码类型是 数组元素操作
		if (opline && (opline->opcode == ZEND_FETCH_DIM_W
				|| opline->opcode == ZEND_FETCH_DIM_RW
				|| opline->opcode == ZEND_FETCH_DIM_FUNC_ARG
				|| opline->opcode == ZEND_FETCH_DIM_UNSET)) {
			// 添加标记 ZEND_FETCH_DIM_OBJ，从数组中获取对象
			opline->extended_value = ZEND_FETCH_DIM_OBJ;
		}
		// 如果是写入或者调用方法的话，需要插入一个 ZEND_SEPARATE 操作码
		zend_separate_if_call_and_write(&obj_node, obj_ast, type);
		// 如果是短路调用
		if (nullsafe) {
			// 对象类型是临时变量
			if (obj_node.op_type == IS_TMP_VAR) {
				// 清空延时操作码列表
				/* Flush delayed oplines */
				// 取得延时操作码堆栈元素的开头位置
				zend_op *opline = NULL, *oplines = zend_stack_base(&CG(delayed_oplines_stack));
				uint32_t var = obj_node.u.op.var;
				// 延时操作码堆栈深度
				uint32_t count = zend_stack_count(&CG(delayed_oplines_stack));
				uint32_t i = count;

				// 这是在做什么？
				// 倒着遍历所有 类型是临时变量、结果值是 var 的操作码
				while (i > 0 && oplines[i-1].result_type == IS_TMP_VAR && oplines[i-1].result.var == var) {
					i--;
					// 如果第一的操作对象类型是临时变量
					if (oplines[i].op1_type == IS_TMP_VAR) {
						// 取回第一个操作对象里的变量
						var = oplines[i].op1.var;
					} else {
						break;
					}
				}
				// 
				for (; i < count; ++i) {
					if (oplines[i].opcode != ZEND_NOP) {
						opline = get_next_op();
						memcpy(opline, &oplines[i], sizeof(zend_op));
						oplines[i].opcode = ZEND_NOP;
						oplines[i].extended_value = opline - CG(active_op_array)->opcodes;
					}
				}
			}
			// ing3, 创建 ZEND_JMP_NULL 操作码
			zend_emit_jmp_null(&obj_node, type);
		}
	}

	// 编译表达式语句, p1:返回结果，p2:表达式语句
	zend_compile_expr(&prop_node, prop_ast);

	// 创建延时操作码 ZEND_FETCH_OBJ_R
	opline = zend_delayed_emit_op(result, ZEND_FETCH_OBJ_R, &obj_node, &prop_node);
	// 如果操作码第二个操作对象类型是常量
	if (opline->op2_type == IS_CONST) {
		// 常量转成字串
		convert_to_string(CT_CONSTANT(opline->op2));
		// 创建哈希值
		zend_string_hash_val(Z_STR_P(CT_CONSTANT(opline->op2)));
		// 给 CG(active_op_array) 添加缓存大小，添加 count 个void*指针的大小, 返回缓存大小
		opline->extended_value = zend_alloc_cache_slots(3);
	}
	// 按变量的操作类型给opline适配操作码
	zend_adjust_for_fetch_type(opline, result, type);

	return opline;
}
/* }}} */

// ing3, 编译类属性
static zend_op *zend_compile_prop(znode *result, zend_ast *ast, uint32_t type, bool by_ref) /* {{{ */
{
	// 开始延时编译
	uint32_t offset = zend_delayed_compile_begin();
	// 延时编译类属性
	zend_op *opline = zend_delayed_compile_prop(result, ast, type);
	// 如果是引用，共享 cache_slot 
	if (by_ref) { /* shared with cache_slot */
		// 扩展信息 ZEND_FETCH_REF
		opline->extended_value |= ZEND_FETCH_REF;
	}
	// 结束延时编译
	return zend_delayed_compile_end(offset);
}
/* }}} */

// ing3, 编译静态属性
static zend_op *zend_compile_static_prop(znode *result, zend_ast *ast, uint32_t type, bool by_ref, bool delayed) /* {{{ */
{
	// 类名语句
	zend_ast *class_ast = ast->child[0];
	// 属性名语句
	zend_ast *prop_ast = ast->child[1];

	znode class_node, prop_node;
	zend_op *opline;
	// 如果是可以短路的类型， 添加短路标记
	zend_short_circuiting_mark_inner(class_ast);
	// 编译类引用
	zend_compile_class_ref(&class_node, class_ast, ZEND_FETCH_CLASS_EXCEPTION);

	// 编译表达式语句, p1:返回结果，p2:表达式语句
	zend_compile_expr(&prop_node, prop_ast);

	// 如果要求延时
	if (delayed) {
		// 创建延时操作码 ZEND_FETCH_STATIC_PROP_R
		opline = zend_delayed_emit_op(result, ZEND_FETCH_STATIC_PROP_R, &prop_node, NULL);
	} else {
		// 创建操作码 ZEND_FETCH_STATIC_PROP_R
		opline = zend_emit_op(result, ZEND_FETCH_STATIC_PROP_R, &prop_node, NULL);
	}
	// 如果操作码第一个操作对象是常量
	if (opline->op1_type == IS_CONST) {
		// 把第一个操作对象的常量转成 字串
		convert_to_string(CT_CONSTANT(opline->op1));
		// 给 CG(active_op_array) 添加缓存大小，添加 count 个void*指针的大小, 返回缓存大小
		opline->extended_value = zend_alloc_cache_slots(3);
	}
	// 如果类名是常量
	if (class_node.op_type == IS_CONST) {
		// 把类名放在操作码的第二个操作对象里
		opline->op2_type = IS_CONST;
		opline->op2.constant = zend_add_class_name_literal(
			Z_STR(class_node.u.constant));
		if (opline->op1_type != IS_CONST) {
			// 给 CG(active_op_array) 添加缓存大小，添加 一个 void*指针的大小，返回添加后的缓存大小
			opline->extended_value = zend_alloc_cache_slot();
		}
	} else {
		SET_NODE(opline->op2, &class_node);
	}

	// 如果是地址引用，并且类型是写入 或 参数列表？
	if (by_ref && (type == BP_VAR_W || type == BP_VAR_FUNC_ARG)) { /* shared with cache_slot */
		// 添加地址引用标记
		opline->extended_value |= ZEND_FETCH_REF;
	}
	// 按变量的操作类型给opline适配操作码
	zend_adjust_for_fetch_type(opline, result, type);
	return opline;
}
/* }}} */

// ing3, 检查数组赋值目标变量
static void zend_verify_list_assign_target(zend_ast *var_ast, zend_ast_attr array_style) /* {{{ */ {
	// 如果目标变量是 键值对
	if (var_ast->kind == ZEND_AST_ARRAY) {
		// array() 风格
		if (var_ast->attr == ZEND_ARRAY_SYNTAX_LONG) {
			// 不可以给 array() 赋值。（测试过：[array($a)] = [2];）
			zend_error_noreturn(E_COMPILE_ERROR, "Cannot assign to array(), use [] instead");
		}
		// 如果两边风格不一致 (测试过 [list($a)] = [[2]];）
		if (array_style != var_ast->attr) {
			zend_error_noreturn(E_COMPILE_ERROR, "Cannot mix [] and list()");
		}
	// 检查变量是否可写，只能给可写变量赋值 （测试过 [1] = [2];）
	} else if (!zend_can_write_to_variable(var_ast)) {
		zend_error_noreturn(E_COMPILE_ERROR, "Assignments can only happen to writable values");
	}
}
/* }}} */

// 
static inline void zend_emit_assign_ref_znode(zend_ast *var_ast, znode *value_node);

// 传播查找引用
/* Propagate refs used on leaf elements to the surrounding list() structures. */
// ing4, 递归到叶子节点，检查 elem_ast->attr 看是否包含引用
static bool zend_propagate_list_refs(zend_ast *ast) { /* {{{ */
	// 语句列表
	zend_ast_list *list = zend_ast_get_list(ast);
	bool has_refs = 0;
	uint32_t i;
	// 遍历语句列表
	for (i = 0; i < list->children; ++i) {
		zend_ast *elem_ast = list->child[i];
		// 如果元素有效
		if (elem_ast) {
			zend_ast *var_ast = elem_ast->child[0];
			// 如果类型是数组键值对
			if (var_ast->kind == ZEND_AST_ARRAY) {
				// 递归到里层，看是否包含引用
				elem_ast->attr = zend_propagate_list_refs(var_ast);
			}
			// 继承里层节点的属性
			has_refs |= elem_ast->attr;
		}
	}
	// 返回结果 
	return has_refs;
}
/* }}} */

// ing4, 判断赋值语句是否是键值对
static bool list_is_keyed(zend_ast_list *list)
{
	// 遍历所有子语句
	for (uint32_t i = 0; i < list->children; i++) {
		zend_ast *child = list->child[i];
		// 子语句有效
		if (child) {
			// 如果第一个有效子语句，【是数组元素】，并且有 key
			return child->kind == ZEND_AST_ARRAY_ELEM && child->child[1] != NULL;
		}
	}
	// 不是键值对
	return false;
}

// ing2, 编译给列表赋值，被赋值变量是列表（结果，变量，值，风格）
static void zend_compile_list_assign(
		znode *result, zend_ast *ast, znode *expr_node, zend_ast_attr array_style) /* {{{ */
{
	// 被赋值语句列表
	zend_ast_list *list = zend_ast_get_list(ast);
	uint32_t i;
	bool has_elems = 0;
	// 赋值语句是否是数组的键值对
	bool is_keyed = list_is_keyed(list);

	// 如果有子句 并且 值类型是常量 并且 值为string
	if (list->children && expr_node->op_type == IS_CONST && Z_TYPE(expr_node->u.constant) == IS_STRING) {
		// 创建保留字
		zval_make_interned_string(&expr_node->u.constant);
	}

	// 遍历所有被赋值语句
	for (i = 0; i < list->children; ++i) {
		zend_ast *elem_ast = list->child[i];
		zend_ast *var_ast, *key_ast;
		znode fetch_result, dim_node;
		zend_op *opline;

		// 被赋值语句为空，不是Null, (测试过 $v = 2;[,1=>$v] = [];）
		if (elem_ast == NULL) {
			// 如果是键值对赋值，语句不可以是null ?
			if (is_keyed) {
				zend_error(E_COMPILE_ERROR,
					"Cannot use empty array entries in keyed array assignment");
			// 跳过空语句
			} else {
				continue;
			}
		}

		// 被赋值语句是打包参数 (测试过 $v = 2;[...$v] = [];）
		if (elem_ast->kind == ZEND_AST_UNPACK) {
			// 报错 不可以用打包语法进行赋值
			zend_error(E_COMPILE_ERROR,
					"Spread operator is not supported in assignments");
		}

		// 被赋值的 value 语句
		var_ast = elem_ast->child[0];
		// 被赋值的 key 语句
		key_ast = elem_ast->child[1];
		// 能走到这里就有元素了
		has_elems = 1;

		// 如果有key
		if (is_keyed) {
			// 被赋值列表里不可以既有索引元素又有顺序元素，（测试过 $v = 2;[1=>$v,null] = [];）
			if (key_ast == NULL) {
				zend_error(E_COMPILE_ERROR,
					"Cannot mix keyed and unkeyed array entries in assignments");
			}
			// 编译表达式语句, p1:返回结果，p2:表达式语句
			zend_compile_expr(&dim_node, key_ast);
		// 没有key
		} else {
			// 如果有key语句
			if (key_ast != NULL) {
				// 报错：不可以在赋值时使用有key和无key的混合数组
				zend_error(E_COMPILE_ERROR,
					"Cannot mix keyed and unkeyed array entries in assignments");
			}
			// 赋值node的类型是 常量
			dim_node.op_type = IS_CONST;
			// 把元素序号存进 znode里
			ZVAL_LONG(&dim_node.u.constant, i);
		}

		// 表达式是常量
		if (expr_node->op_type == IS_CONST) {
			// 尝试添加引用
			Z_TRY_ADDREF(expr_node->u.constant);
		}

		// 检查数组赋值目标变量
		zend_verify_list_assign_target(var_ast, array_style);

		// 创建操作码 如果表达式node类型是 编译变量 ？ ZEND_FETCH_DIM_W ：ZEND_FETCH_LIST_W
		opline = zend_emit_op(&fetch_result,
			elem_ast->attr ? (expr_node->op_type == IS_CV ? ZEND_FETCH_DIM_W : ZEND_FETCH_LIST_W) : ZEND_FETCH_LIST_R, expr_node, &dim_node);

		// 如果 赋值node类型是常量
		if (dim_node.op_type == IS_CONST) {
			// 处理数字字串类型的数组索引
			zend_handle_numeric_dim(opline, &dim_node);
		}

		// 如果元素语句有附加属性
		if (elem_ast->attr) {
			// 创建操作码 ZEND_MAKE_REF
			zend_emit_op(&fetch_result, ZEND_MAKE_REF, &fetch_result, NULL);
		}
		// 如果变量语句 类型是 数组语句
		if (var_ast->kind == ZEND_AST_ARRAY) {
			// 深一层，递归
			zend_compile_list_assign(NULL, var_ast, &fetch_result, var_ast->attr);
		// 如果元素语句有附加属性
		} else if (elem_ast->attr) {
			// 创建一个地址赋值语句，用 var_ast 接收 znode 的地址
			zend_emit_assign_ref_znode(var_ast, &fetch_result);
		} else {
			// 执行赋值：创建临时赋值语句计算 value_node 的结果，把结果存入 var_ast 用
			zend_emit_assign_znode(var_ast, &fetch_result);
		}
	}

	// 如果没有元素
	if (has_elems == 0) {
		// 报错：不只可以使用空数组
		zend_error_noreturn(E_COMPILE_ERROR, "Cannot use empty list");
	}

	// 如果接收返回值
	if (result) {
		// 表达式 node作为返回值
		*result = *expr_node;
	} else {
		// 销毁编译结果
		zend_do_free(expr_node);
	}
}
/* }}} */

// ing3, 确定变量可写
static void zend_ensure_writable_variable(const zend_ast *ast) /* {{{ */
{
	// 调用函数，不可以给函数返回值赋值。（测试过 substr('11',0,1)=1;）
	if (ast->kind == ZEND_AST_CALL) {
		zend_error_noreturn(E_COMPILE_ERROR, "Can't use function return value in write context");
	}
	// 调用成员方法，允null的成员方法，静态方法（测试过 class a{ static function b(){}};a::b()=1;）
	if (
		ast->kind == ZEND_AST_METHOD_CALL
		|| ast->kind == ZEND_AST_NULLSAFE_METHOD_CALL
		|| ast->kind == ZEND_AST_STATIC_CALL
	) {		
		zend_error_noreturn(E_COMPILE_ERROR, "Can't use method return value in write context");
	}
	// 如果有短路，允null的方法已经在上面过滤过了，这里能捕捉到允null的成员变量。（测试过 $c?->b=1;）
	if (zend_ast_is_short_circuited(ast)) {
		zend_error_noreturn(E_COMPILE_ERROR, "Can't use nullsafe operator in write context");
	}
	// 直接给 $GLOBALS 赋值 (测试过 $GLOBALS=1;)
	if (is_globals_fetch(ast)) {
		zend_error_noreturn(E_COMPILE_ERROR,
			"$GLOBALS can only be modified using the $GLOBALS[$name] = $value syntax");
	}
}
/* }}} */

// 
/* Detects $a... = $a pattern */
// ing2, 检查是否是把自己赋值给自己或自己的子元素, zend_compile_expr_with_potential_assign_to_self 一次调用
static bool zend_is_assign_to_self(zend_ast *var_ast, zend_ast *expr_ast) /* {{{ */
{
	// 表达式类型不是 php变量 或 第一个子句不是变量名
	if (expr_ast->kind != ZEND_AST_VAR || expr_ast->child[0]->kind != ZEND_AST_ZVAL) {
		// 返回false
		return 0;
	}
	// 表达式类型是变量，并且第一个子句是变量名，才能走到这里

	// 是否是变量：php变量，数组元素，对象属性，允空对象属性，静态属性
	// 查找最终被赋值的那个变量。最终找到的那个东西要不是（php变量，数组元素，对象属性，允空对象属性，静态属性），或者类型不是Php变量
	while (zend_is_variable(var_ast) && var_ast->kind != ZEND_AST_VAR) {
		// 递归查找子元句
		var_ast = var_ast->child[0];
	}

	// 如果被赋值变量类型不是普通变量 或 第一个子句（变量名）不是内置变量
	if (var_ast->kind != ZEND_AST_VAR || var_ast->child[0]->kind != ZEND_AST_ZVAL) {
		// 返回false
		return 0;
	}

	{
		// 取得被赋值的变量名
		zend_string *name1 = zval_get_string(zend_ast_get_zval(var_ast->child[0]));
		// 取得表达式里的变量名
		zend_string *name2 = zval_get_string(zend_ast_get_zval(expr_ast->child[0]));
		// 看两个名字是否相等
		bool result = zend_string_equals(name1, name2);
		// 销毁两个名字
		zend_string_release_ex(name1, 0);
		zend_string_release_ex(name2, 0);
		// 
		return result;
	}
}
/* }}} */

// ing3, 编译表达式，并检测是否是间接赋值给自己
static void zend_compile_expr_with_potential_assign_to_self(
		znode *expr_node, zend_ast *expr_ast, zend_ast *var_ast) {
	// 检查是否是把自己赋值给自己或自己的子元素
	if (zend_is_assign_to_self(var_ast, expr_ast) && !is_this_fetch(expr_ast)) {
		/* $a[0] = $a should evaluate the right $a first */
		znode cv_node;
		// 先尝试编译表达式语句
		if (zend_try_compile_cv(&cv_node, expr_ast) == FAILURE) {
			// 如果不成功，编译简单变量，不使用CV
			zend_compile_simple_var_no_cv(expr_node, expr_ast, BP_VAR_R, 0);
		// 如果成功
		} else {
			// 创建操作码 ZEND_QM_ASSIGN
			zend_emit_op_tmp(expr_node, ZEND_QM_ASSIGN, &cv_node, NULL);
		}
	// 编译表达式语句
	} else {
		// 编译表达式语句, p1:返回结果，p2:表达式语句
		zend_compile_expr(expr_node, expr_ast);
	}
}

// ing3, 编译赋值
static void zend_compile_assign(znode *result, zend_ast *ast) /* {{{ */
{
	// 接收变量名
	zend_ast *var_ast = ast->child[0];
	// 表达式
	zend_ast *expr_ast = ast->child[1];

	znode var_node, expr_node;
	zend_op *opline;
	
	// 
	uint32_t offset;
	
	// 如果是$this, 报错，不可以给 $this 赋值
	if (is_this_fetch(var_ast)) {
		zend_error_noreturn(E_COMPILE_ERROR, "Cannot re-assign $this");
	}

	// 确定变量可写
	zend_ensure_writable_variable(var_ast);

	// $GLOBALS['x'] 和变量一样处理
	/* Treat $GLOBALS['x'] assignment like assignment to variable. */
	zend_ast_kind kind = is_global_var_fetch(var_ast) ? ZEND_AST_VAR : var_ast->kind;
	// 根据类型处理
	switch (kind) {
		// 给 php变量 赋值
		case ZEND_AST_VAR:
			// 延时编译，获取起始堆栈高度
			offset = zend_delayed_compile_begin();
			// 编译接收变量名
			zend_delayed_compile_var(&var_node, var_ast, BP_VAR_W, 0);
			// 编译表达式语句, p1:返回结果，p2:表达式语句
			zend_compile_expr(&expr_node, expr_ast);
			// 结束延时编译
			zend_delayed_compile_end(offset);
			// 取得赋值语句行号
			CG(zend_lineno) = zend_ast_get_lineno(var_ast);
			// 添加临时变量操作码 ZEND_ASSIGN
			zend_emit_op_tmp(result, ZEND_ASSIGN, &var_node, &expr_node);
			return;
		// 给类的静态变量 赋值
		case ZEND_AST_STATIC_PROP:
			// 开始延时编译，返回起始堆栈高度
			offset = zend_delayed_compile_begin();
			// 延时编译变量名
			zend_delayed_compile_var(result, var_ast, BP_VAR_W, 0);
			// 编译表达式语句, p1:返回结果，p2:表达式语句
			zend_compile_expr(&expr_node, expr_ast);
			// 结束延时编译
			opline = zend_delayed_compile_end(offset);
			// 创建操作码 ZEND_ASSIGN_STATIC_PROP
			opline->opcode = ZEND_ASSIGN_STATIC_PROP;
			// 结果类型为临时变量
			opline->result_type = IS_TMP_VAR;
			result->op_type = IS_TMP_VAR;
			// 创建 ZEND_OP_DATA 操作码
			zend_emit_op_data(&expr_node);
			return;
		// 给数组元素 赋值
		case ZEND_AST_DIM:
			// 开始延时编译
			offset = zend_delayed_compile_begin();
			// 延时编译写入数组元素
			zend_delayed_compile_dim(result, var_ast, BP_VAR_W, /* by_ref */ false);
			// 编译表达式，并检查是否间接赋值给自己
			zend_compile_expr_with_potential_assign_to_self(&expr_node, expr_ast, var_ast);
			// 结束延时编译
			opline = zend_delayed_compile_end(offset);
			// 创建操作码 ZEND_ASSIGN_DIM
			opline->opcode = ZEND_ASSIGN_DIM;
			// 结果类型是临时变量
			opline->result_type = IS_TMP_VAR;
			result->op_type = IS_TMP_VAR;
			// 创建 ZEND_OP_DATA 操作码
			opline = zend_emit_op_data(&expr_node);
			return;
		// 给类属性 赋值，允null的类属性
		case ZEND_AST_PROP:
		case ZEND_AST_NULLSAFE_PROP:
			// 开始延时编译
			offset = zend_delayed_compile_begin();
			// 延时编译类属性
			zend_delayed_compile_prop(result, var_ast, BP_VAR_W);
			// 编译表达式语句, p1:返回结果，p2:表达式语句
			zend_compile_expr(&expr_node, expr_ast);
			// 结束延时编译
			opline = zend_delayed_compile_end(offset);
			// 创建操作码 ZEND_ASSIGN_OBJ
			opline->opcode = ZEND_ASSIGN_OBJ;
			// 结果类型是 临时变量
			opline->result_type = IS_TMP_VAR;
			// 
			result->op_type = IS_TMP_VAR;
			// 创建 ZEND_OP_DATA 操作码
			zend_emit_op_data(&expr_node);
			return;
		// 给数组键值对列表赋值 ,这是个非常古怪的东西 
		// $k = 1;$v = 2;[$k=>$v] = [1=>22];
		case ZEND_AST_ARRAY:
			// 递归看被赋值变量，如果有引用的元素
			if (zend_propagate_list_refs(var_ast)) {
				// 如果不是变量或调用函数 
				if (!zend_is_variable_or_call(expr_ast)) {
					// 报错 不能把引用赋值给无法引用的对象
					zend_error_noreturn(E_COMPILE_ERROR,
						"Cannot assign reference to non referenceable value");
				}
				// 编译变量，p1:返回值，p2:语句，p3:操作类型，p4:是否引用传递
				zend_compile_var(&expr_node, expr_ast, BP_VAR_W, 1);
				/* MAKE_REF is usually not necessary for CVs. However, if there are
				 * self-assignments, this forces the RHS to evaluate first. */
				// 创建操作码 ZEND_MAKE_REF
				zend_emit_op(&expr_node, ZEND_MAKE_REF, &expr_node, NULL);
			// 如果 被赋值变量 没有引用的元素
			} else {
				// 如果语句类型是变量
				if (expr_ast->kind == ZEND_AST_VAR) {
					// 分散赋值需要先计算右边的元素
					/* list($a, $b) = $a should evaluate the right $a first */
					znode cv_node;
					// 先尝试编译表达式
					if (zend_try_compile_cv(&cv_node, expr_ast) == FAILURE) {
						// 如果失败，编译简单变量，不使用CV
						zend_compile_simple_var_no_cv(&expr_node, expr_ast, BP_VAR_R, 0);
					} else {
						// 创建临时操作码 ZEND_QM_ASSIGN
						zend_emit_op_tmp(&expr_node, ZEND_QM_ASSIGN, &cv_node, NULL);
					}
				// 如果语句类型不是变量
				} else {
					// 编译表达式语句, p1:返回结果，p2:表达式语句
					zend_compile_expr(&expr_node, expr_ast);
				}
			}
			// 编译列表赋值
			zend_compile_list_assign(result, var_ast, &expr_node, var_ast->attr);
			return;
		// 其它情况
		EMPTY_SWITCH_DEFAULT_CASE();
	}
}
/* }}} */

// ing3，引用赋值, $a = &$b;
static void zend_compile_assign_ref(znode *result, zend_ast *ast) /* {{{ */
{
	// 目标变量语句
	zend_ast *target_ast = ast->child[0];
	// 原变量语句
	zend_ast *source_ast = ast->child[1];

	znode target_node, source_node;
	zend_op *opline;
	uint32_t offset, flags;

	// 如果目标是 $this
	if (is_this_fetch(target_ast)) {
		zend_error_noreturn(E_COMPILE_ERROR, "Cannot re-assign $this");
	}
	// 确定变量可写
	zend_ensure_writable_variable(target_ast);
	// 如果源变量包含短路，报错
	if (zend_ast_is_short_circuited(source_ast)) {
		zend_error_noreturn(E_COMPILE_ERROR, "Cannot take reference of a nullsafe chain");
	}
	// 如果源变量是 $GLOBALS[] 元素，报错
	if (is_globals_fetch(source_ast)) {
		zend_error_noreturn(E_COMPILE_ERROR, "Cannot acquire reference to $GLOBALS");
	}

	// 开始延时编译
	offset = zend_delayed_compile_begin();
	// 延时编译目标变量
	zend_delayed_compile_var(&target_node, target_ast, BP_VAR_W, 1);
	// 编译变量，p1:返回值，p2:语句，p3:操作类型，p4:是否引用传递
	zend_compile_var(&source_node, source_ast, BP_VAR_W, 1);

	// 如果 （目标变量不是简单变量 或 目标变量类型不是内置变量） 并且 原变量类型不是 ZEND_AST_ZNODE 也不是 编译变量
	if ((target_ast->kind != ZEND_AST_VAR
	  || target_ast->child[0]->kind != ZEND_AST_ZVAL)
	 && source_ast->kind != ZEND_AST_ZNODE
	 && source_node.op_type != IS_CV) {
		/* Both LHS and RHS expressions may modify the same data structure,
		 * and the modification during RHS evaluation may dangle the pointer
		 * to the result of the LHS evaluation.
		 * Use MAKE_REF instruction to replace direct pointer with REFERENCE.
		 * See: Bug #71539
		 */
		// 创建操作码 ZEND_MAKE_REF
		zend_emit_op(&source_node, ZEND_MAKE_REF, &source_node, NULL);
	}

	// 结束延时编译
	opline = zend_delayed_compile_end(offset);
	
	// 如果原变量 类型不是变量，也不是调用函数 
	if (source_node.op_type != IS_VAR && zend_is_call(source_ast)) {
		// 报错 不可以用内置函数的返回值 做原变量 （测试过 $a = &strlen(1); ）
		zend_error_noreturn(E_COMPILE_ERROR, "Cannot use result of built-in function in write context");
	}

	// 如果原变量是调用函数，添加 ZEND_RETURNS_FUNCTION 语句
	flags = zend_is_call(source_ast) ? ZEND_RETURNS_FUNCTION : 0;

	// 如果操作码类型是 写入对象属性
	if (opline && opline->opcode == ZEND_FETCH_OBJ_W) {
		// 创建 ZEND_ASSIGN_OBJ_REF 操作码
		opline->opcode = ZEND_ASSIGN_OBJ_REF;
		opline->extended_value &= ~ZEND_FETCH_REF;
		opline->extended_value |= flags;
		// 创建 ZEND_OP_DATA 操作码
		zend_emit_op_data(&source_node);
		*result = target_node;
	// 如果操作码类型是 写入对象静态属性
	} else if (opline && opline->opcode == ZEND_FETCH_STATIC_PROP_W) {
		// 创建 ZEND_ASSIGN_STATIC_PROP_REF 操作码
		opline->opcode = ZEND_ASSIGN_STATIC_PROP_REF;
		// 添加标记
		opline->extended_value &= ~ZEND_FETCH_REF;
		opline->extended_value |= flags;
		// 创建 ZEND_OP_DATA 操作码
		zend_emit_op_data(&source_node);
		*result = target_node;
	// 其他情况
	} else {
		// 创建 ZEND_ASSIGN_REF 操作码
		opline = zend_emit_op(result, ZEND_ASSIGN_REF, &target_node, &source_node);
		// 添加扩展标记
		opline->extended_value = flags;
	}
}
/* }}} */

// ing4, 创建一个地址赋值语句，用 var_ast 接收 znode 的地址
static inline void zend_emit_assign_ref_znode(zend_ast *var_ast, znode *value_node) /* {{{ */
{
	// 临时节点
	znode dummy_node;
	// 创建传递引用语句 $a = &$b;
	zend_ast *assign_ast = zend_ast_create(ZEND_AST_ASSIGN_REF, var_ast,
		zend_ast_create_znode(value_node));
	// 编译表达式语句, p1:返回结果，p2:表达式语句
	zend_compile_expr(&dummy_node, assign_ast);
	// 删除临时节点
	zend_do_free(&dummy_node);
}
/* }}} */

// ing3, 变量后面加 (运算符和= ) 再加表达式，如 $a += 2;
static void zend_compile_compound_assign(znode *result, zend_ast *ast) /* {{{ */
{
	// 接收变量语句
	zend_ast *var_ast = ast->child[0];
	// 表达式语句
	zend_ast *expr_ast = ast->child[1];
	uint32_t opcode = ast->attr;

	znode var_node, expr_node;
	zend_op *opline;
	uint32_t offset, cache_slot;
	// 确定变量可写
	zend_ensure_writable_variable(var_ast);

	/* Treat $GLOBALS['x'] assignment like assignment to variable. */
	zend_ast_kind kind = is_global_var_fetch(var_ast) ? ZEND_AST_VAR : var_ast->kind;
	switch (kind) {
		// 如果对象是简单变量
		case ZEND_AST_VAR:
			// 开始延时编译
			offset = zend_delayed_compile_begin();
			// 延时编译 变量
			zend_delayed_compile_var(&var_node, var_ast, BP_VAR_RW, 0);
			// 编译表达式语句, p1:返回结果，p2:表达式语句
			zend_compile_expr(&expr_node, expr_ast);
			// 结束延时编译
			zend_delayed_compile_end(offset);
			// 创建操作码 ZEND_ASSIGN_OP 
			opline = zend_emit_op_tmp(result, ZEND_ASSIGN_OP, &var_node, &expr_node);
			opline->extended_value = opcode;
			return;
		// 如果对象是类的静态属性
		case ZEND_AST_STATIC_PROP:
			// 开始延时编译
			offset = zend_delayed_compile_begin();
			// 编译接收变量
			zend_delayed_compile_var(result, var_ast, BP_VAR_RW, 0);
			// 编译表达式语句, p1:返回结果，p2:表达式语句
			zend_compile_expr(&expr_node, expr_ast);
			// 结束延时编译
			opline = zend_delayed_compile_end(offset);
			//
			cache_slot = opline->extended_value;
			// 创建操作码 ZEND_ASSIGN_STATIC_PROP_OP
			opline->opcode = ZEND_ASSIGN_STATIC_PROP_OP;
			// 语句的扩展信息，添加到操作码中
			opline->extended_value = opcode;
			// 类型是临时变量
			opline->result_type = IS_TMP_VAR;
			result->op_type = IS_TMP_VAR;
			// 创建 ZEND_OP_DATA 操作码
			opline = zend_emit_op_data(&expr_node);
			opline->extended_value = cache_slot;
			return;
		// 如果对象是数组元素
		case ZEND_AST_DIM:
			// 开始延时编译
			offset = zend_delayed_compile_begin();
			// 延时编译接收变量
			zend_delayed_compile_dim(result, var_ast, BP_VAR_RW, /* by_ref */ false);
			// 编译表达式，有可能赋值给自己的子元素
			zend_compile_expr_with_potential_assign_to_self(&expr_node, expr_ast, var_ast);
			// 结束延时编译
			opline = zend_delayed_compile_end(offset);
			// 创建操作码 ZEND_ASSIGN_DIM_OP
			opline->opcode = ZEND_ASSIGN_DIM_OP;
			// 语句的扩展信息，添加到操作码中
			opline->extended_value = opcode;
			// 类型为临时变量
			opline->result_type = IS_TMP_VAR;
			result->op_type = IS_TMP_VAR;
			// 创建 ZEND_OP_DATA 操作码
			zend_emit_op_data(&expr_node);
			return;
		// 如果对象是，类属性
		case ZEND_AST_PROP:
		case ZEND_AST_NULLSAFE_PROP:
			// 开始延时编译
			offset = zend_delayed_compile_begin();
			// 延时编译接收变量
			zend_delayed_compile_prop(result, var_ast, BP_VAR_RW);
			// 编译表达式语句, p1:返回结果，p2:表达式语句
			zend_compile_expr(&expr_node, expr_ast);
			// 结束延时编译
			opline = zend_delayed_compile_end(offset);
			// ?
			cache_slot = opline->extended_value;
			opline->opcode = ZEND_ASSIGN_OBJ_OP;
			// 语句的扩展信息，添加到操作码中
			opline->extended_value = opcode;
			// 类型为临时变量
			opline->result_type = IS_TMP_VAR;
			result->op_type = IS_TMP_VAR;
			// 创建 ZEND_OP_DATA 操作码
			opline = zend_emit_op_data(&expr_node);
			opline->extended_value = cache_slot;
			return;
		// 其他情况
		EMPTY_SWITCH_DEFAULT_CASE()
	}
}
/* }}} */

// ing3, 获取 当前参数名 在函数形参列表中的序号, 1处调用
static uint32_t zend_get_arg_num(zend_function *fn, zend_string *arg_name) {
	// 如果是用户代码 ， /Zend/zend_language_scanner.l：compile_file 中赋值，只要是php脚本都带这个参数
	// TODO: Caching?
	if (fn->type == ZEND_USER_FUNCTION) {
		// 遍历所有形参
		for (uint32_t i = 0; i < fn->common.num_args; i++) {
			// 取得参数信息
			zend_arg_info *arg_info = &fn->op_array.arg_info[i];
			// 如果匹配到参数名，返回序号
			if (zend_string_equals(arg_info->name, arg_name)) {
				return i + 1;
			}
		}
	// 不是用户代码
	} else {
		// 遍历所有形参
		for (uint32_t i = 0; i < fn->common.num_args; i++) {
			// 取得内置参数信息
			zend_internal_arg_info *arg_info = &fn->internal_function.arg_info[i];
			// 参数名长度
			size_t len = strlen(arg_info->name);
			// 如果匹配到参数名，返回序号
			if (zend_string_equals_cstr(arg_name, arg_info->name, len)) {
				return i + 1;
			}
		}
	}

	// 无效的参数名，或者 收集到 variadic 参数中
	/* Either an invalid argument name, or collected into a variadic argument. */
	return (uint32_t) -1;
}

// 260行
// ing2, 编译函数调用时的参数列表，只有 zend_compile_call_common 调用
static uint32_t zend_compile_args(
		zend_ast *ast, zend_function *fbc, bool *may_have_extra_named_args) /* {{{ */
{
	// 参数列表语句
	zend_ast_list *args = zend_ast_get_list(ast);
	uint32_t i;
	bool uses_arg_unpack = 0;
	uint32_t arg_count = 0; /* number of arguments not including unpacks */

	// 命名参数是否按句法调用，加强语法级别的限制。不要真的用命名参数进行传递。
	/* Whether named arguments are used syntactically, to enforce language level limitations.
	 * May not actually use named argument passing. */
	bool uses_named_args = 0;
	// 使用命名参数时是否有未定义参数。
	/* Whether there may be any undef arguments due to the use of named arguments. */
	bool may_have_undef = 0;
	// 是否有扩展的命名参数进入变量字典
	/* Whether there may be any extra named arguments collected into a variadic. */
	*may_have_extra_named_args = 0;

	// 遍历所有参数
	for (i = 0; i < args->children; ++i) {
		zend_ast *arg = args->child[i];
		// 参数名
		zend_string *arg_name = NULL;
		// 顺序号
		uint32_t arg_num = i + 1;

		znode arg_node;
		zend_op *opline;
		zend_uchar opcode;

		// 如果是打包参数 func(...[1,2,3])
		if (arg->kind == ZEND_AST_UNPACK) {
			// 如果使用了带名称的参数
			if (uses_named_args) {
				// 报错，不可以在命名参数后面使用打包参数（ 测试过：func(a3:99,...[11,22]); ）
				zend_error_noreturn(E_COMPILE_ERROR,
					"Cannot use argument unpacking after named arguments");
			}
			// 标记，使用打包参数
			uses_arg_unpack = 1;
			// 清空函数指针
			fbc = NULL;
			// 编译表达式语句, p1:返回结果，p2:表达式语句
			zend_compile_expr(&arg_node, arg->child[0]);
			// 创建 ZEND_SEND_UNPACK 操作码
			opline = zend_emit_op(NULL, ZEND_SEND_UNPACK, &arg_node, NULL);
			// 第二个操作对象里放，打包参数以外的参数数
			opline->op2.num = arg_count;
			//  操作码结果 arg_count - 1 做什么用？
			opline->result.var = EX_NUM_TO_VAR(arg_count - 1);

			// 打包参数可能包含命名参数 （测试过 a(...['a3'=>119]); 有效）
			/* Unpack may contain named arguments. */
			may_have_undef = 1;
			// 已经在上面赋值成null了
			if (!fbc || (fbc->common.fn_flags & ZEND_ACC_VARIADIC)) {
				// 有可能包含扩展 命名参数
				*may_have_extra_named_args = 1;
			}
			// 下一个参数
			continue;
		}

		// 如果是命名参数, func(a:1)，第一个子句是参数名，第二是值
		if (arg->kind == ZEND_AST_NAMED_ARG) {
			// 使用命名参数
			uses_named_args = 1;
			// 给参数名，创建保留字 
			arg_name = zval_make_interned_string(zend_ast_get_zval(arg->child[0]));
			// 参数值
			arg = arg->child[1];
			// 如果有传入函数 ，且没有用到打包参数
			if (fbc && !uses_arg_unpack) {
				// 获取 当前参数名 在函数形参列表中的序号
				arg_num = zend_get_arg_num(fbc, arg_name);
				// 参数数量+1 ，如果没有未定义参数 
				if (arg_num == arg_count + 1 && !may_have_undef) {
					// 使用了命名参数，但按顺序每个都传递了
					/* Using named arguments, but passing in order. */
					// 清空参数名
					arg_name = NULL;
					// 参数数量 +1
					arg_count++;
				// 如果参数没有按顺序传递
				} else {
					// 即使不在顺序中，也要追踪传入的参数
					// TODO: We could track which arguments were passed, even if out of order.
					// 有未定义参数
					may_have_undef = 1;
					// 参数序号为异常（这个是是传入参数，不是返回值） 并且 如果有 字典参数
					if (arg_num == (uint32_t) -1 && (fbc->common.fn_flags & ZEND_ACC_VARIADIC)) {
						// 包含扩展命名参数
						*may_have_extra_named_args = 1;
					}
				}
			// 如没有传入函数或者 有打包参数
			} else {
				// 有打包参数，无法计算序号
				arg_num = (uint32_t) -1;
				// 有可能包含未定义参数
				may_have_undef = 1;
				// 包含额外命名参数
				*may_have_extra_named_args = 1;
			}
		// 如果不是命名参数
		} else {
			// 如果使用打包参数
			if (uses_arg_unpack) {
				// 报错，顺序参数不可以在打包参数后面
				zend_error_noreturn(E_COMPILE_ERROR,
					"Cannot use positional argument after argument unpacking");
			}
			// 如果使用了命名参数
			if (uses_named_args) {
				// 报错，顺序参数不可以在命名参数后面
				zend_error_noreturn(E_COMPILE_ERROR,
					"Cannot use positional argument after named argument");
			}
			// 参数数量 +1
			arg_count++;
		}

		// 处理传递 $GLOBALS 与传递调用相同，如果参数是引用传递，运行时会报错
		/* Treat passing of $GLOBALS the same as passing a call.
		 * This will error at runtime if the argument is by-ref. */
		// 如果参数是函数调用语句 或 获获取全局变量的元素
		if (zend_is_call(arg) || is_globals_fetch(arg)) {
			// 编译变量，p1:返回值，p2:语句，p3:操作类型，p4:是否引用传递
			zend_compile_var(&arg_node, arg, BP_VAR_R, 0);
			// 如果编译结果是常量或临时变量
			if (arg_node.op_type & (IS_CONST|IS_TMP_VAR)) {
				// 函数调用 转换成 内置命令 ？
				/* Function call was converted into builtin instruction */
				// 如果没有传入函数，或者函数的参数必须通过引用传递
				if (!fbc || ARG_MUST_BE_SENT_BY_REF(fbc, arg_num)) {
					// 
					opcode = ZEND_SEND_VAL_EX;
				// 有传入函数 并且 不必须通过引用传递
				} else {
					//
					opcode = ZEND_SEND_VAL;
				}
			// 如果编译结果是其他类型
			} else {
				// 如果有传入函数并且此参数 序号正常
				if (fbc && arg_num != (uint32_t) -1) {
					// 如果必须用引用传递
					if (ARG_MUST_BE_SENT_BY_REF(fbc, arg_num)) {
						//
						opcode = ZEND_SEND_VAR_NO_REF;
					// 如果可以用引用传递
					} else if (ARG_MAY_BE_SENT_BY_REF(fbc, arg_num)) {
						// 对于变量运算对象，SEND_VAL 会直接传递变量，不解引用
						// ？？
						/* For IS_VAR operands, SEND_VAL will pass through the operand without
						 * dereferencing, so it will use a by-ref pass if the call returned by-ref
						 * and a by-value pass if it returned by-value. */
						opcode = ZEND_SEND_VAL;
					// 不用引用传递
					} else {
						opcode = ZEND_SEND_VAR;
					}
				// 没有传入函数 或 参数序号不正常
				} else {
					opcode = ZEND_SEND_VAR_NO_REF_EX;
				}
			}
		// 如果是变量：php变量，数组元素，对象属性，允空对象属性，静态属性
		// 并且不允许短路
		} else if (zend_is_variable(arg) && !zend_ast_is_short_circuited(arg)) {
			// 如果有传入函数并且此参数 序号正常
			if (fbc && arg_num != (uint32_t) -1) {
				// 如果可以用引用传递
				if (ARG_SHOULD_BE_SENT_BY_REF(fbc, arg_num)) {
					// 编译变量，p1:返回值，p2:语句，p3:操作类型，p4:是否引用传递
					zend_compile_var(&arg_node, arg, BP_VAR_W, 1);
					// 
					opcode = ZEND_SEND_REF;
				// 录可以用引用传递
				} else {
					// 编译变量，p1:返回值，p2:语句，p3:操作类型，p4:是否引用传递
					zend_compile_var(&arg_node, arg, BP_VAR_R, 0);
					// 
					opcode = (arg_node.op_type == IS_TMP_VAR) ? ZEND_SEND_VAL : ZEND_SEND_VAR;
				}
			// 没有传入函数 或 序号不正常
			} else {
				do {
					// 如果参数类型为变量
					if (arg->kind == ZEND_AST_VAR) {
						// 取得语句行号
						CG(zend_lineno) = zend_ast_get_lineno(ast);
						// 如果是 从$this获取
						if (is_this_fetch(arg)) {
							// 创建操作码 ZEND_FETCH_THIS
							zend_emit_op(&arg_node, ZEND_FETCH_THIS, NULL, NULL);
							// 操作码 
							opcode = ZEND_SEND_VAR_EX;
							// 活跃操作码组添加 使用 $this 标记
							CG(active_op_array)->fn_flags |= ZEND_ACC_USES_THIS;
							// 跳出
							break;
						// 不是$this。 编译变量，如果成功
						} else if (zend_try_compile_cv(&arg_node, arg) == SUCCESS) {
							// 
							opcode = ZEND_SEND_VAR_EX;
							// 跳出
							break;
						}
					}
					// 创建操作码 ZEND_CHECK_FUNC_ARG
					opline = zend_emit_op(NULL, ZEND_CHECK_FUNC_ARG, NULL, NULL);
					// 如果有参数名
					if (arg_name) {
						// 第二个操作对象类型是 常量
						opline->op2_type = IS_CONST;
						// 添加引用次数，保留字串返回1，其他返回添加后的引用次数
						zend_string_addref(arg_name);
						// 参数名添加到 第二个运算对象里
						opline->op2.constant = zend_add_literal_string(&arg_name);
						// 给 CG(active_op_array) 添加缓存大小，添加 count 个void*指针的大小, 返回缓存大小
						opline->result.num = zend_alloc_cache_slots(2);
					// 如果没有参数名
					} else {
						// 编号记录到第二个操作对象里
						opline->op2.num = arg_num;
					}
					// 编译变量，p1:返回值，p2:语句，p3:操作类型，p4:是否引用传递
					zend_compile_var(&arg_node, arg, BP_VAR_FUNC_ARG, 1);
					// 
					opcode = ZEND_SEND_FUNC_ARG;
				} while (0);
			}
		// 其他情况
		} else {
			// 编译表达式语句, p1:返回结果，p2:表达式语句
			zend_compile_expr(&arg_node, arg);
			// 如果参数编译结果 类型是变量
			if (arg_node.op_type == IS_VAR) {
				/* pass ++$a or something similar */
				// 如果有传入函数并且此参数 序号正常
				if (fbc && arg_num != (uint32_t) -1) {
					// 如果必须使用引用传递
					if (ARG_MUST_BE_SENT_BY_REF(fbc, arg_num)) {
						opcode = ZEND_SEND_VAR_NO_REF;
					// 如果可以用引用传递
					} else if (ARG_MAY_BE_SENT_BY_REF(fbc, arg_num)) {
						opcode = ZEND_SEND_VAL;
					// 如果不可以用引用传递
					} else {
						opcode = ZEND_SEND_VAR;
					}
				// 如果有打包参数
				} else {
					opcode = ZEND_SEND_VAR_NO_REF_EX;
				}
			// 如果参数编译结果 类型是编译变量
			} else if (arg_node.op_type == IS_CV) {
				// 如果有传入函数并且此参数 序号正常
				if (fbc && arg_num != (uint32_t) -1) {
					// 如果可以用引用传递
					if (ARG_SHOULD_BE_SENT_BY_REF(fbc, arg_num)) {
						opcode = ZEND_SEND_REF;
					// 不可以用引用传递
					} else {
						opcode = ZEND_SEND_VAR;
					}
				// 如果有打包参数
				} else {
					opcode = ZEND_SEND_VAR_EX;
				}
			// 其他情况 ZEND_SEND_VAL,ZEND_SEND_VAR 什么区别
			} else {
				// 把 “只有变量可以被引用传递” 这个错误延时到 执行时发生
				/* Delay "Only variables can be passed by reference" error to execution */
				// 如果有传入函数并且此参数 序号正常，并且没有要求引用传递
				if (fbc && arg_num != (uint32_t) -1 && !ARG_MUST_BE_SENT_BY_REF(fbc, arg_num)) {
					opcode = ZEND_SEND_VAL;
				// 如果有打包参数
				} else {
					opcode = ZEND_SEND_VAL_EX;
				}
			}
		}
		
		// 创建操作码 opcode
		opline = zend_emit_op(NULL, opcode, &arg_node, NULL);
		// 如果有参数名
		if (arg_name) {
			// 第二个操作对象类型是常量
			opline->op2_type = IS_CONST;
			// 参数名增加引用次数（保留字永远是1）
			zend_string_addref(arg_name);
			// 常量值为 参数名
			opline->op2.constant = zend_add_literal_string(&arg_name);
			// 给 CG(active_op_array) 添加缓存大小，添加 count 个void*指针的大小, 返回缓存大小
			opline->result.num = zend_alloc_cache_slots(2);
		// 没有参数名
		} else {
			// 第二个操作对象中记录参数序号
			opline->op2.opline_num = arg_num;
			// 结果为 参数序号 - 1
			opline->result.var = EX_NUM_TO_VAR(arg_num - 1);
		}
	}

	// 如果包含未定义参数
	if (may_have_undef) {
		// 创建操作码 ZEND_CHECK_UNDEF_ARGS
		zend_emit_op(NULL, ZEND_CHECK_UNDEF_ARGS, NULL, NULL);
	}
	// 返回参数数量 
	return arg_count;
}
/* }}} */

// ing3, 创建 call 操作码
ZEND_API zend_uchar zend_get_call_op(const zend_op *init_op, zend_function *fbc) /* {{{ */
{
	// 如果方法存在
	if (fbc) {
		// 方法不可以通过 __call, __callstatic 调用
		ZEND_ASSERT(!(fbc->common.fn_flags & ZEND_ACC_CALL_VIA_TRAMPOLINE));
		// 如果是内置函数 并且 编译选项要求 忽略内置函数
		if (fbc->type == ZEND_INTERNAL_FUNCTION && !(CG(compiler_options) & ZEND_COMPILE_IGNORE_INTERNAL_FUNCTIONS)) {
			// 如果操作码是 ZEND_INIT_FCALL ，并且不存在 zend_execute_internal 方法
			if (init_op->opcode == ZEND_INIT_FCALL && !zend_execute_internal) {
				// 如果函数没有 弃用标记
				if (!(fbc->common.fn_flags & ZEND_ACC_DEPRECATED)) {
					// 返回操作码 ZEND_DO_ICALL
					return ZEND_DO_ICALL;
				} else {
					// 返回操作码 ZEND_DO_FCALL_BY_NAME
					return ZEND_DO_FCALL_BY_NAME;
				}
			}
		// 如果是内置函数 并且 编译选项没有要求 忽略忽略用户函数
		} else if (!(CG(compiler_options) & ZEND_COMPILE_IGNORE_USER_FUNCTIONS)){
			// ？ 如果要求使用 zend_execute_ex 是 execute_ex 方法
			if (zend_execute_ex == execute_ex) {
				// 返回操作码 ZEND_DO_UCALL
				return ZEND_DO_UCALL;
			}
		}
	// 如果 fbc 不存在，并且 求使用 execute_ex 方法
	} else if (zend_execute_ex == execute_ex &&
				// 并且没有 zend_execute_internal 方法
	           !zend_execute_internal &&
			   // 并且 （ 操作码是 ZEND_INIT_FCALL_BY_NAME 或 ZEND_INIT_NS_FCALL_BY_NAME ）
	           (init_op->opcode == ZEND_INIT_FCALL_BY_NAME ||
	            init_op->opcode == ZEND_INIT_NS_FCALL_BY_NAME)) {
		// 返回操作码 ZEND_DO_FCALL_BY_NAME
		return ZEND_DO_FCALL_BY_NAME;
	}
	// 默认返回操作码 ZEND_DO_FCALL
	return ZEND_DO_FCALL;
}
/* }}} */

// ing3, 函数和闭包调用，方法调用，静态方法调用，new class调构造方法
static bool zend_compile_call_common(znode *result, zend_ast *args_ast, zend_function *fbc, uint32_t lineno) /* {{{ */
{
	zend_op *opline;
	// 取得上一个操作码的索引号。get_next_op_number 只是返回数量，没有创建新操作码
	uint32_t opnum_init = get_next_op_number() - 1;

	// 闭包转换语句，这是个奇怪的东西，func(...) 可以把已有的函数转换成闭包！
	// 测试过： function a(){var_dump($arguments);}; $b = a(...); var_dump($b);
	if (args_ast->kind == ZEND_AST_CALLABLE_CONVERT) {
		// 取得上一个操作码
		opline = &CG(active_op_array)->opcodes[opnum_init];
		// 清空扩展信息
		opline->extended_value = 0;
		// 如果初始操作码是new 语句
		if (opline->opcode == ZEND_NEW) {
			// 报错，不可以new 一个闭包转换语句 （测试过 new a(...); ）
		    zend_error_noreturn(E_COMPILE_ERROR, "Cannot create Closure for new expression");
		}
		// 如果操作码类型是 ZEND_INIT_FCALL
		if (opline->opcode == ZEND_INIT_FCALL) {
			// ？
			opline->op1.num = zend_vm_calc_used_stack(0, fbc);
		}
		// 创建 ZEND_CALLABLE_CONVERT 操作码
		zend_emit_op_tmp(result, ZEND_CALLABLE_CONVERT, NULL, NULL);
		// 闭包转换语句，返回true 
		return true;
	}

	bool may_have_extra_named_args;
	// 编译参数列表
	uint32_t arg_count = zend_compile_args(args_ast, fbc, &may_have_extra_named_args);
	// 创建 ZEND_EXT_FCALL_BEGIN 操作码
	zend_do_extended_fcall_begin();

	// 上一个操作码
	opline = &CG(active_op_array)->opcodes[opnum_init];
	// 扩展信息：参数数量 
	opline->extended_value = arg_count;

	// 如果操作码类型是，初始化方法调用
	if (opline->opcode == ZEND_INIT_FCALL) {
		// ？
		opline->op1.num = zend_vm_calc_used_stack(arg_count, fbc);
	}
	// 创建操作码，用 zend_get_call_op 方法取回操作码
	opline = zend_emit_op(result, zend_get_call_op(opline, fbc), NULL, NULL);
	// 如果可能包含打包参数
	if (may_have_extra_named_args) {
		// 添加标记
		opline->extended_value = ZEND_FCALL_MAY_HAVE_EXTRA_NAMED_PARAMS;
	}
	// 记录行号
	opline->lineno = lineno;
	// 创建 ZEND_EXT_FCALL_END 操作码
	zend_do_extended_fcall_end();
	// 不是 闭包转换语句 返回false
	return false;
}
/* }}} */

// ing3, 编译函数名
static bool zend_compile_function_name(znode *name_node, zend_ast *name_ast) /* {{{ */
{
	// 获取原始名称
	zend_string *orig_name = zend_ast_get_str(name_ast);
	bool is_fully_qualified;
	// 结果类型标记成常量
	name_node->op_type = IS_CONST;
	// 取得完整的常量名，返回
	// 把用户传入的函数名转成 运行时的函数名, 大小写不敏感
	ZVAL_STR(&name_node->u.constant, zend_resolve_function_name(
		orig_name, name_ast->attr, &is_fully_qualified));
	// 返回：是否是命名空间中的函数 （名称不是完整路径名，并且当前在命名空间中 ）
	return !is_fully_qualified && FC(current_namespace);
}
/* }}} */

// ing3, 编译带命名空间的函数调用 ，zend_compile_call，1次引用
static void zend_compile_ns_call(znode *result, znode *name_node, zend_ast *args_ast, uint32_t lineno) /* {{{ */
{
	// 创建操作码
	zend_op *opline = get_next_op();
	// 类型：通过名称初始化 带命名空间的函数调用
	opline->opcode = ZEND_INIT_NS_FCALL_BY_NAME;
	// 第二个操作对象为常量
	opline->op2_type = IS_CONST;
	// 值为带命名空间的函数名
	opline->op2.constant = zend_add_ns_func_name_literal(
		Z_STR(name_node->u.constant));
	// 给 CG(active_op_array) 添加缓存大小，添加 一个 void*指针的大小，返回添加后的缓存大小
	opline->result.num = zend_alloc_cache_slot();
	// 调用函数 
	zend_compile_call_common(result, args_ast, NULL, lineno);
}
/* }}} */

// ing3, 动态调用
static void zend_compile_dynamic_call(znode *result, znode *name_node, zend_ast *args_ast, uint32_t lineno) /* {{{ */
{
	// 如果方法名是常量 并且 类型是string
	if (name_node->op_type == IS_CONST && Z_TYPE(name_node->u.constant) == IS_STRING) {
		const char *colon;
		// 函数名
		zend_string *str = Z_STR(name_node->u.constant);
		// 调用类的静态方法： 函数名里有冒号 并且 :不是第一个字符 并且 是连续两个冒号
		if ((colon = zend_memrchr(ZSTR_VAL(str), ':', ZSTR_LEN(str))) != NULL && colon > ZSTR_VAL(str) && *(colon - 1) == ':') {
			// 类名
			zend_string *class = zend_string_init(ZSTR_VAL(str), colon - ZSTR_VAL(str) - 1, 0);
			// 方法名
			zend_string *method = zend_string_init(colon + 1, ZSTR_LEN(str) - (colon - ZSTR_VAL(str)) - 1, 0);
			// 创建操作码
			zend_op *opline = get_next_op();
			// 操作码 ZEND_INIT_STATIC_METHOD_CALL
			opline->opcode = ZEND_INIT_STATIC_METHOD_CALL;
			// 第1 个运算对象是常量，值是类名
			opline->op1_type = IS_CONST;
			opline->op1.constant = zend_add_class_name_literal(class);
			// 第2 个运算对象是常量，值是方法名
			opline->op2_type = IS_CONST;
			opline->op2.constant = zend_add_func_name_literal(method);
			// 2个slot分别 给 class 和 method
			/* 2 slots, for class and method */
			// 给 CG(active_op_array) 添加缓存大小，添加 count 个void*指针的大小, 返回缓存大小
			opline->result.num = zend_alloc_cache_slots(2);
			// 销毁函数名
			zval_ptr_dtor(&name_node->u.constant);
		// 调用普通函数
		} else {
			// 创建操作码
			zend_op *opline = get_next_op();
			// ZEND_INIT_FCALL_BY_NAME
			opline->opcode = ZEND_INIT_FCALL_BY_NAME;
			// 第二个操作对象类型是常量，值是 函数名
			opline->op2_type = IS_CONST;
			opline->op2.constant = zend_add_func_name_literal(str);
			// 给 CG(active_op_array) 添加缓存大小，添加 一个 void*指针的大小，返回添加后的缓存大小
			opline->result.num = zend_alloc_cache_slot();
		}
	// 否则 创建操作码 ZEND_INIT_DYNAMIC_CALL
	} else {
		zend_emit_op(NULL, ZEND_INIT_DYNAMIC_CALL, NULL, name_node);
	}

	// 调用方法
	zend_compile_call_common(result, args_ast, NULL, lineno);
}
/* }}} */

// ing4, 检测是否包含 ... 或 带参数名的参数
static inline bool zend_args_contain_unpack_or_named(zend_ast_list *args) /* {{{ */
{
	uint32_t i;
	// 遍历所有参数
	for (i = 0; i < args->children; ++i) {
		zend_ast *arg = args->child[i];
		// 如果类型是打包参数，或类型是命名参数，返回 true
		if (arg->kind == ZEND_AST_UNPACK || arg->kind == ZEND_AST_NAMED_ARG) {
			return 1;
		}
	}
	return 0;
}
/* }}} */

// ing3, 特殊函数，php的 strlen 函数
static zend_result zend_compile_func_strlen(znode *result, zend_ast_list *args) /* {{{ */
{
	znode arg_node;

	if (args->children != 1) {
		return FAILURE;
	}
	// 编译表达式语句, p1:返回结果，p2:表达式语句
	zend_compile_expr(&arg_node, args->child[0]);
	// 常量直接计算长度
	if (arg_node.op_type == IS_CONST && Z_TYPE(arg_node.u.constant) == IS_STRING) {
		result->op_type = IS_CONST;
		ZVAL_LONG(&result->u.constant, Z_STRLEN(arg_node.u.constant));
		zval_ptr_dtor_str(&arg_node.u.constant);
	// 变量才需要用操作码计算
	} else {
		zend_emit_op_tmp(result, ZEND_STRLEN, &arg_node, NULL);
	}
	return SUCCESS;
}
/* }}} */

// ing3, 特殊函数，php的 类型判断函数
// is_null: IS_NULL. is_bool:_IS_BOOL. is_long,is_int,is_integer:IS_LONG. is_double: IS_DOUBLE. 
// is_string: IS_STRING. is_array: IS_ARRAY .is_object: IS_OBJECT. is_resource: IS_RESOURCE.
static zend_result zend_compile_func_typecheck(znode *result, zend_ast_list *args, uint32_t type) /* {{{ */
{
	znode arg_node;
	zend_op *opline;

	if (args->children != 1) {
		return FAILURE;
	}
	// 编译表达式语句, p1:返回结果，p2:表达式语句
	zend_compile_expr(&arg_node, args->child[0]);
	// 创建操作码
	opline = zend_emit_op_tmp(result, ZEND_TYPE_CHECK, &arg_node, NULL);
	// 把具体检查的类型存入 extended_value
	// 检查其它类型
	if (type != _IS_BOOL) {
		opline->extended_value = (1 << type);
	// 检查bool类型
	} else {
		opline->extended_value = (1 << IS_FALSE) | (1 << IS_TRUE);
	}
	return SUCCESS;
}
/* }}} */

// ing3, 特殊函数，php的 is_scalar 函数 ：检查是否是标量
// 规则：true,false,double,long,string 都算标量 （测试过）
static zend_result zend_compile_func_is_scalar(znode *result, zend_ast_list *args) /* {{{ */
{
	znode arg_node;
	zend_op *opline;
	
	// 必须是1个参数
	if (args->children != 1) {
		return FAILURE;
	}
	// 编译表达式语句, p1:返回结果，p2:表达式语句
	zend_compile_expr(&arg_node, args->child[0]);
	// 创建操作码
	opline = zend_emit_op_tmp(result, ZEND_TYPE_CHECK, &arg_node, NULL);
	// 规则：true,false,double,long,string 都算标量
	opline->extended_value = (1 << IS_FALSE | 1 << IS_TRUE | 1 << IS_DOUBLE | 1 << IS_LONG | 1 << IS_STRING);
	return SUCCESS;
}

// ing3, 特殊函数，php的 类型转换函数
// boolval: _IS_BOOL. intval: IS_LONG. floatval,doubleval: IS_DOUBLE. strval: IS_STRING
static zend_result zend_compile_func_cast(znode *result, zend_ast_list *args, uint32_t type) /* {{{ */
{
	znode arg_node;
	zend_op *opline;

	// 必须是1个参数
	if (args->children != 1) {
		return FAILURE;
	}
	// 编译表达式语句, p1:返回结果，p2:表达式语句
	zend_compile_expr(&arg_node, args->child[0]);
	// 创建操作码
	// bool类型
	if (type == _IS_BOOL) {
		opline = zend_emit_op_tmp(result, ZEND_BOOL, &arg_node, NULL);
	// 其它类型，记录需要转换的类型
	} else {
		opline = zend_emit_op_tmp(result, ZEND_CAST, &arg_node, NULL);
		opline->extended_value = type;
	}
	return SUCCESS;
}
/* }}} */

// ing2, 特殊函数，php的 defined
static zend_result zend_compile_func_defined(znode *result, zend_ast_list *args) /* {{{ */
{
	zend_string *name;
	zend_op *opline;

	// 必须是一个函数，类型是内置变量
	if (args->children != 1 || args->child[0]->kind != ZEND_AST_ZVAL) {
		return FAILURE;
	}
	// 取得要校验的变量名
	name = zval_get_string(zend_ast_get_zval(args->child[0]));
	// 如果包含 \ 或者 : 直接报错。经测试，没有这个限制
	if (zend_memrchr(ZSTR_VAL(name), '\\', ZSTR_LEN(name)) || zend_memrchr(ZSTR_VAL(name), ':', ZSTR_LEN(name))) {
		// 释放函数名
		zend_string_release_ex(name, 0);
		return FAILURE;
	}

	// try 把常量名当成语句直接执行，如果不出错，结果返回到 result->u.constant
	if (zend_try_ct_eval_const(&result->u.constant, name, 0)) {
		zend_string_release_ex(name, 0);
		// 删除 result->u.constant 里的临时常量
		zval_ptr_dtor(&result->u.constant);
		// result->u.constant 设置成 true
		ZVAL_TRUE(&result->u.constant);
		// 记录类型
		result->op_type = IS_CONST;
		return SUCCESS;
	}
	// 如果执行出错，创建操作码 ZEND_DEFINED
	opline = zend_emit_op_tmp(result, ZEND_DEFINED, NULL, NULL);
	// 第一个操作对象是常量
	opline->op1_type = IS_CONST;
	// 添加literal，并把返回的literal序号存放到 op.constant里
	LITERAL_STR(opline->op1, name);
	// 给 CG(active_op_array) 添加缓存大小，添加 一个 void*指针的大小，返回添加后的缓存大小
	opline->extended_value = zend_alloc_cache_slot();

	return SUCCESS;
}
/* }}} */

// ing2, 特殊函数，php的 chr, 通过ascii 码获取字符
static zend_result zend_compile_func_chr(znode *result, zend_ast_list *args) /* {{{ */
{
	// 必须是一个参数，内置变量，长整型。否则报错
	if (args->children == 1 &&
	    args->child[0]->kind == ZEND_AST_ZVAL &&
	    Z_TYPE_P(zend_ast_get_zval(args->child[0])) == IS_LONG) {
		// 参数强转成 long 型
		zend_long c = Z_LVAL_P(zend_ast_get_zval(args->child[0])) & 0xff;
		// 返回结果是常量
		result->op_type = IS_CONST;
		// 保存返回结果 
		ZVAL_CHAR(&result->u.constant, c);
		return SUCCESS;
	// 实际上php的报错可能不是这里返回的，因为这里没有error 信息
	} else {
		return FAILURE;
	}
}
/* }}} */

// ing2, 特殊函数，php的 chr, 通过字符 获取 ascii 码
static zend_result zend_compile_func_ord(znode *result, zend_ast_list *args) /* {{{ */
{
	// 必须是一个参数，内置变量，string型。否则报错
	if (args->children == 1 &&
	    args->child[0]->kind == ZEND_AST_ZVAL &&
	    Z_TYPE_P(zend_ast_get_zval(args->child[0])) == IS_STRING) {
		// 返回结果是常量
		result->op_type = IS_CONST;
		// 保存返回结果 
		ZVAL_LONG(&result->u.constant, (unsigned char)Z_STRVAL_P(zend_ast_get_zval(args->child[0]))[0]);
		return SUCCESS;
	} else {
		return FAILURE;
	}
}
/* }}} */

// ing3, 不是用户定义的函数 或 有 ZEND_ACC_DONE_PASS_TWO 标记
// 只能计算已经完整编译过的方法的 堆栈大小，否则还会继续添加附加的 CV 或 TMP slots。
// 它防止了直接或间接调用短路方法
/* We can only calculate the stack size for functions that have been fully compiled, otherwise
 * additional CV or TMP slots may still be added. This prevents the use of INIT_FCALL for
 * directly or indirectly recursive function calls. */
static bool fbc_is_finalized(zend_function *fbc) {
	// 如果是内置方法 或 有 ZEND_ACC_DONE_PASS_TWO 标记，返回 true
	// #define ZEND_USER_CODE(type) ((type) != ZEND_INTERNAL_FUNCTION)
	return !ZEND_USER_CODE(fbc->type) || (fbc->common.fn_flags & ZEND_ACC_DONE_PASS_TWO);
}

// ing2, ?, zend_compile_init_user_func 一处调用
static zend_result zend_try_compile_ct_bound_init_user_func(zend_ast *name_ast, uint32_t num_args) /* {{{ */
{
	zend_string *name, *lcname;
	zend_function *fbc;
	zend_op *opline;

	// 如果语句类型不是常量 或 不是string，返回 FAILURE
	if (name_ast->kind != ZEND_AST_ZVAL || Z_TYPE_P(zend_ast_get_zval(name_ast)) != IS_STRING) {
		return FAILURE;
	}

	// 函数名
	name = zend_ast_get_str(name_ast);
	// 小写函数名
	lcname = zend_string_tolower(name);

	// 取得函数表中的函数指针（在哈希表中查找指针对象）
	fbc = zend_hash_find_ptr(CG(function_table), lcname);
	// 如果函数指针存在 或 ？？
	if (!fbc || !fbc_is_finalized(fbc)
	// 或 （类型是内置函数 并且 配置要求忽略内置函数 ）
	 || (fbc->type == ZEND_INTERNAL_FUNCTION && (CG(compiler_options) & ZEND_COMPILE_IGNORE_INTERNAL_FUNCTIONS))
	// 或 （类型是用户函数 并且 配置要求忽略用户函数 ）
	 || (fbc->type == ZEND_USER_FUNCTION && (CG(compiler_options) & ZEND_COMPILE_IGNORE_USER_FUNCTIONS))
	// 或 （类型是用户函数 并且 配置要求忽略其他文件 并且 函数在其他文件中）
	 || (fbc->type == ZEND_USER_FUNCTION && (CG(compiler_options) & ZEND_COMPILE_IGNORE_OTHER_FILES) && fbc->op_array.filename != CG(active_op_array)->filename)
	) {
		// 释放函数名
		zend_string_release_ex(lcname, 0);
		return FAILURE;
	}
	// 创建操作码 ZEND_INIT_FCALL
	opline = zend_emit_op(NULL, ZEND_INIT_FCALL, NULL, NULL);
	// 操作码扩展信息里存放 参数数量
	opline->extended_value = num_args;
	// ？通过vm 计算已使用的堆栈高度？
	opline->op1.num = zend_vm_calc_used_stack(num_args, fbc);
	// 第二个操作对象
	opline->op2_type = IS_CONST;
	// 添加literal，并把返回的literal序号存放到 op.constant里
	LITERAL_STR(opline->op2, lcname);
	// 给 CG(active_op_array) 添加缓存大小，添加 一个 void*指针的大小，返回添加后的缓存大小
	opline->result.num = zend_alloc_cache_slot();

	return SUCCESS;
}
/* }}} */

// ing2, 编译初始化用户函数
static void zend_compile_init_user_func(zend_ast *name_ast, uint32_t num_args, zend_string *orig_func_name) /* {{{ */
{
	zend_op *opline;
	znode name_node;
	// ？
	if (zend_try_compile_ct_bound_init_user_func(name_ast, num_args) == SUCCESS) {
		return;
	}

	// 编译表达式语句, p1:返回结果，p2:表达式语句
	zend_compile_expr(&name_node, name_ast);

	// 创建操作码 ZEND_INIT_USER_CALL
	opline = zend_emit_op(NULL, ZEND_INIT_USER_CALL, NULL, &name_node);
	// 第一个操作对象，类型为常量，复制原函数名
	opline->op1_type = IS_CONST;
	// 添加literal，并把返回的literal序号存放到 op.constant里
	LITERAL_STR(opline->op1, zend_string_copy(orig_func_name));
	// 操作码扩展信息里存放参数数量
	opline->extended_value = num_args;
}
/* }}} */

// ing2, 特殊函数，php的 call_user_func_array, 调用指定函数，参数在一个数组里
/* cufa = call_user_func_array */
static zend_result zend_compile_func_cufa(znode *result, zend_ast_list *args, zend_string *lcname) /* {{{ */
{
	znode arg_node;
	zend_op *opline;

	// 必须是2个参数
	if (args->children != 2) {
		return FAILURE;
	}

	zend_compile_init_user_func(args->child[0], 0, lcname);
	// 第二个参数，类型必须是 调用函数或方法
	if (args->child[1]->kind == ZEND_AST_CALL
	// 第一个参数，类型必须是内置变量
	 && args->child[1]->child[0]->kind == ZEND_AST_ZVAL
	// 第一个参数第一个子元素 类型必须是 string
	 && Z_TYPE_P(zend_ast_get_zval(args->child[1]->child[0])) == IS_STRING
	// 第二个参数的第二个子元素 类型必须是参数列表
	 && args->child[1]->child[1]->kind == ZEND_AST_ARG_LIST) {
		// 传入的方法名
		zend_string *orig_name = zend_ast_get_str(args->child[1]->child[0]);
		// 参数列表
		zend_ast_list *list = zend_ast_get_list(args->child[1]->child[1]);
		// 是否是带命名空间的完整路径
		bool is_fully_qualified;
		// 把用户传入的函数名转成 运行时的函数名, 大小写不敏感
		zend_string *name = zend_resolve_function_name(orig_name, args->child[1]->child[0]->attr, &is_fully_qualified);

		// 如果调用 array_slice 方法 （特殊处理）
		if (zend_string_equals_literal_ci(name, "array_slice")
		// ?
	     && !zend_args_contain_unpack_or_named(list)
		// 参数是3个
		 && list->children == 3
		// 第二个参数类型是内置变量
		 && list->child[1]->kind == ZEND_AST_ZVAL) {
			// 第二个参数
			zval *zv = zend_ast_get_zval(list->child[1]);
			// 如果地址有效
			if (Z_TYPE_P(zv) == IS_LONG
			 && Z_LVAL_P(zv) >= 0
			 && Z_LVAL_P(zv) <= 0x7fffffff) {
				zend_op *opline;
				znode len_node;
				// 编译表达式语句, p1:返回结果，p2:表达式语句
				zend_compile_expr(&arg_node, list->child[0]);
				// 编译表达式语句, p1:返回结果，p2:表达式语句
				zend_compile_expr(&len_node, list->child[2]);
				// 创建操作码：ZEND_SEND_ARRAY
				opline = zend_emit_op(NULL, ZEND_SEND_ARRAY, &arg_node, &len_node);
				opline->extended_value = Z_LVAL_P(zv);
				// 创建操作码：ZEND_DO_FCALL
				zend_emit_op(result, ZEND_DO_FCALL, NULL, NULL);
				zend_string_release_ex(name, 0);
				return SUCCESS;
			}
		}
		zend_string_release_ex(name, 0);
	}
	// 编译表达式语句, p1:返回结果，p2:表达式语句
	zend_compile_expr(&arg_node, args->child[1]);
	// 创建操作码：ZEND_SEND_ARRAY，ZEND_CHECK_UNDEF_ARGS，
	zend_emit_op(NULL, ZEND_SEND_ARRAY, &arg_node, NULL);
	zend_emit_op(NULL, ZEND_CHECK_UNDEF_ARGS, NULL, NULL);
	// 创建操作码：ZEND_DO_FCALL
	opline = zend_emit_op(result, ZEND_DO_FCALL, NULL, NULL);
	opline->extended_value = ZEND_FCALL_MAY_HAVE_EXTRA_NAMED_PARAMS;

	return SUCCESS;
}
/* }}} */

// ing2, 特殊函数，php的 call_user_func, 调用指定函数，参数分别传入
/* cuf = call_user_func */
static zend_result zend_compile_func_cuf(znode *result, zend_ast_list *args, zend_string *lcname) /* {{{ */
{
	uint32_t i;

	if (args->children < 1) {
		return FAILURE;
	}
	// ？初始化指定函数
	zend_compile_init_user_func(args->child[0], args->children - 1, lcname);
	// 遍历所有参数
	for (i = 1; i < args->children; ++i) {
		zend_ast *arg_ast = args->child[i];
		znode arg_node;
		zend_op *opline;
		// 编译表达式语句, p1:返回结果，p2:表达式语句
		zend_compile_expr(&arg_node, arg_ast);
		// 创建操作码：ZEND_SEND_USER
		opline = zend_emit_op(NULL, ZEND_SEND_USER, &arg_node, NULL);
		// 操作码序号
		opline->op2.num = i;
		// ？
		opline->result.var = EX_NUM_TO_VAR(i - 1);
	}
	// 创建操作码：ZEND_DO_FCALL
	zend_emit_op(result, ZEND_DO_FCALL, NULL, NULL);

	return SUCCESS;
}
/* }}} */

// ing3, （尝试测试）编译 assert 语句，fbc 回调函数？ zend_compile_call 中两次调用
// PHP的assert函数最多2个参数，第二个参数类型是 Throwable|string|null
static void zend_compile_assert(znode *result, zend_ast_list *args, zend_string *name, zend_function *fbc, uint32_t lineno) /* {{{ */
{
	// 如果配置文件里开启了 assertion 选项, 否则一律返回true
	if (EG(assertions) >= 0) {
		znode name_node;
		zend_op *opline;
		// 最新行号：前上下文中的操作码数量
		uint32_t check_op_number = get_next_op_number();
		// 创建操作码：ZEND_ASSERT_CHECK
		zend_emit_op(NULL, ZEND_ASSERT_CHECK, NULL, NULL);

		// 如果有函数指针 并且 fbc_is_finalized？
		if (fbc && fbc_is_finalized(fbc)) {
			// 函数名类型为常量
			name_node.op_type = IS_CONST;
			// 复制函数名
			ZVAL_STR_COPY(&name_node.u.constant, name);
			
			// 创建操作码 ZEND_INIT_FCALL
			opline = zend_emit_op(NULL, ZEND_INIT_FCALL, NULL, &name_node);
		} else {
			// 创建操作码 ZEND_INIT_NS_FCALL_BY_NAME
			opline = zend_emit_op(NULL, ZEND_INIT_NS_FCALL_BY_NAME, NULL, NULL);
			// 第二个操作对象类型为常量
			opline->op2_type = IS_CONST;
			// 值为添加了命名空间的函数名
			opline->op2.constant = zend_add_ns_func_name_literal(name);
		}
		// 给 CG(active_op_array) 添加缓存大小，添加 一个 void*指针的大小，返回添加后的缓存大小
		opline->result.num = zend_alloc_cache_slot();

		// 如果有1个参数
		if (args->children == 1) {
			// 创建断言 提示信息
			/* add "assert(condition) as assertion message */
			zend_ast *arg = zend_ast_create_zval_from_str(
				// 导出 php 语句
				zend_ast_export("assert(", args->child[0], ")"));
			// 如果包含命名参数
			if (args->child[0]->kind == ZEND_AST_NAMED_ARG) {
				// 如果原始参数有名称，和命名参数一样添加新参数，因为不能同时用命名和非命名参数
				/* If the original argument was named, add the new argument as named as well,
				 * as mixing named and positional is not allowed. */
				// 用str 创建内置变量
				zend_ast *name = zend_ast_create_zval_from_str(
					zend_string_init("description", sizeof("description") - 1, 0));
				// 创建语句，类型为命名参数 ZEND_AST_NAMED_ARG
				arg = zend_ast_create(ZEND_AST_NAMED_ARG, name, arg);
			}
			// 参数添加到参数列表中
			zend_ast_list_add((zend_ast *) args, arg);
		}

		// 编译函数调用
		zend_compile_call_common(result, (zend_ast*)args, fbc, lineno);

		// 最新操作码
		opline = &CG(active_op_array)->opcodes[check_op_number];
		// 第二个操作对象存放 最新行号：前上下文中的操作码数量
		opline->op2.opline_num = get_next_op_number();
		// 
		SET_NODE(opline->result, result);
	// 
	} else {
		// 如果没有函数指针
		if (!fbc) {
			// 释放函数名
			zend_string_release_ex(name, 0);
		}
		// 结果类型为常量 值为 true
		result->op_type = IS_CONST;
		ZVAL_TRUE(&result->u.constant);
	}
}
/* }}} */

// ing2, 特殊函数，php的 in_array
static zend_result zend_compile_func_in_array(znode *result, zend_ast_list *args) /* {{{ */
{
	bool strict = 0;
	znode array, needly;
	zend_op *opline;
	
	// 如果是3个参数
	if (args->children == 3) {
		if (args->child[2]->kind == ZEND_AST_ZVAL) {
			strict = zend_is_true(zend_ast_get_zval(args->child[2]));
		} else if (args->child[2]->kind == ZEND_AST_CONST) {
			zval value;
			zend_ast *name_ast = args->child[2]->child[0];
			bool is_fully_qualified;
			// 完整的常量名
			zend_string *resolved_name = zend_resolve_const_name(
				zend_ast_get_str(name_ast), name_ast->attr, &is_fully_qualified);

			if (!zend_try_ct_eval_const(&value, resolved_name, is_fully_qualified)) {
				zend_string_release_ex(resolved_name, 0);
				return FAILURE;
			}

			zend_string_release_ex(resolved_name, 0);
			strict = zend_is_true(&value);
			zval_ptr_dtor(&value);
		} else {
			return FAILURE;
		}
	// 如果不是三个或2个参数，报错
	} else if (args->children != 2) {
		return FAILURE;
	}

	// 如果第二个参数不是 数组或 ？
	if (args->child[1]->kind != ZEND_AST_ARRAY
	 || !zend_try_ct_eval_array(&array.u.constant, args->child[1])) {
		return FAILURE;
	}

	if (zend_hash_num_elements(Z_ARRVAL(array.u.constant)) > 0) {
		bool ok = 1;
		zval *val, tmp;
		HashTable *src = Z_ARRVAL(array.u.constant);
		HashTable *dst = zend_new_array(zend_hash_num_elements(src));

		ZVAL_TRUE(&tmp);

		if (strict) {
			ZEND_HASH_FOREACH_VAL(src, val) {
				if (Z_TYPE_P(val) == IS_STRING) {
					zend_hash_add(dst, Z_STR_P(val), &tmp);
				} else if (Z_TYPE_P(val) == IS_LONG) {
					zend_hash_index_add(dst, Z_LVAL_P(val), &tmp);
				} else {
					zend_array_destroy(dst);
					ok = 0;
					break;
				}
			} ZEND_HASH_FOREACH_END();
		} else {
			ZEND_HASH_FOREACH_VAL(src, val) {
				if (Z_TYPE_P(val) != IS_STRING
				 || is_numeric_string(Z_STRVAL_P(val), Z_STRLEN_P(val), NULL, NULL, 0)) {
					zend_array_destroy(dst);
					ok = 0;
					break;
				}
				zend_hash_add(dst, Z_STR_P(val), &tmp);
			} ZEND_HASH_FOREACH_END();
		}

		zend_array_destroy(src);
		if (!ok) {
			return FAILURE;
		}
		Z_ARRVAL(array.u.constant) = dst;
	}
	array.op_type = IS_CONST;
	// 编译表达式语句, p1:返回结果，p2:表达式语句
	zend_compile_expr(&needly, args->child[0]);

	opline = zend_emit_op_tmp(result, ZEND_IN_ARRAY, &needly, &array);
	opline->extended_value = strict;

	return SUCCESS;
}
/* }}} */

// ing3, 特殊函数，php的 count 和 sizeof 函数, (都只能应用于array)
static zend_result zend_compile_func_count(znode *result, zend_ast_list *args, zend_string *lcname) /* {{{ */
{
	znode arg_node;
	zend_op *opline;
	// 必须有一个参数
	if (args->children != 1) {
		return FAILURE;
	}

	// 编译表达式语句, p1:返回结果，p2:表达式语句
	zend_compile_expr(&arg_node, args->child[0]);
	// 创建临时操作码, COUNT 要转成操作码来运算，这里result 没有算好的结果 
	// 实际操作码： 
		//L0003 0001 T2 = COUNT CV0($a)
		//L0003 0002 FREE T2
		//L0003 0003 RETURN int(1)
	opline = zend_emit_op_tmp(result, ZEND_COUNT, &arg_node, NULL);
	// 返回Bool, 是否是sizeof 函数
	opline->extended_value = zend_string_equals_literal(lcname, "sizeof");

	return SUCCESS;
}
/* }}} */

// ing2, 特殊函数，php的 get_class 函数. 两个用途，获取指定对象的所属类，或者当前所在类名
static zend_result zend_compile_func_get_class(znode *result, zend_ast_list *args) /* {{{ */
{
	// 必须不能有参数
	if (args->children == 0) {
		zend_emit_op_tmp(result, ZEND_GET_CLASS, NULL, NULL);
	} else {
		znode arg_node;
		// ？
		if (args->children != 1) {
			return FAILURE;
		}
		// 编译表达式语句, p1:返回结果，p2:表达式语句
		zend_compile_expr(&arg_node, args->child[0]);
		// 创建操作码 ZEND_GET_CLASS
		zend_emit_op_tmp(result, ZEND_GET_CLASS, &arg_node, NULL);
	}
	return SUCCESS;
}
/* }}} */

// ing3, 特殊函数，php的 get_called_class 函数
static zend_result zend_compile_func_get_called_class(znode *result, zend_ast_list *args) /* {{{ */
{
	// 必须不能有参数
	if (args->children != 0) {
		return FAILURE;
	}
	// 创建操作码 ZEND_GET_CALLED_CLASS
	zend_emit_op_tmp(result, ZEND_GET_CALLED_CLASS, NULL, NULL);
	return SUCCESS;
}
/* }}} */

// ing3, 特殊函数，php的 gettype 函数
static zend_result zend_compile_func_gettype(znode *result, zend_ast_list *args) /* {{{ */
{
	znode arg_node;
	
	// 必须是1个参数
	if (args->children != 1) {
		return FAILURE;
	}
	// 编译表达式语句, p1:返回结果，p2:表达式语句
	zend_compile_expr(&arg_node, args->child[0]);
	// 创建操作码：ZEND_GET_TYPE
	zend_emit_op_tmp(result, ZEND_GET_TYPE, &arg_node, NULL);
	return SUCCESS;
}
/* }}} */

// ing2, 特殊函数，php的 func_num_args 函数
static zend_result zend_compile_func_num_args(znode *result, zend_ast_list *args) /* {{{ */
{
	// 同下面
	if (CG(active_op_array)->function_name && args->children == 0) {
		// 创建操作码 ZEND_FUNC_GET_ARGS
		zend_emit_op_tmp(result, ZEND_FUNC_NUM_ARGS, NULL, NULL);
		return SUCCESS;
	} else {
		return FAILURE;
	}
}
/* }}} */

// ing2, 特殊函数，php的 func_get_args 函数
static zend_result zend_compile_func_get_args(znode *result, zend_ast_list *args) /* {{{ */
{
	// args->children == 0 是什么
	// 必须已经在函数中，否则报错
	if (CG(active_op_array)->function_name && args->children == 0) {
		// 创建操作码 ZEND_FUNC_GET_ARGS
		zend_emit_op_tmp(result, ZEND_FUNC_GET_ARGS, NULL, NULL);
		return SUCCESS;
	} else {
		return FAILURE;
	}
}
/* }}} */

// ing3, 特殊函数，php的 array_key_exists 函数
static zend_result zend_compile_func_array_key_exists(znode *result, zend_ast_list *args) /* {{{ */
{
	znode subject, needle;
	// 必须要2个参数
	if (args->children != 2) {
		return FAILURE;
	}
	// 编译表达式语句, p1:返回结果，p2:表达式语句
	zend_compile_expr(&needle, args->child[0]);
	// 编译表达式语句, p1:返回结果，p2:表达式语句
	zend_compile_expr(&subject, args->child[1]);
	// 创带带2个参数的操作码 ZEND_ARRAY_KEY_EXISTS
	zend_emit_op_tmp(result, ZEND_ARRAY_KEY_EXISTS, &needle, &subject);
	return SUCCESS;
}
/* }}} */

// ing3, 处理 array_slice调用的一个特殊情况：第一个参数使用了 func_get_args，要多创建一个操作码 ZEND_FUNC_GET_ARGS
static zend_result zend_compile_func_array_slice(znode *result, zend_ast_list *args) /* {{{ */
{
	// 如果当前在函数里，有函数名
	if (CG(active_op_array)->function_name
	// 并且有 2 个参数
	 && args->children == 2
	// 参数类型是调用函数 
	 && args->child[0]->kind == ZEND_AST_CALL
	// 第一个参数 第一个子元素类型是 内置变量
	 && args->child[0]->child[0]->kind == ZEND_AST_ZVAL
	// 第一个参数 第一个子元素是 string
	 && Z_TYPE_P(zend_ast_get_zval(args->child[0]->child[0])) == IS_STRING
	// 第一个参数的第 2 个子元素类型是参数表
	 && args->child[0]->child[1]->kind == ZEND_AST_ARG_LIST
	// 第二个参数是内置变量
	 && args->child[1]->kind == ZEND_AST_ZVAL) {
		 
		// 取出第一参数的第一个子元素的值（函数 名）
		zend_string *orig_name = zend_ast_get_str(args->child[0]->child[0]);
		// 完整路径
		bool is_fully_qualified;
		// 把用户传入的函数名转成 运行时的函数名, 大小写不敏感
		zend_string *name = zend_resolve_function_name(orig_name, args->child[0]->child[0]->attr, &is_fully_qualified);
		// 子语句1，的子语句1，转成列表
		zend_ast_list *list = zend_ast_get_list(args->child[0]->child[1]);
		// 取得第二个子元素，offset
		zval *zv = zend_ast_get_zval(args->child[1]);
		// 创建操作码用的临时变量 ？
		znode first;

		// 如果函数是  func_get_args
		if (zend_string_equals_literal_ci(name, "func_get_args")
		// 并且 没有给它传参数
		 && list->children == 0
		// offset 参数是 整数 
		 && Z_TYPE_P(zv) == IS_LONG
		// offset 参数是 整数 >= 0
		 && Z_LVAL_P(zv) >= 0) {
			// 第一个
			first.op_type = IS_CONST;
			// offset存到 first 里
			ZVAL_LONG(&first.u.constant, Z_LVAL_P(zv));
			// 创建操作码 ZEND_FUNC_GET_ARGS （主要是这里，要多创建一个操作码）
			zend_emit_op_tmp(result, ZEND_FUNC_GET_ARGS, &first, NULL);
			// 释放方法名 
			zend_string_release_ex(name, 0);
			// 返回
			return SUCCESS;
		}
		// 释放函数名
		zend_string_release_ex(name, 0);
	}
	// 以上只是做一种特殊情况处理，就算没有那种情况，也不会导致报错
	return FAILURE;
}
/* }}} */

// ing3, 编译特殊函数，这里返回 FAILURE 不会报错， zend_compile_call 一处调用
static zend_result zend_try_compile_special_func(znode *result, zend_string *lcname, zend_ast_list *args, zend_function *fbc, uint32_t type) /* {{{ */
{
	// 如果有 ZEND_COMPILE_NO_BUILTINS 选项, 返回 FAILURE
	if (CG(compiler_options) & ZEND_COMPILE_NO_BUILTINS) {
		return FAILURE;
	}

	// 如果函数指针类型不是内置函数, 返回 FAILURE
	if (fbc->type != ZEND_INTERNAL_FUNCTION) {
		// 如果函数被禁用，它可以被用户重新字义并实现。这种情况下不使用 VM 创建内置函数。
		/* If the function is part of disabled_functions, it may be redeclared as a userland
		 * function with a different implementation. Don't use the VM builtin in that case. */
		return FAILURE;
	}

	// 如果参数中有打包参数或命名参数, 返回 FAILURE
	if (zend_args_contain_unpack_or_named(args)) {
		return FAILURE;
	}
	// ing3, 特殊函数，php的  函数
	if (zend_string_equals_literal(lcname, "strlen")) {
		return zend_compile_func_strlen(result, args);
	// 通过 zend_compile_func_typecheck 实现, 
	// is_null: IS_NULL. is_bool:_IS_BOOL. is_long,is_int,is_integer:IS_LONG. is_double: IS_DOUBLE. 
	// is_string: IS_STRING. is_array: IS_ARRAY .is_object: IS_OBJECT. is_resource: IS_RESOURCE.
	} else if (zend_string_equals_literal(lcname, "is_null")) {
		return zend_compile_func_typecheck(result, args, IS_NULL);
	} else if (zend_string_equals_literal(lcname, "is_bool")) {
		return zend_compile_func_typecheck(result, args, _IS_BOOL);
	} else if (zend_string_equals_literal(lcname, "is_long")
		|| zend_string_equals_literal(lcname, "is_int")
		|| zend_string_equals_literal(lcname, "is_integer")
	) {
		return zend_compile_func_typecheck(result, args, IS_LONG);
	} else if (zend_string_equals_literal(lcname, "is_float")
		|| zend_string_equals_literal(lcname, "is_double")
	) {
		return zend_compile_func_typecheck(result, args, IS_DOUBLE);
	} else if (zend_string_equals_literal(lcname, "is_string")) {
		return zend_compile_func_typecheck(result, args, IS_STRING);
	} else if (zend_string_equals_literal(lcname, "is_array")) {
		return zend_compile_func_typecheck(result, args, IS_ARRAY);
	} else if (zend_string_equals_literal(lcname, "is_object")) {
		return zend_compile_func_typecheck(result, args, IS_OBJECT);
	} else if (zend_string_equals_literal(lcname, "is_resource")) {
		return zend_compile_func_typecheck(result, args, IS_RESOURCE);
	// 	以上 调用 zend_compile_func_typecheck
	} else if (zend_string_equals_literal(lcname, "is_scalar")) {
		return zend_compile_func_is_scalar(result, args);
	// 通过 zend_compile_func_cast 实现：
	// boolval: _IS_BOOL. intval: IS_LONG. floatval,doubleval: IS_DOUBLE. strval: IS_STRING
	} else if (zend_string_equals_literal(lcname, "boolval")) {
		return zend_compile_func_cast(result, args, _IS_BOOL);
	} else if (zend_string_equals_literal(lcname, "intval")) {
		return zend_compile_func_cast(result, args, IS_LONG);
	} else if (zend_string_equals_literal(lcname, "floatval")
		|| zend_string_equals_literal(lcname, "doubleval")
	) {
		return zend_compile_func_cast(result, args, IS_DOUBLE);
	} else if (zend_string_equals_literal(lcname, "strval")) {
		return zend_compile_func_cast(result, args, IS_STRING);
	// 	以上 调用 zend_compile_func_cast
	} else if (zend_string_equals_literal(lcname, "defined")) {
		return zend_compile_func_defined(result, args);
	} else if (zend_string_equals_literal(lcname, "chr") && type == BP_VAR_R) {
		return zend_compile_func_chr(result, args);
	} else if (zend_string_equals_literal(lcname, "ord") && type == BP_VAR_R) {
		return zend_compile_func_ord(result, args);
	} else if (zend_string_equals_literal(lcname, "call_user_func_array")) {
		return zend_compile_func_cufa(result, args, lcname);
	} else if (zend_string_equals_literal(lcname, "call_user_func")) {
		return zend_compile_func_cuf(result, args, lcname);
	} else if (zend_string_equals_literal(lcname, "in_array")) {
		return zend_compile_func_in_array(result, args);
	} else if (zend_string_equals_literal(lcname, "count")
			|| zend_string_equals_literal(lcname, "sizeof")) {
		return zend_compile_func_count(result, args, lcname);
	} else if (zend_string_equals_literal(lcname, "get_class")) {
		return zend_compile_func_get_class(result, args);
	} else if (zend_string_equals_literal(lcname, "get_called_class")) {
		return zend_compile_func_get_called_class(result, args);
	} else if (zend_string_equals_literal(lcname, "gettype")) {
		return zend_compile_func_gettype(result, args);
	} else if (zend_string_equals_literal(lcname, "func_num_args")) {
		return zend_compile_func_num_args(result, args);
	} else if (zend_string_equals_literal(lcname, "func_get_args")) {
		return zend_compile_func_get_args(result, args);
	} else if (zend_string_equals_literal(lcname, "array_slice")) {
		return zend_compile_func_array_slice(result, args);
	} else if (zend_string_equals_literal(lcname, "array_key_exists")) {
		return zend_compile_func_array_key_exists(result, args);
	} else {
		return FAILURE;
	}
}
/* }}} */

// ing3, 调用函数，1处调用 ， zend_compile_var_inner
static void zend_compile_call(znode *result, zend_ast *ast, uint32_t type) /* {{{ */
{
	// 函数名
	zend_ast *name_ast = ast->child[0];
	// 参数列表
	zend_ast *args_ast = ast->child[1];
	// 是否是把函数转成闭包
	bool is_callable_convert = args_ast->kind == ZEND_AST_CALLABLE_CONVERT;

	znode name_node;

	// 如果名称不是内置变量 或 类型不是 string
	if (name_ast->kind != ZEND_AST_ZVAL || Z_TYPE_P(zend_ast_get_zval(name_ast)) != IS_STRING) {
		// 编译表达式语句, p1:返回结果，p2:表达式语句
		zend_compile_expr(&name_node, name_ast);
		// 动态调用函数 
		zend_compile_dynamic_call(result, &name_node, args_ast, ast->lineno);
		return;
	}

	{
		// 编译函数名，返回：是否是命名空间中的函数
		bool runtime_resolution = zend_compile_function_name(&name_node, name_ast);
		// 如果是 是否是命名空间中的函数
		if (runtime_resolution) {
			// 如果函数名是 assert
			if (zend_string_equals_literal_ci(zend_ast_get_str(name_ast), "assert")
					&& !is_callable_convert) {
				// 编译 assert
				zend_compile_assert(result, zend_ast_get_list(args_ast), Z_STR(name_node.u.constant), NULL, ast->lineno);
			// 其他函数 
			} else {
				// 编译带命名空间的函数调用
				zend_compile_ns_call(result, &name_node, args_ast, ast->lineno);
			}
			return;
		}
	}

	{
		// 
		zval *name = &name_node.u.constant;
		zend_string *lcname;
		zend_function *fbc;
		zend_op *opline;
		// 小写函数名
		lcname = zend_string_tolower(Z_STR_P(name));
		// 函数指针
		fbc = zend_hash_find_ptr(CG(function_table), lcname);

		// 特殊的 assert句柄可以在 编译标记里 独立应用
		/* Special assert() handling should apply independently of compiler flags. */
		// 如果有函数指针 并且 函数名是 assert 并且不是 转换成闭包
		if (fbc && zend_string_equals_literal(lcname, "assert") && !is_callable_convert) {
			// 编译 assert  ？
			zend_compile_assert(result, zend_ast_get_list(args_ast), lcname, fbc, ast->lineno);
			// 释放小写函数我
			zend_string_release(lcname);
			// 销毁函数名常量
			zval_ptr_dtor(&name_node.u.constant);
			return;
		}
		// 如果函数指针不存在 或 函数有final 标记
		if (!fbc || !fbc_is_finalized(fbc)
		// 或 （ 是内置函数 并且有 忽略内置函数 选项）
		 || (fbc->type == ZEND_INTERNAL_FUNCTION && (CG(compiler_options) & ZEND_COMPILE_IGNORE_INTERNAL_FUNCTIONS))
		// 或 （ 是用户函数 并且有 忽略用户函数 选项） 
		 || (fbc->type == ZEND_USER_FUNCTION && (CG(compiler_options) & ZEND_COMPILE_IGNORE_USER_FUNCTIONS))
		// 或 （ 是用户函数 并且有 忽略其他文件 选项 并且 函数在其他文件中）  
		 || (fbc->type == ZEND_USER_FUNCTION && (CG(compiler_options) & ZEND_COMPILE_IGNORE_OTHER_FILES) && fbc->op_array.filename != CG(active_op_array)->filename)
		) {
			// 释放函数名
			zend_string_release_ex(lcname, 0);
			// 动态编译函数
			zend_compile_dynamic_call(result, &name_node, args_ast, ast->lineno);
			return;
		}

		// 如果不是转成闭包
		if (!is_callable_convert &&
			// 并且编译特殊函数成功
		    zend_try_compile_special_func(result, lcname,
				zend_ast_get_list(args_ast), fbc, type) == SUCCESS
		) {
			// 销毁小写名字
			zend_string_release_ex(lcname, 0);
			// 销毁名字节点中的常量
			zval_ptr_dtor(&name_node.u.constant);
			return;
		}
		
		// 不成功也能正常处理
		
		// 销毁名字节点中的函数名
		zval_ptr_dtor(&name_node.u.constant);
		// 小写函数名放到名字节点中
		ZVAL_NEW_STR(&name_node.u.constant, lcname);
		// 创建操作码 初始化调用函数 
		opline = zend_emit_op(NULL, ZEND_INIT_FCALL, NULL, &name_node);
		// 给 CG(active_op_array) 添加缓存大小，添加 一个 void*指针的大小，返回添加后的缓存大小
		opline->result.num = zend_alloc_cache_slot();

		// 调用方法
		zend_compile_call_common(result, args_ast, fbc, ast->lineno);
	}
}
/* }}} */

// ing3, 编译方法调用
static void zend_compile_method_call(znode *result, zend_ast *ast, uint32_t type) /* {{{ */
{
	// 对象语句
	zend_ast *obj_ast = ast->child[0];
	// 方法语句
	zend_ast *method_ast = ast->child[1];
	// 参数列表语句
	zend_ast *args_ast = ast->child[2];

	znode obj_node, method_node;
	zend_op *opline;
	zend_function *fbc = NULL;
	// 是否使用了短路调用 $a?->b();
	bool nullsafe = ast->kind == ZEND_AST_NULLSAFE_METHOD_CALL;
	// 短路监测点，返回短路操作码堆栈深度
	uint32_t short_circuiting_checkpoint = zend_short_circuiting_checkpoint();

	// 对象是 $this
	if (is_this_fetch(obj_ast)) {
		// 如果 $this 一定存在
		if (this_guaranteed_exists()) {
			// 对象节点操作类型为 IS_UNUSED
			obj_node.op_type = IS_UNUSED;
		// 否则 
		} else {
			// 创建操作码 ZEND_FETCH_THIS
			zend_emit_op(&obj_node, ZEND_FETCH_THIS, NULL, NULL);
		}
		// 操作码列表添加标记 ZEND_ACC_USES_THIS
		CG(active_op_array)->fn_flags |= ZEND_ACC_USES_THIS;

		/* We will throw if $this doesn't exist, so there's no need to emit a JMP_NULL
		 * check for a nullsafe access. */
	// 对象不是 $this
	} else {
		// 如果是可以短路的类型， 添加短路标记
		zend_short_circuiting_mark_inner(obj_ast);
		// 编译表达式语句, p1:返回结果，p2:表达式语句
		zend_compile_expr(&obj_node, obj_ast);
		// 如果用到短路
		if (nullsafe) {
			// ing3, 创建 ZEND_JMP_NULL 操作码
			zend_emit_jmp_null(&obj_node, type);
		}
	}

	// 编译表达式语句, p1:返回结果，p2:表达式语句
	zend_compile_expr(&method_node, method_ast);
	// 创建操作码 ZEND_INIT_METHOD_CALL
	opline = zend_emit_op(NULL, ZEND_INIT_METHOD_CALL, &obj_node, NULL);

	// 如果方法名类型是常量
	if (method_node.op_type == IS_CONST) {
		// 如果方法名 不是 字串，报错
		if (Z_TYPE(method_node.u.constant) != IS_STRING) {
			zend_error_noreturn(E_COMPILE_ERROR, "Method name must be a string");
		}

		// 第二个操作对象类型是常量
		opline->op2_type = IS_CONST;
		// 值为方法名
		opline->op2.constant = zend_add_func_name_literal(
			Z_STR(method_node.u.constant));
		// 给 CG(active_op_array) 添加缓存大小，添加 count 个void*指针的大小, 返回缓存大小
		opline->result.num = zend_alloc_cache_slots(2);
	// 方法名不是常量
	} else {
		SET_NODE(opline->op2, &method_node);
	}

	// 检查是否在调用 $this 的已经方法
	/* Check if this calls a known method on $this */
	if (opline->op1_type == IS_UNUSED && opline->op2_type == IS_CONST &&
			// 并且 在类方法中，或者在【被引用到类里的】trait 的方法中
			CG(active_class_entry) && zend_is_scope_known()) {
		// ？, 获取方法名：当前操作码列表中 第 (node).constant 个 literal
		zend_string *lcname = Z_STR_P(CT_CONSTANT(opline->op2) + 1);
		// 获取方法名对应的方法
		fbc = zend_hash_find_ptr(&CG(active_class_entry)->function_table, lcname);

		// 只有当方法是 private 或 final时，才能获取到准备的方法名（因为不会被重载）
		/* We only know the exact method that is being called if it is either private or final.
		 * Otherwise an overriding method in a child class may be called. */
		// 如果是 private 或 final
		if (fbc && !(fbc->common.fn_flags & (ZEND_ACC_PRIVATE|ZEND_ACC_FINAL))) {
			fbc = NULL;
		}
	}

	// 调用方法，如果成功
	if (zend_compile_call_common(result, args_ast, fbc, zend_ast_get_lineno(method_ast))) {
		// 检查短路监测点，zend_short_circuiting_checkpoint返回短路操作码堆栈深度，如果不相符
		if (short_circuiting_checkpoint != zend_short_circuiting_checkpoint()) {
			// 不可以滥用短路和创建 闭包
			zend_error_noreturn(E_COMPILE_ERROR,
				"Cannot combine nullsafe operator with Closure creation");
		}
	}
}
/* }}} */

// ing3, 是否是构造方法
static bool zend_is_constructor(zend_string *name) /* {{{ */
{
	// 名字是否是 “__construct”
	return zend_string_equals_literal_ci(name, ZEND_CONSTRUCTOR_FUNC_NAME);
}
/* }}} */

// ing3，获取兼容的方法名或 null， zend_compile_static_call 一次调用
static zend_function *zend_get_compatible_func_or_null(zend_class_entry *ce, zend_string *lcname) /* {{{ */
{
	// 在类的方法列表里找方法名
	zend_function *fbc = zend_hash_find_ptr(&ce->function_table, lcname);
	// 如果没找到 或 找到的是 public类型 或 类入口与当前类入口一致
	if (!fbc || (fbc->common.fn_flags & ZEND_ACC_PUBLIC) || ce == CG(active_class_entry)) {
		// 返回方法指针
		return fbc;
	}

	// 如果方法不是 private类型 并且 与此方法的类有继承关系 并且 （当前不在类中 或 当前类有继承关系 ） 并且 通过 protect 检测
	if (!(fbc->common.fn_flags & ZEND_ACC_PRIVATE)
		&& (fbc->common.scope->ce_flags & ZEND_ACC_LINKED)
		&& (!CG(active_class_entry) || (CG(active_class_entry)->ce_flags & ZEND_ACC_LINKED))
		// /Zend/zend_object_handlers.h	中的方法
		&& zend_check_protected(zend_get_function_root_class(fbc), CG(active_class_entry))) {
		return fbc;
	}

	return NULL;
}
/* }}} */

// ing3, 编译静态方法调用
static void zend_compile_static_call(znode *result, zend_ast *ast, uint32_t type) /* {{{ */
{
	// 类名语句
	zend_ast *class_ast = ast->child[0];
	// 方法名语句
	zend_ast *method_ast = ast->child[1];
	// 参数列表语句
	zend_ast *args_ast = ast->child[2];

	znode class_node, method_node;
	zend_op *opline;
	zend_function *fbc = NULL;
	// 如果是可以短路的类型， 添加短路标记
	zend_short_circuiting_mark_inner(class_ast);
	// 编译类引用
	zend_compile_class_ref(&class_node, class_ast, ZEND_FETCH_CLASS_EXCEPTION);

	// 编译表达式语句, p1:返回结果，p2:表达式语句
	zend_compile_expr(&method_node, method_ast);

	// 如果方法名是常量
	if (method_node.op_type == IS_CONST) {
		zval *name = &method_node.u.constant;
		// 如果方法名不是字串，报错（测试过 $a = new StdClass();$b = 1;$a->$b();）
		if (Z_TYPE_P(name) != IS_STRING) {
			zend_error_noreturn(E_COMPILE_ERROR, "Method name must be a string");
		}
		// 如果方法名是 __construct()
		if (zend_is_constructor(Z_STR_P(name))) {
			// 销毁方法名
			zval_ptr_dtor(name);
			// 方法标记成 IS_UNUSED
			method_node.op_type = IS_UNUSED;
		}
	}

	// 创建操作码
	opline = get_next_op();
	// 类型为 ZEND_INIT_STATIC_METHOD_CALL
	opline->opcode = ZEND_INIT_STATIC_METHOD_CALL;

	// 把类名放到 opline->op1 里
	zend_set_class_name_op1(opline, &class_node);

	// 如果方法名是常量
	if (method_node.op_type == IS_CONST) {
		// 第二个操作对象类型为常量
		opline->op2_type = IS_CONST;
		// 值为方法名
		opline->op2.constant = zend_add_func_name_literal(
			Z_STR(method_node.u.constant));
		// 给 CG(active_op_array) 添加缓存大小，添加 count 个void*指针的大小, 返回缓存大小
		opline->result.num = zend_alloc_cache_slots(2);
	} else {
		// 如果第一个操作对象类型为常量
		if (opline->op1_type == IS_CONST) {
			// 给 CG(active_op_array) 添加缓存大小，添加 一个 void*指针的大小，返回添加后的缓存大小
			opline->result.num = zend_alloc_cache_slot();
		}
		// 
		SET_NODE(opline->op2, &method_node);
	}

	// 如果已经知道了要调用的方法
	/* Check if we already know which method we're calling */
	// 如果方法名已经是常量了
	if (opline->op2_type == IS_CONST) {
		zend_class_entry *ce = NULL;
		// 如果第一个操作对象类型是常量（类名）
		if (opline->op1_type == IS_CONST) {
			// 取得小写类名
			zend_string *lcname = Z_STR_P(CT_CONSTANT(opline->op1) + 1);
			// 获取类入口
			ce = zend_hash_find_ptr(CG(class_table), lcname);
			// 如果无法获取类入口 并且 当前在类中 并且 类名与当前类名相同
			if (!ce && CG(active_class_entry)
					&& zend_string_equals_ci(CG(active_class_entry)->name, lcname)) {
				// 应用当前类入口
				ce = CG(active_class_entry);
			}
		// 如果是构造方法 并且（通过self调用） 并且 已经有作用域
		} else if (opline->op1_type == IS_UNUSED
				&& (opline->op1.num & ZEND_FETCH_CLASS_MASK) == ZEND_FETCH_CLASS_SELF
				// 并且 在类方法中，或者在【被引用到类里的】trait 的方法中
				&& zend_is_scope_known()) {
			// 应用当前类入口
			ce = CG(active_class_entry);
		}
		// 如果类入口存在
		if (ce) {
			// ？
			zend_string *lcname = Z_STR_P(CT_CONSTANT(opline->op2) + 1);
			// 获取兼容的方法名或 null
			fbc = zend_get_compatible_func_or_null(ce, lcname);
		}
	}
	// 调用方法
	zend_compile_call_common(result, args_ast, fbc, zend_ast_get_lineno(method_ast));
}
/* }}} */

// 编译类声明
static void zend_compile_class_decl(znode *result, zend_ast *ast, bool toplevel);

// ing3, 编译 new 语句
static void zend_compile_new(znode *result, zend_ast *ast) /* {{{ */
{
	// 类名
	zend_ast *class_ast = ast->child[0];
	// 参数列表
	zend_ast *args_ast = ast->child[1];

	znode class_node, ctor_result;
	zend_op *opline;

	// 如果 class_ast 是一个类声名语句
	if (class_ast->kind == ZEND_AST_CLASS) {
		// 匿名类
		/* anon class declaration */
		// 编译类声名语句
		zend_compile_class_decl(&class_node, class_ast, 0);
	// 普通类
	} else {
		// 编译类引用语句
		zend_compile_class_ref(&class_node, class_ast, ZEND_FETCH_CLASS_EXCEPTION);
	}

	// 创建操作码 new
	opline = zend_emit_op(result, ZEND_NEW, NULL, NULL);

	// 如果类名是常量
	if (class_node.op_type == IS_CONST) {
		opline->op1_type = IS_CONST;
		// 类名添加到 literal 列表里
		opline->op1.constant = zend_add_class_name_literal(
			Z_STR(class_node.u.constant));
		// 给 CG(active_op_array) 添加缓存大小，添加 一个 void*指针的大小，返回添加后的缓存大小
		opline->op2.num = zend_alloc_cache_slot();
	// 类名不是常量，存到第一个操作对象里
	} else {
		SET_NODE(opline->op1, &class_node);
	}

	// 调构造用方法（这个是必须的吗？）
	zend_compile_call_common(&ctor_result, args_ast, NULL, ast->lineno);
	// 释放结果 
	zend_do_free(&ctor_result);
}
/* }}} */

// ing4, 编译 clone 语句,（可以是语句或函数）
static void zend_compile_clone(znode *result, zend_ast *ast) /* {{{ */
{
	// 被克隆对象
	zend_ast *obj_ast = ast->child[0];

	znode obj_node;
	// 编译表达式语句, p1:返回结果，p2:表达式语句
	zend_compile_expr(&obj_node, obj_ast);
	// 创建操作码 ZEND_CLONE，临时变量
	zend_emit_op_tmp(result, ZEND_CLONE, &obj_node, NULL);
}
/* }}} */

// ing3，编译global语句中的每一个变量, 多个变量名会分多次编译
// global语句只能设置变量，不能同时赋值， global $a=1; 报语法错误。
static void zend_compile_global_var(zend_ast *ast) /* {{{ */
{
	// 单个变量名语句
	zend_ast *var_ast = ast->child[0];
	// 变量名，简单变量名由 $+变量名组成，如$name, ${name} $$name 不可以加 ->, [] 
	zend_ast *name_ast = var_ast->child[0];

	// 两个node,一个放变量名，一个放结果
	znode name_node, result;

	// 编译表达式语句, p1:返回结果，p2:表达式语句
	zend_compile_expr(&name_node, name_ast);
	// 如果结果是常量，直接转字串
	if (name_node.op_type == IS_CONST) {
		convert_to_string(&name_node.u.constant);
	}
	// global $GLOBALS; 未禁用，也无效果
	// TODO(GLOBALS) Forbid "global $GLOBALS"?
	
	// 如果语句是从 $this 获取属性（测试过：global $this）
	if (is_this_fetch(var_ast)) {
		zend_error_noreturn(E_COMPILE_ERROR, "Cannot use $this as global variable");
	// 如果编译变量成功
	} else if (zend_try_compile_cv(&result, var_ast) == SUCCESS) {
		// 创建操作码 ZEND_BIND_GLOBAL
		zend_op *opline = zend_emit_op(NULL, ZEND_BIND_GLOBAL, &result, &name_node);
		// 给 CG(active_op_array) 添加缓存大小，添加 一个 void*指针的大小，返回添加后的缓存大小
		opline->extended_value = zend_alloc_cache_slot();
	} else {
		// 只计算名称语。？
		/* name_ast should be evaluated only. FETCH_GLOBAL_LOCK instructs FETCH_W
		 * to not free the name_node operand, so it can be reused in the following
		 * ASSIGN_REF, which then frees it. */
		// 创建 ZEND_FETCH_W 操作码
		zend_op *opline = zend_emit_op(&result, ZEND_FETCH_W, &name_node, NULL);
		// 添加标记 ZEND_FETCH_GLOBAL_LOCK
		opline->extended_value = ZEND_FETCH_GLOBAL_LOCK;

		// 如果名称是常量
		if (name_node.op_type == IS_CONST) {
			// 字串添加引用次数
			zend_string_addref(Z_STR(name_node.u.constant));
		}

		// 创建一个地址赋值语句，用 var_ast 接收 znode 的地址
		zend_emit_assign_ref_znode(
			// 用名称 创建 声名变量语句
			zend_ast_create(ZEND_AST_VAR, zend_ast_create_znode(&name_node)),
			&result
		);
	}
}
/* }}} */

// ing2, 编译静态变量，可能在类或闭包里用到
static void zend_compile_static_var_common(zend_string *var_name, zval *value, uint32_t mode) /* {{{ */
{
	zend_op *opline;
	// 如果当前操作码列表中无静态变量
	if (!CG(active_op_array)->static_variables) {
		// 如果当前有作用域
		if (CG(active_op_array)->scope) {
			// 添加标记 ZEND_HAS_STATIC_IN_METHODS
			CG(active_op_array)->scope->ce_flags |= ZEND_HAS_STATIC_IN_METHODS;
		}
		// 创建静态变量哈希表
		CG(active_op_array)->static_variables = zend_new_array(8);
	}
	// 在哈希表中，更新静态变量
	value = zend_hash_update(CG(active_op_array)->static_variables, var_name, value);

	// 不可以把$this用作静态变量
	if (zend_string_equals_literal(var_name, "this")) {
		zend_error_noreturn(E_COMPILE_ERROR, "Cannot use $this as static variable");
	}
	
	// 创建操作码 ZEND_BIND_STATIC
	opline = zend_emit_op(NULL, ZEND_BIND_STATIC, NULL, NULL);
	// 类型是编译变量
	opline->op1_type = IS_CV;
	// 绑定值 = 查找编译变量，如果没有的话创建一个新的并返回序号
	opline->op1.var = lookup_cv(var_name);
	// ？
	opline->extended_value = (uint32_t)((char*)value - (char*)CG(active_op_array)->static_variables->arData) | mode;
}
/* }}} */

// ing4, 编函数中的译静态变量，不是静态成员变量哈！
static void zend_compile_static_var(zend_ast *ast) /* {{{ */
{
	// 变量名
	zend_ast *var_ast = ast->child[0];
	// 变量值
	zend_ast **value_ast_ptr = &ast->child[1];
	//
	zval value_zv;

	// 如果有值，值只能是常量表达式
	if (*value_ast_ptr) {
		// 计算常量表达式
		zend_const_expr_to_zval(&value_zv, value_ast_ptr, /* allow_dynamic */ true);
	} else {
		// 值为null
		ZVAL_NULL(&value_zv);
	}
	// 编译静态变量，公用部分
	zend_compile_static_var_common(zend_ast_get_str(var_ast), &value_zv, ZEND_BIND_REF);
}
/* }}} */

// ing4, 编译 unset
static void zend_compile_unset(zend_ast *ast) /* {{{ */
{
	zend_ast *var_ast = ast->child[0];
	znode var_node;
	zend_op *opline;
	// 确定变量可写
	zend_ensure_writable_variable(var_ast);

	// 如果是全局变量
	if (is_global_var_fetch(var_ast)) {
		// 如果没有键名，报错
		if (!var_ast->child[1]) {
			zend_error_noreturn(E_COMPILE_ERROR, "Cannot use [] for unsetting");
		}
		// 编译表达式语句, p1:返回结果，p2:表达式语句
		zend_compile_expr(&var_node, var_ast->child[1]);
		// 如果键名是常量，转成string
		if (var_node.op_type == IS_CONST) {
			convert_to_string(&var_node.u.constant);
		}
		// 创建操作码 ZEND_UNSET_VAR
		opline = zend_emit_op(NULL, ZEND_UNSET_VAR, &var_node, NULL);
		// 扩展标记 ZEND_FETCH_GLOBAL
		opline->extended_value = ZEND_FETCH_GLOBAL;
		return;
	}

	// 根据类型处理
	switch (var_ast->kind) {
		// 普通变量
		case ZEND_AST_VAR:
			// 如果是$this, 报错
			if (is_this_fetch(var_ast)) {
				zend_error_noreturn(E_COMPILE_ERROR, "Cannot unset $this");
			// 如果可以编译目标变量
			} else if (zend_try_compile_cv(&var_node, var_ast) == SUCCESS) {
				// 创建操作码 ZEND_UNSET_CV
				opline = zend_emit_op(NULL, ZEND_UNSET_CV, &var_node, NULL);
			} else {
				// 编译简单变量，不使用CV
				opline = zend_compile_simple_var_no_cv(NULL, var_ast, BP_VAR_UNSET, 0);
				opline->opcode = ZEND_UNSET_VAR;
			}
			return;
		// 数组元素
		case ZEND_AST_DIM:
			// 编译变量：删除变量 BP_VAR_UNSET
			opline = zend_compile_dim(NULL, var_ast, BP_VAR_UNSET, /* by_ref */ false);
			// 创建操作码 ZEND_UNSET_DIM
			opline->opcode = ZEND_UNSET_DIM;
			return;
		// 允空的类属性
		case ZEND_AST_PROP:
		case ZEND_AST_NULLSAFE_PROP:
			// 编译类属性：删除变量 BP_VAR_UNSET
			opline = zend_compile_prop(NULL, var_ast, BP_VAR_UNSET, 0);
			// 创建操作码 ZEND_UNSET_OBJ
			opline->opcode = ZEND_UNSET_OBJ;
			return;
		// 类的静态属性
		case ZEND_AST_STATIC_PROP:
			// 编译类静态属性：删除变量 BP_VAR_UNSET
			opline = zend_compile_static_prop(NULL, var_ast, BP_VAR_UNSET, 0, 0);
			// 创建操作码 ZEND_UNSET_STATIC_PROP
			opline->opcode = ZEND_UNSET_STATIC_PROP;
			return;
		// 其它
		EMPTY_SWITCH_DEFAULT_CASE()
	}
}
/* }}} */

// 2次调用
// ing3, 处理循环 和 finally。p1:层数，p2:返回值
static bool zend_handle_loops_and_finally_ex(zend_long depth, znode *return_value) /* {{{ */
{
	zend_loop_var *base;
	// 取得循环变量堆栈的最上一个元素
	zend_loop_var *loop_var = zend_stack_top(&CG(loop_var_stack));

	// 如果没有循环变量
	if (!loop_var) {
		// 返回true
		return 1;
	}
	// 返回堆栈元素列表的指针
	base = zend_stack_base(&CG(loop_var_stack));
	// 倒序遍历所有元素
	for (; loop_var >= base; loop_var--) {
		// 如果操作码是 ZEND_FAST_CALL
		if (loop_var->opcode == ZEND_FAST_CALL) {
			// 创建操作码
			zend_op *opline = get_next_op();
			// 类型 ZEND_FAST_CALL
			opline->opcode = ZEND_FAST_CALL;
			// 结果类型为 临时变量
			opline->result_type = IS_TMP_VAR;
			// 结果值为 变量数量
			opline->result.var = loop_var->var_num;
			// 如果有返回值
			if (return_value) {
				// 返回值放到操作码里
				SET_NODE(opline->op2, return_value);
			}
			// 
			opline->op1.num = loop_var->try_catch_offset;
		// 如果变量类型为 ZEND_DISCARD_EXCEPTION （放弃异常）
		} else if (loop_var->opcode == ZEND_DISCARD_EXCEPTION) {
			// 创建操作码
			zend_op *opline = get_next_op();
			// ZEND_DISCARD_EXCEPTION 操作码
			opline->opcode = ZEND_DISCARD_EXCEPTION;
			// 类型： 临时变量
			opline->op1_type = IS_TMP_VAR;
			// 变量编号 
			opline->op1.var = loop_var->var_num;
		// 如果循环变量的操作码是 ZEND_RETURN
		} else if (loop_var->opcode == ZEND_RETURN) {
			// 堆栈 分离器
			/* Stack separator */
			// 跳出
			break;
		// 如果层数 <=1
		} else if (depth <= 1) {
			// 返回true
			return 1;
		// 如果是 ZEND_NOP 操作码
		} else if (loop_var->opcode == ZEND_NOP) {
			// 循环里没有可释放的变量
			/* Loop doesn't have freeable variable */
			// 跳出一层
			depth--;
		// 其他情况
		} else {
			zend_op *opline;
			// 循环变量必须是 变量 或 临时变量
			ZEND_ASSERT(loop_var->var_type & (IS_VAR|IS_TMP_VAR));
			// 新建操作码
			opline = get_next_op();
			// 循环变量中的操作码
			opline->opcode = loop_var->opcode;
			// 循环变量的类型
			opline->op1_type = loop_var->var_type;
			// 循环变量的变量序号
			opline->op1.var = loop_var->var_num;
			// 扩展操作码 为 ZEND_FREE_ON_RETURN
			opline->extended_value = ZEND_FREE_ON_RETURN;
			// 跳出1层
			depth--;
	    }
	}
	
	// 返回：深度是否为 0
	return (depth == 0);
}
/* }}} */


// ing3, 处理loop和finally, 2次调用
static bool zend_handle_loops_and_finally(znode *return_value) /* {{{ */
{
	// 处理循环 和 finally。p1:层数，p2:返回值
	return zend_handle_loops_and_finally_ex(zend_stack_count(&CG(loop_var_stack)) + 1, return_value);
}
/* }}} */

// ing3, 判断是否有 finally 语句, p1:深度
// zend_has_finally 调用
static bool zend_has_finally_ex(zend_long depth) /* {{{ */
{
	zend_loop_var *base;
	// 取得循环堆栈的最上一个变量
	zend_loop_var *loop_var = zend_stack_top(&CG(loop_var_stack));

	// 如果深度为空，返回 0
	if (!loop_var) {
		return 0;
	}
	// 返回堆栈元素列表的指针
	base = zend_stack_base(&CG(loop_var_stack));
	// 倒着遍历所有元素
	for (; loop_var >= base; loop_var--) {
		// 碰到 ZEND_FAST_CALL，返回 1
		if (loop_var->opcode == ZEND_FAST_CALL) {
			return 1;
		// 跳过  ZEND_DISCARD_EXCEPTION
		} else if (loop_var->opcode == ZEND_DISCARD_EXCEPTION) {
		// 碰到 ZEND_RETURN，返回 1
		} else if (loop_var->opcode == ZEND_RETURN) {
			/* Stack separator */
			return 0;
		// 深度 到底，返回0
		} else if (depth <= 1) {
			return 0;
		// 其他情况下一个元素
		} else {
			depth--;
	    }
	}
	return 0;
}
/* }}} */

// ing3, 判断是否有 finally 语句, zend_compile_return 一处调用
static bool zend_has_finally(void) /* {{{ */
{
	// 判断是否有 finally 语句, p1:深度
	return zend_has_finally_ex(zend_stack_count(&CG(loop_var_stack)) + 1);
}
/* }}} */

// ing2, 编译return 
static void zend_compile_return(zend_ast *ast) /* {{{ */
{
	// 表达式语句
	zend_ast *expr_ast = ast->child[0];
	// 是否在 yield 语句中
	bool is_generator = (CG(active_op_array)->fn_flags & ZEND_ACC_GENERATOR) != 0;
	// 是否返回引用地址
	bool by_ref = (CG(active_op_array)->fn_flags & ZEND_ACC_RETURN_REFERENCE) != 0;

	znode expr_node;
	zend_op *opline;

	// 如果在 yield 语句中，不允许返回引用
	if (is_generator) {
		// ？
		/* For generators the by-ref flag refers to yields, not returns */
		by_ref = 0;
	}

	// 如果没有表达式
	if (!expr_ast) {
		// 返回类型为常量，值为null
		expr_node.op_type = IS_CONST;
		ZVAL_NULL(&expr_node.u.constant);
	// 要求返回引用，并且值是变量：php变量，数组元素，对象属性，允空对象属性，静态属性
	} else if (by_ref && zend_is_variable(expr_ast)) {
		// 如果是短路类型
		if (zend_ast_is_short_circuited(expr_ast)) {
			// 返回引用时不可以使用短路语句，（测试过 function &a(){	$a =  new stdClass();	return $a?->a; } a();）
			zend_error_noreturn(E_COMPILE_ERROR, "Cannot take reference of a nullsafe chain");
		}
		// 编译变量，p1:返回值，p2:语句，p3:操作类型，p4:是否引用传递
		zend_compile_var(&expr_node, expr_ast, BP_VAR_W, 1);
	// 其他情况
	} else {
		// 编译表达式语句, p1:返回结果，p2:表达式语句
		zend_compile_expr(&expr_node, expr_ast);
	}

	// 如果有 finally 语句块 并且 （结果类型是编译变量 或 （要求返回引用地址 并且 结果是变量） ）
	if ((CG(active_op_array)->fn_flags & ZEND_ACC_HAS_FINALLY_BLOCK)
	 && (expr_node.op_type == IS_CV || (by_ref && expr_node.op_type == IS_VAR))
	 && zend_has_finally()) {
		// 把返回值复制到临时变量里 避免 在 finally 语句块中被修改
		/* Copy return value into temporary VAR to avoid modification in finally code */
		// 如果要求返回地址
		if (by_ref) {
			// 创建操作码 ZEND_MAKE_REF
			zend_emit_op(&expr_node, ZEND_MAKE_REF, &expr_node, NULL);
		} else {
			// 创建临时操作码 ZEND_QM_ASSIGN
			zend_emit_op_tmp(&expr_node, ZEND_QM_ASSIGN, &expr_node, NULL);
		}
	}

	// 如果没在yield 语句里 并且 有要求返回类型
	/* Generator return types are handled separately */
	if (!is_generator && (CG(active_op_array)->fn_flags & ZEND_ACC_HAS_RETURN_TYPE)) {
		// 检查函数的返回类型（可能创建操作码）
		zend_emit_return_type_check(
			expr_ast ? &expr_node : NULL, CG(active_op_array)->arg_info - 1, 0);
	}

	// 处理loop和finally
	zend_handle_loops_and_finally((expr_node.op_type & (IS_TMP_VAR | IS_VAR)) ? &expr_node : NULL);

	// 创建操作码 ZEND_RETURN_BY_REF 或 ZEND_RETURN
	opline = zend_emit_op(NULL, by_ref ? ZEND_RETURN_BY_REF : ZEND_RETURN,
		&expr_node, NULL);

	// 如果要求返回地址并且有表达式
	if (by_ref && expr_ast) {
		// 如果是函数调用语句
		if (zend_is_call(expr_ast)) {
			opline->extended_value = ZEND_RETURNS_FUNCTION;
		// 是否是变量：php变量，数组元素，对象属性，允空对象属性，静态属性
		} else if (!zend_is_variable(expr_ast) || zend_ast_is_short_circuited(expr_ast)) {
			opline->extended_value = ZEND_RETURNS_VALUE;
		}
	}
}
/* }}} */

// ing3, 编译echo 语句
static void zend_compile_echo(zend_ast *ast) /* {{{ */
{
	zend_op *opline;
	zend_ast *expr_ast = ast->child[0];

	// 表达式节点
	znode expr_node;
	// 编译表达式语句, p1:返回结果，p2:表达式语句
	zend_compile_expr(&expr_node, expr_ast);
	// 操作码类型 ZEND_ECHO，没有result和op2
	opline = zend_emit_op(NULL, ZEND_ECHO, &expr_node, NULL);
	opline->extended_value = 0;
}
/* }}} */

// ing3, 编译 throw 语句
static void zend_compile_throw(znode *result, zend_ast *ast) /* {{{ */
{
	// throw 后面的表达式
	zend_ast *expr_ast = ast->child[0];

	znode expr_node;
	// 编译表达式语句, p1:返回结果，p2:表达式语句
	zend_compile_expr(&expr_node, expr_ast);

	// 创建操作码 ZEND_THROW
	zend_op *opline = zend_emit_op(NULL, ZEND_THROW, &expr_node, NULL);
	// 如果有接收错误的对象
	if (result) {
		// 标记成：表达式抛错
		/* Mark this as an "expression throw" for opcache. */
		opline->extended_value = ZEND_THROW_IS_EXPR;
		// 结果类型为常量
		result->op_type = IS_CONST;
		// 值为true
		ZVAL_TRUE(&result->u.constant);
	}
}
/* }}} */

// ing2, break 和 continue 语句，很多校验
static void zend_compile_break_continue(zend_ast *ast) /* {{{ */
{
	// 第一个子元素, break 或 continue 后面的数字，跳出层数。
	zend_ast *depth_ast = ast->child[0];

	zend_op *opline;
	zend_long depth;
	// 必须是break 或 continue
	ZEND_ASSERT(ast->kind == ZEND_AST_BREAK || ast->kind == ZEND_AST_CONTINUE);

	// 如果 后面有表达式
	if (depth_ast) {
		zval *depth_zv;
		// 如果表达式不是内置变量
		if (depth_ast->kind != ZEND_AST_ZVAL) {
			// 抛错:只支持int类型（表达式返回值必须是int）
			zend_error_noreturn(E_COMPILE_ERROR, "'%s' operator with non-integer operand "
				"is no longer supported", ast->kind == ZEND_AST_BREAK ? "break" : "continue");
		}
		// 获取表达式的值
		depth_zv = zend_ast_get_zval(depth_ast);
		// 如果不是lang 或 值<1, 抛错（返回值必须是正整数）
		if (Z_TYPE_P(depth_zv) != IS_LONG || Z_LVAL_P(depth_zv) < 1) {
			// 抛错:必须是正数
			zend_error_noreturn(E_COMPILE_ERROR, "'%s' operator accepts only positive integers",
				ast->kind == ZEND_AST_BREAK ? "break" : "continue");
		}

		// 取得层数中的整数 
		depth = Z_LVAL_P(depth_zv);
	// 默认是 1
	} else {
		// 跳出层数
		depth = 1;
	}
	// 如果没有在 循环或switch中调用，抛错
	if (CG(context).current_brk_cont == -1) {
		// 报错：此语句必须在循环和switch上下文中使用
		zend_error_noreturn(E_COMPILE_ERROR, "'%s' not in the 'loop' or 'switch' context",
			ast->kind == ZEND_AST_BREAK ? "break" : "continue");
	// 如果在循环或 switch 中
	} else {
		// 处理循环 和 finally。p1:层数，p2:返回值
		if (!zend_handle_loops_and_finally_ex(depth, NULL)) {
			// 报错：不可以跳出 n 层（测试过，层数过大）
			zend_error_noreturn(E_COMPILE_ERROR, "Cannot '%s' " ZEND_LONG_FMT " level%s",
				ast->kind == ZEND_AST_BREAK ? "break" : "continue",
				depth, depth == 1 ? "" : "s");
		}
	}

	// 如果是continue语句
	if (ast->kind == ZEND_AST_CONTINUE) {
		int d, cur = CG(context).current_brk_cont;
		// 遍历一串 brk_cont
		for (d = depth - 1; d > 0; d--) {
			// 依次验证每个 parent 元素
			cur = CG(context).brk_cont_array[cur].parent;
			// parent 元素必须 有效
			ZEND_ASSERT(cur != -1);
		}
		// 如果在switch中
		if (CG(context).brk_cont_array[cur].is_switch) {
			// 如果深度是1
			if (depth == 1) {
				// 如果没有父 元素（当前是最外层）
				if (CG(context).brk_cont_array[cur].parent == -1) {
					// 报警告：switch 中 continue等同于 break。（测试过）
					zend_error(E_WARNING,
						"\"continue\" targeting switch is equivalent to \"break\"");
				// 有父元素
				} else {
					// 报警告：switch 中 continue等同于 break，是否想跳到第n层？（测试过，两个switch嵌套）
					zend_error(E_WARNING,
						"\"continue\" targeting switch is equivalent to \"break\". " \
						"Did you mean to use \"continue " ZEND_LONG_FMT "\"?",
						depth + 1);
				}
			// 如果深度不是1
			} else {
				// 如果没有父 元素（当前是最外层）
				if (CG(context).brk_cont_array[cur].parent == -1) {
					// 报警告：switch 中 continue等同于 break。
					zend_error(E_WARNING,
						"\"continue " ZEND_LONG_FMT "\" targeting switch is equivalent to \"break " ZEND_LONG_FMT "\"",
						depth, depth);
				} else {
					// 报警告：switch 中 continue等同于 break。
					zend_error(E_WARNING,
						"\"continue " ZEND_LONG_FMT "\" targeting switch is equivalent to \"break " ZEND_LONG_FMT "\". " \
						"Did you mean to use \"continue " ZEND_LONG_FMT "\"?",
						depth, depth, depth + 1);
				}
			}
		}
	}
	// 校验通过，创建操作码。break 语句创建 ZEND_BRK，continue 创建 ZEND_CONT
	opline = zend_emit_op(NULL, ast->kind == ZEND_AST_BREAK ? ZEND_BRK : ZEND_CONT, NULL, NULL);
	// 第一个操作对象里记录 brk_cont 编号
	opline->op1.num = CG(context).current_brk_cont;
	// 第二个操作对象里记录 深度
	opline->op2.num = depth;
}
/* }}} */

// ing2, 完成 goto 跳转到标签的编译
void zend_resolve_goto_label(zend_op_array *op_array, zend_op *opline) /* {{{ */
{
	zend_label *dest;
	// 删除行数？
	int current, remove_oplines = opline->op1.num;
	//
	zval *label;
	// 操作码序号
	uint32_t opnum = opline - op_array->opcodes;

	// 获取操作码组中第 num 个 literal。p1->literals + p2
	label = CT_CONSTANT_EX(op_array, opline->op2.constant);
	// 如果没有 标签 或 找不到这个标签
	if (CG(context).labels == NULL ||
	    (dest = zend_hash_find_ptr(CG(context).labels, Z_STR_P(label))) == NULL
	) {
		// 标记，未完成编译
		CG(in_compilation) = 1;
		// 当前操作码组
		CG(active_op_array) = op_array;
		// 记录操作码行号
		CG(zend_lineno) = opline->lineno;
		// 报错：goto到未定义的标签
		zend_error_noreturn(E_COMPILE_ERROR, "'goto' to undefined label '%s'", Z_STRVAL_P(label));
	}

	// 通过指针销毁 zval（字串）
	zval_ptr_dtor_str(label);
	// 清空label
	ZVAL_NULL(label);

	// 取得操作码扩展信息
	current = opline->extended_value;
	// 如果操作码不是中跳到这个 dest, 找上一层 brk_cont_array
	for (; current != dest->brk_cont; current = CG(context).brk_cont_array[current].parent) {
		// 如果跳到有层数的地方 （循环或 switch里面）
		if (current == -1) {
			// 标记：未完成编译
			CG(in_compilation) = 1;
			// 操作码组作为当前 活跃
			CG(active_op_array) = op_array;
			// 记录操作码行号
			CG(zend_lineno) = opline->lineno;
			// 报错：goto不可以跳到循环或switch里面（测试过）
			zend_error_noreturn(E_COMPILE_ERROR, "'goto' into loop or switch statement is disallowed");
		}
		// 如果 zend_try_catch_element 有开始位置
		if (CG(context).brk_cont_array[current].start >= 0) {
			// 删除行数 -1
			remove_oplines--;
		}
	}

	// 遍历所有 try_catch
	for (current = 0; current < op_array->last_try_catch; ++current) {
		// 当前 try_catch
		zend_try_catch_element *elem = &op_array->try_catch_array[current];
		// 如果 序号大于 当前操作码序号
		if (elem->try_op > opnum) {
			// 跳出
			break;
		}
		// 如果有 finally 操作码 并且 序号小于 finally 操作码
		if (elem->finally_op && opnum < elem->finally_op - 1
			// 并且 （目的操作码序号 > finally 结序号 或 目的操作码序号 < 元素中的try操作码序号）
			&& (dest->opline_num > elem->finally_end || dest->opline_num < elem->try_op)
		) {
			// 删除行数 -1
			remove_oplines--;
		}
	}

	// 操作码 ZEND_JMP
	opline->opcode = ZEND_JMP;
	// 第一个运算对象设置成未使用
	SET_UNUSED(opline->op1);
	// 第二个运算对象设置成未使用
	SET_UNUSED(opline->op2);
	// 第运算结果对象设置成未使用
	SET_UNUSED(opline->result);
	// 第一个运算对象中 存储 跳转目的操作码编号
	opline->op1.opline_num = dest->opline_num;
	// 元扩展信息
	opline->extended_value = 0;

	ZEND_ASSERT(remove_oplines >= 0);
	// 遍历要删除的行
	while (remove_oplines--) {
		// 下一行
		opline--;
		// 操作码设置成 ZEND_NOP，清空操作码的操作对象和结果
		MAKE_NOP(opline);
		// zend_vm_set_opcode_handler : /Zend/zend_vm.h
		// ？
		ZEND_VM_SET_OPCODE_HANDLER(opline);
	}
}
/* }}} */

// ing2, 编译goto语句
static void zend_compile_goto(zend_ast *ast) /* {{{ */
{
	// 跳转标签
	zend_ast *label_ast = ast->child[0];
	znode label_node;
	zend_op *opline;
	// 最新行号：前上下文中的操作码数量
	uint32_t opnum_start = get_next_op_number();

	// 编译表达式语句, p1:返回结果，p2:表达式语句
	zend_compile_expr(&label_node, label_ast);

	// label解决方案 和 解开 调整在 pass two 里 ?
	/* Label resolution and unwinding adjustments happen in pass two. */
	
	// 处理loop和finally
	zend_handle_loops_and_finally(NULL);
	// 创建操作码 ZEND_GOTO
	opline = zend_emit_op(NULL, ZEND_GOTO, NULL, &label_node);
	// 当前上下文中的操作码数量 - opnum_start - 1
	opline->op1.num = get_next_op_number() - opnum_start - 1;
	// 扩展信息存放 跳转点序号
	opline->extended_value = CG(context).current_brk_cont;
}
/* }}} */

// ing3, 编译跳转标签
static void zend_compile_label(zend_ast *ast) /* {{{ */
{
	// 跳转点名字
	zend_string *label = zend_ast_get_str(ast->child[0]);
	zend_label dest;

	// 如果没有跳转标签，创建一组标签
	if (!CG(context).labels) {
		// 创建并初始化哈希表
		ALLOC_HASHTABLE(CG(context).labels);
		zend_hash_init(CG(context).labels, 8, NULL, label_ptr_dtor, 0);
	}
	
	// 目标地址 ？
	dest.brk_cont = CG(context).current_brk_cont;
	// 当前上下文中的操作码数量
	dest.opline_num = get_next_op_number();

	// 在哈希表中创建跳转标签
	if (!zend_hash_add_mem(CG(context).labels, label, &dest, sizeof(zend_label))) {
		// 如果失败，报错
		zend_error_noreturn(E_COMPILE_ERROR, "Label '%s' already defined", ZSTR_VAL(label));
	}
}
/* }}} */

// ing3, 编译while 语句
static void zend_compile_while(zend_ast *ast) /* {{{ */
{
	// 条件语句
	zend_ast *cond_ast = ast->child[0];
	// 代码块
	zend_ast *stmt_ast = ast->child[1];
	znode cond_node;
	uint32_t opnum_start, opnum_jmp, opnum_cond;
	
	// 创建 ZEND_JMP 操作码，跳转到指定编号的操作码：跳转到第一行
	opnum_jmp = zend_emit_jump(0);
	// 开始循环
	zend_begin_loop(ZEND_NOP, NULL, 0);
	// 循环起始行号：当前上下文中的操作码数量
	opnum_start = get_next_op_number();
	// 编译代码块
	zend_compile_stmt(stmt_ast);
	// 当前上下文中的操作码数量
	opnum_cond = get_next_op_number();
	// 更新操作码的跳转行号
	zend_update_jump_target(opnum_jmp, opnum_cond);
	// 编译表达式语句, p1:返回结果，p2:表达式语句
	zend_compile_expr(&cond_node, cond_ast);
	// 创建 按条件(znode) 跳转 操作码，返回新操作码顺序号
	zend_emit_cond_jump(ZEND_JMPNZ, &cond_node, opnum_start);
	// 结束循环
	zend_end_loop(opnum_cond, NULL);
}
/* }}} */

// ing3, 编译 do-while 语句
static void zend_compile_do_while(zend_ast *ast) /* {{{ */
{
	// 代码块
	zend_ast *stmt_ast = ast->child[0];
	// 条件
	zend_ast *cond_ast = ast->child[1];

	znode cond_node;
	uint32_t opnum_start, opnum_cond;
	// 开始循环
	zend_begin_loop(ZEND_NOP, NULL, 0);
	// 当前上下文中的操作码数量
	opnum_start = get_next_op_number();
	// 编译代码块
	zend_compile_stmt(stmt_ast);
	// 当前上下文中的操作码数量
	opnum_cond = get_next_op_number();
	// 编译表达式语句, p1:返回结果，p2:表达式语句
	zend_compile_expr(&cond_node, cond_ast);

	// 创建 按条件(znode) 跳转 操作码，返回新操作码顺序号
	zend_emit_cond_jump(ZEND_JMPNZ, &cond_node, opnum_start);

	// 结束循环
	zend_end_loop(opnum_cond, NULL);
}
/* }}} */

// ing3, 编译表达式列表
static void zend_compile_expr_list(znode *result, zend_ast *ast) /* {{{ */
{
	zend_ast_list *list;
	uint32_t i;
	// 结果类型标记成常量
	result->op_type = IS_CONST;
	// 结果中必须已经有常量
	ZVAL_TRUE(&result->u.constant);

	// 如果没有语句，直接返回
	if (!ast) {
		return;
	}

	// 取得表达式列表
	list = zend_ast_get_list(ast);
	// 遍历每个语句
	for (i = 0; i < list->children; ++i) {
		zend_ast *expr_ast = list->child[i];
		// 先清空result
		zend_do_free(result);
		// 编译表达式语句, p1:返回结果，p2:表达式语句
		zend_compile_expr(result, expr_ast);
	}
}
/* }}} */

// ing3, 编译 for 循环。编译循环类，都只是把代码转成操作码
static void zend_compile_for(zend_ast *ast) /* {{{ */
{
	// 初始化表达式语句
	zend_ast *init_ast = ast->child[0];
	// 条件表达式语句
	zend_ast *cond_ast = ast->child[1];
	// step 表达式语句
	zend_ast *loop_ast = ast->child[2];
	// 代码块
	zend_ast *stmt_ast = ast->child[3];

	znode result;
	uint32_t opnum_start, opnum_jmp, opnum_loop;

	// 编译初始化表达式
	zend_compile_expr_list(&result, init_ast);
	// 销毁结果 
	zend_do_free(&result);

	// 创建 ZEND_JMP 操作码，跳转到指定编号的操作码
	opnum_jmp = zend_emit_jump(0);

	// 开始循环
	zend_begin_loop(ZEND_NOP, NULL, 0);
	// 当前上下文中的操作码数量
	opnum_start = get_next_op_number();
	// 编译代码块
	zend_compile_stmt(stmt_ast);
	// 当前上下文中的操作码数量
	opnum_loop = get_next_op_number();
	// 编译loop表达式列表
	zend_compile_expr_list(&result, loop_ast);
	//
	zend_do_free(&result);
	// 更新指定跳转型操作码，跳转到下一个创建的新操作码 。
	zend_update_jump_target_to_next(opnum_jmp);
	// 编译条件表达式列表
	zend_compile_expr_list(&result, cond_ast);
	// 创建操作码： ZEND_EXT_STMT
	zend_do_extended_stmt();
	// 创建 按条件(znode) 跳转 操作码，返回新操作码顺序号
	zend_emit_cond_jump(ZEND_JMPNZ, &result, opnum_start);
	// 结束循环
	zend_end_loop(opnum_loop, NULL);
}
/* }}} */

// ing3, 编译foreach
static void zend_compile_foreach(zend_ast *ast) /* {{{ */
{
	// 被遍历的内容
	zend_ast *expr_ast = ast->child[0];
	// value 语句
	zend_ast *value_ast = ast->child[1];
	// key 语句
	zend_ast *key_ast = ast->child[2];
	// 代码块
	zend_ast *stmt_ast = ast->child[3];
	// 是否要求传址遍历
	bool by_ref = value_ast->kind == ZEND_AST_REF;
	// 被遍历表达式是否是可写的变量
	// 是否是变量：php变量，数组元素，对象属性，允空对象属性，静态属性。并且可写
	bool is_variable = zend_is_variable(expr_ast) && zend_can_write_to_variable(expr_ast);
	//
	znode expr_node, reset_node, value_node, key_node;
	zend_op *opline;
	uint32_t opnum_reset, opnum_fetch;

	// 如果有 key 语句
	if (key_ast) {
		// 如果要求key是变量地址，报错
		if (key_ast->kind == ZEND_AST_REF) {
			// key 不可以是地址
			zend_error_noreturn(E_COMPILE_ERROR, "Key element cannot be a reference");
		}
		// 如果key是数组键值对 ，报错
		if (key_ast->kind == ZEND_AST_ARRAY) {
			// key不可以是数组
			zend_error_noreturn(E_COMPILE_ERROR, "Cannot use list as key element");
		}
	}

	// 如果是传址遍历，取得原始value
	if (by_ref) {
		value_ast = value_ast->child[0];
	}

	// 如果变量类型是数组， 递归到叶子节点，检查 elem_ast->attr 看是否包含引用
	if (value_ast->kind == ZEND_AST_ARRAY && zend_propagate_list_refs(value_ast)) {
		// 如果包含引用，自动转成传址遍历
		by_ref = 1;
	}

	// 如果是传址遍历 并且是可以遍历的变量
	if (by_ref && is_variable) {
		// 编译变量，p1:返回值，p2:语句，p3:操作类型，p4:是否引用传递
		zend_compile_var(&expr_node, expr_ast, BP_VAR_W, 1);
	// 如果是其他表达式语句
	} else {
		// 编译表达式语句, p1:返回结果，p2:表达式语句
		zend_compile_expr(&expr_node, expr_ast);
	}

	// 如果是传址遍历 
	if (by_ref) {
		// 如果是写入或者调用方法的话，需要插入一个 ZEND_SEPARATE 操作码
		zend_separate_if_call_and_write(&expr_node, expr_ast, BP_VAR_W);
	}
	// 当前上下文中的操作码数量
	opnum_reset = get_next_op_number();
	// 创建操作码 ZEND_FE_RESET_RW 或 ZEND_FE_RESET_R
	opline = zend_emit_op(&reset_node, by_ref ? ZEND_FE_RESET_RW : ZEND_FE_RESET_R, &expr_node, NULL);

	// 开始循环
	zend_begin_loop(ZEND_FE_FREE, &reset_node, 0);
	// 当前上下文中的操作码数量
	opnum_fetch = get_next_op_number();
	// 创建操作码 ZEND_FE_FETCH_RW
	opline = zend_emit_op(NULL, by_ref ? ZEND_FE_FETCH_RW : ZEND_FE_FETCH_R, &reset_node, NULL);

	// $this
	if (is_this_fetch(value_ast)) {
		zend_error_noreturn(E_COMPILE_ERROR, "Cannot re-assign $this");
	// 如果 value子句 是变量
	} else if (value_ast->kind == ZEND_AST_VAR &&
		// 并且可转换成 编译变量
		zend_try_compile_cv(&value_node, value_ast) == SUCCESS) {
		// 
		SET_NODE(opline->op2, &value_node);
	// value子句不是变量或无法转换成 编译变量
	} else {
		opline->op2_type = IS_VAR;
		// 增加变量计数
		opline->op2.var = get_temporary_variable();
		// 
		GET_NODE(&value_node, opline->op2);
		// 数组键值对
		if (value_ast->kind == ZEND_AST_ARRAY) {
			// 
			zend_compile_list_assign(NULL, value_ast, &value_node, value_ast->attr);
		// 引用地址
		} else if (by_ref) {
			// 创建一个地址赋值语句，用 var_ast 接收 znode 的地址
			zend_emit_assign_ref_znode(value_ast, &value_node);
		// 其他情况
		} else {
			// 执行赋值：创建临时赋值语句计算 value_node 的结果，把结果存入 var_ast 用
			zend_emit_assign_znode(value_ast, &value_node);
		}
	}

	// 如果有key语句
	if (key_ast) {
		// 上面创建的 ZEND_FE_FETCH_RW 操作码
		opline = &CG(active_op_array)->opcodes[opnum_fetch];
		// 操作码结果类型 为临时变量
		zend_make_tmp_result(&key_node, opline);
		// 执行赋值：创建临时赋值语句计算 value_node 的结果，把结果存入 var_ast 用
		zend_emit_assign_znode(key_ast, &key_node);
	}

	// 编译代码块
	zend_compile_stmt(stmt_ast);

	// 把 JMP 和 FE_FREE 放到 foreach 开头。放到尾行更好一些，但这个信息目前不可用。
	/* Place JMP and FE_FREE on the line where foreach starts. It would be
	 * better to use the end line, but this information is not available
	 * currently. */
	// 记录语句的行号
	CG(zend_lineno) = ast->lineno;
	// 创建 ZEND_JMP 操作码，跳转到指定编号的操作码
	zend_emit_jump(opnum_fetch);

	// 找到需要重置的操作码
	opline = &CG(active_op_array)->opcodes[opnum_reset];
	// 第二个操作对象中存放，当前上下文中的操作码数量
	opline->op2.opline_num = get_next_op_number();

	// 指定操作码
	opline = &CG(active_op_array)->opcodes[opnum_fetch];
	// 当前上下文中的操作码数量
	opline->extended_value = get_next_op_number();

	// 停止循环
	zend_end_loop(opnum_fetch, &reset_node);

	// 创建操作码 ZEND_FE_FREE
	opline = zend_emit_op(NULL, ZEND_FE_FREE, &reset_node, NULL);
}
/* }}} */

// ing3, 编译 if
static void zend_compile_if(zend_ast *ast) /* {{{ */
{
	zend_ast_list *list = zend_ast_get_list(ast);
	uint32_t i;
	uint32_t *jmp_opnums = NULL;

	// 第一个if以外的条件数量：如果大于1个参数 jmp_opnums 中记录 子语句数 -1。
	if (list->children > 1) {
		jmp_opnums = safe_emalloc(sizeof(uint32_t), list->children - 1, 0);
	}

	// 遍历所有子语句
	for (i = 0; i < list->children; ++i) {
		// 当前子语句
		zend_ast *elem_ast = list->child[i];
		// 条件语句
		zend_ast *cond_ast = elem_ast->child[0];
		// 对应代码块
		zend_ast *stmt_ast = elem_ast->child[1];
		// 如果有条件：不是 else
		if (cond_ast) {
			znode cond_node;
			uint32_t opnum_jmpz;
			// 如果不是第一个子语句
			if (i > 0) {
				// 记录正在编译的语句行号
				CG(zend_lineno) = cond_ast->lineno;
				// 创建操作码： ZEND_EXT_STMT
				zend_do_extended_stmt();
			}

			// 编译表达式语句, p1:返回结果，p2:表达式语句
			zend_compile_expr(&cond_node, cond_ast);
			// 新操作码顺序号： 创建按条件跳转操作码，跳转操作码序号是 0
			opnum_jmpz = zend_emit_cond_jump(ZEND_JMPZ, &cond_node, 0);
			
			// 编译代码块
			zend_compile_stmt(stmt_ast);

			// 如果不是最后一个元素（else）
			if (i != list->children - 1) {
				// 创建 ZEND_JMP 操作码，跳转到指定编号的操作码
				jmp_opnums[i] = zend_emit_jump(0);
			}
			// 让顺序号是 opnum_jmpz 跳转到当前执行顺序的最后
			zend_update_jump_target_to_next(opnum_jmpz);
		// else 条件
		} else {
			// else 必须是最后一个条件。（经测试，这一点 parser 已经保证了）
			/* "else" can only occur as last element. */
			ZEND_ASSERT(i == list->children - 1);
			// 编译代码块
			zend_compile_stmt(stmt_ast);
		}
	}

	// 如果有一个以上的条件
	if (list->children > 1) {
		// 遍历每一个
		for (i = 0; i < list->children - 1; ++i) {
			// 更新 每个条件对应的条转型操作码的 跳转目标
			zend_update_jump_target_to_next(jmp_opnums[i]);
		}
		efree(jmp_opnums);
	}
}
/* }}} */

// ing4, 决定跳转表的类型：int，string 等，混合返回 IS_UNDEF
static zend_uchar determine_switch_jumptable_type(zend_ast_list *cases) {
	uint32_t i;
	// 默认类型 
	zend_uchar common_type = IS_UNDEF;
	// 遍历所有case 
	for (i = 0; i < cases->children; i++) {
		zend_ast *case_ast = cases->child[i];
		// case条件语句
		zend_ast **cond_ast = &case_ast->child[0];
		// 
		zval *cond_zv;
		// 没有值的是default, 跳过 default
		if (!case_ast->child[0]) {
			/* Skip default clause */
			continue;
		}
		// 计算 case条件语句
		zend_eval_const_expr(cond_ast);
		// 如果结果不是内置变量，返回 IS_UNDEF 类型
		if ((*cond_ast)->kind != ZEND_AST_ZVAL) {
			/* Non-constant case */
			return IS_UNDEF;
		}
		// 是内置变量
		
		// 取得case 值
		cond_zv = zend_ast_get_zval(case_ast->child[0]);
		// 如果值不是 整数也不是 string 
		if (Z_TYPE_P(cond_zv) != IS_LONG && Z_TYPE_P(cond_zv) != IS_STRING) {
			/* We only optimize switched on integers and strings */
			return IS_UNDEF;
		}

		// 如果默认类型是 IS_UNDEF
		if (common_type == IS_UNDEF) {
			// 更换默认类型为当前类型
			common_type = Z_TYPE_P(cond_zv);
		// 其他情况 如果默认类型和当前case类型不同，返回 IS_UNDEF
		} else if (common_type != Z_TYPE_P(cond_zv)) {
			// 表示类型不统一
			/* Non-uniform case types */
			return IS_UNDEF;
		}
		
		// 如果当前case 类型是string 并且可以转成 整数，返回 IS_UNDEF
		if (Z_TYPE_P(cond_zv) == IS_STRING
				&& is_numeric_string(Z_STRVAL_P(cond_zv), Z_STRLEN_P(cond_zv), NULL, NULL, 0)) {
			/* Numeric strings cannot be compared with a simple hash lookup */
			return IS_UNDEF;
		}
	}

	// 其他情况返回默认类型
	return common_type;
}

// ing3, 检查跳转表是否可用（整数case >= 5个，或字串case >= 2个）,p1:没用到，p2:跳转表类型
static bool should_use_jumptable(zend_ast_list *cases, zend_uchar jumptable_type) {
	// 如果编译配置要求不用跳转表
	if (CG(compiler_options) & ZEND_COMPILE_NO_JUMPTABLES) {
		// 返回：不可用
		return 0;
	}

	// 
	/* Thresholds are chosen based on when the average switch time for equidistributed
	 * input becomes smaller when using the jumptable optimization. */
	// 如果跳转表类型是整数
	if (jumptable_type == IS_LONG) {
		// case大于等于5个？返回可用：返回不可用
		return cases->children >= 5;
	// 跳转表类型是字串
	} else {
		ZEND_ASSERT(jumptable_type == IS_STRING);
		// case大于等于2个？返回可用：返回不可用
		return cases->children >= 2;
	}
}

// ing2, 编译 switch 语句
static void zend_compile_switch(zend_ast *ast) /* {{{ */
{
	// 条件变量
	zend_ast *expr_ast = ast->child[0];
	// case 语句块
	zend_ast_list *cases = zend_ast_get_list(ast->child[1]);

	uint32_t i;
	// 默认没有default
	bool has_default_case = 0;

	//
	znode expr_node, case_node;
	zend_op *opline;
	uint32_t *jmpnz_opnums, opnum_default_jmp, opnum_switch = (uint32_t)-1;
	zend_uchar jumptable_type;
	HashTable *jumptable = NULL;

	// 编译表达式语句, p1:返回结果，p2:表达式语句
	zend_compile_expr(&expr_node, expr_ast);

	// 开始循环
	zend_begin_loop(ZEND_FREE, &expr_node, 1);

	// case 结果类型是临时变量
	case_node.op_type = IS_TMP_VAR;
	// 增加变量计数
	case_node.u.op.var = get_temporary_variable();

	// 取得跳转表的类型
	jumptable_type = determine_switch_jumptable_type(cases);
	// 如果跳转表不是未定义
	// 检查跳转表是否可用（整数case >= 5个，或字串case >= 2个）,p1:没用到，p2:跳转表类型
	if (jumptable_type != IS_UNDEF && should_use_jumptable(cases, jumptable_type)) {
		// 跳转表操作 node
		znode jumptable_op;
		// 创建跳转表
		ALLOC_HASHTABLE(jumptable);
		// 初始化跳转表
		zend_hash_init(jumptable, cases->children, NULL, NULL, 0);
		// znode类型，常量
		jumptable_op.op_type = IS_CONST;
		// 跳转表关联到node
		ZVAL_ARR(&jumptable_op.u.constant, jumptable);

		// 创建操作码 ZEND_SWITCH_LONG 或 ZEND_SWITCH_STRING
		// jumptable_op作为第二个运算对象，第一个运算对象是switch值
		opline = zend_emit_op(NULL,
			jumptable_type == IS_LONG ? ZEND_SWITCH_LONG : ZEND_SWITCH_STRING,
			&expr_node, &jumptable_op);
		// 如果操作码第一个运算对象是常量
		if (opline->op1_type == IS_CONST) {
			// 增加引用次数
			// 获取当前操作码列表中 第 (node).constant 个 literal
			Z_TRY_ADDREF_P(CT_CONSTANT(opline->op1));
		}
		// 操作码顺序号
		opnum_switch = opline - CG(active_op_array)->opcodes;
	}

	// 创建一串整数 ，跳转序号表
	jmpnz_opnums = safe_emalloc(sizeof(uint32_t), cases->children, 0);
	// 遍历所有 case
	for (i = 0; i < cases->children; ++i) {
		// 当前 case
		zend_ast *case_ast = cases->child[i];
		// 条件语句
		zend_ast *cond_ast = case_ast->child[0];
		// 条件 node
		znode cond_node;

		// 如果没有条件语句
		if (!cond_ast) {
			// 如果已经有了 default
			if (has_default_case) {
				// 记录 case 行号
				CG(zend_lineno) = case_ast->lineno;
				// 报错：只能有一个default子句
				zend_error_noreturn(E_COMPILE_ERROR,
					"Switch statements may only contain one default clause");
			}
			// 已经有了default
			has_default_case = 1;
			// 下一个
			continue;
		}
		// 编译表达式语句, p1:返回结果，p2:表达式语句
		zend_compile_expr(&cond_node, cond_ast);

		// 如果case 条件node 类型是常量，并且值为false
		if (expr_node.op_type == IS_CONST
			&& Z_TYPE(expr_node.u.constant) == IS_FALSE) {
			// 创建 按条件(znode) 跳转 操作码，返回新操作码顺序号
			jmpnz_opnums[i] = zend_emit_cond_jump(ZEND_JMPZ, &cond_node, 0);
		// 如果case 条件node 类型是常量，并且值为true
		} else if (expr_node.op_type == IS_CONST
			&& Z_TYPE(expr_node.u.constant) == IS_TRUE) {
			// 创建 按条件(znode) 跳转 操作码，返回新操作码顺序号
			jmpnz_opnums[i] = zend_emit_cond_jump(ZEND_JMPNZ, &cond_node, 0);
		// 其他情况
		} else {
			// 创建操作码 node 是变量或临时变量 ？ZEND_CASE ： ZEND_IS_EQUAL
			opline = zend_emit_op(NULL,
				(expr_node.op_type & (IS_VAR|IS_TMP_VAR)) ? ZEND_CASE : ZEND_IS_EQUAL,
				&expr_node, &cond_node);
			// case_node作为操作码结果
			SET_NODE(opline->result, &case_node);
			// 如果第一个运算对象是常量
			if (opline->op1_type == IS_CONST) {
				// 增加引用次数
				// 获取当前操作码列表中 第 (node).constant 个 literal
				Z_TRY_ADDREF_P(CT_CONSTANT(opline->op1));
			}
			// 创建 按条件(znode) 跳转 操作码，返回新操作码顺序号
			jmpnz_opnums[i] = zend_emit_cond_jump(ZEND_JMPNZ, &case_node, 0);
		}
	}
	// 创建 ZEND_JMP 操作码，跳转到指定编号的操作码
	opnum_default_jmp = zend_emit_jump(0);

	// 遍历所有case
	for (i = 0; i < cases->children; ++i) {
		// 当前 case
		zend_ast *case_ast = cases->child[i];
		// 条件语句
		zend_ast *cond_ast = case_ast->child[0];
		// 代码块
		zend_ast *stmt_ast = case_ast->child[1];

		// 如果有条件语句
		if (cond_ast) {
			// 更新指定跳转型操作码，跳转到下一个创建的新操作码 。
			zend_update_jump_target_to_next(jmpnz_opnums[i]);
			// 如果有跳转表
			if (jumptable) {
				// 语句编译成 zval
				zval *cond_zv = zend_ast_get_zval(cond_ast);
				//
				zval jmp_target;
				// 当前上下文中的操作码数量
				ZVAL_LONG(&jmp_target, get_next_op_number());
				//
				ZEND_ASSERT(Z_TYPE_P(cond_zv) == jumptable_type);
				// 如果条件值是整数 
				if (Z_TYPE_P(cond_zv) == IS_LONG) {
					// 跳转表里 通过哈希值添加这个跳转目标
					zend_hash_index_add(jumptable, Z_LVAL_P(cond_zv), &jmp_target);
				// 如果条件值是字串 
				} else {
					ZEND_ASSERT(Z_TYPE_P(cond_zv) == IS_STRING);
					// 跳转表里 通过 key 添加这个跳转目标
					zend_hash_add(jumptable, Z_STR_P(cond_zv), &jmp_target);
				}
			}
		// 如果没有条件语句
		} else {
			// 更新指定跳转型操作码，跳转到下一个创建的新操作码 。
			zend_update_jump_target_to_next(opnum_default_jmp);

			// 如果有跳转表
			if (jumptable) {
				// switch操作码编号不可以是-1
				ZEND_ASSERT(opnum_switch != (uint32_t)-1);
				// switch操作码行号
				opline = &CG(active_op_array)->opcodes[opnum_switch];
				// 当前上下文中的操作码数量
				opline->extended_value = get_next_op_number();
			}
		}
		// 编译代码块
		zend_compile_stmt(stmt_ast);
	}

	// 如果没有默认 case
	if (!has_default_case) {
		// 更新指定跳转型操作码，跳转到下一个创建的新操作码 。
		zend_update_jump_target_to_next(opnum_default_jmp);

		// 如果有跳转表
		if (jumptable) {
			// switch 语句操作码
			opline = &CG(active_op_array)->opcodes[opnum_switch];
			// 记录当前上下文中的操作码数量
			opline->extended_value = get_next_op_number();
		}
	}
	// 结束循环
	// 当前上下文中的操作码数量
	zend_end_loop(get_next_op_number(), &expr_node);

	// 如果 znode 类型是变量或 临时变量
	if (expr_node.op_type & (IS_VAR|IS_TMP_VAR)) {
		// 创建操作码 ZEND_FREE
		opline = zend_emit_op(NULL, ZEND_FREE, &expr_node, NULL);
		// 扩展信息 ZEND_FREE_SWITCH
		opline->extended_value = ZEND_FREE_SWITCH;
	// 类型是常量
	} else if (expr_node.op_type == IS_CONST) {
		// 销毁常量
		zval_ptr_dtor_nogc(&expr_node.u.constant);
	}

	// 释放跳转序号表
	efree(jmpnz_opnums);
}
/* }}} */

// ing4, match语句的条件总数
static uint32_t count_match_conds(zend_ast_list *arms)
{
	uint32_t num_conds = 0;
	// 遍历每一个 arm
	for (uint32_t i = 0; i < arms->children; i++) {
		zend_ast *arm_ast = arms->child[i];
		if (arm_ast->child[0] == NULL) {
			continue;
		}
		// 每个 arm 可有多个条件
		zend_ast_list *conds = zend_ast_get_list(arm_ast->child[0]);
		// 总条件数
		num_conds += conds->children;
	}

	return num_conds;
}

// ing3, 检验是否可用跳转表：每一个arm值都是整数或字串
// match语句也用到跳转表，和switch语句一样
static bool can_match_use_jumptable(zend_ast_list *arms) {
	// 遍历 每一个 arm
	for (uint32_t i = 0; i < arms->children; i++) {
		// 取得arm
		zend_ast *arm_ast = arms->child[i];
		// 如果arm没有子语句，说明是默认arm, default
		if (!arm_ast->child[0]) {
			// 跳过默认 arm
			/* Skip default arm */
			continue;
		}

		// arm下的语句列表
		zend_ast_list *conds = zend_ast_get_list(arm_ast->child[0]);
		// 遍历语句列表
		for (uint32_t j = 0; j < conds->children; j++) {
			// 取得一个语句指针
			zend_ast **cond_ast = &conds->child[j];

			// 计算这个语句
			zend_eval_const_expr(cond_ast);
			// 如果类型不是常量语句
			if ((*cond_ast)->kind != ZEND_AST_ZVAL) {
				// 返回false
				return 0;
			}

			// 把语句编译成 zval
			zval *cond_zv = zend_ast_get_zval(*cond_ast);
			// 如果结果不是整数 也不是 string
			if (Z_TYPE_P(cond_zv) != IS_LONG && Z_TYPE_P(cond_zv) != IS_STRING) {
				// 返回false
				return 0;
			}
		}
	}
	// 返回true
	return 1;
}

// 200行
// ing2, 编译 match。p1:返回第一个case的值，p2:match语句
static void zend_compile_match(znode *result, zend_ast *ast)
{
	// 第一个子元素，传入值
	zend_ast *expr_ast = ast->child[0];
	// 第二个子元素，arm列表
	zend_ast_list *arms = zend_ast_get_list(ast->child[1]);
	//
	bool has_default_arm = 0;
	// 
	uint32_t opnum_match = (uint32_t)-1;

	znode expr_node;
	// 编译表达式语句, p1:返回结果，p2:表达式语句
	zend_compile_expr(&expr_node, expr_ast);

	// 临时 znode
	znode case_node;
	// 类型是临时变量
	case_node.op_type = IS_TMP_VAR;
	// 取得临时变量序号
	case_node.u.op.var = get_temporary_variable();

	// 取得 match语句的条件总数
	uint32_t num_conds = count_match_conds(arms);
	// 检验是否可用跳转表：每一个arm值都是整数或字串
	zend_uchar can_use_jumptable = can_match_use_jumptable(arms);
	// 使用跳转表的条件：可使用，并且arm大于等于2个
	bool uses_jumptable = can_use_jumptable && num_conds >= 2;
	// 跳转表
	HashTable *jumptable = NULL;
	// 跳转编号列表
	uint32_t *jmpnz_opnums = NULL;

	// 遍历每一个arm
	for (uint32_t i = 0; i < arms->children; ++i) {
		// 取出arm
		zend_ast *arm_ast = arms->child[i];

		// 如果没有值
		if (!arm_ast->child[0]) {
			// 如果有默认 arm
			if (has_default_arm) {
				// 记录行号
				CG(zend_lineno) = arm_ast->lineno;
				// 报错：只能有一个默认 arm
				zend_error_noreturn(E_COMPILE_ERROR,
					"Match expressions may only contain one default arm");
			}
			// 标记成，含有默认arm
			has_default_arm = 1;
		}
	}

	// 如果需要使用跳转表
	if (uses_jumptable) {
		// 跳转表 node
		znode jumptable_op;
		// 分配跳转表
		ALLOC_HASHTABLE(jumptable);
		// 初始化跳转表
		zend_hash_init(jumptable, num_conds, NULL, NULL, 0);
		// node标记成常量
		jumptable_op.op_type = IS_CONST;
		// 把跳转表关联给它
		ZVAL_ARR(&jumptable_op.u.constant, jumptable);

		// 创建 ZEND_MATCH 操作码
		zend_op *opline = zend_emit_op(NULL, ZEND_MATCH, &expr_node, &jumptable_op);
		// 如果第一个操作对象是 常量
		if (opline->op1_type == IS_CONST) {
			// 尝试给 pz(指针) 增加引用次数
			Z_TRY_ADDREF_P(CT_CONSTANT(opline->op1));
		}
		// match操作码在活动列表中的序号
		opnum_match = opline - CG(active_op_array)->opcodes;
	// 如果不使用跳转表
	} else {
		// 分配 与 arm 数量相同的整数列表
		jmpnz_opnums = safe_emalloc(sizeof(uint32_t), num_conds, 0);
		// 条件数量
		uint32_t cond_count = 0;
		// 遍历所有arm
		for (uint32_t i = 0; i < arms->children; ++i) {
			// 取出 arm
			zend_ast *arm_ast = arms->child[i];

			// 如果是默认arm
			if (!arm_ast->child[0]) {
				// 跳过
				continue;
			}

			// arm下的条件语句列表（每个arm对应一串条件表达式）
			zend_ast_list *conds = zend_ast_get_list(arm_ast->child[0]);
			// 遍历  arm下的语句列表
			for (uint32_t j = 0; j < conds->children; j++) {
				// 条件表达式
				zend_ast *cond_ast = conds->child[j];
				// 临时node
				znode cond_node;
				// 编译表达式语句, p1:返回结果，p2:表达式语句
				zend_compile_expr(&cond_node, cond_ast);
				// 如果传入值是变量或临时变量，使用 ZEND_CASE_STRICT 操作码，否则 使用 ZEND_IS_IDENTICAL 操作码
				uint32_t opcode = (expr_node.op_type & (IS_VAR|IS_TMP_VAR)) ? ZEND_CASE_STRICT : ZEND_IS_IDENTICAL;
				// 创建操作码
				zend_op *opline = zend_emit_op(NULL, opcode, &expr_node, &cond_node);
				// 返回结果，使用临时node接收
				SET_NODE(opline->result, &case_node);
				// 如果第一个运算对象是常量
				if (opline->op1_type == IS_CONST) {
					// 第一个运算对象增加引用次数
					Z_TRY_ADDREF_P(CT_CONSTANT(opline->op1));
				}
				// 创建 按条件(znode) 跳转 操作码，返回新操作码顺序号
				jmpnz_opnums[cond_count] = zend_emit_cond_jump(ZEND_JMPNZ, &case_node, 0);
				// 条件表达式数量 增加
				cond_count++;
			}
		}
	}

	uint32_t opnum_default_jmp = 0;
	// 如果不使用跳转表。就要使用操作码跳转
	if (!uses_jumptable) {
		// 创建 ZEND_JMP 操作码，跳转到指定编号的操作码
		opnum_default_jmp = zend_emit_jump(0);
	}

	// 默认是第一个case
	bool is_first_case = 1;
	// 条件语句数量 0
	uint32_t cond_count = 0;
	// 和arm数量相同的一串数字
	uint32_t *jmp_end_opnums = safe_emalloc(sizeof(uint32_t), arms->children, 0);

	// 创建的默认arm要第一个发布，避免活动区域问题？？
	// The generated default arm is emitted first to avoid live range issues where the tmpvar
	// for the arm result is freed even though it has not been initialized yet.
	// 如果没有默认arm
	if (!has_default_arm) {
		// 如果不使用跳转表
		if (!uses_jumptable) {
			// 更新指定跳转型操作码，跳转到下一个创建的新操作码 。
			zend_update_jump_target_to_next(opnum_default_jmp);
		}
		// 如果有跳转表
		if (jumptable) {
			// 取得match语句操作码
			zend_op *opline = &CG(active_op_array)->opcodes[opnum_match];
			// 当前上下文中的操作码数量
			opline->extended_value = get_next_op_number();
		}

		// 创建操作码 ZEND_MATCH_ERROR
		zend_op *opline = zend_emit_op(NULL, ZEND_MATCH_ERROR, &expr_node, NULL);
		// 如果第一个操作对象是常量
		if (opline->op1_type == IS_CONST) {
			// 引用计数 +1
			Z_TRY_ADDREF_P(CT_CONSTANT(opline->op1));
		}
		// 如果没有 arm
		if (arms->children == 0) {
			// 把操作码标记成 表达式抛出
			/* Mark this as an "expression throw" for opcache. */
			opline->extended_value = ZEND_THROW_IS_EXPR;
		}
	}

	// 遍历每一个arm语句
	for (uint32_t i = 0; i < arms->children; ++i) {
		// arm语句
		zend_ast *arm_ast = arms->child[i];
		// 语句块
		zend_ast *body_ast = arm_ast->child[1];

		// 如果arm有值
		if (arm_ast->child[0] != NULL) {
			// 条件语句
			zend_ast_list *conds = zend_ast_get_list(arm_ast->child[0]);
			// 遍历条件语句
			for (uint32_t j = 0; j < conds->children; j++) {
				// 条件语句
				zend_ast *cond_ast = conds->child[j];

				// 如果有跳转编号
				if (jmpnz_opnums != NULL) {
					// 更新指定跳转型操作码，跳转到下一个创建的新操作码 。
					zend_update_jump_target_to_next(jmpnz_opnums[cond_count]);
				}
				// 如果有跳转表
				if (jumptable) {
					// 条件语句转成 zval
					zval *cond_zv = zend_ast_get_zval(cond_ast);
					// 临时变量
					zval jmp_target;
					// 跳转目标，是新操作码
					ZVAL_LONG(&jmp_target, get_next_op_number());

					// 如果条件已成整数
					if (Z_TYPE_P(cond_zv) == IS_LONG) {
						// 用整数添加到跳转表中
						zend_hash_index_add(jumptable, Z_LVAL_P(cond_zv), &jmp_target);
					// 如果没有转成整数
					} else {
						// 条件必须是字串
						ZEND_ASSERT(Z_TYPE_P(cond_zv) == IS_STRING);
						// 字串作为key，添加到哈希表中
						zend_hash_add(jumptable, Z_STR_P(cond_zv), &jmp_target);
					}
				}
				// 条件数量 +1
				cond_count++;
			}
		// arm没有值，default
		} else {
			// 不使用跳转表
			if (!uses_jumptable) {
				// 更新指定跳转型操作码，跳转到下一个创建的新操作码 。
				zend_update_jump_target_to_next(opnum_default_jmp);
			}

			// 如果有跳转表
			if (jumptable) {
				// opnum_match 必须有效
				ZEND_ASSERT(opnum_match != (uint32_t)-1);
				// 取出操作码
				zend_op *opline = &CG(active_op_array)->opcodes[opnum_match];
				// 给它添加附加操作码
				opline->extended_value = get_next_op_number();
			}
		}

		znode body_node;
		// 编译表达式语句, p1:返回结果，p2:表达式语句
		zend_compile_expr(&body_node, body_ast);

		// 如果 是第一个case
		if (is_first_case) {
			// 创建临时操作码 ZEND_QM_ASSIGN
			zend_emit_op_tmp(result, ZEND_QM_ASSIGN, &body_node, NULL);
			// 不再有第一个case
			is_first_case = 0;
		// 不是第一个case
		} else {
			// 创建操作码 ZEND_QM_ASSIGN
			zend_op *opline_qm_assign = zend_emit_op(NULL, ZEND_QM_ASSIGN, &body_node, NULL);
			// 用result接收结果
			SET_NODE(opline_qm_assign->result, result);
		}
		// 创建 ZEND_JMP 操作码，跳转到指定编号的操作码
		jmp_end_opnums[i] = zend_emit_jump(0);
	}

	// 初始化没有 arm的结果 
	// Initialize result in case there is no arm
	// 如果没有arm
	if (arms->children == 0) {
		// 结果类型是常量
		result->op_type = IS_CONST;
		// 值为null
		ZVAL_NULL(&result->u.constant);
	}

	// 遍历所有arm
	for (uint32_t i = 0; i < arms->children; ++i) {
		// 更新指定跳转型操作码，跳转到下一个创建的新操作码 。
		zend_update_jump_target_to_next(jmp_end_opnums[i]);
	}

	// 如果传入值是 变量或临时变量
	if (expr_node.op_type & (IS_VAR|IS_TMP_VAR)) {
		// 创建 ZEND_FREE 操作码
		zend_op *opline = zend_emit_op(NULL, ZEND_FREE, &expr_node, NULL);
		// 扩展值为 ZEND_FREE_SWITCH
		opline->extended_value = ZEND_FREE_SWITCH;
	// 传入值是常量
	} else if (expr_node.op_type == IS_CONST) {
		// 销毁常量
		zval_ptr_dtor_nogc(&expr_node.u.constant);
	}

	// 如果有跳转序号列表
	if (jmpnz_opnums != NULL) {
		// 释放这个列表
		efree(jmpnz_opnums);
	}
	// 释放列表
	efree(jmp_end_opnums);
}

// ing2, 编译 try
static void zend_compile_try(zend_ast *ast) /* {{{ */
{
	// 子句0，try 语句
	zend_ast *try_ast = ast->child[0];
	// 子句1，若干个 catch 语句
	zend_ast_list *catches = zend_ast_get_list(ast->child[1]);
	// 子句2，finally 语句
	zend_ast *finally_ast = ast->child[2];

	uint32_t i, j;
	zend_op *opline;
	uint32_t try_catch_offset;
	// 每个catch的行号
	uint32_t *jmp_opnums = safe_emalloc(sizeof(uint32_t), catches->children, 0);
	// 原 finally 语句编号
	uint32_t orig_fast_call_var = CG(context).fast_call_var;
	// 上一个 try操作码的位置
	uint32_t orig_try_catch_offset = CG(context).try_catch_offset;

	// 至少要有一个catch 或 finally,否则报错
	if (catches->children == 0 && !finally_ast) {
		// 报错：使用try不可以没有 catch 或 finally
		zend_error_noreturn(E_COMPILE_ERROR, "Cannot use try without catch or finally");
	}

	// label: try 和 try { label: } 绝对不一样。
	/* label: try { } must not be equal to try { label: } */
	if (CG(context).labels) {
		zend_label *label;
		// 遍历所有label
		ZEND_HASH_MAP_REVERSE_FOREACH_PTR(CG(context).labels, label) {
			// 当前上下文中的操作码数量
			if (label->opline_num == get_next_op_number()) {
				// 创建操作码 ZEND_NOP
				zend_emit_op(NULL, ZEND_NOP, NULL, NULL);
			}
			break;
		} ZEND_HASH_FOREACH_END();
	}
	// 当前 try操作码的位置
	try_catch_offset = zend_add_try_element(get_next_op_number());

	// 如果有 finally 语句
	if (finally_ast) {
		zend_loop_var fast_call;
		// 如果没有 finally 语句块
		if (!(CG(active_op_array)->fn_flags & ZEND_ACC_HAS_FINALLY_BLOCK)) {
			// 添加包含 finally 语句块 标记
			CG(active_op_array)->fn_flags |= ZEND_ACC_HAS_FINALLY_BLOCK;
		}
		// 增加变量计数
		CG(context).fast_call_var = get_temporary_variable();

		// 把 FAST_CALL 压到栈里
		/* Push FAST_CALL on unwind stack */
		// 操作码 ZEND_FAST_CALL
		fast_call.opcode = ZEND_FAST_CALL;
		// 类型 临时变量
		fast_call.var_type = IS_TMP_VAR;
		// 变量序号
		fast_call.var_num = CG(context).fast_call_var;
		// 操作码序号
		fast_call.try_catch_offset = try_catch_offset;
		// 把 fast_call 操作码压到栈里
		zend_stack_push(&CG(loop_var_stack), &fast_call);
	}
	// 最后一个try_catch操作码序号
	CG(context).try_catch_offset = try_catch_offset;

	// 编译try 语句
	zend_compile_stmt(try_ast);

	// 如果有 catch语句
	if (catches->children != 0) {
		// 创建 ZEND_JMP 操作码，跳转到指定编号的操作码
		jmp_opnums[0] = zend_emit_jump(0);
	}

	// 遍历所有的catch语句
	for (i = 0; i < catches->children; ++i) {
		// 当前catch语句
		zend_ast *catch_ast = catches->child[i];
		// 捕获的异常类名列表
		zend_ast_list *classes = zend_ast_get_list(catch_ast->child[0]);
		// 捕获的异常变量
		zend_ast *var_ast = catch_ast->child[1];
		// 语句块
		zend_ast *stmt_ast = catch_ast->child[2];
		// 如果有变量名语句，变量名创建内置字串。没有则为 null
		zend_string *var_name = var_ast ? zval_make_interned_string(zend_ast_get_zval(var_ast)) : NULL;
		// 是否是最后一个catch
		bool is_last_catch = (i + 1 == catches->children);

		// 创建一串整数列表，数量和catch一样多
		uint32_t *jmp_multicatch = safe_emalloc(sizeof(uint32_t), classes->children - 1, 0);
		// 编号 ，默认为 -1
		uint32_t opnum_catch = (uint32_t)-1;
		// 行号
		CG(zend_lineno) = catch_ast->lineno;

		// 遍历 所有异常类名
		for (j = 0; j < classes->children; j++) {
			// 异常类名
			zend_ast *class_ast = classes->child[j];
			// 是否是最后一个异常类
			bool is_last_class = (j + 1 == classes->children);

			// 是否正在引用 self,parent,static 以外的普通类名。如果不是
			if (!zend_is_const_default_class_ref(class_ast)) {
				// 报错：类名错误
				zend_error_noreturn(E_COMPILE_ERROR, "Bad class name in the catch statement");
			}
			// 当前上下文中的操作码数量
			opnum_catch = get_next_op_number();
			// 如果是第一个 catch语句的 第一个异常类型
			if (i == 0 && j == 0) {
				// 更新 对应zend_try_catch_element 实例的操作码序号
				CG(active_op_array)->try_catch_array[try_catch_offset].catch_op = opnum_catch;
			}

			// 下一个操作码序号
			opline = get_next_op();
			// 操作码 ZEND_CATCH
			opline->opcode = ZEND_CATCH;
			// 第一个运算求对象类型是 常量
			opline->op1_type = IS_CONST;
			// 通过语句获取完整类名
			opline->op1.constant = zend_add_class_name_literal(
					zend_resolve_class_name_ast(class_ast));
			// 给 CG(active_op_array) 添加缓存大小，添加 一个 void*指针的大小，返回添加后的缓存大小
			opline->extended_value = zend_alloc_cache_slot();

			// 如果变量名是 $this
			if (var_name && zend_string_equals_literal(var_name, "this")) {
				// 报错：不可以引用传递 $this
				zend_error_noreturn(E_COMPILE_ERROR, "Cannot re-assign $this");
			}

			// 结果类型，有变量名？编译变量：未定义
			opline->result_type = var_name ? IS_CV : IS_UNUSED;
			// 查找编译变量，如果没有的话创建一个新的并返回序号
			opline->result.var = var_name ? lookup_cv(var_name) : -1;

			// 如果是最后一个 catch的最后一个 异常类型
			if (is_last_catch && is_last_class) {
				// 操作码添加 ZEND_LAST_CATCH 标记
				opline->extended_value |= ZEND_LAST_CATCH;
			}

			// 如果不是最后一个异常类型
			if (!is_last_class) {
				// 创建 ZEND_JMP 操作码，跳转到指定编号的操作码
				jmp_multicatch[j] = zend_emit_jump(0);
				// 取得 catch语句行号
				opline = &CG(active_op_array)->opcodes[opnum_catch];
				// 当前上下文中的操作码数量
				opline->op2.opline_num = get_next_op_number();
			}
		}

		for (j = 0; j < classes->children - 1; j++) {
			// 更新指定跳转型操作码，跳转到下一个创建的新操作码 。
			zend_update_jump_target_to_next(jmp_multicatch[j]);
		}

		// 释放跳转序号串
		efree(jmp_multicatch);

		// 编译代码块
		zend_compile_stmt(stmt_ast);

		// 如果不是最后一个 catch语句
		if (!is_last_catch) {
			// 创建 ZEND_JMP 操作码，跳转到指定编号的操作码
			jmp_opnums[i + 1] = zend_emit_jump(0);
		}

		// 
		ZEND_ASSERT(opnum_catch != (uint32_t)-1 && "Should have at least one class");
		// 记录catch操作码行号
		opline = &CG(active_op_array)->opcodes[opnum_catch];
		// 如果不是最后一个catch
		if (!is_last_catch) {
			// 当前上下文中的操作码数量
			opline->op2.opline_num = get_next_op_number();
		}
	}

	// 遍历所有 catch语句
	for (i = 0; i < catches->children; ++i) {
		// 更新指定跳转型操作码，跳转到下一个创建的新操作码 。
		zend_update_jump_target_to_next(jmp_opnums[i]);
	}

	// 如果有 finally 语句
	if (finally_ast) {
		// 循环变量 ？
		zend_loop_var discard_exception;
		// 当前上下文中的操作码数量
		uint32_t opnum_jmp = get_next_op_number() + 1;

		// 从解开的 堆栈中 弹出 FAST_CALL
		/* Pop FAST_CALL from unwind stack */
		zend_stack_del_top(&CG(loop_var_stack));

		// 把 丢弃异常 操作码 压入栈中
		/* Push DISCARD_EXCEPTION on unwind stack */
		discard_exception.opcode = ZEND_DISCARD_EXCEPTION;
		// 类型是 临时变量
		discard_exception.var_type = IS_TMP_VAR;
		// 记录 finally 序号 ？
		discard_exception.var_num = CG(context).fast_call_var;
		// 压入栈里
		zend_stack_push(&CG(loop_var_stack), &discard_exception);
		// finally_ast 语句行号
		CG(zend_lineno) = finally_ast->lineno;

		// 创建 ZEND_FAST_CALL 操作码
		opline = zend_emit_op(NULL, ZEND_FAST_CALL, NULL, NULL);
		// 第一个操作对象里记录 try_catch 语句序号
		opline->op1.num = try_catch_offset;
		// 结果类型为 临时变量
		opline->result_type = IS_TMP_VAR;
		// finally 语句序号
		opline->result.var = CG(context).fast_call_var;

		// 创建 ZEND_JMP 操作码
		zend_emit_op(NULL, ZEND_JMP, NULL, NULL);
		// 编译 finally 语句块
		zend_compile_stmt(finally_ast);

		// 更新 finally 操作码序号 ？
		CG(active_op_array)->try_catch_array[try_catch_offset].finally_op = opnum_jmp + 1;
		// 当前上下文中的操作码数量
		CG(active_op_array)->try_catch_array[try_catch_offset].finally_end
			= get_next_op_number();

		// 创建操作码 ZEND_FAST_RET
		opline = zend_emit_op(NULL, ZEND_FAST_RET, NULL, NULL);
		// 变量类型为 临时变量
		opline->op1_type = IS_TMP_VAR;
		// 变量编号
		opline->op1.var = CG(context).fast_call_var;
		// 第二个运算对象里记录 原 try_catch 语句序号 ？
		opline->op2.num = orig_try_catch_offset;
		// 更新指定跳转型操作码，跳转到下一个创建的新操作码 。
		zend_update_jump_target_to_next(opnum_jmp);
		// 原 fast_call 变量序号
		CG(context).fast_call_var = orig_fast_call_var;

		// 弹出 DISCARD_EXCEPTION 操作码
		/* Pop DISCARD_EXCEPTION from unwind stack */
		zend_stack_del_top(&CG(loop_var_stack));
	}

	// 恢复原来的偏移量
	CG(context).try_catch_offset = orig_try_catch_offset;

	// 释放 ZEND_JMP 操作码
	efree(jmp_opnums);
}
/* }}} */

// declare 语句，parse时直接调用
/* Encoding declarations must already be handled during parsing */
// ing3, 编译 declare里面的 Encoding
bool zend_handle_encoding_declaration(zend_ast *ast) /* {{{ */
{
	// declare 里面的子句
	zend_ast_list *declares = zend_ast_get_list(ast);
	uint32_t i;
	// 遍历所有子句
	for (i = 0; i < declares->children; ++i) {
		zend_ast *declare_ast = declares->child[i];
		// 属性名
		zend_ast *name_ast = declare_ast->child[0];
		// 元素值
		zend_ast *value_ast = declare_ast->child[1];
		// 属性名字串
		zend_string *name = zend_ast_get_str(name_ast);

		// 如果是 encoding 属性
		if (zend_string_equals_literal_ci(name, "encoding")) {
			// 如果值不是常量，报错
			if (value_ast->kind != ZEND_AST_ZVAL) {
				zend_throw_exception(zend_ce_compile_error, "Encoding must be a literal", 0);
				return 0;
			}

			// 如果开启了 多字符集 功能
			if (CG(multibyte)) {
				// 字符集名
				zend_string *encoding_name = zval_get_string(zend_ast_get_zval(value_ast));

				const zend_encoding *new_encoding, *old_encoding;
				zend_encoding_filter old_input_filter;

				// 有字符集定义
				CG(encoding_declared) = 1;
				// 新字符集
				new_encoding = zend_multibyte_fetch_encoding(ZSTR_VAL(encoding_name));
				// 如果不存在，报错，不支持此字符集
				if (!new_encoding) {
					zend_error(E_COMPILE_WARNING, "Unsupported encoding [%s]", ZSTR_VAL(encoding_name));
				// 如果支持
				} else {
					// 
					old_input_filter = LANG_SCNG(input_filter);
					// 
					old_encoding = LANG_SCNG(script_encoding);
					// 设置字符集
					zend_multibyte_set_filter(new_encoding);

					// 如果 输入 filter 改变了，或，需要变更字符集, 重新扫描
					/* need to re-scan if input filter changed */
					if (old_input_filter != LANG_SCNG(input_filter) ||
						 (old_input_filter && new_encoding != old_encoding)) {
						// zend_language_scanner.l
						zend_multibyte_yyinput_again(old_input_filter, old_encoding);
					}
				}
				// 销毁字符集名
				zend_string_release_ex(encoding_name, 0);
			// 多字符集 功能已关闭
			} else {
				zend_error(E_COMPILE_WARNING, "declare(encoding=...) ignored because "
					"Zend multibyte feature is turned off by settings");
			}
		}
	}

	return 1;
}
/* }}} */

// 

// ing3, 检查是否是第一个语句，不计 declare 语句
/* Check whether this is the first statement, not counting declares. */
static zend_result zend_is_first_statement(zend_ast *ast, bool allow_nop) /* {{{ */
{
	uint32_t i = 0;
	// ? CG(ast) 是parser 生成的最外层语句块， zend_language_scanner.l 也有相关操作
	zend_ast_list *file_ast = zend_ast_get_list(CG(ast));

	// 遍历所有最外层语句
	while (i < file_ast->children) {
		// 如果第一个（跳过declare 和 null）是 目标语句，返回 是
		if (file_ast->child[i] == ast) {
			return SUCCESS;
		// 如果是Null，并且不允许有Null，返回否。否则跳过
		} else if (file_ast->child[i] == NULL) {
			if (!allow_nop) {
				return FAILURE;
			}
		// 跳过declare : 如果语句类型不是 ZEND_AST_DECLARE 返回否
		} else if (file_ast->child[i]->kind != ZEND_AST_DECLARE) {
			return FAILURE;
		}
		// 下一个
		i++;
	}
	// 如果没找到目标语句
	return FAILURE;
}
/* }}} */

// ing3, 编译 declare
static void zend_compile_declare(zend_ast *ast) /* {{{ */
{
	// decalre 里面的子语句
	zend_ast_list *declares = zend_ast_get_list(ast->child[0]);
	// decalre 后面的代码块
	zend_ast *stmt_ast = ast->child[1];
	// 
	zend_declarables orig_declarables = FC(declarables);
	uint32_t i;

	// 遍历每个子语句
	for (i = 0; i < declares->children; ++i) {
		zend_ast *declare_ast = declares->child[i];
		zend_ast *name_ast = declare_ast->child[0];
		zend_ast **value_ast_ptr = &declare_ast->child[1];
		zend_string *name = zend_ast_get_str(name_ast);

		if ((*value_ast_ptr)->kind != ZEND_AST_ZVAL) {
			zend_error_noreturn(E_COMPILE_ERROR, "declare(%s) value must be a literal", ZSTR_VAL(name));
		}
		// declare(...)里面定义的 ticks
		if (zend_string_equals_literal_ci(name, "ticks")) {
			zval value_zv;
			// 计算变量的值
			zend_const_expr_to_zval(&value_zv, value_ast_ptr, /* allow_dynamic */ false);
			// 值是长整型，存在 FC(declarables).ticks 里
			FC(declarables).ticks = zval_get_long(&value_zv);
			// 销毁临时变量
			zval_ptr_dtor_nogc(&value_zv);
		// encoding，必须放在最前面
		} else if (zend_string_equals_literal_ci(name, "encoding")) {
			// 如果语句不是最前面一行，抛错
			if (FAILURE == zend_is_first_statement(ast, /* allow_nop */ 0)) {
				zend_error_noreturn(E_COMPILE_ERROR, "Encoding declaration pragma must be "
					"the very first statement in the script");
			}
		// strict_types
		} else if (zend_string_equals_literal_ci(name, "strict_types")) {
			zval value_zv;
			
			// 如果语句不是最前面一行，抛错
			if (FAILURE == zend_is_first_statement(ast, /* allow_nop */ 0)) {
				zend_error_noreturn(E_COMPILE_ERROR, "strict_types declaration must be "
					"the very first statement in the script");
			}
			// 后面不可以有代码块
			if (ast->child[1] != NULL) {
				zend_error_noreturn(E_COMPILE_ERROR, "strict_types declaration must not "
					"use block mode");
			}
			// 计算语句的值
			zend_const_expr_to_zval(&value_zv, value_ast_ptr, /* allow_dynamic */ false);
			// 值必须是整型0或1，否则报错
			if (Z_TYPE(value_zv) != IS_LONG || (Z_LVAL(value_zv) != 0 && Z_LVAL(value_zv) != 1)) {
				zend_error_noreturn(E_COMPILE_ERROR, "strict_types declaration must have 0 or 1 as its value");
			}
			// 如果要求 strict_types
			if (Z_LVAL(value_zv) == 1) {
				// 给当前操作码列表添加 ZEND_ACC_STRICT_TYPES 标记
				CG(active_op_array)->fn_flags |= ZEND_ACC_STRICT_TYPES;
			}
		// 其他变量都不支持，抛错
		} else {
			zend_error(E_COMPILE_WARNING, "Unsupported declare '%s'", ZSTR_VAL(name));
		}
	}

	// 如果后面有代码块，编译它
	if (stmt_ast) {
		zend_compile_stmt(stmt_ast);
		//  FC(declarables) 在前面的逻辑中可能变化
		FC(declarables) = orig_declarables;
	}
}
/* }}} */

// ing4， 编译语句列表
static void zend_compile_stmt_list(zend_ast *ast) /* {{{ */
{
	zend_ast_list *list = zend_ast_get_list(ast);
	uint32_t i;
	// 遍历，编译每个语句
	for (i = 0; i < list->children; ++i) {
		zend_compile_stmt(list->child[i]);
	}
}
/* }}} */

// ing3, 设置函数的参数标识，是否通过引用传递（字典参数特殊处理）
ZEND_API void zend_set_function_arg_flags(zend_function *func) /* {{{ */
{
	uint32_t i, n;
	// 设置3个参数标记
	func->common.arg_flags[0] = 0;
	func->common.arg_flags[1] = 0;
	func->common.arg_flags[2] = 0;
	// 如果有参数列表
	if (func->common.arg_info) {
		// 取得参数数量 或 最大参数标记数量 两个中小的一个
		n = MIN(func->common.num_args, MAX_ARG_FLAG_NUM);
		// 
		i = 0;
		// 遍历 所有参数
		while (i < n) {
			// 在函数 ->quick_arg_flags 中添加指定序号的参数的 标记，每个参数2个位的空间
			// 标记内容是此参数的传递方式,是否引用传递
			ZEND_SET_ARG_FLAG(func, i + 1, ZEND_ARG_SEND_MODE(&func->common.arg_info[i]));
			// 下一个参数
			i++;
		}
		// 如果此函数有字典参数 并且 最后一个参数的传递方式 是引用传递
		if (UNEXPECTED((func->common.fn_flags & ZEND_ACC_VARIADIC) && ZEND_ARG_SEND_MODE(&func->common.arg_info[i]))) {
			// 取得最后一个参数的传递方式
			uint32_t pass_by_reference = ZEND_ARG_SEND_MODE(&func->common.arg_info[i]);
			// 如果没有超过最大 标记使用数，遍历所有剩余空间
			while (i < MAX_ARG_FLAG_NUM) {
				// 把剩余空间都标记成 引用传递
				ZEND_SET_ARG_FLAG(func, i + 1, pass_by_reference);
				// 下一个
				i++;
			}
		}
	}
}
/* }}} */

// ing2 ,编译单个类型名, zend_compile_typename 中3次调用
static zend_type zend_compile_single_typename(zend_ast *ast)
{
	// 当前语句不可以使用短路调用
	ZEND_ASSERT(!(ast->attr & ZEND_TYPE_NULLABLE));
	// 如果语句是 类型指定语句
	if (ast->kind == ZEND_AST_TYPE) {
		// 如果有static修饰符，并且不在类中，或者在【被引用到类里的】trait 的方法中
		if (ast->attr == IS_STATIC && !CG(active_class_entry) && zend_is_scope_known()) {
			// 报错 （测试过 function a():static{}; ）
			zend_error_noreturn(E_COMPILE_ERROR,
				"Cannot use \"static\" when no class scope is active");
		}

		return (zend_type) ZEND_TYPE_INIT_CODE(ast->attr, 0, 0);
	// 如果语句不是类型指定语句
	} else {
		zend_string *class_name = zend_ast_get_str(ast);
		// 用名称查找内置类型
		zend_uchar type_code = zend_lookup_builtin_type_by_name(class_name);

		// 如果不是内置类型
		if (type_code != 0) {
			// 如果不是相对路径
			if ((ast->attr & ZEND_NAME_NOT_FQ) != ZEND_NAME_NOT_FQ) {
				// 类型定义必须是绝对路径（如何测试）
				zend_error_noreturn(E_COMPILE_ERROR,
					"Type declaration '%s' must be unqualified",
					ZSTR_VAL(zend_string_tolower(class_name)));
			}

			// 把迭代类型转换成一个 联合别名
			/* Transform iterable into a type union alias */
			if (type_code == IS_ITERABLE) {
				// 在反射 和 类型的 string 表示中，设置 迭代类型
				/* Set iterable bit for BC compat during Reflection and string representation of type */
				// ？？ zend_types.h ZEND_STR_TRAVERSABLE : Traversable
				zend_type iterable = (zend_type) ZEND_TYPE_INIT_CLASS_CONST_MASK(ZSTR_KNOWN(ZEND_STR_TRAVERSABLE),
                	(MAY_BE_ARRAY|_ZEND_TYPE_ITERABLE_BIT));
				// 返回迭代类型
				return iterable;
			}

			// ？？ zend_types.h
			return (zend_type) ZEND_TYPE_INIT_CODE(type_code, 0, 0);
		// 如果是内置类型
		} else {
			const char *correct_name;
			// 从语句中取出字串
			zend_string *orig_name = zend_ast_get_str(ast);
			// 通过语句 zend_ast 获取类引用方式 self,parent,static 或其他(ZEND_FETCH_CLASS_DEFAULT)
			uint32_t fetch_type = zend_get_class_fetch_type_ast(ast);
			// 不是 self,parent,static
			if (fetch_type == ZEND_FETCH_CLASS_DEFAULT) {
				// 通过语句获取完整类名
				class_name = zend_resolve_class_name_ast(ast);
				// 保证类名有效
				zend_assert_valid_class_name(class_name);
			// self,parent,static 其中之一
			} else {
				// 保证调用类的方式有效, self,parent,static 没有用错
				zend_ensure_valid_class_fetch_type(fetch_type);
				// 类名添加引用次数
				zend_string_addref(class_name);
			}
			// 处理易混淆类型：如果类型不是完整路径 。警告：要在类型前面加 “\”
			if (ast->attr == ZEND_NAME_NOT_FQ
					// 并且 类型是 （boolean , integer, double, null） 并且
					&& zend_is_confusable_type(orig_name, &correct_name)
					// 并且类名没有导入过
					&& zend_is_not_imported(orig_name)) {
				// 如果当前有命名空间，添加扩展信息
				const char *extra =
					FC(current_namespace) ? " or import the class with \"use\"" : "";
				// 如果有易混淆名字对应的正确名字
				if (correct_name) {
					// 警告, 测试过（ function a():double{ $a=1;} ）
					zend_error(E_COMPILE_WARNING,
						"\"%s\" will be interpreted as a class name. Did you mean \"%s\"? "
						"Write \"\\%s\"%s to suppress this warning",
						ZSTR_VAL(orig_name), correct_name, ZSTR_VAL(class_name), extra);
				// 如果没有对应的正确名字，应该是专门针对 resource 类型，参见  confusable_types 列表
				} else {
					// 警告, 测试过（ function a():resource{ $a=1;} ）
					zend_error(E_COMPILE_WARNING,
						"\"%s\" is not a supported builtin type "
						"and will be interpreted as a class name. "
						"Write \"\\%s\"%s to suppress this warning",
						ZSTR_VAL(orig_name), ZSTR_VAL(class_name), extra);
				}
			}
			// 类名创建保留字
			class_name = zend_new_interned_string(class_name);
			// ？ zend.c 里定义
			zend_alloc_ce_cache(class_name);
			// 返回新创建的zend_type
			return (zend_type) ZEND_TYPE_INIT_CLASS(class_name, 0, 0);
		}
	}
}

// ing4, 验证两个类型是否有一个是多余的，如果一个包含另一个，复杂的一个算多余的。p1:类型1，p2:类型2
static void zend_are_intersection_types_redundant(zend_type left_type, zend_type right_type)
{
	// left_type 必须是交叉类型
	ZEND_ASSERT(ZEND_TYPE_IS_INTERSECTION(left_type));
	// right_type 必须是交叉类型
	ZEND_ASSERT(ZEND_TYPE_IS_INTERSECTION(right_type));
	// 左侧的类型列表
	zend_type_list *l_type_list = ZEND_TYPE_LIST(left_type);
	// 右侧的类型列表
	zend_type_list *r_type_list = ZEND_TYPE_LIST(right_type);
	// 临时变量
	zend_type_list *smaller_type_list, *larger_type_list;
	// 
	bool flipped = false;

	// 如果左侧的子类型多
	if (r_type_list->num_types < l_type_list->num_types) {
		// 右侧是小类型
		smaller_type_list = r_type_list;
		// 左侧是大类型
		larger_type_list = l_type_list;
		// 已翻转
		flipped = true;
	// 右侧子类型多，或相等
	} else {
		// 左侧是小类型
		smaller_type_list = l_type_list;
		// 右侧是大类型
		larger_type_list = r_type_list;
	}

	// 
	unsigned int sum = 0;
	//
	zend_type *outer_type;
	// 遍历小类型
	ZEND_TYPE_LIST_FOREACH(smaller_type_list, outer_type)
		//
		zend_type *inner_type;
		// 遍历大类型
		ZEND_TYPE_LIST_FOREACH(larger_type_list, inner_type)
			// 如果 大类型和小类型 的子类型名称相同
			if (zend_string_equals_ci(ZEND_TYPE_NAME(*inner_type), ZEND_TYPE_NAME(*outer_type))) {
				// 交叉数量 +1（最大不会超过小类型元素数）
				sum++;
				// 因为：找到一个就跳出
				break;
			}
		// 
		ZEND_TYPE_LIST_FOREACH_END();
	//
	ZEND_TYPE_LIST_FOREACH_END();

	// 如果小类型的每个类型都满足
	if (sum == smaller_type_list->num_types) {
		zend_string *smaller_type_str;
		zend_string *larger_type_str;
		// 如果翻转了
		if (flipped) {
			// 小类型字串：右侧类型
			smaller_type_str = zend_type_to_string(right_type);
			// 大类型字串：左侧类型
			larger_type_str = zend_type_to_string(left_type);
		} else {
			// 小类型字串：左侧类型
			smaller_type_str = zend_type_to_string(left_type);
			// 大类型字串：右侧类型
			larger_type_str = zend_type_to_string(right_type);
		}
		// 如果大小两个子元素数相等
		if (smaller_type_list->num_types == larger_type_list->num_types) {
			// 报错：此类型是多余的
			zend_error_noreturn(E_COMPILE_ERROR, "Type %s is redundant with type %s",
				ZSTR_VAL(smaller_type_str), ZSTR_VAL(larger_type_str));
		} else {
			// 报错：大类型 A 是多余的，它比类型 B 要求更严格
			zend_error_noreturn(E_COMPILE_ERROR, "Type %s is redundant as it is more restrictive than type %s",
				ZSTR_VAL(larger_type_str), ZSTR_VAL(smaller_type_str));
		}
	}
}

// ing3, 检查交叉类型对某单一类型来说，是否是冗余的（包含单一类型）。p1:交叉类型，p2:单一类型
static void zend_is_intersection_type_redundant_by_single_type(zend_type intersection_type, zend_type single_type)
{
	ZEND_ASSERT(ZEND_TYPE_IS_INTERSECTION(intersection_type));
	ZEND_ASSERT(!ZEND_TYPE_IS_INTERSECTION(single_type));

	zend_type *single_intersection_type = NULL;
	// 遍历交叉类型
	ZEND_TYPE_FOREACH(intersection_type, single_intersection_type)
		// 如果找到 冗余的 单个类型
		if (zend_string_equals_ci(ZEND_TYPE_NAME(*single_intersection_type), ZEND_TYPE_NAME(single_type))) {
			// 单个类型转成字串
			zend_string *single_type_str = zend_type_to_string(single_type);
			// 交叉类型转成字串
			zend_string *complete_type = zend_type_to_string(intersection_type);
			// 报错：交叉类型 A 是冗余的 ，它比 类型 B 更严格
			zend_error_noreturn(E_COMPILE_ERROR, "Type %s is redundant as it is more restrictive than type %s",
					ZSTR_VAL(complete_type), ZSTR_VAL(single_type_str));
		}
	ZEND_TYPE_FOREACH_END();
}

// 使用交叉和联合类型优先，把类型列表转成完整类型
/* Used by both intersection and union types prior to transforming the type list to a full zend_type */
// ing3, 检查类型列表 和 一个类型，是否互相冗余
static void zend_is_type_list_redundant_by_single_type(zend_type_list *type_list, zend_type type)
{
	ZEND_ASSERT(!ZEND_TYPE_IS_INTERSECTION(type));
	// 遍历类型列表
	for (size_t i = 0; i < type_list->num_types - 1; i++) {
		// 如果此类型是交叉类型
		if (ZEND_TYPE_IS_INTERSECTION(type_list->types[i])) {
			// 检查交叉类型对某单一类型来说，是否是冗余的（包含单一类型）。p1:交叉类型，p2:单一类型
			zend_is_intersection_type_redundant_by_single_type(type_list->types[i], type);
			// 如果没报错，继续
			continue;
		}
		// 如果不是交叉类型，并且与 type 类型相同
		if (zend_string_equals_ci(ZEND_TYPE_NAME(type_list->types[i]), ZEND_TYPE_NAME(type))) {
			// 取得 type的字串表示 
			zend_string *single_type_str = zend_type_to_string(type);
			// 重复的类型名 A 是冗余的
			zend_error_noreturn(E_COMPILE_ERROR, "Duplicate type %s is redundant", ZSTR_VAL(single_type_str));
		}
	}
}

// 230行，内部引用较多
// ing3, 编译类型名。p1:语句实例，p2:是否强制允null
static zend_type zend_compile_typename(
		zend_ast *ast, bool force_allow_null) /* {{{ */
{
	// 标记，是否可以是null
	bool is_marked_nullable = ast->attr & ZEND_TYPE_NULLABLE;
	// 原始修饰符
	zend_ast_attr orig_ast_attr = ast->attr;
	// 创建一个空类型
	zend_type type = ZEND_TYPE_INIT_NONE(0);

	// 如果可以是null
	if (is_marked_nullable) {
		// 取消掉允null标记
		ast->attr &= ~ZEND_TYPE_NULLABLE;
	}

	// 情况1：如果类型是联合类型
	if (ast->kind == ZEND_AST_TYPE_UNION) {
		// 转成类型列表
		zend_ast_list *list = zend_ast_get_list(ast);
		// 
		zend_type_list *type_list;
		// 默认不是混合类型
		bool is_composite = false;
		// 默认只包含可迭代类型
		bool has_only_iterable_class = true;
		ALLOCA_FLAG(use_heap)

		// 分配类型列表
		type_list = do_alloca(ZEND_TYPE_LIST_SIZE(list->children), use_heap);
		// 子类型数 0
		type_list->num_types = 0;

		// 遍历列表中的每个子类型
		for (uint32_t i = 0; i < list->children; i++) {
			// try语句
			zend_ast *type_ast = list->child[i];
			// 
			zend_type single_type;
			// 新类型的 mask
			uint32_t type_mask = ZEND_TYPE_FULL_MASK(type);

			// 如果子类型是交叉类型
			if (type_ast->kind == ZEND_AST_TYPE_INTERSECTION) {
				// 不包含可迭代类
				has_only_iterable_class = false;
				// 是组合类型
				is_composite = true;
				// 第一个类型可以直接存储成 指针指向的类型
				/* The first class type can be stored directly as the type ptr payload. */
				
				// 如果是复杂类型 并且 不是列表
				if (ZEND_TYPE_IS_COMPLEX(type) && !ZEND_TYPE_HAS_LIST(type)) {
					// 从单个名称 转成 名称列表
					/* Switch from single name to name list. */
					// 类型数量1
					type_list->num_types = 1;
					// 第一个类型
					type_list->types[0] = type;
					// 去掉 _ZEND_TYPE_MAY_BE_MASK 标记
					ZEND_TYPE_FULL_MASK(type_list->types[0]) &= ~_ZEND_TYPE_MAY_BE_MASK;
				}
				// 类型标记成列表类型
				/* Mark type as list type */
				// 给列表类型设置类入口和 type_mask
				ZEND_TYPE_SET_LIST(type, type_list);

				// 编译类型名。p1:语句实例，p2:是否强制允null
				single_type = zend_compile_typename(type_ast, false);
				// 单独类型必须是交叉类型
				ZEND_ASSERT(ZEND_TYPE_IS_INTERSECTION(single_type));

				// 把单个类型添加进去
				type_list->types[type_list->num_types++] = single_type;

				// 检查冗余的类型
				/* Check for trivially redundant class types */
				// 遍历所有类型
				for (size_t i = 0; i < type_list->num_types - 1; i++) {
					// 如果是交叉类型
					if (ZEND_TYPE_IS_INTERSECTION(type_list->types[i])) {
						// 验证两个类型是否有一个是多余的，如果一个包含另一个，复杂的一个算多余的。p1:类型1，p2:类型2
						zend_are_intersection_types_redundant(single_type, type_list->types[i]);
						// 下一个
						continue;
					}
					// 不是交叉类型。是简单类型
					/* Type from type list is a simple type */
					// 检查交叉类型对某单一类型来说，是否是冗余的（包含单一类型）。p1:交叉类型，p2:单一类型
					zend_is_intersection_type_redundant_by_single_type(single_type, type_list->types[i]);
				}
				// 下一个
				continue;
			}

			// 编译单个类型
			single_type = zend_compile_single_typename(type_ast);
			// 取得简单类型的 mask
			uint32_t single_type_mask = ZEND_TYPE_PURE_MASK(single_type);

			// 如果可能是任何类型
			if (single_type_mask == MAY_BE_ANY) {
				// mixed 类型只能单独使用
				zend_error_noreturn(E_COMPILE_ERROR, "Type mixed can only be used as a standalone type");
			}
			// 如果是复杂类型 并且 不是可迭代类型
			if (ZEND_TYPE_IS_COMPLEX(single_type) && !ZEND_TYPE_IS_ITERABLE_FALLBACK(single_type)) {
				// 不仅包含可迭代类
				has_only_iterable_class = false;
			}

			// 类型和 简单类型的 交叉部分
			uint32_t type_mask_overlap = ZEND_TYPE_PURE_MASK(type) & single_type_mask;
			// 如果有交叉
			if (type_mask_overlap) {
				// 取出交叉部分
				zend_type overlap_type = ZEND_TYPE_INIT_MASK(type_mask_overlap);
				// 转成字串
				zend_string *overlap_type_str = zend_type_to_string(overlap_type);
				// 报错：类型复重
				zend_error_noreturn(E_COMPILE_ERROR,
					"Duplicate type %s is redundant", ZSTR_VAL(overlap_type_str));
			}

			// 如果同时是 true 或 false 
			if ( ((type_mask & MAY_BE_TRUE) && (single_type_mask == MAY_BE_FALSE))
					|| ((type_mask & MAY_BE_FALSE) && (single_type_mask == MAY_BE_TRUE)) ) {
				// 不可以同时是 true 和 false
				zend_error_noreturn(E_COMPILE_ERROR,
					"Type contains both true and false, bool should be used instead");
			}
			// 把 单个类型添加到类型里
			ZEND_TYPE_FULL_MASK(type) |= ZEND_TYPE_PURE_MASK(single_type);
			// 去年 mixed 类型
			ZEND_TYPE_FULL_MASK(single_type) &= ~_ZEND_TYPE_MAY_BE_MASK;

			// 如果 单个类型是复杂类型
			if (ZEND_TYPE_IS_COMPLEX(single_type)) {
				// 如果类型不是复杂类型 并且 不是混合类型
				if (!ZEND_TYPE_IS_COMPLEX(type) && !is_composite) {
					// 第一个类型只可以直接存储成 类型指针指的目标
					/* The first class type can be stored directly as the type ptr payload. */
					ZEND_TYPE_SET_PTR(type, ZEND_TYPE_NAME(single_type));
					// 添加 _ZEND_TYPE_NAME_BIT 标记
					ZEND_TYPE_FULL_MASK(type) |= _ZEND_TYPE_NAME_BIT;
				// 如果是复杂类型
				} else {
					// 如果列表里一个类型也没有
					if (type_list->num_types == 0) {
						// 从单个类型转成类型列表
						/* Switch from single name to name list. */
						// 类型数量1
						type_list->num_types = 1;
						// 第一个类型
						type_list->types[0] = type;
						// 去掉 _ZEND_TYPE_MAY_BE_MASK 标记
						ZEND_TYPE_FULL_MASK(type_list->types[0]) &= ~_ZEND_TYPE_MAY_BE_MASK;
						// 给列表类型设置类入口和 type_mask
						ZEND_TYPE_SET_LIST(type, type_list);
					}

					// 把单个类型添加进来
					type_list->types[type_list->num_types++] = single_type;
					// 检查冗余
					/* Check for trivially redundant class types */
					// 检查类型列表 和 一个类型，是否互相冗余
					zend_is_type_list_redundant_by_single_type(type_list, single_type);
				}
			}
		}

		// 如果类型列表不为空
		if (type_list->num_types) {
			// 分配内存创建类型列表
			zend_type_list *list = zend_arena_alloc(
				&CG(arena), ZEND_TYPE_LIST_SIZE(type_list->num_types));
			// 把列表复制过来
			memcpy(list, type_list, ZEND_TYPE_LIST_SIZE(type_list->num_types));
			// 给列表类型设置类入口和 type_mask
			ZEND_TYPE_SET_LIST(type, list);
			// 添加标记 通过ARENA分配
			ZEND_TYPE_FULL_MASK(type) |= _ZEND_TYPE_ARENA_BIT;
			// 标记成联合类型
			/* Inform that the type list is a union type */
			// 添加联合类型标记
			ZEND_TYPE_FULL_MASK(type) |= _ZEND_TYPE_UNION_BIT;
		}
		// 释放 type_list
		free_alloca(type_list, use_heap);
		// 取得类型整数
		uint32_t type_mask = ZEND_TYPE_FULL_MASK(type);
		// 如果可以是对象 并且 (( 不是可迭代类 并且是 复杂类型 ) 或 有static标记 )
		if ((type_mask & MAY_BE_OBJECT) &&
				((!has_only_iterable_class && ZEND_TYPE_IS_COMPLEX(type)) || (type_mask & MAY_BE_STATIC))) {
			// 类型名
			zend_string *type_str = zend_type_to_string(type);
			// 类型包含 对象和 冗余的类类型
			zend_error_noreturn(E_COMPILE_ERROR,
				"Type %s contains both object and a class type, which is redundant",
				ZSTR_VAL(type_str));
		}
	// 情况2：如果是 交叉类型
	} else if (ast->kind == ZEND_AST_TYPE_INTERSECTION) {
		// 语句列表
		zend_ast_list *list = zend_ast_get_list(ast);
		zend_type_list *type_list;

		// 直接在 arena 分配类型列表，它必须和语句中的类型列表元素数量相同。
		/* Allocate the type list directly on the arena as it must be a type
		 * list of the same number of elements as the AST list has children */
		type_list = zend_arena_alloc(&CG(arena), ZEND_TYPE_LIST_SIZE(list->children));
		// 类型数量 0
		type_list->num_types = 0;

		ZEND_ASSERT(list->children > 1);

		// 遍历语句列表
		for (uint32_t i = 0; i < list->children; i++) {
			// 取得类型语句
			zend_ast *type_ast = list->child[i];
			// 编译单个类型名
			zend_type single_type = zend_compile_single_typename(type_ast);

			// 不可能存在联合类型组成的交叉类型，所以让它失效
			// 目前只可能 和 迭代数组和对象？
			/* An intersection of union types cannot exist so invalidate it
			 * Currently only can happen with iterable getting canonicalized to Traversable|array */
			// 如果是可迭代类型
			if (ZEND_TYPE_IS_ITERABLE_FALLBACK(single_type)) {
				// 取得单个类型 字串
				zend_string *standard_type_str = zend_type_to_string(single_type);
				// 报错：此类型不可以成为交叉类型的一部分
				zend_error_noreturn(E_COMPILE_ERROR,
					"Type %s cannot be part of an intersection type", ZSTR_VAL(standard_type_str));
				// 释放类型字串
				zend_string_release_ex(standard_type_str, false);
			}
			// 不可以存在 标准类型的交叉类型。所以让它失效
			/* An intersection of standard types cannot exist so invalidate it */
			// 如果 是有效的基础类型，并且指针为空
			if (ZEND_TYPE_IS_ONLY_MASK(single_type)) {
				// 取得类型字串
				zend_string *standard_type_str = zend_type_to_string(single_type);
				// 报错：此类型不可以成为交叉类型的一部分
				zend_error_noreturn(E_COMPILE_ERROR,
					"Type %s cannot be part of an intersection type", ZSTR_VAL(standard_type_str));
				zend_string_release_ex(standard_type_str, false);
			}
			// 同样检查 self和parent
			/* Check for "self" and "parent" too */
			// 如果遇到 self或parent
			if (zend_string_equals_literal_ci(ZEND_TYPE_NAME(single_type), "self")
					|| zend_string_equals_literal_ci(ZEND_TYPE_NAME(single_type), "parent")) {
				// self和parent不可以成为交叉类型的一部分
				zend_error_noreturn(E_COMPILE_ERROR,
					"Type %s cannot be part of an intersection type", ZSTR_VAL(ZEND_TYPE_NAME(single_type)));
			}
			
			// 把类型添加到列表中
			/* Add type to the type list */
			type_list->types[type_list->num_types++] = single_type;

			// 检查冗余类型
			/* Check for trivially redundant class types */
			// 检查类型列表 和 一个类型，是否互相冗余
			zend_is_type_list_redundant_by_single_type(type_list, single_type);
		}

		// 语句中的类型数必须和类型列表中相同
		ZEND_ASSERT(list->children == type_list->num_types);

		// 隐式的允null交叉类型需要转成 DNF 类型
		/* An implicitly nullable intersection type needs to be converted to a DNF type */
		// 如果强制允null
		if (force_allow_null) {
			// 创建一个空类型
			zend_type intersection_type = ZEND_TYPE_INIT_NONE(0);
			// 给列表类型设置类入口和 type_mask
			ZEND_TYPE_SET_LIST(intersection_type, type_list);
			// 添加交叉类型标记
			ZEND_TYPE_FULL_MASK(intersection_type) |= _ZEND_TYPE_INTERSECTION_BIT;
			// 添加 ARENA 标记
			ZEND_TYPE_FULL_MASK(intersection_type) |= _ZEND_TYPE_ARENA_BIT;

			// 创建 DNF类型
			zend_type_list *dnf_type_list = zend_arena_alloc(&CG(arena), ZEND_TYPE_LIST_SIZE(1));
			// 包含类型数 1个
			dnf_type_list->num_types = 1;
			// 把交叉类型添加进来
			dnf_type_list->types[0] = intersection_type;
			// 给列表类型设置类入口和 type_mask
			ZEND_TYPE_SET_LIST(type, dnf_type_list);
			// 标记成 DNF类型
			/* Inform that the type list is a DNF type */
			// 添加 联合 和 ARENA 标记 
			ZEND_TYPE_FULL_MASK(type) |= _ZEND_TYPE_UNION_BIT;
			ZEND_TYPE_FULL_MASK(type) |= _ZEND_TYPE_ARENA_BIT;
		// 没有强制允null
		} else {
			// 给列表类型设置类入口和 type_mask
			ZEND_TYPE_SET_LIST(type, type_list);
			// 标记成 交叉类型
			/* Inform that the type list is an intersection type */
			// 添加交叉类型标记
			ZEND_TYPE_FULL_MASK(type) |= _ZEND_TYPE_INTERSECTION_BIT;
			// 添加 ARENA 标记
			ZEND_TYPE_FULL_MASK(type) |= _ZEND_TYPE_ARENA_BIT;
		}
	// 情况3：其他情况，不是交叉和联合，就是单个类型了
	} else {
		// 编译单个类型
		type = zend_compile_single_typename(ast);
	}

	// 计算 mask
	uint32_t type_mask = ZEND_TYPE_PURE_MASK(type);

	// 如果可以是任何类型，并且允null
	if (type_mask == MAY_BE_ANY && is_marked_nullable) {
		// 报错：mix类型不能标记成 允null，因为它已经包含null了
		zend_error_noreturn(E_COMPILE_ERROR, "Type mixed cannot be marked as nullable since mixed already includes null");
	}

	// 如果值可能为null 并且 有 is_marked_nullable标记
	if ((type_mask & MAY_BE_NULL) && is_marked_nullable) {
		// 报错：有了null 值就不能标记成允null了
		zend_error_noreturn(E_COMPILE_ERROR, "null cannot be marked as nullable");
	}

	// 如果允null 或 强制允null
	if (is_marked_nullable || force_allow_null) {
		// 添加 MAY_BE_NULL 标记
		ZEND_TYPE_FULL_MASK(type) |= MAY_BE_NULL;
		// 更新mask
		type_mask = ZEND_TYPE_PURE_MASK(type);
	}

	// 如类型里有 void 并且 （是组合类型 或 不仅仅是 void）
	if ((type_mask & MAY_BE_VOID) && (ZEND_TYPE_IS_COMPLEX(type) || type_mask != MAY_BE_VOID)) {
		// 报错：void 只能作为独立类型使用（不能和其他类型组合）
		zend_error_noreturn(E_COMPILE_ERROR, "Void can only be used as a standalone type");
	}

	// 如类型里有 never 并且 （是组合类型 或 不仅仅是 never）
	if ((type_mask & MAY_BE_NEVER) && (ZEND_TYPE_IS_COMPLEX(type) || type_mask != MAY_BE_NEVER)) {
		// 报错：never 只能作为独立类型使用（不能和其他类型组合）
		zend_error_noreturn(E_COMPILE_ERROR, "never can only be used as a standalone type");
	}
	
	// 把原附加值放回去
	ast->attr = orig_ast_attr;
	// 返回类型
	return type;
}
/* }}} */

// 可能从int 转成 float
/* May convert value from int to float. */
// ing4, 检查是否是有效的默认值。
static bool zend_is_valid_default_value(zend_type type, zval *value)
{
	// 必须是有效的type，参见zend_types.h
	ZEND_ASSERT(ZEND_TYPE_IS_SET(type));
	// 如果值属于这个type, 返回true
	if (ZEND_TYPE_CONTAINS_CODE(type, Z_TYPE_P(value))) {
		return 1;
	}
	// 如果类型要求是float ,值是int型
	if ((ZEND_TYPE_FULL_MASK(type) & MAY_BE_DOUBLE) && Z_TYPE_P(value) == IS_LONG) {
		// int 类型可以用在 float 类型的初始化中
		/* Integers are allowed as initializers for floating-point values. */
		convert_to_double(value);
		return 1;
	}
	// 其他情况，返回无效
	return 0;
}

// 修饰属性和修饰符都在这里么？
// ing3, 编译修饰属性。主要是检查，真正的业务逻辑在 /Zend/zend_attributes.c
static void zend_compile_attributes(
	HashTable **attributes, zend_ast *ast, uint32_t offset, uint32_t target, uint32_t promoted
) /* {{{ */ {
	zend_attribute *attr;
	zend_internal_attribute *config;

	zend_ast_list *list = zend_ast_get_list(ast);
	uint32_t g, i, j;
	// 类型必须是修饰属性列表
	ZEND_ASSERT(ast->kind == ZEND_AST_ATTRIBUTE_LIST);

	// 遍历每组修饰属性
	for (g = 0; g < list->children; g++) {
		zend_ast_list *group = zend_ast_get_list(list->child[g]);
		// 必须是一组修饰属性
		ZEND_ASSERT(group->kind == ZEND_AST_ATTRIBUTE_GROUP);
		// 遍历每个修饰属性
		for (i = 0; i < group->children; i++) {
			// 类型必须是修饰属性
			ZEND_ASSERT(group->child[i]->kind == ZEND_AST_ATTRIBUTE);

			zend_ast *el = group->child[i];
			// 如果 带有参数
			if (el->child[1] &&
				// 参数是(...) 。 ZEND_AST_CALLABLE_CONVERT 是 (...) ，参见zend_language_parser.y
			    el->child[1]->kind == ZEND_AST_CALLABLE_CONVERT) {
				// 报错不能创建 Closure 当作 修饰属性的参数
			    zend_error_noreturn(E_COMPILE_ERROR,
			        "Cannot create Closure as attribute argument");
			}
			// 通过语句获取：修饰属性的完整类名
			zend_string *name = zend_resolve_class_name_ast(el->child[0]);
			// 小写类名
			zend_string *lcname = zend_string_tolower_ex(name, false);
			// 参数列表
			zend_ast_list *args = el->child[1] ? zend_ast_get_list(el->child[1]) : NULL;
			// ？ /Zend/zend_attributes.c	
			config = zend_internal_attribute_get(lcname);
			// 销毁小写类名
			zend_string_release(lcname);
			// 排除与类属性修饰符（public,private,protected,readonly）不匹配的内置属性
			/* Exclude internal attributes that do not match on promoted properties. */
			// 如果目标和配置中的 ZEND_ATTRIBUTE_TARGET_ALL 选项 不一致
			if (config && !(target & (config->flags & ZEND_ATTRIBUTE_TARGET_ALL))) {
				// 如果 
				if (promoted & (config->flags & ZEND_ATTRIBUTE_TARGET_ALL)) {
					// 销毁类名，跳过
					zend_string_release(name);
					continue;
				}
			}

			// 是否 strict_types
			uint32_t flags = (CG(active_op_array)->fn_flags & ZEND_ACC_STRICT_TYPES)
				? ZEND_ATTRIBUTE_STRICT_TYPES : 0;
			// ？ 添加修饰属性 /Zend/zend_attributes.c
			attr = zend_add_attribute(
				attributes, name, args ? args->children : 0, flags, offset, el->lineno);
			// 销毁类名
			zend_string_release(name);
			// 如果存在参数列表
			/* Populate arguments */
			if (args) {
				// 类型必须是参数列表
				ZEND_ASSERT(args->kind == ZEND_AST_ARG_LIST);
				// 未使用命名参数
				bool uses_named_args = 0;
				// 遍历参数
				for (j = 0; j < args->children; j++) {
					zend_ast **arg_ast_ptr = &args->child[j];
					zend_ast *arg_ast = *arg_ast_ptr;
					// 如果是打包参数
					if (arg_ast->kind == ZEND_AST_UNPACK) {
						// 报错，修饰属性不可以使用打包参数 (测试过 #[ a(...[1,2,3]) ]）
						zend_error_noreturn(E_COMPILE_ERROR,
							"Cannot use unpacking in attribute argument list");
					}
					// 如果类型是命名参数
					if (arg_ast->kind == ZEND_AST_NAMED_ARG) {
						// 设置参数名
						attr->args[j].name = zend_string_copy(zend_ast_get_str(arg_ast->child[0]));
						// 语句指针
						arg_ast_ptr = &arg_ast->child[1];
						// 已使用命名参数
						uses_named_args = 1;
						// 遍历已有参数
						for (uint32_t k = 0; k < j; k++) {
							// 如果参数名重复，报错 （测试过 #[ a1(a:1,b:2,a:3) ] ）
							if (attr->args[k].name &&
									zend_string_equals(attr->args[k].name, attr->args[j].name)) {
								zend_error_noreturn(E_COMPILE_ERROR, "Duplicate named parameter $%s",
									ZSTR_VAL(attr->args[j].name));
							}
						}
					// 如果用了命名参数
					} else if (uses_named_args) {
						// 报错，顺序参数不可以放在命名参数后面（测试过 #[ a1(a:1,1) ]）
						zend_error_noreturn(E_COMPILE_ERROR,
							"Cannot use positional argument after named argument");
					}

					// 参数转成内置变量
					zend_const_expr_to_zval(
						&attr->args[j].value, arg_ast_ptr, /* allow_dynamic */ true);
				}
			}
		}
	}

	// 如果有添加的修饰属性
	if (*attributes != NULL) {
		// 在第二次循环中检验属性列表（需要检测重复的属性）
		/* Validate attributes in a secondary loop (needed to detect repeated attributes). */
		ZEND_HASH_PACKED_FOREACH_PTR(*attributes, attr) {
			// 和传入的offset 不同 或 属性名已经存在， 跳过
			if (attr->offset != offset || NULL == (config = zend_internal_attribute_get(attr->lcname))) {
				continue;
			}
			// 如果目标和 配置中的 ZEND_ATTRIBUTE_TARGET_ALL 选项不一致
			if (!(target & (config->flags & ZEND_ATTRIBUTE_TARGET_ALL))) {
				zend_string *location = zend_get_attribute_target_names(target);
				zend_string *allowed = zend_get_attribute_target_names(config->flags);

				zend_error_noreturn(E_ERROR, "Attribute \"%s\" cannot target %s (allowed targets: %s)",
					ZSTR_VAL(attr->name), ZSTR_VAL(location), ZSTR_VAL(allowed)
				);
			}

			// 如果配置里没有可重复标记
			if (!(config->flags & ZEND_ATTRIBUTE_IS_REPEATABLE)) {
				// 如果碰到重复属性
				if (zend_is_attribute_repeated(*attributes, attr)) {
					// 报错，不可以使用重复属性
					zend_error_noreturn(E_ERROR, "Attribute \"%s\" must not be repeated", ZSTR_VAL(attr->name));
				}
			}
			// 如果有检验器
			if (config->validator != NULL) {
				// 用检验器检验属性
				config->validator(attr, target, CG(active_class_entry));
			}
		} ZEND_HASH_FOREACH_END();
	}
}
/* }}} */

// ing2，编译形式参数列表，只有zend_compile_func_decl用到。 return_type_ast 返回类型
// 当方法名是 __tostring时，fallback_return_type 值为 IS_STRING
// 这算是特别复杂的一块了
static void zend_compile_params(zend_ast *ast, zend_ast *return_type_ast, uint32_t fallback_return_type) /* {{{ */
{
	// 参数列表
	zend_ast_list *list = zend_ast_get_list(ast);
	uint32_t i;
	// 当前操作码列表
	zend_op_array *op_array = CG(active_op_array);
	zend_arg_info *arg_infos;

	// 1. 处理返回值类型
	// 如果有定义返回结果类型，或者 调用方法是 __tostring
	if (return_type_ast || fallback_return_type) {
		// op_array->arg_info 的第一个元素是返回类型，所以创建时要多创建1个。
		/* Use op_array->arg_info[-1] for return type */
		arg_infos = safe_emalloc(sizeof(zend_arg_info), list->children + 1, 0);
		// 返回结果名称，不需要
		arg_infos->name = NULL;
		// 如果要求返回结果类型
		if (return_type_ast) {
			// 编译类型名。p1:语句实例，p2:是否强制允null
			arg_infos->type = zend_compile_typename(
				return_type_ast, /* force_allow_null */ 0);
			// 如果有返回类型，返回结果 和 当前操作码也标记成：返回引用地址
			ZEND_TYPE_FULL_MASK(arg_infos->type) |= _ZEND_ARG_INFO_FLAGS(
				(op_array->fn_flags & ZEND_ACC_RETURN_REFERENCE) != 0, /* is_variadic */ 0, /* is_tentative */ 0);
		// 如果调用方法是 __tostring
		} else {
			// 返回结果类型为 string
			arg_infos->type = (zend_type) ZEND_TYPE_INIT_CODE(fallback_return_type, 0, 0);
		}
		// 移动到下一个 参数信息
		arg_infos++;
		// 给当前操作码添加标记：有返回类型
		op_array->fn_flags |= ZEND_ACC_HAS_RETURN_TYPE;

		// 如果返回类型是IS_VOID ，并且要求引用返回，报错。 -1 这个序号是如何实现的呢 ？
		if (ZEND_TYPE_CONTAINS_CODE(arg_infos[-1].type, IS_VOID)
				&& (op_array->fn_flags & ZEND_ACC_RETURN_REFERENCE)) {
			// 因为void是不存在，逻辑上无法引用返回，在程序实现上还是可以的
			zend_error(E_DEPRECATED, "Returning by reference from a void function is deprecated");
		}
	// 如果没有要求返回结果类型
	} else {
		// 如果参数为空，直接返回
		if (list->children == 0) {
			return;
		}
		// 否则，初始化集数信息列表
		arg_infos = safe_emalloc(sizeof(zend_arg_info), list->children, 0);
	}

	// 2. 必填参数数量 
	// 查找最后一个必填参数的 序号，给报错信息使用
	/* Find last required parameter number for deprecation message. */
	// 初始化上一个必做的参数序号 -1
	uint32_t last_required_param = (uint32_t) -1;
	// 遍历所有参数的默认值
	for (i = 0; i < list->children; ++i) {
		zend_ast *param_ast = list->child[i];
		// 获得默认值语句
		zend_ast *default_ast_ptr = param_ast->child[2];
		// 参数语句是 参数字典（形参里的 “...”）
		bool is_variadic = (param_ast->attr & ZEND_PARAM_VARIADIC) != 0;
		// 如果没有默认值并且它不是 参数字典
		if (!default_ast_ptr && !is_variadic) {
			// 必须参数 索引号
			last_required_param = i;
		}
	}

	// 3. 遍历所有形参子句
	for (i = 0; i < list->children; ++i) {
		zend_ast *param_ast = list->child[i];
		// 类型子句
		zend_ast *type_ast = param_ast->child[0];
		// 变量子句
		zend_ast *var_ast = param_ast->child[1];
		// 默认值子句
		zend_ast **default_ast_ptr = &param_ast->child[2];
		// 修饰属性
		zend_ast *attributes_ast = param_ast->child[3];
		// 注释文档
		zend_ast *doc_comment_ast = param_ast->child[4];
		// 用 形参名 创建保留字
		zend_string *name = zval_make_interned_string(zend_ast_get_zval(var_ast));
		// 如果参数是引用传递
		bool is_ref = (param_ast->attr & ZEND_PARAM_REF) != 0;
		// 参数语句是 参数字典（形参里的 “...”）
		bool is_variadic = (param_ast->attr & ZEND_PARAM_VARIADIC) != 0;
		// 找出 protected,public,protected,private 这几个修饰符
		uint32_t property_flags = param_ast->attr & (ZEND_ACC_PPP_MASK | ZEND_ACC_READONLY);

		znode var_node, default_node;
		zend_uchar opcode;
		zend_op *opline;
		zend_arg_info *arg_info;
		// 3.1 如果是自动全局变量 _GET,_POST等
		if (zend_is_auto_global(name)) {
			// 报错，不可以重复定义自动全局变量。（ Cannot use auto-global variable %s as parameter 会不会更好些。）
			zend_error_noreturn(E_COMPILE_ERROR, "Cannot re-assign auto-global variable %s",
				ZSTR_VAL(name));
		}

		// 参数变量 类型标记成  编译变量
		var_node.op_type = IS_CV;
		// 查找变量名在当前操作码列表中的顺序号
		var_node.u.op.var = lookup_cv(name);

		// 3.2 如果参数序号和在操作码列表中的序号不一致，说明变量名已经出现过
		if (EX_VAR_TO_NUM(var_node.u.op.var) != i) {
			// 报错，不可以重复定义变量
			zend_error_noreturn(E_COMPILE_ERROR, "Redefinition of parameter $%s",
				ZSTR_VAL(name));
		// 3.3 不可以把 $this 当成参数名
		} else if (zend_string_equals_literal(name, "this")) {
			zend_error_noreturn(E_COMPILE_ERROR, "Cannot use $this as parameter");
		}

		// 3.4 如果操作码列表已经包含 变量字典标记
		if (op_array->fn_flags & ZEND_ACC_VARIADIC) {
			// 报错变量字典只能作为最后一个参数
			zend_error_noreturn(E_COMPILE_ERROR, "Only the last parameter can be variadic");
		}

		// 3.5处理默认值
		// 如果当前参数是变量字典
		if (is_variadic) {
			// 操作码 ZEND_RECV_VARIADIC
			opcode = ZEND_RECV_VARIADIC;
			// 默认类型为 IS_UNUSED
			default_node.op_type = IS_UNUSED;
			// 当前操作码添加 变量字典标记
			op_array->fn_flags |= ZEND_ACC_VARIADIC;

			// 如果有默认值语句
			if (*default_ast_ptr) {
				// 报错，变量字典参数不能有默认值
				zend_error_noreturn(E_COMPILE_ERROR,
					"Variadic parameter cannot have a default value");
			}
		// 如果有默认值
		} else if (*default_ast_ptr) {
			// 这种情况下不可以替换常量，否则它会破坏 ReflectionParameter::getDefaultValueConstantName() 和 ReflectionParameter::isDefaultValueConstant()
			/* we cannot substitute constants here or it will break ReflectionParameter::getDefaultValueConstantName() and ReflectionParameter::isDefaultValueConstant() */
			uint32_t cops = CG(compiler_options);
			// 编译选项里添加 ZEND_COMPILE_NO_CONSTANT_SUBSTITUTION ， ZEND_COMPILE_NO_PERSISTENT_CONSTANT_SUBSTITUTION
			CG(compiler_options) |= ZEND_COMPILE_NO_CONSTANT_SUBSTITUTION | ZEND_COMPILE_NO_PERSISTENT_CONSTANT_SUBSTITUTION;
			// 操作码类型 ZEND_RECV_INIT
			opcode = ZEND_RECV_INIT;
			// 默认值类型：常量
			default_node.op_type = IS_CONST;
			// 计算默认值，结果必须是常量
			zend_const_expr_to_zval(
				&default_node.u.constant, default_ast_ptr, /* allow_dynamic */ true);
			// 还原编译选项
			CG(compiler_options) = cops;

			// 如果存在必须的参数，并且当前参数在最后一个必填参数之前
			if (last_required_param != (uint32_t) -1 && i < last_required_param) {
				// 忽略默认值为null的参数
				/* Ignore parameters of the form "Type $param = null".
				 * This is the PHP 5 style way of writing "?Type $param", so allow it for now. */
				// 间接允null：如果类型存在，并且没有ZEND_TYPE_NULLABLE标记（前面没有？），并且默认值为null
				bool is_implicit_nullable =
					type_ast && !(type_ast->attr & ZEND_TYPE_NULLABLE)
					&& Z_TYPE(default_node.u.constant) == IS_NULL;
				// 如果不是间接的 nullable，说明是必须参数。
				if (!is_implicit_nullable) {
					// 取出最后一个必填参数
					zend_ast *required_param_ast = list->child[last_required_param];
					// 报错 可选参数在必填参数前定义，会间接处理成必填参数
					// （未测试成功） function a($a,a $b=null,$c){}
					zend_error(E_DEPRECATED,
						"Optional parameter $%s declared before required parameter $%s "
						"is implicitly treated as a required parameter",
						ZSTR_VAL(name), ZSTR_VAL(zend_ast_get_str(required_param_ast->child[1])));
				}

				// 不论是否弃用，把这个参数转换成无默认值的必填参数。保证它不会被当成一个有名称的可选参数使用。
				/* Regardless of whether we issue a deprecation, convert this parameter into
				 * a required parameter without a default value. This ensures that it cannot be
				 * used as an optional parameter even with named parameters. */
				// 创建操作码 ZEND_RECV
				opcode = ZEND_RECV;
				// 默认值类型：IS_UNUSED
				default_node.op_type = IS_UNUSED;
				// 销毁原默认值
				zval_ptr_dtor(&default_node.u.constant);
			}
		// 不是字典并且没有默认值
		} else {
			// 创建操作码 ZEND_RECV
			opcode = ZEND_RECV;
			// 默认值类型：IS_UNUSED
			default_node.op_type = IS_UNUSED;
			// 必填参数数量 +1
			op_array->required_num_args = i + 1;
		}

		arg_info = &arg_infos[i];
		arg_info->name = zend_string_copy(name);
		arg_info->type = (zend_type) ZEND_TYPE_INIT_NONE(0);
		// 3.6 处理修饰属性
		// 如果有修饰属性
		if (attributes_ast) {
			// 编译修饰属性
			zend_compile_attributes(
				&op_array->attributes, attributes_ast, i + 1, ZEND_ATTRIBUTE_TARGET_PARAMETER,
				// protected,public,protected,private 这几个修饰符
				property_flags ? ZEND_ATTRIBUTE_TARGET_PROPERTY : 0
			);
		}

		// 3.7处理类型
		// 如果有类型
		if (type_ast) {
			// 如果有默认值，类型为默认值的类型，否则 未定义
			uint32_t default_type = *default_ast_ptr ? Z_TYPE(default_node.u.constant) : IS_UNDEF;
			// 是否允null，如果默认值是null，并且没有 property_flags标记 : protected,public,protected,private 这几个修饰符
			bool force_nullable = default_type == IS_NULL && !property_flags;

			// 添加 ZEND_ACC_HAS_TYPE_HINTS标记
			op_array->fn_flags |= ZEND_ACC_HAS_TYPE_HINTS;
			// 编译类型名。p1:语句实例，p2:是否强制允null
			arg_info->type = zend_compile_typename(type_ast, force_nullable);

			// 如果 类型是void
			if (ZEND_TYPE_FULL_MASK(arg_info->type) & MAY_BE_VOID) {
				// 报错，参数类型不可以是void （已测试：function a($a=1,void $b){}）
				zend_error_noreturn(E_COMPILE_ERROR, "void cannot be used as a parameter type");
			}

			// 如果类型是never
			if (ZEND_TYPE_FULL_MASK(arg_info->type) & MAY_BE_NEVER) {
				// 报错，参数类型不可以是 never
				zend_error_noreturn(E_COMPILE_ERROR, "never cannot be used as a parameter type");
			}

			// 如果未定义类型 ，且类型不是常量语句，且不允null，并且 类型和默认值不匹配
			if (default_type != IS_UNDEF && default_type != IS_CONSTANT_AST && !force_nullable
					&& !zend_is_valid_default_value(arg_info->type, &default_node.u.constant)) {
				// 类型字串
				zend_string *type_str = zend_type_to_string(arg_info->type);
				// 报错：类型不匹配（已测试：function a(int $a='kj'){} ）
				zend_error_noreturn(E_COMPILE_ERROR,
					"Cannot use %s as default value for parameter $%s of type %s",
					zend_get_type_by_const(default_type),
					ZSTR_VAL(name), ZSTR_VAL(type_str));
			}
		}
		// 3.8 创建操作码
		opline = zend_emit_op(NULL, opcode, NULL, &default_node);
		//
		SET_NODE(opline->result, &var_node);
		// 第一个操作对象中记录参数序号
		opline->op1.num = i + 1;

		// 如果有类型语句
		if (type_ast) {
			/* Allocate cache slot to speed-up run-time class resolution */
			// 给 CG(active_op_array) 添加缓存大小，添加 count 个void*指针的大小, 返回缓存大小
			// 有几个类型加几个cache slots
			opline->extended_value =
				zend_alloc_cache_slots(zend_type_get_num_classes(arg_info->type));
		}

		// 参数标记 ？
		uint32_t arg_info_flags = _ZEND_ARG_INFO_FLAGS(is_ref, is_variadic, /* is_tentative */ 0)
			| (property_flags ? _ZEND_IS_PROMOTED_BIT : 0);
		// ？
		ZEND_TYPE_FULL_MASK(arg_info->type) |= arg_info_flags;
		// 如果是 ZEND_RECV 操作码
		if (opcode == ZEND_RECV) {
			// 第二个操作对象中记录 参数类型
			opline->op2.num = type_ast ?
				ZEND_TYPE_FULL_MASK(arg_info->type) : MAY_BE_ANY;
		}

		// 3.9处理 修饰符
		// 如果有 protected,public,protected,private 这几个修饰符
		if (property_flags) {
			// 当前操作码列表
			zend_op_array *op_array = CG(active_op_array);
			// 类入口
			zend_class_entry *scope = op_array->scope;

			// 是否是构造方法
			bool is_ctor =
				scope && zend_is_constructor(op_array->function_name);
			// 如果不是构造方法
			if (!is_ctor) {
				// 不可以在构造方法外面定义 带修饰符的属性，（测试过： function __construct2(private int $a=0) ）
				zend_error_noreturn(E_COMPILE_ERROR,
					"Cannot declare promoted property outside a constructor");
				// 测试过：可以在构造方法里声名类属性 class a{	function __construct(private int $a=0){		var_dump($this);	}}
			}
			// 不可以给抽象方法定义带修饰符的参数。（测试过）
			if ((op_array->fn_flags & ZEND_ACC_ABSTRACT)
					|| (scope->ce_flags & ZEND_ACC_INTERFACE)) {
				zend_error_noreturn(E_COMPILE_ERROR,
					"Cannot declare promoted property in an abstract constructor");
			}
			// 不可以定义带修饰符的字典参数（测试过：function __construct(private ...$c){} ）
			if (is_variadic) {
				zend_error_noreturn(E_COMPILE_ERROR,
					"Cannot declare variadic promoted property");
			}
			// 不可以重复定义相同名称的参数
			if (zend_hash_exists(&scope->properties_info, name)) {
				zend_error_noreturn(E_COMPILE_ERROR, "Cannot redeclare %s::$%s",
					ZSTR_VAL(scope->name), ZSTR_VAL(name));
			}
			// 不可以声名 带修饰符的 闭包类型
			if (ZEND_TYPE_FULL_MASK(arg_info->type) & MAY_BE_CALLABLE) {
				zend_string *str = zend_type_to_string(arg_info->type);
				zend_error_noreturn(E_COMPILE_ERROR,
					"Property %s::$%s cannot have type %s",
					ZSTR_VAL(scope->name), ZSTR_VAL(name), ZSTR_VAL(str));
			}

			// 如果不是readonly 并且 所属类是 readonly 类型
			if (!(property_flags & ZEND_ACC_READONLY) && (scope->ce_flags & ZEND_ACC_READONLY_CLASS)) {
				// 属性标记也要加上 readonly
				property_flags |= ZEND_ACC_READONLY;
			}

			// 重新编译 类型，当它有不同的内存管理要求
			/* Recompile the type, as it has different memory management requirements. */
			// 新类型
			zend_type type = ZEND_TYPE_INIT_NONE(0);
			// 如果有类型语句
			if (type_ast) {
				// 编译类型名。p1:语句实例，p2:是否强制允null
				type = zend_compile_typename(type_ast, /* force_allow_null */ 0);
			}

			// 不要给属性明确的默认值。对于有类型的属性，这表示未初始化，对于无类型的属性，它表示明确的默认值null
			/* Don't give the property an explicit default value. For typed properties this means
			 * uninitialized, for untyped properties it means an implicit null default value. */
			zval default_value;
			// 如果是有效的type，参见zend_types.h
			if (ZEND_TYPE_IS_SET(type)) {
				// 类型设置成未定义
				ZVAL_UNDEF(&default_value);
			// 否则
			} else {
				// 如果有readonly修饰符
				if (property_flags & ZEND_ACC_READONLY) {
					// 报错，readonly 类型的属性必须要有默认傎
					zend_error_noreturn(E_COMPILE_ERROR, "Readonly property %s::$%s must have type",
						ZSTR_VAL(scope->name), ZSTR_VAL(name));
				}
				// 默认值null 
				ZVAL_NULL(&default_value);
			}
			// 注释文档
			zend_string *doc_comment =
				doc_comment_ast ? zend_string_copy(zend_ast_get_str(doc_comment_ast)) : NULL;
			// 属性信息：定义带类型的属性	
			zend_property_info *prop = zend_declare_typed_property(
				scope, name, &default_value, property_flags | ZEND_ACC_PROMOTED, doc_comment, type);
			// 如果有修饰属性语句
			if (attributes_ast) {
				// 编译修饰属性
				zend_compile_attributes(
					&prop->attributes, attributes_ast, 0, ZEND_ATTRIBUTE_TARGET_PROPERTY, ZEND_ATTRIBUTE_TARGET_PARAMETER);
			}
		}
	}
	// 4 更新操作码列表
	// 记录参数数量 
	/* These are assigned at the end to avoid uninitialized memory in case of an error */
	op_array->num_args = list->children;
	// 更新参数信息指针
	op_array->arg_info = arg_infos;

	// 不要把字典参数算进去
	/* Don't count the variadic argument */
	if (op_array->fn_flags & ZEND_ACC_VARIADIC) {
		op_array->num_args--;
	}
	// 5 ?
	zend_set_function_arg_flags((zend_function*)op_array);

	// 6. 遍历所有参数：把有 flags 的参数添加到类属性上
	for (i = 0; i < list->children; i++) {
		zend_ast *param_ast = list->child[i];
		// 是否是引用传递
		bool is_ref = (param_ast->attr & ZEND_PARAM_REF) != 0;
		// 找出 protected,public,protected,private 这几个修饰符
		uint32_t flags = param_ast->attr & (ZEND_ACC_PPP_MASK | ZEND_ACC_READONLY);
		// 如果没有修饰符，跳过
		if (!flags) {
			continue;
		}

		// 6.1 把有 flags 的参数添加到类属性上
		/* Emit $this->prop = $prop for promoted properties. */
		// 参数名
		zend_string *name = zend_ast_get_str(param_ast->child[1]);
		znode name_node, value_node;
		// 类型，常量
		name_node.op_type = IS_CONST;
		// 设置名称
		ZVAL_STR_COPY(&name_node.u.constant, name);
		// 值的类型为编译变量
		value_node.op_type = IS_CV;
		// 值里记录参数序号 = 查找编译变量，如果没有的话创建一个新的并返回序号
		value_node.u.op.var = lookup_cv(name);

		// 创建操作码
		zend_op *opline = zend_emit_op(NULL,
			is_ref ? ZEND_ASSIGN_OBJ_REF : ZEND_ASSIGN_OBJ, NULL, &name_node);
		// 给 CG(active_op_array) 添加缓存大小，添加 count 个void*指针的大小, 返回缓存大小
		opline->extended_value = zend_alloc_cache_slots(3);
		// 创建 ZEND_OP_DATA 操作码
		zend_emit_op_data(&value_node);
	}
}
/* }}} */

// ing3, 编译闭包绑定 use() 中的变量
static void zend_compile_closure_binding(znode *closure, zend_op_array *op_array, zend_ast *uses_ast) /* {{{ */
{
	// use 变量列表
	zend_ast_list *list = zend_ast_get_list(uses_ast);
	uint32_t i;
	// 如果没有use变量，返回
	if (!list->children) {
		return;
	}

	// 如果没有 静态变量列表，创建它
	if (!op_array->static_variables) {
		op_array->static_variables = zend_new_array(8);
	}

	// 遍历变量列表
	for (i = 0; i < list->children; ++i) {
		zend_ast *var_name_ast = list->child[i];
		// 创建保留字串
		zend_string *var_name = zval_make_interned_string(zend_ast_get_zval(var_name_ast));
		// 修饰属性 ？
		uint32_t mode = var_name_ast->attr;
		zend_op *opline;
		zval *value;
		// 不可以把 $this 用在闭包的 use 里
		if (zend_string_equals_literal(var_name, "this")) {
			zend_error_noreturn(E_COMPILE_ERROR, "Cannot use $this as lexical variable");
		}
		// 如果是自动全局变量 _GET,_POST等，报错，不可以把们用在use里
		if (zend_is_auto_global(var_name)) {
			zend_error_noreturn(E_COMPILE_ERROR, "Cannot use auto-global as lexical variable");
		}
		// 把变量名添加到 静态变量列表中
		value = zend_hash_add(op_array->static_variables, var_name, &EG(uninitialized_zval));
		// 如果已经有值，报错，同一个变量不能use两次（测试过）
		if (!value) {
			zend_error_noreturn(E_COMPILE_ERROR,
				"Cannot use variable $%s twice", ZSTR_VAL(var_name));
		}
		// 记录行号
		CG(zend_lineno) = zend_ast_get_lineno(var_name_ast);
		// 创建操作码 ZEND_BIND_LEXICAL
		opline = zend_emit_op(NULL, ZEND_BIND_LEXICAL, closure, NULL);
		// 类型是编译变量（总是去源变量取值）
		opline->op2_type = IS_CV;
		// 变量名 = 查找编译变量，如果没有的话创建一个新的并返回序号
		opline->op2.var = lookup_cv(var_name);
		// 扩展信息， ？
		opline->extended_value =
			(uint32_t)((char*)value - (char*)op_array->static_variables->arData) | mode;
	}
}
/* }}} */

// 闭包信息
typedef struct {
	HashTable uses;
	bool varvars_used;
} closure_info;

// ing3, 递归绑定闭包业务逻辑 里面用到的参数（为了通过use与外部衔接），只有 find_implicit_binds 用到
static void find_implicit_binds_recursively(closure_info *info, zend_ast *ast) {
	// 没有语句，直接返回
	if (!ast) {
		return;
	}

	// 如果类型是变量
	if (ast->kind == ZEND_AST_VAR) {
		// 取得变量名
		zend_ast *name_ast = ast->child[0];
		// 只有这里不递归
		// 如果类型是内置变量，并且 名称类型是字串，
		if (name_ast->kind == ZEND_AST_ZVAL && Z_TYPE_P(zend_ast_get_zval(name_ast)) == IS_STRING) {
			// 变量名称
			zend_string *name = zend_ast_get_str(name_ast);
			// 如果是自动全局变量 _GET,_POST等。返回
			if (zend_is_auto_global(name)) {
				/* These is no need to explicitly import auto-globals. */
				return;
			}
			// 如果变量名称是$this。返回
			if (zend_string_equals_literal(name, "this")) {
				/* $this does not need to be explicitly imported. */
				return;
			}
			// 把变量名添加到 info->uses 里面
			zend_hash_add_empty_element(&info->uses, name);
		// 类型不是内置变量，递归
		} else {
			// ？
			info->varvars_used = 1;
			// 递归
			find_implicit_binds_recursively(info, name_ast);
		}
	// 如果类型是语句列表，递归
	} else if (zend_ast_is_list(ast)) {
		zend_ast_list *list = zend_ast_get_list(ast);
		uint32_t i;
		for (i = 0; i < list->children; i++) {
			// 递归
			find_implicit_binds_recursively(info, list->child[i]);
		}
	// 类型是闭包，递归
	} else if (ast->kind == ZEND_AST_CLOSURE) {
		// 普通闭包，添加 use() 列表
		/* For normal closures add the use() list. */
		zend_ast_decl *closure_ast = (zend_ast_decl *) ast;
		// use语句列表
		zend_ast *uses_ast = closure_ast->child[1];
		// 如果存在use语句
		if (uses_ast) {
			// 遍历每个子语句
			zend_ast_list *uses_list = zend_ast_get_list(uses_ast);
			uint32_t i;
			for (i = 0; i < uses_list->children; i++) {
				// info->uses 哈希表中添加空元素，键名是 use的每一个参数名
				zend_hash_add_empty_element(&info->uses, zend_ast_get_str(uses_list->child[i]));
			}
		}
	// 类型是 fn()=>，递归
	} else if (ast->kind == ZEND_AST_ARROW_FUNC) {
		/* For arrow functions recursively check the expression. */
		zend_ast_decl *closure_ast = (zend_ast_decl *) ast;
		// 递归
		find_implicit_binds_recursively(info, closure_ast->child[2]);
	// ZEND_AST_ZVAL 以外的所有语句，递归
	} else if (!zend_ast_is_special(ast)) {
		// 子语句数量 
		uint32_t i, children = zend_ast_get_num_children(ast);
		// 遍历所有子语句 ，递归绑定
		for (i = 0; i < children; i++) {
			// 递归
			find_implicit_binds_recursively(info, ast->child[i]);
		}
	}
}

// ing3, 绑定闭包业务逻辑里面用到的参数（为了通过use()与外部衔接）
static void find_implicit_binds(closure_info *info, zend_ast *params_ast, zend_ast *stmt_ast)
{
	// 参数列表
	zend_ast_list *param_list = zend_ast_get_list(params_ast);
	uint32_t i;

	// 初始化hash表 info->uses
	zend_hash_init(&info->uses, param_list->children, NULL, NULL, 0);

	// 递归绑定闭包业务逻辑里面用到的参数
	find_implicit_binds_recursively(info, stmt_ast);

	// 遍历 参数列表，把参数名从use 列表里删除
	/* Remove variables that are parameters */
	for (i = 0; i < param_list->children; i++) {
		zend_ast *param_ast = param_list->child[i];
		// 在use 哈希表里
		zend_hash_del(&info->uses, zend_ast_get_str(param_ast->child[1]));
	}
}

// ing3, 给 fn() => 闭包 绑定 use() 字典
static void compile_implicit_lexical_binds(
		closure_info *info, znode *closure, zend_op_array *op_array)
{
	zend_string *var_name;
	zend_op *opline;

	/* TODO We might want to use a special binding mode if varvars_used is set. */
	// 如果没有use
	if (zend_hash_num_elements(&info->uses) == 0) {
		// 直接返回
		return;
	}

	// 如果没有静态变量
	if (!op_array->static_variables) {
		// 创建静态变量表
		op_array->static_variables = zend_new_array(8);
	}

	// 遍历use表
	ZEND_HASH_MAP_FOREACH_STR_KEY(&info->uses, var_name)
		// 把use变量添加到静态变量列表中，值为空 zval
		zval *value = zend_hash_add(
			op_array->static_variables, var_name, &EG(uninitialized_zval));
		// 此变量的 偏移量
		uint32_t offset = (uint32_t)((char*)value - (char*)op_array->static_variables->arData);
		// 创建操作码 ZEND_BIND_LEXICAL
		opline = zend_emit_op(NULL, ZEND_BIND_LEXICAL, closure, NULL);
		// 第二个运算运算为 编译变量
		opline->op2_type = IS_CV;
		// 查找编译变量，如果没有的话创建一个新的并返回序号
		opline->op2.var = lookup_cv(var_name);
		// 操作码中存储偏移量 + 隐式绑定标记
		opline->extended_value = offset | ZEND_BIND_IMPLICIT;
	ZEND_HASH_FOREACH_END();
}

// ing2, 编译闭包的 use 部分。 lexical variable（字典变量） 专指 闭包 Use() 里面的变量。
static void zend_compile_closure_uses(zend_ast *ast) /* {{{ */
{
	// 当前操作码列表
	zend_op_array *op_array = CG(active_op_array);
	// use变量列表
	zend_ast_list *list = zend_ast_get_list(ast);
	uint32_t i;

	// 遍历参数列表
	for (i = 0; i < list->children; ++i) {
		// 显式绑定？
		uint32_t mode = ZEND_BIND_EXPLICIT;
		// 参数名
		zend_ast *var_ast = list->child[i];
		// 获取参数名字串
		zend_string *var_name = zend_ast_get_str(var_ast);
		zval zv;
		ZVAL_NULL(&zv);

		{
			int i;
			// 遍历所形参 ？
			for (i = 0; i < op_array->last_var; i++) {
				// 如果 use() 中的参数和形参同名，报错（测试过）
				if (zend_string_equals(op_array->vars[i], var_name)) {
					// 不可以把use() 里用过的变量名当成参数名
					zend_error_noreturn(E_COMPILE_ERROR,
						"Cannot use lexical variable $%s as a parameter name", ZSTR_VAL(var_name));
				}
			}
		}
		//  记录变量行号
		CG(zend_lineno) = zend_ast_get_lineno(var_ast);

		// 如果变量有修饰属性 
		if (var_ast->attr) {
			// 绑定引用模式
			mode |= ZEND_BIND_REF;
		}
		// 编译静态变量
		zend_compile_static_var_common(var_name, &zv, mode);
	}
}
/* }}} */

// ing3, 编译闭包 use() 子句中的变量（use中的变量在闭包中会被编译成静态变量）
static void zend_compile_implicit_closure_uses(closure_info *info)
{
	zend_string *var_name;
	// 遍历 use 参数列表
	ZEND_HASH_MAP_FOREACH_STR_KEY(&info->uses, var_name)
		zval zv;
		// 值为null
		ZVAL_NULL(&zv);
		// use中的变量在闭包中会被编译成静态变量
		zend_compile_static_var_common(var_name, &zv, ZEND_BIND_IMPLICIT);
	ZEND_HASH_FOREACH_END();
}

// ing4, 添加 stringable 接口，（可使用__tostring方法）
static void add_stringable_interface(zend_class_entry *ce) {
	// 遍历所有接口
	for (uint32_t i = 0; i < ce->num_interfaces; i++) {
		// 如果已经有了 stringable，就不用再添加了
		if (zend_string_equals_literal(ce->interface_names[i].lc_name, "stringable")) {
			/* Interface already explicitly implemented */
			return;
		}
	}

	// 接口数+1
	ce->num_interfaces++;
	// 接口名，列表开辟空间
	ce->interface_names =
		erealloc(ce->interface_names, sizeof(zend_class_name) * ce->num_interfaces);
	// TODO: Add known interned strings instead?
	// 接口名里添加 Stringable
	ce->interface_names[ce->num_interfaces - 1].name =
		zend_string_init("Stringable", sizeof("Stringable") - 1, 0);
	// 接口名里添加 小写 stringable
	ce->interface_names[ce->num_interfaces - 1].lc_name =
		zend_string_init("stringable", sizeof("stringable") - 1, 0);
}

// ing3, 开始类方法定义
static zend_string *zend_begin_method_decl(zend_op_array *op_array, zend_string *name, bool has_body) /* {{{ */
{
	// 所属类
	zend_class_entry *ce = CG(active_class_entry);
	// 是否在接口中
	bool in_interface = (ce->ce_flags & ZEND_ACC_INTERFACE) != 0;
	// 函数标记
	uint32_t fn_flags = op_array->fn_flags;
	// 小写方法名
	zend_string *lcname;

	// 方法名不可以用 readonly 修饰（测试过）
	if (fn_flags & ZEND_ACC_READONLY) {
		zend_error(E_COMPILE_ERROR, "Cannot use 'readonly' as method modifier");
	}

	// privated + final + 不是构造方法，报错（测试过）
	if ((fn_flags & ZEND_ACC_PRIVATE) && (fn_flags & ZEND_ACC_FINAL) && !zend_is_constructor(name)) {
		zend_error(E_COMPILE_WARNING, "Private methods cannot be final as they are never overridden by other classes");
	}

	// 如果在接口里
	if (in_interface) {
		// 接口里的方法必须是 public（测试过）
		if (!(fn_flags & ZEND_ACC_PUBLIC)) {
			zend_error_noreturn(E_COMPILE_ERROR, "Access type for interface method "
				"%s::%s() must be public", ZSTR_VAL(ce->name), ZSTR_VAL(name));
		}
		// 接口里的方法不可以是final（测试过）
		if (fn_flags & ZEND_ACC_FINAL) {
			zend_error_noreturn(E_COMPILE_ERROR, "Interface method "
				"%s::%s() must not be final", ZSTR_VAL(ce->name), ZSTR_VAL(name));
		}
		// 接口里的方法不有 abstract 标记（测试过）
		if (fn_flags & ZEND_ACC_ABSTRACT) {
			zend_error_noreturn(E_COMPILE_ERROR, "Interface method "
				"%s::%s() must not be abstract", ZSTR_VAL(ce->name), ZSTR_VAL(name));
		}
		// 接口里所有方法一定是 abstract
		op_array->fn_flags |= ZEND_ACC_ABSTRACT;
	}

	// 如果是 抽象方法
	if (op_array->fn_flags & ZEND_ACC_ABSTRACT) {
		// 如果是私有 并且不在 trait 里
		if ((op_array->fn_flags & ZEND_ACC_PRIVATE) && !(ce->ce_flags & ZEND_ACC_TRAIT)) {
			zend_error_noreturn(E_COMPILE_ERROR, "%s function %s::%s() cannot be declared private",
				in_interface ? "Interface" : "Abstract", ZSTR_VAL(ce->name), ZSTR_VAL(name));
		}
		// 如果方法包含代码块（不是虚函数）
		if (has_body) {
			// 报错，接口里的方法或者抽象方法，不可以包含业务逻辑
			zend_error_noreturn(E_COMPILE_ERROR, "%s function %s::%s() cannot contain body",
				in_interface ? "Interface" : "Abstract", ZSTR_VAL(ce->name), ZSTR_VAL(name));
		}

		ce->ce_flags |= ZEND_ACC_IMPLICIT_ABSTRACT_CLASS;
	// 不是抽象方法，并且没有代码块，报错（测试过）
	} else if (!has_body) {
		zend_error_noreturn(E_COMPILE_ERROR, "Non-abstract method %s::%s() must contain body",
			ZSTR_VAL(ce->name), ZSTR_VAL(name));
	}

	// 设置操作码的作用域
	op_array->scope = ce;
	// 设置操作码用的方法名
	op_array->function_name = zend_string_copy(name);

	// 方法名小写
	lcname = zend_string_tolower(name);
	// 创建保留字串
	lcname = zend_new_interned_string(lcname);

	// 在成员方法hash表中，添加操作码列表的指针
	if (zend_hash_add_ptr(&ce->function_table, lcname, op_array) == NULL) {
		// 如果返回失败，说明重复定义，报错
		zend_error_noreturn(E_COMPILE_ERROR, "Cannot redeclare %s::%s()",
			ZSTR_VAL(ce->name), ZSTR_VAL(name));
	}

	//添加魔术方法 
	zend_add_magic_method(ce, (zend_function *) op_array, lcname);
	// __tostring, 并且当前在trait中
	if (zend_string_equals_literal(lcname, ZEND_TOSTRING_FUNC_NAME)
			&& !(ce->ce_flags & ZEND_ACC_TRAIT)) {
		// 添加stringable 接口
		add_stringable_interface(ce);
	}

	return lcname;
}
/* }}} */

// ing3, 添加动态函数引用
static uint32_t zend_add_dynamic_func_def(zend_op_array *def) {
	// 当前操作码列表
	zend_op_array *op_array = CG(active_op_array);
	// 当前动态函数定义数量 +1
	uint32_t def_offset = op_array->num_dynamic_func_defs++;
	// 给动态函数指针列表 增加内存
	op_array->dynamic_func_defs = erealloc(
		op_array->dynamic_func_defs, op_array->num_dynamic_func_defs * sizeof(zend_op_array *));
	// 新的函数指针指到当前函数
	op_array->dynamic_func_defs[def_offset] = def;
	// 返回顺序号
	return def_offset;
}

// ing4, 开始声明函数，正式声名前的检查工作
static zend_string *zend_begin_func_decl(znode *result, zend_op_array *op_array, zend_ast_decl *decl, bool toplevel) /* {{{ */
{
	zend_string *unqualified_name, *name, *lcname;
	zend_op *opline;
	// 不带路径的函数名
	unqualified_name = decl->name;
	// 设置函数名，完整路径
	op_array->function_name = name = zend_prefix_with_ns(unqualified_name);
	// 小写函数名
	lcname = zend_string_tolower(name);

	// 如果有引用函数列表
	if (FC(imports_function)) {
		// 在引用列表中查找此函数名
		zend_string *import_name =
			zend_hash_find_ptr_lc(FC(imports_function), unqualified_name);
		// 如果函数名已经在引用列表中
		if (import_name && !zend_string_equals_ci(lcname, import_name)) {
			// 报错，不可以重复定义
			zend_error_noreturn(E_COMPILE_ERROR, "Cannot declare function %s "
				"because the name is already in use", ZSTR_VAL(name));
		}
	}

	// 如果函数名是 __autoload
	if (zend_string_equals_literal(lcname, "__autoload")) {
		// 报错，使用新函数代替它
		zend_error_noreturn(E_COMPILE_ERROR,
			"__autoload() is no longer supported, use spl_autoload_register() instead");
	}

	// 如果函数名是 assert
	if (zend_string_equals_literal_ci(unqualified_name, "assert")) {
		// 报错，不可以定义 assert 函数，它有特殊语义
		zend_error(E_COMPILE_ERROR,
			"Defining a custom assert() function is not allowed, "
			"as the function has special semantics");
	}
	// 函数名加入已知符号列表
	zend_register_seen_symbol(lcname, ZEND_SYMBOL_FUNCTION);
	// 如果在最外层
	if (toplevel) {
		// 在函数表中添加此函数, 失败时报错
		if (UNEXPECTED(zend_hash_add_ptr(CG(function_table), lcname, op_array) == NULL)) {
			do_bind_function_error(lcname, op_array, 1);
		}
	// 如果不是在最外层
	} else {
		// 添加动态函数定义，返回新函数的顺序号
		uint32_t func_ref = zend_add_dynamic_func_def(op_array);
		// 如果函数是闭包
		if (op_array->fn_flags & ZEND_ACC_CLOSURE) {
			// 创建操作码 ZEND_DECLARE_LAMBDA_FUNCTION
			opline = zend_emit_op_tmp(result, ZEND_DECLARE_LAMBDA_FUNCTION, NULL, NULL);
			// 顺序号写到第二个操作对象里
			opline->op2.num = func_ref;
		// 如果不是闭包
		} else {
			// 创建操作码 ZEND_DECLARE_FUNCTION
			opline = get_next_op();
			opline->opcode = ZEND_DECLARE_FUNCTION;
			// 第一个操作对象类型是常量，值为小写函数名
			opline->op1_type = IS_CONST;
			// 添加literal，并把返回的literal序号存放到 op.constant里
			LITERAL_STR(opline->op1, zend_string_copy(lcname));
			// 第二个操作对象记录函数的顺序号
			opline->op2.num = func_ref;
		}
	}
	// 返回小写函数名
	return lcname;
}
/* }}} */

// ing2, 编译函数定义，普通函数， 最外层定义的函数 toplevel=1
static void zend_compile_func_decl(znode *result, zend_ast *ast, bool toplevel) /* {{{ */
{
	// 语句转换成定义语句
	zend_ast_decl *decl = (zend_ast_decl *) ast;
	// 参数表
	zend_ast *params_ast = decl->child[0];
	// use 子句
	zend_ast *uses_ast = decl->child[1];
	// 代码块
	zend_ast *stmt_ast = decl->child[2];
	// 返回类型
	zend_ast *return_type_ast = decl->child[3];
	// 是否是类方法
	bool is_method = decl->kind == ZEND_AST_METHOD;
	// 小写函数名
	zend_string *lcname = NULL;

	// 类作用域
	zend_class_entry *orig_class_entry = CG(active_class_entry);
	// 当前操作码列表
	zend_op_array *orig_op_array = CG(active_op_array);
	// 新操作码列表
	zend_op_array *op_array = zend_arena_alloc(&CG(arena), sizeof(zend_op_array));
	// 源操作码上下文
	zend_oparray_context orig_oparray_context;
	// 闭包的数据
	closure_info info;
	// 指针放在内存开头（干嘛用？）
	memset(&info, 0, sizeof(closure_info));

	// 初始化新操作码列表
	init_op_array(op_array, ZEND_USER_FUNCTION, INITIAL_OP_ARRAY_SIZE);

	// 如果有 ZEND_COMPILE_PRELOAD 标记，
	if (CG(compiler_options) & ZEND_COMPILE_PRELOAD) {
		// 新操作码列表添加 ZEND_ACC_PRELOADED 标记
		op_array->fn_flags |= ZEND_ACC_PRELOADED;
	}

	// 继承原操作码 ZEND_ACC_STRICT_TYPES 标记
	op_array->fn_flags |= (orig_op_array->fn_flags & ZEND_ACC_STRICT_TYPES);
	// 添加本语句的 flags
	op_array->fn_flags |= decl->flags;
	// 首位行号
	op_array->line_start = decl->start_lineno;
	// 结束行号
	op_array->line_end = decl->end_lineno;
	// 文档注释
	if (decl->doc_comment) {
		op_array->doc_comment = zend_string_copy(decl->doc_comment);
	}

	// 如果是闭包 或 fn()=> 型函数
	if (decl->kind == ZEND_AST_CLOSURE || decl->kind == ZEND_AST_ARROW_FUNC) {
		// 添加闭包标记
		op_array->fn_flags |= ZEND_ACC_CLOSURE;
	}

	// 如果是类方法
	if (is_method) {
		bool has_body = stmt_ast != NULL;
		// 开始编译成员方法的定义
		lcname = zend_begin_method_decl(op_array, decl->name, has_body);
	// 如果不是类方法
	} else {
		// 开始定义函数
		lcname = zend_begin_func_decl(result, op_array, decl, toplevel);
		// fn() => 函数
		if (decl->kind == ZEND_AST_ARROW_FUNC) {
			// 绑定间接引用参数
			find_implicit_binds(&info, params_ast, stmt_ast);
			// 给 fn() => 闭包 绑定 use() 字典
			compile_implicit_lexical_binds(&info, result, op_array);
		// 普通函数
		} else if (uses_ast) {
			// 编译绑定闭包
			zend_compile_closure_binding(result, op_array, uses_ast);
		}
	}

	// 当前操作码列表 切换到 新操作码列表
	CG(active_op_array) = op_array;

	// 如果有第四个子元素，修饰属性
	if (decl->child[4]) {
		// 修饰属性类型，普通函数
		int target = ZEND_ATTRIBUTE_TARGET_FUNCTION;

		if (is_method) {
			// 修饰属性类型，类方法
			target = ZEND_ATTRIBUTE_TARGET_METHOD;
		}
		// 编译修饰属性
		zend_compile_attributes(&op_array->attributes, decl->child[4], 0, target, 0);
	}

	// 不可以把 类作用域 泄露给独立函数(非闭包)，即使是在类方法里动态定义的也不可以。
	// 这样才保证魔术常量正确。例如 __CLASS__在独立函数里应该是空（在闭包里不一定）。
	/* Do not leak the class scope into free standing functions, even if they are dynamically
	 * defined inside a class method. This is necessary for correct handling of magic constants.
	 * For example __CLASS__ should always be "" inside a free standing function. */
	if (decl->kind == ZEND_AST_FUNC_DECL) {
		CG(active_class_entry) = NULL;
	}

	// 如果是顶层
	if (toplevel) {
		// 操作码列表添加顶层标记
		op_array->fn_flags |= ZEND_ACC_TOP_LEVEL;
	}

	// 操作码上下文开始
	zend_oparray_context_begin(&orig_oparray_context);

	{
		// 向变量堆栈压入一个分隔符，干嘛用？
		/* Push a separator to the loop variable stack */
		zend_loop_var dummy_var;
		dummy_var.opcode = ZEND_RETURN;
		// 压栈 
		zend_stack_push(&CG(loop_var_stack), (void *) &dummy_var);
	}

	// 编译参数
	zend_compile_params(params_ast, return_type_ast,
		is_method && zend_string_equals_literal(lcname, ZEND_TOSTRING_FUNC_NAME) ? IS_STRING : 0);
	// 是否在 yield 语句中
	if (CG(active_op_array)->fn_flags & ZEND_ACC_GENERATOR) {
		// 把 function 标记生成生器
		zend_mark_function_as_generator();
		// 创建操作码 ZEND_GENERATOR_CREATE
		zend_emit_op(NULL, ZEND_GENERATOR_CREATE, NULL, NULL);
	}
	// fn() 闭包
	if (decl->kind == ZEND_AST_ARROW_FUNC) {
		// 处理 use() 子句的变量。（use中的变量在闭包中会被编译成静态变量）
		zend_compile_implicit_closure_uses(&info);
		// 销毁use变量名列表
		zend_hash_destroy(&info.uses);
	// 普通闭包
	} else if (uses_ast) {
		// 编译 use 语句
		zend_compile_closure_uses(uses_ast);
	}

	// fn()=> 闭包
	if (ast->kind == ZEND_AST_ARROW_FUNC) {
		bool needs_return = true;
		// 如果定义了返回类型
		if (op_array->fn_flags & ZEND_ACC_HAS_RETURN_TYPE) {
			zend_arg_info *return_info = CG(active_op_array)->arg_info - 1;
			// 如果返回类型不是 IS_NEVER （需要有return语句）
			needs_return = !ZEND_TYPE_CONTAINS_CODE(return_info->type, IS_NEVER);
		}
		// 如果需要return语句，自动创建
		if (needs_return) {
			// 创建 ZEND_AST_RETURN 语句
			stmt_ast = zend_ast_create(ZEND_AST_RETURN, stmt_ast);
			// 把 ZEND_AST_RETURN 语句作为当前声名语句的第三个子句
			decl->child[2] = stmt_ast;
		}
	}
	
	// 编译代码块
	zend_compile_stmt(stmt_ast);

	// 如果是类方法
	if (is_method) {
		// 更新开始行
		CG(zend_lineno) = decl->start_lineno;
		// 检验魔术方法的定义是否符合要求，p1:类，p2:方法，p3:方法名，p4:错误类型
		// /Zend/zend_API.c
		zend_check_magic_method_implementation(
			CG(active_class_entry), (zend_function *) op_array, lcname, E_COMPILE_ERROR);
	}

	// 把隐式返回放在真正的最后一行上
	/* put the implicit return on the really last line */
	CG(zend_lineno) = decl->end_lineno;
	// 创建操作码： ZEND_EXT_STMT
	zend_do_extended_stmt();
	// 创建 ZEND_RETURN (返回) 操作码
	zend_emit_final_return(0);
	// ？ zend_opcode.c 
	pass_two(CG(active_op_array));
	// 操作码上下文结束
	zend_oparray_context_end(&orig_oparray_context);

	// 弹出循环变量堆栈的最上一个
	/* Pop the loop variable stack separator */
	zend_stack_del_top(&CG(loop_var_stack));

	// 如果不是0 层
	if (toplevel) {
		// 依次调用每个 声名回调 函数: zend_observer.h
		zend_observer_function_declared_notify(op_array, lcname);
	}
	// 销毁函数名
	zend_string_release_ex(lcname, 0);
	// 当前操作码切回到上一层
	CG(active_op_array) = orig_op_array;
	// 当前类入口切换回上一层
	CG(active_class_entry) = orig_class_entry;
}
/* }}} */

// ing3, 编译属性定义，一次多个
static void zend_compile_prop_decl(zend_ast *ast, zend_ast *type_ast, uint32_t flags, zend_ast *attr_ast) /* {{{ */
{
	// 属性列表
	zend_ast_list *list = zend_ast_get_list(ast);
	// 当前类入口
	zend_class_entry *ce = CG(active_class_entry);
	// 属性数量
	uint32_t i, children = list->children;

	// 如果定义在接口中
	if (ce->ce_flags & ZEND_ACC_INTERFACE) {
		// 报错，接口里不可以有 属性
		zend_error_noreturn(E_COMPILE_ERROR, "Interfaces may not include properties");
	}

	// 如果定义在枚举里
	if (ce->ce_flags & ZEND_ACC_ENUM) {
		// 报错，枚举里不可以包含类属性
		zend_error_noreturn(E_COMPILE_ERROR, "Enum %s cannot include properties", ZSTR_VAL(ce->name));
	}

	// 如果加了 abstract 修饰符，报错，类属性不可以定义是 abstract
	if (flags & ZEND_ACC_ABSTRACT) {
		zend_error_noreturn(E_COMPILE_ERROR, "Properties cannot be declared abstract");
	}

	// 遍历每个属性
	for (i = 0; i < children; ++i) {
		zend_property_info *info;
		zend_ast *prop_ast = list->child[i];
		// 属性名
		zend_ast *name_ast = prop_ast->child[0];
		// 值语句的指针
		zend_ast **value_ast_ptr = &prop_ast->child[1];
		// 注释文档语句
		zend_ast *doc_comment_ast = prop_ast->child[2];
		// 创建保留字串
		zend_string *name = zval_make_interned_string(zend_ast_get_zval(name_ast));
		zend_string *doc_comment = NULL;
		zval value_zv;
		// 初始化type，why？
		zend_type type = ZEND_TYPE_INIT_NONE(0);

		// 如果有定义类型
		if (type_ast) {
			// 编译类型名。p1:语句实例，p2:是否强制允null
			type = zend_compile_typename(type_ast, /* force_allow_null */ 0);

			// 如果类型是 void 或 never 或 闭包
			if (ZEND_TYPE_FULL_MASK(type) & (MAY_BE_VOID|MAY_BE_NEVER|MAY_BE_CALLABLE)) {
				// 报错，属性不可以是这几种类型
				zend_string *str = zend_type_to_string(type);
				zend_error_noreturn(E_COMPILE_ERROR,
					"Property %s::$%s cannot have type %s",
					ZSTR_VAL(ce->name), ZSTR_VAL(name), ZSTR_VAL(str));
			}
		}
		
		// 注释文档已经在 ZEND_AST_PROP_ELEM 语句中追加成最后一个元素
		/* Doc comment has been appended as last element in ZEND_AST_PROP_ELEM ast */
		if (doc_comment_ast) {
			// 复制文档字串
			doc_comment = zend_string_copy(zend_ast_get_str(doc_comment_ast));
		}

		// 如果有 final 修饰符
		if (flags & ZEND_ACC_FINAL) {
			// 报错，final不可以加在属性上，只能加在，类，方法，或类常量上！
			zend_error_noreturn(E_COMPILE_ERROR, "Cannot declare property %s::$%s final, "
				"the final modifier is allowed only for methods, classes, and class constants",
				ZSTR_VAL(ce->name), ZSTR_VAL(name));
		}

		// 如果类属性名已经存在
		if (zend_hash_exists(&ce->properties_info, name)) {
			// 报错，不可以重复定义
			zend_error_noreturn(E_COMPILE_ERROR, "Cannot redeclare %s::$%s",
				ZSTR_VAL(ce->name), ZSTR_VAL(name));
		}

		// 如果有值语句
		if (*value_ast_ptr) {
			// 把值计算成常量
			zend_const_expr_to_zval(&value_zv, value_ast_ptr, /* allow_dynamic */ false);
			// 如果是有效的type（参见zend_types.h） 并且值不是常量，并且不是有效的默认值，报错
			if (ZEND_TYPE_IS_SET(type) && !Z_CONSTANT(value_zv)
					&& !zend_is_valid_default_value(type, &value_zv)) {
				// type 转成tring型
				zend_string *str = zend_type_to_string(type);
				// 如果值为null , type不是 _ZEND_TYPE_INTERSECTION_BIT 类型
				if (Z_TYPE(value_zv) == IS_NULL && !ZEND_TYPE_IS_INTERSECTION(type)) {
					// 类型里添加 MAY_BE_NULL
					ZEND_TYPE_FULL_MASK(type) |= MAY_BE_NULL;
					// 类型转成 string。 加了MAY_BE_NULL，声名时是在类型前面加一个？（测试过）
					zend_string *nullable_str = zend_type_to_string(type);
					// 报错，type默认值不可以是null, 可以用 ?type 这个类型来声名（测试过）
					zend_error_noreturn(E_COMPILE_ERROR,
						"Default value for property of type %s may not be null. "
						"Use the nullable type %s to allow null default value",
						ZSTR_VAL(str), ZSTR_VAL(nullable_str));
				// 其他情况
				} else {
					// 报错，不可以用此默认值，因为类型不匹配
					zend_error_noreturn(E_COMPILE_ERROR,
						"Cannot use %s as default value for property %s::$%s of type %s",
						zend_zval_type_name(&value_zv),
						ZSTR_VAL(ce->name), ZSTR_VAL(name), ZSTR_VAL(str));
				}
			}
		// 是否是有效的type，参见zend_types.h
		} else if (!ZEND_TYPE_IS_SET(type)) {
			ZVAL_NULL(&value_zv);
		} else {
			ZVAL_UNDEF(&value_zv);
		}

		// 如果是只读类
		if ((ce->ce_flags & ZEND_ACC_READONLY_CLASS)) {
			// 此属性也添加 readonly 修饰符
			flags |= ZEND_ACC_READONLY;
		}

		// 如果是 readonly
		if (flags & ZEND_ACC_READONLY) {
			// 如果不是有效的type，参见zend_types.h
			if (!ZEND_TYPE_IS_SET(type)) {
				// 只读类型的属性必须有类型 （测试过）
				zend_error_noreturn(E_COMPILE_ERROR, "Readonly property %s::$%s must have type",
					ZSTR_VAL(ce->name), ZSTR_VAL(name));
			}
			// 如果值已定义
			if (!Z_ISUNDEF(value_zv)) {
				// 只读属性不能有默认值 （测试过）， 它只能赋值1次
				zend_error_noreturn(E_COMPILE_ERROR,
					"Readonly property %s::$%s cannot have default value",
					ZSTR_VAL(ce->name), ZSTR_VAL(name));
			}
			// 如果是静态属性
			if (flags & ZEND_ACC_STATIC) {
				// 静态属性不可以是readonly
				zend_error_noreturn(E_COMPILE_ERROR,
					"Static property %s::$%s cannot be readonly",
					ZSTR_VAL(ce->name), ZSTR_VAL(name));
			}
		}

		// 声名有类型的属性, /Zend/zend_API.h
		info = zend_declare_typed_property(ce, name, &value_zv, flags, doc_comment, type);
		// 修饰符，public,private,protected,adstract,final,readonly
		if (attr_ast) {
			// 编译类成员修饰符
			zend_compile_attributes(&info->attributes, attr_ast, 0, ZEND_ATTRIBUTE_TARGET_PROPERTY, 0);
		}
	}
}
/* }}} */

// ing4, 编译成组的 类属性
static void zend_compile_prop_group(zend_ast *ast) /* {{{ */
{
	// 第一个参数是类型
	zend_ast *type_ast = ast->child[0];
	// 第二个参数是属性名
	zend_ast *prop_ast = ast->child[1];
	// 第三个修饰符。在声名顺序上，修饰符在最前，语法解析时把它当成第三个参数传入了
	zend_ast *attr_ast = ast->child[2];

	// 编译类属性定义
	zend_compile_prop_decl(prop_ast, type_ast, ast->attr, attr_ast);
}
/* }}} */

// ing3, 检查常量和 trait 别名 的修饰符。违规直接报错
static void zend_check_const_and_trait_alias_attr(uint32_t attr, const char* entity) /* {{{ */
{
	// 不可以是 static , abstract, final , readonly
	if (attr & ZEND_ACC_STATIC) {
		zend_error_noreturn(E_COMPILE_ERROR, "Cannot use 'static' as %s modifier", entity);
	} else if (attr & ZEND_ACC_ABSTRACT) {
		zend_error_noreturn(E_COMPILE_ERROR, "Cannot use 'abstract' as %s modifier", entity);
	} else if (attr & ZEND_ACC_FINAL) {
		zend_error_noreturn(E_COMPILE_ERROR, "Cannot use 'final' as %s modifier", entity);
	} else if (attr & ZEND_ACC_READONLY) {
		zend_error_noreturn(E_COMPILE_ERROR, "Cannot use 'readonly' as %s modifier", entity);
	}
}
/* }}} */

// ing3, 编译类常量声明
static void zend_compile_class_const_decl(zend_ast *ast, uint32_t flags, zend_ast *attr_ast) /* {{{ */
{
	// 子语句列表
	zend_ast_list *list = zend_ast_get_list(ast);
	// 当前类入口
	zend_class_entry *ce = CG(active_class_entry);
	// 子句数量 
	uint32_t i, children = list->children;
	// 遍历每个子句
	for (i = 0; i < children; ++i) {
		zend_class_constant *c;
		zend_ast *const_ast = list->child[i];
		// 常量名
		zend_ast *name_ast = const_ast->child[0];
		// 常量值
		zend_ast **value_ast_ptr = &const_ast->child[1];
		// 注释文档
		zend_ast *doc_comment_ast = const_ast->child[2];
		// 创建保留字
		zend_string *name = zval_make_interned_string(zend_ast_get_zval(name_ast));
		// 取出注释文档
		zend_string *doc_comment = doc_comment_ast ? zend_string_copy(zend_ast_get_str(doc_comment_ast)) : NULL;
		//
		zval value_zv;
		// 不可以是 static , abstract, final , readonly, 否则报错
		if (UNEXPECTED(flags & (ZEND_ACC_STATIC|ZEND_ACC_ABSTRACT|ZEND_ACC_READONLY))) {
			zend_check_const_and_trait_alias_attr(flags, "constant");
		}

		// 不可以又是 private 又是 final
		if (UNEXPECTED((flags & ZEND_ACC_PRIVATE) && (flags & ZEND_ACC_FINAL))) {
			zend_error_noreturn(
				E_COMPILE_ERROR, "Private constant %s::%s cannot be final as it is not visible to other classes",
				ZSTR_VAL(ce->name), ZSTR_VAL(name)
			);
		}
		// 编译常量值
		zend_const_expr_to_zval(&value_zv, value_ast_ptr, /* allow_dynamic */ false);
		// 定义类常量业务逻辑
		c = zend_declare_class_constant_ex(ce, name, &value_zv, flags, doc_comment);
		// 如果有修饰符，编译它
		if (attr_ast) {
			zend_compile_attributes(&c->attributes, attr_ast, 0, ZEND_ATTRIBUTE_TARGET_CLASS_CONST, 0);
		}
	}
}
/* }}} */

// ing4, 编译成组的 类常量声明。这两个方法合并成一个也行吧？
static void zend_compile_class_const_group(zend_ast *ast) /* {{{ */
{
	// 常量子句
	zend_ast *const_ast = ast->child[0];
	// 修饰符子句
	zend_ast *attr_ast = ast->child[1];
	// 
	zend_compile_class_const_decl(const_ast, ast->attr, attr_ast);
}
/* }}} */

// ing2, 编译方法引用
static void zend_compile_method_ref(zend_ast *ast, zend_trait_method_reference *method_ref) /* {{{ */
{
	// 类名
	zend_ast *class_ast = ast->child[0];
	// 方法名
	zend_ast *method_ast = ast->child[1];
	// 复制方法名
	method_ref->method_name = zend_string_copy(zend_ast_get_str(method_ast));

	// 如果有类名语句, 这是在trait里吗？
	if (class_ast) {
		// 语句对象中获取类名
		method_ref->class_name = zend_resolve_const_class_name_reference(class_ast, "trait name");
	} else {
		// 类名是null 
		method_ref->class_name = NULL;
	}
}
/* }}} */

// ing3, 编译trait 优先顺序
static void zend_compile_trait_precedence(zend_ast *ast) /* {{{ */
{
	// 方法名
	zend_ast *method_ref_ast = ast->child[0];
	// insteadof 语句
	zend_ast *insteadof_ast = ast->child[1];
	// insteadof 列表
	zend_ast_list *insteadof_list = zend_ast_get_list(insteadof_ast);
	
	// 临时变量
	uint32_t i;

	// 创建 zend_trait_precedence 后面跟着一串 zend_string 指针
	zend_trait_precedence *precedence = emalloc(sizeof(zend_trait_precedence) + (insteadof_list->children - 1) * sizeof(zend_string*));
	// 编译方法引用
	zend_compile_method_ref(method_ref_ast, &precedence->trait_method);
	// 排除数量
	precedence->num_excludes = insteadof_list->children;

	// 遍历
	for (i = 0; i < insteadof_list->children; ++i) {
		zend_ast *name_ast = insteadof_list->child[i];
		// 语句对象中获取类名
		precedence->exclude_class_names[i] =
			zend_resolve_const_class_name_reference(name_ast, "trait name");
	}
	// 添加到优先顺序列表里
	zend_add_to_list(&CG(active_class_entry)->trait_precedences, precedence);
}
/* }}} */

// ing3, 编译trait 别名，zend_compile_use_trait引用
static void zend_compile_trait_alias(zend_ast *ast) /* {{{ */
{
	// trait 里的方法名语句
	zend_ast *method_ref_ast = ast->child[0];
	// 别名语句
	zend_ast *alias_ast = ast->child[1];
	uint32_t modifiers = ast->attr;

	zend_trait_alias *alias;
	// 不可以是 static , abstract, final , readonly
	zend_check_const_and_trait_alias_attr(modifiers, "method");
	// 分配内存创建 trait别名 对象
	alias = emalloc(sizeof(zend_trait_alias));
	// 编译方法引用
	zend_compile_method_ref(method_ref_ast, &alias->trait_method);
	// 可以修改方法修饰符
	alias->modifiers = modifiers;

	// 如果有别名，把别名 复制到 zend_trait_alias 对象里
	if (alias_ast) {
		alias->alias = zend_string_copy(zend_ast_get_str(alias_ast));
	} else {
		alias->alias = NULL;
	}
	// 别名添加到当前入口的 trait别名 列表里
	zend_add_to_list(&CG(active_class_entry)->trait_aliases, alias);
}
/* }}} */

// ing3, 编译 use trait
static void zend_compile_use_trait(zend_ast *ast) /* {{{ */
{
	// trait 列表语句
	zend_ast_list *traits = zend_ast_get_list(ast->child[0]);
	// 适配选项
	zend_ast_list *adaptations = ast->child[1] ? zend_ast_get_list(ast->child[1]) : NULL;
	// 当前类入口
	zend_class_entry *ce = CG(active_class_entry);
	uint32_t i;
	// 给 trait名列表 分配内存
	ce->trait_names = erealloc(ce->trait_names, sizeof(zend_class_name) * (ce->num_traits + traits->children));

	// 遍历所有trait
	for (i = 0; i < traits->children; ++i) {
		//
		zend_ast *trait_ast = traits->child[i];
		// 如果当前在接口定义中，报错
		if (ce->ce_flags & ZEND_ACC_INTERFACE) {
			// 不可以在接口用引用trait
			zend_string *name = zend_ast_get_str(trait_ast);
			zend_error_noreturn(E_COMPILE_ERROR, "Cannot use traits inside of interfaces. "
				"%s is used in %s", ZSTR_VAL(name), ZSTR_VAL(ce->name));
		}
		// 语句对象中获取trait 名
		ce->trait_names[ce->num_traits].name =
			zend_resolve_const_class_name_reference(trait_ast, "trait name");
		// trait 名添加到入口的 trait名列表里
		ce->trait_names[ce->num_traits].lc_name = zend_string_tolower(ce->trait_names[ce->num_traits].name);
		// trait名列表指针右移
		ce->num_traits++;
	}

	// 如果不需要适配，可以返回了
	if (!adaptations) {
		return;
	}

	// 遍历所有适配选项
	for (i = 0; i < adaptations->children; ++i) {
		zend_ast *adaptation_ast = adaptations->child[i];
		switch (adaptation_ast->kind) {
			// 设置优先级
			case ZEND_AST_TRAIT_PRECEDENCE:
				zend_compile_trait_precedence(adaptation_ast);
				break;
			// 设置别名
			case ZEND_AST_TRAIT_ALIAS:
				zend_compile_trait_alias(adaptation_ast);
				break;
			EMPTY_SWITCH_DEFAULT_CASE()
		}
	}
}
/* }}} */

// ing3, 编译类声明的接口实现
static void zend_compile_implements(zend_ast *ast) /* {{{ */
{
	// 实现的接口列表
	zend_ast_list *list = zend_ast_get_list(ast);
	// ce: class entry
	zend_class_entry *ce = CG(active_class_entry);
	// 接口名
	zend_class_name *interface_names;
	uint32_t i;

	// 分配内存，创建每个接口的 zend_class_name
	interface_names = emalloc(sizeof(zend_class_name) * list->children);

	// 遍历每个接口
	for (i = 0; i < list->children; ++i) {
		zend_ast *class_ast = list->child[i];
		// 语句对象中获取类名
		interface_names[i].name =
			zend_resolve_const_class_name_reference(class_ast, "interface name");
		// 小写接口名
		interface_names[i].lc_name = zend_string_tolower(interface_names[i].name);
	}

	// 更新当前类入口的接口数量 
	ce->num_interfaces = list->children;
	// 更新当前类入口的接口名列表
	ce->interface_names = interface_names;
}
/* }}} */

// ing3, 创建匿名类名称： 前缀 + @anonymous + \0 + 文件名 ：行号 + CG(rtd_key_counter)++
// 同一个语句会被调用多次，所以同一个语句会创建多个匿名类
static zend_string *zend_generate_anon_class_name(zend_ast_decl *decl)
{
	// 所属文件名
	zend_string *filename = CG(active_op_array)->filename;
	// 所属行号
	uint32_t start_lineno = decl->start_lineno;

	// 用父类或者第一个接口做前缀
	/* Use parent or first interface as prefix. */
	zend_string *prefix = ZSTR_KNOWN(ZEND_STR_CLASS);
	// 继承语句
	if (decl->child[0]) {
		// 语句对象中获取类名
		prefix = zend_resolve_const_class_name_reference(decl->child[0], "class name");
	// 接口实现语句
	} else if (decl->child[1]) {
		// 可同时实现多个接口
		zend_ast_list *list = zend_ast_get_list(decl->child[1]);
		// 语句对象中获取接口名
		prefix = zend_resolve_const_class_name_reference(list->child[0], "interface name");
	}

	// define ZEND_XLONG_FMT \"%\" PRIx32
	// 
	// 类名，第二个是format。前缀 + @anonymous + \0 + 文件名 ：行号 + CG(rtd_key_counter)++
	zend_string *result = zend_strpprintf(0, "%s@anonymous%c%s:%" PRIu32 "$%" PRIx32,
		ZSTR_VAL(prefix), '\0', ZSTR_VAL(filename), start_lineno, CG(rtd_key_counter)++);
	// 销毁prefix
	zend_string_release(prefix);
	// 创建内置字串
	return zend_new_interned_string(result);
}

// ing3, 编译 enum 指定类型
static void zend_compile_enum_backing_type(zend_class_entry *ce, zend_ast *enum_backing_type_ast)
{
	ZEND_ASSERT(ce->ce_flags & ZEND_ACC_ENUM);
	// 编译类型名。p1:语句实例，p2:是否强制允null
	zend_type type = zend_compile_typename(enum_backing_type_ast, 0);
	// 取出纯类型标记
	uint32_t type_mask = ZEND_TYPE_PURE_MASK(type);
	// 如果是复杂类型 或 不是是null和string
	// 为啥还要判断 complex ?
	if (ZEND_TYPE_IS_COMPLEX(type) || (type_mask != MAY_BE_LONG && type_mask != MAY_BE_STRING)) {
		// 取出类型字串
		zend_string *type_string = zend_type_to_string(type);
		// 报错：枚举类型只能是 整数 或 字串
		zend_error_noreturn(E_COMPILE_ERROR,
			"Enum backing type must be int or string, %s given",
			ZSTR_VAL(type_string));
	}
	// 如果是整数
	if (type_mask == MAY_BE_LONG) {
		// 类型为整数
		ce->enum_backing_type = IS_LONG;
	// 如果是字串
	} else {
		ZEND_ASSERT(type_mask == MAY_BE_STRING);
		// 类型为字串
		ce->enum_backing_type = IS_STRING;
	}
	// 释放临时变量
	zend_type_release(type, 0);
}

// ing2, 编译类定义， 近200行
static void zend_compile_class_decl(znode *result, zend_ast *ast, bool toplevel) /* {{{ */
{
	// 定义语句
	zend_ast_decl *decl = (zend_ast_decl *) ast;
	// 继承语句
	zend_ast *extends_ast = decl->child[0];
	// 实现接口语句
	zend_ast *implements_ast = decl->child[1];
	// 代码块
	zend_ast *stmt_ast = decl->child[2];
	// 枚举类型的 限制类型
	zend_ast *enum_backing_type_ast = decl->child[4];
	// 完整类名，包含命名空间 。小写类名，不包含命名空间。
	zend_string *name, *lcname;
	// 创建类入口
	zend_class_entry *ce = zend_arena_alloc(&CG(arena), sizeof(zend_class_entry));
	// 操作码
	zend_op *opline;
	// 当前类
	zend_class_entry *original_ce = CG(active_class_entry);

	// 如果不是匿名类
	if (EXPECTED((decl->flags & ZEND_ACC_ANON_CLASS) == 0)) {
		// 
		zend_string *unqualified_name = decl->name;
		// 如果已经在类中：提示 不能嵌套定义类（匿名类可以嵌套）
		if (CG(active_class_entry)) {
			zend_error_noreturn(E_COMPILE_ERROR, "Class declarations may not be nested");
		}

		// 必须是有效类名，不能是保留字
		zend_assert_valid_class_name(unqualified_name);
		// 类名前添加命名空间
		name = zend_prefix_with_ns(unqualified_name);
		// 类名创建保留字
		name = zend_new_interned_string(name);
		// 小写类名
		lcname = zend_string_tolower(name);

		// 如果已经导入过类
		if (FC(imports)) {
			// 导入列表中查找类名
			zend_string *import_name =
				zend_hash_find_ptr_lc(FC(imports), unqualified_name);
			// 如果类名已经存在
			if (import_name && !zend_string_equals_ci(lcname, import_name)) {
				zend_error_noreturn(E_COMPILE_ERROR, "Cannot declare class %s "
						"because the name is already in use", ZSTR_VAL(name));
			}
		}
		// 类名加入已知符号列表
		zend_register_seen_symbol(lcname, ZEND_SYMBOL_CLASS);
	// 如果是匿名类
	} else {
		// 找到未被使用过的匿名类名称
		/* Find an anon class name that is not in use yet. */
		name = NULL;
		lcname = NULL;
		// 同一行语句可能创建多个匿名类
		do {
			// 释放类名和小写类名字串
			zend_tmp_string_release(name);
			zend_tmp_string_release(lcname);
			// 创建匿名类名
			name = zend_generate_anon_class_name(decl);
			// 小写匿名类名
			lcname = zend_string_tolower(name);
		// 如果类名存在，重来
		} while (zend_hash_exists(CG(class_table), lcname));
	}
	lcname = zend_new_interned_string(lcname);

	// 标记成用户定义的类
	ce->type = ZEND_USER_CLASS;
	// 类名
	ce->name = name;
	// 初始化类入口
	zend_initialize_class_data(ce, 1);
	// 如果不是匿名类
	if (!(decl->flags & ZEND_ACC_ANON_CLASS)) {
		// 分配类入口缓存。新建一个 map_ptr位置给type_name,把新map_ptr的偏移位置（序号）放进type_name的引用次数里。
		zend_alloc_ce_cache(ce->name);
	}

	// 如果编译配置开启了 ZEND_COMPILE_PRELOAD
	if (CG(compiler_options) & ZEND_COMPILE_PRELOAD) {
		// 添加 ZEND_ACC_PRELOADED 标记
		ce->ce_flags |= ZEND_ACC_PRELOADED;
		// ？
		ZEND_MAP_PTR_NEW(ce->static_members_table);
		ZEND_MAP_PTR_NEW(ce->mutable_data);
	}

	// 添加标记 flags
	ce->ce_flags |= decl->flags;
	// 记录文件名
	ce->info.user.filename = zend_string_copy(zend_get_compiled_filename());
	// 记录起始行号
	ce->info.user.line_start = decl->start_lineno;
	// 记录结束行号
	ce->info.user.line_end = decl->end_lineno;

	// 如果有注释文档
	if (decl->doc_comment) {
		// 记录注释文档
		ce->info.user.doc_comment = zend_string_copy(decl->doc_comment);
	}

	// 如果是匿名类
	if (UNEXPECTED((decl->flags & ZEND_ACC_ANON_CLASS))) {
		// 匿名类不支持序列化
		/* Serialization is not supported for anonymous classes */
		ce->ce_flags |= ZEND_ACC_NOT_SERIALIZABLE;
	}

	// 如果有 继承语句
	if (extends_ast) {
		// 语句对象中获取父类名
		ce->parent_name =
			zend_resolve_const_class_name_reference(extends_ast, "class name");
	}

	// 当前类入口切换到此类
	CG(active_class_entry) = ce;

	// 如果有修饰符
	if (decl->child[3]) {
		// 编译修饰符
		zend_compile_attributes(&ce->attributes, decl->child[3], 0, ZEND_ATTRIBUTE_TARGET_CLASS, 0);
	}

	// 如果有接口实现
	if (implements_ast) {
		// 编译接口实现
		zend_compile_implements(implements_ast);
	}

	// 如果是枚举类型
	if (ce->ce_flags & ZEND_ACC_ENUM) {
		// 如果有限制类型
		if (enum_backing_type_ast != NULL) {
			// 编译枚举限制类型
			zend_compile_enum_backing_type(ce, enum_backing_type_ast);
		}
		// ？ 枚举添加接口 /Zend/zend_enum.c
		zend_enum_add_interfaces(ce);
		// ？ 枚举注册属性
		zend_enum_register_props(ce);
	}

	// 编译代码块
	zend_compile_stmt(stmt_ast);

	// 给最终 的 操作码和error 重置 行号
	/* Reset lineno for final opcodes and errors */
	CG(zend_lineno) = ast->lineno;

	// 如果是抽象类：【含蓄的抽象类】 或 【接口】 或 【trait】 或 【明确的抽象类】
	if ((ce->ce_flags & (ZEND_ACC_IMPLICIT_ABSTRACT_CLASS|ZEND_ACC_INTERFACE|ZEND_ACC_TRAIT|ZEND_ACC_EXPLICIT_ABSTRACT_CLASS)) == ZEND_ACC_IMPLICIT_ABSTRACT_CLASS) {
		// ？ 检查抽象类  /Zend/zend_inheritance.c
		zend_verify_abstract_class(ce);
	}

	// 切换回原来的类入口
	CG(active_class_entry) = original_ce;

	// 如果在顶层
	if (toplevel) {
		// 添加顶层标记
		ce->ce_flags |= ZEND_ACC_TOP_LEVEL;
	}

	// 目前不 预先绑定 实现了接口或应用了 trait 的类
	/* We currently don't early-bind classes that implement interfaces or use traits */
	// 如果没有接口 并且没有 trait 并且 没有 ZEND_COMPILE_WITHOUT_EXECUTION 标记
	if (!ce->num_interfaces && !ce->num_traits
	 && !(CG(compiler_options) & ZEND_COMPILE_WITHOUT_EXECUTION)) {
		// 如果在顶层
		if (toplevel) {
			// 如果有继承语句
			if (extends_ast) {
				zend_class_entry *parent_ce = zend_lookup_class_ex(
					ce->parent_name, NULL, ZEND_FETCH_CLASS_NO_AUTOLOAD);

				if (parent_ce
				 && ((parent_ce->type != ZEND_INTERNAL_CLASS) || !(CG(compiler_options) & ZEND_COMPILE_IGNORE_INTERNAL_CLASSES))
				 // 如果是用户类（通过 zend_compile_class_decl 方法定义的类）
				 && ((parent_ce->type != ZEND_USER_CLASS) || !(CG(compiler_options) & ZEND_COMPILE_IGNORE_OTHER_FILES) || (parent_ce->info.user.filename == ce->info.user.filename))) {

					if (zend_try_early_bind(ce, parent_ce, lcname, NULL)) {
						zend_string_release(lcname);
						return;
					}
				}
			// 如果没有继承语句，把类添加到类列表中
			} else if (EXPECTED(zend_hash_add_ptr(CG(class_table), lcname, ce) != NULL)) {
				// 释放类名
				zend_string_release(lcname);
				// ？ /Zend/zend_inheritance.c	
				zend_build_properties_info_table(ce);
				// 添加 ZEND_ACC_LINKED 标记： 链接了 父类，借口或trait
				ce->ce_flags |= ZEND_ACC_LINKED;
				// 依次调用每个 链接回调 函数 : /Zend/zend_observer.h
				zend_observer_class_linked_notify(ce, lcname);
				return;
			}
		// 如果没有继承语句
		} else if (!extends_ast) {
			// 解除简单类的绑定
			/* Link unbound simple class */
			// ？ /Zend/zend_inheritance.c	
			zend_build_properties_info_table(ce);
			// 添加 ZEND_ACC_LINKED 标记： 链接了 父类，借口或trait
			ce->ce_flags |= ZEND_ACC_LINKED;
		}
	}

	// 操作操作码
	opline = get_next_op();

	// 如果有父类
	if (ce->parent_name) {
		// 小写父类名
		/* Lowercased parent name */
		zend_string *lc_parent_name = zend_string_tolower(ce->parent_name);
		// 第二个操作对象类型是常量
		opline->op2_type = IS_CONST;
		// 值为父类名
		LITERAL_STR(opline->op2, lc_parent_name);
	}
	// 第一个操作对象是常量
	opline->op1_type = IS_CONST;
	// 类名放到第一个操作对象里
	LITERAL_STR(opline->op1, lcname);

	// 如果是匿名类
	if (decl->flags & ZEND_ACC_ANON_CLASS) {
		opline->opcode = ZEND_DECLARE_ANON_CLASS;
		// 给 CG(active_op_array) 添加缓存大小，添加 一个 void*指针的大小，返回添加后的缓存大小
		opline->extended_value = zend_alloc_cache_slot();
		// 把操作码的运行结果返回给 result
		zend_make_var_result(result, opline);
		if (!zend_hash_add_ptr(CG(class_table), lcname, ce)) {
			/* We checked above that the class name is not used. This really shouldn't happen. */
			zend_error_noreturn(E_ERROR,
				"Runtime definition key collision for %s. This is a bug", ZSTR_VAL(name));
		}
	// 如果不是匿名类
	} else {
		// 必须创建一个未使用的 RTD key
		/* Generate RTD keys until we find one that isn't in use yet. */
		zend_string *key = NULL;
		// 不会死循环么？
		do {
			// 释放key
			zend_tmp_string_release(key);
			// 取得运行时类名
			key = zend_build_runtime_definition_key(lcname, decl->start_lineno);
		// 如果添加到类列表不成功，重来
		} while (!zend_hash_add_ptr(CG(class_table), key, ce));

		/* RTD key is placed after lcname literal in op1 */
		// 小写类名添加保留字
		zend_add_literal_string(&key);

		// 操作码类型为 ZEND_DECLARE_CLASS
		opline->opcode = ZEND_DECLARE_CLASS;
		// 如果有继承子句，并且当前在最上层 ，并且编译选项有延时绑定，并且没有接口实现，并且没有引用trait
		if (extends_ast && toplevel
			 && (CG(compiler_options) & ZEND_COMPILE_DELAYED_BINDING)
				/* We currently don't early-bind classes that implement interfaces or use traits */
			 && !ce->num_interfaces && !ce->num_traits
		) {
			// 添加标记 ZEND_ACC_EARLY_BINDING
			CG(active_op_array)->fn_flags |= ZEND_ACC_EARLY_BINDING;
			// 延时声名类
			opline->opcode = ZEND_DECLARE_CLASS_DELAYED;
			// 给 CG(active_op_array) 添加缓存大小，添加 一个 void*指针的大小，返回添加后的缓存大小
			opline->extended_value = zend_alloc_cache_slot();
			// 结果类型是 IS_UNUSED
			opline->result_type = IS_UNUSED;
			// 结果行号为-1
			opline->result.opline_num = -1;
		}
	}
}
/* }}} */

// ing3, 编译 enum 里的 case
static void zend_compile_enum_case(zend_ast *ast)
{
	// 取得当前类入口
	zend_class_entry *enum_class = CG(active_class_entry);
	// 如果当前类不是枚举
	if (!(enum_class->ce_flags & ZEND_ACC_ENUM)) {
		// 报错： case 只能在枚举中用
		zend_error_noreturn(E_COMPILE_ERROR, "Case can only be used in enums");
	}

	// 第一个子元素，case 名，创建内置字串
	zend_string *enum_case_name = zval_make_interned_string(zend_ast_get_zval(ast->child[0]));
	// 枚举名
	zend_string *enum_class_name = enum_class->name;

	//
	zval class_name_zval;
	// 把字串复制到  zval里
	ZVAL_STR_COPY(&class_name_zval, enum_class_name);
	// 给类名创建一个语句对象
	zend_ast *class_name_ast = zend_ast_create_zval(&class_name_zval);

	zval case_name_zval;
	// 把字串复制到  zval里
	ZVAL_STR_COPY(&case_name_zval, enum_case_name);
	// 给case名创建一个语句对象
	zend_ast *case_name_ast = zend_ast_create_zval(&case_name_zval);

	// case 值语句
	zend_ast *case_value_ast = ast->child[1];
	
	// 从原来的 语句中删除 case 值语句 来避免释放它，它会被 zend_const_expr_to_zval 释放
	// Remove case_value_ast from the original AST to avoid freeing it, as it will be freed by zend_const_expr_to_zval
	// 清空指针
	ast->child[1] = NULL;
	// 如果有备用类型 并且 值语句是null
	if (enum_class->enum_backing_type != IS_UNDEF && case_value_ast == NULL) {
		// 报错，有类型的枚举项必须有值
		zend_error_noreturn(E_COMPILE_ERROR, "Case %s of backed enum %s must have a value",
			ZSTR_VAL(enum_case_name),
			ZSTR_VAL(enum_class_name));
	// 如果没有备用类型 并且 值不是null
	} else if (enum_class->enum_backing_type == IS_UNDEF && case_value_ast != NULL) {
		// 报错，没有类型的枚举项 不能有值
		zend_error_noreturn(E_COMPILE_ERROR, "Case %s of non-backed enum %s must not have a value",
			ZSTR_VAL(enum_case_name),
			ZSTR_VAL(enum_class_name));
	}

	// 创建语句 ZEND_AST_CONST_ENUM_INIT 初始化枚举常量语句
	zend_ast *const_enum_init_ast = zend_ast_create(ZEND_AST_CONST_ENUM_INIT, class_name_ast, case_name_ast, case_value_ast);

	zval value_zv;
	// 把上面创建的语句解析成 zval
	zend_const_expr_to_zval(&value_zv, &const_enum_init_ast, /* allow_dynamic */ false);

	// 在 ZEND_AST_ENUM 语句中文档注释被添加成第倒数第二个元素，修饰属性依照惯例放在最后。
	/* Doc comment has been appended as second last element in ZEND_AST_ENUM ast - attributes are conventionally last */
	// 文档注释语句
	zend_ast *doc_comment_ast = ast->child[2];
	// 字串
	zend_string *doc_comment = NULL;
	// 如果有文档注释
	if (doc_comment_ast) {
		// 取出里面的字串
		doc_comment = zend_string_copy(zend_ast_get_str(doc_comment_ast));
	}
	// zend_API.h
	// 计算枚举项的值。定义类常量 p1:类, p2:常量名, p3:常量值, p4:标记, p5:文档注释
	zend_class_constant *c = zend_declare_class_constant_ex(enum_class, enum_case_name, &value_zv, ZEND_ACC_PUBLIC, doc_comment);
	// 添加 ZEND_CLASS_CONST_IS_CASE 标记，表示是枚举里的case
	ZEND_CLASS_CONST_FLAGS(c) |= ZEND_CLASS_CONST_IS_CASE;
	// 销毁临时语句
	zend_ast_destroy(const_enum_init_ast);

	// 修饰属性语句
	zend_ast *attr_ast = ast->child[3];
	// 如果有修饰属性
	if (attr_ast) {
		// 编译修饰属性
		zend_compile_attributes(&c->attributes, attr_ast, 0, ZEND_ATTRIBUTE_TARGET_CLASS_CONST, 0);
	}
}

// ing4, 获取导入哈希表：分配内存创建哈希表，初始化哈希表
static HashTable *zend_get_import_ht(uint32_t type) /* {{{ */
{
	switch (type) {
		// 导入类列表
		case ZEND_SYMBOL_CLASS:
			if (!FC(imports)) {
				FC(imports) = emalloc(sizeof(HashTable));
				zend_hash_init(FC(imports), 8, NULL, str_dtor, 0);
			}
			return FC(imports);
		// 导入函数列表
		case ZEND_SYMBOL_FUNCTION:
			if (!FC(imports_function)) {
				FC(imports_function) = emalloc(sizeof(HashTable));
				zend_hash_init(FC(imports_function), 8, NULL, str_dtor, 0);
			}
			return FC(imports_function);
		// 导入常量列表
		case ZEND_SYMBOL_CONST:
			if (!FC(imports_const)) {
				FC(imports_const) = emalloc(sizeof(HashTable));
				zend_hash_init(FC(imports_const), 8, NULL, str_dtor, 0);
			}
			return FC(imports_const);
		EMPTY_SWITCH_DEFAULT_CASE()
	}

	return NULL;
}
/* }}} */

// clear，获取被use的类型, 用在报错提示中， 函数 和 常量 提示类型，类不提示类型
static char *zend_get_use_type_str(uint32_t type) /* {{{ */
{
	switch (type) {
		case ZEND_SYMBOL_CLASS:
			return "";
		case ZEND_SYMBOL_FUNCTION:
			return " function";
		case ZEND_SYMBOL_CONST:
			return " const";
		EMPTY_SWITCH_DEFAULT_CASE()
	}

	return " unknown";
}
/* }}} */

// ing4, 忽略大小写检查，是否已经use了，zend_compile_use调用
static void zend_check_already_in_use(uint32_t type, zend_string *old_name, zend_string *new_name, zend_string *check_name) /* {{{ */
{
	// 如果忽略大小写后相同
	if (zend_string_equals_ci(old_name, check_name)) {
		return;
	}

	// 否则报错
	// 获取被use的类型, 用在报错提示中， 函数 和 常量 提示类型，类不提示类型 ( 测试过 ）
	zend_error_noreturn(E_COMPILE_ERROR, "Cannot use%s %s as %s because the name "
		"is already in use", zend_get_use_type_str(type), ZSTR_VAL(old_name), ZSTR_VAL(new_name));
}
/* }}} */

// ing4, 编译use 
static void zend_compile_use(zend_ast *ast) /* {{{ */
{
	// 语句列表
	zend_ast_list *list = zend_ast_get_list(ast);
	uint32_t i;
	// 当前命名空间
	zend_string *current_ns = FC(current_namespace);
	// 类型：类，函数，常量 都可以被 use
	uint32_t type = ast->attr;
	// 当前引用列表
	HashTable *current_import = zend_get_import_ht(type);
	// 类常量大小写敏感，其他大小写不敏感
	bool case_sensitive = type == ZEND_SYMBOL_CONST;

	// 遍历每个语句
	for (i = 0; i < list->children; ++i) {
		zend_ast *use_ast = list->child[i];
		// 旧名称
		zend_ast *old_name_ast = use_ast->child[0];
		// 新名称
		zend_ast *new_name_ast = use_ast->child[1];
		// 旧名称字串
		zend_string *old_name = zend_ast_get_str(old_name_ast);
		zend_string *new_name, *lookup_name;

		// 不可以给内置类型设置别名。
		/* Check that we are not attempting to alias a built-in type */
		// use string,int,float 等类型（测试过）
		if (type == ZEND_SYMBOL_CLASS && zend_is_reserved_class_name(old_name)) {
			zend_error_noreturn(E_COMPILE_ERROR,
				"Cannot alias '%s' as it is a built-in type", ZSTR_VAL(old_name));
		}

		// 如果有别名子句
		if (new_name_ast) {
			// 取得别名
			new_name = zend_string_copy(zend_ast_get_str(new_name_ast));
		} else {
			const char *unqualified_name;
			size_t unqualified_name_len;
			// 取得无命名空间的类名
			if (zend_get_unqualified_name(old_name, &unqualified_name, &unqualified_name_len)) {
				/* The form "use A\B" is equivalent to "use A\B as B" */
				new_name = zend_string_init(unqualified_name, unqualified_name_len, 0);
			// 如果获取失败，报错
			} else {
				new_name = zend_string_copy(old_name);
				// 如果当前无命名空间，报错
				if (!current_ns) {
					zend_error(E_WARNING, "The use statement with non-compound name '%s' "
						"has no effect", ZSTR_VAL(new_name));
				}
			}
		}

		// 大小写敏感（只有，类常量）
		if (case_sensitive) {
			lookup_name = zend_string_copy(new_name);
		// 大小写不敏感
		} else {
			lookup_name = zend_string_tolower(new_name);
		}

		// 如果被use 的是类，并且别名是保留字，报错 （ 测试过 use a as int;）
		if (type == ZEND_SYMBOL_CLASS && zend_is_reserved_class_name(new_name)) {
			zend_error_noreturn(E_COMPILE_ERROR, "Cannot use %s as %s because '%s' "
				"is a special class name", ZSTR_VAL(old_name), ZSTR_VAL(new_name), ZSTR_VAL(new_name));
		}

		// 如果当前有命名空间
		if (current_ns) {
			// 命名空间字串，分配内存
			zend_string *ns_name = zend_string_alloc(ZSTR_LEN(current_ns) + 1 + ZSTR_LEN(new_name), 0);
			// 复制小写命名空间
			zend_str_tolower_copy(ZSTR_VAL(ns_name), ZSTR_VAL(current_ns), ZSTR_LEN(current_ns));
			// 添加分隔符
			ZSTR_VAL(ns_name)[ZSTR_LEN(current_ns)] = '\\';
			// 带命名空间的别名
			memcpy(ZSTR_VAL(ns_name) + ZSTR_LEN(current_ns) + 1, ZSTR_VAL(lookup_name), ZSTR_LEN(lookup_name) + 1);
			// 检查是否存在于已知符号列表，已知符号列表：包含类名，函数名，常量名，因为可能重复，所以添加 type
			if (zend_have_seen_symbol(ns_name, type)) {
				// 如果已经use 了，要报错
				zend_check_already_in_use(type, old_name, new_name, ns_name);
			}

			zend_string_efree(ns_name);
		// 检查是否存在于已知符号列表，已知符号列表：包含类名，函数名，常量名，因为可能重复，所以添加 type
		} else if (zend_have_seen_symbol(lookup_name, type)) {
			// 如果已经use 了，要报错
			zend_check_already_in_use(type, old_name, new_name, lookup_name);
		}
		// 旧名称添加引用次数
		zend_string_addref(old_name);
		// 旧名字创建保留字串
		old_name = zend_new_interned_string(old_name);
		// 如果创建引用失败，只可能是名字重复，报错（不太好测试，因为，错误信息和上面一样）
		if (!zend_hash_add_ptr(current_import, lookup_name, old_name)) {
			// 获取被use的类型, 用在报错提示中， 函数 和 常量 提示类型，类不提示类型
			zend_error_noreturn(E_COMPILE_ERROR, "Cannot use%s %s as %s because the name "
				"is already in use", zend_get_use_type_str(type), ZSTR_VAL(old_name), ZSTR_VAL(new_name));
		}

		// 释放新名字和旧名字
		zend_string_release_ex(lookup_name, 0);
		zend_string_release_ex(new_name, 0);
	}
}
/* }}} */

// ing3, 编译打包 use，把里面的每个子句重新创建副本，然后编译
static void zend_compile_group_use(zend_ast *ast) /* {{{ */
{
	uint32_t i;
	// 第一个子元素 中取出命名空间
	zend_string *ns = zend_ast_get_str(ast->child[0]);
	// 第二个子元素中取出 语句列表
	zend_ast_list *list = zend_ast_get_list(ast->child[1]);

	// 遍历所有子语句
	for (i = 0; i < list->children; i++) {
		// 取出子语句
		zend_ast *inline_use, *use = list->child[i];
		// 从语句中取得计算好的内置变量，变量名称
		zval *name_zval = zend_ast_get_zval(use->child[0]);
		// 取出变量名称
		zend_string *name = Z_STR_P(name_zval);
		// 完整名称：命名空间 + 名称
		zend_string *compound_ns = zend_concat_names(ZSTR_VAL(ns), ZSTR_LEN(ns), ZSTR_VAL(name), ZSTR_LEN(name));
		// 释放名称
		zend_string_release_ex(name, 0);
		// 更新到变量名中
		ZVAL_STR(name_zval, compound_ns);
		// 创建临时语句， ZEND_AST_USE ，把当前子句放进去
		inline_use = zend_ast_create_list(1, ZEND_AST_USE, use);
		// 复制附加数据
		inline_use->attr = ast->attr ? ast->attr : use->attr;
		// 编译一个use语句
		zend_compile_use(inline_use);
	}
}
/* }}} */

// ing4, 编译定义常量
static void zend_compile_const_decl(zend_ast *ast) /* {{{ */
{
	// 可以一次定义多个
	zend_ast_list *list = zend_ast_get_list(ast);
	uint32_t i;
	// 遍历每个常量
	for (i = 0; i < list->children; ++i) {
		// 定义语句
		zend_ast *const_ast = list->child[i];
		// 常量名
		zend_ast *name_ast = const_ast->child[0];
		// 值的语句
		zend_ast **value_ast_ptr = &const_ast->child[1];
		// 不含命名空间的名字
		zend_string *unqualified_name = zend_ast_get_str(name_ast);
		// 名字
		zend_string *name;
		//
		znode name_node, value_node;
		// 常量值
		zval *value_zv = &value_node.u.constant;
		// 值类型，常量
		value_node.op_type = IS_CONST;
		// 
		zend_const_expr_to_zval(value_zv, value_ast_ptr, /* allow_dynamic */ true);

		// 获取特殊常量 true,false,null 三个
		if (zend_get_special_const(ZSTR_VAL(unqualified_name), ZSTR_LEN(unqualified_name))) {
			// 报错，不可重新字义这3个常量
			zend_error_noreturn(E_COMPILE_ERROR,
				"Cannot redeclare constant '%s'", ZSTR_VAL(unqualified_name));
		}
		
		// 取得完整路径名
		name = zend_prefix_with_ns(unqualified_name);
		// 创建保留字
		name = zend_new_interned_string(name);

		// 如果有引入常量
		if (FC(imports_const)) {
			// 在引入列表里找常量名
			zend_string *import_name = zend_hash_find_ptr(FC(imports_const), unqualified_name);
			// 如果名称存在，报错，不可以重复定义
			if (import_name && !zend_string_equals(import_name, name)) {
				zend_error_noreturn(E_COMPILE_ERROR, "Cannot declare const %s because "
					"the name is already in use", ZSTR_VAL(name));
			}
		}

		// 结果类型是常量
		name_node.op_type = IS_CONST;
		// 应用完整名字
		ZVAL_STR(&name_node.u.constant, name);

		// 创建操作码 ZEND_DECLARE_CONST
		zend_emit_op(NULL, ZEND_DECLARE_CONST, &name_node, &value_node);
		// 常量名加入已知符号列表
		zend_register_seen_symbol(name, ZEND_SYMBOL_CONST);
	}
}
/* }}}*/

// ing4, 编译 命名空间
static void zend_compile_namespace(zend_ast *ast) /* {{{ */
{
	// 命名空间名字
	zend_ast *name_ast = ast->child[0];
	// 代码块
	zend_ast *stmt_ast = ast->child[1];
	//
	zend_string *name;
	// 是否包含代码块
	bool with_bracket = stmt_ast != NULL;

	// 混合定义或嵌套的命名空间（命名空间不能嵌套定义，见下文）
	/* handle mixed syntax declaration or nested namespaces */
	// 如果没有在带括号的命名空间里
	if (!FC(has_bracketed_namespaces)) {
		// 如果有当前命名空间
		if (FC(current_namespace)) {
			/* previous namespace declarations were unbracketed */
			// 当前命名空间有代码段，报错
			if (with_bracket) {
				zend_error_noreturn(E_COMPILE_ERROR, "Cannot mix bracketed namespace declarations "
					"with unbracketed namespace declarations");
			}
		}
	// 如果在带括号的命名空间里
	} else {
		// 如果没有代码块
		/* previous namespace declarations were bracketed */
		if (!with_bracket) {
			zend_error_noreturn(E_COMPILE_ERROR, "Cannot mix bracketed namespace declarations "
				"with unbracketed namespace declarations");
				
		// 如果已经在命名空间中，或当前在命名空间里？？
		} else if (FC(current_namespace) || FC(in_namespace)) {
			zend_error_noreturn(E_COMPILE_ERROR, "Namespace declarations cannot be nested");
		}
	}

	// 如果 （没有代码块，并且当前不在命名空间里） 或 （有代码块 并且，当前不在有代码块的命名空间里）
	bool is_first_namespace = (!with_bracket && !FC(current_namespace))
		|| (with_bracket && !FC(has_bracketed_namespaces));
	// 如果定义命名空间，但不是第一个语句，报错
	if (is_first_namespace && FAILURE == zend_is_first_statement(ast, /* allow_nop */ 1)) {
		zend_error_noreturn(E_COMPILE_ERROR, "Namespace declaration statement has to be "
			"the very first statement or after any declare call in the script");
	}

	// 如果有正在编译的命名空间名字，释放它
	if (FC(current_namespace)) {
		zend_string_release_ex(FC(current_namespace), 0);
	}

	// 如果有名字语句
	if (name_ast) {
		name = zend_ast_get_str(name_ast);
		// 命名空间名字不可以是namespace
		if (zend_string_equals_literal_ci(name, "namespace")) {
			zend_error_noreturn(E_COMPILE_ERROR, "Cannot use '%s' as namespace name", ZSTR_VAL(name));
		}
		// 设置命名空间名字
		FC(current_namespace) = zend_string_copy(name);
	// 匿名命名空间
	} else {
		FC(current_namespace) = NULL;
	}
	// 重置导入列表
	zend_reset_import_tables();

	// 当前在命名空间中
	FC(in_namespace) = 1;
	// 包含代码块
	if (with_bracket) {
		// 标记：正在编译的命名空间 包含代码块
		FC(has_bracketed_namespaces) = 1;
	}
	// 如果代码块存在，为啥不放在上面一起？
	if (stmt_ast) {
		// 编译代码块
		zend_compile_top_stmt(stmt_ast);
		// 编译命名空间结束, 释放当前命名空间
		zend_end_namespace();
	}
}
/* }}} */

// ing3, 编译 __halt_compiler，实际上是创建了一个常量
static void zend_compile_halt_compiler(zend_ast *ast) /* {{{ */
{
	// 这里可能有一个包含 zend_get_scanned_file_offset 的子语句，在 zend_language_scanner.l 中
	zend_ast *offset_ast = ast->child[0];
	// 取得里面的变量，long 型
	zend_long offset = Z_LVAL_P(zend_ast_get_zval(offset_ast));

	zend_string *filename, *name;
	// 
	const char const_name[] = "__COMPILER_HALT_OFFSET__";

	// 不可以在namespace {} 里面用
	if (FC(has_bracketed_namespaces) && FC(in_namespace)) {
		zend_error_noreturn(E_COMPILE_ERROR,
			"__HALT_COMPILER() can only be used from the outermost scope");
	}

	// 获取正在编译的文件名
	filename = zend_get_compiled_filename();
	// 拼接属性名，把 const_name filename 拼在一起成为 name
	name = zend_mangle_property_name(const_name, sizeof(const_name) - 1,
		ZSTR_VAL(filename), ZSTR_LEN(filename), 0);

	// 创建long型内置常量，值是名称的长度
	zend_register_long_constant(ZSTR_VAL(name), ZSTR_LEN(name), offset, CONST_CS, 0);
	// 释放名称
	zend_string_release_ex(name, 0);
}
/* }}} */

// ing3, 执行语句获取魔术常量结果, 获取失败时返回空字串
static bool zend_try_ct_eval_magic_const(zval *zv, zend_ast *ast) /* {{{ */
{
	// 操作码集合
	zend_op_array *op_array = CG(active_op_array);
	// 当前类实体，叫object也行哇
	zend_class_entry *ce = CG(active_class_entry);

	switch (ast->attr) {
		// clear, 行号，直接返回语句行号
		case T_LINE:
			ZVAL_LONG(zv, ast->lineno);
			break;
		// clear, 文件名，返回当前编译的文件名
		case T_FILE:
			ZVAL_STR_COPY(zv, CG(compiled_filename));
			break;
		// ? 目录名
		case T_DIR:
		{
			// 正在编译的文件
			zend_string *filename = CG(compiled_filename);
			// 创建新string ,复制 filename 
			zend_string *dirname = zend_string_init(ZSTR_VAL(filename), ZSTR_LEN(filename), 0);
			// 获取目录名 win32:php_win32_ioutil_dirname,其他:zend_dirname
#ifdef ZEND_WIN32
			ZSTR_LEN(dirname) = php_win32_ioutil_dirname(ZSTR_VAL(dirname), ZSTR_LEN(dirname));
#else
			ZSTR_LEN(dirname) = zend_dirname(ZSTR_VAL(dirname), ZSTR_LEN(dirname));
#endif
			// 如果获取到的目录名是 .
			if (zend_string_equals_literal(dirname, ".")) {
				dirname = zend_string_extend(dirname, MAXPATHLEN, 0);
#if HAVE_GETCWD
				ZEND_IGNORE_VALUE(VCWD_GETCWD(ZSTR_VAL(dirname), MAXPATHLEN));
#elif HAVE_GETWD
				ZEND_IGNORE_VALUE(VCWD_GETWD(ZSTR_VAL(dirname)));
#endif
				ZSTR_LEN(dirname) = strlen(ZSTR_VAL(dirname));
			}

			ZVAL_STR(zv, dirname);
			break;
		}
		// ing4, 函数名
		case T_FUNC_C:
			if (op_array && op_array->function_name) {
				ZVAL_STR_COPY(zv, op_array->function_name);
			} else {
				ZVAL_EMPTY_STRING(zv);
			}
			break;
		// 成员方法名
		case T_METHOD_C:
			/* Detect whether we are directly inside a class (e.g. a class constant) and treat
			 * this as not being inside a function. */
			if (op_array && ce && !op_array->scope && !(op_array->fn_flags & ZEND_ACC_CLOSURE)) {
				op_array = NULL;
			}
			if (op_array && op_array->function_name) {
				if (op_array->scope) {
					ZVAL_NEW_STR(zv,
						zend_create_member_string(op_array->scope->name, op_array->function_name));
				} else {
					ZVAL_STR_COPY(zv, op_array->function_name);
				}
			} else {
				ZVAL_EMPTY_STRING(zv);
			}
			break;
		// ing3, 类名
		case T_CLASS_C:
			if (ce) {
				// 如果在 trait 里 ，返回false
				if ((ce->ce_flags & ZEND_ACC_TRAIT) != 0) {
					return 0;
				// 否则返回类名
				} else {
					ZVAL_STR_COPY(zv, ce->name);
				}
			} else {
				ZVAL_EMPTY_STRING(zv);
			}
			break;
		// ing3, trait 名
		case T_TRAIT_C:
			// 如果在 trait 里 ，返回trait名
			if (ce && (ce->ce_flags & ZEND_ACC_TRAIT) != 0) {
				ZVAL_STR_COPY(zv, ce->name);
			// 否则返回false
			} else {
				ZVAL_EMPTY_STRING(zv);
			}
			break;
		// ing4, namespace , 返回当前namespace 或 ''
		case T_NS_C:
			if (FC(current_namespace)) {
				ZVAL_STR_COPY(zv, FC(current_namespace));
			} else {
				ZVAL_EMPTY_STRING(zv);
			}
			break;
		EMPTY_SWITCH_DEFAULT_CASE()
	}

	return 1;
}
/* }}} */

// ing3, zval 是否兼容long型(是否能转成数字）
ZEND_API bool zend_is_op_long_compatible(zval *op)
{
	// 数组，不兼容
	if (Z_TYPE_P(op) == IS_ARRAY) {
		return false;
	}

	// 双精度，
	if (Z_TYPE_P(op) == IS_DOUBLE
		// 如果转成 long 后不兼容，这里没调用 zend_dval_to_lval_safe，要自己控制抛错
		&& !zend_is_long_compatible(Z_DVAL_P(op), zend_dval_to_lval(Z_DVAL_P(op)))) {
		return false;
	}

	// 如果是字串
	if (Z_TYPE_P(op) == IS_STRING) {
		double dval = 0;
		// 是否是数字字串
		zend_uchar is_num = is_numeric_str_function(Z_STR_P(op), NULL, &dval);
		// 如果不是数字字串，或者是不能转成数字的 double型，返回false 
		if (is_num == 0 || (is_num == IS_DOUBLE && !zend_is_long_compatible(dval, zend_dval_to_lval(dval)))) {
			return false;
		}
	}

	return true;
}

// ing3, 操作可行性检查。true报错，false不报错。 一个操作符和两个表达式。 少量外部引用
ZEND_API bool zend_binary_op_produces_error(uint32_t opcode, zval *op1, zval *op2) /* {{{ */
{
	// 如果是连接操作，其中一个是数组，报错
	if ((opcode == ZEND_CONCAT || opcode == ZEND_FAST_CONCAT)) {
		/* Array to string warning. */
		return Z_TYPE_P(op1) == IS_ARRAY || Z_TYPE_P(op2) == IS_ARRAY;
	}

	// 如果操作是 + - * / 幂 模 << >> or and xor 其中之一，才检查。其他操作不检查。
	if (!(opcode == ZEND_ADD || opcode == ZEND_SUB || opcode == ZEND_MUL || opcode == ZEND_DIV
               || opcode == ZEND_POW || opcode == ZEND_MOD || opcode == ZEND_SL || opcode == ZEND_SR
               || opcode == ZEND_BW_OR || opcode == ZEND_BW_AND || opcode == ZEND_BW_XOR)) {
		/* Only the numeric operations throw errors. */
		return 0;
	}

	// 如果有表达式是 array
	if (Z_TYPE_P(op1) == IS_ARRAY || Z_TYPE_P(op2) == IS_ARRAY) {
		// 如果算加法，两个参数都是数组，不报错
		if (opcode == ZEND_ADD && Z_TYPE_P(op1) == IS_ARRAY && Z_TYPE_P(op2) == IS_ARRAY) {
			/* Adding two arrays is allowed. */
			return 0;
		}
		// 否则报错
		/* Numeric operators throw when one of the operands is an array. */
		return 1;
	}

	// ? 基本算数运算符碰总要检查（会产生）数字字符错误。二进制运算符碰到两个字串不报错。
	/* While basic arithmetic operators always produce numeric string errors,
	 * bitwise operators don't produce errors if both operands are strings */
	 
	// 布尔运算 or and ^(xor) 其中之一，碰到两个字串，不报错
	if ((opcode == ZEND_BW_OR || opcode == ZEND_BW_AND || opcode == ZEND_BW_XOR)
		&& Z_TYPE_P(op1) == IS_STRING && Z_TYPE_P(op2) == IS_STRING) {
		return 0;
	}

	// 如果第一个是字串，又不是数字字串，报错
	if (Z_TYPE_P(op1) == IS_STRING
		&& !is_numeric_string(Z_STRVAL_P(op1), Z_STRLEN_P(op1), NULL, NULL, 0)) {
		return 1;
	}

	// 如果第二个是字串，又不是数字字串，报错
	if (Z_TYPE_P(op2) == IS_STRING
		&& !is_numeric_string(Z_STRVAL_P(op2), Z_STRLEN_P(op2), NULL, NULL, 0)) {
		return 1;
	}

	// 如果用0做除数来取余或，用0.0做除数做除法，直接报错
	if ((opcode == ZEND_MOD && zval_get_long(op2) == 0)
			|| (opcode == ZEND_DIV && zval_get_double(op2) == 0.0)) {
		/* Division by zero throws an error. */
		return 1;
	}
	
	// 对负数进行位移，报错
	if ((opcode == ZEND_SL || opcode == ZEND_SR) && zval_get_long(op2) < 0) {
		/* Shift by negative number throws an error. */
		return 1;
	}

	// 把 浮点数/浮点字串 转换成 整数 可能产生 【转换不兼容】错误
	/* Operation which cast float/float-strings to integers might produce incompatible float to int errors */
	if (opcode == ZEND_SL || opcode == ZEND_SR || opcode == ZEND_BW_OR
			|| opcode == ZEND_BW_AND || opcode == ZEND_BW_XOR || opcode == ZEND_MOD) {
		// op1 不能转成数字 或 op2 不能转成数字 
		return !zend_is_op_long_compatible(op1) || !zend_is_op_long_compatible(op2);
	}

	return 0;
}
/* }}} */

// ing4, 二元操作符运算，内部引用2
static inline bool zend_try_ct_eval_binary_op(zval *result, uint32_t opcode, zval *op1, zval *op2) /* {{{ */
{
	// 检查运算是否可行，如果无法进行，返回 false
	if (zend_binary_op_produces_error(opcode, op1, op2)) {
		return 0;
	}

	// 取得运算函数
	binary_op_type fn = get_binary_op(opcode);
	// 执行运算
	fn(result, op1, op2);
	return 1;
}
/* }}} */

// ing3, ！~ 两个操作符，可行性检查
ZEND_API bool zend_unary_op_produces_error(uint32_t opcode, zval *op)
{
	// 如果要 更换符号，~
	if (opcode == ZEND_BW_NOT) {
		// 给string转换符号时，不把它转成int。
		/* BW_NOT on string does not convert the string into an integer. */
		// 碰到string 直接返回 false（不报错）
		if (Z_TYPE_P(op) == IS_STRING) {
			return 0;
		}
		// IS_UNDEF，IS_NULL，IS_FALSE，IS_TRUE。 这四种情况报错（测试过）
		// 其他情况验证 long 兼容性，兼容才报错（测试？）
		// zval 是否兼容long型(是否能转成数字）
		return Z_TYPE_P(op) <= IS_TRUE || !zend_is_op_long_compatible(op);
	}
	// 其他操作返回 false（不报错）
	return 0;
}

// ing3, ！~ 两个操作符。内部引用2
static inline bool zend_try_ct_eval_unary_op(zval *result, uint32_t opcode, zval *op) /* {{{ */
{
	if (zend_unary_op_produces_error(opcode, op)) {
		return 0;
	}
	// 获取操作函数，在 zend_operators.c 里
	unary_op_type fn = get_unary_op(opcode);
	fn(result, op);
	return 1;
}
/* }}} */

// ing4, +- 号的运算
static inline bool zend_try_ct_eval_unary_pm(zval *result, zend_ast_kind kind, zval *op) /* {{{ */
{
	zval right;
	// right 是被乘数， 正号要 乘1 负号 乘 -1
	ZVAL_LONG(&right, (kind == ZEND_AST_UNARY_PLUS) ? 1 : -1);
	// 执行乘法运算
	return zend_try_ct_eval_binary_op(result, ZEND_MUL, op, &right);
}
/* }}} */

// ing4, >和>=运算， 比较两个 zval 的大小
static inline void zend_ct_eval_greater(zval *result, zend_ast_kind kind, zval *op1, zval *op2) /* {{{ */
{
	binary_op_type fn = kind == ZEND_AST_GREATER
		? is_smaller_function : is_smaller_or_equal_function;
	fn(result, op2, op1);
}
/* }}} */

// ing3, 尝试计算 array，内部调用2次
static bool zend_try_ct_eval_array(zval *result, zend_ast *ast) /* {{{ */
{
	// 语句列表
	zend_ast_list *list = zend_ast_get_list(ast);
	zend_ast *last_elem_ast = NULL;
	uint32_t i;
	bool is_constant = 1;

	// 不可以用 list() 当做独立语句。 只能是以下3种用法，
	// ZEND_ARRAY_SYNTAX_LIST: 情况1. list($a,$b) = [1,2];
	// 情况2. foreach($a as list($a,$b))， 情况3. foreach($a as $k => list($a,$b))
	// 以及嵌套用法 以及嵌套用法 foreach([] as list($a,list($b,$c))){}
	if (ast->attr == ZEND_ARRAY_SYNTAX_LIST) {
		zend_error(E_COMPILE_ERROR, "Cannot use list() as standalone expression");
	}

	// 首先保证所有子语句 都是常量并且附带 value
	/* First ensure that *all* child nodes are constant and by-val */
	// 遍历所有子语句
	for (i = 0; i < list->children; ++i) {
		zend_ast *elem_ast = list->child[i];
		// 如果碰到 null 语句
		if (elem_ast == NULL) {
			// 在最后一个非 null 语句这一行报错
			/* Report error at line of last non-empty element */
			// 如果有非 null 元素，取得行号
			if (last_elem_ast) {
				CG(zend_lineno) = zend_ast_get_lineno(last_elem_ast);
			}
			// 报错,如何测试？
			zend_error(E_COMPILE_ERROR, "Cannot use empty array elements in arrays");
		}
		
		// 如果不需要解包
		if (elem_ast->kind != ZEND_AST_UNPACK) {
			zend_eval_const_expr(&elem_ast->child[0]);
			zend_eval_const_expr(&elem_ast->child[1]);
			// 如果语句有附加属性(说明是带引用符号的，参见parser) 
			// 或 第一个子元素类型不是内置变量 
			// 或 （第二个子元素有效 并且 不是内置变量）
			if (elem_ast->attr /* by_ref */ || elem_ast->child[0]->kind != ZEND_AST_ZVAL
				|| (elem_ast->child[1] && elem_ast->child[1]->kind != ZEND_AST_ZVAL)
			) {
				// 返回类型不是常量
				is_constant = 0;
			}
		// 如果需要解包
		} else {
			// 编译第一个子语句
			zend_eval_const_expr(&elem_ast->child[0]);
			// 如果结果不是内置变量
			if (elem_ast->child[0]->kind != ZEND_AST_ZVAL) {
				// 返回类型不是常量
				is_constant = 0;
			}
		}
		// 到这里说明元素是有效的
		last_elem_ast = elem_ast;
	}

	// 不是常量的话可以返回了
	if (!is_constant) {
		return 0;
	}

	// 如果没有有效元素，返回空数组。这个判断为什么不放在前面呢？
	if (!list->children) {
		ZVAL_EMPTY_ARRAY(result);
		return 1;
	}

	// 初始化数组，分配内存，创建 nSize大小 非持久哈希表
	array_init_size(result, list->children);
	// 遍历所有子句
	for (i = 0; i < list->children; ++i) {
		zend_ast *elem_ast = list->child[i];
		// 值语句
		zend_ast *value_ast = elem_ast->child[0];
		zend_ast *key_ast;
		// 从值语句中取出值
		zval *value = zend_ast_get_zval(value_ast);
		// 如果类型是打包参数或数组
		if (elem_ast->kind == ZEND_AST_UNPACK) {
			// 如果类型是数组 
			if (Z_TYPE_P(value) == IS_ARRAY) {
				HashTable *ht = Z_ARRVAL_P(value);
				zval *val;
				zend_string *key;

				// 遍历ht 
				ZEND_HASH_FOREACH_STR_KEY_VAL(ht, key, val) {
					// 如果key有效
					if (key) {
						// 更新对应的值
						zend_hash_update(Z_ARRVAL_P(result), key, val);
					// 给返回结果插入元素，如果失败，返回false
					} else if (!zend_hash_next_index_insert(Z_ARRVAL_P(result), val)) {
						// 销毁返回结果指针
						zval_ptr_dtor(result);
						return 0;
					}
					// 给 val 添加引用数
					Z_TRY_ADDREF_P(val);
				} ZEND_HASH_FOREACH_END();

				continue;
			// 只有数组和实现 Traversable 接口的对象可以解包
			} else {
				// 只有数组或 Traversable 类型可以解包
				zend_error_noreturn(E_COMPILE_ERROR, "Only arrays and Traversables can be unpacked");
			}
		}
		
		// 给 val 添加引用数
		Z_TRY_ADDREF_P(value);

		// 键名语句
		key_ast = elem_ast->child[1];
		// 如果键名语句存在
		if (key_ast) {
			zval *key = zend_ast_get_zval(key_ast);
			// 按 键名类型操作
			switch (Z_TYPE_P(key)) {
				// 如果键名是整数，更新返回结果哈希表
				case IS_LONG:
					zend_hash_index_update(Z_ARRVAL_P(result), Z_LVAL_P(key), value);
					break;
				// 如果键名是字串，更新返回结果哈希表
				case IS_STRING:
					// 更新哈希表，数字字串当成数字
					zend_symtable_update(Z_ARRVAL_P(result), Z_STR_P(key), value);
					break;
				// 如果键名是双精度。 [1.6=>1] 会转成 [1=>1]，舍掉小数部分
				case IS_DOUBLE: {
					// 双精度转成 整数
					zend_long lval = zend_dval_to_lval(Z_DVAL_P(key));
					// 转换float型会产生error, 把这种情况留到运行时处理
					/* Incompatible float will generate an error, leave this to run-time */
					// 如果 key 和 lval 不兼容
					if (!zend_is_long_compatible(Z_DVAL_P(key), lval)) {
						// 销毁 value
						zval_ptr_dtor_nogc(value);
						// 销毁 返回结果
						zval_ptr_dtor(result);
						return 0;
					}
					// 更新返回结果哈希表
					zend_hash_index_update(Z_ARRVAL_P(result), lval, value);
					break;
				}
				// 如果键名是 ，转成 0
				case IS_FALSE:
					// 更新返回结果哈希表
					zend_hash_index_update(Z_ARRVAL_P(result), 0, value);
					break;
				// 如果键名是 true，转成 1
				case IS_TRUE:
					zend_hash_index_update(Z_ARRVAL_P(result), 1, value);
					break;
				// 如果键名是 null，转成空 ""
				case IS_NULL:
					zend_hash_update(Z_ARRVAL_P(result), ZSTR_EMPTY_ALLOC(), value);
					break;
				// 其他情况，报错，键名非法
				default:
					zend_error_noreturn(E_COMPILE_ERROR, "Illegal offset type");
					break;
			}
		// 给返回结果插入元素，如果失败
		} else if (!zend_hash_next_index_insert(Z_ARRVAL_P(result), value)) {
			// 销毁值
			zval_ptr_dtor_nogc(value);
			// 销毁返回结果 
			zval_ptr_dtor(result);
			return 0;
		}
	}

	return 1;
}
/* }}} */

// ing3, 编译元操作。针对一些特殊情况作了优化。内部引用1
static void zend_compile_binary_op(znode *result, zend_ast *ast) /* {{{ */
{
	// 左侧表达式语句
	zend_ast *left_ast = ast->child[0];
	// 右侧表达式语句
	zend_ast *right_ast = ast->child[1];
	// 操作符
	uint32_t opcode = ast->attr;

	// 左侧运算结果，右侧运算结果
	znode left_node, right_node;

	// 编译表达式语句, p1:返回结果，p2:表达式语句
	zend_compile_expr(&left_node, left_ast);
	// 编译表达式语句, p1:返回结果，p2:表达式语句
	zend_compile_expr(&right_node, right_ast);

	// 两侧结果是常量
	if (left_node.op_type == IS_CONST && right_node.op_type == IS_CONST) {
		// 进行运算 opcode 是运算符
		if (zend_try_ct_eval_binary_op(&result->u.constant, opcode,
				&left_node.u.constant, &right_node.u.constant)
		) {
			// 结果类型也是常量
			result->op_type = IS_CONST;
			// 销毁临时变量
			zval_ptr_dtor(&left_node.u.constant);
			zval_ptr_dtor(&right_node.u.constant);
			return;
		}
	}

	// 只运行一次，为什么要写do呢？因为要break;
	do {
		// 验证相等
		if (opcode == ZEND_IS_EQUAL || opcode == ZEND_IS_NOT_EQUAL) {
			// 左侧是常量
			if (left_node.op_type == IS_CONST) {
				// 左侧是 FALSE
				if (Z_TYPE(left_node.u.constant) == IS_FALSE) {
					// 操作码 false != $a ? ZEND_BOOL : ZEND_BOOL_NOT
					opcode = (opcode == ZEND_IS_NOT_EQUAL) ? ZEND_BOOL : ZEND_BOOL_NOT;
					zend_emit_op_tmp(result, opcode, &right_node, NULL);
					break;
				// 左侧是 TRUE
				} else if (Z_TYPE(left_node.u.constant) == IS_TRUE) {
					// 操作码 true == $a ? ZEND_BOOL : ZEND_BOOL_NOT
					opcode = (opcode == ZEND_IS_EQUAL) ? ZEND_BOOL : ZEND_BOOL_NOT;
					zend_emit_op_tmp(result, opcode, &right_node, NULL);
					break;
				}
			// 右侧是常量，逻辑同上
			} else if (right_node.op_type == IS_CONST) {
				if (Z_TYPE(right_node.u.constant) == IS_FALSE) {
					opcode = (opcode == ZEND_IS_NOT_EQUAL) ? ZEND_BOOL : ZEND_BOOL_NOT;
					zend_emit_op_tmp(result, opcode, &left_node, NULL);
					break;
				} else if (Z_TYPE(right_node.u.constant) == IS_TRUE) {
					opcode = (opcode == ZEND_IS_EQUAL) ? ZEND_BOOL : ZEND_BOOL_NOT;
					zend_emit_op_tmp(result, opcode, &left_node, NULL);
					break;
				}
			}
		// 验证相同（绝对等于）
		} else if (opcode == ZEND_IS_IDENTICAL || opcode == ZEND_IS_NOT_IDENTICAL) {
			// 如果有一侧是常量, $x === null 转换成 is_null($x), 例如 ZEND_TYPE_CHECK 操作码 ， 包含 IS_NULL, IS_FALSE, and IS_TRUE 
			/* convert $x === null to is_null($x) (i.e. ZEND_TYPE_CHECK opcode). Do the same thing for false/true. (covers IS_NULL, IS_FALSE, and IS_TRUE) */
			// 左侧是常量
			if (left_node.op_type == IS_CONST) {
				// IS_NULL IS_FALSE IS_TRUE
				if (Z_TYPE(left_node.u.constant) <= IS_TRUE && Z_TYPE(left_node.u.constant) >= IS_NULL) {
					// 创建操作码 ZEND_TYPE_CHECK
					zend_op *opline = zend_emit_op_tmp(result, ZEND_TYPE_CHECK, &right_node, NULL);
					// 附加值，类型
					opline->extended_value =
						// 如果是检查相同
						(opcode == ZEND_IS_IDENTICAL) ?
							// 取得类型
							(1 << Z_TYPE(left_node.u.constant)) :
							// #define MAY_BE_ANY (MAY_BE_NULL|MAY_BE_FALSE|MAY_BE_TRUE|MAY_BE_LONG|MAY_BE_DOUBLE|MAY_BE_STRING|MAY_BE_ARRAY|MAY_BE_OBJECT|MAY_BE_RESOURCE)
							// 取得其他所有类型
							(MAY_BE_ANY - (1 << Z_TYPE(left_node.u.constant)));
					return;
				}
			// 否则，如果右侧是常量，逻辑同上。
			} else if (right_node.op_type == IS_CONST) {
				if (Z_TYPE(right_node.u.constant) <= IS_TRUE && Z_TYPE(right_node.u.constant) >= IS_NULL) {
					zend_op *opline = zend_emit_op_tmp(result, ZEND_TYPE_CHECK, &left_node, NULL);
					opline->extended_value =
						(opcode == ZEND_IS_IDENTICAL) ?
							(1 << Z_TYPE(right_node.u.constant)) :
							(MAY_BE_ANY - (1 << Z_TYPE(right_node.u.constant)));
					return;
				}
			}
		// 连接操作
		} else if (opcode == ZEND_CONCAT) {
			// 编译时把 常量转成 string
			/* convert constant operands to strings at compile-time */
			// 左侧是常量
			if (left_node.op_type == IS_CONST) {
				// 如果左侧是 array
				if (Z_TYPE(left_node.u.constant) == IS_ARRAY) {
					// 创建转换操作码：ZEND_CAST，返回结果类型是 string
					zend_emit_op_tmp(&left_node, ZEND_CAST, &left_node, NULL)->extended_value = IS_STRING;
				} else {
					// 左侧不是 array，直接转换类型
					convert_to_string(&left_node.u.constant);
				}
			}
			// 右侧，逻辑同上
			if (right_node.op_type == IS_CONST) {
				if (Z_TYPE(right_node.u.constant) == IS_ARRAY) {
					zend_emit_op_tmp(&right_node, ZEND_CAST, &right_node, NULL)->extended_value = IS_STRING;
				} else {
					convert_to_string(&right_node.u.constant);
				}
			}
			// 如果两侧类型都是常量
			if (left_node.op_type == IS_CONST && right_node.op_type == IS_CONST) {
				// 操作码设置成：快速连接
				opcode = ZEND_FAST_CONCAT;
			}
		}
		// 创建操作码
		zend_emit_op_tmp(result, opcode, &left_node, &right_node);
	} while (0);
}
/* }}} */

// 为了保证从左到右的运算顺序，不使用 zend_compile_binary_op
/* We do not use zend_compile_binary_op for this because we want to retain the left-to-right
 * evaluation order. */
// ing3, >, >= 运算
static void zend_compile_greater(znode *result, zend_ast *ast) /* {{{ */
{
	// 两个子元素
	zend_ast *left_ast = ast->child[0];
	zend_ast *right_ast = ast->child[1];
	// 两个运算结果
	znode left_node, right_node;

	// 必须是 > 或 >=
	ZEND_ASSERT(ast->kind == ZEND_AST_GREATER || ast->kind == ZEND_AST_GREATER_EQUAL);

	// 编译左右两个公式
	// 编译表达式语句, p1:返回结果，p2:表达式语句
	zend_compile_expr(&left_node, left_ast);
	// 编译表达式语句, p1:返回结果，p2:表达式语句
	zend_compile_expr(&right_node, right_ast);

	// 如果两结果都是常量
	if (left_node.op_type == IS_CONST && right_node.op_type == IS_CONST) {
		result->op_type = IS_CONST;
		// 计算 >,>=
		zend_ct_eval_greater(&result->u.constant, ast->kind,
			&left_node.u.constant, &right_node.u.constant);
		// 销毁两个表达式的值
		zval_ptr_dtor(&left_node.u.constant);
		zval_ptr_dtor(&right_node.u.constant);
		return;
	}
	// 创建临时操作码 ZEND_IS_SMALLER 或 ZEND_IS_SMALLER_OR_EQUAL
	zend_emit_op_tmp(result,
		ast->kind == ZEND_AST_GREATER ? ZEND_IS_SMALLER : ZEND_IS_SMALLER_OR_EQUAL,
		&right_node, &left_node);
}
/* }}} */

// ing3, ！~ 两个操作符
static void zend_compile_unary_op(znode *result, zend_ast *ast) /* {{{ */
{
	zend_ast *expr_ast = ast->child[0];
	// attr在语法解析时已赋值
	uint32_t opcode = ast->attr;

	znode expr_node;
	// 编译表达式语句, p1:返回结果，p2:表达式语句
	zend_compile_expr(&expr_node, expr_ast);

	// znode 类型是常量 
	if (expr_node.op_type == IS_CONST
			// ！~ 两个操作符 如果直接进行运算 成功
			&& zend_try_ct_eval_unary_op(&result->u.constant, opcode, &expr_node.u.constant)) {
		// 结果类型是常量
		result->op_type = IS_CONST;
		// 销毁表达式常量
		zval_ptr_dtor(&expr_node.u.constant);
		return;
	}

	// 创建临时操作码
	zend_emit_op_tmp(result, opcode, &expr_node, NULL);
}
/* }}} */

// ing4, 正负号，一元操作符 +，- 
static void zend_compile_unary_pm(znode *result, zend_ast *ast) /* {{{ */
{
	// 表达式
	zend_ast *expr_ast = ast->child[0];
	znode expr_node, right_node;

	// 必须是 正号 或 负号
	ZEND_ASSERT(ast->kind == ZEND_AST_UNARY_PLUS || ast->kind == ZEND_AST_UNARY_MINUS);

	// 编译表达式语句, p1:返回结果，p2:表达式语句
	zend_compile_expr(&expr_node, expr_ast);

	// 如果结果是常量
	if (expr_node.op_type == IS_CONST
		// 并且 进行乘法运算 成功
		&& zend_try_ct_eval_unary_pm(&result->u.constant, ast->kind, &expr_node.u.constant)) {
		// 结果标记成常量， 销毁表达式中的常量
		result->op_type = IS_CONST;
		zval_ptr_dtor(&expr_node.u.constant);
		return;
	}
	// 结果不是常量或 无法直接进行乘法运算
	
	// 被乘数，类型是常量
	right_node.op_type = IS_CONST;
	// +号 乘1 -号 乘-1
	ZVAL_LONG(&right_node.u.constant, (ast->kind == ZEND_AST_UNARY_PLUS) ? 1 : -1);
	// 创建临时操作码 ZEND_MUL
	zend_emit_op_tmp(result, ZEND_MUL, &expr_node, &right_node);
}
/* }}} */

// ing3, "and" "or" 逻辑操作，相关语句有 and or isset()，zend_compile_expr_inner 一处操作
// 这个和短路有毛关系吗？为什么叫 short_circuiting？
static void zend_compile_short_circuiting(znode *result, zend_ast *ast) /* {{{ */
{
	// 左侧子句
	zend_ast *left_ast = ast->child[0];
	// 右侧子句
	zend_ast *right_ast = ast->child[1];

	znode left_node, right_node;
	zend_op *opline_jmpz, *opline_bool;
	uint32_t opnum_jmpz;

	// 语句类型必须是 and 或 or
	ZEND_ASSERT(ast->kind == ZEND_AST_AND || ast->kind == ZEND_AST_OR);

	// 编译左侧表达式
	zend_compile_expr(&left_node, left_ast);

	// 如果左侧结果是常量
	if (left_node.op_type == IS_CONST) {
		// 如果(是 and 运算 并且 左侧不是 true) 或（是 or 运算 并且 左侧结果是 true) ，不需要计算右侧表达式了
		if ((ast->kind == ZEND_AST_AND && !zend_is_true(&left_node.u.constant))
		 || (ast->kind == ZEND_AST_OR && zend_is_true(&left_node.u.constant))) {
			// 运算结果为常量
			result->op_type = IS_CONST;
			// 值与左侧表达式相同
			ZVAL_BOOL(&result->u.constant, zend_is_true(&left_node.u.constant));
		// 否则
		} else {
			// 编译右侧表达式
			zend_compile_expr(&right_node, right_ast);

			// 如果左右侧结果都是常量
			if (right_node.op_type == IS_CONST) {
				// 结果类型是常量
				result->op_type = IS_CONST;
				// 值与右侧值相同
				ZVAL_BOOL(&result->u.constant, zend_is_true(&right_node.u.constant));
				// 销毁右侧结果常量
				zval_ptr_dtor(&right_node.u.constant);
			// 如果右侧结果不是常量
			} else {
				// 创建操作码 ZEND_BOOL，右侧结果的布尔运算
				zend_emit_op_tmp(result, ZEND_BOOL, &right_node, NULL);
			}
		}
		// 销毁左侧结果的常量
		zval_ptr_dtor(&left_node.u.constant);
		return;
	}
	// 如果左侧结果不是常量
	// 当前上下文中的操作码数量
	opnum_jmpz = get_next_op_number();
	
	// 创建操作码 and： ZEND_JMPZ_EX ， or: ZEND_JMPNZ_EX
	opline_jmpz = zend_emit_op(NULL, ast->kind == ZEND_AST_AND ? ZEND_JMPZ_EX : ZEND_JMPNZ_EX,
		&left_node, NULL);

	// 如果左侧结果为 临时变量
	if (left_node.op_type == IS_TMP_VAR) {
		//  ？
		SET_NODE(opline_jmpz->result, &left_node);
		GET_NODE(result, opline_jmpz->result);
	} else {
		// 操作码结果类型 为临时变量
		zend_make_tmp_result(result, opline_jmpz);
	}

	// 编译右侧表达式
	zend_compile_expr(&right_node, right_ast);

	// 创建操作码 ZEND_BOOL，进行布尔型操作
	opline_bool = zend_emit_op(NULL, ZEND_BOOL, &right_node, NULL);
	// 
	SET_NODE(opline_bool->result, result);
	// 更新指定跳转型操作码，跳转到下一个创建的新操作码 。
	zend_update_jump_target_to_next(opnum_jmpz);
}
/* }}} */

// ing4, 编译变量后面的++或--
static void zend_compile_post_incdec(znode *result, zend_ast *ast) /* {{{ */
{
	zend_ast *var_ast = ast->child[0];
	ZEND_ASSERT(ast->kind == ZEND_AST_POST_INC || ast->kind == ZEND_AST_POST_DEC);
	// 确定变量可写
	zend_ensure_writable_variable(var_ast);

	// 如果是类属性
	if (var_ast->kind == ZEND_AST_PROP || var_ast->kind == ZEND_AST_NULLSAFE_PROP) {
		// 编译：更新类属性
		zend_op *opline = zend_compile_prop(NULL, var_ast, BP_VAR_RW, 0);
		// 确定是 加 或 减
		opline->opcode = ast->kind == ZEND_AST_POST_INC ? ZEND_POST_INC_OBJ : ZEND_POST_DEC_OBJ;
		// 操作码结果类型 为临时变量
		zend_make_tmp_result(result, opline);
	// 如果是静态属性
	} else if (var_ast->kind == ZEND_AST_STATIC_PROP) {
		// 编译：更新类静态属性
		zend_op *opline = zend_compile_static_prop(NULL, var_ast, BP_VAR_RW, 0, 0);
		// 确定是 加 或 减
		opline->opcode = ast->kind == ZEND_AST_POST_INC ? ZEND_POST_INC_STATIC_PROP : ZEND_POST_DEC_STATIC_PROP;
		// 操作码结果类型 为临时变量
		zend_make_tmp_result(result, opline);
	// 其他情况
	} else {
		znode var_node;
		// 编译变量，p1:返回值，p2:语句，p3:操作类型，p4:是否引用传递
		zend_op *opline = zend_compile_var(&var_node, var_ast, BP_VAR_RW, 0);
		// 如果操作码类型是数组赋值
		if (opline && opline->opcode == ZEND_FETCH_DIM_RW) {
			// 添加扩展信息 
			opline->extended_value = ZEND_FETCH_DIM_INCDEC;
		}
		// 创建操作码 ZEND_PRE_INC 或 ZEND_PRE_DEC
		zend_emit_op_tmp(result, ast->kind == ZEND_AST_POST_INC ? ZEND_POST_INC : ZEND_POST_DEC,
			&var_node, NULL);
	}
}
/* }}} */

// ing4, 编译变量前面的++或--
static void zend_compile_pre_incdec(znode *result, zend_ast *ast) /* {{{ */
{
	zend_ast *var_ast = ast->child[0];
	// 语句类型是 ++ 或 --
	ZEND_ASSERT(ast->kind == ZEND_AST_PRE_INC || ast->kind == ZEND_AST_PRE_DEC);
	// 确定变量可写
	zend_ensure_writable_variable(var_ast);

	// 如果是类属性
	if (var_ast->kind == ZEND_AST_PROP || var_ast->kind == ZEND_AST_NULLSAFE_PROP) {
		// 编译：更新类属性
		zend_op *opline = zend_compile_prop(result, var_ast, BP_VAR_RW, 0);
		// 确定是 加 或 减
		opline->opcode = ast->kind == ZEND_AST_PRE_INC ? ZEND_PRE_INC_OBJ : ZEND_PRE_DEC_OBJ;
		// 类型是临时变量
		opline->result_type = IS_TMP_VAR;
		result->op_type = IS_TMP_VAR;
	// 如果是类的静态属性
	} else if (var_ast->kind == ZEND_AST_STATIC_PROP) {
		// 编译：更新类静态属性
		zend_op *opline = zend_compile_static_prop(result, var_ast, BP_VAR_RW, 0, 0);
		// 确定是 加 或 减
		opline->opcode = ast->kind == ZEND_AST_PRE_INC ? ZEND_PRE_INC_STATIC_PROP : ZEND_PRE_DEC_STATIC_PROP;
		// 临时变量
		opline->result_type = IS_TMP_VAR;
		result->op_type = IS_TMP_VAR;
	// 其他情况
	} else {
		znode var_node;
		// 编译变量，p1:返回值，p2:语句，p3:操作类型，p4:是否引用传递
		zend_op *opline = zend_compile_var(&var_node, var_ast, BP_VAR_RW, 0);
		// 如果操作码类型是数组赋值
		if (opline && opline->opcode == ZEND_FETCH_DIM_RW) {
			// 添加扩展信息 
			opline->extended_value = ZEND_FETCH_DIM_INCDEC;
		}
		// 创建操作码 ZEND_PRE_INC 或 ZEND_PRE_DEC
		zend_emit_op_tmp(result, ast->kind == ZEND_AST_PRE_INC ? ZEND_PRE_INC : ZEND_PRE_DEC,
			&var_node, NULL);
	}
}
/* }}} */

// ing4, 编译 强制转换类型 (int) (double) (string) (array) (object) (bool) (unset) $a;
static void zend_compile_cast(znode *result, zend_ast *ast) /* {{{ */
{
	// 被转换的表达式
	zend_ast *expr_ast = ast->child[0];
	znode expr_node;
	zend_op *opline;

	// 编译表达式语句, p1:返回结果，p2:表达式语句
	zend_compile_expr(&expr_node, expr_ast);

	// 如果要转成bool
	if (ast->attr == _IS_BOOL) {
		// 创建临时操作码
		opline = zend_emit_op_tmp(result, ZEND_BOOL, &expr_node, NULL);
	// 如果要转成null, (unset)， 报错，这个方法已弃用
	} else if (ast->attr == IS_NULL) {
		zend_error(E_COMPILE_ERROR, "The (unset) cast is no longer supported");
	// 其他类型
	} else {
		// 创建对应类型的操作码
		opline = zend_emit_op_tmp(result, ZEND_CAST, &expr_node, NULL);
		// 操作码里写入类型
		opline->extended_value = ast->attr;
	}
}
/* }}} */

// ing3, 没有真值的三元操作符, 一次调用
static void zend_compile_shorthand_conditional(znode *result, zend_ast *ast) /* {{{ */
{
	// 条件
	zend_ast *cond_ast = ast->child[0];
	// 假值
	zend_ast *false_ast = ast->child[2];

	znode cond_node, false_node;
	zend_op *opline_qm_assign;
	uint32_t opnum_jmp_set;

	// 真值必须是null
	ZEND_ASSERT(ast->child[1] == NULL);

	// 编译条件表达式
	zend_compile_expr(&cond_node, cond_ast);

	// 最新操作码行号：当前上下文中的操作码数量
	opnum_jmp_set = get_next_op_number();
	// 创建临时操作码 ZEND_JMP_SET
	zend_emit_op_tmp(result, ZEND_JMP_SET, &cond_node, NULL);
	// 编译 假值
	zend_compile_expr(&false_node, false_ast);
	// 创建临时操作码 ZEND_QM_ASSIGN ?
	opline_qm_assign = zend_emit_op_tmp(NULL, ZEND_QM_ASSIGN, &false_node, NULL);
	// 把 ZEND_JMP_SET 操作码 的返回值传给 ZEND_QM_ASSIGN 操作码
	SET_NODE(opline_qm_assign->result, result);
	// 更新指定跳转型操作码，跳转到下一个创建的新操作码 。
	zend_update_jump_target_to_next(opnum_jmp_set);
}
/* }}} */

// ing3, 编译三元操作符
static void zend_compile_conditional(znode *result, zend_ast *ast) /* {{{ */
{
	// 条件
	zend_ast *cond_ast = ast->child[0];
	// 真值
	zend_ast *true_ast = ast->child[1];
	// 假值
	zend_ast *false_ast = ast->child[2];

	znode cond_node, true_node, false_node;
	zend_op *opline_qm_assign2;
	uint32_t opnum_jmpz, opnum_jmp;

	// 如果条件语句是没加括号的三元操作符
	if (cond_ast->kind == ZEND_AST_CONDITIONAL
			//  加括号的标记
			&& cond_ast->attr != ZEND_PARENTHESIZED_CONDITIONAL) {
		// 如果，作为条件的三元操作符有真值。当前操作符有歧义，报错
		if (cond_ast->child[1]) {
			// 如果当前三元操作符也有真值
			if (true_ast) {
				zend_error(E_COMPILE_ERROR,
					"Unparenthesized `a ? b : c ? d : e` is not supported. "
					"Use either `(a ? b : c) ? d : e` or `a ? b : (c ? d : e)`");
			// 如果当前没有真值
			} else {
				zend_error(E_COMPILE_ERROR,
					"Unparenthesized `a ? b : c ?: d` is not supported. "
					"Use either `(a ? b : c) ?: d` or `a ? b : (c ?: d)`");
			}
		// 如果，作为条件的三元操作符没有真值
		} else {
			// 如果当前三元操作符有真值，有歧义 ，报错
			if (true_ast) {
				zend_error(E_COMPILE_ERROR,
					"Unparenthesized `a ?: b ? c : d` is not supported. "
					"Use either `(a ?: b) ? c : d` or `a ?: (b ? c : d)`");
			// 如果当前三元操作符没有真值，不报错
			} else {
				// 因为 (a ?: b) ?: c 与 a ?: (b ?: c) 总是相等
				/* This case is harmless:  (a ?: b) ?: c always produces the same result
				 * as a ?: (b ?: c). */
			}
		}
	}

	// 如果当前三元操作符没有真值
	if (!true_ast) {
		// 编译简写三元表达式
		zend_compile_shorthand_conditional(result, ast);
		return;
	}

	// 编译条件表达式
	zend_compile_expr(&cond_node, cond_ast);

	// 创建 按条件(znode) 跳转 操作码，返回新操作码顺序号
	opnum_jmpz = zend_emit_cond_jump(ZEND_JMPZ, &cond_node, 0);

	// 编译真值语句
	zend_compile_expr(&true_node, true_ast);

	// 创建操作码 ZEND_QM_ASSIGN
	zend_emit_op_tmp(result, ZEND_QM_ASSIGN, &true_node, NULL);

	// 创建 ZEND_JMP 操作码，跳转到指定编号的操作码
	opnum_jmp = zend_emit_jump(0);
	// 更新指定跳转型操作码，跳转到下一个创建的新操作码 。
	zend_update_jump_target_to_next(opnum_jmpz);

	// 编译表达式
	zend_compile_expr(&false_node, false_ast);

	// 创建操作码 ZEND_QM_ASSIGN
	opline_qm_assign2 = zend_emit_op(NULL, ZEND_QM_ASSIGN, &false_node, NULL);
	// 
	SET_NODE(opline_qm_assign2->result, result);
	// 更新指定跳转型操作码，跳转到下一个创建的新操作码 。
	zend_update_jump_target_to_next(opnum_jmp);
}
/* }}} */

// ing3, "??" 连接的两个表达式，第二个是默认值
static void zend_compile_coalesce(znode *result, zend_ast *ast) /* {{{ */
{
	// 表达式
	zend_ast *expr_ast = ast->child[0];
	// 默认值
	zend_ast *default_ast = ast->child[1];

	znode expr_node, default_node;
	zend_op *opline;
	uint32_t opnum;

	// 编译变量，p1:返回值，p2:语句，p3:操作类型，p4:是否引用传递
	zend_compile_var(&expr_node, expr_ast, BP_VAR_IS, 0);
	// 当前上下文中的操作码数量
	opnum = get_next_op_number();
	// 创建操作码 ZEND_COALESCE
	zend_emit_op_tmp(result, ZEND_COALESCE, &expr_node, NULL);

	// 编译默认值表达式
	zend_compile_expr(&default_node, default_ast);

	// 创建临时操作码 ZEND_QM_ASSIGN
	opline = zend_emit_op_tmp(NULL, ZEND_QM_ASSIGN, &default_node, NULL);
	//
	SET_NODE(opline->result, result);

	opline = &CG(active_op_array)->opcodes[opnum];
	// 当前上下文中的操作码数量
	opline->op2.opline_num = get_next_op_number();
}
/* }}} */

// ing3, 销毁 znode
static void znode_dtor(zval *zv) {
	znode *node = Z_PTR_P(zv);
	// 如果类型是常量，用 zval_ptr_dtor_nogc 方法销毁目标常量
	if (node->op_type == IS_CONST) {
		zval_ptr_dtor_nogc(&node->u.constant);
	}
	// 然后释放节点内存
	efree(node);
}

// ing3, 编译 ??= 操作符（添加默认值操作）
static void zend_compile_assign_coalesce(znode *result, zend_ast *ast) /* {{{ */
{
	// 接收变量
	zend_ast *var_ast = ast->child[0];
	// 默认值
	zend_ast *default_ast = ast->child[1];

	znode var_node_is, var_node_w, default_node, assign_node, *node;
	zend_op *opline;
	uint32_t coalesce_opnum;
	bool need_frees = 0;

	/* Remember expressions compiled during the initial BP_VAR_IS lookup,
	 * to avoid double-evaluation when we compile again with BP_VAR_W. */
	HashTable *orig_memoized_exprs = CG(memoized_exprs);
	int orig_memoize_mode = CG(memoize_mode);
	// 确定变量可写
	zend_ensure_writable_variable(var_ast);
	// 接收变量不可以是 $this
	if (is_this_fetch(var_ast)) {
		zend_error_noreturn(E_COMPILE_ERROR, "Cannot re-assign $this");
	}

	// 创建并初始化 记忆表达式表
	ALLOC_HASHTABLE(CG(memoized_exprs));
	zend_hash_init(CG(memoized_exprs), 0, NULL, znode_dtor, 0);

	// 切换内存模式： ZEND_MEMOIZE_COMPILE
	CG(memoize_mode) = ZEND_MEMOIZE_COMPILE;
	// 编译变量，p1:返回值，p2:语句，p3:操作类型，p4:是否引用传递
	zend_compile_var(&var_node_is, var_ast, BP_VAR_IS, 0);
	// 当前上下文中的操作码数量
	coalesce_opnum = get_next_op_number();
	// 创建操作码 ZEND_COALESCE
	zend_emit_op_tmp(result, ZEND_COALESCE, &var_node_is, NULL);

	// 切换内存模式： ZEND_MEMOIZE_NONE
	CG(memoize_mode) = ZEND_MEMOIZE_NONE;
	// 如果接收变量是数组元素
	if (var_ast->kind == ZEND_AST_DIM) {
		// 编译可能给自己的子元素赋值的表达式
		zend_compile_expr_with_potential_assign_to_self(&default_node, default_ast, var_ast);
	// 其他情况
	} else {
		// 编译表达式语句, p1:返回结果，p2:表达式语句
		zend_compile_expr(&default_node, default_ast);
	}

	// 切换内存模式： ZEND_MEMOIZE_FETCH
	CG(memoize_mode) = ZEND_MEMOIZE_FETCH;
	// 编译变量，p1:返回值，p2:语句，p3:操作类型，p4:是否引用传递
	zend_compile_var(&var_node_w, var_ast, BP_VAR_W, 0);

	// 这里重新执行一些 zend_compile_assign() 里面的操作码业务逻辑
	/* Reproduce some of the zend_compile_assign() opcode fixup logic here. */
	// 取得最后一个操作码
	opline = &CG(active_op_array)->opcodes[CG(active_op_array)->last-1];
	// $GLOBALS['x'] 像变量赋值一样处理
	/* Treat $GLOBALS['x'] assignment like assignment to variable. */
	// 如果是全局变量 类型是 ZEND_AST_VAR， 否则用变量自带类型
	zend_ast_kind kind = is_global_var_fetch(var_ast) ? ZEND_AST_VAR : var_ast->kind;
	// 按接收变量的类型处理
	switch (kind) {
		// 如果是简单变量
		case ZEND_AST_VAR:
			// 创建操作码 ZEND_ASSIGN
			zend_emit_op_tmp(&assign_node, ZEND_ASSIGN, &var_node_w, &default_node);
			break;
		// 如果是类的静态属性
		case ZEND_AST_STATIC_PROP:
			// 操作码类型 ZEND_ASSIGN_STATIC_PROP
			opline->opcode = ZEND_ASSIGN_STATIC_PROP;
			// 结果类型是临时变量
			opline->result_type = IS_TMP_VAR;
			var_node_w.op_type = IS_TMP_VAR;
			// 创建 ZEND_OP_DATA 操作码
			zend_emit_op_data(&default_node);
			// 
			assign_node = var_node_w;
			break;
		// 如果是数组元素
		case ZEND_AST_DIM:
			// 操作码类型 ZEND_ASSIGN_DIM
			opline->opcode = ZEND_ASSIGN_DIM;
			// 结果为临时变量
			opline->result_type = IS_TMP_VAR;
			var_node_w.op_type = IS_TMP_VAR;
			// 创建 ZEND_OP_DATA 操作码
			zend_emit_op_data(&default_node);
			assign_node = var_node_w;
			break;
		// 如果是对象属性
		case ZEND_AST_PROP:
		case ZEND_AST_NULLSAFE_PROP:
			// 操作码类型是 ZEND_ASSIGN_OBJ
			opline->opcode = ZEND_ASSIGN_OBJ;
			opline->result_type = IS_TMP_VAR;
			var_node_w.op_type = IS_TMP_VAR;
			// 创建 ZEND_OP_DATA 操作码
			zend_emit_op_data(&default_node);
			assign_node = var_node_w;
			break;
		// 其他情况
		EMPTY_SWITCH_DEFAULT_CASE();
	}

	// 创建临时操作码 ZEND_QM_ASSIGN
	opline = zend_emit_op_tmp(NULL, ZEND_QM_ASSIGN, &assign_node, NULL);
	SET_NODE(opline->result, result);

	// 遍历记忆中的表达式
	ZEND_HASH_FOREACH_PTR(CG(memoized_exprs), node) {
		// 如果类型是变量或临时变量
		if (node->op_type == IS_TMP_VAR || node->op_type == IS_VAR) {
			// 标记：需要释放放记忆表达式
			need_frees = 1;
			break;
		}
	} ZEND_HASH_FOREACH_END();

	// 如果有重复的表达式，释放重复的表达式
	/* Free DUPed expressions if there are any */
	if (need_frees) {
		// 创建 ZEND_JMP 操作码，跳转到指定编号的操作码
		uint32_t jump_opnum = zend_emit_jump(0);
		// 更新指定跳转型操作码，跳转到下一个创建的新操作码 。
		zend_update_jump_target_to_next(coalesce_opnum);
		ZEND_HASH_FOREACH_PTR(CG(memoized_exprs), node) {
			if (node->op_type == IS_TMP_VAR || node->op_type == IS_VAR) {
				zend_emit_op(NULL, ZEND_FREE, node, NULL);
			}
		} ZEND_HASH_FOREACH_END();
		// 更新指定跳转型操作码，跳转到下一个创建的新操作码 。
		zend_update_jump_target_to_next(jump_opnum);
	} else {
		// 更新指定跳转型操作码，跳转到下一个创建的新操作码 。
		zend_update_jump_target_to_next(coalesce_opnum);
	}

	// 销毁记忆表达式哈希表
	zend_hash_destroy(CG(memoized_exprs));
	// 释放内存
	FREE_HASHTABLE(CG(memoized_exprs));
	// 表达式模式切换回原来的模式
	CG(memoized_exprs) = orig_memoized_exprs;
	// 内存模式恢复到原模式
	CG(memoize_mode) = orig_memoize_mode;
}
/* }}} */

// ing3, 编译 print 语句
static void zend_compile_print(znode *result, zend_ast *ast) /* {{{ */
{
	zend_op *opline;
	// 第一个子语句是表达式
	zend_ast *expr_ast = ast->child[0];

	znode expr_node;
	// 编译表达式语句, p1:返回结果，p2:表达式语句
	zend_compile_expr(&expr_node, expr_ast);

	// 创建 ZEND_ECHO 操作码
	opline = zend_emit_op(NULL, ZEND_ECHO, &expr_node, NULL);
	opline->extended_value = 1;

	// 结果类型设置成常量
	result->op_type = IS_CONST;
	// 常量标记设置成 1。
	ZVAL_LONG(&result->u.constant, 1);
}
/* }}} */

// ing2, exit 语句
static void zend_compile_exit(znode *result, zend_ast *ast) /* {{{ */
{
	zend_ast *expr_ast = ast->child[0];
	znode expr_node;

	// 如果有表达式，编译表达式
	if (expr_ast) {
		// 编译表达式语句, p1:返回结果，p2:表达式语句
		zend_compile_expr(&expr_node, expr_ast);
	// 否则结果标记成未使用
	} else {
		expr_node.op_type = IS_UNUSED;
	}
	// 创建操作码 ZEND_EXIT
	zend_op *opline = zend_emit_op(NULL, ZEND_EXIT, &expr_node, NULL);
	// 如果有结果对象
	if (result) {
		// ？ 标记成 表达式抛出 
		/* Mark this as an "expression throw" for opcache. */
		opline->extended_value = ZEND_THROW_IS_EXPR;
		// 结果类型常量，值为 true
		result->op_type = IS_CONST;
		ZVAL_TRUE(&result->u.constant);
	}
}
/* }}} */

// ing3, 编译 yield 语句：yield; yield 1+1; yield 1=>2;
static void zend_compile_yield(znode *result, zend_ast *ast) /* {{{ */
{
	// 值语句
	zend_ast *value_ast = ast->child[0];
	// key语句
	zend_ast *key_ast = ast->child[1];

	znode value_node, key_node;
	znode *value_node_ptr = NULL, *key_node_ptr = NULL;
	zend_op *opline;
	// 是否返回引用地址
	bool returns_by_ref = (CG(active_op_array)->fn_flags & ZEND_ACC_RETURN_REFERENCE) != 0;
	// 把 function 标记生成生器
	zend_mark_function_as_generator();

	// 如果有key语句
	if (key_ast) {
		// 编译表达式语句, p1:返回结果，p2:表达式语句
		zend_compile_expr(&key_node, key_ast);
		// 
		key_node_ptr = &key_node;
	}

	// 如果有值语句
	if (value_ast) {
		// 如果是变量：php变量，数组元素，对象属性，允空对象属性，静态属性
		if (returns_by_ref && zend_is_variable(value_ast)) {
			// 编译变量，p1:返回值，p2:语句，p3:操作类型，p4:是否引用传递
			zend_compile_var(&value_node, value_ast, BP_VAR_W, 1);
		// 否则编译表达式
		} else {
			// 编译表达式语句, p1:返回结果，p2:表达式语句
			zend_compile_expr(&value_node, value_ast);
		}
		// 
		value_node_ptr = &value_node;
	}

	// 创建操作码 ZEND_YIELD
	opline = zend_emit_op(result, ZEND_YIELD, value_node_ptr, key_node_ptr);

	// 如果有值语句，并且要求引用返回，并且值语句是 函数调用语句
	if (value_ast && returns_by_ref && zend_is_call(value_ast)) {
		// 添加标记 ZEND_RETURNS_FUNCTION
		opline->extended_value = ZEND_RETURNS_FUNCTION;
	}
}
/* }}} */

// ing3, 编译 yield from 语句
static void zend_compile_yield_from(znode *result, zend_ast *ast) /* {{{ */
{
	// 表达式
	zend_ast *expr_ast = ast->child[0];
	znode expr_node;
	// 把 function 标记生成生器
	zend_mark_function_as_generator();
	// 如果要求返回引用地址
	if (CG(active_op_array)->fn_flags & ZEND_ACC_RETURN_REFERENCE) {
		// 报错：不可以在返回地址的方法用使用 yield from，测试过（ function &a(){ yield from $a=1;} ）
		zend_error_noreturn(E_COMPILE_ERROR,
			"Cannot use \"yield from\" inside a by-reference generator");
	}

	// 编译表达式语句, p1:返回结果，p2:表达式语句
	zend_compile_expr(&expr_node, expr_ast);
	// 创建临时操作码 ZEND_YIELD_FROM
	zend_emit_op_tmp(result, ZEND_YIELD_FROM, &expr_node, NULL);
}
/* }}} */

// ing3, 编译 instanceof
static void zend_compile_instanceof(znode *result, zend_ast *ast) /* {{{ */
{
	// 第一个子语句是对象，第二个是类名
	zend_ast *obj_ast = ast->child[0];
	zend_ast *class_ast = ast->child[1];

	znode obj_node, class_node;
	zend_op *opline;
	// 编译表达式语句, p1:返回结果，p2:表达式语句// 编译对象语句
	zend_compile_expr(&obj_node, obj_ast);
	// 如果编译结果是常量。 常量不属于任何类型
	if (obj_node.op_type == IS_CONST) {
		// 删除编译结果
		zend_do_free(&obj_node);
		// 返回类型为常量，值为 False
		result->op_type = IS_CONST;
		// 值为 false
		ZVAL_FALSE(&result->u.constant);
		return;
	}
	// 编译结果不是常量

	// 编译类引用，添加3个标记 
	zend_compile_class_ref(&class_node, class_ast,
		// ？
		ZEND_FETCH_CLASS_NO_AUTOLOAD | ZEND_FETCH_CLASS_EXCEPTION | ZEND_FETCH_CLASS_SILENT);

	// 创建临时操作码 ZEND_INSTANCEOF
	opline = zend_emit_op_tmp(result, ZEND_INSTANCEOF, &obj_node, NULL);

	// 如果返回结果类型是常量
	if (class_node.op_type == IS_CONST) {
		// 第二个操作对象类型为常量
		opline->op2_type = IS_CONST;
		// 值为 类名
		opline->op2.constant = zend_add_class_name_literal(
			Z_STR(class_node.u.constant));
		// 给 CG(active_op_array) 添加缓存大小，添加 一个 void*指针的大小，返回添加后的缓存大小
		opline->extended_value = zend_alloc_cache_slot();
	} else {
		// 
		SET_NODE(opline->op2, &class_node);
	}
}
/* }}} */

// ing3, 编译 include 或 eval 
static void zend_compile_include_or_eval(znode *result, zend_ast *ast) /* {{{ */
{
	// 表达式
	zend_ast *expr_ast = ast->child[0];
	znode expr_node;
	zend_op *opline;
	// 创建 ZEND_EXT_FCALL_BEGIN 操作码
	zend_do_extended_fcall_begin();
	// 编译表达式语句, p1:返回结果，p2:表达式语句
	zend_compile_expr(&expr_node, expr_ast);

	// 用编译结果，创建 ZEND_INCLUDE_OR_EVAL 操作码
	opline = zend_emit_op(result, ZEND_INCLUDE_OR_EVAL, &expr_node, NULL);
	// 
	opline->extended_value = ast->attr;
	
	// 创建 ZEND_EXT_FCALL_END 操作码
	zend_do_extended_fcall_end();
}
/* }}} */

// ing4, 编译 isset 和 empty, 创建操作码检查目标变量
static void zend_compile_isset_or_empty(znode *result, zend_ast *ast) /* {{{ */
{
	// 被检查的变量
	zend_ast *var_ast = ast->child[0];

	znode var_node;
	zend_op *opline = NULL;

	// 语句必须是 isset 或 empty
	ZEND_ASSERT(ast->kind == ZEND_AST_ISSET || ast->kind == ZEND_AST_EMPTY);

	// 是否是变量：php变量，数组元素，对象属性，允空对象属性，静态属性
	if (!zend_is_variable(var_ast)) {
		// 如果正在调用 empty方法
		if (ast->kind == ZEND_AST_EMPTY) {
			// 创建一个 否定语句(!$a)，把参数放进去
			/* empty(expr) can be transformed to !expr */
			zend_ast *not_ast = zend_ast_create_ex(ZEND_AST_UNARY_OP, ZEND_BOOL_NOT, var_ast);
			// 编译表达式语句, p1:返回结果，p2:表达式语句
			zend_compile_expr(result, not_ast);
			return;
		// 如果正在调用isset方法，报错，不可以用表达式当参数（测试过）
		} else {
			zend_error_noreturn(E_COMPILE_ERROR,
				"Cannot use isset() on the result of an expression "
				"(you can use \"null !== expression\" instead)");
		}
	}

	// 如果是 $GLOBALS 变量
	if (is_globals_fetch(var_ast)) {
		// 结果类型为常量
		result->op_type = IS_CONST;
		// $GLOBALS一直存在
		ZVAL_BOOL(&result->u.constant, ast->kind == ZEND_AST_ISSET);
		return;
	}

	// 如果是 $GLOBALS[];
	if (is_global_var_fetch(var_ast)) {
		// $GLOBALS[] 没写键名，报错（测试过 isset($GLOBALS[]);）
		if (!var_ast->child[1]) {
			zend_error_noreturn(E_COMPILE_ERROR, "Cannot use [] for reading");
		}
		// 编译键名表达式
		zend_compile_expr(&var_node, var_ast->child[1]);
		// 如果键名是常量
		if (var_node.op_type == IS_CONST) {
			// 键名转成字串
			convert_to_string(&var_node.u.constant);
		}
		// 创建 ZEND_ISSET_ISEMPTY_VAR 操作码
		opline = zend_emit_op_tmp(result, ZEND_ISSET_ISEMPTY_VAR, &var_node, NULL);
		// 操作码附加信息
		opline->extended_value =
			ZEND_FETCH_GLOBAL | (ast->kind == ZEND_AST_EMPTY ? ZEND_ISEMPTY : 0);
		return;
	}
	// 如果是可以短路的类型， 添加短路标记
	zend_short_circuiting_mark_inner(var_ast);
	// 根据类型进行不同操作
	switch (var_ast->kind) {
		// 简单变量
		case ZEND_AST_VAR:
			// $this->
			if (is_this_fetch(var_ast)) {
				// 检查$this, 创建操作码 ZEND_ISSET_ISEMPTY_THIS
				opline = zend_emit_op(result, ZEND_ISSET_ISEMPTY_THIS, NULL, NULL);
				// 给操作码列表添加标记 ZEND_ACC_USES_THIS
				CG(active_op_array)->fn_flags |= ZEND_ACC_USES_THIS;
			// 如果编译变量成功
			} else if (zend_try_compile_cv(&var_node, var_ast) == SUCCESS) {
				// 检查编译变量，创建操作码 ZEND_ISSET_ISEMPTY_CV
				opline = zend_emit_op(result, ZEND_ISSET_ISEMPTY_CV, &var_node, NULL);
			// 其他情况
			} else {
				// 编译简单变量，不使用CV
				opline = zend_compile_simple_var_no_cv(result, var_ast, BP_VAR_IS, 0);
				// 其他简单变量，创建操作码 ZEND_ISSET_ISEMPTY_VAR
				opline->opcode = ZEND_ISSET_ISEMPTY_VAR;
			}
			break;
		// $a[1], 获取数组元素
		case ZEND_AST_DIM:
			opline = zend_compile_dim(result, var_ast, BP_VAR_IS, /* by_ref */ false);
			// 检查数组元素，创建操作码 ZEND_ISSET_ISEMPTY_DIM_OBJ
			opline->opcode = ZEND_ISSET_ISEMPTY_DIM_OBJ;
			break;
		// 获取对象属性
		case ZEND_AST_PROP:
		case ZEND_AST_NULLSAFE_PROP:
			opline = zend_compile_prop(result, var_ast, BP_VAR_IS, 0);
			// 检查对象属性，创建操作码  ZEND_ISSET_ISEMPTY_PROP_OBJ
			opline->opcode = ZEND_ISSET_ISEMPTY_PROP_OBJ;
			break;
		// 获取对象静态属性
		case ZEND_AST_STATIC_PROP:
			// 编译静态属性
			opline = zend_compile_static_prop(result, var_ast, BP_VAR_IS, 0, 0);
			// 检查对象静态属性，创建操作码  ZEND_ISSET_ISEMPTY_STATIC_PROP
			opline->opcode = ZEND_ISSET_ISEMPTY_STATIC_PROP;
			break;
		EMPTY_SWITCH_DEFAULT_CASE()
	}
	// 类型：临时变量
	result->op_type = opline->result_type = IS_TMP_VAR;
	// 如果不是 isset
	if (!(ast->kind == ZEND_AST_ISSET)) {
		// 操作码添加 ZEND_ISEMPTY 标记
		opline->extended_value |= ZEND_ISEMPTY;
	}
}
/* }}} */

// ing2, 编译静默符号
static void zend_compile_silence(znode *result, zend_ast *ast) /* {{{ */
{
	// 第一个语句是被静默的php语句
	zend_ast *expr_ast = ast->child[0];
	znode silence_node;
	// 创建操作码 ：开始静默
	zend_emit_op_tmp(&silence_node, ZEND_BEGIN_SILENCE, NULL, NULL);

	// 如果是变量赋值语句（包括全局变量，callable）
	if (expr_ast->kind == ZEND_AST_VAR) {
		// @$var 赋值语句需要强制 fetch, 否则 对CV的访问会掉到静默外面 ？
		/* For @$var we need to force a FETCH instruction, otherwise the CV access will
		 * happen outside the silenced section. */
		// 编译简单变量，不使用CV
		zend_compile_simple_var_no_cv(result, expr_ast, BP_VAR_R, 0 );
	} else {
		// 编译表达式语句, p1:返回结果，p2:表达式语句
		zend_compile_expr(result, expr_ast);
	}
	// 创建操作码：终止静默
	zend_emit_op(NULL, ZEND_END_SILENCE, &silence_node, NULL);
}
/* }}} */

// ing3, 编译 shell_exec 执行外部命令
static void zend_compile_shell_exec(znode *result, zend_ast *ast) /* {{{ */
{
	// 表达试
	zend_ast *expr_ast = ast->child[0];

	zval fn_name;
	// 名称语句，参数列表语句，调用语句
	zend_ast *name_ast, *args_ast, *call_ast;

	// 函数名固定用 shell_exec
	ZVAL_STRING(&fn_name, "shell_exec");
	// 名称语句
	name_ast = zend_ast_create_zval(&fn_name);
	// 参数列表语句，传入的第一个参数
	args_ast = zend_ast_create_list(1, ZEND_AST_ARG_LIST, expr_ast);
	// 创建表达式语句，其实就是 shell_exec( 参数 )
	call_ast = zend_ast_create(ZEND_AST_CALL, name_ast, args_ast);
	
	// 编译表达式，结果传给 result
	zend_compile_expr(result, call_ast);

	// 销毁函数名
	zval_ptr_dtor(&fn_name);
}
/* }}} */

// ing3, 编译数组
static void zend_compile_array(znode *result, zend_ast *ast) /* {{{ */
{
	// 取得语句列表
	zend_ast_list *list = zend_ast_get_list(ast);
	zend_op *opline;
	uint32_t i, opnum_init = -1;
	// 默认是顺序数组
	bool packed = 1;

	// 尝试计算 array, 如果成功，直接返回
	if (zend_try_ct_eval_array(&result->u.constant, ast)) {
		// 结果类型是常量
		result->op_type = IS_CONST;
		return;
	}

	// 必须不能为空 ，在编译时处理
	/* Empty arrays are handled at compile-time */
	ZEND_ASSERT(list->children > 0);

	// 编译所有键值对
	for (i = 0; i < list->children; ++i) {
		zend_ast *elem_ast = list->child[i];
		zend_ast *value_ast, *key_ast;
		bool by_ref;
		znode value_node, key_node, *key_node_ptr = NULL;
		// 如果 键值对是null, 报错（如何测试？）
		if (elem_ast == NULL) {
			zend_error(E_COMPILE_ERROR, "Cannot use empty array elements in arrays");
		}

		// 值语句
		value_ast = elem_ast->child[0];

		// 如果键值对需要解包
		if (elem_ast->kind == ZEND_AST_UNPACK) {
			// 编译表达式语句, p1:返回结果，p2:表达式语句
			zend_compile_expr(&value_node, value_ast);
			// 如果是第一个元素
			if (i == 0) {
				// 当前上下文中的操作码数量
				opnum_init = get_next_op_number();
				// 创建操作码 ZEND_INIT_ARRAY
				opline = zend_emit_op_tmp(result, ZEND_INIT_ARRAY, NULL, NULL);
			}
			// 创建操作码 ZEND_ADD_ARRAY_UNPACK
			opline = zend_emit_op(NULL, ZEND_ADD_ARRAY_UNPACK, &value_node, NULL);
			// 
			SET_NODE(opline->result, result);
			// 下一个键值对
			continue;
		}

		// 键名语句
		key_ast = elem_ast->child[1];
		// 引用地址标记
		by_ref = elem_ast->attr;

		// 如果有key语句
		if (key_ast) {
			// 编译key语句
			zend_compile_expr(&key_node, key_ast);
			// 把数字字串的数组元素名转成整数 （测试过 $a = ["1"=>1];var_dump($a[1]);）
			zend_handle_numeric_op(&key_node);
			key_node_ptr = &key_node;
		}
		// 如果是传址
		if (by_ref) {
			// 确定变量可写
			zend_ensure_writable_variable(value_ast);
			// 编译变量，p1:返回值，p2:语句，p3:操作类型，p4:是否引用传递// 编译变量 
			zend_compile_var(&value_node, value_ast, BP_VAR_W, 1);
		// 否则 
		} else {
			// 编译表达式语句, p1:返回结果，p2:表达式语句
			zend_compile_expr(&value_node, value_ast);
		}

		// 如果是第一个元素
		if (i == 0) {
			// 当前上下文中的操作码数量
			opnum_init = get_next_op_number();
			// 创建操作码 ZEND_INIT_ARRAY
			opline = zend_emit_op_tmp(result, ZEND_INIT_ARRAY, &value_node, key_node_ptr);
			// 添加扩展信息 元素数 << 2
			opline->extended_value = list->children << ZEND_ARRAY_SIZE_SHIFT;
		// 不是第一个元素
		} else {
			// 创建操作码 ZEND_ADD_ARRAY_ELEMENT
			opline = zend_emit_op(NULL, ZEND_ADD_ARRAY_ELEMENT,
				&value_node, key_node_ptr);
			// 
			SET_NODE(opline->result, result);
		}
		// 操作码添加 传址标记
		opline->extended_value |= by_ref;

		// 如果有key 并且key是常量，并且是string 型
		if (key_ast && key_node.op_type == IS_CONST && Z_TYPE(key_node.u.constant) == IS_STRING) {
			// 不是顺序数组
			packed = 0;
		}
	}

	/* Add a flag to INIT_ARRAY if we know this array cannot be packed */
	// 如果不是顺序数组
	if (!packed) {
		// ?
		ZEND_ASSERT(opnum_init != (uint32_t)-1);
		// 取得初始化操作码
		opline = &CG(active_op_array)->opcodes[opnum_init];
		// 标记成非顺序数组
		opline->extended_value |= ZEND_ARRAY_NOT_PACKED;
	}
}
/* }}} */

// ing3, 编译常量, __COMPILER_HALT_OFFSET__ 干嘛用的？
static void zend_compile_const(znode *result, zend_ast *ast) /* {{{ */
{
	// 常量名
	zend_ast *name_ast = ast->child[0];

	zend_op *opline;

	bool is_fully_qualified;
	// 字串常量名
	zend_string *orig_name = zend_ast_get_str(name_ast);
	// 完整的常量名
	zend_string *resolved_name = zend_resolve_const_name(orig_name, name_ast->attr, &is_fully_qualified);
	// 如果完整常量名是 __COMPILER_HALT_OFFSET__，（未测试过） 
	// 或 名称语句带有 ZEND_NAME_RELATIVE 标记（第一节是namespace) 并且 源名字是 __COMPILER_HALT_OFFSET__
	if (zend_string_equals_literal(resolved_name, "__COMPILER_HALT_OFFSET__") || (name_ast->attr != ZEND_NAME_RELATIVE && zend_string_equals_literal(orig_name, "__COMPILER_HALT_OFFSET__"))) {
		// 取得最后一个语句
		zend_ast *last = CG(ast);

		// 如果类型是语句列表
		while (last && last->kind == ZEND_AST_STMT_LIST) {
			zend_ast_list *list = zend_ast_get_list(last);
			if (list->children == 0) {
				break;
			}
			// 取得列表中最后一个语句
			last = list->child[list->children-1];
		}
		// 如果最后一个语句的类型是 ZEND_AST_HALT_COMPILER，（语句是T_HALT_COMPILER），直接处理
		if (last && last->kind == ZEND_AST_HALT_COMPILER) {
			// 结果类型标记成常量
			result->op_type = IS_CONST;
			// 取回的变量是行号，在语法解析时通过 zend_get_scanned_file_offset() 获取到的
			ZVAL_LONG(&result->u.constant, Z_LVAL_P(zend_ast_get_zval(last->child[0])));
			// 清空 resolved_name
			zend_string_release_ex(resolved_name, 0);
			return;
		}
	}

	// 尝试计算常量的值，如果成功
	if (zend_try_ct_eval_const(&result->u.constant, resolved_name, is_fully_qualified)) {
		// 结果类型是常量
		result->op_type = IS_CONST;
		// 释放常量名
		zend_string_release_ex(resolved_name, 0);
		return;
	}

	// 添加操作码 resolved_name
	opline = zend_emit_op_tmp(result, ZEND_FETCH_CONSTANT, NULL, NULL);
	// 第二个操作对象的类型是常量
	opline->op2_type = IS_CONST;

	// 如果是完整路径， 或者 当前不在命名空间里
	if (is_fully_qualified || !FC(current_namespace)) {
		// 第一个操作对象里清空标记
		opline->op1.num = 0;
		// 第二个操作对象的常量放常量名
		// 把常量名添加到 当前 CG(active_op_array)-> literals 里面, 0表示相对路径
		opline->op2.constant = zend_add_const_name_literal(
			resolved_name, 0);
	// 否则 
	} else {
		// 第一个操作对象里标记： 常量是 命名空间中的相对路径
		opline->op1.num = IS_CONSTANT_UNQUALIFIED_IN_NAMESPACE;
		// 第二个操作对象的常量放常量名
		// 把常量名添加到 当前 CG(active_op_array)-> literals 里面, 1表示绝对路径
		opline->op2.constant = zend_add_const_name_literal(
			resolved_name, 1);
	}
	// 给 CG(active_op_array) 添加缓存大小，添加 一个 void*指针的大小，返回添加后的缓存大小
	opline->extended_value = zend_alloc_cache_slot();
}
/* }}} */

// ing3, 编译类常量
static void zend_compile_class_const(znode *result, zend_ast *ast) /* {{{ */
{
	zend_ast *class_ast;
	zend_ast *const_ast;
	znode class_node, const_node;
	zend_op *opline;

	// 先计算 类名 和 常量名
	zend_eval_const_expr(&ast->child[0]);
	zend_eval_const_expr(&ast->child[1]);
	// 类名语句
	class_ast = ast->child[0];
	// 常量名语句
	const_ast = ast->child[1];

	// 如果类名是内置变量
	if (class_ast->kind == ZEND_AST_ZVAL) {
		zend_string *resolved_name;
		// 通过语句获取完整常量名
		resolved_name = zend_resolve_class_name_ast(class_ast);
		// 如果常量语句是内置变量 并且 成功计算并返回（包含权限控制）
		if (const_ast->kind == ZEND_AST_ZVAL && zend_try_ct_eval_class_const(&result->u.constant, resolved_name, zend_ast_get_str(const_ast))) {
			// 结果类型是常量
			result->op_type = IS_CONST;
			// 释放 常量名
			zend_string_release_ex(resolved_name, 0);
			return;
		}
		// 释放 常量名
		zend_string_release_ex(resolved_name, 0);
	}

	// 编译类引用
	zend_compile_class_ref(&class_node, class_ast, ZEND_FETCH_CLASS_EXCEPTION);

	// 编译表达式语句, p1:返回结果，p2:表达式语句
	zend_compile_expr(&const_node, const_ast);

	// 创建操作码
	opline = zend_emit_op_tmp(result, ZEND_FETCH_CLASS_CONSTANT, NULL, &const_node);

	// 把类名放到 opline->op1 里
	zend_set_class_name_op1(opline, &class_node);

	// 给 CG(active_op_array) 添加缓存大小，添加 count 个void*指针的大小, 返回缓存大小
	opline->extended_value = zend_alloc_cache_slots(2);
}
/* }}} */

// ing2, 编译类名
static void zend_compile_class_name(znode *result, zend_ast *ast) /* {{{ */
{
	zend_ast *class_ast = ast->child[0];
	// 计算并返回类名 成功
	if (zend_try_compile_const_expr_resolve_class_name(&result->u.constant, class_ast)) {
		// 结果标记成常量
		result->op_type = IS_CONST;
		return;
	}

	// 如果类型是内置变量
	if (class_ast->kind == ZEND_AST_ZVAL) {
		// 创建操作码 ZEND_FETCH_CLASS_NAME
		zend_op *opline = zend_emit_op_tmp(result, ZEND_FETCH_CLASS_NAME, NULL, NULL);
		// 通过类名获取类的引用方式，self,parent,static 或其他(ZEND_FETCH_CLASS_DEFAULT)
		opline->op1.num = zend_get_class_fetch_type(zend_ast_get_str(class_ast));
	// 类型不是内置变量
	} else {
		znode expr_node;
		// 编译类名语句
		zend_compile_expr(&expr_node, class_ast);
		// ？ 如果编译结果是常量 怎么知道是 ::class
		// 这里应该是用了 static 其它情况前面排除了 ？
		if (expr_node.op_type == IS_CONST) {
			// 不同于 class_ast 是多层常量时的情况
			/* Unlikely case that happen if class_ast is constant folded.
			// 在这里处理掉，避免在 VM 中 创建特殊常量
			 * Handle it here, to avoid needing a CONST specialization in the VM. */
			zend_error_noreturn(E_COMPILE_ERROR, "Cannot use \"::class\" on value of type %s",
				zend_zval_type_name(&expr_node.u.constant));
		}
		// 创建操作码 ZEND_FETCH_CLASS_NAME
		zend_emit_op_tmp(result, ZEND_FETCH_CLASS_NAME, &expr_node, NULL);
	}
}
/* }}} */

// ing3, 添加引号中的变量，p1:操作码，p2:操作码结果，p3:变量序号，p4:元素节点
static zend_op *zend_compile_rope_add_ex(zend_op *opline, znode *result, uint32_t num, znode *elem_node) /* {{{ */
{
	// 如果编号是0
	if (num == 0) {
		// 返回类型为 临时变量
		result->op_type = IS_TMP_VAR;
		// 编号 -1
		result->u.op.var = -1;
		// 操作码：初始化 ROPE
		opline->opcode = ZEND_ROPE_INIT;
	// 编号不是0	
	} else {
		// 操作码：添加 ROPE
		opline->opcode = ZEND_ROPE_ADD;
		// 返回结果绑定到操作码 第一个运算对象上
		SET_NODE(opline->op1, result);
	}
	// 传入元素作为第二个 运算对象
	SET_NODE(opline->op2, elem_node);
	// 返回结果绑定到操作码 上
	SET_NODE(opline->result, result);
	// 操作码扩展信息里保存 变量编号
	opline->extended_value = num;
	// 返回操作码
	return opline;
}
/* }}} */

// ing3, 创建操作码，并添加引号中的变量，p1:操作码结果，p2:变量序号，p3:元素节点
static zend_op *zend_compile_rope_add(znode *result, uint32_t num, znode *elem_node) /* {{{ */
{
	// 创建新操作码
	zend_op *opline = get_next_op();

	// 数量是0，初始化
	if (num == 0) {
		// 运算对象类型 临时变量
		result->op_type = IS_TMP_VAR;
		// 位置序号 -1
		result->u.op.var = -1;
		// 操作码 ZEND_ROPE_INIT
		opline->opcode = ZEND_ROPE_INIT;
	// 数量不是0
	} else {
		// 操作码 ZEND_ROPE_ADD
		opline->opcode = ZEND_ROPE_ADD;
		// 
		SET_NODE(opline->op1, result);
	}
	SET_NODE(opline->op2, elem_node);
	SET_NODE(opline->result, result);
	opline->extended_value = num;
	return opline;
}
/* }}} */

// 150行
// ing2, 编译引号里的变量
static void zend_compile_encaps_list(znode *result, zend_ast *ast) /* {{{ */
{
	uint32_t i, j;
	uint32_t rope_init_lineno = -1;
	zend_op *opline = NULL, *init_opline;
	znode elem_node, last_const_node;
	// 取得语句列表。然后就没有用到 ast 了
	zend_ast_list *list = zend_ast_get_list(ast);
	// 保留操作码数量
	uint32_t reserved_op_number = -1;
	// 最少有一个子语句
	ZEND_ASSERT(list->children > 0);

	j = 0;
	// 最后一个常量节点，标记成未使用
	last_const_node.op_type = IS_UNUSED;
	// 遍历所有子语句
	for (i = 0; i < list->children; i++) {
		// 变量
		zend_ast *encaps_var = list->child[i];

		// 如果变量格式是 ${abc} 或 ${abc}[123] 或 ${1+1}
		// 经测试，这两个限制没有发生。可能是版本不一致。
		if (encaps_var->attr & (ZEND_ENCAPS_VAR_DOLLAR_CURLY|ZEND_ENCAPS_VAR_DOLLAR_CURLY_VAR_VAR)) {
			// （如果类型是php变量或者 数组元素变量 ）并且 （格式是 ${abc} 或 ${abc}[123]）
			if ((encaps_var->kind == ZEND_AST_VAR || encaps_var->kind == ZEND_AST_DIM) && (encaps_var->attr & ZEND_ENCAPS_VAR_DOLLAR_CURLY)) {
				// 报错
				zend_error(E_DEPRECATED, "Using ${var} in strings is deprecated, use {$var} instead");
			// 如果类型是php变量 并且 格式是 ${1+1}
			} else if (encaps_var->kind == ZEND_AST_VAR && (encaps_var->attr & ZEND_ENCAPS_VAR_DOLLAR_CURLY_VAR_VAR)) {
				// 报错：此语法已弃用，推荐使用新语法
				zend_error(E_DEPRECATED, "Using ${expr} (variable variables) in strings is deprecated, use {${expr}} instead");
			}
		}

		// 编译表达式语句, p1:返回结果，p2:表达式语句
		zend_compile_expr(&elem_node, encaps_var);

		// 如果结果类型是常量
		if (elem_node.op_type == IS_CONST) {
			// 结果转成字串
			convert_to_string(&elem_node.u.constant);

			// 如果长度是0 
			if (Z_STRLEN(elem_node.u.constant) == 0) {
				// 销毁内置变量
				zval_ptr_dtor(&elem_node.u.constant);
			//  如果最后一个常量节点类型是常量
			} else if (last_const_node.op_type == IS_CONST) {
				// 连接后面这几个变量, zend_operators.c
				concat_function(&last_const_node.u.constant, &last_const_node.u.constant, &elem_node.u.constant);
				// 销毁没有引用次数的对象，有引用次数的添加到gc回收周期里。并没有销毁 zval本身。
				zval_ptr_dtor(&elem_node.u.constant);
			// 其他情况
			} else {
				// 最后一个常量节点类型 更新成 常量
				last_const_node.op_type = IS_CONST;
				// 把常量值复制过来
				ZVAL_COPY_VALUE(&last_const_node.u.constant, &elem_node.u.constant);
				// 给 ZEND_ROPE_ADD 操作码预留一个位置
				/* Reserve place for ZEND_ROPE_ADD instruction */
				// 取得一个预留位置
				reserved_op_number = get_next_op_number();
				// 再取得一个新位置
				opline = get_next_op();
				// 操作码 ZEND_NOP
				opline->opcode = ZEND_NOP;
			}
			// 下一个
			continue;
		// 结果类型不是常量，这里用到 j，也会改变j
		} else {
			// 如果是第一个
			if (j == 0) {
				// 如果 node类型是 常量
				if (last_const_node.op_type == IS_CONST) {
					// 初始化行号 是保存的操作码行号
					rope_init_lineno = reserved_op_number;
				// 类型不是常量
				} else {
					// 当前上下文中的操作码数量
					rope_init_lineno = get_next_op_number();
				}
			}
			// 如果 node类型是 常量
			if (last_const_node.op_type == IS_CONST) {
				// 取出保存的操作码
				opline = &CG(active_op_array)->opcodes[reserved_op_number];
				// 添加变量。下面还要再 zend_compile_rope_add
				zend_compile_rope_add_ex(opline, result, j++, &last_const_node);
				// node类型是未使用
				last_const_node.op_type = IS_UNUSED;
			}
			//  创建操作码，并添加引号中的变量，p1:操作码结果，p2:变量序号，p3:元素节点
			opline = zend_compile_rope_add(result, j++, &elem_node);
		}
	}
	// 遍历完所有变量

	// 如果一个 rope也没添加，所有变量名都解析成常量了
	if (j == 0) {
		// 如果类型是常量
		result->op_type = IS_CONST;
		// 如果node类型是常量
		if (last_const_node.op_type == IS_CONST) {
			// node里的常量复制给结果
			ZVAL_COPY_VALUE(&result->u.constant, &last_const_node.u.constant);
		// node类型不是常量
		} else {
			// 结果为空字串
			ZVAL_EMPTY_STRING(&result->u.constant);
			/* empty string */
		}
		// 临时变量数量 = 保存操作码数 -1
		CG(active_op_array)->last = reserved_op_number - 1;
		// 完毕
		return;
	// 如果 node类型是常量
	} else if (last_const_node.op_type == IS_CONST) {
		// 取出保存的操作码
		opline = &CG(active_op_array)->opcodes[reserved_op_number];
		// 添加引号中的变量，p1:操作码，p2:操作码结果，p3:变量序号，p4:元素节点
		opline = zend_compile_rope_add_ex(opline, result, j++, &last_const_node);
	}
	// 取出初始化操作码
	init_opline = CG(active_op_array)->opcodes + rope_init_lineno;
	// 如果有一个变量
	if (j == 1) {
		// 如果第二个运算对象是常量
		if (opline->op2_type == IS_CONST) {
			// 把它当成结果
			GET_NODE(result, opline->op2);
			// 操作码为 ZEND_NOP
			MAKE_NOP(opline);
		// 如果不是
		} else {
			// 操作码 ZEND_CAST
			opline->opcode = ZEND_CAST;
			// 扩展信息： 字串型
			opline->extended_value = IS_STRING;
			// 第一个运算对象类型，从第二个复制
			opline->op1_type = opline->op2_type;
			// 第一个运算对象数据，从第二个复制
			opline->op1 = opline->op2;
			// 第二个清空
			SET_UNUSED(opline->op2);
			// 操作码结果类型 为临时变量
			zend_make_tmp_result(result, opline);
		}
	// 如果有二个变量
	} else if (j == 2) {
		// 操作码 ZEND_FAST_CONCAT
		opline->opcode = ZEND_FAST_CONCAT;
		// 无扩展信息
		opline->extended_value = 0;
		// 第一个运算对象类型，从第二个复制
		opline->op1_type = init_opline->op2_type;
		// 第一个运算对象数据，从第二个复制
		opline->op1 = init_opline->op2;
		// 操作码结果类型 为临时变量
		zend_make_tmp_result(result, opline);
		// 清空初始化操作码
		MAKE_NOP(init_opline);
	// 其他情况
	} else {
		uint32_t var;
		// 数量放在初始化操作码里
		init_opline->extended_value = j;
		// 操作码 ZEND_ROPE_END，添加变量结束
		opline->opcode = ZEND_ROPE_END;
		// 操作码结果类型 为临时变量
		zend_make_tmp_result(result, opline);
		// 增加变量计数
		var = opline->op1.var = get_temporary_variable();

		// 分配必要数量的 zval位置来存放 rope
		/* Allocates the necessary number of zval slots to keep the rope */
		// 需要的大小
		i = ((j * sizeof(zend_string*)) + (sizeof(zval) - 1)) / sizeof(zval);
		// 创建i个临时变量
		while (i > 1) {
			// 增加变量计数
			get_temporary_variable();
			// 下一个
			i--;
		}

		// 更新使用同一个变量的所有前面的操作码
		/* Update all the previous opcodes to use the same variable */
		// 遍历：从当前操作码到初化操作码这一段
		while (opline != init_opline) {
			// 前一个操作码
			opline--;
			// 如果是 ZEND_ROPE_ADD 并且结果变量里编号是 -1
			if (opline->opcode == ZEND_ROPE_ADD &&
			    opline->result.var == (uint32_t)-1) {
				// 更新操作对象1的变量编号
				opline->op1.var = var;
				// 更新结果的变量编号
				opline->result.var = var;
			// 如果是 ZEND_ROPE_INIT 并且结果变量里编号是 -1
			} else if (opline->opcode == ZEND_ROPE_INIT &&
			           opline->result.var == (uint32_t)-1) {
				// 更新结果的变量编号
				opline->result.var = var;
			}
		}
	}
}
/* }}} */

// ing2, 编译魔术常量
static void zend_compile_magic_const(znode *result, zend_ast *ast) /* {{{ */
{
	zend_op *opline;
	// 执行语句获取魔术常量结果
	if (zend_try_ct_eval_magic_const(&result->u.constant, ast)) {
		result->op_type = IS_CONST;
		return;
	}
	// 如果计算失败
	// 必须是 '__CLASS__' 必须要在 Trait 里
	ZEND_ASSERT(ast->attr == T_CLASS_C &&
	            CG(active_class_entry) &&
	            (CG(active_class_entry)->ce_flags & ZEND_ACC_TRAIT) != 0);

	// 创建操作码 ： ZEND_FETCH_CLASS_NAME
	opline = zend_emit_op_tmp(result, ZEND_FETCH_CLASS_NAME, NULL, NULL);
	opline->op1.num = ZEND_FETCH_CLASS_SELF;
}
/* }}} */

// ing3, 常量表达式中 允许的语句类型
static bool zend_is_allowed_in_const_expr(zend_ast_kind kind) /* {{{ */
{
	return kind == ZEND_AST_ZVAL || kind == ZEND_AST_BINARY_OP
		|| kind == ZEND_AST_GREATER || kind == ZEND_AST_GREATER_EQUAL
		|| kind == ZEND_AST_AND || kind == ZEND_AST_OR
		|| kind == ZEND_AST_UNARY_OP
		|| kind == ZEND_AST_UNARY_PLUS || kind == ZEND_AST_UNARY_MINUS
		|| kind == ZEND_AST_CONDITIONAL || kind == ZEND_AST_DIM
		|| kind == ZEND_AST_ARRAY || kind == ZEND_AST_ARRAY_ELEM
		|| kind == ZEND_AST_UNPACK
		|| kind == ZEND_AST_CONST || kind == ZEND_AST_CLASS_CONST
		|| kind == ZEND_AST_CLASS_NAME
		|| kind == ZEND_AST_MAGIC_CONST || kind == ZEND_AST_COALESCE
		|| kind == ZEND_AST_CONST_ENUM_INIT
		|| kind == ZEND_AST_NEW || kind == ZEND_AST_ARG_LIST
		|| kind == ZEND_AST_NAMED_ARG
		|| kind == ZEND_AST_PROP || kind == ZEND_AST_NULLSAFE_PROP;
}
/* }}} */

// ing3, 编译常量表达式类常量
static void zend_compile_const_expr_class_const(zend_ast **ast_ptr) /* {{{ */
{
	zend_ast *ast = *ast_ptr;
	// 类名语句
	zend_ast *class_ast = ast->child[0];
	zend_string *class_name;
	int fetch_type;

	// 类型必须是内置变量
	if (class_ast->kind != ZEND_AST_ZVAL) {
		// 编译时不允许有动态类名
		zend_error_noreturn(E_COMPILE_ERROR,
			"Dynamic class names are not allowed in compile-time class constant references");
	}

	// 类名
	class_name = zend_ast_get_str(class_ast);
	// 通过类名获取类的引用方式，self,parent,static 或其他(ZEND_FETCH_CLASS_DEFAULT)
	fetch_type = zend_get_class_fetch_type(class_name);

	// static:: 不可以用在编译阶段
	if (ZEND_FETCH_CLASS_STATIC == fetch_type) {
		zend_error_noreturn(E_COMPILE_ERROR,
			"\"static::\" is not allowed in compile-time constants");
	}

	// 普通情况， 非 self,parent,static
	if (ZEND_FETCH_CLASS_DEFAULT == fetch_type) {
		// 通过语句获取类名
		zend_string *tmp = zend_resolve_class_name_ast(class_ast);
		// 
		zend_string_release_ex(class_name, 0);
		// 如果tmp有效
		if (tmp != class_name) {
			// 更新语句中的类名（内置变量）
			zval *zv = zend_ast_get_zval(class_ast);
			ZVAL_STR(zv, tmp);
			// 类名标记成 FULL QUALIFIED
			class_ast->attr = ZEND_NAME_FQ;
		}
	}

	// ? /Zend/zend_vm_execute.h 里面用到
	ast->attr |= ZEND_FETCH_CLASS_EXCEPTION;
}
/* }}} */

// ing3, 编译常量表达式，获取类名，主要是处理 self,parent
static void zend_compile_const_expr_class_name(zend_ast **ast_ptr) /* {{{ */
{
	zend_ast *ast = *ast_ptr;
	zend_ast *class_ast = ast->child[0];
	// 必须是计算好的内置变量
	if (class_ast->kind != ZEND_AST_ZVAL) {
		zend_error_noreturn(E_COMPILE_ERROR,
			"(expression)::class cannot be used in constant expressions");
	}

	zend_string *class_name = zend_ast_get_str(class_ast);
	// 通过类名获取类的引用方式，self,parent,static 或其他(ZEND_FETCH_CLASS_DEFAULT)
	uint32_t fetch_type = zend_get_class_fetch_type(class_name);

	switch (fetch_type) {
		// self 或 parent
		case ZEND_FETCH_CLASS_SELF:
		case ZEND_FETCH_CLASS_PARENT:
			// fetch type 代替 名称
			/* For the const-eval representation store the fetch type instead of the name. */
			zend_string_release(class_name);
			ast->child[0] = NULL;
			ast->attr = fetch_type;
			return;
		// static， 编译时不允许出现 static
		case ZEND_FETCH_CLASS_STATIC:
			zend_error_noreturn(E_COMPILE_ERROR,
				"static::class cannot be used for compile-time class name resolution");
			return;
		EMPTY_SWITCH_DEFAULT_CASE()
	}
}

// ing2, 编译常量
static void zend_compile_const_expr_const(zend_ast **ast_ptr) /* {{{ */
{
	zend_ast *ast = *ast_ptr;
	// 常量名
	zend_ast *name_ast = ast->child[0];
	zend_string *orig_name = zend_ast_get_str(name_ast);
	bool is_fully_qualified;
	zval result;
	zend_string *resolved_name;

	// 取得完整的常量名
	resolved_name = zend_resolve_const_name(
		orig_name, name_ast->attr, &is_fully_qualified);

	// 计算常量值
	if (zend_try_ct_eval_const(&result, resolved_name, is_fully_qualified)) {
		// 销毁名称变量
		zend_string_release_ex(resolved_name, 0);
		// 销毁语句
		zend_ast_destroy(ast);
		// 创建内部变量语句
		*ast_ptr = zend_ast_create_zval(&result);
		return;
	}

	// 销毁语句
	zend_ast_destroy(ast);
	// 创建常量语句 ? 为什么 又创建个语句 
	*ast_ptr = zend_ast_create_constant(resolved_name,
		!is_fully_qualified && FC(current_namespace) ? IS_CONSTANT_UNQUALIFIED_IN_NAMESPACE : 0);
}
/* }}} */

// ing3, 编译魔术常量表达式
static void zend_compile_const_expr_magic_const(zend_ast **ast_ptr) /* {{{ */
{
	zend_ast *ast = *ast_ptr;

	// 其他情况已经处理过了，这里只剩下 T_CLASS_C，也就是 __CLASS__
	/* Other cases already resolved by constant folding */
	ZEND_ASSERT(ast->attr == T_CLASS_C);

	zend_ast_destroy(ast);
	// 创建类名语句 ，创建时会自动赋值成 scope->name
	*ast_ptr = zend_ast_create(ZEND_AST_CONSTANT_CLASS);
}
/* }}} */

// ing3,编译 new 语句中的类名，类名回传给 ast_ptr。主要是把类名更新成完整路径名
static void zend_compile_const_expr_new(zend_ast **ast_ptr)
{
	zend_ast *class_ast = (*ast_ptr)->child[0];
	// 常量表达式中不可以用匿名类 ?
	if (class_ast->kind == ZEND_AST_CLASS) {
		zend_error_noreturn(E_COMPILE_ERROR,
			"Cannot use anonymous class in constant expression");
	}
	// 常量表达式中不可以用动态类名 ?
	if (class_ast->kind != ZEND_AST_ZVAL) {
		zend_error_noreturn(E_COMPILE_ERROR,
			"Cannot use dynamic class name in constant expression");
	}

	// 取得语句中的类名
	zend_string *class_name = zend_resolve_class_name_ast(class_ast);
	
	// 通过类名获取类的引用方式，self,parent,static 或其他(ZEND_FETCH_CLASS_DEFAULT)
	int fetch_type = zend_get_class_fetch_type(class_name);
	
	// 不可以在编译时用 static
	if (ZEND_FETCH_CLASS_STATIC == fetch_type) {
		zend_error_noreturn(E_COMPILE_ERROR,
			"\"static\" is not allowed in compile-time constants");
	}
	
	// 类名
	zval *class_ast_zv = zend_ast_get_zval(class_ast);
	zval_ptr_dtor_nogc(class_ast_zv);
	// 把类名更新成完整路径名
	ZVAL_STR(class_ast_zv, class_name);
	// 添加标记， 进行过类型转换
	class_ast->attr = fetch_type << ZEND_CONST_EXPR_NEW_FETCH_TYPE_SHIFT;
}

// ing3, 编译参数列表常量表达式，主要是处理 positional argument
static void zend_compile_const_expr_args(zend_ast **ast_ptr)
{
	zend_ast_list *list = zend_ast_get_list(*ast_ptr);
	bool uses_named_args = false;
	// 遍历列表中的参数
	for (uint32_t i = 0; i < list->children; i++) {
		zend_ast *arg = list->child[i];
		// 这个地方不可以是打包参数，那打包参数是在哪里处理的呢？
		if (arg->kind == ZEND_AST_UNPACK) {
			zend_error_noreturn(E_COMPILE_ERROR,
				"Argument unpacking in constant expressions is not supported");
		}
		// 带参数名的参数 function a($a=1,$b=2){var_dump($a,$b);} a(b:1);
		if (arg->kind == ZEND_AST_NAMED_ARG) {
			uses_named_args = true;
		// uses_named_args 必须是最后一个参数(测试过)
		} else if (uses_named_args) {
			zend_error_noreturn(E_COMPILE_ERROR,
				"Cannot use positional argument after named argument");
		}
	}
	// 如果使用了 uses_named_args，要添加标记
	if (uses_named_args) {
		list->attr = 1;
	}
}

// 常量表达式上下文
typedef struct {
	/* Whether the value of this expression may differ on each evaluation. */
	bool allow_dynamic;
} const_expr_context;

// ing2, 编译常量表达式
static void zend_compile_const_expr(zend_ast **ast_ptr, void *context) /* {{{ */
{
	// 上下文
	const_expr_context *ctx = (const_expr_context *) context;
	// 语句指针
	zend_ast *ast = *ast_ptr;
	// null 或已经算好的，就不用再算了
	if (ast == NULL || ast->kind == ZEND_AST_ZVAL) {
		return;
	}

	// ing3, 校验常量表达式中 允许的语句类型
	if (!zend_is_allowed_in_const_expr(ast->kind)) {
		zend_error_noreturn(E_COMPILE_ERROR, "Constant expression contains invalid operations");
	}

	// 按语句类型处理
	switch (ast->kind) {
		// 类常量
		case ZEND_AST_CLASS_CONST:
			zend_compile_const_expr_class_const(ast_ptr);
			break;
		// 类名
		case ZEND_AST_CLASS_NAME:
			zend_compile_const_expr_class_name(ast_ptr);
			break;
		// 常量
		case ZEND_AST_CONST:
			zend_compile_const_expr_const(ast_ptr);
			break;
		// 魔术常量
		case ZEND_AST_MAGIC_CONST:
			zend_compile_const_expr_magic_const(ast_ptr);
			break;
		// New 语句
		case ZEND_AST_NEW:
			// 如果不允许动态上下文
			if (!ctx->allow_dynamic) {
				zend_error_noreturn(E_COMPILE_ERROR,
					"New expressions are not supported in this context");
			}
			zend_compile_const_expr_new(ast_ptr);
			break;
		// 参数列表
		case ZEND_AST_ARG_LIST:
			zend_compile_const_expr_args(ast_ptr);
			break;
	}

	// clear, 逐个编译表达式
	// 对 ast 或者 ast_list 的每个子元素调用 fn
	zend_ast_apply(ast, zend_compile_const_expr, context);
}
/* }}} */

// ing4, 常量表达式转成内置变量
void zend_const_expr_to_zval(zval *result, zend_ast **ast_ptr, bool allow_dynamic) /* {{{ */
{
	const_expr_context context;
	// 允许动态
	context.allow_dynamic = allow_dynamic;

	// 计算常量表达式
	zend_eval_const_expr(ast_ptr);
	// 编译常量表达式，结果放在语句里
	zend_compile_const_expr(ast_ptr, &context);
	// 如果语句类型 不是 内置变量
	if ((*ast_ptr)->kind != ZEND_AST_ZVAL) {
		/* Replace with compiled AST zval representation. */
		zval ast_zv;
		// 语句里的内置变量放到 ast_zv 里
		ZVAL_AST(&ast_zv, zend_ast_copy(*ast_ptr));
		// 删除语句
		zend_ast_destroy(*ast_ptr);
		// 创建新 内置变量语句
		*ast_ptr = zend_ast_create_zval(&ast_zv);
	}
	// 复制返回结果
	ZVAL_COPY(result, zend_ast_get_zval(*ast_ptr));
}
/* }}} */

// 和 compile_stmt 一样，不过调用更早
/* Same as compile_stmt, but with early binding */
// ing4, 编译最外层语句， zend_compile_namespace 调用，/Zend/zend_language_scanner.l 调用
void zend_compile_top_stmt(zend_ast *ast) /* {{{ */
{
	if (!ast) {
		return;
	}
	// 语句列表
	if (ast->kind == ZEND_AST_STMT_LIST) {
		zend_ast_list *list = zend_ast_get_list(ast);
		uint32_t i;
		// 遍历语句列表
		for (i = 0; i < list->children; ++i) {
			// 编译每个语句
			zend_compile_top_stmt(list->child[i]);
		}
		return;
	}

	// 函数定义
	if (ast->kind == ZEND_AST_FUNC_DECL) {
		CG(zend_lineno) = ast->lineno;
		// 编译函数定义
		zend_compile_func_decl(NULL, ast, 1);
		CG(zend_lineno) = ((zend_ast_decl *) ast)->end_lineno;
	// 类定义
	} else if (ast->kind == ZEND_AST_CLASS) {
		CG(zend_lineno) = ast->lineno;
		// 编译类定义
		zend_compile_class_decl(NULL, ast, 1);
		CG(zend_lineno) = ((zend_ast_decl *) ast)->end_lineno;
	// 其他语句
	} else {
		// 编译语句
		zend_compile_stmt(ast);
	}
	// 如果不是 定义命名空间和 __halt
	if (ast->kind != ZEND_AST_NAMESPACE && ast->kind != ZEND_AST_HALT_COMPILER) {
		// 验证命名空间是否合法, 不合法会报错
		zend_verify_namespace();
	}
}
/* }}} */

// ing3, 编译语句
static void zend_compile_stmt(zend_ast *ast) /* {{{ */
{
	if (!ast) {
		return;
	}
	// 更新当前行号
	CG(zend_lineno) = ast->lineno;

	// 如果需要编译扩展语句并且
	// 不是未标记的语句：语句块，挑转标签，类属性定义，类常量定义，引用trait，类方法定义
	if ((CG(compiler_options) & ZEND_COMPILE_EXTENDED_STMT) && !zend_is_unticked_stmt(ast)) {
		// 创建操作码： ZEND_EXT_STMT
		zend_do_extended_stmt();
	}
	// 根据类型编译
	switch (ast->kind) {
		// 语句列表
		case ZEND_AST_STMT_LIST:
			zend_compile_stmt_list(ast);
			break;
		// 声明全局变量
		case ZEND_AST_GLOBAL:
			zend_compile_global_var(ast);
			break;
		// 声明静态变量。不是静态成员变量哈！
		case ZEND_AST_STATIC:
			zend_compile_static_var(ast);
			break;
		// Unset 函数
		case ZEND_AST_UNSET:
			zend_compile_unset(ast);
			break;
		// return 语句
		case ZEND_AST_RETURN:
			zend_compile_return(ast);
			break;
		// echo 语句
		case ZEND_AST_ECHO:
			zend_compile_echo(ast);
			break;
		// break，continue 语句
		case ZEND_AST_BREAK:
		case ZEND_AST_CONTINUE:
			zend_compile_break_continue(ast);
			break;
		// goto 语句
		case ZEND_AST_GOTO:
			zend_compile_goto(ast);
			break;
		// 跳转 label
		case ZEND_AST_LABEL:
			zend_compile_label(ast);
			break;
		// while 语句
		case ZEND_AST_WHILE:
			zend_compile_while(ast);
			break;
		// do-wihle 语句
		case ZEND_AST_DO_WHILE:
			zend_compile_do_while(ast);
			break;
		// for 循环语句
		case ZEND_AST_FOR:
			zend_compile_for(ast);
			break;
		// foreach 语句
		case ZEND_AST_FOREACH:
			zend_compile_foreach(ast);
			break;
		// if 语句
		case ZEND_AST_IF:
			zend_compile_if(ast);
			break;
		// switch 语句
		case ZEND_AST_SWITCH:
			zend_compile_switch(ast);
			break;
		// try 语句
		case ZEND_AST_TRY:
			zend_compile_try(ast);
			break;
		// declare 语句
		case ZEND_AST_DECLARE:
			zend_compile_declare(ast);
			break;
		// 函数和类方法的定义
		case ZEND_AST_FUNC_DECL:
		case ZEND_AST_METHOD:
			zend_compile_func_decl(NULL, ast, 0);
			break;
		// enum 里的 case 语句
		case ZEND_AST_ENUM_CASE:
			zend_compile_enum_case(ast);
			break;
		// 一组类属性
		case ZEND_AST_PROP_GROUP:
			zend_compile_prop_group(ast);
			break;
		// 一组类常量
		case ZEND_AST_CLASS_CONST_GROUP:
			zend_compile_class_const_group(ast);
			break;
		// use trait
		case ZEND_AST_USE_TRAIT:
			zend_compile_use_trait(ast);
			break;
		// 类定义
		case ZEND_AST_CLASS:
			zend_compile_class_decl(NULL, ast, 0);
			break;
		// use 一组内容
		case ZEND_AST_GROUP_USE:
			zend_compile_group_use(ast);
			break;
		// 单个 use
		case ZEND_AST_USE:
			zend_compile_use(ast);
			break;
		// 定义常量
		case ZEND_AST_CONST_DECL:
			zend_compile_const_decl(ast);
			break;
		// 定义命名空间
		case ZEND_AST_NAMESPACE:
			zend_compile_namespace(ast);
			break;
		// __halt_compiler
		case ZEND_AST_HALT_COMPILER:
			zend_compile_halt_compiler(ast);
			break;
		// throw 和 exit 语句
		case ZEND_AST_THROW:
		case ZEND_AST_EXIT:
			// 编译表达式语句, p1:返回结果，p2:表达式语句
			zend_compile_expr(NULL, ast);
			break;
		// 其他语句
		default:
		{
			znode result;
			// 编译表达式语句, p1:返回结果，p2:表达式语句
			zend_compile_expr(&result, ast);
			// 销毁编译结果（why）
			zend_do_free(&result);
		}
	}

	// 如果 declare(...)里面定义了ticks，
	// 并且语句不是未标记的语句：语句块，挑转标签，类属性定义，类常量定义，引用trait，类方法定义
	if (FC(declarables).ticks && !zend_is_unticked_stmt(ast)) {
		// 创建ticks操作码
		zend_emit_tick();
	}
}
/* }}} */

// ing4, 编译表达式的内部逻辑，只有 zend_compile_expr 调用，这是真正的业务逻辑
static void zend_compile_expr_inner(znode *result, zend_ast *ast) /* {{{ */
{
	// 行号
	/* CG(zend_lineno) = ast->lineno; */
	CG(zend_lineno) = zend_ast_get_lineno(ast);

	// 如果允许记忆编译，主要是 zend_compile_assign_coalesce 里面用
	if (CG(memoize_mode) != ZEND_MEMOIZE_NONE) {
		// 使用记忆缓存表 编译
		zend_compile_memoized_expr(result, ast);
		return;
	}

	// 支持的类型
	switch (ast->kind) {
		// 内置变量
		case ZEND_AST_ZVAL:
			ZVAL_COPY(&result->u.constant, zend_ast_get_zval(ast));
			result->op_type = IS_CONST;
			return;
		// 直接取出 ZNODE
		case ZEND_AST_ZNODE:
			*result = *zend_ast_get_znode(ast);
			return;
		// 普通变量
		case ZEND_AST_VAR:
		// 数组元素
		case ZEND_AST_DIM:
		// 调用类属性 ->prop ,?->prop
		case ZEND_AST_PROP:
		case ZEND_AST_NULLSAFE_PROP:
		// ::
		case ZEND_AST_STATIC_PROP:
		// 调用函数
		case ZEND_AST_CALL:
		// 调用类方法 ->method() ,?->method()
		case ZEND_AST_METHOD_CALL:
		case ZEND_AST_NULLSAFE_METHOD_CALL:
		// 调用类静态方法 static::method()
		case ZEND_AST_STATIC_CALL:
			// 编译变量，p1:返回值，p2:语句，p3:操作类型，p4:是否引用传递
			zend_compile_var(result, ast, BP_VAR_R, 0);
			return;
		// 赋值
		case ZEND_AST_ASSIGN:
			zend_compile_assign(result, ast);
			return;
		// 引用赋值, $a = &$b;
		case ZEND_AST_ASSIGN_REF:
			zend_compile_assign_ref(result, ast);
			return;
		// new 语句
		case ZEND_AST_NEW:
			zend_compile_new(result, ast);
			return;
		// clone 语句
		case ZEND_AST_CLONE:
			zend_compile_clone(result, ast);
			return;
		// 变量后面加 (运算符和= ) 再加表达式
		case ZEND_AST_ASSIGN_OP:
			zend_compile_compound_assign(result, ast);
			return;
		// 二元操作
		case ZEND_AST_BINARY_OP:
			zend_compile_binary_op(result, ast);
			return;
		// >, >=
		case ZEND_AST_GREATER:
		case ZEND_AST_GREATER_EQUAL:
			zend_compile_greater(result, ast);
			return;
		// ！~ 两个操作符，语句的一元操作
		case ZEND_AST_UNARY_OP:
			zend_compile_unary_op(result, ast);
			return;
		// 正负号，一元操作符 +，- 
		case ZEND_AST_UNARY_PLUS:
		case ZEND_AST_UNARY_MINUS:
			zend_compile_unary_pm(result, ast);
			return;
		// and, or 逻辑操作，相关语句有 and or isset()
		case ZEND_AST_AND:
		case ZEND_AST_OR:
			zend_compile_short_circuiting(result, ast);
			return;
		// $variable ++, $variable ++
		case ZEND_AST_POST_INC:
		case ZEND_AST_POST_DEC:
			zend_compile_post_incdec(result, ast);
			return;
		// ++ $variable, -- $variable
		case ZEND_AST_PRE_INC:
		case ZEND_AST_PRE_DEC:
			zend_compile_pre_incdec(result, ast);
			return;
		// 类型转换
		case ZEND_AST_CAST:
			zend_compile_cast(result, ast);
			return;
		// 三元操作符
		case ZEND_AST_CONDITIONAL:
			zend_compile_conditional(result, ast);
			return;
		// "??" 运算符 连接的两个表达式
		case ZEND_AST_COALESCE:
			zend_compile_coalesce(result, ast);
			return;
		// "??=" 运算符
		case ZEND_AST_ASSIGN_COALESCE:
			zend_compile_assign_coalesce(result, ast);
			return;
		// print 语句
		case ZEND_AST_PRINT:
			zend_compile_print(result, ast);
			return;
		// exit 语句
		case ZEND_AST_EXIT:
			zend_compile_exit(result, ast);
			return;
		// yield 语句
		case ZEND_AST_YIELD:
			zend_compile_yield(result, ast);
			return;
		// yield from 语句
		case ZEND_AST_YIELD_FROM:
			zend_compile_yield_from(result, ast);
			return;
		// instanceof 语句
		case ZEND_AST_INSTANCEOF:
			zend_compile_instanceof(result, ast);
			return;
		// include 或 eval 函数
		case ZEND_AST_INCLUDE_OR_EVAL:
			zend_compile_include_or_eval(result, ast);
			return;
		// isset,empty 函数
		case ZEND_AST_ISSET:
		case ZEND_AST_EMPTY:
			zend_compile_isset_or_empty(result, ast);
			return;
		// 静默符号 @
		case ZEND_AST_SILENCE:
			zend_compile_silence(result, ast);
			return;
		// shell_exec 函数
		case ZEND_AST_SHELL_EXEC:
			zend_compile_shell_exec(result, ast);
			return;
		// array
		case ZEND_AST_ARRAY:
			zend_compile_array(result, ast);
			return;
		// 常量
		case ZEND_AST_CONST:
			zend_compile_const(result, ast);
			return;
		// 类常量
		case ZEND_AST_CLASS_CONST:
			zend_compile_class_const(result, ast);
			return;
		// 类名
		case ZEND_AST_CLASS_NAME:
			zend_compile_class_name(result, ast);
			return;
		// 引号中的变量列表
		case ZEND_AST_ENCAPS_LIST:
			zend_compile_encaps_list(result, ast);
			return;
		// 编译魔术常量
		case ZEND_AST_MAGIC_CONST:
			zend_compile_magic_const(result, ast);
			return;
		// 编译闭包
		case ZEND_AST_CLOSURE:
		// fn() => stmt; 定义的闭包
		case ZEND_AST_ARROW_FUNC:
			zend_compile_func_decl(result, ast, 0);
			return;
		// throw 语句
		case ZEND_AST_THROW:
			zend_compile_throw(result, ast);
			return;
		// match 语句
		case ZEND_AST_MATCH:
			zend_compile_match(result, ast);
			return;
		// 
		default:
			// 不可以是其他类型
			ZEND_ASSERT(0 /* not supported */);
	}
}
/* }}} */

// ing3, 编译表达式语句, p1:返回结果，p2:表达式语句
// 这个是调用最多的方法，内部调用
static void zend_compile_expr(znode *result, zend_ast *ast)
{
	// 短路监测点，返回短路操作码堆栈深度
	uint32_t checkpoint = zend_short_circuiting_checkpoint();
	// 内部业务逻辑
		(result, ast);
	// 提交带短路的操作集合
	zend_short_circuiting_commit(checkpoint, result, ast);
}

// ing2, 编译变量 inner内部逻辑，一处调用 zend_compile_var
static zend_op *zend_compile_var_inner(znode *result, zend_ast *ast, uint32_t type, bool by_ref)
{
	// 设置全局行号
	CG(zend_lineno) = zend_ast_get_lineno(ast);

	switch (ast->kind) {
		// 类型是 Php变量
		case ZEND_AST_VAR:
			return zend_compile_simple_var(result, ast, type, 0);
		// 获取数组元素
		case ZEND_AST_DIM:
			return zend_compile_dim(result, ast, type, by_ref);
		// 类属性
		case ZEND_AST_PROP:
		case ZEND_AST_NULLSAFE_PROP:
			return zend_compile_prop(result, ast, type, by_ref);
		// 类静态属性
		case ZEND_AST_STATIC_PROP:
			return zend_compile_static_prop(result, ast, type, by_ref, 0);
		// 调用函数
		case ZEND_AST_CALL:
			zend_compile_call(result, ast, type);
			return NULL;
		// 调用类成员方法
		case ZEND_AST_METHOD_CALL:
		case ZEND_AST_NULLSAFE_METHOD_CALL:
			zend_compile_method_call(result, ast, type);
			return NULL;
		// 调用静态方法
		case ZEND_AST_STATIC_CALL:
			zend_compile_static_call(result, ast, type);
			return NULL;
		// 直接获取对象里的 znode
		case ZEND_AST_ZNODE:
			// return &((zend_ast_znode *) ast)->node;
			*result = *zend_ast_get_znode(ast);
			return NULL;
		// 其他情况
		default:
			// 不能给临时表达式赋值（已测试：array($a,$b)[0] = [1,2];）
			if (type == BP_VAR_W || type == BP_VAR_RW || type == BP_VAR_UNSET) {
				zend_error_noreturn(E_COMPILE_ERROR,
					"Cannot use temporary expression in write context");
			}
			// 编译表达式语句, p1:返回结果，p2:表达式语句
			zend_compile_expr(result, ast);
			return NULL;
	}
}

// ing4, 编译变量，p1:返回值，p2:语句，p3:操作类型，p4:是否引用传递
static zend_op *zend_compile_var(znode *result, zend_ast *ast, uint32_t type, bool by_ref) /* {{{ */
{
	// 短路监测点，返回短路操作码堆栈深度
	uint32_t checkpoint = zend_short_circuiting_checkpoint();
	// 编译变量，内部逻辑
	zend_op *opcode = zend_compile_var_inner(result, ast, type, by_ref);
	// 提交带短路的操作集合
	zend_short_circuiting_commit(checkpoint, result, ast);
	// 返回操作码
	return opcode;
}

// ing3，延时编译变量
static zend_op *zend_delayed_compile_var(znode *result, zend_ast *ast, uint32_t type, bool by_ref) /* {{{ */
{
	switch (ast->kind) {
		// php变量
		case ZEND_AST_VAR:
			return zend_compile_simple_var(result, ast, type, 1);
		// 数组元素
		case ZEND_AST_DIM:
			return zend_delayed_compile_dim(result, ast, type, by_ref);
		// 类属性（允许短路）
		case ZEND_AST_PROP:
		case ZEND_AST_NULLSAFE_PROP:
		{
			// 延时编译类属性
			zend_op *opline = zend_delayed_compile_prop(result, ast, type);
			// 如果是地址引用
			if (by_ref) {
				// 添加 ZEND_FETCH_REF 扩展标记
				opline->extended_value |= ZEND_FETCH_REF;
			}
			return opline;
		}
		// 静态属性
		case ZEND_AST_STATIC_PROP:
			// 编译 类的静态属性
			return zend_compile_static_prop(result, ast, type, by_ref, 1);
		// 其他
		default:
			// 编译变量，p1:返回值，p2:语句，p3:操作类型，p4:是否引用传递
			return zend_compile_var(result, ast, type, 0);
	}
}
/* }}} */

// ing3, 计算常量表达式，传入语句指针的指针，30多次调用，大量递归
static void zend_eval_const_expr(zend_ast **ast_ptr) /* {{{ */
{
	// 语句或表达式
	zend_ast *ast = *ast_ptr;
	// 结果
	zval result;
	// 语句可能为空
	if (!ast) {
		return;
	}

	// 根据类型操作
	switch (ast->kind) {
		// 二元操作符，需要前后两个表达式
		case ZEND_AST_BINARY_OP:
			zend_eval_const_expr(&ast->child[0]);
			zend_eval_const_expr(&ast->child[1]);
			// 如果两个不都是内置变量，中断
			if (ast->child[0]->kind != ZEND_AST_ZVAL || ast->child[1]->kind != ZEND_AST_ZVAL) {
				return;
			}
			// 如果尝试二元运算失败，中断
			if (!zend_try_ct_eval_binary_op(&result, ast->attr,
					zend_ast_get_zval(ast->child[0]), zend_ast_get_zval(ast->child[1]))
			) {
				return;
			}
			// 顺利执行完才break
			break;
		// >, >=
		case ZEND_AST_GREATER:
		case ZEND_AST_GREATER_EQUAL:
			// 计算两侧表达式
			zend_eval_const_expr(&ast->child[0]);
			zend_eval_const_expr(&ast->child[1]);
			// 两个子元素必须都是内置变量，否则 中断
			if (ast->child[0]->kind != ZEND_AST_ZVAL || ast->child[1]->kind != ZEND_AST_ZVAL) {
				return;
			}
			
			// 比较大小
			zend_ct_eval_greater(&result, ast->kind,
				zend_ast_get_zval(ast->child[0]), zend_ast_get_zval(ast->child[1]));
			break;
		// and or
		case ZEND_AST_AND:
		case ZEND_AST_OR:
		{
			bool child0_is_true, child1_is_true;
			// 计算左右两个表达式
			zend_eval_const_expr(&ast->child[0]);
			zend_eval_const_expr(&ast->child[1]);
			// 第一个结果必须是内置变量，否则中断
			if (ast->child[0]->kind != ZEND_AST_ZVAL) {
				return;
			}
			// 第一个是否为真
			child0_is_true = zend_is_true(zend_ast_get_zval(ast->child[0]));
			// 第一个为真，并且是or运算。或，第一个为假并且是 and 运算
			if (child0_is_true == (ast->kind == ZEND_AST_OR)) {
				// 直接返回结果
				ZVAL_BOOL(&result, ast->kind == ZEND_AST_OR);
				break;
			}

			// 第二个结果必须是内置变量，否则中断
			if (ast->child[1]->kind != ZEND_AST_ZVAL) {
				return;
			}
			// 第二个结果是否为真
			child1_is_true = zend_is_true(zend_ast_get_zval(ast->child[1]));
			// or 运算
			if (ast->kind == ZEND_AST_OR) {
				// 有一个真即可
				ZVAL_BOOL(&result, child0_is_true || child1_is_true);
			// and 运算
			} else {
				// 两个都要是真
				ZVAL_BOOL(&result, child0_is_true && child1_is_true);
			}
			break;
		}
		// ! ~ ，否运算
		case ZEND_AST_UNARY_OP:
			// 计算表达式
			zend_eval_const_expr(&ast->child[0]);
			// 不是内置变量，中断
			if (ast->child[0]->kind != ZEND_AST_ZVAL) {
				return;
			}
			// 如果计算语句失败，直接返回
			if (!zend_try_ct_eval_unary_op(&result, ast->attr, zend_ast_get_zval(ast->child[0]))) {
				return;
			}
			break;
		// 正负号，+-
		case ZEND_AST_UNARY_PLUS:
		case ZEND_AST_UNARY_MINUS:
			zend_eval_const_expr(&ast->child[0]);
			// 如果表达式结果不是内置变量，中断
			if (ast->child[0]->kind != ZEND_AST_ZVAL) {
				return;
			}
			// 如果计算失败，直接返回
			if (!zend_try_ct_eval_unary_pm(&result, ast->kind, zend_ast_get_zval(ast->child[0]))) {
				return;
			}
			break;
		// "??"
		case ZEND_AST_COALESCE:
			// 设置fetch标示 ,opcache不允许 AST 运行时变更
			/* Set isset fetch indicator here, opcache disallows runtime altering of the AST */
			// 如果左侧原素是数组元素
			if (ast->child[0]->kind == ZEND_AST_DIM) {
				// 添加标记 ZEND_DIM_IS ，is是issue
				ast->child[0]->attr |= ZEND_DIM_IS;
			}
			// 计算左侧常量表达式
			zend_eval_const_expr(&ast->child[0]);

			// 如果结果不是内置变量
			if (ast->child[0]->kind != ZEND_AST_ZVAL) {
				// 保证每个元素都在编译时计算至少1次
				/* ensure everything was compile-time evaluated at least once */
				// 计算右侧常量表达式
				zend_eval_const_expr(&ast->child[1]);
				return;
			}

			// 如果左侧结果是null
			if (Z_TYPE_P(zend_ast_get_zval(ast->child[0])) == IS_NULL) {
				// 计算右侧表达式
				zend_eval_const_expr(&ast->child[1]);
				// 右侧表达式当成返回结果
				*ast_ptr = ast->child[1];
				// 销毁右侧指针
				ast->child[1] = NULL;
				zend_ast_destroy(ast);
			// 左侧结果不是null
			} else {
				// 左侧表达式当成返回结果 
				*ast_ptr = ast->child[0];
				// 销毁左侧指针
				ast->child[0] = NULL;
				zend_ast_destroy(ast);
			}
			return;
		// 三元操作符
		case ZEND_AST_CONDITIONAL:
		{
			zend_ast **child, *child_ast;
			// 计算条件表达式
			zend_eval_const_expr(&ast->child[0]);
			// 如果条件结果不是 内置变量
			if (ast->child[0]->kind != ZEND_AST_ZVAL) {
				// 保证每个元素都在编译时计算至少1次
				/* ensure everything was compile-time evaluated at least once */
				// 如果存在真值语句
				if (ast->child[1]) {
					// 计算真值语句
					zend_eval_const_expr(&ast->child[1]);
				}
				// 计算假值语句
				zend_eval_const_expr(&ast->child[2]);
				return;
			}
			// 确定使用真值或假值
			child = &ast->child[2 - zend_is_true(zend_ast_get_zval(ast->child[0]))];
			// 如果使用真值并且为空
			if (*child == NULL) {
				// 切换到条件表达式（刚好是真值表达式前面一个）
				child--;
			}
			// 应用返回结果
			child_ast = *child;
			// 清空临时指针
			*child = NULL;
			// 销毁语句
			zend_ast_destroy(ast);
			// 结果指针指向临时指针的目标。这几下指针应用要细研究一下，为什么这么麻烦
			*ast_ptr = child_ast;
			// 计算结果语句
			zend_eval_const_expr(ast_ptr);
			return;
		}
		// 取数组元素
		case ZEND_AST_DIM:
		{
			/* constant expression should be always read context ... */
			zval *container, *dim;
			// 如果没有键名，报错（测试过 var_dump($a[]); ）
			if (ast->child[1] == NULL) {
				zend_error_noreturn(E_COMPILE_ERROR, "Cannot use [] for reading");
			}
			// 如果是大括号语法，提示已弃用
			if (ast->attr & ZEND_DIM_ALTERNATIVE_SYNTAX) {
				// 删除标记防止重复报错
				ast->attr &= ~ZEND_DIM_ALTERNATIVE_SYNTAX; /* remove flag to avoid duplicate warning */
				zend_error(E_COMPILE_ERROR, "Array and string offset access syntax with curly braces is no longer supported");
			}
			// 在这里添加 ZEND_DIM_IS 标示，opcache 不允许运行时更改语句对象
			/* Set isset fetch indicator here, opcache disallows runtime altering of the AST */
			if ((ast->attr & ZEND_DIM_IS) && ast->child[0]->kind == ZEND_AST_DIM) {
				ast->child[0]->attr |= ZEND_DIM_IS;
			}
			// 数组表达式
			zend_eval_const_expr(&ast->child[0]);
			// 元素名表达式
			zend_eval_const_expr(&ast->child[1]);
			// 两个里有一个不是常量，结束操作
			if (ast->child[0]->kind != ZEND_AST_ZVAL || ast->child[1]->kind != ZEND_AST_ZVAL) {
				return;
			}
			// 取得数组内置变量
			container = zend_ast_get_zval(ast->child[0]);
			// 取得键名
			dim = zend_ast_get_zval(ast->child[1]);

			// 如果元素是数组
			if (Z_TYPE_P(container) == IS_ARRAY) {
				zval *el;
				// 如果元素名是整数
				if (Z_TYPE_P(dim) == IS_LONG) {
					// 查找这个元素
					el = zend_hash_index_find(Z_ARR_P(container), Z_LVAL_P(dim));
					// 如果找到了
					if (el) {
						// 值复制给返回结果
						ZVAL_COPY(&result, el);
					// 找不到，结束操作
					} else {
						return;
					}
				// 如果元素名是字串
				} else if (Z_TYPE_P(dim) == IS_STRING) {
					// 在哈希表里找到对应的元素
					el = zend_symtable_find(Z_ARR_P(container), Z_STR_P(dim));
					// 值复制给返回结果
					if (el) {
						ZVAL_COPY(&result, el);
					// 找不到，结束操作
					} else {
						return;
					}
				// 其他情况，结束操作
				} else {
					return; /* warning... handle at runtime */
				}
			// 如果元素是字串
			} else if (Z_TYPE_P(container) == IS_STRING) {
				zend_long offset;
				zend_uchar c;
				// 如果元素名是整数
				if (Z_TYPE_P(dim) == IS_LONG) {
					// 取得整数下标
					offset = Z_LVAL_P(dim);
				// 如果元素名不是字串 或 不是转成整数的字串 或 下标元素不存在。 结束操作
				} else if (Z_TYPE_P(dim) != IS_STRING || is_numeric_string(Z_STRVAL_P(dim), Z_STRLEN_P(dim), &offset, NULL, 1) != IS_LONG) {
					return;
				}
				// 如果下标小于0或大于字串长度，结束操作
				if (offset < 0 || (size_t)offset >= Z_STRLEN_P(container)) {
					return;
				}
				// 取得下标对应的字符
				c = (zend_uchar) Z_STRVAL_P(container)[offset];
				// 把字符作为返回结果 #define RETVAL_CHAR(c) ZVAL_CHAR(return_value, c)
				ZVAL_CHAR(&result, c);
			// 如果是其他有效类型
			} else if (Z_TYPE_P(container) <= IS_FALSE) {
				return; /* warning... handle at runtime */
			// 如果不是有效类型
			} else {
				return;
			}
			break;
		}
		// array 语句列表
		case ZEND_AST_ARRAY:
			// 尝试计算array
			if (!zend_try_ct_eval_array(&result, ast)) {
				return;
			}
			break;
		// 魔术常量
		case ZEND_AST_MAGIC_CONST:
			// 尝试计算魔术常量
			if (!zend_try_ct_eval_magic_const(&result, ast)) {
				return;
			}
			break;
		// 常量
		case ZEND_AST_CONST:
		{
			// 常量名
			zend_ast *name_ast = ast->child[0];
			bool is_fully_qualified;
			// 完整常量名
			zend_string *resolved_name = zend_resolve_const_name(
				zend_ast_get_str(name_ast), name_ast->attr, &is_fully_qualified);
			// 尝试计算常量，如果失败
			if (!zend_try_ct_eval_const(&result, resolved_name, is_fully_qualified)) {
				// 清空常量名
				zend_string_release_ex(resolved_name, 0);
				return;
			}
			// 释放常量名
			zend_string_release_ex(resolved_name, 0);
			break;
		}
		// 类常量
		case ZEND_AST_CLASS_CONST:
		{
			zend_ast *class_ast;
			zend_ast *name_ast;
			zend_string *resolved_name;
			// 计算类名
			zend_eval_const_expr(&ast->child[0]);
			// 计算常量名
			zend_eval_const_expr(&ast->child[1]);
			// 
			class_ast = ast->child[0];
			name_ast = ast->child[1];

			// 如果类名或常量名不是内置变量，返回
			if (class_ast->kind != ZEND_AST_ZVAL || name_ast->kind != ZEND_AST_ZVAL) {
				return;
			}
			// 获取完整类名
			resolved_name = zend_resolve_class_name_ast(class_ast);
			// 计算并返回类常量，如果失败
			if (!zend_try_ct_eval_class_const(&result, resolved_name, zend_ast_get_str(name_ast))) {
				// 清空常量名
				zend_string_release_ex(resolved_name, 0);
				return;
			}
			// 清空常量名
			zend_string_release_ex(resolved_name, 0);
			break;
		}
		// 类名
		case ZEND_AST_CLASS_NAME:
		{
			// 第一个子语句是类名
			zend_ast *class_ast = ast->child[0];
			// 计算并返回类名
			if (!zend_try_compile_const_expr_resolve_class_name(&result, class_ast)) {
				return;
			}
			break;
		}
		//应该使用 zend_ast_apply 来递归处理 nodes 不用特殊的 handling
		// TODO: We should probably use zend_ast_apply to recursively walk nodes without
		// 常量表达式的每一部分都是可访问的。
		// special handling. It is required that all nodes that are part of a const expr
		// are visited. Probably we should be distinguishing evaluation of const expr and
		// normal exprs here.
		// 参数列表
		case ZEND_AST_ARG_LIST:
		{
			zend_ast_list *list = zend_ast_get_list(ast);
			for (uint32_t i = 0; i < list->children; i++) {
				// 依次计算每个参数
				zend_eval_const_expr(&list->child[i]);
			}
			return;
		}
		// new 语句
		case ZEND_AST_NEW:
			// 计算类名
			zend_eval_const_expr(&ast->child[0]);
			// 计算参数列表
			zend_eval_const_expr(&ast->child[1]);
			return;
		// 带名称的参数
		case ZEND_AST_NAMED_ARG:
			zend_eval_const_expr(&ast->child[1]);
			return;
		// 初始化枚举（这个类型是在编译枚举 case 时产生的）
		case ZEND_AST_CONST_ENUM_INIT:
			// 第三个子句是case 后面的值，参见 zend_compile_enum_case
			zend_eval_const_expr(&ast->child[2]);
			return;
		// 类属性，或允null
		case ZEND_AST_PROP:
		case ZEND_AST_NULLSAFE_PROP:
			// 计算对象名
			zend_eval_const_expr(&ast->child[0]);
			// 计算属性名
			zend_eval_const_expr(&ast->child[1]);
			return;
		// 其它类型什么也不做
		default:
			return;
	}
	// 销毁原来的语句
	zend_ast_destroy(ast);
	
	// 用运算结果创建新语句，类型为内置变量语句
	*ast_ptr = zend_ast_create_zval(&result);
}
/* }}} */
