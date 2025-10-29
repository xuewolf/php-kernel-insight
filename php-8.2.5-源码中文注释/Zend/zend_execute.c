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
   +----------------------------------------------------------------------+
*/

#define ZEND_INTENSIVE_DEBUGGING 0

#include <stdio.h>
#include <signal.h>

#include "zend.h"
#include "zend_compile.h"
#include "zend_execute.h"
#include "zend_API.h"
#include "zend_ptr_stack.h"
#include "zend_constants.h"
#include "zend_extensions.h"
#include "zend_ini.h"
#include "zend_exceptions.h"
#include "zend_interfaces.h"
#include "zend_closures.h"
#include "zend_generators.h"
#include "zend_vm.h"
#include "zend_dtrace.h"
#include "zend_inheritance.h"
#include "zend_type_info.h"
#include "zend_smart_str.h"
#include "zend_observer.h"
#include "zend_system_id.h"
#include "Optimizer/zend_func_info.h"

// 支持虚拟当前工作目录
/* Virtual current working directory support */
#include "zend_virtual_cwd.h"

// ing1, 是否包含GCC全局 REGS，这是外部环境变量，参看：Zend.m4
// windows 里没有这些东西
#ifdef HAVE_GCC_GLOBAL_REGS
# if defined(__GNUC__) && ZEND_GCC_VERSION >= 4008 && defined(i386)
#  define ZEND_VM_FP_GLOBAL_REG "%esi"
#  define ZEND_VM_IP_GLOBAL_REG "%edi"
# elif defined(__GNUC__) && ZEND_GCC_VERSION >= 4008 && defined(__x86_64__)
#  define ZEND_VM_FP_GLOBAL_REG "%r14"
#  define ZEND_VM_IP_GLOBAL_REG "%r15"
# elif defined(__GNUC__) && ZEND_GCC_VERSION >= 4008 && defined(__powerpc64__)
#  define ZEND_VM_FP_GLOBAL_REG "r14"
#  define ZEND_VM_IP_GLOBAL_REG "r15"
# elif defined(__IBMC__) && ZEND_GCC_VERSION >= 4002 && defined(__powerpc64__)
#  define ZEND_VM_FP_GLOBAL_REG "r14"
#  define ZEND_VM_IP_GLOBAL_REG "r15"
# elif defined(__GNUC__) && ZEND_GCC_VERSION >= 4008 && defined(__aarch64__)
#  define ZEND_VM_FP_GLOBAL_REG "x27"
#  define ZEND_VM_IP_GLOBAL_REG "x28"
# endif
#endif

// windows 里没有这些东西
#if defined(ZEND_VM_FP_GLOBAL_REG) && ((ZEND_VM_KIND == ZEND_VM_KIND_CALL) || (ZEND_VM_KIND == ZEND_VM_KIND_HYBRID))
# pragma GCC diagnostic ignored "-Wvolatile-register-var"
  register zend_execute_data* volatile execute_data __asm__(ZEND_VM_FP_GLOBAL_REG);
# pragma GCC diagnostic warning "-Wvolatile-register-var"
#endif

// 这一堆东西是用来作参数的，根据运行环境自动添加参数到函数声名和调用里
// 如果有 HAVE_GCC_GLOBAL_REGS 并且 运行模式是 ZEND_VM_KIND_CALL 或 ZEND_VM_KIND_HYBRID
// windows 里没有这些东西
#if defined(ZEND_VM_FP_GLOBAL_REG) && ((ZEND_VM_KIND == ZEND_VM_KIND_CALL) || (ZEND_VM_KIND == ZEND_VM_KIND_HYBRID))
# define EXECUTE_DATA_D     void
# define EXECUTE_DATA_C
# define EXECUTE_DATA_DC
# define EXECUTE_DATA_CC
# define NO_EXECUTE_DATA_CC
// 没有 HAVE_GCC_GLOBAL_REGS 或运行模式不是 ZEND_VM_KIND_CALL 和 ZEND_VM_KIND_HYBRID
#else
// 用在函数声名里的形式参数
# define EXECUTE_DATA_D     zend_execute_data* execute_data
// 用在函数调用里的实际参数
# define EXECUTE_DATA_C     execute_data
// 不在开头的形式参数
# define EXECUTE_DATA_DC    , EXECUTE_DATA_D
// 不在开头的实际参数
# define EXECUTE_DATA_CC    , EXECUTE_DATA_C
// 实际参数 null 
# define NO_EXECUTE_DATA_CC , NULL
#endif

// 和上面一样，型参和实参，windows默认没有 ZEND_VM_FP_GLOBAL_REG
#if defined(ZEND_VM_FP_GLOBAL_REG) && ((ZEND_VM_KIND == ZEND_VM_KIND_CALL) || (ZEND_VM_KIND == ZEND_VM_KIND_HYBRID))
# define OPLINE_D           void
# define OPLINE_C
# define OPLINE_DC
# define OPLINE_CC
#else
# define OPLINE_D           const zend_op* opline
# define OPLINE_C           opline
# define OPLINE_DC          , OPLINE_D
# define OPLINE_CC          , OPLINE_C
#endif

// 这些都是 __GNUC__ 用的，windows里没有
#if defined(ZEND_VM_IP_GLOBAL_REG) && ((ZEND_VM_KIND == ZEND_VM_KIND_CALL) || (ZEND_VM_KIND == ZEND_VM_KIND_HYBRID))
# pragma GCC diagnostic ignored "-Wvolatile-register-var"
  register const zend_op* volatile opline __asm__(ZEND_VM_IP_GLOBAL_REG);
# pragma GCC diagnostic warning "-Wvolatile-register-var"
#else
#endif

#define _CONST_CODE  0
#define _TMP_CODE    1
#define _VAR_CODE    2
#define _UNUSED_CODE 3
#define _CV_CODE     4

typedef int (ZEND_FASTCALL *incdec_t)(zval *);
// 这一堆函数的简写，把和环境有关的参数做成自动。这一堆到底是干嘛的

// ing4, 在执行数据中通过 p1:运算对象类型，p3:操作类型，取回p2:运算对象 指向的zval（编译变量有附加处理）
#define get_zval_ptr(op_type, node, type) _get_zval_ptr(op_type, node, type EXECUTE_DATA_CC OPLINE_CC)
// ing4, 在执行数据中通过 p1:运算对象类型，p3:操作类型，取回p2:运算对象 指向的zval（编译变量有附加处理）（解引用）
#define get_zval_ptr_deref(op_type, node, type) _get_zval_ptr_deref(op_type, node, type EXECUTE_DATA_CC OPLINE_CC)
// ing4, 在执行数据中通过 p1:运算对象类型，p3:操作类型，取回p2:运算对象 指向的zval（编译变量无附加处理）
#define get_zval_ptr_undef(op_type, node, type) _get_zval_ptr_undef(op_type, node, type EXECUTE_DATA_CC OPLINE_CC)

// ing4, 在执行数据中通过 p1:运算对象类型，p3:操作类型，取回p2:运算对象指向的zval
//（常量取下一行操作码）（编译变量未定义报错）
#define get_op_data_zval_ptr_r(op_type, node) _get_op_data_zval_ptr_r(op_type, node EXECUTE_DATA_CC OPLINE_CC)
// ing4, 在执行数据中通过 p1:运算对象类型，p3:操作类型，取回p2:运算对象指向的zval
//（常量取下一行操作码）（编译变量未定义报错）（解引用）
#define get_op_data_zval_ptr_deref_r(op_type, node) _get_op_data_zval_ptr_deref_r(op_type, node EXECUTE_DATA_CC OPLINE_CC)

// ing4, 在执行数据中通过 p1:运算对象类型，p3:操作类型，取回p2:运算对象 指向的zval
// 编译变量带附加处理，普通变量解间接引用
#define get_zval_ptr_ptr(op_type, node, type) _get_zval_ptr_ptr(op_type, node, type EXECUTE_DATA_CC)
// 同上，没区别
#define get_zval_ptr_ptr_undef(op_type, node, type) _get_zval_ptr_ptr(op_type, node, type EXECUTE_DATA_CC)

// ing4, 在执行数据中通过 p1:运算对象类型，p2:运算对象，p3:操作类型，取回需要的zval
// 运算对象类型为 IS_UNUSED 时返回 this
#define get_obj_zval_ptr(op_type, node, type) _get_obj_zval_ptr(op_type, node, type EXECUTE_DATA_CC OPLINE_CC)
// ing4, 在执行数据中通过 p1:运算对象类型，p3:操作类型，取回p2:运算对象 指向的zval
// （编译变量无附加处理） 运算对象类型为 IS_UNUSED 时返回 this
#define get_obj_zval_ptr_undef(op_type, node, type) _get_obj_zval_ptr_undef(op_type, node, type EXECUTE_DATA_CC OPLINE_CC)
// ing4, 在执行数据中通过 p1:运算对象类型，p3:操作类型，取回p2:运算对象 指向的zval
// 编译变量带附加处理，普通变量解间接引用，运算对象类型为 IS_UNUSED 时返回 this
#define get_obj_zval_ptr_ptr(op_type, node, type) _get_obj_zval_ptr_ptr(op_type, node, type EXECUTE_DATA_CC)
// ing4, 检验：opline 返回类型不是IS_UNUSED
#define RETURN_VALUE_USED(opline) ((opline)->result_type != IS_UNUSED)

// 空函数，虚拟机会用到
static ZEND_FUNCTION(pass)
{
}

ZEND_BEGIN_ARG_INFO_EX(zend_pass_function_arg_info, 0, 0, 0)
ZEND_END_ARG_INFO()

// 
ZEND_API const zend_internal_function zend_pass_function = {
	ZEND_INTERNAL_FUNCTION, /* type              */
	{0, 0, 0},              /* arg_flags         */
	0,                      /* fn_flags          */
	NULL,                   /* name              */
	NULL,                   /* scope             */
	NULL,                   /* prototype         */
	0,                      /* num_args          */
	0,                      /* required_num_args */
	(zend_internal_arg_info *) zend_pass_function_arg_info + 1, /* arg_info */
	NULL,                   /* attributes        */
	0,                      /* T                 */
	NULL,                   /* run_time_cache    */
	ZEND_FN(pass),          /* handler           */
	NULL,                   /* module            */
	{NULL,NULL,NULL,NULL}   /* reserved          */
};

// ing3, p1中的引用实例 引用数-1（如果为0：目标关联到opline->result,解引间接引用 并 销毁 p1）
// #define EX_VAR(n) ZEND_CALL_VAR(execute_data, n)
// #define ZEND_CALL_VAR(call, n) ((zval*)(((char*)(call)) + ((int)(n))))
#define FREE_VAR_PTR_AND_EXTRACT_RESULT_IF_NECESSARY(free_var) do {			\
	/* 找到要释放的 zval */ \
	zval *__container_to_free = EX_VAR(free_var);							\
	/* 如果它是可计数的 */ \
	if (UNEXPECTED(Z_REFCOUNTED_P(__container_to_free))) {					\
		/* 取得它的计数器（也是目标变量） */ \
		zend_refcounted *__ref = Z_COUNTED_P(__container_to_free);			\
		/* 如果 引用数 -1后为0 */ \
		if (UNEXPECTED(!GC_DELREF(__ref))) {								\
			/* 取得操作码结果变量 */ \
			zval *__zv = EX_VAR(opline->result.var);						\
			/* 如果结果是间接引用类型 */ \
			if (EXPECTED(Z_TYPE_P(__zv) == IS_INDIRECT)) {					\
				/* 解引用 */ \
				ZVAL_COPY(__zv, Z_INDIRECT_P(__zv));						\
			}																\
			/* 调用每个type对应的销毁函数执行销毁 */ \
			rc_dtor_func(__ref);											\
		}																	\
	}																		\
} while (0)

// ing4, 释放（变量 和 临时变量 类型的）运算对象（zval）：p1:类型，p2:编号
#define FREE_OP(type, var) \
	/* 如果类型包含临时变量或变量 */ \
	if ((type) & (IS_TMP_VAR|IS_VAR)) { \
		/* 销毁没有引用次数的对象,并没有销毁 zval本身。nogc的意思是，不放入gc回收周期。 */ \
		zval_ptr_dtor_nogc(EX_VAR(var)); \
	}

// ing4, 获取操作码列表里的第n个编译变量的名字 , 内部引用
#define CV_DEF_OF(i) (EX(func)->op_array.vars[i])

// 16384，必须是2的幂
#define ZEND_VM_STACK_PAGE_SLOTS (16 * 1024) /* should be a power of 2 */

// ing4, 16*1024*16 = 262144
#define ZEND_VM_STACK_PAGE_SIZE  (ZEND_VM_STACK_PAGE_SLOTS * sizeof(zval))

// ing3, size 添加头数据后 对齐到 pagesize
#define ZEND_VM_STACK_PAGE_ALIGNED_SIZE(size, page_size) \
	(((size) + ZEND_VM_STACK_HEADER_SLOTS * sizeof(zval) \
	  + ((page_size) - 1)) & ~((page_size) - 1))

// ing3, 初始化vm堆栈
ZEND_API void zend_vm_stack_init(void)
{
	// 262144
	EG(vm_stack_page_size) = ZEND_VM_STACK_PAGE_SIZE;
	// 创建新的虚拟机堆栈页 zend_execute.h
	EG(vm_stack) = zend_vm_stack_new_page(ZEND_VM_STACK_PAGE_SIZE, NULL);
	// 记录堆栈的开头和结尾
	EG(vm_stack_top) = EG(vm_stack)->top;
	EG(vm_stack_end) = EG(vm_stack)->end;
}

// ing3, 初始化vm堆栈，p1:指定大小
ZEND_API void zend_vm_stack_init_ex(size_t page_size)
{
	// page_size 必须是2的幂
	/* page_size must be a power of 2 */
	ZEND_ASSERT(page_size > 0 && (page_size & (page_size - 1)) == 0);
	// 记录堆栈大小
	EG(vm_stack_page_size) = page_size;
	// 创建新的虚拟机堆栈页 zend_execute.h
	EG(vm_stack) = zend_vm_stack_new_page(page_size, NULL);
	// 记录堆栈的开头和结尾
	EG(vm_stack_top) = EG(vm_stack)->top;
	EG(vm_stack_end) = EG(vm_stack)->end;
}

// ing4, 销毁虚拟机堆栈
ZEND_API void zend_vm_stack_destroy(void)
{
	// 取得虚拟机堆栈
	zend_vm_stack stack = EG(vm_stack);

	// 逐个销毁
	while (stack != NULL) {
		zend_vm_stack p = stack->prev;
		efree(stack);
		stack = p;
	}
}

// ing3, 创建新的虚拟机堆栈页，返回最上一个元素指针，p1:大小
ZEND_API void* zend_vm_stack_extend(size_t size)
{
	zend_vm_stack stack;
	void *ptr;
	// 全局变量里的堆栈（不是指针）
	stack = EG(vm_stack);
	// 记录头原素
	stack->top = EG(vm_stack_top);
	// 创建新的虚拟机堆栈页。p1:大小，p2:前一个堆栈页
	EG(vm_stack) = stack = zend_vm_stack_new_page(
		// size < 剩余大小 ？EG(vm_stack_page_size) : size 添加头数据后 对齐到 EG(vm_stack_page_size)
		EXPECTED(size < EG(vm_stack_page_size) - (ZEND_VM_STACK_HEADER_SLOTS * sizeof(zval))) ?
			EG(vm_stack_page_size) : ZEND_VM_STACK_PAGE_ALIGNED_SIZE(size, EG(vm_stack_page_size)),
		stack);
	// 头元素
	ptr = stack->top;
	// 新的顶点位置
	EG(vm_stack_top) = (void*)(((char*)ptr) + size);
	// 尾元素
	EG(vm_stack_end) = stack->end;
	// 返回头元素指针
	return ptr;
}

// ing4, 通过偏移量获取 execute_data 中的 第n个 zval
ZEND_API zval* zend_get_compiled_variable_value(const zend_execute_data *execute_data, uint32_t var)
{
	// #define EX_VAR(n) ZEND_CALL_VAR(execute_data, n)
	// #define ZEND_CALL_VAR(call, n) ((zval*)(((char*)(call)) + ((int)(n))))
	return EX_VAR(var);
}

// suspend, windows 返回0
ZEND_API bool zend_gcc_global_regs(void)
{
  #if defined(HAVE_GCC_GLOBAL_REGS)
        return 1;
  #else
        return 0;
  #endif
}

// ing4, 通过偏移量获取 执行数据 中的 第n个 zval, 不可以是引用类型。p1:变量序号
static zend_always_inline zval *_get_zval_ptr_tmp(uint32_t var EXECUTE_DATA_DC)
{
	// #define EX_VAR(n) ZEND_CALL_VAR(execute_data, n)
	// #define ZEND_CALL_VAR(call, n) ((zval*)(((char*)(call)) + ((int)(n))))
	zval *ret = EX_VAR(var);
	// 临时变量最重要的是不可以是引用类型
	ZEND_ASSERT(Z_TYPE_P(ret) != IS_REFERENCE);

	return ret;
}

// ing4, 通过偏移量获取 执行数据 中的 第n个 zval。p1:变量序号
static zend_always_inline zval *_get_zval_ptr_var(uint32_t var EXECUTE_DATA_DC)
{
	// #define EX_VAR(n) ZEND_CALL_VAR(execute_data, n)
	// #define ZEND_CALL_VAR(call, n) ((zval*)(((char*)(call)) + ((int)(n))))
	zval *ret = EX_VAR(var);

	return ret;
}

// ing4, 通过偏移量获取 执行数据 中的 第n个 zval（解引用）
static zend_always_inline zval *_get_zval_ptr_var_deref(uint32_t var EXECUTE_DATA_DC)
{
	// 通过偏移量获取 execute_data 中的 zval
	// #define EX_VAR(n) ZEND_CALL_VAR(execute_data, n)
	// #define ZEND_CALL_VAR(call, n) ((zval*)(((char*)(call)) + ((int)(n))))
	zval *ret = EX_VAR(var);

	// 如果是引用类型，追踪到被引用的对象
	ZVAL_DEREF(ret);
	// 返回
	return ret;
}

// ing4, 报错：未定义的变量（变量名）, 并返未初始化zval, EXECUTE_DATA_DC 传入也用不到
static zend_never_inline ZEND_COLD zval* zval_undefined_cv(uint32_t var EXECUTE_DATA_DC)
{
	// 如果没有异常
	if (EXPECTED(EG(exception) == NULL)) {
		// ing4, 获取操作码列表里的第n个编译变量的名字, 内部引用
		// 把偏移量转成 zval 序号 （减掉 ZEND_CALL_FRAME_SLOT）
		zend_string *cv = CV_DEF_OF(EX_VAR_TO_NUM(var));
		// 报错未定义的变量
		zend_error(E_WARNING, "Undefined variable $%s", ZSTR_VAL(cv));
	}
	// 返回未定义变量
	return &EG(uninitialized_zval);
}

// ing4, 报错：p1的变量（变量名）未定义, 并返未初始化zval。 EXECUTE_DATA_DC 传入也用不到
static zend_never_inline ZEND_COLD zval* ZEND_FASTCALL _zval_undefined_op1(EXECUTE_DATA_D)
{
	// #define EX(element) ((execute_data)->element)
	// ing4, 报错：未定义的变量（变量名）, 并返未初始化zval, EXECUTE_DATA_DC 传入也用不到
	return zval_undefined_cv(EX(opline)->op1.var EXECUTE_DATA_CC);
}

// ing4, 报错：p2的变量（变量名）未定义, 并返未初始化zval。  EXECUTE_DATA_DC 传入也用不到
static zend_never_inline ZEND_COLD zval* ZEND_FASTCALL _zval_undefined_op2(EXECUTE_DATA_D)
{
	// ing4, 报错：未定义的变量（变量名）, 并返未初始化zval, EXECUTE_DATA_DC 传入也用不到
	return zval_undefined_cv(EX(opline)->op2.var EXECUTE_DATA_CC);
}

// ing4, 报错：p1的变量（变量名）未定义, 并返未初始化zval。 EXECUTE_DATA_DC 传入也用不到
#define ZVAL_UNDEFINED_OP1() _zval_undefined_op1(EXECUTE_DATA_C)

// ing4, 报错：p2的变量（变量名）未定义, 并返未初始化zval。  EXECUTE_DATA_DC 传入也用不到
#define ZVAL_UNDEFINED_OP2() _zval_undefined_op2(EXECUTE_DATA_C)

// ing4, 根据操作类型处理：读或删除:报错并返回未初始化变量，检查:返回未初始化变量，更新：报错，创建：返回null
static zend_never_inline ZEND_COLD zval *_get_zval_cv_lookup(zval *ptr, uint32_t var, int type EXECUTE_DATA_DC)
{
	switch (type) {
		// 读取或删除
		case BP_VAR_R:
		case BP_VAR_UNSET:
			// ing4, 报错：未定义的变量（变量名）, 并返未初始化zval, EXECUTE_DATA_DC 传入也用不到
			ptr = zval_undefined_cv(var EXECUTE_DATA_CC);
			break;
		// 检查
		case BP_VAR_IS:
			// 返回 未初始化过的zval
			ptr = &EG(uninitialized_zval);
			break;
		// 更新
		case BP_VAR_RW:
			// ing4, 报错：未定义的变量（变量名）, 并返未初始化zval, EXECUTE_DATA_DC 传入也用不到
			zval_undefined_cv(var EXECUTE_DATA_CC);
			// ((void)0)
			ZEND_FALLTHROUGH;
		// 创建
		case BP_VAR_W:
			// 值写成null
			ZVAL_NULL(ptr);
			break;
	}
	// 返回指针
	return ptr;
}

// ing4, 根据序号 和 操作类型，返回zval。变量是未定义时，附加处理：
// 读或删除:报错并返回未初始化zval，检查:返回未初始化zval，更新：报错，创建：返回null
static zend_always_inline zval *_get_zval_ptr_cv(uint32_t var, int type EXECUTE_DATA_DC)
{
	// 取得执行数据中的第n个变量
	// #define EX_VAR(n) ZEND_CALL_VAR(execute_data, n)
	// #define ZEND_CALL_VAR(call, n) ((zval*)(((char*)(call)) + ((int)(n))))
	zval *ret = EX_VAR(var);

	// 如果变量类型是未定义
	if (UNEXPECTED(Z_TYPE_P(ret) == IS_UNDEF)) {
		// 操作类型是 写入。这个操作也和else 里相同
		if (type == BP_VAR_W) {
			// 变量值赋为 null
			ZVAL_NULL(ret);
		// 操作类型是其他
		} else {
			// ing4, 根据操作类型处理：读或删除:报错并返回未初始化变量，检查:返回未初始化变量，更新：报错，创建：返回null
			return _get_zval_cv_lookup(ret, var, type EXECUTE_DATA_CC);
		}
	}
	return ret;
}

// ing4, 根据序号 和 操作类型，返回zval（解引用）,p1:变量序号
static zend_always_inline zval *_get_zval_ptr_cv_deref(uint32_t var, int type EXECUTE_DATA_DC)
{
	// 通过序号取出变量
	// #define EX_VAR(n) ZEND_CALL_VAR(execute_data, n)
	// #define ZEND_CALL_VAR(call, n) ((zval*)(((char*)(call)) + ((int)(n))))
	zval *ret = EX_VAR(var);
	// 如果变量是未定义
	if (UNEXPECTED(Z_TYPE_P(ret) == IS_UNDEF)) {
		// 如果操作是创建（和else里操作一样）
		if (type == BP_VAR_W) {
			// 值为null
			ZVAL_NULL(ret);
			return ret;
		} else {
			// ing4, 根据操作类型处理：读或删除:报错并返回未初始化变量，检查:返回未初始化变量，更新：报错，创建：返回null
			return _get_zval_cv_lookup(ret, var, type EXECUTE_DATA_CC);
		}
	}
	// 如果是引用类型，追踪到被引用的对象
	ZVAL_DEREF(ret);
	return ret;
}

// ing4, 从执行数据中，取回指定编号的变量，是未定义则报错, 并返未初始化zval
static zend_always_inline zval *_get_zval_ptr_cv_BP_VAR_R(uint32_t var EXECUTE_DATA_DC)
{
	// 通过序号取出变量
	// #define EX_VAR(n) ZEND_CALL_VAR(execute_data, n)
	// #define ZEND_CALL_VAR(call, n) ((zval*)(((char*)(call)) + ((int)(n))))
	zval *ret = EX_VAR(var);
	// 如果是未定义
	if (UNEXPECTED(Z_TYPE_P(ret) == IS_UNDEF)) {
		// ing4, 报错：未定义的变量（变量名）, 并返未初始化zval, EXECUTE_DATA_DC 传入也用不到
		return zval_undefined_cv(var EXECUTE_DATA_CC);
	}
	return ret;
}

// ing4, 从执行数据中，取回指定编号的变量（解引用），是未定义则报错
static zend_always_inline zval *_get_zval_ptr_cv_deref_BP_VAR_R(uint32_t var EXECUTE_DATA_DC)
{
	// 通过序号取出变量
	// #define EX_VAR(n) ZEND_CALL_VAR(execute_data, n)
	// #define ZEND_CALL_VAR(call, n) ((zval*)(((char*)(call)) + ((int)(n))))
	zval *ret = EX_VAR(var);

	// 如果是未定义
	if (UNEXPECTED(Z_TYPE_P(ret) == IS_UNDEF)) {
		// ing4, 报错：未定义的变量（变量名）, 并返未初始化zval, EXECUTE_DATA_DC 传入也用不到
		return zval_undefined_cv(var EXECUTE_DATA_CC);
	}
	// 如果是引用类型，追踪到被引用的对象
	ZVAL_DEREF(ret);
	return ret;
}

// ing3, 从执行数据中，取回指定编号的变量
static zend_always_inline zval *_get_zval_ptr_cv_BP_VAR_IS(uint32_t var EXECUTE_DATA_DC)
{
	// 通过序号取出变量
	// #define EX_VAR(n) ZEND_CALL_VAR(execute_data, n)
	// #define ZEND_CALL_VAR(call, n) ((zval*)(((char*)(call)) + ((int)(n))))
	zval *ret = EX_VAR(var);

	return ret;
}

// ing3, 从执行数据中，取回指定编号的变量，是未定义则报错 并赋值为 null
static zend_always_inline zval *_get_zval_ptr_cv_BP_VAR_RW(uint32_t var EXECUTE_DATA_DC)
{
	// #define EX_VAR(n) ZEND_CALL_VAR(execute_data, n)
	// #define ZEND_CALL_VAR(call, n) ((zval*)(((char*)(call)) + ((int)(n))))
	zval *ret = EX_VAR(var);

	// 如果是未定义
	if (UNEXPECTED(Z_TYPE_P(ret) == IS_UNDEF)) {
		// ing4, 报错：未定义的变量（变量名）, 并返未初始化zval, EXECUTE_DATA_DC 传入也用不到
		zval_undefined_cv(var EXECUTE_DATA_CC);
		// 值为null
		ZVAL_NULL(ret);
		// 
		return ret;
	}
	return ret;
}

// ing3, 从执行数据中，取回指定编号的变量，是未定义则 赋值为 null
static zend_always_inline zval *_get_zval_ptr_cv_BP_VAR_W(uint32_t var EXECUTE_DATA_DC)
{
	// #define EX_VAR(n) ZEND_CALL_VAR(execute_data, n)
	// #define ZEND_CALL_VAR(call, n) ((zval*)(((char*)(call)) + ((int)(n))))
	zval *ret = EX_VAR(var);
	// 如果是未定义
	if (Z_TYPE_P(ret) == IS_UNDEF) {
		// 值为null
		ZVAL_NULL(ret);
	}
	return ret;
}

// ing4, 在执行数据中通过 p1:运算对象类型，p3:操作类型，取回p2:运算对象 指向的zval（编译变量有附加处理）
static zend_always_inline zval *_get_zval_ptr(int op_type, znode_op node, int type EXECUTE_DATA_DC OPLINE_DC)
{
	// 如果类型是 变量或临时变量
	if (op_type & (IS_TMP_VAR|IS_VAR)) {
		// 非调试模式 或 变量
		if (!ZEND_DEBUG || op_type == IS_VAR) {
			// ing4, 通过偏移量获取 执行数据 中的 第n个 zval
			return _get_zval_ptr_var(node.var EXECUTE_DATA_CC);
		// 调试模式 并且 是临时变量
		} else {
			// 必须是临时变量
			ZEND_ASSERT(op_type == IS_TMP_VAR);
			// ing4, 通过偏移量获取 执行数据 中的 第n个 zval, 不可以是引用类型
			return _get_zval_ptr_tmp(node.var EXECUTE_DATA_CC);
		}
	// 类型是其他
	} else {
		// 类型是常量
		if (op_type == IS_CONST) {
			// 在p1中通过偏移量p2.constant 获取 zval 指针
			return RT_CONSTANT(opline, node);
		// 类型是编译变量
		} else if (op_type == IS_CV) {
			// ing4, 根据序号 和 操作类型，返回zval。变量是未定义时，附加处理：
			// 读或删除:报错并返回未初始化zval，检查:返回未初始化zval，更新：报错，创建：返回null
			return _get_zval_ptr_cv(node.var, type EXECUTE_DATA_CC);
		// 其他类型
		} else {
			return NULL;
		}
	}
}

// ing4, 在执行数据中通过 p1:运算对象类型，p3:操作类型，取回p2:运算对象指向的zval
//（常量取下一行操作码）（编译变量未定义报错）
static zend_always_inline zval *_get_op_data_zval_ptr_r(int op_type, znode_op node EXECUTE_DATA_DC OPLINE_DC)
{
	// 类型是变量
	if (op_type & (IS_TMP_VAR|IS_VAR)) {
		if (!ZEND_DEBUG || op_type == IS_VAR) {
			// ing4, 通过偏移量获取 执行数据 中的 第n个 zval
			return _get_zval_ptr_var(node.var EXECUTE_DATA_CC);
		} else {
			ZEND_ASSERT(op_type == IS_TMP_VAR);
			// ing4, 通过偏移量获取 执行数据 中的 第n个 zval, 不可以是引用类型
			return _get_zval_ptr_tmp(node.var EXECUTE_DATA_CC);
		}
	// 其他类型
	} else {
		// 类型是常量
		if (op_type == IS_CONST) {
			// 在p1中通过偏移量p2.constant 获取 zval 指针
			return RT_CONSTANT(opline + 1, node);
		// 类型是编译变量
		} else if (op_type == IS_CV) {
			// ing4, 从执行数据中，取回指定编号的变量，是未定义则报错
			return _get_zval_ptr_cv_BP_VAR_R(node.var EXECUTE_DATA_CC);
		// 其他类型
		} else {
			return NULL;
		}
	}
}

// ing4, 在执行数据中通过 p1:运算对象类型，p3:操作类型，取回p2:运算对象指向的zval（解引用）
static zend_always_inline ZEND_ATTRIBUTE_UNUSED zval *_get_zval_ptr_deref(int op_type, znode_op node, int type EXECUTE_DATA_DC OPLINE_DC)
{
	// 如果类型是变量
	if (op_type & (IS_TMP_VAR|IS_VAR)) {
		if (op_type == IS_TMP_VAR) {
			// ing4, 通过偏移量获取 执行数据 中的 第n个 zval, 不可以是引用类型
			return _get_zval_ptr_tmp(node.var EXECUTE_DATA_CC);
		} else {
			ZEND_ASSERT(op_type == IS_VAR);
			// ing4, 通过偏移量获取 执行数据 中的 第n个 zval（解引用）
			return _get_zval_ptr_var_deref(node.var EXECUTE_DATA_CC);
		}
	// 类型是其他
	} else {
		if (op_type == IS_CONST) {
			// 在p1中通过偏移量p2.constant 获取 zval 指针
			return RT_CONSTANT(opline, node);
		} else if (op_type == IS_CV) {
			// ing4, 根据序号 和 操作类型，返回zval（解引用）
			return _get_zval_ptr_cv_deref(node.var, type EXECUTE_DATA_CC);
		} else {
			return NULL;
		}
	}
}

// ing4, 在执行数据中通过 p1:运算对象类型，p3:操作类型，取回p2:运算对象指向的zval
//（常量取下一行操作码）（编译变量未定义报错）（解引用）
static zend_always_inline ZEND_ATTRIBUTE_UNUSED zval *_get_op_data_zval_ptr_deref_r(int op_type, znode_op node EXECUTE_DATA_DC OPLINE_DC)
{
	if (op_type & (IS_TMP_VAR|IS_VAR)) {
		if (op_type == IS_TMP_VAR) {
			// ing4, 通过偏移量获取 执行数据 中的 第n个 zval, 不可以是引用类型
			return _get_zval_ptr_tmp(node.var EXECUTE_DATA_CC);
		} else {
			ZEND_ASSERT(op_type == IS_VAR);
			// ing4, 通过偏移量获取 执行数据 中的 第n个 zval（解引用）
			return _get_zval_ptr_var_deref(node.var EXECUTE_DATA_CC);
		}
	} else {
		if (op_type == IS_CONST) {
			// 在p1中通过偏移量p2.constant 获取 zval 指针
			return RT_CONSTANT(opline + 1, node);
		} else if (op_type == IS_CV) {
			// ing4, 从执行数据中，取回指定编号的变量（解引用），是未定义则报错
			return _get_zval_ptr_cv_deref_BP_VAR_R(node.var EXECUTE_DATA_CC);
		} else {
			return NULL;
		}
	}
}

// ing4, 在执行数据中通过 p1:运算对象类型，p3:操作类型，取回p2:运算对象 指向的zval（编译变量无附加处理）
static zend_always_inline zval *_get_zval_ptr_undef(int op_type, znode_op node, int type EXECUTE_DATA_DC OPLINE_DC)
{
	// 如果类型是变量
	if (op_type & (IS_TMP_VAR|IS_VAR)) {
		// 正式环境 或 是普通变量
		if (!ZEND_DEBUG || op_type == IS_VAR) {
			// ing4, 通过偏移量获取 执行数据 中的 第n个 zval
			return _get_zval_ptr_var(node.var EXECUTE_DATA_CC);
		} else {
			// 临时变量
			ZEND_ASSERT(op_type == IS_TMP_VAR);
			// ing4, 通过偏移量获取 执行数据 中的 第n个 zval, 不可以是引用类型
			return _get_zval_ptr_tmp(node.var EXECUTE_DATA_CC);
		}
	// 类型不是变量
	} else {
		if (op_type == IS_CONST) {
			// 在p1中通过偏移量p2.constant 获取 zval 指针
			return RT_CONSTANT(opline, node);
		} else if (op_type == IS_CV) {
			// 获取指定序号的变量
			// #define EX_VAR(n) ZEND_CALL_VAR(execute_data, n)
			// #define ZEND_CALL_VAR(call, n) ((zval*)(((char*)(call)) + ((int)(n))))
			return EX_VAR(node.var);
		} else {
			return NULL;
		}
	}
}

// ing4, 在执行数据中通过 p1:序号 取回zval（解间接引用）
static zend_always_inline zval *_get_zval_ptr_ptr_var(uint32_t var EXECUTE_DATA_DC)
{
	// 取得指定的变量
	// #define EX_VAR(n) ZEND_CALL_VAR(execute_data, n)
	// #define ZEND_CALL_VAR(call, n) ((zval*)(((char*)(call)) + ((int)(n))))
	zval *ret = EX_VAR(var);
	
	// 如果它是间接引用类型
	if (EXPECTED(Z_TYPE_P(ret) == IS_INDIRECT)) {
		// 追踪到引用目标
		ret = Z_INDIRECT_P(ret);
	}
	// 返回这个变量
	return ret;
}

// ing4, 在执行数据中通过 p1:运算对象类型，p3:操作类型，取回p2:运算对象 指向的zval
// 编译变量带附加处理，普通变量解间接引用
static inline zval *_get_zval_ptr_ptr(int op_type, znode_op node, int type EXECUTE_DATA_DC)
{
	// 类型是编译变量
	if (op_type == IS_CV) {
		// ing4, 根据序号 和 操作类型，返回zval。变量是未定义时，附加处理：
		// 读或删除:报错并返回未初始化zval，检查:返回未初始化zval，更新：报错，创建：返回null
		return _get_zval_ptr_cv(node.var, type EXECUTE_DATA_CC);
	// 类型是变量
	} else /* if (op_type == IS_VAR) */ {
		ZEND_ASSERT(op_type == IS_VAR);
		// ing4, 在执行数据中通过 p1:序号 取回zval（解间接引用）
		return _get_zval_ptr_ptr_var(node.var EXECUTE_DATA_CC);
	}
}

// ing4, 在执行数据中通过 p1:运算对象类型，p2:运算对象，p3:操作类型，取回需要的zval
// 运算对象类型为 IS_UNUSED 时返回 this
static inline ZEND_ATTRIBUTE_UNUSED zval *_get_obj_zval_ptr(int op_type, znode_op op, int type EXECUTE_DATA_DC OPLINE_DC)
{
	// 如果 变量类型 未定义，返回This
	if (op_type == IS_UNUSED) {
		return &EX(This);
	}
	// ing4, 在执行数据中通过 p1:运算对象类型，p2:运算对象，p3:操作类型，取回需要的zval
	return get_zval_ptr(op_type, op, type);
}

// ing4, 在执行数据中通过 p1:运算对象类型，p3:操作类型，取回p2:运算对象 指向的zval
// （编译变量无附加处理） 运算对象类型为 IS_UNUSED 时返回 this
static inline ZEND_ATTRIBUTE_UNUSED zval *_get_obj_zval_ptr_undef(int op_type, znode_op op, int type EXECUTE_DATA_DC OPLINE_DC)
{
	// 如果 变量类型 未定义，返回This
	if (op_type == IS_UNUSED) {
		return &EX(This);
	}
	// ing4, 在执行数据中通过 p1:运算对象类型，p3:操作类型，取回p2:运算对象 指向的zval（编译变量无附加处理）
	return get_zval_ptr_undef(op_type, op, type);
}

// ing4, 在执行数据中通过 p1:运算对象类型，p3:操作类型，取回p2:运算对象 指向的zval
// 编译变量带附加处理，普通变量解间接引用，运算对象类型为 IS_UNUSED 时返回 this
static inline ZEND_ATTRIBUTE_UNUSED zval *_get_obj_zval_ptr_ptr(int op_type, znode_op node, int type EXECUTE_DATA_DC)
{
	// 如果 变量类型 未定义，返回This
	if (op_type == IS_UNUSED) {
		return &EX(This);
	}
	// ing4, 在执行数据中通过 p1:运算对象类型，p3:操作类型，取回p2:运算对象 指向的zval
	// 编译变量带附加处理，普通变量解间接引用
	return get_zval_ptr_ptr(op_type, node, type);
}

// ing3, 对变量进行引用赋值。p1:变量指针，p2:值指针
static inline void zend_assign_to_variable_reference(zval *variable_ptr, zval *value_ptr)
{
	zend_reference *ref;
	// 如果值不是引用类型
	if (EXPECTED(!Z_ISREF_P(value_ptr))) {
		// 转化成引用类型
		ZVAL_NEW_REF(value_ptr, value_ptr);
	// 如果是引用类型，并且值与变量是同一个
	} else if (UNEXPECTED(variable_ptr == value_ptr)) {
		// 中断，效果已达到
		return;
	}

	// 取出引用实例
	ref = Z_REF_P(value_ptr);
	// 引用实例增加引用 次数
	GC_ADDREF(ref);
	// 如果变量可计数
	if (Z_REFCOUNTED_P(variable_ptr)) {
		// 取得计数器
		zend_refcounted *garbage = Z_COUNTED_P(variable_ptr);

		// 如果原值 引用-1后为0
		if (GC_DELREF(garbage) == 0) {
			// 新值 引用赋值给变量
			ZVAL_REF(variable_ptr, ref);
			// 销毁旧值
			rc_dtor_func(garbage);
			// 完成
			return;
		// 否则
		} else {
			// 原值添加到垃圾回收
			gc_check_possible_root(garbage);
		}
	}
	// 新值 引用赋值给变量
	ZVAL_REF(variable_ptr, ref);
}

// ing3, 对有类型的属性进行引用赋值，p1:属性信息，p2:属性实例，p3:新值
static zend_never_inline zval* zend_assign_to_typed_property_reference(zend_property_info *prop_info, zval *prop, zval *value_ptr EXECUTE_DATA_DC)
{
	// 某执行上下文所属函数 的参数是否使用严格类型限制
	if (!zend_verify_prop_assignable_by_ref(prop_info, value_ptr, EX_USES_STRICT_TYPES())) {
		// 没有类型限制，返回未初始化zval
		return &EG(uninitialized_zval);
	}
	// 如果值为引用类型
	if (Z_ISREF_P(prop)) {
		// 找到并删除源指针列表中 指向属性信息的指针。p1:源列表，p2:属性信息指针
		ZEND_REF_DEL_TYPE_SOURCE(Z_REF_P(prop), prop_info);
	}
	// 对变量进行引用赋值
	zend_assign_to_variable_reference(prop, value_ptr);
	// 向zend_property_info_source_list指向的 zend_property_info 列表中添加 zend_property_info
	ZEND_REF_ADD_TYPE_SOURCE(Z_REF_P(prop), prop_info);
	
	return prop;
}

// ing3, 给引用类型 赋值出错，有错误提示信息，但还是进行赋值了。p1:变量指针，p2:值指针
static zend_never_inline ZEND_COLD zval *zend_wrong_assign_to_variable_reference(zval *variable_ptr, zval *value_ptr OPLINE_DC EXECUTE_DATA_DC)
{
	// 提示信息：只有变量可以引用传递
	zend_error(E_NOTICE, "Only variables should be assigned by reference");
	// 如果有异常
	if (UNEXPECTED(EG(exception) != NULL)) {
		// 返回未初始化的 zval 
		return &EG(uninitialized_zval);
	}
	// 没有异常到这里

	// 使用临时变量代替变量 来 避免 引用类型检查
	/* Use IS_TMP_VAR instead of IS_VAR to avoid ISREF check */
	Z_TRY_ADDREF_P(value_ptr);
	
	// 给变量赋值，p1:变量，p2:值，p3:值的变量类型（常量，变量，编译变量），p4:是否严格类型
	// 某执行上下文所属函数 的参数是否使用严格类型限制
	return zend_assign_to_variable(variable_ptr, value_ptr, IS_TMP_VAR, EX_USES_STRICT_TYPES());
}

// ing3, 抛错：此参数不可以使用引用传递
ZEND_API ZEND_COLD void ZEND_FASTCALL zend_cannot_pass_by_reference(uint32_t arg_num)
{
	// 执行数据
	const zend_execute_data *execute_data = EG(current_execute_data);
	// 获取函数名
	zend_string *func_name = get_function_or_method_name(EX(call)->func);
	// 取得要求的参数名
	const char *param_name = get_function_arg_name(EX(call)->func, arg_num);
	// 抛错; 此参数不可以使用引用传递
	zend_throw_error(NULL, "%s(): Argument #%d%s%s%s cannot be passed by reference",
		ZSTR_VAL(func_name), arg_num, param_name ? " ($" : "", param_name ? param_name : "", param_name ? ")" : ""
	);
	// 销毁函数名
	zend_string_release(func_name);
}

// ing3, 报错：自动初始化属性错误
static zend_never_inline ZEND_COLD void zend_throw_auto_init_in_prop_error(zend_property_info *prop) {
	// 属性信息的类型转成字串
	zend_string *type_str = zend_type_to_string(prop->type);
	// 报错：不可以在A类型的属性B::C中自动初始化数组
	zend_type_error(
		"Cannot auto-initialize an array inside property %s::$%s of type %s",
		ZSTR_VAL(prop->ce->name), zend_get_unmangled_property_name(prop->name),
		ZSTR_VAL(type_str)
	);
	// 释放类型字串
	zend_string_release(type_str);
}

// ing3, 报错：无法自动初始化 属性 中 的引用类型 中 的数组
static zend_never_inline ZEND_COLD void zend_throw_auto_init_in_ref_error(zend_property_info *prop) {
	// 属性信息的类型转成字串
	zend_string *type_str = zend_type_to_string(prop->type);
	// 无法自动初始化 属性 中 的引用类型 中 的数组
	zend_type_error(
		"Cannot auto-initialize an array inside a reference held by property %s::$%s of type %s",
		ZSTR_VAL(prop->ce->name), zend_get_unmangled_property_name(prop->name),
		ZSTR_VAL(type_str)
	);
	// 释放类型字串
	zend_string_release(type_str);
}

// ing4, 报错：通过引用访问未初始化的属性
static zend_never_inline ZEND_COLD void zend_throw_access_uninit_prop_by_ref_error(
		zend_property_info *prop) {
	// 报错：不可以 通过引用访问未初始化的非null属性
	zend_throw_error(NULL,
		"Cannot access uninitialized non-nullable property %s::$%s by reference",
		ZSTR_VAL(prop->ce->name),
		zend_get_unmangled_property_name(prop->name));
}

/* this should modify object only if it's empty */
// ing3, 抛出【对象无效】异常
static zend_never_inline ZEND_COLD void ZEND_FASTCALL zend_throw_non_object_error(zval *object, zval *property OPLINE_DC EXECUTE_DATA_DC)
{
	zend_string *tmp_property_name;
	zend_string *property_name = zval_get_tmp_string(property, &tmp_property_name);
	// 如果是obj前面或后面的 ++/--
	if (opline->opcode == ZEND_PRE_INC_OBJ
	 || opline->opcode == ZEND_PRE_DEC_OBJ
	 || opline->opcode == ZEND_POST_INC_OBJ
	 || opline->opcode == ZEND_POST_DEC_OBJ) {
		// 报错 ：对属性进行自增/自减操作出错
		zend_throw_error(NULL,
			"Attempt to increment/decrement property \"%s\" on %s",
			ZSTR_VAL(property_name), zend_zval_type_name(object)
		);
	// 创建或更新对象属性，获取函数参数，引用赋值。这几种操作
	} else if (opline->opcode == ZEND_FETCH_OBJ_W
			|| opline->opcode == ZEND_FETCH_OBJ_RW
			|| opline->opcode == ZEND_FETCH_OBJ_FUNC_ARG
			|| opline->opcode == ZEND_ASSIGN_OBJ_REF) {
		// 报错 ：尝试修改属性
		zend_throw_error(NULL,
			"Attempt to modify property \"%s\" on %s",
			ZSTR_VAL(property_name), zend_zval_type_name(object)
		);
	// 其他情况
	} else {
		// 报错 ：尝试给属性赋值
		zend_throw_error(NULL,
			"Attempt to assign property \"%s\" on %s",
			ZSTR_VAL(property_name), zend_zval_type_name(object)
		);
	}
	// 释放临时属性名
	zend_tmp_string_release(tmp_property_name);

	// ing4, 检验：opline 返回类型不是IS_UNUSED
	if (UNEXPECTED(RETURN_VALUE_USED(opline))) {
		// 结果指向的变量 赋值为null
		ZVAL_NULL(EX_VAR(opline->result.var));
	}
}

// ing3, 根据传入的值返回类型：p1:函数，p2:参数信息, p3:值。以下返回，p4:函数名，p5:分隔符，p6:类名，p7:消息，p8:传入值类型
static ZEND_COLD void zend_verify_type_error_common(
		const zend_function *zf, const zend_arg_info *arg_info, zval *value,
		const char **fname, const char **fsep, const char **fclass,
		zend_string **need_msg, const char **given_kind)
{
	// 函数名
	*fname = ZSTR_VAL(zf->common.function_name);
	// 如果有所属类
	if (zf->common.scope) {
		// 分隔符
		*fsep =  "::";
		// 类名
		*fclass = ZSTR_VAL(zf->common.scope->name);
	} else {
		*fsep =  "";
		*fclass = "";
	}

	// 类型转成字串
	*need_msg = zend_type_to_string_resolved(arg_info->type, zf->common.scope);

	// 如果有值
	if (value) {
		// 传入类型
		*given_kind = zend_zval_type_name(value);
	// 没有值
	} else {
		// 类型为none
		*given_kind = "none";
	}
}

// ing3, 报错：参数验证错误
ZEND_API ZEND_COLD void zend_verify_arg_error(
		const zend_function *zf, const zend_arg_info *arg_info, uint32_t arg_num, zval *value)
{
	// 前一个执行数据
	zend_execute_data *ptr = EG(current_execute_data)->prev_execute_data;
	// 临时变量
	const char *fname, *fsep, *fclass;
	zend_string *need_msg;
	const char *given_msg;
	// 根据传入的值返回类型：p1:函数，p2:参数信息, p3:值。以下返回，p4:函数名，p5:分隔符，p6:类名，p7:消息，p8:传入值类型
	zend_verify_type_error_common(
		zf, arg_info, value, &fname, &fsep, &fclass, &need_msg, &given_msg);

	ZEND_ASSERT(zf->common.type == ZEND_USER_FUNCTION
		&& "Arginfo verification is not performed for internal functions");
	// 如果当前在函数中，并且是用户定义函数 
	if (ptr && ptr->func && ZEND_USER_CODE(ptr->func->common.type)) {
		// 报错：参数必须是 A 类型，但给了 B 类型，某文件某行
		zend_argument_type_error(arg_num, "must be of type %s, %s given, called in %s on line %d",
			ZSTR_VAL(need_msg), given_msg,
			ZSTR_VAL(ptr->func->op_array.filename), ptr->opline->lineno
		);
	// 其他情况
	} else {
		// 报错：参数必须是 A 类型，但给了 B 类型
		zend_argument_type_error(arg_num,
			"must be of type %s, %s given", ZSTR_VAL(need_msg), given_msg);
	}
	// 释放临时变量
	zend_string_release(need_msg);
}

// ing3, 弱标量类型检查和适配, 类型优先顺序 ：整数 -> 小数 -> 字串 -> 布尔
static bool zend_verify_weak_scalar_type_hint(uint32_t type_mask, zval *arg)
{
	zend_long lval;
	double dval;
	zend_string *str;
	bool bval;
	// 类型优先顺序 ：整数 -> 小数 -> 字串 -> 布尔
	/* Type preference order: int -> float -> string -> bool */
	// 支持 整数类型
	if (type_mask & MAY_BE_LONG) {
		// 对于 整数|小数 联合类型 和 字串值，通过 is_numeric_string 来决定选择的类型
		/* For an int|float union type and string value,
		 * determine chosen type by is_numeric_string() semantics. */
		 
		// 如果允许小数
		if ((type_mask & MAY_BE_DOUBLE) && Z_TYPE_P(arg) == IS_STRING) {
			// 转成数值
			zend_uchar type = is_numeric_str_function(Z_STR_P(arg), &lval, &dval);
			// 如果转成整数
			if (type == IS_LONG) {
				// 释放原来的字串
				zend_string_release(Z_STR_P(arg));
				// 写入整数
				ZVAL_LONG(arg, lval);
				// 返回成功
				return 1;
			}
			// 如果转成小数
			if (type == IS_DOUBLE) {
				// 释放原来的字串
				zend_string_release(Z_STR_P(arg));
				// 写入小数
				ZVAL_DOUBLE(arg, dval);
				// 返回成功
				return 1;
			}
		// 如果不允许小数。把传入的参数转换成整数，遇到null会报错。结果从参数2返回，参数3是 arg参数序号，报错时用
		} else if (zend_parse_arg_long_weak(arg, &lval, 0)) {
			// 销毁没有引用次数的对象，有引用次数的添加到gc回收周期里。并没有销毁 zval本身。
			zval_ptr_dtor(arg);
			// 写入整数值
			ZVAL_LONG(arg, lval);
			// 返回 成功
			return 1;
		// 如果有异常
		} else if (UNEXPECTED(EG(exception))) {
			// 返回失败
			return 0;
		}
	}
	
	// 如果支持小数 。 把传入的参数转换成小数，遇到null会报错。结果从参数2返回，参数3是 arg参数序号，报错时用
	if ((type_mask & MAY_BE_DOUBLE) && zend_parse_arg_double_weak(arg, &dval, 0)) {
		// 销毁没有引用次数的对象，有引用次数的添加到gc回收周期里。并没有销毁 zval本身。
		zval_ptr_dtor(arg);
		ZVAL_DOUBLE(arg, dval);
		return 1;
	}
	// 如果支持字串，把传入的参数转换成字串，遇到null会报错。p2:返回值，p3:arg参数序号，报错用
	if ((type_mask & MAY_BE_STRING) && zend_parse_arg_str_weak(arg, &str, 0)) {
		/* on success "arg" is converted to IS_STRING */
		return 1;
	}
	// 如果支持布尔型，获取第n个 bool 类型的值，p2:返回值，p3:arg参数序号，报错用
	if ((type_mask & MAY_BE_BOOL) == MAY_BE_BOOL && zend_parse_arg_bool_weak(arg, &bval, 0)) {
		// 销毁没有引用次数的对象，有引用次数的添加到gc回收周期里。并没有销毁 zval本身。
		zval_ptr_dtor(arg);
		ZVAL_BOOL(arg, bval);
		return 1;
	}
	// 不支持其他类型
	return 0;
}

#if ZEND_DEBUG
// suspend, 调试用
static bool can_convert_to_string(zval *zv) {
	/* We don't call cast_object here, because this check must be side-effect free. As this
	 * is only used for a sanity check of arginfo/zpp consistency, it's okay if we accept
	 * more than actually allowed here. */
	if (Z_TYPE_P(zv) == IS_OBJECT) {
		return Z_OBJ_HT_P(zv)->cast_object != zend_std_cast_object_tostring
			|| Z_OBJCE_P(zv)->__tostring;
	}
	return Z_TYPE_P(zv) <= IS_STRING;
}

/* Used to sanity-check internal arginfo types without performing any actual type conversions. */
// suspend, 调试用
static bool zend_verify_weak_scalar_type_hint_no_sideeffect(uint32_t type_mask, zval *arg)
{
	zend_long lval;
	double dval;
	bool bval;

	/* Pass (uint32_t)-1 as arg_num to indicate to ZPP not to emit any deprecation notice,
	 * this is needed because the version with side effects also uses 0 (e.g. for typed properties) */
	if ((type_mask & MAY_BE_LONG) && zend_parse_arg_long_weak(arg, &lval, (uint32_t)-1)) {
		return 1;
	}
	if ((type_mask & MAY_BE_DOUBLE) && zend_parse_arg_double_weak(arg, &dval, (uint32_t)-1)) {
		return 1;
	}
	if ((type_mask & MAY_BE_STRING) && can_convert_to_string(arg)) {
		return 1;
	}
	if ((type_mask & MAY_BE_BOOL) == MAY_BE_BOOL && zend_parse_arg_bool_weak(arg, &bval, (uint32_t)-1)) {
		return 1;
	}
	return 0;
}
#endif

// ing3, 检查 参数 和 类型是否匹配，p1:类型，p2:参数，p3:是否严格，p4:是否内置参数
ZEND_API bool zend_verify_scalar_type_hint(uint32_t type_mask, zval *arg, bool strict, bool is_internal_arg)
{
	// 如果用严格类型
	if (UNEXPECTED(strict)) {
		// SSTH异常：整数 也可以 允许小数（需要转换）
		/* SSTH Exception: IS_LONG may be accepted as IS_DOUBLE (converted) */
		// 如果类型不包含小数 并且参数不是整数
		if (!(type_mask & MAY_BE_DOUBLE) || Z_TYPE_P(arg) != IS_LONG) {
			// 返回 false
			return 0;
		}
	// 不用严格类型. 如果值为null
	} else if (UNEXPECTED(Z_TYPE_P(arg) == IS_NULL)) {
		// null只有在nullable类型有效（检验过）
		// 对于内置函数的异常，在 弱模式中， 标题类型允许null
		/* NULL may be accepted only by nullable hints (this is already checked).
		 * As an exception for internal functions, null is allowed for scalar types in weak mode. */
		// 如果类型包含 true 或 false 或 整数 或 小数 或 字串。结果为真，都不包含时为假。
		return is_internal_arg
			&& (type_mask & (MAY_BE_TRUE|MAY_BE_FALSE|MAY_BE_LONG|MAY_BE_DOUBLE|MAY_BE_STRING));
	}
// 调试用
#if ZEND_DEBUG
	if (is_internal_arg) {
		return zend_verify_weak_scalar_type_hint_no_sideeffect(type_mask, arg);
	}
#endif
	// 弱标量类型检查和适配, 类型优先顺序 ：整数 -> 小数 -> 字串 -> 布尔
	return zend_verify_weak_scalar_type_hint(type_mask, arg);
}

// ing3, 属性类型查检报错
ZEND_COLD zend_never_inline void zend_verify_property_type_error(zend_property_info *info, zval *property)
{
	zend_string *type_str;

	// 当已经有异常信息 并且 运行时缓存 没有被更新（例如：它包含一个可用但无关的信息）
	/* we _may_ land here in case reading already errored and runtime cache thus has not been updated (i.e. it contains a valid but unrelated info) */
	// 如果已经有异常了，直接返回
	if (EG(exception)) {
		return;
	}

	// 类型转成字串
	type_str = zend_type_to_string(info->type);
	// 报错：不可以把此类型的值 赋给 要求不同类型的类属性
	zend_type_error("Cannot assign %s to property %s::$%s of type %s",
		zend_zval_type_name(property),
		ZSTR_VAL(info->ce->name),
		zend_get_unmangled_property_name(info->name),
		ZSTR_VAL(type_str));
	// 释放类型名
	zend_string_release(type_str);
}

// ing4, match() 匹配出错 
ZEND_COLD void zend_match_unhandled_error(zval *value)
{
	smart_str msg = {0};
	// 如果是整数或字串上
	if (Z_TYPE_P(value) <= IS_STRING) {
		// 把value添加进报错信息
		smart_str_append_scalar(&msg, value, EG(exception_string_param_max_len));
	// 其他类型
	} else {
		// 把value的类型添加进报错信息
		smart_str_appendl(&msg, "of type ", sizeof("of type ")-1);
		smart_str_appends(&msg, zend_zval_type_name(value));
	}

	// 添加\0
	smart_str_0(&msg);
	// 抛错
	zend_throw_exception_ex(
		zend_ce_unhandled_match_error, 0, "Unhandled match case %s", ZSTR_VAL(msg.s));
	// 删除错误信息
	smart_str_free(&msg);
}

// ing4, 报错：不能修改只读属性
ZEND_API ZEND_COLD void ZEND_FASTCALL zend_readonly_property_modification_error(
		zend_property_info *info) {
	zend_throw_error(NULL, "Cannot modify readonly property %s::$%s",
		ZSTR_VAL(info->ce->name), zend_get_unmangled_property_name(info->name));
}

// ing4, 报错：不能间接修改只读属性
ZEND_API ZEND_COLD void ZEND_FASTCALL zend_readonly_property_indirect_modification_error(zend_property_info *info)
{
	zend_throw_error(NULL, "Cannot indirectly modify readonly property %s::$%s",
		ZSTR_VAL(info->ce->name), zend_get_unmangled_property_name(info->name));
}

// ing3, 根据类名（self,parent,其他）返回类入口, 可传入this
static zend_class_entry *resolve_single_class_type(zend_string *name, zend_class_entry *self_ce) {
	// self
	if (zend_string_equals_literal_ci(name, "self")) {
		return self_ce;
	// parent
	} else if (zend_string_equals_literal_ci(name, "parent")) {
		return self_ce->parent;
	// 其他情况，查找类
	} else {
		// 查找类。p1:类名，p2:key，p3:flags
		return zend_lookup_class_ex(name, NULL, ZEND_FETCH_CLASS_NO_AUTOLOAD);
	}
}

// ing4, 获取类入口（先用类型在缓存中找。再用类型和属性信息进行普通查找：支持self,parent）
static zend_always_inline zend_class_entry *zend_ce_from_type(
		zend_property_info *info, zend_type *type) {
	// type 必须要有名称
	ZEND_ASSERT(ZEND_TYPE_HAS_NAME(*type));
	// 取得类型名称
	zend_string *name = ZEND_TYPE_NAME(*type);
	// 先在缓存中找，如果有类缓存
	if (ZSTR_HAS_CE_CACHE(name)) {
		// 取得类入口
		zend_class_entry *ce = ZSTR_GET_CE_CACHE(name);
		// 如果获取失败
		if (!ce) {
			// 查找类。p1:类名，p2:key，p3:flags
			ce = zend_lookup_class_ex(name, NULL, ZEND_FETCH_CLASS_NO_AUTOLOAD);
		}
		// 
		return ce;
	}
	// 根据类名（self,parent,其他）返回类入口, 可传入this
	return resolve_single_class_type(name, info->ce);
}

// ing3, 为类属性信息 检验交叉类型（多个类型只要有一个类型不满足，结果为假）, object_ce 是对象的所属类
static bool zend_check_intersection_for_property_class_type(zend_type_list *intersection_type_list,
	zend_property_info *info, zend_class_entry *object_ce)
{
	zend_type *list_type;
	// 遍历 交叉类型 type 列表
	ZEND_TYPE_LIST_FOREACH(intersection_type_list, list_type) {
		// 此元素里不可以还有列表
		ZEND_ASSERT(!ZEND_TYPE_HAS_LIST(*list_type));
		// 获取类入口（先用类型在缓存中找。再用类型和属性信息进行普通查找：支持self,parent）
		zend_class_entry *ce = zend_ce_from_type(info, list_type);
		// 如果此类型没有类入口，或 此对象不属于此属性要求的类型
		if (!ce || !instanceof_function(object_ce, ce)) {
			// 只要有一个不满足，结果为假
			return false;
		}
	} ZEND_TYPE_LIST_FOREACH_END();
	// 必须交叉类型的每一个类型都满足，才会返回真
	return true;
}

// ing4, 检查此对象 与此属性信息是否匹配（是否可以作为此属性的值）
static bool zend_check_and_resolve_property_class_type(
		zend_property_info *info, zend_class_entry *object_ce) {
	// 如果type包含属性列表
	if (ZEND_TYPE_HAS_LIST(info->type)) {
		zend_type *list_type;
		// 如果是交叉类型
		if (ZEND_TYPE_IS_INTERSECTION(info->type)) {
			// 为类属性信息 检验交叉类型（多个类型只要有一个类型不满足，结果为假）, object_ce 是对象的所属类
			return zend_check_intersection_for_property_class_type(
				ZEND_TYPE_LIST(info->type), info, object_ce);
		// 不是交叉，那就是union咯
		} else {
			// 遍历 类型列表, union列表里只要有一组 子类型 匹配就返回真
			ZEND_TYPE_LIST_FOREACH(ZEND_TYPE_LIST(info->type), list_type) {
				// 如果当前 子类型 是交叉类型
				if (ZEND_TYPE_IS_INTERSECTION(*list_type)) {
					// 为类属性信息 检验交叉类型（多个类型只要有一个类型不满足，结果为假）, object_ce 是对象的所属类
					if (zend_check_intersection_for_property_class_type(
							ZEND_TYPE_LIST(*list_type), info, object_ce)) {
						// 只要有一组类型匹配就返回真
						return true;
                    }
					// 下一个类型
					continue;
				}
				
				// 如果前 子类型不是交叉类型，这里不可以是类型列表，因为已经在列表里了
				ZEND_ASSERT(!ZEND_TYPE_HAS_LIST(*list_type));
				// 获取类入口（先用类型在缓存中找。再用类型和属性信息进行普通查找：支持self,parent）
				zend_class_entry *ce = zend_ce_from_type(info, list_type);
				// 检验继承或接口实现，单向
				if (ce && instanceof_function(object_ce, ce)) {
					// 只要有一个类型匹配就返回真
					return true;
				}
			} ZEND_TYPE_LIST_FOREACH_END();
			return false;
		}
	// 如果 type不包含属性列表, 那它是单个属性
	} else {
		// 获取类入口（先用类型在缓存中找。再用类型和属性信息进行普通查找：支持self,parent）
		zend_class_entry *ce = zend_ce_from_type(info, &info->type);
		// 检验继承或接口实现，单向
		return ce && instanceof_function(object_ce, ce);
	}
}

// ing3, 检查属性值类型 与 属性信息要求的类型是否匹配
static zend_always_inline bool i_zend_check_property_type(zend_property_info *info, zval *property, bool strict)
{
	ZEND_ASSERT(!Z_ISREF_P(property));
	// 如果属性信息中的类型 包含 属性值的类型
	if (EXPECTED(ZEND_TYPE_CONTAINS_CODE(info->type, Z_TYPE_P(property)))) {
		// 验证通过
		return 1;
	}

	// 1，检验是否是复杂类型, 复杂类型是指：传入的是（一个列表加一个(union 或 intersection)） 或者 （类名）
	// 2，property 本身要是对象（这样才有复杂类型）
	// 3，检查此对象 与此属性信息是否匹配（是否可以作为此属性的值）
	if (ZEND_TYPE_IS_COMPLEX(info->type) && Z_TYPE_P(property) == IS_OBJECT
			&& zend_check_and_resolve_property_class_type(info, Z_OBJCE_P(property))) {
		// 验证通过
		return 1;
	}

	// 获取此类型的mask（所有可能类型的组合）
	uint32_t type_mask = ZEND_TYPE_FULL_MASK(info->type);
	
	// 必须不是闭包 也没有 static 标记
	ZEND_ASSERT(!(type_mask & (MAY_BE_CALLABLE|MAY_BE_STATIC)));
	// 检查 参数 和 类型是否匹配，p1:类型，p2:参数，p3:是否严格，p4:是否内置参数
	return zend_verify_scalar_type_hint(type_mask, property, strict, 0);
}

// ing3, 检查属性值类型 与 属性信息要求的类型是否匹配
static zend_always_inline bool i_zend_verify_property_type(zend_property_info *info, zval *property, bool strict)
{
	// 检查属性值类型 与 属性信息要求的类型是否匹配
	if (i_zend_check_property_type(info, property, strict)) {
		// 返回是
		return 1;
	}
	// 属性类型查检报错
	zend_verify_property_type_error(info, property);
	// 返回 否
	return 0;
}

// ing4, 检查属性值类型 与 属性信息要求的类型是否匹配
ZEND_API bool zend_never_inline zend_verify_property_type(zend_property_info *info, zval *property, bool strict) {
	return i_zend_verify_property_type(info, property, strict);
}

// ing3, 给有类型的属性 赋值。p1:属性信息，p2:属性值zval，p3:新值zval
static zend_never_inline zval* zend_assign_to_typed_prop(zend_property_info *info, zval *property_val, zval *value EXECUTE_DATA_DC)
{
	zval tmp;
	// 如果是只读属性
	if (UNEXPECTED(info->flags & ZEND_ACC_READONLY)) {
		// 报错：不能修改只读属性
		zend_readonly_property_modification_error(info);
		// 返回空zval
		return &EG(uninitialized_zval);
	}

	// 如果是引用类型，追踪到被引用的对象
	ZVAL_DEREF(value);
	// 复制到临时变量
	ZVAL_COPY(&tmp, value);

	// 检查属性值类型 与 属性信息要求的类型是否匹配
	// 某执行上下文所属函数 的参数是否使用严格类型限制
	if (UNEXPECTED(!i_zend_verify_property_type(info, &tmp, EX_USES_STRICT_TYPES()))) {
		// 销毁没有引用次数的对象，有引用次数的添加到gc回收周期里。并没有销毁 zval本身。
		zval_ptr_dtor(&tmp);
		// 返回空
		return &EG(uninitialized_zval);
	}

	// 给变量赋值，p1:变量，p2:值，p3:值的变量类型（常量，变量，编译变量），p4:是否严格类型
	// 某执行上下文所属函数 的参数是否使用严格类型限制
	return zend_assign_to_variable(property_val, &tmp, IS_TMP_VAR, EX_USES_STRICT_TYPES());
}

// ing4, 检查 zval 是否是 当前执行域的 子类
static zend_always_inline bool zend_value_instanceof_static(zval *zv) {
	// 如果值不是对象
	if (Z_TYPE_P(zv) != IS_OBJECT) {
		// 返回否
		return 0;
	}

	// 取得执行时 中的域
	zend_class_entry *called_scope = zend_get_called_scope(EG(current_execute_data));
	// 如果不在类中
	if (!called_scope) {
		// 返回否
		return 0;
	}
	// 检验继承或接口实现，单向
	return instanceof_function(Z_OBJCE_P(zv), called_scope);
}

/* The cache_slot may only be NULL in debug builds, where arginfo verification of
 * internal functions is enabled. Avoid unnecessary checks in release builds. */
//
#if ZEND_DEBUG
# define HAVE_CACHE_SLOT (cache_slot != NULL)
// 正式环境为 1
#else
# define HAVE_CACHE_SLOT 1
#endif

// ing4， 正式环境 cache_slot++;
#define PROGRESS_CACHE_SLOT() if (HAVE_CACHE_SLOT) {cache_slot++;}

// ing3, 先在 p1 中获取类，如果失败尝试通过 p2 查找，并把找到的类放入p1缓存位置。p1:缓存位置，p2:类型
static zend_always_inline zend_class_entry *zend_fetch_ce_from_cache_slot(
		void **cache_slot, zend_type *type)
{
	// 如果： 1 && cache_slot 目标有效
	if (EXPECTED(HAVE_CACHE_SLOT && *cache_slot)) {
		// 直接把 *cache_slot 转成类入口返回
		return (zend_class_entry *) *cache_slot;
	}

	// *cache_slot 无效
	// 取得类型名
	zend_string *name = ZEND_TYPE_NAME(*type);
	//
	zend_class_entry *ce;
	// 如果。查检是否是ce缓存位置：对象的type_info中指定位置有 IS_STR_CLASS_NAME_MAP_PTR 标记
	if (ZSTR_HAS_CE_CACHE(name)) {
		// 在 CG(map_ptr_base) 通过偏移量（p1的引用计数）取回 类入口指针，带校验
		ce = ZSTR_GET_CE_CACHE(name);
		// 如果没找到类
		if (!ce) {
			// 查找类。p1:类名，p2:key，p3:flags
			ce = zend_lookup_class_ex(name, NULL, ZEND_FETCH_CLASS_NO_AUTOLOAD);
			// 如果没找到类
			if (UNEXPECTED(!ce)) {
				// 无法解决
				/* Cannot resolve */
				// 返回空
				return NULL;
			}
		}
	// 如果 name 不是 ce缓存位置
	} else {
		// 查找类 ( 支持使用 class_name 的类型 | 不要自动加载 | 静默获取类）
		ce = zend_fetch_class(name,
			ZEND_FETCH_CLASS_AUTO | ZEND_FETCH_CLASS_NO_AUTOLOAD | ZEND_FETCH_CLASS_SILENT);
		// 如果没找到类
		if (UNEXPECTED(!ce)) {
			// 返回空
			return NULL;
		}
	}
	// 正式环境为 1
	if (HAVE_CACHE_SLOT) {
		// 把类指针添加到 缓存位置
		*cache_slot = (void *) ce;
	}
	// 返回类
	return ce;
}

// ing3, 验证交叉类型，p1里的所有类型都必须 是 p2的父类。p3: 返回最后一个 cache_slot
static bool zend_check_intersection_type_from_cache_slot(zend_type_list *intersection_type_list,
	zend_class_entry *arg_ce, void ***cache_slot_ptr)
{
	// 解开一层还有2层
	void **cache_slot = *cache_slot_ptr;
	zend_class_entry *ce;
	zend_type *list_type;
	// 
	bool status = true;
	// 遍历交叉类型
	ZEND_TYPE_LIST_FOREACH(intersection_type_list, list_type) {
		// 只有类型有效时才检查类
		/* Only check classes if the type might be valid */
		// 如果有效
		if (status) {
			// 先在 p1 中获取类，如果失败尝试通过 p2 查找，并把找到的类放入p1缓存位置。p1:缓存位置，p2:类型
			ce = zend_fetch_ce_from_cache_slot(cache_slot, list_type);
			/* If type is not an instance of one of the types taking part in the
			 * intersection it cannot be a valid instance of the whole intersection type. */
			// 只要有一个类不存在 或者不是 arg_ce的父类
			if (!ce || !instanceof_function(arg_ce, ce)) {
				// 状态：无效
				status = false;
			}
		}
		// 就算状态无效也一直循环完
		
		// 正式环境 cache_slot++;
		PROGRESS_CACHE_SLOT();	
	} ZEND_TYPE_LIST_FOREACH_END();
	// 正式环境为 1
	if (HAVE_CACHE_SLOT) {
		// 复制缓存位置
		*cache_slot_ptr = cache_slot;
	}
	// 返回状态
	return status;
}

// ing3, 检验参数类型是否匹配，p1:类型，p2:参数，p3:引用实例,参数是引用类型的时候才有，
// p4:缓存位置，p5:是否是返回类型，p6:是否是内置函数
static zend_always_inline bool zend_check_type_slow(
		zend_type *type, zval *arg, zend_reference *ref, void **cache_slot,
		bool is_return_type, bool is_internal)
{
	// 类型 mack
	uint32_t type_mask;
	// 情况1：检验是否是复杂类型, 复杂类型是指：传入的是（一个列表加一个(union 或 intersection)） 或者 （类名）
	// 如果type是复杂类型，并且arg是对象
	if (ZEND_TYPE_IS_COMPLEX(*type) && EXPECTED(Z_TYPE_P(arg) == IS_OBJECT)) {
		// 临时变量
		zend_class_entry *ce;
		// 如果type是类型列表
		if (UNEXPECTED(ZEND_TYPE_HAS_LIST(*type))) {
			// 临时变量
			zend_type *list_type;
			// 如果type是交叉类型
			if (ZEND_TYPE_IS_INTERSECTION(*type)) {
				// 验证交叉类型，p1里的所有类型都必须 是 p2的父类。p3: 返回最后一个 cache_slot
				return zend_check_intersection_type_from_cache_slot(ZEND_TYPE_LIST(*type), Z_OBJCE_P(arg), &cache_slot);
			// 如果是联合类型
			} else {
				// 遍历下属类型
				ZEND_TYPE_LIST_FOREACH(ZEND_TYPE_LIST(*type), list_type) {
					// 如果下属类型是交叉类型
					if (ZEND_TYPE_IS_INTERSECTION(*list_type)) {
						// 验证交叉类型，p1里的所有类型都必须 是 p2的父类。p3: 返回最后一个 cache_slot
						if (zend_check_intersection_type_from_cache_slot(ZEND_TYPE_LIST(*list_type), Z_OBJCE_P(arg), &cache_slot)) {
							// 返回 是
							return true;
						}
						// 这个 cache_slot 在 zend_check_intersection_type_from_cache_slot 中处理过了
						/* The cache_slot is progressed in zend_check_intersection_type_from_cache_slot() */
					// 下属类型不是交叉类型
					} else {
						ZEND_ASSERT(!ZEND_TYPE_HAS_LIST(*list_type));
						// 先在 p1 中获取类，如果失败尝试通过 p2 查找，并把找到的类放入p1缓存位置。p1:缓存位置，p2:类型
						ce = zend_fetch_ce_from_cache_slot(cache_slot, list_type);
						/* Instance of a single type part of a union is sufficient to pass the type check */
						// 检验继承或接口实现，单向
						if (ce && instanceof_function(Z_OBJCE_P(arg), ce)) {
							// 返回 是
							return true;
						}
						// 正式环境 cache_slot++;
						PROGRESS_CACHE_SLOT();
					}
				} ZEND_TYPE_LIST_FOREACH_END();
			}
		// 如果type是单个类型
		} else {
			// 先在 p1 中获取类，如果失败尝试通过 p2 查找，并把找到的类放入p1缓存位置。p1:缓存位置，p2:类型
			ce = zend_fetch_ce_from_cache_slot(cache_slot, type);
			
			// 如果有类入口，检验它是否满足类型强制，否则会检验 标准类型是否满足它
			/* If we have a CE we check if it satisfies the type constraint,
			 * otherwise it will check if a standard type satisfies it. */
			// 检验继承或接口实现，单向
			if (ce && instanceof_function(Z_OBJCE_P(arg), ce)) {
				// 返回 是
				return true;
			}
		}
	}
	// 取得 type可能的类型
	type_mask = ZEND_TYPE_FULL_MASK(*type);
	// 情况2：如果包含 可调用对象
	if ((type_mask & MAY_BE_CALLABLE) &&
		// 并且： 检查此调用在 当前指定执行数据中 是否可行。 p1:调用信息，p2:检验级别，p3:返回调用字串
		zend_is_callable(arg, is_internal ? IS_CALLABLE_SUPPRESS_DEPRECATIONS : 0, NULL)) {
		// 返回 是
		return 1;
	}
	// 情况3：如果包含 static,并且 arg是当前类的下属类
	if ((type_mask & MAY_BE_STATIC) && zend_value_instanceof_static(arg)) {
		// 返回：成功
		return 1;
	}
	
	// 情况4：检验引用目标是否有效 ((p1)->sources).ptr != NULL
	if (ref && ZEND_REF_HAS_TYPE_SOURCES(ref)) {
		// 不能有 带类型的引用的转换
		/* We cannot have conversions for typed refs. */
		// 返回否
		return 0;
	}
	// 情况5：内部 并且 有返回类型
	if (is_internal && is_return_type) {
		// 对于内部返回，类型必须准确匹配，因为我们不会再检测它，所以没有机会使用强制值
		/* For internal returns, the type has to match exactly, because we're not
		 * going to check it for non-debug builds, and there will be no chance to
		 * apply coercions. */
		// 返回 否
		return 0;
	}
	// 检查 参数 和 类型是否匹配，p1:类型，p2:参数，p3:是否严格，p4:是否内置参数
	return zend_verify_scalar_type_hint(type_mask, arg,
		// is_return_type ？
		// 当前执行上下文函数 的参数是否使用严格类型限制 ：
		// 前一个执行数据所属函数 的参数使用严格类型限制
		is_return_type ? ZEND_RET_USES_STRICT_TYPES() : ZEND_ARG_USES_STRICT_TYPES(),
		is_internal);

	// 不需要再对 IS_VOID 进行特殊处理了（为返回类型），因为这种情况已经在编译时处理过了。
	/* Special handling for IS_VOID is not necessary (for return types),
	 * because this case is already checked at compile-time. */
}

// ing3, 检验参数类型是否匹配，p1:类型，p2:参数，p3:缓存位置，p4:类入口（没用到），p5:是否是返回类型，p6:是否是内置函数
static zend_always_inline bool zend_check_type(
		zend_type *type, zval *arg, void **cache_slot, zend_class_entry *scope,
		bool is_return_type, bool is_internal)
{
	zend_reference *ref = NULL;
	// 类型必须存在
	ZEND_ASSERT(ZEND_TYPE_IS_SET(*type));

	// 如果参数是引用类型
	if (UNEXPECTED(Z_ISREF_P(arg))) {
		// 获得引用实例
		ref = Z_REF_P(arg);
		// 解引用
		arg = Z_REFVAL_P(arg);
	}

	// 如果传入的类型包含此 参数的类型
	if (EXPECTED(ZEND_TYPE_CONTAINS_CODE(*type, Z_TYPE_P(arg)))) {
		// 返回成功
		return 1;
	}

	// ing3, 检验参数类型是否匹配，p1:类型，p2:参数，p3:引用实例,参数是引用类型的时候才有
	// p4:缓存位置，p5:是否是返回类型，p6:是否是内置函数
	return zend_check_type_slow(type, arg, ref, cache_slot, is_return_type, is_internal);
}

// ing3, 非内置函数，检验参数类型是否匹配，p1:类型，p2:参数，p3:引用实例,参数是引用类型的时候才有
// p4:缓存位置，p5:是否是返回类型
ZEND_API bool zend_check_user_type_slow(
		zend_type *type, zval *arg, zend_reference *ref, void **cache_slot, bool is_return_type)
{
	// ing3, 检验参数类型是否匹配，p1:类型，p2:参数，p3:引用实例,参数是引用类型的时候才有，
	// p4:缓存位置，p5:是否是返回类型，p6:是否是内置函数
	return zend_check_type_slow(
		type, arg, ref, cache_slot, is_return_type, /* is_internal */ false);
}

// ing3, 检验参数 的类型是否匹配，p1:函数，p2:参数序号，p3:参数，p4:缓存位置
static zend_always_inline bool zend_verify_recv_arg_type(zend_function *zf, uint32_t arg_num, zval *arg, void **cache_slot)
{
	zend_arg_info *cur_arg_info;

	ZEND_ASSERT(arg_num <= zf->common.num_args);
	// 取得参数信息
	cur_arg_info = &zf->common.arg_info[arg_num-1];

	// 类型存在 并且
	// 检验参数类型是否匹配，p1:类型，p2:参数，p3:缓存位置，p4:类入口（没用到），p5:是否是返回类型，p6:是否是内置函数
	if (ZEND_TYPE_IS_SET(cur_arg_info->type)
			&& UNEXPECTED(!zend_check_type(&cur_arg_info->type, arg, cache_slot, zf->common.scope, 0, 0))) {
		// 报错：参数验证错误
		zend_verify_arg_error(zf, cur_arg_info, arg_num, arg);
		// 返回否
		return 0;
	}

	// 返回是
	return 1;
}

// ing3, 检验字典参数类型，p1:函数，p2:参数信息，p3:参数序号，p4:参数，p5:缓存位置
static zend_always_inline bool zend_verify_variadic_arg_type(
		zend_function *zf, zend_arg_info *arg_info, uint32_t arg_num, zval *arg, void **cache_slot)
{
	// 类型必须存在
	ZEND_ASSERT(ZEND_TYPE_IS_SET(arg_info->type));
	// 检验参数类型是否匹配，p1:类型，p2:参数，p3:缓存位置，p4:类入口（没用到），p5:是否是返回类型，p6:是否是内置函数
	if (UNEXPECTED(!zend_check_type(&arg_info->type, arg, cache_slot, zf->common.scope, 0, 0))) {
		// 报错：参数验证错误
		zend_verify_arg_error(zf, arg_info, arg_num, arg);
		// 返回否
		return 0;
	}

	// 返回是
	return 1;
}

// ing3，检验内置参数类型，p1:函数，p2:执行数据
static zend_never_inline ZEND_ATTRIBUTE_UNUSED bool zend_verify_internal_arg_types(zend_function *fbc, zend_execute_data *call)
{
	uint32_t i;
	// 参数数量
	uint32_t num_args = ZEND_CALL_NUM_ARGS(call);
	// 取得列表中第 （ZEND_CALL_FRAME_SLOT +） n 个 zval
	zval *arg = ZEND_CALL_ARG(call, 1);

	// 遍历所有参数
	for (i = 0; i < num_args; ++i) {
		// 临时变量
		zend_arg_info *cur_arg_info;
		// 如果是普通参数
		if (EXPECTED(i < fbc->common.num_args)) {
			// 取得此 参数的信息
			cur_arg_info = &fbc->common.arg_info[i];
		// 字典参数
		} else if (UNEXPECTED(fbc->common.fn_flags & ZEND_ACC_VARIADIC)) {
			// 取得最后一个 参数信息
			cur_arg_info = &fbc->common.arg_info[fbc->common.num_args];
		// 其他情况
		} else {
			// 中断遍历
			break;
		}

		// 如果参数信息中的类型有效 并且 
		// 检验参数类型是否匹配，p1:类型，p2:参数，p3:缓存位置，p4:类入口（没用到），p5:是否是返回类型，p6:是否是内置函数
		if (ZEND_TYPE_IS_SET(cur_arg_info->type)
				&& UNEXPECTED(!zend_check_type(&cur_arg_info->type, arg, /* cache_slot */ NULL, fbc->common.scope, 0, /* is_internal */ 1))) {
			return 0;
		}
		// 下一个参数
		arg++;
	}
	return 1;
}

// 调试用
#if ZEND_DEBUG
/* Determine whether an internal call should throw, because the passed arguments violate
 * an arginfo constraint. This is only checked in debug builds. In release builds, we
 * trust that arginfo matches what is enforced by zend_parse_parameters. */
// suspend, 
ZEND_API bool zend_internal_call_should_throw(zend_function *fbc, zend_execute_data *call)
{
	if (fbc->internal_function.handler == ZEND_FN(pass) || (fbc->internal_function.fn_flags & ZEND_ACC_FAKE_CLOSURE)) {
		/* Be lenient about the special pass function and about fake closures. */
		return 0;
	}

	if (fbc->common.required_num_args > ZEND_CALL_NUM_ARGS(call)) {
		/* Required argument not passed. */
		return 1;
	}

	if (fbc->common.num_args < ZEND_CALL_NUM_ARGS(call)
			&& !(fbc->common.fn_flags & ZEND_ACC_VARIADIC)) {
		/* Too many arguments passed. For internal functions (unlike userland functions),
		 * this should always throw. */
		return 1;
	}

	if ((fbc->common.fn_flags & ZEND_ACC_HAS_TYPE_HINTS) &&
			!zend_verify_internal_arg_types(fbc, call)) {
		return 1;
	}

	return 0;
}

// suspend, 
ZEND_API ZEND_COLD void zend_internal_call_arginfo_violation(zend_function *fbc)
{
	zend_error(E_ERROR, "Arginfo / zpp mismatch during call of %s%s%s()",
		fbc->common.scope ? ZSTR_VAL(fbc->common.scope->name) : "",
		fbc->common.scope ? "::" : "",
		ZSTR_VAL(fbc->common.function_name));
}

#ifndef ZEND_VERIFY_FUNC_INFO
# define ZEND_VERIFY_FUNC_INFO 0
#endif

// suspend, 
static void zend_verify_internal_func_info(zend_function *fn, zval *retval) {
#if ZEND_VERIFY_FUNC_INFO
	zend_string *name = fn->common.function_name;
	uint32_t type_mask = zend_get_internal_func_info(fn, NULL, NULL);
	if (!type_mask) {
		return;
	}

	/* Always check refcount of arrays, as immutable arrays are RCN. */
	if (Z_REFCOUNTED_P(retval) || Z_TYPE_P(retval) == IS_ARRAY) {
		if (!(type_mask & MAY_BE_RC1)) {
			zend_error_noreturn(E_CORE_ERROR, "%s() missing rc1", ZSTR_VAL(name));
		}
		if (Z_REFCOUNT_P(retval) > 1 && !(type_mask & MAY_BE_RCN)) {
			zend_error_noreturn(E_CORE_ERROR, "%s() missing rcn", ZSTR_VAL(name));
		}
	}

	uint32_t type = 1u << Z_TYPE_P(retval);
	if (!(type_mask & type)) {
		zend_error_noreturn(E_CORE_ERROR, "%s() missing type %s",
			ZSTR_VAL(name), zend_get_type_by_const(Z_TYPE_P(retval)));
	}

	if (Z_TYPE_P(retval) == IS_ARRAY) {
		HashTable *ht = Z_ARRVAL_P(retval);
		uint32_t num_checked = 0;
		zend_string *str;
		zval *val;
		ZEND_HASH_FOREACH_STR_KEY_VAL(ht, str, val) {
			if (str) {
				if (!(type_mask & MAY_BE_ARRAY_KEY_STRING)) {
					zend_error_noreturn(E_CORE_ERROR,
						"%s() missing array_key_string", ZSTR_VAL(name));
				}
			} else {
				if (!(type_mask & MAY_BE_ARRAY_KEY_LONG)) {
					zend_error_noreturn(E_CORE_ERROR,
						"%s() missing array_key_long", ZSTR_VAL(name));
				}
			}

			uint32_t array_type = 1u << (Z_TYPE_P(val) + MAY_BE_ARRAY_SHIFT);
			if (!(type_mask & array_type)) {
				zend_error_noreturn(E_CORE_ERROR,
					"%s() missing array element type %s",
					ZSTR_VAL(name), zend_get_type_by_const(Z_TYPE_P(retval)));
			}

			/* Don't check all elements of large arrays. */
			if (++num_checked > 16) {
				break;
			}
		} ZEND_HASH_FOREACH_END();
	}
#endif
}
#endif

//ing3, 报错：传入参数太少
ZEND_API ZEND_COLD void ZEND_FASTCALL zend_missing_arg_error(zend_execute_data *execute_data)
{
	// 前一个执行数据
	zend_execute_data *ptr = EX(prev_execute_data);
	// 如果是用户函数
	if (ptr && ptr->func && ZEND_USER_CODE(ptr->func->common.type)) {
		// 报错：传入参数太少
		zend_throw_error(zend_ce_argument_count_error, "Too few arguments to function %s%s%s(), %d passed in %s on line %d and %s %d expected",
			EX(func)->common.scope ? ZSTR_VAL(EX(func)->common.scope->name) : "",
			EX(func)->common.scope ? "::" : "",
			ZSTR_VAL(EX(func)->common.function_name),
			EX_NUM_ARGS(),
			ZSTR_VAL(ptr->func->op_array.filename),
			ptr->opline->lineno,
			EX(func)->common.required_num_args == EX(func)->common.num_args ? "exactly" : "at least",
			EX(func)->common.required_num_args);
	// 如果是内置函数
	} else {
		zend_throw_error(zend_ce_argument_count_error, "Too few arguments to function %s%s%s(), %d passed and %s %d expected",
			EX(func)->common.scope ? ZSTR_VAL(EX(func)->common.scope->name) : "",
			EX(func)->common.scope ? "::" : "",
			ZSTR_VAL(EX(func)->common.function_name),
			EX_NUM_ARGS(),
			EX(func)->common.required_num_args == EX(func)->common.num_args ? "exactly" : "at least",
			EX(func)->common.required_num_args);
	}
}

// ing3, 报错：返回类型不符。p1:函数，p2:返回值
ZEND_API ZEND_COLD void zend_verify_return_error(const zend_function *zf, zval *value)
{
	const zend_arg_info *arg_info = &zf->common.arg_info[-1];
	const char *fname, *fsep, *fclass;
	zend_string *need_msg;
	const char *given_msg;
	// 根据传入的值返回类型：p1:函数，p2:参数信息, p3:值。以下返回，p4:函数名，p5:分隔符，p6:类名，p7:消息，p8:传入值类型
	zend_verify_type_error_common(
		zf, arg_info, value, &fname, &fsep, &fclass, &need_msg, &given_msg);

	zend_type_error("%s%s%s(): Return value must be of type %s, %s returned",
		fclass, fsep, fname, ZSTR_VAL(need_msg), given_msg);

	zend_string_release(need_msg);
}

// ing3, 返回类型不符合 :never 时的报错
ZEND_API ZEND_COLD void zend_verify_never_error(const zend_function *zf)
{
	// 函数名
	zend_string *func_name = get_function_or_method_name(zf);
	// 禁止返回的函数 ，不可以有隐式的返回
	zend_type_error("%s(): never-returning function must not implicitly return",
		ZSTR_VAL(func_name));
	// 释放临时函数名
	zend_string_release(func_name);
}

// 调试用
#if ZEND_DEBUG
// suspend
static ZEND_COLD void zend_verify_internal_return_error(const zend_function *zf, zval *value)
{
	const zend_arg_info *arg_info = &zf->common.arg_info[-1];
	const char *fname, *fsep, *fclass;
	zend_string *need_msg;
	const char *given_msg;
	// 根据传入的值返回类型：p1:函数，p2:参数信息, p3:值。以下返回，p4:函数名，p5:分隔符，p6:类名，p7:消息，p8:传入值类型
	zend_verify_type_error_common(
		zf, arg_info, value, &fname, &fsep, &fclass, &need_msg, &given_msg);

	zend_error_noreturn(E_CORE_ERROR, "%s%s%s(): Return value must be of type %s, %s returned",
		fclass, fsep, fname, ZSTR_VAL(need_msg), given_msg);
}
// suspend
static ZEND_COLD void zend_verify_void_return_error(const zend_function *zf, const char *returned_msg, const char *returned_kind)
{
	const char *fname = ZSTR_VAL(zf->common.function_name);
	const char *fsep;
	const char *fclass;

	if (zf->common.scope) {
		fsep =  "::";
		fclass = ZSTR_VAL(zf->common.scope->name);
	} else {
		fsep =  "";
		fclass = "";
	}

	zend_type_error("%s%s%s() must not return a value, %s%s returned",
		fclass, fsep, fname, returned_msg, returned_kind);
}
// suspend
ZEND_API bool zend_verify_internal_return_type(zend_function *zf, zval *ret)
{
	zend_internal_arg_info *ret_info = zf->internal_function.arg_info - 1;

	if (ZEND_TYPE_FULL_MASK(ret_info->type) & MAY_BE_VOID) {
		if (UNEXPECTED(Z_TYPE_P(ret) != IS_NULL)) {
			zend_verify_void_return_error(zf, zend_zval_type_name(ret), "");
			return 0;
		}
		return 1;
	}

	// 检验参数类型是否匹配，p1:类型，p2:参数，p3:缓存位置，p4:类入口（没用到），p5:是否是返回类型，p6:是否是内置函数
	if (UNEXPECTED(!zend_check_type(&ret_info->type, ret, /* cache_slot */ NULL, NULL, 1, /* is_internal */ 1))) {
		zend_verify_internal_return_error(zf, ret);
		return 0;
	}

	return 1;
}
#endif

// ing3, 报错：返回类型不符
static ZEND_COLD void zend_verify_missing_return_type(const zend_function *zf)
{
	// VERIFY_RETURN_TYPE 不会来自 void 类型的函数，所以它一定是错误
	/* VERIFY_RETURN_TYPE is not emitted for "void" functions, so this is always an error. */
	// 报错：返回类型不符
	zend_verify_return_error(zf, NULL);
}

// ing4, 报错：不可以把对象当数组用
static zend_never_inline ZEND_COLD void ZEND_FASTCALL zend_use_object_as_array(void)
{
	zend_throw_error(NULL, "Cannot use object as array");
}

// ing4, 报错：非法的偏移量类型
static zend_never_inline ZEND_COLD void ZEND_FASTCALL zend_illegal_offset(void)
{
	zend_type_error("Illegal offset type");
}

// ing4, 报错：不可以用此类型作为 string的偏移量
static zend_never_inline ZEND_COLD void ZEND_FASTCALL zend_illegal_string_offset(const zval *offset)
{
	zend_type_error("Cannot access offset of type %s on string", zend_zval_type_name(offset));
}

// ing3, 像数组一样给对象赋值
static zend_never_inline void zend_assign_to_object_dim(zend_object *obj, zval *dim, zval *value OPLINE_DC EXECUTE_DATA_DC)
{
	// zend_std_write_dimension
	obj->handlers->write_dimension(obj, dim, value);
	// ing4, 检验：opline 返回类型不是IS_UNUSED
	if (UNEXPECTED(RETURN_VALUE_USED(opline))) {
		// 把 值 复制给 opline 结果
		ZVAL_COPY(EX_VAR(opline->result.var), value);
	}
}

// ing4, 调用操作码（在opline参数里）对应的二进制处理方法，p1:返回值，p2:op1，p3:op2
// 加/减/乘/除/左右位移/连接/或/与/抑或/幂 运算
static zend_always_inline int zend_binary_op(zval *ret, zval *op1, zval *op2 OPLINE_DC)
{
	// 二进制操作方法. 加/减/乘/除/左右位移/连接/或/与/抑或/幂 运算
	static const binary_op_type zend_binary_ops[] = {
		add_function,
		sub_function,
		mul_function,
		div_function,
		mod_function,
		shift_left_function,
		shift_right_function,
		concat_function,
		bitwise_or_function,
		bitwise_and_function,
		bitwise_xor_function,
		pow_function
	};
	// size_t 类型让GCC更好地优化 64位 PIC 码
	/* size_t cast makes GCC to better optimize 64-bit PIC code */
	size_t opcode = (size_t)opline->extended_value;
	// 调用操作码对应的方法
	return zend_binary_ops[opcode - ZEND_ADD](ret, op1, op2);
}

// ing3, 像数组一样给属性赋值
static zend_never_inline void zend_binary_assign_op_obj_dim(zend_object *obj, zval *property OPLINE_DC EXECUTE_DATA_DC)
{
	zval *value;
	zval *z;
	zval rv, res;
	// 增加引用次数
	GC_ADDREF(obj);
	// 如果有属性值 并且属性值有效
	if (property && UNEXPECTED(Z_ISUNDEF_P(property))) {
		// _zval_undefined_op2, 检查op2的变量，没有定义则抛异常
		property = ZVAL_UNDEFINED_OP2();
	}
	// ing4, 在执行数据中通过 p1:运算对象类型，p3:操作类型，取回p2:运算对象指向的zval
	//（常量取下一行操作码）（编译变量未定义报错）
	value = get_op_data_zval_ptr_r((opline+1)->op1_type, (opline+1)->op1);
	
	// zend_std_read_dimension，像数组一样读取 对象属性, 如果获取到的值有效
	if ((z = obj->handlers->read_dimension(obj, property, BP_VAR_R, &rv)) != NULL) {
		// 调用操作码（在opline参数里）对应的二进制处理方法，p1:返回值，p2:op1，p3:op2
		if (zend_binary_op(&res, z, value OPLINE_CC) == SUCCESS) {
			// 如果成功，
			// zend_std_write_dimension ,像数组一样写入 对象属性
			obj->handlers->write_dimension(obj, property, &res);
		}
		// 如果 读取到的的就是rv本身
		if (z == &rv) {
			// 销毁没有引用次数的对象，有引用次数的添加到gc回收周期里。并没有销毁 zval本身。
			zval_ptr_dtor(&rv);
		}
		// ing4, 检验：opline 返回类型不是IS_UNUSED
		if (UNEXPECTED(RETURN_VALUE_USED(opline))) {
			// 把结果复制给 指定编号的 zval
			// #define EX_VAR(n) ZEND_CALL_VAR(execute_data, n)
			// #define ZEND_CALL_VAR(call, n) ((zval*)(((char*)(call)) + ((int)(n))))
			ZVAL_COPY(EX_VAR(opline->result.var), &res);
		}
		// 销毁没有引用次数的对象，有引用次数的添加到gc回收周期里。并没有销毁 zval本身。
		zval_ptr_dtor(&res);
	} else {
		// 报错：不可以把对象当数组用
		zend_use_object_as_array();
		// ing4, 检验：opline 返回类型不是IS_UNUSED
		if (UNEXPECTED(RETURN_VALUE_USED(opline))) {
			// 把 指定编号的 zval 值更新为 null
			ZVAL_NULL(EX_VAR(opline->result.var));
		}
	}
	// 释放（变量 和 临时变量 类型的）运算对象（zval）：p1:类型，p2:编号
	FREE_OP((opline+1)->op1_type, (opline+1)->op1.var);
	// 如果obj 引用-1后为0
	if (UNEXPECTED(GC_DELREF(obj) == 0)) {
		// 释放指定的对象，并更新 objects_store 中的指针
		zend_objects_store_del(obj);
	}
}

// ing3, 根据操作码类型 对引用目标 进行 二进制 赋值操作，p1:引用实例，p2:第二个运算值
// 加/减/乘/除/左右位移/连接/或/与/抑或/幂 运算
static zend_never_inline void zend_binary_assign_op_typed_ref(zend_reference *ref, zval *value OPLINE_DC EXECUTE_DATA_DC)
{
	zval z_copy;

	// 如果引用目标是字串 ，确保使用 进程内的连接
	/* Make sure that in-place concatenation is used if the LHS is a string. */
	// 如果扩展操作码是 ZEND_CONCAT 并且 引用目标为 字串
	if (opline->extended_value == ZEND_CONCAT && Z_TYPE(ref->val) == IS_STRING) {
		// 接连引用目标和给出的字串 
		concat_function(&ref->val, &ref->val, value);
		// 结果必须还是 字串
		ZEND_ASSERT(Z_TYPE(ref->val) == IS_STRING && "Concat should return string");
		// 完毕
		return;
	}
	
	// 调用操作码（在opline参数里）对应的二进制处理方法，p1:返回值，p2:op1，p3:op2
	zend_binary_op(&z_copy, &ref->val, value OPLINE_CC);
	// 检查值是否适配属性信息列表的每一条信息，有一条不适配就返回失败。 p1:属性信息列表的引用,p2:值
	// 过程中可能用 zend_verify_weak_scalar_type_hint 强制转换p2
		// 某执行上下文所属函数 的参数是否使用严格类型限制
	if (EXPECTED(zend_verify_ref_assignable_zval(ref, &z_copy, EX_USES_STRICT_TYPES()))) {
		// 销毁没有引用次数的对象，有引用次数的添加到gc回收周期里。并没有销毁 zval本身。
		zval_ptr_dtor(&ref->val);
		// 把返回值复制到 引用目标
		ZVAL_COPY_VALUE(&ref->val, &z_copy);
	} else {
		// 销毁没有引用次数的对象，有引用次数的添加到gc回收周期里。并没有销毁 zval本身。
		zval_ptr_dtor(&z_copy);
	}
}

// ing3, 根据属性信息的类型 对运算对象 进行 二进制 赋值操作，p1:属性信息，p2:第一个运算值，p3:第二个运算值
static zend_never_inline void zend_binary_assign_op_typed_prop(zend_property_info *prop_info, zval *zptr, zval *value OPLINE_DC EXECUTE_DATA_DC)
{
	zval z_copy;

	// 如果引用目标是字串 ，确保使用 进程内的连接
	/* Make sure that in-place concatenation is used if the LHS is a string. */
	if (opline->extended_value == ZEND_CONCAT && Z_TYPE_P(zptr) == IS_STRING) {
		// 接连 zptr 和给出的字串 
		concat_function(zptr, zptr, value);
		// 结果必须还是 字串
		ZEND_ASSERT(Z_TYPE_P(zptr) == IS_STRING && "Concat should return string");
		// 完毕
		return;
	}
	// 调用操作码（在 opline 参数里）对应的二进制处理方法，p1:返回值，p2:op1，p3:op2
	zend_binary_op(&z_copy, zptr, value OPLINE_CC);
	// 检查属性值类型 与 属性信息要求的类型是否匹配
	if (EXPECTED(zend_verify_property_type(prop_info, &z_copy, EX_USES_STRICT_TYPES()))) {
		// 销毁没有引用次数的对象，有引用次数的添加到gc回收周期里。并没有销毁 zval本身。
		zval_ptr_dtor(zptr);
		// 把返回值 复制到 zptr
		ZVAL_COPY_VALUE(zptr, &z_copy);
	} else {
		// 销毁没有引用次数的对象，有引用次数的添加到gc回收周期里。并没有销毁 zval本身。
		zval_ptr_dtor(&z_copy);
	}
}

// ing3, 像数组一样操作字串，把索引号转成整数
static zend_never_inline zend_long zend_check_string_offset(zval *dim, int type EXECUTE_DATA_DC)
{
	zend_long offset;

try_again:
	// 根据索引类型操作
	switch(Z_TYPE_P(dim)) {
		// 整数
		case IS_LONG:
			// 取出并返回整数 
			return Z_LVAL_P(dim);
		// 字串
		case IS_STRING:
		{
			// 没有尾数据
			bool trailing_data = false;
			/* For BC reasons we allow errors so that we can warn on leading numeric string */
			// 转成数值
			if (IS_LONG == is_numeric_string_ex(Z_STRVAL_P(dim), Z_STRLEN_P(dim), &offset, NULL,
					/* allow errors */ true, NULL, &trailing_data)) {
				// 如果不是删除操作
				if (UNEXPECTED(trailing_data) && type != BP_VAR_UNSET) {
					// 报错：非法的字串偏移量
					zend_error(E_WARNING, "Illegal string offset \"%s\"", Z_STRVAL_P(dim));
				}
				// 转换成整数成功，返回偏移量
				return offset;
			}
			// 报错：不可以用此类型作为 string的偏移量
			zend_illegal_string_offset(dim);
			return 0;
		}
		// 未定义
		case IS_UNDEF:
			// _zval_undefined_op2, 检查op2的变量，没有定义则抛异常
			ZVAL_UNDEFINED_OP2();
			// ((void)0)
			ZEND_FALLTHROUGH;
		// 小数，null, false, true
		case IS_DOUBLE:
		case IS_NULL:
		case IS_FALSE:
		case IS_TRUE:
			// 以上5种情况都报错：索引号无效
			zend_error(E_WARNING, "String offset cast occurred");
			break;
		// 索引类型
		case IS_REFERENCE:
			// 解引用
			dim = Z_REFVAL_P(dim);
			// 再试一次
			goto try_again;
		// 其他情况
		default:
			// 报错：不可以用此类型作为 string的偏移量
			zend_illegal_string_offset(dim);
			return 0;
	}
	// 索引号转成整数
	return zval_get_long_func(dim, /* is_strict */ false);
}

// ing3, 报错：字串偏移量用法错误
ZEND_API ZEND_COLD void zend_wrong_string_offset_error(void)
{
	const char *msg = NULL;
	// 当前操作数据
	const zend_execute_data *execute_data = EG(current_execute_data);
	// 操作码
	const zend_op *opline = execute_data->opline;
	// 如果没有异常
	if (UNEXPECTED(EG(exception) != NULL)) {
		// 中断
		return;
	}

	// 根据操作码 组织错误信息
	switch (opline->opcode) {
		// 数组赋值
		case ZEND_ASSIGN_DIM_OP:
			// 错误信息：不能对字串偏移位置 赋值（测试过）
			// $s = "012345"; $s[2] .="999";
			msg = "Cannot use assign-op operators with string offsets";
			break;
		// 无法创建对字串偏移位置 的引用
		case ZEND_FETCH_LIST_W:
			msg = "Cannot create references to/from string offsets";
			break;
		// 如果是像数组一样：写入/更新/获取函数参数/删除
		case ZEND_FETCH_DIM_W:
		case ZEND_FETCH_DIM_RW:
		case ZEND_FETCH_DIM_FUNC_ARG:
		case ZEND_FETCH_DIM_UNSET:
			// 扩展操作标记
			switch (opline->extended_value) {
				case ZEND_FETCH_DIM_REF:
					// 无法创建对字串偏移位置 的引用
					msg = "Cannot create references to/from string offsets";
					break;
				case ZEND_FETCH_DIM_DIM:
					// 无法把字串偏移位置 当成数组使用（测试过）
					// $s = "012345"; $s[2][1]="999";
					msg = "Cannot use string offset as an array";
					break;
				case ZEND_FETCH_DIM_OBJ:
					// 无法把字串偏移位置 当成对象使用
					msg = "Cannot use string offset as an object";
					break;
				case ZEND_FETCH_DIM_INCDEC:
					// 无法对字串偏移位置做自增/自减操作（测试过）
					// $s = "012345"; $s[2]++;
					msg = "Cannot increment/decrement string offsets";
					break;
				EMPTY_SWITCH_DEFAULT_CASE();
			}
			break;
		EMPTY_SWITCH_DEFAULT_CASE();
	}
	// 错误信息不可以为空
	ZEND_ASSERT(msg != NULL);
	// 报错
	zend_throw_error(NULL, "%s", msg);
}

// ing4, 报错：函数或方法已弃用
ZEND_API ZEND_COLD void ZEND_FASTCALL zend_deprecated_function(const zend_function *fbc)
{
	// 如果有所属类
	if (fbc->common.scope) {
		// 报错：方法弃用
		zend_error(E_DEPRECATED, "Method %s::%s() is deprecated",
			ZSTR_VAL(fbc->common.scope->name),
			ZSTR_VAL(fbc->common.function_name)
		);
	} else {
		// 报错：函数弃用
		zend_error(E_DEPRECATED, "Function %s() is deprecated", ZSTR_VAL(fbc->common.function_name));
	}
}

// ing4, 报错：把false转成数组已弃用
ZEND_API ZEND_COLD void ZEND_FASTCALL zend_false_to_array_deprecated(void)
{
	// 报错：把false转成数组已弃用
	zend_error(E_DEPRECATED, "Automatic conversion of false to array is deprecated");
}

// ing4, 修改字串中的某1个字符（测试过）
static zend_never_inline void zend_assign_to_string_offset(zval *str, zval *dim, zval *value OPLINE_DC EXECUTE_DATA_DC)
{
	zend_uchar c;
	size_t string_len;
	zend_long offset;
	zend_string *s;
	// 创建副本
	/* separate string */
	// 如果可计数并且引用次数为 1
	if (Z_REFCOUNTED_P(str) && Z_REFCOUNT_P(str) == 1) {
		// 获取字串
		s = Z_STR_P(str);
	// 否则 
	} else {
		// 创建新 字串
		s = zend_string_init(Z_STRVAL_P(str), Z_STRLEN_P(str), 0);
		// 复制哈希值
		ZSTR_H(s) = ZSTR_H(Z_STR_P(str));
		// 如果原值可计数
		if (Z_REFCOUNTED_P(str)) {
			// 原值引用数 -1
			GC_DELREF(Z_STR_P(str));
		}
		// 把zend_string 添加给 zval，不支持保留字
		ZVAL_NEW_STR(str, s);
	}

	// 如果索引号是整数 
	if (EXPECTED(Z_TYPE_P(dim) == IS_LONG)) {
		// 取出整数
		offset = Z_LVAL_P(dim);
	// 索引号不是整数
	} else {
		// 在抛出提示时，数组可能被销毁了，临时增加引用次数来侦测此种情况
		/* The string may be destroyed while throwing the notice.
		 * Temporarily increase the refcount to detect this situation. */
		GC_ADDREF(s);
		// 像数组一样操作字串，把索引号转成整数
		offset = zend_check_string_offset(dim, BP_VAR_W EXECUTE_DATA_CC);
		// 如果 -1后 字串引用数为0
		if (UNEXPECTED(GC_DELREF(s) == 0)) {
			// 释放字串
			zend_string_efree(s);
			// ing4, 检验：opline 返回类型不是IS_UNUSED
			if (UNEXPECTED(RETURN_VALUE_USED(opline))) {
				// 操作码结果赋值成 null
				// #define EX_VAR(n) ZEND_CALL_VAR(execute_data, n)
				// #define ZEND_CALL_VAR(call, n) ((zval*)(((char*)(call)) + ((int)(n))))
				ZVAL_NULL(EX_VAR(opline->result.var));
			}
			// 完成
			return;
		}
		// 非法的偏移量赋值
		/* Illegal offset assignment */
		// 如果有异常
		if (UNEXPECTED(EG(exception) != NULL)) {
			// ing4, 检验：opline 返回类型不是IS_UNUSED
			if (UNEXPECTED(RETURN_VALUE_USED(opline))) {
				// 操作码结果赋值成 未定义
				ZVAL_UNDEF(EX_VAR(opline->result.var));
			}
			// 完成
			return;
		}
	}
	
	// 操作码是数字

	// 偏移位置溢出
	if (UNEXPECTED(offset < -(zend_long)ZSTR_LEN(s))) {
		// 错误的负数 偏移量
		/* Error on negative offset */
		// 报错：偏移量非法
		zend_error(E_WARNING, "Illegal string offset " ZEND_LONG_FMT, offset);
		// ing4, 检验：opline 返回类型不是IS_UNUSED
		if (UNEXPECTED(RETURN_VALUE_USED(opline))) {
			// 操作码结果赋值成 null
			ZVAL_NULL(EX_VAR(opline->result.var));
		}
		// 完成
		return;
	}
	// 如果偏移量为负数，处理负数
	if (offset < 0) { /* Handle negative offset */
		// 转成正数位置
		offset += (zend_long)ZSTR_LEN(s);
	}

	// 如果值 不是字串
	if (UNEXPECTED(Z_TYPE_P(value) != IS_STRING)) {
		zend_string *tmp;
		// 在抛出提示时，数组可能被销毁了，临时增加引用次数来侦测此种情况
		/* The string may be destroyed while throwing the notice.
		 * Temporarily increase the refcount to detect this situation. */
		// 字串增加引用次数
		GC_ADDREF(s);
		// 如果值为 未定义
		if (UNEXPECTED(Z_TYPE_P(value) == IS_UNDEF)) {
			// ing4, 报错：未定义的变量（变量名）, 并返未初始化zval, EXECUTE_DATA_DC 传入也用不到
			zval_undefined_cv((opline+1)->op1.var EXECUTE_DATA_CC);
		}
		// zval 转成 字串, 有try
		/* Convert to string, just the time to pick the 1st byte */
		tmp = zval_try_get_string_func(value);
		// 如果字串引用数-1后为0
		if (UNEXPECTED(GC_DELREF(s) == 0)) {
			// 释放字串
			zend_string_efree(s);
			// 如果有临时变量
			if (tmp) {
				// 释放临时变量
				zend_string_release_ex(tmp, 0);
			}
			// ing4, 检验：opline 返回类型不是IS_UNUSED
			if (UNEXPECTED(RETURN_VALUE_USED(opline))) {
				// 操作码结果赋值成 null
				// #define EX_VAR(n) ZEND_CALL_VAR(execute_data, n)
				// #define ZEND_CALL_VAR(call, n) ((zval*)(((char*)(call)) + ((int)(n))))
				ZVAL_NULL(EX_VAR(opline->result.var));
			}
			// 完成
			return;
		}
		// 如果转成字串后无效
		if (UNEXPECTED(!tmp)) {
			// ing4, 检验：opline 返回类型不是IS_UNUSED
			if (UNEXPECTED(RETURN_VALUE_USED(opline))) {
				// 操作码结果赋值成 未定义
				ZVAL_UNDEF(EX_VAR(opline->result.var));
			}
			// 完成
			return;
		}

		// 字串长度
		string_len = ZSTR_LEN(tmp);
		// 第一个字符
		c = (zend_uchar)ZSTR_VAL(tmp)[0];
		// 释放字串
		zend_string_release_ex(tmp, 0);
	// 值是字串
	} else {
		// 获取长度
		string_len = Z_STRLEN_P(value);
		// 第一个字符
		c = (zend_uchar)Z_STRVAL_P(value)[0];
	}

	// 如果字串长度不是1
	if (UNEXPECTED(string_len != 1)) {
		if (string_len == 0) {
			// 报错：不能把指定位置更新成空字符。（测试过）
			/* Error on empty input string */
			zend_throw_error(NULL, "Cannot assign an empty string to a string offset");
			// ing4, 检验：opline 返回类型不是IS_UNUSED
			if (UNEXPECTED(RETURN_VALUE_USED(opline))) {
				// 操作码结果赋值成 null
				// #define EX_VAR(n) ZEND_CALL_VAR(execute_data, n)
				// #define ZEND_CALL_VAR(call, n) ((zval*)(((char*)(call)) + ((int)(n))))
				ZVAL_NULL(EX_VAR(opline->result.var));
			}
			// 完成
			return;
		}
		// 在抛出提示时，数组可能被销毁了，临时增加引用次数来侦测此种情况
		/* The string may be destroyed while throwing the notice.
		 * Temporarily increase the refcount to detect this situation. */
		GC_ADDREF(s);
		// 警告：只有第一个字符会被赋值到字串中
		zend_error(E_WARNING, "Only the first byte will be assigned to the string offset");
		// 如果引用数-1后为0
		if (UNEXPECTED(GC_DELREF(s) == 0)) {
			// 释放字串
			zend_string_efree(s);
			// ing4, 检验：opline 返回类型不是IS_UNUSED
			if (UNEXPECTED(RETURN_VALUE_USED(opline))) {
				// 操作码结果赋值成 null
				ZVAL_NULL(EX_VAR(opline->result.var));
			}
			// 完成
			return;
		}
		// 非法的赋值位置
		/* Illegal offset assignment */
		// 如果有异常
		if (UNEXPECTED(EG(exception) != NULL)) {
			// ing4, 检验：opline 返回类型不是IS_UNUSED
			if (UNEXPECTED(RETURN_VALUE_USED(opline))) {
				// 操作码结果赋值成 未定义
				ZVAL_UNDEF(EX_VAR(opline->result.var));
			}
			// 完成
			return;
		}
	}

	// 位置正向溢出
	if ((size_t)offset >= ZSTR_LEN(s)) {
		// 按需要延长字串
		/* Extend string if needed */
		// 原长度
		zend_long old_len = ZSTR_LEN(s);
		// 延长字串
		ZVAL_NEW_STR(str, zend_string_extend(s, (size_t)offset + 1, 0));
		// 后面全是空
		memset(Z_STRVAL_P(str) + old_len, ' ', offset - old_len);
		// 最后一个 结束符
		Z_STRVAL_P(str)[offset+1] = 0;
	// 位置合法
	} else {
		// 删除字串的哈希值
		zend_string_forget_hash_val(Z_STR_P(str));
	}

	// 修改此位置的字符（核心代码在这里）
	Z_STRVAL_P(str)[offset] = c;
	// ing4, 检验：opline 返回类型不是IS_UNUSED
	if (UNEXPECTED(RETURN_VALUE_USED(opline))) {
		
		// 返回新字符 
		/* Return the new character */
		// 新字符赋值给 操作码结果
		// #define EX_VAR(n) ZEND_CALL_VAR(execute_data, n)
		// #define ZEND_CALL_VAR(call, n) ((zval*)(((char*)(call)) + ((int)(n))))
		ZVAL_CHAR(EX_VAR(opline->result.var), c);
	}
}

// ing3, 找到并返回属性信息列表中不允许 小数类型 的属性信息
static zend_property_info *zend_get_prop_not_accepting_double(zend_reference *ref)
{
	zend_property_info *prop;
	// 追踪到到引用的 属性信息列表，并遍历它（开始段）
	ZEND_REF_FOREACH_TYPE_SOURCES(ref, prop) {
		// 如果有一个 不允许 小数
		if (!(ZEND_TYPE_FULL_MASK(prop->type) & MAY_BE_DOUBLE)) {
			// 返回此属性信息
			return prop;
		}
	// 普通的遍历结束，只有括号
	} ZEND_REF_FOREACH_TYPE_SOURCES_END();
	// 返回null
	return NULL;
}

// ing3, 报错：加减法错误
static ZEND_COLD zend_long zend_throw_incdec_ref_error(
		zend_reference *ref, zend_property_info *error_prop OPLINE_DC)
{
	// 取得类型字串
	zend_string *type_str = zend_type_to_string(error_prop->type);
	// 验证是否是加法操作码
	if (ZEND_IS_INCREMENT(opline->opcode)) {
		// 报错：不能给 属性中 A类型 的引用目标 做加法运算，会超过最大值
		zend_type_error(
			"Cannot increment a reference held by property %s::$%s of type %s past its maximal value",
			ZSTR_VAL(error_prop->ce->name),
			zend_get_unmangled_property_name(error_prop->name),
			ZSTR_VAL(type_str));
		// 释放类型字串
		zend_string_release(type_str);
		return ZEND_LONG_MAX;
	// 是减法操作码
	} else {
		// 报错：不能给 属性中 A类型 的引用目标 做减法运算，会超过最大值
		zend_type_error(
			"Cannot decrement a reference held by property %s::$%s of type %s past its minimal value",
			ZSTR_VAL(error_prop->ce->name),
			zend_get_unmangled_property_name(error_prop->name),
			ZSTR_VAL(type_str));
		// 释放类型字串
		zend_string_release(type_str);
		return ZEND_LONG_MIN;
	}
}

// ing3, 报错：加减法错误
static ZEND_COLD zend_long zend_throw_incdec_prop_error(zend_property_info *prop OPLINE_DC) {
	// 取得类型字串
	zend_string *type_str = zend_type_to_string(prop->type);
	// 验证是否是加法操作码
	if (ZEND_IS_INCREMENT(opline->opcode)) {
		// 报错：不能给 A类型 的属性 做加法运算，会超过最大值
		zend_type_error("Cannot increment property %s::$%s of type %s past its maximal value",
			ZSTR_VAL(prop->ce->name),
			zend_get_unmangled_property_name(prop->name),
			ZSTR_VAL(type_str));
		// 释放类型字串
		zend_string_release(type_str);
		return ZEND_LONG_MAX;
	// 是减法操作码
	} else {
		// 报错：不能给 A类型 的属性 做减法运算，会超过最大值
		zend_type_error("Cannot decrement property %s::$%s of type %s past its minimal value",
			ZSTR_VAL(prop->ce->name),
			zend_get_unmangled_property_name(prop->name),
			ZSTR_VAL(type_str));
		// 释放类型字串
		zend_string_release(type_str);
		return ZEND_LONG_MIN;
	}
}

// ing3, 给带类型的引用目标作 自增 或 自减。p1:引用对象，p2:返回值
static void zend_incdec_typed_ref(zend_reference *ref, zval *copy OPLINE_DC EXECUTE_DATA_DC)
{
	zval tmp;
	zval *var_ptr = &ref->val;
	// 如果返回副本无效
	if (!copy) {
		// 使用临时变量占位
		copy = &tmp;
	}

	// 复制引用值
	ZVAL_COPY(copy, var_ptr);

	// 如果是加法运算
	if (ZEND_IS_INCREMENT(opline->opcode)) {
		// 调用自增函数
		increment_function(var_ptr);
	// 是减法运算
	} else {
		// 调用自减函数
		decrement_function(var_ptr);
	}

	// 如果原数是 原数是小数 新加数是整数
	if (UNEXPECTED(Z_TYPE_P(var_ptr) == IS_DOUBLE) && Z_TYPE_P(copy) == IS_LONG) {
		// 找到并返回属性信息列表中不允许 小数类型 的属性信息
		zend_property_info *error_prop = zend_get_prop_not_accepting_double(ref);
		// 如果有报错的属性信息
		if (UNEXPECTED(error_prop)) {
			// 报错：加减法错误
			zend_long val = zend_throw_incdec_ref_error(ref, error_prop OPLINE_CC);
			// 更新返回值（ ZEND_LONG_MAX 或 ZEND_LONG_MIN）
			ZVAL_LONG(var_ptr, val);
		}
	// 检查值是否适配属性信息列表的每一条信息，有一条不适配就返回失败。 p1:属性信息列表的引用,p2:值
	// 过程中可能用 zend_verify_weak_scalar_type_hint 强制转换p2
		// 某执行上下文所属函数 的参数是否使用严格类型限制
	} else if (UNEXPECTED(!zend_verify_ref_assignable_zval(ref, var_ptr, EX_USES_STRICT_TYPES()))) {
		// 销毁没有引用次数的对象，有引用次数的添加到gc回收周期里。并没有销毁 zval本身。
		zval_ptr_dtor(var_ptr);
		// 应用计算后的副本
		ZVAL_COPY_VALUE(var_ptr, copy);
		// 副本刻成成未定义
		ZVAL_UNDEF(copy);
		
	// 如果不接收返回值
	} else if (copy == &tmp) {
		// 销毁没有引用次数的对象，有引用次数的添加到gc回收周期里。并没有销毁 zval本身。
		zval_ptr_dtor(&tmp);
	}
}

// ing3, 有类型的属性 自增/自减。p1:属性信息，p2:属性值
static void zend_incdec_typed_prop(zend_property_info *prop_info, zval *var_ptr, zval *copy OPLINE_DC EXECUTE_DATA_DC)
{
	zval tmp;

	// 如果不要返回值
	if (!copy) {
		// 先使用临时变量接收
		copy = &tmp;
	}

	// 直接复制给返回值
	ZVAL_COPY(copy, var_ptr);

	// 如果是自增操作码
	if (ZEND_IS_INCREMENT(opline->opcode)) {
		// 原变量 自增
		increment_function(var_ptr);
	// 自减
	} else {
		// 原变量 自减
		decrement_function(var_ptr);
	}

	// 如果 变量是小数 并且 副本是整数
	if (UNEXPECTED(Z_TYPE_P(var_ptr) == IS_DOUBLE) && Z_TYPE_P(copy) == IS_LONG) {
		// 如果属性信息不允许小数
		if (!(ZEND_TYPE_FULL_MASK(prop_info->type) & MAY_BE_DOUBLE)) {
			// 报错：加减法错误
			zend_long val = zend_throw_incdec_prop_error(prop_info OPLINE_CC);
			// 更新原变量
			ZVAL_LONG(var_ptr, val);
		}
	// 检查属性值类型 与 属性信息要求的类型是否匹配
	} else if (UNEXPECTED(!zend_verify_property_type(prop_info, var_ptr, EX_USES_STRICT_TYPES()))) {
		// 销毁没有引用次数的对象，有引用次数的添加到gc回收周期里。并没有销毁 zval本身。
		zval_ptr_dtor(var_ptr);
		// 副本复制给原变量
		ZVAL_COPY_VALUE(var_ptr, copy);
		// 副本设置成未定义
		ZVAL_UNDEF(copy);
	// 如果没有要求返回值
	} else if (copy == &tmp) {
		// 销毁临时变量
		// 销毁没有引用次数的对象，有引用次数的添加到gc回收周期里。并没有销毁 zval本身。
		zval_ptr_dtor(&tmp);
	}
}

// ing3, 属性 前置 自增或自减
static void zend_pre_incdec_property_zval(zval *prop, zend_property_info *prop_info OPLINE_DC EXECUTE_DATA_DC)
{
	// 如果属性是整数
	if (EXPECTED(Z_TYPE_P(prop) == IS_LONG)) {
		// 如果是自增运算
		if (ZEND_IS_INCREMENT(opline->opcode)) {
			// 整数自增
			fast_long_increment_function(prop);
		// 如果是自减运算
		} else {
			// 整数自减
			fast_long_decrement_function(prop);
		}
		// 属性不是整数 并且 有属性信息 
		if (UNEXPECTED(Z_TYPE_P(prop) != IS_LONG) && UNEXPECTED(prop_info)
				&& !(ZEND_TYPE_FULL_MASK(prop_info->type) & MAY_BE_DOUBLE)) {
			// 报错：加减法错误
			zend_long val = zend_throw_incdec_prop_error(prop_info OPLINE_CC);
			// 赋值给属性
			ZVAL_LONG(prop, val);
		}
	// 属性不是整数
	} else {
		do {
			// 如果属性是引用类型
			if (Z_ISREF_P(prop)) {
				// 取出引用实例
				zend_reference *ref = Z_REF_P(prop);
				// 解引用
				prop = Z_REFVAL_P(prop);
				// 检验引用目标是否有效 ((p1)->sources).ptr != NULL
				if (UNEXPECTED(ZEND_REF_HAS_TYPE_SOURCES(ref))) {
					// 给带类型的引用目标作 自增 或 自减
					zend_incdec_typed_ref(ref, NULL OPLINE_CC EXECUTE_DATA_CC);
					// 跳出
					break;
				}
			}
			// 如果属性信息有效
			if (UNEXPECTED(prop_info)) {
				// 有类型的属性 自增/自减。p1:属性信息，p2:属性值
				zend_incdec_typed_prop(prop_info, prop, NULL OPLINE_CC EXECUTE_DATA_CC);
			// 如果是自增操作码
			} else if (ZEND_IS_INCREMENT(opline->opcode)) {
				// 属性自增
				increment_function(prop);
			} else {
				// 属性自减
				decrement_function(prop);
			}
		} while (0);
	}
	// ing4, 检验：opline 返回类型不是IS_UNUSED
	if (UNEXPECTED(RETURN_VALUE_USED(opline))) {
		// 属性复制给 操作码返回值
		ZVAL_COPY(EX_VAR(opline->result.var), prop);
	}
}

// ing3, 属性 后置 自增或自减
static void zend_post_incdec_property_zval(zval *prop, zend_property_info *prop_info OPLINE_DC EXECUTE_DATA_DC)
{
	// 属性值是整数
	if (EXPECTED(Z_TYPE_P(prop) == IS_LONG)) {
		// 赋值给操作码结果
		// #define EX_VAR(n) ZEND_CALL_VAR(execute_data, n)
		// #define ZEND_CALL_VAR(call, n) ((zval*)(((char*)(call)) + ((int)(n))))
		ZVAL_LONG(EX_VAR(opline->result.var), Z_LVAL_P(prop));
		// 如果是加法操作码
		if (ZEND_IS_INCREMENT(opline->opcode)) {
			// 整数自增
			fast_long_increment_function(prop);
		// 减法操作码
		} else {
			// 整数自减
			fast_long_decrement_function(prop);
		}
		// 属性不是整数 并且有 属性信息 并且 属性不允许 小数
		if (UNEXPECTED(Z_TYPE_P(prop) != IS_LONG) && UNEXPECTED(prop_info)
				&& !(ZEND_TYPE_FULL_MASK(prop_info->type) & MAY_BE_DOUBLE)) {
			// 报错：加减法错误
			zend_long val = zend_throw_incdec_prop_error(prop_info OPLINE_CC);
			// 更新属性值
			ZVAL_LONG(prop, val);
		}
	// 属性值不是整数
	} else {
		// 如果 属性 是引用类型
		if (Z_ISREF_P(prop)) {
			// 取得引用实例
			zend_reference *ref = Z_REF_P(prop);
			// 解引用
			prop = Z_REFVAL_P(prop);
			// 检验引用目标是否有效 ((p1)->sources).ptr != NULL
			if (ZEND_REF_HAS_TYPE_SOURCES(ref)) {
				// 给带类型的引用目标作 自增 或 自减
				zend_incdec_typed_ref(ref, EX_VAR(opline->result.var) OPLINE_CC EXECUTE_DATA_CC);
				// 结束
				return;
			}
		}

		// 如果有属性信息
		if (UNEXPECTED(prop_info)) {
			// 有类型的属性 自增/自减。p1:属性信息，p2:属性值
			zend_incdec_typed_prop(prop_info, prop, EX_VAR(opline->result.var) OPLINE_CC EXECUTE_DATA_CC);
		// 没有属性信息
		} else {
			ZVAL_COPY(EX_VAR(opline->result.var), prop);
			// 如果是自增操作码
			if (ZEND_IS_INCREMENT(opline->opcode)) {
				// 属性自增
				increment_function(prop);
			} else {
				// 属性自减
				decrement_function(prop);
			}
		}
	}
}

// ing3, 取出属性值，进行 后自增/自减 运算后更新回去。overloaded 是什么
static zend_never_inline void zend_post_incdec_overloaded_property(zend_object *object, zend_string *name, void **cache_slot OPLINE_DC EXECUTE_DATA_DC)
{
	zval rv;
	zval *z;
	zval z_copy;
	// 对象增加引用次数
	GC_ADDREF(object);
	// zend_std_read_property 读取属性， rv是返回值的副本。p1:对象，p2:属性名，p3:类型，p4:缓存位置，p5:返回值
	z =object->handlers->read_property(object, name, BP_VAR_R, cache_slot, &rv);
	// 如果有异常
	if (UNEXPECTED(EG(exception))) {
		// 释放对象
		OBJ_RELEASE(object);
		// 操作码结果设置为 未定义
		// #define EX_VAR(n) ZEND_CALL_VAR(execute_data, n)
		// #define ZEND_CALL_VAR(call, n) ((zval*)(((char*)(call)) + ((int)(n))))
		ZVAL_UNDEF(EX_VAR(opline->result.var));
		// 返回
		return;
	}
	// 复制引用目标 和 附加信息，并增加引用计数
	ZVAL_COPY_DEREF(&z_copy, z);
	// 副本复制给操作码结果
	ZVAL_COPY(EX_VAR(opline->result.var), &z_copy);
	// 如果是加法运算
	if (ZEND_IS_INCREMENT(opline->opcode)) {
		// 自增
		increment_function(&z_copy);
	// 如果是减法运算
	} else {
		// 自减
		decrement_function(&z_copy);
	}
	// zend_std_write_property 写入属性，p1:对象，p2:属性名，p3:属性值，p4:缓存位置
	object->handlers->write_property(object, name, &z_copy, cache_slot);
	// 释放对象
	OBJ_RELEASE(object);
	// 销毁没有引用次数的对象，有引用次数的添加到gc回收周期里。并没有销毁 zval本身。
	zval_ptr_dtor(&z_copy);
	// ?? 如果两个返回值是同一个实例
	if (z == &rv) {
		// 销毁没有引用次数的对象，有引用次数的添加到gc回收周期里。并没有销毁 zval本身。
		zval_ptr_dtor(z);
	}
}

// ing3, 取出属性值，进行 前自增/自减 运算后更新回去
static zend_never_inline void zend_pre_incdec_overloaded_property(zend_object *object, zend_string *name, void **cache_slot OPLINE_DC EXECUTE_DATA_DC)
{
	zval rv;
	zval *z;
	zval z_copy;
	// 对象增加引用次数
	GC_ADDREF(object);
	// zend_std_read_property 读取属性， rv是返回值的副本。p1:对象，p2:属性名，p3:类型，p4:缓存位置，p5:返回值
	z = object->handlers->read_property(object, name, BP_VAR_R, cache_slot, &rv);
	// 如果有异常
	if (UNEXPECTED(EG(exception))) {
		// 释放对象
		OBJ_RELEASE(object);
		// ing4, 检验：opline 返回类型不是IS_UNUSED
		if (UNEXPECTED(RETURN_VALUE_USED(opline))) {
			// 操作码结果为null
			// #define EX_VAR(n) ZEND_CALL_VAR(execute_data, n)
			// #define ZEND_CALL_VAR(call, n) ((zval*)(((char*)(call)) + ((int)(n))))
			ZVAL_NULL(EX_VAR(opline->result.var));
		}
		// 中断
		return;
	}
	// 复制引用目标 和 附加信息，并增加引用计数
	ZVAL_COPY_DEREF(&z_copy, z);
	// 如果是加法运算
	if (ZEND_IS_INCREMENT(opline->opcode)) {
		// 自增
		increment_function(&z_copy);
	// 如果是减法运算
	} else {
		// 自减
		decrement_function(&z_copy);
	}
	// ing4, 检验：opline 返回类型不是IS_UNUSED
	if (UNEXPECTED(RETURN_VALUE_USED(opline))) {
		// 副本复制给操作码结果
		ZVAL_COPY(EX_VAR(opline->result.var), &z_copy);
	}
	// zend_std_write_property 写入属性，p1:对象，p2:属性名，p3:属性值，p4:缓存位置
	object->handlers->write_property(object, name, &z_copy, cache_slot);
	// 释放对象
	OBJ_RELEASE(object);
	// 销毁没有引用次数的对象，有引用次数的添加到gc回收周期里。并没有销毁 zval本身。
	zval_ptr_dtor(&z_copy);
	if (z == &rv) {
		// 销毁没有引用次数的对象，有引用次数的添加到gc回收周期里。并没有销毁 zval本身。
		zval_ptr_dtor(z);
	}
}

// ing3, 读取属性值，进行运算，如果成功，把新值写回属性里。
// p1:对象，p2:属性名，p3:缓存位置，p4:op2（op1是原属性值）
static zend_never_inline void zend_assign_op_overloaded_property(zend_object *object, zend_string *name, void **cache_slot, zval *value OPLINE_DC EXECUTE_DATA_DC)
{
	zval *z;
	zval rv, res;

	// 对象引用数+1
	GC_ADDREF(object);
	// zend_std_read_property 读取属性， rv是返回值的副本。p1:对象，p2:属性名，p3:类型，p4:缓存位置，p5:返回值
	z = object->handlers->read_property(object, name, BP_VAR_R, cache_slot, &rv);
	// 如果有异常
	if (UNEXPECTED(EG(exception))) {
		// 释放对象
		OBJ_RELEASE(object);
		// ing4, 检验：opline 返回类型不是IS_UNUSED
		if (UNEXPECTED(RETURN_VALUE_USED(opline))) {
			// 操作码结果 为 未定义
			// #define EX_VAR(n) ZEND_CALL_VAR(execute_data, n)
			// #define ZEND_CALL_VAR(call, n) ((zval*)(((char*)(call)) + ((int)(n))))
			ZVAL_UNDEF(EX_VAR(opline->result.var));
		}
		// 完成
		return;
	}
	// 调用操作码（在opline参数里）对应的二进制处理方法，p1:返回值，p2:op1，p3:op2
	if (zend_binary_op(&res, z, value OPLINE_CC) == SUCCESS) {
		// zend_std_write_property 写入属性，p1:对象，p2:属性名，p3:属性值，p4:缓存位置
		object->handlers->write_property(object, name, &res, cache_slot);
	}
	// ing4, 检验：opline 返回类型不是IS_UNUSED
	if (UNEXPECTED(RETURN_VALUE_USED(opline))) {
		// 把运算结果 复制 给操作码结果
		ZVAL_COPY(EX_VAR(opline->result.var), &res);
	}
	
	// 如果 zend_std_read_property 两个返回值相同
	if (z == &rv) {
		// 销毁没有引用次数的对象，有引用次数的添加到gc回收周期里。并没有销毁 zval本身。
		zval_ptr_dtor(z);
	}
	// 销毁没有引用次数的对象，有引用次数的添加到gc回收周期里。并没有销毁 zval本身。
	zval_ptr_dtor(&res);
	// 释放对象
	OBJ_RELEASE(object);
}

// 扩展用的工具方法
/* Utility Functions for Extensions */
// ing3, 扩展的语句处理器
static void zend_extension_statement_handler(const zend_extension *extension, zend_execute_data *frame)
{
	if (extension->statement_handler) {
		extension->statement_handler(frame);
	}
}

// ing3, 扩展函数调用开始
static void zend_extension_fcall_begin_handler(const zend_extension *extension, zend_execute_data *frame)
{
	if (extension->fcall_begin_handler) {
		extension->fcall_begin_handler(frame);
	}
}

// ing3, 扩展函数调用结束
static void zend_extension_fcall_end_handler(const zend_extension *extension, zend_execute_data *frame)
{
	if (extension->fcall_end_handler) {
		extension->fcall_end_handler(frame);
	}
}

// ing3, 取得目标符号表。运行时符号表 或 当前执行数据中的符号表
static zend_always_inline HashTable *zend_get_target_symbol_table(int fetch_type EXECUTE_DATA_DC)
{
	HashTable *ht;
	// 如果是从global里获取
	if (EXPECTED(fetch_type & (ZEND_FETCH_GLOBAL_LOCK | ZEND_FETCH_GLOBAL))) {
		// 哈希表是执行时 符号表
		ht = &EG(symbol_table);
	// 其他情况
	} else {
		// 获取类型必须是 本地获取
		ZEND_ASSERT(fetch_type & ZEND_FETCH_LOCAL);
		// 如果调用中 没有符号表
		// Z_TYPE_INFO((execute_data)->This))
		if (!(EX_CALL_INFO() & ZEND_CALL_HAS_SYMBOL_TABLE)) {
			// 重建符号表，zend_execute_API.c
			zend_rebuild_symbol_table();
		}
		// #define EX(element) 			((execute_data)->element)
		ht = EX(symbol_table);
	}
	// 返回符号表
	return ht;
}

// ing3, 报错：未定义的数组key（整数）
static zend_never_inline ZEND_COLD void ZEND_FASTCALL zend_undefined_offset(zend_long lval)
{
	zend_error(E_WARNING, "Undefined array key " ZEND_LONG_FMT, lval);
}

// ing3, 报错：未定义的数组key（字串）
static zend_never_inline ZEND_COLD void ZEND_FASTCALL zend_undefined_index(const zend_string *offset)
{
	zend_error(E_WARNING, "Undefined array key \"%s\"", ZSTR_VAL(offset));
}

// ing3, 写入未知的 索引位置（数字）。
// 会报一个错，但只要没有exception，最后会添加 一个此哈希值 值为空的元素
ZEND_API ZEND_COLD zval* ZEND_FASTCALL zend_undefined_offset_write(HashTable *ht, zend_long lval)
{
	// 当抛出提示时，数组可能已经被销毁了
	// 临时增加引用数来侦察这种情况
	/* The array may be destroyed while throwing the notice.
	 * Temporarily increase the refcount to detect this situation. */
	 
	// 哈希表不是不可变
	if (!(GC_FLAGS(ht) & IS_ARRAY_IMMUTABLE)) {
		// 增加引用次数
		GC_ADDREF(ht);
	}
	// 报错：未定义的数组key（整数）
	zend_undefined_offset(lval);
	// 如果不是不可变 并且 引用次数-1后不是1
	if (!(GC_FLAGS(ht) & IS_ARRAY_IMMUTABLE) && GC_DELREF(ht) != 1) {
		// 如果引用数为0
		if (!GC_REFCOUNT(ht)) {
			// 销毁哈希表
			zend_array_destroy(ht);
		}
		// 返回null
		return NULL;
	}
	// 如果有异常
	if (EG(exception)) {
		// 返回null
		return NULL;
	}
	// 给哈希表添加元素，哈希值，值为未定义
	return zend_hash_index_add_new(ht, lval, &EG(uninitialized_zval));
}

// ing3, 报错：写入未知的 索引位置（字串）。然后创建一个空元素。
ZEND_API ZEND_COLD zval* ZEND_FASTCALL zend_undefined_index_write(HashTable *ht, zend_string *offset)
{
	zval *retval;
	// 抛出提示时，array可能已经销毁了
	// 临时增加引用计数 来 侦测这种情况
	/* The array may be destroyed while throwing the notice.
	 * Temporarily increase the refcount to detect this situation. */
	 
	// 不是不可变的
	if (!(GC_FLAGS(ht) & IS_ARRAY_IMMUTABLE)) {
		// 增加引用次数
		GC_ADDREF(ht);
	}
	// 当抛出未定义索引号警告时，key可以已经被释放了
	/* Key may be released while throwing the undefined index warning. */
	// key增加引用次数
	zend_string_addref(offset);
	
	// 报错：未定义的数组key（字串）
	zend_undefined_index(offset);
	// 如果不是不可更改 并且-1后引用次数不是1
	if (!(GC_FLAGS(ht) & IS_ARRAY_IMMUTABLE) && GC_DELREF(ht) != 1) {
		// 如果ht没有引用
		if (!GC_REFCOUNT(ht)) {
			// 销毁它
			zend_array_destroy(ht);
		}
		// 返回null
		retval = NULL;
	// 如果有异常
	} else if (EG(exception)) {
		// 返回null
		retval = NULL;
	// 其他情况
	} else {
		// 先创建一个空元素，等待后面的写入（因为这是更新操作）
		retval = zend_hash_add_new(ht, offset, &EG(uninitialized_zval));
	}
	// 释放key
	zend_string_release(offset);
	// 返回结果 
	return retval;
}

// ing3, 报错：调用未定义的方法
static zend_never_inline ZEND_COLD void ZEND_FASTCALL zend_undefined_method(const zend_class_entry *ce, const zend_string *method)
{
	zend_throw_error(NULL, "Call to undefined method %s::%s()", ZSTR_VAL(ce->name), ZSTR_VAL(method));
}

// ing3, 报错：无效的调用
static zend_never_inline ZEND_COLD void ZEND_FASTCALL zend_invalid_method_call(zval *object, zval *function_name)
{
	zend_throw_error(NULL, "Call to a member function %s() on %s",
		Z_STRVAL_P(function_name), zend_zval_type_name(object));
}

// ing3, 报错：静态调用非静态方法
static zend_never_inline ZEND_COLD void ZEND_FASTCALL zend_non_static_method_call(const zend_function *fbc)
{
	zend_throw_error(
		zend_ce_error,
		"Non-static method %s::%s() cannot be called statically",
		ZSTR_VAL(fbc->common.scope->name), ZSTR_VAL(fbc->common.function_name));
}

// ing3, 报错，此参数必须引用传递
ZEND_COLD void ZEND_FASTCALL zend_param_must_be_ref(const zend_function *func, uint32_t arg_num)
{
	// 获取参数名
	const char *arg_name = get_function_arg_name(func, arg_num);

	zend_error(E_WARNING, "%s%s%s(): Argument #%d%s%s%s must be passed by reference, value given",
		func->common.scope ? ZSTR_VAL(func->common.scope->name) : "",
		func->common.scope ? "::" : "",
		ZSTR_VAL(func->common.function_name),
		arg_num,
		arg_name ? " ($" : "",
		arg_name ? arg_name : "",
		arg_name ? ")" : ""
	);
}

// ing3, 报错：不可以把标量当成数组用
static zend_never_inline ZEND_COLD void ZEND_FASTCALL zend_use_scalar_as_array(void)
{
	zend_throw_error(NULL, "Cannot use a scalar value as an array");
}
// ing3, 报错：添加元素失败，下一个元素已经存在
static zend_never_inline ZEND_COLD void ZEND_FASTCALL zend_cannot_add_element(void)
{
	zend_throw_error(NULL, "Cannot add element to the array as the next element is already occupied");
}

// ing3, 抛出警告：资源作为偏移量，转成了数字（自己的ID）
ZEND_API ZEND_COLD void ZEND_FASTCALL zend_use_resource_as_offset(const zval *dim)
{
	zend_error(E_WARNING,
		"Resource ID#" ZEND_LONG_FMT " used as offset, casting to integer (" ZEND_LONG_FMT ")",
		Z_RES_HANDLE_P(dim), Z_RES_HANDLE_P(dim));
}

// ing3, 报错：不可以对string使用[]操作
static zend_never_inline ZEND_COLD void ZEND_FASTCALL zend_use_new_element_for_string(void)
{
	zend_throw_error(NULL, "[] operator not supported for strings");
}

// ing3, 就只是报错，无法使用偏移量进行操作
static ZEND_COLD void zend_binary_assign_op_dim_slow(zval *container, zval *dim OPLINE_DC EXECUTE_DATA_DC)
{
	// 如果容器是字串
	if (UNEXPECTED(Z_TYPE_P(container) == IS_STRING)) {
		// 如果是unset操作
		if (opline->op2_type == IS_UNUSED) {
			// 报错：不可以对string使用[]操作
			zend_use_new_element_for_string();
		// 其他操作
		} else {
			// 像数组一样操作字串，把索引号转成整数
			zend_check_string_offset(dim, BP_VAR_RW EXECUTE_DATA_CC);
			// 报错：字串偏移量用法错误
			zend_wrong_string_offset_error();
		}
	// 容器是其他类型
	} else {
		// 报错：不可以把标量当成数组用
		zend_use_scalar_as_array();
	}
}

// ing3, 把各种类型的 索引号 转成可用类型，ht用来在必要的时候做删除操作，p1:哈希表, p2:索引号，p3:转换后的索引号返回值
static zend_never_inline zend_uchar slow_index_convert(HashTable *ht, const zval *dim, zend_value *value EXECUTE_DATA_DC)
{
	// 根据索引的类型操作
	switch (Z_TYPE_P(dim)) {
		// 未定义
		case IS_UNDEF: {
			// 在抛出提示信息地时，数组可能被销毁了
			// 临时增加引用次数来侦测此种情况
			/* The array may be destroyed while throwing the notice.
			 * Temporarily increase the refcount to detect this situation. */
			if (!(GC_FLAGS(ht) & IS_ARRAY_IMMUTABLE)) {
				// 增加引用次数
				GC_ADDREF(ht);
			}
			// _zval_undefined_op2, 检查op2的变量，没有定义则抛异常
			ZVAL_UNDEFINED_OP2();
			
			// 如果不是不可变数组 并且 -1后引用次数为0
			if (!(GC_FLAGS(ht) & IS_ARRAY_IMMUTABLE) && !GC_DELREF(ht)) {
				// 销毁哈希表
				zend_array_destroy(ht);
				// 返回类型：null
				return IS_NULL;
			}
			
			// 如果有异常
			if (EG(exception)) {
				// 返回类型：null
				return IS_NULL;
			}
			// ((void)0)
			ZEND_FALLTHROUGH;
		}
		// null
		case IS_NULL:
			// 空字串
			value->str = ZSTR_EMPTY_ALLOC();
			// 返回类型：字串
			return IS_STRING;
		// 小数
		case IS_DOUBLE:
			// 小数转成整数
			value->lval = zend_dval_to_lval(Z_DVAL_P(dim));
			// 如果转换前后不兼容
			if (!zend_is_long_compatible(Z_DVAL_P(dim), value->lval)) {
				// 在抛出提示时，数组可能被销毁了
				// 临时增加引用次数来侦测此种情况
				/* The array may be destroyed while throwing the notice.
				 * Temporarily increase the refcount to detect this situation. */
				// 如果不是不可更改数组
				if (!(GC_FLAGS(ht) & IS_ARRAY_IMMUTABLE)) {
					// 增加引用次数
					GC_ADDREF(ht);
				}
				// 报错：隐式转换，从小数转成整数会丢失精度
				zend_incompatible_double_to_long_error(Z_DVAL_P(dim));
				// 如果不是不可变数组 并且 -1后引用次数为0
				if (!(GC_FLAGS(ht) & IS_ARRAY_IMMUTABLE) && !GC_DELREF(ht)) {
					// 销毁数组
					zend_array_destroy(ht);
					// 返回类型：null
					return IS_NULL;
				}
				// 如果有异常
				if (EG(exception)) {
					// 返回类型：null
					return IS_NULL;
				}
			}
			// 返回类型：null
			return IS_LONG;
		// 资源
		case IS_RESOURCE:
			// 在抛出提示时，数组可能被销毁了
			// 临时增加引用次数来侦测此种情况
			/* The array may be destroyed while throwing the notice.
			 * Temporarily increase the refcount to detect this situation. */
			if (!(GC_FLAGS(ht) & IS_ARRAY_IMMUTABLE)) {
				// 增加引用次数
				GC_ADDREF(ht);
			}
			// 抛出警告：资源作为偏移量，转成了数字（自己的ID）
			zend_use_resource_as_offset(dim);
			// 如果ht不是不可变的 并且 引用数-1后为0
			if (!(GC_FLAGS(ht) & IS_ARRAY_IMMUTABLE) && !GC_DELREF(ht)) {
				// 销毁 ht
				zend_array_destroy(ht);
				// 返回类型：null
				return IS_NULL;
			}
			// 如果有异常
			if (EG(exception)) {
				// 返回类型：null
				return IS_NULL;
			}
			// 取得资源类型的编号
			value->lval = Z_RES_HANDLE_P(dim);
			// 返回，是整数类型
			return IS_LONG;
		// false
		case IS_FALSE:
			// 返回值 0
			value->lval = 0;
			// 返回类型：整数
			return IS_LONG;
		// true
		case IS_TRUE:
			// 返回值 1
			value->lval = 1;
			// 返回类型：整数
			return IS_LONG;
		// 其他情况
		default:
			// 报错：非法的偏移量类型
			zend_illegal_offset();
			// 返回类型：null
			return IS_NULL;
	}
}

// ing3, 把各种类型的 索引号 转成可用类型，ht用来在必要的时候做删除操作，p1:哈希表, p2:索引号，p3:转换后的索引号返回值
// 如果处理完后 哈希表引用次数不是1，会返回null。是怕脏写么？
static zend_never_inline zend_uchar slow_index_convert_w(HashTable *ht, const zval *dim, zend_value *value EXECUTE_DATA_DC)
{
	// 根据索引号类型
	switch (Z_TYPE_P(dim)) {
		// 未定义
		case IS_UNDEF: {
			// 在抛出提示时，数组可能被销毁了
			// 临时增加引用次数来侦测此种情况
			/* The array may be destroyed while throwing the notice.
			 * Temporarily increase the refcount to detect this situation. */
			if (!(GC_FLAGS(ht) & IS_ARRAY_IMMUTABLE)) {
				// 增加引用次数
				GC_ADDREF(ht);
			}
			// _zval_undefined_op2, 检查op2的变量，没有定义则抛异常
			ZVAL_UNDEFINED_OP2();
			// 如果不是不可更改 并且 -1后引用次数不是1
			if (!(GC_FLAGS(ht) & IS_ARRAY_IMMUTABLE) && GC_DELREF(ht) != 1) {
				// 如果引用次数为0
				if (!GC_REFCOUNT(ht)) {
					// 销毁哈希表
					zend_array_destroy(ht);
				}
				// 返回null
				return IS_NULL;
			}
			// 如果有异常
			if (EG(exception)) {
				// 返回null
				return IS_NULL;
			}
			// ((void)0)
			ZEND_FALLTHROUGH;
		}
		// null
		case IS_NULL:
			// 空字串
			value->str = ZSTR_EMPTY_ALLOC();
			// 返回类型：字串
			return IS_STRING;
		// 小数
		case IS_DOUBLE:
			// 小数转成整数
			value->lval = zend_dval_to_lval(Z_DVAL_P(dim));
			// 如果转换前后不兼容
			if (!zend_is_long_compatible(Z_DVAL_P(dim), value->lval)) {
				// 在抛出提示时，数组可能被销毁了
				// 临时增加引用次数来侦测此种情况
				/* The array may be destroyed while throwing the notice.
				 * Temporarily increase the refcount to detect this situation. */
				if (!(GC_FLAGS(ht) & IS_ARRAY_IMMUTABLE)) {
					// 增加引用次数
					GC_ADDREF(ht);
				}
				// 报错：隐式转换，从小数转成整数会丢失精度
				zend_incompatible_double_to_long_error(Z_DVAL_P(dim));
				// 如果不是不可更改 并且 -1后引用次数不是1
				if (!(GC_FLAGS(ht) & IS_ARRAY_IMMUTABLE) && GC_DELREF(ht) != 1) {
					// 如果引用次数为0
					if (!GC_REFCOUNT(ht)) {
						// 销毁哈希表
						zend_array_destroy(ht);
					}
					// 返回null
					return IS_NULL;
				}
				// 如果有异常
				if (EG(exception)) {
					// 返回null
					return IS_NULL;
				}
			}
			// 返回类型：整数
			return IS_LONG;
		// 资源
		case IS_RESOURCE:
		// 在抛出提示时，数组可能被销毁了
			// 临时增加引用次数来侦测此种情况
			/* The array may be destroyed while throwing the notice.
			 * Temporarily increase the refcount to detect this situation. */
			if (!(GC_FLAGS(ht) & IS_ARRAY_IMMUTABLE)) {
				GC_ADDREF(ht);
			}
			// 抛出警告：资源作为偏移量，转成了数字（自己的ID）
			zend_use_resource_as_offset(dim);
			// 如果不是不可更改 并且 -1后引用次数不是1
			if (!(GC_FLAGS(ht) & IS_ARRAY_IMMUTABLE) && GC_DELREF(ht) != 1) {
				// 如果引用次数为0
				if (!GC_REFCOUNT(ht)) {
					// 销毁哈希表
					zend_array_destroy(ht);
				}
				// 返回null
				return IS_NULL;
			}
			// 如果有异常
			if (EG(exception)) {
				// 返回null
				return IS_NULL;
			}
			// 取得资源类型的编号
			value->lval = Z_RES_HANDLE_P(dim);
			// 返回，是整数类型
			return IS_LONG;
		// false
		case IS_FALSE:
			// 返回值 0
			value->lval = 0;
			// 返回类型：整数
			return IS_LONG;
		// true
		case IS_TRUE:
			// 返回值 1
			value->lval = 1;
			// 返回类型：整数
			return IS_LONG;
		// 其他情况
		default:
			// 报错：非法的偏移量类型
			zend_illegal_offset();
			// 返回类型：null
			return IS_NULL;
	}
}

// ing3, 像数组一样读写，的内部实现。p1:哈希表，p2:索引，p3:索引变量类型，p4:操作类型
static zend_always_inline zval *zend_fetch_dimension_address_inner(HashTable *ht, const zval *dim, int dim_type, int type EXECUTE_DATA_DC)
{
	zval *retval = NULL;
	zend_string *offset_key;
	zend_ulong hval;

try_again:
	// 情况1：维度索引 是 整数
	if (EXPECTED(Z_TYPE_P(dim) == IS_LONG)) {
		// 取出整数
		hval = Z_LVAL_P(dim);
// 当成数字索引处理
num_index:
		// 如果不是新建（创建总是单独处理，而且比较简单）
		if (type != BP_VAR_W) {
			// 用数字索引号在哈希表里查找，找不到就 goto p4
			ZEND_HASH_INDEX_FIND(ht, hval, retval, num_undef);
			// 找到了直接返回
			return retval;
// 无法通过数字索引找到元素
num_undef:
			// 没找到，按操作类型处理
			switch (type) {
				// 如果是读
				case BP_VAR_R:
					// 报错：未定义的数组key（整数）
					zend_undefined_offset(hval);
					// ((void)0)
					ZEND_FALLTHROUGH;
				// 如果是删除 或 isset。其实没做什么
				case BP_VAR_UNSET:
				case BP_VAR_IS:
					// 上面3种情况都返回 未初始化 zval
					retval = &EG(uninitialized_zval);
					break;
				// 如果是更新
				case BP_VAR_RW:
					// 报错：写入未知的 索引位置。然后创建一个空元素。
					retval = zend_undefined_offset_write(ht, hval);
					break;
				}
		} else {
			// 在哈希表中用整数索引读取
			ZEND_HASH_INDEX_LOOKUP(ht, hval, retval);
		}
	// 情况2：维度索引 是 字串
	} else if (EXPECTED(Z_TYPE_P(dim) == IS_STRING)) {
		// 取出字串
		offset_key = Z_STR_P(dim);
		// ZEND_CONST_COND：windows直接取 p2
		if (ZEND_CONST_COND(dim_type != IS_CONST, 1)) {
			// 如果字串能转成数值
			if (ZEND_HANDLE_NUMERIC(offset_key, hval)) {
				// 按数值处理
				goto num_index;
			}
		}
str_index:
		// 如果操作类型不是创建
		if (type != BP_VAR_W) {
			// 哈希表中查找这个元素
			// ZEND_CONST_COND：windows直接取 p2
			retval = zend_hash_find_ex(ht, offset_key, ZEND_CONST_COND(dim_type == IS_CONST, 0));
			// 如果没找到
			if (!retval) {
				// 按操作类型
				switch (type) {
					// 读取
					case BP_VAR_R:
						// 报错：未定义的数组key（字串）
						zend_undefined_index(offset_key);
						// ((void)0)
						ZEND_FALLTHROUGH;
					// 删除或isset, 什么也没做
					case BP_VAR_UNSET:
					case BP_VAR_IS:
						// 返回值为空
						retval = &EG(uninitialized_zval);
						break;
					// 更新
					case BP_VAR_RW:
						// 报错：写入未知的 索引位置（字串）。然后创建一个空元素。
						retval = zend_undefined_index_write(ht, offset_key);
						break;
				}
			}
		// 操作类型是创建
		} else {
			// 哈希表中查找元素
			retval = zend_hash_lookup(ht, offset_key);
		}
	// 情况3：维度索引 是 引用类型
	} else if (EXPECTED(Z_TYPE_P(dim) == IS_REFERENCE)) {
		// 追踪引用目标
		dim = Z_REFVAL_P(dim);
		// 从头再来
		goto try_again;
	// 情况4：其他类型
	} else {
		zend_value val;
		zend_uchar t;
		// 如果不是创建 或 更新
		if (type != BP_VAR_W && type != BP_VAR_RW) {
			// 把各种类型的 索引号 转成可用类型，ht用来在必要的时候做删除操作，p1:哈希表, p2:索引号，p3:转换后的索引号返回值
			t = slow_index_convert(ht, dim, &val EXECUTE_DATA_CC);
		} else {
			// 把各种类型的 索引号 转成可用类型，ht用来在必要的时候做删除操作，p1:哈希表, p2:索引号，p3:转换后的索引号返回值
			// 如果处理完后 哈希表引用次数不是1，会返回null。是怕脏写么？
			t = slow_index_convert_w(ht, dim, &val EXECUTE_DATA_CC);
		}
		// 索引号是字串
		if (t == IS_STRING) {
			// 取出字串
			offset_key = val.str;
			// 按字串索引操作
			goto str_index;
		// 索引号是整数 
		} else if (t == IS_LONG) {
			// 取出整数 
			hval = val.lval;
			// 按整数索引操作
			goto num_index;
		// 其他类型
		} else {
			// 如果是写和更新看你咯 结果为 null ，否则为空 zval
			retval = (type == BP_VAR_W || type == BP_VAR_RW) ?
					NULL : &EG(uninitialized_zval);
		}
	}
	// 返回结果 
	return retval;
}

// ing3, 像数组一样创建元素，索引为变量
static zend_never_inline zval* ZEND_FASTCALL zend_fetch_dimension_address_inner_W(HashTable *ht, const zval *dim EXECUTE_DATA_DC)
{
	// 像数组一样读写，的内部实现。p1:哈希表，p2:索引，p3:索引变量类型，p4:操作类型
	return zend_fetch_dimension_address_inner(ht, dim, IS_TMP_VAR, BP_VAR_W EXECUTE_DATA_CC);
}

// ing3, 像数组一样创建元素，索引为常量
static zend_never_inline zval* ZEND_FASTCALL zend_fetch_dimension_address_inner_W_CONST(HashTable *ht, const zval *dim EXECUTE_DATA_DC)
{
	// 像数组一样读写，的内部实现。p1:哈希表，p2:索引，p3:索引变量类型，p4:操作类型
	return zend_fetch_dimension_address_inner(ht, dim, IS_CONST, BP_VAR_W EXECUTE_DATA_CC);
}

// ing3, 像数组一样更新，索引为变量
static zend_never_inline zval* ZEND_FASTCALL zend_fetch_dimension_address_inner_RW(HashTable *ht, const zval *dim EXECUTE_DATA_DC)
{
	// 像数组一样读写，的内部实现。p1:哈希表，p2:索引，p3:索引变量类型，p4:操作类型
	return zend_fetch_dimension_address_inner(ht, dim, IS_TMP_VAR, BP_VAR_RW EXECUTE_DATA_CC);
}

// ing3, 像数组一样更新，索引为常量。
static zend_never_inline zval* ZEND_FASTCALL zend_fetch_dimension_address_inner_RW_CONST(HashTable *ht, const zval *dim EXECUTE_DATA_DC)
{
	// 像数组一样读写，的内部实现。p1:哈希表，p2:索引，p3:索引变量类型，p4:操作类型
	return zend_fetch_dimension_address_inner(ht, dim, IS_CONST, BP_VAR_RW EXECUTE_DATA_CC);
}

// ing3, 像数组一样读取地址，p1：返回值，p2:容器（各种类型），p3:索引值，p4:索引类型，p5:操作类型
static zend_always_inline void zend_fetch_dimension_address(zval *result, zval *container, zval *dim, int dim_type, int type EXECUTE_DATA_DC)
{
	zval *retval;
	// 情况1：如果容器是数组
	if (EXPECTED(Z_TYPE_P(container) == IS_ARRAY)) {
// 尝试当作数组处理
try_array:
		// 给数组创建副本，使用副本
		SEPARATE_ARRAY(container);
// 从数组中读取
fetch_from_array:
		// 没有键（新增元素）
		if (dim == NULL) {
			// 插入并返回，新的未定义元素
			retval = zend_hash_next_index_insert(Z_ARRVAL_P(container), &EG(uninitialized_zval));
			// 如果结果是null
			if (UNEXPECTED(retval == NULL)) {
				// 报错：添加元素失败，下一个元素已经存在
				zend_cannot_add_element();
				// 结果清空
				ZVAL_UNDEF(result);
				// 返回
				return;
			}
		// 有键
		} else {
			// 像数组一样读写，的内部实现。p1:哈希表，p2:索引，p3:索引变量类型，p4:操作类型
			retval = zend_fetch_dimension_address_inner(Z_ARRVAL_P(container), dim, dim_type, type EXECUTE_DATA_CC);
			// 如果结果无效
			if (UNEXPECTED(!retval)) {
				// 如果抛出未定义索引异常时数组被修改了，这里可能会操作失败
				/* This may fail without throwing if the array was modified while throwing an
				 * undefined index error. */
				// 返回值为 null
				ZVAL_NULL(result);
				// 返回
				return;
			}
		}
		// 结果更新成 间接引用
		ZVAL_INDIRECT(result, retval);
		// 返回
		return;
	// 情况2：如果容器是引用类型
	} else if (EXPECTED(Z_TYPE_P(container) == IS_REFERENCE)) {
		// 取出引用实例
		zend_reference *ref = Z_REF_P(container);
		// 解引用
		container = Z_REFVAL_P(container);
		// 如果类型是数组
		if (EXPECTED(Z_TYPE_P(container) == IS_ARRAY)) {
			// 按数组处理
			goto try_array;
		// 如果值为 未定义，null，false
		} else if (EXPECTED(Z_TYPE_P(container) <= IS_FALSE)) {
			// 如果操作类型不是删除
			if (type != BP_VAR_UNSET) {
				// 检验引用目标是否有效 ((p1)->sources).ptr != NULL
				if (ZEND_REF_HAS_TYPE_SOURCES(ref)) {
					// 检验数组是否可以被 赋值给引用实例。如果不可以，报错
					if (UNEXPECTED(!zend_verify_ref_array_assignable(ref))) {
						// 结果设置成未定义
						ZVAL_UNDEF(result);
						// 返回
						return;
					}
				}
				// 容器初始化成数组 
				array_init(container);
				// 转到从数组中获取
				goto fetch_from_array;
			// 操作类型是删除
			} else {
				// 返回 Null
				goto return_null;
			}
		}
	}
	// 情况3：如果容器是 字串
	if (UNEXPECTED(Z_TYPE_P(container) == IS_STRING)) {
		// 如果索引为null
		if (dim == NULL) {
			// 报错：不可以对string使用[]操作
			zend_use_new_element_for_string();
		} else {
			// 像数组一样操作字串，把索引号转成整数
			zend_check_string_offset(dim, type EXECUTE_DATA_CC);
			// 报错：字串偏移量用法错误
			zend_wrong_string_offset_error();
		}
		// 结果为未定义
		ZVAL_UNDEF(result);
	// 情况4：如果容器是 对象
	} else if (EXPECTED(Z_TYPE_P(container) == IS_OBJECT)) {
		// 取出对象
		zend_object *obj = Z_OBJ_P(container);
		// 增加引用次数
		GC_ADDREF(obj);
		// 如果索引类型是编译 并且索引 不是null 并且 索引类型为 未定义
		// ZEND_CONST_COND：windows直接取 p2
		if (ZEND_CONST_COND(dim_type == IS_CV, dim != NULL) && UNEXPECTED(Z_TYPE_P(dim) == IS_UNDEF)) {
			// _zval_undefined_op2, 检查op2的变量，没有定义则抛异常
			dim = ZVAL_UNDEFINED_OP2();
		// 否则 ，如果索引类型为常量 并且 有扩展值 ZEND_EXTRA_VALUE？
		} else if (dim_type == IS_CONST && Z_EXTRA_P(dim) == ZEND_EXTRA_VALUE) {
			// 用下一个zval
			dim++;
		}
		// zend_std_read_dimension，像数组一样读取
		retval = obj->handlers->read_dimension(obj, dim, type, result);

		// 如果结果是未初化变量
		if (UNEXPECTED(retval == &EG(uninitialized_zval))) {
			// 找到所属类
			zend_class_entry *ce = obj->ce;
			// 返回值为 null
			ZVAL_NULL(result);
			// 超载元素的 间接修改 是无效的
			zend_error(E_NOTICE, "Indirect modification of overloaded element of %s has no effect", ZSTR_VAL(ce->name));
		// 如果有返回值，并且返回值不是 未定义
		} else if (EXPECTED(retval && Z_TYPE_P(retval) != IS_UNDEF)) {
			// 如果返回值不是引用类型
			if (!Z_ISREF_P(retval)) {
				// 如果结果 与 读取到的值不同
				if (result != retval) {
					// 把读取到的值复制给 结果
					ZVAL_COPY(result, retval);
					// 更新读取到的值，和返回值保持一致
					retval = result;
				}
				// 如果读取到的值不是对象
				if (Z_TYPE_P(retval) != IS_OBJECT) {
					// 找到被读取对象的所属类
					zend_class_entry *ce = obj->ce;
					// 提示：此类的 超载元素 间接修改是无效的 ？
					zend_error(E_NOTICE, "Indirect modification of overloaded element of %s has no effect", ZSTR_VAL(ce->name));
				}
			// 返回值是引用类型 并且 引用次数是1
			} else if (UNEXPECTED(Z_REFCOUNT_P(retval) == 1)) {
				// 返回值更新为未定义
				ZVAL_UNREF(retval);
			}
			// 如果读取到的值 与返回结果不同
			if (result != retval) {
				// 把结果更新成间接引用
				ZVAL_INDIRECT(result, retval);
			}
		} else {
			// 到这里肯定有 异常
			ZEND_ASSERT(EG(exception) && "read_dimension() returned NULL without exception");
			// 结果更新成未定义
			ZVAL_UNDEF(result);
		}
		// 如果 被读取对象引用次数-1后为0
		if (UNEXPECTED(GC_DELREF(obj) == 0)) {
			// 释放指定的对象，并更新 objects_store 中的指针
			zend_objects_store_del(obj);
		}
	// 其他情况
	} else {
		// 情况5：如果容器类型是 false,null,未定义
		if (EXPECTED(Z_TYPE_P(container) <= IS_FALSE)) {
			// 操作类型不是写入 并且 容器是未定义
			if (type != BP_VAR_W && UNEXPECTED(Z_TYPE_P(container) == IS_UNDEF)) {
				// _zval_undefined_op1, 检查op1的变量，没有定义则抛异常
				ZVAL_UNDEFINED_OP1();
			}
			// 如果操作类型不是删除
			if (type != BP_VAR_UNSET) {
				// 创建新哈希表
				HashTable *ht = zend_new_array(0);
				// 容器的原类型
				zend_uchar old_type = Z_TYPE_P(container);
				// 把新哈希表添加进容器中
				ZVAL_ARR(container, ht);
				// 如果原类型为false
				if (UNEXPECTED(old_type == IS_FALSE)) {
					// 哈希表增加引用次数
					GC_ADDREF(ht);
					//  报错：把false转成数组已弃用
					zend_false_to_array_deprecated();
					// 哈希表减少引用次数后为0
					if (UNEXPECTED(GC_DELREF(ht) == 0)) {
						// 销毁哈希表
						zend_array_destroy(ht);
						// 返回null
						goto return_null;
					}
				}
				// 尝试从哈希表中读取
				goto fetch_from_array;
			// 如果操作类型是删除
			} else {
				// 容器值为false
				if (UNEXPECTED(Z_TYPE_P(container) == IS_FALSE)) {
					// 报错：把false转成数组已弃用
					zend_false_to_array_deprecated();
				}
return_null:
				// 给只读模式用
				/* for read-mode only */
				// 如果索引值不是null 并且 索引值是未定义
				// ZEND_CONST_COND：windows直接取 p2
				if (ZEND_CONST_COND(dim_type == IS_CV, dim != NULL) && UNEXPECTED(Z_TYPE_P(dim) == IS_UNDEF)) {
					// _zval_undefined_op2, 检查op2的变量，没有定义则抛异常
					ZVAL_UNDEFINED_OP2();
				}
				// 返回值为 null
				ZVAL_NULL(result);
			}
		// 情况5：容器类型 不是 （数组，引用，字串，对象,false,null,未定义）
		} else {
			// 如果操作类型是删除
			if (type == BP_VAR_UNSET) {
				// 报错：不可以在非数据对象中用 偏移量 进行删除操作
				zend_throw_error(NULL, "Cannot unset offset in a non-array variable");
				// 结果为 未定义
				ZVAL_UNDEF(result);
			// 操作类型不是删除
			} else {
				// 报错：不可以把标量当成数组用
				zend_use_scalar_as_array();
				// 结果为 未定义
				ZVAL_UNDEF(result);
			}
		}
	}
}

// ing3, 像数组一样读取地址，操作类型为 BP_VAR_W，p1：返回值，p2:容器（各种类型），p3:索引值，p4:索引类型
static zend_never_inline void ZEND_FASTCALL zend_fetch_dimension_address_W(zval *container_ptr, zval *dim, int dim_type OPLINE_DC EXECUTE_DATA_DC)
{
	// 取出 操作码结果变量
	zval *result = EX_VAR(opline->result.var);
	// 像数组一样读取地址，p1：返回值，p2:容器（各种类型），p3:索引值，p4:索引类型，p5:操作类型
	zend_fetch_dimension_address(result, container_ptr, dim, dim_type, BP_VAR_W EXECUTE_DATA_CC);
}

// ing3, 像数组一样读取地址，操作类型为 BP_VAR_RW，p1：返回值，p2:容器（各种类型），p3:索引值，p4:索引类型
static zend_never_inline void ZEND_FASTCALL zend_fetch_dimension_address_RW(zval *container_ptr, zval *dim, int dim_type OPLINE_DC EXECUTE_DATA_DC)
{
	// 取出 操作码结果变量
	zval *result = EX_VAR(opline->result.var);
	// 像数组一样读取地址，p1：返回值，p2:容器（各种类型），p3:索引值，p4:索引类型，p5:操作类型
	zend_fetch_dimension_address(result, container_ptr, dim, dim_type, BP_VAR_RW EXECUTE_DATA_CC);
}

// ing3, 像数组一样读取地址，操作类型为 BP_VAR_UNSET，p1：返回值，p2:容器（各种类型），p3:索引值，p4:索引类型
static zend_never_inline void ZEND_FASTCALL zend_fetch_dimension_address_UNSET(zval *container_ptr, zval *dim, int dim_type OPLINE_DC EXECUTE_DATA_DC)
{
	// 取出 操作码结果变量
	zval *result = EX_VAR(opline->result.var);
	// 像数组一样读取地址，p1：返回值，p2:容器（各种类型），p3:索引值，p4:索引类型，p5:操作类型
	zend_fetch_dimension_address(result, container_ptr, dim, dim_type, BP_VAR_UNSET EXECUTE_DATA_CC);
}

// ing3,（各种类型的）像数组一样读取，p1:返回结果，p2:被读取变量，p3:索引，p4:索引类型，p5:操作类型，
// p6:p1必须是列表（不支持字串），p7:是否慢速（不支持数组/哈希表）
static zend_always_inline void zend_fetch_dimension_address_read(zval *result, zval *container, zval *dim, int dim_type, int type, bool is_list, int slow EXECUTE_DATA_DC)
{
	zval *retval;
	// 情况1：不要慢操作 并且 容器是数组
	// 如果不要慢操作（慢操作里没有考虑数组类型）
	if (!slow) {
		// 如果容器是数组
		if (EXPECTED(Z_TYPE_P(container) == IS_ARRAY)) {
// 尝试当作数组处理
try_array:
			// 像数组一样读写，的内部实现。p1:哈希表，p2:索引，p3:索引类型？，p4:操作类型
			retval = zend_fetch_dimension_address_inner(Z_ARRVAL_P(container), dim, dim_type, type EXECUTE_DATA_CC);
			// 把找到的值 复制给 返回结果 
			ZVAL_COPY_DEREF(result, retval);
			// 结束
			return;
		// 容器是引用类型
		} else if (EXPECTED(Z_TYPE_P(container) == IS_REFERENCE)) {
			// 取得引用目标
			container = Z_REFVAL_P(container);
			// 如果窗口是数组
			if (EXPECTED(Z_TYPE_P(container) == IS_ARRAY)) {
				// 作为数组处理
				goto try_array;
			}
		}
	}
	
	// 情况2 , 如果 不必须是列表 并且 容器类型为字串
	if (!is_list && EXPECTED(Z_TYPE_P(container) == IS_STRING)) {
		// 取出目标字串
		zend_string *str = Z_STR_P(container);
		// 整数索引号
		zend_long offset;

// 尝试字串索引
try_string_offset:
		// 如果索引类型不是整数
		if (UNEXPECTED(Z_TYPE_P(dim) != IS_LONG)) {
			// 根据索引类型操作
			switch (Z_TYPE_P(dim)) {
				// 情况2.1, 字串 索引
				case IS_STRING:
				{
					// 没有尾数据
					bool trailing_data = false;
					// 由于BC原因 ，允许错误 所以 可以在 数字字串的开头做警告
					/* For BC reasons we allow errors so that we can warn on leading numeric string */
					// 整数转成数值，如果结果是整数（结果返回到 offset 里）
					if (IS_LONG == is_numeric_string_ex(Z_STRVAL_P(dim), Z_STRLEN_P(dim), &offset,
							NULL, /* allow errors */ true, NULL, &trailing_data)) {
						// 如果有尾数据
						if (UNEXPECTED(trailing_data)) {
							// 报错：非法的字串索引号
							zend_error(E_WARNING, "Illegal string offset \"%s\"", Z_STRVAL_P(dim));
						}
						// 尝试应用数字索引号
						goto out;
					}
					// 如果操作类型是 isset
					if (type == BP_VAR_IS) {
						// 返回值为 null
						ZVAL_NULL(result);
						// 返回
						return;
					}
					// 报错：不可以用此类型作为 string的偏移量
					zend_illegal_string_offset(dim);
					// 返回值为 null
					ZVAL_NULL(result);
					// 返回
					return;
				}
				// 情况2.2, 未定义 索引
				case IS_UNDEF:
					// 在抛出提示时，数组可能被销毁了，临时增加引用次数来侦测此种情况
					/* The string may be destroyed while throwing the notice.
					 * Temporarily increase the refcount to detect this situation. */
					if (!(GC_FLAGS(str) & IS_STR_INTERNED)) {
						GC_ADDREF(str);
					}
					// _zval_undefined_op2, 检查op2的变量，没有定义则抛异常
					ZVAL_UNDEFINED_OP2();
					if (!(GC_FLAGS(str) & IS_STR_INTERNED) && UNEXPECTED(GC_DELREF(str) == 0)) {
						zend_string_efree(str);
						// 返回值为 null
						ZVAL_NULL(result);
						return;
					}
					// ((void)0)
					ZEND_FALLTHROUGH;
				// 情况2.3, 小数，null，false,true 索引
				case IS_DOUBLE:
				case IS_NULL:
				case IS_FALSE:
				case IS_TRUE:
					if (type != BP_VAR_IS) {
						// 在抛出提示时，数组可能被销毁了，临时增加引用次数来侦测此种情况
						/* The string may be destroyed while throwing the notice.
						 * Temporarily increase the refcount to detect this situation. */
						// 如果不是间接引用
						if (!(GC_FLAGS(str) & IS_STR_INTERNED)) {
							// 增加引用次数
							GC_ADDREF(str);
						}
						// 警告：正在转换成字串索引
						zend_error(E_WARNING, "String offset cast occurred");
						// 如果要读取的字串不是内置字串 并且 减少引用后为0
						if (!(GC_FLAGS(str) & IS_STR_INTERNED) && UNEXPECTED(GC_DELREF(str) == 0)) {
							// 释放此字串
							zend_string_efree(str);
							// 返回值为 null
							ZVAL_NULL(result);
							// 中断
							return;
						}
					}
					break;
				// 情况2.4, 引用类型索引
				case IS_REFERENCE:
					// 解引用
					dim = Z_REFVAL_P(dim);
					// 重新尝试字串索引号
					goto try_string_offset;
				// 情况2.5, 其他类型
				default:
					// 报错：不可以用此类型作为 string的偏移量
					zend_illegal_string_offset(dim);
					// 返回值为 null
					ZVAL_NULL(result);
					return;
			}
			// 从zval里取出整数，p1:zval，p2:是否严格类型
			offset = zval_get_long_func(dim, /* is_strict */ false);
		// 索引类型是整数
		} else {
			// 直接取出整数
			offset = Z_LVAL_P(dim);
		}
		// 尝试应用数字索引号
		out:
		// 如果索引越界： 字串长度 < ( 索引号<0 ? -索引号 ：索引号 +1 )
		// 正着取 索引号+1，倒着取不加
		if (UNEXPECTED(ZSTR_LEN(str) < ((offset < 0) ? -(size_t)offset : ((size_t)offset + 1)))) {
			// 如果操作 不是 isset
			if (type != BP_VAR_IS) {
				// 警告：未初始化的字串 索引位置
				zend_error(E_WARNING, "Uninitialized string offset " ZEND_LONG_FMT, offset);
				// 返回值为 空字串
				ZVAL_EMPTY_STRING(result);
			// 操作是 isset
			} else {
				// 返回值为 null
				ZVAL_NULL(result);
			}
		// 索引位置可用
		} else {
			zend_uchar c;
			zend_long real_offset;
			// 索引位置小于0 ？长度+索引位置 ：索引位置
			real_offset = (UNEXPECTED(offset < 0)) /* Handle negative offset */
				? (zend_long)ZSTR_LEN(str) + offset : offset;
			// 取得此位置的字符
			c = (zend_uchar)ZSTR_VAL(str)[real_offset];
			// 返回此字符，类型为 char
			ZVAL_CHAR(result, c);
		}
	// 情况3, 如果容器是 对象类型
	} else if (EXPECTED(Z_TYPE_P(container) == IS_OBJECT)) {
		// 取得容器中的对象
		zend_object *obj = Z_OBJ_P(container);
		// 对象增加引用
		GC_ADDREF(obj);
		// ZEND_CONST_COND：windows直接取 p2
		if (ZEND_CONST_COND(dim_type == IS_CV, 1) && UNEXPECTED(Z_TYPE_P(dim) == IS_UNDEF)) {
			// _zval_undefined_op2, 检查op2的变量，没有定义则抛异常
			dim = ZVAL_UNDEFINED_OP2();
		}
		// 索引是常量 并且 索引有扩展数据 ZEND_EXTRA_VALUE=1
		if (dim_type == IS_CONST && Z_EXTRA_P(dim) == ZEND_EXTRA_VALUE) {
			// 取得索引的下一个zval（扩展数据）
			dim++;
		}
		// zend_std_read_dimension 像数组一样读取对象
		retval = obj->handlers->read_dimension(obj, dim, type, result);
		// 返回值不可以为 null
		ZEND_ASSERT(result != NULL);
		// 如果有查找到值
		if (retval) {
			// 返回值和查找到的不是同一个 实例
			if (result != retval) {
				// 把查找到的值 复制给 result
				ZVAL_COPY_DEREF(result, retval);
			// 如果 查找到 引用类型
			} else if (UNEXPECTED(Z_ISREF_P(retval))) {
				//  解包引用对象：引用的值赋给当前变量
				zend_unwrap_reference(result);
			}
		// 没有查找到值
		} else {
			// 返回值为 null
			ZVAL_NULL(result);
		}
		// 如果 -1 后引用数为 0
		if (UNEXPECTED(GC_DELREF(obj) == 0)) {
			// 释放指定的对象，并更新 objects_store 中的指针
			zend_objects_store_del(obj);
		}
	// 情况4, 容器是其他类型
	} else {
		// 操作类型不是 isset  并且 容器是未定义类型
		if (type != BP_VAR_IS && UNEXPECTED(Z_TYPE_P(container) == IS_UNDEF)) {
			// _zval_undefined_op1, 检查op1的变量，没有定义则抛异常
			container = ZVAL_UNDEFINED_OP1();
		}
		// 1 并且 索引是未定义
		// ZEND_CONST_COND：windows直接取 p2。 
		if (ZEND_CONST_COND(dim_type == IS_CV, 1) && UNEXPECTED(Z_TYPE_P(dim) == IS_UNDEF)) {
			// _zval_undefined_op2, 检查op2的变量，没有定义则抛异常
			ZVAL_UNDEFINED_OP2();
		}
		// 如果 不必要是列表 并且 操作不是 isset
		if (!is_list && type != BP_VAR_IS) {
			// 警告：尝试把 A 类型的值当成数组访问
			zend_error(E_WARNING, "Trying to access array offset on value of type %s",
				zend_zval_type_name(container));
		}
		// 返回值为 null// 返回空
		ZVAL_NULL(result);
	}
}

// ing3,（各种类型的）像数组一样读取,结果返回到操作码中，操作类型为 BP_VAR_R。 
// p1:被读取变量，p2:索引，p3:索引类型
static zend_never_inline void ZEND_FASTCALL zend_fetch_dimension_address_read_R(zval *container, zval *dim, int dim_type OPLINE_DC EXECUTE_DATA_DC)
{
	// #define EX_VAR(n) ZEND_CALL_VAR(execute_data, n)
	// #define ZEND_CALL_VAR(call, n) ((zval*)(((char*)(call)) + ((int)(n))))
	zval *result = EX_VAR(opline->result.var);
	// （各种类型的）像数组一样读取，p1:返回结果，p2:被读取变量，p3:索引，p4:索引类型，p5:操作类型，
	// p6:p1必须是列表（不支持字串），p7:是否慢速（不支持数组/哈希表）
	zend_fetch_dimension_address_read(result, container, dim, dim_type, BP_VAR_R, 0, 0 EXECUTE_DATA_CC);
}

// ing3,（哈希表以外的）像数组一样读取,结果返回到操作码中，操作类型为 BP_VAR_R。 
// p1:被读取变量，p2:索引，p3:索引类型
static zend_never_inline void zend_fetch_dimension_address_read_R_slow(zval *container, zval *dim OPLINE_DC EXECUTE_DATA_DC)
{
	// #define EX_VAR(n) ZEND_CALL_VAR(execute_data, n)
	// #define ZEND_CALL_VAR(call, n) ((zval*)(((char*)(call)) + ((int)(n))))
	zval *result = EX_VAR(opline->result.var);
	// （各种类型的）像数组一样读取，p1:返回结果，p2:被读取变量，p3:索引，p4:索引类型，p5:操作类型，
	// p6:p1必须是列表（不支持字串），p7:是否慢速（不支持数组/哈希表）
	zend_fetch_dimension_address_read(result, container, dim, IS_CV, BP_VAR_R, 0, 1 EXECUTE_DATA_CC);
}

// ing3,（各种类型的）像数组一样读取,结果返回到操作码中，操作类型为 BP_VAR_IS。 
// p1:被读取变量，p2:索引，p3:索引类型
static zend_never_inline void ZEND_FASTCALL zend_fetch_dimension_address_read_IS(zval *container, zval *dim, int dim_type OPLINE_DC EXECUTE_DATA_DC)
{
	// #define EX_VAR(n) ZEND_CALL_VAR(execute_data, n)
	// #define ZEND_CALL_VAR(call, n) ((zval*)(((char*)(call)) + ((int)(n))))
	zval *result = EX_VAR(opline->result.var);
	// （各种类型的）像数组一样读取，p1:返回结果，p2:被读取变量，p3:索引，p4:索引类型，p5:操作类型，
	// p6:p1必须是列表（不支持字串），p7:是否慢速（不支持数组/哈希表）
	zend_fetch_dimension_address_read(result, container, dim, dim_type, BP_VAR_IS, 0, 0 EXECUTE_DATA_CC);
}

// ing3,（字串以外类型的）像数组一样读取,结果返回到操作码中，操作类型为 BP_VAR_R。 
// p1:被读取变量，p2:索引，p3:索引类型
static zend_never_inline void ZEND_FASTCALL zend_fetch_dimension_address_LIST_r(zval *container, zval *dim, int dim_type OPLINE_DC EXECUTE_DATA_DC)
{
	// #define EX_VAR(n) ZEND_CALL_VAR(execute_data, n)
	// #define ZEND_CALL_VAR(call, n) ((zval*)(((char*)(call)) + ((int)(n))))
	zval *result = EX_VAR(opline->result.var);
	// （各种类型的）像数组一样读取，p1:返回结果，p2:被读取变量，p3:索引，p4:索引类型，p5:操作类型，
	// p6:p1必须是列表（不支持字串），p7:是否慢速（不支持数组/哈希表）
	zend_fetch_dimension_address_read(result, container, dim, dim_type, BP_VAR_R, 1, 0 EXECUTE_DATA_CC);
}

// ing3,（各种类型的）像数组一样读取,结果返回到操作码中。p1:被读取变量，p2:索引，p3:索引类型，p4:操作类型
ZEND_API void zend_fetch_dimension_const(zval *result, zval *container, zval *dim, int type)
{
	// （各种类型的）像数组一样读取，p1:返回结果，p2:被读取变量，p3:索引，p4:索引类型，p5:操作类型，
	// p6:p1必须是列表（不支持字串），p7:是否慢速（不支持数组/哈希表）
	zend_fetch_dimension_address_read(result, container, dim, IS_TMP_VAR, type, 0, 0 NO_EXECUTE_DATA_CC);
}

// ing3, 像数组一样查找哈希表，兼容各种类型的索引号
static zend_never_inline zval* ZEND_FASTCALL zend_find_array_dim_slow(HashTable *ht, zval *offset EXECUTE_DATA_DC)
{
	zend_ulong hval;
	// 偏移量是小数
	if (Z_TYPE_P(offset) == IS_DOUBLE) {
		// 转成整数
		hval = zend_dval_to_lval_safe(Z_DVAL_P(offset));
// 数字索引
num_idx:
		// 哈希表中查找此元素
		return zend_hash_index_find(ht, hval);
	// 如果索引号是 null
	} else if (Z_TYPE_P(offset) == IS_NULL) {
// 字串索引号
str_idx:
		// 哈希表中查找此元素, key为空字串
		return zend_hash_find_known_hash(ht, ZSTR_EMPTY_ALLOC());
	// 如果索引号是 false
	} else if (Z_TYPE_P(offset) == IS_FALSE) {
		// 用整数 0 索引
		hval = 0;
		// 按数字索引操作
		goto num_idx;
	// 如果索引号是 true
	} else if (Z_TYPE_P(offset) == IS_TRUE) {
		// 用整数 1 索引
		hval = 1;
		// 按数字索引操作
		goto num_idx;
	// 如果偏移量是资源类型
	} else if (Z_TYPE_P(offset) == IS_RESOURCE) {
		// 抛出警告：资源作为偏移量，转成了数字（自己的ID）
		zend_use_resource_as_offset(offset);
		// 取得资源id
		hval = Z_RES_HANDLE_P(offset);
		// 按数字索引操作
		goto num_idx;
	// 如果索引 是未定义
	} else if (/*OP2_TYPE == IS_CV &&*/ Z_TYPE_P(offset) == IS_UNDEF) {
		// _zval_undefined_op2, 检查op2的变量，没有定义则抛异常
		ZVAL_UNDEFINED_OP2();
		// 按字串索引号操作
		goto str_idx;
	// 其他情况
	} else {
		// 在isset 或 empty 里使用了非法索引号
		zend_type_error("Illegal offset type in isset or empty");
		// 返回空
		return NULL;
	}
}

// ing3, 检验 对象 或 字串 中 此索引对应的值是否 存在
static zend_never_inline bool ZEND_FASTCALL zend_isset_dim_slow(zval *container, zval *offset EXECUTE_DATA_DC)
{
	// 如果偏移量无效
	if (/*OP2_TYPE == IS_CV &&*/ UNEXPECTED(Z_TYPE_P(offset) == IS_UNDEF)) {
		// _zval_undefined_op2, 检查op2的变量，没有定义则抛异常
		offset = ZVAL_UNDEFINED_OP2();
	}

	// 如果容器是对象
	if (/*OP1_TYPE != IS_CONST &&*/ EXPECTED(Z_TYPE_P(container) == IS_OBJECT)) {
		// zend_std_has_dimension
		// 像数组一样确认 key 是否存在，并按要求检查元素值。p1:对象，p2:索引号，p3:是否检查元素empty
		return Z_OBJ_HT_P(container)->has_dimension(Z_OBJ_P(container), offset, 0);
	// 如果窗口是字串
	} else if (EXPECTED(Z_TYPE_P(container) == IS_STRING)) { /* string offsets */
		zend_long lval;
		// 如果偏移量是整数 
		if (EXPECTED(Z_TYPE_P(offset) == IS_LONG)) {
			// 取出整数
			lval = Z_LVAL_P(offset);
// 按整数处理
str_offset:
			// 如果 小于 0
			if (UNEXPECTED(lval < 0)) { /* Handle negative offset */
				// + 容器字串的长度
				lval += (zend_long)Z_STRLEN_P(container);
			}
			// 如果长度在 容器字串长度以内
			if (EXPECTED(lval >= 0) && (size_t)lval < Z_STRLEN_P(container)) {
				// 返回是
				return 1;
			// 位置溢出
			} else {
				// 返回否
				return 0;
			}
		// 偏移量不是整数
		} else {
			/*if (OP2_TYPE & (IS_CV|IS_VAR)) {*/
				// 如果是引用类型，追踪到被引用的对象
				ZVAL_DEREF(offset);
			/*}*/
			// 如果 offset是字串以下的标量类型 或 （是字串 并且可以转成整数）
			if (Z_TYPE_P(offset) < IS_STRING /* simple scalar types */
					|| (Z_TYPE_P(offset) == IS_STRING /* or numeric string */
						&& IS_LONG == is_numeric_string(Z_STRVAL_P(offset), Z_STRLEN_P(offset), NULL, NULL, 0))) {
				// 把偏移量转成整数
				lval = zval_get_long_ex(offset, /* is_strict */ true);
				// 按整数处理
				goto str_offset;
			}
			// 反回 否
			return 0;
		}
	} else {
		// 反回 否
		return 0;
	}
}

// ing3, 检验 对象 或 字串 中 此索引对应的值是否是 empty
static zend_never_inline bool ZEND_FASTCALL zend_isempty_dim_slow(zval *container, zval *offset EXECUTE_DATA_DC)
{
	// 如果偏移量无效
	if (/*OP2_TYPE == IS_CV &&*/ UNEXPECTED(Z_TYPE_P(offset) == IS_UNDEF)) {
		// _zval_undefined_op2, 检查op2的变量，没有定义则抛异常
		offset = ZVAL_UNDEFINED_OP2();
	}
	// 如果容器是对象
	if (/*OP1_TYPE != IS_CONST &&*/ EXPECTED(Z_TYPE_P(container) == IS_OBJECT)) {
		// zend_std_has_dimension 
		// ! 像数组一样确认 key 是否存在，并按要求检查元素值。p1:对象，p2:索引号，p3:是否检查元素empty
		return !Z_OBJ_HT_P(container)->has_dimension(Z_OBJ_P(container), offset, 1);
	// 如果容器是字串
	} else if (EXPECTED(Z_TYPE_P(container) == IS_STRING)) { /* string offsets */
		zend_long lval;
		// 如果偏移量是整数 
		if (EXPECTED(Z_TYPE_P(offset) == IS_LONG)) {
			// 取出整数
			lval = Z_LVAL_P(offset);
// 按整数处理
str_offset:
			// 如果 小于 0
			if (UNEXPECTED(lval < 0)) { /* Handle negative offset */
				// + 容器字串的长度
				lval += (zend_long)Z_STRLEN_P(container);
			}
			// 如果长度在 容器字串长度以内
			if (EXPECTED(lval >= 0) && (size_t)lval < Z_STRLEN_P(container)) {
				// 返回此字符是否是 '0'
				return (Z_STRVAL_P(container)[lval] == '0');
			// 其他情况
			} else {
				// 返回 是
				return 1;
			}
		// 偏移量不是整数
		} else {
			/*if (OP2_TYPE & (IS_CV|IS_VAR)) {*/
				// 如果是引用类型，追踪到被引用的对象
				ZVAL_DEREF(offset);
			/*}*/
			// 如果 offset是字串以下的标量类型 或 （是字串 并且可以转成整数）
			if (Z_TYPE_P(offset) < IS_STRING /* simple scalar types */
					|| (Z_TYPE_P(offset) == IS_STRING /* or numeric string */
						&& IS_LONG == is_numeric_string(Z_STRVAL_P(offset), Z_STRLEN_P(offset), NULL, NULL, 0))) {
				// 把偏移量转成整数
				lval = zval_get_long_ex(offset, /* is_strict */ true);
				// 按整数处理
				goto str_offset;
			}
			// 反回 是
			return 1;
		}
	// 容器不是字串和对象
	} else {
		// 反回 是
		return 1;
	}
}

// ing3, 检查哈希表里是否存在这个键，p1:哈希表，p2:key
static zend_never_inline bool ZEND_FASTCALL zend_array_key_exists_fast(HashTable *ht, zval *key OPLINE_DC EXECUTE_DATA_DC)
{
	zend_string *str;
	zend_ulong hval;

try_again:
	// 如果key是字串
	if (EXPECTED(Z_TYPE_P(key) == IS_STRING)) {
		// 找到字串
		str = Z_STR_P(key);
		// 如果字串可以转成数字
		if (ZEND_HANDLE_NUMERIC(str, hval)) {
			// 当作数字处理
			goto num_key;
		}
str_key:
		// 不能当成数字，按字串处理
		// 返回：检查哈希表里是否存在这个键
		return zend_hash_exists(ht, str);
	// 如果key是整数
	} else if (EXPECTED(Z_TYPE_P(key) == IS_LONG)) {
		// 取出整数
		hval = Z_LVAL_P(key);
num_key:
		// 返回：检查是否存在
		return zend_hash_index_exists(ht, hval);
	// 如果key是引用类型
	} else if (EXPECTED(Z_ISREF_P(key))) {
		// 解引用并从头开始
		key = Z_REFVAL_P(key);
		goto try_again;
	// 如果key是小数
	} else if (Z_TYPE_P(key) == IS_DOUBLE) {
		// 小数转成整数
		hval = zend_dval_to_lval_safe(Z_DVAL_P(key));
		// 按整数处理
		goto num_key;
	// key是 false
	} else if (Z_TYPE_P(key) == IS_FALSE) {
		// 按整数 0处理
		hval = 0;
		goto num_key;
	// key是 true
	} else if (Z_TYPE_P(key) == IS_TRUE) {
		// 按整数 1处理
		hval = 1;
		goto num_key;
	// key是资源
	} else if (Z_TYPE_P(key) == IS_RESOURCE) {
		// 抛出警告：资源作为偏移量，转成了数字（自己的ID）
		zend_use_resource_as_offset(key);
		// 使用自己的ID
		hval = Z_RES_HANDLE_P(key);
		goto num_key;
	// key 是 未定义
	} else if (Z_TYPE_P(key) <= IS_NULL) {
		// 如果是未定义
		if (UNEXPECTED(Z_TYPE_P(key) == IS_UNDEF)) {
			// _zval_undefined_op1, 检查op1的变量，没有定义则抛异常
			ZVAL_UNDEFINED_OP1();
		}
		// 按空字串处理
		str = ZSTR_EMPTY_ALLOC();
		goto str_key;
	// 其他情况
	} else {
		// 非法的偏移量
		zend_illegal_offset();
		// 返回否
		return 0;
	}
}

// ing4, 报错：array_key_exists 函数错误
static ZEND_COLD void ZEND_FASTCALL zend_array_key_exists_error(
		zval *subject, zval *key OPLINE_DC EXECUTE_DATA_DC)
{
	// 如果key未定义
	if (Z_TYPE_P(key) == IS_UNDEF) {
		// _zval_undefined_op1, 检查op1的变量，没有定义则抛异常
		ZVAL_UNDEFINED_OP1();
	}
	// 如果 subject 未定义
	if (Z_TYPE_P(subject) == IS_UNDEF) {
		// _zval_undefined_op2, 检查op2的变量，没有定义则抛异常
		ZVAL_UNDEFINED_OP2();
	}
	// 如果两个都有定义，并且没有异常
	if (!EG(exception)) {
		// array_key_exists() 第二个参数必须是数组 ，但给出了 A 类型
		zend_type_error("array_key_exists(): Argument #2 ($array) must be of type array, %s given",
			zend_zval_type_name(subject));
	}
}

// ing4, 验证值为 false,null 或 未定义（解引用）
static zend_always_inline bool promotes_to_array(zval *val) {
	// 值为 false,null 或 未定义 或 （是引用类型 并且目标值 为 false,null 或 未定义）
	return Z_TYPE_P(val) <= IS_FALSE
		|| (Z_ISREF_P(val) && Z_TYPE_P(Z_REFVAL_P(val)) <= IS_FALSE);
}

// ing4, 检验类型 存在 或 兼容数组（MAY_BE_ARRAY）
static zend_always_inline bool check_type_array_assignable(zend_type type) {
	// 如果类型有效
	if (!ZEND_TYPE_IS_SET(type)) {
		// 返回是
		return 1;
	}
	// 否则返回：是否兼容 数组
	return (ZEND_TYPE_FULL_MASK(type) & MAY_BE_ARRAY) != 0;
}

// ing3, 检验数组是否可以被 赋值给引用实例。如果不可以，报错
/* Checks whether an array can be assigned to the reference. Throws error if not assignable. */
ZEND_API bool zend_verify_ref_array_assignable(zend_reference *ref) {
	zend_property_info *prop;
	
	// 检验引用目标是否有效 ((p1)->sources).ptr != NULL
	ZEND_ASSERT(ZEND_REF_HAS_TYPE_SOURCES(ref));
	// 追踪到到引用的 属性信息列表，并遍历它（开始段）
	ZEND_REF_FOREACH_TYPE_SOURCES(ref, prop) {
		// 检验类型 存在 或 兼容数组（MAY_BE_ARRAY），如果检验失败
		if (!check_type_array_assignable(prop->type)) {
			// 报错：无法自动初始化 属性 中 的引用类型 中 的数组
			zend_throw_auto_init_in_ref_error(prop);
			// 返回 否
			return 0;
		}
	} ZEND_REF_FOREACH_TYPE_SOURCES_END();
	// 返回 是
	return 1;
}

// ing4, 验证 属性有效（在属性表中） 并 获取它的属性信息，p1:对象，p2:属性zval
static zend_property_info *zend_object_fetch_property_type_info(
		zend_object *obj, zval *slot)
{
	// 如果没有类入口，返回null
	if (EXPECTED(!ZEND_CLASS_HAS_TYPE_HINTS(obj->ce))) {
		return NULL;
	}

	// 不是一个已定义的属性，返回null
	/* Not a declared property */
	// 如果指向位置不在属性表中 
	if (UNEXPECTED(slot < obj->properties_table ||
			slot >= obj->properties_table + obj->ce->default_properties_count)) {
		// 返回空
		return NULL;
	}

	// 获取指定属性的属性信息，p1:对象，p2:属性zval
	return zend_get_typed_property_info_for_slot(obj, slot);
}

// ing3, 验证操作是否可行，p1:返回错误信息，p2:属性zval，p3:对象，p4:返回属性信息，p5:操作类型
static zend_never_inline bool zend_handle_fetch_obj_flags(
		zval *result, zval *ptr, zend_object *obj, zend_property_info *prop_info, uint32_t flags)
{
	// 根据标记操作
	switch (flags) {
		// 像数组一样写入
		case ZEND_FETCH_DIM_WRITE:
			// 如果：验证值为 false,null 或 未定义（解引用）
			if (promotes_to_array(ptr)) {
				// 如果无属性信息
				if (!prop_info) {
					// 验证 属性有效（在属性表中） 并 获取它的属性信息，p1:对象，p2:属性zval
					prop_info = zend_object_fetch_property_type_info(obj, ptr);
					// 如果获取属性信息失败
					if (!prop_info) {
						// 跳出
						break;
					}
				}
				// 检验类型 存在 或 兼容数组（MAY_BE_ARRAY）
				if (!check_type_array_assignable(prop_info->type)) {
					// 报错：自动初始化属性错误
					zend_throw_auto_init_in_prop_error(prop_info);
					// 如果有接收结果，结果类型为错误
					if (result) ZVAL_ERROR(result);
					// 返回 否
					return 0;
				}
			}
			// 跳出
			break;
		// 获取引用结果 
		case ZEND_FETCH_REF:
			// 如果值类型不是引用
			if (Z_TYPE_P(ptr) != IS_REFERENCE) {
				// 如果属性信息无效
				if (!prop_info) {
					// 验证 属性有效（在属性表中） 并 获取它的属性信息，p1:对象，p2:属性zval
					prop_info = zend_object_fetch_property_type_info(obj, ptr);
					// 如果验证失败
					if (!prop_info) {
						// 跳出
						break;
					}
				}
				// 如果值无效
				if (Z_TYPE_P(ptr) == IS_UNDEF) {
					// 检查是否可以是 null，如果不可以
					if (!ZEND_TYPE_ALLOW_NULL(prop_info->type)) {
						// 报错：通过引用访问未初始化的属性
						zend_throw_access_uninit_prop_by_ref_error(prop_info);
						// 如果有接收结果，结果类型为错误
						if (result) ZVAL_ERROR(result);
						// 返回 否
						return 0;
					}
					// 值更新为 null
					ZVAL_NULL(ptr);
				}
				// 值更新为 引用类型
				ZVAL_NEW_REF(ptr, ptr);
				// 向zend_property_info_source_list指向的 zend_property_info 列表中添加 zend_property_info
				ZEND_REF_ADD_TYPE_SOURCE(Z_REF_P(ptr), prop_info);
			}
			break;
		EMPTY_SWITCH_DEFAULT_CASE()
	}
	return 1;
}

// 140行
// ing3, 获取属性的地址。p1:返回结果，p2:容器zval，p3:容器运算对象类型，p4:属性值，p5:属性运算对象类型
// p6:缓存位置，p7:操作类型，p8:flags，p9:是否初始化未定义类型
static zend_always_inline void zend_fetch_property_address(zval *result, zval *container, uint32_t container_op_type, zval *prop_ptr, uint32_t prop_op_type, void **cache_slot, int type, uint32_t flags, bool init_undef OPLINE_DC EXECUTE_DATA_DC)
{
	zval *ptr;
	zend_object *zobj;
	zend_string *name, *tmp_name;
	// 情况1：容器运算对象类型 不是未定义 并且 容器不是对象
	if (container_op_type != IS_UNUSED && UNEXPECTED(Z_TYPE_P(container) != IS_OBJECT)) {
		do {
			// 如果容器是引用类型，并且解引用后是对象
			if (Z_ISREF_P(container) && Z_TYPE_P(Z_REFVAL_P(container)) == IS_OBJECT) {
				// 解引用
				container = Z_REFVAL_P(container);
				// 跳出wihle
				break;
			}

			// 如果容器操作对象是编译变量 并且 操作类型不是创建 并且 容器是未定义
			if (container_op_type == IS_CV
			 && type != BP_VAR_W
			 && UNEXPECTED(Z_TYPE_P(container) == IS_UNDEF)) {
				 // _zval_undefined_op1, 检查op1的变量，没有定义则抛异常
				ZVAL_UNDEFINED_OP1();
			}

			// 只有对象为空时才修改它
			/* this should modify object only if it's empty */
			// 如果操作是删除
			if (type == BP_VAR_UNSET) {
				// 返回值为 null
				ZVAL_NULL(result);
				// 直接返回
				return;
			}
			// 抛出【对象无效】异常
			zend_throw_non_object_error(container, prop_ptr OPLINE_CC EXECUTE_DATA_CC);
			// 把 zval 标记成 错误 类型
			ZVAL_ERROR(result);
			return;
		} while (0);
	}

	// 取出容器中的对象
	zobj = Z_OBJ_P(container);
	// 情况2：属性运算对象类型是 常量 并且 对象所属类 并且
	if (prop_op_type == IS_CONST &&
		// 如果对象的所属类和缓存的第一个元素相同
		// 返回数组的第一个元素
	    EXPECTED(zobj->ce == CACHED_PTR_EX(cache_slot))) {
		// 找到缓存的下一个位置
		uintptr_t prop_offset = (uintptr_t)CACHED_PTR_EX(cache_slot + 1);
		// 如果缓存位置有效
		if (EXPECTED(IS_VALID_PROPERTY_OFFSET(prop_offset))) {
			// 取出这个位置的属性
			ptr = OBJ_PROP(zobj, prop_offset);
			// 如果它不是 未定义
			if (EXPECTED(Z_TYPE_P(ptr) != IS_UNDEF)) {
				// 用间接引用方式 赋值给结果
				ZVAL_INDIRECT(result, ptr);
				// 返回数组的第一个元素
				zend_property_info *prop_info = CACHED_PTR_EX(cache_slot + 2);
				// 如果属性信息有效
				if (prop_info) {
					// 如果是只读属性
					if (UNEXPECTED(prop_info->flags & ZEND_ACC_READONLY)) {
						// 对于对象，创建/更新/删除 操作模式 并不会真的改变对象
						// 类似魔术方法 __get 所允许的，但返回值的副本来保证 不会发生真实的修改
						/* For objects, W/RW/UNSET fetch modes might not actually modify object.
						 * Similar as with magic __get() allow them, but return the value as a copy
						 * to make sure no actual modification is possible. */
						 
						// 创建/更新/删除 操作模式
						ZEND_ASSERT(type == BP_VAR_W || type == BP_VAR_RW || type == BP_VAR_UNSET);
						// 如果目标是对象
						if (Z_TYPE_P(ptr) == IS_OBJECT) {
							// 把它复制到结果里
							ZVAL_COPY(result, ptr);
						// 目标不是对象
						} else {
							// 报错：不能修改只读属性
							zend_readonly_property_modification_error(prop_info);
							// 结果类型标记成 错误
							ZVAL_ERROR(result);
						}
						// 完毕
						return;
					}
					// 检验标记：从对象中获取
					flags &= ZEND_FETCH_OBJ_FLAGS;
					// 如果有这个标记
					if (flags) {
						//  验证操作是否可行，p1:返回错误信息，p2:属性zval，p3:对象，p4:返回属性信息，p5:操作类型
						zend_handle_fetch_obj_flags(result, ptr, NULL, prop_info, flags);
					}
				}
				// 完毕
				return;
			}
		// 如果附加属性表有效
		} else if (EXPECTED(zobj->properties != NULL)) {
			// 附加属性表引用次数>1
			if (UNEXPECTED(GC_REFCOUNT(zobj->properties) > 1)) {
				// 附加属性表不是 不可更改
				if (EXPECTED(!(GC_FLAGS(zobj->properties) & IS_ARRAY_IMMUTABLE))) {
					// 减少引用次数
					GC_DELREF(zobj->properties);
				}
				// 附加属性表创建副本
				zobj->properties = zend_array_dup(zobj->properties);
			}
			// 查找此属性
			ptr = zend_hash_find_known_hash(zobj->properties, Z_STR_P(prop_ptr));
			// 如果找到
			if (EXPECTED(ptr)) {
				// 间接引用 到返回结果
				ZVAL_INDIRECT(result, ptr);
				// 返回
				return;
			}
		}
	}

	// 如果属性 运算对象 类型是常量
	if (prop_op_type == IS_CONST) {
		// 取得属性中的字串，作为名称
		name = Z_STR_P(prop_ptr);
		
	// 属性 运算对象 类型不是常量
	} else {
		// 运算对象转成临时字串（本身是字串则返回字串指针），p1:运算对象，p2:接收返回字串（p1字串时返回null）
		name = zval_get_tmp_string(prop_ptr, &tmp_name);
	}
	
	// 取得属性指针的指针，p1:对象，p2:属性名，p3:操作类型，p4:缓存位置
	ptr = zobj->handlers->get_property_ptr_ptr(zobj, name, type, cache_slot);
	// 情况3：如果没取到指针
	if (NULL == ptr) {
		// zend_std_read_property 读取属性， rv是返回值的副本。p1:对象，p2:属性名，p3:类型，p4:缓存位置，p5:返回值
		ptr = zobj->handlers->read_property(zobj, name, type, cache_slot, result);
		// 如果两个返回值相同
		if (ptr == result) {
			// 如果 ptr是引用类型 并且引用次数是1
			if (UNEXPECTED(Z_ISREF_P(ptr) && Z_REFCOUNT_P(ptr) == 1)) {
				// 赋值为 未定义
				ZVAL_UNREF(ptr);
			}
			// 清理变量，完毕
			goto end;
		}
		// 如果有异常
		if (UNEXPECTED(EG(exception))) {
			// 结果类型为错误
			ZVAL_ERROR(result);
			// 清理变量，完毕
			goto end;
		}
	// 有返回结果 ，但结果是错误
	} else if (UNEXPECTED(Z_ISERROR_P(ptr))) {
		// 结果类型为错误
		ZVAL_ERROR(result);
		// 清理变量，完毕
		goto end;
	}
	// 间接引用 赋值给 返回值
	ZVAL_INDIRECT(result, ptr);
	// 只保留 【从对象中获取】 标记
	flags &= ZEND_FETCH_OBJ_FLAGS;
	
	// 情况4：flags有效
	if (flags) {
		// 接收返回值
		zend_property_info *prop_info;
		// 如果 属性运算对象 为常量
		if (prop_op_type == IS_CONST) {
			// 返回数组的第一个元素
			prop_info = CACHED_PTR_EX(cache_slot + 2);
			// 如果获取到属性信息 
			if (prop_info) {
				//  验证操作是否可行，p1:返回错误信息，p2:属性zval，p3:对象，p4:返回属性信息，p5:操作类型
				if (UNEXPECTED(!zend_handle_fetch_obj_flags(result, ptr, NULL, prop_info, flags))) {
					// 清理变量，完毕
					goto end;
				}
			}
		// 属性运算对象 不是常量
		} else {
			//  验证操作是否可行，p1:返回错误信息，p2:属性zval，p3:对象，p4:返回属性信息，p5:操作类型
			if (UNEXPECTED(!zend_handle_fetch_obj_flags(result, ptr, Z_OBJ_P(container), NULL, flags))) {
				// 清理变量，完毕
				goto end;
			}
		}
	}
	// 如果需要初始化未定义 并且 ptr是未定义
	if (init_undef && UNEXPECTED(Z_TYPE_P(ptr) == IS_UNDEF)) {
		// ptr为null
		ZVAL_NULL(ptr);
	}
// 清理变量，完毕
end:
	// 如果属性 运算对象不是常量
	if (prop_op_type != IS_CONST) {
		// 释放变量名
		zend_tmp_string_release(tmp_name);
	}
}

// ing3, 对属性进行引用赋值，p1:属性容器，p2:容器运算对象类型，p3:属性指针，p4:属性运算对象类型，p5:新值
static zend_always_inline void zend_assign_to_property_reference(zval *container, uint32_t container_op_type, zval *prop_ptr, uint32_t prop_op_type, zval *value_ptr OPLINE_DC EXECUTE_DATA_DC)
{
	// 
	zval variable, *variable_ptr = &variable;
	// 属性运算对象是 常量 ？通过偏移量 在运行时缓存中获取一个 (void**) ： null
	void **cache_addr = (prop_op_type == IS_CONST) ? CACHE_ADDR(opline->extended_value & ~ZEND_RETURNS_FUNCTION) : NULL;

	// 获取属性的地址。p1:返回结果，p2:容器zval，p3:容器运算对象类型，p4:属性值，p5:属性运算对象类型
	// p6:操作类型，p7:flags，p8:是否初始化未定义类型
	zend_fetch_property_address(variable_ptr, container, container_op_type, prop_ptr, prop_op_type,
		cache_addr, BP_VAR_W, 0, 0 OPLINE_CC EXECUTE_DATA_CC);

	// 如果 变量指针是间接引用
	if (EXPECTED(Z_TYPE_P(variable_ptr) == IS_INDIRECT)) {
		// 追踪到目标
		variable_ptr = Z_INDIRECT_P(variable_ptr);
		// 如果扩展操作是 ZEND_RETURNS_FUNCTION 并且 新值不是引用类型
		if (/*OP_DATA_TYPE == IS_VAR &&*/
				   (opline->extended_value & ZEND_RETURNS_FUNCTION) &&
				   UNEXPECTED(!Z_ISREF_P(value_ptr))) {
			//  给引用类型 赋值出错，有错误提示信息，但还是进行赋值了
			variable_ptr = zend_wrong_assign_to_variable_reference(
				variable_ptr, value_ptr OPLINE_CC EXECUTE_DATA_CC);
		} else {
			// 属性信息
			zend_property_info *prop_info = NULL;

			// 如果属性运算对象是 常量
			if (prop_op_type == IS_CONST) {
				// 返回数组的第一个元素，转成属性信息指针
				prop_info = (zend_property_info *) CACHED_PTR_EX(cache_addr + 2);
			// 属性运算对象不是常量
			} else {
				// 如果是引用类型，追踪到被引用的对象
				ZVAL_DEREF(container);
				// 验证 属性有效（在属性表中） 并 获取它的属性信息，p1:对象，p2:属性zval
				prop_info = zend_object_fetch_property_type_info(Z_OBJ_P(container), variable_ptr);
			}
			
			// 如果属性信息有效
			if (UNEXPECTED(prop_info)) {
				// 对有类型的属性进行引用赋值，p1:属性信息，p2:属性实例，p3:新值
				variable_ptr = zend_assign_to_typed_property_reference(prop_info, variable_ptr, value_ptr EXECUTE_DATA_CC);
			// 属性信息无效
			} else {
				// 对变量进行引用赋值
				zend_assign_to_variable_reference(variable_ptr, value_ptr);
			}
		}
	// 如果变量指针是 错误
	} else if (Z_ISERROR_P(variable_ptr)) {
		// 变量是未初始化 zval
		variable_ptr = &EG(uninitialized_zval);
	// 其他情况
	} else {
		// 抛错：无法对超载对象进行引用赋值
		zend_throw_error(NULL, "Cannot assign by reference to overloaded object");
		// 销毁没有引用次数的对象，有引用次数的添加到gc回收周期里。并没有销毁 zval本身。
		zval_ptr_dtor(&variable);
		// 变量是未初始化 zval
		variable_ptr = &EG(uninitialized_zval);
	}

	// ing4, 检验：opline 返回类型不是IS_UNUSED
	if (UNEXPECTED(RETURN_VALUE_USED(opline))) {
		// 把变量复制给操作码 运行结果
		// #define EX_VAR(n) ZEND_CALL_VAR(execute_data, n)
		// #define ZEND_CALL_VAR(call, n) ((zval*)(((char*)(call)) + ((int)(n))))
		ZVAL_COPY(EX_VAR(opline->result.var), variable_ptr);
	}
}

// ing3, 对属性进行引用赋值，容器运算对象类型为未使用，属性运算对象类型为常量
// p1:属性容器，p2:属性指针，p3:新值
static zend_never_inline void zend_assign_to_property_reference_this_const(zval *container, zval *prop_ptr, zval *value_ptr OPLINE_DC EXECUTE_DATA_DC)
{
	// 对属性进行引用赋值，p1:属性容器，p2:容器运算对象类型，p3:属性指针，p4:属性运算对象类型，p5:新值
	zend_assign_to_property_reference(container, IS_UNUSED, prop_ptr, IS_CONST, value_ptr
		OPLINE_CC EXECUTE_DATA_CC);
}

// ing3, 对属性进行引用赋值，容器运算对象类型为 变量，属性运算对象类型为常量
// p1:属性容器，p2:属性指针，p3:新值
static zend_never_inline void zend_assign_to_property_reference_var_const(zval *container, zval *prop_ptr, zval *value_ptr OPLINE_DC EXECUTE_DATA_DC)
{
	// 对属性进行引用赋值，p1:属性容器，p2:容器运算对象类型，p3:属性指针，p4:属性运算对象类型，p5:新值
	zend_assign_to_property_reference(container, IS_VAR, prop_ptr, IS_CONST, value_ptr
		OPLINE_CC EXECUTE_DATA_CC);
}

// ing3, 对属性进行引用赋值，容器运算对象类型为 未使用，属性运算对象类型为变量
// p1:属性容器，p2:属性指针，p3:新值
static zend_never_inline void zend_assign_to_property_reference_this_var(zval *container, zval *prop_ptr, zval *value_ptr OPLINE_DC EXECUTE_DATA_DC)
{
	// 对属性进行引用赋值，p1:属性容器，p2:容器运算对象类型，p3:属性指针，p4:属性运算对象类型，p5:新值
	zend_assign_to_property_reference(container, IS_UNUSED, prop_ptr, IS_VAR, value_ptr
		OPLINE_CC EXECUTE_DATA_CC);
}

// ing3, 对属性进行引用赋值，容器运算对象类型为 变量，属性运算对象类型为变量
// p1:属性容器，p2:属性指针，p3:新值
static zend_never_inline void zend_assign_to_property_reference_var_var(zval *container, zval *prop_ptr, zval *value_ptr OPLINE_DC EXECUTE_DATA_DC)
{
	// 对属性进行引用赋值，p1:属性容器，p2:容器运算对象类型，p3:属性指针，p4:属性运算对象类型，p5:新值
	zend_assign_to_property_reference(container, IS_VAR, prop_ptr, IS_VAR, value_ptr
		OPLINE_CC EXECUTE_DATA_CC);
}

// ing3, 通过缓存位置和类型，取得静态属性地址。p1:返回属性值，p2:返回属性信息，p3:缓存位置，p4:类型（?）
static zend_never_inline zend_result zend_fetch_static_property_address_ex(zval **retval, zend_property_info **prop_info, uint32_t cache_slot, int fetch_type OPLINE_DC EXECUTE_DATA_DC) {
	zend_string *name;
	zend_class_entry *ce;
	zend_property_info *property_info;

	zend_uchar op1_type = opline->op1_type, op2_type = opline->op2_type;

	// 运算对象2 的类型是 常量
	if (EXPECTED(op2_type == IS_CONST)) {
		// 在p1中通过偏移量p2.constant 获取 zval 指针
		zval *class_name = RT_CONSTANT(opline, opline->op2);
		// 如果运算对象1 不是常量 或 缓存位置为空
		// CACHED_PTR: 通过偏移量 在运行时缓存中获取一个 (void**) 中的第一个元素
		ZEND_ASSERT(op1_type != IS_CONST || CACHED_PTR(cache_slot) == NULL);
		
		// CACHED_PTR: 通过偏移量 在运行时缓存中获取一个 (void**) 中的第一个元素
		if (EXPECTED((ce = CACHED_PTR(cache_slot)) == NULL)) {
			// 获取类，找不到会报错。p1:类名，p2:key，p3:flags
			ce = zend_fetch_class_by_name(Z_STR_P(class_name), Z_STR_P(class_name + 1), ZEND_FETCH_CLASS_DEFAULT | ZEND_FETCH_CLASS_EXCEPTION);
			// 如果找不到类
			if (UNEXPECTED(ce == NULL)) {
				// 释放（变量 和 临时变量 类型的）运算对象（zval）：p1:类型，p2:编号
				FREE_OP(op1_type, opline->op1.var);
				// 返回失败
				return FAILURE;
			}
			// 如果运算对象1 类型不是 常量
			if (UNEXPECTED(op1_type != IS_CONST)) {
				// 通过偏移量 在 EX(run_time_cache) 中获取一个 (void**) 中的第一个元素，并给它赋值
				CACHE_PTR(cache_slot, ce);
			}
		}
	// 运算对象2 的类型不是 常量
	} else {
		// 如果运算对象2 的 类型不是 未使用
		if (EXPECTED(op2_type == IS_UNUSED)) {
			// 通过类名或类型查找类，p1:类名，p2:类型
			ce = zend_fetch_class(NULL, opline->op2.num);
			// 如果没获取到类
			if (UNEXPECTED(ce == NULL)) {
				// 释放（变量 和 临时变量 类型的）运算对象（zval）：p1:类型，p2:编号
				FREE_OP(op1_type, opline->op1.var);
				// 返回失败
				return FAILURE;
			}
		} else {
			// 找到变量中的类入口
			// #define EX_VAR(n) ZEND_CALL_VAR(execute_data, n)
			// #define ZEND_CALL_VAR(call, n) ((zval*)(((char*)(call)) + ((int)(n))))
			ce = Z_CE_P(EX_VAR(opline->op2.var));
		}
		
		// 如果运算对象1 类型是常量 并且 缓存位置的正是这个类
		// CACHED_PTR: 通过偏移量 在运行时缓存中获取一个 (void**) 中的第一个元素
		if (EXPECTED(op1_type == IS_CONST) && EXPECTED(CACHED_PTR(cache_slot) == ce)) {
			// 返回值：缓存位置后的第 1个指针
			*retval = CACHED_PTR(cache_slot + sizeof(void *));
			// 返回属性信息：缓存位置后的第 2个指针
			*prop_info = CACHED_PTR(cache_slot + sizeof(void *) * 2);
			// 返回成功
			return SUCCESS;
		}
	}

	// 如果运算对象1 类型是 常量
	if (EXPECTED(op1_type == IS_CONST)) {
		// 在p1中通过偏移量p2.constant 获取 zval 指针
		name = Z_STR_P(RT_CONSTANT(opline, opline->op1));
		// 标准方法：获取静态成员 和 成员信息。p1:类，p2:属性名，p3:类型，p4:属性信息列表指针
		*retval = zend_std_get_static_property_with_info(ce, name, fetch_type, &property_info);
	} else {
		zend_string *tmp_name;
		// ing4, 在执行数据中通过 p1:运算对象类型，p3:操作类型，取回p2:运算对象 指向的zval（编译变量无附加处理）
		zval *varname = get_zval_ptr_undef(opline->op1_type, opline->op1, BP_VAR_R);
		// 如果变量名是字串
		if (EXPECTED(Z_TYPE_P(varname) == IS_STRING)) {
			// 取出字串
			name = Z_STR_P(varname);
			// 临时名称为 null
			tmp_name = NULL;
		// 变量名不是字串
		} else {
			// 运算对象1 类型是编译变量 并且 变量名是 未定义
			if (op1_type == IS_CV && UNEXPECTED(Z_TYPE_P(varname) == IS_UNDEF)) {
				// ing4, 报错：未定义的变量（变量名）, 并返未初始化zval, EXECUTE_DATA_DC 传入也用不到
				zval_undefined_cv(opline->op1.var EXECUTE_DATA_CC);
			}
			// 运算对象转成临时字串（本身是字串则返回字串指针），p1:运算对象，p2:接收返回字串（p1字串时返回null）
			name = zval_get_tmp_string(varname, &tmp_name);
		}
		
		// 标准方法：获取静态成员 和 成员信息。p1:类，p2:属性名，p3:类型，p4:属性信息列表指针
		*retval = zend_std_get_static_property_with_info(ce, name, fetch_type, &property_info);

		// 如果运算对象1 类型是 常量
		if (UNEXPECTED(op1_type != IS_CONST)) {
			// 释放临时名称
			zend_tmp_string_release(tmp_name);
			// 释放（变量 和 临时变量 类型的）运算对象（zval）：p1:类型，p2:编号
			FREE_OP(op1_type, opline->op1.var);
		}
	}

	// 如果返回值是 null
	if (UNEXPECTED(*retval == NULL)) {
		// 返回失败
		return FAILURE;
	}

	// 属性信息
	*prop_info = property_info;

	// 如果运算对象1 是常量 并且 属性属于 trait
	if (EXPECTED(op1_type == IS_CONST)
			&& EXPECTED(!(property_info->ce->ce_flags & ZEND_ACC_TRAIT))) {
		// 找到运行时缓存的 p1 个位置，把p2 和p3依次保存进去
		CACHE_POLYMORPHIC_PTR(cache_slot, ce, *retval);
		// 通过偏移量 在 EX(run_time_cache) 中获取一个 (void**) 中的第一个元素，并给它赋值
		CACHE_PTR(cache_slot + sizeof(void *) * 2, property_info);
	}

	// 返回成功
	return SUCCESS;
}


// ing3, 通过缓存位置和类型，返回静态属性地址。p1:返回属性值，p2:返回属性信息，p3:缓存位置，p4:获取类型，p5:falgs
static zend_always_inline zend_result zend_fetch_static_property_address(zval **retval, zend_property_info **prop_info, uint32_t cache_slot, int fetch_type, int flags OPLINE_DC EXECUTE_DATA_DC) {
	// 属性信息
	zend_property_info *property_info;

	// CACHED_PTR: 通过偏移量 在运行时缓存中获取一个 (void**) 中的第一个元素
	if (opline->op1_type == IS_CONST && (opline->op2_type == IS_CONST || (opline->op2_type == IS_UNUSED && (opline->op2.num == ZEND_FETCH_CLASS_SELF || opline->op2.num == ZEND_FETCH_CLASS_PARENT))) && EXPECTED(CACHED_PTR(cache_slot) != NULL)) {
		// 从运行时缓存里取出 属性值。并返回
		*retval = CACHED_PTR(cache_slot + sizeof(void *));
		// 从运行时缓存里取出 返回值
		property_info = CACHED_PTR(cache_slot + sizeof(void *) * 2);
		// 如果 （获取类型为 读取 或 更新） 并且 属性值是 未定义 并且 属性信息类型存在
		if ((fetch_type == BP_VAR_R || fetch_type == BP_VAR_RW)
				&& UNEXPECTED(Z_TYPE_P(*retval) == IS_UNDEF)
				&& UNEXPECTED(ZEND_TYPE_IS_SET(property_info->type))) {
			// 报错：有类型的静态属性不可以在初始化前访问
			zend_throw_error(NULL, "Typed static property %s::$%s must not be accessed before initialization",
				ZSTR_VAL(property_info->ce->name),
				zend_get_unmangled_property_name(property_info->name));
			// 返回失败
			return FAILURE;
		}
	} else {
		zend_result success;
		// 取得静态属性地址。p1:返回属性值，p2:返回属性信息，p3:缓存位置，p4:类型
		success = zend_fetch_static_property_address_ex(retval, &property_info, cache_slot, fetch_type OPLINE_CC EXECUTE_DATA_CC);
		// 如果返回的不是成功
		if (UNEXPECTED(success != SUCCESS)) {
			// 返回失败
			return FAILURE;
		}
	}

	// 验证 ZEND_FETCH_OBJ_FLAGS 标记
	flags &= ZEND_FETCH_OBJ_FLAGS;
	// 如果有 ZEND_FETCH_OBJ_FLAGS 并且 属性信息类型存在
	if (flags && ZEND_TYPE_IS_SET(property_info->type)) {
		//  验证操作是否可行，p1:返回错误信息，p2:属性zval，p3:对象，p4:返回属性信息，p5:操作类型
		zend_handle_fetch_obj_flags(NULL, *retval, NULL, property_info, flags);
	}

	// 如果要求返回属性信息
	if (prop_info) {
		// 返回属性信息
		*prop_info = property_info;
	}

	// 返回成功
	return SUCCESS;
}


// ing3, 抛出引用类型错误
ZEND_API ZEND_COLD void zend_throw_ref_type_error_type(zend_property_info *prop1, zend_property_info *prop2, zval *zv) {
	// 属性信息1的类型
	zend_string *type1_str = zend_type_to_string(prop1->type);
	// 属性信息2的类型
	zend_string *type2_str = zend_type_to_string(prop2->type);
	// 抛错：引用类型不兼容
	zend_type_error("Reference with value of type %s held by property %s::$%s of type %s is not compatible with property %s::$%s of type %s",
		zend_zval_type_name(zv),
		ZSTR_VAL(prop1->ce->name),
		zend_get_unmangled_property_name(prop1->name),
		ZSTR_VAL(type1_str),
		ZSTR_VAL(prop2->ce->name),
		zend_get_unmangled_property_name(prop2->name),
		ZSTR_VAL(type2_str)
	);
	// 释放临时字串
	zend_string_release(type1_str);
	zend_string_release(type2_str);
}

// ing3, 报错，不能给 A类型 的 属性B::C的引用 进行D类型的赋值
ZEND_API ZEND_COLD void zend_throw_ref_type_error_zval(zend_property_info *prop, zval *zv) {
	zend_string *type_str = zend_type_to_string(prop->type);
	zend_type_error("Cannot assign %s to reference held by property %s::$%s of type %s",
		zend_zval_type_name(zv),
		ZSTR_VAL(prop->ce->name),
		zend_get_unmangled_property_name(prop->name),
		ZSTR_VAL(type_str)
	);
	zend_string_release(type_str);
}

// ing3, 报错：冲突强制错误
ZEND_API ZEND_COLD void zend_throw_conflicting_coercion_error(zend_property_info *prop1, zend_property_info *prop2, zval *zv) {
	// 获得类型字串
	zend_string *type1_str = zend_type_to_string(prop1->type);
	zend_string *type2_str = zend_type_to_string(prop2->type);
	// 报错 不能赋值给 A 类型的属性的引用目标 和 B类型的属性，它会导致一个不一致的类型转换	
	zend_type_error("Cannot assign %s to reference held by property %s::$%s of type %s and property %s::$%s of type %s, as this would result in an inconsistent type conversion",
		zend_zval_type_name(zv),
		ZSTR_VAL(prop1->ce->name),
		zend_get_unmangled_property_name(prop1->name),
		ZSTR_VAL(type1_str),
		ZSTR_VAL(prop2->ce->name),
		zend_get_unmangled_property_name(prop2->name),
		ZSTR_VAL(type2_str)
	);
	// 释放两个类名
	zend_string_release(type1_str);
	zend_string_release(type2_str);
}

// 1：有效，0：无效，-1：强制后可能有效
/* 1: valid, 0: invalid, -1: may be valid after type coercion */
// ing4, 赋值检查，检查值是否适配此属性信息。成功返回1，失败返回0，强制可能返回-1
static zend_always_inline int i_zend_verify_type_assignable_zval(
		zend_property_info *info, zval *zv, bool strict) {
	// 属性信息里的类型
	zend_type type = info->type;
	uint32_t type_mask;
	// 属性值的类型
	zend_uchar zv_type = Z_TYPE_P(zv);
	// 情况1，直接可以适配
	// 检查 type 的类型中 是否包含需要的类型 zv_type
	if (EXPECTED(ZEND_TYPE_CONTAINS_CODE(type, zv_type))) {
		// 返回有效
		return 1;
	}
	// 情况2，对象与交叉类型匹配
	// 1. 检验是否是复杂类型, 复杂类型是指：传入的是（一个列表加一个(union 或 intersection)） 或者 （类名）
	// 2. zv的类型是对象 
	// 3. 检查此对象 与此属性信息是否匹配（是否可以作为此属性的值）
	if (ZEND_TYPE_IS_COMPLEX(type) && zv_type == IS_OBJECT
			&& zend_check_and_resolve_property_class_type(info, Z_OBJCE_P(zv))) {
		// 3个条件都为真，返回 有效
		return 1;
	}
	// 取得属性信息 的 类型码
	type_mask = ZEND_TYPE_FULL_MASK(type);
	// 如果不是闭包或 static
	ZEND_ASSERT(!(type_mask & (MAY_BE_CALLABLE|MAY_BE_STATIC)));

	// 情况3，严格类型下的整数和小数匹配
	// IS_LONG 可以替换成 IS_DOUBLE
	/* SSTH Exception: IS_LONG may be accepted as IS_DOUBLE (converted) */
	// 如果是严格类型
	if (strict) {
		// 类型码包含 小数 并且 类型是 整数 
		if ((type_mask & MAY_BE_DOUBLE) && zv_type == IS_LONG) {
			// 强制后可能有效
			return -1;
		}
		// 无效
		return 0;
	}
	
	// 情况4，值为 Null
	// 只有允null类型这里才可以是null（验证过）
	/* NULL may be accepted only by nullable hints (this is already checked) */
	if (zv_type == IS_NULL) {
		// 无效
		return 0;
	}

	// 情况5，没有包含 可强制转换的 类型
	/* Does not contain any type to which a coercion is possible */
	// 整数，小数，字串，布尔都不支持（布尔型是两个值，所以分开判断）
	if (!(type_mask & (MAY_BE_LONG|MAY_BE_DOUBLE|MAY_BE_STRING))
			&& (type_mask & MAY_BE_BOOL) != MAY_BE_BOOL) {
		// 无效
		return 0;
	}
	// 情况6, 强制后可能有效, 分开验证
	/* Coercion may be necessary, check separately */
	return -1;
}

// ing2, 检查值是否适配属性信息列表的每一条信息，有一条不适配就返回失败。 p1:属性信息列表的引用,p2:值
// 过程中可能用 zend_verify_weak_scalar_type_hint 强制转换p2
ZEND_API bool ZEND_FASTCALL zend_verify_ref_assignable_zval(zend_reference *ref, zval *zv, bool strict)
{
	zend_property_info *prop;

	// 值必须满足每个属性的类型，对同一个值 强制使用 每个属性类型
	// 为了这个目标，要记录第一个强制的类型 和 值。
	/* The value must satisfy each property type, and coerce to the same value for each property
	 * type. Remember the first coerced type and value we've seen for this purpose. */
	zend_property_info *first_prop = NULL;
	
	// 强制值
	zval coerced_value;
	// 设置成undef
	ZVAL_UNDEF(&coerced_value);
	// 不可以是引用类型
	ZEND_ASSERT(Z_TYPE_P(zv) != IS_REFERENCE);
	
	// ref是 属性信息列表的引用
	// 追踪到到引用的 属性信息列表，并遍历它（开始段）
	ZEND_REF_FOREACH_TYPE_SOURCES(ref, prop) {
		// 赋值检查，检查值是否适配此属性信息。成功返回1，失败返回0，强制可能返回-1
		int result = i_zend_verify_type_assignable_zval(prop, zv, strict);
		// 如果失败
		if (result == 0) {
type_error:
			// 报错，不能给 A类型 的 属性B::C的引用 进行D类型的赋值
			zend_throw_ref_type_error_zval(prop, zv);
			// 销毁没有引用次数的对象，有引用次数的添加到gc回收周期里。并没有销毁 zval本身。
			zval_ptr_dtor(&coerced_value);
			// 有一个不能适配，就返回失败
			return 0;
		}

		// 如果结果是强制后有效
		if (result < 0) {
			// 如果没有第一个属性
			if (!first_prop) {
				// 记录第一个属性
				first_prop = prop;
				// 值复制给 强制值
				ZVAL_COPY(&coerced_value, zv);
				// 弱标量类型检查和适配, 类型优先顺序 ：整数 -> 小数 -> 字串 -> 布尔
				if (!zend_verify_weak_scalar_type_hint(
						ZEND_TYPE_FULL_MASK(prop->type), &coerced_value)) {
					// 检查失败，转到报错
					goto type_error;
				}
			// 如果强制值未定义
			} else if (Z_ISUNDEF(coerced_value)) {
				// 前一个属性不需要强制，但这个需要，所以它们是兼容的
				/* A previous property did not require coercion, but this one does,
				 * so they are incompatible. */
				// 转到：冲突强制错误
				goto conflicting_coercion_error;
			// 其他情况
			} else {
				zval tmp;
				// 复制临时变量
				ZVAL_COPY(&tmp, zv);
				// 弱标量类型检查和适配, 类型优先顺序 ：整数 -> 小数 -> 字串 -> 布尔
				if (!zend_verify_weak_scalar_type_hint(ZEND_TYPE_FULL_MASK(prop->type), &tmp)) {
					// 销毁没有引用次数的对象，有引用次数的添加到gc回收周期里。并没有销毁 zval本身。
					zval_ptr_dtor(&tmp);
					// 转到报错
					goto type_error;
				}
				// 如果强制值和临时变量不是同一个
				if (!zend_is_identical(&coerced_value, &tmp)) {
					// 销毁没有引用次数的对象，有引用次数的添加到gc回收周期里。并没有销毁 zval本身。
					zval_ptr_dtor(&tmp);
					// 转到：冲突强制错误
					goto conflicting_coercion_error;
				}
				// 销毁没有引用次数的对象，有引用次数的添加到gc回收周期里。并没有销毁 zval本身。
				zval_ptr_dtor(&tmp);
			}
		// 如果结果是成功
		} else {
			// 如果没有第一个属性
			if (!first_prop) {
				// 保存成第一个属性
				first_prop = prop;
			// 如果强制值 未定义
			} else if (!Z_ISUNDEF(coerced_value)) {
				// 前一个属性需要强制，但这个不需要，所以它们是兼容的
				/* A previous property required coercion, but this one doesn't,
				 * so they are incompatible. */
// 错误：强制值冲突
conflicting_coercion_error:
				// 报错：冲突强制错误 （需要测试）
				zend_throw_conflicting_coercion_error(first_prop, prop, zv);
				// 销毁没有引用次数的对象，有引用次数的添加到gc回收周期里。并没有销毁 zval本身。
				zval_ptr_dtor(&coerced_value);
				return 0;
			}
		}
	} ZEND_REF_FOREACH_TYPE_SOURCES_END();

	// 如果 强制值有效
	if (!Z_ISUNDEF(coerced_value)) {
		// 销毁没有引用次数的对象，有引用次数的添加到gc回收周期里。并没有销毁 zval本身。
		zval_ptr_dtor(zv);
		// 复制强制值
		ZVAL_COPY_VALUE(zv, &coerced_value);
	}
	// 返回成功
	return 1;
}

// ing3, 回收非引用类型zval。引用次数为0时销毁，否则进入回收队列
static zend_always_inline void i_zval_ptr_dtor_noref(zval *zval_ptr) {
	// 如果是可计数变量
	if (Z_REFCOUNTED_P(zval_ptr)) {
		// 计数器
		zend_refcounted *ref = Z_COUNTED_P(zval_ptr);
		// 不可以是引用类型
		ZEND_ASSERT(Z_TYPE_P(zval_ptr) != IS_REFERENCE);
		// 引用次数 -1 后为0
		if (!GC_DELREF(ref)) {
			// 销毁 ref：按类型调用销毁器来销毁实例， zend_variables.c
			rc_dtor_func(ref);
		// 如果有可能溢出
		} else if (UNEXPECTED(GC_MAY_LEAK(ref))) {
			// 添加到 gc回收中
			gc_possible_root(ref);
		}
	}
}

// ing3, 给有规定类型的引用 赋值
// p1:被赋值的引用对象，p2:赋值（可能被强制转换），p3:值类型（变量或其他），p4:是否严格类型
ZEND_API zval* zend_assign_to_typed_ref(zval *variable_ptr, zval *orig_value, zend_uchar value_type, bool strict)
{
	bool ret;
	zval value;
	zend_refcounted *ref = NULL;

	// 如果原始值是引用类型
	if (Z_ISREF_P(orig_value)) {
		// 取得引用计数器
		ref = Z_COUNTED_P(orig_value);
		// 解引用
		orig_value = Z_REFVAL_P(orig_value);
	}

	// 把原始值复制给 value
	ZVAL_COPY(&value, orig_value);
	// 检查值是否适配属性信息列表的每一条信息，有一条不适配就返回失败。 p1:属性信息列表的引用,p2:值
	// 过程中可能用 zend_verify_weak_scalar_type_hint 强制转换p2
	ret = zend_verify_ref_assignable_zval(Z_REF_P(variable_ptr), &value, strict);
	// 变量指针解引用
	variable_ptr = Z_REFVAL_P(variable_ptr);
	// 如果属性信息可用
	if (EXPECTED(ret)) {
		// 回收非引用类型zval。引用次数为0时销毁，否则进入回收队列
		i_zval_ptr_dtor_noref(variable_ptr);
		// 把检查过的值复制到返回值里
		ZVAL_COPY_VALUE(variable_ptr, &value);
	} else {
		// 销毁没有引用次数的对象,并没有销毁 zval本身。nogc的意思是，不放入gc回收周期。
		zval_ptr_dtor_nogc(&value);
	}
	// 如果值类型是 变量或 临时变量
	if (value_type & (IS_VAR|IS_TMP_VAR)) {
		// 如果引用计数器有效
		if (UNEXPECTED(ref)) {
			// 如果-1后为0
			if (UNEXPECTED(GC_DELREF(ref) == 0)) {
				// 销毁没有引用次数的对象，有引用次数的添加到gc回收周期里。并没有销毁 zval本身。
				zval_ptr_dtor(orig_value);
				// 释放 ref
				efree_size(ref, sizeof(zend_reference));
			}
		} else {
			// 回收非引用类型zval。引用次数为0时销毁，否则进入回收队列
			i_zval_ptr_dtor_noref(orig_value);
		}
	}
	// 返回
	return variable_ptr;
}

// ing3, 赋值检查，检查值是否适配此属性信息。成功返回1，失败返回0。带强制转换
ZEND_API bool ZEND_FASTCALL zend_verify_prop_assignable_by_ref(zend_property_info *prop_info, zval *orig_val, bool strict) {
	zval *val = orig_val;
	// 如果val是引用类型，并且 检验引用目标是否有效 ((p1)->sources).ptr != NULL
	if (Z_ISREF_P(val) && ZEND_REF_HAS_TYPE_SOURCES(Z_REF_P(val))) {
		int result;
		// 追踪到目标
		val = Z_REFVAL_P(val);
		// 赋值检查，检查值是否适配此属性信息。成功返回1，失败返回0，强制可能返回-1
		result = i_zend_verify_type_assignable_zval(prop_info, val, strict);
		// 如果成功
		if (result > 0) {
			// 返回 true
			return 1;
		}
		// 强制执行
		if (result < 0) {
			// 这绝对是个错误，但仍然需要确定为什么： 值与类型不匹配 或 由于一个冲突的强制值
			/* This is definitely an error, but we still need to determined why: Either because
			 * the value is simply illegal for the type, or because or a conflicting coercion. */
			zval tmp;
			// 把值复制到临时变量
			ZVAL_COPY(&tmp, val);
			// 弱标量类型检查和适配, 类型优先顺序 ：整数 -> 小数 -> 字串 -> 布尔
			if (zend_verify_weak_scalar_type_hint(ZEND_TYPE_FULL_MASK(prop_info->type), &tmp)) {
				zend_property_info *ref_prop = ZEND_REF_FIRST_SOURCE(Z_REF_P(orig_val));
				zend_throw_ref_type_error_type(ref_prop, prop_info, val);
				// 销毁没有引用次数的对象，有引用次数的添加到gc回收周期里。并没有销毁 zval本身。
				zval_ptr_dtor(&tmp);
				// 返回false
				return 0;
			}
			// 销毁没有引用次数的对象，有引用次数的添加到gc回收周期里。并没有销毁 zval本身。
			zval_ptr_dtor(&tmp);
		}
	// 如果不是引用类型 或 目标没有类型
	} else {
		// 如果是引用类型，追踪到被引用的对象
		ZVAL_DEREF(val);
		// 检查属性值类型 与 属性信息要求的类型是否匹配
		if (i_zend_check_property_type(prop_info, val, strict)) {
			// 返回 true
			return 1;
		}
	}
	// 属性类型查检报错
	zend_verify_property_type_error(prop_info, val);
	// 返回 false
	return 0;
}

/*
typedef union {	struct _zend_property_info *ptr;
	uintptr_t list; } zend_property_info_source_list;
*/
// ing3, 向zend_property_info_source_list指向的 zend_property_info 列表中添加 zend_property_info
ZEND_API void ZEND_FASTCALL zend_ref_add_type_source(zend_property_info_source_list *source_list, zend_property_info *prop)
{
	zend_property_info_list *list;
	// 如果此 list 没有 zend_property_info 指针
	if (source_list->ptr == NULL) {
		// 添加指针并返回
		source_list->ptr = prop;
		return;
	}
	// 如果已经有了 zend_property_info 指针

	// 把 list 转成 zend_property_info_list 指针（去掉最后一位的1）
	list = ZEND_PROPERTY_INFO_SOURCE_TO_LIST(source_list->list);
	// 检检验 属性信息源 是否是 列表（最后 1 位是 1），如果不是
	if (!ZEND_PROPERTY_INFO_SOURCE_IS_LIST(source_list->list)) {
		// 创建 zend_property_info_list(本身长度是24) ，增加3个 zend_property_info 指针的大小。总大小是 24+24=48
		list = emalloc(sizeof(zend_property_info_list) + (4 - 1) * sizeof(zend_property_info *));
		// zend_property_info 指针 ？？
		list->ptr[0] = source_list->ptr;
		// 分配数量为4 
		list->num_allocated = 4;
		// 使用数量为1
		list->num = 1;
	// 如果指针最后1位有1， 并且 分配的数量都用光光
	} else if (list->num_allocated == list->num) {
		// 分配数量翻倍
		list->num_allocated = list->num * 2;
		// 调整大小
		list = erealloc(list, sizeof(zend_property_info_list) + (list->num_allocated - 1) * sizeof(zend_property_info *));
	}
	// 把prop添加进来
	list->ptr[list->num++] = prop;
	// 把指针转成整数并把最后一位写成 1
	source_list->list = ZEND_PROPERTY_INFO_SOURCE_FROM_LIST(list);
}

// ing3, 找到并删除源指针列表中 指向属性信息的指针。p1:源列表，p2:属性信息指针
ZEND_API void ZEND_FASTCALL zend_ref_del_type_source(zend_property_info_source_list *source_list, zend_property_info *prop)
{
	// 把 list 转成 zend_property_info_list 指针（去掉最后一位的1）
	zend_property_info_list *list = ZEND_PROPERTY_INFO_SOURCE_TO_LIST(source_list->list);
	// 
	zend_property_info **ptr, **end;
	// prop 必须要是有效指针
	ZEND_ASSERT(prop);
	// 检检验 属性信息源 是否是 列表（最后 1 位是 1）,如果不是：
	if (!ZEND_PROPERTY_INFO_SOURCE_IS_LIST(source_list->list)) {
		// 列表的指针必须指向 此属性信息
		ZEND_ASSERT(source_list->ptr == prop);
		// 清空资源列表指针
		source_list->ptr = NULL;
		// 返回
		return;
	}

	// 如果是列表，并且包含1个元素
	if (list->num == 1) {
		// 指针列表的第一个元素必须指向 此属性信息
		ZEND_ASSERT(*list->ptr == prop);
		// 释放列表
		efree(list);
		// 清空列表指针
		source_list->ptr = NULL;
		// 返回
		return;
	}

	// 
	/* Checking against end here to get a more graceful failure mode if we missed adding a type
	 * source at some point. */
	// 指针列表的开头和结尾
	ptr = list->ptr;
	end = ptr + list->num;
	
	// 直到找到指向prop的指针
	// 没到结尾并且 列表中的指针不是指向prop
	while (ptr < end && *ptr != prop) {
		// 下一个指针
		ptr++;
	}
	// 找到
	ZEND_ASSERT(*ptr == prop);

	// 把列表中最后一个元素复制到删除的位置
	/* Copy the last list element into the deleted slot. */
	*ptr = list->ptr[--list->num];

	// 如果列表元素大于4个 并且 分配的空间刚好够4个
	if (list->num >= 4 && list->num * 4 == list->num_allocated) {
		// 再多分配一倍空间
		list->num_allocated = list->num * 2;
		// 重新分配内存
		// 把指针转成整数并把最后一位写成 1
		source_list->list = ZEND_PROPERTY_INFO_SOURCE_FROM_LIST(erealloc(list, sizeof(zend_property_info_list) + (list->num_allocated - 1) * sizeof(zend_property_info *)));
	}
}

// ing3, 读写 $this
static zend_never_inline void zend_fetch_this_var(int type OPLINE_DC EXECUTE_DATA_DC)
{
	// #define EX_VAR(n) ZEND_CALL_VAR(execute_data, n)
	// #define ZEND_CALL_VAR(call, n) ((zval*)(((char*)(call)) + ((int)(n))))
	zval *result = EX_VAR(opline->result.var);

	// 根据操作类型操作
	switch (type) {
		// 读取
		case BP_VAR_R:
			// 如果 this是对象
			if (EXPECTED(Z_TYPE(EX(This)) == IS_OBJECT)) {
				// 返回 this 里的对象
				ZVAL_OBJ(result, Z_OBJ(EX(This)));
				// 增加引用次数
				Z_ADDREF_P(result);
			// 如果 不是对象
			} else {
				// 结果为null
				ZVAL_NULL(result);
				// 报错：未定义 $this
				zend_error(E_WARNING, "Undefined variable $this");
			}
			break;
		// isset
		case BP_VAR_IS:
			// 如果 this是对象
			if (EXPECTED(Z_TYPE(EX(This)) == IS_OBJECT)) {
				// 返回 this 里的对象
				ZVAL_OBJ(result, Z_OBJ(EX(This)));
				// 增加引用次数
				Z_ADDREF_P(result);
			// 如果 不是对象
			} else {
				// 结果为null
				ZVAL_NULL(result);
			}
			break;
		// 更新或创建
		case BP_VAR_RW:
		case BP_VAR_W:
			ZVAL_UNDEF(result);
			// 报错，不可以给 $this 重新赋值
			zend_throw_error(NULL, "Cannot re-assign $this");
			break;
		// 删除
		case BP_VAR_UNSET:
			// 清空结果
			ZVAL_UNDEF(result);
			// 报错，不可以删除 $this
			zend_throw_error(NULL, "Cannot unset $this");
			break;
		EMPTY_SWITCH_DEFAULT_CASE()
	}
}

// ing3, 报错：调用 __clone方法错误
static zend_never_inline ZEND_COLD void ZEND_FASTCALL zend_wrong_clone_call(zend_function *clone, zend_class_entry *scope)
{
	zend_throw_error(NULL, "Call to %s %s::__clone() from %s%s",
		// 可见性，函数名
		zend_visibility_string(clone->common.fn_flags), ZSTR_VAL(clone->common.scope->name),
		// 没有域显示  global scope
		scope ? "scope " : "global scope",
		// 域名
		scope ? ZSTR_VAL(scope->name) : ""
	);
}

// 调试用
#if ZEND_INTENSIVE_DEBUGGING

// suspend
#define CHECK_SYMBOL_TABLES()													\
	/**/ \
	zend_hash_apply(&EG(symbol_table), zend_check_symbol);			\
	if (&EG(symbol_table)!=EX(symbol_table)) {							\
		zend_hash_apply(EX(symbol_table), zend_check_symbol);	\
	}

// suspend
static void zend_check_symbol(zval *pz)
{
	if (Z_TYPE_P(pz) == IS_INDIRECT) {
		pz = Z_INDIRECT_P(pz);
	}
	if (Z_TYPE_P(pz) > 10) {
		fprintf(stderr, "Warning!  %x has invalid type!\n", *pz);
/* See http://support.microsoft.com/kb/190351 */
#ifdef ZEND_WIN32
		fflush(stderr);
#endif
	} else if (Z_TYPE_P(pz) == IS_ARRAY) {
		zend_hash_apply(Z_ARRVAL_P(pz), zend_check_symbol);
	} else if (Z_TYPE_P(pz) == IS_OBJECT) {
		/* OBJ-TBI - doesn't support new object model! */
		zend_hash_apply(Z_OBJPROP_P(pz), zend_check_symbol);
	}
}


#else
// ing4, 正式环境无意义
#define CHECK_SYMBOL_TABLES()
#endif

// ing3, 执行内部函数
ZEND_API void execute_internal(zend_execute_data *execute_data, zval *return_value)
{
	// 直接找到并执行内部函数 处理器
	execute_data->func->internal_function.handler(execute_data, return_value);
}

// ing3, 清空并缓存 符号表
ZEND_API void zend_clean_and_cache_symbol_table(zend_array *symbol_table) /* {{{ */
{
	// 放进缓存前先清理，清理时可以调用 使用缓存哈希的销毁器
	// 进行可用缓存位置检查前也要做这个清理，因为它们可以被销毁器使用过
	/* Clean before putting into the cache, since clean could call dtors,
	 * which could use the cached hash. Also do this before the check for
	 * available cache slots, as those may be used by a dtor as well. */
	 
	// 清空符号表, 销毁所有元素
	zend_symtable_clean(symbol_table);
	// 如果缓存点使用超限了
	if (EG(symtable_cache_ptr) >= EG(symtable_cache_limit)) {
		// 销毁符号表
		zend_array_destroy(symbol_table);
	// 找到下一个符号表缓存位置，把符号表放进去
	} else {
		// 放入符号表缓存表
		*(EG(symtable_cache_ptr)++) = symbol_table;
	}
}
/* }}} */

// ing3, 销毁 执行函数中的所有临时变量
static zend_always_inline void i_free_compiled_variables(zend_execute_data *execute_data) /* {{{ */
{
	// 执行数据中的第一个变量
	zval *cv = EX_VAR_NUM(0);
	// 临时变量数量
	int count = EX(func)->op_array.last_var;
	// 依次处理
	while (EXPECTED(count != 0)) {
		// 销毁临时变量
		i_zval_ptr_dtor(cv);
		// 下一个变量
		cv++;
		// 数量-1
		count--;
	}
}
/* }}} */

// ing3, 销毁 执行函数中的所有临时变量
ZEND_API void ZEND_FASTCALL zend_free_compiled_variables(zend_execute_data *execute_data) /* {{{ */
{
	i_free_compiled_variables(execute_data);
}
/* }}} */

// ing2, 打断并返回
#define ZEND_VM_INTERRUPT_CHECK() do { \
		/* 返回 p1->value ，布尔值 */ \
		if (UNEXPECTED(zend_atomic_bool_load_ex(&EG(vm_interrupt)))) { \
			/* ZEND_VM_TAIL_CALL(zend_interrupt_helper_SPEC(execute_data)) */ \
			/* 牵扯太多其他逻辑 */ \
			ZEND_VM_INTERRUPT(); \
		} \
	} while (0)

// ing2, 循环打断，不返回
#define ZEND_VM_LOOP_INTERRUPT_CHECK() do { \
		/* 返回 p1->value ，布尔值 */ \
		if (UNEXPECTED(zend_atomic_bool_load_ex(&EG(vm_interrupt)))) { \
			/* zend_interrupt_helper_SPEC(execute_data) */\
			/* 牵扯太多其他逻辑 */ \
			ZEND_VM_LOOP_INTERRUPT(); \
		} \
	} while (0)

/*
 堆栈框架布局
 * Stack Frame Layout (the whole stack frame is allocated at once)
 * ==================
 *
 *                             +========================================+
 * EG(current_execute_data) -> | zend_execute_data                      |
 *                             +----------------------------------------+
 *     EX_VAR_NUM(0) --------> | VAR[0] = ARG[1]                        |
 *                             | ...                                    |
 *                             | VAR[op_array->num_args-1] = ARG[N]     |
 *                             | ...                                    |
 *                             | VAR[op_array->last_var-1]              |
 *                             | VAR[op_array->last_var] = TMP[0]       |
 *                             | ...                                    |
 *                             | VAR[op_array->last_var+op_array->T-1]  |
 *                             | ARG[N+1] (extra_args)                  |
 *                             | ...                                    |
 *                             +----------------------------------------+
 */

// 实际传入参数数量 EX_NUM_ARGS 比 函数定义的参数数量 (op_array->num_args) 多时，才用到 zend_copy_extra_args
// 所有变量都编译完成后，额外的参数会被复制到调用框架中
// 如果复制了额外的参数， ZEND_CALL_FREE_EXTRA_ARGS 标记会被添加到 zend_execute_data，
// 当 执行器离开函数 ，参数会在 zend_leave_helper 中释放
/* zend_copy_extra_args is used when the actually passed number of arguments
 * (EX_NUM_ARGS) is greater than what the function defined (op_array->num_args).
 *
 * The extra arguments will be copied into the call frame after all the compiled variables.
 *
 * If there are extra arguments copied, a flag "ZEND_CALL_FREE_EXTRA_ARGS" will be set
 * on the zend_execute_data, and when the executor leaves the function, the
 * args will be freed in zend_leave_helper.
 */
 
// ing2, 复制额外参数（超过形参数量的参数）
static zend_never_inline void zend_copy_extra_args(EXECUTE_DATA_D)
{
	// 执行数据中的操作码列表
	zend_op_array *op_array = &EX(func)->op_array;
	// 第一个额外参数序号（等于形参数量）
	uint32_t first_extra_arg = op_array->num_args;
	// 参数数量
	uint32_t num_args = EX_NUM_ARGS();
	// 
	zval *src;
	size_t delta;
	uint32_t count;
	uint32_t type_flags = 0;

	// 如果操作码中 没有 规定类型
	if (EXPECTED((op_array->fn_flags & ZEND_ACC_HAS_TYPE_HINTS) == 0)) {
		/* Skip useless ZEND_RECV and ZEND_RECV_INIT opcodes */
// 这些都是 __GNUC__ 用的，windows里没有
#if defined(ZEND_VM_IP_GLOBAL_REG) && ((ZEND_VM_KIND == ZEND_VM_KIND_CALL) || (ZEND_VM_KIND == ZEND_VM_KIND_HYBRID))
		opline += first_extra_arg;
// windows走这里
#else
		// 操作码后移 first_extra_arg 个位置。跳过所有非扩展参数。
		EX(opline) += first_extra_arg;
#endif

	}

	// 把额外的参数移动到单独的数组中，放在所有 编译变量 和 临时变量 后面
	/* move extra args into separate array after all CV and TMP vars */
	// 源位置
	src = EX_VAR_NUM(num_args - 1);
	// ？需要挪动的位置
	delta = op_array->last_var + op_array->T - first_extra_arg;
	// 额外参数数量
	count = num_args - first_extra_arg;
	// 如果有空间留给额外参数
	if (EXPECTED(delta != 0)) {
		// 总大小
		delta *= sizeof(zval);
		// 倒序遍历所有额外参数。把额外参数挪动到结尾段
		do {
			// 取得 type_info
			type_flags |= Z_TYPE_INFO_P(src);
			// 把变量复制到同一个串里的新位置，向后 delta 这么多
			ZVAL_COPY_VALUE((zval*)(((char*)src) + delta), src);
			// 清空原变量
			ZVAL_UNDEF(src);
			// 下一个
			src--;
		// 计数 -1
		} while (--count);
		
		// 通过 type_info 确定是否可记录引用数（只要有一个参数可计数）
		if (Z_TYPE_INFO_REFCOUNTED(type_flags)) {
			// 执行数据 添加标记 ZEND_CALL_FREE_EXTRA_ARGS
			// Z_TYPE_INFO((p1)->This) |= p2
			ZEND_ADD_CALL_FLAG(execute_data, ZEND_CALL_FREE_EXTRA_ARGS);
		}
	// 
	} else {
		// 倒序遍历所有额外参数
		do {
			// 只要有一个参数可计数
			if (Z_REFCOUNTED_P(src)) {
				// 执行数据 添加标记 ZEND_CALL_FREE_EXTRA_ARGS
				// Z_TYPE_INFO((p1)->This) |= p2
				ZEND_ADD_CALL_FLAG(execute_data, ZEND_CALL_FREE_EXTRA_ARGS);
				// 跳出
				break;
			}
			// 下一个			
			src--;
		// 计数 -1
		} while (--count);
	}
}

// ing3, 初始化编译变量，这一串变量设置成未定义。p1:开始位置，p2:结束位置
static zend_always_inline void zend_init_cvs(uint32_t first, uint32_t last EXECUTE_DATA_DC)
{
	// 两个位置必须有效
	if (EXPECTED(first < last)) {
		// 数量
		uint32_t count = last - first;
		// 取得第一个变量
		zval *var = EX_VAR_NUM(first);
		// 把这一串变量设置成未定义
		do {
			ZVAL_UNDEF(var);
			var++;
		} while (--count);
	}
}

// ing2, 初始化函数的执行数据，p1:操作码组，p2:返回值
static zend_always_inline void i_init_func_execute_data(zend_op_array *op_array, zval *return_value, bool may_be_trampoline EXECUTE_DATA_DC) /* {{{ */
{
	// 临时变量
	uint32_t first_extra_arg, num_args;
	// execute_data->func 与 操作码所属函数相同
	ZEND_ASSERT(EX(func) == (zend_function*)op_array);
	
// 这些都是 __GNUC__ 用的，windows里没有
#if defined(ZEND_VM_IP_GLOBAL_REG) && ((ZEND_VM_KIND == ZEND_VM_KIND_CALL) || (ZEND_VM_KIND == ZEND_VM_KIND_HYBRID))
	opline = op_array->opcodes;
// windows走这里
#else
	EX(opline) = op_array->opcodes;
#endif

	// ？虚拟机里大量使用
	EX(call) = NULL;
	// 返回值
	EX(return_value) = return_value;

	// 处理参数
	/* Handle arguments */
	// 第一个额外参数序号（形式参数位置）
	first_extra_arg = op_array->num_args;
	// 参数数量
	num_args = EX_NUM_ARGS();
	// 如果实际参数数量 大于 第一个额外参数序号（形式参数位置）
	if (UNEXPECTED(num_args > first_extra_arg)) {
		// 如果不可能用弹跳  或 操作码不是由弹跳调用
		if (!may_be_trampoline || EXPECTED(!(op_array->fn_flags & ZEND_ACC_CALL_VIA_TRAMPOLINE))) {
			// 复制额外参数（超过形参数量的参数）
			zend_copy_extra_args(EXECUTE_DATA_C);
		}
	// 如果函数里有规定参数类型
	} else if (EXPECTED((op_array->fn_flags & ZEND_ACC_HAS_TYPE_HINTS) == 0)) {
		/* Skip useless ZEND_RECV and ZEND_RECV_INIT opcodes */
// 这些都是 __GNUC__ 用的，windows里没有
#if defined(ZEND_VM_IP_GLOBAL_REG) && ((ZEND_VM_KIND == ZEND_VM_KIND_CALL) || (ZEND_VM_KIND == ZEND_VM_KIND_HYBRID))
		opline += num_args;
// windows 走这里
#else
		// 向后移 num_args 个操作码
		EX(opline) += num_args;
#endif
	}

	// 初始化编译变量（跳过参数）
	/* Initialize CV variables (skip arguments) */
	// 初始化编译变量，这一串变量设置成未定义。p1:开始位置，p2:结束位置
	zend_init_cvs(num_args, op_array->last_var EXECUTE_DATA_CC);
	// 把操作码的运行时缓存，连接到执行数据
	EX(run_time_cache) = RUN_TIME_CACHE(op_array);
	// 更新当前执行数据
	EG(current_execute_data) = execute_data;
}
/* }}} */

// ing3, 为op_array创建并 初始化运行时缓存
static zend_always_inline void init_func_run_time_cache_i(zend_op_array *op_array) /* {{{ */
{
	void **run_time_cache;
	// op_array 必须没有运行时缓存
	ZEND_ASSERT(RUN_TIME_CACHE(op_array) == NULL);
	// 创建内存，大小按 op_array 的缓存 大小
	run_time_cache = zend_arena_alloc(&CG(arena), op_array->cache_size);
	// 全都初始化成0
	memset(run_time_cache, 0, op_array->cache_size);
	// 关联到 op_array 
	ZEND_MAP_PTR_SET(op_array->run_time_cache, run_time_cache);
}
/* }}} */

// ing3, 为op_array创建并 初始化运行时缓存
static zend_never_inline void ZEND_FASTCALL init_func_run_time_cache(zend_op_array *op_array) /* {{{ */
{
	// 为op_array创建并 初始化运行时缓存
	init_func_run_time_cache_i(op_array);
}
/* }}} */

// ing3, 用函数名 zend_string 获取函数
ZEND_API zend_function * ZEND_FASTCALL zend_fetch_function(zend_string *name) /* {{{ */
{
	// 在执行时函数表中查找
	zval *zv = zend_hash_find(EG(function_table), name);
	// 如果找到
	if (EXPECTED(zv != NULL)) {
		// 函数指针
		zend_function *fbc = Z_FUNC_P(zv);
		// 如果是用户函数，并且没有运行缓存
		if (EXPECTED(fbc->type == ZEND_USER_FUNCTION) && UNEXPECTED(!RUN_TIME_CACHE(&fbc->op_array))) {
			// 为op_array创建并 初始化运行时缓存
			init_func_run_time_cache_i(&fbc->op_array);
		}
		// 返回函数
		return fbc;
	}
	// 返回null
	return NULL;
} /* }}} */

// ing3, 用函数名字串 和 长度 获取函数
ZEND_API zend_function * ZEND_FASTCALL zend_fetch_function_str(const char *name, size_t len) /* {{{ */
{
	zval *zv = zend_hash_str_find(EG(function_table), name, len);

	if (EXPECTED(zv != NULL)) {
		zend_function *fbc = Z_FUNC_P(zv);

		if (EXPECTED(fbc->type == ZEND_USER_FUNCTION) && UNEXPECTED(!RUN_TIME_CACHE(&fbc->op_array))) {
			// 为op_array创建并 初始化运行时缓存
			init_func_run_time_cache_i(&fbc->op_array);
		}
		return fbc;
	}
	return NULL;
} /* }}} */

// ing3, 为op_array创建并 初始化运行时缓存
ZEND_API void ZEND_FASTCALL zend_init_func_run_time_cache(zend_op_array *op_array) /* {{{ */
{
	// 如果没有运行时缓存
	if (!RUN_TIME_CACHE(op_array)) {
		// 为op_array创建并 初始化运行时缓存
		init_func_run_time_cache_i(op_array);
	}
} /* }}} */

// ing3, 初始化p1执行数据，p1:执行数据，p2:包含 执行时缓存 的操作码列表，p3:返回值
static zend_always_inline void i_init_code_execute_data(zend_execute_data *execute_data, zend_op_array *op_array, zval *return_value) /* {{{ */
{
	// 执行时数据中的函数 要和操作码所属函数相同
	ZEND_ASSERT(EX(func) == (zend_function*)op_array);

	// 执行数据 添加的操作码列表
	EX(opline) = op_array->opcodes;
	// call ?
	EX(call) = NULL;
	// 添加返回值
	EX(return_value) = return_value;

	// 如果操作码中有临时变量
	if (op_array->last_var) {
		// 从附加符号表里读取所有临时变量，并更新到临时变量列表中
		zend_attach_symbol_table(execute_data);
	}

	// 如果没有运行时缓存
	if (!ZEND_MAP_PTR(op_array->run_time_cache)) {
		void *ptr;
		// 必须是【heap 上分配的 运行时缓存（用户用）】
		ZEND_ASSERT(op_array->fn_flags & ZEND_ACC_HEAP_RT_CACHE);
		// 分配内存创建运行时缓存
		ptr = emalloc(op_array->cache_size);
		// 关联到 运行时缓存指针
		ZEND_MAP_PTR_INIT(op_array->run_time_cache, ptr);
		// 全部置为0
		memset(ptr, 0, op_array->cache_size);
	}
	// 使用此运行时缓存 作为当前 运行时缓存
	EX(run_time_cache) = RUN_TIME_CACHE(op_array);
	// 此执行数据 作为 当前执行数据
	EG(current_execute_data) = execute_data;
}
/* }}} */

// ing3, 初始化函数的执行数据，p1:执行数据，p2:操作码组，p3:返回值
ZEND_API void zend_init_func_execute_data(zend_execute_data *ex, zend_op_array *op_array, zval *return_value) /* {{{ */
{
// 这些都是 __GNUC__ 用的，windows里没有
#if defined(ZEND_VM_FP_GLOBAL_REG) && ((ZEND_VM_KIND == ZEND_VM_KIND_CALL) || (ZEND_VM_KIND == ZEND_VM_KIND_HYBRID))
	zend_execute_data *orig_execute_data = execute_data;
#endif

// 这些都是 __GNUC__ 用的，windows里没有
#if defined(ZEND_VM_IP_GLOBAL_REG) && ((ZEND_VM_KIND == ZEND_VM_KIND_CALL) || (ZEND_VM_KIND == ZEND_VM_KIND_HYBRID))
	const zend_op *orig_opline = opline;
#endif

// 这些都是 __GNUC__ 用的，windows里没有
#if defined(ZEND_VM_FP_GLOBAL_REG) && ((ZEND_VM_KIND == ZEND_VM_KIND_CALL) || (ZEND_VM_KIND == ZEND_VM_KIND_HYBRID))
	execute_data = ex;
// windows走这里
#else
	zend_execute_data *execute_data = ex;
#endif

	// 核心逻辑只这几行
	// 把当前执行数据记录成前一个
	EX(prev_execute_data) = EG(current_execute_data);
	
	// 如果操作码列表没有 run_time_cache
	if (!RUN_TIME_CACHE(op_array)) {
		// 为op_array创建并 初始化运行时缓存
		init_func_run_time_cache(op_array);
	}
	// 初始化函数的执行数据，p1:操作码组，p2:返回值
	i_init_func_execute_data(op_array, return_value, 1 EXECUTE_DATA_CC);

// 这些都是 __GNUC__ 用的，windows里没有
#if defined(ZEND_VM_IP_GLOBAL_REG) && ((ZEND_VM_KIND == ZEND_VM_KIND_CALL) || (ZEND_VM_KIND == ZEND_VM_KIND_HYBRID))
	EX(opline) = opline;
	opline = orig_opline;
#endif
// 这些都是 __GNUC__ 用的，windows里没有
#if defined(ZEND_VM_FP_GLOBAL_REG) && ((ZEND_VM_KIND == ZEND_VM_KIND_CALL) || (ZEND_VM_KIND == ZEND_VM_KIND_HYBRID))
	execute_data = orig_execute_data;
#endif
}
/* }}} */

// ing3, 初始化p1执行数据，p1:执行数据，p2:包含 执行时缓存 的操作码列表，p3:返回值
ZEND_API void zend_init_code_execute_data(zend_execute_data *execute_data, zend_op_array *op_array, zval *return_value) /* {{{ */
{
	// 把当前执行数据转到前一个
	EX(prev_execute_data) = EG(current_execute_data);
	// 初始化p1执行数据，p1:执行数据，p2:包含 执行时缓存 的操作码列表，p3:返回值
	i_init_code_execute_data(execute_data, op_array, return_value);
}
/* }}} */

// ing3, 初始化函数的执行数据，p1:执行数据，p2:操作码组，p3:返回值
ZEND_API void zend_init_execute_data(zend_execute_data *execute_data, zend_op_array *op_array, zval *return_value) /* {{{ */
{
	// ing3, Z_TYPE_INFO((execute_data)->This))
	// 如果里面有符号表
	if (EX_CALL_INFO() & ZEND_CALL_HAS_SYMBOL_TABLE) {
		// 初始化p1执行数据，p1:执行数据，p2:包含 执行时缓存 的操作码列表，p3:返回值
		zend_init_code_execute_data(execute_data, op_array, return_value);
	// 如果里面没有符号表
	} else {
		// 初始化函数的执行数据，p1:执行数据，p2:操作码组，p3:返回值
		zend_init_func_execute_data(execute_data, op_array, return_value);
	}
}
/* }}} */

// ing3, 复制执行数据到新创建的执行数据中。 p1:执行数据，p2:传入参数，p3:附加参数
zend_execute_data *zend_vm_stack_copy_call_frame(zend_execute_data *call, uint32_t passed_args, uint32_t additional_args) /* {{{ */
{
	zend_execute_data *new_call;
	// 使用的变量数量  = 堆栈最高位置 - 堆栈开头位置 + 附加参数数
	int used_stack = (EG(vm_stack_top) - (zval*)call) + additional_args;

	// 把整个框架复制到新的堆栈 片
	/* copy call frame into new stack segment */
	// 创建新的堆栈页
	new_call = zend_vm_stack_extend(used_stack * sizeof(zval));
	// 执行数据指针
	*new_call = *call;
	// 添加标记 ZEND_CALL_ALLOCATED
	// Z_TYPE_INFO((p1)->This) |= p2
	ZEND_ADD_CALL_FLAG(new_call, ZEND_CALL_ALLOCATED);

	// 如果有传入参数
	if (passed_args) {
		// 取得 call里 第一个参数
		zval *src = ZEND_CALL_ARG(call, 1);
		// 取得 new_call里 第一个参数
		zval *dst = ZEND_CALL_ARG(new_call, 1);
		// 
		do {
			// 从 call 复制到 new_call
			ZVAL_COPY_VALUE(dst, src);
			// 数量-1
			passed_args--;
			// 下一个参数
			src++;
			dst++;
		// 遍历 每一个参数
		} while (passed_args);
	}

	// 在前一个堆栈片中， 删除旧的调用
	/* delete old call_frame from previous stack segment */
	EG(vm_stack)->prev->top = (zval*)call;

	// 如果前一个堆栈片空了， 删除堆栈片
	/* delete previous stack segment if it became empty */
	
	// 如果前一个堆栈的top位置 = 开始位置。说明它空了
	// ZEND_VM_STACK_ELEMENTS：stack 跳过头部分
	if (UNEXPECTED(EG(vm_stack)->prev->top == ZEND_VM_STACK_ELEMENTS(EG(vm_stack)->prev))) {
		// 当前堆栈的前一个
		zend_vm_stack r = EG(vm_stack)->prev;
		// 使用 前一个 的 前一个
		EG(vm_stack)->prev = r->prev;
		// 释放 前一个 堆栈页
		efree(r);
	}

	// 返回新调用数据
	return new_call;
}
/* }}} */

// ing3, 取得正在运行的生成器 EX(return_value)
static zend_always_inline zend_generator *zend_get_running_generator(EXECUTE_DATA_D) /* {{{ */
{
	// 生成器对象存储在 EX(return_value)
	/* The generator object is stored in EX(return_value) */
	// 取出生成器
	zend_generator *generator = (zend_generator *) EX(return_value);
	// 即使控制权已指派给其他生成器，这个仍然是想要的
	/* However control may currently be delegated to another generator.
	 * That's the one we're interested in. */
	return generator;
}
/* }}} */

// ing2, 对未完成的调用进行清理，把相关变量放入垃圾回收，(操作码意义还不够清晰)
// p1:执行数据, p2:调用的执行数据，p3:操作码数量，p4:垃圾回收位置
ZEND_API void zend_unfinished_calls_gc(zend_execute_data *execute_data, zend_execute_data *call, uint32_t op_num, zend_get_gc_buffer *buf) /* {{{ */
{
	// 找到最后一个操作码，准备倒序遍历
	zend_op *opline = EX(func)->op_array.opcodes + op_num;
	int level;
	int do_exit;
	uint32_t num_args;

	// 跳过这些
	if (UNEXPECTED(opline->opcode == ZEND_INIT_FCALL ||
		opline->opcode == ZEND_INIT_FCALL_BY_NAME ||
		opline->opcode == ZEND_INIT_NS_FCALL_BY_NAME ||
		opline->opcode == ZEND_INIT_DYNAMIC_CALL ||
		opline->opcode == ZEND_INIT_USER_CALL ||
		opline->opcode == ZEND_INIT_METHOD_CALL ||
		opline->opcode == ZEND_INIT_STATIC_METHOD_CALL ||
		opline->opcode == ZEND_NEW)) {
		ZEND_ASSERT(op_num);
		opline--;
	}

	// 
	do {
		// 找到真实传递的参数数量
		/* find the number of actually passed arguments */
		// 调用层数
		level = 0;
		// 跳出while循环
		do_exit = 0;
		// 传入参数数量
		num_args = ZEND_CALL_NUM_ARGS(call);
		do {
			// 根据操作码操作
			switch (opline->opcode) {
				// 
				case ZEND_DO_FCALL:
				case ZEND_DO_ICALL:
				case ZEND_DO_UCALL:
				case ZEND_DO_FCALL_BY_NAME:
					// 这几个，加深一层
					level++;
					// 跳出 switch
					break;
				case ZEND_INIT_FCALL:
				case ZEND_INIT_FCALL_BY_NAME:
				case ZEND_INIT_NS_FCALL_BY_NAME:
				case ZEND_INIT_DYNAMIC_CALL:
				case ZEND_INIT_USER_CALL:
				case ZEND_INIT_METHOD_CALL:
				case ZEND_INIT_STATIC_METHOD_CALL:
				case ZEND_NEW:
					// 如果在最外层
					if (level == 0) {
						// 参数数为0
						num_args = 0;
						// 要跳出while
						do_exit = 1;
					}
					// 再跳出一层
					level--;
					// 跳出 switch
					break;
				// 
				case ZEND_SEND_VAL:
				case ZEND_SEND_VAL_EX:
				case ZEND_SEND_VAR:
				case ZEND_SEND_VAR_EX:
				case ZEND_SEND_FUNC_ARG:
				case ZEND_SEND_REF:
				case ZEND_SEND_VAR_NO_REF:
				case ZEND_SEND_VAR_NO_REF_EX:
				case ZEND_SEND_USER:
					// 如果在最外层
					if (level == 0) {
						// 对于命名参数，参数数量是最新的
						/* For named args, the number of arguments is up to date. */
						// 如果 操作对象2的类型 不是常量
						if (opline->op2_type != IS_CONST) {
							// 更新，参数数量
							num_args = opline->op2.num;
						}
						// 需要跳出while
						do_exit = 1;
					}
					// 跳出 switch
					break;
				//
				case ZEND_SEND_ARRAY:
				case ZEND_SEND_UNPACK:
				case ZEND_CHECK_UNDEF_ARGS:
					// 如果在最外层
					if (level == 0) {
						// 需要跳出while
						do_exit = 1;
					}
					// 跳出 switch
					break;
			}
			// 不需要跳出时
			if (!do_exit) {
				// 下一个操作码
				opline--;
			}
		// 不需要跳出就一直循环
		} while (!do_exit);
		
		
		// 如果有前一个执行数据
		if (call->prev_execute_data) {
			// 跳过当前调用域
			/* skip current call region */
			// 调用层数
			level = 0;
			// 跳出while循环
			do_exit = 0;
			do {
				// 按操作码操作
				switch (opline->opcode) {
					// 
					case ZEND_DO_FCALL:
					case ZEND_DO_ICALL:
					case ZEND_DO_UCALL:
					case ZEND_DO_FCALL_BY_NAME:
						// 这几个要加深一层调用
						level++;
						// 跳出
						break;
					// 
					case ZEND_INIT_FCALL:
					case ZEND_INIT_FCALL_BY_NAME:
					case ZEND_INIT_NS_FCALL_BY_NAME:
					case ZEND_INIT_DYNAMIC_CALL:
					case ZEND_INIT_USER_CALL:
					case ZEND_INIT_METHOD_CALL:
					case ZEND_INIT_STATIC_METHOD_CALL:
					case ZEND_NEW:
						// 如果在最上层调用这几个
						if (level == 0) {
							// 需要退出循环
							do_exit = 1;
						}
						// 跳出一层
						level--;
						// 跳出
						break;
				}
				// 下一个操作码
				opline--;
			} while (!do_exit);
		}

		// 如果有参数数量 
		if (EXPECTED(num_args > 0)) {
			// 取得第一个参数
			zval *p = ZEND_CALL_ARG(call, 1);
			// 
			do {
				// 给游标（p1->cur）指向的zval赋值
				zend_get_gc_buffer_add_zval(buf, p);
				// 下一个参数
				p++;
			// 遍历 所有参数
			} while (--num_args);
		}
		// 如果调用有 【需要释放this】 标记
		// Z_TYPE_INFO((execute_data)->This))
		if (ZEND_CALL_INFO(call) & ZEND_CALL_RELEASE_THIS) {
			// 给游标指向的zval赋值添加对象
			zend_get_gc_buffer_add_obj(buf, Z_OBJ(call->This));
		}
		// 如果调用包含额外的参数
		// Z_TYPE_INFO((execute_data)->This))
		if (ZEND_CALL_INFO(call) & ZEND_CALL_HAS_EXTRA_NAMED_PARAMS) {
			// 临时变量
			zval *val;
			// 遍历命名参数
			ZEND_HASH_FOREACH_VAL(call->extra_named_params, val) {
				// 给游标（p1->cur）指向的zval赋值
				zend_get_gc_buffer_add_zval(buf, val);
			} ZEND_HASH_FOREACH_END();
		}
		// 如果是调用闭包
		if (call->func->common.fn_flags & ZEND_ACC_CLOSURE) {
			// 给游标指向的zval赋值添加对象
			zend_get_gc_buffer_add_obj(buf, ZEND_CLOSURE_OBJECT(call->func));
		}
		// 前一个执行数据
		call = call->prev_execute_data;
	// 
	} while (call);
}
/* }}} */

// ing3, 清理未完成的调用。p1:执行数据，p2:操作码数量
static void cleanup_unfinished_calls(zend_execute_data *execute_data, uint32_t op_num) /* {{{ */
{
	// 如果执行数据里有 调用
	if (UNEXPECTED(EX(call))) {
		// 取出调用
		zend_execute_data *call = EX(call);
		// 操作码
		zend_op *opline = EX(func)->op_array.opcodes + op_num;
		int level;
		int do_exit;

		// 
		if (UNEXPECTED(opline->opcode == ZEND_INIT_FCALL ||
			opline->opcode == ZEND_INIT_FCALL_BY_NAME ||
			opline->opcode == ZEND_INIT_NS_FCALL_BY_NAME ||
			opline->opcode == ZEND_INIT_DYNAMIC_CALL ||
			opline->opcode == ZEND_INIT_USER_CALL ||
			opline->opcode == ZEND_INIT_METHOD_CALL ||
			opline->opcode == ZEND_INIT_STATIC_METHOD_CALL ||
			opline->opcode == ZEND_NEW)) {
			ZEND_ASSERT(op_num);
			// 跳过这些操作码
			opline--;
		}

		// 
		do {
			// 如果异常在函数调用中发生，有一些参数会压在堆栈里 需要清理
			/* If the exception was thrown during a function call there might be
			 * arguments pushed to the stack that have to be dtor'ed. */

			// 找到实际传递的参数
			/* find the number of actually passed arguments */
			// 调用层数
			level = 0;
			// 是否退出while循环
			do_exit = 0;
			// 
			do {
				// 根据操作码操作
				switch (opline->opcode) {
					//
					case ZEND_DO_FCALL:
					case ZEND_DO_ICALL:
					case ZEND_DO_UCALL:
					case ZEND_DO_FCALL_BY_NAME:
						// 调用层数 +1
						level++;
						// 跳出switch
						break;
					//
					case ZEND_INIT_FCALL:
					case ZEND_INIT_FCALL_BY_NAME:
					case ZEND_INIT_NS_FCALL_BY_NAME:
					case ZEND_INIT_DYNAMIC_CALL:
					case ZEND_INIT_USER_CALL:
					case ZEND_INIT_METHOD_CALL:
					case ZEND_INIT_STATIC_METHOD_CALL:
					case ZEND_NEW:
						// 如果在最外层
						if (level == 0) {
							// 参数数量设置成 0
							ZEND_CALL_NUM_ARGS(call) = 0;
							// 跳出while循环
							do_exit = 1;
						}
						// 调用层数 -1
						level--;
						// 跳出 switch
						break;
					// 
					case ZEND_SEND_VAL:
					case ZEND_SEND_VAL_EX:
					case ZEND_SEND_VAR:
					case ZEND_SEND_VAR_EX:
					case ZEND_SEND_FUNC_ARG:
					case ZEND_SEND_REF:
					case ZEND_SEND_VAR_NO_REF:
					case ZEND_SEND_VAR_NO_REF_EX:
					case ZEND_SEND_USER:
						// 如果在最外层
						if (level == 0) {
							// 对于命名参数，参数数量已经更新到最新
							/* For named args, the number of arguments is up to date. */
							// 如果 ，操作对象2 不是常量
							if (opline->op2_type != IS_CONST) {
								// 更新 调用执行数据中的参数数量 
								ZEND_CALL_NUM_ARGS(call) = opline->op2.num;
							}
							// 要跳出 while
							do_exit = 1;
						}
						// 跳出 switch
						break;
					//
					case ZEND_SEND_ARRAY:
					case ZEND_SEND_UNPACK:
					case ZEND_CHECK_UNDEF_ARGS:
						if (level == 0) {
							do_exit = 1;
						}
						// 跳出 switch
						break;
				}
				// 如果没有要求退出 while
				if (!do_exit) {
					// 下一个操作码
					opline--;
				}
			// 如果没有要求退出 while
			} while (!do_exit);
			
			// 找到前一个执行数据 
			if (call->prev_execute_data) {
				// 跳过当前调用域
				/* skip current call region */
				// 调用层数
				level = 0;
				// 需要跳出while
				do_exit = 0;
				// 循环所有操作码
				do {
					switch (opline->opcode) {
						case ZEND_DO_FCALL:
						case ZEND_DO_ICALL:
						case ZEND_DO_UCALL:
						case ZEND_DO_FCALL_BY_NAME:
							// 调用层数+1
							level++;
							// 跳出 switch
							break;
						case ZEND_INIT_FCALL:
						case ZEND_INIT_FCALL_BY_NAME:
						case ZEND_INIT_NS_FCALL_BY_NAME:
						case ZEND_INIT_DYNAMIC_CALL:
						case ZEND_INIT_USER_CALL:
						case ZEND_INIT_METHOD_CALL:
						case ZEND_INIT_STATIC_METHOD_CALL:
						case ZEND_NEW:
							// 在最外层
							if (level == 0) {
								// 如果要跳出while循环
								do_exit = 1;
							}
							// 调用层数-1
							level--;
							// 跳出 switch
							break;
					}
					// 下一个操作码
					opline--;
				// 如果不要求跳出，就一直循环
				} while (!do_exit);
			}

			// 释放所有执行数据中的 函数参数
			zend_vm_stack_free_args(EX(call));

			// 如果需要释放 this
			// Z_TYPE_INFO((execute_data)->This))
			if (ZEND_CALL_INFO(call) & ZEND_CALL_RELEASE_THIS) {
				// 释放 this
				OBJ_RELEASE(Z_OBJ(call->This));
			}
			
			// 如果调用中有额外参数
			// Z_TYPE_INFO((execute_data)->This))
			if (ZEND_CALL_INFO(call) & ZEND_CALL_HAS_EXTRA_NAMED_PARAMS) {
				// 释放额外参数
				zend_free_extra_named_params(call->extra_named_params);
			}
			// 如果是调用闭包
			if (call->func->common.fn_flags & ZEND_ACC_CLOSURE) {
				// 释放闭包对象
				zend_object_release(ZEND_CLOSURE_OBJECT(call->func));
			// 如果是通过弹跳调用
			} else if (call->func->common.fn_flags & ZEND_ACC_CALL_VIA_TRAMPOLINE) {
				// 释放函数名
				zend_string_release_ex(call->func->common.function_name, 0);
				// 释放弹跳调用
				zend_free_trampoline(call->func);
			}
			// 前一个执行数据
			EX(call) = call->prev_execute_data;
			// 释放调用框架（执行数据）。p1:执行数据
			zend_vm_stack_free_call_frame(call);
			// 取得执行数据中的调用
			call = EX(call);
		} while (call);
	}
}
/* }}} */

// ing3, 查找操作码序号和组序号都匹配的活动区域。p1:操作码组，p2:操作码序号，p3:组序号
static const zend_live_range *find_live_range(const zend_op_array *op_array, uint32_t op_num, uint32_t var_num) /* {{{ */
{
	int i;
	// 遍历所有活动区间
	for (i = 0; i < op_array->last_live_range; i++) {
		// 取出活动区间
		const zend_live_range *range = &op_array->live_range[i];
		// 如果 操作码编号在区间内 并且 要求的序号和 区域的序号（去掉标记位） 相等 ?
		if (op_num >= range->start && op_num < range->end
				&& var_num == (range->var & ~ZEND_LIVE_MASK)) {
			// 返回此区域
			return range;
		}
	}
	// 返回null
	return NULL;
}
/* }}} */

// ing2, 清理区域中的变量。 p1:执行数据，p2:包含操作码数据，p3:catch_op_num (?)
static void cleanup_live_vars(zend_execute_data *execute_data, uint32_t op_num, uint32_t catch_op_num) /* {{{ */
{
	int i;

	// 遍历所有活动区域
	for (i = 0; i < EX(func)->op_array.last_live_range; i++) {
		// 取出活动区域
		const zend_live_range *range = &EX(func)->op_array.live_range[i];
		// 如果 编号不在此区域中
		if (range->start > op_num) {
			/* further blocks will not be relevant... */
			// 跳过
			break;
		// 在此区域中
		} else if (op_num < range->end) {
			// 没有 catch序号 或 catch号 不在此域
			if (!catch_op_num || catch_op_num >= range->end) {
				// 取得live类型
				uint32_t kind = range->var & ZEND_LIVE_MASK;
				// 取得变量序号
				uint32_t var_num = range->var & ~ZEND_LIVE_MASK;
				// 取得此序号的变量
				// #define EX_VAR(n) ZEND_CALL_VAR(execute_data, n)
				// #define ZEND_CALL_VAR(call, n) ((zval*)(((char*)(call)) + ((int)(n))))
				zval *var = EX_VAR(var_num);

				// 类型为：
				if (kind == ZEND_LIVE_TMPVAR) {
					// 销毁没有引用次数的对象,并没有销毁 zval本身。nogc的意思是，不放入gc回收周期。
					zval_ptr_dtor_nogc(var);
				// ZEND_LIVE_NEW
				} else if (kind == ZEND_LIVE_NEW) {
					zend_object *obj;
					// 变量类型必须 是对象
					ZEND_ASSERT(Z_TYPE_P(var) == IS_OBJECT);
					// 取出对象
					obj = Z_OBJ_P(var);
					// 给对象添加【已销毁】标记
					zend_object_store_ctor_failed(obj);
					// 释放对象
					OBJ_RELEASE(obj);
				// ZEND_LIVE_LOOP
				} else if (kind == ZEND_LIVE_LOOP) {
					// 如果变量类型不是数组  并且 迭代序号不是 -1
					if (Z_TYPE_P(var) != IS_ARRAY && Z_FE_ITER_P(var) != (uint32_t)-1) {
						// 删除 指定序号的 哈希表迭代器
						zend_hash_iterator_del(Z_FE_ITER_P(var));
					}
					// 销毁没有引用次数的对象,并没有销毁 zval本身。nogc的意思是，不放入gc回收周期。
					zval_ptr_dtor_nogc(var);
				// ZEND_LIVE_ROPE
				} else if (kind == ZEND_LIVE_ROPE) {
					// 变量 转成 zend_string 指针列表
					zend_string **rope = (zend_string **)var;
					// 最后一个操作码
					zend_op *last = EX(func)->op_array.opcodes + op_num;
					// ？ 如果不是 ZEND_ROPE_ADD 也不是 ZEND_ROPE_INIT 或 操作码结果!=变量序号 
					while ((last->opcode != ZEND_ROPE_ADD && last->opcode != ZEND_ROPE_INIT)
							|| last->result.var != var_num) {
						// 必须没到最后一个操作码
						ZEND_ASSERT(last >= EX(func)->op_array.opcodes);
						// 下一个操作码
						last--;
					}
					// ZEND_ROPE_INIT
					if (last->opcode == ZEND_ROPE_INIT) {
						// 释放 rope
						zend_string_release_ex(*rope, 0);
					// 其他情况
					} else {
						// 扩展信息（rope数量？）
						int j = last->extended_value;
						// 循环销毁 zend_string
						do {
							// 释放zend_string
							zend_string_release_ex(rope[j], 0);
						// 下一个
						} while (j--);
					}
				// ZEND_LIVE_SILENCE
				} else if (kind == ZEND_LIVE_SILENCE) {
					// 保存前一个 error_reporting值
					/* restore previous error_reporting value */
					// 如果只显示致使错误 并且 var里 不是只显示致使错误
					if (E_HAS_ONLY_FATAL_ERRORS(EG(error_reporting))
							&& !E_HAS_ONLY_FATAL_ERRORS(Z_LVAL_P(var))) {
						// 更新 EG(error_reporting)
						EG(error_reporting) = Z_LVAL_P(var);
					}
				}
			}
		}
	}
}
/* }}} */

// ing3, 清理未完成的执行。p1:执行数据，p2:操作码数量，p3:catch_op_num
ZEND_API void zend_cleanup_unfinished_execution(zend_execute_data *execute_data, uint32_t op_num, uint32_t catch_op_num) {
	// 清理未完成的调用。p1:执行数据，p2:操作码数量
	cleanup_unfinished_calls(execute_data, op_num);
	// 清理区域中的变量。 p1:执行数据，p2:包含操作码数据，p3:catch_op_num
	cleanup_live_vars(execute_data, op_num, catch_op_num);
}

// ing3, 回收未完成的执行。p1:执行数据，p2:调用执行数据，p3:回收缓冲区
ZEND_API ZEND_ATTRIBUTE_DEPRECATED HashTable *zend_unfinished_execution_gc(zend_execute_data *execute_data, zend_execute_data *call, zend_get_gc_buffer *gc_buffer)
{
	// 通过 yield 暂停
	bool suspended_by_yield = false;
	// 如果 ZEND_CALL_GENERATOR （调用了生成器？）
	if (Z_TYPE_INFO(EX(This)) & ZEND_CALL_GENERATOR) {
		// 必须有返回值
		ZEND_ASSERT(EX(return_value));
		
		// 生成器对象保存在 EX(return_value)
		/* The generator object is stored in EX(return_value) */
		zend_generator *generator = (zend_generator*) EX(return_value);
		// 生成器 必须 属于这个执行数据
		ZEND_ASSERT(execute_data == generator->execute_data);

		// 生成器当前 没有在运行，就算暂停
		suspended_by_yield = !(generator->flags & ZEND_GENERATOR_CURRENTLY_RUNNING);
	}
	
	// 回收未完成的执行。p1:执行数据，p2:调用执行数据，p3:回收缓冲区，p4:是否被yield暂停
	return zend_unfinished_execution_gc_ex(execute_data, call, gc_buffer, suspended_by_yield);
}

// ing3, 回收未完成的执行，并返回符号表。p1:执行数据，p2:调用执行数据，p3:回收缓冲区，p4:是否被yield暂停
ZEND_API HashTable *zend_unfinished_execution_gc_ex(zend_execute_data *execute_data, zend_execute_data *call, zend_get_gc_buffer *gc_buffer, bool suspended_by_yield)
{
	// 如果 执行数据中没有函数 或 函数不是用户定义的
	if (!EX(func) || !ZEND_USER_CODE(EX(func)->common.type)) {
		// 返回 null
		return NULL;
	}

	// 函数操作码列表
	zend_op_array *op_array = &EX(func)->op_array;

	// 如果， Z_TYPE_INFO((execute_data)->This)) 中没有符号表
	if (!(EX_CALL_INFO() & ZEND_CALL_HAS_SYMBOL_TABLE)) {
		// 操作码中编译变量数量
		uint32_t i, num_cvs = EX(func)->op_array.last_var;
		// 遍历所有编译变量
		for (i = 0; i < num_cvs; i++) {
			// 给游标（p1->cur）指向的zval赋值
			zend_get_gc_buffer_add_zval(gc_buffer, EX_VAR_NUM(i));
		}
	}

	// 如果， Z_TYPE_INFO((execute_data)->This)) 中没有 额外参数
	if (EX_CALL_INFO() & ZEND_CALL_FREE_EXTRA_ARGS) {
		// 编译变量数量 + 临时变量数量
		zval *zv = EX_VAR_NUM(op_array->last_var + op_array->T);
		// 参数结尾处（不包含额外参数）
		zval *end = zv + (EX_NUM_ARGS() - op_array->num_args);
		// 遍历 所有参数
		while (zv != end) {
			// 给游标（p1->cur）指向的zval赋值
			zend_get_gc_buffer_add_zval(gc_buffer, zv++);
		}
	}

	// 如果需要释放 this
	if (EX_CALL_INFO() & ZEND_CALL_RELEASE_THIS) {
		// 给游标指向的zval赋值添加对象，完全是外部调用
		zend_get_gc_buffer_add_obj(gc_buffer, Z_OBJ(execute_data->This));
	}
	
	// 如果是调用闭包
	if (EX_CALL_INFO() & ZEND_CALL_CLOSURE) {
		// 给游标指向的zval赋值添加对象，完全是外部调用
		zend_get_gc_buffer_add_obj(gc_buffer, ZEND_CLOSURE_OBJECT(EX(func)));
	}
	
	// 如果有额外的命名参数
	if (EX_CALL_INFO() & ZEND_CALL_HAS_EXTRA_NAMED_PARAMS) {
		zval extra_named_params;
		// 先把参数保存到临时变量中
		ZVAL_ARR(&extra_named_params, EX(extra_named_params));
		// 给游标（p1->cur）指向的zval赋值
		zend_get_gc_buffer_add_zval(gc_buffer, &extra_named_params);
	}

	// 如果有传入调用
	if (call) {
		// 操作码数量
		uint32_t op_num = execute_data->opline - op_array->opcodes;
		// 如果生成器已暂停
		if (suspended_by_yield) {
			// 如果执行被yield暂停了，EX(opline)指向下一个要执行的操作码。
			// 否则 ，它指向暂停执行的操作码
			/* When the execution was suspended by yield, EX(opline) points to
			 * next opline to execute. Otherwise, it points to the opline that
			 * suspended execution. */
			// 操作码数量 -1
			op_num--;
			// 最后一个操作码必须是 ZEND_YIELD 或 ZEND_YIELD_FROM
			ZEND_ASSERT(EX(func)->op_array.opcodes[op_num].opcode == ZEND_YIELD
				|| EX(func)->op_array.opcodes[op_num].opcode == ZEND_YIELD_FROM);
		}
		// 对未完成的调用进行清理，把相关变量放入垃圾回收，(操作码意义还不够清晰)
		// p1:执行数据, p2:调用的执行数据，p3:操作码数量，p4:垃圾回收位置
		zend_unfinished_calls_gc(execute_data, call, op_num, gc_buffer);
	}

	// 如果两个操作码有中间段 ？
	if (execute_data->opline != op_array->opcodes) {
		// 取得操作码数量
		uint32_t i, op_num = execute_data->opline - op_array->opcodes - 1;
		// 遍历所有区域
		for (i = 0; i < op_array->last_live_range; i++) {
			// 取出域
			const zend_live_range *range = &op_array->live_range[i];
			// 如果在区域外
			if (range->start > op_num) {
				// 跳出
				break;
			// 如果在区域内
			} else if (op_num < range->end) {
				// 变量类型
				uint32_t kind = range->var & ZEND_LIVE_MASK;
				// 变量编号
				uint32_t var_num = range->var & ~ZEND_LIVE_MASK;
				// 取出变量
				// #define EX_VAR(n) ZEND_CALL_VAR(execute_data, n)
				// #define ZEND_CALL_VAR(call, n) ((zval*)(((char*)(call)) + ((int)(n))))
				zval *var = EX_VAR(var_num);
				// ？
				if (kind == ZEND_LIVE_TMPVAR || kind == ZEND_LIVE_LOOP) {
					// 给游标（p1->cur）指向的zval赋值
					zend_get_gc_buffer_add_zval(gc_buffer, var);
				}
			}
		}
	}

	// 如果调用中有 符号表
	if (EX_CALL_INFO() & ZEND_CALL_HAS_SYMBOL_TABLE) {
		// 返回符号表
		return execute_data->symbol_table;
	// 否则
	} else {
		// 返回null
		return NULL;
	}
}

#if ZEND_VM_SPEC
// clear 交换操作对象，把op下面的op1和op2互换
static void zend_swap_operands(zend_op *op) /* {{{ */
{
	znode_op     tmp;
	zend_uchar   tmp_type;

	tmp          = op->op1;
	tmp_type     = op->op1_type;
	op->op1      = op->op2;
	op->op1_type = op->op2_type;
	op->op2      = tmp;
	op->op2_type = tmp_type;
}
/* }}} */
#endif

// ing3, 通过字串创建调用 执行数据，不要弹跳创建的。p1:调用方法名（函数名或静态方法名），p2:参数数量
static zend_never_inline zend_execute_data *zend_init_dynamic_call_string(zend_string *function, uint32_t num_args) /* {{{ */
{
	// 调用方法
	zend_function *fbc;
	// 调用函数
	zval *func;
	// 调用类入口
	zend_class_entry *called_scope;
	// 小写类名
	zend_string *lcname;
	// 字串中冒号位置
	const char *colon;

	// 如果找到 ::
	if ((colon = zend_memrchr(ZSTR_VAL(function), ':', ZSTR_LEN(function))) != NULL &&
		colon > ZSTR_VAL(function) &&
		*(colon-1) == ':'
	) {
		// 方法名
		zend_string *mname;
		// 类名长度
		size_t cname_length = colon - ZSTR_VAL(function) - 1;
		// 方法名长度
		size_t mname_length = ZSTR_LEN(function) - cname_length - (sizeof("::") - 1);

		// 新字串，小写类名
		lcname = zend_string_init(ZSTR_VAL(function), cname_length, 0);
		// 获取类，找不到会报错。p1:类名，p2:key，p3:flags
		called_scope = zend_fetch_class_by_name(lcname, NULL, ZEND_FETCH_CLASS_DEFAULT | ZEND_FETCH_CLASS_EXCEPTION);
		// 如果查找类失败
		if (UNEXPECTED(called_scope == NULL)) {
			// 释放类名
			zend_string_release_ex(lcname, 0);
			// 返回null
			return NULL;
		}
		// 方法名字串
		mname = zend_string_init(ZSTR_VAL(function) + (cname_length + sizeof("::") - 1), mname_length, 0);

		// 如果找到类 并且类有 get_static_method 方法
		if (called_scope->get_static_method) {
			// 使用自己的方法获取静态方法
			fbc = called_scope->get_static_method(called_scope, mname);
		// 类没有 get_static_method 方法
		} else {
			// 调用标准方法 获取静态方法。p1:类，p2:方法名，p3:key
			fbc = zend_std_get_static_method(called_scope, mname, NULL);
		}
		// 如果没获取到方法
		if (UNEXPECTED(fbc == NULL)) {
			// 没有异常
			if (EXPECTED(!EG(exception))) {
				// 报错：调用未定义的方法
				zend_undefined_method(called_scope, mname);
			}
			// 释放类型名和方法名
			zend_string_release_ex(lcname, 0);
			zend_string_release_ex(mname, 0);
			// 返回 null
			return NULL;
		}
		// 释放类型名和方法名
		zend_string_release_ex(lcname, 0);
		zend_string_release_ex(mname, 0);

		// 如果获取到的方法不是静态的
		if (UNEXPECTED(!(fbc->common.fn_flags & ZEND_ACC_STATIC))) {
			// 报错：静态调用非静态方法
			zend_non_static_method_call(fbc);
			// 如果是通过 弹跳 获取的方法
			if (fbc->common.fn_flags & ZEND_ACC_CALL_VIA_TRAMPOLINE) {
				// 释放方法名
				zend_string_release_ex(fbc->common.function_name, 0);
				// 释放弹跳方法
				zend_free_trampoline(fbc);
			}
			// 返回 null
			return NULL;
		}
		// 如果是用户函数 并且 没有运行时缓存
		if (EXPECTED(fbc->type == ZEND_USER_FUNCTION) && UNEXPECTED(!RUN_TIME_CACHE(&fbc->op_array))) {
			// 为op_array创建并 初始化运行时缓存
			init_func_run_time_cache(&fbc->op_array);
		}
	// 调用函数
	} else {
		// 如果是完整路径调用
		if (ZSTR_VAL(function)[0] == '\\') {
			// 新建字串
			lcname = zend_string_alloc(ZSTR_LEN(function) - 1, 0);
			// 更别提小写函数名，不要开头的 \ 
			zend_str_tolower_copy(ZSTR_VAL(lcname), ZSTR_VAL(function) + 1, ZSTR_LEN(function) - 1);
		// 不是完整路径
		} else {
			// 函数名转小写
			lcname = zend_string_tolower(function);
		}
		// 在执行时函数列表中查找此函数，如果找不到
		if (UNEXPECTED((func = zend_hash_find(EG(function_table), lcname)) == NULL)) {
			// 报错：调用未定义的函数 
			zend_throw_error(NULL, "Call to undefined function %s()", ZSTR_VAL(function));
			// 释放函数名
			zend_string_release_ex(lcname, 0);
			// 返回null
			return NULL;
		}
		// 释放函数名
		zend_string_release_ex(lcname, 0);

		// 取出函数实例
		fbc = Z_FUNC_P(func);
		// 如果是用户定义函数 并且 没有运行时缓存
		if (EXPECTED(fbc->type == ZEND_USER_FUNCTION) && UNEXPECTED(!RUN_TIME_CACHE(&fbc->op_array))) {
			// 为op_array创建并 初始化运行时缓存
			init_func_run_time_cache(&fbc->op_array);
		}
		// 所属类为空
		called_scope = NULL;
	}

	// 初始化并返回 堆栈最上一个执行数据，空间不够则扩展大小
	// p1:调用信息，p2:函数，p3:参数数量，p4:对象或调用域
	return zend_vm_stack_push_call_frame(ZEND_CALL_NESTED_FUNCTION | ZEND_CALL_DYNAMIC,
		fbc, num_args, called_scope);
}
/* }}} */

// ing3, 创建调用闭包（或假闭包）执行数据。p1:用来获取闭包的对象，p2:参数数量
static zend_never_inline zend_execute_data *zend_init_dynamic_call_object(zend_object *function, uint32_t num_args) /* {{{ */
{
	zend_function *fbc;
	void *object_or_called_scope;
	zend_class_entry *called_scope;
	zend_object *object;
	uint32_t call_info;

	// 如果函数有获取闭包方法 并且 获取闭包成功
	if (EXPECTED(function->handlers->get_closure) &&
		// 取得闭包，对于标准对象，这个是取得__invoke方法.
		// p1:对象，p2:返回类指针，p3:返回方法指针，p4:返回对象指针，p5:仅检查
	    EXPECTED(function->handlers->get_closure(function, &called_scope, &fbc, &object, 0) == SUCCESS)) {
		// 调用
		object_or_called_scope = called_scope;
		// 如果函数是 闭包
		if (EXPECTED(fbc->common.fn_flags & ZEND_ACC_CLOSURE)) {
			// 延时销毁闭包，直到它的 invocation
			/* Delay closure destruction until its invocation */
			
			// 闭包对象添加引用次数
			GC_ADDREF(ZEND_CLOSURE_OBJECT(fbc));
			// 两个都是  (1 << 23) 
			ZEND_ASSERT(ZEND_ACC_FAKE_CLOSURE == ZEND_CALL_FAKE_CLOSURE);
			// 调用信息 = 嵌套调用 | 动态调用 | 调用闭包 | 按需要（ZEND_ACC_FAKE_CLOSURE）
			call_info = ZEND_CALL_NESTED_FUNCTION | ZEND_CALL_DYNAMIC | ZEND_CALL_CLOSURE |
				(fbc->common.fn_flags & ZEND_ACC_FAKE_CLOSURE);
			// 如果返回对象有效
			if (object) {
				// 释放this | 有this 
				call_info |= ZEND_CALL_HAS_THIS;
				object_or_called_scope = object;
			}
		// 函数不是闭包
		} else {
			// 嵌套调用 | 动态调用
			call_info = ZEND_CALL_NESTED_FUNCTION | ZEND_CALL_DYNAMIC;
			// 如果有返回所属对象
			if (object) {
				// 释放this | 有this 
				call_info |= ZEND_CALL_RELEASE_THIS | ZEND_CALL_HAS_THIS;
				// 对象增加引用次数
				GC_ADDREF(object); /* For $this pointer */
				// 调用对象或域
				object_or_called_scope = object;
			}
		}
	} else {
		// 报错：此类型的对象不可以调用
		zend_throw_error(NULL, "Object of type %s is not callable", ZSTR_VAL(function->ce->name));
		// 返回 null
		return NULL;
	}

	// 如果是用户定义 并且 没有运行时缓存
	if (EXPECTED(fbc->type == ZEND_USER_FUNCTION) && UNEXPECTED(!RUN_TIME_CACHE(&fbc->op_array))) {
		// 为op_array创建并 初始化运行时缓存
		init_func_run_time_cache(&fbc->op_array);
	}

	// 初始化并返回 堆栈最上一个执行数据，空间不够则扩展大小
	// p1:调用信息，p2:函数，p3:参数数量，p4:对象或调用域
	return zend_vm_stack_push_call_frame(call_info,
		fbc, num_args, object_or_called_scope);
}
/* }}} */

// ing3, 从数组，初始化动态调用执行数据。p1:调用数组，p2:参数数量
static zend_never_inline zend_execute_data *zend_init_dynamic_call_array(zend_array *function, uint32_t num_args) /* {{{ */
{
	zend_function *fbc;
	// 调用域：可能是类 或 对象
	void *object_or_called_scope;
	// 调用信息： 嵌套调用，动态调用
	uint32_t call_info = ZEND_CALL_NESTED_FUNCTION | ZEND_CALL_DYNAMIC;

	// 如果函数哈希表中有2个元素
	if (zend_hash_num_elements(function) == 2) {
		zval *obj;
		zval *method;
		// 第一个元素是 对象
		obj = zend_hash_index_find(function, 0);
		// 第二个元素是 方法名
		method = zend_hash_index_find(function, 1);

		// 两个里有一个不存在
		if (UNEXPECTED(!obj) || UNEXPECTED(!method)) {
			// 数组回调 必须包含两个元素
			zend_throw_error(NULL, "Array callback has to contain indices 0 and 1");
			return NULL;
		}
		// 如果是引用类型，追踪到被引用的对象
		ZVAL_DEREF(obj);
		// 如果对象不是 字串 也不是对象
		if (UNEXPECTED(Z_TYPE_P(obj) != IS_STRING) && UNEXPECTED(Z_TYPE_P(obj) != IS_OBJECT)) {
			// 抛错：第一个元素不是有效的类名或对象
			zend_throw_error(NULL, "First array member is not a valid class name or object");
			return NULL;
		}
		// 如果是引用类型，追踪到被引用的对象
		ZVAL_DEREF(method);
		// 如果方法名不是 字串
		if (UNEXPECTED(Z_TYPE_P(method) != IS_STRING)) {
			// 抛错：第二个元素不是有效的方法名
			zend_throw_error(NULL, "Second array member is not a valid method");
			return NULL;
		}

		// 如果通过 类名 调用
		if (Z_TYPE_P(obj) == IS_STRING) {
			// 获取类，找不到会报错。p1:类名，p2:key，p3:flags
			zend_class_entry *called_scope = zend_fetch_class_by_name(Z_STR_P(obj), NULL, ZEND_FETCH_CLASS_DEFAULT | ZEND_FETCH_CLASS_EXCEPTION);

			// 如果获取失败
			if (UNEXPECTED(called_scope == NULL)) {
				// 返回null
				return NULL;
			}
			
			// 如果类有 获取静态方法 的方法
			if (called_scope->get_static_method) {
				// 调用它获取静态方法
				fbc = called_scope->get_static_method(called_scope, Z_STR_P(method));
			// 如果没有
			} else {
				// 使用标准方法获取静态方法
				fbc = zend_std_get_static_method(called_scope, Z_STR_P(method), NULL);
			}
			// 没获取到静态方法
			if (UNEXPECTED(fbc == NULL)) {
				// 如果没有异常
				if (EXPECTED(!EG(exception))) {
					// 报错：调用未定义的方法
					zend_undefined_method(called_scope, Z_STR_P(method));
				}
				// 反回 null
				return NULL;
			}
			
			// 如果获取到的方法不是静态方法
			if (!(fbc->common.fn_flags & ZEND_ACC_STATIC)) {
				// 报错：静态调用非静态方法
				zend_non_static_method_call(fbc);
				// 如果方法有 通过弹跳调用 标记
				if (fbc->common.fn_flags & ZEND_ACC_CALL_VIA_TRAMPOLINE) {
					// 释放方法名
					zend_string_release_ex(fbc->common.function_name, 0);
					// 释放弹跳函数
					zend_free_trampoline(fbc);
				}
				return NULL;
			}
			// 调用的域
			object_or_called_scope = called_scope;
		// 如果通过 对象 调用
		} else {
			// 取得对象
			zend_object *object = Z_OBJ_P(obj);
			// 取得对象中的方法
			fbc = Z_OBJ_HT_P(obj)->get_method(&object, Z_STR_P(method), NULL);
			// 如果没有此方法
			if (UNEXPECTED(fbc == NULL)) {
				// 如果没有异常 
				if (EXPECTED(!EG(exception))) {
					// 报错：调用未定义的方法
					zend_undefined_method(object->ce, Z_STR_P(method));
				}
				// 返回null
				return NULL;
			}
			// 如果是静态方法
			if ((fbc->common.fn_flags & ZEND_ACC_STATIC) != 0) {
				// 调用域，用此对象的所属类
				object_or_called_scope = object->ce;
			// 不是静态方法
			} else {
				// 调用信息 添加 ZEND_CALL_RELEASE_THIS | ZEND_CALL_HAS_THIS 标记
				call_info |= ZEND_CALL_RELEASE_THIS | ZEND_CALL_HAS_THIS;
				// 对象添加引用次数（给$this指针）
				GC_ADDREF(object); /* For $this pointer */
				// 调用域，用此对象本身
				object_or_called_scope = object;
			}
		}
	// 如果不是2个元素
	} else {
		// 报错：数组回调必须有2个元素
		zend_throw_error(NULL, "Array callback must have exactly two elements");
		// 返回null
		return NULL;
	}

	// 如果是用户定义函数 并且 没有运行时缓存
	if (EXPECTED(fbc->type == ZEND_USER_FUNCTION) && UNEXPECTED(!RUN_TIME_CACHE(&fbc->op_array))) {
		// 为op_array创建并 初始化运行时缓存
		init_func_run_time_cache(&fbc->op_array);
	}

	// 初始化并返回 堆栈最上一个执行数据，空间不够则扩展大小
	// p1:调用信息，p2:函数，p3:参数数量，p4:对象或调用域
	return zend_vm_stack_push_call_frame(call_info,
		fbc, num_args, object_or_called_scope);
}
/* }}} */

#define ZEND_FAKE_OP_ARRAY ((zend_op_array*)(zend_intptr_t)-1)

// ing3, include 或 eval 函数的执行逻辑。p1:文件名，p2:类型
static zend_never_inline zend_op_array* ZEND_FASTCALL zend_include_or_eval(zval *inc_filename_zv, int type) /* {{{ */
{
	// 操作码组
	zend_op_array *new_op_array = NULL;
	// 包含文件名
	zend_string *tmp_inc_filename;
	// 被包含的文件名
	// 获取临时字串（不抛错）返回
	zend_string *inc_filename = zval_try_get_tmp_string(inc_filename_zv, &tmp_inc_filename);
	// 如果获取文件名失败，返回
	if (UNEXPECTED(!inc_filename)) {
		// 返回 null
		return NULL;
	}

	// 按调用函数 分 3种情况
	switch (type) {
		// include_once 或 require_once
		case ZEND_INCLUDE_ONCE:
		case ZEND_REQUIRE_ONCE: {
				//
				zend_file_handle file_handle;
				//
				zend_string *resolved_path;
				// 获取完整路径
				// 	zend_resolve_path = utility_functions->resolve_path_function = php_resolve_path_for_zend:main.c
				resolved_path = zend_resolve_path(inc_filename);
				
				// 如果有完整路径
				if (EXPECTED(resolved_path)) {
					if (zend_hash_exists(&EG(included_files), resolved_path)) {
						new_op_array = ZEND_FAKE_OP_ARRAY;
						zend_string_release_ex(resolved_path, 0);
						break;
					}
				// 如果有异常
				} else if (UNEXPECTED(EG(exception))) {
					// 跳出
					break;
				// 如果文件名错误
				} else if (UNEXPECTED(strlen(ZSTR_VAL(inc_filename)) != ZSTR_LEN(inc_filename))) {
					// 报错：无法打开文件
					zend_message_dispatcher(
						(type == ZEND_INCLUDE_ONCE) ?
							ZMSG_FAILED_INCLUDE_FOPEN : ZMSG_FAILED_REQUIRE_FOPEN,
							ZSTR_VAL(inc_filename));
					break;
				// 其他情况
				} else {
					// 包含文件路径副本
					resolved_path = zend_string_copy(inc_filename);
				}

				// 初始货文件句柄
				zend_stream_init_filename_ex(&file_handle, resolved_path);
				// 如果打开文件成功
				if (SUCCESS == zend_stream_open(&file_handle)) {
					// 如果句柄中没有文件路径
					if (!file_handle.opened_path) {
						// 设置文件路径
						file_handle.opened_path = zend_string_copy(resolved_path);
					}

					// 把打开路径添加进包含文件中，如果成功
					if (zend_hash_add_empty_element(&EG(included_files), file_handle.opened_path)) {
						// 编译文件，取回操作码
						new_op_array = zend_compile_file(&file_handle, (type==ZEND_INCLUDE_ONCE?ZEND_INCLUDE:ZEND_REQUIRE));
					// 如果文件已经 include过
					} else {
						// 返回伪操作码 ZEND_FAKE_OP_ARRAY = -1
						new_op_array = ZEND_FAKE_OP_ARRAY;
					}
				// 如果没有异常
				} else if (!EG(exception)) {
					// 报错：无法打开文件
					zend_message_dispatcher(
						(type == ZEND_INCLUDE_ONCE) ?
							ZMSG_FAILED_INCLUDE_FOPEN : ZMSG_FAILED_REQUIRE_FOPEN,
							ZSTR_VAL(inc_filename));
				}
				// 销毁文件句柄
				zend_destroy_file_handle(&file_handle);
				// 释放路径
				zend_string_release_ex(resolved_path, 0);
			}
			break;
		// include 或 require
		case ZEND_INCLUDE:
		case ZEND_REQUIRE:
			// 文件名称字串有异常
			if (UNEXPECTED(strlen(ZSTR_VAL(inc_filename)) != ZSTR_LEN(inc_filename))) {
				// 报错：文件名错误，无法打开
				zend_message_dispatcher(
					(type == ZEND_INCLUDE) ?
						ZMSG_FAILED_INCLUDE_FOPEN : ZMSG_FAILED_REQUIRE_FOPEN,
						ZSTR_VAL(inc_filename));
				// 跳出
				break;
			}
			// 编译文件取回操作码组
			new_op_array = compile_filename(type, inc_filename);
			// 跳出
			break;
		// eval
		case ZEND_EVAL: {
				// zend.c
				char *eval_desc = zend_make_compiled_string_description("eval()'d code");
				// 直接编译字串
				new_op_array = zend_compile_string(inc_filename, eval_desc, ZEND_COMPILE_POSITION_AFTER_OPEN_TAG);
				// 释放返回的字串
				efree(eval_desc);
			}
			break;
		EMPTY_SWITCH_DEFAULT_CASE()
	}
	// 释放文件名
	zend_tmp_string_release(tmp_inc_filename);
	// 返回操作码组
	return new_op_array;
}
/* }}} */

// ing3, 重置迭代器，并解除和对象的关联，返回当前位置是否为空。p1:传入类数组对象 zval，p2:是否引用
static zend_never_inline bool ZEND_FASTCALL zend_fe_reset_iterator(zval *array_ptr, int by_ref OPLINE_DC EXECUTE_DATA_DC) /* {{{ */
{
	// 对象所属类
	zend_class_entry *ce = Z_OBJCE_P(array_ptr);
	// 取得对象的迭代器
	zend_object_iterator *iter = ce->get_iterator(ce, array_ptr, by_ref);
	//
	bool is_empty;

	// 如果迭代器不存在 或 有异常
	if (UNEXPECTED(!iter) || UNEXPECTED(EG(exception))) {
		// 如果有迭代器
		if (iter) {
			// 释放迭代器
			OBJ_RELEASE(&iter->std);
		}
		// 没有异常
		if (!EG(exception)) {
			// 报错：此类型的对象无法创建迭代器
			zend_throw_exception_ex(NULL, 0, "Object of type %s did not create an Iterator", ZSTR_VAL(ce->name));
		}
		// 操作码结果 设置为未定义
		ZVAL_UNDEF(EX_VAR(opline->result.var));
		// 返回 成功。并没有释放迭代器。
		return 1;
	}

	// 重置迭代器指针
	iter->index = 0;
	// 如果有重置功能
	if (iter->funcs->rewind) {
		// 重置迭代器
		iter->funcs->rewind(iter);
		// 如果有异常
		if (UNEXPECTED(EG(exception) != NULL)) {
			// 释放迭代器中的对象
			OBJ_RELEASE(&iter->std);
			// 操作码结果 设置为未定义
			ZVAL_UNDEF(EX_VAR(opline->result.var));
			// 返回成功。并没有释放迭代器。
			return 1;
		}
	}

	// 返回：当前位置无效
	is_empty = iter->funcs->valid(iter) != SUCCESS;

	// 如果有异常
	if (UNEXPECTED(EG(exception) != NULL)) {
		// 释放迭代器中的对象
		OBJ_RELEASE(&iter->std);
		// 操作码结果 设置为未定义
		ZVAL_UNDEF(EX_VAR(opline->result.var));
		// 返回成功。并没有释放迭代器。
		return 1;
	}
	
	// 一直没有销毁迭代器。这里是什么处理 ？
	
	// 当前指向位置为-1。 在使用下个处理器这前它会被设置成0
	iter->index = -1; /* will be set to 0 before using next handler */
	// 把迭代器指向的对象，给操作码结果
	// #define EX_VAR(n) ZEND_CALL_VAR(execute_data, n)
	// #define ZEND_CALL_VAR(call, n) ((zval*)(((char*)(call)) + ((int)(n))))
	ZVAL_OBJ(EX_VAR(opline->result.var), &iter->std);
	
	// 清空对象的迭代器索引号（不再关联这个迭代器）
	// 通过指针访问zval的 迭代索引号
	Z_FE_ITER_P(EX_VAR(opline->result.var)) = (uint32_t)-1;

	// 返回当前指向位置是否为空
	return is_empty;
}
/* }}} */

// ing3, 快速读取常量，p1:常量名，p2:flags，p3:只否检查声名
static zend_always_inline zend_result _zend_quick_get_constant(
		const zval *key, uint32_t flags, bool check_defined_only OPLINE_DC EXECUTE_DATA_DC) /* {{{ */
{
	zval *zv;
	zend_constant *c = NULL;

	// null/true/false 在编译时解决过了，所以不用再检验了
	/* null/true/false are resolved during compilation, so don't check for them here. */
	// 在常量表中查找常量
	zv = zend_hash_find_known_hash(EG(zend_constants), Z_STR_P(key));
	// 如果找到
	if (zv) {
		// 取出 zval中的常量
		c = (zend_constant*)Z_PTR_P(zv);
	// 如果没找到 ，并且有 IS_CONSTANT_UNQUALIFIED_IN_NAMESPACE 标记
	} else if (flags & IS_CONSTANT_UNQUALIFIED_IN_NAMESPACE) {
		// 取得下一个key
		key++;
		// 在常量表中查找常量
		zv = zend_hash_find_known_hash(EG(zend_constants), Z_STR_P(key));
		// 如果找到
		if (zv) {
			// 取出 zval中的常量
			c = (zend_constant*)Z_PTR_P(zv);
		}
	}

	// 如果没找到常量
	if (!c) {
		// 如果不只是检验存在
		if (!check_defined_only) {
			// 报错：常量未定义
			// 在p1中通过偏移量p2.constant 获取 zval 指针
			zend_throw_error(NULL, "Undefined constant \"%s\"", Z_STRVAL_P(RT_CONSTANT(opline, opline->op2)));
			// 结果 设置成未定义
			// #define EX_VAR(n) ZEND_CALL_VAR(execute_data, n)
			// #define ZEND_CALL_VAR(call, n) ((zval*)(((char*)(call)) + ((int)(n))))
			ZVAL_UNDEF(EX_VAR(opline->result.var));
		}
		// 返回失败
		return FAILURE;
	}

	// 如果不只是检验存在
	if (!check_defined_only) {
		// 值复制进 opline 结果
		ZVAL_COPY_OR_DUP(EX_VAR(opline->result.var), &c->value);
		// 如果常量有弃用标记
		if (ZEND_CONSTANT_FLAGS(c) & CONST_DEPRECATED) {
			// 报错：常量已弃用
			zend_error(E_DEPRECATED, "Constant %s is deprecated", ZSTR_VAL(c->name));
			// 还是返回成功
			return SUCCESS;
		}
	}
	// 把常量保存到运行时缓存中
	// 通过偏移量 在 EX(run_time_cache) 中获取一个 (void**) 中的第一个元素，并给它赋值
	CACHE_PTR(opline->extended_value, c);
	// 返回成功
	return SUCCESS;
}
/* }}} */

// ing3, 快速读取常量。p1:常量名，p2:flags
static zend_never_inline void ZEND_FASTCALL zend_quick_get_constant(
		const zval *key, uint32_t flags OPLINE_DC EXECUTE_DATA_DC) /* {{{ */
{
	// 快速读取常量，p1:常量名，p2:flags，p3:只否检查声名
	_zend_quick_get_constant(key, flags, 0 OPLINE_CC EXECUTE_DATA_CC);
} /* }}} */

// ing3, 快速检测常量存在。p1:常量名，p2:flags
static zend_never_inline zend_result ZEND_FASTCALL zend_quick_check_constant(
		const zval *key OPLINE_DC EXECUTE_DATA_DC) /* {{{ */
{
	// 快速读取常量，p1:常量名，p2:flags，p3:只否检查声名
	return _zend_quick_get_constant(key, 0, 1 OPLINE_CC EXECUTE_DATA_CC);
} /* }}} */

// ing3, 通过参数名称获取偏移量（参数序号）
static zend_always_inline uint32_t zend_get_arg_offset_by_name(
		zend_function *fbc, zend_string *arg_name, void **cache_slot) {
	// 如果缓存位置 和当前 函数指针相同
	if (EXPECTED(*cache_slot == fbc)) {
		// 返回下一个缓存位置（转成int指针）
		return *(uintptr_t *)(cache_slot + 1);
	}

	// TODO: Use a hash table?
	// 函数的参数数量
	uint32_t num_args = fbc->common.num_args;
	// 如果函数类型是用户定义函数 
	if (EXPECTED(fbc->type == ZEND_USER_FUNCTION)
			// 或者带有 ZEND_ACC_USER_ARG_INFO （Closure::__invoke() 使用 的方法 flag，只能是 int 型）
			|| EXPECTED(fbc->common.fn_flags & ZEND_ACC_USER_ARG_INFO)) {
		// 遍历所有参数
		for (uint32_t i = 0; i < num_args; i++) {
			// 取得参数信息
			zend_arg_info *arg_info = &fbc->op_array.arg_info[i];
			// 如果参数名匹配
			if (zend_string_equals(arg_name, arg_info->name)) {
				// 把当前函数指针存到缓存位置
				*cache_slot = fbc;
				// 下一个缓存位置放 参数序号（所以上面可以这样取）
				*(uintptr_t *)(cache_slot + 1) = i;
				// 返回参数序号
				return i;
			}
		}
	// 如果是内置函数：不是用户定义，并且没有 ZEND_ACC_USER_ARG_INFO 
	} else {
		// 遍历所有参数
		for (uint32_t i = 0; i < num_args; i++) {
			// 获取内置函数参数信息
			zend_internal_arg_info *arg_info = &fbc->internal_function.arg_info[i];
			// 参数名长度
			size_t len = strlen(arg_info->name);
			// 如果参数名匹配
			if (zend_string_equals_cstr(arg_name, arg_info->name, len)) {
				// 和上面操作一样，把当前函数指针存到缓存位置
				*cache_slot = fbc;
				// 下一个缓存位置放 参数序号
				*(uintptr_t *)(cache_slot + 1) = i;
				// 返回参数序号
				return i;
			}
		}
	}
	
	// 如果上面没匹配到
	// 如果函数支持字典参数
	if (fbc->common.fn_flags & ZEND_ACC_VARIADIC) {
		// 把当前函数指针存到缓存位置
		*cache_slot = fbc;
		// 下一个缓存位置放 参数数量 
		*(uintptr_t *)(cache_slot + 1) = fbc->common.num_args;
		// 返回参数数量
		return fbc->common.num_args;
	}

	// 查找失败
	return (uint32_t) -1;
}

// ing3, 处理命名参数，p1:执行数据指针的指针，p2:参数名，p3:参数数量 ，p4:缓存位置
zval * ZEND_FASTCALL zend_handle_named_arg(
		zend_execute_data **call_ptr, zend_string *arg_name,
		uint32_t *arg_num_ptr, void **cache_slot) {
	// 执行数据指针
	zend_execute_data *call = *call_ptr;
	// 执行数据中的函数
	zend_function *fbc = call->func;
	// 获取参数序号
	uint32_t arg_offset = zend_get_arg_offset_by_name(fbc, arg_name, cache_slot);
	// 如果序号无效
	if (UNEXPECTED(arg_offset == (uint32_t) -1)) {
		// 抛错：未知的命名参数 
		zend_throw_error(NULL, "Unknown named parameter $%s", ZSTR_VAL(arg_name));
		// 返回null
		return NULL;
	}

	zval *arg;
	// 如果参数在列表后面
	if (UNEXPECTED(arg_offset == fbc->common.num_args)) {
		// 未知的命名参数会被收集到字典中
		/* Unknown named parameter that will be collected into a variadic. */
		// 如果调用中不包含额外参数
		// Z_TYPE_INFO((execute_data)->This))
		if (!(ZEND_CALL_INFO(call) & ZEND_CALL_HAS_EXTRA_NAMED_PARAMS)) {
			// 添加额外参数标记， 
			// Z_TYPE_INFO((p1)->This) |= p2
			ZEND_ADD_CALL_FLAG(call, ZEND_CALL_HAS_EXTRA_NAMED_PARAMS);
			// 创建额外参数列表
			call->extra_named_params = zend_new_array(0);
		}
		// 添加空元素，元素名是当前参数名
		arg = zend_hash_add_empty_element(call->extra_named_params, arg_name);
		// 如果添加失败
		if (!arg) {
			// 抛错：命名参数覆盖了前面的参数
			zend_throw_error(NULL, "Named parameter $%s overwrites previous argument",
				ZSTR_VAL(arg_name));
			// 返回 null
			return NULL;
		}
		// 指针右移
		*arg_num_ptr = arg_offset + 1;
		// 返回参数
		return arg;
	}
	// 取出$this中的参数数量 ？
	uint32_t current_num_args = ZEND_CALL_NUM_ARGS(call);
	// TODO: We may wish to optimize the arg_offset == current_num_args case,
	// which is probably common (if the named parameters are in order of declaration).
	// 如果参数序号大于参数数量 
	if (arg_offset >= current_num_args) {
		// 新参数的位置
		uint32_t new_num_args = arg_offset + 1;
		// 更新 $this中的参数数量 
		ZEND_CALL_NUM_ARGS(call) = new_num_args;
		// 额外参数数量  = 新参数数量 - 原参数数量 
		uint32_t num_extra_args = new_num_args - current_num_args;
		// 扩展执行数据。如果当堆栈前空间够用，增加使用数。
		// 否则，新建堆栈并复制执行数据。p1:执行数据，p2:传入参数，p3:增加参数
		zend_vm_stack_extend_call_frame(call_ptr, current_num_args, num_extra_args);
		// 返回的执行数据
		call = *call_ptr;

		// 取得第n个参数
		arg = ZEND_CALL_VAR_NUM(call, arg_offset);
		// 如果额外参数数 > 1
		if (num_extra_args > 1) {
			// 取得第n个参数
			zval *zv = ZEND_CALL_VAR_NUM(call, current_num_args);
			// 遍历 zv 到 arg中间这一段
			do {
				// 全都设置成 未定义
				ZVAL_UNDEF(zv);
				// 下一个参数
				zv++;
			} while (zv != arg);
			// 添加，调用可以包含未定义 标记
			//  Z_TYPE_INFO((p1)->This) |= p2
			ZEND_ADD_CALL_FLAG(call, ZEND_CALL_MAY_HAVE_UNDEF);
		}
	// 参数序号在参数数量内
	} else {
		// 取出这个参数
		arg = ZEND_CALL_VAR_NUM(call, arg_offset);
		// 如果参数不是未定义
		if (UNEXPECTED(!Z_ISUNDEF_P(arg))) {
			// 命名参数覆盖了前面的参数
			zend_throw_error(NULL, "Named parameter $%s overwrites previous argument",
				ZSTR_VAL(arg_name));
			// 返回 null
			return NULL;
		}
	}
	// 返回，参数数量
	*arg_num_ptr = arg_offset + 1;
	// 
	return arg;
}

// ing3, 更换执行数据来执行操作码, p1:执行数据，p2:操作码
// p1:作为当前执行数据来执行p2（操作码），把当前执行数据放到p1的前一个，并返回原来的前一个执行数据
static zend_execute_data *start_fake_frame(zend_execute_data *call, const zend_op *opline) {
	// call 的前一个执行数据
	zend_execute_data *old_prev_execute_data = call->prev_execute_data;
	// 更新call的前一个执行数据，把它换成当前执行数据
	call->prev_execute_data = EG(current_execute_data);
	// 更新这执行数据的操作码
	call->opline = opline;
	// 把当前执行数据换成 call
	EG(current_execute_data) = call;
	// 返回更新下来的执行数据
	return old_prev_execute_data;
}

// ing3, 结束临时作用域，恢复原有执行数据。p1:调用执行数据，p2:旧的前一个执行数据
static void end_fake_frame(zend_execute_data *call, zend_execute_data *old_prev_execute_data) {
	// 取得前一个执行数据
	zend_execute_data *prev_execute_data = call->prev_execute_data;
	// 前一个执行数据转到当前
	EG(current_execute_data) = prev_execute_data;
	// 更新前一个执行数据
	call->prev_execute_data = old_prev_execute_data;
	// 如果有并且 并且 前一个执行数据的函数是用户定义
	if (UNEXPECTED(EG(exception)) && ZEND_USER_CODE(prev_execute_data->func->common.type)) {
		// 重新抛异常
		zend_rethrow_exception(prev_execute_data);
	}
}

// ing3, 处理未定义的参数，给它们添加默认值，p1:执行数据
ZEND_API zend_result ZEND_FASTCALL zend_handle_undef_args(zend_execute_data *call) {
	// 执行数据中的函数
	zend_function *fbc = call->func;
	// 如果是用户函数 
	if (fbc->type == ZEND_USER_FUNCTION) {
		// 函数的操作码
		zend_op_array *op_array = &fbc->op_array;
		// 执行数据中的参数数量 
		uint32_t num_args = ZEND_CALL_NUM_ARGS(call);
		// 遍历 每个参数
		for (uint32_t i = 0; i < num_args; i++) {
			// 取得列表中第 5+n 个 zval
			zval *arg = ZEND_CALL_VAR_NUM(call, i);
			// 如果参数未定义
			if (!Z_ISUNDEF_P(arg)) {
				// 下一个
				continue;
			}
			// 取出这个操作码
			zend_op *opline = &op_array->opcodes[i];
			// 操作码是 ZEND_RECV_INIT
			if (EXPECTED(opline->opcode == ZEND_RECV_INIT)) {
				// 在p1中通过偏移量p2.constant 获取 zval 指针
				zval *default_value = RT_CONSTANT(opline, opline->op2);
				// 如果是表达式常量
				if (Z_OPT_TYPE_P(default_value) == IS_CONSTANT_AST) {
					// 如果没有运行时缓存
					if (UNEXPECTED(!RUN_TIME_CACHE(op_array))) {
						// 初始化运行时缓存 
						init_func_run_time_cache(op_array);
					}
					// 运行时缓存指针
					void *run_time_cache = RUN_TIME_CACHE(op_array);
					
					// 取得运行缓存中的变量
					//  #define Z_CACHE_SLOT(zval)			(zval).u2.cache_slot
					// 	#define Z_CACHE_SLOT_P(zval_p) Z_CACHE_SLOT(*(zval_p))
					zval *cache_val =
						(zval *) ((char *) run_time_cache + Z_CACHE_SLOT_P(default_value));

					// 如果缓存值有效
					if (Z_TYPE_P(cache_val) != IS_UNDEF) {
						// 缓存中只保存没有引用次数的值
						/* We keep in cache only not refcounted values */
						// 直接使用缓存值
						ZVAL_COPY_VALUE(arg, cache_val);
					} else {
						// 在临时zval中更新常量，保证 CONSTANT_AST 值在追踪时不可访问。
						/* Update constant inside a temporary zval, to make sure the CONSTANT_AST
						 * value is not accessible through back traces. */
						// 
						zval tmp;
						// 默认值复制给临时变量
						ZVAL_COPY(&tmp, default_value);
						// 更换执行数据来执行操作码, p1:执行数据，p2:操作码
						zend_execute_data *old = start_fake_frame(call, opline);
						// 更新临时变量的值
						zend_result ret = zval_update_constant_ex(&tmp, fbc->op_array.scope);
						// 结束临时作用域，恢复原有执行数据。p1:调用执行数据，p2:旧的前一个执行数据
						end_fake_frame(call, old);
						// 如果结果是失败
						if (UNEXPECTED(ret == FAILURE)) {
							// 销毁没有引用次数的对象,并没有销毁 zval本身。nogc的意思是，不放入gc回收周期。
							zval_ptr_dtor_nogc(&tmp);
							// 返回失败
							return FAILURE;
						}
						// 临时变量复制给 参数
						ZVAL_COPY_VALUE(arg, &tmp);
						// 如果临时变量不可计数
						if (!Z_REFCOUNTED(tmp)) {
							// 值复制给缓存值
							ZVAL_COPY_VALUE(cache_val, &tmp);
						}
					}
				// 不是表达式常量
				} else {
					// 直接把值复制给参数
					ZVAL_COPY(arg, default_value);
				}
			} else {
				ZEND_ASSERT(opline->opcode == ZEND_RECV);
				// 更换执行数据来执行操作码, p1:执行数据，p2:操作码
				zend_execute_data *old = start_fake_frame(call, opline);
				// 报错：参数数量错误：未传递
				zend_argument_error(zend_ce_argument_count_error, i + 1, "not passed");
				// 结束临时作用域，恢复原有执行数据。p1:调用执行数据，p2:旧的前一个执行数据
				end_fake_frame(call, old);
				// 返回失败
				return FAILURE;
			}
		}

		return SUCCESS;
	// 如果是内置函数 
	} else {
		// 如果函数 有 ZEND_ACC_USER_ARG_INFO  标记
		// Closure::__invoke() 使用 的方法 flag，只能是 int 型
		if (fbc->common.fn_flags & ZEND_ACC_USER_ARG_INFO) {
			// 魔术方法，让它自己处理
			/* Magic function, let it deal with it. */
			return SUCCESS;
		}

		// 参数数量 
		uint32_t num_args = ZEND_CALL_NUM_ARGS(call);
		// 遍历所有参数
		for (uint32_t i = 0; i < num_args; i++) {
			// 取出参数
			zval *arg = ZEND_CALL_VAR_NUM(call, i);
			// 如果不是未定义
			if (!Z_ISUNDEF_P(arg)) {
				// 下一个
				continue;
			}

			// 取得参数信息
			zend_internal_arg_info *arg_info = &fbc->internal_function.arg_info[i];
			// 如果是必须参数
			if (i < fbc->common.required_num_args) {
				// 更换执行数据来执行操作码, p1:执行数据，p2:操作码
				zend_execute_data *old = start_fake_frame(call, NULL);
				// 报错：参数数量错误
				zend_argument_error(zend_ce_argument_count_error, i + 1, "not passed");
				// 结束临时作用域，恢复原有执行数据。p1:调用执行数据，p2:旧的前一个执行数据
				end_fake_frame(call, old);
				// 返回失败
				return FAILURE;
			}

			zval default_value;
			// 解析参数的默认值。p1:默认值，p2:参数信息
			if (zend_get_default_from_internal_arg_info(&default_value, arg_info) == FAILURE) {
				// 更换执行数据来执行操作码, p1:执行数据，p2:操作码
				zend_execute_data *old = start_fake_frame(call, NULL);
				// 报错：参数数量错误，必须显示传递，因为默认值未知
				zend_argument_error(zend_ce_argument_count_error, i + 1,
					"must be passed explicitly, because the default value is not known");
				// 结束临时作用域，恢复原有执行数据。p1:调用执行数据，p2:旧的前一个执行数据
				end_fake_frame(call, old);
				// 返回失败
				return FAILURE;
			}

			// 如果默认值是常量表达式
			if (Z_TYPE(default_value) == IS_CONSTANT_AST) {
				// 更换执行数据来执行操作码, p1:执行数据，p2:操作码
				zend_execute_data *old = start_fake_frame(call, NULL);
				// 更新默认值
				zend_result ret = zval_update_constant_ex(&default_value, fbc->common.scope);
				// 结束临时作用域，恢复原有执行数据。p1:调用执行数据，p2:旧的前一个执行数据
				end_fake_frame(call, old);
				// 如果更新结果是失败
				if (ret == FAILURE) {
					// 返回失败
					return FAILURE;
				}
			}
			
			// 默认值赋给参数
			ZVAL_COPY_VALUE(arg, &default_value);
			// 如果传递方式是，引用传递
			if (ZEND_ARG_SEND_MODE(arg_info) & ZEND_SEND_BY_REF) {
				// 把参数更新成引用类型
				ZVAL_NEW_REF(arg, arg);
			}
		}
	}
	// 返回成功
	return SUCCESS;
}

// ing4, 释放 扩展命名参数（哈希表）
ZEND_API void ZEND_FASTCALL zend_free_extra_named_params(zend_array *extra_named_params)
{
	// 扩展的命名参数可能被共享
	/* Extra named params may be shared. */
	zend_array_release(extra_named_params);
}

// 这些都是 __GNUC__ 用的，windows里没有
#if defined(ZEND_VM_IP_GLOBAL_REG) && ((ZEND_VM_KIND == ZEND_VM_KIND_CALL) || (ZEND_VM_KIND == ZEND_VM_KIND_HYBRID))
/* Special versions of functions that sets EX(opline) before calling zend_vm_stack_extend() */
static zend_always_inline zend_execute_data *_zend_vm_stack_push_call_frame_ex(uint32_t used_stack, uint32_t call_info, zend_function *func, uint32_t num_args, void *object_or_called_scope) /* {{{ */
{
	zend_execute_data *call = (zend_execute_data*)EG(vm_stack_top);

	ZEND_ASSERT_VM_STACK_GLOBAL;

	if (UNEXPECTED(used_stack > (size_t)(((char*)EG(vm_stack_end)) - (char*)call))) {
		EX(opline) = opline; /* this is the only difference */
		call = (zend_execute_data*)zend_vm_stack_extend(used_stack);
		ZEND_ASSERT_VM_STACK_GLOBAL;
		// 虚拟机方法，初始化调用框架。为把被调用函数的相关信息附加到 执行数据对象上
		zend_vm_init_call_frame(call, call_info | ZEND_CALL_ALLOCATED, func, num_args, object_or_called_scope);
		return call;
	} else {
		EG(vm_stack_top) = (zval*)((char*)call + used_stack);
		// 虚拟机方法，初始化调用框架。为把被调用函数的相关信息附加到 执行数据对象上
		zend_vm_init_call_frame(call, call_info, func, num_args, object_or_called_scope);
		return call;
	}
} /* }}} */

static zend_always_inline zend_execute_data *_zend_vm_stack_push_call_frame(uint32_t call_info, zend_function *func, uint32_t num_args, void *object_or_called_scope) /* {{{ */
{
	uint32_t used_stack = zend_vm_calc_used_stack(num_args, func);

	return _zend_vm_stack_push_call_frame_ex(used_stack, call_info,
		func, num_args, object_or_called_scope);
} /* }}} */

// windows走这里。 这两个方法在  头文件里定义
#else
# define _zend_vm_stack_push_call_frame_ex zend_vm_stack_push_call_frame_ex
# define _zend_vm_stack_push_call_frame    zend_vm_stack_push_call_frame
#endif

#ifdef ZEND_VM_TRACE_HANDLERS
# include "zend_vm_trace_handlers.h"
#elif defined(ZEND_VM_TRACE_LINES)
# include "zend_vm_trace_lines.h"
#elif defined(ZEND_VM_TRACE_MAP)
# include "zend_vm_trace_map.h"
#endif

// ing3, 偏移到目标操作码并 return ,p1:是否检查异常？使用 EX(opline)  ：使用 opline 。p2:偏移量
#define ZEND_VM_NEXT_OPCODE_EX(check_exception, skip) \
	/* 调试用 */ \
	CHECK_SYMBOL_TABLES() \
	/* 如果检查异常 */ \
	if (check_exception) { \
		/* #define EX(element) 			((execute_data)->element) */ \
		/* 在执行数据中，找到目标操作码 */ \
		OPLINE = EX(opline) + (skip); \
	/* 如果不检查异常 */ \
	} else { \
		/* 不可以有异常 */ \
		ZEND_ASSERT(!EG(exception)); \
		/* 通过偏移，找到目标操作码 */ \
		OPLINE = opline + (skip); \
	} \
	/* windows: return  0 */ \
	ZEND_VM_CONTINUE()

// ing4, 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
#define ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION() \
	/* 找到目标操作码并 return ,p1:是否检查异常。p2 ？在 EX(opline) 里偏移 ：opline直接偏移 */ \
	ZEND_VM_NEXT_OPCODE_EX(1, 1)

// ing4, opline向右偏移1个位置，到目标操作码并 return
#define ZEND_VM_NEXT_OPCODE() \
	/* 找到目标操作码并 return ,p1:是否检查异常。p2 ？在 EX(opline) 里偏移 ：opline直接偏移 */ \
	ZEND_VM_NEXT_OPCODE_EX(0, 1)

// ing4, 设置下一个操作码 OPLINE=p1
#define ZEND_VM_SET_NEXT_OPCODE(new_op) \
	/* 调试用 */ \
	CHECK_SYMBOL_TABLES() \
	OPLINE = new_op
	
// ing3, 修改当前操作码并返回。EX(opline) = p1 
#define ZEND_VM_SET_OPCODE(new_op) \
	/* 调试用 */ \
	CHECK_SYMBOL_TABLES() \
	/* EX(opline) = new_op */ \
	OPLINE = new_op; \
	/* 打断并返回 */ \
	ZEND_VM_INTERRUPT_CHECK()

// ing3, 找到目标操作码，设置成当前操作码并返回。 EX(opline) = (zend_op*)((char*)p1 + p2)
#define ZEND_VM_SET_RELATIVE_OPCODE(opline, offset) \
	/* 修改当前操作码并返回。EX(opline) = p1 */ \
	/* (zend_op*)((char*)p1 + p2) */ \
	ZEND_VM_SET_OPCODE(ZEND_OFFSET_TO_OPLINE(opline, offset))

// ing3, 要检查异常 并 有异常时使用旧操作码，无异常使用新操作码, p1:新操作码，p2:是否检查异常
#define ZEND_VM_JMP_EX(new_op, check_exception) do { \
		/* 如果要检查异常并且有异常 */ \
		if (check_exception && UNEXPECTED(EG(exception))) { \
			/* windows: opline = EX(opline); return 0; */ \
			HANDLE_EXCEPTION(); \
		} \
		/* 修改当前操作码并返回。EX(opline) = p1  */ \
		ZEND_VM_SET_OPCODE(new_op); \
		/* windows: return  0 */ \
		ZEND_VM_CONTINUE(); \
	} while (0)

// ing3, 有异常时使用旧操作码，无异常使用新操作码, p1:新操作码
#define ZEND_VM_JMP(new_op) \
	ZEND_VM_JMP_EX(new_op, 1)

// ing3, 切换到下一行操作码
#define ZEND_VM_INC_OPCODE() \
	OPLINE++


// ing3, do {
#define ZEND_VM_REPEATABLE_OPCODE \
	do {

// ing3, while() 循环结尾段，找下一个操作码，直到碰到p1操作码
#define ZEND_VM_REPEAT_OPCODE(_opcode) \
	} while (UNEXPECTED((++opline)->opcode == _opcode)); \
	/* EX(opline) = opline */\
	OPLINE = opline; \
	/* windows: return  0 */ \
	ZEND_VM_CONTINUE()

// ing3, 根据当前操作码结果选择下一个操作码，p1:传入值，主要用来做判断。p2:是否检查异常
#define ZEND_VM_SMART_BRANCH(_result, _check) do { \
		/* 如果要检查异常并且有异常 */ \
		if ((_check) && UNEXPECTED(EG(exception))) { \
			/* 使用旧操作码 */ \
			OPLINE = EX(opline); \
		/* 操作码结果类型为 (IS_SMART_BRANCH_JMPZ|IS_TMP_VAR) */ \
		} else if (EXPECTED(opline->result_type == (IS_SMART_BRANCH_JMPZ|IS_TMP_VAR))) { \
			/* 有传入值 */ \
			if (_result) { \
				/* 当前操作码后移2个 */ \
				ZEND_VM_SET_NEXT_OPCODE(opline + 2); \
			/* 没有传入值 */ \
			} else { \
				/* 修改当前操作码并返回。EX(opline) = p1  */ \
				/* 后移一段并返回操作码，(zend_op*)((char*)p1 + p2.jmp_offset) */ \
				ZEND_VM_SET_OPCODE(OP_JMP_ADDR(opline + 1, (opline+1)->op2)); \
			} \
		/* 操作码结果类型为 (IS_SMART_BRANCH_JMPNZ|IS_TMP_VAR) */ \
		} else if (EXPECTED(opline->result_type == (IS_SMART_BRANCH_JMPNZ|IS_TMP_VAR))) { \
			/* 没有传入值 */ \
			if (!(_result)) { \
				/* 当前操作码后移2个 */ \
				ZEND_VM_SET_NEXT_OPCODE(opline + 2); \
			/* 有传入值 */ \
			} else { \
				/* 修改当前操作码并返回。EX(opline) = p1  */ \
				/* 后移一段并返回操作码，(zend_op*)((char*)p1 + p2.jmp_offset) */ \
				ZEND_VM_SET_OPCODE(OP_JMP_ADDR(opline + 1, (opline+1)->op2)); \
			} \
		/*  */ \
		} else { \
			/* 给操作码结果 赋值 布尔型结果 */ \
			ZVAL_BOOL(EX_VAR(opline->result.var), _result); \
			/* 当前操作码后移1个 */ \
			ZEND_VM_SET_NEXT_OPCODE(opline + 1); \
		} \
		/* windows: return  0 */ \
		ZEND_VM_CONTINUE(); \
	} while (0)

// ing3, 根据当前操作码结果选择下一个操作码，有_result时后移2个。p1:传入值，主要用来做判断。p2:是否检查异常
#define ZEND_VM_SMART_BRANCH_JMPZ(_result, _check) do { \
		/* 如果要检查异常并且有异常 */ \
		if ((_check) && UNEXPECTED(EG(exception))) { \
			/* EX(opline)=EX(opline) */ \
			OPLINE = EX(opline); \
		/* 如果有传入值 */ \
		} else if (_result) { \
			/* 当前操作码后移2个 */ \
			ZEND_VM_SET_NEXT_OPCODE(opline + 2); \
		/* 其他情况 */ \
		} else { \
			/* 修改当前操作码并返回。EX(opline) = p1  */ \
			/* 后移一段并返回操作码，(zend_op*)((char*)p1 + p2.jmp_offset) */ \
			ZEND_VM_SET_OPCODE(OP_JMP_ADDR(opline + 1, (opline+1)->op2)); \
		} \
		/* windows: return  0 */ \
		ZEND_VM_CONTINUE(); \
	} while (0)

// ing3, 根据当前操作码结果选择下一个操作码，无_result时后移2个。p1:传入值，主要用来做判断。p2:是否检查异常
#define ZEND_VM_SMART_BRANCH_JMPNZ(_result, _check) do { \
		/* 如果要检查异常并且有异常 */ \
		if ((_check) && UNEXPECTED(EG(exception))) { \
			/* EX(opline)=EX(opline) */ \
			OPLINE = EX(opline); \
		/* 如果有传入值 */ \
		} else if (!(_result)) { \
			/* 当前操作码后移2个 */ \
			ZEND_VM_SET_NEXT_OPCODE(opline + 2); \
		/* 其他情况 */ \
		} else { \
			/* 修改当前操作码并返回。EX(opline) = p1  */ \
			/* 后移一段并返回操作码，(zend_op*)((char*)p1 + p2.jmp_offset) */ \
			ZEND_VM_SET_OPCODE(OP_JMP_ADDR(opline + 1, (opline+1)->op2)); \
		} \
		/* windows: return  0 */ \
		ZEND_VM_CONTINUE(); \
	} while (0)

// ing3, 赋值为布尔型，并选择下一个操作码，在 EX(opline) 里偏移。p1:传入值。p2:是否检查异常
#define ZEND_VM_SMART_BRANCH_NONE(_result, _check) do { \
		/* 给操作码结果 赋值 布尔型结果 */ \
		ZVAL_BOOL(EX_VAR(opline->result.var), _result); \
		/* 找到目标操作码并 return ,p1:是否检查异常。p2 ？在 EX(opline) 里偏移 ：opline直接偏移 */ \
		ZEND_VM_NEXT_OPCODE_EX(_check, 1); \
		/* windows: return  0 */ \
		ZEND_VM_CONTINUE(); \
	} while (0)
		
// ing3, 根据当前操作码结果选择下一个操作码。如果不是跳转类型，结果赋值为true。
#define ZEND_VM_SMART_BRANCH_TRUE() do { \
		/* 操作码结果类型为 (IS_SMART_BRANCH_JMPNZ|IS_TMP_VAR) */ \
		if (EXPECTED(opline->result_type == (IS_SMART_BRANCH_JMPNZ|IS_TMP_VAR))) { \
			/* 修改当前操作码并返回。EX(opline) = p1  */ \
			/* 后移一段并返回操作码，(zend_op*)((char*)p1 + p2.jmp_offset) */ \
			ZEND_VM_SET_OPCODE(OP_JMP_ADDR(opline + 1, (opline+1)->op2)); \
		/* 操作码结果类型为 (IS_SMART_BRANCH_JMPZ|IS_TMP_VAR) */ \
		} else if (EXPECTED(opline->result_type == (IS_SMART_BRANCH_JMPZ|IS_TMP_VAR))) { \
			/* 当前操作码后移2个 */ \
			ZEND_VM_SET_NEXT_OPCODE(opline + 2); \
		/* 其他情况 */ \
		} else { \
			/* 给操作码结果 赋值 true */ \
			ZVAL_TRUE(EX_VAR(opline->result.var)); \
			/* 当前操作码后移1个 */ \
			ZEND_VM_SET_NEXT_OPCODE(opline + 1); \
		} \
		/* windows: return  0 */ \
		ZEND_VM_CONTINUE(); \
	} while (0)
		
// ing3, 当前操作码后移2个, 并return 0;
#define ZEND_VM_SMART_BRANCH_TRUE_JMPZ() do { \
		/* 当前操作码后移2个 */ \
		ZEND_VM_SET_NEXT_OPCODE(opline + 2); \
		/* windows: return  0 */ \
		ZEND_VM_CONTINUE(); \
	} while (0)

// ing3, 跳转到目标操作码，并return 0;
#define ZEND_VM_SMART_BRANCH_TRUE_JMPNZ() do { \
		/* 修改当前操作码并返回。EX(opline) = p1  */ \
		/* 后移一段并返回操作码，(zend_op*)((char*)p1 + p2.jmp_offset) */ \
		ZEND_VM_SET_OPCODE(OP_JMP_ADDR(opline + 1, (opline+1)->op2)); \
		/* windows: return  0 */ \
		ZEND_VM_CONTINUE(); \
	} while (0)

// ing3, 操作码结果赋值为布true，并选择下一个操作码
#define ZEND_VM_SMART_BRANCH_TRUE_NONE() do { \
		/* 给操作码结果 赋值 true */ \
		ZVAL_TRUE(EX_VAR(opline->result.var)); \
		/* opline向右偏移1个位置，到目标操作码并 return */ \
		ZEND_VM_NEXT_OPCODE(); \
	} while (0)

// ing3, 根据当前操作码结果选择下一个操作码。如果不是跳转类型，结果赋值为false。
#define ZEND_VM_SMART_BRANCH_FALSE() do { \
		/* 操作码结果类型为 (IS_SMART_BRANCH_JMPNZ|IS_TMP_VAR) */ \
		if (EXPECTED(opline->result_type == (IS_SMART_BRANCH_JMPNZ|IS_TMP_VAR))) { \
			/* 当前操作码后移2个 */ \
			ZEND_VM_SET_NEXT_OPCODE(opline + 2); \
		/* 操作码结果类型为 (IS_SMART_BRANCH_JMPZ|IS_TMP_VAR) */ \
		} else if (EXPECTED(opline->result_type == (IS_SMART_BRANCH_JMPZ|IS_TMP_VAR))) { \
			/* 修改当前操作码并返回。EX(opline) = p1  */ \
			/* 后移一段并返回操作码，(zend_op*)((char*)p1 + p2.jmp_offset) */ \
			ZEND_VM_SET_OPCODE(OP_JMP_ADDR(opline + 1, (opline+1)->op2)); \
		/* 其他情况 */ \
		} else { \
			/* 给操作码结果 赋值 false */ \
			ZVAL_FALSE(EX_VAR(opline->result.var)); \
			/* 当前操作码后移1个 */ \
			ZEND_VM_SET_NEXT_OPCODE(opline + 1); \
		} \
		/* windows: return  0 */ \
		ZEND_VM_CONTINUE(); \
	} while (0)

// ing3, 跳转到目标操作码，并return 0;		
#define ZEND_VM_SMART_BRANCH_FALSE_JMPZ() do { \
		/* 修改当前操作码并返回。EX(opline) = p1  */ \
		/* 后移一段并返回操作码，(zend_op*)((char*)p1 + p2.jmp_offset) */ \
		ZEND_VM_SET_OPCODE(OP_JMP_ADDR(opline + 1, (opline+1)->op2)); \
		/* windows: return  0 */ \
		ZEND_VM_CONTINUE(); \
	} while (0)

// ing3, 当前操作码后移2个, 并return 0;		
#define ZEND_VM_SMART_BRANCH_FALSE_JMPNZ() do { \
		/* 当前操作码后移2个 */ \
		ZEND_VM_SET_NEXT_OPCODE(opline + 2); \
		/* windows: return  0 */ \
		ZEND_VM_CONTINUE(); \
	} while (0)

// ing3, 操作码结果赋值为布false，并选择下一个操作码		
#define ZEND_VM_SMART_BRANCH_FALSE_NONE() do { \
		/* 修改当前操作码并返回。EX(opline) = p1  */ \
		/* 后移一段并返回操作码，(zend_op*)((char*)p1 + p2.jmp_offset) */ \
		ZVAL_FALSE(EX_VAR(opline->result.var)); \
		/* opline向右偏移1个位置，到目标操作码并 return */ \
		ZEND_VM_NEXT_OPCODE(); \
	} while (0)

// windows 里没有这个
#ifdef __GNUC__
// 调用汇编？
# define ZEND_VM_GUARD(name) __asm__("#" #name)
#else
// clear, windows 里没什么用
# define ZEND_VM_GUARD(name)
#endif

// ing4, 如果 opline 结果类型是 变量或临时变量，把结果 zval 标记为未定义
// #define EX_VAR(n) ZEND_CALL_VAR(execute_data, n)
// #define ZEND_CALL_VAR(call, n) ((zval*)(((char*)(call)) + ((int)(n))))
#define UNDEF_RESULT() do { \
		if (opline->result_type & (IS_VAR | IS_TMP_VAR)) { \
			ZVAL_UNDEF(EX_VAR(opline->result.var)); \
		} \
	} while (0)

/* This callback disables optimization of "vm_stack_data" variable in VM */
ZEND_API void (ZEND_FASTCALL *zend_touch_vm_stack_data)(void *vm_stack_data) = NULL;

// 唯一引用的地方
#include "zend_vm_execute.h"

// ing3, 添加用户定义的操作码和对应的处理器
ZEND_API zend_result zend_set_user_opcode_handler(zend_uchar opcode, user_opcode_handler_t handler)
{
	// 如果 不是 ZEND_USER_OPCODE
	if (opcode != ZEND_USER_OPCODE) {
		// 如果没有处理器
		if (handler == NULL) {
			/* restore the original handler */
			// 先把操作码保存到列表里
			zend_user_opcodes[opcode] = opcode;
		// 如果有处理器。有处理器的操作码索引号都是 ZEND_USER_OPCODE
		} else {
			// 操作码索引号还使用 ZEND_USER_OPCODE
			zend_user_opcodes[opcode] = ZEND_USER_OPCODE;
		}
		// 添加处理器
		zend_user_opcode_handlers[opcode] = handler;
		// 返回成功
		return SUCCESS;
	}
	// 如果操作码是 ZEND_USER_OPCODE
	// 返回失败
	return FAILURE;
}

// ing4, 获取用户操作码处理器
ZEND_API user_opcode_handler_t zend_get_user_opcode_handler(zend_uchar opcode)
{
	return zend_user_opcode_handlers[opcode];
}

// ing4, 获取 zval 指针，支持 常量、变量、临时变量、编译变量
ZEND_API zval *zend_get_zval_ptr(const zend_op *opline, int op_type, const znode_op *node, const zend_execute_data *execute_data)
{
	zval *ret;

	// 根据类型进行操作
	switch (op_type) {
		// 如果类型是常量
		case IS_CONST:
			/* run-time constant */
			// 在p1中通过偏移量p2.constant 获取 zval 指针
			ret = RT_CONSTANT(opline, *node);
			break;
		// 变量，临时变量，编译变量，在这里用相同的处理方式
		case IS_TMP_VAR:
		case IS_VAR:
		case IS_CV:
			// #define EX_VAR(n) ZEND_CALL_VAR(execute_data, n)
			// #define ZEND_CALL_VAR(call, n) ((zval*)(((char*)(call)) + ((int)(n))))
			ret = EX_VAR(node->var);
			break;
		// 不支持其他类型
		default:
			ret = NULL;
			break;
	}
	return ret;
}
