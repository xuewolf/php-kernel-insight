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
   | Authors: Aaron Piotrowski <aaron@trowski.com>                        |
   |          Martin Schröder <m.schroeder2007@gmail.com>                 |
   +----------------------------------------------------------------------+
*/

#ifndef ZEND_FIBERS_H
#define ZEND_FIBERS_H

#include "zend_API.h"
#include "zend_types.h"

#define ZEND_FIBER_GUARD_PAGES 1

// fiber 默认堆栈大小：2097152
#define ZEND_FIBER_DEFAULT_C_STACK_SIZE (4096 * (((sizeof(void *)) < 8) ? 256 : 512))
// fiber vm 堆栈大小：16384
#define ZEND_FIBER_VM_STACK_SIZE (1024 * sizeof(zval))

BEGIN_EXTERN_C()

// fiber 状态
typedef enum {
	// 初始化
	ZEND_FIBER_STATUS_INIT,
	// 运行中
	ZEND_FIBER_STATUS_RUNNING,
	// 暂停
	ZEND_FIBER_STATUS_SUSPENDED,
	// 已完结
	ZEND_FIBER_STATUS_DEAD,
} zend_fiber_status;

// fiber 标记
typedef enum {
	// 已抛异常
	ZEND_FIBER_FLAG_THREW     = 1 << 0,
	// 已跳伞
	ZEND_FIBER_FLAG_BAILOUT   = 1 << 1,
	// 已销毁 
	ZEND_FIBER_FLAG_DESTROYED = 1 << 2,
} zend_fiber_flag;

// fiber传送标记
typedef enum {
	// fiber传送标记：出错
	ZEND_FIBER_TRANSFER_FLAG_ERROR = 1 << 0,
	// fiber传送标记：跳伞
	ZEND_FIBER_TRANSFER_FLAG_BAILOUT = 1 << 1
} zend_fiber_transfer_flag;

void zend_register_fiber_ce(void);
void zend_fiber_init(void);
void zend_fiber_shutdown(void);

extern ZEND_API zend_class_entry *zend_ce_fiber;

typedef struct _zend_fiber_stack zend_fiber_stack;

// 切换上下文里用到的数据
/* Encapsulates data needed for a context switch. */
typedef struct _zend_fiber_transfer {
	// 
	/* Fiber that will be switched to / has resumed us. */
	zend_fiber_context *context;
	// 
	/* Value to that should be send to (or was received from) a fiber. */
	zval value;
	// 
	/* Bitmask of flags defined in enum zend_fiber_transfer_flag. */
	uint8_t flags;
} zend_fiber_transfer;


// 协同函数必须 在返回前 生成 给出的传送，附带一个新上下文和（可选的）数据
/* Coroutine functions must populate the given transfer with a new context
 * and (optional) data before they return. */
typedef void (*zend_fiber_coroutine)(zend_fiber_transfer *transfer);
typedef void (*zend_fiber_clean)(zend_fiber_context *context);

struct _zend_fiber_context {
	// boost.context 或 ucontext_t 数据 的指针
	/* Pointer to boost.context or ucontext_t data. */
	void *handle;
	// 识别fiber类型的指针
	/* Pointer that identifies the fiber type. */
	void *kind;
	
	// fiber 函数进入点
	/* Entrypoint function of the fiber. */
	zend_fiber_coroutine function;

	// fiber 的清理函数
	/* Cleanup function for fiber. */
	zend_fiber_clean cleanup;

	// 分配的 c 堆栈
	/* Assigned C stack. */
	zend_fiber_stack *stack;

	// fiber 状态
	/* Fiber status. */
	zend_fiber_status status;

	// 观察状态
	/* Observer state */
	zend_execute_data *top_observed_frame;
	
	// 扩展保存的
	/* Reserved for extensions */
	void *reserved[ZEND_MAX_RESERVED_RESOURCES];
};

struct _zend_fiber {
	// php 对象处理器
	/* PHP object handle. */
	zend_object std;

	// zend_fiber_flag 中定义的标记
	/* Flags are defined in enum zend_fiber_flag. */
	uint8_t flags;

	// 原生C fiber上下文
	/* Native C fiber context. */
	zend_fiber_context context;

	// 恢复的 fiber
	/* Fiber that resumed us. */
	zend_fiber_context *caller;

	// 暂停的 fiber
	/* Fiber that suspended us. */
	zend_fiber_context *previous;

	// 当fiber启动后，使用的回调和 info/cache.
	/* Callback and info / cache to be used when fiber is started. */
	zend_fcall_info fci;
	zend_fcall_info_cache fci_cache;

	// fiber 运行的当前 vm 执行数据
	/* Current Zend VM execute data being run by the fiber. */
	zend_execute_data *execute_data;

	// fiber vm 堆栈底部框架
	/* Frame on the bottom of the fiber vm stack. */
	zend_execute_data *stack_bottom;

	// 活跃的fiber vm 堆栈
	/* Active fiber vm stack. */
	zend_vm_stack vm_stack;

	// fiber 返回值存储
	/* Storage for fiber return value. */
	zval result;
};

/* These functions may be used to create custom fiber objects using the bundled fiber switching context. */
ZEND_API bool zend_fiber_init_context(zend_fiber_context *context, void *kind, zend_fiber_coroutine coroutine, size_t stack_size);
ZEND_API void zend_fiber_destroy_context(zend_fiber_context *context);
ZEND_API void zend_fiber_switch_context(zend_fiber_transfer *transfer);

ZEND_API void zend_fiber_switch_block(void);
ZEND_API void zend_fiber_switch_unblock(void);
ZEND_API bool zend_fiber_switch_blocked(void);

END_EXTERN_C()

// ing3, 通过context 找到所属 zend_fiber 实例的指针
static zend_always_inline zend_fiber *zend_fiber_from_context(zend_fiber_context *context)
{
	// 上下文类型是 zend_class_entry 指针
	ZEND_ASSERT(context->kind == zend_ce_fiber && "Fiber context does not belong to a Zend fiber");
	// 通过context 找到所属 zend_fiber 实例的指针
	return (zend_fiber *)(((char *) context) - XtOffsetOf(zend_fiber, context));
}

// ing3, 返回fiber实例指向的上下文
static zend_always_inline zend_fiber_context *zend_fiber_get_context(zend_fiber *fiber)
{
	return &fiber->context;
}

#endif
