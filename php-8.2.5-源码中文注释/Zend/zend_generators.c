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

#include "zend.h"
#include "zend_API.h"
#include "zend_interfaces.h"
#include "zend_exceptions.h"
#include "zend_generators.h"
#include "zend_closures.h"
#include "zend_generators_arginfo.h"
#include "zend_observer.h"

ZEND_API zend_class_entry *zend_ce_generator;
ZEND_API zend_class_entry *zend_ce_ClosedGeneratorException;
static zend_object_handlers zend_generator_handlers;

static zend_object *zend_generator_create(zend_class_entry *class_type);

// ing3, 生成存储（整理）生成器的调用堆栈
ZEND_API void zend_generator_restore_call_stack(zend_generator *generator) /* {{{ */
{
	zend_execute_data *call, *new_call, *prev_call = NULL;
	// 找到冻结的调用堆栈
	call = generator->frozen_call_stack;
	// 遍历 执行数据链
	do {
		// 初始化并返回 堆栈最上一个执行数据，空间不够则扩展大小
		// p1:调用信息，p2:函数，p3:参数数量，p4:对象或调用域
		new_call = zend_vm_stack_push_call_frame(
			// 调用信息，整数。不是函数调用信息。去掉 ZEND_CALL_ALLOCATED标记
			(ZEND_CALL_INFO(call) & ~ZEND_CALL_ALLOCATED),
			// 调用函数
			call->func,
			// 参数数量
			ZEND_CALL_NUM_ARGS(call),
			// $this 指针
			Z_PTR(call->This));
		// 
		memcpy(((zval*)new_call) + ZEND_CALL_FRAME_SLOT, ((zval*)call) + ZEND_CALL_FRAME_SLOT, ZEND_CALL_NUM_ARGS(call) * sizeof(zval));
		// 关联原来的额外命名参数列表
		new_call->extra_named_params = call->extra_named_params;
		// 关联前一个调用数据
		new_call->prev_execute_data = prev_call;
		// 暂存成前一个调用
		prev_call = new_call;

		// 切换到前一个执行数据
		call = call->prev_execute_data;
	} while (call);
	// 关联前一个调用信息
	generator->execute_data->call = prev_call;
	// 释放冻结的调用堆栈
	efree(generator->frozen_call_stack);
	// 清空 冻结的调用堆栈 指针
	generator->frozen_call_stack = NULL;
}
/* }}} */

// ing2, 冻结执行数据堆栈，p1:执行数据
ZEND_API zend_execute_data* zend_generator_freeze_call_stack(zend_execute_data *execute_data) /* {{{ */
{
	size_t used_stack;
	zend_execute_data *call, *new_call, *prev_call = NULL;
	zval *stack;

	// 计算需要的堆栈大小
	/* calculate required stack size */
	used_stack = 0;
	// 调用的执行数据？
	call = EX(call);
	do {
		// 已使用堆栈大小 = 头大小 + 参数数量
		used_stack += ZEND_CALL_FRAME_SLOT + ZEND_CALL_NUM_ARGS(call);
		// 取得前一个执行数据
		call = call->prev_execute_data;
	// 如果前一个执行数据存在，一直向前取
	} while (call);

	// 分配内存创建堆栈
	stack = emalloc(used_stack * sizeof(zval));

	// 保存堆栈，把 执行数据反序串联起来
	/* save stack, linking frames in reverse order */
	call = EX(call);
	// 
	do {
		// 执行数据大小
		size_t frame_size = ZEND_CALL_FRAME_SLOT + ZEND_CALL_NUM_ARGS(call);
		// 变量列表的开头
		new_call = (zend_execute_data*)(stack + used_stack - frame_size);
		// 把原执行数据的所有变量，复制到新执行数据
		memcpy(new_call, call, frame_size * sizeof(zval));
		// ？这个应该是头大小
		used_stack -= frame_size;
		// 新执行数据 的前一个执行数据 
		new_call->prev_execute_data = prev_call;
		// 前一个执行数据
		prev_call = new_call;

		// 前一个执行数据作为新执行数据
		new_call = call->prev_execute_data;
		// 释放调用框架（执行数据）。p1:执行数据
		zend_vm_stack_free_call_frame(call);
		// 使用新的 执行数据
		call = new_call;
	// 如果 前一个执行数据 有效，继续
	} while (call);

	// 清空调用执行数据指针
	execute_data->call = NULL;
	// 
	ZEND_ASSERT(prev_call == (zend_execute_data*)stack);
	// 返回前一个执行数据
	return prev_call;
}
/* }}} */

// ing3, 把执行数据反着串起来，并返回最前一个执行数据
static zend_execute_data* zend_generator_revert_call_stack(zend_execute_data *call)
{
	zend_execute_data *prev = NULL;

	// 
	do {
		// 取得前一个执行数据
		zend_execute_data *next = call->prev_execute_data;
		// 更新前一个执行数据
		call->prev_execute_data = prev;
		// 记录当前执行数据
		prev = call;
		// 下一个操作的执行数据
		call = next;
	// 如果还有待操作数据，一直执行
	} while (call);

	// 返回最前一个执行数据
	return prev;
}

// ing3, 生成器清理未完成的执行。p1:生成器，p2:执行数据，p3:catch_op_num
static void zend_generator_cleanup_unfinished_execution(
		zend_generator *generator, zend_execute_data *execute_data, uint32_t catch_op_num) /* {{{ */
{
	// 函数中的操作码组
	zend_op_array *op_array = &execute_data->func->op_array;
	
	// 如果当前操作码不是 操作码列表的开头
	if (execute_data->opline != op_array->opcodes) {
		// 需要 -1 因为需要找到最后运行的 操作码，而不是将要运行的操作码
		/* -1 required because we want the last run opcode, not the next to-be-run one. */
		
		// 要找到最后运行的 操作码位置
		uint32_t op_num = execute_data->opline - op_array->opcodes - 1;

		// 如果有冻结的调用堆栈
		if (UNEXPECTED(generator->frozen_call_stack)) {
			// 临时保存 generator->execute_data，如果它已经被设置成null了
			/* Temporarily restore generator->execute_data if it has been NULLed out already. */
			// 保存原执行数据指针
			zend_execute_data *save_ex = generator->execute_data;
			// 使用新的执行数据
			generator->execute_data = execute_data;
			// 生成存储（整理）生成器的调用堆栈
			zend_generator_restore_call_stack(generator);
			// 恢复原来的执行数据
			generator->execute_data = save_ex;
		}
		//  清理未完成的执行。p1:执行数据，p2:操作码数量，p3:catch_op_num
		zend_cleanup_unfinished_execution(execute_data, op_num, catch_op_num);
	}
}
/* }}} */

// ing3, 关闭生成器。p1:生成器，p2:运行是否已完成
ZEND_API void zend_generator_close(zend_generator *generator, bool finished_execution) /* {{{ */
{
	// 如果生成器有 执行数据
	if (EXPECTED(generator->execute_data)) {
		// 取出执行数据
		zend_execute_data *execute_data = generator->execute_data;
		
		// 先把 execute_data 清空，防止 当我们清理execute_data时，垃圾回民造成的双重释放
		/* Null out execute_data early, to prevent double frees if GC runs while we're
		 * already cleaning up execute_data. */
		generator->execute_data = NULL;

		// 如果 $this 有符号表
		// Z_TYPE_INFO(execute_data->This)
		if (EX_CALL_INFO() & ZEND_CALL_HAS_SYMBOL_TABLE) {
			// 清空并缓存 符号表
			zend_clean_and_cache_symbol_table(execute_data->symbol_table);
		}
		
		// 总是释放掉编译变量，符号表里只有未释放的 间接引用
		/* always free the CV's, in the symtable are only not-free'd IS_INDIRECT's */
		// 释放编译变量
		zend_free_compiled_variables(execute_data);
		// 如果有命名参数
		if (EX_CALL_INFO() & ZEND_CALL_HAS_EXTRA_NAMED_PARAMS) {
			// 释放额外的命名参数
			zend_free_extra_named_params(execute_data->extra_named_params);
		}

		// 如果需要释放 $this
		if (EX_CALL_INFO() & ZEND_CALL_RELEASE_THIS) {
			// 释放 $this
			OBJ_RELEASE(Z_OBJ(execute_data->This));
		}

		// 在生成器运行中存在致使的 error或die
		// 在这种情况下， 清理堆栈是不安全的
		/* A fatal error / die occurred during the generator execution.
		 * Trying to clean up the stack may not be safe in this case. */
		// 如果有未清理的关闭
		if (UNEXPECTED(CG(unclean_shutdown))) {
			// 清空生成器的执行数据
			generator->execute_data = NULL;
			// 返回
			return;
		}

		// 在虚拟机堆栈中释放参数表
		zend_vm_stack_free_extra_args(execute_data);

		// 只有在完成运行前成器关闭了，清理才是必要的。（碰到返回语句）
		/* Some cleanups are only necessary if the generator was closed
		 * before it could finish execution (reach a return statement). */
		// 如果有未完成的运行
		if (UNEXPECTED(!finished_execution)) {
			// 生成器清理未完成的执行。p1:生成器，p2:执行数据，p3:catch_op_num
			zend_generator_cleanup_unfinished_execution(generator, execute_data, 0);
		}

		// 释放闭包对象
		/* Free closure object */
		// 如果 $this 是闭包
		if (EX_CALL_INFO() & ZEND_CALL_CLOSURE) {
			// 释放执行数据中的函数
			OBJ_RELEASE(ZEND_CLOSURE_OBJECT(EX(func)));
		}

		// 释放执行数据
		efree(execute_data);
	}
}
/* }}} */

// ing4, 生成器删除子节点。p1:生成器，p2:子节点
static void zend_generator_remove_child(zend_generator_node *node, zend_generator *child)
{
	ZEND_ASSERT(node->children >= 1);
	// 如果有一个子节点
	if (node->children == 1) {
		// 清空子节点
		node->child.single = NULL;
	// 多个子节点
	} else {
		// 子节点表
		HashTable *ht = node->child.ht;
		// 哈希表中删除这个子节点
		zend_hash_index_del(ht, (zend_ulong) child);
		// 如果有2个子节点
		if (node->children == 2) {
			// 
			zend_generator *other_child;
			// 遍历哈希表
			ZEND_HASH_FOREACH_PTR(ht, other_child) {
				// 更新成单个节点
				node->child.single = other_child;
				// 跳出
				break;
			} ZEND_HASH_FOREACH_END();
			// 销毁哈希表
			zend_hash_destroy(ht);
			// 释放哈希表
			efree(ht);
		}
		// 其他数量，不需要额外操作，直接减数就行了
	}
	// 子节点数量 -1
	node->children--;
}

// ing4, 清空此生成器与叶子节点的链接，并返回叶子节点 p1:生成器
static zend_always_inline zend_generator *clear_link_to_leaf(zend_generator *generator) {
	// 生成器不可以有父节点
	ZEND_ASSERT(!generator->node.parent);
	// 找到叶子节点
	zend_generator *leaf = generator->node.ptr.leaf;
	// 如果叶子存在
	if (leaf) {
		// 清空叶子 指向生成器 的指针
		leaf->node.ptr.root = NULL;
		// 清空生成器 指向 叶子 的指针
		generator->node.ptr.leaf = NULL;
		// 返回叶子
		return leaf;
	}
	// 返回null
	return NULL;
}

// ing4, 清空此生成器和root节点的链接
static zend_always_inline void clear_link_to_root(zend_generator *generator) {
	ZEND_ASSERT(generator->node.parent);
	// 如果生成器有指向root
	if (generator->node.ptr.root) {
		// 把root指回生成器的指针清空
		generator->node.ptr.root->node.ptr.leaf = NULL;
		// 把生成器指向root的指针清空
		generator->node.ptr.root = NULL;
	}
}

// 100行
// ing3, 清空相关临时变量，并关闭生成器。p1:生成器
static void zend_generator_dtor_storage(zend_object *object) /* {{{ */
{
	// 生成器
	zend_generator *generator = (zend_generator*) object;
	// 执行数据
	zend_execute_data *ex = generator->execute_data;
	//
	uint32_t op_num, try_catch_offset;
	int i;

	// 生成器的停止的 fiber中运行，它会在fiber销毁时一起销毁
	/* Generator is running in a suspended fiber.
	 * Will be dtor during fiber dtor */
	// 更新并返回当前生成器
	// 如果有 【在fiber中】标记
	if (zend_generator_get_current(generator)->flags & ZEND_GENERATOR_IN_FIBER) {
		// 防止最终 在 yielding 时锁定
		/* Prevent finally blocks from yielding */
		// 添加标记【强制关闭】
		generator->flags |= ZEND_GENERATOR_FORCED_CLOSE;
		// 完毕
		return;
	}

	// 离开yield ,优先允许最终执行
	/* leave yield from mode to properly allow finally execution */
	// 如果生成器有值
	if (UNEXPECTED(Z_TYPE(generator->values) != IS_UNDEF)) {
		// 销毁值
		zval_ptr_dtor(&generator->values);
		// 值设置成 未定义
		ZVAL_UNDEF(&generator->values);
	}

	// 生成器节点的父节点
	zend_generator *parent = generator->node.parent;
	// 如果有父节点
	if (parent) {
		// 生成器删除子节点。p1:生成器，p2:子节点
		zend_generator_remove_child(&parent->node, generator);
		// 清空此生成器和root节点的链接
		clear_link_to_root(generator);
		// 清空父节点指针
		generator->node.parent = NULL;
		// 释放父节点中的对象
		OBJ_RELEASE(&parent->std);
	// 没有父节点
	} else {
		// 清空此生成器与叶子节点的链接，并返回叶子节点 p1:生成器
		clear_link_to_leaf(generator);
	}

	// 如果没有执行数据 或执行数据中没有 finally 语句块 或 当前是未清理的关闭
	if (EXPECTED(!ex) || EXPECTED(!(ex->func->op_array.fn_flags & ZEND_ACC_HAS_FINALLY_BLOCK))
			|| CG(unclean_shutdown)) {
		// 关闭生成器。p1:生成器，p2:运行是否已完成
		zend_generator_close(generator, 0);
		return;
	}

	// 需要 -1 因为需要找到最后运行的 操作码，而不是将要运行的操作码
	/* -1 required because we want the last run opcode, not the
	 * next to-be-run one. */
	// 找到最后运行的 操作码
	op_num = ex->opline - ex->func->op_array.opcodes - 1;
	// 默认没有 try catch
	try_catch_offset = -1;

	// 找到所在的最里层（最后一个） try/catch
	/* Find the innermost try/catch that we are inside of. */
	// 遍历所有的try/catch
	for (i = 0; i < ex->func->op_array.last_try_catch; i++) {
		// 取得 try/catch 元素
		zend_try_catch_element *try_catch = &ex->func->op_array.try_catch_array[i];
		// 如果操作码在try前
		if (op_num < try_catch->try_op) {
			// 跳出（有顺序的，后面的都不用查了）
			break;
		}
		// 如果操作码在 try后，finally 结束前
		if (op_num < try_catch->catch_op || op_num < try_catch->finally_end) {
			// 采用最后一个符合条件的 try/catch
			try_catch_offset = i;
		}
	}

	// 向前遍历 try/catch/finally 结构体，执行必要的操作
	/* Walk try/catch/finally structures upwards, performing the necessary actions. */
	// 如果找到 try/catch
	while (try_catch_offset != (uint32_t) -1) {
		// 取得 try/catch 元素
		zend_try_catch_element *try_catch = &ex->func->op_array.try_catch_array[try_catch_offset];

		// 如果操作操作码在 try 中
		if (op_num < try_catch->finally_op) {
			/* Go to finally block */
			zval *fast_call =
				ZEND_CALL_VAR(ex, ex->func->op_array.opcodes[try_catch->finally_end].op1.var);
			// 生成器清理未完成的执行。p1:生成器，p2:执行数据，p3:catch_op_num
			zend_generator_cleanup_unfinished_execution(generator, ex, try_catch->finally_op);
			Z_OBJ_P(fast_call) = EG(exception);
			// 清空异常
			EG(exception) = NULL;
			// 不在 finally 中
			Z_OPLINE_NUM_P(fast_call) = (uint32_t)-1;

			// 更新当前 操作码
			ex->opline = &ex->func->op_array.opcodes[try_catch->finally_op];
			// 生成器添加标记 强制关闭
			generator->flags |= ZEND_GENERATOR_FORCED_CLOSE;
			// 恢复生成器
			zend_generator_resume(generator);

			/* TODO: If we hit another yield inside try/finally,
			 * should we also jump to the next finally block? */
			break;
		// 如果操作码在 finally 中
		} else if (op_num < try_catch->finally_end) {
			
			// 找到 finally 的最后一行操作码的变量，它就是 fast_call
			// 把p1 右移p2字节，转成zval返回  ((zval*)((char*)(p1) + p2))
			zval *fast_call =
				ZEND_CALL_VAR(ex, ex->func->op_array.opcodes[try_catch->finally_end].op1.var);
			// 清理不完整的返回语句
			/* Clean up incomplete return statement */
			// 如果快速调用的操作码有效（不是-1）
			// 通过指针访问zval的 操作码序号
			if (Z_OPLINE_NUM_P(fast_call) != (uint32_t) -1) {
				// 找到返回值 操作码
				zend_op *retval_op = &ex->func->op_array.opcodes[Z_OPLINE_NUM_P(fast_call)];
				// 如果操作对象2类型是 临时变量或变量
				if (retval_op->op2_type & (IS_TMP_VAR | IS_VAR)) {
					// 销毁操作对象2的变量
					zval_ptr_dtor(ZEND_CALL_VAR(ex, retval_op->op2.var));
				}
			}
			
			// 清理备用执行
			/* Clean up backed-up exception */
			// 如果有快速调用对象
			if (Z_OBJ_P(fast_call)) {
				// 释放这个对象
				OBJ_RELEASE(Z_OBJ_P(fast_call));
			}
		}
		// 前一个 try/catch
		try_catch_offset--;
	}
	// 关闭生成器。p1:生成器，p2:运行是否已完成
	zend_generator_close(generator, 0);
}
/* }}} */

// ing3, 释放生成器
static void zend_generator_free_storage(zend_object *object) /* {{{ */
{
	zend_generator *generator = (zend_generator*) object;

	// 关闭生成器。p1:生成器，p2:运行是否已完成
	zend_generator_close(generator, 0);

	// 不能在 zend_generator_close() 中马上释放它们，其他 yield from 会找不到它
	/* we can't immediately free them in zend_generator_close() else yield from won't be able to fetch it */
	// 销毁生成器的当前值
	zval_ptr_dtor(&generator->value);
	// 销毁生成器的当前key
	zval_ptr_dtor(&generator->key);

	// 如果返回值有效
	if (EXPECTED(!Z_ISUNDEF(generator->retval))) {
		// 销毁返回值
		zval_ptr_dtor(&generator->retval);
	}

	// 如果有大于1个孩子
	if (UNEXPECTED(generator->node.children > 1)) {
		// 销毁 子节点表
		zend_hash_destroy(generator->node.child.ht);
		// 释放子节点表
		efree(generator->node.child.ht);
	}

	// 销毁生成器内置对象
	zend_object_std_dtor(&generator->std);
}
/* }}} */

// ing3, 回收生成器中的变量。p1:生成器，p2:返回变量指针表，p3:返回数量
static HashTable *zend_generator_get_gc(zend_object *object, zval **table, int *n) /* {{{ */
{
	// 生成器
	zend_generator *generator = (zend_generator*)object;
	// 执行数据
	zend_execute_data *execute_data = generator->execute_data;
	//
	zend_execute_data *call = NULL;

	// 如果有执行数据
	if (!execute_data) {
		// 如果生成器已经关闭，它只能有三个值：当前值，当前key,返回值。
		// 三个zval 按顺序存在 &generator->value 指针下
		/* If the generator has been closed, it can only hold on to three values: The value, key
		 * and retval. These three zvals are stored sequentially starting at &generator->value. */
		// 返回zval指针
		*table = &generator->value;
		// 返回值，3
		*n = 3;
		// 返回null
		return NULL;
	}

	// 如果生成器正在运行
	if (generator->flags & ZEND_GENERATOR_CURRENTLY_RUNNING) {
		// 如果生成器正在运行, 当前不能回收任何生成器里的元素。
		// 在执行中，执行数据的状态可能不一致（例如：因为垃圾回收可能在赋值中途被触发。
		// 所以这里不尝试去检查它。
		/* If the generator is currently running, we certainly won't be able to GC any values it
		 * holds on to. The execute_data state might be inconsistent during execution (e.g. because
		 * GC has been triggered in the middle of a variable reassignment), so we should not try
		 * to inspect it here. */
		// 不返回值表
		*table = NULL;
		// 变量数量0个
		*n = 0;
		// 返回 null
		return NULL;
	}

	// 创建回收队列
	zend_get_gc_buffer *gc_buffer = zend_get_gc_buffer_create();
	// 当前值，当前key，返回值，值表，都加到回收队列里
	zend_get_gc_buffer_add_zval(gc_buffer, &generator->value);
	zend_get_gc_buffer_add_zval(gc_buffer, &generator->key);
	zend_get_gc_buffer_add_zval(gc_buffer, &generator->retval);
	zend_get_gc_buffer_add_zval(gc_buffer, &generator->values);

	// 如果有冻结的执行堆栈
	if (UNEXPECTED(generator->frozen_call_stack)) {
		/* The frozen stack is linked in reverse order */
		// 把执行数据反着串起来，并返回最前一个执行数据
		call = zend_generator_revert_call_stack(generator->frozen_call_stack);
	}

	// 回收未完成的执行，并返回符号表。p1:执行数据，p2:调用执行数据，p3:回收缓冲区，p4:是否被yield暂停
	zend_unfinished_execution_gc_ex(execute_data, call, gc_buffer, true);

	// 如果有冻结的执行堆栈
	if (UNEXPECTED(generator->frozen_call_stack)) {
		// 把执行数据反着串起来，并返回最前一个执行数据
		zend_generator_revert_call_stack(call);
	}

	// 如果有父节点
	if (generator->node.parent) {
		// 把父节点的内置对象添加到垃圾回收
		zend_get_gc_buffer_add_obj(gc_buffer, &generator->node.parent->std);
	}

	// 列表添加到垃圾回收
	zend_get_gc_buffer_use(gc_buffer, table, n);
	// 如果执行数据有 符号表
	if (EX_CALL_INFO() & ZEND_CALL_HAS_SYMBOL_TABLE) {
		// 返回符号表
		return execute_data->symbol_table;
	// 没有符号表
	} else {
		// 返回null
		return NULL;
	}
}
/* }}} */

// ing4, 创建并返回生成器，p1:生成器内置对象的类型（类入口）
static zend_object *zend_generator_create(zend_class_entry *class_type) /* {{{ */
{
	zend_generator *generator;
	// 分配内存创建生成器
	generator = emalloc(sizeof(zend_generator));
	// 清空内存
	memset(generator, 0, sizeof(zend_generator));

	// 第一次使用时，这个key会自增，所以它会从0开始
	/* The key will be incremented on first use, so it'll start at 0 */
	generator->largest_used_integer_key = -1;

	// 清空返回值
	ZVAL_UNDEF(&generator->retval);
	// 清空values
	ZVAL_UNDEF(&generator->values);

	// 默认情况下，有一个只有一个node的树
	/* By default we have a tree of only one node */
	// 父节点为 null
	generator->node.parent = NULL;
	// 没有子节点
	generator->node.children = 0;
	// 根节点为 null
	generator->node.ptr.root = NULL;

	// 初始化生成器中的对象
	zend_object_std_init(&generator->std, class_type);
	// 对象使用生成器方法集
	generator->std.handlers = &zend_generator_handlers;

	// 返回生成器
	return (zend_object*)generator;
}
/* }}} */

// ing4, 实例化方法
static ZEND_COLD zend_function *zend_generator_get_constructor(zend_object *object) /* {{{ */
{
	// 报错：Generator 类保留为内部使用，不可以手工实例化
	zend_throw_error(NULL, "The \"Generator\" class is reserved for internal use and cannot be manually instantiated");
	// 返回null
	return NULL;
}
/* }}} */

// ing3, 给所有父节点添加 prev_execute_data，连成一串。p1:执行数据
ZEND_API zend_execute_data *zend_generator_check_placeholder_frame(zend_execute_data *ptr)
{
	// 执行数据中没有函数 并且 有$this (??)
	if (!ptr->func && Z_TYPE(ptr->This) == IS_OBJECT) {
		// 如果$this是生成器
		if (Z_OBJCE(ptr->This) == zend_ce_generator) {
			// 取出生成器
			zend_generator *generator = (zend_generator *) Z_OBJ(ptr->This);
			// 取出前一个执行数据
			zend_execute_data *prev = ptr->prev_execute_data;
			// 生成器必须 有父节点
			ZEND_ASSERT(generator->node.parent && "Placeholder only used with delegation");
			// 如果父节点有效
			while (generator->node.parent->node.parent) {
				// 添加前一个执行数据
				generator->execute_data->prev_execute_data = prev;
				// 当前执行数据作为前一个执行数据
				prev = generator->execute_data;
				// parent作为当前生成器
				generator = generator->node.parent;
			}
			// 最后一个生成器也要添加前一个执行数据
			generator->execute_data->prev_execute_data = prev;
			// 当前生成器的执行数据
			ptr = generator->execute_data;
		}
	}
	// 返回执行数据指针
	return ptr;
}

// ing3, 在生成器执行数据中抛出异常，没有异常时，使用当前执行数据中的异常，p1:生成器，p2:异常
static void zend_generator_throw_exception(zend_generator *generator, zval *exception)
{
	// 原执行数据
	zend_execute_data *original_execute_data = EG(current_execute_data);

	// 在生成器上下文中抛出异常。操作码指针-1，伪装成异常是从 YIELD 操作码发生的
	/* Throw the exception in the context of the generator. Decrementing the opline
	 * to pretend the exception happened during the YIELD opcode. */
	// 使用生成器执行数据作为当前执行数据
	EG(current_execute_data) = generator->execute_data;
	// 操作码指针-1
	generator->execute_data->opline--;

	// 如果有异常
	if (exception) {
		// 抛出异常
		zend_throw_exception_object(exception);
	// 没有异常
	} else {
		// 通过操作码，重新抛异常，p1:执行数据
		zend_rethrow_exception(EG(current_execute_data));
	}

	// 如果不能停止 数组/迭代器 的 yield from，异常会在遍历整个值表后遇到
	/* if we don't stop an array/iterator yield from, the exception will only reach the generator after the values were all iterated over */
	// 如果值表存在
	if (UNEXPECTED(Z_TYPE(generator->values) != IS_UNDEF)) {
		// 销毁值表
		zval_ptr_dtor(&generator->values);
		// 清空值表
		ZVAL_UNDEF(&generator->values);
	}

	// 操作码指针 +1
	generator->execute_data->opline++;
	// 恢复到原来的执行数据
	EG(current_execute_data) = original_execute_data;
}

// ing3, 添加子节点。p1:生成器，p2:新子节点
static void zend_generator_add_child(zend_generator *generator, zend_generator *child)
{
	// 
	zend_generator_node *node = &generator->node;

	// 如果没有子节点
	if (node->children == 0) {
		// 添加到单独子节点上
		node->child.single = child;
	// 如果有
	} else {
		// 如果有1个
		if (node->children == 1) {
			// 创建子节点哈希表
			HashTable *ht = emalloc(sizeof(HashTable));
			// 初始化哈希表
			zend_hash_init(ht, 0, NULL, NULL, 0);
			// 先把单独的子节点添加进去
			zend_hash_index_add_new_ptr(ht,
				(zend_ulong) node->child.single, node->child.single);
			// 哈希表关联到node
			node->child.ht = ht;
		}

		// 把新的子节点添加到哈希表里
		zend_hash_index_add_new_ptr(node->child.ht, (zend_ulong) child, child);
	}

	// 子节点数量 +1
	++node->children;
}

// ing3, 在生成器树中用 p2代替p1,并把p1作为p2的子节点
void zend_generator_yield_from(zend_generator *generator, zend_generator *from)
{
	// 生成器必须没有父节点
	ZEND_ASSERT(!generator->node.parent && "Already has parent?");
	// 清空此生成器与叶子节点的链接，并返回叶子节点 p1:生成器
	zend_generator *leaf = clear_link_to_leaf(generator);
	// 如果有叶子节点 并且 from节点没有父节点 和 叶子节点
	if (leaf && !from->node.parent && !from->node.ptr.leaf) {
		// 给from节点添加叶子节点，关联到叶子节点
		from->node.ptr.leaf = leaf;
		// 给叶子节点添加root节点，关联到from
		leaf->node.ptr.root = from;
	}
	// 当前生成器的父节点 是 from
	generator->node.parent = from;
	// 把当前生成器添加成from的子节点
	zend_generator_add_child(from, generator);
	// 生成器添加 【已启动】标记
	generator->flags |= ZEND_GENERATOR_DO_INIT;
}

// ing4, 更新生成器的root节点（追踪到最终的root）。p1:生成器
ZEND_API zend_generator *zend_generator_update_root(zend_generator *generator)
{
	// 取得父节点
	zend_generator *root = generator->node.parent;
	// 如果还有父节点
	while (root->node.parent) {
		// 继续追踪父节点
		root = root->node.parent;
	}

	// 清空此生成器与叶子节点的链接，并返回叶子节点 p1:生成器
	clear_link_to_leaf(root);
	// 让叶子节点关联到生成器
	root->node.ptr.leaf = generator;
	// 生成器关联root节点
	generator->node.ptr.root = root;
	// 返回root节点
	return root;
}

// ing3, 如果root在执行，返回 的最深单个子节点，否则返回生成器的最上父节点 p1:生成器，p2:root（也是生成器）
static zend_generator *get_new_root(zend_generator *generator, zend_generator *root)
{
	// 如果root有执行数据 并且只有一个子节点
	while (!root->execute_data && root->node.children == 1) {
		// 追踪 最深的 单个子节点
		root = root->node.child.single;
	}

	// 如果root有执行数据 （正在执行中）
	if (root->execute_data) {
		// 返回root
		return root;
	}

	// 这时找到一个多孩子的节点，但没找到root。不知带追踪哪个子节点，所以向另一个方向追踪
	/* We have reached a multi-child node haven't found the root yet. We don't know which
	 * child to follow, so perform the search from the other direction instead. */
	// 如果父节点正在执行
	while (generator->node.parent->execute_data) {
		// 追踪最深层父节点
		generator = generator->node.parent;
	}

	// 返回
	return generator;
}

// ing3, 更新当前生成器
ZEND_API zend_generator *zend_generator_update_current(zend_generator *generator)
{
	// 记录原root
	zend_generator *old_root = generator->node.ptr.root;
	// 原root不可以有执行数据（在运行中）
	ZEND_ASSERT(!old_root->execute_data && "Nothing to update?");

	// 如果root在执行，返回 的最深单个子节点，否则返回生成器的最上父节点 p1:生成器，p2:root（也是生成器）
	zend_generator *new_root = get_new_root(generator, old_root);
	// 原root的叶子指针必须指向当前生成器
	ZEND_ASSERT(old_root->node.ptr.leaf == generator);
	// 生成器的root指针，链接到新root
	generator->node.ptr.root = new_root;
	// 新root的叶子指针，链接到生成器
	new_root->node.ptr.leaf = generator;
	// 旧root的叶子，和生成器断开
	old_root->node.ptr.leaf = NULL;

	// 新root的父节点
	zend_generator *new_root_parent = new_root->node.parent;
	// 父节点必须存在
	ZEND_ASSERT(new_root_parent);
	// 生成器删除子节点。p1:生成器，p2:子节点
	zend_generator_remove_child(&new_root_parent->node, new_root);

	// 如果有异常 或 生成器中的内置对象已被销毁 
	if (EXPECTED(EG(exception) == NULL) && EXPECTED((OBJ_FLAGS(&generator->std) & IS_OBJ_DESTRUCTOR_CALLED) == 0)) {
		// 上一个操作码是 yield_from
		zend_op *yield_from = (zend_op *) new_root->execute_data->opline - 1;

		// 如果操作码确实是 ZEND_YIELD_FROM
		if (yield_from->opcode == ZEND_YIELD_FROM) {
			// 如果新root的父节点无返回值
			if (Z_ISUNDEF(new_root_parent->retval)) {
				// 在生成器上下文中抛出异常
				/* Throw the exception in the context of the generator */
				// 原当前执行数值
				zend_execute_data *original_execute_data = EG(current_execute_data);
				// 当前执行数据更新成 新root的执行数据
				EG(current_execute_data) = new_root->execute_data;
				// 如果新root就是此生成器
				if (new_root == generator) {
					// 原执行数据放在新root 执行数据的前一个
					new_root->execute_data->prev_execute_data = original_execute_data;
					
				// 如果新root不是此生成器
				} else {
					// 新root执行数据 的前一个 = 生成器的 假执行数据（用于追踪过程）
					new_root->execute_data->prev_execute_data = &generator->execute_fake;
					// 生成器的假执行数据 的前一个 = 原执行数据
					generator->execute_fake.prev_execute_data = original_execute_data;
				}

				// ZEND_YIELD(_FROM) 已经向前移动了，所以自减，回退到正确的位置
				/* ZEND_YIELD(_FROM) already advance, so decrement opline to throw from correct place */
				
				// 新root执行数据的操作码指针 -1
				new_root->execute_data->oplN1ne--;
				// 抛错：从中断的生成器中获取，无有效返回值
				zend_throw_exception(zend_ce_ClosedGeneratorException, "Generator yielded from aborted, no return value available", 0);

				// 恢复 当前执行数据
				EG(current_execute_data) = original_execute_data;

				// 如果 （旧root或生成器） 没有正在运行
				if (!((old_root ? old_root : generator)->flags & ZEND_GENERATOR_CURRENTLY_RUNNING)) {
					// 新root的父节点为空
					new_root->node.parent = NULL;
					// 释放新root父节点的内置对象
					OBJ_RELEASE(&new_root_parent->std);
					// 恢复生成器
					zend_generator_resume(generator);
					// 这里间接递归了
					// 更新并返回当前生成器
					return zend_generator_get_current(generator);
				}
			// 如果新root的父节点有返回值
			} else {
				// 销毁新root的值
				zval_ptr_dtor(&new_root->value);
				// 把新root父节点的值 复制给新root
				ZVAL_COPY(&new_root->value, &new_root_parent->value);
				// 把新root父节点的返回值 复制给 新root执行数据的 yield_from 操作码
				ZVAL_COPY(ZEND_CALL_VAR(new_root->execute_data, yield_from->result.var), &new_root_parent->retval);
			}
		}
	}

	// 清空新root的父节点
	new_root->node.parent = NULL;
	// 释放新root父节点的内置对象
	OBJ_RELEASE(&new_root_parent->std);
	// 返回新root
	return new_root;
}

// 100行
// ing3, 生成器的值表中，切换下一个有效值
static zend_result zend_generator_get_next_delegated_value(zend_generator *generator) /* {{{ */
{
	// 操作码指针 -1
	--generator->execute_data->opline;

	// 临时变量
	zval *value;
	// 如果生成器的值是数组
	if (Z_TYPE(generator->values) == IS_ARRAY) {
		// 取出 数组
		HashTable *ht = Z_ARR(generator->values);
		// 访问zval的 foreach位置 p1.u2.fe_pos
		HashPosition pos = Z_FE_POS(generator->values);

		// 如果是顺序数组
		if (HT_IS_PACKED(ht)) {
			do {
				// 如果位置超过有效位置
				if (UNEXPECTED(pos >= ht->nNumUsed)) {
					// 到达末尾
					/* Reached end of array */
					// 返回失败
					goto failure;
				}
				// 找到当前值
				value = &ht->arPacked[pos];
				// 迭代指针后移
				pos++;
			// 如果值无效，要继续找，找到有效的为止
			} while (Z_ISUNDEF_P(value));
			// 销毁生成器的 当前值
			zval_ptr_dtor(&generator->value);
			// 找到的值复制给当前值
			ZVAL_COPY(&generator->value, value);
			// 销毁当前key
			zval_ptr_dtor(&generator->key);
			// 更新当前key
			ZVAL_LONG(&generator->key, pos - 1);
		// 如果是哈希表
		} else {
			// 临时变量
			Bucket *p;
			//
			do {
				// 如果位置到达末尾
				if (UNEXPECTED(pos >= ht->nNumUsed)) {
					// 到达末尾
					/* Reached end of array */
					// 返回失败
					goto failure;
				}
				// 找到当前位置键值对
				p = &ht->arData[pos];
				// 取出值
				value = &p->val;
				// 位置后移
				pos++;
			// 如果值为未定义，还要继续找
			} while (Z_ISUNDEF_P(value));

			// 销毁生成器 的 当前值
			zval_ptr_dtor(&generator->value);
			// 找到的值复制到当前值
			ZVAL_COPY(&generator->value, value);
			// 销毁当前key
			zval_ptr_dtor(&generator->key);
			// 如果有key
			if (p->key) {
				// 更新生成器的当前key
				ZVAL_STR_COPY(&generator->key, p->key);
			// 如果没有key
			} else {
				// 使用哈希值
				ZVAL_LONG(&generator->key, p->h);
			}
		}
		// 更新迭代位置
		// 访问zval的 foreach位置 p1.u2.fe_pos
		Z_FE_POS(generator->values) = pos;
	// 生成器的值不是哈希表
	} else {
		// 把值转成迭代器
		zend_object_iterator *iter = (zend_object_iterator *) Z_OBJ(generator->values);

		// 如果 已经开始迭代
		if (iter->index++ > 0) {
			// 迭代器向前移动
			iter->funcs->move_forward(iter);
			// 如果有异常
			if (UNEXPECTED(EG(exception) != NULL)) {
				// 返回失败
				goto failure;
			}
		}

		// 如果迭代器当前位置无效
		if (iter->funcs->valid(iter) == FAILURE) {
			// 到达迭代末尾
			/* reached end of iteration */
			// 返回失败
			goto failure;
		}

		// 取得当前位置的值
		value = iter->funcs->get_current_data(iter);
		// 如果有异常 或 值无效
		if (UNEXPECTED(EG(exception) != NULL) || UNEXPECTED(!value)) {
			// 返回失败
			goto failure;
		}

		// 销毁迭代器当前值
		zval_ptr_dtor(&generator->value);
		// 复制当前值
		ZVAL_COPY(&generator->value, value);
		// 销毁生成器的当前 key
		zval_ptr_dtor(&generator->key);
		// 如果有获取当前 key的方法
		if (iter->funcs->get_current_key) {
			// 调用此方法
			iter->funcs->get_current_key(iter, &generator->key);
			// 如果有异常
			if (UNEXPECTED(EG(exception) != NULL)) {
				// 清空key
				ZVAL_UNDEF(&generator->key);
				// 返回失败
				goto failure;
			}
		// 如果没有此方法
		} else {
			// 使用迭代器的 迭代索引号 作为当前key
			ZVAL_LONG(&generator->key, iter->index);
		}
	}
	// 操作码指针 +1
	++generator->execute_data->opline;
	// 返回成功
	return SUCCESS;

// 失败
failure:
	// 销毁生成器的值
	zval_ptr_dtor(&generator->values);
	// 值复位成未定义
	ZVAL_UNDEF(&generator->values);

	// 操作码指针 +1
	++generator->execute_data->opline;
	// 返回失败
	return FAILURE;
}
/* }}} */

// 120行
// ing2, 恢复到原生成器，p1:原生成器
ZEND_API void zend_generator_resume(zend_generator *orig_generator) /* {{{ */
{
	// 更新并返回当前生成器
	zend_generator *generator = zend_generator_get_current(orig_generator);

	// 如果生成器已经关闭了，就不能恢复了
	/* The generator is already closed, thus can't resume */
	if (UNEXPECTED(!generator->execute_data)) {
		// 中断
		return;
	}

// 再试一次
try_again:
	// 如果生成器正在运行
	if (generator->flags & ZEND_GENERATOR_CURRENTLY_RUNNING) {
		// 报错：不可以恢复正在运行的生成器
		zend_throw_error(NULL, "Cannot resume an already running generator");
		// 中断
		return;
	}

	// 如果生成器没有启动 并且 没有有效当前值
	if (UNEXPECTED((orig_generator->flags & ZEND_GENERATOR_DO_INIT) != 0 && !Z_ISUNDEF(generator->value))) {
		// 如果从当前运行的生成器中获取值，它必须是初始状态
		/* We must not advance Generator if we yield from a Generator being currently run */
		// 原生成器标记成未启动
		orig_generator->flags &= ~ZEND_GENERATOR_DO_INIT;
		// 中断
		return;
	}

	// 删除 AT_FIRST_YIELD 标记
	/* Drop the AT_FIRST_YIELD flag */
	orig_generator->flags &= ~ZEND_GENERATOR_AT_FIRST_YIELD;

	// 保存 执行器全局变量
	/* Backup executor globals */
	// 保存原执行数据
	zend_execute_data *original_execute_data = EG(current_execute_data);
	
	// 取得运行时中的 jit_trace_num
	uint32_t original_jit_trace_num = EG(jit_trace_num);

	// 设置执行器全局变量
	/* Set executor globals */
	// 使用生成器执行数据作为当前插数据
	EG(current_execute_data) = generator->execute_data;
	// 运行时中的 jit_trace_num 复位成0
	EG(jit_trace_num) = 0;

	// 需要追踪器直来像：生成器函数的调用来自 任何正在运行的方法（例如 next())
	// 必须把生成器的执行数据 链接到  调用者 执行数据
	/* We want the backtrace to look as if the generator function was
	 * called from whatever method we are current running (e.g. next()).
	 * So we have to link generator call frame with caller call frame. */
	// 如果生成器和原生成器相同
	if (generator == orig_generator) {
		// 生成器的执行数据的前一个执行数据 ，更新成原执行数据
		generator->execute_data->prev_execute_data = original_execute_data;
	} else {
		// 需要时需要 堆栈执中中的 执行数据空间来保存真正的 堆栈追踪 
		/* We need some execute_data placeholder in stacktrace to be replaced
		 * by the real stack trace when needed */
		// 生成器的执行数据的前一个执行数据 ，更新成原生成器的 假执行数据
		generator->execute_data->prev_execute_data = &orig_generator->execute_fake;
		// 原生成器的假执行数据 的前一个执行数据 ，更新成原执行数据
		orig_generator->execute_fake.prev_execute_data = original_execute_data;
	}

	// 保证在 执行数据 交换后执行，来生成合适的 堆栈追踪
	/* Ensure this is run after executor_data swap to have a proper stack trace */
	if (UNEXPECTED(!Z_ISUNDEF(generator->values))) {
		// 生成器的值表中，切换下一个有效值
		if (EXPECTED(zend_generator_get_next_delegated_value(generator) == SUCCESS)) {
			// 恢复执行器全局变量
			/* Restore executor globals */
			// 恢复执行数据
			EG(current_execute_data) = original_execute_data;
			// 恢复运行时 jit_trace_num
			EG(jit_trace_num) = original_jit_trace_num;
			// 删除 已启动 标记
			orig_generator->flags &= ~ZEND_GENERATOR_DO_INIT;
			// 返回
			return;
		}
		// 如果不再有委托值，在 "yield from" 表达式后恢复生成器
		/* If there are no more delegated values, resume the generator
		 * after the "yield from" expression. */
	}

	// 如果有冻结的调用堆栈
	if (UNEXPECTED(generator->frozen_call_stack)) {
		// 恢复 冻结的调用堆栈
		/* Restore frozen call-stack */
		// 生成存储（整理）生成器的调用堆栈
		zend_generator_restore_call_stack(generator);
	}

	// 恢复执行
	/* Resume execution */
	// 添加【在运行中】标记，如果有活跃fiber，添加【在fiber中】标记
	generator->flags |= ZEND_GENERATOR_CURRENTLY_RUNNING
						| (EG(active_fiber) ? ZEND_GENERATOR_IN_FIBER : 0);
	// 如果不允许观察者
	if (!ZEND_OBSERVER_ENABLED) {
		// 执行 生成器
		zend_execute_ex(generator->execute_data);
	} else {
		// 调用开始函数列表中的函数， zend_generators.c 用到
		zend_observer_generator_resume(generator->execute_data);
		// 执行 生成器
		zend_execute_ex(generator->execute_data);
		// 如果生成器 有执行数据
		if (generator->execute_data) {
			// 在最终返回中，这个会从 ZEND_GENERATOR_RETURN 调用
			/* On the final return, this will be called from ZEND_GENERATOR_RETURN */
			// 结束观察者调用
			zend_observer_fcall_end(generator->execute_data, &generator->value);
		}
	}
	// 删除 【正在运行】 和【在fiber中】 两个标记
	generator->flags &= ~(ZEND_GENERATOR_CURRENTLY_RUNNING | ZEND_GENERATOR_IN_FIBER);

	// 生成器冻结调用堆栈为空
	generator->frozen_call_stack = NULL;
	// 如果生成器在执行中 并且 有call方法
	if (EXPECTED(generator->execute_data) &&
		UNEXPECTED(generator->execute_data->call)) {
		// 冻结调用堆栈
		/* Frize call-stack */
		// 冻结执行数据堆栈，p1:执行数据
		generator->frozen_call_stack = zend_generator_freeze_call_stack(generator->execute_data);
	}

	// 保存执行全局变量
	/* Restore executor globals */
	EG(current_execute_data) = original_execute_data;
	// 恢复运行时 jit_trace_num
	EG(jit_trace_num) = original_jit_trace_num;

	// 如果生成器抛出了异常，需要内部 在它的父域中重新抛出这个异常
	// 这种情况 执行 yield from，异常必须被抛到它的执行数据中（看前面的 if (check_yield_from)）
	/* If an exception was thrown in the generator we have to internally
	 * rethrow it in the parent scope.
	 * In case we did yield from, the Exception must be rethrown into
	 * its calling frame (see above in if (check_yield_from). */
	if (UNEXPECTED(EG(exception) != NULL)) {
		if (generator == orig_generator) {
			// 关闭生成器。p1:生成器，p2:运行是否已完成
			zend_generator_close(generator, 0);
			// 如果没有当前执行数据
			if (!EG(current_execute_data)) {
				// 抛出内部异常。
				zend_throw_exception_internal(NULL);
			// 如果当前在函数中 并且 是用户定义函数
			} else if (EG(current_execute_data)->func &&
					ZEND_USER_CODE(EG(current_execute_data)->func->common.type)) {
				// 通过操作码，重新抛异常，p1:执行数据
				zend_rethrow_exception(EG(current_execute_data));
			}
		} else {
			// 更新并返回当前生成器
			generator = zend_generator_get_current(orig_generator);
			// 在生成器执行数据中抛出异常，没有异常时，使用当前执行数据中的异常，p1:生成器，p2:异常
			zend_generator_throw_exception(generator, NULL);
			// 原生成器中删除 已启动 标记
			orig_generator->flags &= ~ZEND_GENERATOR_DO_INIT;
			// 再来一次
			goto try_again;
		}
	}

	// yield from 已经使用过了，尝试恢复另一个
	/* yield from was used, try another resume. */
	// （如果当前生成器不是原生成器 并且当前生成器没有返回值）或（当前生成器在运行中 并且 前一个操作码是 ZEND_YIELD_FROM）
	if (UNEXPECTED((generator != orig_generator && !Z_ISUNDEF(generator->retval)) || (generator->execute_data && (generator->execute_data->opline - 1)->opcode == ZEND_YIELD_FROM))) {
		// 更新并返回当前生成器
		generator = zend_generator_get_current(orig_generator);
		// 再试一次
		goto try_again;
	}

	// 删除启动标记
	orig_generator->flags &= ~ZEND_GENERATOR_DO_INIT;
}
/* }}} */


// ing3, 保证生成器初始化
static inline void zend_generator_ensure_initialized(zend_generator *generator) /* {{{ */
{
	// 如果生成器的当前值未无效 并且 生成器有执行数据 并且 生成器没有父节点
	if (UNEXPECTED(Z_TYPE(generator->value) == IS_UNDEF) && EXPECTED(generator->execute_data) && EXPECTED(generator->node.parent == NULL)) {
		// 恢复生成器
		zend_generator_resume(generator);
		// 添加：【AT_FIRST_YIELD】标记
		generator->flags |= ZEND_GENERATOR_AT_FIRST_YIELD;
	}
}
/* }}} */

// ing3, 复位生成器
static inline void zend_generator_rewind(zend_generator *generator) /* {{{ */
{
	// 保证生成器初始化
	zend_generator_ensure_initialized(generator);

	// 如果没有 【AT_FIRST_YIELD】标记
	if (!(generator->flags & ZEND_GENERATOR_AT_FIRST_YIELD)) {
		// 抛出异常 无法复位一个正在运行的生成器
		zend_throw_exception(NULL, "Cannot rewind a generator that was already run", 0);
	}
}
/* }}} */

// ing3, 复位生成器
/* {{{ Rewind the generator */
ZEND_METHOD(Generator, rewind)
{
	zend_generator *generator;

	// 不可以有参数
	ZEND_PARSE_PARAMETERS_NONE();

	// $this 生成器
	generator = (zend_generator *) Z_OBJ_P(ZEND_THIS);
	// 复位生成器
	zend_generator_rewind(generator);
}
/* }}} */

// ing3, 验证此生成器是否有效
/* {{{ Check whether the generator is valid */
ZEND_METHOD(Generator, valid)
{
	zend_generator *generator;

	// 不可以传入参数
	ZEND_PARSE_PARAMETERS_NONE();

	// $this 生成器
	generator = (zend_generator *) Z_OBJ_P(ZEND_THIS);
	// 保证生成器初始化
	zend_generator_ensure_initialized(generator);
	// 更新并返回当前生成器
	zend_generator_get_current(generator);

	// 有执行数据算有效
	RETURN_BOOL(EXPECTED(generator->execute_data != NULL));
}
/* }}} */

// ing3, 取回当前值
/* {{{ Get the current value */
ZEND_METHOD(Generator, current)
{
	zend_generator *generator, *root;
	// 不可以传入参数
	ZEND_PARSE_PARAMETERS_NONE();
	// $this 生成器
	generator = (zend_generator *) Z_OBJ_P(ZEND_THIS);
	// 保证生成器初始化
	zend_generator_ensure_initialized(generator);
	// 更新并返回当前生成器
	root = zend_generator_get_current(generator);
	// 如果有执行数据 并且 当前值有效
	if (EXPECTED(generator->execute_data != NULL && Z_TYPE(root->value) != IS_UNDEF)) {
		// 解引用并 返回当前值副本
		RETURN_COPY_DEREF(&root->value);
	}
}
/* }}} */

// ing3, 取回当前key
/* {{{ Get the current key */
ZEND_METHOD(Generator, key)
{
	zend_generator *generator, *root;
	// 不可以传入参数
	ZEND_PARSE_PARAMETERS_NONE();
	// $this 生成器
	generator = (zend_generator *) Z_OBJ_P(ZEND_THIS);
	// 保证生成器初始化
	zend_generator_ensure_initialized(generator);
	// 更新并返回当前生成器
	root = zend_generator_get_current(generator);
	// 如果生成器有执行数据 并且 key有效
	if (EXPECTED(generator->execute_data != NULL && Z_TYPE(root->key) != IS_UNDEF)) {
		// 解引用并 返回当前 key 副本
		RETURN_COPY_DEREF(&root->key);
	}
}
/* }}} */

// ing2, 生成器前进
/* {{{ Advances the generator */
ZEND_METHOD(Generator, next)
{
	zend_generator *generator;
	// 不可以传入参数
	ZEND_PARSE_PARAMETERS_NONE();
	// $this 生成器
	generator = (zend_generator *) Z_OBJ_P(ZEND_THIS);
	// 保证生成器初始化
	zend_generator_ensure_initialized(generator);
	// 恢复生成器
	zend_generator_resume(generator);
}
/* }}} */

// ing3, 发送值给生成器
// 重点关注 ZEND_GENERATOR_CURRENTLY_RUNNING 和 有->execute_data 之间的区别
/* {{{ Sends a value to the generator */
ZEND_METHOD(Generator, send)
{
	zval *value;
	zend_generator *generator, *root;

	// 必须是1个参数
	ZEND_PARSE_PARAMETERS_START(1, 1)
		// 取得参数 value
		Z_PARAM_ZVAL(value)
	ZEND_PARSE_PARAMETERS_END();

	// $this 生成器
	generator = (zend_generator *) Z_OBJ_P(ZEND_THIS);
	// 保证生成器初始化
	zend_generator_ensure_initialized(generator);

	// 生成器已经关闭了，无法发送
	/* The generator is already closed, thus can't send anything */
	if (UNEXPECTED(!generator->execute_data)) {
		// 中断
		return;
	}
	// 更新并返回当前生成器
	root = zend_generator_get_current(generator);
	// 把发送值放在 目标 VAR 位置，如果它存在的话
	/* Put sent value in the target VAR slot, if it is used */
	// 如果有目标位置 并且 生成器当前没在运行中
	if (root->send_target && !(root->flags & ZEND_GENERATOR_CURRENTLY_RUNNING)) {
		// 把value复制到目标位置
		ZVAL_COPY(root->send_target, value);
	}
	// 恢复生成器
	zend_generator_resume(generator);
	// 更新并返回当前生成器
	root = zend_generator_get_current(generator);
	// 如果生成器有执行数据
	if (EXPECTED(generator->execute_data)) {
		// 返回当前值副本
		RETURN_COPY_DEREF(&root->value);
	}
}
/* }}} */

// ing3, 抛出异常
/* {{{ Throws an exception into the generator */
ZEND_METHOD(Generator, throw)
{
	zval *exception;
	zend_generator *generator;

	// 必须是1个参数
	ZEND_PARSE_PARAMETERS_START(1, 1)
		// 取得带类型的参数 exception
		Z_PARAM_OBJECT_OF_CLASS(exception, zend_ce_throwable);
	ZEND_PARSE_PARAMETERS_END();

	// 增加引用次数
	Z_TRY_ADDREF_P(exception);
	
	// $this 生成器
	generator = (zend_generator *) Z_OBJ_P(ZEND_THIS);
	// 保证生成器初始化
	zend_generator_ensure_initialized(generator);

	// 如果生成器有执行数据
	if (generator->execute_data) {
		// 更新并返回当前生成器
		zend_generator *root = zend_generator_get_current(generator);
		// 在生成器执行数据中抛出异常，没有异常时，使用当前执行数据中的异常，p1:生成器，p2:异常
		zend_generator_throw_exception(root, exception);

		// 恢复生成器
		zend_generator_resume(generator);
		// 更新并返回当前生成器
		root = zend_generator_get_current(generator);
		// 如果生成器有执行数据
		if (generator->execute_data) {
			// 返回当前值副本
			RETURN_COPY_DEREF(&root->value);
		}
	} else {
		// 如果生成器已经关闭了，在当前上下文中抛异常
		/* If the generator is already closed throw the exception in the
		 * current context */
		// 抛出异常
		zend_throw_exception_object(exception);
	}
}
/* }}} */

// ing3, 获取返回值
/* {{{ Retrieves the return value of the generator */
ZEND_METHOD(Generator, getReturn)
{
	zend_generator *generator;

	ZEND_PARSE_PARAMETERS_NONE();

	generator = (zend_generator *) Z_OBJ_P(ZEND_THIS);
	// 保证生成器初始化
	zend_generator_ensure_initialized(generator);
	// 如果有异常
	if (UNEXPECTED(EG(exception))) {
		// 返回
		return;
	}

	// 如果有返回值
	if (Z_ISUNDEF(generator->retval)) {
		// 生成器还没返回，报错
		/* Generator hasn't returned yet -> error! */
		// 报错：不可以从未返回的生成器中获取返回值
		zend_throw_exception(NULL,
			"Cannot get return value of a generator that hasn't returned", 0);
		// 返回
		return;
	}

	// 返回 返回值 副本
	ZVAL_COPY(return_value, &generator->retval);
}
/* }}} */

/* get_iterator implementation */

// ing4, 销毁迭代器
static void zend_generator_iterator_dtor(zend_object_iterator *iterator) /* {{{ */
{
	// 销毁没有引用次数的对象，有引用次数的添加到gc回收周期里。并没有销毁 zval本身。
	zval_ptr_dtor(&iterator->data);
}
/* }}} */

// ing3, 检验迭代器 指向的 当前 生成器 是否有效
static int zend_generator_iterator_valid(zend_object_iterator *iterator) /* {{{ */
{
	// 取出迭代器
	zend_generator *generator = (zend_generator*)Z_OBJ(iterator->data);
	// 保证生成器初始化
	zend_generator_ensure_initialized(generator);
	// 更新并返回当前生成器
	zend_generator_get_current(generator);
	// 如果有执行数据 ？返回成功 ：返回失败
	return generator->execute_data ? SUCCESS : FAILURE;
}
/* }}} */

// ing3, 取得迭代器当前指向的生成器的 当前值
static zval *zend_generator_iterator_get_data(zend_object_iterator *iterator) /* {{{ */
{
	zend_generator *generator = (zend_generator*)Z_OBJ(iterator->data), *root;
	// 保证生成器初始化
	zend_generator_ensure_initialized(generator);
	// 更新并返回当前生成器
	root = zend_generator_get_current(generator);
	// 返回当前值
	return &root->value;
}
/* }}} */

// ing3, 取得迭代器当前指向的生成器的 当前key
static void zend_generator_iterator_get_key(zend_object_iterator *iterator, zval *key) /* {{{ */
{
	zend_generator *generator = (zend_generator*)Z_OBJ(iterator->data), *root;
	// 保证生成器初始化
	zend_generator_ensure_initialized(generator);
	// 更新并返回当前生成器
	root = zend_generator_get_current(generator);

	// 如果当前key有效
	if (EXPECTED(Z_TYPE(root->key) != IS_UNDEF)) {
		// 取出key
		zval *zv = &root->key;
		// 解引用 并 创建副本放到key里
		ZVAL_COPY_DEREF(key, zv);
	// key无效
	} else {
		// 值为null
		ZVAL_NULL(key);
	}
}
/* }}} */

// ing3, 找到迭代器指向的当前生成器，恢复此生成器
static void zend_generator_iterator_move_forward(zend_object_iterator *iterator) /* {{{ */
{
	zend_generator *generator = (zend_generator*)Z_OBJ(iterator->data);
	// 保证生成器初始化
	zend_generator_ensure_initialized(generator);

	// 恢复生成器
	zend_generator_resume(generator);
}
/* }}} */

// 复位迭代器
static void zend_generator_iterator_rewind(zend_object_iterator *iterator) /* {{{ */
{
	zend_generator *generator = (zend_generator*)Z_OBJ(iterator->data);
	// 复位生成器
	zend_generator_rewind(generator);
}
/* }}} */

// ing3, 取得回收的数据
static HashTable *zend_generator_iterator_get_gc(
		zend_object_iterator *iterator, zval **table, int *n)
{
	// 回收的数据
	*table = &iterator->data;
	// 数量 1
	*n = 1;
	return NULL;
}

static const zend_object_iterator_funcs zend_generator_iterator_functions = {
	zend_generator_iterator_dtor,
	zend_generator_iterator_valid,
	zend_generator_iterator_get_data,
	zend_generator_iterator_get_key,
	zend_generator_iterator_move_forward,
	zend_generator_iterator_rewind,
	NULL,
	zend_generator_iterator_get_gc,
};
// 在迭代器api中，by_ref是整数
/* by_ref is int due to Iterator API */
// ing3, 创建生成器迭代器，p1:没用到，p2:存入迭代器中的数据，p3:是否引用传递
zend_object_iterator *zend_generator_get_iterator(zend_class_entry *ce, zval *object, int by_ref) /* {{{ */
{
	// 迭代器
	zend_object_iterator *iterator;
	// 生成器
	zend_generator *generator = (zend_generator*)Z_OBJ_P(object);

	// 如果生成器没有执行数据
	if (!generator->execute_data) {
		// 报错：不可以遍历一个关闭的生成器
		zend_throw_exception(NULL, "Cannot traverse an already closed generator", 0);
		// 返回null
		return NULL;
	}

	// 如果要求通过引用传递 并且 执行数据中的函数不通过引用返回
	if (UNEXPECTED(by_ref) && !(generator->execute_data->func->op_array.fn_flags & ZEND_ACC_RETURN_REFERENCE)) {
		// 报错： 只有通过引用返回的生成器才能通过引用迭代
		zend_throw_exception(NULL, "You can only iterate a generator by-reference if it declared that it yields by-reference", 0);
		return NULL;
	}
	// 分配内存创建迭代器
	iterator = emalloc(sizeof(zend_object_iterator));
	// 初始化迭代器
	zend_iterator_init(iterator);

	// 迭代器方法，使用生成器迭代方法
	iterator->funcs = &zend_generator_iterator_functions;
	// 传入的对象复制到迭代器中
	ZVAL_OBJ_COPY(&iterator->data, Z_OBJ_P(object));

	// 返回迭代器
	return iterator;
}
/* }}} */

// ing4, 注册 Generator 类
void zend_register_generator_ce(void) /* {{{ */
{
	// 注册 Generator 类
	zend_ce_generator = register_class_Generator(zend_ce_iterator);
	// 创建方法
	zend_ce_generator->create_object = zend_generator_create;
	// get_iterator 方法必须在实现接口后加入
	/* get_iterator has to be assigned *after* implementing the interface */
	// 使用迭代器专有的 get_iterator 方法
	zend_ce_generator->get_iterator = zend_generator_get_iterator;

	// 使用标准对象处理方法
	memcpy(&zend_generator_handlers, &std_object_handlers, sizeof(zend_object_handlers));
	// 释放对象
	zend_generator_handlers.free_obj = zend_generator_free_storage;
	// 销毁对象
	zend_generator_handlers.dtor_obj = zend_generator_dtor_storage;
	// 回收对象
	zend_generator_handlers.get_gc = zend_generator_get_gc;
	// 不可以克隆
	zend_generator_handlers.clone_obj = NULL;
	// 获取构造器
	zend_generator_handlers.get_constructor = zend_generator_get_constructor;

	// 注册类 ClosedGeneratorException
	zend_ce_ClosedGeneratorException = register_class_ClosedGeneratorException(zend_ce_exception);
}
/* }}} */
