// 默认是 ZEND_VM_KIND_HYBRID 模式
// 执行 _zend_execute_data ,
// 8057行代码，扣掉 labels 3450 行， HYBRID_SWITCH 4532 行, 
// 这里是完整版，下面分 ZEND_VM_KIND_HYBRID 和其他模式两种情况分析
ZEND_API void execute_ex(zend_execute_data *ex)
{
	DCL_OPLINE

#if defined(ZEND_VM_IP_GLOBAL_REG) || defined(ZEND_VM_FP_GLOBAL_REG)
	struct {
#ifdef ZEND_VM_IP_GLOBAL_REG
		const zend_op *orig_opline;
#endif
#ifdef ZEND_VM_FP_GLOBAL_REG
		zend_execute_data *orig_execute_data;
#ifdef ZEND_VM_HYBRID_JIT_RED_ZONE_SIZE
		char hybrid_jit_red_zone[ZEND_VM_HYBRID_JIT_RED_ZONE_SIZE];
#endif
#endif
	} vm_stack_data;
#endif
#ifdef ZEND_VM_IP_GLOBAL_REG
	vm_stack_data.orig_opline = opline;
#endif
#ifdef ZEND_VM_FP_GLOBAL_REG
	vm_stack_data.orig_execute_data = execute_data;
	execute_data = ex;
#else
	zend_execute_data *execute_data = ex;
#endif

#if (ZEND_VM_KIND == ZEND_VM_KIND_HYBRID)
	// 
	if (UNEXPECTED(execute_data == NULL)) {
		// 共3451 个
		static const void * const labels[] = {
		};
		zend_opcode_handlers = (const void **) labels;
		zend_handlers_count = sizeof(labels) / sizeof(void*);
		memset(&hybrid_halt_op, 0, sizeof(hybrid_halt_op));
		hybrid_halt_op.handler = (void*)&&HYBRID_HALT_LABEL;
#ifdef ZEND_VM_HYBRID_JIT_RED_ZONE_SIZE
		memset(vm_stack_data.hybrid_jit_red_zone, 0, ZEND_VM_HYBRID_JIT_RED_ZONE_SIZE);
#endif
		if (zend_touch_vm_stack_data) {
			zend_touch_vm_stack_data(&vm_stack_data);
		}
		goto HYBRID_HALT_LABEL;
	}
#endif

	LOAD_OPLINE();
	ZEND_VM_LOOP_INTERRUPT_CHECK();

	while (1) {
#if !defined(ZEND_VM_FP_GLOBAL_REG) || !defined(ZEND_VM_IP_GLOBAL_REG)
			int ret;
#endif
#if (ZEND_VM_KIND == ZEND_VM_KIND_HYBRID)
		// 这块是核心业务逻辑
		// HYBRID_SWITCH = #define HYBRID_NEXT() goto *(void**)(OPLINE->handler)
		HYBRID_SWITCH() {
#else
#if defined(ZEND_VM_FP_GLOBAL_REG) && defined(ZEND_VM_IP_GLOBAL_REG)
		((opcode_handler_t)OPLINE->handler)(ZEND_OPCODE_HANDLER_ARGS_PASSTHRU);
		if (UNEXPECTED(!OPLINE)) {
#else
		if (UNEXPECTED((ret = ((opcode_handler_t)OPLINE->handler)(ZEND_OPCODE_HANDLER_ARGS_PASSTHRU)) != 0)) {
#endif
#endif
#if (ZEND_VM_KIND == ZEND_VM_KIND_HYBRID)
			// #define HYBRID_CASE(op)   op ## _LABEL
			// HYBRID_CASE 是获取每个 label 对应的数字 ，/Zend/zend_vm_handlers.h 中定义
			// 比如 ZEND_ASSIGN_STATIC_PROP_OP_SPEC 对应 1066
			HYBRID_CASE(ZEND_ASSIGN_STATIC_PROP_OP_SPEC):
				// define VM_TRACE(op) 。 这个宏没有业务逻辑
				VM_TRACE(ZEND_ASSIGN_STATIC_PROP_OP_SPEC)
				// 这里是各种 static 方法，都是在本文件里定义的, 约有1000个 ，这是这个文件的主要业务逻辑
				ZEND_ASSIGN_STATIC_PROP_OP_SPEC_HANDLER(ZEND_OPCODE_HANDLER_ARGS_PASSTHRU);
				// HYBRID_BREAK = #define HYBRID_NEXT() goto *(void**)(OPLINE->handler)
				HYBRID_BREAK();
			
			HYBRID_DEFAULT:
				VM_TRACE(ZEND_NULL)
				ZEND_NULL_HANDLER(ZEND_OPCODE_HANDLER_ARGS_PASSTHRU);
				HYBRID_BREAK(); /* Never reached */
#else
#ifdef ZEND_VM_FP_GLOBAL_REG
			execute_data = vm_stack_data.orig_execute_data;
# ifdef ZEND_VM_IP_GLOBAL_REG
			opline = vm_stack_data.orig_opline;
# endif
			return;
#else
			if (EXPECTED(ret > 0)) {
				execute_data = EG(current_execute_data);
				ZEND_VM_LOOP_INTERRUPT_CHECK();
			} else {
# ifdef ZEND_VM_IP_GLOBAL_REG
				opline = vm_stack_data.orig_opline;
# endif
				return;
			}
#endif
#endif
		}

	}
	zend_error_noreturn(E_CORE_ERROR, "Arrived at end of main loop which shouldn't happen");
}
#if (ZEND_VM_KIND != ZEND_VM_KIND_CALL) && (ZEND_GCC_VERSION >= 4000) && !defined(__clang__)
# pragma GCC pop_options
#endif







// 如果没有 ZEND_VM_KIND_HYBRID， 这里逻辑要简单很多，连 zend_opcode_handlers 都不需要了
ZEND_API void execute_ex(zend_execute_data *ex)
{
	// 空的 #define DCL_OPLINE 	
	DCL_OPLINE

// 如果有 ZEND_VM_IP_GLOBAL_REG 或 ZEND_VM_FP_GLOBAL_REG
#if defined(ZEND_VM_IP_GLOBAL_REG) || defined(ZEND_VM_FP_GLOBAL_REG)
	struct {
#ifdef ZEND_VM_IP_GLOBAL_REG
		const zend_op *orig_opline;
#endif
#ifdef ZEND_VM_FP_GLOBAL_REG
		zend_execute_data *orig_execute_data;
#ifdef ZEND_VM_HYBRID_JIT_RED_ZONE_SIZE
		char hybrid_jit_red_zone[ZEND_VM_HYBRID_JIT_RED_ZONE_SIZE];
#endif
#endif
	} vm_stack_data;
#endif
#ifdef ZEND_VM_IP_GLOBAL_REG
	vm_stack_data.orig_opline = opline;
#endif
#ifdef ZEND_VM_FP_GLOBAL_REG
	vm_stack_data.orig_execute_data = execute_data;
	execute_data = ex;
#else
	zend_execute_data *execute_data = ex;
#endif
// END: 如果有 ZEND_VM_IP_GLOBAL_REG 或 ZEND_VM_FP_GLOBAL_REG

	// # define LOAD_OPLINE() opline = EX(opline) 或 # define LOAD_OPLINE()
	LOAD_OPLINE();
	// 
	ZEND_VM_LOOP_INTERRUPT_CHECK();

	// 核心逻辑是 调用 opline->handler( execute_data )
	// #define EX(element) 			((execute_data)->element)
	// 所以最终执行的逻辑是 execute_data->opline->handler( execute_data )
	while (1) {
#if !defined(ZEND_VM_FP_GLOBAL_REG) || !defined(ZEND_VM_IP_GLOBAL_REG)
			int ret;
#endif

// define OPLINE opline
// 这一行代码是核心业务逻辑，可以代替 ZEND_VM_KIND_HYBRID 模式的几千行代码
// 这两种情况唯一的区别是，一个用 ret 接收结果 ，一个不用
#if defined(ZEND_VM_FP_GLOBAL_REG) && defined(ZEND_VM_IP_GLOBAL_REG)
		// # define ZEND_OPCODE_HANDLER_ARGS_PASSTHRU execute_data
		// handler 在编译时没有赋值，什么时候加进去的呢
		// typedef int (*opcode_handler_t)(void);
		((opcode_handler_t)OPLINE->handler)(ZEND_OPCODE_HANDLER_ARGS_PASSTHRU);
		// 如果没有 OPLINE？怎么先执行后判断呢，handler里会自动切换到下一个操作码吗
		if (UNEXPECTED(!OPLINE)) {
// 这种情况用 ret接收结果 
#else
		// 如果执行函数的返回结果不是 0（包含大于0，小于0两种情况） ，准备返回
		if (UNEXPECTED((ret = ((opcode_handler_t)OPLINE->handler)(ZEND_OPCODE_HANDLER_ARGS_PASSTHRU)) != 0)) {
#endif

// 如果有 ZEND_VM_FP_GLOBAL_REG 在这里返回，
// 两种情况里如果有 ZEND_VM_IP_GLOBAL_REG ,多一行代码 opline = vm_stack_data.orig_opline;
#ifdef ZEND_VM_FP_GLOBAL_REG
			// 
			execute_data = vm_stack_data.orig_execute_data;
# ifdef ZEND_VM_IP_GLOBAL_REG
			opline = vm_stack_data.orig_opline;
# endif
			return;
// 如果没有 ZEND_VM_FP_GLOBAL_REG 在这里返回
#else
			// 如果结果大于0
			if (EXPECTED(ret > 0)) {
				// 
				execute_data = EG(current_execute_data);
				ZEND_VM_LOOP_INTERRUPT_CHECK();
			// 如果结果小于0 
			} else {
# ifdef ZEND_VM_IP_GLOBAL_REG
				opline = vm_stack_data.orig_opline;
# endif
				return;
			}
#endif
		}

	}
	zend_error_noreturn(E_CORE_ERROR, "Arrived at end of main loop which shouldn't happen");
}
#if (ZEND_VM_KIND != ZEND_VM_KIND_CALL) && (ZEND_GCC_VERSION >= 4000) && !defined(__clang__)
# pragma GCC pop_options
#endif





// 实际情况：如果是 ZEND_VM_KIND_HYBRID 模式
ZEND_API void execute_ex(zend_execute_data *ex)
{
	DCL_OPLINE

#if defined(ZEND_VM_IP_GLOBAL_REG) || defined(ZEND_VM_FP_GLOBAL_REG)
	struct {
#ifdef ZEND_VM_IP_GLOBAL_REG
		const zend_op *orig_opline;
#endif
#ifdef ZEND_VM_FP_GLOBAL_REG
		zend_execute_data *orig_execute_data;
#ifdef ZEND_VM_HYBRID_JIT_RED_ZONE_SIZE
		char hybrid_jit_red_zone[ZEND_VM_HYBRID_JIT_RED_ZONE_SIZE];
#endif
#endif
	} vm_stack_data;
#endif
#ifdef ZEND_VM_IP_GLOBAL_REG
	vm_stack_data.orig_opline = opline;
#endif
#ifdef ZEND_VM_FP_GLOBAL_REG
	vm_stack_data.orig_execute_data = execute_data;
	execute_data = ex;
#else
	zend_execute_data *execute_data = ex;
#endif

	// 没有 execute_data 才会走这里
	if (UNEXPECTED(execute_data == NULL)) {
		// 共3451 个
		static const void * const labels[] = {
		};
		zend_opcode_handlers = (const void **) labels;
		zend_handlers_count = sizeof(labels) / sizeof(void*);
		memset(&hybrid_halt_op, 0, sizeof(hybrid_halt_op));
		hybrid_halt_op.handler = (void*)&&HYBRID_HALT_LABEL;
#ifdef ZEND_VM_HYBRID_JIT_RED_ZONE_SIZE
		memset(vm_stack_data.hybrid_jit_red_zone, 0, ZEND_VM_HYBRID_JIT_RED_ZONE_SIZE);
#endif
		if (zend_touch_vm_stack_data) {
			zend_touch_vm_stack_data(&vm_stack_data);
		}
		// 
		goto HYBRID_HALT_LABEL;
	}

	// # define LOAD_OPLINE() opline = EX(opline) 或 # define LOAD_OPLINE()
	LOAD_OPLINE();
	ZEND_VM_LOOP_INTERRUPT_CHECK();

	while (1) {
#if !defined(ZEND_VM_FP_GLOBAL_REG) || !defined(ZEND_VM_IP_GLOBAL_REG)
			int ret;
#endif
		// 这块是核心业务逻辑， 这个goto是怎么用的呢，用很多个goto实现类似switch的功能，
		// HYBRID_SWITCH 是第一个goto ，HYBRID_BREAK 是运行完后的goto，这样不停goto下去
		// HYBRID_SWITCH = #define HYBRID_NEXT() goto *(void**)(OPLINE->handler)
		// goto *(void**)(OPLINE->handler) (){
		HYBRID_SWITCH() {

			// #define HYBRID_CASE(op)   op ## _LABEL
			// HYBRID_CASE 是获取每个 label 对应的数字 ，/Zend/zend_vm_handlers.h 中定义
			// 比如 ZEND_ASSIGN_STATIC_PROP_OP_SPEC 对应 1066
			// 1066:
			HYBRID_CASE(ZEND_ASSIGN_STATIC_PROP_OP_SPEC):
				// define VM_TRACE(op) 。 这个宏没有业务逻辑
				VM_TRACE(ZEND_ASSIGN_STATIC_PROP_OP_SPEC)
				// 这里是各种 static 方法，都是在本文件里定义的, 约有1000个 ，这是这个文件的主要业务逻辑
				// 每个 handler都负责执行完成后跳切换到下一个操作码，这个地方分几种情况处理
				ZEND_ASSIGN_STATIC_PROP_OP_SPEC_HANDLER(ZEND_OPCODE_HANDLER_ARGS_PASSTHRU);
				// HYBRID_BREAK = #define HYBRID_NEXT() goto *(void**)(OPLINE->handler)
				// goto *(void**)(OPLINE->handler)
				HYBRID_BREAK();
			
			HYBRID_DEFAULT:
				VM_TRACE(ZEND_NULL)
				ZEND_NULL_HANDLER(ZEND_OPCODE_HANDLER_ARGS_PASSTHRU);
				HYBRID_BREAK(); /* Never reached */

		}

	}
	zend_error_noreturn(E_CORE_ERROR, "Arrived at end of main loop which shouldn't happen");
}
#if (ZEND_VM_KIND != ZEND_VM_KIND_CALL) && (ZEND_GCC_VERSION >= 4000) && !defined(__clang__)
# pragma GCC pop_options
#endif



# 把宏替换成原始程序 
	while (1) {
		// 这块是核心业务逻辑， 这个goto是怎么用的呢
		// HYBRID_SWITCH = #define HYBRID_NEXT() goto *(void**)(OPLINE->handler)
		goto *(void**)(OPLINE->handler) {

			// #define HYBRID_CASE(op)   op ## _LABEL
			// HYBRID_CASE 是获取每个 label 对应的数字 ，/Zend/zend_vm_handlers.h 中定义
			// 比如 ZEND_ASSIGN_STATIC_PROP_OP_SPEC 对应 1066
			ZEND_ASSIGN_STATIC_PROP_OP_SPEC ## _LABEL:
				// 这里是各种 static 方法，都是在本文件里定义的, 约有1000个 ，这是这个文件的主要业务逻辑
				ZEND_ASSIGN_STATIC_PROP_OP_SPEC_HANDLER(ZEND_OPCODE_HANDLER_ARGS_PASSTHRU);
				// HYBRID_BREAK = #define HYBRID_NEXT() goto *(void**)(OPLINE->handler)
				goto *(void**)(OPLINE->handler) 
			
			HYBRID_DEFAULT:
				VM_TRACE(ZEND_NULL)
				ZEND_NULL_HANDLER(ZEND_OPCODE_HANDLER_ARGS_PASSTHRU);
				HYBRID_BREAK(); /* Never reached */

		}

	}

// 操作码方法示例, 	切换操作码也是在这里面！
// # define ZEND_OPCODE_HANDLER_ARGS zend_execute_data *execute_data
// ZEND_OPCODE_HANDLER_ARGS 就是 zend_execute_data, 业务逻辑里可能不用它，但第一行获取操作码一定要用它
static ZEND_OPCODE_HANDLER_RET ZEND_FASTCALL ZEND_ASSIGN_STATIC_PROP_OP_SPEC_HANDLER(ZEND_OPCODE_HANDLER_ARGS)
{
	/* This helper actually never will receive IS_VAR as second op, and has the same handling for VAR and TMP in the first op, but for interoperability with the other binary_assign_op helpers, it is necessary to "include" it */

	// 进来先引入操作码
	// #define EX(element) 			((execute_data)->element)
	// # define USE_OPLINE const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *prop, *value;
	zend_property_info *prop_info;
	zend_reference *ref;

	// # define SAVE_OPLINE() EX(opline) = opline
	SAVE_OPLINE();

	if (UNEXPECTED(zend_fetch_static_property_address(&prop, &prop_info, (opline+1)->extended_value, BP_VAR_RW, 0 OPLINE_CC EXECUTE_DATA_CC) != SUCCESS)) {
		UNDEF_RESULT();
		FREE_OP((opline+1)->op1_type, (opline+1)->op1.var);
		HANDLE_EXCEPTION();
	}

	value = get_op_data_zval_ptr_r((opline+1)->op1_type, (opline+1)->op1);

	do {
		if (UNEXPECTED(Z_ISREF_P(prop))) {
			ref = Z_REF_P(prop);
			prop = Z_REFVAL_P(prop);
			if (UNEXPECTED(ZEND_REF_HAS_TYPE_SOURCES(ref))) {
				zend_binary_assign_op_typed_ref(ref, value OPLINE_CC EXECUTE_DATA_CC);
				break;
			}
		}

		if (UNEXPECTED(ZEND_TYPE_IS_SET(prop_info->type))) {
			/* special case for typed properties */
			zend_binary_assign_op_typed_prop(prop_info, prop, value OPLINE_CC EXECUTE_DATA_CC);
		} else {
			zend_binary_op(prop, prop, value OPLINE_CC);
		}
	} while (0);

	if (UNEXPECTED(RETURN_VALUE_USED(opline))) {
		ZVAL_COPY(EX_VAR(opline->result.var), prop);
	}

	FREE_OP((opline+1)->op1_type, (opline+1)->op1.var);
	/* assign_static_prop has two opcodes! */
	// 这里是跳到下一个操作码，这下全通了！
	ZEND_VM_NEXT_OPCODE_EX(1, 2);
}