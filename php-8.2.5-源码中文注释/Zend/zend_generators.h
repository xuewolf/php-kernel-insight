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
   | Authors: Nikita Popov <nikic@php.net>                                |
   |          Bob Weinand <bobwei9@hotmail.com>                           |
   +----------------------------------------------------------------------+
*/

#ifndef ZEND_GENERATORS_H
#define ZEND_GENERATORS_H

BEGIN_EXTERN_C()

extern ZEND_API zend_class_entry *zend_ce_generator;
extern ZEND_API zend_class_entry *zend_ce_ClosedGeneratorException;

typedef struct _zend_generator_node zend_generator_node;
typedef struct _zend_generator zend_generator;

/* 
`yield from` 概念 用于在 委派生成器 链的不同层级的访问 中揭示  问题。
需要在所有情况下 关联当前执行生成器 并 可以访问已完成的生成器的值。
The concept of `yield from` exposes problems when accessed at different levels of the chain of delegated generators. We need to be able to reference the currently executed Generator in all cases and still being able to access the return values of finished Generators.

这个问题的解决方法是一个双向链接的树，所有引用的生成器 都引用这个树。
在所有情况下都不可以避免 追溯 这个树。
用这种方式，当 `yield from` 链的一些部分 关联到 其他 `yield from` 时，只需要从叶子到根追溯这个树。
（当叶子获取一个子节点 直接从叶子到根，叶子节点的指针和 子节点列表节点都需要更新）
但只有这种情况，是相对少见的情况（它可能发生，但不是全局最简便）
 * The solution to this problem is a doubly-linked tree, which all Generators referenced in maintain a reference to. It should be impossible to avoid walking the tree in all cases. This way, we only need tree walks from leaf to root in case where some part of the `yield from` chain is passed to another `yield from`. (Update of leaf node pointer and list of multi-children nodes needed when leaf gets a child in direct path from leaf to root node.) But only in that case, which should be a fairly rare case (which is then possible, but not totally cheap).
 
 树的根是当前执行过的生成器。树的子节点（除根节点外）是所有 进行 `yield from` 的生成器。
 树的每个节点 有一个指针指向 一个叶子后继节点。
 每个有多个孩子的节点需要一个 存储所有后继节点的 ？？
 （堆栈由叶子节点的指针决定）只有单个孩子的节点不需要列表，有一个指向孩子的指针就够了。
 另外，叶子节点有一个指向根节点的指针
 * The root of the tree is then the currently executed Generator. The subnodes of the tree (all except the root node) are all Generators which do `yield from`. Each node of the tree knows a pointer to one leaf descendant node. Each node with multiple children needs a list of all leaf descendant nodes paired with pointers to their respective child node. (The stack is determined by leaf node pointers) Nodes with only one child just don't need a list, there it is enough to just have a pointer to the child node. Further, leaf nodes store a pointer to the root node.
 
 这个方式当使用生成器时，只需要查找一个叶子节点（它们都有指向根的引用）
 就可以从根上看到生成器是否已完成。如果没完成，没问题，还可以继续执行。
 如果生成器完成了，有两种情况。如果是只有一个孩子的简单节点，找到孩子节点。
 如果有多个子节点，需要从列表中删除当前叶子节点（非必要的微优化）并找到与当前叶子节点配对的子节点。
 如果当前节点引用的生成器正在执行，可以通过 YIELD_FROM 操作码来继续。
 当一个节点被当作某一叶子的根，同时它又有parent，需要身上查找直到找到没有父节点的node。
 * That way, when we advance any generator, we just need to look up a leaf node (which all have a reference to a root node). Then we can see at the root node whether current Generator is finished. If it isn't, all is fine and we can just continue. If the Generator finished, there will be two cases. Either it is a simple node with just one child, then go down to child node. Or it has multiple children and we now will remove the current leaf node from the list of nodes (unnecessary, is microoptimization) and go down to the child node whose reference was paired with current leaf node. Child node is then removed its parent reference and becomes new top node. Or the current node references the Generator we're currently executing, then we can continue from the YIELD_FROM opcode. When a node referenced as root node in a leaf node has a parent, then we go the way up until we find a root node without parent.
 
 这种情况 进入一个 新的 `yield from` 层级，一个在当前root的顶端创建 并成为新root 的节点。叶子节点需要和新root一起更新。
 * In case we go into a new `yield from` level, a node is created on top of current root and becomes the new root. Leaf node needs to be updated with new root node then.
 
 当一个被树节点引用的生成器 被添加到 `yield from`，这个节点有了孩子列表（ ？？
 当没有多个孩子节点时（线形树），只需要添加一对（指向叶子节点，指向孩子节点），孩子节点作为叶子节点到此节点的
 ？？
 * When a Generator referenced by a node of the tree is added to `yield from`, that node now gets a list of children (we need to walk the descendants of that node and nodes of the tree of the other Generator down to the first multi-children node and copy all the leaf node pointers from there). In case there was no multi-children node (linear tree), we just add a pair (pointer to leaf node, pointer to child node), with the child node being in a direct path from leaf to this node.
 */

// 生成器节点
struct _zend_generator_node {
	// 根节点是 null
	zend_generator *parent; /* NULL for root */
	// 子节点数量
	uint32_t children;
	// 子节点
	union {
		// 如果有多个子节点
		HashTable *ht; /* if multiple children */
		// 如果有单个子节点
		zend_generator *single; /* if one child */
	} child;
	// 一个生成器可以缓存 一个直接指向当前root的指针
	// 叶子节点指回 使用root缓存的生成器
	/* One generator can cache a direct pointer to the current root.
	 * The leaf member points back to the generator using the root cache. */
	// 指针
	union {
		// 叶子。如果有父节点
		zend_generator *leaf; /* if parent != NULL */
		// 根。如果没有父节点
		zend_generator *root; /* if parent == NULL */
	} ptr;
};

// 生成器
struct _zend_generator {
	// 里面自带一个对象
	zend_object std;

	// 停止的执行上下文
	/* The suspended execution context. */
	zend_execute_data *execute_data;

	// 其他调用上下文中使用的 “yield” 的冻结堆栈
	/* Frozen call stack for "yield" used in context of other calls */
	zend_execute_data *frozen_call_stack;

	// 当前value
	/* Current value */
	zval value;
	// 当前 key
	/* Current key */
	zval key;
	// 返回值
	/* Return value */
	zval retval;
	// 存放发送值的变量
	/* Variable to put sent value into */
	zval *send_target;
	
	// 自增key的最大值
	/* Largest used integer key for auto-incrementing keys */
	zend_long largest_used_integer_key;

	// 此生成器产生的 "yield from" 指定的值
	// 这个只用于数组或 非生成器的可 遍历对象
	// 这个zval也 和foreach引用值一样 使用 u2 结构
	/* Values specified by "yield from" to yield from this generator.
	 * This is only used for arrays or non-generator Traversables.
	 * This zval also uses the u2 structure in the same way as
	 * by-value foreach. */
	zval values;

	// 当多个 "yield from" 嵌套时 的 等待节点
	/* Node of waiting generators when multiple "yield from" expressions
	 * are nested. */
	zend_generator_node node;

	// 假执行数据，用于堆栈追踪的执行数据
	/* Fake execute_data for stacktraces */
	zend_execute_data execute_fake;

	// ZEND_GENERATOR_ 开关头的标记
	/* ZEND_GENERATOR_* flags */
	zend_uchar flags;
};

// 当前正在运行
static const zend_uchar ZEND_GENERATOR_CURRENTLY_RUNNING = 0x1;
// 强制关闭
static const zend_uchar ZEND_GENERATOR_FORCED_CLOSE      = 0x2;
// 第一次yield
static const zend_uchar ZEND_GENERATOR_AT_FIRST_YIELD    = 0x4;
// 初始化
static const zend_uchar ZEND_GENERATOR_DO_INIT           = 0x8;
// 在fiber里
static const zend_uchar ZEND_GENERATOR_IN_FIBER          = 0x10;

void zend_register_generator_ce(void);
ZEND_API void zend_generator_close(zend_generator *generator, bool finished_execution);
ZEND_API void zend_generator_resume(zend_generator *generator);

ZEND_API void zend_generator_restore_call_stack(zend_generator *generator);
ZEND_API zend_execute_data* zend_generator_freeze_call_stack(zend_execute_data *execute_data);

void zend_generator_yield_from(zend_generator *generator, zend_generator *from);
ZEND_API zend_execute_data *zend_generator_check_placeholder_frame(zend_execute_data *ptr);

ZEND_API zend_generator *zend_generator_update_current(zend_generator *generator);
ZEND_API zend_generator *zend_generator_update_root(zend_generator *generator);

// ing3, 更新并返回当前生成器
static zend_always_inline zend_generator *zend_generator_get_current(zend_generator *generator)
{
	// 如果生成器的节点没有父节点
	if (EXPECTED(generator->node.parent == NULL)) {
		// 不在 yield from 模式
		/* we're not in yield from mode */
		// 直接返回生成器
		return generator;
	}
	// 找到迭代器的根节点
	zend_generator *root = generator->node.ptr.root;
	// 如果没有根节点
	if (!root) {
		// 更新生成器的root节点（追踪到最终的root）。p1:生成器
		root = zend_generator_update_root(generator);
	}

	// 如果root有执行数据
	if (EXPECTED(root->execute_data)) {
		// 生成器还在运行
		/* generator still running */
		// 返回root
		return root;
	}
	// 更新当前生成器
	return zend_generator_update_current(generator);
}

END_EXTERN_C()

#endif
