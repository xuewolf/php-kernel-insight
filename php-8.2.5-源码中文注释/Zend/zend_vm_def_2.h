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

/* If you change this file, please regenerate the zend_vm_execute.h and
 * zend_vm_opcodes.h files by running:
 * php zend_vm_gen.php
 */

// 修改此文件后要运行 php zend_vm_gen.php 
// 这个文件没有被任何程序引用，纯纯是为了生成 zend_vm_opcodes.h，这里面不是可运行的c程序，而是一些关键内容的缩写
// 重新生成 the zend_vm_execute.h 和 zend_vm_opcodes.h

// zend_vm 并不如想象中的复杂，本以为里面会有特别多的优化处理，让人眼花缭乱，实际上并没有，只是分各种情况处理操作码而已。
	// VM 之所以叫 VM不是因为它特别复杂，有特别多的机器接口或底层优化，而是因为它执行操作码
	// 由于执行自己的指令集，所以才叫VM
	
// 操作码存在的意义是什么呢？最重要的意义是对业务逻辑进行抽象，降低耦合，就像软件和硬件。
	// 把整个程序拆分成编译部分和执行部分，通过操作码分割开
	// 操作码是在编译后生成，它可以被缓存，让运行效率变得更高

// 共 350 个方法，这里才是核心，不要被 zend_vm_execute.h 吓到
// 239个 handler, 39个helper. helper 其实是用来给handler 调用的方法。不会在其他地方用到。

// SPEC_HANDLER 是特殊处理器

// ing3, 加法助手 , 检验传入的变量是否有效，进行加法运算，然后销毁运算对象op1,op2 
ZEND_VM_HELPER(zend_add_helper, ANY, ANY, zval *op_1, zval *op_2)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	SAVE_OPLINE();
	// 如果 变量类型为未定义
	if (UNEXPECTED(Z_TYPE_INFO_P(op_1) == IS_UNDEF)) {
		// 报错：p1的变量（变量名）未定义, 并返未初始化zval
		op_1 = ZVAL_UNDEFINED_OP1();
	}
	// 如果 变量类型为 未定义
	if (UNEXPECTED(Z_TYPE_INFO_P(op_2) == IS_UNDEF)) {
		// 报错：p2的变量（变量名）未定义, 并返未初始化zval
		op_2 = ZVAL_UNDEFINED_OP2();
	}
	// 加法函数 
	add_function(EX_VAR(opline->result.var), op_1, op_2);
	// 如果 op1 类型 是变量或临时变量
	if (OP1_TYPE & (IS_TMP_VAR|IS_VAR)) {
		// 销毁没有引用次数的对象,并没有销毁 zval本身，不放入gc回收周期。
		zval_ptr_dtor_nogc(op_1);
	}
	// 如果 op2 类型 是变量或临时变量
	if (OP2_TYPE & (IS_TMP_VAR|IS_VAR)) {
		// 销毁没有引用次数的对象,并没有销毁 zval本身，不放入gc回收周期。
		zval_ptr_dtor_nogc(op_2);
	}
	// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, 加法运算 1/4
ZEND_VM_HOT_NOCONSTCONST_HANDLER(1, ZEND_ADD, CONST|TMPVARCV, CONST|TMPVARCV)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2, *result;
	double d1, d2;

	// 获取zval指针, UNUSED 返回null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 获取zval指针, UNUSED 返回null
	op2 = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// op1或op2有一个不是ANY 并且 如果两个都是常量
	if (ZEND_VM_SPEC && OP1_TYPE == IS_CONST && OP2_TYPE == IS_CONST) {
		/* pass */
	// 如果 op1 是整数
	} else if (EXPECTED(Z_TYPE_INFO_P(op1) == IS_LONG)) {
		// 如果 op2 也是整数
		if (EXPECTED(Z_TYPE_INFO_P(op2) == IS_LONG)) {
			// 在执行数据中取出 指定序号的变量
			result = EX_VAR(opline->result.var);
			// 快速加法
			fast_long_add_function(result, op1, op2);
			// opline + 1，到目标操作码并 return
			ZEND_VM_NEXT_OPCODE();
		// 如果 op2 是双精度
		} else if (EXPECTED(Z_TYPE_INFO_P(op2) == IS_DOUBLE)) {
			// op1 的值转成 双精度 
			d1 = (double)Z_LVAL_P(op1);
			// 取出小数
			d2 = Z_DVAL_P(op2);
			// 跳转到指定标签 add_double
			ZEND_VM_C_GOTO(add_double);
		}
	// 如果 op1 是双精度
	} else if (EXPECTED(Z_TYPE_INFO_P(op1) == IS_DOUBLE)) {
		// 如果 op2 是双精度
		if (EXPECTED(Z_TYPE_INFO_P(op2) == IS_DOUBLE)) {
			// 取出小数
			d1 = Z_DVAL_P(op1);
			// 取出小数
			d2 = Z_DVAL_P(op2);
// 创建跳转标签 add_double
ZEND_VM_C_LABEL(add_double):
			// 在执行数据中取出 指定序号的变量
			result = EX_VAR(opline->result.var);
			// 做加法并把值赋给结果 
			ZVAL_DOUBLE(result, d1 + d2);
			// opline + 1，到目标操作码并 return
			ZEND_VM_NEXT_OPCODE();
		// 如果 op2 是整数
		} else if (EXPECTED(Z_TYPE_INFO_P(op2) == IS_LONG)) {
			// 取出小数
			d1 = Z_DVAL_P(op1);
			// op2的值转成双精度
			d2 = (double)Z_LVAL_P(op2);
			// 跳转到指定标签 add_double
			ZEND_VM_C_GOTO(add_double);
		}
	}
	
	// 调用方法 zend_add_helper
	ZEND_VM_DISPATCH_TO_HELPER(zend_add_helper, op_1, op1, op_2, op2);
}

// ing3, 减法助手 , 检验传入的变量是否有效，进行减法运算，然后销毁运算对象op1,op2 
ZEND_VM_HELPER(zend_sub_helper, ANY, ANY, zval *op_1, zval *op_2)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// windows: 无操作
	SAVE_OPLINE();
	
	// 如果 变量类型为未定义
	if (UNEXPECTED(Z_TYPE_INFO_P(op_1) == IS_UNDEF)) {
		// 报错：p1的变量（变量名）未定义, 并返未初始化zval
		op_1 = ZVAL_UNDEFINED_OP1();
	}
	// 如果 变量类型为 未定义
	if (UNEXPECTED(Z_TYPE_INFO_P(op_2) == IS_UNDEF)) {
		// 报错：p2的变量（变量名）未定义, 并返未初始化zval
		op_2 = ZVAL_UNDEFINED_OP2();
	}
	// 做减法，把值存到结果里
	sub_function(EX_VAR(opline->result.var), op_1, op_2);
	// 如果 op1 类型 是变量或临时变量
	if (OP1_TYPE & (IS_TMP_VAR|IS_VAR)) {
		// 销毁没有引用次数的对象,并没有销毁 zval本身，不放入gc回收周期。
		zval_ptr_dtor_nogc(op_1);
	}
	// 如果 op2 类型 是变量或临时变量
	if (OP2_TYPE & (IS_TMP_VAR|IS_VAR)) {
		// 销毁没有引用次数的对象,并没有销毁 zval本身，不放入gc回收周期。
		zval_ptr_dtor_nogc(op_2);
	}
	// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, 减法 1/4
ZEND_VM_HOT_NOCONSTCONST_HANDLER(2, ZEND_SUB, CONST|TMPVARCV, CONST|TMPVARCV)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2, *result;
	double d1, d2;

	// 获取zval指针, UNUSED 返回null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 获取zval指针, UNUSED 返回null
	op2 = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// op1或op2有一个不是ANY 并且两个都是常量
	if (ZEND_VM_SPEC && OP1_TYPE == IS_CONST && OP2_TYPE == IS_CONST) {
		/* pass */
	// 如果op1是整数
	} else if (EXPECTED(Z_TYPE_INFO_P(op1) == IS_LONG)) {
		// 如果op2是整数
		if (EXPECTED(Z_TYPE_INFO_P(op2) == IS_LONG)) {
			// 在执行数据中取出 指定序号的变量
			result = EX_VAR(opline->result.var);
			// 快速减法
			fast_long_sub_function(result, op1, op2);
			// opline + 1，到目标操作码并 return
			ZEND_VM_NEXT_OPCODE();
		// 如果op2是小数	
		} else if (EXPECTED(Z_TYPE_INFO_P(op2) == IS_DOUBLE)) {
			// op1转成小数
			d1 = (double)Z_LVAL_P(op1);
			// 取出小数
			d2 = Z_DVAL_P(op2);
			// 跳转到指定标签 sub_double
			ZEND_VM_C_GOTO(sub_double);
		}
	// 如果op1是小数
	} else if (EXPECTED(Z_TYPE_INFO_P(op1) == IS_DOUBLE)) {
		// 如果op2是小数
		if (EXPECTED(Z_TYPE_INFO_P(op2) == IS_DOUBLE)) {
			// 取出小数
			d1 = Z_DVAL_P(op1);
			// 取出小数
			d2 = Z_DVAL_P(op2);
// 创建跳转标签 sub_double
ZEND_VM_C_LABEL(sub_double):
			// 在执行数据中取出 指定序号的变量
			result = EX_VAR(opline->result.var);
			// 算减法，结果为小数
			ZVAL_DOUBLE(result, d1 - d2);
			// opline + 1，到目标操作码并 return
			ZEND_VM_NEXT_OPCODE();
		// 否则 如果 op2 是整数
		} else if (EXPECTED(Z_TYPE_INFO_P(op2) == IS_LONG)) {
			// 取出小数
			d1 = Z_DVAL_P(op1);
			// 取出小数
			d2 = (double)Z_LVAL_P(op2);
			// 跳转到指定标签 sub_double
			ZEND_VM_C_GOTO(sub_double);
		}
	}

	// 调用方法 zend_sub_helper
	ZEND_VM_DISPATCH_TO_HELPER(zend_sub_helper, op_1, op1, op_2, op2);
}

// ing3, 乘法助手, 检验传入的变量是否有效，进行乘法运算，然后销毁运算对象op1,op2 
ZEND_VM_HELPER(zend_mul_helper, ANY, ANY, zval *op_1, zval *op_2)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// windows: 无操作
	SAVE_OPLINE();
	// 如果 变量类型为未定义
	if (UNEXPECTED(Z_TYPE_INFO_P(op_1) == IS_UNDEF)) {
		// 报错：p1的变量（变量名）未定义, 并返未初始化zval
		op_1 = ZVAL_UNDEFINED_OP1();
	}
	// 如果 变量类型为 未定义
	if (UNEXPECTED(Z_TYPE_INFO_P(op_2) == IS_UNDEF)) {
		// 报错：p2的变量（变量名）未定义, 并返未初始化zval
		op_2 = ZVAL_UNDEFINED_OP2();
	}
	// 进行乘法运算
	mul_function(EX_VAR(opline->result.var), op_1, op_2);
	// 如果 op1 类型 是变量或临时变量
	if (OP1_TYPE & (IS_TMP_VAR|IS_VAR)) {
		// 销毁没有引用次数的对象,并没有销毁 zval本身，不放入gc回收周期。
		zval_ptr_dtor_nogc(op_1);
	}
	// 如果 op2 类型 是变量或临时变量
	if (OP2_TYPE & (IS_TMP_VAR|IS_VAR)) {
		// 销毁没有引用次数的对象,并没有销毁 zval本身，不放入gc回收周期。
		zval_ptr_dtor_nogc(op_2);
	}
	// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ZEND_MUL，乘法 1/4， 下属？个分支（验证过？）
// ing3, 乘法
ZEND_VM_COLD_CONSTCONST_HANDLER(3, ZEND_MUL, CONST|TMPVARCV, CONST|TMPVARCV, SPEC(COMMUTATIVE))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2, *result;
	double d1, d2;

	// 获取zval指针, UNUSED 返回null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 获取zval指针, UNUSED 返回null
	op2 = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// op1或op2有一个不是ANY
	if (ZEND_VM_SPEC && OP1_TYPE == IS_CONST && OP2_TYPE == IS_CONST) {
		/* pass */
	// 如果op1是整数
	} else if (EXPECTED(Z_TYPE_INFO_P(op1) == IS_LONG)) {
		// 如果op2是整数
		if (EXPECTED(Z_TYPE_INFO_P(op2) == IS_LONG)) {
			// 溢出标记
			zend_long overflow;
			// 在执行数据中取出 指定序号的变量
			result = EX_VAR(opline->result.var);
			// 计数整数乘法
			ZEND_SIGNED_MULTIPLY_LONG(Z_LVAL_P(op1), Z_LVAL_P(op2), Z_LVAL_P(result), Z_DVAL_P(result), overflow);
			// 如果溢出了，结果转成小数
			Z_TYPE_INFO_P(result) = overflow ? IS_DOUBLE : IS_LONG;
			// opline + 1，到目标操作码并 return
			ZEND_VM_NEXT_OPCODE();
			
		// 如果op2是小数
		} else if (EXPECTED(Z_TYPE_INFO_P(op2) == IS_DOUBLE)) {
			// op1转成小数
			d1 = (double)Z_LVAL_P(op1);
			// 取出小数
			d2 = Z_DVAL_P(op2);
			// 跳转到指定标签 mul_double
			ZEND_VM_C_GOTO(mul_double);
		}
	// 如果op1是小数
	} else if (EXPECTED(Z_TYPE_INFO_P(op1) == IS_DOUBLE)) {
		// 如果 op2 是小数
		if (EXPECTED(Z_TYPE_INFO_P(op2) == IS_DOUBLE)) {
			// 取出小数
			d1 = Z_DVAL_P(op1);
			// 取出小数
			d2 = Z_DVAL_P(op2);
// 创建跳转标签 mul_double
ZEND_VM_C_LABEL(mul_double):
			// 在执行数据中取出 指定序号的变量
			result = EX_VAR(opline->result.var);
			// 直接算乘法，结果为小数
			ZVAL_DOUBLE(result, d1 * d2);
			// opline + 1，到目标操作码并 return
			ZEND_VM_NEXT_OPCODE();
		// 如果 op2 是整数
		} else if (EXPECTED(Z_TYPE_INFO_P(op2) == IS_LONG)) {
			// 取出小数
			d1 = Z_DVAL_P(op1);
			// op2转成小数
			d2 = (double)Z_LVAL_P(op2);
			// 跳转到指定标签 mul_double
			ZEND_VM_C_GOTO(mul_double);
		}
	}

	// 调用方法 zend_mul_helper
	ZEND_VM_DISPATCH_TO_HELPER(zend_mul_helper, op_1, op1, op_2, op2);
}

// ing3, 除法 1/1 
// 下属9个分支（验证过）
ZEND_VM_COLD_CONSTCONST_HANDLER(4, ZEND_DIV, CONST|TMPVAR|CV, CONST|TMPVAR|CV)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2;

	// windows: 无操作
	SAVE_OPLINE();
	// 获取zval指针，UNUSED 类型返回null
	op1 = GET_OP1_ZVAL_PTR(BP_VAR_R);
	// 获取zval指针，UNUSED 类型返回null
	op2 = GET_OP2_ZVAL_PTR(BP_VAR_R);
	// 除法函数 
	div_function(EX_VAR(opline->result.var), op1, op2);
	FREE_OP1();
	// 释放操作对象的附加变量
	FREE_OP2();
	// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, 除以0，助手 ，1个分支（验证过）
ZEND_VM_COLD_HELPER(zend_mod_by_zero_helper, ANY, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// windows: 无操作
	SAVE_OPLINE();
	// 抛异常
	zend_throw_exception_ex(zend_ce_division_by_zero_error, 0, "Modulo by zero");
	// 结果为 未定义
	ZVAL_UNDEF(EX_VAR(opline->result.var));
	// 主要是 return
	HANDLE_EXCEPTION();
}

// ing3, 取余助手 , 检验传入的变量是否有效，进行取余运算，然后销毁运算对象op1,op2 
ZEND_VM_HELPER(zend_mod_helper, ANY, ANY, zval *op_1, zval *op_2)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// windows: 无操作
	SAVE_OPLINE();
	// 如果 变量类型为未定义
	if (UNEXPECTED(Z_TYPE_INFO_P(op_1) == IS_UNDEF)) {
		// 报错：p1的变量（变量名）未定义, 并返未初始化zval
		op_1 = ZVAL_UNDEFINED_OP1();
	}
	// 如果 变量类型为 未定义
	if (UNEXPECTED(Z_TYPE_INFO_P(op_2) == IS_UNDEF)) {
		// 报错：p2的变量（变量名）未定义, 并返未初始化zval
		op_2 = ZVAL_UNDEFINED_OP2();
	}
	// 进行取余运算
	mod_function(EX_VAR(opline->result.var), op_1, op_2);
	// 如果 op1 类型 是变量或临时变量
	if (OP1_TYPE & (IS_TMP_VAR|IS_VAR)) {
		// 销毁没有引用次数的对象,并没有销毁 zval本身，不放入gc回收周期。
		zval_ptr_dtor_nogc(op_1);
	}
	// 如果 op2 类型 是变量或临时变量
	if (OP2_TYPE & (IS_TMP_VAR|IS_VAR)) {
		// 销毁没有引用次数的对象,并没有销毁 zval本身，不放入gc回收周期。
		zval_ptr_dtor_nogc(op_2);
	}
	// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, 取余运算
ZEND_VM_COLD_CONSTCONST_HANDLER(5, ZEND_MOD, CONST|TMPVARCV, CONST|TMPVARCV)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2, *result;

	// 获取zval指针, UNUSED 返回null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 获取zval指针, UNUSED 返回null
	op2 = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// op1或op2有一个不是ANY 并且 两个都是常量
	if (ZEND_VM_SPEC && OP1_TYPE == IS_CONST && OP2_TYPE == IS_CONST) {
		/* pass */
	// 如果op2不是整数 
	} else if (EXPECTED(Z_TYPE_INFO_P(op1) == IS_LONG)) {
		// 如果 op2 是整数 
		if (EXPECTED(Z_TYPE_INFO_P(op2) == IS_LONG)) {
			// 在执行数据中取出 指定序号的变量
			result = EX_VAR(opline->result.var);
			// 如果操作对象2值为0
			if (UNEXPECTED(Z_LVAL_P(op2) == 0)) {
				// 调用方法 zend_mod_by_zero_helper
				ZEND_VM_DISPATCH_TO_HELPER(zend_mod_by_zero_helper);
			// 如果操作对象2 值为 -1
			} else if (UNEXPECTED(Z_LVAL_P(op2) == -1)) {
				// 如果op1值为 ZEND_LONG_MIN，防止 溢出报错
				/* Prevent overflow error/crash if op1==ZEND_LONG_MIN */
				// 结果为0
				ZVAL_LONG(result, 0);
			// 否则
			} else {
				// 直接取余运算
				ZVAL_LONG(result, Z_LVAL_P(op1) % Z_LVAL_P(op2));
			}
			// opline + 1，到目标操作码并 return
			ZEND_VM_NEXT_OPCODE();
		}
	}

	// 取余助手 , 检验传入的变量是否有效，进行取余运算，然后销毁运算对象op1,op2 
	// 调用方法 zend_mod_helper
	ZEND_VM_DISPATCH_TO_HELPER(zend_mod_helper, op_1, op1, op_2, op2);
}

// ing3, 左移运算助手 , 检验传入的变量是否有效，进行左移运算，然后销毁运算对象op1,op2 
ZEND_VM_HELPER(zend_shift_left_helper, ANY, ANY, zval *op_1, zval *op_2)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// windows: 无操作
	SAVE_OPLINE();
	// 如果 变量类型为未定义
	if (UNEXPECTED(Z_TYPE_INFO_P(op_1) == IS_UNDEF)) {
		// 报错：p1的变量（变量名）未定义, 并返未初始化zval
		op_1 = ZVAL_UNDEFINED_OP1();
	}
	// 如果 变量类型为 未定义
	if (UNEXPECTED(Z_TYPE_INFO_P(op_2) == IS_UNDEF)) {
		// 报错：p2的变量（变量名）未定义, 并返未初始化zval
		op_2 = ZVAL_UNDEFINED_OP2();
	}
	// 进行左移运算
	shift_left_function(EX_VAR(opline->result.var), op_1, op_2);
	// 如果 op1 类型 是变量或临时变量
	if (OP1_TYPE & (IS_TMP_VAR|IS_VAR)) {
		// 销毁没有引用次数的对象,并没有销毁 zval本身，不放入gc回收周期。
		zval_ptr_dtor_nogc(op_1);
	}
	// 如果 op2 类型 是变量或临时变量
	if (OP2_TYPE & (IS_TMP_VAR|IS_VAR)) {
		// 销毁没有引用次数的对象,并没有销毁 zval本身，不放入gc回收周期。
		zval_ptr_dtor_nogc(op_2);
	}
	// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// 4个分支（验证过）
// ing3, 左移
ZEND_VM_COLD_CONSTCONST_HANDLER(6, ZEND_SL, CONST|TMPVARCV, CONST|TMPVARCV)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2;

	// 获取zval指针, UNUSED 返回null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 获取zval指针, UNUSED 返回null
	op2 = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// op1或op2有一个不是ANY
	// 如果两个都是常量，什么也不做，因为应该处理过了
	if (ZEND_VM_SPEC && OP1_TYPE == IS_CONST && OP2_TYPE == IS_CONST) {
		/* pass */
	// 如果两个都是整数 并且 op2 小于一个完整整数的位置数（移动后结果不为0）
	} else if (EXPECTED(Z_TYPE_INFO_P(op1) == IS_LONG)
			&& EXPECTED(Z_TYPE_INFO_P(op2) == IS_LONG)
			&& EXPECTED((zend_ulong)Z_LVAL_P(op2) < SIZEOF_ZEND_LONG * 8)) {
		
		// 转成无符号来进行位移，可以得到一个更好的封装行为？
		/* Perform shift on unsigned numbers to get well-defined wrap behavior. */
		
		// 把op1转成无符号，进行位移运算，再转回来
		ZVAL_LONG(EX_VAR(opline->result.var),
			(zend_long) ((zend_ulong) Z_LVAL_P(op1) << Z_LVAL_P(op2)));
		// opline + 1，到目标操作码并 return
		ZEND_VM_NEXT_OPCODE();
	}

	// 调用方法 zend_shift_left_helper
	ZEND_VM_DISPATCH_TO_HELPER(zend_shift_left_helper, op_1, op1, op_2, op2);
}

// ing3, 右移运算助手, 检验传入的变量是否有效，进行右移运算，然后销毁运算对象op1,op2 
ZEND_VM_HELPER(zend_shift_right_helper, ANY, ANY, zval *op_1, zval *op_2)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// windows: 无操作
	SAVE_OPLINE();
	// 如果 变量类型为未定义
	if (UNEXPECTED(Z_TYPE_INFO_P(op_1) == IS_UNDEF)) {
		// 报错：p1的变量（变量名）未定义, 并返未初始化zval
		op_1 = ZVAL_UNDEFINED_OP1();
	}
	// 如果 变量类型为 未定义
	if (UNEXPECTED(Z_TYPE_INFO_P(op_2) == IS_UNDEF)) {
		// 报错：p2的变量（变量名）未定义, 并返未初始化zval
		op_2 = ZVAL_UNDEFINED_OP2();
	}
	// 进行右移运算
	shift_right_function(EX_VAR(opline->result.var), op_1, op_2);
	// 如果 op1 类型 是变量或临时变量
	if (OP1_TYPE & (IS_TMP_VAR|IS_VAR)) {
		// 销毁没有引用次数的对象,并没有销毁 zval本身，不放入gc回收周期。
		zval_ptr_dtor_nogc(op_1);
	}
	// 如果 op2 类型 是变量或临时变量
	if (OP2_TYPE & (IS_TMP_VAR|IS_VAR)) {
		// 销毁没有引用次数的对象,并没有销毁 zval本身，不放入gc回收周期。
		zval_ptr_dtor_nogc(op_2);
	}
	// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// 2*2 = 4个分支
// ing3, 右移运算
ZEND_VM_COLD_CONSTCONST_HANDLER(7, ZEND_SR, CONST|TMPVARCV, CONST|TMPVARCV)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2;

	// 获取zval指针, UNUSED 返回null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 获取zval指针, UNUSED 返回null
	op2 = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// op1或op2有一个不是ANY
	if (ZEND_VM_SPEC && OP1_TYPE == IS_CONST && OP2_TYPE == IS_CONST) {
		/* pass */
	// 如果两个都是整数 并且 op2 小于一个完整整数的位置数（移动后结果不为0）
	} else if (EXPECTED(Z_TYPE_INFO_P(op1) == IS_LONG)
			&& EXPECTED(Z_TYPE_INFO_P(op2) == IS_LONG)
			&& EXPECTED((zend_ulong)Z_LVAL_P(op2) < SIZEOF_ZEND_LONG * 8)) {
		// 直接进行位移运算
		ZVAL_LONG(EX_VAR(opline->result.var), Z_LVAL_P(op1) >> Z_LVAL_P(op2));
		// opline + 1，到目标操作码并 return
		ZEND_VM_NEXT_OPCODE();
	}

	// 右移运算助手, 检验传入的变量是否有效，进行右移运算，然后销毁运算对象op1,op2 
	// 调用方法 zend_shift_right_helper
	ZEND_VM_DISPATCH_TO_HELPER(zend_shift_right_helper, op_1, op1, op_2, op2);
}

// ing3, 幂运算
ZEND_VM_COLD_CONSTCONST_HANDLER(12, ZEND_POW, CONST|TMPVAR|CV, CONST|TMPVAR|CV)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2;

	// windows: 无操作
	SAVE_OPLINE();
	// 获取zval指针，UNUSED 类型返回null
	op1 = GET_OP1_ZVAL_PTR(BP_VAR_R);
	// 获取zval指针，UNUSED 类型返回null
	op2 = GET_OP2_ZVAL_PTR(BP_VAR_R);
	// 慢幂运算
	pow_function(EX_VAR(opline->result.var), op1, op2);
	// 释放操作对象的附加变量
	FREE_OP1();
	// 释放操作对象的附加变量
	FREE_OP2();
	// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, 连接字串 1/1。额外规则：不能两个都是常量。3*3-1 = 8个分支（确认过）
// 为什么不能两个常量呢？因为两个常量用不到操作码，直接在编译时连接就行了
ZEND_VM_HANDLER(8, ZEND_CONCAT, CONST|TMPVAR|CV, CONST|TMPVAR|CV, SPEC(NO_CONST_CONST))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2;

	// 取得两个操作对象中的内置变量
	// 获取zval指针, UNUSED 返回null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 获取zval指针, UNUSED 返回null
	op2 = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);

	// 两个类型都是：常量 或 字串
	if ((OP1_TYPE == IS_CONST || EXPECTED(Z_TYPE_P(op1) == IS_STRING)) &&
	    (OP2_TYPE == IS_CONST || EXPECTED(Z_TYPE_P(op2) == IS_STRING))) {
		// 取出两个字串
		zend_string *op1_str = Z_STR_P(op1);
		zend_string *op2_str = Z_STR_P(op2);
		zend_string *str;

		// 如果 op1不是常量 并且字串长度是0。op1可忽略
		if (OP1_TYPE != IS_CONST && UNEXPECTED(ZSTR_LEN(op1_str) == 0)) {
			// op2类型是常量 或 编译变量
			if (OP2_TYPE == IS_CONST || OP2_TYPE == IS_CV) {
				// op2的字串当成结果
				ZVAL_STR_COPY(EX_VAR(opline->result.var), op2_str);
			// op2类型是临时变量 TMPVAR
			// 否则
			} else {
				// op2的字串当成结果
				ZVAL_STR(EX_VAR(opline->result.var), op2_str);
			}
			// 如果 op1 类型 是变量或临时变量
			if (OP1_TYPE & (IS_TMP_VAR|IS_VAR)) {
				// 释放op1字串
				zend_string_release_ex(op1_str, 0);
			}
			// （结束了）
		// 如果 op2不是常量 并且字串长度是 0。op2可忽略
		} else if (OP2_TYPE != IS_CONST && UNEXPECTED(ZSTR_LEN(op2_str) == 0)) {
			// op1类型是常量 或 编译变量
			if (OP1_TYPE == IS_CONST || OP1_TYPE == IS_CV) {
				// op1的字串当成结果
				ZVAL_STR_COPY(EX_VAR(opline->result.var), op1_str);
			// op1类型是临时变量 TMPVAR	
			// 否则
			} else {
				ZVAL_STR(EX_VAR(opline->result.var), op1_str);
			}
			// 如果 op2 类型 是变量或临时变量
			if (OP2_TYPE & (IS_TMP_VAR|IS_VAR)) {
				// 释放op2字串
				zend_string_release_ex(op2_str, 0);
			}
		// 如果 op1,op2都不可忽略。如果op1不是常量也不是编译变量 ，它的值也不是保留字，并且引用次数是1
		} else if (OP1_TYPE != IS_CONST && OP1_TYPE != IS_CV &&
		    !ZSTR_IS_INTERNED(op1_str) && GC_REFCOUNT(op1_str) == 1) {
			// 字串长度
			size_t len = ZSTR_LEN(op1_str);
			// 如果 连接后长度会大于字串最大长度
			if (UNEXPECTED(len > ZSTR_MAX_LEN - ZSTR_LEN(op2_str))) {
				// 报错，字串超长
				zend_error_noreturn(E_ERROR, "Integer overflow in memory allocation");
			}
			// 给op1的字串增加长度
			str = zend_string_extend(op1_str, len + ZSTR_LEN(op2_str), 0);
			// 后面一段复制op2的内容
			memcpy(ZSTR_VAL(str) + len, ZSTR_VAL(op2_str), ZSTR_LEN(op2_str)+1);
			// 把zend_string 添加给 zval，不支持保留字
			ZVAL_NEW_STR(EX_VAR(opline->result.var), str);
			// 如果 op2 类型 是变量或临时变量
			if (OP2_TYPE & (IS_TMP_VAR|IS_VAR)) {
				// 释放op2字串
				zend_string_release_ex(op2_str, 0);
			}
		// 其他情况？
		// 否则
		} else {
			// 创建新字串，长度是两个字串的和
			str = zend_string_alloc(ZSTR_LEN(op1_str) + ZSTR_LEN(op2_str), 0);
			// 把两个字串复制到新串里
			memcpy(ZSTR_VAL(str), ZSTR_VAL(op1_str), ZSTR_LEN(op1_str));
			memcpy(ZSTR_VAL(str) + ZSTR_LEN(op1_str), ZSTR_VAL(op2_str), ZSTR_LEN(op2_str)+1);
			// 把zend_string 添加给 zval，不支持保留字
			ZVAL_NEW_STR(EX_VAR(opline->result.var), str);
			// 如果 op1 类型 是变量或临时变量
			if (OP1_TYPE & (IS_TMP_VAR|IS_VAR)) {
				// 释放op1字串
				zend_string_release_ex(op1_str, 0);
			}
			// 如果 op2 类型 是变量或临时变量
			if (OP2_TYPE & (IS_TMP_VAR|IS_VAR)) {
				// 释放op2字串
				zend_string_release_ex(op2_str, 0);
			}
		}
		// 下一个操作码
		// opline + 1，到目标操作码并 return
		ZEND_VM_NEXT_OPCODE();
	// 其他情况
	// 否则
	} else {
		// windows: 无操作
		SAVE_OPLINE();

		// 如果操作对象类型是编译变量 ，内置变量类型是 未定义
		if (OP1_TYPE == IS_CV && UNEXPECTED(Z_TYPE_P(op1) == IS_UNDEF)) {
			// 报错：p1的变量（变量名）未定义, 并返未初始化zval
			op1 = ZVAL_UNDEFINED_OP1();
		}
		// 如果 op2 是编译变量 并且 类型是 未定义
		if (OP2_TYPE == IS_CV && UNEXPECTED(Z_TYPE_P(op2) == IS_UNDEF)) {
			// 报错：p2的变量（变量名）未定义, 并返未初始化zval
			op2 = ZVAL_UNDEFINED_OP2();
		}
		// 连接两个内置变量，并把结果返回给 result
		concat_function(EX_VAR(opline->result.var), op1, op2);
		// 释放操作对象的附加变量
		FREE_OP1();
		// 释放操作对象的附加变量
		FREE_OP2();
		// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
		ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
	}
}

// ing3, 比较相同运算，然后销毁运算对象op1,op2 
ZEND_VM_COLD_CONSTCONST_HANDLER(16, ZEND_IS_IDENTICAL, CONST|TMP|VAR|CV, CONST|TMP|VAR|CV, SPEC(COMMUTATIVE))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2;
	bool result;

	// windows: 无操作
	SAVE_OPLINE();
	// 获取zval指针, UNUSED 返回null，不支持 TMPVAR/TMPVARCV
	op1 = GET_OP1_ZVAL_PTR_DEREF(BP_VAR_R);
	// 获取zval指针, UNUSED 返回null，不支持 TMPVAR/TMPVARCV
	op2 = GET_OP2_ZVAL_PTR_DEREF(BP_VAR_R);
	// 检查是否相同
	result = fast_is_identical_function(op1, op2);
	// 释放操作对象的附加变量
	FREE_OP1();
	// 释放操作对象的附加变量
	FREE_OP2();
	// 根据当前操作码结果选择下一个操作码，p1:传入值，主要用来做判断。p2:是否检查异常
	ZEND_VM_SMART_BRANCH(result, 1);
}

// ing3, case语句 比较相同运算，然后销毁运算对象 op2 
ZEND_VM_HANDLER(196, ZEND_CASE_STRICT, TMP|VAR, CONST|TMP|VAR|CV)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2;
	bool result;

	// windows: 无操作
	SAVE_OPLINE();
	// 获取zval指针, UNUSED 返回null，不支持 TMPVAR/TMPVARCV
	op1 = GET_OP1_ZVAL_PTR_DEREF(BP_VAR_R);
	// 获取zval指针, UNUSED 返回null，不支持 TMPVAR/TMPVARCV
	op2 = GET_OP2_ZVAL_PTR_DEREF(BP_VAR_R);
	// 检查是否相同
	result = fast_is_identical_function(op1, op2);
	// 释放操作对象的附加变量
	FREE_OP2();
	// 根据当前操作码结果选择下一个操作码，p1:传入值，主要用来做判断。p2:是否检查异常
	ZEND_VM_SMART_BRANCH(result, 1);
}

// ing3, 比较不相同 运算，然后销毁运算对象op1,op2 
ZEND_VM_COLD_CONSTCONST_HANDLER(17, ZEND_IS_NOT_IDENTICAL, CONST|TMP|VAR|CV, CONST|TMP|VAR|CV, SPEC(COMMUTATIVE))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2;
	bool result;

	// windows: 无操作
	SAVE_OPLINE();
	// 获取zval指针, UNUSED 返回null，不支持 TMPVAR/TMPVARCV
	op1 = GET_OP1_ZVAL_PTR_DEREF(BP_VAR_R);
	// 获取zval指针, UNUSED 返回null，不支持 TMPVAR/TMPVARCV
	op2 = GET_OP2_ZVAL_PTR_DEREF(BP_VAR_R);
	// 检查是否 不相同
	result = fast_is_not_identical_function(op1, op2);
	// 释放操作对象的附加变量
	FREE_OP1();
	// 释放操作对象的附加变量
	FREE_OP2();
	// 根据当前操作码结果选择下一个操作码，p1:传入值，主要用来做判断。p2:是否检查异常
	ZEND_VM_SMART_BRANCH(result, 1);
}

// ing3, 计算相等, 检验传入的变量是否有效，进行 比较 运算，然后销毁运算对象op1,op2 
ZEND_VM_HELPER(zend_is_equal_helper, ANY, ANY, zval *op_1, zval *op_2)
{
	int ret;
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// windows: 无操作
	SAVE_OPLINE();
	// 如果 变量类型为未定义
	if (UNEXPECTED(Z_TYPE_INFO_P(op_1) == IS_UNDEF)) {
		// 报错：p1的变量（变量名）未定义, 并返未初始化zval
		op_1 = ZVAL_UNDEFINED_OP1();
	}
	// 如果 变量类型为 未定义
	if (UNEXPECTED(Z_TYPE_INFO_P(op_2) == IS_UNDEF)) {
		// 报错：p2的变量（变量名）未定义, 并返未初始化zval
		op_2 = ZVAL_UNDEFINED_OP2();
	}
	// 进行比较运算
	ret = zend_compare(op_1, op_2);
	// 如果 op1 类型 是变量或临时变量
	if (OP1_TYPE & (IS_TMP_VAR|IS_VAR)) {
		// 销毁没有引用次数的对象,并没有销毁 zval本身，不放入gc回收周期。
		zval_ptr_dtor_nogc(op_1);
	}
	// 如果 op2 类型 是变量或临时变量
	if (OP2_TYPE & (IS_TMP_VAR|IS_VAR)) {
		// 销毁没有引用次数的对象,并没有销毁 zval本身，不放入gc回收周期。
		zval_ptr_dtor_nogc(op_2);
	}
	// 根据当前操作码结果选择下一个操作码，p1:传入值，主要用来做判断。p2:是否检查异常
	// 如果比较结果 等于0 返回true ,否则 false
	ZEND_VM_SMART_BRANCH(ret == 0, 1);
}

// ing3, 比较相等 操作码
ZEND_VM_COLD_CONSTCONST_HANDLER(18, ZEND_IS_EQUAL, CONST|TMPVAR|CV, CONST|TMPVAR|CV, SPEC(SMART_BRANCH,COMMUTATIVE))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2;
	double d1, d2;
	// 获取zval指针, UNUSED 返回null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 获取zval指针, UNUSED 返回null
	op2 = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// op1或op2有一个不是ANY
	if (ZEND_VM_SPEC && OP1_TYPE == IS_CONST && OP2_TYPE == IS_CONST) {
		/* pass */
	// 否则 如果类型是 整数
	} else if (EXPECTED(Z_TYPE_P(op1) == IS_LONG)) {
		// 如果类型是 整数
		if (EXPECTED(Z_TYPE_P(op2) == IS_LONG)) {
			// 如果两个整数相等
			if (EXPECTED(Z_LVAL_P(op1) == Z_LVAL_P(op2))) {
// 创建跳转标签 is_equal_true
ZEND_VM_C_LABEL(is_equal_true):
				// 根据当前操作码结果选择下一个操作码。如果不是跳转类型，结果赋值为true。
				ZEND_VM_SMART_BRANCH_TRUE();
			// 否则
			} else {
// 创建跳转标签 is_equal_false
ZEND_VM_C_LABEL(is_equal_false):
				// 根据当前操作码结果选择下一个操作码。如果不是跳转类型，结果赋值为false。
				ZEND_VM_SMART_BRANCH_FALSE();
			}
		// 如果类型是 小数
		} else if (EXPECTED(Z_TYPE_P(op2) == IS_DOUBLE)) {
			// op1转成小数
			d1 = (double)Z_LVAL_P(op1);
			// 取出小数
			d2 = Z_DVAL_P(op2);
			// 跳转到指定标签 is_equal_double
			ZEND_VM_C_GOTO(is_equal_double);
		}
	// 否则 如果类型是 小数
	} else if (EXPECTED(Z_TYPE_P(op1) == IS_DOUBLE)) {
		// 如果类型是 小数
		if (EXPECTED(Z_TYPE_P(op2) == IS_DOUBLE)) {
			// 取出小数
			d1 = Z_DVAL_P(op1);
			// 取出小数
			d2 = Z_DVAL_P(op2);
// 创建跳转标签 is_equal_double
ZEND_VM_C_LABEL(is_equal_double):
			// 如果两个小数相等
			if (d1 == d2) {
				// 跳转到指定标签 is_equal_true
				ZEND_VM_C_GOTO(is_equal_true);
			// 否则
			} else {
				// 跳转到指定标签 is_equal_false
				ZEND_VM_C_GOTO(is_equal_false);
			}
		// 否则 如果类型是 整数
		} else if (EXPECTED(Z_TYPE_P(op2) == IS_LONG)) {
			// 取出小数
			d1 = Z_DVAL_P(op1);
			// op2转成小数
			d2 = (double)Z_LVAL_P(op2);
			// 跳转到指定标签 is_equal_double
			ZEND_VM_C_GOTO(is_equal_double);
		}
	// 否则 如果类型是 字串
	} else if (EXPECTED(Z_TYPE_P(op1) == IS_STRING)) {
		// 如果类型是 字串
		if (EXPECTED(Z_TYPE_P(op2) == IS_STRING)) {
			// 比较两个字串相等
			bool result = zend_fast_equal_strings(Z_STR_P(op1), Z_STR_P(op2));
			// 如果 op1 类型 是变量或临时变量
			if (OP1_TYPE & (IS_TMP_VAR|IS_VAR)) {
				// 通过指针销毁 zval（字串）
				zval_ptr_dtor_str(op1);
			}
			// 如果 op2 类型 是变量或临时变量
			if (OP2_TYPE & (IS_TMP_VAR|IS_VAR)) {
				// 通过指针销毁 zval（字串）
				zval_ptr_dtor_str(op2);
			}
			if (result) {
				// 跳转到指定标签 is_equal_true
				ZEND_VM_C_GOTO(is_equal_true);
			// 否则
			} else {
				// 跳转到指定标签 is_equal_false
				ZEND_VM_C_GOTO(is_equal_false);
			}
		}
	}
	// 计算相等, 检验传入的变量是否有效，进行 比较 运算，然后销毁运算对象op1,op2 
	// 调用方法 zend_is_equal_helper
	ZEND_VM_DISPATCH_TO_HELPER(zend_is_equal_helper, op_1, op1, op_2, op2);
}

// ing3, 计算不相等, 检验传入的变量是否有效，进行 比较 运算，然后销毁运算对象op1,op2 
ZEND_VM_HELPER(zend_is_not_equal_helper, ANY, ANY, zval *op_1, zval *op_2)
{
	int ret;
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// windows: 无操作
	SAVE_OPLINE();
	// 如果 变量类型为未定义
	if (UNEXPECTED(Z_TYPE_INFO_P(op_1) == IS_UNDEF)) {
		// 报错：p1的变量（变量名）未定义, 并返未初始化zval
		op_1 = ZVAL_UNDEFINED_OP1();
	}
	// 如果 变量类型为 未定义
	if (UNEXPECTED(Z_TYPE_INFO_P(op_2) == IS_UNDEF)) {
		// 报错：p2的变量（变量名）未定义, 并返未初始化zval
		op_2 = ZVAL_UNDEFINED_OP2();
	}
	// 进行比较运算
	ret = zend_compare(op_1, op_2);
	// 如果 op1 类型 是变量或临时变量
	if (OP1_TYPE & (IS_TMP_VAR|IS_VAR)) {
		// 销毁没有引用次数的对象,并没有销毁 zval本身，不放入gc回收周期。
		zval_ptr_dtor_nogc(op_1);
	}
	// 如果 op2 类型 是变量或临时变量
	if (OP2_TYPE & (IS_TMP_VAR|IS_VAR)) {
		// 销毁没有引用次数的对象,并没有销毁 zval本身，不放入gc回收周期。
		zval_ptr_dtor_nogc(op_2);
	}
	// 根据当前操作码结果选择下一个操作码，p1:传入值，主要用来做判断。p2:是否检查异常
	// 如果比较结果 不相等 返回true ,否则 false
	ZEND_VM_SMART_BRANCH(ret != 0, 1);
}

// ing3, 比较不相等，操作码
ZEND_VM_COLD_CONSTCONST_HANDLER(19, ZEND_IS_NOT_EQUAL, CONST|TMPVAR|CV, CONST|TMPVAR|CV, SPEC(SMART_BRANCH,COMMUTATIVE))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2;
	double d1, d2;
	// 获取zval指针, UNUSED 返回null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 获取zval指针, UNUSED 返回null
	op2 = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// op1或op2有一个不是ANY
	if (ZEND_VM_SPEC && OP1_TYPE == IS_CONST && OP2_TYPE == IS_CONST) {
		/* pass */
	// 如果 op1 类型是 整数
	} else if (EXPECTED(Z_TYPE_P(op1) == IS_LONG)) {
		// 如果 op2 类型是 整数
		if (EXPECTED(Z_TYPE_P(op2) == IS_LONG)) {
			// 如果两修整数不相等
			if (EXPECTED(Z_LVAL_P(op1) != Z_LVAL_P(op2))) {
// 创建跳转标签 is_not_equal_true
ZEND_VM_C_LABEL(is_not_equal_true):
				// 根据当前操作码结果选择下一个操作码。如果不是跳转类型，结果赋值为true。
				ZEND_VM_SMART_BRANCH_TRUE();
			// 否则
			} else {
// 创建跳转标签 is_not_equal_false
ZEND_VM_C_LABEL(is_not_equal_false):
				// 根据当前操作码结果选择下一个操作码。如果不是跳转类型，结果赋值为false。
				ZEND_VM_SMART_BRANCH_FALSE();
			}
		// 否则 如果 op2 类型是 小数
		} else if (EXPECTED(Z_TYPE_P(op2) == IS_DOUBLE)) {
			// op1转成小数
			d1 = (double)Z_LVAL_P(op1);
			// 取出小数
			d2 = Z_DVAL_P(op2);
			// 跳转到指定标签 is_not_equal_double
			ZEND_VM_C_GOTO(is_not_equal_double);
		}
	// 如果 op1 类型是 小数
	} else if (EXPECTED(Z_TYPE_P(op1) == IS_DOUBLE)) {
		// 如果 op2 类型是 小数
		if (EXPECTED(Z_TYPE_P(op2) == IS_DOUBLE)) {
			// 取出小数
			d1 = Z_DVAL_P(op1);
			// 取出小数
			d2 = Z_DVAL_P(op2);
// 创建跳转标签 is_not_equal_double
ZEND_VM_C_LABEL(is_not_equal_double):
			if (d1 != d2) {
				// 跳转到指定标签 is_not_equal_true
				ZEND_VM_C_GOTO(is_not_equal_true);
			// 否则
			} else {
				// 跳转到指定标签 is_not_equal_false
				ZEND_VM_C_GOTO(is_not_equal_false);
			}
		// 否则 如果 op2 类型是 整数
		} else if (EXPECTED(Z_TYPE_P(op2) == IS_LONG)) {
			// 取出小数
			d1 = Z_DVAL_P(op1);
			// op2转成小数
			d2 = (double)Z_LVAL_P(op2);
			// 跳转到指定标签 is_not_equal_double
			ZEND_VM_C_GOTO(is_not_equal_double);
		}
	// 如果 op1 类型是 
	} else if (EXPECTED(Z_TYPE_P(op1) == IS_STRING)) {
		// 如果 op2 类型是 字串
		if (EXPECTED(Z_TYPE_P(op2) == IS_STRING)) {
			// 比较两个字串是否相等
			bool result = zend_fast_equal_strings(Z_STR_P(op1), Z_STR_P(op2));
			// 如果 op1 类型 是变量或临时变量
			if (OP1_TYPE & (IS_TMP_VAR|IS_VAR)) {
				// 通过指针销毁 zval（字串）
				zval_ptr_dtor_str(op1);
			}
			// 如果 op2 类型 是变量或临时变量
			if (OP2_TYPE & (IS_TMP_VAR|IS_VAR)) {
				// 通过指针销毁 zval（字串）
				zval_ptr_dtor_str(op2);
			}
			// 如果两个字串相等
			if (!result) {
				// 跳转到指定标签 is_not_equal_true
				ZEND_VM_C_GOTO(is_not_equal_true);
			// 否则
			} else {
				// 跳转到指定标签 is_not_equal_false
				ZEND_VM_C_GOTO(is_not_equal_false);
			}
		}
	}
	// 计算不相等, 检验传入的变量是否有效，进行 比较 运算，然后销毁运算对象op1,op2 
	// 调用方法 zend_is_not_equal_helper
	ZEND_VM_DISPATCH_TO_HELPER(zend_is_not_equal_helper, op_1, op1, op_2, op2);
}

// ing3, 计算小于, 检验传入的变量是否有效，进行 比较 运算，然后销毁运算对象op1,op2 
ZEND_VM_HELPER(zend_is_smaller_helper, ANY, ANY, zval *op_1, zval *op_2)
{
	int ret;
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// windows: 无操作
	SAVE_OPLINE();
	// 如果 变量类型为未定义
	if (UNEXPECTED(Z_TYPE_INFO_P(op_1) == IS_UNDEF)) {
		// 报错：p1的变量（变量名）未定义, 并返未初始化zval
		op_1 = ZVAL_UNDEFINED_OP1();
	}
	// 如果 变量类型为 未定义
	if (UNEXPECTED(Z_TYPE_INFO_P(op_2) == IS_UNDEF)) {
		// 报错：p2的变量（变量名）未定义, 并返未初始化zval
		op_2 = ZVAL_UNDEFINED_OP2();
	}
	// 进行比较运算
	ret = zend_compare(op_1, op_2);
	// 如果 op1 类型 是变量或临时变量
	if (OP1_TYPE & (IS_TMP_VAR|IS_VAR)) {
		// 销毁没有引用次数的对象,并没有销毁 zval本身，不放入gc回收周期。
		zval_ptr_dtor_nogc(op_1);
	}
	// 如果 op2 类型 是变量或临时变量
	if (OP2_TYPE & (IS_TMP_VAR|IS_VAR)) {
		// 销毁没有引用次数的对象,并没有销毁 zval本身，不放入gc回收周期。
		zval_ptr_dtor_nogc(op_2);
	}
	// 根据当前操作码结果选择下一个操作码，p1:传入值，主要用来做判断。p2:是否检查异常
	// 如果比较结果 小于0 返回true ,否则 false
	ZEND_VM_SMART_BRANCH(ret < 0, 1);
}

// ing3, 小于，操作码, 1/3
ZEND_VM_HOT_NOCONSTCONST_HANDLER(20, ZEND_IS_SMALLER, CONST|TMPVARCV, CONST|TMPVARCV, SPEC(SMART_BRANCH))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2;
	double d1, d2;
	// 获取zval指针, UNUSED 返回null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 获取zval指针, UNUSED 返回null
	op2 = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// op1或op2有一个不是ANY 并且两个都是常量
	if (ZEND_VM_SPEC && OP1_TYPE == IS_CONST && OP2_TYPE == IS_CONST) {
		/* pass */
	// 如果op1是整数
	} else if (EXPECTED(Z_TYPE_INFO_P(op1) == IS_LONG)) {
		// 如果op2是整数
		if (EXPECTED(Z_TYPE_INFO_P(op2) == IS_LONG)) {
			// 整数直接比较大小
			if (EXPECTED(Z_LVAL_P(op1) < Z_LVAL_P(op2))) {
// 创建跳转标签 is_smaller_true
ZEND_VM_C_LABEL(is_smaller_true):
				// 根据当前操作码结果选择下一个操作码。如果不是跳转类型，结果赋值为true。
				ZEND_VM_SMART_BRANCH_TRUE();
			// 否则
			} else {
// 创建跳转标签 is_smaller_false
ZEND_VM_C_LABEL(is_smaller_false):
				// 根据当前操作码结果选择下一个操作码。如果不是跳转类型，结果赋值为false。
				ZEND_VM_SMART_BRANCH_FALSE();
			}
		// 如果op2是小数
		} else if (EXPECTED(Z_TYPE_INFO_P(op2) == IS_DOUBLE)) {
			// op1转成小数
			d1 = (double)Z_LVAL_P(op1);
			// 取出小数
			d2 = Z_DVAL_P(op2);
			// 跳转到指定标签 is_smaller_double
			ZEND_VM_C_GOTO(is_smaller_double);
		}
	// 如果 op1是小数
	} else if (EXPECTED(Z_TYPE_INFO_P(op1) == IS_DOUBLE)) {
		// 如果 op2是小数
		if (EXPECTED(Z_TYPE_INFO_P(op2) == IS_DOUBLE)) {
			// 取出小数
			d1 = Z_DVAL_P(op1);
			// 取出小数
			d2 = Z_DVAL_P(op2);
// 创建跳转标签 is_smaller_double
ZEND_VM_C_LABEL(is_smaller_double):
			// 如果op2大
			if (d1 < d2) {
				// 跳转到指定标签 is_smaller_true
				ZEND_VM_C_GOTO(is_smaller_true);
			// 否则
			} else {
				// 跳转到指定标签 is_smaller_false
				ZEND_VM_C_GOTO(is_smaller_false);
			}
		// 如果op2是整数 
		} else if (EXPECTED(Z_TYPE_INFO_P(op2) == IS_LONG)) {
			// op1取出小数
			d1 = Z_DVAL_P(op1);
			// op2转成小数
			d2 = (double)Z_LVAL_P(op2);
			// 跳转到指定标签 is_smaller_double
			ZEND_VM_C_GOTO(is_smaller_double);
		}
	}
	// 调用方法 zend_is_smaller_helper
	ZEND_VM_DISPATCH_TO_HELPER(zend_is_smaller_helper, op_1, op1, op_2, op2);
}

// ing3, 计算小于等于, 检验传入的变量是否有效，进行 比较 运算，然后销毁运算对象op1,op2 
ZEND_VM_HELPER(zend_is_smaller_or_equal_helper, ANY, ANY, zval *op_1, zval *op_2)
{
	int ret;
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// windows: 无操作
	SAVE_OPLINE();
	// 如果 变量类型为未定义
	if (UNEXPECTED(Z_TYPE_INFO_P(op_1) == IS_UNDEF)) {
		// 报错：p1的变量（变量名）未定义, 并返未初始化zval
		op_1 = ZVAL_UNDEFINED_OP1();
	}
	// 如果 变量类型为未定义
	if (UNEXPECTED(Z_TYPE_INFO_P(op_2) == IS_UNDEF)) {
		// 报错：p2的变量（变量名）未定义, 并返未初始化zval
		op_2 = ZVAL_UNDEFINED_OP2();
	}
	// 进行比较运算
	ret = zend_compare(op_1, op_2);
	// 如果 op1 类型 是变量或临时变量
	if (OP1_TYPE & (IS_TMP_VAR|IS_VAR)) {
		// 销毁没有引用次数的对象,并没有销毁 zval本身，不放入gc回收周期。
		zval_ptr_dtor_nogc(op_1);
	}
	// 如果 op2 类型 是变量或临时变量
	if (OP2_TYPE & (IS_TMP_VAR|IS_VAR)) {
		// 销毁没有引用次数的对象,并没有销毁 zval本身，不放入gc回收周期。
		zval_ptr_dtor_nogc(op_2);
	}
	// 根据当前操作码结果选择下一个操作码，p1:传入值，主要用来做判断。p2:是否检查异常
	// 如果比较结果 小于等于0 返回true ,否则 false
	ZEND_VM_SMART_BRANCH(ret <= 0, 1);
}

// ing3, 小于等于，处理器 , 1/3
ZEND_VM_HOT_NOCONSTCONST_HANDLER(21, ZEND_IS_SMALLER_OR_EQUAL, CONST|TMPVARCV, CONST|TMPVARCV, SPEC(SMART_BRANCH))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2;
	double d1, d2;
	// 获取zval指针, UNUSED 返回null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 获取zval指针, UNUSED 返回null
	op2 = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// op1或op2有一个不是ANY
	if (ZEND_VM_SPEC && OP1_TYPE == IS_CONST && OP2_TYPE == IS_CONST) {
		/* pass */
	// 如果 op1 变量类型为 整数
	} else if (EXPECTED(Z_TYPE_INFO_P(op1) == IS_LONG)) {
		// 如果 op2 变量类型为 整数
		if (EXPECTED(Z_TYPE_INFO_P(op2) == IS_LONG)) {
			// 两个整数直接比较
			if (EXPECTED(Z_LVAL_P(op1) <= Z_LVAL_P(op2))) {
// 创建跳转标签 is_smaller_or_equal_true
ZEND_VM_C_LABEL(is_smaller_or_equal_true):
				// 根据当前操作码结果选择下一个操作码。如果不是跳转类型，结果赋值为true。
				ZEND_VM_SMART_BRANCH_TRUE();
				// 结果为true
				ZVAL_TRUE(EX_VAR(opline->result.var));
				// opline + 1，到目标操作码并 return
				ZEND_VM_NEXT_OPCODE();
			// 否则
			} else {
// 创建跳转标签 is_smaller_or_equal_false
ZEND_VM_C_LABEL(is_smaller_or_equal_false):
				// 根据当前操作码结果选择下一个操作码。如果不是跳转类型，结果赋值为false。
				ZEND_VM_SMART_BRANCH_FALSE();
				// 结果为false
				ZVAL_FALSE(EX_VAR(opline->result.var));
				// opline + 1，到目标操作码并 return
				ZEND_VM_NEXT_OPCODE();
			}
		// 否则。 如果 变量类型为 小数
		} else if (EXPECTED(Z_TYPE_INFO_P(op2) == IS_DOUBLE)) {
			d1 = (double)Z_LVAL_P(op1);
			// 取出小数
			d2 = Z_DVAL_P(op2);
			// 跳转到指定标签 is_smaller_or_equal_double
			ZEND_VM_C_GOTO(is_smaller_or_equal_double);
		}
	// 否则。 如果 变量类型为 小数
	} else if (EXPECTED(Z_TYPE_INFO_P(op1) == IS_DOUBLE)) {
		// 如果 变量类型为 小数
		if (EXPECTED(Z_TYPE_INFO_P(op2) == IS_DOUBLE)) {
			// 取出小数
			d1 = Z_DVAL_P(op1);
			// 取出小数
			d2 = Z_DVAL_P(op2);
// 创建跳转标签 is_smaller_or_equal_double
ZEND_VM_C_LABEL(is_smaller_or_equal_double):
			// 小数直接比大小，d1小
			if (d1 <= d2) {
				// 跳转到指定标签 is_smaller_or_equal_true
				ZEND_VM_C_GOTO(is_smaller_or_equal_true);
			// 否则
			} else {
				// 跳转到指定标签 is_smaller_or_equal_false
				ZEND_VM_C_GOTO(is_smaller_or_equal_false);
			}
		// 如果 变量类型为 整数
		} else if (EXPECTED(Z_TYPE_INFO_P(op2) == IS_LONG)) {
			// 取出小数
			d1 = Z_DVAL_P(op1);
			// 整数转成小数
			d2 = (double)Z_LVAL_P(op2);
			// 跳转到指定标签 is_smaller_or_equal_double
			ZEND_VM_C_GOTO(is_smaller_or_equal_double);
		}
	}
	// 调用方法 zend_is_smaller_or_equal_helper
	ZEND_VM_DISPATCH_TO_HELPER(zend_is_smaller_or_equal_helper, op_1, op1, op_2, op2);
}

// ing3, 新的比大小运算符 <=> ，大于返回1，等于返回0，小于返回-1
ZEND_VM_COLD_CONSTCONST_HANDLER(170, ZEND_SPACESHIP, CONST|TMPVAR|CV, CONST|TMPVAR|CV)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2;

	// windows: 无操作
	SAVE_OPLINE();
	// 获取zval指针，UNUSED 类型返回null
	op1 = GET_OP1_ZVAL_PTR(BP_VAR_R);
	// 获取zval指针，UNUSED 类型返回null
	op2 = GET_OP2_ZVAL_PTR(BP_VAR_R);
	// 比较两个变量。p1:返回结果, p2:变量1, p3:变量2
	compare_function(EX_VAR(opline->result.var), op1, op2);
	// 释放操作对象的附加变量
	FREE_OP1();
	// 释放操作对象的附加变量
	FREE_OP2();
	// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, 二进制 或 运算, 检验传入的变量是否有效，进行 或 运算，然后销毁运算对象op1,op2 
ZEND_VM_HELPER(zend_bw_or_helper, ANY, ANY, zval *op_1, zval *op_2)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// windows: 无操作
	SAVE_OPLINE();
	// 如果 变量类型为 未定义
	if (UNEXPECTED(Z_TYPE_INFO_P(op_1) == IS_UNDEF)) {
		// 报错：p1的变量（变量名）未定义, 并返未初始化zval
		op_1 = ZVAL_UNDEFINED_OP1();
	}
	// 如果 变量类型为 未定义
	if (UNEXPECTED(Z_TYPE_INFO_P(op_2) == IS_UNDEF)) {
		// 报错：p2的变量（变量名）未定义, 并返未初始化zval
		op_2 = ZVAL_UNDEFINED_OP2();
	}
	// 整数或字串（1个或多个字符）的 按位 或 运算
	bitwise_or_function(EX_VAR(opline->result.var), op_1, op_2);
	// 如果 op1 类型 是变量或临时变量
	if (OP1_TYPE & (IS_TMP_VAR|IS_VAR)) {
		// 销毁没有引用次数的对象,并没有销毁 zval本身，不放入gc回收周期。
		zval_ptr_dtor_nogc(op_1);
	}
	// 如果 op2 类型 是变量或临时变量
	if (OP2_TYPE & (IS_TMP_VAR|IS_VAR)) {
		// 销毁没有引用次数的对象,并没有销毁 zval本身，不放入gc回收周期。
		zval_ptr_dtor_nogc(op_2);
	}
	// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// 1/1 ,ZEND_BW_OR
// ing3, 二进制 或 运算
ZEND_VM_HOT_NOCONSTCONST_HANDLER(9, ZEND_BW_OR, CONST|TMPVARCV, CONST|TMPVARCV, SPEC(COMMUTATIVE))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2;
	// 获取zval指针, UNUSED 返回null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 获取zval指针, UNUSED 返回null
	op2 = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// op1或op2有一个不是ANY
	if (ZEND_VM_SPEC && OP1_TYPE == IS_CONST && OP2_TYPE == IS_CONST) {
		/* pass */
		
	// 否则， 如果 两个 变量类型 都为 整数
	} else if (EXPECTED(Z_TYPE_INFO_P(op1) == IS_LONG)
			&& EXPECTED(Z_TYPE_INFO_P(op2) == IS_LONG)) {
		// 直接做 或运算 
		ZVAL_LONG(EX_VAR(opline->result.var), Z_LVAL_P(op1) | Z_LVAL_P(op2));
		// opline + 1，到目标操作码并 return
		ZEND_VM_NEXT_OPCODE();
	}

	// 二进制 或 运算, 检验传入的变量是否有效，进行 或 运算，然后销毁运算对象op1,op2 
	// 调用方法 zend_bw_or_helper
	ZEND_VM_DISPATCH_TO_HELPER(zend_bw_or_helper, op_1, op1, op_2, op2);
}

// ing3, 二进制 与 运算, 检验传入的变量是否有效，进行 与 运算，然后销毁运算对象op1,op2 
ZEND_VM_HELPER(zend_bw_and_helper, ANY, ANY, zval *op_1, zval *op_2)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// windows: 无操作
	SAVE_OPLINE();
	// 如果 变量类型为 未定义
	if (UNEXPECTED(Z_TYPE_INFO_P(op_1) == IS_UNDEF)) {
		// 报错：p1的变量（变量名）未定义, 并返未初始化zval
		op_1 = ZVAL_UNDEFINED_OP1();
	}
	// 如果 变量类型为 未定义
	if (UNEXPECTED(Z_TYPE_INFO_P(op_2) == IS_UNDEF)) {
		// 报错：p2的变量（变量名）未定义, 并返未初始化zval
		op_2 = ZVAL_UNDEFINED_OP2();
	}
	// 整数或字串（1个或多个字符）的 按位与 运算
	bitwise_and_function(EX_VAR(opline->result.var), op_1, op_2);
	// 如果 op1 类型 是变量或临时变量
	if (OP1_TYPE & (IS_TMP_VAR|IS_VAR)) {
		// 销毁没有引用次数的对象,并没有销毁 zval本身，不放入gc回收周期。
		zval_ptr_dtor_nogc(op_1);
	}
	// 如果 op2 类型 是变量或临时变量
	if (OP2_TYPE & (IS_TMP_VAR|IS_VAR)) {
		// 销毁没有引用次数的对象,并没有销毁 zval本身，不放入gc回收周期。
		zval_ptr_dtor_nogc(op_2);
	}
	// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}
// 1/1, ZEND_BW_AND
// ing3, 二进制 与 运算
ZEND_VM_HOT_NOCONSTCONST_HANDLER(10, ZEND_BW_AND, CONST|TMPVARCV, CONST|TMPVARCV, SPEC(COMMUTATIVE))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2;
	// 获取zval指针, UNUSED 返回null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 获取zval指针, UNUSED 返回null
	op2 = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// op1或op2有一个不是ANY
	if (ZEND_VM_SPEC && OP1_TYPE == IS_CONST && OP2_TYPE == IS_CONST) {
		/* pass */
	// 否则 如果 两个变量类型都为 整数
	} else if (EXPECTED(Z_TYPE_INFO_P(op1) == IS_LONG)
			&& EXPECTED(Z_TYPE_INFO_P(op2) == IS_LONG)) {
		// 两个整数 直接做 并 运算
		ZVAL_LONG(EX_VAR(opline->result.var), Z_LVAL_P(op1) & Z_LVAL_P(op2));
		// opline + 1，到目标操作码并 return
		ZEND_VM_NEXT_OPCODE();
	}

	// 二进制 与 运算, 检验传入的变量是否有效，进行 与 运算，然后销毁运算对象op1,op2 
	// 调用方法 zend_bw_and_helper
	ZEND_VM_DISPATCH_TO_HELPER(zend_bw_and_helper, op_1, op1, op_2, op2);
}

// ing3, 二进制 抑或 运算, 检验传入的变量是否有效，进行 抑或 运算，然后销毁运算对象op1,op2 
ZEND_VM_HELPER(zend_bw_xor_helper, ANY, ANY, zval *op_1, zval *op_2)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// windows: 无操作
	SAVE_OPLINE();
	// 如果 变量类型为未定义
	if (UNEXPECTED(Z_TYPE_INFO_P(op_1) == IS_UNDEF)) {
		// 报错：p1的变量（变量名）未定义, 并返未初始化zval
		op_1 = ZVAL_UNDEFINED_OP1();
	}
	// 如果 变量类型为 未定义
	if (UNEXPECTED(Z_TYPE_INFO_P(op_2) == IS_UNDEF)) {
		// 报错：p2的变量（变量名）未定义, 并返未初始化zval
		op_2 = ZVAL_UNDEFINED_OP2();
	}
	// 整数或字串（1个或多个字符）的 按位 抑或 运算
	bitwise_xor_function(EX_VAR(opline->result.var), op_1, op_2);
	// 如果 op1 类型 是变量或临时变量
	if (OP1_TYPE & (IS_TMP_VAR|IS_VAR)) {
		// 销毁没有引用次数的对象,并没有销毁 zval本身，不放入gc回收周期。
		zval_ptr_dtor_nogc(op_1);
	}
	// 如果 op2 类型 是变量或临时变量
	if (OP2_TYPE & (IS_TMP_VAR|IS_VAR)) {
		// 销毁没有引用次数的对象,并没有销毁 zval本身，不放入gc回收周期。
		zval_ptr_dtor_nogc(op_2);
	}
	// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// 1/1, ZEND_BW_XOR
// ing3, 二进制 抑或 运算
ZEND_VM_HOT_NOCONSTCONST_HANDLER(11, ZEND_BW_XOR, CONST|TMPVARCV, CONST|TMPVARCV, SPEC(COMMUTATIVE))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2;
	// 获取zval指针, UNUSED 返回null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 获取zval指针, UNUSED 返回null
	op2 = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// op1或op2有一个不是ANY
	if (ZEND_VM_SPEC && OP1_TYPE == IS_CONST && OP2_TYPE == IS_CONST) {
		/* pass */
	// 否则 如果 两个变量类型都为 整数
	} else if (EXPECTED(Z_TYPE_INFO_P(op1) == IS_LONG)
			&& EXPECTED(Z_TYPE_INFO_P(op2) == IS_LONG)) {
		// 两个整数直接进行抑或运算
		ZVAL_LONG(EX_VAR(opline->result.var), Z_LVAL_P(op1) ^ Z_LVAL_P(op2));
		// opline + 1，到目标操作码并 return
		ZEND_VM_NEXT_OPCODE();
	}

	// 二进制 抑或 运算, 检验传入的变量是否有效，进行 抑或 运算，然后销毁运算对象op1,op2 
	// 调用方法 zend_bw_xor_helper
	ZEND_VM_DISPATCH_TO_HELPER(zend_bw_xor_helper, op_1, op1, op_2, op2);
}

// ing3, 布尔抑或（两个值不同时返回 true）
ZEND_VM_COLD_CONSTCONST_HANDLER(15, ZEND_BOOL_XOR, CONST|TMPVAR|CV, CONST|TMPVAR|CV, SPEC(COMMUTATIVE))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2;

	// windows: 无操作
	SAVE_OPLINE();
	// 获取zval指针，UNUSED 类型返回null
	op1 = GET_OP1_ZVAL_PTR(BP_VAR_R);
	// 获取zval指针，UNUSED 类型返回null
	op2 = GET_OP2_ZVAL_PTR(BP_VAR_R);
	// 布尔抑或（两个值不同时返回 true）
	boolean_xor_function(EX_VAR(opline->result.var), op1, op2);
	// 释放操作对象的附加变量
	FREE_OP1();
	// 释放操作对象的附加变量
	FREE_OP2();
	// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, 二进制 取反 运算, 检验传入的变量是否有效，进行 取反 运算，然后销毁运算对象op1
ZEND_VM_HELPER(zend_bw_not_helper, ANY, ANY, zval *op_1)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// windows: 无操作
	SAVE_OPLINE();
	// 如果 op1 类型是 未定义
	if (UNEXPECTED(Z_TYPE_P(op_1) == IS_UNDEF)) {
		// 报错：p1的变量（变量名）未定义, 并返未初始化zval
		op_1 = ZVAL_UNDEFINED_OP1();
	}
	// ~ 操作符，一元操作符，二进制取反
	bitwise_not_function(EX_VAR(opline->result.var), op_1);
	// 释放操作对象的附加变量
	FREE_OP1();
	// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, 二进制 取反 运算
ZEND_VM_HOT_NOCONST_HANDLER(13, ZEND_BW_NOT, CONST|TMPVARCV, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1;
	// 获取zval指针, UNUSED 返回null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 如果 变量类型为 整数
	if (EXPECTED(Z_TYPE_INFO_P(op1) == IS_LONG)) {
		// 整数直接 用【反】运算
		ZVAL_LONG(EX_VAR(opline->result.var), ~Z_LVAL_P(op1));
		// opline + 1，到目标操作码并 return
		ZEND_VM_NEXT_OPCODE();
	}

	// 二进制 取反 运算, 检验传入的变量是否有效，进行 取反 运算，然后销毁运算对象op1
	// 调用方法 zend_bw_xor_helper
	ZEND_VM_DISPATCH_TO_HELPER(zend_bw_not_helper, op_1, op1);
}

// ing3, bool取反
ZEND_VM_COLD_CONST_HANDLER(14, ZEND_BOOL_NOT, CONST|TMPVAR|CV, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	// 
	zval *val;
	// 获取zval指针, UNUSED 返回null
	val = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 如果 变量为 true
	if (Z_TYPE_INFO_P(val) == IS_TRUE) {
		// 结果为 false
		ZVAL_FALSE(EX_VAR(opline->result.var));
	// 否则 如果 变量类型为 false,null,未定义
	} else if (EXPECTED(Z_TYPE_INFO_P(val) <= IS_TRUE)) {
		/* The result and op1 can be the same cv zval */
		// 获取变量类型
		const uint32_t orig_val_type = Z_TYPE_INFO_P(val);
		// 结果为 true
		ZVAL_TRUE(EX_VAR(opline->result.var));
		// 如果 op1 是 编译变量 并且 原值类型是 未定义
		if (OP1_TYPE == IS_CV && UNEXPECTED(orig_val_type == IS_UNDEF)) {
			// windows: 无操作
			SAVE_OPLINE();
			// 报错：p1的变量（变量名）未定义, 并返未初始化zval
			ZVAL_UNDEFINED_OP1();
			// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
			ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
		}
	// 否则
	} else {
		// windows: 无操作
		SAVE_OPLINE();
		// 把值转成bool型后 取反。放到结果变量中
		ZVAL_BOOL(EX_VAR(opline->result.var), !i_zend_is_true(val));
		// 释放操作对象的附加变量
		FREE_OP1();
		// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
		ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
	}
	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, 报错，不可以在非对象上下文中使用 $this
ZEND_VM_COLD_HELPER(zend_this_not_in_object_context_helper, ANY, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// windows: 无操作
	SAVE_OPLINE();
	zend_throw_error(NULL, "Using $this when not in object context");
	// 如果 opline 结果类型是 变量或临时变量，把结果 zval 标记为未定义
	UNDEF_RESULT();
	// 主要是 return
	HANDLE_EXCEPTION();
}

// ing3, 报错，没用了未定义的函数 
ZEND_VM_COLD_HELPER(zend_undefined_function_helper, ANY, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *function_name;

	// windows: 无操作
	SAVE_OPLINE();
	// 访问 (p2).zv。p1:opline,p2:node
	function_name = RT_CONSTANT(opline, opline->op2);
	zend_throw_error(NULL, "Call to undefined function %s()", Z_STRVAL_P(function_name));
	// 主要是 return
	HANDLE_EXCEPTION();
}
// ing3, 对象属性赋值操作 1/1。3（THIS不算）*3=9个分支
ZEND_VM_HANDLER(28, ZEND_ASSIGN_OBJ_OP, VAR|UNUSED|THIS|CV, CONST|TMPVAR|CV, OP)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *object;
	zval *property;
	zval *value;
	zval *zptr;
	void **cache_slot;
	zend_property_info *prop_info;
	zend_object *zobj;
	zend_string *name, *tmp_name;

	// windows: 无操作
	SAVE_OPLINE();
	// 获取zval指针, TMP/CONST返回null, UNUSED 返回 $this，不支持 TMPVAR/TMPVARCV。CV 直接返回。
	object = GET_OP1_OBJ_ZVAL_PTR_PTR_UNDEF(BP_VAR_RW);
	// 获取zval指针，UNUSED 类型返回null
	property = GET_OP2_ZVAL_PTR(BP_VAR_R);

	// 为了break
	do {
		// 取得下一个操作码的 zval。UNUSED返回null, 不支持 TMPVARCV。
		value = GET_OP_DATA_ZVAL_PTR(BP_VAR_R);

		// 如果 op1是未使用 并且 op1 类型 不是 对象
		if (OP1_TYPE != IS_UNUSED && UNEXPECTED(Z_TYPE_P(object) != IS_OBJECT)) {
			// 如果 op1是引用类型 并且 目标是对象
			if (Z_ISREF_P(object) && Z_TYPE_P(Z_REFVAL_P(object)) == IS_OBJECT) {
				// 解引用
				object = Z_REFVAL_P(object);
				// 跳转到指定标签 assign_op_object
				ZEND_VM_C_GOTO(assign_op_object);
			}
			// 如果 op1 是 编译变量 并且 对象类型为 未定义
			if (OP1_TYPE == IS_CV
			 && UNEXPECTED(Z_TYPE_P(object) == IS_UNDEF)) {
				// 报错：p1的变量（变量名）未定义, 并返未初始化zval
				ZVAL_UNDEFINED_OP1();
			}
			// 抛出【对象无效】异常
			zend_throw_non_object_error(object, property OPLINE_CC EXECUTE_DATA_CC);
			// 跳出
			break;
		}

// 创建跳转标签 assign_op_object
ZEND_VM_C_LABEL(assign_op_object):
		// 这里已经确定处理的是对象了
		/* here we are sure we are dealing with an object */
		// 取出对象
		zobj = Z_OBJ_P(object);
		// 如果op2是常量
		if (OP2_TYPE == IS_CONST) {
			// 取出属性名
			name = Z_STR_P(property);
		// 否则
		} else {
			// 获取临时字串（不抛错）返回
			name = zval_try_get_tmp_string(property, &tmp_name);
			// 如果没有属性名
			if (UNEXPECTED(!name)) {
				// 如果 opline 结果类型是 变量或临时变量，把结果 zval 标记为未定义
				UNDEF_RESULT();
				// 跳出
				break;
			}
		}
		// 如果是常量，用下一个操作码的扩展值作为位置，否则是null
		cache_slot = (OP2_TYPE == IS_CONST) ? CACHE_ADDR((opline+1)->extended_value) : NULL;
		// 取得属性指针的指针，p1:对象，p2:属性名，p3:操作类型，p4:缓存位置
		// zend_std_get_property_ptr_ptr。 如果找到属性
		if (EXPECTED((zptr = zobj->handlers->get_property_ptr_ptr(zobj, name, BP_VAR_RW, cache_slot)) != NULL)) {
			// 如果返回的是错误
			if (UNEXPECTED(Z_ISERROR_P(zptr))) {
				// 如果 操作码的运算结果有效(不是IS_UNUSED)
				if (UNEXPECTED(RETURN_VALUE_USED(opline))) {
					// 结果 为 null
					ZVAL_NULL(EX_VAR(opline->result.var));
				}
			// 否则
			} else {
				zval *orig_zptr = zptr;
				zend_reference *ref;

				// 为了break;
				do {
					// 如果是引用类型
					if (UNEXPECTED(Z_ISREF_P(zptr))) {
						// 取得引用实例
						ref = Z_REF_P(zptr);
						// 解引用
						zptr = Z_REFVAL_P(zptr);
						// 如果引用实例 有 类型
						if (UNEXPECTED(ZEND_REF_HAS_TYPE_SOURCES(ref))) {
							// 根据操作码类型 对引用目标 进行 二进制 赋值操作，p1:引用实例，p2:第二个运算值
							// 加/减/乘/除/左右位移/连接/或/与/抑或/幂 运算
							zend_binary_assign_op_typed_ref(ref, value OPLINE_CC EXECUTE_DATA_CC);
							// 跳出
							break;
						}
					}

					// 如果op2是常量
					if (OP2_TYPE == IS_CONST) {
						// 从缓存获取
						// 返回数组的第一个元素
						prop_info = (zend_property_info*)CACHED_PTR_EX(cache_slot + 2);
					// 否则
					} else {
						// 验证 属性有效（在属性表中） 并 获取它的属性信息，p1:对象，p2:属性zval
						prop_info = zend_object_fetch_property_type_info(Z_OBJ_P(object), orig_zptr);
					}
					// 如果有属性信息
					if (UNEXPECTED(prop_info)) {
						// 对有类型的属性进行特殊处理
						/* special case for typed properties */
						// 根据属性信息的类型 对运算对象 进行 二进制 赋值操作，p1:属性信息，p2:第一个运算值，p3:第二个运算值
						zend_binary_assign_op_typed_prop(prop_info, zptr, value OPLINE_CC EXECUTE_DATA_CC);
					// 否则
					} else {
						// 调用操作码（在opline参数里）对应的二进制处理方法，p1:返回值，p2:op1，p3:op2
						zend_binary_op(zptr, zptr, value OPLINE_CC);
					}
				} while (0);

				// 如果 操作码的运算结果有效(不是IS_UNUSED)
				if (UNEXPECTED(RETURN_VALUE_USED(opline))) {
					// 返回值可以作为结果 
					ZVAL_COPY(EX_VAR(opline->result.var), zptr);
				}
			}
		// 否则
		} else {
			// 读取属性值，进行运算，如果成功，把新值写回属性里。
			// p1:对象，p2:属性名，p3:缓存位置，p4:op2（op1是原属性值）
			zend_assign_op_overloaded_property(zobj, name, cache_slot, value OPLINE_CC EXECUTE_DATA_CC);
		}
		// 如果 OP2 不是常量
		if (OP2_TYPE != IS_CONST) {
			// 释放临时名称
			zend_tmp_string_release(tmp_name);
		}
	} while (0);

	// 释放下一个操作码的 op1。CONST/UNUSED/CV 免操作。不支持 TMPVARCV。
	FREE_OP_DATA();
	// 释放操作对象的附加变量
	FREE_OP2();
	// 释放操作对象的附加变量
	FREE_OP1();
	// 给对象属性赋值有两个操作码
	/* assign_obj has two opcodes! */
	// 偏移到目标操作码并 return ,p1:是否检查异常？使用 EX(opline)  ：使用 opline 。p2:偏移量
	ZEND_VM_NEXT_OPCODE_EX(1, 2);
}

// ing3, 读取静态属性值
/* No specialization for op_types (CONST|TMP|VAR|CV, UNUSED|CONST|TMPVAR) */
ZEND_VM_HANDLER(29, ZEND_ASSIGN_STATIC_PROP_OP, ANY, ANY, OP)
{
	// 这个助手 
	/* This helper actually never will receive IS_VAR as second op, and has the same handling for VAR and TMP in the first op, but for interoperability with the other binary_assign_op helpers, it is necessary to "include" it */

	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *prop, *value;
	zend_property_info *prop_info;
	zend_reference *ref;

	// windows: 无操作
	SAVE_OPLINE();

	// 通过缓存位置和类型，返回静态属性地址。p1:返回属性值，p2:返回属性信息，p3:缓存位置，p4:获取类型，p5:falgs
	if (UNEXPECTED(zend_fetch_static_property_address(&prop, &prop_info, (opline+1)->extended_value, BP_VAR_RW, 0 OPLINE_CC EXECUTE_DATA_CC) != SUCCESS)) {
		// 如果 opline 结果类型是 变量或临时变量，把结果 zval 标记为未定义
		UNDEF_RESULT();
		// 释放下一个操作码的 op1。CONST/UNUSED/CV 免操作。不支持 TMPVARCV。
		FREE_OP_DATA();
		// 主要是 return
		HANDLE_EXCEPTION();
	}

	// 取得下一个操作码的 zval。UNUSED返回null, 不支持 TMPVARCV。
	value = GET_OP_DATA_ZVAL_PTR(BP_VAR_R);

	// 为了break
	do {
		// 如果 属性值是引用类型
		if (UNEXPECTED(Z_ISREF_P(prop))) {
			// 取出引用实例
			ref = Z_REF_P(prop);
			// 解引用
			prop = Z_REFVAL_P(prop);
			// 如果引用实例 有 类型
			if (UNEXPECTED(ZEND_REF_HAS_TYPE_SOURCES(ref))) {
				// 根据操作码类型 对引用目标 进行 二进制 赋值操作，p1:引用实例，p2:第二个运算值
				// 加/减/乘/除/左右位移/连接/或/与/抑或/幂 运算
				zend_binary_assign_op_typed_ref(ref, value OPLINE_CC EXECUTE_DATA_CC);
				// 跳出
				break;
			}
		}
		// 如果类型存在
		if (UNEXPECTED(ZEND_TYPE_IS_SET(prop_info->type))) {
			// 对有类型的属性进行特殊处理
			/* special case for typed properties */
			// 根据属性信息的类型 对运算对象 进行 二进制 赋值操作，p1:属性信息，p2:第一个运算值，p3:第二个运算值
			zend_binary_assign_op_typed_prop(prop_info, prop, value OPLINE_CC EXECUTE_DATA_CC);
		// 否则
		} else {
			// 调用操作码（在 opline 参数里）对应的二进制处理方法，p1:返回值，p2:op1，p3:op2
			zend_binary_op(prop, prop, value OPLINE_CC);
		}
	} while (0);

	// 如果 操作码的运算结果有效(不是IS_UNUSED)
	if (UNEXPECTED(RETURN_VALUE_USED(opline))) {
		// 结果为属性值
		ZVAL_COPY(EX_VAR(opline->result.var), prop);
	}

	// 释放下一个操作码的 op1。CONST/UNUSED/CV 免操作。不支持 TMPVARCV。
	FREE_OP_DATA();
	// 静态属性赋值有2个操作码
	/* assign_static_prop has two opcodes! */
	// 偏移到目标操作码并 return ,p1:是否检查异常？使用 EX(opline)  ：使用 opline 。p2:偏移量
	ZEND_VM_NEXT_OPCODE_EX(1, 2);
}

// ing2, 骑过维度赋值
ZEND_VM_HANDLER(27, ZEND_ASSIGN_DIM_OP, VAR|CV, CONST|TMPVAR|UNUSED|NEXT|CV, OP)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *var_ptr;
	zval *value, *container, *dim;
	HashTable *ht;

	// windows: 无操作
	SAVE_OPLINE();
	// 获取zval指针, TMP/CONST返回null, UNUSED 返回 $this，不支持 TMPVAR/TMPVARCV。CV 直接返回。
	container = GET_OP1_OBJ_ZVAL_PTR_PTR_UNDEF(BP_VAR_RW);

	// 如果 容器 类型是 数组
	if (EXPECTED(Z_TYPE_P(container) == IS_ARRAY)) {
// 创建跳转标签 assign_dim_op_array
ZEND_VM_C_LABEL(assign_dim_op_array):
		// 给容器数组创建副本，并使用副本
		SEPARATE_ARRAY(container);
		// 从窗口中取出哈希表
		ht = Z_ARRVAL_P(container);
// 创建跳转标签 assign_dim_op_new_array
ZEND_VM_C_LABEL(assign_dim_op_new_array):
		// 获取zval指针, UNUSED 返回null
		dim = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
		// 如果 op2 类型是未定义
		if (OP2_TYPE == IS_UNUSED) {
			// 向哈希表添加未初始化的zval
			var_ptr = zend_hash_next_index_insert(ht, &EG(uninitialized_zval));
			// 如果添加失败
			if (UNEXPECTED(!var_ptr)) {
				// 报错：添加元素失败，下一个元素已经存在
				zend_cannot_add_element();
				// 跳转到指定标签 assign_dim_op_ret_null
				ZEND_VM_C_GOTO(assign_dim_op_ret_null);
			}
		// 否则
		} else {
			// 如果op2是常量
			if (OP2_TYPE == IS_CONST) {
				// 像数组一样更新，索引为常量。
				var_ptr = zend_fetch_dimension_address_inner_RW_CONST(ht, dim EXECUTE_DATA_CC);
			// 否则
			} else {
				// 像数组一样更新，索引为变量
				var_ptr = zend_fetch_dimension_address_inner_RW(ht, dim EXECUTE_DATA_CC);
			}
			// 如果更新 不成功
			if (UNEXPECTED(!var_ptr)) {
				// 跳转到指定标签 assign_dim_op_ret_null
				ZEND_VM_C_GOTO(assign_dim_op_ret_null);
			}
		}

		// 在执行数据中通过 p1:运算对象类型，p3:操作类型，取回p2:运算对象指向的zval
		//（常量取下一行操作码）（编译变量未定义报错）
		value = get_op_data_zval_ptr_r((opline+1)->op1_type, (opline+1)->op1);

		do {
			// 如果op2有效 并且是引用类型
			if (OP2_TYPE != IS_UNUSED && UNEXPECTED(Z_ISREF_P(var_ptr))) {
				// 取出引用 实例
				zend_reference *ref = Z_REF_P(var_ptr);
				// 解引用
				var_ptr = Z_REFVAL_P(var_ptr);
				// 如果引用实例 有 类型
				if (UNEXPECTED(ZEND_REF_HAS_TYPE_SOURCES(ref))) {
					// 根据操作码类型 对引用目标 进行 二进制 赋值操作，p1:引用实例，p2:第二个运算值
					// 加/减/乘/除/左右位移/连接/或/与/抑或/幂 运算
					zend_binary_assign_op_typed_ref(ref, value OPLINE_CC EXECUTE_DATA_CC);
					// 跳出
					break;
				}
			}
			// 调用操作码（在opline参数里）对应的二进制处理方法，p1:返回值，p2:op1，p3:op2
			zend_binary_op(var_ptr, var_ptr, value OPLINE_CC);
		} while (0);

		// 如果 操作码的运算结果有效(不是IS_UNUSED)
		if (UNEXPECTED(RETURN_VALUE_USED(opline))) {
			// 运算结果 复制 给结果变量
			ZVAL_COPY(EX_VAR(opline->result.var), var_ptr);
		}
		// 释放（变量 和 临时变量 类型的）运算对象（zval）：p1:类型，p2:编号
		FREE_OP((opline+1)->op1_type, (opline+1)->op1.var);
	// 否则
	} else {
		// 如果容器是引用类型
		if (EXPECTED(Z_ISREF_P(container))) {
			// 解引用
			container = Z_REFVAL_P(container);
			// 如果 容器 类型是 数组
			if (EXPECTED(Z_TYPE_P(container) == IS_ARRAY)) {
				// 跳转到指定标签 assign_dim_op_array
				ZEND_VM_C_GOTO(assign_dim_op_array);
			}
		}

		// 如果 容器 类型是 对象
		if (EXPECTED(Z_TYPE_P(container) == IS_OBJECT)) {
			// 取出窗口中的对象
			zend_object *obj = Z_OBJ_P(container);

			// 获取zval指针, UNUSED 返回null
			dim = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
			// 如果op2是常量 并且 有额外值？
			if (OP2_TYPE == IS_CONST && Z_EXTRA_P(dim) == ZEND_EXTRA_VALUE) {
				// 指向下一个位置的变量
				dim++;
			}
			// 像数组一样给属性赋值
			zend_binary_assign_op_obj_dim(obj, dim OPLINE_CC EXECUTE_DATA_CC);
		// 否则 如果 容器 类型是 undef/null/false
		} else if (EXPECTED(Z_TYPE_P(container) <= IS_FALSE)) {
			zend_uchar old_type;

			// 如果op1是编译变量 并且 容器类型为未定义
			if (OP1_TYPE == IS_CV && UNEXPECTED(Z_TYPE_INFO_P(container) == IS_UNDEF)) {
				// 报错：p1的变量（变量名）未定义, 并返未初始化zval
				ZVAL_UNDEFINED_OP1();
			}
			// 创建新数组，8个元素
			ht = zend_new_array(8);
			// 容器原来的类型
			old_type = Z_TYPE_P(container);
			// 哈希表放到窗口里
			ZVAL_ARR(container, ht);
			// 如果旧类型是 false
			if (UNEXPECTED(old_type == IS_FALSE)) {
				// 增加引用次数
				GC_ADDREF(ht);
				// 报错：把false转成数组已弃用
				zend_false_to_array_deprecated();
				// 引用次数-1，如果为0
				if (UNEXPECTED(GC_DELREF(ht) == 0)) {
					// 销毁哈希表
					zend_array_destroy(ht);
					// 跳转到指定标签 assign_dim_op_ret_null
					ZEND_VM_C_GOTO(assign_dim_op_ret_null);
				}
			}
			// 跳转到指定标签 assign_dim_op_new_array
			ZEND_VM_C_GOTO(assign_dim_op_new_array);
		// 否则
		} else {
			// 获取zval指针，UNUSED 类型返回null
			dim = GET_OP2_ZVAL_PTR(BP_VAR_R);
			// 就只是报错，无法使用偏移量进行操作
			zend_binary_assign_op_dim_slow(container, dim OPLINE_CC EXECUTE_DATA_CC);
// 创建跳转标签 assign_dim_op_ret_null
ZEND_VM_C_LABEL(assign_dim_op_ret_null):
			// 释放下一个操作码的 op1。CONST/UNUSED/CV 免操作。不支持 TMPVARCV。
			FREE_OP_DATA();
			// 如果 操作码的运算结果有效(不是IS_UNUSED)
			if (UNEXPECTED(RETURN_VALUE_USED(opline))) {
				// 结果为null
				ZVAL_NULL(EX_VAR(opline->result.var));
			}
		}
	}

	// 释放操作对象的附加变量
	FREE_OP2();
	// 释放操作对象的附加变量
	FREE_OP1();
	// 偏移到目标操作码并 return ,p1:是否检查异常？使用 EX(opline)  ：使用 opline 。p2:偏移量
	ZEND_VM_NEXT_OPCODE_EX(1, 2);
}

// ing3, 二进制赋值运算： 加/减/乘/除/左右位移/连接/或/与/抑或/幂 运算
ZEND_VM_HANDLER(26, ZEND_ASSIGN_OP, VAR|CV, CONST|TMPVAR|CV, OP)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *var_ptr;
	zval *value;

	// windows: 无操作
	SAVE_OPLINE();
	// 获取zval指针，UNUSED 类型返回null
	value = GET_OP2_ZVAL_PTR(BP_VAR_R);
	// 获取zval指针,TMP/CONST/UNUSED 返回null，不支持 TMPVAR/TMPVARCV
	var_ptr = GET_OP1_ZVAL_PTR_PTR(BP_VAR_RW);

	do {
		// 如果原值是引用类型
		if (UNEXPECTED(Z_TYPE_P(var_ptr) == IS_REFERENCE)) {
			// 取出引用 实例
			zend_reference *ref = Z_REF_P(var_ptr);
			// 解引用
			var_ptr = Z_REFVAL_P(var_ptr);
			// 如果引用实例 有 类型
			if (UNEXPECTED(ZEND_REF_HAS_TYPE_SOURCES(ref))) {
				// 根据操作码类型 对引用目标 进行 二进制 赋值操作，p1:引用实例，p2:第二个运算值
				zend_binary_assign_op_typed_ref(ref, value OPLINE_CC EXECUTE_DATA_CC);
				break;
			}
		}
		// 调用操作码（在opline参数里）对应的二进制处理方法，p1:返回值，p2:op1，p3:op2
		zend_binary_op(var_ptr, var_ptr, value OPLINE_CC);
	} while (0);

	// 如果 操作码的运算结果有效(不是IS_UNUSED)
	if (UNEXPECTED(RETURN_VALUE_USED(opline))) {
		// 运算结果 复制 给结果变量
		ZVAL_COPY(EX_VAR(opline->result.var), var_ptr);
	}

	// 释放操作对象的附加变量
	FREE_OP2();
	// 释放操作对象的附加变量
	FREE_OP1();
	// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, 对象属性（前）自增
ZEND_VM_HANDLER(132, ZEND_PRE_INC_OBJ, VAR|UNUSED|THIS|CV, CONST|TMPVAR|CV, CACHE_SLOT)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *object;
	zval *property;
	zval *zptr;
	void **cache_slot;
	zend_property_info *prop_info;
	zend_object *zobj;
	zend_string *name, *tmp_name;

	// windows: 无操作
	SAVE_OPLINE();
	// 获取zval指针, TMP/CONST返回null, UNUSED 返回 $this，不支持 TMPVAR/TMPVARCV。CV 直接返回。
	object = GET_OP1_OBJ_ZVAL_PTR_PTR_UNDEF(BP_VAR_RW);
	// 获取zval指针，UNUSED 类型返回null
	property = GET_OP2_ZVAL_PTR(BP_VAR_R);

	// 为了break
	do {
		// 如果op1已使用 并且 类型不是对象
		if (OP1_TYPE != IS_UNUSED && UNEXPECTED(Z_TYPE_P(object) != IS_OBJECT)) {
			// 如果是引用类型 并且 引用目标是对象
			if (Z_ISREF_P(object) && Z_TYPE_P(Z_REFVAL_P(object)) == IS_OBJECT) {
				// 解引用
				object = Z_REFVAL_P(object);
				// 跳转到指定标签 pre_incdec_object
				ZEND_VM_C_GOTO(pre_incdec_object);
			}
			// 如果 op1 是 编译变量 并且 对象类型为 未定义
			if (OP1_TYPE == IS_CV
			 && UNEXPECTED(Z_TYPE_P(object) == IS_UNDEF)) {
				// 报错：p1的变量（变量名）未定义, 并返未初始化zval
				ZVAL_UNDEFINED_OP1();
			}
			// 没有访问的对象，报错
			// 抛出【对象无效】异常
			zend_throw_non_object_error(object, property OPLINE_CC EXECUTE_DATA_CC);
			// 跳出
			break;
		}

// 创建跳转标签 pre_incdec_object
ZEND_VM_C_LABEL(pre_incdec_object):
		// 这里可以确定是针对  对象 进行操作
		/* here we are sure we are dealing with an object */
		// 取出 对象
		zobj = Z_OBJ_P(object);
		// 如果op2是常量
		if (OP2_TYPE == IS_CONST) {
			// 取出 属性名
			name = Z_STR_P(property);
		// 否则，op2不是常量
		} else {
			// 获取临时字串（不抛错）返回
			name = zval_try_get_tmp_string(property, &tmp_name);
			// 如果没有属性名
			if (UNEXPECTED(!name)) {
				// 如果 opline 结果类型是 变量或临时变量，把结果 zval 标记为未定义
				UNDEF_RESULT();
				// 跳出
				break;
			}
		}
		// 如果op2是常量，获取缓存位置
		cache_slot = (OP2_TYPE == IS_CONST) ? CACHE_ADDR(opline->extended_value) : NULL;
		// 取得对象属性。如果成功
		if (EXPECTED((zptr = zobj->handlers->get_property_ptr_ptr(zobj, name, BP_VAR_RW, cache_slot)) != NULL)) {
			// 如果获取到错误
			if (UNEXPECTED(Z_ISERROR_P(zptr))) {
				// 如果 操作码的运算结果有效(不是IS_UNUSED)
				if (UNEXPECTED(RETURN_VALUE_USED(opline))) {
					// 结果为null
					ZVAL_NULL(EX_VAR(opline->result.var));
				}
			// 否则，获取成功
			} else {
				// 如果op2是常量
				if (OP2_TYPE == IS_CONST) {
					// 返回数组的第一个元素
					prop_info = (zend_property_info *) CACHED_PTR_EX(cache_slot + 2);
				// 否则
				} else {
					// 验证 属性有效（在属性表中） 并 获取它的属性信息，p1:对象，p2:属性zval
					prop_info = zend_object_fetch_property_type_info(Z_OBJ_P(object), zptr);
				}
				// 属性 前置 自增或自减
				zend_pre_incdec_property_zval(zptr, prop_info OPLINE_CC EXECUTE_DATA_CC);
			}
		// 获取属性失败
		} else {
			// 取出属性值，进行 前自增/自减 运算后更新回去（这里会创建）
			zend_pre_incdec_overloaded_property(zobj, name, cache_slot OPLINE_CC EXECUTE_DATA_CC);
		}
		// 如果 OP2 不是常量
		if (OP2_TYPE != IS_CONST) {
			// 释放临时名称
			zend_tmp_string_release(tmp_name);
		}
	} while (0);

	// 释放操作对象的附加变量
	FREE_OP2();
	// 释放操作对象的附加变量
	FREE_OP1();
	// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, 对象属性（前）自减
ZEND_VM_HANDLER(133, ZEND_PRE_DEC_OBJ, VAR|UNUSED|THIS|CV, CONST|TMPVAR|CV, CACHE_SLOT)
{
	// 跳转到操作码标签 
	ZEND_VM_DISPATCH_TO_HANDLER(ZEND_PRE_INC_OBJ);
}

// ing3, 后面的自加 “++”。 3*3=9个分支（确认过）。
ZEND_VM_HANDLER(134, ZEND_POST_INC_OBJ, VAR|UNUSED|THIS|CV, CONST|TMPVAR|CV, CACHE_SLOT)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *object;
	zval *property;
	zval *zptr;
	void **cache_slot;
	zend_property_info *prop_info;
	zend_object *zobj;
	zend_string *name, *tmp_name;

	// windows: 无操作
	SAVE_OPLINE();
	// 获取zval指针, TMP/CONST返回null, UNUSED 返回 $this，不支持 TMPVAR/TMPVARCV。CV 直接返回。
	object = GET_OP1_OBJ_ZVAL_PTR_PTR_UNDEF(BP_VAR_RW);
	// 获取zval指针，UNUSED 类型返回null
	property = GET_OP2_ZVAL_PTR(BP_VAR_R);

	// 加一个do是为了可以方便地用 break跳出，否则要加标识或用if比较麻烦
	do {
		// op1类型不是未定义 并且 不是对象
		if (OP1_TYPE != IS_UNUSED && UNEXPECTED(Z_TYPE_P(object) != IS_OBJECT)) {
			// 如果是引用 并且 引用的是对象
			if (Z_ISREF_P(object) && Z_TYPE_P(Z_REFVAL_P(object)) == IS_OBJECT) {
				// 切换到被引用的对象
				object = Z_REFVAL_P(object);
				// 跳转到指定标签 post_incdec_object
				ZEND_VM_C_GOTO(post_incdec_object);
			}
			// 如果op1类型是编译变量 并且 对象类型是 未定义
			if (OP1_TYPE == IS_CV
			 && UNEXPECTED(Z_TYPE_P(object) == IS_UNDEF)) {
				// 报错：p1的变量（变量名）未定义, 并返未初始化zval
				ZVAL_UNDEFINED_OP1();
			}
			// 其他情况，报错 
			zend_throw_non_object_error(object, property OPLINE_CC EXECUTE_DATA_CC);
			// 跳出
			break;
		}
// 创建跳转标签 post_incdec_object
ZEND_VM_C_LABEL(post_incdec_object):
		// 这里确定是在对对象进行操作
		/* here we are sure we are dealing with an object */
		// 取出变量中的对象
		zobj = Z_OBJ_P(object);
		// op2类型是常量
		if (OP2_TYPE == IS_CONST) {
			// 属性名
			name = Z_STR_P(property);
		// op2类型不是常量
		// 否则
		} else {
			// 取出临时字串，对象属性名
			name = zval_try_get_tmp_string(property, &tmp_name);
			// 如果不存在
			if (UNEXPECTED(!name)) {
				// 结果为 未定义
				ZVAL_UNDEF(EX_VAR(opline->result.var));
				// 跳出
				break;
			}
		}
		// 取得缓存位置（op2是常量时才有效）
		cache_slot = (OP2_TYPE == IS_CONST) ? CACHE_ADDR(opline->extended_value) : NULL;
		// 如果能从缓存中获取对象属性
		if (EXPECTED((zptr = zobj->handlers->get_property_ptr_ptr(zobj, name, BP_VAR_RW, cache_slot)) != NULL)) {
			// 如果 运行出错
			if (UNEXPECTED(Z_ISERROR_P(zptr))) {
				// 结果为null
				ZVAL_NULL(EX_VAR(opline->result.var));
			// 正常情况
			// 否则
			} else {
				// op2类型为常量
				if (OP2_TYPE == IS_CONST) {
					// 获取属性信息
					// 返回数组的第一个元素
					prop_info = (zend_property_info*)CACHED_PTR_EX(cache_slot + 2);					
				// op2不是常量
				// 否则
				} else {
					// 验证 属性有效（在属性表中） 并 获取它的属性信息，p1:对象，p2:属性zval
					prop_info = zend_object_fetch_property_type_info(Z_OBJ_P(object), zptr);
				}
				// 属性的后置 自增或自减运算？ 两个可能有的参数 opline,execute_data
				zend_post_incdec_property_zval(zptr, prop_info OPLINE_CC EXECUTE_DATA_CC);
			}
		// 否则
		} else {
			// 取出属性值，进行 后自增/自减 运算后更新回去
			zend_post_incdec_overloaded_property(zobj, name, cache_slot OPLINE_CC EXECUTE_DATA_CC);
		}
		// op2类型不是常量
		if (OP2_TYPE != IS_CONST) {
			// 释放名称
			zend_tmp_string_release(tmp_name);
		}
	} while (0);
	
	// 释放操作对象的附加变量
	FREE_OP2();
	// 释放操作对象的附加变量
	FREE_OP1();
	// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, 后面的自减 “--”。
ZEND_VM_HANDLER(135, ZEND_POST_DEC_OBJ, VAR|UNUSED|THIS|CV, CONST|TMPVAR|CV, CACHE_SLOT)
{
	// 跳转到操作码标签 
	ZEND_VM_DISPATCH_TO_HANDLER(ZEND_POST_INC_OBJ);
}

// ing3, （前）属性自增
/* No specialization for op_types (CONST|TMPVAR|CV, UNUSED|CONST|VAR) */
ZEND_VM_HANDLER(38, ZEND_PRE_INC_STATIC_PROP, ANY, ANY, CACHE_SLOT)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *prop;
	zend_property_info *prop_info;

	// windows: 无操作
	SAVE_OPLINE();

	// 通过缓存位置和类型，返回静态属性地址。p1:返回属性值，p2:返回属性信息，p3:缓存位置，p4:获取类型，p5:falgs
	if (zend_fetch_static_property_address(&prop, &prop_info, opline->extended_value, BP_VAR_RW, 0 OPLINE_CC EXECUTE_DATA_CC) != SUCCESS) {
		// 如果 opline 结果类型是 变量或临时变量，把结果 zval 标记为未定义
		UNDEF_RESULT();
		// 主要是 return
		HANDLE_EXCEPTION();
	}

	// 属性 前置 自增或自减
	// 如果类型存在，使得属性信息，否则 null
	zend_pre_incdec_property_zval(prop,
		ZEND_TYPE_IS_SET(prop_info->type) ? prop_info : NULL OPLINE_CC EXECUTE_DATA_CC);

	// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, （前）属性自减
/* No specialization for op_types (CONST|TMPVAR|CV, UNUSED|CONST|VAR) */
ZEND_VM_HANDLER(39, ZEND_PRE_DEC_STATIC_PROP, ANY, ANY, CACHE_SLOT)
{
	// 跳转到操作码标签 ZEND_PRE_INC_STATIC_PROP
	ZEND_VM_DISPATCH_TO_HANDLER(ZEND_PRE_INC_STATIC_PROP);
}

// ing3, （后）属性自增
/* No specialization for op_types (CONST|TMPVAR|CV, UNUSED|CONST|VAR) */
ZEND_VM_HANDLER(40, ZEND_POST_INC_STATIC_PROP, ANY, ANY, CACHE_SLOT)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *prop;
	zend_property_info *prop_info;

	// windows: 无操作
	SAVE_OPLINE();

	// 通过缓存位置和类型，返回静态属性地址。p1:返回属性值，p2:返回属性信息，p3:缓存位置，p4:获取类型，p5:falgs
	if (zend_fetch_static_property_address(&prop, &prop_info, opline->extended_value, BP_VAR_RW, 0 OPLINE_CC EXECUTE_DATA_CC) != SUCCESS) {
		// 如果 opline 结果类型是 变量或临时变量，把结果 zval 标记为未定义
		UNDEF_RESULT();
		// 主要是 return
		HANDLE_EXCEPTION();
	}

	// 属性 后置 自增或自减
	// 如果类型存在，使用此属性信息，否则无属性信息
	zend_post_incdec_property_zval(prop,
		ZEND_TYPE_IS_SET(prop_info->type) ? prop_info : NULL OPLINE_CC EXECUTE_DATA_CC);

	// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, （后）属性自减
/* No specialization for op_types (CONST|TMPVAR|CV, UNUSED|CONST|VAR) */
ZEND_VM_HANDLER(41, ZEND_POST_DEC_STATIC_PROP, ANY, ANY, CACHE_SLOT)
{
	// 跳转到操作码标签 ZEND_POST_INC_STATIC_PROP
	ZEND_VM_DISPATCH_TO_HANDLER(ZEND_POST_INC_STATIC_PROP);
}

// ing3, (前)自增 运算, 检验传入的变量是否有效，进行 自增 运算，然后销毁运算对象op1
ZEND_VM_HELPER(zend_pre_inc_helper, VAR|CV, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *var_ptr;
	// 获取zval指针, TMP/CONST/UNUSED 返回null，不支持 TMPVAR/TMPVARCV
	var_ptr = GET_OP1_ZVAL_PTR_PTR_UNDEF(BP_VAR_RW);

	// windows: 无操作
	SAVE_OPLINE();
	// 如果 op1 是 编译变量 并且 变量指针是 未定义
	if (OP1_TYPE == IS_CV && UNEXPECTED(Z_TYPE_P(var_ptr) == IS_UNDEF)) {
		// 报错：p1的变量（变量名）未定义, 并返未初始化zval
		ZVAL_UNDEFINED_OP1();
		// 把原值设置为 null
		ZVAL_NULL(var_ptr);
	}

	do {
		// 如果原值是引用类型
		if (UNEXPECTED(Z_TYPE_P(var_ptr) == IS_REFERENCE)) {
			// 取出引用 实例
			zend_reference *ref = Z_REF_P(var_ptr);
			// 解引用
			var_ptr = Z_REFVAL_P(var_ptr);
			// 如果引用实例 有 类型
			if (UNEXPECTED(ZEND_REF_HAS_TYPE_SOURCES(ref))) {
				// 给带类型的引用目标作 自增 或 自减。p1:引用对象，p2:返回值
				zend_incdec_typed_ref(ref, NULL OPLINE_CC EXECUTE_DATA_CC);
				// 跳过自增方法
				break;
			}
		}
		// 自增运算
		increment_function(var_ptr);
	} while (0);

	// 如果 操作码的运算结果有效(不是IS_UNUSED)
	if (UNEXPECTED(RETURN_VALUE_USED(opline))) {
		// 运算结果 复制 给结果变量
		ZVAL_COPY(EX_VAR(opline->result.var), var_ptr);
	}

	// 释放操作对象的附加变量
	FREE_OP1();
	// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, （前）自增
ZEND_VM_HOT_HANDLER(34, ZEND_PRE_INC, VAR|CV, ANY, SPEC(RETVAL))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *var_ptr;
	// 获取zval指针, TMP/CONST/UNUSED 返回null，不支持 TMPVAR/TMPVARCV
	var_ptr = GET_OP1_ZVAL_PTR_PTR_UNDEF(BP_VAR_RW);

	// 变量是整数
	if (EXPECTED(Z_TYPE_P(var_ptr) == IS_LONG)) {
		// 计算自增
		fast_long_increment_function(var_ptr);
		// 如果 操作码的运算结果有效(不是IS_UNUSED)
		if (UNEXPECTED(RETURN_VALUE_USED(opline))) {
			// 值复制给 结果 
			ZVAL_COPY_VALUE(EX_VAR(opline->result.var), var_ptr);
		}
		// opline + 1，到目标操作码并 return
		ZEND_VM_NEXT_OPCODE();
	}

	// 调用方法 zend_pre_inc_helper
	ZEND_VM_DISPATCH_TO_HELPER(zend_pre_inc_helper);
}

// ing3, (前)自减 运算, 检验传入的变量是否有效，进行 自减 运算，然后销毁运算对象op1
ZEND_VM_HELPER(zend_pre_dec_helper, VAR|CV, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *var_ptr;
	// 获取zval指针, TMP/CONST/UNUSED 返回null，不支持 TMPVAR/TMPVARCV
	var_ptr = GET_OP1_ZVAL_PTR_PTR_UNDEF(BP_VAR_RW);

	// windows: 无操作
	SAVE_OPLINE();
	// 如果 op1 是 编译变量 并且 变量指针是未定义
	if (OP1_TYPE == IS_CV && UNEXPECTED(Z_TYPE_P(var_ptr) == IS_UNDEF)) {
		// 报错：p1的变量（变量名）未定义, 并返未初始化zval
		ZVAL_UNDEFINED_OP1();
		// 把原值设置为 null
		ZVAL_NULL(var_ptr);
	}

	do {
		// 如果原值是引用类型
		if (UNEXPECTED(Z_TYPE_P(var_ptr) == IS_REFERENCE)) {
			// 取出引用 实例
			zend_reference *ref = Z_REF_P(var_ptr);
			// 解引用
			var_ptr = Z_REFVAL_P(var_ptr);
			// 如果引用实例 有 类型
			if (UNEXPECTED(ZEND_REF_HAS_TYPE_SOURCES(ref))) {
				// 给带类型的引用目标作 自增 或 自减。p1:引用对象，p2:返回值
				zend_incdec_typed_ref(ref, NULL OPLINE_CC EXECUTE_DATA_CC);
				// 跳过自减方法
				break;
			}
		}
		// 自减运算
		decrement_function(var_ptr);
	} while (0);

	// 如果 操作码的运算结果有效(不是IS_UNUSED)
	if (UNEXPECTED(RETURN_VALUE_USED(opline))) {
		// 运算结果 复制 给结果变量
		ZVAL_COPY(EX_VAR(opline->result.var), var_ptr);
	}

	// 释放操作对象的附加变量
	FREE_OP1();
	// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, (前)自减
ZEND_VM_HOT_HANDLER(35, ZEND_PRE_DEC, VAR|CV, ANY, SPEC(RETVAL))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *var_ptr;
	// 获取zval指针, TMP/CONST/UNUSED 返回null，不支持 TMPVAR/TMPVARCV
	var_ptr = GET_OP1_ZVAL_PTR_PTR_UNDEF(BP_VAR_RW);

	// 如果是整数
	if (EXPECTED(Z_TYPE_P(var_ptr) == IS_LONG)) {
		// 先自增
		fast_long_decrement_function(var_ptr);
		// 如果 操作码的运算结果有效(不是IS_UNUSED)
		if (UNEXPECTED(RETURN_VALUE_USED(opline))) {
			// 保存结果
			ZVAL_COPY_VALUE(EX_VAR(opline->result.var), var_ptr);
		}
		// opline + 1，到目标操作码并 return
		ZEND_VM_NEXT_OPCODE();
	}

	// 调用方法 zend_pre_inc_helper
	ZEND_VM_DISPATCH_TO_HELPER(zend_pre_dec_helper);
}

// ing3, (后)自增 运算, 检验传入的变量是否有效，进行 自增 运算，然后销毁运算对象op1
ZEND_VM_HELPER(zend_post_inc_helper, VAR|CV, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *var_ptr;
	// 获取zval指针, TMP/CONST/UNUSED 返回null，不支持 TMPVAR/TMPVARCV
	var_ptr = GET_OP1_ZVAL_PTR_PTR_UNDEF(BP_VAR_RW);

	// windows: 无操作
	SAVE_OPLINE();
	// 如果 op1 是 编译变量 并且变量指针是 未定义
	if (OP1_TYPE == IS_CV && UNEXPECTED(Z_TYPE_P(var_ptr) == IS_UNDEF)) {
		// 报错：p1的变量（变量名）未定义, 并返未初始化zval
		ZVAL_UNDEFINED_OP1();
		// 把原值设置为 null
		ZVAL_NULL(var_ptr);
	}

	do {
		// 如果原值是引用类型
		if (UNEXPECTED(Z_TYPE_P(var_ptr) == IS_REFERENCE)) {
			// 取出引用 实例
			zend_reference *ref = Z_REF_P(var_ptr);
			// 解引用
			var_ptr = Z_REFVAL_P(var_ptr);
			// 如果引用实例 有 类型
			if (UNEXPECTED(ZEND_REF_HAS_TYPE_SOURCES(ref))) {
				// 给带类型的引用目标作 自增 或 自减。p1:引用对象，p2:返回值
				zend_incdec_typed_ref(ref, EX_VAR(opline->result.var) OPLINE_CC EXECUTE_DATA_CC);
				// 跳过自增 函数
				break;
			}
		}
		// 运算结果 复制 给结果变量
		ZVAL_COPY(EX_VAR(opline->result.var), var_ptr);
		// 返回后，进行自增运算
		increment_function(var_ptr);
	} while (0);

	// 释放操作对象的附加变量
	FREE_OP1();
	// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3,（后）自增
ZEND_VM_HOT_HANDLER(36, ZEND_POST_INC, VAR|CV, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *var_ptr;
	// 获取zval指针, TMP/CONST/UNUSED 返回null，不支持 TMPVAR/TMPVARCV
	var_ptr = GET_OP1_ZVAL_PTR_PTR_UNDEF(BP_VAR_RW);

	// 如果， op1是整数
	if (EXPECTED(Z_TYPE_P(var_ptr) == IS_LONG)) {
		// 先保存进结果
		ZVAL_LONG(EX_VAR(opline->result.var), Z_LVAL_P(var_ptr));
		// 自增运算
		fast_long_increment_function(var_ptr);
		// opline + 1，到目标操作码并 return
		ZEND_VM_NEXT_OPCODE();
	}

	// 调用方法 zend_post_inc_helper
	ZEND_VM_DISPATCH_TO_HELPER(zend_post_inc_helper);
}

// ing3, (后)自减 运算, 检验传入的变量是否有效，进行 自减 运算，然后销毁运算对象op1
ZEND_VM_HELPER(zend_post_dec_helper, VAR|CV, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *var_ptr;
	// 获取zval指针, TMP/CONST/UNUSED 返回null，不支持 TMPVAR/TMPVARCV
	var_ptr = GET_OP1_ZVAL_PTR_PTR_UNDEF(BP_VAR_RW);

	// windows: 无操作
	SAVE_OPLINE();
	// 如果 op1 是 编译变量 并且 变量指针 类型是未定义
	if (OP1_TYPE == IS_CV && UNEXPECTED(Z_TYPE_P(var_ptr) == IS_UNDEF)) {
		// 报错：p1的变量（变量名）未定义, 并返未初始化zval
		ZVAL_UNDEFINED_OP1();
		// 把原值设置为 null
		ZVAL_NULL(var_ptr);
	}

	do {
		// 如果原值是引用类型
		if (UNEXPECTED(Z_TYPE_P(var_ptr) == IS_REFERENCE)) {
			// 取出引用 实例
			zend_reference *ref = Z_REF_P(var_ptr);
			// 解引用
			var_ptr = Z_REFVAL_P(var_ptr);
			// 如果引用实例 有 类型
			if (UNEXPECTED(ZEND_REF_HAS_TYPE_SOURCES(ref))) {
				// 给带类型的引用目标作 自增 或 自减。p1:引用对象，p2:返回值
				zend_incdec_typed_ref(ref, EX_VAR(opline->result.var) OPLINE_CC EXECUTE_DATA_CC);
				// 跳过自增方法
				break;
			}
		}
		// 运算结果 复制 给结果变量
		ZVAL_COPY(EX_VAR(opline->result.var), var_ptr);
		// 进行自增运算
		decrement_function(var_ptr);
	} while (0);

	// 释放操作对象的附加变量
	FREE_OP1();
	// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, （后）自减
ZEND_VM_HOT_HANDLER(37, ZEND_POST_DEC, VAR|CV, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *var_ptr;

	// 获取zval指针, TMP/CONST/UNUSED 返回null，不支持 TMPVAR/TMPVARCV
	var_ptr = GET_OP1_ZVAL_PTR_PTR_UNDEF(BP_VAR_RW);

	// 如果是整数
	if (EXPECTED(Z_TYPE_P(var_ptr) == IS_LONG)) {
		// 先保存到结果里
		ZVAL_LONG(EX_VAR(opline->result.var), Z_LVAL_P(var_ptr));
		// 再算自减
		fast_long_decrement_function(var_ptr);
		// opline + 1，到目标操作码并 return
		ZEND_VM_NEXT_OPCODE();
	}

	// 调用方法 zend_post_dec_helper
	ZEND_VM_DISPATCH_TO_HELPER(zend_post_dec_helper);
}

// ing3, 操作码 ZEND_ECHO 处理器，第一个操作对象可以是 CONST|TMPVAR|CV 
ZEND_VM_HANDLER(136, ZEND_ECHO, CONST|TMPVAR|CV, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *z;

	// windows: 无操作
	SAVE_OPLINE();
	// 获取zval指针, UNUSED 返回null
	z = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);

	// 如果类型是 string
	if (Z_TYPE_P(z) == IS_STRING) {
		// 取出 变量中的 string
		zend_string *str = Z_STR_P(z);

		// 如果不是空
		if (ZSTR_LEN(str) != 0) {
			// 打印字串
			zend_write(ZSTR_VAL(str), ZSTR_LEN(str));
		}
	// 其他类型
	} else {
		// 调用函数 取出 op1 中的 string
		zend_string *str = zval_get_string_func(z);

		// 如果不是空
		if (ZSTR_LEN(str) != 0) {
			// 打印字串
			zend_write(ZSTR_VAL(str), ZSTR_LEN(str));
		// 如果长度是空，op1 类型是编译变量 并且 ？
		} else if (OP1_TYPE == IS_CV && UNEXPECTED(Z_TYPE_P(z) == IS_UNDEF)) {
			// 报错：p1的变量（变量名）未定义, 并返未初始化zval
			ZVAL_UNDEFINED_OP1();
		}
		// 释放string
		zend_string_release_ex(str, 0);
	}

	// 释放操作对象的附加变量
	FREE_OP1();
	// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}


// ing3, 在 GLOBAL 或 当前上下文中获取变量
ZEND_VM_HELPER(zend_fetch_var_address_helper, CONST|TMPVAR|CV, UNUSED, int type)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *varname;
	zval *retval;
	zend_string *name, *tmp_name;
	HashTable *target_symbol_table;

	// windows: 无操作
	SAVE_OPLINE();
	// 获取zval指针, UNUSED 返回null
	varname = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);

	// 如果op1是常量
	if (OP1_TYPE == IS_CONST) {
		// 变量名
		name = Z_STR_P(varname);
	// 如果变量名 是字串
	} else if (EXPECTED(Z_TYPE_P(varname) == IS_STRING)) {
		// 取出 字串
		name = Z_STR_P(varname);
		// 临时名称为 null
		tmp_name = NULL;
	// 否则
	} else {
		// 如果 op1 是 编译变量 并且 变量名是 未定义
		if (OP1_TYPE == IS_CV && UNEXPECTED(Z_TYPE_P(varname) == IS_UNDEF)) {
			// 报错：p1的变量（变量名）未定义, 并返未初始化zval
			ZVAL_UNDEFINED_OP1();
		}
		// 获取临时字串（不抛错）返回
		name = zval_try_get_tmp_string(varname, &tmp_name);
		// 如果没有名称
		if (UNEXPECTED(!name)) {
			// 如果扩展值不是 ZEND_FETCH_GLOBAL_LOCK
			if (!(opline->extended_value & ZEND_FETCH_GLOBAL_LOCK)) {
				// 释放操作对象的附加变量
				FREE_OP1();
			}
			// 结果为 未定义
			ZVAL_UNDEF(EX_VAR(opline->result.var));
			// 主要是 return
			HANDLE_EXCEPTION();
		}
	}

	// 这里获取的符号表是 global 或当前执行数据 上下文中的变量列表
	// 取得目标符号表。运行时符号表 或 当前执行数据中的符号表
	target_symbol_table = zend_get_target_symbol_table(opline->extended_value EXECUTE_DATA_CC);
	// 从符号表中取出此变量
	retval = zend_hash_find_ex(target_symbol_table, name, OP1_TYPE == IS_CONST);
	// 如果没有找到
	if (retval == NULL) {
		// 如果 变量名是 $this
		if (UNEXPECTED(zend_string_equals(name, ZSTR_KNOWN(ZEND_STR_THIS)))) {
// 创建跳转标签 fetch_this
ZEND_VM_C_LABEL(fetch_this):
			// 读写 $this
			zend_fetch_this_var(type OPLINE_CC EXECUTE_DATA_CC);
			// 如果 OP1 不是常量
			if (OP1_TYPE != IS_CONST) {
				// 释放临时变量
				zend_tmp_string_release(tmp_name);
			}
			// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
			ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
		}
		// 写入操作
		if (type == BP_VAR_W) {
			// 在符号表中添加一个未初始化变量，name是键
			retval = zend_hash_add_new(target_symbol_table, name, &EG(uninitialized_zval));
		// isset 或 unset操作
		} else if (type == BP_VAR_IS || type == BP_VAR_UNSET) {
			// 不需要操作
			// 结果为未初始化变量
			retval = &EG(uninitialized_zval);
		// 否则，其他操作 如 BP_VAR_RW
		} else {
			// 报错：变量未定义
			zend_error(E_WARNING, "Undefined %svariable $%s",
				(opline->extended_value & ZEND_FETCH_GLOBAL ? "global " : ""), ZSTR_VAL(name));
			// 如果	操作是 BP_VAR_RW 并且没有异常
			if (type == BP_VAR_RW && !EG(exception)) {
				// 符号表中更新 name, 值为未定义变量
				retval = zend_hash_update(target_symbol_table, name, &EG(uninitialized_zval));
			// 否则
			} else {
				// 结果为未初始化变量
				retval = &EG(uninitialized_zval);
			}
		}
	// GLOBAL 或 $$name 变量可以当作 指向编译变量的 间接引用指针
	/* GLOBAL or $$name variable may be an INDIRECT pointer to CV */
	} else if (Z_TYPE_P(retval) == IS_INDIRECT) {
		// 
		retval = Z_INDIRECT_P(retval);
		// 如果结果为 未定义
		if (Z_TYPE_P(retval) == IS_UNDEF) {
			// 如果变量名为 $this
			if (UNEXPECTED(zend_string_equals(name, ZSTR_KNOWN(ZEND_STR_THIS)))) {
				// 跳转到指定标签 fetch_this
				ZEND_VM_C_GOTO(fetch_this);
			}
			// 如果操作为创建 
			if (type == BP_VAR_W) {
				// 结果为null
				ZVAL_NULL(retval);
			// 如果操作类型为 isset 或 删除
			} else if (type == BP_VAR_IS || type == BP_VAR_UNSET) {
				// 结果为未初始化zval
				retval = &EG(uninitialized_zval);
			// 否则
			} else {
				// 报错：未定义变量
				zend_error(E_WARNING, "Undefined %svariable $%s",
					(opline->extended_value & ZEND_FETCH_GLOBAL ? "global " : ""), ZSTR_VAL(name));
				// 如果是更新操作 并且 没有异常
				if (type == BP_VAR_RW && !EG(exception)) {
					// 结果为null
					ZVAL_NULL(retval);
				// 否则
				} else {
					// 结果为未初始化zval
					retval = &EG(uninitialized_zval);
				}
			}
		}
	}

	// 如果没有 ZEND_FETCH_GLOBAL_LOCK
	if (!(opline->extended_value & ZEND_FETCH_GLOBAL_LOCK)) {
		// 释放操作对象的附加变量
		FREE_OP1();
	}

	// 如果 OP1 不是常量
	if (OP1_TYPE != IS_CONST) {
		// 释放临时变量
		zend_tmp_string_release(tmp_name);
	}

	// 返回值必须有效
	ZEND_ASSERT(retval != NULL);
	// 如果操作是 读或 isset
	if (type == BP_VAR_R || type == BP_VAR_IS) {
		// 复制引用目标 和 附加信息，并增加引用计数
		ZVAL_COPY_DEREF(EX_VAR(opline->result.var), retval);
	// 否则
	} else {
		// 间接引用赋值
		// 给 zval.zv 添加指针, 并标记成 间接引用 类型
		ZVAL_INDIRECT(EX_VAR(opline->result.var), retval);
	}
	// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, 读取global或当前上下文（执行数据）中的变量
ZEND_VM_HANDLER(80, ZEND_FETCH_R, CONST|TMPVAR|CV, UNUSED, VAR_FETCH)
{
	// 调用方法 zend_fetch_var_address_helper
	ZEND_VM_DISPATCH_TO_HELPER(zend_fetch_var_address_helper, type, BP_VAR_R);
}

// ing3, 创建global或当前上下文（执行数据）中的变量
ZEND_VM_HANDLER(83, ZEND_FETCH_W, CONST|TMPVAR|CV, UNUSED, VAR_FETCH)
{
	// 调用方法 zend_fetch_var_address_helper
	ZEND_VM_DISPATCH_TO_HELPER(zend_fetch_var_address_helper, type, BP_VAR_W);
}

// ing3, 更新global或当前上下文（执行数据）中的变量
ZEND_VM_HANDLER(86, ZEND_FETCH_RW, CONST|TMPVAR|CV, UNUSED, VAR_FETCH)
{
	// 调用方法 zend_fetch_var_address_helper
	ZEND_VM_DISPATCH_TO_HELPER(zend_fetch_var_address_helper, type, BP_VAR_RW);
}

// ing3, 获取 函数参数（执行数据，中的变量）
ZEND_VM_HANDLER(92, ZEND_FETCH_FUNC_ARG, CONST|TMPVAR|CV, UNUSED, VAR_FETCH)
{
	// 如果引用传递，用 BP_VAR_W ，否则 BP_VAR_R
	int fetch_type =
		(UNEXPECTED(ZEND_CALL_INFO(EX(call)) & ZEND_CALL_SEND_ARG_BY_REF)) ?
			BP_VAR_W : BP_VAR_R;
	// 调用方法 zend_fetch_var_address_helper
	ZEND_VM_DISPATCH_TO_HELPER(zend_fetch_var_address_helper, type, fetch_type);
}

// ing3, 删除global或当前上下文（执行数据）中的变量
ZEND_VM_HANDLER(95, ZEND_FETCH_UNSET, CONST|TMPVAR|CV, UNUSED, VAR_FETCH)
{
	// 调用方法 zend_fetch_var_address_helper
	ZEND_VM_DISPATCH_TO_HELPER(zend_fetch_var_address_helper, type, BP_VAR_UNSET);
}

// ing3, 检查 global或当前上下文（执行数据）中的变量
ZEND_VM_HANDLER(89, ZEND_FETCH_IS, CONST|TMPVAR|CV, UNUSED, VAR_FETCH)
{
	// 调用方法 zend_fetch_var_address_helper
	ZEND_VM_DISPATCH_TO_HELPER(zend_fetch_var_address_helper, type, BP_VAR_IS);
}

/* No specialization for op_types (CONST|TMPVAR|CV, UNUSED|CONST|VAR) */

// ing3, 通过操作码扩展信息中的缓存位置，获取静态属性地址。
ZEND_VM_HELPER(zend_fetch_static_prop_helper, ANY, ANY, int type)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *prop;

	// windows: 无操作
	SAVE_OPLINE();

	// 返回静态属性地址。p1:返回属性值，p2:返回属性信息，p3:缓存位置，p4:获取类型，p5:falgs
	if (UNEXPECTED(zend_fetch_static_property_address(&prop, NULL, opline->extended_value & ~ZEND_FETCH_OBJ_FLAGS, type, opline->extended_value OPLINE_CC EXECUTE_DATA_CC) != SUCCESS)) {
		// 如果失败。必须要有 异常 或 操作类型是 BP_VAR_IS
		ZEND_ASSERT(EG(exception) || (type == BP_VAR_IS));
		// 结果 为未初始化zval
		prop = &EG(uninitialized_zval);
	}

	// 如果操作类型为 BP_VAR_R 或 BP_VAR_IS
	if (type == BP_VAR_R || type == BP_VAR_IS) {
		// 复制引用目标 和 附加信息，到结果变量， 并增加引用计数
		ZVAL_COPY_DEREF(EX_VAR(opline->result.var), prop);
	// 否则
	} else {
		// 间接引用赋值给 结果变量
		// 给 zval.zv 添加指针, 并标记成 间接引用 类型
		ZVAL_INDIRECT(EX_VAR(opline->result.var), prop);
	}
	// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, 读取静态属性
/* No specialization for op_types (CONST|TMPVAR|CV, UNUSED|CLASS_FETCH|CONST|VAR) */
ZEND_VM_HANDLER(173, ZEND_FETCH_STATIC_PROP_R, ANY, CLASS_FETCH, CACHE_SLOT)
{
	// 通过操作码扩展信息中的缓存位置，获取静态属性地址。
	// 调用方法 zend_fetch_static_prop_helper
	ZEND_VM_DISPATCH_TO_HELPER(zend_fetch_static_prop_helper, type, BP_VAR_R);
}

// ing3, 写入静态属性
/* No specialization for op_types (CONST|TMPVAR|CV, UNUSED|CLASS_FETCH|CONST|VAR) */
ZEND_VM_HANDLER(174, ZEND_FETCH_STATIC_PROP_W, ANY, CLASS_FETCH, FETCH_REF|DIM_WRITE|CACHE_SLOT)
{
	// 通过操作码扩展信息中的缓存位置，获取静态属性地址。
	// 调用方法 zend_fetch_static_prop_helper
	ZEND_VM_DISPATCH_TO_HELPER(zend_fetch_static_prop_helper, type, BP_VAR_W);
}

// ing3, 更新静态属性
/* No specialization for op_types (CONST|TMPVAR|CV, UNUSED|CLASS_FETCH|CONST|VAR) */
ZEND_VM_HANDLER(175, ZEND_FETCH_STATIC_PROP_RW, ANY, CLASS_FETCH, CACHE_SLOT)
{
	// 通过操作码扩展信息中的缓存位置，获取静态属性地址。
	// 调用方法 zend_fetch_static_prop_helper
	ZEND_VM_DISPATCH_TO_HELPER(zend_fetch_static_prop_helper, type, BP_VAR_RW);
}


// ing3, 访问静态属性，自动适配引用类型
/* No specialization for op_types (CONST|TMPVAR|CV, UNUSED|CLASS_FETCH|CONST|VAR) */
ZEND_VM_HANDLER(177, ZEND_FETCH_STATIC_PROP_FUNC_ARG, ANY, CLASS_FETCH, FETCH_REF|CACHE_SLOT)
{
	// 如果要求引用传递
	int fetch_type =
		(UNEXPECTED(ZEND_CALL_INFO(EX(call)) & ZEND_CALL_SEND_ARG_BY_REF)) ?
			BP_VAR_W : BP_VAR_R;
	// 通过操作码扩展信息中的缓存位置，获取静态属性地址。
	// 调用方法 zend_fetch_static_prop_helper
	ZEND_VM_DISPATCH_TO_HELPER(zend_fetch_static_prop_helper, type, fetch_type);
}

// ing3, 删除静态属性
/* No specialization for op_types (CONST|TMPVAR|CV, UNUSED|CLASS_FETCH|CONST|VAR) */
ZEND_VM_HANDLER(178, ZEND_FETCH_STATIC_PROP_UNSET, ANY, CLASS_FETCH, CACHE_SLOT)
{
	// 通过操作码扩展信息中的缓存位置，获取静态属性地址。
	// 调用方法 zend_fetch_static_prop_helper
	ZEND_VM_DISPATCH_TO_HELPER(zend_fetch_static_prop_helper, type, BP_VAR_UNSET);
}

// ing3, 查检静态属性 是否存在
/* No specialization for op_types (CONST|TMPVAR|CV, UNUSED|CLASS_FETCH|CONST|VAR) */
ZEND_VM_HANDLER(176, ZEND_FETCH_STATIC_PROP_IS, ANY, CLASS_FETCH, CACHE_SLOT)
{
	// 通过操作码扩展信息中的缓存位置，获取静态属性地址。
	// 调用方法 zend_fetch_static_prop_helper
	ZEND_VM_DISPATCH_TO_HELPER(zend_fetch_static_prop_helper, type, BP_VAR_IS);
}

// ing3,（各种类型的）像数组一样读取, 结果返回到操作码中，操作类型为 BP_VAR_R。op1:容器（各种类型），op2:偏移量
ZEND_VM_COLD_CONSTCONST_HANDLER(81, ZEND_FETCH_DIM_R, CONST|TMPVAR|CV, CONST|TMPVAR|CV)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *container, *dim, *value;

	// windows: 无操作
	SAVE_OPLINE();
	// 获取zval指针, UNUSED 返回null
	container = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 获取zval指针, UNUSED 返回null
	dim = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 如果 OP1 不是常量
	if (OP1_TYPE != IS_CONST) {
		// 如果 容器 类型是 数组
		if (EXPECTED(Z_TYPE_P(container) == IS_ARRAY)) {
// 创建跳转标签 fetch_dim_r_array
ZEND_VM_C_LABEL(fetch_dim_r_array):
			// 像数组一样读写，的内部实现。p1:哈希表，p2:索引，p3:索引变量类型，p4:操作类型
			value = zend_fetch_dimension_address_inner(Z_ARRVAL_P(container), dim, OP2_TYPE, BP_VAR_R EXECUTE_DATA_CC);
			// 复制引用目标 和 附加信息，并增加引用计数
			ZVAL_COPY_DEREF(EX_VAR(opline->result.var), value);
		// 否则 如果 容器 类型是 引用
		} else if (EXPECTED(Z_TYPE_P(container) == IS_REFERENCE)) {
			// 解引用
			container = Z_REFVAL_P(container);
			// 如果 容器 类型是 数组
			if (EXPECTED(Z_TYPE_P(container) == IS_ARRAY)) {
				// 跳转到指定标签 fetch_dim_r_array
				ZEND_VM_C_GOTO(fetch_dim_r_array);
			// 否则
			} else {
				// 跳转到指定标签 fetch_dim_r_slow
				ZEND_VM_C_GOTO(fetch_dim_r_slow);
			}
		// 否则
		} else {
// 创建跳转标签 fetch_dim_r_slow
ZEND_VM_C_LABEL(fetch_dim_r_slow):
			// 如果op2是常量 并且 额外类型为 ZEND_EXTRA_VALUE
			if (OP2_TYPE == IS_CONST && Z_EXTRA_P(dim) == ZEND_EXTRA_VALUE) {
				// 取得下一个 zval
				dim++;
			}
			// （哈希表以外的）像数组一样读取,结果返回到操作码中，操作类型为 BP_VAR_R。 
			zend_fetch_dimension_address_read_R_slow(container, dim OPLINE_CC EXECUTE_DATA_CC);
		}
	// 否则
	} else {
		//（各种类型的）像数组一样读取,结果返回到操作码中，操作类型为 BP_VAR_R。  p1:被读取变量，p2:索引，p3:索引类型
		zend_fetch_dimension_address_read_R(container, dim, OP2_TYPE OPLINE_CC EXECUTE_DATA_CC);
	}
	// 释放操作对象的附加变量
	FREE_OP2();
	// 释放操作对象的附加变量
	FREE_OP1();
	// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}


// ing3, 像数组一样读取地址，操作类型为 BP_VAR_W。op1:容器（各种类型），op2:偏移量
ZEND_VM_HANDLER(84, ZEND_FETCH_DIM_W, VAR|CV, CONST|TMPVAR|UNUSED|NEXT|CV)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *container;

	// windows: 无操作
	SAVE_OPLINE();
	// 获取zval指针, TMP/CONST/UNUSED 返回null，不支持 TMPVAR/TMPVARCV
	container = GET_OP1_ZVAL_PTR_PTR_UNDEF(BP_VAR_W);
	// 像数组一样读取地址，操作类型为 BP_VAR_W，p1：返回值，p2:容器（各种类型），p3:索引值，p4:索引类型
	// 获取zval指针, UNUSED 返回null
	zend_fetch_dimension_address_W(container, GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R), OP2_TYPE OPLINE_CC EXECUTE_DATA_CC);
	// 释放操作对象的附加变量
	FREE_OP2();
	// 如果 op1 是 普通变量
	if (OP1_TYPE == IS_VAR) {
		// p1中的引用实例 引用数-1（如果为0：目标关联到opline->result,解引间接引用 并 销毁 p1）
		FREE_VAR_PTR_AND_EXTRACT_RESULT_IF_NECESSARY(opline->op1.var);
	}
	// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, 像数组一样读取地址，操作类型为 BP_VAR_RW。op1:容器（各种类型），op2:偏移量
ZEND_VM_HANDLER(87, ZEND_FETCH_DIM_RW, VAR|CV, CONST|TMPVAR|UNUSED|NEXT|CV)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *container;

	// windows: 无操作
	SAVE_OPLINE();
	// 获取zval指针, TMP/CONST/UNUSED 返回null，不支持 TMPVAR/TMPVARCV
	container = GET_OP1_ZVAL_PTR_PTR_UNDEF(BP_VAR_RW);
	// 获取zval指针, UNUSED 返回null
	zend_fetch_dimension_address_RW(container, GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R), OP2_TYPE OPLINE_CC EXECUTE_DATA_CC);
	// 释放操作对象的附加变量
	FREE_OP2();
	// 如果 op1 是 普通变量
	if (OP1_TYPE == IS_VAR) {
		// p1中的引用实例 引用数-1（如果为0：目标关联到opline->result,解引间接引用 并 销毁 p1）
		FREE_VAR_PTR_AND_EXTRACT_RESULT_IF_NECESSARY(opline->op1.var);
	}
	// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}


// ing3,（各种类型的）像数组一样读取, 操作类型为 BP_VAR_IS。op1:容器，op2:偏移量
ZEND_VM_COLD_CONSTCONST_HANDLER(90, ZEND_FETCH_DIM_IS, CONST|TMPVAR|CV, CONST|TMPVAR|CV)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *container;

	// windows: 无操作
	SAVE_OPLINE();
	// 获取zval指针, UNUSED 返回null
	container = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_IS);
	//（各种类型的）像数组一样读取,结果返回到操作码中，操作类型为 BP_VAR_IS。  p1:被读取变量，p2:索引，p3:索引类型
	// 获取op2的zval指针, UNUSED 返回null
	zend_fetch_dimension_address_read_IS(container, GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R), OP2_TYPE OPLINE_CC EXECUTE_DATA_CC);
	// 释放操作对象的附加变量
	FREE_OP2();
	// 释放操作对象的附加变量
	FREE_OP1();
	// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, 报错：不可以在写入上下文中使用临时表达式
ZEND_VM_COLD_HELPER(zend_use_tmp_in_write_context_helper, ANY, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// windows: 无操作
	SAVE_OPLINE();
	// 报错：不可以在写入上下文中使用临时表达式
	zend_throw_error(NULL, "Cannot use temporary expression in write context");
	// 释放操作对象的附加变量
	FREE_OP2();
	// 释放操作对象的附加变量
	FREE_OP1();
	// 结果为 未定义
	ZVAL_UNDEF(EX_VAR(opline->result.var));
	// 主要是 return
	HANDLE_EXCEPTION();
}

// ing3, 报错：不可以使用空索引号来进行读取
ZEND_VM_COLD_HELPER(zend_use_undef_in_read_context_helper, ANY, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// windows: 无操作
	SAVE_OPLINE();
	zend_throw_error(NULL, "Cannot use [] for reading");
	// 释放操作对象的附加变量
	FREE_OP2();
	// 释放操作对象的附加变量
	FREE_OP1();
	// 结果为 未定义
	ZVAL_UNDEF(EX_VAR(opline->result.var));
	// 主要是 return
	HANDLE_EXCEPTION();
}

// ing3, 如果需要引用传递，跳到 ZEND_FETCH_DIM_W 否则跳到 ZEND_FETCH_DIM_R
ZEND_VM_COLD_CONSTCONST_HANDLER(93, ZEND_FETCH_DIM_FUNC_ARG, CONST|TMP|VAR|CV, CONST|TMPVAR|UNUSED|NEXT|CV)
{
// op1和op2 都是ANY。
#if !ZEND_VM_SPEC
	// const zend_op *opline = EX(opline);
	USE_OPLINE
#endif

	// 如果有引用传递的参数
	if (UNEXPECTED(ZEND_CALL_INFO(EX(call)) & ZEND_CALL_SEND_ARG_BY_REF)) {
		// 如果 op1是 常量或临时变量
		if ((OP1_TYPE & (IS_CONST|IS_TMP_VAR))) {
			// 报错：不可以在写入上下文中使用临时表达式
			// 调用方法 zend_use_tmp_in_write_context_helper
			ZEND_VM_DISPATCH_TO_HELPER(zend_use_tmp_in_write_context_helper);
		}
		// 使用写入模式访问
		// 跳转到操作码标签 ZEND_FETCH_DIM_W
		ZEND_VM_DISPATCH_TO_HANDLER(ZEND_FETCH_DIM_W);
	// 否则
	} else {
		// 如果 op2 类型是未定义
		if (OP2_TYPE == IS_UNUSED) {
			// 报错：不可以使用空索引号来进行读取
			// 调用方法 zend_use_undef_in_read_context_helper
			ZEND_VM_DISPATCH_TO_HELPER(zend_use_undef_in_read_context_helper);
		}
		// 使用读取模式访问
		// 跳转到操作码标签 ZEND_FETCH_DIM_R
		ZEND_VM_DISPATCH_TO_HANDLER(ZEND_FETCH_DIM_R);
	}
}

// ing3, 像数组一样删除元素
ZEND_VM_HANDLER(96, ZEND_FETCH_DIM_UNSET, VAR|CV, CONST|TMPVAR|CV)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *container;

	// windows: 无操作
	SAVE_OPLINE();
	// 获取zval指针, TMP/CONST/UNUSED 返回null，不支持 TMPVAR/TMPVARCV
	container = GET_OP1_ZVAL_PTR_PTR_UNDEF(BP_VAR_UNSET);
	
	// 像数组一样读取地址，操作类型为 BP_VAR_UNSET，p1：返回值，p2:容器（各种类型），p3:索引值，p4:索引类型
	// 获取zval指针, UNUSED 返回null
	zend_fetch_dimension_address_UNSET(container, GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R), OP2_TYPE OPLINE_CC EXECUTE_DATA_CC);
	
	// 释放操作对象的附加变量
	FREE_OP2();
	// 如果 op1 是 普通变量
	if (OP1_TYPE == IS_VAR) {
		// p1中的引用实例 引用数-1（如果为0：目标关联到opline->result,解引间接引用 并 销毁 p1）
		FREE_VAR_PTR_AND_EXTRACT_RESULT_IF_NECESSARY(opline->op1.var);
	}
	// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, 只读方式获取对象属性
ZEND_VM_HOT_OBJ_HANDLER(82, ZEND_FETCH_OBJ_R, CONST|TMPVAR|UNUSED|THIS|CV, CONST|TMPVAR|CV, CACHE_SLOT)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *container;
	void **cache_slot = NULL;

	// windows: 无操作
	SAVE_OPLINE();
	// 获取op1 的 zval指针, UNUSED 返回 $this
	container = GET_OP1_OBJ_ZVAL_PTR_UNDEF(BP_VAR_R);

	// 如果op1 是常量 或 （op1是未定义 并且 容器不是对象）
	if (OP1_TYPE == IS_CONST ||
	    (OP1_TYPE != IS_UNUSED && UNEXPECTED(Z_TYPE_P(container) != IS_OBJECT))) {
		// 为了break;
		do {
			// 如果 op1 是普通变量或编译变量 并且 op1是引用类型
			if ((OP1_TYPE & (IS_VAR|IS_CV)) && Z_ISREF_P(container)) {
				// 解引用
				container = Z_REFVAL_P(container);
				// 如果 容器 类型是 对象
				if (EXPECTED(Z_TYPE_P(container) == IS_OBJECT)) {
					// 跳出
					break;
				}
			}
			
			// 如果 op1 是 编译变量 并且 窗口类型是未定义
			if (OP1_TYPE == IS_CV && UNEXPECTED(Z_TYPE_P(container) == IS_UNDEF)) {
				// 报错：p1的变量（变量名）未定义, 并返未初始化zval
				ZVAL_UNDEFINED_OP1();
			}
			// 获取zval指针，UNUSED 类型返回null
			zend_wrong_property_read(container, GET_OP2_ZVAL_PTR(BP_VAR_R));
			/ 结果为null
			ZVAL_NULL(EX_VAR(opline->result.var));
			// 跳转到指定标签 fetch_obj_r_finish
			ZEND_VM_C_GOTO(fetch_obj_r_finish);
		} while (0);
	}

	// 到这里可以确定在对对象进行操作
	/* here we are sure we are dealing with an object */
	
	// 为了break，主要逻辑在这里
	do {
		// 取出对象
		zend_object *zobj = Z_OBJ_P(container);
		zend_string *name, *tmp_name;
		zval *retval;

		// 如果op2是常量
		if (OP2_TYPE == IS_CONST) {
			// 找到缓存位置
			cache_slot = CACHE_ADDR(opline->extended_value & ~ZEND_FETCH_REF /* FUNC_ARG fetch may contain it */);

			// 如果缓存位置存放的是当前对象的所属类
			// 返回数组的第一个元素					
			if (EXPECTED(zobj->ce == CACHED_PTR_EX(cache_slot))) {
				// 返回数组的第一个元素					
				uintptr_t prop_offset = (uintptr_t)CACHED_PTR_EX(cache_slot + 1);
				// 检验：属性序号有效 
				if (EXPECTED(IS_VALID_PROPERTY_OFFSET(prop_offset))) {
					// 取出属性值
					retval = OBJ_PROP(zobj, prop_offset);
					// 如果 返回值类 有效
					if (EXPECTED(Z_TYPE_INFO_P(retval) != IS_UNDEF)) {
						// op1和op2 都是ANY。 或 (op1 是临时变量 或 普通变量 )
						if (!ZEND_VM_SPEC || (OP1_TYPE & (IS_TMP_VAR|IS_VAR)) != 0) {
							// 跳转到指定标签 fetch_obj_r_copy
							ZEND_VM_C_GOTO(fetch_obj_r_copy);
						// 否则
						} else {
// 创建跳转标签 fetch_obj_r_fast_copy
ZEND_VM_C_LABEL(fetch_obj_r_fast_copy):
							// 复制引用目标 和 附加信息，并增加引用计数
							ZVAL_COPY_DEREF(EX_VAR(opline->result.var), retval);
							// opline + 1，到目标操作码并 return
							ZEND_VM_NEXT_OPCODE();
						}
					}
				// 如果对象有属性表
				} else if (EXPECTED(zobj->properties != NULL)) {
					// 
					// 获取zval指针，UNUSED 类型返回null
					name = Z_STR_P(GET_OP2_ZVAL_PTR(BP_VAR_R));
					// 检验：是未知的动态的属性编号，如果不是
					if (!IS_UNKNOWN_DYNAMIC_PROPERTY_OFFSET(prop_offset)) {
						// 解码 动态属性号，转成负数再-2
						uintptr_t idx = ZEND_DECODE_DYN_PROP_OFFSET(prop_offset);
						// 如果索引号在有效范围内
						if (EXPECTED(idx < zobj->properties->nNumUsed * sizeof(Bucket))) {
							// 找到属性 bucket
							Bucket *p = (Bucket*)((char*)zobj->properties->arData + idx);
							// 如果属性名是同一个字串 或 （哈希匹配，名字一样）
							if (EXPECTED(p->key == name) ||
							    (EXPECTED(p->h == ZSTR_H(name)) &&
							     EXPECTED(p->key != NULL) &&
							     EXPECTED(zend_string_equal_content(p->key, name)))) {
								// 这个值作为返回值
								retval = &p->val;
								// op1和op2 都是ANY。
								if (!ZEND_VM_SPEC || (OP1_TYPE & (IS_TMP_VAR|IS_VAR)) != 0) {
									// 跳转到指定标签 fetch_obj_r_copy
									ZEND_VM_C_GOTO(fetch_obj_r_copy);
								// 否则
								} else {
									// 跳转到指定标签 fetch_obj_r_fast_copy
									ZEND_VM_C_GOTO(fetch_obj_r_fast_copy);
								}
							}
						}
						// 缓存位置存放【动态属性编号】标记
						CACHE_PTR_EX(cache_slot + 1, (void*)ZEND_DYNAMIC_PROPERTY_OFFSET);
					}
					// 在属性表里用名称查找
					retval = zend_hash_find_known_hash(zobj->properties, name);
					// 如果找到属性
					if (EXPECTED(retval)) {
						// 属性偏移量
						uintptr_t idx = (char*)retval - (char*)zobj->properties->arData;
						// 把偏移量存到缓存里
						CACHE_PTR_EX(cache_slot + 1, (void*)ZEND_ENCODE_DYN_PROP_OFFSET(idx));
						// op1和op2 都是ANY。
						if (!ZEND_VM_SPEC || (OP1_TYPE & (IS_TMP_VAR|IS_VAR)) != 0) {
							// 跳转到指定标签 fetch_obj_r_copy
							ZEND_VM_C_GOTO(fetch_obj_r_copy);
						// 否则
						} else {
							// 跳转到指定标签 fetch_obj_r_fast_copy
							ZEND_VM_C_GOTO(fetch_obj_r_fast_copy);
						}
					}
				}
			}
			// 获取zval指针，UNUSED 类型返回null
			name = Z_STR_P(GET_OP2_ZVAL_PTR(BP_VAR_R));
		// 否则
		} else {
			// 获取zval指针，UNUSED 类型返回null
			name = zval_try_get_tmp_string(GET_OP2_ZVAL_PTR(BP_VAR_R), &tmp_name);
			// 如果名称无效
			if (UNEXPECTED(!name)) {
				// 结果为 未定义
				ZVAL_UNDEF(EX_VAR(opline->result.var));
				// 跳出
				break;
			}
		}
// 调试用
#if ZEND_DEBUG
		/* For non-standard object handlers, verify a declared property type in debug builds.
		 * Fetch prop_info before calling read_property(), as it may deallocate the object. */
		zend_property_info *prop_info = NULL;
		
		// 如果读取方法不是标准方法
		if (zobj->handlers->read_property != zend_std_read_property) {
			prop_info = zend_get_property_info(zobj->ce, name, /* silent */ true);
		}
#endif
		// 读取属性。p1:对象，p2:属性名，p3:类型，p4:缓存位置，p5:返回值（副本）
		// zend_std_read_property
		retval = zobj->handlers->read_property(zobj, name, BP_VAR_R, cache_slot, EX_VAR(opline->result.var));
// 调试用
#if ZEND_DEBUG
		if (!EG(exception) && prop_info && prop_info != ZEND_WRONG_PROPERTY_INFO
				&& ZEND_TYPE_IS_SET(prop_info->type)) {
			ZVAL_OPT_DEREF(retval);
			zend_verify_property_type(prop_info, retval, /* strict */ true);
		}
#endif

		// 如果 OP2 不是常量
		if (OP2_TYPE != IS_CONST) {
			// 释放临时名称
			zend_tmp_string_release(tmp_name);
		}
		// 如果返回值 和 结果变量不是同一个zval
		if (retval != EX_VAR(opline->result.var)) {
// 创建跳转标签 fetch_obj_r_copy
ZEND_VM_C_LABEL(fetch_obj_r_copy):
			// 复制引用目标 和 附加信息，并增加引用计数
			ZVAL_COPY_DEREF(EX_VAR(opline->result.var), retval);
		} else if (UNEXPECTED(Z_ISREF_P(retval))) {
			// 解包引用：p1引用数为1？p1赋值为未定义：减少p1引用次数，p1引用目标复制给p1
			zend_unwrap_reference(retval);
		}
	} while (0);

// 创建跳转标签 fetch_obj_r_finish
ZEND_VM_C_LABEL(fetch_obj_r_finish):
	// 释放操作对象的附加变量
	FREE_OP2();
	// 释放操作对象的附加变量
	FREE_OP1();
	// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, 创建对象属性
ZEND_VM_HANDLER(85, ZEND_FETCH_OBJ_W, VAR|UNUSED|THIS|CV, CONST|TMPVAR|CV, FETCH_REF|DIM_WRITE|CACHE_SLOT)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *property, *container, *result;

	// windows: 无操作
	SAVE_OPLINE();

	// 获取zval指针, TMP/CONST返回null, UNUSED 返回 $this，不支持 TMPVAR/TMPVARCV。CV 直接返回。
	container = GET_OP1_OBJ_ZVAL_PTR_PTR_UNDEF(BP_VAR_W);
	// 获取zval指针，UNUSED 类型返回null
	property = GET_OP2_ZVAL_PTR(BP_VAR_R);
	// 在执行数据中取出 指定序号的变量
	result = EX_VAR(opline->result.var);
	// 获取属性的地址。p1:返回结果，p2:容器zval，p3:容器运算对象类型，p4:属性值，p5:属性运算对象类型
	// p6:缓存位置，p7:操作类型，p8:flags，p9:是否初始化未定义类型
	zend_fetch_property_address(
		result, container, OP1_TYPE, property, OP2_TYPE,
		((OP2_TYPE == IS_CONST) ? CACHE_ADDR(opline->extended_value & ~ZEND_FETCH_OBJ_FLAGS) : NULL),
		BP_VAR_W, opline->extended_value, 1 OPLINE_CC EXECUTE_DATA_CC);
	// 释放操作对象的附加变量
	FREE_OP2();
	// 如果 op1 是 普通变量
	if (OP1_TYPE == IS_VAR) {
		// p1中的引用实例 引用数-1（如果为0：目标关联到opline->result,解引间接引用 并 销毁 p1）
		FREE_VAR_PTR_AND_EXTRACT_RESULT_IF_NECESSARY(opline->op1.var);
	}
	// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, 更新对象属性
ZEND_VM_HANDLER(88, ZEND_FETCH_OBJ_RW, VAR|UNUSED|THIS|CV, CONST|TMPVAR|CV, CACHE_SLOT)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *property, *container, *result;

	// windows: 无操作
	SAVE_OPLINE();
	// 获取zval指针, TMP/CONST返回null, UNUSED 返回 $this，不支持 TMPVAR/TMPVARCV。CV 直接返回。
	container = GET_OP1_OBJ_ZVAL_PTR_PTR_UNDEF(BP_VAR_RW);
	// 获取zval指针，UNUSED 类型返回null
	property = GET_OP2_ZVAL_PTR(BP_VAR_R);
	// 在执行数据中取出 指定序号的变量
	result = EX_VAR(opline->result.var);
	// 获取属性的地址。p1:返回结果，p2:容器zval，p3:容器运算对象类型，p4:属性值，p5:属性运算对象类型
	// p6:缓存位置，p7:操作类型，p8:flags，p9:是否初始化未定义类型
	zend_fetch_property_address(result, container, OP1_TYPE, property, OP2_TYPE, ((OP2_TYPE == IS_CONST) ? CACHE_ADDR(opline->extended_value) : NULL), BP_VAR_RW, 0, 1 OPLINE_CC EXECUTE_DATA_CC);
	// 释放操作对象的附加变量
	FREE_OP2();
	// 如果 op1 是 普通变量
	if (OP1_TYPE == IS_VAR) {
		// p1中的引用实例 引用数-1（如果为0：目标关联到opline->result,解引间接引用 并 销毁 p1）
		FREE_VAR_PTR_AND_EXTRACT_RESULT_IF_NECESSARY(opline->op1.var);
	}
	// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, 检查对象属性是否存在
ZEND_VM_COLD_CONST_HANDLER(91, ZEND_FETCH_OBJ_IS, CONST|TMPVAR|UNUSED|THIS|CV, CONST|TMPVAR|CV, CACHE_SLOT)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *container;
	void **cache_slot = NULL;

	// windows: 无操作
	SAVE_OPLINE();
	// 获取zval指针, UNUSED 返回 $this，不支持 TMPVARCV
	container = GET_OP1_OBJ_ZVAL_PTR(BP_VAR_IS);
	// 如果op1是常量 或 （op2有效 并且 容器不是对象 ）
	if (OP1_TYPE == IS_CONST ||
	    (OP1_TYPE != IS_UNUSED && UNEXPECTED(Z_TYPE_P(container) != IS_OBJECT))) {
		do {
			// 如果 op1 是普通变量或编译变量 并且 op1是引用类型
			if ((OP1_TYPE & (IS_VAR|IS_CV)) && Z_ISREF_P(container)) {
				// 解引用
				container = Z_REFVAL_P(container);
				// 如果 容器 类型是 对象
				if (EXPECTED(Z_TYPE_P(container) == IS_OBJECT)) {
					break;
				}
			}
			// 如果 op2 是 编译变量 并且 zval类型为 未定义
			if (OP2_TYPE == IS_CV && Z_TYPE_P(EX_VAR(opline->op2.var)) == IS_UNDEF) {
				// 报错：p2的变量（变量名）未定义, 并返未初始化zval
				ZVAL_UNDEFINED_OP2();
			}
			// 结果为null
			ZVAL_NULL(EX_VAR(opline->result.var));
			// 跳转到指定标签 fetch_obj_is_finish
			ZEND_VM_C_GOTO(fetch_obj_is_finish);
		} while (0);
	}

	// 这里可以确定在操作对象
	/* here we are sure we are dealing with an object */
	do {
		// 窗口对象
		zend_object *zobj = Z_OBJ_P(container);
		zend_string *name, *tmp_name;
		zval *retval;

		// 如果属性名是常量
		if (OP2_TYPE == IS_CONST) {
			cache_slot = CACHE_ADDR(opline->extended_value);
			// 返回数组的第一个元素
			if (EXPECTED(zobj->ce == CACHED_PTR_EX(cache_slot))) {
				// 返回数组的第一个元素
				uintptr_t prop_offset = (uintptr_t)CACHED_PTR_EX(cache_slot + 1);
				// 检验：属性序号有效 
				if (EXPECTED(IS_VALID_PROPERTY_OFFSET(prop_offset))) {
					// 取得属性值
					retval = OBJ_PROP(zobj, prop_offset);
					// 如果值不是未定义
					if (EXPECTED(Z_TYPE_P(retval) != IS_UNDEF)) {
						// op1和op2 都是ANY。
						if (!ZEND_VM_SPEC || (OP1_TYPE & (IS_TMP_VAR|IS_VAR)) != 0) {
							// 跳转到指定标签 fetch_obj_is_copy
							ZEND_VM_C_GOTO(fetch_obj_is_copy);
						// 否则	
						} else {
// 创建跳转标签 fetch_obj_is_fast_copy
ZEND_VM_C_LABEL(fetch_obj_is_fast_copy):
							// 复制引用目标 和 附加信息，并增加引用计数
							ZVAL_COPY_DEREF(EX_VAR(opline->result.var), retval);
							// opline + 1，到目标操作码并 return
							ZEND_VM_NEXT_OPCODE();
						}
					}
				// 如果有属性表
				} else if (EXPECTED(zobj->properties != NULL)) {
					// 获取zval指针，UNUSED 类型返回null
					name = Z_STR_P(GET_OP2_ZVAL_PTR(BP_VAR_R));
					// 检验：是未知的动态的属性编号，如果不是
					if (!IS_UNKNOWN_DYNAMIC_PROPERTY_OFFSET(prop_offset)) {
						// 解码 动态属性号，转成负数再-2
						uintptr_t idx = ZEND_DECODE_DYN_PROP_OFFSET(prop_offset);
						// 如果偏移量在有效匹配
						if (EXPECTED(idx < zobj->properties->nNumUsed * sizeof(Bucket))) {
							Bucket *p = (Bucket*)((char*)zobj->properties->arData + idx);
							// 如果和键名指针相同 或 （哈希值相同 并且 key匹配）
							if (EXPECTED(p->key == name) ||
							    (EXPECTED(p->h == ZSTR_H(name)) &&
							     EXPECTED(p->key != NULL) &&
							     EXPECTED(zend_string_equal_content(p->key, name)))) {
								// 使用属性值作返回值
								retval = &p->val;
								// op1和op2 都是ANY
								if (!ZEND_VM_SPEC || (OP1_TYPE & (IS_TMP_VAR|IS_VAR)) != 0) {
									// 跳转到指定标签 fetch_obj_is_copy
									ZEND_VM_C_GOTO(fetch_obj_is_copy);
								// 否则
								} else {
									// 跳转到指定标签 fetch_obj_is_fast_copy
									ZEND_VM_C_GOTO(fetch_obj_is_fast_copy);
								}
							}
						}
						// 缓存位置存放【动态属性编号】标记
						CACHE_PTR_EX(cache_slot + 1, (void*)ZEND_DYNAMIC_PROPERTY_OFFSET);
					}
					// 用名称查找 属性
					retval = zend_hash_find_known_hash(zobj->properties, name);
					// 如果找到属性
					if (EXPECTED(retval)) {
						// 取出此属性的偏移量
						uintptr_t idx = (char*)retval - (char*)zobj->properties->arData;
						// 把偏移量记录到缓存里
						CACHE_PTR_EX(cache_slot + 1, (void*)ZEND_ENCODE_DYN_PROP_OFFSET(idx));
						// op1和op2 都是ANY
						if (!ZEND_VM_SPEC || (OP1_TYPE & (IS_TMP_VAR|IS_VAR)) != 0) {
							// 跳转到指定标签 fetch_obj_is_copy
							ZEND_VM_C_GOTO(fetch_obj_is_copy);
						// 否则
						} else {
							// 跳转到指定标签 fetch_obj_is_fast_copy
							ZEND_VM_C_GOTO(fetch_obj_is_fast_copy);
						}
					}
				}
			}
			// 获取zval指针，UNUSED 类型返回null
			name = Z_STR_P(GET_OP2_ZVAL_PTR(BP_VAR_R));
		// 否则，属性名不是常量
		} else {
			// 获取zval指针，UNUSED 类型返回null
			name = zval_try_get_tmp_string(GET_OP2_ZVAL_PTR(BP_VAR_R), &tmp_name);
			// 如果获取属性名失败
			if (UNEXPECTED(!name)) {
				// 结果为 未定义
				ZVAL_UNDEF(EX_VAR(opline->result.var));
				// 跳出
				break;
			}
		}

		// 读取属性。p1:对象，p2:属性名，p3:类型，p4:缓存位置，p5:返回值（副本）
		// zend_std_read_property
		retval = zobj->handlers->read_property(zobj, name, BP_VAR_IS, cache_slot, EX_VAR(opline->result.var));

		// 如果 OP2 不是常量
		if (OP2_TYPE != IS_CONST) {
			// 释放临时名称
			zend_tmp_string_release(tmp_name);
		}

		// 如果结果 结果值不是此变量
		if (retval != EX_VAR(opline->result.var)) {
// 创建跳转标签 fetch_obj_is_copy
ZEND_VM_C_LABEL(fetch_obj_is_copy):
			// 复制引用目标 和 附加信息，并增加引用计数
			ZVAL_COPY_DEREF(EX_VAR(opline->result.var), retval);
		// 如果值就是此变量，并且是引用
		} else if (UNEXPECTED(Z_ISREF_P(retval))) {
			// 解包引用：p1引用数为1？p1赋值为未定义：减少p1引用次数，p1引用目标复制给p1
			zend_unwrap_reference(retval);
		}
	} while (0);

// 创建跳转标签 fetch_obj_is_finish
ZEND_VM_C_LABEL(fetch_obj_is_finish):
	// 释放操作对象的附加变量
	FREE_OP2();
	// 释放操作对象的附加变量
	FREE_OP1();
	// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, 需要引用传递 跳到 ZEND_FETCH_OBJ_W 否则跳到 ZEND_FETCH_OBJ_R
ZEND_VM_COLD_CONST_HANDLER(94, ZEND_FETCH_OBJ_FUNC_ARG, CONST|TMP|VAR|UNUSED|THIS|CV, CONST|TMPVAR|CV, FETCH_REF|CACHE_SLOT)
{
// op1和op2 都是ANY。
#if !ZEND_VM_SPEC
	// const zend_op *opline = EX(opline);
	USE_OPLINE
#endif

	// 如果要求引用传递参数
	if (UNEXPECTED(ZEND_CALL_INFO(EX(call)) & ZEND_CALL_SEND_ARG_BY_REF)) {
		// 类似 FETCH_OBJ_W
		/* Behave like FETCH_OBJ_W */
		// 如果op1是常量 或 临时变量
		if ((OP1_TYPE & (IS_CONST|IS_TMP_VAR))) {
			// 报错：不可以在写入上下文中使用临时表达式
			// 调用方法 zend_use_tmp_in_write_context_helper
			ZEND_VM_DISPATCH_TO_HELPER(zend_use_tmp_in_write_context_helper);
		}
		// 跳转到操作码标签 ZEND_FETCH_OBJ_W
		ZEND_VM_DISPATCH_TO_HANDLER(ZEND_FETCH_OBJ_W);
	// 否则
	} else {
		// 跳转到操作码标签 ZEND_FETCH_OBJ_R
		ZEND_VM_DISPATCH_TO_HANDLER(ZEND_FETCH_OBJ_R);
	}
}

// ing3, 删除对象属性
ZEND_VM_HANDLER(97, ZEND_FETCH_OBJ_UNSET, VAR|UNUSED|THIS|CV, CONST|TMPVAR|CV, CACHE_SLOT)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *container, *property, *result;

	// windows: 无操作
	SAVE_OPLINE();
	// 获取zval指针, TMP/CONST返回null, UNUSED 返回 $this，不支持 TMPVAR/TMPVARCV。CV 直接返回。
	container = GET_OP1_OBJ_ZVAL_PTR_PTR_UNDEF(BP_VAR_UNSET);
	// 获取zval指针，UNUSED 类型返回null
	property = GET_OP2_ZVAL_PTR(BP_VAR_R);
	// 在执行数据中取出 指定序号的变量
	result = EX_VAR(opline->result.var);
	// 获取属性的地址。p1:返回结果，p2:容器zval，p3:容器运算对象类型，p4:属性值，p5:属性运算对象类型
	// p6:缓存位置，p7:操作类型，p8:flags，p9:是否初始化未定义类型
	zend_fetch_property_address(result, container, OP1_TYPE, property, OP2_TYPE, ((OP2_TYPE == IS_CONST) ? CACHE_ADDR(opline->extended_value) : NULL), BP_VAR_UNSET, 0, 1 OPLINE_CC EXECUTE_DATA_CC);
	// 释放操作对象的附加变量
	FREE_OP2();
	// 如果 op1 是 普通变量
	if (OP1_TYPE == IS_VAR) {
		// p1中的引用实例 引用数-1（如果为0：目标关联到opline->result,解引间接引用 并 销毁 p1）
		FREE_VAR_PTR_AND_EXTRACT_RESULT_IF_NECESSARY(opline->op1.var);
	}
	// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3,（字串以外类型的）像数组一样读取
ZEND_VM_HANDLER(98, ZEND_FETCH_LIST_R, CONST|TMPVARCV, CONST|TMPVAR|CV)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *container;

	// windows: 无操作
	SAVE_OPLINE();
	// 获取zval指针, UNUSED 返回null
	container = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// （字串以外类型的）像数组一样读取,结果返回到操作码中，操作类型为 BP_VAR_R。  p1:被读取变量，p2:索引，p3:索引类型
	// 获取zval指针, UNUSED 返回null
	zend_fetch_dimension_address_LIST_r(container, GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R), OP2_TYPE OPLINE_CC EXECUTE_DATA_CC);
	// 释放操作对象的附加变量
	FREE_OP2();
	// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, 用维度访问写入
ZEND_VM_HANDLER(155, ZEND_FETCH_LIST_W, VAR, CONST|TMPVAR|CV)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *container, *dim;

	// windows: 无操作
	SAVE_OPLINE();
	// 获取zval指针, TMP/CONST/UNUSED 返回null，不支持 TMPVAR/TMPVARCV
	container = GET_OP1_ZVAL_PTR_PTR_UNDEF(BP_VAR_W);
	// 获取zval指针, UNUSED 返回null
	dim = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);

	// 如果op1是变量 并且 op1不是引用类型 并且 container不是引用类型
	if (OP1_TYPE == IS_VAR
		&& Z_TYPE_P(EX_VAR(opline->op1.var)) != IS_INDIRECT
		&& UNEXPECTED(!Z_ISREF_P(container))
	) {
		// 报错：尝试向非引用值中 放入 引用实例
		zend_error(E_NOTICE, "Attempting to set reference to non referenceable value");
		// （字串以外类型的）像数组一样读取,结果返回到操作码中，操作类型为 BP_VAR_R。 
		// p1:被读取变量，p2:索引，p3:索引类型
		zend_fetch_dimension_address_LIST_r(container, dim, OP2_TYPE OPLINE_CC EXECUTE_DATA_CC);
	// 否则
	} else {
		// 像数组一样读取地址，操作类型为 BP_VAR_W，p1：返回值，p2:容器（各种类型），p3:索引值，p4:索引类型
		zend_fetch_dimension_address_W(container, dim, OP2_TYPE OPLINE_CC EXECUTE_DATA_CC);
	}

	// 释放操作对象的附加变量
	FREE_OP2();
	// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing2, 对象属性赋值
ZEND_VM_HANDLER(24, ZEND_ASSIGN_OBJ, VAR|UNUSED|THIS|CV, CONST|TMPVAR|CV, CACHE_SLOT, SPEC(OP_DATA=CONST|TMP|VAR|CV))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *object, *value, tmp;
	zend_object *zobj;
	zend_string *name, *tmp_name;

	// windows: 无操作
	SAVE_OPLINE();
	// 获取zval指针, TMP/CONST返回null, UNUSED 返回 $this，不支持 TMPVAR/TMPVARCV。CV 直接返回。
	object = GET_OP1_OBJ_ZVAL_PTR_PTR_UNDEF(BP_VAR_W);
	// 取得下一个操作码的 zval。UNUSED返回null, 不支持 TMPVARCV。
	value = GET_OP_DATA_ZVAL_PTR(BP_VAR_R);

	// 如果op1有效 并且 op1类型不是对象
	if (OP1_TYPE != IS_UNUSED && UNEXPECTED(Z_TYPE_P(object) != IS_OBJECT)) {
		// 如果对象是引用类型 并且 引用目标是对象
		if (Z_ISREF_P(object) && Z_TYPE_P(Z_REFVAL_P(object)) == IS_OBJECT) {
			// 解引用
			object = Z_REFVAL_P(object);
			// 跳转到指定标签 assign_object
			ZEND_VM_C_GOTO(assign_object);
		}
		// 获取zval指针，UNUSED 类型返回null
		zend_throw_non_object_error(object, GET_OP2_ZVAL_PTR(BP_VAR_R) OPLINE_CC EXECUTE_DATA_CC);
		// 未初始化zval
		value = &EG(uninitialized_zval);
		// 跳转到指定标签 free_and_exit_assign_obj
		ZEND_VM_C_GOTO(free_and_exit_assign_obj);
	}

// 创建跳转标签 assign_object
ZEND_VM_C_LABEL(assign_object):
	// 取出对象
	zobj = Z_OBJ_P(object);
	// 如果op2是常量
	if (OP2_TYPE == IS_CONST) {
		// 通过偏移量 在运行时缓存中获取一个 (void**) 中的第一个元素
		if (EXPECTED(zobj->ce == CACHED_PTR(opline->extended_value))) {
			// 取得缓存位置
			void **cache_slot = CACHE_ADDR(opline->extended_value);
			// 返回数组的第一个元素
			uintptr_t prop_offset = (uintptr_t)CACHED_PTR_EX(cache_slot + 1);
			// 取出 对象
			zend_object *zobj = Z_OBJ_P(object);
			zval *property_val;

			// 检验：属性序号有效 
			if (EXPECTED(IS_VALID_PROPERTY_OFFSET(prop_offset))) {
				// 取得属性
				property_val = OBJ_PROP(zobj, prop_offset);
				// 如果值无效
				if (Z_TYPE_P(property_val) != IS_UNDEF) {
					// 返回数组的第一个元素
					zend_property_info *prop_info = (zend_property_info*) CACHED_PTR_EX(cache_slot + 2);

					// 如果属性信息有效
					if (UNEXPECTED(prop_info != NULL)) {
						// 给有类型的属性 赋值。p1:属性信息，p2:属性值zval，p3:新值zval
						value = zend_assign_to_typed_prop(prop_info, property_val, value EXECUTE_DATA_CC);
						// 跳转到指定标签 free_and_exit_assign_obj
						ZEND_VM_C_GOTO(free_and_exit_assign_obj);
					// 否则
					} else {
// 创建跳转标签 fast_assign_obj
ZEND_VM_C_LABEL(fast_assign_obj):
						// 给变量赋值，p1:变量，p2:值，p3:值的变量类型（常量，变量，编译变量），p4:是否严格类型
						// 某执行上下文所属函数 的参数是否使用严格类型限制
						value = zend_assign_to_variable(property_val, value, OP_DATA_TYPE, EX_USES_STRICT_TYPES());
						// 如果 操作码的运算结果有效(不是IS_UNUSED)
						if (UNEXPECTED(RETURN_VALUE_USED(opline))) {
							// 值 复制给 结果 
							ZVAL_COPY(EX_VAR(opline->result.var), value);
						}
						// 跳转到指定标签 exit_assign_obj
						ZEND_VM_C_GOTO(exit_assign_obj);
					}
				}
			// 否则，属性序号无效
			} else {
				// 获取zval指针，UNUSED 类型返回null
				name = Z_STR_P(GET_OP2_ZVAL_PTR(BP_VAR_R));
				// 如果有属性表
				if (EXPECTED(zobj->properties != NULL)) {
					// 如果属性表引用数>1
					if (UNEXPECTED(GC_REFCOUNT(zobj->properties) > 1)) {
						// 如果属性表不是 不可更改
						if (EXPECTED(!(GC_FLAGS(zobj->properties) & IS_ARRAY_IMMUTABLE))) {
							// 引用次数-1
							GC_DELREF(zobj->properties);
						}
						// 属性表创建副本，使用副本
						zobj->properties = zend_array_dup(zobj->properties);
					}
					// 取得已知名字的属性
					property_val = zend_hash_find_known_hash(zobj->properties, name);
					// 如果有属性值
					if (property_val) {
						// 跳转到指定标签 fast_assign_obj
						ZEND_VM_C_GOTO(fast_assign_obj);
					}
				}

				// 如果没有 __set 方法并且 有【允许动态属性】标记
				if (!zobj->ce->__set && (zobj->ce->ce_flags & ZEND_ACC_ALLOW_DYNAMIC_PROPERTIES)) {
					// 如果没有属性表
					if (EXPECTED(zobj->properties == NULL)) {
						// 重建属性表
						rebuild_object_properties(zobj);
					}
					// 如果操作对象是常量
					if (OP_DATA_TYPE == IS_CONST) {
						// 如果是可以计数类型
						if (UNEXPECTED(Z_OPT_REFCOUNTED_P(value))) {
							// 引用次数+1
							Z_ADDREF_P(value);
						}
					// 如果操作数据不是临时变量
					} else if (OP_DATA_TYPE != IS_TMP_VAR) {
						// 如果值是引用类型
						if (Z_ISREF_P(value)) {
							// 如果操作数据是 变量
							if (OP_DATA_TYPE == IS_VAR) {
								// 取出引用实例
								zend_reference *ref = Z_REF_P(value);
								// 引用次数-1，如果为0
								if (GC_DELREF(ref) == 0) {
									// value引用目标复制给 临时变量
									ZVAL_COPY_VALUE(&tmp, Z_REFVAL_P(value));
									// 释放引用实例
									efree_size(ref, sizeof(zend_reference));
									// 值为临时变量
									value = &tmp;
								// 否则
								} else {
									// 解引用
									value = Z_REFVAL_P(value);
									// 增加引用次数
									Z_TRY_ADDREF_P(value);
								}
							// 否则
							} else {
								// 解引用
								value = Z_REFVAL_P(value);
								// 增加引用次数
								Z_TRY_ADDREF_P(value);
							}
						// 如果操作数据是 编译变量
						} else if (OP_DATA_TYPE == IS_CV) {
							// 增加引用次数
							Z_TRY_ADDREF_P(value);
						}
					// 这里原来就是这样的 对应上面的 if (Z_ISREF_P(value)) {
						}
					
					//
					//	哈希表里添加新元素
					zend_hash_add_new(zobj->properties, name, value);
					// 如果 操作码的运算结果有效(不是IS_UNUSED)
					if (UNEXPECTED(RETURN_VALUE_USED(opline))) {
						// 值复制到结果里
						ZVAL_COPY(EX_VAR(opline->result.var), value);
					}
					// 跳转到指定标签 exit_assign_obj
					ZEND_VM_C_GOTO(exit_assign_obj);
				}
			}
		}
		// 获取zval指针，UNUSED 类型返回null
		name = Z_STR_P(GET_OP2_ZVAL_PTR(BP_VAR_R));
	// 否则
	} else {
		// 获取zval指针，UNUSED 类型返回null
		name = zval_try_get_tmp_string(GET_OP2_ZVAL_PTR(BP_VAR_R), &tmp_name);
		// 如果没获取到
		if (UNEXPECTED(!name)) {
			// 释放下一个操作码的 op1。CONST/UNUSED/CV 免操作。不支持 TMPVARCV。
			FREE_OP_DATA();
			// 如果 opline 结果类型是 变量或临时变量，把结果 zval 标记为未定义
			UNDEF_RESULT();
			// 跳转到指定标签 exit_assign_obj
			ZEND_VM_C_GOTO(exit_assign_obj);
		}
	}

	// 如果数据是 编译变量或 变量
	if (OP_DATA_TYPE == IS_CV || OP_DATA_TYPE == IS_VAR) {
		// 减少引用次数
		ZVAL_DEREF(value);
	}
	
	// 写入属性，p1:对象，p2:属性名，p3:属性值，p4:缓存位置
	// zend_std_write_property 
	value = zobj->handlers->write_property(zobj, name, value, (OP2_TYPE == IS_CONST) ? CACHE_ADDR(opline->extended_value) : NULL);

	// 如果 OP2 不是常量
	if (OP2_TYPE != IS_CONST) {
		// 释放临时名称
		zend_tmp_string_release(tmp_name);
	}

// 创建跳转标签 free_and_exit_assign_obj
ZEND_VM_C_LABEL(free_and_exit_assign_obj):
	// 如果 操作码的运算结果有效(不是IS_UNUSED)
	if (UNEXPECTED(RETURN_VALUE_USED(opline))) {
		// 复制引用目标 和 附加信息，并增加引用计数
		ZVAL_COPY_DEREF(EX_VAR(opline->result.var), value);
	}
	// 释放下一个操作码的 op1。CONST/UNUSED/CV 免操作。不支持 TMPVARCV。
	FREE_OP_DATA();
// 创建跳转标签 exit_assign_obj
ZEND_VM_C_LABEL(exit_assign_obj):
	// 释放操作对象的附加变量
	FREE_OP2();
	// 释放操作对象的附加变量
	FREE_OP1();
	
	// assign_obj 有两个操作码
	/* assign_obj has two opcodes! */
	// 偏移到目标操作码并 return ,p1:是否检查异常？使用 EX(opline)  ：使用 opline 。p2:偏移量
	ZEND_VM_NEXT_OPCODE_EX(1, 2);
}

// ing3, 给静态属性赋值
/* No specialization for op_types (CONST|TMPVAR|CV, UNUSED|CONST|VAR) */
ZEND_VM_HANDLER(25, ZEND_ASSIGN_STATIC_PROP, ANY, ANY, CACHE_SLOT, SPEC(OP_DATA=CONST|TMP|VAR|CV))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *prop, *value;
	zend_property_info *prop_info;

	// windows: 无操作
	SAVE_OPLINE();

	// 通过缓存位置和类型，返回静态属性地址。p1:返回属性值，p2:返回属性信息，p3:缓存位置，p4:获取类型，p5:falgs
	if (zend_fetch_static_property_address(&prop, &prop_info, opline->extended_value, BP_VAR_W, 0 OPLINE_CC EXECUTE_DATA_CC) != SUCCESS) {
		// 释放下一个操作码的 op1。CONST/UNUSED/CV 免操作。不支持 TMPVARCV。
		FREE_OP_DATA();
		// 如果 opline 结果类型是 变量或临时变量，把结果 zval 标记为未定义
		UNDEF_RESULT();
		// 主要是 return
		HANDLE_EXCEPTION();
	}
	// 取得下一个操作码的 zval。UNUSED返回null, 不支持 TMPVARCV。
	value = GET_OP_DATA_ZVAL_PTR(BP_VAR_R);

	// 如果类型存在
	if (UNEXPECTED(ZEND_TYPE_IS_SET(prop_info->type))) {
		// 给有类型的属性 赋值。p1:属性信息，p2:属性值zval，p3:新值zval
		value = zend_assign_to_typed_prop(prop_info, prop, value EXECUTE_DATA_CC);
		// 释放下一个操作码的 op1。CONST/UNUSED/CV 免操作。不支持 TMPVARCV。
		FREE_OP_DATA();
	// 否则
	} else {
		// 给变量赋值，p1:变量，p2:值，p3:值的变量类型（常量，变量，编译变量），p4:是否严格类型
		// 某执行上下文所属函数 的参数是否使用严格类型限制
		value = zend_assign_to_variable(prop, value, OP_DATA_TYPE, EX_USES_STRICT_TYPES());
	}

	// 如果 操作码的运算结果有效(不是IS_UNUSED)
	if (UNEXPECTED(RETURN_VALUE_USED(opline))) {
		// 值复制给结果
		ZVAL_COPY(EX_VAR(opline->result.var), value);
	}

	// 静态属性赋值有两个操作码
	/* assign_static_prop has two opcodes! */
	
	// 偏移到目标操作码并 return ,p1:是否检查异常？使用 EX(opline)  ：使用 opline 。p2:偏移量
	ZEND_VM_NEXT_OPCODE_EX(1, 2);
}

// ing2, 维度访问赋值
ZEND_VM_HANDLER(23, ZEND_ASSIGN_DIM, VAR|CV, CONST|TMPVAR|UNUSED|NEXT|CV, SPEC(OP_DATA=CONST|TMP|VAR|CV))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *object_ptr, *orig_object_ptr;
	zval *value;
	zval *variable_ptr;
	zval *dim;

	// windows: 无操作
	SAVE_OPLINE();
	// op1 源对象和对象是同一个
	// 获取zval指针, TMP/CONST/UNUSED 返回null，不支持 TMPVAR/TMPVARCV
	orig_object_ptr = object_ptr = GET_OP1_ZVAL_PTR_PTR_UNDEF(BP_VAR_W);

	// 如果对象是数组
	if (EXPECTED(Z_TYPE_P(object_ptr) == IS_ARRAY)) {
// 创建跳转标签 try_assign_dim_array
ZEND_VM_C_LABEL(try_assign_dim_array):
		// 给数组创建副本，使用副本
		SEPARATE_ARRAY(object_ptr);
		// 如果 op2 类型是未定义
		if (OP2_TYPE == IS_UNUSED) {
			// 取得下一个操作码的 zval。UNUSED返回null, CV/TMPVARCV 直接返回
			value = GET_OP_DATA_ZVAL_PTR_UNDEF(BP_VAR_R);
			// 如果操作数据是 编译变量 并且 执行数据值为 未定义
			if (OP_DATA_TYPE == IS_CV && UNEXPECTED(Z_TYPE_P(value) == IS_UNDEF)) {
				// 取出数组
				HashTable *ht = Z_ARRVAL_P(object_ptr);
				// 如果哈希表不是不可更改
				if (!(GC_FLAGS(ht) & IS_ARRAY_IMMUTABLE)) {
					// 增加引用次数
					GC_ADDREF(ht);
				}
				// 报错：未定义的变量（变量名）, 并返未初始化zval, EXECUTE_DATA_DC 传入也用不到
				value = zval_undefined_cv((opline+1)->op1.var EXECUTE_DATA_CC);
				// 引用次数-1，如果为0
				if (!(GC_FLAGS(ht) & IS_ARRAY_IMMUTABLE) && !GC_DELREF(ht)) {
					// 销毁哈希表
					zend_array_destroy(ht);
					// 跳转到指定标签 assign_dim_error
					ZEND_VM_C_GOTO(assign_dim_error);
				}
			}
			// 如果 操作数据类型是编译变量 或 变量
			if (OP_DATA_TYPE == IS_CV || OP_DATA_TYPE == IS_VAR) {
				// 值解引用
				ZVAL_DEREF(value);
			}
			// 在哈希表中插入新元素
			value = zend_hash_next_index_insert(Z_ARRVAL_P(object_ptr), value);
			// 如果插入失败
			if (UNEXPECTED(value == NULL)) {
				// 报错：添加元素失败，下一个元素已经存在
				zend_cannot_add_element();
				// 跳转到指定标签 assign_dim_error
				ZEND_VM_C_GOTO(assign_dim_error);
			// 如果操作数据类型是 编译变量
			} else if (OP_DATA_TYPE == IS_CV) {
				// 如果是 可计数类型
				if (Z_REFCOUNTED_P(value)) {
					// 引用次数+1
					Z_ADDREF_P(value);
				}
			// 如果 操作数据类型是 变量
			} else if (OP_DATA_TYPE == IS_VAR) {
				// 在执行数据中取出 指定序号的变量
				zval *free_op_data = EX_VAR((opline+1)->op1.var);
				// 如果 是引用哦个大
				if (Z_ISREF_P(free_op_data)) {
					// 如果是 可计数类型
					if (Z_REFCOUNTED_P(value)) {
						// 引用次数+1
						Z_ADDREF_P(value);
					}
					// 销毁没有引用次数的对象,并没有销毁 zval本身，不放入gc回收周期。
					zval_ptr_dtor_nogc(free_op_data);
				}
			// 如果操作数据是常量
			} else if (OP_DATA_TYPE == IS_CONST) {
				// 如果是 可计数类型
				if (UNEXPECTED(Z_REFCOUNTED_P(value))) {
					// 引用次数+1
					Z_ADDREF_P(value);
				}
			}
		// 否则
		} else {
			// 获取zval指针, UNUSED 返回null
			dim = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
			// 如果 op2 是常量
			if (OP2_TYPE == IS_CONST) {
				// 像数组一样创建元素，索引为常量
				variable_ptr = zend_fetch_dimension_address_inner_W_CONST(Z_ARRVAL_P(object_ptr), dim EXECUTE_DATA_CC);
			// 否则，op2是变量
			} else {
				// 像数组一样创建元素，索引为变量
				variable_ptr = zend_fetch_dimension_address_inner_W(Z_ARRVAL_P(object_ptr), dim EXECUTE_DATA_CC);
			}
			
			// 如果没有查找到元素
			if (UNEXPECTED(variable_ptr == NULL)) {
				// 跳转到指定标签 assign_dim_error
				ZEND_VM_C_GOTO(assign_dim_error);
			}
			// 取得下一个操作码的 zval。UNUSED返回null, 不支持 TMPVARCV。
			value = GET_OP_DATA_ZVAL_PTR(BP_VAR_R);
			// 给变量赋值，p1:变量，p2:值，p3:值的变量类型（常量，变量，编译变量），p4:是否严格类型
			// 某执行上下文所属函数 的参数是否使用严格类型限制
			value = zend_assign_to_variable(variable_ptr, value, OP_DATA_TYPE, EX_USES_STRICT_TYPES());
		}
		// 如果 操作码的运算结果有效(不是IS_UNUSED)
		if (UNEXPECTED(RETURN_VALUE_USED(opline))) {
			// 取出的值复制给结果
			ZVAL_COPY(EX_VAR(opline->result.var), value);
		}
	// 否则，对象不是数组
	} else {
		// 如果 op1是引用类型
		if (EXPECTED(Z_ISREF_P(object_ptr))) {
			// 解引用
			object_ptr = Z_REFVAL_P(object_ptr);
			// 如果op1是数组 
			if (EXPECTED(Z_TYPE_P(object_ptr) == IS_ARRAY)) {
				// 跳转到指定标签 try_assign_dim_array
				ZEND_VM_C_GOTO(try_assign_dim_array);
			}
		}
		// 如果 op1是对象
		if (EXPECTED(Z_TYPE_P(object_ptr) == IS_OBJECT)) {
			zend_object *obj = Z_OBJ_P(object_ptr);
			// 增加引用次数
			GC_ADDREF(obj);
			// 获取zval指针, UNUSED 返回null
			dim = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
			// 如果 op2 是 编译变量 并且 dim 类型是未定义
			if (OP2_TYPE == IS_CV && UNEXPECTED(Z_ISUNDEF_P(dim))) {
				// 报错：p2的变量（变量名）未定义, 并返未初始化zval
				dim = ZVAL_UNDEFINED_OP2();
			// 否则，如果 o2是常量 并且 有额外值
			} else if (OP2_TYPE == IS_CONST && Z_EXTRA_P(dim) == ZEND_EXTRA_VALUE) {
				// 获取额外值
				dim++;
			}

			// 取得下一个操作码的 zval。UNUSED返回null, CV/TMPVARCV 直接返回
			value = GET_OP_DATA_ZVAL_PTR_UNDEF(BP_VAR_R);
			
			// 如果操作数据类型是 编译变量 并且 值为未定义
			if (OP_DATA_TYPE == IS_CV && UNEXPECTED(Z_ISUNDEF_P(value))) {
				// 报错：未定义的变量（变量名）, 并返未初始化zval, EXECUTE_DATA_DC 传入也用不到
				value = zval_undefined_cv((opline+1)->op1.var EXECUTE_DATA_CC);
			// 如果 操作数据类型为 编译变量 或 普通变量
			} else if (OP_DATA_TYPE & (IS_CV|IS_VAR)) {
				// 值解引用
				ZVAL_DEREF(value);
			}
			// 像数组一样给对象赋值
			zend_assign_to_object_dim(obj, dim, value OPLINE_CC EXECUTE_DATA_CC);

			// 释放下一个操作码的 op1。CONST/UNUSED/CV 免操作。不支持 TMPVARCV。
			FREE_OP_DATA();
			// 引用次数-1，如果为0
			if (UNEXPECTED(GC_DELREF(obj) == 0)) {
				// 释放指定的对象，并更新 objects_store 中的指针
				zend_objects_store_del(obj);
			}
		// 如果 op1类型是字串
		} else if (EXPECTED(Z_TYPE_P(object_ptr) == IS_STRING)) {
			// 如果 op2 类型是未定义
			if (OP2_TYPE == IS_UNUSED) {
				// 报错：不可以对string使用[]操作
				zend_use_new_element_for_string();
				// 释放下一个操作码的 op1。CONST/UNUSED/CV 免操作。不支持 TMPVARCV。
				FREE_OP_DATA();
				// 如果 opline 结果类型是 变量或临时变量，把结果 zval 标记为未定义
				UNDEF_RESULT();
			// 否则
			} else {
				// 获取zval指针, UNUSED 返回null
				dim = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
				// 取得下一个操作码的 zval。UNUSED返回null, CV/TMPVARCV 直接返回
				value = GET_OP_DATA_ZVAL_PTR_UNDEF(BP_VAR_R);
				// 修改字串中的某1个字符（测试过）
				zend_assign_to_string_offset(object_ptr, dim, value OPLINE_CC EXECUTE_DATA_CC);
				// 释放下一个操作码的 op1。CONST/UNUSED/CV 免操作。不支持 TMPVARCV。
				FREE_OP_DATA();
			}
		// 如果op1类型是 undef/null/false 
		} else if (EXPECTED(Z_TYPE_P(object_ptr) <= IS_FALSE)) {
			// 如果 op1 原来是引用类型 
			if (Z_ISREF_P(orig_object_ptr)
			// 并且 有类型
			 && ZEND_REF_HAS_TYPE_SOURCES(Z_REF_P(orig_object_ptr))
			// 并且 检验数组是否可以被 赋值给引用实例。如果不可以
			 && !zend_verify_ref_array_assignable(Z_REF_P(orig_object_ptr))) {
				// 获取zval指针，UNUSED 类型返回null
				dim = GET_OP2_ZVAL_PTR(BP_VAR_R);
				// 释放下一个操作码的 op1。CONST/UNUSED/CV 免操作。不支持 TMPVARCV。
				FREE_OP_DATA();
				// 如果 opline 结果类型是 变量或临时变量，把结果 zval 标记为未定义
				UNDEF_RESULT();
			// 否则 
			} else {
				// 创建新数组，8个元素
				HashTable *ht = zend_new_array(8);
				// op1 的类型
				zend_uchar old_type = Z_TYPE_P(object_ptr);

				// 把哈希表添加到 zval里
				ZVAL_ARR(object_ptr, ht);
				// 如果旧类型是 false
				if (UNEXPECTED(old_type == IS_FALSE)) {
					// 增加引用次数
					GC_ADDREF(ht);
					//  报错：把false转成数组已弃用
					zend_false_to_array_deprecated();
					// 引用次数-1，如果为0
					if (UNEXPECTED(GC_DELREF(ht) == 0)) {
						// 销毁哈希表
						zend_array_destroy(ht);
						// 跳转到指定标签 assign_dim_error
						ZEND_VM_C_GOTO(assign_dim_error);
					}
				}
				// 跳转到指定标签 try_assign_dim_array
				ZEND_VM_C_GOTO(try_assign_dim_array);
			}
		// 否则
		} else {
			// 报错：不可以把标量当成数组用
			zend_use_scalar_as_array();
			// 获取zval指针，UNUSED 类型返回null
			dim = GET_OP2_ZVAL_PTR(BP_VAR_R);
// 创建跳转标签 assign_dim_error
ZEND_VM_C_LABEL(assign_dim_error):
			// 释放下一个操作码的 op1。CONST/UNUSED/CV 免操作。不支持 TMPVARCV。
			FREE_OP_DATA();
			// 如果 操作码的运算结果有效(不是IS_UNUSED)
			if (UNEXPECTED(RETURN_VALUE_USED(opline))) {
				// 操作码 结果为null
				ZVAL_NULL(EX_VAR(opline->result.var));
			}
		}
	}
	// 如果 op2有效
	if (OP2_TYPE != IS_UNUSED) {
		// 释放操作对象的附加变量
		FREE_OP2();
	}
	// 释放操作对象的附加变量
	FREE_OP1();
	// 维度赋值有两个操作码
	/* assign_dim has two opcodes! */
	// 偏移到目标操作码并 return ,p1:是否检查异常？使用 EX(opline)  ：使用 opline 。p2:偏移量
	ZEND_VM_NEXT_OPCODE_EX(1, 2);
}

// ing3, 给变量赋值
ZEND_VM_HANDLER(22, ZEND_ASSIGN, VAR|CV, CONST|TMP|VAR|CV, SPEC(RETVAL))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *value;
	zval *variable_ptr;

	// windows: 无操作
	SAVE_OPLINE();
	// 获取zval指针，UNUSED 类型返回null
	value = GET_OP2_ZVAL_PTR(BP_VAR_R);
	// 获取zval指针, TMP/CONST/UNUSED 返回null，不支持 TMPVAR/TMPVARCV
	variable_ptr = GET_OP1_ZVAL_PTR_PTR_UNDEF(BP_VAR_W);

	// 给变量赋值，p1:变量，p2:值，p3:值的变量类型（常量，变量，编译变量），p4:是否严格类型
	// 某执行上下文所属函数 的参数是否使用严格类型限制
	value = zend_assign_to_variable(variable_ptr, value, OP2_TYPE, EX_USES_STRICT_TYPES());
	// 如果 操作码的运算结果有效(不是IS_UNUSED)
	if (UNEXPECTED(RETURN_VALUE_USED(opline))) {
		// 值放到结果中
		ZVAL_COPY(EX_VAR(opline->result.var), value);
	}
	// 释放操作对象的附加变量
	FREE_OP1();
	// zend_assign_to_variable 总会用到 op1,不要释放它
	/* zend_assign_to_variable() always takes care of op2, never free it! */

	// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, 引用赋值
ZEND_VM_HANDLER(30, ZEND_ASSIGN_REF, VAR|CV, VAR|CV, SRC)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *variable_ptr;
	zval *value_ptr;

	// windows: 无操作
	SAVE_OPLINE();
	// 获取zval指针,TMP/CONST/UNUSED 返回null，不支持 TMPVAR/TMPVARCV
	value_ptr = GET_OP2_ZVAL_PTR_PTR(BP_VAR_W);
	// 获取zval指针, TMP/CONST/UNUSED 返回null，不支持 TMPVAR/TMPVARCV
	variable_ptr = GET_OP1_ZVAL_PTR_PTR_UNDEF(BP_VAR_W);

	// 如果op1是变量 并且 op1不是间接引用
	if (OP1_TYPE == IS_VAR &&
	           UNEXPECTED(Z_TYPE_P(EX_VAR(opline->op1.var)) != IS_INDIRECT)) {
		// 报错：不可以给一个对象进行维度赋值时使用引用类型
		zend_throw_error(NULL, "Cannot assign by reference to an array dimension of an object");
		// 使用未初始化zval
		variable_ptr = &EG(uninitialized_zval);
	// 如果op2是变量 并且 在return语句中有调用函数 并且 value_ptr不是引用类型
	} else if (OP2_TYPE == IS_VAR &&
	           opline->extended_value == ZEND_RETURNS_FUNCTION &&
			   UNEXPECTED(!Z_ISREF_P(value_ptr))) {
		// 给引用类型 赋值出错，有错误提示信息，但还是进行赋值了。p1:变量指针，p2:值指针
		variable_ptr = zend_wrong_assign_to_variable_reference(
			variable_ptr, value_ptr OPLINE_CC EXECUTE_DATA_CC);
	// 否则
	} else {
		// 对变量进行引用赋值。p1:变量指针，p2:值指针
		zend_assign_to_variable_reference(variable_ptr, value_ptr);
	}

	// 如果 操作码的运算结果有效(不是IS_UNUSED)
	if (UNEXPECTED(RETURN_VALUE_USED(opline))) {
		// 把值复制给结果
		ZVAL_COPY(EX_VAR(opline->result.var), variable_ptr);
	}

	// 释放操作对象的附加变量
	FREE_OP2();
	// 释放操作对象的附加变量
	FREE_OP1();
	// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, 给对象属性 引用赋值
ZEND_VM_HANDLER(32, ZEND_ASSIGN_OBJ_REF, VAR|UNUSED|THIS|CV, CONST|TMPVAR|CV, CACHE_SLOT|SRC, SPEC(OP_DATA=VAR|CV))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *property, *container, *value_ptr;

	// windows: 无操作
	SAVE_OPLINE();

	// 获取zval指针, TMP/CONST返回null, UNUSED 返回 $this，不支持 TMPVAR/TMPVARCV。CV 直接返回。
	container = GET_OP1_OBJ_ZVAL_PTR_PTR_UNDEF(BP_VAR_W);
	// 获取zval指针，UNUSED 类型返回null
	property = GET_OP2_ZVAL_PTR(BP_VAR_R);

	// 取得下一个操作码的 zval。TMP/CONST/UNUSED返回null, 不支持 TMPVAR/TMPVARCV
	value_ptr = GET_OP_DATA_ZVAL_PTR_PTR(BP_VAR_W);
	// op1或op2有一个不是ANY
	if (ZEND_VM_SPEC) {
		// 如果 op1 类型是未定义
		if (OP1_TYPE == IS_UNUSED) {
			// 如果op2 是常量
			if (OP2_TYPE == IS_CONST) {
				// 对属性进行引用赋值，容器运算对象类型为未使用，属性运算对象类型为常量
				// p1:属性容器，p2:属性指针，p3:新值
				zend_assign_to_property_reference_this_const(container, property, value_ptr OPLINE_CC EXECUTE_DATA_CC);
			// 否则，op2不是常量
			} else {
				// 对属性进行引用赋值，容器运算对象类型为 未使用，属性运算对象类型为变量
				// p1:属性容器，p2:属性指针，p3:新值
				zend_assign_to_property_reference_this_var(container, property, value_ptr OPLINE_CC EXECUTE_DATA_CC);
			}
		// 否则
		} else {
			// 如果op2是常量
			if (OP2_TYPE == IS_CONST) {
				// 对属性进行引用赋值，容器运算对象类型为 变量，属性运算对象类型为常量
				// p1:属性容器，p2:属性指针，p3:新值
				zend_assign_to_property_reference_var_const(container, property, value_ptr OPLINE_CC EXECUTE_DATA_CC);
			// 否则，op2不是常量
			} else {
				// 对属性进行引用赋值，容器运算对象类型为 变量，属性运算对象类型为变量
				// p1:属性容器，p2:属性指针，p3:新值
				zend_assign_to_property_reference_var_var(container, property, value_ptr OPLINE_CC EXECUTE_DATA_CC);
			}
		}
	// 否则
	} else {
		// 对属性进行引用赋值，p1:属性容器，p2:容器运算对象类型，p3:属性指针，p4:属性运算对象类型，p5:新值
		zend_assign_to_property_reference(container, OP1_TYPE, property, OP2_TYPE, value_ptr OPLINE_CC EXECUTE_DATA_CC);
	}

	// 释放操作对象的附加变量
	FREE_OP1();
	// 释放操作对象的附加变量
	FREE_OP2();
	// 释放下一个操作码的 op1。CONST/UNUSED/CV 免操作。不支持 TMPVARCV。
	FREE_OP_DATA();
	// 偏移到目标操作码并 return ,p1:是否检查异常？使用 EX(opline)  ：使用 opline 。p2:偏移量
	ZEND_VM_NEXT_OPCODE_EX(1, 2);
}

// ing2, 给静态属性引用赋值,后面会带一个 OP_DATA 专门传递数据的操作码？
/* No specialization for op_types (CONST|TMPVAR|CV, UNUSED|CONST|VAR) */
ZEND_VM_HANDLER(33, ZEND_ASSIGN_STATIC_PROP_REF, ANY, ANY, CACHE_SLOT|SRC)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *prop, *value_ptr;
	zend_property_info *prop_info;

	// windows: 无操作
	SAVE_OPLINE();

	// 通过缓存位置和类型，返回静态属性地址。p1:返回属性值，p2:返回属性信息，p3:缓存位置，p4:获取类型，p5:falgs
	if (zend_fetch_static_property_address(&prop, &prop_info, opline->extended_value & ~ZEND_RETURNS_FUNCTION, BP_VAR_W, 0 OPLINE_CC EXECUTE_DATA_CC) != SUCCESS) {
		// 释放下一个操作码的 op1。CONST/UNUSED/CV 免操作。不支持 TMPVARCV。
		FREE_OP_DATA();
		// 如果 opline 结果类型是 变量或临时变量，把结果 zval 标记为未定义
		UNDEF_RESULT();
		// 主要是 return
		HANDLE_EXCEPTION();
	}

	// 取得下一个操作码的 zval。TMP/CONST/UNUSED返回null, 不支持 TMPVAR/TMPVARCV
	value_ptr = GET_OP_DATA_ZVAL_PTR_PTR(BP_VAR_W);

	// 如果 数据类型是变量 并且 ZEND_RETURNS_FUNCTION 并且 value_ptr 不是引用类型
	if (OP_DATA_TYPE == IS_VAR && (opline->extended_value & ZEND_RETURNS_FUNCTION) && UNEXPECTED(!Z_ISREF_P(value_ptr))) {
		// 给引用类型 赋值出错，有错误提示信息，但还是进行赋值了。p1:变量指针，p2:值指针
		if (UNEXPECTED(!zend_wrong_assign_to_variable_reference(prop, value_ptr OPLINE_CC EXECUTE_DATA_CC))) {
			// 值为，未初始化 zval
			prop = &EG(uninitialized_zval);
		}
	// 否则 如果类型存在
	} else if (UNEXPECTED(ZEND_TYPE_IS_SET(prop_info->type))) {
		// 对有类型的属性进行引用赋值，p1:属性信息，p2:属性实例，p3:新值
		prop = zend_assign_to_typed_property_reference(prop_info, prop, value_ptr EXECUTE_DATA_CC);
	// 否则
	} else {
		// 对变量进行引用赋值。p1:变量指针，p2:值指针
		zend_assign_to_variable_reference(prop, value_ptr);
	}

	// 如果 操作码的运算结果有效(不是IS_UNUSED)
	if (UNEXPECTED(RETURN_VALUE_USED(opline))) {
		// 把
		ZVAL_COPY(EX_VAR(opline->result.var), prop);
	}

	// 释放下一个操作码的 op1。CONST/UNUSED/CV 免操作。不支持 TMPVARCV。
	FREE_OP_DATA();
	// 偏移到目标操作码并 return ,p1:是否检查异常？使用 EX(opline)  ：使用 opline 。p2:偏移量
	ZEND_VM_NEXT_OPCODE_EX(1, 2);
}

// 140行，
// ing2, 退出当前执行数据
ZEND_VM_HOT_HELPER(zend_leave_helper, ANY, ANY)
{
	// 
	zend_execute_data *old_execute_data;
	// 取出调用信息
	uint32_t call_info = EX_CALL_INFO();
	// windows: 无操作
	SAVE_OPLINE();

	// 如果这几个标记一个也没有
	if (EXPECTED((call_info & (ZEND_CALL_CODE|ZEND_CALL_TOP|ZEND_CALL_HAS_SYMBOL_TABLE|ZEND_CALL_FREE_EXTRA_ARGS|ZEND_CALL_ALLOCATED|ZEND_CALL_HAS_EXTRA_NAMED_PARAMS)) == 0)) {
		// 使用前一个执行码
		EG(current_execute_data) = EX(prev_execute_data);
		// 销毁 执行函数中的所有临时变量
		i_free_compiled_variables(execute_data);

#ifdef ZEND_PREFER_RELOAD
		call_info = EX_CALL_INFO();
#endif
		// 如果需要释放$this
		if (UNEXPECTED(call_info & ZEND_CALL_RELEASE_THIS)) {
			// 释放$this
			OBJ_RELEASE(Z_OBJ(execute_data->This));
		// 如果是在调用闭包
		} else if (UNEXPECTED(call_info & ZEND_CALL_CLOSURE)) {
			// 释放闭包
			// 从 _zend_closure.zend_function 找到 _zend_closure 开头。
			OBJ_RELEASE(ZEND_CLOSURE_OBJECT(EX(func)));
		}
		// 此执行数据放在虚拟机堆栈顶
		EG(vm_stack_top) = (zval*)execute_data;
		// 转到前一个执行数据
		execute_data = EX(prev_execute_data);

		// 如果有异常
		if (UNEXPECTED(EG(exception) != NULL)) {
			// 重新抛异常
			zend_rethrow_exception(execute_data);
			// windows: return 2;
			HANDLE_EXCEPTION_LEAVE();
		}

		// windows: ZEND_VM_INC_OPCODE()： // OPLINE++
		LOAD_NEXT_OPLINE();
		// windows return 2;
		ZEND_VM_LEAVE();
	// 否则 如果有调用代码 或 是顶层调用
	} else if (EXPECTED((call_info & (ZEND_CALL_CODE|ZEND_CALL_TOP)) == 0)) {
		// 前一个执行数据
		EG(current_execute_data) = EX(prev_execute_data);
		// 销毁 执行函数中的所有临时变量
		i_free_compiled_variables(execute_data);

#ifdef ZEND_PREFER_RELOAD
		call_info = EX_CALL_INFO();
#endif
		// 如果有符号表
		if (UNEXPECTED(call_info & ZEND_CALL_HAS_SYMBOL_TABLE)) {
			// 清空并缓存 符号表
			zend_clean_and_cache_symbol_table(EX(symbol_table));
		}

		// 如果有额外命名参数
		if (UNEXPECTED(call_info & ZEND_CALL_HAS_EXTRA_NAMED_PARAMS)) {
			// 释放额外命名参数
			zend_free_extra_named_params(EX(extra_named_params));
		}

		// 在释放闭包前释放额外参数，因为它会释放 op_array
		/* Free extra args before releasing the closure,
		 * as that may free the op_array. */
		 
		// 销毁 虚拟机堆栈里的额外参数，p1:调用信息，p2:执行数据
		zend_vm_stack_free_extra_args_ex(call_info, execute_data);

		// 如果需要释放 $this
		if (UNEXPECTED(call_info & ZEND_CALL_RELEASE_THIS)) {
			// 释放 $this
			OBJ_RELEASE(Z_OBJ(execute_data->This));
		// 否则如果 在调用闭包
		} else if (UNEXPECTED(call_info & ZEND_CALL_CLOSURE)) {
			// 释放闭包
			OBJ_RELEASE(ZEND_CLOSURE_OBJECT(EX(func)));
		}

		// 当前执行数据作为旧的
		old_execute_data = execute_data;
		// 切换到前一个执行数据
		execute_data = EX(prev_execute_data);
		// 释放调用框架（执行数据），p1:调用信息，p2:执行数据
		zend_vm_stack_free_call_frame_ex(call_info, old_execute_data);

		// 如果有异常
		if (UNEXPECTED(EG(exception) != NULL)) {
			// 重新抛异常
			zend_rethrow_exception(execute_data);
			// windows: return 2;
			HANDLE_EXCEPTION_LEAVE();
		}

		// windows: ZEND_VM_INC_OPCODE() : // OPLINE++
		LOAD_NEXT_OPLINE();
		// windows return 2;
		ZEND_VM_LEAVE();
	// 如果不是顶层调用
	} else if (EXPECTED((call_info & ZEND_CALL_TOP) == 0)) {
		// 如果有临时变量
		if (EX(func)->op_array.last_var > 0) {
			// 把临时变量中的编译变量 同步到符号表中，然后清空临时变量
			zend_detach_symbol_table(execute_data);
			// 添加【需要链接符号表】标记
			call_info |= ZEND_CALL_NEEDS_REATTACH;
		}
		// 销毁静态变量表
		zend_destroy_static_vars(&EX(func)->op_array);
		// 销毁操作码组
		destroy_op_array(&EX(func)->op_array);
		// 释放销毁操作码组
		efree_size(EX(func), sizeof(zend_op_array));
		// 成为旧执行数据
		old_execute_data = execute_data;
		// 当前执行数据切换到前一个
		execute_data = EG(current_execute_data) = EX(prev_execute_data);
		// 释放调用框架（执行数据），p1:调用信息，p2:执行数据
		zend_vm_stack_free_call_frame_ex(call_info, old_execute_data);

		// 如果有 【需要链接符号表】标记
		if (call_info & ZEND_CALL_NEEDS_REATTACH) {
			// 如果有临时变量
			if (EX(func)->op_array.last_var > 0) {
				// 从附加符号表里读取所有临时变量，并更新到临时变量列表中
				zend_attach_symbol_table(execute_data);
			// 否则
			} else {
				// 添加标记，需要重新连接符号表
				ZEND_ADD_CALL_FLAG(execute_data, ZEND_CALL_NEEDS_REATTACH);
			}
		}
		// 如果有异常
		if (UNEXPECTED(EG(exception) != NULL)) {
			// 重新抛异常
			zend_rethrow_exception(execute_data);
			// windows: return 2;
			HANDLE_EXCEPTION_LEAVE();
		}

		// windows: ZEND_VM_INC_OPCODE() : // OPLINE++
		LOAD_NEXT_OPLINE();
		// windows return 2;
		ZEND_VM_LEAVE();
	// 否则
	} else {
		// 如果没有调用代码标记
		if (EXPECTED((call_info & ZEND_CALL_CODE) == 0)) {
			// 切换到前一个执行数据
			EG(current_execute_data) = EX(prev_execute_data);
			// 销毁 执行函数中的所有临时变量
			i_free_compiled_variables(execute_data);
#ifdef ZEND_PREFER_RELOAD
			call_info = EX_CALL_INFO();
#endif
			// 如果有 【有符号表】【需要释放额外参数】【有额外命名参数】3个中任何一个
			if (UNEXPECTED(call_info & (ZEND_CALL_HAS_SYMBOL_TABLE|ZEND_CALL_FREE_EXTRA_ARGS|ZEND_CALL_HAS_EXTRA_NAMED_PARAMS))) {
				// 如果有符号表
				if (UNEXPECTED(call_info & ZEND_CALL_HAS_SYMBOL_TABLE)) {
					// 清空并缓存 符号表
					zend_clean_and_cache_symbol_table(EX(symbol_table));
				}
				// 销毁 虚拟机堆栈里的额外参数，p1:调用信息，p2:执行数据
				zend_vm_stack_free_extra_args_ex(call_info, execute_data);
				// 如果有额外命名参数
				if (UNEXPECTED(call_info & ZEND_CALL_HAS_EXTRA_NAMED_PARAMS)) {
					// 释放额外参数
					zend_free_extra_named_params(EX(extra_named_params));
				}
			}
			// 如果是调用闭包
			if (UNEXPECTED(call_info & ZEND_CALL_CLOSURE)) {
				// 释放闭包
				OBJ_RELEASE(ZEND_CLOSURE_OBJECT(EX(func)));
			}
			// windows: return -1
			ZEND_VM_RETURN();
		// 否则，是调用顶层代码
		} else /* if (call_kind == ZEND_CALL_TOP_CODE) */ {
			// 当前执行数据的符号表
			zend_array *symbol_table = EX(symbol_table);

			// 如果有临时变量
			if (EX(func)->op_array.last_var > 0) {
				// 把临时变量中的编译变量 同步到符号表中，然后清空临时变量
				zend_detach_symbol_table(execute_data);
				// 添加标记，需要重新连接符号表
				call_info |= ZEND_CALL_NEEDS_REATTACH;
			}
			// 如果需要重新连接符号表
			if (call_info & ZEND_CALL_NEEDS_REATTACH) {
				// 前一个执行数据
				old_execute_data = EX(prev_execute_data);
				// 向前遍历整个链，找到有符号表的执行数据
				while (old_execute_data) {
					// 如果此执行数据有函数 并且 有符号表
					if (old_execute_data->func && (ZEND_CALL_INFO(old_execute_data) & ZEND_CALL_HAS_SYMBOL_TABLE)) {
						// 如果就是要找的符号表
						if (old_execute_data->symbol_table == symbol_table) {
							// 如果函数有临时变量
							if (old_execute_data->func->op_array.last_var > 0) {
								// 从附加符号表里读取所有临时变量，并更新到临时变量列表中
								zend_attach_symbol_table(old_execute_data);
							// 否则
							} else {
								// 添加标记，需要重新连接符号表
								ZEND_ADD_CALL_FLAG(old_execute_data, ZEND_CALL_NEEDS_REATTACH);
							}
						}
						// 只要有符号表就跳出
						break;
					}
					// 到 前一个执行数据
					old_execute_data = old_execute_data->prev_execute_data;
				}
			}
			// 切换到前一个执行数据
			EG(current_execute_data) = EX(prev_execute_data);
			// windows: return -1
			ZEND_VM_RETURN();
		}
	}
}

// ing3, 跳转到op1指定的操作码
ZEND_VM_HOT_HANDLER(42, ZEND_JMP, JMP_ADDR, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// 要检查异常 并 有异常时使用旧操作码，无异常使用新操作码, p1:新操作码，p2:是否检查异常
	// OP_JMP_ADDR: 访问 p2.jmp_addr
	ZEND_VM_JMP_EX(OP_JMP_ADDR(opline, opline->op1), 0);
}

// ing3, 根据op1跳转
ZEND_VM_HOT_NOCONST_HANDLER(43, ZEND_JMPZ, CONST|TMPVAR|CV, JMP_ADDR)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *val;
	zend_uchar op1_type;

	// 获取zval指针, UNUSED 返回null
	val = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);

	// 如果 op1 变量 为 true
	if (Z_TYPE_INFO_P(val) == IS_TRUE) {
		// opline + 1，到目标操作码并 return
		ZEND_VM_NEXT_OPCODE();
	// 否则 如果 op1 变量为 false,null,未定义
	} else if (EXPECTED(Z_TYPE_INFO_P(val) <= IS_TRUE)) {
		// 如果 op1 类型为 编译变量 并且 值类型为未定义
		if (OP1_TYPE == IS_CV && UNEXPECTED(Z_TYPE_INFO_P(val) == IS_UNDEF)) {
			// windows: 无操作
			SAVE_OPLINE();
			// 报错：p1的变量（变量名）未定义, 并返未初始化zval
			ZVAL_UNDEFINED_OP1();
			// 如果有异常
			if (UNEXPECTED(EG(exception))) {
				// 主要是 return
				HANDLE_EXCEPTION();
			}
		}
		// 要检查异常 并 有异常时使用旧操作码，无异常使用新操作码, p1:新操作码，p2:是否检查异常
		// OP_JMP_ADDR: 访问 p2.jmp_addr
		ZEND_VM_JMP_EX(OP_JMP_ADDR(opline, opline->op2), 0);
	}

	// windows: 无操作
	SAVE_OPLINE();
	// op1类型
	op1_type = OP1_TYPE;
	// 值转成布尔型
	if (i_zend_is_true(val)) {
		// 下一个操作码
		opline++;
	// 否则
	} else {
		// 跳到指定操作码
		// OP_JMP_ADDR: 访问 p2.jmp_addr
		opline = OP_JMP_ADDR(opline, opline->op2);
	}
	// 如果op1是 普通变量 或 临时变量
	if (op1_type & (IS_TMP_VAR|IS_VAR)) {
		// 销毁没有引用次数的对象,并没有销毁 zval本身，不放入gc回收周期。
		zval_ptr_dtor_nogc(val);
	}
	// 有异常时使用旧操作码，无异常使用新操作码, p1:新操作码
	ZEND_VM_JMP(opline);
}

// ing3, 根据op1 跳转到不同的地方
ZEND_VM_HOT_NOCONST_HANDLER(44, ZEND_JMPNZ, CONST|TMPVAR|CV, JMP_ADDR)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *val;
	zend_uchar op1_type;

	// 获取zval指针, UNUSED 返回null
	val = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);

	// 如果 变量 为 true
	if (Z_TYPE_INFO_P(val) == IS_TRUE) {
		// 要检查异常 并 有异常时使用旧操作码，无异常使用新操作码, p1:新操作码，p2:是否检查异常
		// OP_JMP_ADDR: 访问 p2.jmp_addr
		ZEND_VM_JMP_EX(OP_JMP_ADDR(opline, opline->op2), 0);
	// 否则 如果 变量为 false,null,未定义
	} else if (EXPECTED(Z_TYPE_INFO_P(val) <= IS_TRUE)) {
		// 如果 op1 类型为 编译变量 并且 值类型为未定义
		if (OP1_TYPE == IS_CV && UNEXPECTED(Z_TYPE_INFO_P(val) == IS_UNDEF)) {
			// windows: 无操作
			SAVE_OPLINE();
			// 报错：p1的变量（变量名）未定义, 并返未初始化zval
			ZVAL_UNDEFINED_OP1();
			// 如果有异常
			if (UNEXPECTED(EG(exception))) {
				// 主要是 return
				HANDLE_EXCEPTION();
			}
		}
		// opline + 1，到目标操作码并 return
		ZEND_VM_NEXT_OPCODE();
	}

	// windows: 无操作
	SAVE_OPLINE();
	// op1的类型
	op1_type = OP1_TYPE;
	// 转成布尔型，如果是true
	if (i_zend_is_true(val)) {
		// OP_JMP_ADDR: 访问 p2.jmp_addr
		opline = OP_JMP_ADDR(opline, opline->op2);
	// 否则,是false
	} else {
		// 下一个操作码
		opline++;
	}
	// 如果op1是临时变量或变量
	if (op1_type & (IS_TMP_VAR|IS_VAR)) {
		// 销毁没有引用次数的对象,并没有销毁 zval本身，不放入gc回收周期。
		zval_ptr_dtor_nogc(val);
	}
	// 有异常时使用旧操作码，无异常使用新操作码, p1:新操作码
	ZEND_VM_JMP(opline);
}

// ing3, 根据op1跳转
ZEND_VM_COLD_CONST_HANDLER(46, ZEND_JMPZ_EX, CONST|TMPVAR|CV, JMP_ADDR)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *val;
	bool ret;

	// 获取zval指针, UNUSED 返回null
	val = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);

	// 如果 变量 为 true
	if (Z_TYPE_INFO_P(val) == IS_TRUE) {
		// 结果为 true
		ZVAL_TRUE(EX_VAR(opline->result.var));
		// opline + 1，到目标操作码并 return
		ZEND_VM_NEXT_OPCODE();
	// 否则 如果 变量为 false,null,未定义
	} else if (EXPECTED(Z_TYPE_INFO_P(val) <= IS_TRUE)) {
		// 结果为 false
		ZVAL_FALSE(EX_VAR(opline->result.var));
		// 如果 op1 类型为 编译变量 并且 值类型为未定义
		if (OP1_TYPE == IS_CV && UNEXPECTED(Z_TYPE_INFO_P(val) == IS_UNDEF)) {
			// windows: 无操作
			SAVE_OPLINE();
			// 报错：p1的变量（变量名）未定义, 并返未初始化zval
			ZVAL_UNDEFINED_OP1();
			// 如果有异常
			if (UNEXPECTED(EG(exception))) {
				// 主要是 return
				HANDLE_EXCEPTION();
			}
		}
		// 要检查异常 并 有异常时使用旧操作码，无异常使用新操作码, p1:新操作码，p2:是否检查异常
		// OP_JMP_ADDR: 访问 p2.jmp_addr
		ZEND_VM_JMP_EX(OP_JMP_ADDR(opline, opline->op2), 0);
	}

	// windows: 无操作
	SAVE_OPLINE();
	// 值转成布尔型
	ret = i_zend_is_true(val);
	// 释放操作对象的附加变量
	FREE_OP1();
	// 如果值为true
	if (ret) {
		// 结果为true
		ZVAL_TRUE(EX_VAR(opline->result.var));
		// 下一个操作码
		opline++;
	// 否则，值为false
	} else {
		// 结果为false
		ZVAL_FALSE(EX_VAR(opline->result.var));
		// OP_JMP_ADDR: 访问 p2.jmp_addr
		opline = OP_JMP_ADDR(opline, opline->op2);
	}
	// 有异常时使用旧操作码，无异常使用新操作码, p1:新操作码
	ZEND_VM_JMP(opline);
}

// ing3,根据真假值跳转（如if）
ZEND_VM_COLD_CONST_HANDLER(47, ZEND_JMPNZ_EX, CONST|TMPVAR|CV, JMP_ADDR)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *val;
	bool ret;

	// 获取zval指针, UNUSED 返回null
	val = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);

	// 如果 变量 为 true
	if (Z_TYPE_INFO_P(val) == IS_TRUE) {
		ZVAL_TRUE(EX_VAR(opline->result.var));
		// 要检查异常 并 有异常时使用旧操作码，无异常使用新操作码, p1:新操作码，p2:是否检查异常
		// OP_JMP_ADDR: 访问 p2.jmp_addr
		ZEND_VM_JMP_EX(OP_JMP_ADDR(opline, opline->op2), 0);
	// 否则 如果 变量为 false,null,未定义
	} else if (EXPECTED(Z_TYPE_INFO_P(val) <= IS_TRUE)) {
		// 结果为假
		ZVAL_FALSE(EX_VAR(opline->result.var));
		// 如果 op1 类型为 编译变量 并且 值类型为未定义
		if (OP1_TYPE == IS_CV && UNEXPECTED(Z_TYPE_INFO_P(val) == IS_UNDEF)) {
			// windows: 无操作
			SAVE_OPLINE();
			// 报错：p1的变量（变量名）未定义, 并返未初始化zval
			ZVAL_UNDEFINED_OP1();
			// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
			ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
		// 否则
		} else {
			// opline + 1，到目标操作码并 return
			ZEND_VM_NEXT_OPCODE();
		}
	}

	// windows: 无操作
	SAVE_OPLINE();
	// 值转成bool型
	ret = i_zend_is_true(val);
	// 释放操作对象的附加变量
	FREE_OP1();
	// 如果值为真
	if (ret) {
		// 结果为真
		ZVAL_TRUE(EX_VAR(opline->result.var));
		// 访问 p2.jmp_addr
		opline = OP_JMP_ADDR(opline, opline->op2);
	// 否则
	} else {
		// 结果为假
		ZVAL_FALSE(EX_VAR(opline->result.var));
		// 到下一个操作码（值为假，的跳转目标）
		opline++;
	}
	// 跳转
	// 有异常时使用旧操作码，无异常使用新操作码, p1:新操作码
	ZEND_VM_JMP(opline);
}

// ing3, 销毁操作码 op1 的变量
ZEND_VM_HANDLER(70, ZEND_FREE, TMPVAR, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// windows: 无操作
	SAVE_OPLINE();
	// 销毁没有引用次数的对象,并没有销毁 zval本身，不放入gc回收周期。
	zval_ptr_dtor_nogc(EX_VAR(opline->op1.var));
	// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, 销毁数组和迭代器
ZEND_VM_HOT_HANDLER(127, ZEND_FE_FREE, TMPVAR, ANY)
{
	zval *var;
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// 在执行数据中取出 指定序号的变量
	var = EX_VAR(opline->op1.var);
	// 如果op1不是数组
	if (Z_TYPE_P(var) != IS_ARRAY) {
		// windows: 无操作
		SAVE_OPLINE();
		// 通过指针访问zval的 迭代索引号 , (*p1).u2.fe_iter_idx
		// 如果迭代器有效
		if (Z_FE_ITER_P(var) != (uint32_t)-1) {
			// 在迭代器哈希表中删除这个迭代器
			// 通过指针访问zval的 迭代索引号 , (*p1).u2.fe_iter_idx
			zend_hash_iterator_del(Z_FE_ITER_P(var));
		}
		// 销毁没有引用次数的对象,并没有销毁 zval本身，不放入gc回收周期。
		zval_ptr_dtor_nogc(var);
		// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
		ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
	}

	// 为了销毁数组，使用 一个 zval_ptr_dtor_nogc 的行内版本
	// 只有当最后的引用被垃圾回收了，php才需要保存opline并检验异常（数组中的元素销毁 不会抛错）。
	/* This is freeing an array. Use an inlined version of zval_ptr_dtor_nogc. */
	/* PHP only needs to save the opline and check for an exception if the last reference to the array was garbage collected (destructors of elements in the array could throw an exception) */
	// 如果是 可计数类型 并且 引用次数 -1后为0
	if (Z_REFCOUNTED_P(var) && !Z_DELREF_P(var)) {
		// windows: 无操作
		SAVE_OPLINE();
		// 调用每个type对应的销毁函数执行销毁
		rc_dtor_func(Z_COUNTED_P(var));
		// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
		ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
	}
	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, 快速连接
ZEND_VM_COLD_CONSTCONST_HANDLER(53, ZEND_FAST_CONCAT, CONST|TMPVAR|CV, CONST|TMPVAR|CV)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2;
	zend_string *op1_str, *op2_str, *str;


	// 获取zval指针, UNUSED 返回null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 获取zval指针, UNUSED 返回null
	op2 = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 如果 op1,op2 都是（字串 或 常量）
	if ((OP1_TYPE == IS_CONST || EXPECTED(Z_TYPE_P(op1) == IS_STRING)) &&
	    (OP2_TYPE == IS_CONST || EXPECTED(Z_TYPE_P(op2) == IS_STRING))) {
		// 取出字串
		zend_string *op1_str = Z_STR_P(op1);
		zend_string *op2_str = Z_STR_P(op2);
		// 
		zend_string *str;

		// 如果 OP1 不是常量 并且 长度为0
		if (OP1_TYPE != IS_CONST && UNEXPECTED(ZSTR_LEN(op1_str) == 0)) {
			// 如果 op2 是 常量 或 编译变量
			if (OP2_TYPE == IS_CONST || OP2_TYPE == IS_CV) {
				// op2 复制到结果里
				ZVAL_STR_COPY(EX_VAR(opline->result.var), op2_str);
			// 否则
			} else {
				// op2 的字串直接放到结果里
				ZVAL_STR(EX_VAR(opline->result.var), op2_str);
			}
			// 如果 op1 类型 是变量或临时变量
			if (OP1_TYPE & (IS_TMP_VAR|IS_VAR)) {
				// 释放 临时字串
				zend_string_release_ex(op1_str, 0);
			}
		// 否则 如果 OP2 不是常量 并且 长度为0
		} else if (OP2_TYPE != IS_CONST && UNEXPECTED(ZSTR_LEN(op2_str) == 0)) {
			// 如果 op1 是 常量 或 编译变量
			if (OP1_TYPE == IS_CONST || OP1_TYPE == IS_CV) {
				// op1复制到结果里
				ZVAL_STR_COPY(EX_VAR(opline->result.var), op1_str);
			// 否则
			} else {
				// op1字串指针放到结果里
				ZVAL_STR(EX_VAR(opline->result.var), op1_str);
			}
			// 如果 op2 类型 是变量或临时变量
			if (OP2_TYPE & (IS_TMP_VAR|IS_VAR)) {
				// 释放 临时字串
				zend_string_release_ex(op2_str, 0);
			}
		// 否则 如果 OP1 不是常量 也不是 编译变量 也不是内置字串 并且 引用数为1
		} else if (OP1_TYPE != IS_CONST && OP1_TYPE != IS_CV &&
		    !ZSTR_IS_INTERNED(op1_str) && GC_REFCOUNT(op1_str) == 1) {
			// 长度
			size_t len = ZSTR_LEN(op1_str);
			// 字串增加长度
			str = zend_string_extend(op1_str, len + ZSTR_LEN(op2_str), 0);
			// 把op2复制到后面
			memcpy(ZSTR_VAL(str) + len, ZSTR_VAL(op2_str), ZSTR_LEN(op2_str)+1);
			// 把zend_string 添加给 zval，不支持保留字
			ZVAL_NEW_STR(EX_VAR(opline->result.var), str);
			// 如果 op2 类型 是变量或临时变量
			if (OP2_TYPE & (IS_TMP_VAR|IS_VAR)) {
				// 释放临时变量
				zend_string_release_ex(op2_str, 0);
			}
		// 否则
		} else {
			// 重新分配字串
			str = zend_string_alloc(ZSTR_LEN(op1_str) + ZSTR_LEN(op2_str), 0);
			// 前面一段放op1
			memcpy(ZSTR_VAL(str), ZSTR_VAL(op1_str), ZSTR_LEN(op1_str));
			// 后面一段放op2
			memcpy(ZSTR_VAL(str) + ZSTR_LEN(op1_str), ZSTR_VAL(op2_str), ZSTR_LEN(op2_str)+1);
			// 把zend_string 添加给 zval，不支持保留字
			ZVAL_NEW_STR(EX_VAR(opline->result.var), str);
			// 如果 op1 类型 是变量或临时变量
			if (OP1_TYPE & (IS_TMP_VAR|IS_VAR)) {
				// 释放op1 临时变量
				zend_string_release_ex(op1_str, 0);
			}
			// 如果 op2 类型 是变量或临时变量
			if (OP2_TYPE & (IS_TMP_VAR|IS_VAR)) {
				// 释放op2 临时变量
				zend_string_release_ex(op2_str, 0);
			}
		}
		// opline + 1，到目标操作码并 return
		ZEND_VM_NEXT_OPCODE();
	}

	// windows: 无操作
	SAVE_OPLINE();
	// 如果op1是常量
	if (OP1_TYPE == IS_CONST) {
		// 取出字串
		op1_str = Z_STR_P(op1);
	// 如果类型是字串
	} else if (EXPECTED(Z_TYPE_P(op1) == IS_STRING)) {
		// op1 增加引用次数. 这个和 ZVAL_COPY 不同
		op1_str = zend_string_copy(Z_STR_P(op1));
	// 否则
	} else {
		// 如果 op1 是 编译变量 并且 值为未定义
		if (OP1_TYPE == IS_CV && UNEXPECTED(Z_TYPE_P(op1) == IS_UNDEF)) {
			// 报错：p1的变量（变量名）未定义, 并返未初始化zval
			ZVAL_UNDEFINED_OP1();
		}
		// zval 转成 字串, 没有try
		op1_str = zval_get_string_func(op1);
	}
	
	// 如果op2是常量
	if (OP2_TYPE == IS_CONST) {
		// 取出字串
		op2_str = Z_STR_P(op2);
	// op2是字串
	} else if (EXPECTED(Z_TYPE_P(op2) == IS_STRING)) {
		// op2 增加引用次数. 这个和 ZVAL_COPY 不同
		op2_str = zend_string_copy(Z_STR_P(op2));
	// 否则
	} else {
		// 如果 op2 是 编译变量 并且 类型为 未定义
		if (OP2_TYPE == IS_CV && UNEXPECTED(Z_TYPE_P(op2) == IS_UNDEF)) {
			// 报错：p2的变量（变量名）未定义, 并返未初始化zval
			ZVAL_UNDEFINED_OP2();
		}
		// zval 转成 字串, 没有try
		op2_str = zval_get_string_func(op2);
	}
	do {
		// 如果 OP1 不是常量
		if (OP1_TYPE != IS_CONST) {
			// 如果op1长度为0
			if (UNEXPECTED(ZSTR_LEN(op1_str) == 0)) {
				// 如果类型为 常量
				if (OP2_TYPE == IS_CONST) {
					// 如果是 可计数类型
					if (UNEXPECTED(Z_REFCOUNTED_P(op2))) {
						// 增加引用次数
						GC_ADDREF(op2_str);
					}
				}
				// 直接把op2放到结果里
				ZVAL_STR(EX_VAR(opline->result.var), op2_str);
				// 释放op1临时变量
				zend_string_release_ex(op1_str, 0);
				// 跳出
				break;
			}
		}
		// 如果 OP2 不是常量
		if (OP2_TYPE != IS_CONST) {
			// 如果op2长度为为
			if (UNEXPECTED(ZSTR_LEN(op2_str) == 0)) {
				// 如果op1是常量
				if (OP1_TYPE == IS_CONST) {
					// 如果是 可计数类型
					if (UNEXPECTED(Z_REFCOUNTED_P(op1))) {
						// 增加引用次数
						GC_ADDREF(op1_str);
					}
				}
				// op1直接放到结果里
				ZVAL_STR(EX_VAR(opline->result.var), op1_str);
				// 释放op2临时变量
				zend_string_release_ex(op2_str, 0);
				// 跳出
				break;
			}
		}
		// 分配新字串
		str = zend_string_alloc(ZSTR_LEN(op1_str) + ZSTR_LEN(op2_str), 0);
		// 先把op1放进来
		memcpy(ZSTR_VAL(str), ZSTR_VAL(op1_str), ZSTR_LEN(op1_str));
		// 再把op2放进来
		memcpy(ZSTR_VAL(str) + ZSTR_LEN(op1_str), ZSTR_VAL(op2_str), ZSTR_LEN(op2_str)+1);
		// 把zend_string 添加给 zval，不支持保留字
		ZVAL_NEW_STR(EX_VAR(opline->result.var), str);
		// 如果 OP1 不是常量
		if (OP1_TYPE != IS_CONST) {
			// 释放op1临时变量
			zend_string_release_ex(op1_str, 0);
		}
		// 如果 OP2 不是常量
		if (OP2_TYPE != IS_CONST) {
			// 释放op2临时变量
			zend_string_release_ex(op2_str, 0);
		}
	} while (0);
	// 释放操作对象的附加变量
	FREE_OP1();
	// 释放操作对象的附加变量
	FREE_OP2();
	// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, 初始化，字串里的变量列表，添加第一个变量
ZEND_VM_HANDLER(54, ZEND_ROPE_INIT, UNUSED, CONST|TMPVAR|CV, NUM)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zend_string **rope;
	zval *var;

	// 编译器已经分配了必要数量的zval位置来保存 rope
	/* Compiler allocates the necessary number of zval slots to keep the rope */
	// 取出rope，zend_string指针的指针
	rope = (zend_string**)EX_VAR(opline->result.var);
	// 如果类型为 常量
	if (OP2_TYPE == IS_CONST) {
		// 获取zval指针，UNUSED 类型返回null
		var = GET_OP2_ZVAL_PTR(BP_VAR_R);
		// 当成第一个rope
		rope[0] = Z_STR_P(var);
		// 如果是 可计数类型
		if (UNEXPECTED(Z_REFCOUNTED_P(var))) {
			// 引用次数+1
			Z_ADDREF_P(var);
		}
	// 否则,不是常量
	} else {
		// 获取op2的zval指针, UNUSED 返回null
		var = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
		// 如果类型是字串
		if (EXPECTED(Z_TYPE_P(var) == IS_STRING)) {
			// 如果 op2 是 编译变量
			if (OP2_TYPE == IS_CV) {
				// 取出字串，当作第一个rope
				// 增加引用次数. 这个和 ZVAL_COPY 不同
				rope[0] = zend_string_copy(Z_STR_P(var));
			// 否则
			} else {
				// 取出 字串，当作第一个rope
				rope[0] = Z_STR_P(var);
			}
		// 否则
		} else {
			// windows: 无操作
			SAVE_OPLINE();
			// 如果 op2 是 编译变量 并且 变量类型为未定义
			if (OP2_TYPE == IS_CV && UNEXPECTED(Z_TYPE_P(var) == IS_UNDEF)) {
				// 报错：p2的变量（变量名）未定义, 并返未初始化zval
				ZVAL_UNDEFINED_OP2();
			}
			// zval 转成 字串, 没有try
			rope[0] = zval_get_string_func(var);
			// 释放操作对象的附加变量
			FREE_OP2();
			// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
			ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
		}
	}
	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, 向字串变量列表中添加变量
ZEND_VM_HANDLER(55, ZEND_ROPE_ADD, TMP, CONST|TMPVAR|CV, NUM)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zend_string **rope;
	zval *var;

	/* op1 and result are the same */
	rope = (zend_string**)EX_VAR(opline->op1.var);
	// 如果类型为 常量
	if (OP2_TYPE == IS_CONST) {
		// 获取zval指针，UNUSED 类型返回null
		var = GET_OP2_ZVAL_PTR(BP_VAR_R);
		// 取出字串添加进列表
		rope[opline->extended_value] = Z_STR_P(var);
		// 如果是 可计数类型
		if (UNEXPECTED(Z_REFCOUNTED_P(var))) {
			// 引用次数+1
			Z_ADDREF_P(var);
		}
	// 否则，类型不是常量
	} else {
		// 获取zval指针, UNUSED 返回null
		var = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
		// 如果类型是字串
		if (EXPECTED(Z_TYPE_P(var) == IS_STRING)) {
			// 如果 op2 是 编译变量
			if (OP2_TYPE == IS_CV) {
				// 把字串添加到 rope列表里
				// 增加引用次数. 这个和 ZVAL_COPY 不同
				rope[opline->extended_value] = zend_string_copy(Z_STR_P(var));
			// 否则，op2 不是编译变量
			} else {
				// 把字串添加到rope列表中
				rope[opline->extended_value] = Z_STR_P(var);
			}
		// 否则，类型不是字串
		} else {
			// windows: 无操作
			SAVE_OPLINE();
			// 如果 op2 是 编译变量 并且 变量类型为 未定义
			if (OP2_TYPE == IS_CV && UNEXPECTED(Z_TYPE_P(var) == IS_UNDEF)) {
				// 报错：p2的变量（变量名）未定义, 并返未初始化zval
				ZVAL_UNDEFINED_OP2();
			}
			// zval 转成 字串, 没有try
			rope[opline->extended_value] = zval_get_string_func(var);
			// 释放操作对象的附加变量
			FREE_OP2();
			// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
			ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
		}
	}
	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, 添加元素并 把rope列表组合成字串
ZEND_VM_HANDLER(56, ZEND_ROPE_END, TMP, CONST|TMPVAR|CV, NUM)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zend_string **rope;
	zval *var, *ret;
	uint32_t i;
	size_t len = 0;
	char *target;
	// 取得rope位置
	rope = (zend_string**)EX_VAR(opline->op1.var);
	// 如果类型为 常量
	if (OP2_TYPE == IS_CONST) {
		// 获取zval指针，UNUSED 类型返回null
		var = GET_OP2_ZVAL_PTR(BP_VAR_R);
		// 取出字串，添加到rope列表里
		rope[opline->extended_value] = Z_STR_P(var);
		// 如果是 可计数类型
		if (UNEXPECTED(Z_REFCOUNTED_P(var))) {
			// 引用次数+1
			Z_ADDREF_P(var);
		}
	// 否则
	} else {
		// 获取zval指针, UNUSED 返回null
		var = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
		// 如果是字串
		if (EXPECTED(Z_TYPE_P(var) == IS_STRING)) {
			// 如果 op2 是 编译变量
			if (OP2_TYPE == IS_CV) {
				// 字串添加到rope里
				// 增加引用次数. 这个和 ZVAL_COPY 不同
				rope[opline->extended_value] = zend_string_copy(Z_STR_P(var));
			// 否则
			} else {
				// 字串添加到rope里
				rope[opline->extended_value] = Z_STR_P(var);
			}
		// 否则
		} else {
			// windows: 无操作
			SAVE_OPLINE();
			// 如果 op2 是 编译变量 并且 变量类型为 未定义
			if (OP2_TYPE == IS_CV && UNEXPECTED(Z_TYPE_P(var) == IS_UNDEF)) {
				// 报错：p2的变量（变量名）未定义, 并返未初始化zval
				ZVAL_UNDEFINED_OP2();
			}
			// zval 转成 字串, 没有try
			rope[opline->extended_value] = zval_get_string_func(var);
			// 释放操作对象的附加变量
			FREE_OP2();
			// 如果有异常
			if (UNEXPECTED(EG(exception))) {
				// 遍历所有rope
				for (i = 0; i <= opline->extended_value; i++) {
					// 释放所有rope
					zend_string_release_ex(rope[i], 0);
				}
				// 结果为 未定义
				ZVAL_UNDEF(EX_VAR(opline->result.var));
				// 主要是 return
				HANDLE_EXCEPTION();
			}
		}
	}
	// 遍历所有rope
	for (i = 0; i <= opline->extended_value; i++) {
		// 计算长度
		len += ZSTR_LEN(rope[i]);
	}
	// 在执行数据中取出 指定序号的变量
	ret = EX_VAR(opline->result.var);
	// 创建字串
	ZVAL_STR(ret, zend_string_alloc(len, 0));
	// 变量中的字串指针
	target = Z_STRVAL_P(ret);
	// 遍历所有rope
	for (i = 0; i <= opline->extended_value; i++) {
		// 一段一段地复制到新字串中
		memcpy(target, ZSTR_VAL(rope[i]), ZSTR_LEN(rope[i]));
		// 移到下一段
		target += ZSTR_LEN(rope[i]);
		// 释放复制过的rope
		zend_string_release_ex(rope[i], 0);
	}
	// 结束
	*target = '\0';

	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, 获取类.op1:类型，op2：类名
ZEND_VM_HANDLER(109, ZEND_FETCH_CLASS, UNUSED|CLASS_FETCH, CONST|TMPVAR|UNUSED|CV, CACHE_SLOT)
{
	zval *class_name;
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// windows: 无操作
	SAVE_OPLINE();
	// 如果 op2 类型是未定义
	if (OP2_TYPE == IS_UNUSED) {
		// 通过类名或类型查找类，p1:类名，p2:类型
		Z_CE_P(EX_VAR(opline->result.var)) = zend_fetch_class(NULL, opline->op1.num);
		// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
		ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
	// 否则， 如果类型为 常量
	} else if (OP2_TYPE == IS_CONST) {
		// 通过偏移量 在运行时缓存中获取一个 (void**) 中的第一个元素
		zend_class_entry *ce = CACHED_PTR(opline->extended_value);

		// 如果 类不存在
		if (UNEXPECTED(ce == NULL)) {
			// 获取zval指针, UNUSED 返回null
			class_name = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
			// 获取类，找不到会报错。p1:类名，p2:key，p3:flags
			ce = zend_fetch_class_by_name(Z_STR_P(class_name), Z_STR_P(class_name + 1), opline->op1.num);
			// 通过偏移量 在 EX(run_time_cache) 中获取一个 (void**) 中的第一个元素，并给它赋值
			CACHE_PTR(opline->extended_value, ce);
		}
		// 找到的类保存到结果中
		Z_CE_P(EX_VAR(opline->result.var)) = ce;
	// 否则
	} else {
		// 从op2里获取类名
		// 获取zval指针, UNUSED 返回null
		class_name = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
// 创建跳转标签 try_class_name
ZEND_VM_C_LABEL(try_class_name):
		// 如果类名是对象
		if (Z_TYPE_P(class_name) == IS_OBJECT) {
			// 把找到的类 保存到结果中
			Z_CE_P(EX_VAR(opline->result.var)) = Z_OBJCE_P(class_name);
		// 否则 ，如果类名是字串
		} else if (Z_TYPE_P(class_name) == IS_STRING) {
			// 通过类名或类型查找类，p1:类名，p2:类型
			Z_CE_P(EX_VAR(opline->result.var)) = zend_fetch_class(Z_STR_P(class_name), opline->op1.num);
		// 否则 ，如果op2是普通变量 或 编译变量 并且 类名是引用类型
		} else if ((OP2_TYPE & (IS_VAR|IS_CV)) && Z_TYPE_P(class_name) == IS_REFERENCE) {
			// 解引用
			class_name = Z_REFVAL_P(class_name);
			// 跳转到指定标签 try_class_name
			ZEND_VM_C_GOTO(try_class_name);
		// 否则
		} else {
			// 如果 op2 是 编译变量 类型类名为 未定义
			if (OP2_TYPE == IS_CV && UNEXPECTED(Z_TYPE_P(class_name) == IS_UNDEF)) {
				// 报错：p2的变量（变量名）未定义, 并返未初始化zval
				ZVAL_UNDEFINED_OP2();
				// 如果有异常
				if (UNEXPECTED(EG(exception) != NULL)) {
					// 主要是 return
					HANDLE_EXCEPTION();
				}
			}
			// 报错：类型必须是有效对象 或 字串
			zend_throw_error(NULL, "Class name must be a valid object or a string");
		}
	}

	// 释放操作对象的附加变量
	FREE_OP2();
	// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, 调用类方法
ZEND_VM_HOT_OBJ_HANDLER(112, ZEND_INIT_METHOD_CALL, CONST|TMPVAR|UNUSED|THIS|CV, CONST|TMPVAR|CV, NUM|CACHE_SLOT)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *function_name;
	zval *object;
	zend_function *fbc;
	zend_class_entry *called_scope;
	zend_object *obj;
	zend_execute_data *call;
	uint32_t call_info;

	// windows: 无操作
	SAVE_OPLINE();
	// 获取zval指针, UNUSED 返回 $this
	object = GET_OP1_OBJ_ZVAL_PTR_UNDEF(BP_VAR_R);

	// 如果 OP2 不是常量
	if (OP2_TYPE != IS_CONST) {
		// 获取zval指针, UNUSED 返回null
		function_name = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	}

	// 如果 OP2 不是常量 并且 函数名 不是 字串
	if (OP2_TYPE != IS_CONST &&
	    UNEXPECTED(Z_TYPE_P(function_name) != IS_STRING)) {
		// 为了break;
		do {
			// 否则 如果 op2 是普通变量或编译变量 并且 op2 是引用类型
			if ((OP2_TYPE & (IS_VAR|IS_CV)) && Z_ISREF_P(function_name)) {
				// 解引用
				function_name = Z_REFVAL_P(function_name);
				// 如果方法名是 字串
				if (EXPECTED(Z_TYPE_P(function_name) == IS_STRING)) {
					// 跳出
					break;
				}
			// 如果 op2 是 编译变量 并且 函数名 为未定义
			} else if (OP2_TYPE == IS_CV && UNEXPECTED(Z_TYPE_P(function_name) == IS_UNDEF)) {
				// 报错：p2的变量（变量名）未定义, 并返未初始化zval
				ZVAL_UNDEFINED_OP2();
				// 如果有异常
				if (UNEXPECTED(EG(exception) != NULL)) {
					// 释放操作对象的附加变量
					FREE_OP1();
					// 主要是 return
					HANDLE_EXCEPTION();
				}
			}
			// 抛错：方法名必须是字串
			zend_throw_error(NULL, "Method name must be a string");
			// 释放操作对象的附加变量
			FREE_OP2();
			// 释放操作对象的附加变量
			FREE_OP1();
			// 主要是 return
			HANDLE_EXCEPTION();
		} while (0);
	}
	
	// 如果 op1 类型是未定义
	if (OP1_TYPE == IS_UNUSED) {
		// 取出对象
		obj = Z_OBJ_P(object);
	// 否则
	} else {
		do {
			// 如果 OP1 不是常量 并且 对象类型是object
			if (OP1_TYPE != IS_CONST && EXPECTED(Z_TYPE_P(object) == IS_OBJECT)) {
				// 取出对象
				obj = Z_OBJ_P(object);
			// 否则，OP1是常量 或 object不是对象
			} else {
				// 如果 op1 是普通变量或编译变量 并且 op1是引用类型
				if ((OP1_TYPE & (IS_VAR|IS_CV)) && EXPECTED(Z_ISREF_P(object))) {
					// 取出引用实例
					zend_reference *ref = Z_REF_P(object);
					// 引用实例中的zval
					object = &ref->val;
					// 如果它是对象
					if (EXPECTED(Z_TYPE_P(object) == IS_OBJECT)) {
						// 取出对象
						obj = Z_OBJ_P(object);
						// 如果op1是变量
						if (OP1_TYPE & IS_VAR) {
							// 引用次数-1，如果为0
							if (UNEXPECTED(GC_DELREF(ref) == 0)) {
								// 释放 引用实例
								efree_size(ref, sizeof(zend_reference));
							// 否则
							} else {
								// 引用次数+1
								Z_ADDREF_P(object);
							}
						}
						// 跳出
						break;
					}
				}
				// 如果 op1 是 编译变量 并且对象值为未定义
				if (OP1_TYPE == IS_CV && UNEXPECTED(Z_TYPE_P(object) == IS_UNDEF)) {
					// 报错：p1的变量（变量名）未定义, 并返未初始化zval
					object = ZVAL_UNDEFINED_OP1();
					// 如果有异常
					if (UNEXPECTED(EG(exception) != NULL)) {
						// 如果 OP2 不是常量
						if (OP2_TYPE != IS_CONST) {
							// 释放操作对象的附加变量
							FREE_OP2();
						}
						// 主要是 return
						HANDLE_EXCEPTION();
					}
				}
				// 如果类型为 常量
				if (OP2_TYPE == IS_CONST) {
					// 获取zval指针, UNUSED 返回null
					function_name = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
				}
				// 报错：无效的调用
				zend_invalid_method_call(object, function_name);
				// 释放操作对象的附加变量
				FREE_OP2();
				// 释放操作对象的附加变量
				FREE_OP1();
				// 主要是 return
				HANDLE_EXCEPTION();
			}
		} while (0);
	}

	// 调用过的域
	called_scope = obj->ce;
	// 如果类型为 常量 并且 结果是调用过的域
	// 通过偏移量 在运行时缓存中获取一个 (void**) 中的第一个元素
	if (OP2_TYPE == IS_CONST &&
	    EXPECTED(CACHED_PTR(opline->result.num) == called_scope)) {
		// 找到方法
		// 通过偏移量 在运行时缓存中获取一个 (void**) 中的第一个元素
		fbc = CACHED_PTR(opline->result.num + sizeof(void*));
	// 否则
	} else {
		zend_object *orig_obj = obj;

		// 如果类型为 常量
		if (OP2_TYPE == IS_CONST) {
			// 获取zval指针, UNUSED 返回null
			function_name = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
		}

		// 首先，查找方法
		/* First, locate the function. */
		// 用 方法名 查找 方法
		fbc = obj->handlers->get_method(&obj, Z_STR_P(function_name), ((OP2_TYPE == IS_CONST) ? (RT_CONSTANT(opline, opline->op2) + 1) : NULL));
		// 如果没找到
		if (UNEXPECTED(fbc == NULL)) {
			// 如果没有异常
			if (EXPECTED(!EG(exception))) {
				// 报错：调用未定义的方法
				zend_undefined_method(obj->ce, Z_STR_P(function_name));
			}
			// 释放操作对象的附加变量
			FREE_OP2();
			// 引用次数-1，如果为0
			if ((OP1_TYPE & (IS_VAR|IS_TMP_VAR)) && GC_DELREF(orig_obj) == 0) {
				// 释放指定的对象，并更新 objects_store 中的指针
				zend_objects_store_del(orig_obj);
			}
			// 主要是 return
			HANDLE_EXCEPTION();
		}
		
		// 如果类型为 常量 并且 （函数不是通过弹跳调用 也不是不可缓存） 并且 对象是原对象
		if (OP2_TYPE == IS_CONST &&
		    EXPECTED(!(fbc->common.fn_flags & (ZEND_ACC_CALL_VIA_TRAMPOLINE|ZEND_ACC_NEVER_CACHE))) &&
		    EXPECTED(obj == orig_obj)) {
			// 找到运行时缓存的 p1 个位置，把p2 和p3依次保存进去	
			CACHE_POLYMORPHIC_PTR(opline->result.num, called_scope, fbc);
		}
		
		// 如果op1 是 变量 或 临时变量  并且 对象不是原来的对象
		if ((OP1_TYPE & (IS_VAR|IS_TMP_VAR)) && UNEXPECTED(obj != orig_obj)) {
			// 增加引用次数
			GC_ADDREF(obj); /* For $this pointer */
			// 引用次数-1，如果为0
			if (GC_DELREF(orig_obj) == 0) {
				// 释放指定的对象，并更新 objects_store 中的指针
				zend_objects_store_del(orig_obj);
			}
		}
		
		// 如果是用户定义函数 并且 没有 运行时缓存
		if (EXPECTED(fbc->type == ZEND_USER_FUNCTION) && UNEXPECTED(!RUN_TIME_CACHE(&fbc->op_array))) {
			// 为op_array创建并 初始化运行时缓存
			init_func_run_time_cache(&fbc->op_array);
		}
	}

	// 如果 OP2 不是常量
	if (OP2_TYPE != IS_CONST) {
		// 释放操作对象的附加变量
		FREE_OP2();
	}

	// 调用信息 【嵌套调用】+【有$this】
	call_info = ZEND_CALL_NESTED_FUNCTION | ZEND_CALL_HAS_THIS;
	// 如果 是静态方法
	if (UNEXPECTED((fbc->common.fn_flags & ZEND_ACC_STATIC) != 0)) {
		// 引用次数-1，如果为0
		if ((OP1_TYPE & (IS_VAR|IS_TMP_VAR)) && GC_DELREF(obj) == 0) {
			// 释放指定的对象，并更新 objects_store 中的指针
			zend_objects_store_del(obj);
			// 如果有异常
			if (UNEXPECTED(EG(exception))) {
				// 主要是 return
				HANDLE_EXCEPTION();
			}
		}
		// 调用静态方法
		/* call static method */
		obj = (zend_object*)called_scope;
		// 调用信息：【嵌套调用】
		call_info = ZEND_CALL_NESTED_FUNCTION;
	// 如果不是静态方法 并且 op1 是（变量 或 临时变量 或 编译变量）
	} else if (OP1_TYPE & (IS_VAR|IS_TMP_VAR|IS_CV)) {
		// 如果 op1 是 编译变量
		if (OP1_TYPE == IS_CV) {
			// 增加引用次数
			GC_ADDREF(obj); /* For $this pointer */
		}
		// 编译变量可以被间接改变（例如，如果有引用的时候）
		/* CV may be changed indirectly (e.g. when it's a reference) */
		// 调用信息 【嵌套调用】+【有$this】+【需要释放$this】
		call_info = ZEND_CALL_NESTED_FUNCTION | ZEND_CALL_HAS_THIS | ZEND_CALL_RELEASE_THIS;
	}
	// ing3, 初始化并返回 堆栈最上一个执行数据，空间不够则扩展大小
	// p1:调用信息，p2:函数，p3:参数数量，p4:对象或调用域
	call = zend_vm_stack_push_call_frame(call_info,
		fbc, opline->extended_value, obj);
	// 串联前一个执行数据
	call->prev_execute_data = EX(call);
	// 更新调用执行数据
	EX(call) = call;

	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, 初始化静态方法调用
ZEND_VM_HANDLER(113, ZEND_INIT_STATIC_METHOD_CALL, UNUSED|CLASS_FETCH|CONST|VAR, CONST|TMPVAR|UNUSED|CONSTRUCTOR|CV, NUM|CACHE_SLOT)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *function_name;
	zend_class_entry *ce;
	uint32_t call_info;
	zend_function *fbc;
	zend_execute_data *call;

	// windows: 无操作
	SAVE_OPLINE();

	// 如果op1是常量
	if (OP1_TYPE == IS_CONST) {
		/* no function found. try a static method in class */
		// 通过偏移量 在运行时缓存中获取一个 (void**) 中的第一个元素
		ce = CACHED_PTR(opline->result.num);
		// 如果 类不存在
		if (UNEXPECTED(ce == NULL)) {
			// 获取类，找不到会报错。p1:类名，p2:key，p3:flags
			ce = zend_fetch_class_by_name(Z_STR_P(RT_CONSTANT(opline, opline->op1)), Z_STR_P(RT_CONSTANT(opline, opline->op1) + 1), ZEND_FETCH_CLASS_DEFAULT | ZEND_FETCH_CLASS_EXCEPTION);
			// 如果 类不存在
			if (UNEXPECTED(ce == NULL)) {
				// 释放操作对象的附加变量
				FREE_OP2();
				// 主要是 return
				HANDLE_EXCEPTION();
			}
			// 如果 OP2 不是常量
			if (OP2_TYPE != IS_CONST) {
				// 通过偏移量 在 EX(run_time_cache) 中获取一个 (void**) 中的第一个元素，并给它赋值
				CACHE_PTR(opline->result.num, ce);
			}
		}
	// 否则 如果 op1 类型是未定义
	} else if (OP1_TYPE == IS_UNUSED) {
		// 通过类名或类型查找类，p1:类名，p2:类型
		ce = zend_fetch_class(NULL, opline->op1.num);
		// 如果 类不存在
		if (UNEXPECTED(ce == NULL)) {
			// 释放操作对象的附加变量
			FREE_OP2();
			// 主要是 return
			HANDLE_EXCEPTION();
		}
	// 否则
	} else {
		// 取出 类入口
		ce = Z_CE_P(EX_VAR(opline->op1.var));
	}

	// 如果两个 op 类型都为常量 并且 可以从缓存中取得此函数
	// 通过偏移量 在运行时缓存中获取一个 (void**) 中的第一个元素
	if (OP1_TYPE == IS_CONST &&
	    OP2_TYPE == IS_CONST &&
	    EXPECTED((fbc = CACHED_PTR(opline->result.num + sizeof(void*))) != NULL)) {
		// 什么也不做
		/* nothing to do */
	
	// 如果op1不是常量，op2是常量 并且 结果里存的是 找到的类 ce
	// 通过偏移量 在运行时缓存中获取一个 (void**) 中的第一个元素
	} else if (OP1_TYPE != IS_CONST &&
	           OP2_TYPE == IS_CONST &&
	           EXPECTED(CACHED_PTR(opline->result.num) == ce)) {
		// 通过偏移量 在运行时缓存中获取一个 (void**) 中的第一个元素
		fbc = CACHED_PTR(opline->result.num + sizeof(void*));
		
	// 如果op2的类型不是未使用
	} else if (OP2_TYPE != IS_UNUSED) {
		// 获取zval指针, UNUSED 返回null
		function_name = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
		// 如果 OP2 不是常量
		if (OP2_TYPE != IS_CONST) {
			// 如果函数名不是字串
			if (UNEXPECTED(Z_TYPE_P(function_name) != IS_STRING)) {
				// 为了break;
				do {
					// 否则 如果 op2 是普通变量或编译变量 并且 op2 是引用类型
					if (OP2_TYPE & (IS_VAR|IS_CV) && Z_ISREF_P(function_name)) {
						// 解引用
						function_name = Z_REFVAL_P(function_name);
						// 如果函数名是字串
						if (EXPECTED(Z_TYPE_P(function_name) == IS_STRING)) {
							// 跳出
							break;
						}
					// 如果 op2 是 编译变量 并且 函数名 为未定义
					} else if (OP2_TYPE == IS_CV && UNEXPECTED(Z_TYPE_P(function_name) == IS_UNDEF)) {
						// 报错：p2的变量（变量名）未定义, 并返未初始化zval
						ZVAL_UNDEFINED_OP2();
						// 如果有异常
						if (UNEXPECTED(EG(exception) != NULL)) {
							// 主要是 return
							HANDLE_EXCEPTION();
						}
					}
					// 报错：方法名必须是字串
					zend_throw_error(NULL, "Method name must be a string");
					// 释放操作对象的附加变量
					FREE_OP2();
					// 主要是 return
					HANDLE_EXCEPTION();
				} while (0);
			}
		}

		// 如果有获取静态方法的方法
		if (ce->get_static_method) {
			// 获取静态方法
			fbc = ce->get_static_method(ce, Z_STR_P(function_name));
		// 否则
		} else {
			// 使用标准方法获取静态方法，p1:类，p2:方法名，p3:key
				// op2是常量时有key
			fbc = zend_std_get_static_method(ce, Z_STR_P(function_name), ((OP2_TYPE == IS_CONST) ? (RT_CONSTANT(opline, opline->op2) + 1) : NULL));
		}
		// 如果没获取到方法
		if (UNEXPECTED(fbc == NULL)) {
			// 如果没有异常
			if (EXPECTED(!EG(exception))) {
				// 报错：调用未定义的方法
				zend_undefined_method(ce, Z_STR_P(function_name));
			}
			// 释放操作对象的附加变量
			FREE_OP2();
			// 主要是 return
			HANDLE_EXCEPTION();
		}
		// 如果op2是常量 并且 （没有【通过弹跳调用】或【不缓存】标记） 并且（不是在trait里）
		if (OP2_TYPE == IS_CONST &&
		    EXPECTED(!(fbc->common.fn_flags & (ZEND_ACC_CALL_VIA_TRAMPOLINE|ZEND_ACC_NEVER_CACHE))) &&
			EXPECTED(!(fbc->common.scope->ce_flags & ZEND_ACC_TRAIT))) {
			// 找到运行时缓存的 p1 个位置，把p2 和p3依次保存进去
			CACHE_POLYMORPHIC_PTR(opline->result.num, ce, fbc);
		}
		// 如果是用户定义函数 并且 没有 运行时缓存
		if (EXPECTED(fbc->type == ZEND_USER_FUNCTION) && UNEXPECTED(!RUN_TIME_CACHE(&fbc->op_array))) {
			// 为op_array创建并 初始化运行时缓存
			init_func_run_time_cache(&fbc->op_array);
		}
		// 如果 OP2 不是常量
		if (OP2_TYPE != IS_CONST) {
			// 释放操作对象的附加变量
			FREE_OP2();
		}
	// 情况4：	
	// 否则
	} else {
		// 如果类没有构造方法
		if (UNEXPECTED(ce->constructor == NULL)) {
			// 报错：无法调用构造方法
			zend_throw_error(NULL, "Cannot call constructor");
			// 主要是 return
			HANDLE_EXCEPTION();
		}
		// $this是对象 并且 构造方法不属于此类 并且 构造方法是私有的
		if (Z_TYPE(EX(This)) == IS_OBJECT && Z_OBJ(EX(This))->ce != ce->constructor->common.scope && (ce->constructor->common.fn_flags & ZEND_ACC_PRIVATE)) {
			// 报错：无法调用私有构造方法
			zend_throw_error(NULL, "Cannot call private %s::__construct()", ZSTR_VAL(ce->name));
			// 主要是 return
			HANDLE_EXCEPTION();
		}
		// 可以调用
		// 取出构造方法
		fbc = ce->constructor;
		// 如果是用户定义函数 并且 没有 运行时缓存
		if (EXPECTED(fbc->type == ZEND_USER_FUNCTION) && UNEXPECTED(!RUN_TIME_CACHE(&fbc->op_array))) {
			// 为op_array创建并 初始化运行时缓存
			init_func_run_time_cache(&fbc->op_array);
		}
	}

	// 如果不是静态方法
	if (!(fbc->common.fn_flags & ZEND_ACC_STATIC)) {
		// 如果$this是对象 并且 $this属于ce类
		if (Z_TYPE(EX(This)) == IS_OBJECT && instanceof_function(Z_OBJCE(EX(This)), ce)) {
			// $this 当类使用?
			ce = (zend_class_entry*)Z_OBJ(EX(This));
			// 调用信息里添加 【有$this】和【嵌套调用】两个标记
			call_info = ZEND_CALL_NESTED_FUNCTION | ZEND_CALL_HAS_THIS;
		// 否则
		} else {
			// 报错：静态调用非静态方法
			zend_non_static_method_call(fbc);
			// 主要是 return
			HANDLE_EXCEPTION();
		}
	// 否则，是静态方法
	} else {
		// 前一个操作码是 ZEND_FETCH_CLASS
		/* previous opcode is ZEND_FETCH_CLASS */
		// 如果 op1 类型是未定义 并且 （使用了parent或self关键字）
		if (OP1_TYPE == IS_UNUSED
		 && ((opline->op1.num & ZEND_FETCH_CLASS_MASK) == ZEND_FETCH_CLASS_PARENT ||
		     (opline->op1.num & ZEND_FETCH_CLASS_MASK) == ZEND_FETCH_CLASS_SELF)) {
			// 如果 $this是对象
			if (Z_TYPE(EX(This)) == IS_OBJECT) {
				// 取出 对象的类
				ce = Z_OBJCE(EX(This));
			// 否则, $this不是对象
			} else {
				// 使用 $this 类
				ce = Z_CE(EX(This));
			}
		}
		// 调用信息：嵌套调用
		call_info = ZEND_CALL_NESTED_FUNCTION;
	}

	// ing3, 初始化并返回 堆栈最上一个执行数据，空间不够则扩展大小
	// p1:调用信息，p2:函数，p3:参数数量，p4:对象或调用域
	call = zend_vm_stack_push_call_frame(call_info,
		fbc, opline->extended_value, ce);
	// 串联前一个执行数据
	call->prev_execute_data = EX(call);
	// 把调用的执行数据，改成新执行数据
	EX(call) = call;

	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, 使用名称调用函数
ZEND_VM_HOT_HANDLER(59, ZEND_INIT_FCALL_BY_NAME, ANY, CONST, NUM|CACHE_SLOT)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zend_function *fbc;
	zval *function_name, *func;
	zend_execute_data *call;

	// 通过偏移量 在运行时缓存中获取一个 (void**) 中的第一个元素
	fbc = CACHED_PTR(opline->result.num);
	// 如果没获取到函数
	if (UNEXPECTED(fbc == NULL)) {
		// op2是函数名
		// 访问 (p2).zv。p1:opline,p2:node
		function_name = (zval*)RT_CONSTANT(opline, opline->op2);
		// 取出函数
		func = zend_hash_find_known_hash(EG(function_table), Z_STR_P(function_name+1));
		// 如果没有此函数
		if (UNEXPECTED(func == NULL)) {
			// 报错，没用了未定义的函数
			// 调用方法 zend_undefined_function_helper
			ZEND_VM_DISPATCH_TO_HELPER(zend_undefined_function_helper);
		}
		// 取出函数
		fbc = Z_FUNC_P(func);
		// 如果是用户定义 并且 没有运行时缓存
		if (EXPECTED(fbc->type == ZEND_USER_FUNCTION) && UNEXPECTED(!RUN_TIME_CACHE(&fbc->op_array))) {
			// 为op_array创建并 初始化运行时缓存
			init_func_run_time_cache(&fbc->op_array);
		}
		// 通过偏移量 在 EX(run_time_cache) 中获取一个 (void**) 中的第一个元素，并给它赋值
		CACHE_PTR(opline->result.num, fbc);
	}
	// ing3, 初始化并返回 堆栈最上一个执行数据，空间不够则扩展大小
	// p1:调用信息，p2:函数，p3:参数数量，p4:对象或调用域
	call = _zend_vm_stack_push_call_frame(ZEND_CALL_NESTED_FUNCTION,
		fbc, opline->extended_value, NULL);
	// 新执行数据的 链接前一个执行数据：发起调用的执行数据
	call->prev_execute_data = EX(call);
	// 把正在调用的执行数据，改成新执行数据
	EX(call) = call;

	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing2, 初始化动态调用执行数据
ZEND_VM_HANDLER(128, ZEND_INIT_DYNAMIC_CALL, ANY, CONST|TMPVAR|CV, NUM)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *function_name;
	zend_execute_data *call;

	// windows: 无操作
	SAVE_OPLINE();
	// 获取zval指针, UNUSED 返回null
	function_name = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);

// 创建跳转标签 try_function_name
ZEND_VM_C_LABEL(try_function_name):
	// 如果 OP2 不是常量 并且 函数名 是字串
	if (OP2_TYPE != IS_CONST && EXPECTED(Z_TYPE_P(function_name) == IS_STRING)) {
		// 通过字串创建调用 执行数据，不要弹跳创建的。p1:调用方法名（函数名或静态方法名），p2:参数数量
		call = zend_init_dynamic_call_string(Z_STR_P(function_name), opline->extended_value);
	
	// 否则 如果 OP2 不是常量 并且 函数名 是对象
	} else if (OP2_TYPE != IS_CONST && EXPECTED(Z_TYPE_P(function_name) == IS_OBJECT)) {
		// 创建调用闭包（或假闭包）执行数据。p1:用来获取闭包的对象，p2:参数数量
		call = zend_init_dynamic_call_object(Z_OBJ_P(function_name), opline->extended_value);
	
	// 否则 如果函数名是数组
	} else if (EXPECTED(Z_TYPE_P(function_name) == IS_ARRAY)) {
		// 从数组，初始化动态调用执行数据。p1:调用数组，p2:参数数量
		call = zend_init_dynamic_call_array(Z_ARRVAL_P(function_name), opline->extended_value);
	
	// 否则 如果 op2 是普通变量或编译变量 并且 op2 是引用类型
	} else if ((OP2_TYPE & (IS_VAR|IS_CV)) && EXPECTED(Z_TYPE_P(function_name) == IS_REFERENCE)) {
		// 解引用
		function_name = Z_REFVAL_P(function_name);
		// 跳转到指定标签 try_function_name
		ZEND_VM_C_GOTO(try_function_name);
	
	// 否则，无法调用，报错
	} else {
		// 如果 op2 是 编译变量 并且 函数名 为未定义
		if (OP2_TYPE == IS_CV && UNEXPECTED(Z_TYPE_P(function_name) == IS_UNDEF)) {
			// 报错：p2的变量（变量名）未定义, 并返未初始化zval
			function_name = ZVAL_UNDEFINED_OP2();
			// 如果有异常
			if (UNEXPECTED(EG(exception) != NULL)) {
				// 主要是 return
				HANDLE_EXCEPTION();
			}
		}
		// 抛错：此类型的值不可调用
		zend_throw_error(NULL, "Value of type %s is not callable",
			zend_zval_type_name(function_name));
		// 没有执行数据
		call = NULL;
	}

	// 如果op2是 变量 或 临时变量
	if (OP2_TYPE & (IS_VAR|IS_TMP_VAR)) {
		// 释放操作对象的附加变量
		FREE_OP2();
		// 并且有异常
		if (UNEXPECTED(EG(exception))) {
			// 如果有执行数据
			if (call) {
				// 如果是通过弹跳创建的
				if (call->func->common.fn_flags & ZEND_ACC_CALL_VIA_TRAMPOLINE) {
					// 释放函数名
					zend_string_release_ex(call->func->common.function_name, 0);
					// 释放弹跳函数
					zend_free_trampoline(call->func);
				}
				// 释放调用框架（执行数据）。p1:执行数据
				zend_vm_stack_free_call_frame(call);
			}
			// 主要是 return
			HANDLE_EXCEPTION();
		}
	// 否则，如果没有执行数据
	} else if (!call) {
		// 主要是 return
		HANDLE_EXCEPTION();
	}

	// 连接前一个执行数据？
	call->prev_execute_data = EX(call);
	// 正在调用的执行数据？
	EX(call) = call;

	// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing2, 初始化用户调用执行数据
ZEND_VM_HANDLER(118, ZEND_INIT_USER_CALL, CONST, CONST|TMPVAR|CV, NUM)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *function_name;
	zend_fcall_info_cache fcc;
	char *error = NULL;
	zend_function *func;
	void *object_or_called_scope;
	zend_execute_data *call;
	uint32_t call_info = ZEND_CALL_NESTED_FUNCTION | ZEND_CALL_DYNAMIC;

	// windows: 无操作
	SAVE_OPLINE();
	// 获取zval指针，UNUSED 类型返回null
	function_name = GET_OP2_ZVAL_PTR(BP_VAR_R);
	// 检查此调用在 当前指定执行数据中 是否可行
	// p1:调用信息，p2:对象, p3:检验级别，p4:返回调用字串，p5:fcc,关键是它 ，p6:返回错误信息
	if (zend_is_callable_ex(function_name, NULL, 0, NULL, &fcc, &error)) {
		// 不可以有错误
		ZEND_ASSERT(!error);
		// 如果有调用函数
		func = fcc.function_handler;
		// 域
		object_or_called_scope = fcc.called_scope;
		// 如果函数是闭包
		if (func->common.fn_flags & ZEND_ACC_CLOSURE) {
			/* Delay closure destruction until its invocation */
			// 增加引用次数
			GC_ADDREF(ZEND_CLOSURE_OBJECT(func));
			// 添加 调用闭包 标记
			call_info |= ZEND_CALL_CLOSURE;
			// 如果 函数 是假闭包
			if (func->common.fn_flags & ZEND_ACC_FAKE_CLOSURE) {
				// 调用信息里添加 假闭包标记
				call_info |= ZEND_CALL_FAKE_CLOSURE;
			}
			// 如果调用信息缓存里有 被调用对象
			if (fcc.object) {
				// 被调用对象
				object_or_called_scope = fcc.object;
				// 调用信息 添加 【有$this】 标记
				call_info |= ZEND_CALL_HAS_THIS;
			}
		// 如果不是闭包 并且 有 被调用对象
		} else if (fcc.object) {
			// 增加引用次数
			GC_ADDREF(fcc.object); /* For $this pointer */
			// 被调用对象
			object_or_called_scope = fcc.object;
			// 添加 【有$this】和【需要释放$this】标记
			call_info |= ZEND_CALL_RELEASE_THIS | ZEND_CALL_HAS_THIS;
		}

		// 释放操作对象的附加变量
		FREE_OP2();
		// 如果op2是 临时变量 或 普通变量 并且有 异常
		if ((OP2_TYPE & (IS_TMP_VAR|IS_VAR)) && UNEXPECTED(EG(exception))) {
			// 如果 调用信息里 有 调用闭包标记
			if (call_info & ZEND_CALL_CLOSURE) {
				// 释放闭包对象
				// 从 _zend_closure.zend_function 找到 _zend_closure 开头。
				zend_object_release(ZEND_CLOSURE_OBJECT(func));
			// 如果需要释放 【$this】
			} else if (call_info & ZEND_CALL_RELEASE_THIS) {
				// 释放调用信息缓存中的 object
				zend_object_release(fcc.object);
			}
			// 主要是 return
			HANDLE_EXCEPTION();
		}
		// 如果是用户定义函数 并且 没有 运行时缓存
		if (EXPECTED(func->type == ZEND_USER_FUNCTION) && UNEXPECTED(!RUN_TIME_CACHE(&func->op_array))) {
			// 为op_array创建并 初始化运行时缓存
			init_func_run_time_cache(&func->op_array);
		}
	// 否则，如果调用不可行
	} else {
		// 报错，参数1 必须是一个有效的 callback
		// 访问 (p2).zv。p1:opline,p2:node
		zend_type_error("%s(): Argument #1 ($callback) must be a valid callback, %s", Z_STRVAL_P(RT_CONSTANT(opline, opline->op1)), error);
		// 释放错误
		efree(error);
		// 释放操作对象的附加变量
		FREE_OP2();
		// 主要是 return
		HANDLE_EXCEPTION();
	}

	// 初始化并返回 堆栈最上一个执行数据，空间不够则扩展大小
	// p1:调用信息，p2:函数，p3:参数数量，p4:对象或调用域
	call = zend_vm_stack_push_call_frame(call_info,
		func, opline->extended_value, object_or_called_scope);
	// 关联前一个执行数据
	call->prev_execute_data = EX(call);
	// 新的执行数据 作为调用执行数据
	EX(call) = call;

	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing2, 初始化按 带命名空间的 名称调用函数
ZEND_VM_HOT_HANDLER(69, ZEND_INIT_NS_FCALL_BY_NAME, ANY, CONST, NUM|CACHE_SLOT)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *func_name;
	zval *func;
	zend_function *fbc;
	zend_execute_data *call;

	// 通过偏移量 在运行时缓存中获取一个 (void**) 中的第一个元素
	fbc = CACHED_PTR(opline->result.num);
	// 如果没找到函数
	if (UNEXPECTED(fbc == NULL)) {
		// 函数名
		// 访问 (p2).zv。p1:opline,p2:node
		func_name = (zval *)RT_CONSTANT(opline, opline->op2);
		// 使用下一个函数名取得函数实例
		func = zend_hash_find_known_hash(EG(function_table), Z_STR_P(func_name + 1));
		// 如果找不到
		if (func == NULL) {
			// 使用下2个函数名 取得函数实例
			func = zend_hash_find_known_hash(EG(function_table), Z_STR_P(func_name + 2));
			// 如果找不到
			if (UNEXPECTED(func == NULL)) {
				// 报错，没用了未定义的函数
				// 调用方法 zend_undefined_function_helper
				ZEND_VM_DISPATCH_TO_HELPER(zend_undefined_function_helper);
			}
		}
		// 取出函数实例
		fbc = Z_FUNC_P(func);
		// 如果是用户定义函数 并且 没有 运行时缓存
		if (EXPECTED(fbc->type == ZEND_USER_FUNCTION) && UNEXPECTED(!RUN_TIME_CACHE(&fbc->op_array))) {
			// 为op_array创建并 初始化运行时缓存
			init_func_run_time_cache(&fbc->op_array);
		}
		// 通过偏移量 在 EX(run_time_cache) 中获取一个 (void**) 中的第一个元素，并给它赋值
		CACHE_PTR(opline->result.num, fbc);
	}

	// ing3, 初始化并返回 堆栈最上一个执行数据，空间不够则扩展大小
	// p1:调用信息，p2:函数，p3:参数数量，p4:对象或调用域
	call = _zend_vm_stack_push_call_frame(ZEND_CALL_NESTED_FUNCTION,
		fbc, opline->extended_value, NULL);
	// 连接前一个执行数据
	call->prev_execute_data = EX(call);
	// 新执行数据为作调用的执行数据
	EX(call) = call;

	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing2, 初始化函数调用
ZEND_VM_HOT_HANDLER(61, ZEND_INIT_FCALL, NUM, CONST, NUM|CACHE_SLOT)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *fname;
	zval *func;
	zend_function *fbc;
	zend_execute_data *call;

	// 通过偏移量 在运行时缓存中获取一个 (void**) 中的第一个元素
	fbc = CACHED_PTR(opline->result.num);
	// 如果没有函数 实例
	if (UNEXPECTED(fbc == NULL)) {
		// 取出op2 函数名
		// 访问 (p2).zv。p1:opline,p2:node
		fname = (zval*)RT_CONSTANT(opline, opline->op2);
		// 使用函数名查找函数
		func = zend_hash_find_known_hash(EG(function_table), Z_STR_P(fname));
		// 必须找到
		ZEND_ASSERT(func != NULL && "Function existence must be checked at compile time");
		// 取出函数实例
		fbc = Z_FUNC_P(func);
		// 如果是用户定义函数 并且 没有 运行时缓存
		if (EXPECTED(fbc->type == ZEND_USER_FUNCTION) && UNEXPECTED(!RUN_TIME_CACHE(&fbc->op_array))) {
			// 为op_array创建并 初始化运行时缓存
			init_func_run_time_cache(&fbc->op_array);
		}
		// 通过偏移量 在 EX(run_time_cache) 中获取一个 (void**) 中的第一个元素，并给它赋值
		CACHE_PTR(opline->result.num, fbc);
	}

	// 初始化并返回 堆栈最上一个执行数据，空间不够则扩展大小
	// p1:使用大小，p2:调用信息，p3:函数，p4:参数数量，p5:对象或调用域
	call = _zend_vm_stack_push_call_frame_ex(
		opline->op1.num, ZEND_CALL_NESTED_FUNCTION,
		fbc, opline->extended_value, NULL);
	// 连接前一个执行数据
	call->prev_execute_data = EX(call);
	// 新执行数据作为调用的执行数据
	EX(call) = call;

	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing2, 调用内置函数
ZEND_VM_HOT_HANDLER(129, ZEND_DO_ICALL, ANY, ANY, SPEC(RETVAL,OBSERVER))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	// 正在调用的执行数据
	zend_execute_data *call = EX(call);
	// 正在调用的函数
	zend_function *fbc = call->func;
	zval *ret;
	zval retval;

	// windows: 无操作
	SAVE_OPLINE();
	// 正在调用的执行数据 ，切换到前一个执行数据
	EX(call) = call->prev_execute_data;

	// 串连前一个执行数据
	call->prev_execute_data = execute_data;
	// 转到被调用的执行数据
	EG(current_execute_data) = call;

// 调试用
#if ZEND_DEBUG
	bool should_throw = zend_internal_call_should_throw(fbc, call);
#endif

	// 如果 操作码的运算结果有效(不是IS_UNUSED) ？操作码的结果 ：临时变量
	ret = RETURN_VALUE_USED(opline) ? EX_VAR(opline->result.var) : &retval;
	// 结果置空
	ZVAL_NULL(ret);
	// 开始观察者调用
	// "/ZEND_OBSERVER_FCALL_BEGIN\(\s*(.*)\s*\)/" => isset($extra_spec['OBSERVER']) ?            ($extra_spec['OBSERVER'] == 0 ? "" : "zend_observer_fcall_begin(\\1)")            : "",
	ZEND_OBSERVER_FCALL_BEGIN(call);
	// 调用内置函数
	fbc->internal_function.handler(call, ret);

// 调试用
#if ZEND_DEBUG
	if (!EG(exception) && call->func) {
		if (should_throw) {
			zend_internal_call_arginfo_violation(call->func);
		}
		ZEND_ASSERT(!(call->func->common.fn_flags & ZEND_ACC_HAS_RETURN_TYPE) ||
			zend_verify_internal_return_type(call->func, ret));
		ZEND_ASSERT((call->func->common.fn_flags & ZEND_ACC_RETURN_REFERENCE)
			? Z_ISREF_P(ret) : !Z_ISREF_P(ret));
		zend_verify_internal_func_info(call->func, ret);
	}
#endif
	// 如果用到了观察者，结束观察者调用
	// "/ZEND_OBSERVER_FCALL_END\(\s*([^,]*)\s*,\s*(.*)\s*\)/" => isset($extra_spec['OBSERVER']) ?            ($extra_spec['OBSERVER'] == 0 ? "" : "zend_observer_fcall_end(\\1, \\2)")            : "",
	ZEND_OBSERVER_FCALL_END(call, EG(exception) ? NULL : ret);

	// 切换到此执行数据
	EG(current_execute_data) = execute_data;
	// 释放所有执行数据中的 函数参数
	zend_vm_stack_free_args(call);
	
	// 获取调用信息
	uint32_t call_info = ZEND_CALL_INFO(call);
	// 如果有标记【有额外命名参数】或【调用分配】
	if (UNEXPECTED(call_info & (ZEND_CALL_HAS_EXTRA_NAMED_PARAMS|ZEND_CALL_ALLOCATED))) {
		// 如果有标记【有额外命名参数】
		if (call_info & ZEND_CALL_HAS_EXTRA_NAMED_PARAMS) {
			// 释放 额外命名参数
			zend_free_extra_named_params(call->extra_named_params);
		}
		// 释放调用框架（执行数据），p1:调用信息，p2:执行数据
		zend_vm_stack_free_call_frame_ex(call_info, call);
	// 否则
	} else {
		// call 放到虚拟机堆栈顶层
		EG(vm_stack_top) = (zval*)call;
	}

	// 如果 操作码的运算结果 无效(是IS_UNUSED)
	if (!RETURN_VALUE_USED(opline)) {
		// 销毁ret
		i_zval_ptr_dtor(ret);
	}

	// 如果有异常
	if (UNEXPECTED(EG(exception) != NULL)) {
		// 重新抛异常
		zend_rethrow_exception(execute_data);
		// 主要是 return
		HANDLE_EXCEPTION();
	}
	// 修改当前操作码并返回。EX(opline) = p1 
	ZEND_VM_SET_OPCODE(opline + 1);
	// windows: return  0
	ZEND_VM_CONTINUE();
}

// ing2, 调用用户函数
ZEND_VM_HOT_HANDLER(130, ZEND_DO_UCALL, ANY, ANY, SPEC(RETVAL,OBSERVER))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	// 调用的 执行数据
	zend_execute_data *call = EX(call);
	// 当前执行函数
	zend_function *fbc = call->func;
	zval *ret;

	// windows: 无操作
	SAVE_OPLINE();
	// 更新 EX(call) 执行数据为前一个
	EX(call) = call->prev_execute_data;
	// 初始为null
	ret = NULL;
	// 如果 操作码的运算结果有效(不是IS_UNUSED)
	if (RETURN_VALUE_USED(opline)) {
		// 在执行数据中取出 指定序号的变量
		ret = EX_VAR(opline->result.var);
	}
	// 串联前一个执行数据
	call->prev_execute_data = execute_data;
	// 切换当前执行数据
	execute_data = call;
	// 初始化函数的执行数据，p1:操作码组，p2:返回值
	i_init_func_execute_data(&fbc->op_array, ret, 0 EXECUTE_DATA_CC);
	// windows: 无操作
	LOAD_OPLINE_EX();
	// zend_vm_gen.php 处理 如果开启了观察者, SAVE_OPLINE()
	// "/ZEND_OBSERVER_SAVE_OPLINE\(\)/" => isset($extra_spec['OBSERVER']) && $extra_spec['OBSERVER'] == 1 ? "SAVE_OPLINE()" : "",
	ZEND_OBSERVER_SAVE_OPLINE();
	// zend_vm_gen.php 处理 如果开启了观察者，开始观察者调用: zend_observer_fcall_begin()
	// "/ZEND_OBSERVER_FCALL_BEGIN\(\s*(.*)\s*\)/" => isset($extra_spec['OBSERVER']) ?            ($extra_spec['OBSERVER'] == 0 ? "" : "zend_observer_fcall_begin(\\1)")            : "",
	ZEND_OBSERVER_FCALL_BEGIN(execute_data);
	// return  1
	ZEND_VM_ENTER_EX();
}

// ing2,
ZEND_VM_HOT_HANDLER(131, ZEND_DO_FCALL_BY_NAME, ANY, ANY, SPEC(RETVAL,OBSERVER))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	// 调用的执行数据
	zend_execute_data *call = EX(call);
	// 调用的函数
	zend_function *fbc = call->func;
	zval *ret;

	// windows: 无操作
	SAVE_OPLINE();
	// 转回到前一个执行数据
	EX(call) = call->prev_execute_data;

	// 如果是用户函数
	if (EXPECTED(fbc->type == ZEND_USER_FUNCTION)) {
		// 结果为null
		ret = NULL;
		// 如果 操作码的运算结果有效(不是IS_UNUSED)
		if (RETURN_VALUE_USED(opline)) {
			// 在执行数据中取出 指定序号的变量
			ret = EX_VAR(opline->result.var);
		}
		// 更新前一个执行数据
		call->prev_execute_data = execute_data;
		// 更新当前执行数据
		execute_data = call;
		// 初始化函数的执行数据，p1:操作码组，p2:返回值
		i_init_func_execute_data(&fbc->op_array, ret, 0 EXECUTE_DATA_CC);
		// windows: 无操作
		LOAD_OPLINE_EX();
		// zend_vm_gen.php 处理
		// "/ZEND_OBSERVER_SAVE_OPLINE\(\)/" => isset($extra_spec['OBSERVER']) && $extra_spec['OBSERVER'] == 1 ? "SAVE_OPLINE()" : "",
		ZEND_OBSERVER_SAVE_OPLINE();
		// 开始观察者调用
		// "/ZEND_OBSERVER_FCALL_BEGIN\(\s*(.*)\s*\)/" => isset($extra_spec['OBSERVER']) ?            ($extra_spec['OBSERVER'] == 0 ? "" : "zend_observer_fcall_begin(\\1)")            : "",
		ZEND_OBSERVER_FCALL_BEGIN(execute_data);
		// return  1
		ZEND_VM_ENTER_EX();
	// 否则，不是用户函数
	} else {
		zval retval;
		// 必须 是内置函数
		ZEND_ASSERT(fbc->type == ZEND_INTERNAL_FUNCTION);
		// 如果开启了观察者
		// "/ZEND_OBSERVER_ENABLED/" => isset($extra_spec['OBSERVER']) && $extra_spec['OBSERVER'] == 1 ? "1" : "0",
		if (ZEND_OBSERVER_ENABLED) {
			// 结果为null
			ret = NULL;
		}
		// 如果有弃用标记
		if (UNEXPECTED((fbc->common.fn_flags & ZEND_ACC_DEPRECATED) != 0)) {
			// 报错：函数或方法已弃用
			zend_deprecated_function(fbc);
			// 如果有异常
			if (UNEXPECTED(EG(exception) != NULL)) {
				// 如果 opline 结果类型是 变量或临时变量，把结果 zval 标记为未定义
				UNDEF_RESULT();
				// 如果 操作码的运算结果无效(是IS_UNUSED)
				if (!RETURN_VALUE_USED(opline)) {
					// 使用临时结果
					ret = &retval;
					// 结果为 未定义
					ZVAL_UNDEF(ret);
				}
				// 跳转到指定标签 fcall_by_name_end
				ZEND_VM_C_GOTO(fcall_by_name_end);
			}
		}

		// 串连执行数据
		call->prev_execute_data = execute_data;
		// 切换当前执行数据
		EG(current_execute_data) = call;

#if ZEND_DEBUG
		bool should_throw = zend_internal_call_should_throw(fbc, call);
#endif

		// 如果 操作码的运算结果有效(不是IS_UNUSED) ？？
		ret = RETURN_VALUE_USED(opline) ? EX_VAR(opline->result.var) : &retval;
		// ret = null
		ZVAL_NULL(ret);
		// 开始观察者调用
		// "/ZEND_OBSERVER_FCALL_BEGIN\(\s*(.*)\s*\)/" => isset($extra_spec['OBSERVER']) ?            ($extra_spec['OBSERVER'] == 0 ? "" : "zend_observer_fcall_begin(\\1)")            : "",
		ZEND_OBSERVER_FCALL_BEGIN(call);
		// 调用内置函数
		fbc->internal_function.handler(call, ret);

#if ZEND_DEBUG
		if (!EG(exception) && call->func) {
			if (should_throw) {
				zend_internal_call_arginfo_violation(call->func);
			}
			ZEND_ASSERT(!(call->func->common.fn_flags & ZEND_ACC_HAS_RETURN_TYPE) ||
				zend_verify_internal_return_type(call->func, ret));
			ZEND_ASSERT((call->func->common.fn_flags & ZEND_ACC_RETURN_REFERENCE)
				? Z_ISREF_P(ret) : !Z_ISREF_P(ret));
			zend_verify_internal_func_info(call->func, ret);
		}
#endif
		// 如果用到了观察者，结束观察者调用
		// "/ZEND_OBSERVER_FCALL_END\(\s*([^,]*)\s*,\s*(.*)\s*\)/" => isset($extra_spec['OBSERVER']) ?            ($extra_spec['OBSERVER'] == 0 ? "" : "zend_observer_fcall_end(\\1, \\2)")            : "",
		ZEND_OBSERVER_FCALL_END(call, EG(exception) ? NULL : ret);

		// 切换 当前执行数据
		EG(current_execute_data) = execute_data;

// 创建跳转标签 fcall_by_name_end
ZEND_VM_C_LABEL(fcall_by_name_end):
		// 释放所有执行数据中的 函数参数
		zend_vm_stack_free_args(call);

		uint32_t call_info = ZEND_CALL_INFO(call);
		// 如果有 【包含额外参数】 或 【调用分配】 标记
		if (UNEXPECTED(call_info & (ZEND_CALL_HAS_EXTRA_NAMED_PARAMS|ZEND_CALL_ALLOCATED))) {
			// 如果有【包含额外参数】 标记
			if (call_info & ZEND_CALL_HAS_EXTRA_NAMED_PARAMS) {
				// 释放额外参数
				zend_free_extra_named_params(call->extra_named_params);
			}
			// 释放调用框架（执行数据），p1:调用信息，p2:执行数据
			zend_vm_stack_free_call_frame_ex(call_info, call);
		// 否则
		} else {
			// 当前执行数据放在虚拟机堆栈顶层
			EG(vm_stack_top) = (zval*)call;
		}

		// 如果 操作码的运算结果无效(是IS_UNUSED)
		if (!RETURN_VALUE_USED(opline)) {
			// 销毁 ret
			i_zval_ptr_dtor(ret);
		}
	}

	// 如果有异常
	if (UNEXPECTED(EG(exception) != NULL)) {
		// 重新抛错
		zend_rethrow_exception(execute_data);
		// 主要是 return
		HANDLE_EXCEPTION();
	}
	// 修改当前操作码并返回。EX(opline) = p1 
	ZEND_VM_SET_OPCODE(opline + 1);
	// windows: return  0
	ZEND_VM_CONTINUE();
}

// ing2, 调用函数
ZEND_VM_HOT_HANDLER(60, ZEND_DO_FCALL, ANY, ANY, SPEC(RETVAL,OBSERVER))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zend_execute_data *call = EX(call);
	zend_function *fbc = call->func;
	zval *ret;

	// windows: 无操作
	SAVE_OPLINE();
	// 正在调用的转到前一个执行数据
	EX(call) = call->prev_execute_data;

	// 如果调用用户函数
	if (EXPECTED(fbc->type == ZEND_USER_FUNCTION)) {
		// 结果为null
		ret = NULL;
		// 如果 操作码的运算结果有效(不是IS_UNUSED)
		if (RETURN_VALUE_USED(opline)) {
			// 在执行数据中取出 指定序号的变量
			ret = EX_VAR(opline->result.var);
		}

		// 链接前一个执行数据
		call->prev_execute_data = execute_data;
		// 切换执行数据 
		execute_data = call;
		// 初始化函数的执行数据，p1:操作码组，p2:返回值
		i_init_func_execute_data(&fbc->op_array, ret, 1 EXECUTE_DATA_CC);

		// 如果 执行函数是 execute_ex
		if (EXPECTED(zend_execute_ex == execute_ex)) {
			// windows: 无操作
			LOAD_OPLINE_EX();
			// zend_vm_gen.php 处理
			// "/ZEND_OBSERVER_SAVE_OPLINE\(\)/" => isset($extra_spec['OBSERVER']) && $extra_spec['OBSERVER'] == 1 ? "SAVE_OPLINE()" : "",
			ZEND_OBSERVER_SAVE_OPLINE();
			// 开始观察者调用
			// "/ZEND_OBSERVER_FCALL_BEGIN\(\s*(.*)\s*\)/" => isset($extra_spec['OBSERVER']) ?            ($extra_spec['OBSERVER'] == 0 ? "" : "zend_observer_fcall_begin(\\1)")            : "",
			ZEND_OBSERVER_FCALL_BEGIN(execute_data);
			// return  1
			ZEND_VM_ENTER_EX();
		// 否则，使用 zend_execute_ex
		} else {
			// windows: 无操作
			SAVE_OPLINE_EX();
			// 开始观察者调用
			// "/ZEND_OBSERVER_FCALL_BEGIN\(\s*(.*)\s*\)/" => isset($extra_spec['OBSERVER']) ?            ($extra_spec['OBSERVER'] == 0 ? "" : "zend_observer_fcall_begin(\\1)")            : "",
			ZEND_OBSERVER_FCALL_BEGIN(execute_data);
			// 切换执行数据到前一个
			execute_data = EX(prev_execute_data);
			// windows: 无操作
			LOAD_OPLINE();
			// 添加标记【顶层调用】
			ZEND_ADD_CALL_FLAG(call, ZEND_CALL_TOP);
			// 其他执行函数
			zend_execute_ex(call);
		}
	// 否则，调用内置函数
	} else {
		zval retval;
		// 必须是调用内置函数
		ZEND_ASSERT(fbc->type == ZEND_INTERNAL_FUNCTION);
		// "/ZEND_OBSERVER_ENABLED/" => isset($extra_spec['OBSERVER']) && $extra_spec['OBSERVER'] == 1 ? "1" : "0",
		if (ZEND_OBSERVER_ENABLED) {
			ret = NULL;
		}

		// 如果是弃用的函数
		if (UNEXPECTED((fbc->common.fn_flags & ZEND_ACC_DEPRECATED) != 0)) {
			// 报错：函数或方法已弃用
			zend_deprecated_function(fbc);
			// 如果有异常
			if (UNEXPECTED(EG(exception) != NULL)) {
				// 如果 opline 结果类型是 变量或临时变量，把结果 zval 标记为未定义
				UNDEF_RESULT();
				// 如果 操作码的运算结果无效(是IS_UNUSED)
				if (!RETURN_VALUE_USED(opline)) {
					// 使用临时变量接收返回值
					ret = &retval;
					// 结果为 未定义
					ZVAL_UNDEF(ret);
				}
				// 跳转到指定标签 fcall_end
				ZEND_VM_C_GOTO(fcall_end);
			}
		}

		// 当前执行数据当成前一个
		call->prev_execute_data = execute_data;
		// 切换执行数据
		EG(current_execute_data) = call;

// 调试用
#if ZEND_DEBUG
		bool should_throw = zend_internal_call_should_throw(fbc, call);
#endif

		// 如果 操作码的运算结果有效(不是IS_UNUSED) ？？
		ret = RETURN_VALUE_USED(opline) ? EX_VAR(opline->result.var) : &retval;
		// 结果为null
		ZVAL_NULL(ret);
		// 开始观察者调用
		// "/ZEND_OBSERVER_FCALL_BEGIN\(\s*(.*)\s*\)/" => isset($extra_spec['OBSERVER']) ?            ($extra_spec['OBSERVER'] == 0 ? "" : "zend_observer_fcall_begin(\\1)")            : "",
		ZEND_OBSERVER_FCALL_BEGIN(call);
		// 一般情况下为null 
		if (!zend_execute_internal) {
			// 如果没有 zend_execute_internal 保存一个函数调用
			/* saves one function call if zend_execute_internal is not used */
			// 调用内置函数
			fbc->internal_function.handler(call, ret);
		// 否则
		} else {
			// 调用 zend_execute_internal（原型）
			zend_execute_internal(call, ret);
		}

// 调试用
#if ZEND_DEBUG
		if (!EG(exception) && call->func && !(call->func->common.fn_flags & ZEND_ACC_FAKE_CLOSURE)) {
			if (should_throw) {
				zend_internal_call_arginfo_violation(call->func);
			}
			ZEND_ASSERT(!(call->func->common.fn_flags & ZEND_ACC_HAS_RETURN_TYPE) ||
				zend_verify_internal_return_type(call->func, ret));
			ZEND_ASSERT((call->func->common.fn_flags & ZEND_ACC_RETURN_REFERENCE)
				? Z_ISREF_P(ret) : !Z_ISREF_P(ret));
			zend_verify_internal_func_info(call->func, ret);
		}
#endif
		// 如果用到了观察者，结束观察者调用
		// "/ZEND_OBSERVER_FCALL_END\(\s*([^,]*)\s*,\s*(.*)\s*\)/" => isset($extra_spec['OBSERVER']) ?            ($extra_spec['OBSERVER'] == 0 ? "" : "zend_observer_fcall_end(\\1, \\2)")            : "",
		ZEND_OBSERVER_FCALL_END(call, EG(exception) ? NULL : ret);

		// 当前执行数据作为前一个
		EG(current_execute_data) = execute_data;

// 创建跳转标签 fcall_end
ZEND_VM_C_LABEL(fcall_end):
		// 释放所有执行数据中的 函数参数
		zend_vm_stack_free_args(call);
		// 如果有额外命名参数
		if (UNEXPECTED(ZEND_CALL_INFO(call) & ZEND_CALL_HAS_EXTRA_NAMED_PARAMS)) {
			// 释放额外命名参数
			zend_free_extra_named_params(call->extra_named_params);
		}

		// 如果 操作码的运算结果无效(是IS_UNUSED)
		if (!RETURN_VALUE_USED(opline)) {
			// 销毁没有引用次数的对象，有引用次数的添加到gc回收周期里。并没有销毁 zval本身。
			i_zval_ptr_dtor(ret);
		}
	}

	// 如果需要释放$this
	if (UNEXPECTED(ZEND_CALL_INFO(call) & ZEND_CALL_RELEASE_THIS)) {
		// 释放$this
		OBJ_RELEASE(Z_OBJ(call->This));
	}
	// 释放调用框架（执行数据）。p1:执行数据
	zend_vm_stack_free_call_frame(call);
	// 如果有异常
	if (UNEXPECTED(EG(exception) != NULL)) {
		// 重新抛异常
		zend_rethrow_exception(execute_data);
		// 主要是 return
		HANDLE_EXCEPTION();
	}
	// 修改当前操作码并返回。EX(opline) = p1 
	ZEND_VM_SET_OPCODE(opline + 1);
	// windows: return  0
	ZEND_VM_CONTINUE();
}

// ing3, 验证函数返回值类型
ZEND_VM_COLD_CONST_HANDLER(124, ZEND_VERIFY_RETURN_TYPE, CONST|TMP|VAR|UNUSED|CV, UNUSED|CACHE_SLOT)
{
	// 如果 op1 类型是未定义
	if (OP1_TYPE == IS_UNUSED) {
		// windows: 无操作
		SAVE_OPLINE();
		// 报错：返回类型不符
		zend_verify_missing_return_type(EX(func));
		// 主要是 return
		HANDLE_EXCEPTION();
	// 否则, op1不是未定义
	} else {
/* prevents "undefined variable opline" errors */
// op1和op2 都是ANY
#if !ZEND_VM_SPEC || (OP1_TYPE != IS_UNUSED)
		// const zend_op *opline = EX(opline);
		USE_OPLINE
		zval *retval_ref, *retval_ptr;
		// 返回值信息
		zend_arg_info *ret_info = EX(func)->common.arg_info - 1;
		// 获取zval指针, UNUSED 返回null
		retval_ref = retval_ptr = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);

		// 如果op1是常量
		if (OP1_TYPE == IS_CONST) {
			// op1复制给 op1
			ZVAL_COPY(EX_VAR(opline->result.var), retval_ptr);
			// 在执行数据中取出 指定序号的变量
			retval_ref = retval_ptr = EX_VAR(opline->result.var);
		// 否则 如果 op1 是 普通变量
		} else if (OP1_TYPE == IS_VAR) {
			// op1是间接引用
			if (UNEXPECTED(Z_TYPE_P(retval_ptr) == IS_INDIRECT)) {
				// 解间接引用，追踪到引用目标
				retval_ref = retval_ptr = Z_INDIRECT_P(retval_ptr);
			}
			// op1 解引用
			ZVAL_DEREF(retval_ptr);
		// 否则 如果 op1 是 编译变量
		} else if (OP1_TYPE == IS_CV) {
			// op1 解引用
			ZVAL_DEREF(retval_ptr);
		}

		// 检查 p1的类型中 是否包含需要的类型 p2。如果有
		if (EXPECTED(ZEND_TYPE_CONTAINS_CODE(ret_info->type, Z_TYPE_P(retval_ptr)))) {
			// opline + 1，到目标操作码并 return
			ZEND_VM_NEXT_OPCODE();
		}

		// 如果 op1 是 编译变量 并且 追踪到未定义变量
		if (OP1_TYPE == IS_CV && UNEXPECTED(Z_ISUNDEF_P(retval_ptr))) {
			// windows: 无操作
			SAVE_OPLINE();
			// 报错：p1的变量（变量名）未定义, 并返未初始化zval
			retval_ref = retval_ptr = ZVAL_UNDEFINED_OP1();
			// 如果有异常
			if (UNEXPECTED(EG(exception))) {
				// 主要是 return
				HANDLE_EXCEPTION();
			}
			// 如果类型中 允许 null
			if (ZEND_TYPE_FULL_MASK(ret_info->type) & MAY_BE_NULL) {
				// opline + 1，到目标操作码并 return
				ZEND_VM_NEXT_OPCODE();
			}
		}

		// 临时引用实例
		zend_reference *ref = NULL;
		// op2中的序号从缓存位置中取出 指针
		void *cache_slot = CACHE_ADDR(opline->op2.num);
		// 这时候两个值不相等，说明 retval_ptr 被解引用过
		if (UNEXPECTED(retval_ref != retval_ptr)) {
			// 如果要求引用返回
			if (UNEXPECTED(EX(func)->op_array.fn_flags & ZEND_ACC_RETURN_REFERENCE)) {
				// 取出引用实例
				ref = Z_REF_P(retval_ref);
			// 否则。这里有点复杂
			} else {
				// 可能出现这种情况：如果这里返回了一个引用类型，需要解包
				/* A cast might happen - unwrap the reference if this is a by-value return */
				// 如果引用数为1
				if (Z_REFCOUNT_P(retval_ref) == 1) {
					// 解引用：追踪到引用目标，把目标中的变量复制过来，并销毁目标。
					ZVAL_UNREF(retval_ref);
					// 最后 使用 retval_ref
				// 否则
				} else {
					// 减少引用次数
					Z_DELREF_P(retval_ref);
					// 使用 retval_ptr
					ZVAL_COPY(retval_ref, retval_ptr);
				}
				// 最后使用 retval_ref
				retval_ptr = retval_ref;
			}
		}

		// windows: 无操作
		SAVE_OPLINE();
		// 检验参数类型是否匹配，p1:类型，p2:参数，p3:引用实例,参数是引用类型的时候才有，
		// p4:缓存位置，p5:是否是返回类型，p6:是否是内置函数
		if (UNEXPECTED(!zend_check_type_slow(&ret_info->type, retval_ptr, ref, cache_slot, 1, 0))) {
			// 报错：返回类型不符。p1:函数，p2:返回值
			zend_verify_return_error(EX(func), retval_ptr);
			// 主要是 return
			HANDLE_EXCEPTION();
		}
		// opline + 1，到目标操作码并 return
		ZEND_VM_NEXT_OPCODE();
#endif
	}
}

// ing3, 检验返回值类型，符合 :never
ZEND_VM_COLD_HANDLER(201, ZEND_VERIFY_NEVER_TYPE, UNUSED, UNUSED)
{
	// windows: 无操作
	SAVE_OPLINE();
	// 返回类型不符合 :never 时的报错
	zend_verify_never_error(EX(func));
	// 主要是 return
	HANDLE_EXCEPTION();
}

// ing2, return语句
ZEND_VM_INLINE_HANDLER(62, ZEND_RETURN, CONST|TMP|VAR|CV, ANY, SPEC(OBSERVER))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *retval_ptr;
	zval *return_value;
	// zend_vm_gen.php 处理观察者：zval observer_retval;
	// "/ZEND_OBSERVER_USE_RETVAL/" => isset($extra_spec['OBSERVER']) && $extra_spec['OBSERVER'] == 1 ? "zval observer_retval" : ""
	ZEND_OBSERVER_USE_RETVAL;

	// 获取zval指针, UNUSED 返回null
	retval_ptr = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 取出返回值
	return_value = EX(return_value);
	
	// zend_vm_gen.php 处理观察者：if (!return_value) { return_value = &observer_retval; }
	// "/ZEND_OBSERVER_SET_RETVAL\(\)/" => isset($extra_spec['OBSERVER']) && $extra_spec['OBSERVER'] == 1 ? "if (!return_value) { return_value = &observer_retval; }" : "",        
	ZEND_OBSERVER_SET_RETVAL();
	
	// 如果 op1 是 编译变量 并且 返回值类型为 未定义
	if (OP1_TYPE == IS_CV && UNEXPECTED(Z_TYPE_INFO_P(retval_ptr) == IS_UNDEF)) {
		// windows: 无操作
		SAVE_OPLINE();
		// 报错：p1的变量（变量名）未定义, 并返未初始化zval
		retval_ptr = ZVAL_UNDEFINED_OP1();
		// 如果有返回值
		if (return_value) {
			// 置null
			ZVAL_NULL(return_value);
		}
	// 如果没有返回值
	} else if (!return_value) {
		// 如果 op1 类型是变量 或 临时变量
		if (OP1_TYPE & (IS_VAR|IS_TMP_VAR)) {
			// 如果是 可计数类型 并且 引用计数-1 后为0
			if (Z_REFCOUNTED_P(retval_ptr) && !Z_DELREF_P(retval_ptr)) {
				// windows: 无操作
				SAVE_OPLINE();
				// 调用每个type对应的销毁函数执行销毁
				rc_dtor_func(Z_COUNTED_P(retval_ptr));
			}
		}
	// 否则
	} else {
		// 如果 op1 类型是变量 或 临时变量
		if ((OP1_TYPE & (IS_CONST|IS_TMP_VAR))) {
			// op1作为返回值
			ZVAL_COPY_VALUE(return_value, retval_ptr);
			// 如果op1是常量
			if (OP1_TYPE == IS_CONST) {
				// 如果是可以计数类型
				if (UNEXPECTED(Z_OPT_REFCOUNTED_P(return_value))) {
					// 引用次数+1
					Z_ADDREF_P(return_value);
				}
			}
		// 否则 如果 op1 是 编译变量
		} else if (OP1_TYPE == IS_CV) {
			// 为了break
			do {
				// 如果是可以计数类型
				if (Z_OPT_REFCOUNTED_P(retval_ptr)) {
					// 通过指针验证 zval的type 是否是 IS_REFERENCE
					// 如果不是引用类型
					if (EXPECTED(!Z_OPT_ISREF_P(retval_ptr))) {
						// 如果没有 ZEND_CALL_CODE 和 ZEND_CALL_OBSERVED
						if (EXPECTED(!(EX_CALL_INFO() & (ZEND_CALL_CODE|ZEND_CALL_OBSERVED)))) {
							// 取出计数器
							zend_refcounted *ref = Z_COUNTED_P(retval_ptr);
							// op1 复制给 结果
							ZVAL_COPY_VALUE(return_value, retval_ptr);
							// 如果 ref 可能泄露
							if (GC_MAY_LEAK(ref)) {
								// windows: 无操作
								SAVE_OPLINE();
								// ref 放入回收队列
								gc_possible_root(ref);
							}
							// 清空指针
							ZVAL_NULL(retval_ptr);
							// 跳出
							break;
						// 否则
						} else {
							// 引用次数+1
							Z_ADDREF_P(retval_ptr);
						}
					// 否则
					} else {
						// 解引用
						retval_ptr = Z_REFVAL_P(retval_ptr);
						// 如果是可以计数类型
						if (Z_OPT_REFCOUNTED_P(retval_ptr)) {
							// 引用次数+1
							Z_ADDREF_P(retval_ptr);
						}
					}
				}
				// op1 复制给 结果
				ZVAL_COPY_VALUE(return_value, retval_ptr);
			} while (0);
		// 否则 如果 op1 是 普通变量
		} else /* if (OP1_TYPE == IS_VAR) */ {
			// 如果 op1是引用类型
			if (UNEXPECTED(Z_ISREF_P(retval_ptr))) {
				// 取出计数器
				zend_refcounted *ref = Z_COUNTED_P(retval_ptr);
				// 解引用
				retval_ptr = Z_REFVAL_P(retval_ptr);
				// op1 复制给 结果
				ZVAL_COPY_VALUE(return_value, retval_ptr);
				// 引用次数-1，如果为0
				if (UNEXPECTED(GC_DELREF(ref) == 0)) {
					// 释放引用实例
					efree_size(ref, sizeof(zend_reference));
				// 否则 ，如果是可以计数类型
				} else if (Z_OPT_REFCOUNTED_P(retval_ptr)) {
					// 引用次数+1
					Z_ADDREF_P(retval_ptr);
				}
			// 否则
			} else {
				// op1 复制给 结果
				ZVAL_COPY_VALUE(return_value, retval_ptr);
			}
		}
	}
	// zend_vm_gen.php 处理
	// "/ZEND_OBSERVER_SAVE_OPLINE\(\)/" => isset($extra_spec['OBSERVER']) && $extra_spec['OBSERVER'] == 1 ? "SAVE_OPLINE()" : "",
	ZEND_OBSERVER_SAVE_OPLINE();
	
	// 如果用到了观察者，结束观察者调用
	// "/ZEND_OBSERVER_FCALL_END\(\s*([^,]*)\s*,\s*(.*)\s*\)/" => isset($extra_spec['OBSERVER']) ?            ($extra_spec['OBSERVER'] == 0 ? "" : "zend_observer_fcall_end(\\1, \\2)")            : "",
	ZEND_OBSERVER_FCALL_END(execute_data, return_value);
	
	// 处理观察者：if (return_value == &observer_retval) { zval_ptr_dtor_nogc(&observer_retval); }
	// "/ZEND_OBSERVER_FREE_RETVAL\(\)/" => isset($extra_spec['OBSERVER']) && $extra_spec['OBSERVER'] == 1 ? "if (return_value == &observer_retval) { zval_ptr_dtor_nogc(&observer_retval); }" : "",
    ZEND_OBSERVER_FREE_RETVAL();
	// 
	// 调用方法 zend_leave_helper
	ZEND_VM_DISPATCH_TO_HELPER(zend_leave_helper);
}

// ing2, 引用返回
ZEND_VM_COLD_CONST_HANDLER(111, ZEND_RETURN_BY_REF, CONST|TMP|VAR|CV, ANY, SRC, SPEC(OBSERVER))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *retval_ptr;
	zval *return_value;
	// "/ZEND_OBSERVER_USE_RETVAL/" => isset($extra_spec['OBSERVER']) && $extra_spec['OBSERVER'] == 1 ? "zval observer_retval" : ""
	ZEND_OBSERVER_USE_RETVAL;

	// windows: 无操作
	SAVE_OPLINE();

	// 上下文中的返回值
	return_value = EX(return_value);
	// "/ZEND_OBSERVER_SET_RETVAL\(\)/" => isset($extra_spec['OBSERVER']) && $extra_spec['OBSERVER'] == 1 ? "if (!return_value) { return_value = &observer_retval; }" : "",
	ZEND_OBSERVER_SET_RETVAL();
	// 为了break
	do {
		// 如果 （op1 是常量 或 临时变量） 或 （op1是变量 并且 ZEND_RETURNS_VALUE）
		if ((OP1_TYPE & (IS_CONST|IS_TMP_VAR)) ||
		    (OP1_TYPE == IS_VAR && opline->extended_value == ZEND_RETURNS_VALUE)) {
			// 不是引用类型，但允许操作，只报个提示
			/* Not supposed to happen, but we'll allow it */
			// 提示： 只在变量的引用 可以被 引用传递
			zend_error(E_NOTICE, "Only variable references should be returned by reference");

			// 获取zval指针，UNUSED 类型返回null
			retval_ptr = GET_OP1_ZVAL_PTR(BP_VAR_R);
			// 如果没有返回值
			if (!return_value) {
				// 释放操作对象的附加变量
				FREE_OP1();
			// 否则，有返回值
			} else {
				// 如果op1是变量 并且 op1是引用类型
				if (OP1_TYPE == IS_VAR && UNEXPECTED(Z_ISREF_P(retval_ptr))) {
					// op1 复制给返回值
					ZVAL_COPY_VALUE(return_value, retval_ptr);
					// 跳出
					break;
				}
				// 给 zval 创建新的 zend_reference ，从给出的 zval 中复制附加内容
				ZVAL_NEW_REF(return_value, retval_ptr);
				// 如果op1是常量
				if (OP1_TYPE == IS_CONST) {
					// 增加引用次数
					Z_TRY_ADDREF_P(retval_ptr);
				}
			}
			// 跳出
			break;
		}
		
		// 获取zval指针,TMP/CONST/UNUSED 返回null，不支持 TMPVAR/TMPVARCV
		retval_ptr = GET_OP1_ZVAL_PTR_PTR(BP_VAR_W);

		// 如果 op1 是 普通变量
		if (OP1_TYPE == IS_VAR) {
			// op1必须有效
			ZEND_ASSERT(retval_ptr != &EG(uninitialized_zval));
			// 如果 ZEND_RETURNS_FUNCTION 并且 结果不是引用类型
			if (opline->extended_value == ZEND_RETURNS_FUNCTION && !Z_ISREF_P(retval_ptr)) {
				// 提示：只有变量的引用可以被引用返回
				zend_error(E_NOTICE, "Only variable references should be returned by reference");
				// 如果有返回值
				if (return_value) {
					// 给 zval 创建新的 zend_reference ，从给出的 zval 中复制附加内容
					ZVAL_NEW_REF(return_value, retval_ptr);
				// 否则
				} else {
					// 释放操作对象的附加变量
					FREE_OP1();
				}
				// 跳出
				break;
			}
		}

		// 如果有返回值
		if (return_value) {
			// 如果是引用类型
			if (Z_ISREF_P(retval_ptr)) {
				// 引用次数+1
				Z_ADDREF_P(retval_ptr);
			// 否则，不是引用类型
			} else {
				// 把zval转为引用，p1:zval，p2:引用次数
				ZVAL_MAKE_REF_EX(retval_ptr, 2);
			}
			// 给返回值引用赋值
			ZVAL_REF(return_value, Z_REF_P(retval_ptr));
		}

		// 释放操作对象的附加变量
		FREE_OP1();
	} while (0);

	// 如果用到了观察者，结束观察者调用
	// "/ZEND_OBSERVER_FCALL_END\(\s*([^,]*)\s*,\s*(.*)\s*\)/" => isset($extra_spec['OBSERVER']) ?            ($extra_spec['OBSERVER'] == 0 ? "" : "zend_observer_fcall_end(\\1, \\2)")            : "",
	ZEND_OBSERVER_FCALL_END(execute_data, return_value);
	
	// "/ZEND_OBSERVER_FREE_RETVAL\(\)/" => isset($extra_spec['OBSERVER']) && $extra_spec['OBSERVER'] == 1 ? "if (return_value == &observer_retval) { zval_ptr_dtor_nogc(&observer_retval); }" : "",
    ZEND_OBSERVER_FREE_RETVAL();
	
	// 调用方法 zend_leave_helper
	ZEND_VM_DISPATCH_TO_HELPER(zend_leave_helper);
}

// ing2, 生成器 create 方法
ZEND_VM_HANDLER(139, ZEND_GENERATOR_CREATE, ANY, ANY)
{
	// 返回值
	zval *return_value = EX(return_value);

	// 如果有返回值
	if (EXPECTED(return_value)) {
		// const zend_op *opline = EX(opline);
		USE_OPLINE
		zend_generator *generator;
		zend_execute_data *gen_execute_data;
		uint32_t num_args, used_stack, call_info;

		// windows: 无操作
		SAVE_OPLINE();
		// 初始化对象，传入zval指针和类入口
		object_init_ex(return_value, zend_ce_generator);

		/*
		 * Normally the execute_data is allocated on the VM stack (because it does
		 * not actually do any allocation and thus is faster). For generators
		 * though this behavior would be suboptimal, because the (rather large)
		 * structure would have to be copied back and forth every time execution is
		 * suspended or resumed. That's why for generators the execution context
		 * is allocated on heap.
		 */
		num_args = EX_NUM_ARGS();
		if (EXPECTED(num_args <= EX(func)->op_array.num_args)) {
			used_stack = (ZEND_CALL_FRAME_SLOT + EX(func)->op_array.last_var + EX(func)->op_array.T) * sizeof(zval);
			gen_execute_data = (zend_execute_data*)emalloc(used_stack);
			used_stack = (ZEND_CALL_FRAME_SLOT + EX(func)->op_array.last_var) * sizeof(zval);
		// 否则
		} else {
			used_stack = (ZEND_CALL_FRAME_SLOT + num_args + EX(func)->op_array.last_var + EX(func)->op_array.T - EX(func)->op_array.num_args) * sizeof(zval);
			gen_execute_data = (zend_execute_data*)emalloc(used_stack);
		}
		memcpy(gen_execute_data, execute_data, used_stack);

		// 把执行上下文保存到生成器的对象中
		/* Save execution context in generator object. */
		// 返回值转成生成器
		generator = (zend_generator *) Z_OBJ_P(EX(return_value));
		// 关联上下文
		generator->execute_data = gen_execute_data;
		// 冻结调用堆栈为空
		generator->frozen_call_stack = NULL;
		// 操作码为空
		generator->execute_fake.opline = NULL;
		// 函数为空
		generator->execute_fake.func = NULL;
		// 前一个执行数据为空
		generator->execute_fake.prev_execute_data = NULL;
		// 把生成器关联到它自己的 $this
		ZVAL_OBJ(&generator->execute_fake.This, (zend_object *) generator);

		// 切换到下一个操作码
		gen_execute_data->opline = opline + 1;
		
		// EX(return_value) 保存 zend_object 的指针，并不是真的 zval
		/* EX(return_value) keeps pointer to zend_object (not a real zval) */
		
		// 更新 生成器上下文的 返回值为 生成器本身
		gen_execute_data->return_value = (zval*)generator;
		// 调用信息
		call_info = Z_TYPE_INFO(EX(This));
		// 如果调用信息 是 对象格式 并且 （（不是调用闭包也 不需要释放$this） 或 zend_execute_ex != execute_ex）
		if ((call_info & Z_TYPE_MASK) == IS_OBJECT
		 && (!(call_info & (ZEND_CALL_CLOSURE|ZEND_CALL_RELEASE_THIS))
			 /* Bug #72523 */
			|| UNEXPECTED(zend_execute_ex != execute_ex))) {
			// p1 |= p2	
			ZEND_ADD_CALL_FLAG_EX(call_info, ZEND_CALL_RELEASE_THIS);
			// $this增加引用次数
			Z_ADDREF(gen_execute_data->This);
		}
		// p1 |= p2
		ZEND_ADD_CALL_FLAG_EX(call_info, (ZEND_CALL_TOP_FUNCTION | ZEND_CALL_ALLOCATED | ZEND_CALL_GENERATOR));
		// 更新生成器执行数据的 调用信息
		Z_TYPE_INFO(gen_execute_data->This) = call_info;
		// 生成器执行数据的前一个执行数据为 null
		gen_execute_data->prev_execute_data = NULL;

		// 取出调用信息
		call_info = EX_CALL_INFO();
		// 切换到前一个执行数据
		EG(current_execute_data) = EX(prev_execute_data);
		// 如果没有【顶层调用】和【调用分配】标记
		if (EXPECTED(!(call_info & (ZEND_CALL_TOP|ZEND_CALL_ALLOCATED)))) {
			// 执行数据放到虚拟机堆栈的最上面
			EG(vm_stack_top) = (zval*)execute_data;
			// 切换到前一个执行数据
			execute_data = EX(prev_execute_data);
			// windows: ZEND_VM_INC_OPCODE() : // OPLINE++
			LOAD_NEXT_OPLINE();
			// windows return 2;
			ZEND_VM_LEAVE();
		// 如果不是【顶层调用】
		} else if (EXPECTED(!(call_info & ZEND_CALL_TOP))) {
			// 原执行数据
			zend_execute_data *old_execute_data = execute_data;
			// 切换到前一个执行数据
			execute_data = EX(prev_execute_data);
			// 释放调用框架（执行数据），p1:调用信息，p2:执行数据
			zend_vm_stack_free_call_frame_ex(call_info, old_execute_data);
			// windows: ZEND_VM_INC_OPCODE() : // OPLINE++
			LOAD_NEXT_OPLINE();
			// windows return 2;
			ZEND_VM_LEAVE();
		// 否则
		} else {
			// windows: return -1
			ZEND_VM_RETURN();
		}
	// 否则
	} else {
		// 调用方法 zend_leave_helper
		ZEND_VM_DISPATCH_TO_HELPER(zend_leave_helper);
	}
}

// ing3, 生成器的返回值
ZEND_VM_HANDLER(161, ZEND_GENERATOR_RETURN, CONST|TMP|VAR|CV, ANY, SPEC(OBSERVER))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *retval;

	// 获取正在运行的生成器
	zend_generator *generator = zend_get_running_generator(EXECUTE_DATA_C);

	// windows: 无操作
	SAVE_OPLINE();
	// 获取zval指针，UNUSED 类型返回null
	retval = GET_OP1_ZVAL_PTR(BP_VAR_R);

	// 把返回值 op1 复制到 生成器的返回值中
	/* Copy return value into generator->retval */
	// 如果op1是 常量 或 变量
	if ((OP1_TYPE & (IS_CONST|IS_TMP_VAR))) {
		// 把返回值 op1 复制到 生成器的返回值中
		ZVAL_COPY_VALUE(&generator->retval, retval);
		// 如果op1是常量
		if (OP1_TYPE == IS_CONST) {
			// 如果生成器的返回值是可计数类型
			if (UNEXPECTED(Z_OPT_REFCOUNTED(generator->retval))) {
				// 增加引用次数
				Z_ADDREF(generator->retval);
			}
		}
	// 否则 如果 op1 是 编译变量
	} else if (OP1_TYPE == IS_CV) {
		// 复制引用目标 和 附加信息，并增加引用计数
		ZVAL_COPY_DEREF(&generator->retval, retval);
	// 否则 如果 op1 是 普通变量
	} else /* if (OP1_TYPE == IS_VAR) */ {
		// 如果返回值是引用类型
		if (UNEXPECTED(Z_ISREF_P(retval))) {
			// 取出引用实例
			zend_refcounted *ref = Z_COUNTED_P(retval);
			// 解引用
			retval = Z_REFVAL_P(retval);
			// 把返回值 op1 复制到 生成器的返回值中
			ZVAL_COPY_VALUE(&generator->retval, retval);
			// 引用次数-1，如果为0
			if (UNEXPECTED(GC_DELREF(ref) == 0)) {
				// 释放引用实例
				efree_size(ref, sizeof(zend_reference));
			// 否则 ，如果是可以计数类型
			} else if (Z_OPT_REFCOUNTED_P(retval)) {
				// 引用次数+1
				Z_ADDREF_P(retval);
			}
		// 否则
		} else {
			// 把返回值 op1 复制到 生成器的返回值中
			ZVAL_COPY_VALUE(&generator->retval, retval);
		}
	}

	// 如果用到了观察者，结束观察者调用
	// "/ZEND_OBSERVER_FCALL_END\(\s*([^,]*)\s*,\s*(.*)\s*\)/" => isset($extra_spec['OBSERVER']) ?            ($extra_spec['OBSERVER'] == 0 ? "" : "zend_observer_fcall_end(\\1, \\2)")            : "",
	ZEND_OBSERVER_FCALL_END(generator->execute_data, &generator->retval);

	// 关闭生成器 释放上面的资源
	/* Close the generator to free up resources */
	zend_generator_close(generator, 1);

	// 把执行数据传回 处理逻辑
	/* Pass execution back to handling code */
	// windows: return -1
	ZEND_VM_RETURN();
}

// ing3, 抛出异常，throw语句
ZEND_VM_COLD_CONST_HANDLER(108, ZEND_THROW, CONST|TMPVAR|CV, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *value;

	// windows: 无操作
	SAVE_OPLINE();
	// 获取zval指针, UNUSED 返回null
	value = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);

	do {
		// 如果op1是常量 或 值不是对象
		if (OP1_TYPE == IS_CONST || UNEXPECTED(Z_TYPE_P(value) != IS_OBJECT)) {
			// 如果 op1 是普通变量或编译变量 并且 op1是引用类型
			if ((OP1_TYPE & (IS_VAR|IS_CV)) && Z_ISREF_P(value)) {
				// 解引用
				value = Z_REFVAL_P(value);
				// 如果值是对象
				if (EXPECTED(Z_TYPE_P(value) == IS_OBJECT)) {
					// 跳出
					break;
				}
			}
			// 如果 op1 是 编译变量 并且 值 类型为 未定义
			if (OP1_TYPE == IS_CV && UNEXPECTED(Z_TYPE_P(value) == IS_UNDEF)) {
				// 报错：p1的变量（变量名）未定义, 并返未初始化zval
				ZVAL_UNDEFINED_OP1();
				// 如果有异常
				if (UNEXPECTED(EG(exception) != NULL)) {
					// 主要是 return
					HANDLE_EXCEPTION();
				}
			}
			// 抛异常：只能抛出 对象
			zend_throw_error(NULL, "Can only throw objects");
			// 释放操作对象的附加变量
			FREE_OP1();
			// 主要是 return
			HANDLE_EXCEPTION();
		}
	} while (0);

	// 把异常保存到 previous 链里 ,清空当前异常
	zend_exception_save();
	// 增加引用次数
	Z_TRY_ADDREF_P(value);
	// 抛出异常实例
	zend_throw_exception_object(value);
	// 如果有当前异常，把上一个异常存储起来，如果没有，把上一个作为当前异常
	zend_exception_restore();
	// 释放操作对象的附加变量
	FREE_OP1();
	// 主要是 return
	HANDLE_EXCEPTION();
}

// ing3, catch语句
ZEND_VM_HANDLER(107, ZEND_CATCH, CONST, JMP_ADDR, LAST_CATCH|CACHE_SLOT)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zend_class_entry *ce, *catch_ce;
	zend_object *exception;

	// windows: 无操作
	SAVE_OPLINE();
	/* Check whether an exception has been thrown, if not, jump over code */
	// 如果有当前异常，把上一个异常存储起来，如果没有，把上一个作为当前异常
	zend_exception_restore();
	// 如果有异常
	if (EG(exception) == NULL) {
		// 要检查异常 并 有异常时使用旧操作码，无异常使用新操作码, p1:新操作码，p2:是否检查异常
		// 访问 p2.jmp_addr
		ZEND_VM_JMP_EX(OP_JMP_ADDR(opline, opline->op2), 0);
	}
	
	// 这个catch想要捕获的类型
	// 通过偏移量 在运行时缓存中获取一个 (void**) 中的第一个元素
	catch_ce = CACHED_PTR(opline->extended_value & ~ZEND_LAST_CATCH);
	// 如果没有指针cache类型
	if (UNEXPECTED(catch_ce == NULL)) {
		// 获取类，找不到会报错。p1:类名，p2:key，p3:flags
		// 访问 (p2).zv。p1:opline,p2:node
		catch_ce = zend_fetch_class_by_name(Z_STR_P(RT_CONSTANT(opline, opline->op1)), Z_STR_P(RT_CONSTANT(opline, opline->op1) + 1), ZEND_FETCH_CLASS_NO_AUTOLOAD | ZEND_FETCH_CLASS_SILENT);

		// 通过偏移量 在 EX(run_time_cache) 中获取一个 (void**) 中的第一个元素，并给它赋值
		CACHE_PTR(opline->extended_value & ~ZEND_LAST_CATCH, catch_ce);
	}
	// 取出cache类型
	ce = EG(exception)->ce;

// 
#ifdef HAVE_DTRACE
	if (DTRACE_EXCEPTION_CAUGHT_ENABLED()) {
		DTRACE_EXCEPTION_CAUGHT((char *)ce->name);
	}
#endif /* HAVE_DTRACE */

	// 如果已有异常和 cache到的类型不同
	if (ce != catch_ce) {
		// 如果没有cache类型 或 异常类型不 属于 cache类型
		if (!catch_ce || !instanceof_function(ce, catch_ce)) {
			// 如果有 最后一个catch标记
			if (opline->extended_value & ZEND_LAST_CATCH) {
				// 重新抛出异常
				zend_rethrow_exception(execute_data);
				// 主要是 return
				HANDLE_EXCEPTION();
			}
			// 要检查异常 并 有异常时使用旧操作码，无异常使用新操作码, p1:新操作码，p2:是否检查异常
			// OP_JMP_ADDR: 访问 p2.jmp_addr
			ZEND_VM_JMP_EX(OP_JMP_ADDR(opline, opline->op2), 0);
		}
	}

	// 取出现有异常
	exception = EG(exception);
	// 清空现有异常
	EG(exception) = NULL;
	// 如果 操作码的运算结果有效(不是IS_UNUSED)
	if (RETURN_VALUE_USED(opline)) {
		// 总是进行一个严格的赋值。
		/* Always perform a strict assignment. There is a reasonable expectation that if you
		 * write "catch (Exception $e)" then $e will actually be instanceof Exception. As such,
		 * we should not permit coercion to string here. */
		zval tmp;
		// 异常存到临时变量中
		ZVAL_OBJ(&tmp, exception);
		// 给变量赋值，p1:变量，p2:值，p3:值的变量类型（常量，变量，编译变量），p4:是否严格类型
		// 某执行上下文所属函数 的参数是否使用严格类型限制
		zend_assign_to_variable(EX_VAR(opline->result.var), &tmp, IS_TMP_VAR, /* strict */ 1);
		
	// 否则，操作码的运行结果无效
	} else {
		// 释放异常
		OBJ_RELEASE(exception);
	}
	// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, op1复制给 result.如果有op2，说明是命名参数
ZEND_VM_HOT_HANDLER(65, ZEND_SEND_VAL, CONST|TMPVAR, CONST|UNUSED|NUM)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *value, *arg;

	// 如果类型为 常量
	if (OP2_TYPE == IS_CONST) {
		// windows: 无操作
		SAVE_OPLINE();
		// 取出操作码中，op2的常量
		zend_string *arg_name = Z_STR_P(RT_CONSTANT(opline, opline->op2));
		uint32_t arg_num;
		// 处理命名参数，p1:执行数据指针的指针，p2:参数名，p3:参数数量 ，p4:缓存位置
		arg = zend_handle_named_arg(&EX(call), arg_name, &arg_num, CACHE_ADDR(opline->result.num));
		// 如果获取失败
		if (UNEXPECTED(!arg)) {
			// 释放操作对象的附加变量
			FREE_OP1();
			// 主要是 return
			HANDLE_EXCEPTION();
		}
	// 否则，op2类型不是常量
	} else {
		// 把p1 右移p2字节，转成zval返回
		arg = ZEND_CALL_VAR(EX(call), opline->result.var);
	}

	// 获取zval指针，UNUSED 类型返回null
	value = GET_OP1_ZVAL_PTR(BP_VAR_R);
	// op1复制给 result
	ZVAL_COPY_VALUE(arg, value);
	// 如果op1是常量
	if (OP1_TYPE == IS_CONST) {
		// 如果是可以计数类型
		if (UNEXPECTED(Z_OPT_REFCOUNTED_P(arg))) {
			// 引用次数+1
			Z_ADDREF_P(arg);
		}
	}
	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, 抛错：此参数不可以使用引用传递
ZEND_VM_COLD_HELPER(zend_cannot_pass_by_ref_helper, ANY, ANY, uint32_t _arg_num, zval *_arg)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	// windows: 无操作
	SAVE_OPLINE();
	// 抛错：此参数不可以使用引用传递
	zend_cannot_pass_by_reference(_arg_num);
	// 释放操作对象的附加变量
	FREE_OP1();
	// 参数 为 未定义
	ZVAL_UNDEF(_arg);
	// 主要是 return
	HANDLE_EXCEPTION();
}

// ing2, 检验并使用函数参数
ZEND_VM_HOT_SEND_HANDLER(116, ZEND_SEND_VAL_EX, CONST|TMP, CONST|UNUSED|NUM, SPEC(QUICK_ARG))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *value, *arg;
	uint32_t arg_num;

	// 如果 op2 类型为 常量
	if (OP2_TYPE == IS_CONST) {
		// windows: 无操作
		SAVE_OPLINE();
		// 访问 (p2).zv。p1:opline,p2:node
		zend_string *arg_name = Z_STR_P(RT_CONSTANT(opline, opline->op2));
		// 处理命名参数，p1:执行数据指针的指针，p2:参数名，p3:参数数量 ，p4:缓存位置
		arg = zend_handle_named_arg(&EX(call), arg_name, &arg_num, CACHE_ADDR(opline->result.num));
		// 如果没有获取到参数
		if (UNEXPECTED(!arg)) {
			// 释放操作对象的附加变量
			FREE_OP1();
			// 主要是 return
			HANDLE_EXCEPTION();
		}
	// 否则, op2 不是常量
	} else {
		// 把p1 右移p2字节，转成zval返回
		arg = ZEND_CALL_VAR(EX(call), opline->result.var);
		// 从op2中取出参数序号
		arg_num = opline->op2.num;
	}

	// 如果参数数量没有超限
	if (EXPECTED(arg_num <= MAX_ARG_FLAG_NUM)) {
		// 如果参数 要求只能通过引用传递
		if (QUICK_ARG_MUST_BE_SENT_BY_REF(EX(call)->func, arg_num)) {
			// 跳转到指定标签 send_val_by_ref
			ZEND_VM_C_GOTO(send_val_by_ref);
		}
	// 如果数量超限，并且要求引用传递	
	} else if (ARG_MUST_BE_SENT_BY_REF(EX(call)->func, arg_num)) {
// 创建跳转标签 send_val_by_ref
ZEND_VM_C_LABEL(send_val_by_ref):
		// 抛错：此参数不可以使用引用传递
		// 调用方法 zend_cannot_pass_by_ref_helper
		ZEND_VM_DISPATCH_TO_HELPER(zend_cannot_pass_by_ref_helper, _arg_num, arg_num, _arg, arg);
	}
	// 获取zval指针，UNUSED 类型返回null
	value = GET_OP1_ZVAL_PTR(BP_VAR_R);
	// 使用 value的 副本
	ZVAL_COPY_VALUE(arg, value);
	// 如果op1是常量
	if (OP1_TYPE == IS_CONST) {
		// 如果是可以计数类型
		if (UNEXPECTED(Z_OPT_REFCOUNTED_P(arg))) {
			// 引用次数+1
			Z_ADDREF_P(arg);
		}
	}
	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, op1复制给result ,有校验op1，和解引用。op2有值说明是命名参数
ZEND_VM_HOT_HANDLER(117, ZEND_SEND_VAR, VAR|CV, CONST|UNUSED|NUM)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *varptr, *arg;

	// 如果类型为 常量
	if (OP2_TYPE == IS_CONST) {
		// windows: 无操作
		SAVE_OPLINE();
		// 访问 (p2).zv。p1:opline,p2:node
		zend_string *arg_name = Z_STR_P(RT_CONSTANT(opline, opline->op2));
		uint32_t arg_num;
		// 处理命名参数，p1:执行数据指针的指针，p2:参数名，p3:参数数量 ，p4:缓存位置
		arg = zend_handle_named_arg(&EX(call), arg_name, &arg_num, CACHE_ADDR(opline->result.num));
		// 如果获取结果失败
		if (UNEXPECTED(!arg)) {
			// 释放操作对象的附加变量
			FREE_OP1();
			// 主要是 return
			HANDLE_EXCEPTION();
		}
	// 否则
	} else {
		// 把p1 右移p2字节，转成zval返回
		arg = ZEND_CALL_VAR(EX(call), opline->result.var);
	}

	// 获取zval指针, UNUSED 返回null
	varptr = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 如果 op1 是 编译变量 并且变量指针 类型为 未定义
	if (OP1_TYPE == IS_CV && UNEXPECTED(Z_TYPE_INFO_P(varptr) == IS_UNDEF)) {
		// windows: 无操作
		SAVE_OPLINE();
		// 报错：p1的变量（变量名）未定义, 并返未初始化zval
		ZVAL_UNDEFINED_OP1();
		// 结果为null
		ZVAL_NULL(arg);
		// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
		ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
	}

	// 如果 op1 是 编译变量
	if (OP1_TYPE == IS_CV) {
		// 复制引用目标 和 附加信息，并增加引用计数
		ZVAL_COPY_DEREF(arg, varptr);
	// 否则 如果 op1 是 普通变量
	} else /* if (OP1_TYPE == IS_VAR) */ {
		if (UNEXPECTED(Z_ISREF_P(varptr))) {
			// 取得 引用实例
			zend_refcounted *ref = Z_COUNTED_P(varptr);
			// 解引用
			varptr = Z_REFVAL_P(varptr);
			// 复制结果 
			ZVAL_COPY_VALUE(arg, varptr);
			// 引用次数-1，如果为0
			if (UNEXPECTED(GC_DELREF(ref) == 0)) {
				// 释放引用实例
				efree_size(ref, sizeof(zend_reference));
			// 否则， 如果是可以计数类型
			} else if (Z_OPT_REFCOUNTED_P(arg)) {
				// 引用次数+1
				Z_ADDREF_P(arg);
			}
		// 否则
		} else {
			// 复制并 增加引用次数
			ZVAL_COPY_VALUE(arg, varptr);
		}
	}

	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, op1复制给result，不支持引用类型
ZEND_VM_HANDLER(106, ZEND_SEND_VAR_NO_REF, VAR, CONST|UNUSED|NUM)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *varptr, *arg;

	// 如果类型为 常量
	if (OP2_TYPE == IS_CONST) {
		// windows: 无操作
		SAVE_OPLINE();
		// 访问 (p2).zv。p1:opline,p2:node
		zend_string *arg_name = Z_STR_P(RT_CONSTANT(opline, opline->op2));
		uint32_t arg_num;
		// 处理命名参数，p1:执行数据指针的指针，p2:参数名，p3:参数数量 ，p4:缓存位置
		arg = zend_handle_named_arg(&EX(call), arg_name, &arg_num, CACHE_ADDR(opline->result.num));
		// 如果获取失败
		if (UNEXPECTED(!arg)) {
			// 释放操作对象的附加变量
			FREE_OP1();
			// 主要是 return
			HANDLE_EXCEPTION();
		}
	// 否则
	} else {
		// 把p1 右移p2字节，转成zval返回
		arg = ZEND_CALL_VAR(EX(call), opline->result.var);
	}

	// 获取zval指针，UNUSED 类型返回null
	varptr = GET_OP1_ZVAL_PTR(BP_VAR_R);
	// op1复制给result
	ZVAL_COPY_VALUE(arg, varptr);

	// 如果op1是引用类型
	if (EXPECTED(Z_ISREF_P(varptr))) {
		// opline + 1，到目标操作码并 return
		ZEND_VM_NEXT_OPCODE();
	}

	// windows: 无操作
	SAVE_OPLINE();
	// 给 zval 创建新的 zend_reference ，从给出的 zval 中复制附加内容
	ZVAL_NEW_REF(arg, arg);
	// 报错：只有变量可以通过引用传递
	zend_error(E_NOTICE, "Only variables should be passed by reference");
	// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, op1复制给result,传递非引用类型的变量
ZEND_VM_HOT_SEND_HANDLER(50, ZEND_SEND_VAR_NO_REF_EX, VAR, CONST|UNUSED|NUM, SPEC(QUICK_ARG))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *varptr, *arg;
	uint32_t arg_num;

	// 如果类型为 常量
	if (OP2_TYPE == IS_CONST) {
		// windows: 无操作
		SAVE_OPLINE();
		// 访问 (p2).zv。p1:opline,p2:node
		zend_string *arg_name = Z_STR_P(RT_CONSTANT(opline, opline->op2));
		// 处理命名参数，p1:执行数据指针的指针，p2:参数名，p3:参数数量 ，p4:缓存位置
		arg = zend_handle_named_arg(&EX(call), arg_name, &arg_num, CACHE_ADDR(opline->result.num));
		// 如果获取失败
		if (UNEXPECTED(!arg)) {
			// 释放操作对象的附加变量
			FREE_OP1();
			// 主要是 return
			HANDLE_EXCEPTION();
		}
	// 否则
	} else {
		// 把p1 右移p2字节，转成zval返回
		arg = ZEND_CALL_VAR(EX(call), opline->result.var);
		// 参数序号
		arg_num = opline->op2.num;
	}

	// 如果参数数量 没有超限制
	if (EXPECTED(arg_num <= MAX_ARG_FLAG_NUM)) {
		// 如果不可以使用引用传递
		if (!QUICK_ARG_SHOULD_BE_SENT_BY_REF(EX(call)->func, arg_num)) {
			// 跳转到指定标签 send_val_by_ref
			ZEND_VM_C_GOTO(send_var);
		}

		// 获取zval指针，UNUSED 类型返回null
		varptr = GET_OP1_ZVAL_PTR(BP_VAR_R);
		// op1复制给result
		ZVAL_COPY_VALUE(arg, varptr);

		// 如果op1是引用类型 或 参数可以通过引用传递
		if (EXPECTED(Z_ISREF_P(varptr) ||
		    QUICK_ARG_MAY_BE_SENT_BY_REF(EX(call)->func, arg_num))) {
			// opline + 1，到目标操作码并 return
			ZEND_VM_NEXT_OPCODE();
		}
	// 否则，参数数量超限制
	} else {
		// 如果参数不必须使用引用传递
		if (!ARG_SHOULD_BE_SENT_BY_REF(EX(call)->func, arg_num)) {
			// 跳转到指定标签 send_var
			ZEND_VM_C_GOTO(send_var);
		}

		// 获取zval指针，UNUSED 类型返回null
		varptr = GET_OP1_ZVAL_PTR(BP_VAR_R);
		// op1复制给result
		ZVAL_COPY_VALUE(arg, varptr);

		// 如果op1是引用类型 或 参数可以使用引用传递
		if (EXPECTED(Z_ISREF_P(varptr) ||
			ARG_MAY_BE_SENT_BY_REF(EX(call)->func, arg_num))) {
			// opline + 1，到目标操作码并 return
			ZEND_VM_NEXT_OPCODE();
		}
	}

	// windows: 无操作
	SAVE_OPLINE();
	// 给 zval 创建新的 zend_reference ，从给出的 zval 中复制附加内容
	ZVAL_NEW_REF(arg, arg);
	// 提示：只有变量可以引用传递
	zend_error(E_NOTICE, "Only variables should be passed by reference");
	// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();

// 创建跳转标签 send_var
ZEND_VM_C_LABEL(send_var):
	// 获取zval指针，UNUSED 类型返回null
	varptr = GET_OP1_ZVAL_PTR(BP_VAR_R);
	// 如果变量是引用类型
	if (UNEXPECTED(Z_ISREF_P(varptr))) {
		zend_refcounted *ref = Z_COUNTED_P(varptr);
		// 解引用
		varptr = Z_REFVAL_P(varptr);
		ZVAL_COPY_VALUE(arg, varptr);
		// 引用次数-1，如果为0
		if (UNEXPECTED(GC_DELREF(ref) == 0)) {
			efree_size(ref, sizeof(zend_reference));
		// 否则， 如果是可以计数类型
		} else if (Z_OPT_REFCOUNTED_P(arg)) {
			// 引用次数+1
			Z_ADDREF_P(arg);
		}
	// 否则
	} else {
		ZVAL_COPY_VALUE(arg, varptr);
	}
	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, op1复制给result，不是引用转成引用传递
ZEND_VM_HANDLER(67, ZEND_SEND_REF, VAR|CV, CONST|UNUSED|NUM)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *varptr, *arg;

	// windows: 无操作
	SAVE_OPLINE();
	// 如果类型为 常量
	if (OP2_TYPE == IS_CONST) {
		// 参数名
		// 访问 (p2).zv。p1:opline,p2:node
		zend_string *arg_name = Z_STR_P(RT_CONSTANT(opline, opline->op2));
		// 
		uint32_t arg_num;
		// 处理命名参数，p1:执行数据指针的指针，p2:参数名，p3:参数数量 ，p4:缓存位置
		arg = zend_handle_named_arg(&EX(call), arg_name, &arg_num, CACHE_ADDR(opline->result.num));
		// 如果没找到参数
		if (UNEXPECTED(!arg)) {
			// 释放操作对象的附加变量
			FREE_OP1();
			// 主要是 return
			HANDLE_EXCEPTION();
		}
	// 否则
	} else {
		// 把p1 右移p2字节，转成zval返回
		arg = ZEND_CALL_VAR(EX(call), opline->result.var);
	}
	// 获取zval指针,TMP/CONST/UNUSED 返回null，不支持 TMPVAR/TMPVARCV
	varptr = GET_OP1_ZVAL_PTR_PTR(BP_VAR_W);
	// 如果op1是引用类型
	if (Z_ISREF_P(varptr)) {
		// 引用次数+1
		Z_ADDREF_P(varptr);
	// 否则
	} else {
		// 把zval转为引用，p1:zval，p2:引用次数
		ZVAL_MAKE_REF_EX(varptr, 2);
	}
	// op1的引用目标，引用赋值给 result
	ZVAL_REF(arg, Z_REF_P(varptr));

	// 释放操作对象的附加变量
	FREE_OP1();
	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// op1 复制给 result，支持命名参数
ZEND_VM_HOT_SEND_HANDLER(66, ZEND_SEND_VAR_EX, VAR|CV, CONST|UNUSED|NUM, SPEC(QUICK_ARG))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *varptr, *arg;
	uint32_t arg_num;

	// 如果类型为 常量。命名参数
	if (OP2_TYPE == IS_CONST) {
		// windows: 无操作
		SAVE_OPLINE();
		// 参数名
		// 访问 (p2).zv。p1:opline,p2:node
		zend_string *arg_name = Z_STR_P(RT_CONSTANT(opline, opline->op2));
		// 处理命名参数，p1:执行数据指针的指针，p2:参数名，p3:参数数量 ，p4:缓存位置
		arg = zend_handle_named_arg(&EX(call), arg_name, &arg_num, CACHE_ADDR(opline->result.num));
		// 如果没有获取到参数
		if (UNEXPECTED(!arg)) {
			// 释放操作对象的附加变量
			FREE_OP1();
			// 主要是 return
			HANDLE_EXCEPTION();
		}
	// 否则
	} else {
		// 把p1 右移p2字节，转成zval返回
		arg = ZEND_CALL_VAR(EX(call), opline->result.var);
		// 参数序号
		arg_num = opline->op2.num;
	}

	// 如果参数数量没有超限制
	if (EXPECTED(arg_num <= MAX_ARG_FLAG_NUM)) {
		// 如果参数可以引用传递
		if (QUICK_ARG_SHOULD_BE_SENT_BY_REF(EX(call)->func, arg_num)) {
			// 跳转到指定标签 send_var_by_ref
			ZEND_VM_C_GOTO(send_var_by_ref);
		}
	// 如果数量超限制，并且参数必须引用传递
	} else if (ARG_SHOULD_BE_SENT_BY_REF(EX(call)->func, arg_num)) {
// 创建跳转标签 send_var_by_ref
ZEND_VM_C_LABEL(send_var_by_ref):
		// 获取zval指针,TMP/CONST/UNUSED 返回null，不支持 TMPVAR/TMPVARCV
		varptr = GET_OP1_ZVAL_PTR_PTR(BP_VAR_W);
		// 如果op1是引用类型
		if (Z_ISREF_P(varptr)) {
			// 引用次数+1
			Z_ADDREF_P(varptr);
		// 否则
		} else {
			// 把zval转为引用，p1:zval，p2:引用次数
			ZVAL_MAKE_REF_EX(varptr, 2);
		}
		// 引用赋值 op1的目标给 result
		ZVAL_REF(arg, Z_REF_P(varptr));

		// 释放操作对象的附加变量
		FREE_OP1();
		// opline + 1，到目标操作码并 return
		ZEND_VM_NEXT_OPCODE();
	}

	// 获取zval指针, UNUSED 返回null
	varptr = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 如果 op1 是 编译变量 并且 值为未定义
	if (OP1_TYPE == IS_CV && UNEXPECTED(Z_TYPE_INFO_P(varptr) == IS_UNDEF)) {
		// windows: 无操作
		SAVE_OPLINE();
		// 报错：p1的变量（变量名）未定义, 并返未初始化zval
		ZVAL_UNDEFINED_OP1();
		// 结果为null
		ZVAL_NULL(arg);
		// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
		ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
	}

	// 如果 op1 是 编译变量
	if (OP1_TYPE == IS_CV) {
		// 复制引用目标 和 附加信息，并增加引用计数
		ZVAL_COPY_DEREF(arg, varptr);
	// 否则 如果 op1 是 普通变量
	} else /* if (OP1_TYPE == IS_VAR) */ {
		if (UNEXPECTED(Z_ISREF_P(varptr))) {
			// 取出op1的引用实例
			zend_refcounted *ref = Z_COUNTED_P(varptr);
			// 解引用
			varptr = Z_REFVAL_P(varptr);
			// op1 复制给 result
			ZVAL_COPY_VALUE(arg, varptr);
			// 引用次数-1，如果为0
			if (UNEXPECTED(GC_DELREF(ref) == 0)) {
				// 释放引用实例
				efree_size(ref, sizeof(zend_reference));
			// 否则， 如果是可以计数类型
			} else if (Z_OPT_REFCOUNTED_P(arg)) {
				// 引用次数+1
				Z_ADDREF_P(arg);
			}
		// 否则
		} else {
			// op1复制给result
			ZVAL_COPY_VALUE(arg, varptr);
		}
	}

	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, 检查参数是否通过引用传递，按需要添加 ZEND_CALL_SEND_ARG_BY_REF 标记
ZEND_VM_HOT_SEND_HANDLER(100, ZEND_CHECK_FUNC_ARG, UNUSED, CONST|UNUSED|NUM, SPEC(QUICK_ARG))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	uint32_t arg_num;

	// 如果op2类型为 常量
	if (OP2_TYPE == IS_CONST) {
		// 参数名
		// 访问 (p2).zv。p1:opline,p2:node
		zend_string *arg_name = Z_STR_P(RT_CONSTANT(opline, opline->op2));
		// 通过参数名称获取偏移量（参数序号）
		arg_num = zend_get_arg_offset_by_name(
			EX(call)->func, arg_name, CACHE_ADDR(opline->result.num)) + 1;
		// 如果参数数量是0
		if (UNEXPECTED(arg_num == 0)) {
			// 把它当成引用参数，在SEND时抛出错误
			/* Treat this as a by-value argument, and throw an error during SEND. */
			// 添加引用传递参数，标记
			ZEND_DEL_CALL_FLAG(EX(call), ZEND_CALL_SEND_ARG_BY_REF);
			// opline + 1，到目标操作码并 return
			ZEND_VM_NEXT_OPCODE();
		}
	// 否则，op2不是常量
	} else {
		// 参数数量 
		arg_num = opline->op2.num;
	}

	// 如果序号没有越界
	if (EXPECTED(arg_num <= MAX_ARG_FLAG_NUM)) {
		// 检验是否可以通过引用传递，如果可以
		if (QUICK_ARG_SHOULD_BE_SENT_BY_REF(EX(call)->func, arg_num)) {
			// 添加【发送引用参数】标记
			ZEND_ADD_CALL_FLAG(EX(call), ZEND_CALL_SEND_ARG_BY_REF);
		// 否则
		} else {
			// 删除【发送引用参数】标记
			ZEND_DEL_CALL_FLAG(EX(call), ZEND_CALL_SEND_ARG_BY_REF);
		}
	// 如果必须通过引用传递
	} else if (ARG_SHOULD_BE_SENT_BY_REF(EX(call)->func, arg_num)) {
		// 添加【发送引用参数】标记
		ZEND_ADD_CALL_FLAG(EX(call), ZEND_CALL_SEND_ARG_BY_REF);
	// 否则
	} else {
		// 删除【发送引用参数】标记
		ZEND_DEL_CALL_FLAG(EX(call), ZEND_CALL_SEND_ARG_BY_REF);
	}
	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, op1直接复制给result, 根据参数定义自动适配引用传递
ZEND_VM_HOT_HANDLER(185, ZEND_SEND_FUNC_ARG, VAR, CONST|UNUSED|NUM)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *varptr, *arg;

	// 如果类型为 常量
	if (OP2_TYPE == IS_CONST) {
		// TODO: Would it make sense to share the cache slot with CHECK_FUNC_ARG?
		// windows: 无操作
		SAVE_OPLINE();
		// 访问 (p2).zv。p1:opline,p2:node
		zend_string *arg_name = Z_STR_P(RT_CONSTANT(opline, opline->op2));
		uint32_t arg_num;
		// 处理命名参数，p1:执行数据指针的指针，p2:参数名，p3:参数数量 ，p4:缓存位置
		arg = zend_handle_named_arg(&EX(call), arg_name, &arg_num, CACHE_ADDR(opline->result.num));
		// 如果获取参数失败
		if (UNEXPECTED(!arg)) {
			// 释放操作对象的附加变量
			FREE_OP1();
			// 主要是 return
			HANDLE_EXCEPTION();
		}
	// 否则
	} else {
		// 把p1 右移p2字节，转成zval返回
		arg = ZEND_CALL_VAR(EX(call), opline->result.var);
	}

	// 如果要求引用传递
	if (UNEXPECTED(ZEND_CALL_INFO(EX(call)) & ZEND_CALL_SEND_ARG_BY_REF)) {
		// 获取zval指针,TMP/CONST/UNUSED 返回null，不支持 TMPVAR/TMPVARCV
		varptr = GET_OP1_ZVAL_PTR_PTR(BP_VAR_W);
		// 如果是引用类型
		if (Z_ISREF_P(varptr)) {
			// 引用次数+1
			Z_ADDREF_P(varptr);
		// 否则
		} else {
			// 把zval转为引用，p1:zval，p2:引用次数
			ZVAL_MAKE_REF_EX(varptr, 2);
		}
		// op1的引用目标 引用赋值给 result
		ZVAL_REF(arg, Z_REF_P(varptr));

		// 释放操作对象的附加变量
		FREE_OP1();
		// opline + 1，到目标操作码并 return
		ZEND_VM_NEXT_OPCODE();
	}

	// 获取zval指针, UNUSED 返回null
	varptr = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);

	// 如果 op1是引用类型
	if (UNEXPECTED(Z_ISREF_P(varptr))) {
		// 取出引用实例
		zend_refcounted *ref = Z_COUNTED_P(varptr);
		// 解引用
		varptr = Z_REFVAL_P(varptr);
		// 目标值复制给 result
		ZVAL_COPY_VALUE(arg, varptr);
		// 引用次数-1，如果为0
		if (UNEXPECTED(GC_DELREF(ref) == 0)) {
			// 释放引用实例
			efree_size(ref, sizeof(zend_reference));
		// 否则， 如果是可以计数类型
		} else if (Z_OPT_REFCOUNTED_P(arg)) {
			// 引用次数+1
			Z_ADDREF_P(arg);
		}
	// 否则，op1不是引用类型
	} else {
		// op1直接复制给result
		ZVAL_COPY_VALUE(arg, varptr);
	}

	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing2, 发送解包参数
ZEND_VM_HANDLER(165, ZEND_SEND_UNPACK, ANY, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *args;
	uint32_t arg_num;

	// windows: 无操作
	SAVE_OPLINE();
	// 获取zval指针, UNUSED 返回null
	args = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 执行数据中，参数数量 +1
	arg_num = ZEND_CALL_NUM_ARGS(EX(call)) + 1;

// 创建跳转标签 send_again
ZEND_VM_C_LABEL(send_again):
	// 如果 op1 是数组 
	if (EXPECTED(Z_TYPE_P(args) == IS_ARRAY)) {
		// 取出数组 
		HashTable *ht = Z_ARRVAL_P(args);
		zval *arg, *top;
		zend_string *name;
		// 命名参数数 0
		bool have_named_params = 0;

		// 扩展执行数据。如果当堆栈前空间够用，增加使用数。
		// 否则，新建堆栈并复制执行数据。p1:执行数据，p2:传入参数，p3:增加参数
		zend_vm_stack_extend_call_frame(&EX(call), arg_num - 1, zend_hash_num_elements(ht));

		// TODO: Speed this up using a flag that specifies whether there are any ref parameters.
		// 如果 op1 是普通变量或编译变量 并且 op1是引用类型
		if ((OP1_TYPE & (IS_VAR|IS_CV)) && Z_REFCOUNT_P(args) > 1) {
			// 
			uint32_t tmp_arg_num = arg_num;
			bool separate = 0;
			// 检查是否有参数要使用引用传递
			/* check if any of arguments are going to be passed by reference */
			// 遍历哈希表
			ZEND_HASH_FOREACH_STR_KEY_VAL(ht, name, arg) {
				// 如果有key
				if (UNEXPECTED(name)) {
					// 执行需要的临时变量
					void *cache_slot[2] = {NULL, NULL};
					// 通过参数名称获取偏移量（参数序号）
					tmp_arg_num = zend_get_arg_offset_by_name(
						EX(call)->func, name, cache_slot) + 1;
				}
				// 如果必须使用引用传递
				if (ARG_SHOULD_BE_SENT_BY_REF(EX(call)->func, tmp_arg_num)) {
					// 需要创建副本
					separate = 1;
					// 跳出
					break;
				}
				// 临时计数器
				tmp_arg_num++;
			} ZEND_HASH_FOREACH_END();
			// 如果需要创建副本
			if (separate) {
				// args创建副本，使用副本
				SEPARATE_ARRAY(args);
				// 取出哈希表
				ht = Z_ARRVAL_P(args);
			}
		}

		// 遍历 哈希表
		ZEND_HASH_FOREACH_STR_KEY_VAL(ht, name, arg) {
			// 如果有key
			if (UNEXPECTED(name)) {
				void *cache_slot[2] = {NULL, NULL};
				// 参数数量 1个
				have_named_params = 1;
				// 处理命名参数，p1:执行数据指针的指针，p2:参数名，p3:参数数量 ，p4:缓存位置
				top = zend_handle_named_arg(&EX(call), name, &arg_num, cache_slot);
				// 如果 top 无效
				if (UNEXPECTED(!top)) {
					// 释放操作对象的附加变量
					FREE_OP1();
					// 主要是 return
					HANDLE_EXCEPTION();
				}
			// 否则,没有key
			} else {
				// 如果有命名参数
				if (have_named_params) {
					// 报错：不可以把顺序参数放在命名参数后面
					zend_throw_error(NULL,
						"Cannot use positional argument after named argument during unpacking");
					// 释放操作对象的附加变量
					FREE_OP1();
					// 主要是 return
					HANDLE_EXCEPTION();
				}
				// 取得列表中第 （ZEND_CALL_FRAME_SLOT +） n 个 zval
				top = ZEND_CALL_ARG(EX(call), arg_num);
				// 执行数据中参数数量 +1
				ZEND_CALL_NUM_ARGS(EX(call))++;
			}

			// 如果必须用引用传递
			if (ARG_SHOULD_BE_SENT_BY_REF(EX(call)->func, arg_num)) {
				// 如果是引用类型
				if (Z_ISREF_P(arg)) {
					// 引用次数+1
					Z_ADDREF_P(arg);
					// arg的目标，引用赋值给top
					ZVAL_REF(top, Z_REF_P(arg));
				// 否则 如果 op1 是普通变量或编译变量
				} else if (OP1_TYPE & (IS_VAR|IS_CV)) {
					// array已经在上面创建了副本
					/* array is already separated above */
					// 把zval转为引用，p1:zval，p2:引用次数
					ZVAL_MAKE_REF_EX(arg, 2);
					// arg的目标，引用赋值给top
					ZVAL_REF(top, Z_REF_P(arg));
				// 否则
				} else {
					// 增加引用次数
					Z_TRY_ADDREF_P(arg);
					// 给 zval 创建新的 zend_reference ，从给出的 zval 中复制附加内容
					ZVAL_NEW_REF(top, arg);
				}
			// 否则
			} else {
				// 复制引用目标 和 附加信息，并增加引用计数
				ZVAL_COPY_DEREF(top, arg);
			}
			// 临时计数变量
			arg_num++;
		} ZEND_HASH_FOREACH_END();
	// 如果是对象
	} else if (EXPECTED(Z_TYPE_P(args) == IS_OBJECT)) {
		// 对象所属类
		zend_class_entry *ce = Z_OBJCE_P(args);
		//
		zend_object_iterator *iter;
		// 命名参数数量 0
		bool have_named_params = 0;

		// 如果无法取得迭代器
		if (!ce || !ce->get_iterator) {
			// 报错：只有数据和可遍历类型可被解包
			zend_type_error("Only arrays and Traversables can be unpacked");
		// 否则，可以取得迭代器
		} else {
			// 取得迭代器
			iter = ce->get_iterator(ce, args, 0);
			// 如果没有获取到
			if (UNEXPECTED(!iter)) {
				// 释放操作对象的附加变量
				FREE_OP1();
				// 如果没有异常
				if (!EG(exception)) {
					// 抛异常：此类型的对象无法创建迭代器
					zend_throw_exception_ex(
						NULL, 0, "Object of type %s did not create an Iterator", ZSTR_VAL(ce->name)
					);
				}
				// 主要是 return
				HANDLE_EXCEPTION();
			}
			// 迭代器方法列表
			const zend_object_iterator_funcs *funcs = iter->funcs;
			// 如果有重置方法
			if (funcs->rewind) {
				// 先重置
				funcs->rewind(iter);
			}

			// 如果迭代位置有效，就一直迭代下去，arg_num 计数
			for (; funcs->valid(iter) == SUCCESS; ++arg_num) {
				zval *arg, *top;

				// 如果有异常
				if (UNEXPECTED(EG(exception) != NULL)) {
					// 跳出
					break;
				}
				// 取出当前数据
				arg = funcs->get_current_data(iter);
				// 如果有异常
				if (UNEXPECTED(EG(exception) != NULL)) {
					// 跳出
					break;
				}
				// 
				zend_string *name = NULL;
				// 如果能获取当前key
				if (funcs->get_current_key) {
					zval key;
					// 获取当前key
					funcs->get_current_key(iter, &key);
					// 如果有异常
					if (UNEXPECTED(EG(exception) != NULL)) {
						// 跳出
						break;
					}
					// 如果key不是整数
					if (UNEXPECTED(Z_TYPE(key) != IS_LONG)) {
						// 如果key不是字串
						if (UNEXPECTED(Z_TYPE(key) != IS_STRING)) {
							// 报错：解包时key必须是整数或数字
							zend_throw_error(NULL,
								"Keys must be of type int|string during argument unpacking");
							// 销毁 key
							zval_ptr_dtor(&key);
							// 跳出
							break;
						}
						// 取出key 字串
						name = Z_STR_P(&key);
					}
				}

				// 如果取到了key，变量名
				if (UNEXPECTED(name)) {
					// 执行需要的临时变量
					void *cache_slot[2] = {NULL, NULL};
					// 命名参数，默认1个
					have_named_params = 1;
					// 处理命名参数，p1:执行数据指针的指针，p2:参数名，p3:参数数量 ，p4:缓存位置
					top = zend_handle_named_arg(&EX(call), name, &arg_num, cache_slot);
					// 如果参数不存在
					if (UNEXPECTED(!top)) {
						// 释放变量名
						zend_string_release(name);
						// 跳出
						break;
					}
					// 减少引用次数
					ZVAL_DEREF(arg);
					// 增加引用次数
					Z_TRY_ADDREF_P(arg);
					// 如果参数必须使用引用传递
					if (ARG_MUST_BE_SENT_BY_REF(EX(call)->func, arg_num)) {
						// 报错：不可以通过解包来传递引用参数。直接传递引用参数即可。
						zend_error(
							E_WARNING, "Cannot pass by-reference argument %d of %s%s%s()"
							" by unpacking a Traversable, passing by-value instead", arg_num,
							EX(call)->func->common.scope ? ZSTR_VAL(EX(call)->func->common.scope->name) : "",
							EX(call)->func->common.scope ? "::" : "",
							ZSTR_VAL(EX(call)->func->common.function_name)
						);
						// 给 zval 创建新的 zend_reference ，从给出的 zval 中复制附加内容
						ZVAL_NEW_REF(top, arg);
					// 否则
					} else {
						// 把
						ZVAL_COPY_VALUE(top, arg);
					}
					// 释放变量名
					zend_string_release(name);
				// 否则，没有变量名
				} else {
					// 如果有命名参数
					if (have_named_params) {
						// 报错：不可以把顺序参数放在命名参数后面
						zend_throw_error(NULL,
							"Cannot use positional argument after named argument during unpacking");
						// 跳出
						break;
					}
					// 扩展执行数据。如果当堆栈前空间够用，增加使用数。
					// 否则，新建堆栈并复制执行数据。p1:执行数据，p2:传入参数，p3:增加参数
					zend_vm_stack_extend_call_frame(&EX(call), arg_num - 1, 1);
					// 取得列表中第 （ZEND_CALL_FRAME_SLOT +） n 个 zval
					top = ZEND_CALL_ARG(EX(call), arg_num);
					// 减少引用次数
					ZVAL_DEREF(arg);
					// 增加引用次数
					Z_TRY_ADDREF_P(arg);
					// 如果参数必须用引用传递
					if (ARG_MUST_BE_SENT_BY_REF(EX(call)->func, arg_num)) {
						// 警告：不可以引用传递？？
						zend_error(
							E_WARNING, "Cannot pass by-reference argument %d of %s%s%s()"
							" by unpacking a Traversable, passing by-value instead", arg_num,
							EX(call)->func->common.scope ? ZSTR_VAL(EX(call)->func->common.scope->name) : "",
							EX(call)->func->common.scope ? "::" : "",
							ZSTR_VAL(EX(call)->func->common.function_name)
						);
						// 给 zval 创建新的 zend_reference ，从给出的 zval 中复制附加内容
						ZVAL_NEW_REF(top, arg);
					// 否则，不必须用引用传递
					} else {
						// arg 复制给 top
						ZVAL_COPY_VALUE(top, arg);
					}

					// 执行数据中参数数量 +1
					ZEND_CALL_NUM_ARGS(EX(call))++;
				}

				// 迭代 下一个元素
				funcs->move_forward(iter);
			}
			// 销毁迭代器
			zend_iterator_dtor(iter);
		}
	// 如果是引用类型
	} else if (EXPECTED(Z_ISREF_P(args))) {
		// 解引用
		args = Z_REFVAL_P(args);
		// 跳转到指定标签 send_again
		ZEND_VM_C_GOTO(send_again);
	// 否则
	} else {
		// 如果 op1 是 编译变量 并且 参数值为未定义
		if (OP1_TYPE == IS_CV && UNEXPECTED(Z_TYPE_P(args) == IS_UNDEF)) {
			// 报错：p1的变量（变量名）未定义, 并返未初始化zval
			ZVAL_UNDEFINED_OP1();
		}
		// 报错：只有数据和可遍历类型可被解包
		zend_type_error("Only arrays and Traversables can be unpacked");
	}

	// 释放操作对象的附加变量
	FREE_OP1();
	// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing, 发送数组
ZEND_VM_HANDLER(119, ZEND_SEND_ARRAY, ANY, ANY, NUM)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *args;

	// windows: 无操作
	SAVE_OPLINE();
	// 获取zval指针，UNUSED 类型返回null
	args = GET_OP1_ZVAL_PTR(BP_VAR_R);
	// 如果参数表不是array
	if (UNEXPECTED(Z_TYPE_P(args) != IS_ARRAY)) {
		// #define IS_VAR (1<<2)
		// #define IS_CV (1<<3) /* Compiled variable */
		// 如果op1是变量或编译变量，并且参数列表是引用类型
		if ((OP1_TYPE & (IS_VAR|IS_CV)) && Z_ISREF_P(args)) {
			// 追踪到引用
			args = Z_REFVAL_P(args);
			// 如果追踪后的参数列表是 array
			if (EXPECTED(Z_TYPE_P(args) == IS_ARRAY)) {
				// 跳转到指定标签 send_array
				ZEND_VM_C_GOTO(send_array);
			}
		}
		// 报错，call_user_func_array 的第二个参数必须是数组 
		zend_type_error("call_user_func_array(): Argument #2 ($args) must be of type array, %s given", zend_zval_type_name(args));
		// 清空两个op
		// 释放操作对象的附加变量
		FREE_OP2();
		// 释放操作对象的附加变量
		FREE_OP1();
		// 主要是 return
		HANDLE_EXCEPTION();
	// 如果参数表是array
	// 否则
	} else {
		uint32_t arg_num;
		HashTable *ht;
		zval *arg, *param;

// ing2, 创建跳转标签 send_array
ZEND_VM_C_LABEL(send_array):
		// 取出参数列表
		ht = Z_ARRVAL_P(args);
		// op2的值不是 未使用
		if (OP2_TYPE != IS_UNUSED) {
			// 这里不需要处理命名参数，因为 array_slice 被调用时 $preserve_keys == false
			/* We don't need to handle named params in this case,
			 * because array_slice() is called with $preserve_keys == false. */
			// 获取zval指针, UNUSED 返回null，不支持 TMPVAR/TMPVARCV
			zval *op2 = GET_OP2_ZVAL_PTR_DEREF(BP_VAR_R);
			// 要跳过的参数数量 
			uint32_t skip = opline->extended_value;
			// 参数数量 
			uint32_t count = zend_hash_num_elements(ht);
			zend_long len;
			// 如果op2的类型是 整数 
			if (EXPECTED(Z_TYPE_P(op2) == IS_LONG)) {
				// 取出整数
				len = Z_LVAL_P(op2);
			// 如果op2的类型是 NULL
			} else if (Z_TYPE_P(op2) == IS_NULL) {
				// 数量 - 跳过数
				len = count - skip;
			// 如果使用严格类型，或 
			// 把传入的参数转换成整数，遇到null会报错。p2:返回值，p3:arg参数序号，报错用。转换失败
			} else if (EX_USES_STRICT_TYPES()
					|| !zend_parse_arg_long_weak(op2, &len, /* arg_num */ 3)) {
				// 报错：array_slice 参数3必须是 int型或为null
				zend_type_error(
					"array_slice(): Argument #3 ($length) must be of type ?int, %s given",
					zend_zval_type_name(op2));
				// 释放操作对象的附加变量
				FREE_OP2();
				// 释放操作对象的附加变量
				FREE_OP1();
				// 主要是 return
				HANDLE_EXCEPTION();
			}

			// 这里主要是为了切片
			// 如果长度小于0 ，倒着切片
			if (len < 0) {
				// 换算切片位置
				len += (zend_long)(count - skip);
			}
			// 如果skip没有超过有效值，并且len有效
			if (skip < count && len > 0) {
				// 如果切片长度越界
				if (len > (zend_long)(count - skip)) {
					// 调整到最大可能长度
					len = (zend_long)(count - skip);
				}
				// 扩展执行数据。如果当堆栈前空间够用，增加使用数。
				// 否则，新建堆栈并复制执行数据。p1:执行数据，p2:传入参数，p3:增加参数
				zend_vm_stack_extend_call_frame(&EX(call), 0, len);
				// 参数数量 1
				arg_num = 1;
				// 取得列表中第 （ZEND_CALL_FRAME_SLOT +） n 个 zval
				param = ZEND_CALL_ARG(EX(call), 1);
				// 遍历哈希表
				ZEND_HASH_FOREACH_VAL(ht, arg) {
					// 默认无需包装成引用
					bool must_wrap = 0;
					// 如果需要skip
					if (skip > 0) {
						// 已跳过1个
						skip--;
						// 跳过
						continue;
					// 如果获取数量已经大于等于需要的数量
					} else if ((zend_long)(arg_num - 1) >= len) {
						// 跳出
						break;
					// 如果此参数必须 引用传递
					} else if (ARG_SHOULD_BE_SENT_BY_REF(EX(call)->func, arg_num)) {
						// 如果不是引用类型
						if (UNEXPECTED(!Z_ISREF_P(arg))) {
							// 如果此参数不可以使用引用传递
							if (!ARG_MAY_BE_SENT_BY_REF(EX(call)->func, arg_num)) {
								// 不可以使用引用传递，发出警告，但仍然进行调用
								/* By-value send is not allowed -- emit a warning,
								 * but still perform the call. */
								// 报错，此参数必须引用传递
								zend_param_must_be_ref(EX(call)->func, arg_num);
								// 必须包装成引用
								must_wrap = 1;
							}
						}
					// 否则，此参数不可引用传递
					} else {
						// 如果是引用类型
						if (Z_ISREF_P(arg) &&
						    !(EX(call)->func->common.fn_flags & ZEND_ACC_CALL_VIA_TRAMPOLINE)) {
							/* don't separate references for __call */
							// 解引用
							arg = Z_REFVAL_P(arg);
						}
					}
					// 如果不必须包装
					if (EXPECTED(!must_wrap)) {
						// 复制参数
						ZVAL_COPY(param, arg);
					// 否则
					} else {
						// 增加引用次数
						Z_TRY_ADDREF_P(arg);
						// 给 zval 创建新的 zend_reference ，从给出的 zval 中复制附加内容
						ZVAL_NEW_REF(param, arg);
					}
					// 上下文中参数数量+1
					ZEND_CALL_NUM_ARGS(EX(call))++;
					// 获取到参数数量 +1
					arg_num++;
					// 下一个参数
					param++;
				} ZEND_HASH_FOREACH_END();
			}
			// 释放操作对象的附加变量
			FREE_OP2();
		// 否则 op2的值是 未使用
		} else {
			zend_string *name;
			bool have_named_params;
			// 扩展执行数据。如果当堆栈前空间够用，增加使用数。
			// 否则，新建堆栈并复制执行数据。p1:执行数据，p2:传入参数，p3:增加参数
			zend_vm_stack_extend_call_frame(&EX(call), 0, zend_hash_num_elements(ht));
			// 参数数量
			arg_num = 1;
			// 取得列表中第 （ZEND_CALL_FRAME_SLOT +） n 个 zval
			param = ZEND_CALL_ARG(EX(call), 1);
			// 默认没有命名参数
			have_named_params = 0;
			// 遍历哈希表
			ZEND_HASH_FOREACH_STR_KEY_VAL(ht, name, arg) {
				// 如果有key
				if (name) {
					// 临时变量
					void *cache_slot[2] = {NULL, NULL};
					// 有命名参数
					have_named_params = 1;
					// 处理命名参数，p1:执行数据指针的指针，p2:参数名，p3:参数数量 ，p4:缓存位置
					param = zend_handle_named_arg(&EX(call), name, &arg_num, cache_slot);
					// 如果参数获取失败
					if (!param) {
						// 释放操作对象的附加变量
						FREE_OP1();
						// 主要是 return
						HANDLE_EXCEPTION();
					}
				// 如果没有key 又有命名参数
				} else if (have_named_params) {
					// 抛错：不可以把顺序参数放在命名参数后面
					zend_throw_error(NULL,
						"Cannot use positional argument after named argument");
					// 释放操作对象的附加变量
					FREE_OP1();
					// 主要是 return
					HANDLE_EXCEPTION();
				}

				// 默认无须包装成引用
				bool must_wrap = 0;
				// 如果此参数必须通过引用传递
				if (ARG_SHOULD_BE_SENT_BY_REF(EX(call)->func, arg_num)) {
					// 如果值不是引用类型
					if (UNEXPECTED(!Z_ISREF_P(arg))) {
						// 如果此参数不可以用引用传递
						if (!ARG_MAY_BE_SENT_BY_REF(EX(call)->func, arg_num)) {
							// 不允许使用引用传递，发出警告，但仍然进行调用
							/* By-value send is not allowed -- emit a warning,
							 * but still perform the call. */
							// 报错，此参数必须引用传递
							zend_param_must_be_ref(EX(call)->func, arg_num);
							// 必须 包装成引用
							must_wrap = 1;
						}
					}
				// 否则
				} else {
					// 如果值为引用类型 并且 此函数不是通过弹跳创建的
					if (Z_ISREF_P(arg) &&
					    !(EX(call)->func->common.fn_flags & ZEND_ACC_CALL_VIA_TRAMPOLINE)) {
						// 不需要为了 __call 创建引用副本
						/* don't separate references for __call */
						// 解引用
						arg = Z_REFVAL_P(arg);
					}
				}
				// 如果不必须包装
				if (EXPECTED(!must_wrap)) {
					// 复制参数
					ZVAL_COPY(param, arg);
				// 否则
				} else {
					// 增加引用次数
					Z_TRY_ADDREF_P(arg);
					// 给 zval 创建新的 zend_reference ，从给出的 zval 中复制附加内容
					ZVAL_NEW_REF(param, arg);
				}
				// 如果没有名称
				if (!name) {
					// 操作码参数数+1
					ZEND_CALL_NUM_ARGS(EX(call))++;
					// 参数数量 +1
					arg_num++;
					// 下一个参数
					param++;
				}
			} ZEND_HASH_FOREACH_END();
		}
	}
	// 释放操作对象的附加变量
	FREE_OP1();
	// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, 读取参数，有一个引用处理
ZEND_VM_HANDLER(120, ZEND_SEND_USER, CONST|TMP|VAR|CV, NUM)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *arg, *param;

	// windows: 无操作
	SAVE_OPLINE();
	// 获取zval指针, UNUSED 返回null，不支持 TMPVAR/TMPVARCV
	arg = GET_OP1_ZVAL_PTR_DEREF(BP_VAR_R);
	// 把p1 右移p2字节，转成zval返回
	param = ZEND_CALL_VAR(EX(call), opline->result.var);
	// 如果参数必须要引用传递
	if (UNEXPECTED(ARG_MUST_BE_SENT_BY_REF(EX(call)->func, opline->op2.num))) {
		// 报错，此参数必须引用传递
		zend_param_must_be_ref(EX(call)->func, opline->op2.num);
		// 增加引用次数
		Z_TRY_ADDREF_P(arg);
		// 把 param 更新成引用类型
		// 给 zval 创建新的 zend_reference ，从给出的 zval 中复制附加内容
		ZVAL_NEW_REF(param, arg);
	// 否则
	} else {
		// 把 op1的值直接给参数
		ZVAL_COPY(param, arg);
	}

	// 释放操作对象的附加变量
	FREE_OP1();
	// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, 处理未定义的参数，给它们添加默认值
ZEND_VM_HOT_HANDLER(199, ZEND_CHECK_UNDEF_ARGS, UNUSED, UNUSED)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	// 调用上下文
	zend_execute_data *call = execute_data->call;
	
	// ??
	// 如果没有 ZEND_CALL_MAY_HAVE_UNDEF 标记
	if (EXPECTED(!(ZEND_CALL_INFO(call) & ZEND_CALL_MAY_HAVE_UNDEF))) {
		// opline + 1，到目标操作码并 return
		ZEND_VM_NEXT_OPCODE();
	}

	// windows: 无操作
	SAVE_OPLINE();
	// 处理未定义的参数，给它们添加默认值，p1:执行数据
	zend_handle_undef_args(call);
	// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, 报错：传入的参数太少
ZEND_VM_COLD_HELPER(zend_missing_arg_helper, ANY, ANY)
{
#ifdef ZEND_VM_IP_GLOBAL_REG
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// windows: 无操作
	SAVE_OPLINE();
#endif
	// 报错：传入参数太少
	zend_missing_arg_error(execute_data);
	// 主要是 return
	HANDLE_EXCEPTION();
}

// ing3, 检验参数 的类型是否匹配
ZEND_VM_HELPER(zend_verify_recv_arg_type_helper, ANY, ANY, zval *op_1)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// windows: 无操作
	SAVE_OPLINE();
	// 检验参数 的类型是否匹配，p1:函数，p2:参数序号，p3:参数，p4:缓存位置。 如果不匹配
	if (UNEXPECTED(!zend_verify_recv_arg_type(EX(func), opline->op1.num, op_1, CACHE_ADDR(opline->extended_value)))) {
		// 主要是 return
		HANDLE_EXCEPTION();
	}

	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, 检验获取到的参数 数量和类型
ZEND_VM_HOT_HANDLER(63, ZEND_RECV, NUM, UNUSED, CACHE_SLOT)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	// op1是参数序号
	uint32_t arg_num = opline->op1.num;
	zval *param;

	// 如果需要参数数量大于 实际参数数量
	if (UNEXPECTED(arg_num > EX_NUM_ARGS())) {
		// 报错：传入的参数太少
		// 调用方法 zend_missing_arg_helper
		ZEND_VM_DISPATCH_TO_HELPER(zend_missing_arg_helper);
	}

	// 取出结果
	// 在执行数据中取出 指定序号的变量
	param = EX_VAR(opline->result.var);

	// op2是类型
	// 如果参数类型不是要求的类型
	if (UNEXPECTED(!(opline->op2.num & (1u << Z_TYPE_P(param))))) {
		// 检验参数 的类型是否匹配
		// 调用方法 zend_verify_recv_arg_type_helper
		ZEND_VM_DISPATCH_TO_HELPER(zend_verify_recv_arg_type_helper, op_1, param);
	}
	
	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, 检验获取到的参数 数量
ZEND_VM_HOT_TYPE_SPEC_HANDLER(ZEND_RECV, op->op2.num == MAY_BE_ANY, ZEND_RECV_NOTYPE, NUM, NUM, CACHE_SLOT)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	// 参数序号
	uint32_t arg_num = opline->op1.num;

	// 如果需要参数数量大于 实际参数数量
	if (UNEXPECTED(arg_num > EX_NUM_ARGS())) {
		// 报错：传入的参数太少
		// 调用方法 zend_missing_arg_helper
		ZEND_VM_DISPATCH_TO_HELPER(zend_missing_arg_helper);
	}

	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, 接收参数，有默认值
ZEND_VM_HOT_HANDLER(64, ZEND_RECV_INIT, NUM, CONST, CACHE_SLOT)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	uint32_t arg_num;
	zval *param;
	// do {
	ZEND_VM_REPEATABLE_OPCODE
	// 参数数量
	arg_num = opline->op1.num;
	// 结果
	// 在执行数据中取出 指定序号的变量
	param = EX_VAR(opline->result.var);
	// 如果此参数不是必填参数（有默认值）
	if (arg_num > EX_NUM_ARGS()) {
		// op2 是 默认值
		// 访问 (p2).zv。p1:opline,p2:node
		zval *default_value = RT_CONSTANT(opline, opline->op2);

		// 如果默认值是常量表达式
		if (Z_OPT_TYPE_P(default_value) == IS_CONSTANT_AST) {
			// 从缓存中取出
			zval *cache_val = (zval*)CACHE_ADDR(Z_CACHE_SLOT_P(default_value));

			// 缓存中只保存不计引用数的值
			/* we keep in cache only not refcounted values */
			// 如果缓值存有效
			if (Z_TYPE_P(cache_val) != IS_UNDEF) {
				// 缓存值 复制给 结果
				ZVAL_COPY_VALUE(param, cache_val);
			// 否则
			} else {
				// windows: 无操作
				SAVE_OPLINE();
				// 使用默认值
				ZVAL_COPY(param, default_value);
				// 更新常量
				if (UNEXPECTED(zval_update_constant_ex(param, EX(func)->op_array.scope) != SUCCESS)) {
					// 销毁没有引用次数的对象,并没有销毁 zval本身，不放入gc回收周期。
					zval_ptr_dtor_nogc(param);
					// 参数 为 未定义
					ZVAL_UNDEF(param);
					// 主要是 return
					HANDLE_EXCEPTION();
				}
				// 如果不是 可计数类型
				if (!Z_REFCOUNTED_P(param)) {
					// 结果记录到缓存里
					ZVAL_COPY_VALUE(cache_val, param);
				}
			}
			// 跳转到指定标签 recv_init_check_type
			ZEND_VM_C_GOTO(recv_init_check_type);
		// 否则
		} else {
			// 使用默认值
			ZVAL_COPY(param, default_value);
		}
	// 否则，是必填参数
	} else {
// 创建跳转标签 recv_init_check_type
ZEND_VM_C_LABEL(recv_init_check_type):
		// 如果参数有指定类型
		if (UNEXPECTED((EX(func)->op_array.fn_flags & ZEND_ACC_HAS_TYPE_HINTS) != 0)) {
			// windows: 无操作
			SAVE_OPLINE();
			// 检验参数 的类型是否匹配，p1:函数，p2:参数序号，p3:参数，p4:缓存位置
			// 如果失败
			if (UNEXPECTED(!zend_verify_recv_arg_type(EX(func), arg_num, param, CACHE_ADDR(opline->extended_value)))) {
				// 主要是 return
				HANDLE_EXCEPTION();
			}
		}
	}

	// 循环，直到碰到 ZEND_RECV_INIT
	// while() 循环结尾段，找下一个操作码，直到碰到p1操作码
	ZEND_VM_REPEAT_OPCODE(ZEND_RECV_INIT);
	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing2, 接收字典参数
ZEND_VM_HANDLER(164, ZEND_RECV_VARIADIC, NUM, UNUSED, CACHE_SLOT)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	// 参数序号
	uint32_t arg_num = opline->op1.num;
	// 必要参数数
	uint32_t arg_count = EX_NUM_ARGS();
	//
	zval *params;

	// windows: 无操作
	SAVE_OPLINE();

	// 在执行数据中取出 指定序号的变量
	params = EX_VAR(opline->result.var);

	// 参数在有效位置
	if (arg_num <= arg_count) {
		// 必须要能接收 字典参数
		ZEND_ASSERT(EX(func)->common.fn_flags & ZEND_ACC_VARIADIC);
		// 参数数量 必须是 此序号-1
		ZEND_ASSERT(EX(func)->common.num_args == arg_num - 1);
		// 取得参数信息
		zend_arg_info *arg_info = &EX(func)->common.arg_info[arg_num - 1];

		// 初始化参数表
		array_init_size(params, arg_count - arg_num + 1);
		// 初始化成顺序数组
		zend_hash_real_init_packed(Z_ARRVAL_P(params));
		// 向参数表中插入元素
		ZEND_HASH_FILL_PACKED(Z_ARRVAL_P(params)) {
			// 取得一个参数
			zval *param = EX_VAR_NUM(EX(func)->op_array.last_var + EX(func)->op_array.T);
			// 如果类型存在
			if (UNEXPECTED(ZEND_TYPE_IS_SET(arg_info->type))) {
				// Z_TYPE_INFO((p1)->This) |= p2
				ZEND_ADD_CALL_FLAG(execute_data, ZEND_CALL_FREE_EXTRA_ARGS);
				// 遍历所有非字典参数
				do {
					// 检验字典参数类型，p1:函数，p2:参数信息，p3:参数序号，p4:参数，p5:缓存位置
					// 如果不成功
					if (UNEXPECTED(!zend_verify_variadic_arg_type(EX(func), arg_info, arg_num, param, CACHE_ADDR(opline->extended_value)))) {
						// 完成填充
						ZEND_HASH_FILL_FINISH();
						// 主要是 return
						HANDLE_EXCEPTION();
					}
					// 如果是可以计数类型
					// 引用次数+1
					if (Z_OPT_REFCOUNTED_P(param)) Z_ADDREF_P(param);
					// 把参数添加进参数表
					ZEND_HASH_FILL_ADD(param);
					// 下一个参数
					param++;
					// 
				} while (++arg_num <= arg_count);
			// 否则，类型不存在
			} else {
				// 遍历所有非字典参数
				do {
					// 如果是可以计数类型
					// 引用次数+1
					if (Z_OPT_REFCOUNTED_P(param)) Z_ADDREF_P(param);
					// 把此 zval 添加进参数表
					ZEND_HASH_FILL_ADD(param);
					// 下一个
					param++;
				} while (++arg_num <= arg_count);
			}
		} ZEND_HASH_FILL_END();
	// 否则，参数不在有效位置
	} else {
		// 参数表是空数组
		ZVAL_EMPTY_ARRAY(params);
	}

	// 如果有额外的命名参数
	if (EX_CALL_INFO() & ZEND_CALL_HAS_EXTRA_NAMED_PARAMS) {
		zend_string *name;
		zval *param;
		// 参数信息
		zend_arg_info *arg_info = &EX(func)->common.arg_info[EX(func)->common.num_args];
		// 如果类型存在
		if (ZEND_TYPE_IS_SET(arg_info->type)) {
			// 给参数表创建副本，使用副本
			SEPARATE_ARRAY(params);
			// 遍历所有额外命名参数
			ZEND_HASH_MAP_FOREACH_STR_KEY_VAL(EX(extra_named_params), name, param) {
				// 检验字典参数类型，p1:函数，p2:参数信息，p3:参数序号，p4:参数，p5:缓存位置
				// 如果不成功
				if (UNEXPECTED(!zend_verify_variadic_arg_type(EX(func), arg_info, arg_num, param, CACHE_ADDR(opline->extended_value)))) {
					// 主要是 return
					HANDLE_EXCEPTION();
				}
				// 增加引用次数
				Z_TRY_ADDREF_P(param);
				// 添加新参数
				zend_hash_add_new(Z_ARRVAL_P(params), name, param);
			} ZEND_HASH_FOREACH_END();
		// 如果参数表里没有元素
		} else if (zend_hash_num_elements(Z_ARRVAL_P(params)) == 0) {
			// 原额外参数表，增加引用次数
			GC_ADDREF(EX(extra_named_params));
			// 直接使用原额外参数表
			ZVAL_ARR(params, EX(extra_named_params));
		// 否则。没有类型，并且参数表里有参数
		} else {
			// 给参数表创建副本，使用副本
			SEPARATE_ARRAY(params);
			// 遍历额外命名参数
			ZEND_HASH_MAP_FOREACH_STR_KEY_VAL(EX(extra_named_params), name, param) {
				// 增加引用次数
				Z_TRY_ADDREF_P(param);
				// 把命名参数添加进参数表
				zend_hash_add_new(Z_ARRVAL_P(params), name, param);
			} ZEND_HASH_FOREACH_END();
		}
	}

	// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, 把变量转成 布尔 型
ZEND_VM_COLD_CONST_HANDLER(52, ZEND_BOOL, CONST|TMPVAR|CV, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *val;

	// 获取op1的zval指针, UNUSED 返回null
	val = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 如果值为true
	if (Z_TYPE_INFO_P(val) == IS_TRUE) {
		// 结果为 true
		ZVAL_TRUE(EX_VAR(opline->result.var));
	// 否则 如果值为 undef/null/false
	} else if (EXPECTED(Z_TYPE_INFO_P(val) <= IS_TRUE)) {
		// result和op1两个运算对象 可能是同一个编译变量
		/* The result and op1 can be the same cv zval */
		// 原 op1 的变量类型
		const uint32_t orig_val_type = Z_TYPE_INFO_P(val);
		// 结果为 false
		ZVAL_FALSE(EX_VAR(opline->result.var));
		// 如果 op1 是 编译变量 并且 原值类型为 未定义
		if (OP1_TYPE == IS_CV && UNEXPECTED(orig_val_type == IS_UNDEF)) {
			// windows: 无操作
			SAVE_OPLINE();
			// 报错：p1的变量（变量名）未定义, 并返未初始化zval
			ZVAL_UNDEFINED_OP1();
			// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
			ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
		}
	// 否则，其他值都为true
	} else {
		// windows: 无操作
		SAVE_OPLINE();
		// 把值转成 bool 型放到结果里
		ZVAL_BOOL(EX_VAR(opline->result.var), i_zend_is_true(val));
		// 释放操作对象的附加变量
		FREE_OP1();
		// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
		ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
	}
	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, 给case语句计算相等的比较运算, 检验传入的变量是否有效，进行 比较 运算，然后销毁运算对象op2
ZEND_VM_HELPER(zend_case_helper, ANY, ANY, zval *op_1, zval *op_2)
{
	int ret;
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// windows: 无操作
	SAVE_OPLINE();
	// 如果 变量类型为未定义
	if (UNEXPECTED(Z_TYPE_INFO_P(op_1) == IS_UNDEF)) {
		// 报错：p1的变量（变量名）未定义, 并返未初始化zval
		op_1 = ZVAL_UNDEFINED_OP1();
	}
	// 如果 变量类型为 未定义
	if (UNEXPECTED(Z_TYPE_INFO_P(op_2) == IS_UNDEF)) {
		// 报错：p2的变量（变量名）未定义, 并返未初始化zval
		op_2 = ZVAL_UNDEFINED_OP2();
	}
	// 进行比较运算
	ret = zend_compare(op_1, op_2);
	// 如果 op2 类型 是变量或临时变量
	if (OP2_TYPE & (IS_TMP_VAR|IS_VAR)) {
		// 销毁没有引用次数的对象,并没有销毁 zval本身，不放入gc回收周期。
		zval_ptr_dtor_nogc(op_2);
	}
	// 根据当前操作码结果选择下一个操作码，p1:传入值，主要用来做判断。p2:是否检查异常
	ZEND_VM_SMART_BRANCH(ret == 0, 1);
}

// ing3, case语句，计算相等的比较运算
ZEND_VM_HANDLER(48, ZEND_CASE, TMPVAR, CONST|TMPVAR|CV)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2;
	double d1, d2;

	// 获取zval指针, UNUSED 返回null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 获取zval指针, UNUSED 返回null
	op2 = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 如果op1是整数
	if (EXPECTED(Z_TYPE_P(op1) == IS_LONG)) {
		// 如果op2是整数
		if (EXPECTED(Z_TYPE_P(op2) == IS_LONG)) {
			// 如果op1,op2相等
			if (EXPECTED(Z_LVAL_P(op1) == Z_LVAL_P(op2))) {
// 创建跳转标签 case_true
ZEND_VM_C_LABEL(case_true):
				// 根据当前操作码结果选择下一个操作码。如果不是跳转类型，结果赋值为true。
				ZEND_VM_SMART_BRANCH_TRUE();
			// 否则, 不匹配
			} else {
// 创建跳转标签 case_false
ZEND_VM_C_LABEL(case_false):
				// 根据当前操作码结果选择下一个操作码。如果不是跳转类型，结果赋值为false。
				ZEND_VM_SMART_BRANCH_FALSE();
			}
		// 如果op2是小数
		} else if (EXPECTED(Z_TYPE_P(op2) == IS_DOUBLE)) {
			// 整数转小数
			d1 = (double)Z_LVAL_P(op1);
			// 取出小数
			d2 = Z_DVAL_P(op2);
			// 跳转到指定标签 case_double
			ZEND_VM_C_GOTO(case_double);
		}
	// 否则 如果 op1 是小数
	} else if (EXPECTED(Z_TYPE_P(op1) == IS_DOUBLE)) {
		// 否则如果 op2 是小数
		if (EXPECTED(Z_TYPE_P(op2) == IS_DOUBLE)) {
			// 取出小数
			d1 = Z_DVAL_P(op1);
			// 取出小数
			d2 = Z_DVAL_P(op2);
// 创建跳转标签 case_double
ZEND_VM_C_LABEL(case_double):
			// 直接比较相等
			if (d1 == d2) {
				// 跳转到指定标签 case_true
				ZEND_VM_C_GOTO(case_true);
			// 否则
			} else {
				// 跳转到指定标签 case_false
				ZEND_VM_C_GOTO(case_false);
			}
		// 如果op2是整数
		} else if (EXPECTED(Z_TYPE_P(op2) == IS_LONG)) {
			// 取出小数
			d1 = Z_DVAL_P(op1);
			// 整数转成小数
			d2 = (double)Z_LVAL_P(op2);
			// 跳转到指定标签 case_double
			ZEND_VM_C_GOTO(case_double);
		}
	// 如果op1是字串
	} else if (EXPECTED(Z_TYPE_P(op1) == IS_STRING)) {
		// 如果op2是字串
		if (EXPECTED(Z_TYPE_P(op2) == IS_STRING)) {
			// 比较两个字串相等
			bool result = zend_fast_equal_strings(Z_STR_P(op1), Z_STR_P(op2));
			// 释放操作对象的附加变量
			FREE_OP2();
			// 如果是相等
			if (result) {
				// 跳转到指定标签 case_true
				ZEND_VM_C_GOTO(case_true);
			// 否则
			} else {
				// 跳转到指定标签 case_false
				ZEND_VM_C_GOTO(case_false);
			}
		}
	}
	// 调用方法 zend_case_helper
	ZEND_VM_DISPATCH_TO_HELPER(zend_case_helper, op_1, op1, op_2, op2);
}

// ing3, new语句,创建对象
ZEND_VM_HANDLER(68, ZEND_NEW, UNUSED|CLASS_FETCH|CONST|VAR, UNUSED|CACHE_SLOT, NUM)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *result;
	zend_function *constructor;
	zend_class_entry *ce;
	zend_execute_data *call;

	// windows: 无操作
	SAVE_OPLINE();
	// 如果op1是常量
	if (OP1_TYPE == IS_CONST) {
		// 通过偏移量 在运行时缓存中获取一个 (void**) 中的第一个元素
		ce = CACHED_PTR(opline->op2.num);
		// 如果 类不存在
		if (UNEXPECTED(ce == NULL)) {
			// 获取类，找不到会报错。p1:类名，p2:key，p3:flags
			// 访问 (p2).zv。p1:opline,p2:node
			ce = zend_fetch_class_by_name(Z_STR_P(RT_CONSTANT(opline, opline->op1)), Z_STR_P(RT_CONSTANT(opline, opline->op1) + 1), ZEND_FETCH_CLASS_DEFAULT | ZEND_FETCH_CLASS_EXCEPTION);
			// 如果 类不存在
			if (UNEXPECTED(ce == NULL)) {
				// 结果为 未定义
				ZVAL_UNDEF(EX_VAR(opline->result.var));
				// 主要是 return
				HANDLE_EXCEPTION();
			}
			// 通过偏移量 在 EX(run_time_cache) 中获取一个 (void**) 中的第一个元素，并给它赋值
			CACHE_PTR(opline->op2.num, ce);
		}
	// 否则 如果 op1 类型是未定义
	} else if (OP1_TYPE == IS_UNUSED) {
		// 通过类名或类型查找类，p1:类名，p2:类型
		ce = zend_fetch_class(NULL, opline->op1.num);
		// 如果 类不存在
		if (UNEXPECTED(ce == NULL)) {
			// 结果为 未定义
			ZVAL_UNDEF(EX_VAR(opline->result.var));
			// 主要是 return
			HANDLE_EXCEPTION();
		}
	// 否则
	} else {
		// 取出 类入口
		ce = Z_CE_P(EX_VAR(opline->op1.var));
	}

	// 在执行数据中取出 指定序号的变量
	result = EX_VAR(opline->result.var);
	// 初始化对象，传入zval指针和类入口
	if (UNEXPECTED(object_init_ex(result, ce) != SUCCESS)) {
		// 结果为 未定义
		ZVAL_UNDEF(result);
		// 主要是 return
		HANDLE_EXCEPTION();
	}

	// 获取构造方法
	constructor = Z_OBJ_HT_P(result)->get_constructor(Z_OBJ_P(result));
	// 如果没有构造方法
	if (constructor == NULL) {
		if (UNEXPECTED(EG(exception))) {
			// 主要是 return
			HANDLE_EXCEPTION();
		}

		// 如果没有参数，跳过 DO_FCALL 操作码。
		/* If there are no arguments, skip over the DO_FCALL opcode. We check if the next
		 * opcode is DO_FCALL in case EXT instructions are used. */
		if (EXPECTED(opline->extended_value == 0 && (opline+1)->opcode == ZEND_DO_FCALL)) {
			// 偏移到目标操作码并 return ,p1:是否检查异常？使用 EX(opline)  ：使用 opline 。p2:偏移量
			ZEND_VM_NEXT_OPCODE_EX(1, 2);
		}

		// 进行一个假的方法调用， zend_pass_function 是空函数
		/* Perform a dummy function call */
		// ing3, 初始化并返回 堆栈最上一个执行数据，空间不够则扩展大小
		// p1:调用信息，p2:函数，p3:参数数量，p4:对象或调用域
		call = zend_vm_stack_push_call_frame(
			ZEND_CALL_FUNCTION, (zend_function *) &zend_pass_function,
			opline->extended_value, NULL);
	// 否则，获取到了构造方法
	} else {
		// 如果是用户定义函数 并且 没有 运行时缓存
		if (EXPECTED(constructor->type == ZEND_USER_FUNCTION) && UNEXPECTED(!RUN_TIME_CACHE(&constructor->op_array))) {
			// 为op_array创建并 初始化运行时缓存
			init_func_run_time_cache(&constructor->op_array);
		}
		/* We are not handling overloaded classes right now */
		// ing3, 初始化并返回 堆栈最上一个执行数据，空间不够则扩展大小
		// p1:调用信息，p2:函数，p3:参数数量，p4:对象或调用域
		// 调用标记：调用函数，需要释放 $this, 有$this
		call = zend_vm_stack_push_call_frame(
			ZEND_CALL_FUNCTION | ZEND_CALL_RELEASE_THIS | ZEND_CALL_HAS_THIS,
			constructor,
			opline->extended_value,
			Z_OBJ_P(result));
		// 引用次数+1
		Z_ADDREF_P(result);
	}

	// 链接前一个执行数据
	call->prev_execute_data = EX(call);
	// 新执行数据 作为调用数据
	EX(call) = call;
	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, 验证并调用对象的 clone 方法
ZEND_VM_COLD_CONST_HANDLER(110, ZEND_CLONE, CONST|TMPVAR|UNUSED|THIS|CV, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *obj;
	zend_object *zobj;
	zend_class_entry *ce, *scope;
	zend_function *clone;
	// 函数原型
	zend_object_clone_obj_t clone_call;

	// windows: 无操作
	SAVE_OPLINE();
	// 获取zval指针, UNUSED 返回 $this
	obj = GET_OP1_OBJ_ZVAL_PTR_UNDEF(BP_VAR_R);

	// 为了break
	do {
		// 如果op1是常量 或 （ op1不是未使用类型 并且 op1不是对象 ）
		if (OP1_TYPE == IS_CONST ||
		    (OP1_TYPE != IS_UNUSED && UNEXPECTED(Z_TYPE_P(obj) != IS_OBJECT))) {
			// 如果 op1 是普通变量或编译变量 并且 是引用类型
			if ((OP1_TYPE & (IS_VAR|IS_CV)) && Z_ISREF_P(obj)) {
				// 解引用
				obj = Z_REFVAL_P(obj);
				// 如果是对象
				if (EXPECTED(Z_TYPE_P(obj) == IS_OBJECT)) {
					// 跳出
					break;
				}
			}
			// 结果为 未定义
			ZVAL_UNDEF(EX_VAR(opline->result.var));
			// 如果 op1 是 编译变量 并且对象 值为 未定义
			if (OP1_TYPE == IS_CV && UNEXPECTED(Z_TYPE_P(obj) == IS_UNDEF)) {
				// 报错：p1的变量（变量名）未定义, 并返未初始化zval
				ZVAL_UNDEFINED_OP1();
				// 如果有异常
				if (UNEXPECTED(EG(exception) != NULL)) {
					// 主要是 return
					HANDLE_EXCEPTION();
				}
			}
			// 报错：在非对象变量上 调用 __clone方法
			zend_throw_error(NULL, "__clone method called on non-object");
			// 释放操作对象的附加变量
			FREE_OP1();
			// 主要是 return
			HANDLE_EXCEPTION();
		}
	} while (0);
	// 确定了是对象，可以clone

	// 取出对象
	zobj = Z_OBJ_P(obj);
	// 所属类
	ce = zobj->ce;
	// clone 方法
	clone = ce->clone;
	// clone_obj 方法
	clone_call = zobj->handlers->clone_obj;
	// 如果没有 clone_obj
	if (UNEXPECTED(clone_call == NULL)) {
		// 报错：A是无法克隆的对象
		zend_throw_error(NULL, "Trying to clone an uncloneable object of class %s", ZSTR_VAL(ce->name));
		// 释放操作对象的附加变量
		FREE_OP1();
		// 结果为 未定义
		ZVAL_UNDEF(EX_VAR(opline->result.var));
		// 主要是 return
		HANDLE_EXCEPTION();
	}

	// 如果有clone方法 并且 不是public
	if (clone && !(clone->common.fn_flags & ZEND_ACC_PUBLIC)) {
		// 找到当前 域（类）
		scope = EX(func)->op_array.scope;
		// 如果方法不属于当前 类
		if (clone->common.scope != scope) {
			// 如果clone方法是私有的，或者 是检验继承关系失败
			if (UNEXPECTED(clone->common.fn_flags & ZEND_ACC_PRIVATE)
				
			 || UNEXPECTED(!zend_check_protected(zend_get_function_root_class(clone), scope))) {
				// 报错：调用 __clone方法错误
				zend_wrong_clone_call(clone, scope);
				// 释放操作对象的附加变量
				FREE_OP1();
				// 结果为 未定义
				ZVAL_UNDEF(EX_VAR(opline->result.var));
				// 主要是 return
				HANDLE_EXCEPTION();
			}
		}
	}

	// 调用 clone_call，返回结果
	ZVAL_OBJ(EX_VAR(opline->result.var), clone_call(zobj));

	// 释放操作对象的附加变量
	FREE_OP1();
	// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, 获取常量
// , 无业务逻辑， 1/1
// ZEND_FASTCALL ZEND_EXT_NOP_SPEC，1个分支（验证过）
ZEND_VM_HOT_HANDLER(99, ZEND_FETCH_CONSTANT, UNUSED|CONST_FETCH, CONST, CACHE_SLOT)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zend_constant *c;
	// 先尝试在缓存中获取
	// 通过偏移量 在运行时缓存中获取一个 (void**) 中的第一个元素
	c = CACHED_PTR(opline->extended_value);
	// 如果常量有效 并且 不是 CACHE_SPECIAL
	// ((uintptr_t)(ptr)) & CACHE_SPECIAL， CACHE_SPECIAL=1
	if (EXPECTED(c != NULL) && EXPECTED(!IS_SPECIAL_CACHE_VAL(c))) {
		// 把扩展信息的值复制给 结果
		ZVAL_COPY_OR_DUP(EX_VAR(opline->result.var), &c->value);
		// opline + 1，到目标操作码并 return
		ZEND_VM_NEXT_OPCODE();
	}
	// 如果从缓存中获取失败
	
	// windows: 无操作
	SAVE_OPLINE();
	// 快速读取常量。p1:常量名，p2:flags
	// 访问 (p2).zv。p1:opline,p2:node
	zend_quick_get_constant(RT_CONSTANT(opline, opline->op2) + 1, opline->op1.num OPLINE_CC EXECUTE_DATA_CC);
	// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}


// ing3, 获取类常量
ZEND_VM_HANDLER(181, ZEND_FETCH_CLASS_CONSTANT, VAR|CONST|UNUSED|CLASS_FETCH, CONST, CACHE_SLOT)
{
	zend_class_entry *ce, *scope;
	zend_class_constant *c;
	zval *value, *zv;
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// windows: 无操作
	SAVE_OPLINE();
	// 为了break
	do {
		// 如果op1是常量
		if (OP1_TYPE == IS_CONST) {
			// 通过偏移量 在运行时缓存中获取一个 (void**) 中的第一个元素，如果存在
			if (EXPECTED(CACHED_PTR(opline->extended_value + sizeof(void*)))) {
				// 使用这个值
				value = CACHED_PTR(opline->extended_value + sizeof(void*));
				// 跳出
				break;
			// 通过偏移量 在运行时缓存中获取一个 (void**) 中的第一个元素，如果存在
			} else if (EXPECTED(CACHED_PTR(opline->extended_value))) {
				// 使用这个值
				ce = CACHED_PTR(opline->extended_value);
			// 如果没法在缓存中读取到
			} else {
				// 获取类，找不到会报错。p1:类名，p2:key，p3:flags
				// 访问 (p2).zv。p1:opline,p2:node
				ce = zend_fetch_class_by_name(Z_STR_P(RT_CONSTANT(opline, opline->op1)), Z_STR_P(RT_CONSTANT(opline, opline->op1) + 1), ZEND_FETCH_CLASS_DEFAULT | ZEND_FETCH_CLASS_EXCEPTION);
				// 如果 类不存在
				if (UNEXPECTED(ce == NULL)) {
					// 结果为 未定义
					ZVAL_UNDEF(EX_VAR(opline->result.var));
					// 主要是 return
					HANDLE_EXCEPTION();
				}
			}
		// 否则，op1不是常量
		} else {
			// 如果 op1 类型是未定义
			if (OP1_TYPE == IS_UNUSED) {
				// 通过类名或类型查找类，p1:类名，p2:类型
				ce = zend_fetch_class(NULL, opline->op1.num);
				// 如果 类不存在
				if (UNEXPECTED(ce == NULL)) {
					// 结果为 未定义
					ZVAL_UNDEF(EX_VAR(opline->result.var));
					// 主要是 return
					HANDLE_EXCEPTION();
				}
			// 否则
			} else {
				// 取出 类入口
				ce = Z_CE_P(EX_VAR(opline->op1.var));
			}
			// 通过偏移量 在运行时缓存中获取一个 (void**) 中的第一个元素
			if (EXPECTED(CACHED_PTR(opline->extended_value) == ce)) {
				// 通过偏移量 在运行时缓存中获取一个 (void**) 中的第一个元素
				value = CACHED_PTR(opline->extended_value + sizeof(void*));
				// 跳出
				break;
			}
		}

		// 从常量表中查找
		// 访问 (p2).zv。p1:opline,p2:node
		zv = zend_hash_find_known_hash(CE_CONSTANTS_TABLE(ce), Z_STR_P(RT_CONSTANT(opline, opline->op2)));
		// 如果找到了
		if (EXPECTED(zv != NULL)) {
			// 把指向目标取出
			c = Z_PTR_P(zv);
			// 域
			scope = EX(func)->op_array.scope;
			// 如果常量不可访问
			if (!zend_verify_const_access(c, scope)) {
				// 访问 (p2).zv。p1:opline,p2:node
				zend_throw_error(NULL, "Cannot access %s constant %s::%s", zend_visibility_string(ZEND_CLASS_CONST_FLAGS(c)), ZSTR_VAL(ce->name), Z_STRVAL_P(RT_CONSTANT(opline, opline->op2)));
				// 结果为 未定义
				ZVAL_UNDEF(EX_VAR(opline->result.var));
				// 主要是 return
				HANDLE_EXCEPTION();
			}

			// 如果骑过trait访问常量
			if (ce->ce_flags & ZEND_ACC_TRAIT) {
				// 访问 (p2).zv。p1:opline,p2:node
				zend_throw_error(NULL, "Cannot access trait constant %s::%s directly", ZSTR_VAL(ce->name), Z_STRVAL_P(RT_CONSTANT(opline, opline->op2)));
				// 结果为 未定义
				ZVAL_UNDEF(EX_VAR(opline->result.var));
				// 主要是 return
				HANDLE_EXCEPTION();
			}

			// 常量值
			value = &c->value;
			// 枚举类型需要加载所有类常量 来构建备用枚举表
			// Enums require loading of all class constants to build the backed enum table
			// 如果是枚举 并且 有规定类型 并且 是用户定义的枚举类 并且 没有更新过常量
			if (ce->ce_flags & ZEND_ACC_ENUM && ce->enum_backing_type != IS_UNDEF && ce->type == ZEND_USER_CLASS && !(ce->ce_flags & ZEND_ACC_CONSTANTS_UPDATED)) {
				// 如果更新常量失败
				if (UNEXPECTED(zend_update_class_constants(ce) == FAILURE)) {
					// 结果为 未定义
					ZVAL_UNDEF(EX_VAR(opline->result.var));
					// 主要是 return
					HANDLE_EXCEPTION();
				}
			}
			// 如果值是 常量表达式
			if (Z_TYPE_P(value) == IS_CONSTANT_AST) {
				// 更新此常量
				zval_update_constant_ex(value, c->ce);
				// 如果没有异常
				if (UNEXPECTED(EG(exception) != NULL)) {
					// 结果为 未定义
					ZVAL_UNDEF(EX_VAR(opline->result.var));
					// 主要是 return
					HANDLE_EXCEPTION();
				}
			}
			// 找到运行时缓存的 p1 个位置，把p2 和p3依次保存进去
			CACHE_POLYMORPHIC_PTR(opline->extended_value, ce, value);
		// 否则
		} else {
			// 报错：常量未定义
			// 访问 (p2).zv。p1:opline,p2:node
			zend_throw_error(NULL, "Undefined constant %s::%s",
				ZSTR_VAL(ce->name), Z_STRVAL_P(RT_CONSTANT(opline, opline->op2)));
			// 结果为 未定义
			ZVAL_UNDEF(EX_VAR(opline->result.var));
			// 主要是 return
			HANDLE_EXCEPTION();
		}
	} while (0);

	// 值复制给结果 
	ZVAL_COPY_OR_DUP(EX_VAR(opline->result.var), value);

	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, 向数组中添加元素
ZEND_VM_HANDLER(72, ZEND_ADD_ARRAY_ELEMENT, CONST|TMP|VAR|CV, CONST|TMPVAR|UNUSED|NEXT|CV, REF)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *expr_ptr, new_expr;

	// windows: 无操作
	SAVE_OPLINE();
	// 如果 op1 是 （普通变量 或 编译变量 ） 并且 有添加引用元素标记
	if ((OP1_TYPE == IS_VAR || OP1_TYPE == IS_CV) &&
	    UNEXPECTED(opline->extended_value & ZEND_ARRAY_ELEMENT_REF)) {
		// 获取zval指针,TMP/CONST/UNUSED 返回null，不支持 TMPVAR/TMPVARCV
		expr_ptr = GET_OP1_ZVAL_PTR_PTR(BP_VAR_W);
		// 如果表达式是引用类型
		if (Z_ISREF_P(expr_ptr)) {
			// 引用次数+1
			Z_ADDREF_P(expr_ptr);
		// 否则
		} else {
			// 转成引用类型，引用次数为2
			ZVAL_MAKE_REF_EX(expr_ptr, 2);
		}
		// 释放操作对象的附加变量
		FREE_OP1();
	// 否则
	} else {
		// 获取zval指针，UNUSED 类型返回null
		expr_ptr = GET_OP1_ZVAL_PTR(BP_VAR_R);
		// 如果 op1是临时变量
		if (OP1_TYPE == IS_TMP_VAR) {
			/* pass */
		// 否则 如果op1是常量
		} else if (OP1_TYPE == IS_CONST) {
			// 增加引用次数
			Z_TRY_ADDREF_P(expr_ptr);
		// 否则 如果 op1 是 编译变量
		} else if (OP1_TYPE == IS_CV) {
			// 尝试解引用 expr_ptr
			ZVAL_DEREF(expr_ptr);
			// 增加引用次数
			Z_TRY_ADDREF_P(expr_ptr);
		// 否则 如果 op1 是 普通变量
		} else /* if (OP1_TYPE == IS_VAR) */ {
			// 如果 元素式是 引用类型
			if (UNEXPECTED(Z_ISREF_P(expr_ptr))) {
				// 取出 引用实例
				zend_refcounted *ref = Z_COUNTED_P(expr_ptr);
				// 解引用
				expr_ptr = Z_REFVAL_P(expr_ptr);
				// 引用次数-1，如果为0
				if (UNEXPECTED(GC_DELREF(ref) == 0)) {
					// 值复制给新 new_expr
					ZVAL_COPY_VALUE(&new_expr, expr_ptr);
					// 使用新值
					expr_ptr = &new_expr;
					// 释放引用实例
					efree_size(ref, sizeof(zend_reference));
				// 否则， 如果是可以计数类型
				} else if (Z_OPT_REFCOUNTED_P(expr_ptr)) {
					// 引用次数+1
					Z_ADDREF_P(expr_ptr);
				}
			}
		}
	}

	// 如果op2 已使用, 说明有key或哈希值
	if (OP2_TYPE != IS_UNUSED) {
		// 获取zval指针, UNUSED 返回null
		zval *offset = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
		zend_string *str;
		zend_ulong hval;

// 创建跳转标签 add_again
ZEND_VM_C_LABEL(add_again):
		// 如果op2是字串
		if (EXPECTED(Z_TYPE_P(offset) == IS_STRING)) {
			// 取出字串
			str = Z_STR_P(offset);
			// 如果 OP2 不是常量
			if (OP2_TYPE != IS_CONST) {
				// 如果字串全是整数
				if (ZEND_HANDLE_NUMERIC(str, hval)) {
					// 跳转到指定标签 num_index，按数字索引操作
					ZEND_VM_C_GOTO(num_index);
				}
			}
// 创建跳转标签 str_index，按字串索引操作
ZEND_VM_C_LABEL(str_index):
			// 用 key 更新数组元素，哈希值为操作码结果，值为 表达式
			zend_hash_update(Z_ARRVAL_P(EX_VAR(opline->result.var)), str, expr_ptr);
		// 否则 如果op2是整数
		} else if (EXPECTED(Z_TYPE_P(offset) == IS_LONG)) {
			// 取出偏移量整数
			hval = Z_LVAL_P(offset);
// 创建跳转标签 num_index，按数字索引操作
ZEND_VM_C_LABEL(num_index):
			// 用哈希值更新数组元素，哈希值为操作码结果，值为 表达式
			zend_hash_index_update(Z_ARRVAL_P(EX_VAR(opline->result.var)), hval, expr_ptr);
		// 否则 如果 op2 是普通变量或编译变量 并且 op2 是引用类型
		} else if ((OP2_TYPE & (IS_VAR|IS_CV)) && EXPECTED(Z_TYPE_P(offset) == IS_REFERENCE)) {
			// 解引用
			offset = Z_REFVAL_P(offset);
			// 跳转到指定标签 add_again
			ZEND_VM_C_GOTO(add_again);
		// 否则 如果 op2是null
		} else if (Z_TYPE_P(offset) == IS_NULL) {
			// 空字串
			str = ZSTR_EMPTY_ALLOC();
			// 跳转到指定标签 str_index，按字串索引操作
			ZEND_VM_C_GOTO(str_index);
		// 否则 如果 op2是double
		} else if (Z_TYPE_P(offset) == IS_DOUBLE) {
			// 取出小数
			hval = zend_dval_to_lval_safe(Z_DVAL_P(offset));
			// 跳转到指定标签 num_index，按数字索引操作
			ZEND_VM_C_GOTO(num_index);
		// 否则 如果 op2是false
		} else if (Z_TYPE_P(offset) == IS_FALSE) {
			// 值为0
			hval = 0;
			// 跳转到指定标签 num_index，按数字索引操作
			ZEND_VM_C_GOTO(num_index);
		// 否则 如果 op2是true
		} else if (Z_TYPE_P(offset) == IS_TRUE) {
			// 值为1
			hval = 1;
			// 跳转到指定标签 num_index，按数字索引操作
			ZEND_VM_C_GOTO(num_index);
		// 否则 如果 op2是资源类型
		} else if (Z_TYPE_P(offset) == IS_RESOURCE) {
			// 抛出警告：资源作为偏移量，转成了数字（自己的ID）
			zend_use_resource_as_offset(offset);
			// 取出资源编号
			hval = Z_RES_HANDLE_P(offset);
			// 跳转到指定标签 num_index，按数字索引操作
			ZEND_VM_C_GOTO(num_index);
		// 否则 如果 op2 是 编译变量 并且 偏移量 为未定义
		} else if (OP2_TYPE == IS_CV && Z_TYPE_P(offset) == IS_UNDEF) {
			// 报错：p2的变量（变量名）未定义, 并返未初始化zval
			ZVAL_UNDEFINED_OP2();
			// 空字串
			str = ZSTR_EMPTY_ALLOC();
			// 跳转到指定标签 str_index，按字串索引操作
			ZEND_VM_C_GOTO(str_index);
		// 否则，报错
		} else {
			// 报错：偏移量非法
			zend_illegal_offset();
			// 销毁没有引用次数的对象,并没有销毁 zval本身，不放入gc回收周期。
			zval_ptr_dtor_nogc(expr_ptr);
		}
		// 释放操作对象的附加变量
		FREE_OP2();
	// 否则，没有key和哈希值
	} else {
		// 把新值添加到结果中，如果失败
		if (!zend_hash_next_index_insert(Z_ARRVAL_P(EX_VAR(opline->result.var)), expr_ptr)) {
			// 报错：添加元素失败，下一个元素已经存在
			zend_cannot_add_element();
			// 销毁没有引用次数的对象,并没有销毁 zval本身，不放入gc回收周期。
			zval_ptr_dtor_nogc(expr_ptr);
		}
	}
	// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, 数组中添加需要解包的元素
ZEND_VM_HANDLER(147, ZEND_ADD_ARRAY_UNPACK, ANY, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1;
	HashTable *result_ht;

	// windows: 无操作
	SAVE_OPLINE();
	// 获取 op1 的 zval指针，UNUSED 类型返回null
	op1 = GET_OP1_ZVAL_PTR(BP_VAR_R);
	// 取出结果 中的哈希表
	result_ht = Z_ARRVAL_P(EX_VAR(opline->result.var));

// 创建跳转标签 add_unpack_again
ZEND_VM_C_LABEL(add_unpack_again):
	// 如果op1类型是数组 
	if (EXPECTED(Z_TYPE_P(op1) == IS_ARRAY)) {
		// 取出哈希表
		HashTable *ht = Z_ARRVAL_P(op1);
		zval *val;
		zend_string *key;
		// 遍历哈希表
		ZEND_HASH_FOREACH_STR_KEY_VAL(ht, key, val) {
			// 如果是引用类型 并且 引用次数是1
			if (Z_ISREF_P(val) && Z_REFCOUNT_P(val) == 1) {
				// 解引用
				val = Z_REFVAL_P(val);
			}
			// 增加引用次数
			Z_TRY_ADDREF_P(val);
			// 如果有key
			if (key) {
				// 使用key更新元素
				zend_hash_update(result_ht, key, val);
			// 否则，没有key
			} else {
				// 把引用目标作为新值插入到哈希表里，如果失败
				if (!zend_hash_next_index_insert(result_ht, val)) {
					// 报错：添加元素失败，下一个元素已经存在
					zend_cannot_add_element();
					// 销毁没有引用次数的对象,并没有销毁 zval本身，不放入gc回收周期。
					zval_ptr_dtor_nogc(val);
					// 跳出
					break;
				}
			}
		} ZEND_HASH_FOREACH_END();
		
	// op1类型是对象
	} else if (EXPECTED(Z_TYPE_P(op1) == IS_OBJECT)) {
		// op1里取出所属类
		zend_class_entry *ce = Z_OBJCE_P(op1);
		// 
		zend_object_iterator *iter;
		// 如果没有类 或 类没有迭代器
		if (!ce || !ce->get_iterator) {
			// 报错：只有可遍历类型可以被 解包
			zend_type_error("Only arrays and Traversables can be unpacked");
		// 否则
		} else {
			// 取得迭代器，0:不使用引用
			iter = ce->get_iterator(ce, op1, 0);
			// 如果获取迭代器失败
			if (UNEXPECTED(!iter)) {
				// 释放操作对象的附加变量
				FREE_OP1();
				// 如果没有异常
				if (!EG(exception)) {
					// 抛异常：此类型的对象无法创建迭代器
					zend_throw_exception_ex(
						NULL, 0, "Object of type %s did not create an Iterator", ZSTR_VAL(ce->name)
					);
				}
				// 主要是 return
				HANDLE_EXCEPTION();
			}
			// 创建迭代器成功
			
			const zend_object_iterator_funcs *funcs = iter->funcs;
			// 如果有复位方法
			if (funcs->rewind) {
				// 先复位迭代器
				funcs->rewind(iter);
			}

			// 只要元素有效就一直迭代
			for (; funcs->valid(iter) == SUCCESS; ) {
				zval *val;
				// 如果有异常
				if (UNEXPECTED(EG(exception) != NULL)) {
					// 跳出
					break;
				}

				// 取出当前元素
				val = funcs->get_current_data(iter);
				// 如果有异常
				if (UNEXPECTED(EG(exception) != NULL)) {
					// 跳出
					break;
				}

				zval key;
				// 如果有获取key的方法
				if (funcs->get_current_key) {
					// 获取当前key
					funcs->get_current_key(iter, &key);
					// 如果有异常
					if (UNEXPECTED(EG(exception) != NULL)) {
						// 跳出
						break;
					}
					// key不是整数 并且 不是字串
					if (UNEXPECTED(Z_TYPE(key) != IS_LONG && Z_TYPE(key) != IS_STRING)) {
						// 报错：解包过程中，key必须是整数|字串
						zend_throw_error(NULL,
							"Keys must be of type int|string during array unpacking");
						// 清空key
						zval_ptr_dtor(&key);
						// 跳出
						break;
					}
				// 否则
				} else {
					// 结果为 未定义
					ZVAL_UNDEF(&key);
				}
				// 解引用
				ZVAL_DEREF(val);
				// 增加引用次数
				Z_TRY_ADDREF_P(val);

				zend_ulong num_key;
				// 如果key是字串，并且不能转成整数
				if (Z_TYPE(key) == IS_STRING && !ZEND_HANDLE_NUMERIC(Z_STR(key), num_key)) {
					// 更新哈希表里的值
					zend_hash_update(result_ht, Z_STR(key), val);
					// 通过指针销毁 zval（字串）
					zval_ptr_dtor_str(&key);
				// 否则
				} else {
					// 销毁key
					zval_ptr_dtor(&key);
					// 在哈希表中插入新元素.如果失败
					if (!zend_hash_next_index_insert(result_ht, val)) {
						// 报错：添加元素失败，下一个元素已经存在
						zend_cannot_add_element();
						// 销毁没有引用次数的对象,并没有销毁 zval本身，不放入gc回收周期。
						zval_ptr_dtor_nogc(val);
						// 跳出
						break;
					}
				}

				// 迭代器向后移动
				funcs->move_forward(iter);
				// 如果有异常
				if (UNEXPECTED(EG(exception))) {
					// 跳出
					break;
				}
			}
			// 销毁迭代器
			zend_iterator_dtor(iter);
		}
	// op1类型是引用
	} else if (EXPECTED(Z_ISREF_P(op1))) {
		// 解引用
		op1 = Z_REFVAL_P(op1);
		// 跳转到指定标签 add_unpack_again
		ZEND_VM_C_GOTO(add_unpack_again);
	// 否则，其他类型
	} else {
		// 报错：只有可遍历类型可以被 解包
		zend_throw_error(NULL, "Only arrays and Traversables can be unpacked");
	}

	// 释放操作对象的附加变量
	FREE_OP1();
	// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing2 , 初始化数组， 1/1，ARRAY_INIT|REF ？
// ZEND_FASTCALL ZEND_INIT_ARRAY_SPEC，20个分支（验证过）
ZEND_VM_HANDLER(71, ZEND_INIT_ARRAY, CONST|TMP|VAR|CV|UNUSED, CONST|TMPVAR|UNUSED|NEXT|CV, ARRAY_INIT|REF)
{
	zval *array;
	uint32_t size;
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	// 返回值
	// 在执行数据中取出 指定序号的变量
	array = EX_VAR(opline->result.var);
	// 如果 op1 不是 未使用
	if (OP1_TYPE != IS_UNUSED) {
		// 数组大小，在扩展信息里
		size = opline->extended_value >> ZEND_ARRAY_SIZE_SHIFT;
		// 按要求的大小创建新数组，并关联到结果
		ZVAL_ARR(array, zend_new_array(size));
		// 显示初始化成哈希表
		/* Explicitly initialize array as not-packed if flag is set */
		// 如果操作码中要求 初始化成哈希表
		if (opline->extended_value & ZEND_ARRAY_NOT_PACKED) {
			// 把新数组初始化成哈希表
			zend_hash_real_init_mixed(Z_ARRVAL_P(array));
		}
		// 跳转到 ZEND_ADD_ARRAY_ELEMENT, 添加元素
		ZEND_VM_DISPATCH_TO_HANDLER(ZEND_ADD_ARRAY_ELEMENT);
	// 如果 op1 是未使用
	// 否则
	} else {
		// 新建一个数组，关联到返回值
		ZVAL_ARR(array, zend_new_array(0));
		// opline + 1，到目标操作码并 return
		ZEND_VM_NEXT_OPCODE();
	}
}

// ing3, cast函数
// boolval: _IS_BOOL. intval: IS_LONG. floatval,doubleval: IS_DOUBLE. strval: IS_STRING
ZEND_VM_COLD_CONST_HANDLER(51, ZEND_CAST, CONST|TMP|VAR|CV, ANY, TYPE)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *expr;
	// 在执行数据中取出 指定序号的变量
	zval *result = EX_VAR(opline->result.var);
	HashTable *ht;

	// windows: 无操作
	SAVE_OPLINE();
	// 获取zval指针，UNUSED 类型返回null
	expr = GET_OP1_ZVAL_PTR(BP_VAR_R);

	// 根据扩展值操作
	switch (opline->extended_value) {
		// 整数
		case IS_LONG:
			// 直接转成整数
			ZVAL_LONG(result, zval_get_long(expr));
			break;
		// 小数
		case IS_DOUBLE:
			// 直接转成小数
			ZVAL_DOUBLE(result, zval_get_double(expr));
			break;
		// 字串
		case IS_STRING:
			// 直接转成字串
			ZVAL_STR(result, zval_get_string(expr));
			break;
		// 其他
		default:
			// 不可以是 _IS_BOOL
			ZEND_ASSERT(opline->extended_value != _IS_BOOL && "Must use ZEND_BOOL instead");
			// 如果 op1 是普通变量或编译变量
			if (OP1_TYPE & (IS_VAR|IS_CV)) {
				// 解引用
				ZVAL_DEREF(expr);
			}
			// 如果已经是正确的类型，直接返回它
			/* If value is already of correct type, return it directly */
			// 如果类型是要求的类型
			if (Z_TYPE_P(expr) == opline->extended_value) {
				// 复制给结果 
				ZVAL_COPY_VALUE(result, expr);
				// 如果op1是常量
				if (OP1_TYPE == IS_CONST) {
					// 如果是可以计数类型
					// 引用次数+1
					if (UNEXPECTED(Z_OPT_REFCOUNTED_P(result))) Z_ADDREF_P(result);
				// 否则 如果 op1 不是 临时变量
				} else if (OP1_TYPE != IS_TMP_VAR) {
					// 如果是可以计数类型
					// 引用次数+1
					if (Z_OPT_REFCOUNTED_P(result)) Z_ADDREF_P(result);
				}
				// 只释放类型为 IS_VAR 的变量
				FREE_OP1_IF_VAR();
				// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
				ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
			}
			// 如果不是正确的类型

			// 目标类型是数组
			if (opline->extended_value == IS_ARRAY) {
				// 如果op1类型是 常量 或 表达式不是对象 或表达式是闭包
				if (OP1_TYPE == IS_CONST || Z_TYPE_P(expr) != IS_OBJECT || Z_OBJCE_P(expr) == zend_ce_closure) {
					// 如果表达式不是null
					if (Z_TYPE_P(expr) != IS_NULL) {
						// 结果是有1个元素的新数组 
						ZVAL_ARR(result, zend_new_array(1));
						// 把表达式结果添加进来
						expr = zend_hash_index_add_new(Z_ARRVAL_P(result), 0, expr);
						// 如果op1是常量
						if (OP1_TYPE == IS_CONST) {
							// 如果是可以计数类型
							// 引用次数+1
							if (UNEXPECTED(Z_OPT_REFCOUNTED_P(expr))) Z_ADDREF_P(expr);
						// 否则
						} else {
							// 如果是可以计数类型
							// 引用次数+1
							if (Z_OPT_REFCOUNTED_P(expr)) Z_ADDREF_P(expr);
						}
					// 否则，表达式是null
					} else {
						// 结果为空数组
						ZVAL_EMPTY_ARRAY(result);
					}
				// 如果对象 没有属性表 并且 没有获取属性的方法 并且 使用标准属性获取方法
				} else if (Z_OBJ_P(expr)->properties == NULL
				 && Z_OBJ_HT_P(expr)->get_properties_for == NULL
				 && Z_OBJ_HT_P(expr)->get_properties == zend_std_get_properties) {
					// 不需要重建属性哈希表的优化方法
					/* Optimized version without rebuilding properties HashTable */
					// 调用标准方法获取对象的属性表，放进结果里
					ZVAL_ARR(result, zend_std_build_object_properties_array(Z_OBJ_P(expr)));
				// 否则，有自己的属性获取方法
				} else {
					// 调用对象自己的属性获取方法
					HashTable *obj_ht = zend_get_properties_for(expr, ZEND_PROP_PURPOSE_ARRAY_CAST);
					// 如果属性表有效
					if (obj_ht) {
						// 快速复制
						/* fast copy */
						// 取得对象的符号表，放进结果里
						// 把属性表（只包含字串key），转成符号表 （包含数字和非数字key）。p1：哈希表，p2:总是返回副本
						// 必须返回副本的情况：1.有默认属性。2.没有使用标准对象方法。3.有递归引用		
						ZVAL_ARR(result, zend_proptable_to_symtable(obj_ht,
							(Z_OBJCE_P(expr)->default_properties_count ||
							 Z_OBJ_P(expr)->handlers != &std_object_handlers ||
							 GC_IS_RECURSIVE(obj_ht))));
						// 释放临时属性哈希表
						zend_release_properties(obj_ht);
					// 否则，属性表无效
					} else {
						// 结果是空数组 
						ZVAL_EMPTY_ARRAY(result);
					}
				}
			// 否则，目标类型不是数组 
			} else {
				// 目标类型必须是对象
				ZEND_ASSERT(opline->extended_value == IS_OBJECT);
				// 结果为 新建的 标准引用类 实例
				ZVAL_OBJ(result, zend_objects_new(zend_standard_class_def));
				// 如果源值是数组
				if (Z_TYPE_P(expr) == IS_ARRAY) {
					// 取得符号表
					ht = zend_symtable_to_proptable(Z_ARR_P(expr));
					// 如果符号表不可更改
					if (GC_FLAGS(ht) & IS_ARRAY_IMMUTABLE) {
						/* TODO: try not to duplicate immutable arrays as well ??? */
						// 复制符号表
						ht = zend_array_dup(ht);
					}
					// 把符号表作为 新对象的属性表
					Z_OBJ_P(result)->properties = ht;
				// 如果源值不是数组，也不是 null
				} else if (Z_TYPE_P(expr) != IS_NULL) {
					// 创建1个包含1个元素的哈希表作为 新对象的属性表
					Z_OBJ_P(result)->properties = ht = zend_new_array(1);
					// 符号表中添加新元素 key为 scalar
					expr = zend_hash_add_new(ht, ZSTR_KNOWN(ZEND_STR_SCALAR), expr);
					// 如果op1是常量
					if (OP1_TYPE == IS_CONST) {
						// 如果是可以计数类型
						// 引用次数+1
						if (UNEXPECTED(Z_OPT_REFCOUNTED_P(expr))) Z_ADDREF_P(expr);
					// 否则
					} else {
						// 如果是可以计数类型
						// 引用次数+1
						if (Z_OPT_REFCOUNTED_P(expr)) Z_ADDREF_P(expr);
					}
				}
			}
	}

	// 释放操作对象的附加变量
	FREE_OP1();
	// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, include 或 eval 函数
ZEND_VM_HANDLER(73, ZEND_INCLUDE_OR_EVAL, CONST|TMPVAR|CV, ANY, EVAL, SPEC(OBSERVER))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zend_op_array *new_op_array;
	zval *inc_filename;

	// windows: 无操作
	SAVE_OPLINE();
	// 获取zval指针，UNUSED 类型返回null
	inc_filename = GET_OP1_ZVAL_PTR(BP_VAR_R);
	// include 或 eval 函数的执行逻辑。p1:文件名，p2:类型
	new_op_array = zend_include_or_eval(inc_filename, opline->extended_value);
	// 如果有异常
	if (UNEXPECTED(EG(exception) != NULL)) {
		// 释放操作对象的附加变量
		FREE_OP1();
		// 如果操作码不是 -1 并且 不是null
		if (new_op_array != ZEND_FAKE_OP_ARRAY && new_op_array != NULL) {
			// 销毁新生成的操作码
			destroy_op_array(new_op_array);
			// 释放新生成的操作码
			efree_size(new_op_array, sizeof(zend_op_array));
		}
		// 如果 opline 结果类型是 变量或临时变量，把结果 zval 标记为未定义
		UNDEF_RESULT();
		// 主要是 return
		HANDLE_EXCEPTION();
	// 如果返回的是 -1。不需要使用操作码
	} else if (new_op_array == ZEND_FAKE_OP_ARRAY) {
		// 如果 操作码的运算结果有效(不是IS_UNUSED)
		if (RETURN_VALUE_USED(opline)) {
			// 结果为 true
			ZVAL_TRUE(EX_VAR(opline->result.var));
		}
	// 如果返回的是 null
	} else if (UNEXPECTED(new_op_array == NULL)) {
		// 如果 操作码的运算结果有效(不是IS_UNUSED)
		if (RETURN_VALUE_USED(opline)) {
			// 结果为 false
			ZVAL_FALSE(EX_VAR(opline->result.var));
		}
	// 如果操作码只有1行，并且是 return ,并且 op1类型是常量，并且 zend_execute_ex == execute_ex ？
	// 不需要执行
	} else if (new_op_array->last == 1
			&& new_op_array->opcodes[0].opcode == ZEND_RETURN
			&& new_op_array->opcodes[0].op1_type == IS_CONST
			&& EXPECTED(zend_execute_ex == execute_ex)) {
		// 如果 操作码的运算结果有效(不是IS_UNUSED)
		if (RETURN_VALUE_USED(opline)) {
			// 新操作码列表
			const zend_op *op = new_op_array->opcodes;

			// 把op1的常量 复制给结果
			// 访问 (p2).zv。p1:opline,p2:node
			ZVAL_COPY(EX_VAR(opline->result.var), RT_CONSTANT(op, op->op1));
		}
		// 销毁新操作码静态变量表
		zend_destroy_static_vars(new_op_array);
		// 销毁新操作码
		destroy_op_array(new_op_array);
		// 释放新操作码
		efree_size(new_op_array, sizeof(zend_op_array));
	// 否则，需要执行
	} else {
		zval *return_value = NULL;
		zend_execute_data *call;
		// 如果 操作码的运算结果有效(不是IS_UNUSED)
		if (RETURN_VALUE_USED(opline)) {
			// 在执行数据中取出 指定序号的变量
			return_value = EX_VAR(opline->result.var);
		}

		// 域
		new_op_array->scope = EX(func)->op_array.scope;

		// ing3, 初始化并返回 堆栈最上一个执行数据，空间不够则扩展大小
		// p1:调用信息，p2:函数，p3:参数数量，p4:对象或调用域
		// 标记 保持原来的 【有$this】+ 【嵌套调用】+有符号表
		call = zend_vm_stack_push_call_frame(
			(Z_TYPE_INFO(EX(This)) & ZEND_CALL_HAS_THIS) | ZEND_CALL_NESTED_CODE | ZEND_CALL_HAS_SYMBOL_TABLE,
			(zend_function*)new_op_array, 0,
			Z_PTR(EX(This)));

		// 如果 有符号表 标记
		if (EX_CALL_INFO() & ZEND_CALL_HAS_SYMBOL_TABLE) {
			// 使用执行数据的符号表
			call->symbol_table = EX(symbol_table);
		// 否则，没有符号表标记
		} else {
			// 重建符号表
			call->symbol_table = zend_rebuild_symbol_table();
		}

		// 绑定前一个执行数据
		call->prev_execute_data = execute_data;
		// 初始化p1执行数据，p1:执行数据，p2:包含 执行时缓存 的操作码列表，p3:返回值
		i_init_code_execute_data(call, new_op_array, return_value);
		// 开始观察者调用
		// "/ZEND_OBSERVER_FCALL_BEGIN\(\s*(.*)\s*\)/" => isset($extra_spec['OBSERVER']) ?            ($extra_spec['OBSERVER'] == 0 ? "" : "zend_observer_fcall_begin(\\1)")            : "",
		ZEND_OBSERVER_FCALL_BEGIN(call);
		// zend_execute_ex 做什么的？
		if (EXPECTED(zend_execute_ex == execute_ex)) {
			// 释放操作对象的附加变量
			FREE_OP1();
			// ing3, windows return 1;
			ZEND_VM_ENTER();
		// 否则
		} else {
			// 添加调用标记：顶层调用
			ZEND_ADD_CALL_FLAG(call, ZEND_CALL_TOP);
			// 执行此数据
			zend_execute_ex(call);
			// 释放调用框架（执行数据）。p1:执行数据
			zend_vm_stack_free_call_frame(call);
		}
		// 销毁新操作码静态变量表
		zend_destroy_static_vars(new_op_array);
		// 销毁新操作码
		destroy_op_array(new_op_array);
		// 释放新操作码
		efree_size(new_op_array, sizeof(zend_op_array));
		// 如果有异常
		if (UNEXPECTED(EG(exception) != NULL)) {
			// 在执行数据中 重新抛错
			zend_rethrow_exception(execute_data);
			// 释放操作对象的附加变量
			FREE_OP1();
			// 如果 opline 结果类型是 变量或临时变量，把结果 zval 标记为未定义
			UNDEF_RESULT();
			// 主要是 return
			HANDLE_EXCEPTION();
		}
	}
	// 释放操作对象的附加变量
	FREE_OP1();
	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, 删除 编译变量
ZEND_VM_HANDLER(153, ZEND_UNSET_CV, CV, UNUSED)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	// 在执行数据中取出 指定序号的变量
	zval *var = EX_VAR(opline->op1.var);

	// 如果是 可计数类型
	if (Z_REFCOUNTED_P(var)) {
		// 取出计数器
		zend_refcounted *garbage = Z_COUNTED_P(var);

		// 结果为 未定义
		ZVAL_UNDEF(var);
		// windows: 无操作
		SAVE_OPLINE();
		// 引用次数-1，如果为0
		if (!GC_DELREF(garbage)) {
			// 调用每个type对应的销毁函数执行销毁
			rc_dtor_func(garbage);
		// 否则
		} else {
			// 添加到回收队列
			gc_check_possible_root(garbage);
		}
		// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
		ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
	// 否则
	} else {
		// 结果为 未定义
		ZVAL_UNDEF(var);
	}
	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, 在 GLOBAL 或 当前上下文中 删除变量
ZEND_VM_HANDLER(74, ZEND_UNSET_VAR, CONST|TMPVAR|CV, UNUSED, VAR_FETCH)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *varname;
	zend_string *name, *tmp_name;
	HashTable *target_symbol_table;

	// windows: 无操作
	SAVE_OPLINE();

	// op1中存放变量名
	// 获取zval指针, UNUSED 返回null
	varname = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);

	// 如果op1是常量
	if (OP1_TYPE == IS_CONST) {
		// 直接取出变量名
		name = Z_STR_P(varname);
	// 如果变量名是字串
	} else if (EXPECTED(Z_TYPE_P(varname) == IS_STRING)) {
		// 直接取出变量名
		name = Z_STR_P(varname);
		// 临时变量为空
		tmp_name = NULL;
	// 否则
	} else {
		// 如果 op1 是 编译变量 并且变量名为 未定义
		if (OP1_TYPE == IS_CV && UNEXPECTED(Z_TYPE_P(varname) == IS_UNDEF)) {
			// 报错：p1的变量（变量名）未定义, 并返未初始化zval
			varname = ZVAL_UNDEFINED_OP1();
		}
		// 获取临时字串（不抛错）返回
		name = zval_try_get_tmp_string(varname, &tmp_name);
		// 如果没有变量名
		if (UNEXPECTED(!name)) {
			// 释放操作对象的附加变量
			FREE_OP1();
			// 主要是 return
			HANDLE_EXCEPTION();
		}
	}

	// 这里获取的符号表是 global 或当前执行数据 上下文中的变量列表
	// 取得目标符号表。运行时符号表 或 当前执行数据中的符号表
	target_symbol_table = zend_get_target_symbol_table(opline->extended_value EXECUTE_DATA_CC);
	// ** 在符号表中删除 这个元素
	zend_hash_del_ind(target_symbol_table, name);

	// 如果 OP1 不是常量
	if (OP1_TYPE != IS_CONST) {
		// 释放临时字串
		zend_tmp_string_release(tmp_name);
	}
	// 释放操作对象的附加变量
	FREE_OP1();
	// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, 删除静态属性（各种报错）
/* No specialization for op_types (CONST|TMPVAR|CV, UNUSED|CLASS_FETCH|CONST|VAR) */
ZEND_VM_COLD_HANDLER(179, ZEND_UNSET_STATIC_PROP, ANY, ANY, CACHE_SLOT)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *varname;
	zend_string *name, *tmp_name = NULL;
	zend_class_entry *ce;

	// windows: 无操作
	SAVE_OPLINE();

	// 如果类型为 常量
	if (OP2_TYPE == IS_CONST) {
		
		// 从操作码扩展信息中 取出类信息
		// 通过偏移量 在运行时缓存中获取一个 (void**) 中的第一个元素
		ce = CACHED_PTR(opline->extended_value);
		// 如果 没获取到有效信息
		if (UNEXPECTED(ce == NULL)) {
			// 获取类，找不到会报错。p1:类名，p2:key，p3:flags
			// 访问 (p2).zv。p1:opline,p2:node
			ce = zend_fetch_class_by_name(Z_STR_P(RT_CONSTANT(opline, opline->op2)), Z_STR_P(RT_CONSTANT(opline, opline->op2) + 1), ZEND_FETCH_CLASS_DEFAULT | ZEND_FETCH_CLASS_EXCEPTION);
			// 如果 类不存在
			if (UNEXPECTED(ce == NULL)) {
				// 释放操作对象的附加变量
				FREE_OP1();
				// 主要是 return
				HANDLE_EXCEPTION();
			}
			/*CACHE_PTR(opline->extended_value, ce);*/
		}
	// 否则
	// 如果 op2 类型是未定义
	} else if (OP2_TYPE == IS_UNUSED) {
		// 通过类名或类型查找类，p1:类名，p2:类型
		ce = zend_fetch_class(NULL, opline->op2.num);
		// 如果 类不存在
		if (UNEXPECTED(ce == NULL)) {
			// 释放操作对象的附加变量
			FREE_OP1();
			// 主要是 return
			HANDLE_EXCEPTION();
		}
	// 否则
	} else {
		// 取出 类入口
		ce = Z_CE_P(EX_VAR(opline->op2.var));
	}

	// 获取zval指针, UNUSED 返回null
	varname = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 如果op1是常量
	if (OP1_TYPE == IS_CONST) {
		// 取出变量名
		name = Z_STR_P(varname);
	// 如果 变量名是 字串
	} else if (EXPECTED(Z_TYPE_P(varname) == IS_STRING)) {
		// 取出 变量名字串
		name = Z_STR_P(varname);
	// 否则
	} else {
		// 如果 op1 是 编译变量 并且变量名为 未定义
		if (OP1_TYPE == IS_CV && UNEXPECTED(Z_TYPE_P(varname) == IS_UNDEF)) {
			// 报错：p1的变量（变量名）未定义, 并返未初始化zval
			varname = ZVAL_UNDEFINED_OP1();
		}
		// 获取临时字串（不抛错）返回
		name = zval_try_get_tmp_string(varname, &tmp_name);
		// 如果没有名称
		if (UNEXPECTED(!name)) {
			// 释放操作对象的附加变量
			FREE_OP1();
			// 主要是 return
			HANDLE_EXCEPTION();
		}
	}
	// 报错: 静态属性不能删除
	zend_std_unset_static_property(ce, name);
	// 释放临时名称
	zend_tmp_string_release(tmp_name);
	// 释放操作对象的附加变量
	FREE_OP1();
	// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, 各种类型，像数组一样删除元素
ZEND_VM_HANDLER(75, ZEND_UNSET_DIM, VAR|CV, CONST|TMPVAR|CV)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *container;
	zval *offset;
	zend_ulong hval;
	zend_string *key;

	// windows: 无操作
	SAVE_OPLINE();
	// 从op1中取出容器
	// 获取zval指针, TMP/CONST返回null, UNUSED 返回 $this，不支持 TMPVAR/TMPVARCV。CV 直接返回。
	container = GET_OP1_OBJ_ZVAL_PTR_PTR_UNDEF(BP_VAR_UNSET);
	
	// 从op2中取出 offset
	// 获取zval指针, UNUSED 返回null
	offset = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);

	do {
		// 如果 容器 类型是 数组
		if (EXPECTED(Z_TYPE_P(container) == IS_ARRAY)) {
			HashTable *ht;

// 创建跳转标签 unset_dim_array
ZEND_VM_C_LABEL(unset_dim_array):
			// 给 zval 指向的 数组 创建副本，并把指针指向新副本
			SEPARATE_ARRAY(container);
			// 取出数组
			ht = Z_ARRVAL_P(container);
// 创建跳转标签 offset_again
ZEND_VM_C_LABEL(offset_again):
			// 如果偏移量是字串(key)
			if (EXPECTED(Z_TYPE_P(offset) == IS_STRING)) {
				// 取出key
				key = Z_STR_P(offset);
				// 如果 OP2 不是常量
				if (OP2_TYPE != IS_CONST) {
					// 如果key都是整数
					if (ZEND_HANDLE_NUMERIC(key, hval)) {
						// 跳转到指定标签 num_index_dim
						ZEND_VM_C_GOTO(num_index_dim);
					}
				}
// 创建跳转标签 str_index_dim
ZEND_VM_C_LABEL(str_index_dim):
				// 数组不是可以是当前符号表
				ZEND_ASSERT(ht != &EG(symbol_table));
				// 使用key, 从数组里删除元素
				zend_hash_del(ht, key);
			// 如果偏移量是整数
			} else if (EXPECTED(Z_TYPE_P(offset) == IS_LONG)) {
				// 取出哈希值
				hval = Z_LVAL_P(offset);
// 创建跳转标签 num_index_dim
ZEND_VM_C_LABEL(num_index_dim):
				// 使用哈希值，从数组中删除指针元素
				zend_hash_index_del(ht, hval);
			// 否则 如果 op2 是普通变量或编译变量 并且 op2 是引用类型
			} else if ((OP2_TYPE & (IS_VAR|IS_CV)) && EXPECTED(Z_TYPE_P(offset) == IS_REFERENCE)) {
				// 解引用
				offset = Z_REFVAL_P(offset);
				// 跳转到指定标签 offset_again
				ZEND_VM_C_GOTO(offset_again);
			// 偏移量是小数
			} else if (Z_TYPE_P(offset) == IS_DOUBLE) {
				// 取出小数，并转成整数
				hval = zend_dval_to_lval_safe(Z_DVAL_P(offset));
				// 跳转到指定标签 num_index_dim
				ZEND_VM_C_GOTO(num_index_dim);
			// 偏移量是null
			} else if (Z_TYPE_P(offset) == IS_NULL) {
				// 空字串
				key = ZSTR_EMPTY_ALLOC();
				// 跳转到指定标签 str_index_dim
				ZEND_VM_C_GOTO(str_index_dim);
			// 偏移量 false
			} else if (Z_TYPE_P(offset) == IS_FALSE) {
				// 哈希值为0
				hval = 0;
				// 跳转到指定标签 num_index_dim
				ZEND_VM_C_GOTO(num_index_dim);
			// 偏移量 true
			} else if (Z_TYPE_P(offset) == IS_TRUE) {
				// 哈希值为1
				hval = 1;
				// 跳转到指定标签 num_index_dim
				ZEND_VM_C_GOTO(num_index_dim);
			// 偏移量是 资源类型
			} else if (Z_TYPE_P(offset) == IS_RESOURCE) {
				// 抛出警告：资源作为偏移量，转成了数字（自己的ID）
				zend_use_resource_as_offset(offset);
				// 取出资源id
				hval = Z_RES_HANDLE_P(offset);
				// 跳转到指定标签 num_index_dim
				ZEND_VM_C_GOTO(num_index_dim);
			// 否则 如果 op2 是 编译变量 并且 偏移量 为未定义
			} else if (OP2_TYPE == IS_CV && Z_TYPE_P(offset) == IS_UNDEF) {
				// 报错：p2的变量（变量名）未定义, 并返未初始化zval
				ZVAL_UNDEFINED_OP2();
				// key是空字串
				key = ZSTR_EMPTY_ALLOC();
				// 跳转到指定标签 str_index_dim
				ZEND_VM_C_GOTO(str_index_dim);
			// 否则, 无法识别的偏移量
			} else {
				// unset时使用了非法类型的偏移量
				zend_type_error("Illegal offset type in unset");
			}
			break;
		// 如果容器是引用类型
		} else if (Z_ISREF_P(container)) {
			// 解引用
			container = Z_REFVAL_P(container);
			// 如果 容器 类型是 数组
			if (EXPECTED(Z_TYPE_P(container) == IS_ARRAY)) {
				// 跳转到指定标签 unset_dim_array
				ZEND_VM_C_GOTO(unset_dim_array);
			}
		}
		// 如果 op1 是 编译变量 并且 窗口类型 为未定义
		if (OP1_TYPE == IS_CV && UNEXPECTED(Z_TYPE_P(container) == IS_UNDEF)) {
			// 报错：p1的变量（变量名）未定义, 并返未初始化zval
			container = ZVAL_UNDEFINED_OP1();
		}
		// 如果 op2 是 编译变量 并且 偏移量 为未定义
		if (OP2_TYPE == IS_CV && UNEXPECTED(Z_TYPE_P(offset) == IS_UNDEF)) {
			// 报错：p2的变量（变量名）未定义, 并返未初始化zval
			offset = ZVAL_UNDEFINED_OP2();
		}
		// 如果 容器 类型是 对象
		if (EXPECTED(Z_TYPE_P(container) == IS_OBJECT)) {
			// 如果op2是常量 并且 有扩展值
			if (OP2_TYPE == IS_CONST && Z_EXTRA_P(offset) == ZEND_EXTRA_VALUE) {
				// 取得下一个zval
				offset++;
			}
			// 调用对象的 unset_dimension 方法
			Z_OBJ_HT_P(container)->unset_dimension(Z_OBJ_P(container), offset);
		// 否则 如果 容器 类型是 字串
		} else if (UNEXPECTED(Z_TYPE_P(container) == IS_STRING)) {
			// 报错：不可以对给字串 使用偏移量
			zend_throw_error(NULL, "Cannot unset string offsets");
		// 否则 如果 容器 类型 大于 false (true或以上）
		} else if (UNEXPECTED(Z_TYPE_P(container) > IS_FALSE)) {
			// 报错：不可以在对非array的类型使用 偏移量删除
			zend_throw_error(NULL, "Cannot unset offset in a non-array variable");
		// 否则 如果 容器 类型 是false
		} else if (UNEXPECTED(Z_TYPE_P(container) == IS_FALSE)) {
			// 报错：把false转成数组已弃用
			zend_false_to_array_deprecated();
		}
	} while (0);

	// 释放操作对象的附加变量
	FREE_OP2();
	// 释放操作对象的附加变量
	FREE_OP1();
	// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, 删除对象属性
ZEND_VM_HANDLER(76, ZEND_UNSET_OBJ, VAR|UNUSED|THIS|CV, CONST|TMPVAR|CV, CACHE_SLOT)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *container;
	zval *offset;
	zend_string *name, *tmp_name;

	// windows: 无操作
	SAVE_OPLINE();
	// op1的变量是容器
	// 获取zval指针, TMP/CONST返回null, UNUSED 返回 $this，不支持 TMPVAR/TMPVARCV。CV 直接返回。
	container = GET_OP1_OBJ_ZVAL_PTR_PTR_UNDEF(BP_VAR_UNSET);
	// 获取zval指针，UNUSED 类型返回null
	offset = GET_OP2_ZVAL_PTR(BP_VAR_R);

	// 为了break
	do {
		// 如果op1有效 并且 容器不是 对象
		if (OP1_TYPE != IS_UNUSED && UNEXPECTED(Z_TYPE_P(container) != IS_OBJECT)) {
			// 如果是引用类型
			if (Z_ISREF_P(container)) {
				// 解引用
				container = Z_REFVAL_P(container);
				// 如果还不是对象
				if (Z_TYPE_P(container) != IS_OBJECT) {
					// 如果 op1 是 编译变量 并且容器类型为 未定义
					if (OP1_TYPE == IS_CV
					 && UNEXPECTED(Z_TYPE_P(container) == IS_UNDEF)) {
						 // 报错：p1的变量（变量名）未定义, 并返未初始化zval
						ZVAL_UNDEFINED_OP1();
					}
					// 跳过删除
					break;
				}
			// 否则, 不是引用类型
			} else {
				// 没有有效对象，跳过删除
				break;
			}
		}
		// 如果类型为 常量
		if (OP2_TYPE == IS_CONST) {
			// 获取，属性名
			name = Z_STR_P(offset);
		// 否则
		} else {
			// 获取临时字串（不抛错）返回
			name = zval_try_get_tmp_string(offset, &tmp_name);
			// 如果没有属性名
			if (UNEXPECTED(!name)) {
				// 跳过删除
				break;
			}
		}
		// 调用容器的 删除属性方法 
		Z_OBJ_HT_P(container)->unset_property(Z_OBJ_P(container), name, ((OP2_TYPE == IS_CONST) ? CACHE_ADDR(opline->extended_value) : NULL));
		// 如果 OP2 不是常量
		if (OP2_TYPE != IS_CONST) {
			// 释放临时变量
			zend_tmp_string_release(tmp_name);
		}
	} while (0);

	// 释放操作对象的附加变量
	FREE_OP2();
	// 释放操作对象的附加变量
	FREE_OP1();
	// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, 重置只读迭代器
ZEND_VM_HANDLER(77, ZEND_FE_RESET_R, CONST|TMP|VAR|CV, JMP_ADDR)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *array_ptr, *result;

	// windows: 无操作
	SAVE_OPLINE();
	// 获取zval指针, UNUSED 返回null，不支持 TMPVAR/TMPVARCV
	array_ptr = GET_OP1_ZVAL_PTR_DEREF(BP_VAR_R);
	// 如果 op1 是数组
	if (EXPECTED(Z_TYPE_P(array_ptr) == IS_ARRAY)) {
		// 在执行数据中取出 指定序号的变量
		result = EX_VAR(opline->result.var);
		// op1 复制给 结果
		ZVAL_COPY_VALUE(result, array_ptr);
		// 如果 op1 不是临时变量，并且 是可以计数类型
		if (OP1_TYPE != IS_TMP_VAR && Z_OPT_REFCOUNTED_P(result)) {
			// 引用次数+1
			Z_ADDREF_P(array_ptr);
		}
		// 通过指针访问zval的 foreach位置。(*p1).u2.fe_pos
		Z_FE_POS_P(result) = 0;
		// 只释放类型为 IS_VAR 的变量
		FREE_OP1_IF_VAR();
		// opline + 1，到目标操作码并 return
		ZEND_VM_NEXT_OPCODE();
	// op1 不是常量 并且 类型是对象
	} else if (OP1_TYPE != IS_CONST && EXPECTED(Z_TYPE_P(array_ptr) == IS_OBJECT)) {
		// 取出对象
		zend_object *zobj = Z_OBJ_P(array_ptr);
		// 如果没有获取迭代器方法
		if (!zobj->ce->get_iterator) {
			// 属性表
			HashTable *properties = zobj->properties;
			// 如果有属性表
			if (properties) {
				// 如果属性表引用数>1
				if (UNEXPECTED(GC_REFCOUNT(properties) > 1)) {
					// 如果属性表不可更改
					if (EXPECTED(!(GC_FLAGS(properties) & IS_ARRAY_IMMUTABLE))) {
						// 引用次数-1
						GC_DELREF(properties);
					}
					// 给属性表制作副本，使用副本
					properties = zobj->properties = zend_array_dup(properties);
				}
			// 否则，没有属性表
			} else {
				// 调用方法动态获取属性表
				properties = zobj->handlers->get_properties(zobj);
			}

			//
			// 在执行数据中取出 指定序号的变量
			result = EX_VAR(opline->result.var);
			// op1 复制给 结果
			ZVAL_COPY_VALUE(result, array_ptr);
			// 如果 op1 不是 临时变量
			if (OP1_TYPE != IS_TMP_VAR) {
				// 引用次数+1
				Z_ADDREF_P(array_ptr);
			}

			// 如果属性表不是空
			if (zend_hash_num_elements(properties) == 0) {
				// 结果的迭代位置 -1
				// 通过指针访问zval的 迭代索引号 , (*p1).u2.fe_iter_idx
				Z_FE_ITER_P(result) = (uint32_t) -1;
				// 只释放类型为 IS_VAR 的变量
				FREE_OP1_IF_VAR();
				
				// 有异常时使用旧操作码，无异常使用新操作码, p1:新操作码
				// OP_JMP_ADDR: 访问 p2.jmp_addr
				ZEND_VM_JMP(OP_JMP_ADDR(opline, opline->op2));
			}
			// 通过指针访问zval的 迭代索引号 , (*p1).u2.fe_iter_idx
			Z_FE_ITER_P(result) = zend_hash_iterator_add(properties, 0);
			// 只释放类型为 IS_VAR 的变量
			FREE_OP1_IF_VAR();
			// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
			ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
		// 否则，可以获取迭代器
		} else {
			// 重置迭代器，并解除和对象的关联，返回当前位置是否为空。p1:传入类数组对象 zval，p2:是否引用
			bool is_empty = zend_fe_reset_iterator(array_ptr, 0 OPLINE_CC EXECUTE_DATA_CC);

			// 释放操作对象的附加变量
			FREE_OP1();
			// 如果有异常
			if (UNEXPECTED(EG(exception))) {
				// 主要是 return
				HANDLE_EXCEPTION();
			// 如果迭代器无效（没有有效位置）
			} else if (is_empty) {
				// 要检查异常 并 有异常时使用旧操作码，无异常使用新操作码, p1:新操作码，p2:是否检查异常
				// OP_JMP_ADDR: 访问 p2.jmp_addr
				ZEND_VM_JMP_EX(OP_JMP_ADDR(opline, opline->op2), 0);
			// 否则
			} else {
				// opline + 1，到目标操作码并 return
				ZEND_VM_NEXT_OPCODE();
			}
		}
	// 否则，其他类型
	} else {
		// 报错： foreach() 的参数必须是数组或对象
		zend_error(E_WARNING, "foreach() argument must be of type array|object, %s given", zend_zval_type_name(array_ptr));
		// 结果为 未定义
		ZVAL_UNDEF(EX_VAR(opline->result.var));
		// 通过指针访问zval的 迭代索引号 , (*p1).u2.fe_iter_idx
		Z_FE_ITER_P(EX_VAR(opline->result.var)) = (uint32_t)-1;
		// 释放操作对象的附加变量
		FREE_OP1();
		// 有异常时使用旧操作码，无异常使用新操作码, p1:新操作码
		// OP_JMP_ADDR: 访问 p2.jmp_addr
		ZEND_VM_JMP(OP_JMP_ADDR(opline, opline->op2));
	}
}

// ing2, 重置读写迭代
ZEND_VM_COLD_CONST_HANDLER(125, ZEND_FE_RESET_RW, CONST|TMP|VAR|CV, JMP_ADDR)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *array_ptr, *array_ref;

	// windows: 无操作
	SAVE_OPLINE();

	// 如果 op1 是 普通变量 或 编译变量
	if (OP1_TYPE == IS_VAR || OP1_TYPE == IS_CV) {
		// 获取zval指针,TMP/CONST/UNUSED 返回null，不支持 TMPVAR/TMPVARCV
		array_ref = array_ptr = GET_OP1_ZVAL_PTR_PTR(BP_VAR_R);
		// 如果是引用类型
		if (Z_ISREF_P(array_ref)) {
			// 解引用
			array_ptr = Z_REFVAL_P(array_ref);
		}
	// 否则
	} else {
		// 获取zval指针，UNUSED 类型返回null
		array_ref = array_ptr = GET_OP1_ZVAL_PTR(BP_VAR_R);
	}

	// 如果目标是数组
	if (EXPECTED(Z_TYPE_P(array_ptr) == IS_ARRAY)) {
		// 如果 op1 是 普通变量 或 编译变量
		if (OP1_TYPE == IS_VAR || OP1_TYPE == IS_CV) {
			if (array_ptr == array_ref) {
				// 给 zval 创建新的 zend_reference ，从给出的 zval 中复制附加内容
				ZVAL_NEW_REF(array_ref, array_ref);
				// 解引用
				array_ptr = Z_REFVAL_P(array_ref);
			}
			// 引用次数+1
			Z_ADDREF_P(array_ref);
			// 
			ZVAL_COPY_VALUE(EX_VAR(opline->result.var), array_ref);
		// 否则
		} else {
			// 在执行数据中取出 指定序号的变量
			array_ref = EX_VAR(opline->result.var);
			// 给 zval 创建新的 zend_reference ，从给出的 zval 中复制附加内容
			ZVAL_NEW_REF(array_ref, array_ptr);
			// 解引用
			array_ptr = Z_REFVAL_P(array_ref);
		}
		// 如果op1是常量
		if (OP1_TYPE == IS_CONST) {
			// 
			ZVAL_ARR(array_ptr, zend_array_dup(Z_ARRVAL_P(array_ptr)));
		// 否则
		} else {
			SEPARATE_ARRAY(array_ptr);
		}
		// 通过指针访问zval的 迭代索引号 , (*p1).u2.fe_iter_idx
		Z_FE_ITER_P(EX_VAR(opline->result.var)) = zend_hash_iterator_add(Z_ARRVAL_P(array_ptr), 0);
		// 只释放类型为 IS_VAR 的变量
		FREE_OP1_IF_VAR();
		// opline + 1，到目标操作码并 return
		ZEND_VM_NEXT_OPCODE();
	// 如果目标是对象
	} else if (OP1_TYPE != IS_CONST && EXPECTED(Z_TYPE_P(array_ptr) == IS_OBJECT)) {
		// 如果没有获取迭代器的方法
		if (!Z_OBJCE_P(array_ptr)->get_iterator) {
			HashTable *properties;
			// 如果 op1 是 普通变量 或 编译变量
			if (OP1_TYPE == IS_VAR || OP1_TYPE == IS_CV) {
				if (array_ptr == array_ref) {
					// 给 zval 创建新的 zend_reference ，从给出的 zval 中复制附加内容
					ZVAL_NEW_REF(array_ref, array_ref);
					// 解引用
					array_ptr = Z_REFVAL_P(array_ref);
				}
				// 引用次数+1
				Z_ADDREF_P(array_ref);
				// op1 复制给 result
				ZVAL_COPY_VALUE(EX_VAR(opline->result.var), array_ref);
			// 否则
			} else {
				// 在执行数据中取出 指定序号的变量
				array_ptr = EX_VAR(opline->result.var);
				// 
				ZVAL_COPY_VALUE(array_ptr, array_ref);
			}
			// 如果对象有属性表 并且 属性表引用次数>1
			if (Z_OBJ_P(array_ptr)->properties
			 && UNEXPECTED(GC_REFCOUNT(Z_OBJ_P(array_ptr)->properties) > 1)) {
				// 如果属性表不是不可更改
				if (EXPECTED(!(GC_FLAGS(Z_OBJ_P(array_ptr)->properties) & IS_ARRAY_IMMUTABLE))) {
					// 引用次数-1
					GC_DELREF(Z_OBJ_P(array_ptr)->properties);
				}
				// 给属性表创建副本
				Z_OBJ_P(array_ptr)->properties = zend_array_dup(Z_OBJ_P(array_ptr)->properties);
			}

			// 取出属性表
			properties = Z_OBJPROP_P(array_ptr);
			// 如果属性表里没有元素
			if (zend_hash_num_elements(properties) == 0) {
				// 通过指针访问zval的 迭代索引号 , (*p1).u2.fe_iter_idx
				Z_FE_ITER_P(EX_VAR(opline->result.var)) = (uint32_t) -1;
				// 只释放类型为 IS_VAR 的变量
				FREE_OP1_IF_VAR();
				// 有异常时使用旧操作码，无异常使用新操作码, p1:新操作码
				// OP_JMP_ADDR: 访问 p2.jmp_addr
				ZEND_VM_JMP(OP_JMP_ADDR(opline, opline->op2));
			}
			// 通过指针访问zval的 迭代索引号 , (*p1).u2.fe_iter_idx
			Z_FE_ITER_P(EX_VAR(opline->result.var)) = zend_hash_iterator_add(properties, 0);
			// 只释放类型为 IS_VAR 的变量
			FREE_OP1_IF_VAR();
			// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
			ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
		// 否则，有获取迭代器的方法
		} else {
			// 重置迭代器，并解除和对象的关联，返回当前位置是否为空。p1:传入类数组对象 zval，p2:是否引用
			bool is_empty = zend_fe_reset_iterator(array_ptr, 1 OPLINE_CC EXECUTE_DATA_CC);
			// 释放操作对象的附加变量
			FREE_OP1();
			// 如果有异常
			if (UNEXPECTED(EG(exception))) {
				// 主要是 return
				HANDLE_EXCEPTION();
			// 如果迭代器无效
			} else if (is_empty) {
				// 要检查异常 并 有异常时使用旧操作码，无异常使用新操作码, p1:新操作码，p2:是否检查异常
				// OP_JMP_ADDR: 访问 p2.jmp_addr
				ZEND_VM_JMP_EX(OP_JMP_ADDR(opline, opline->op2), 0);
			// 否则
			} else {
				// opline + 1，到目标操作码并 return
				ZEND_VM_NEXT_OPCODE();
			}
		}
	// 否则，不是数组和对象
	} else {
		// 警告：foreach()参数 只能是 数组和对象
		zend_error(E_WARNING, "foreach() argument must be of type array|object, %s given", zend_zval_type_name(array_ptr));
		// 结果为 未定义
		ZVAL_UNDEF(EX_VAR(opline->result.var));
		// 通过指针访问zval的 迭代索引号 , (*p1).u2.fe_iter_idx
		Z_FE_ITER_P(EX_VAR(opline->result.var)) = (uint32_t)-1;
		// 释放操作对象的附加变量
		FREE_OP1();
		// 有异常时使用旧操作码，无异常使用新操作码, p1:新操作码
		// OP_JMP_ADDR: 访问 p2.jmp_addr
		ZEND_VM_JMP(OP_JMP_ADDR(opline, opline->op2));
	}
}

// ing2, 循环助手 
ZEND_VM_HELPER(zend_fe_fetch_object_helper, ANY, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *array;
	zval *value;
	uint32_t value_type;
	HashTable *fe_ht;
	HashPosition pos;
	Bucket *p;
	zend_object_iterator *iter;
	// op1
	// 在执行数据中取出 指定序号的变量
	array = EX_VAR(opline->op1.var);
	// windows: 无操作
	SAVE_OPLINE();

	// array必须是对象
	ZEND_ASSERT(Z_TYPE_P(array) == IS_OBJECT);
	// 获取 zval中的迭代器，加了个判断，类型是否是迭代器
	// 如果没有迭代器
	if ((iter = zend_iterator_unwrap(array)) == NULL) {
		// 简单对象
		/* plain object */
		
		// 通过属性名 zval指针 获取 zend_object 的 属性值
		fe_ht = Z_OBJPROP_P(array);
		// 获取指定 序号 迭代器的指向位置，如果迭代器和哈希表不匹配，更新迭代器
		pos = zend_hash_iterator_pos(Z_FE_ITER_P(array), fe_ht);
		// 找到指定元素
		p = fe_ht->arData + pos;
		// 死循环
		while (1) {
			// 如果位置超过了有效位置
			if (UNEXPECTED(pos >= fe_ht->nNumUsed)) {
				// 到达末尾
				/* reached end of iteration */
				// 跳转到指定标签 fe_fetch_r_exit
				ZEND_VM_C_GOTO(fe_fetch_r_exit);
			}
			// 下一个位置
			pos++;
			// 键值对的值
			value = &p->val;
			// 值的类型
			value_type = Z_TYPE_INFO_P(value);
			// 类型不是 未定义
			if (EXPECTED(value_type != IS_UNDEF)) {
				// 如果类型是 间接引用
				if (UNEXPECTED(value_type == IS_INDIRECT)) {
					// 取得引用目标
					value = Z_INDIRECT_P(value);
					// 目标的类型
					value_type = Z_TYPE_INFO_P(value);
					// 类型不是 未定义 并且 
					// 从属性信息 检验属性是否可访问 p1:对象，p2:属性名，p3:是否是动态
					if (EXPECTED(value_type != IS_UNDEF)
					 && EXPECTED(zend_check_property_access(Z_OBJ_P(array), p->key, 0) == SUCCESS)) {
						// 找到了有效元素 跳出
						break;
					}
				// 如果不是间接引用 
				// 如果没有属性 或 属性名字无效 或 
				// 从属性信息 检验属性是否可访问 p1:对象，p2:属性名，p3:是否是动态
				} else if (EXPECTED(Z_OBJCE_P(array)->default_properties_count == 0)
						|| !p->key
						|| zend_check_property_access(Z_OBJ_P(array), p->key, 1) == SUCCESS) {
					// 无法查找，或找到有效元素， 跳出		
					break;
				}
			}
			// 下一个键值对
			p++;
		}
		// 更新迭代器的位置
		// 通过指针访问zval的 迭代索引号 , (*p1).u2.fe_iter_idx
		EG(ht_iterators)[Z_FE_ITER_P(array)].pos = pos;
		// 如果 操作码的运算结果有效(不是IS_UNUSED)
		if (RETURN_VALUE_USED(opline)) {
			// 如果没有key
			if (UNEXPECTED(!p->key)) {
				// 结果为 bucket 的哈希值
				ZVAL_LONG(EX_VAR(opline->result.var), p->h);
			//  如果key有效
			} else if (ZSTR_VAL(p->key)[0]) {
				// key复制给结果 
				ZVAL_STR_COPY(EX_VAR(opline->result.var), p->key);
			// 否则
			} else {
				const char *class_name, *prop_name;
				size_t prop_name_len;
				// 拼接类名和方法名
				zend_unmangle_property_name_ex(
					p->key, &class_name, &prop_name, &prop_name_len);
				// 属性名放到结果里
				ZVAL_STRINGL(EX_VAR(opline->result.var), prop_name, prop_name_len);
			}
		}
	// 否则，有迭代器
	} else {
		// 方法列表
		const zend_object_iterator_funcs *funcs = iter->funcs;
		
		// 如果 迭代位置 ？？？
		if (EXPECTED(++iter->index > 0)) {
			// 如果index再次变成0 ，这里会进入死循环。加一个标记来防止这种情况发生。
			/* This could cause an endless loop if index becomes zero again.
			 * In case that ever happens we need an additional flag. */
			// 向后迭代
			funcs->move_forward(iter);
			// 如果有异常
			if (UNEXPECTED(EG(exception) != NULL)) {
				// 如果 opline 结果类型是 变量或临时变量，把结果 zval 标记为未定义
				UNDEF_RESULT();
				// 主要是 return
				HANDLE_EXCEPTION();
			}
			// 如果当前位置无效
			if (UNEXPECTED(funcs->valid(iter) == FAILURE)) {
				// 到达迭代结尾
				/* reached end of iteration */
				// 如果有异常
				if (UNEXPECTED(EG(exception) != NULL)) {
					// 如果 opline 结果类型是 变量或临时变量，把结果 zval 标记为未定义
					UNDEF_RESULT();
					// 主要是 return
					HANDLE_EXCEPTION();
				}
// 创建跳转标签 fe_fetch_r_exit
ZEND_VM_C_LABEL(fe_fetch_r_exit):
				// 找到目标操作码，设置成当前操作码并返回。 EX(opline) = (zend_op*)((char*)p1 + p2)
				ZEND_VM_SET_RELATIVE_OPCODE(opline, opline->extended_value);
				// windows: return  0
				ZEND_VM_CONTINUE();
			}
		}
		// 取得当前 值
		value = funcs->get_current_data(iter);
		// 如果有异常
		if (UNEXPECTED(EG(exception) != NULL)) {
			// 如果 opline 结果类型是 变量或临时变量，把结果 zval 标记为未定义
			UNDEF_RESULT();
			// 主要是 return
			HANDLE_EXCEPTION();
		}
		// 如果没获取 到值
		if (!value) {
			/* failure in get_current_data */
			// 跳转到指定标签 fe_fetch_r_exit
			ZEND_VM_C_GOTO(fe_fetch_r_exit);
		}
		// 如果 操作码的运算结果有效(不是IS_UNUSED)
		if (RETURN_VALUE_USED(opline)) {
			// 如果有获取key的方法
			if (funcs->get_current_key) {
				// 取得当前key放到result里
				funcs->get_current_key(iter, EX_VAR(opline->result.var));
				// 如果有异常
				if (UNEXPECTED(EG(exception) != NULL)) {
					// 如果 opline 结果类型是 变量或临时变量，把结果 zval 标记为未定义
					UNDEF_RESULT();
					// 主要是 return
					HANDLE_EXCEPTION();
				}
			// 否则
			} else {
				// 写入结果
				// 迭代器指向 fe_reset/fe_fetch 的私有指针（整数）
				ZVAL_LONG(EX_VAR(opline->result.var), iter->index);
			}
		}
		// 值的类型
		value_type = Z_TYPE_INFO_P(value);
	}

	// 如果 op2 是 编译变量
	if (EXPECTED(OP2_TYPE == IS_CV)) {
		// 在执行数据中取出 指定序号的变量
		zval *variable_ptr = EX_VAR(opline->op2.var);
		// 给变量赋值，p1:变量，p2:值，p3:值的变量类型（常量，变量，编译变量），p4:是否严格类型
		// 某执行上下文所属函数 的参数是否使用严格类型限制
		zend_assign_to_variable(variable_ptr, value, IS_CV, EX_USES_STRICT_TYPES());
	// 否则
	} else {
		// 在执行数据中取出 指定序号的变量
		zval *res = EX_VAR(opline->op2.var);
		// 取出计数器
		zend_refcounted *gc = Z_COUNTED_P(value);

		// ？？？
		ZVAL_COPY_VALUE_EX(res, value, gc, value_type);
		// 是否是可以计引用数的类型，如果可计数
		if (Z_TYPE_INFO_REFCOUNTED(value_type)) {
			// 增加引用次数
			GC_ADDREF(gc);
		}
	}
	// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

ZEND_VM_HOT_HANDLER(78, ZEND_FE_FETCH_R, VAR, ANY, JMP_ADDR)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *array;
	zval *value;
	uint32_t value_type;
	HashTable *fe_ht;
	HashPosition pos;

	// 在执行数据中取出 指定序号的变量
	array = EX_VAR(opline->op1.var);
	// 如果 op1 不是数组 
	if (UNEXPECTED(Z_TYPE_P(array) != IS_ARRAY)) {
		// 调用方法 zend_fe_fetch_object_helper
		ZEND_VM_DISPATCH_TO_HELPER(zend_fe_fetch_object_helper);
	}
	// 取得正在遍历的哈希表
	fe_ht = Z_ARRVAL_P(array);
	// 哈希表当前位置
	// 通过指针访问zval的 foreach位置。(*p1).u2.fe_pos
	pos = Z_FE_POS_P(array);
	// 如果是顺序数组
	if (HT_IS_PACKED(fe_ht)) {
		// 取得指向的元素
		value = fe_ht->arPacked + pos;
		// 向后遍历到底
		while (1) {
			// 如果位置超过有效位置
			if (UNEXPECTED(pos >= fe_ht->nNumUsed)) {
				// 迭代到达结尾
				/* reached end of iteration */
				// 找到目标操作码，设置成当前操作码并返回。 EX(opline) = (zend_op*)((char*)p1 + p2)
				ZEND_VM_SET_RELATIVE_OPCODE(opline, opline->extended_value);
				// windows: return  0
				ZEND_VM_CONTINUE();
			}
			// 取得类型
			value_type = Z_TYPE_INFO_P(value);
			// 类型不可以是间接引用
			ZEND_ASSERT(value_type != IS_INDIRECT);
			// 类型不是 未定义
			if (EXPECTED(value_type != IS_UNDEF)) {
				// 跳出循环
				break;
			}
			// 位置后移
			pos++;
			// 元素指针后移
			value++;
		}
		// 指针指向下一个元素
		// 通过指针访问zval的 foreach位置。(*p1).u2.fe_pos
		Z_FE_POS_P(array) = pos + 1;
		// 如果 操作码的运算结果有效(不是IS_UNUSED)
		if (RETURN_VALUE_USED(opline)) {
			// 返回值为整数
			ZVAL_LONG(EX_VAR(opline->result.var), pos);
		}
	// 否则
	} else {
		Bucket *p;
		// 当前 bucket
		p = fe_ht->arData + pos;
		// 向后遍历所有元素
		while (1) {
			// 如果已经到达结尾
			if (UNEXPECTED(pos >= fe_ht->nNumUsed)) {
				// 到达迭代结尾
				/* reached end of iteration */
				// 找到目标操作码，设置成当前操作码并返回。 EX(opline) = (zend_op*)((char*)p1 + p2)
				ZEND_VM_SET_RELATIVE_OPCODE(opline, opline->extended_value);
				// windows: return  0
				ZEND_VM_CONTINUE();
			}
			// 位置后移
			pos++;
			// 值
			value = &p->val;
			// 值的类型
			value_type = Z_TYPE_INFO_P(value);
			// 值不可以是间接引用类型
			ZEND_ASSERT(value_type != IS_INDIRECT);
			// 类型不是 未定义
			if (EXPECTED(value_type != IS_UNDEF)) {
				// 跳出
				break;
			}
			// 下一个bucket
			p++;
		}
		// 通过指针访问zval的 foreach位置。(*p1).u2.fe_pos
		Z_FE_POS_P(array) = pos;
		// 如果 操作码的运算结果有效(不是IS_UNUSED)
		if (RETURN_VALUE_USED(opline)) {
			if (!p->key) {
				ZVAL_LONG(EX_VAR(opline->result.var), p->h);
			// 否则
			} else {
				ZVAL_STR_COPY(EX_VAR(opline->result.var), p->key);
			}
		}
	}
	// 如果 op2 是 编译变量
	if (EXPECTED(OP2_TYPE == IS_CV)) {
		// 在执行数据中取出 指定序号的变量
		zval *variable_ptr = EX_VAR(opline->op2.var);
		// windows: 无操作
		SAVE_OPLINE();
		// 给变量赋值，p1:变量，p2:值，p3:值的变量类型（常量，变量，编译变量），p4:是否严格类型
		// 某执行上下文所属函数 的参数是否使用严格类型限制
		zend_assign_to_variable(variable_ptr, value, IS_CV, EX_USES_STRICT_TYPES());
		// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
		ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
	// 否则
	} else {
		// 在执行数据中取出 指定序号的变量
		zval *res = EX_VAR(opline->op2.var);
		// 
		zend_refcounted *gc = Z_COUNTED_P(value);
		// 
		ZVAL_COPY_VALUE_EX(res, value, gc, value_type);
		// 
		if (Z_TYPE_INFO_REFCOUNTED(value_type)) {
			// 增加引用次数
			GC_ADDREF(gc);
		}
		// opline + 1，到目标操作码并 return
		ZEND_VM_NEXT_OPCODE();
	}
}

// ing2, 读写方式遍历 数组
ZEND_VM_HANDLER(126, ZEND_FE_FETCH_RW, VAR, ANY, JMP_ADDR)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *array;
	zval *value;
	uint32_t value_type;
	HashTable *fe_ht;
	HashPosition pos;
	Bucket *p;

	// 在执行数据中取出 指定序号的变量
	array = EX_VAR(opline->op1.var);
	// windows: 无操作
	SAVE_OPLINE();
	// 减少引用次数
	ZVAL_DEREF(array);
	// 如果op1是数组
	if (EXPECTED(Z_TYPE_P(array) == IS_ARRAY)) {
		// 通过指针访问zval的 迭代索引号 , (*p1).u2.fe_iter_idx
		pos = zend_hash_iterator_pos_ex(Z_FE_ITER_P(EX_VAR(opline->op1.var)), array);
		// 迭代的哈希表
		fe_ht = Z_ARRVAL_P(array);
		// 如果是顺序数组
		if (HT_IS_PACKED(fe_ht)) {
			// 找到元素位置
			value = fe_ht->arPacked + pos;
			// 死循环
			while (1) {
				// 如果位置超出可用范围
				if (UNEXPECTED(pos >= fe_ht->nNumUsed)) {
					// 到达迭代末尾
					/* reached end of iteration */
					// 跳转到指定标签 fe_fetch_w_exit
					ZEND_VM_C_GOTO(fe_fetch_w_exit);
				}
				// 值的类型
				value_type = Z_TYPE_INFO_P(value);
				// 不可以是间接引用
				ZEND_ASSERT(value_type != IS_INDIRECT);
				// 类型不是 未定义
				if (EXPECTED(value_type != IS_UNDEF)) {
					// 跳出出
					break;
				}
				// 下一个位置
				pos++;
				// 下一个元素
				value++;
			}
			// 通过指针访问zval的 迭代索引号 , (*p1).u2.fe_iter_idx
			EG(ht_iterators)[Z_FE_ITER_P(EX_VAR(opline->op1.var))].pos = pos + 1;
			// 如果 操作码的运算结果有效(不是IS_UNUSED)
			if (RETURN_VALUE_USED(opline)) {
				// 把位置，用整数格式放到结果中
				ZVAL_LONG(EX_VAR(opline->result.var), pos);
			}
		// 否则，是哈希表
		} else {
			// 元素位置
			p = fe_ht->arData + pos;
			// 死循环
			while (1) {
				// 如果位置超出可用范围
				if (UNEXPECTED(pos >= fe_ht->nNumUsed)) {
					// 到达迭代末尾
					/* reached end of iteration */
					// 跳转到指定标签 fe_fetch_w_exit
					ZEND_VM_C_GOTO(fe_fetch_w_exit);
				}
				// 下一个位置
				pos++;
				// 元素值
				value = &p->val;
				// 类型
				value_type = Z_TYPE_INFO_P(value);
				// 类型不可以是间接引用
				ZEND_ASSERT(value_type != IS_INDIRECT);
				// 类型不是 未定义
				if (EXPECTED(value_type != IS_UNDEF)) {
					// 跳出
					break;
				}
				// 下一个元素
				p++;
			}
			// 通过指针访问zval的 迭代索引号 , (*p1).u2.fe_iter_idx
			EG(ht_iterators)[Z_FE_ITER_P(EX_VAR(opline->op1.var))].pos = pos;
			// 如果 操作码的运算结果有效(不是IS_UNUSED)
			if (RETURN_VALUE_USED(opline)) {
				// 如果没有key
				if (!p->key) {
					// 哈希值当成结果
					ZVAL_LONG(EX_VAR(opline->result.var), p->h);
				// 否则，有key
				} else {
					// key复制到结果里
					ZVAL_STR_COPY(EX_VAR(opline->result.var), p->key);
				}
			}
		}
	// 如果类型是对象
	} else if (EXPECTED(Z_TYPE_P(array) == IS_OBJECT)) {
		zend_object_iterator *iter;

		// 获取 zval中的迭代器，加了个判断，类型是否是迭代器
		// 如果获取失败
		if ((iter = zend_iterator_unwrap(array)) == NULL) {
			// 简单对象
			/* plain object */

			// 取出属性表
			fe_ht = Z_OBJPROP_P(array);
			// 通过指针访问zval的 迭代索引号 , (*p1).u2.fe_iter_idx
			pos = zend_hash_iterator_pos(Z_FE_ITER_P(EX_VAR(opline->op1.var)), fe_ht);
			// 找到元素
			p = fe_ht->arData + pos;
			// 死循环
			while (1) {
				// 如果位置无效
				if (UNEXPECTED(pos >= fe_ht->nNumUsed)) {
					// 到达迭代末尾
					/* reached end of iteration */
					// 跳转到指定标签 fe_fetch_w_exit
					ZEND_VM_C_GOTO(fe_fetch_w_exit);
				}
				// 下一个位置
				pos++;
				// 值
				value = &p->val;
				// 值的类型
				value_type = Z_TYPE_INFO_P(value);
				// 类型不是 未定义
				if (EXPECTED(value_type != IS_UNDEF)) {
					// 如果值类型是间接引用
					if (UNEXPECTED(value_type == IS_INDIRECT)) {
						// 解间接引用
						value = Z_INDIRECT_P(value);
						// 值的类型
						value_type = Z_TYPE_INFO_P(value);
						// 类型不是 未定义 并且 可访问
						if (EXPECTED(value_type != IS_UNDEF)
						 && EXPECTED(zend_check_property_access(Z_OBJ_P(array), p->key, 0) == SUCCESS)) {
							// 如果类型不是引用
							if ((value_type & Z_TYPE_MASK) != IS_REFERENCE) {
								// 通过slot（它也是属性）指针计算slot的序号，返回此序号对应的属性信息
								zend_property_info *prop_info =
									zend_get_property_info_for_slot(Z_OBJ_P(array), value);
								// 如果有属性信息
								if (UNEXPECTED(prop_info)) {
									// 如果属性是 只读的
									if (UNEXPECTED(prop_info->flags & ZEND_ACC_READONLY)) {
										// 抛异常：不可以访问只读属性的 引用 （测试过，访问时前面不能加&）
										zend_throw_error(NULL,
											"Cannot acquire reference to readonly property %s::$%s",
											ZSTR_VAL(prop_info->ce->name), ZSTR_VAL(p->key));
										// 如果 opline 结果类型是 变量或临时变量，把结果 zval 标记为未定义
										UNDEF_RESULT();
										// 主要是 return
										HANDLE_EXCEPTION();
									}
									// 如果类型存在
									if (ZEND_TYPE_IS_SET(prop_info->type)) {
										// 给 zval 创建新的 zend_reference ，从给出的 zval 中复制附加内容
										ZVAL_NEW_REF(value, value);
										// 向zend_property_info_source_list指向的 zend_property_info 列表中添加 zend_property_info
										ZEND_REF_ADD_TYPE_SOURCE(Z_REF_P(value), prop_info);
										// 类型为引用
										value_type = IS_REFERENCE_EX;
									}
								}
							}
							// 跳出
							break;
						}
					// 值的类型不是间接引用 并且 （默认属性数是0 或 没有key 或 可以通过key访问）
					} else if (EXPECTED(Z_OBJCE_P(array)->default_properties_count == 0)
							|| !p->key
							|| zend_check_property_access(Z_OBJ_P(array), p->key, 1) == SUCCESS) {
						// 跳出
						break;
					}
				}
				// 下一个元素
				p++;
			}
			// 通过指针访问zval的 迭代索引号 , (*p1).u2.fe_iter_idx
			EG(ht_iterators)[Z_FE_ITER_P(EX_VAR(opline->op1.var))].pos = pos;
			// 如果 操作码的运算结果有效(不是IS_UNUSED)
			if (RETURN_VALUE_USED(opline)) {
				// 如果没有key
				if (UNEXPECTED(!p->key)) {
					// 结果为哈希值
					ZVAL_LONG(EX_VAR(opline->result.var), p->h);
				// 如果key有效
				} else if (ZSTR_VAL(p->key)[0]) {
					// key 复制给 结果
					ZVAL_STR_COPY(EX_VAR(opline->result.var), p->key);
				// 否则
				} else {
					const char *class_name, *prop_name;
					size_t prop_name_len;
					// 拼接 类名，属性名
					zend_unmangle_property_name_ex(
						p->key, &class_name, &prop_name, &prop_name_len);
					// 属性名放到结果里
					ZVAL_STRINGL(EX_VAR(opline->result.var), prop_name, prop_name_len);
				}
			}
		// 否则
		} else {
			// 迭代器方法列表
			const zend_object_iterator_funcs *funcs = iter->funcs;
			// 如果迭代器有 
			// 指向 fe_reset/fe_fetch 的私有指针（整数）
			if (++iter->index > 0) {
				/* This could cause an endless loop if index becomes zero again.
				 * In case that ever happens we need an additional flag. */
				// 向后迭代
				funcs->move_forward(iter);
				// 如果有异常
				if (UNEXPECTED(EG(exception) != NULL)) {
					// 如果 opline 结果类型是 变量或临时变量，把结果 zval 标记为未定义
					UNDEF_RESULT();
					// 主要是 return
					HANDLE_EXCEPTION();
				}
				// 如果当前位置无效
				if (UNEXPECTED(funcs->valid(iter) == FAILURE)) {
					// 到达迭代末尾
					/* reached end of iteration */
					// 如果有异常
					if (UNEXPECTED(EG(exception) != NULL)) {
						// 如果 opline 结果类型是 变量或临时变量，把结果 zval 标记为未定义
						UNDEF_RESULT();
						// 主要是 return
						HANDLE_EXCEPTION();
					}
					// 跳转到指定标签 fe_fetch_w_exit
					ZEND_VM_C_GOTO(fe_fetch_w_exit);
				}
			}
			// 取出当前值
			value = funcs->get_current_data(iter);
			// 如果有异常
			if (UNEXPECTED(EG(exception) != NULL)) {
				// 如果 opline 结果类型是 变量或临时变量，把结果 zval 标记为未定义
				UNDEF_RESULT();
				// 主要是 return
				HANDLE_EXCEPTION();
			}
			// 如果没有当前值
			if (!value) {
				// get_current_data 时出错了
				/* failure in get_current_data */
				// 跳转到指定标签 fe_fetch_w_exit
				ZEND_VM_C_GOTO(fe_fetch_w_exit);
			}
			// 如果 操作码的运算结果有效(不是IS_UNUSED)
			if (RETURN_VALUE_USED(opline)) {
				// 如果有获取当前key的方法(函数原型）
				if (funcs->get_current_key) {
					// 获取当前key
					funcs->get_current_key(iter, EX_VAR(opline->result.var));
					// 如果有异常
					if (UNEXPECTED(EG(exception) != NULL)) {
						// 如果 opline 结果类型是 变量或临时变量，把结果 zval 标记为未定义
						UNDEF_RESULT();
						// 主要是 return
						HANDLE_EXCEPTION();
					}
				// 否则
				} else {
					// 读取 指向 fe_reset/fe_fetch 的私有指针（整数）， 复制给结果里
					ZVAL_LONG(EX_VAR(opline->result.var), iter->index);
				}
			}
			// 值的类型
			value_type = Z_TYPE_INFO_P(value);
		}
	// 否则，op1不是对象
	} else {
		// 警告：foreach()的参数必须是数组或对象
		zend_error(E_WARNING, "foreach() argument must be of type array|object, %s given", zend_zval_type_name(array));
		// 如果有异常
		if (UNEXPECTED(EG(exception))) {
			// 如果 opline 结果类型是 变量或临时变量，把结果 zval 标记为未定义
			UNDEF_RESULT();
			// 主要是 return
			HANDLE_EXCEPTION();
		}
// 创建跳转标签 fe_fetch_w_exit
ZEND_VM_C_LABEL(fe_fetch_w_exit):
		// 找到目标操作码，设置成当前操作码并返回。 EX(opline) = (zend_op*)((char*)p1 + p2)
		ZEND_VM_SET_RELATIVE_OPCODE(opline, opline->extended_value);
		// windows: return  0
		ZEND_VM_CONTINUE();
	}

	// 如果值 不是引用类型
	if (EXPECTED((value_type & Z_TYPE_MASK) != IS_REFERENCE)) {
		// 取出 gc对象
		zend_refcounted *gc = Z_COUNTED_P(value);
		// 引用实例
		zval *ref;
		// 给 zval 创建新的空的 zend_reference  
		ZVAL_NEW_EMPTY_REF(value);
		// 解引用
		ref = Z_REFVAL_P(value);
		// ??
		ZVAL_COPY_VALUE_EX(ref, value, gc, value_type);
	}
	// 如果 op2 是 编译变量
	if (EXPECTED(OP2_TYPE == IS_CV)) {
		// 在执行数据中取出 指定序号的变量
		zval *variable_ptr = EX_VAR(opline->op2.var);
		// op2的zval 和 value不是同一个
		if (EXPECTED(variable_ptr != value)) {
			zend_reference *ref;
			// 取出 ref实例
			ref = Z_REF_P(value);
			// 增加引用次数
			GC_ADDREF(ref);
			// 清空  variable_ptr
			i_zval_ptr_dtor(variable_ptr);
			// 把 ref 放回到 op2
			ZVAL_REF(variable_ptr, ref);
		}
	// 否则
	} else {
		// 引用次数+1
		Z_ADDREF_P(value);
		// value的引用目标，引用赋值给op2
		ZVAL_REF(EX_VAR(opline->op2.var), Z_REF_P(value));
	}
	// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, isset/empty函数
ZEND_VM_HOT_HANDLER(154, ZEND_ISSET_ISEMPTY_CV, CV, UNUSED, ISSET, SPEC(ISSET))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *value;
	// 在执行数据中取出 指定序号的变量
	value = EX_VAR(opline->op1.var);
	// 如果没有 ZEND_ISEMPTY。说明在调用isset方法
	if (!(opline->extended_value & ZEND_ISEMPTY)) {
		// 为什么引用目标不需要 > IS_NULL 呢？
		// 如果值大于null 并且 （不是引用类型 或 引用目标不是 null）
		if (Z_TYPE_P(value) > IS_NULL &&
		    (!Z_ISREF_P(value) || Z_TYPE_P(Z_REFVAL_P(value)) != IS_NULL)) {
			// 根据当前操作码结果选择下一个操作码。如果不是跳转类型，结果赋值为true。
			ZEND_VM_SMART_BRANCH_TRUE();
		// 否则
		} else {
			// 根据当前操作码结果选择下一个操作码。如果不是跳转类型，结果赋值为false。
			ZEND_VM_SMART_BRANCH_FALSE();
		}
	// 否则，empty方法
	} else {
		bool result;

		// windows: 无操作
		SAVE_OPLINE();
		// empty函数要把结果取反
		result = !i_zend_is_true(value);
		// 根据当前操作码结果选择下一个操作码，p1:传入值，主要用来做判断。p2:是否检查异常
		ZEND_VM_SMART_BRANCH(result, 1);
	}
}

// ing3, 在 GLOBAL 或 当前上下文中判断变量 isset 或 empty
ZEND_VM_HANDLER(114, ZEND_ISSET_ISEMPTY_VAR, CONST|TMPVAR|CV, UNUSED, VAR_FETCH|ISSET)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *value;
	// 
	/* Should be bool result? as below got: result = (opline->extended_value & ZEND_ISEMPTY) */
	int result;
	zval *varname;
	zend_string *name, *tmp_name;
	HashTable *target_symbol_table;

	SAVE_OPLINE();
	// 获取zval指针，UNUSED 类型返回null
	varname = GET_OP1_ZVAL_PTR(BP_VAR_IS);
	// 如果op1是常量
	if (OP1_TYPE == IS_CONST) {
		// 变量名
		name = Z_STR_P(varname);
	// 否则，op1不是常量
	} else {
		// 取出 变量名
		name = zval_get_tmp_string(varname, &tmp_name);
	}

	// 这里获取的符号表是 global 或当前执行数据 上下文中的变量列表
	// 取得目标符号表。运行时符号表 或 当前执行数据中的符号表
	target_symbol_table = zend_get_target_symbol_table(opline->extended_value EXECUTE_DATA_CC);
	// 在符号表中查找变量
	value = zend_hash_find_ex(target_symbol_table, name, OP1_TYPE == IS_CONST);

	// 如果 OP1 不是常量
	if (OP1_TYPE != IS_CONST) {
		// 释放临时名称
		zend_tmp_string_release(tmp_name);
	}
	// 释放操作对象的附加变量
	FREE_OP1();

	// 如果 符号表中 没有查找 到变量
	if (!value) {
		// 如果是empty函数，结果为1
		result = (opline->extended_value & ZEND_ISEMPTY);
	// 否则，找到有变量
	} else {
		// 如果找到的是间接引用类型
		if (Z_TYPE_P(value) == IS_INDIRECT) {
			// 解引用
			value = Z_INDIRECT_P(value);
		}
		// 如果没有 ZEND_ISEMPTY 标记（isset解引用）
		if (!(opline->extended_value & ZEND_ISEMPTY)) {
			// 如果 值是引用类型
			if (Z_ISREF_P(value)) {
				// 解引用
				value = Z_REFVAL_P(value);
			}
			// 取回的值不可以是 undef/null/false
			result = Z_TYPE_P(value) > IS_NULL;
		// 如果有 ZEND_ISEMPTY 标记（empty不解引用）
		} else {
			// 把结果取反
			result = !i_zend_is_true(value);
		}
	}
	// 根据当前操作码结果选择下一个操作码，p1:传入值，主要用来做判断。p2:是否检查异常
	ZEND_VM_SMART_BRANCH(result, 1);
}

// ing3, 静态属性的 isset 和 empty 函数
/* No specialization for op_types (CONST|TMPVAR|CV, UNUSED|CLASS_FETCH|CONST|VAR) */
ZEND_VM_HANDLER(180, ZEND_ISSET_ISEMPTY_STATIC_PROP, ANY, CLASS_FETCH, ISSET|CACHE_SLOT)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *value;
	zend_result fetch_result;
	bool result;

	SAVE_OPLINE();

	// 通过缓存位置和类型，返回静态属性地址。p1:返回属性值，p2:返回属性信息，p3:缓存位置，p4:获取类型，p5:falgs
	fetch_result = zend_fetch_static_property_address(&value, NULL, opline->extended_value & ~ZEND_ISEMPTY, BP_VAR_IS, 0 OPLINE_CC EXECUTE_DATA_CC);

	// 如果不是empty, 是isset
	if (!(opline->extended_value & ZEND_ISEMPTY)) {
		// 如果元素存在，并且类型不是 null,undef 并且（值不是引用类型 或 引用目标不是null）
			// 结果为是 
		result = fetch_result == SUCCESS && Z_TYPE_P(value) > IS_NULL &&
		    (!Z_ISREF_P(value) || Z_TYPE_P(Z_REFVAL_P(value)) != IS_NULL);
	// 否则
	} else {
		// 没有元素 或 值为否。结果为是
		result = fetch_result != SUCCESS || !i_zend_is_true(value);
	}
	// 根据当前操作码结果选择下一个操作码，p1:传入值，主要用来做判断。p2:是否检查异常
	ZEND_VM_SMART_BRANCH(result, 1);
}

// ing3, 对象属性的 isset 和 empty 函数
ZEND_VM_COLD_CONSTCONST_HANDLER(115, ZEND_ISSET_ISEMPTY_DIM_OBJ, CONST|TMPVAR|CV, CONST|TMPVAR|CV, ISSET)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *container;
	bool result;
	zend_ulong hval;
	zval *offset;

	// windows: 无操作
	SAVE_OPLINE();
	// 获取zval指针, UNUSED 返回 $this
	container = GET_OP1_OBJ_ZVAL_PTR_UNDEF(BP_VAR_IS);
	// 获取zval指针, UNUSED 返回null
	offset = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);

	// 如果 容器 类型是 数组
	if (EXPECTED(Z_TYPE_P(container) == IS_ARRAY)) {
		HashTable *ht;
		zval *value;
		zend_string *str;

// 创建跳转标签 isset_dim_obj_array
ZEND_VM_C_LABEL(isset_dim_obj_array):
		// 从容器中取出哈希表
		ht = Z_ARRVAL_P(container);
// 创建跳转标签 isset_again
ZEND_VM_C_LABEL(isset_again):
		// 如果偏移量是 字串
		if (EXPECTED(Z_TYPE_P(offset) == IS_STRING)) {
			// 取出 字串
			str = Z_STR_P(offset);
			// 如果 OP2 不是常量
			if (OP2_TYPE != IS_CONST) {
				// 如果字串全是数字
				if (ZEND_HANDLE_NUMERIC(str, hval)) {
					// 跳转到指定标签 num_index_prop
					ZEND_VM_C_GOTO(num_index_prop);
				}
			}
			// 哈希表里用 key 查找元素。p1:哈希表，p2:key，p3：key上是否有哈希值。
			// op2是常量时有哈希值。
			value = zend_hash_find_ex(ht, str, OP2_TYPE == IS_CONST);
		// 如果offset是整数
		} else if (EXPECTED(Z_TYPE_P(offset) == IS_LONG)) {
			// 取出整数
			hval = Z_LVAL_P(offset);
// 创建跳转标签 num_index_prop
ZEND_VM_C_LABEL(num_index_prop):
			// 按哈希值查询，兼容顺序数组。p1:哈希表，p2:哈希值
			value = zend_hash_index_find(ht, hval);
		// 否则 如果 op2 是普通变量或编译变量 并且 op2 是引用类型
		} else if ((OP2_TYPE & (IS_VAR|IS_CV)) && EXPECTED(Z_ISREF_P(offset))) {
			// 解引用
			offset = Z_REFVAL_P(offset);
			// 跳转到指定标签 isset_again
			ZEND_VM_C_GOTO(isset_again);
		// 否则, 其他情况
		} else {
			// 在数组里用维度查找
			value = zend_find_array_dim_slow(ht, offset EXECUTE_DATA_CC);
			// 如果有异常
			if (UNEXPECTED(EG(exception))) {
				// 结果为 0
				result = 0;
				// 跳转到指定标签 isset_dim_obj_exit
				ZEND_VM_C_GOTO(isset_dim_obj_exit);
			}
		}

		// 如果不是empty函数，是isset
		if (!(opline->extended_value & ZEND_ISEMPTY)) {
			// >IS_NULL 说明 不是 IS_UNDEF 和 IS_NULL
			/* > IS_NULL means not IS_UNDEF and not IS_NULL */
			// 为真的条件：值不是null并且类型大于IS_NULL 并且（如果是引用，引用目标不是null）
			result = value != NULL && Z_TYPE_P(value) > IS_NULL &&
			    (!Z_ISREF_P(value) || Z_TYPE_P(Z_REFVAL_P(value)) != IS_NULL);

			// 如果op1是常量或编译变量
			if (OP1_TYPE & (IS_CONST|IS_CV)) {
				// 检查无效异常
				/* avoid exception check */
				// 释放操作对象的附加变量
				FREE_OP2();
				// 根据当前操作码结果选择下一个操作码，p1:传入值，主要用来做判断。p2:是否检查异常
				ZEND_VM_SMART_BRANCH(result, 0);
			}
		// 否则，是empty方法
		} else {
			// 检查是 null或false
			result = (value == NULL || !i_zend_is_true(value));
		}
		// 跳转到指定标签 isset_dim_obj_exit
		ZEND_VM_C_GOTO(isset_dim_obj_exit);
	// 如果 op1 是普通变量或编译变量 并且 op1是引用类型
	} else if ((OP1_TYPE & (IS_VAR|IS_CV)) && EXPECTED(Z_ISREF_P(container))) {
		// 解引用
		container = Z_REFVAL_P(container);
		// 如果 容器 类型是 数组
		if (EXPECTED(Z_TYPE_P(container) == IS_ARRAY)) {
			// 跳转到指定标签 isset_dim_obj_array
			ZEND_VM_C_GOTO(isset_dim_obj_array);
		}
	}

	// 如果op2是常量 并且 有额外的参数
	if (OP2_TYPE == IS_CONST && Z_EXTRA_P(offset) == ZEND_EXTRA_VALUE) {
		// 使用下一个zval做为偏移量
		offset++;
	}
	// 如果是isset方法
	if (!(opline->extended_value & ZEND_ISEMPTY)) {
		// 检验 对象 或 字串 中 此索引对应的值是否 存在
		result = zend_isset_dim_slow(container, offset EXECUTE_DATA_CC);
	// 否则，empty方法
	} else {
		// 检验 对象 或 字串 中 此索引对应的值是否是 empty
		result = zend_isempty_dim_slow(container, offset EXECUTE_DATA_CC);
	}

// 创建跳转标签 isset_dim_obj_exit
ZEND_VM_C_LABEL(isset_dim_obj_exit):
	// 释放操作对象的附加变量
	FREE_OP2();
	// 释放操作对象的附加变量
	FREE_OP1();
	// 根据当前操作码结果选择下一个操作码，p1:传入值，主要用来做判断。p2:是否检查异常
	ZEND_VM_SMART_BRANCH(result, 1);
}

// ing3, 针对对象属性的 emtpy, isset
ZEND_VM_COLD_CONST_HANDLER(148, ZEND_ISSET_ISEMPTY_PROP_OBJ, CONST|TMPVAR|UNUSED|THIS|CV, CONST|TMPVAR|CV, ISSET|CACHE_SLOT)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *container;
	int result;
	zval *offset;
	zend_string *name, *tmp_name;

	// windows: 无操作
	SAVE_OPLINE();
	// 获取zval指针, UNUSED 返回 $this，不支持 TMPVARCV
	container = GET_OP1_OBJ_ZVAL_PTR(BP_VAR_IS);
	// 获取zval指针，UNUSED 类型返回null
	offset = GET_OP2_ZVAL_PTR(BP_VAR_R);

	// 如果op1是常量 或 （op1不是未使用 并且 容器不是对象）
	if (OP1_TYPE == IS_CONST ||
	    (OP1_TYPE != IS_UNUSED && UNEXPECTED(Z_TYPE_P(container) != IS_OBJECT))) {
		// 如果 op1 是（变量 或 编译变量） 并且 容器是引用类型
		if ((OP1_TYPE & (IS_VAR|IS_CV)) && Z_ISREF_P(container)) {
			// 解引用
			container = Z_REFVAL_P(container);
			// 如果容器不是对象
			if (UNEXPECTED(Z_TYPE_P(container) != IS_OBJECT)) {
				// empty返回true, isset返回false
				result = (opline->extended_value & ZEND_ISEMPTY);
				// 跳转到指定标签 isset_object_finish
				ZEND_VM_C_GOTO(isset_object_finish);
			}
		// 否则
		} else {
			// empty返回true, isset返回false
			result = (opline->extended_value & ZEND_ISEMPTY);
			// 跳转到指定标签 isset_object_finish
			ZEND_VM_C_GOTO(isset_object_finish);
		}
	}

	// 如果类型为 常量
	if (OP2_TYPE == IS_CONST) {
		// 取出索引名
		name = Z_STR_P(offset);
	// 否则
	} else {
		// 获取临时字串（不抛错）返回
		name = zval_try_get_tmp_string(offset, &tmp_name);
		// 如果名字无效
		if (UNEXPECTED(!name)) {
			// 结果为否
			result = 0;
			// 跳转到指定标签 isset_object_finish
			ZEND_VM_C_GOTO(isset_object_finish);
		}
	}

	// （empty方法） 抑或 检查属性名存在
		// p1:对象
		// p2:属性名
		// p3:是否使用empty
		// p4:缓存位置: op2是常量 ？ 缓存位置 ：null
	result =
		(opline->extended_value & ZEND_ISEMPTY) ^
		Z_OBJ_HT_P(container)->has_property(Z_OBJ_P(container), name, (opline->extended_value & ZEND_ISEMPTY), ((OP2_TYPE == IS_CONST) ? CACHE_ADDR(opline->extended_value & ~ZEND_ISEMPTY) : NULL));

	// 如果 OP2 不是常量
	if (OP2_TYPE != IS_CONST) {
		// 释放临时名称
		zend_tmp_string_release(tmp_name);
	}

// 创建跳转标签 isset_object_finish
ZEND_VM_C_LABEL(isset_object_finish):
	// 释放操作对象的附加变量
	FREE_OP2();
	// 释放操作对象的附加变量
	FREE_OP1();
	// 根据当前操作码结果选择下一个操作码，p1:传入值，主要用来做判断。p2:是否检查异常
	ZEND_VM_SMART_BRANCH(result, 1);
}

// ing3, 检查哈希表中key是否存在 array_key_exists
ZEND_VM_HANDLER(194, ZEND_ARRAY_KEY_EXISTS, CV|TMPVAR|CONST, CV|TMPVAR|CONST)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	zval *key, *subject;
	HashTable *ht;
	bool result;

	// windows: 无操作
	SAVE_OPLINE();

	// 获取zval指针, UNUSED 返回null
	key = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	
	// 获取zval指针, UNUSED 返回null
	subject = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 如果类型是 数组
	if (EXPECTED(Z_TYPE_P(subject) == IS_ARRAY)) {
// 创建跳转标签 array_key_exists_array
ZEND_VM_C_LABEL(array_key_exists_array):
		// 取出数组
		ht = Z_ARRVAL_P(subject);
		// 检查哈希表里是否存在这个键，p1:哈希表，p2:key
		result = zend_array_key_exists_fast(ht, key OPLINE_CC EXECUTE_DATA_CC);
	// 否则
	} else {
		// 否则 如果 op2 是普通变量或编译变量 并且 op2 是引用类型
		if ((OP2_TYPE & (IS_VAR|IS_CV)) && EXPECTED(Z_ISREF_P(subject))) {
			// 解引用
			subject = Z_REFVAL_P(subject);
			// 如果类型是 数组
			if (EXPECTED(Z_TYPE_P(subject) == IS_ARRAY)) {
				// 跳转到指定标签 array_key_exists_array
				ZEND_VM_C_GOTO(array_key_exists_array);
			}
		}
		// 不是数组，也不是引用类型。或引用目标不是数组。
		// 报错：array_key_exists 函数错误
		zend_array_key_exists_error(subject, key OPLINE_CC EXECUTE_DATA_CC);
		// 结果为 否
		result = 0;
	}

	// 释放操作对象的附加变量
	FREE_OP2();
	// 释放操作对象的附加变量
	FREE_OP1();
	// 根据当前操作码结果选择下一个操作码，p1:传入值，主要用来做判断。p2:是否检查异常
	ZEND_VM_SMART_BRANCH(result, 1);
}

/* No specialization for op_types (CONST|TMPVAR|UNUSED|CV, ANY) */
// ing3, 退出
ZEND_VM_COLD_HANDLER(79, ZEND_EXIT, ANY, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// windows: 无操作
	SAVE_OPLINE();
	// 如果 op1 类型不是 未使用
	if (OP1_TYPE != IS_UNUSED) {
		// 获取zval指针，UNUSED 类型返回null
		zval *ptr = GET_OP1_ZVAL_PTR(BP_VAR_R);
		// 为了break
		do {
			// 如果op1的值是整数
			if (Z_TYPE_P(ptr) == IS_LONG) {
				// 更新 退出状态
				EG(exit_status) = Z_LVAL_P(ptr);
			// 否则，op1不是整数
			} else {
				// 如果 op1 类型是 IS_VAR 或 IS_CV 并且 op1是引用类型
				if ((OP1_TYPE & (IS_VAR|IS_CV)) && Z_ISREF_P(ptr)) {
					// 解引用
					ptr = Z_REFVAL_P(ptr);
					// 如果目标值是整数 
					if (Z_TYPE_P(ptr) == IS_LONG) {
						// 更新 退出状态
						EG(exit_status) = Z_LVAL_P(ptr);
						// 跳过打印
						break;
					}
				}
				// 如果op1不是整数 ，把op1打印出来 
				zend_print_zval(ptr, 0);
			}
		} while (0);
		// 释放操作对象的附加变量
		FREE_OP1();
	}

	// 如果没有异常 
	if (!EG(exception)) {
		// 抛出 unwind_exit 异常
		zend_throw_unwind_exit();
	}
	// 主要是 return
	HANDLE_EXCEPTION();
}

// ing3, 开始静默
ZEND_VM_HANDLER(57, ZEND_BEGIN_SILENCE, ANY, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// 把现有 EG(error_reporting) 存入结果中
	ZVAL_LONG(EX_VAR(opline->result.var), EG(error_reporting));

	// 如果不是 只有报错
	if (!E_HAS_ONLY_FATAL_ERRORS(EG(error_reporting))) {
		// 为了break
		do {
			// 不可以禁用致使错误
			/* Do not silence fatal errors */
			// 如果原来开启了 致使错误，保持开启
			EG(error_reporting) &= E_FATAL_ERRORS;
			// 如果没设置 EG(error_reporting_ini_entry)
			if (!EG(error_reporting_ini_entry)) {
				// 在ini配置表里 查找 error_reporting
				zval *zv = zend_hash_find_known_hash(EG(ini_directives), ZSTR_KNOWN(ZEND_STR_ERROR_REPORTING));
				// 如果有这个值
				if (zv) {
					// 把找到的值写入 EG(error_reporting_ini_entry)
					EG(error_reporting_ini_entry) = (zend_ini_entry *)Z_PTR_P(zv);
				// 否则，没有这个值
				} else {
					// 跳出
					break;
				}
			}
			// 如果 EG(error_reporting_ini_entry)->modified 没有修改过
			if (!EG(error_reporting_ini_entry)->modified) {
				// 如果没有设置 EG(modified_ini_directives)
				if (!EG(modified_ini_directives)) {
					// 创建哈希表 EG(modified_ini_directives)
					ALLOC_HASHTABLE(EG(modified_ini_directives));
					// 初始化哈希表 ，8个元素
					zend_hash_init(EG(modified_ini_directives), 8, NULL, NULL, 0);
				}
				// EG(modified_ini_directives) 哈希表中添加 error_reporting ，如果成功
				if (EXPECTED(zend_hash_add_ptr(EG(modified_ini_directives), ZSTR_KNOWN(ZEND_STR_ERROR_REPORTING), EG(error_reporting_ini_entry)) != NULL)) {
					// 更新原始值
					EG(error_reporting_ini_entry)->orig_value = EG(error_reporting_ini_entry)->value;
					// 原始可更新状态
					EG(error_reporting_ini_entry)->orig_modifiable = EG(error_reporting_ini_entry)->modifiable;
					// 标记成已更新
					EG(error_reporting_ini_entry)->modified = 1;
				}
			}
		} while (0);
	}
	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, 结束静默
ZEND_VM_HANDLER(58, ZEND_END_SILENCE, TMP, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	// 如果 仅包含 致命错误 并且 原值不是仅包含 致命错误
	if (E_HAS_ONLY_FATAL_ERRORS(EG(error_reporting))
			&& !E_HAS_ONLY_FATAL_ERRORS(Z_LVAL_P(EX_VAR(opline->op1.var)))) {
		// 恢复原始值
		EG(error_reporting) = Z_LVAL_P(EX_VAR(opline->op1.var));
	}
	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, 检查异常并跳转
ZEND_VM_COLD_CONST_HANDLER(152, ZEND_JMP_SET, CONST|TMP|VAR|CV, JMP_ADDR)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *value;
	zend_reference *ref = NULL;
	bool ret;

	// windows: 无操作
	SAVE_OPLINE();
	// 获取zval指针，UNUSED 类型返回null
	value = GET_OP1_ZVAL_PTR(BP_VAR_R);

	// 如果 op1 是 普通变量 或 编译变量 并且 值为引用类型
	if ((OP1_TYPE == IS_VAR || OP1_TYPE == IS_CV) && Z_ISREF_P(value)) {
		// 如果 op1 是 普通变量
		if (OP1_TYPE == IS_VAR) {
			// 取出引用实例
			ref = Z_REF_P(value);
		}
		// 解引用
		value = Z_REFVAL_P(value);
	}

	// 值转成 布尔型
	ret = i_zend_is_true(value);

	// 如果有异常
	if (UNEXPECTED(EG(exception))) {
		// 释放操作对象的附加变量
		FREE_OP1();
		// 结果为 未定义
		ZVAL_UNDEF(EX_VAR(opline->result.var));
		// 主要是 return
		HANDLE_EXCEPTION();
	}

	// 如果值有效
	if (ret) {
		// 在执行数据中取出 指定序号的变量
		zval *result = EX_VAR(opline->result.var);
		// 值 放到结果里
		ZVAL_COPY_VALUE(result, value);
		// 如果op1是常量
		if (OP1_TYPE == IS_CONST) {
			// 如果是可以计数类型
			// 引用次数+1
			if (UNEXPECTED(Z_OPT_REFCOUNTED_P(result))) Z_ADDREF_P(result);
		// 否则 如果 op1 是 编译变量
		} else if (OP1_TYPE == IS_CV) {
			// 如果是可以计数类型
			// 引用次数+1
			if (Z_OPT_REFCOUNTED_P(result)) Z_ADDREF_P(result);
		// 否则 如果op1是变量 并且 有引用实例
		} else if (OP1_TYPE == IS_VAR && ref) {
			// 引用次数-1，如果为0
			if (UNEXPECTED(GC_DELREF(ref) == 0)) {
				// 释放引用实例
				efree_size(ref, sizeof(zend_reference));
			// 否则， 如果是可以计数类型
			} else if (Z_OPT_REFCOUNTED_P(result)) {
				// 引用次数+1
				Z_ADDREF_P(result);
			}
		}
		// 要检查异常 并 有异常时使用旧操作码，无异常使用新操作码, p1:新操作码，p2:是否检查异常
		// OP_JMP_ADDR: 访问 p2.jmp_addr
		ZEND_VM_JMP_EX(OP_JMP_ADDR(opline, opline->op2), 0);
	}

	// 释放操作对象的附加变量
	FREE_OP1();
	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, "??" 连接的两个表达式（默认值）
ZEND_VM_COLD_CONST_HANDLER(169, ZEND_COALESCE, CONST|TMP|VAR|CV, JMP_ADDR)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *value;
	zend_reference *ref = NULL;

	// windows: 无操作
	SAVE_OPLINE();
	// 获取zval指针，UNUSED 类型返回null
	value = GET_OP1_ZVAL_PTR(BP_VAR_IS);

	// op1 是变量或编译变量 并且 值是引用类型
	if ((OP1_TYPE & (IS_VAR|IS_CV)) && Z_ISREF_P(value)) {
		// 如果 op1是普通变量
		if (OP1_TYPE & IS_VAR) {
			// 取出引用实例
			ref = Z_REF_P(value);
		}
		// 解引用
		value = Z_REFVAL_P(value);
	}

	// 如果值不是 undef 和 null
	if (Z_TYPE_P(value) > IS_NULL) {
		// 在执行数据中取出 指定序号的变量
		zval *result = EX_VAR(opline->result.var);
		// 把值复制给 操作码结果 
		ZVAL_COPY_VALUE(result, value);
		// 如果op1是常量
		if (OP1_TYPE == IS_CONST) {
			// 如果是可以计数类型
			// 引用次数+1
			if (UNEXPECTED(Z_OPT_REFCOUNTED_P(result))) Z_ADDREF_P(result);
		// 否则 如果 op1 是 编译变量
		} else if (OP1_TYPE == IS_CV) {
			// 如果是可以计数类型
			// 引用次数+1
			if (Z_OPT_REFCOUNTED_P(result)) Z_ADDREF_P(result);
		// 否则， 如果op1是普通变量 并且 有引用实例
		} else if ((OP1_TYPE & IS_VAR) && ref) {
			// 引用次数-1，如果为0
			if (UNEXPECTED(GC_DELREF(ref) == 0)) {
				// 释放引用实例
				efree_size(ref, sizeof(zend_reference));
			// 否则， 如果是可以计数类型
			} else if (Z_OPT_REFCOUNTED_P(result)) {
				// 引用次数+1
				Z_ADDREF_P(result);
			}
		}
		// 要检查异常 并 有异常时使用旧操作码，无异常使用新操作码, p1:新操作码，p2:是否检查异常
		// OP_JMP_ADDR: 访问 p2.jmp_addr
		ZEND_VM_JMP_EX(OP_JMP_ADDR(opline, opline->op2), 0);
	}

	// 如果op1是普通变量 并且 有引用实例
	if ((OP1_TYPE & IS_VAR) && ref) {
		// 引用次数-1，如果为0
		if (UNEXPECTED(GC_DELREF(ref) == 0)) {
			// 释放引用实例
			efree_size(ref, sizeof(zend_reference));
		}
	}
	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, 验证op1，碰到null时触发短路, 并跳到 p2指定的位置
ZEND_VM_HOT_NOCONST_HANDLER(198, ZEND_JMP_NULL, CONST|TMP|VAR|CV, JMP_ADDR)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *val, *result;

	// 获取zval指针, UNUSED 返回null
	val = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 如果 op1 类型不是 null,undef
	if (Z_TYPE_P(val) > IS_NULL) {
		// 为了break
		do {
			// 如果 op1 是 编译变量 或 普通变量 并且 值是引用类型
			if ((OP1_TYPE == IS_CV || OP1_TYPE == IS_VAR) && Z_TYPE_P(val) == IS_REFERENCE) {
				// 解引用
				val = Z_REFVAL_P(val);
				// 如果值为 null,undef
				if (Z_TYPE_P(val) <= IS_NULL) {
					// 释放操作对象的附加变量
					FREE_OP1();
					// 跳出
					break;
				}
			}
			// 能找这里说明没有出现短路
			// opline + 1，到目标操作码并 return
			ZEND_VM_NEXT_OPCODE();
		} while (0);
	}
	// 在执行数据中取出 指定序号的变量
	result = EX_VAR(opline->result.var);
	// 短路类型
	uint32_t short_circuiting_type = opline->extended_value & ZEND_SHORT_CIRCUITING_CHAIN_MASK;
	// 如果类型为 短路链表达式
	if (EXPECTED(short_circuiting_type == ZEND_SHORT_CIRCUITING_CHAIN_EXPR)) {
		// 结果为null
		ZVAL_NULL(result);
		// 如果 op1 是 编译变量 并且 值为未定义 并且 扩展值中没有 【ZEND_JMP_NULL_BP_VAR_IS】 
		if (OP1_TYPE == IS_CV 
			&& UNEXPECTED(Z_TYPE_P(val) == IS_UNDEF)
			&& (opline->extended_value & ZEND_JMP_NULL_BP_VAR_IS) == 0
		) {
			// windows: 无操作
			SAVE_OPLINE();
			// 报错：p1的变量（变量名）未定义, 并返未初始化zval
			ZVAL_UNDEFINED_OP1();
			// 如果有异常
			if (UNEXPECTED(EG(exception) != NULL)) {
				// 主要是 return
				HANDLE_EXCEPTION();
			}
		}
	// 如果类型为 短路链isset
	} else if (short_circuiting_type == ZEND_SHORT_CIRCUITING_CHAIN_ISSET) {
		// 结果为假
		ZVAL_FALSE(result);
	// 否则, 类型为 短路链empty
	} else {
		// 也就是empty函数验证短路表达式
		ZEND_ASSERT(short_circuiting_type == ZEND_SHORT_CIRCUITING_CHAIN_EMPTY);
		// 结果为真
		ZVAL_TRUE(result);
	}

	// 要检查异常 并 有异常时使用旧操作码，无异常使用新操作码, p1:新操作码，p2:是否检查异常
	// OP_JMP_ADDR: 访问 p2.jmp_addr
	ZEND_VM_JMP_EX(OP_JMP_ADDR(opline, opline->op2), 0);
}

// ing3, op1复制给result
ZEND_VM_HOT_HANDLER(31, ZEND_QM_ASSIGN, CONST|TMP|VAR|CV, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *value;
	// 在执行数据中取出 指定序号的变量
	zval *result = EX_VAR(opline->result.var);

	// 获取zval指针, UNUSED 返回null
	value = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 如果 op1 是 编译变量 并且值为 未定义
	if (OP1_TYPE == IS_CV && UNEXPECTED(Z_TYPE_P(value) == IS_UNDEF)) {
		// windows: 无操作
		SAVE_OPLINE();
		// 报错：p1的变量（变量名）未定义, 并返未初始化zval
		ZVAL_UNDEFINED_OP1();
		// 结果为null
		ZVAL_NULL(result);
		// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
		ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
	}

	// 如果 op1 是 编译变量
	if (OP1_TYPE == IS_CV) {
		// 复制引用目标 和 附加信息，并增加引用计数
		ZVAL_COPY_DEREF(result, value);
	// 否则 如果 op1 是 普通变量
	} else if (OP1_TYPE == IS_VAR) {
		// 如果op1是引用类型
		if (UNEXPECTED(Z_ISREF_P(value))) {
			// op1引用目标复制给result
			ZVAL_COPY_VALUE(result, Z_REFVAL_P(value));
			// 如果目标 引用次数-1后为0
			if (UNEXPECTED(Z_DELREF_P(value) == 0)) {
				// 释放引用实例
				efree_size(Z_REF_P(value), sizeof(zend_reference));
			// 否则， 如果是可以计数类型
			} else if (Z_OPT_REFCOUNTED_P(result)) {
				// 引用次数+1
				Z_ADDREF_P(result);
			}
		// 否则，op1不是引用类型
		} else {
			// op1的复制给result
			ZVAL_COPY_VALUE(result, value);
		}
	// 否则
	} else {
		// op1复制给result
		ZVAL_COPY_VALUE(result, value);
		// 如果op1是常量
		if (OP1_TYPE == IS_CONST) {
			// 如果是可以计数类型
			if (UNEXPECTED(Z_OPT_REFCOUNTED_P(result))) {
				// 引用次数+1
				Z_ADDREF_P(result);
			}
		}
	}
	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, 调用扩展处理语句，附加参数 execute_data
ZEND_VM_COLD_HANDLER(101, ZEND_EXT_STMT, ANY, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	// 如果没有禁用扩展
	if (!EG(no_extensions)) {
		// windows: 无操作
		SAVE_OPLINE();
		// 对每个扩展调用 zend_extension_statement_handler，附加参数 execute_data
		zend_llist_apply_with_argument(&zend_extensions, (llist_apply_with_arg_func_t) zend_extension_statement_handler, execute_data);
		// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
		ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
	}
	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, 对每个扩展调用 zend_extension_fcall_begin_handler
ZEND_VM_COLD_HANDLER(102, ZEND_EXT_FCALL_BEGIN, ANY, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// 如果没有禁用扩展
	if (!EG(no_extensions)) {
		// windows: 无操作
		SAVE_OPLINE();
		// 对每个扩展调用 zend_extension_fcall_begin_handler, 附加参数是 execute_data
		zend_llist_apply_with_argument(&zend_extensions, (llist_apply_with_arg_func_t) zend_extension_fcall_begin_handler, execute_data);
		// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
		ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
	}
	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, 对每个扩展调用 zend_extension_fcall_end_handler
ZEND_VM_COLD_HANDLER(103, ZEND_EXT_FCALL_END, ANY, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// 如果没有禁用 扩展
	if (!EG(no_extensions)) {
		// windows: 无操作
		SAVE_OPLINE();
		// 对每个扩展调用 zend_extension_fcall_end_handler，附加参数是 execute_data
		zend_llist_apply_with_argument(&zend_extensions, (llist_apply_with_arg_func_t) zend_extension_fcall_end_handler, execute_data);
		// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
		ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
	}
	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, 声名类，类已经解析过了，这里主要是链接
ZEND_VM_HANDLER(144, ZEND_DECLARE_CLASS, CONST, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// windows: 无操作
	SAVE_OPLINE();
	
	// 绑定父类。p1:类型名，p2:父类名
	// op1 类名，op2父类名
	// 访问 (p2).zv。p1:opline,p2:node
	do_bind_class(RT_CONSTANT(opline, opline->op1), (OP2_TYPE == IS_CONST) ? Z_STR_P(RT_CONSTANT(opline, opline->op2)) : NULL);
	// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, 延时声名类
ZEND_VM_HANDLER(145, ZEND_DECLARE_CLASS_DELAYED, CONST, CONST)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// 通过偏移量 在运行时缓存中获取一个 (void**) 中的第一个元素
	zend_class_entry *ce = CACHED_PTR(opline->extended_value);
	// 如果 类不存在
	if (ce == NULL) {
		// op1是小写函数名
		// 访问 (p2).zv。p1:opline,p2:node
		zval *lcname = RT_CONSTANT(opline, opline->op1);
		// 用函数名的下一个 zval在类表里查找
		zval *zv = zend_hash_find_known_hash(EG(class_table), Z_STR_P(lcname + 1));
		// 如果找到类
		if (zv) {
			// windows: 无操作
			SAVE_OPLINE();
			// 构建类，并添加到类表中，p1:类 zval，p2:类名，p3:父类名
			// 访问 (p2).zv。p1:opline,p2:node
			ce = zend_bind_class_in_slot(zv, lcname, Z_STR_P(RT_CONSTANT(opline, opline->op2)));
			// 如果没有返回类
			if (!ce) {
				// 主要是 return
				HANDLE_EXCEPTION();
			}
		}
		// 通过偏移量 在 EX(run_time_cache) 中获取一个 (void**) 中的第一个元素，并给它赋值
		CACHE_PTR(opline->extended_value, ce);
	}
	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, 声名匿名类。类已经解析过了，这里主要是链接
ZEND_VM_HANDLER(146, ZEND_DECLARE_ANON_CLASS, ANY, ANY, CACHE_SLOT)
{
	zval *zv;
	zend_class_entry *ce;
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// 通过偏移量 在运行时缓存中获取一个 (void**) 中的第一个元素
	ce = CACHED_PTR(opline->extended_value);
	// 如果 类不存在
	if (UNEXPECTED(ce == NULL)) {
		// 取得唯一编号
		// 访问 (p2).zv。p1:opline,p2:node
		zend_string *rtd_key = Z_STR_P(RT_CONSTANT(opline, opline->op1));
		// 通过唯一编号在类表里查找
		zv = zend_hash_find_known_hash(EG(class_table), rtd_key);
		// 必须要找到
		ZEND_ASSERT(zv != NULL);
		// 取出类入口
		ce = Z_CE_P(zv);
		// 如果类没有链接过
		if (!(ce->ce_flags & ZEND_ACC_LINKED)) {
			// windows: 无操作
			SAVE_OPLINE();
			// 链接类 : zend_inheritance.c
			// 访问 (p2).zv。p1:opline,p2:node
			ce = zend_do_link_class(ce, (OP2_TYPE == IS_CONST) ? Z_STR_P(RT_CONSTANT(opline, opline->op2)) : NULL, rtd_key);
			// 如果链接失败，没有返回类
			if (!ce) {
				// 主要是 return
				HANDLE_EXCEPTION();
			}
		}
		// 通过偏移量 在 EX(run_time_cache) 中获取一个 (void**) 中的第一个元素，并给它赋值
		CACHE_PTR(opline->extended_value, ce);
	}
	// 取出类入口
	Z_CE_P(EX_VAR(opline->result.var)) = ce;
	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, 把指定编号的函数添加到函数列表，op2:指定编号，p1:函数名
ZEND_VM_HANDLER(141, ZEND_DECLARE_FUNCTION, ANY, NUM)
{
	zend_function *func;
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// windows: 无操作
	SAVE_OPLINE();
	// 找到指定编号的函数
	func = (zend_function *) EX(func)->op_array.dynamic_func_defs[opline->op2.num];
	// 把函数添加到函数列表中，p1:函数，p2:函数名
	// 访问 (p2).zv。p1:opline,p2:node
	do_bind_function(func, RT_CONSTANT(opline, opline->op1));
	// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing2, 处理 declare(...) 里面定义的 ticks
ZEND_VM_HANDLER(105, ZEND_TICKS, ANY, ANY, NUM)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// 如果 ticks 数量大于 操作码中记录的数量 
	if ((uint32_t)++EG(ticks_count) >= opline->extended_value) {
		// 计数归0
		EG(ticks_count) = 0;
		// 如果有 zend_ticks_function -> php_run_ticks : /main/php_ticks.c
		if (zend_ticks_function) {
			// windows: 无操作
			SAVE_OPLINE();
			// 锁定 fiber
			zend_fiber_switch_block();
			// php_run_ticks: main/ticks.c
			zend_ticks_function(opline->extended_value);
			// 解锁 fiber 
			zend_fiber_switch_unblock();
			// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
			ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
		}
	}
	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, 检验对象从属于类
ZEND_VM_HANDLER(138, ZEND_INSTANCEOF, TMPVAR|CV, UNUSED|CLASS_FETCH|CONST|VAR, CACHE_SLOT)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *expr;
	bool result;

	// windows: 无操作
	SAVE_OPLINE();
	// 获取zval指针, UNUSED 返回null
	expr = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);

// 创建跳转标签 try_instanceof
ZEND_VM_C_LABEL(try_instanceof):
	// 如果 op1 是对象
	if (Z_TYPE_P(expr) == IS_OBJECT) {
		zend_class_entry *ce;

		// 如果类型为 常量
		if (OP2_TYPE == IS_CONST) {
			// 通过偏移量 在运行时缓存中获取一个 (void**) 中的第一个元素
			ce = CACHED_PTR(opline->extended_value);
			// 如果 类不存在
			if (UNEXPECTED(ce == NULL)) {
				// 查找类。p1:类名，p2:key，p3:flags
				// 访问 (p2).zv。p1:opline,p2:node
				ce = zend_lookup_class_ex(Z_STR_P(RT_CONSTANT(opline, opline->op2)), Z_STR_P(RT_CONSTANT(opline, opline->op2) + 1), ZEND_FETCH_CLASS_NO_AUTOLOAD);
				// 如果找到类
				if (EXPECTED(ce)) {
					// 通过偏移量 在 EX(run_time_cache) 中获取一个 (void**) 中的第一个元素，并给它赋值
					CACHE_PTR(opline->extended_value, ce);
				}
			}
		// 否则 如果 op2 类型是未定义
		} else if (OP2_TYPE == IS_UNUSED) {
			// 通过类名或类型查找类，p1:类名，p2:类型
			ce = zend_fetch_class(NULL, opline->op2.num);
			// 如果 类不存在
			if (UNEXPECTED(ce == NULL)) {
				// 释放操作对象的附加变量
				FREE_OP1();
				// 结果为 未定义
				ZVAL_UNDEF(EX_VAR(opline->result.var));
				// 主要是 return
				HANDLE_EXCEPTION();
			}
		// 否则
		} else {
			// 取出类入口
			ce = Z_CE_P(EX_VAR(opline->op2.var));
		}
		// 类有效 并且 op1 属于 此类。为真。
		result = ce && instanceof_function(Z_OBJCE_P(expr), ce);
	// 如果op1不是对象。 op1是普通变量 或 编译变量 并且 类型是引用
	} else if ((OP1_TYPE & (IS_VAR|IS_CV)) && Z_TYPE_P(expr) == IS_REFERENCE) {
		// 解引用
		expr = Z_REFVAL_P(expr);
		// 跳转到指定标签 try_instanceof
		ZEND_VM_C_GOTO(try_instanceof);
	// 否则
	} else {
		// 如果 op1 是 编译变量 并且 表达式是 未定义
		if (OP1_TYPE == IS_CV && UNEXPECTED(Z_TYPE_P(expr) == IS_UNDEF)) {
			// 报错：p1的变量（变量名）未定义, 并返未初始化zval
			ZVAL_UNDEFINED_OP1();
		}
		// 结果为否
		result = 0;
	}
	// 释放操作对象的附加变量
	FREE_OP1();
	// 根据当前操作码结果选择下一个操作码，p1:传入值，主要用来做判断。p2:是否检查异常
	ZEND_VM_SMART_BRANCH(result, 1);
}


// ing3, 无业务逻辑， 1/1
// ZEND_FASTCALL ZEND_EXT_NOP_SPEC，1个分支（验证过）
ZEND_VM_HOT_HANDLER(104, ZEND_EXT_NOP, ANY, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, 无业务逻辑， 1/1
// ZEND_FASTCALL ZEND_NOP_SPEC，1个分支（验证过）
ZEND_VM_HOT_HANDLER(0, ZEND_NOP, ANY, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, 分发 try/catch/finally 助手
ZEND_VM_HELPER(zend_dispatch_try_catch_finally_helper, ANY, ANY, uint32_t try_catch_offset, uint32_t op_num)
{
	// 在生成过程中可能是NULL（只级finally块被执行）
	/* May be NULL during generator closing (only finally blocks are executed) */
	// 当前异常
	zend_object *ex = EG(exception);

	// 倒序遍历 try/catch/finally 结构，进行必要的操作
	/* Walk try/catch/finally structures upwards, performing the necessary actions */
	// 如果偏移量不是 -1，偏移量-1
	for (; try_catch_offset != (uint32_t) -1; try_catch_offset--) {
		// 取得 try/catch 元素
		zend_try_catch_element *try_catch =
			&EX(func)->op_array.try_catch_array[try_catch_offset];

		// 如果操作码在catch前面（try块）,并且 执行数据有效
		if (op_num < try_catch->catch_op && ex) {
			// 跳到catch块
			/* Go to catch block */
			// 清理区域中的变量。 p1:执行数据，p2:包含操作码数据，p3:catch_op_num (?)
			cleanup_live_vars(execute_data, op_num, try_catch->catch_op);
			// 要检查异常 并 有异常时使用旧操作码，无异常使用新操作码, p1:新操作码，p2:是否检查异常
			ZEND_VM_JMP_EX(&EX(func)->op_array.opcodes[try_catch->catch_op], 0);
		// 如果在 finally 前面（catch块）
		} else if (op_num < try_catch->finally_op) {
			// 如果ex有效并且 ex->ce 是 zend_is_unwind_exit 类型
			if (ex && zend_is_unwind_exit(ex)) {
				// 退出前不执行 finally段
				/* Don't execute finally blocks on exit (for now) */
				continue;
			}
			// 跳到 finally段	
			/* Go to finally block */
			// fast_call 是finall段最后一个操作码的 op1
			// 在执行数据中取出 指定序号的变量
			zval *fast_call = EX_VAR(EX(func)->op_array.opcodes[try_catch->finally_end].op1.var);
			// 清理区域中的变量。 p1:执行数据，p2:包含操作码数据，p3:catch_op_num (?)
			cleanup_live_vars(execute_data, op_num, try_catch->finally_op);
			// 记录当前异常，后面怎么用?
			Z_OBJ_P(fast_call) = EG(exception);
			// 清空当前异常
			EG(exception) = NULL;
			// 操作码序号更新成 -1
			// 通过指针访问zval的 操作码序号
			Z_OPLINE_NUM_P(fast_call) = (uint32_t)-1;
			// 要检查异常 并 有异常时使用旧操作码，无异常使用新操作码, p1:新操作码，p2:是否检查异常
			ZEND_VM_JMP_EX(&EX(func)->op_array.opcodes[try_catch->finally_op], 0);
		// 如果在finally块
		} else if (op_num < try_catch->finally_end) {
			// 在执行数据中取出 指定序号的变量
			zval *fast_call = EX_VAR(EX(func)->op_array.opcodes[try_catch->finally_end].op1.var);

			// 清理未完成的 return 语句
			/* cleanup incomplete RETURN statement */
			// 通过指针访问zval的 操作码序号
			if (Z_OPLINE_NUM_P(fast_call) != (uint32_t)-1
			 && (EX(func)->op_array.opcodes[Z_OPLINE_NUM_P(fast_call)].op2_type & (IS_TMP_VAR | IS_VAR))) {
				// 在执行数据中取出 指定序号的变量
				// 通过指针访问zval的 操作码序号
				zval *return_value = EX_VAR(EX(func)->op_array.opcodes[Z_OPLINE_NUM_P(fast_call)].op2.var);
				// 销毁返回值
				zval_ptr_dtor(return_value);
			}

			// 在包装finally块时， 把潜在的异常串连起来
			/* Chain potential exception from wrapping finally block */
			// 如果有 fast_call
			if (Z_OBJ_P(fast_call)) {
				// 执行数据有效
				if (ex) {
					// 如果 ex->ce 是 zend_ce_unwind_exit 类型 或 zend_ce_graceful_exit 类型
					if (zend_is_unwind_exit(ex) || zend_is_graceful_exit(ex)) {
						// 抛弃前面抛出的异常 
						/* discard the previously thrown exception */
						// 释放 fast_call
						OBJ_RELEASE(Z_OBJ_P(fast_call));
					// 否则
					} else {
						// 如果ex和add_previous都不在对方的previous链上，把add_previous添加到exprevious链的源头上。
						zend_exception_set_previous(ex, Z_OBJ_P(fast_call));
					}
				// 否则
				} else {
					// fast_call 作为当前异常
					ex = EG(exception) = Z_OBJ_P(fast_call);
				}
			}
		}
	}

	// 未捕获的 异常
	/* Uncaught exception */
	// 如果有 zend_observer_fcall_op_array_extension
	if (zend_observer_fcall_op_array_extension != -1) {
		// 结束观察者调用
		zend_observer_fcall_end(execute_data, NULL);
	}
	// 清理区域中的变量。 p1:执行数据，p2:包含操作码数据，p3:catch_op_num (?)
	cleanup_live_vars(execute_data, op_num, 0);
	// 如果在调用生成器
	if (UNEXPECTED((EX_CALL_INFO() & ZEND_CALL_GENERATOR) != 0)) {
		// 获取正在运行的生成器
		zend_generator *generator = zend_get_running_generator(EXECUTE_DATA_C);
		// 关闭生成器
		zend_generator_close(generator, 1);
		// windows: return -1
		ZEND_VM_RETURN();
	// 否则，没有在调用生成器
	} else {
		// 不执行return, 必须初始化 返回值
		/* We didn't execute RETURN, and have to initialize return_value */
		// 如果有返回值
		if (EX(return_value)) {
			// 返回值 为 未定义
			ZVAL_UNDEF(EX(return_value));
		}
		// 调用方法 zend_leave_helper
		ZEND_VM_DISPATCH_TO_HELPER(zend_leave_helper);
	}
}

// ing3, 处理异常
ZEND_VM_HANDLER(149, ZEND_HANDLE_EXCEPTION, ANY, ANY)
{
	// 异常前的操作码，抛出操作码
	const zend_op *throw_op = EG(opline_before_exception);
	// 招聘操作码的序号
	uint32_t throw_op_num = throw_op - EX(func)->op_array.opcodes;
	// try/catch元素 序号
	int i, current_try_catch_offset = -1;

	// 如果（操作码是 ZEND_FREE 或 ZEND_FE_FREE）并且（throw语句 有 ZEND_FREE_ON_RETURN 标记）
	if ((throw_op->opcode == ZEND_FREE || throw_op->opcode == ZEND_FE_FREE)
		&& throw_op->extended_value & ZEND_FREE_ON_RETURN) {
		// return/break/... 语句引起 循环变量销毁造成的异常 逻辑上会在foreach循环的结抛出，所以调整 throw_op_num
		/* exceptions thrown because of loop var destruction on return/break/...
		 * are logically thrown at the end of the foreach loop, so adjust the
		 * throw_op_num.
		 */
		// 查找活动区域
		const zend_live_range *range = find_live_range(
			&EX(func)->op_array, throw_op_num, throw_op->op1.var);
		
		// 释放 相应的 RETURN 的op1
		/* free op1 of the corresponding RETURN */
		for (i = throw_op_num; i < range->end; i++) {
			// 如果操作码是 ZEND_FREE 或 ZEND_FE_FREE
			if (EX(func)->op_array.opcodes[i].opcode == ZEND_FREE
			 || EX(func)->op_array.opcodes[i].opcode == ZEND_FE_FREE) {
				/* pass */
			// 否则
			} else {
				// 如果操作码是 ZEND_RETURN 并且 op1是变量或临时变量
				if (EX(func)->op_array.opcodes[i].opcode == ZEND_RETURN
				 && (EX(func)->op_array.opcodes[i].op1_type & (IS_VAR|IS_TMP_VAR))) {
					// 销毁 op1
					zval_ptr_dtor(EX_VAR(EX(func)->op_array.opcodes[i].op1.var));
				}
				// 跳出
				break;
			}
		}
		// 调整 throw_op_num
		throw_op_num = range->end;
	}

	// 找到抛 异常的 最里层的 try/catch/finally
	/* Find the innermost try/catch/finally the exception was thrown in */
	// 遍历每个 zend_try_catch_element
	for (i = 0; i < EX(func)->op_array.last_try_catch; i++) {
		// 每个 zend_try_catch_element 元素
		zend_try_catch_element *try_catch = &EX(func)->op_array.try_catch_array[i];
		// 如果 找到了
		if (try_catch->try_op > throw_op_num) {
			// 再里面的块 就和它无关了
			/* further blocks will not be relevant... */
			break;
		}
		// 如果找到的是cache段
		if (throw_op_num < try_catch->catch_op || throw_op_num < try_catch->finally_end) {
			// 更新序号
			current_try_catch_offset = i;
		}
	}

	// 清理未完成的调用
	cleanup_unfinished_calls(execute_data, throw_op_num);

	// 如果结果是 变量或 临时变量
	if (throw_op->result_type & (IS_VAR | IS_TMP_VAR)) {
		// 根据操作码操作
		switch (throw_op->opcode) {
			//  这几个跳过
			case ZEND_ADD_ARRAY_ELEMENT:
			case ZEND_ADD_ARRAY_UNPACK:
			case ZEND_ROPE_INIT:
			case ZEND_ROPE_ADD:
				// 异常 会建立一些实例，活动区域处理器会释放它们
				break; /* exception while building structures, live range handling will free those */

			// 跳过
			case ZEND_FETCH_CLASS:
			case ZEND_DECLARE_ANON_CLASS:
				// 返回值是 类入口指针
				break; /* return value is zend_class_entry pointer */

			// 其他情况
			default:
				// 智能分支可能没有初始结果 
				/* smart branch opcodes may not initialize result */
				// 如果没有智能分支（一些用作判断的操作码）
				if (!zend_is_smart_branch(throw_op)) {
					// 销毁没有引用次数的对象,并没有销毁 zval本身，不放入gc回收周期。
					zval_ptr_dtor_nogc(EX_VAR(throw_op->result.var));
				}
		}
	}

	// 分发 try/catch/finally 助手
	// 调用方法 zend_dispatch_try_catch_finally_helper
	ZEND_VM_DISPATCH_TO_HELPER(zend_dispatch_try_catch_finally_helper, try_catch_offset, current_try_catch_offset, op_num, throw_op_num);
}

// ing2, 处理用户操作码
ZEND_VM_HANDLER(150, ZEND_USER_OPCODE, ANY, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	int ret;

	// windows: 无操作
	SAVE_OPLINE();
	// 取得 并调用 操作码的处理器
	ret = zend_user_opcode_handlers[opline->opcode](execute_data);
	// 当前操作码
	opline = EX(opline);

	// 根据返回值操作
	switch (ret) {
		// 继续操作
		case ZEND_USER_OPCODE_CONTINUE:
			// windows: return  0
			ZEND_VM_CONTINUE();
		// 返回
		case ZEND_USER_OPCODE_RETURN:
			// 如果调用了生成器
			if (UNEXPECTED((EX_CALL_INFO() & ZEND_CALL_GENERATOR) != 0)) {
				// 取得生成器
				zend_generator *generator = zend_get_running_generator(EXECUTE_DATA_C);
				// 关闭生成器
				zend_generator_close(generator, 1);
				// windows: return -1
				ZEND_VM_RETURN();
			// 否则
			} else {
				// 调用方法 zend_leave_helper
				ZEND_VM_DISPATCH_TO_HELPER(zend_leave_helper);
			}
		// 进入
		case ZEND_USER_OPCODE_ENTER:
			// windows return 1;
			ZEND_VM_ENTER();
		// 离开
		case ZEND_USER_OPCODE_LEAVE:
			// windows return 2;
			ZEND_VM_LEAVE();
		// 分派
		case ZEND_USER_OPCODE_DISPATCH:
			// 分派，处理操作码并return
			ZEND_VM_DISPATCH(opline->opcode, opline);
		// 默认情况
		default:
			// 分派，处理操作码并return 
			// 返回值是255内
			ZEND_VM_DISPATCH((zend_uchar)(ret & 0xff), opline);
	}
}

// ing3, 声名常量
ZEND_VM_HANDLER(143, ZEND_DECLARE_CONST, CONST, CONST)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *name;
	zval *val;
	zend_constant c;

	// windows: 无操作
	SAVE_OPLINE();
	// 获取zval指针，UNUSED 类型返回null
	name  = GET_OP1_ZVAL_PTR(BP_VAR_R);
	// 获取zval指针，UNUSED 类型返回null
	val   = GET_OP2_ZVAL_PTR(BP_VAR_R);

	// 值放到常量里
	ZVAL_COPY(&c.value, val);
	// 如果值为常量语句
	if (Z_OPT_CONSTANT(c.value)) {
		// 更新此变量
		if (UNEXPECTED(zval_update_constant_ex(&c.value, EX(func)->op_array.scope) != SUCCESS)) {
			// 销毁没有引用次数的对象,并没有销毁 zval本身，不放入gc回收周期。
			zval_ptr_dtor_nogc(&c.value);
			// 释放操作对象的附加变量
			FREE_OP1();
			// 释放操作对象的附加变量
			FREE_OP2();
			// 主要是 return
			HANDLE_EXCEPTION();
		}
	}
	
	// 非持久的，大小写敏感
	/* non persistent, case sensitive */
	//  写入 constant_flags ，p1:常量，p2:_flags，p3:模块编号
	ZEND_CONSTANT_SET_FLAGS(&c, CONST_CS, PHP_USER_CONSTANT);
	// 变量名，增加引用次数. 这个和 ZVAL_COPY 不同
	c.name = zend_string_copy(Z_STR_P(name));

	// 注册常量，失败也不报错
	if (zend_register_constant(&c) == FAILURE) {
	}

	// 释放操作对象的附加变量
	FREE_OP1();
	// 释放操作对象的附加变量
	FREE_OP2();
	// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, 使用已有函数或方法，创建闭包
ZEND_VM_HANDLER(142, ZEND_DECLARE_LAMBDA_FUNCTION, CONST, NUM)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zend_function *func;
	zval *object;
	zend_class_entry *called_scope;
	// 通过编号找到函数
	func = (zend_function *) EX(func)->op_array.dynamic_func_defs[opline->op2.num];
	// 如果$this是对象
	if (Z_TYPE(EX(This)) == IS_OBJECT) {
		// 调用域
		called_scope = Z_OBJCE(EX(This));
		// 如果方法是static 或 当前正处于静态方法中
		if (UNEXPECTED((func->common.fn_flags & ZEND_ACC_STATIC) ||
				(EX(func)->common.fn_flags & ZEND_ACC_STATIC))) {
			// 不需要对象
			object = NULL;
		// 否则，普通方法
		} else {
			// 记录 $this
			object = &EX(This);
		}
	// 否则, $this不是对象
	} else {
		// 记录 $this
		called_scope = Z_CE(EX(This));
		// 没有对象
		object = NULL;
	}
	// 创建闭包，放在结果里
	zend_create_closure(EX_VAR(opline->result.var), func,
		EX(func)->op_array.scope, called_scope, object);

	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, 分隔， 1/1，如果op1中的变量是引用，并且引用次数是1，转到引用目标（解除引用）
// ZEND_FASTCALL ZEND_SEPARATE_SPEC，1个分支（验证过）
ZEND_VM_HANDLER(156, ZEND_SEPARATE, VAR, UNUSED)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *var_ptr;

	// 在执行数据中取出 指定序号的变量
	var_ptr = EX_VAR(opline->op1.var);
	// 如果op1中的变量是引用
	if (UNEXPECTED(Z_ISREF_P(var_ptr))) {
		// 并且引用次数是1
		if (UNEXPECTED(Z_REFCOUNT_P(var_ptr) == 1)) {
			// 转到引用目标
			ZVAL_UNREF(var_ptr);
		}
	}

	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, 报错：不可以在一个强制关闭的生成器用 yield
ZEND_VM_COLD_HELPER(zend_yield_in_closed_generator_helper, ANY, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// windows: 无操作
	SAVE_OPLINE();
	// 报错：不可以在一个强制关闭的生成器用 yield
	zend_throw_error(NULL, "Cannot yield from finally in a force-closed generator");
	// 释放操作对象的附加变量
	FREE_OP2();
	// 释放操作对象的附加变量
	FREE_OP1();
	// 如果 opline 结果类型是 变量或临时变量，把结果 zval 标记为未定义
	UNDEF_RESULT();
	// 主要是 return
	HANDLE_EXCEPTION();
}

// ing2, yield语句
ZEND_VM_HANDLER(160, ZEND_YIELD, CONST|TMP|VAR|CV|UNUSED, CONST|TMPVAR|CV|UNUSED, SRC)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// 正在运行的生成器
	zend_generator *generator = zend_get_running_generator(EXECUTE_DATA_C);

	// windows: 无操作
	SAVE_OPLINE();
	// 如果生成器有强制关闭标记
	if (UNEXPECTED(generator->flags & ZEND_GENERATOR_FORCED_CLOSE)) {
		// 报错：不可以在一个强制关闭的生成器用 yield
		// 调用方法 zend_yield_in_closed_generator_helper
		ZEND_VM_DISPATCH_TO_HELPER(zend_yield_in_closed_generator_helper);
	}

	// 销毁前面生成的 值
	/* Destroy the previously yielded value */
	zval_ptr_dtor(&generator->value);

	// 销毁前面生成的 键
	/* Destroy the previously yielded key */
	zval_ptr_dtor(&generator->key);

	// 设置新的 yield 值
	/* Set the new yielded value */
	if (OP1_TYPE != IS_UNUSED) {
		// 函数要求引用返回
		if (UNEXPECTED(EX(func)->op_array.fn_flags & ZEND_ACC_RETURN_REFERENCE)) {
			// 常量和临时变量可以被通过引用  yield，但仍然需要发出提示
			/* Constants and temporary variables aren't yieldable by reference,
			 * but we still allow them with a notice. */
			// 如果op1 是常量或临时变量
			if (OP1_TYPE & (IS_CONST|IS_TMP_VAR)) {
				//
				zval *value;
				// 提示：只有变量的引用 可以 通过引用 yield.
				zend_error(E_NOTICE, "Only variable references should be yielded by reference");
				// 获取zval指针，UNUSED 类型返回null
				value = GET_OP1_ZVAL_PTR(BP_VAR_R);
				// op1复制给 生成器的值
				ZVAL_COPY_VALUE(&generator->value, value);
				// 如果op1是常量
				if (OP1_TYPE == IS_CONST) {
					// 如果生成器的值可计数
					if (UNEXPECTED(Z_OPT_REFCOUNTED(generator->value))) {
						// 增加引用次数
						Z_ADDREF(generator->value);
					}
				}
			// 否则
			} else {
				// 获取zval指针,TMP/CONST/UNUSED 返回null，不支持 TMPVAR/TMPVARCV
				zval *value_ptr = GET_OP1_ZVAL_PTR_PTR(BP_VAR_W);

				// 如果一个函数调用的结果 被 yield 并且 函数并没有通过引用返回，需要抛出提示
				/* If a function call result is yielded and the function did
				 * not return by reference we throw a notice. */
				// 为了break;
				do {
					// 如果 op1 是 普通变量
					if (OP1_TYPE == IS_VAR) {
						// 断言，op1不可以是未初始化zval
						ZEND_ASSERT(value_ptr != &EG(uninitialized_zval));
						// 如果扩展值是 ZEND_RETURNS_FUNCTION 并且 op1不是引用类型
						if (opline->extended_value == ZEND_RETURNS_FUNCTION
						 && !Z_ISREF_P(value_ptr)) {
							// 提示：只有变量的引用 可以 通过引用 yield.
							zend_error(E_NOTICE, "Only variable references should be yielded by reference");
							// op1复制给生成器的值
							ZVAL_COPY(&generator->value, value_ptr);
							// 跳出
							break;
						}
					}
					// 如果 op1 是引用类型
					if (Z_ISREF_P(value_ptr)) {
						// 引用次数+1
						Z_ADDREF_P(value_ptr);
					// 否则，op1不是引用类型
					} else {
						// 更新成引用类型，引用次数2
						ZVAL_MAKE_REF_EX(value_ptr, 2);
					}
					// op1的引用目标，引用赋值给 生成器的值
					ZVAL_REF(&generator->value, Z_REF_P(value_ptr));
				} while (0);

				// 释放操作对象的附加变量
				FREE_OP1();
			}
		// 否则
		} else {
			// 获取zval指针，UNUSED 类型返回null
			zval *value = GET_OP1_ZVAL_PTR(BP_VAR_R);

			// 常量，临时变量，引用 都需要复制
			/* Consts, temporary variables and references need copying */
			// 如果op1是常量
			if (OP1_TYPE == IS_CONST) {
				// op1复制给生成器的值
				ZVAL_COPY_VALUE(&generator->value, value);
				// 如果 生成器的值是可计数
				if (UNEXPECTED(Z_OPT_REFCOUNTED(generator->value))) {
					// 增加引用次数
					Z_ADDREF(generator->value);
				}
			// op1是临时变量
			} else if (OP1_TYPE == IS_TMP_VAR) {
				// op1复制给生成器的值
				ZVAL_COPY_VALUE(&generator->value, value);
			// 如果 op1 是普通变量或编译变量 并且 op1是引用类型
			} else if ((OP1_TYPE & (IS_VAR|IS_CV)) && Z_ISREF_P(value)) {
				// op1的引用目标 复制给生成器的值，并增加引用次数？
				ZVAL_COPY(&generator->value, Z_REFVAL_P(value));
				// 只释放类型为 IS_VAR 的变量
				FREE_OP1_IF_VAR();
			// 否则
			} else {
				// op1 复制给生成器的值
				ZVAL_COPY_VALUE(&generator->value, value);
				// 如果 op1 是 编译变量
				if (OP1_TYPE == IS_CV) {
					// 如果是可以计数类型
					// 引用次数+1
					if (Z_OPT_REFCOUNTED_P(value)) Z_ADDREF_P(value);
				}
			}
		}
	// 否则
	} else {
		// 如果没有指定值，yield null
		/* If no value was specified yield null */
		// 生成器的值为null
		ZVAL_NULL(&generator->value);
	}

	// 设置新生成的key
	/* Set the new yielded key */
	// 如果op2有效
	if (OP2_TYPE != IS_UNUSED) {
		// 获取zval指针，UNUSED 类型返回null
		zval *key = GET_OP2_ZVAL_PTR(BP_VAR_R);
		// 如果op2是编译变量或变量 并且 op2是引用类型
		if ((OP2_TYPE & (IS_CV|IS_VAR)) && UNEXPECTED(Z_TYPE_P(key) == IS_REFERENCE)) {
			// 解引用
			key = Z_REFVAL_P(key);
		}
		// key复制给生成器
		ZVAL_COPY(&generator->key, key);
		// 释放操作对象的附加变量
		FREE_OP2();

		// 如果key是整数 并且 key 大于 最后使用的整数 key
		if (Z_TYPE(generator->key) == IS_LONG
		    && Z_LVAL(generator->key) > generator->largest_used_integer_key
		) {
			// 更新最后使用的整数key
			generator->largest_used_integer_key = Z_LVAL(generator->key);
		}
	// 否则
	} else {
		// 如果没有指定key，使用自增key
		/* If no key was specified we use auto-increment keys */
		generator->largest_used_integer_key++;
		// 最后一个整数key 复制给生成器
		ZVAL_LONG(&generator->key, generator->largest_used_integer_key);
	}

	// 如果 操作码的运算结果有效(不是IS_UNUSED)
	if (RETURN_VALUE_USED(opline)) {
		// 如果yield的返回值已经被使用，设置发送目标，并初始化成null
		/* If the return value of yield is used set the send
		 * target and initialize it to NULL */
		 // 在执行数据中取出 指定序号的变量
		generator->send_target = EX_VAR(opline->result.var);
		// 发送目标为 null 类型
		ZVAL_NULL(generator->send_target);
	// 否则
	} else {
		// 发送目标为 null
		generator->send_target = NULL;
	}

	// 自增找到下一个操作码，所以当生成器恢复的时候，就到正确的位置了。
	/* We increment to the next op, so we are at the correct position when the
	 * generator is resumed. */
	// OPLINE++
	ZEND_VM_INC_OPCODE();

	// GOTO VM 使用一个本地操作码变量。需要把它放到 执行数据中 ，所以没有在旧位置恢复。
	/* The GOTO VM uses a local opline variable. We need to set the opline
	 * variable in execute_data so we don't resume at an old position. */
	// windows: 无操作
	SAVE_OPLINE();
	// windows: return -1
	ZEND_VM_RETURN();
}

// ing2, yield from 语句
ZEND_VM_HANDLER(166, ZEND_YIELD_FROM, CONST|TMPVAR|CV, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	// 取出正在运行的生成器
	zend_generator *generator = zend_get_running_generator(EXECUTE_DATA_C);
	zval *val;

	// windows: 无操作
	SAVE_OPLINE();
	// 获取zval指针，UNUSED 类型返回null
	val = GET_OP1_ZVAL_PTR(BP_VAR_R);

	if (UNEXPECTED(generator->flags & ZEND_GENERATOR_FORCED_CLOSE)) {
		zend_throw_error(NULL, "Cannot use \"yield from\" in a force-closed generator");
		// 释放操作对象的附加变量
		FREE_OP1();
		// 如果 opline 结果类型是 变量或临时变量，把结果 zval 标记为未定义
		UNDEF_RESULT();
		// 主要是 return
		HANDLE_EXCEPTION();
	}

// 创建跳转标签 yield_from_try_again
ZEND_VM_C_LABEL(yield_from_try_again):
	// 如果op1是数组
	if (Z_TYPE_P(val) == IS_ARRAY) {
		// op1作为生成器的值表
		ZVAL_COPY_VALUE(&generator->values, val);
		// 如果是可以计数类型
		if (Z_OPT_REFCOUNTED_P(val)) {
			// 引用次数+1
			Z_ADDREF_P(val);
		}
		// 值哈希表的遍历位置归0
		Z_FE_POS(generator->values) = 0;
		// 释放操作对象的附加变量
		FREE_OP1();
	// 如果op1不是常量 并且 op1是对象 并且 有获取迭代器方法
	} else if (OP1_TYPE != IS_CONST && Z_TYPE_P(val) == IS_OBJECT && Z_OBJCE_P(val)->get_iterator) {
		// 取出 op1的类
		zend_class_entry *ce = Z_OBJCE_P(val);
		// 如果类是 标准生成器
		if (ce == zend_ce_generator) {
			// op1转成生成器实例
			zend_generator *new_gen = (zend_generator *) Z_OBJ_P(val);
			// 引用次数+1
			Z_ADDREF_P(val);
			// 释放操作对象的附加变量
			FREE_OP1();

			// 如果生成器没有执行数据
			if (UNEXPECTED(new_gen->execute_data == NULL)) {
				// 报错：传递给 yield from 的生成器没有正常返回，无法继续操作
				zend_throw_error(NULL, "Generator passed to yield from was aborted without proper return and is unable to continue");
				// 销毁 op1
				zval_ptr_dtor(val);
				// 如果 opline 结果类型是 变量或临时变量，把结果 zval 标记为未定义
				UNDEF_RESULT();
				// 主要是 return
				HANDLE_EXCEPTION();
			// 如果生成器有返回值
			} else if (Z_ISUNDEF(new_gen->retval)) {
				// 如果 此生成器正在运行
				if (UNEXPECTED(zend_generator_get_current(new_gen) == generator)) {
					// 抛错：不可以 yield from 正在运行的生成器
					zend_throw_error(NULL, "Impossible to yield from the Generator being currently run");
					// 销毁 op1
					zval_ptr_dtor(val);
					// 如果 opline 结果类型是 变量或临时变量，把结果 zval 标记为未定义
					UNDEF_RESULT();
					// 主要是 return
					HANDLE_EXCEPTION();
				// 否则
				} else {
					// 在生成器树中用 p2代替p1,并把p1作为p2的子节点
					zend_generator_yield_from(generator, new_gen);
				}
			// 否则
			} else {
				// 如果 操作码的运算结果有效(不是IS_UNUSED)
				if (RETURN_VALUE_USED(opline)) {
					// 生成器返回值 复制给 结果
					ZVAL_COPY(EX_VAR(opline->result.var), &new_gen->retval);
				}
				// opline + 1，到目标操作码并 return
				ZEND_VM_NEXT_OPCODE();
			}
		// 否则
		} else {
			// 获取对象的第一个迭代器
			zend_object_iterator *iter = ce->get_iterator(ce, val, 0);
			// 释放操作对象的附加变量
			FREE_OP1();

			// 如果没有迭代器 或 有异常
			if (UNEXPECTED(!iter) || UNEXPECTED(EG(exception))) {
				// 如果没有异常
				if (!EG(exception)) {
					// 报错：此对象无法创建迭代器
					zend_throw_error(NULL, "Object of type %s did not create an Iterator", ZSTR_VAL(ce->name));
				}
				// 如果 opline 结果类型是 变量或临时变量，把结果 zval 标记为未定义
				UNDEF_RESULT();
				// 主要是 return
				HANDLE_EXCEPTION();
			}
			// 迭代器指向 fe_reset/fe_fetch 的私有指针（整数）
			// 指向0
			iter->index = 0;
			// 如果有重置方法
			if (iter->funcs->rewind) {
				// 重置
				iter->funcs->rewind(iter);
				// 如果有异常
				if (UNEXPECTED(EG(exception) != NULL)) {
					// 清空迭代器内置zval
					OBJ_RELEASE(&iter->std);
					// 如果 opline 结果类型是 变量或临时变量，把结果 zval 标记为未定义
					UNDEF_RESULT();
					// 主要是 return
					HANDLE_EXCEPTION();
				}
			}
			// 把迭代器内置zval当成生成器的值表
			ZVAL_OBJ(&generator->values, &iter->std);
		}
	// 如果 op1 是普通变量或编译变量 并且 op1是引用类型
	} else if ((OP1_TYPE & (IS_VAR|IS_CV)) && Z_TYPE_P(val) == IS_REFERENCE) {
		// 解引用
		val = Z_REFVAL_P(val);
		// 跳转到指定标签 yield_from_try_again
		ZEND_VM_C_GOTO(yield_from_try_again);
	// 否则
	} else {
		// 抛错：只可以对 数组和可遍历对象使用 yield from
		zend_throw_error(NULL, "Can use \"yield from\" only with arrays and Traversables");
		// 释放操作对象的附加变量
		FREE_OP1();
		// 如果 opline 结果类型是 变量或临时变量，把结果 zval 标记为未定义
		UNDEF_RESULT();
		// 主要是 return
		HANDLE_EXCEPTION();
	}

	// 这是默认返回值，当表达式是生成器，它会在 zend_generator_resume 中被覆盖
	/* This is the default return value
	 * when the expression is a Generator, it will be overwritten in zend_generator_resume() */
	// 如果 操作码的运算结果有效(不是IS_UNUSED)
	if (RETURN_VALUE_USED(opline)) {
		// 结果 为null
		ZVAL_NULL(EX_VAR(opline->result.var));
	}

	// 此生成器没有发送目标（尽管被委派的生成器可能有目标）
	/* This generator has no send target (though the generator we delegate to might have one) */
	generator->send_target = NULL;

	// 自增找到下一个操作码，所以当生成器恢复的时候，就到正确的位置了。
	/* We increment to the next op, so we are at the correct position when the
	 * generator is resumed. */
	 
	// OPLINE++
	ZEND_VM_INC_OPCODE();

	// GOTO VM 使用一个本地操作码变量。需要把它放到 执行数据中 ，所以没有在旧位置恢复。
	/* The GOTO VM uses a local opline variable. We need to set the opline
	 * variable in execute_data so we don't resume at an old position. */
	// windows: 无操作
	SAVE_OPLINE();
	// windows: return -1
	ZEND_VM_RETURN();
}

// ing3, 抛弃前面产生的异常
ZEND_VM_HANDLER(159, ZEND_DISCARD_EXCEPTION, ANY, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	// 在执行数据中取出 指定序号的变量
	zval *fast_call = EX_VAR(opline->op1.var);
	// windows: 无操作
	SAVE_OPLINE();

	// 清理未完成的 return 语句
	/* cleanup incomplete RETURN statement */
	
	// 如果操作码有效 并且 操作码的op2是 （临时变量 或 变量）
	// 通过指针访问zval的 操作码序号
	if (Z_OPLINE_NUM_P(fast_call) != (uint32_t)-1
	 && (EX(func)->op_array.opcodes[Z_OPLINE_NUM_P(fast_call)].op2_type & (IS_TMP_VAR | IS_VAR))) {
		// 取得目标操作码的op2的变量（op2）
		// 在执行数据中取出 指定序号的变量
		// 通过指针访问zval的 操作码序号
		zval *return_value = EX_VAR(EX(func)->op_array.opcodes[Z_OPLINE_NUM_P(fast_call)].op2.var);
		// 销毁没有引用次数的对象，有引用次数的添加到gc回收周期里。并没有销毁 zval本身。
		zval_ptr_dtor(return_value);
	}

	// 清理延时异常
	/* cleanup delayed exception */
	// 如果目标对象存在
	if (Z_OBJ_P(fast_call) != NULL) {
		// 放弃之前抛出的异常
		/* discard the previously thrown exception */
		// 释放目标对象
		OBJ_RELEASE(Z_OBJ_P(fast_call));
		// 指针置空
		Z_OBJ_P(fast_call) = NULL;
	}

	// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, 快速调用, 直接跳转到目标操作码
ZEND_VM_HANDLER(162, ZEND_FAST_CALL, JMP_ADDR, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	// 在执行数据中取出 指定序号的变量
	zval *fast_call = EX_VAR(opline->result.var);

	// 对象指针设置为空
	Z_OBJ_P(fast_call) = NULL;
	
	// 设置返回地址，当前操作码编号
	/* set return address */
	// 通过指针访问zval的 操作码序号
	Z_OPLINE_NUM_P(fast_call) = opline - EX(func)->op_array.opcodes;
	// 跳转到p2指定的操作码
	// 要检查异常 并 有异常时使用旧操作码，无异常使用新操作码, p1:新操作码，p2:是否检查异常
	// OP_JMP_ADDR: 访问 p2.jmp_addr
	ZEND_VM_JMP_EX(OP_JMP_ADDR(opline, opline->op1), 0);
}

// ing2,
ZEND_VM_HANDLER(163, ZEND_FAST_RET, ANY, TRY_CATCH)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	// 在执行数据中取出 指定序号的变量
	zval *fast_call = EX_VAR(opline->op1.var);
	// 
	uint32_t current_try_catch_offset, current_op_num;

	// 通过指针访问zval的 操作码序号
	if (Z_OPLINE_NUM_P(fast_call) != (uint32_t)-1) {
		// 通过指针访问zval的 操作码序号
		const zend_op *fast_ret = EX(func)->op_array.opcodes + Z_OPLINE_NUM_P(fast_call);
		// 要检查异常 并 有异常时使用旧操作码，无异常使用新操作码, p1:新操作码，p2:是否检查异常
		ZEND_VM_JMP_EX(fast_ret + 1, 0);
	}
	// 对于未捕获异常的特殊处理
	/* special case for unhandled exceptions */
	// op1作为当前异常
	EG(exception) = Z_OBJ_P(fast_call);
	// 清空op1
	Z_OBJ_P(fast_call) = NULL;
	// 当前 try/catch 序号 op2的值
	current_try_catch_offset = opline->op2.num;
	// 当前操作码序号
	current_op_num = opline - EX(func)->op_array.opcodes;
	
	// 分发 try/catch/finally 助手
	// 调用方法 zend_dispatch_try_catch_finally_helper
	ZEND_VM_DISPATCH_TO_HELPER(zend_dispatch_try_catch_finally_helper, try_catch_offset, current_try_catch_offset, op_num, current_op_num);
}

// ing3, 绑定全局变量 EG(symbol_table) 表
ZEND_VM_HOT_HANDLER(168, ZEND_BIND_GLOBAL, CV, CONST, CACHE_SLOT)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zend_string *varname;
	zval *value;
	zval *variable_ptr;
	uintptr_t idx;
	zend_reference *ref;

	ZEND_VM_REPEATABLE_OPCODE
	// 获取zval指针，UNUSED 类型返回null
	varname = Z_STR_P(GET_OP2_ZVAL_PTR(BP_VAR_R));

	/* We store "hash slot index" + 1 (NULL is a mark of uninitialized cache slot) */
	// 通过偏移量 在运行时缓存中获取一个 (void**) 中的第一个元素
	// 然后把指针转成整数 ，-1
	idx = (uintptr_t)CACHED_PTR(opline->extended_value) - 1;
	// 如果位置在已使用范围内
	if (EXPECTED(idx < EG(symbol_table).nNumUsed * sizeof(Bucket))) {
		// 取出 bucket
		Bucket *p = (Bucket*)((char*)EG(symbol_table).arData + idx);
		// 如果key和变量名指针相同 或（哈希值和变量名相符 并且 有key 并且key相同）
		if (EXPECTED(p->key == varname) ||
		    (EXPECTED(p->h == ZSTR_H(varname)) &&
		     EXPECTED(p->key != NULL) &&
		     EXPECTED(zend_string_equal_content(p->key, varname)))) {
			// 使用找到的值
			value = (zval*)p; /* value = &p->val; */
			
			// 跳转到指定标签 check_indirect
			ZEND_VM_C_GOTO(check_indirect);
		}
	}

	// 使用变量名在哈希表里查找 
	value = zend_hash_find_known_hash(&EG(symbol_table), varname);
	// 如果没找到
	if (UNEXPECTED(value == NULL)) {
		// 添加此变量，值为未定义变量
		value = zend_hash_add_new(&EG(symbol_table), varname, &EG(uninitialized_zval));
		// 取得这个bucket的位置
		idx = (char*)value - (char*)EG(symbol_table).arData;
		// 保存 “哈希位置序号”+1 （null是未初始化缓存位置的标记）
		/* Store "hash slot index" + 1 (NULL is a mark of uninitialized cache slot) */
		// 通过偏移量 在 EX(run_time_cache) 中获取一个 (void**) 中的第一个元素，并给它赋值
		CACHE_PTR(opline->extended_value, (void*)(idx + 1));
	// 否则，如果找到了
	} else {
		// 取得这个bucket的位置
		idx = (char*)value - (char*)EG(symbol_table).arData;
		// 保存 “哈希位置序号”+1 （null是未初始化缓存位置的标记）
		/* Store "hash slot index" + 1 (NULL is a mark of uninitialized cache slot) */
		// 通过偏移量 在 EX(run_time_cache) 中获取一个 (void**) 中的第一个元素，并给它赋值
		CACHE_PTR(opline->extended_value, (void*)(idx + 1));
// 创建跳转标签 check_indirect
ZEND_VM_C_LABEL(check_indirect):
		// 全局变量可能是一个 间接指针指向 编译变量
		/* GLOBAL variable may be an INDIRECT pointer to CV */
		if (UNEXPECTED(Z_TYPE_P(value) == IS_INDIRECT)) {
			// 解间接引用
			value = Z_INDIRECT_P(value);
			// 如果值是未定义
			if (UNEXPECTED(Z_TYPE_P(value) == IS_UNDEF)) {
				// 值为null
				ZVAL_NULL(value);
			}
		}
	}

	// 如果值不是引用类型
	if (UNEXPECTED(!Z_ISREF_P(value))) {
		// 更新成引用类型，引用次数2
		ZVAL_MAKE_REF_EX(value, 2);
		// 引用对象
		ref = Z_REF_P(value);
	// 否则
	} else {
		// 取出引用对象
		ref = Z_REF_P(value);
		// 增加引用次数
		GC_ADDREF(ref);
	}
	// 获取zval指针, TMP/CONST/UNUSED 返回null，不支持 TMPVAR/TMPVARCV
	variable_ptr = GET_OP1_ZVAL_PTR_PTR_UNDEF(BP_VAR_W);

	// 如果 op1 是 可计数类型
	if (UNEXPECTED(Z_REFCOUNTED_P(variable_ptr))) {
		// 取得回收指针
		zend_refcounted *garbage = Z_COUNTED_P(variable_ptr);
		// op1 转成引用类型
		ZVAL_REF(variable_ptr, ref);
		// windows: 无操作
		SAVE_OPLINE();
		// 引用次数-1，如果为0
		if (GC_DELREF(garbage) == 0) {
			// 调用每个type对应的销毁函数执行销毁
			rc_dtor_func(garbage);
			// 如果有异常
			if (UNEXPECTED(EG(exception))) {
				// 赋值为null
				ZVAL_NULL(variable_ptr);
				// 主要是 return
				HANDLE_EXCEPTION();
			}
		// 否则，-1后还有次数
		} else {
			// 添加到垃圾回收
			gc_check_possible_root(garbage);
		}
	// 否则，不是可计数类型
	} else {
		// 直接引用赋值给  op1
		ZVAL_REF(variable_ptr, ref);
	}
	// while() 循环结尾段，找下一个操作码，直到碰到p1操作码
	ZEND_VM_REPEAT_OPCODE(ZEND_BIND_GLOBAL);
	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, strlen函数
ZEND_VM_COLD_CONST_HANDLER(121, ZEND_STRLEN, CONST|TMPVAR|CV, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *value;

	// 获取zval指针, UNUSED 返回null
	value = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 如果类型是 字串
	if (EXPECTED(Z_TYPE_P(value) == IS_STRING)) {
		ZVAL_LONG(EX_VAR(opline->result.var), Z_STRLEN_P(value));
		// 如果 op1 类型 是变量或临时变量
		if (OP1_TYPE & (IS_TMP_VAR|IS_VAR)) {
			// 通过指针销毁 zval（字串）
			zval_ptr_dtor_str(value);
		}
		// opline + 1，到目标操作码并 return
		ZEND_VM_NEXT_OPCODE();
	// 否则
	} else {
		bool strict;
		// 如果 op1是 （普通变量 或 编译变量） 并且值是引用型
		if ((OP1_TYPE & (IS_VAR|IS_CV)) && Z_TYPE_P(value) == IS_REFERENCE) {
			// 解引用
			value = Z_REFVAL_P(value);
			// 如果类型是 字串
			if (EXPECTED(Z_TYPE_P(value) == IS_STRING)) {
				// 把长度转成整数 放在结果 里
				ZVAL_LONG(EX_VAR(opline->result.var), Z_STRLEN_P(value));
				// 释放操作对象的附加变量
				FREE_OP1();
				// opline + 1，到目标操作码并 return
				ZEND_VM_NEXT_OPCODE();
			}
		}

		// windows: 无操作
		SAVE_OPLINE();
		// 如果 op1 是 编译变量 并且值为 未定义
		if (OP1_TYPE == IS_CV && UNEXPECTED(Z_TYPE_P(value) == IS_UNDEF)) {
			// 报错：p1的变量（变量名）未定义, 并返未初始化zval
			value = ZVAL_UNDEFINED_OP1();
		}
		// 某执行上下文所属函数 的参数是否使用严格类型限制
		strict = EX_USES_STRICT_TYPES();
		//
		do {
			// 如果不是严格类型
			if (EXPECTED(!strict)) {
				// 临时变量
				zend_string *str;
				zval tmp;
				// 如果值是null
				if (UNEXPECTED(Z_TYPE_P(value) == IS_NULL)) {
					// 报错：给strlen传入null，是已弃用的用法
					zend_error(E_DEPRECATED,
						"strlen(): Passing null to parameter #1 ($string) of type string is deprecated");
					// 结果为0
					ZVAL_LONG(EX_VAR(opline->result.var), 0);
					// 如果有异常
					if (UNEXPECTED(EG(exception))) {
						// 主要是 return
						HANDLE_EXCEPTION();
					}
					// 跳出
					break;
				}
				
				// 如果值不是null
				// 先复制到临时变量里
				ZVAL_COPY(&tmp, value);
				// 把传入的参数转换成字串，遇到null会报错。p2:返回值，p3:arg参数序号，报错用
				if (zend_parse_arg_str_weak(&tmp, &str, 1)) {
					// 为转换后的字串计算长度
					ZVAL_LONG(EX_VAR(opline->result.var), ZSTR_LEN(str));
					// 销毁临时字串
					zval_ptr_dtor(&tmp);
					// 跳出
					break;
				}
				// 销毁临时字串
				zval_ptr_dtor(&tmp);
			}
			// 如果没有异常
			if (!EG(exception)) {
				// 报错： strlen第一个参数必须是字串类型，但给了A类型
				zend_type_error("strlen(): Argument #1 ($string) must be of type string, %s given", zend_zval_type_name(value));
			}
			// 结果为 未定义
			ZVAL_UNDEF(EX_VAR(opline->result.var));
		} while (0);
	}
	// 释放操作对象的附加变量
	FREE_OP1();
	// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, 检验类型
ZEND_VM_HOT_NOCONST_HANDLER(123, ZEND_TYPE_CHECK, CONST|TMPVAR|CV, ANY, TYPE_MASK)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *value;
	int result = 0;

	// 获取zval指针, UNUSED 返回null
	value = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 类型为null时 右移1位，如果有效
	if ((opline->extended_value >> (uint32_t)Z_TYPE_P(value)) & 1) {
// 创建跳转标签 type_check_resource
ZEND_VM_C_LABEL(type_check_resource):
		// 如果 不可能是资源类型 或 在资源列表里
		if (opline->extended_value != MAY_BE_RESOURCE
		// 取得资源列表类型，如果有效
		 || EXPECTED(NULL != zend_rsrc_list_get_rsrc_type(Z_RES_P(value)))) {
			// 结果为1
			result = 1;
		}
	// 如果op1 是编译变量 或 普通变量 并且是 引用类型
	} else if ((OP1_TYPE & (IS_CV|IS_VAR)) && Z_ISREF_P(value)) {
		// 解引用
		value = Z_REFVAL_P(value);
		// 如果是null，右移1位
		if ((opline->extended_value >> (uint32_t)Z_TYPE_P(value)) & 1) {
			// 跳转到指定标签 type_check_resource
			ZEND_VM_C_GOTO(type_check_resource);
		}
	// 如果 op1 是 编译变量 并且 值为未定义
	} else if (OP1_TYPE == IS_CV && UNEXPECTED(Z_TYPE_P(value) == IS_UNDEF)) {
		result = ((1 << IS_NULL) & opline->extended_value) != 0;
		// windows: 无操作
		SAVE_OPLINE();
		// 报错：p1的变量（变量名）未定义, 并返未初始化zval
		ZVAL_UNDEFINED_OP1();
		// 如果有异常
		if (UNEXPECTED(EG(exception))) {
			// 结果为 未定义
			ZVAL_UNDEF(EX_VAR(opline->result.var));
			// 主要是 return
			HANDLE_EXCEPTION();
		}
	}
	// 如果 op1 类型 是变量或临时变量
	if (OP1_TYPE & (IS_TMP_VAR|IS_VAR)) {
		// windows: 无操作
		SAVE_OPLINE();
		// 释放操作对象的附加变量
		FREE_OP1();
		// 根据当前操作码结果选择下一个操作码，p1:传入值，主要用来做判断。p2:是否检查异常
		ZEND_VM_SMART_BRANCH(result, 1);
	// 否则
	} else {
		// 根据当前操作码结果选择下一个操作码，p1:传入值，主要用来做判断。p2:是否检查异常
		ZEND_VM_SMART_BRANCH(result, 0);
	}
}

// ing3, defined 函数
ZEND_VM_HOT_HANDLER(122, ZEND_DEFINED, CONST, ANY, CACHE_SLOT)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zend_constant *c;

	// 取出常量
	// 通过偏移量 在运行时缓存中获取一个 (void**) 中的第一个元素
	c = CACHED_PTR(opline->extended_value);
	// 如果 常量有效
	if (EXPECTED(c != NULL)) {
		// 验证地址是否是特殊缓存地址（最后一位是1），如果不是
		if (!IS_SPECIAL_CACHE_VAL(c)) {
// 创建跳转标签 defined_true
ZEND_VM_C_LABEL(defined_true):
			// 根据当前操作码结果选择下一个操作码。如果不是跳转类型，结果赋值为true。
			ZEND_VM_SMART_BRANCH_TRUE();
		// 是特殊缓存地址。如果常量数 == 解码特殊缓存序号，把序号 右移1位
		} else if (EXPECTED(zend_hash_num_elements(EG(zend_constants)) == DECODE_SPECIAL_CACHE_NUM(c))) {
// 创建跳转标签 defined_false
ZEND_VM_C_LABEL(defined_false):
			// 根据当前操作码结果选择下一个操作码。如果不是跳转类型，结果赋值为false。
			ZEND_VM_SMART_BRANCH_FALSE();
		}
	}
	// 快速检测常量存在。p1:常量名，p2:flags
	// 访问 (p2).zv。p1:opline,p2:node
	if (zend_quick_check_constant(RT_CONSTANT(opline, opline->op1) OPLINE_CC EXECUTE_DATA_CC) != SUCCESS) {
		// 通过偏移量 在 EX(run_time_cache) 中获取一个 (void**) 中的第一个元素，并给它赋值
		// 扩展值中保存： 解码特殊缓存序号，把序号 右移1位
		CACHE_PTR(opline->extended_value, ENCODE_SPECIAL_CACHE_NUM(zend_hash_num_elements(EG(zend_constants))));
		// 跳转到指定标签 defined_false
		ZEND_VM_C_GOTO(defined_false);
	// 否则
	} else {
		// 跳转到指定标签 defined_true
		ZEND_VM_C_GOTO(defined_true);
	}
}

//  ing3, 断言， 1/1，如果验证成功，把结果放在 opline 的结果里，然后继续。 
// ZEND_FASTCALL ZEND_ASSERT_CHECK_SPEC，1个分支（验证过）
ZEND_VM_HANDLER(151, ZEND_ASSERT_CHECK, ANY, JMP_ADDR)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	// STD_ZEND_INI_ENTRY(\"zend.assertions\", \"1\", ZEND_INI_ALL, OnUpdateAssertions, assertions, zend_executor_globals, executor_globals)
	// 如果开启了断言功能
	if (EG(assertions) <= 0) {
		//  访问 p2.jmp_addr
		zend_op *target = OP_JMP_ADDR(opline, opline->op2);
		// 如果 操作码的运算结果有效(不是IS_UNUSED)
		if (RETURN_VALUE_USED(opline)) {
			// 结果为ture
			ZVAL_TRUE(EX_VAR(opline->result.var));
		}
		// 要检查异常 并 有异常时使用旧操作码，无异常使用新操作码, p1:新操作码，p2:是否检查异常
		ZEND_VM_JMP_EX(target, 0);
	// 否则，直接到下一个操作码
	// 否则
	} else {
		// opline + 1，到目标操作码并 return
		ZEND_VM_NEXT_OPCODE();
	}
}

// ing3, 获取类名的魔术变量 ::class, self,parent,static
ZEND_VM_HANDLER(157, ZEND_FETCH_CLASS_NAME, CV|TMPVAR|UNUSED|CLASS_FETCH, ANY)
{
	uint32_t fetch_type;
	zend_class_entry *called_scope, *scope;
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	// 如果op1类型为 未定义
	if (OP1_TYPE != IS_UNUSED) {
		// windows: 无操作
		SAVE_OPLINE();
		// 获取zval指针，UNUSED 类型返回null
		zval *op = GET_OP1_ZVAL_PTR(BP_VAR_R);
		// 如果op1不是对象
		if (UNEXPECTED(Z_TYPE_P(op) != IS_OBJECT)) {
			// 如果是引用类型，追踪到被引用的对象
			ZVAL_DEREF(op);
			// 如果还不是对象
			if (Z_TYPE_P(op) != IS_OBJECT) {
				// 不可以在此类型上使用  ::class
				zend_type_error("Cannot use \"::class\" on value of type %s", zend_zval_type_name(op));
				// 结果为 未定义
				ZVAL_UNDEF(EX_VAR(opline->result.var));
				// 释放操作对象的附加变量
				FREE_OP1();
				// 主要是 return
				HANDLE_EXCEPTION();
			}
		}

		ZVAL_STR_COPY(EX_VAR(opline->result.var), Z_OBJCE_P(op)->name);
		// 释放操作对象的附加变量
		FREE_OP1();
		// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
		ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
	}

	// 类型
	fetch_type = opline->op1.num;
	// 当前域
	scope = EX(func)->op_array.scope;
	// 如果当前域是null
	if (UNEXPECTED(scope == NULL)) {
		// windows: 无操作
		SAVE_OPLINE();
		// 报错：不可以在全局域中使用 self/parent/static 
		zend_throw_error(NULL, "Cannot use \"%s\" in the global scope",
			fetch_type == ZEND_FETCH_CLASS_SELF ? "self" :
			fetch_type == ZEND_FETCH_CLASS_PARENT ? "parent" : "static");
		// 结果为 未定义
		ZVAL_UNDEF(EX_VAR(opline->result.var));
		// 主要是 return
		HANDLE_EXCEPTION();
	}

	// 按类型操作
	switch (fetch_type) {
		// self
		case ZEND_FETCH_CLASS_SELF:
			// 直接使用当前 域类名
			ZVAL_STR_COPY(EX_VAR(opline->result.var), scope->name);
			// 跳出
			break;
		// parent
		case ZEND_FETCH_CLASS_PARENT:
			// 如果没有 parent 父类
			if (UNEXPECTED(scope->parent == NULL)) {
				// windows: 无操作
				SAVE_OPLINE();
				// 报错：没有父类的域中不能使用 parent
				zend_throw_error(NULL,
					"Cannot use \"parent\" when current class scope has no parent");
				// 结果为 未定义
				ZVAL_UNDEF(EX_VAR(opline->result.var));
				// 主要是 return
				HANDLE_EXCEPTION();
			}
			// 找到有效域，把名称复制给结果 
			ZVAL_STR_COPY(EX_VAR(opline->result.var), scope->parent->name);
			break;
		// static
		case ZEND_FETCH_CLASS_STATIC:
			// 如果 $this 是对象
			if (Z_TYPE(EX(This)) == IS_OBJECT) {
				// 取出对象
				called_scope = Z_OBJCE(EX(This));
			// 否则
			} else {
				called_scope = Z_CE(EX(This));
			}
			ZVAL_STR_COPY(EX_VAR(opline->result.var), called_scope->name);
			break;
		EMPTY_SWITCH_DEFAULT_CASE()
	}
	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing2, 调用弹跳方法 
ZEND_VM_HANDLER(158, ZEND_CALL_TRAMPOLINE, ANY, ANY, SPEC(OBSERVER))
{
	zend_array *args = NULL;
	zend_function *fbc = EX(func);
	// 执行数据中的返回值
	zval *ret = EX(return_value);
	// 调用信息中添加 嵌套调用，顶层调用，需要释放$this, 包含额外参数 4个标记
	uint32_t call_info = EX_CALL_INFO() & (ZEND_CALL_NESTED | ZEND_CALL_TOP | ZEND_CALL_RELEASE_THIS | ZEND_CALL_HAS_EXTRA_NAMED_PARAMS);
	// 参数数量
	uint32_t num_args = EX_NUM_ARGS();
	zend_execute_data *call;

	// windows: 无操作
	SAVE_OPLINE();

	// 如果有参数
	if (num_args) {
		// 第一个参数
		zval *p = ZEND_CALL_ARG(execute_data, 1);
		// 参数列表结尾
		zval *end = p + num_args;

		// 创建新参数表
		args = zend_new_array(num_args);
		// 初始化成顺序数组
		zend_hash_real_init_packed(args);
		// 把参数依次添加到新表里
		ZEND_HASH_FILL_PACKED(args) {
			// 遍历参数表
			do {
				// 向新表里添加参数
				ZEND_HASH_FILL_ADD(p);
				// 下一个参数
				p++;
			} while (p != end);
		} ZEND_HASH_FILL_END();
	}

	call = execute_data;
	// 转到前一个执行数据
	execute_data = EG(current_execute_data) = EX(prev_execute_data);

	// 如果是静态方法，使用 __callstatic 否则 __call
	call->func = (fbc->op_array.fn_flags & ZEND_ACC_STATIC) ? fbc->op_array.scope->__callstatic : fbc->op_array.scope->__call;
	// 断言：??
	ZEND_ASSERT(zend_vm_calc_used_stack(2, call->func) <= (size_t)(((char*)EG(vm_stack_end)) - (char*)call));
	// 参数数量更新为2个
	ZEND_CALL_NUM_ARGS(call) = 2;

	// 第一个参数是 函数名
	ZVAL_STR(ZEND_CALL_ARG(call, 1), fbc->common.function_name);

	// 第二个参数 指针
	zval *call_args = ZEND_CALL_ARG(call, 2);
	// 如果有参数表
	if (args) {
		// 第二个参数是 参数表
		ZVAL_ARR(call_args, args);
	// 否则
	} else {
		// 第二个参数是空数组
		ZVAL_EMPTY_ARRAY(call_args);
	}
	// 如果有额外参数
	if (UNEXPECTED(call_info & ZEND_CALL_HAS_EXTRA_NAMED_PARAMS)) {
		// 如果参数数量是0
		if (zend_hash_num_elements(Z_ARRVAL_P(call_args)) == 0) {
			// 额外 命名 参数表
			// 增加引用次数
			GC_ADDREF(call->extra_named_params);
			// 额外 命名 参数表，作为本次调用的参数表
			ZVAL_ARR(call_args, call->extra_named_params);
		// 否则
		} else {
			// 分裂参数表，使用副本
			SEPARATE_ARRAY(call_args);
			// 把额外参数复制进来，使用 zval_add_ref 函数作为每个元素的构造方法（引用传递）
			zend_hash_copy(Z_ARRVAL_P(call_args), call->extra_named_params, zval_add_ref);
		}
	}
	// 释放弹跳函数
	zend_free_trampoline(fbc);
	// 使用当前函数
	fbc = call->func;

	// 如果函数是 用户定义
	if (EXPECTED(fbc->type == ZEND_USER_FUNCTION)) {
		// 如果没有运行时缓存
		if (UNEXPECTED(!RUN_TIME_CACHE(&fbc->op_array))) {
			// 为op_array创建并 初始化运行时缓存
			init_func_run_time_cache(&fbc->op_array);
		}
		// 使用刚修改过的执行数据
		execute_data = call;
		// 初始化函数的执行数据，p1:操作码组，p2:返回值
		i_init_func_execute_data(&fbc->op_array, ret, 0 EXECUTE_DATA_CC);
		// 如果使用 zend_execute_ex 方法
		if (EXPECTED(zend_execute_ex == execute_ex)) {
			// windows: 无操作
			LOAD_OPLINE_EX();
			// zend_vm_gen.php 处理
			// "/ZEND_OBSERVER_SAVE_OPLINE\(\)/" => isset($extra_spec['OBSERVER']) && $extra_spec['OBSERVER'] == 1 ? "SAVE_OPLINE()" : "",
			ZEND_OBSERVER_SAVE_OPLINE();
			// 开始观察者调用
			// "/ZEND_OBSERVER_FCALL_BEGIN\(\s*(.*)\s*\)/" => isset($extra_spec['OBSERVER']) ?            ($extra_spec['OBSERVER'] == 0 ? "" : "zend_observer_fcall_begin(\\1)")            : "",
			ZEND_OBSERVER_FCALL_BEGIN(execute_data);
			// return  1
			ZEND_VM_ENTER_EX();
		// 否则
		} else {
			// windows: 无操作
			SAVE_OPLINE_EX();
			// 开始观察者调用
			// "/ZEND_OBSERVER_FCALL_BEGIN\(\s*(.*)\s*\)/" => isset($extra_spec['OBSERVER']) ?            ($extra_spec['OBSERVER'] == 0 ? "" : "zend_observer_fcall_begin(\\1)")            : "",
			ZEND_OBSERVER_FCALL_BEGIN(execute_data);
			// 使用前一个执行数据
			execute_data = EX(prev_execute_data);
			// 如果执行数据存在
			if (execute_data) {
				// windows: 无操作
				LOAD_OPLINE();
			}
			// 添加标记：顶层调用
			ZEND_ADD_CALL_FLAG(call, ZEND_CALL_TOP);
			// 执行此 数据
			zend_execute_ex(call);
		}
	// 否则，是内置函数
	} else {
		zval retval;
		// 必须是内置函数
		ZEND_ASSERT(fbc->type == ZEND_INTERNAL_FUNCTION);
		// 把执行数据转到当前
		EG(current_execute_data) = call;

// 调试用
#if ZEND_DEBUG
		bool should_throw = zend_internal_call_should_throw(fbc, call);
#endif

		// 如果返回值本来为null
		if (ret == NULL) {
			// 使用临时返回值
			ret = &retval;
		}

		// 返回值为nul
		ZVAL_NULL(ret);
		// 开始观察者调用 zend_observer_fcall_begin()
		// "/ZEND_OBSERVER_FCALL_BEGIN\(\s*(.*)\s*\)/" => isset($extra_spec['OBSERVER']) ?            ($extra_spec['OBSERVER'] == 0 ? "" : "zend_observer_fcall_begin(\\1)")            : "",
		ZEND_OBSERVER_FCALL_BEGIN(call);
		
		// 如果没有内部执行方法
		if (!zend_execute_internal) {
			// 如果没有 内部执行数据 保存一个函数调用
			/* saves one function call if zend_execute_internal is not used */
			fbc->internal_function.handler(call, ret);
		// 否则
		} else {
			// 调用 内部执行方法
			zend_execute_internal(call, ret);
		}
// 调试用
#if ZEND_DEBUG
		if (!EG(exception) && call->func) {
			if (should_throw) {
				zend_internal_call_arginfo_violation(call->func);
			}
			ZEND_ASSERT(!(call->func->common.fn_flags & ZEND_ACC_HAS_RETURN_TYPE) ||
				zend_verify_internal_return_type(call->func, ret));
			ZEND_ASSERT((call->func->common.fn_flags & ZEND_ACC_RETURN_REFERENCE)
				? Z_ISREF_P(ret) : !Z_ISREF_P(ret));
			zend_verify_internal_func_info(call->func, ret);
		}
#endif
		// 如果用到了观察者，结束观察者调用
		// "/ZEND_OBSERVER_FCALL_END\(\s*([^,]*)\s*,\s*(.*)\s*\)/" => isset($extra_spec['OBSERVER']) ?            ($extra_spec['OBSERVER'] == 0 ? "" : "zend_observer_fcall_end(\\1, \\2)")            : "",
		ZEND_OBSERVER_FCALL_END(call, EG(exception) ? NULL : ret);

		// 转回前一个执行数据
		EG(current_execute_data) = call->prev_execute_data;

		// 释放所有执行数据中的 函数参数
		zend_vm_stack_free_args(call);
		// 如果使用的是临时变量
		if (ret == &retval) {
			// 销毁临时变量
			zval_ptr_dtor(ret);
		}
	}

	// 当前执行数据
	execute_data = EG(current_execute_data);

	// 如果没有执行数据 或 没有函数 或函数没有类型 或 正在面层调用
	if (!execute_data || !EX(func) || !ZEND_USER_CODE(EX(func)->type) || (call_info & ZEND_CALL_TOP)) {
		// windows: return -1
		ZEND_VM_RETURN();
	}
	
	// 如果需要释放this
	if (UNEXPECTED(call_info & ZEND_CALL_RELEASE_THIS)) {
		// 取出 $this
		zend_object *object = Z_OBJ(call->This);
		// 释放它
		OBJ_RELEASE(object);
	}
	// 释放调用框架（执行数据）。p1:执行数据
	zend_vm_stack_free_call_frame(call);

	// 如果有异常
	if (UNEXPECTED(EG(exception) != NULL)) {
		// 在执行数据中 重新抛错
		zend_rethrow_exception(execute_data);
		// windows: return 2;
		HANDLE_EXCEPTION_LEAVE();
	}

	// windows: 无操作
	LOAD_OPLINE();
	// OPLINE++
	ZEND_VM_INC_OPCODE();
	// windows return 2;
	ZEND_VM_LEAVE();
}

// ing3, 绑定闭包里 use() 的变量
ZEND_VM_HANDLER(182, ZEND_BIND_LEXICAL, TMP, CV, REF)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *closure, *var;

	// 获取zval指针，UNUSED 类型返回null
	closure = GET_OP1_ZVAL_PTR(BP_VAR_R);
	// 如果需要绑定引用
	if (opline->extended_value & ZEND_BIND_REF) {
		/* By-ref binding */
		// 获取 op2的zval指针，UNUSED 类型返回null
		var = GET_OP2_ZVAL_PTR(BP_VAR_W);
		// 如果op2是引用类型
		if (Z_ISREF_P(var)) {
			// 引用次数+1
			Z_ADDREF_P(var);
		// 否则
		} else {
			// 把zval转为引用，p1:zval，p2:引用次数
			ZVAL_MAKE_REF_EX(var, 2);
		}
	// 否则
	} else {
		// 获取zval指针, UNUSED 返回null
		var = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
		// 如果var是未定义 并且 没有 隐式绑定
		if (UNEXPECTED(Z_ISUNDEF_P(var)) && !(opline->extended_value & ZEND_BIND_IMPLICIT)) {
			// windows: 无操作
			SAVE_OPLINE();
			// 报错：p2的变量（变量名）未定义, 并返未初始化zval
			var = ZVAL_UNDEFINED_OP2();
			// 如果有异常
			if (UNEXPECTED(EG(exception))) {
				// 主要是 return
				HANDLE_EXCEPTION();
			}
		}
		// 解引用
		ZVAL_DEREF(var);
		// 增加引用次数
		Z_TRY_ADDREF_P(var);
	}
	// 通过偏移量绑定外部变量（更新）。p1:闭包，p2:偏移量，p3:新值
	// 偏移量里去掉 【绑定引用，隐式绑定】 两个标记
	zend_closure_bind_var_ex(closure,
		(opline->extended_value & ~(ZEND_BIND_REF|ZEND_BIND_IMPLICIT)), var);
	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, 声名函数内的静态变量
ZEND_VM_HANDLER(183, ZEND_BIND_STATIC, CV, UNUSED, REF)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	HashTable *ht;
	zval *value;
	zval *variable_ptr;
	// 获取zval指针, TMP/CONST/UNUSED 返回null，不支持 TMPVAR/TMPVARCV
	variable_ptr = GET_OP1_ZVAL_PTR_PTR_UNDEF(BP_VAR_W);

	// 取得静态变量哈希表指针
	ht = ZEND_MAP_PTR_GET(EX(func)->op_array.static_variables_ptr);
	// 如果哈希表不存在
	if (!ht) {
		// 把现有静态变量表 复制一份
		ht = zend_array_dup(EX(func)->op_array.static_variables);
		// 关联到指针
		ZEND_MAP_PTR_SET(EX(func)->op_array.static_variables_ptr, ht);
	}
	// 引用次数必须是1
	ZEND_ASSERT(GC_REFCOUNT(ht) == 1);

	// 找到 zval 位置
	value = (zval*)((char*)ht->arData + (opline->extended_value & ~(ZEND_BIND_REF|ZEND_BIND_IMPLICIT|ZEND_BIND_EXPLICIT)));

	// windows: 无操作
	SAVE_OPLINE();
	// 如果需要绑定引用
	if (opline->extended_value & ZEND_BIND_REF) {
		// 如果value是常量表达式
		if (Z_TYPE_P(value) == IS_CONSTANT_AST) {
			// 更新value
			if (UNEXPECTED(zval_update_constant_ex(value, EX(func)->op_array.scope) != SUCCESS)) {
				// 主要是 return
				HANDLE_EXCEPTION();
			}
		}
		// value不是常量表达式
		// 销毁没有引用次数的对象，有引用次数的添加到gc回收周期里。并没有销毁 zval本身。
		i_zval_ptr_dtor(variable_ptr);
		// 如果 value不是引用类型，要把它更新成引用类型
		if (UNEXPECTED(!Z_ISREF_P(value))) {
			// 创建引用实例
			zend_reference *ref = (zend_reference*)emalloc(sizeof(zend_reference));
			// 引用次数为2
			GC_SET_REFCOUNT(ref, 2);
			// 类型, 支持GC
			GC_TYPE_INFO(ref) = GC_REFERENCE;
			// 引用目标为 vlaue的副本
			ZVAL_COPY_VALUE(&ref->val, value);
			// 指针为空
			ref->sources.ptr = NULL;
			// 更新value的引用类型指针
			Z_REF_P(value) = ref;
			// 更新类型为 引用类型
			Z_TYPE_INFO_P(value) = IS_REFERENCE_EX;
			// variable_ptr 也更新成引用类型，ref作为引用实例（和value）一样
			ZVAL_REF(variable_ptr, ref);
		// 否则，value已经是引用类型
		} else {
			// 引用次数+1
			Z_ADDREF_P(value);
			// variable_ptr 和value引用相同目标
			ZVAL_REF(variable_ptr, Z_REF_P(value));
		}
	// 否则，不绑定引用
	} else {
		// 销毁没有引用次数的对象，有引用次数的添加到gc回收周期里。并没有销毁 zval本身。
		i_zval_ptr_dtor(variable_ptr);
		// 把找到的值复制过来
		ZVAL_COPY(variable_ptr, value);
	}

	// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, 取得 $this
ZEND_VM_HOT_HANDLER(184, ZEND_FETCH_THIS, UNUSED, UNUSED)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// 如果 $this 是对象
	if (EXPECTED(Z_TYPE(EX(This)) == IS_OBJECT)) {
		// 在执行数据中取出 指定序号的变量
		zval *result = EX_VAR(opline->result.var);
		// $this 放到返回结果里
		ZVAL_OBJ(result, Z_OBJ(EX(This)));
		// 引用次数+1
		Z_ADDREF_P(result);
		// opline + 1，到目标操作码并 return
		ZEND_VM_NEXT_OPCODE();
	// 否则
	} else {
		// 调用方法 zend_this_not_in_object_context_helper
		ZEND_VM_DISPATCH_TO_HELPER(zend_this_not_in_object_context_helper);
	}
}

// ing2 ,获取全局变量 $GLOBAL 本身，不是子元素
ZEND_VM_HANDLER(200, ZEND_FETCH_GLOBALS, UNUSED, UNUSED)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// 符号表中碰到的问题 正 与属性表中的问题相同
	/* For symbol tables we need to deal with exactly the same problems as for property tables. */
	// 从符号表中找到 global并返回给 opline->result.var
	ZVAL_ARR(EX_VAR(opline->result.var),
		zend_proptable_to_symtable(&EG(symbol_table), /* always_duplicate */ 1));
	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}


// ing3, 对($this)检查isempty和isset
ZEND_VM_HANDLER(186, ZEND_ISSET_ISEMPTY_THIS, UNUSED, UNUSED)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// 结果为  （ 操作码扩展值 有 ZEND_ISEMPTY ）^ （$this是对象）
		// 有ZEND_ISEMPTY时$this 不是对象，结果为真
		// 无ZEND_ISEMPTY时，$this是对象，结果为真
	ZVAL_BOOL(EX_VAR(opline->result.var),
		(opline->extended_value & ZEND_ISEMPTY) ^
		 (Z_TYPE(EX(This)) == IS_OBJECT));
	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, 检验op1是否有效，无效则报错， 1/1，从操op1的变量中获取类型 并返回给 结果变量。 
// ZEND_FASTCALL ZEND_MATCH_ERROR_SPEC，2个分支（验证过）
ZEND_VM_HANDLER(49, ZEND_CHECK_VAR, CV, UNUSED)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	// 在执行数据中取出 指定序号的变量
	zval *op1 = EX_VAR(opline->op1.var);

	// 如果 op1 类型是未定义
	if (UNEXPECTED(Z_TYPE_INFO_P(op1) == IS_UNDEF)) {
		// windows: 无操作
		SAVE_OPLINE();
		// 报错：p1的变量（变量名）未定义, 并返未初始化zval
		ZVAL_UNDEFINED_OP1();
		// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
		ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
	}
	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, 把op1 使用引用复制给result
ZEND_VM_HANDLER(140, ZEND_MAKE_REF, VAR|CV, UNUSED)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	// 在执行数据中取出 指定序号的变量
	zval *op1 = EX_VAR(opline->op1.var);

	// 如果 op1 是 编译变量
	if (OP1_TYPE == IS_CV) {
		// 如果op1是未定义
		if (UNEXPECTED(Z_TYPE_P(op1) == IS_UNDEF)) {
			// op1 引用赋值成新的空引用
			// 给 zval 创建新的空的 zend_reference 
			ZVAL_NEW_EMPTY_REF(op1);
			// 引用次数2
			Z_SET_REFCOUNT_P(op1, 2);
			// op1 引用目标 赋值成 null
			ZVAL_NULL(Z_REFVAL_P(op1));
			// 结果为解引用的 op1, 使用引用赋值
			ZVAL_REF(EX_VAR(opline->result.var), Z_REF_P(op1));
		// 否则, op1不是编译变量
		} else {
			// 如果op1是引用哦嘎
			if (Z_ISREF_P(op1)) {
				// 引用次数+1
				Z_ADDREF_P(op1);
			// 否则，op1不是引用类型
			} else {
				// 把zval转为引用，p1:zval，p2:引用次数
				ZVAL_MAKE_REF_EX(op1, 2);
			}
			// 结果为解引用的 op1, 使用引用赋值
			ZVAL_REF(EX_VAR(opline->result.var), Z_REF_P(op1));
		}
	// 否则 如果类型是 间接引用
	} else if (EXPECTED(Z_TYPE_P(op1) == IS_INDIRECT)) {
		// op1解间接引用
		op1 = Z_INDIRECT_P(op1);
		// 如果op1不是引用类型
		if (EXPECTED(!Z_ISREF_P(op1))) {
			// 把zval转为引用，p1:zval，p2:引用次数
			ZVAL_MAKE_REF_EX(op1, 2);
		// 否则
		} else {
			// 增加引用次数
			GC_ADDREF(Z_REF_P(op1));
		}
		// 结果为解引用的 op1, 使用引用赋值
		ZVAL_REF(EX_VAR(opline->result.var), Z_REF_P(op1));
	// 否则，不是编译变量也不是间接引用
	} else {
		// 把op1复制给result
		ZVAL_COPY_VALUE(EX_VAR(opline->result.var), op1);
	}
	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, switch 数跳转，整数 case
ZEND_VM_COLD_CONSTCONST_HANDLER(187, ZEND_SWITCH_LONG, CONST|TMPVARCV, CONST, JMP_ADDR)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op, *jump_zv;
	HashTable *jumptable;

	// 获取zval指针, UNUSED 返回null
	op = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);

	// 如果类型不是整数
	if (Z_TYPE_P(op) != IS_LONG) {
		// 解引用
		ZVAL_DEREF(op);
		// 如果类型不是整数
		if (Z_TYPE_P(op) != IS_LONG) {
			/* Wrong type, fall back to ZEND_CASE chain */
			// opline + 1，到目标操作码并 return
			ZEND_VM_NEXT_OPCODE();
		}
	}
	// op2是跳转表
	// 获取zval指针，UNUSED 类型返回null
	jumptable = Z_ARRVAL_P(GET_OP2_ZVAL_PTR(BP_VAR_R));
	// 获取跳转位置
	jump_zv = zend_hash_index_find(jumptable, Z_LVAL_P(op));
	// 如果位置有效
	if (jump_zv != NULL) {
		// 找到目标操作码，设置成当前操作码并返回。 EX(opline) = (zend_op*)((char*)p1 + p2)
		ZEND_VM_SET_RELATIVE_OPCODE(opline, Z_LVAL_P(jump_zv));
		// windows: return  0
		ZEND_VM_CONTINUE();
	// 否则
	} else {
		/* default */
		// 找到目标操作码，设置成当前操作码并返回。 EX(opline) = (zend_op*)((char*)p1 + p2)
		ZEND_VM_SET_RELATIVE_OPCODE(opline, opline->extended_value);
		// windows: return  0
		ZEND_VM_CONTINUE();
	}
}

// ing3, switch 数跳转，字串 case
ZEND_VM_COLD_CONSTCONST_HANDLER(188, ZEND_SWITCH_STRING, CONST|TMPVARCV, CONST, JMP_ADDR)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op, *jump_zv;
	HashTable *jumptable;

	// 获取zval指针, UNUSED 返回null
	op = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);

	// 如果op1不是字串
	if (Z_TYPE_P(op) != IS_STRING) {
		// 如果op1是常量
		if (OP1_TYPE == IS_CONST) {
			// 类型错误，返回 ZEND_CASE 链
			/* Wrong type, fall back to ZEND_CASE chain */
			
			// opline + 1，到目标操作码并 return
			ZEND_VM_NEXT_OPCODE();
		// 否则，是其他类型
		} else {
			// 解引用
			ZVAL_DEREF(op);
			// 如果类型不是字串
			if (Z_TYPE_P(op) != IS_STRING) {
				// 类型错误，返回 ZEND_CASE 链
				/* Wrong type, fall back to ZEND_CASE chain */
				// opline + 1，到目标操作码并 return
				ZEND_VM_NEXT_OPCODE();
			}
		}
	}

	// 取得跳转表 op2
	// 获取zval指针，UNUSED 类型返回null
	jumptable = Z_ARRVAL_P(GET_OP2_ZVAL_PTR(BP_VAR_R));
	// 取得跳转位置
	jump_zv = zend_hash_find_ex(jumptable, Z_STR_P(op), OP1_TYPE == IS_CONST);
	// 如果有跳转位置
	if (jump_zv != NULL) {
		// 找到目标操作码，设置成当前操作码并返回。 EX(opline) = (zend_op*)((char*)p1 + p2)
		ZEND_VM_SET_RELATIVE_OPCODE(opline, Z_LVAL_P(jump_zv));
		// windows: return  0
		ZEND_VM_CONTINUE();
	// 否则
	} else {
		/* default */
		// 找到目标操作码，设置成当前操作码并返回。 EX(opline) = (zend_op*)((char*)p1 + p2)
		ZEND_VM_SET_RELATIVE_OPCODE(opline, opline->extended_value);
		// windows: return  0
		ZEND_VM_CONTINUE();
	}
}

// ing3, match语句
ZEND_VM_COLD_CONSTCONST_HANDLER(195, ZEND_MATCH, CONST|TMPVARCV, CONST, JMP_ADDR)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op, *jump_zv;
	HashTable *jumptable;

	// 获取zval指针, UNUSED 返回null
	op = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 获取zval指针，UNUSED 类型返回null
	jumptable = Z_ARRVAL_P(GET_OP2_ZVAL_PTR(BP_VAR_R));

// 创建跳转标签 match_try_again
ZEND_VM_C_LABEL(match_try_again):
	// 如果 op1是整数 
	if (Z_TYPE_P(op) == IS_LONG) {
		// 取得跳转位置
		jump_zv = zend_hash_index_find(jumptable, Z_LVAL_P(op));
	// 如果 op1是字串
	} else if (Z_TYPE_P(op) == IS_STRING) {
		// 取得跳转位置
		jump_zv = zend_hash_find_ex(jumptable, Z_STR_P(op), OP1_TYPE == IS_CONST);
	// 如果是引用类型
	} else if (Z_TYPE_P(op) == IS_REFERENCE) {
		// 解引用
		op = Z_REFVAL_P(op);
		// 跳转到指定标签 match_try_again
		ZEND_VM_C_GOTO(match_try_again);
	// 否则，其他类型（报错）
	} else {
		// 如果 op1 是编译变量 并且 是未定义
		if (UNEXPECTED((OP1_TYPE & IS_CV) && Z_TYPE_P(op) == IS_UNDEF)) {
			// windows: 无操作
			SAVE_OPLINE();
			// 报错：p1的变量（变量名）未定义, 并返未初始化zval
			op = ZVAL_UNDEFINED_OP1();
			// 如果有异常
			if (UNEXPECTED(EG(exception))) {
				// 主要是 return
				HANDLE_EXCEPTION();
			}
			// 跳转到指定标签 match_try_again
			ZEND_VM_C_GOTO(match_try_again);
		}

		// 跳转到指定标签 default_branch
		ZEND_VM_C_GOTO(default_branch);
	}

	// 如果 有跳转目标
	if (jump_zv != NULL) {
		// 找到目标操作码，设置成当前操作码并返回。 EX(opline) = (zend_op*)((char*)p1 + p2)
		ZEND_VM_SET_RELATIVE_OPCODE(opline, Z_LVAL_P(jump_zv));
		// windows: return  0
		ZEND_VM_CONTINUE();
	// 否则
	} else {
// 创建跳转标签 default_branch
ZEND_VM_C_LABEL(default_branch):
		/* default */
		// 找到目标操作码，设置成当前操作码并返回。 EX(opline) = (zend_op*)((char*)p1 + p2)
		ZEND_VM_SET_RELATIVE_OPCODE(opline, opline->extended_value);
		// windows: return  0
		ZEND_VM_CONTINUE();
	}
}

//  ing3, match()语句的报错， 1/1，从操op1的变量中获取类型 并返回给 结果变量。 
// ZEND_FASTCALL ZEND_MATCH_ERROR_SPEC，2个分支（验证过）
ZEND_VM_COLD_CONST_HANDLER(197, ZEND_MATCH_ERROR, CONST|TMPVARCV, UNUSED)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op;

	// windows: 无操作
	SAVE_OPLINE();
	// 获取zval指针, UNUSED 返回null
	op = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ing4, 调用函数进行报错
	zend_match_unhandled_error(op);
	// 主要是 return
	HANDLE_EXCEPTION();
}

// ing3, 查检 key在哈希表里是否存在
ZEND_VM_COLD_CONSTCONST_HANDLER(189, ZEND_IN_ARRAY, CONST|TMP|VAR|CV, CONST, NUM)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	// 
	zval *op1;
	// op2是被查数组
	// 访问 (p2).zv。p1:opline,p2:node
	HashTable *ht = Z_ARRVAL_P(RT_CONSTANT(opline, opline->op2));
	//
	zval *result;

	// 获取zval指针, UNUSED 返回null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	
	// 情况1:
	// 如果 op1 类型是 字串
	if (EXPECTED(Z_TYPE_P(op1) == IS_STRING)) {
		// 哈希表里用 key 查找元素。p1:哈希表，p2:key，p3：key上是否有哈希值
		result = zend_hash_find_ex(ht, Z_STR_P(op1), OP1_TYPE == IS_CONST);
		// 如果 op1 类型 是变量或临时变量
		if (OP1_TYPE & (IS_TMP_VAR|IS_VAR)) {
			// 通过指针销毁 zval（字串）
			zval_ptr_dtor_str(op1);
		}
		// 根据当前操作码结果选择下一个操作码，p1:传入值，主要用来做判断。p2:是否检查异常
		ZEND_VM_SMART_BRANCH(result, 0);
	}
	
	// 情况2：如果直接查找不成功（主要是考虑解引用）
	// 如果有扩展值。
	if (opline->extended_value) {
		// 如果op1类型是 整数
		if (EXPECTED(Z_TYPE_P(op1) == IS_LONG)) {
			// 使用哈希值查找
			result = zend_hash_index_find(ht, Z_LVAL_P(op1));
			// 根据当前操作码结果选择下一个操作码，p1:传入值，主要用来做判断。p2:是否检查异常
			ZEND_VM_SMART_BRANCH(result, 0);
		}
		
		// 如果用哈希值也找不到
		// windows: 无操作
		SAVE_OPLINE();
		// 如果 op1 是普通变量或编译变量 并且 op1是引用类型
		if ((OP1_TYPE & (IS_VAR|IS_CV)) && Z_TYPE_P(op1) == IS_REFERENCE) {
			// 解引用
			op1 = Z_REFVAL_P(op1);
			// 如果类型是 字串
			if (EXPECTED(Z_TYPE_P(op1) == IS_STRING)) {
				// 使用 key 获取哈希表的 bucket值。p1:哈希表，p2:key(zend_string)
				result = zend_hash_find(ht, Z_STR_P(op1));
				// 释放操作对象的附加变量
				FREE_OP1();
				// 根据当前操作码结果选择下一个操作码，p1:传入值，主要用来做判断。p2:是否检查异常
				ZEND_VM_SMART_BRANCH(result, 0);
			// 否则 如果类型是 整数
			} else if (EXPECTED(Z_TYPE_P(op1) == IS_LONG)) {
				// 按哈希值查询，兼容顺序数组。p1:哈希表，p2:哈希值
				result = zend_hash_index_find(ht, Z_LVAL_P(op1));
				// 释放操作对象的附加变量
				FREE_OP1();
				// 根据当前操作码结果选择下一个操作码，p1:传入值，主要用来做判断。p2:是否检查异常
				ZEND_VM_SMART_BRANCH(result, 0);
			}
		// 如果 op1 是 编译变量 并且 op1类型为未定义
		} else if (OP1_TYPE == IS_CV && UNEXPECTED(Z_TYPE_P(op1) == IS_UNDEF)) {
			// 报错：p1的变量（变量名）未定义, 并返未初始化zval
			ZVAL_UNDEFINED_OP1();
		}
	// 情况3：（处理几个特殊类型）
	// 否则 如果 op1 类型是 undef,null,false
	} else if (Z_TYPE_P(op1) <= IS_FALSE) {
		// 如果 op1是编译变量 并且 类型是 未定义
		if (OP1_TYPE == IS_CV && UNEXPECTED(Z_TYPE_P(op1) == IS_UNDEF)) {
			// windows: 无操作
			SAVE_OPLINE();
			// 报错：p1的变量（变量名）未定义, 并返未初始化zval
			ZVAL_UNDEFINED_OP1();
			// 如果有异常
			if (UNEXPECTED(EG(exception) != NULL)) {
				// 主要是 return
				HANDLE_EXCEPTION();
			}
		}
		// 使用 有哈希值的键(zend_string) 获取哈希表的 值
		// key为空字串
		result = zend_hash_find_known_hash(ht, ZSTR_EMPTY_ALLOC());
		// 根据当前操作码结果选择下一个操作码，p1:传入值，主要用来做判断。p2:是否检查异常
		ZEND_VM_SMART_BRANCH(result, 0);
	// 情况4：如果直接查找不成功（主要是考虑解引用），没有扩展值，且不是特殊类型。只能遍历哈希表查找。
	// 其他情况
	} else {
		zend_string *key;
		zval key_tmp;

		// 如果op1是变量 或 编译变量 并且 类型是 引用
		if ((OP1_TYPE & (IS_VAR|IS_CV)) && Z_TYPE_P(op1) == IS_REFERENCE) {
			// 解引用
			op1 = Z_REFVAL_P(op1);
			// 如果类型是 字串
			if (EXPECTED(Z_TYPE_P(op1) == IS_STRING)) {
				// 使用 key 获取哈希表的 bucket值。p1:哈希表，p2:key(zend_string)
				result = zend_hash_find(ht, Z_STR_P(op1));
				// 释放操作对象的附加变量
				FREE_OP1();				
				// 根据当前操作码结果选择下一个操作码，p1:传入值，主要用来做判断。p2:是否检查异常
				ZEND_VM_SMART_BRANCH(result, 0);
			}
		}

		// windows: 无操作
		SAVE_OPLINE();
		// 遍历 哈希表
		ZEND_HASH_MAP_FOREACH_STR_KEY(ht, key) {
			// key存到临时变量里
			ZVAL_STR(&key_tmp, key);
			// 如果op1和key相同。
			if (zend_compare(op1, &key_tmp) == 0) {
				// 释放操作对象的附加变量
				FREE_OP1();
				// 结果为 1
				// 根据当前操作码结果选择下一个操作码，p1:传入值，主要用来做判断。p2:是否检查异常
				ZEND_VM_SMART_BRANCH(1, 1);
			}
		} ZEND_HASH_FOREACH_END();
	}
	// 释放操作对象的附加变量
	FREE_OP1();
	// 结果为 0
	// 根据当前操作码结果选择下一个操作码，p1:传入值，主要用来做判断。p2:是否检查异常
	ZEND_VM_SMART_BRANCH(0, 1);
}

// ing4, count函数
ZEND_VM_COLD_CONST_HANDLER(190, ZEND_COUNT, CONST|TMPVAR|CV, UNUSED)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1;
	zend_long count;

	// windows: 无操作
	SAVE_OPLINE();
	// 获取zval指针, UNUSED 返回null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);

	// 没有break就一直循环
	while (1) {
		// 如果类型是 数组
		if (Z_TYPE_P(op1) == IS_ARRAY) {
			// 取出哈希表元素数
			count = zend_hash_num_elements(Z_ARRVAL_P(op1));
			// 跳出
			break;
		// 否则 如果类型是 对象
		} else if (Z_TYPE_P(op1) == IS_OBJECT) {
			// 取出对象
			zend_object *zobj = Z_OBJ_P(op1);

			// 首先判断是否自带处理器
			/* first, we check if the handler is defined */
			// 如果有 count_elements 方法
			if (zobj->handlers->count_elements) {
				// 直接调用，如果成功
				if (SUCCESS == zobj->handlers->count_elements(zobj, &count)) {
					// 跳出
					break;
				}
				// 如果失败，并且有 异常
				if (UNEXPECTED(EG(exception))) {
					// 数量为 0 
					count = 0;
					// 跳出
					break;
				}
			}

			// 如果 count_elements失败。并且 是实现了 Countable 接口的对象，调用count方法
			/* if not and the object implements Countable we call its count() method */
			if (zend_class_implements_interface(zobj->ce, zend_ce_countable)) {
				// 结果 临时变量
				zval retval;

				// 在对象所属类的方法表里查找  count方法
				zend_function *count_fn = zend_hash_find_ptr(&zobj->ce->function_table, ZSTR_KNOWN(ZEND_STR_COUNT));
				// 调用count方法
				zend_call_known_instance_method_with_0_params(count_fn, zobj, &retval);
				// 返回值转成 整数 
				count = zval_get_long(&retval);
				// 销毁临时返回值
				zval_ptr_dtor(&retval);
				// 跳出
				break;
			}

			// 如果没有处理方法，也没有实现 Countable接口，需要报错
			/* If There's no handler and it doesn't implement Countable then emit a TypeError */
		// 否则 如果 op1是（普通变量或编译变量） 并且 类型是 引用
		} else if ((OP1_TYPE & (IS_VAR|IS_CV)) != 0 && Z_TYPE_P(op1) == IS_REFERENCE) {
			// 解引用
			op1 = Z_REFVAL_P(op1);
			// 从头再来
			continue;
		// 否则 如果 op1是编译变量 并且 类型是未定义
		} else if (OP1_TYPE == IS_CV && UNEXPECTED(Z_TYPE_P(op1) == IS_UNDEF)) {
			// 报错：p1的变量（变量名）未定义, 并返未初始化zval
			ZVAL_UNDEFINED_OP1();
		}
		
		// 如果上面没跳出，剩下的就是错误
		count = 0;
		// 报错：参数1必须是可计数对象或 array,但给出了A类型
		zend_type_error("%s(): Argument #1 ($value) must be of type Countable|array, %s given", opline->extended_value ? "sizeof" : "count", zend_zval_type_name(op1));
		// 跳出
		break;
	}

	// count转成整数存入结果
	ZVAL_LONG(EX_VAR(opline->result.var), count);
	// 释放操作对象的附加变量
	FREE_OP1();
	// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3 , 获取类名， 1/1，从操op1的变量中（或者当前运行上下文中）获取类名 并返回给 结果变量。 
// ZEND_FASTCALL ZEND_GET_CLASS_SPEC，4个分支（验证过）
ZEND_VM_COLD_CONST_HANDLER(191, ZEND_GET_CLASS, UNUSED|CONST|TMPVAR|CV, UNUSED)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	// 如果 op1 未使用
	if (OP1_TYPE == IS_UNUSED) {
		// 如果当前函数无所作用域
		if (UNEXPECTED(!EX(func)->common.scope)) {
			// windows: 无操作
			SAVE_OPLINE();
			// 报错：无参数的get_class()只能在类中调用
			zend_throw_error(NULL, "get_class() without arguments must be called from within a class");
			// 结果为未定义
			ZVAL_UNDEF(EX_VAR(opline->result.var));
			// 主要是 return
			HANDLE_EXCEPTION();
		// 有作用域，直接返回作用域
		// 否则
		} else {
			ZVAL_STR_COPY(EX_VAR(opline->result.var), EX(func)->common.scope->name);
			// opline + 1，到目标操作码并 return
			ZEND_VM_NEXT_OPCODE();
		}
	// op1有效
	// 否则
	} else {
		zval *op1;

		// windows: 无操作
		SAVE_OPLINE();
		// 获取zval指针, UNUSED 返回null
		op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
		while (1) {
			// op1类型是对象
			if (Z_TYPE_P(op1) == IS_OBJECT) {
				// 取出 zend_object->name 返回给结果
				ZVAL_STR_COPY(EX_VAR(opline->result.var), Z_OBJCE_P(op1)->name);
			// op1 是变量或编译变量，并且类型是引用
			} else if ((OP1_TYPE & (IS_VAR|IS_CV)) != 0 && Z_TYPE_P(op1) == IS_REFERENCE) {
				// 追踪到引用地址
				op1 = Z_REFVAL_P(op1);
				// 
				continue;
			// 其他情况
			// 否则
			} else {
				// 如果op1是编译变量，并且 未使用
				if (OP1_TYPE == IS_CV && UNEXPECTED(Z_TYPE_P(op1) == IS_UNDEF)) {
					// 报错：p1的变量（变量名）未定义, 并返未初始化zval
					ZVAL_UNDEFINED_OP1();
				}
				// 抛错
				zend_type_error("get_class(): Argument #1 ($object) must be of type object, %s given", zend_zval_type_name(op1));
				// 结果为 未定义
				ZVAL_UNDEF(EX_VAR(opline->result.var));
			}
			// 只要不是引用，就会跳出
			break;
		}
		// 释放操作对象的附加变量
		FREE_OP1();
		// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
		ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
	}
}

//  ing3, 获取类型， 1/1，从操op1的变量中获取类型 并返回给 结果变量。 
// ZEND_FASTCALL ZEND_GET_CALLED_CLASS_SPEC，1个分支（验证过）
ZEND_VM_HANDLER(192, ZEND_GET_CALLED_CLASS, UNUSED, UNUSED)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// 如果有 This 并且是对象（new过的对象）
	if (Z_TYPE(EX(This)) == IS_OBJECT) {
		// 返回 This对象 的名字
		ZVAL_STR_COPY(EX_VAR(opline->result.var), Z_OBJCE(EX(This))->name);
	// 如果this不是对象（静态调用）
	} else if (Z_CE(EX(This))) {
		// 返回 ce 的名字
		ZVAL_STR_COPY(EX_VAR(opline->result.var), Z_CE(EX(This))->name);
	// 其他情况
	} else {
		// 必须没有作用域
		ZEND_ASSERT(!EX(func)->common.scope);
		// # define SAVE_OPLINE() EX(opline) = opline
		SAVE_OPLINE();
		// 抛错
		zend_throw_error(NULL, "get_called_class() must be called from within a class");
		// 结果为 未定义
		ZVAL_UNDEF(EX_VAR(opline->result.var));
		// 处理异常
		// 主要是 return
		HANDLE_EXCEPTION();
	}
	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3 , 获取类型， 1/1，从操op1的变量中获取类型 并返回给 结果变量。 
// ZEND_FASTCALL ZEND_GET_TYPE_SPEC，4个分支（验证过）
ZEND_VM_COLD_CONST_HANDLER(193, ZEND_GET_TYPE, CONST|TMP|VAR|CV, UNUSED)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1;
	zend_string *type;

	// windows: 无操作
	SAVE_OPLINE();
	// 获取zval指针, UNUSED 返回null，不支持 TMPVAR/TMPVARCV
	op1 = GET_OP1_ZVAL_PTR_DEREF(BP_VAR_R);
	// 从操op1的变量中获取类型
	type = zend_zval_get_legacy_type(op1);
	// 如果成功，返回给结果
	if (EXPECTED(type)) {
		// type 本身是保留字
		ZVAL_INTERNED_STR(EX_VAR(opline->result.var), type);
	// 如果失败
	} else {
		// 返回 unknown type
		ZVAL_STRING(EX_VAR(opline->result.var), "unknown type");
	}
	// 释放操作对象的附加变量
	FREE_OP1();
	// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3 , 获取参数数量， 1/1，从操作码列表中获取参数数量并 返回给 结果变量。 
// ZEND_FASTCALL ZEND_FUNC_NUM_ARGS_SPEC，1个分支（验证过）
ZEND_VM_HANDLER(171, ZEND_FUNC_NUM_ARGS, UNUSED, UNUSED)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	// (call)->This.u2.num_args
	ZVAL_LONG(EX_VAR(opline->result.var), EX_NUM_ARGS());
	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing2, 获取传入的参数，op1:跳过数量
ZEND_VM_HANDLER(172, ZEND_FUNC_GET_ARGS, UNUSED|CONST, UNUSED)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zend_array *ht;
	uint32_t arg_count, result_size, skip;

	// 执行数据中的参数数量 
	arg_count = EX_NUM_ARGS();
	// 如果op1是常量
	if (OP1_TYPE == IS_CONST) {
		// 跳过数量
		// 访问 (p2).zv。p1:opline,p2:node
		skip = Z_LVAL_P(RT_CONSTANT(opline, opline->op1));
		// 如果，要跳过所有参数
		if (arg_count < skip) {
			// 没有剩下的参数需要处理
			result_size = 0;
		// 否则
		} else {
			// 跳过后剩下的参数数量
			result_size = arg_count - skip;
		}
	// 否则，op1不是常量
	} else {
		// 
		skip = 0;
		// 需要处理所有参数
		result_size = arg_count;
	}

	// 如果有需要获取的参数
	if (result_size) {
		// 第一个额外参数编号
		uint32_t first_extra_arg = EX(func)->op_array.num_args;
		// 创建新数组，8个元素
		ht = zend_new_array(result_size);
		// 哈希表放到 结果里
		ZVAL_ARR(EX_VAR(opline->result.var), ht);
		// 初始化成数组
		zend_hash_real_init_packed(ht);
		// 向哈希表中填充元素
		ZEND_HASH_FILL_PACKED(ht) {
			zval *p, *q;
			uint32_t i = skip;
			// 取得变量
			p = EX_VAR_NUM(i);
			// 如果数量大于 额外参数序号
			if (arg_count > first_extra_arg) {
				// 先处理必须参数
				while (i < first_extra_arg) {
					// 当前参数
					q = p;
					// 如果参数有效
					if (EXPECTED(Z_TYPE_INFO_P(q) != IS_UNDEF)) {
						// 减少引用次数
						ZVAL_DEREF(q);
						// 如果是可以计数类型
						if (Z_OPT_REFCOUNTED_P(q)) {
							// 引用次数+1
							Z_ADDREF_P(q);
						}
						// 设置填充元素。p1:填充元素
						ZEND_HASH_FILL_SET(q);
					// 否则
					} else {
						// 值为null
						ZEND_HASH_FILL_SET_NULL();
					}
					// 切换找到下一个填充位置和索引号
					ZEND_HASH_FILL_NEXT();
					// 原列表指针后移
					p++;
					// 序号+1
					i++;
				}
				
				// 如果路过数小于必须参数数
				if (skip < first_extra_arg) {
					// 不用再跳过了
					skip = 0;
				// 否则
				} else {
					// 倒着往回取？？
					skip -= first_extra_arg;
				}
				// 通过序号取得变量
				p = EX_VAR_NUM(EX(func)->op_array.last_var + EX(func)->op_array.T + skip);
			}
			
			// 如果 还有未处理的参数
			while (i < arg_count) {
				// 参数
				q = p;
				// 如果参数有效
				if (EXPECTED(Z_TYPE_INFO_P(q) != IS_UNDEF)) {
					// 减少引用次数
					ZVAL_DEREF(q);
					// 如果是可以计数类型
					if (Z_OPT_REFCOUNTED_P(q)) {
						// 引用次数+1
						Z_ADDREF_P(q);
					}
					// 设置填充元素。p1:填充元素
					ZEND_HASH_FILL_SET(q);
				// 否则
				} else {
					// 值为null
					ZEND_HASH_FILL_SET_NULL();
				}
				// 切换找到下一个填充位置和索引号
				ZEND_HASH_FILL_NEXT();
				// 下一个参数
				p++;
				// 序号+1
				i++;
			}
		} ZEND_HASH_FILL_END();
		// 更新数组的有效元素数
		ht->nNumOfElements = result_size;
	// 否则
	} else {
		// 返回值为 空数组
		ZVAL_EMPTY_ARRAY(EX_VAR(opline->result.var));
	}
	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, 复制变量， 1/1，把op1的值复制给结果。 
// ZEND_FASTCALL ZEND_COPY_TMP_SPEC，1个分支（验证过）
ZEND_VM_HANDLER(167, ZEND_COPY_TMP, TMPVAR, UNUSED)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	// 获取zval指针，UNUSED 类型返回null
	zval *value = GET_OP1_ZVAL_PTR(BP_VAR_R);
	// 在执行数据中取出 指定序号的变量
	zval *result = EX_VAR(opline->result.var);
	// 直接把op1 的值复制给结果
	ZVAL_COPY(result, value);
	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing2， 从闭包切换回前一个执行数据？
ZEND_VM_HANDLER(202, ZEND_CALLABLE_CONVERT, UNUSED, UNUSED)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	// 调用的 上下文
	zend_execute_data *call = EX(call);

	// 从执行数据创建假闭包，p1:返回值，p2:执行数据
	zend_closure_from_frame(EX_VAR(opline->result.var), call);

	// 如果调用信息中有 【需要释放$this】
	if (ZEND_CALL_INFO(call) & ZEND_CALL_RELEASE_THIS) {
		// 释放 $this
		OBJ_RELEASE(Z_OBJ(call->This));
	}

	// 使用前一个执行数据
	EX(call) = call->prev_execute_data;
	// 释放call
	// 释放调用框架（执行数据）。p1:执行数据
	zend_vm_stack_free_call_frame(call);

	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, 跳转， 1/1，跳转到 op1 中指定的位置。 
// ZEND_FASTCALL ZEND_JMP_SPEC，1个分支（验证过）
ZEND_VM_HOT_TYPE_SPEC_HANDLER(ZEND_JMP, (OP_JMP_ADDR(op, op->op1) > op), ZEND_JMP_FORWARD, JMP_ADDR, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	
	// 获取跳转位置，传给 OPLINE
	// windows：EX(opline)
	// OP_JMP_ADDR: 访问 p2.jmp_addr
	OPLINE = OP_JMP_ADDR(opline, opline->op1);
	// 再调用continue 即可完成跳转
	// windows: return  0
	ZEND_VM_CONTINUE();
}

// ing3, 加法运算， 2/4，ZEND_ADD_LONG_NO_OVERFLOW，针对无溢出的整数。 
// 要求：op1,op2,返回值都是整数 , 额外规则：不能两个常量、可交换。
// ZEND_FASTCALL ZEND_ADD_LONG_NO_OVERFLOW_SPEC，2个分支（验证过）
ZEND_VM_HOT_TYPE_SPEC_HANDLER(ZEND_ADD, (res_info == MAY_BE_LONG && op1_info == MAY_BE_LONG && op2_info == MAY_BE_LONG), ZEND_ADD_LONG_NO_OVERFLOW, CONST|TMPVARCV, CONST|TMPVARCV, SPEC(NO_CONST_CONST,COMMUTATIVE))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2, *result;

	// 获取zval指针, UNUSED 返回null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 获取zval指针, UNUSED 返回null
	op2 = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 在执行数据中取出 指定序号的变量
	result = EX_VAR(opline->result.var);
	// 三个方法只有这里不一样，取出整数直接加
	ZVAL_LONG(result, Z_LVAL_P(op1) + Z_LVAL_P(op2));
	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, 加法运算， 3/4,ZEND_ADD_LONG，针对整数。
// 要求：op1,op2 都是整数, 额外规则：不能两个常量、可交换。
// ZEND_FASTCALL ZEND_ADD_LONG_SPEC，2个分支（验证过）
ZEND_VM_HOT_TYPE_SPEC_HANDLER(ZEND_ADD, (op1_info == MAY_BE_LONG && op2_info == MAY_BE_LONG), ZEND_ADD_LONG, CONST|TMPVARCV, CONST|TMPVARCV, SPEC(NO_CONST_CONST,COMMUTATIVE))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2, *result;

	// 获取zval指针, UNUSED 返回null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 获取zval指针, UNUSED 返回null
	op2 = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 在执行数据中取出 指定序号的变量
	result = EX_VAR(opline->result.var);
	// 整数快速加法
	fast_long_add_function(result, op1, op2);
	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, 加法运算， 4/4。ZEND_ADD_DOUBLE，针对小数。
// 要求：两个操作对象都是小数, 额外规则：不能两个常量、可交换。
// ZEND_FASTCALL ZEND_ADD_DOUBLE_SPEC，2个分支（验证过）
ZEND_VM_HOT_TYPE_SPEC_HANDLER(ZEND_ADD, (op1_info == MAY_BE_DOUBLE && op2_info == MAY_BE_DOUBLE), ZEND_ADD_DOUBLE, CONST|TMPVARCV, CONST|TMPVARCV, SPEC(NO_CONST_CONST,COMMUTATIVE))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2, *result;

	// 获取zval指针, UNUSED 返回null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 获取zval指针, UNUSED 返回null
	op2 = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 在执行数据中取出 指定序号的变量
	result = EX_VAR(opline->result.var);
	// 取出小数直接加
	ZVAL_DOUBLE(result, Z_DVAL_P(op1) + Z_DVAL_P(op2));
	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, 减法运算， 2/4，ZEND_SUB_LONG_NO_OVERFLOW，针对无溢出的整数。 
// 要求：op1,op2,返回值都是整数 , 额外规则：不能两个常量
// ZEND_FASTCALL ZEND_SUB_LONG_NO_OVERFLOW_SPEC，3个分支（验证过）
ZEND_VM_HOT_TYPE_SPEC_HANDLER(ZEND_SUB, (res_info == MAY_BE_LONG && op1_info == MAY_BE_LONG && op2_info == MAY_BE_LONG), ZEND_SUB_LONG_NO_OVERFLOW, CONST|TMPVARCV, CONST|TMPVARCV, SPEC(NO_CONST_CONST))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2, *result;

	// 获取zval指针, UNUSED 返回null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 获取zval指针, UNUSED 返回null
	op2 = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 在执行数据中取出 指定序号的变量
	result = EX_VAR(opline->result.var);
	// 取出整数直接减
	ZVAL_LONG(result, Z_LVAL_P(op1) - Z_LVAL_P(op2));
	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, 减法运算， 3/4, ZEND_SUB_LONG，针对整数。
// 要求：op1,op2 都是整数, 额外规则：不能两个常量
// ZEND_FASTCALL ZEND_SUB_LONG_SPEC，3个分支（验证过）
ZEND_VM_HOT_TYPE_SPEC_HANDLER(ZEND_SUB, (op1_info == MAY_BE_LONG && op2_info == MAY_BE_LONG), ZEND_SUB_LONG, CONST|TMPVARCV, CONST|TMPVARCV, SPEC(NO_CONST_CONST))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2, *result;

	// 获取zval指针, UNUSED 返回null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 获取zval指针, UNUSED 返回null
	op2 = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 在执行数据中取出 指定序号的变量
	result = EX_VAR(opline->result.var);
	// 快速减法
	fast_long_sub_function(result, op1, op2);
	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, 加法运算， 4/4。ZEND_SUB_DOUBLE，针对小数。
// 要求：两个操作对象都是小数, 额外规则：不能两个常量
// ZEND_FASTCALL ZEND_SUB_DOUBLE_SPEC，3个分支（验证过）
ZEND_VM_HOT_TYPE_SPEC_HANDLER(ZEND_SUB, (op1_info == MAY_BE_DOUBLE && op2_info == MAY_BE_DOUBLE), ZEND_SUB_DOUBLE, CONST|TMPVARCV, CONST|TMPVARCV, SPEC(NO_CONST_CONST))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2, *result;

	// 获取zval指针, UNUSED 返回null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 获取zval指针, UNUSED 返回null
	op2 = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 在执行数据中取出 指定序号的变量
	result = EX_VAR(opline->result.var);
	// 取出小数直接减
	ZVAL_DOUBLE(result, Z_DVAL_P(op1) - Z_DVAL_P(op2));
	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, 乘法运算， 2/4，ZEND_MUL_LONG_NO_OVERFLOW，针对无溢出的整数。 
// 要求：op1,op2,返回值都是整数 , 额外规则：不能两个常量、可交换。
// ZEND_FASTCALL ZEND_MUL_LONG_NO_OVERFLOW_SPEC，2个分支（验证过）
ZEND_VM_HOT_TYPE_SPEC_HANDLER(ZEND_MUL, (res_info == MAY_BE_LONG && op1_info == MAY_BE_LONG && op2_info == MAY_BE_LONG), ZEND_MUL_LONG_NO_OVERFLOW, CONST|TMPVARCV, CONST|TMPVARCV, SPEC(NO_CONST_CONST,COMMUTATIVE))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2, *result;

	// 获取zval指针, UNUSED 返回null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 获取zval指针, UNUSED 返回null
	op2 = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 在执行数据中取出 指定序号的变量
	result = EX_VAR(opline->result.var);
	// 取出整数直接乘
	ZVAL_LONG(result, Z_LVAL_P(op1) * Z_LVAL_P(op2));
	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, 乘法运算， 3/4, ZEND_MUL_LONG，针对整数。
// 要求：op1,op2 都是整数, 额外规则：不能两个常量、可交换。
// ZEND_FASTCALL ZEND_MUL_LONG_SPEC，2个分支（验证过）
ZEND_VM_HOT_TYPE_SPEC_HANDLER(ZEND_MUL, (op1_info == MAY_BE_LONG && op2_info == MAY_BE_LONG), ZEND_MUL_LONG, CONST|TMPVARCV, CONST|TMPVARCV, SPEC(NO_CONST_CONST,COMMUTATIVE))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2, *result;
	zend_long overflow;

	// 获取zval指针, UNUSED 返回null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 获取zval指针, UNUSED 返回null
	op2 = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 在执行数据中取出 指定序号的变量
	result = EX_VAR(opline->result.var);
	// 乘法运算
	ZEND_SIGNED_MULTIPLY_LONG(Z_LVAL_P(op1), Z_LVAL_P(op2), Z_LVAL_P(result), Z_DVAL_P(result), overflow);
	Z_TYPE_INFO_P(result) = overflow ? IS_DOUBLE : IS_LONG;
	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, 加法运算， 4/4。ZEND_MUL_DOUBLE，针对小数。
// 要求：两个操作对象都是小数, 额外规则：不能两个常量、可交换。
// ZEND_FASTCALL ZEND_MUL_DOUBLE_SPEC，2个分支（验证过）
ZEND_VM_HOT_TYPE_SPEC_HANDLER(ZEND_MUL, (op1_info == MAY_BE_DOUBLE && op2_info == MAY_BE_DOUBLE), ZEND_MUL_DOUBLE, CONST|TMPVARCV, CONST|TMPVARCV, SPEC(NO_CONST_CONST,COMMUTATIVE))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2, *result;

	// 获取zval指针, UNUSED 返回null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 获取zval指针, UNUSED 返回null
	op2 = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 在执行数据中取出 指定序号的变量
	result = EX_VAR(opline->result.var);
	// 取出小数直接乘
	ZVAL_DOUBLE(result, Z_DVAL_P(op1) * Z_DVAL_P(op2));
	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, 相等 或 相同， 2/3。ZEND_IS_EQUAL_LONG，整数。
// 要求：两个操作对象都是整数, 额外规则：智能分支(*3个分支)、不能两个常量、可交换。
// ZEND_FASTCALL ZEND_IS_EQUAL_LONG_SPEC，6个分支（验证过）
ZEND_VM_HOT_TYPE_SPEC_HANDLER(ZEND_IS_EQUAL|ZEND_IS_IDENTICAL, (op1_info == MAY_BE_LONG && op2_info == MAY_BE_LONG), ZEND_IS_EQUAL_LONG, CONST|TMPVARCV, CONST|TMPVARCV, SPEC(SMART_BRANCH,NO_CONST_CONST,COMMUTATIVE))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2;
	bool result;

	// 获取zval指针, UNUSED 返回null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 获取zval指针, UNUSED 返回null
	op2 = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 取出整数直接比较
	result = (Z_LVAL_P(op1) == Z_LVAL_P(op2));
	// 根据当前操作码结果选择下一个操作码，p1:传入值，主要用来做判断。p2:是否检查异常
	ZEND_VM_SMART_BRANCH(result, 0);
}

// ing3, 相等 或 相同， 3/3。ZEND_IS_EQUAL_DOUBLE，小数。
// 要求：两个操作对象都是小数, 额外规则：智能分支(*3个分支)、不能两个常量、可交换。
// ZEND_FASTCALL ZEND_IS_EQUAL_DOUBLE_SPEC，6个分支（验证过）
ZEND_VM_HOT_TYPE_SPEC_HANDLER(ZEND_IS_EQUAL|ZEND_IS_IDENTICAL, (op1_info == MAY_BE_DOUBLE && op2_info == MAY_BE_DOUBLE), ZEND_IS_EQUAL_DOUBLE, CONST|TMPVARCV, CONST|TMPVARCV, SPEC(SMART_BRANCH,NO_CONST_CONST,COMMUTATIVE))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2;
	bool result;

	// 获取zval指针, UNUSED 返回null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 获取zval指针, UNUSED 返回null
	op2 = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 取出直接比较
	result = (Z_DVAL_P(op1) == Z_DVAL_P(op2));
	// 根据当前操作码结果选择下一个操作码，p1:传入值，主要用来做判断。p2:是否检查异常
	ZEND_VM_SMART_BRANCH(result, 0);
}

// ing3, 不相等 或 不相同， 2/3。ZEND_IS_NOT_EQUAL_LONG，整数。
// 要求：两个操作对象都是整数, 额外规则：智能分支(*3个分支)、不能两个常量、可交换。
// ZEND_FASTCALL ZEND_IS_NOT_EQUAL_LONG_SPEC，6个分支（验证过）
ZEND_VM_HOT_TYPE_SPEC_HANDLER(ZEND_IS_NOT_EQUAL|ZEND_IS_NOT_IDENTICAL, (op1_info == MAY_BE_LONG && op2_info == MAY_BE_LONG), ZEND_IS_NOT_EQUAL_LONG, CONST|TMPVARCV, CONST|TMPVARCV, SPEC(SMART_BRANCH,NO_CONST_CONST,COMMUTATIVE))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2;
	bool result;

	// 获取zval指针, UNUSED 返回null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 获取zval指针, UNUSED 返回null
	op2 = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 取出整数直接比较
	result = (Z_LVAL_P(op1) != Z_LVAL_P(op2));
	// 根据当前操作码结果选择下一个操作码，p1:传入值，主要用来做判断。p2:是否检查异常
	ZEND_VM_SMART_BRANCH(result, 0);
}

// ing3, 不相等 或 不相同， 3/3。ZEND_IS_NOT_EQUAL_DOUBLE，整数。
// 要求：两个操作对象都是整数, 额外规则：智能分支(*3个分支)、不能两个常量、可交换。
// ZEND_FASTCALL ZEND_IS_NOT_EQUAL_DOUBLE_SPEC，6个分支（验证过）
ZEND_VM_HOT_TYPE_SPEC_HANDLER(ZEND_IS_NOT_EQUAL|ZEND_IS_NOT_IDENTICAL, (op1_info == MAY_BE_DOUBLE && op2_info == MAY_BE_DOUBLE), ZEND_IS_NOT_EQUAL_DOUBLE, CONST|TMPVARCV, CONST|TMPVARCV, SPEC(SMART_BRANCH,NO_CONST_CONST,COMMUTATIVE))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2;
	bool result;

	// 获取zval指针, UNUSED 返回null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 获取zval指针, UNUSED 返回null
	op2 = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 取出小数直接比较
	result = (Z_DVAL_P(op1) != Z_DVAL_P(op2));
	// 根据当前操作码结果选择下一个操作码，p1:传入值，主要用来做判断。p2:是否检查异常
	ZEND_VM_SMART_BRANCH(result, 0);
}

// ing3, 相同， 4/4。ZEND_IS_IDENTICAL_NOTHROW，不抛错。
// 要求：复杂, 额外规则：可交换。
// ZEND_FASTCALL ZEND_IS_IDENTICAL_NOTHROW_SPEC，2个分支（验证过）
ZEND_VM_TYPE_SPEC_HANDLER(ZEND_IS_IDENTICAL, op->op1_type == IS_CV && (op->op2_type & (IS_CONST|IS_CV)) && !(op1_info & (MAY_BE_UNDEF|MAY_BE_REF)) && !(op2_info & (MAY_BE_UNDEF|MAY_BE_REF)), ZEND_IS_IDENTICAL_NOTHROW, CV, CONST|CV, SPEC(COMMUTATIVE))
{
	/* This is declared below the specializations for MAY_BE_LONG/MAY_BE_DOUBLE so those will be used instead if possible. */
	/* This optimizes $x === SOME_CONST_EXPR and $x === $y for non-refs and non-undef, which can't throw. */
	/* (Infinite recursion when comparing arrays is an uncatchable fatal error) */
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2;
	bool result;

	// 获取zval指针, UNUSED 返回null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 获取zval指针, UNUSED 返回null
	op2 = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 调用函数进行比较
	result = fast_is_identical_function(op1, op2);
	// const 和 cv 不需要 free
	/* Free is a no-op for const/cv */
	// 根据当前操作码结果选择下一个操作码，p1:传入值，主要用来做判断。p2:是否检查异常
	ZEND_VM_SMART_BRANCH(result, 0);
}

// ing3, 相同， 4/4。ZEND_IS_NOT_IDENTICAL_NOTHROW，不抛错。
// 要求：复杂, 额外规则：可交换。
// ZEND_FASTCALL ZEND_IS_NOT_IDENTICAL_NOTHROW_SPEC，2个分支（验证过）
ZEND_VM_TYPE_SPEC_HANDLER(ZEND_IS_NOT_IDENTICAL, op->op1_type == IS_CV && (op->op2_type & (IS_CONST|IS_CV)) && !(op1_info & (MAY_BE_UNDEF|MAY_BE_REF)) && !(op2_info & (MAY_BE_UNDEF|MAY_BE_REF)), ZEND_IS_NOT_IDENTICAL_NOTHROW, CV, CONST|CV, SPEC(COMMUTATIVE))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2;
	bool result;

	// 获取zval指针, UNUSED 返回null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 获取zval指针, UNUSED 返回null
	op2 = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 调用函数进行比较
	result = fast_is_identical_function(op1, op2);
	// const 和 cv 不需要 free
	/* Free is a no-op for const/cv */
	
	// 根据当前操作码结果选择下一个操作码，p1:传入值，主要用来做判断。p2:是否检查异常
	ZEND_VM_SMART_BRANCH(!result, 0);
}

// ing3, 小于， 2/3。ZEND_IS_SMALLER_LONG，针对整数。
// 要求：op1、op2是整数 , 额外规则：智能分支(*3个分支)，不能双常量（-1个分支）。
// ZEND_FASTCALL ZEND_IS_SMALLER_LONG_SPEC，9个分支（验证过）
ZEND_VM_HOT_TYPE_SPEC_HANDLER(ZEND_IS_SMALLER, (op1_info == MAY_BE_LONG && op2_info == MAY_BE_LONG), ZEND_IS_SMALLER_LONG, CONST|TMPVARCV, CONST|TMPVARCV, SPEC(SMART_BRANCH,NO_CONST_CONST))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2;
	bool result;

	// 获取zval指针, UNUSED 返回null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 获取zval指针, UNUSED 返回null
	op2 = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 取出整数直接比较
	result = (Z_LVAL_P(op1) < Z_LVAL_P(op2));
	// 根据当前操作码结果选择下一个操作码，p1:传入值，主要用来做判断。p2:是否检查异常
	ZEND_VM_SMART_BRANCH(result, 0);
}

// ing3, 小于， 3/3。ZEND_IS_SMALLER_DOUBLE，针对小数。
// 要求：op1、op2是小数 , 额外规则：智能分支(*3个分支)，不能双常量（-1个分支）。
// ZEND_FASTCALL ZEND_IS_SMALLER_DOUBLE_SPEC，9个分支（验证过）
ZEND_VM_HOT_TYPE_SPEC_HANDLER(ZEND_IS_SMALLER, (op1_info == MAY_BE_DOUBLE && op2_info == MAY_BE_DOUBLE), ZEND_IS_SMALLER_DOUBLE, CONST|TMPVARCV, CONST|TMPVARCV, SPEC(SMART_BRANCH,NO_CONST_CONST))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2;
	bool result;

	// 获取zval指针, UNUSED 返回null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 获取zval指针, UNUSED 返回null
	op2 = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 取出小数直接比较
	result = (Z_DVAL_P(op1) < Z_DVAL_P(op2));
	// 根据当前操作码结果选择下一个操作码，p1:传入值，主要用来做判断。p2:是否检查异常
	ZEND_VM_SMART_BRANCH(result, 0);
}

// ing3, 小于等于， 2/3。ZEND_IS_SMALLER_OR_EQUAL_LONG，针对整数。
// 要求：op1、op2是整数 , 额外规则：智能分支(*3个分支)，不能双常量（-1个分支）。
// ZEND_FASTCALL ZEND_IS_SMALLER_OR_EQUAL_LONG_SPEC，9个分支（验证过）
ZEND_VM_HOT_TYPE_SPEC_HANDLER(ZEND_IS_SMALLER_OR_EQUAL, (op1_info == MAY_BE_LONG && op2_info == MAY_BE_LONG), ZEND_IS_SMALLER_OR_EQUAL_LONG, CONST|TMPVARCV, CONST|TMPVARCV, SPEC(SMART_BRANCH,NO_CONST_CONST))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2;
	bool result;

	// 获取zval指针, UNUSED 返回null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 获取zval指针, UNUSED 返回null
	op2 = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 取出整数直接比较
	result = (Z_LVAL_P(op1) <= Z_LVAL_P(op2));
	// 根据当前操作码结果选择下一个操作码，p1:传入值，主要用来做判断。p2:是否检查异常
	ZEND_VM_SMART_BRANCH(result, 0);
}

// ing3, 小于等于， 3/3。ZEND_IS_SMALLER_OR_EQUAL_DOUBLE，针对小数。
// 要求：op1、op2是小数 , 额外规则：智能分支(*3个分支)，不能双常量（-1个分支）。
// ZEND_FASTCALL ZEND_IS_SMALLER_OR_EQUAL_DOUBLE_SPEC，9个分支（验证过）
ZEND_VM_HOT_TYPE_SPEC_HANDLER(ZEND_IS_SMALLER_OR_EQUAL, (op1_info == MAY_BE_DOUBLE && op2_info == MAY_BE_DOUBLE), ZEND_IS_SMALLER_OR_EQUAL_DOUBLE, CONST|TMPVARCV, CONST|TMPVARCV, SPEC(SMART_BRANCH,NO_CONST_CONST))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2;
	bool result;

	// 获取zval指针, UNUSED 返回null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 获取zval指针, UNUSED 返回null
	op2 = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 取出小数直接比较
	result = (Z_DVAL_P(op1) <= Z_DVAL_P(op2));
	// 根据当前操作码结果选择下一个操作码，p1:传入值，主要用来做判断。p2:是否检查异常
	ZEND_VM_SMART_BRANCH(result, 0);
}

// ing3, 自加（前）， 2/3。ZEND_PRE_INC_LONG_NO_OVERFLOW，无溢出整数自减。
// 要求：结果是整数，op1是整数 , 额外规则：有返回结果（*2分支：结果有接收或无接收）。
// ZEND_FASTCALL ZEND_PRE_INC_LONG_NO_OVERFLOW_SPEC，2个分支（验证过）
ZEND_VM_HOT_TYPE_SPEC_HANDLER(ZEND_PRE_INC, (res_info == MAY_BE_LONG && op1_info == MAY_BE_LONG), ZEND_PRE_INC_LONG_NO_OVERFLOW, CV, ANY, SPEC(RETVAL))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *var_ptr;
	// 获取zval指针, TMP/CONST/UNUSED 返回null，不支持 TMPVAR/TMPVARCV
	var_ptr = GET_OP1_ZVAL_PTR_PTR_UNDEF(BP_VAR_RW);
	// 先给整数算加法
	Z_LVAL_P(var_ptr)++;
	// 如果 操作码的运算结果有效(不是IS_UNUSED)
	if (UNEXPECTED(RETURN_VALUE_USED(opline))) {
		// 再把整数复制给结果
		ZVAL_LONG(EX_VAR(opline->result.var), Z_LVAL_P(var_ptr));
	}
	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, 自加（前）， 3/3。ZEND_PRE_INC_LONG，整数自减。
// 要求：op1是整数 , 额外规则：有返回结果（*2分支：结果有接收或无接收）。
// ZEND_FASTCALL ZEND_PRE_INC_LONG_SPEC，2个分支（验证过）
ZEND_VM_HOT_TYPE_SPEC_HANDLER(ZEND_PRE_INC, (op1_info == MAY_BE_LONG), ZEND_PRE_INC_LONG, CV, ANY, SPEC(RETVAL))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *var_ptr;
	// 获取zval指针, TMP/CONST/UNUSED 返回null，不支持 TMPVAR/TMPVARCV
	var_ptr = GET_OP1_ZVAL_PTR_PTR_UNDEF(BP_VAR_RW);
	// 先给整数算加法
	fast_long_increment_function(var_ptr);
	// 如果 操作码的运算结果有效(不是IS_UNUSED)
	if (UNEXPECTED(RETURN_VALUE_USED(opline))) {
		// 再把整数复制给结果
		ZVAL_COPY_VALUE(EX_VAR(opline->result.var), var_ptr);
	}
	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, 自减（前）， 2/3。ZEND_PRE_DEC_LONG_NO_OVERFLOW，无溢出整数自减。
// 要求：结果是整数，op1是整数 , 额外规则：有返回结果（*2分支：结果有接收或无接收）。
// ZEND_FASTCALL ZEND_PRE_DEC_LONG_NO_OVERFLOW_SPEC，2个分支（验证过）
ZEND_VM_HOT_TYPE_SPEC_HANDLER(ZEND_PRE_DEC, (res_info == MAY_BE_LONG && op1_info == MAY_BE_LONG), ZEND_PRE_DEC_LONG_NO_OVERFLOW, CV, ANY, SPEC(RETVAL))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *var_ptr;
	// 获取zval指针, TMP/CONST/UNUSED 返回null，不支持 TMPVAR/TMPVARCV
	var_ptr = GET_OP1_ZVAL_PTR_PTR_UNDEF(BP_VAR_RW);
	// 先给整数算减法
	Z_LVAL_P(var_ptr)--;
	// 如果 操作码的运算结果有效(不是IS_UNUSED)
	if (UNEXPECTED(RETURN_VALUE_USED(opline))) {
		// 再把整数复制给结果 
		ZVAL_LONG(EX_VAR(opline->result.var), Z_LVAL_P(var_ptr));
	}
	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, 自减（前）， 3/3。ZEND_PRE_DEC_LONG，整数自减。
// 要求：op1是整数 , 额外规则：有返回结果（*2分支：结果有接收或无接收）。
// ZEND_FASTCALL ZEND_PRE_DEC_LONG_SPEC，2个分支（验证过）
ZEND_VM_HOT_TYPE_SPEC_HANDLER(ZEND_PRE_DEC, (op1_info == MAY_BE_LONG), ZEND_PRE_DEC_LONG, CV, ANY, SPEC(RETVAL))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *var_ptr;
	// 获取zval指针, TMP/CONST/UNUSED 返回null，不支持 TMPVAR/TMPVARCV
	var_ptr = GET_OP1_ZVAL_PTR_PTR_UNDEF(BP_VAR_RW);
	// 先给整数算减法
	fast_long_decrement_function(var_ptr);
	// 如果 操作码的运算结果有效(不是IS_UNUSED)
	if (UNEXPECTED(RETURN_VALUE_USED(opline))) {
		// 再把整数复制给结果 
		ZVAL_COPY_VALUE(EX_VAR(opline->result.var), var_ptr);
	}
	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, 自加（后）， 2/3。ZEND_POST_INC_LONG_NO_OVERFLOW，无溢出整数自减。
// 要求：结果是整数 、op1是整数 , 额外规则：无。
// ZEND_FASTCALL ZEND_POST_INC_LONG_NO_OVERFLOW_SPEC，1个分支（验证过）
ZEND_VM_HOT_TYPE_SPEC_HANDLER(ZEND_POST_INC, (res_info == MAY_BE_LONG && op1_info == MAY_BE_LONG), ZEND_POST_INC_LONG_NO_OVERFLOW, CV, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *var_ptr;
	// 获取zval指针, TMP/CONST/UNUSED 返回null，不支持 TMPVAR/TMPVARCV
	var_ptr = GET_OP1_ZVAL_PTR_PTR_UNDEF(BP_VAR_RW);
	// 先把整数复制给结果 
	ZVAL_LONG(EX_VAR(opline->result.var), Z_LVAL_P(var_ptr));
	// 再 +1
	Z_LVAL_P(var_ptr)++;
	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, 自加（后）， 3/3。ZEND_POST_INC_LONG，整数自减。
// 要求：op1是整数 , 额外规则：无。
// ZEND_FASTCALL ZEND_POST_INC_LONG_SPEC，1个分支（验证过）
ZEND_VM_HOT_TYPE_SPEC_HANDLER(ZEND_POST_INC, (op1_info == MAY_BE_LONG), ZEND_POST_INC_LONG, CV, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *var_ptr;
	// 获取zval指针, TMP/CONST/UNUSED 返回null，不支持 TMPVAR/TMPVARCV
	var_ptr = GET_OP1_ZVAL_PTR_PTR_UNDEF(BP_VAR_RW);
	// 先把整数复制给结果 
	ZVAL_LONG(EX_VAR(opline->result.var), Z_LVAL_P(var_ptr));
	// 再 +1
	fast_long_increment_function(var_ptr);
	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, 自减（后）， 2/3。ZEND_POST_DEC_LONG_NO_OVERFLOW，无溢出整数自减。
// 要求：结果是整数 、op1是整数 , 额外规则：无。
// ZEND_FASTCALL ZEND_POST_DEC_LONG_NO_OVERFLOW_SPEC，1个分支（验证过）
ZEND_VM_HOT_TYPE_SPEC_HANDLER(ZEND_POST_DEC, (res_info == MAY_BE_LONG && op1_info == MAY_BE_LONG), ZEND_POST_DEC_LONG_NO_OVERFLOW, CV, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *var_ptr;
	// 获取zval指针, TMP/CONST/UNUSED 返回null，不支持 TMPVAR/TMPVARCV
	var_ptr = GET_OP1_ZVAL_PTR_PTR_UNDEF(BP_VAR_RW);
	// 先把整数复制给结果
	ZVAL_LONG(EX_VAR(opline->result.var), Z_LVAL_P(var_ptr));
	// 把整数 -1
	Z_LVAL_P(var_ptr)--;
	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, 自减（后）， 3/3。ZEND_POST_DEC_LONG，整数自减。
// 要求：op1是整数 , 额外规则：无。
// ZEND_FASTCALL ZEND_POST_DEC_LONG_SPEC，1个分支（验证过）
ZEND_VM_HOT_TYPE_SPEC_HANDLER(ZEND_POST_DEC, (op1_info == MAY_BE_LONG), ZEND_POST_DEC_LONG, CV, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *var_ptr;
	// 获取zval指针, TMP/CONST/UNUSED 返回null，不支持 TMPVAR/TMPVARCV
	var_ptr = GET_OP1_ZVAL_PTR_PTR_UNDEF(BP_VAR_RW);
	// 先把整数复制给结果 （怪不得）
	ZVAL_LONG(EX_VAR(opline->result.var), Z_LVAL_P(var_ptr));
	// 再把整数 -1
	fast_long_decrement_function(var_ptr);
	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, 赋值， 2/4。ZEND_QM_ASSIGN_LONG，整数赋值。
// 要求：op1是整数 , 额外规则：无。
// ZEND_FASTCALL ZEND_QM_ASSIGN_LONG_SPEC，2个分支（验证过）
ZEND_VM_HOT_TYPE_SPEC_HANDLER(ZEND_QM_ASSIGN, (op1_info == MAY_BE_LONG), ZEND_QM_ASSIGN_LONG, CONST|TMPVARCV, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *value;

	// 获取zval指针, UNUSED 返回null
	value = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 取出zval中的整数，赋值给结果变量
	ZVAL_LONG(EX_VAR(opline->result.var), Z_LVAL_P(value));
	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, 赋值， 3/4。ZEND_QM_ASSIGN_DOUBLE，小数赋值。
// 要求：op1是小数 , 额外规则：无。
// ZEND_FASTCALL ZEND_QM_ASSIGN_DOUBLE_SPEC，2个分支（验证过）
ZEND_VM_HOT_TYPE_SPEC_HANDLER(ZEND_QM_ASSIGN, (op1_info == MAY_BE_DOUBLE), ZEND_QM_ASSIGN_DOUBLE, CONST|TMPVARCV, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *value;

	// 获取zval指针, UNUSED 返回null
	value = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 取出zval中的小数，赋值给结果变量
	ZVAL_DOUBLE(EX_VAR(opline->result.var), Z_DVAL_P(value));
	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, 赋值， 4/4。ZEND_QM_ASSIGN_NOREF，非引用赋值。
// 要求：复杂, 额外规则：无。
// ZEND_FASTCALL ZEND_QM_ASSIGN_NOREF_SPEC，2个分支（验证过）
ZEND_VM_HOT_TYPE_SPEC_HANDLER(ZEND_QM_ASSIGN, ((op->op1_type == IS_CONST) ? !Z_REFCOUNTED_P(RT_CONSTANT(op, op->op1)) : (!(op1_info & ((MAY_BE_ANY|MAY_BE_UNDEF)-(MAY_BE_NULL|MAY_BE_FALSE|MAY_BE_TRUE|MAY_BE_LONG|MAY_BE_DOUBLE))))), ZEND_QM_ASSIGN_NOREF, CONST|TMPVARCV, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *value;
	// 取得第一个操作对象里的 zval
	// 获取zval指针, UNUSED 返回null
	value = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 直接复制 zval
	ZVAL_COPY_VALUE(EX_VAR(opline->result.var), value);
	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, 像数组一样读取，的特殊处理
ZEND_VM_HOT_TYPE_SPEC_HANDLER(ZEND_FETCH_DIM_R, (!(op2_info & (MAY_BE_UNDEF|MAY_BE_NULL|MAY_BE_STRING|MAY_BE_ARRAY|MAY_BE_OBJECT|MAY_BE_RESOURCE|MAY_BE_REF))), ZEND_FETCH_DIM_R_INDEX, CONST|TMPVAR|CV, CONST|TMPVARCV, SPEC(NO_CONST_CONST))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *container, *dim, *value;
	zend_long offset;
	HashTable *ht;

	// 获取zval指针, UNUSED 返回null
	container = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 获取zval指针, UNUSED 返回null
	dim = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 如果类型是 数组
	if (EXPECTED(Z_TYPE_P(container) == IS_ARRAY)) {
// 创建跳转标签 fetch_dim_r_index_array
ZEND_VM_C_LABEL(fetch_dim_r_index_array):
		// 如果 维度 类型是 整数
		if (EXPECTED(Z_TYPE_P(dim) == IS_LONG)) {
			// 转成 偏移量，整数
			offset = Z_LVAL_P(dim);
		// 否则
		} else {
			// windows: 无操作
			SAVE_OPLINE();
			// 从容器中 用维度 读取
			zend_fetch_dimension_address_read_R(container, dim, OP2_TYPE OPLINE_CC EXECUTE_DATA_CC);
			// 释放操作对象的附加变量
			FREE_OP1();
			// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
			ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
		}
		// 到这里说明 传入了 整数偏移量
		// 从容器中取出哈希表
		ht = Z_ARRVAL_P(container);
		// 在哈希表中查找元素，找不到跳到 fetch_dim_r_index_undef
		ZEND_HASH_INDEX_FIND(ht, offset, value, ZEND_VM_C_LABEL(fetch_dim_r_index_undef));
		
		// 到这里说明找到了目标原素
		// 复制引用目标 和 附加信息，并增加引用计数
		ZVAL_COPY_DEREF(EX_VAR(opline->result.var), value);
		// 如果 op1 类型 是变量或临时变量
		if (OP1_TYPE & (IS_TMP_VAR|IS_VAR)) {
			// windows: 无操作
			SAVE_OPLINE();
			// 释放操作对象的附加变量
			FREE_OP1();
			// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
			ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
		// 否则
		} else {
			// opline + 1，到目标操作码并 return
			ZEND_VM_NEXT_OPCODE();
		}
	// 否则如果 op1不是常量 并且 窗口类型是 引用
	} else if (OP1_TYPE != IS_CONST && EXPECTED(Z_TYPE_P(container) == IS_REFERENCE)) {
		// 解引用
		container = Z_REFVAL_P(container);
		// 如果类型是 数组
		if (EXPECTED(Z_TYPE_P(container) == IS_ARRAY)) {
			// 跳转到指定标签 fetch_dim_r_index_array
			ZEND_VM_C_GOTO(fetch_dim_r_index_array);
		// 否则
		} else {
			// 跳转到指定标签 fetch_dim_r_index_slow
			ZEND_VM_C_GOTO(fetch_dim_r_index_slow);
		}
	// 否则
	} else {
// 创建跳转标签 fetch_dim_r_index_slow
ZEND_VM_C_LABEL(fetch_dim_r_index_slow):
		// windows: 无操作
		SAVE_OPLINE();
		// 如果op2是常量 并且 维度为 额外值
		if (OP2_TYPE == IS_CONST && Z_EXTRA_P(dim) == ZEND_EXTRA_VALUE) {
			// 找到下一个zval
			dim++;
		}
		// （哈希表以外的）像数组一样读取,结果返回到操作码中，操作类型为 BP_VAR_R。 
		// p1:被读取变量，p2:索引，p3:索引类型
		zend_fetch_dimension_address_read_R_slow(container, dim OPLINE_CC EXECUTE_DATA_CC);
		// 释放操作对象的附加变量
		FREE_OP1();
		// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
		ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
	}

// 创建跳转标签 fetch_dim_r_index_undef
ZEND_VM_C_LABEL(fetch_dim_r_index_undef):
	// 结果为 null
	ZVAL_NULL(EX_VAR(opline->result.var));
	// windows: 无操作
	SAVE_OPLINE();
	// 报错：未定义的数组key（整数）
	zend_undefined_offset(offset);
	// 释放操作对象的附加变量
	FREE_OP1();
	// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, 把op1复制给result，如果不是编变量，增加1次引用
ZEND_VM_HOT_TYPE_SPEC_HANDLER(ZEND_SEND_VAR, op->op2_type == IS_UNUSED && (op1_info & (MAY_BE_UNDEF|MAY_BE_REF)) == 0, ZEND_SEND_VAR_SIMPLE, CV|VAR, NUM)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *varptr, *arg;

	// 获取zval指针, UNUSED 返回null
	varptr = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 把p1 右移p2字节，转成zval返回
	arg = ZEND_CALL_VAR(EX(call), opline->result.var);

	// 如果 op1 是 编译变量
	if (OP1_TYPE == IS_CV) {
		// 复制 zval 变量和附加信息
		ZVAL_COPY(arg, varptr);
	// 否则 如果 op1 是 普通变量
	} else /* if (OP1_TYPE == IS_VAR) */ {
		// 复制 zval 变量和附加信息，并增加引用次数
		ZVAL_COPY_VALUE(arg, varptr);
	}

	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, op1复制给result, 编译变量增加引用次数
ZEND_VM_HOT_TYPE_SPEC_HANDLER(ZEND_SEND_VAR_EX, op->op2_type == IS_UNUSED && op->op2.num <= MAX_ARG_FLAG_NUM && (op1_info & (MAY_BE_UNDEF|MAY_BE_REF)) == 0, ZEND_SEND_VAR_EX_SIMPLE, CV|VAR, UNUSED|NUM)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *varptr, *arg;
	// 参数序号
	uint32_t arg_num = opline->op2.num;

	// 如果可以引用传递
	if (QUICK_ARG_SHOULD_BE_SENT_BY_REF(EX(call)->func, arg_num)) {
		// 跳转到操作码标签 
		ZEND_VM_DISPATCH_TO_HANDLER(ZEND_SEND_REF);
	}

	// 获取zval指针, UNUSED 返回null
	varptr = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// 把p1 右移p2字节，转成zval返回
	arg = ZEND_CALL_VAR(EX(call), opline->result.var);

	// 如果 op1 是 编译变量
	if (OP1_TYPE == IS_CV) {
		// op1复制给result, 并增加引用次数
		ZVAL_COPY(arg, varptr);
	// 否则 如果 op1 是 普通变量
	} else /* if (OP1_TYPE == IS_VAR) */ {
		// op1复制给result
		ZVAL_COPY_VALUE(arg, varptr);
	}

	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, op1 直接复制给 result
ZEND_VM_HOT_TYPE_SPEC_HANDLER(ZEND_SEND_VAL, op->op1_type == IS_CONST && op->op2_type == IS_UNUSED && !Z_REFCOUNTED_P(RT_CONSTANT(op, op->op1)), ZEND_SEND_VAL_SIMPLE, CONST, NUM)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *value, *arg;

	// 获取zval指针，UNUSED 类型返回null
	value = GET_OP1_ZVAL_PTR(BP_VAR_R);
	// 把p1 右移p2字节，转成zval返回
	arg = ZEND_CALL_VAR(EX(call), opline->result.var);
	// op1 直接复制给 result
	ZVAL_COPY_VALUE(arg, value);
	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, 把op1传给result，op2是参数序号。碰到强制引用时报错
ZEND_VM_HOT_TYPE_SPEC_HANDLER(ZEND_SEND_VAL_EX, op->op2_type == IS_UNUSED && op->op2.num <= MAX_ARG_FLAG_NUM && op->op1_type == IS_CONST && !Z_REFCOUNTED_P(RT_CONSTANT(op, op->op1)), ZEND_SEND_VAL_EX_SIMPLE, CONST, NUM)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *value, *arg;
	// 参数序号
	uint32_t arg_num = opline->op2.num;

	// 把p1 右移p2字节，转成zval返回
	arg = ZEND_CALL_VAR(EX(call), opline->result.var);
	// 如果必须通过引用传递
	if (QUICK_ARG_MUST_BE_SENT_BY_REF(EX(call)->func, arg_num)) {
		// 抛错：此参数不可以使用引用传递
		// 调用方法 zend_cannot_pass_by_ref_helper
		ZEND_VM_DISPATCH_TO_HELPER(zend_cannot_pass_by_ref_helper, _arg_num, arg_num, _arg, arg);
	}
	// 获取zval指针，UNUSED 类型返回null
	value = GET_OP1_ZVAL_PTR(BP_VAR_R);
	// 把op1 复制给参数
	ZVAL_COPY_VALUE(arg, value);
	// opline + 1，到目标操作码并 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, 读取方式遍历 数组
ZEND_VM_HOT_TYPE_SPEC_HANDLER(ZEND_FE_FETCH_R, op->op2_type == IS_CV && (op1_info & (MAY_BE_ANY|MAY_BE_REF)) == MAY_BE_ARRAY, ZEND_FE_FETCH_R_SIMPLE, VAR, CV, JMP_ADDR, SPEC(RETVAL))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *array;
	zval *value, *variable_ptr;
	uint32_t value_type;
	HashTable *fe_ht;
	HashPosition pos;

	// 在执行数据中取出 指定序号的变量
	array = EX_VAR(opline->op1.var);
	// windows: 无操作
	SAVE_OPLINE();
	// 取得遍历的哈希表
	fe_ht = Z_ARRVAL_P(array);
	// 通过指针访问zval的 foreach位置。(*p1).u2.fe_pos
	pos = Z_FE_POS_P(array);
	// 如果遍历的是顺序数据
	if (HT_IS_PACKED(fe_ht)) {
		// 取得指定位置的zval
		value = fe_ht->arPacked + pos;
		// 此开始遍历后面所有
		while (1) {
			// 如果位置超过有效位置
			if (UNEXPECTED(pos >= fe_ht->nNumUsed)) {
				// 到达迭代末尾
				/* reached end of iteration */
				// 找到目标操作码，设置成当前操作码并返回。 EX(opline) = (zend_op*)((char*)p1 + p2)
				ZEND_VM_SET_RELATIVE_OPCODE(opline, opline->extended_value);
				// windows: return  0
				ZEND_VM_CONTINUE();
			}
			// 取出 当前值类型
			value_type = Z_TYPE_INFO_P(value);
			// 不可以是间接引用
			ZEND_ASSERT(value_type != IS_INDIRECT);
			// 类型不是 未定义
			if (EXPECTED(value_type != IS_UNDEF)) {
				// 跳出
				break;
			}
			// 迭代位置后移
			pos++;
			// 元素指针后移
			value++;
		}
		// 更新数组内部指针
		// 通过指针访问zval的 foreach位置。(*p1).u2.fe_pos
		Z_FE_POS_P(array) = pos + 1;
		// 如果 操作码的运算结果有效(不是IS_UNUSED)
		if (RETURN_VALUE_USED(opline)) {
			// 位置 放到结果中
			ZVAL_LONG(EX_VAR(opline->result.var), pos);
		}
	// 否则，如果遍历的是哈希表
	} else {
		Bucket *p;
		// 找到指定的 bucket
		p = fe_ht->arData + pos;
		// 向后遍历全部元素
		while (1) {
			// 如果已经到了元素列表末尾
			if (UNEXPECTED(pos >= fe_ht->nNumUsed)) {
				// 到达迭代末尾
				/* reached end of iteration */
				// 找到目标操作码，设置成当前操作码并返回。 EX(opline) = (zend_op*)((char*)p1 + p2)
				ZEND_VM_SET_RELATIVE_OPCODE(opline, opline->extended_value);
				// windows: return  0
				ZEND_VM_CONTINUE();
			}
			// 位置后移
			pos++;
			// bucket 值
			value = &p->val;
			// 值类型
			value_type = Z_TYPE_INFO_P(value);
			// 不可以是间接引用
			ZEND_ASSERT(value_type != IS_INDIRECT);
			// 类型不是 未定义
			if (EXPECTED(value_type != IS_UNDEF)) {
				// 跳出
				break;
			}
			// 下一个bucket 
			p++;
		}
		// 更新哈希表内部指针
		// 通过指针访问zval的 foreach位置。(*p1).u2.fe_pos
		Z_FE_POS_P(array) = pos;
		// 如果 操作码的运算结果有效(不是IS_UNUSED)
		if (RETURN_VALUE_USED(opline)) {
			// 如果没有key
			if (!p->key) {
				// 哈希值作为结果
				ZVAL_LONG(EX_VAR(opline->result.var), p->h);
			// 否则
			} else {
				// 字串复制到结果里
				ZVAL_STR_COPY(EX_VAR(opline->result.var), p->key);
			}
		}
	}

	// 值返回在op2里
	// 在执行数据中取出 指定序号的变量
	variable_ptr = EX_VAR(opline->op2.var);
	// 给变量赋值，p1:变量，p2:值，p3:值的变量类型（常量，变量，编译变量），p4:是否严格类型
	// 某执行上下文所属函数 的参数是否使用严格类型限制
	zend_assign_to_variable(variable_ptr, value, IS_CV, EX_USES_STRICT_TYPES());
	// 在 EX(opline)向右偏移1个位置，找到目标操作码并 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing2, zend_vm_gen.php处理
ZEND_VM_DEFINE_OP(137, ZEND_OP_DATA);

// ing3，检查超时并且调用 zend_interrupt_function 函数
ZEND_VM_HELPER(zend_interrupt_helper, ANY, ANY)
{
	// EG(vm_interrupt) = false
	zend_atomic_bool_store_ex(&EG(vm_interrupt), false);
	// windows: 无操作
	SAVE_OPLINE();
	// if(EG(timed_out))
	if (zend_atomic_bool_load_ex(&EG(timed_out))) {
		// 超时
		zend_timeout();
	// 如果没超时，并且有 zend_interrupt_function函数
	} else if (zend_interrupt_function) {
		// 调用这个函数
		// windows : php_win32_signal_ctrl_interrupt_function
		zend_interrupt_function(execute_data);
		// 如果有异常
		if (EG(exception)) {
			// 必须把结果设置成 未定义，因为 ZEND_HANDLE_EXCEPTION 会释放它
			/* We have to UNDEF result, because ZEND_HANDLE_EXCEPTION is going to free it */
			// 抛异常的操作码
			const zend_op *throw_op = EG(opline_before_exception);
			// 如果操作码存在 并且 结果 是（临时变量 或 变量）
			if (throw_op
			 && throw_op->result_type & (IS_TMP_VAR|IS_VAR)
			// 并且操作码不是 ZEND_ADD_ARRAY_ELEMENT，ZEND_ADD_ARRAY_UNPACK，ZEND_ROPE_INIT，ZEND_ROPE_ADD
			 && throw_op->opcode != ZEND_ADD_ARRAY_ELEMENT
			 && throw_op->opcode != ZEND_ADD_ARRAY_UNPACK
			 && throw_op->opcode != ZEND_ROPE_INIT
			 && throw_op->opcode != ZEND_ROPE_ADD) {
				// 把p1 右移p2字节，转成zval返回  ((zval*)((char*)(p1) + p2))
				ZVAL_UNDEF(ZEND_CALL_VAR(EG(current_execute_data), throw_op->result.var));

			}
		}
		// ing3, windows return 1;
		ZEND_VM_ENTER();
	}
	// windows: return  0
	ZEND_VM_CONTINUE();
}
