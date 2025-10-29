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

// �޸Ĵ��ļ���Ҫ���� php zend_vm_gen.php 
// ����ļ�û�б��κγ������ã�������Ϊ������ zend_vm_opcodes.h�������治�ǿ����е�c���򣬶���һЩ�ؼ����ݵ���д
// �������� the zend_vm_execute.h �� zend_vm_opcodes.h

// zend_vm �����������еĸ��ӣ�����Ϊ��������ر����Ż����������ۻ����ң�ʵ���ϲ�û�У�ֻ�Ƿָ�����������������ѡ�
	// VM ֮���Խ� VM������Ϊ���ر��ӣ����ر��Ļ����ӿڻ�ײ��Ż���������Ϊ��ִ�в�����
	// ����ִ���Լ���ָ������ԲŽ�VM
	
// ��������ڵ�������ʲô�أ�����Ҫ�������Ƕ�ҵ���߼����г��󣬽�����ϣ����������Ӳ����
	// �����������ֳɱ��벿�ֺ�ִ�в��֣�ͨ��������ָ
	// ���������ڱ�������ɣ������Ա����棬������Ч�ʱ�ø���

// �� 350 ��������������Ǻ��ģ���Ҫ�� zend_vm_execute.h �ŵ�
// 239�� handler, 39��helper. helper ��ʵ��������handler ���õķ����������������ط��õ���

// SPEC_HANDLER �����⴦����

// ing3, �ӷ����� , ���鴫��ı����Ƿ���Ч�����мӷ����㣬Ȼ�������������op1,op2 
ZEND_VM_HELPER(zend_add_helper, ANY, ANY, zval *op_1, zval *op_2)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	SAVE_OPLINE();
	// ��� ��������Ϊδ����
	if (UNEXPECTED(Z_TYPE_INFO_P(op_1) == IS_UNDEF)) {
		// ����p1�ı�������������δ����, ����δ��ʼ��zval
		op_1 = ZVAL_UNDEFINED_OP1();
	}
	// ��� ��������Ϊ δ����
	if (UNEXPECTED(Z_TYPE_INFO_P(op_2) == IS_UNDEF)) {
		// ����p2�ı�������������δ����, ����δ��ʼ��zval
		op_2 = ZVAL_UNDEFINED_OP2();
	}
	// �ӷ����� 
	add_function(EX_VAR(opline->result.var), op_1, op_2);
	// ��� op1 ���� �Ǳ�������ʱ����
	if (OP1_TYPE & (IS_TMP_VAR|IS_VAR)) {
		// ����û�����ô����Ķ���,��û������ zval����������gc�������ڡ�
		zval_ptr_dtor_nogc(op_1);
	}
	// ��� op2 ���� �Ǳ�������ʱ����
	if (OP2_TYPE & (IS_TMP_VAR|IS_VAR)) {
		// ����û�����ô����Ķ���,��û������ zval����������gc�������ڡ�
		zval_ptr_dtor_nogc(op_2);
	}
	// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, �ӷ����� 1/4
ZEND_VM_HOT_NOCONSTCONST_HANDLER(1, ZEND_ADD, CONST|TMPVARCV, CONST|TMPVARCV)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2, *result;
	double d1, d2;

	// ��ȡzvalָ��, UNUSED ����null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ��ȡzvalָ��, UNUSED ����null
	op2 = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// op1��op2��һ������ANY ���� ����������ǳ���
	if (ZEND_VM_SPEC && OP1_TYPE == IS_CONST && OP2_TYPE == IS_CONST) {
		/* pass */
	// ��� op1 ������
	} else if (EXPECTED(Z_TYPE_INFO_P(op1) == IS_LONG)) {
		// ��� op2 Ҳ������
		if (EXPECTED(Z_TYPE_INFO_P(op2) == IS_LONG)) {
			// ��ִ��������ȡ�� ָ����ŵı���
			result = EX_VAR(opline->result.var);
			// ���ټӷ�
			fast_long_add_function(result, op1, op2);
			// opline + 1����Ŀ������벢 return
			ZEND_VM_NEXT_OPCODE();
		// ��� op2 ��˫����
		} else if (EXPECTED(Z_TYPE_INFO_P(op2) == IS_DOUBLE)) {
			// op1 ��ֵת�� ˫���� 
			d1 = (double)Z_LVAL_P(op1);
			// ȡ��С��
			d2 = Z_DVAL_P(op2);
			// ��ת��ָ����ǩ add_double
			ZEND_VM_C_GOTO(add_double);
		}
	// ��� op1 ��˫����
	} else if (EXPECTED(Z_TYPE_INFO_P(op1) == IS_DOUBLE)) {
		// ��� op2 ��˫����
		if (EXPECTED(Z_TYPE_INFO_P(op2) == IS_DOUBLE)) {
			// ȡ��С��
			d1 = Z_DVAL_P(op1);
			// ȡ��С��
			d2 = Z_DVAL_P(op2);
// ������ת��ǩ add_double
ZEND_VM_C_LABEL(add_double):
			// ��ִ��������ȡ�� ָ����ŵı���
			result = EX_VAR(opline->result.var);
			// ���ӷ�����ֵ������� 
			ZVAL_DOUBLE(result, d1 + d2);
			// opline + 1����Ŀ������벢 return
			ZEND_VM_NEXT_OPCODE();
		// ��� op2 ������
		} else if (EXPECTED(Z_TYPE_INFO_P(op2) == IS_LONG)) {
			// ȡ��С��
			d1 = Z_DVAL_P(op1);
			// op2��ֵת��˫����
			d2 = (double)Z_LVAL_P(op2);
			// ��ת��ָ����ǩ add_double
			ZEND_VM_C_GOTO(add_double);
		}
	}
	
	// ���÷��� zend_add_helper
	ZEND_VM_DISPATCH_TO_HELPER(zend_add_helper, op_1, op1, op_2, op2);
}

// ing3, �������� , ���鴫��ı����Ƿ���Ч�����м������㣬Ȼ�������������op1,op2 
ZEND_VM_HELPER(zend_sub_helper, ANY, ANY, zval *op_1, zval *op_2)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// windows: �޲���
	SAVE_OPLINE();
	
	// ��� ��������Ϊδ����
	if (UNEXPECTED(Z_TYPE_INFO_P(op_1) == IS_UNDEF)) {
		// ����p1�ı�������������δ����, ����δ��ʼ��zval
		op_1 = ZVAL_UNDEFINED_OP1();
	}
	// ��� ��������Ϊ δ����
	if (UNEXPECTED(Z_TYPE_INFO_P(op_2) == IS_UNDEF)) {
		// ����p2�ı�������������δ����, ����δ��ʼ��zval
		op_2 = ZVAL_UNDEFINED_OP2();
	}
	// ����������ֵ�浽�����
	sub_function(EX_VAR(opline->result.var), op_1, op_2);
	// ��� op1 ���� �Ǳ�������ʱ����
	if (OP1_TYPE & (IS_TMP_VAR|IS_VAR)) {
		// ����û�����ô����Ķ���,��û������ zval����������gc�������ڡ�
		zval_ptr_dtor_nogc(op_1);
	}
	// ��� op2 ���� �Ǳ�������ʱ����
	if (OP2_TYPE & (IS_TMP_VAR|IS_VAR)) {
		// ����û�����ô����Ķ���,��û������ zval����������gc�������ڡ�
		zval_ptr_dtor_nogc(op_2);
	}
	// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, ���� 1/4
ZEND_VM_HOT_NOCONSTCONST_HANDLER(2, ZEND_SUB, CONST|TMPVARCV, CONST|TMPVARCV)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2, *result;
	double d1, d2;

	// ��ȡzvalָ��, UNUSED ����null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ��ȡzvalָ��, UNUSED ����null
	op2 = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// op1��op2��һ������ANY �����������ǳ���
	if (ZEND_VM_SPEC && OP1_TYPE == IS_CONST && OP2_TYPE == IS_CONST) {
		/* pass */
	// ���op1������
	} else if (EXPECTED(Z_TYPE_INFO_P(op1) == IS_LONG)) {
		// ���op2������
		if (EXPECTED(Z_TYPE_INFO_P(op2) == IS_LONG)) {
			// ��ִ��������ȡ�� ָ����ŵı���
			result = EX_VAR(opline->result.var);
			// ���ټ���
			fast_long_sub_function(result, op1, op2);
			// opline + 1����Ŀ������벢 return
			ZEND_VM_NEXT_OPCODE();
		// ���op2��С��	
		} else if (EXPECTED(Z_TYPE_INFO_P(op2) == IS_DOUBLE)) {
			// op1ת��С��
			d1 = (double)Z_LVAL_P(op1);
			// ȡ��С��
			d2 = Z_DVAL_P(op2);
			// ��ת��ָ����ǩ sub_double
			ZEND_VM_C_GOTO(sub_double);
		}
	// ���op1��С��
	} else if (EXPECTED(Z_TYPE_INFO_P(op1) == IS_DOUBLE)) {
		// ���op2��С��
		if (EXPECTED(Z_TYPE_INFO_P(op2) == IS_DOUBLE)) {
			// ȡ��С��
			d1 = Z_DVAL_P(op1);
			// ȡ��С��
			d2 = Z_DVAL_P(op2);
// ������ת��ǩ sub_double
ZEND_VM_C_LABEL(sub_double):
			// ��ִ��������ȡ�� ָ����ŵı���
			result = EX_VAR(opline->result.var);
			// ����������ΪС��
			ZVAL_DOUBLE(result, d1 - d2);
			// opline + 1����Ŀ������벢 return
			ZEND_VM_NEXT_OPCODE();
		// ���� ��� op2 ������
		} else if (EXPECTED(Z_TYPE_INFO_P(op2) == IS_LONG)) {
			// ȡ��С��
			d1 = Z_DVAL_P(op1);
			// ȡ��С��
			d2 = (double)Z_LVAL_P(op2);
			// ��ת��ָ����ǩ sub_double
			ZEND_VM_C_GOTO(sub_double);
		}
	}

	// ���÷��� zend_sub_helper
	ZEND_VM_DISPATCH_TO_HELPER(zend_sub_helper, op_1, op1, op_2, op2);
}

// ing3, �˷�����, ���鴫��ı����Ƿ���Ч�����г˷����㣬Ȼ�������������op1,op2 
ZEND_VM_HELPER(zend_mul_helper, ANY, ANY, zval *op_1, zval *op_2)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// windows: �޲���
	SAVE_OPLINE();
	// ��� ��������Ϊδ����
	if (UNEXPECTED(Z_TYPE_INFO_P(op_1) == IS_UNDEF)) {
		// ����p1�ı�������������δ����, ����δ��ʼ��zval
		op_1 = ZVAL_UNDEFINED_OP1();
	}
	// ��� ��������Ϊ δ����
	if (UNEXPECTED(Z_TYPE_INFO_P(op_2) == IS_UNDEF)) {
		// ����p2�ı�������������δ����, ����δ��ʼ��zval
		op_2 = ZVAL_UNDEFINED_OP2();
	}
	// ���г˷�����
	mul_function(EX_VAR(opline->result.var), op_1, op_2);
	// ��� op1 ���� �Ǳ�������ʱ����
	if (OP1_TYPE & (IS_TMP_VAR|IS_VAR)) {
		// ����û�����ô����Ķ���,��û������ zval����������gc�������ڡ�
		zval_ptr_dtor_nogc(op_1);
	}
	// ��� op2 ���� �Ǳ�������ʱ����
	if (OP2_TYPE & (IS_TMP_VAR|IS_VAR)) {
		// ����û�����ô����Ķ���,��û������ zval����������gc�������ڡ�
		zval_ptr_dtor_nogc(op_2);
	}
	// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ZEND_MUL���˷� 1/4�� ����������֧����֤������
// ing3, �˷�
ZEND_VM_COLD_CONSTCONST_HANDLER(3, ZEND_MUL, CONST|TMPVARCV, CONST|TMPVARCV, SPEC(COMMUTATIVE))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2, *result;
	double d1, d2;

	// ��ȡzvalָ��, UNUSED ����null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ��ȡzvalָ��, UNUSED ����null
	op2 = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// op1��op2��һ������ANY
	if (ZEND_VM_SPEC && OP1_TYPE == IS_CONST && OP2_TYPE == IS_CONST) {
		/* pass */
	// ���op1������
	} else if (EXPECTED(Z_TYPE_INFO_P(op1) == IS_LONG)) {
		// ���op2������
		if (EXPECTED(Z_TYPE_INFO_P(op2) == IS_LONG)) {
			// ������
			zend_long overflow;
			// ��ִ��������ȡ�� ָ����ŵı���
			result = EX_VAR(opline->result.var);
			// ���������˷�
			ZEND_SIGNED_MULTIPLY_LONG(Z_LVAL_P(op1), Z_LVAL_P(op2), Z_LVAL_P(result), Z_DVAL_P(result), overflow);
			// �������ˣ����ת��С��
			Z_TYPE_INFO_P(result) = overflow ? IS_DOUBLE : IS_LONG;
			// opline + 1����Ŀ������벢 return
			ZEND_VM_NEXT_OPCODE();
			
		// ���op2��С��
		} else if (EXPECTED(Z_TYPE_INFO_P(op2) == IS_DOUBLE)) {
			// op1ת��С��
			d1 = (double)Z_LVAL_P(op1);
			// ȡ��С��
			d2 = Z_DVAL_P(op2);
			// ��ת��ָ����ǩ mul_double
			ZEND_VM_C_GOTO(mul_double);
		}
	// ���op1��С��
	} else if (EXPECTED(Z_TYPE_INFO_P(op1) == IS_DOUBLE)) {
		// ��� op2 ��С��
		if (EXPECTED(Z_TYPE_INFO_P(op2) == IS_DOUBLE)) {
			// ȡ��С��
			d1 = Z_DVAL_P(op1);
			// ȡ��С��
			d2 = Z_DVAL_P(op2);
// ������ת��ǩ mul_double
ZEND_VM_C_LABEL(mul_double):
			// ��ִ��������ȡ�� ָ����ŵı���
			result = EX_VAR(opline->result.var);
			// ֱ����˷������ΪС��
			ZVAL_DOUBLE(result, d1 * d2);
			// opline + 1����Ŀ������벢 return
			ZEND_VM_NEXT_OPCODE();
		// ��� op2 ������
		} else if (EXPECTED(Z_TYPE_INFO_P(op2) == IS_LONG)) {
			// ȡ��С��
			d1 = Z_DVAL_P(op1);
			// op2ת��С��
			d2 = (double)Z_LVAL_P(op2);
			// ��ת��ָ����ǩ mul_double
			ZEND_VM_C_GOTO(mul_double);
		}
	}

	// ���÷��� zend_mul_helper
	ZEND_VM_DISPATCH_TO_HELPER(zend_mul_helper, op_1, op1, op_2, op2);
}

// ing3, ���� 1/1 
// ����9����֧����֤����
ZEND_VM_COLD_CONSTCONST_HANDLER(4, ZEND_DIV, CONST|TMPVAR|CV, CONST|TMPVAR|CV)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2;

	// windows: �޲���
	SAVE_OPLINE();
	// ��ȡzvalָ�룬UNUSED ���ͷ���null
	op1 = GET_OP1_ZVAL_PTR(BP_VAR_R);
	// ��ȡzvalָ�룬UNUSED ���ͷ���null
	op2 = GET_OP2_ZVAL_PTR(BP_VAR_R);
	// �������� 
	div_function(EX_VAR(opline->result.var), op1, op2);
	FREE_OP1();
	// �ͷŲ�������ĸ��ӱ���
	FREE_OP2();
	// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, ����0������ ��1����֧����֤����
ZEND_VM_COLD_HELPER(zend_mod_by_zero_helper, ANY, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// windows: �޲���
	SAVE_OPLINE();
	// ���쳣
	zend_throw_exception_ex(zend_ce_division_by_zero_error, 0, "Modulo by zero");
	// ���Ϊ δ����
	ZVAL_UNDEF(EX_VAR(opline->result.var));
	// ��Ҫ�� return
	HANDLE_EXCEPTION();
}

// ing3, ȡ������ , ���鴫��ı����Ƿ���Ч������ȡ�����㣬Ȼ�������������op1,op2 
ZEND_VM_HELPER(zend_mod_helper, ANY, ANY, zval *op_1, zval *op_2)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// windows: �޲���
	SAVE_OPLINE();
	// ��� ��������Ϊδ����
	if (UNEXPECTED(Z_TYPE_INFO_P(op_1) == IS_UNDEF)) {
		// ����p1�ı�������������δ����, ����δ��ʼ��zval
		op_1 = ZVAL_UNDEFINED_OP1();
	}
	// ��� ��������Ϊ δ����
	if (UNEXPECTED(Z_TYPE_INFO_P(op_2) == IS_UNDEF)) {
		// ����p2�ı�������������δ����, ����δ��ʼ��zval
		op_2 = ZVAL_UNDEFINED_OP2();
	}
	// ����ȡ������
	mod_function(EX_VAR(opline->result.var), op_1, op_2);
	// ��� op1 ���� �Ǳ�������ʱ����
	if (OP1_TYPE & (IS_TMP_VAR|IS_VAR)) {
		// ����û�����ô����Ķ���,��û������ zval����������gc�������ڡ�
		zval_ptr_dtor_nogc(op_1);
	}
	// ��� op2 ���� �Ǳ�������ʱ����
	if (OP2_TYPE & (IS_TMP_VAR|IS_VAR)) {
		// ����û�����ô����Ķ���,��û������ zval����������gc�������ڡ�
		zval_ptr_dtor_nogc(op_2);
	}
	// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, ȡ������
ZEND_VM_COLD_CONSTCONST_HANDLER(5, ZEND_MOD, CONST|TMPVARCV, CONST|TMPVARCV)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2, *result;

	// ��ȡzvalָ��, UNUSED ����null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ��ȡzvalָ��, UNUSED ����null
	op2 = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// op1��op2��һ������ANY ���� �������ǳ���
	if (ZEND_VM_SPEC && OP1_TYPE == IS_CONST && OP2_TYPE == IS_CONST) {
		/* pass */
	// ���op2�������� 
	} else if (EXPECTED(Z_TYPE_INFO_P(op1) == IS_LONG)) {
		// ��� op2 ������ 
		if (EXPECTED(Z_TYPE_INFO_P(op2) == IS_LONG)) {
			// ��ִ��������ȡ�� ָ����ŵı���
			result = EX_VAR(opline->result.var);
			// �����������2ֵΪ0
			if (UNEXPECTED(Z_LVAL_P(op2) == 0)) {
				// ���÷��� zend_mod_by_zero_helper
				ZEND_VM_DISPATCH_TO_HELPER(zend_mod_by_zero_helper);
			// �����������2 ֵΪ -1
			} else if (UNEXPECTED(Z_LVAL_P(op2) == -1)) {
				// ���op1ֵΪ ZEND_LONG_MIN����ֹ �������
				/* Prevent overflow error/crash if op1==ZEND_LONG_MIN */
				// ���Ϊ0
				ZVAL_LONG(result, 0);
			// ����
			} else {
				// ֱ��ȡ������
				ZVAL_LONG(result, Z_LVAL_P(op1) % Z_LVAL_P(op2));
			}
			// opline + 1����Ŀ������벢 return
			ZEND_VM_NEXT_OPCODE();
		}
	}

	// ȡ������ , ���鴫��ı����Ƿ���Ч������ȡ�����㣬Ȼ�������������op1,op2 
	// ���÷��� zend_mod_helper
	ZEND_VM_DISPATCH_TO_HELPER(zend_mod_helper, op_1, op1, op_2, op2);
}

// ing3, ������������ , ���鴫��ı����Ƿ���Ч�������������㣬Ȼ�������������op1,op2 
ZEND_VM_HELPER(zend_shift_left_helper, ANY, ANY, zval *op_1, zval *op_2)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// windows: �޲���
	SAVE_OPLINE();
	// ��� ��������Ϊδ����
	if (UNEXPECTED(Z_TYPE_INFO_P(op_1) == IS_UNDEF)) {
		// ����p1�ı�������������δ����, ����δ��ʼ��zval
		op_1 = ZVAL_UNDEFINED_OP1();
	}
	// ��� ��������Ϊ δ����
	if (UNEXPECTED(Z_TYPE_INFO_P(op_2) == IS_UNDEF)) {
		// ����p2�ı�������������δ����, ����δ��ʼ��zval
		op_2 = ZVAL_UNDEFINED_OP2();
	}
	// ������������
	shift_left_function(EX_VAR(opline->result.var), op_1, op_2);
	// ��� op1 ���� �Ǳ�������ʱ����
	if (OP1_TYPE & (IS_TMP_VAR|IS_VAR)) {
		// ����û�����ô����Ķ���,��û������ zval����������gc�������ڡ�
		zval_ptr_dtor_nogc(op_1);
	}
	// ��� op2 ���� �Ǳ�������ʱ����
	if (OP2_TYPE & (IS_TMP_VAR|IS_VAR)) {
		// ����û�����ô����Ķ���,��û������ zval����������gc�������ڡ�
		zval_ptr_dtor_nogc(op_2);
	}
	// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// 4����֧����֤����
// ing3, ����
ZEND_VM_COLD_CONSTCONST_HANDLER(6, ZEND_SL, CONST|TMPVARCV, CONST|TMPVARCV)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2;

	// ��ȡzvalָ��, UNUSED ����null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ��ȡzvalָ��, UNUSED ����null
	op2 = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// op1��op2��һ������ANY
	// ����������ǳ�����ʲôҲ��������ΪӦ�ô������
	if (ZEND_VM_SPEC && OP1_TYPE == IS_CONST && OP2_TYPE == IS_CONST) {
		/* pass */
	// ��������������� ���� op2 С��һ������������λ�������ƶ�������Ϊ0��
	} else if (EXPECTED(Z_TYPE_INFO_P(op1) == IS_LONG)
			&& EXPECTED(Z_TYPE_INFO_P(op2) == IS_LONG)
			&& EXPECTED((zend_ulong)Z_LVAL_P(op2) < SIZEOF_ZEND_LONG * 8)) {
		
		// ת���޷���������λ�ƣ����Եõ�һ�����õķ�װ��Ϊ��
		/* Perform shift on unsigned numbers to get well-defined wrap behavior. */
		
		// ��op1ת���޷��ţ�����λ�����㣬��ת����
		ZVAL_LONG(EX_VAR(opline->result.var),
			(zend_long) ((zend_ulong) Z_LVAL_P(op1) << Z_LVAL_P(op2)));
		// opline + 1����Ŀ������벢 return
		ZEND_VM_NEXT_OPCODE();
	}

	// ���÷��� zend_shift_left_helper
	ZEND_VM_DISPATCH_TO_HELPER(zend_shift_left_helper, op_1, op1, op_2, op2);
}

// ing3, ������������, ���鴫��ı����Ƿ���Ч�������������㣬Ȼ�������������op1,op2 
ZEND_VM_HELPER(zend_shift_right_helper, ANY, ANY, zval *op_1, zval *op_2)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// windows: �޲���
	SAVE_OPLINE();
	// ��� ��������Ϊδ����
	if (UNEXPECTED(Z_TYPE_INFO_P(op_1) == IS_UNDEF)) {
		// ����p1�ı�������������δ����, ����δ��ʼ��zval
		op_1 = ZVAL_UNDEFINED_OP1();
	}
	// ��� ��������Ϊ δ����
	if (UNEXPECTED(Z_TYPE_INFO_P(op_2) == IS_UNDEF)) {
		// ����p2�ı�������������δ����, ����δ��ʼ��zval
		op_2 = ZVAL_UNDEFINED_OP2();
	}
	// ������������
	shift_right_function(EX_VAR(opline->result.var), op_1, op_2);
	// ��� op1 ���� �Ǳ�������ʱ����
	if (OP1_TYPE & (IS_TMP_VAR|IS_VAR)) {
		// ����û�����ô����Ķ���,��û������ zval����������gc�������ڡ�
		zval_ptr_dtor_nogc(op_1);
	}
	// ��� op2 ���� �Ǳ�������ʱ����
	if (OP2_TYPE & (IS_TMP_VAR|IS_VAR)) {
		// ����û�����ô����Ķ���,��û������ zval����������gc�������ڡ�
		zval_ptr_dtor_nogc(op_2);
	}
	// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// 2*2 = 4����֧
// ing3, ��������
ZEND_VM_COLD_CONSTCONST_HANDLER(7, ZEND_SR, CONST|TMPVARCV, CONST|TMPVARCV)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2;

	// ��ȡzvalָ��, UNUSED ����null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ��ȡzvalָ��, UNUSED ����null
	op2 = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// op1��op2��һ������ANY
	if (ZEND_VM_SPEC && OP1_TYPE == IS_CONST && OP2_TYPE == IS_CONST) {
		/* pass */
	// ��������������� ���� op2 С��һ������������λ�������ƶ�������Ϊ0��
	} else if (EXPECTED(Z_TYPE_INFO_P(op1) == IS_LONG)
			&& EXPECTED(Z_TYPE_INFO_P(op2) == IS_LONG)
			&& EXPECTED((zend_ulong)Z_LVAL_P(op2) < SIZEOF_ZEND_LONG * 8)) {
		// ֱ�ӽ���λ������
		ZVAL_LONG(EX_VAR(opline->result.var), Z_LVAL_P(op1) >> Z_LVAL_P(op2));
		// opline + 1����Ŀ������벢 return
		ZEND_VM_NEXT_OPCODE();
	}

	// ������������, ���鴫��ı����Ƿ���Ч�������������㣬Ȼ�������������op1,op2 
	// ���÷��� zend_shift_right_helper
	ZEND_VM_DISPATCH_TO_HELPER(zend_shift_right_helper, op_1, op1, op_2, op2);
}

// ing3, ������
ZEND_VM_COLD_CONSTCONST_HANDLER(12, ZEND_POW, CONST|TMPVAR|CV, CONST|TMPVAR|CV)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2;

	// windows: �޲���
	SAVE_OPLINE();
	// ��ȡzvalָ�룬UNUSED ���ͷ���null
	op1 = GET_OP1_ZVAL_PTR(BP_VAR_R);
	// ��ȡzvalָ�룬UNUSED ���ͷ���null
	op2 = GET_OP2_ZVAL_PTR(BP_VAR_R);
	// ��������
	pow_function(EX_VAR(opline->result.var), op1, op2);
	// �ͷŲ�������ĸ��ӱ���
	FREE_OP1();
	// �ͷŲ�������ĸ��ӱ���
	FREE_OP2();
	// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, �����ִ� 1/1��������򣺲����������ǳ�����3*3-1 = 8����֧��ȷ�Ϲ���
// Ϊʲô�������������أ���Ϊ���������ò��������룬ֱ���ڱ���ʱ���Ӿ�����
ZEND_VM_HANDLER(8, ZEND_CONCAT, CONST|TMPVAR|CV, CONST|TMPVAR|CV, SPEC(NO_CONST_CONST))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2;

	// ȡ���������������е����ñ���
	// ��ȡzvalָ��, UNUSED ����null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ��ȡzvalָ��, UNUSED ����null
	op2 = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);

	// �������Ͷ��ǣ����� �� �ִ�
	if ((OP1_TYPE == IS_CONST || EXPECTED(Z_TYPE_P(op1) == IS_STRING)) &&
	    (OP2_TYPE == IS_CONST || EXPECTED(Z_TYPE_P(op2) == IS_STRING))) {
		// ȡ�������ִ�
		zend_string *op1_str = Z_STR_P(op1);
		zend_string *op2_str = Z_STR_P(op2);
		zend_string *str;

		// ��� op1���ǳ��� �����ִ�������0��op1�ɺ���
		if (OP1_TYPE != IS_CONST && UNEXPECTED(ZSTR_LEN(op1_str) == 0)) {
			// op2�����ǳ��� �� �������
			if (OP2_TYPE == IS_CONST || OP2_TYPE == IS_CV) {
				// op2���ִ����ɽ��
				ZVAL_STR_COPY(EX_VAR(opline->result.var), op2_str);
			// op2��������ʱ���� TMPVAR
			// ����
			} else {
				// op2���ִ����ɽ��
				ZVAL_STR(EX_VAR(opline->result.var), op2_str);
			}
			// ��� op1 ���� �Ǳ�������ʱ����
			if (OP1_TYPE & (IS_TMP_VAR|IS_VAR)) {
				// �ͷ�op1�ִ�
				zend_string_release_ex(op1_str, 0);
			}
			// �������ˣ�
		// ��� op2���ǳ��� �����ִ������� 0��op2�ɺ���
		} else if (OP2_TYPE != IS_CONST && UNEXPECTED(ZSTR_LEN(op2_str) == 0)) {
			// op1�����ǳ��� �� �������
			if (OP1_TYPE == IS_CONST || OP1_TYPE == IS_CV) {
				// op1���ִ����ɽ��
				ZVAL_STR_COPY(EX_VAR(opline->result.var), op1_str);
			// op1��������ʱ���� TMPVAR	
			// ����
			} else {
				ZVAL_STR(EX_VAR(opline->result.var), op1_str);
			}
			// ��� op2 ���� �Ǳ�������ʱ����
			if (OP2_TYPE & (IS_TMP_VAR|IS_VAR)) {
				// �ͷ�op2�ִ�
				zend_string_release_ex(op2_str, 0);
			}
		// ��� op1,op2�����ɺ��ԡ����op1���ǳ���Ҳ���Ǳ������ ������ֵҲ���Ǳ����֣��������ô�����1
		} else if (OP1_TYPE != IS_CONST && OP1_TYPE != IS_CV &&
		    !ZSTR_IS_INTERNED(op1_str) && GC_REFCOUNT(op1_str) == 1) {
			// �ִ�����
			size_t len = ZSTR_LEN(op1_str);
			// ��� ���Ӻ󳤶Ȼ�����ִ���󳤶�
			if (UNEXPECTED(len > ZSTR_MAX_LEN - ZSTR_LEN(op2_str))) {
				// �����ִ�����
				zend_error_noreturn(E_ERROR, "Integer overflow in memory allocation");
			}
			// ��op1���ִ����ӳ���
			str = zend_string_extend(op1_str, len + ZSTR_LEN(op2_str), 0);
			// ����һ�θ���op2������
			memcpy(ZSTR_VAL(str) + len, ZSTR_VAL(op2_str), ZSTR_LEN(op2_str)+1);
			// ��zend_string ��Ӹ� zval����֧�ֱ�����
			ZVAL_NEW_STR(EX_VAR(opline->result.var), str);
			// ��� op2 ���� �Ǳ�������ʱ����
			if (OP2_TYPE & (IS_TMP_VAR|IS_VAR)) {
				// �ͷ�op2�ִ�
				zend_string_release_ex(op2_str, 0);
			}
		// ���������
		// ����
		} else {
			// �������ִ��������������ִ��ĺ�
			str = zend_string_alloc(ZSTR_LEN(op1_str) + ZSTR_LEN(op2_str), 0);
			// �������ִ����Ƶ��´���
			memcpy(ZSTR_VAL(str), ZSTR_VAL(op1_str), ZSTR_LEN(op1_str));
			memcpy(ZSTR_VAL(str) + ZSTR_LEN(op1_str), ZSTR_VAL(op2_str), ZSTR_LEN(op2_str)+1);
			// ��zend_string ��Ӹ� zval����֧�ֱ�����
			ZVAL_NEW_STR(EX_VAR(opline->result.var), str);
			// ��� op1 ���� �Ǳ�������ʱ����
			if (OP1_TYPE & (IS_TMP_VAR|IS_VAR)) {
				// �ͷ�op1�ִ�
				zend_string_release_ex(op1_str, 0);
			}
			// ��� op2 ���� �Ǳ�������ʱ����
			if (OP2_TYPE & (IS_TMP_VAR|IS_VAR)) {
				// �ͷ�op2�ִ�
				zend_string_release_ex(op2_str, 0);
			}
		}
		// ��һ��������
		// opline + 1����Ŀ������벢 return
		ZEND_VM_NEXT_OPCODE();
	// �������
	// ����
	} else {
		// windows: �޲���
		SAVE_OPLINE();

		// ����������������Ǳ������ �����ñ��������� δ����
		if (OP1_TYPE == IS_CV && UNEXPECTED(Z_TYPE_P(op1) == IS_UNDEF)) {
			// ����p1�ı�������������δ����, ����δ��ʼ��zval
			op1 = ZVAL_UNDEFINED_OP1();
		}
		// ��� op2 �Ǳ������ ���� ������ δ����
		if (OP2_TYPE == IS_CV && UNEXPECTED(Z_TYPE_P(op2) == IS_UNDEF)) {
			// ����p2�ı�������������δ����, ����δ��ʼ��zval
			op2 = ZVAL_UNDEFINED_OP2();
		}
		// �����������ñ��������ѽ�����ظ� result
		concat_function(EX_VAR(opline->result.var), op1, op2);
		// �ͷŲ�������ĸ��ӱ���
		FREE_OP1();
		// �ͷŲ�������ĸ��ӱ���
		FREE_OP2();
		// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
		ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
	}
}

// ing3, �Ƚ���ͬ���㣬Ȼ�������������op1,op2 
ZEND_VM_COLD_CONSTCONST_HANDLER(16, ZEND_IS_IDENTICAL, CONST|TMP|VAR|CV, CONST|TMP|VAR|CV, SPEC(COMMUTATIVE))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2;
	bool result;

	// windows: �޲���
	SAVE_OPLINE();
	// ��ȡzvalָ��, UNUSED ����null����֧�� TMPVAR/TMPVARCV
	op1 = GET_OP1_ZVAL_PTR_DEREF(BP_VAR_R);
	// ��ȡzvalָ��, UNUSED ����null����֧�� TMPVAR/TMPVARCV
	op2 = GET_OP2_ZVAL_PTR_DEREF(BP_VAR_R);
	// ����Ƿ���ͬ
	result = fast_is_identical_function(op1, op2);
	// �ͷŲ�������ĸ��ӱ���
	FREE_OP1();
	// �ͷŲ�������ĸ��ӱ���
	FREE_OP2();
	// ���ݵ�ǰ��������ѡ����һ�������룬p1:����ֵ����Ҫ�������жϡ�p2:�Ƿ����쳣
	ZEND_VM_SMART_BRANCH(result, 1);
}

// ing3, case��� �Ƚ���ͬ���㣬Ȼ������������� op2 
ZEND_VM_HANDLER(196, ZEND_CASE_STRICT, TMP|VAR, CONST|TMP|VAR|CV)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2;
	bool result;

	// windows: �޲���
	SAVE_OPLINE();
	// ��ȡzvalָ��, UNUSED ����null����֧�� TMPVAR/TMPVARCV
	op1 = GET_OP1_ZVAL_PTR_DEREF(BP_VAR_R);
	// ��ȡzvalָ��, UNUSED ����null����֧�� TMPVAR/TMPVARCV
	op2 = GET_OP2_ZVAL_PTR_DEREF(BP_VAR_R);
	// ����Ƿ���ͬ
	result = fast_is_identical_function(op1, op2);
	// �ͷŲ�������ĸ��ӱ���
	FREE_OP2();
	// ���ݵ�ǰ��������ѡ����һ�������룬p1:����ֵ����Ҫ�������жϡ�p2:�Ƿ����쳣
	ZEND_VM_SMART_BRANCH(result, 1);
}

// ing3, �Ƚϲ���ͬ ���㣬Ȼ�������������op1,op2 
ZEND_VM_COLD_CONSTCONST_HANDLER(17, ZEND_IS_NOT_IDENTICAL, CONST|TMP|VAR|CV, CONST|TMP|VAR|CV, SPEC(COMMUTATIVE))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2;
	bool result;

	// windows: �޲���
	SAVE_OPLINE();
	// ��ȡzvalָ��, UNUSED ����null����֧�� TMPVAR/TMPVARCV
	op1 = GET_OP1_ZVAL_PTR_DEREF(BP_VAR_R);
	// ��ȡzvalָ��, UNUSED ����null����֧�� TMPVAR/TMPVARCV
	op2 = GET_OP2_ZVAL_PTR_DEREF(BP_VAR_R);
	// ����Ƿ� ����ͬ
	result = fast_is_not_identical_function(op1, op2);
	// �ͷŲ�������ĸ��ӱ���
	FREE_OP1();
	// �ͷŲ�������ĸ��ӱ���
	FREE_OP2();
	// ���ݵ�ǰ��������ѡ����һ�������룬p1:����ֵ����Ҫ�������жϡ�p2:�Ƿ����쳣
	ZEND_VM_SMART_BRANCH(result, 1);
}

// ing3, �������, ���鴫��ı����Ƿ���Ч������ �Ƚ� ���㣬Ȼ�������������op1,op2 
ZEND_VM_HELPER(zend_is_equal_helper, ANY, ANY, zval *op_1, zval *op_2)
{
	int ret;
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// windows: �޲���
	SAVE_OPLINE();
	// ��� ��������Ϊδ����
	if (UNEXPECTED(Z_TYPE_INFO_P(op_1) == IS_UNDEF)) {
		// ����p1�ı�������������δ����, ����δ��ʼ��zval
		op_1 = ZVAL_UNDEFINED_OP1();
	}
	// ��� ��������Ϊ δ����
	if (UNEXPECTED(Z_TYPE_INFO_P(op_2) == IS_UNDEF)) {
		// ����p2�ı�������������δ����, ����δ��ʼ��zval
		op_2 = ZVAL_UNDEFINED_OP2();
	}
	// ���бȽ�����
	ret = zend_compare(op_1, op_2);
	// ��� op1 ���� �Ǳ�������ʱ����
	if (OP1_TYPE & (IS_TMP_VAR|IS_VAR)) {
		// ����û�����ô����Ķ���,��û������ zval����������gc�������ڡ�
		zval_ptr_dtor_nogc(op_1);
	}
	// ��� op2 ���� �Ǳ�������ʱ����
	if (OP2_TYPE & (IS_TMP_VAR|IS_VAR)) {
		// ����û�����ô����Ķ���,��û������ zval����������gc�������ڡ�
		zval_ptr_dtor_nogc(op_2);
	}
	// ���ݵ�ǰ��������ѡ����һ�������룬p1:����ֵ����Ҫ�������жϡ�p2:�Ƿ����쳣
	// ����ȽϽ�� ����0 ����true ,���� false
	ZEND_VM_SMART_BRANCH(ret == 0, 1);
}

// ing3, �Ƚ���� ������
ZEND_VM_COLD_CONSTCONST_HANDLER(18, ZEND_IS_EQUAL, CONST|TMPVAR|CV, CONST|TMPVAR|CV, SPEC(SMART_BRANCH,COMMUTATIVE))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2;
	double d1, d2;
	// ��ȡzvalָ��, UNUSED ����null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ��ȡzvalָ��, UNUSED ����null
	op2 = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// op1��op2��һ������ANY
	if (ZEND_VM_SPEC && OP1_TYPE == IS_CONST && OP2_TYPE == IS_CONST) {
		/* pass */
	// ���� ��������� ����
	} else if (EXPECTED(Z_TYPE_P(op1) == IS_LONG)) {
		// ��������� ����
		if (EXPECTED(Z_TYPE_P(op2) == IS_LONG)) {
			// ��������������
			if (EXPECTED(Z_LVAL_P(op1) == Z_LVAL_P(op2))) {
// ������ת��ǩ is_equal_true
ZEND_VM_C_LABEL(is_equal_true):
				// ���ݵ�ǰ��������ѡ����һ�������롣���������ת���ͣ������ֵΪtrue��
				ZEND_VM_SMART_BRANCH_TRUE();
			// ����
			} else {
// ������ת��ǩ is_equal_false
ZEND_VM_C_LABEL(is_equal_false):
				// ���ݵ�ǰ��������ѡ����һ�������롣���������ת���ͣ������ֵΪfalse��
				ZEND_VM_SMART_BRANCH_FALSE();
			}
		// ��������� С��
		} else if (EXPECTED(Z_TYPE_P(op2) == IS_DOUBLE)) {
			// op1ת��С��
			d1 = (double)Z_LVAL_P(op1);
			// ȡ��С��
			d2 = Z_DVAL_P(op2);
			// ��ת��ָ����ǩ is_equal_double
			ZEND_VM_C_GOTO(is_equal_double);
		}
	// ���� ��������� С��
	} else if (EXPECTED(Z_TYPE_P(op1) == IS_DOUBLE)) {
		// ��������� С��
		if (EXPECTED(Z_TYPE_P(op2) == IS_DOUBLE)) {
			// ȡ��С��
			d1 = Z_DVAL_P(op1);
			// ȡ��С��
			d2 = Z_DVAL_P(op2);
// ������ת��ǩ is_equal_double
ZEND_VM_C_LABEL(is_equal_double):
			// �������С�����
			if (d1 == d2) {
				// ��ת��ָ����ǩ is_equal_true
				ZEND_VM_C_GOTO(is_equal_true);
			// ����
			} else {
				// ��ת��ָ����ǩ is_equal_false
				ZEND_VM_C_GOTO(is_equal_false);
			}
		// ���� ��������� ����
		} else if (EXPECTED(Z_TYPE_P(op2) == IS_LONG)) {
			// ȡ��С��
			d1 = Z_DVAL_P(op1);
			// op2ת��С��
			d2 = (double)Z_LVAL_P(op2);
			// ��ת��ָ����ǩ is_equal_double
			ZEND_VM_C_GOTO(is_equal_double);
		}
	// ���� ��������� �ִ�
	} else if (EXPECTED(Z_TYPE_P(op1) == IS_STRING)) {
		// ��������� �ִ�
		if (EXPECTED(Z_TYPE_P(op2) == IS_STRING)) {
			// �Ƚ������ִ����
			bool result = zend_fast_equal_strings(Z_STR_P(op1), Z_STR_P(op2));
			// ��� op1 ���� �Ǳ�������ʱ����
			if (OP1_TYPE & (IS_TMP_VAR|IS_VAR)) {
				// ͨ��ָ������ zval���ִ���
				zval_ptr_dtor_str(op1);
			}
			// ��� op2 ���� �Ǳ�������ʱ����
			if (OP2_TYPE & (IS_TMP_VAR|IS_VAR)) {
				// ͨ��ָ������ zval���ִ���
				zval_ptr_dtor_str(op2);
			}
			if (result) {
				// ��ת��ָ����ǩ is_equal_true
				ZEND_VM_C_GOTO(is_equal_true);
			// ����
			} else {
				// ��ת��ָ����ǩ is_equal_false
				ZEND_VM_C_GOTO(is_equal_false);
			}
		}
	}
	// �������, ���鴫��ı����Ƿ���Ч������ �Ƚ� ���㣬Ȼ�������������op1,op2 
	// ���÷��� zend_is_equal_helper
	ZEND_VM_DISPATCH_TO_HELPER(zend_is_equal_helper, op_1, op1, op_2, op2);
}

// ing3, ���㲻���, ���鴫��ı����Ƿ���Ч������ �Ƚ� ���㣬Ȼ�������������op1,op2 
ZEND_VM_HELPER(zend_is_not_equal_helper, ANY, ANY, zval *op_1, zval *op_2)
{
	int ret;
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// windows: �޲���
	SAVE_OPLINE();
	// ��� ��������Ϊδ����
	if (UNEXPECTED(Z_TYPE_INFO_P(op_1) == IS_UNDEF)) {
		// ����p1�ı�������������δ����, ����δ��ʼ��zval
		op_1 = ZVAL_UNDEFINED_OP1();
	}
	// ��� ��������Ϊ δ����
	if (UNEXPECTED(Z_TYPE_INFO_P(op_2) == IS_UNDEF)) {
		// ����p2�ı�������������δ����, ����δ��ʼ��zval
		op_2 = ZVAL_UNDEFINED_OP2();
	}
	// ���бȽ�����
	ret = zend_compare(op_1, op_2);
	// ��� op1 ���� �Ǳ�������ʱ����
	if (OP1_TYPE & (IS_TMP_VAR|IS_VAR)) {
		// ����û�����ô����Ķ���,��û������ zval����������gc�������ڡ�
		zval_ptr_dtor_nogc(op_1);
	}
	// ��� op2 ���� �Ǳ�������ʱ����
	if (OP2_TYPE & (IS_TMP_VAR|IS_VAR)) {
		// ����û�����ô����Ķ���,��û������ zval����������gc�������ڡ�
		zval_ptr_dtor_nogc(op_2);
	}
	// ���ݵ�ǰ��������ѡ����һ�������룬p1:����ֵ����Ҫ�������жϡ�p2:�Ƿ����쳣
	// ����ȽϽ�� ����� ����true ,���� false
	ZEND_VM_SMART_BRANCH(ret != 0, 1);
}

// ing3, �Ƚϲ���ȣ�������
ZEND_VM_COLD_CONSTCONST_HANDLER(19, ZEND_IS_NOT_EQUAL, CONST|TMPVAR|CV, CONST|TMPVAR|CV, SPEC(SMART_BRANCH,COMMUTATIVE))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2;
	double d1, d2;
	// ��ȡzvalָ��, UNUSED ����null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ��ȡzvalָ��, UNUSED ����null
	op2 = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// op1��op2��һ������ANY
	if (ZEND_VM_SPEC && OP1_TYPE == IS_CONST && OP2_TYPE == IS_CONST) {
		/* pass */
	// ��� op1 ������ ����
	} else if (EXPECTED(Z_TYPE_P(op1) == IS_LONG)) {
		// ��� op2 ������ ����
		if (EXPECTED(Z_TYPE_P(op2) == IS_LONG)) {
			// ����������������
			if (EXPECTED(Z_LVAL_P(op1) != Z_LVAL_P(op2))) {
// ������ת��ǩ is_not_equal_true
ZEND_VM_C_LABEL(is_not_equal_true):
				// ���ݵ�ǰ��������ѡ����һ�������롣���������ת���ͣ������ֵΪtrue��
				ZEND_VM_SMART_BRANCH_TRUE();
			// ����
			} else {
// ������ת��ǩ is_not_equal_false
ZEND_VM_C_LABEL(is_not_equal_false):
				// ���ݵ�ǰ��������ѡ����һ�������롣���������ת���ͣ������ֵΪfalse��
				ZEND_VM_SMART_BRANCH_FALSE();
			}
		// ���� ��� op2 ������ С��
		} else if (EXPECTED(Z_TYPE_P(op2) == IS_DOUBLE)) {
			// op1ת��С��
			d1 = (double)Z_LVAL_P(op1);
			// ȡ��С��
			d2 = Z_DVAL_P(op2);
			// ��ת��ָ����ǩ is_not_equal_double
			ZEND_VM_C_GOTO(is_not_equal_double);
		}
	// ��� op1 ������ С��
	} else if (EXPECTED(Z_TYPE_P(op1) == IS_DOUBLE)) {
		// ��� op2 ������ С��
		if (EXPECTED(Z_TYPE_P(op2) == IS_DOUBLE)) {
			// ȡ��С��
			d1 = Z_DVAL_P(op1);
			// ȡ��С��
			d2 = Z_DVAL_P(op2);
// ������ת��ǩ is_not_equal_double
ZEND_VM_C_LABEL(is_not_equal_double):
			if (d1 != d2) {
				// ��ת��ָ����ǩ is_not_equal_true
				ZEND_VM_C_GOTO(is_not_equal_true);
			// ����
			} else {
				// ��ת��ָ����ǩ is_not_equal_false
				ZEND_VM_C_GOTO(is_not_equal_false);
			}
		// ���� ��� op2 ������ ����
		} else if (EXPECTED(Z_TYPE_P(op2) == IS_LONG)) {
			// ȡ��С��
			d1 = Z_DVAL_P(op1);
			// op2ת��С��
			d2 = (double)Z_LVAL_P(op2);
			// ��ת��ָ����ǩ is_not_equal_double
			ZEND_VM_C_GOTO(is_not_equal_double);
		}
	// ��� op1 ������ 
	} else if (EXPECTED(Z_TYPE_P(op1) == IS_STRING)) {
		// ��� op2 ������ �ִ�
		if (EXPECTED(Z_TYPE_P(op2) == IS_STRING)) {
			// �Ƚ������ִ��Ƿ����
			bool result = zend_fast_equal_strings(Z_STR_P(op1), Z_STR_P(op2));
			// ��� op1 ���� �Ǳ�������ʱ����
			if (OP1_TYPE & (IS_TMP_VAR|IS_VAR)) {
				// ͨ��ָ������ zval���ִ���
				zval_ptr_dtor_str(op1);
			}
			// ��� op2 ���� �Ǳ�������ʱ����
			if (OP2_TYPE & (IS_TMP_VAR|IS_VAR)) {
				// ͨ��ָ������ zval���ִ���
				zval_ptr_dtor_str(op2);
			}
			// ��������ִ����
			if (!result) {
				// ��ת��ָ����ǩ is_not_equal_true
				ZEND_VM_C_GOTO(is_not_equal_true);
			// ����
			} else {
				// ��ת��ָ����ǩ is_not_equal_false
				ZEND_VM_C_GOTO(is_not_equal_false);
			}
		}
	}
	// ���㲻���, ���鴫��ı����Ƿ���Ч������ �Ƚ� ���㣬Ȼ�������������op1,op2 
	// ���÷��� zend_is_not_equal_helper
	ZEND_VM_DISPATCH_TO_HELPER(zend_is_not_equal_helper, op_1, op1, op_2, op2);
}

// ing3, ����С��, ���鴫��ı����Ƿ���Ч������ �Ƚ� ���㣬Ȼ�������������op1,op2 
ZEND_VM_HELPER(zend_is_smaller_helper, ANY, ANY, zval *op_1, zval *op_2)
{
	int ret;
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// windows: �޲���
	SAVE_OPLINE();
	// ��� ��������Ϊδ����
	if (UNEXPECTED(Z_TYPE_INFO_P(op_1) == IS_UNDEF)) {
		// ����p1�ı�������������δ����, ����δ��ʼ��zval
		op_1 = ZVAL_UNDEFINED_OP1();
	}
	// ��� ��������Ϊ δ����
	if (UNEXPECTED(Z_TYPE_INFO_P(op_2) == IS_UNDEF)) {
		// ����p2�ı�������������δ����, ����δ��ʼ��zval
		op_2 = ZVAL_UNDEFINED_OP2();
	}
	// ���бȽ�����
	ret = zend_compare(op_1, op_2);
	// ��� op1 ���� �Ǳ�������ʱ����
	if (OP1_TYPE & (IS_TMP_VAR|IS_VAR)) {
		// ����û�����ô����Ķ���,��û������ zval����������gc�������ڡ�
		zval_ptr_dtor_nogc(op_1);
	}
	// ��� op2 ���� �Ǳ�������ʱ����
	if (OP2_TYPE & (IS_TMP_VAR|IS_VAR)) {
		// ����û�����ô����Ķ���,��û������ zval����������gc�������ڡ�
		zval_ptr_dtor_nogc(op_2);
	}
	// ���ݵ�ǰ��������ѡ����һ�������룬p1:����ֵ����Ҫ�������жϡ�p2:�Ƿ����쳣
	// ����ȽϽ�� С��0 ����true ,���� false
	ZEND_VM_SMART_BRANCH(ret < 0, 1);
}

// ing3, С�ڣ�������, 1/3
ZEND_VM_HOT_NOCONSTCONST_HANDLER(20, ZEND_IS_SMALLER, CONST|TMPVARCV, CONST|TMPVARCV, SPEC(SMART_BRANCH))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2;
	double d1, d2;
	// ��ȡzvalָ��, UNUSED ����null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ��ȡzvalָ��, UNUSED ����null
	op2 = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// op1��op2��һ������ANY �����������ǳ���
	if (ZEND_VM_SPEC && OP1_TYPE == IS_CONST && OP2_TYPE == IS_CONST) {
		/* pass */
	// ���op1������
	} else if (EXPECTED(Z_TYPE_INFO_P(op1) == IS_LONG)) {
		// ���op2������
		if (EXPECTED(Z_TYPE_INFO_P(op2) == IS_LONG)) {
			// ����ֱ�ӱȽϴ�С
			if (EXPECTED(Z_LVAL_P(op1) < Z_LVAL_P(op2))) {
// ������ת��ǩ is_smaller_true
ZEND_VM_C_LABEL(is_smaller_true):
				// ���ݵ�ǰ��������ѡ����һ�������롣���������ת���ͣ������ֵΪtrue��
				ZEND_VM_SMART_BRANCH_TRUE();
			// ����
			} else {
// ������ת��ǩ is_smaller_false
ZEND_VM_C_LABEL(is_smaller_false):
				// ���ݵ�ǰ��������ѡ����һ�������롣���������ת���ͣ������ֵΪfalse��
				ZEND_VM_SMART_BRANCH_FALSE();
			}
		// ���op2��С��
		} else if (EXPECTED(Z_TYPE_INFO_P(op2) == IS_DOUBLE)) {
			// op1ת��С��
			d1 = (double)Z_LVAL_P(op1);
			// ȡ��С��
			d2 = Z_DVAL_P(op2);
			// ��ת��ָ����ǩ is_smaller_double
			ZEND_VM_C_GOTO(is_smaller_double);
		}
	// ��� op1��С��
	} else if (EXPECTED(Z_TYPE_INFO_P(op1) == IS_DOUBLE)) {
		// ��� op2��С��
		if (EXPECTED(Z_TYPE_INFO_P(op2) == IS_DOUBLE)) {
			// ȡ��С��
			d1 = Z_DVAL_P(op1);
			// ȡ��С��
			d2 = Z_DVAL_P(op2);
// ������ת��ǩ is_smaller_double
ZEND_VM_C_LABEL(is_smaller_double):
			// ���op2��
			if (d1 < d2) {
				// ��ת��ָ����ǩ is_smaller_true
				ZEND_VM_C_GOTO(is_smaller_true);
			// ����
			} else {
				// ��ת��ָ����ǩ is_smaller_false
				ZEND_VM_C_GOTO(is_smaller_false);
			}
		// ���op2������ 
		} else if (EXPECTED(Z_TYPE_INFO_P(op2) == IS_LONG)) {
			// op1ȡ��С��
			d1 = Z_DVAL_P(op1);
			// op2ת��С��
			d2 = (double)Z_LVAL_P(op2);
			// ��ת��ָ����ǩ is_smaller_double
			ZEND_VM_C_GOTO(is_smaller_double);
		}
	}
	// ���÷��� zend_is_smaller_helper
	ZEND_VM_DISPATCH_TO_HELPER(zend_is_smaller_helper, op_1, op1, op_2, op2);
}

// ing3, ����С�ڵ���, ���鴫��ı����Ƿ���Ч������ �Ƚ� ���㣬Ȼ�������������op1,op2 
ZEND_VM_HELPER(zend_is_smaller_or_equal_helper, ANY, ANY, zval *op_1, zval *op_2)
{
	int ret;
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// windows: �޲���
	SAVE_OPLINE();
	// ��� ��������Ϊδ����
	if (UNEXPECTED(Z_TYPE_INFO_P(op_1) == IS_UNDEF)) {
		// ����p1�ı�������������δ����, ����δ��ʼ��zval
		op_1 = ZVAL_UNDEFINED_OP1();
	}
	// ��� ��������Ϊδ����
	if (UNEXPECTED(Z_TYPE_INFO_P(op_2) == IS_UNDEF)) {
		// ����p2�ı�������������δ����, ����δ��ʼ��zval
		op_2 = ZVAL_UNDEFINED_OP2();
	}
	// ���бȽ�����
	ret = zend_compare(op_1, op_2);
	// ��� op1 ���� �Ǳ�������ʱ����
	if (OP1_TYPE & (IS_TMP_VAR|IS_VAR)) {
		// ����û�����ô����Ķ���,��û������ zval����������gc�������ڡ�
		zval_ptr_dtor_nogc(op_1);
	}
	// ��� op2 ���� �Ǳ�������ʱ����
	if (OP2_TYPE & (IS_TMP_VAR|IS_VAR)) {
		// ����û�����ô����Ķ���,��û������ zval����������gc�������ڡ�
		zval_ptr_dtor_nogc(op_2);
	}
	// ���ݵ�ǰ��������ѡ����һ�������룬p1:����ֵ����Ҫ�������жϡ�p2:�Ƿ����쳣
	// ����ȽϽ�� С�ڵ���0 ����true ,���� false
	ZEND_VM_SMART_BRANCH(ret <= 0, 1);
}

// ing3, С�ڵ��ڣ������� , 1/3
ZEND_VM_HOT_NOCONSTCONST_HANDLER(21, ZEND_IS_SMALLER_OR_EQUAL, CONST|TMPVARCV, CONST|TMPVARCV, SPEC(SMART_BRANCH))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2;
	double d1, d2;
	// ��ȡzvalָ��, UNUSED ����null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ��ȡzvalָ��, UNUSED ����null
	op2 = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// op1��op2��һ������ANY
	if (ZEND_VM_SPEC && OP1_TYPE == IS_CONST && OP2_TYPE == IS_CONST) {
		/* pass */
	// ��� op1 ��������Ϊ ����
	} else if (EXPECTED(Z_TYPE_INFO_P(op1) == IS_LONG)) {
		// ��� op2 ��������Ϊ ����
		if (EXPECTED(Z_TYPE_INFO_P(op2) == IS_LONG)) {
			// ��������ֱ�ӱȽ�
			if (EXPECTED(Z_LVAL_P(op1) <= Z_LVAL_P(op2))) {
// ������ת��ǩ is_smaller_or_equal_true
ZEND_VM_C_LABEL(is_smaller_or_equal_true):
				// ���ݵ�ǰ��������ѡ����һ�������롣���������ת���ͣ������ֵΪtrue��
				ZEND_VM_SMART_BRANCH_TRUE();
				// ���Ϊtrue
				ZVAL_TRUE(EX_VAR(opline->result.var));
				// opline + 1����Ŀ������벢 return
				ZEND_VM_NEXT_OPCODE();
			// ����
			} else {
// ������ת��ǩ is_smaller_or_equal_false
ZEND_VM_C_LABEL(is_smaller_or_equal_false):
				// ���ݵ�ǰ��������ѡ����һ�������롣���������ת���ͣ������ֵΪfalse��
				ZEND_VM_SMART_BRANCH_FALSE();
				// ���Ϊfalse
				ZVAL_FALSE(EX_VAR(opline->result.var));
				// opline + 1����Ŀ������벢 return
				ZEND_VM_NEXT_OPCODE();
			}
		// ���� ��� ��������Ϊ С��
		} else if (EXPECTED(Z_TYPE_INFO_P(op2) == IS_DOUBLE)) {
			d1 = (double)Z_LVAL_P(op1);
			// ȡ��С��
			d2 = Z_DVAL_P(op2);
			// ��ת��ָ����ǩ is_smaller_or_equal_double
			ZEND_VM_C_GOTO(is_smaller_or_equal_double);
		}
	// ���� ��� ��������Ϊ С��
	} else if (EXPECTED(Z_TYPE_INFO_P(op1) == IS_DOUBLE)) {
		// ��� ��������Ϊ С��
		if (EXPECTED(Z_TYPE_INFO_P(op2) == IS_DOUBLE)) {
			// ȡ��С��
			d1 = Z_DVAL_P(op1);
			// ȡ��С��
			d2 = Z_DVAL_P(op2);
// ������ת��ǩ is_smaller_or_equal_double
ZEND_VM_C_LABEL(is_smaller_or_equal_double):
			// С��ֱ�ӱȴ�С��d1С
			if (d1 <= d2) {
				// ��ת��ָ����ǩ is_smaller_or_equal_true
				ZEND_VM_C_GOTO(is_smaller_or_equal_true);
			// ����
			} else {
				// ��ת��ָ����ǩ is_smaller_or_equal_false
				ZEND_VM_C_GOTO(is_smaller_or_equal_false);
			}
		// ��� ��������Ϊ ����
		} else if (EXPECTED(Z_TYPE_INFO_P(op2) == IS_LONG)) {
			// ȡ��С��
			d1 = Z_DVAL_P(op1);
			// ����ת��С��
			d2 = (double)Z_LVAL_P(op2);
			// ��ת��ָ����ǩ is_smaller_or_equal_double
			ZEND_VM_C_GOTO(is_smaller_or_equal_double);
		}
	}
	// ���÷��� zend_is_smaller_or_equal_helper
	ZEND_VM_DISPATCH_TO_HELPER(zend_is_smaller_or_equal_helper, op_1, op1, op_2, op2);
}

// ing3, �µıȴ�С����� <=> �����ڷ���1�����ڷ���0��С�ڷ���-1
ZEND_VM_COLD_CONSTCONST_HANDLER(170, ZEND_SPACESHIP, CONST|TMPVAR|CV, CONST|TMPVAR|CV)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2;

	// windows: �޲���
	SAVE_OPLINE();
	// ��ȡzvalָ�룬UNUSED ���ͷ���null
	op1 = GET_OP1_ZVAL_PTR(BP_VAR_R);
	// ��ȡzvalָ�룬UNUSED ���ͷ���null
	op2 = GET_OP2_ZVAL_PTR(BP_VAR_R);
	// �Ƚ�����������p1:���ؽ��, p2:����1, p3:����2
	compare_function(EX_VAR(opline->result.var), op1, op2);
	// �ͷŲ�������ĸ��ӱ���
	FREE_OP1();
	// �ͷŲ�������ĸ��ӱ���
	FREE_OP2();
	// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, ������ �� ����, ���鴫��ı����Ƿ���Ч������ �� ���㣬Ȼ�������������op1,op2 
ZEND_VM_HELPER(zend_bw_or_helper, ANY, ANY, zval *op_1, zval *op_2)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// windows: �޲���
	SAVE_OPLINE();
	// ��� ��������Ϊ δ����
	if (UNEXPECTED(Z_TYPE_INFO_P(op_1) == IS_UNDEF)) {
		// ����p1�ı�������������δ����, ����δ��ʼ��zval
		op_1 = ZVAL_UNDEFINED_OP1();
	}
	// ��� ��������Ϊ δ����
	if (UNEXPECTED(Z_TYPE_INFO_P(op_2) == IS_UNDEF)) {
		// ����p2�ı�������������δ����, ����δ��ʼ��zval
		op_2 = ZVAL_UNDEFINED_OP2();
	}
	// �������ִ���1�������ַ����� ��λ �� ����
	bitwise_or_function(EX_VAR(opline->result.var), op_1, op_2);
	// ��� op1 ���� �Ǳ�������ʱ����
	if (OP1_TYPE & (IS_TMP_VAR|IS_VAR)) {
		// ����û�����ô����Ķ���,��û������ zval����������gc�������ڡ�
		zval_ptr_dtor_nogc(op_1);
	}
	// ��� op2 ���� �Ǳ�������ʱ����
	if (OP2_TYPE & (IS_TMP_VAR|IS_VAR)) {
		// ����û�����ô����Ķ���,��û������ zval����������gc�������ڡ�
		zval_ptr_dtor_nogc(op_2);
	}
	// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// 1/1 ,ZEND_BW_OR
// ing3, ������ �� ����
ZEND_VM_HOT_NOCONSTCONST_HANDLER(9, ZEND_BW_OR, CONST|TMPVARCV, CONST|TMPVARCV, SPEC(COMMUTATIVE))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2;
	// ��ȡzvalָ��, UNUSED ����null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ��ȡzvalָ��, UNUSED ����null
	op2 = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// op1��op2��һ������ANY
	if (ZEND_VM_SPEC && OP1_TYPE == IS_CONST && OP2_TYPE == IS_CONST) {
		/* pass */
		
	// ���� ��� ���� �������� ��Ϊ ����
	} else if (EXPECTED(Z_TYPE_INFO_P(op1) == IS_LONG)
			&& EXPECTED(Z_TYPE_INFO_P(op2) == IS_LONG)) {
		// ֱ���� ������ 
		ZVAL_LONG(EX_VAR(opline->result.var), Z_LVAL_P(op1) | Z_LVAL_P(op2));
		// opline + 1����Ŀ������벢 return
		ZEND_VM_NEXT_OPCODE();
	}

	// ������ �� ����, ���鴫��ı����Ƿ���Ч������ �� ���㣬Ȼ�������������op1,op2 
	// ���÷��� zend_bw_or_helper
	ZEND_VM_DISPATCH_TO_HELPER(zend_bw_or_helper, op_1, op1, op_2, op2);
}

// ing3, ������ �� ����, ���鴫��ı����Ƿ���Ч������ �� ���㣬Ȼ�������������op1,op2 
ZEND_VM_HELPER(zend_bw_and_helper, ANY, ANY, zval *op_1, zval *op_2)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// windows: �޲���
	SAVE_OPLINE();
	// ��� ��������Ϊ δ����
	if (UNEXPECTED(Z_TYPE_INFO_P(op_1) == IS_UNDEF)) {
		// ����p1�ı�������������δ����, ����δ��ʼ��zval
		op_1 = ZVAL_UNDEFINED_OP1();
	}
	// ��� ��������Ϊ δ����
	if (UNEXPECTED(Z_TYPE_INFO_P(op_2) == IS_UNDEF)) {
		// ����p2�ı�������������δ����, ����δ��ʼ��zval
		op_2 = ZVAL_UNDEFINED_OP2();
	}
	// �������ִ���1�������ַ����� ��λ�� ����
	bitwise_and_function(EX_VAR(opline->result.var), op_1, op_2);
	// ��� op1 ���� �Ǳ�������ʱ����
	if (OP1_TYPE & (IS_TMP_VAR|IS_VAR)) {
		// ����û�����ô����Ķ���,��û������ zval����������gc�������ڡ�
		zval_ptr_dtor_nogc(op_1);
	}
	// ��� op2 ���� �Ǳ�������ʱ����
	if (OP2_TYPE & (IS_TMP_VAR|IS_VAR)) {
		// ����û�����ô����Ķ���,��û������ zval����������gc�������ڡ�
		zval_ptr_dtor_nogc(op_2);
	}
	// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}
// 1/1, ZEND_BW_AND
// ing3, ������ �� ����
ZEND_VM_HOT_NOCONSTCONST_HANDLER(10, ZEND_BW_AND, CONST|TMPVARCV, CONST|TMPVARCV, SPEC(COMMUTATIVE))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2;
	// ��ȡzvalָ��, UNUSED ����null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ��ȡzvalָ��, UNUSED ����null
	op2 = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// op1��op2��һ������ANY
	if (ZEND_VM_SPEC && OP1_TYPE == IS_CONST && OP2_TYPE == IS_CONST) {
		/* pass */
	// ���� ��� �����������Ͷ�Ϊ ����
	} else if (EXPECTED(Z_TYPE_INFO_P(op1) == IS_LONG)
			&& EXPECTED(Z_TYPE_INFO_P(op2) == IS_LONG)) {
		// �������� ֱ���� �� ����
		ZVAL_LONG(EX_VAR(opline->result.var), Z_LVAL_P(op1) & Z_LVAL_P(op2));
		// opline + 1����Ŀ������벢 return
		ZEND_VM_NEXT_OPCODE();
	}

	// ������ �� ����, ���鴫��ı����Ƿ���Ч������ �� ���㣬Ȼ�������������op1,op2 
	// ���÷��� zend_bw_and_helper
	ZEND_VM_DISPATCH_TO_HELPER(zend_bw_and_helper, op_1, op1, op_2, op2);
}

// ing3, ������ �ֻ� ����, ���鴫��ı����Ƿ���Ч������ �ֻ� ���㣬Ȼ�������������op1,op2 
ZEND_VM_HELPER(zend_bw_xor_helper, ANY, ANY, zval *op_1, zval *op_2)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// windows: �޲���
	SAVE_OPLINE();
	// ��� ��������Ϊδ����
	if (UNEXPECTED(Z_TYPE_INFO_P(op_1) == IS_UNDEF)) {
		// ����p1�ı�������������δ����, ����δ��ʼ��zval
		op_1 = ZVAL_UNDEFINED_OP1();
	}
	// ��� ��������Ϊ δ����
	if (UNEXPECTED(Z_TYPE_INFO_P(op_2) == IS_UNDEF)) {
		// ����p2�ı�������������δ����, ����δ��ʼ��zval
		op_2 = ZVAL_UNDEFINED_OP2();
	}
	// �������ִ���1�������ַ����� ��λ �ֻ� ����
	bitwise_xor_function(EX_VAR(opline->result.var), op_1, op_2);
	// ��� op1 ���� �Ǳ�������ʱ����
	if (OP1_TYPE & (IS_TMP_VAR|IS_VAR)) {
		// ����û�����ô����Ķ���,��û������ zval����������gc�������ڡ�
		zval_ptr_dtor_nogc(op_1);
	}
	// ��� op2 ���� �Ǳ�������ʱ����
	if (OP2_TYPE & (IS_TMP_VAR|IS_VAR)) {
		// ����û�����ô����Ķ���,��û������ zval����������gc�������ڡ�
		zval_ptr_dtor_nogc(op_2);
	}
	// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// 1/1, ZEND_BW_XOR
// ing3, ������ �ֻ� ����
ZEND_VM_HOT_NOCONSTCONST_HANDLER(11, ZEND_BW_XOR, CONST|TMPVARCV, CONST|TMPVARCV, SPEC(COMMUTATIVE))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2;
	// ��ȡzvalָ��, UNUSED ����null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ��ȡzvalָ��, UNUSED ����null
	op2 = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// op1��op2��һ������ANY
	if (ZEND_VM_SPEC && OP1_TYPE == IS_CONST && OP2_TYPE == IS_CONST) {
		/* pass */
	// ���� ��� �����������Ͷ�Ϊ ����
	} else if (EXPECTED(Z_TYPE_INFO_P(op1) == IS_LONG)
			&& EXPECTED(Z_TYPE_INFO_P(op2) == IS_LONG)) {
		// ��������ֱ�ӽ����ֻ�����
		ZVAL_LONG(EX_VAR(opline->result.var), Z_LVAL_P(op1) ^ Z_LVAL_P(op2));
		// opline + 1����Ŀ������벢 return
		ZEND_VM_NEXT_OPCODE();
	}

	// ������ �ֻ� ����, ���鴫��ı����Ƿ���Ч������ �ֻ� ���㣬Ȼ�������������op1,op2 
	// ���÷��� zend_bw_xor_helper
	ZEND_VM_DISPATCH_TO_HELPER(zend_bw_xor_helper, op_1, op1, op_2, op2);
}

// ing3, �����ֻ�����ֵ��ͬʱ���� true��
ZEND_VM_COLD_CONSTCONST_HANDLER(15, ZEND_BOOL_XOR, CONST|TMPVAR|CV, CONST|TMPVAR|CV, SPEC(COMMUTATIVE))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2;

	// windows: �޲���
	SAVE_OPLINE();
	// ��ȡzvalָ�룬UNUSED ���ͷ���null
	op1 = GET_OP1_ZVAL_PTR(BP_VAR_R);
	// ��ȡzvalָ�룬UNUSED ���ͷ���null
	op2 = GET_OP2_ZVAL_PTR(BP_VAR_R);
	// �����ֻ�����ֵ��ͬʱ���� true��
	boolean_xor_function(EX_VAR(opline->result.var), op1, op2);
	// �ͷŲ�������ĸ��ӱ���
	FREE_OP1();
	// �ͷŲ�������ĸ��ӱ���
	FREE_OP2();
	// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, ������ ȡ�� ����, ���鴫��ı����Ƿ���Ч������ ȡ�� ���㣬Ȼ�������������op1
ZEND_VM_HELPER(zend_bw_not_helper, ANY, ANY, zval *op_1)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// windows: �޲���
	SAVE_OPLINE();
	// ��� op1 ������ δ����
	if (UNEXPECTED(Z_TYPE_P(op_1) == IS_UNDEF)) {
		// ����p1�ı�������������δ����, ����δ��ʼ��zval
		op_1 = ZVAL_UNDEFINED_OP1();
	}
	// ~ ��������һԪ��������������ȡ��
	bitwise_not_function(EX_VAR(opline->result.var), op_1);
	// �ͷŲ�������ĸ��ӱ���
	FREE_OP1();
	// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, ������ ȡ�� ����
ZEND_VM_HOT_NOCONST_HANDLER(13, ZEND_BW_NOT, CONST|TMPVARCV, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1;
	// ��ȡzvalָ��, UNUSED ����null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ��� ��������Ϊ ����
	if (EXPECTED(Z_TYPE_INFO_P(op1) == IS_LONG)) {
		// ����ֱ�� �á���������
		ZVAL_LONG(EX_VAR(opline->result.var), ~Z_LVAL_P(op1));
		// opline + 1����Ŀ������벢 return
		ZEND_VM_NEXT_OPCODE();
	}

	// ������ ȡ�� ����, ���鴫��ı����Ƿ���Ч������ ȡ�� ���㣬Ȼ�������������op1
	// ���÷��� zend_bw_xor_helper
	ZEND_VM_DISPATCH_TO_HELPER(zend_bw_not_helper, op_1, op1);
}

// ing3, boolȡ��
ZEND_VM_COLD_CONST_HANDLER(14, ZEND_BOOL_NOT, CONST|TMPVAR|CV, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	// 
	zval *val;
	// ��ȡzvalָ��, UNUSED ����null
	val = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ��� ����Ϊ true
	if (Z_TYPE_INFO_P(val) == IS_TRUE) {
		// ���Ϊ false
		ZVAL_FALSE(EX_VAR(opline->result.var));
	// ���� ��� ��������Ϊ false,null,δ����
	} else if (EXPECTED(Z_TYPE_INFO_P(val) <= IS_TRUE)) {
		/* The result and op1 can be the same cv zval */
		// ��ȡ��������
		const uint32_t orig_val_type = Z_TYPE_INFO_P(val);
		// ���Ϊ true
		ZVAL_TRUE(EX_VAR(opline->result.var));
		// ��� op1 �� ������� ���� ԭֵ������ δ����
		if (OP1_TYPE == IS_CV && UNEXPECTED(orig_val_type == IS_UNDEF)) {
			// windows: �޲���
			SAVE_OPLINE();
			// ����p1�ı�������������δ����, ����δ��ʼ��zval
			ZVAL_UNDEFINED_OP1();
			// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
			ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
		}
	// ����
	} else {
		// windows: �޲���
		SAVE_OPLINE();
		// ��ֵת��bool�ͺ� ȡ�����ŵ����������
		ZVAL_BOOL(EX_VAR(opline->result.var), !i_zend_is_true(val));
		// �ͷŲ�������ĸ��ӱ���
		FREE_OP1();
		// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
		ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
	}
	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, �����������ڷǶ�����������ʹ�� $this
ZEND_VM_COLD_HELPER(zend_this_not_in_object_context_helper, ANY, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// windows: �޲���
	SAVE_OPLINE();
	zend_throw_error(NULL, "Using $this when not in object context");
	// ��� opline ��������� ��������ʱ�������ѽ�� zval ���Ϊδ����
	UNDEF_RESULT();
	// ��Ҫ�� return
	HANDLE_EXCEPTION();
}

// ing3, ����û����δ����ĺ��� 
ZEND_VM_COLD_HELPER(zend_undefined_function_helper, ANY, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *function_name;

	// windows: �޲���
	SAVE_OPLINE();
	// ���� (p2).zv��p1:opline,p2:node
	function_name = RT_CONSTANT(opline, opline->op2);
	zend_throw_error(NULL, "Call to undefined function %s()", Z_STRVAL_P(function_name));
	// ��Ҫ�� return
	HANDLE_EXCEPTION();
}
// ing3, �������Ը�ֵ���� 1/1��3��THIS���㣩*3=9����֧
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

	// windows: �޲���
	SAVE_OPLINE();
	// ��ȡzvalָ��, TMP/CONST����null, UNUSED ���� $this����֧�� TMPVAR/TMPVARCV��CV ֱ�ӷ��ء�
	object = GET_OP1_OBJ_ZVAL_PTR_PTR_UNDEF(BP_VAR_RW);
	// ��ȡzvalָ�룬UNUSED ���ͷ���null
	property = GET_OP2_ZVAL_PTR(BP_VAR_R);

	// Ϊ��break
	do {
		// ȡ����һ��������� zval��UNUSED����null, ��֧�� TMPVARCV��
		value = GET_OP_DATA_ZVAL_PTR(BP_VAR_R);

		// ��� op1��δʹ�� ���� op1 ���� ���� ����
		if (OP1_TYPE != IS_UNUSED && UNEXPECTED(Z_TYPE_P(object) != IS_OBJECT)) {
			// ��� op1���������� ���� Ŀ���Ƕ���
			if (Z_ISREF_P(object) && Z_TYPE_P(Z_REFVAL_P(object)) == IS_OBJECT) {
				// ������
				object = Z_REFVAL_P(object);
				// ��ת��ָ����ǩ assign_op_object
				ZEND_VM_C_GOTO(assign_op_object);
			}
			// ��� op1 �� ������� ���� ��������Ϊ δ����
			if (OP1_TYPE == IS_CV
			 && UNEXPECTED(Z_TYPE_P(object) == IS_UNDEF)) {
				// ����p1�ı�������������δ����, ����δ��ʼ��zval
				ZVAL_UNDEFINED_OP1();
			}
			// �׳���������Ч���쳣
			zend_throw_non_object_error(object, property OPLINE_CC EXECUTE_DATA_CC);
			// ����
			break;
		}

// ������ת��ǩ assign_op_object
ZEND_VM_C_LABEL(assign_op_object):
		// �����Ѿ�ȷ��������Ƕ�����
		/* here we are sure we are dealing with an object */
		// ȡ������
		zobj = Z_OBJ_P(object);
		// ���op2�ǳ���
		if (OP2_TYPE == IS_CONST) {
			// ȡ��������
			name = Z_STR_P(property);
		// ����
		} else {
			// ��ȡ��ʱ�ִ������״�����
			name = zval_try_get_tmp_string(property, &tmp_name);
			// ���û��������
			if (UNEXPECTED(!name)) {
				// ��� opline ��������� ��������ʱ�������ѽ�� zval ���Ϊδ����
				UNDEF_RESULT();
				// ����
				break;
			}
		}
		// ����ǳ���������һ�����������չֵ��Ϊλ�ã�������null
		cache_slot = (OP2_TYPE == IS_CONST) ? CACHE_ADDR((opline+1)->extended_value) : NULL;
		// ȡ������ָ���ָ�룬p1:����p2:��������p3:�������ͣ�p4:����λ��
		// zend_std_get_property_ptr_ptr�� ����ҵ�����
		if (EXPECTED((zptr = zobj->handlers->get_property_ptr_ptr(zobj, name, BP_VAR_RW, cache_slot)) != NULL)) {
			// ������ص��Ǵ���
			if (UNEXPECTED(Z_ISERROR_P(zptr))) {
				// ��� ���������������Ч(����IS_UNUSED)
				if (UNEXPECTED(RETURN_VALUE_USED(opline))) {
					// ��� Ϊ null
					ZVAL_NULL(EX_VAR(opline->result.var));
				}
			// ����
			} else {
				zval *orig_zptr = zptr;
				zend_reference *ref;

				// Ϊ��break;
				do {
					// �������������
					if (UNEXPECTED(Z_ISREF_P(zptr))) {
						// ȡ������ʵ��
						ref = Z_REF_P(zptr);
						// ������
						zptr = Z_REFVAL_P(zptr);
						// �������ʵ�� �� ����
						if (UNEXPECTED(ZEND_REF_HAS_TYPE_SOURCES(ref))) {
							// ���ݲ��������� ������Ŀ�� ���� ������ ��ֵ������p1:����ʵ����p2:�ڶ�������ֵ
							// ��/��/��/��/����λ��/����/��/��/�ֻ�/�� ����
							zend_binary_assign_op_typed_ref(ref, value OPLINE_CC EXECUTE_DATA_CC);
							// ����
							break;
						}
					}

					// ���op2�ǳ���
					if (OP2_TYPE == IS_CONST) {
						// �ӻ����ȡ
						// ��������ĵ�һ��Ԫ��
						prop_info = (zend_property_info*)CACHED_PTR_EX(cache_slot + 2);
					// ����
					} else {
						// ��֤ ������Ч�������Ա��У� �� ��ȡ����������Ϣ��p1:����p2:����zval
						prop_info = zend_object_fetch_property_type_info(Z_OBJ_P(object), orig_zptr);
					}
					// �����������Ϣ
					if (UNEXPECTED(prop_info)) {
						// �������͵����Խ������⴦��
						/* special case for typed properties */
						// ����������Ϣ������ ��������� ���� ������ ��ֵ������p1:������Ϣ��p2:��һ������ֵ��p3:�ڶ�������ֵ
						zend_binary_assign_op_typed_prop(prop_info, zptr, value OPLINE_CC EXECUTE_DATA_CC);
					// ����
					} else {
						// ���ò����루��opline�������Ӧ�Ķ����ƴ�������p1:����ֵ��p2:op1��p3:op2
						zend_binary_op(zptr, zptr, value OPLINE_CC);
					}
				} while (0);

				// ��� ���������������Ч(����IS_UNUSED)
				if (UNEXPECTED(RETURN_VALUE_USED(opline))) {
					// ����ֵ������Ϊ��� 
					ZVAL_COPY(EX_VAR(opline->result.var), zptr);
				}
			}
		// ����
		} else {
			// ��ȡ����ֵ���������㣬����ɹ�������ֵд�������
			// p1:����p2:��������p3:����λ�ã�p4:op2��op1��ԭ����ֵ��
			zend_assign_op_overloaded_property(zobj, name, cache_slot, value OPLINE_CC EXECUTE_DATA_CC);
		}
		// ��� OP2 ���ǳ���
		if (OP2_TYPE != IS_CONST) {
			// �ͷ���ʱ����
			zend_tmp_string_release(tmp_name);
		}
	} while (0);

	// �ͷ���һ��������� op1��CONST/UNUSED/CV ���������֧�� TMPVARCV��
	FREE_OP_DATA();
	// �ͷŲ�������ĸ��ӱ���
	FREE_OP2();
	// �ͷŲ�������ĸ��ӱ���
	FREE_OP1();
	// ���������Ը�ֵ������������
	/* assign_obj has two opcodes! */
	// ƫ�Ƶ�Ŀ������벢 return ,p1:�Ƿ����쳣��ʹ�� EX(opline)  ��ʹ�� opline ��p2:ƫ����
	ZEND_VM_NEXT_OPCODE_EX(1, 2);
}

// ing3, ��ȡ��̬����ֵ
/* No specialization for op_types (CONST|TMP|VAR|CV, UNUSED|CONST|TMPVAR) */
ZEND_VM_HANDLER(29, ZEND_ASSIGN_STATIC_PROP_OP, ANY, ANY, OP)
{
	// ������� 
	/* This helper actually never will receive IS_VAR as second op, and has the same handling for VAR and TMP in the first op, but for interoperability with the other binary_assign_op helpers, it is necessary to "include" it */

	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *prop, *value;
	zend_property_info *prop_info;
	zend_reference *ref;

	// windows: �޲���
	SAVE_OPLINE();

	// ͨ������λ�ú����ͣ����ؾ�̬���Ե�ַ��p1:��������ֵ��p2:����������Ϣ��p3:����λ�ã�p4:��ȡ���ͣ�p5:falgs
	if (UNEXPECTED(zend_fetch_static_property_address(&prop, &prop_info, (opline+1)->extended_value, BP_VAR_RW, 0 OPLINE_CC EXECUTE_DATA_CC) != SUCCESS)) {
		// ��� opline ��������� ��������ʱ�������ѽ�� zval ���Ϊδ����
		UNDEF_RESULT();
		// �ͷ���һ��������� op1��CONST/UNUSED/CV ���������֧�� TMPVARCV��
		FREE_OP_DATA();
		// ��Ҫ�� return
		HANDLE_EXCEPTION();
	}

	// ȡ����һ��������� zval��UNUSED����null, ��֧�� TMPVARCV��
	value = GET_OP_DATA_ZVAL_PTR(BP_VAR_R);

	// Ϊ��break
	do {
		// ��� ����ֵ����������
		if (UNEXPECTED(Z_ISREF_P(prop))) {
			// ȡ������ʵ��
			ref = Z_REF_P(prop);
			// ������
			prop = Z_REFVAL_P(prop);
			// �������ʵ�� �� ����
			if (UNEXPECTED(ZEND_REF_HAS_TYPE_SOURCES(ref))) {
				// ���ݲ��������� ������Ŀ�� ���� ������ ��ֵ������p1:����ʵ����p2:�ڶ�������ֵ
				// ��/��/��/��/����λ��/����/��/��/�ֻ�/�� ����
				zend_binary_assign_op_typed_ref(ref, value OPLINE_CC EXECUTE_DATA_CC);
				// ����
				break;
			}
		}
		// ������ʹ���
		if (UNEXPECTED(ZEND_TYPE_IS_SET(prop_info->type))) {
			// �������͵����Խ������⴦��
			/* special case for typed properties */
			// ����������Ϣ������ ��������� ���� ������ ��ֵ������p1:������Ϣ��p2:��һ������ֵ��p3:�ڶ�������ֵ
			zend_binary_assign_op_typed_prop(prop_info, prop, value OPLINE_CC EXECUTE_DATA_CC);
		// ����
		} else {
			// ���ò����루�� opline �������Ӧ�Ķ����ƴ�������p1:����ֵ��p2:op1��p3:op2
			zend_binary_op(prop, prop, value OPLINE_CC);
		}
	} while (0);

	// ��� ���������������Ч(����IS_UNUSED)
	if (UNEXPECTED(RETURN_VALUE_USED(opline))) {
		// ���Ϊ����ֵ
		ZVAL_COPY(EX_VAR(opline->result.var), prop);
	}

	// �ͷ���һ��������� op1��CONST/UNUSED/CV ���������֧�� TMPVARCV��
	FREE_OP_DATA();
	// ��̬���Ը�ֵ��2��������
	/* assign_static_prop has two opcodes! */
	// ƫ�Ƶ�Ŀ������벢 return ,p1:�Ƿ����쳣��ʹ�� EX(opline)  ��ʹ�� opline ��p2:ƫ����
	ZEND_VM_NEXT_OPCODE_EX(1, 2);
}

// ing2, ���ά�ȸ�ֵ
ZEND_VM_HANDLER(27, ZEND_ASSIGN_DIM_OP, VAR|CV, CONST|TMPVAR|UNUSED|NEXT|CV, OP)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *var_ptr;
	zval *value, *container, *dim;
	HashTable *ht;

	// windows: �޲���
	SAVE_OPLINE();
	// ��ȡzvalָ��, TMP/CONST����null, UNUSED ���� $this����֧�� TMPVAR/TMPVARCV��CV ֱ�ӷ��ء�
	container = GET_OP1_OBJ_ZVAL_PTR_PTR_UNDEF(BP_VAR_RW);

	// ��� ���� ������ ����
	if (EXPECTED(Z_TYPE_P(container) == IS_ARRAY)) {
// ������ת��ǩ assign_dim_op_array
ZEND_VM_C_LABEL(assign_dim_op_array):
		// ���������鴴����������ʹ�ø���
		SEPARATE_ARRAY(container);
		// �Ӵ�����ȡ����ϣ��
		ht = Z_ARRVAL_P(container);
// ������ת��ǩ assign_dim_op_new_array
ZEND_VM_C_LABEL(assign_dim_op_new_array):
		// ��ȡzvalָ��, UNUSED ����null
		dim = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
		// ��� op2 ������δ����
		if (OP2_TYPE == IS_UNUSED) {
			// ���ϣ�����δ��ʼ����zval
			var_ptr = zend_hash_next_index_insert(ht, &EG(uninitialized_zval));
			// ������ʧ��
			if (UNEXPECTED(!var_ptr)) {
				// �������Ԫ��ʧ�ܣ���һ��Ԫ���Ѿ�����
				zend_cannot_add_element();
				// ��ת��ָ����ǩ assign_dim_op_ret_null
				ZEND_VM_C_GOTO(assign_dim_op_ret_null);
			}
		// ����
		} else {
			// ���op2�ǳ���
			if (OP2_TYPE == IS_CONST) {
				// ������һ�����£�����Ϊ������
				var_ptr = zend_fetch_dimension_address_inner_RW_CONST(ht, dim EXECUTE_DATA_CC);
			// ����
			} else {
				// ������һ�����£�����Ϊ����
				var_ptr = zend_fetch_dimension_address_inner_RW(ht, dim EXECUTE_DATA_CC);
			}
			// ������� ���ɹ�
			if (UNEXPECTED(!var_ptr)) {
				// ��ת��ָ����ǩ assign_dim_op_ret_null
				ZEND_VM_C_GOTO(assign_dim_op_ret_null);
			}
		}

		// ��ִ��������ͨ�� p1:����������ͣ�p3:�������ͣ�ȡ��p2:�������ָ���zval
		//������ȡ��һ�в����룩���������δ���屨��
		value = get_op_data_zval_ptr_r((opline+1)->op1_type, (opline+1)->op1);

		do {
			// ���op2��Ч ��������������
			if (OP2_TYPE != IS_UNUSED && UNEXPECTED(Z_ISREF_P(var_ptr))) {
				// ȡ������ ʵ��
				zend_reference *ref = Z_REF_P(var_ptr);
				// ������
				var_ptr = Z_REFVAL_P(var_ptr);
				// �������ʵ�� �� ����
				if (UNEXPECTED(ZEND_REF_HAS_TYPE_SOURCES(ref))) {
					// ���ݲ��������� ������Ŀ�� ���� ������ ��ֵ������p1:����ʵ����p2:�ڶ�������ֵ
					// ��/��/��/��/����λ��/����/��/��/�ֻ�/�� ����
					zend_binary_assign_op_typed_ref(ref, value OPLINE_CC EXECUTE_DATA_CC);
					// ����
					break;
				}
			}
			// ���ò����루��opline�������Ӧ�Ķ����ƴ�������p1:����ֵ��p2:op1��p3:op2
			zend_binary_op(var_ptr, var_ptr, value OPLINE_CC);
		} while (0);

		// ��� ���������������Ч(����IS_UNUSED)
		if (UNEXPECTED(RETURN_VALUE_USED(opline))) {
			// ������ ���� ���������
			ZVAL_COPY(EX_VAR(opline->result.var), var_ptr);
		}
		// �ͷţ����� �� ��ʱ���� ���͵ģ��������zval����p1:���ͣ�p2:���
		FREE_OP((opline+1)->op1_type, (opline+1)->op1.var);
	// ����
	} else {
		// �����������������
		if (EXPECTED(Z_ISREF_P(container))) {
			// ������
			container = Z_REFVAL_P(container);
			// ��� ���� ������ ����
			if (EXPECTED(Z_TYPE_P(container) == IS_ARRAY)) {
				// ��ת��ָ����ǩ assign_dim_op_array
				ZEND_VM_C_GOTO(assign_dim_op_array);
			}
		}

		// ��� ���� ������ ����
		if (EXPECTED(Z_TYPE_P(container) == IS_OBJECT)) {
			// ȡ�������еĶ���
			zend_object *obj = Z_OBJ_P(container);

			// ��ȡzvalָ��, UNUSED ����null
			dim = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
			// ���op2�ǳ��� ���� �ж���ֵ��
			if (OP2_TYPE == IS_CONST && Z_EXTRA_P(dim) == ZEND_EXTRA_VALUE) {
				// ָ����һ��λ�õı���
				dim++;
			}
			// ������һ�������Ը�ֵ
			zend_binary_assign_op_obj_dim(obj, dim OPLINE_CC EXECUTE_DATA_CC);
		// ���� ��� ���� ������ undef/null/false
		} else if (EXPECTED(Z_TYPE_P(container) <= IS_FALSE)) {
			zend_uchar old_type;

			// ���op1�Ǳ������ ���� ��������Ϊδ����
			if (OP1_TYPE == IS_CV && UNEXPECTED(Z_TYPE_INFO_P(container) == IS_UNDEF)) {
				// ����p1�ı�������������δ����, ����δ��ʼ��zval
				ZVAL_UNDEFINED_OP1();
			}
			// ���������飬8��Ԫ��
			ht = zend_new_array(8);
			// ����ԭ��������
			old_type = Z_TYPE_P(container);
			// ��ϣ��ŵ�������
			ZVAL_ARR(container, ht);
			// ����������� false
			if (UNEXPECTED(old_type == IS_FALSE)) {
				// �������ô���
				GC_ADDREF(ht);
				// ������falseת������������
				zend_false_to_array_deprecated();
				// ���ô���-1�����Ϊ0
				if (UNEXPECTED(GC_DELREF(ht) == 0)) {
					// ���ٹ�ϣ��
					zend_array_destroy(ht);
					// ��ת��ָ����ǩ assign_dim_op_ret_null
					ZEND_VM_C_GOTO(assign_dim_op_ret_null);
				}
			}
			// ��ת��ָ����ǩ assign_dim_op_new_array
			ZEND_VM_C_GOTO(assign_dim_op_new_array);
		// ����
		} else {
			// ��ȡzvalָ�룬UNUSED ���ͷ���null
			dim = GET_OP2_ZVAL_PTR(BP_VAR_R);
			// ��ֻ�Ǳ����޷�ʹ��ƫ�������в���
			zend_binary_assign_op_dim_slow(container, dim OPLINE_CC EXECUTE_DATA_CC);
// ������ת��ǩ assign_dim_op_ret_null
ZEND_VM_C_LABEL(assign_dim_op_ret_null):
			// �ͷ���һ��������� op1��CONST/UNUSED/CV ���������֧�� TMPVARCV��
			FREE_OP_DATA();
			// ��� ���������������Ч(����IS_UNUSED)
			if (UNEXPECTED(RETURN_VALUE_USED(opline))) {
				// ���Ϊnull
				ZVAL_NULL(EX_VAR(opline->result.var));
			}
		}
	}

	// �ͷŲ�������ĸ��ӱ���
	FREE_OP2();
	// �ͷŲ�������ĸ��ӱ���
	FREE_OP1();
	// ƫ�Ƶ�Ŀ������벢 return ,p1:�Ƿ����쳣��ʹ�� EX(opline)  ��ʹ�� opline ��p2:ƫ����
	ZEND_VM_NEXT_OPCODE_EX(1, 2);
}

// ing3, �����Ƹ�ֵ���㣺 ��/��/��/��/����λ��/����/��/��/�ֻ�/�� ����
ZEND_VM_HANDLER(26, ZEND_ASSIGN_OP, VAR|CV, CONST|TMPVAR|CV, OP)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *var_ptr;
	zval *value;

	// windows: �޲���
	SAVE_OPLINE();
	// ��ȡzvalָ�룬UNUSED ���ͷ���null
	value = GET_OP2_ZVAL_PTR(BP_VAR_R);
	// ��ȡzvalָ��,TMP/CONST/UNUSED ����null����֧�� TMPVAR/TMPVARCV
	var_ptr = GET_OP1_ZVAL_PTR_PTR(BP_VAR_RW);

	do {
		// ���ԭֵ����������
		if (UNEXPECTED(Z_TYPE_P(var_ptr) == IS_REFERENCE)) {
			// ȡ������ ʵ��
			zend_reference *ref = Z_REF_P(var_ptr);
			// ������
			var_ptr = Z_REFVAL_P(var_ptr);
			// �������ʵ�� �� ����
			if (UNEXPECTED(ZEND_REF_HAS_TYPE_SOURCES(ref))) {
				// ���ݲ��������� ������Ŀ�� ���� ������ ��ֵ������p1:����ʵ����p2:�ڶ�������ֵ
				zend_binary_assign_op_typed_ref(ref, value OPLINE_CC EXECUTE_DATA_CC);
				break;
			}
		}
		// ���ò����루��opline�������Ӧ�Ķ����ƴ�������p1:����ֵ��p2:op1��p3:op2
		zend_binary_op(var_ptr, var_ptr, value OPLINE_CC);
	} while (0);

	// ��� ���������������Ч(����IS_UNUSED)
	if (UNEXPECTED(RETURN_VALUE_USED(opline))) {
		// ������ ���� ���������
		ZVAL_COPY(EX_VAR(opline->result.var), var_ptr);
	}

	// �ͷŲ�������ĸ��ӱ���
	FREE_OP2();
	// �ͷŲ�������ĸ��ӱ���
	FREE_OP1();
	// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, �������ԣ�ǰ������
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

	// windows: �޲���
	SAVE_OPLINE();
	// ��ȡzvalָ��, TMP/CONST����null, UNUSED ���� $this����֧�� TMPVAR/TMPVARCV��CV ֱ�ӷ��ء�
	object = GET_OP1_OBJ_ZVAL_PTR_PTR_UNDEF(BP_VAR_RW);
	// ��ȡzvalָ�룬UNUSED ���ͷ���null
	property = GET_OP2_ZVAL_PTR(BP_VAR_R);

	// Ϊ��break
	do {
		// ���op1��ʹ�� ���� ���Ͳ��Ƕ���
		if (OP1_TYPE != IS_UNUSED && UNEXPECTED(Z_TYPE_P(object) != IS_OBJECT)) {
			// ������������� ���� ����Ŀ���Ƕ���
			if (Z_ISREF_P(object) && Z_TYPE_P(Z_REFVAL_P(object)) == IS_OBJECT) {
				// ������
				object = Z_REFVAL_P(object);
				// ��ת��ָ����ǩ pre_incdec_object
				ZEND_VM_C_GOTO(pre_incdec_object);
			}
			// ��� op1 �� ������� ���� ��������Ϊ δ����
			if (OP1_TYPE == IS_CV
			 && UNEXPECTED(Z_TYPE_P(object) == IS_UNDEF)) {
				// ����p1�ı�������������δ����, ����δ��ʼ��zval
				ZVAL_UNDEFINED_OP1();
			}
			// û�з��ʵĶ��󣬱���
			// �׳���������Ч���쳣
			zend_throw_non_object_error(object, property OPLINE_CC EXECUTE_DATA_CC);
			// ����
			break;
		}

// ������ת��ǩ pre_incdec_object
ZEND_VM_C_LABEL(pre_incdec_object):
		// �������ȷ�������  ���� ���в���
		/* here we are sure we are dealing with an object */
		// ȡ�� ����
		zobj = Z_OBJ_P(object);
		// ���op2�ǳ���
		if (OP2_TYPE == IS_CONST) {
			// ȡ�� ������
			name = Z_STR_P(property);
		// ����op2���ǳ���
		} else {
			// ��ȡ��ʱ�ִ������״�����
			name = zval_try_get_tmp_string(property, &tmp_name);
			// ���û��������
			if (UNEXPECTED(!name)) {
				// ��� opline ��������� ��������ʱ�������ѽ�� zval ���Ϊδ����
				UNDEF_RESULT();
				// ����
				break;
			}
		}
		// ���op2�ǳ�������ȡ����λ��
		cache_slot = (OP2_TYPE == IS_CONST) ? CACHE_ADDR(opline->extended_value) : NULL;
		// ȡ�ö������ԡ�����ɹ�
		if (EXPECTED((zptr = zobj->handlers->get_property_ptr_ptr(zobj, name, BP_VAR_RW, cache_slot)) != NULL)) {
			// �����ȡ������
			if (UNEXPECTED(Z_ISERROR_P(zptr))) {
				// ��� ���������������Ч(����IS_UNUSED)
				if (UNEXPECTED(RETURN_VALUE_USED(opline))) {
					// ���Ϊnull
					ZVAL_NULL(EX_VAR(opline->result.var));
				}
			// ���򣬻�ȡ�ɹ�
			} else {
				// ���op2�ǳ���
				if (OP2_TYPE == IS_CONST) {
					// ��������ĵ�һ��Ԫ��
					prop_info = (zend_property_info *) CACHED_PTR_EX(cache_slot + 2);
				// ����
				} else {
					// ��֤ ������Ч�������Ա��У� �� ��ȡ����������Ϣ��p1:����p2:����zval
					prop_info = zend_object_fetch_property_type_info(Z_OBJ_P(object), zptr);
				}
				// ���� ǰ�� �������Լ�
				zend_pre_incdec_property_zval(zptr, prop_info OPLINE_CC EXECUTE_DATA_CC);
			}
		// ��ȡ����ʧ��
		} else {
			// ȡ������ֵ������ ǰ����/�Լ� �������»�ȥ������ᴴ����
			zend_pre_incdec_overloaded_property(zobj, name, cache_slot OPLINE_CC EXECUTE_DATA_CC);
		}
		// ��� OP2 ���ǳ���
		if (OP2_TYPE != IS_CONST) {
			// �ͷ���ʱ����
			zend_tmp_string_release(tmp_name);
		}
	} while (0);

	// �ͷŲ�������ĸ��ӱ���
	FREE_OP2();
	// �ͷŲ�������ĸ��ӱ���
	FREE_OP1();
	// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, �������ԣ�ǰ���Լ�
ZEND_VM_HANDLER(133, ZEND_PRE_DEC_OBJ, VAR|UNUSED|THIS|CV, CONST|TMPVAR|CV, CACHE_SLOT)
{
	// ��ת���������ǩ 
	ZEND_VM_DISPATCH_TO_HANDLER(ZEND_PRE_INC_OBJ);
}

// ing3, ������Լ� ��++���� 3*3=9����֧��ȷ�Ϲ�����
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

	// windows: �޲���
	SAVE_OPLINE();
	// ��ȡzvalָ��, TMP/CONST����null, UNUSED ���� $this����֧�� TMPVAR/TMPVARCV��CV ֱ�ӷ��ء�
	object = GET_OP1_OBJ_ZVAL_PTR_PTR_UNDEF(BP_VAR_RW);
	// ��ȡzvalָ�룬UNUSED ���ͷ���null
	property = GET_OP2_ZVAL_PTR(BP_VAR_R);

	// ��һ��do��Ϊ�˿��Է������ break����������Ҫ�ӱ�ʶ����if�Ƚ��鷳
	do {
		// op1���Ͳ���δ���� ���� ���Ƕ���
		if (OP1_TYPE != IS_UNUSED && UNEXPECTED(Z_TYPE_P(object) != IS_OBJECT)) {
			// ��������� ���� ���õ��Ƕ���
			if (Z_ISREF_P(object) && Z_TYPE_P(Z_REFVAL_P(object)) == IS_OBJECT) {
				// �л��������õĶ���
				object = Z_REFVAL_P(object);
				// ��ת��ָ����ǩ post_incdec_object
				ZEND_VM_C_GOTO(post_incdec_object);
			}
			// ���op1�����Ǳ������ ���� ���������� δ����
			if (OP1_TYPE == IS_CV
			 && UNEXPECTED(Z_TYPE_P(object) == IS_UNDEF)) {
				// ����p1�ı�������������δ����, ����δ��ʼ��zval
				ZVAL_UNDEFINED_OP1();
			}
			// ������������� 
			zend_throw_non_object_error(object, property OPLINE_CC EXECUTE_DATA_CC);
			// ����
			break;
		}
// ������ת��ǩ post_incdec_object
ZEND_VM_C_LABEL(post_incdec_object):
		// ����ȷ�����ڶԶ�����в���
		/* here we are sure we are dealing with an object */
		// ȡ�������еĶ���
		zobj = Z_OBJ_P(object);
		// op2�����ǳ���
		if (OP2_TYPE == IS_CONST) {
			// ������
			name = Z_STR_P(property);
		// op2���Ͳ��ǳ���
		// ����
		} else {
			// ȡ����ʱ�ִ�������������
			name = zval_try_get_tmp_string(property, &tmp_name);
			// ���������
			if (UNEXPECTED(!name)) {
				// ���Ϊ δ����
				ZVAL_UNDEF(EX_VAR(opline->result.var));
				// ����
				break;
			}
		}
		// ȡ�û���λ�ã�op2�ǳ���ʱ����Ч��
		cache_slot = (OP2_TYPE == IS_CONST) ? CACHE_ADDR(opline->extended_value) : NULL;
		// ����ܴӻ����л�ȡ��������
		if (EXPECTED((zptr = zobj->handlers->get_property_ptr_ptr(zobj, name, BP_VAR_RW, cache_slot)) != NULL)) {
			// ��� ���г���
			if (UNEXPECTED(Z_ISERROR_P(zptr))) {
				// ���Ϊnull
				ZVAL_NULL(EX_VAR(opline->result.var));
			// �������
			// ����
			} else {
				// op2����Ϊ����
				if (OP2_TYPE == IS_CONST) {
					// ��ȡ������Ϣ
					// ��������ĵ�һ��Ԫ��
					prop_info = (zend_property_info*)CACHED_PTR_EX(cache_slot + 2);					
				// op2���ǳ���
				// ����
				} else {
					// ��֤ ������Ч�������Ա��У� �� ��ȡ����������Ϣ��p1:����p2:����zval
					prop_info = zend_object_fetch_property_type_info(Z_OBJ_P(object), zptr);
				}
				// ���Եĺ��� �������Լ����㣿 ���������еĲ��� opline,execute_data
				zend_post_incdec_property_zval(zptr, prop_info OPLINE_CC EXECUTE_DATA_CC);
			}
		// ����
		} else {
			// ȡ������ֵ������ ������/�Լ� �������»�ȥ
			zend_post_incdec_overloaded_property(zobj, name, cache_slot OPLINE_CC EXECUTE_DATA_CC);
		}
		// op2���Ͳ��ǳ���
		if (OP2_TYPE != IS_CONST) {
			// �ͷ�����
			zend_tmp_string_release(tmp_name);
		}
	} while (0);
	
	// �ͷŲ�������ĸ��ӱ���
	FREE_OP2();
	// �ͷŲ�������ĸ��ӱ���
	FREE_OP1();
	// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, ������Լ� ��--����
ZEND_VM_HANDLER(135, ZEND_POST_DEC_OBJ, VAR|UNUSED|THIS|CV, CONST|TMPVAR|CV, CACHE_SLOT)
{
	// ��ת���������ǩ 
	ZEND_VM_DISPATCH_TO_HANDLER(ZEND_POST_INC_OBJ);
}

// ing3, ��ǰ����������
/* No specialization for op_types (CONST|TMPVAR|CV, UNUSED|CONST|VAR) */
ZEND_VM_HANDLER(38, ZEND_PRE_INC_STATIC_PROP, ANY, ANY, CACHE_SLOT)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *prop;
	zend_property_info *prop_info;

	// windows: �޲���
	SAVE_OPLINE();

	// ͨ������λ�ú����ͣ����ؾ�̬���Ե�ַ��p1:��������ֵ��p2:����������Ϣ��p3:����λ�ã�p4:��ȡ���ͣ�p5:falgs
	if (zend_fetch_static_property_address(&prop, &prop_info, opline->extended_value, BP_VAR_RW, 0 OPLINE_CC EXECUTE_DATA_CC) != SUCCESS) {
		// ��� opline ��������� ��������ʱ�������ѽ�� zval ���Ϊδ����
		UNDEF_RESULT();
		// ��Ҫ�� return
		HANDLE_EXCEPTION();
	}

	// ���� ǰ�� �������Լ�
	// ������ʹ��ڣ�ʹ��������Ϣ������ null
	zend_pre_incdec_property_zval(prop,
		ZEND_TYPE_IS_SET(prop_info->type) ? prop_info : NULL OPLINE_CC EXECUTE_DATA_CC);

	// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, ��ǰ�������Լ�
/* No specialization for op_types (CONST|TMPVAR|CV, UNUSED|CONST|VAR) */
ZEND_VM_HANDLER(39, ZEND_PRE_DEC_STATIC_PROP, ANY, ANY, CACHE_SLOT)
{
	// ��ת���������ǩ ZEND_PRE_INC_STATIC_PROP
	ZEND_VM_DISPATCH_TO_HANDLER(ZEND_PRE_INC_STATIC_PROP);
}

// ing3, ������������
/* No specialization for op_types (CONST|TMPVAR|CV, UNUSED|CONST|VAR) */
ZEND_VM_HANDLER(40, ZEND_POST_INC_STATIC_PROP, ANY, ANY, CACHE_SLOT)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *prop;
	zend_property_info *prop_info;

	// windows: �޲���
	SAVE_OPLINE();

	// ͨ������λ�ú����ͣ����ؾ�̬���Ե�ַ��p1:��������ֵ��p2:����������Ϣ��p3:����λ�ã�p4:��ȡ���ͣ�p5:falgs
	if (zend_fetch_static_property_address(&prop, &prop_info, opline->extended_value, BP_VAR_RW, 0 OPLINE_CC EXECUTE_DATA_CC) != SUCCESS) {
		// ��� opline ��������� ��������ʱ�������ѽ�� zval ���Ϊδ����
		UNDEF_RESULT();
		// ��Ҫ�� return
		HANDLE_EXCEPTION();
	}

	// ���� ���� �������Լ�
	// ������ʹ��ڣ�ʹ�ô�������Ϣ��������������Ϣ
	zend_post_incdec_property_zval(prop,
		ZEND_TYPE_IS_SET(prop_info->type) ? prop_info : NULL OPLINE_CC EXECUTE_DATA_CC);

	// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, ���������Լ�
/* No specialization for op_types (CONST|TMPVAR|CV, UNUSED|CONST|VAR) */
ZEND_VM_HANDLER(41, ZEND_POST_DEC_STATIC_PROP, ANY, ANY, CACHE_SLOT)
{
	// ��ת���������ǩ ZEND_POST_INC_STATIC_PROP
	ZEND_VM_DISPATCH_TO_HANDLER(ZEND_POST_INC_STATIC_PROP);
}

// ing3, (ǰ)���� ����, ���鴫��ı����Ƿ���Ч������ ���� ���㣬Ȼ�������������op1
ZEND_VM_HELPER(zend_pre_inc_helper, VAR|CV, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *var_ptr;
	// ��ȡzvalָ��, TMP/CONST/UNUSED ����null����֧�� TMPVAR/TMPVARCV
	var_ptr = GET_OP1_ZVAL_PTR_PTR_UNDEF(BP_VAR_RW);

	// windows: �޲���
	SAVE_OPLINE();
	// ��� op1 �� ������� ���� ����ָ���� δ����
	if (OP1_TYPE == IS_CV && UNEXPECTED(Z_TYPE_P(var_ptr) == IS_UNDEF)) {
		// ����p1�ı�������������δ����, ����δ��ʼ��zval
		ZVAL_UNDEFINED_OP1();
		// ��ԭֵ����Ϊ null
		ZVAL_NULL(var_ptr);
	}

	do {
		// ���ԭֵ����������
		if (UNEXPECTED(Z_TYPE_P(var_ptr) == IS_REFERENCE)) {
			// ȡ������ ʵ��
			zend_reference *ref = Z_REF_P(var_ptr);
			// ������
			var_ptr = Z_REFVAL_P(var_ptr);
			// �������ʵ�� �� ����
			if (UNEXPECTED(ZEND_REF_HAS_TYPE_SOURCES(ref))) {
				// �������͵�����Ŀ���� ���� �� �Լ���p1:���ö���p2:����ֵ
				zend_incdec_typed_ref(ref, NULL OPLINE_CC EXECUTE_DATA_CC);
				// ������������
				break;
			}
		}
		// ��������
		increment_function(var_ptr);
	} while (0);

	// ��� ���������������Ч(����IS_UNUSED)
	if (UNEXPECTED(RETURN_VALUE_USED(opline))) {
		// ������ ���� ���������
		ZVAL_COPY(EX_VAR(opline->result.var), var_ptr);
	}

	// �ͷŲ�������ĸ��ӱ���
	FREE_OP1();
	// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, ��ǰ������
ZEND_VM_HOT_HANDLER(34, ZEND_PRE_INC, VAR|CV, ANY, SPEC(RETVAL))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *var_ptr;
	// ��ȡzvalָ��, TMP/CONST/UNUSED ����null����֧�� TMPVAR/TMPVARCV
	var_ptr = GET_OP1_ZVAL_PTR_PTR_UNDEF(BP_VAR_RW);

	// ����������
	if (EXPECTED(Z_TYPE_P(var_ptr) == IS_LONG)) {
		// ��������
		fast_long_increment_function(var_ptr);
		// ��� ���������������Ч(����IS_UNUSED)
		if (UNEXPECTED(RETURN_VALUE_USED(opline))) {
			// ֵ���Ƹ� ��� 
			ZVAL_COPY_VALUE(EX_VAR(opline->result.var), var_ptr);
		}
		// opline + 1����Ŀ������벢 return
		ZEND_VM_NEXT_OPCODE();
	}

	// ���÷��� zend_pre_inc_helper
	ZEND_VM_DISPATCH_TO_HELPER(zend_pre_inc_helper);
}

// ing3, (ǰ)�Լ� ����, ���鴫��ı����Ƿ���Ч������ �Լ� ���㣬Ȼ�������������op1
ZEND_VM_HELPER(zend_pre_dec_helper, VAR|CV, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *var_ptr;
	// ��ȡzvalָ��, TMP/CONST/UNUSED ����null����֧�� TMPVAR/TMPVARCV
	var_ptr = GET_OP1_ZVAL_PTR_PTR_UNDEF(BP_VAR_RW);

	// windows: �޲���
	SAVE_OPLINE();
	// ��� op1 �� ������� ���� ����ָ����δ����
	if (OP1_TYPE == IS_CV && UNEXPECTED(Z_TYPE_P(var_ptr) == IS_UNDEF)) {
		// ����p1�ı�������������δ����, ����δ��ʼ��zval
		ZVAL_UNDEFINED_OP1();
		// ��ԭֵ����Ϊ null
		ZVAL_NULL(var_ptr);
	}

	do {
		// ���ԭֵ����������
		if (UNEXPECTED(Z_TYPE_P(var_ptr) == IS_REFERENCE)) {
			// ȡ������ ʵ��
			zend_reference *ref = Z_REF_P(var_ptr);
			// ������
			var_ptr = Z_REFVAL_P(var_ptr);
			// �������ʵ�� �� ����
			if (UNEXPECTED(ZEND_REF_HAS_TYPE_SOURCES(ref))) {
				// �������͵�����Ŀ���� ���� �� �Լ���p1:���ö���p2:����ֵ
				zend_incdec_typed_ref(ref, NULL OPLINE_CC EXECUTE_DATA_CC);
				// �����Լ�����
				break;
			}
		}
		// �Լ�����
		decrement_function(var_ptr);
	} while (0);

	// ��� ���������������Ч(����IS_UNUSED)
	if (UNEXPECTED(RETURN_VALUE_USED(opline))) {
		// ������ ���� ���������
		ZVAL_COPY(EX_VAR(opline->result.var), var_ptr);
	}

	// �ͷŲ�������ĸ��ӱ���
	FREE_OP1();
	// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, (ǰ)�Լ�
ZEND_VM_HOT_HANDLER(35, ZEND_PRE_DEC, VAR|CV, ANY, SPEC(RETVAL))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *var_ptr;
	// ��ȡzvalָ��, TMP/CONST/UNUSED ����null����֧�� TMPVAR/TMPVARCV
	var_ptr = GET_OP1_ZVAL_PTR_PTR_UNDEF(BP_VAR_RW);

	// ���������
	if (EXPECTED(Z_TYPE_P(var_ptr) == IS_LONG)) {
		// ������
		fast_long_decrement_function(var_ptr);
		// ��� ���������������Ч(����IS_UNUSED)
		if (UNEXPECTED(RETURN_VALUE_USED(opline))) {
			// ������
			ZVAL_COPY_VALUE(EX_VAR(opline->result.var), var_ptr);
		}
		// opline + 1����Ŀ������벢 return
		ZEND_VM_NEXT_OPCODE();
	}

	// ���÷��� zend_pre_inc_helper
	ZEND_VM_DISPATCH_TO_HELPER(zend_pre_dec_helper);
}

// ing3, (��)���� ����, ���鴫��ı����Ƿ���Ч������ ���� ���㣬Ȼ�������������op1
ZEND_VM_HELPER(zend_post_inc_helper, VAR|CV, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *var_ptr;
	// ��ȡzvalָ��, TMP/CONST/UNUSED ����null����֧�� TMPVAR/TMPVARCV
	var_ptr = GET_OP1_ZVAL_PTR_PTR_UNDEF(BP_VAR_RW);

	// windows: �޲���
	SAVE_OPLINE();
	// ��� op1 �� ������� ���ұ���ָ���� δ����
	if (OP1_TYPE == IS_CV && UNEXPECTED(Z_TYPE_P(var_ptr) == IS_UNDEF)) {
		// ����p1�ı�������������δ����, ����δ��ʼ��zval
		ZVAL_UNDEFINED_OP1();
		// ��ԭֵ����Ϊ null
		ZVAL_NULL(var_ptr);
	}

	do {
		// ���ԭֵ����������
		if (UNEXPECTED(Z_TYPE_P(var_ptr) == IS_REFERENCE)) {
			// ȡ������ ʵ��
			zend_reference *ref = Z_REF_P(var_ptr);
			// ������
			var_ptr = Z_REFVAL_P(var_ptr);
			// �������ʵ�� �� ����
			if (UNEXPECTED(ZEND_REF_HAS_TYPE_SOURCES(ref))) {
				// �������͵�����Ŀ���� ���� �� �Լ���p1:���ö���p2:����ֵ
				zend_incdec_typed_ref(ref, EX_VAR(opline->result.var) OPLINE_CC EXECUTE_DATA_CC);
				// �������� ����
				break;
			}
		}
		// ������ ���� ���������
		ZVAL_COPY(EX_VAR(opline->result.var), var_ptr);
		// ���غ󣬽�����������
		increment_function(var_ptr);
	} while (0);

	// �ͷŲ�������ĸ��ӱ���
	FREE_OP1();
	// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3,��������
ZEND_VM_HOT_HANDLER(36, ZEND_POST_INC, VAR|CV, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *var_ptr;
	// ��ȡzvalָ��, TMP/CONST/UNUSED ����null����֧�� TMPVAR/TMPVARCV
	var_ptr = GET_OP1_ZVAL_PTR_PTR_UNDEF(BP_VAR_RW);

	// ����� op1������
	if (EXPECTED(Z_TYPE_P(var_ptr) == IS_LONG)) {
		// �ȱ�������
		ZVAL_LONG(EX_VAR(opline->result.var), Z_LVAL_P(var_ptr));
		// ��������
		fast_long_increment_function(var_ptr);
		// opline + 1����Ŀ������벢 return
		ZEND_VM_NEXT_OPCODE();
	}

	// ���÷��� zend_post_inc_helper
	ZEND_VM_DISPATCH_TO_HELPER(zend_post_inc_helper);
}

// ing3, (��)�Լ� ����, ���鴫��ı����Ƿ���Ч������ �Լ� ���㣬Ȼ�������������op1
ZEND_VM_HELPER(zend_post_dec_helper, VAR|CV, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *var_ptr;
	// ��ȡzvalָ��, TMP/CONST/UNUSED ����null����֧�� TMPVAR/TMPVARCV
	var_ptr = GET_OP1_ZVAL_PTR_PTR_UNDEF(BP_VAR_RW);

	// windows: �޲���
	SAVE_OPLINE();
	// ��� op1 �� ������� ���� ����ָ�� ������δ����
	if (OP1_TYPE == IS_CV && UNEXPECTED(Z_TYPE_P(var_ptr) == IS_UNDEF)) {
		// ����p1�ı�������������δ����, ����δ��ʼ��zval
		ZVAL_UNDEFINED_OP1();
		// ��ԭֵ����Ϊ null
		ZVAL_NULL(var_ptr);
	}

	do {
		// ���ԭֵ����������
		if (UNEXPECTED(Z_TYPE_P(var_ptr) == IS_REFERENCE)) {
			// ȡ������ ʵ��
			zend_reference *ref = Z_REF_P(var_ptr);
			// ������
			var_ptr = Z_REFVAL_P(var_ptr);
			// �������ʵ�� �� ����
			if (UNEXPECTED(ZEND_REF_HAS_TYPE_SOURCES(ref))) {
				// �������͵�����Ŀ���� ���� �� �Լ���p1:���ö���p2:����ֵ
				zend_incdec_typed_ref(ref, EX_VAR(opline->result.var) OPLINE_CC EXECUTE_DATA_CC);
				// ������������
				break;
			}
		}
		// ������ ���� ���������
		ZVAL_COPY(EX_VAR(opline->result.var), var_ptr);
		// ������������
		decrement_function(var_ptr);
	} while (0);

	// �ͷŲ�������ĸ��ӱ���
	FREE_OP1();
	// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, �����Լ�
ZEND_VM_HOT_HANDLER(37, ZEND_POST_DEC, VAR|CV, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *var_ptr;

	// ��ȡzvalָ��, TMP/CONST/UNUSED ����null����֧�� TMPVAR/TMPVARCV
	var_ptr = GET_OP1_ZVAL_PTR_PTR_UNDEF(BP_VAR_RW);

	// ���������
	if (EXPECTED(Z_TYPE_P(var_ptr) == IS_LONG)) {
		// �ȱ��浽�����
		ZVAL_LONG(EX_VAR(opline->result.var), Z_LVAL_P(var_ptr));
		// �����Լ�
		fast_long_decrement_function(var_ptr);
		// opline + 1����Ŀ������벢 return
		ZEND_VM_NEXT_OPCODE();
	}

	// ���÷��� zend_post_dec_helper
	ZEND_VM_DISPATCH_TO_HELPER(zend_post_dec_helper);
}

// ing3, ������ ZEND_ECHO ����������һ��������������� CONST|TMPVAR|CV 
ZEND_VM_HANDLER(136, ZEND_ECHO, CONST|TMPVAR|CV, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *z;

	// windows: �޲���
	SAVE_OPLINE();
	// ��ȡzvalָ��, UNUSED ����null
	z = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);

	// ��������� string
	if (Z_TYPE_P(z) == IS_STRING) {
		// ȡ�� �����е� string
		zend_string *str = Z_STR_P(z);

		// ������ǿ�
		if (ZSTR_LEN(str) != 0) {
			// ��ӡ�ִ�
			zend_write(ZSTR_VAL(str), ZSTR_LEN(str));
		}
	// ��������
	} else {
		// ���ú��� ȡ�� op1 �е� string
		zend_string *str = zval_get_string_func(z);

		// ������ǿ�
		if (ZSTR_LEN(str) != 0) {
			// ��ӡ�ִ�
			zend_write(ZSTR_VAL(str), ZSTR_LEN(str));
		// ��������ǿգ�op1 �����Ǳ������ ���� ��
		} else if (OP1_TYPE == IS_CV && UNEXPECTED(Z_TYPE_P(z) == IS_UNDEF)) {
			// ����p1�ı�������������δ����, ����δ��ʼ��zval
			ZVAL_UNDEFINED_OP1();
		}
		// �ͷ�string
		zend_string_release_ex(str, 0);
	}

	// �ͷŲ�������ĸ��ӱ���
	FREE_OP1();
	// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}


// ing3, �� GLOBAL �� ��ǰ�������л�ȡ����
ZEND_VM_HELPER(zend_fetch_var_address_helper, CONST|TMPVAR|CV, UNUSED, int type)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *varname;
	zval *retval;
	zend_string *name, *tmp_name;
	HashTable *target_symbol_table;

	// windows: �޲���
	SAVE_OPLINE();
	// ��ȡzvalָ��, UNUSED ����null
	varname = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);

	// ���op1�ǳ���
	if (OP1_TYPE == IS_CONST) {
		// ������
		name = Z_STR_P(varname);
	// ��������� ���ִ�
	} else if (EXPECTED(Z_TYPE_P(varname) == IS_STRING)) {
		// ȡ�� �ִ�
		name = Z_STR_P(varname);
		// ��ʱ����Ϊ null
		tmp_name = NULL;
	// ����
	} else {
		// ��� op1 �� ������� ���� �������� δ����
		if (OP1_TYPE == IS_CV && UNEXPECTED(Z_TYPE_P(varname) == IS_UNDEF)) {
			// ����p1�ı�������������δ����, ����δ��ʼ��zval
			ZVAL_UNDEFINED_OP1();
		}
		// ��ȡ��ʱ�ִ������״�����
		name = zval_try_get_tmp_string(varname, &tmp_name);
		// ���û������
		if (UNEXPECTED(!name)) {
			// �����չֵ���� ZEND_FETCH_GLOBAL_LOCK
			if (!(opline->extended_value & ZEND_FETCH_GLOBAL_LOCK)) {
				// �ͷŲ�������ĸ��ӱ���
				FREE_OP1();
			}
			// ���Ϊ δ����
			ZVAL_UNDEF(EX_VAR(opline->result.var));
			// ��Ҫ�� return
			HANDLE_EXCEPTION();
		}
	}

	// �����ȡ�ķ��ű��� global ��ǰִ������ �������еı����б�
	// ȡ��Ŀ����ű�����ʱ���ű� �� ��ǰִ�������еķ��ű�
	target_symbol_table = zend_get_target_symbol_table(opline->extended_value EXECUTE_DATA_CC);
	// �ӷ��ű���ȡ���˱���
	retval = zend_hash_find_ex(target_symbol_table, name, OP1_TYPE == IS_CONST);
	// ���û���ҵ�
	if (retval == NULL) {
		// ��� �������� $this
		if (UNEXPECTED(zend_string_equals(name, ZSTR_KNOWN(ZEND_STR_THIS)))) {
// ������ת��ǩ fetch_this
ZEND_VM_C_LABEL(fetch_this):
			// ��д $this
			zend_fetch_this_var(type OPLINE_CC EXECUTE_DATA_CC);
			// ��� OP1 ���ǳ���
			if (OP1_TYPE != IS_CONST) {
				// �ͷ���ʱ����
				zend_tmp_string_release(tmp_name);
			}
			// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
			ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
		}
		// д�����
		if (type == BP_VAR_W) {
			// �ڷ��ű������һ��δ��ʼ��������name�Ǽ�
			retval = zend_hash_add_new(target_symbol_table, name, &EG(uninitialized_zval));
		// isset �� unset����
		} else if (type == BP_VAR_IS || type == BP_VAR_UNSET) {
			// ����Ҫ����
			// ���Ϊδ��ʼ������
			retval = &EG(uninitialized_zval);
		// ������������ �� BP_VAR_RW
		} else {
			// ��������δ����
			zend_error(E_WARNING, "Undefined %svariable $%s",
				(opline->extended_value & ZEND_FETCH_GLOBAL ? "global " : ""), ZSTR_VAL(name));
			// ���	������ BP_VAR_RW ����û���쳣
			if (type == BP_VAR_RW && !EG(exception)) {
				// ���ű��и��� name, ֵΪδ�������
				retval = zend_hash_update(target_symbol_table, name, &EG(uninitialized_zval));
			// ����
			} else {
				// ���Ϊδ��ʼ������
				retval = &EG(uninitialized_zval);
			}
		}
	// GLOBAL �� $$name �������Ե��� ָ���������� �������ָ��
	/* GLOBAL or $$name variable may be an INDIRECT pointer to CV */
	} else if (Z_TYPE_P(retval) == IS_INDIRECT) {
		// 
		retval = Z_INDIRECT_P(retval);
		// ������Ϊ δ����
		if (Z_TYPE_P(retval) == IS_UNDEF) {
			// ���������Ϊ $this
			if (UNEXPECTED(zend_string_equals(name, ZSTR_KNOWN(ZEND_STR_THIS)))) {
				// ��ת��ָ����ǩ fetch_this
				ZEND_VM_C_GOTO(fetch_this);
			}
			// �������Ϊ���� 
			if (type == BP_VAR_W) {
				// ���Ϊnull
				ZVAL_NULL(retval);
			// �����������Ϊ isset �� ɾ��
			} else if (type == BP_VAR_IS || type == BP_VAR_UNSET) {
				// ���Ϊδ��ʼ��zval
				retval = &EG(uninitialized_zval);
			// ����
			} else {
				// ����δ�������
				zend_error(E_WARNING, "Undefined %svariable $%s",
					(opline->extended_value & ZEND_FETCH_GLOBAL ? "global " : ""), ZSTR_VAL(name));
				// ����Ǹ��²��� ���� û���쳣
				if (type == BP_VAR_RW && !EG(exception)) {
					// ���Ϊnull
					ZVAL_NULL(retval);
				// ����
				} else {
					// ���Ϊδ��ʼ��zval
					retval = &EG(uninitialized_zval);
				}
			}
		}
	}

	// ���û�� ZEND_FETCH_GLOBAL_LOCK
	if (!(opline->extended_value & ZEND_FETCH_GLOBAL_LOCK)) {
		// �ͷŲ�������ĸ��ӱ���
		FREE_OP1();
	}

	// ��� OP1 ���ǳ���
	if (OP1_TYPE != IS_CONST) {
		// �ͷ���ʱ����
		zend_tmp_string_release(tmp_name);
	}

	// ����ֵ������Ч
	ZEND_ASSERT(retval != NULL);
	// ��������� ���� isset
	if (type == BP_VAR_R || type == BP_VAR_IS) {
		// ��������Ŀ�� �� ������Ϣ�����������ü���
		ZVAL_COPY_DEREF(EX_VAR(opline->result.var), retval);
	// ����
	} else {
		// ������ø�ֵ
		// �� zval.zv ���ָ��, ����ǳ� ������� ����
		ZVAL_INDIRECT(EX_VAR(opline->result.var), retval);
	}
	// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, ��ȡglobal��ǰ�����ģ�ִ�����ݣ��еı���
ZEND_VM_HANDLER(80, ZEND_FETCH_R, CONST|TMPVAR|CV, UNUSED, VAR_FETCH)
{
	// ���÷��� zend_fetch_var_address_helper
	ZEND_VM_DISPATCH_TO_HELPER(zend_fetch_var_address_helper, type, BP_VAR_R);
}

// ing3, ����global��ǰ�����ģ�ִ�����ݣ��еı���
ZEND_VM_HANDLER(83, ZEND_FETCH_W, CONST|TMPVAR|CV, UNUSED, VAR_FETCH)
{
	// ���÷��� zend_fetch_var_address_helper
	ZEND_VM_DISPATCH_TO_HELPER(zend_fetch_var_address_helper, type, BP_VAR_W);
}

// ing3, ����global��ǰ�����ģ�ִ�����ݣ��еı���
ZEND_VM_HANDLER(86, ZEND_FETCH_RW, CONST|TMPVAR|CV, UNUSED, VAR_FETCH)
{
	// ���÷��� zend_fetch_var_address_helper
	ZEND_VM_DISPATCH_TO_HELPER(zend_fetch_var_address_helper, type, BP_VAR_RW);
}

// ing3, ��ȡ ����������ִ�����ݣ��еı�����
ZEND_VM_HANDLER(92, ZEND_FETCH_FUNC_ARG, CONST|TMPVAR|CV, UNUSED, VAR_FETCH)
{
	// ������ô��ݣ��� BP_VAR_W ������ BP_VAR_R
	int fetch_type =
		(UNEXPECTED(ZEND_CALL_INFO(EX(call)) & ZEND_CALL_SEND_ARG_BY_REF)) ?
			BP_VAR_W : BP_VAR_R;
	// ���÷��� zend_fetch_var_address_helper
	ZEND_VM_DISPATCH_TO_HELPER(zend_fetch_var_address_helper, type, fetch_type);
}

// ing3, ɾ��global��ǰ�����ģ�ִ�����ݣ��еı���
ZEND_VM_HANDLER(95, ZEND_FETCH_UNSET, CONST|TMPVAR|CV, UNUSED, VAR_FETCH)
{
	// ���÷��� zend_fetch_var_address_helper
	ZEND_VM_DISPATCH_TO_HELPER(zend_fetch_var_address_helper, type, BP_VAR_UNSET);
}

// ing3, ��� global��ǰ�����ģ�ִ�����ݣ��еı���
ZEND_VM_HANDLER(89, ZEND_FETCH_IS, CONST|TMPVAR|CV, UNUSED, VAR_FETCH)
{
	// ���÷��� zend_fetch_var_address_helper
	ZEND_VM_DISPATCH_TO_HELPER(zend_fetch_var_address_helper, type, BP_VAR_IS);
}

/* No specialization for op_types (CONST|TMPVAR|CV, UNUSED|CONST|VAR) */

// ing3, ͨ����������չ��Ϣ�еĻ���λ�ã���ȡ��̬���Ե�ַ��
ZEND_VM_HELPER(zend_fetch_static_prop_helper, ANY, ANY, int type)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *prop;

	// windows: �޲���
	SAVE_OPLINE();

	// ���ؾ�̬���Ե�ַ��p1:��������ֵ��p2:����������Ϣ��p3:����λ�ã�p4:��ȡ���ͣ�p5:falgs
	if (UNEXPECTED(zend_fetch_static_property_address(&prop, NULL, opline->extended_value & ~ZEND_FETCH_OBJ_FLAGS, type, opline->extended_value OPLINE_CC EXECUTE_DATA_CC) != SUCCESS)) {
		// ���ʧ�ܡ�����Ҫ�� �쳣 �� ���������� BP_VAR_IS
		ZEND_ASSERT(EG(exception) || (type == BP_VAR_IS));
		// ��� Ϊδ��ʼ��zval
		prop = &EG(uninitialized_zval);
	}

	// �����������Ϊ BP_VAR_R �� BP_VAR_IS
	if (type == BP_VAR_R || type == BP_VAR_IS) {
		// ��������Ŀ�� �� ������Ϣ������������� ���������ü���
		ZVAL_COPY_DEREF(EX_VAR(opline->result.var), prop);
	// ����
	} else {
		// ������ø�ֵ�� �������
		// �� zval.zv ���ָ��, ����ǳ� ������� ����
		ZVAL_INDIRECT(EX_VAR(opline->result.var), prop);
	}
	// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, ��ȡ��̬����
/* No specialization for op_types (CONST|TMPVAR|CV, UNUSED|CLASS_FETCH|CONST|VAR) */
ZEND_VM_HANDLER(173, ZEND_FETCH_STATIC_PROP_R, ANY, CLASS_FETCH, CACHE_SLOT)
{
	// ͨ����������չ��Ϣ�еĻ���λ�ã���ȡ��̬���Ե�ַ��
	// ���÷��� zend_fetch_static_prop_helper
	ZEND_VM_DISPATCH_TO_HELPER(zend_fetch_static_prop_helper, type, BP_VAR_R);
}

// ing3, д�뾲̬����
/* No specialization for op_types (CONST|TMPVAR|CV, UNUSED|CLASS_FETCH|CONST|VAR) */
ZEND_VM_HANDLER(174, ZEND_FETCH_STATIC_PROP_W, ANY, CLASS_FETCH, FETCH_REF|DIM_WRITE|CACHE_SLOT)
{
	// ͨ����������չ��Ϣ�еĻ���λ�ã���ȡ��̬���Ե�ַ��
	// ���÷��� zend_fetch_static_prop_helper
	ZEND_VM_DISPATCH_TO_HELPER(zend_fetch_static_prop_helper, type, BP_VAR_W);
}

// ing3, ���¾�̬����
/* No specialization for op_types (CONST|TMPVAR|CV, UNUSED|CLASS_FETCH|CONST|VAR) */
ZEND_VM_HANDLER(175, ZEND_FETCH_STATIC_PROP_RW, ANY, CLASS_FETCH, CACHE_SLOT)
{
	// ͨ����������չ��Ϣ�еĻ���λ�ã���ȡ��̬���Ե�ַ��
	// ���÷��� zend_fetch_static_prop_helper
	ZEND_VM_DISPATCH_TO_HELPER(zend_fetch_static_prop_helper, type, BP_VAR_RW);
}


// ing3, ���ʾ�̬���ԣ��Զ�������������
/* No specialization for op_types (CONST|TMPVAR|CV, UNUSED|CLASS_FETCH|CONST|VAR) */
ZEND_VM_HANDLER(177, ZEND_FETCH_STATIC_PROP_FUNC_ARG, ANY, CLASS_FETCH, FETCH_REF|CACHE_SLOT)
{
	// ���Ҫ�����ô���
	int fetch_type =
		(UNEXPECTED(ZEND_CALL_INFO(EX(call)) & ZEND_CALL_SEND_ARG_BY_REF)) ?
			BP_VAR_W : BP_VAR_R;
	// ͨ����������չ��Ϣ�еĻ���λ�ã���ȡ��̬���Ե�ַ��
	// ���÷��� zend_fetch_static_prop_helper
	ZEND_VM_DISPATCH_TO_HELPER(zend_fetch_static_prop_helper, type, fetch_type);
}

// ing3, ɾ����̬����
/* No specialization for op_types (CONST|TMPVAR|CV, UNUSED|CLASS_FETCH|CONST|VAR) */
ZEND_VM_HANDLER(178, ZEND_FETCH_STATIC_PROP_UNSET, ANY, CLASS_FETCH, CACHE_SLOT)
{
	// ͨ����������չ��Ϣ�еĻ���λ�ã���ȡ��̬���Ե�ַ��
	// ���÷��� zend_fetch_static_prop_helper
	ZEND_VM_DISPATCH_TO_HELPER(zend_fetch_static_prop_helper, type, BP_VAR_UNSET);
}

// ing3, ��쾲̬���� �Ƿ����
/* No specialization for op_types (CONST|TMPVAR|CV, UNUSED|CLASS_FETCH|CONST|VAR) */
ZEND_VM_HANDLER(176, ZEND_FETCH_STATIC_PROP_IS, ANY, CLASS_FETCH, CACHE_SLOT)
{
	// ͨ����������չ��Ϣ�еĻ���λ�ã���ȡ��̬���Ե�ַ��
	// ���÷��� zend_fetch_static_prop_helper
	ZEND_VM_DISPATCH_TO_HELPER(zend_fetch_static_prop_helper, type, BP_VAR_IS);
}

// ing3,���������͵ģ�������һ����ȡ, ������ص��������У���������Ϊ BP_VAR_R��op1:�������������ͣ���op2:ƫ����
ZEND_VM_COLD_CONSTCONST_HANDLER(81, ZEND_FETCH_DIM_R, CONST|TMPVAR|CV, CONST|TMPVAR|CV)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *container, *dim, *value;

	// windows: �޲���
	SAVE_OPLINE();
	// ��ȡzvalָ��, UNUSED ����null
	container = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ��ȡzvalָ��, UNUSED ����null
	dim = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ��� OP1 ���ǳ���
	if (OP1_TYPE != IS_CONST) {
		// ��� ���� ������ ����
		if (EXPECTED(Z_TYPE_P(container) == IS_ARRAY)) {
// ������ת��ǩ fetch_dim_r_array
ZEND_VM_C_LABEL(fetch_dim_r_array):
			// ������һ����д�����ڲ�ʵ�֡�p1:��ϣ��p2:������p3:�����������ͣ�p4:��������
			value = zend_fetch_dimension_address_inner(Z_ARRVAL_P(container), dim, OP2_TYPE, BP_VAR_R EXECUTE_DATA_CC);
			// ��������Ŀ�� �� ������Ϣ�����������ü���
			ZVAL_COPY_DEREF(EX_VAR(opline->result.var), value);
		// ���� ��� ���� ������ ����
		} else if (EXPECTED(Z_TYPE_P(container) == IS_REFERENCE)) {
			// ������
			container = Z_REFVAL_P(container);
			// ��� ���� ������ ����
			if (EXPECTED(Z_TYPE_P(container) == IS_ARRAY)) {
				// ��ת��ָ����ǩ fetch_dim_r_array
				ZEND_VM_C_GOTO(fetch_dim_r_array);
			// ����
			} else {
				// ��ת��ָ����ǩ fetch_dim_r_slow
				ZEND_VM_C_GOTO(fetch_dim_r_slow);
			}
		// ����
		} else {
// ������ת��ǩ fetch_dim_r_slow
ZEND_VM_C_LABEL(fetch_dim_r_slow):
			// ���op2�ǳ��� ���� ��������Ϊ ZEND_EXTRA_VALUE
			if (OP2_TYPE == IS_CONST && Z_EXTRA_P(dim) == ZEND_EXTRA_VALUE) {
				// ȡ����һ�� zval
				dim++;
			}
			// ����ϣ������ģ�������һ����ȡ,������ص��������У���������Ϊ BP_VAR_R�� 
			zend_fetch_dimension_address_read_R_slow(container, dim OPLINE_CC EXECUTE_DATA_CC);
		}
	// ����
	} else {
		//���������͵ģ�������һ����ȡ,������ص��������У���������Ϊ BP_VAR_R��  p1:����ȡ������p2:������p3:��������
		zend_fetch_dimension_address_read_R(container, dim, OP2_TYPE OPLINE_CC EXECUTE_DATA_CC);
	}
	// �ͷŲ�������ĸ��ӱ���
	FREE_OP2();
	// �ͷŲ�������ĸ��ӱ���
	FREE_OP1();
	// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}


// ing3, ������һ����ȡ��ַ����������Ϊ BP_VAR_W��op1:�������������ͣ���op2:ƫ����
ZEND_VM_HANDLER(84, ZEND_FETCH_DIM_W, VAR|CV, CONST|TMPVAR|UNUSED|NEXT|CV)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *container;

	// windows: �޲���
	SAVE_OPLINE();
	// ��ȡzvalָ��, TMP/CONST/UNUSED ����null����֧�� TMPVAR/TMPVARCV
	container = GET_OP1_ZVAL_PTR_PTR_UNDEF(BP_VAR_W);
	// ������һ����ȡ��ַ����������Ϊ BP_VAR_W��p1������ֵ��p2:�������������ͣ���p3:����ֵ��p4:��������
	// ��ȡzvalָ��, UNUSED ����null
	zend_fetch_dimension_address_W(container, GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R), OP2_TYPE OPLINE_CC EXECUTE_DATA_CC);
	// �ͷŲ�������ĸ��ӱ���
	FREE_OP2();
	// ��� op1 �� ��ͨ����
	if (OP1_TYPE == IS_VAR) {
		// p1�е�����ʵ�� ������-1�����Ϊ0��Ŀ�������opline->result,����������� �� ���� p1��
		FREE_VAR_PTR_AND_EXTRACT_RESULT_IF_NECESSARY(opline->op1.var);
	}
	// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, ������һ����ȡ��ַ����������Ϊ BP_VAR_RW��op1:�������������ͣ���op2:ƫ����
ZEND_VM_HANDLER(87, ZEND_FETCH_DIM_RW, VAR|CV, CONST|TMPVAR|UNUSED|NEXT|CV)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *container;

	// windows: �޲���
	SAVE_OPLINE();
	// ��ȡzvalָ��, TMP/CONST/UNUSED ����null����֧�� TMPVAR/TMPVARCV
	container = GET_OP1_ZVAL_PTR_PTR_UNDEF(BP_VAR_RW);
	// ��ȡzvalָ��, UNUSED ����null
	zend_fetch_dimension_address_RW(container, GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R), OP2_TYPE OPLINE_CC EXECUTE_DATA_CC);
	// �ͷŲ�������ĸ��ӱ���
	FREE_OP2();
	// ��� op1 �� ��ͨ����
	if (OP1_TYPE == IS_VAR) {
		// p1�е�����ʵ�� ������-1�����Ϊ0��Ŀ�������opline->result,����������� �� ���� p1��
		FREE_VAR_PTR_AND_EXTRACT_RESULT_IF_NECESSARY(opline->op1.var);
	}
	// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}


// ing3,���������͵ģ�������һ����ȡ, ��������Ϊ BP_VAR_IS��op1:������op2:ƫ����
ZEND_VM_COLD_CONSTCONST_HANDLER(90, ZEND_FETCH_DIM_IS, CONST|TMPVAR|CV, CONST|TMPVAR|CV)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *container;

	// windows: �޲���
	SAVE_OPLINE();
	// ��ȡzvalָ��, UNUSED ����null
	container = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_IS);
	//���������͵ģ�������һ����ȡ,������ص��������У���������Ϊ BP_VAR_IS��  p1:����ȡ������p2:������p3:��������
	// ��ȡop2��zvalָ��, UNUSED ����null
	zend_fetch_dimension_address_read_IS(container, GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R), OP2_TYPE OPLINE_CC EXECUTE_DATA_CC);
	// �ͷŲ�������ĸ��ӱ���
	FREE_OP2();
	// �ͷŲ�������ĸ��ӱ���
	FREE_OP1();
	// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, ������������д����������ʹ����ʱ���ʽ
ZEND_VM_COLD_HELPER(zend_use_tmp_in_write_context_helper, ANY, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// windows: �޲���
	SAVE_OPLINE();
	// ������������д����������ʹ����ʱ���ʽ
	zend_throw_error(NULL, "Cannot use temporary expression in write context");
	// �ͷŲ�������ĸ��ӱ���
	FREE_OP2();
	// �ͷŲ�������ĸ��ӱ���
	FREE_OP1();
	// ���Ϊ δ����
	ZVAL_UNDEF(EX_VAR(opline->result.var));
	// ��Ҫ�� return
	HANDLE_EXCEPTION();
}

// ing3, ����������ʹ�ÿ������������ж�ȡ
ZEND_VM_COLD_HELPER(zend_use_undef_in_read_context_helper, ANY, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// windows: �޲���
	SAVE_OPLINE();
	zend_throw_error(NULL, "Cannot use [] for reading");
	// �ͷŲ�������ĸ��ӱ���
	FREE_OP2();
	// �ͷŲ�������ĸ��ӱ���
	FREE_OP1();
	// ���Ϊ δ����
	ZVAL_UNDEF(EX_VAR(opline->result.var));
	// ��Ҫ�� return
	HANDLE_EXCEPTION();
}

// ing3, �����Ҫ���ô��ݣ����� ZEND_FETCH_DIM_W �������� ZEND_FETCH_DIM_R
ZEND_VM_COLD_CONSTCONST_HANDLER(93, ZEND_FETCH_DIM_FUNC_ARG, CONST|TMP|VAR|CV, CONST|TMPVAR|UNUSED|NEXT|CV)
{
// op1��op2 ����ANY��
#if !ZEND_VM_SPEC
	// const zend_op *opline = EX(opline);
	USE_OPLINE
#endif

	// ��������ô��ݵĲ���
	if (UNEXPECTED(ZEND_CALL_INFO(EX(call)) & ZEND_CALL_SEND_ARG_BY_REF)) {
		// ��� op1�� ��������ʱ����
		if ((OP1_TYPE & (IS_CONST|IS_TMP_VAR))) {
			// ������������д����������ʹ����ʱ���ʽ
			// ���÷��� zend_use_tmp_in_write_context_helper
			ZEND_VM_DISPATCH_TO_HELPER(zend_use_tmp_in_write_context_helper);
		}
		// ʹ��д��ģʽ����
		// ��ת���������ǩ ZEND_FETCH_DIM_W
		ZEND_VM_DISPATCH_TO_HANDLER(ZEND_FETCH_DIM_W);
	// ����
	} else {
		// ��� op2 ������δ����
		if (OP2_TYPE == IS_UNUSED) {
			// ����������ʹ�ÿ������������ж�ȡ
			// ���÷��� zend_use_undef_in_read_context_helper
			ZEND_VM_DISPATCH_TO_HELPER(zend_use_undef_in_read_context_helper);
		}
		// ʹ�ö�ȡģʽ����
		// ��ת���������ǩ ZEND_FETCH_DIM_R
		ZEND_VM_DISPATCH_TO_HANDLER(ZEND_FETCH_DIM_R);
	}
}

// ing3, ������һ��ɾ��Ԫ��
ZEND_VM_HANDLER(96, ZEND_FETCH_DIM_UNSET, VAR|CV, CONST|TMPVAR|CV)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *container;

	// windows: �޲���
	SAVE_OPLINE();
	// ��ȡzvalָ��, TMP/CONST/UNUSED ����null����֧�� TMPVAR/TMPVARCV
	container = GET_OP1_ZVAL_PTR_PTR_UNDEF(BP_VAR_UNSET);
	
	// ������һ����ȡ��ַ����������Ϊ BP_VAR_UNSET��p1������ֵ��p2:�������������ͣ���p3:����ֵ��p4:��������
	// ��ȡzvalָ��, UNUSED ����null
	zend_fetch_dimension_address_UNSET(container, GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R), OP2_TYPE OPLINE_CC EXECUTE_DATA_CC);
	
	// �ͷŲ�������ĸ��ӱ���
	FREE_OP2();
	// ��� op1 �� ��ͨ����
	if (OP1_TYPE == IS_VAR) {
		// p1�е�����ʵ�� ������-1�����Ϊ0��Ŀ�������opline->result,����������� �� ���� p1��
		FREE_VAR_PTR_AND_EXTRACT_RESULT_IF_NECESSARY(opline->op1.var);
	}
	// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, ֻ����ʽ��ȡ��������
ZEND_VM_HOT_OBJ_HANDLER(82, ZEND_FETCH_OBJ_R, CONST|TMPVAR|UNUSED|THIS|CV, CONST|TMPVAR|CV, CACHE_SLOT)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *container;
	void **cache_slot = NULL;

	// windows: �޲���
	SAVE_OPLINE();
	// ��ȡop1 �� zvalָ��, UNUSED ���� $this
	container = GET_OP1_OBJ_ZVAL_PTR_UNDEF(BP_VAR_R);

	// ���op1 �ǳ��� �� ��op1��δ���� ���� �������Ƕ���
	if (OP1_TYPE == IS_CONST ||
	    (OP1_TYPE != IS_UNUSED && UNEXPECTED(Z_TYPE_P(container) != IS_OBJECT))) {
		// Ϊ��break;
		do {
			// ��� op1 ����ͨ������������ ���� op1����������
			if ((OP1_TYPE & (IS_VAR|IS_CV)) && Z_ISREF_P(container)) {
				// ������
				container = Z_REFVAL_P(container);
				// ��� ���� ������ ����
				if (EXPECTED(Z_TYPE_P(container) == IS_OBJECT)) {
					// ����
					break;
				}
			}
			
			// ��� op1 �� ������� ���� ����������δ����
			if (OP1_TYPE == IS_CV && UNEXPECTED(Z_TYPE_P(container) == IS_UNDEF)) {
				// ����p1�ı�������������δ����, ����δ��ʼ��zval
				ZVAL_UNDEFINED_OP1();
			}
			// ��ȡzvalָ�룬UNUSED ���ͷ���null
			zend_wrong_property_read(container, GET_OP2_ZVAL_PTR(BP_VAR_R));
			/ ���Ϊnull
			ZVAL_NULL(EX_VAR(opline->result.var));
			// ��ת��ָ����ǩ fetch_obj_r_finish
			ZEND_VM_C_GOTO(fetch_obj_r_finish);
		} while (0);
	}

	// ���������ȷ���ڶԶ�����в���
	/* here we are sure we are dealing with an object */
	
	// Ϊ��break����Ҫ�߼�������
	do {
		// ȡ������
		zend_object *zobj = Z_OBJ_P(container);
		zend_string *name, *tmp_name;
		zval *retval;

		// ���op2�ǳ���
		if (OP2_TYPE == IS_CONST) {
			// �ҵ�����λ��
			cache_slot = CACHE_ADDR(opline->extended_value & ~ZEND_FETCH_REF /* FUNC_ARG fetch may contain it */);

			// �������λ�ô�ŵ��ǵ�ǰ�����������
			// ��������ĵ�һ��Ԫ��					
			if (EXPECTED(zobj->ce == CACHED_PTR_EX(cache_slot))) {
				// ��������ĵ�һ��Ԫ��					
				uintptr_t prop_offset = (uintptr_t)CACHED_PTR_EX(cache_slot + 1);
				// ���飺���������Ч 
				if (EXPECTED(IS_VALID_PROPERTY_OFFSET(prop_offset))) {
					// ȡ������ֵ
					retval = OBJ_PROP(zobj, prop_offset);
					// ��� ����ֵ�� ��Ч
					if (EXPECTED(Z_TYPE_INFO_P(retval) != IS_UNDEF)) {
						// op1��op2 ����ANY�� �� (op1 ����ʱ���� �� ��ͨ���� )
						if (!ZEND_VM_SPEC || (OP1_TYPE & (IS_TMP_VAR|IS_VAR)) != 0) {
							// ��ת��ָ����ǩ fetch_obj_r_copy
							ZEND_VM_C_GOTO(fetch_obj_r_copy);
						// ����
						} else {
// ������ת��ǩ fetch_obj_r_fast_copy
ZEND_VM_C_LABEL(fetch_obj_r_fast_copy):
							// ��������Ŀ�� �� ������Ϣ�����������ü���
							ZVAL_COPY_DEREF(EX_VAR(opline->result.var), retval);
							// opline + 1����Ŀ������벢 return
							ZEND_VM_NEXT_OPCODE();
						}
					}
				// ������������Ա�
				} else if (EXPECTED(zobj->properties != NULL)) {
					// 
					// ��ȡzvalָ�룬UNUSED ���ͷ���null
					name = Z_STR_P(GET_OP2_ZVAL_PTR(BP_VAR_R));
					// ���飺��δ֪�Ķ�̬�����Ա�ţ��������
					if (!IS_UNKNOWN_DYNAMIC_PROPERTY_OFFSET(prop_offset)) {
						// ���� ��̬���Ժţ�ת�ɸ�����-2
						uintptr_t idx = ZEND_DECODE_DYN_PROP_OFFSET(prop_offset);
						// �������������Ч��Χ��
						if (EXPECTED(idx < zobj->properties->nNumUsed * sizeof(Bucket))) {
							// �ҵ����� bucket
							Bucket *p = (Bucket*)((char*)zobj->properties->arData + idx);
							// �����������ͬһ���ִ� �� ����ϣƥ�䣬����һ����
							if (EXPECTED(p->key == name) ||
							    (EXPECTED(p->h == ZSTR_H(name)) &&
							     EXPECTED(p->key != NULL) &&
							     EXPECTED(zend_string_equal_content(p->key, name)))) {
								// ���ֵ��Ϊ����ֵ
								retval = &p->val;
								// op1��op2 ����ANY��
								if (!ZEND_VM_SPEC || (OP1_TYPE & (IS_TMP_VAR|IS_VAR)) != 0) {
									// ��ת��ָ����ǩ fetch_obj_r_copy
									ZEND_VM_C_GOTO(fetch_obj_r_copy);
								// ����
								} else {
									// ��ת��ָ����ǩ fetch_obj_r_fast_copy
									ZEND_VM_C_GOTO(fetch_obj_r_fast_copy);
								}
							}
						}
						// ����λ�ô�š���̬���Ա�š����
						CACHE_PTR_EX(cache_slot + 1, (void*)ZEND_DYNAMIC_PROPERTY_OFFSET);
					}
					// �����Ա��������Ʋ���
					retval = zend_hash_find_known_hash(zobj->properties, name);
					// ����ҵ�����
					if (EXPECTED(retval)) {
						// ����ƫ����
						uintptr_t idx = (char*)retval - (char*)zobj->properties->arData;
						// ��ƫ�����浽������
						CACHE_PTR_EX(cache_slot + 1, (void*)ZEND_ENCODE_DYN_PROP_OFFSET(idx));
						// op1��op2 ����ANY��
						if (!ZEND_VM_SPEC || (OP1_TYPE & (IS_TMP_VAR|IS_VAR)) != 0) {
							// ��ת��ָ����ǩ fetch_obj_r_copy
							ZEND_VM_C_GOTO(fetch_obj_r_copy);
						// ����
						} else {
							// ��ת��ָ����ǩ fetch_obj_r_fast_copy
							ZEND_VM_C_GOTO(fetch_obj_r_fast_copy);
						}
					}
				}
			}
			// ��ȡzvalָ�룬UNUSED ���ͷ���null
			name = Z_STR_P(GET_OP2_ZVAL_PTR(BP_VAR_R));
		// ����
		} else {
			// ��ȡzvalָ�룬UNUSED ���ͷ���null
			name = zval_try_get_tmp_string(GET_OP2_ZVAL_PTR(BP_VAR_R), &tmp_name);
			// ���������Ч
			if (UNEXPECTED(!name)) {
				// ���Ϊ δ����
				ZVAL_UNDEF(EX_VAR(opline->result.var));
				// ����
				break;
			}
		}
// ������
#if ZEND_DEBUG
		/* For non-standard object handlers, verify a declared property type in debug builds.
		 * Fetch prop_info before calling read_property(), as it may deallocate the object. */
		zend_property_info *prop_info = NULL;
		
		// �����ȡ�������Ǳ�׼����
		if (zobj->handlers->read_property != zend_std_read_property) {
			prop_info = zend_get_property_info(zobj->ce, name, /* silent */ true);
		}
#endif
		// ��ȡ���ԡ�p1:����p2:��������p3:���ͣ�p4:����λ�ã�p5:����ֵ��������
		// zend_std_read_property
		retval = zobj->handlers->read_property(zobj, name, BP_VAR_R, cache_slot, EX_VAR(opline->result.var));
// ������
#if ZEND_DEBUG
		if (!EG(exception) && prop_info && prop_info != ZEND_WRONG_PROPERTY_INFO
				&& ZEND_TYPE_IS_SET(prop_info->type)) {
			ZVAL_OPT_DEREF(retval);
			zend_verify_property_type(prop_info, retval, /* strict */ true);
		}
#endif

		// ��� OP2 ���ǳ���
		if (OP2_TYPE != IS_CONST) {
			// �ͷ���ʱ����
			zend_tmp_string_release(tmp_name);
		}
		// �������ֵ �� �����������ͬһ��zval
		if (retval != EX_VAR(opline->result.var)) {
// ������ת��ǩ fetch_obj_r_copy
ZEND_VM_C_LABEL(fetch_obj_r_copy):
			// ��������Ŀ�� �� ������Ϣ�����������ü���
			ZVAL_COPY_DEREF(EX_VAR(opline->result.var), retval);
		} else if (UNEXPECTED(Z_ISREF_P(retval))) {
			// ������ã�p1������Ϊ1��p1��ֵΪδ���壺����p1���ô�����p1����Ŀ�긴�Ƹ�p1
			zend_unwrap_reference(retval);
		}
	} while (0);

// ������ת��ǩ fetch_obj_r_finish
ZEND_VM_C_LABEL(fetch_obj_r_finish):
	// �ͷŲ�������ĸ��ӱ���
	FREE_OP2();
	// �ͷŲ�������ĸ��ӱ���
	FREE_OP1();
	// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, ������������
ZEND_VM_HANDLER(85, ZEND_FETCH_OBJ_W, VAR|UNUSED|THIS|CV, CONST|TMPVAR|CV, FETCH_REF|DIM_WRITE|CACHE_SLOT)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *property, *container, *result;

	// windows: �޲���
	SAVE_OPLINE();

	// ��ȡzvalָ��, TMP/CONST����null, UNUSED ���� $this����֧�� TMPVAR/TMPVARCV��CV ֱ�ӷ��ء�
	container = GET_OP1_OBJ_ZVAL_PTR_PTR_UNDEF(BP_VAR_W);
	// ��ȡzvalָ�룬UNUSED ���ͷ���null
	property = GET_OP2_ZVAL_PTR(BP_VAR_R);
	// ��ִ��������ȡ�� ָ����ŵı���
	result = EX_VAR(opline->result.var);
	// ��ȡ���Եĵ�ַ��p1:���ؽ����p2:����zval��p3:��������������ͣ�p4:����ֵ��p5:���������������
	// p6:����λ�ã�p7:�������ͣ�p8:flags��p9:�Ƿ��ʼ��δ��������
	zend_fetch_property_address(
		result, container, OP1_TYPE, property, OP2_TYPE,
		((OP2_TYPE == IS_CONST) ? CACHE_ADDR(opline->extended_value & ~ZEND_FETCH_OBJ_FLAGS) : NULL),
		BP_VAR_W, opline->extended_value, 1 OPLINE_CC EXECUTE_DATA_CC);
	// �ͷŲ�������ĸ��ӱ���
	FREE_OP2();
	// ��� op1 �� ��ͨ����
	if (OP1_TYPE == IS_VAR) {
		// p1�е�����ʵ�� ������-1�����Ϊ0��Ŀ�������opline->result,����������� �� ���� p1��
		FREE_VAR_PTR_AND_EXTRACT_RESULT_IF_NECESSARY(opline->op1.var);
	}
	// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, ���¶�������
ZEND_VM_HANDLER(88, ZEND_FETCH_OBJ_RW, VAR|UNUSED|THIS|CV, CONST|TMPVAR|CV, CACHE_SLOT)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *property, *container, *result;

	// windows: �޲���
	SAVE_OPLINE();
	// ��ȡzvalָ��, TMP/CONST����null, UNUSED ���� $this����֧�� TMPVAR/TMPVARCV��CV ֱ�ӷ��ء�
	container = GET_OP1_OBJ_ZVAL_PTR_PTR_UNDEF(BP_VAR_RW);
	// ��ȡzvalָ�룬UNUSED ���ͷ���null
	property = GET_OP2_ZVAL_PTR(BP_VAR_R);
	// ��ִ��������ȡ�� ָ����ŵı���
	result = EX_VAR(opline->result.var);
	// ��ȡ���Եĵ�ַ��p1:���ؽ����p2:����zval��p3:��������������ͣ�p4:����ֵ��p5:���������������
	// p6:����λ�ã�p7:�������ͣ�p8:flags��p9:�Ƿ��ʼ��δ��������
	zend_fetch_property_address(result, container, OP1_TYPE, property, OP2_TYPE, ((OP2_TYPE == IS_CONST) ? CACHE_ADDR(opline->extended_value) : NULL), BP_VAR_RW, 0, 1 OPLINE_CC EXECUTE_DATA_CC);
	// �ͷŲ�������ĸ��ӱ���
	FREE_OP2();
	// ��� op1 �� ��ͨ����
	if (OP1_TYPE == IS_VAR) {
		// p1�е�����ʵ�� ������-1�����Ϊ0��Ŀ�������opline->result,����������� �� ���� p1��
		FREE_VAR_PTR_AND_EXTRACT_RESULT_IF_NECESSARY(opline->op1.var);
	}
	// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, �����������Ƿ����
ZEND_VM_COLD_CONST_HANDLER(91, ZEND_FETCH_OBJ_IS, CONST|TMPVAR|UNUSED|THIS|CV, CONST|TMPVAR|CV, CACHE_SLOT)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *container;
	void **cache_slot = NULL;

	// windows: �޲���
	SAVE_OPLINE();
	// ��ȡzvalָ��, UNUSED ���� $this����֧�� TMPVARCV
	container = GET_OP1_OBJ_ZVAL_PTR(BP_VAR_IS);
	// ���op1�ǳ��� �� ��op2��Ч ���� �������Ƕ��� ��
	if (OP1_TYPE == IS_CONST ||
	    (OP1_TYPE != IS_UNUSED && UNEXPECTED(Z_TYPE_P(container) != IS_OBJECT))) {
		do {
			// ��� op1 ����ͨ������������ ���� op1����������
			if ((OP1_TYPE & (IS_VAR|IS_CV)) && Z_ISREF_P(container)) {
				// ������
				container = Z_REFVAL_P(container);
				// ��� ���� ������ ����
				if (EXPECTED(Z_TYPE_P(container) == IS_OBJECT)) {
					break;
				}
			}
			// ��� op2 �� ������� ���� zval����Ϊ δ����
			if (OP2_TYPE == IS_CV && Z_TYPE_P(EX_VAR(opline->op2.var)) == IS_UNDEF) {
				// ����p2�ı�������������δ����, ����δ��ʼ��zval
				ZVAL_UNDEFINED_OP2();
			}
			// ���Ϊnull
			ZVAL_NULL(EX_VAR(opline->result.var));
			// ��ת��ָ����ǩ fetch_obj_is_finish
			ZEND_VM_C_GOTO(fetch_obj_is_finish);
		} while (0);
	}

	// �������ȷ���ڲ�������
	/* here we are sure we are dealing with an object */
	do {
		// ���ڶ���
		zend_object *zobj = Z_OBJ_P(container);
		zend_string *name, *tmp_name;
		zval *retval;

		// ����������ǳ���
		if (OP2_TYPE == IS_CONST) {
			cache_slot = CACHE_ADDR(opline->extended_value);
			// ��������ĵ�һ��Ԫ��
			if (EXPECTED(zobj->ce == CACHED_PTR_EX(cache_slot))) {
				// ��������ĵ�һ��Ԫ��
				uintptr_t prop_offset = (uintptr_t)CACHED_PTR_EX(cache_slot + 1);
				// ���飺���������Ч 
				if (EXPECTED(IS_VALID_PROPERTY_OFFSET(prop_offset))) {
					// ȡ������ֵ
					retval = OBJ_PROP(zobj, prop_offset);
					// ���ֵ����δ����
					if (EXPECTED(Z_TYPE_P(retval) != IS_UNDEF)) {
						// op1��op2 ����ANY��
						if (!ZEND_VM_SPEC || (OP1_TYPE & (IS_TMP_VAR|IS_VAR)) != 0) {
							// ��ת��ָ����ǩ fetch_obj_is_copy
							ZEND_VM_C_GOTO(fetch_obj_is_copy);
						// ����	
						} else {
// ������ת��ǩ fetch_obj_is_fast_copy
ZEND_VM_C_LABEL(fetch_obj_is_fast_copy):
							// ��������Ŀ�� �� ������Ϣ�����������ü���
							ZVAL_COPY_DEREF(EX_VAR(opline->result.var), retval);
							// opline + 1����Ŀ������벢 return
							ZEND_VM_NEXT_OPCODE();
						}
					}
				// ��������Ա�
				} else if (EXPECTED(zobj->properties != NULL)) {
					// ��ȡzvalָ�룬UNUSED ���ͷ���null
					name = Z_STR_P(GET_OP2_ZVAL_PTR(BP_VAR_R));
					// ���飺��δ֪�Ķ�̬�����Ա�ţ��������
					if (!IS_UNKNOWN_DYNAMIC_PROPERTY_OFFSET(prop_offset)) {
						// ���� ��̬���Ժţ�ת�ɸ�����-2
						uintptr_t idx = ZEND_DECODE_DYN_PROP_OFFSET(prop_offset);
						// ���ƫ��������Чƥ��
						if (EXPECTED(idx < zobj->properties->nNumUsed * sizeof(Bucket))) {
							Bucket *p = (Bucket*)((char*)zobj->properties->arData + idx);
							// ����ͼ���ָ����ͬ �� ����ϣֵ��ͬ ���� keyƥ�䣩
							if (EXPECTED(p->key == name) ||
							    (EXPECTED(p->h == ZSTR_H(name)) &&
							     EXPECTED(p->key != NULL) &&
							     EXPECTED(zend_string_equal_content(p->key, name)))) {
								// ʹ������ֵ������ֵ
								retval = &p->val;
								// op1��op2 ����ANY
								if (!ZEND_VM_SPEC || (OP1_TYPE & (IS_TMP_VAR|IS_VAR)) != 0) {
									// ��ת��ָ����ǩ fetch_obj_is_copy
									ZEND_VM_C_GOTO(fetch_obj_is_copy);
								// ����
								} else {
									// ��ת��ָ����ǩ fetch_obj_is_fast_copy
									ZEND_VM_C_GOTO(fetch_obj_is_fast_copy);
								}
							}
						}
						// ����λ�ô�š���̬���Ա�š����
						CACHE_PTR_EX(cache_slot + 1, (void*)ZEND_DYNAMIC_PROPERTY_OFFSET);
					}
					// �����Ʋ��� ����
					retval = zend_hash_find_known_hash(zobj->properties, name);
					// ����ҵ�����
					if (EXPECTED(retval)) {
						// ȡ�������Ե�ƫ����
						uintptr_t idx = (char*)retval - (char*)zobj->properties->arData;
						// ��ƫ������¼��������
						CACHE_PTR_EX(cache_slot + 1, (void*)ZEND_ENCODE_DYN_PROP_OFFSET(idx));
						// op1��op2 ����ANY
						if (!ZEND_VM_SPEC || (OP1_TYPE & (IS_TMP_VAR|IS_VAR)) != 0) {
							// ��ת��ָ����ǩ fetch_obj_is_copy
							ZEND_VM_C_GOTO(fetch_obj_is_copy);
						// ����
						} else {
							// ��ת��ָ����ǩ fetch_obj_is_fast_copy
							ZEND_VM_C_GOTO(fetch_obj_is_fast_copy);
						}
					}
				}
			}
			// ��ȡzvalָ�룬UNUSED ���ͷ���null
			name = Z_STR_P(GET_OP2_ZVAL_PTR(BP_VAR_R));
		// �������������ǳ���
		} else {
			// ��ȡzvalָ�룬UNUSED ���ͷ���null
			name = zval_try_get_tmp_string(GET_OP2_ZVAL_PTR(BP_VAR_R), &tmp_name);
			// �����ȡ������ʧ��
			if (UNEXPECTED(!name)) {
				// ���Ϊ δ����
				ZVAL_UNDEF(EX_VAR(opline->result.var));
				// ����
				break;
			}
		}

		// ��ȡ���ԡ�p1:����p2:��������p3:���ͣ�p4:����λ�ã�p5:����ֵ��������
		// zend_std_read_property
		retval = zobj->handlers->read_property(zobj, name, BP_VAR_IS, cache_slot, EX_VAR(opline->result.var));

		// ��� OP2 ���ǳ���
		if (OP2_TYPE != IS_CONST) {
			// �ͷ���ʱ����
			zend_tmp_string_release(tmp_name);
		}

		// ������ ���ֵ���Ǵ˱���
		if (retval != EX_VAR(opline->result.var)) {
// ������ת��ǩ fetch_obj_is_copy
ZEND_VM_C_LABEL(fetch_obj_is_copy):
			// ��������Ŀ�� �� ������Ϣ�����������ü���
			ZVAL_COPY_DEREF(EX_VAR(opline->result.var), retval);
		// ���ֵ���Ǵ˱���������������
		} else if (UNEXPECTED(Z_ISREF_P(retval))) {
			// ������ã�p1������Ϊ1��p1��ֵΪδ���壺����p1���ô�����p1����Ŀ�긴�Ƹ�p1
			zend_unwrap_reference(retval);
		}
	} while (0);

// ������ת��ǩ fetch_obj_is_finish
ZEND_VM_C_LABEL(fetch_obj_is_finish):
	// �ͷŲ�������ĸ��ӱ���
	FREE_OP2();
	// �ͷŲ�������ĸ��ӱ���
	FREE_OP1();
	// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, ��Ҫ���ô��� ���� ZEND_FETCH_OBJ_W �������� ZEND_FETCH_OBJ_R
ZEND_VM_COLD_CONST_HANDLER(94, ZEND_FETCH_OBJ_FUNC_ARG, CONST|TMP|VAR|UNUSED|THIS|CV, CONST|TMPVAR|CV, FETCH_REF|CACHE_SLOT)
{
// op1��op2 ����ANY��
#if !ZEND_VM_SPEC
	// const zend_op *opline = EX(opline);
	USE_OPLINE
#endif

	// ���Ҫ�����ô��ݲ���
	if (UNEXPECTED(ZEND_CALL_INFO(EX(call)) & ZEND_CALL_SEND_ARG_BY_REF)) {
		// ���� FETCH_OBJ_W
		/* Behave like FETCH_OBJ_W */
		// ���op1�ǳ��� �� ��ʱ����
		if ((OP1_TYPE & (IS_CONST|IS_TMP_VAR))) {
			// ������������д����������ʹ����ʱ���ʽ
			// ���÷��� zend_use_tmp_in_write_context_helper
			ZEND_VM_DISPATCH_TO_HELPER(zend_use_tmp_in_write_context_helper);
		}
		// ��ת���������ǩ ZEND_FETCH_OBJ_W
		ZEND_VM_DISPATCH_TO_HANDLER(ZEND_FETCH_OBJ_W);
	// ����
	} else {
		// ��ת���������ǩ ZEND_FETCH_OBJ_R
		ZEND_VM_DISPATCH_TO_HANDLER(ZEND_FETCH_OBJ_R);
	}
}

// ing3, ɾ����������
ZEND_VM_HANDLER(97, ZEND_FETCH_OBJ_UNSET, VAR|UNUSED|THIS|CV, CONST|TMPVAR|CV, CACHE_SLOT)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *container, *property, *result;

	// windows: �޲���
	SAVE_OPLINE();
	// ��ȡzvalָ��, TMP/CONST����null, UNUSED ���� $this����֧�� TMPVAR/TMPVARCV��CV ֱ�ӷ��ء�
	container = GET_OP1_OBJ_ZVAL_PTR_PTR_UNDEF(BP_VAR_UNSET);
	// ��ȡzvalָ�룬UNUSED ���ͷ���null
	property = GET_OP2_ZVAL_PTR(BP_VAR_R);
	// ��ִ��������ȡ�� ָ����ŵı���
	result = EX_VAR(opline->result.var);
	// ��ȡ���Եĵ�ַ��p1:���ؽ����p2:����zval��p3:��������������ͣ�p4:����ֵ��p5:���������������
	// p6:����λ�ã�p7:�������ͣ�p8:flags��p9:�Ƿ��ʼ��δ��������
	zend_fetch_property_address(result, container, OP1_TYPE, property, OP2_TYPE, ((OP2_TYPE == IS_CONST) ? CACHE_ADDR(opline->extended_value) : NULL), BP_VAR_UNSET, 0, 1 OPLINE_CC EXECUTE_DATA_CC);
	// �ͷŲ�������ĸ��ӱ���
	FREE_OP2();
	// ��� op1 �� ��ͨ����
	if (OP1_TYPE == IS_VAR) {
		// p1�е�����ʵ�� ������-1�����Ϊ0��Ŀ�������opline->result,����������� �� ���� p1��
		FREE_VAR_PTR_AND_EXTRACT_RESULT_IF_NECESSARY(opline->op1.var);
	}
	// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3,���ִ��������͵ģ�������һ����ȡ
ZEND_VM_HANDLER(98, ZEND_FETCH_LIST_R, CONST|TMPVARCV, CONST|TMPVAR|CV)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *container;

	// windows: �޲���
	SAVE_OPLINE();
	// ��ȡzvalָ��, UNUSED ����null
	container = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ���ִ��������͵ģ�������һ����ȡ,������ص��������У���������Ϊ BP_VAR_R��  p1:����ȡ������p2:������p3:��������
	// ��ȡzvalָ��, UNUSED ����null
	zend_fetch_dimension_address_LIST_r(container, GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R), OP2_TYPE OPLINE_CC EXECUTE_DATA_CC);
	// �ͷŲ�������ĸ��ӱ���
	FREE_OP2();
	// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, ��ά�ȷ���д��
ZEND_VM_HANDLER(155, ZEND_FETCH_LIST_W, VAR, CONST|TMPVAR|CV)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *container, *dim;

	// windows: �޲���
	SAVE_OPLINE();
	// ��ȡzvalָ��, TMP/CONST/UNUSED ����null����֧�� TMPVAR/TMPVARCV
	container = GET_OP1_ZVAL_PTR_PTR_UNDEF(BP_VAR_W);
	// ��ȡzvalָ��, UNUSED ����null
	dim = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);

	// ���op1�Ǳ��� ���� op1������������ ���� container������������
	if (OP1_TYPE == IS_VAR
		&& Z_TYPE_P(EX_VAR(opline->op1.var)) != IS_INDIRECT
		&& UNEXPECTED(!Z_ISREF_P(container))
	) {
		// ���������������ֵ�� ���� ����ʵ��
		zend_error(E_NOTICE, "Attempting to set reference to non referenceable value");
		// ���ִ��������͵ģ�������һ����ȡ,������ص��������У���������Ϊ BP_VAR_R�� 
		// p1:����ȡ������p2:������p3:��������
		zend_fetch_dimension_address_LIST_r(container, dim, OP2_TYPE OPLINE_CC EXECUTE_DATA_CC);
	// ����
	} else {
		// ������һ����ȡ��ַ����������Ϊ BP_VAR_W��p1������ֵ��p2:�������������ͣ���p3:����ֵ��p4:��������
		zend_fetch_dimension_address_W(container, dim, OP2_TYPE OPLINE_CC EXECUTE_DATA_CC);
	}

	// �ͷŲ�������ĸ��ӱ���
	FREE_OP2();
	// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing2, �������Ը�ֵ
ZEND_VM_HANDLER(24, ZEND_ASSIGN_OBJ, VAR|UNUSED|THIS|CV, CONST|TMPVAR|CV, CACHE_SLOT, SPEC(OP_DATA=CONST|TMP|VAR|CV))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *object, *value, tmp;
	zend_object *zobj;
	zend_string *name, *tmp_name;

	// windows: �޲���
	SAVE_OPLINE();
	// ��ȡzvalָ��, TMP/CONST����null, UNUSED ���� $this����֧�� TMPVAR/TMPVARCV��CV ֱ�ӷ��ء�
	object = GET_OP1_OBJ_ZVAL_PTR_PTR_UNDEF(BP_VAR_W);
	// ȡ����һ��������� zval��UNUSED����null, ��֧�� TMPVARCV��
	value = GET_OP_DATA_ZVAL_PTR(BP_VAR_R);

	// ���op1��Ч ���� op1���Ͳ��Ƕ���
	if (OP1_TYPE != IS_UNUSED && UNEXPECTED(Z_TYPE_P(object) != IS_OBJECT)) {
		// ����������������� ���� ����Ŀ���Ƕ���
		if (Z_ISREF_P(object) && Z_TYPE_P(Z_REFVAL_P(object)) == IS_OBJECT) {
			// ������
			object = Z_REFVAL_P(object);
			// ��ת��ָ����ǩ assign_object
			ZEND_VM_C_GOTO(assign_object);
		}
		// ��ȡzvalָ�룬UNUSED ���ͷ���null
		zend_throw_non_object_error(object, GET_OP2_ZVAL_PTR(BP_VAR_R) OPLINE_CC EXECUTE_DATA_CC);
		// δ��ʼ��zval
		value = &EG(uninitialized_zval);
		// ��ת��ָ����ǩ free_and_exit_assign_obj
		ZEND_VM_C_GOTO(free_and_exit_assign_obj);
	}

// ������ת��ǩ assign_object
ZEND_VM_C_LABEL(assign_object):
	// ȡ������
	zobj = Z_OBJ_P(object);
	// ���op2�ǳ���
	if (OP2_TYPE == IS_CONST) {
		// ͨ��ƫ���� ������ʱ�����л�ȡһ�� (void**) �еĵ�һ��Ԫ��
		if (EXPECTED(zobj->ce == CACHED_PTR(opline->extended_value))) {
			// ȡ�û���λ��
			void **cache_slot = CACHE_ADDR(opline->extended_value);
			// ��������ĵ�һ��Ԫ��
			uintptr_t prop_offset = (uintptr_t)CACHED_PTR_EX(cache_slot + 1);
			// ȡ�� ����
			zend_object *zobj = Z_OBJ_P(object);
			zval *property_val;

			// ���飺���������Ч 
			if (EXPECTED(IS_VALID_PROPERTY_OFFSET(prop_offset))) {
				// ȡ������
				property_val = OBJ_PROP(zobj, prop_offset);
				// ���ֵ��Ч
				if (Z_TYPE_P(property_val) != IS_UNDEF) {
					// ��������ĵ�һ��Ԫ��
					zend_property_info *prop_info = (zend_property_info*) CACHED_PTR_EX(cache_slot + 2);

					// ���������Ϣ��Ч
					if (UNEXPECTED(prop_info != NULL)) {
						// �������͵����� ��ֵ��p1:������Ϣ��p2:����ֵzval��p3:��ֵzval
						value = zend_assign_to_typed_prop(prop_info, property_val, value EXECUTE_DATA_CC);
						// ��ת��ָ����ǩ free_and_exit_assign_obj
						ZEND_VM_C_GOTO(free_and_exit_assign_obj);
					// ����
					} else {
// ������ת��ǩ fast_assign_obj
ZEND_VM_C_LABEL(fast_assign_obj):
						// ��������ֵ��p1:������p2:ֵ��p3:ֵ�ı������ͣ������������������������p4:�Ƿ��ϸ�����
						// ĳִ���������������� �Ĳ����Ƿ�ʹ���ϸ���������
						value = zend_assign_to_variable(property_val, value, OP_DATA_TYPE, EX_USES_STRICT_TYPES());
						// ��� ���������������Ч(����IS_UNUSED)
						if (UNEXPECTED(RETURN_VALUE_USED(opline))) {
							// ֵ ���Ƹ� ��� 
							ZVAL_COPY(EX_VAR(opline->result.var), value);
						}
						// ��ת��ָ����ǩ exit_assign_obj
						ZEND_VM_C_GOTO(exit_assign_obj);
					}
				}
			// �������������Ч
			} else {
				// ��ȡzvalָ�룬UNUSED ���ͷ���null
				name = Z_STR_P(GET_OP2_ZVAL_PTR(BP_VAR_R));
				// ��������Ա�
				if (EXPECTED(zobj->properties != NULL)) {
					// ������Ա�������>1
					if (UNEXPECTED(GC_REFCOUNT(zobj->properties) > 1)) {
						// ������Ա��� ���ɸ���
						if (EXPECTED(!(GC_FLAGS(zobj->properties) & IS_ARRAY_IMMUTABLE))) {
							// ���ô���-1
							GC_DELREF(zobj->properties);
						}
						// ���Ա���������ʹ�ø���
						zobj->properties = zend_array_dup(zobj->properties);
					}
					// ȡ����֪���ֵ�����
					property_val = zend_hash_find_known_hash(zobj->properties, name);
					// ���������ֵ
					if (property_val) {
						// ��ת��ָ����ǩ fast_assign_obj
						ZEND_VM_C_GOTO(fast_assign_obj);
					}
				}

				// ���û�� __set �������� �С�����̬���ԡ����
				if (!zobj->ce->__set && (zobj->ce->ce_flags & ZEND_ACC_ALLOW_DYNAMIC_PROPERTIES)) {
					// ���û�����Ա�
					if (EXPECTED(zobj->properties == NULL)) {
						// �ؽ����Ա�
						rebuild_object_properties(zobj);
					}
					// ������������ǳ���
					if (OP_DATA_TYPE == IS_CONST) {
						// ����ǿ��Լ�������
						if (UNEXPECTED(Z_OPT_REFCOUNTED_P(value))) {
							// ���ô���+1
							Z_ADDREF_P(value);
						}
					// ����������ݲ�����ʱ����
					} else if (OP_DATA_TYPE != IS_TMP_VAR) {
						// ���ֵ����������
						if (Z_ISREF_P(value)) {
							// ������������� ����
							if (OP_DATA_TYPE == IS_VAR) {
								// ȡ������ʵ��
								zend_reference *ref = Z_REF_P(value);
								// ���ô���-1�����Ϊ0
								if (GC_DELREF(ref) == 0) {
									// value����Ŀ�긴�Ƹ� ��ʱ����
									ZVAL_COPY_VALUE(&tmp, Z_REFVAL_P(value));
									// �ͷ�����ʵ��
									efree_size(ref, sizeof(zend_reference));
									// ֵΪ��ʱ����
									value = &tmp;
								// ����
								} else {
									// ������
									value = Z_REFVAL_P(value);
									// �������ô���
									Z_TRY_ADDREF_P(value);
								}
							// ����
							} else {
								// ������
								value = Z_REFVAL_P(value);
								// �������ô���
								Z_TRY_ADDREF_P(value);
							}
						// ������������� �������
						} else if (OP_DATA_TYPE == IS_CV) {
							// �������ô���
							Z_TRY_ADDREF_P(value);
						}
					// ����ԭ������������ ��Ӧ����� if (Z_ISREF_P(value)) {
						}
					
					//
					//	��ϣ���������Ԫ��
					zend_hash_add_new(zobj->properties, name, value);
					// ��� ���������������Ч(����IS_UNUSED)
					if (UNEXPECTED(RETURN_VALUE_USED(opline))) {
						// ֵ���Ƶ������
						ZVAL_COPY(EX_VAR(opline->result.var), value);
					}
					// ��ת��ָ����ǩ exit_assign_obj
					ZEND_VM_C_GOTO(exit_assign_obj);
				}
			}
		}
		// ��ȡzvalָ�룬UNUSED ���ͷ���null
		name = Z_STR_P(GET_OP2_ZVAL_PTR(BP_VAR_R));
	// ����
	} else {
		// ��ȡzvalָ�룬UNUSED ���ͷ���null
		name = zval_try_get_tmp_string(GET_OP2_ZVAL_PTR(BP_VAR_R), &tmp_name);
		// ���û��ȡ��
		if (UNEXPECTED(!name)) {
			// �ͷ���һ��������� op1��CONST/UNUSED/CV ���������֧�� TMPVARCV��
			FREE_OP_DATA();
			// ��� opline ��������� ��������ʱ�������ѽ�� zval ���Ϊδ����
			UNDEF_RESULT();
			// ��ת��ָ����ǩ exit_assign_obj
			ZEND_VM_C_GOTO(exit_assign_obj);
		}
	}

	// ��������� ��������� ����
	if (OP_DATA_TYPE == IS_CV || OP_DATA_TYPE == IS_VAR) {
		// �������ô���
		ZVAL_DEREF(value);
	}
	
	// д�����ԣ�p1:����p2:��������p3:����ֵ��p4:����λ��
	// zend_std_write_property 
	value = zobj->handlers->write_property(zobj, name, value, (OP2_TYPE == IS_CONST) ? CACHE_ADDR(opline->extended_value) : NULL);

	// ��� OP2 ���ǳ���
	if (OP2_TYPE != IS_CONST) {
		// �ͷ���ʱ����
		zend_tmp_string_release(tmp_name);
	}

// ������ת��ǩ free_and_exit_assign_obj
ZEND_VM_C_LABEL(free_and_exit_assign_obj):
	// ��� ���������������Ч(����IS_UNUSED)
	if (UNEXPECTED(RETURN_VALUE_USED(opline))) {
		// ��������Ŀ�� �� ������Ϣ�����������ü���
		ZVAL_COPY_DEREF(EX_VAR(opline->result.var), value);
	}
	// �ͷ���һ��������� op1��CONST/UNUSED/CV ���������֧�� TMPVARCV��
	FREE_OP_DATA();
// ������ת��ǩ exit_assign_obj
ZEND_VM_C_LABEL(exit_assign_obj):
	// �ͷŲ�������ĸ��ӱ���
	FREE_OP2();
	// �ͷŲ�������ĸ��ӱ���
	FREE_OP1();
	
	// assign_obj ������������
	/* assign_obj has two opcodes! */
	// ƫ�Ƶ�Ŀ������벢 return ,p1:�Ƿ����쳣��ʹ�� EX(opline)  ��ʹ�� opline ��p2:ƫ����
	ZEND_VM_NEXT_OPCODE_EX(1, 2);
}

// ing3, ����̬���Ը�ֵ
/* No specialization for op_types (CONST|TMPVAR|CV, UNUSED|CONST|VAR) */
ZEND_VM_HANDLER(25, ZEND_ASSIGN_STATIC_PROP, ANY, ANY, CACHE_SLOT, SPEC(OP_DATA=CONST|TMP|VAR|CV))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *prop, *value;
	zend_property_info *prop_info;

	// windows: �޲���
	SAVE_OPLINE();

	// ͨ������λ�ú����ͣ����ؾ�̬���Ե�ַ��p1:��������ֵ��p2:����������Ϣ��p3:����λ�ã�p4:��ȡ���ͣ�p5:falgs
	if (zend_fetch_static_property_address(&prop, &prop_info, opline->extended_value, BP_VAR_W, 0 OPLINE_CC EXECUTE_DATA_CC) != SUCCESS) {
		// �ͷ���һ��������� op1��CONST/UNUSED/CV ���������֧�� TMPVARCV��
		FREE_OP_DATA();
		// ��� opline ��������� ��������ʱ�������ѽ�� zval ���Ϊδ����
		UNDEF_RESULT();
		// ��Ҫ�� return
		HANDLE_EXCEPTION();
	}
	// ȡ����һ��������� zval��UNUSED����null, ��֧�� TMPVARCV��
	value = GET_OP_DATA_ZVAL_PTR(BP_VAR_R);

	// ������ʹ���
	if (UNEXPECTED(ZEND_TYPE_IS_SET(prop_info->type))) {
		// �������͵����� ��ֵ��p1:������Ϣ��p2:����ֵzval��p3:��ֵzval
		value = zend_assign_to_typed_prop(prop_info, prop, value EXECUTE_DATA_CC);
		// �ͷ���һ��������� op1��CONST/UNUSED/CV ���������֧�� TMPVARCV��
		FREE_OP_DATA();
	// ����
	} else {
		// ��������ֵ��p1:������p2:ֵ��p3:ֵ�ı������ͣ������������������������p4:�Ƿ��ϸ�����
		// ĳִ���������������� �Ĳ����Ƿ�ʹ���ϸ���������
		value = zend_assign_to_variable(prop, value, OP_DATA_TYPE, EX_USES_STRICT_TYPES());
	}

	// ��� ���������������Ч(����IS_UNUSED)
	if (UNEXPECTED(RETURN_VALUE_USED(opline))) {
		// ֵ���Ƹ����
		ZVAL_COPY(EX_VAR(opline->result.var), value);
	}

	// ��̬���Ը�ֵ������������
	/* assign_static_prop has two opcodes! */
	
	// ƫ�Ƶ�Ŀ������벢 return ,p1:�Ƿ����쳣��ʹ�� EX(opline)  ��ʹ�� opline ��p2:ƫ����
	ZEND_VM_NEXT_OPCODE_EX(1, 2);
}

// ing2, ά�ȷ��ʸ�ֵ
ZEND_VM_HANDLER(23, ZEND_ASSIGN_DIM, VAR|CV, CONST|TMPVAR|UNUSED|NEXT|CV, SPEC(OP_DATA=CONST|TMP|VAR|CV))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *object_ptr, *orig_object_ptr;
	zval *value;
	zval *variable_ptr;
	zval *dim;

	// windows: �޲���
	SAVE_OPLINE();
	// op1 Դ����Ͷ�����ͬһ��
	// ��ȡzvalָ��, TMP/CONST/UNUSED ����null����֧�� TMPVAR/TMPVARCV
	orig_object_ptr = object_ptr = GET_OP1_ZVAL_PTR_PTR_UNDEF(BP_VAR_W);

	// �������������
	if (EXPECTED(Z_TYPE_P(object_ptr) == IS_ARRAY)) {
// ������ת��ǩ try_assign_dim_array
ZEND_VM_C_LABEL(try_assign_dim_array):
		// �����鴴��������ʹ�ø���
		SEPARATE_ARRAY(object_ptr);
		// ��� op2 ������δ����
		if (OP2_TYPE == IS_UNUSED) {
			// ȡ����һ��������� zval��UNUSED����null, CV/TMPVARCV ֱ�ӷ���
			value = GET_OP_DATA_ZVAL_PTR_UNDEF(BP_VAR_R);
			// ������������� ������� ���� ִ������ֵΪ δ����
			if (OP_DATA_TYPE == IS_CV && UNEXPECTED(Z_TYPE_P(value) == IS_UNDEF)) {
				// ȡ������
				HashTable *ht = Z_ARRVAL_P(object_ptr);
				// �����ϣ���ǲ��ɸ���
				if (!(GC_FLAGS(ht) & IS_ARRAY_IMMUTABLE)) {
					// �������ô���
					GC_ADDREF(ht);
				}
				// ����δ����ı�������������, ����δ��ʼ��zval, EXECUTE_DATA_DC ����Ҳ�ò���
				value = zval_undefined_cv((opline+1)->op1.var EXECUTE_DATA_CC);
				// ���ô���-1�����Ϊ0
				if (!(GC_FLAGS(ht) & IS_ARRAY_IMMUTABLE) && !GC_DELREF(ht)) {
					// ���ٹ�ϣ��
					zend_array_destroy(ht);
					// ��ת��ָ����ǩ assign_dim_error
					ZEND_VM_C_GOTO(assign_dim_error);
				}
			}
			// ��� �������������Ǳ������ �� ����
			if (OP_DATA_TYPE == IS_CV || OP_DATA_TYPE == IS_VAR) {
				// ֵ������
				ZVAL_DEREF(value);
			}
			// �ڹ�ϣ���в�����Ԫ��
			value = zend_hash_next_index_insert(Z_ARRVAL_P(object_ptr), value);
			// �������ʧ��
			if (UNEXPECTED(value == NULL)) {
				// �������Ԫ��ʧ�ܣ���һ��Ԫ���Ѿ�����
				zend_cannot_add_element();
				// ��ת��ָ����ǩ assign_dim_error
				ZEND_VM_C_GOTO(assign_dim_error);
			// ����������������� �������
			} else if (OP_DATA_TYPE == IS_CV) {
				// ����� �ɼ�������
				if (Z_REFCOUNTED_P(value)) {
					// ���ô���+1
					Z_ADDREF_P(value);
				}
			// ��� �������������� ����
			} else if (OP_DATA_TYPE == IS_VAR) {
				// ��ִ��������ȡ�� ָ����ŵı���
				zval *free_op_data = EX_VAR((opline+1)->op1.var);
				// ��� ������Ŷ����
				if (Z_ISREF_P(free_op_data)) {
					// ����� �ɼ�������
					if (Z_REFCOUNTED_P(value)) {
						// ���ô���+1
						Z_ADDREF_P(value);
					}
					// ����û�����ô����Ķ���,��û������ zval����������gc�������ڡ�
					zval_ptr_dtor_nogc(free_op_data);
				}
			// ������������ǳ���
			} else if (OP_DATA_TYPE == IS_CONST) {
				// ����� �ɼ�������
				if (UNEXPECTED(Z_REFCOUNTED_P(value))) {
					// ���ô���+1
					Z_ADDREF_P(value);
				}
			}
		// ����
		} else {
			// ��ȡzvalָ��, UNUSED ����null
			dim = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
			// ��� op2 �ǳ���
			if (OP2_TYPE == IS_CONST) {
				// ������һ������Ԫ�أ�����Ϊ����
				variable_ptr = zend_fetch_dimension_address_inner_W_CONST(Z_ARRVAL_P(object_ptr), dim EXECUTE_DATA_CC);
			// ����op2�Ǳ���
			} else {
				// ������һ������Ԫ�أ�����Ϊ����
				variable_ptr = zend_fetch_dimension_address_inner_W(Z_ARRVAL_P(object_ptr), dim EXECUTE_DATA_CC);
			}
			
			// ���û�в��ҵ�Ԫ��
			if (UNEXPECTED(variable_ptr == NULL)) {
				// ��ת��ָ����ǩ assign_dim_error
				ZEND_VM_C_GOTO(assign_dim_error);
			}
			// ȡ����һ��������� zval��UNUSED����null, ��֧�� TMPVARCV��
			value = GET_OP_DATA_ZVAL_PTR(BP_VAR_R);
			// ��������ֵ��p1:������p2:ֵ��p3:ֵ�ı������ͣ������������������������p4:�Ƿ��ϸ�����
			// ĳִ���������������� �Ĳ����Ƿ�ʹ���ϸ���������
			value = zend_assign_to_variable(variable_ptr, value, OP_DATA_TYPE, EX_USES_STRICT_TYPES());
		}
		// ��� ���������������Ч(����IS_UNUSED)
		if (UNEXPECTED(RETURN_VALUE_USED(opline))) {
			// ȡ����ֵ���Ƹ����
			ZVAL_COPY(EX_VAR(opline->result.var), value);
		}
	// ���򣬶���������
	} else {
		// ��� op1����������
		if (EXPECTED(Z_ISREF_P(object_ptr))) {
			// ������
			object_ptr = Z_REFVAL_P(object_ptr);
			// ���op1������ 
			if (EXPECTED(Z_TYPE_P(object_ptr) == IS_ARRAY)) {
				// ��ת��ָ����ǩ try_assign_dim_array
				ZEND_VM_C_GOTO(try_assign_dim_array);
			}
		}
		// ��� op1�Ƕ���
		if (EXPECTED(Z_TYPE_P(object_ptr) == IS_OBJECT)) {
			zend_object *obj = Z_OBJ_P(object_ptr);
			// �������ô���
			GC_ADDREF(obj);
			// ��ȡzvalָ��, UNUSED ����null
			dim = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
			// ��� op2 �� ������� ���� dim ������δ����
			if (OP2_TYPE == IS_CV && UNEXPECTED(Z_ISUNDEF_P(dim))) {
				// ����p2�ı�������������δ����, ����δ��ʼ��zval
				dim = ZVAL_UNDEFINED_OP2();
			// ������� o2�ǳ��� ���� �ж���ֵ
			} else if (OP2_TYPE == IS_CONST && Z_EXTRA_P(dim) == ZEND_EXTRA_VALUE) {
				// ��ȡ����ֵ
				dim++;
			}

			// ȡ����һ��������� zval��UNUSED����null, CV/TMPVARCV ֱ�ӷ���
			value = GET_OP_DATA_ZVAL_PTR_UNDEF(BP_VAR_R);
			
			// ����������������� ������� ���� ֵΪδ����
			if (OP_DATA_TYPE == IS_CV && UNEXPECTED(Z_ISUNDEF_P(value))) {
				// ����δ����ı�������������, ����δ��ʼ��zval, EXECUTE_DATA_DC ����Ҳ�ò���
				value = zval_undefined_cv((opline+1)->op1.var EXECUTE_DATA_CC);
			// ��� ������������Ϊ ������� �� ��ͨ����
			} else if (OP_DATA_TYPE & (IS_CV|IS_VAR)) {
				// ֵ������
				ZVAL_DEREF(value);
			}
			// ������һ��������ֵ
			zend_assign_to_object_dim(obj, dim, value OPLINE_CC EXECUTE_DATA_CC);

			// �ͷ���һ��������� op1��CONST/UNUSED/CV ���������֧�� TMPVARCV��
			FREE_OP_DATA();
			// ���ô���-1�����Ϊ0
			if (UNEXPECTED(GC_DELREF(obj) == 0)) {
				// �ͷ�ָ���Ķ��󣬲����� objects_store �е�ָ��
				zend_objects_store_del(obj);
			}
		// ��� op1�������ִ�
		} else if (EXPECTED(Z_TYPE_P(object_ptr) == IS_STRING)) {
			// ��� op2 ������δ����
			if (OP2_TYPE == IS_UNUSED) {
				// ���������Զ�stringʹ��[]����
				zend_use_new_element_for_string();
				// �ͷ���һ��������� op1��CONST/UNUSED/CV ���������֧�� TMPVARCV��
				FREE_OP_DATA();
				// ��� opline ��������� ��������ʱ�������ѽ�� zval ���Ϊδ����
				UNDEF_RESULT();
			// ����
			} else {
				// ��ȡzvalָ��, UNUSED ����null
				dim = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
				// ȡ����һ��������� zval��UNUSED����null, CV/TMPVARCV ֱ�ӷ���
				value = GET_OP_DATA_ZVAL_PTR_UNDEF(BP_VAR_R);
				// �޸��ִ��е�ĳ1���ַ������Թ���
				zend_assign_to_string_offset(object_ptr, dim, value OPLINE_CC EXECUTE_DATA_CC);
				// �ͷ���һ��������� op1��CONST/UNUSED/CV ���������֧�� TMPVARCV��
				FREE_OP_DATA();
			}
		// ���op1������ undef/null/false 
		} else if (EXPECTED(Z_TYPE_P(object_ptr) <= IS_FALSE)) {
			// ��� op1 ԭ������������ 
			if (Z_ISREF_P(orig_object_ptr)
			// ���� ������
			 && ZEND_REF_HAS_TYPE_SOURCES(Z_REF_P(orig_object_ptr))
			// ���� ���������Ƿ���Ա� ��ֵ������ʵ�������������
			 && !zend_verify_ref_array_assignable(Z_REF_P(orig_object_ptr))) {
				// ��ȡzvalָ�룬UNUSED ���ͷ���null
				dim = GET_OP2_ZVAL_PTR(BP_VAR_R);
				// �ͷ���һ��������� op1��CONST/UNUSED/CV ���������֧�� TMPVARCV��
				FREE_OP_DATA();
				// ��� opline ��������� ��������ʱ�������ѽ�� zval ���Ϊδ����
				UNDEF_RESULT();
			// ���� 
			} else {
				// ���������飬8��Ԫ��
				HashTable *ht = zend_new_array(8);
				// op1 ������
				zend_uchar old_type = Z_TYPE_P(object_ptr);

				// �ѹ�ϣ����ӵ� zval��
				ZVAL_ARR(object_ptr, ht);
				// ����������� false
				if (UNEXPECTED(old_type == IS_FALSE)) {
					// �������ô���
					GC_ADDREF(ht);
					//  ������falseת������������
					zend_false_to_array_deprecated();
					// ���ô���-1�����Ϊ0
					if (UNEXPECTED(GC_DELREF(ht) == 0)) {
						// ���ٹ�ϣ��
						zend_array_destroy(ht);
						// ��ת��ָ����ǩ assign_dim_error
						ZEND_VM_C_GOTO(assign_dim_error);
					}
				}
				// ��ת��ָ����ǩ try_assign_dim_array
				ZEND_VM_C_GOTO(try_assign_dim_array);
			}
		// ����
		} else {
			// ���������԰ѱ�������������
			zend_use_scalar_as_array();
			// ��ȡzvalָ�룬UNUSED ���ͷ���null
			dim = GET_OP2_ZVAL_PTR(BP_VAR_R);
// ������ת��ǩ assign_dim_error
ZEND_VM_C_LABEL(assign_dim_error):
			// �ͷ���һ��������� op1��CONST/UNUSED/CV ���������֧�� TMPVARCV��
			FREE_OP_DATA();
			// ��� ���������������Ч(����IS_UNUSED)
			if (UNEXPECTED(RETURN_VALUE_USED(opline))) {
				// ������ ���Ϊnull
				ZVAL_NULL(EX_VAR(opline->result.var));
			}
		}
	}
	// ��� op2��Ч
	if (OP2_TYPE != IS_UNUSED) {
		// �ͷŲ�������ĸ��ӱ���
		FREE_OP2();
	}
	// �ͷŲ�������ĸ��ӱ���
	FREE_OP1();
	// ά�ȸ�ֵ������������
	/* assign_dim has two opcodes! */
	// ƫ�Ƶ�Ŀ������벢 return ,p1:�Ƿ����쳣��ʹ�� EX(opline)  ��ʹ�� opline ��p2:ƫ����
	ZEND_VM_NEXT_OPCODE_EX(1, 2);
}

// ing3, ��������ֵ
ZEND_VM_HANDLER(22, ZEND_ASSIGN, VAR|CV, CONST|TMP|VAR|CV, SPEC(RETVAL))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *value;
	zval *variable_ptr;

	// windows: �޲���
	SAVE_OPLINE();
	// ��ȡzvalָ�룬UNUSED ���ͷ���null
	value = GET_OP2_ZVAL_PTR(BP_VAR_R);
	// ��ȡzvalָ��, TMP/CONST/UNUSED ����null����֧�� TMPVAR/TMPVARCV
	variable_ptr = GET_OP1_ZVAL_PTR_PTR_UNDEF(BP_VAR_W);

	// ��������ֵ��p1:������p2:ֵ��p3:ֵ�ı������ͣ������������������������p4:�Ƿ��ϸ�����
	// ĳִ���������������� �Ĳ����Ƿ�ʹ���ϸ���������
	value = zend_assign_to_variable(variable_ptr, value, OP2_TYPE, EX_USES_STRICT_TYPES());
	// ��� ���������������Ч(����IS_UNUSED)
	if (UNEXPECTED(RETURN_VALUE_USED(opline))) {
		// ֵ�ŵ������
		ZVAL_COPY(EX_VAR(opline->result.var), value);
	}
	// �ͷŲ�������ĸ��ӱ���
	FREE_OP1();
	// zend_assign_to_variable �ܻ��õ� op1,��Ҫ�ͷ���
	/* zend_assign_to_variable() always takes care of op2, never free it! */

	// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, ���ø�ֵ
ZEND_VM_HANDLER(30, ZEND_ASSIGN_REF, VAR|CV, VAR|CV, SRC)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *variable_ptr;
	zval *value_ptr;

	// windows: �޲���
	SAVE_OPLINE();
	// ��ȡzvalָ��,TMP/CONST/UNUSED ����null����֧�� TMPVAR/TMPVARCV
	value_ptr = GET_OP2_ZVAL_PTR_PTR(BP_VAR_W);
	// ��ȡzvalָ��, TMP/CONST/UNUSED ����null����֧�� TMPVAR/TMPVARCV
	variable_ptr = GET_OP1_ZVAL_PTR_PTR_UNDEF(BP_VAR_W);

	// ���op1�Ǳ��� ���� op1���Ǽ������
	if (OP1_TYPE == IS_VAR &&
	           UNEXPECTED(Z_TYPE_P(EX_VAR(opline->op1.var)) != IS_INDIRECT)) {
		// ���������Ը�һ���������ά�ȸ�ֵʱʹ����������
		zend_throw_error(NULL, "Cannot assign by reference to an array dimension of an object");
		// ʹ��δ��ʼ��zval
		variable_ptr = &EG(uninitialized_zval);
	// ���op2�Ǳ��� ���� ��return������е��ú��� ���� value_ptr������������
	} else if (OP2_TYPE == IS_VAR &&
	           opline->extended_value == ZEND_RETURNS_FUNCTION &&
			   UNEXPECTED(!Z_ISREF_P(value_ptr))) {
		// ���������� ��ֵ�����д�����ʾ��Ϣ�������ǽ��и�ֵ�ˡ�p1:����ָ�룬p2:ֵָ��
		variable_ptr = zend_wrong_assign_to_variable_reference(
			variable_ptr, value_ptr OPLINE_CC EXECUTE_DATA_CC);
	// ����
	} else {
		// �Ա����������ø�ֵ��p1:����ָ�룬p2:ֵָ��
		zend_assign_to_variable_reference(variable_ptr, value_ptr);
	}

	// ��� ���������������Ч(����IS_UNUSED)
	if (UNEXPECTED(RETURN_VALUE_USED(opline))) {
		// ��ֵ���Ƹ����
		ZVAL_COPY(EX_VAR(opline->result.var), variable_ptr);
	}

	// �ͷŲ�������ĸ��ӱ���
	FREE_OP2();
	// �ͷŲ�������ĸ��ӱ���
	FREE_OP1();
	// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, ���������� ���ø�ֵ
ZEND_VM_HANDLER(32, ZEND_ASSIGN_OBJ_REF, VAR|UNUSED|THIS|CV, CONST|TMPVAR|CV, CACHE_SLOT|SRC, SPEC(OP_DATA=VAR|CV))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *property, *container, *value_ptr;

	// windows: �޲���
	SAVE_OPLINE();

	// ��ȡzvalָ��, TMP/CONST����null, UNUSED ���� $this����֧�� TMPVAR/TMPVARCV��CV ֱ�ӷ��ء�
	container = GET_OP1_OBJ_ZVAL_PTR_PTR_UNDEF(BP_VAR_W);
	// ��ȡzvalָ�룬UNUSED ���ͷ���null
	property = GET_OP2_ZVAL_PTR(BP_VAR_R);

	// ȡ����һ��������� zval��TMP/CONST/UNUSED����null, ��֧�� TMPVAR/TMPVARCV
	value_ptr = GET_OP_DATA_ZVAL_PTR_PTR(BP_VAR_W);
	// op1��op2��һ������ANY
	if (ZEND_VM_SPEC) {
		// ��� op1 ������δ����
		if (OP1_TYPE == IS_UNUSED) {
			// ���op2 �ǳ���
			if (OP2_TYPE == IS_CONST) {
				// �����Խ������ø�ֵ�����������������Ϊδʹ�ã����������������Ϊ����
				// p1:����������p2:����ָ�룬p3:��ֵ
				zend_assign_to_property_reference_this_const(container, property, value_ptr OPLINE_CC EXECUTE_DATA_CC);
			// ����op2���ǳ���
			} else {
				// �����Խ������ø�ֵ�����������������Ϊ δʹ�ã����������������Ϊ����
				// p1:����������p2:����ָ�룬p3:��ֵ
				zend_assign_to_property_reference_this_var(container, property, value_ptr OPLINE_CC EXECUTE_DATA_CC);
			}
		// ����
		} else {
			// ���op2�ǳ���
			if (OP2_TYPE == IS_CONST) {
				// �����Խ������ø�ֵ�����������������Ϊ ���������������������Ϊ����
				// p1:����������p2:����ָ�룬p3:��ֵ
				zend_assign_to_property_reference_var_const(container, property, value_ptr OPLINE_CC EXECUTE_DATA_CC);
			// ����op2���ǳ���
			} else {
				// �����Խ������ø�ֵ�����������������Ϊ ���������������������Ϊ����
				// p1:����������p2:����ָ�룬p3:��ֵ
				zend_assign_to_property_reference_var_var(container, property, value_ptr OPLINE_CC EXECUTE_DATA_CC);
			}
		}
	// ����
	} else {
		// �����Խ������ø�ֵ��p1:����������p2:��������������ͣ�p3:����ָ�룬p4:��������������ͣ�p5:��ֵ
		zend_assign_to_property_reference(container, OP1_TYPE, property, OP2_TYPE, value_ptr OPLINE_CC EXECUTE_DATA_CC);
	}

	// �ͷŲ�������ĸ��ӱ���
	FREE_OP1();
	// �ͷŲ�������ĸ��ӱ���
	FREE_OP2();
	// �ͷ���һ��������� op1��CONST/UNUSED/CV ���������֧�� TMPVARCV��
	FREE_OP_DATA();
	// ƫ�Ƶ�Ŀ������벢 return ,p1:�Ƿ����쳣��ʹ�� EX(opline)  ��ʹ�� opline ��p2:ƫ����
	ZEND_VM_NEXT_OPCODE_EX(1, 2);
}

// ing2, ����̬�������ø�ֵ,������һ�� OP_DATA ר�Ŵ������ݵĲ����룿
/* No specialization for op_types (CONST|TMPVAR|CV, UNUSED|CONST|VAR) */
ZEND_VM_HANDLER(33, ZEND_ASSIGN_STATIC_PROP_REF, ANY, ANY, CACHE_SLOT|SRC)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *prop, *value_ptr;
	zend_property_info *prop_info;

	// windows: �޲���
	SAVE_OPLINE();

	// ͨ������λ�ú����ͣ����ؾ�̬���Ե�ַ��p1:��������ֵ��p2:����������Ϣ��p3:����λ�ã�p4:��ȡ���ͣ�p5:falgs
	if (zend_fetch_static_property_address(&prop, &prop_info, opline->extended_value & ~ZEND_RETURNS_FUNCTION, BP_VAR_W, 0 OPLINE_CC EXECUTE_DATA_CC) != SUCCESS) {
		// �ͷ���һ��������� op1��CONST/UNUSED/CV ���������֧�� TMPVARCV��
		FREE_OP_DATA();
		// ��� opline ��������� ��������ʱ�������ѽ�� zval ���Ϊδ����
		UNDEF_RESULT();
		// ��Ҫ�� return
		HANDLE_EXCEPTION();
	}

	// ȡ����һ��������� zval��TMP/CONST/UNUSED����null, ��֧�� TMPVAR/TMPVARCV
	value_ptr = GET_OP_DATA_ZVAL_PTR_PTR(BP_VAR_W);

	// ��� ���������Ǳ��� ���� ZEND_RETURNS_FUNCTION ���� value_ptr ������������
	if (OP_DATA_TYPE == IS_VAR && (opline->extended_value & ZEND_RETURNS_FUNCTION) && UNEXPECTED(!Z_ISREF_P(value_ptr))) {
		// ���������� ��ֵ�����д�����ʾ��Ϣ�������ǽ��и�ֵ�ˡ�p1:����ָ�룬p2:ֵָ��
		if (UNEXPECTED(!zend_wrong_assign_to_variable_reference(prop, value_ptr OPLINE_CC EXECUTE_DATA_CC))) {
			// ֵΪ��δ��ʼ�� zval
			prop = &EG(uninitialized_zval);
		}
	// ���� ������ʹ���
	} else if (UNEXPECTED(ZEND_TYPE_IS_SET(prop_info->type))) {
		// �������͵����Խ������ø�ֵ��p1:������Ϣ��p2:����ʵ����p3:��ֵ
		prop = zend_assign_to_typed_property_reference(prop_info, prop, value_ptr EXECUTE_DATA_CC);
	// ����
	} else {
		// �Ա����������ø�ֵ��p1:����ָ�룬p2:ֵָ��
		zend_assign_to_variable_reference(prop, value_ptr);
	}

	// ��� ���������������Ч(����IS_UNUSED)
	if (UNEXPECTED(RETURN_VALUE_USED(opline))) {
		// ��
		ZVAL_COPY(EX_VAR(opline->result.var), prop);
	}

	// �ͷ���һ��������� op1��CONST/UNUSED/CV ���������֧�� TMPVARCV��
	FREE_OP_DATA();
	// ƫ�Ƶ�Ŀ������벢 return ,p1:�Ƿ����쳣��ʹ�� EX(opline)  ��ʹ�� opline ��p2:ƫ����
	ZEND_VM_NEXT_OPCODE_EX(1, 2);
}

// 140�У�
// ing2, �˳���ǰִ������
ZEND_VM_HOT_HELPER(zend_leave_helper, ANY, ANY)
{
	// 
	zend_execute_data *old_execute_data;
	// ȡ��������Ϣ
	uint32_t call_info = EX_CALL_INFO();
	// windows: �޲���
	SAVE_OPLINE();

	// ����⼸�����һ��Ҳû��
	if (EXPECTED((call_info & (ZEND_CALL_CODE|ZEND_CALL_TOP|ZEND_CALL_HAS_SYMBOL_TABLE|ZEND_CALL_FREE_EXTRA_ARGS|ZEND_CALL_ALLOCATED|ZEND_CALL_HAS_EXTRA_NAMED_PARAMS)) == 0)) {
		// ʹ��ǰһ��ִ����
		EG(current_execute_data) = EX(prev_execute_data);
		// ���� ִ�к����е�������ʱ����
		i_free_compiled_variables(execute_data);

#ifdef ZEND_PREFER_RELOAD
		call_info = EX_CALL_INFO();
#endif
		// �����Ҫ�ͷ�$this
		if (UNEXPECTED(call_info & ZEND_CALL_RELEASE_THIS)) {
			// �ͷ�$this
			OBJ_RELEASE(Z_OBJ(execute_data->This));
		// ������ڵ��ñհ�
		} else if (UNEXPECTED(call_info & ZEND_CALL_CLOSURE)) {
			// �ͷűհ�
			// �� _zend_closure.zend_function �ҵ� _zend_closure ��ͷ��
			OBJ_RELEASE(ZEND_CLOSURE_OBJECT(EX(func)));
		}
		// ��ִ�����ݷ����������ջ��
		EG(vm_stack_top) = (zval*)execute_data;
		// ת��ǰһ��ִ������
		execute_data = EX(prev_execute_data);

		// ������쳣
		if (UNEXPECTED(EG(exception) != NULL)) {
			// �������쳣
			zend_rethrow_exception(execute_data);
			// windows: return 2;
			HANDLE_EXCEPTION_LEAVE();
		}

		// windows: ZEND_VM_INC_OPCODE()�� // OPLINE++
		LOAD_NEXT_OPLINE();
		// windows return 2;
		ZEND_VM_LEAVE();
	// ���� ����е��ô��� �� �Ƕ������
	} else if (EXPECTED((call_info & (ZEND_CALL_CODE|ZEND_CALL_TOP)) == 0)) {
		// ǰһ��ִ������
		EG(current_execute_data) = EX(prev_execute_data);
		// ���� ִ�к����е�������ʱ����
		i_free_compiled_variables(execute_data);

#ifdef ZEND_PREFER_RELOAD
		call_info = EX_CALL_INFO();
#endif
		// ����з��ű�
		if (UNEXPECTED(call_info & ZEND_CALL_HAS_SYMBOL_TABLE)) {
			// ��ղ����� ���ű�
			zend_clean_and_cache_symbol_table(EX(symbol_table));
		}

		// ����ж�����������
		if (UNEXPECTED(call_info & ZEND_CALL_HAS_EXTRA_NAMED_PARAMS)) {
			// �ͷŶ�����������
			zend_free_extra_named_params(EX(extra_named_params));
		}

		// ���ͷűհ�ǰ�ͷŶ����������Ϊ�����ͷ� op_array
		/* Free extra args before releasing the closure,
		 * as that may free the op_array. */
		 
		// ���� �������ջ��Ķ��������p1:������Ϣ��p2:ִ������
		zend_vm_stack_free_extra_args_ex(call_info, execute_data);

		// �����Ҫ�ͷ� $this
		if (UNEXPECTED(call_info & ZEND_CALL_RELEASE_THIS)) {
			// �ͷ� $this
			OBJ_RELEASE(Z_OBJ(execute_data->This));
		// ������� �ڵ��ñհ�
		} else if (UNEXPECTED(call_info & ZEND_CALL_CLOSURE)) {
			// �ͷűհ�
			OBJ_RELEASE(ZEND_CLOSURE_OBJECT(EX(func)));
		}

		// ��ǰִ��������Ϊ�ɵ�
		old_execute_data = execute_data;
		// �л���ǰһ��ִ������
		execute_data = EX(prev_execute_data);
		// �ͷŵ��ÿ�ܣ�ִ�����ݣ���p1:������Ϣ��p2:ִ������
		zend_vm_stack_free_call_frame_ex(call_info, old_execute_data);

		// ������쳣
		if (UNEXPECTED(EG(exception) != NULL)) {
			// �������쳣
			zend_rethrow_exception(execute_data);
			// windows: return 2;
			HANDLE_EXCEPTION_LEAVE();
		}

		// windows: ZEND_VM_INC_OPCODE() : // OPLINE++
		LOAD_NEXT_OPLINE();
		// windows return 2;
		ZEND_VM_LEAVE();
	// ������Ƕ������
	} else if (EXPECTED((call_info & ZEND_CALL_TOP) == 0)) {
		// �������ʱ����
		if (EX(func)->op_array.last_var > 0) {
			// ����ʱ�����еı������ ͬ�������ű��У�Ȼ�������ʱ����
			zend_detach_symbol_table(execute_data);
			// ��ӡ���Ҫ���ӷ��ű����
			call_info |= ZEND_CALL_NEEDS_REATTACH;
		}
		// ���پ�̬������
		zend_destroy_static_vars(&EX(func)->op_array);
		// ���ٲ�������
		destroy_op_array(&EX(func)->op_array);
		// �ͷ����ٲ�������
		efree_size(EX(func), sizeof(zend_op_array));
		// ��Ϊ��ִ������
		old_execute_data = execute_data;
		// ��ǰִ�������л���ǰһ��
		execute_data = EG(current_execute_data) = EX(prev_execute_data);
		// �ͷŵ��ÿ�ܣ�ִ�����ݣ���p1:������Ϣ��p2:ִ������
		zend_vm_stack_free_call_frame_ex(call_info, old_execute_data);

		// ����� ����Ҫ���ӷ��ű����
		if (call_info & ZEND_CALL_NEEDS_REATTACH) {
			// �������ʱ����
			if (EX(func)->op_array.last_var > 0) {
				// �Ӹ��ӷ��ű����ȡ������ʱ�����������µ���ʱ�����б���
				zend_attach_symbol_table(execute_data);
			// ����
			} else {
				// ��ӱ�ǣ���Ҫ�������ӷ��ű�
				ZEND_ADD_CALL_FLAG(execute_data, ZEND_CALL_NEEDS_REATTACH);
			}
		}
		// ������쳣
		if (UNEXPECTED(EG(exception) != NULL)) {
			// �������쳣
			zend_rethrow_exception(execute_data);
			// windows: return 2;
			HANDLE_EXCEPTION_LEAVE();
		}

		// windows: ZEND_VM_INC_OPCODE() : // OPLINE++
		LOAD_NEXT_OPLINE();
		// windows return 2;
		ZEND_VM_LEAVE();
	// ����
	} else {
		// ���û�е��ô�����
		if (EXPECTED((call_info & ZEND_CALL_CODE) == 0)) {
			// �л���ǰһ��ִ������
			EG(current_execute_data) = EX(prev_execute_data);
			// ���� ִ�к����е�������ʱ����
			i_free_compiled_variables(execute_data);
#ifdef ZEND_PREFER_RELOAD
			call_info = EX_CALL_INFO();
#endif
			// ����� ���з��ű�����Ҫ�ͷŶ�����������ж�������������3�����κ�һ��
			if (UNEXPECTED(call_info & (ZEND_CALL_HAS_SYMBOL_TABLE|ZEND_CALL_FREE_EXTRA_ARGS|ZEND_CALL_HAS_EXTRA_NAMED_PARAMS))) {
				// ����з��ű�
				if (UNEXPECTED(call_info & ZEND_CALL_HAS_SYMBOL_TABLE)) {
					// ��ղ����� ���ű�
					zend_clean_and_cache_symbol_table(EX(symbol_table));
				}
				// ���� �������ջ��Ķ��������p1:������Ϣ��p2:ִ������
				zend_vm_stack_free_extra_args_ex(call_info, execute_data);
				// ����ж�����������
				if (UNEXPECTED(call_info & ZEND_CALL_HAS_EXTRA_NAMED_PARAMS)) {
					// �ͷŶ������
					zend_free_extra_named_params(EX(extra_named_params));
				}
			}
			// ����ǵ��ñհ�
			if (UNEXPECTED(call_info & ZEND_CALL_CLOSURE)) {
				// �ͷűհ�
				OBJ_RELEASE(ZEND_CLOSURE_OBJECT(EX(func)));
			}
			// windows: return -1
			ZEND_VM_RETURN();
		// �����ǵ��ö������
		} else /* if (call_kind == ZEND_CALL_TOP_CODE) */ {
			// ��ǰִ�����ݵķ��ű�
			zend_array *symbol_table = EX(symbol_table);

			// �������ʱ����
			if (EX(func)->op_array.last_var > 0) {
				// ����ʱ�����еı������ ͬ�������ű��У�Ȼ�������ʱ����
				zend_detach_symbol_table(execute_data);
				// ��ӱ�ǣ���Ҫ�������ӷ��ű�
				call_info |= ZEND_CALL_NEEDS_REATTACH;
			}
			// �����Ҫ�������ӷ��ű�
			if (call_info & ZEND_CALL_NEEDS_REATTACH) {
				// ǰһ��ִ������
				old_execute_data = EX(prev_execute_data);
				// ��ǰ�������������ҵ��з��ű��ִ������
				while (old_execute_data) {
					// �����ִ�������к��� ���� �з��ű�
					if (old_execute_data->func && (ZEND_CALL_INFO(old_execute_data) & ZEND_CALL_HAS_SYMBOL_TABLE)) {
						// �������Ҫ�ҵķ��ű�
						if (old_execute_data->symbol_table == symbol_table) {
							// �����������ʱ����
							if (old_execute_data->func->op_array.last_var > 0) {
								// �Ӹ��ӷ��ű����ȡ������ʱ�����������µ���ʱ�����б���
								zend_attach_symbol_table(old_execute_data);
							// ����
							} else {
								// ��ӱ�ǣ���Ҫ�������ӷ��ű�
								ZEND_ADD_CALL_FLAG(old_execute_data, ZEND_CALL_NEEDS_REATTACH);
							}
						}
						// ֻҪ�з��ű������
						break;
					}
					// �� ǰһ��ִ������
					old_execute_data = old_execute_data->prev_execute_data;
				}
			}
			// �л���ǰһ��ִ������
			EG(current_execute_data) = EX(prev_execute_data);
			// windows: return -1
			ZEND_VM_RETURN();
		}
	}
}

// ing3, ��ת��op1ָ���Ĳ�����
ZEND_VM_HOT_HANDLER(42, ZEND_JMP, JMP_ADDR, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// Ҫ����쳣 �� ���쳣ʱʹ�þɲ����룬���쳣ʹ���²�����, p1:�²����룬p2:�Ƿ����쳣
	// OP_JMP_ADDR: ���� p2.jmp_addr
	ZEND_VM_JMP_EX(OP_JMP_ADDR(opline, opline->op1), 0);
}

// ing3, ����op1��ת
ZEND_VM_HOT_NOCONST_HANDLER(43, ZEND_JMPZ, CONST|TMPVAR|CV, JMP_ADDR)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *val;
	zend_uchar op1_type;

	// ��ȡzvalָ��, UNUSED ����null
	val = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);

	// ��� op1 ���� Ϊ true
	if (Z_TYPE_INFO_P(val) == IS_TRUE) {
		// opline + 1����Ŀ������벢 return
		ZEND_VM_NEXT_OPCODE();
	// ���� ��� op1 ����Ϊ false,null,δ����
	} else if (EXPECTED(Z_TYPE_INFO_P(val) <= IS_TRUE)) {
		// ��� op1 ����Ϊ ������� ���� ֵ����Ϊδ����
		if (OP1_TYPE == IS_CV && UNEXPECTED(Z_TYPE_INFO_P(val) == IS_UNDEF)) {
			// windows: �޲���
			SAVE_OPLINE();
			// ����p1�ı�������������δ����, ����δ��ʼ��zval
			ZVAL_UNDEFINED_OP1();
			// ������쳣
			if (UNEXPECTED(EG(exception))) {
				// ��Ҫ�� return
				HANDLE_EXCEPTION();
			}
		}
		// Ҫ����쳣 �� ���쳣ʱʹ�þɲ����룬���쳣ʹ���²�����, p1:�²����룬p2:�Ƿ����쳣
		// OP_JMP_ADDR: ���� p2.jmp_addr
		ZEND_VM_JMP_EX(OP_JMP_ADDR(opline, opline->op2), 0);
	}

	// windows: �޲���
	SAVE_OPLINE();
	// op1����
	op1_type = OP1_TYPE;
	// ֵת�ɲ�����
	if (i_zend_is_true(val)) {
		// ��һ��������
		opline++;
	// ����
	} else {
		// ����ָ��������
		// OP_JMP_ADDR: ���� p2.jmp_addr
		opline = OP_JMP_ADDR(opline, opline->op2);
	}
	// ���op1�� ��ͨ���� �� ��ʱ����
	if (op1_type & (IS_TMP_VAR|IS_VAR)) {
		// ����û�����ô����Ķ���,��û������ zval����������gc�������ڡ�
		zval_ptr_dtor_nogc(val);
	}
	// ���쳣ʱʹ�þɲ����룬���쳣ʹ���²�����, p1:�²�����
	ZEND_VM_JMP(opline);
}

// ing3, ����op1 ��ת����ͬ�ĵط�
ZEND_VM_HOT_NOCONST_HANDLER(44, ZEND_JMPNZ, CONST|TMPVAR|CV, JMP_ADDR)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *val;
	zend_uchar op1_type;

	// ��ȡzvalָ��, UNUSED ����null
	val = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);

	// ��� ���� Ϊ true
	if (Z_TYPE_INFO_P(val) == IS_TRUE) {
		// Ҫ����쳣 �� ���쳣ʱʹ�þɲ����룬���쳣ʹ���²�����, p1:�²����룬p2:�Ƿ����쳣
		// OP_JMP_ADDR: ���� p2.jmp_addr
		ZEND_VM_JMP_EX(OP_JMP_ADDR(opline, opline->op2), 0);
	// ���� ��� ����Ϊ false,null,δ����
	} else if (EXPECTED(Z_TYPE_INFO_P(val) <= IS_TRUE)) {
		// ��� op1 ����Ϊ ������� ���� ֵ����Ϊδ����
		if (OP1_TYPE == IS_CV && UNEXPECTED(Z_TYPE_INFO_P(val) == IS_UNDEF)) {
			// windows: �޲���
			SAVE_OPLINE();
			// ����p1�ı�������������δ����, ����δ��ʼ��zval
			ZVAL_UNDEFINED_OP1();
			// ������쳣
			if (UNEXPECTED(EG(exception))) {
				// ��Ҫ�� return
				HANDLE_EXCEPTION();
			}
		}
		// opline + 1����Ŀ������벢 return
		ZEND_VM_NEXT_OPCODE();
	}

	// windows: �޲���
	SAVE_OPLINE();
	// op1������
	op1_type = OP1_TYPE;
	// ת�ɲ����ͣ������true
	if (i_zend_is_true(val)) {
		// OP_JMP_ADDR: ���� p2.jmp_addr
		opline = OP_JMP_ADDR(opline, opline->op2);
	// ����,��false
	} else {
		// ��һ��������
		opline++;
	}
	// ���op1����ʱ���������
	if (op1_type & (IS_TMP_VAR|IS_VAR)) {
		// ����û�����ô����Ķ���,��û������ zval����������gc�������ڡ�
		zval_ptr_dtor_nogc(val);
	}
	// ���쳣ʱʹ�þɲ����룬���쳣ʹ���²�����, p1:�²�����
	ZEND_VM_JMP(opline);
}

// ing3, ����op1��ת
ZEND_VM_COLD_CONST_HANDLER(46, ZEND_JMPZ_EX, CONST|TMPVAR|CV, JMP_ADDR)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *val;
	bool ret;

	// ��ȡzvalָ��, UNUSED ����null
	val = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);

	// ��� ���� Ϊ true
	if (Z_TYPE_INFO_P(val) == IS_TRUE) {
		// ���Ϊ true
		ZVAL_TRUE(EX_VAR(opline->result.var));
		// opline + 1����Ŀ������벢 return
		ZEND_VM_NEXT_OPCODE();
	// ���� ��� ����Ϊ false,null,δ����
	} else if (EXPECTED(Z_TYPE_INFO_P(val) <= IS_TRUE)) {
		// ���Ϊ false
		ZVAL_FALSE(EX_VAR(opline->result.var));
		// ��� op1 ����Ϊ ������� ���� ֵ����Ϊδ����
		if (OP1_TYPE == IS_CV && UNEXPECTED(Z_TYPE_INFO_P(val) == IS_UNDEF)) {
			// windows: �޲���
			SAVE_OPLINE();
			// ����p1�ı�������������δ����, ����δ��ʼ��zval
			ZVAL_UNDEFINED_OP1();
			// ������쳣
			if (UNEXPECTED(EG(exception))) {
				// ��Ҫ�� return
				HANDLE_EXCEPTION();
			}
		}
		// Ҫ����쳣 �� ���쳣ʱʹ�þɲ����룬���쳣ʹ���²�����, p1:�²����룬p2:�Ƿ����쳣
		// OP_JMP_ADDR: ���� p2.jmp_addr
		ZEND_VM_JMP_EX(OP_JMP_ADDR(opline, opline->op2), 0);
	}

	// windows: �޲���
	SAVE_OPLINE();
	// ֵת�ɲ�����
	ret = i_zend_is_true(val);
	// �ͷŲ�������ĸ��ӱ���
	FREE_OP1();
	// ���ֵΪtrue
	if (ret) {
		// ���Ϊtrue
		ZVAL_TRUE(EX_VAR(opline->result.var));
		// ��һ��������
		opline++;
	// ����ֵΪfalse
	} else {
		// ���Ϊfalse
		ZVAL_FALSE(EX_VAR(opline->result.var));
		// OP_JMP_ADDR: ���� p2.jmp_addr
		opline = OP_JMP_ADDR(opline, opline->op2);
	}
	// ���쳣ʱʹ�þɲ����룬���쳣ʹ���²�����, p1:�²�����
	ZEND_VM_JMP(opline);
}

// ing3,�������ֵ��ת����if��
ZEND_VM_COLD_CONST_HANDLER(47, ZEND_JMPNZ_EX, CONST|TMPVAR|CV, JMP_ADDR)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *val;
	bool ret;

	// ��ȡzvalָ��, UNUSED ����null
	val = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);

	// ��� ���� Ϊ true
	if (Z_TYPE_INFO_P(val) == IS_TRUE) {
		ZVAL_TRUE(EX_VAR(opline->result.var));
		// Ҫ����쳣 �� ���쳣ʱʹ�þɲ����룬���쳣ʹ���²�����, p1:�²����룬p2:�Ƿ����쳣
		// OP_JMP_ADDR: ���� p2.jmp_addr
		ZEND_VM_JMP_EX(OP_JMP_ADDR(opline, opline->op2), 0);
	// ���� ��� ����Ϊ false,null,δ����
	} else if (EXPECTED(Z_TYPE_INFO_P(val) <= IS_TRUE)) {
		// ���Ϊ��
		ZVAL_FALSE(EX_VAR(opline->result.var));
		// ��� op1 ����Ϊ ������� ���� ֵ����Ϊδ����
		if (OP1_TYPE == IS_CV && UNEXPECTED(Z_TYPE_INFO_P(val) == IS_UNDEF)) {
			// windows: �޲���
			SAVE_OPLINE();
			// ����p1�ı�������������δ����, ����δ��ʼ��zval
			ZVAL_UNDEFINED_OP1();
			// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
			ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
		// ����
		} else {
			// opline + 1����Ŀ������벢 return
			ZEND_VM_NEXT_OPCODE();
		}
	}

	// windows: �޲���
	SAVE_OPLINE();
	// ֵת��bool��
	ret = i_zend_is_true(val);
	// �ͷŲ�������ĸ��ӱ���
	FREE_OP1();
	// ���ֵΪ��
	if (ret) {
		// ���Ϊ��
		ZVAL_TRUE(EX_VAR(opline->result.var));
		// ���� p2.jmp_addr
		opline = OP_JMP_ADDR(opline, opline->op2);
	// ����
	} else {
		// ���Ϊ��
		ZVAL_FALSE(EX_VAR(opline->result.var));
		// ����һ�������루ֵΪ�٣�����תĿ�꣩
		opline++;
	}
	// ��ת
	// ���쳣ʱʹ�þɲ����룬���쳣ʹ���²�����, p1:�²�����
	ZEND_VM_JMP(opline);
}

// ing3, ���ٲ����� op1 �ı���
ZEND_VM_HANDLER(70, ZEND_FREE, TMPVAR, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// windows: �޲���
	SAVE_OPLINE();
	// ����û�����ô����Ķ���,��û������ zval����������gc�������ڡ�
	zval_ptr_dtor_nogc(EX_VAR(opline->op1.var));
	// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, ��������͵�����
ZEND_VM_HOT_HANDLER(127, ZEND_FE_FREE, TMPVAR, ANY)
{
	zval *var;
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// ��ִ��������ȡ�� ָ����ŵı���
	var = EX_VAR(opline->op1.var);
	// ���op1��������
	if (Z_TYPE_P(var) != IS_ARRAY) {
		// windows: �޲���
		SAVE_OPLINE();
		// ͨ��ָ�����zval�� ���������� , (*p1).u2.fe_iter_idx
		// �����������Ч
		if (Z_FE_ITER_P(var) != (uint32_t)-1) {
			// �ڵ�������ϣ����ɾ�����������
			// ͨ��ָ�����zval�� ���������� , (*p1).u2.fe_iter_idx
			zend_hash_iterator_del(Z_FE_ITER_P(var));
		}
		// ����û�����ô����Ķ���,��û������ zval����������gc�������ڡ�
		zval_ptr_dtor_nogc(var);
		// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
		ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
	}

	// Ϊ���������飬ʹ�� һ�� zval_ptr_dtor_nogc �����ڰ汾
	// ֻ�е��������ñ����������ˣ�php����Ҫ����opline�������쳣�������е�Ԫ������ �����״���
	/* This is freeing an array. Use an inlined version of zval_ptr_dtor_nogc. */
	/* PHP only needs to save the opline and check for an exception if the last reference to the array was garbage collected (destructors of elements in the array could throw an exception) */
	// ����� �ɼ������� ���� ���ô��� -1��Ϊ0
	if (Z_REFCOUNTED_P(var) && !Z_DELREF_P(var)) {
		// windows: �޲���
		SAVE_OPLINE();
		// ����ÿ��type��Ӧ�����ٺ���ִ������
		rc_dtor_func(Z_COUNTED_P(var));
		// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
		ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
	}
	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, ��������
ZEND_VM_COLD_CONSTCONST_HANDLER(53, ZEND_FAST_CONCAT, CONST|TMPVAR|CV, CONST|TMPVAR|CV)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2;
	zend_string *op1_str, *op2_str, *str;


	// ��ȡzvalָ��, UNUSED ����null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ��ȡzvalָ��, UNUSED ����null
	op2 = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ��� op1,op2 ���ǣ��ִ� �� ������
	if ((OP1_TYPE == IS_CONST || EXPECTED(Z_TYPE_P(op1) == IS_STRING)) &&
	    (OP2_TYPE == IS_CONST || EXPECTED(Z_TYPE_P(op2) == IS_STRING))) {
		// ȡ���ִ�
		zend_string *op1_str = Z_STR_P(op1);
		zend_string *op2_str = Z_STR_P(op2);
		// 
		zend_string *str;

		// ��� OP1 ���ǳ��� ���� ����Ϊ0
		if (OP1_TYPE != IS_CONST && UNEXPECTED(ZSTR_LEN(op1_str) == 0)) {
			// ��� op2 �� ���� �� �������
			if (OP2_TYPE == IS_CONST || OP2_TYPE == IS_CV) {
				// op2 ���Ƶ������
				ZVAL_STR_COPY(EX_VAR(opline->result.var), op2_str);
			// ����
			} else {
				// op2 ���ִ�ֱ�ӷŵ������
				ZVAL_STR(EX_VAR(opline->result.var), op2_str);
			}
			// ��� op1 ���� �Ǳ�������ʱ����
			if (OP1_TYPE & (IS_TMP_VAR|IS_VAR)) {
				// �ͷ� ��ʱ�ִ�
				zend_string_release_ex(op1_str, 0);
			}
		// ���� ��� OP2 ���ǳ��� ���� ����Ϊ0
		} else if (OP2_TYPE != IS_CONST && UNEXPECTED(ZSTR_LEN(op2_str) == 0)) {
			// ��� op1 �� ���� �� �������
			if (OP1_TYPE == IS_CONST || OP1_TYPE == IS_CV) {
				// op1���Ƶ������
				ZVAL_STR_COPY(EX_VAR(opline->result.var), op1_str);
			// ����
			} else {
				// op1�ִ�ָ��ŵ������
				ZVAL_STR(EX_VAR(opline->result.var), op1_str);
			}
			// ��� op2 ���� �Ǳ�������ʱ����
			if (OP2_TYPE & (IS_TMP_VAR|IS_VAR)) {
				// �ͷ� ��ʱ�ִ�
				zend_string_release_ex(op2_str, 0);
			}
		// ���� ��� OP1 ���ǳ��� Ҳ���� ������� Ҳ���������ִ� ���� ������Ϊ1
		} else if (OP1_TYPE != IS_CONST && OP1_TYPE != IS_CV &&
		    !ZSTR_IS_INTERNED(op1_str) && GC_REFCOUNT(op1_str) == 1) {
			// ����
			size_t len = ZSTR_LEN(op1_str);
			// �ִ����ӳ���
			str = zend_string_extend(op1_str, len + ZSTR_LEN(op2_str), 0);
			// ��op2���Ƶ�����
			memcpy(ZSTR_VAL(str) + len, ZSTR_VAL(op2_str), ZSTR_LEN(op2_str)+1);
			// ��zend_string ��Ӹ� zval����֧�ֱ�����
			ZVAL_NEW_STR(EX_VAR(opline->result.var), str);
			// ��� op2 ���� �Ǳ�������ʱ����
			if (OP2_TYPE & (IS_TMP_VAR|IS_VAR)) {
				// �ͷ���ʱ����
				zend_string_release_ex(op2_str, 0);
			}
		// ����
		} else {
			// ���·����ִ�
			str = zend_string_alloc(ZSTR_LEN(op1_str) + ZSTR_LEN(op2_str), 0);
			// ǰ��һ�η�op1
			memcpy(ZSTR_VAL(str), ZSTR_VAL(op1_str), ZSTR_LEN(op1_str));
			// ����һ�η�op2
			memcpy(ZSTR_VAL(str) + ZSTR_LEN(op1_str), ZSTR_VAL(op2_str), ZSTR_LEN(op2_str)+1);
			// ��zend_string ��Ӹ� zval����֧�ֱ�����
			ZVAL_NEW_STR(EX_VAR(opline->result.var), str);
			// ��� op1 ���� �Ǳ�������ʱ����
			if (OP1_TYPE & (IS_TMP_VAR|IS_VAR)) {
				// �ͷ�op1 ��ʱ����
				zend_string_release_ex(op1_str, 0);
			}
			// ��� op2 ���� �Ǳ�������ʱ����
			if (OP2_TYPE & (IS_TMP_VAR|IS_VAR)) {
				// �ͷ�op2 ��ʱ����
				zend_string_release_ex(op2_str, 0);
			}
		}
		// opline + 1����Ŀ������벢 return
		ZEND_VM_NEXT_OPCODE();
	}

	// windows: �޲���
	SAVE_OPLINE();
	// ���op1�ǳ���
	if (OP1_TYPE == IS_CONST) {
		// ȡ���ִ�
		op1_str = Z_STR_P(op1);
	// ����������ִ�
	} else if (EXPECTED(Z_TYPE_P(op1) == IS_STRING)) {
		// op1 �������ô���. ����� ZVAL_COPY ��ͬ
		op1_str = zend_string_copy(Z_STR_P(op1));
	// ����
	} else {
		// ��� op1 �� ������� ���� ֵΪδ����
		if (OP1_TYPE == IS_CV && UNEXPECTED(Z_TYPE_P(op1) == IS_UNDEF)) {
			// ����p1�ı�������������δ����, ����δ��ʼ��zval
			ZVAL_UNDEFINED_OP1();
		}
		// zval ת�� �ִ�, û��try
		op1_str = zval_get_string_func(op1);
	}
	
	// ���op2�ǳ���
	if (OP2_TYPE == IS_CONST) {
		// ȡ���ִ�
		op2_str = Z_STR_P(op2);
	// op2���ִ�
	} else if (EXPECTED(Z_TYPE_P(op2) == IS_STRING)) {
		// op2 �������ô���. ����� ZVAL_COPY ��ͬ
		op2_str = zend_string_copy(Z_STR_P(op2));
	// ����
	} else {
		// ��� op2 �� ������� ���� ����Ϊ δ����
		if (OP2_TYPE == IS_CV && UNEXPECTED(Z_TYPE_P(op2) == IS_UNDEF)) {
			// ����p2�ı�������������δ����, ����δ��ʼ��zval
			ZVAL_UNDEFINED_OP2();
		}
		// zval ת�� �ִ�, û��try
		op2_str = zval_get_string_func(op2);
	}
	do {
		// ��� OP1 ���ǳ���
		if (OP1_TYPE != IS_CONST) {
			// ���op1����Ϊ0
			if (UNEXPECTED(ZSTR_LEN(op1_str) == 0)) {
				// �������Ϊ ����
				if (OP2_TYPE == IS_CONST) {
					// ����� �ɼ�������
					if (UNEXPECTED(Z_REFCOUNTED_P(op2))) {
						// �������ô���
						GC_ADDREF(op2_str);
					}
				}
				// ֱ�Ӱ�op2�ŵ������
				ZVAL_STR(EX_VAR(opline->result.var), op2_str);
				// �ͷ�op1��ʱ����
				zend_string_release_ex(op1_str, 0);
				// ����
				break;
			}
		}
		// ��� OP2 ���ǳ���
		if (OP2_TYPE != IS_CONST) {
			// ���op2����ΪΪ
			if (UNEXPECTED(ZSTR_LEN(op2_str) == 0)) {
				// ���op1�ǳ���
				if (OP1_TYPE == IS_CONST) {
					// ����� �ɼ�������
					if (UNEXPECTED(Z_REFCOUNTED_P(op1))) {
						// �������ô���
						GC_ADDREF(op1_str);
					}
				}
				// op1ֱ�ӷŵ������
				ZVAL_STR(EX_VAR(opline->result.var), op1_str);
				// �ͷ�op2��ʱ����
				zend_string_release_ex(op2_str, 0);
				// ����
				break;
			}
		}
		// �������ִ�
		str = zend_string_alloc(ZSTR_LEN(op1_str) + ZSTR_LEN(op2_str), 0);
		// �Ȱ�op1�Ž���
		memcpy(ZSTR_VAL(str), ZSTR_VAL(op1_str), ZSTR_LEN(op1_str));
		// �ٰ�op2�Ž���
		memcpy(ZSTR_VAL(str) + ZSTR_LEN(op1_str), ZSTR_VAL(op2_str), ZSTR_LEN(op2_str)+1);
		// ��zend_string ��Ӹ� zval����֧�ֱ�����
		ZVAL_NEW_STR(EX_VAR(opline->result.var), str);
		// ��� OP1 ���ǳ���
		if (OP1_TYPE != IS_CONST) {
			// �ͷ�op1��ʱ����
			zend_string_release_ex(op1_str, 0);
		}
		// ��� OP2 ���ǳ���
		if (OP2_TYPE != IS_CONST) {
			// �ͷ�op2��ʱ����
			zend_string_release_ex(op2_str, 0);
		}
	} while (0);
	// �ͷŲ�������ĸ��ӱ���
	FREE_OP1();
	// �ͷŲ�������ĸ��ӱ���
	FREE_OP2();
	// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, ��ʼ�����ִ���ı����б���ӵ�һ������
ZEND_VM_HANDLER(54, ZEND_ROPE_INIT, UNUSED, CONST|TMPVAR|CV, NUM)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zend_string **rope;
	zval *var;

	// �������Ѿ������˱�Ҫ������zvalλ�������� rope
	/* Compiler allocates the necessary number of zval slots to keep the rope */
	// ȡ��rope��zend_stringָ���ָ��
	rope = (zend_string**)EX_VAR(opline->result.var);
	// �������Ϊ ����
	if (OP2_TYPE == IS_CONST) {
		// ��ȡzvalָ�룬UNUSED ���ͷ���null
		var = GET_OP2_ZVAL_PTR(BP_VAR_R);
		// ���ɵ�һ��rope
		rope[0] = Z_STR_P(var);
		// ����� �ɼ�������
		if (UNEXPECTED(Z_REFCOUNTED_P(var))) {
			// ���ô���+1
			Z_ADDREF_P(var);
		}
	// ����,���ǳ���
	} else {
		// ��ȡop2��zvalָ��, UNUSED ����null
		var = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
		// ����������ִ�
		if (EXPECTED(Z_TYPE_P(var) == IS_STRING)) {
			// ��� op2 �� �������
			if (OP2_TYPE == IS_CV) {
				// ȡ���ִ���������һ��rope
				// �������ô���. ����� ZVAL_COPY ��ͬ
				rope[0] = zend_string_copy(Z_STR_P(var));
			// ����
			} else {
				// ȡ�� �ִ���������һ��rope
				rope[0] = Z_STR_P(var);
			}
		// ����
		} else {
			// windows: �޲���
			SAVE_OPLINE();
			// ��� op2 �� ������� ���� ��������Ϊδ����
			if (OP2_TYPE == IS_CV && UNEXPECTED(Z_TYPE_P(var) == IS_UNDEF)) {
				// ����p2�ı�������������δ����, ����δ��ʼ��zval
				ZVAL_UNDEFINED_OP2();
			}
			// zval ת�� �ִ�, û��try
			rope[0] = zval_get_string_func(var);
			// �ͷŲ�������ĸ��ӱ���
			FREE_OP2();
			// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
			ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
		}
	}
	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, ���ִ������б�����ӱ���
ZEND_VM_HANDLER(55, ZEND_ROPE_ADD, TMP, CONST|TMPVAR|CV, NUM)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zend_string **rope;
	zval *var;

	/* op1 and result are the same */
	rope = (zend_string**)EX_VAR(opline->op1.var);
	// �������Ϊ ����
	if (OP2_TYPE == IS_CONST) {
		// ��ȡzvalָ�룬UNUSED ���ͷ���null
		var = GET_OP2_ZVAL_PTR(BP_VAR_R);
		// ȡ���ִ���ӽ��б�
		rope[opline->extended_value] = Z_STR_P(var);
		// ����� �ɼ�������
		if (UNEXPECTED(Z_REFCOUNTED_P(var))) {
			// ���ô���+1
			Z_ADDREF_P(var);
		}
	// �������Ͳ��ǳ���
	} else {
		// ��ȡzvalָ��, UNUSED ����null
		var = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
		// ����������ִ�
		if (EXPECTED(Z_TYPE_P(var) == IS_STRING)) {
			// ��� op2 �� �������
			if (OP2_TYPE == IS_CV) {
				// ���ִ���ӵ� rope�б���
				// �������ô���. ����� ZVAL_COPY ��ͬ
				rope[opline->extended_value] = zend_string_copy(Z_STR_P(var));
			// ����op2 ���Ǳ������
			} else {
				// ���ִ���ӵ�rope�б���
				rope[opline->extended_value] = Z_STR_P(var);
			}
		// �������Ͳ����ִ�
		} else {
			// windows: �޲���
			SAVE_OPLINE();
			// ��� op2 �� ������� ���� ��������Ϊ δ����
			if (OP2_TYPE == IS_CV && UNEXPECTED(Z_TYPE_P(var) == IS_UNDEF)) {
				// ����p2�ı�������������δ����, ����δ��ʼ��zval
				ZVAL_UNDEFINED_OP2();
			}
			// zval ת�� �ִ�, û��try
			rope[opline->extended_value] = zval_get_string_func(var);
			// �ͷŲ�������ĸ��ӱ���
			FREE_OP2();
			// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
			ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
		}
	}
	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, ���Ԫ�ز� ��rope�б���ϳ��ִ�
ZEND_VM_HANDLER(56, ZEND_ROPE_END, TMP, CONST|TMPVAR|CV, NUM)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zend_string **rope;
	zval *var, *ret;
	uint32_t i;
	size_t len = 0;
	char *target;
	// ȡ��ropeλ��
	rope = (zend_string**)EX_VAR(opline->op1.var);
	// �������Ϊ ����
	if (OP2_TYPE == IS_CONST) {
		// ��ȡzvalָ�룬UNUSED ���ͷ���null
		var = GET_OP2_ZVAL_PTR(BP_VAR_R);
		// ȡ���ִ�����ӵ�rope�б���
		rope[opline->extended_value] = Z_STR_P(var);
		// ����� �ɼ�������
		if (UNEXPECTED(Z_REFCOUNTED_P(var))) {
			// ���ô���+1
			Z_ADDREF_P(var);
		}
	// ����
	} else {
		// ��ȡzvalָ��, UNUSED ����null
		var = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
		// ������ִ�
		if (EXPECTED(Z_TYPE_P(var) == IS_STRING)) {
			// ��� op2 �� �������
			if (OP2_TYPE == IS_CV) {
				// �ִ���ӵ�rope��
				// �������ô���. ����� ZVAL_COPY ��ͬ
				rope[opline->extended_value] = zend_string_copy(Z_STR_P(var));
			// ����
			} else {
				// �ִ���ӵ�rope��
				rope[opline->extended_value] = Z_STR_P(var);
			}
		// ����
		} else {
			// windows: �޲���
			SAVE_OPLINE();
			// ��� op2 �� ������� ���� ��������Ϊ δ����
			if (OP2_TYPE == IS_CV && UNEXPECTED(Z_TYPE_P(var) == IS_UNDEF)) {
				// ����p2�ı�������������δ����, ����δ��ʼ��zval
				ZVAL_UNDEFINED_OP2();
			}
			// zval ת�� �ִ�, û��try
			rope[opline->extended_value] = zval_get_string_func(var);
			// �ͷŲ�������ĸ��ӱ���
			FREE_OP2();
			// ������쳣
			if (UNEXPECTED(EG(exception))) {
				// ��������rope
				for (i = 0; i <= opline->extended_value; i++) {
					// �ͷ�����rope
					zend_string_release_ex(rope[i], 0);
				}
				// ���Ϊ δ����
				ZVAL_UNDEF(EX_VAR(opline->result.var));
				// ��Ҫ�� return
				HANDLE_EXCEPTION();
			}
		}
	}
	// ��������rope
	for (i = 0; i <= opline->extended_value; i++) {
		// ���㳤��
		len += ZSTR_LEN(rope[i]);
	}
	// ��ִ��������ȡ�� ָ����ŵı���
	ret = EX_VAR(opline->result.var);
	// �����ִ�
	ZVAL_STR(ret, zend_string_alloc(len, 0));
	// �����е��ִ�ָ��
	target = Z_STRVAL_P(ret);
	// ��������rope
	for (i = 0; i <= opline->extended_value; i++) {
		// һ��һ�εظ��Ƶ����ִ���
		memcpy(target, ZSTR_VAL(rope[i]), ZSTR_LEN(rope[i]));
		// �Ƶ���һ��
		target += ZSTR_LEN(rope[i]);
		// �ͷŸ��ƹ���rope
		zend_string_release_ex(rope[i], 0);
	}
	// ����
	*target = '\0';

	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, ��ȡ��.op1:���ͣ�op2������
ZEND_VM_HANDLER(109, ZEND_FETCH_CLASS, UNUSED|CLASS_FETCH, CONST|TMPVAR|UNUSED|CV, CACHE_SLOT)
{
	zval *class_name;
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// windows: �޲���
	SAVE_OPLINE();
	// ��� op2 ������δ����
	if (OP2_TYPE == IS_UNUSED) {
		// ͨ�����������Ͳ����࣬p1:������p2:����
		Z_CE_P(EX_VAR(opline->result.var)) = zend_fetch_class(NULL, opline->op1.num);
		// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
		ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
	// ���� �������Ϊ ����
	} else if (OP2_TYPE == IS_CONST) {
		// ͨ��ƫ���� ������ʱ�����л�ȡһ�� (void**) �еĵ�һ��Ԫ��
		zend_class_entry *ce = CACHED_PTR(opline->extended_value);

		// ��� �಻����
		if (UNEXPECTED(ce == NULL)) {
			// ��ȡzvalָ��, UNUSED ����null
			class_name = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
			// ��ȡ�࣬�Ҳ����ᱨ��p1:������p2:key��p3:flags
			ce = zend_fetch_class_by_name(Z_STR_P(class_name), Z_STR_P(class_name + 1), opline->op1.num);
			// ͨ��ƫ���� �� EX(run_time_cache) �л�ȡһ�� (void**) �еĵ�һ��Ԫ�أ���������ֵ
			CACHE_PTR(opline->extended_value, ce);
		}
		// �ҵ����ౣ�浽�����
		Z_CE_P(EX_VAR(opline->result.var)) = ce;
	// ����
	} else {
		// ��op2���ȡ����
		// ��ȡzvalָ��, UNUSED ����null
		class_name = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
// ������ת��ǩ try_class_name
ZEND_VM_C_LABEL(try_class_name):
		// ��������Ƕ���
		if (Z_TYPE_P(class_name) == IS_OBJECT) {
			// ���ҵ����� ���浽�����
			Z_CE_P(EX_VAR(opline->result.var)) = Z_OBJCE_P(class_name);
		// ���� ������������ִ�
		} else if (Z_TYPE_P(class_name) == IS_STRING) {
			// ͨ�����������Ͳ����࣬p1:������p2:����
			Z_CE_P(EX_VAR(opline->result.var)) = zend_fetch_class(Z_STR_P(class_name), opline->op1.num);
		// ���� �����op2����ͨ���� �� ������� ���� ��������������
		} else if ((OP2_TYPE & (IS_VAR|IS_CV)) && Z_TYPE_P(class_name) == IS_REFERENCE) {
			// ������
			class_name = Z_REFVAL_P(class_name);
			// ��ת��ָ����ǩ try_class_name
			ZEND_VM_C_GOTO(try_class_name);
		// ����
		} else {
			// ��� op2 �� ������� ��������Ϊ δ����
			if (OP2_TYPE == IS_CV && UNEXPECTED(Z_TYPE_P(class_name) == IS_UNDEF)) {
				// ����p2�ı�������������δ����, ����δ��ʼ��zval
				ZVAL_UNDEFINED_OP2();
				// ������쳣
				if (UNEXPECTED(EG(exception) != NULL)) {
					// ��Ҫ�� return
					HANDLE_EXCEPTION();
				}
			}
			// �������ͱ�������Ч���� �� �ִ�
			zend_throw_error(NULL, "Class name must be a valid object or a string");
		}
	}

	// �ͷŲ�������ĸ��ӱ���
	FREE_OP2();
	// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, �����෽��
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

	// windows: �޲���
	SAVE_OPLINE();
	// ��ȡzvalָ��, UNUSED ���� $this
	object = GET_OP1_OBJ_ZVAL_PTR_UNDEF(BP_VAR_R);

	// ��� OP2 ���ǳ���
	if (OP2_TYPE != IS_CONST) {
		// ��ȡzvalָ��, UNUSED ����null
		function_name = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	}

	// ��� OP2 ���ǳ��� ���� ������ ���� �ִ�
	if (OP2_TYPE != IS_CONST &&
	    UNEXPECTED(Z_TYPE_P(function_name) != IS_STRING)) {
		// Ϊ��break;
		do {
			// ���� ��� op2 ����ͨ������������ ���� op2 ����������
			if ((OP2_TYPE & (IS_VAR|IS_CV)) && Z_ISREF_P(function_name)) {
				// ������
				function_name = Z_REFVAL_P(function_name);
				// ����������� �ִ�
				if (EXPECTED(Z_TYPE_P(function_name) == IS_STRING)) {
					// ����
					break;
				}
			// ��� op2 �� ������� ���� ������ Ϊδ����
			} else if (OP2_TYPE == IS_CV && UNEXPECTED(Z_TYPE_P(function_name) == IS_UNDEF)) {
				// ����p2�ı�������������δ����, ����δ��ʼ��zval
				ZVAL_UNDEFINED_OP2();
				// ������쳣
				if (UNEXPECTED(EG(exception) != NULL)) {
					// �ͷŲ�������ĸ��ӱ���
					FREE_OP1();
					// ��Ҫ�� return
					HANDLE_EXCEPTION();
				}
			}
			// �״��������������ִ�
			zend_throw_error(NULL, "Method name must be a string");
			// �ͷŲ�������ĸ��ӱ���
			FREE_OP2();
			// �ͷŲ�������ĸ��ӱ���
			FREE_OP1();
			// ��Ҫ�� return
			HANDLE_EXCEPTION();
		} while (0);
	}
	
	// ��� op1 ������δ����
	if (OP1_TYPE == IS_UNUSED) {
		// ȡ������
		obj = Z_OBJ_P(object);
	// ����
	} else {
		do {
			// ��� OP1 ���ǳ��� ���� ����������object
			if (OP1_TYPE != IS_CONST && EXPECTED(Z_TYPE_P(object) == IS_OBJECT)) {
				// ȡ������
				obj = Z_OBJ_P(object);
			// ����OP1�ǳ��� �� object���Ƕ���
			} else {
				// ��� op1 ����ͨ������������ ���� op1����������
				if ((OP1_TYPE & (IS_VAR|IS_CV)) && EXPECTED(Z_ISREF_P(object))) {
					// ȡ������ʵ��
					zend_reference *ref = Z_REF_P(object);
					// ����ʵ���е�zval
					object = &ref->val;
					// ������Ƕ���
					if (EXPECTED(Z_TYPE_P(object) == IS_OBJECT)) {
						// ȡ������
						obj = Z_OBJ_P(object);
						// ���op1�Ǳ���
						if (OP1_TYPE & IS_VAR) {
							// ���ô���-1�����Ϊ0
							if (UNEXPECTED(GC_DELREF(ref) == 0)) {
								// �ͷ� ����ʵ��
								efree_size(ref, sizeof(zend_reference));
							// ����
							} else {
								// ���ô���+1
								Z_ADDREF_P(object);
							}
						}
						// ����
						break;
					}
				}
				// ��� op1 �� ������� ���Ҷ���ֵΪδ����
				if (OP1_TYPE == IS_CV && UNEXPECTED(Z_TYPE_P(object) == IS_UNDEF)) {
					// ����p1�ı�������������δ����, ����δ��ʼ��zval
					object = ZVAL_UNDEFINED_OP1();
					// ������쳣
					if (UNEXPECTED(EG(exception) != NULL)) {
						// ��� OP2 ���ǳ���
						if (OP2_TYPE != IS_CONST) {
							// �ͷŲ�������ĸ��ӱ���
							FREE_OP2();
						}
						// ��Ҫ�� return
						HANDLE_EXCEPTION();
					}
				}
				// �������Ϊ ����
				if (OP2_TYPE == IS_CONST) {
					// ��ȡzvalָ��, UNUSED ����null
					function_name = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
				}
				// ������Ч�ĵ���
				zend_invalid_method_call(object, function_name);
				// �ͷŲ�������ĸ��ӱ���
				FREE_OP2();
				// �ͷŲ�������ĸ��ӱ���
				FREE_OP1();
				// ��Ҫ�� return
				HANDLE_EXCEPTION();
			}
		} while (0);
	}

	// ���ù�����
	called_scope = obj->ce;
	// �������Ϊ ���� ���� ����ǵ��ù�����
	// ͨ��ƫ���� ������ʱ�����л�ȡһ�� (void**) �еĵ�һ��Ԫ��
	if (OP2_TYPE == IS_CONST &&
	    EXPECTED(CACHED_PTR(opline->result.num) == called_scope)) {
		// �ҵ�����
		// ͨ��ƫ���� ������ʱ�����л�ȡһ�� (void**) �еĵ�һ��Ԫ��
		fbc = CACHED_PTR(opline->result.num + sizeof(void*));
	// ����
	} else {
		zend_object *orig_obj = obj;

		// �������Ϊ ����
		if (OP2_TYPE == IS_CONST) {
			// ��ȡzvalָ��, UNUSED ����null
			function_name = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
		}

		// ���ȣ����ҷ���
		/* First, locate the function. */
		// �� ������ ���� ����
		fbc = obj->handlers->get_method(&obj, Z_STR_P(function_name), ((OP2_TYPE == IS_CONST) ? (RT_CONSTANT(opline, opline->op2) + 1) : NULL));
		// ���û�ҵ�
		if (UNEXPECTED(fbc == NULL)) {
			// ���û���쳣
			if (EXPECTED(!EG(exception))) {
				// ��������δ����ķ���
				zend_undefined_method(obj->ce, Z_STR_P(function_name));
			}
			// �ͷŲ�������ĸ��ӱ���
			FREE_OP2();
			// ���ô���-1�����Ϊ0
			if ((OP1_TYPE & (IS_VAR|IS_TMP_VAR)) && GC_DELREF(orig_obj) == 0) {
				// �ͷ�ָ���Ķ��󣬲����� objects_store �е�ָ��
				zend_objects_store_del(orig_obj);
			}
			// ��Ҫ�� return
			HANDLE_EXCEPTION();
		}
		
		// �������Ϊ ���� ���� ����������ͨ���������� Ҳ���ǲ��ɻ��棩 ���� ������ԭ����
		if (OP2_TYPE == IS_CONST &&
		    EXPECTED(!(fbc->common.fn_flags & (ZEND_ACC_CALL_VIA_TRAMPOLINE|ZEND_ACC_NEVER_CACHE))) &&
		    EXPECTED(obj == orig_obj)) {
			// �ҵ�����ʱ����� p1 ��λ�ã���p2 ��p3���α����ȥ	
			CACHE_POLYMORPHIC_PTR(opline->result.num, called_scope, fbc);
		}
		
		// ���op1 �� ���� �� ��ʱ����  ���� ������ԭ���Ķ���
		if ((OP1_TYPE & (IS_VAR|IS_TMP_VAR)) && UNEXPECTED(obj != orig_obj)) {
			// �������ô���
			GC_ADDREF(obj); /* For $this pointer */
			// ���ô���-1�����Ϊ0
			if (GC_DELREF(orig_obj) == 0) {
				// �ͷ�ָ���Ķ��󣬲����� objects_store �е�ָ��
				zend_objects_store_del(orig_obj);
			}
		}
		
		// ������û����庯�� ���� û�� ����ʱ����
		if (EXPECTED(fbc->type == ZEND_USER_FUNCTION) && UNEXPECTED(!RUN_TIME_CACHE(&fbc->op_array))) {
			// Ϊop_array������ ��ʼ������ʱ����
			init_func_run_time_cache(&fbc->op_array);
		}
	}

	// ��� OP2 ���ǳ���
	if (OP2_TYPE != IS_CONST) {
		// �ͷŲ�������ĸ��ӱ���
		FREE_OP2();
	}

	// ������Ϣ ��Ƕ�׵��á�+����$this��
	call_info = ZEND_CALL_NESTED_FUNCTION | ZEND_CALL_HAS_THIS;
	// ��� �Ǿ�̬����
	if (UNEXPECTED((fbc->common.fn_flags & ZEND_ACC_STATIC) != 0)) {
		// ���ô���-1�����Ϊ0
		if ((OP1_TYPE & (IS_VAR|IS_TMP_VAR)) && GC_DELREF(obj) == 0) {
			// �ͷ�ָ���Ķ��󣬲����� objects_store �е�ָ��
			zend_objects_store_del(obj);
			// ������쳣
			if (UNEXPECTED(EG(exception))) {
				// ��Ҫ�� return
				HANDLE_EXCEPTION();
			}
		}
		// ���þ�̬����
		/* call static method */
		obj = (zend_object*)called_scope;
		// ������Ϣ����Ƕ�׵��á�
		call_info = ZEND_CALL_NESTED_FUNCTION;
	// ������Ǿ�̬���� ���� op1 �ǣ����� �� ��ʱ���� �� ���������
	} else if (OP1_TYPE & (IS_VAR|IS_TMP_VAR|IS_CV)) {
		// ��� op1 �� �������
		if (OP1_TYPE == IS_CV) {
			// �������ô���
			GC_ADDREF(obj); /* For $this pointer */
		}
		// ����������Ա���Ӹı䣨���磬��������õ�ʱ��
		/* CV may be changed indirectly (e.g. when it's a reference) */
		// ������Ϣ ��Ƕ�׵��á�+����$this��+����Ҫ�ͷ�$this��
		call_info = ZEND_CALL_NESTED_FUNCTION | ZEND_CALL_HAS_THIS | ZEND_CALL_RELEASE_THIS;
	}
	// ing3, ��ʼ�������� ��ջ����һ��ִ�����ݣ��ռ䲻������չ��С
	// p1:������Ϣ��p2:������p3:����������p4:����������
	call = zend_vm_stack_push_call_frame(call_info,
		fbc, opline->extended_value, obj);
	// ����ǰһ��ִ������
	call->prev_execute_data = EX(call);
	// ���µ���ִ������
	EX(call) = call;

	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, ��ʼ����̬��������
ZEND_VM_HANDLER(113, ZEND_INIT_STATIC_METHOD_CALL, UNUSED|CLASS_FETCH|CONST|VAR, CONST|TMPVAR|UNUSED|CONSTRUCTOR|CV, NUM|CACHE_SLOT)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *function_name;
	zend_class_entry *ce;
	uint32_t call_info;
	zend_function *fbc;
	zend_execute_data *call;

	// windows: �޲���
	SAVE_OPLINE();

	// ���op1�ǳ���
	if (OP1_TYPE == IS_CONST) {
		/* no function found. try a static method in class */
		// ͨ��ƫ���� ������ʱ�����л�ȡһ�� (void**) �еĵ�һ��Ԫ��
		ce = CACHED_PTR(opline->result.num);
		// ��� �಻����
		if (UNEXPECTED(ce == NULL)) {
			// ��ȡ�࣬�Ҳ����ᱨ��p1:������p2:key��p3:flags
			ce = zend_fetch_class_by_name(Z_STR_P(RT_CONSTANT(opline, opline->op1)), Z_STR_P(RT_CONSTANT(opline, opline->op1) + 1), ZEND_FETCH_CLASS_DEFAULT | ZEND_FETCH_CLASS_EXCEPTION);
			// ��� �಻����
			if (UNEXPECTED(ce == NULL)) {
				// �ͷŲ�������ĸ��ӱ���
				FREE_OP2();
				// ��Ҫ�� return
				HANDLE_EXCEPTION();
			}
			// ��� OP2 ���ǳ���
			if (OP2_TYPE != IS_CONST) {
				// ͨ��ƫ���� �� EX(run_time_cache) �л�ȡһ�� (void**) �еĵ�һ��Ԫ�أ���������ֵ
				CACHE_PTR(opline->result.num, ce);
			}
		}
	// ���� ��� op1 ������δ����
	} else if (OP1_TYPE == IS_UNUSED) {
		// ͨ�����������Ͳ����࣬p1:������p2:����
		ce = zend_fetch_class(NULL, opline->op1.num);
		// ��� �಻����
		if (UNEXPECTED(ce == NULL)) {
			// �ͷŲ�������ĸ��ӱ���
			FREE_OP2();
			// ��Ҫ�� return
			HANDLE_EXCEPTION();
		}
	// ����
	} else {
		// ȡ�� �����
		ce = Z_CE_P(EX_VAR(opline->op1.var));
	}

	// ������� op ���Ͷ�Ϊ���� ���� ���Դӻ�����ȡ�ô˺���
	// ͨ��ƫ���� ������ʱ�����л�ȡһ�� (void**) �еĵ�һ��Ԫ��
	if (OP1_TYPE == IS_CONST &&
	    OP2_TYPE == IS_CONST &&
	    EXPECTED((fbc = CACHED_PTR(opline->result.num + sizeof(void*))) != NULL)) {
		// ʲôҲ����
		/* nothing to do */
	
	// ���op1���ǳ�����op2�ǳ��� ���� ��������� �ҵ����� ce
	// ͨ��ƫ���� ������ʱ�����л�ȡһ�� (void**) �еĵ�һ��Ԫ��
	} else if (OP1_TYPE != IS_CONST &&
	           OP2_TYPE == IS_CONST &&
	           EXPECTED(CACHED_PTR(opline->result.num) == ce)) {
		// ͨ��ƫ���� ������ʱ�����л�ȡһ�� (void**) �еĵ�һ��Ԫ��
		fbc = CACHED_PTR(opline->result.num + sizeof(void*));
		
	// ���op2�����Ͳ���δʹ��
	} else if (OP2_TYPE != IS_UNUSED) {
		// ��ȡzvalָ��, UNUSED ����null
		function_name = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
		// ��� OP2 ���ǳ���
		if (OP2_TYPE != IS_CONST) {
			// ��������������ִ�
			if (UNEXPECTED(Z_TYPE_P(function_name) != IS_STRING)) {
				// Ϊ��break;
				do {
					// ���� ��� op2 ����ͨ������������ ���� op2 ����������
					if (OP2_TYPE & (IS_VAR|IS_CV) && Z_ISREF_P(function_name)) {
						// ������
						function_name = Z_REFVAL_P(function_name);
						// ������������ִ�
						if (EXPECTED(Z_TYPE_P(function_name) == IS_STRING)) {
							// ����
							break;
						}
					// ��� op2 �� ������� ���� ������ Ϊδ����
					} else if (OP2_TYPE == IS_CV && UNEXPECTED(Z_TYPE_P(function_name) == IS_UNDEF)) {
						// ����p2�ı�������������δ����, ����δ��ʼ��zval
						ZVAL_UNDEFINED_OP2();
						// ������쳣
						if (UNEXPECTED(EG(exception) != NULL)) {
							// ��Ҫ�� return
							HANDLE_EXCEPTION();
						}
					}
					// �����������������ִ�
					zend_throw_error(NULL, "Method name must be a string");
					// �ͷŲ�������ĸ��ӱ���
					FREE_OP2();
					// ��Ҫ�� return
					HANDLE_EXCEPTION();
				} while (0);
			}
		}

		// ����л�ȡ��̬�����ķ���
		if (ce->get_static_method) {
			// ��ȡ��̬����
			fbc = ce->get_static_method(ce, Z_STR_P(function_name));
		// ����
		} else {
			// ʹ�ñ�׼������ȡ��̬������p1:�࣬p2:��������p3:key
				// op2�ǳ���ʱ��key
			fbc = zend_std_get_static_method(ce, Z_STR_P(function_name), ((OP2_TYPE == IS_CONST) ? (RT_CONSTANT(opline, opline->op2) + 1) : NULL));
		}
		// ���û��ȡ������
		if (UNEXPECTED(fbc == NULL)) {
			// ���û���쳣
			if (EXPECTED(!EG(exception))) {
				// ��������δ����ķ���
				zend_undefined_method(ce, Z_STR_P(function_name));
			}
			// �ͷŲ�������ĸ��ӱ���
			FREE_OP2();
			// ��Ҫ�� return
			HANDLE_EXCEPTION();
		}
		// ���op2�ǳ��� ���� ��û�С�ͨ���������á��򡾲����桿��ǣ� ���ң�������trait�
		if (OP2_TYPE == IS_CONST &&
		    EXPECTED(!(fbc->common.fn_flags & (ZEND_ACC_CALL_VIA_TRAMPOLINE|ZEND_ACC_NEVER_CACHE))) &&
			EXPECTED(!(fbc->common.scope->ce_flags & ZEND_ACC_TRAIT))) {
			// �ҵ�����ʱ����� p1 ��λ�ã���p2 ��p3���α����ȥ
			CACHE_POLYMORPHIC_PTR(opline->result.num, ce, fbc);
		}
		// ������û����庯�� ���� û�� ����ʱ����
		if (EXPECTED(fbc->type == ZEND_USER_FUNCTION) && UNEXPECTED(!RUN_TIME_CACHE(&fbc->op_array))) {
			// Ϊop_array������ ��ʼ������ʱ����
			init_func_run_time_cache(&fbc->op_array);
		}
		// ��� OP2 ���ǳ���
		if (OP2_TYPE != IS_CONST) {
			// �ͷŲ�������ĸ��ӱ���
			FREE_OP2();
		}
	// ���4��	
	// ����
	} else {
		// �����û�й��췽��
		if (UNEXPECTED(ce->constructor == NULL)) {
			// �����޷����ù��췽��
			zend_throw_error(NULL, "Cannot call constructor");
			// ��Ҫ�� return
			HANDLE_EXCEPTION();
		}
		// $this�Ƕ��� ���� ���췽�������ڴ��� ���� ���췽����˽�е�
		if (Z_TYPE(EX(This)) == IS_OBJECT && Z_OBJ(EX(This))->ce != ce->constructor->common.scope && (ce->constructor->common.fn_flags & ZEND_ACC_PRIVATE)) {
			// �����޷�����˽�й��췽��
			zend_throw_error(NULL, "Cannot call private %s::__construct()", ZSTR_VAL(ce->name));
			// ��Ҫ�� return
			HANDLE_EXCEPTION();
		}
		// ���Ե���
		// ȡ�����췽��
		fbc = ce->constructor;
		// ������û����庯�� ���� û�� ����ʱ����
		if (EXPECTED(fbc->type == ZEND_USER_FUNCTION) && UNEXPECTED(!RUN_TIME_CACHE(&fbc->op_array))) {
			// Ϊop_array������ ��ʼ������ʱ����
			init_func_run_time_cache(&fbc->op_array);
		}
	}

	// ������Ǿ�̬����
	if (!(fbc->common.fn_flags & ZEND_ACC_STATIC)) {
		// ���$this�Ƕ��� ���� $this����ce��
		if (Z_TYPE(EX(This)) == IS_OBJECT && instanceof_function(Z_OBJCE(EX(This)), ce)) {
			// $this ����ʹ��?
			ce = (zend_class_entry*)Z_OBJ(EX(This));
			// ������Ϣ����� ����$this���͡�Ƕ�׵��á��������
			call_info = ZEND_CALL_NESTED_FUNCTION | ZEND_CALL_HAS_THIS;
		// ����
		} else {
			// ������̬���÷Ǿ�̬����
			zend_non_static_method_call(fbc);
			// ��Ҫ�� return
			HANDLE_EXCEPTION();
		}
	// �����Ǿ�̬����
	} else {
		// ǰһ���������� ZEND_FETCH_CLASS
		/* previous opcode is ZEND_FETCH_CLASS */
		// ��� op1 ������δ���� ���� ��ʹ����parent��self�ؼ��֣�
		if (OP1_TYPE == IS_UNUSED
		 && ((opline->op1.num & ZEND_FETCH_CLASS_MASK) == ZEND_FETCH_CLASS_PARENT ||
		     (opline->op1.num & ZEND_FETCH_CLASS_MASK) == ZEND_FETCH_CLASS_SELF)) {
			// ��� $this�Ƕ���
			if (Z_TYPE(EX(This)) == IS_OBJECT) {
				// ȡ�� �������
				ce = Z_OBJCE(EX(This));
			// ����, $this���Ƕ���
			} else {
				// ʹ�� $this ��
				ce = Z_CE(EX(This));
			}
		}
		// ������Ϣ��Ƕ�׵���
		call_info = ZEND_CALL_NESTED_FUNCTION;
	}

	// ing3, ��ʼ�������� ��ջ����һ��ִ�����ݣ��ռ䲻������չ��С
	// p1:������Ϣ��p2:������p3:����������p4:����������
	call = zend_vm_stack_push_call_frame(call_info,
		fbc, opline->extended_value, ce);
	// ����ǰһ��ִ������
	call->prev_execute_data = EX(call);
	// �ѵ��õ�ִ�����ݣ��ĳ���ִ������
	EX(call) = call;

	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, ʹ�����Ƶ��ú���
ZEND_VM_HOT_HANDLER(59, ZEND_INIT_FCALL_BY_NAME, ANY, CONST, NUM|CACHE_SLOT)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zend_function *fbc;
	zval *function_name, *func;
	zend_execute_data *call;

	// ͨ��ƫ���� ������ʱ�����л�ȡһ�� (void**) �еĵ�һ��Ԫ��
	fbc = CACHED_PTR(opline->result.num);
	// ���û��ȡ������
	if (UNEXPECTED(fbc == NULL)) {
		// op2�Ǻ�����
		// ���� (p2).zv��p1:opline,p2:node
		function_name = (zval*)RT_CONSTANT(opline, opline->op2);
		// ȡ������
		func = zend_hash_find_known_hash(EG(function_table), Z_STR_P(function_name+1));
		// ���û�д˺���
		if (UNEXPECTED(func == NULL)) {
			// ����û����δ����ĺ���
			// ���÷��� zend_undefined_function_helper
			ZEND_VM_DISPATCH_TO_HELPER(zend_undefined_function_helper);
		}
		// ȡ������
		fbc = Z_FUNC_P(func);
		// ������û����� ���� û������ʱ����
		if (EXPECTED(fbc->type == ZEND_USER_FUNCTION) && UNEXPECTED(!RUN_TIME_CACHE(&fbc->op_array))) {
			// Ϊop_array������ ��ʼ������ʱ����
			init_func_run_time_cache(&fbc->op_array);
		}
		// ͨ��ƫ���� �� EX(run_time_cache) �л�ȡһ�� (void**) �еĵ�һ��Ԫ�أ���������ֵ
		CACHE_PTR(opline->result.num, fbc);
	}
	// ing3, ��ʼ�������� ��ջ����һ��ִ�����ݣ��ռ䲻������չ��С
	// p1:������Ϣ��p2:������p3:����������p4:����������
	call = _zend_vm_stack_push_call_frame(ZEND_CALL_NESTED_FUNCTION,
		fbc, opline->extended_value, NULL);
	// ��ִ�����ݵ� ����ǰһ��ִ�����ݣ�������õ�ִ������
	call->prev_execute_data = EX(call);
	// �����ڵ��õ�ִ�����ݣ��ĳ���ִ������
	EX(call) = call;

	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing2, ��ʼ����̬����ִ������
ZEND_VM_HANDLER(128, ZEND_INIT_DYNAMIC_CALL, ANY, CONST|TMPVAR|CV, NUM)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *function_name;
	zend_execute_data *call;

	// windows: �޲���
	SAVE_OPLINE();
	// ��ȡzvalָ��, UNUSED ����null
	function_name = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);

// ������ת��ǩ try_function_name
ZEND_VM_C_LABEL(try_function_name):
	// ��� OP2 ���ǳ��� ���� ������ ���ִ�
	if (OP2_TYPE != IS_CONST && EXPECTED(Z_TYPE_P(function_name) == IS_STRING)) {
		// ͨ���ִ��������� ִ�����ݣ���Ҫ���������ġ�p1:���÷���������������̬����������p2:��������
		call = zend_init_dynamic_call_string(Z_STR_P(function_name), opline->extended_value);
	
	// ���� ��� OP2 ���ǳ��� ���� ������ �Ƕ���
	} else if (OP2_TYPE != IS_CONST && EXPECTED(Z_TYPE_P(function_name) == IS_OBJECT)) {
		// �������ñհ�����ٱհ���ִ�����ݡ�p1:������ȡ�հ��Ķ���p2:��������
		call = zend_init_dynamic_call_object(Z_OBJ_P(function_name), opline->extended_value);
	
	// ���� ���������������
	} else if (EXPECTED(Z_TYPE_P(function_name) == IS_ARRAY)) {
		// �����飬��ʼ����̬����ִ�����ݡ�p1:�������飬p2:��������
		call = zend_init_dynamic_call_array(Z_ARRVAL_P(function_name), opline->extended_value);
	
	// ���� ��� op2 ����ͨ������������ ���� op2 ����������
	} else if ((OP2_TYPE & (IS_VAR|IS_CV)) && EXPECTED(Z_TYPE_P(function_name) == IS_REFERENCE)) {
		// ������
		function_name = Z_REFVAL_P(function_name);
		// ��ת��ָ����ǩ try_function_name
		ZEND_VM_C_GOTO(try_function_name);
	
	// �����޷����ã�����
	} else {
		// ��� op2 �� ������� ���� ������ Ϊδ����
		if (OP2_TYPE == IS_CV && UNEXPECTED(Z_TYPE_P(function_name) == IS_UNDEF)) {
			// ����p2�ı�������������δ����, ����δ��ʼ��zval
			function_name = ZVAL_UNDEFINED_OP2();
			// ������쳣
			if (UNEXPECTED(EG(exception) != NULL)) {
				// ��Ҫ�� return
				HANDLE_EXCEPTION();
			}
		}
		// �״������͵�ֵ���ɵ���
		zend_throw_error(NULL, "Value of type %s is not callable",
			zend_zval_type_name(function_name));
		// û��ִ������
		call = NULL;
	}

	// ���op2�� ���� �� ��ʱ����
	if (OP2_TYPE & (IS_VAR|IS_TMP_VAR)) {
		// �ͷŲ�������ĸ��ӱ���
		FREE_OP2();
		// �������쳣
		if (UNEXPECTED(EG(exception))) {
			// �����ִ������
			if (call) {
				// �����ͨ������������
				if (call->func->common.fn_flags & ZEND_ACC_CALL_VIA_TRAMPOLINE) {
					// �ͷź�����
					zend_string_release_ex(call->func->common.function_name, 0);
					// �ͷŵ�������
					zend_free_trampoline(call->func);
				}
				// �ͷŵ��ÿ�ܣ�ִ�����ݣ���p1:ִ������
				zend_vm_stack_free_call_frame(call);
			}
			// ��Ҫ�� return
			HANDLE_EXCEPTION();
		}
	// �������û��ִ������
	} else if (!call) {
		// ��Ҫ�� return
		HANDLE_EXCEPTION();
	}

	// ����ǰһ��ִ�����ݣ�
	call->prev_execute_data = EX(call);
	// ���ڵ��õ�ִ�����ݣ�
	EX(call) = call;

	// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing2, ��ʼ���û�����ִ������
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

	// windows: �޲���
	SAVE_OPLINE();
	// ��ȡzvalָ�룬UNUSED ���ͷ���null
	function_name = GET_OP2_ZVAL_PTR(BP_VAR_R);
	// ���˵����� ��ǰָ��ִ�������� �Ƿ����
	// p1:������Ϣ��p2:����, p3:���鼶��p4:���ص����ִ���p5:fcc,�ؼ����� ��p6:���ش�����Ϣ
	if (zend_is_callable_ex(function_name, NULL, 0, NULL, &fcc, &error)) {
		// �������д���
		ZEND_ASSERT(!error);
		// ����е��ú���
		func = fcc.function_handler;
		// ��
		object_or_called_scope = fcc.called_scope;
		// ��������Ǳհ�
		if (func->common.fn_flags & ZEND_ACC_CLOSURE) {
			/* Delay closure destruction until its invocation */
			// �������ô���
			GC_ADDREF(ZEND_CLOSURE_OBJECT(func));
			// ��� ���ñհ� ���
			call_info |= ZEND_CALL_CLOSURE;
			// ��� ���� �Ǽٱհ�
			if (func->common.fn_flags & ZEND_ACC_FAKE_CLOSURE) {
				// ������Ϣ����� �ٱհ����
				call_info |= ZEND_CALL_FAKE_CLOSURE;
			}
			// ���������Ϣ�������� �����ö���
			if (fcc.object) {
				// �����ö���
				object_or_called_scope = fcc.object;
				// ������Ϣ ��� ����$this�� ���
				call_info |= ZEND_CALL_HAS_THIS;
			}
		// ������Ǳհ� ���� �� �����ö���
		} else if (fcc.object) {
			// �������ô���
			GC_ADDREF(fcc.object); /* For $this pointer */
			// �����ö���
			object_or_called_scope = fcc.object;
			// ��� ����$this���͡���Ҫ�ͷ�$this�����
			call_info |= ZEND_CALL_RELEASE_THIS | ZEND_CALL_HAS_THIS;
		}

		// �ͷŲ�������ĸ��ӱ���
		FREE_OP2();
		// ���op2�� ��ʱ���� �� ��ͨ���� ������ �쳣
		if ((OP2_TYPE & (IS_TMP_VAR|IS_VAR)) && UNEXPECTED(EG(exception))) {
			// ��� ������Ϣ�� �� ���ñհ����
			if (call_info & ZEND_CALL_CLOSURE) {
				// �ͷűհ�����
				// �� _zend_closure.zend_function �ҵ� _zend_closure ��ͷ��
				zend_object_release(ZEND_CLOSURE_OBJECT(func));
			// �����Ҫ�ͷ� ��$this��
			} else if (call_info & ZEND_CALL_RELEASE_THIS) {
				// �ͷŵ�����Ϣ�����е� object
				zend_object_release(fcc.object);
			}
			// ��Ҫ�� return
			HANDLE_EXCEPTION();
		}
		// ������û����庯�� ���� û�� ����ʱ����
		if (EXPECTED(func->type == ZEND_USER_FUNCTION) && UNEXPECTED(!RUN_TIME_CACHE(&func->op_array))) {
			// Ϊop_array������ ��ʼ������ʱ����
			init_func_run_time_cache(&func->op_array);
		}
	// ����������ò�����
	} else {
		// ��������1 ������һ����Ч�� callback
		// ���� (p2).zv��p1:opline,p2:node
		zend_type_error("%s(): Argument #1 ($callback) must be a valid callback, %s", Z_STRVAL_P(RT_CONSTANT(opline, opline->op1)), error);
		// �ͷŴ���
		efree(error);
		// �ͷŲ�������ĸ��ӱ���
		FREE_OP2();
		// ��Ҫ�� return
		HANDLE_EXCEPTION();
	}

	// ��ʼ�������� ��ջ����һ��ִ�����ݣ��ռ䲻������չ��С
	// p1:������Ϣ��p2:������p3:����������p4:����������
	call = zend_vm_stack_push_call_frame(call_info,
		func, opline->extended_value, object_or_called_scope);
	// ����ǰһ��ִ������
	call->prev_execute_data = EX(call);
	// �µ�ִ������ ��Ϊ����ִ������
	EX(call) = call;

	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing2, ��ʼ���� �������ռ�� ���Ƶ��ú���
ZEND_VM_HOT_HANDLER(69, ZEND_INIT_NS_FCALL_BY_NAME, ANY, CONST, NUM|CACHE_SLOT)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *func_name;
	zval *func;
	zend_function *fbc;
	zend_execute_data *call;

	// ͨ��ƫ���� ������ʱ�����л�ȡһ�� (void**) �еĵ�һ��Ԫ��
	fbc = CACHED_PTR(opline->result.num);
	// ���û�ҵ�����
	if (UNEXPECTED(fbc == NULL)) {
		// ������
		// ���� (p2).zv��p1:opline,p2:node
		func_name = (zval *)RT_CONSTANT(opline, opline->op2);
		// ʹ����һ��������ȡ�ú���ʵ��
		func = zend_hash_find_known_hash(EG(function_table), Z_STR_P(func_name + 1));
		// ����Ҳ���
		if (func == NULL) {
			// ʹ����2�������� ȡ�ú���ʵ��
			func = zend_hash_find_known_hash(EG(function_table), Z_STR_P(func_name + 2));
			// ����Ҳ���
			if (UNEXPECTED(func == NULL)) {
				// ����û����δ����ĺ���
				// ���÷��� zend_undefined_function_helper
				ZEND_VM_DISPATCH_TO_HELPER(zend_undefined_function_helper);
			}
		}
		// ȡ������ʵ��
		fbc = Z_FUNC_P(func);
		// ������û����庯�� ���� û�� ����ʱ����
		if (EXPECTED(fbc->type == ZEND_USER_FUNCTION) && UNEXPECTED(!RUN_TIME_CACHE(&fbc->op_array))) {
			// Ϊop_array������ ��ʼ������ʱ����
			init_func_run_time_cache(&fbc->op_array);
		}
		// ͨ��ƫ���� �� EX(run_time_cache) �л�ȡһ�� (void**) �еĵ�һ��Ԫ�أ���������ֵ
		CACHE_PTR(opline->result.num, fbc);
	}

	// ing3, ��ʼ�������� ��ջ����һ��ִ�����ݣ��ռ䲻������չ��С
	// p1:������Ϣ��p2:������p3:����������p4:����������
	call = _zend_vm_stack_push_call_frame(ZEND_CALL_NESTED_FUNCTION,
		fbc, opline->extended_value, NULL);
	// ����ǰһ��ִ������
	call->prev_execute_data = EX(call);
	// ��ִ������Ϊ�����õ�ִ������
	EX(call) = call;

	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing2, ��ʼ����������
ZEND_VM_HOT_HANDLER(61, ZEND_INIT_FCALL, NUM, CONST, NUM|CACHE_SLOT)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *fname;
	zval *func;
	zend_function *fbc;
	zend_execute_data *call;

	// ͨ��ƫ���� ������ʱ�����л�ȡһ�� (void**) �еĵ�һ��Ԫ��
	fbc = CACHED_PTR(opline->result.num);
	// ���û�к��� ʵ��
	if (UNEXPECTED(fbc == NULL)) {
		// ȡ��op2 ������
		// ���� (p2).zv��p1:opline,p2:node
		fname = (zval*)RT_CONSTANT(opline, opline->op2);
		// ʹ�ú��������Һ���
		func = zend_hash_find_known_hash(EG(function_table), Z_STR_P(fname));
		// �����ҵ�
		ZEND_ASSERT(func != NULL && "Function existence must be checked at compile time");
		// ȡ������ʵ��
		fbc = Z_FUNC_P(func);
		// ������û����庯�� ���� û�� ����ʱ����
		if (EXPECTED(fbc->type == ZEND_USER_FUNCTION) && UNEXPECTED(!RUN_TIME_CACHE(&fbc->op_array))) {
			// Ϊop_array������ ��ʼ������ʱ����
			init_func_run_time_cache(&fbc->op_array);
		}
		// ͨ��ƫ���� �� EX(run_time_cache) �л�ȡһ�� (void**) �еĵ�һ��Ԫ�أ���������ֵ
		CACHE_PTR(opline->result.num, fbc);
	}

	// ��ʼ�������� ��ջ����һ��ִ�����ݣ��ռ䲻������չ��С
	// p1:ʹ�ô�С��p2:������Ϣ��p3:������p4:����������p5:����������
	call = _zend_vm_stack_push_call_frame_ex(
		opline->op1.num, ZEND_CALL_NESTED_FUNCTION,
		fbc, opline->extended_value, NULL);
	// ����ǰһ��ִ������
	call->prev_execute_data = EX(call);
	// ��ִ��������Ϊ���õ�ִ������
	EX(call) = call;

	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing2, �������ú���
ZEND_VM_HOT_HANDLER(129, ZEND_DO_ICALL, ANY, ANY, SPEC(RETVAL,OBSERVER))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	// ���ڵ��õ�ִ������
	zend_execute_data *call = EX(call);
	// ���ڵ��õĺ���
	zend_function *fbc = call->func;
	zval *ret;
	zval retval;

	// windows: �޲���
	SAVE_OPLINE();
	// ���ڵ��õ�ִ������ ���л���ǰһ��ִ������
	EX(call) = call->prev_execute_data;

	// ����ǰһ��ִ������
	call->prev_execute_data = execute_data;
	// ת�������õ�ִ������
	EG(current_execute_data) = call;

// ������
#if ZEND_DEBUG
	bool should_throw = zend_internal_call_should_throw(fbc, call);
#endif

	// ��� ���������������Ч(����IS_UNUSED) ��������Ľ�� ����ʱ����
	ret = RETURN_VALUE_USED(opline) ? EX_VAR(opline->result.var) : &retval;
	// ����ÿ�
	ZVAL_NULL(ret);
	// ��ʼ�۲��ߵ���
	// "/ZEND_OBSERVER_FCALL_BEGIN\(\s*(.*)\s*\)/" => isset($extra_spec['OBSERVER']) ?            ($extra_spec['OBSERVER'] == 0 ? "" : "zend_observer_fcall_begin(\\1)")            : "",
	ZEND_OBSERVER_FCALL_BEGIN(call);
	// �������ú���
	fbc->internal_function.handler(call, ret);

// ������
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
	// ����õ��˹۲��ߣ������۲��ߵ���
	// "/ZEND_OBSERVER_FCALL_END\(\s*([^,]*)\s*,\s*(.*)\s*\)/" => isset($extra_spec['OBSERVER']) ?            ($extra_spec['OBSERVER'] == 0 ? "" : "zend_observer_fcall_end(\\1, \\2)")            : "",
	ZEND_OBSERVER_FCALL_END(call, EG(exception) ? NULL : ret);

	// �л�����ִ������
	EG(current_execute_data) = execute_data;
	// �ͷ�����ִ�������е� ��������
	zend_vm_stack_free_args(call);
	
	// ��ȡ������Ϣ
	uint32_t call_info = ZEND_CALL_INFO(call);
	// ����б�ǡ��ж��������������򡾵��÷��䡿
	if (UNEXPECTED(call_info & (ZEND_CALL_HAS_EXTRA_NAMED_PARAMS|ZEND_CALL_ALLOCATED))) {
		// ����б�ǡ��ж�������������
		if (call_info & ZEND_CALL_HAS_EXTRA_NAMED_PARAMS) {
			// �ͷ� ������������
			zend_free_extra_named_params(call->extra_named_params);
		}
		// �ͷŵ��ÿ�ܣ�ִ�����ݣ���p1:������Ϣ��p2:ִ������
		zend_vm_stack_free_call_frame_ex(call_info, call);
	// ����
	} else {
		// call �ŵ��������ջ����
		EG(vm_stack_top) = (zval*)call;
	}

	// ��� ������������� ��Ч(��IS_UNUSED)
	if (!RETURN_VALUE_USED(opline)) {
		// ����ret
		i_zval_ptr_dtor(ret);
	}

	// ������쳣
	if (UNEXPECTED(EG(exception) != NULL)) {
		// �������쳣
		zend_rethrow_exception(execute_data);
		// ��Ҫ�� return
		HANDLE_EXCEPTION();
	}
	// �޸ĵ�ǰ�����벢���ء�EX(opline) = p1 
	ZEND_VM_SET_OPCODE(opline + 1);
	// windows: return  0
	ZEND_VM_CONTINUE();
}

// ing2, �����û�����
ZEND_VM_HOT_HANDLER(130, ZEND_DO_UCALL, ANY, ANY, SPEC(RETVAL,OBSERVER))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	// ���õ� ִ������
	zend_execute_data *call = EX(call);
	// ��ǰִ�к���
	zend_function *fbc = call->func;
	zval *ret;

	// windows: �޲���
	SAVE_OPLINE();
	// ���� EX(call) ִ������Ϊǰһ��
	EX(call) = call->prev_execute_data;
	// ��ʼΪnull
	ret = NULL;
	// ��� ���������������Ч(����IS_UNUSED)
	if (RETURN_VALUE_USED(opline)) {
		// ��ִ��������ȡ�� ָ����ŵı���
		ret = EX_VAR(opline->result.var);
	}
	// ����ǰһ��ִ������
	call->prev_execute_data = execute_data;
	// �л���ǰִ������
	execute_data = call;
	// ��ʼ��������ִ�����ݣ�p1:�������飬p2:����ֵ
	i_init_func_execute_data(&fbc->op_array, ret, 0 EXECUTE_DATA_CC);
	// windows: �޲���
	LOAD_OPLINE_EX();
	// zend_vm_gen.php ���� ��������˹۲���, SAVE_OPLINE()
	// "/ZEND_OBSERVER_SAVE_OPLINE\(\)/" => isset($extra_spec['OBSERVER']) && $extra_spec['OBSERVER'] == 1 ? "SAVE_OPLINE()" : "",
	ZEND_OBSERVER_SAVE_OPLINE();
	// zend_vm_gen.php ���� ��������˹۲��ߣ���ʼ�۲��ߵ���: zend_observer_fcall_begin()
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
	// ���õ�ִ������
	zend_execute_data *call = EX(call);
	// ���õĺ���
	zend_function *fbc = call->func;
	zval *ret;

	// windows: �޲���
	SAVE_OPLINE();
	// ת�ص�ǰһ��ִ������
	EX(call) = call->prev_execute_data;

	// ������û�����
	if (EXPECTED(fbc->type == ZEND_USER_FUNCTION)) {
		// ���Ϊnull
		ret = NULL;
		// ��� ���������������Ч(����IS_UNUSED)
		if (RETURN_VALUE_USED(opline)) {
			// ��ִ��������ȡ�� ָ����ŵı���
			ret = EX_VAR(opline->result.var);
		}
		// ����ǰһ��ִ������
		call->prev_execute_data = execute_data;
		// ���µ�ǰִ������
		execute_data = call;
		// ��ʼ��������ִ�����ݣ�p1:�������飬p2:����ֵ
		i_init_func_execute_data(&fbc->op_array, ret, 0 EXECUTE_DATA_CC);
		// windows: �޲���
		LOAD_OPLINE_EX();
		// zend_vm_gen.php ����
		// "/ZEND_OBSERVER_SAVE_OPLINE\(\)/" => isset($extra_spec['OBSERVER']) && $extra_spec['OBSERVER'] == 1 ? "SAVE_OPLINE()" : "",
		ZEND_OBSERVER_SAVE_OPLINE();
		// ��ʼ�۲��ߵ���
		// "/ZEND_OBSERVER_FCALL_BEGIN\(\s*(.*)\s*\)/" => isset($extra_spec['OBSERVER']) ?            ($extra_spec['OBSERVER'] == 0 ? "" : "zend_observer_fcall_begin(\\1)")            : "",
		ZEND_OBSERVER_FCALL_BEGIN(execute_data);
		// return  1
		ZEND_VM_ENTER_EX();
	// ���򣬲����û�����
	} else {
		zval retval;
		// ���� �����ú���
		ZEND_ASSERT(fbc->type == ZEND_INTERNAL_FUNCTION);
		// ��������˹۲���
		// "/ZEND_OBSERVER_ENABLED/" => isset($extra_spec['OBSERVER']) && $extra_spec['OBSERVER'] == 1 ? "1" : "0",
		if (ZEND_OBSERVER_ENABLED) {
			// ���Ϊnull
			ret = NULL;
		}
		// ��������ñ��
		if (UNEXPECTED((fbc->common.fn_flags & ZEND_ACC_DEPRECATED) != 0)) {
			// ���������򷽷�������
			zend_deprecated_function(fbc);
			// ������쳣
			if (UNEXPECTED(EG(exception) != NULL)) {
				// ��� opline ��������� ��������ʱ�������ѽ�� zval ���Ϊδ����
				UNDEF_RESULT();
				// ��� ���������������Ч(��IS_UNUSED)
				if (!RETURN_VALUE_USED(opline)) {
					// ʹ����ʱ���
					ret = &retval;
					// ���Ϊ δ����
					ZVAL_UNDEF(ret);
				}
				// ��ת��ָ����ǩ fcall_by_name_end
				ZEND_VM_C_GOTO(fcall_by_name_end);
			}
		}

		// ����ִ������
		call->prev_execute_data = execute_data;
		// �л���ǰִ������
		EG(current_execute_data) = call;

#if ZEND_DEBUG
		bool should_throw = zend_internal_call_should_throw(fbc, call);
#endif

		// ��� ���������������Ч(����IS_UNUSED) ����
		ret = RETURN_VALUE_USED(opline) ? EX_VAR(opline->result.var) : &retval;
		// ret = null
		ZVAL_NULL(ret);
		// ��ʼ�۲��ߵ���
		// "/ZEND_OBSERVER_FCALL_BEGIN\(\s*(.*)\s*\)/" => isset($extra_spec['OBSERVER']) ?            ($extra_spec['OBSERVER'] == 0 ? "" : "zend_observer_fcall_begin(\\1)")            : "",
		ZEND_OBSERVER_FCALL_BEGIN(call);
		// �������ú���
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
		// ����õ��˹۲��ߣ������۲��ߵ���
		// "/ZEND_OBSERVER_FCALL_END\(\s*([^,]*)\s*,\s*(.*)\s*\)/" => isset($extra_spec['OBSERVER']) ?            ($extra_spec['OBSERVER'] == 0 ? "" : "zend_observer_fcall_end(\\1, \\2)")            : "",
		ZEND_OBSERVER_FCALL_END(call, EG(exception) ? NULL : ret);

		// �л� ��ǰִ������
		EG(current_execute_data) = execute_data;

// ������ת��ǩ fcall_by_name_end
ZEND_VM_C_LABEL(fcall_by_name_end):
		// �ͷ�����ִ�������е� ��������
		zend_vm_stack_free_args(call);

		uint32_t call_info = ZEND_CALL_INFO(call);
		// ����� ��������������� �� �����÷��䡿 ���
		if (UNEXPECTED(call_info & (ZEND_CALL_HAS_EXTRA_NAMED_PARAMS|ZEND_CALL_ALLOCATED))) {
			// ����С�������������� ���
			if (call_info & ZEND_CALL_HAS_EXTRA_NAMED_PARAMS) {
				// �ͷŶ������
				zend_free_extra_named_params(call->extra_named_params);
			}
			// �ͷŵ��ÿ�ܣ�ִ�����ݣ���p1:������Ϣ��p2:ִ������
			zend_vm_stack_free_call_frame_ex(call_info, call);
		// ����
		} else {
			// ��ǰִ�����ݷ����������ջ����
			EG(vm_stack_top) = (zval*)call;
		}

		// ��� ���������������Ч(��IS_UNUSED)
		if (!RETURN_VALUE_USED(opline)) {
			// ���� ret
			i_zval_ptr_dtor(ret);
		}
	}

	// ������쳣
	if (UNEXPECTED(EG(exception) != NULL)) {
		// �����״�
		zend_rethrow_exception(execute_data);
		// ��Ҫ�� return
		HANDLE_EXCEPTION();
	}
	// �޸ĵ�ǰ�����벢���ء�EX(opline) = p1 
	ZEND_VM_SET_OPCODE(opline + 1);
	// windows: return  0
	ZEND_VM_CONTINUE();
}

// ing2, ���ú���
ZEND_VM_HOT_HANDLER(60, ZEND_DO_FCALL, ANY, ANY, SPEC(RETVAL,OBSERVER))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zend_execute_data *call = EX(call);
	zend_function *fbc = call->func;
	zval *ret;

	// windows: �޲���
	SAVE_OPLINE();
	// ���ڵ��õ�ת��ǰһ��ִ������
	EX(call) = call->prev_execute_data;

	// ��������û�����
	if (EXPECTED(fbc->type == ZEND_USER_FUNCTION)) {
		// ���Ϊnull
		ret = NULL;
		// ��� ���������������Ч(����IS_UNUSED)
		if (RETURN_VALUE_USED(opline)) {
			// ��ִ��������ȡ�� ָ����ŵı���
			ret = EX_VAR(opline->result.var);
		}

		// ����ǰһ��ִ������
		call->prev_execute_data = execute_data;
		// �л�ִ������ 
		execute_data = call;
		// ��ʼ��������ִ�����ݣ�p1:�������飬p2:����ֵ
		i_init_func_execute_data(&fbc->op_array, ret, 1 EXECUTE_DATA_CC);

		// ��� ִ�к����� execute_ex
		if (EXPECTED(zend_execute_ex == execute_ex)) {
			// windows: �޲���
			LOAD_OPLINE_EX();
			// zend_vm_gen.php ����
			// "/ZEND_OBSERVER_SAVE_OPLINE\(\)/" => isset($extra_spec['OBSERVER']) && $extra_spec['OBSERVER'] == 1 ? "SAVE_OPLINE()" : "",
			ZEND_OBSERVER_SAVE_OPLINE();
			// ��ʼ�۲��ߵ���
			// "/ZEND_OBSERVER_FCALL_BEGIN\(\s*(.*)\s*\)/" => isset($extra_spec['OBSERVER']) ?            ($extra_spec['OBSERVER'] == 0 ? "" : "zend_observer_fcall_begin(\\1)")            : "",
			ZEND_OBSERVER_FCALL_BEGIN(execute_data);
			// return  1
			ZEND_VM_ENTER_EX();
		// ����ʹ�� zend_execute_ex
		} else {
			// windows: �޲���
			SAVE_OPLINE_EX();
			// ��ʼ�۲��ߵ���
			// "/ZEND_OBSERVER_FCALL_BEGIN\(\s*(.*)\s*\)/" => isset($extra_spec['OBSERVER']) ?            ($extra_spec['OBSERVER'] == 0 ? "" : "zend_observer_fcall_begin(\\1)")            : "",
			ZEND_OBSERVER_FCALL_BEGIN(execute_data);
			// �л�ִ�����ݵ�ǰһ��
			execute_data = EX(prev_execute_data);
			// windows: �޲���
			LOAD_OPLINE();
			// ��ӱ�ǡ�������á�
			ZEND_ADD_CALL_FLAG(call, ZEND_CALL_TOP);
			// ����ִ�к���
			zend_execute_ex(call);
		}
	// ���򣬵������ú���
	} else {
		zval retval;
		// �����ǵ������ú���
		ZEND_ASSERT(fbc->type == ZEND_INTERNAL_FUNCTION);
		// "/ZEND_OBSERVER_ENABLED/" => isset($extra_spec['OBSERVER']) && $extra_spec['OBSERVER'] == 1 ? "1" : "0",
		if (ZEND_OBSERVER_ENABLED) {
			ret = NULL;
		}

		// ��������õĺ���
		if (UNEXPECTED((fbc->common.fn_flags & ZEND_ACC_DEPRECATED) != 0)) {
			// ���������򷽷�������
			zend_deprecated_function(fbc);
			// ������쳣
			if (UNEXPECTED(EG(exception) != NULL)) {
				// ��� opline ��������� ��������ʱ�������ѽ�� zval ���Ϊδ����
				UNDEF_RESULT();
				// ��� ���������������Ч(��IS_UNUSED)
				if (!RETURN_VALUE_USED(opline)) {
					// ʹ����ʱ�������շ���ֵ
					ret = &retval;
					// ���Ϊ δ����
					ZVAL_UNDEF(ret);
				}
				// ��ת��ָ����ǩ fcall_end
				ZEND_VM_C_GOTO(fcall_end);
			}
		}

		// ��ǰִ�����ݵ���ǰһ��
		call->prev_execute_data = execute_data;
		// �л�ִ������
		EG(current_execute_data) = call;

// ������
#if ZEND_DEBUG
		bool should_throw = zend_internal_call_should_throw(fbc, call);
#endif

		// ��� ���������������Ч(����IS_UNUSED) ����
		ret = RETURN_VALUE_USED(opline) ? EX_VAR(opline->result.var) : &retval;
		// ���Ϊnull
		ZVAL_NULL(ret);
		// ��ʼ�۲��ߵ���
		// "/ZEND_OBSERVER_FCALL_BEGIN\(\s*(.*)\s*\)/" => isset($extra_spec['OBSERVER']) ?            ($extra_spec['OBSERVER'] == 0 ? "" : "zend_observer_fcall_begin(\\1)")            : "",
		ZEND_OBSERVER_FCALL_BEGIN(call);
		// һ�������Ϊnull 
		if (!zend_execute_internal) {
			// ���û�� zend_execute_internal ����һ����������
			/* saves one function call if zend_execute_internal is not used */
			// �������ú���
			fbc->internal_function.handler(call, ret);
		// ����
		} else {
			// ���� zend_execute_internal��ԭ�ͣ�
			zend_execute_internal(call, ret);
		}

// ������
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
		// ����õ��˹۲��ߣ������۲��ߵ���
		// "/ZEND_OBSERVER_FCALL_END\(\s*([^,]*)\s*,\s*(.*)\s*\)/" => isset($extra_spec['OBSERVER']) ?            ($extra_spec['OBSERVER'] == 0 ? "" : "zend_observer_fcall_end(\\1, \\2)")            : "",
		ZEND_OBSERVER_FCALL_END(call, EG(exception) ? NULL : ret);

		// ��ǰִ��������Ϊǰһ��
		EG(current_execute_data) = execute_data;

// ������ת��ǩ fcall_end
ZEND_VM_C_LABEL(fcall_end):
		// �ͷ�����ִ�������е� ��������
		zend_vm_stack_free_args(call);
		// ����ж�����������
		if (UNEXPECTED(ZEND_CALL_INFO(call) & ZEND_CALL_HAS_EXTRA_NAMED_PARAMS)) {
			// �ͷŶ�����������
			zend_free_extra_named_params(call->extra_named_params);
		}

		// ��� ���������������Ч(��IS_UNUSED)
		if (!RETURN_VALUE_USED(opline)) {
			// ����û�����ô����Ķ��������ô�������ӵ�gc�����������û������ zval����
			i_zval_ptr_dtor(ret);
		}
	}

	// �����Ҫ�ͷ�$this
	if (UNEXPECTED(ZEND_CALL_INFO(call) & ZEND_CALL_RELEASE_THIS)) {
		// �ͷ�$this
		OBJ_RELEASE(Z_OBJ(call->This));
	}
	// �ͷŵ��ÿ�ܣ�ִ�����ݣ���p1:ִ������
	zend_vm_stack_free_call_frame(call);
	// ������쳣
	if (UNEXPECTED(EG(exception) != NULL)) {
		// �������쳣
		zend_rethrow_exception(execute_data);
		// ��Ҫ�� return
		HANDLE_EXCEPTION();
	}
	// �޸ĵ�ǰ�����벢���ء�EX(opline) = p1 
	ZEND_VM_SET_OPCODE(opline + 1);
	// windows: return  0
	ZEND_VM_CONTINUE();
}

// ing3, ��֤��������ֵ����
ZEND_VM_COLD_CONST_HANDLER(124, ZEND_VERIFY_RETURN_TYPE, CONST|TMP|VAR|UNUSED|CV, UNUSED|CACHE_SLOT)
{
	// ��� op1 ������δ����
	if (OP1_TYPE == IS_UNUSED) {
		// windows: �޲���
		SAVE_OPLINE();
		// �����������Ͳ���
		zend_verify_missing_return_type(EX(func));
		// ��Ҫ�� return
		HANDLE_EXCEPTION();
	// ����, op1����δ����
	} else {
/* prevents "undefined variable opline" errors */
// op1��op2 ����ANY
#if !ZEND_VM_SPEC || (OP1_TYPE != IS_UNUSED)
		// const zend_op *opline = EX(opline);
		USE_OPLINE
		zval *retval_ref, *retval_ptr;
		// ����ֵ��Ϣ
		zend_arg_info *ret_info = EX(func)->common.arg_info - 1;
		// ��ȡzvalָ��, UNUSED ����null
		retval_ref = retval_ptr = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);

		// ���op1�ǳ���
		if (OP1_TYPE == IS_CONST) {
			// op1���Ƹ� op1
			ZVAL_COPY(EX_VAR(opline->result.var), retval_ptr);
			// ��ִ��������ȡ�� ָ����ŵı���
			retval_ref = retval_ptr = EX_VAR(opline->result.var);
		// ���� ��� op1 �� ��ͨ����
		} else if (OP1_TYPE == IS_VAR) {
			// op1�Ǽ������
			if (UNEXPECTED(Z_TYPE_P(retval_ptr) == IS_INDIRECT)) {
				// �������ã�׷�ٵ�����Ŀ��
				retval_ref = retval_ptr = Z_INDIRECT_P(retval_ptr);
			}
			// op1 ������
			ZVAL_DEREF(retval_ptr);
		// ���� ��� op1 �� �������
		} else if (OP1_TYPE == IS_CV) {
			// op1 ������
			ZVAL_DEREF(retval_ptr);
		}

		// ��� p1�������� �Ƿ������Ҫ������ p2�������
		if (EXPECTED(ZEND_TYPE_CONTAINS_CODE(ret_info->type, Z_TYPE_P(retval_ptr)))) {
			// opline + 1����Ŀ������벢 return
			ZEND_VM_NEXT_OPCODE();
		}

		// ��� op1 �� ������� ���� ׷�ٵ�δ�������
		if (OP1_TYPE == IS_CV && UNEXPECTED(Z_ISUNDEF_P(retval_ptr))) {
			// windows: �޲���
			SAVE_OPLINE();
			// ����p1�ı�������������δ����, ����δ��ʼ��zval
			retval_ref = retval_ptr = ZVAL_UNDEFINED_OP1();
			// ������쳣
			if (UNEXPECTED(EG(exception))) {
				// ��Ҫ�� return
				HANDLE_EXCEPTION();
			}
			// ��������� ���� null
			if (ZEND_TYPE_FULL_MASK(ret_info->type) & MAY_BE_NULL) {
				// opline + 1����Ŀ������벢 return
				ZEND_VM_NEXT_OPCODE();
			}
		}

		// ��ʱ����ʵ��
		zend_reference *ref = NULL;
		// op2�е���Ŵӻ���λ����ȡ�� ָ��
		void *cache_slot = CACHE_ADDR(opline->op2.num);
		// ��ʱ������ֵ����ȣ�˵�� retval_ptr �������ù�
		if (UNEXPECTED(retval_ref != retval_ptr)) {
			// ���Ҫ�����÷���
			if (UNEXPECTED(EX(func)->op_array.fn_flags & ZEND_ACC_RETURN_REFERENCE)) {
				// ȡ������ʵ��
				ref = Z_REF_P(retval_ref);
			// ���������е㸴��
			} else {
				// ���ܳ������������������ﷵ����һ���������ͣ���Ҫ���
				/* A cast might happen - unwrap the reference if this is a by-value return */
				// ���������Ϊ1
				if (Z_REFCOUNT_P(retval_ref) == 1) {
					// �����ã�׷�ٵ�����Ŀ�꣬��Ŀ���еı������ƹ�����������Ŀ�ꡣ
					ZVAL_UNREF(retval_ref);
					// ��� ʹ�� retval_ref
				// ����
				} else {
					// �������ô���
					Z_DELREF_P(retval_ref);
					// ʹ�� retval_ptr
					ZVAL_COPY(retval_ref, retval_ptr);
				}
				// ���ʹ�� retval_ref
				retval_ptr = retval_ref;
			}
		}

		// windows: �޲���
		SAVE_OPLINE();
		// ������������Ƿ�ƥ�䣬p1:���ͣ�p2:������p3:����ʵ��,�������������͵�ʱ����У�
		// p4:����λ�ã�p5:�Ƿ��Ƿ������ͣ�p6:�Ƿ������ú���
		if (UNEXPECTED(!zend_check_type_slow(&ret_info->type, retval_ptr, ref, cache_slot, 1, 0))) {
			// �����������Ͳ�����p1:������p2:����ֵ
			zend_verify_return_error(EX(func), retval_ptr);
			// ��Ҫ�� return
			HANDLE_EXCEPTION();
		}
		// opline + 1����Ŀ������벢 return
		ZEND_VM_NEXT_OPCODE();
#endif
	}
}

// ing3, ���鷵��ֵ���ͣ����� :never
ZEND_VM_COLD_HANDLER(201, ZEND_VERIFY_NEVER_TYPE, UNUSED, UNUSED)
{
	// windows: �޲���
	SAVE_OPLINE();
	// �������Ͳ����� :never ʱ�ı���
	zend_verify_never_error(EX(func));
	// ��Ҫ�� return
	HANDLE_EXCEPTION();
}

// ing2, return���
ZEND_VM_INLINE_HANDLER(62, ZEND_RETURN, CONST|TMP|VAR|CV, ANY, SPEC(OBSERVER))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *retval_ptr;
	zval *return_value;
	// zend_vm_gen.php ����۲��ߣ�zval observer_retval;
	// "/ZEND_OBSERVER_USE_RETVAL/" => isset($extra_spec['OBSERVER']) && $extra_spec['OBSERVER'] == 1 ? "zval observer_retval" : ""
	ZEND_OBSERVER_USE_RETVAL;

	// ��ȡzvalָ��, UNUSED ����null
	retval_ptr = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ȡ������ֵ
	return_value = EX(return_value);
	
	// zend_vm_gen.php ����۲��ߣ�if (!return_value) { return_value = &observer_retval; }
	// "/ZEND_OBSERVER_SET_RETVAL\(\)/" => isset($extra_spec['OBSERVER']) && $extra_spec['OBSERVER'] == 1 ? "if (!return_value) { return_value = &observer_retval; }" : "",        
	ZEND_OBSERVER_SET_RETVAL();
	
	// ��� op1 �� ������� ���� ����ֵ����Ϊ δ����
	if (OP1_TYPE == IS_CV && UNEXPECTED(Z_TYPE_INFO_P(retval_ptr) == IS_UNDEF)) {
		// windows: �޲���
		SAVE_OPLINE();
		// ����p1�ı�������������δ����, ����δ��ʼ��zval
		retval_ptr = ZVAL_UNDEFINED_OP1();
		// ����з���ֵ
		if (return_value) {
			// ��null
			ZVAL_NULL(return_value);
		}
	// ���û�з���ֵ
	} else if (!return_value) {
		// ��� op1 �����Ǳ��� �� ��ʱ����
		if (OP1_TYPE & (IS_VAR|IS_TMP_VAR)) {
			// ����� �ɼ������� ���� ���ü���-1 ��Ϊ0
			if (Z_REFCOUNTED_P(retval_ptr) && !Z_DELREF_P(retval_ptr)) {
				// windows: �޲���
				SAVE_OPLINE();
				// ����ÿ��type��Ӧ�����ٺ���ִ������
				rc_dtor_func(Z_COUNTED_P(retval_ptr));
			}
		}
	// ����
	} else {
		// ��� op1 �����Ǳ��� �� ��ʱ����
		if ((OP1_TYPE & (IS_CONST|IS_TMP_VAR))) {
			// op1��Ϊ����ֵ
			ZVAL_COPY_VALUE(return_value, retval_ptr);
			// ���op1�ǳ���
			if (OP1_TYPE == IS_CONST) {
				// ����ǿ��Լ�������
				if (UNEXPECTED(Z_OPT_REFCOUNTED_P(return_value))) {
					// ���ô���+1
					Z_ADDREF_P(return_value);
				}
			}
		// ���� ��� op1 �� �������
		} else if (OP1_TYPE == IS_CV) {
			// Ϊ��break
			do {
				// ����ǿ��Լ�������
				if (Z_OPT_REFCOUNTED_P(retval_ptr)) {
					// ͨ��ָ����֤ zval��type �Ƿ��� IS_REFERENCE
					// ���������������
					if (EXPECTED(!Z_OPT_ISREF_P(retval_ptr))) {
						// ���û�� ZEND_CALL_CODE �� ZEND_CALL_OBSERVED
						if (EXPECTED(!(EX_CALL_INFO() & (ZEND_CALL_CODE|ZEND_CALL_OBSERVED)))) {
							// ȡ��������
							zend_refcounted *ref = Z_COUNTED_P(retval_ptr);
							// op1 ���Ƹ� ���
							ZVAL_COPY_VALUE(return_value, retval_ptr);
							// ��� ref ����й¶
							if (GC_MAY_LEAK(ref)) {
								// windows: �޲���
								SAVE_OPLINE();
								// ref ������ն���
								gc_possible_root(ref);
							}
							// ���ָ��
							ZVAL_NULL(retval_ptr);
							// ����
							break;
						// ����
						} else {
							// ���ô���+1
							Z_ADDREF_P(retval_ptr);
						}
					// ����
					} else {
						// ������
						retval_ptr = Z_REFVAL_P(retval_ptr);
						// ����ǿ��Լ�������
						if (Z_OPT_REFCOUNTED_P(retval_ptr)) {
							// ���ô���+1
							Z_ADDREF_P(retval_ptr);
						}
					}
				}
				// op1 ���Ƹ� ���
				ZVAL_COPY_VALUE(return_value, retval_ptr);
			} while (0);
		// ���� ��� op1 �� ��ͨ����
		} else /* if (OP1_TYPE == IS_VAR) */ {
			// ��� op1����������
			if (UNEXPECTED(Z_ISREF_P(retval_ptr))) {
				// ȡ��������
				zend_refcounted *ref = Z_COUNTED_P(retval_ptr);
				// ������
				retval_ptr = Z_REFVAL_P(retval_ptr);
				// op1 ���Ƹ� ���
				ZVAL_COPY_VALUE(return_value, retval_ptr);
				// ���ô���-1�����Ϊ0
				if (UNEXPECTED(GC_DELREF(ref) == 0)) {
					// �ͷ�����ʵ��
					efree_size(ref, sizeof(zend_reference));
				// ���� ������ǿ��Լ�������
				} else if (Z_OPT_REFCOUNTED_P(retval_ptr)) {
					// ���ô���+1
					Z_ADDREF_P(retval_ptr);
				}
			// ����
			} else {
				// op1 ���Ƹ� ���
				ZVAL_COPY_VALUE(return_value, retval_ptr);
			}
		}
	}
	// zend_vm_gen.php ����
	// "/ZEND_OBSERVER_SAVE_OPLINE\(\)/" => isset($extra_spec['OBSERVER']) && $extra_spec['OBSERVER'] == 1 ? "SAVE_OPLINE()" : "",
	ZEND_OBSERVER_SAVE_OPLINE();
	
	// ����õ��˹۲��ߣ������۲��ߵ���
	// "/ZEND_OBSERVER_FCALL_END\(\s*([^,]*)\s*,\s*(.*)\s*\)/" => isset($extra_spec['OBSERVER']) ?            ($extra_spec['OBSERVER'] == 0 ? "" : "zend_observer_fcall_end(\\1, \\2)")            : "",
	ZEND_OBSERVER_FCALL_END(execute_data, return_value);
	
	// ����۲��ߣ�if (return_value == &observer_retval) { zval_ptr_dtor_nogc(&observer_retval); }
	// "/ZEND_OBSERVER_FREE_RETVAL\(\)/" => isset($extra_spec['OBSERVER']) && $extra_spec['OBSERVER'] == 1 ? "if (return_value == &observer_retval) { zval_ptr_dtor_nogc(&observer_retval); }" : "",
    ZEND_OBSERVER_FREE_RETVAL();
	// 
	// ���÷��� zend_leave_helper
	ZEND_VM_DISPATCH_TO_HELPER(zend_leave_helper);
}

// ing2, ���÷���
ZEND_VM_COLD_CONST_HANDLER(111, ZEND_RETURN_BY_REF, CONST|TMP|VAR|CV, ANY, SRC, SPEC(OBSERVER))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *retval_ptr;
	zval *return_value;
	// "/ZEND_OBSERVER_USE_RETVAL/" => isset($extra_spec['OBSERVER']) && $extra_spec['OBSERVER'] == 1 ? "zval observer_retval" : ""
	ZEND_OBSERVER_USE_RETVAL;

	// windows: �޲���
	SAVE_OPLINE();

	// �������еķ���ֵ
	return_value = EX(return_value);
	// "/ZEND_OBSERVER_SET_RETVAL\(\)/" => isset($extra_spec['OBSERVER']) && $extra_spec['OBSERVER'] == 1 ? "if (!return_value) { return_value = &observer_retval; }" : "",
	ZEND_OBSERVER_SET_RETVAL();
	// Ϊ��break
	do {
		// ��� ��op1 �ǳ��� �� ��ʱ������ �� ��op1�Ǳ��� ���� ZEND_RETURNS_VALUE��
		if ((OP1_TYPE & (IS_CONST|IS_TMP_VAR)) ||
		    (OP1_TYPE == IS_VAR && opline->extended_value == ZEND_RETURNS_VALUE)) {
			// �����������ͣ������������ֻ������ʾ
			/* Not supposed to happen, but we'll allow it */
			// ��ʾ�� ֻ�ڱ��������� ���Ա� ���ô���
			zend_error(E_NOTICE, "Only variable references should be returned by reference");

			// ��ȡzvalָ�룬UNUSED ���ͷ���null
			retval_ptr = GET_OP1_ZVAL_PTR(BP_VAR_R);
			// ���û�з���ֵ
			if (!return_value) {
				// �ͷŲ�������ĸ��ӱ���
				FREE_OP1();
			// �����з���ֵ
			} else {
				// ���op1�Ǳ��� ���� op1����������
				if (OP1_TYPE == IS_VAR && UNEXPECTED(Z_ISREF_P(retval_ptr))) {
					// op1 ���Ƹ�����ֵ
					ZVAL_COPY_VALUE(return_value, retval_ptr);
					// ����
					break;
				}
				// �� zval �����µ� zend_reference ���Ӹ����� zval �и��Ƹ�������
				ZVAL_NEW_REF(return_value, retval_ptr);
				// ���op1�ǳ���
				if (OP1_TYPE == IS_CONST) {
					// �������ô���
					Z_TRY_ADDREF_P(retval_ptr);
				}
			}
			// ����
			break;
		}
		
		// ��ȡzvalָ��,TMP/CONST/UNUSED ����null����֧�� TMPVAR/TMPVARCV
		retval_ptr = GET_OP1_ZVAL_PTR_PTR(BP_VAR_W);

		// ��� op1 �� ��ͨ����
		if (OP1_TYPE == IS_VAR) {
			// op1������Ч
			ZEND_ASSERT(retval_ptr != &EG(uninitialized_zval));
			// ��� ZEND_RETURNS_FUNCTION ���� ���������������
			if (opline->extended_value == ZEND_RETURNS_FUNCTION && !Z_ISREF_P(retval_ptr)) {
				// ��ʾ��ֻ�б��������ÿ��Ա����÷���
				zend_error(E_NOTICE, "Only variable references should be returned by reference");
				// ����з���ֵ
				if (return_value) {
					// �� zval �����µ� zend_reference ���Ӹ����� zval �и��Ƹ�������
					ZVAL_NEW_REF(return_value, retval_ptr);
				// ����
				} else {
					// �ͷŲ�������ĸ��ӱ���
					FREE_OP1();
				}
				// ����
				break;
			}
		}

		// ����з���ֵ
		if (return_value) {
			// �������������
			if (Z_ISREF_P(retval_ptr)) {
				// ���ô���+1
				Z_ADDREF_P(retval_ptr);
			// ���򣬲�����������
			} else {
				// ��zvalתΪ���ã�p1:zval��p2:���ô���
				ZVAL_MAKE_REF_EX(retval_ptr, 2);
			}
			// ������ֵ���ø�ֵ
			ZVAL_REF(return_value, Z_REF_P(retval_ptr));
		}

		// �ͷŲ�������ĸ��ӱ���
		FREE_OP1();
	} while (0);

	// ����õ��˹۲��ߣ������۲��ߵ���
	// "/ZEND_OBSERVER_FCALL_END\(\s*([^,]*)\s*,\s*(.*)\s*\)/" => isset($extra_spec['OBSERVER']) ?            ($extra_spec['OBSERVER'] == 0 ? "" : "zend_observer_fcall_end(\\1, \\2)")            : "",
	ZEND_OBSERVER_FCALL_END(execute_data, return_value);
	
	// "/ZEND_OBSERVER_FREE_RETVAL\(\)/" => isset($extra_spec['OBSERVER']) && $extra_spec['OBSERVER'] == 1 ? "if (return_value == &observer_retval) { zval_ptr_dtor_nogc(&observer_retval); }" : "",
    ZEND_OBSERVER_FREE_RETVAL();
	
	// ���÷��� zend_leave_helper
	ZEND_VM_DISPATCH_TO_HELPER(zend_leave_helper);
}

// ing2, ������ create ����
ZEND_VM_HANDLER(139, ZEND_GENERATOR_CREATE, ANY, ANY)
{
	// ����ֵ
	zval *return_value = EX(return_value);

	// ����з���ֵ
	if (EXPECTED(return_value)) {
		// const zend_op *opline = EX(opline);
		USE_OPLINE
		zend_generator *generator;
		zend_execute_data *gen_execute_data;
		uint32_t num_args, used_stack, call_info;

		// windows: �޲���
		SAVE_OPLINE();
		// ��ʼ�����󣬴���zvalָ��������
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
		// ����
		} else {
			used_stack = (ZEND_CALL_FRAME_SLOT + num_args + EX(func)->op_array.last_var + EX(func)->op_array.T - EX(func)->op_array.num_args) * sizeof(zval);
			gen_execute_data = (zend_execute_data*)emalloc(used_stack);
		}
		memcpy(gen_execute_data, execute_data, used_stack);

		// ��ִ�������ı��浽�������Ķ�����
		/* Save execution context in generator object. */
		// ����ֵת��������
		generator = (zend_generator *) Z_OBJ_P(EX(return_value));
		// ����������
		generator->execute_data = gen_execute_data;
		// ������ö�ջΪ��
		generator->frozen_call_stack = NULL;
		// ������Ϊ��
		generator->execute_fake.opline = NULL;
		// ����Ϊ��
		generator->execute_fake.func = NULL;
		// ǰһ��ִ������Ϊ��
		generator->execute_fake.prev_execute_data = NULL;
		// �����������������Լ��� $this
		ZVAL_OBJ(&generator->execute_fake.This, (zend_object *) generator);

		// �л�����һ��������
		gen_execute_data->opline = opline + 1;
		
		// EX(return_value) ���� zend_object ��ָ�룬��������� zval
		/* EX(return_value) keeps pointer to zend_object (not a real zval) */
		
		// ���� �����������ĵ� ����ֵΪ ����������
		gen_execute_data->return_value = (zval*)generator;
		// ������Ϣ
		call_info = Z_TYPE_INFO(EX(This));
		// ���������Ϣ �� �����ʽ ���� �������ǵ��ñհ�Ҳ ����Ҫ�ͷ�$this�� �� zend_execute_ex != execute_ex��
		if ((call_info & Z_TYPE_MASK) == IS_OBJECT
		 && (!(call_info & (ZEND_CALL_CLOSURE|ZEND_CALL_RELEASE_THIS))
			 /* Bug #72523 */
			|| UNEXPECTED(zend_execute_ex != execute_ex))) {
			// p1 |= p2	
			ZEND_ADD_CALL_FLAG_EX(call_info, ZEND_CALL_RELEASE_THIS);
			// $this�������ô���
			Z_ADDREF(gen_execute_data->This);
		}
		// p1 |= p2
		ZEND_ADD_CALL_FLAG_EX(call_info, (ZEND_CALL_TOP_FUNCTION | ZEND_CALL_ALLOCATED | ZEND_CALL_GENERATOR));
		// ����������ִ�����ݵ� ������Ϣ
		Z_TYPE_INFO(gen_execute_data->This) = call_info;
		// ������ִ�����ݵ�ǰһ��ִ������Ϊ null
		gen_execute_data->prev_execute_data = NULL;

		// ȡ��������Ϣ
		call_info = EX_CALL_INFO();
		// �л���ǰһ��ִ������
		EG(current_execute_data) = EX(prev_execute_data);
		// ���û�С�������á��͡����÷��䡿���
		if (EXPECTED(!(call_info & (ZEND_CALL_TOP|ZEND_CALL_ALLOCATED)))) {
			// ִ�����ݷŵ��������ջ��������
			EG(vm_stack_top) = (zval*)execute_data;
			// �л���ǰһ��ִ������
			execute_data = EX(prev_execute_data);
			// windows: ZEND_VM_INC_OPCODE() : // OPLINE++
			LOAD_NEXT_OPLINE();
			// windows return 2;
			ZEND_VM_LEAVE();
		// ������ǡ�������á�
		} else if (EXPECTED(!(call_info & ZEND_CALL_TOP))) {
			// ԭִ������
			zend_execute_data *old_execute_data = execute_data;
			// �л���ǰһ��ִ������
			execute_data = EX(prev_execute_data);
			// �ͷŵ��ÿ�ܣ�ִ�����ݣ���p1:������Ϣ��p2:ִ������
			zend_vm_stack_free_call_frame_ex(call_info, old_execute_data);
			// windows: ZEND_VM_INC_OPCODE() : // OPLINE++
			LOAD_NEXT_OPLINE();
			// windows return 2;
			ZEND_VM_LEAVE();
		// ����
		} else {
			// windows: return -1
			ZEND_VM_RETURN();
		}
	// ����
	} else {
		// ���÷��� zend_leave_helper
		ZEND_VM_DISPATCH_TO_HELPER(zend_leave_helper);
	}
}

// ing3, �������ķ���ֵ
ZEND_VM_HANDLER(161, ZEND_GENERATOR_RETURN, CONST|TMP|VAR|CV, ANY, SPEC(OBSERVER))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *retval;

	// ��ȡ�������е�������
	zend_generator *generator = zend_get_running_generator(EXECUTE_DATA_C);

	// windows: �޲���
	SAVE_OPLINE();
	// ��ȡzvalָ�룬UNUSED ���ͷ���null
	retval = GET_OP1_ZVAL_PTR(BP_VAR_R);

	// �ѷ���ֵ op1 ���Ƶ� �������ķ���ֵ��
	/* Copy return value into generator->retval */
	// ���op1�� ���� �� ����
	if ((OP1_TYPE & (IS_CONST|IS_TMP_VAR))) {
		// �ѷ���ֵ op1 ���Ƶ� �������ķ���ֵ��
		ZVAL_COPY_VALUE(&generator->retval, retval);
		// ���op1�ǳ���
		if (OP1_TYPE == IS_CONST) {
			// ����������ķ���ֵ�ǿɼ�������
			if (UNEXPECTED(Z_OPT_REFCOUNTED(generator->retval))) {
				// �������ô���
				Z_ADDREF(generator->retval);
			}
		}
	// ���� ��� op1 �� �������
	} else if (OP1_TYPE == IS_CV) {
		// ��������Ŀ�� �� ������Ϣ�����������ü���
		ZVAL_COPY_DEREF(&generator->retval, retval);
	// ���� ��� op1 �� ��ͨ����
	} else /* if (OP1_TYPE == IS_VAR) */ {
		// �������ֵ����������
		if (UNEXPECTED(Z_ISREF_P(retval))) {
			// ȡ������ʵ��
			zend_refcounted *ref = Z_COUNTED_P(retval);
			// ������
			retval = Z_REFVAL_P(retval);
			// �ѷ���ֵ op1 ���Ƶ� �������ķ���ֵ��
			ZVAL_COPY_VALUE(&generator->retval, retval);
			// ���ô���-1�����Ϊ0
			if (UNEXPECTED(GC_DELREF(ref) == 0)) {
				// �ͷ�����ʵ��
				efree_size(ref, sizeof(zend_reference));
			// ���� ������ǿ��Լ�������
			} else if (Z_OPT_REFCOUNTED_P(retval)) {
				// ���ô���+1
				Z_ADDREF_P(retval);
			}
		// ����
		} else {
			// �ѷ���ֵ op1 ���Ƶ� �������ķ���ֵ��
			ZVAL_COPY_VALUE(&generator->retval, retval);
		}
	}

	// ����õ��˹۲��ߣ������۲��ߵ���
	// "/ZEND_OBSERVER_FCALL_END\(\s*([^,]*)\s*,\s*(.*)\s*\)/" => isset($extra_spec['OBSERVER']) ?            ($extra_spec['OBSERVER'] == 0 ? "" : "zend_observer_fcall_end(\\1, \\2)")            : "",
	ZEND_OBSERVER_FCALL_END(generator->execute_data, &generator->retval);

	// �ر������� �ͷ��������Դ
	/* Close the generator to free up resources */
	zend_generator_close(generator, 1);

	// ��ִ�����ݴ��� �����߼�
	/* Pass execution back to handling code */
	// windows: return -1
	ZEND_VM_RETURN();
}

// ing3, �׳��쳣��throw���
ZEND_VM_COLD_CONST_HANDLER(108, ZEND_THROW, CONST|TMPVAR|CV, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *value;

	// windows: �޲���
	SAVE_OPLINE();
	// ��ȡzvalָ��, UNUSED ����null
	value = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);

	do {
		// ���op1�ǳ��� �� ֵ���Ƕ���
		if (OP1_TYPE == IS_CONST || UNEXPECTED(Z_TYPE_P(value) != IS_OBJECT)) {
			// ��� op1 ����ͨ������������ ���� op1����������
			if ((OP1_TYPE & (IS_VAR|IS_CV)) && Z_ISREF_P(value)) {
				// ������
				value = Z_REFVAL_P(value);
				// ���ֵ�Ƕ���
				if (EXPECTED(Z_TYPE_P(value) == IS_OBJECT)) {
					// ����
					break;
				}
			}
			// ��� op1 �� ������� ���� ֵ ����Ϊ δ����
			if (OP1_TYPE == IS_CV && UNEXPECTED(Z_TYPE_P(value) == IS_UNDEF)) {
				// ����p1�ı�������������δ����, ����δ��ʼ��zval
				ZVAL_UNDEFINED_OP1();
				// ������쳣
				if (UNEXPECTED(EG(exception) != NULL)) {
					// ��Ҫ�� return
					HANDLE_EXCEPTION();
				}
			}
			// ���쳣��ֻ���׳� ����
			zend_throw_error(NULL, "Can only throw objects");
			// �ͷŲ�������ĸ��ӱ���
			FREE_OP1();
			// ��Ҫ�� return
			HANDLE_EXCEPTION();
		}
	} while (0);

	// ���쳣���浽 previous ���� ,��յ�ǰ�쳣
	zend_exception_save();
	// �������ô���
	Z_TRY_ADDREF_P(value);
	// �׳��쳣ʵ��
	zend_throw_exception_object(value);
	// ����е�ǰ�쳣������һ���쳣�洢���������û�У�����һ����Ϊ��ǰ�쳣
	zend_exception_restore();
	// �ͷŲ�������ĸ��ӱ���
	FREE_OP1();
	// ��Ҫ�� return
	HANDLE_EXCEPTION();
}

// ing3, catch���
ZEND_VM_HANDLER(107, ZEND_CATCH, CONST, JMP_ADDR, LAST_CATCH|CACHE_SLOT)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zend_class_entry *ce, *catch_ce;
	zend_object *exception;

	// windows: �޲���
	SAVE_OPLINE();
	/* Check whether an exception has been thrown, if not, jump over code */
	// ����е�ǰ�쳣������һ���쳣�洢���������û�У�����һ����Ϊ��ǰ�쳣
	zend_exception_restore();
	// ������쳣
	if (EG(exception) == NULL) {
		// Ҫ����쳣 �� ���쳣ʱʹ�þɲ����룬���쳣ʹ���²�����, p1:�²����룬p2:�Ƿ����쳣
		// ���� p2.jmp_addr
		ZEND_VM_JMP_EX(OP_JMP_ADDR(opline, opline->op2), 0);
	}
	
	// ���catch��Ҫ���������
	// ͨ��ƫ���� ������ʱ�����л�ȡһ�� (void**) �еĵ�һ��Ԫ��
	catch_ce = CACHED_PTR(opline->extended_value & ~ZEND_LAST_CATCH);
	// ���û��ָ��cache����
	if (UNEXPECTED(catch_ce == NULL)) {
		// ��ȡ�࣬�Ҳ����ᱨ��p1:������p2:key��p3:flags
		// ���� (p2).zv��p1:opline,p2:node
		catch_ce = zend_fetch_class_by_name(Z_STR_P(RT_CONSTANT(opline, opline->op1)), Z_STR_P(RT_CONSTANT(opline, opline->op1) + 1), ZEND_FETCH_CLASS_NO_AUTOLOAD | ZEND_FETCH_CLASS_SILENT);

		// ͨ��ƫ���� �� EX(run_time_cache) �л�ȡһ�� (void**) �еĵ�һ��Ԫ�أ���������ֵ
		CACHE_PTR(opline->extended_value & ~ZEND_LAST_CATCH, catch_ce);
	}
	// ȡ��cache����
	ce = EG(exception)->ce;

// 
#ifdef HAVE_DTRACE
	if (DTRACE_EXCEPTION_CAUGHT_ENABLED()) {
		DTRACE_EXCEPTION_CAUGHT((char *)ce->name);
	}
#endif /* HAVE_DTRACE */

	// ��������쳣�� cache�������Ͳ�ͬ
	if (ce != catch_ce) {
		// ���û��cache���� �� �쳣���Ͳ� ���� cache����
		if (!catch_ce || !instanceof_function(ce, catch_ce)) {
			// ����� ���һ��catch���
			if (opline->extended_value & ZEND_LAST_CATCH) {
				// �����׳��쳣
				zend_rethrow_exception(execute_data);
				// ��Ҫ�� return
				HANDLE_EXCEPTION();
			}
			// Ҫ����쳣 �� ���쳣ʱʹ�þɲ����룬���쳣ʹ���²�����, p1:�²����룬p2:�Ƿ����쳣
			// OP_JMP_ADDR: ���� p2.jmp_addr
			ZEND_VM_JMP_EX(OP_JMP_ADDR(opline, opline->op2), 0);
		}
	}

	// ȡ�������쳣
	exception = EG(exception);
	// ��������쳣
	EG(exception) = NULL;
	// ��� ���������������Ч(����IS_UNUSED)
	if (RETURN_VALUE_USED(opline)) {
		// ���ǽ���һ���ϸ�ĸ�ֵ��
		/* Always perform a strict assignment. There is a reasonable expectation that if you
		 * write "catch (Exception $e)" then $e will actually be instanceof Exception. As such,
		 * we should not permit coercion to string here. */
		zval tmp;
		// �쳣�浽��ʱ������
		ZVAL_OBJ(&tmp, exception);
		// ��������ֵ��p1:������p2:ֵ��p3:ֵ�ı������ͣ������������������������p4:�Ƿ��ϸ�����
		// ĳִ���������������� �Ĳ����Ƿ�ʹ���ϸ���������
		zend_assign_to_variable(EX_VAR(opline->result.var), &tmp, IS_TMP_VAR, /* strict */ 1);
		
	// ���򣬲���������н����Ч
	} else {
		// �ͷ��쳣
		OBJ_RELEASE(exception);
	}
	// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, op1���Ƹ� result.�����op2��˵������������
ZEND_VM_HOT_HANDLER(65, ZEND_SEND_VAL, CONST|TMPVAR, CONST|UNUSED|NUM)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *value, *arg;

	// �������Ϊ ����
	if (OP2_TYPE == IS_CONST) {
		// windows: �޲���
		SAVE_OPLINE();
		// ȡ���������У�op2�ĳ���
		zend_string *arg_name = Z_STR_P(RT_CONSTANT(opline, opline->op2));
		uint32_t arg_num;
		// ��������������p1:ִ������ָ���ָ�룬p2:��������p3:�������� ��p4:����λ��
		arg = zend_handle_named_arg(&EX(call), arg_name, &arg_num, CACHE_ADDR(opline->result.num));
		// �����ȡʧ��
		if (UNEXPECTED(!arg)) {
			// �ͷŲ�������ĸ��ӱ���
			FREE_OP1();
			// ��Ҫ�� return
			HANDLE_EXCEPTION();
		}
	// ����op2���Ͳ��ǳ���
	} else {
		// ��p1 ����p2�ֽڣ�ת��zval����
		arg = ZEND_CALL_VAR(EX(call), opline->result.var);
	}

	// ��ȡzvalָ�룬UNUSED ���ͷ���null
	value = GET_OP1_ZVAL_PTR(BP_VAR_R);
	// op1���Ƹ� result
	ZVAL_COPY_VALUE(arg, value);
	// ���op1�ǳ���
	if (OP1_TYPE == IS_CONST) {
		// ����ǿ��Լ�������
		if (UNEXPECTED(Z_OPT_REFCOUNTED_P(arg))) {
			// ���ô���+1
			Z_ADDREF_P(arg);
		}
	}
	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, �״��˲���������ʹ�����ô���
ZEND_VM_COLD_HELPER(zend_cannot_pass_by_ref_helper, ANY, ANY, uint32_t _arg_num, zval *_arg)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	// windows: �޲���
	SAVE_OPLINE();
	// �״��˲���������ʹ�����ô���
	zend_cannot_pass_by_reference(_arg_num);
	// �ͷŲ�������ĸ��ӱ���
	FREE_OP1();
	// ���� Ϊ δ����
	ZVAL_UNDEF(_arg);
	// ��Ҫ�� return
	HANDLE_EXCEPTION();
}

// ing2, ���鲢ʹ�ú�������
ZEND_VM_HOT_SEND_HANDLER(116, ZEND_SEND_VAL_EX, CONST|TMP, CONST|UNUSED|NUM, SPEC(QUICK_ARG))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *value, *arg;
	uint32_t arg_num;

	// ��� op2 ����Ϊ ����
	if (OP2_TYPE == IS_CONST) {
		// windows: �޲���
		SAVE_OPLINE();
		// ���� (p2).zv��p1:opline,p2:node
		zend_string *arg_name = Z_STR_P(RT_CONSTANT(opline, opline->op2));
		// ��������������p1:ִ������ָ���ָ�룬p2:��������p3:�������� ��p4:����λ��
		arg = zend_handle_named_arg(&EX(call), arg_name, &arg_num, CACHE_ADDR(opline->result.num));
		// ���û�л�ȡ������
		if (UNEXPECTED(!arg)) {
			// �ͷŲ�������ĸ��ӱ���
			FREE_OP1();
			// ��Ҫ�� return
			HANDLE_EXCEPTION();
		}
	// ����, op2 ���ǳ���
	} else {
		// ��p1 ����p2�ֽڣ�ת��zval����
		arg = ZEND_CALL_VAR(EX(call), opline->result.var);
		// ��op2��ȡ���������
		arg_num = opline->op2.num;
	}

	// �����������û�г���
	if (EXPECTED(arg_num <= MAX_ARG_FLAG_NUM)) {
		// ������� Ҫ��ֻ��ͨ�����ô���
		if (QUICK_ARG_MUST_BE_SENT_BY_REF(EX(call)->func, arg_num)) {
			// ��ת��ָ����ǩ send_val_by_ref
			ZEND_VM_C_GOTO(send_val_by_ref);
		}
	// ����������ޣ�����Ҫ�����ô���	
	} else if (ARG_MUST_BE_SENT_BY_REF(EX(call)->func, arg_num)) {
// ������ת��ǩ send_val_by_ref
ZEND_VM_C_LABEL(send_val_by_ref):
		// �״��˲���������ʹ�����ô���
		// ���÷��� zend_cannot_pass_by_ref_helper
		ZEND_VM_DISPATCH_TO_HELPER(zend_cannot_pass_by_ref_helper, _arg_num, arg_num, _arg, arg);
	}
	// ��ȡzvalָ�룬UNUSED ���ͷ���null
	value = GET_OP1_ZVAL_PTR(BP_VAR_R);
	// ʹ�� value�� ����
	ZVAL_COPY_VALUE(arg, value);
	// ���op1�ǳ���
	if (OP1_TYPE == IS_CONST) {
		// ����ǿ��Լ�������
		if (UNEXPECTED(Z_OPT_REFCOUNTED_P(arg))) {
			// ���ô���+1
			Z_ADDREF_P(arg);
		}
	}
	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, op1���Ƹ�result ,��У��op1���ͽ����á�op2��ֵ˵������������
ZEND_VM_HOT_HANDLER(117, ZEND_SEND_VAR, VAR|CV, CONST|UNUSED|NUM)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *varptr, *arg;

	// �������Ϊ ����
	if (OP2_TYPE == IS_CONST) {
		// windows: �޲���
		SAVE_OPLINE();
		// ���� (p2).zv��p1:opline,p2:node
		zend_string *arg_name = Z_STR_P(RT_CONSTANT(opline, opline->op2));
		uint32_t arg_num;
		// ��������������p1:ִ������ָ���ָ�룬p2:��������p3:�������� ��p4:����λ��
		arg = zend_handle_named_arg(&EX(call), arg_name, &arg_num, CACHE_ADDR(opline->result.num));
		// �����ȡ���ʧ��
		if (UNEXPECTED(!arg)) {
			// �ͷŲ�������ĸ��ӱ���
			FREE_OP1();
			// ��Ҫ�� return
			HANDLE_EXCEPTION();
		}
	// ����
	} else {
		// ��p1 ����p2�ֽڣ�ת��zval����
		arg = ZEND_CALL_VAR(EX(call), opline->result.var);
	}

	// ��ȡzvalָ��, UNUSED ����null
	varptr = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ��� op1 �� ������� ���ұ���ָ�� ����Ϊ δ����
	if (OP1_TYPE == IS_CV && UNEXPECTED(Z_TYPE_INFO_P(varptr) == IS_UNDEF)) {
		// windows: �޲���
		SAVE_OPLINE();
		// ����p1�ı�������������δ����, ����δ��ʼ��zval
		ZVAL_UNDEFINED_OP1();
		// ���Ϊnull
		ZVAL_NULL(arg);
		// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
		ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
	}

	// ��� op1 �� �������
	if (OP1_TYPE == IS_CV) {
		// ��������Ŀ�� �� ������Ϣ�����������ü���
		ZVAL_COPY_DEREF(arg, varptr);
	// ���� ��� op1 �� ��ͨ����
	} else /* if (OP1_TYPE == IS_VAR) */ {
		if (UNEXPECTED(Z_ISREF_P(varptr))) {
			// ȡ�� ����ʵ��
			zend_refcounted *ref = Z_COUNTED_P(varptr);
			// ������
			varptr = Z_REFVAL_P(varptr);
			// ���ƽ�� 
			ZVAL_COPY_VALUE(arg, varptr);
			// ���ô���-1�����Ϊ0
			if (UNEXPECTED(GC_DELREF(ref) == 0)) {
				// �ͷ�����ʵ��
				efree_size(ref, sizeof(zend_reference));
			// ���� ����ǿ��Լ�������
			} else if (Z_OPT_REFCOUNTED_P(arg)) {
				// ���ô���+1
				Z_ADDREF_P(arg);
			}
		// ����
		} else {
			// ���Ʋ� �������ô���
			ZVAL_COPY_VALUE(arg, varptr);
		}
	}

	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, op1���Ƹ�result����֧����������
ZEND_VM_HANDLER(106, ZEND_SEND_VAR_NO_REF, VAR, CONST|UNUSED|NUM)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *varptr, *arg;

	// �������Ϊ ����
	if (OP2_TYPE == IS_CONST) {
		// windows: �޲���
		SAVE_OPLINE();
		// ���� (p2).zv��p1:opline,p2:node
		zend_string *arg_name = Z_STR_P(RT_CONSTANT(opline, opline->op2));
		uint32_t arg_num;
		// ��������������p1:ִ������ָ���ָ�룬p2:��������p3:�������� ��p4:����λ��
		arg = zend_handle_named_arg(&EX(call), arg_name, &arg_num, CACHE_ADDR(opline->result.num));
		// �����ȡʧ��
		if (UNEXPECTED(!arg)) {
			// �ͷŲ�������ĸ��ӱ���
			FREE_OP1();
			// ��Ҫ�� return
			HANDLE_EXCEPTION();
		}
	// ����
	} else {
		// ��p1 ����p2�ֽڣ�ת��zval����
		arg = ZEND_CALL_VAR(EX(call), opline->result.var);
	}

	// ��ȡzvalָ�룬UNUSED ���ͷ���null
	varptr = GET_OP1_ZVAL_PTR(BP_VAR_R);
	// op1���Ƹ�result
	ZVAL_COPY_VALUE(arg, varptr);

	// ���op1����������
	if (EXPECTED(Z_ISREF_P(varptr))) {
		// opline + 1����Ŀ������벢 return
		ZEND_VM_NEXT_OPCODE();
	}

	// windows: �޲���
	SAVE_OPLINE();
	// �� zval �����µ� zend_reference ���Ӹ����� zval �и��Ƹ�������
	ZVAL_NEW_REF(arg, arg);
	// ����ֻ�б�������ͨ�����ô���
	zend_error(E_NOTICE, "Only variables should be passed by reference");
	// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, op1���Ƹ�result,���ݷ��������͵ı���
ZEND_VM_HOT_SEND_HANDLER(50, ZEND_SEND_VAR_NO_REF_EX, VAR, CONST|UNUSED|NUM, SPEC(QUICK_ARG))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *varptr, *arg;
	uint32_t arg_num;

	// �������Ϊ ����
	if (OP2_TYPE == IS_CONST) {
		// windows: �޲���
		SAVE_OPLINE();
		// ���� (p2).zv��p1:opline,p2:node
		zend_string *arg_name = Z_STR_P(RT_CONSTANT(opline, opline->op2));
		// ��������������p1:ִ������ָ���ָ�룬p2:��������p3:�������� ��p4:����λ��
		arg = zend_handle_named_arg(&EX(call), arg_name, &arg_num, CACHE_ADDR(opline->result.num));
		// �����ȡʧ��
		if (UNEXPECTED(!arg)) {
			// �ͷŲ�������ĸ��ӱ���
			FREE_OP1();
			// ��Ҫ�� return
			HANDLE_EXCEPTION();
		}
	// ����
	} else {
		// ��p1 ����p2�ֽڣ�ת��zval����
		arg = ZEND_CALL_VAR(EX(call), opline->result.var);
		// �������
		arg_num = opline->op2.num;
	}

	// ����������� û�г�����
	if (EXPECTED(arg_num <= MAX_ARG_FLAG_NUM)) {
		// ���������ʹ�����ô���
		if (!QUICK_ARG_SHOULD_BE_SENT_BY_REF(EX(call)->func, arg_num)) {
			// ��ת��ָ����ǩ send_val_by_ref
			ZEND_VM_C_GOTO(send_var);
		}

		// ��ȡzvalָ�룬UNUSED ���ͷ���null
		varptr = GET_OP1_ZVAL_PTR(BP_VAR_R);
		// op1���Ƹ�result
		ZVAL_COPY_VALUE(arg, varptr);

		// ���op1���������� �� ��������ͨ�����ô���
		if (EXPECTED(Z_ISREF_P(varptr) ||
		    QUICK_ARG_MAY_BE_SENT_BY_REF(EX(call)->func, arg_num))) {
			// opline + 1����Ŀ������벢 return
			ZEND_VM_NEXT_OPCODE();
		}
	// ���򣬲�������������
	} else {
		// �������������ʹ�����ô���
		if (!ARG_SHOULD_BE_SENT_BY_REF(EX(call)->func, arg_num)) {
			// ��ת��ָ����ǩ send_var
			ZEND_VM_C_GOTO(send_var);
		}

		// ��ȡzvalָ�룬UNUSED ���ͷ���null
		varptr = GET_OP1_ZVAL_PTR(BP_VAR_R);
		// op1���Ƹ�result
		ZVAL_COPY_VALUE(arg, varptr);

		// ���op1���������� �� ��������ʹ�����ô���
		if (EXPECTED(Z_ISREF_P(varptr) ||
			ARG_MAY_BE_SENT_BY_REF(EX(call)->func, arg_num))) {
			// opline + 1����Ŀ������벢 return
			ZEND_VM_NEXT_OPCODE();
		}
	}

	// windows: �޲���
	SAVE_OPLINE();
	// �� zval �����µ� zend_reference ���Ӹ����� zval �и��Ƹ�������
	ZVAL_NEW_REF(arg, arg);
	// ��ʾ��ֻ�б����������ô���
	zend_error(E_NOTICE, "Only variables should be passed by reference");
	// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();

// ������ת��ǩ send_var
ZEND_VM_C_LABEL(send_var):
	// ��ȡzvalָ�룬UNUSED ���ͷ���null
	varptr = GET_OP1_ZVAL_PTR(BP_VAR_R);
	// �����������������
	if (UNEXPECTED(Z_ISREF_P(varptr))) {
		zend_refcounted *ref = Z_COUNTED_P(varptr);
		// ������
		varptr = Z_REFVAL_P(varptr);
		ZVAL_COPY_VALUE(arg, varptr);
		// ���ô���-1�����Ϊ0
		if (UNEXPECTED(GC_DELREF(ref) == 0)) {
			efree_size(ref, sizeof(zend_reference));
		// ���� ����ǿ��Լ�������
		} else if (Z_OPT_REFCOUNTED_P(arg)) {
			// ���ô���+1
			Z_ADDREF_P(arg);
		}
	// ����
	} else {
		ZVAL_COPY_VALUE(arg, varptr);
	}
	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, op1���Ƹ�result����������ת�����ô���
ZEND_VM_HANDLER(67, ZEND_SEND_REF, VAR|CV, CONST|UNUSED|NUM)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *varptr, *arg;

	// windows: �޲���
	SAVE_OPLINE();
	// �������Ϊ ����
	if (OP2_TYPE == IS_CONST) {
		// ������
		// ���� (p2).zv��p1:opline,p2:node
		zend_string *arg_name = Z_STR_P(RT_CONSTANT(opline, opline->op2));
		// 
		uint32_t arg_num;
		// ��������������p1:ִ������ָ���ָ�룬p2:��������p3:�������� ��p4:����λ��
		arg = zend_handle_named_arg(&EX(call), arg_name, &arg_num, CACHE_ADDR(opline->result.num));
		// ���û�ҵ�����
		if (UNEXPECTED(!arg)) {
			// �ͷŲ�������ĸ��ӱ���
			FREE_OP1();
			// ��Ҫ�� return
			HANDLE_EXCEPTION();
		}
	// ����
	} else {
		// ��p1 ����p2�ֽڣ�ת��zval����
		arg = ZEND_CALL_VAR(EX(call), opline->result.var);
	}
	// ��ȡzvalָ��,TMP/CONST/UNUSED ����null����֧�� TMPVAR/TMPVARCV
	varptr = GET_OP1_ZVAL_PTR_PTR(BP_VAR_W);
	// ���op1����������
	if (Z_ISREF_P(varptr)) {
		// ���ô���+1
		Z_ADDREF_P(varptr);
	// ����
	} else {
		// ��zvalתΪ���ã�p1:zval��p2:���ô���
		ZVAL_MAKE_REF_EX(varptr, 2);
	}
	// op1������Ŀ�꣬���ø�ֵ�� result
	ZVAL_REF(arg, Z_REF_P(varptr));

	// �ͷŲ�������ĸ��ӱ���
	FREE_OP1();
	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// op1 ���Ƹ� result��֧����������
ZEND_VM_HOT_SEND_HANDLER(66, ZEND_SEND_VAR_EX, VAR|CV, CONST|UNUSED|NUM, SPEC(QUICK_ARG))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *varptr, *arg;
	uint32_t arg_num;

	// �������Ϊ ��������������
	if (OP2_TYPE == IS_CONST) {
		// windows: �޲���
		SAVE_OPLINE();
		// ������
		// ���� (p2).zv��p1:opline,p2:node
		zend_string *arg_name = Z_STR_P(RT_CONSTANT(opline, opline->op2));
		// ��������������p1:ִ������ָ���ָ�룬p2:��������p3:�������� ��p4:����λ��
		arg = zend_handle_named_arg(&EX(call), arg_name, &arg_num, CACHE_ADDR(opline->result.num));
		// ���û�л�ȡ������
		if (UNEXPECTED(!arg)) {
			// �ͷŲ�������ĸ��ӱ���
			FREE_OP1();
			// ��Ҫ�� return
			HANDLE_EXCEPTION();
		}
	// ����
	} else {
		// ��p1 ����p2�ֽڣ�ת��zval����
		arg = ZEND_CALL_VAR(EX(call), opline->result.var);
		// �������
		arg_num = opline->op2.num;
	}

	// �����������û�г�����
	if (EXPECTED(arg_num <= MAX_ARG_FLAG_NUM)) {
		// ��������������ô���
		if (QUICK_ARG_SHOULD_BE_SENT_BY_REF(EX(call)->func, arg_num)) {
			// ��ת��ָ����ǩ send_var_by_ref
			ZEND_VM_C_GOTO(send_var_by_ref);
		}
	// ������������ƣ����Ҳ����������ô���
	} else if (ARG_SHOULD_BE_SENT_BY_REF(EX(call)->func, arg_num)) {
// ������ת��ǩ send_var_by_ref
ZEND_VM_C_LABEL(send_var_by_ref):
		// ��ȡzvalָ��,TMP/CONST/UNUSED ����null����֧�� TMPVAR/TMPVARCV
		varptr = GET_OP1_ZVAL_PTR_PTR(BP_VAR_W);
		// ���op1����������
		if (Z_ISREF_P(varptr)) {
			// ���ô���+1
			Z_ADDREF_P(varptr);
		// ����
		} else {
			// ��zvalתΪ���ã�p1:zval��p2:���ô���
			ZVAL_MAKE_REF_EX(varptr, 2);
		}
		// ���ø�ֵ op1��Ŀ��� result
		ZVAL_REF(arg, Z_REF_P(varptr));

		// �ͷŲ�������ĸ��ӱ���
		FREE_OP1();
		// opline + 1����Ŀ������벢 return
		ZEND_VM_NEXT_OPCODE();
	}

	// ��ȡzvalָ��, UNUSED ����null
	varptr = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ��� op1 �� ������� ���� ֵΪδ����
	if (OP1_TYPE == IS_CV && UNEXPECTED(Z_TYPE_INFO_P(varptr) == IS_UNDEF)) {
		// windows: �޲���
		SAVE_OPLINE();
		// ����p1�ı�������������δ����, ����δ��ʼ��zval
		ZVAL_UNDEFINED_OP1();
		// ���Ϊnull
		ZVAL_NULL(arg);
		// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
		ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
	}

	// ��� op1 �� �������
	if (OP1_TYPE == IS_CV) {
		// ��������Ŀ�� �� ������Ϣ�����������ü���
		ZVAL_COPY_DEREF(arg, varptr);
	// ���� ��� op1 �� ��ͨ����
	} else /* if (OP1_TYPE == IS_VAR) */ {
		if (UNEXPECTED(Z_ISREF_P(varptr))) {
			// ȡ��op1������ʵ��
			zend_refcounted *ref = Z_COUNTED_P(varptr);
			// ������
			varptr = Z_REFVAL_P(varptr);
			// op1 ���Ƹ� result
			ZVAL_COPY_VALUE(arg, varptr);
			// ���ô���-1�����Ϊ0
			if (UNEXPECTED(GC_DELREF(ref) == 0)) {
				// �ͷ�����ʵ��
				efree_size(ref, sizeof(zend_reference));
			// ���� ����ǿ��Լ�������
			} else if (Z_OPT_REFCOUNTED_P(arg)) {
				// ���ô���+1
				Z_ADDREF_P(arg);
			}
		// ����
		} else {
			// op1���Ƹ�result
			ZVAL_COPY_VALUE(arg, varptr);
		}
	}

	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, �������Ƿ�ͨ�����ô��ݣ�����Ҫ��� ZEND_CALL_SEND_ARG_BY_REF ���
ZEND_VM_HOT_SEND_HANDLER(100, ZEND_CHECK_FUNC_ARG, UNUSED, CONST|UNUSED|NUM, SPEC(QUICK_ARG))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	uint32_t arg_num;

	// ���op2����Ϊ ����
	if (OP2_TYPE == IS_CONST) {
		// ������
		// ���� (p2).zv��p1:opline,p2:node
		zend_string *arg_name = Z_STR_P(RT_CONSTANT(opline, opline->op2));
		// ͨ���������ƻ�ȡƫ������������ţ�
		arg_num = zend_get_arg_offset_by_name(
			EX(call)->func, arg_name, CACHE_ADDR(opline->result.num)) + 1;
		// �������������0
		if (UNEXPECTED(arg_num == 0)) {
			// �����������ò�������SENDʱ�׳�����
			/* Treat this as a by-value argument, and throw an error during SEND. */
			// ������ô��ݲ��������
			ZEND_DEL_CALL_FLAG(EX(call), ZEND_CALL_SEND_ARG_BY_REF);
			// opline + 1����Ŀ������벢 return
			ZEND_VM_NEXT_OPCODE();
		}
	// ����op2���ǳ���
	} else {
		// �������� 
		arg_num = opline->op2.num;
	}

	// ������û��Խ��
	if (EXPECTED(arg_num <= MAX_ARG_FLAG_NUM)) {
		// �����Ƿ����ͨ�����ô��ݣ��������
		if (QUICK_ARG_SHOULD_BE_SENT_BY_REF(EX(call)->func, arg_num)) {
			// ��ӡ��������ò��������
			ZEND_ADD_CALL_FLAG(EX(call), ZEND_CALL_SEND_ARG_BY_REF);
		// ����
		} else {
			// ɾ�����������ò��������
			ZEND_DEL_CALL_FLAG(EX(call), ZEND_CALL_SEND_ARG_BY_REF);
		}
	// �������ͨ�����ô���
	} else if (ARG_SHOULD_BE_SENT_BY_REF(EX(call)->func, arg_num)) {
		// ��ӡ��������ò��������
		ZEND_ADD_CALL_FLAG(EX(call), ZEND_CALL_SEND_ARG_BY_REF);
	// ����
	} else {
		// ɾ�����������ò��������
		ZEND_DEL_CALL_FLAG(EX(call), ZEND_CALL_SEND_ARG_BY_REF);
	}
	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, op1ֱ�Ӹ��Ƹ�result, ���ݲ��������Զ��������ô���
ZEND_VM_HOT_HANDLER(185, ZEND_SEND_FUNC_ARG, VAR, CONST|UNUSED|NUM)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *varptr, *arg;

	// �������Ϊ ����
	if (OP2_TYPE == IS_CONST) {
		// TODO: Would it make sense to share the cache slot with CHECK_FUNC_ARG?
		// windows: �޲���
		SAVE_OPLINE();
		// ���� (p2).zv��p1:opline,p2:node
		zend_string *arg_name = Z_STR_P(RT_CONSTANT(opline, opline->op2));
		uint32_t arg_num;
		// ��������������p1:ִ������ָ���ָ�룬p2:��������p3:�������� ��p4:����λ��
		arg = zend_handle_named_arg(&EX(call), arg_name, &arg_num, CACHE_ADDR(opline->result.num));
		// �����ȡ����ʧ��
		if (UNEXPECTED(!arg)) {
			// �ͷŲ�������ĸ��ӱ���
			FREE_OP1();
			// ��Ҫ�� return
			HANDLE_EXCEPTION();
		}
	// ����
	} else {
		// ��p1 ����p2�ֽڣ�ת��zval����
		arg = ZEND_CALL_VAR(EX(call), opline->result.var);
	}

	// ���Ҫ�����ô���
	if (UNEXPECTED(ZEND_CALL_INFO(EX(call)) & ZEND_CALL_SEND_ARG_BY_REF)) {
		// ��ȡzvalָ��,TMP/CONST/UNUSED ����null����֧�� TMPVAR/TMPVARCV
		varptr = GET_OP1_ZVAL_PTR_PTR(BP_VAR_W);
		// �������������
		if (Z_ISREF_P(varptr)) {
			// ���ô���+1
			Z_ADDREF_P(varptr);
		// ����
		} else {
			// ��zvalתΪ���ã�p1:zval��p2:���ô���
			ZVAL_MAKE_REF_EX(varptr, 2);
		}
		// op1������Ŀ�� ���ø�ֵ�� result
		ZVAL_REF(arg, Z_REF_P(varptr));

		// �ͷŲ�������ĸ��ӱ���
		FREE_OP1();
		// opline + 1����Ŀ������벢 return
		ZEND_VM_NEXT_OPCODE();
	}

	// ��ȡzvalָ��, UNUSED ����null
	varptr = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);

	// ��� op1����������
	if (UNEXPECTED(Z_ISREF_P(varptr))) {
		// ȡ������ʵ��
		zend_refcounted *ref = Z_COUNTED_P(varptr);
		// ������
		varptr = Z_REFVAL_P(varptr);
		// Ŀ��ֵ���Ƹ� result
		ZVAL_COPY_VALUE(arg, varptr);
		// ���ô���-1�����Ϊ0
		if (UNEXPECTED(GC_DELREF(ref) == 0)) {
			// �ͷ�����ʵ��
			efree_size(ref, sizeof(zend_reference));
		// ���� ����ǿ��Լ�������
		} else if (Z_OPT_REFCOUNTED_P(arg)) {
			// ���ô���+1
			Z_ADDREF_P(arg);
		}
	// ����op1������������
	} else {
		// op1ֱ�Ӹ��Ƹ�result
		ZVAL_COPY_VALUE(arg, varptr);
	}

	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing2, ���ͽ������
ZEND_VM_HANDLER(165, ZEND_SEND_UNPACK, ANY, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *args;
	uint32_t arg_num;

	// windows: �޲���
	SAVE_OPLINE();
	// ��ȡzvalָ��, UNUSED ����null
	args = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ִ�������У��������� +1
	arg_num = ZEND_CALL_NUM_ARGS(EX(call)) + 1;

// ������ת��ǩ send_again
ZEND_VM_C_LABEL(send_again):
	// ��� op1 ������ 
	if (EXPECTED(Z_TYPE_P(args) == IS_ARRAY)) {
		// ȡ������ 
		HashTable *ht = Z_ARRVAL_P(args);
		zval *arg, *top;
		zend_string *name;
		// ���������� 0
		bool have_named_params = 0;

		// ��չִ�����ݡ��������ջǰ�ռ乻�ã�����ʹ������
		// �����½���ջ������ִ�����ݡ�p1:ִ�����ݣ�p2:���������p3:���Ӳ���
		zend_vm_stack_extend_call_frame(&EX(call), arg_num - 1, zend_hash_num_elements(ht));

		// TODO: Speed this up using a flag that specifies whether there are any ref parameters.
		// ��� op1 ����ͨ������������ ���� op1����������
		if ((OP1_TYPE & (IS_VAR|IS_CV)) && Z_REFCOUNT_P(args) > 1) {
			// 
			uint32_t tmp_arg_num = arg_num;
			bool separate = 0;
			// ����Ƿ��в���Ҫʹ�����ô���
			/* check if any of arguments are going to be passed by reference */
			// ������ϣ��
			ZEND_HASH_FOREACH_STR_KEY_VAL(ht, name, arg) {
				// �����key
				if (UNEXPECTED(name)) {
					// ִ����Ҫ����ʱ����
					void *cache_slot[2] = {NULL, NULL};
					// ͨ���������ƻ�ȡƫ������������ţ�
					tmp_arg_num = zend_get_arg_offset_by_name(
						EX(call)->func, name, cache_slot) + 1;
				}
				// �������ʹ�����ô���
				if (ARG_SHOULD_BE_SENT_BY_REF(EX(call)->func, tmp_arg_num)) {
					// ��Ҫ��������
					separate = 1;
					// ����
					break;
				}
				// ��ʱ������
				tmp_arg_num++;
			} ZEND_HASH_FOREACH_END();
			// �����Ҫ��������
			if (separate) {
				// args����������ʹ�ø���
				SEPARATE_ARRAY(args);
				// ȡ����ϣ��
				ht = Z_ARRVAL_P(args);
			}
		}

		// ���� ��ϣ��
		ZEND_HASH_FOREACH_STR_KEY_VAL(ht, name, arg) {
			// �����key
			if (UNEXPECTED(name)) {
				void *cache_slot[2] = {NULL, NULL};
				// �������� 1��
				have_named_params = 1;
				// ��������������p1:ִ������ָ���ָ�룬p2:��������p3:�������� ��p4:����λ��
				top = zend_handle_named_arg(&EX(call), name, &arg_num, cache_slot);
				// ��� top ��Ч
				if (UNEXPECTED(!top)) {
					// �ͷŲ�������ĸ��ӱ���
					FREE_OP1();
					// ��Ҫ�� return
					HANDLE_EXCEPTION();
				}
			// ����,û��key
			} else {
				// �������������
				if (have_named_params) {
					// ���������԰�˳���������������������
					zend_throw_error(NULL,
						"Cannot use positional argument after named argument during unpacking");
					// �ͷŲ�������ĸ��ӱ���
					FREE_OP1();
					// ��Ҫ�� return
					HANDLE_EXCEPTION();
				}
				// ȡ���б��е� ��ZEND_CALL_FRAME_SLOT +�� n �� zval
				top = ZEND_CALL_ARG(EX(call), arg_num);
				// ִ�������в������� +1
				ZEND_CALL_NUM_ARGS(EX(call))++;
			}

			// ������������ô���
			if (ARG_SHOULD_BE_SENT_BY_REF(EX(call)->func, arg_num)) {
				// �������������
				if (Z_ISREF_P(arg)) {
					// ���ô���+1
					Z_ADDREF_P(arg);
					// arg��Ŀ�꣬���ø�ֵ��top
					ZVAL_REF(top, Z_REF_P(arg));
				// ���� ��� op1 ����ͨ������������
				} else if (OP1_TYPE & (IS_VAR|IS_CV)) {
					// array�Ѿ������洴���˸���
					/* array is already separated above */
					// ��zvalתΪ���ã�p1:zval��p2:���ô���
					ZVAL_MAKE_REF_EX(arg, 2);
					// arg��Ŀ�꣬���ø�ֵ��top
					ZVAL_REF(top, Z_REF_P(arg));
				// ����
				} else {
					// �������ô���
					Z_TRY_ADDREF_P(arg);
					// �� zval �����µ� zend_reference ���Ӹ����� zval �и��Ƹ�������
					ZVAL_NEW_REF(top, arg);
				}
			// ����
			} else {
				// ��������Ŀ�� �� ������Ϣ�����������ü���
				ZVAL_COPY_DEREF(top, arg);
			}
			// ��ʱ��������
			arg_num++;
		} ZEND_HASH_FOREACH_END();
	// ����Ƕ���
	} else if (EXPECTED(Z_TYPE_P(args) == IS_OBJECT)) {
		// ����������
		zend_class_entry *ce = Z_OBJCE_P(args);
		//
		zend_object_iterator *iter;
		// ������������ 0
		bool have_named_params = 0;

		// ����޷�ȡ�õ�����
		if (!ce || !ce->get_iterator) {
			// ����ֻ�����ݺͿɱ������Ϳɱ����
			zend_type_error("Only arrays and Traversables can be unpacked");
		// ���򣬿���ȡ�õ�����
		} else {
			// ȡ�õ�����
			iter = ce->get_iterator(ce, args, 0);
			// ���û�л�ȡ��
			if (UNEXPECTED(!iter)) {
				// �ͷŲ�������ĸ��ӱ���
				FREE_OP1();
				// ���û���쳣
				if (!EG(exception)) {
					// ���쳣�������͵Ķ����޷�����������
					zend_throw_exception_ex(
						NULL, 0, "Object of type %s did not create an Iterator", ZSTR_VAL(ce->name)
					);
				}
				// ��Ҫ�� return
				HANDLE_EXCEPTION();
			}
			// �����������б�
			const zend_object_iterator_funcs *funcs = iter->funcs;
			// ��������÷���
			if (funcs->rewind) {
				// ������
				funcs->rewind(iter);
			}

			// �������λ����Ч����һֱ������ȥ��arg_num ����
			for (; funcs->valid(iter) == SUCCESS; ++arg_num) {
				zval *arg, *top;

				// ������쳣
				if (UNEXPECTED(EG(exception) != NULL)) {
					// ����
					break;
				}
				// ȡ����ǰ����
				arg = funcs->get_current_data(iter);
				// ������쳣
				if (UNEXPECTED(EG(exception) != NULL)) {
					// ����
					break;
				}
				// 
				zend_string *name = NULL;
				// ����ܻ�ȡ��ǰkey
				if (funcs->get_current_key) {
					zval key;
					// ��ȡ��ǰkey
					funcs->get_current_key(iter, &key);
					// ������쳣
					if (UNEXPECTED(EG(exception) != NULL)) {
						// ����
						break;
					}
					// ���key��������
					if (UNEXPECTED(Z_TYPE(key) != IS_LONG)) {
						// ���key�����ִ�
						if (UNEXPECTED(Z_TYPE(key) != IS_STRING)) {
							// �������ʱkey����������������
							zend_throw_error(NULL,
								"Keys must be of type int|string during argument unpacking");
							// ���� key
							zval_ptr_dtor(&key);
							// ����
							break;
						}
						// ȡ��key �ִ�
						name = Z_STR_P(&key);
					}
				}

				// ���ȡ����key��������
				if (UNEXPECTED(name)) {
					// ִ����Ҫ����ʱ����
					void *cache_slot[2] = {NULL, NULL};
					// ����������Ĭ��1��
					have_named_params = 1;
					// ��������������p1:ִ������ָ���ָ�룬p2:��������p3:�������� ��p4:����λ��
					top = zend_handle_named_arg(&EX(call), name, &arg_num, cache_slot);
					// �������������
					if (UNEXPECTED(!top)) {
						// �ͷű�����
						zend_string_release(name);
						// ����
						break;
					}
					// �������ô���
					ZVAL_DEREF(arg);
					// �������ô���
					Z_TRY_ADDREF_P(arg);
					// �����������ʹ�����ô���
					if (ARG_MUST_BE_SENT_BY_REF(EX(call)->func, arg_num)) {
						// ����������ͨ��������������ò�����ֱ�Ӵ������ò������ɡ�
						zend_error(
							E_WARNING, "Cannot pass by-reference argument %d of %s%s%s()"
							" by unpacking a Traversable, passing by-value instead", arg_num,
							EX(call)->func->common.scope ? ZSTR_VAL(EX(call)->func->common.scope->name) : "",
							EX(call)->func->common.scope ? "::" : "",
							ZSTR_VAL(EX(call)->func->common.function_name)
						);
						// �� zval �����µ� zend_reference ���Ӹ����� zval �и��Ƹ�������
						ZVAL_NEW_REF(top, arg);
					// ����
					} else {
						// ��
						ZVAL_COPY_VALUE(top, arg);
					}
					// �ͷű�����
					zend_string_release(name);
				// ����û�б�����
				} else {
					// �������������
					if (have_named_params) {
						// ���������԰�˳���������������������
						zend_throw_error(NULL,
							"Cannot use positional argument after named argument during unpacking");
						// ����
						break;
					}
					// ��չִ�����ݡ��������ջǰ�ռ乻�ã�����ʹ������
					// �����½���ջ������ִ�����ݡ�p1:ִ�����ݣ�p2:���������p3:���Ӳ���
					zend_vm_stack_extend_call_frame(&EX(call), arg_num - 1, 1);
					// ȡ���б��е� ��ZEND_CALL_FRAME_SLOT +�� n �� zval
					top = ZEND_CALL_ARG(EX(call), arg_num);
					// �������ô���
					ZVAL_DEREF(arg);
					// �������ô���
					Z_TRY_ADDREF_P(arg);
					// ����������������ô���
					if (ARG_MUST_BE_SENT_BY_REF(EX(call)->func, arg_num)) {
						// ���棺���������ô��ݣ���
						zend_error(
							E_WARNING, "Cannot pass by-reference argument %d of %s%s%s()"
							" by unpacking a Traversable, passing by-value instead", arg_num,
							EX(call)->func->common.scope ? ZSTR_VAL(EX(call)->func->common.scope->name) : "",
							EX(call)->func->common.scope ? "::" : "",
							ZSTR_VAL(EX(call)->func->common.function_name)
						);
						// �� zval �����µ� zend_reference ���Ӹ����� zval �и��Ƹ�������
						ZVAL_NEW_REF(top, arg);
					// ���򣬲����������ô���
					} else {
						// arg ���Ƹ� top
						ZVAL_COPY_VALUE(top, arg);
					}

					// ִ�������в������� +1
					ZEND_CALL_NUM_ARGS(EX(call))++;
				}

				// ���� ��һ��Ԫ��
				funcs->move_forward(iter);
			}
			// ���ٵ�����
			zend_iterator_dtor(iter);
		}
	// �������������
	} else if (EXPECTED(Z_ISREF_P(args))) {
		// ������
		args = Z_REFVAL_P(args);
		// ��ת��ָ����ǩ send_again
		ZEND_VM_C_GOTO(send_again);
	// ����
	} else {
		// ��� op1 �� ������� ���� ����ֵΪδ����
		if (OP1_TYPE == IS_CV && UNEXPECTED(Z_TYPE_P(args) == IS_UNDEF)) {
			// ����p1�ı�������������δ����, ����δ��ʼ��zval
			ZVAL_UNDEFINED_OP1();
		}
		// ����ֻ�����ݺͿɱ������Ϳɱ����
		zend_type_error("Only arrays and Traversables can be unpacked");
	}

	// �ͷŲ�������ĸ��ӱ���
	FREE_OP1();
	// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing, ��������
ZEND_VM_HANDLER(119, ZEND_SEND_ARRAY, ANY, ANY, NUM)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *args;

	// windows: �޲���
	SAVE_OPLINE();
	// ��ȡzvalָ�룬UNUSED ���ͷ���null
	args = GET_OP1_ZVAL_PTR(BP_VAR_R);
	// �����������array
	if (UNEXPECTED(Z_TYPE_P(args) != IS_ARRAY)) {
		// #define IS_VAR (1<<2)
		// #define IS_CV (1<<3) /* Compiled variable */
		// ���op1�Ǳ����������������Ҳ����б�����������
		if ((OP1_TYPE & (IS_VAR|IS_CV)) && Z_ISREF_P(args)) {
			// ׷�ٵ�����
			args = Z_REFVAL_P(args);
			// ���׷�ٺ�Ĳ����б��� array
			if (EXPECTED(Z_TYPE_P(args) == IS_ARRAY)) {
				// ��ת��ָ����ǩ send_array
				ZEND_VM_C_GOTO(send_array);
			}
		}
		// ����call_user_func_array �ĵڶ����������������� 
		zend_type_error("call_user_func_array(): Argument #2 ($args) must be of type array, %s given", zend_zval_type_name(args));
		// �������op
		// �ͷŲ�������ĸ��ӱ���
		FREE_OP2();
		// �ͷŲ�������ĸ��ӱ���
		FREE_OP1();
		// ��Ҫ�� return
		HANDLE_EXCEPTION();
	// �����������array
	// ����
	} else {
		uint32_t arg_num;
		HashTable *ht;
		zval *arg, *param;

// ing2, ������ת��ǩ send_array
ZEND_VM_C_LABEL(send_array):
		// ȡ�������б�
		ht = Z_ARRVAL_P(args);
		// op2��ֵ���� δʹ��
		if (OP2_TYPE != IS_UNUSED) {
			// ���ﲻ��Ҫ����������������Ϊ array_slice ������ʱ $preserve_keys == false
			/* We don't need to handle named params in this case,
			 * because array_slice() is called with $preserve_keys == false. */
			// ��ȡzvalָ��, UNUSED ����null����֧�� TMPVAR/TMPVARCV
			zval *op2 = GET_OP2_ZVAL_PTR_DEREF(BP_VAR_R);
			// Ҫ�����Ĳ������� 
			uint32_t skip = opline->extended_value;
			// �������� 
			uint32_t count = zend_hash_num_elements(ht);
			zend_long len;
			// ���op2�������� ���� 
			if (EXPECTED(Z_TYPE_P(op2) == IS_LONG)) {
				// ȡ������
				len = Z_LVAL_P(op2);
			// ���op2�������� NULL
			} else if (Z_TYPE_P(op2) == IS_NULL) {
				// ���� - ������
				len = count - skip;
			// ���ʹ���ϸ����ͣ��� 
			// �Ѵ���Ĳ���ת��������������null�ᱨ��p2:����ֵ��p3:arg������ţ������á�ת��ʧ��
			} else if (EX_USES_STRICT_TYPES()
					|| !zend_parse_arg_long_weak(op2, &len, /* arg_num */ 3)) {
				// ����array_slice ����3������ int�ͻ�Ϊnull
				zend_type_error(
					"array_slice(): Argument #3 ($length) must be of type ?int, %s given",
					zend_zval_type_name(op2));
				// �ͷŲ�������ĸ��ӱ���
				FREE_OP2();
				// �ͷŲ�������ĸ��ӱ���
				FREE_OP1();
				// ��Ҫ�� return
				HANDLE_EXCEPTION();
			}

			// ������Ҫ��Ϊ����Ƭ
			// �������С��0 ��������Ƭ
			if (len < 0) {
				// ������Ƭλ��
				len += (zend_long)(count - skip);
			}
			// ���skipû�г�����Чֵ������len��Ч
			if (skip < count && len > 0) {
				// �����Ƭ����Խ��
				if (len > (zend_long)(count - skip)) {
					// �����������ܳ���
					len = (zend_long)(count - skip);
				}
				// ��չִ�����ݡ��������ջǰ�ռ乻�ã�����ʹ������
				// �����½���ջ������ִ�����ݡ�p1:ִ�����ݣ�p2:���������p3:���Ӳ���
				zend_vm_stack_extend_call_frame(&EX(call), 0, len);
				// �������� 1
				arg_num = 1;
				// ȡ���б��е� ��ZEND_CALL_FRAME_SLOT +�� n �� zval
				param = ZEND_CALL_ARG(EX(call), 1);
				// ������ϣ��
				ZEND_HASH_FOREACH_VAL(ht, arg) {
					// Ĭ�������װ������
					bool must_wrap = 0;
					// �����Ҫskip
					if (skip > 0) {
						// ������1��
						skip--;
						// ����
						continue;
					// �����ȡ�����Ѿ����ڵ�����Ҫ������
					} else if ((zend_long)(arg_num - 1) >= len) {
						// ����
						break;
					// ����˲������� ���ô���
					} else if (ARG_SHOULD_BE_SENT_BY_REF(EX(call)->func, arg_num)) {
						// ���������������
						if (UNEXPECTED(!Z_ISREF_P(arg))) {
							// ����˲���������ʹ�����ô���
							if (!ARG_MAY_BE_SENT_BY_REF(EX(call)->func, arg_num)) {
								// ������ʹ�����ô��ݣ��������棬����Ȼ���е���
								/* By-value send is not allowed -- emit a warning,
								 * but still perform the call. */
								// �����˲����������ô���
								zend_param_must_be_ref(EX(call)->func, arg_num);
								// �����װ������
								must_wrap = 1;
							}
						}
					// ���򣬴˲����������ô���
					} else {
						// �������������
						if (Z_ISREF_P(arg) &&
						    !(EX(call)->func->common.fn_flags & ZEND_ACC_CALL_VIA_TRAMPOLINE)) {
							/* don't separate references for __call */
							// ������
							arg = Z_REFVAL_P(arg);
						}
					}
					// ����������װ
					if (EXPECTED(!must_wrap)) {
						// ���Ʋ���
						ZVAL_COPY(param, arg);
					// ����
					} else {
						// �������ô���
						Z_TRY_ADDREF_P(arg);
						// �� zval �����µ� zend_reference ���Ӹ����� zval �и��Ƹ�������
						ZVAL_NEW_REF(param, arg);
					}
					// �������в�������+1
					ZEND_CALL_NUM_ARGS(EX(call))++;
					// ��ȡ���������� +1
					arg_num++;
					// ��һ������
					param++;
				} ZEND_HASH_FOREACH_END();
			}
			// �ͷŲ�������ĸ��ӱ���
			FREE_OP2();
		// ���� op2��ֵ�� δʹ��
		} else {
			zend_string *name;
			bool have_named_params;
			// ��չִ�����ݡ��������ջǰ�ռ乻�ã�����ʹ������
			// �����½���ջ������ִ�����ݡ�p1:ִ�����ݣ�p2:���������p3:���Ӳ���
			zend_vm_stack_extend_call_frame(&EX(call), 0, zend_hash_num_elements(ht));
			// ��������
			arg_num = 1;
			// ȡ���б��е� ��ZEND_CALL_FRAME_SLOT +�� n �� zval
			param = ZEND_CALL_ARG(EX(call), 1);
			// Ĭ��û����������
			have_named_params = 0;
			// ������ϣ��
			ZEND_HASH_FOREACH_STR_KEY_VAL(ht, name, arg) {
				// �����key
				if (name) {
					// ��ʱ����
					void *cache_slot[2] = {NULL, NULL};
					// ����������
					have_named_params = 1;
					// ��������������p1:ִ������ָ���ָ�룬p2:��������p3:�������� ��p4:����λ��
					param = zend_handle_named_arg(&EX(call), name, &arg_num, cache_slot);
					// ���������ȡʧ��
					if (!param) {
						// �ͷŲ�������ĸ��ӱ���
						FREE_OP1();
						// ��Ҫ�� return
						HANDLE_EXCEPTION();
					}
				// ���û��key ������������
				} else if (have_named_params) {
					// �״������԰�˳���������������������
					zend_throw_error(NULL,
						"Cannot use positional argument after named argument");
					// �ͷŲ�������ĸ��ӱ���
					FREE_OP1();
					// ��Ҫ�� return
					HANDLE_EXCEPTION();
				}

				// Ĭ�������װ������
				bool must_wrap = 0;
				// ����˲�������ͨ�����ô���
				if (ARG_SHOULD_BE_SENT_BY_REF(EX(call)->func, arg_num)) {
					// ���ֵ������������
					if (UNEXPECTED(!Z_ISREF_P(arg))) {
						// ����˲��������������ô���
						if (!ARG_MAY_BE_SENT_BY_REF(EX(call)->func, arg_num)) {
							// ������ʹ�����ô��ݣ��������棬����Ȼ���е���
							/* By-value send is not allowed -- emit a warning,
							 * but still perform the call. */
							// �����˲����������ô���
							zend_param_must_be_ref(EX(call)->func, arg_num);
							// ���� ��װ������
							must_wrap = 1;
						}
					}
				// ����
				} else {
					// ���ֵΪ�������� ���� �˺�������ͨ������������
					if (Z_ISREF_P(arg) &&
					    !(EX(call)->func->common.fn_flags & ZEND_ACC_CALL_VIA_TRAMPOLINE)) {
						// ����ҪΪ�� __call �������ø���
						/* don't separate references for __call */
						// ������
						arg = Z_REFVAL_P(arg);
					}
				}
				// ����������װ
				if (EXPECTED(!must_wrap)) {
					// ���Ʋ���
					ZVAL_COPY(param, arg);
				// ����
				} else {
					// �������ô���
					Z_TRY_ADDREF_P(arg);
					// �� zval �����µ� zend_reference ���Ӹ����� zval �и��Ƹ�������
					ZVAL_NEW_REF(param, arg);
				}
				// ���û������
				if (!name) {
					// �����������+1
					ZEND_CALL_NUM_ARGS(EX(call))++;
					// �������� +1
					arg_num++;
					// ��һ������
					param++;
				}
			} ZEND_HASH_FOREACH_END();
		}
	}
	// �ͷŲ�������ĸ��ӱ���
	FREE_OP1();
	// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, ��ȡ��������һ�����ô���
ZEND_VM_HANDLER(120, ZEND_SEND_USER, CONST|TMP|VAR|CV, NUM)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *arg, *param;

	// windows: �޲���
	SAVE_OPLINE();
	// ��ȡzvalָ��, UNUSED ����null����֧�� TMPVAR/TMPVARCV
	arg = GET_OP1_ZVAL_PTR_DEREF(BP_VAR_R);
	// ��p1 ����p2�ֽڣ�ת��zval����
	param = ZEND_CALL_VAR(EX(call), opline->result.var);
	// �����������Ҫ���ô���
	if (UNEXPECTED(ARG_MUST_BE_SENT_BY_REF(EX(call)->func, opline->op2.num))) {
		// �����˲����������ô���
		zend_param_must_be_ref(EX(call)->func, opline->op2.num);
		// �������ô���
		Z_TRY_ADDREF_P(arg);
		// �� param ���³���������
		// �� zval �����µ� zend_reference ���Ӹ����� zval �и��Ƹ�������
		ZVAL_NEW_REF(param, arg);
	// ����
	} else {
		// �� op1��ֱֵ�Ӹ�����
		ZVAL_COPY(param, arg);
	}

	// �ͷŲ�������ĸ��ӱ���
	FREE_OP1();
	// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, ����δ����Ĳ��������������Ĭ��ֵ
ZEND_VM_HOT_HANDLER(199, ZEND_CHECK_UNDEF_ARGS, UNUSED, UNUSED)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	// ����������
	zend_execute_data *call = execute_data->call;
	
	// ??
	// ���û�� ZEND_CALL_MAY_HAVE_UNDEF ���
	if (EXPECTED(!(ZEND_CALL_INFO(call) & ZEND_CALL_MAY_HAVE_UNDEF))) {
		// opline + 1����Ŀ������벢 return
		ZEND_VM_NEXT_OPCODE();
	}

	// windows: �޲���
	SAVE_OPLINE();
	// ����δ����Ĳ��������������Ĭ��ֵ��p1:ִ������
	zend_handle_undef_args(call);
	// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, ��������Ĳ���̫��
ZEND_VM_COLD_HELPER(zend_missing_arg_helper, ANY, ANY)
{
#ifdef ZEND_VM_IP_GLOBAL_REG
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// windows: �޲���
	SAVE_OPLINE();
#endif
	// �����������̫��
	zend_missing_arg_error(execute_data);
	// ��Ҫ�� return
	HANDLE_EXCEPTION();
}

// ing3, ������� �������Ƿ�ƥ��
ZEND_VM_HELPER(zend_verify_recv_arg_type_helper, ANY, ANY, zval *op_1)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// windows: �޲���
	SAVE_OPLINE();
	// ������� �������Ƿ�ƥ�䣬p1:������p2:������ţ�p3:������p4:����λ�á� �����ƥ��
	if (UNEXPECTED(!zend_verify_recv_arg_type(EX(func), opline->op1.num, op_1, CACHE_ADDR(opline->extended_value)))) {
		// ��Ҫ�� return
		HANDLE_EXCEPTION();
	}

	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, �����ȡ���Ĳ��� ����������
ZEND_VM_HOT_HANDLER(63, ZEND_RECV, NUM, UNUSED, CACHE_SLOT)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	// op1�ǲ������
	uint32_t arg_num = opline->op1.num;
	zval *param;

	// �����Ҫ������������ ʵ�ʲ�������
	if (UNEXPECTED(arg_num > EX_NUM_ARGS())) {
		// ��������Ĳ���̫��
		// ���÷��� zend_missing_arg_helper
		ZEND_VM_DISPATCH_TO_HELPER(zend_missing_arg_helper);
	}

	// ȡ�����
	// ��ִ��������ȡ�� ָ����ŵı���
	param = EX_VAR(opline->result.var);

	// op2������
	// ����������Ͳ���Ҫ�������
	if (UNEXPECTED(!(opline->op2.num & (1u << Z_TYPE_P(param))))) {
		// ������� �������Ƿ�ƥ��
		// ���÷��� zend_verify_recv_arg_type_helper
		ZEND_VM_DISPATCH_TO_HELPER(zend_verify_recv_arg_type_helper, op_1, param);
	}
	
	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, �����ȡ���Ĳ��� ����
ZEND_VM_HOT_TYPE_SPEC_HANDLER(ZEND_RECV, op->op2.num == MAY_BE_ANY, ZEND_RECV_NOTYPE, NUM, NUM, CACHE_SLOT)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	// �������
	uint32_t arg_num = opline->op1.num;

	// �����Ҫ������������ ʵ�ʲ�������
	if (UNEXPECTED(arg_num > EX_NUM_ARGS())) {
		// ��������Ĳ���̫��
		// ���÷��� zend_missing_arg_helper
		ZEND_VM_DISPATCH_TO_HELPER(zend_missing_arg_helper);
	}

	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, ���ղ�������Ĭ��ֵ
ZEND_VM_HOT_HANDLER(64, ZEND_RECV_INIT, NUM, CONST, CACHE_SLOT)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	uint32_t arg_num;
	zval *param;
	// do {
	ZEND_VM_REPEATABLE_OPCODE
	// ��������
	arg_num = opline->op1.num;
	// ���
	// ��ִ��������ȡ�� ָ����ŵı���
	param = EX_VAR(opline->result.var);
	// ����˲������Ǳ����������Ĭ��ֵ��
	if (arg_num > EX_NUM_ARGS()) {
		// op2 �� Ĭ��ֵ
		// ���� (p2).zv��p1:opline,p2:node
		zval *default_value = RT_CONSTANT(opline, opline->op2);

		// ���Ĭ��ֵ�ǳ������ʽ
		if (Z_OPT_TYPE_P(default_value) == IS_CONSTANT_AST) {
			// �ӻ�����ȡ��
			zval *cache_val = (zval*)CACHE_ADDR(Z_CACHE_SLOT_P(default_value));

			// ������ֻ���治����������ֵ
			/* we keep in cache only not refcounted values */
			// �����ֵ����Ч
			if (Z_TYPE_P(cache_val) != IS_UNDEF) {
				// ����ֵ ���Ƹ� ���
				ZVAL_COPY_VALUE(param, cache_val);
			// ����
			} else {
				// windows: �޲���
				SAVE_OPLINE();
				// ʹ��Ĭ��ֵ
				ZVAL_COPY(param, default_value);
				// ���³���
				if (UNEXPECTED(zval_update_constant_ex(param, EX(func)->op_array.scope) != SUCCESS)) {
					// ����û�����ô����Ķ���,��û������ zval����������gc�������ڡ�
					zval_ptr_dtor_nogc(param);
					// ���� Ϊ δ����
					ZVAL_UNDEF(param);
					// ��Ҫ�� return
					HANDLE_EXCEPTION();
				}
				// ������� �ɼ�������
				if (!Z_REFCOUNTED_P(param)) {
					// �����¼��������
					ZVAL_COPY_VALUE(cache_val, param);
				}
			}
			// ��ת��ָ����ǩ recv_init_check_type
			ZEND_VM_C_GOTO(recv_init_check_type);
		// ����
		} else {
			// ʹ��Ĭ��ֵ
			ZVAL_COPY(param, default_value);
		}
	// �����Ǳ������
	} else {
// ������ת��ǩ recv_init_check_type
ZEND_VM_C_LABEL(recv_init_check_type):
		// ���������ָ������
		if (UNEXPECTED((EX(func)->op_array.fn_flags & ZEND_ACC_HAS_TYPE_HINTS) != 0)) {
			// windows: �޲���
			SAVE_OPLINE();
			// ������� �������Ƿ�ƥ�䣬p1:������p2:������ţ�p3:������p4:����λ��
			// ���ʧ��
			if (UNEXPECTED(!zend_verify_recv_arg_type(EX(func), arg_num, param, CACHE_ADDR(opline->extended_value)))) {
				// ��Ҫ�� return
				HANDLE_EXCEPTION();
			}
		}
	}

	// ѭ����ֱ������ ZEND_RECV_INIT
	// while() ѭ����β�Σ�����һ�������룬ֱ������p1������
	ZEND_VM_REPEAT_OPCODE(ZEND_RECV_INIT);
	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing2, �����ֵ����
ZEND_VM_HANDLER(164, ZEND_RECV_VARIADIC, NUM, UNUSED, CACHE_SLOT)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	// �������
	uint32_t arg_num = opline->op1.num;
	// ��Ҫ������
	uint32_t arg_count = EX_NUM_ARGS();
	//
	zval *params;

	// windows: �޲���
	SAVE_OPLINE();

	// ��ִ��������ȡ�� ָ����ŵı���
	params = EX_VAR(opline->result.var);

	// ��������Чλ��
	if (arg_num <= arg_count) {
		// ����Ҫ�ܽ��� �ֵ����
		ZEND_ASSERT(EX(func)->common.fn_flags & ZEND_ACC_VARIADIC);
		// �������� ������ �����-1
		ZEND_ASSERT(EX(func)->common.num_args == arg_num - 1);
		// ȡ�ò�����Ϣ
		zend_arg_info *arg_info = &EX(func)->common.arg_info[arg_num - 1];

		// ��ʼ��������
		array_init_size(params, arg_count - arg_num + 1);
		// ��ʼ����˳������
		zend_hash_real_init_packed(Z_ARRVAL_P(params));
		// ��������в���Ԫ��
		ZEND_HASH_FILL_PACKED(Z_ARRVAL_P(params)) {
			// ȡ��һ������
			zval *param = EX_VAR_NUM(EX(func)->op_array.last_var + EX(func)->op_array.T);
			// ������ʹ���
			if (UNEXPECTED(ZEND_TYPE_IS_SET(arg_info->type))) {
				// Z_TYPE_INFO((p1)->This) |= p2
				ZEND_ADD_CALL_FLAG(execute_data, ZEND_CALL_FREE_EXTRA_ARGS);
				// �������з��ֵ����
				do {
					// �����ֵ�������ͣ�p1:������p2:������Ϣ��p3:������ţ�p4:������p5:����λ��
					// ������ɹ�
					if (UNEXPECTED(!zend_verify_variadic_arg_type(EX(func), arg_info, arg_num, param, CACHE_ADDR(opline->extended_value)))) {
						// ������
						ZEND_HASH_FILL_FINISH();
						// ��Ҫ�� return
						HANDLE_EXCEPTION();
					}
					// ����ǿ��Լ�������
					// ���ô���+1
					if (Z_OPT_REFCOUNTED_P(param)) Z_ADDREF_P(param);
					// �Ѳ�����ӽ�������
					ZEND_HASH_FILL_ADD(param);
					// ��һ������
					param++;
					// 
				} while (++arg_num <= arg_count);
			// �������Ͳ�����
			} else {
				// �������з��ֵ����
				do {
					// ����ǿ��Լ�������
					// ���ô���+1
					if (Z_OPT_REFCOUNTED_P(param)) Z_ADDREF_P(param);
					// �Ѵ� zval ��ӽ�������
					ZEND_HASH_FILL_ADD(param);
					// ��һ��
					param++;
				} while (++arg_num <= arg_count);
			}
		} ZEND_HASH_FILL_END();
	// ���򣬲���������Чλ��
	} else {
		// �������ǿ�����
		ZVAL_EMPTY_ARRAY(params);
	}

	// ����ж������������
	if (EX_CALL_INFO() & ZEND_CALL_HAS_EXTRA_NAMED_PARAMS) {
		zend_string *name;
		zval *param;
		// ������Ϣ
		zend_arg_info *arg_info = &EX(func)->common.arg_info[EX(func)->common.num_args];
		// ������ʹ���
		if (ZEND_TYPE_IS_SET(arg_info->type)) {
			// ����������������ʹ�ø���
			SEPARATE_ARRAY(params);
			// �������ж�����������
			ZEND_HASH_MAP_FOREACH_STR_KEY_VAL(EX(extra_named_params), name, param) {
				// �����ֵ�������ͣ�p1:������p2:������Ϣ��p3:������ţ�p4:������p5:����λ��
				// ������ɹ�
				if (UNEXPECTED(!zend_verify_variadic_arg_type(EX(func), arg_info, arg_num, param, CACHE_ADDR(opline->extended_value)))) {
					// ��Ҫ�� return
					HANDLE_EXCEPTION();
				}
				// �������ô���
				Z_TRY_ADDREF_P(param);
				// ����²���
				zend_hash_add_new(Z_ARRVAL_P(params), name, param);
			} ZEND_HASH_FOREACH_END();
		// �����������û��Ԫ��
		} else if (zend_hash_num_elements(Z_ARRVAL_P(params)) == 0) {
			// ԭ����������������ô���
			GC_ADDREF(EX(extra_named_params));
			// ֱ��ʹ��ԭ���������
			ZVAL_ARR(params, EX(extra_named_params));
		// ����û�����ͣ����Ҳ��������в���
		} else {
			// ����������������ʹ�ø���
			SEPARATE_ARRAY(params);
			// ����������������
			ZEND_HASH_MAP_FOREACH_STR_KEY_VAL(EX(extra_named_params), name, param) {
				// �������ô���
				Z_TRY_ADDREF_P(param);
				// ������������ӽ�������
				zend_hash_add_new(Z_ARRVAL_P(params), name, param);
			} ZEND_HASH_FOREACH_END();
		}
	}

	// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, �ѱ���ת�� ���� ��
ZEND_VM_COLD_CONST_HANDLER(52, ZEND_BOOL, CONST|TMPVAR|CV, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *val;

	// ��ȡop1��zvalָ��, UNUSED ����null
	val = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ���ֵΪtrue
	if (Z_TYPE_INFO_P(val) == IS_TRUE) {
		// ���Ϊ true
		ZVAL_TRUE(EX_VAR(opline->result.var));
	// ���� ���ֵΪ undef/null/false
	} else if (EXPECTED(Z_TYPE_INFO_P(val) <= IS_TRUE)) {
		// result��op1����������� ������ͬһ���������
		/* The result and op1 can be the same cv zval */
		// ԭ op1 �ı�������
		const uint32_t orig_val_type = Z_TYPE_INFO_P(val);
		// ���Ϊ false
		ZVAL_FALSE(EX_VAR(opline->result.var));
		// ��� op1 �� ������� ���� ԭֵ����Ϊ δ����
		if (OP1_TYPE == IS_CV && UNEXPECTED(orig_val_type == IS_UNDEF)) {
			// windows: �޲���
			SAVE_OPLINE();
			// ����p1�ı�������������δ����, ����δ��ʼ��zval
			ZVAL_UNDEFINED_OP1();
			// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
			ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
		}
	// ��������ֵ��Ϊtrue
	} else {
		// windows: �޲���
		SAVE_OPLINE();
		// ��ֵת�� bool �ͷŵ������
		ZVAL_BOOL(EX_VAR(opline->result.var), i_zend_is_true(val));
		// �ͷŲ�������ĸ��ӱ���
		FREE_OP1();
		// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
		ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
	}
	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, ��case��������ȵıȽ�����, ���鴫��ı����Ƿ���Ч������ �Ƚ� ���㣬Ȼ�������������op2
ZEND_VM_HELPER(zend_case_helper, ANY, ANY, zval *op_1, zval *op_2)
{
	int ret;
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// windows: �޲���
	SAVE_OPLINE();
	// ��� ��������Ϊδ����
	if (UNEXPECTED(Z_TYPE_INFO_P(op_1) == IS_UNDEF)) {
		// ����p1�ı�������������δ����, ����δ��ʼ��zval
		op_1 = ZVAL_UNDEFINED_OP1();
	}
	// ��� ��������Ϊ δ����
	if (UNEXPECTED(Z_TYPE_INFO_P(op_2) == IS_UNDEF)) {
		// ����p2�ı�������������δ����, ����δ��ʼ��zval
		op_2 = ZVAL_UNDEFINED_OP2();
	}
	// ���бȽ�����
	ret = zend_compare(op_1, op_2);
	// ��� op2 ���� �Ǳ�������ʱ����
	if (OP2_TYPE & (IS_TMP_VAR|IS_VAR)) {
		// ����û�����ô����Ķ���,��û������ zval����������gc�������ڡ�
		zval_ptr_dtor_nogc(op_2);
	}
	// ���ݵ�ǰ��������ѡ����һ�������룬p1:����ֵ����Ҫ�������жϡ�p2:�Ƿ����쳣
	ZEND_VM_SMART_BRANCH(ret == 0, 1);
}

// ing3, case��䣬������ȵıȽ�����
ZEND_VM_HANDLER(48, ZEND_CASE, TMPVAR, CONST|TMPVAR|CV)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2;
	double d1, d2;

	// ��ȡzvalָ��, UNUSED ����null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ��ȡzvalָ��, UNUSED ����null
	op2 = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ���op1������
	if (EXPECTED(Z_TYPE_P(op1) == IS_LONG)) {
		// ���op2������
		if (EXPECTED(Z_TYPE_P(op2) == IS_LONG)) {
			// ���op1,op2���
			if (EXPECTED(Z_LVAL_P(op1) == Z_LVAL_P(op2))) {
// ������ת��ǩ case_true
ZEND_VM_C_LABEL(case_true):
				// ���ݵ�ǰ��������ѡ����һ�������롣���������ת���ͣ������ֵΪtrue��
				ZEND_VM_SMART_BRANCH_TRUE();
			// ����, ��ƥ��
			} else {
// ������ת��ǩ case_false
ZEND_VM_C_LABEL(case_false):
				// ���ݵ�ǰ��������ѡ����һ�������롣���������ת���ͣ������ֵΪfalse��
				ZEND_VM_SMART_BRANCH_FALSE();
			}
		// ���op2��С��
		} else if (EXPECTED(Z_TYPE_P(op2) == IS_DOUBLE)) {
			// ����תС��
			d1 = (double)Z_LVAL_P(op1);
			// ȡ��С��
			d2 = Z_DVAL_P(op2);
			// ��ת��ָ����ǩ case_double
			ZEND_VM_C_GOTO(case_double);
		}
	// ���� ��� op1 ��С��
	} else if (EXPECTED(Z_TYPE_P(op1) == IS_DOUBLE)) {
		// ������� op2 ��С��
		if (EXPECTED(Z_TYPE_P(op2) == IS_DOUBLE)) {
			// ȡ��С��
			d1 = Z_DVAL_P(op1);
			// ȡ��С��
			d2 = Z_DVAL_P(op2);
// ������ת��ǩ case_double
ZEND_VM_C_LABEL(case_double):
			// ֱ�ӱȽ����
			if (d1 == d2) {
				// ��ת��ָ����ǩ case_true
				ZEND_VM_C_GOTO(case_true);
			// ����
			} else {
				// ��ת��ָ����ǩ case_false
				ZEND_VM_C_GOTO(case_false);
			}
		// ���op2������
		} else if (EXPECTED(Z_TYPE_P(op2) == IS_LONG)) {
			// ȡ��С��
			d1 = Z_DVAL_P(op1);
			// ����ת��С��
			d2 = (double)Z_LVAL_P(op2);
			// ��ת��ָ����ǩ case_double
			ZEND_VM_C_GOTO(case_double);
		}
	// ���op1���ִ�
	} else if (EXPECTED(Z_TYPE_P(op1) == IS_STRING)) {
		// ���op2���ִ�
		if (EXPECTED(Z_TYPE_P(op2) == IS_STRING)) {
			// �Ƚ������ִ����
			bool result = zend_fast_equal_strings(Z_STR_P(op1), Z_STR_P(op2));
			// �ͷŲ�������ĸ��ӱ���
			FREE_OP2();
			// ��������
			if (result) {
				// ��ת��ָ����ǩ case_true
				ZEND_VM_C_GOTO(case_true);
			// ����
			} else {
				// ��ת��ָ����ǩ case_false
				ZEND_VM_C_GOTO(case_false);
			}
		}
	}
	// ���÷��� zend_case_helper
	ZEND_VM_DISPATCH_TO_HELPER(zend_case_helper, op_1, op1, op_2, op2);
}

// ing3, new���,��������
ZEND_VM_HANDLER(68, ZEND_NEW, UNUSED|CLASS_FETCH|CONST|VAR, UNUSED|CACHE_SLOT, NUM)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *result;
	zend_function *constructor;
	zend_class_entry *ce;
	zend_execute_data *call;

	// windows: �޲���
	SAVE_OPLINE();
	// ���op1�ǳ���
	if (OP1_TYPE == IS_CONST) {
		// ͨ��ƫ���� ������ʱ�����л�ȡһ�� (void**) �еĵ�һ��Ԫ��
		ce = CACHED_PTR(opline->op2.num);
		// ��� �಻����
		if (UNEXPECTED(ce == NULL)) {
			// ��ȡ�࣬�Ҳ����ᱨ��p1:������p2:key��p3:flags
			// ���� (p2).zv��p1:opline,p2:node
			ce = zend_fetch_class_by_name(Z_STR_P(RT_CONSTANT(opline, opline->op1)), Z_STR_P(RT_CONSTANT(opline, opline->op1) + 1), ZEND_FETCH_CLASS_DEFAULT | ZEND_FETCH_CLASS_EXCEPTION);
			// ��� �಻����
			if (UNEXPECTED(ce == NULL)) {
				// ���Ϊ δ����
				ZVAL_UNDEF(EX_VAR(opline->result.var));
				// ��Ҫ�� return
				HANDLE_EXCEPTION();
			}
			// ͨ��ƫ���� �� EX(run_time_cache) �л�ȡһ�� (void**) �еĵ�һ��Ԫ�أ���������ֵ
			CACHE_PTR(opline->op2.num, ce);
		}
	// ���� ��� op1 ������δ����
	} else if (OP1_TYPE == IS_UNUSED) {
		// ͨ�����������Ͳ����࣬p1:������p2:����
		ce = zend_fetch_class(NULL, opline->op1.num);
		// ��� �಻����
		if (UNEXPECTED(ce == NULL)) {
			// ���Ϊ δ����
			ZVAL_UNDEF(EX_VAR(opline->result.var));
			// ��Ҫ�� return
			HANDLE_EXCEPTION();
		}
	// ����
	} else {
		// ȡ�� �����
		ce = Z_CE_P(EX_VAR(opline->op1.var));
	}

	// ��ִ��������ȡ�� ָ����ŵı���
	result = EX_VAR(opline->result.var);
	// ��ʼ�����󣬴���zvalָ��������
	if (UNEXPECTED(object_init_ex(result, ce) != SUCCESS)) {
		// ���Ϊ δ����
		ZVAL_UNDEF(result);
		// ��Ҫ�� return
		HANDLE_EXCEPTION();
	}

	// ��ȡ���췽��
	constructor = Z_OBJ_HT_P(result)->get_constructor(Z_OBJ_P(result));
	// ���û�й��췽��
	if (constructor == NULL) {
		if (UNEXPECTED(EG(exception))) {
			// ��Ҫ�� return
			HANDLE_EXCEPTION();
		}

		// ���û�в��������� DO_FCALL �����롣
		/* If there are no arguments, skip over the DO_FCALL opcode. We check if the next
		 * opcode is DO_FCALL in case EXT instructions are used. */
		if (EXPECTED(opline->extended_value == 0 && (opline+1)->opcode == ZEND_DO_FCALL)) {
			// ƫ�Ƶ�Ŀ������벢 return ,p1:�Ƿ����쳣��ʹ�� EX(opline)  ��ʹ�� opline ��p2:ƫ����
			ZEND_VM_NEXT_OPCODE_EX(1, 2);
		}

		// ����һ���ٵķ������ã� zend_pass_function �ǿպ���
		/* Perform a dummy function call */
		// ing3, ��ʼ�������� ��ջ����һ��ִ�����ݣ��ռ䲻������չ��С
		// p1:������Ϣ��p2:������p3:����������p4:����������
		call = zend_vm_stack_push_call_frame(
			ZEND_CALL_FUNCTION, (zend_function *) &zend_pass_function,
			opline->extended_value, NULL);
	// ���򣬻�ȡ���˹��췽��
	} else {
		// ������û����庯�� ���� û�� ����ʱ����
		if (EXPECTED(constructor->type == ZEND_USER_FUNCTION) && UNEXPECTED(!RUN_TIME_CACHE(&constructor->op_array))) {
			// Ϊop_array������ ��ʼ������ʱ����
			init_func_run_time_cache(&constructor->op_array);
		}
		/* We are not handling overloaded classes right now */
		// ing3, ��ʼ�������� ��ջ����һ��ִ�����ݣ��ռ䲻������չ��С
		// p1:������Ϣ��p2:������p3:����������p4:����������
		// ���ñ�ǣ����ú�������Ҫ�ͷ� $this, ��$this
		call = zend_vm_stack_push_call_frame(
			ZEND_CALL_FUNCTION | ZEND_CALL_RELEASE_THIS | ZEND_CALL_HAS_THIS,
			constructor,
			opline->extended_value,
			Z_OBJ_P(result));
		// ���ô���+1
		Z_ADDREF_P(result);
	}

	// ����ǰһ��ִ������
	call->prev_execute_data = EX(call);
	// ��ִ������ ��Ϊ��������
	EX(call) = call;
	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, ��֤�����ö���� clone ����
ZEND_VM_COLD_CONST_HANDLER(110, ZEND_CLONE, CONST|TMPVAR|UNUSED|THIS|CV, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *obj;
	zend_object *zobj;
	zend_class_entry *ce, *scope;
	zend_function *clone;
	// ����ԭ��
	zend_object_clone_obj_t clone_call;

	// windows: �޲���
	SAVE_OPLINE();
	// ��ȡzvalָ��, UNUSED ���� $this
	obj = GET_OP1_OBJ_ZVAL_PTR_UNDEF(BP_VAR_R);

	// Ϊ��break
	do {
		// ���op1�ǳ��� �� �� op1����δʹ������ ���� op1���Ƕ��� ��
		if (OP1_TYPE == IS_CONST ||
		    (OP1_TYPE != IS_UNUSED && UNEXPECTED(Z_TYPE_P(obj) != IS_OBJECT))) {
			// ��� op1 ����ͨ������������ ���� ����������
			if ((OP1_TYPE & (IS_VAR|IS_CV)) && Z_ISREF_P(obj)) {
				// ������
				obj = Z_REFVAL_P(obj);
				// ����Ƕ���
				if (EXPECTED(Z_TYPE_P(obj) == IS_OBJECT)) {
					// ����
					break;
				}
			}
			// ���Ϊ δ����
			ZVAL_UNDEF(EX_VAR(opline->result.var));
			// ��� op1 �� ������� ���Ҷ��� ֵΪ δ����
			if (OP1_TYPE == IS_CV && UNEXPECTED(Z_TYPE_P(obj) == IS_UNDEF)) {
				// ����p1�ı�������������δ����, ����δ��ʼ��zval
				ZVAL_UNDEFINED_OP1();
				// ������쳣
				if (UNEXPECTED(EG(exception) != NULL)) {
					// ��Ҫ�� return
					HANDLE_EXCEPTION();
				}
			}
			// �����ڷǶ�������� ���� __clone����
			zend_throw_error(NULL, "__clone method called on non-object");
			// �ͷŲ�������ĸ��ӱ���
			FREE_OP1();
			// ��Ҫ�� return
			HANDLE_EXCEPTION();
		}
	} while (0);
	// ȷ�����Ƕ��󣬿���clone

	// ȡ������
	zobj = Z_OBJ_P(obj);
	// ������
	ce = zobj->ce;
	// clone ����
	clone = ce->clone;
	// clone_obj ����
	clone_call = zobj->handlers->clone_obj;
	// ���û�� clone_obj
	if (UNEXPECTED(clone_call == NULL)) {
		// ����A���޷���¡�Ķ���
		zend_throw_error(NULL, "Trying to clone an uncloneable object of class %s", ZSTR_VAL(ce->name));
		// �ͷŲ�������ĸ��ӱ���
		FREE_OP1();
		// ���Ϊ δ����
		ZVAL_UNDEF(EX_VAR(opline->result.var));
		// ��Ҫ�� return
		HANDLE_EXCEPTION();
	}

	// �����clone���� ���� ����public
	if (clone && !(clone->common.fn_flags & ZEND_ACC_PUBLIC)) {
		// �ҵ���ǰ ���ࣩ
		scope = EX(func)->op_array.scope;
		// ������������ڵ�ǰ ��
		if (clone->common.scope != scope) {
			// ���clone������˽�еģ����� �Ǽ���̳й�ϵʧ��
			if (UNEXPECTED(clone->common.fn_flags & ZEND_ACC_PRIVATE)
				
			 || UNEXPECTED(!zend_check_protected(zend_get_function_root_class(clone), scope))) {
				// �������� __clone��������
				zend_wrong_clone_call(clone, scope);
				// �ͷŲ�������ĸ��ӱ���
				FREE_OP1();
				// ���Ϊ δ����
				ZVAL_UNDEF(EX_VAR(opline->result.var));
				// ��Ҫ�� return
				HANDLE_EXCEPTION();
			}
		}
	}

	// ���� clone_call�����ؽ��
	ZVAL_OBJ(EX_VAR(opline->result.var), clone_call(zobj));

	// �ͷŲ�������ĸ��ӱ���
	FREE_OP1();
	// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, ��ȡ����
// , ��ҵ���߼��� 1/1
// ZEND_FASTCALL ZEND_EXT_NOP_SPEC��1����֧����֤����
ZEND_VM_HOT_HANDLER(99, ZEND_FETCH_CONSTANT, UNUSED|CONST_FETCH, CONST, CACHE_SLOT)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zend_constant *c;
	// �ȳ����ڻ����л�ȡ
	// ͨ��ƫ���� ������ʱ�����л�ȡһ�� (void**) �еĵ�һ��Ԫ��
	c = CACHED_PTR(opline->extended_value);
	// ���������Ч ���� ���� CACHE_SPECIAL
	// ((uintptr_t)(ptr)) & CACHE_SPECIAL�� CACHE_SPECIAL=1
	if (EXPECTED(c != NULL) && EXPECTED(!IS_SPECIAL_CACHE_VAL(c))) {
		// ����չ��Ϣ��ֵ���Ƹ� ���
		ZVAL_COPY_OR_DUP(EX_VAR(opline->result.var), &c->value);
		// opline + 1����Ŀ������벢 return
		ZEND_VM_NEXT_OPCODE();
	}
	// ����ӻ����л�ȡʧ��
	
	// windows: �޲���
	SAVE_OPLINE();
	// ���ٶ�ȡ������p1:��������p2:flags
	// ���� (p2).zv��p1:opline,p2:node
	zend_quick_get_constant(RT_CONSTANT(opline, opline->op2) + 1, opline->op1.num OPLINE_CC EXECUTE_DATA_CC);
	// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}


// ing3, ��ȡ�ೣ��
ZEND_VM_HANDLER(181, ZEND_FETCH_CLASS_CONSTANT, VAR|CONST|UNUSED|CLASS_FETCH, CONST, CACHE_SLOT)
{
	zend_class_entry *ce, *scope;
	zend_class_constant *c;
	zval *value, *zv;
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// windows: �޲���
	SAVE_OPLINE();
	// Ϊ��break
	do {
		// ���op1�ǳ���
		if (OP1_TYPE == IS_CONST) {
			// ͨ��ƫ���� ������ʱ�����л�ȡһ�� (void**) �еĵ�һ��Ԫ�أ��������
			if (EXPECTED(CACHED_PTR(opline->extended_value + sizeof(void*)))) {
				// ʹ�����ֵ
				value = CACHED_PTR(opline->extended_value + sizeof(void*));
				// ����
				break;
			// ͨ��ƫ���� ������ʱ�����л�ȡһ�� (void**) �еĵ�һ��Ԫ�أ��������
			} else if (EXPECTED(CACHED_PTR(opline->extended_value))) {
				// ʹ�����ֵ
				ce = CACHED_PTR(opline->extended_value);
			// ���û���ڻ����ж�ȡ��
			} else {
				// ��ȡ�࣬�Ҳ����ᱨ��p1:������p2:key��p3:flags
				// ���� (p2).zv��p1:opline,p2:node
				ce = zend_fetch_class_by_name(Z_STR_P(RT_CONSTANT(opline, opline->op1)), Z_STR_P(RT_CONSTANT(opline, opline->op1) + 1), ZEND_FETCH_CLASS_DEFAULT | ZEND_FETCH_CLASS_EXCEPTION);
				// ��� �಻����
				if (UNEXPECTED(ce == NULL)) {
					// ���Ϊ δ����
					ZVAL_UNDEF(EX_VAR(opline->result.var));
					// ��Ҫ�� return
					HANDLE_EXCEPTION();
				}
			}
		// ����op1���ǳ���
		} else {
			// ��� op1 ������δ����
			if (OP1_TYPE == IS_UNUSED) {
				// ͨ�����������Ͳ����࣬p1:������p2:����
				ce = zend_fetch_class(NULL, opline->op1.num);
				// ��� �಻����
				if (UNEXPECTED(ce == NULL)) {
					// ���Ϊ δ����
					ZVAL_UNDEF(EX_VAR(opline->result.var));
					// ��Ҫ�� return
					HANDLE_EXCEPTION();
				}
			// ����
			} else {
				// ȡ�� �����
				ce = Z_CE_P(EX_VAR(opline->op1.var));
			}
			// ͨ��ƫ���� ������ʱ�����л�ȡһ�� (void**) �еĵ�һ��Ԫ��
			if (EXPECTED(CACHED_PTR(opline->extended_value) == ce)) {
				// ͨ��ƫ���� ������ʱ�����л�ȡһ�� (void**) �еĵ�һ��Ԫ��
				value = CACHED_PTR(opline->extended_value + sizeof(void*));
				// ����
				break;
			}
		}

		// �ӳ������в���
		// ���� (p2).zv��p1:opline,p2:node
		zv = zend_hash_find_known_hash(CE_CONSTANTS_TABLE(ce), Z_STR_P(RT_CONSTANT(opline, opline->op2)));
		// ����ҵ���
		if (EXPECTED(zv != NULL)) {
			// ��ָ��Ŀ��ȡ��
			c = Z_PTR_P(zv);
			// ��
			scope = EX(func)->op_array.scope;
			// ����������ɷ���
			if (!zend_verify_const_access(c, scope)) {
				// ���� (p2).zv��p1:opline,p2:node
				zend_throw_error(NULL, "Cannot access %s constant %s::%s", zend_visibility_string(ZEND_CLASS_CONST_FLAGS(c)), ZSTR_VAL(ce->name), Z_STRVAL_P(RT_CONSTANT(opline, opline->op2)));
				// ���Ϊ δ����
				ZVAL_UNDEF(EX_VAR(opline->result.var));
				// ��Ҫ�� return
				HANDLE_EXCEPTION();
			}

			// ������trait���ʳ���
			if (ce->ce_flags & ZEND_ACC_TRAIT) {
				// ���� (p2).zv��p1:opline,p2:node
				zend_throw_error(NULL, "Cannot access trait constant %s::%s directly", ZSTR_VAL(ce->name), Z_STRVAL_P(RT_CONSTANT(opline, opline->op2)));
				// ���Ϊ δ����
				ZVAL_UNDEF(EX_VAR(opline->result.var));
				// ��Ҫ�� return
				HANDLE_EXCEPTION();
			}

			// ����ֵ
			value = &c->value;
			// ö��������Ҫ���������ೣ�� ����������ö�ٱ�
			// Enums require loading of all class constants to build the backed enum table
			// �����ö�� ���� �й涨���� ���� ���û������ö���� ���� û�и��¹�����
			if (ce->ce_flags & ZEND_ACC_ENUM && ce->enum_backing_type != IS_UNDEF && ce->type == ZEND_USER_CLASS && !(ce->ce_flags & ZEND_ACC_CONSTANTS_UPDATED)) {
				// ������³���ʧ��
				if (UNEXPECTED(zend_update_class_constants(ce) == FAILURE)) {
					// ���Ϊ δ����
					ZVAL_UNDEF(EX_VAR(opline->result.var));
					// ��Ҫ�� return
					HANDLE_EXCEPTION();
				}
			}
			// ���ֵ�� �������ʽ
			if (Z_TYPE_P(value) == IS_CONSTANT_AST) {
				// ���´˳���
				zval_update_constant_ex(value, c->ce);
				// ���û���쳣
				if (UNEXPECTED(EG(exception) != NULL)) {
					// ���Ϊ δ����
					ZVAL_UNDEF(EX_VAR(opline->result.var));
					// ��Ҫ�� return
					HANDLE_EXCEPTION();
				}
			}
			// �ҵ�����ʱ����� p1 ��λ�ã���p2 ��p3���α����ȥ
			CACHE_POLYMORPHIC_PTR(opline->extended_value, ce, value);
		// ����
		} else {
			// ��������δ����
			// ���� (p2).zv��p1:opline,p2:node
			zend_throw_error(NULL, "Undefined constant %s::%s",
				ZSTR_VAL(ce->name), Z_STRVAL_P(RT_CONSTANT(opline, opline->op2)));
			// ���Ϊ δ����
			ZVAL_UNDEF(EX_VAR(opline->result.var));
			// ��Ҫ�� return
			HANDLE_EXCEPTION();
		}
	} while (0);

	// ֵ���Ƹ���� 
	ZVAL_COPY_OR_DUP(EX_VAR(opline->result.var), value);

	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, �����������Ԫ��
ZEND_VM_HANDLER(72, ZEND_ADD_ARRAY_ELEMENT, CONST|TMP|VAR|CV, CONST|TMPVAR|UNUSED|NEXT|CV, REF)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *expr_ptr, new_expr;

	// windows: �޲���
	SAVE_OPLINE();
	// ��� op1 �� ����ͨ���� �� ������� �� ���� ���������Ԫ�ر��
	if ((OP1_TYPE == IS_VAR || OP1_TYPE == IS_CV) &&
	    UNEXPECTED(opline->extended_value & ZEND_ARRAY_ELEMENT_REF)) {
		// ��ȡzvalָ��,TMP/CONST/UNUSED ����null����֧�� TMPVAR/TMPVARCV
		expr_ptr = GET_OP1_ZVAL_PTR_PTR(BP_VAR_W);
		// ������ʽ����������
		if (Z_ISREF_P(expr_ptr)) {
			// ���ô���+1
			Z_ADDREF_P(expr_ptr);
		// ����
		} else {
			// ת���������ͣ����ô���Ϊ2
			ZVAL_MAKE_REF_EX(expr_ptr, 2);
		}
		// �ͷŲ�������ĸ��ӱ���
		FREE_OP1();
	// ����
	} else {
		// ��ȡzvalָ�룬UNUSED ���ͷ���null
		expr_ptr = GET_OP1_ZVAL_PTR(BP_VAR_R);
		// ��� op1����ʱ����
		if (OP1_TYPE == IS_TMP_VAR) {
			/* pass */
		// ���� ���op1�ǳ���
		} else if (OP1_TYPE == IS_CONST) {
			// �������ô���
			Z_TRY_ADDREF_P(expr_ptr);
		// ���� ��� op1 �� �������
		} else if (OP1_TYPE == IS_CV) {
			// ���Խ����� expr_ptr
			ZVAL_DEREF(expr_ptr);
			// �������ô���
			Z_TRY_ADDREF_P(expr_ptr);
		// ���� ��� op1 �� ��ͨ����
		} else /* if (OP1_TYPE == IS_VAR) */ {
			// ��� Ԫ��ʽ�� ��������
			if (UNEXPECTED(Z_ISREF_P(expr_ptr))) {
				// ȡ�� ����ʵ��
				zend_refcounted *ref = Z_COUNTED_P(expr_ptr);
				// ������
				expr_ptr = Z_REFVAL_P(expr_ptr);
				// ���ô���-1�����Ϊ0
				if (UNEXPECTED(GC_DELREF(ref) == 0)) {
					// ֵ���Ƹ��� new_expr
					ZVAL_COPY_VALUE(&new_expr, expr_ptr);
					// ʹ����ֵ
					expr_ptr = &new_expr;
					// �ͷ�����ʵ��
					efree_size(ref, sizeof(zend_reference));
				// ���� ����ǿ��Լ�������
				} else if (Z_OPT_REFCOUNTED_P(expr_ptr)) {
					// ���ô���+1
					Z_ADDREF_P(expr_ptr);
				}
			}
		}
	}

	// ���op2 ��ʹ��, ˵����key���ϣֵ
	if (OP2_TYPE != IS_UNUSED) {
		// ��ȡzvalָ��, UNUSED ����null
		zval *offset = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
		zend_string *str;
		zend_ulong hval;

// ������ת��ǩ add_again
ZEND_VM_C_LABEL(add_again):
		// ���op2���ִ�
		if (EXPECTED(Z_TYPE_P(offset) == IS_STRING)) {
			// ȡ���ִ�
			str = Z_STR_P(offset);
			// ��� OP2 ���ǳ���
			if (OP2_TYPE != IS_CONST) {
				// ����ִ�ȫ������
				if (ZEND_HANDLE_NUMERIC(str, hval)) {
					// ��ת��ָ����ǩ num_index����������������
					ZEND_VM_C_GOTO(num_index);
				}
			}
// ������ת��ǩ str_index�����ִ���������
ZEND_VM_C_LABEL(str_index):
			// �� key ��������Ԫ�أ���ϣֵΪ����������ֵΪ ���ʽ
			zend_hash_update(Z_ARRVAL_P(EX_VAR(opline->result.var)), str, expr_ptr);
		// ���� ���op2������
		} else if (EXPECTED(Z_TYPE_P(offset) == IS_LONG)) {
			// ȡ��ƫ��������
			hval = Z_LVAL_P(offset);
// ������ת��ǩ num_index����������������
ZEND_VM_C_LABEL(num_index):
			// �ù�ϣֵ��������Ԫ�أ���ϣֵΪ����������ֵΪ ���ʽ
			zend_hash_index_update(Z_ARRVAL_P(EX_VAR(opline->result.var)), hval, expr_ptr);
		// ���� ��� op2 ����ͨ������������ ���� op2 ����������
		} else if ((OP2_TYPE & (IS_VAR|IS_CV)) && EXPECTED(Z_TYPE_P(offset) == IS_REFERENCE)) {
			// ������
			offset = Z_REFVAL_P(offset);
			// ��ת��ָ����ǩ add_again
			ZEND_VM_C_GOTO(add_again);
		// ���� ��� op2��null
		} else if (Z_TYPE_P(offset) == IS_NULL) {
			// ���ִ�
			str = ZSTR_EMPTY_ALLOC();
			// ��ת��ָ����ǩ str_index�����ִ���������
			ZEND_VM_C_GOTO(str_index);
		// ���� ��� op2��double
		} else if (Z_TYPE_P(offset) == IS_DOUBLE) {
			// ȡ��С��
			hval = zend_dval_to_lval_safe(Z_DVAL_P(offset));
			// ��ת��ָ����ǩ num_index����������������
			ZEND_VM_C_GOTO(num_index);
		// ���� ��� op2��false
		} else if (Z_TYPE_P(offset) == IS_FALSE) {
			// ֵΪ0
			hval = 0;
			// ��ת��ָ����ǩ num_index����������������
			ZEND_VM_C_GOTO(num_index);
		// ���� ��� op2��true
		} else if (Z_TYPE_P(offset) == IS_TRUE) {
			// ֵΪ1
			hval = 1;
			// ��ת��ָ����ǩ num_index����������������
			ZEND_VM_C_GOTO(num_index);
		// ���� ��� op2����Դ����
		} else if (Z_TYPE_P(offset) == IS_RESOURCE) {
			// �׳����棺��Դ��Ϊƫ������ת�������֣��Լ���ID��
			zend_use_resource_as_offset(offset);
			// ȡ����Դ���
			hval = Z_RES_HANDLE_P(offset);
			// ��ת��ָ����ǩ num_index����������������
			ZEND_VM_C_GOTO(num_index);
		// ���� ��� op2 �� ������� ���� ƫ���� Ϊδ����
		} else if (OP2_TYPE == IS_CV && Z_TYPE_P(offset) == IS_UNDEF) {
			// ����p2�ı�������������δ����, ����δ��ʼ��zval
			ZVAL_UNDEFINED_OP2();
			// ���ִ�
			str = ZSTR_EMPTY_ALLOC();
			// ��ת��ָ����ǩ str_index�����ִ���������
			ZEND_VM_C_GOTO(str_index);
		// ���򣬱���
		} else {
			// ����ƫ�����Ƿ�
			zend_illegal_offset();
			// ����û�����ô����Ķ���,��û������ zval����������gc�������ڡ�
			zval_ptr_dtor_nogc(expr_ptr);
		}
		// �ͷŲ�������ĸ��ӱ���
		FREE_OP2();
	// ����û��key�͹�ϣֵ
	} else {
		// ����ֵ��ӵ�����У����ʧ��
		if (!zend_hash_next_index_insert(Z_ARRVAL_P(EX_VAR(opline->result.var)), expr_ptr)) {
			// �������Ԫ��ʧ�ܣ���һ��Ԫ���Ѿ�����
			zend_cannot_add_element();
			// ����û�����ô����Ķ���,��û������ zval����������gc�������ڡ�
			zval_ptr_dtor_nogc(expr_ptr);
		}
	}
	// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, �����������Ҫ�����Ԫ��
ZEND_VM_HANDLER(147, ZEND_ADD_ARRAY_UNPACK, ANY, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1;
	HashTable *result_ht;

	// windows: �޲���
	SAVE_OPLINE();
	// ��ȡ op1 �� zvalָ�룬UNUSED ���ͷ���null
	op1 = GET_OP1_ZVAL_PTR(BP_VAR_R);
	// ȡ����� �еĹ�ϣ��
	result_ht = Z_ARRVAL_P(EX_VAR(opline->result.var));

// ������ת��ǩ add_unpack_again
ZEND_VM_C_LABEL(add_unpack_again):
	// ���op1���������� 
	if (EXPECTED(Z_TYPE_P(op1) == IS_ARRAY)) {
		// ȡ����ϣ��
		HashTable *ht = Z_ARRVAL_P(op1);
		zval *val;
		zend_string *key;
		// ������ϣ��
		ZEND_HASH_FOREACH_STR_KEY_VAL(ht, key, val) {
			// ������������� ���� ���ô�����1
			if (Z_ISREF_P(val) && Z_REFCOUNT_P(val) == 1) {
				// ������
				val = Z_REFVAL_P(val);
			}
			// �������ô���
			Z_TRY_ADDREF_P(val);
			// �����key
			if (key) {
				// ʹ��key����Ԫ��
				zend_hash_update(result_ht, key, val);
			// ����û��key
			} else {
				// ������Ŀ����Ϊ��ֵ���뵽��ϣ������ʧ��
				if (!zend_hash_next_index_insert(result_ht, val)) {
					// �������Ԫ��ʧ�ܣ���һ��Ԫ���Ѿ�����
					zend_cannot_add_element();
					// ����û�����ô����Ķ���,��û������ zval����������gc�������ڡ�
					zval_ptr_dtor_nogc(val);
					// ����
					break;
				}
			}
		} ZEND_HASH_FOREACH_END();
		
	// op1�����Ƕ���
	} else if (EXPECTED(Z_TYPE_P(op1) == IS_OBJECT)) {
		// op1��ȡ��������
		zend_class_entry *ce = Z_OBJCE_P(op1);
		// 
		zend_object_iterator *iter;
		// ���û���� �� ��û�е�����
		if (!ce || !ce->get_iterator) {
			// ����ֻ�пɱ������Ϳ��Ա� ���
			zend_type_error("Only arrays and Traversables can be unpacked");
		// ����
		} else {
			// ȡ�õ�������0:��ʹ������
			iter = ce->get_iterator(ce, op1, 0);
			// �����ȡ������ʧ��
			if (UNEXPECTED(!iter)) {
				// �ͷŲ�������ĸ��ӱ���
				FREE_OP1();
				// ���û���쳣
				if (!EG(exception)) {
					// ���쳣�������͵Ķ����޷�����������
					zend_throw_exception_ex(
						NULL, 0, "Object of type %s did not create an Iterator", ZSTR_VAL(ce->name)
					);
				}
				// ��Ҫ�� return
				HANDLE_EXCEPTION();
			}
			// �����������ɹ�
			
			const zend_object_iterator_funcs *funcs = iter->funcs;
			// ����и�λ����
			if (funcs->rewind) {
				// �ȸ�λ������
				funcs->rewind(iter);
			}

			// ֻҪԪ����Ч��һֱ����
			for (; funcs->valid(iter) == SUCCESS; ) {
				zval *val;
				// ������쳣
				if (UNEXPECTED(EG(exception) != NULL)) {
					// ����
					break;
				}

				// ȡ����ǰԪ��
				val = funcs->get_current_data(iter);
				// ������쳣
				if (UNEXPECTED(EG(exception) != NULL)) {
					// ����
					break;
				}

				zval key;
				// ����л�ȡkey�ķ���
				if (funcs->get_current_key) {
					// ��ȡ��ǰkey
					funcs->get_current_key(iter, &key);
					// ������쳣
					if (UNEXPECTED(EG(exception) != NULL)) {
						// ����
						break;
					}
					// key�������� ���� �����ִ�
					if (UNEXPECTED(Z_TYPE(key) != IS_LONG && Z_TYPE(key) != IS_STRING)) {
						// ������������У�key����������|�ִ�
						zend_throw_error(NULL,
							"Keys must be of type int|string during array unpacking");
						// ���key
						zval_ptr_dtor(&key);
						// ����
						break;
					}
				// ����
				} else {
					// ���Ϊ δ����
					ZVAL_UNDEF(&key);
				}
				// ������
				ZVAL_DEREF(val);
				// �������ô���
				Z_TRY_ADDREF_P(val);

				zend_ulong num_key;
				// ���key���ִ������Ҳ���ת������
				if (Z_TYPE(key) == IS_STRING && !ZEND_HANDLE_NUMERIC(Z_STR(key), num_key)) {
					// ���¹�ϣ�����ֵ
					zend_hash_update(result_ht, Z_STR(key), val);
					// ͨ��ָ������ zval���ִ���
					zval_ptr_dtor_str(&key);
				// ����
				} else {
					// ����key
					zval_ptr_dtor(&key);
					// �ڹ�ϣ���в�����Ԫ��.���ʧ��
					if (!zend_hash_next_index_insert(result_ht, val)) {
						// �������Ԫ��ʧ�ܣ���һ��Ԫ���Ѿ�����
						zend_cannot_add_element();
						// ����û�����ô����Ķ���,��û������ zval����������gc�������ڡ�
						zval_ptr_dtor_nogc(val);
						// ����
						break;
					}
				}

				// ����������ƶ�
				funcs->move_forward(iter);
				// ������쳣
				if (UNEXPECTED(EG(exception))) {
					// ����
					break;
				}
			}
			// ���ٵ�����
			zend_iterator_dtor(iter);
		}
	// op1����������
	} else if (EXPECTED(Z_ISREF_P(op1))) {
		// ������
		op1 = Z_REFVAL_P(op1);
		// ��ת��ָ����ǩ add_unpack_again
		ZEND_VM_C_GOTO(add_unpack_again);
	// ������������
	} else {
		// ����ֻ�пɱ������Ϳ��Ա� ���
		zend_throw_error(NULL, "Only arrays and Traversables can be unpacked");
	}

	// �ͷŲ�������ĸ��ӱ���
	FREE_OP1();
	// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing2 , ��ʼ�����飬 1/1��ARRAY_INIT|REF ��
// ZEND_FASTCALL ZEND_INIT_ARRAY_SPEC��20����֧����֤����
ZEND_VM_HANDLER(71, ZEND_INIT_ARRAY, CONST|TMP|VAR|CV|UNUSED, CONST|TMPVAR|UNUSED|NEXT|CV, ARRAY_INIT|REF)
{
	zval *array;
	uint32_t size;
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	// ����ֵ
	// ��ִ��������ȡ�� ָ����ŵı���
	array = EX_VAR(opline->result.var);
	// ��� op1 ���� δʹ��
	if (OP1_TYPE != IS_UNUSED) {
		// �����С������չ��Ϣ��
		size = opline->extended_value >> ZEND_ARRAY_SIZE_SHIFT;
		// ��Ҫ��Ĵ�С���������飬�����������
		ZVAL_ARR(array, zend_new_array(size));
		// ��ʾ��ʼ���ɹ�ϣ��
		/* Explicitly initialize array as not-packed if flag is set */
		// �����������Ҫ�� ��ʼ���ɹ�ϣ��
		if (opline->extended_value & ZEND_ARRAY_NOT_PACKED) {
			// ���������ʼ���ɹ�ϣ��
			zend_hash_real_init_mixed(Z_ARRVAL_P(array));
		}
		// ��ת�� ZEND_ADD_ARRAY_ELEMENT, ���Ԫ��
		ZEND_VM_DISPATCH_TO_HANDLER(ZEND_ADD_ARRAY_ELEMENT);
	// ��� op1 ��δʹ��
	// ����
	} else {
		// �½�һ�����飬����������ֵ
		ZVAL_ARR(array, zend_new_array(0));
		// opline + 1����Ŀ������벢 return
		ZEND_VM_NEXT_OPCODE();
	}
}

// ing3, cast����
// boolval: _IS_BOOL. intval: IS_LONG. floatval,doubleval: IS_DOUBLE. strval: IS_STRING
ZEND_VM_COLD_CONST_HANDLER(51, ZEND_CAST, CONST|TMP|VAR|CV, ANY, TYPE)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *expr;
	// ��ִ��������ȡ�� ָ����ŵı���
	zval *result = EX_VAR(opline->result.var);
	HashTable *ht;

	// windows: �޲���
	SAVE_OPLINE();
	// ��ȡzvalָ�룬UNUSED ���ͷ���null
	expr = GET_OP1_ZVAL_PTR(BP_VAR_R);

	// ������չֵ����
	switch (opline->extended_value) {
		// ����
		case IS_LONG:
			// ֱ��ת������
			ZVAL_LONG(result, zval_get_long(expr));
			break;
		// С��
		case IS_DOUBLE:
			// ֱ��ת��С��
			ZVAL_DOUBLE(result, zval_get_double(expr));
			break;
		// �ִ�
		case IS_STRING:
			// ֱ��ת���ִ�
			ZVAL_STR(result, zval_get_string(expr));
			break;
		// ����
		default:
			// �������� _IS_BOOL
			ZEND_ASSERT(opline->extended_value != _IS_BOOL && "Must use ZEND_BOOL instead");
			// ��� op1 ����ͨ������������
			if (OP1_TYPE & (IS_VAR|IS_CV)) {
				// ������
				ZVAL_DEREF(expr);
			}
			// ����Ѿ�����ȷ�����ͣ�ֱ�ӷ�����
			/* If value is already of correct type, return it directly */
			// ���������Ҫ�������
			if (Z_TYPE_P(expr) == opline->extended_value) {
				// ���Ƹ���� 
				ZVAL_COPY_VALUE(result, expr);
				// ���op1�ǳ���
				if (OP1_TYPE == IS_CONST) {
					// ����ǿ��Լ�������
					// ���ô���+1
					if (UNEXPECTED(Z_OPT_REFCOUNTED_P(result))) Z_ADDREF_P(result);
				// ���� ��� op1 ���� ��ʱ����
				} else if (OP1_TYPE != IS_TMP_VAR) {
					// ����ǿ��Լ�������
					// ���ô���+1
					if (Z_OPT_REFCOUNTED_P(result)) Z_ADDREF_P(result);
				}
				// ֻ�ͷ�����Ϊ IS_VAR �ı���
				FREE_OP1_IF_VAR();
				// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
				ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
			}
			// ���������ȷ������

			// Ŀ������������
			if (opline->extended_value == IS_ARRAY) {
				// ���op1������ ���� �� ���ʽ���Ƕ��� ����ʽ�Ǳհ�
				if (OP1_TYPE == IS_CONST || Z_TYPE_P(expr) != IS_OBJECT || Z_OBJCE_P(expr) == zend_ce_closure) {
					// ������ʽ����null
					if (Z_TYPE_P(expr) != IS_NULL) {
						// �������1��Ԫ�ص������� 
						ZVAL_ARR(result, zend_new_array(1));
						// �ѱ��ʽ�����ӽ���
						expr = zend_hash_index_add_new(Z_ARRVAL_P(result), 0, expr);
						// ���op1�ǳ���
						if (OP1_TYPE == IS_CONST) {
							// ����ǿ��Լ�������
							// ���ô���+1
							if (UNEXPECTED(Z_OPT_REFCOUNTED_P(expr))) Z_ADDREF_P(expr);
						// ����
						} else {
							// ����ǿ��Լ�������
							// ���ô���+1
							if (Z_OPT_REFCOUNTED_P(expr)) Z_ADDREF_P(expr);
						}
					// ���򣬱��ʽ��null
					} else {
						// ���Ϊ������
						ZVAL_EMPTY_ARRAY(result);
					}
				// ������� û�����Ա� ���� û�л�ȡ���Եķ��� ���� ʹ�ñ�׼���Ի�ȡ����
				} else if (Z_OBJ_P(expr)->properties == NULL
				 && Z_OBJ_HT_P(expr)->get_properties_for == NULL
				 && Z_OBJ_HT_P(expr)->get_properties == zend_std_get_properties) {
					// ����Ҫ�ؽ����Թ�ϣ����Ż�����
					/* Optimized version without rebuilding properties HashTable */
					// ���ñ�׼������ȡ��������Ա��Ž������
					ZVAL_ARR(result, zend_std_build_object_properties_array(Z_OBJ_P(expr)));
				// �������Լ������Ի�ȡ����
				} else {
					// ���ö����Լ������Ի�ȡ����
					HashTable *obj_ht = zend_get_properties_for(expr, ZEND_PROP_PURPOSE_ARRAY_CAST);
					// ������Ա���Ч
					if (obj_ht) {
						// ���ٸ���
						/* fast copy */
						// ȡ�ö���ķ��ű��Ž������
						// �����Ա�ֻ�����ִ�key����ת�ɷ��ű� ���������ֺͷ�����key����p1����ϣ��p2:���Ƿ��ظ���
						// ���뷵�ظ����������1.��Ĭ�����ԡ�2.û��ʹ�ñ�׼���󷽷���3.�еݹ�����		
						ZVAL_ARR(result, zend_proptable_to_symtable(obj_ht,
							(Z_OBJCE_P(expr)->default_properties_count ||
							 Z_OBJ_P(expr)->handlers != &std_object_handlers ||
							 GC_IS_RECURSIVE(obj_ht))));
						// �ͷ���ʱ���Թ�ϣ��
						zend_release_properties(obj_ht);
					// �������Ա���Ч
					} else {
						// ����ǿ����� 
						ZVAL_EMPTY_ARRAY(result);
					}
				}
			// ����Ŀ�����Ͳ������� 
			} else {
				// Ŀ�����ͱ����Ƕ���
				ZEND_ASSERT(opline->extended_value == IS_OBJECT);
				// ���Ϊ �½��� ��׼������ ʵ��
				ZVAL_OBJ(result, zend_objects_new(zend_standard_class_def));
				// ���Դֵ������
				if (Z_TYPE_P(expr) == IS_ARRAY) {
					// ȡ�÷��ű�
					ht = zend_symtable_to_proptable(Z_ARR_P(expr));
					// ������ű��ɸ���
					if (GC_FLAGS(ht) & IS_ARRAY_IMMUTABLE) {
						/* TODO: try not to duplicate immutable arrays as well ??? */
						// ���Ʒ��ű�
						ht = zend_array_dup(ht);
					}
					// �ѷ��ű���Ϊ �¶�������Ա�
					Z_OBJ_P(result)->properties = ht;
				// ���Դֵ�������飬Ҳ���� null
				} else if (Z_TYPE_P(expr) != IS_NULL) {
					// ����1������1��Ԫ�صĹ�ϣ����Ϊ �¶�������Ա�
					Z_OBJ_P(result)->properties = ht = zend_new_array(1);
					// ���ű��������Ԫ�� keyΪ scalar
					expr = zend_hash_add_new(ht, ZSTR_KNOWN(ZEND_STR_SCALAR), expr);
					// ���op1�ǳ���
					if (OP1_TYPE == IS_CONST) {
						// ����ǿ��Լ�������
						// ���ô���+1
						if (UNEXPECTED(Z_OPT_REFCOUNTED_P(expr))) Z_ADDREF_P(expr);
					// ����
					} else {
						// ����ǿ��Լ�������
						// ���ô���+1
						if (Z_OPT_REFCOUNTED_P(expr)) Z_ADDREF_P(expr);
					}
				}
			}
	}

	// �ͷŲ�������ĸ��ӱ���
	FREE_OP1();
	// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, include �� eval ����
ZEND_VM_HANDLER(73, ZEND_INCLUDE_OR_EVAL, CONST|TMPVAR|CV, ANY, EVAL, SPEC(OBSERVER))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zend_op_array *new_op_array;
	zval *inc_filename;

	// windows: �޲���
	SAVE_OPLINE();
	// ��ȡzvalָ�룬UNUSED ���ͷ���null
	inc_filename = GET_OP1_ZVAL_PTR(BP_VAR_R);
	// include �� eval ������ִ���߼���p1:�ļ�����p2:����
	new_op_array = zend_include_or_eval(inc_filename, opline->extended_value);
	// ������쳣
	if (UNEXPECTED(EG(exception) != NULL)) {
		// �ͷŲ�������ĸ��ӱ���
		FREE_OP1();
		// ��������벻�� -1 ���� ����null
		if (new_op_array != ZEND_FAKE_OP_ARRAY && new_op_array != NULL) {
			// ���������ɵĲ�����
			destroy_op_array(new_op_array);
			// �ͷ������ɵĲ�����
			efree_size(new_op_array, sizeof(zend_op_array));
		}
		// ��� opline ��������� ��������ʱ�������ѽ�� zval ���Ϊδ����
		UNDEF_RESULT();
		// ��Ҫ�� return
		HANDLE_EXCEPTION();
	// ������ص��� -1������Ҫʹ�ò�����
	} else if (new_op_array == ZEND_FAKE_OP_ARRAY) {
		// ��� ���������������Ч(����IS_UNUSED)
		if (RETURN_VALUE_USED(opline)) {
			// ���Ϊ true
			ZVAL_TRUE(EX_VAR(opline->result.var));
		}
	// ������ص��� null
	} else if (UNEXPECTED(new_op_array == NULL)) {
		// ��� ���������������Ч(����IS_UNUSED)
		if (RETURN_VALUE_USED(opline)) {
			// ���Ϊ false
			ZVAL_FALSE(EX_VAR(opline->result.var));
		}
	// ���������ֻ��1�У������� return ,���� op1�����ǳ��������� zend_execute_ex == execute_ex ��
	// ����Ҫִ��
	} else if (new_op_array->last == 1
			&& new_op_array->opcodes[0].opcode == ZEND_RETURN
			&& new_op_array->opcodes[0].op1_type == IS_CONST
			&& EXPECTED(zend_execute_ex == execute_ex)) {
		// ��� ���������������Ч(����IS_UNUSED)
		if (RETURN_VALUE_USED(opline)) {
			// �²������б�
			const zend_op *op = new_op_array->opcodes;

			// ��op1�ĳ��� ���Ƹ����
			// ���� (p2).zv��p1:opline,p2:node
			ZVAL_COPY(EX_VAR(opline->result.var), RT_CONSTANT(op, op->op1));
		}
		// �����²����뾲̬������
		zend_destroy_static_vars(new_op_array);
		// �����²�����
		destroy_op_array(new_op_array);
		// �ͷ��²�����
		efree_size(new_op_array, sizeof(zend_op_array));
	// ������Ҫִ��
	} else {
		zval *return_value = NULL;
		zend_execute_data *call;
		// ��� ���������������Ч(����IS_UNUSED)
		if (RETURN_VALUE_USED(opline)) {
			// ��ִ��������ȡ�� ָ����ŵı���
			return_value = EX_VAR(opline->result.var);
		}

		// ��
		new_op_array->scope = EX(func)->op_array.scope;

		// ing3, ��ʼ�������� ��ջ����һ��ִ�����ݣ��ռ䲻������չ��С
		// p1:������Ϣ��p2:������p3:����������p4:����������
		// ��� ����ԭ���� ����$this��+ ��Ƕ�׵��á�+�з��ű�
		call = zend_vm_stack_push_call_frame(
			(Z_TYPE_INFO(EX(This)) & ZEND_CALL_HAS_THIS) | ZEND_CALL_NESTED_CODE | ZEND_CALL_HAS_SYMBOL_TABLE,
			(zend_function*)new_op_array, 0,
			Z_PTR(EX(This)));

		// ��� �з��ű� ���
		if (EX_CALL_INFO() & ZEND_CALL_HAS_SYMBOL_TABLE) {
			// ʹ��ִ�����ݵķ��ű�
			call->symbol_table = EX(symbol_table);
		// ����û�з��ű���
		} else {
			// �ؽ����ű�
			call->symbol_table = zend_rebuild_symbol_table();
		}

		// ��ǰһ��ִ������
		call->prev_execute_data = execute_data;
		// ��ʼ��p1ִ�����ݣ�p1:ִ�����ݣ�p2:���� ִ��ʱ���� �Ĳ������б�p3:����ֵ
		i_init_code_execute_data(call, new_op_array, return_value);
		// ��ʼ�۲��ߵ���
		// "/ZEND_OBSERVER_FCALL_BEGIN\(\s*(.*)\s*\)/" => isset($extra_spec['OBSERVER']) ?            ($extra_spec['OBSERVER'] == 0 ? "" : "zend_observer_fcall_begin(\\1)")            : "",
		ZEND_OBSERVER_FCALL_BEGIN(call);
		// zend_execute_ex ��ʲô�ģ�
		if (EXPECTED(zend_execute_ex == execute_ex)) {
			// �ͷŲ�������ĸ��ӱ���
			FREE_OP1();
			// ing3, windows return 1;
			ZEND_VM_ENTER();
		// ����
		} else {
			// ��ӵ��ñ�ǣ��������
			ZEND_ADD_CALL_FLAG(call, ZEND_CALL_TOP);
			// ִ�д�����
			zend_execute_ex(call);
			// �ͷŵ��ÿ�ܣ�ִ�����ݣ���p1:ִ������
			zend_vm_stack_free_call_frame(call);
		}
		// �����²����뾲̬������
		zend_destroy_static_vars(new_op_array);
		// �����²�����
		destroy_op_array(new_op_array);
		// �ͷ��²�����
		efree_size(new_op_array, sizeof(zend_op_array));
		// ������쳣
		if (UNEXPECTED(EG(exception) != NULL)) {
			// ��ִ�������� �����״�
			zend_rethrow_exception(execute_data);
			// �ͷŲ�������ĸ��ӱ���
			FREE_OP1();
			// ��� opline ��������� ��������ʱ�������ѽ�� zval ���Ϊδ����
			UNDEF_RESULT();
			// ��Ҫ�� return
			HANDLE_EXCEPTION();
		}
	}
	// �ͷŲ�������ĸ��ӱ���
	FREE_OP1();
	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, ɾ�� �������
ZEND_VM_HANDLER(153, ZEND_UNSET_CV, CV, UNUSED)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	// ��ִ��������ȡ�� ָ����ŵı���
	zval *var = EX_VAR(opline->op1.var);

	// ����� �ɼ�������
	if (Z_REFCOUNTED_P(var)) {
		// ȡ��������
		zend_refcounted *garbage = Z_COUNTED_P(var);

		// ���Ϊ δ����
		ZVAL_UNDEF(var);
		// windows: �޲���
		SAVE_OPLINE();
		// ���ô���-1�����Ϊ0
		if (!GC_DELREF(garbage)) {
			// ����ÿ��type��Ӧ�����ٺ���ִ������
			rc_dtor_func(garbage);
		// ����
		} else {
			// ��ӵ����ն���
			gc_check_possible_root(garbage);
		}
		// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
		ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
	// ����
	} else {
		// ���Ϊ δ����
		ZVAL_UNDEF(var);
	}
	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, �� GLOBAL �� ��ǰ�������� ɾ������
ZEND_VM_HANDLER(74, ZEND_UNSET_VAR, CONST|TMPVAR|CV, UNUSED, VAR_FETCH)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *varname;
	zend_string *name, *tmp_name;
	HashTable *target_symbol_table;

	// windows: �޲���
	SAVE_OPLINE();

	// op1�д�ű�����
	// ��ȡzvalָ��, UNUSED ����null
	varname = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);

	// ���op1�ǳ���
	if (OP1_TYPE == IS_CONST) {
		// ֱ��ȡ��������
		name = Z_STR_P(varname);
	// ������������ִ�
	} else if (EXPECTED(Z_TYPE_P(varname) == IS_STRING)) {
		// ֱ��ȡ��������
		name = Z_STR_P(varname);
		// ��ʱ����Ϊ��
		tmp_name = NULL;
	// ����
	} else {
		// ��� op1 �� ������� ���ұ�����Ϊ δ����
		if (OP1_TYPE == IS_CV && UNEXPECTED(Z_TYPE_P(varname) == IS_UNDEF)) {
			// ����p1�ı�������������δ����, ����δ��ʼ��zval
			varname = ZVAL_UNDEFINED_OP1();
		}
		// ��ȡ��ʱ�ִ������״�����
		name = zval_try_get_tmp_string(varname, &tmp_name);
		// ���û�б�����
		if (UNEXPECTED(!name)) {
			// �ͷŲ�������ĸ��ӱ���
			FREE_OP1();
			// ��Ҫ�� return
			HANDLE_EXCEPTION();
		}
	}

	// �����ȡ�ķ��ű��� global ��ǰִ������ �������еı����б�
	// ȡ��Ŀ����ű�����ʱ���ű� �� ��ǰִ�������еķ��ű�
	target_symbol_table = zend_get_target_symbol_table(opline->extended_value EXECUTE_DATA_CC);
	// ** �ڷ��ű���ɾ�� ���Ԫ��
	zend_hash_del_ind(target_symbol_table, name);

	// ��� OP1 ���ǳ���
	if (OP1_TYPE != IS_CONST) {
		// �ͷ���ʱ�ִ�
		zend_tmp_string_release(tmp_name);
	}
	// �ͷŲ�������ĸ��ӱ���
	FREE_OP1();
	// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, ɾ����̬���ԣ����ֱ���
/* No specialization for op_types (CONST|TMPVAR|CV, UNUSED|CLASS_FETCH|CONST|VAR) */
ZEND_VM_COLD_HANDLER(179, ZEND_UNSET_STATIC_PROP, ANY, ANY, CACHE_SLOT)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *varname;
	zend_string *name, *tmp_name = NULL;
	zend_class_entry *ce;

	// windows: �޲���
	SAVE_OPLINE();

	// �������Ϊ ����
	if (OP2_TYPE == IS_CONST) {
		
		// �Ӳ�������չ��Ϣ�� ȡ������Ϣ
		// ͨ��ƫ���� ������ʱ�����л�ȡһ�� (void**) �еĵ�һ��Ԫ��
		ce = CACHED_PTR(opline->extended_value);
		// ��� û��ȡ����Ч��Ϣ
		if (UNEXPECTED(ce == NULL)) {
			// ��ȡ�࣬�Ҳ����ᱨ��p1:������p2:key��p3:flags
			// ���� (p2).zv��p1:opline,p2:node
			ce = zend_fetch_class_by_name(Z_STR_P(RT_CONSTANT(opline, opline->op2)), Z_STR_P(RT_CONSTANT(opline, opline->op2) + 1), ZEND_FETCH_CLASS_DEFAULT | ZEND_FETCH_CLASS_EXCEPTION);
			// ��� �಻����
			if (UNEXPECTED(ce == NULL)) {
				// �ͷŲ�������ĸ��ӱ���
				FREE_OP1();
				// ��Ҫ�� return
				HANDLE_EXCEPTION();
			}
			/*CACHE_PTR(opline->extended_value, ce);*/
		}
	// ����
	// ��� op2 ������δ����
	} else if (OP2_TYPE == IS_UNUSED) {
		// ͨ�����������Ͳ����࣬p1:������p2:����
		ce = zend_fetch_class(NULL, opline->op2.num);
		// ��� �಻����
		if (UNEXPECTED(ce == NULL)) {
			// �ͷŲ�������ĸ��ӱ���
			FREE_OP1();
			// ��Ҫ�� return
			HANDLE_EXCEPTION();
		}
	// ����
	} else {
		// ȡ�� �����
		ce = Z_CE_P(EX_VAR(opline->op2.var));
	}

	// ��ȡzvalָ��, UNUSED ����null
	varname = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ���op1�ǳ���
	if (OP1_TYPE == IS_CONST) {
		// ȡ��������
		name = Z_STR_P(varname);
	// ��� �������� �ִ�
	} else if (EXPECTED(Z_TYPE_P(varname) == IS_STRING)) {
		// ȡ�� �������ִ�
		name = Z_STR_P(varname);
	// ����
	} else {
		// ��� op1 �� ������� ���ұ�����Ϊ δ����
		if (OP1_TYPE == IS_CV && UNEXPECTED(Z_TYPE_P(varname) == IS_UNDEF)) {
			// ����p1�ı�������������δ����, ����δ��ʼ��zval
			varname = ZVAL_UNDEFINED_OP1();
		}
		// ��ȡ��ʱ�ִ������״�����
		name = zval_try_get_tmp_string(varname, &tmp_name);
		// ���û������
		if (UNEXPECTED(!name)) {
			// �ͷŲ�������ĸ��ӱ���
			FREE_OP1();
			// ��Ҫ�� return
			HANDLE_EXCEPTION();
		}
	}
	// ����: ��̬���Բ���ɾ��
	zend_std_unset_static_property(ce, name);
	// �ͷ���ʱ����
	zend_tmp_string_release(tmp_name);
	// �ͷŲ�������ĸ��ӱ���
	FREE_OP1();
	// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, �������ͣ�������һ��ɾ��Ԫ��
ZEND_VM_HANDLER(75, ZEND_UNSET_DIM, VAR|CV, CONST|TMPVAR|CV)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *container;
	zval *offset;
	zend_ulong hval;
	zend_string *key;

	// windows: �޲���
	SAVE_OPLINE();
	// ��op1��ȡ������
	// ��ȡzvalָ��, TMP/CONST����null, UNUSED ���� $this����֧�� TMPVAR/TMPVARCV��CV ֱ�ӷ��ء�
	container = GET_OP1_OBJ_ZVAL_PTR_PTR_UNDEF(BP_VAR_UNSET);
	
	// ��op2��ȡ�� offset
	// ��ȡzvalָ��, UNUSED ����null
	offset = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);

	do {
		// ��� ���� ������ ����
		if (EXPECTED(Z_TYPE_P(container) == IS_ARRAY)) {
			HashTable *ht;

// ������ת��ǩ unset_dim_array
ZEND_VM_C_LABEL(unset_dim_array):
			// �� zval ָ��� ���� ��������������ָ��ָ���¸���
			SEPARATE_ARRAY(container);
			// ȡ������
			ht = Z_ARRVAL_P(container);
// ������ת��ǩ offset_again
ZEND_VM_C_LABEL(offset_again):
			// ���ƫ�������ִ�(key)
			if (EXPECTED(Z_TYPE_P(offset) == IS_STRING)) {
				// ȡ��key
				key = Z_STR_P(offset);
				// ��� OP2 ���ǳ���
				if (OP2_TYPE != IS_CONST) {
					// ���key��������
					if (ZEND_HANDLE_NUMERIC(key, hval)) {
						// ��ת��ָ����ǩ num_index_dim
						ZEND_VM_C_GOTO(num_index_dim);
					}
				}
// ������ת��ǩ str_index_dim
ZEND_VM_C_LABEL(str_index_dim):
				// ���鲻�ǿ����ǵ�ǰ���ű�
				ZEND_ASSERT(ht != &EG(symbol_table));
				// ʹ��key, ��������ɾ��Ԫ��
				zend_hash_del(ht, key);
			// ���ƫ����������
			} else if (EXPECTED(Z_TYPE_P(offset) == IS_LONG)) {
				// ȡ����ϣֵ
				hval = Z_LVAL_P(offset);
// ������ת��ǩ num_index_dim
ZEND_VM_C_LABEL(num_index_dim):
				// ʹ�ù�ϣֵ����������ɾ��ָ��Ԫ��
				zend_hash_index_del(ht, hval);
			// ���� ��� op2 ����ͨ������������ ���� op2 ����������
			} else if ((OP2_TYPE & (IS_VAR|IS_CV)) && EXPECTED(Z_TYPE_P(offset) == IS_REFERENCE)) {
				// ������
				offset = Z_REFVAL_P(offset);
				// ��ת��ָ����ǩ offset_again
				ZEND_VM_C_GOTO(offset_again);
			// ƫ������С��
			} else if (Z_TYPE_P(offset) == IS_DOUBLE) {
				// ȡ��С������ת������
				hval = zend_dval_to_lval_safe(Z_DVAL_P(offset));
				// ��ת��ָ����ǩ num_index_dim
				ZEND_VM_C_GOTO(num_index_dim);
			// ƫ������null
			} else if (Z_TYPE_P(offset) == IS_NULL) {
				// ���ִ�
				key = ZSTR_EMPTY_ALLOC();
				// ��ת��ָ����ǩ str_index_dim
				ZEND_VM_C_GOTO(str_index_dim);
			// ƫ���� false
			} else if (Z_TYPE_P(offset) == IS_FALSE) {
				// ��ϣֵΪ0
				hval = 0;
				// ��ת��ָ����ǩ num_index_dim
				ZEND_VM_C_GOTO(num_index_dim);
			// ƫ���� true
			} else if (Z_TYPE_P(offset) == IS_TRUE) {
				// ��ϣֵΪ1
				hval = 1;
				// ��ת��ָ����ǩ num_index_dim
				ZEND_VM_C_GOTO(num_index_dim);
			// ƫ������ ��Դ����
			} else if (Z_TYPE_P(offset) == IS_RESOURCE) {
				// �׳����棺��Դ��Ϊƫ������ת�������֣��Լ���ID��
				zend_use_resource_as_offset(offset);
				// ȡ����Դid
				hval = Z_RES_HANDLE_P(offset);
				// ��ת��ָ����ǩ num_index_dim
				ZEND_VM_C_GOTO(num_index_dim);
			// ���� ��� op2 �� ������� ���� ƫ���� Ϊδ����
			} else if (OP2_TYPE == IS_CV && Z_TYPE_P(offset) == IS_UNDEF) {
				// ����p2�ı�������������δ����, ����δ��ʼ��zval
				ZVAL_UNDEFINED_OP2();
				// key�ǿ��ִ�
				key = ZSTR_EMPTY_ALLOC();
				// ��ת��ָ����ǩ str_index_dim
				ZEND_VM_C_GOTO(str_index_dim);
			// ����, �޷�ʶ���ƫ����
			} else {
				// unsetʱʹ���˷Ƿ����͵�ƫ����
				zend_type_error("Illegal offset type in unset");
			}
			break;
		// �����������������
		} else if (Z_ISREF_P(container)) {
			// ������
			container = Z_REFVAL_P(container);
			// ��� ���� ������ ����
			if (EXPECTED(Z_TYPE_P(container) == IS_ARRAY)) {
				// ��ת��ָ����ǩ unset_dim_array
				ZEND_VM_C_GOTO(unset_dim_array);
			}
		}
		// ��� op1 �� ������� ���� �������� Ϊδ����
		if (OP1_TYPE == IS_CV && UNEXPECTED(Z_TYPE_P(container) == IS_UNDEF)) {
			// ����p1�ı�������������δ����, ����δ��ʼ��zval
			container = ZVAL_UNDEFINED_OP1();
		}
		// ��� op2 �� ������� ���� ƫ���� Ϊδ����
		if (OP2_TYPE == IS_CV && UNEXPECTED(Z_TYPE_P(offset) == IS_UNDEF)) {
			// ����p2�ı�������������δ����, ����δ��ʼ��zval
			offset = ZVAL_UNDEFINED_OP2();
		}
		// ��� ���� ������ ����
		if (EXPECTED(Z_TYPE_P(container) == IS_OBJECT)) {
			// ���op2�ǳ��� ���� ����չֵ
			if (OP2_TYPE == IS_CONST && Z_EXTRA_P(offset) == ZEND_EXTRA_VALUE) {
				// ȡ����һ��zval
				offset++;
			}
			// ���ö���� unset_dimension ����
			Z_OBJ_HT_P(container)->unset_dimension(Z_OBJ_P(container), offset);
		// ���� ��� ���� ������ �ִ�
		} else if (UNEXPECTED(Z_TYPE_P(container) == IS_STRING)) {
			// ���������ԶԸ��ִ� ʹ��ƫ����
			zend_throw_error(NULL, "Cannot unset string offsets");
		// ���� ��� ���� ���� ���� false (true�����ϣ�
		} else if (UNEXPECTED(Z_TYPE_P(container) > IS_FALSE)) {
			// �����������ڶԷ�array������ʹ�� ƫ����ɾ��
			zend_throw_error(NULL, "Cannot unset offset in a non-array variable");
		// ���� ��� ���� ���� ��false
		} else if (UNEXPECTED(Z_TYPE_P(container) == IS_FALSE)) {
			// ������falseת������������
			zend_false_to_array_deprecated();
		}
	} while (0);

	// �ͷŲ�������ĸ��ӱ���
	FREE_OP2();
	// �ͷŲ�������ĸ��ӱ���
	FREE_OP1();
	// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, ɾ����������
ZEND_VM_HANDLER(76, ZEND_UNSET_OBJ, VAR|UNUSED|THIS|CV, CONST|TMPVAR|CV, CACHE_SLOT)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *container;
	zval *offset;
	zend_string *name, *tmp_name;

	// windows: �޲���
	SAVE_OPLINE();
	// op1�ı���������
	// ��ȡzvalָ��, TMP/CONST����null, UNUSED ���� $this����֧�� TMPVAR/TMPVARCV��CV ֱ�ӷ��ء�
	container = GET_OP1_OBJ_ZVAL_PTR_PTR_UNDEF(BP_VAR_UNSET);
	// ��ȡzvalָ�룬UNUSED ���ͷ���null
	offset = GET_OP2_ZVAL_PTR(BP_VAR_R);

	// Ϊ��break
	do {
		// ���op1��Ч ���� �������� ����
		if (OP1_TYPE != IS_UNUSED && UNEXPECTED(Z_TYPE_P(container) != IS_OBJECT)) {
			// �������������
			if (Z_ISREF_P(container)) {
				// ������
				container = Z_REFVAL_P(container);
				// ��������Ƕ���
				if (Z_TYPE_P(container) != IS_OBJECT) {
					// ��� op1 �� ������� ������������Ϊ δ����
					if (OP1_TYPE == IS_CV
					 && UNEXPECTED(Z_TYPE_P(container) == IS_UNDEF)) {
						 // ����p1�ı�������������δ����, ����δ��ʼ��zval
						ZVAL_UNDEFINED_OP1();
					}
					// ����ɾ��
					break;
				}
			// ����, ������������
			} else {
				// û����Ч��������ɾ��
				break;
			}
		}
		// �������Ϊ ����
		if (OP2_TYPE == IS_CONST) {
			// ��ȡ��������
			name = Z_STR_P(offset);
		// ����
		} else {
			// ��ȡ��ʱ�ִ������״�����
			name = zval_try_get_tmp_string(offset, &tmp_name);
			// ���û��������
			if (UNEXPECTED(!name)) {
				// ����ɾ��
				break;
			}
		}
		// ���������� ɾ�����Է��� 
		Z_OBJ_HT_P(container)->unset_property(Z_OBJ_P(container), name, ((OP2_TYPE == IS_CONST) ? CACHE_ADDR(opline->extended_value) : NULL));
		// ��� OP2 ���ǳ���
		if (OP2_TYPE != IS_CONST) {
			// �ͷ���ʱ����
			zend_tmp_string_release(tmp_name);
		}
	} while (0);

	// �ͷŲ�������ĸ��ӱ���
	FREE_OP2();
	// �ͷŲ�������ĸ��ӱ���
	FREE_OP1();
	// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, ����ֻ��������
ZEND_VM_HANDLER(77, ZEND_FE_RESET_R, CONST|TMP|VAR|CV, JMP_ADDR)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *array_ptr, *result;

	// windows: �޲���
	SAVE_OPLINE();
	// ��ȡzvalָ��, UNUSED ����null����֧�� TMPVAR/TMPVARCV
	array_ptr = GET_OP1_ZVAL_PTR_DEREF(BP_VAR_R);
	// ��� op1 ������
	if (EXPECTED(Z_TYPE_P(array_ptr) == IS_ARRAY)) {
		// ��ִ��������ȡ�� ָ����ŵı���
		result = EX_VAR(opline->result.var);
		// op1 ���Ƹ� ���
		ZVAL_COPY_VALUE(result, array_ptr);
		// ��� op1 ������ʱ���������� �ǿ��Լ�������
		if (OP1_TYPE != IS_TMP_VAR && Z_OPT_REFCOUNTED_P(result)) {
			// ���ô���+1
			Z_ADDREF_P(array_ptr);
		}
		// ͨ��ָ�����zval�� foreachλ�á�(*p1).u2.fe_pos
		Z_FE_POS_P(result) = 0;
		// ֻ�ͷ�����Ϊ IS_VAR �ı���
		FREE_OP1_IF_VAR();
		// opline + 1����Ŀ������벢 return
		ZEND_VM_NEXT_OPCODE();
	// op1 ���ǳ��� ���� �����Ƕ���
	} else if (OP1_TYPE != IS_CONST && EXPECTED(Z_TYPE_P(array_ptr) == IS_OBJECT)) {
		// ȡ������
		zend_object *zobj = Z_OBJ_P(array_ptr);
		// ���û�л�ȡ����������
		if (!zobj->ce->get_iterator) {
			// ���Ա�
			HashTable *properties = zobj->properties;
			// ��������Ա�
			if (properties) {
				// ������Ա�������>1
				if (UNEXPECTED(GC_REFCOUNT(properties) > 1)) {
					// ������Ա��ɸ���
					if (EXPECTED(!(GC_FLAGS(properties) & IS_ARRAY_IMMUTABLE))) {
						// ���ô���-1
						GC_DELREF(properties);
					}
					// �����Ա�����������ʹ�ø���
					properties = zobj->properties = zend_array_dup(properties);
				}
			// ����û�����Ա�
			} else {
				// ���÷�����̬��ȡ���Ա�
				properties = zobj->handlers->get_properties(zobj);
			}

			//
			// ��ִ��������ȡ�� ָ����ŵı���
			result = EX_VAR(opline->result.var);
			// op1 ���Ƹ� ���
			ZVAL_COPY_VALUE(result, array_ptr);
			// ��� op1 ���� ��ʱ����
			if (OP1_TYPE != IS_TMP_VAR) {
				// ���ô���+1
				Z_ADDREF_P(array_ptr);
			}

			// ������Ա��ǿ�
			if (zend_hash_num_elements(properties) == 0) {
				// ����ĵ���λ�� -1
				// ͨ��ָ�����zval�� ���������� , (*p1).u2.fe_iter_idx
				Z_FE_ITER_P(result) = (uint32_t) -1;
				// ֻ�ͷ�����Ϊ IS_VAR �ı���
				FREE_OP1_IF_VAR();
				
				// ���쳣ʱʹ�þɲ����룬���쳣ʹ���²�����, p1:�²�����
				// OP_JMP_ADDR: ���� p2.jmp_addr
				ZEND_VM_JMP(OP_JMP_ADDR(opline, opline->op2));
			}
			// ͨ��ָ�����zval�� ���������� , (*p1).u2.fe_iter_idx
			Z_FE_ITER_P(result) = zend_hash_iterator_add(properties, 0);
			// ֻ�ͷ�����Ϊ IS_VAR �ı���
			FREE_OP1_IF_VAR();
			// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
			ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
		// ���򣬿��Ի�ȡ������
		} else {
			// ���õ�������������Ͷ���Ĺ��������ص�ǰλ���Ƿ�Ϊ�ա�p1:������������� zval��p2:�Ƿ�����
			bool is_empty = zend_fe_reset_iterator(array_ptr, 0 OPLINE_CC EXECUTE_DATA_CC);

			// �ͷŲ�������ĸ��ӱ���
			FREE_OP1();
			// ������쳣
			if (UNEXPECTED(EG(exception))) {
				// ��Ҫ�� return
				HANDLE_EXCEPTION();
			// �����������Ч��û����Чλ�ã�
			} else if (is_empty) {
				// Ҫ����쳣 �� ���쳣ʱʹ�þɲ����룬���쳣ʹ���²�����, p1:�²����룬p2:�Ƿ����쳣
				// OP_JMP_ADDR: ���� p2.jmp_addr
				ZEND_VM_JMP_EX(OP_JMP_ADDR(opline, opline->op2), 0);
			// ����
			} else {
				// opline + 1����Ŀ������벢 return
				ZEND_VM_NEXT_OPCODE();
			}
		}
	// ������������
	} else {
		// ���� foreach() �Ĳ�����������������
		zend_error(E_WARNING, "foreach() argument must be of type array|object, %s given", zend_zval_type_name(array_ptr));
		// ���Ϊ δ����
		ZVAL_UNDEF(EX_VAR(opline->result.var));
		// ͨ��ָ�����zval�� ���������� , (*p1).u2.fe_iter_idx
		Z_FE_ITER_P(EX_VAR(opline->result.var)) = (uint32_t)-1;
		// �ͷŲ�������ĸ��ӱ���
		FREE_OP1();
		// ���쳣ʱʹ�þɲ����룬���쳣ʹ���²�����, p1:�²�����
		// OP_JMP_ADDR: ���� p2.jmp_addr
		ZEND_VM_JMP(OP_JMP_ADDR(opline, opline->op2));
	}
}

// ing2, ���ö�д����
ZEND_VM_COLD_CONST_HANDLER(125, ZEND_FE_RESET_RW, CONST|TMP|VAR|CV, JMP_ADDR)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *array_ptr, *array_ref;

	// windows: �޲���
	SAVE_OPLINE();

	// ��� op1 �� ��ͨ���� �� �������
	if (OP1_TYPE == IS_VAR || OP1_TYPE == IS_CV) {
		// ��ȡzvalָ��,TMP/CONST/UNUSED ����null����֧�� TMPVAR/TMPVARCV
		array_ref = array_ptr = GET_OP1_ZVAL_PTR_PTR(BP_VAR_R);
		// �������������
		if (Z_ISREF_P(array_ref)) {
			// ������
			array_ptr = Z_REFVAL_P(array_ref);
		}
	// ����
	} else {
		// ��ȡzvalָ�룬UNUSED ���ͷ���null
		array_ref = array_ptr = GET_OP1_ZVAL_PTR(BP_VAR_R);
	}

	// ���Ŀ��������
	if (EXPECTED(Z_TYPE_P(array_ptr) == IS_ARRAY)) {
		// ��� op1 �� ��ͨ���� �� �������
		if (OP1_TYPE == IS_VAR || OP1_TYPE == IS_CV) {
			if (array_ptr == array_ref) {
				// �� zval �����µ� zend_reference ���Ӹ����� zval �и��Ƹ�������
				ZVAL_NEW_REF(array_ref, array_ref);
				// ������
				array_ptr = Z_REFVAL_P(array_ref);
			}
			// ���ô���+1
			Z_ADDREF_P(array_ref);
			// 
			ZVAL_COPY_VALUE(EX_VAR(opline->result.var), array_ref);
		// ����
		} else {
			// ��ִ��������ȡ�� ָ����ŵı���
			array_ref = EX_VAR(opline->result.var);
			// �� zval �����µ� zend_reference ���Ӹ����� zval �и��Ƹ�������
			ZVAL_NEW_REF(array_ref, array_ptr);
			// ������
			array_ptr = Z_REFVAL_P(array_ref);
		}
		// ���op1�ǳ���
		if (OP1_TYPE == IS_CONST) {
			// 
			ZVAL_ARR(array_ptr, zend_array_dup(Z_ARRVAL_P(array_ptr)));
		// ����
		} else {
			SEPARATE_ARRAY(array_ptr);
		}
		// ͨ��ָ�����zval�� ���������� , (*p1).u2.fe_iter_idx
		Z_FE_ITER_P(EX_VAR(opline->result.var)) = zend_hash_iterator_add(Z_ARRVAL_P(array_ptr), 0);
		// ֻ�ͷ�����Ϊ IS_VAR �ı���
		FREE_OP1_IF_VAR();
		// opline + 1����Ŀ������벢 return
		ZEND_VM_NEXT_OPCODE();
	// ���Ŀ���Ƕ���
	} else if (OP1_TYPE != IS_CONST && EXPECTED(Z_TYPE_P(array_ptr) == IS_OBJECT)) {
		// ���û�л�ȡ�������ķ���
		if (!Z_OBJCE_P(array_ptr)->get_iterator) {
			HashTable *properties;
			// ��� op1 �� ��ͨ���� �� �������
			if (OP1_TYPE == IS_VAR || OP1_TYPE == IS_CV) {
				if (array_ptr == array_ref) {
					// �� zval �����µ� zend_reference ���Ӹ����� zval �и��Ƹ�������
					ZVAL_NEW_REF(array_ref, array_ref);
					// ������
					array_ptr = Z_REFVAL_P(array_ref);
				}
				// ���ô���+1
				Z_ADDREF_P(array_ref);
				// op1 ���Ƹ� result
				ZVAL_COPY_VALUE(EX_VAR(opline->result.var), array_ref);
			// ����
			} else {
				// ��ִ��������ȡ�� ָ����ŵı���
				array_ptr = EX_VAR(opline->result.var);
				// 
				ZVAL_COPY_VALUE(array_ptr, array_ref);
			}
			// ������������Ա� ���� ���Ա����ô���>1
			if (Z_OBJ_P(array_ptr)->properties
			 && UNEXPECTED(GC_REFCOUNT(Z_OBJ_P(array_ptr)->properties) > 1)) {
				// ������Ա��ǲ��ɸ���
				if (EXPECTED(!(GC_FLAGS(Z_OBJ_P(array_ptr)->properties) & IS_ARRAY_IMMUTABLE))) {
					// ���ô���-1
					GC_DELREF(Z_OBJ_P(array_ptr)->properties);
				}
				// �����Ա�������
				Z_OBJ_P(array_ptr)->properties = zend_array_dup(Z_OBJ_P(array_ptr)->properties);
			}

			// ȡ�����Ա�
			properties = Z_OBJPROP_P(array_ptr);
			// ������Ա���û��Ԫ��
			if (zend_hash_num_elements(properties) == 0) {
				// ͨ��ָ�����zval�� ���������� , (*p1).u2.fe_iter_idx
				Z_FE_ITER_P(EX_VAR(opline->result.var)) = (uint32_t) -1;
				// ֻ�ͷ�����Ϊ IS_VAR �ı���
				FREE_OP1_IF_VAR();
				// ���쳣ʱʹ�þɲ����룬���쳣ʹ���²�����, p1:�²�����
				// OP_JMP_ADDR: ���� p2.jmp_addr
				ZEND_VM_JMP(OP_JMP_ADDR(opline, opline->op2));
			}
			// ͨ��ָ�����zval�� ���������� , (*p1).u2.fe_iter_idx
			Z_FE_ITER_P(EX_VAR(opline->result.var)) = zend_hash_iterator_add(properties, 0);
			// ֻ�ͷ�����Ϊ IS_VAR �ı���
			FREE_OP1_IF_VAR();
			// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
			ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
		// �����л�ȡ�������ķ���
		} else {
			// ���õ�������������Ͷ���Ĺ��������ص�ǰλ���Ƿ�Ϊ�ա�p1:������������� zval��p2:�Ƿ�����
			bool is_empty = zend_fe_reset_iterator(array_ptr, 1 OPLINE_CC EXECUTE_DATA_CC);
			// �ͷŲ�������ĸ��ӱ���
			FREE_OP1();
			// ������쳣
			if (UNEXPECTED(EG(exception))) {
				// ��Ҫ�� return
				HANDLE_EXCEPTION();
			// �����������Ч
			} else if (is_empty) {
				// Ҫ����쳣 �� ���쳣ʱʹ�þɲ����룬���쳣ʹ���²�����, p1:�²����룬p2:�Ƿ����쳣
				// OP_JMP_ADDR: ���� p2.jmp_addr
				ZEND_VM_JMP_EX(OP_JMP_ADDR(opline, opline->op2), 0);
			// ����
			} else {
				// opline + 1����Ŀ������벢 return
				ZEND_VM_NEXT_OPCODE();
			}
		}
	// ���򣬲�������Ͷ���
	} else {
		// ���棺foreach()���� ֻ���� ����Ͷ���
		zend_error(E_WARNING, "foreach() argument must be of type array|object, %s given", zend_zval_type_name(array_ptr));
		// ���Ϊ δ����
		ZVAL_UNDEF(EX_VAR(opline->result.var));
		// ͨ��ָ�����zval�� ���������� , (*p1).u2.fe_iter_idx
		Z_FE_ITER_P(EX_VAR(opline->result.var)) = (uint32_t)-1;
		// �ͷŲ�������ĸ��ӱ���
		FREE_OP1();
		// ���쳣ʱʹ�þɲ����룬���쳣ʹ���²�����, p1:�²�����
		// OP_JMP_ADDR: ���� p2.jmp_addr
		ZEND_VM_JMP(OP_JMP_ADDR(opline, opline->op2));
	}
}

// ing2, ѭ������ 
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
	// ��ִ��������ȡ�� ָ����ŵı���
	array = EX_VAR(opline->op1.var);
	// windows: �޲���
	SAVE_OPLINE();

	// array�����Ƕ���
	ZEND_ASSERT(Z_TYPE_P(array) == IS_OBJECT);
	// ��ȡ zval�еĵ����������˸��жϣ������Ƿ��ǵ�����
	// ���û�е�����
	if ((iter = zend_iterator_unwrap(array)) == NULL) {
		// �򵥶���
		/* plain object */
		
		// ͨ�������� zvalָ�� ��ȡ zend_object �� ����ֵ
		fe_ht = Z_OBJPROP_P(array);
		// ��ȡָ�� ��� ��������ָ��λ�ã�����������͹�ϣ��ƥ�䣬���µ�����
		pos = zend_hash_iterator_pos(Z_FE_ITER_P(array), fe_ht);
		// �ҵ�ָ��Ԫ��
		p = fe_ht->arData + pos;
		// ��ѭ��
		while (1) {
			// ���λ�ó�������Чλ��
			if (UNEXPECTED(pos >= fe_ht->nNumUsed)) {
				// ����ĩβ
				/* reached end of iteration */
				// ��ת��ָ����ǩ fe_fetch_r_exit
				ZEND_VM_C_GOTO(fe_fetch_r_exit);
			}
			// ��һ��λ��
			pos++;
			// ��ֵ�Ե�ֵ
			value = &p->val;
			// ֵ������
			value_type = Z_TYPE_INFO_P(value);
			// ���Ͳ��� δ����
			if (EXPECTED(value_type != IS_UNDEF)) {
				// ��������� �������
				if (UNEXPECTED(value_type == IS_INDIRECT)) {
					// ȡ������Ŀ��
					value = Z_INDIRECT_P(value);
					// Ŀ�������
					value_type = Z_TYPE_INFO_P(value);
					// ���Ͳ��� δ���� ���� 
					// ��������Ϣ ���������Ƿ�ɷ��� p1:����p2:��������p3:�Ƿ��Ƕ�̬
					if (EXPECTED(value_type != IS_UNDEF)
					 && EXPECTED(zend_check_property_access(Z_OBJ_P(array), p->key, 0) == SUCCESS)) {
						// �ҵ�����ЧԪ�� ����
						break;
					}
				// ������Ǽ������ 
				// ���û������ �� ����������Ч �� 
				// ��������Ϣ ���������Ƿ�ɷ��� p1:����p2:��������p3:�Ƿ��Ƕ�̬
				} else if (EXPECTED(Z_OBJCE_P(array)->default_properties_count == 0)
						|| !p->key
						|| zend_check_property_access(Z_OBJ_P(array), p->key, 1) == SUCCESS) {
					// �޷����ң����ҵ���ЧԪ�أ� ����		
					break;
				}
			}
			// ��һ����ֵ��
			p++;
		}
		// ���µ�������λ��
		// ͨ��ָ�����zval�� ���������� , (*p1).u2.fe_iter_idx
		EG(ht_iterators)[Z_FE_ITER_P(array)].pos = pos;
		// ��� ���������������Ч(����IS_UNUSED)
		if (RETURN_VALUE_USED(opline)) {
			// ���û��key
			if (UNEXPECTED(!p->key)) {
				// ���Ϊ bucket �Ĺ�ϣֵ
				ZVAL_LONG(EX_VAR(opline->result.var), p->h);
			//  ���key��Ч
			} else if (ZSTR_VAL(p->key)[0]) {
				// key���Ƹ���� 
				ZVAL_STR_COPY(EX_VAR(opline->result.var), p->key);
			// ����
			} else {
				const char *class_name, *prop_name;
				size_t prop_name_len;
				// ƴ�������ͷ�����
				zend_unmangle_property_name_ex(
					p->key, &class_name, &prop_name, &prop_name_len);
				// �������ŵ������
				ZVAL_STRINGL(EX_VAR(opline->result.var), prop_name, prop_name_len);
			}
		}
	// �����е�����
	} else {
		// �����б�
		const zend_object_iterator_funcs *funcs = iter->funcs;
		
		// ��� ����λ�� ������
		if (EXPECTED(++iter->index > 0)) {
			// ���index�ٴα��0 ������������ѭ������һ���������ֹ�������������
			/* This could cause an endless loop if index becomes zero again.
			 * In case that ever happens we need an additional flag. */
			// ������
			funcs->move_forward(iter);
			// ������쳣
			if (UNEXPECTED(EG(exception) != NULL)) {
				// ��� opline ��������� ��������ʱ�������ѽ�� zval ���Ϊδ����
				UNDEF_RESULT();
				// ��Ҫ�� return
				HANDLE_EXCEPTION();
			}
			// �����ǰλ����Ч
			if (UNEXPECTED(funcs->valid(iter) == FAILURE)) {
				// ���������β
				/* reached end of iteration */
				// ������쳣
				if (UNEXPECTED(EG(exception) != NULL)) {
					// ��� opline ��������� ��������ʱ�������ѽ�� zval ���Ϊδ����
					UNDEF_RESULT();
					// ��Ҫ�� return
					HANDLE_EXCEPTION();
				}
// ������ת��ǩ fe_fetch_r_exit
ZEND_VM_C_LABEL(fe_fetch_r_exit):
				// �ҵ�Ŀ������룬���óɵ�ǰ�����벢���ء� EX(opline) = (zend_op*)((char*)p1 + p2)
				ZEND_VM_SET_RELATIVE_OPCODE(opline, opline->extended_value);
				// windows: return  0
				ZEND_VM_CONTINUE();
			}
		}
		// ȡ�õ�ǰ ֵ
		value = funcs->get_current_data(iter);
		// ������쳣
		if (UNEXPECTED(EG(exception) != NULL)) {
			// ��� opline ��������� ��������ʱ�������ѽ�� zval ���Ϊδ����
			UNDEF_RESULT();
			// ��Ҫ�� return
			HANDLE_EXCEPTION();
		}
		// ���û��ȡ ��ֵ
		if (!value) {
			/* failure in get_current_data */
			// ��ת��ָ����ǩ fe_fetch_r_exit
			ZEND_VM_C_GOTO(fe_fetch_r_exit);
		}
		// ��� ���������������Ч(����IS_UNUSED)
		if (RETURN_VALUE_USED(opline)) {
			// ����л�ȡkey�ķ���
			if (funcs->get_current_key) {
				// ȡ�õ�ǰkey�ŵ�result��
				funcs->get_current_key(iter, EX_VAR(opline->result.var));
				// ������쳣
				if (UNEXPECTED(EG(exception) != NULL)) {
					// ��� opline ��������� ��������ʱ�������ѽ�� zval ���Ϊδ����
					UNDEF_RESULT();
					// ��Ҫ�� return
					HANDLE_EXCEPTION();
				}
			// ����
			} else {
				// д����
				// ������ָ�� fe_reset/fe_fetch ��˽��ָ�루������
				ZVAL_LONG(EX_VAR(opline->result.var), iter->index);
			}
		}
		// ֵ������
		value_type = Z_TYPE_INFO_P(value);
	}

	// ��� op2 �� �������
	if (EXPECTED(OP2_TYPE == IS_CV)) {
		// ��ִ��������ȡ�� ָ����ŵı���
		zval *variable_ptr = EX_VAR(opline->op2.var);
		// ��������ֵ��p1:������p2:ֵ��p3:ֵ�ı������ͣ������������������������p4:�Ƿ��ϸ�����
		// ĳִ���������������� �Ĳ����Ƿ�ʹ���ϸ���������
		zend_assign_to_variable(variable_ptr, value, IS_CV, EX_USES_STRICT_TYPES());
	// ����
	} else {
		// ��ִ��������ȡ�� ָ����ŵı���
		zval *res = EX_VAR(opline->op2.var);
		// ȡ��������
		zend_refcounted *gc = Z_COUNTED_P(value);

		// ������
		ZVAL_COPY_VALUE_EX(res, value, gc, value_type);
		// �Ƿ��ǿ��Լ������������ͣ�����ɼ���
		if (Z_TYPE_INFO_REFCOUNTED(value_type)) {
			// �������ô���
			GC_ADDREF(gc);
		}
	}
	// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
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

	// ��ִ��������ȡ�� ָ����ŵı���
	array = EX_VAR(opline->op1.var);
	// ��� op1 �������� 
	if (UNEXPECTED(Z_TYPE_P(array) != IS_ARRAY)) {
		// ���÷��� zend_fe_fetch_object_helper
		ZEND_VM_DISPATCH_TO_HELPER(zend_fe_fetch_object_helper);
	}
	// ȡ�����ڱ����Ĺ�ϣ��
	fe_ht = Z_ARRVAL_P(array);
	// ��ϣ��ǰλ��
	// ͨ��ָ�����zval�� foreachλ�á�(*p1).u2.fe_pos
	pos = Z_FE_POS_P(array);
	// �����˳������
	if (HT_IS_PACKED(fe_ht)) {
		// ȡ��ָ���Ԫ��
		value = fe_ht->arPacked + pos;
		// ����������
		while (1) {
			// ���λ�ó�����Чλ��
			if (UNEXPECTED(pos >= fe_ht->nNumUsed)) {
				// ���������β
				/* reached end of iteration */
				// �ҵ�Ŀ������룬���óɵ�ǰ�����벢���ء� EX(opline) = (zend_op*)((char*)p1 + p2)
				ZEND_VM_SET_RELATIVE_OPCODE(opline, opline->extended_value);
				// windows: return  0
				ZEND_VM_CONTINUE();
			}
			// ȡ������
			value_type = Z_TYPE_INFO_P(value);
			// ���Ͳ������Ǽ������
			ZEND_ASSERT(value_type != IS_INDIRECT);
			// ���Ͳ��� δ����
			if (EXPECTED(value_type != IS_UNDEF)) {
				// ����ѭ��
				break;
			}
			// λ�ú���
			pos++;
			// Ԫ��ָ�����
			value++;
		}
		// ָ��ָ����һ��Ԫ��
		// ͨ��ָ�����zval�� foreachλ�á�(*p1).u2.fe_pos
		Z_FE_POS_P(array) = pos + 1;
		// ��� ���������������Ч(����IS_UNUSED)
		if (RETURN_VALUE_USED(opline)) {
			// ����ֵΪ����
			ZVAL_LONG(EX_VAR(opline->result.var), pos);
		}
	// ����
	} else {
		Bucket *p;
		// ��ǰ bucket
		p = fe_ht->arData + pos;
		// ����������Ԫ��
		while (1) {
			// ����Ѿ������β
			if (UNEXPECTED(pos >= fe_ht->nNumUsed)) {
				// ���������β
				/* reached end of iteration */
				// �ҵ�Ŀ������룬���óɵ�ǰ�����벢���ء� EX(opline) = (zend_op*)((char*)p1 + p2)
				ZEND_VM_SET_RELATIVE_OPCODE(opline, opline->extended_value);
				// windows: return  0
				ZEND_VM_CONTINUE();
			}
			// λ�ú���
			pos++;
			// ֵ
			value = &p->val;
			// ֵ������
			value_type = Z_TYPE_INFO_P(value);
			// ֵ�������Ǽ����������
			ZEND_ASSERT(value_type != IS_INDIRECT);
			// ���Ͳ��� δ����
			if (EXPECTED(value_type != IS_UNDEF)) {
				// ����
				break;
			}
			// ��һ��bucket
			p++;
		}
		// ͨ��ָ�����zval�� foreachλ�á�(*p1).u2.fe_pos
		Z_FE_POS_P(array) = pos;
		// ��� ���������������Ч(����IS_UNUSED)
		if (RETURN_VALUE_USED(opline)) {
			if (!p->key) {
				ZVAL_LONG(EX_VAR(opline->result.var), p->h);
			// ����
			} else {
				ZVAL_STR_COPY(EX_VAR(opline->result.var), p->key);
			}
		}
	}
	// ��� op2 �� �������
	if (EXPECTED(OP2_TYPE == IS_CV)) {
		// ��ִ��������ȡ�� ָ����ŵı���
		zval *variable_ptr = EX_VAR(opline->op2.var);
		// windows: �޲���
		SAVE_OPLINE();
		// ��������ֵ��p1:������p2:ֵ��p3:ֵ�ı������ͣ������������������������p4:�Ƿ��ϸ�����
		// ĳִ���������������� �Ĳ����Ƿ�ʹ���ϸ���������
		zend_assign_to_variable(variable_ptr, value, IS_CV, EX_USES_STRICT_TYPES());
		// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
		ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
	// ����
	} else {
		// ��ִ��������ȡ�� ָ����ŵı���
		zval *res = EX_VAR(opline->op2.var);
		// 
		zend_refcounted *gc = Z_COUNTED_P(value);
		// 
		ZVAL_COPY_VALUE_EX(res, value, gc, value_type);
		// 
		if (Z_TYPE_INFO_REFCOUNTED(value_type)) {
			// �������ô���
			GC_ADDREF(gc);
		}
		// opline + 1����Ŀ������벢 return
		ZEND_VM_NEXT_OPCODE();
	}
}

// ing2, ��д��ʽ���� ����
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

	// ��ִ��������ȡ�� ָ����ŵı���
	array = EX_VAR(opline->op1.var);
	// windows: �޲���
	SAVE_OPLINE();
	// �������ô���
	ZVAL_DEREF(array);
	// ���op1������
	if (EXPECTED(Z_TYPE_P(array) == IS_ARRAY)) {
		// ͨ��ָ�����zval�� ���������� , (*p1).u2.fe_iter_idx
		pos = zend_hash_iterator_pos_ex(Z_FE_ITER_P(EX_VAR(opline->op1.var)), array);
		// �����Ĺ�ϣ��
		fe_ht = Z_ARRVAL_P(array);
		// �����˳������
		if (HT_IS_PACKED(fe_ht)) {
			// �ҵ�Ԫ��λ��
			value = fe_ht->arPacked + pos;
			// ��ѭ��
			while (1) {
				// ���λ�ó������÷�Χ
				if (UNEXPECTED(pos >= fe_ht->nNumUsed)) {
					// �������ĩβ
					/* reached end of iteration */
					// ��ת��ָ����ǩ fe_fetch_w_exit
					ZEND_VM_C_GOTO(fe_fetch_w_exit);
				}
				// ֵ������
				value_type = Z_TYPE_INFO_P(value);
				// �������Ǽ������
				ZEND_ASSERT(value_type != IS_INDIRECT);
				// ���Ͳ��� δ����
				if (EXPECTED(value_type != IS_UNDEF)) {
					// ������
					break;
				}
				// ��һ��λ��
				pos++;
				// ��һ��Ԫ��
				value++;
			}
			// ͨ��ָ�����zval�� ���������� , (*p1).u2.fe_iter_idx
			EG(ht_iterators)[Z_FE_ITER_P(EX_VAR(opline->op1.var))].pos = pos + 1;
			// ��� ���������������Ч(����IS_UNUSED)
			if (RETURN_VALUE_USED(opline)) {
				// ��λ�ã���������ʽ�ŵ������
				ZVAL_LONG(EX_VAR(opline->result.var), pos);
			}
		// �����ǹ�ϣ��
		} else {
			// Ԫ��λ��
			p = fe_ht->arData + pos;
			// ��ѭ��
			while (1) {
				// ���λ�ó������÷�Χ
				if (UNEXPECTED(pos >= fe_ht->nNumUsed)) {
					// �������ĩβ
					/* reached end of iteration */
					// ��ת��ָ����ǩ fe_fetch_w_exit
					ZEND_VM_C_GOTO(fe_fetch_w_exit);
				}
				// ��һ��λ��
				pos++;
				// Ԫ��ֵ
				value = &p->val;
				// ����
				value_type = Z_TYPE_INFO_P(value);
				// ���Ͳ������Ǽ������
				ZEND_ASSERT(value_type != IS_INDIRECT);
				// ���Ͳ��� δ����
				if (EXPECTED(value_type != IS_UNDEF)) {
					// ����
					break;
				}
				// ��һ��Ԫ��
				p++;
			}
			// ͨ��ָ�����zval�� ���������� , (*p1).u2.fe_iter_idx
			EG(ht_iterators)[Z_FE_ITER_P(EX_VAR(opline->op1.var))].pos = pos;
			// ��� ���������������Ч(����IS_UNUSED)
			if (RETURN_VALUE_USED(opline)) {
				// ���û��key
				if (!p->key) {
					// ��ϣֵ���ɽ��
					ZVAL_LONG(EX_VAR(opline->result.var), p->h);
				// ������key
				} else {
					// key���Ƶ������
					ZVAL_STR_COPY(EX_VAR(opline->result.var), p->key);
				}
			}
		}
	// ��������Ƕ���
	} else if (EXPECTED(Z_TYPE_P(array) == IS_OBJECT)) {
		zend_object_iterator *iter;

		// ��ȡ zval�еĵ����������˸��жϣ������Ƿ��ǵ�����
		// �����ȡʧ��
		if ((iter = zend_iterator_unwrap(array)) == NULL) {
			// �򵥶���
			/* plain object */

			// ȡ�����Ա�
			fe_ht = Z_OBJPROP_P(array);
			// ͨ��ָ�����zval�� ���������� , (*p1).u2.fe_iter_idx
			pos = zend_hash_iterator_pos(Z_FE_ITER_P(EX_VAR(opline->op1.var)), fe_ht);
			// �ҵ�Ԫ��
			p = fe_ht->arData + pos;
			// ��ѭ��
			while (1) {
				// ���λ����Ч
				if (UNEXPECTED(pos >= fe_ht->nNumUsed)) {
					// �������ĩβ
					/* reached end of iteration */
					// ��ת��ָ����ǩ fe_fetch_w_exit
					ZEND_VM_C_GOTO(fe_fetch_w_exit);
				}
				// ��һ��λ��
				pos++;
				// ֵ
				value = &p->val;
				// ֵ������
				value_type = Z_TYPE_INFO_P(value);
				// ���Ͳ��� δ����
				if (EXPECTED(value_type != IS_UNDEF)) {
					// ���ֵ�����Ǽ������
					if (UNEXPECTED(value_type == IS_INDIRECT)) {
						// ��������
						value = Z_INDIRECT_P(value);
						// ֵ������
						value_type = Z_TYPE_INFO_P(value);
						// ���Ͳ��� δ���� ���� �ɷ���
						if (EXPECTED(value_type != IS_UNDEF)
						 && EXPECTED(zend_check_property_access(Z_OBJ_P(array), p->key, 0) == SUCCESS)) {
							// ������Ͳ�������
							if ((value_type & Z_TYPE_MASK) != IS_REFERENCE) {
								// ͨ��slot����Ҳ�����ԣ�ָ�����slot����ţ����ش���Ŷ�Ӧ��������Ϣ
								zend_property_info *prop_info =
									zend_get_property_info_for_slot(Z_OBJ_P(array), value);
								// �����������Ϣ
								if (UNEXPECTED(prop_info)) {
									// ��������� ֻ����
									if (UNEXPECTED(prop_info->flags & ZEND_ACC_READONLY)) {
										// ���쳣�������Է���ֻ�����Ե� ���� �����Թ�������ʱǰ�治�ܼ�&��
										zend_throw_error(NULL,
											"Cannot acquire reference to readonly property %s::$%s",
											ZSTR_VAL(prop_info->ce->name), ZSTR_VAL(p->key));
										// ��� opline ��������� ��������ʱ�������ѽ�� zval ���Ϊδ����
										UNDEF_RESULT();
										// ��Ҫ�� return
										HANDLE_EXCEPTION();
									}
									// ������ʹ���
									if (ZEND_TYPE_IS_SET(prop_info->type)) {
										// �� zval �����µ� zend_reference ���Ӹ����� zval �и��Ƹ�������
										ZVAL_NEW_REF(value, value);
										// ��zend_property_info_source_listָ��� zend_property_info �б������ zend_property_info
										ZEND_REF_ADD_TYPE_SOURCE(Z_REF_P(value), prop_info);
										// ����Ϊ����
										value_type = IS_REFERENCE_EX;
									}
								}
							}
							// ����
							break;
						}
					// ֵ�����Ͳ��Ǽ������ ���� ��Ĭ����������0 �� û��key �� ����ͨ��key���ʣ�
					} else if (EXPECTED(Z_OBJCE_P(array)->default_properties_count == 0)
							|| !p->key
							|| zend_check_property_access(Z_OBJ_P(array), p->key, 1) == SUCCESS) {
						// ����
						break;
					}
				}
				// ��һ��Ԫ��
				p++;
			}
			// ͨ��ָ�����zval�� ���������� , (*p1).u2.fe_iter_idx
			EG(ht_iterators)[Z_FE_ITER_P(EX_VAR(opline->op1.var))].pos = pos;
			// ��� ���������������Ч(����IS_UNUSED)
			if (RETURN_VALUE_USED(opline)) {
				// ���û��key
				if (UNEXPECTED(!p->key)) {
					// ���Ϊ��ϣֵ
					ZVAL_LONG(EX_VAR(opline->result.var), p->h);
				// ���key��Ч
				} else if (ZSTR_VAL(p->key)[0]) {
					// key ���Ƹ� ���
					ZVAL_STR_COPY(EX_VAR(opline->result.var), p->key);
				// ����
				} else {
					const char *class_name, *prop_name;
					size_t prop_name_len;
					// ƴ�� ������������
					zend_unmangle_property_name_ex(
						p->key, &class_name, &prop_name, &prop_name_len);
					// �������ŵ������
					ZVAL_STRINGL(EX_VAR(opline->result.var), prop_name, prop_name_len);
				}
			}
		// ����
		} else {
			// �����������б�
			const zend_object_iterator_funcs *funcs = iter->funcs;
			// ����������� 
			// ָ�� fe_reset/fe_fetch ��˽��ָ�루������
			if (++iter->index > 0) {
				/* This could cause an endless loop if index becomes zero again.
				 * In case that ever happens we need an additional flag. */
				// ������
				funcs->move_forward(iter);
				// ������쳣
				if (UNEXPECTED(EG(exception) != NULL)) {
					// ��� opline ��������� ��������ʱ�������ѽ�� zval ���Ϊδ����
					UNDEF_RESULT();
					// ��Ҫ�� return
					HANDLE_EXCEPTION();
				}
				// �����ǰλ����Ч
				if (UNEXPECTED(funcs->valid(iter) == FAILURE)) {
					// �������ĩβ
					/* reached end of iteration */
					// ������쳣
					if (UNEXPECTED(EG(exception) != NULL)) {
						// ��� opline ��������� ��������ʱ�������ѽ�� zval ���Ϊδ����
						UNDEF_RESULT();
						// ��Ҫ�� return
						HANDLE_EXCEPTION();
					}
					// ��ת��ָ����ǩ fe_fetch_w_exit
					ZEND_VM_C_GOTO(fe_fetch_w_exit);
				}
			}
			// ȡ����ǰֵ
			value = funcs->get_current_data(iter);
			// ������쳣
			if (UNEXPECTED(EG(exception) != NULL)) {
				// ��� opline ��������� ��������ʱ�������ѽ�� zval ���Ϊδ����
				UNDEF_RESULT();
				// ��Ҫ�� return
				HANDLE_EXCEPTION();
			}
			// ���û�е�ǰֵ
			if (!value) {
				// get_current_data ʱ������
				/* failure in get_current_data */
				// ��ת��ָ����ǩ fe_fetch_w_exit
				ZEND_VM_C_GOTO(fe_fetch_w_exit);
			}
			// ��� ���������������Ч(����IS_UNUSED)
			if (RETURN_VALUE_USED(opline)) {
				// ����л�ȡ��ǰkey�ķ���(����ԭ�ͣ�
				if (funcs->get_current_key) {
					// ��ȡ��ǰkey
					funcs->get_current_key(iter, EX_VAR(opline->result.var));
					// ������쳣
					if (UNEXPECTED(EG(exception) != NULL)) {
						// ��� opline ��������� ��������ʱ�������ѽ�� zval ���Ϊδ����
						UNDEF_RESULT();
						// ��Ҫ�� return
						HANDLE_EXCEPTION();
					}
				// ����
				} else {
					// ��ȡ ָ�� fe_reset/fe_fetch ��˽��ָ�루�������� ���Ƹ������
					ZVAL_LONG(EX_VAR(opline->result.var), iter->index);
				}
			}
			// ֵ������
			value_type = Z_TYPE_INFO_P(value);
		}
	// ����op1���Ƕ���
	} else {
		// ���棺foreach()�Ĳ�����������������
		zend_error(E_WARNING, "foreach() argument must be of type array|object, %s given", zend_zval_type_name(array));
		// ������쳣
		if (UNEXPECTED(EG(exception))) {
			// ��� opline ��������� ��������ʱ�������ѽ�� zval ���Ϊδ����
			UNDEF_RESULT();
			// ��Ҫ�� return
			HANDLE_EXCEPTION();
		}
// ������ת��ǩ fe_fetch_w_exit
ZEND_VM_C_LABEL(fe_fetch_w_exit):
		// �ҵ�Ŀ������룬���óɵ�ǰ�����벢���ء� EX(opline) = (zend_op*)((char*)p1 + p2)
		ZEND_VM_SET_RELATIVE_OPCODE(opline, opline->extended_value);
		// windows: return  0
		ZEND_VM_CONTINUE();
	}

	// ���ֵ ������������
	if (EXPECTED((value_type & Z_TYPE_MASK) != IS_REFERENCE)) {
		// ȡ�� gc����
		zend_refcounted *gc = Z_COUNTED_P(value);
		// ����ʵ��
		zval *ref;
		// �� zval �����µĿյ� zend_reference  
		ZVAL_NEW_EMPTY_REF(value);
		// ������
		ref = Z_REFVAL_P(value);
		// ??
		ZVAL_COPY_VALUE_EX(ref, value, gc, value_type);
	}
	// ��� op2 �� �������
	if (EXPECTED(OP2_TYPE == IS_CV)) {
		// ��ִ��������ȡ�� ָ����ŵı���
		zval *variable_ptr = EX_VAR(opline->op2.var);
		// op2��zval �� value����ͬһ��
		if (EXPECTED(variable_ptr != value)) {
			zend_reference *ref;
			// ȡ�� refʵ��
			ref = Z_REF_P(value);
			// �������ô���
			GC_ADDREF(ref);
			// ���  variable_ptr
			i_zval_ptr_dtor(variable_ptr);
			// �� ref �Żص� op2
			ZVAL_REF(variable_ptr, ref);
		}
	// ����
	} else {
		// ���ô���+1
		Z_ADDREF_P(value);
		// value������Ŀ�꣬���ø�ֵ��op2
		ZVAL_REF(EX_VAR(opline->op2.var), Z_REF_P(value));
	}
	// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, isset/empty����
ZEND_VM_HOT_HANDLER(154, ZEND_ISSET_ISEMPTY_CV, CV, UNUSED, ISSET, SPEC(ISSET))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *value;
	// ��ִ��������ȡ�� ָ����ŵı���
	value = EX_VAR(opline->op1.var);
	// ���û�� ZEND_ISEMPTY��˵���ڵ���isset����
	if (!(opline->extended_value & ZEND_ISEMPTY)) {
		// Ϊʲô����Ŀ�겻��Ҫ > IS_NULL �أ�
		// ���ֵ����null ���� �������������� �� ����Ŀ�겻�� null��
		if (Z_TYPE_P(value) > IS_NULL &&
		    (!Z_ISREF_P(value) || Z_TYPE_P(Z_REFVAL_P(value)) != IS_NULL)) {
			// ���ݵ�ǰ��������ѡ����һ�������롣���������ת���ͣ������ֵΪtrue��
			ZEND_VM_SMART_BRANCH_TRUE();
		// ����
		} else {
			// ���ݵ�ǰ��������ѡ����һ�������롣���������ת���ͣ������ֵΪfalse��
			ZEND_VM_SMART_BRANCH_FALSE();
		}
	// ����empty����
	} else {
		bool result;

		// windows: �޲���
		SAVE_OPLINE();
		// empty����Ҫ�ѽ��ȡ��
		result = !i_zend_is_true(value);
		// ���ݵ�ǰ��������ѡ����һ�������룬p1:����ֵ����Ҫ�������жϡ�p2:�Ƿ����쳣
		ZEND_VM_SMART_BRANCH(result, 1);
	}
}

// ing3, �� GLOBAL �� ��ǰ���������жϱ��� isset �� empty
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
	// ��ȡzvalָ�룬UNUSED ���ͷ���null
	varname = GET_OP1_ZVAL_PTR(BP_VAR_IS);
	// ���op1�ǳ���
	if (OP1_TYPE == IS_CONST) {
		// ������
		name = Z_STR_P(varname);
	// ����op1���ǳ���
	} else {
		// ȡ�� ������
		name = zval_get_tmp_string(varname, &tmp_name);
	}

	// �����ȡ�ķ��ű��� global ��ǰִ������ �������еı����б�
	// ȡ��Ŀ����ű�����ʱ���ű� �� ��ǰִ�������еķ��ű�
	target_symbol_table = zend_get_target_symbol_table(opline->extended_value EXECUTE_DATA_CC);
	// �ڷ��ű��в��ұ���
	value = zend_hash_find_ex(target_symbol_table, name, OP1_TYPE == IS_CONST);

	// ��� OP1 ���ǳ���
	if (OP1_TYPE != IS_CONST) {
		// �ͷ���ʱ����
		zend_tmp_string_release(tmp_name);
	}
	// �ͷŲ�������ĸ��ӱ���
	FREE_OP1();

	// ��� ���ű��� û�в��� ������
	if (!value) {
		// �����empty���������Ϊ1
		result = (opline->extended_value & ZEND_ISEMPTY);
	// �����ҵ��б���
	} else {
		// ����ҵ����Ǽ����������
		if (Z_TYPE_P(value) == IS_INDIRECT) {
			// ������
			value = Z_INDIRECT_P(value);
		}
		// ���û�� ZEND_ISEMPTY ��ǣ�isset�����ã�
		if (!(opline->extended_value & ZEND_ISEMPTY)) {
			// ��� ֵ����������
			if (Z_ISREF_P(value)) {
				// ������
				value = Z_REFVAL_P(value);
			}
			// ȡ�ص�ֵ�������� undef/null/false
			result = Z_TYPE_P(value) > IS_NULL;
		// ����� ZEND_ISEMPTY ��ǣ�empty�������ã�
		} else {
			// �ѽ��ȡ��
			result = !i_zend_is_true(value);
		}
	}
	// ���ݵ�ǰ��������ѡ����һ�������룬p1:����ֵ����Ҫ�������жϡ�p2:�Ƿ����쳣
	ZEND_VM_SMART_BRANCH(result, 1);
}

// ing3, ��̬���Ե� isset �� empty ����
/* No specialization for op_types (CONST|TMPVAR|CV, UNUSED|CLASS_FETCH|CONST|VAR) */
ZEND_VM_HANDLER(180, ZEND_ISSET_ISEMPTY_STATIC_PROP, ANY, CLASS_FETCH, ISSET|CACHE_SLOT)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *value;
	zend_result fetch_result;
	bool result;

	SAVE_OPLINE();

	// ͨ������λ�ú����ͣ����ؾ�̬���Ե�ַ��p1:��������ֵ��p2:����������Ϣ��p3:����λ�ã�p4:��ȡ���ͣ�p5:falgs
	fetch_result = zend_fetch_static_property_address(&value, NULL, opline->extended_value & ~ZEND_ISEMPTY, BP_VAR_IS, 0 OPLINE_CC EXECUTE_DATA_CC);

	// �������empty, ��isset
	if (!(opline->extended_value & ZEND_ISEMPTY)) {
		// ���Ԫ�ش��ڣ��������Ͳ��� null,undef ���ң�ֵ������������ �� ����Ŀ�겻��null��
			// ���Ϊ�� 
		result = fetch_result == SUCCESS && Z_TYPE_P(value) > IS_NULL &&
		    (!Z_ISREF_P(value) || Z_TYPE_P(Z_REFVAL_P(value)) != IS_NULL);
	// ����
	} else {
		// û��Ԫ�� �� ֵΪ�񡣽��Ϊ��
		result = fetch_result != SUCCESS || !i_zend_is_true(value);
	}
	// ���ݵ�ǰ��������ѡ����һ�������룬p1:����ֵ����Ҫ�������жϡ�p2:�Ƿ����쳣
	ZEND_VM_SMART_BRANCH(result, 1);
}

// ing3, �������Ե� isset �� empty ����
ZEND_VM_COLD_CONSTCONST_HANDLER(115, ZEND_ISSET_ISEMPTY_DIM_OBJ, CONST|TMPVAR|CV, CONST|TMPVAR|CV, ISSET)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *container;
	bool result;
	zend_ulong hval;
	zval *offset;

	// windows: �޲���
	SAVE_OPLINE();
	// ��ȡzvalָ��, UNUSED ���� $this
	container = GET_OP1_OBJ_ZVAL_PTR_UNDEF(BP_VAR_IS);
	// ��ȡzvalָ��, UNUSED ����null
	offset = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);

	// ��� ���� ������ ����
	if (EXPECTED(Z_TYPE_P(container) == IS_ARRAY)) {
		HashTable *ht;
		zval *value;
		zend_string *str;

// ������ת��ǩ isset_dim_obj_array
ZEND_VM_C_LABEL(isset_dim_obj_array):
		// ��������ȡ����ϣ��
		ht = Z_ARRVAL_P(container);
// ������ת��ǩ isset_again
ZEND_VM_C_LABEL(isset_again):
		// ���ƫ������ �ִ�
		if (EXPECTED(Z_TYPE_P(offset) == IS_STRING)) {
			// ȡ�� �ִ�
			str = Z_STR_P(offset);
			// ��� OP2 ���ǳ���
			if (OP2_TYPE != IS_CONST) {
				// ����ִ�ȫ������
				if (ZEND_HANDLE_NUMERIC(str, hval)) {
					// ��ת��ָ����ǩ num_index_prop
					ZEND_VM_C_GOTO(num_index_prop);
				}
			}
			// ��ϣ������ key ����Ԫ�ء�p1:��ϣ��p2:key��p3��key���Ƿ��й�ϣֵ��
			// op2�ǳ���ʱ�й�ϣֵ��
			value = zend_hash_find_ex(ht, str, OP2_TYPE == IS_CONST);
		// ���offset������
		} else if (EXPECTED(Z_TYPE_P(offset) == IS_LONG)) {
			// ȡ������
			hval = Z_LVAL_P(offset);
// ������ת��ǩ num_index_prop
ZEND_VM_C_LABEL(num_index_prop):
			// ����ϣֵ��ѯ������˳�����顣p1:��ϣ��p2:��ϣֵ
			value = zend_hash_index_find(ht, hval);
		// ���� ��� op2 ����ͨ������������ ���� op2 ����������
		} else if ((OP2_TYPE & (IS_VAR|IS_CV)) && EXPECTED(Z_ISREF_P(offset))) {
			// ������
			offset = Z_REFVAL_P(offset);
			// ��ת��ָ����ǩ isset_again
			ZEND_VM_C_GOTO(isset_again);
		// ����, �������
		} else {
			// ����������ά�Ȳ���
			value = zend_find_array_dim_slow(ht, offset EXECUTE_DATA_CC);
			// ������쳣
			if (UNEXPECTED(EG(exception))) {
				// ���Ϊ 0
				result = 0;
				// ��ת��ָ����ǩ isset_dim_obj_exit
				ZEND_VM_C_GOTO(isset_dim_obj_exit);
			}
		}

		// �������empty��������isset
		if (!(opline->extended_value & ZEND_ISEMPTY)) {
			// >IS_NULL ˵�� ���� IS_UNDEF �� IS_NULL
			/* > IS_NULL means not IS_UNDEF and not IS_NULL */
			// Ϊ���������ֵ����null�������ʹ���IS_NULL ���ң���������ã�����Ŀ�겻��null��
			result = value != NULL && Z_TYPE_P(value) > IS_NULL &&
			    (!Z_ISREF_P(value) || Z_TYPE_P(Z_REFVAL_P(value)) != IS_NULL);

			// ���op1�ǳ�����������
			if (OP1_TYPE & (IS_CONST|IS_CV)) {
				// �����Ч�쳣
				/* avoid exception check */
				// �ͷŲ�������ĸ��ӱ���
				FREE_OP2();
				// ���ݵ�ǰ��������ѡ����һ�������룬p1:����ֵ����Ҫ�������жϡ�p2:�Ƿ����쳣
				ZEND_VM_SMART_BRANCH(result, 0);
			}
		// ������empty����
		} else {
			// ����� null��false
			result = (value == NULL || !i_zend_is_true(value));
		}
		// ��ת��ָ����ǩ isset_dim_obj_exit
		ZEND_VM_C_GOTO(isset_dim_obj_exit);
	// ��� op1 ����ͨ������������ ���� op1����������
	} else if ((OP1_TYPE & (IS_VAR|IS_CV)) && EXPECTED(Z_ISREF_P(container))) {
		// ������
		container = Z_REFVAL_P(container);
		// ��� ���� ������ ����
		if (EXPECTED(Z_TYPE_P(container) == IS_ARRAY)) {
			// ��ת��ָ����ǩ isset_dim_obj_array
			ZEND_VM_C_GOTO(isset_dim_obj_array);
		}
	}

	// ���op2�ǳ��� ���� �ж���Ĳ���
	if (OP2_TYPE == IS_CONST && Z_EXTRA_P(offset) == ZEND_EXTRA_VALUE) {
		// ʹ����һ��zval��Ϊƫ����
		offset++;
	}
	// �����isset����
	if (!(opline->extended_value & ZEND_ISEMPTY)) {
		// ���� ���� �� �ִ� �� ��������Ӧ��ֵ�Ƿ� ����
		result = zend_isset_dim_slow(container, offset EXECUTE_DATA_CC);
	// ����empty����
	} else {
		// ���� ���� �� �ִ� �� ��������Ӧ��ֵ�Ƿ��� empty
		result = zend_isempty_dim_slow(container, offset EXECUTE_DATA_CC);
	}

// ������ת��ǩ isset_dim_obj_exit
ZEND_VM_C_LABEL(isset_dim_obj_exit):
	// �ͷŲ�������ĸ��ӱ���
	FREE_OP2();
	// �ͷŲ�������ĸ��ӱ���
	FREE_OP1();
	// ���ݵ�ǰ��������ѡ����һ�������룬p1:����ֵ����Ҫ�������жϡ�p2:�Ƿ����쳣
	ZEND_VM_SMART_BRANCH(result, 1);
}

// ing3, ��Զ������Ե� emtpy, isset
ZEND_VM_COLD_CONST_HANDLER(148, ZEND_ISSET_ISEMPTY_PROP_OBJ, CONST|TMPVAR|UNUSED|THIS|CV, CONST|TMPVAR|CV, ISSET|CACHE_SLOT)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *container;
	int result;
	zval *offset;
	zend_string *name, *tmp_name;

	// windows: �޲���
	SAVE_OPLINE();
	// ��ȡzvalָ��, UNUSED ���� $this����֧�� TMPVARCV
	container = GET_OP1_OBJ_ZVAL_PTR(BP_VAR_IS);
	// ��ȡzvalָ�룬UNUSED ���ͷ���null
	offset = GET_OP2_ZVAL_PTR(BP_VAR_R);

	// ���op1�ǳ��� �� ��op1����δʹ�� ���� �������Ƕ���
	if (OP1_TYPE == IS_CONST ||
	    (OP1_TYPE != IS_UNUSED && UNEXPECTED(Z_TYPE_P(container) != IS_OBJECT))) {
		// ��� op1 �ǣ����� �� ��������� ���� ��������������
		if ((OP1_TYPE & (IS_VAR|IS_CV)) && Z_ISREF_P(container)) {
			// ������
			container = Z_REFVAL_P(container);
			// ����������Ƕ���
			if (UNEXPECTED(Z_TYPE_P(container) != IS_OBJECT)) {
				// empty����true, isset����false
				result = (opline->extended_value & ZEND_ISEMPTY);
				// ��ת��ָ����ǩ isset_object_finish
				ZEND_VM_C_GOTO(isset_object_finish);
			}
		// ����
		} else {
			// empty����true, isset����false
			result = (opline->extended_value & ZEND_ISEMPTY);
			// ��ת��ָ����ǩ isset_object_finish
			ZEND_VM_C_GOTO(isset_object_finish);
		}
	}

	// �������Ϊ ����
	if (OP2_TYPE == IS_CONST) {
		// ȡ��������
		name = Z_STR_P(offset);
	// ����
	} else {
		// ��ȡ��ʱ�ִ������״�����
		name = zval_try_get_tmp_string(offset, &tmp_name);
		// ���������Ч
		if (UNEXPECTED(!name)) {
			// ���Ϊ��
			result = 0;
			// ��ת��ָ����ǩ isset_object_finish
			ZEND_VM_C_GOTO(isset_object_finish);
		}
	}

	// ��empty������ �ֻ� �������������
		// p1:����
		// p2:������
		// p3:�Ƿ�ʹ��empty
		// p4:����λ��: op2�ǳ��� �� ����λ�� ��null
	result =
		(opline->extended_value & ZEND_ISEMPTY) ^
		Z_OBJ_HT_P(container)->has_property(Z_OBJ_P(container), name, (opline->extended_value & ZEND_ISEMPTY), ((OP2_TYPE == IS_CONST) ? CACHE_ADDR(opline->extended_value & ~ZEND_ISEMPTY) : NULL));

	// ��� OP2 ���ǳ���
	if (OP2_TYPE != IS_CONST) {
		// �ͷ���ʱ����
		zend_tmp_string_release(tmp_name);
	}

// ������ת��ǩ isset_object_finish
ZEND_VM_C_LABEL(isset_object_finish):
	// �ͷŲ�������ĸ��ӱ���
	FREE_OP2();
	// �ͷŲ�������ĸ��ӱ���
	FREE_OP1();
	// ���ݵ�ǰ��������ѡ����һ�������룬p1:����ֵ����Ҫ�������жϡ�p2:�Ƿ����쳣
	ZEND_VM_SMART_BRANCH(result, 1);
}

// ing3, ����ϣ����key�Ƿ���� array_key_exists
ZEND_VM_HANDLER(194, ZEND_ARRAY_KEY_EXISTS, CV|TMPVAR|CONST, CV|TMPVAR|CONST)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	zval *key, *subject;
	HashTable *ht;
	bool result;

	// windows: �޲���
	SAVE_OPLINE();

	// ��ȡzvalָ��, UNUSED ����null
	key = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	
	// ��ȡzvalָ��, UNUSED ����null
	subject = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ��������� ����
	if (EXPECTED(Z_TYPE_P(subject) == IS_ARRAY)) {
// ������ת��ǩ array_key_exists_array
ZEND_VM_C_LABEL(array_key_exists_array):
		// ȡ������
		ht = Z_ARRVAL_P(subject);
		// ����ϣ�����Ƿ�����������p1:��ϣ��p2:key
		result = zend_array_key_exists_fast(ht, key OPLINE_CC EXECUTE_DATA_CC);
	// ����
	} else {
		// ���� ��� op2 ����ͨ������������ ���� op2 ����������
		if ((OP2_TYPE & (IS_VAR|IS_CV)) && EXPECTED(Z_ISREF_P(subject))) {
			// ������
			subject = Z_REFVAL_P(subject);
			// ��������� ����
			if (EXPECTED(Z_TYPE_P(subject) == IS_ARRAY)) {
				// ��ת��ָ����ǩ array_key_exists_array
				ZEND_VM_C_GOTO(array_key_exists_array);
			}
		}
		// �������飬Ҳ�����������͡�������Ŀ�겻�����顣
		// ����array_key_exists ��������
		zend_array_key_exists_error(subject, key OPLINE_CC EXECUTE_DATA_CC);
		// ���Ϊ ��
		result = 0;
	}

	// �ͷŲ�������ĸ��ӱ���
	FREE_OP2();
	// �ͷŲ�������ĸ��ӱ���
	FREE_OP1();
	// ���ݵ�ǰ��������ѡ����һ�������룬p1:����ֵ����Ҫ�������жϡ�p2:�Ƿ����쳣
	ZEND_VM_SMART_BRANCH(result, 1);
}

/* No specialization for op_types (CONST|TMPVAR|UNUSED|CV, ANY) */
// ing3, �˳�
ZEND_VM_COLD_HANDLER(79, ZEND_EXIT, ANY, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// windows: �޲���
	SAVE_OPLINE();
	// ��� op1 ���Ͳ��� δʹ��
	if (OP1_TYPE != IS_UNUSED) {
		// ��ȡzvalָ�룬UNUSED ���ͷ���null
		zval *ptr = GET_OP1_ZVAL_PTR(BP_VAR_R);
		// Ϊ��break
		do {
			// ���op1��ֵ������
			if (Z_TYPE_P(ptr) == IS_LONG) {
				// ���� �˳�״̬
				EG(exit_status) = Z_LVAL_P(ptr);
			// ����op1��������
			} else {
				// ��� op1 ������ IS_VAR �� IS_CV ���� op1����������
				if ((OP1_TYPE & (IS_VAR|IS_CV)) && Z_ISREF_P(ptr)) {
					// ������
					ptr = Z_REFVAL_P(ptr);
					// ���Ŀ��ֵ������ 
					if (Z_TYPE_P(ptr) == IS_LONG) {
						// ���� �˳�״̬
						EG(exit_status) = Z_LVAL_P(ptr);
						// ������ӡ
						break;
					}
				}
				// ���op1�������� ����op1��ӡ���� 
				zend_print_zval(ptr, 0);
			}
		} while (0);
		// �ͷŲ�������ĸ��ӱ���
		FREE_OP1();
	}

	// ���û���쳣 
	if (!EG(exception)) {
		// �׳� unwind_exit �쳣
		zend_throw_unwind_exit();
	}
	// ��Ҫ�� return
	HANDLE_EXCEPTION();
}

// ing3, ��ʼ��Ĭ
ZEND_VM_HANDLER(57, ZEND_BEGIN_SILENCE, ANY, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// ������ EG(error_reporting) ��������
	ZVAL_LONG(EX_VAR(opline->result.var), EG(error_reporting));

	// ������� ֻ�б���
	if (!E_HAS_ONLY_FATAL_ERRORS(EG(error_reporting))) {
		// Ϊ��break
		do {
			// �����Խ�����ʹ����
			/* Do not silence fatal errors */
			// ���ԭ�������� ��ʹ���󣬱��ֿ���
			EG(error_reporting) &= E_FATAL_ERRORS;
			// ���û���� EG(error_reporting_ini_entry)
			if (!EG(error_reporting_ini_entry)) {
				// ��ini���ñ��� ���� error_reporting
				zval *zv = zend_hash_find_known_hash(EG(ini_directives), ZSTR_KNOWN(ZEND_STR_ERROR_REPORTING));
				// ��������ֵ
				if (zv) {
					// ���ҵ���ֵд�� EG(error_reporting_ini_entry)
					EG(error_reporting_ini_entry) = (zend_ini_entry *)Z_PTR_P(zv);
				// ����û�����ֵ
				} else {
					// ����
					break;
				}
			}
			// ��� EG(error_reporting_ini_entry)->modified û���޸Ĺ�
			if (!EG(error_reporting_ini_entry)->modified) {
				// ���û������ EG(modified_ini_directives)
				if (!EG(modified_ini_directives)) {
					// ������ϣ�� EG(modified_ini_directives)
					ALLOC_HASHTABLE(EG(modified_ini_directives));
					// ��ʼ����ϣ�� ��8��Ԫ��
					zend_hash_init(EG(modified_ini_directives), 8, NULL, NULL, 0);
				}
				// EG(modified_ini_directives) ��ϣ������� error_reporting ������ɹ�
				if (EXPECTED(zend_hash_add_ptr(EG(modified_ini_directives), ZSTR_KNOWN(ZEND_STR_ERROR_REPORTING), EG(error_reporting_ini_entry)) != NULL)) {
					// ����ԭʼֵ
					EG(error_reporting_ini_entry)->orig_value = EG(error_reporting_ini_entry)->value;
					// ԭʼ�ɸ���״̬
					EG(error_reporting_ini_entry)->orig_modifiable = EG(error_reporting_ini_entry)->modifiable;
					// ��ǳ��Ѹ���
					EG(error_reporting_ini_entry)->modified = 1;
				}
			}
		} while (0);
	}
	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, ������Ĭ
ZEND_VM_HANDLER(58, ZEND_END_SILENCE, TMP, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	// ��� ������ �������� ���� ԭֵ���ǽ����� ��������
	if (E_HAS_ONLY_FATAL_ERRORS(EG(error_reporting))
			&& !E_HAS_ONLY_FATAL_ERRORS(Z_LVAL_P(EX_VAR(opline->op1.var)))) {
		// �ָ�ԭʼֵ
		EG(error_reporting) = Z_LVAL_P(EX_VAR(opline->op1.var));
	}
	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, ����쳣����ת
ZEND_VM_COLD_CONST_HANDLER(152, ZEND_JMP_SET, CONST|TMP|VAR|CV, JMP_ADDR)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *value;
	zend_reference *ref = NULL;
	bool ret;

	// windows: �޲���
	SAVE_OPLINE();
	// ��ȡzvalָ�룬UNUSED ���ͷ���null
	value = GET_OP1_ZVAL_PTR(BP_VAR_R);

	// ��� op1 �� ��ͨ���� �� ������� ���� ֵΪ��������
	if ((OP1_TYPE == IS_VAR || OP1_TYPE == IS_CV) && Z_ISREF_P(value)) {
		// ��� op1 �� ��ͨ����
		if (OP1_TYPE == IS_VAR) {
			// ȡ������ʵ��
			ref = Z_REF_P(value);
		}
		// ������
		value = Z_REFVAL_P(value);
	}

	// ֵת�� ������
	ret = i_zend_is_true(value);

	// ������쳣
	if (UNEXPECTED(EG(exception))) {
		// �ͷŲ�������ĸ��ӱ���
		FREE_OP1();
		// ���Ϊ δ����
		ZVAL_UNDEF(EX_VAR(opline->result.var));
		// ��Ҫ�� return
		HANDLE_EXCEPTION();
	}

	// ���ֵ��Ч
	if (ret) {
		// ��ִ��������ȡ�� ָ����ŵı���
		zval *result = EX_VAR(opline->result.var);
		// ֵ �ŵ������
		ZVAL_COPY_VALUE(result, value);
		// ���op1�ǳ���
		if (OP1_TYPE == IS_CONST) {
			// ����ǿ��Լ�������
			// ���ô���+1
			if (UNEXPECTED(Z_OPT_REFCOUNTED_P(result))) Z_ADDREF_P(result);
		// ���� ��� op1 �� �������
		} else if (OP1_TYPE == IS_CV) {
			// ����ǿ��Լ�������
			// ���ô���+1
			if (Z_OPT_REFCOUNTED_P(result)) Z_ADDREF_P(result);
		// ���� ���op1�Ǳ��� ���� ������ʵ��
		} else if (OP1_TYPE == IS_VAR && ref) {
			// ���ô���-1�����Ϊ0
			if (UNEXPECTED(GC_DELREF(ref) == 0)) {
				// �ͷ�����ʵ��
				efree_size(ref, sizeof(zend_reference));
			// ���� ����ǿ��Լ�������
			} else if (Z_OPT_REFCOUNTED_P(result)) {
				// ���ô���+1
				Z_ADDREF_P(result);
			}
		}
		// Ҫ����쳣 �� ���쳣ʱʹ�þɲ����룬���쳣ʹ���²�����, p1:�²����룬p2:�Ƿ����쳣
		// OP_JMP_ADDR: ���� p2.jmp_addr
		ZEND_VM_JMP_EX(OP_JMP_ADDR(opline, opline->op2), 0);
	}

	// �ͷŲ�������ĸ��ӱ���
	FREE_OP1();
	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, "??" ���ӵ��������ʽ��Ĭ��ֵ��
ZEND_VM_COLD_CONST_HANDLER(169, ZEND_COALESCE, CONST|TMP|VAR|CV, JMP_ADDR)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *value;
	zend_reference *ref = NULL;

	// windows: �޲���
	SAVE_OPLINE();
	// ��ȡzvalָ�룬UNUSED ���ͷ���null
	value = GET_OP1_ZVAL_PTR(BP_VAR_IS);

	// op1 �Ǳ����������� ���� ֵ����������
	if ((OP1_TYPE & (IS_VAR|IS_CV)) && Z_ISREF_P(value)) {
		// ��� op1����ͨ����
		if (OP1_TYPE & IS_VAR) {
			// ȡ������ʵ��
			ref = Z_REF_P(value);
		}
		// ������
		value = Z_REFVAL_P(value);
	}

	// ���ֵ���� undef �� null
	if (Z_TYPE_P(value) > IS_NULL) {
		// ��ִ��������ȡ�� ָ����ŵı���
		zval *result = EX_VAR(opline->result.var);
		// ��ֵ���Ƹ� �������� 
		ZVAL_COPY_VALUE(result, value);
		// ���op1�ǳ���
		if (OP1_TYPE == IS_CONST) {
			// ����ǿ��Լ�������
			// ���ô���+1
			if (UNEXPECTED(Z_OPT_REFCOUNTED_P(result))) Z_ADDREF_P(result);
		// ���� ��� op1 �� �������
		} else if (OP1_TYPE == IS_CV) {
			// ����ǿ��Լ�������
			// ���ô���+1
			if (Z_OPT_REFCOUNTED_P(result)) Z_ADDREF_P(result);
		// ���� ���op1����ͨ���� ���� ������ʵ��
		} else if ((OP1_TYPE & IS_VAR) && ref) {
			// ���ô���-1�����Ϊ0
			if (UNEXPECTED(GC_DELREF(ref) == 0)) {
				// �ͷ�����ʵ��
				efree_size(ref, sizeof(zend_reference));
			// ���� ����ǿ��Լ�������
			} else if (Z_OPT_REFCOUNTED_P(result)) {
				// ���ô���+1
				Z_ADDREF_P(result);
			}
		}
		// Ҫ����쳣 �� ���쳣ʱʹ�þɲ����룬���쳣ʹ���²�����, p1:�²����룬p2:�Ƿ����쳣
		// OP_JMP_ADDR: ���� p2.jmp_addr
		ZEND_VM_JMP_EX(OP_JMP_ADDR(opline, opline->op2), 0);
	}

	// ���op1����ͨ���� ���� ������ʵ��
	if ((OP1_TYPE & IS_VAR) && ref) {
		// ���ô���-1�����Ϊ0
		if (UNEXPECTED(GC_DELREF(ref) == 0)) {
			// �ͷ�����ʵ��
			efree_size(ref, sizeof(zend_reference));
		}
	}
	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, ��֤op1������nullʱ������·, ������ p2ָ����λ��
ZEND_VM_HOT_NOCONST_HANDLER(198, ZEND_JMP_NULL, CONST|TMP|VAR|CV, JMP_ADDR)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *val, *result;

	// ��ȡzvalָ��, UNUSED ����null
	val = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ��� op1 ���Ͳ��� null,undef
	if (Z_TYPE_P(val) > IS_NULL) {
		// Ϊ��break
		do {
			// ��� op1 �� ������� �� ��ͨ���� ���� ֵ����������
			if ((OP1_TYPE == IS_CV || OP1_TYPE == IS_VAR) && Z_TYPE_P(val) == IS_REFERENCE) {
				// ������
				val = Z_REFVAL_P(val);
				// ���ֵΪ null,undef
				if (Z_TYPE_P(val) <= IS_NULL) {
					// �ͷŲ�������ĸ��ӱ���
					FREE_OP1();
					// ����
					break;
				}
			}
			// ��������˵��û�г��ֶ�·
			// opline + 1����Ŀ������벢 return
			ZEND_VM_NEXT_OPCODE();
		} while (0);
	}
	// ��ִ��������ȡ�� ָ����ŵı���
	result = EX_VAR(opline->result.var);
	// ��·����
	uint32_t short_circuiting_type = opline->extended_value & ZEND_SHORT_CIRCUITING_CHAIN_MASK;
	// �������Ϊ ��·�����ʽ
	if (EXPECTED(short_circuiting_type == ZEND_SHORT_CIRCUITING_CHAIN_EXPR)) {
		// ���Ϊnull
		ZVAL_NULL(result);
		// ��� op1 �� ������� ���� ֵΪδ���� ���� ��չֵ��û�� ��ZEND_JMP_NULL_BP_VAR_IS�� 
		if (OP1_TYPE == IS_CV 
			&& UNEXPECTED(Z_TYPE_P(val) == IS_UNDEF)
			&& (opline->extended_value & ZEND_JMP_NULL_BP_VAR_IS) == 0
		) {
			// windows: �޲���
			SAVE_OPLINE();
			// ����p1�ı�������������δ����, ����δ��ʼ��zval
			ZVAL_UNDEFINED_OP1();
			// ������쳣
			if (UNEXPECTED(EG(exception) != NULL)) {
				// ��Ҫ�� return
				HANDLE_EXCEPTION();
			}
		}
	// �������Ϊ ��·��isset
	} else if (short_circuiting_type == ZEND_SHORT_CIRCUITING_CHAIN_ISSET) {
		// ���Ϊ��
		ZVAL_FALSE(result);
	// ����, ����Ϊ ��·��empty
	} else {
		// Ҳ����empty������֤��·���ʽ
		ZEND_ASSERT(short_circuiting_type == ZEND_SHORT_CIRCUITING_CHAIN_EMPTY);
		// ���Ϊ��
		ZVAL_TRUE(result);
	}

	// Ҫ����쳣 �� ���쳣ʱʹ�þɲ����룬���쳣ʹ���²�����, p1:�²����룬p2:�Ƿ����쳣
	// OP_JMP_ADDR: ���� p2.jmp_addr
	ZEND_VM_JMP_EX(OP_JMP_ADDR(opline, opline->op2), 0);
}

// ing3, op1���Ƹ�result
ZEND_VM_HOT_HANDLER(31, ZEND_QM_ASSIGN, CONST|TMP|VAR|CV, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *value;
	// ��ִ��������ȡ�� ָ����ŵı���
	zval *result = EX_VAR(opline->result.var);

	// ��ȡzvalָ��, UNUSED ����null
	value = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ��� op1 �� ������� ����ֵΪ δ����
	if (OP1_TYPE == IS_CV && UNEXPECTED(Z_TYPE_P(value) == IS_UNDEF)) {
		// windows: �޲���
		SAVE_OPLINE();
		// ����p1�ı�������������δ����, ����δ��ʼ��zval
		ZVAL_UNDEFINED_OP1();
		// ���Ϊnull
		ZVAL_NULL(result);
		// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
		ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
	}

	// ��� op1 �� �������
	if (OP1_TYPE == IS_CV) {
		// ��������Ŀ�� �� ������Ϣ�����������ü���
		ZVAL_COPY_DEREF(result, value);
	// ���� ��� op1 �� ��ͨ����
	} else if (OP1_TYPE == IS_VAR) {
		// ���op1����������
		if (UNEXPECTED(Z_ISREF_P(value))) {
			// op1����Ŀ�긴�Ƹ�result
			ZVAL_COPY_VALUE(result, Z_REFVAL_P(value));
			// ���Ŀ�� ���ô���-1��Ϊ0
			if (UNEXPECTED(Z_DELREF_P(value) == 0)) {
				// �ͷ�����ʵ��
				efree_size(Z_REF_P(value), sizeof(zend_reference));
			// ���� ����ǿ��Լ�������
			} else if (Z_OPT_REFCOUNTED_P(result)) {
				// ���ô���+1
				Z_ADDREF_P(result);
			}
		// ����op1������������
		} else {
			// op1�ĸ��Ƹ�result
			ZVAL_COPY_VALUE(result, value);
		}
	// ����
	} else {
		// op1���Ƹ�result
		ZVAL_COPY_VALUE(result, value);
		// ���op1�ǳ���
		if (OP1_TYPE == IS_CONST) {
			// ����ǿ��Լ�������
			if (UNEXPECTED(Z_OPT_REFCOUNTED_P(result))) {
				// ���ô���+1
				Z_ADDREF_P(result);
			}
		}
	}
	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, ������չ������䣬���Ӳ��� execute_data
ZEND_VM_COLD_HANDLER(101, ZEND_EXT_STMT, ANY, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	// ���û�н�����չ
	if (!EG(no_extensions)) {
		// windows: �޲���
		SAVE_OPLINE();
		// ��ÿ����չ���� zend_extension_statement_handler�����Ӳ��� execute_data
		zend_llist_apply_with_argument(&zend_extensions, (llist_apply_with_arg_func_t) zend_extension_statement_handler, execute_data);
		// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
		ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
	}
	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, ��ÿ����չ���� zend_extension_fcall_begin_handler
ZEND_VM_COLD_HANDLER(102, ZEND_EXT_FCALL_BEGIN, ANY, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// ���û�н�����չ
	if (!EG(no_extensions)) {
		// windows: �޲���
		SAVE_OPLINE();
		// ��ÿ����չ���� zend_extension_fcall_begin_handler, ���Ӳ����� execute_data
		zend_llist_apply_with_argument(&zend_extensions, (llist_apply_with_arg_func_t) zend_extension_fcall_begin_handler, execute_data);
		// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
		ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
	}
	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, ��ÿ����չ���� zend_extension_fcall_end_handler
ZEND_VM_COLD_HANDLER(103, ZEND_EXT_FCALL_END, ANY, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// ���û�н��� ��չ
	if (!EG(no_extensions)) {
		// windows: �޲���
		SAVE_OPLINE();
		// ��ÿ����չ���� zend_extension_fcall_end_handler�����Ӳ����� execute_data
		zend_llist_apply_with_argument(&zend_extensions, (llist_apply_with_arg_func_t) zend_extension_fcall_end_handler, execute_data);
		// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
		ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
	}
	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, �����࣬���Ѿ��������ˣ�������Ҫ������
ZEND_VM_HANDLER(144, ZEND_DECLARE_CLASS, CONST, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// windows: �޲���
	SAVE_OPLINE();
	
	// �󶨸��ࡣp1:��������p2:������
	// op1 ������op2������
	// ���� (p2).zv��p1:opline,p2:node
	do_bind_class(RT_CONSTANT(opline, opline->op1), (OP2_TYPE == IS_CONST) ? Z_STR_P(RT_CONSTANT(opline, opline->op2)) : NULL);
	// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, ��ʱ������
ZEND_VM_HANDLER(145, ZEND_DECLARE_CLASS_DELAYED, CONST, CONST)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// ͨ��ƫ���� ������ʱ�����л�ȡһ�� (void**) �еĵ�һ��Ԫ��
	zend_class_entry *ce = CACHED_PTR(opline->extended_value);
	// ��� �಻����
	if (ce == NULL) {
		// op1��Сд������
		// ���� (p2).zv��p1:opline,p2:node
		zval *lcname = RT_CONSTANT(opline, opline->op1);
		// �ú���������һ�� zval����������
		zval *zv = zend_hash_find_known_hash(EG(class_table), Z_STR_P(lcname + 1));
		// ����ҵ���
		if (zv) {
			// windows: �޲���
			SAVE_OPLINE();
			// �����࣬����ӵ�����У�p1:�� zval��p2:������p3:������
			// ���� (p2).zv��p1:opline,p2:node
			ce = zend_bind_class_in_slot(zv, lcname, Z_STR_P(RT_CONSTANT(opline, opline->op2)));
			// ���û�з�����
			if (!ce) {
				// ��Ҫ�� return
				HANDLE_EXCEPTION();
			}
		}
		// ͨ��ƫ���� �� EX(run_time_cache) �л�ȡһ�� (void**) �еĵ�һ��Ԫ�أ���������ֵ
		CACHE_PTR(opline->extended_value, ce);
	}
	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, ���������ࡣ���Ѿ��������ˣ�������Ҫ������
ZEND_VM_HANDLER(146, ZEND_DECLARE_ANON_CLASS, ANY, ANY, CACHE_SLOT)
{
	zval *zv;
	zend_class_entry *ce;
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// ͨ��ƫ���� ������ʱ�����л�ȡһ�� (void**) �еĵ�һ��Ԫ��
	ce = CACHED_PTR(opline->extended_value);
	// ��� �಻����
	if (UNEXPECTED(ce == NULL)) {
		// ȡ��Ψһ���
		// ���� (p2).zv��p1:opline,p2:node
		zend_string *rtd_key = Z_STR_P(RT_CONSTANT(opline, opline->op1));
		// ͨ��Ψһ�������������
		zv = zend_hash_find_known_hash(EG(class_table), rtd_key);
		// ����Ҫ�ҵ�
		ZEND_ASSERT(zv != NULL);
		// ȡ�������
		ce = Z_CE_P(zv);
		// �����û�����ӹ�
		if (!(ce->ce_flags & ZEND_ACC_LINKED)) {
			// windows: �޲���
			SAVE_OPLINE();
			// ������ : zend_inheritance.c
			// ���� (p2).zv��p1:opline,p2:node
			ce = zend_do_link_class(ce, (OP2_TYPE == IS_CONST) ? Z_STR_P(RT_CONSTANT(opline, opline->op2)) : NULL, rtd_key);
			// �������ʧ�ܣ�û�з�����
			if (!ce) {
				// ��Ҫ�� return
				HANDLE_EXCEPTION();
			}
		}
		// ͨ��ƫ���� �� EX(run_time_cache) �л�ȡһ�� (void**) �еĵ�һ��Ԫ�أ���������ֵ
		CACHE_PTR(opline->extended_value, ce);
	}
	// ȡ�������
	Z_CE_P(EX_VAR(opline->result.var)) = ce;
	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, ��ָ����ŵĺ�����ӵ������б�op2:ָ����ţ�p1:������
ZEND_VM_HANDLER(141, ZEND_DECLARE_FUNCTION, ANY, NUM)
{
	zend_function *func;
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// windows: �޲���
	SAVE_OPLINE();
	// �ҵ�ָ����ŵĺ���
	func = (zend_function *) EX(func)->op_array.dynamic_func_defs[opline->op2.num];
	// �Ѻ�����ӵ������б��У�p1:������p2:������
	// ���� (p2).zv��p1:opline,p2:node
	do_bind_function(func, RT_CONSTANT(opline, opline->op1));
	// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing2, ���� declare(...) ���涨��� ticks
ZEND_VM_HANDLER(105, ZEND_TICKS, ANY, ANY, NUM)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// ��� ticks �������� �������м�¼������ 
	if ((uint32_t)++EG(ticks_count) >= opline->extended_value) {
		// ������0
		EG(ticks_count) = 0;
		// ����� zend_ticks_function -> php_run_ticks : /main/php_ticks.c
		if (zend_ticks_function) {
			// windows: �޲���
			SAVE_OPLINE();
			// ���� fiber
			zend_fiber_switch_block();
			// php_run_ticks: main/ticks.c
			zend_ticks_function(opline->extended_value);
			// ���� fiber 
			zend_fiber_switch_unblock();
			// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
			ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
		}
	}
	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, ��������������
ZEND_VM_HANDLER(138, ZEND_INSTANCEOF, TMPVAR|CV, UNUSED|CLASS_FETCH|CONST|VAR, CACHE_SLOT)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *expr;
	bool result;

	// windows: �޲���
	SAVE_OPLINE();
	// ��ȡzvalָ��, UNUSED ����null
	expr = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);

// ������ת��ǩ try_instanceof
ZEND_VM_C_LABEL(try_instanceof):
	// ��� op1 �Ƕ���
	if (Z_TYPE_P(expr) == IS_OBJECT) {
		zend_class_entry *ce;

		// �������Ϊ ����
		if (OP2_TYPE == IS_CONST) {
			// ͨ��ƫ���� ������ʱ�����л�ȡһ�� (void**) �еĵ�һ��Ԫ��
			ce = CACHED_PTR(opline->extended_value);
			// ��� �಻����
			if (UNEXPECTED(ce == NULL)) {
				// �����ࡣp1:������p2:key��p3:flags
				// ���� (p2).zv��p1:opline,p2:node
				ce = zend_lookup_class_ex(Z_STR_P(RT_CONSTANT(opline, opline->op2)), Z_STR_P(RT_CONSTANT(opline, opline->op2) + 1), ZEND_FETCH_CLASS_NO_AUTOLOAD);
				// ����ҵ���
				if (EXPECTED(ce)) {
					// ͨ��ƫ���� �� EX(run_time_cache) �л�ȡһ�� (void**) �еĵ�һ��Ԫ�أ���������ֵ
					CACHE_PTR(opline->extended_value, ce);
				}
			}
		// ���� ��� op2 ������δ����
		} else if (OP2_TYPE == IS_UNUSED) {
			// ͨ�����������Ͳ����࣬p1:������p2:����
			ce = zend_fetch_class(NULL, opline->op2.num);
			// ��� �಻����
			if (UNEXPECTED(ce == NULL)) {
				// �ͷŲ�������ĸ��ӱ���
				FREE_OP1();
				// ���Ϊ δ����
				ZVAL_UNDEF(EX_VAR(opline->result.var));
				// ��Ҫ�� return
				HANDLE_EXCEPTION();
			}
		// ����
		} else {
			// ȡ�������
			ce = Z_CE_P(EX_VAR(opline->op2.var));
		}
		// ����Ч ���� op1 ���� ���ࡣΪ�档
		result = ce && instanceof_function(Z_OBJCE_P(expr), ce);
	// ���op1���Ƕ��� op1����ͨ���� �� ������� ���� ����������
	} else if ((OP1_TYPE & (IS_VAR|IS_CV)) && Z_TYPE_P(expr) == IS_REFERENCE) {
		// ������
		expr = Z_REFVAL_P(expr);
		// ��ת��ָ����ǩ try_instanceof
		ZEND_VM_C_GOTO(try_instanceof);
	// ����
	} else {
		// ��� op1 �� ������� ���� ���ʽ�� δ����
		if (OP1_TYPE == IS_CV && UNEXPECTED(Z_TYPE_P(expr) == IS_UNDEF)) {
			// ����p1�ı�������������δ����, ����δ��ʼ��zval
			ZVAL_UNDEFINED_OP1();
		}
		// ���Ϊ��
		result = 0;
	}
	// �ͷŲ�������ĸ��ӱ���
	FREE_OP1();
	// ���ݵ�ǰ��������ѡ����һ�������룬p1:����ֵ����Ҫ�������жϡ�p2:�Ƿ����쳣
	ZEND_VM_SMART_BRANCH(result, 1);
}


// ing3, ��ҵ���߼��� 1/1
// ZEND_FASTCALL ZEND_EXT_NOP_SPEC��1����֧����֤����
ZEND_VM_HOT_HANDLER(104, ZEND_EXT_NOP, ANY, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, ��ҵ���߼��� 1/1
// ZEND_FASTCALL ZEND_NOP_SPEC��1����֧����֤����
ZEND_VM_HOT_HANDLER(0, ZEND_NOP, ANY, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, �ַ� try/catch/finally ����
ZEND_VM_HELPER(zend_dispatch_try_catch_finally_helper, ANY, ANY, uint32_t try_catch_offset, uint32_t op_num)
{
	// �����ɹ����п�����NULL��ֻ��finally�鱻ִ�У�
	/* May be NULL during generator closing (only finally blocks are executed) */
	// ��ǰ�쳣
	zend_object *ex = EG(exception);

	// ������� try/catch/finally �ṹ�����б�Ҫ�Ĳ���
	/* Walk try/catch/finally structures upwards, performing the necessary actions */
	// ���ƫ�������� -1��ƫ����-1
	for (; try_catch_offset != (uint32_t) -1; try_catch_offset--) {
		// ȡ�� try/catch Ԫ��
		zend_try_catch_element *try_catch =
			&EX(func)->op_array.try_catch_array[try_catch_offset];

		// �����������catchǰ�棨try�飩,���� ִ��������Ч
		if (op_num < try_catch->catch_op && ex) {
			// ����catch��
			/* Go to catch block */
			// ���������еı����� p1:ִ�����ݣ�p2:�������������ݣ�p3:catch_op_num (?)
			cleanup_live_vars(execute_data, op_num, try_catch->catch_op);
			// Ҫ����쳣 �� ���쳣ʱʹ�þɲ����룬���쳣ʹ���²�����, p1:�²����룬p2:�Ƿ����쳣
			ZEND_VM_JMP_EX(&EX(func)->op_array.opcodes[try_catch->catch_op], 0);
		// ����� finally ǰ�棨catch�飩
		} else if (op_num < try_catch->finally_op) {
			// ���ex��Ч���� ex->ce �� zend_is_unwind_exit ����
			if (ex && zend_is_unwind_exit(ex)) {
				// �˳�ǰ��ִ�� finally��
				/* Don't execute finally blocks on exit (for now) */
				continue;
			}
			// ���� finally��	
			/* Go to finally block */
			// fast_call ��finall�����һ��������� op1
			// ��ִ��������ȡ�� ָ����ŵı���
			zval *fast_call = EX_VAR(EX(func)->op_array.opcodes[try_catch->finally_end].op1.var);
			// ���������еı����� p1:ִ�����ݣ�p2:�������������ݣ�p3:catch_op_num (?)
			cleanup_live_vars(execute_data, op_num, try_catch->finally_op);
			// ��¼��ǰ�쳣��������ô��?
			Z_OBJ_P(fast_call) = EG(exception);
			// ��յ�ǰ�쳣
			EG(exception) = NULL;
			// ��������Ÿ��³� -1
			// ͨ��ָ�����zval�� ���������
			Z_OPLINE_NUM_P(fast_call) = (uint32_t)-1;
			// Ҫ����쳣 �� ���쳣ʱʹ�þɲ����룬���쳣ʹ���²�����, p1:�²����룬p2:�Ƿ����쳣
			ZEND_VM_JMP_EX(&EX(func)->op_array.opcodes[try_catch->finally_op], 0);
		// �����finally��
		} else if (op_num < try_catch->finally_end) {
			// ��ִ��������ȡ�� ָ����ŵı���
			zval *fast_call = EX_VAR(EX(func)->op_array.opcodes[try_catch->finally_end].op1.var);

			// ����δ��ɵ� return ���
			/* cleanup incomplete RETURN statement */
			// ͨ��ָ�����zval�� ���������
			if (Z_OPLINE_NUM_P(fast_call) != (uint32_t)-1
			 && (EX(func)->op_array.opcodes[Z_OPLINE_NUM_P(fast_call)].op2_type & (IS_TMP_VAR | IS_VAR))) {
				// ��ִ��������ȡ�� ָ����ŵı���
				// ͨ��ָ�����zval�� ���������
				zval *return_value = EX_VAR(EX(func)->op_array.opcodes[Z_OPLINE_NUM_P(fast_call)].op2.var);
				// ���ٷ���ֵ
				zval_ptr_dtor(return_value);
			}

			// �ڰ�װfinally��ʱ�� ��Ǳ�ڵ��쳣��������
			/* Chain potential exception from wrapping finally block */
			// ����� fast_call
			if (Z_OBJ_P(fast_call)) {
				// ִ��������Ч
				if (ex) {
					// ��� ex->ce �� zend_ce_unwind_exit ���� �� zend_ce_graceful_exit ����
					if (zend_is_unwind_exit(ex) || zend_is_graceful_exit(ex)) {
						// ����ǰ���׳����쳣 
						/* discard the previously thrown exception */
						// �ͷ� fast_call
						OBJ_RELEASE(Z_OBJ_P(fast_call));
					// ����
					} else {
						// ���ex��add_previous�����ڶԷ���previous���ϣ���add_previous��ӵ�exprevious����Դͷ�ϡ�
						zend_exception_set_previous(ex, Z_OBJ_P(fast_call));
					}
				// ����
				} else {
					// fast_call ��Ϊ��ǰ�쳣
					ex = EG(exception) = Z_OBJ_P(fast_call);
				}
			}
		}
	}

	// δ����� �쳣
	/* Uncaught exception */
	// ����� zend_observer_fcall_op_array_extension
	if (zend_observer_fcall_op_array_extension != -1) {
		// �����۲��ߵ���
		zend_observer_fcall_end(execute_data, NULL);
	}
	// ���������еı����� p1:ִ�����ݣ�p2:�������������ݣ�p3:catch_op_num (?)
	cleanup_live_vars(execute_data, op_num, 0);
	// ����ڵ���������
	if (UNEXPECTED((EX_CALL_INFO() & ZEND_CALL_GENERATOR) != 0)) {
		// ��ȡ�������е�������
		zend_generator *generator = zend_get_running_generator(EXECUTE_DATA_C);
		// �ر�������
		zend_generator_close(generator, 1);
		// windows: return -1
		ZEND_VM_RETURN();
	// ����û���ڵ���������
	} else {
		// ��ִ��return, �����ʼ�� ����ֵ
		/* We didn't execute RETURN, and have to initialize return_value */
		// ����з���ֵ
		if (EX(return_value)) {
			// ����ֵ Ϊ δ����
			ZVAL_UNDEF(EX(return_value));
		}
		// ���÷��� zend_leave_helper
		ZEND_VM_DISPATCH_TO_HELPER(zend_leave_helper);
	}
}

// ing3, �����쳣
ZEND_VM_HANDLER(149, ZEND_HANDLE_EXCEPTION, ANY, ANY)
{
	// �쳣ǰ�Ĳ����룬�׳�������
	const zend_op *throw_op = EG(opline_before_exception);
	// ��Ƹ����������
	uint32_t throw_op_num = throw_op - EX(func)->op_array.opcodes;
	// try/catchԪ�� ���
	int i, current_try_catch_offset = -1;

	// ������������� ZEND_FREE �� ZEND_FE_FREE�����ң�throw��� �� ZEND_FREE_ON_RETURN ��ǣ�
	if ((throw_op->opcode == ZEND_FREE || throw_op->opcode == ZEND_FE_FREE)
		&& throw_op->extended_value & ZEND_FREE_ON_RETURN) {
		// return/break/... ������� ѭ������������ɵ��쳣 �߼��ϻ���foreachѭ���Ľ��׳������Ե��� throw_op_num
		/* exceptions thrown because of loop var destruction on return/break/...
		 * are logically thrown at the end of the foreach loop, so adjust the
		 * throw_op_num.
		 */
		// ���һ����
		const zend_live_range *range = find_live_range(
			&EX(func)->op_array, throw_op_num, throw_op->op1.var);
		
		// �ͷ� ��Ӧ�� RETURN ��op1
		/* free op1 of the corresponding RETURN */
		for (i = throw_op_num; i < range->end; i++) {
			// ����������� ZEND_FREE �� ZEND_FE_FREE
			if (EX(func)->op_array.opcodes[i].opcode == ZEND_FREE
			 || EX(func)->op_array.opcodes[i].opcode == ZEND_FE_FREE) {
				/* pass */
			// ����
			} else {
				// ����������� ZEND_RETURN ���� op1�Ǳ�������ʱ����
				if (EX(func)->op_array.opcodes[i].opcode == ZEND_RETURN
				 && (EX(func)->op_array.opcodes[i].op1_type & (IS_VAR|IS_TMP_VAR))) {
					// ���� op1
					zval_ptr_dtor(EX_VAR(EX(func)->op_array.opcodes[i].op1.var));
				}
				// ����
				break;
			}
		}
		// ���� throw_op_num
		throw_op_num = range->end;
	}

	// �ҵ��� �쳣�� ������ try/catch/finally
	/* Find the innermost try/catch/finally the exception was thrown in */
	// ����ÿ�� zend_try_catch_element
	for (i = 0; i < EX(func)->op_array.last_try_catch; i++) {
		// ÿ�� zend_try_catch_element Ԫ��
		zend_try_catch_element *try_catch = &EX(func)->op_array.try_catch_array[i];
		// ��� �ҵ���
		if (try_catch->try_op > throw_op_num) {
			// ������Ŀ� �ͺ����޹���
			/* further blocks will not be relevant... */
			break;
		}
		// ����ҵ�����cache��
		if (throw_op_num < try_catch->catch_op || throw_op_num < try_catch->finally_end) {
			// �������
			current_try_catch_offset = i;
		}
	}

	// ����δ��ɵĵ���
	cleanup_unfinished_calls(execute_data, throw_op_num);

	// �������� ������ ��ʱ����
	if (throw_op->result_type & (IS_VAR | IS_TMP_VAR)) {
		// ���ݲ��������
		switch (throw_op->opcode) {
			//  �⼸������
			case ZEND_ADD_ARRAY_ELEMENT:
			case ZEND_ADD_ARRAY_UNPACK:
			case ZEND_ROPE_INIT:
			case ZEND_ROPE_ADD:
				// �쳣 �Ὠ��һЩʵ����������������ͷ�����
				break; /* exception while building structures, live range handling will free those */

			// ����
			case ZEND_FETCH_CLASS:
			case ZEND_DECLARE_ANON_CLASS:
				// ����ֵ�� �����ָ��
				break; /* return value is zend_class_entry pointer */

			// �������
			default:
				// ���ܷ�֧����û�г�ʼ��� 
				/* smart branch opcodes may not initialize result */
				// ���û�����ܷ�֧��һЩ�����жϵĲ����룩
				if (!zend_is_smart_branch(throw_op)) {
					// ����û�����ô����Ķ���,��û������ zval����������gc�������ڡ�
					zval_ptr_dtor_nogc(EX_VAR(throw_op->result.var));
				}
		}
	}

	// �ַ� try/catch/finally ����
	// ���÷��� zend_dispatch_try_catch_finally_helper
	ZEND_VM_DISPATCH_TO_HELPER(zend_dispatch_try_catch_finally_helper, try_catch_offset, current_try_catch_offset, op_num, throw_op_num);
}

// ing2, �����û�������
ZEND_VM_HANDLER(150, ZEND_USER_OPCODE, ANY, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	int ret;

	// windows: �޲���
	SAVE_OPLINE();
	// ȡ�� ������ ������Ĵ�����
	ret = zend_user_opcode_handlers[opline->opcode](execute_data);
	// ��ǰ������
	opline = EX(opline);

	// ���ݷ���ֵ����
	switch (ret) {
		// ��������
		case ZEND_USER_OPCODE_CONTINUE:
			// windows: return  0
			ZEND_VM_CONTINUE();
		// ����
		case ZEND_USER_OPCODE_RETURN:
			// ���������������
			if (UNEXPECTED((EX_CALL_INFO() & ZEND_CALL_GENERATOR) != 0)) {
				// ȡ��������
				zend_generator *generator = zend_get_running_generator(EXECUTE_DATA_C);
				// �ر�������
				zend_generator_close(generator, 1);
				// windows: return -1
				ZEND_VM_RETURN();
			// ����
			} else {
				// ���÷��� zend_leave_helper
				ZEND_VM_DISPATCH_TO_HELPER(zend_leave_helper);
			}
		// ����
		case ZEND_USER_OPCODE_ENTER:
			// windows return 1;
			ZEND_VM_ENTER();
		// �뿪
		case ZEND_USER_OPCODE_LEAVE:
			// windows return 2;
			ZEND_VM_LEAVE();
		// ����
		case ZEND_USER_OPCODE_DISPATCH:
			// ���ɣ���������벢return
			ZEND_VM_DISPATCH(opline->opcode, opline);
		// Ĭ�����
		default:
			// ���ɣ���������벢return 
			// ����ֵ��255��
			ZEND_VM_DISPATCH((zend_uchar)(ret & 0xff), opline);
	}
}

// ing3, ��������
ZEND_VM_HANDLER(143, ZEND_DECLARE_CONST, CONST, CONST)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *name;
	zval *val;
	zend_constant c;

	// windows: �޲���
	SAVE_OPLINE();
	// ��ȡzvalָ�룬UNUSED ���ͷ���null
	name  = GET_OP1_ZVAL_PTR(BP_VAR_R);
	// ��ȡzvalָ�룬UNUSED ���ͷ���null
	val   = GET_OP2_ZVAL_PTR(BP_VAR_R);

	// ֵ�ŵ�������
	ZVAL_COPY(&c.value, val);
	// ���ֵΪ�������
	if (Z_OPT_CONSTANT(c.value)) {
		// ���´˱���
		if (UNEXPECTED(zval_update_constant_ex(&c.value, EX(func)->op_array.scope) != SUCCESS)) {
			// ����û�����ô����Ķ���,��û������ zval����������gc�������ڡ�
			zval_ptr_dtor_nogc(&c.value);
			// �ͷŲ�������ĸ��ӱ���
			FREE_OP1();
			// �ͷŲ�������ĸ��ӱ���
			FREE_OP2();
			// ��Ҫ�� return
			HANDLE_EXCEPTION();
		}
	}
	
	// �ǳ־õģ���Сд����
	/* non persistent, case sensitive */
	//  д�� constant_flags ��p1:������p2:_flags��p3:ģ����
	ZEND_CONSTANT_SET_FLAGS(&c, CONST_CS, PHP_USER_CONSTANT);
	// ���������������ô���. ����� ZVAL_COPY ��ͬ
	c.name = zend_string_copy(Z_STR_P(name));

	// ע�᳣����ʧ��Ҳ������
	if (zend_register_constant(&c) == FAILURE) {
	}

	// �ͷŲ�������ĸ��ӱ���
	FREE_OP1();
	// �ͷŲ�������ĸ��ӱ���
	FREE_OP2();
	// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, ʹ�����к����򷽷��������հ�
ZEND_VM_HANDLER(142, ZEND_DECLARE_LAMBDA_FUNCTION, CONST, NUM)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zend_function *func;
	zval *object;
	zend_class_entry *called_scope;
	// ͨ������ҵ�����
	func = (zend_function *) EX(func)->op_array.dynamic_func_defs[opline->op2.num];
	// ���$this�Ƕ���
	if (Z_TYPE(EX(This)) == IS_OBJECT) {
		// ������
		called_scope = Z_OBJCE(EX(This));
		// ���������static �� ��ǰ�����ھ�̬������
		if (UNEXPECTED((func->common.fn_flags & ZEND_ACC_STATIC) ||
				(EX(func)->common.fn_flags & ZEND_ACC_STATIC))) {
			// ����Ҫ����
			object = NULL;
		// ������ͨ����
		} else {
			// ��¼ $this
			object = &EX(This);
		}
	// ����, $this���Ƕ���
	} else {
		// ��¼ $this
		called_scope = Z_CE(EX(This));
		// û�ж���
		object = NULL;
	}
	// �����հ������ڽ����
	zend_create_closure(EX_VAR(opline->result.var), func,
		EX(func)->op_array.scope, called_scope, object);

	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, �ָ��� 1/1�����op1�еı��������ã��������ô�����1��ת������Ŀ�꣨������ã�
// ZEND_FASTCALL ZEND_SEPARATE_SPEC��1����֧����֤����
ZEND_VM_HANDLER(156, ZEND_SEPARATE, VAR, UNUSED)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *var_ptr;

	// ��ִ��������ȡ�� ָ����ŵı���
	var_ptr = EX_VAR(opline->op1.var);
	// ���op1�еı���������
	if (UNEXPECTED(Z_ISREF_P(var_ptr))) {
		// �������ô�����1
		if (UNEXPECTED(Z_REFCOUNT_P(var_ptr) == 1)) {
			// ת������Ŀ��
			ZVAL_UNREF(var_ptr);
		}
	}

	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, ������������һ��ǿ�ƹرյ��������� yield
ZEND_VM_COLD_HELPER(zend_yield_in_closed_generator_helper, ANY, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// windows: �޲���
	SAVE_OPLINE();
	// ������������һ��ǿ�ƹرյ��������� yield
	zend_throw_error(NULL, "Cannot yield from finally in a force-closed generator");
	// �ͷŲ�������ĸ��ӱ���
	FREE_OP2();
	// �ͷŲ�������ĸ��ӱ���
	FREE_OP1();
	// ��� opline ��������� ��������ʱ�������ѽ�� zval ���Ϊδ����
	UNDEF_RESULT();
	// ��Ҫ�� return
	HANDLE_EXCEPTION();
}

// ing2, yield���
ZEND_VM_HANDLER(160, ZEND_YIELD, CONST|TMP|VAR|CV|UNUSED, CONST|TMPVAR|CV|UNUSED, SRC)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// �������е�������
	zend_generator *generator = zend_get_running_generator(EXECUTE_DATA_C);

	// windows: �޲���
	SAVE_OPLINE();
	// �����������ǿ�ƹرձ��
	if (UNEXPECTED(generator->flags & ZEND_GENERATOR_FORCED_CLOSE)) {
		// ������������һ��ǿ�ƹرյ��������� yield
		// ���÷��� zend_yield_in_closed_generator_helper
		ZEND_VM_DISPATCH_TO_HELPER(zend_yield_in_closed_generator_helper);
	}

	// ����ǰ�����ɵ� ֵ
	/* Destroy the previously yielded value */
	zval_ptr_dtor(&generator->value);

	// ����ǰ�����ɵ� ��
	/* Destroy the previously yielded key */
	zval_ptr_dtor(&generator->key);

	// �����µ� yield ֵ
	/* Set the new yielded value */
	if (OP1_TYPE != IS_UNUSED) {
		// ����Ҫ�����÷���
		if (UNEXPECTED(EX(func)->op_array.fn_flags & ZEND_ACC_RETURN_REFERENCE)) {
			// ��������ʱ�������Ա�ͨ������  yield������Ȼ��Ҫ������ʾ
			/* Constants and temporary variables aren't yieldable by reference,
			 * but we still allow them with a notice. */
			// ���op1 �ǳ�������ʱ����
			if (OP1_TYPE & (IS_CONST|IS_TMP_VAR)) {
				//
				zval *value;
				// ��ʾ��ֻ�б��������� ���� ͨ������ yield.
				zend_error(E_NOTICE, "Only variable references should be yielded by reference");
				// ��ȡzvalָ�룬UNUSED ���ͷ���null
				value = GET_OP1_ZVAL_PTR(BP_VAR_R);
				// op1���Ƹ� ��������ֵ
				ZVAL_COPY_VALUE(&generator->value, value);
				// ���op1�ǳ���
				if (OP1_TYPE == IS_CONST) {
					// �����������ֵ�ɼ���
					if (UNEXPECTED(Z_OPT_REFCOUNTED(generator->value))) {
						// �������ô���
						Z_ADDREF(generator->value);
					}
				}
			// ����
			} else {
				// ��ȡzvalָ��,TMP/CONST/UNUSED ����null����֧�� TMPVAR/TMPVARCV
				zval *value_ptr = GET_OP1_ZVAL_PTR_PTR(BP_VAR_W);

				// ���һ���������õĽ�� �� yield ���� ������û��ͨ�����÷��أ���Ҫ�׳���ʾ
				/* If a function call result is yielded and the function did
				 * not return by reference we throw a notice. */
				// Ϊ��break;
				do {
					// ��� op1 �� ��ͨ����
					if (OP1_TYPE == IS_VAR) {
						// ���ԣ�op1��������δ��ʼ��zval
						ZEND_ASSERT(value_ptr != &EG(uninitialized_zval));
						// �����չֵ�� ZEND_RETURNS_FUNCTION ���� op1������������
						if (opline->extended_value == ZEND_RETURNS_FUNCTION
						 && !Z_ISREF_P(value_ptr)) {
							// ��ʾ��ֻ�б��������� ���� ͨ������ yield.
							zend_error(E_NOTICE, "Only variable references should be yielded by reference");
							// op1���Ƹ���������ֵ
							ZVAL_COPY(&generator->value, value_ptr);
							// ����
							break;
						}
					}
					// ��� op1 ����������
					if (Z_ISREF_P(value_ptr)) {
						// ���ô���+1
						Z_ADDREF_P(value_ptr);
					// ����op1������������
					} else {
						// ���³��������ͣ����ô���2
						ZVAL_MAKE_REF_EX(value_ptr, 2);
					}
					// op1������Ŀ�꣬���ø�ֵ�� ��������ֵ
					ZVAL_REF(&generator->value, Z_REF_P(value_ptr));
				} while (0);

				// �ͷŲ�������ĸ��ӱ���
				FREE_OP1();
			}
		// ����
		} else {
			// ��ȡzvalָ�룬UNUSED ���ͷ���null
			zval *value = GET_OP1_ZVAL_PTR(BP_VAR_R);

			// ��������ʱ���������� ����Ҫ����
			/* Consts, temporary variables and references need copying */
			// ���op1�ǳ���
			if (OP1_TYPE == IS_CONST) {
				// op1���Ƹ���������ֵ
				ZVAL_COPY_VALUE(&generator->value, value);
				// ��� ��������ֵ�ǿɼ���
				if (UNEXPECTED(Z_OPT_REFCOUNTED(generator->value))) {
					// �������ô���
					Z_ADDREF(generator->value);
				}
			// op1����ʱ����
			} else if (OP1_TYPE == IS_TMP_VAR) {
				// op1���Ƹ���������ֵ
				ZVAL_COPY_VALUE(&generator->value, value);
			// ��� op1 ����ͨ������������ ���� op1����������
			} else if ((OP1_TYPE & (IS_VAR|IS_CV)) && Z_ISREF_P(value)) {
				// op1������Ŀ�� ���Ƹ���������ֵ�����������ô�����
				ZVAL_COPY(&generator->value, Z_REFVAL_P(value));
				// ֻ�ͷ�����Ϊ IS_VAR �ı���
				FREE_OP1_IF_VAR();
			// ����
			} else {
				// op1 ���Ƹ���������ֵ
				ZVAL_COPY_VALUE(&generator->value, value);
				// ��� op1 �� �������
				if (OP1_TYPE == IS_CV) {
					// ����ǿ��Լ�������
					// ���ô���+1
					if (Z_OPT_REFCOUNTED_P(value)) Z_ADDREF_P(value);
				}
			}
		}
	// ����
	} else {
		// ���û��ָ��ֵ��yield null
		/* If no value was specified yield null */
		// ��������ֵΪnull
		ZVAL_NULL(&generator->value);
	}

	// ���������ɵ�key
	/* Set the new yielded key */
	// ���op2��Ч
	if (OP2_TYPE != IS_UNUSED) {
		// ��ȡzvalָ�룬UNUSED ���ͷ���null
		zval *key = GET_OP2_ZVAL_PTR(BP_VAR_R);
		// ���op2�Ǳ����������� ���� op2����������
		if ((OP2_TYPE & (IS_CV|IS_VAR)) && UNEXPECTED(Z_TYPE_P(key) == IS_REFERENCE)) {
			// ������
			key = Z_REFVAL_P(key);
		}
		// key���Ƹ�������
		ZVAL_COPY(&generator->key, key);
		// �ͷŲ�������ĸ��ӱ���
		FREE_OP2();

		// ���key������ ���� key ���� ���ʹ�õ����� key
		if (Z_TYPE(generator->key) == IS_LONG
		    && Z_LVAL(generator->key) > generator->largest_used_integer_key
		) {
			// �������ʹ�õ�����key
			generator->largest_used_integer_key = Z_LVAL(generator->key);
		}
	// ����
	} else {
		// ���û��ָ��key��ʹ������key
		/* If no key was specified we use auto-increment keys */
		generator->largest_used_integer_key++;
		// ���һ������key ���Ƹ�������
		ZVAL_LONG(&generator->key, generator->largest_used_integer_key);
	}

	// ��� ���������������Ч(����IS_UNUSED)
	if (RETURN_VALUE_USED(opline)) {
		// ���yield�ķ���ֵ�Ѿ���ʹ�ã����÷���Ŀ�꣬����ʼ����null
		/* If the return value of yield is used set the send
		 * target and initialize it to NULL */
		 // ��ִ��������ȡ�� ָ����ŵı���
		generator->send_target = EX_VAR(opline->result.var);
		// ����Ŀ��Ϊ null ����
		ZVAL_NULL(generator->send_target);
	// ����
	} else {
		// ����Ŀ��Ϊ null
		generator->send_target = NULL;
	}

	// �����ҵ���һ�������룬���Ե��������ָ���ʱ�򣬾͵���ȷ��λ���ˡ�
	/* We increment to the next op, so we are at the correct position when the
	 * generator is resumed. */
	// OPLINE++
	ZEND_VM_INC_OPCODE();

	// GOTO VM ʹ��һ�����ز������������Ҫ�����ŵ� ִ�������� ������û���ھ�λ�ûָ���
	/* The GOTO VM uses a local opline variable. We need to set the opline
	 * variable in execute_data so we don't resume at an old position. */
	// windows: �޲���
	SAVE_OPLINE();
	// windows: return -1
	ZEND_VM_RETURN();
}

// ing2, yield from ���
ZEND_VM_HANDLER(166, ZEND_YIELD_FROM, CONST|TMPVAR|CV, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	// ȡ���������е�������
	zend_generator *generator = zend_get_running_generator(EXECUTE_DATA_C);
	zval *val;

	// windows: �޲���
	SAVE_OPLINE();
	// ��ȡzvalָ�룬UNUSED ���ͷ���null
	val = GET_OP1_ZVAL_PTR(BP_VAR_R);

	if (UNEXPECTED(generator->flags & ZEND_GENERATOR_FORCED_CLOSE)) {
		zend_throw_error(NULL, "Cannot use \"yield from\" in a force-closed generator");
		// �ͷŲ�������ĸ��ӱ���
		FREE_OP1();
		// ��� opline ��������� ��������ʱ�������ѽ�� zval ���Ϊδ����
		UNDEF_RESULT();
		// ��Ҫ�� return
		HANDLE_EXCEPTION();
	}

// ������ת��ǩ yield_from_try_again
ZEND_VM_C_LABEL(yield_from_try_again):
	// ���op1������
	if (Z_TYPE_P(val) == IS_ARRAY) {
		// op1��Ϊ��������ֵ��
		ZVAL_COPY_VALUE(&generator->values, val);
		// ����ǿ��Լ�������
		if (Z_OPT_REFCOUNTED_P(val)) {
			// ���ô���+1
			Z_ADDREF_P(val);
		}
		// ֵ��ϣ��ı���λ�ù�0
		Z_FE_POS(generator->values) = 0;
		// �ͷŲ�������ĸ��ӱ���
		FREE_OP1();
	// ���op1���ǳ��� ���� op1�Ƕ��� ���� �л�ȡ����������
	} else if (OP1_TYPE != IS_CONST && Z_TYPE_P(val) == IS_OBJECT && Z_OBJCE_P(val)->get_iterator) {
		// ȡ�� op1����
		zend_class_entry *ce = Z_OBJCE_P(val);
		// ������� ��׼������
		if (ce == zend_ce_generator) {
			// op1ת��������ʵ��
			zend_generator *new_gen = (zend_generator *) Z_OBJ_P(val);
			// ���ô���+1
			Z_ADDREF_P(val);
			// �ͷŲ�������ĸ��ӱ���
			FREE_OP1();

			// ���������û��ִ������
			if (UNEXPECTED(new_gen->execute_data == NULL)) {
				// �������ݸ� yield from ��������û���������أ��޷���������
				zend_throw_error(NULL, "Generator passed to yield from was aborted without proper return and is unable to continue");
				// ���� op1
				zval_ptr_dtor(val);
				// ��� opline ��������� ��������ʱ�������ѽ�� zval ���Ϊδ����
				UNDEF_RESULT();
				// ��Ҫ�� return
				HANDLE_EXCEPTION();
			// ����������з���ֵ
			} else if (Z_ISUNDEF(new_gen->retval)) {
				// ��� ����������������
				if (UNEXPECTED(zend_generator_get_current(new_gen) == generator)) {
					// �״������� yield from �������е�������
					zend_throw_error(NULL, "Impossible to yield from the Generator being currently run");
					// ���� op1
					zval_ptr_dtor(val);
					// ��� opline ��������� ��������ʱ�������ѽ�� zval ���Ϊδ����
					UNDEF_RESULT();
					// ��Ҫ�� return
					HANDLE_EXCEPTION();
				// ����
				} else {
					// �������������� p2����p1,����p1��Ϊp2���ӽڵ�
					zend_generator_yield_from(generator, new_gen);
				}
			// ����
			} else {
				// ��� ���������������Ч(����IS_UNUSED)
				if (RETURN_VALUE_USED(opline)) {
					// ����������ֵ ���Ƹ� ���
					ZVAL_COPY(EX_VAR(opline->result.var), &new_gen->retval);
				}
				// opline + 1����Ŀ������벢 return
				ZEND_VM_NEXT_OPCODE();
			}
		// ����
		} else {
			// ��ȡ����ĵ�һ��������
			zend_object_iterator *iter = ce->get_iterator(ce, val, 0);
			// �ͷŲ�������ĸ��ӱ���
			FREE_OP1();

			// ���û�е����� �� ���쳣
			if (UNEXPECTED(!iter) || UNEXPECTED(EG(exception))) {
				// ���û���쳣
				if (!EG(exception)) {
					// �����˶����޷�����������
					zend_throw_error(NULL, "Object of type %s did not create an Iterator", ZSTR_VAL(ce->name));
				}
				// ��� opline ��������� ��������ʱ�������ѽ�� zval ���Ϊδ����
				UNDEF_RESULT();
				// ��Ҫ�� return
				HANDLE_EXCEPTION();
			}
			// ������ָ�� fe_reset/fe_fetch ��˽��ָ�루������
			// ָ��0
			iter->index = 0;
			// ��������÷���
			if (iter->funcs->rewind) {
				// ����
				iter->funcs->rewind(iter);
				// ������쳣
				if (UNEXPECTED(EG(exception) != NULL)) {
					// ��յ���������zval
					OBJ_RELEASE(&iter->std);
					// ��� opline ��������� ��������ʱ�������ѽ�� zval ���Ϊδ����
					UNDEF_RESULT();
					// ��Ҫ�� return
					HANDLE_EXCEPTION();
				}
			}
			// �ѵ���������zval������������ֵ��
			ZVAL_OBJ(&generator->values, &iter->std);
		}
	// ��� op1 ����ͨ������������ ���� op1����������
	} else if ((OP1_TYPE & (IS_VAR|IS_CV)) && Z_TYPE_P(val) == IS_REFERENCE) {
		// ������
		val = Z_REFVAL_P(val);
		// ��ת��ָ����ǩ yield_from_try_again
		ZEND_VM_C_GOTO(yield_from_try_again);
	// ����
	} else {
		// �״�ֻ���Զ� ����Ϳɱ�������ʹ�� yield from
		zend_throw_error(NULL, "Can use \"yield from\" only with arrays and Traversables");
		// �ͷŲ�������ĸ��ӱ���
		FREE_OP1();
		// ��� opline ��������� ��������ʱ�������ѽ�� zval ���Ϊδ����
		UNDEF_RESULT();
		// ��Ҫ�� return
		HANDLE_EXCEPTION();
	}

	// ����Ĭ�Ϸ���ֵ�������ʽ���������������� zend_generator_resume �б�����
	/* This is the default return value
	 * when the expression is a Generator, it will be overwritten in zend_generator_resume() */
	// ��� ���������������Ч(����IS_UNUSED)
	if (RETURN_VALUE_USED(opline)) {
		// ��� Ϊnull
		ZVAL_NULL(EX_VAR(opline->result.var));
	}

	// ��������û�з���Ŀ�꣨���ܱ�ί�ɵ�������������Ŀ�꣩
	/* This generator has no send target (though the generator we delegate to might have one) */
	generator->send_target = NULL;

	// �����ҵ���һ�������룬���Ե��������ָ���ʱ�򣬾͵���ȷ��λ���ˡ�
	/* We increment to the next op, so we are at the correct position when the
	 * generator is resumed. */
	 
	// OPLINE++
	ZEND_VM_INC_OPCODE();

	// GOTO VM ʹ��һ�����ز������������Ҫ�����ŵ� ִ�������� ������û���ھ�λ�ûָ���
	/* The GOTO VM uses a local opline variable. We need to set the opline
	 * variable in execute_data so we don't resume at an old position. */
	// windows: �޲���
	SAVE_OPLINE();
	// windows: return -1
	ZEND_VM_RETURN();
}

// ing3, ����ǰ��������쳣
ZEND_VM_HANDLER(159, ZEND_DISCARD_EXCEPTION, ANY, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	// ��ִ��������ȡ�� ָ����ŵı���
	zval *fast_call = EX_VAR(opline->op1.var);
	// windows: �޲���
	SAVE_OPLINE();

	// ����δ��ɵ� return ���
	/* cleanup incomplete RETURN statement */
	
	// �����������Ч ���� �������op2�� ����ʱ���� �� ������
	// ͨ��ָ�����zval�� ���������
	if (Z_OPLINE_NUM_P(fast_call) != (uint32_t)-1
	 && (EX(func)->op_array.opcodes[Z_OPLINE_NUM_P(fast_call)].op2_type & (IS_TMP_VAR | IS_VAR))) {
		// ȡ��Ŀ��������op2�ı�����op2��
		// ��ִ��������ȡ�� ָ����ŵı���
		// ͨ��ָ�����zval�� ���������
		zval *return_value = EX_VAR(EX(func)->op_array.opcodes[Z_OPLINE_NUM_P(fast_call)].op2.var);
		// ����û�����ô����Ķ��������ô�������ӵ�gc�����������û������ zval����
		zval_ptr_dtor(return_value);
	}

	// ������ʱ�쳣
	/* cleanup delayed exception */
	// ���Ŀ��������
	if (Z_OBJ_P(fast_call) != NULL) {
		// ����֮ǰ�׳����쳣
		/* discard the previously thrown exception */
		// �ͷ�Ŀ�����
		OBJ_RELEASE(Z_OBJ_P(fast_call));
		// ָ���ÿ�
		Z_OBJ_P(fast_call) = NULL;
	}

	// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, ���ٵ���, ֱ����ת��Ŀ�������
ZEND_VM_HANDLER(162, ZEND_FAST_CALL, JMP_ADDR, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	// ��ִ��������ȡ�� ָ����ŵı���
	zval *fast_call = EX_VAR(opline->result.var);

	// ����ָ������Ϊ��
	Z_OBJ_P(fast_call) = NULL;
	
	// ���÷��ص�ַ����ǰ��������
	/* set return address */
	// ͨ��ָ�����zval�� ���������
	Z_OPLINE_NUM_P(fast_call) = opline - EX(func)->op_array.opcodes;
	// ��ת��p2ָ���Ĳ�����
	// Ҫ����쳣 �� ���쳣ʱʹ�þɲ����룬���쳣ʹ���²�����, p1:�²����룬p2:�Ƿ����쳣
	// OP_JMP_ADDR: ���� p2.jmp_addr
	ZEND_VM_JMP_EX(OP_JMP_ADDR(opline, opline->op1), 0);
}

// ing2,
ZEND_VM_HANDLER(163, ZEND_FAST_RET, ANY, TRY_CATCH)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	// ��ִ��������ȡ�� ָ����ŵı���
	zval *fast_call = EX_VAR(opline->op1.var);
	// 
	uint32_t current_try_catch_offset, current_op_num;

	// ͨ��ָ�����zval�� ���������
	if (Z_OPLINE_NUM_P(fast_call) != (uint32_t)-1) {
		// ͨ��ָ�����zval�� ���������
		const zend_op *fast_ret = EX(func)->op_array.opcodes + Z_OPLINE_NUM_P(fast_call);
		// Ҫ����쳣 �� ���쳣ʱʹ�þɲ����룬���쳣ʹ���²�����, p1:�²����룬p2:�Ƿ����쳣
		ZEND_VM_JMP_EX(fast_ret + 1, 0);
	}
	// ����δ�����쳣�����⴦��
	/* special case for unhandled exceptions */
	// op1��Ϊ��ǰ�쳣
	EG(exception) = Z_OBJ_P(fast_call);
	// ���op1
	Z_OBJ_P(fast_call) = NULL;
	// ��ǰ try/catch ��� op2��ֵ
	current_try_catch_offset = opline->op2.num;
	// ��ǰ���������
	current_op_num = opline - EX(func)->op_array.opcodes;
	
	// �ַ� try/catch/finally ����
	// ���÷��� zend_dispatch_try_catch_finally_helper
	ZEND_VM_DISPATCH_TO_HELPER(zend_dispatch_try_catch_finally_helper, try_catch_offset, current_try_catch_offset, op_num, current_op_num);
}

// ing3, ��ȫ�ֱ��� EG(symbol_table) ��
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
	// ��ȡzvalָ�룬UNUSED ���ͷ���null
	varname = Z_STR_P(GET_OP2_ZVAL_PTR(BP_VAR_R));

	/* We store "hash slot index" + 1 (NULL is a mark of uninitialized cache slot) */
	// ͨ��ƫ���� ������ʱ�����л�ȡһ�� (void**) �еĵ�һ��Ԫ��
	// Ȼ���ָ��ת������ ��-1
	idx = (uintptr_t)CACHED_PTR(opline->extended_value) - 1;
	// ���λ������ʹ�÷�Χ��
	if (EXPECTED(idx < EG(symbol_table).nNumUsed * sizeof(Bucket))) {
		// ȡ�� bucket
		Bucket *p = (Bucket*)((char*)EG(symbol_table).arData + idx);
		// ���key�ͱ�����ָ����ͬ �򣨹�ϣֵ�ͱ�������� ���� ��key ����key��ͬ��
		if (EXPECTED(p->key == varname) ||
		    (EXPECTED(p->h == ZSTR_H(varname)) &&
		     EXPECTED(p->key != NULL) &&
		     EXPECTED(zend_string_equal_content(p->key, varname)))) {
			// ʹ���ҵ���ֵ
			value = (zval*)p; /* value = &p->val; */
			
			// ��ת��ָ����ǩ check_indirect
			ZEND_VM_C_GOTO(check_indirect);
		}
	}

	// ʹ�ñ������ڹ�ϣ������� 
	value = zend_hash_find_known_hash(&EG(symbol_table), varname);
	// ���û�ҵ�
	if (UNEXPECTED(value == NULL)) {
		// ��Ӵ˱�����ֵΪδ�������
		value = zend_hash_add_new(&EG(symbol_table), varname, &EG(uninitialized_zval));
		// ȡ�����bucket��λ��
		idx = (char*)value - (char*)EG(symbol_table).arData;
		// ���� ����ϣλ����š�+1 ��null��δ��ʼ������λ�õı�ǣ�
		/* Store "hash slot index" + 1 (NULL is a mark of uninitialized cache slot) */
		// ͨ��ƫ���� �� EX(run_time_cache) �л�ȡһ�� (void**) �еĵ�һ��Ԫ�أ���������ֵ
		CACHE_PTR(opline->extended_value, (void*)(idx + 1));
	// ��������ҵ���
	} else {
		// ȡ�����bucket��λ��
		idx = (char*)value - (char*)EG(symbol_table).arData;
		// ���� ����ϣλ����š�+1 ��null��δ��ʼ������λ�õı�ǣ�
		/* Store "hash slot index" + 1 (NULL is a mark of uninitialized cache slot) */
		// ͨ��ƫ���� �� EX(run_time_cache) �л�ȡһ�� (void**) �еĵ�һ��Ԫ�أ���������ֵ
		CACHE_PTR(opline->extended_value, (void*)(idx + 1));
// ������ת��ǩ check_indirect
ZEND_VM_C_LABEL(check_indirect):
		// ȫ�ֱ���������һ�� ���ָ��ָ�� �������
		/* GLOBAL variable may be an INDIRECT pointer to CV */
		if (UNEXPECTED(Z_TYPE_P(value) == IS_INDIRECT)) {
			// ��������
			value = Z_INDIRECT_P(value);
			// ���ֵ��δ����
			if (UNEXPECTED(Z_TYPE_P(value) == IS_UNDEF)) {
				// ֵΪnull
				ZVAL_NULL(value);
			}
		}
	}

	// ���ֵ������������
	if (UNEXPECTED(!Z_ISREF_P(value))) {
		// ���³��������ͣ����ô���2
		ZVAL_MAKE_REF_EX(value, 2);
		// ���ö���
		ref = Z_REF_P(value);
	// ����
	} else {
		// ȡ�����ö���
		ref = Z_REF_P(value);
		// �������ô���
		GC_ADDREF(ref);
	}
	// ��ȡzvalָ��, TMP/CONST/UNUSED ����null����֧�� TMPVAR/TMPVARCV
	variable_ptr = GET_OP1_ZVAL_PTR_PTR_UNDEF(BP_VAR_W);

	// ��� op1 �� �ɼ�������
	if (UNEXPECTED(Z_REFCOUNTED_P(variable_ptr))) {
		// ȡ�û���ָ��
		zend_refcounted *garbage = Z_COUNTED_P(variable_ptr);
		// op1 ת����������
		ZVAL_REF(variable_ptr, ref);
		// windows: �޲���
		SAVE_OPLINE();
		// ���ô���-1�����Ϊ0
		if (GC_DELREF(garbage) == 0) {
			// ����ÿ��type��Ӧ�����ٺ���ִ������
			rc_dtor_func(garbage);
			// ������쳣
			if (UNEXPECTED(EG(exception))) {
				// ��ֵΪnull
				ZVAL_NULL(variable_ptr);
				// ��Ҫ�� return
				HANDLE_EXCEPTION();
			}
		// ����-1���д���
		} else {
			// ��ӵ���������
			gc_check_possible_root(garbage);
		}
	// ���򣬲��ǿɼ�������
	} else {
		// ֱ�����ø�ֵ��  op1
		ZVAL_REF(variable_ptr, ref);
	}
	// while() ѭ����β�Σ�����һ�������룬ֱ������p1������
	ZEND_VM_REPEAT_OPCODE(ZEND_BIND_GLOBAL);
	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, strlen����
ZEND_VM_COLD_CONST_HANDLER(121, ZEND_STRLEN, CONST|TMPVAR|CV, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *value;

	// ��ȡzvalָ��, UNUSED ����null
	value = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ��������� �ִ�
	if (EXPECTED(Z_TYPE_P(value) == IS_STRING)) {
		ZVAL_LONG(EX_VAR(opline->result.var), Z_STRLEN_P(value));
		// ��� op1 ���� �Ǳ�������ʱ����
		if (OP1_TYPE & (IS_TMP_VAR|IS_VAR)) {
			// ͨ��ָ������ zval���ִ���
			zval_ptr_dtor_str(value);
		}
		// opline + 1����Ŀ������벢 return
		ZEND_VM_NEXT_OPCODE();
	// ����
	} else {
		bool strict;
		// ��� op1�� ����ͨ���� �� ��������� ����ֵ��������
		if ((OP1_TYPE & (IS_VAR|IS_CV)) && Z_TYPE_P(value) == IS_REFERENCE) {
			// ������
			value = Z_REFVAL_P(value);
			// ��������� �ִ�
			if (EXPECTED(Z_TYPE_P(value) == IS_STRING)) {
				// �ѳ���ת������ ���ڽ�� ��
				ZVAL_LONG(EX_VAR(opline->result.var), Z_STRLEN_P(value));
				// �ͷŲ�������ĸ��ӱ���
				FREE_OP1();
				// opline + 1����Ŀ������벢 return
				ZEND_VM_NEXT_OPCODE();
			}
		}

		// windows: �޲���
		SAVE_OPLINE();
		// ��� op1 �� ������� ����ֵΪ δ����
		if (OP1_TYPE == IS_CV && UNEXPECTED(Z_TYPE_P(value) == IS_UNDEF)) {
			// ����p1�ı�������������δ����, ����δ��ʼ��zval
			value = ZVAL_UNDEFINED_OP1();
		}
		// ĳִ���������������� �Ĳ����Ƿ�ʹ���ϸ���������
		strict = EX_USES_STRICT_TYPES();
		//
		do {
			// ��������ϸ�����
			if (EXPECTED(!strict)) {
				// ��ʱ����
				zend_string *str;
				zval tmp;
				// ���ֵ��null
				if (UNEXPECTED(Z_TYPE_P(value) == IS_NULL)) {
					// ������strlen����null���������õ��÷�
					zend_error(E_DEPRECATED,
						"strlen(): Passing null to parameter #1 ($string) of type string is deprecated");
					// ���Ϊ0
					ZVAL_LONG(EX_VAR(opline->result.var), 0);
					// ������쳣
					if (UNEXPECTED(EG(exception))) {
						// ��Ҫ�� return
						HANDLE_EXCEPTION();
					}
					// ����
					break;
				}
				
				// ���ֵ����null
				// �ȸ��Ƶ���ʱ������
				ZVAL_COPY(&tmp, value);
				// �Ѵ���Ĳ���ת�����ִ�������null�ᱨ��p2:����ֵ��p3:arg������ţ�������
				if (zend_parse_arg_str_weak(&tmp, &str, 1)) {
					// Ϊת������ִ����㳤��
					ZVAL_LONG(EX_VAR(opline->result.var), ZSTR_LEN(str));
					// ������ʱ�ִ�
					zval_ptr_dtor(&tmp);
					// ����
					break;
				}
				// ������ʱ�ִ�
				zval_ptr_dtor(&tmp);
			}
			// ���û���쳣
			if (!EG(exception)) {
				// ���� strlen��һ�������������ִ����ͣ�������A����
				zend_type_error("strlen(): Argument #1 ($string) must be of type string, %s given", zend_zval_type_name(value));
			}
			// ���Ϊ δ����
			ZVAL_UNDEF(EX_VAR(opline->result.var));
		} while (0);
	}
	// �ͷŲ�������ĸ��ӱ���
	FREE_OP1();
	// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, ��������
ZEND_VM_HOT_NOCONST_HANDLER(123, ZEND_TYPE_CHECK, CONST|TMPVAR|CV, ANY, TYPE_MASK)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *value;
	int result = 0;

	// ��ȡzvalָ��, UNUSED ����null
	value = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ����Ϊnullʱ ����1λ�������Ч
	if ((opline->extended_value >> (uint32_t)Z_TYPE_P(value)) & 1) {
// ������ת��ǩ type_check_resource
ZEND_VM_C_LABEL(type_check_resource):
		// ��� ����������Դ���� �� ����Դ�б���
		if (opline->extended_value != MAY_BE_RESOURCE
		// ȡ����Դ�б����ͣ������Ч
		 || EXPECTED(NULL != zend_rsrc_list_get_rsrc_type(Z_RES_P(value)))) {
			// ���Ϊ1
			result = 1;
		}
	// ���op1 �Ǳ������ �� ��ͨ���� ������ ��������
	} else if ((OP1_TYPE & (IS_CV|IS_VAR)) && Z_ISREF_P(value)) {
		// ������
		value = Z_REFVAL_P(value);
		// �����null������1λ
		if ((opline->extended_value >> (uint32_t)Z_TYPE_P(value)) & 1) {
			// ��ת��ָ����ǩ type_check_resource
			ZEND_VM_C_GOTO(type_check_resource);
		}
	// ��� op1 �� ������� ���� ֵΪδ����
	} else if (OP1_TYPE == IS_CV && UNEXPECTED(Z_TYPE_P(value) == IS_UNDEF)) {
		result = ((1 << IS_NULL) & opline->extended_value) != 0;
		// windows: �޲���
		SAVE_OPLINE();
		// ����p1�ı�������������δ����, ����δ��ʼ��zval
		ZVAL_UNDEFINED_OP1();
		// ������쳣
		if (UNEXPECTED(EG(exception))) {
			// ���Ϊ δ����
			ZVAL_UNDEF(EX_VAR(opline->result.var));
			// ��Ҫ�� return
			HANDLE_EXCEPTION();
		}
	}
	// ��� op1 ���� �Ǳ�������ʱ����
	if (OP1_TYPE & (IS_TMP_VAR|IS_VAR)) {
		// windows: �޲���
		SAVE_OPLINE();
		// �ͷŲ�������ĸ��ӱ���
		FREE_OP1();
		// ���ݵ�ǰ��������ѡ����һ�������룬p1:����ֵ����Ҫ�������жϡ�p2:�Ƿ����쳣
		ZEND_VM_SMART_BRANCH(result, 1);
	// ����
	} else {
		// ���ݵ�ǰ��������ѡ����һ�������룬p1:����ֵ����Ҫ�������жϡ�p2:�Ƿ����쳣
		ZEND_VM_SMART_BRANCH(result, 0);
	}
}

// ing3, defined ����
ZEND_VM_HOT_HANDLER(122, ZEND_DEFINED, CONST, ANY, CACHE_SLOT)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zend_constant *c;

	// ȡ������
	// ͨ��ƫ���� ������ʱ�����л�ȡһ�� (void**) �еĵ�һ��Ԫ��
	c = CACHED_PTR(opline->extended_value);
	// ��� ������Ч
	if (EXPECTED(c != NULL)) {
		// ��֤��ַ�Ƿ������⻺���ַ�����һλ��1�����������
		if (!IS_SPECIAL_CACHE_VAL(c)) {
// ������ת��ǩ defined_true
ZEND_VM_C_LABEL(defined_true):
			// ���ݵ�ǰ��������ѡ����һ�������롣���������ת���ͣ������ֵΪtrue��
			ZEND_VM_SMART_BRANCH_TRUE();
		// �����⻺���ַ����������� == �������⻺����ţ������ ����1λ
		} else if (EXPECTED(zend_hash_num_elements(EG(zend_constants)) == DECODE_SPECIAL_CACHE_NUM(c))) {
// ������ת��ǩ defined_false
ZEND_VM_C_LABEL(defined_false):
			// ���ݵ�ǰ��������ѡ����һ�������롣���������ת���ͣ������ֵΪfalse��
			ZEND_VM_SMART_BRANCH_FALSE();
		}
	}
	// ���ټ�ⳣ�����ڡ�p1:��������p2:flags
	// ���� (p2).zv��p1:opline,p2:node
	if (zend_quick_check_constant(RT_CONSTANT(opline, opline->op1) OPLINE_CC EXECUTE_DATA_CC) != SUCCESS) {
		// ͨ��ƫ���� �� EX(run_time_cache) �л�ȡһ�� (void**) �еĵ�һ��Ԫ�أ���������ֵ
		// ��չֵ�б��棺 �������⻺����ţ������ ����1λ
		CACHE_PTR(opline->extended_value, ENCODE_SPECIAL_CACHE_NUM(zend_hash_num_elements(EG(zend_constants))));
		// ��ת��ָ����ǩ defined_false
		ZEND_VM_C_GOTO(defined_false);
	// ����
	} else {
		// ��ת��ָ����ǩ defined_true
		ZEND_VM_C_GOTO(defined_true);
	}
}

//  ing3, ���ԣ� 1/1�������֤�ɹ����ѽ������ opline �Ľ���Ȼ������� 
// ZEND_FASTCALL ZEND_ASSERT_CHECK_SPEC��1����֧����֤����
ZEND_VM_HANDLER(151, ZEND_ASSERT_CHECK, ANY, JMP_ADDR)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	// STD_ZEND_INI_ENTRY(\"zend.assertions\", \"1\", ZEND_INI_ALL, OnUpdateAssertions, assertions, zend_executor_globals, executor_globals)
	// ��������˶��Թ���
	if (EG(assertions) <= 0) {
		//  ���� p2.jmp_addr
		zend_op *target = OP_JMP_ADDR(opline, opline->op2);
		// ��� ���������������Ч(����IS_UNUSED)
		if (RETURN_VALUE_USED(opline)) {
			// ���Ϊture
			ZVAL_TRUE(EX_VAR(opline->result.var));
		}
		// Ҫ����쳣 �� ���쳣ʱʹ�þɲ����룬���쳣ʹ���²�����, p1:�²����룬p2:�Ƿ����쳣
		ZEND_VM_JMP_EX(target, 0);
	// ����ֱ�ӵ���һ��������
	// ����
	} else {
		// opline + 1����Ŀ������벢 return
		ZEND_VM_NEXT_OPCODE();
	}
}

// ing3, ��ȡ������ħ������ ::class, self,parent,static
ZEND_VM_HANDLER(157, ZEND_FETCH_CLASS_NAME, CV|TMPVAR|UNUSED|CLASS_FETCH, ANY)
{
	uint32_t fetch_type;
	zend_class_entry *called_scope, *scope;
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	// ���op1����Ϊ δ����
	if (OP1_TYPE != IS_UNUSED) {
		// windows: �޲���
		SAVE_OPLINE();
		// ��ȡzvalָ�룬UNUSED ���ͷ���null
		zval *op = GET_OP1_ZVAL_PTR(BP_VAR_R);
		// ���op1���Ƕ���
		if (UNEXPECTED(Z_TYPE_P(op) != IS_OBJECT)) {
			// ������������ͣ�׷�ٵ������õĶ���
			ZVAL_DEREF(op);
			// ��������Ƕ���
			if (Z_TYPE_P(op) != IS_OBJECT) {
				// �������ڴ�������ʹ��  ::class
				zend_type_error("Cannot use \"::class\" on value of type %s", zend_zval_type_name(op));
				// ���Ϊ δ����
				ZVAL_UNDEF(EX_VAR(opline->result.var));
				// �ͷŲ�������ĸ��ӱ���
				FREE_OP1();
				// ��Ҫ�� return
				HANDLE_EXCEPTION();
			}
		}

		ZVAL_STR_COPY(EX_VAR(opline->result.var), Z_OBJCE_P(op)->name);
		// �ͷŲ�������ĸ��ӱ���
		FREE_OP1();
		// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
		ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
	}

	// ����
	fetch_type = opline->op1.num;
	// ��ǰ��
	scope = EX(func)->op_array.scope;
	// �����ǰ����null
	if (UNEXPECTED(scope == NULL)) {
		// windows: �޲���
		SAVE_OPLINE();
		// ������������ȫ������ʹ�� self/parent/static 
		zend_throw_error(NULL, "Cannot use \"%s\" in the global scope",
			fetch_type == ZEND_FETCH_CLASS_SELF ? "self" :
			fetch_type == ZEND_FETCH_CLASS_PARENT ? "parent" : "static");
		// ���Ϊ δ����
		ZVAL_UNDEF(EX_VAR(opline->result.var));
		// ��Ҫ�� return
		HANDLE_EXCEPTION();
	}

	// �����Ͳ���
	switch (fetch_type) {
		// self
		case ZEND_FETCH_CLASS_SELF:
			// ֱ��ʹ�õ�ǰ ������
			ZVAL_STR_COPY(EX_VAR(opline->result.var), scope->name);
			// ����
			break;
		// parent
		case ZEND_FETCH_CLASS_PARENT:
			// ���û�� parent ����
			if (UNEXPECTED(scope->parent == NULL)) {
				// windows: �޲���
				SAVE_OPLINE();
				// ����û�и�������в���ʹ�� parent
				zend_throw_error(NULL,
					"Cannot use \"parent\" when current class scope has no parent");
				// ���Ϊ δ����
				ZVAL_UNDEF(EX_VAR(opline->result.var));
				// ��Ҫ�� return
				HANDLE_EXCEPTION();
			}
			// �ҵ���Ч�򣬰����Ƹ��Ƹ���� 
			ZVAL_STR_COPY(EX_VAR(opline->result.var), scope->parent->name);
			break;
		// static
		case ZEND_FETCH_CLASS_STATIC:
			// ��� $this �Ƕ���
			if (Z_TYPE(EX(This)) == IS_OBJECT) {
				// ȡ������
				called_scope = Z_OBJCE(EX(This));
			// ����
			} else {
				called_scope = Z_CE(EX(This));
			}
			ZVAL_STR_COPY(EX_VAR(opline->result.var), called_scope->name);
			break;
		EMPTY_SWITCH_DEFAULT_CASE()
	}
	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing2, ���õ������� 
ZEND_VM_HANDLER(158, ZEND_CALL_TRAMPOLINE, ANY, ANY, SPEC(OBSERVER))
{
	zend_array *args = NULL;
	zend_function *fbc = EX(func);
	// ִ�������еķ���ֵ
	zval *ret = EX(return_value);
	// ������Ϣ����� Ƕ�׵��ã�������ã���Ҫ�ͷ�$this, ����������� 4�����
	uint32_t call_info = EX_CALL_INFO() & (ZEND_CALL_NESTED | ZEND_CALL_TOP | ZEND_CALL_RELEASE_THIS | ZEND_CALL_HAS_EXTRA_NAMED_PARAMS);
	// ��������
	uint32_t num_args = EX_NUM_ARGS();
	zend_execute_data *call;

	// windows: �޲���
	SAVE_OPLINE();

	// ����в���
	if (num_args) {
		// ��һ������
		zval *p = ZEND_CALL_ARG(execute_data, 1);
		// �����б��β
		zval *end = p + num_args;

		// �����²�����
		args = zend_new_array(num_args);
		// ��ʼ����˳������
		zend_hash_real_init_packed(args);
		// �Ѳ���������ӵ��±���
		ZEND_HASH_FILL_PACKED(args) {
			// ����������
			do {
				// ���±�����Ӳ���
				ZEND_HASH_FILL_ADD(p);
				// ��һ������
				p++;
			} while (p != end);
		} ZEND_HASH_FILL_END();
	}

	call = execute_data;
	// ת��ǰһ��ִ������
	execute_data = EG(current_execute_data) = EX(prev_execute_data);

	// ����Ǿ�̬������ʹ�� __callstatic ���� __call
	call->func = (fbc->op_array.fn_flags & ZEND_ACC_STATIC) ? fbc->op_array.scope->__callstatic : fbc->op_array.scope->__call;
	// ���ԣ�??
	ZEND_ASSERT(zend_vm_calc_used_stack(2, call->func) <= (size_t)(((char*)EG(vm_stack_end)) - (char*)call));
	// ������������Ϊ2��
	ZEND_CALL_NUM_ARGS(call) = 2;

	// ��һ�������� ������
	ZVAL_STR(ZEND_CALL_ARG(call, 1), fbc->common.function_name);

	// �ڶ������� ָ��
	zval *call_args = ZEND_CALL_ARG(call, 2);
	// ����в�����
	if (args) {
		// �ڶ��������� ������
		ZVAL_ARR(call_args, args);
	// ����
	} else {
		// �ڶ��������ǿ�����
		ZVAL_EMPTY_ARRAY(call_args);
	}
	// ����ж������
	if (UNEXPECTED(call_info & ZEND_CALL_HAS_EXTRA_NAMED_PARAMS)) {
		// �������������0
		if (zend_hash_num_elements(Z_ARRVAL_P(call_args)) == 0) {
			// ���� ���� ������
			// �������ô���
			GC_ADDREF(call->extra_named_params);
			// ���� ���� ��������Ϊ���ε��õĲ�����
			ZVAL_ARR(call_args, call->extra_named_params);
		// ����
		} else {
			// ���Ѳ�����ʹ�ø���
			SEPARATE_ARRAY(call_args);
			// �Ѷ���������ƽ�����ʹ�� zval_add_ref ������Ϊÿ��Ԫ�صĹ��췽�������ô��ݣ�
			zend_hash_copy(Z_ARRVAL_P(call_args), call->extra_named_params, zval_add_ref);
		}
	}
	// �ͷŵ�������
	zend_free_trampoline(fbc);
	// ʹ�õ�ǰ����
	fbc = call->func;

	// ��������� �û�����
	if (EXPECTED(fbc->type == ZEND_USER_FUNCTION)) {
		// ���û������ʱ����
		if (UNEXPECTED(!RUN_TIME_CACHE(&fbc->op_array))) {
			// Ϊop_array������ ��ʼ������ʱ����
			init_func_run_time_cache(&fbc->op_array);
		}
		// ʹ�ø��޸Ĺ���ִ������
		execute_data = call;
		// ��ʼ��������ִ�����ݣ�p1:�������飬p2:����ֵ
		i_init_func_execute_data(&fbc->op_array, ret, 0 EXECUTE_DATA_CC);
		// ���ʹ�� zend_execute_ex ����
		if (EXPECTED(zend_execute_ex == execute_ex)) {
			// windows: �޲���
			LOAD_OPLINE_EX();
			// zend_vm_gen.php ����
			// "/ZEND_OBSERVER_SAVE_OPLINE\(\)/" => isset($extra_spec['OBSERVER']) && $extra_spec['OBSERVER'] == 1 ? "SAVE_OPLINE()" : "",
			ZEND_OBSERVER_SAVE_OPLINE();
			// ��ʼ�۲��ߵ���
			// "/ZEND_OBSERVER_FCALL_BEGIN\(\s*(.*)\s*\)/" => isset($extra_spec['OBSERVER']) ?            ($extra_spec['OBSERVER'] == 0 ? "" : "zend_observer_fcall_begin(\\1)")            : "",
			ZEND_OBSERVER_FCALL_BEGIN(execute_data);
			// return  1
			ZEND_VM_ENTER_EX();
		// ����
		} else {
			// windows: �޲���
			SAVE_OPLINE_EX();
			// ��ʼ�۲��ߵ���
			// "/ZEND_OBSERVER_FCALL_BEGIN\(\s*(.*)\s*\)/" => isset($extra_spec['OBSERVER']) ?            ($extra_spec['OBSERVER'] == 0 ? "" : "zend_observer_fcall_begin(\\1)")            : "",
			ZEND_OBSERVER_FCALL_BEGIN(execute_data);
			// ʹ��ǰһ��ִ������
			execute_data = EX(prev_execute_data);
			// ���ִ�����ݴ���
			if (execute_data) {
				// windows: �޲���
				LOAD_OPLINE();
			}
			// ��ӱ�ǣ��������
			ZEND_ADD_CALL_FLAG(call, ZEND_CALL_TOP);
			// ִ�д� ����
			zend_execute_ex(call);
		}
	// ���������ú���
	} else {
		zval retval;
		// ���������ú���
		ZEND_ASSERT(fbc->type == ZEND_INTERNAL_FUNCTION);
		// ��ִ������ת����ǰ
		EG(current_execute_data) = call;

// ������
#if ZEND_DEBUG
		bool should_throw = zend_internal_call_should_throw(fbc, call);
#endif

		// �������ֵ����Ϊnull
		if (ret == NULL) {
			// ʹ����ʱ����ֵ
			ret = &retval;
		}

		// ����ֵΪnul
		ZVAL_NULL(ret);
		// ��ʼ�۲��ߵ��� zend_observer_fcall_begin()
		// "/ZEND_OBSERVER_FCALL_BEGIN\(\s*(.*)\s*\)/" => isset($extra_spec['OBSERVER']) ?            ($extra_spec['OBSERVER'] == 0 ? "" : "zend_observer_fcall_begin(\\1)")            : "",
		ZEND_OBSERVER_FCALL_BEGIN(call);
		
		// ���û���ڲ�ִ�з���
		if (!zend_execute_internal) {
			// ���û�� �ڲ�ִ������ ����һ����������
			/* saves one function call if zend_execute_internal is not used */
			fbc->internal_function.handler(call, ret);
		// ����
		} else {
			// ���� �ڲ�ִ�з���
			zend_execute_internal(call, ret);
		}
// ������
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
		// ����õ��˹۲��ߣ������۲��ߵ���
		// "/ZEND_OBSERVER_FCALL_END\(\s*([^,]*)\s*,\s*(.*)\s*\)/" => isset($extra_spec['OBSERVER']) ?            ($extra_spec['OBSERVER'] == 0 ? "" : "zend_observer_fcall_end(\\1, \\2)")            : "",
		ZEND_OBSERVER_FCALL_END(call, EG(exception) ? NULL : ret);

		// ת��ǰһ��ִ������
		EG(current_execute_data) = call->prev_execute_data;

		// �ͷ�����ִ�������е� ��������
		zend_vm_stack_free_args(call);
		// ���ʹ�õ�����ʱ����
		if (ret == &retval) {
			// ������ʱ����
			zval_ptr_dtor(ret);
		}
	}

	// ��ǰִ������
	execute_data = EG(current_execute_data);

	// ���û��ִ������ �� û�к��� ����û������ �� ����������
	if (!execute_data || !EX(func) || !ZEND_USER_CODE(EX(func)->type) || (call_info & ZEND_CALL_TOP)) {
		// windows: return -1
		ZEND_VM_RETURN();
	}
	
	// �����Ҫ�ͷ�this
	if (UNEXPECTED(call_info & ZEND_CALL_RELEASE_THIS)) {
		// ȡ�� $this
		zend_object *object = Z_OBJ(call->This);
		// �ͷ���
		OBJ_RELEASE(object);
	}
	// �ͷŵ��ÿ�ܣ�ִ�����ݣ���p1:ִ������
	zend_vm_stack_free_call_frame(call);

	// ������쳣
	if (UNEXPECTED(EG(exception) != NULL)) {
		// ��ִ�������� �����״�
		zend_rethrow_exception(execute_data);
		// windows: return 2;
		HANDLE_EXCEPTION_LEAVE();
	}

	// windows: �޲���
	LOAD_OPLINE();
	// OPLINE++
	ZEND_VM_INC_OPCODE();
	// windows return 2;
	ZEND_VM_LEAVE();
}

// ing3, �󶨱հ��� use() �ı���
ZEND_VM_HANDLER(182, ZEND_BIND_LEXICAL, TMP, CV, REF)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *closure, *var;

	// ��ȡzvalָ�룬UNUSED ���ͷ���null
	closure = GET_OP1_ZVAL_PTR(BP_VAR_R);
	// �����Ҫ������
	if (opline->extended_value & ZEND_BIND_REF) {
		/* By-ref binding */
		// ��ȡ op2��zvalָ�룬UNUSED ���ͷ���null
		var = GET_OP2_ZVAL_PTR(BP_VAR_W);
		// ���op2����������
		if (Z_ISREF_P(var)) {
			// ���ô���+1
			Z_ADDREF_P(var);
		// ����
		} else {
			// ��zvalתΪ���ã�p1:zval��p2:���ô���
			ZVAL_MAKE_REF_EX(var, 2);
		}
	// ����
	} else {
		// ��ȡzvalָ��, UNUSED ����null
		var = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
		// ���var��δ���� ���� û�� ��ʽ��
		if (UNEXPECTED(Z_ISUNDEF_P(var)) && !(opline->extended_value & ZEND_BIND_IMPLICIT)) {
			// windows: �޲���
			SAVE_OPLINE();
			// ����p2�ı�������������δ����, ����δ��ʼ��zval
			var = ZVAL_UNDEFINED_OP2();
			// ������쳣
			if (UNEXPECTED(EG(exception))) {
				// ��Ҫ�� return
				HANDLE_EXCEPTION();
			}
		}
		// ������
		ZVAL_DEREF(var);
		// �������ô���
		Z_TRY_ADDREF_P(var);
	}
	// ͨ��ƫ�������ⲿ���������£���p1:�հ���p2:ƫ������p3:��ֵ
	// ƫ������ȥ�� �������ã���ʽ�󶨡� �������
	zend_closure_bind_var_ex(closure,
		(opline->extended_value & ~(ZEND_BIND_REF|ZEND_BIND_IMPLICIT)), var);
	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, ���������ڵľ�̬����
ZEND_VM_HANDLER(183, ZEND_BIND_STATIC, CV, UNUSED, REF)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	HashTable *ht;
	zval *value;
	zval *variable_ptr;
	// ��ȡzvalָ��, TMP/CONST/UNUSED ����null����֧�� TMPVAR/TMPVARCV
	variable_ptr = GET_OP1_ZVAL_PTR_PTR_UNDEF(BP_VAR_W);

	// ȡ�þ�̬������ϣ��ָ��
	ht = ZEND_MAP_PTR_GET(EX(func)->op_array.static_variables_ptr);
	// �����ϣ������
	if (!ht) {
		// �����о�̬������ ����һ��
		ht = zend_array_dup(EX(func)->op_array.static_variables);
		// ������ָ��
		ZEND_MAP_PTR_SET(EX(func)->op_array.static_variables_ptr, ht);
	}
	// ���ô���������1
	ZEND_ASSERT(GC_REFCOUNT(ht) == 1);

	// �ҵ� zval λ��
	value = (zval*)((char*)ht->arData + (opline->extended_value & ~(ZEND_BIND_REF|ZEND_BIND_IMPLICIT|ZEND_BIND_EXPLICIT)));

	// windows: �޲���
	SAVE_OPLINE();
	// �����Ҫ������
	if (opline->extended_value & ZEND_BIND_REF) {
		// ���value�ǳ������ʽ
		if (Z_TYPE_P(value) == IS_CONSTANT_AST) {
			// ����value
			if (UNEXPECTED(zval_update_constant_ex(value, EX(func)->op_array.scope) != SUCCESS)) {
				// ��Ҫ�� return
				HANDLE_EXCEPTION();
			}
		}
		// value���ǳ������ʽ
		// ����û�����ô����Ķ��������ô�������ӵ�gc�����������û������ zval����
		i_zval_ptr_dtor(variable_ptr);
		// ��� value�����������ͣ�Ҫ�������³���������
		if (UNEXPECTED(!Z_ISREF_P(value))) {
			// ��������ʵ��
			zend_reference *ref = (zend_reference*)emalloc(sizeof(zend_reference));
			// ���ô���Ϊ2
			GC_SET_REFCOUNT(ref, 2);
			// ����, ֧��GC
			GC_TYPE_INFO(ref) = GC_REFERENCE;
			// ����Ŀ��Ϊ vlaue�ĸ���
			ZVAL_COPY_VALUE(&ref->val, value);
			// ָ��Ϊ��
			ref->sources.ptr = NULL;
			// ����value����������ָ��
			Z_REF_P(value) = ref;
			// ��������Ϊ ��������
			Z_TYPE_INFO_P(value) = IS_REFERENCE_EX;
			// variable_ptr Ҳ���³��������ͣ�ref��Ϊ����ʵ������value��һ��
			ZVAL_REF(variable_ptr, ref);
		// ����value�Ѿ�����������
		} else {
			// ���ô���+1
			Z_ADDREF_P(value);
			// variable_ptr ��value������ͬĿ��
			ZVAL_REF(variable_ptr, Z_REF_P(value));
		}
	// ���򣬲�������
	} else {
		// ����û�����ô����Ķ��������ô�������ӵ�gc�����������û������ zval����
		i_zval_ptr_dtor(variable_ptr);
		// ���ҵ���ֵ���ƹ���
		ZVAL_COPY(variable_ptr, value);
	}

	// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, ȡ�� $this
ZEND_VM_HOT_HANDLER(184, ZEND_FETCH_THIS, UNUSED, UNUSED)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// ��� $this �Ƕ���
	if (EXPECTED(Z_TYPE(EX(This)) == IS_OBJECT)) {
		// ��ִ��������ȡ�� ָ����ŵı���
		zval *result = EX_VAR(opline->result.var);
		// $this �ŵ����ؽ����
		ZVAL_OBJ(result, Z_OBJ(EX(This)));
		// ���ô���+1
		Z_ADDREF_P(result);
		// opline + 1����Ŀ������벢 return
		ZEND_VM_NEXT_OPCODE();
	// ����
	} else {
		// ���÷��� zend_this_not_in_object_context_helper
		ZEND_VM_DISPATCH_TO_HELPER(zend_this_not_in_object_context_helper);
	}
}

// ing2 ,��ȡȫ�ֱ��� $GLOBAL ����������Ԫ��
ZEND_VM_HANDLER(200, ZEND_FETCH_GLOBALS, UNUSED, UNUSED)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// ���ű������������� �� �����Ա��е�������ͬ
	/* For symbol tables we need to deal with exactly the same problems as for property tables. */
	// �ӷ��ű����ҵ� global�����ظ� opline->result.var
	ZVAL_ARR(EX_VAR(opline->result.var),
		zend_proptable_to_symtable(&EG(symbol_table), /* always_duplicate */ 1));
	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}


// ing3, ��($this)���isempty��isset
ZEND_VM_HANDLER(186, ZEND_ISSET_ISEMPTY_THIS, UNUSED, UNUSED)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// ���Ϊ  �� ��������չֵ �� ZEND_ISEMPTY ��^ ��$this�Ƕ���
		// ��ZEND_ISEMPTYʱ$this ���Ƕ��󣬽��Ϊ��
		// ��ZEND_ISEMPTYʱ��$this�Ƕ��󣬽��Ϊ��
	ZVAL_BOOL(EX_VAR(opline->result.var),
		(opline->extended_value & ZEND_ISEMPTY) ^
		 (Z_TYPE(EX(This)) == IS_OBJECT));
	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, ����op1�Ƿ���Ч����Ч�򱨴� 1/1���Ӳ�op1�ı����л�ȡ���� �����ظ� ��������� 
// ZEND_FASTCALL ZEND_MATCH_ERROR_SPEC��2����֧����֤����
ZEND_VM_HANDLER(49, ZEND_CHECK_VAR, CV, UNUSED)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	// ��ִ��������ȡ�� ָ����ŵı���
	zval *op1 = EX_VAR(opline->op1.var);

	// ��� op1 ������δ����
	if (UNEXPECTED(Z_TYPE_INFO_P(op1) == IS_UNDEF)) {
		// windows: �޲���
		SAVE_OPLINE();
		// ����p1�ı�������������δ����, ����δ��ʼ��zval
		ZVAL_UNDEFINED_OP1();
		// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
		ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
	}
	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, ��op1 ʹ�����ø��Ƹ�result
ZEND_VM_HANDLER(140, ZEND_MAKE_REF, VAR|CV, UNUSED)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	// ��ִ��������ȡ�� ָ����ŵı���
	zval *op1 = EX_VAR(opline->op1.var);

	// ��� op1 �� �������
	if (OP1_TYPE == IS_CV) {
		// ���op1��δ����
		if (UNEXPECTED(Z_TYPE_P(op1) == IS_UNDEF)) {
			// op1 ���ø�ֵ���µĿ�����
			// �� zval �����µĿյ� zend_reference 
			ZVAL_NEW_EMPTY_REF(op1);
			// ���ô���2
			Z_SET_REFCOUNT_P(op1, 2);
			// op1 ����Ŀ�� ��ֵ�� null
			ZVAL_NULL(Z_REFVAL_P(op1));
			// ���Ϊ�����õ� op1, ʹ�����ø�ֵ
			ZVAL_REF(EX_VAR(opline->result.var), Z_REF_P(op1));
		// ����, op1���Ǳ������
		} else {
			// ���op1������Ŷ��
			if (Z_ISREF_P(op1)) {
				// ���ô���+1
				Z_ADDREF_P(op1);
			// ����op1������������
			} else {
				// ��zvalתΪ���ã�p1:zval��p2:���ô���
				ZVAL_MAKE_REF_EX(op1, 2);
			}
			// ���Ϊ�����õ� op1, ʹ�����ø�ֵ
			ZVAL_REF(EX_VAR(opline->result.var), Z_REF_P(op1));
		}
	// ���� ��������� �������
	} else if (EXPECTED(Z_TYPE_P(op1) == IS_INDIRECT)) {
		// op1��������
		op1 = Z_INDIRECT_P(op1);
		// ���op1������������
		if (EXPECTED(!Z_ISREF_P(op1))) {
			// ��zvalתΪ���ã�p1:zval��p2:���ô���
			ZVAL_MAKE_REF_EX(op1, 2);
		// ����
		} else {
			// �������ô���
			GC_ADDREF(Z_REF_P(op1));
		}
		// ���Ϊ�����õ� op1, ʹ�����ø�ֵ
		ZVAL_REF(EX_VAR(opline->result.var), Z_REF_P(op1));
	// ���򣬲��Ǳ������Ҳ���Ǽ������
	} else {
		// ��op1���Ƹ�result
		ZVAL_COPY_VALUE(EX_VAR(opline->result.var), op1);
	}
	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, switch ����ת������ case
ZEND_VM_COLD_CONSTCONST_HANDLER(187, ZEND_SWITCH_LONG, CONST|TMPVARCV, CONST, JMP_ADDR)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op, *jump_zv;
	HashTable *jumptable;

	// ��ȡzvalָ��, UNUSED ����null
	op = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);

	// ������Ͳ�������
	if (Z_TYPE_P(op) != IS_LONG) {
		// ������
		ZVAL_DEREF(op);
		// ������Ͳ�������
		if (Z_TYPE_P(op) != IS_LONG) {
			/* Wrong type, fall back to ZEND_CASE chain */
			// opline + 1����Ŀ������벢 return
			ZEND_VM_NEXT_OPCODE();
		}
	}
	// op2����ת��
	// ��ȡzvalָ�룬UNUSED ���ͷ���null
	jumptable = Z_ARRVAL_P(GET_OP2_ZVAL_PTR(BP_VAR_R));
	// ��ȡ��תλ��
	jump_zv = zend_hash_index_find(jumptable, Z_LVAL_P(op));
	// ���λ����Ч
	if (jump_zv != NULL) {
		// �ҵ�Ŀ������룬���óɵ�ǰ�����벢���ء� EX(opline) = (zend_op*)((char*)p1 + p2)
		ZEND_VM_SET_RELATIVE_OPCODE(opline, Z_LVAL_P(jump_zv));
		// windows: return  0
		ZEND_VM_CONTINUE();
	// ����
	} else {
		/* default */
		// �ҵ�Ŀ������룬���óɵ�ǰ�����벢���ء� EX(opline) = (zend_op*)((char*)p1 + p2)
		ZEND_VM_SET_RELATIVE_OPCODE(opline, opline->extended_value);
		// windows: return  0
		ZEND_VM_CONTINUE();
	}
}

// ing3, switch ����ת���ִ� case
ZEND_VM_COLD_CONSTCONST_HANDLER(188, ZEND_SWITCH_STRING, CONST|TMPVARCV, CONST, JMP_ADDR)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op, *jump_zv;
	HashTable *jumptable;

	// ��ȡzvalָ��, UNUSED ����null
	op = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);

	// ���op1�����ִ�
	if (Z_TYPE_P(op) != IS_STRING) {
		// ���op1�ǳ���
		if (OP1_TYPE == IS_CONST) {
			// ���ʹ��󣬷��� ZEND_CASE ��
			/* Wrong type, fall back to ZEND_CASE chain */
			
			// opline + 1����Ŀ������벢 return
			ZEND_VM_NEXT_OPCODE();
		// ��������������
		} else {
			// ������
			ZVAL_DEREF(op);
			// ������Ͳ����ִ�
			if (Z_TYPE_P(op) != IS_STRING) {
				// ���ʹ��󣬷��� ZEND_CASE ��
				/* Wrong type, fall back to ZEND_CASE chain */
				// opline + 1����Ŀ������벢 return
				ZEND_VM_NEXT_OPCODE();
			}
		}
	}

	// ȡ����ת�� op2
	// ��ȡzvalָ�룬UNUSED ���ͷ���null
	jumptable = Z_ARRVAL_P(GET_OP2_ZVAL_PTR(BP_VAR_R));
	// ȡ����תλ��
	jump_zv = zend_hash_find_ex(jumptable, Z_STR_P(op), OP1_TYPE == IS_CONST);
	// �������תλ��
	if (jump_zv != NULL) {
		// �ҵ�Ŀ������룬���óɵ�ǰ�����벢���ء� EX(opline) = (zend_op*)((char*)p1 + p2)
		ZEND_VM_SET_RELATIVE_OPCODE(opline, Z_LVAL_P(jump_zv));
		// windows: return  0
		ZEND_VM_CONTINUE();
	// ����
	} else {
		/* default */
		// �ҵ�Ŀ������룬���óɵ�ǰ�����벢���ء� EX(opline) = (zend_op*)((char*)p1 + p2)
		ZEND_VM_SET_RELATIVE_OPCODE(opline, opline->extended_value);
		// windows: return  0
		ZEND_VM_CONTINUE();
	}
}

// ing3, match���
ZEND_VM_COLD_CONSTCONST_HANDLER(195, ZEND_MATCH, CONST|TMPVARCV, CONST, JMP_ADDR)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op, *jump_zv;
	HashTable *jumptable;

	// ��ȡzvalָ��, UNUSED ����null
	op = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ��ȡzvalָ�룬UNUSED ���ͷ���null
	jumptable = Z_ARRVAL_P(GET_OP2_ZVAL_PTR(BP_VAR_R));

// ������ת��ǩ match_try_again
ZEND_VM_C_LABEL(match_try_again):
	// ��� op1������ 
	if (Z_TYPE_P(op) == IS_LONG) {
		// ȡ����תλ��
		jump_zv = zend_hash_index_find(jumptable, Z_LVAL_P(op));
	// ��� op1���ִ�
	} else if (Z_TYPE_P(op) == IS_STRING) {
		// ȡ����תλ��
		jump_zv = zend_hash_find_ex(jumptable, Z_STR_P(op), OP1_TYPE == IS_CONST);
	// �������������
	} else if (Z_TYPE_P(op) == IS_REFERENCE) {
		// ������
		op = Z_REFVAL_P(op);
		// ��ת��ָ����ǩ match_try_again
		ZEND_VM_C_GOTO(match_try_again);
	// �����������ͣ�����
	} else {
		// ��� op1 �Ǳ������ ���� ��δ����
		if (UNEXPECTED((OP1_TYPE & IS_CV) && Z_TYPE_P(op) == IS_UNDEF)) {
			// windows: �޲���
			SAVE_OPLINE();
			// ����p1�ı�������������δ����, ����δ��ʼ��zval
			op = ZVAL_UNDEFINED_OP1();
			// ������쳣
			if (UNEXPECTED(EG(exception))) {
				// ��Ҫ�� return
				HANDLE_EXCEPTION();
			}
			// ��ת��ָ����ǩ match_try_again
			ZEND_VM_C_GOTO(match_try_again);
		}

		// ��ת��ָ����ǩ default_branch
		ZEND_VM_C_GOTO(default_branch);
	}

	// ��� ����תĿ��
	if (jump_zv != NULL) {
		// �ҵ�Ŀ������룬���óɵ�ǰ�����벢���ء� EX(opline) = (zend_op*)((char*)p1 + p2)
		ZEND_VM_SET_RELATIVE_OPCODE(opline, Z_LVAL_P(jump_zv));
		// windows: return  0
		ZEND_VM_CONTINUE();
	// ����
	} else {
// ������ת��ǩ default_branch
ZEND_VM_C_LABEL(default_branch):
		/* default */
		// �ҵ�Ŀ������룬���óɵ�ǰ�����벢���ء� EX(opline) = (zend_op*)((char*)p1 + p2)
		ZEND_VM_SET_RELATIVE_OPCODE(opline, opline->extended_value);
		// windows: return  0
		ZEND_VM_CONTINUE();
	}
}

//  ing3, match()���ı��� 1/1���Ӳ�op1�ı����л�ȡ���� �����ظ� ��������� 
// ZEND_FASTCALL ZEND_MATCH_ERROR_SPEC��2����֧����֤����
ZEND_VM_COLD_CONST_HANDLER(197, ZEND_MATCH_ERROR, CONST|TMPVARCV, UNUSED)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op;

	// windows: �޲���
	SAVE_OPLINE();
	// ��ȡzvalָ��, UNUSED ����null
	op = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ing4, ���ú������б���
	zend_match_unhandled_error(op);
	// ��Ҫ�� return
	HANDLE_EXCEPTION();
}

// ing3, ��� key�ڹ�ϣ�����Ƿ����
ZEND_VM_COLD_CONSTCONST_HANDLER(189, ZEND_IN_ARRAY, CONST|TMP|VAR|CV, CONST, NUM)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	// 
	zval *op1;
	// op2�Ǳ�������
	// ���� (p2).zv��p1:opline,p2:node
	HashTable *ht = Z_ARRVAL_P(RT_CONSTANT(opline, opline->op2));
	//
	zval *result;

	// ��ȡzvalָ��, UNUSED ����null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	
	// ���1:
	// ��� op1 ������ �ִ�
	if (EXPECTED(Z_TYPE_P(op1) == IS_STRING)) {
		// ��ϣ������ key ����Ԫ�ء�p1:��ϣ��p2:key��p3��key���Ƿ��й�ϣֵ
		result = zend_hash_find_ex(ht, Z_STR_P(op1), OP1_TYPE == IS_CONST);
		// ��� op1 ���� �Ǳ�������ʱ����
		if (OP1_TYPE & (IS_TMP_VAR|IS_VAR)) {
			// ͨ��ָ������ zval���ִ���
			zval_ptr_dtor_str(op1);
		}
		// ���ݵ�ǰ��������ѡ����һ�������룬p1:����ֵ����Ҫ�������жϡ�p2:�Ƿ����쳣
		ZEND_VM_SMART_BRANCH(result, 0);
	}
	
	// ���2�����ֱ�Ӳ��Ҳ��ɹ�����Ҫ�ǿ��ǽ����ã�
	// �������չֵ��
	if (opline->extended_value) {
		// ���op1������ ����
		if (EXPECTED(Z_TYPE_P(op1) == IS_LONG)) {
			// ʹ�ù�ϣֵ����
			result = zend_hash_index_find(ht, Z_LVAL_P(op1));
			// ���ݵ�ǰ��������ѡ����һ�������룬p1:����ֵ����Ҫ�������жϡ�p2:�Ƿ����쳣
			ZEND_VM_SMART_BRANCH(result, 0);
		}
		
		// ����ù�ϣֵҲ�Ҳ���
		// windows: �޲���
		SAVE_OPLINE();
		// ��� op1 ����ͨ������������ ���� op1����������
		if ((OP1_TYPE & (IS_VAR|IS_CV)) && Z_TYPE_P(op1) == IS_REFERENCE) {
			// ������
			op1 = Z_REFVAL_P(op1);
			// ��������� �ִ�
			if (EXPECTED(Z_TYPE_P(op1) == IS_STRING)) {
				// ʹ�� key ��ȡ��ϣ��� bucketֵ��p1:��ϣ��p2:key(zend_string)
				result = zend_hash_find(ht, Z_STR_P(op1));
				// �ͷŲ�������ĸ��ӱ���
				FREE_OP1();
				// ���ݵ�ǰ��������ѡ����һ�������룬p1:����ֵ����Ҫ�������жϡ�p2:�Ƿ����쳣
				ZEND_VM_SMART_BRANCH(result, 0);
			// ���� ��������� ����
			} else if (EXPECTED(Z_TYPE_P(op1) == IS_LONG)) {
				// ����ϣֵ��ѯ������˳�����顣p1:��ϣ��p2:��ϣֵ
				result = zend_hash_index_find(ht, Z_LVAL_P(op1));
				// �ͷŲ�������ĸ��ӱ���
				FREE_OP1();
				// ���ݵ�ǰ��������ѡ����һ�������룬p1:����ֵ����Ҫ�������жϡ�p2:�Ƿ����쳣
				ZEND_VM_SMART_BRANCH(result, 0);
			}
		// ��� op1 �� ������� ���� op1����Ϊδ����
		} else if (OP1_TYPE == IS_CV && UNEXPECTED(Z_TYPE_P(op1) == IS_UNDEF)) {
			// ����p1�ı�������������δ����, ����δ��ʼ��zval
			ZVAL_UNDEFINED_OP1();
		}
	// ���3�����������������ͣ�
	// ���� ��� op1 ������ undef,null,false
	} else if (Z_TYPE_P(op1) <= IS_FALSE) {
		// ��� op1�Ǳ������ ���� ������ δ����
		if (OP1_TYPE == IS_CV && UNEXPECTED(Z_TYPE_P(op1) == IS_UNDEF)) {
			// windows: �޲���
			SAVE_OPLINE();
			// ����p1�ı�������������δ����, ����δ��ʼ��zval
			ZVAL_UNDEFINED_OP1();
			// ������쳣
			if (UNEXPECTED(EG(exception) != NULL)) {
				// ��Ҫ�� return
				HANDLE_EXCEPTION();
			}
		}
		// ʹ�� �й�ϣֵ�ļ�(zend_string) ��ȡ��ϣ��� ֵ
		// keyΪ���ִ�
		result = zend_hash_find_known_hash(ht, ZSTR_EMPTY_ALLOC());
		// ���ݵ�ǰ��������ѡ����һ�������룬p1:����ֵ����Ҫ�������жϡ�p2:�Ƿ����쳣
		ZEND_VM_SMART_BRANCH(result, 0);
	// ���4�����ֱ�Ӳ��Ҳ��ɹ�����Ҫ�ǿ��ǽ����ã���û����չֵ���Ҳ����������͡�ֻ�ܱ�����ϣ����ҡ�
	// �������
	} else {
		zend_string *key;
		zval key_tmp;

		// ���op1�Ǳ��� �� ������� ���� ������ ����
		if ((OP1_TYPE & (IS_VAR|IS_CV)) && Z_TYPE_P(op1) == IS_REFERENCE) {
			// ������
			op1 = Z_REFVAL_P(op1);
			// ��������� �ִ�
			if (EXPECTED(Z_TYPE_P(op1) == IS_STRING)) {
				// ʹ�� key ��ȡ��ϣ��� bucketֵ��p1:��ϣ��p2:key(zend_string)
				result = zend_hash_find(ht, Z_STR_P(op1));
				// �ͷŲ�������ĸ��ӱ���
				FREE_OP1();				
				// ���ݵ�ǰ��������ѡ����һ�������룬p1:����ֵ����Ҫ�������жϡ�p2:�Ƿ����쳣
				ZEND_VM_SMART_BRANCH(result, 0);
			}
		}

		// windows: �޲���
		SAVE_OPLINE();
		// ���� ��ϣ��
		ZEND_HASH_MAP_FOREACH_STR_KEY(ht, key) {
			// key�浽��ʱ������
			ZVAL_STR(&key_tmp, key);
			// ���op1��key��ͬ��
			if (zend_compare(op1, &key_tmp) == 0) {
				// �ͷŲ�������ĸ��ӱ���
				FREE_OP1();
				// ���Ϊ 1
				// ���ݵ�ǰ��������ѡ����һ�������룬p1:����ֵ����Ҫ�������жϡ�p2:�Ƿ����쳣
				ZEND_VM_SMART_BRANCH(1, 1);
			}
		} ZEND_HASH_FOREACH_END();
	}
	// �ͷŲ�������ĸ��ӱ���
	FREE_OP1();
	// ���Ϊ 0
	// ���ݵ�ǰ��������ѡ����һ�������룬p1:����ֵ����Ҫ�������жϡ�p2:�Ƿ����쳣
	ZEND_VM_SMART_BRANCH(0, 1);
}

// ing4, count����
ZEND_VM_COLD_CONST_HANDLER(190, ZEND_COUNT, CONST|TMPVAR|CV, UNUSED)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1;
	zend_long count;

	// windows: �޲���
	SAVE_OPLINE();
	// ��ȡzvalָ��, UNUSED ����null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);

	// û��break��һֱѭ��
	while (1) {
		// ��������� ����
		if (Z_TYPE_P(op1) == IS_ARRAY) {
			// ȡ����ϣ��Ԫ����
			count = zend_hash_num_elements(Z_ARRVAL_P(op1));
			// ����
			break;
		// ���� ��������� ����
		} else if (Z_TYPE_P(op1) == IS_OBJECT) {
			// ȡ������
			zend_object *zobj = Z_OBJ_P(op1);

			// �����ж��Ƿ��Դ�������
			/* first, we check if the handler is defined */
			// ����� count_elements ����
			if (zobj->handlers->count_elements) {
				// ֱ�ӵ��ã�����ɹ�
				if (SUCCESS == zobj->handlers->count_elements(zobj, &count)) {
					// ����
					break;
				}
				// ���ʧ�ܣ������� �쳣
				if (UNEXPECTED(EG(exception))) {
					// ����Ϊ 0 
					count = 0;
					// ����
					break;
				}
			}

			// ��� count_elementsʧ�ܡ����� ��ʵ���� Countable �ӿڵĶ��󣬵���count����
			/* if not and the object implements Countable we call its count() method */
			if (zend_class_implements_interface(zobj->ce, zend_ce_countable)) {
				// ��� ��ʱ����
				zval retval;

				// �ڶ���������ķ����������  count����
				zend_function *count_fn = zend_hash_find_ptr(&zobj->ce->function_table, ZSTR_KNOWN(ZEND_STR_COUNT));
				// ����count����
				zend_call_known_instance_method_with_0_params(count_fn, zobj, &retval);
				// ����ֵת�� ���� 
				count = zval_get_long(&retval);
				// ������ʱ����ֵ
				zval_ptr_dtor(&retval);
				// ����
				break;
			}

			// ���û�д�������Ҳû��ʵ�� Countable�ӿڣ���Ҫ����
			/* If There's no handler and it doesn't implement Countable then emit a TypeError */
		// ���� ��� op1�ǣ���ͨ�������������� ���� ������ ����
		} else if ((OP1_TYPE & (IS_VAR|IS_CV)) != 0 && Z_TYPE_P(op1) == IS_REFERENCE) {
			// ������
			op1 = Z_REFVAL_P(op1);
			// ��ͷ����
			continue;
		// ���� ��� op1�Ǳ������ ���� ������δ����
		} else if (OP1_TYPE == IS_CV && UNEXPECTED(Z_TYPE_P(op1) == IS_UNDEF)) {
			// ����p1�ı�������������δ����, ����δ��ʼ��zval
			ZVAL_UNDEFINED_OP1();
		}
		
		// �������û������ʣ�µľ��Ǵ���
		count = 0;
		// ��������1�����ǿɼ�������� array,��������A����
		zend_type_error("%s(): Argument #1 ($value) must be of type Countable|array, %s given", opline->extended_value ? "sizeof" : "count", zend_zval_type_name(op1));
		// ����
		break;
	}

	// countת������������
	ZVAL_LONG(EX_VAR(opline->result.var), count);
	// �ͷŲ�������ĸ��ӱ���
	FREE_OP1();
	// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3 , ��ȡ������ 1/1���Ӳ�op1�ı����У����ߵ�ǰ�����������У���ȡ���� �����ظ� ��������� 
// ZEND_FASTCALL ZEND_GET_CLASS_SPEC��4����֧����֤����
ZEND_VM_COLD_CONST_HANDLER(191, ZEND_GET_CLASS, UNUSED|CONST|TMPVAR|CV, UNUSED)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	// ��� op1 δʹ��
	if (OP1_TYPE == IS_UNUSED) {
		// �����ǰ��������������
		if (UNEXPECTED(!EX(func)->common.scope)) {
			// windows: �޲���
			SAVE_OPLINE();
			// �����޲�����get_class()ֻ�������е���
			zend_throw_error(NULL, "get_class() without arguments must be called from within a class");
			// ���Ϊδ����
			ZVAL_UNDEF(EX_VAR(opline->result.var));
			// ��Ҫ�� return
			HANDLE_EXCEPTION();
		// ��������ֱ�ӷ���������
		// ����
		} else {
			ZVAL_STR_COPY(EX_VAR(opline->result.var), EX(func)->common.scope->name);
			// opline + 1����Ŀ������벢 return
			ZEND_VM_NEXT_OPCODE();
		}
	// op1��Ч
	// ����
	} else {
		zval *op1;

		// windows: �޲���
		SAVE_OPLINE();
		// ��ȡzvalָ��, UNUSED ����null
		op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
		while (1) {
			// op1�����Ƕ���
			if (Z_TYPE_P(op1) == IS_OBJECT) {
				// ȡ�� zend_object->name ���ظ����
				ZVAL_STR_COPY(EX_VAR(opline->result.var), Z_OBJCE_P(op1)->name);
			// op1 �Ǳ���������������������������
			} else if ((OP1_TYPE & (IS_VAR|IS_CV)) != 0 && Z_TYPE_P(op1) == IS_REFERENCE) {
				// ׷�ٵ����õ�ַ
				op1 = Z_REFVAL_P(op1);
				// 
				continue;
			// �������
			// ����
			} else {
				// ���op1�Ǳ������������ δʹ��
				if (OP1_TYPE == IS_CV && UNEXPECTED(Z_TYPE_P(op1) == IS_UNDEF)) {
					// ����p1�ı�������������δ����, ����δ��ʼ��zval
					ZVAL_UNDEFINED_OP1();
				}
				// �״�
				zend_type_error("get_class(): Argument #1 ($object) must be of type object, %s given", zend_zval_type_name(op1));
				// ���Ϊ δ����
				ZVAL_UNDEF(EX_VAR(opline->result.var));
			}
			// ֻҪ�������ã��ͻ�����
			break;
		}
		// �ͷŲ�������ĸ��ӱ���
		FREE_OP1();
		// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
		ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
	}
}

//  ing3, ��ȡ���ͣ� 1/1���Ӳ�op1�ı����л�ȡ���� �����ظ� ��������� 
// ZEND_FASTCALL ZEND_GET_CALLED_CLASS_SPEC��1����֧����֤����
ZEND_VM_HANDLER(192, ZEND_GET_CALLED_CLASS, UNUSED, UNUSED)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE

	// ����� This �����Ƕ���new���Ķ���
	if (Z_TYPE(EX(This)) == IS_OBJECT) {
		// ���� This���� ������
		ZVAL_STR_COPY(EX_VAR(opline->result.var), Z_OBJCE(EX(This))->name);
	// ���this���Ƕ��󣨾�̬���ã�
	} else if (Z_CE(EX(This))) {
		// ���� ce ������
		ZVAL_STR_COPY(EX_VAR(opline->result.var), Z_CE(EX(This))->name);
	// �������
	} else {
		// ����û��������
		ZEND_ASSERT(!EX(func)->common.scope);
		// # define SAVE_OPLINE() EX(opline) = opline
		SAVE_OPLINE();
		// �״�
		zend_throw_error(NULL, "get_called_class() must be called from within a class");
		// ���Ϊ δ����
		ZVAL_UNDEF(EX_VAR(opline->result.var));
		// �����쳣
		// ��Ҫ�� return
		HANDLE_EXCEPTION();
	}
	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3 , ��ȡ���ͣ� 1/1���Ӳ�op1�ı����л�ȡ���� �����ظ� ��������� 
// ZEND_FASTCALL ZEND_GET_TYPE_SPEC��4����֧����֤����
ZEND_VM_COLD_CONST_HANDLER(193, ZEND_GET_TYPE, CONST|TMP|VAR|CV, UNUSED)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1;
	zend_string *type;

	// windows: �޲���
	SAVE_OPLINE();
	// ��ȡzvalָ��, UNUSED ����null����֧�� TMPVAR/TMPVARCV
	op1 = GET_OP1_ZVAL_PTR_DEREF(BP_VAR_R);
	// �Ӳ�op1�ı����л�ȡ����
	type = zend_zval_get_legacy_type(op1);
	// ����ɹ������ظ����
	if (EXPECTED(type)) {
		// type �����Ǳ�����
		ZVAL_INTERNED_STR(EX_VAR(opline->result.var), type);
	// ���ʧ��
	} else {
		// ���� unknown type
		ZVAL_STRING(EX_VAR(opline->result.var), "unknown type");
	}
	// �ͷŲ�������ĸ��ӱ���
	FREE_OP1();
	// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3 , ��ȡ���������� 1/1���Ӳ������б��л�ȡ���������� ���ظ� ��������� 
// ZEND_FASTCALL ZEND_FUNC_NUM_ARGS_SPEC��1����֧����֤����
ZEND_VM_HANDLER(171, ZEND_FUNC_NUM_ARGS, UNUSED, UNUSED)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	// (call)->This.u2.num_args
	ZVAL_LONG(EX_VAR(opline->result.var), EX_NUM_ARGS());
	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing2, ��ȡ����Ĳ�����op1:��������
ZEND_VM_HANDLER(172, ZEND_FUNC_GET_ARGS, UNUSED|CONST, UNUSED)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zend_array *ht;
	uint32_t arg_count, result_size, skip;

	// ִ�������еĲ������� 
	arg_count = EX_NUM_ARGS();
	// ���op1�ǳ���
	if (OP1_TYPE == IS_CONST) {
		// ��������
		// ���� (p2).zv��p1:opline,p2:node
		skip = Z_LVAL_P(RT_CONSTANT(opline, opline->op1));
		// �����Ҫ�������в���
		if (arg_count < skip) {
			// û��ʣ�µĲ�����Ҫ����
			result_size = 0;
		// ����
		} else {
			// ������ʣ�µĲ�������
			result_size = arg_count - skip;
		}
	// ����op1���ǳ���
	} else {
		// 
		skip = 0;
		// ��Ҫ�������в���
		result_size = arg_count;
	}

	// �������Ҫ��ȡ�Ĳ���
	if (result_size) {
		// ��һ������������
		uint32_t first_extra_arg = EX(func)->op_array.num_args;
		// ���������飬8��Ԫ��
		ht = zend_new_array(result_size);
		// ��ϣ��ŵ� �����
		ZVAL_ARR(EX_VAR(opline->result.var), ht);
		// ��ʼ��������
		zend_hash_real_init_packed(ht);
		// ���ϣ�������Ԫ��
		ZEND_HASH_FILL_PACKED(ht) {
			zval *p, *q;
			uint32_t i = skip;
			// ȡ�ñ���
			p = EX_VAR_NUM(i);
			// ����������� ����������
			if (arg_count > first_extra_arg) {
				// �ȴ���������
				while (i < first_extra_arg) {
					// ��ǰ����
					q = p;
					// ���������Ч
					if (EXPECTED(Z_TYPE_INFO_P(q) != IS_UNDEF)) {
						// �������ô���
						ZVAL_DEREF(q);
						// ����ǿ��Լ�������
						if (Z_OPT_REFCOUNTED_P(q)) {
							// ���ô���+1
							Z_ADDREF_P(q);
						}
						// �������Ԫ�ء�p1:���Ԫ��
						ZEND_HASH_FILL_SET(q);
					// ����
					} else {
						// ֵΪnull
						ZEND_HASH_FILL_SET_NULL();
					}
					// �л��ҵ���һ�����λ�ú�������
					ZEND_HASH_FILL_NEXT();
					// ԭ�б�ָ�����
					p++;
					// ���+1
					i++;
				}
				
				// ���·����С�ڱ��������
				if (skip < first_extra_arg) {
					// ������������
					skip = 0;
				// ����
				} else {
					// ��������ȡ����
					skip -= first_extra_arg;
				}
				// ͨ�����ȡ�ñ���
				p = EX_VAR_NUM(EX(func)->op_array.last_var + EX(func)->op_array.T + skip);
			}
			
			// ��� ����δ����Ĳ���
			while (i < arg_count) {
				// ����
				q = p;
				// ���������Ч
				if (EXPECTED(Z_TYPE_INFO_P(q) != IS_UNDEF)) {
					// �������ô���
					ZVAL_DEREF(q);
					// ����ǿ��Լ�������
					if (Z_OPT_REFCOUNTED_P(q)) {
						// ���ô���+1
						Z_ADDREF_P(q);
					}
					// �������Ԫ�ء�p1:���Ԫ��
					ZEND_HASH_FILL_SET(q);
				// ����
				} else {
					// ֵΪnull
					ZEND_HASH_FILL_SET_NULL();
				}
				// �л��ҵ���һ�����λ�ú�������
				ZEND_HASH_FILL_NEXT();
				// ��һ������
				p++;
				// ���+1
				i++;
			}
		} ZEND_HASH_FILL_END();
		// �����������ЧԪ����
		ht->nNumOfElements = result_size;
	// ����
	} else {
		// ����ֵΪ ������
		ZVAL_EMPTY_ARRAY(EX_VAR(opline->result.var));
	}
	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, ���Ʊ����� 1/1����op1��ֵ���Ƹ������ 
// ZEND_FASTCALL ZEND_COPY_TMP_SPEC��1����֧����֤����
ZEND_VM_HANDLER(167, ZEND_COPY_TMP, TMPVAR, UNUSED)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	// ��ȡzvalָ�룬UNUSED ���ͷ���null
	zval *value = GET_OP1_ZVAL_PTR(BP_VAR_R);
	// ��ִ��������ȡ�� ָ����ŵı���
	zval *result = EX_VAR(opline->result.var);
	// ֱ�Ӱ�op1 ��ֵ���Ƹ����
	ZVAL_COPY(result, value);
	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing2�� �ӱհ��л���ǰһ��ִ�����ݣ�
ZEND_VM_HANDLER(202, ZEND_CALLABLE_CONVERT, UNUSED, UNUSED)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	// ���õ� ������
	zend_execute_data *call = EX(call);

	// ��ִ�����ݴ����ٱհ���p1:����ֵ��p2:ִ������
	zend_closure_from_frame(EX_VAR(opline->result.var), call);

	// ���������Ϣ���� ����Ҫ�ͷ�$this��
	if (ZEND_CALL_INFO(call) & ZEND_CALL_RELEASE_THIS) {
		// �ͷ� $this
		OBJ_RELEASE(Z_OBJ(call->This));
	}

	// ʹ��ǰһ��ִ������
	EX(call) = call->prev_execute_data;
	// �ͷ�call
	// �ͷŵ��ÿ�ܣ�ִ�����ݣ���p1:ִ������
	zend_vm_stack_free_call_frame(call);

	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, ��ת�� 1/1����ת�� op1 ��ָ����λ�á� 
// ZEND_FASTCALL ZEND_JMP_SPEC��1����֧����֤����
ZEND_VM_HOT_TYPE_SPEC_HANDLER(ZEND_JMP, (OP_JMP_ADDR(op, op->op1) > op), ZEND_JMP_FORWARD, JMP_ADDR, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	
	// ��ȡ��תλ�ã����� OPLINE
	// windows��EX(opline)
	// OP_JMP_ADDR: ���� p2.jmp_addr
	OPLINE = OP_JMP_ADDR(opline, opline->op1);
	// �ٵ���continue ���������ת
	// windows: return  0
	ZEND_VM_CONTINUE();
}

// ing3, �ӷ����㣬 2/4��ZEND_ADD_LONG_NO_OVERFLOW������������������ 
// Ҫ��op1,op2,����ֵ�������� , ������򣺲��������������ɽ�����
// ZEND_FASTCALL ZEND_ADD_LONG_NO_OVERFLOW_SPEC��2����֧����֤����
ZEND_VM_HOT_TYPE_SPEC_HANDLER(ZEND_ADD, (res_info == MAY_BE_LONG && op1_info == MAY_BE_LONG && op2_info == MAY_BE_LONG), ZEND_ADD_LONG_NO_OVERFLOW, CONST|TMPVARCV, CONST|TMPVARCV, SPEC(NO_CONST_CONST,COMMUTATIVE))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2, *result;

	// ��ȡzvalָ��, UNUSED ����null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ��ȡzvalָ��, UNUSED ����null
	op2 = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ��ִ��������ȡ�� ָ����ŵı���
	result = EX_VAR(opline->result.var);
	// ��������ֻ�����ﲻһ����ȡ������ֱ�Ӽ�
	ZVAL_LONG(result, Z_LVAL_P(op1) + Z_LVAL_P(op2));
	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, �ӷ����㣬 3/4,ZEND_ADD_LONG�����������
// Ҫ��op1,op2 ��������, ������򣺲��������������ɽ�����
// ZEND_FASTCALL ZEND_ADD_LONG_SPEC��2����֧����֤����
ZEND_VM_HOT_TYPE_SPEC_HANDLER(ZEND_ADD, (op1_info == MAY_BE_LONG && op2_info == MAY_BE_LONG), ZEND_ADD_LONG, CONST|TMPVARCV, CONST|TMPVARCV, SPEC(NO_CONST_CONST,COMMUTATIVE))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2, *result;

	// ��ȡzvalָ��, UNUSED ����null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ��ȡzvalָ��, UNUSED ����null
	op2 = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ��ִ��������ȡ�� ָ����ŵı���
	result = EX_VAR(opline->result.var);
	// �������ټӷ�
	fast_long_add_function(result, op1, op2);
	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, �ӷ����㣬 4/4��ZEND_ADD_DOUBLE�����С����
// Ҫ����������������С��, ������򣺲��������������ɽ�����
// ZEND_FASTCALL ZEND_ADD_DOUBLE_SPEC��2����֧����֤����
ZEND_VM_HOT_TYPE_SPEC_HANDLER(ZEND_ADD, (op1_info == MAY_BE_DOUBLE && op2_info == MAY_BE_DOUBLE), ZEND_ADD_DOUBLE, CONST|TMPVARCV, CONST|TMPVARCV, SPEC(NO_CONST_CONST,COMMUTATIVE))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2, *result;

	// ��ȡzvalָ��, UNUSED ����null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ��ȡzvalָ��, UNUSED ����null
	op2 = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ��ִ��������ȡ�� ָ����ŵı���
	result = EX_VAR(opline->result.var);
	// ȡ��С��ֱ�Ӽ�
	ZVAL_DOUBLE(result, Z_DVAL_P(op1) + Z_DVAL_P(op2));
	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, �������㣬 2/4��ZEND_SUB_LONG_NO_OVERFLOW������������������ 
// Ҫ��op1,op2,����ֵ�������� , ������򣺲�����������
// ZEND_FASTCALL ZEND_SUB_LONG_NO_OVERFLOW_SPEC��3����֧����֤����
ZEND_VM_HOT_TYPE_SPEC_HANDLER(ZEND_SUB, (res_info == MAY_BE_LONG && op1_info == MAY_BE_LONG && op2_info == MAY_BE_LONG), ZEND_SUB_LONG_NO_OVERFLOW, CONST|TMPVARCV, CONST|TMPVARCV, SPEC(NO_CONST_CONST))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2, *result;

	// ��ȡzvalָ��, UNUSED ����null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ��ȡzvalָ��, UNUSED ����null
	op2 = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ��ִ��������ȡ�� ָ����ŵı���
	result = EX_VAR(opline->result.var);
	// ȡ������ֱ�Ӽ�
	ZVAL_LONG(result, Z_LVAL_P(op1) - Z_LVAL_P(op2));
	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, �������㣬 3/4, ZEND_SUB_LONG�����������
// Ҫ��op1,op2 ��������, ������򣺲�����������
// ZEND_FASTCALL ZEND_SUB_LONG_SPEC��3����֧����֤����
ZEND_VM_HOT_TYPE_SPEC_HANDLER(ZEND_SUB, (op1_info == MAY_BE_LONG && op2_info == MAY_BE_LONG), ZEND_SUB_LONG, CONST|TMPVARCV, CONST|TMPVARCV, SPEC(NO_CONST_CONST))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2, *result;

	// ��ȡzvalָ��, UNUSED ����null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ��ȡzvalָ��, UNUSED ����null
	op2 = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ��ִ��������ȡ�� ָ����ŵı���
	result = EX_VAR(opline->result.var);
	// ���ټ���
	fast_long_sub_function(result, op1, op2);
	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, �ӷ����㣬 4/4��ZEND_SUB_DOUBLE�����С����
// Ҫ����������������С��, ������򣺲�����������
// ZEND_FASTCALL ZEND_SUB_DOUBLE_SPEC��3����֧����֤����
ZEND_VM_HOT_TYPE_SPEC_HANDLER(ZEND_SUB, (op1_info == MAY_BE_DOUBLE && op2_info == MAY_BE_DOUBLE), ZEND_SUB_DOUBLE, CONST|TMPVARCV, CONST|TMPVARCV, SPEC(NO_CONST_CONST))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2, *result;

	// ��ȡzvalָ��, UNUSED ����null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ��ȡzvalָ��, UNUSED ����null
	op2 = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ��ִ��������ȡ�� ָ����ŵı���
	result = EX_VAR(opline->result.var);
	// ȡ��С��ֱ�Ӽ�
	ZVAL_DOUBLE(result, Z_DVAL_P(op1) - Z_DVAL_P(op2));
	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, �˷����㣬 2/4��ZEND_MUL_LONG_NO_OVERFLOW������������������ 
// Ҫ��op1,op2,����ֵ�������� , ������򣺲��������������ɽ�����
// ZEND_FASTCALL ZEND_MUL_LONG_NO_OVERFLOW_SPEC��2����֧����֤����
ZEND_VM_HOT_TYPE_SPEC_HANDLER(ZEND_MUL, (res_info == MAY_BE_LONG && op1_info == MAY_BE_LONG && op2_info == MAY_BE_LONG), ZEND_MUL_LONG_NO_OVERFLOW, CONST|TMPVARCV, CONST|TMPVARCV, SPEC(NO_CONST_CONST,COMMUTATIVE))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2, *result;

	// ��ȡzvalָ��, UNUSED ����null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ��ȡzvalָ��, UNUSED ����null
	op2 = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ��ִ��������ȡ�� ָ����ŵı���
	result = EX_VAR(opline->result.var);
	// ȡ������ֱ�ӳ�
	ZVAL_LONG(result, Z_LVAL_P(op1) * Z_LVAL_P(op2));
	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, �˷����㣬 3/4, ZEND_MUL_LONG�����������
// Ҫ��op1,op2 ��������, ������򣺲��������������ɽ�����
// ZEND_FASTCALL ZEND_MUL_LONG_SPEC��2����֧����֤����
ZEND_VM_HOT_TYPE_SPEC_HANDLER(ZEND_MUL, (op1_info == MAY_BE_LONG && op2_info == MAY_BE_LONG), ZEND_MUL_LONG, CONST|TMPVARCV, CONST|TMPVARCV, SPEC(NO_CONST_CONST,COMMUTATIVE))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2, *result;
	zend_long overflow;

	// ��ȡzvalָ��, UNUSED ����null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ��ȡzvalָ��, UNUSED ����null
	op2 = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ��ִ��������ȡ�� ָ����ŵı���
	result = EX_VAR(opline->result.var);
	// �˷�����
	ZEND_SIGNED_MULTIPLY_LONG(Z_LVAL_P(op1), Z_LVAL_P(op2), Z_LVAL_P(result), Z_DVAL_P(result), overflow);
	Z_TYPE_INFO_P(result) = overflow ? IS_DOUBLE : IS_LONG;
	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, �ӷ����㣬 4/4��ZEND_MUL_DOUBLE�����С����
// Ҫ����������������С��, ������򣺲��������������ɽ�����
// ZEND_FASTCALL ZEND_MUL_DOUBLE_SPEC��2����֧����֤����
ZEND_VM_HOT_TYPE_SPEC_HANDLER(ZEND_MUL, (op1_info == MAY_BE_DOUBLE && op2_info == MAY_BE_DOUBLE), ZEND_MUL_DOUBLE, CONST|TMPVARCV, CONST|TMPVARCV, SPEC(NO_CONST_CONST,COMMUTATIVE))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2, *result;

	// ��ȡzvalָ��, UNUSED ����null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ��ȡzvalָ��, UNUSED ����null
	op2 = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ��ִ��������ȡ�� ָ����ŵı���
	result = EX_VAR(opline->result.var);
	// ȡ��С��ֱ�ӳ�
	ZVAL_DOUBLE(result, Z_DVAL_P(op1) * Z_DVAL_P(op2));
	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, ��� �� ��ͬ�� 2/3��ZEND_IS_EQUAL_LONG��������
// Ҫ��������������������, ����������ܷ�֧(*3����֧)�����������������ɽ�����
// ZEND_FASTCALL ZEND_IS_EQUAL_LONG_SPEC��6����֧����֤����
ZEND_VM_HOT_TYPE_SPEC_HANDLER(ZEND_IS_EQUAL|ZEND_IS_IDENTICAL, (op1_info == MAY_BE_LONG && op2_info == MAY_BE_LONG), ZEND_IS_EQUAL_LONG, CONST|TMPVARCV, CONST|TMPVARCV, SPEC(SMART_BRANCH,NO_CONST_CONST,COMMUTATIVE))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2;
	bool result;

	// ��ȡzvalָ��, UNUSED ����null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ��ȡzvalָ��, UNUSED ����null
	op2 = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ȡ������ֱ�ӱȽ�
	result = (Z_LVAL_P(op1) == Z_LVAL_P(op2));
	// ���ݵ�ǰ��������ѡ����һ�������룬p1:����ֵ����Ҫ�������жϡ�p2:�Ƿ����쳣
	ZEND_VM_SMART_BRANCH(result, 0);
}

// ing3, ��� �� ��ͬ�� 3/3��ZEND_IS_EQUAL_DOUBLE��С����
// Ҫ����������������С��, ����������ܷ�֧(*3����֧)�����������������ɽ�����
// ZEND_FASTCALL ZEND_IS_EQUAL_DOUBLE_SPEC��6����֧����֤����
ZEND_VM_HOT_TYPE_SPEC_HANDLER(ZEND_IS_EQUAL|ZEND_IS_IDENTICAL, (op1_info == MAY_BE_DOUBLE && op2_info == MAY_BE_DOUBLE), ZEND_IS_EQUAL_DOUBLE, CONST|TMPVARCV, CONST|TMPVARCV, SPEC(SMART_BRANCH,NO_CONST_CONST,COMMUTATIVE))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2;
	bool result;

	// ��ȡzvalָ��, UNUSED ����null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ��ȡzvalָ��, UNUSED ����null
	op2 = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ȡ��ֱ�ӱȽ�
	result = (Z_DVAL_P(op1) == Z_DVAL_P(op2));
	// ���ݵ�ǰ��������ѡ����һ�������룬p1:����ֵ����Ҫ�������жϡ�p2:�Ƿ����쳣
	ZEND_VM_SMART_BRANCH(result, 0);
}

// ing3, ����� �� ����ͬ�� 2/3��ZEND_IS_NOT_EQUAL_LONG��������
// Ҫ��������������������, ����������ܷ�֧(*3����֧)�����������������ɽ�����
// ZEND_FASTCALL ZEND_IS_NOT_EQUAL_LONG_SPEC��6����֧����֤����
ZEND_VM_HOT_TYPE_SPEC_HANDLER(ZEND_IS_NOT_EQUAL|ZEND_IS_NOT_IDENTICAL, (op1_info == MAY_BE_LONG && op2_info == MAY_BE_LONG), ZEND_IS_NOT_EQUAL_LONG, CONST|TMPVARCV, CONST|TMPVARCV, SPEC(SMART_BRANCH,NO_CONST_CONST,COMMUTATIVE))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2;
	bool result;

	// ��ȡzvalָ��, UNUSED ����null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ��ȡzvalָ��, UNUSED ����null
	op2 = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ȡ������ֱ�ӱȽ�
	result = (Z_LVAL_P(op1) != Z_LVAL_P(op2));
	// ���ݵ�ǰ��������ѡ����һ�������룬p1:����ֵ����Ҫ�������жϡ�p2:�Ƿ����쳣
	ZEND_VM_SMART_BRANCH(result, 0);
}

// ing3, ����� �� ����ͬ�� 3/3��ZEND_IS_NOT_EQUAL_DOUBLE��������
// Ҫ��������������������, ����������ܷ�֧(*3����֧)�����������������ɽ�����
// ZEND_FASTCALL ZEND_IS_NOT_EQUAL_DOUBLE_SPEC��6����֧����֤����
ZEND_VM_HOT_TYPE_SPEC_HANDLER(ZEND_IS_NOT_EQUAL|ZEND_IS_NOT_IDENTICAL, (op1_info == MAY_BE_DOUBLE && op2_info == MAY_BE_DOUBLE), ZEND_IS_NOT_EQUAL_DOUBLE, CONST|TMPVARCV, CONST|TMPVARCV, SPEC(SMART_BRANCH,NO_CONST_CONST,COMMUTATIVE))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2;
	bool result;

	// ��ȡzvalָ��, UNUSED ����null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ��ȡzvalָ��, UNUSED ����null
	op2 = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ȡ��С��ֱ�ӱȽ�
	result = (Z_DVAL_P(op1) != Z_DVAL_P(op2));
	// ���ݵ�ǰ��������ѡ����һ�������룬p1:����ֵ����Ҫ�������жϡ�p2:�Ƿ����쳣
	ZEND_VM_SMART_BRANCH(result, 0);
}

// ing3, ��ͬ�� 4/4��ZEND_IS_IDENTICAL_NOTHROW�����״�
// Ҫ�󣺸���, ������򣺿ɽ�����
// ZEND_FASTCALL ZEND_IS_IDENTICAL_NOTHROW_SPEC��2����֧����֤����
ZEND_VM_TYPE_SPEC_HANDLER(ZEND_IS_IDENTICAL, op->op1_type == IS_CV && (op->op2_type & (IS_CONST|IS_CV)) && !(op1_info & (MAY_BE_UNDEF|MAY_BE_REF)) && !(op2_info & (MAY_BE_UNDEF|MAY_BE_REF)), ZEND_IS_IDENTICAL_NOTHROW, CV, CONST|CV, SPEC(COMMUTATIVE))
{
	/* This is declared below the specializations for MAY_BE_LONG/MAY_BE_DOUBLE so those will be used instead if possible. */
	/* This optimizes $x === SOME_CONST_EXPR and $x === $y for non-refs and non-undef, which can't throw. */
	/* (Infinite recursion when comparing arrays is an uncatchable fatal error) */
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2;
	bool result;

	// ��ȡzvalָ��, UNUSED ����null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ��ȡzvalָ��, UNUSED ����null
	op2 = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ���ú������бȽ�
	result = fast_is_identical_function(op1, op2);
	// const �� cv ����Ҫ free
	/* Free is a no-op for const/cv */
	// ���ݵ�ǰ��������ѡ����һ�������룬p1:����ֵ����Ҫ�������жϡ�p2:�Ƿ����쳣
	ZEND_VM_SMART_BRANCH(result, 0);
}

// ing3, ��ͬ�� 4/4��ZEND_IS_NOT_IDENTICAL_NOTHROW�����״�
// Ҫ�󣺸���, ������򣺿ɽ�����
// ZEND_FASTCALL ZEND_IS_NOT_IDENTICAL_NOTHROW_SPEC��2����֧����֤����
ZEND_VM_TYPE_SPEC_HANDLER(ZEND_IS_NOT_IDENTICAL, op->op1_type == IS_CV && (op->op2_type & (IS_CONST|IS_CV)) && !(op1_info & (MAY_BE_UNDEF|MAY_BE_REF)) && !(op2_info & (MAY_BE_UNDEF|MAY_BE_REF)), ZEND_IS_NOT_IDENTICAL_NOTHROW, CV, CONST|CV, SPEC(COMMUTATIVE))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2;
	bool result;

	// ��ȡzvalָ��, UNUSED ����null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ��ȡzvalָ��, UNUSED ����null
	op2 = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ���ú������бȽ�
	result = fast_is_identical_function(op1, op2);
	// const �� cv ����Ҫ free
	/* Free is a no-op for const/cv */
	
	// ���ݵ�ǰ��������ѡ����һ�������룬p1:����ֵ����Ҫ�������жϡ�p2:�Ƿ����쳣
	ZEND_VM_SMART_BRANCH(!result, 0);
}

// ing3, С�ڣ� 2/3��ZEND_IS_SMALLER_LONG�����������
// Ҫ��op1��op2������ , ����������ܷ�֧(*3����֧)������˫������-1����֧����
// ZEND_FASTCALL ZEND_IS_SMALLER_LONG_SPEC��9����֧����֤����
ZEND_VM_HOT_TYPE_SPEC_HANDLER(ZEND_IS_SMALLER, (op1_info == MAY_BE_LONG && op2_info == MAY_BE_LONG), ZEND_IS_SMALLER_LONG, CONST|TMPVARCV, CONST|TMPVARCV, SPEC(SMART_BRANCH,NO_CONST_CONST))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2;
	bool result;

	// ��ȡzvalָ��, UNUSED ����null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ��ȡzvalָ��, UNUSED ����null
	op2 = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ȡ������ֱ�ӱȽ�
	result = (Z_LVAL_P(op1) < Z_LVAL_P(op2));
	// ���ݵ�ǰ��������ѡ����һ�������룬p1:����ֵ����Ҫ�������жϡ�p2:�Ƿ����쳣
	ZEND_VM_SMART_BRANCH(result, 0);
}

// ing3, С�ڣ� 3/3��ZEND_IS_SMALLER_DOUBLE�����С����
// Ҫ��op1��op2��С�� , ����������ܷ�֧(*3����֧)������˫������-1����֧����
// ZEND_FASTCALL ZEND_IS_SMALLER_DOUBLE_SPEC��9����֧����֤����
ZEND_VM_HOT_TYPE_SPEC_HANDLER(ZEND_IS_SMALLER, (op1_info == MAY_BE_DOUBLE && op2_info == MAY_BE_DOUBLE), ZEND_IS_SMALLER_DOUBLE, CONST|TMPVARCV, CONST|TMPVARCV, SPEC(SMART_BRANCH,NO_CONST_CONST))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2;
	bool result;

	// ��ȡzvalָ��, UNUSED ����null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ��ȡzvalָ��, UNUSED ����null
	op2 = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ȡ��С��ֱ�ӱȽ�
	result = (Z_DVAL_P(op1) < Z_DVAL_P(op2));
	// ���ݵ�ǰ��������ѡ����һ�������룬p1:����ֵ����Ҫ�������жϡ�p2:�Ƿ����쳣
	ZEND_VM_SMART_BRANCH(result, 0);
}

// ing3, С�ڵ��ڣ� 2/3��ZEND_IS_SMALLER_OR_EQUAL_LONG�����������
// Ҫ��op1��op2������ , ����������ܷ�֧(*3����֧)������˫������-1����֧����
// ZEND_FASTCALL ZEND_IS_SMALLER_OR_EQUAL_LONG_SPEC��9����֧����֤����
ZEND_VM_HOT_TYPE_SPEC_HANDLER(ZEND_IS_SMALLER_OR_EQUAL, (op1_info == MAY_BE_LONG && op2_info == MAY_BE_LONG), ZEND_IS_SMALLER_OR_EQUAL_LONG, CONST|TMPVARCV, CONST|TMPVARCV, SPEC(SMART_BRANCH,NO_CONST_CONST))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2;
	bool result;

	// ��ȡzvalָ��, UNUSED ����null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ��ȡzvalָ��, UNUSED ����null
	op2 = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ȡ������ֱ�ӱȽ�
	result = (Z_LVAL_P(op1) <= Z_LVAL_P(op2));
	// ���ݵ�ǰ��������ѡ����һ�������룬p1:����ֵ����Ҫ�������жϡ�p2:�Ƿ����쳣
	ZEND_VM_SMART_BRANCH(result, 0);
}

// ing3, С�ڵ��ڣ� 3/3��ZEND_IS_SMALLER_OR_EQUAL_DOUBLE�����С����
// Ҫ��op1��op2��С�� , ����������ܷ�֧(*3����֧)������˫������-1����֧����
// ZEND_FASTCALL ZEND_IS_SMALLER_OR_EQUAL_DOUBLE_SPEC��9����֧����֤����
ZEND_VM_HOT_TYPE_SPEC_HANDLER(ZEND_IS_SMALLER_OR_EQUAL, (op1_info == MAY_BE_DOUBLE && op2_info == MAY_BE_DOUBLE), ZEND_IS_SMALLER_OR_EQUAL_DOUBLE, CONST|TMPVARCV, CONST|TMPVARCV, SPEC(SMART_BRANCH,NO_CONST_CONST))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *op1, *op2;
	bool result;

	// ��ȡzvalָ��, UNUSED ����null
	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ��ȡzvalָ��, UNUSED ����null
	op2 = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ȡ��С��ֱ�ӱȽ�
	result = (Z_DVAL_P(op1) <= Z_DVAL_P(op2));
	// ���ݵ�ǰ��������ѡ����һ�������룬p1:����ֵ����Ҫ�������жϡ�p2:�Ƿ����쳣
	ZEND_VM_SMART_BRANCH(result, 0);
}

// ing3, �Լӣ�ǰ���� 2/3��ZEND_PRE_INC_LONG_NO_OVERFLOW������������Լ���
// Ҫ�󣺽����������op1������ , ��������з��ؽ����*2��֧������н��ջ��޽��գ���
// ZEND_FASTCALL ZEND_PRE_INC_LONG_NO_OVERFLOW_SPEC��2����֧����֤����
ZEND_VM_HOT_TYPE_SPEC_HANDLER(ZEND_PRE_INC, (res_info == MAY_BE_LONG && op1_info == MAY_BE_LONG), ZEND_PRE_INC_LONG_NO_OVERFLOW, CV, ANY, SPEC(RETVAL))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *var_ptr;
	// ��ȡzvalָ��, TMP/CONST/UNUSED ����null����֧�� TMPVAR/TMPVARCV
	var_ptr = GET_OP1_ZVAL_PTR_PTR_UNDEF(BP_VAR_RW);
	// �ȸ�������ӷ�
	Z_LVAL_P(var_ptr)++;
	// ��� ���������������Ч(����IS_UNUSED)
	if (UNEXPECTED(RETURN_VALUE_USED(opline))) {
		// �ٰ��������Ƹ����
		ZVAL_LONG(EX_VAR(opline->result.var), Z_LVAL_P(var_ptr));
	}
	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, �Լӣ�ǰ���� 3/3��ZEND_PRE_INC_LONG�������Լ���
// Ҫ��op1������ , ��������з��ؽ����*2��֧������н��ջ��޽��գ���
// ZEND_FASTCALL ZEND_PRE_INC_LONG_SPEC��2����֧����֤����
ZEND_VM_HOT_TYPE_SPEC_HANDLER(ZEND_PRE_INC, (op1_info == MAY_BE_LONG), ZEND_PRE_INC_LONG, CV, ANY, SPEC(RETVAL))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *var_ptr;
	// ��ȡzvalָ��, TMP/CONST/UNUSED ����null����֧�� TMPVAR/TMPVARCV
	var_ptr = GET_OP1_ZVAL_PTR_PTR_UNDEF(BP_VAR_RW);
	// �ȸ�������ӷ�
	fast_long_increment_function(var_ptr);
	// ��� ���������������Ч(����IS_UNUSED)
	if (UNEXPECTED(RETURN_VALUE_USED(opline))) {
		// �ٰ��������Ƹ����
		ZVAL_COPY_VALUE(EX_VAR(opline->result.var), var_ptr);
	}
	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, �Լ���ǰ���� 2/3��ZEND_PRE_DEC_LONG_NO_OVERFLOW������������Լ���
// Ҫ�󣺽����������op1������ , ��������з��ؽ����*2��֧������н��ջ��޽��գ���
// ZEND_FASTCALL ZEND_PRE_DEC_LONG_NO_OVERFLOW_SPEC��2����֧����֤����
ZEND_VM_HOT_TYPE_SPEC_HANDLER(ZEND_PRE_DEC, (res_info == MAY_BE_LONG && op1_info == MAY_BE_LONG), ZEND_PRE_DEC_LONG_NO_OVERFLOW, CV, ANY, SPEC(RETVAL))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *var_ptr;
	// ��ȡzvalָ��, TMP/CONST/UNUSED ����null����֧�� TMPVAR/TMPVARCV
	var_ptr = GET_OP1_ZVAL_PTR_PTR_UNDEF(BP_VAR_RW);
	// �ȸ����������
	Z_LVAL_P(var_ptr)--;
	// ��� ���������������Ч(����IS_UNUSED)
	if (UNEXPECTED(RETURN_VALUE_USED(opline))) {
		// �ٰ��������Ƹ���� 
		ZVAL_LONG(EX_VAR(opline->result.var), Z_LVAL_P(var_ptr));
	}
	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, �Լ���ǰ���� 3/3��ZEND_PRE_DEC_LONG�������Լ���
// Ҫ��op1������ , ��������з��ؽ����*2��֧������н��ջ��޽��գ���
// ZEND_FASTCALL ZEND_PRE_DEC_LONG_SPEC��2����֧����֤����
ZEND_VM_HOT_TYPE_SPEC_HANDLER(ZEND_PRE_DEC, (op1_info == MAY_BE_LONG), ZEND_PRE_DEC_LONG, CV, ANY, SPEC(RETVAL))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *var_ptr;
	// ��ȡzvalָ��, TMP/CONST/UNUSED ����null����֧�� TMPVAR/TMPVARCV
	var_ptr = GET_OP1_ZVAL_PTR_PTR_UNDEF(BP_VAR_RW);
	// �ȸ����������
	fast_long_decrement_function(var_ptr);
	// ��� ���������������Ч(����IS_UNUSED)
	if (UNEXPECTED(RETURN_VALUE_USED(opline))) {
		// �ٰ��������Ƹ���� 
		ZVAL_COPY_VALUE(EX_VAR(opline->result.var), var_ptr);
	}
	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, �Լӣ��󣩣� 2/3��ZEND_POST_INC_LONG_NO_OVERFLOW������������Լ���
// Ҫ�󣺽�������� ��op1������ , ��������ޡ�
// ZEND_FASTCALL ZEND_POST_INC_LONG_NO_OVERFLOW_SPEC��1����֧����֤����
ZEND_VM_HOT_TYPE_SPEC_HANDLER(ZEND_POST_INC, (res_info == MAY_BE_LONG && op1_info == MAY_BE_LONG), ZEND_POST_INC_LONG_NO_OVERFLOW, CV, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *var_ptr;
	// ��ȡzvalָ��, TMP/CONST/UNUSED ����null����֧�� TMPVAR/TMPVARCV
	var_ptr = GET_OP1_ZVAL_PTR_PTR_UNDEF(BP_VAR_RW);
	// �Ȱ��������Ƹ���� 
	ZVAL_LONG(EX_VAR(opline->result.var), Z_LVAL_P(var_ptr));
	// �� +1
	Z_LVAL_P(var_ptr)++;
	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, �Լӣ��󣩣� 3/3��ZEND_POST_INC_LONG�������Լ���
// Ҫ��op1������ , ��������ޡ�
// ZEND_FASTCALL ZEND_POST_INC_LONG_SPEC��1����֧����֤����
ZEND_VM_HOT_TYPE_SPEC_HANDLER(ZEND_POST_INC, (op1_info == MAY_BE_LONG), ZEND_POST_INC_LONG, CV, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *var_ptr;
	// ��ȡzvalָ��, TMP/CONST/UNUSED ����null����֧�� TMPVAR/TMPVARCV
	var_ptr = GET_OP1_ZVAL_PTR_PTR_UNDEF(BP_VAR_RW);
	// �Ȱ��������Ƹ���� 
	ZVAL_LONG(EX_VAR(opline->result.var), Z_LVAL_P(var_ptr));
	// �� +1
	fast_long_increment_function(var_ptr);
	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, �Լ����󣩣� 2/3��ZEND_POST_DEC_LONG_NO_OVERFLOW������������Լ���
// Ҫ�󣺽�������� ��op1������ , ��������ޡ�
// ZEND_FASTCALL ZEND_POST_DEC_LONG_NO_OVERFLOW_SPEC��1����֧����֤����
ZEND_VM_HOT_TYPE_SPEC_HANDLER(ZEND_POST_DEC, (res_info == MAY_BE_LONG && op1_info == MAY_BE_LONG), ZEND_POST_DEC_LONG_NO_OVERFLOW, CV, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *var_ptr;
	// ��ȡzvalָ��, TMP/CONST/UNUSED ����null����֧�� TMPVAR/TMPVARCV
	var_ptr = GET_OP1_ZVAL_PTR_PTR_UNDEF(BP_VAR_RW);
	// �Ȱ��������Ƹ����
	ZVAL_LONG(EX_VAR(opline->result.var), Z_LVAL_P(var_ptr));
	// ������ -1
	Z_LVAL_P(var_ptr)--;
	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, �Լ����󣩣� 3/3��ZEND_POST_DEC_LONG�������Լ���
// Ҫ��op1������ , ��������ޡ�
// ZEND_FASTCALL ZEND_POST_DEC_LONG_SPEC��1����֧����֤����
ZEND_VM_HOT_TYPE_SPEC_HANDLER(ZEND_POST_DEC, (op1_info == MAY_BE_LONG), ZEND_POST_DEC_LONG, CV, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *var_ptr;
	// ��ȡzvalָ��, TMP/CONST/UNUSED ����null����֧�� TMPVAR/TMPVARCV
	var_ptr = GET_OP1_ZVAL_PTR_PTR_UNDEF(BP_VAR_RW);
	// �Ȱ��������Ƹ���� ���ֲ��ã�
	ZVAL_LONG(EX_VAR(opline->result.var), Z_LVAL_P(var_ptr));
	// �ٰ����� -1
	fast_long_decrement_function(var_ptr);
	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, ��ֵ�� 2/4��ZEND_QM_ASSIGN_LONG��������ֵ��
// Ҫ��op1������ , ��������ޡ�
// ZEND_FASTCALL ZEND_QM_ASSIGN_LONG_SPEC��2����֧����֤����
ZEND_VM_HOT_TYPE_SPEC_HANDLER(ZEND_QM_ASSIGN, (op1_info == MAY_BE_LONG), ZEND_QM_ASSIGN_LONG, CONST|TMPVARCV, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *value;

	// ��ȡzvalָ��, UNUSED ����null
	value = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ȡ��zval�е���������ֵ���������
	ZVAL_LONG(EX_VAR(opline->result.var), Z_LVAL_P(value));
	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, ��ֵ�� 3/4��ZEND_QM_ASSIGN_DOUBLE��С����ֵ��
// Ҫ��op1��С�� , ��������ޡ�
// ZEND_FASTCALL ZEND_QM_ASSIGN_DOUBLE_SPEC��2����֧����֤����
ZEND_VM_HOT_TYPE_SPEC_HANDLER(ZEND_QM_ASSIGN, (op1_info == MAY_BE_DOUBLE), ZEND_QM_ASSIGN_DOUBLE, CONST|TMPVARCV, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *value;

	// ��ȡzvalָ��, UNUSED ����null
	value = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ȡ��zval�е�С������ֵ���������
	ZVAL_DOUBLE(EX_VAR(opline->result.var), Z_DVAL_P(value));
	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, ��ֵ�� 4/4��ZEND_QM_ASSIGN_NOREF�������ø�ֵ��
// Ҫ�󣺸���, ��������ޡ�
// ZEND_FASTCALL ZEND_QM_ASSIGN_NOREF_SPEC��2����֧����֤����
ZEND_VM_HOT_TYPE_SPEC_HANDLER(ZEND_QM_ASSIGN, ((op->op1_type == IS_CONST) ? !Z_REFCOUNTED_P(RT_CONSTANT(op, op->op1)) : (!(op1_info & ((MAY_BE_ANY|MAY_BE_UNDEF)-(MAY_BE_NULL|MAY_BE_FALSE|MAY_BE_TRUE|MAY_BE_LONG|MAY_BE_DOUBLE))))), ZEND_QM_ASSIGN_NOREF, CONST|TMPVARCV, ANY)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *value;
	// ȡ�õ�һ������������� zval
	// ��ȡzvalָ��, UNUSED ����null
	value = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ֱ�Ӹ��� zval
	ZVAL_COPY_VALUE(EX_VAR(opline->result.var), value);
	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, ������һ����ȡ�������⴦��
ZEND_VM_HOT_TYPE_SPEC_HANDLER(ZEND_FETCH_DIM_R, (!(op2_info & (MAY_BE_UNDEF|MAY_BE_NULL|MAY_BE_STRING|MAY_BE_ARRAY|MAY_BE_OBJECT|MAY_BE_RESOURCE|MAY_BE_REF))), ZEND_FETCH_DIM_R_INDEX, CONST|TMPVAR|CV, CONST|TMPVARCV, SPEC(NO_CONST_CONST))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *container, *dim, *value;
	zend_long offset;
	HashTable *ht;

	// ��ȡzvalָ��, UNUSED ����null
	container = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ��ȡzvalָ��, UNUSED ����null
	dim = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ��������� ����
	if (EXPECTED(Z_TYPE_P(container) == IS_ARRAY)) {
// ������ת��ǩ fetch_dim_r_index_array
ZEND_VM_C_LABEL(fetch_dim_r_index_array):
		// ��� ά�� ������ ����
		if (EXPECTED(Z_TYPE_P(dim) == IS_LONG)) {
			// ת�� ƫ����������
			offset = Z_LVAL_P(dim);
		// ����
		} else {
			// windows: �޲���
			SAVE_OPLINE();
			// �������� ��ά�� ��ȡ
			zend_fetch_dimension_address_read_R(container, dim, OP2_TYPE OPLINE_CC EXECUTE_DATA_CC);
			// �ͷŲ�������ĸ��ӱ���
			FREE_OP1();
			// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
			ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
		}
		// ������˵�� ������ ����ƫ����
		// ��������ȡ����ϣ��
		ht = Z_ARRVAL_P(container);
		// �ڹ�ϣ���в���Ԫ�أ��Ҳ������� fetch_dim_r_index_undef
		ZEND_HASH_INDEX_FIND(ht, offset, value, ZEND_VM_C_LABEL(fetch_dim_r_index_undef));
		
		// ������˵���ҵ���Ŀ��ԭ��
		// ��������Ŀ�� �� ������Ϣ�����������ü���
		ZVAL_COPY_DEREF(EX_VAR(opline->result.var), value);
		// ��� op1 ���� �Ǳ�������ʱ����
		if (OP1_TYPE & (IS_TMP_VAR|IS_VAR)) {
			// windows: �޲���
			SAVE_OPLINE();
			// �ͷŲ�������ĸ��ӱ���
			FREE_OP1();
			// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
			ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
		// ����
		} else {
			// opline + 1����Ŀ������벢 return
			ZEND_VM_NEXT_OPCODE();
		}
	// ������� op1���ǳ��� ���� ���������� ����
	} else if (OP1_TYPE != IS_CONST && EXPECTED(Z_TYPE_P(container) == IS_REFERENCE)) {
		// ������
		container = Z_REFVAL_P(container);
		// ��������� ����
		if (EXPECTED(Z_TYPE_P(container) == IS_ARRAY)) {
			// ��ת��ָ����ǩ fetch_dim_r_index_array
			ZEND_VM_C_GOTO(fetch_dim_r_index_array);
		// ����
		} else {
			// ��ת��ָ����ǩ fetch_dim_r_index_slow
			ZEND_VM_C_GOTO(fetch_dim_r_index_slow);
		}
	// ����
	} else {
// ������ת��ǩ fetch_dim_r_index_slow
ZEND_VM_C_LABEL(fetch_dim_r_index_slow):
		// windows: �޲���
		SAVE_OPLINE();
		// ���op2�ǳ��� ���� ά��Ϊ ����ֵ
		if (OP2_TYPE == IS_CONST && Z_EXTRA_P(dim) == ZEND_EXTRA_VALUE) {
			// �ҵ���һ��zval
			dim++;
		}
		// ����ϣ������ģ�������һ����ȡ,������ص��������У���������Ϊ BP_VAR_R�� 
		// p1:����ȡ������p2:������p3:��������
		zend_fetch_dimension_address_read_R_slow(container, dim OPLINE_CC EXECUTE_DATA_CC);
		// �ͷŲ�������ĸ��ӱ���
		FREE_OP1();
		// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
		ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
	}

// ������ת��ǩ fetch_dim_r_index_undef
ZEND_VM_C_LABEL(fetch_dim_r_index_undef):
	// ���Ϊ null
	ZVAL_NULL(EX_VAR(opline->result.var));
	// windows: �޲���
	SAVE_OPLINE();
	// ����δ���������key��������
	zend_undefined_offset(offset);
	// �ͷŲ�������ĸ��ӱ���
	FREE_OP1();
	// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing3, ��op1���Ƹ�result��������Ǳ����������1������
ZEND_VM_HOT_TYPE_SPEC_HANDLER(ZEND_SEND_VAR, op->op2_type == IS_UNUSED && (op1_info & (MAY_BE_UNDEF|MAY_BE_REF)) == 0, ZEND_SEND_VAR_SIMPLE, CV|VAR, NUM)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *varptr, *arg;

	// ��ȡzvalָ��, UNUSED ����null
	varptr = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ��p1 ����p2�ֽڣ�ת��zval����
	arg = ZEND_CALL_VAR(EX(call), opline->result.var);

	// ��� op1 �� �������
	if (OP1_TYPE == IS_CV) {
		// ���� zval �����͸�����Ϣ
		ZVAL_COPY(arg, varptr);
	// ���� ��� op1 �� ��ͨ����
	} else /* if (OP1_TYPE == IS_VAR) */ {
		// ���� zval �����͸�����Ϣ�����������ô���
		ZVAL_COPY_VALUE(arg, varptr);
	}

	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, op1���Ƹ�result, ��������������ô���
ZEND_VM_HOT_TYPE_SPEC_HANDLER(ZEND_SEND_VAR_EX, op->op2_type == IS_UNUSED && op->op2.num <= MAX_ARG_FLAG_NUM && (op1_info & (MAY_BE_UNDEF|MAY_BE_REF)) == 0, ZEND_SEND_VAR_EX_SIMPLE, CV|VAR, UNUSED|NUM)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *varptr, *arg;
	// �������
	uint32_t arg_num = opline->op2.num;

	// ����������ô���
	if (QUICK_ARG_SHOULD_BE_SENT_BY_REF(EX(call)->func, arg_num)) {
		// ��ת���������ǩ 
		ZEND_VM_DISPATCH_TO_HANDLER(ZEND_SEND_REF);
	}

	// ��ȡzvalָ��, UNUSED ����null
	varptr = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	// ��p1 ����p2�ֽڣ�ת��zval����
	arg = ZEND_CALL_VAR(EX(call), opline->result.var);

	// ��� op1 �� �������
	if (OP1_TYPE == IS_CV) {
		// op1���Ƹ�result, ���������ô���
		ZVAL_COPY(arg, varptr);
	// ���� ��� op1 �� ��ͨ����
	} else /* if (OP1_TYPE == IS_VAR) */ {
		// op1���Ƹ�result
		ZVAL_COPY_VALUE(arg, varptr);
	}

	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, op1 ֱ�Ӹ��Ƹ� result
ZEND_VM_HOT_TYPE_SPEC_HANDLER(ZEND_SEND_VAL, op->op1_type == IS_CONST && op->op2_type == IS_UNUSED && !Z_REFCOUNTED_P(RT_CONSTANT(op, op->op1)), ZEND_SEND_VAL_SIMPLE, CONST, NUM)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *value, *arg;

	// ��ȡzvalָ�룬UNUSED ���ͷ���null
	value = GET_OP1_ZVAL_PTR(BP_VAR_R);
	// ��p1 ����p2�ֽڣ�ת��zval����
	arg = ZEND_CALL_VAR(EX(call), opline->result.var);
	// op1 ֱ�Ӹ��Ƹ� result
	ZVAL_COPY_VALUE(arg, value);
	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, ��op1����result��op2�ǲ�����š�����ǿ������ʱ����
ZEND_VM_HOT_TYPE_SPEC_HANDLER(ZEND_SEND_VAL_EX, op->op2_type == IS_UNUSED && op->op2.num <= MAX_ARG_FLAG_NUM && op->op1_type == IS_CONST && !Z_REFCOUNTED_P(RT_CONSTANT(op, op->op1)), ZEND_SEND_VAL_EX_SIMPLE, CONST, NUM)
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *value, *arg;
	// �������
	uint32_t arg_num = opline->op2.num;

	// ��p1 ����p2�ֽڣ�ת��zval����
	arg = ZEND_CALL_VAR(EX(call), opline->result.var);
	// �������ͨ�����ô���
	if (QUICK_ARG_MUST_BE_SENT_BY_REF(EX(call)->func, arg_num)) {
		// �״��˲���������ʹ�����ô���
		// ���÷��� zend_cannot_pass_by_ref_helper
		ZEND_VM_DISPATCH_TO_HELPER(zend_cannot_pass_by_ref_helper, _arg_num, arg_num, _arg, arg);
	}
	// ��ȡzvalָ�룬UNUSED ���ͷ���null
	value = GET_OP1_ZVAL_PTR(BP_VAR_R);
	// ��op1 ���Ƹ�����
	ZVAL_COPY_VALUE(arg, value);
	// opline + 1����Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE();
}

// ing3, ��ȡ��ʽ���� ����
ZEND_VM_HOT_TYPE_SPEC_HANDLER(ZEND_FE_FETCH_R, op->op2_type == IS_CV && (op1_info & (MAY_BE_ANY|MAY_BE_REF)) == MAY_BE_ARRAY, ZEND_FE_FETCH_R_SIMPLE, VAR, CV, JMP_ADDR, SPEC(RETVAL))
{
	// const zend_op *opline = EX(opline);
	USE_OPLINE
	zval *array;
	zval *value, *variable_ptr;
	uint32_t value_type;
	HashTable *fe_ht;
	HashPosition pos;

	// ��ִ��������ȡ�� ָ����ŵı���
	array = EX_VAR(opline->op1.var);
	// windows: �޲���
	SAVE_OPLINE();
	// ȡ�ñ����Ĺ�ϣ��
	fe_ht = Z_ARRVAL_P(array);
	// ͨ��ָ�����zval�� foreachλ�á�(*p1).u2.fe_pos
	pos = Z_FE_POS_P(array);
	// �����������˳������
	if (HT_IS_PACKED(fe_ht)) {
		// ȡ��ָ��λ�õ�zval
		value = fe_ht->arPacked + pos;
		// �˿�ʼ������������
		while (1) {
			// ���λ�ó�����Чλ��
			if (UNEXPECTED(pos >= fe_ht->nNumUsed)) {
				// �������ĩβ
				/* reached end of iteration */
				// �ҵ�Ŀ������룬���óɵ�ǰ�����벢���ء� EX(opline) = (zend_op*)((char*)p1 + p2)
				ZEND_VM_SET_RELATIVE_OPCODE(opline, opline->extended_value);
				// windows: return  0
				ZEND_VM_CONTINUE();
			}
			// ȡ�� ��ǰֵ����
			value_type = Z_TYPE_INFO_P(value);
			// �������Ǽ������
			ZEND_ASSERT(value_type != IS_INDIRECT);
			// ���Ͳ��� δ����
			if (EXPECTED(value_type != IS_UNDEF)) {
				// ����
				break;
			}
			// ����λ�ú���
			pos++;
			// Ԫ��ָ�����
			value++;
		}
		// ���������ڲ�ָ��
		// ͨ��ָ�����zval�� foreachλ�á�(*p1).u2.fe_pos
		Z_FE_POS_P(array) = pos + 1;
		// ��� ���������������Ч(����IS_UNUSED)
		if (RETURN_VALUE_USED(opline)) {
			// λ�� �ŵ������
			ZVAL_LONG(EX_VAR(opline->result.var), pos);
		}
	// ��������������ǹ�ϣ��
	} else {
		Bucket *p;
		// �ҵ�ָ���� bucket
		p = fe_ht->arData + pos;
		// ������ȫ��Ԫ��
		while (1) {
			// ����Ѿ�����Ԫ���б�ĩβ
			if (UNEXPECTED(pos >= fe_ht->nNumUsed)) {
				// �������ĩβ
				/* reached end of iteration */
				// �ҵ�Ŀ������룬���óɵ�ǰ�����벢���ء� EX(opline) = (zend_op*)((char*)p1 + p2)
				ZEND_VM_SET_RELATIVE_OPCODE(opline, opline->extended_value);
				// windows: return  0
				ZEND_VM_CONTINUE();
			}
			// λ�ú���
			pos++;
			// bucket ֵ
			value = &p->val;
			// ֵ����
			value_type = Z_TYPE_INFO_P(value);
			// �������Ǽ������
			ZEND_ASSERT(value_type != IS_INDIRECT);
			// ���Ͳ��� δ����
			if (EXPECTED(value_type != IS_UNDEF)) {
				// ����
				break;
			}
			// ��һ��bucket 
			p++;
		}
		// ���¹�ϣ���ڲ�ָ��
		// ͨ��ָ�����zval�� foreachλ�á�(*p1).u2.fe_pos
		Z_FE_POS_P(array) = pos;
		// ��� ���������������Ч(����IS_UNUSED)
		if (RETURN_VALUE_USED(opline)) {
			// ���û��key
			if (!p->key) {
				// ��ϣֵ��Ϊ���
				ZVAL_LONG(EX_VAR(opline->result.var), p->h);
			// ����
			} else {
				// �ִ����Ƶ������
				ZVAL_STR_COPY(EX_VAR(opline->result.var), p->key);
			}
		}
	}

	// ֵ������op2��
	// ��ִ��������ȡ�� ָ����ŵı���
	variable_ptr = EX_VAR(opline->op2.var);
	// ��������ֵ��p1:������p2:ֵ��p3:ֵ�ı������ͣ������������������������p4:�Ƿ��ϸ�����
	// ĳִ���������������� �Ĳ����Ƿ�ʹ���ϸ���������
	zend_assign_to_variable(variable_ptr, value, IS_CV, EX_USES_STRICT_TYPES());
	// �� EX(opline)����ƫ��1��λ�ã��ҵ�Ŀ������벢 return
	ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
}

// ing2, zend_vm_gen.php����
ZEND_VM_DEFINE_OP(137, ZEND_OP_DATA);

// ing3����鳬ʱ���ҵ��� zend_interrupt_function ����
ZEND_VM_HELPER(zend_interrupt_helper, ANY, ANY)
{
	// EG(vm_interrupt) = false
	zend_atomic_bool_store_ex(&EG(vm_interrupt), false);
	// windows: �޲���
	SAVE_OPLINE();
	// if(EG(timed_out))
	if (zend_atomic_bool_load_ex(&EG(timed_out))) {
		// ��ʱ
		zend_timeout();
	// ���û��ʱ�������� zend_interrupt_function����
	} else if (zend_interrupt_function) {
		// �����������
		// windows : php_win32_signal_ctrl_interrupt_function
		zend_interrupt_function(execute_data);
		// ������쳣
		if (EG(exception)) {
			// ����ѽ�����ó� δ���壬��Ϊ ZEND_HANDLE_EXCEPTION ���ͷ���
			/* We have to UNDEF result, because ZEND_HANDLE_EXCEPTION is going to free it */
			// ���쳣�Ĳ�����
			const zend_op *throw_op = EG(opline_before_exception);
			// ������������ ���� ��� �ǣ���ʱ���� �� ������
			if (throw_op
			 && throw_op->result_type & (IS_TMP_VAR|IS_VAR)
			// ���Ҳ����벻�� ZEND_ADD_ARRAY_ELEMENT��ZEND_ADD_ARRAY_UNPACK��ZEND_ROPE_INIT��ZEND_ROPE_ADD
			 && throw_op->opcode != ZEND_ADD_ARRAY_ELEMENT
			 && throw_op->opcode != ZEND_ADD_ARRAY_UNPACK
			 && throw_op->opcode != ZEND_ROPE_INIT
			 && throw_op->opcode != ZEND_ROPE_ADD) {
				// ��p1 ����p2�ֽڣ�ת��zval����  ((zval*)((char*)(p1) + p2))
				ZVAL_UNDEF(ZEND_CALL_VAR(EG(current_execute_data), throw_op->result.var));

			}
		}
		// ing3, windows return 1;
		ZEND_VM_ENTER();
	}
	// windows: return  0
	ZEND_VM_CONTINUE();
}
