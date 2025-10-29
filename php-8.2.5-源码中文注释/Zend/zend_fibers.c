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

#include "zend.h"
#include "zend_API.h"
#include "zend_ini.h"
#include "zend_vm.h"
#include "zend_exceptions.h"
#include "zend_builtin_functions.h"
#include "zend_observer.h"
#include "zend_mmap.h"
#include "zend_compile.h"
#include "zend_closures.h"

#include "zend_fibers.h"
#include "zend_fibers_arginfo.h"

// windows 没有 HAVE_VALGRIND
#ifdef HAVE_VALGRIND
# include <valgrind/valgrind.h>
#endif

// windows 没有 __SANITIZE_ADDRESS__， 没有 ZEND_FIBER_UCONTEXT
#ifdef ZEND_FIBER_UCONTEXT
# include <ucontext.h>
#endif

#ifndef ZEND_WIN32
# include <unistd.h>
# include <sys/mman.h>
# include <limits.h>

# if !defined(MAP_ANONYMOUS) && defined(MAP_ANON)
#  define MAP_ANONYMOUS MAP_ANON
# endif

/* FreeBSD require a first (i.e. addr) argument of mmap(2) is not NULL
 * if MAP_STACK is passed.
 * http://www.FreeBSD.org/cgi/query-pr.cgi?pr=158755 */
# if !defined(MAP_STACK) || defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
#  undef MAP_STACK
#  define MAP_STACK 0
# endif

# ifndef MAP_FAILED
#  define MAP_FAILED ((void * ) -1)
# endif
#endif

// windows 没有 __SANITIZE_ADDRESS__， 没有 ZEND_FIBER_UCONTEXT
#ifdef __SANITIZE_ADDRESS__
# include <sanitizer/common_interface_defs.h>
#endif

// 压缩fiber c堆栈，用于调试工具
/* Encapsulates the fiber C stack with extension for debugging tools. */
struct _zend_fiber_stack {
	// 指向数据部分的指针
	void *pointer;
	// 堆栈大小，包含了 ZEND_FIBER_GUARD_PAGES 的总大小
	size_t size;

// windows 没有 HAVE_VALGRIND
#ifdef HAVE_VALGRIND
	unsigned int valgrind_stack_id;
#endif

// windows 没有 __SANITIZE_ADDRESS__， 没有 ZEND_FIBER_UCONTEXT
#ifdef __SANITIZE_ADDRESS__
	const void *asan_pointer;
	size_t asan_size;
#endif

// windows 没有 __SANITIZE_ADDRESS__， 没有 ZEND_FIBER_UCONTEXT
#ifdef ZEND_FIBER_UCONTEXT
	// 嵌入 ucontext 来避免非必要的内存分配
	/* Embedded ucontext to avoid unnecessary memory allocations. */
	ucontext_t ucontext;
#endif
};

// 在fiber上下文切换过程中， zend vm 状态需要被 捕获或 保存
/* Zend VM state that needs to be captured / restored during fiber context switch. */
// vm状态，通过 zend_fiber_capture_vm_state 从 execute global 里获取的状态。每一个都是eg里的值。
typedef struct _zend_fiber_vm_state {
	// 
	zend_vm_stack vm_stack;
	// 堆栈顶
	zval *vm_stack_top;
	// 堆栈底
	zval *vm_stack_end;
	// 
	size_t vm_stack_page_size;
	// 当前执行上下文
	zend_execute_data *current_execute_data;
	// 错误提示设置
	int error_reporting;
	// opcache里用到的追踪编号
	uint32_t jit_trace_num;
	// 跳伞
	JMP_BUF *bailout;
	// 活跃 fiber
	zend_fiber *active_fiber;
} zend_fiber_vm_state;


// ing3, fiber捕获vm状态，存放在一个对象里
static zend_always_inline void zend_fiber_capture_vm_state(zend_fiber_vm_state *state)
{
	// vm堆栈
	state->vm_stack = EG(vm_stack);
	// 堆栈开头
	state->vm_stack_top = EG(vm_stack_top);
	// 堆栈结尾
	state->vm_stack_end = EG(vm_stack_end);
	// 堆栈页大小
	state->vm_stack_page_size = EG(vm_stack_page_size);
	// 当前执行上下文
	state->current_execute_data = EG(current_execute_data);
	// 错误级别
	state->error_reporting = EG(error_reporting);
	// 保存运行时 jit_trace_num
	state->jit_trace_num = EG(jit_trace_num);
	// 跳伞
	state->bailout = EG(bailout);
	// 当前活跃的fiber
	state->active_fiber = EG(active_fiber);
}


// ing3, 还原vm状态
static zend_always_inline void zend_fiber_restore_vm_state(zend_fiber_vm_state *state)
{
	// 把传入的fiberv vm状态，更新到 execute global里
	EG(vm_stack) = state->vm_stack;
	EG(vm_stack_top) = state->vm_stack_top;
	EG(vm_stack_end) = state->vm_stack_end;
	EG(vm_stack_page_size) = state->vm_stack_page_size;
	EG(current_execute_data) = state->current_execute_data;
	EG(error_reporting) = state->error_reporting;
	// 更新运行时 jit_trace_num
	EG(jit_trace_num) = state->jit_trace_num;
	EG(bailout) = state->bailout;
	EG(active_fiber) = state->active_fiber;
}

// windows 没有 __SANITIZE_ADDRESS__， 没有 ZEND_FIBER_UCONTEXT
#ifdef ZEND_FIBER_UCONTEXT
ZEND_TLS zend_fiber_transfer *transfer_data;
#else
/* boost_context_data is our customized definition of struct transfer_t as
 * provided by boost.context in fcontext.hpp:
 *
 * typedef void* fcontext_t;
 *
 * struct transfer_t {
 *     fcontext_t fctx;
 *     void *data;
 * }; */

// 上下文数据
typedef struct {
	void *handle;
	zend_fiber_transfer *transfer;
} boost_context_data;

/* These functions are defined in assembler files provided by boost.context (located in "Zend/asm"). */
extern void *make_fcontext(void *sp, size_t size, void (*fn)(boost_context_data));
extern ZEND_INDIRECT_RETURN boost_context_data jump_fcontext(void *to, zend_fiber_transfer *transfer);
#endif

ZEND_API zend_class_entry *zend_ce_fiber;
static zend_class_entry *zend_ce_fiber_error;

static zend_object_handlers zend_fiber_handlers;

static zend_function zend_fiber_function = { ZEND_INTERNAL_FUNCTION };

ZEND_TLS uint32_t zend_fiber_switch_blocking = 0;

#define ZEND_FIBER_DEFAULT_PAGE_SIZE 4096

// ing4, 获取堆栈页大小（window,4096）
static size_t zend_fiber_get_page_size(void)
{
	static size_t page_size = 0;
	// 如果静态变量值为 0
	if (!page_size) {
		// window 系统返回40969（测试过）
		page_size = zend_get_page_size();
		// 获取size失败 或 pagesize不是2的幂（要求-1以后每个位都变，只能是2的幂）
		if (!page_size || (page_size & (page_size - 1))) {
			// 无论如何都要返回一个有效的值
			/* anyway, we have to return a valid result */
			// 4096
			page_size = ZEND_FIBER_DEFAULT_PAGE_SIZE;
		}
	}

	return page_size;
}

// ing2, 分配内存创建堆栈 zend_fiber_stack。p1:内存尺寸
static zend_fiber_stack *zend_fiber_stack_allocate(size_t size)
{
	void *pointer;
	// 获取堆栈页大小（window,4096）
	const size_t page_size = zend_fiber_get_page_size();
	// ZEND_FIBER_GUARD_PAGES = 1
	// 这里是 page_size *2 = 8192
	const size_t minimum_stack_size = page_size + ZEND_FIBER_GUARD_PAGES * page_size;
	// 如果分配内存过小
	if (size < minimum_stack_size) {
		// 抛错：分配内存过小
		zend_throw_exception_ex(NULL, 0, "Fiber stack size is too small, it needs to be at least %zu bytes", minimum_stack_size);
		return NULL;
	}
	// 对齐到page_size（先加一个page_zize,再对齐后减掉多余的部分）
	const size_t stack_size = (size + page_size - 1) / page_size * page_size;
	// stack_size + page_size
	const size_t alloc_size = stack_size + ZEND_FIBER_GUARD_PAGES * page_size;

// windows
#ifdef ZEND_WIN32
	// 分配内存
	pointer = VirtualAlloc(0, alloc_size, MEM_COMMIT, PAGE_READWRITE);
	// 如果分配失败
	if (!pointer) {
		// 取得错误码
		DWORD err = GetLastError();
		// 取得错误信息
		char *errmsg = php_win32_error_to_msg(err);
		// 报错：分配fiber堆栈出错 
		zend_throw_exception_ex(NULL, 0, "Fiber stack allocate failed: VirtualAlloc failed: [0x%08lx] %s", err, errmsg[0] ? errmsg : "Unknown");
		// 释放错误信息
		php_win32_error_msg_free(errmsg);
		// 返回null
		return NULL;
	}

// ZEND_FIBER_GUARD_PAGES = 1，需要内存保护
# if ZEND_FIBER_GUARD_PAGES
	DWORD protect;

	// 如果内存保护失败
	if (!VirtualProtect(pointer, ZEND_FIBER_GUARD_PAGES * page_size, PAGE_READWRITE | PAGE_GUARD, &protect)) {
		// 取得错误码
		DWORD err = GetLastError();
		// 取得错误信息
		char *errmsg = php_win32_error_to_msg(err);
		// 报错：保护fiber堆栈出错 
		zend_throw_exception_ex(NULL, 0, "Fiber stack protect failed: VirtualProtect failed: [0x%08lx] %s", err, errmsg[0] ? errmsg : "Unknown");
		// 释放错误信息
		php_win32_error_msg_free(errmsg);
		// 释放内存
		VirtualFree(pointer, 0, MEM_RELEASE);
		// 返回null
		return NULL;
	}
# endif
// 不是windows，走这里
#else
	pointer = mmap(NULL, alloc_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);

	if (pointer == MAP_FAILED) {
		zend_throw_exception_ex(NULL, 0, "Fiber stack allocate failed: mmap failed: %s (%d)", strerror(errno), errno);
		return NULL;
	}

	zend_mmap_set_name(pointer, alloc_size, "zend_fiber_stack");

# if ZEND_FIBER_GUARD_PAGES
	// 
	if (mprotect(pointer, ZEND_FIBER_GUARD_PAGES * page_size, PROT_NONE) < 0) {
		zend_throw_exception_ex(NULL, 0, "Fiber stack protect failed: mprotect failed: %s (%d)", strerror(errno), errno);
		munmap(pointer, alloc_size);
		return NULL;
	}
# endif
#endif

	// 分配堆栈
	zend_fiber_stack *stack = emalloc(sizeof(zend_fiber_stack));
	
	// 堆栈指针，跳过 ZEND_FIBER_GUARD_PAGES，指向数据部分
	stack->pointer = (void *) ((uintptr_t) pointer + ZEND_FIBER_GUARD_PAGES * page_size);
	// 更新堆栈尺寸
	stack->size = stack_size;

// windows 没有 VALGRIND_STACK_REGISTER
#ifdef VALGRIND_STACK_REGISTER
	uintptr_t base = (uintptr_t) stack->pointer;
	stack->valgrind_stack_id = VALGRIND_STACK_REGISTER(base, base + stack->size);
#endif

// windows 没有 __SANITIZE_ADDRESS__， 没有 ZEND_FIBER_UCONTEXT
#ifdef __SANITIZE_ADDRESS__
	stack->asan_pointer = stack->pointer;
	stack->asan_size = stack->size;
#endif

	// 返回新创建的堆栈
	return stack;
}

// ing3, 释放堆栈
static void zend_fiber_stack_free(zend_fiber_stack *stack)
{
// windows 没有 VALGRIND_STACK_DEREGISTER
#ifdef VALGRIND_STACK_DEREGISTER
	VALGRIND_STACK_DEREGISTER(stack->valgrind_stack_id);
#endif
	// 获取堆栈页大小（window,4096）
	const size_t page_size = zend_fiber_get_page_size();
	// 从数据部指针跳过  ZEND_FIBER_GUARD_PAGES 找到 stack开头
	void *pointer = (void *) ((uintptr_t) stack->pointer - ZEND_FIBER_GUARD_PAGES * page_size);

// 释放整块内存
#ifdef ZEND_WIN32
	VirtualFree(pointer, 0, MEM_RELEASE);
#else
	munmap(pointer, stack->size + ZEND_FIBER_GUARD_PAGES * page_size);
#endif
	// 释放堆栈
	efree(stack);
}



// windows 没有 __SANITIZE_ADDRESS__， 没有 ZEND_FIBER_UCONTEXT
#ifdef ZEND_FIBER_UCONTEXT
static ZEND_NORETURN void zend_fiber_trampoline(void)
#else
// windows走这里
// ing2, 弹跳：调用当前fiber的协同函数，并切换上下文到 p1.transfer
static ZEND_NORETURN void zend_fiber_trampoline(boost_context_data data)
#endif
{
	// 使用传入的数据来初始化 transfer
	/* Initialize transfer struct with a copy of passed data. */
// windows 没有 __SANITIZE_ADDRESS__， 没有 ZEND_FIBER_UCONTEXT
#ifdef ZEND_FIBER_UCONTEXT
	zend_fiber_transfer transfer = *transfer_data;
// windows 走这里
#else
	zend_fiber_transfer transfer = *data.transfer;
#endif

	// transfer 中的上下文
	zend_fiber_context *from = transfer.context;

// windows 没有 __SANITIZE_ADDRESS__， 没有 ZEND_FIBER_UCONTEXT
#ifdef __SANITIZE_ADDRESS__
	__sanitizer_finish_switch_fiber(NULL, &from->stack->asan_pointer, &from->stack->asan_size);
#endif

// windows 没有 __SANITIZE_ADDRESS__， 没有 ZEND_FIBER_UCONTEXT
#ifndef ZEND_FIBER_UCONTEXT
	/* Get the context that resumed us and update its handle to allow for symmetric coroutines. */
	from->handle = data.handle;
#endif

	// 保证 前一个fiber会被清理（对应的协同程序需要它）
	/* Ensure that previous fiber will be cleaned up (needed by symmetric coroutines). */
	// 如果fiber已经销毁
	if (from->status == ZEND_FIBER_STATUS_DEAD) {
		// 销毁
		zend_fiber_destroy_context(from);
	}

	// 当前fiber上下文
	zend_fiber_context *context = EG(current_fiber_context);

	// 调用fiber协同函数
	context->function(&transfer);
	// 进行为fiber已结束
	context->status = ZEND_FIBER_STATUS_DEAD;

	// 最后的上下文切换，fiber不可以再恢复
	/* Final context switch, the fiber must not be resumed afterwards! */
	zend_fiber_switch_context(&transfer);

	// 中断，因为当前在一个不一致的 程序状态
	/* Abort here because we are in an inconsistent program state. */
	abort();
}

// ing4, 锁 +1
ZEND_API void zend_fiber_switch_block(void)
{
	++zend_fiber_switch_blocking;
}

// ing4, Fiber 切换到 未锁定模式
ZEND_API void zend_fiber_switch_unblock(void)
{
	// zend_fiber_switch_blocking 必须是锁定状态。这样fiber 转换不会被阻止
	ZEND_ASSERT(zend_fiber_switch_blocking && "Fiber switching was not blocked");
	// -1
	--zend_fiber_switch_blocking;
}

// ing4, 返回 Fiber 锁定状态
ZEND_API bool zend_fiber_switch_blocked(void)
{
	return zend_fiber_switch_blocking;
}

// 初始化fiber上下文，p1:fiber上下文，p2:类型，p3:fiber协同函数，p4:堆栈尺寸
ZEND_API bool zend_fiber_init_context(zend_fiber_context *context, void *kind, zend_fiber_coroutine coroutine, size_t stack_size)
{
	// 分配内存创建堆栈 zend_fiber_stack。p1:内存尺寸
	context->stack = zend_fiber_stack_allocate(stack_size);

	// 如果创建失败
	if (UNEXPECTED(!context->stack)) {
		// 返回失败
		return false;
	}

// windows 没有 __SANITIZE_ADDRESS__， 没有 ZEND_FIBER_UCONTEXT
#ifdef ZEND_FIBER_UCONTEXT
	ucontext_t *handle = &context->stack->ucontext;

	getcontext(handle);

	handle->uc_stack.ss_size = context->stack->size;
	handle->uc_stack.ss_sp = context->stack->pointer;
	handle->uc_stack.ss_flags = 0;
	handle->uc_link = NULL;

	makecontext(handle, (void (*)(void)) zend_fiber_trampoline, 0);

	context->handle = handle;
// windows走这里
#else
	// 缩减堆栈，计算堆栈的顶端。make_fcontext 把指针转换成低16位边界？
	// Stack grows down, calculate the top of the stack. make_fcontext then shifts pointer to lower 16-byte boundary.
	// 指针右移 context->stack->size 个整数位
	void *stack = (void *) ((uintptr_t) context->stack->pointer + context->stack->size);

	// ？？？
	context->handle = make_fcontext(stack, context->stack->size, zend_fiber_trampoline);
	ZEND_ASSERT(context->handle != NULL && "make_fcontext() never returns NULL");
#endif

	// 写入类型
	context->kind = kind;
	// 写入协同程序
	context->function = coroutine;

	// Set status in case memory has not been zeroed.
	context->status = ZEND_FIBER_STATUS_INIT;

	// 依次回调每个 fiber初始化 函数
	zend_observer_fiber_init_notify(context);

	// 返回成功
	return true;
}

// ing4, 销毁 fiber 上下文
ZEND_API void zend_fiber_destroy_context(zend_fiber_context *context)
{
	// 依次回调每个 fiber销毁 函数
	zend_observer_fiber_destroy_notify(context);

	// 如果有清理方法
	if (context->cleanup) {
		// 调用方法清理上下文
		context->cleanup(context);
	}

	// 释放堆栈
	zend_fiber_stack_free(context->stack);
}

// ing2, 切换fiber ，看着代码多但业务并不复杂
ZEND_API void zend_fiber_switch_context(zend_fiber_transfer *transfer)
{
	// 当前fiber上下文
	zend_fiber_context *from = EG(current_fiber_context);
	// 新的上下文
	zend_fiber_context *to = transfer->context;
	// 状态
	zend_fiber_vm_state state;
	// 新上下文必须有效，有id，并且status 不是 ZEND_FIBER_STATUS_DEAD
	ZEND_ASSERT(to && to->handle && to->status != ZEND_FIBER_STATUS_DEAD && "Invalid fiber context");
	// from 必须有效
	ZEND_ASSERT(from && "From fiber context must be present");
	// from 和 to 不能是同一个对象
	ZEND_ASSERT(to != from && "Cannot switch into the running fiber context");
	// 所有出错的transfers必须要有一个 Trowable的值
	/* Assert that all error transfers hold a Throwable value. */
	ZEND_ASSERT((
		// transfer 没有 error
		!(transfer->flags & ZEND_FIBER_TRANSFER_FLAG_ERROR) ||
		// 或 值为对象 并且 （类型是 unwind异常 或 graceful异常 或 throwable的子类 ）
		(Z_TYPE(transfer->value) == IS_OBJECT && (
			zend_is_unwind_exit(Z_OBJ(transfer->value)) ||
			zend_is_graceful_exit(Z_OBJ(transfer->value)) ||
			instanceof_function(Z_OBJCE(transfer->value), zend_ce_throwable)
		))
	) && "Error transfer requires a throwable value");

	// 调用此事件的每个观察者函数
	zend_observer_fiber_switch_notify(from, to);
	
	// 缓存 EG 状态
	zend_fiber_capture_vm_state(&state);
	// 新上下文的状态为 运行中
	to->status = ZEND_FIBER_STATUS_RUNNING;
	// 如果旧上下文也在运行中
	if (EXPECTED(from->status == ZEND_FIBER_STATUS_RUNNING)) {
		// 旧上下文标记成 暂停
		from->status = ZEND_FIBER_STATUS_SUSPENDED;
	}
	// 切换前，把原上下文更新到 transfer 中
	/* Update transfer context with the current fiber before switching. */
	transfer->context = from;
	// 当前fiber上下文切换到新上下文
	EG(current_fiber_context) = to;

// windows 没有 __SANITIZE_ADDRESS__， 没有 ZEND_FIBER_UCONTEXT
#ifdef __SANITIZE_ADDRESS__
	void *fake_stack = NULL;
	__sanitizer_start_switch_fiber(
		from->status != ZEND_FIBER_STATUS_DEAD ? &fake_stack : NULL,
		to->stack->asan_pointer,
		to->stack->asan_size);
#endif

// windows 没有 __SANITIZE_ADDRESS__， 没有 ZEND_FIBER_UCONTEXT
#ifdef ZEND_FIBER_UCONTEXT
	transfer_data = transfer;

	swapcontext(from->handle, to->handle);

	/* Copy transfer struct because it might live on the other fiber's stack that will eventually be destroyed. */
	*transfer = *transfer_data;
// windows 走这里
#else
	// 外部函数 ？？
	boost_context_data data = jump_fcontext(to->handle, transfer);

	// 复制transfer结构，因为它可能在其他fiber的堆栈中存在，最终会被销毁
	/* Copy transfer struct because it might live on the other fiber's stack that will eventually be destroyed. */
	*transfer = *data.transfer;
#endif

	// 准备转换到 transfer 关联的上下文
	to = transfer->context;

// windows 没有 __SANITIZE_ADDRESS__， 没有 ZEND_FIBER_UCONTEXT
#ifndef ZEND_FIBER_UCONTEXT
	/* Get the context that resumed us and update its handle to allow for symmetric coroutines. */
	to->handle = data.handle;
#endif

// windows 没有 __SANITIZE_ADDRESS__， 没有 ZEND_FIBER_UCONTEXT
#ifdef __SANITIZE_ADDRESS__
	__sanitizer_finish_switch_fiber(fake_stack, &to->stack->asan_pointer, &to->stack->asan_size);
#endif

	// 当前fiber上下文，恢复成 from
	EG(current_fiber_context) = from;
	// 还原vm状态
	zend_fiber_restore_vm_state(&state);

	// 如果标记成销毁 ，销毁 前面的上下文
	/* Destroy prior context if it has been marked as dead. */
	// 如果状态是 已销毁
	if (to->status == ZEND_FIBER_STATUS_DEAD) {
		// 销毁 fiber 上下文
		zend_fiber_destroy_context(to);
	}
}

// ing3, 清理fiber
static void zend_fiber_cleanup(zend_fiber_context *context)
{
	// 通过context 找到所属 zend_fiber 实例的指针
	zend_fiber *fiber = zend_fiber_from_context(context);
	// 执行时 虚拟机 堆栈
	zend_vm_stack current_stack = EG(vm_stack);
	// 执行时虚拟机堆栈更新成 此fiber指向的虚拟机堆栈
	EG(vm_stack) = fiber->vm_stack;
	// 销毁虚拟机堆栈
	zend_vm_stack_destroy();
	// 销毁虚拟机堆栈 换到原来的堆栈
	EG(vm_stack) = current_stack;
	// 清空fiber的执行数据
	fiber->execute_data = NULL;
	// 清空堆栈底
	fiber->stack_bottom = NULL;
	// 清空调用者
	fiber->caller = NULL;
}

// ing2, 执行fiber
// fiber相当于一整套执行上下文
static ZEND_STACK_ALIGNED void zend_fiber_execute(zend_fiber_transfer *transfer)
{
	// 必须没有 transfer->value
	ZEND_ASSERT(Z_TYPE(transfer->value) == IS_NULL && "Initial transfer value to fiber context must be NULL");
	// 必须没有 transfer->flags
	ZEND_ASSERT(!transfer->flags && "No flags should be set on initial transfer");

	// 当前活跃fiber
	zend_fiber *fiber = EG(active_fiber);

	// 当前报错级别
	/* Determine the current error_reporting ini setting. */
	zend_long error_reporting = INI_INT("error_reporting");
	// 如果error_reporting是0，但不是显式声名成0，INI_STR返回一个空指针
	/* If error_reporting is 0 and not explicitly set to 0, INI_STR returns a null pointer. */
	// 如果没有 error_reporting，并且 error_reporting 值为空
	if (!error_reporting && !INI_STR("error_reporting")) {
		// 错误级别为全部
		error_reporting = E_ALL;
	}

	// 清空虚拟机堆栈
	EG(vm_stack) = NULL;

	zend_first_try {
		// 新建堆栈页
		zend_vm_stack stack = zend_vm_stack_new_page(ZEND_FIBER_VM_STACK_SIZE, NULL);
		// 更新当前虚拟机堆栈
		EG(vm_stack) = stack;
		// 更新当前虚拟机堆栈 top 位置
		EG(vm_stack_top) = stack->top + ZEND_CALL_FRAME_SLOT;
		// 更新当前虚拟机堆栈 end 位置
		EG(vm_stack_end) = stack->end;
		// 更新当前虚拟机堆栈大小
		EG(vm_stack_page_size) = ZEND_FIBER_VM_STACK_SIZE;

		// fiber的执行数据
		fiber->execute_data = (zend_execute_data *) stack->top;
		// fiber执行数据放在fiber堆栈底
		fiber->stack_bottom = fiber->execute_data;

		// fiber执行数据置空
		memset(fiber->execute_data, 0, sizeof(zend_execute_data));

		// 更新fiber执行数据中的 函数指针
		fiber->execute_data->func = &zend_fiber_function;
		// fiber堆栈底的 前一个执行数据 更新成 当前执行数据
		fiber->stack_bottom->prev_execute_data = EG(current_execute_data);

		// 更新当前执行数据 为 fiber执行数据
		EG(current_execute_data) = fiber->execute_data;
		// 运行时 jit_trace_num 复位成0
		EG(jit_trace_num) = 0;
		// 更新报错级别
		EG(error_reporting) = error_reporting;

		// fiber 的结果 绑定到 调用信息
		fiber->fci.retval = &fiber->result;

		// 调用方法
		zend_call_function(&fiber->fci, &fiber->fci_cache);

		// 清理回调 并 删除元素 来防止 GC / 重复销毁 的问题 
		/* Cleanup callback and unset field to prevent GC / duplicate dtor issues. */
		// 销毁函数名
		zval_ptr_dtor(&fiber->fci.function_name);
		// 重置函数名
		ZVAL_UNDEF(&fiber->fci.function_name);

		// 如果有异常
		if (EG(exception)) {
			// 如果fiber未销毁 或 !(异常是 graceful_exit 或 unwind_exit )
			if (!(fiber->flags & ZEND_FIBER_FLAG_DESTROYED)
				|| !(zend_is_graceful_exit(EG(exception)) || zend_is_unwind_exit(EG(exception)))
			) {
				// fiber 添加 THREW（异常） 标记
				fiber->flags |= ZEND_FIBER_FLAG_THREW;
				// transfer 添加 ERROR（错误） 标记
				transfer->flags = ZEND_FIBER_TRANSFER_FLAG_ERROR;
				// 把异常放到 transfer 里
				ZVAL_OBJ_COPY(&transfer->value, EG(exception));
			}

			// 清空exception
			zend_clear_exception();
		}
	} zend_catch {
		// fiber 添加跳伞标记
		fiber->flags |= ZEND_FIBER_FLAG_BAILOUT;
		// transfer 添加跳伞标记
		transfer->flags = ZEND_FIBER_TRANSFER_FLAG_BAILOUT;
	} zend_end_try();

	// 关联cleanup方法
	fiber->context.cleanup = &zend_fiber_cleanup;
	// 关联虚拟机堆栈
	fiber->vm_stack = EG(vm_stack);
	// 调用上下文 关联给 transfer
	transfer->context = fiber->caller;
}


// 从 transfer 中取出结果或错误，放到当前运行的fiber中
/* Handles forwarding of result / error from a transfer into the running fiber. */
// ing3, 返回 p1->value 的副本
static zend_always_inline void zend_fiber_delegate_transfer_result(
	zend_fiber_transfer *transfer, INTERNAL_FUNCTION_PARAMETERS
) {
	// 如果fiber出错
	if (transfer->flags & ZEND_FIBER_TRANSFER_FLAG_ERROR) {
		// 使用内置抛错来跳过 抛出检查，它可能在处理 graceful 时出错退出。
		/* Use internal throw to skip the Throwable-check that would fail for (graceful) exit. */
		// 抛出内部异常。EG里面异常相关的这几个东西比较绕
		zend_throw_exception_internal(Z_OBJ(transfer->value));
		RETURN_THROWS();
	}
	// 返回附加值副本
	RETURN_COPY_VALUE(&transfer->value);
}

// ing3, 切换到新建的 zend_fiber_transfer，并返回它。p1:fiber上下文，p2:附加值，p3:是否异常
static zend_always_inline zend_fiber_transfer zend_fiber_switch_to(
	zend_fiber_context *context, zval *value, bool exception
) {
	// 创建临时对象
	zend_fiber_transfer transfer = {
		.context = context,
		.flags = exception ? ZEND_FIBER_TRANSFER_FLAG_ERROR : 0,
	};
	// value有效
	if (value) {
		// 复制给 transfer.value
		ZVAL_COPY(&transfer.value, value);
	// 无效
	} else {
		// 值为null
		ZVAL_NULL(&transfer.value);
	}
	// 切换fiber上班文
	zend_fiber_switch_context(&transfer);

	// 前一个跳伞 进到当前 fiber里
	/* Forward bailout into current fiber. */
	// 如果有 跳伞标记
	if (UNEXPECTED(transfer.flags & ZEND_FIBER_TRANSFER_FLAG_BAILOUT)) {
		// 活跃fiber为空
		EG(active_fiber) = NULL;
		// 跳伞
		zend_bailout();
	}

	// 返回临时创建的 zend_fiber_transfer
	return transfer;
}

// ing3, 恢复fiber, p1:fiber，p2:附加值，p3:是否抛异常
static zend_always_inline zend_fiber_transfer zend_fiber_resume(zend_fiber *fiber, zval *value, bool exception)
{
	// 前一个活跃的fiber
	zend_fiber *previous = EG(active_fiber);
	// 获取当前 上下文
	fiber->caller = EG(current_fiber_context);
	// 切换fiber
	EG(active_fiber) = fiber;
	// 切换到新建的 zend_fiber_transfer，并返回它。p1:fiber上下文，p2:附加值，p3:是否异常
	zend_fiber_transfer transfer = zend_fiber_switch_to(fiber->previous, value, exception);
	// 前一个
	EG(active_fiber) = previous;
	// 返回 zend_fiber_transfer
	return transfer;
}

// ing3, 切换上下文到 p1->caller 并清空此指针。p1:zend_fiber, p2:附加值
static zend_always_inline zend_fiber_transfer zend_fiber_suspend(zend_fiber *fiber, zval *value)
{
	// 必须要有caller
	ZEND_ASSERT(fiber->caller != NULL);

	// 调用上下文
	zend_fiber_context *caller = fiber->caller;
	// 当前上下文存放到前一个
	fiber->previous = EG(current_fiber_context);
	// 清空caller
	fiber->caller = NULL;

	// 切换到新建的 zend_fiber_transfer，并返回它。p1:fiber上下文，p2:附加值，p3:是否异常
	return zend_fiber_switch_to(caller, value, false);
}

// ing3, 创建fiberp实例
static zend_object *zend_fiber_object_create(zend_class_entry *ce)
{
	// 分配内存创建 fiber实例
	zend_fiber *fiber = emalloc(sizeof(zend_fiber));
	// 清空内存
	memset(fiber, 0, sizeof(zend_fiber));
	// 初始化 fiber 内置对象
	zend_object_std_init(&fiber->std, ce);
	// 执行方法全用biber执行方法
	fiber->std.handlers = &zend_fiber_handlers;
	// 返回内置对象
	return &fiber->std;
}

// ing3, 销毁 fiber 对象
static void zend_fiber_object_destroy(zend_object *object)
{
	zend_fiber *fiber = (zend_fiber *) object;

	// 如果fiber没有停止
	if (fiber->context.status != ZEND_FIBER_STATUS_SUSPENDED) {
		// 中断
		return;
	}

	// 当前异常
	zend_object *exception = EG(exception);
	// 清空当前异常
	EG(exception) = NULL;

	zval graceful_exit;
	// 创建 graceful_exit
	ZVAL_OBJ(&graceful_exit, zend_create_graceful_exit());

	// fiber添加已销毁标记
	fiber->flags |= ZEND_FIBER_FLAG_DESTROYED;

	// 恢复fiber, p1:fiber，p2:附加值，p3:是否抛异常
	zend_fiber_transfer transfer = zend_fiber_resume(fiber, &graceful_exit, true);

	// 销毁 graceful_exit
	zval_ptr_dtor(&graceful_exit);

	// 如果fiber状态是错误
	if (transfer.flags & ZEND_FIBER_TRANSFER_FLAG_ERROR) {
		// 这是前面 创建的 graceful_exit 的副本
		EG(exception) = Z_OBJ(transfer.value);

		// 如果没有异常 并且 当前在函数中 并且函数是用户定义
		if (!exception && EG(current_execute_data) && EG(current_execute_data)->func
				&& ZEND_USER_CODE(EG(current_execute_data)->func->common.type)) {
			// 重新抛异常
			zend_rethrow_exception(EG(current_execute_data));
		}

		// 如果ex和add_previous都不在对方的previous链上，把add_previous添加到exprevious链的源头上。
		zend_exception_set_previous(EG(exception), exception);

		// 如果没有执行数据
		if (!EG(current_execute_data)) {
			// 报错
			zend_exception_error(EG(exception), E_ERROR);
		}
	// fiber没有出错
	} else {
		// 销毁 前面 创建的 graceful_exit 的副本
		zval_ptr_dtor(&transfer.value);
		// 恢复 原来的异常
		EG(exception) = exception;
	}
}

// ing3, 销毁 zend_fiber 内置对象
static void zend_fiber_object_free(zend_object *object)
{
	// 对象转成fiber
	zend_fiber *fiber = (zend_fiber *) object;
	// 释放 回调函数名
	zval_ptr_dtor(&fiber->fci.function_name);
	// 释放回调结果 
	zval_ptr_dtor(&fiber->result);
	// 销毁内置对象
	zend_object_std_dtor(&fiber->std);
}

// ing3, 回收fiber中的数据 和它 关联的执行数据链中的所有符号表
static HashTable *zend_fiber_object_gc(zend_object *object, zval **table, int *num)
{
	zend_fiber *fiber = (zend_fiber *) object;
	// 返回全局的 zend_get_gc_buffer
	zend_get_gc_buffer *buf = zend_get_gc_buffer_create();

	// 给游标（p1->cur）指向的zval赋值， 完全是外部调用
	zend_get_gc_buffer_add_zval(buf, &fiber->fci.function_name);
	// 给游标（p1->cur）指向的zval赋值， 完全是外部调用
	zend_get_gc_buffer_add_zval(buf, &fiber->result);

	// 如果fiber没有停止 或 有->caller
	if (fiber->context.status != ZEND_FIBER_STATUS_SUSPENDED || fiber->caller != NULL) {
		// 完全是外部调用,返回 zend_get_gc_buffer 的开始位置和 游标偏移量
		zend_get_gc_buffer_use(buf, table, num);
		// 返回null
		return NULL;
	}

	// 最后的符号表
	HashTable *lastSymTable = NULL;
	// fiber绑定的执行数据
	zend_execute_data *ex = fiber->execute_data;
	// 遍历执行数据 链。把所有符号表中的元素添加进垃圾回收队列。
	for (; ex; ex = ex->prev_execute_data) {
		// 回收未完成的执行，并返回符号表。p1:执行数据，p2:调用执行数据，p3:回收缓冲区，p4:是否被yield暂停
		HashTable *symTable = zend_unfinished_execution_gc_ex(ex, ex->call, buf, false);
		// 如果符号表有效
		if (symTable) {
			// 如果有上一个符号表
			if (lastSymTable) {
				zval *val;
				// 遍历上一个符号表
				ZEND_HASH_FOREACH_VAL(lastSymTable, val) {
					// 如果是间接引用
					if (EXPECTED(Z_TYPE_P(val) == IS_INDIRECT)) {
						// 解引用
						val = Z_INDIRECT_P(val);
					}
					// 给游标（p1->cur）指向的zval赋值， 完全是外部调用
					zend_get_gc_buffer_add_zval(buf, val);
				} ZEND_HASH_FOREACH_END();
			}
			// 记录前一个符号表
			lastSymTable = symTable;
		}
	}
	// 完全是外部调用,p1:gc_buffer,p2,p3:返回 gc_buffer 的开始位置 和 游标偏移量
	zend_get_gc_buffer_use(buf, table, num);
	// 返回最后一个符号表
	return lastSymTable;
}

// ing3, 构造方法
ZEND_METHOD(Fiber, __construct)
{
	zend_fcall_info fci;
	zend_fcall_info_cache fcc;

	// 必须1个参数
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_FUNC(fci, fcc)
	ZEND_PARSE_PARAMETERS_END();

	//
	zend_fiber *fiber = (zend_fiber *) Z_OBJ_P(ZEND_THIS);

	// 如果状态是没有 启动 或者 或 调用信息中有函数名
	if (UNEXPECTED(fiber->context.status != ZEND_FIBER_STATUS_INIT || Z_TYPE(fiber->fci.function_name) != IS_UNDEF)) {
		// 报错：不可以调用两次构造方法
		zend_throw_error(zend_ce_fiber_error, "Cannot call constructor twice");
		RETURN_THROWS();
	}

	// 给fiber绑定 调用信息 和 调用信息缓存
	fiber->fci = fci;
	fiber->fci_cache = fcc;

	// 当fiber开始运行，给 保持一个对 闭包的引用
	// Keep a reference to closures or callable objects while the fiber is running.
	// 函数名增加引用次数
	Z_TRY_ADDREF(fiber->fci.function_name);
}

// ing3, 启动fiber
ZEND_METHOD(Fiber, start)
{
	zend_fiber *fiber = (zend_fiber *) Z_OBJ_P(ZEND_THIS);

	// 任意多个参数
	ZEND_PARSE_PARAMETERS_START(0, -1)
		// 获取参数列表，放在调用信息中
		Z_PARAM_VARIADIC_WITH_NAMED(fiber->fci.params, fiber->fci.param_count, fiber->fci.named_params);
	ZEND_PARSE_PARAMETERS_END();
	// 如果fiber已锁定
	if (UNEXPECTED(zend_fiber_switch_blocked())) {
		// 报错：在当前执行上下文中无法切换fiber
		zend_throw_error(zend_ce_fiber_error, "Cannot switch fibers in current execution context");
		RETURN_THROWS();
	}

	// 如果fiber没有开始
	if (fiber->context.status != ZEND_FIBER_STATUS_INIT) {
		// 报错：无法启动一个已经启动的fiber
		zend_throw_error(zend_ce_fiber_error, "Cannot start a fiber that has already been started");
		RETURN_THROWS();
	}

	// 初始化fiber上下文，p1:fiber上下文，p2:类型，p3:fiber协同程序，p4:堆栈尺寸
	if (!zend_fiber_init_context(&fiber->context, zend_ce_fiber, zend_fiber_execute, EG(fiber_stack_size))) {
		RETURN_THROWS();
	}

	// 当前上下文作为前一个上下文
	fiber->previous = &fiber->context;

	// 恢复fiber, p1:fiber，p2:附加值，p3:是否抛异常
	zend_fiber_transfer transfer = zend_fiber_resume(fiber, NULL, false);
	// 返回 p1->value 的副本
	zend_fiber_delegate_transfer_result(&transfer, INTERNAL_FUNCTION_PARAM_PASSTHRU);
}

// ing3, 停止fiber
ZEND_METHOD(Fiber, suspend)
{
	zval *value = NULL;

	// 最多1个参数
	ZEND_PARSE_PARAMETERS_START(0, 1)
		// 可选参数
		Z_PARAM_OPTIONAL
		// value 参数
		Z_PARAM_ZVAL(value);
	ZEND_PARSE_PARAMETERS_END();

	// 当前活跃的fiber
	zend_fiber *fiber = EG(active_fiber);

	// 如果没有fiber
	if (UNEXPECTED(!fiber)) {
		// 报错: 不可以在fiber外调用停止
		zend_throw_error(zend_ce_fiber_error, "Cannot suspend outside of a fiber");
		RETURN_THROWS();
	}

	// 如果fiber已经销毁
	if (UNEXPECTED(fiber->flags & ZEND_FIBER_FLAG_DESTROYED)) {
		// 报错: 不可以停止一个强制关闭的fiber
		zend_throw_error(zend_ce_fiber_error, "Cannot suspend in a force-closed fiber");
		RETURN_THROWS();
	}
	// 返回 Fiber 锁定状态
	if (UNEXPECTED(zend_fiber_switch_blocked())) {
		// 报错：不可以把fiber切换到当前执行上下文
		zend_throw_error(zend_ce_fiber_error, "Cannot switch fibers in current execution context");
		RETURN_THROWS();
	}

	// fiber必须在运行中 或 在停止状态
	ZEND_ASSERT(fiber->context.status == ZEND_FIBER_STATUS_RUNNING || fiber->context.status == ZEND_FIBER_STATUS_SUSPENDED);

	// 当前执行上下文关联到fiber
	fiber->execute_data = EG(current_execute_data);
	// 堆栈底的前一个执行数据 为空 ？
	fiber->stack_bottom->prev_execute_data = NULL;

	// 切换上下文到 p1->caller 并清空此指针。p1:zend_fiber, p2:附加值
	zend_fiber_transfer transfer = zend_fiber_suspend(fiber, value);
	// 返回 p1->value 的副本
	zend_fiber_delegate_transfer_result(&transfer, INTERNAL_FUNCTION_PARAM_PASSTHRU);
}

// ing3, 恢复fiber
ZEND_METHOD(Fiber, resume)
{
	zend_fiber *fiber;
	zval *value = NULL;

	// 最多1个参数
	ZEND_PARSE_PARAMETERS_START(0, 1)
		Z_PARAM_OPTIONAL
		Z_PARAM_ZVAL(value);
	ZEND_PARSE_PARAMETERS_END();
	
	// 返回 Fiber 锁定状态
	if (UNEXPECTED(zend_fiber_switch_blocked())) {
		zend_throw_error(zend_ce_fiber_error, "Cannot switch fibers in current execution context");
		RETURN_THROWS();
	}

	// fiber
	fiber = (zend_fiber *) Z_OBJ_P(ZEND_THIS);

	// 如果状态不是停止 或有 ->caller
	if (UNEXPECTED(fiber->context.status != ZEND_FIBER_STATUS_SUSPENDED || fiber->caller != NULL)) {
		// 报错：不可以恢复一个不在停止状态的fiber
		zend_throw_error(zend_ce_fiber_error, "Cannot resume a fiber that is not suspended");
		RETURN_THROWS();
	}

	// 堆栈底的前一个执行数据，切换到当前执行数据
	fiber->stack_bottom->prev_execute_data = EG(current_execute_data);

	// 恢复fiber, p1:fiber，p2:附加值，p3:是否抛异常
	zend_fiber_transfer transfer = zend_fiber_resume(fiber, value, false);
	// 返回 p1->value 的副本
	zend_fiber_delegate_transfer_result(&transfer, INTERNAL_FUNCTION_PARAM_PASSTHRU);
}

// ing3, 传递异常给 fiber 并恢复fiber
ZEND_METHOD(Fiber, throw)
{
	zend_fiber *fiber;
	zval *exception;

	// 必须一个参数
	ZEND_PARSE_PARAMETERS_START(1, 1)
		// 参数为 exception
		Z_PARAM_OBJECT_OF_CLASS(exception, zend_ce_throwable)
	ZEND_PARSE_PARAMETERS_END();

	// 返回 Fiber 锁定状态，如果锁定
	if (UNEXPECTED(zend_fiber_switch_blocked())) {
		// 报错：在当前执行上下文中，无法切换fiber
		zend_throw_error(zend_ce_fiber_error, "Cannot switch fibers in current execution context");
		RETURN_THROWS();
	}

	// 
	fiber = (zend_fiber *) Z_OBJ_P(ZEND_THIS);

	// 如果状态不是停止 或 有 ->caller
	if (UNEXPECTED(fiber->context.status != ZEND_FIBER_STATUS_SUSPENDED || fiber->caller != NULL)) {
		// 报错：无法恢复一个不在停止状态的fiber
		zend_throw_error(zend_ce_fiber_error, "Cannot resume a fiber that is not suspended");
		RETURN_THROWS();
	}

	// 当前执行数据，记录 fiber 前一个执行数据
	fiber->stack_bottom->prev_execute_data = EG(current_execute_data);

	// 恢复fiber, p1:fiber，p2:附加值，p3:是否抛异常
	zend_fiber_transfer transfer = zend_fiber_resume(fiber, exception, true);
	// 返回 p1->value 的副本
	zend_fiber_delegate_transfer_result(&transfer, INTERNAL_FUNCTION_PARAM_PASSTHRU);
}

// ing4，验证，fiber是否已经开始（状态为 ZEND_FIBER_STATUS_INIT）
ZEND_METHOD(Fiber, isStarted)
{
	zend_fiber *fiber;

	// 不可以传入参数
	ZEND_PARSE_PARAMETERS_NONE();

	fiber = (zend_fiber *) Z_OBJ_P(ZEND_THIS);

	RETURN_BOOL(fiber->context.status != ZEND_FIBER_STATUS_INIT);
}

// ing3，验证，fiber是否已经停止（状态为 ZEND_FIBER_STATUS_SUSPENDED 并且没有 ->caller）
ZEND_METHOD(Fiber, isSuspended)
{
	zend_fiber *fiber;

	// 不可以传入参数
	ZEND_PARSE_PARAMETERS_NONE();

	fiber = (zend_fiber *) Z_OBJ_P(ZEND_THIS);

	RETURN_BOOL(fiber->context.status == ZEND_FIBER_STATUS_SUSPENDED && fiber->caller == NULL);
}

// ing3，验证，fiber是否正在运行（状态为 ZEND_FIBER_STATUS_RUNNING 或有 ->caller）
ZEND_METHOD(Fiber, isRunning)
{
	zend_fiber *fiber;

	// 不可以传入参数
	ZEND_PARSE_PARAMETERS_NONE();

	fiber = (zend_fiber *) Z_OBJ_P(ZEND_THIS);

	RETURN_BOOL(fiber->context.status == ZEND_FIBER_STATUS_RUNNING || fiber->caller != NULL);
}

// ing4，验证，fiber是否已经终结（状态为 ZEND_FIBER_STATUS_DEAD）
ZEND_METHOD(Fiber, isTerminated)
{
	zend_fiber *fiber;

	// 不可以传入参数
	ZEND_PARSE_PARAMETERS_NONE();

	fiber = (zend_fiber *) Z_OBJ_P(ZEND_THIS);

	RETURN_BOOL(fiber->context.status == ZEND_FIBER_STATUS_DEAD);
}

// ing3, 获取fiber返回值，出错会获取到错误信息
ZEND_METHOD(Fiber, getReturn)
{
	zend_fiber *fiber;
	const char *message;

	// 不可以传入参数
	ZEND_PARSE_PARAMETERS_NONE();

	// 取得 $this 
	fiber = (zend_fiber *) Z_OBJ_P(ZEND_THIS);

	// 如果已经 终结
	if (fiber->context.status == ZEND_FIBER_STATUS_DEAD) {
		// 如果有 threw
		if (fiber->flags & ZEND_FIBER_FLAG_THREW) {
			// 错误信息：fiber抛出了异常
			message = "The fiber threw an exception";
		} else if (fiber->flags & ZEND_FIBER_FLAG_BAILOUT) {
			// 错误信息：fiber中有致命错误
			message = "The fiber exited with a fatal error";
		// 其他情况
		} else {
			// 复制引用目标 和 附加信息，并增加引用计数
			RETURN_COPY_DEREF(&fiber->result);
		}
	// 如果正在初始化
	} else if (fiber->context.status == ZEND_FIBER_STATUS_INIT) {
		// 错误信息：fiber没有启动
		message = "The fiber has not been started";
	// 其他情况
	} else {
		// 错误信息：fiber没有返回
		message = "The fiber has not returned";
	}

	// 报错：无法获取fiber值
	zend_throw_error(zend_ce_fiber_error, "Cannot get fiber return value: %s", message);
	RETURN_THROWS();
}

// ing3, 获取当前 fiber
ZEND_METHOD(Fiber, getCurrent)
{
	// 必须不能有参数
	ZEND_PARSE_PARAMETERS_NONE();
	// 取得当前 fiber对象
	zend_fiber *fiber = EG(active_fiber);
	// 如果没有，返回null
	if (!fiber) {
		RETURN_NULL();
	}
	// 如果有，返回它的内置对象的副本。
	RETURN_OBJ_COPY(&fiber->std);
}

// ing4, FiberError::__construct
ZEND_METHOD(FiberError, __construct)
{
	// 直接抛错：此类为了内部使用而保留，不能手动实例化
	zend_throw_error(
		NULL,
		"The \"%s\" class is reserved for internal use and cannot be manually instantiated",
		ZSTR_VAL(Z_OBJCE_P(ZEND_THIS)->name)
	);
}


// ing3, 注册fiber相关类
void zend_register_fiber_ce(void)
{
	// 注册fiber类
	zend_ce_fiber = register_class_Fiber();
	// 创建方法
	zend_ce_fiber->create_object = zend_fiber_object_create;
	// 标准对象方法
	zend_fiber_handlers = std_object_handlers;
	// 销毁方法
	zend_fiber_handlers.dtor_obj = zend_fiber_object_destroy;
	// 释放方法
	zend_fiber_handlers.free_obj = zend_fiber_object_free;
	// 获取垃圾回收
	zend_fiber_handlers.get_gc = zend_fiber_object_gc;
	// 不可以克隆
	zend_fiber_handlers.clone_obj = NULL;

	// 注册 FiberError 类
	zend_ce_fiber_error = register_class_FiberError(zend_ce_error);
	// 创建方法
	zend_ce_fiber_error->create_object = zend_ce_error->create_object;
}

// ing4, 初始化fiber，实时候还没有激活
void zend_fiber_init(void)
{
	// 分配内存，创建fiber上下文
	zend_fiber_context *context = ecalloc(1, sizeof(zend_fiber_context));

// windows 没有 __SANITIZE_ADDRESS__， 没有 ZEND_FIBER_UCONTEXT
#if defined(__SANITIZE_ADDRESS__) || defined(ZEND_FIBER_UCONTEXT)
	// 只有允许 ASan 或 ucontext 时，才需要主fiber 堆栈
	// Main fiber stack is only needed if ASan or ucontext is enabled.
	context->stack = emalloc(sizeof(zend_fiber_stack));

// windows 没有 __SANITIZE_ADDRESS__， 没有 ZEND_FIBER_UCONTEXT
#ifdef ZEND_FIBER_UCONTEXT
	context->handle = &context->stack->ucontext;
#endif
#endif

	// 更新上下文 状态为 正在运行中
	context->status = ZEND_FIBER_STATUS_RUNNING;
	// 更新运行时 主要fiber上下文
	EG(main_fiber_context) = context;
	// 更新运行时 当前fiber上下文
	EG(current_fiber_context) = context;
	// 清空活跃fiber
	EG(active_fiber) = NULL;
	// 锁定数 为0
	zend_fiber_switch_blocking = 0;
}

// ing3
void zend_fiber_shutdown(void)
{
// windows 没有 __SANITIZE_ADDRESS__， 没有 ZEND_FIBER_UCONTEXT
#if defined(__SANITIZE_ADDRESS__) || defined(ZEND_FIBER_UCONTEXT)
	efree(EG(main_fiber_context)->stack);
#endif

	// 删除主fiber上下文
	efree(EG(main_fiber_context));
	// 锁 +1
	zend_fiber_switch_block();
}
