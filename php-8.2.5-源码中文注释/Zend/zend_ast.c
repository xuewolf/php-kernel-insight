// 50多个函数，2500 行，平均每个50行
// 只有这几个有外部调用
func	count	file_count	files
zend_ast_destroy	21	6	/ext/tokenizer/tokenizer.c,/Zend/zend_API.c,/Zend/zend_ast.c,/Zend/zend_compile.c,/Zend/zend_language_parser.y,/Zend/zend_language_scanner.l
zend_ast_create	144	3	/Zend/zend_ast.c,/Zend/zend_compile.c,/Zend/zend_language_parser.y
binary_op	48	3	/ext/opcache/jit/zend_jit_helpers.c,/Zend/Optimizer/zend_optimizer.c,/Zend/zend_ast.c
zend_ast_create_ex	33	3	/Zend/zend_ast.h,/Zend/zend_compile.c,/Zend/zend_language_parser.y
zend_ast_evaluate	26	3	/ext/opcache/jit/zend_jit_helpers.c,/Zend/zend_ast.c,/Zend/zend_execute_API.c
zend_ast_create_list	41	2	/Zend/zend_compile.c,/Zend/zend_language_parser.y
// export 只有这里有外部调用，HOHO，refection 在这里！！
zend_ast_export	2	2	/ext/reflection/php_reflection.c,/Zend/zend_compile.c
zend_ast_create_decl	10	1	/Zend/zend_language_parser.y
zend_ast_create_concat_op	1	1	/Zend/zend_language_parser.y
zend_ast_apply	1	1	/Zend/zend_compile.c

// 这几个内部调用比较多
zend_ast_export_ex	58	1	/Zend/zend_ast.c
zend_ast_export_indent	22	1	/Zend/zend_ast.c
zend_ast_export_stmt	18	1	/Zend/zend_ast.c
PREFIX_OP	17	1	/Zend/zend_ast.c
zend_ast_alloc	16	1	/Zend/zend_ast.c
zend_ast_export_name	15	1	/Zend/zend_ast.c
APPEND_STR	14	1	/Zend/zend_ast.c
zend_ast_export_ns_name	11	1	/Zend/zend_ast.c
FUNC_OP	9	1	/Zend/zend_ast.c
zend_ast_list_size	8	1	/Zend/zend_ast.c
zend_ast_export_attributes	7	1	/Zend/zend_ast.c
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
   +----------------------------------------------------------------------+
*/

#include "zend_ast.h"
#include "zend_API.h"
#include "zend_operators.h"
#include "zend_language_parser.h"
#include "zend_smart_str.h"
#include "zend_exceptions.h"
#include "zend_constants.h"
#include "zend_enum.h"

ZEND_API zend_ast_process_t zend_ast_process = NULL;

// 给ast分配内存空间, 这里始终只有一个CG(ast_arena)，所以用的是同一块内存？
static inline void *zend_ast_alloc(size_t size) {
	return zend_arena_alloc(&CG(ast_arena), size);
}

// 给ast重新分配内存空间，再把旧数据放回去？ 
// 专门给ast_list 用的，只有一次内部调用
static inline void *zend_ast_realloc(void *old, size_t old_size, size_t new_size) {
	void *new = zend_ast_alloc(new_size);
	memcpy(new, old, old_size);
	return new;
}

// ast_list 的大小，同样是减掉默认的子元素
static inline size_t zend_ast_list_size(uint32_t children) {
	return sizeof(zend_ast_list) - sizeof(zend_ast *) + sizeof(zend_ast *) * children;
}

// 创建一个 ast 包含给定的node, zend_ast_znode 和 zend_ast 直接通用？
ZEND_API zend_ast * ZEND_FASTCALL zend_ast_create_znode(znode *node) {
	zend_ast_znode *ast;
	// 关键在于 zend_ast_alloc， 实际上创建的还是zend_ast 
	ast = zend_ast_alloc(sizeof(zend_ast_znode));
	ast->kind = ZEND_AST_ZNODE;
	ast->attr = 0;
	ast->lineno = CG(zend_lineno);
	ast->node = *node;
	return (zend_ast *) ast;
}

// 创建包含 变量的语句（这个和int有什么关系？）， 变量值存放在val里
static zend_always_inline zend_ast * zend_ast_create_zval_int(zval *zv, uint32_t attr, uint32_t lineno) {
	zend_ast_zval *ast;

	ast = zend_ast_alloc(sizeof(zend_ast_zval));
	ast->kind = ZEND_AST_ZVAL;
	ast->attr = attr;
	// 把原变量的值复制过来
	ZVAL_COPY_VALUE(&ast->val, zv);
	Z_LINENO(ast->val) = lineno;
	return (zend_ast *) ast;
}

// 用给定变量和行号，创建 ast
ZEND_API zend_ast * ZEND_FASTCALL zend_ast_create_zval_with_lineno(zval *zv, uint32_t lineno) {
	return zend_ast_create_zval_int(zv, 0, lineno);
}

// 用给定变量，和属性，创建 ast
ZEND_API zend_ast * ZEND_FASTCALL zend_ast_create_zval_ex(zval *zv, zend_ast_attr attr) {
	return zend_ast_create_zval_int(zv, attr, CG(zend_lineno));
}

// 用给定变量创建 ast
ZEND_API zend_ast * ZEND_FASTCALL zend_ast_create_zval(zval *zv) {
	return zend_ast_create_zval_int(zv, 0, CG(zend_lineno));
}

// 用给定字串创建 ast
ZEND_API zend_ast * ZEND_FASTCALL zend_ast_create_zval_from_str(zend_string *str) {
	zval zv;
	ZVAL_STR(&zv, str);
	return zend_ast_create_zval_int(&zv, 0, CG(zend_lineno));
}

// 用给长整型变量创建 ast
ZEND_API zend_ast * ZEND_FASTCALL zend_ast_create_zval_from_long(zend_long lval) {
	zval zv;
	ZVAL_LONG(&zv, lval);
	return zend_ast_create_zval_int(&zv, 0, CG(zend_lineno));
}

// compile时，创建常量 ast ，名称存放在 val 里
ZEND_API zend_ast * ZEND_FASTCALL zend_ast_create_constant(zend_string *name, zend_ast_attr attr) {
	zend_ast_zval *ast;

	ast = zend_ast_alloc(sizeof(zend_ast_zval));
	// 常量类型
	ast->kind = ZEND_AST_CONSTANT;
	ast->attr = attr;
	// 名称存放在 val 里
	ZVAL_STR(&ast->val, name);
	Z_LINENO(ast->val) = CG(zend_lineno);
	return (zend_ast *) ast;
}

// 创建类名，或类常量名（why together）
ZEND_API zend_ast * ZEND_FASTCALL zend_ast_create_class_const_or_name(zend_ast *class_name, zend_ast *name) {
	// 取得name中的字串变量
	zend_string *name_str = zend_ast_get_str(name);
	// 如果它是class
	if (zend_string_equals_literal_ci(name_str, "class")) {
		zend_string_release(name_str);
		return zend_ast_create(ZEND_AST_CLASS_NAME, class_name);
	} else {
		return zend_ast_create(ZEND_AST_CLASS_CONST, class_name, name);
	}
}

// 创建一个声明型 语句
// 参数： kind 类型，flags 修饰符,  start_lineno 起始行号，doc_comment 文档，name 类名，
// child0 , child1, child2, child3, child4
ZEND_API zend_ast *zend_ast_create_decl(
	zend_ast_kind kind, uint32_t flags, uint32_t start_lineno, zend_string *doc_comment,
	zend_string *name, zend_ast *child0, zend_ast *child1, zend_ast *child2, zend_ast *child3, zend_ast *child4
) {
	zend_ast_decl *ast;

	ast = zend_ast_alloc(sizeof(zend_ast_decl));
	ast->kind = kind;
	//  没有attr
	ast->attr = 0;
	ast->start_lineno = start_lineno;
	// 行号用全局行号
	ast->end_lineno = CG(zend_lineno);
	ast->flags = flags;
	// ?
	ast->lex_pos = LANG_SCNG(yy_text);
	ast->doc_comment = doc_comment;
	ast->name = name;
	ast->child[0] = child0;
	ast->child[1] = child1;
	ast->child[2] = child2;
	ast->child[3] = child3;
	ast->child[4] = child4;

	return (zend_ast *) ast;
}

#if ZEND_AST_SPEC

// clear, 开辟内存空间，创建0-5个子元素的语句
ZEND_API zend_ast * ZEND_FASTCALL zend_ast_create_0(zend_ast_kind kind) {
	zend_ast *ast;

	ZEND_ASSERT(kind >> ZEND_AST_NUM_CHILDREN_SHIFT == 0);
	ast = zend_ast_alloc(zend_ast_size(0));
	ast->kind = kind;
	ast->attr = 0;
	// CG(zend_lineno) 行号只在解析和编译用到
	ast->lineno = CG(zend_lineno);

	return ast;
}

// 1-5个子元素， 除了复制子元素指针，还要获取第一个有效子元素的行号作为自己的行号
// clear, 如果子元素都无效，获取当前全局行号
ZEND_API zend_ast * ZEND_FASTCALL zend_ast_create_1(zend_ast_kind kind, zend_ast *child) {
	zend_ast *ast;
	uint32_t lineno;

	ZEND_ASSERT(kind >> ZEND_AST_NUM_CHILDREN_SHIFT == 1);
	ast = zend_ast_alloc(zend_ast_size(1));
	ast->kind = kind;
	ast->attr = 0;
	ast->child[0] = child;
	if (child) {
		lineno = zend_ast_get_lineno(child);
	} else {
		lineno = CG(zend_lineno);
	}
	ast->lineno = lineno;

	return ast;
}

// clear,
ZEND_API zend_ast * ZEND_FASTCALL zend_ast_create_2(zend_ast_kind kind, zend_ast *child1, zend_ast *child2) {
	zend_ast *ast;
	uint32_t lineno;

	ZEND_ASSERT(kind >> ZEND_AST_NUM_CHILDREN_SHIFT == 2);
	ast = zend_ast_alloc(zend_ast_size(2));
	ast->kind = kind;
	ast->attr = 0;
	ast->child[0] = child1;
	ast->child[1] = child2;
	if (child1) {
		lineno = zend_ast_get_lineno(child1);
	} else if (child2) {
		lineno = zend_ast_get_lineno(child2);
	} else {
		lineno = CG(zend_lineno);
	}
	ast->lineno = lineno;

	return ast;
}

// clear,
ZEND_API zend_ast * ZEND_FASTCALL zend_ast_create_3(zend_ast_kind kind, zend_ast *child1, zend_ast *child2, zend_ast *child3) {
	zend_ast *ast;
	uint32_t lineno;

	ZEND_ASSERT(kind >> ZEND_AST_NUM_CHILDREN_SHIFT == 3);
	ast = zend_ast_alloc(zend_ast_size(3));
	ast->kind = kind;
	ast->attr = 0;
	ast->child[0] = child1;
	ast->child[1] = child2;
	ast->child[2] = child3;
	if (child1) {
		lineno = zend_ast_get_lineno(child1);
	} else if (child2) {
		lineno = zend_ast_get_lineno(child2);
	} else if (child3) {
		lineno = zend_ast_get_lineno(child3);
	} else {
		lineno = CG(zend_lineno);
	}
	ast->lineno = lineno;

	return ast;
}

// clear,
ZEND_API zend_ast * ZEND_FASTCALL zend_ast_create_4(zend_ast_kind kind, zend_ast *child1, zend_ast *child2, zend_ast *child3, zend_ast *child4) {
	zend_ast *ast;
	uint32_t lineno;

	ZEND_ASSERT(kind >> ZEND_AST_NUM_CHILDREN_SHIFT == 4);
	ast = zend_ast_alloc(zend_ast_size(4));
	ast->kind = kind;
	ast->attr = 0;
	ast->child[0] = child1;
	ast->child[1] = child2;
	ast->child[2] = child3;
	ast->child[3] = child4;
	if (child1) {
		lineno = zend_ast_get_lineno(child1);
	} else if (child2) {
		lineno = zend_ast_get_lineno(child2);
	} else if (child3) {
		lineno = zend_ast_get_lineno(child3);
	} else if (child4) {
		lineno = zend_ast_get_lineno(child4);
	} else {
		lineno = CG(zend_lineno);
	}
	ast->lineno = lineno;

	return ast;
}

// clear,
ZEND_API zend_ast * ZEND_FASTCALL zend_ast_create_5(zend_ast_kind kind, zend_ast *child1, zend_ast *child2, zend_ast *child3, zend_ast *child4, zend_ast *child5) {
	zend_ast *ast;
	uint32_t lineno;

	ZEND_ASSERT(kind >> ZEND_AST_NUM_CHILDREN_SHIFT == 5);
	ast = zend_ast_alloc(zend_ast_size(5));
	ast->kind = kind;
	ast->attr = 0;
	ast->child[0] = child1;
	ast->child[1] = child2;
	ast->child[2] = child3;
	ast->child[3] = child4;
	ast->child[4] = child5;
	if (child1) {
		lineno = zend_ast_get_lineno(child1);
	} else if (child2) {
		lineno = zend_ast_get_lineno(child2);
	} else if (child3) {
		lineno = zend_ast_get_lineno(child3);
	} else if (child4) {
		lineno = zend_ast_get_lineno(child4);
	} else if (child5) {
		lineno = zend_ast_get_lineno(child5);
	} else {
		lineno = CG(zend_lineno);
	}
	ast->lineno = lineno;

	return ast;
}

// 开辟内存空间，创建包含0-2个(why)子元素的 ast_list 。 固定初始化4个子元素。 
// ast_list 比 ast 唯一多的能力是可伸缩。 行号一律用子元素中最小的行号。
ZEND_API zend_ast * ZEND_FASTCALL zend_ast_create_list_0(zend_ast_kind kind) {
	zend_ast *ast;
	zend_ast_list *list;

	ast = zend_ast_alloc(zend_ast_list_size(4));
	list = (zend_ast_list *) ast;
	list->kind = kind;
	list->attr = 0;
	list->lineno = CG(zend_lineno);
	list->children = 0;

	return ast;
}

// clear, 
ZEND_API zend_ast * ZEND_FASTCALL zend_ast_create_list_1(zend_ast_kind kind, zend_ast *child) {
	zend_ast *ast;
	zend_ast_list *list;
	uint32_t lineno;

	ast = zend_ast_alloc(zend_ast_list_size(4));
	list = (zend_ast_list *) ast;
	list->kind = kind;
	list->attr = 0;
	list->children = 1;
	list->child[0] = child;
	if (child) {
		lineno = zend_ast_get_lineno(child);
		if (lineno > CG(zend_lineno)) {
			lineno = CG(zend_lineno);
		}
	} else {
		lineno = CG(zend_lineno);
	}
	list->lineno = lineno;

	return ast;
}

// clear,
ZEND_API zend_ast * ZEND_FASTCALL zend_ast_create_list_2(zend_ast_kind kind, zend_ast *child1, zend_ast *child2) {
	zend_ast *ast;
	zend_ast_list *list;
	uint32_t lineno;

	ast = zend_ast_alloc(zend_ast_list_size(4));
	list = (zend_ast_list *) ast;
	list->kind = kind;
	list->attr = 0;
	list->children = 2;
	list->child[0] = child1;
	list->child[1] = child2;
	if (child1) {
		lineno = zend_ast_get_lineno(child1);
		if (lineno > CG(zend_lineno)) {
			lineno = CG(zend_lineno);
		}
	} else if (child2) {
		lineno = zend_ast_get_lineno(child2);
		if (lineno > CG(zend_lineno)) {
			lineno = CG(zend_lineno);
		}
	} else {
		list->children = 0;
		lineno = CG(zend_lineno);
	}
	list->lineno = lineno;

	return ast;
}
#else
// clear, 创建包含子元素的语句，从参数列表创建子元素，kind决定了子元素数量
// 创建数组键值对的时候，参数是动态的
static zend_ast *zend_ast_create_from_va_list(zend_ast_kind kind, zend_ast_attr attr, va_list va) {
	// 设置子元素数量
	uint32_t i, children = kind >> ZEND_AST_NUM_CHILDREN_SHIFT;
	zend_ast *ast;
	// 分配内存创建ast对象
	ast = zend_ast_alloc(zend_ast_size(children));
	ast->kind = kind;
	ast->attr = attr;
	// 行号-1
	ast->lineno = (uint32_t) -1;
	// 子元素数量
	for (i = 0; i < children; ++i) {
		ast->child[i] = va_arg(va, zend_ast *);
		// 如果子元素不是null (这里很多情况会是null)
		if (ast->child[i] != NULL) {
			// 把当前语句行号更新成最小的行号
			uint32_t lineno = zend_ast_get_lineno(ast->child[i]);
			if (lineno < ast->lineno) {
				ast->lineno = lineno;
			}
		}
	}
	// 如果没设置行号，使用当前全局行号
	if (ast->lineno == UINT_MAX) {
		ast->lineno = CG(zend_lineno);
	}

	return ast;
}

// 创建ast， 实际业务逻辑在 zend_ast_create_from_va_list ，主要给 parser用
// why this alias exists?
ZEND_API zend_ast *zend_ast_create_ex(zend_ast_kind kind, zend_ast_attr attr, ...) {
	va_list va;
	zend_ast *ast;

	va_start(va, attr);
	ast = zend_ast_create_from_va_list(kind, attr, va);
	va_end(va);

	return ast;
}

// 创建语句对象，主要是 parser 用
ZEND_API zend_ast *zend_ast_create(zend_ast_kind kind, ...) {
	va_list va;
	zend_ast *ast;

	va_start(va, kind);
	ast = zend_ast_create_from_va_list(kind, 0, va);
	va_end(va);

	return ast;
}

// 创建 ast_list，添加指定数量的子元素，子元素必须是 zend_ast
ZEND_API zend_ast *zend_ast_create_list(uint32_t init_children, zend_ast_kind kind, ...) {
	zend_ast *ast;
	zend_ast_list *list;
	// 创建包含4个子元素的ast_list
	ast = zend_ast_alloc(zend_ast_list_size(4));
	list = (zend_ast_list *) ast;
	list->kind = kind;
	list->attr = 0;
	// 全局行号
	list->lineno = CG(zend_lineno);
	list->children = 0;

	{
		va_list va;
		uint32_t i;
		va_start(va, kind);
		// 获取指定数量的子元素
		for (i = 0; i < init_children; ++i) {
			zend_ast *child = va_arg(va, zend_ast *);
			// 添加到列表中
			ast = zend_ast_list_add(ast, child);
			if (child != NULL) {
				uint32_t lineno = zend_ast_get_lineno(child);
				// 取得最小的行号
				if (lineno < ast->lineno) {
					ast->lineno = lineno;
				}
			}
		}
		va_end(va);
	}

	return ast;
}
#endif

// ？ 创建一个连接语句
zend_ast *zend_ast_create_concat_op(zend_ast *op0, zend_ast *op1) {
	if (op0->kind == ZEND_AST_ZVAL && op1->kind == ZEND_AST_ZVAL) {
		zval *zv0 = zend_ast_get_zval(op0);
		zval *zv1 = zend_ast_get_zval(op1);
		// try to concat zvals in these operation,return a zend_ast
		if (!zend_binary_op_produces_error(ZEND_CONCAT, zv0, zv1) &&
				concat_function(zv0, zv0, zv1) == SUCCESS) {
			zval_ptr_dtor_nogc(zv1);
			return zend_ast_create_zval(zv0);
		}
	}
	// if failure , create a new zend_ast which contain a concat operator and those two zend_ast
	return zend_ast_create_binary_op(ZEND_CONCAT, op0, op1);
}

// 是2的幂值 ? 010 & 101(110+1) == 010 ?
static inline bool is_power_of_two(uint32_t n) {
	return ((n != 0) && (n == (n & (~n + 1))));
}

// 向 ast_list 中添加 ast，内部一次
ZEND_API zend_ast * ZEND_FASTCALL zend_ast_list_add(zend_ast *ast, zend_ast *op) {
	// ast 转成 ast_list
	zend_ast_list *list = zend_ast_get_list(ast);
	// 如果字元素数量>=4且是2的幂，再增加一倍子元素空间(why)
	if (list->children >= 4 && is_power_of_two(list->children)) {
			list = zend_ast_realloc(list,
			zend_ast_list_size(list->children), zend_ast_list_size(list->children * 2));
	}
	// 把op作为子元素添加给ast
	list->child[list->children++] = op;
	return (zend_ast *) list;
}

// ？ 向数组中添加元素， offset 位置，内部一次
static zend_result zend_ast_add_array_element(zval *result, zval *offset, zval *expr)
{
	// 如果未定义offset ，是索引数组
	if (Z_TYPE_P(offset) == IS_UNDEF) {
		if (!zend_hash_next_index_insert(Z_ARRVAL_P(result), expr)) {
			zend_throw_error(NULL,
				"Cannot add element to the array as the next element is already occupied");
			return FAILURE;
		}
		return SUCCESS;
	}

	// 如果是哈希表
	if (array_set_zval_key(Z_ARRVAL_P(result), offset, expr) == FAILURE) {
		return FAILURE;
	}

	// ? zend_variables
	zval_ptr_dtor_nogc(offset);
	zval_ptr_dtor_nogc(expr);
	return SUCCESS;
}

// ing3, 把指定表达式的值添加到现有数组中，内部一次
static zend_result zend_ast_add_unpacked_element(zval *result, zval *expr) {
	// 如果 expr 类型是 array
	if (EXPECTED(Z_TYPE_P(expr) == IS_ARRAY)) {
		HashTable *ht = Z_ARRVAL_P(expr);
		zval *val;
		zend_string *key;

		ZEND_HASH_FOREACH_STR_KEY_VAL(ht, key, val) {
			// 如果制定了key,更新指定键的值
			if (key) {
				zend_hash_update(Z_ARRVAL_P(result), key, val);
			// 否则，添加进数组
			} else {
				if (!zend_hash_next_index_insert(Z_ARRVAL_P(result), val)) {
					zend_throw_error(NULL,
						"Cannot add element to the array as the next element is already occupied");
					return FAILURE;
				}
			}
			Z_TRY_ADDREF_P(val);
		} ZEND_HASH_FOREACH_END();
		return SUCCESS;
	}

	// 在常量表达式中 unpack 数组
	zend_throw_error(NULL, "Only arrays can be unpacked in constant expression");
	return FAILURE;
}

// ？ 内部一次
zend_class_entry *zend_ast_fetch_class(zend_ast *ast, zend_class_entry *scope)
{
	return zend_fetch_class_with_scope(zend_ast_get_str(ast), (ast->attr >> ZEND_CONST_EXPR_NEW_FETCH_TYPE_SHIFT) | ZEND_FETCH_CLASS_EXCEPTION, scope);
}

// 大量使用 zval_ptr_dtor_nogc, 销毁临时计算结果
// 执行evaluate, 500行， 传入，zval,ast, class_entry, bool， 内部少量调用
// _ex execute： 所谓解释执行，指的是这里，一边解释一边执行
static zend_result ZEND_FASTCALL zend_ast_evaluate_ex(zval *result, zend_ast *ast, zend_class_entry *scope, bool *short_circuited_ptr)
{
	// 计算 ast->child[0]，ast->child[1] 的返回结果
	zval op1, op2;
	zend_result ret = SUCCESS;
	// 用->, ?-> 获取对象属性（prop）名称为空时的标记
	*short_circuited_ptr = false;

	// 空对象的 kind 如何处理？
	switch (ast->kind) {
		// ing3, 二进制操作： + - * 、 . 等， 对应了 operators.c 里面的一大堆方法
		case ZEND_AST_BINARY_OP:
			// 尝试 在 scope 作用域内递归计算 第一个子语句
			if (UNEXPECTED(zend_ast_evaluate(&op1, ast->child[0], scope) != SUCCESS)) {
				ret = FAILURE;
			// 如果失败，尝试 在 scope 作用域内递归计算 第二个子语句
			} else if (UNEXPECTED(zend_ast_evaluate(&op2, ast->child[1], scope) != SUCCESS)) {
				zval_ptr_dtor_nogc(&op1);
				ret = FAILURE;
			// 如果没有子语句要计算
			} else {
				// 取回操作符对应的操作函数名 zend_opcode.c， ast->attr 是 int类型
				binary_op_type op = get_binary_op(ast->attr);
				// 依次调用 -> zend_opcode.c -> zend_operators.c 中的函数 例如 add_function_fast
				ret = op(result, &op1, &op2);
				// 销毁用过的 op （why)
				zval_ptr_dtor_nogc(&op1);
				zval_ptr_dtor_nogc(&op2);
			}
			break;
		// ing3, 比大小操作符 >, >=
		case ZEND_AST_GREATER:
		case ZEND_AST_GREATER_EQUAL:
			// 先计算两个子语句，然后调用操作函数
			if (UNEXPECTED(zend_ast_evaluate(&op1, ast->child[0], scope) != SUCCESS)) {
				ret = FAILURE;
			} else if (UNEXPECTED(zend_ast_evaluate(&op2, ast->child[1], scope) != SUCCESS)) {
				zval_ptr_dtor_nogc(&op1);
				ret = FAILURE;
			} else {
				// 小于就是把大于反过来
				/* op1 > op2 is the same as op2 < op1 */
				// 比大小函数名
				binary_op_type op = ast->kind == ZEND_AST_GREATER
					? is_smaller_function : is_smaller_or_equal_function;
				ret = op(result, &op2, &op1);
				// 销毁
				zval_ptr_dtor_nogc(&op1);
				zval_ptr_dtor_nogc(&op2);
			}
			break;
		// ing3, !~ 一元操作符
		case ZEND_AST_UNARY_OP:
			// 逻辑同上, 如果计算 child[0] 出错
			if (UNEXPECTED(zend_ast_evaluate(&op1, ast->child[0], scope) != SUCCESS)) {
				ret = FAILURE;
			} else {
				unary_op_type op = get_unary_op(ast->attr);
				ret = op(result, &op1);
				zval_ptr_dtor_nogc(&op1);
			}
			break;
		// ing3, 变量赋值
		case ZEND_AST_ZVAL:
		{
			zval *zv = zend_ast_get_zval(ast);
			// 直接把值复制给 result
			ZVAL_COPY(result, zv);
			break;
		}
		// ? , compile时，创建的常量 ast
		case ZEND_AST_CONSTANT:
		{
			zend_string *name = zend_ast_get_constant_name(ast);
			// 计算常量的值，zend_constant.c
			zval *zv = zend_get_constant_ex(name, scope, ast->attr);

			if (UNEXPECTED(zv == NULL)) {
				ZVAL_UNDEF(result);
				return FAILURE;
			}
			ZVAL_COPY_OR_DUP(result, zv);
			break;
		}
		// ing2 编译时创建，这是获取类名 __CLASS__ 的魔术变量的反射
		case ZEND_AST_CONSTANT_CLASS:
			if (scope) {
				ZVAL_STR_COPY(result, scope->name);
			} else {
				ZVAL_EMPTY_STRING(result);
			}
			break;
		// ing3 编译时创建，类名，self 或 parent 两个魔术常量
		case ZEND_AST_CLASS_NAME:
			// 一定要有作用域
			if (!scope) {
				zend_throw_error(NULL, "Cannot use \"self\" when no class scope is active");
				return FAILURE;
			}
			// self, 
			if (ast->attr == ZEND_FETCH_CLASS_SELF) {
				ZVAL_STR_COPY(result, scope->name);
			// parent, 
			} else if (ast->attr == ZEND_FETCH_CLASS_PARENT) {
				if (!scope->parent) {
					zend_throw_error(NULL,
						"Cannot use \"parent\" when current class scope has no parent");
					return FAILURE;
				}
				ZVAL_STR_COPY(result, scope->parent->name);
			// 其他报错
			} else {
				ZEND_ASSERT(0 && "Should have errored during compilation");
			}
			break;
		// ing3, 操作符 and 	
		case ZEND_AST_AND:
			// 如果计算第一个子元素出错,返回 failure
			if (UNEXPECTED(zend_ast_evaluate(&op1, ast->child[0], scope) != SUCCESS)) {
				ret = FAILURE;
				break;
			}
			// 如果第一个子元素返回值是true
			if (zend_is_true(&op1)) {
				// 第二个计算结果出错 返回 failure
				if (UNEXPECTED(zend_ast_evaluate(&op2, ast->child[1], scope) != SUCCESS)) {
					zval_ptr_dtor_nogc(&op1);
					ret = FAILURE;
					break;
				}
				// 结果是 op2 是否为true
				ZVAL_BOOL(result, zend_is_true(&op2));
				zval_ptr_dtor_nogc(&op2);
			} else {
				// 结果 设置为false
				ZVAL_FALSE(result);
			}
			zval_ptr_dtor_nogc(&op1);
			break;
		// ing3, 操作符 or
		case ZEND_AST_OR:
			// 逻辑类似
			if (UNEXPECTED(zend_ast_evaluate(&op1, ast->child[0], scope) != SUCCESS)) {
				ret = FAILURE;
				break;
			}
			if (zend_is_true(&op1)) {
				ZVAL_TRUE(result);
			} else {
				if (UNEXPECTED(zend_ast_evaluate(&op2, ast->child[1], scope) != SUCCESS)) {
					zval_ptr_dtor_nogc(&op1);
					ret = FAILURE;
					break;
				}
				ZVAL_BOOL(result, zend_is_true(&op2));
				zval_ptr_dtor_nogc(&op2);
			}
			zval_ptr_dtor_nogc(&op1);
			break;
			
		// ing3, 只是三元操作符
		case ZEND_AST_CONDITIONAL:
			// child[0] 是条件表达式 ，child[1] 真值，child[2] 假值
			// 如果计算 child[0] 出错，返回 failure
			if (UNEXPECTED(zend_ast_evaluate(&op1, ast->child[0], scope) != SUCCESS)) {
				ret = FAILURE;
				break;
			}
			// 条件为真
			if (zend_is_true(&op1)) {
				// 真值为空，用条件 1 的值
				if (!ast->child[1]) {
					*result = op1;
				// 计算真值并用 result 返回
				} else {
					if (UNEXPECTED(zend_ast_evaluate(result, ast->child[1], scope) != SUCCESS)) {
						zval_ptr_dtor_nogc(&op1);
						ret = FAILURE;
						break;
					}
					zval_ptr_dtor_nogc(&op1);
				}
			// 条件为假
			} else {
				// 计算假值并返回
				if (UNEXPECTED(zend_ast_evaluate(result, ast->child[2], scope) != SUCCESS)) {
					zval_ptr_dtor_nogc(&op1);
					ret = FAILURE;
					break;
				}
				zval_ptr_dtor_nogc(&op1);
			}
			break;
		// ing3, 操作符 “??” 连接的两个表达式
		case ZEND_AST_COALESCE:
			// expr1 计算出错
			if (UNEXPECTED(zend_ast_evaluate(&op1, ast->child[0], scope) != SUCCESS)) {
				ret = FAILURE;
				break;
			}
			// 如果 expr1 返回值不是 null
			if (Z_TYPE(op1) > IS_NULL) {
				*result = op1;
			// 只有 null 时，尝试 expr2 ，其他情况均不尝试
			} else {
				if (UNEXPECTED(zend_ast_evaluate(result, ast->child[1], scope) != SUCCESS)) {
					zval_ptr_dtor_nogc(&op1);
					ret = FAILURE;
					break;
				}
				zval_ptr_dtor_nogc(&op1);
			}
			break;
		// ? , 操作符+~
		case ZEND_AST_UNARY_PLUS:
			// 先计算 expr1
			if (UNEXPECTED(zend_ast_evaluate(&op2, ast->child[0], scope) != SUCCESS)) {
				ret = FAILURE;
			} else {
				ZVAL_LONG(&op1, 0);
				ret = add_function(result, &op1, &op2);
				zval_ptr_dtor_nogc(&op2);
			}
			break;
		// ？ 操作符 -~
		case ZEND_AST_UNARY_MINUS:
			if (UNEXPECTED(zend_ast_evaluate(&op2, ast->child[0], scope) != SUCCESS)) {
				ret = FAILURE;
			} else {
				ZVAL_LONG(&op1, 0);
				ret = sub_function(result, &op1, &op2);
				zval_ptr_dtor_nogc(&op2);
			}
			break;
		// 键值对列表（已经创建成list了），用于定义和遍历数组
		case ZEND_AST_ARRAY:
			{
				uint32_t i;
				zend_ast_list *list = zend_ast_get_list(ast);
				// 没有子元素，设置成空数组
				if (!list->children) {
					ZVAL_EMPTY_ARRAY(result);
					break;
				}
				// 初始化数组
				array_init(result);
				// 遍历所有子元素
				for (i = 0; i < list->children; i++) {
					zend_ast *elem = list->child[i];
					// 如果有解压操作符 (...)
					if (elem->kind == ZEND_AST_UNPACK) {
						if (UNEXPECTED(zend_ast_evaluate(&op1, elem->child[0], scope) != SUCCESS)) {
							zval_ptr_dtor_nogc(result);
							return FAILURE;
						}
						if (UNEXPECTED(zend_ast_add_unpacked_element(result, &op1) != SUCCESS)) {
							zval_ptr_dtor_nogc(&op1);
							zval_ptr_dtor_nogc(result);
							return FAILURE;
						}
						zval_ptr_dtor_nogc(&op1);
						continue;
					}
					// 
					if (elem->child[1]) {
						if (UNEXPECTED(zend_ast_evaluate(&op1, elem->child[1], scope) != SUCCESS)) {
							zval_ptr_dtor_nogc(result);
							return FAILURE;
						}
					} else {
						ZVAL_UNDEF(&op1);
					}
					if (UNEXPECTED(zend_ast_evaluate(&op2, elem->child[0], scope) != SUCCESS)) {
						zval_ptr_dtor_nogc(&op1);
						zval_ptr_dtor_nogc(result);
						return FAILURE;
					}
					if (UNEXPECTED(zend_ast_add_array_element(result, &op1, &op2) != SUCCESS)) {
						zval_ptr_dtor_nogc(&op1);
						zval_ptr_dtor_nogc(&op2);
						zval_ptr_dtor_nogc(result);
						return FAILURE;
					}
				}
			}
			break;
		// 带索引的变量 $a[1] 或 ${a}
		case ZEND_AST_DIM:
			if (ast->child[1] == NULL) {
				zend_error_noreturn(E_COMPILE_ERROR, "Cannot use [] for reading");
			}

			bool short_circuited;
			// ex
			if (UNEXPECTED(zend_ast_evaluate_ex(&op1, ast->child[0], scope, &short_circuited) != SUCCESS)) {
				ret = FAILURE;
				break;
			}
			if (short_circuited) {
				// 情况1
				*short_circuited_ptr = true;
				ZVAL_NULL(result);
				return SUCCESS;
			}

			// DIM on objects is disallowed because it allows executing arbitrary expressions
			if (Z_TYPE(op1) == IS_OBJECT) {
				zval_ptr_dtor_nogc(&op1);
				zend_throw_error(NULL, "Cannot use [] on objects in constant expression");
				ret = FAILURE;
				break;
			}

			if (UNEXPECTED(zend_ast_evaluate(&op2, ast->child[1], scope) != SUCCESS)) {
				zval_ptr_dtor_nogc(&op1);
				ret = FAILURE;
				break;
			}

			zend_fetch_dimension_const(result, &op1, &op2, (ast->attr & ZEND_DIM_IS) ? BP_VAR_IS : BP_VAR_R);

			zval_ptr_dtor_nogc(&op1);
			zval_ptr_dtor_nogc(&op2);
			if (UNEXPECTED(EG(exception))) {
				return FAILURE;
			}

			break;
		// enum 初始化时用的临时节点
		case ZEND_AST_CONST_ENUM_INIT:
		{
			// Preloading will attempt to resolve constants but objects can't be stored in shm
			// Aborting here to store the const AST instead
			if (CG(in_compilation)) {
				return FAILURE;
			}

			zend_ast *class_name_ast = ast->child[0];
			zend_string *class_name = zend_ast_get_str(class_name_ast);

			zend_ast *case_name_ast = ast->child[1];
			zend_string *case_name = zend_ast_get_str(case_name_ast);

			zend_ast *case_value_ast = ast->child[2];

			zval case_value_zv;
			ZVAL_UNDEF(&case_value_zv);
			if (case_value_ast != NULL) {
				if (UNEXPECTED(zend_ast_evaluate(&case_value_zv, case_value_ast, scope) != SUCCESS)) {
					return FAILURE;
				}
			}

			zend_class_entry *ce = zend_lookup_class(class_name);
			zend_enum_new(result, ce, case_name, case_value_ast != NULL ? &case_value_zv : NULL);
			zval_ptr_dtor_nogc(&case_value_zv);
			break;
		}
		// 类常量
		case ZEND_AST_CLASS_CONST:
		{
			zend_string *class_name = zend_ast_get_str(ast->child[0]);
			zend_string *const_name = zend_ast_get_str(ast->child[1]);

			zend_string *previous_filename;
			zend_long previous_lineno;
			if (scope) {
				previous_filename = EG(filename_override);
				previous_lineno = EG(lineno_override);
				EG(filename_override) = scope->info.user.filename;
				EG(lineno_override) = zend_ast_get_lineno(ast);
			}
			zval *zv = zend_get_class_constant_ex(class_name, const_name, scope, ast->attr);
			if (scope) {
				EG(filename_override) = previous_filename;
				EG(lineno_override) = previous_lineno;
			}

			if (UNEXPECTED(zv == NULL)) {
				ZVAL_UNDEF(result);
				return FAILURE;
			}
			ZVAL_COPY_OR_DUP(result, zv);
			break;
		}
		// new 语句
		case ZEND_AST_NEW:
		{
			zend_class_entry *ce = zend_ast_fetch_class(ast->child[0], scope);
			if (!ce) {
				return FAILURE;
			}

			if (object_init_ex(result, ce) != SUCCESS) {
				return FAILURE;
			}

			zend_ast_list *args_ast = zend_ast_get_list(ast->child[1]);
			if (args_ast->attr) {
				/* Has named arguments. */
				HashTable *args = zend_new_array(args_ast->children);
				for (uint32_t i = 0; i < args_ast->children; i++) {
					zend_ast *arg_ast = args_ast->child[i];
					zend_string *name = NULL;
					zval arg;
					if (arg_ast->kind == ZEND_AST_NAMED_ARG) {
						name = zend_ast_get_str(arg_ast->child[0]);
						arg_ast = arg_ast->child[1];
					}
					if (zend_ast_evaluate(&arg, arg_ast, scope) == FAILURE) {
						zend_array_destroy(args);
						zval_ptr_dtor(result);
						return FAILURE;
					}
					if (name) {
						if (!zend_hash_add(args, name, &arg)) {
							zend_throw_error(NULL,
								"Named parameter $%s overwrites previous argument",
								ZSTR_VAL(name));
							zend_array_destroy(args);
							zval_ptr_dtor(result);
							return FAILURE;
						}
					} else {
						zend_hash_next_index_insert(args, &arg);
					}
				}

				zend_function *ctor = Z_OBJ_HT_P(result)->get_constructor(Z_OBJ_P(result));
				if (ctor) {
					zend_call_known_function(
						ctor, Z_OBJ_P(result), Z_OBJCE_P(result), NULL, 0, NULL, args);
				}

				zend_array_destroy(args);
			} else {
				ALLOCA_FLAG(use_heap)
				zval *args = do_alloca(sizeof(zval) * args_ast->children, use_heap);
				for (uint32_t i = 0; i < args_ast->children; i++) {
					if (zend_ast_evaluate(&args[i], args_ast->child[i], scope) == FAILURE) {
						for (uint32_t j = 0; j < i; j++) {
							zval_ptr_dtor(&args[j]);
						}
						free_alloca(args, use_heap);
						zval_ptr_dtor(result);
						return FAILURE;
					}
				}

				zend_function *ctor = Z_OBJ_HT_P(result)->get_constructor(Z_OBJ_P(result));
				if (ctor) {
					zend_call_known_instance_method(
						ctor, Z_OBJ_P(result), NULL, args_ast->children, args);
				}

				for (uint32_t i = 0; i < args_ast->children; i++) {
					zval_ptr_dtor(&args[i]);
				}
				free_alloca(args, use_heap);
			}

			if (EG(exception)) {
				zend_object_store_ctor_failed(Z_OBJ_P(result));
				zval_ptr_dtor(result);
				return FAILURE;
			}
			return SUCCESS;
		}
		// 操作符 -> , ?->，获取对象属性
		case ZEND_AST_PROP:
		case ZEND_AST_NULLSAFE_PROP:
		{
			bool short_circuited;
			// 如果计算 所属对象 返回错误
			if (UNEXPECTED(zend_ast_evaluate_ex(&op1, ast->child[0], scope, &short_circuited) != SUCCESS)) {
				return FAILURE;
			}
			// 如果 所属对象是 null返回 null， 继承 short_circuited 标记
			if (short_circuited) {
				// 情况2
				*short_circuited_ptr = true;
				ZVAL_NULL(result);
				return SUCCESS;
			}
			// 如果允许 所属对象为空，并且所属对象实际为空
			if (ast->kind == ZEND_AST_NULLSAFE_PROP && Z_TYPE(op1) == IS_NULL) {
				// 情况3， 只有这个是非递归设置 short_circuited_ptr
				*short_circuited_ptr = true;
				// 返回 Null
				ZVAL_NULL(result);
				return SUCCESS;
			}
			// 如果计算属性名出错，返回错误
			if (UNEXPECTED(zend_ast_evaluate(&op2, ast->child[1], scope) != SUCCESS)) {
				zval_ptr_dtor_nogc(&op1);
				return FAILURE;
			}
			// 如果属性名无法转成 字串，返回错误
			if (!try_convert_to_string(&op2)) {
				zval_ptr_dtor_nogc(&op1);
				zval_ptr_dtor_nogc(&op2);
				return FAILURE;
			}
			// 如果所属变量不是对象，返回错误
			if (Z_TYPE(op1) != IS_OBJECT) {
				// ing3, 抛错
				zend_wrong_property_read(&op1, &op2);

				zval_ptr_dtor_nogc(&op1);
				zval_ptr_dtor_nogc(&op2);

				ZVAL_NULL(result);
				return SUCCESS;
			}

			// ? enum ?
			zend_object *zobj = Z_OBJ(op1);
			if (!(zobj->ce->ce_flags & ZEND_ACC_ENUM)) {
				zend_throw_error(NULL, "Fetching properties on non-enums in constant expressions is not allowed");
				zval_ptr_dtor_nogc(&op1);
				zval_ptr_dtor_nogc(&op2);
				return FAILURE;
			}

			// 取得属性名
			zend_string *name = Z_STR(op2);
			// 获取属性
			zval *property_result = zend_read_property_ex(scope, zobj, name, 0, result);
			// 如果有exception 返回错误
			if (EG(exception)) {
				zval_ptr_dtor_nogc(&op1);
				zval_ptr_dtor_nogc(&op2);
				return FAILURE;
			}
			// 正常返回获取的内容
			if (result != property_result) {
				ZVAL_COPY(result, property_result);
			}
			zval_ptr_dtor_nogc(&op1);
			zval_ptr_dtor_nogc(&op2);
			return SUCCESS;
		}
		// 必须是已知运算逻辑，否则报错
		default:
			zend_throw_error(NULL, "Unsupported constant expression");
			ret = FAILURE;
	}
	return ret;
}

// clear, 计算语句, 只有 zend_ast_evaluate_ex 中递归调用
ZEND_API zend_result ZEND_FASTCALL zend_ast_evaluate(zval *result, zend_ast *ast, zend_class_entry *scope)
{
	// 回收 prop 名称为 null 的情况，后面会抛出一个warning
	bool short_circuited;
	return zend_ast_evaluate_ex(result, ast, scope, &short_circuited);
}

// clear, 递归求语句树的大小，只有 zend_ast_copy调用
static size_t ZEND_FASTCALL zend_ast_tree_size(zend_ast *ast)
{
	size_t size;
	// 如果是常量
	if (ast->kind == ZEND_AST_ZVAL || ast->kind == ZEND_AST_CONSTANT) {
		size = sizeof(zend_ast_zval);
	// 如果是语句列表
	} else if (zend_ast_is_list(ast)) {
		uint32_t i;
		zend_ast_list *list = zend_ast_get_list(ast);
		// 语句列表的子节点数量是动态的（2的幂）
		size = zend_ast_list_size(list->children);
		for (i = 0; i < list->children; i++) {
			if (list->child[i]) {
				// 递归添加所有子节点
				size += zend_ast_tree_size(list->child[i]);
			}
		}
	// 其他：是普通语句
	} else {
		// 普通语句的子节点数量是固定，并且可以直接获取
		uint32_t i, children = zend_ast_get_num_children(ast);

		size = zend_ast_size(children);
		for (i = 0; i < children; i++) {
			if (ast->child[i]) {
				// 递归添加所有子节点
				size += zend_ast_tree_size(ast->child[i]);
			}
		}
	}
	return size;
}

// ing3, 拷贝整个ast树到指定 buffer中
static void* ZEND_FASTCALL zend_ast_tree_copy(zend_ast *ast, void *buf)
{
	// 所有类型都先复制 kind 和 attr
	// 内部变量
	if (ast->kind == ZEND_AST_ZVAL) {
		zend_ast_zval *new = (zend_ast_zval*)buf;
		new->kind = ZEND_AST_ZVAL;
		new->attr = ast->attr;
		// 直接拷贝变量
		ZVAL_COPY(&new->val, zend_ast_get_zval(ast));
		// ?
		buf = (void*)((char*)buf + sizeof(zend_ast_zval));
	// 常量，这两个没有子元素
	} else if (ast->kind == ZEND_AST_CONSTANT) {
		zend_ast_zval *new = (zend_ast_zval*)buf;
		new->kind = ZEND_AST_CONSTANT;
		new->attr = ast->attr;
		// 拷贝字串
		ZVAL_STR_COPY(&new->val, zend_ast_get_constant_name(ast));
		// 
		buf = (void*)((char*)buf + sizeof(zend_ast_zval));
	// ast列表
	} else if (zend_ast_is_list(ast)) {
		zend_ast_list *list = zend_ast_get_list(ast);
		zend_ast_list *new = (zend_ast_list*)buf;
		uint32_t i;
		new->kind = list->kind;
		new->attr = list->attr;
		new->children = list->children;
		// ?
		buf = (void*)((char*)buf + zend_ast_list_size(list->children));
		// 递归复制所有子元素
		for (i = 0; i < list->children; i++) {
			if (list->child[i]) {
				new->child[i] = (zend_ast*)buf;
				buf = zend_ast_tree_copy(list->child[i], buf);
			} else {
				// 空子元素
				new->child[i] = NULL;
			}
		}
	// 其他所有类型
	} else {
		uint32_t i, children = zend_ast_get_num_children(ast);
		zend_ast *new = (zend_ast*)buf;
		new->kind = ast->kind;
		new->attr = ast->attr;
		new->lineno = ast->lineno;
		// ?
		buf = (void*)((char*)buf + zend_ast_size(children));
		// 递归复制所有子元素
		for (i = 0; i < children; i++) {
			if (ast->child[i]) {
				new->child[i] = (zend_ast*)buf;
				buf = zend_ast_tree_copy(ast->child[i], buf);
			} else {
				new->child[i] = NULL;
			}
		}
	}
	return buf;
}

// ing3, 复制 ast 对象， 
ZEND_API zend_ast_ref * ZEND_FASTCALL zend_ast_copy(zend_ast *ast)
{
	size_t tree_size;
	zend_ast_ref *ref;
	// 必须是有效对象
	ZEND_ASSERT(ast != NULL);
	// 整个ast 树的大小 + 一个引用计数器
	tree_size = zend_ast_tree_size(ast) + sizeof(zend_ast_ref);
	// 直接开辟内存
	ref = emalloc(tree_size);
	// 复制整个ast 树
	zend_ast_tree_copy(ast, GC_AST(ref));
	// 计 1 次引用
	GC_SET_REFCOUNT(ref, 1);
	// ?
	GC_TYPE_INFO(ref) = GC_CONSTANT_AST;
	return ref;
}

// ing4 销毁 ast 对象
ZEND_API void ZEND_FASTCALL zend_ast_destroy(zend_ast *ast)
{
// goto比递归的好处在节省资源，这属于深度优化了
tail_call:
	// null
	if (!ast) {
		return;
	}

	// 最少有一个 子元素的节点
	if (EXPECTED(ast->kind >= ZEND_AST_VAR)) {
		// 只留第一个子元素，其他的递归销毁
		uint32_t i, children = zend_ast_get_num_children(ast);

		for (i = 1; i < children; i++) {
			zend_ast_destroy(ast->child[i]);
		}
		// 切换到第一个上
		ast = ast->child[0];
		goto tail_call;
	// 特殊节点：内部变量
	} else if (EXPECTED(ast->kind == ZEND_AST_ZVAL)) {
		// 直接销毁
		zval_ptr_dtor_nogc(zend_ast_get_zval(ast));
	// 如果是 ast_list，列表型节点
	} else if (EXPECTED(zend_ast_is_list(ast))) {
		zend_ast_list *list = zend_ast_get_list(ast);
		if (list->children) {
			uint32_t i;
			// 只留第一个子元素，其他的递归销毁
			for (i = 1; i < list->children; i++) {
				zend_ast_destroy(list->child[i]);
			}
			ast = list->child[0];
			goto tail_call;
		}
	// 特殊节点：常量，调用 string_release 销毁
	} else if (EXPECTED(ast->kind == ZEND_AST_CONSTANT)) {
		zend_string_release_ex(zend_ast_get_constant_name(ast), 0);
	// 声明型节点
	} else if (EXPECTED(ast->kind >= ZEND_AST_FUNC_DECL)) {
		// 转成声明型结构
		zend_ast_decl *decl = (zend_ast_decl *) ast;
		// 如果有名字，先销毁名字
		if (decl->name) {
		    zend_string_release_ex(decl->name, 0);
		}
		// 销毁注释
		if (decl->doc_comment) {
			zend_string_release_ex(decl->doc_comment, 0);
		}
		// 销毁前4个子元素，
		zend_ast_destroy(decl->child[0]);
		zend_ast_destroy(decl->child[1]);
		zend_ast_destroy(decl->child[2]);
		zend_ast_destroy(decl->child[3]);
		// 切换到第五个子元素
		ast = decl->child[4];
		goto tail_call;
	}
}

// clear 销毁带计数器的 ast对象,gc: global counter
ZEND_API void ZEND_FASTCALL zend_ast_ref_destroy(zend_ast_ref *ast)
{
	zend_ast_destroy(GC_AST(ast));
	efree(ast);
}

// clear, 对 ast 或者 ast_list 的每个子元素调用 fn
ZEND_API void zend_ast_apply(zend_ast *ast, zend_ast_apply_func fn, void *context) {
	if (zend_ast_is_list(ast)) {
		zend_ast_list *list = zend_ast_get_list(ast);
		uint32_t i;
		for (i = 0; i < list->children; ++i) {
			fn(&list->child[i], context);
		}
	} else {
		uint32_t i, children = zend_ast_get_num_children(ast);
		for (i = 0; i < children; ++i) {
			fn(&ast->child[i], context);
		}
	}
}

// 操作符优先顺序
/*
 * Operator Precedence
 * ====================
 * priority  associativity  operators
 * ----------------------------------
 *   10     left            include, include_once, eval, require, require_once
 *   20     left            ,
 *   30     left            or
 *   40     left            xor
 *   50     left            and
 *   60     right           print
 *   70     right           yield
 *   80     right           =>
 *   85     right           yield from
 *   90     right           = += -= *= /= .= %= &= |= ^= <<= >>= **=
 *  100     left            ? :
 *  110     right           ??
 *  120     left            ||
 *  130     left            &&
 *  140     left            |
 *  150     left            ^
 *  160     left            &
 *  170     non-associative == != === !==
 *  180     non-associative < <= > >= <=>
 *  185     left            .
 *  190     left            << >>
 *  200     left            + -
 *  210     left            * / %
 *  220     right           !
 *  230     non-associative instanceof
 *  240     right           + - ++ -- ~ (type) @
 *  250     right           **
 *  260     left            [
 *  270     non-associative clone new
 */

// export 是指导出 php 语句，主要是给 反射（refection） 用的
static ZEND_COLD void zend_ast_export_ex(smart_str *str, zend_ast *ast, int priority, int indent);

// 导出简单转义字符串，内部2处调用
static ZEND_COLD void zend_ast_export_str(smart_str *str, zend_string *s)
{
	size_t i;
	// 遍历s的每一个字符，转义所有的单引号(') 和反斜线(\)
	for (i = 0; i < ZSTR_LEN(s); i++) {
		unsigned char c = ZSTR_VAL(s)[i];
		if (c == '\'' || c == '\\') {
			smart_str_appendc(str, '\\');
			smart_str_appendc(str, c);
		} else {
			smart_str_appendc(str, c);
		}
	}
}

// 导出转义字符串
static ZEND_COLD void zend_ast_export_qstr(smart_str *str, char quote, zend_string *s)
{
	size_t i;

	for (i = 0; i < ZSTR_LEN(s); i++) {
		unsigned char c = ZSTR_VAL(s)[i];
		if (c < ' ') {
			switch (c) {
				case '\n':
					smart_str_appends(str, "\\n");
					break;
				case '\r':
					smart_str_appends(str, "\\r");
					break;
				case '\t':
					smart_str_appends(str, "\\t");
					break;
				case '\f':
					smart_str_appends(str, "\\f");
					break;
				case '\v':
					smart_str_appends(str, "\\v");
					break;
#ifdef ZEND_WIN32
				// #define VK_ESCAPE \'\\e\'
				case VK_ESCAPE:
#else
				case '\e':
#endif
					smart_str_appends(str, "\\e");
					break;
				default:
					smart_str_appends(str, "\\0");
					smart_str_appendc(str, '0' + (c / 8));
					smart_str_appendc(str, '0' + (c % 8));
					break;
			}
		} else {
			// 其他用来代替反斜线 \ 字符
			if (c == quote || c == '$' || c == '\\') {
				smart_str_appendc(str, '\\');
			}
			smart_str_appendc(str, c);
		}
	}
}

// 添加缩进
static ZEND_COLD void zend_ast_export_indent(smart_str *str, int indent)
{
	while (indent > 0) {
		smart_str_appends(str, "    ");
		indent--;
	}
}

// 导出名称
static ZEND_COLD void zend_ast_export_name(smart_str *str, zend_ast *ast, int priority, int indent)
{
	// 如果是内部变量
	if (ast->kind == ZEND_AST_ZVAL) {
		zval *zv = zend_ast_get_zval(ast);
		// 并且是string
		if (Z_TYPE_P(zv) == IS_STRING) {
			// 直接把string 追加在后面
			smart_str_append(str, Z_STR_P(zv));
			return;
		}
	}
	// core logic
	zend_ast_export_ex(str, ast, priority, indent);
}

// 导出 namespace 语句
static ZEND_COLD void zend_ast_export_ns_name(smart_str *str, zend_ast *ast, int priority, int indent)
{
	if (ast->kind == ZEND_AST_ZVAL) {
		zval *zv = zend_ast_get_zval(ast);

		if (Z_TYPE_P(zv) == IS_STRING) {
		    if (ast->attr == ZEND_NAME_FQ) {
				smart_str_appendc(str, '\\');
		    } else if (ast->attr == ZEND_NAME_RELATIVE) {
				smart_str_appends(str, "namespace\\");
		    }
			smart_str_append(str, Z_STR_P(zv));
			return;
		}
	}
	zend_ast_export_ex(str, ast, priority, indent);
}

// [0-9a-zA-Z_]或asci码 >=127 的字符 ，只有 zend_ast_var_needs_braces 调用
static ZEND_COLD bool zend_ast_valid_var_char(char ch)
{
	unsigned char c = (unsigned char)ch;

	if (c != '_' && c < 127 &&
	    (c < '0' || c > '9') &&
	    (c < 'A' || c > 'Z') &&
	    (c < 'a' || c > 'z')) {
		return 0;
	}
	return 1;
}

// 检查变量名，const 相当于 readonly 咯，防更改
static ZEND_COLD bool zend_ast_valid_var_name(const char *s, size_t len)
{
	unsigned char c;
	size_t i;
	// 不可以为空
	if (len == 0) {
		return 0;
	}
	// 首字母只能是 [a-zA-Z_] 或 ascill 码>=127的字符 , 否则无效
	c = (unsigned char)s[0];
	if (c != '_' && c < 127 &&
	    (c < 'A' || c > 'Z') &&
	    (c < 'a' || c > 'z')) {
		return 0;
	}
	// 其他字符必须是 [a-zA-Z0-9_] 或 ascill 码>=127的字符 , 否则无效
	for (i = 1; i < len; i++) {
		c = (unsigned char)s[i];
		if (c != '_' && c < 127 &&
		    (c < '0' || c > '9') &&
		    (c < 'A' || c > 'Z') &&
		    (c < 'a' || c > 'z')) {
			return 0;
		}
	}
	// ascill 码>=127, 有效
	return 1;
}

// 多了个[, [\[0-9a-zA-Z_]或asci码 >=127 的字符，一处调用
static ZEND_COLD bool zend_ast_var_needs_braces(char ch)
{
	return ch == '[' || zend_ast_valid_var_char(ch);
}

// 导出变量名
static ZEND_COLD void zend_ast_export_var(smart_str *str, zend_ast *ast, int priority, int indent)
{
	// 如果是内置变量
	if (ast->kind == ZEND_AST_ZVAL) {
		zval *zv = zend_ast_get_zval(ast);
		// 是string 类型并且是有效变量名
		if (Z_TYPE_P(zv) == IS_STRING &&
		    zend_ast_valid_var_name(Z_STRVAL_P(zv), Z_STRLEN_P(zv))) {
			// 追加变量内容
			smart_str_append(str, Z_STR_P(zv));
			return;
		}
	// 如果是php变量
	} else if (ast->kind == ZEND_AST_VAR) {
		zend_ast_export_ex(str, ast, 0, indent);
		return;
	}
	// 如果是新型变量 ${a} 这种
	smart_str_appendc(str, '{');
	zend_ast_export_name(str, ast, 0, indent);
	smart_str_appendc(str, '}');
}

// 导出列表，简单组合
static ZEND_COLD void zend_ast_export_list(smart_str *str, zend_ast_list *list, bool separator, int priority, int indent)
{
	uint32_t i = 0;

	while (i < list->children) {
		// 如果不是第一个，并且要求分隔，添加分隔符（，）
		if (i != 0 && separator) {
			smart_str_appends(str, ", ");
		}
		// 添加列表元素
		zend_ast_export_ex(str, list->child[i], priority, indent);
		i++;
	}
}

// ing4, 导出添加引号或反引号的字串，内部2次引用：双引号和反引号
static ZEND_COLD void zend_ast_export_encaps_list(smart_str *str, char quote, zend_ast_list *list, int indent)
{
	uint32_t i = 0;
	zend_ast *ast;

	while (i < list->children) {
		ast = list->child[i];
		// 内部变量
		if (ast->kind == ZEND_AST_ZVAL) {
			zval *zv = zend_ast_get_zval(ast);
			// 类型必须是string
			ZEND_ASSERT(Z_TYPE_P(zv) == IS_STRING);
			zend_ast_export_qstr(str, quote, Z_STR_P(zv));
		// php 变量 ?
		} else if (ast->kind == ZEND_AST_VAR &&
					// 第一个子元素是内部变量
		           ast->child[0]->kind == ZEND_AST_ZVAL &&
				   // 只有一个子元素 或 第二个子元素不是内置变量 或第二个变量不是有效变量名
		           (i + 1 == list->children ||
		            list->child[i + 1]->kind != ZEND_AST_ZVAL ||
					// 排除 [\[0-9a-zA-Z_]或asci码 >=127 的字符
		            !zend_ast_var_needs_braces(
		                *Z_STRVAL_P(
		                    zend_ast_get_zval(list->child[i + 1]))))) {
			zend_ast_export_ex(str, ast, 0, indent);
		// 新变量
		} else {
			smart_str_appendc(str, '{');
			zend_ast_export_ex(str, ast, 0, indent);
			smart_str_appendc(str, '}');
		}
		i++;
	}
}

// zend_ast_export_name_list，zend_ast_export_catch_name_list
static ZEND_COLD void zend_ast_export_name_list_ex(smart_str *str, zend_ast_list *list, int indent, const char *separator)
{
	uint32_t i = 0;

	while (i < list->children) {
		// 不是第一个，加分隔符
		if (i != 0) {
			smart_str_appends(str, separator);
		}
		// 名字
		zend_ast_export_name(str, list->child[i], 0, indent);
		i++;
	}
}

// 类名列表中的每一个类名，use trait，extends, implements 时用到的
#define zend_ast_export_name_list(s, l, i) zend_ast_export_name_list_ex(s, l, i, ", ")
// catch 语句里的类名列表 
#define zend_ast_export_catch_name_list(s, l, i) zend_ast_export_name_list_ex(s, l, i, "|")

// 导出变量列表 ast_list
static ZEND_COLD void zend_ast_export_var_list(smart_str *str, zend_ast_list *list, int indent)
{
	uint32_t i = 0;
	//
	while (i < list->children) {
		// 不是第一个，前面加,
		if (i != 0) {
			smart_str_appends(str, ", ");
		}
		// 如果是引用，前面加&
		if (list->child[i]->attr & ZEND_BIND_REF) {
			smart_str_appendc(str, '&');
		}
		// 变量前添加 $
		smart_str_appendc(str, '$');
		// 添加名称
		zend_ast_export_name(str, list->child[i], 20, indent);
		i++;
	}
}

// ing4, 导出代码块（任意行数）
static ZEND_COLD void zend_ast_export_stmt(smart_str *str, zend_ast *ast, int indent)
{
	// 空
	if (!ast) {
		return;
	}

	// 语句列表或 trait 引用
	if (ast->kind == ZEND_AST_STMT_LIST ||
	    ast->kind == ZEND_AST_TRAIT_ADAPTATIONS) {
		zend_ast_list *list = (zend_ast_list*)ast;
		uint32_t i = 0;
		// 递归导出每一个子元素
		while (i < list->children) {
			ast = list->child[i];
			zend_ast_export_stmt(str, ast, indent);
			i++;
		}
	// 其他, 执行导出
	} else {
		zend_ast_export_indent(str, indent);
		zend_ast_export_ex(str, ast, 0, indent);
		// goto结点，if,switch,while,try,for,foreach,
		// 函数定义,类和方法定义，trait,命名空间和declare语句段，跳出。
		switch (ast->kind) {
			case ZEND_AST_LABEL:
			case ZEND_AST_IF:
			case ZEND_AST_SWITCH:
			case ZEND_AST_WHILE:
			case ZEND_AST_TRY:
			case ZEND_AST_FOR:
			case ZEND_AST_FOREACH:
			case ZEND_AST_FUNC_DECL:
			case ZEND_AST_METHOD:
			case ZEND_AST_CLASS:
			case ZEND_AST_USE_TRAIT:
			case ZEND_AST_NAMESPACE:
			case ZEND_AST_DECLARE:
				break;
			// 其他语句后面加;
			default:
				smart_str_appendc(str, ';');
				break;
		}		
		smart_str_appendc(str, '\n');
	}
}

// ing4, 导出 if 语句, ast_list 类型
static ZEND_COLD void zend_ast_export_if_stmt(smart_str *str, zend_ast_list *list, int indent)
{
	uint32_t i;
	zend_ast *ast;

tail_call:
	i = 0;
	// 遍历所有子元素
	while (i < list->children) {
		ast = list->child[i];
		// 必须要是if语句
		ZEND_ASSERT(ast->kind == ZEND_AST_IF_ELEM);
		// 如果第一个子元素有效，说明带条件
		if (ast->child[0]) {
			// 第一节，if
			if (i == 0) {
				smart_str_appends(str, "if (");
			// 其他节, elseif
			} else {
				zend_ast_export_indent(str, indent);
				smart_str_appends(str, "} elseif (");
			}
			// 条件语句
			zend_ast_export_ex(str, ast->child[0], 0, indent);
			smart_str_appends(str, ") {\n");
			// 代码块
			zend_ast_export_stmt(str, ast->child[1], indent + 1);
		} else {
			zend_ast_export_indent(str, indent);
			smart_str_appends(str, "} else ");
			if (ast->child[1] && ast->child[1]->kind == ZEND_AST_IF) {
				list = (zend_ast_list*)ast->child[1];
				goto tail_call;
			} else {
				smart_str_appends(str, "{\n");
				zend_ast_export_stmt(str, ast->child[1], indent + 1);
			}
		}
		i++;
	}
	zend_ast_export_indent(str, indent);
	smart_str_appendc(str, '}');
}

// ing4, 导出php变量 语句
static ZEND_COLD void zend_ast_export_zval(smart_str *str, zval *zv, int priority, int indent)
{
	// 获取被引用对象？
	ZVAL_DEREF(zv);
	// 按类型操作
	switch (Z_TYPE_P(zv)) {
		// 基本类型
		case IS_NULL:
			smart_str_appends(str, "null");
			break;
		case IS_FALSE:
			smart_str_appends(str, "false");
			break;
		case IS_TRUE:
			smart_str_appends(str, "true");
			break;
		case IS_LONG:
			smart_str_append_long(str, Z_LVAL_P(zv));
			break;
		case IS_DOUBLE:
			smart_str_append_double(
				str, Z_DVAL_P(zv), (int) EG(precision), /* zero_fraction */ false);
			break;
		case IS_STRING:
			// 字串前后添加 ' 单引号
			smart_str_appendc(str, '\'');
			zend_ast_export_str(str, Z_STR_P(zv));
			smart_str_appendc(str, '\'');
			break;
		// 数组
		case IS_ARRAY: {
			zend_long idx;
			zend_string *key;
			zval *val;
			bool first = true;
			smart_str_appendc(str, '[');
			// 转成数组，遍历数组。idx: index
			ZEND_HASH_FOREACH_KEY_VAL(Z_ARRVAL_P(zv), idx, key, val) {
				// 不是第一个，添加，
				if (first) {
					first = false;
				} else {
					smart_str_appends(str, ", ");
				}
				// 如果有key，添加 单引号（'） 包裹的 key
				if (key) {
					smart_str_appendc(str, '\'');
					zend_ast_export_str(str, key);
					smart_str_appends(str, "' => ");
				// 否则使用索引号
				} else {
					smart_str_append_long(str, idx);
					smart_str_appends(str, " => ");
				}
				// 添加值
				zend_ast_export_zval(str, val, 0, indent);
			} ZEND_HASH_FOREACH_END();
			smart_str_appendc(str, ']');
			break;
		}
		// 导出常量语句
		case IS_CONSTANT_AST:
			zend_ast_export_ex(str, Z_ASTVAL_P(zv), priority, indent);
			break;
		EMPTY_SWITCH_DEFAULT_CASE();
	}
}

// ing4, 导出类声明， 不带修饰符（ abstract等）
static ZEND_COLD void zend_ast_export_class_no_header(smart_str *str, zend_ast_decl *decl, int indent) {
	// extends 继承
	if (decl->child[0]) {
		smart_str_appends(str, " extends ");
		zend_ast_export_ns_name(str, decl->child[0], 0, indent);
	}
	// implements 接口实现
	if (decl->child[1]) {
		smart_str_appends(str, " implements ");
		zend_ast_export_ex(str, decl->child[1], 0, indent);
	}
	smart_str_appends(str, " {\n");
	// 获取代码块
	zend_ast_export_stmt(str, decl->child[2], indent + 1);
	// 添加代码块
	zend_ast_export_indent(str, indent);
	smart_str_appends(str, "}");
}

// ing4, 导出一组修饰属性
static ZEND_COLD void zend_ast_export_attribute_group(smart_str *str, zend_ast *ast, int indent) {
	zend_ast_list *list = zend_ast_get_list(ast);
	for (uint32_t i = 0; i < list->children; i++) {
		zend_ast *attr = list->child[i];
		// 不是第一个，加分隔符
		if (i) {
			smart_str_appends(str, ", ");
		}
		// 第一个子元素的 ns_name
		zend_ast_export_ns_name(str, attr->child[0], 0, indent);
		// 如果有第二个子元素 ，添加成参数
		if (attr->child[1]) {
			smart_str_appendc(str, '(');
			zend_ast_export_ex(str, attr->child[1], 0, indent);
			smart_str_appendc(str, ')');
		}
	}
}

// ing4, 导出修饰属性
static ZEND_COLD void zend_ast_export_attributes(smart_str *str, zend_ast *ast, int indent, bool newlines) {
	zend_ast_list *list = zend_ast_get_list(ast);
	uint32_t i;

	for (i = 0; i < list->children; i++) {
		smart_str_appends(str, "#[");
		// 导出一组修饰属性
		zend_ast_export_attribute_group(str, list->child[i], indent);
		smart_str_appends(str, "]");

		if (newlines) {
			smart_str_appendc(str, '\n');
			zend_ast_export_indent(str, indent);
		} else {
			smart_str_appendc(str, ' ');
		}
	}
}

// ing4, 导出可见性
static ZEND_COLD void zend_ast_export_visibility(smart_str *str, uint32_t flags) {
	if (flags & ZEND_ACC_PUBLIC) {
		smart_str_appends(str, "public ");
	} else if (flags & ZEND_ACC_PROTECTED) {
		smart_str_appends(str, "protected ");
	} else if (flags & ZEND_ACC_PRIVATE) {
		smart_str_appends(str, "private ");
	}
}

// ing4, 导出类型
static ZEND_COLD void zend_ast_export_type(smart_str *str, zend_ast *ast, int indent) {
	// 多个类型之间 |
	if (ast->kind == ZEND_AST_TYPE_UNION) {
		zend_ast_list *list = zend_ast_get_list(ast);
		for (uint32_t i = 0; i < list->children; i++) {
			if (i != 0) {
				smart_str_appendc(str, '|');
			}
			zend_ast_export_type(str, list->child[i], indent);
		}
		return;
	}
	// 多个类型之间 &
	if (ast->kind == ZEND_AST_TYPE_INTERSECTION) {
		zend_ast_list *list = zend_ast_get_list(ast);
		for (uint32_t i = 0; i < list->children; i++) {
			if (i != 0) {
				smart_str_appendc(str, '&');
			}
			zend_ast_export_type(str, list->child[i], indent);
		}
		return;
	}
	// 如果允许null
	if (ast->attr & ZEND_TYPE_NULLABLE) {
		smart_str_appendc(str, '?');
	}
	// 导出 namespace
	zend_ast_export_ns_name(str, ast, 0, indent);
}

// 这种函数没改变作用域，调用的goto标签在父函数里，
// 有重复的代码行，又没有必要声明成新函数，可以用这个方法
// 只在zend_ast_export_ex里面用，其他地方不用
#define BINARY_OP(_op, _p, _pl, _pr) do { \
		op = _op; \
		p = _p; \
		pl = _pl; \
		pr = _pr; \
		goto binary_op; \
	} while (0)

#define PREFIX_OP(_op, _p, _pl) do { \
		op = _op; \
		p = _p; \
		pl = _pl; \
		goto prefix_op; \
	} while (0)

#define FUNC_OP(_op) do { \
		op = _op; \
		goto func_op; \
	} while (0)

#define POSTFIX_OP(_op, _p, _pl) do { \
		op = _op; \
		p = _p; \
		pl = _pl; \
		goto postfix_op; \
	} while (0)

// global， unset，return， echo， throw， break， continue， 关键字
#define APPEND_NODE_1(_op) do { \
		op = _op; \
		goto append_node_1; \
	} while (0)

#define APPEND_STR(_op) do { \
		op = _op; \
		goto append_str; \
	} while (0)

#define APPEND_DEFAULT_VALUE(n) do { \
		p = n; \
		goto append_default_value; \
	} while (0)

// 执行export 只有内部调用，调用最多，58次，内部46次，这个是核心逻辑,820行
// str是程序代码，ast是语句元素，priority 用于转义字符，indent缩进
static ZEND_COLD void zend_ast_export_ex(smart_str *str, zend_ast *ast, int priority, int indent)
{
	zend_ast_decl *decl;
	int p, pl, pr;
	const char *op;

tail_call:
	if (!ast) {
		return;
	}
	// 按类型操作，类型说明已经在 zend_ast.h 里
	switch (ast->kind) {
		/* special nodes */
		// ing1, 内置变量
		case ZEND_AST_ZVAL:
			zend_ast_export_zval(str, zend_ast_get_zval(ast), priority, indent);
			break;
		// ing1, compile 时创建的常量 ,zend_ast_create_constant, zend_ast_tree_copy 两个方法赋值
		case ZEND_AST_CONSTANT: {
			zend_string *name = zend_ast_get_constant_name(ast);
			smart_str_appendl(str, ZSTR_VAL(name), ZSTR_LEN(name));
			break;
		}
		// 魔术常量
		case ZEND_AST_CONSTANT_CLASS:
			smart_str_appendl(str, "__CLASS__", sizeof("__CLASS__")-1);
			break;
		// compile时创建的 node
		case ZEND_AST_ZNODE:
			/* This AST kind is only used for temporary nodes during compilation */
			ZEND_UNREACHABLE();
			break;

		/* declaration nodes */
		// ing3, 函数，闭包，类方法：
		case ZEND_AST_FUNC_DECL:
		case ZEND_AST_CLOSURE:
		case ZEND_AST_ARROW_FUNC:
		case ZEND_AST_METHOD:
			decl = (zend_ast_decl *) ast;
			// 修饰属性
			if (decl->child[4]) {
				bool newlines = !(ast->kind == ZEND_AST_CLOSURE || ast->kind == ZEND_AST_ARROW_FUNC);
				zend_ast_export_attributes(str, decl->child[4], indent, newlines);
			}
			// public, protected, private
			zend_ast_export_visibility(str, decl->flags);
			// 修饰符
			if (decl->flags & ZEND_ACC_STATIC) {
				smart_str_appends(str, "static ");
			}
			if (decl->flags & ZEND_ACC_ABSTRACT) {
				smart_str_appends(str, "abstract ");
			}
			if (decl->flags & ZEND_ACC_FINAL) {
				smart_str_appends(str, "final ");
			}
			// 函数开头
			if (decl->kind == ZEND_AST_ARROW_FUNC) {
				smart_str_appends(str, "fn");
			} else {
				smart_str_appends(str, "function ");
			}
			// 返回引用
			if (decl->flags & ZEND_ACC_RETURN_REFERENCE) {
				smart_str_appendc(str, '&');
			}
			// 如果不是闭包也不是 fn()
			if (ast->kind != ZEND_AST_CLOSURE && ast->kind != ZEND_AST_ARROW_FUNC) {
				// 添加函数名
				smart_str_appendl(str, ZSTR_VAL(decl->name), ZSTR_LEN(decl->name));
			}
			// 参数列表
			smart_str_appendc(str, '(');
			zend_ast_export_ex(str, decl->child[0], 0, indent);
			smart_str_appendc(str, ')');
			// 匿名函数的 use() 部分
			zend_ast_export_ex(str, decl->child[1], 0, indent);
			// 返回值类型
			if (decl->child[3]) {
				smart_str_appends(str, ": ");
				zend_ast_export_type(str, decl->child[3], indent);
			}
			// 代码块
			if (decl->child[2]) {
				// fn() 语句
				if (decl->kind == ZEND_AST_ARROW_FUNC) {
					zend_ast *body = decl->child[2];
					// 如果有return 关键字，取return 后面的 expr 
					if (body->kind == ZEND_AST_RETURN) {
						body = body->child[0];
					}
					// 添加 => 内容
					smart_str_appends(str, " => ");
					zend_ast_export_ex(str, body, 0, indent);
					break;
				}
				// 普通函数的业务逻辑
				smart_str_appends(str, " {\n");
				// 增加缩进
				zend_ast_export_stmt(str, decl->child[2], indent + 1);
				zend_ast_export_indent(str, indent);
				smart_str_appendc(str, '}');
				// 如果不是闭包，添加回车（;在后面）
				if (ast->kind != ZEND_AST_CLOSURE) {
					smart_str_appendc(str, '\n');
				}
			// 抽象方法
			} else {
				smart_str_appends(str, ";\n");
			}
			break;
		// 定义class，trait，interface，enum，匿名类
		case ZEND_AST_CLASS:
			decl = (zend_ast_decl *) ast;
			// 修饰属性
			if (decl->child[3]) {
				zend_ast_export_attributes(str, decl->child[3], indent, 1);
			}
			// 其他类型
			if (decl->flags & ZEND_ACC_INTERFACE) {
				smart_str_appends(str, "interface ");
			} else if (decl->flags & ZEND_ACC_TRAIT) {
				smart_str_appends(str, "trait ");
			} else if (decl->flags & ZEND_ACC_ENUM) {
				smart_str_appends(str, "enum ");
			// class
			} else {
				// 类修饰符
				if (decl->flags & ZEND_ACC_EXPLICIT_ABSTRACT_CLASS) {
					smart_str_appends(str, "abstract ");
				}
				if (decl->flags & ZEND_ACC_FINAL) {
					smart_str_appends(str, "final ");
				}
				if (decl->flags & ZEND_ACC_READONLY_CLASS) {
					smart_str_appends(str, "readonly ");
				}
				smart_str_appends(str, "class ");
			}
			// 名称
			smart_str_appendl(str, ZSTR_VAL(decl->name), ZSTR_LEN(decl->name));
			// enum 指定类型
			if (decl->flags & ZEND_ACC_ENUM && decl->child[4]) {
				smart_str_appends(str, ": ");
				zend_ast_export_type(str, decl->child[4], indent);
			}
			// 不带修饰属性和修饰符的class 定义（上面几个类型通用）
			zend_ast_export_class_no_header(str, decl, indent);
			smart_str_appendc(str, '\n');
			break;

		/* list nodes */
		// 参数列表，
		case ZEND_AST_ARG_LIST:
		// 表达式列表
		case ZEND_AST_EXPR_LIST:
		// 闭包里use的变量
		case ZEND_AST_PARAM_LIST:
simple_list:
			// 简单组合在一起
			zend_ast_export_list(str, (zend_ast_list*)ast, 1, 20, indent);
			break;
		// array
		case ZEND_AST_ARRAY:
			smart_str_appendc(str, '[');
			zend_ast_export_list(str, (zend_ast_list*)ast, 1, 20, indent);
			smart_str_appendc(str, ']');
			break;
		// 双引号引用的内容，
		case ZEND_AST_ENCAPS_LIST:
			smart_str_appendc(str, '"');
			zend_ast_export_encaps_list(str, '"', (zend_ast_list*)ast, indent);
			smart_str_appendc(str, '"');
			break;
		// 最外层文档，内部代码块，unset语句，global语句,static 语句，echo语句，
		// 类，interface, trait , enum,中的业务逻辑，都用它包装
		case ZEND_AST_STMT_LIST:
		// 一条 use trait 语句后面{} 里的每一段内容
		case ZEND_AST_TRAIT_ADAPTATIONS:
			zend_ast_export_stmt(str, ast, indent);
			break;
		// if语句
		case ZEND_AST_IF:
			zend_ast_export_if_stmt(str, (zend_ast_list*)ast, indent);
			break;
		// switch,catch,macth语句里的arm
		case ZEND_AST_SWITCH_LIST:
		case ZEND_AST_CATCH_LIST:
		case ZEND_AST_MATCH_ARM_LIST:
			zend_ast_export_list(str, (zend_ast_list*)ast, 0, 0, indent);
			break;
		// 闭包的 use 语句
		case ZEND_AST_CLOSURE_USES:
			smart_str_appends(str, " use(");
			zend_ast_export_var_list(str, (zend_ast_list*)ast, indent);
			smart_str_appendc(str, ')');
			break;
		// ? 多个类属性 
		case ZEND_AST_PROP_GROUP: {
			// 第一个元素： static
			zend_ast *type_ast = ast->child[0];
			// 第二个是: 变量名
			zend_ast *prop_ast = ast->child[1];
			
			// ? 第三个，初始化时没有
			if (ast->child[2]) {
				zend_ast_export_attributes(str, ast->child[2], indent, 1);
			}

			zend_ast_export_visibility(str, ast->attr);

			if (ast->attr & ZEND_ACC_STATIC) {
				smart_str_appends(str, "static ");
			}
			if (ast->attr & ZEND_ACC_READONLY) {
				smart_str_appends(str, "readonly ");
			}

			if (type_ast) {
				zend_ast_export_type(str, type_ast, indent);
				smart_str_appendc(str, ' ');
			}

			ast = prop_ast;
			goto simple_list;
		}
		// 常量，多个
		case ZEND_AST_CONST_DECL:
			smart_str_appends(str, "const ");
			goto simple_list;
		// 类常量，多个
		case ZEND_AST_CLASS_CONST_GROUP:
			if (ast->child[1]) {
				zend_ast_export_attributes(str, ast->child[1], indent, 1);
			}

			zend_ast_export_visibility(str, ast->attr);
			smart_str_appends(str, "const ");

			ast = ast->child[0];

			goto simple_list;
		// 类名列表中的每一个类名，use trait，extends, implements 时用到的
		case ZEND_AST_NAME_LIST:
			zend_ast_export_name_list(str, (zend_ast_list*)ast, indent);
			break;
		// use 的每个类名
		case ZEND_AST_USE:
			smart_str_appends(str, "use ");
			// use 可以指定function 或者 const
			if (ast->attr == T_FUNCTION) {
				smart_str_appends(str, "function ");
			} else if (ast->attr == T_CONST) {
				smart_str_appends(str, "const ");
			}
			goto simple_list;

		// 魔术常量
		/* 0 child nodes */
		case ZEND_AST_MAGIC_CONST:
			switch (ast->attr) {
				case T_LINE:     APPEND_STR("__LINE__");
				case T_FILE:     APPEND_STR("__FILE__");
				case T_DIR:      APPEND_STR("__DIR__");
				case T_TRAIT_C:  APPEND_STR("__TRAIT__");
				case T_METHOD_C: APPEND_STR("__METHOD__");
				case T_FUNC_C:   APPEND_STR("__FUNCTION__");
				case T_NS_C:     APPEND_STR("__NAMESPACE__");
				case T_CLASS_C:  APPEND_STR("__CLASS__");
				EMPTY_SWITCH_DEFAULT_CASE();
			}
			break;
		// 变量类型或方法的返回类型，array,callable,static,mixed
		case ZEND_AST_TYPE:
			switch (ast->attr & ~ZEND_TYPE_NULLABLE) {
				case IS_ARRAY:    APPEND_STR("array");
				case IS_CALLABLE: APPEND_STR("callable");
				case IS_STATIC:   APPEND_STR("static");
				case IS_MIXED:    APPEND_STR("mixed");
				EMPTY_SWITCH_DEFAULT_CASE();
			}
			break;
			
		// 一个子元素的节点
		/* 1 child node */
		// ing3, PHP变量
		case ZEND_AST_VAR:
			smart_str_appendc(str, '$');
			zend_ast_export_var(str, ast->child[0], 0, indent);
			break;
		// ing3, 常量名
		case ZEND_AST_CONST:
			zend_ast_export_ns_name(str, ast->child[0], 0, indent);
			break;
		// ing3,
		case ZEND_AST_UNPACK:
			smart_str_appends(str, "...");
			ast = ast->child[0];
			goto tail_call;
		// +-
		case ZEND_AST_UNARY_PLUS:  PREFIX_OP("+", 240, 241);
		case ZEND_AST_UNARY_MINUS: PREFIX_OP("-", 240, 241);
		// 强转类型
		case ZEND_AST_CAST:
			switch (ast->attr) {
				case IS_NULL:      PREFIX_OP("(unset)",  240, 241);
				case _IS_BOOL:     PREFIX_OP("(bool)",   240, 241);
				case IS_LONG:      PREFIX_OP("(int)",    240, 241);
				case IS_DOUBLE:    PREFIX_OP("(double)", 240, 241);
				case IS_STRING:    PREFIX_OP("(string)", 240, 241);
				case IS_ARRAY:     PREFIX_OP("(array)",  240, 241);
				case IS_OBJECT:    PREFIX_OP("(object)", 240, 241);
				EMPTY_SWITCH_DEFAULT_CASE();
			}
			break;
		// ing3
		case ZEND_AST_EMPTY:
			FUNC_OP("empty");
		case ZEND_AST_ISSET:
			FUNC_OP("isset");
		case ZEND_AST_SILENCE:
			PREFIX_OP("@", 240, 241);
		// 调用shell
		case ZEND_AST_SHELL_EXEC:
			smart_str_appendc(str, '`');
			// 字串和变量的拼合
			if (ast->child[0]->kind == ZEND_AST_ENCAPS_LIST) {
				zend_ast_export_encaps_list(str, '`', (zend_ast_list*)ast->child[0], indent);
			} else {
				zval *zv;
				// 必须是内置变量
				ZEND_ASSERT(ast->child[0]->kind == ZEND_AST_ZVAL);
				zv = zend_ast_get_zval(ast->child[0]);
				ZEND_ASSERT(Z_TYPE_P(zv) == IS_STRING);
				zend_ast_export_qstr(str, '`', Z_STR_P(zv));
			}
			smart_str_appendc(str, '`');
			break;
		// ing3, clone
		case ZEND_AST_CLONE:
			PREFIX_OP("clone ", 270, 271);
		// ing3, exit
		case ZEND_AST_EXIT:
			// 如果有参数，当成函数
			if (ast->child[0]) {
				FUNC_OP("exit");
			// 否则，当成语句
			} else {
				APPEND_STR("exit");
			}
			break;
		// ing3, print
		case ZEND_AST_PRINT:
			PREFIX_OP("print ", 60, 61);
		// ing3, include, require, eval
		case ZEND_AST_INCLUDE_OR_EVAL:
			switch (ast->attr) {
				case ZEND_INCLUDE_ONCE: FUNC_OP("include_once");
				case ZEND_INCLUDE:      FUNC_OP("include");
				case ZEND_REQUIRE_ONCE: FUNC_OP("require_once");
				case ZEND_REQUIRE:      FUNC_OP("require");
				case ZEND_EVAL:         FUNC_OP("eval");
				EMPTY_SWITCH_DEFAULT_CASE();
			}
			break;
		// ing3, 二进制操作符 ~!
		case ZEND_AST_UNARY_OP:
			switch (ast->attr) {
				case ZEND_BW_NOT:   PREFIX_OP("~", 240, 241);
				case ZEND_BOOL_NOT: PREFIX_OP("!", 240, 241);
				EMPTY_SWITCH_DEFAULT_CASE();
			}
			break;
		// ing3, 
		case ZEND_AST_PRE_INC:
			PREFIX_OP("++", 240, 241);
		case ZEND_AST_PRE_DEC:
			PREFIX_OP("--", 240, 241);
		case ZEND_AST_POST_INC:
			POSTFIX_OP("++", 240, 241);
		case ZEND_AST_POST_DEC:
			POSTFIX_OP("--", 240, 241);
		// ing3,
		case ZEND_AST_GLOBAL:
			APPEND_NODE_1("global");
		// ing3,
		case ZEND_AST_UNSET:
			FUNC_OP("unset");
		// ing3,
		case ZEND_AST_RETURN:
			APPEND_NODE_1("return");
		// ing3, goto 跳转标签
		case ZEND_AST_LABEL:
			zend_ast_export_name(str, ast->child[0], 0, indent);
			smart_str_appendc(str, ':');
			break;
		// foreach 里value 传址
		case ZEND_AST_REF:
			smart_str_appendc(str, '&');
			ast = ast->child[0];
			goto tail_call;
		// ing3,
		case ZEND_AST_HALT_COMPILER:
			APPEND_STR("__HALT_COMPILER()");
		// ing3, echo, throw
		case ZEND_AST_ECHO:
			APPEND_NODE_1("echo");
		case ZEND_AST_THROW:
			APPEND_NODE_1("throw");
		// ing3, godo
		case ZEND_AST_GOTO:
			smart_str_appends(str, "goto ");
			zend_ast_export_name(str, ast->child[0], 0, indent);
			break;
		// ing3, break, continue
		case ZEND_AST_BREAK:
			APPEND_NODE_1("break");
		case ZEND_AST_CONTINUE:
			APPEND_NODE_1("continue");

		/* 2 child nodes */
		// ing3, 引用数组元素
		case ZEND_AST_DIM:
			zend_ast_export_ex(str, ast->child[0], 260, indent);
			smart_str_appendc(str, '[');
			if (ast->child[1]) {
				zend_ast_export_ex(str, ast->child[1], 0, indent);
			}
			smart_str_appendc(str, ']');
			break;
		// ing3, 对象的属性
		case ZEND_AST_PROP:
		case ZEND_AST_NULLSAFE_PROP:
			zend_ast_export_ex(str, ast->child[0], 0, indent);
			smart_str_appends(str, ast->kind == ZEND_AST_NULLSAFE_PROP ? "?->" : "->");
			zend_ast_export_var(str, ast->child[1], 0, indent);
			break;
		// ing3, 类的静态变量
		case ZEND_AST_STATIC_PROP:
			zend_ast_export_ns_name(str, ast->child[0], 0, indent);
			smart_str_appends(str, "::$");
			zend_ast_export_var(str, ast->child[1], 0, indent);
			break;
		// ing3, 调用函数或匿名函数
		case ZEND_AST_CALL:
			zend_ast_export_ns_name(str, ast->child[0], 0, indent);
			smart_str_appendc(str, '(');
			zend_ast_export_ex(str, ast->child[1], 0, indent);
			smart_str_appendc(str, ')');
			break;
		// ing3, 可伸缩形参
		case ZEND_AST_CALLABLE_CONVERT:
			smart_str_appends(str, "...");
			break;
		// ing3, 类常量
		case ZEND_AST_CLASS_CONST:
			zend_ast_export_ns_name(str, ast->child[0], 0, indent);
			smart_str_appends(str, "::");
			zend_ast_export_name(str, ast->child[1], 0, indent);
			break;
		// ing3, 通过 ::class 常量获取类名
		case ZEND_AST_CLASS_NAME:
			// 如果没有第一个子元素
			if (ast->child[0] == NULL) {
				/* The const expr representation stores the fetch type instead. */
				// self或parent
				switch (ast->attr) {
					case ZEND_FETCH_CLASS_SELF:
						smart_str_appends(str, "self");
						break;
					case ZEND_FETCH_CLASS_PARENT:
						smart_str_appends(str, "parent");
						break;
					EMPTY_SWITCH_DEFAULT_CASE()
				}
			// expr
			} else {
				zend_ast_export_ns_name(str, ast->child[0], 0, indent);
			}
			smart_str_appends(str, "::class");
			break;
		// ing3, 赋值或引用赋值
		case ZEND_AST_ASSIGN:            BINARY_OP(" = ",   90, 91, 90);
		case ZEND_AST_ASSIGN_REF:        BINARY_OP(" =& ",  90, 91, 90);
		// ing3, 一元运算符
		case ZEND_AST_ASSIGN_OP:
			switch (ast->attr) {
				case ZEND_ADD:    BINARY_OP(" += ",  90, 91, 90);
				case ZEND_SUB:    BINARY_OP(" -= ",  90, 91, 90);
				case ZEND_MUL:    BINARY_OP(" *= ",  90, 91, 90);
				case ZEND_DIV:    BINARY_OP(" /= ",  90, 91, 90);
				case ZEND_MOD:    BINARY_OP(" %= ",  90, 91, 90);
				case ZEND_SL:     BINARY_OP(" <<= ", 90, 91, 90);
				case ZEND_SR:     BINARY_OP(" >>= ", 90, 91, 90);
				case ZEND_CONCAT: BINARY_OP(" .= ",  90, 91, 90);
				case ZEND_BW_OR:  BINARY_OP(" |= ",  90, 91, 90);
				case ZEND_BW_AND: BINARY_OP(" &= ",  90, 91, 90);
				case ZEND_BW_XOR: BINARY_OP(" ^= ",  90, 91, 90);
				case ZEND_POW:    BINARY_OP(" **= ", 90, 91, 90);
				EMPTY_SWITCH_DEFAULT_CASE();
			}
			break;
		// ing3, 覆盖null变量
		case ZEND_AST_ASSIGN_COALESCE: BINARY_OP(" \?\?= ", 90, 91, 90);
		// ing3, 二进制操作符
		case ZEND_AST_BINARY_OP:
			switch (ast->attr) {
				case ZEND_ADD:                 BINARY_OP(" + ",   200, 200, 201);
				case ZEND_SUB:                 BINARY_OP(" - ",   200, 200, 201);
				case ZEND_MUL:                 BINARY_OP(" * ",   210, 210, 211);
				case ZEND_DIV:                 BINARY_OP(" / ",   210, 210, 211);
				case ZEND_MOD:                 BINARY_OP(" % ",   210, 210, 211);
				case ZEND_SL:                  BINARY_OP(" << ",  190, 190, 191);
				case ZEND_SR:                  BINARY_OP(" >> ",  190, 190, 191);
				case ZEND_CONCAT:              BINARY_OP(" . ",   185, 185, 186);
				case ZEND_BW_OR:               BINARY_OP(" | ",   140, 140, 141);
				case ZEND_BW_AND:              BINARY_OP(" & ",   160, 160, 161);
				case ZEND_BW_XOR:              BINARY_OP(" ^ ",   150, 150, 151);
				case ZEND_IS_IDENTICAL:        BINARY_OP(" === ", 170, 171, 171);
				case ZEND_IS_NOT_IDENTICAL:    BINARY_OP(" !== ", 170, 171, 171);
				case ZEND_IS_EQUAL:            BINARY_OP(" == ",  170, 171, 171);
				case ZEND_IS_NOT_EQUAL:        BINARY_OP(" != ",  170, 171, 171);
				case ZEND_IS_SMALLER:          BINARY_OP(" < ",   180, 181, 181);
				case ZEND_IS_SMALLER_OR_EQUAL: BINARY_OP(" <= ",  180, 181, 181);
				case ZEND_POW:                 BINARY_OP(" ** ",  250, 251, 250);
				case ZEND_BOOL_XOR:            BINARY_OP(" xor ",  40,  40,  41);
				case ZEND_SPACESHIP:           BINARY_OP(" <=> ", 180, 181, 181);
				EMPTY_SWITCH_DEFAULT_CASE();
			}
			break;
		case ZEND_AST_GREATER:                 BINARY_OP(" > ",   180, 181, 181);
		case ZEND_AST_GREATER_EQUAL:           BINARY_OP(" >= ",  180, 181, 181);
		case ZEND_AST_AND:                     BINARY_OP(" && ",  130, 130, 131);
		case ZEND_AST_OR:                      BINARY_OP(" || ",  120, 120, 121);
		// ing3, 数组元素
		case ZEND_AST_ARRAY_ELEM:
			// 如果有key
			if (ast->child[1]) {
				zend_ast_export_ex(str, ast->child[1], 80, indent);
				smart_str_appends(str, " => ");
			}
			// 如果要求传址
			if (ast->attr)
				smart_str_appendc(str, '&');
			// value
			zend_ast_export_ex(str, ast->child[0], 80, indent);
			break;
		// ing3, new 语句
		case ZEND_AST_NEW:
			smart_str_appends(str, "new ");
			// 如果是定义的类或匿名类
			if (ast->child[0]->kind == ZEND_AST_CLASS) {
				zend_ast_decl *decl = (zend_ast_decl *) ast->child[0];
				// 修饰属性
				if (decl->child[3]) {
					zend_ast_export_attributes(str, decl->child[3], indent, 0);
				}
				smart_str_appends(str, "class");
				if (!zend_ast_is_list(ast->child[1])
						|| zend_ast_get_list(ast->child[1])->children) {
					smart_str_appendc(str, '(');
					zend_ast_export_ex(str, ast->child[1], 0, indent);
					smart_str_appendc(str, ')');
				}
				zend_ast_export_class_no_header(str, decl, indent);
			// 可以是表达式
			} else {
				zend_ast_export_ns_name(str, ast->child[0], 0, indent);
				smart_str_appendc(str, '(');
				zend_ast_export_ex(str, ast->child[1], 0, indent);
				smart_str_appendc(str, ')');
			}
			break;
		// ing3,
		case ZEND_AST_INSTANCEOF:
			zend_ast_export_ex(str, ast->child[0], 0, indent);
			smart_str_appends(str, " instanceof ");
			zend_ast_export_ns_name(str, ast->child[1], 0, indent);
			break;
		case ZEND_AST_YIELD:
			if (priority > 70) smart_str_appendc(str, '(');
			smart_str_appends(str, "yield ");
			if (ast->child[0]) {
				if (ast->child[1]) {
					zend_ast_export_ex(str, ast->child[1], 70, indent);
					smart_str_appends(str, " => ");
				}
				zend_ast_export_ex(str, ast->child[0], 70, indent);
			}
			if (priority > 70) smart_str_appendc(str, ')');
			break;
		case ZEND_AST_YIELD_FROM:
			PREFIX_OP("yield from ", 85, 86);
		// ?
		case ZEND_AST_COALESCE: BINARY_OP(" ?? ", 110, 111, 110);
		// ing3, 声名静态变量
		case ZEND_AST_STATIC:
			smart_str_appends(str, "static $");
			zend_ast_export_name(str, ast->child[0], 0, indent);
			// 添加默认值
			APPEND_DEFAULT_VALUE(1);
		// ing3,
		case ZEND_AST_WHILE:
			smart_str_appends(str, "while (");
			zend_ast_export_ex(str, ast->child[0], 0, indent);
			smart_str_appends(str, ") {\n");
			zend_ast_export_stmt(str, ast->child[1], indent + 1);
			zend_ast_export_indent(str, indent);
			smart_str_appendc(str, '}');
			break;
		// ing3, do -> while
		case ZEND_AST_DO_WHILE:
			smart_str_appends(str, "do {\n");
			zend_ast_export_stmt(str, ast->child[0], indent + 1);
			zend_ast_export_indent(str, indent);
			smart_str_appends(str, "} while (");
			zend_ast_export_ex(str, ast->child[1], 0, indent);
			smart_str_appendc(str, ')');
			break;
		// ing3, if else 语句，没有elseif
		case ZEND_AST_IF_ELEM:
			if (ast->child[0]) {
				smart_str_appends(str, "if (");
				zend_ast_export_ex(str, ast->child[0], 0, indent);
				smart_str_appends(str, ") {\n");
				zend_ast_export_stmt(str, ast->child[1], indent + 1);
			} else {
				smart_str_appends(str, "else {\n");
				zend_ast_export_stmt(str, ast->child[1], indent + 1);
			}
			zend_ast_export_indent(str, indent);
			smart_str_appendc(str, '}');
			break;
		// ing3， switch 语句
		case ZEND_AST_SWITCH:
			smart_str_appends(str, "switch (");
			zend_ast_export_ex(str, ast->child[0], 0, indent);
			smart_str_appends(str, ") {\n");
			zend_ast_export_ex(str, ast->child[1], 0, indent + 1);
			zend_ast_export_indent(str, indent);
			smart_str_appendc(str, '}');
			break;
		// ing3, switch里的case语句，逻辑和match语句蛮相似
		case ZEND_AST_SWITCH_CASE:
			zend_ast_export_indent(str, indent);
			if (ast->child[0]) {
				smart_str_appends(str, "case ");
				zend_ast_export_ex(str, ast->child[0], 0, indent);
				smart_str_appends(str, ":\n");
			} else {
				smart_str_appends(str, "default:\n");
			}
			zend_ast_export_stmt(str, ast->child[1], indent + 1);
			break;
		// ing3, match 语句
		case ZEND_AST_MATCH:
			smart_str_appends(str, "match (");
			zend_ast_export_ex(str, ast->child[0], 0, indent);
			smart_str_appends(str, ") {\n");
			zend_ast_export_ex(str, ast->child[1], 0, indent + 1);
			zend_ast_export_indent(str, indent);
			smart_str_appendc(str, '}');
			break;
		// ing3, match里的每个arm语句
		case ZEND_AST_MATCH_ARM:
			zend_ast_export_indent(str, indent);
			// 如果有指定条件
			if (ast->child[0]) {
				zend_ast_export_list(str, (zend_ast_list*)ast->child[0], 1, 0, indent);
				smart_str_appends(str, " => ");
			// 默认条件
			} else {
				smart_str_appends(str, "default => ");
			}
			// expr
			zend_ast_export_ex(str, ast->child[1], 0, 0);
			smart_str_appends(str, ",\n");
			break;
		// ing3, declare 语句
		case ZEND_AST_DECLARE:
			smart_str_appends(str, "declare(");
			// 必须是常量定义语句
			ZEND_ASSERT(ast->child[0]->kind == ZEND_AST_CONST_DECL);
			zend_ast_export_list(str, (zend_ast_list*)ast->child[0], 1, 0, indent);
			smart_str_appendc(str, ')');
			// 代码块（可选）
			if (ast->child[1]) {
				smart_str_appends(str, " {\n");
				zend_ast_export_stmt(str, ast->child[1], indent + 1);
				zend_ast_export_indent(str, indent);
				smart_str_appendc(str, '}');
			} else {
				smart_str_appendc(str, ';');
			}
			break;
		// ing3, 类里每个属性	
		case ZEND_AST_PROP_ELEM:
			smart_str_appendc(str, '$');
			ZEND_FALLTHROUGH;
		// ing3, 声明常量，每一个元素
		case ZEND_AST_CONST_ELEM:
			zend_ast_export_name(str, ast->child[0], 0, indent);
			// 添加默认值
			APPEND_DEFAULT_VALUE(1);
		// ing3, 类里添加trait引用
		case ZEND_AST_USE_TRAIT:
			smart_str_appends(str, "use ");
			zend_ast_export_ex(str, ast->child[0], 0, indent);
			// 有代码块（详细设置）
			if (ast->child[1]) {
				smart_str_appends(str, " {\n");
				zend_ast_export_ex(str, ast->child[1], 0, indent + 1);
				zend_ast_export_indent(str, indent);
				smart_str_appends(str, "}");
			} else {
				smart_str_appends(str, ";");
			}
			break;
		// ing3, trait引用冲突处理
		case ZEND_AST_TRAIT_PRECEDENCE:
			zend_ast_export_ex(str, ast->child[0], 0, indent);
			smart_str_appends(str, " insteadof ");
			zend_ast_export_ex(str, ast->child[1], 0, indent);
			break;
		// ing3, 引用trait 里的方法名，可以写绝对地址
		case ZEND_AST_METHOD_REFERENCE:
			if (ast->child[0]) {
				zend_ast_export_name(str, ast->child[0], 0, indent);
				smart_str_appends(str, "::");
			}
			zend_ast_export_name(str, ast->child[1], 0, indent);
			break;
		// ing3 定义命名空间
		case ZEND_AST_NAMESPACE:
			smart_str_appends(str, "namespace");
			// ** 命名空间是可以不设置名字的
			if (ast->child[0]) {
				smart_str_appendc(str, ' ');
				zend_ast_export_name(str, ast->child[0], 0, indent);
			}
			// 如果有代码块
			if (ast->child[1]) {
				smart_str_appends(str, " {\n");
				zend_ast_export_stmt(str, ast->child[1], indent + 1);
				zend_ast_export_indent(str, indent);
				smart_str_appends(str, "}\n");
			} else {
				smart_str_appendc(str, ';');
			}
			break;
		// ing3, use trait
		case ZEND_AST_USE_ELEM:
		case ZEND_AST_TRAIT_ALIAS:
			// 名称
			zend_ast_export_name(str, ast->child[0], 0, indent);
			// 设置可见度同时设置别名, 别名放在后面
			// 重新设置可见度
			if (ast->attr & ZEND_ACC_PUBLIC) {
				smart_str_appends(str, " as public");
			} else if (ast->attr & ZEND_ACC_PROTECTED) {
				smart_str_appends(str, " as protected");
			} else if (ast->attr & ZEND_ACC_PRIVATE) {
				smart_str_appends(str, " as private");
			// 没有可见度，只设置别名
			} else if (ast->child[1]) {
				smart_str_appends(str, " as");
			}
			// 别名，可以和可见度同时设置，前面只加一个as
			if (ast->child[1]) {
				smart_str_appendc(str, ' ');
				zend_ast_export_name(str, ast->child[1], 0, indent);
			}
			break;
			
		// 带类型的形参
		case ZEND_AST_NAMED_ARG:
			// 第一个元素当类型
			smart_str_append(str, zend_ast_get_str(ast->child[0]));
			smart_str_appends(str, ": ");
			// 第二个元素当参数
			ast = ast->child[1];
			goto tail_call;

		/* 3 child nodes */
		case ZEND_AST_METHOD_CALL:
		// ing3, 动态调用 $obj ?-> a()
		case ZEND_AST_NULLSAFE_METHOD_CALL:
			zend_ast_export_ex(str, ast->child[0], 0, indent);
			// 是否允null
			smart_str_appends(str, ast->kind == ZEND_AST_NULLSAFE_METHOD_CALL ? "?->" : "->");
			zend_ast_export_var(str, ast->child[1], 0, indent);
			smart_str_appendc(str, '(');
			zend_ast_export_ex(str, ast->child[2], 0, indent);
			smart_str_appendc(str, ')');
			break;
		// ing3, 静态调用 class::f()
		case ZEND_AST_STATIC_CALL:
			zend_ast_export_ns_name(str, ast->child[0], 0, indent);
			smart_str_appends(str, "::");
			zend_ast_export_var(str, ast->child[1], 0, indent);
			smart_str_appendc(str, '(');
			zend_ast_export_ex(str, ast->child[2], 0, indent);
			smart_str_appendc(str, ')');
			break;
		// ing3, 三元运算符
		case ZEND_AST_CONDITIONAL:
			// 如果要优先
			if (priority > 100) smart_str_appendc(str, '(');
			// 条件
			zend_ast_export_ex(str, ast->child[0], 100, indent);
			// 如果有给定真值
			if (ast->child[1]) {
				smart_str_appends(str, " ? ");
				zend_ast_export_ex(str, ast->child[1], 101, indent);
				smart_str_appends(str, " : ");
			// 如果没有给定真值
			} else {
				smart_str_appends(str, " ?: ");
			}
			// 假值
			zend_ast_export_ex(str, ast->child[2], 101, indent);
			if (priority > 100) smart_str_appendc(str, ')');
			break;
		// ing3, try 语句
		case ZEND_AST_TRY:
			smart_str_appends(str, "try {\n");
			// 代码块
			zend_ast_export_stmt(str, ast->child[0], indent + 1);
			zend_ast_export_indent(str, indent);
			zend_ast_export_ex(str, ast->child[1], 0, indent);
			// 如果有finally 语句
			if (ast->child[2]) {
				smart_str_appends(str, "} finally {\n");
				// 代码块
				zend_ast_export_stmt(str, ast->child[2], indent + 1);
				zend_ast_export_indent(str, indent);
			}
			smart_str_appendc(str, '}');
			break;
		// ing3, 单个catch 语句
		case ZEND_AST_CATCH:
			smart_str_appends(str, "} catch (");
			// catch 类型
			zend_ast_export_catch_name_list(str, zend_ast_get_list(ast->child[0]), indent);
			// 如果有变量名，添加变量名
			if (ast->child[1]) {
				smart_str_appends(str, " $");
				zend_ast_export_var(str, ast->child[1], 0, indent);
			}
			smart_str_appends(str, ") {\n");
			// 代码块
			zend_ast_export_stmt(str, ast->child[2], indent + 1);
			zend_ast_export_indent(str, indent);
			// 最后一个 } 哪去了
			break;
		// ing3, ()中单个参数
		case ZEND_AST_PARAM:
			// 修饰属性
			if (ast->child[3]) {
				zend_ast_export_attributes(str, ast->child[3], indent, 0);
			}
			// 类型
			if (ast->child[0]) {
				zend_ast_export_type(str, ast->child[0], indent);
				smart_str_appendc(str, ' ');
			}
			// 引用地址
			if (ast->attr & ZEND_PARAM_REF) {
				smart_str_appendc(str, '&');
			}
			// 解包
			if (ast->attr & ZEND_PARAM_VARIADIC) {
				smart_str_appends(str, "...");
			}
			// $开头的变量
			smart_str_appendc(str, '$');
			zend_ast_export_name(str, ast->child[1], 0, indent);
			// 默认值
			APPEND_DEFAULT_VALUE(2);
		// ing3, enum 里的 case 语句
		case ZEND_AST_ENUM_CASE:
			// 修饰属性
			if (ast->child[3]) {
				zend_ast_export_attributes(str, ast->child[3], indent, 1);
			}
			smart_str_appends(str, "case ");
			// 元素
			zend_ast_export_name(str, ast->child[0], 0, indent);
			// 定义值
			if (ast->child[1]) {
				smart_str_appends(str, " = ");
				zend_ast_export_ex(str, ast->child[1], 0, indent);
			}
			break;

		/* 4 child nodes */
		// ing3, for语句
		case ZEND_AST_FOR:
			smart_str_appends(str, "for (");
			// 第一节
			zend_ast_export_ex(str, ast->child[0], 0, indent);
			smart_str_appendc(str, ';');
			// 第二节
			if (ast->child[1]) {
				smart_str_appendc(str, ' ');
				zend_ast_export_ex(str, ast->child[1], 0, indent);
			}
			smart_str_appendc(str, ';');
			// 第三节
			if (ast->child[2]) {
				smart_str_appendc(str, ' ');
				zend_ast_export_ex(str, ast->child[2], 0, indent);
			}
			smart_str_appends(str, ") {\n");
			// 业务逻辑
			zend_ast_export_stmt(str, ast->child[3], indent + 1);
			zend_ast_export_indent(str, indent);
			smart_str_appendc(str, '}');
			break;
		// ing3, foreach 语句
		case ZEND_AST_FOREACH:
			smart_str_appends(str, "foreach (");
			// 被遍历的 expr
			zend_ast_export_ex(str, ast->child[0], 0, indent);
			smart_str_appends(str, " as ");
			// 如果有key
			if (ast->child[2]) {
				zend_ast_export_ex(str, ast->child[2], 0, indent);
				smart_str_appends(str, " => ");
			}
			// value
			zend_ast_export_ex(str, ast->child[1], 0, indent);
			smart_str_appends(str, ") {\n");
			// 代码块，业务逻辑
			zend_ast_export_stmt(str, ast->child[3], indent + 1);
			zend_ast_export_indent(str, indent);
			smart_str_appendc(str, '}');
			break;
		EMPTY_SWITCH_DEFAULT_CASE();
	}
	return;

// 这些label定义在方法内,return 和 goto 的混用要注意
binary_op:
	if (priority > p) smart_str_appendc(str, '(');
	zend_ast_export_ex(str, ast->child[0], pl, indent);
	smart_str_appends(str, op);
	zend_ast_export_ex(str, ast->child[1], pr, indent);
	if (priority > p) smart_str_appendc(str, ')');
	return;

prefix_op:
	if (priority > p) smart_str_appendc(str, '(');
	smart_str_appends(str, op);
	zend_ast_export_ex(str, ast->child[0], pl, indent);
	if (priority > p) smart_str_appendc(str, ')');
	return;

postfix_op:
	if (priority > p) smart_str_appendc(str, '(');
	zend_ast_export_ex(str, ast->child[0], pl, indent);
	smart_str_appends(str, op);
	if (priority > p) smart_str_appendc(str, ')');
	return;

// 调用函数
func_op:
	smart_str_appends(str, op);
	smart_str_appendc(str, '(');
	zend_ast_export_ex(str, ast->child[0], 0, indent);
	smart_str_appendc(str, ')');
	return;

// 添加 global， unset，return， echo， throw， break， continue， 关键字
append_node_1:
	smart_str_appends(str, op);
	if (ast->child[0]) {
		smart_str_appendc(str, ' ');
		ast = ast->child[0];
		goto tail_call;
	}
	return;
	
// 直接添加字串
append_str:
	smart_str_appends(str, op);
	return;

// 静态变量和常量添加默认值
append_default_value:
	if (ast->child[p]) {
		smart_str_appends(str, " = ");
		ast = ast->child[p];
		goto tail_call;
	}
	return;
}

// 唯一外部调用的 export 
ZEND_API ZEND_COLD zend_string *zend_ast_export(const char *prefix, zend_ast *ast, const char *suffix)
{
	// php代码字串
	smart_str str = {0};

	smart_str_appends(&str, prefix);
	//
	zend_ast_export_ex(&str, ast, 0, 0);
	smart_str_appends(&str, suffix);
	// 添加结束标记
	smart_str_0(&str);
	return str.s;
}

// ? 带修饰属性的语句
zend_ast * ZEND_FASTCALL zend_ast_with_attributes(zend_ast *ast, zend_ast *attr)
{
	// 必须是修饰属性列表
	ZEND_ASSERT(attr->kind == ZEND_AST_ATTRIBUTE_LIST);

	switch (ast->kind) {
	case ZEND_AST_FUNC_DECL:
	case ZEND_AST_CLOSURE:
	case ZEND_AST_METHOD:
	case ZEND_AST_ARROW_FUNC:
		((zend_ast_decl *) ast)->child[4] = attr;
		break;
	case ZEND_AST_CLASS:
		((zend_ast_decl *) ast)->child[3] = attr;
		break;
	case ZEND_AST_PROP_GROUP:
		ast->child[2] = attr;
		break;
	case ZEND_AST_PARAM:
	case ZEND_AST_ENUM_CASE:
		ast->child[3] = attr;
		break;
	case ZEND_AST_CLASS_CONST_GROUP:
		ast->child[1] = attr;
		break;
	EMPTY_SWITCH_DEFAULT_CASE()
	}

	return ast;
}
