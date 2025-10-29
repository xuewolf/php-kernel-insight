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
   | Authors: Dmitry Stogov <dmitry@php.net>                              |
   +----------------------------------------------------------------------+
*/

#include "zend_sort.h"

#define VM_TRACE(op)     zend_vm_trace(execute_data, opline);
#define VM_TRACE_START() zend_vm_trace_init();
#define VM_TRACE_END()   zend_vm_trace_finish();

static FILE *vm_trace_file;

// ing3, 跟踪一个操作码（棒！），这么简单就可能跟踪操作执行过程了
static void zend_vm_trace(const zend_execute_data *execute_data, const zend_op *opline)
{
	// 如果有执行函数 且 有文件名
	if (EX(func) && EX(func)->op_array.filename) {
		// 打印文件名：行号
		fprintf(vm_trace_file, "%s:%d\n", ZSTR_VAL(EX(func)->op_array.filename), opline->lineno);
	}
}

// ing4, 跟踪结束
static void zend_vm_trace_finish(void)
{
	// 关闭日志文件
	fclose(vm_trace_file);
}

// ing4, 初始化虚拟机跟踪
static void zend_vm_trace_init(void)
{
	// 日志目录 zend_vm_trace.log
	vm_trace_file = fopen("zend_vm_trace.log", "w+");
}
