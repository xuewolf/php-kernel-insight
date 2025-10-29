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
   | Authors: Levi Morrison <levim@php.net>                               |
   |          Sammy Kaye Powers <sammyk@php.net>                          |
   +----------------------------------------------------------------------+
*/

#include "zend_observer.h"

#include "zend_extensions.h"
#include "zend_llist.h"
#include "zend_vm.h"

// , 取得一个函数的观察者函数列表。-1会取到什么呢
// 每个函数都有这个列表，在 ->common->run_time_cache__ptr 这个地方，参看 _zend_function 结构体
#define ZEND_OBSERVER_DATA(function) \
	/* ((void**)RUN_TIME_CACHE(op_array))[handle] : zend_compile.h */\
	/* => ((void**)RUN_TIME_CACHE(&(function)->common))[zend_observer_fcall_op_array_extension] */\
	/* => (ZEND_MAP_PTR_GET(&(function)->common)->run_time_cache)[zend_observer_fcall_op_array_extension] */\
	ZEND_OP_ARRAY_EXTENSION((&(function)->common), zend_observer_fcall_op_array_extension)

// ing4, 偏移量2，未使用的观察者
#define ZEND_OBSERVER_NOT_OBSERVED ((void *) 2)

// ing3, 检验是否是可观察的函数
// function->common.run_time_cache__ptr && !通过 TRAMPOLINE 调用
#define ZEND_OBSERVABLE_FN(function) \
	(ZEND_MAP_PTR(function->common.run_time_cache) && !(function->common.fn_flags & ZEND_ACC_CALL_VIA_TRAMPOLINE))

// 一堆链表
// 观察者调用列表。每个观察者函数被调用后会返回 zend_observer_fcall_handlers 实例
zend_llist zend_observers_fcall_list;
// 观察者声名回调，_zend_observer_function_declared_notify 中遍历
zend_llist zend_observer_function_declared_callbacks;
// 观察者链接回调，_zend_observer_class_linked_notify 中遍历 
zend_llist zend_observer_class_linked_callbacks;
// 观察者出错回调，_zend_observer_error_notify 中遍历 
zend_llist zend_observer_error_callbacks;

// 观察者 fiber 初始化回调列表， zend_observer_fiber_init_notify 中遍历
zend_llist zend_observer_fiber_init;
// 观察者 fiber 切换回调列表， 在 zend_observer_fiber_switch_notify 中遍历 
zend_llist zend_observer_fiber_switch;
// 观察者 fiber 销毁回调列表， 在 zend_observer_fiber_destroy_notify 中遍历 
zend_llist zend_observer_fiber_destroy;

// 默认值是-1, 启用后不是-1. 参见 ZEND_OBSERVER_ENABLED
int zend_observer_fcall_op_array_extension;
bool zend_observer_errors_observed;
bool zend_observer_function_declared_observed;
bool zend_observer_class_linked_observed;

// 当前观察者框架
ZEND_TLS zend_execute_data *current_observed_frame;

// 只在 minit或stratup 过程中调用
// Call during minit/startup ONLY
// ing3, 注册新观察者 函数
ZEND_API void zend_observer_fcall_register(zend_observer_fcall_init init)
{
	// 列表里添加元素
	zend_llist_add_element(&zend_observers_fcall_list, &init);
}

// 在 MINITs 之前被引擎调用
// Called by engine before MINITs
// ing3, 启动 观察者, main.c
ZEND_API void zend_observer_startup(void)
{
	// 初始化7个链表
	zend_llist_init(&zend_observers_fcall_list, sizeof(zend_observer_fcall_init), NULL, 1);
	zend_llist_init(&zend_observer_function_declared_callbacks, sizeof(zend_observer_function_declared_cb), NULL, 1);
	zend_llist_init(&zend_observer_class_linked_callbacks, sizeof(zend_observer_class_linked_cb), NULL, 1);
	zend_llist_init(&zend_observer_error_callbacks, sizeof(zend_observer_error_cb), NULL, 1);
	zend_llist_init(&zend_observer_fiber_init, sizeof(zend_observer_fiber_init_handler), NULL, 1);
	zend_llist_init(&zend_observer_fiber_switch, sizeof(zend_observer_fiber_switch_handler), NULL, 1);
	zend_llist_init(&zend_observer_fiber_destroy, sizeof(zend_observer_fiber_destroy_handler), NULL, 1);
	// 默认值是 -1，启用后是其他值
	zend_observer_fcall_op_array_extension = -1;
}

// ing2, 这个和扩展有关，由引擎调用：main.c。在zend_observer_startup后面被调用
ZEND_API void zend_observer_post_startup(void)
{
	// 如果 观察者列表中有元素，才有效
	if (zend_observers_fcall_list.count) {
		// 不会获取一个扩展中的函数， 除非一个扩展安装了观察者。
		// 那样会给它分配一个开始和一个结束 指针。
		/* We don't want to get an extension handle unless an ext installs an observer
		 * Allocate each a begin and an end pointer */
		// 传入模块名和处理器数量，返原有处理器数量
		zend_observer_fcall_op_array_extension =
			zend_get_op_array_extension_handles("Zend Observer", (int) zend_observers_fcall_list.count * 2);

		// #define ZEND_VM_SET_OPCODE_HANDLER(opline) zend_vm_set_opcode_handler(opline) : /Zend/zend_vm_execute.h
		
		// ZEND_CALL_TRAMPOLINE 有 SPEC(OBSERVER)，但 zend_init_call_trampoline_op() 在所有扩展注册成观察者前调用。
		// 所以当需要观察时，需要调整观察者函数的偏移量。
		
		/* ZEND_CALL_TRAMPOLINE has SPEC(OBSERVER) but zend_init_call_trampoline_op()
		 * is called before any extensions have registered as an observer. So we
		 * adjust the offset to the observed handler when we know we need to observe. */
		// 给操 弹跳作码链接它对应的处理器
		ZEND_VM_SET_OPCODE_HANDLER(&EG(call_trampoline_op));

		// ZEND_HANDLE_EXCEPTION 也有 SPEC(OBSERVER) ，并且zend_init_exception_op()调用时还 没有观察者扩展。
		/* ZEND_HANDLE_EXCEPTION also has SPEC(OBSERVER) and no observer extensions
		 * exist when zend_init_exception_op() is called. */
		// 给操 弹跳作码（3个） 链接它对应的处理器
		ZEND_VM_SET_OPCODE_HANDLER(EG(exception_op));
		ZEND_VM_SET_OPCODE_HANDLER(EG(exception_op) + 1);
		ZEND_VM_SET_OPCODE_HANDLER(EG(exception_op) + 2);
		
		// 增加一个观察者临时变量，来存储前一个 观察框架
		// Add an observer temporary to store previous observed frames
		zend_internal_function *zif;
		// 遍历函数表里所有函数
		ZEND_HASH_FOREACH_PTR(CG(function_table), zif) {
			// 给每个函数增加临时变量数
			++zif->T;
		} ZEND_HASH_FOREACH_END();
		
		//
		zend_class_entry *ce;
		// 遍历类表里所有类
		ZEND_HASH_MAP_FOREACH_PTR(CG(class_table), ce) {
			// 遍历类的所有方法
			ZEND_HASH_MAP_FOREACH_PTR(&ce->function_table, zif) {
				// 增加临时变量数
				++zif->T;
			} ZEND_HASH_FOREACH_END();
		} ZEND_HASH_FOREACH_END();
	}
}

// ing4, 激活
ZEND_API void zend_observer_activate(void)
{
	// 激活时把当前观察框架置空
	current_observed_frame = NULL;
}

// ing3, 关闭观察者
ZEND_API void zend_observer_shutdown(void)
{
	// 销毁7个链表
	zend_llist_destroy(&zend_observers_fcall_list);
	zend_llist_destroy(&zend_observer_function_declared_callbacks);
	zend_llist_destroy(&zend_observer_class_linked_callbacks);
	zend_llist_destroy(&zend_observer_error_callbacks);
	zend_llist_destroy(&zend_observer_fiber_init);
	zend_llist_destroy(&zend_observer_fiber_switch);
	zend_llist_destroy(&zend_observer_fiber_destroy);
}

// ing4, 给函数安装观察者。一次安装所有，不可选。
static void zend_observer_fcall_install(zend_execute_data *execute_data)
{
	// 观察者 列表
	zend_llist *list = &zend_observers_fcall_list;
	// 取得执行上下文中的函数名
	zend_function *function = execute_data->func;
	// 观察函数列表必须存在
	// ZEND_MAP_PTR_GET((op_array)->run_time_cache)
	ZEND_ASSERT(RUN_TIME_CACHE(&function->common));
	// 开始函数列表的开头
	zend_observer_fcall_begin_handler *begin_handlers = (zend_observer_fcall_begin_handler *)&ZEND_OBSERVER_DATA(function);
	// 开始函数列表的结尾
	zend_observer_fcall_end_handler *end_handlers = (zend_observer_fcall_end_handler *)begin_handlers + list->count, 
	
	// 它也是结束函数列表的开头
	*end_handlers_start = end_handlers;
	// 开始和结束列表的第一个都标记成，未启用观察者
	*begin_handlers = ZEND_OBSERVER_NOT_OBSERVED;
	*end_handlers = ZEND_OBSERVER_NOT_OBSERVED;
	
	// 正序遍历观察者 列表
	for (zend_llist_element *element = list->head; element; element = element->next) {
		// 初始化函数，返回 zend_observer_fcall_handlers
		zend_observer_fcall_init init;
		// 用链表元素中的数据，创建初始化函数
		memcpy(&init, element->data, sizeof init);
		// 对上下文调用此函数，获取两个观察函数
		zend_observer_fcall_handlers handlers = init(execute_data);
		// 如果有开始函数
		if (handlers.begin) {
			// 添加一个 开始函数
			*(begin_handlers++) = handlers.begin;
		}
		// 如果有结束函数
		if (handlers.end) {
			// 添加一个 结束函数
			*(end_handlers++) = handlers.end;
		}
	}
	
	// 结束函数 要倒序调用
	// end handlers are executed in reverse order
	// 把结束函数反序，end_handlers 是有效数据的结尾，所以只把有效数据段反序。
	for (--end_handlers; end_handlers_start < end_handlers; --end_handlers, ++end_handlers_start) {
		// 开头第n个和末尾第n个位置交换
		zend_observer_fcall_end_handler tmp = *end_handlers;
		*end_handlers = *end_handlers_start;
		*end_handlers_start = tmp;
	}
}

// ing3, 从处理器列表中，删除一个处理器，这里使用 void 抽象指针
static bool zend_observer_remove_handler(void **first_handler, void *old_handler) {
	// 已注册的调用列表 元素数
	size_t registered_observers = zend_observers_fcall_list.count;
	// 取得最后一个处理器的指针
	void **last_handler = first_handler + registered_observers - 1;
	// 从头开始遍历每个处理器
	for (void **cur_handler = first_handler; cur_handler <= last_handler; ++cur_handler) {
		// 如果找到要删除的处理器
		if (*cur_handler == old_handler) {
			// 如果只有一个处理器 或 （要删除的是第一个处理器 并且 第个是null（这里允许有空处理器））
			if (registered_observers == 1 || (cur_handler == first_handler && cur_handler[1] == NULL)) {
				// 当前处理器标记为未启用
				*cur_handler = ZEND_OBSERVER_NOT_OBSERVED;
			// 否则
			} else {
				// 如果找到的不是最后一个处理器
				if (cur_handler != last_handler) {
					// 把后面的所有数据，向前移动一个 cur_handler （普通指针）的位置？
					// 需要进一步测试
					memmove(cur_handler, cur_handler + 1, sizeof(cur_handler) * (last_handler - cur_handler));
				// 如果找到的是最后一个处理器
				} else {
					// 直接清空指针
					*last_handler = NULL;
				}
			}
			// 操作成功
			return true;
		}
	}
	// 查找失败
	return false;
}

// ing3, 添加开始处理器。空间不够时不会自动开辟，只会使用已有空间
ZEND_API void zend_observer_add_begin_handler(zend_function *function, zend_observer_fcall_begin_handler begin) {
	// 现有观察方法数量
	size_t registered_observers = zend_observers_fcall_list.count;
	// 取得最后一个开始调用函数
	// 这样可以一次定义多个相同类型的变量
	zend_observer_fcall_begin_handler *first_handler = (void *)&ZEND_OBSERVER_DATA(function), 
	// 最后一个开始函数
	*last_handler = first_handler + registered_observers - 1;
	
	// 如果列表第一个是未启用观者
	if (*first_handler == ZEND_OBSERVER_NOT_OBSERVED) {
		// 直接放在列表开头
		*first_handler = begin;
	// 如果是有效观察者
	} else {
		// 从开头第二个开始，遍历每个开始调用处理器
		for (zend_observer_fcall_begin_handler *cur_handler = first_handler + 1; cur_handler <= last_handler; ++cur_handler) {
			// 找到的第一个为空的
			if (*cur_handler == NULL) {
				// 设置为begin
				*cur_handler = begin;
				return;
			}
		}
		// 没有空间给新的处理器时，禁止调用此方法
		// there's no space for new handlers, then it's forbidden to call this function
		// 不可到达这里
		ZEND_UNREACHABLE();
	}
}

// ing3, 删除 开始函数，函数指针列表 和 开始函数
ZEND_API bool zend_observer_remove_begin_handler(zend_function *function, zend_observer_fcall_begin_handler begin) {
	// 从开头开始删
	return zend_observer_remove_handler((void **)&ZEND_OBSERVER_DATA(function), begin);
}

// ing3，添加结束函数
ZEND_API void zend_observer_add_end_handler(zend_function *function, zend_observer_fcall_end_handler end) {
	// 观察者数量
	size_t registered_observers = zend_observers_fcall_list.count;
	// 结束函数列表的开头
	zend_observer_fcall_end_handler *end_handler = (zend_observer_fcall_end_handler *)&ZEND_OBSERVER_DATA(function) + registered_observers;
	// 
	// to allow to preserve the invariant that end handlers are in reverse order of begin handlers, push the new end handler in front
	// 如果不是未使用的观察者，说明第一个已经被用过了
	if (*end_handler != ZEND_OBSERVER_NOT_OBSERVED) {
		// 空间不够用咯，禁止调用此函数
		// there's no space for new handlers, then it's forbidden to call this function
		// 最后一个必须是空，这里不做检验，也不管分配空间 
		ZEND_ASSERT(end_handler[registered_observers - 1] == NULL);
		// 结束函数列表整个向后移一个位置
		memmove(end_handler + 1, end_handler, registered_observers - 1);
	}
	// 把新函数插在开头位置
	*end_handler = end;
}

// ing3, 删除结尾处理器
ZEND_API bool zend_observer_remove_end_handler(zend_function *function, zend_observer_fcall_end_handler end) {
	// 观察者数量
	size_t registered_observers = zend_observers_fcall_list.count;
	// 前面n个是begin函数 ，后面n个是end函数。所以要跳到n个后开始删。
	return zend_observer_remove_handler((void **)&ZEND_OBSERVER_DATA(function) + registered_observers, end);
}

// ing2, 前一个观察框架
static inline zend_execute_data **prev_observed_frame(zend_execute_data *execute_data) {
	// #define EX(element) 			((execute_data)->element)
	// 取得执行数据 中的函数指针
	zend_function *func = EX(func);
	// 函数必须有效
	ZEND_ASSERT(func);
	// 这其实是先把 execute_data转成(zval*)，然后取得第 5+n个zval
	// 
	return (zend_execute_data **)&Z_PTR_P(EX_VAR_NUM((ZEND_USER_CODE(func->type) ? func->op_array.last_var : ZEND_CALL_NUM_ARGS(execute_data)) + func->common.T - 1));

/*	
	(zend_execute_data **) &Z_PTR_P(
		// #define EX_VAR_NUM(n) ZEND_CALL_VAR_NUM(execute_data, n)
		// (((zval*)(call)) + (ZEND_CALL_FRAME_SLOT + ((int)(n))))
		// ((zval*)(call)) + (5 + ((int)(n)))
		EX_VAR_NUM(
			(
				// 如果是用户定义类型
				ZEND_USER_CODE(func->type) ? 
				// 编译变量的数量
				func->op_array.last_var : 
				// 上下文中变量数量
				ZEND_CALL_NUM_ARGS(execute_data)
			)
			// 函数中临时变量数 -1
			+ func->common.T - 1
		)
	)
*/
}

// ing3, 调用开始函数列表中的函数
static void ZEND_FASTCALL _zend_observe_fcall_begin(zend_execute_data *execute_data)
{
	// 步骤1， 先进行一些基本验证
	// 如果没启用观察者，直接返回
	if (!ZEND_OBSERVER_ENABLED) {
		return;
	}
	// 取得执行上下文中的方法
	zend_function *function = execute_data->func;

	// 如果不是可观察的函数 ，返回
	if (!ZEND_OBSERVABLE_FN(function)) {
		return;
	}
	
	// 步骤2，检验函数列表
	// 开始函数指针列表
	zend_observer_fcall_begin_handler *handler = (zend_observer_fcall_begin_handler *)&ZEND_OBSERVER_DATA(function);
	// 如果没有
	if (!*handler) {
		// 先添加观察者列表
		zend_observer_fcall_install(execute_data);
	}
	// begin函数列表的结尾
	zend_observer_fcall_begin_handler *possible_handlers_end = handler + zend_observers_fcall_list.count;
	// 它也是end函数列表的开头
	zend_observer_fcall_end_handler *end_handler = (zend_observer_fcall_end_handler *)possible_handlers_end;
	
	// 如果不是未使用的观察者
	if (*end_handler != ZEND_OBSERVER_NOT_OBSERVED) {
		// 当前观察框架转成前一个
		*prev_observed_frame(execute_data) = current_observed_frame;
		// 执行上下文作为当前观察框架
		current_observed_frame = execute_data;
	}

	// 如果 第一个开始函数是未启用观察者
	if (*handler == ZEND_OBSERVER_NOT_OBSERVED) {
		// 返回
		return;
	}
	// 步骤3， 遍历所有 handler
	do {
		// 依次调用每个 handler
		(*handler)(execute_data);
	// 直到 碰到 possible_handlers_end（列表结束） 或 handler 无效
	} while (++handler != possible_handlers_end && *handler != NULL);
}

// ing4, 调用开始函数列表中的函数， zend_generators.c 用到
ZEND_API void ZEND_FASTCALL zend_observer_generator_resume(zend_execute_data *execute_data)
{
	_zend_observe_fcall_begin(execute_data);
}

// ing3, 开始观察者调用
ZEND_API void ZEND_FASTCALL zend_observer_fcall_begin(zend_execute_data *execute_data)
{
	// 执行上下文中要有函数名
	ZEND_ASSUME(execute_data->func);
	// 如果函数不是 生成器
	if (!(execute_data->func->common.fn_flags & ZEND_ACC_GENERATOR)) {
		// 开始观察者调用
		_zend_observe_fcall_begin(execute_data);
	}
	// 生成器什么也不做
}

// ing3, 调用观察者结束函数列表
static inline void call_end_observers(zend_execute_data *execute_data, zval *return_value) {
	// 取得操作码中的函数指针
	zend_function *func = execute_data->func;
	// 函数必须存在
	ZEND_ASSERT(func);

	// 结束函数列表的开头
	zend_observer_fcall_end_handler *handler = (zend_observer_fcall_end_handler *)&ZEND_OBSERVER_DATA(func) + zend_observers_fcall_list.count;
	// TODO: Fix exceptions from generators
	// ZEND_ASSERT(fcall_data);
	// 如果函数无效 或 未启用，返回
	if (!*handler || *handler == ZEND_OBSERVER_NOT_OBSERVED) {
		return;
	}
	// 结束函数列表的结尾
	zend_observer_fcall_end_handler *possible_handlers_end = handler + zend_observers_fcall_list.count;
	// 遍历有效段
	do {
		// 依次调用每个结束函数（它们是倒序排列的）。比开始初始多一个参数，返回值。
		(*handler)(execute_data, return_value);
	// 没到结尾也没遇到无效函数，继续。
	} while (++handler != possible_handlers_end && *handler != NULL);
}

// ing3, 结束观察者调用
ZEND_API void ZEND_FASTCALL zend_observer_fcall_end(zend_execute_data *execute_data, zval *return_value)
{
	// 如果上下文不 是 当前观者框架
	if (execute_data != current_observed_frame) {
		// 返回
		return;
	}
	// 结束观察者调用
	call_end_observers(execute_data, return_value);
	// 当前观者框架切换到前一个框架
	current_observed_frame = *prev_observed_frame(execute_data);
}

// ing3, 结束全部观察者回调
ZEND_API void zend_observer_fcall_end_all(void)
{
	// 取得正在执行上下文
	zend_execute_data *execute_data = current_observed_frame, *original_execute_data = EG(current_execute_data);
	// 当前观察者框架置空
	current_observed_frame = NULL;
	// 如果上下文有效
	while (execute_data) {
		// 执行上下文切换到当前
		EG(current_execute_data) = execute_data;
		// 结束观察者调用
		call_end_observers(execute_data, NULL);
		// 前一个调用上下文
		execute_data = *prev_observed_frame(execute_data);
	}
	// 恢复当前执行上下文到初始状态
	EG(current_execute_data) = original_execute_data;
}

// ing3, 注册新的观察者 声名回调 函数
ZEND_API void zend_observer_function_declared_register(zend_observer_function_declared_cb cb)
{
	zend_observer_function_declared_observed = true;
	zend_llist_add_element(&zend_observer_function_declared_callbacks, &cb);
}

// ing4, 依次调用每个 声名回调 函数
ZEND_API void ZEND_FASTCALL _zend_observer_function_declared_notify(zend_op_array *op_array, zend_string *name)
{
	// 如果编译配置中 要求 忽略观察者
	if (CG(compiler_options) & ZEND_COMPILE_IGNORE_OBSERVER) {
		// 直接返回
		return;
	}
	// 遍历每个 元素
	for (zend_llist_element *element = zend_observer_function_declared_callbacks.head; element; element = element->next) {
		// 取出 声名回调 函数
		zend_observer_function_declared_cb callback = *(zend_observer_function_declared_cb *) (element->data);
		// 调用回调函数 
		callback(op_array, name);
	}
}

// ing3, 注册新的观察者 链接回调 函数
ZEND_API void zend_observer_class_linked_register(zend_observer_class_linked_cb cb)
{
	zend_observer_class_linked_observed = true;
	zend_llist_add_element(&zend_observer_class_linked_callbacks, &cb);
}

// ing4, 依次调用每个 链接回调 函数
ZEND_API void ZEND_FASTCALL _zend_observer_class_linked_notify(zend_class_entry *ce, zend_string *name)
{
	// 如果编译配置中 要求 忽略观察者
	if (CG(compiler_options) & ZEND_COMPILE_IGNORE_OBSERVER) {
		// 直接返回
		return;
	}
	// 遍历每个 元素
	for (zend_llist_element *element = zend_observer_class_linked_callbacks.head; element; element = element->next) {
		// 取出 链接回调 函数
		zend_observer_class_linked_cb callback = *(zend_observer_class_linked_cb *) (element->data);
		// 调用回调函数 
		callback(ce, name);
	}
}

// ing3, 注册新的观察者 出错回调 函数
ZEND_API void zend_observer_error_register(zend_observer_error_cb cb)
{
	zend_observer_errors_observed = true;
	zend_llist_add_element(&zend_observer_error_callbacks, &cb);
}

// ing4, 依次调用每个 出错回调 函数
ZEND_API void _zend_observer_error_notify(int type, zend_string *error_filename, uint32_t error_lineno, zend_string *message)
{
	// 遍历每个 元素
	for (zend_llist_element *element = zend_observer_error_callbacks.head; element; element = element->next) {
		// 取出 出错回调 函数
		zend_observer_error_cb callback = *(zend_observer_error_cb *) (element->data);
		// 调用回调函数 
		callback(type, error_filename, error_lineno, message);
	}
}

// ing4, 注册新的观察者 fiber初始化 函数
ZEND_API void zend_observer_fiber_init_register(zend_observer_fiber_init_handler handler)
{
	zend_llist_add_element(&zend_observer_fiber_init, &handler);
}

// ing4, 注册新的观察者 fiber切换 函数
ZEND_API void zend_observer_fiber_switch_register(zend_observer_fiber_switch_handler handler)
{
	zend_llist_add_element(&zend_observer_fiber_switch, &handler);
}

// ing4, 注册新的观察者 fiber销毁 函数
ZEND_API void zend_observer_fiber_destroy_register(zend_observer_fiber_destroy_handler handler)
{
	zend_llist_add_element(&zend_observer_fiber_destroy, &handler);
}

// ing3, 依次回调每个 fiber初始化 函数
ZEND_API void ZEND_FASTCALL zend_observer_fiber_init_notify(zend_fiber_context *initializing)
{
	zend_llist_element *element;
	zend_observer_fiber_init_handler callback;
	// 初始化上下文的 观察框架 置空
	initializing->top_observed_frame = NULL;
	// 遍历每个 观察者 fiber初始化 函数
	for (element = zend_observer_fiber_init.head; element; element = element->next) {
		// 取出回调函数
		callback = *(zend_observer_fiber_init_handler *) element->data;
		// 调用回调函数
		callback(initializing);
	}
}

// ing3, 依次回调每个 fiber切换fiber上下文 函数 
ZEND_API void ZEND_FASTCALL zend_observer_fiber_switch_notify(zend_fiber_context *from, zend_fiber_context *to)
{
	zend_llist_element *element;
	zend_observer_fiber_switch_handler callback;
	// 如果原 fiber 上下文状态是 dead
	if (from->status == ZEND_FIBER_STATUS_DEAD) {
		// 结束观察者调用 。fiber 可能是 完成 或 跳伞了
		zend_observer_fcall_end_all(); // fiber is either finished (call will do nothing) or has bailed out
	}

	// 遍历 zend_observer_fiber_switch 列表
	for (element = zend_observer_fiber_switch.head; element; element = element->next) {
		// 依次取得每个元素的回调方法
		callback = *(zend_observer_fiber_switch_handler *) element->data;
		// 回调每个元素，传入 旧、新 两个上下文
		callback(from, to);
	}
	// 旧上下文的 最上一个观察者框架 切换到 当前观察者框架
	from->top_observed_frame = current_observed_frame;
	// 当前观察者框架 切换到 新上下文的 最上一个观察者框架
	current_observed_frame = to->top_observed_frame;
}

// ing3, 依次回调每个 fiber销毁 函数
ZEND_API void ZEND_FASTCALL zend_observer_fiber_destroy_notify(zend_fiber_context *destroying)
{
	zend_llist_element *element;
	zend_observer_fiber_destroy_handler callback;
	// 从头开始，遍历 fiber销毁 函数列表
	for (element = zend_observer_fiber_destroy.head; element; element = element->next) {
		// 找到 每个回调函数
		callback = *(zend_observer_fiber_destroy_handler *) element->data;
		// 调用回调函数 
		callback(destroying);
	}
}
