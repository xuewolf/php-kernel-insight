#!/usr/bin/env php
<?php
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

// 加过注释以后尝试生成c文件无误
/*
一共31个函数 
ing4, function out($f, $s) 
ing4, function out_line($f) 
ing4, function is_hot_helper($name) 
ing3, function helper_name($name, $spec, $op1, $op2, $extra_spec) 
ing3, function opcode_name($name, $spec, $op1, $op2, $extra_spec) 
clear, function format_condition($condition) 
ing1, function gen_code($f, $spec, $kind, $code, $op1, $op2, $name, $extra_spec=null) 
ing4, function skip_extra_spec_function($op1, $op2, $extra_spec) 
ing3, function is_hot_handler($hot, $op1, $op2, $extra_spec) 
ing3, function is_cold_handler($hot, $op1, $op2, $extra_spec) 
clear, function is_inline_hybrid_handler($name, $hot, $op1, $op2, $extra_spec) 
ing2, function gen_handler($f, $spec, $kind, $name, $op1, $op2, $use, $code, $lineno, $opcode, $extra_spec = null, &$switch_labels = array()) 
ing3, function gen_helper($f, $spec, $kind, $name, $op1, $op2, $param, $code, $lineno, $inline, $cold = false, $hot = false, $extra_spec = null) 
clear, function gen_null_label($f, $kind, $prolog) 
ing3, function gen_labels($f, $spec, $kind, $prolog, &$specs, $switch_labels = array()) 
ing4, function gen_specs($f, $prolog, $specs) 
ing4, function gen_null_handler($f) 
ing4, function extra_spec_name($extra_spec) 
ing4, function extra_spec_flags($extra_spec) 
ing3, function extra_spec_handler($dsc) 
ing4, function read_order_file($fn) 
ing2, function gen_executor_code($f, $spec, $kind, $prolog, &$switch_labels = array()) 
clear，function skip_blanks($f, $prolog, $epilog) 
function gen_executor($f, $skl, $spec, $kind, $executor_name, $initializer_name) 
ing4, function parse_operand_spec($def, $lineno, $str, &$flags) 
ing4, function parse_ext_spec($def, $lineno, $str) 
ing4, function parse_spec_rules($def, $lineno, $str) 
ing1, function gen_vm_opcodes_header
function gen_vm($def, $skel) 
clear，function write_file_if_changed(string $filename, string $contents) 
ing4，function usage() 

*/

// 文件头内容
const HEADER_TEXT = <<< DATA
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


DATA;
// 使用 已有的 zend_vm_def.h 和 zend_vm_execute.skl 创建 zend_vm_execute.h 和 zend_vm_opcodes.{h,c}
// 所以 zend_vm_opcodes 并不是提前定义好，而是后面才生成 zend_vm_def.h 才是重头戏
/*
    This script creates zend_vm_execute.h and zend_vm_opcodes.{h,c}
    from existing zend_vm_def.h and zend_vm_execute.skl
*/

error_reporting(E_ALL);

const ZEND_VM_KIND_CALL   = 1;
const ZEND_VM_KIND_SWITCH = 2;
const ZEND_VM_KIND_GOTO   = 3;
const ZEND_VM_KIND_HYBRID = 4;

// 虚拟机操作标记，数组 gen_vm_opcodes_header，gen_vm 用到
$vm_op_flags = array(
    "ZEND_VM_OP_SPEC"         => 1<<0,
	// 常量
    "ZEND_VM_OP_CONST"        => 1<<1,
	// 临时变量
    "ZEND_VM_OP_TMPVAR"       => 1<<2,
	// 编译的临时变量
    "ZEND_VM_OP_TMPVARCV"     => 1<<3,
	
	// 
    "ZEND_VM_OP_MASK"         => 0xf0,
	//
    "ZEND_VM_OP_NUM"          => 0x10,
	//
    "ZEND_VM_OP_JMP_ADDR"     => 0x20,
    "ZEND_VM_OP_TRY_CATCH"    => 0x30,
    // unused 0x40
    "ZEND_VM_OP_THIS"         => 0x50,
    "ZEND_VM_OP_NEXT"         => 0x60,
    "ZEND_VM_OP_CLASS_FETCH"  => 0x70,
    "ZEND_VM_OP_CONSTRUCTOR"  => 0x80,
    "ZEND_VM_OP_CONST_FETCH"  => 0x90,
    "ZEND_VM_OP_CACHE_SLOT"   => 0xa0,

	// 
    "ZEND_VM_EXT_VAR_FETCH"   => 1<<16,
    "ZEND_VM_EXT_ISSET"       => 1<<17,
    "ZEND_VM_EXT_CACHE_SLOT"  => 1<<18,
    "ZEND_VM_EXT_ARRAY_INIT"  => 1<<19,
    "ZEND_VM_EXT_REF"         => 1<<20,
    "ZEND_VM_EXT_FETCH_REF"   => 1<<21,
    "ZEND_VM_EXT_DIM_WRITE"    => 1<<22,
	// 
    "ZEND_VM_EXT_MASK"        => 0x0f000000,
    "ZEND_VM_EXT_NUM"         => 0x01000000,
    "ZEND_VM_EXT_LAST_CATCH"  => 0x02000000,
    "ZEND_VM_EXT_JMP_ADDR"    => 0x03000000,
    "ZEND_VM_EXT_OP"          => 0x04000000,
    // unused 0x5000000
    // unused 0x6000000
    //
	"ZEND_VM_EXT_TYPE"        => 0x07000000,
    "ZEND_VM_EXT_EVAL"        => 0x08000000,
    "ZEND_VM_EXT_TYPE_MASK"   => 0x09000000,
    // unused 0x0a000000,
    "ZEND_VM_EXT_SRC"         => 0x0b000000,
    // unused 0x0c000000,
    "ZEND_VM_NO_CONST_CONST"  => 0x40000000,
    "ZEND_VM_COMMUTATIVE"     => 0x80000000,
);

foreach ($vm_op_flags as $name => $val) {
    define($name, $val);
}

// 虚拟机操作解码，parse_operand_spec 用到
$vm_op_decode = array(
    "ANY"                  => 0,
    "CONST"                => ZEND_VM_OP_SPEC | ZEND_VM_OP_CONST,
    "TMP"                  => ZEND_VM_OP_SPEC,
    "VAR"                  => ZEND_VM_OP_SPEC,
    "UNUSED"               => ZEND_VM_OP_SPEC,
    "CV"                   => ZEND_VM_OP_SPEC,
    "TMPVAR"               => ZEND_VM_OP_SPEC | ZEND_VM_OP_TMPVAR,
    "TMPVARCV"             => ZEND_VM_OP_SPEC | ZEND_VM_OP_TMPVARCV,
    "NUM"                  => ZEND_VM_OP_NUM,
    "JMP_ADDR"             => ZEND_VM_OP_JMP_ADDR,
    "TRY_CATCH"            => ZEND_VM_OP_TRY_CATCH,
    "THIS"                 => ZEND_VM_OP_THIS,
    "NEXT"                 => ZEND_VM_OP_NEXT,
    "CLASS_FETCH"          => ZEND_VM_OP_CLASS_FETCH,
    "CONSTRUCTOR"          => ZEND_VM_OP_CONSTRUCTOR,
    "CONST_FETCH"          => ZEND_VM_OP_CONST_FETCH,
    "CACHE_SLOT"           => ZEND_VM_OP_CACHE_SLOT,
);

// 虚拟机扩展解码：parse_ext_spec
$vm_ext_decode = array(
    "NUM"                  => ZEND_VM_EXT_NUM,
    "LAST_CATCH"           => ZEND_VM_EXT_LAST_CATCH,
    "JMP_ADDR"             => ZEND_VM_EXT_JMP_ADDR,
    "OP"                   => ZEND_VM_EXT_OP,
    "VAR_FETCH"            => ZEND_VM_EXT_VAR_FETCH,
    "ARRAY_INIT"           => ZEND_VM_EXT_ARRAY_INIT,
    "TYPE"                 => ZEND_VM_EXT_TYPE,
    "EVAL"                 => ZEND_VM_EXT_EVAL,
    "TYPE_MASK"            => ZEND_VM_EXT_TYPE_MASK,
    "ISSET"                => ZEND_VM_EXT_ISSET,
    "REF"                  => ZEND_VM_EXT_REF,
    "FETCH_REF"            => ZEND_VM_EXT_FETCH_REF,
    "SRC"                  => ZEND_VM_EXT_SRC,
    "CACHE_SLOT"           => ZEND_VM_EXT_CACHE_SLOT,
    "DIM_WRITE"            => ZEND_VM_EXT_DIM_WRITE,
);

// 运行模式名称，此数组没有用到
$vm_kind_name = array(
    ZEND_VM_KIND_CALL      => "ZEND_VM_KIND_CALL",
    ZEND_VM_KIND_SWITCH    => "ZEND_VM_KIND_SWITCH",
    ZEND_VM_KIND_GOTO      => "ZEND_VM_KIND_GOTO",
    ZEND_VM_KIND_HYBRID    => "ZEND_VM_KIND_HYBRID",
);

// 操作对象类型： gen_labels。 gen_vm 里只声名，没用到它 
// 在创建最终规则时，从 op_types 到 op_types_ex 有一个转换过程，op_types 不是最终使用的规则，op_types_ex才是 。
// 参看 gen_labels() -> $foreach_op1
$op_types = array(
    "ANY",
    "CONST",
    "TMP",
    "VAR",
    "UNUSED",
    "CV",
);

// 这两个类型分别是干什么用的 ？ op_types_ex 在 zend_vm_def.h 中大量用到
// 操作对象类型（包含扩展类型）：gen_labels，extra_spec_handler，gen_executor_code
$op_types_ex = array(
    "ANY",
    "CONST",
    "TMPVARCV", // 这个上面没有
    "TMPVAR", // 这个上面没有
    "TMP",
    "VAR",
    "UNUSED",
    "CV",
);

// 前缀，用的地方比较多：helper_name，opcode_name，gen_code，gen_handler，gen_helper，gen_labels，extra_spec_name
$prefix = array(
    "ANY"      => "",
    "TMP"      => "_TMP",
    "VAR"      => "_VAR",
    "CONST"    => "_CONST",
    "UNUSED"   => "_UNUSED",
    "CV"       => "_CV",
    "TMPVAR"   => "_TMPVAR",
    "TMPVARCV" => "_TMPVARCV",
);

// $op_types_ex的 交替顺序： skip_extra_spec_function，gen_labels 里用到
$commutative_order = array(
    "ANY"      => 0,
    "TMP"      => 1,
    "VAR"      => 2,
    "CONST"    => 0,
    "UNUSED"   => 0,
    "CV"       => 4,
    "TMPVAR"   => 2,
    "TMPVARCV" => 4,
);

// 以下这【32个】变量都只用在 gen_code 中，生成代码时使用 ---------------------------

// ing3, 第一个操作对象的类型，对应的C语言表示 
$op1_type = array(
	// 任何类型
    "ANY"      => "opline->op1_type",
    "TMP"      => "IS_TMP_VAR",
    "VAR"      => "IS_VAR",
    "CONST"    => "IS_CONST",
    "UNUSED"   => "IS_UNUSED",
    "CV"       => "IS_CV",
    "TMPVAR"   => "(IS_TMP_VAR|IS_VAR)",
    "TMPVARCV" => "(IS_TMP_VAR|IS_VAR|IS_CV)",
);

// ing3, 第二个操作对象的类型，对应的C语言表示 
$op2_type = array(
    "ANY"      => "opline->op2_type",
    "TMP"      => "IS_TMP_VAR",
    "VAR"      => "IS_VAR",
    "CONST"    => "IS_CONST",
    "UNUSED"   => "IS_UNUSED",
    "CV"       => "IS_CV",
	// 临时变量
    "TMPVAR"   => "(IS_TMP_VAR|IS_VAR)",
    "TMPVARCV" => "(IS_TMP_VAR|IS_VAR|IS_CV)",
);

// ing3, ANY/UNUSED:null, TMP:zval* ASSERT 非引用, VAR/CV/TMPVAR:zval*,CONST:(p2).zv, TMPVARCV:调用错误
$op1_get_zval_ptr = array(
	// 在执行数据中通过 p1:运算对象类型，p3:操作类型，取回p2:运算对象 指向的zval（编译变量有附加处理）
    "ANY"      => "get_zval_ptr(opline->op1_type, opline->op1, \\1)",
	// 通过偏移量获取 执行数据 中的 第n个 zval, 不可以是引用类型。p1:变量序号
    "TMP"      => "_get_zval_ptr_tmp(opline->op1.var EXECUTE_DATA_CC)",
	// 通过偏移量获取 执行数据 中的 第n个 zval。p1:变量序号
    "VAR"      => "_get_zval_ptr_var(opline->op1.var EXECUTE_DATA_CC)",
	// 访问 (p2).zv。p1:opline,p2:node
    "CONST"    => "RT_CONSTANT(opline, opline->op1)",
	// null
    "UNUSED"   => "NULL",
	// 从执行数据中，取回指定编号的变量。 （这是一系列的函数 ，也在zend_execute.c里）
    "CV"       => "_get_zval_ptr_cv_\\1(opline->op1.var EXECUTE_DATA_CC)",
	// 通过偏移量获取 执行数据 中的 第n个 zval。p1:变量序号
    "TMPVAR"   => "_get_zval_ptr_var(opline->op1.var EXECUTE_DATA_CC)",
	// 没有同时处理3种类型的方法
    "TMPVARCV" => "???",
);

// ing3, ANY/UNUSED:null, TMP:zval* ASSERT 非引用, VAR/CV/TMPVAR:zval*,CONST:(p2).zv, TMPVARCV:调用错误
$op2_get_zval_ptr = array(
	// 在执行数据中通过 p1:运算对象类型，p3:操作类型，取回p2:运算对象 指向的zval（编译变量有附加处理）
    "ANY"      => "get_zval_ptr(opline->op2_type, opline->op2, \\1)",
    // 通过偏移量获取 执行数据 中的 第n个 zval, 不可以是引用类型。p1:变量序号
    "TMP"      => "_get_zval_ptr_tmp(opline->op2.var EXECUTE_DATA_CC)",
    // 通过偏移量获取 执行数据 中的 第n个 zval。p1:变量序号
    "VAR"      => "_get_zval_ptr_var(opline->op2.var EXECUTE_DATA_CC)",
    // 访问 (p2).zv。p1:opline,p2:node
    "CONST"    => "RT_CONSTANT(opline, opline->op2)",
    // null
    "UNUSED"   => "NULL",
    // 从执行数据中，取回指定编号的变量。 （这是一系列的函数 ，也在zend_execute.c里）
    "CV"       => "_get_zval_ptr_cv_\\1(opline->op2.var EXECUTE_DATA_CC)",
    // 通过偏移量获取 执行数据 中的 第n个 zval。p1:变量序号
    "TMPVAR"   => "_get_zval_ptr_var(opline->op2.var EXECUTE_DATA_CC)",
	// 没有同时处理3种类型的方法
    "TMPVARCV" => "???",
);

// ing3, ANY:ZEND_ASSERT 不支持, TMP/CONST/UNUSED:null, VAR:zval*，解间接引用, TMPVAR/TMPVARCV:调用错误
$op1_get_zval_ptr_ptr = array(
	// 在执行数据中通过 p1:运算对象类型，p3:操作类型，取回p2:运算对象 指向的zval
	// 编译变量带附加处理，普通变量解间接引用
    "ANY"      => "get_zval_ptr_ptr(opline->op1_type, opline->op1, \\1)",
	// null
    "TMP"      => "NULL",
	// 在执行数据中通过 p1:序号 取回zval（解间接引用）
    "VAR"      => "_get_zval_ptr_ptr_var(opline->op1.var EXECUTE_DATA_CC)",
	// null
    "CONST"    => "NULL",
	// null
    "UNUSED"   => "NULL",
	// 从执行数据中，取回指定编号的变量。 （这是一系列的函数 ，也在zend_execute.c里）
    "CV"       => "_get_zval_ptr_cv_\\1(opline->op1.var EXECUTE_DATA_CC)",
    "TMPVAR"   => "???",
    "TMPVARCV" => "???",
);

// ing3, ANY:ZEND_ASSERT 不支持, TMP/CONST/UNUSED:null, VAR:zval*，解间接引用, TMPVAR/TMPVARCV:调用错误
$op2_get_zval_ptr_ptr = array(
	// 在执行数据中通过 p1:运算对象类型，p3:操作类型，取回p2:运算对象 指向的zval
	// 编译变量带附加处理，普通变量解间接引用
    "ANY"      => "get_zval_ptr_ptr(opline->op2_type, opline->op2, \\1)",
	// null
    "TMP"      => "NULL",
	// 在执行数据中通过 p1:序号 取回zval（解间接引用）
    "VAR"      => "_get_zval_ptr_ptr_var(opline->op2.var EXECUTE_DATA_CC)",
	// null
    "CONST"    => "NULL",
	// null
    "UNUSED"   => "NULL",
	// 从执行数据中，取回指定编号的变量。 （这是一系列的函数 ，也在zend_execute.c里）
    "CV"       => "_get_zval_ptr_cv_\\1(opline->op2.var EXECUTE_DATA_CC)",
    "TMPVAR"   => "???",
    "TMPVARCV" => "???",
);

// ing3, ANY/UNUSED:null, TMP:zval* ASSERT 非引用, VAR/CV:zval* 解引用,CONST:(p2).zv, TMPVAR/TMPVARCV:调用错误
$op1_get_zval_ptr_deref = array(
	// 在执行数据中通过 p1:运算对象类型，p3:操作类型，取回p2:运算对象指向的zval（解引用）
    "ANY"      => "get_zval_ptr_deref(opline->op1_type, opline->op1, \\1)",
	// 通过偏移量获取 执行数据 中的 第n个 zval, 不可以是引用类型
    "TMP"      => "_get_zval_ptr_tmp(opline->op1.var EXECUTE_DATA_CC)",
	// 通过偏移量获取 执行数据 中的 第n个 zval，减少引用次数后返回
    "VAR"      => "_get_zval_ptr_var_deref(opline->op1.var EXECUTE_DATA_CC)",
	// 访问 (p2).zv。p1:opline,p2:node
    "CONST"    => "RT_CONSTANT(opline, opline->op1)",
	// null
    "UNUSED"   => "NULL",
	// 从执行数据中，取回指定编号的变量（解引用）
    "CV"       => "_get_zval_ptr_cv_deref_\\1(opline->op1.var EXECUTE_DATA_CC)",
    "TMPVAR"   => "???",
    "TMPVARCV" => "???",
);

// ing3, ANY/UNUSED:null, TMP:zval* ASSERT 非引用, VAR/CV:zval* 解引用,CONST:(p2).zv, TMPVAR/TMPVARCV:调用错误
$op2_get_zval_ptr_deref = array(
	// 在执行数据中通过 p1:运算对象类型，p3:操作类型，取回p2:运算对象指向的zval（解引用）
    "ANY"      => "get_zval_ptr_deref(opline->op2_type, opline->op2, \\1)",
	// 通过偏移量获取 执行数据 中的 第n个 zval, 不可以是引用类型
    "TMP"      => "_get_zval_ptr_tmp(opline->op2.var EXECUTE_DATA_CC)",
	// 通过偏移量获取 执行数据 中的 第n个 zval，减少引用次数后返回
    "VAR"      => "_get_zval_ptr_var_deref(opline->op2.var EXECUTE_DATA_CC)",
	// 访问 (p2).zv。p1:opline,p2:node
    "CONST"    => "RT_CONSTANT(opline, opline->op2)",
	// null
    "UNUSED"   => "NULL",
	// 从执行数据中，取回指定编号的变量（解引用）
    "CV"       => "_get_zval_ptr_cv_deref_\\1(opline->op2.var EXECUTE_DATA_CC)",
    "TMPVAR"   => "???",
    "TMPVARCV" => "???",
);

// ing3, ANY/UNUSED:null, TMP:zval* ASSERT 非引用, VAR/CV/TMPVAR/TMPVARCV:zval* 解引用,CONST:(p2).zv
$op1_get_zval_ptr_undef = array(
	// 在执行数据中通过 p1:运算对象类型，p3:操作类型，取回p2:运算对象 指向的zval（编译变量无附加处理）
    "ANY"      => "get_zval_ptr_undef(opline->op1_type, opline->op1, \\1)",
	// 通过偏移量获取 执行数据 中的 第n个 zval, 不可以是引用类型。p1:变量序号
    "TMP"      => "_get_zval_ptr_tmp(opline->op1.var EXECUTE_DATA_CC)",
	// 通过偏移量获取 执行数据 中的 第n个 zval。p1:变量序号
    "VAR"      => "_get_zval_ptr_var(opline->op1.var EXECUTE_DATA_CC)",
	// 访问 (p2).zv。p1:opline,p2:node
    "CONST"    => "RT_CONSTANT(opline, opline->op1)",
	// null
    "UNUSED"   => "NULL",
	// 通过偏移量获取 execute_data 中的 zval
    "CV"       => "EX_VAR(opline->op1.var)",
	// 通过偏移量获取 执行数据 中的 第n个 zval。p1:变量序号
	"TMPVAR"   => "_get_zval_ptr_var(opline->op1.var EXECUTE_DATA_CC)",
	// 通过偏移量获取 execute_data 中的 zval
    "TMPVARCV" => "EX_VAR(opline->op1.var)",
);

// ing3, ANY/UNUSED:null, TMP:zval* ASSERT 非引用, VAR/CV/TMPVAR/TMPVARCV:zval* 解引用,CONST:(p2).zv
$op2_get_zval_ptr_undef = array(
	// 在执行数据中通过 p1:运算对象类型，p3:操作类型，取回p2:运算对象 指向的zval（编译变量无附加处理）
    "ANY"      => "get_zval_ptr_undef(opline->op2_type, opline->op2, \\1)",
	// 通过偏移量获取 执行数据 中的 第n个 zval, 不可以是引用类型。p1:变量序号
    "TMP"      => "_get_zval_ptr_tmp(opline->op2.var EXECUTE_DATA_CC)",
	// 通过偏移量获取 执行数据 中的 第n个 zval。p1:变量序号
    "VAR"      => "_get_zval_ptr_var(opline->op2.var EXECUTE_DATA_CC)",
	// 访问 (p2).zv。p1:opline,p2:node
    "CONST"    => "RT_CONSTANT(opline, opline->op2)",
	// null
    "UNUSED"   => "NULL",
	// 通过偏移量获取 execute_data 中的 zval
    "CV"       => "EX_VAR(opline->op2.var)",
	// 通过偏移量获取 执行数据 中的 第n个 zval。p1:变量序号
    "TMPVAR"   => "_get_zval_ptr_var(opline->op2.var EXECUTE_DATA_CC)",
	// 通过偏移量获取 execute_data 中的 zval
    "TMPVARCV" => "EX_VAR(opline->op2.var)",
);

// ing3, ANY:ZEND_ASSERT 不支持, TMP/CONST/UNUSED:null, VAR:zval*，解间接引用, TMPVAR/TMPVARCV:调用错误
$op1_get_zval_ptr_ptr_undef = array(
	// 同 get_obj_zval_ptr
	//在执行数据中通过 p1:运算对象类型，p3:操作类型，取回p2:运算对象 指向的zval
	// 编译变量带附加处理，普通变量解间接引用
    "ANY"      => "get_zval_ptr_ptr_undef(opline->op1_type, opline->op1, \\1)",
	// null
    "TMP"      => "NULL",
	// 在执行数据中通过 p1:序号 取回zval（解间接引用）
    "VAR"      => "_get_zval_ptr_ptr_var(opline->op1.var EXECUTE_DATA_CC)",
	// null
    "CONST"    => "NULL",
	// null
    "UNUSED"   => "NULL",
	// 通过偏移量获取 execute_data 中的 zval
    "CV"       => "EX_VAR(opline->op1.var)",
    "TMPVAR"   => "???",
    "TMPVARCV" => "???",
);

// ing3, ANY:ZEND_ASSERT 不支持, TMP/CONST/UNUSED:null, VAR:zval*，解间接引用, TMPVAR/TMPVARCV:调用错误
$op2_get_zval_ptr_ptr_undef = array(
	// 同 get_obj_zval_ptr
	//在执行数据中通过 p1:运算对象类型，p3:操作类型，取回p2:运算对象 指向的zval
	// 编译变量带附加处理，普通变量解间接引用
    "ANY"      => "get_zval_ptr_ptr_undef(opline->op2_type, opline->op2, \\1)",
	// null
    "TMP"      => "NULL",
	// 在执行数据中通过 p1:序号 取回zval（解间接引用）
    "VAR"      => "_get_zval_ptr_ptr_var(opline->op2.var EXECUTE_DATA_CC)",
	// null
    "CONST"    => "NULL",
	// null
    "UNUSED"   => "NULL",
	// 通过偏移量获取 execute_data 中的 zval
    "CV"       => "EX_VAR(opline->op2.var)",
    "TMPVAR"   => "???",
    "TMPVARCV" => "???",
);

// ing3, ANY:ZEND_ASSERT 不支持, TMP:zval* ASSERT 非引用, VAR/CV/TMPVAR:zval*,CONST:(p2).zv,UNUSED:&EX(This), TMPVARCV:调用错误
$op1_get_obj_zval_ptr = array(
	// 在执行数据中通过 p1:运算对象类型，p2:运算对象，p3:操作类型，取回需要的zval
	// 运算对象类型为 IS_UNUSED 时返回 this
    "ANY"      => "get_obj_zval_ptr(opline->op1_type, opline->op1, \\1)",
	// 通过偏移量获取 执行数据 中的 第n个 zval, 不可以是引用类型。p1:变量序号
    "TMP"      => "_get_zval_ptr_tmp(opline->op1.var EXECUTE_DATA_CC)",
	// 通过偏移量获取 执行数据 中的 第n个 zval。p1:变量序号
    "VAR"      => "_get_zval_ptr_var(opline->op1.var EXECUTE_DATA_CC)",
	// 访问 (p2).zv。p1:opline,p2:node
    "CONST"    => "RT_CONSTANT(opline, opline->op1)",
	// $this
    "UNUSED"   => "&EX(This)",
	// 从执行数据中，取回指定编号的变量。 （这是一系列的函数 ，也在zend_execute.c里）
    "CV"       => "_get_zval_ptr_cv_\\1(opline->op1.var EXECUTE_DATA_CC)",
	// 通过偏移量获取 执行数据 中的 第n个 zval。p1:变量序号
    "TMPVAR"   => "_get_zval_ptr_var(opline->op1.var EXECUTE_DATA_CC)",
    "TMPVARCV" => "???",
);

// ing3, ANY:ZEND_ASSERT 不支持, TMP:zval* ASSERT 非引用, VAR/CV/TMPVAR:zval*,CONST:(p2).zv,UNUSED:&EX(This), TMPVARCV:调用错误
$op2_get_obj_zval_ptr = array(
	// 在执行数据中通过 p1:运算对象类型，p2:运算对象，p3:操作类型，取回需要的zval
	// 运算对象类型为 IS_UNUSED 时返回 this    
    "ANY"      => "get_obj_zval_ptr(opline->op2_type, opline->op2, \\1)",
	// 通过偏移量获取 执行数据 中的 第n个 zval, 不可以是引用类型。p1:变量序号
    "TMP"      => "_get_zval_ptr_tmp(opline->op2.var EXECUTE_DATA_CC)",
	// 通过偏移量获取 执行数据 中的 第n个 zval。p1:变量序号
    "VAR"      => "_get_zval_ptr_var(opline->op2.var EXECUTE_DATA_CC)",
	// 访问 (p2).zv。p1:opline,p2:node
    "CONST"    => "RT_CONSTANT(opline, opline->op2)",
	// $this
    "UNUSED"   => "&EX(This)",
	// 从执行数据中，取回指定编号的变量。 （这是一系列的函数 ，也在zend_execute.c里）
    "CV"       => "_get_zval_ptr_cv_\\1(opline->op2.var EXECUTE_DATA_CC)",
	// 通过偏移量获取 执行数据 中的 第n个 zval。p1:变量序号
    "TMPVAR"   => "_get_zval_ptr_var(opline->op2.var EXECUTE_DATA_CC)",
    "TMPVARCV" => "???",
);

// ing3, ANY:ZEND_ASSERT 不支持, TMP:zval* ASSERT 非引用, VAR/CV/TMPVAR/TMPVARCV:zval*,CONST:(p2).zv,UNUSED:&EX(This)
$op1_get_obj_zval_ptr_undef = array(
	// 在执行数据中通过 p1:运算对象类型，p3:操作类型，取回p2:运算对象 指向的zval
	// （编译变量无附加处理）(不解引用） 运算对象类型为 IS_UNUSED 时返回 this
    "ANY"      => "get_obj_zval_ptr_undef(opline->op1_type, opline->op1, \\1)",
	// 通过偏移量获取 执行数据 中的 第n个 zval, 不可以是引用类型。p1:变量序号
    "TMP"      => "_get_zval_ptr_tmp(opline->op1.var EXECUTE_DATA_CC)",
	// 通过偏移量获取 执行数据 中的 第n个 zval。p1:变量序号
    "VAR"      => "_get_zval_ptr_var(opline->op1.var EXECUTE_DATA_CC)",
	// 访问 (p2).zv。p1:opline,p2:node
    "CONST"    => "RT_CONSTANT(opline, opline->op1)",
	// $this
    "UNUSED"   => "&EX(This)",
	// 通过偏移量获取 execute_data 中的 zval
    "CV"       => "EX_VAR(opline->op1.var)",
	// 通过偏移量获取 执行数据 中的 第n个 zval。p1:变量序号
    "TMPVAR"   => "_get_zval_ptr_var(opline->op1.var EXECUTE_DATA_CC)",
	// 通过偏移量获取 execute_data 中的 zval
    "TMPVARCV" => "EX_VAR(opline->op1.var)",
);

// ing3, ANY:ZEND_ASSERT 不支持, TMP:zval* ASSERT 非引用, VAR/CV/TMPVAR/TMPVARCV:zval*,CONST:(p2).zv,UNUSED:&EX(This)
$op2_get_obj_zval_ptr_undef = array(
	// 在执行数据中通过 p1:运算对象类型，p3:操作类型，取回p2:运算对象 指向的zval
	// （编译变量无附加处理）(不解引用） 运算对象类型为 IS_UNUSED 时返回 this
    "ANY"      => "get_obj_zval_ptr_undef(opline->op2_type, opline->op2, \\1)",
	// 通过偏移量获取 执行数据 中的 第n个 zval, 不可以是引用类型。p1:变量序号
    "TMP"      => "_get_zval_ptr_tmp(opline->op2.var EXECUTE_DATA_CC)",
	// 通过偏移量获取 执行数据 中的 第n个 zval。p1:变量序号
    "VAR"      => "_get_zval_ptr_var(opline->op2.var EXECUTE_DATA_CC)",
	// 访问 (p2).zv。p1:opline,p2:node
    "CONST"    => "RT_CONSTANT(opline, opline->op2)",
	// $this
    "UNUSED"   => "&EX(This)",
	// 通过偏移量获取 execute_data 中的 zval
    "CV"       => "EX_VAR(opline->op2.var)",
	// 通过偏移量获取 执行数据 中的 第n个 zval。p1:变量序号
    "TMPVAR"   => "_get_zval_ptr_var(opline->op2.var EXECUTE_DATA_CC)",
	// 通过偏移量获取 execute_data 中的 zval
    "TMPVARCV" => "EX_VAR(opline->op2.var)",
);

// ing3, ANY:ZEND_ASSERT 不支持, TMP:zval* ASSERT 非引用, VAR/CV:zval* 解引用,CONST:(p2).zv,UNUSED:&EX(This), TMPVAR/TMPVARCV:调用错误
$op1_get_obj_zval_ptr_deref = array(
	// 在执行数据中通过 p1:运算对象类型，p2:运算对象，p3:操作类型，取回需要的zval
	// 运算对象类型为 IS_UNUSED 时返回 this
    "ANY"      => "get_obj_zval_ptr(opline->op1_type, opline->op1, \\1)",
	// 通过偏移量获取 执行数据 中的 第n个 zval, 不可以是引用类型。p1:变量序号
    "TMP"      => "_get_zval_ptr_tmp(opline->op1.var EXECUTE_DATA_CC)",
	// 通过偏移量获取 执行数据 中的 第n个 zval，减少引用次数后返回
    "VAR"      => "_get_zval_ptr_var_deref(opline->op1.var EXECUTE_DATA_CC)",
	// 访问 (p2).zv。p1:opline,p2:node
    "CONST"    => "RT_CONSTANT(opline, opline->op1)",
	// $this
    "UNUSED"   => "&EX(This)",
	// 从执行数据中，取回指定编号的变量（解引用）
    "CV"       => "_get_zval_ptr_cv_deref_\\1(opline->op1.var EXECUTE_DATA_CC)",
    "TMPVAR"   => "???",
    "TMPVARCV" => "???",
);

// ing3, ANY:ZEND_ASSERT 不支持, TMP:zval* ASSERT 非引用, VAR/CV:zval* 解引用,CONST:(p2).zv,UNUSED:&EX(This), TMPVAR/TMPVARCV:调用错误
$op2_get_obj_zval_ptr_deref = array(
	// 在执行数据中通过 p1:运算对象类型，p2:运算对象，p3:操作类型，取回需要的zval
	// 运算对象类型为 IS_UNUSED 时返回 this
    "ANY"      => "get_obj_zval_ptr(opline->op2_type, opline->op2, \\1)",
	// 通过偏移量获取 执行数据 中的 第n个 zval, 不可以是引用类型。p1:变量序号
    "TMP"      => "_get_zval_ptr_tmp(opline->op2.var EXECUTE_DATA_CC)",
	// 通过偏移量获取 执行数据 中的 第n个 zval，减少引用次数后返回
    "VAR"      => "_get_zval_ptr_var_deref(opline->op2.var EXECUTE_DATA_CC)",
	// 访问 (p2).zv。p1:opline,p2:node
    "CONST"    => "RT_CONSTANT(opline, opline->op2)",
	// $this
    "UNUSED"   => "&EX(This)",
	// 从执行数据中，取回指定编号的变量（解引用）
    "CV"       => "_get_zval_ptr_cv_deref_\\1(opline->op2.var EXECUTE_DATA_CC)",
    "TMPVAR"   => "???",
    "TMPVARCV" => "???",
);

// ing3, ANY:ZEND_ASSERT 不支持, TMP/CONST:null, VAR:zval* 解引用,UNUSED:&EX(This),CV:zval*, TMPVAR/TMPVARCV:调用错误
$op1_get_obj_zval_ptr_ptr = array(
	// 在执行数据中通过 p1:运算对象类型，p3:操作类型，取回p2:运算对象 指向的zval
	// 编译变量带附加处理，普通变量解间接引用，运算对象类型为 IS_UNUSED 时返回 this
    "ANY"      => "get_obj_zval_ptr_ptr(opline->op1_type, opline->op1, \\1)",
	// null
    "TMP"      => "NULL",
	// 在执行数据中通过 p1:序号 取回zval（解间接引用）
    "VAR"      => "_get_zval_ptr_ptr_var(opline->op1.var EXECUTE_DATA_CC)",
	// null
    "CONST"    => "NULL",
	// $this
    "UNUSED"   => "&EX(This)",
	// 从执行数据中，取回指定编号的变量。 （这是一系列的函数 ，也在zend_execute.c里）
    "CV"       => "_get_zval_ptr_cv_\\1(opline->op1.var EXECUTE_DATA_CC)",
    "TMPVAR"   => "???",
    "TMPVARCV" => "???",
);

// ing3, ANY:ZEND_ASSERT 不支持, TMP/CONST:null, VAR:zval* 解引用,UNUSED:&EX(This),CV:zval*, TMPVAR/TMPVARCV:调用错误
$op2_get_obj_zval_ptr_ptr = array(
	// 在执行数据中通过 p1:运算对象类型，p3:操作类型，取回p2:运算对象 指向的zval
	// 编译变量带附加处理，普通变量解间接引用，运算对象类型为 IS_UNUSED 时返回 this
    "ANY"      => "get_obj_zval_ptr_ptr(opline->op2_type, opline->op2, \\1)",
	// null
    "TMP"      => "NULL",
	// 在执行数据中通过 p1:序号 取回zval（解间接引用）
    "VAR"      => "_get_zval_ptr_ptr_var(opline->op2.var EXECUTE_DATA_CC)",
	// null
    "CONST"    => "NULL",
	// $this
    "UNUSED"   => "&EX(This)",
	// 从执行数据中，取回指定编号的变量。 （这是一系列的函数 ，也在zend_execute.c里）
    "CV"       => "_get_zval_ptr_cv_\\1(opline->op2.var EXECUTE_DATA_CC)",
    "TMPVAR"   => "???",
    "TMPVARCV" => "???",
);

// ing3, ANY:ZEND_ASSERT 不支持, TMP/CONST:null, VAR:zval* 解引用,UNUSED:&EX(This),CV:zval*, TMPVAR/TMPVARCV:调用错误
$op1_get_obj_zval_ptr_ptr_undef = array(
	// 在执行数据中通过 p1:运算对象类型，p3:操作类型，取回p2:运算对象 指向的zval
	// 编译变量带附加处理，普通变量解间接引用，运算对象类型为 IS_UNUSED 时返回 this
    "ANY"      => "get_obj_zval_ptr_ptr(opline->op1_type, opline->op1, \\1)",
	// null
    "TMP"      => "NULL",
	// 在执行数据中通过 p1:序号 取回zval（解间接引用）
    "VAR"      => "_get_zval_ptr_ptr_var(opline->op1.var EXECUTE_DATA_CC)",
	// null
    "CONST"    => "NULL",
	// $this
    "UNUSED"   => "&EX(This)",
	// 通过偏移量获取 execute_data 中的 zval
    "CV"       => "EX_VAR(opline->op1.var)",
    "TMPVAR"   => "???",
    "TMPVARCV" => "???",
);

// ing3, ANY:ZEND_ASSERT 不支持, TMP/CONST:null, VAR:zval* 解引用,UNUSED:&EX(This),CV:zval*, TMPVAR/TMPVARCV:调用错误
$op2_get_obj_zval_ptr_ptr_undef = array(
	// 在执行数据中通过 p1:运算对象类型，p3:操作类型，取回p2:运算对象 指向的zval
	// 编译变量带附加处理，普通变量解间接引用，运算对象类型为 IS_UNUSED 时返回 this
    "ANY"      => "get_obj_zval_ptr_ptr(opline->op2_type, opline->op2, \\1)",
	// null
    "TMP"      => "NULL",
	// 在执行数据中通过 p1:序号 取回zval（解间接引用）
    "VAR"      => "_get_zval_ptr_ptr_var(opline->op2.var EXECUTE_DATA_CC)",
	// null
    "CONST"    => "NULL",
	// $this
	"UNUSED"   => "&EX(This)",
	// 通过偏移量获取 execute_data 中的 zval
    "CV"       => "EX_VAR(opline->op2.var)",
    "TMPVAR"   => "???",
    "TMPVARCV" => "???",
);

// ing3, ANY:无操作, TMP/VAR/TMPVAR:非gc，销毁无引用变量, CONST/UNUSED/CV:未调用方法, TMPVARCV:调用错误
$op1_free_op = array(
	// 释放（变量 和 临时变量 类型的）运算对象（zval）：p1:类型，p2:编号
    "ANY"      => "FREE_OP(opline->op1_type, opline->op1.var)",
	// 销毁没有引用次数的对象,并没有销毁 zval本身。不放入gc回收周期。
    "TMP"      => "zval_ptr_dtor_nogc(EX_VAR(opline->op1.var))",
	// 销毁没有引用次数的对象,并没有销毁 zval本身。不放入gc回收周期。
    "VAR"      => "zval_ptr_dtor_nogc(EX_VAR(opline->op1.var))",
    "CONST"    => "",
    "UNUSED"   => "",
    "CV"       => "",
	// 销毁没有引用次数的对象,并没有销毁 zval本身。不放入gc回收周期。
    "TMPVAR"   => "zval_ptr_dtor_nogc(EX_VAR(opline->op1.var))",
    "TMPVARCV" => "???",
);

// ing3, ANY:无操作, TMP/VAR/TMPVAR:非gc，销毁无引用变量, CONST/UNUSED/CV:未调用方法, TMPVARCV:调用错误
$op2_free_op = array(
	// 释放（变量 和 临时变量 类型的）运算对象（zval）：p1:类型，p2:编号
    "ANY"      => "FREE_OP(opline->op2_type, opline->op2.var)",
	// 销毁没有引用次数的对象,并没有销毁 zval本身。不放入gc回收周期。
    "TMP"      => "zval_ptr_dtor_nogc(EX_VAR(opline->op2.var))",
	// 销毁没有引用次数的对象,并没有销毁 zval本身。不放入gc回收周期。
    "VAR"      => "zval_ptr_dtor_nogc(EX_VAR(opline->op2.var))",
    "CONST"    => "",
    "UNUSED"   => "",
    "CV"       => "",
	// 销毁没有引用次数的对象,并没有销毁 zval本身。不放入gc回收周期。
    "TMPVAR"   => "zval_ptr_dtor_nogc(EX_VAR(opline->op2.var))",
    "TMPVARCV" => "???",
);

// ing3, ANY/VAR:非gc，销毁无引用变量, TMP/CONST/UNUSED/CV:未调用方法, TMPVAR/TMPVARCV:调用错误
$op1_free_op_if_var = array(
	// 销毁没有引用次数的对象,并没有销毁 zval本身。不放入gc回收周期。
    "ANY"      => "if (opline->op1_type == IS_VAR) {zval_ptr_dtor_nogc(EX_VAR(opline->op1.var));}",
    "TMP"      => "",
	// 销毁没有引用次数的对象,并没有销毁 zval本身。不放入gc回收周期。
    "VAR"      => "zval_ptr_dtor_nogc(EX_VAR(opline->op1.var))",
    "CONST"    => "",
    "UNUSED"   => "",
    "CV"       => "",
    "TMPVAR"   => "???",
    "TMPVARCV" => "???",
);

// ing3, ANY/VAR:非gc，销毁无引用变量, TMP/CONST/UNUSED/CV:未调用方法, TMPVAR/TMPVARCV:调用错误
$op2_free_op_if_var = array(
	// 销毁没有引用次数的对象,并没有销毁 zval本身。不放入gc回收周期。
    "ANY"      => "if (opline->op2_type == IS_VAR) {zval_ptr_dtor_nogc(EX_VAR(opline->op2.var));}",
    "TMP"      => "",
	// 销毁没有引用次数的对象,并没有销毁 zval本身。不放入gc回收周期。
    "VAR"      => "zval_ptr_dtor_nogc(EX_VAR(opline->op2.var))",
    "CONST"    => "",
    "UNUSED"   => "",
    "CV"       => "",
    "TMPVAR"   => "???",
    "TMPVARCV" => "???",
);

// ing3, 类型对应的C语言表示。any返回下一个操作码的op1的类型
$op_data_type = array(
    "ANY"      => "(opline+1)->op1_type",
    "TMP"      => "IS_TMP_VAR",
    "VAR"      => "IS_VAR",
    "CONST"    => "IS_CONST",
    "UNUSED"   => "IS_UNUSED",
    "CV"       => "IS_CV",
    "TMPVAR"   => "(IS_TMP_VAR|IS_VAR)",
    "TMPVARCV" => "(IS_TMP_VAR|IS_VAR|IS_CV)",
);

// ing3, ANY/UNUSED:null, TMP:zval* ASSERT 非引用, VAR/CV/TMPVAR:zval*,CONST:(p2).zv, TMPVARCV:调用错误
$op_data_get_zval_ptr = array(
	// 在执行数据中通过 p1:运算对象类型，p3:操作类型，取回p2:运算对象指向的zval
	//（常量取下一行操作码）（编译变量未定义报错）
    "ANY"      => "get_op_data_zval_ptr_r((opline+1)->op1_type, (opline+1)->op1)",
	// 通过偏移量获取 执行数据 中的 第n个 zval, 不可以是引用类型。p1:变量序号
    "TMP"      => "_get_zval_ptr_tmp((opline+1)->op1.var EXECUTE_DATA_CC)",
	// 通过偏移量获取 执行数据 中的 第n个 zval。p1:变量序号
    "VAR"      => "_get_zval_ptr_var((opline+1)->op1.var EXECUTE_DATA_CC)",
	// 访问 (p2).zv。p1:opline,p2:node
    "CONST"    => "RT_CONSTANT((opline+1), (opline+1)->op1)",
	// null
    "UNUSED"   => "NULL",
	// 从执行数据中，取回指定编号的变量。 （这是一系列的函数 ，也在zend_execute.c里）
    "CV"       => "_get_zval_ptr_cv_\\1((opline+1)->op1.var EXECUTE_DATA_CC)",
	// 通过偏移量获取 执行数据 中的 第n个 zval。p1:变量序号
    "TMPVAR"   => "_get_zval_ptr_var((opline+1)->op1.var EXECUTE_DATA_CC)",
    "TMPVARCV" => "???",
);

// ing3, ANY/UNUSED:无此方法, TMP:zval* ASSERT 非引用, VAR/CV/TMPVAR/TMPVARCV:zval*,CONST:(p2).zv
$op_data_get_zval_ptr_undef = array(
	// 没有这个方法。zend_vm_execute.h里也没有这个
    "ANY"      => "get_op_data_zval_ptr_undef((opline+1)->op1_type, (opline+1)->op1)",
	// 通过偏移量获取 执行数据 中的 第n个 zval, 不可以是引用类型。p1:变量序号
    "TMP"      => "_get_zval_ptr_tmp((opline+1)->op1.var EXECUTE_DATA_CC)",
	// 通过偏移量获取 执行数据 中的 第n个 zval。p1:变量序号
    "VAR"      => "_get_zval_ptr_var((opline+1)->op1.var EXECUTE_DATA_CC)",
	// 访问 (p2).zv。p1:opline,p2:node
    "CONST"    => "RT_CONSTANT((opline+1), (opline+1)->op1)",
	// null
    "UNUSED"   => "NULL",
	// 通过偏移量获取 execute_data 中的 zval
    "CV"       => "EX_VAR((opline+1)->op1.var)",
	// 通过偏移量获取 执行数据 中的 第n个 zval。p1:变量序号
    "TMPVAR"   => "_get_zval_ptr_var((opline+1)->op1.var EXECUTE_DATA_CC)",
	// 通过偏移量获取 execute_data 中的 zval
    "TMPVARCV" => "EX_VAR((opline+1)->op1.var)",
);

// ing3, ANY/UNUSED:null, TMP:zval* ASSERT 非引用, VAR/CV:zval* 解引用,CONST:(p2).zv, TMPVAR/TMPVARCV:调用错误
$op_data_get_zval_ptr_deref = array(
	//在执行数据中通过 p1:运算对象类型，p3:操作类型，取回p2:运算对象指向的zval
	//（常量取下一行操作码）（编译变量未定义报错）（解引用）
    "ANY"      => "get_op_data_zval_ptr_deref_r((opline+1)->op1_type, (opline+1)->op1)",
	// 通过偏移量获取 执行数据 中的 第n个 zval, 不可以是引用类型。p1:变量序号
    "TMP"      => "_get_zval_ptr_tmp((opline+1)->op1.var EXECUTE_DATA_CC)",
	// 通过偏移量获取 执行数据 中的 第n个 zval，减少引用次数后返回
    "VAR"      => "_get_zval_ptr_var_deref((opline+1)->op1.var EXECUTE_DATA_CC)",
	// 访问 (p2).zv。p1:opline,p2:node
    "CONST"    => "RT_CONSTANT((opline+1), (opline+1)->op1)",
	// null
    "UNUSED"   => "NULL",
	// 从执行数据中，取回指定编号的变量（解引用）
    "CV"       => "_get_zval_ptr_cv_deref_\\1((opline+1)->op1.var EXECUTE_DATA_CC)",
    "TMPVAR"   => "???",
    "TMPVARCV" => "???",
);

// ing3, ANY:zval*（解间接引用）, TMP/CONST/UNUSED:null, VAR:zval*, 解间接引用, CV:zval*, TMPVAR/TMPVARCV:调用错误
$op_data_get_zval_ptr_ptr = array(
	// 在执行数据中通过 p1:运算对象类型，p3:操作类型，取回p2:运算对象 指向的zval
	// 编译变量带附加处理，普通变量解间接引用
    "ANY"      => "get_zval_ptr_ptr((opline+1)->op1_type, (opline+1)->op1, \\1)",
	// null
    "TMP"      => "NULL",
	// 在执行数据中通过 p1:序号 取回zval（解间接引用）
    "VAR"      => "_get_zval_ptr_ptr_var((opline+1)->op1.var EXECUTE_DATA_CC)",
	// null
    "CONST"    => "NULL",
	// null
    "UNUSED"   => "NULL",
	// 从执行数据中，取回指定编号的变量。 （这是一系列的函数 ，也在zend_execute.c里）
    "CV"       => "_get_zval_ptr_cv_\\1((opline+1)->op1.var EXECUTE_DATA_CC)",
    "TMPVAR"   => "???",
    "TMPVARCV" => "???",
);

// ing3, ANY:无操作, TMP/VAR/TMPVAR:非gc，销毁无引用变量, CONST/UNUSED/CV:未调用方法, TMPVARCV:调用错误
$op_data_free_op = array(
	// 释放（变量 和 临时变量 类型的）运算对象（zval）：p1:类型，p2:编号
    "ANY"      => "FREE_OP((opline+1)->op1_type, (opline+1)->op1.var)",
	// 销毁没有引用次数的对象,并没有销毁 zval本身。不放入gc回收周期。
    "TMP"      => "zval_ptr_dtor_nogc(EX_VAR((opline+1)->op1.var))",
	// 销毁没有引用次数的对象,并没有销毁 zval本身。不放入gc回收周期。
    "VAR"      => "zval_ptr_dtor_nogc(EX_VAR((opline+1)->op1.var))",
    "CONST"    => "",
    "UNUSED"   => "",
    "CV"       => "",
	// 销毁没有引用次数的对象,并没有销毁 zval本身。不放入gc回收周期。
    "TMPVAR"   => "zval_ptr_dtor_nogc(EX_VAR((opline+1)->op1.var))",
    "TMPVARCV" => "???",
);
// 以上这堆变量都用在 gen_code 中，生成代码时使用 ---------------------------

// 操作码处理器和助手，按原始顺序排列：gen_labels，gen_executor_code，gen_vm
// [ [行号 =>['handler'=>操作码号]] ,[行号 =>['helper'=>助手名称]] ] ，虽然行号很大，但实际只有270+个元素
$list    = array(); // list of opcode handlers and helpers in original order

// 操作码编号 索引的 处理器列表：opcode_name，gen_labels，gen_executor_code，gen_vm_opcodes_header，gen_vm
$opcodes = array(); // opcode handlers by code

// 操作码名索引的 操作助手列表：is_hot_helper，helper_name，gen_executor_code，gen_vm
$helpers = array(); // opcode helpers by name

// 操作助手的参数列表：gen_executor，gen_vm
$params  = array(); // parameters of helpers

// 操作码名到值的对应关系：opcode_name，gen_handler，gen_labels，gen_vm
$opnames = array(); // opcode name to code mapping

// 起始行号：out，out_line，gen_executor
$line_no = 1;

// 额外专用扩展：parse_spec_rules，gen_vm
// 规则放在键名里，有8个： OP_DATA，RETVAL，QUICK_ARG，SMART_BRANCH，NO_CONST_CONST，COMMUTATIVE，ISSET，OBSERVER
// 值都是 1
$used_extra_spec = array();

// ing4, 把字串写到最后的执行器里： gen_code，gen_handler，gen_helper，gen_null_label，gen_labels，gen_specs，gen_null_handler，gen_executor_code，skip_blanks，gen_executor，gen_vm 调用
// Writes $s into resulting executor
function out($f, $s) {
    global $line_no;

    fputs($f,$s);
    $line_no += substr_count($s, "\n");
}

// ing4, 在结果执行器里，重置 #line 指令： gen_executor_code 调用
// Resets #line directives in resulting executor
function out_line($f) {
    global $line_no, $executor_file;

    fputs($f,"#line ".($line_no+1)." \"".$executor_file."\"\n");
    ++$line_no;
}

// ing4, 是否是“热”助手( 有hot 标记 )：gen_code 调用
function is_hot_helper($name) {
    global $helpers;

    if (isset($helpers[$name]["hot"])) {
        return $helpers[$name]["hot"];
    }

    return false;
}

// ing3, 返回专用助手的名字：gen_code 调用
// Returns name of specialized helper
// 名字 + 专用规则  + op1前缀 + op2前缀 + 额外的规则
// 所有传递的op1和op2都是两个操作对象的类型
function helper_name($name, $spec, $op1, $op2, $extra_spec) {
    global $prefix, $helpers;

    $extra = "";
	// 如果名字已经存在
    if (isset($helpers[$name])) {
		// 如果没有包含特别声名的专用操作对象 的助手 ， 使用非专用助手 
        // If we have no helper with specified specialized operands then
        // using unspecialized helper
		// op1 的类型适配
        if (!isset($helpers[$name]["op1"][$op1])) {
            if (($op1 == 'TMP' || $op1 == 'VAR') &&
                isset($helpers[$name]["op1"]["TMPVAR"])) {
                $op1 = "TMPVAR";
            } else if (($op1 == 'TMP' || $op1 == 'VAR') &&
                isset($helpers[$name]["op1"]["TMPVARCV"])) {
                $op1 = "TMPVARCV";
            } else if ($op1 == 'CV' &&
                isset($helpers[$name]["op1"]["TMPVARCV"])) {
                $op1 = "TMPVARCV";
            } else if (isset($helpers[$name]["op1"]["ANY"])) {
                $op1 = "ANY";
            }
        }
		// op2 的类型适配
        if (!isset($helpers[$name]["op2"][$op2])) {
            if (($op2 == 'TMP' || $op2 == 'VAR') &&
                isset($helpers[$name]["op2"]["TMPVAR"])) {
                $op2 = "TMPVAR";
            } else if (($op2 == 'TMP' || $op2 == 'VAR') &&
                isset($helpers[$name]["op2"]["TMPVARCV"])) {
                $op2 = "TMPVARCV";
            } else if ($op2 == 'CV' &&
                isset($helpers[$name]["op2"]["TMPVARCV"])) {
                $op2 = "TMPVARCV";
            } else if (isset($helpers[$name]["op2"]["ANY"])) {
                $op2 = "ANY";
            }
        }
        /* forward common specs (e.g. in ZEND_VM_DISPATCH_TO_HELPER) */
        if (isset($extra_spec, $helpers[$name]["spec"])) {
			// 按额外规则 ，生成后缀, array_intersect_key 获取键名是交集的元素
            $extra = extra_spec_name(array_intersect_key($extra_spec, $helpers[$name]["spec"]));
        }
    }

	// 名字 + 专用规则  + op1前缀 + op2前缀 + 额外的规则
    return $name . ($spec ? "_SPEC" : "") . $prefix[$op1] . $prefix[$op2] . $extra;
}

// 操作码名：gen_code 调用
// ing3, 所有传递的op1和op2都是两个操作对象的类型
function opcode_name($name, $spec, $op1, $op2, $extra_spec) {
    global $prefix, $opnames, $opcodes;

    $extra = "";

    if (isset($opnames[$name])) {
		// 操作码
        $opcode = $opcodes[$opnames[$name]];
		// 如果没有包含特别声名的专用操作对象 的助手 ， 使用非专用助手
        // If we have no helper with specified specialized operands then
        // using unspecialized helper
		// 第一个操作对象的类型适配
        if (!isset($opcode["op1"][$op1])) {
			// op1 是 TMP 或 VAR
            if (($op1 == 'TMP' || $op1 == 'VAR') &&
				// 并且支持 TMPVAR 类型
                isset($opcode["op1"]["TMPVAR"])) {
				// p1 使用 TMPVAR 类型
                $op1 = "TMPVAR";
			// op1 是 TMP 或 VAR
            } else if (($op1 == 'TMP' || $op1 == 'VAR') &&
				// 并且支持 TMPVARCV 类型
                isset($opcode["op1"]["TMPVARCV"])) {
				// p1 使用 TMPVARCV 类型
                $op1 = "TMPVARCV";
			// op1 是 CV
            } else if ($op1 == 'CV' &&
				// 并且支持 TMPVARCV 类型
                isset($opcode["op1"]["TMPVARCV"])) {
				// p1 使用 TMPVARCV 类型
                $op1 = "TMPVARCV";
			// op1 支持 ANY类型
            } else if (isset($opcode["op1"]["ANY"])) {
				// p1 使用 ANY 类型
                $op1 = "ANY";
			// 如果还是没匹配到，但是有 $spec
            } else if ($spec) {
				// 无法处理的code 分发到无效处理器
                /* dispatch to invalid handler from unreachable code */
				// 返回 ZEND_NULL 无效处理器
                return "ZEND_NULL";
            }
        }
		// 第二个操作对象的类型适配
        if (!isset($opcode["op2"][$op2])) {
            if (($op2 == 'TMP' || $op2 == 'VAR') &&
                isset($opcode["op2"]["TMPVAR"])) {
                $op2 = "TMPVAR";
            } else if (($op2 == 'TMP' || $op2 == 'VAR') &&
                isset($opcode["op2"]["TMPVARCV"])) {
                $op2 = "TMPVARCV";
            } else if ($op2 == 'CV' &&
                isset($opcode["op2"]["TMPVARCV"])) {
                $op2 = "TMPVARCV";
            } else if (isset($opcode["op2"]["ANY"])) {
                $op2 = "ANY";
            } else if ($spec) {
                /* dispatch to unknown handler in unreachable code */
                return "ZEND_NULL";
            }
        }
        /* forward common specs (e.g. in ZEND_VM_DISPATCH_TO_HANDLER) */
		// 如果有额外规则 ，并且此操作码有规则限制
        if (isset($extra_spec, $opcode["spec"])) {
			// 按额外规则 ，生成后缀。 两个规则表里交叉的
            $extra = extra_spec_name(array_intersect_key($extra_spec, $opcode["spec"]));
        }
    }

	// 操作码名 + 规则 + op1前缀 + op2前缀 + 扩展规则 
    return $name . ($spec ? "_SPEC" : "") . $prefix[$op1] . $prefix[$op2] . $extra;
}

// clear, 格式化条件, 当需要时，用括号保护起来：gen_vm 调用
// Formats condition, protecting it by parentheses when needed.
function format_condition($condition) {
    if ($condition === "") {
		// 条件不可以为空
        throw new InvalidArgumentException("A non empty string condition was expected.");
    }
	// 如果已经加了 （） 直接返回
    if ($condition[0] === "(" && substr($condition, -1) === ")") {
        return $condition;
    }

    return "(" . $condition . ")";
}

// ing1, 生成 操作码处理器和助手用的【代码行】：gen_handler，gen_helper 调用
// Generates code for opcode handler or helper
// 参数：zend_vm_execute.h文件句柄, 1, ZEND_VM_KIND, 操作码名，第一第二个操作对象，函数名，额外规则
// 所有传递的op1和op2都是两个操作对象的类型
function gen_code($f, $spec, $kind, $code, $op1, $op2, $name, $extra_spec=null) {
	// 关键字列表
    global $op1_type, $op2_type, $op1_get_zval_ptr, $op2_get_zval_ptr,
        $op1_get_zval_ptr_deref, $op2_get_zval_ptr_deref,
        $op1_get_zval_ptr_undef, $op2_get_zval_ptr_undef,
        $op1_get_zval_ptr_ptr, $op2_get_zval_ptr_ptr,
        $op1_get_zval_ptr_ptr_undef, $op2_get_zval_ptr_ptr_undef,
        $op1_get_obj_zval_ptr, $op2_get_obj_zval_ptr,
        $op1_get_obj_zval_ptr_undef, $op2_get_obj_zval_ptr_undef,
        $op1_get_obj_zval_ptr_deref, $op2_get_obj_zval_ptr_deref,
        $op1_get_obj_zval_ptr_ptr, $op2_get_obj_zval_ptr_ptr,
        $op1_get_obj_zval_ptr_ptr_undef, $op2_get_obj_zval_ptr_ptr_undef,
        $op1_free_op, $op2_free_op, $op1_free_op_if_var, $op2_free_op_if_var,
        $prefix,
        $op_data_type, $op_data_get_zval_ptr, $op_data_get_zval_ptr_undef,
        $op_data_get_zval_ptr_deref, $op_data_get_zval_ptr_ptr,
        $op_data_free_op;

	// 专用化
    // Specializing
	// 替换规则 ：把函数和变量的调用，替换成各类型专用的调用语句
    $specialized_replacements = array(
        // 类型，对应的C语言表示。260+次引用
        "/OP1_TYPE/" => $op1_type[$op1],
        "/OP2_TYPE/" => $op2_type[$op2],
        // 获取zval指针，UNUSED 类型返回null。31 + 40次引用。
        "/GET_OP1_ZVAL_PTR\(([^)]*)\)/" => $op1_get_zval_ptr[$op1],
        "/GET_OP2_ZVAL_PTR\(([^)]*)\)/" => $op2_get_zval_ptr[$op2],
        // 获取zval指针, UNUSED 返回null，不支持 TMPVAR/TMPVARCV。6 + 4次引用。
        "/GET_OP1_ZVAL_PTR_DEREF\(([^)]*)\)/" => $op1_get_zval_ptr_deref[$op1],
        "/GET_OP2_ZVAL_PTR_DEREF\(([^)]*)\)/" => $op2_get_zval_ptr_deref[$op2],
        // 获取zval指针, UNUSED 返回null。75 + 63次引用。
        "/GET_OP1_ZVAL_PTR_UNDEF\(([^)]*)\)/" => $op1_get_zval_ptr_undef[$op1],
        "/GET_OP2_ZVAL_PTR_UNDEF\(([^)]*)\)/" => $op2_get_zval_ptr_undef[$op2],
        // 获取zval指针,TMP/CONST/UNUSED 返回null，不支持 TMPVAR/TMPVARCV。 8+1次引用。
        "/GET_OP1_ZVAL_PTR_PTR\(([^)]*)\)/" => $op1_get_zval_ptr_ptr[$op1],
        "/GET_OP2_ZVAL_PTR_PTR\(([^)]*)\)/" => $op2_get_zval_ptr_ptr[$op2],
        // 获取zval指针, TMP/CONST/UNUSED 返回null，不支持 TMPVAR/TMPVARCV。25+0次引用。
        "/GET_OP1_ZVAL_PTR_PTR_UNDEF\(([^)]*)\)/" => $op1_get_zval_ptr_ptr_undef[$op1],
        "/GET_OP2_ZVAL_PTR_PTR_UNDEF\(([^)]*)\)/" => $op2_get_zval_ptr_ptr_undef[$op2],
        // 获取zval指针, UNUSED 返回 $this，不支持 TMPVARCV。 2+0 次引用。
        "/GET_OP1_OBJ_ZVAL_PTR\(([^)]*)\)/" => $op1_get_obj_zval_ptr[$op1],
        "/GET_OP2_OBJ_ZVAL_PTR\(([^)]*)\)/" => $op2_get_obj_zval_ptr[$op2],
        // 获取zval指针, UNUSED 返回 $this。 4+0次引用。
        "/GET_OP1_OBJ_ZVAL_PTR_UNDEF\(([^)]*)\)/" => $op1_get_obj_zval_ptr_undef[$op1],
        "/GET_OP2_OBJ_ZVAL_PTR_UNDEF\(([^)]*)\)/" => $op2_get_obj_zval_ptr_undef[$op2],
        // 获取zval指针, UNUSED 返回 $this，不支持 TMPVAR/TMPVARCV。0次引用。
        "/GET_OP1_OBJ_ZVAL_PTR_DEREF\(([^)]*)\)/" => $op1_get_obj_zval_ptr_deref[$op1],
        "/GET_OP2_OBJ_ZVAL_PTR_DEREF\(([^)]*)\)/" => $op2_get_obj_zval_ptr_deref[$op2],
        // 获取zval指针, TMP/CONST返回null, UNUSED 返回 $this，不支持 TMPVAR/TMPVARCV。0次引用。
        "/GET_OP1_OBJ_ZVAL_PTR_PTR\(([^)]*)\)/" => $op1_get_obj_zval_ptr_ptr[$op1],
        "/GET_OP2_OBJ_ZVAL_PTR_PTR\(([^)]*)\)/" => $op2_get_obj_zval_ptr_ptr[$op2],
        // 获取zval指针, TMP/CONST返回null, UNUSED 返回 $this，不支持 TMPVAR/TMPVARCV。CV 直接返回。11+0次引用。
        "/GET_OP1_OBJ_ZVAL_PTR_PTR_UNDEF\(([^)]*)\)/" => $op1_get_obj_zval_ptr_ptr_undef[$op1],
        "/GET_OP2_OBJ_ZVAL_PTR_PTR_UNDEF\(([^)]*)\)/" => $op2_get_obj_zval_ptr_ptr_undef[$op2],
        // 释放操作对象。126 + 64次引用。
        "/FREE_OP1\(\)/" => $op1_free_op[$op1],
        "/FREE_OP2\(\)/" => $op2_free_op[$op2],
        // 只释放类型为 IS_VAR 的变量。8次引用。
        "/FREE_OP1_IF_VAR\(\)/" => $op1_free_op_if_var[$op1],
        "/FREE_OP2_IF_VAR\(\)/" => $op2_free_op_if_var[$op2],
        // op1和op2 都是ANY。 23次引用。
        "/\!ZEND_VM_SPEC/m" => ($op1!="ANY"||$op2!="ANY")?"0":"1",
        // op1或op2有一个不是ANY。 
        "/ZEND_VM_SPEC/m" => ($op1!="ANY"||$op2!="ANY")?"1":"0",
		// 如果不是用 ZEND_VM_KIND_CALL 模式生成,// 按额外规则 ，生成后缀
        "/ZEND_VM_C_LABEL\(\s*([A-Za-z_]*)\s*\)/m" => "\\1".(($spec && $kind != ZEND_VM_KIND_CALL)?("_SPEC".$prefix[$op1].$prefix[$op2].extra_spec_name($extra_spec)):""),
		// 如果不是用 ZEND_VM_KIND_CALL 模式生成,// 按额外规则 ，生成后缀
        "/ZEND_VM_C_GOTO\(\s*([A-Za-z_]*)\s*\)/m" => "goto \\1".(($spec && $kind != ZEND_VM_KIND_CALL)?("_SPEC".$prefix[$op1].$prefix[$op2].extra_spec_name($extra_spec)):""),
        //
        "/^#(\s*)if\s+1\s*\\|\\|.*[^\\\\]$/m" => "#\\1if 1",
        //
        "/^#(\s*)if\s+0\s*&&.*[^\\\\]$/m" => "#\\1if 0",
        //
        "/^#(\s*)elif\s+1\s*\\|\\|.*[^\\\\]$/m" => "#\\1elif 1",
        //
        "/^#(\s*)elif\s+0\s*&&.*[^\\\\]$/m" => "#\\1elif 0",
        // 类型对应的C语言表示。ANY返回下一个操作码的op1的类型。18次引用。
        "/OP_DATA_TYPE/" => $op_data_type[isset($extra_spec['OP_DATA']) ? $extra_spec['OP_DATA'] : "ANY"],
        // 取得下一个操作码的 zval。UNUSED返回null, 不支持 TMPVARCV。5次引用。
        "/GET_OP_DATA_ZVAL_PTR\(([^)]*)\)/" => $op_data_get_zval_ptr[isset($extra_spec['OP_DATA']) ? $extra_spec['OP_DATA'] : "ANY"],
        // 取得下一个操作码的 zval。UNUSED返回null, CV/TMPVARCV 直接返回。3次引用。
        "/GET_OP_DATA_ZVAL_PTR_UNDEF\(([^)]*)\)/" => $op_data_get_zval_ptr_undef[isset($extra_spec['OP_DATA']) ? $extra_spec['OP_DATA'] : "ANY"],
        // 取得下一个操作码的 zval。UNUSED返回null, 不支持 TMPVAR/TMPVARCV。0次引用。
        "/GET_OP_DATA_ZVAL_PTR_DEREF\(([^)]*)\)/" => $op_data_get_zval_ptr_deref[isset($extra_spec['OP_DATA']) ? $extra_spec['OP_DATA'] : "ANY"],
        // 取得下一个操作码的 zval。TMP/CONST/UNUSED返回null, 不支持 TMPVAR/TMPVARCV。2次引用。
        "/GET_OP_DATA_ZVAL_PTR_PTR\(([^)]*)\)/" => $op_data_get_zval_ptr_ptr[isset($extra_spec['OP_DATA']) ? $extra_spec['OP_DATA'] : "ANY"],
        // 释放下一个操作码的 op1。CONST/UNUSED/CV 免操作。不支持 TMPVARCV。16次引用。
        "/FREE_OP_DATA\(\)/" => $op_data_free_op[isset($extra_spec['OP_DATA']) ? $extra_spec['OP_DATA'] : "ANY"],
        //
        "/RETURN_VALUE_USED\(opline\)/" => isset($extra_spec['RETVAL']) ? $extra_spec['RETVAL'] : "RETURN_VALUE_USED(opline)",
        //
        "/arg_num <= MAX_ARG_FLAG_NUM/" => isset($extra_spec['QUICK_ARG']) ? $extra_spec['QUICK_ARG'] : "arg_num <= MAX_ARG_FLAG_NUM",
        //
        "/ZEND_VM_SMART_BRANCH\(\s*([^,)]*)\s*,\s*([^)]*)\s*\)/" => isset($extra_spec['SMART_BRANCH']) ?
            ($extra_spec['SMART_BRANCH'] == 1 ?
                    "ZEND_VM_SMART_BRANCH_JMPZ(\\1, \\2)"
                :	($extra_spec['SMART_BRANCH'] == 2 ?
                        "ZEND_VM_SMART_BRANCH_JMPNZ(\\1, \\2)" : "ZEND_VM_SMART_BRANCH_NONE(\\1, \\2)"))
            :	"ZEND_VM_SMART_BRANCH(\\1, \\2)",
        //
        "/ZEND_VM_SMART_BRANCH_TRUE\(\s*\)/" => isset($extra_spec['SMART_BRANCH']) ?
            ($extra_spec['SMART_BRANCH'] == 1 ?
                    "ZEND_VM_SMART_BRANCH_TRUE_JMPZ()"
                :	($extra_spec['SMART_BRANCH'] == 2 ?
                        "ZEND_VM_SMART_BRANCH_TRUE_JMPNZ()" : "ZEND_VM_SMART_BRANCH_TRUE_NONE()"))
            :	"ZEND_VM_SMART_BRANCH_TRUE()",
        //
        "/ZEND_VM_SMART_BRANCH_FALSE\(\s*\)/" => isset($extra_spec['SMART_BRANCH']) ?
            ($extra_spec['SMART_BRANCH'] == 1 ?
                    "ZEND_VM_SMART_BRANCH_FALSE_JMPZ()"
                :	($extra_spec['SMART_BRANCH'] == 2 ?
                        "ZEND_VM_SMART_BRANCH_FALSE_JMPNZ()" : "ZEND_VM_SMART_BRANCH_FALSE_NONE()"))
            :	"ZEND_VM_SMART_BRANCH_FALSE()",
        //
        "/opline->extended_value\s*&\s*ZEND_ISEMPTY/" => isset($extra_spec['ISSET']) ?
            ($extra_spec['ISSET'] == 0 ? "0" : "1")
            : "\\0",
        //
        "/opline->extended_value\s*&\s*~\s*ZEND_ISEMPTY/" => isset($extra_spec['ISSET']) ?
            ($extra_spec['ISSET'] == 0 ? "\\0" : "opline->extended_value")
            : "\\0",
		// 2处
		"/ZEND_OBSERVER_ENABLED/" => isset($extra_spec['OBSERVER']) && $extra_spec['OBSERVER'] == 1 ? "1" : "0",
		// 2处
        "/ZEND_OBSERVER_USE_RETVAL/" => isset($extra_spec['OBSERVER']) && $extra_spec['OBSERVER'] == 1 ? "zval observer_retval" : "",
        // 2处
        "/ZEND_OBSERVER_SET_RETVAL\(\)/" => isset($extra_spec['OBSERVER']) && $extra_spec['OBSERVER'] == 1 ? "if (!return_value) { return_value = &observer_retval; }" : "",
        // 2处
        "/ZEND_OBSERVER_FREE_RETVAL\(\)/" => isset($extra_spec['OBSERVER']) && $extra_spec['OBSERVER'] == 1 ? "if (return_value == &observer_retval) { zval_ptr_dtor_nogc(&observer_retval); }" : "",
        // 5处
        "/ZEND_OBSERVER_SAVE_OPLINE\(\)/" => isset($extra_spec['OBSERVER']) && $extra_spec['OBSERVER'] == 1 ? "SAVE_OPLINE()" : "",
        // 11处
        "/ZEND_OBSERVER_FCALL_BEGIN\(\s*(.*)\s*\)/" => isset($extra_spec['OBSERVER']) ?
            ($extra_spec['OBSERVER'] == 0 ? "" : "zend_observer_fcall_begin(\\1)")
            : "",
        // 7处
        "/ZEND_OBSERVER_FCALL_END\(\s*([^,]*)\s*,\s*(.*)\s*\)/" => isset($extra_spec['OBSERVER']) ?
            ($extra_spec['OBSERVER'] == 0 ? "" : "zend_observer_fcall_end(\\1, \\2)")
            : "",
    );
	// 替换源码中的内容
    $code = preg_replace(array_keys($specialized_replacements), array_values($specialized_replacements), $code);

	// 无效代码
    if (0 && strpos($code, '{') === 0) {
        $code = "{\n\tfprintf(stderr, \"$name\\n\");\n" . substr($code, 1);
    }
	// 把代码更新到 已选择的 线程模式
    // Updating code according to selected threading model
    switch ($kind) {
		// BYBRID 模式
        case ZEND_VM_KIND_HYBRID:
			// 最后处理这3个正则
            $code = preg_replace_callback(
				//
                array(
					// EXECUTE_DATA
                    "/EXECUTE_DATA(?=[^_])/m",
					// ZEND_VM_DISPATCH_TO_HANDLER
                    "/ZEND_VM_DISPATCH_TO_HANDLER\(\s*([A-Z_]*)\s*\)/m",
					// ZEND_VM_DISPATCH_TO_HELPER
                    "/ZEND_VM_DISPATCH_TO_HELPER\(\s*([A-Za-z_]*)\s*(,[^)]*)?\)/m",
                ),
				// 所有传递的op1和op2都是两个操作对象的类型
                function($matches) use ($spec, $prefix, $op1, $op2, $extra_spec) {
					// strncasecmp 比较前n个字符大小，大小写不敏感
					// execute_data
					// 如果前面一段字符是 EXECUTE_DATA
                    if (strncasecmp($matches[0], "EXECUTE_DATA", strlen("EXECUTE_DATA")) == 0) {
						// 返回 execute_data
                        return "execute_data";
					// 如果前面一段字符是 ZEND_VM_DISPATCH_TO_HANDLER
                    } else if (strncasecmp($matches[0], "ZEND_VM_DISPATCH_TO_HANDLER", strlen("ZEND_VM_DISPATCH_TO_HANDLER")) == 0) {
                        global $opcodes, $opnames;
						// 取得匹配到的名字
                        $name = $matches[1];
						// 取得对应的操作码
                        $opcode = $opcodes[$opnames[$name]];
						// 返回源码： 跳转到指定的label
						// 所有传递的op1和op2都是两个操作对象的类型
                        return "goto " . opcode_name($name, $spec, $op1, $op2, $extra_spec) . "_LABEL";
					// 其他情况 ：前面是 ZEND_VM_DISPATCH_TO_HELPER
                    } else {
                        // ZEND_VM_DISPATCH_TO_HELPER
						// 如果 helper 是 hot
                        if (is_hot_helper($matches[1])) {
							// 如果有第二个参数, 例如 ZEND_VM_DISPATCH_TO_HELPER(zend_fetch_var_address_helper, type, BP_VAR_W);
                            if (isset($matches[2])) {
								// 额外的参数
                                // extra args
								// 把参数格式替换成 $1 = $2，例如 type=BP_VAR_W
                                $args = preg_replace("/,\s*([A-Za-z0-9_]*)\s*,\s*([^,)\s]*)\s*/", "$1 = $2; ", $matches[2]);
								// ？没见goto前面有字符的，所有传递的op1和op2都是两个操作对象的类型
                                return $args . "goto " . helper_name($matches[1], $spec, $op1, $op2, $extra_spec) . "_LABEL";
                            }
							// 没有第二个参数
							// 所有传递的op1和op2都是两个操作对象的类型
                            return "goto " . helper_name($matches[1], $spec, $op1, $op2, $extra_spec) . "_LABEL";
                        }
						// 如果helper 类型不是 hot
						
						// 如果有第二个参数
                        if (isset($matches[2])) {
							// 处理额外的参数
                            // extra args
							// 跳过前两个字符
                            $args = substr(preg_replace("/,\s*[A-Za-z0-9_]*\s*,\s*([^,)\s]*)\s*/", ", $1", $matches[2]), 2);
							// 所有传递的op1和op2都是两个操作对象的类型
                            return "ZEND_VM_TAIL_CALL(" . helper_name($matches[1], $spec, $op1, $op2, $extra_spec) . "(" . $args. " ZEND_OPCODE_HANDLER_ARGS_PASSTHRU_CC))";
                        }
						// 如果 没有第二个参数
						// 所有传递的op1和op2都是两个操作对象的类型
                        return "ZEND_VM_TAIL_CALL(" . helper_name($matches[1], $spec, $op1, $op2, $extra_spec) . "(ZEND_OPCODE_HANDLER_ARGS_PASSTHRU))";
                    }
                },
                $code);
            break;
		// CALL 模式，用到
        case ZEND_VM_KIND_CALL:
            $code = preg_replace_callback(
                array(
                    "/EXECUTE_DATA(?=[^_])/m",
                    "/ZEND_VM_DISPATCH_TO_HANDLER\(\s*([A-Z_]*)\s*\)/m",
                    "/ZEND_VM_DISPATCH_TO_HELPER\(\s*([A-Za-z_]*)\s*(,[^)]*)?\)/m",
                ),
				// 所有传递的op1和op2都是两个操作对象的类型
                function($matches) use ($spec, $prefix, $op1, $op2, $extra_spec, $name) {
                    if (strncasecmp($matches[0], "EXECUTE_DATA", strlen("EXECUTE_DATA")) == 0) {
                        return "execute_data";
                    } else if (strncasecmp($matches[0], "ZEND_VM_DISPATCH_TO_HANDLER", strlen("ZEND_VM_DISPATCH_TO_HANDLER")) == 0) {
                        global $opcodes, $opnames;

                        $handler = $matches[1];
                        $opcode = $opcodes[$opnames[$handler]];
                        $inline =
							// 如果用 ZEND_VM_KIND_HYBRID 模式生成
                            ZEND_VM_KIND == ZEND_VM_KIND_HYBRID &&
                            isset($opcode["use"]) &&
							// 所有传递的op1和op2都是两个操作对象的类型
                            is_hot_handler($opcode["hot"], $op1, $op2, $extra_spec) &&
							// 所有传递的op1和op2都是两个操作对象的类型
                            is_hot_handler($opcodes[$opnames[$name]]["hot"], $op1, $op2, $extra_spec) ?
                            "_INLINE" : "";
						// 所有传递的op1和op2都是两个操作对象的类型
                        return "ZEND_VM_TAIL_CALL(" . opcode_name($handler, $spec, $op1, $op2, $extra_spec) . $inline . "_HANDLER(ZEND_OPCODE_HANDLER_ARGS_PASSTHRU))";
                    } else {
                        // ZEND_VM_DISPATCH_TO_HELPER
                        if (isset($matches[2])) {
                            // extra args
                            $args = substr(preg_replace("/,\s*[A-Za-z0-9_]*\s*,\s*([^,)\s]*)\s*/", ", $1", $matches[2]), 2);
							// 所有传递的op1和op2都是两个操作对象的类型
                            $t = "ZEND_VM_TAIL_CALL(" . helper_name($matches[1], $spec, $op1, $op2, $extra_spec) . "(" . $args. " ZEND_OPCODE_HANDLER_ARGS_PASSTHRU_CC))";
							return $t;
                        }
						// 所有传递的op1和op2都是两个操作对象的类型
                        return "ZEND_VM_TAIL_CALL(" . helper_name($matches[1], $spec, $op1, $op2, $extra_spec) . "(ZEND_OPCODE_HANDLER_ARGS_PASSTHRU))";
						return $t;
                    }
                },
                $code);
            break;
		// 其他运行模式 ...
    }

	// 删除没必要的 分号;
    /* Remove unnecessary ';' */
    $code = preg_replace('/^\s*;\s*$/m', '', $code);

	// 删除没必要的 制表符
    /* Remove WS */
    $code = preg_replace('/[ \t]+\n/m', "\n", $code);

	// 输出到文件
    out($f, $code);
}

// ing4, 跳过 额外的专用函数 ：gen_handler，gen_helper，gen_labels 用到
// 第一，第二个操作对象，额外规则
// 所有传递的op1和op2都是两个操作对象的类型
function skip_extra_spec_function($op1, $op2, $extra_spec) {
    global $commutative_order;

	// 如果规则不允许两个常量，但两个操作对象都是常量
    if (isset($extra_spec["NO_CONST_CONST"]) &&
        $op1 == "CONST" && $op2 == "CONST") {
		// 路过无用的常量处理器
        // Skip useless constant handlers
        return true;
    }

	// 如果规规则要求交换，并且 第一个操作对象的顺序号小于第二个
    if (isset($extra_spec["COMMUTATIVE"]) &&
        $commutative_order[$op1] < $commutative_order[$op2]) {
		// 跳过重复的交换 处理器
        // Skip duplicate commutative handlers
        return true;
    }

	// 其他情况不跳过
    return false;
}

// 所有传递的op1和op2都是两个操作对象的类型
// ing3, 是否是热助手 ：gen_handler
// 所有传递的op1和op2都是两个操作对象的类型
function is_hot_handler($hot, $op1, $op2, $extra_spec) {
	// 如果关闭智能分支，返回false
    if (isset($extra_spec["SMART_BRANCH"]) && $extra_spec["SMART_BRANCH"] == 0) {
        return false;
    }
	// 如果开启了 观察者模式 返回false
    if (isset($extra_spec["OBSERVER"]) && $extra_spec["OBSERVER"] == 1) {
        return false;
    }
	// 情况1: HOT_ 或 INLINE_ 前缀，算hot
    if ($hot === 'HOT_' || $hot === 'INLINE_') {
        return true;
	// 情况2: 不允许常量
    } else if ($hot === 'HOT_NOCONST_') {
		// 并且 op1 不是常量
        return ($op1 !== 'CONST');
	// 情况3: 不允许两个常量
    } else if ($hot === 'HOT_NOCONSTCONST_') {
		// 并且有一个不是常量
        return (($op1 !== 'CONST') || ($op2 !== 'CONST')) ;
	// 情况4: 要求是对象
    } else if ($hot === 'HOT_OBJ_') {
		// op1 是 UNUSED 或 op1 是 CV 并且 op2 是常量
        return (($op1 === 'UNUSED') || ($op1 === 'CV')) && ($op2 === 'CONST');
	// 情况5: 如果是 HOT_SEND_
    } else if ($hot === 'HOT_SEND_') {
		// 扩展规则里必须包含 QUICK_ARG
        return !empty($extra_spec["QUICK_ARG"]);
    } else {
        return false;
    }
}
// 所有传递的op1和op2都是两个操作对象的类型
// ing3, 是否是冷助手：gen_handler
function is_cold_handler($hot, $op1, $op2, $extra_spec) {
	// 标识是 COLD_，返回 true
    if ($hot === 'COLD_') {
        return true;
	// 如果有附加规则 OBSERVER，并且启用 ，返回 true
    } else if (isset($extra_spec["OBSERVER"]) && $extra_spec["OBSERVER"] == 1) {
        return true;
	// 常量
    } else if ($hot === 'COLD_CONST_') {
		// 必须第一个是常量
        return ($op1 === 'CONST');
	// 两个操作对象都是常量
    } else if ($hot === 'COLD_CONSTCONST_') {
		// 必须两个都是常量
        return ($op1 === 'CONST' && $op2 === 'CONST');
	// 热对象
    } else if ($hot === 'HOT_OBJ_') {
		// 必须第一个是常量
        return ($op1 === 'CONST');
	// 热非常量数据
    } else if ($hot === 'HOT_NOCONST_') {
		// 必须第一个是常量
        return ($op1 === 'CONST');
	// 热，不允许两个常量
    } else if ($hot === 'HOT_NOCONSTCONST_') {
		// 必须两个都是常量
        return ($op1 === 'CONST' && $op2 === 'CONST');
    } else {
		// 其他都false
        return false;
    }
}

// 所有传递的op1和op2都是两个操作对象的类型
// clear, 是否是行内混合处理器（$hot === 'INLINE_'）：gen_handler。要这么多参数做什么，又不用？
function is_inline_hybrid_handler($name, $hot, $op1, $op2, $extra_spec) {
    return ($hot === 'INLINE_');
}

// 所有传递的op1和op2都是两个操作对象的类型
// ing2, 生成操作码处理器：gen_executor_code
// Generates opcode handler
// zend_vm_execute.h文件句柄, 1, ZEND_VM_KIND, 操作码名，第一个，第二个操作对象，是否有use
// 源码，行号，整个处理器，额外规则，$switch_labels 引用返回
function gen_handler($f, $spec, $kind, $name, $op1, $op2, $use, $code, $lineno, $opcode, $extra_spec = null, &$switch_labels = array()) {
    global $definition_file, $prefix, $opnames, $gen_order;

    static $used_observer_handlers = array();
	// 如果处理器有别名，并且 （$spec==1  或 不是用 ZEND_VM_KIND_SWITCH 模式生成）
    if (isset($opcode['alias']) && ($spec || $kind != ZEND_VM_KIND_SWITCH)) {
		// var_dump($opcode);
        return;
    }
	// 所有传递的op1和op2都是两个操作对象的类型
	// 如果  $spec==1 并且 此处理器不合规则，跳过
    if ($spec && skip_extra_spec_function($op1, $op2, $extra_spec)) {
        return;
    }

	// 为 "cold"类型，两个都是常量的情况，跳过 SMART_BRANCH 规则
    /* Skip SMART_BRANCH specialization for "cold" CONST_CONST instructions */
	// 如果额外规则包含 SMART_BRANCH
    if (isset($extra_spec["SMART_BRANCH"])) {
		// 如果 hot 类型是 HOT_NOCONSTCONST_（hot不允许两个常量） 或 COLD_CONSTCONST_（cold允许两个常量）
        if ($opcode["hot"] === 'HOT_NOCONSTCONST_'
         || $opcode["hot"] === 'COLD_CONSTCONST_') {
			// 如果两个操作对象都是常量
            if (($op1 === 'CONST') && ($op2 === 'CONST')) {
				// 如果 SMART_BRANCH 值为否，删除它
                if ($extra_spec["SMART_BRANCH"] == 0) {
                    unset($extra_spec["SMART_BRANCH"]);
				// 直接返回
                } else {
                    return;
                }
            }
        }
    }
	
	// 为命名参数 跳过 QUICK_ARG 规则
    /* Skip QUICK_ARG specialization for named parameters */
	// 如果额外规则包含 QUICK_ARG 
    if (isset($extra_spec["QUICK_ARG"])) {
		// 第二个操作对象是常量
        if ($op2 === "CONST") {
			// 如果 QUICK_ARG 值为否，删除它
            if ($extra_spec["QUICK_ARG"] == 0) {
                unset($extra_spec["QUICK_ARG"]);
            } else {
				// 直接返回
                return;
            }
        }
    }

	// 为 OBSERVER 处理器跳过所有规则 
    /* Skip all specialization for OBSERVER handlers */
	// 如果额外规则包含 OBSERVER 并且值为 1
    if (isset($extra_spec["OBSERVER"]) && $extra_spec["OBSERVER"] == 1) {
		// 如果有 RETVAL 规则
        if (isset($extra_spec["RETVAL"])) {
			// 值为否，删除它
            if ($extra_spec["RETVAL"] == 0) {
                unset($extra_spec["RETVAL"]);
			// 直接返回
            } else {
                return;
            }
        }
		// 如果其中一个操作对象类型不是 ANY
        if ($op1 != "ANY" || $op2 != "ANY") {
			// ？
            if (!isset($used_observer_handlers[$kind][$opcode["op"]])) {
				// 
                $used_observer_handlers[$kind][$opcode["op"]] = true;
                $op1 = "ANY";
                $op2 = "ANY";
			// 直接返回
            } else {
                return;
            }
        }
    }

	// 如果有行调试选项
    if (ZEND_VM_LINES) {
		// 复制所有行的代码
        out($f, "#line $lineno \"$definition_file\"\n");
    }

	// 按照所选的线程模式， 生成操作码处理器的进入点
    // Generate opcode handler's entry point according to selected threading model
    $additional_func = false;
	// 规则名 +（如果 $spec 是1，添加 _SPEC 前缀）+ 两个操作对象类型的前缀 + （如果 $spec 是1，按额外规则生成后缀）
    $spec_name = $name.($spec?"_SPEC":"").$prefix[$op1].$prefix[$op2].($spec?extra_spec_name($extra_spec):"");
	// 按生成模式处理
    switch ($kind) {
		// HYBRID
        case ZEND_VM_KIND_HYBRID:
			// 所有传递的op1和op2都是两个操作对象的类型
			// 如果是行内混合处理器（$hot === 'INLINE_'）
            if (is_inline_hybrid_handler($name, $opcode["hot"], $op1, $op2, $extra_spec)) {
				// 在内存中生成代码
                $out = fopen('php://memory', 'w+');
				// 生成代码
                gen_code($out, $spec, $kind, $code, $op1, $op2, $name, $extra_spec);
                rewind($out);
                $code =
                      "\t\t\tHYBRID_CASE({$spec_name}):\n"
                    . "\t\t\t\tVM_TRACE($spec_name)\n"
                    . stream_get_contents($out);
				// 关闭内存
                fclose($out);
			// 其他情况
            } else {
                $inline =
                    isset($opcode["use"]) &&
					// 所有传递的op1和op2都是两个操作对象的类型
                    is_hot_handler($opcode["hot"], $op1, $op2, $extra_spec) ?
                    "_INLINE" : "";
                $code =
                      "\t\t\tHYBRID_CASE({$spec_name}):\n"
                    . "\t\t\t\tVM_TRACE($spec_name)\n"
                    . "\t\t\t\t{$spec_name}{$inline}_HANDLER(ZEND_OPCODE_HANDLER_ARGS_PASSTHRU);\n"
                    . "\t\t\t\tHYBRID_BREAK();\n";
            }
			// 
            if (is_array($gen_order)) {
                $gen_order[$spec_name] = $code;
            } else {
                out($f, $code);
            }
            return;
		// 用到
        case ZEND_VM_KIND_CALL:
			// 如果用 ZEND_VM_KIND_HYBRID 模式生成
            // 所有传递的op1和op2都是两个操作对象的类型
			if ($opcode["hot"] && ZEND_VM_KIND == ZEND_VM_KIND_HYBRID && is_hot_handler($opcode["hot"], $op1, $op2, $extra_spec)) {
                if (isset($opcode["use"])) {
                    out($f,"static zend_always_inline ZEND_OPCODE_HANDLER_RET ZEND_FASTCALL {$spec_name}_INLINE_HANDLER(ZEND_OPCODE_HANDLER_ARGS)\n");
                    $additional_func = true;
                } else {
                    out($f,"static ZEND_VM_HOT ZEND_OPCODE_HANDLER_RET ZEND_FASTCALL {$spec_name}_HANDLER(ZEND_OPCODE_HANDLER_ARGS)\n");
                }
			// 所有传递的op1和op2都是两个操作对象的类型
            } else if ($opcode["hot"] && is_cold_handler($opcode["hot"], $op1, $op2, $extra_spec)) {
                out($f,"static ZEND_VM_COLD ZEND_OPCODE_HANDLER_RET ZEND_FASTCALL {$spec_name}_HANDLER(ZEND_OPCODE_HANDLER_ARGS)\n");
            } else {
                out($f,"static ZEND_OPCODE_HANDLER_RET ZEND_FASTCALL {$spec_name}_HANDLER(ZEND_OPCODE_HANDLER_ARGS)\n");
            }
            break;
        // 其他运行模式 ...
    }

	// 生成操作码处理器代码
    // Generate opcode handler's code
	// zend_vm_execute.h文件句柄, 1, ZEND_VM_KIND, 操作码名，第一第二个操作对象，函数名，额外规则
	// 所有传递的op1和op2都是两个操作对象的类型
    gen_code($f, $spec, $kind, $code, $op1, $op2, $name, $extra_spec);

	// 生成附加函数
    if ($additional_func) {
		// 附加函数 ZEND_OPCODE_HANDLER_RET ZEND_FASTCALL {$spec_name}_HANDLER
        out($f,"static ZEND_OPCODE_HANDLER_RET ZEND_FASTCALL {$spec_name}_HANDLER(ZEND_OPCODE_HANDLER_ARGS)\n");
        out($f,"{\n");
		// 此规则的 尾调用行内处理器  ZEND_VM_TAIL_CALL({$spec_name}_INLINE_HANDLER
        out($f,"\tZEND_VM_TAIL_CALL({$spec_name}_INLINE_HANDLER(ZEND_OPCODE_HANDLER_ARGS_PASSTHRU));\n");
        out($f,"}\n");
        out($f,"\n");
    }
}
// 所有传递的op1和op2都是两个操作对象的类型
// ing3, 生成助手：gen_executor_code 用到
// Generates helper
function gen_helper($f, $spec, $kind, $name, $op1, $op2, $param, $code, $lineno, $inline, $cold = false, $hot = false, $extra_spec = null) {
    global $definition_file, $prefix;
	// 如果用 ZEND_VM_KIND_HYBRID 模式生成
    if ($kind == ZEND_VM_KIND_HYBRID && !$hot) {
        return;
    }
	// 如果规则存在，并且需要跳过
	// 所有传递的op1和op2都是两个操作对象的类型
    if ($spec && skip_extra_spec_function($op1, $op2, $extra_spec)) {
        return;
    }
	
	// 如果要要附带原来的代码
    if (ZEND_VM_LINES) {
        out($f, "#line $lineno \"$definition_file\"\n");
    }

	// 按额外规则 ，生成后缀
    $spec_name = $name.($spec?"_SPEC":"").$prefix[$op1].$prefix[$op2].($spec?extra_spec_name($extra_spec):"");
//var_dump($kind,$spec_name);
	// 按照选择的线程模式，生成助手实体
    // Generate helper's entry point according to selected threading model
    switch ($kind) {
		// HYBRID 模式
        case ZEND_VM_KIND_HYBRID:
			// 直接输出  规则名字_LABEL:
            out($f, $spec_name . "_LABEL:\n");
            break;
		// CALL 模式，用到
        case ZEND_VM_KIND_CALL:
			// 有 $inline 标记，？这是什么
            if ($inline) {
                $zend_attributes = " zend_always_inline";
                $zend_fastcall = "";
			// 没有 $inline 标记
            } else {
				// 如果有 $cold 标记，多一个ZEND_COLD前缀
                if ($cold) {
                    $zend_attributes = " zend_never_inline ZEND_COLD";
				// 没有 $code 标记
                } else {
                    $zend_attributes = " zend_never_inline";
                }
				// 
                $zend_fastcall = " ZEND_FASTCALL";
            }
			// 参数为空
            if ($param == null) {
			  // 没有参数的 助手 
              // Helper without parameters
                out($f, "static$zend_attributes ZEND_OPCODE_HANDLER_RET$zend_fastcall $spec_name(ZEND_OPCODE_HANDLER_ARGS)\n");
			// 参数不为空
            } else {
			  // 有参数的助手 
              // Helper with parameter
                out($f, "static$zend_attributes ZEND_OPCODE_HANDLER_RET$zend_fastcall $spec_name($param ZEND_OPCODE_HANDLER_ARGS_DC)\n");
            }
            break;
		// 其他运行模式 ...
    }

	// 生成助手源码
    // Generate helper's code
	// 所有传递的op1和op2都是两个操作对象的类型
    gen_code($f, $spec, $kind, $code, $op1, $op2, $name, $extra_spec);
}


// clear, 生成 null 标签：gen_labels 用到
function gen_null_label($f, $kind, $prolog) {
	switch ($kind) {
		// CALL 模式
        case ZEND_VM_KIND_CALL:
            out($f,$prolog."ZEND_NULL_HANDLER,\n");
            break;
		// 其他运行模式 ...
		// GOTO 模式
        case ZEND_VM_KIND_GOTO:
            out($f,$prolog."(void*)&&ZEND_NULL_LABEL,\n");
            break;
    }
}

// 330行， 最复杂的方法之一了
// ing3, 1.创建 zend_vm_handlers.h 中的内容。2.生成$specs。
// 生成 操作码处理器 数组（专用|非专用）：gen_executor 调用
// Generates array of opcode handlers (specialized or unspecialized)
function gen_labels($f, $spec, $kind, $prolog, &$specs, $switch_labels = array()) {
    global $opcodes, $opnames, $op_types, $prefix, $op_types_ex;
//var_dump($spec,$kind);
    $list = [];
    $next = 0;
    $label = 0;
	// 如果有传入模式，一般应该是走这个分支
    if ($spec) {
	  // 为专用执行器创建 标签
      // Emit labels for specialized executor

		// 遍历 操作码编号索引的 处理器列表
      // For each opcode in opcode number order
	  // 给每个处理器来一大套操作
        foreach ($opcodes as $num => $dsc) {
			// var_dump($num.'_'.isset($dsc['alias']));
			// 如果处理器是别名
            if (isset($dsc['alias'])) {
				// 在规则表中，使用原名称的配置
                $specs[$num] = $specs[$opnames[$dsc['alias']]];
				// 跳过其他操作
                continue;
            }
			// 规则表中中添加这个操作码编号 $num
            $specs[$num] = "$label"; // var_dump("$label");
			// 先清空操作对象规则和 额外规则 
            $spec_op1 = $spec_op2 = $spec_extra = false;
			// 两个操作对象类型都是 ANY
            $def_op1_type = $def_op2_type = "ANY";
			// 下一个编号 
            $next = $num + 1;
			// 如果op1有类型限制 ，并且不是ANY
            if (isset($dsc["op1"]) && !isset($dsc["op1"]["ANY"])) {
                $count = 0;
				// 遍历所有类型
                foreach ($op_types_ex as $t) {
					// 如果p1支持此类型
                    if (isset($dsc["op1"][$t])) {
						// 更新类型名，多个的话这里是最后一个
                        $def_op1_type = $t;
						// 每找到一个支持的类型，计数+1
                        $count++;
                    }
                }
				// 如果有1个以上的类型
                if ($count > 1) {
					// 有专用规则 
                    $spec_op1 = true;
					// 添加专用规则标记
                    $specs[$num] .= " | SPEC_RULE_OP1";
					// 类型为 ANY
                    $def_op1_type = "ANY";
                }
            }
			// 第二个操作对象，逻辑与上面相似
            if (isset($dsc["op2"]) && !isset($dsc["op2"]["ANY"])) {
                $count = 0;
                foreach ($op_types_ex as $t) {
                    if (isset($dsc["op2"][$t])) {
                        $def_op2_type = $t;
						// 每找到一个支持的类型，计数+1
                        $count++;
                    }
                }
                if ($count > 1) {
                    $spec_op2 = true;
                    $specs[$num] .= " | SPEC_RULE_OP2";
                    $def_op2_type = "ANY";
                }
            }
			// 把多个扩展标记合并到一起
            $spec_extra = call_user_func_array("array_merge", extra_spec_handler($dsc) ?: array(array()));
			// var_dump(json_encode($dsc),json_encode($spec_extra));//die;
			// 根据$extra_spec，返回额外专用标记列表
            $flags = extra_spec_flags($spec_extra);
			// 规则里添加额外专用标记
            if ($flags) {
                $specs[$num] .= " | " . implode(" | ", $flags);
            }
			// 如果序号大于256
            if ($num >= 256) {
				// 把规则 记录进来
                $opcodes[$num]['spec_code'] = $specs[$num];
				// 删除这个规则 
                unset($specs[$num]);
            }
			// var_dump($specs);die;
			
			// ** 到这里就把 specs 生成完了 ** 

			// 1. 闭包：处理第一个操作对象的类型, 计算出最终使用的类型
            $foreach_op1 = function($do) use ($dsc, $op_types) {
				// 返回一个闭包
                return function($_, $op2) use ($do, $dsc, $op_types) {
					// 遍历第所有操作对象的类型
                    // For each op1.op_type except ANY
                    foreach ($op_types as $op1) {
						// 如果不是ANY， 那就是 "CONST", "TMP", "VAR", "UNUSED", "CV" 之一
                        if ($op1 != "ANY") {
							// 如果不存在这个类型
                            if (!isset($dsc["op1"][$op1])) {
								// 如果类型是 TMP 或 VAR
                                if ($op1 == "TMP" || $op1 == "VAR") {
									// 如果支持 TMPVAR 类型
                                    if (isset($dsc["op1"]["TMPVAR"])) {
										// 使用 TMPVAR
                                        $op1 = "TMPVAR";
									// 如果支持 TMPVARCV 类型
                                    } else if (isset($dsc["op1"]["TMPVARCV"])) {
										// 使用 TMPVARCV 类型
                                        $op1 = "TMPVARCV";
									// 其他情况
                                    } else {
										// 使用 ANY
                                        $op1 = "ANY";
                                    }
								// 如果第一个操作对象类型是CV 并且支持 TMPVARCV 类型
                                } else if ($op1 == "CV" && isset($dsc["op1"]["TMPVARCV"])) {
									// 返回 TMPVARCV 类型
                                    $op1 = "TMPVARCV";
								// 其他情况
                                } else {
									// 使用专用处理器
                                    // Try to use unspecialized handler
                                    $op1 = "ANY";
                                }
                            }
							// 所有传递的op1和op2都是两个操作对象的类型
                            $do($op1, $op2);
                        }
						// 不处理any
                    }
                };
            };
			// 2. 闭包：处理第二个操作对象的类型, 计算出最终使用的类型
            $foreach_op2 = function($do) use ($dsc, $op_types) {
                return function($op1, $_) use ($do, $dsc, $op_types) {
					// 遍历所有类型
                    // For each op2.op_type except ANY
                    foreach ($op_types as $op2) {
						// 如果类型是不 ANY， 那就是 "CONST", "TMP", "VAR", "UNUSED", "CV" 之一
                        if ($op2 != "ANY") {
							// 如果不支持 此类型
                            if (!isset($dsc["op2"][$op2])) {
								// 如果类型是 TMP 或 VAR
                                if ($op2 == "TMP" || $op2 == "VAR") {
									// 如果支持 TMPVAR
                                    if (isset($dsc["op2"]["TMPVAR"])) {
										// 类型为 TMPVAR
                                        $op2 = "TMPVAR";
									// 如果支持 TMPVARCV
                                    } else if (isset($dsc["op2"]["TMPVARCV"])) {
										// 类型为 TMPVARCV
                                        $op2 = "TMPVARCV";
									// 其他情况
                                    } else {
										// 类型为 ANY
                                        $op2 = "ANY";
                                    }
								// 如果类型是 CV 并且 支持 TMPVARCV
                                } else if ($op2 == "CV" && isset($dsc["op2"]["TMPVARCV"])) {
									// 类型为 TMPVARCV
                                    $op2 = "TMPVARCV";
								// 其他情况
                                } else {
									// 使用非专用处理器
                                    // Try to use unspecialized handler
                                    $op2 = "ANY";
                                }
                            }
							// 所有传递的op1和op2都是两个操作对象的类型
                            $do($op1, $op2);
                        }
                    }
                };
            };
			// 3.1 处理OP_DATA规则 
            $foreach_op_data = function($do) use ($dsc, $op_types) {
				// 所有传递的op1和op2都是两个操作对象的类型
                return function($op1, $op2, $extra_spec = array()) use ($do, $dsc, $op_types) {
					// 遍历所有类型
                    // For each op_data.op_type except ANY
                    foreach ($op_types as $op_data) {
						// 如果类型不是 ANY
                        if ($op_data != "ANY") {
							// 如果不支持此类型
                            if (!isset($dsc["spec"]["OP_DATA"][$op_data])) {
								// 如果类型是 TMP 或 VAR
                                if ($op_data == "TMP" || $op_data == "VAR") {
									// 如果支持 TMPVAR
                                    if (isset($dsc["spec"]["OP_DATA"]["TMPVAR"])) {
										// 类型为 TMPVAR
                                        $op_data = "TMPVAR";
									// 如果支持 
                                    } else if (isset($dsc["spec"]["OP_DATA"]["TMPVARCV"])) {
										// 类型为 TMPVARCV
                                        $op_data = "TMPVARCV";
									// 其他情况 
                                    } else {
										// 使用非专用处理器
                                        // Try to use unspecialized handler
										// 类型为 ANY
                                        $op_data = "ANY";
                                    }
								// 如果类型是 CV 并且支持 TMPVARCV
                                } else if ($op_data == "CV" && isset($dsc["OP_DATA"]["TMPVARCV"])) {
									// 类型为 TMPVARCV
                                    $op_data = "TMPVARCV";
								// 其他情况
                                } else {
									// 使用非专用处理器
                                    // Try to use unspecialized handler
									// 类型为 ANY
                                    $op_data = "ANY";
                                }
                            }
							// 所有传递的op1和op2都是两个操作对象的类型
                            $do($op1, $op2, array("OP_DATA" => $op_data) + $extra_spec);
                        }
                    }
                };
            };
			// 3.2 处理额外规则 
            $foreach_extra_spec = function($do, $spec) use ($dsc) {
				// 所有传递的op1和op2都是两个操作对象的类型
                return function($op1, $op2, $extra_spec = array()) use ($do, $spec, $dsc) {
					// 遍历所有规则 
                    foreach ($dsc["spec"][$spec] as $val) {
						// 所有传递的op1和op2都是两个操作对象的类型
						// 把额外规则附加到 规则 后面一块处理。相同索引号后面不会覆盖前面的
                        $do($op1, $op2, array($spec => $val) + $extra_spec);
                    }
                };
            };
			// 4.生成器， 所有传递的op1和op2都是两个操作对象的类型
			// 这里面会创建很多空label
            $generate = function ($op1, $op2, $extra_spec = array()) use ($f, $kind, $dsc, $prefix, $prolog, $num, $switch_labels, &$label, &$list) {
                global $commutative_order;

				// 检验是否定义了专用处理器
                // Check if specialized handler is defined
                /* TODO: figure out better way to signal "specialized and not defined" than an extra lookup */
                // 如果op1 op2都支持，
				if (isset($dsc["op1"][$op1]) &&
                    isset($dsc["op2"][$op2]) &&
					// 并且（没有 OP_DATA 或 有 OP_DATA 的特殊规则）
                    (!isset($extra_spec["OP_DATA"]) || isset($dsc["spec"]["OP_DATA"][$extra_spec["OP_DATA"]]))) {
					//if(isset($extra_spec["OP_DATA"])) var_dump($extra_spec["OP_DATA"]);
					// 如果需要跳过此额外规则 ，所有传递的op1和op2都是两个操作对象的类型
                    if (skip_extra_spec_function($op1, $op2, $extra_spec)) {
						// 创建 null label
                        gen_null_label($f, $kind, $prolog);
						// 值为null 的第1/2种情况
                        $list[$label] = null;
						// 数量增加
                        $label++;
                        return;
                    }
					// 为cold双常量的情况， 跳过智能分支
                    /* Skip SMART_BRANCH specialization for "cold" CONST_CONST instructions */
					// 有智能分枝标记
                    if (isset($extra_spec["SMART_BRANCH"])) {
						// hot 不允许双常量，或COLD允许双常量
                        if ($dsc["hot"] === 'HOT_NOCONSTCONST_'
                         || $dsc["hot"] === 'COLD_CONSTCONST_') {
							// 并且两个都是常量
                            if (($op1 === 'CONST') && ($op2 === 'CONST')) {
								// 删除智能分支
                                unset($extra_spec["SMART_BRANCH"]);
                            }
                        }
                    }
					
					// 为有名字的参数路过 QUICK_ARG  规则 
                    /* Skip QUICK_ARG specialization for named parameters */
					// 如果有 QUICK_ARG
                    if (isset($extra_spec["QUICK_ARG"])) {
						// 第二个操作对象类型是常量
                        if ($op2 === "CONST") {
							// 删除 QUICK_ARG
                            unset($extra_spec["QUICK_ARG"]);
                        }
                    }

					// 为观察者处理器 跳过所有规则 
                    /* Skip all specialization for OBSERVER handlers */
                    if (isset($extra_spec["OBSERVER"]) && $extra_spec["OBSERVER"] == 1) {
						// 如果有 RETVAL 规则 
                        if (isset($extra_spec["RETVAL"])) {
							// 删除它
                            unset($extra_spec["RETVAL"]);
                        }
						// 如果两个操作对象有一个不是ANY
                        if ($op1 != "ANY" || $op2 != "ANY") {
							// 两个都换成ANY
                            $op1 = "ANY";
                            $op2 = "ANY";
                        }
                    }
					
					// 指针发送到规则处理器
                    // Emit pointer to specialized handler
					// 按额外规则 ，生成后缀
                    $spec_name = $dsc["op"]."_SPEC".$prefix[$op1].$prefix[$op2].extra_spec_name($extra_spec);
					// 按模式划分
					switch ($kind) {
						// call模式
                        case ZEND_VM_KIND_CALL:
                            out($f,"$prolog{$spec_name}_HANDLER,\n");
                            break;
						// 其他运行模式 ...
						// goto模式
                        case ZEND_VM_KIND_GOTO:
                            out($f,$prolog."(void*)&&{$spec_name}_LABEL,\n");
                            break;
						// HYBRID模式不生成任何东西
                    }
					// 值为规则名
                    $list[$label] = $spec_name;
                    $label++;
				// 其他情况 
                } else {
					// 指针指向未知操作码
                    // Emit pointer to handler of undefined opcode
                    gen_null_label($f, $kind, $prolog);
					// 值为null 的第2/2种情况
                    $list[$label] = null;
					// 数量+1
                    $label++;
                }
            };

			// 4. 最后调用的是 $generate
            $do = $generate;
			// 如果有额外的标记
            if ($spec_extra) {
				// 遍历额外规则 
                foreach ($spec_extra as $extra => $devnull) {
					// OP_DATA
                    if ($extra == "OP_DATA") {
						// 3.1
                        $do = $foreach_op_data($do);
					// 其他额外规则 
                    } else {
						// 3.2
                        $do = $foreach_extra_spec($do, $extra);
                    }
                }
            }
			// 如果有操作码2的规则 
            if ($spec_op2) {
				// 2. 处理第二个操作码
                $do = $foreach_op2($do);
            }
			// 如果有操作码1的规则 
            if ($spec_op1) {
				// 1. 处理第一个操作码
                $do = $foreach_op1($do);
            }
			// 明显这里倒着加是为了正着运行。
			// 这个do最多包了4层，套这么多层到底有什么实际的好处呢（封装）
			// 传入的是op1的类型 和 op2的类型
            $do($def_op1_type, $def_op2_type);
        }
    // 如果没有传入 $spec 模式
	} else {
		var_dump(1);
		// 其他运行模式 ...
    }

	// 添加最后一个处理器的label
    // Emit last handler's label (undefined opcode)
	switch ($kind) {
		// call模式
        case ZEND_VM_KIND_CALL:
            out($f,$prolog."ZEND_NULL_HANDLER\n");
            break;
		// 其他运行模式 ...
		// godo模式
        case ZEND_VM_KIND_GOTO:
            out($f,$prolog."(void*)&&ZEND_NULL_LABEL\n");
            break;
    }
	// 
    $specs[$num + 1] = "$label";

	// 写入 zend_vm_handlers.h
    $l = fopen(__DIR__ . "/zend_vm_handlers_2.h", "w+") or die("ERROR: Cannot create zend_vm_handlers.h\n");
	// 先写宏名称
    out($l, "#define VM_HANDLERS(_) \\\n");
	// 遍历所有，序号=>名字，这里面有大量重复的
    foreach ($list as $n => $name) {
		// 跳过值为null的
        if (null !== $name) {
            out($l, "\t_($n, $name) \\\n");
        }
    }
	// 最后加一个null的
    out($l, "\t_($n+1, ZEND_NULL)\n");
    fclose($l);
}

// ing4, 生成专用 规则列表语句
// Generates specialized offsets
function gen_specs($f, $prolog, $specs) {
    $lastdef = array_pop($specs);
    $last = 0;
	// 遍历规则列表
    foreach ($specs as $num => $def) {
		// 
        while (++$last < $num) {
            out($f, "$prolog$lastdef,\n");
        }
        $last = $num;
        out($f, "$prolog$def,\n");
    }
	// 
    while ($last++ < 255) {
        out($f, "$prolog$lastdef,\n");
    }
}

// ing4, 生成未定义操作码的处理器（CALL 线程模式）：gen_executor_code(
// Generates handler for undefined opcodes (CALL threading model)
function gen_null_handler($f) {
    static $done = 0;

    // New and all executors with CALL threading model can use the same handler
	
	// 对于未定义的操作码只生成 1 次
    // for undefined opcodes, do we emit code for it only once
    if (!$done) {
        $done = 1;
        out($f,"static ZEND_OPCODE_HANDLER_RET ZEND_FASTCALL ZEND_NULL_HANDLER(ZEND_OPCODE_HANDLER_ARGS)\n");
        out($f,"{\n");
        out($f,"\tUSE_OPLINE\n");
        out($f,"\n");
        out($f,"\tSAVE_OPLINE();\n");
		// 调用时报错 ： 无效操作码
        out($f,"\tzend_error_noreturn(E_ERROR, \"Invalid opcode %d/%d/%d.\", OPLINE->opcode, OPLINE->op1_type, OPLINE->op2_type);\n");
        out($f,"\tZEND_VM_NEXT_OPCODE(); /* Never reached */\n");
        out($f,"}\n\n");
    }
}

// ing4, 按额外规则 ，生成后缀：helper_name，opcode_name，gen_code，gen_handler，gen_helper 调用
function extra_spec_name($extra_spec) {
    global $prefix;

    $s = "";
	// 如果规则包含 OP_DATA
    if (isset($extra_spec["OP_DATA"])) {
		// 添加 "_OP_DATA" . $extra_spec["OP_DATA"]类型的前缀
        $s .= "_OP_DATA" . $prefix[$extra_spec["OP_DATA"]];
    }
	
	// 如果规则包含 RETVAL
    if (isset($extra_spec["RETVAL"])) {
		// 添加 "_RETVAL_" . USED 或 UNUSED
        $s .= "_RETVAL_".($extra_spec["RETVAL"] ? "USED" : "UNUSED");
    }
	
	// 如果规则包含 QUICK_ARG
    if (isset($extra_spec["QUICK_ARG"])) {
		// 并且启用 QUICK_ARG
        if ($extra_spec["QUICK_ARG"]) {
			// 添加 
            $s .= "_QUICK";
        }
    }
	
	//  如果规则包含 SMART_BRANCH
    if (isset($extra_spec["SMART_BRANCH"])) {
		// 按规则添加 _JMPZ 或 _JMPNZ
        if ($extra_spec["SMART_BRANCH"] == 1) {
			$s .= "_JMPZ";
        } else if ($extra_spec["SMART_BRANCH"] == 2) {
            $s .= "_JMPNZ";
        }
    }
	
	// 如果规则包含 ISSET
    if (isset($extra_spec["ISSET"])) {
		// 未启用 添加 _SET，启用 添加 _EMPTY
        if ($extra_spec["ISSET"] == 0) {
            $s .= "_SET";
        } else {
            $s .= "_EMPTY";
        }
    }
	
	// 如果规则包含 OBSERVER
    if (isset($extra_spec["OBSERVER"])) {
		// 如果规则启用 添加后缀 _OBSERVER
        if ($extra_spec["OBSERVER"]) {
            $s .= "_OBSERVER";
        }
    }
    return $s;
}

// ing4, 根据$extra_spec，返回额外专用标记列表：gen_labels 调用
function extra_spec_flags($extra_spec) {
    $s = array();
    if (isset($extra_spec["OP_DATA"])) {
        $s[] = "SPEC_RULE_OP_DATA";
    }
    if (isset($extra_spec["RETVAL"])) {
        $s[] = "SPEC_RULE_RETVAL";
    }
    if (isset($extra_spec["QUICK_ARG"])) {
        $s[] = "SPEC_RULE_QUICK_ARG";
    }
    if (isset($extra_spec["SMART_BRANCH"])) {
        $s[] = "SPEC_RULE_SMART_BRANCH";
    }
    if (isset($extra_spec["COMMUTATIVE"])) {
        $s[] = "SPEC_RULE_COMMUTATIVE";
    }
    if (isset($extra_spec["ISSET"])) {
        $s[] = "SPEC_RULE_ISSET";
    }
    if (isset($extra_spec["OBSERVER"])) {
        $s[] = "SPEC_RULE_OBSERVER";
    }
    return $s;
}

// ing3, 额外专用处理器，参数是某一个操作码的处理器：gen_executor_code，gen_labels 调用
function extra_spec_handler($dsc) {
    global $op_types_ex;

	// 如果没有特殊标记
    if (!isset($dsc["spec"])) {
        return array(array());
    }
	// 用到 "spec" 的地方：helper_name，opcode_name，gen_labels，extra_spec_handler，gen_vm	
	// 它是由 parse_spec_rules 这个方法生成的
    $specs = $dsc["spec"];

	// 如果有 OP_DATA 标记. OP_DATA 和其他的区别是 OP_DATA 是 parse_spec_rules 生成的 有键名的数组 ，其他是索引数组 
    if (isset($specs["OP_DATA"])) {
		// 取出单独处理，这个其实没用到
        $op_data_specs = $specs["OP_DATA"];
		// ** 重置这个列表，下面把它转成索引数组
        $specs["OP_DATA"] = array();
		// 遍历所有类型。这些类型在下面要作为 $mode 使用
        foreach ($op_types_ex as $op_data) {
			// 如果原列表中有这个类型
            if (isset($dsc["spec"]["OP_DATA"][$op_data])) {
				// 把这个类型添加到新列表中
                $specs["OP_DATA"][] = $op_data;
            }
        }
    }

	// 递归处理，为什么要递归这么麻烦呢？
    $f = function($specs) use (&$f) {
		// 取出第一个键名
        $spec = key($specs);
		// 弹出第一个，它其实是 parse_spec_rules 函数里面定义的每个类型对应的 数组 
        $top = array_shift($specs);
		// 如果不为空
        if ($specs) {
			// 递归操作下一个
            $next = $f($specs);
		// 如果为空，next 是两层数组
        } else {
			// 
            $next = array(array());
        }
		// 
        $ret = array();
		// 遍历 。这里是递归的业务逻辑，前面的全都卡住，从最后一个倒着来
		// $next 最一开始是空数组 [[]]
        foreach ($next as $existing) {
			// 遍历 一个 spec 的每个 $mode
            foreach ($top as $mode) {
				// 把这个 mode 加到最前面，后来居上
                $ret[] = array($spec => $mode) + $existing;
            }
        }
		//
        return $ret;
    };
	// 开始执行获取每个 spec对应每个mode 的列表 例如： [ [RETVAL=>0],[RETVAL=>1] ]
	// 返回的是个纯索引数组
	$t = $f($specs);
	// var_dump(json_encode($dsc["spec"]),json_encode($t));
    return $t;
}

// ing4, 读取文件，返回一个行内容做索引的数组 
// 木有找到 /zend_vm_order.txt 这个文件：gen_executor 调用
function read_order_file($fn) {
    $f = fopen($fn, "r");
	// 
    if (!is_resource($f)) {
        return false;
    }
    $order = [];
    while (!feof($f)) {
		// 读取1行
        $op = trim(fgets($f));
		// 只保留有效行
        if ($op !== "") {
			// 内容作键名
            $order[$op] = null;
        }
    }
	// 关闭
    fclose($f);
    return $order;
}


// ing2, 生成操作操作码处理器 和 助手（专用或非专用）。所有的处理器都是在第一次调用中生成的
// Generates all opcode handlers and helpers (specialized or unspecialized)
// 生成执行器代码：gen_executor 调用。
// 参数通常是 zend_vm_execute.h文件句柄, 1, ZEND_VM_KIND, 标签前面的内容 , 最后一个参数是返回值
function gen_executor_code($f, $spec, $kind, $prolog, &$switch_labels = array()) {
	global $list, $opcodes, $helpers, $op_types_ex, $gen_order;
//var_dump($opcodes);
	// 生成专用执行器
	// Produce specialized executor
	// 操作对象的类型
	$op1t = $op_types_ex;
	
	// 遍历所有类型，处理op1
	// for each op1.op_type
	foreach ($op1t as $op1) {
		// 操作对象的类型
		$op2t = $op_types_ex;
		
		// 遍历所有类型，处理op2
		// for each op2.op_type
		foreach ($op2t as $op2) {
			// 用原本顺序遍历 助手 和 处理器（$list 里是处理器和助手 ）
			// for each handlers in helpers in original order
			foreach ($list as $lineno => $dsc) {
				// 如果类型是处理器
				if (isset($dsc["handler"])) {
					// 操作码编号
					$num = $dsc["handler"];
					// extra_spec_handler 非常重要，它决定了一个操作码生成多少个处理函数
					// 遍历一个 【处理器】 的所有 额外类型 和 mode 的组合
					// 所以一个 【处理器】 有多少个分支是由 专用规则 和 扩展规则决定的 决定的
					foreach (extra_spec_handler($opcodes[$num]) as $extra_spec) {
						// 检查 处理器是否 允许这个类型的操作对象
						// Check if handler accepts such types of operands (op1 and op2)
						if (isset($opcodes[$num]["op1"][$op1]) &&
							isset($opcodes[$num]["op2"][$op2])) {
							// 生成处理器代码
						  // Generate handler code
							// zend_vm_execute.h文件句柄, 1, ZEND_VM_KIND, 操作码名，第一个对象的类型，第二个操作对象的类型，是否有use
							// 源码，行号，处理器，额外规则，$switch_labels 引用返回
							gen_handler($f, 1, $kind, $opcodes[$num]["op"], $op1, $op2, isset($opcodes[$num]["use"]), $opcodes[$num]["code"], $lineno, $opcodes[$num], $extra_spec, $switch_labels);
						}
					}
				// 类型是助手 
				} else if (isset($dsc["helper"])) {
					$num = $dsc["helper"];
					// 遍历 一个 【助手】 的所有 额外类型 和 mode 的组合
					// 所以一个 【助手】 有多少个分支是由 专用规则 和 扩展规则决定的 决定的
					foreach (extra_spec_handler($helpers[$num]) as $extra_spec) {
						// 检查 处理器是否 允许这个类型的操作对象
						// Check if handler accepts such types of operands (op1 and op2)
						if (isset($helpers[$num]["op1"][$op1]) &&
							isset($helpers[$num]["op2"][$op2])) {
							// 生成助手代码
						  // Generate helper code
							// zend_vm_execute.h文件句柄, 1, ZEND_VM_KIND, 操作码名，
							// 所有传递的op1和op2都是两个操作对象的类型
							gen_helper($f, 1, $kind, $num, $op1, $op2, $helpers[$num]["param"], $helpers[$num]["code"], $lineno, $helpers[$num]["inline"], $helpers[$num]["cold"], $helpers[$num]["hot"], $extra_spec);
						}
					}
				}
			}
		}
	}

    if (is_array($gen_order)) { // 如果有顺序要求（默认没有）
		// 这里用不到 ...
    }

	// 如果要求用原语句进行调试，复制原语句
    if (ZEND_VM_LINES) { // ZEND_VM_LINES 默认为0
		// 这里用不到 ...
    }

	// 为未定义操作码生成处理器
    // Generate handler for undefined opcodes
	// 根据不同模式进行处理
	switch ($kind) {
		// 其他运行模式 ...
		// CALL 模式，用到
        case ZEND_VM_KIND_CALL:
            gen_null_handler($f);
            break;
		// ing3, HYBRID模式（重点）
        case ZEND_VM_KIND_HYBRID:
			//  #define HYBRID_CASE(op)   op ## _LABEL
			// 获取 HYBRID_HALT 的 label?
            out($f,"\t\t\tHYBRID_CASE(HYBRID_HALT):\n");
			
			// 如果有 ZEND_VM_FP_GLOBAL_REG
            out($f,"#ifdef ZEND_VM_FP_GLOBAL_REG\n");
			// 从虚拟机堆栈中获取 execute_data
            out($f,"\t\t\t\texecute_data = vm_stack_data.orig_execute_data;\n");
            out($f,"#endif\n");
			
			// 如果有 ZEND_VM_IP_GLOBAL_REG
            out($f,"#ifdef ZEND_VM_IP_GLOBAL_REG\n");
			// 从虚拟机堆栈中获取 opline
            out($f,"\t\t\t\topline = vm_stack_data.orig_opline;\n");
            out($f,"#endif\n");
			
			// 返回
            out($f,"\t\t\t\treturn;\n");
			
			// 默认情况
            out($f,"\t\t\tHYBRID_DEFAULT:\n");
            out($f,"\t\t\t\tVM_TRACE(ZEND_NULL)\n");
			// 调用 ZEND_NULL_HANDLER
            out($f,"\t\t\t\tZEND_NULL_HANDLER(ZEND_OPCODE_HANDLER_ARGS_PASSTHRU);\n");
            out($f,"\t\t\t\tHYBRID_BREAK(); /* Never reached */\n");
			
            break;
    }
}

// clear, 输出：跳过空行
function skip_blanks($f, $prolog, $epilog) {
    if (trim($prolog) != "" || trim($epilog) != "") {
        out($f, $prolog.$epilog);
    }
}

// 使用 框架文件和 定义（专用|非专用） 生成操作器：gen_vm 调用
// Generates executor from skeleton file and definition (specialized or unspecialized)
// 参数：zend_vm_execute.h文件句柄，zend_vm_execute.skl文件内容，1, ZEND_VM_KIND, "execute", "zend_vm_init"
function gen_executor($f, $skl, $spec, $kind, $executor_name, $initializer_name) {
    global $params, $skeleton_file, $line_no, $gen_order;

	// 如果用 ZEND_VM_KIND_HYBRID 模式生成, 并且顺序文件存在
	// 没有发现有这个order文件
    if ($kind == ZEND_VM_KIND_HYBRID && file_exists(__DIR__ . "/zend_vm_order.txt")) {
		// 读取文件，返回一个行内容做索引的数组 
        $gen_order = read_order_file(__DIR__ . "/zend_vm_order.txt");
    } else {
        $gen_order = null;
    }

    $switch_labels = array();
    $lineno = 0;
	// 遍历 zend_vm_execute.skl文件内容
    foreach ($skl as $line) {
		// 文件中包含 %NAME% 格式的特殊标记，要用定制代码替换它
      // Skeleton file contains special markers in form %NAME% those are
      // substituted by custom code
	  // 找到这些标记，每行最多1个, "/(第一段，标记前的内容.*)[{][%](第二段，标记[A-Z_]*)[%][}](第三段，标记后的内容.*)/"
        if (preg_match("/(.*)[{][%]([A-Z_]*)[%][}](.*)/", $line, $m)) {
			// 按不同标记进行处理，一共10种标记: DEFINES，EXECUTOR_NAME，HELPER_VARS，INTERNAL_LABELS，
			// ZEND_VM_CONTINUE_LABEL，ZEND_VM_DISPATCH，INTERNAL_EXECUTOR，EXTERNAL_EXECUTOR，INITIALIZER_NAME，EXTERNAL_LABELS
            // 按标记名处理，第二段，标记名
			switch ($m[2]) {
				// 1. ing2, 定义常量和宏
                case "DEFINES":
					// 这几个用在 zend_vm_get_opcode_handler_idx 和 zend_vm_get_opcode_handler_idx
					// 专用掩码，后16位 1111111111111111，zend_vm_get_opcode_handler_idx 用到
                    out($f,"#define SPEC_START_MASK        0x0000ffff\n");
					// 扩展专用掩码，前16位 1111111111111011，zend_vm_get_opcode_handler_idx 用到
                    out($f,"#define SPEC_EXTRA_MASK        0xfffc0000\n");
					// zend_vm_get_opcode_handler_idx，zend_vm_set_opcode_handler_ex 用到
                    out($f,"#define SPEC_RULE_OP1          0x00010000\n");
					// zend_vm_get_opcode_handler_idx，zend_vm_set_opcode_handler_ex 用到
                    out($f,"#define SPEC_RULE_OP2          0x00020000\n");
					// zend_vm_get_opcode_handler_idx 用到
                    out($f,"#define SPEC_RULE_OP_DATA      0x00040000\n");
					// zend_vm_get_opcode_handler_idx，zend_vm_set_opcode_handler_ex 用到
                    out($f,"#define SPEC_RULE_RETVAL       0x00080000\n");
					// zend_vm_get_opcode_handler_idx 用到
                    out($f,"#define SPEC_RULE_QUICK_ARG    0x00100000\n");
					// zend_vm_get_opcode_handler_idx，zend_vm_set_opcode_handler_ex 用到
                    out($f,"#define SPEC_RULE_SMART_BRANCH 0x00200000\n");
					// zend_vm_set_opcode_handler，zend_vm_set_opcode_handler_ex 用到
                    out($f,"#define SPEC_RULE_COMMUTATIVE  0x00800000\n");
					// zend_vm_get_opcode_handler_idx 用到
                    out($f,"#define SPEC_RULE_ISSET        0x01000000\n");
					// zend_vm_get_opcode_handler_idx 用到
                    out($f,"#define SPEC_RULE_OBSERVER     0x02000000\n");
                    out($f,"\n");
					
					// zend_vm_init() 里面定义的 specs
                    out($f,"static const uint32_t *zend_spec_handlers;\n");
					// zend_vm_init() 里面定义的 labels
                    out($f,"static const void * const *zend_opcode_handlers;\n");
					// zend_vm_init() 里面定义的 labels 的数量 
                    out($f,"static int zend_handlers_count;\n");
					
					
					// 如果用 ZEND_VM_KIND_HYBRID 模式生成
                    if ($kind == ZEND_VM_KIND_HYBRID) {
						// 如果运行时是 ZEND_VM_KIND_HYBRID 模式
                        out($f,"#if (ZEND_VM_KIND == ZEND_VM_KIND_HYBRID)\n");
						// 也是 zend_vm_init() 里面定义的 labels
                        out($f,"static const void * const * zend_opcode_handler_funcs;\n");
						// 应该是 __compile_halt() 语句的操作码 ?
                        out($f,"static zend_op hybrid_halt_op;\n");
                        out($f,"#endif\n");
                    }
					
					
					// 如果运行时不是 ZEND_VM_KIND_HYBRID 模式 或没有 ZEND_VM_SPEC。
					// ZEND_VM_SPEC 只有一个定义，在 /Zend/zend_vm_opcodes.h 
                    out($f,"#if (ZEND_VM_KIND != ZEND_VM_KIND_HYBRID) || !ZEND_VM_SPEC\n");
					// zend_vm_get_opcode_handler 函数定义
                    out($f,"static const void *zend_vm_get_opcode_handler(zend_uchar opcode, const zend_op* op);\n");
                    out($f,"#endif\n\n");
					
					
					// 如果用 ZEND_VM_KIND_HYBRID 模式生成
                    if ($kind == ZEND_VM_KIND_HYBRID) {
						// 如果运行时是 ZEND_VM_KIND_HYBRID 模式
                        out($f,"#if (ZEND_VM_KIND == ZEND_VM_KIND_HYBRID)\n");
						// 定义函数 zend_vm_get_opcode_handler_func
                        out($f,"static const void *zend_vm_get_opcode_handler_func(zend_uchar opcode, const zend_op* op);\n");
                        // 运行时不是 ZEND_VM_KIND_HYBRID 模式
						out($f,"#else\n");
						// 定义宏 zend_vm_get_opcode_handler_func
                        out($f,"# define zend_vm_get_opcode_handler_func zend_vm_get_opcode_handler\n");
                        out($f,"#endif\n\n");
                    }
					
					
					// 如果没有定义 VM_TRACE 
                    out($f,"#ifndef VM_TRACE\n");
					// 如果有固定顺序 调用 ZEND_VM_GUARD
                    if (is_array($gen_order)) {
                        out($f,"# define VM_TRACE(op) ZEND_VM_GUARD(op);\n");
					// 没有固定顺序 VM_TRACE 无业务逻辑
                    } else {
                        out($f,"# define VM_TRACE(op)\n");
                    }
                    out($f,"#endif\n");
					
					// 定义 VM_TRACE_START，VM_TRACE_END 两个空的宏
					// 有可能在 /Zend/zend_vm_trace_lines.h 定义过
                    out($f,"#ifndef VM_TRACE_START\n");
                    out($f,"# define VM_TRACE_START()\n");
                    out($f,"#endif\n");
                    out($f,"#ifndef VM_TRACE_END\n");
                    out($f,"# define VM_TRACE_END()\n");
                    out($f,"#endif\n");
					
					// 按生成模式区分
					switch ($kind) {
						// HYBRID 模式
                        case ZEND_VM_KIND_HYBRID:
							// 如果运行时是 ZEND_VM_KIND_HYBRID 模式
                            out($f,"#if (ZEND_VM_KIND == ZEND_VM_KIND_HYBRID)\n");
							// 这几个是 HYBRID 模式用于跳转的宏，goto 语句如何运行的？
                            out($f,"#define HYBRID_NEXT()     goto *(void**)(OPLINE->handler)\n");
							// HYBRID_NEXT 的别名
                            out($f,"#define HYBRID_SWITCH()   HYBRID_NEXT();\n");
							// ？获取操作码对应的函数
                            out($f,"#define HYBRID_CASE(op)   op ## _LABEL\n");
							// HYBRID_NEXT 的别名
                            out($f,"#define HYBRID_BREAK()    HYBRID_NEXT()\n");
							// 默认标签
                            out($f,"#define HYBRID_DEFAULT    ZEND_NULL_LABEL\n");
                            out($f,"#endif\n");
							
						// CALL 模式，用到
                        case ZEND_VM_KIND_CALL:
                            out($f,"\n");
							// 如果有 ZEND_VM_FP_GLOBAL_REG，创建一组空的宏
                            out($f,"#ifdef ZEND_VM_FP_GLOBAL_REG\n");
                            out($f,"# define ZEND_OPCODE_HANDLER_ARGS void\n");
                            out($f,"# define ZEND_OPCODE_HANDLER_ARGS_PASSTHRU\n");
                            out($f,"# define ZEND_OPCODE_HANDLER_ARGS_DC\n");
                            out($f,"# define ZEND_OPCODE_HANDLER_ARGS_PASSTHRU_CC\n");
							// 如果没有 ZEND_VM_FP_GLOBAL_REG，
                            out($f,"#else\n");
                            out($f,"# define ZEND_OPCODE_HANDLER_ARGS zend_execute_data *execute_data\n");
                            out($f,"# define ZEND_OPCODE_HANDLER_ARGS_PASSTHRU execute_data\n");
                            out($f,"# define ZEND_OPCODE_HANDLER_ARGS_DC , ZEND_OPCODE_HANDLER_ARGS\n");
                            out($f,"# define ZEND_OPCODE_HANDLER_ARGS_PASSTHRU_CC , ZEND_OPCODE_HANDLER_ARGS_PASSTHRU\n");
                            out($f,"#endif\n");
                            out($f,"\n");
							
							// 如果有 ZEND_VM_FP_GLOBAL_REG 和 ZEND_VM_IP_GLOBAL_REG
                            out($f,"#if defined(ZEND_VM_FP_GLOBAL_REG) && defined(ZEND_VM_IP_GLOBAL_REG)\n");
                            out($f,"# define ZEND_OPCODE_HANDLER_RET void\n");
                            out($f,"# define ZEND_VM_TAIL_CALL(call) call; return\n");
							// 如果有 ZEND_VM_TAIL_CALL_DISPATCH
                            out($f,"# ifdef ZEND_VM_TAIL_CALL_DISPATCH\n");
                            out($f,"#  define ZEND_VM_CONTINUE()     ((opcode_handler_t)OPLINE->handler)(ZEND_OPCODE_HANDLER_ARGS_PASSTHRU); return\n");
                            out($f,"# else\n");
                            out($f,"#  define ZEND_VM_CONTINUE()     return\n");
                            out($f,"# endif\n");
							// 如果用 ZEND_VM_KIND_HYBRID 模式生成
                            if ($kind == ZEND_VM_KIND_HYBRID) {
								// 如果运行时是 ZEND_VM_KIND_HYBRID 模式
                                out($f,"# if (ZEND_VM_KIND == ZEND_VM_KIND_HYBRID)\n");
                                out($f,"#  define ZEND_VM_RETURN()        opline = &hybrid_halt_op; return\n");
                                out($f,"#  define ZEND_VM_HOT             zend_always_inline ZEND_COLD ZEND_OPT_SIZE\n");
                                out($f,"#  define ZEND_VM_COLD            ZEND_COLD ZEND_OPT_SIZE\n");
                                out($f,"# else\n");
                                out($f,"#  define ZEND_VM_RETURN()        opline = NULL; return\n");
                                out($f,"#  define ZEND_VM_HOT\n");
                                out($f,"#  define ZEND_VM_COLD            ZEND_COLD ZEND_OPT_SIZE\n");
                                out($f,"# endif\n");
							// 如果不是 HYBRID 模式
                            } else {
                                // 其他运行模式 ...
                            }
							// 
                            out($f,"#else\n");
                            out($f,"# define ZEND_OPCODE_HANDLER_RET int\n");
                            out($f,"# define ZEND_VM_TAIL_CALL(call) return call\n");
                            out($f,"# define ZEND_VM_CONTINUE()      return  0\n");
                            out($f,"# define ZEND_VM_RETURN()        return -1\n");
							// 如果用 ZEND_VM_KIND_HYBRID 模式生成
                            if ($kind == ZEND_VM_KIND_HYBRID) {
                                out($f,"# define ZEND_VM_HOT\n");
                            }
                            out($f,"# define ZEND_VM_COLD            ZEND_COLD ZEND_OPT_SIZE\n");
                            out($f,"#endif\n");
                            out($f,"\n");
                            out($f,"typedef ZEND_OPCODE_HANDLER_RET (ZEND_FASTCALL *opcode_handler_t) (ZEND_OPCODE_HANDLER_ARGS);\n");
                            out($f,"\n");
                            out($f,"#define DCL_OPLINE\n");
                            out($f,"#ifdef ZEND_VM_IP_GLOBAL_REG\n");
                            out($f,"# define OPLINE opline\n");
                            out($f,"# define USE_OPLINE\n");
                            out($f,"# define LOAD_OPLINE() opline = EX(opline)\n");
                            out($f,"# define LOAD_OPLINE_EX()\n");
                            out($f,"# define LOAD_NEXT_OPLINE() opline = EX(opline) + 1\n");
                            out($f,"# define SAVE_OPLINE() EX(opline) = opline\n");
                            out($f,"# define SAVE_OPLINE_EX() SAVE_OPLINE()\n");
                            out($f,"#else\n");
                            out($f,"# define OPLINE EX(opline)\n");
                            out($f,"# define USE_OPLINE const zend_op *opline = EX(opline);\n");
                            out($f,"# define LOAD_OPLINE()\n");
                            out($f,"# define LOAD_OPLINE_EX()\n");
                            out($f,"# define LOAD_NEXT_OPLINE() ZEND_VM_INC_OPCODE()\n");
                            out($f,"# define SAVE_OPLINE()\n");
                            out($f,"# define SAVE_OPLINE_EX()\n");
                            out($f,"#endif\n");
                            out($f,"#define HANDLE_EXCEPTION() ZEND_ASSERT(EG(exception)); LOAD_OPLINE(); ZEND_VM_CONTINUE()\n");
                            out($f,"#define HANDLE_EXCEPTION_LEAVE() ZEND_ASSERT(EG(exception)); LOAD_OPLINE(); ZEND_VM_LEAVE()\n");
                            out($f,"#if defined(ZEND_VM_FP_GLOBAL_REG)\n");
                            out($f,"# define ZEND_VM_ENTER_EX()        ZEND_VM_INTERRUPT_CHECK(); ZEND_VM_CONTINUE()\n");
                            out($f,"# define ZEND_VM_ENTER()           execute_data = EG(current_execute_data); LOAD_OPLINE(); ZEND_VM_ENTER_EX()\n");
                            out($f,"# define ZEND_VM_LEAVE()           ZEND_VM_CONTINUE()\n");
                            out($f,"#elif defined(ZEND_VM_IP_GLOBAL_REG)\n");
                            out($f,"# define ZEND_VM_ENTER_EX()        return  1\n");
                            out($f,"# define ZEND_VM_ENTER()           opline = EG(current_execute_data)->opline; ZEND_VM_ENTER_EX()\n");
                            out($f,"# define ZEND_VM_LEAVE()           return  2\n");
                            out($f,"#else\n");
                            out($f,"# define ZEND_VM_ENTER_EX()        return  1\n");
                            out($f,"# define ZEND_VM_ENTER()           return  1\n");
                            out($f,"# define ZEND_VM_LEAVE()           return  2\n");
                            out($f,"#endif\n");
                            out($f,"#define ZEND_VM_INTERRUPT()      ZEND_VM_TAIL_CALL(zend_interrupt_helper".($spec?"_SPEC":"")."(ZEND_OPCODE_HANDLER_ARGS_PASSTHRU));\n");
                            out($f,"#define ZEND_VM_LOOP_INTERRUPT() zend_interrupt_helper".($spec?"_SPEC":"")."(ZEND_OPCODE_HANDLER_ARGS_PASSTHRU);\n");
							// 如果用 ZEND_VM_KIND_HYBRID 模式生成
                            if ($kind == ZEND_VM_KIND_HYBRID) {
                                out($f,"#define ZEND_VM_DISPATCH(opcode, opline) ZEND_VM_TAIL_CALL(((opcode_handler_t)zend_vm_get_opcode_handler_func(opcode, opline))(ZEND_OPCODE_HANDLER_ARGS_PASSTHRU));\n");
							// 如果用其他模式
                            } else {
                                // 其他运行模式 ...
                            }
                            out($f,"\n");
                            out($f,"static ZEND_OPCODE_HANDLER_RET ZEND_FASTCALL zend_interrupt_helper".($spec?"_SPEC":"")."(ZEND_OPCODE_HANDLER_ARGS);\n");
                            out($f,"static ZEND_OPCODE_HANDLER_RET ZEND_FASTCALL ZEND_NULL_HANDLER(ZEND_OPCODE_HANDLER_ARGS);\n");
                            out($f,"\n");
                            break;
							
						// 其他运行模式 ...
                    }
					// 如果用 HYBRID 模式生成
                    if ($kind == ZEND_VM_KIND_HYBRID) {
						// 生成执行器代码，这是最重要的部分 zend_vm_execute.h文件句柄, 1, ZEND_VM_KIND, 标签 前面的内容
                        gen_executor_code($f, $spec, ZEND_VM_KIND_CALL, $m[1]);
                        out($f,"\n");
						// 如果运行时是 ZEND_VM_KIND_HYBRID 模式
                        out($f,"#if (ZEND_VM_KIND == ZEND_VM_KIND_HYBRID)\n");
						// 删除这几个宏，重新定义
                        out($f,"# undef ZEND_VM_TAIL_CALL\n");
                        out($f,"# undef ZEND_VM_CONTINUE\n");
                        out($f,"# undef ZEND_VM_RETURN\n");
//						out($f,"# undef ZEND_VM_INTERRUPT\n");
                        out($f,"\n");
						// 尾调用。调用语句后，调用 HYBRID_NEXT
                        out($f,"# define ZEND_VM_TAIL_CALL(call) call; ZEND_VM_CONTINUE()\n");
						// HYBRID_NEXT 的别名
                        out($f,"# define ZEND_VM_CONTINUE()      HYBRID_NEXT()\n");
						// ？HYBRID_HALT_LABEL 只有引用，没有定义
                        out($f,"# define ZEND_VM_RETURN()        goto HYBRID_HALT_LABEL\n");
//						out($f,"# define ZEND_VM_INTERRUPT()     goto zend_interrupt_helper_SPEC_LABEL\n");
                        out($f,"#endif\n\n");
                    }
                    break;
					
				// 2. clear, 把标签换成 执行器名称 前面后面不变
                case "EXECUTOR_NAME":
                    out($f, $m[1].$executor_name.$m[3]."\n");
                    break;
					
				// 3.
                case "HELPER_VARS":
					// 如果用 ZEND_VM_KIND_SWITCH 模式生成
                    if ($kind == ZEND_VM_KIND_SWITCH) {
						// ...
                    }
					// var_dump($params);
					// 如果不是用 ZEND_VM_KIND_CALL 模式生成
                    if ($kind != ZEND_VM_KIND_CALL && count($params)) {
						// 如果用 ZEND_VM_KIND_HYBRID 模式生成
                        if ($kind == ZEND_VM_KIND_HYBRID) {
							// 如果运行时是 ZEND_VM_KIND_HYBRID 模式
                            out($f, "#if (ZEND_VM_KIND == ZEND_VM_KIND_HYBRID)\n");
                        }
                        // Emit local variables those are used for helpers' parameters
                        foreach ($params as $param => $x) {
                            out($f,$m[1].$param.";\n");
                        }
						// 如果用 ZEND_VM_KIND_HYBRID 模式生成
                        if ($kind == ZEND_VM_KIND_HYBRID) {
                            out($f, "#endif\n");
                        }
                    }
					// 如果不是用 ZEND_VM_KIND_CALL 也不是用 ZEND_VM_KIND_HYBRID 模式生成
                    if ($kind != ZEND_VM_KIND_CALL && $kind != ZEND_VM_KIND_HYBRID) {
                        // 其他模式 ...
					// 如果是用 ZEND_VM_KIND_CALL 或 ZEND_VM_KIND_HYBRID 模式生成
                    } else {
                        out($f,"#if defined(ZEND_VM_IP_GLOBAL_REG) || defined(ZEND_VM_FP_GLOBAL_REG)\n");
                        out($f,$m[1]."struct {\n");
                        out($f,"#ifdef ZEND_VM_IP_GLOBAL_REG\n");
                        out($f,$m[1]."\tconst zend_op *orig_opline;\n");
                        out($f,"#endif\n");
                        out($f,"#ifdef ZEND_VM_FP_GLOBAL_REG\n");
                        out($f,$m[1]."\tzend_execute_data *orig_execute_data;\n");
                        out($f,"#ifdef ZEND_VM_HYBRID_JIT_RED_ZONE_SIZE\n");
                        out($f,$m[1]."\tchar hybrid_jit_red_zone[ZEND_VM_HYBRID_JIT_RED_ZONE_SIZE];\n");
                        out($f,"#endif\n");
                        out($f,"#endif\n");
                        out($f,$m[1]."} vm_stack_data;\n");
                        out($f,"#endif\n");
                        out($f,"#ifdef ZEND_VM_IP_GLOBAL_REG\n");
                        out($f,$m[1]."vm_stack_data.orig_opline = opline;\n");
                        out($f,"#endif\n");
                        out($f,"#ifdef ZEND_VM_FP_GLOBAL_REG\n");
                        out($f,$m[1]."vm_stack_data.orig_execute_data = execute_data;\n");
                        out($f,$m[1]."execute_data = ex;\n");
                        out($f,"#else\n");
                        out($f,$m[1]."zend_execute_data *execute_data = ex;\n");
                        out($f,"#endif\n");
                    }
                    break;
					
				// 4.
                case "INTERNAL_LABELS":
					// 如果用 ZEND_VM_KIND_GOTO 或 ZEND_VM_KIND_HYBRID 模式生成
                    if ($kind == ZEND_VM_KIND_GOTO || $kind == ZEND_VM_KIND_HYBRID) {
                      // Emit array of labels of opcode handlers and code for
                      // zend_opcode_handlers initialization
						// 如果用 ZEND_VM_KIND_HYBRID 模式生成
                        if ($kind == ZEND_VM_KIND_HYBRID) {
							// 如果运行时是 ZEND_VM_KIND_HYBRID 模式
                            out($f,"#if (ZEND_VM_KIND == ZEND_VM_KIND_HYBRID)\n");
                        }
                        $prolog = $m[1];
                        out($f,$prolog."if (UNEXPECTED(execute_data == NULL)) {\n");
                        out($f,$prolog."\tstatic const void * const labels[] = {\n");
						// 如果用 ZEND_VM_KIND_HYBRID 模式生成，$specs 是在这里返回的
                        gen_labels($f, $spec, ($kind == ZEND_VM_KIND_HYBRID) ? ZEND_VM_KIND_GOTO : $kind, $prolog."\t\t", $specs);
						var_dump($specs);
                        out($f,$prolog."\t};\n");
                        out($f,$prolog."\tzend_opcode_handlers = (const void **) labels;\n");
                        out($f,$prolog."\tzend_handlers_count = sizeof(labels) / sizeof(void*);\n");
						// 如果用 ZEND_VM_KIND_HYBRID 模式生成
                        if ($kind == ZEND_VM_KIND_HYBRID) {
                            out($f,$prolog."\tmemset(&hybrid_halt_op, 0, sizeof(hybrid_halt_op));\n");
                            out($f,$prolog."\thybrid_halt_op.handler = (void*)&&HYBRID_HALT_LABEL;\n");
	                        out($f,"#ifdef ZEND_VM_HYBRID_JIT_RED_ZONE_SIZE\n");
	                        out($f,$prolog."\tmemset(vm_stack_data.hybrid_jit_red_zone, 0, ZEND_VM_HYBRID_JIT_RED_ZONE_SIZE);\n");
	                        out($f,"#endif\n");
	                        out($f,$prolog."\tif (zend_touch_vm_stack_data) {\n");
	                        out($f,$prolog."\t\tzend_touch_vm_stack_data(&vm_stack_data);\n");
	                        out($f,$prolog."\t}\n");
                            out($f,$prolog."\tgoto HYBRID_HALT_LABEL;\n");
                        } else {
                            out($f,$prolog."\treturn;\n");
                        }
                        out($f,$prolog."}\n");
						// 如果用 ZEND_VM_KIND_HYBRID 模式生成
                        if ($kind == ZEND_VM_KIND_HYBRID) {
                            out($f,"#endif\n");
                        }
                    } else {
                        skip_blanks($f, $m[1], $m[3]);
                    }
                    break;
					
				// 5. 
                case "ZEND_VM_CONTINUE_LABEL":
					// 如果用 ZEND_VM_KIND_CALL 或 ZEND_VM_KIND_HYBRID 模式生成
                    if ($kind == ZEND_VM_KIND_CALL || $kind == ZEND_VM_KIND_HYBRID) {
                      // Only SWITCH dispatch method use it
                        out($f,"#if !defined(ZEND_VM_FP_GLOBAL_REG) || !defined(ZEND_VM_IP_GLOBAL_REG)\n");
                        out($f,$m[1]."\tint ret;".$m[3]."\n");
                        out($f,"#endif\n");
                    } 
					// 其他运行模式 ...
                    break;
					
				// 6. 分发调用
                case "ZEND_VM_DISPATCH":
					// 调用 opcode 对应的 处理器（重点）
                  // Emit code that dispatches to opcode handler
                    switch ($kind) {
                        // 其他运行模式 ...
						// 如果用 HYBRID 模式生成
                        case ZEND_VM_KIND_HYBRID:
							// 如果运行时是 ZEND_VM_KIND_HYBRID 模式
                            out($f,"#if (ZEND_VM_KIND == ZEND_VM_KIND_HYBRID)\n");
							// 把标签替换成 HYBRID_SWITCH()
                            out($f, $m[1]."HYBRID_SWITCH()".$m[3]."\n");
                            out($f,"#else\n");
                        case ZEND_VM_KIND_CALL:
                            out($f,"#if defined(ZEND_VM_FP_GLOBAL_REG) && defined(ZEND_VM_IP_GLOBAL_REG)\n");
                            out($f, $m[1]."((opcode_handler_t)OPLINE->handler)(ZEND_OPCODE_HANDLER_ARGS_PASSTHRU);\n");
                            out($f, $m[1]."if (UNEXPECTED(!OPLINE))".$m[3]."\n");
                            out($f,"#else\n");
                            out($f, $m[1]."if (UNEXPECTED((ret = ((opcode_handler_t)OPLINE->handler)(ZEND_OPCODE_HANDLER_ARGS_PASSTHRU)) != 0))".$m[3]."\n");
                            out($f,"#endif\n");
							// 如果用 ZEND_VM_KIND_CALL 或 ZEND_VM_KIND_HYBRID 模式生成
                            if ($kind == ZEND_VM_KIND_HYBRID) {
                                out($f,"#endif\n");
                            }
                            break;
                    }
                    break;
					
				// 7. ing3, 内部执行器
                case "INTERNAL_EXECUTOR":
					// 如果不是用 ZEND_VM_KIND_CALL 模式生成
                    if ($kind != ZEND_VM_KIND_CALL) {
                        // Emit executor code
						// 如果用 ZEND_VM_KIND_CALL 或 ZEND_VM_KIND_HYBRID 模式生成
                        if ($kind == ZEND_VM_KIND_HYBRID) {
							// 如果运行时是 ZEND_VM_KIND_HYBRID 模式
                            out($f,"#if (ZEND_VM_KIND == ZEND_VM_KIND_HYBRID)\n");
                        }
						// zend_vm_execute.h文件句柄, 1, ZEND_VM_KIND, 标签前面的内容
						// $switch_labels 是引用返回
                        gen_executor_code($f, $spec, $kind, $m[1], $switch_labels);
                    }
					// 如果用 ZEND_VM_KIND_CALL 或 ZEND_VM_KIND_HYBRID 模式生成
                    if ($kind == ZEND_VM_KIND_CALL || $kind == ZEND_VM_KIND_HYBRID) {
                        // Executor is defined as a set of functions
						// 如果用 ZEND_VM_KIND_HYBRID 模式生成
                        if ($kind == ZEND_VM_KIND_HYBRID) {
                            out($f,"#else\n");
                        }
						// 
                        out($f,
								// 如果有 ZEND_VM_FP_GLOBAL_REG
                                "#ifdef ZEND_VM_FP_GLOBAL_REG\n" .
								// 从虚拟机堆栈里获取 execute_data
                                $m[1]."execute_data = vm_stack_data.orig_execute_data;\n" .
								// 如果有 ZEND_VM_IP_GLOBAL_REG
                                "# ifdef ZEND_VM_IP_GLOBAL_REG\n" .
								// 从虚拟机堆栈里获取 opline
                                $m[1]."opline = vm_stack_data.orig_opline;\n" .
                                "# endif\n" .
                                $m[1]."return;\n" .
								
								// 如果没有 ZEND_VM_FP_GLOBAL_REG
                                "#else\n" .
								// 如果 ret>0
                                $m[1]."if (EXPECTED(ret > 0)) {\n" .
								// 更新 execute_data, EG ：Executor global
                                $m[1]."\texecute_data = EG(current_execute_data);\n".
								// 调用 ZEND_VM_LOOP_INTERRUPT_CHECK();
                                $m[1]."\tZEND_VM_LOOP_INTERRUPT_CHECK();\n".
								// 如果 ret <= 0
                                $m[1]."} else {\n" .
								// 如果有 ZEND_VM_IP_GLOBAL_REG
                                "# ifdef ZEND_VM_IP_GLOBAL_REG\n" .
								// 从虚拟机堆栈里获取 opline
                                $m[1]."\topline = vm_stack_data.orig_opline;\n" .
                                "# endif\n".
                                $m[1]."\treturn;\n".
                                $m[1]."}\n".
                                "#endif\n");
						// 如果用 ZEND_VM_KIND_HYBRID 模式生成
                        if ($kind == ZEND_VM_KIND_HYBRID) {
                            out($f,"#endif\n");
                        }
                    }
                    break;
					
				// 8. ing4, 额外的执行器
                case "EXTERNAL_EXECUTOR":
					// 其他运行模式 ...
                    break;
					
				// 9. clear, 把标签替换成 初始化器 名称，上下文不变
                case "INITIALIZER_NAME":
					// $initializer_name 是传入的变量
                    out($f, $m[1].$initializer_name.$m[3]."\n");
                    break;
					
				// 10. 额外的标签
                case "EXTERNAL_LABELS":
					// 生成初始化 zend_opcode_handlers 数组的代码
                  // Emit code that initializes zend_opcode_handlers array
					// 标签前面的部分
                    $prolog = $m[1];
					// 如果运行时是 ZEND_VM_KIND_GOTO 模式
                    if ($kind == ZEND_VM_KIND_GOTO) {
						// 其他运行模式 ...
					// 如果运行时不是 ZEND_VM_KIND_GOTO 模式
                    } else {
						// 定义 labels
                        out($f,$prolog."static const void * const labels[] = {\n");
						// 如果用 ZEND_VM_KIND_HYBRID 模式生成 ZEND_VM_KIND_CALL, 否则 原 $kind
						// $specs 是传入的, 这里生成430行代码
                        gen_labels($f, $spec, ($kind == ZEND_VM_KIND_HYBRID) ? ZEND_VM_KIND_CALL : $kind, $prolog."\t", $specs, $switch_labels);
						// var_dump($specs);
						//var_dump(json_encode($specs));
						// 
                        out($f,$prolog."};\n");
						// 定义 specs
                        out($f,$prolog."static const uint32_t specs[] = {\n");
						// 生成 gen_specs
                        gen_specs($f, $prolog."\t", $specs);
						// 
                        out($f,$prolog."};\n");
						// 如果用 ZEND_VM_KIND_HYBRID 模式生成
                        if ($kind == ZEND_VM_KIND_HYBRID) {
							// 如果运行时是 ZEND_VM_KIND_HYBRID 模式
                            out($f,"#if (ZEND_VM_KIND == ZEND_VM_KIND_HYBRID)\n");
							// labels 对应的函数 
                            out($f,$prolog."zend_opcode_handler_funcs = labels;\n");
							// 规则处理器
                            out($f,$prolog."zend_spec_handlers = specs;\n");
							// 调用执行器 参数为 空
                            out($f,$prolog.$executor_name."_ex(NULL);\n");
                            out($f,"#else\n");
                        }
						// 如果不用 ZEND_VM_KIND_HYBRID 模式运行
						// 操作码处理器
                        out($f,$prolog."zend_opcode_handlers = labels;\n");
						// 操作码处理器数量 
                        out($f,$prolog."zend_handlers_count = sizeof(labels) / sizeof(void*);\n");
						// 规则处理器
                        out($f,$prolog."zend_spec_handlers = specs;\n");
						// 如果用 ZEND_VM_KIND_HYBRID 模式生成
                        if ($kind == ZEND_VM_KIND_HYBRID) {
                            out($f,"#endif\n");
                        }
                    }
                    break;
            }
		// 如果没有特殊标记，直接复制这一行
        } else {
          // Copy the line as is
            out($f, $line);
        }
    }
}

// ing4, 解析操作对象类型 如：CONST|TMPVAR|CV，(文件路径，行号，类型，附加标记)
// 返回结果中，多个类型放在数组键名里，值是索引号
// ：parse_spec_rules，gen_vm 调用
function parse_operand_spec($def, $lineno, $str, &$flags) {
    global $vm_op_decode;

    $flags = 0;
	// 把类型拆分开
    $a = explode("|",$str);
	// 遍历每一个类型
    foreach ($a as $val) {
		// 如果已经在 vm_op_decode 中，是有效类型
        if (isset($vm_op_decode[$val])) {
			// 添加此类型的标记
            $flags |= $vm_op_decode[$val];
		// 否则
        } 
    }
	// 如果没有 ZEND_VM_OP_SPEC 标记。类型是 ANY
    if (!($flags & ZEND_VM_OP_SPEC)) {
		// 如果不只有一个类型
        if (count($a) != 1) {
			// 报错：$str 是错误类型
            die("ERROR ($def:$lineno): Wrong operand type '$str'\n");
        }
		// 如果是只有一个类型，类型是 ANY
        $a = array("ANY");
    }
	// 交换键和值
    return array_flip($a);
}

// ing4, 解析专用扩展,如: CACHE_SLOT 。 gen_vm 调用
function parse_ext_spec($def, $lineno, $str) {
    global $vm_ext_decode;

    $flags = 0;
	// 用 “|” 分隔开
    $a = explode("|",$str);
	// 遍历每一个
    foreach ($a as $val) {
		// 如果类型有效，添加标记
        if (isset($vm_ext_decode[$val])) {
            $flags |= $vm_ext_decode[$val];
        } 
    }
    return $flags;
}

// ing4, 解析专用规则, 如 SPEC(OP_DATA=CONST|TMP|VAR|CV) 或 SPEC(RETVAL,OBSERVER), (文件路径，行号，类型)
// 只有 OP_DATA= 用到等号，其他情况都是逗号分隔。
// 返回结果中 OP_DATA 和其他的区别是 OP_DATA 是 parse_operand_spec 生成的 有键名的数组 ，其他是索引数组 
// gen_vm 调用
// 参数：zend_vm_def.h，行号，规则
function parse_spec_rules($def, $lineno, $str) {
    global $used_extra_spec;

    $ret = array();
	// 用逗号拆分
    $a = explode(",", $str);
	// 遍历每一个规则 
    foreach ($a as $rule) {
		// 是否有=
		$n = strpos($rule, "=");
		// 如果有 “= ”
        if ($n !== false) {
			// = 前面是ID
            $id = trim(substr($rule, 0, $n));
			// = 后面是 value
            $val = trim(substr($rule, $n+1));
			// $id 
            switch ($id) {
				// 只能是 OP_DATA，
				// 返回结果中 OP_DATA 和其他的区别是 OP_DATA 是 parse_operand_spec 生成的 有键名的数组 ，其他是索引数组 
                case "OP_DATA":
					// 解析操作对象的特殊属性, $devnull 是返回的 flag, 这里没有用到
                    $ret["OP_DATA"] = parse_operand_spec($def, $lineno, $val, $devnull);
                    break;
				// 不支持其他ID
                default:
                    die("ERROR ($def:$lineno): Wrong specialization rules '$str'\n");
            }
			// 把ID添加到额外标记列表里
            $used_extra_spec[$id] = 1;
		// 如果没有 = 
        } else {
			// 支持的类型 7个
            switch ($rule) {
				//
                case "RETVAL":
                    $ret["RETVAL"] = array(0, 1);
                    break;
				// 
                case "QUICK_ARG":
                    $ret["QUICK_ARG"] = array(0, 1);
                    break;
				// 
                case "SMART_BRANCH":
                    $ret["SMART_BRANCH"] = array(0, 1, 2);
                    break;
				// 
                case "NO_CONST_CONST":
                    $ret["NO_CONST_CONST"] = array(1);
                    break;
				// 
                case "COMMUTATIVE":
                    $ret["COMMUTATIVE"] = array(1);
                    break;
				// 
                case "ISSET":
                    $ret["ISSET"] = array(0, 1);
                    break;
				// 
                case "OBSERVER":
                    $ret["OBSERVER"] = array(0, 1);
                    break;
				// 这7个以外，报错
                default:
                    die("ERROR ($def:$lineno): Wrong specialization rules '$str'\n");
            }
			// 添加额外 扩展规则标记
            $used_extra_spec[$rule] = 1;
        }
    }
	// 返回规则列表
    return $ret;
}

// zend_vm_opcodes.h 文件头部内容， gen_vm 调用
function gen_vm_opcodes_header(
    array $opcodes, int $max_opcode, int $max_opcode_len, array $vm_op_flags
): string {
	// var_dump($GLOBALS["vm_kind_name"][ZEND_VM_KIND]);
	// 版权信息
    $str = HEADER_TEXT;
	// 跳转用的宏
    $str .= "#ifndef ZEND_VM_OPCODES_H\n#define ZEND_VM_OPCODES_H\n\n";
    $str .= "#define ZEND_VM_SPEC\t\t" . ZEND_VM_SPEC . "\n";
    $str .= "#define ZEND_VM_LINES\t\t" . ZEND_VM_LINES . "\n";
    $str .= "#define ZEND_VM_KIND_CALL\t" . ZEND_VM_KIND_CALL . "\n";
    $str .= "#define ZEND_VM_KIND_SWITCH\t" . ZEND_VM_KIND_SWITCH . "\n";
    $str .= "#define ZEND_VM_KIND_GOTO\t" . ZEND_VM_KIND_GOTO . "\n";
    $str .= "#define ZEND_VM_KIND_HYBRID\t" . ZEND_VM_KIND_HYBRID . "\n";
	// ZEND_VM_KIND_HYBRID 模式
    if ($GLOBALS["vm_kind_name"][ZEND_VM_KIND] === "ZEND_VM_KIND_HYBRID") {
		// 
        $str .= "/* HYBRID requires support for computed GOTO and global register variables*/\n";
        $str .= "#if (defined(__GNUC__) && defined(HAVE_GCC_GLOBAL_REGS))\n";
        $str .= "# define ZEND_VM_KIND\t\tZEND_VM_KIND_HYBRID\n";
        $str .= "#else\n";
        $str .= "# define ZEND_VM_KIND\t\tZEND_VM_KIND_CALL\n";
        $str .= "#endif\n";
	// 其他运行模式 ...
    } else {        
    }
    $str .= "\n";
	// 如果运行时是 ZEND_VM_KIND_HYBRID 模式
    $str .= "#if (ZEND_VM_KIND == ZEND_VM_KIND_HYBRID) && !defined(__SANITIZE_ADDRESS__)\n";
    $str .= "# if ((defined(i386) && !defined(__PIC__)) || defined(__x86_64__) || defined(_M_X64))\n";
    $str .= "#  define ZEND_VM_HYBRID_JIT_RED_ZONE_SIZE 16\n";
    $str .= "# endif\n";
    $str .= "#endif\n";
    $str .= "\n";
	// 
    foreach ($vm_op_flags as $name => $val) {
        $str .= sprintf("#define %-24s 0x%08x\n", $name, $val);
    }
    $str .= "#define ZEND_VM_OP1_FLAGS(flags) (flags & 0xff)\n";
    $str .= "#define ZEND_VM_OP2_FLAGS(flags) ((flags >> 8) & 0xff)\n";
    $str .= "\n";
    $str .= "BEGIN_EXTERN_C()\n\n";
    $str .= "ZEND_API const char* ZEND_FASTCALL zend_get_opcode_name(zend_uchar opcode);\n";
    $str .= "ZEND_API uint32_t ZEND_FASTCALL zend_get_opcode_flags(zend_uchar opcode);\n";
    $str .= "ZEND_API zend_uchar zend_get_opcode_id(const char *name, size_t length);\n\n";
    $str .= "END_EXTERN_C()\n\n";

    $code_len = strlen((string) $max_opcode);
	// 遍历 操作码编号索引的 处理器列表
    foreach ($opcodes as $code => $dsc) {
        $code = str_pad((string)$code, $code_len, " ", STR_PAD_LEFT);
        $op = str_pad($dsc["op"], $max_opcode_len);
        if ($code <= $max_opcode) {
            $str .= "#define $op $code\n";
        }
    }
	// 
    $code = str_pad((string)$max_opcode, $code_len, " ", STR_PAD_LEFT);
    $op = str_pad("ZEND_VM_LAST_OPCODE", $max_opcode_len);
    $str .= "\n#define $op $code\n";

    $str .= "\n#endif\n";
    return $str;
}

// 主方法
function gen_vm($def, $skel) {
	//
    global $definition_file, $skeleton_file, $executor_file,
        $op_types, $list, $opcodes, $helpers, $params, $opnames,
        $vm_op_flags, $used_extra_spec;

	// 加载定义文件：zend_vm_def.h
    // Load definition file
    $in = @file($def);
	
	// 在 #line 指定中需要用到绝对路径
    // We need absolute path to definition file to use it in #line directives
    $definition_file = realpath($def);

	// 加载骨架 
    // Load skeleton file
    $skl = @file($skel);
	
	// 需要框架文件的绝对路径，用在 #line 指令里
    $skeleton_file = realpath($skel);

	// 把定义文件解析成树结构 
    // Parse definition file into tree
    $lineno         = 0;
    $handler        = null;
    $helper         = null;
    $max_opcode_len = 0;
    $max_opcode     = 0;
	// 额外数量 ： 256
    $extra_num      = 256;
	// 1. 数据解析
	// 遍历定义 zend_vm_def.h 的每一行
    foreach ($in as $line) {
        ++$lineno;
		// 1.1 处理以下几种情况：虚拟机处理器：107个
        if (strpos($line,"ZEND_VM_HANDLER(") === 0 ||
			// 虚拟机行内处理器：1个
            strpos($line,"ZEND_VM_INLINE_HANDLER(") === 0 ||
			// 虚拟机热处理器：28个
            strpos($line,"ZEND_VM_HOT_HANDLER(") === 0 ||
			// 虚拟机 热 非常量处理器：5个
            strpos($line,"ZEND_VM_HOT_NOCONST_HANDLER(") === 0 ||
			// ??：7个
            strpos($line,"ZEND_VM_HOT_NOCONSTCONST_HANDLER(") === 0 ||
            // 虚拟机热发送处理器：4个
			strpos($line,"ZEND_VM_HOT_SEND_HANDLER(") === 0 ||
            // 虚拟机热对象处理器：2个
			strpos($line,"ZEND_VM_HOT_OBJ_HANDLER(") === 0 ||
            // 虚拟机冷处理器：6个
			strpos($line,"ZEND_VM_COLD_HANDLER(") === 0 ||
            // 虚拟机冷常量处理器：20个
			strpos($line,"ZEND_VM_COLD_CONST_HANDLER(") === 0 ||
            // 两个操作对象都是常量的操作：21个
			strpos($line,"ZEND_VM_COLD_CONSTCONST_HANDLER(") === 0) {
			// 解析操作码处理器的定义	
          // Parsing opcode handler's definition
		  // 检查定义是否合法
            if (preg_match(
			//  "/^ZEND_VM_(第一段HOT_|INLINE_|HOT_OBJ_|HOT_SEND_|HOT_NOCONST_|HOT_NOCONSTCONST_|COLD_|COLD_CONST_|COLD_CONSTCONST_)?HANDLER\(\s*(第二段[0-9]+)\s*,\s*(第三段[A-Z_]+)\s*,\s*(第四段[A-Z_|]+)\s*,\s*(第五段[A-Z_|]+)\s*(第六段,\s*(第七段[A-Z_|]+)\s*)?(第八段,\s*SPEC\((第九段[A-Z_|=,]+)\)\s*)?\)/"
                    "/^ZEND_VM_(HOT_|INLINE_|HOT_OBJ_|HOT_SEND_|HOT_NOCONST_|HOT_NOCONSTCONST_|COLD_|COLD_CONST_|COLD_CONSTCONST_)?HANDLER\(\s*([0-9]+)\s*,\s*([A-Z_]+)\s*,\s*([A-Z_|]+)\s*,\s*([A-Z_|]+)\s*(,\s*([A-Z_|]+)\s*)?(,\s*SPEC\(([A-Z_|=,]+)\)\s*)?\)/",
                    $line,
                    $m) == 0) {
				// 如果不合法，报错：无效的虚拟机处理器定义
                die("ERROR ($def:$lineno): Invalid ZEND_VM_HANDLER definition.\n");
            }
			// 正则的第一段，处理类型：九种情况，可以是空 (HOT_|INLINE_|HOT_OBJ_|HOT_SEND_|HOT_NOCONST_|HOT_NOCONSTCONST_|COLD_|COLD_CONST_|COLD_CONSTCONST_)
			$hot = !empty($m[1]) ? $m[1] : false;
			// 正则第二段 ([0-9]+)，第一个参数：操作码号，数字，如：24
            $code = (int)$m[2];
			// 正则第三段 ([A-Z_]+)，第二个参数：操作码名，如：ZEND_ASSIGN_OBJ
            $op   = $m[3];
			// 操作码长度
            $len  = strlen($op);
			// 正则第四段，([A-Z_|]+)，第三个参数：第一个操作对象的类型，如：：VAR|UNUSED|THIS|CV
            $op1  = parse_operand_spec($def, $lineno, $m[4], $flags1);
			// 正则第五段，([A-Z_|]+)，第四个参数：第二个操作对象的类型，如：：CONST|TMPVAR|CV
            $op2  = parse_operand_spec($def, $lineno, $m[5], $flags2);
            $flags = $flags1 | ($flags2 << 8);
			// 正则第七段，([A-Z_|]+)，可选参数：标记，如：CACHE_SLOT
            if (!empty($m[7])) {
                $flags |= parse_ext_spec($def, $lineno, $m[7]);
            }
			// 长度不可以大于操作码最大长度
            if ($len > $max_opcode_len) {
                $max_opcode_len = $len;
            }
			// 操作码不可以大于最大操作码
            if ($code > $max_opcode) {
                $max_opcode = $code;
            }
			
			// 创建操作码
            $opcodes[$code] = array("op"=>$op,"op1"=>$op1,"op2"=>$op2,"code"=>"","flags"=>$flags,"hot"=>$hot);
			// 正则第九段，([A-Z_|=,]+)，可选参数：专用规则，如 SPEC(OP_DATA=CONST|TMP|VAR|CV)
            if (isset($m[9])) {
				// 解析专用规则：zend_vm_def.h，行号，规则
                $opcodes[$code]["spec"] = parse_spec_rules($def, $lineno, $m[9]);
				// 如果已经存在 NO_CONST_CONST 标记 
                if (isset($opcodes[$code]["spec"]["NO_CONST_CONST"])) {
					// 添加 ZEND_VM_NO_CONST_CONST 标记
                    $opcodes[$code]["flags"] |= $vm_op_flags["ZEND_VM_NO_CONST_CONST"];
                }
				// 如果已经存在 COMMUTATIVE（交换的） 标记 
                if (isset($opcodes[$code]["spec"]["COMMUTATIVE"])) {
					// 添加 ZEND_VM_COMMUTATIVE 标记
                    $opcodes[$code]["flags"] |= $vm_op_flags["ZEND_VM_COMMUTATIVE"];
                }
            }
			// 操作码名=>操作码号
            $opnames[$op] = $code;
			// 操作码号
            $handler = $code;
			// 助手
            $helper = null;
			// 行号：['handler'=>操作码号]
            $list[$lineno] = array("handler"=>$handler);
		// 1.2 处理以下几种情况：虚拟机类型专用处理器：2个
        } else if (strpos($line,"ZEND_VM_TYPE_SPEC_HANDLER(") === 0 ||
				// 虚拟机热类型专用处理器：36个
                   strpos($line,"ZEND_VM_HOT_TYPE_SPEC_HANDLER(") === 0 ) {
			// 解析操作码处理器的定义	
          // Parsing opcode handler's definition
		  // 检查定义是否合法
            if (preg_match(
                   //"/^ZEND_VM_(第一段HOT_|INLINE_|HOT_OBJ_|HOT_SEND_|HOT_NOCONST_|HOT_NOCONSTCONST_)?TYPE_SPEC_HANDLER\(\s*(第二段[A-Z_|]+)\s*,\s*(第三段(不计数段?:[^(,]|\([^()]*|(?R)*\))*),\s*(第四段[A-Za-z_]+)\s*,\s*(第五段[A-Z_|]+)\s*,\s*(第六段[A-Z_|]+)\s*(第七段,\s*(第八段[A-Z_|]+)\s*)?(第九段,\s*SPEC\((第十段[A-Z_|=,]+)\)\s*)?\)/",
				   "/^ZEND_VM_(HOT_|INLINE_|HOT_OBJ_|HOT_SEND_|HOT_NOCONST_|HOT_NOCONSTCONST_)?TYPE_SPEC_HANDLER\(\s*([A-Z_|]+)\s*,\s*((?:[^(,]|\([^()]*|(?R)*\))*),\s*([A-Za-z_]+)\s*,\s*([A-Z_|]+)\s*,\s*([A-Z_|]+)\s*(,\s*([A-Z_|]+)\s*)?(,\s*SPEC\(([A-Z_|=,]+)\)\s*)?\)/",
                    $line,
                    $m) == 0) {
				// 如果不合法，报错：无效的 虚拟机类型处理器 处理器定义
                die("ERROR ($def:$lineno): Invalid ZEND_VM_TYPE_HANDLER_HANDLER definition.\n");
            }
			// 正则第一段，处理类型：六种情况，可以是空 HOT_|INLINE_|HOT_OBJ_|HOT_SEND_|HOT_NOCONST_|HOT_NOCONSTCONST_
            $hot = !empty($m[1]) ? $m[1] : false;
			// 正则第二段 [A-Z_|]+ ，第一个参数，操作码名列表（|连接多个操作码），如：ZEND_IS_EQUAL|ZEND_IS_IDENTICAL
            $orig_op_list = $m[2];
			// 扩展编号+1
            $code = $extra_num++;
			// 把 $orig_op_list 用 | 分隔开，遍历每个操作码
            foreach (explode('|', $orig_op_list) as $orig_op) {
				// 取得原操作码编号
                $orig_code = $opnames[$orig_op];
				// 正则第三段，(?:[^(,]|\([^()]*|(?R)*\))*，第二个参数，触发条件如：(op1_info == MAY_BE_DOUBLE && op2_info == MAY_BE_DOUBLE)
                $condition = $m[3];
				// 给添加码添加专用类型，值为触发条件
                $opcodes[$orig_code]['type_spec'][$code] = $condition;
            }
			// 正则第四段，[A-Za-z_]+，第三个参数，操作码名，如：ZEND_ASSIGN_OBJ
            $op = $m[4];
			// 正则第五段，[A-Z_|]+，第四个参数，第一个操作对象的类型，如：VAR|UNUSED|THIS|CV
            $op1 = parse_operand_spec($def, $lineno, $m[5], $flags1);
			// 正则第六段，[A-Z_|]+，第五个参数，第二个操作对象的类型，如：CONST|TMPVAR|CV
            $op2 = parse_operand_spec($def, $lineno, $m[6], $flags2);
			// 一个标记有两个标记位置 $flags1 和 $flags2 << 8
            $flags = $flags1 | ($flags2 << 8);
			// 正则第八段，[A-Z_|]+，可选参数，专用扩展标记，如： CACHE_SLOT
            if (!empty($m[8])) {
				// 添加标记
                $flags |= parse_ext_spec($def, $lineno, $m[8]);
            }
			
			// 专用扩展类型，1，？
            $used_extra_spec["TYPE"] = 1;
			// 创建操作码
            $opcodes[$code] = array("op"=>$op,"op1"=>$op1,"op2"=>$op2,"code"=>"","flags"=>$flags,"hot"=>$hot,"is_type_spec"=>true);
			// 正则第十段，[A-Z_|=,]+，可选参数：专用规则，如 SPEC(OP_DATA=CONST|TMP|VAR|CV)
            if (isset($m[10])) {
				// 解析专用规则 
                $opcodes[$code]["spec"] = parse_spec_rules($def, $lineno, $m[10]);
				// 如果存在 NO_CONST_CONST 标记
                if (isset($opcodes[$code]["spec"]["NO_CONST_CONST"])) {
					// 添加 ZEND_VM_NO_CONST_CONST 标记
                    $opcodes[$code]["flags"] |= $vm_op_flags["ZEND_VM_NO_CONST_CONST"];
                }
				// 如果存在 COMMUTATIVE（交换）标记
                if (isset($opcodes[$code]["spec"]["COMMUTATIVE"])) {
					// 添加 ZEND_VM_COMMUTATIVE标记
                    $opcodes[$code]["flags"] |= $vm_op_flags["ZEND_VM_COMMUTATIVE"];
                }
            }
			// 创建操作码
            $opnames[$op] = $code;
			// 处理器为当前操作码编号
            $handler = $code;
			// 定义处理器时清空助手定义
            $helper = null;
			// 助手添加到列表中
            $list[$lineno] = array("handler"=>$handler);
		// 1.3 处理以下几种情况：虚拟机助手：25个
        } else if (strpos($line,"ZEND_VM_HELPER(") === 0 ||
				// 虚拟机冷助手：8个
                   strpos($line,"ZEND_VM_COLD_HELPER(") === 0 ||
				// 虚拟机热助手：1个
                   strpos($line,"ZEND_VM_HOT_HELPER(") === 0) {
			// 解析助手定义
          // Parsing helper's definition
			//检查定义是否合法
            if (preg_match(
					// "/^ZEND_VM(第一段_INLINE|_COLD|_HOT)?_HELPER\(\s*(第二段[A-Za-z_]+)\s*,\s*(第三段[A-Z_|]+)\s*,\s*(第四 段[A-Z_|]+)\s*(不计数段?:,\s*SPEC\((第五段[A-Z_|=,]+)\)\s*)?(?:,\s*(第六段[^)]*)\s*)?\)/",
                    "/^ZEND_VM(_INLINE|_COLD|_HOT)?_HELPER\(\s*([A-Za-z_]+)\s*,\s*([A-Z_|]+)\s*,\s*([A-Z_|]+)\s*(?:,\s*SPEC\(([A-Z_|=,]+)\)\s*)?(?:,\s*([^)]*)\s*)?\)/",
                    $line,
                    $m) == 0) {
				// 如果不合法，报错
                die("ERROR ($def:$lineno): Invalid ZEND_VM_HELPER definition.\n");
            }
			// 正则第一段，处理类型：三种情况，可以是空 _INLINE|_COLD|_HOT
            $inline = !empty($m[1]) && $m[1] === "_INLINE";
            $cold   = !empty($m[1]) && $m[1] === "_COLD";
            $hot    = !empty($m[1]) && $m[1] === "_HOT";
			// 正则第二段，[A-Za-z_]+，第一个参数，助手名，如：zend_sub_helper
            $helper = $m[2];
			// 正则第三段，[A-Z_|]+，第一个操作对象的类型，如：ANY
            $op1    = parse_operand_spec($def, $lineno, $m[3], $flags1);
			// 正则第四段，[A-Z_|]+，第二个操作对象的类型，如：ANY
            $op2    = parse_operand_spec($def, $lineno, $m[4], $flags2);
			// 正则第六段，[^)]*，这是什么东西？
            $param  = isset($m[6]) ? $m[6] : null;
			

			// 创建参数列表
            // Store parameters
			// 
            if ((ZEND_VM_KIND == ZEND_VM_KIND_GOTO
             || ZEND_VM_KIND == ZEND_VM_KIND_SWITCH
			 // ZEND_VM_KIND_HYBRID  + _HOT 模式
             || (ZEND_VM_KIND == ZEND_VM_KIND_HYBRID && $hot))
             && $param) {
				// 拆分参数列表，遍历每个参数
                foreach (explode(",", $param ) as $p) {
					$p = trim($p);
					// 如果参数存在，添加到参数列表里
                    if ($p !== "") {
                        $params[$p] = 1;
                    }
                }
            }
			// 创建助手 
            $helpers[$helper] = array("op1"=>$op1,"op2"=>$op2,"param"=>$param,"code"=>"","inline"=>$inline,"cold"=>$cold,"hot"=>$hot);
			
			// 定义助手时清空处理器定义
            $handler = null;
			// 把助手添加到列表中
            $list[$lineno] = array("helper"=>$helper);
		// 1.4 虚拟机定义操作码：1个
        } else if (strpos($line,"ZEND_VM_DEFINE_OP(") === 0) {
			// 检查定义是否合法
            if (preg_match(
					// "/^ZEND_VM_DEFINE_OP\(\s*(第一段[0-9]+)\s*,\s*(第二段[A-Z_]+)\s*\);/",
                    "/^ZEND_VM_DEFINE_OP\(\s*([0-9]+)\s*,\s*([A-Z_]+)\s*\);/",
                    $line,
                    $m) == 0) {
				// 如果不合法，报错：无效的虚拟机操作定义
                die("ERROR ($def:$lineno): Invalid ZEND_VM_DEFINE_OP definition.\n");
            }
			// 正则第一段，[0-9]+，第一个参数，操作码编号，如：137
            $code = (int)$m[1];
			// 正则第二段，[A-Z_]+，第二个参数，操作码名，如：ZEND_OP_DATA
            $op   = $m[2];
			// 操作码长度
            $len  = strlen($op);

			// 不可以超过最大长度
            if ($len > $max_opcode_len) {
                $max_opcode_len = $len;
            }
			// 不可以超过最大值
            if ($code > $max_opcode) {
                $max_opcode = $code;
            }
			
			// 创建操作码 值为空
            $opcodes[$code] = array("op"=>$op,"code"=>"");
			// 创建操作码名
            $opnames[$op] = $code;
		// 这两行是添加非定义行的代码行
		// 1.5：如果有处理器
        } else if ($handler !== null) {
			// 把 这行代码 添加到当前操作码处理器的代码中
          // Add line of code to current opcode handler
            $opcodes[$handler]["code"] .= $line;
		// 1.6：如果有操作助手 
        } else if ($helper !== null) {
			// 把这行代码添加到 操作助手的代码中
          // Add line of code to current helper
            $helpers[$helper]["code"] .= $line;
        }
    }

	// 键名排序操作码
    ksort($opcodes);
	// 统计处理器参数
	$t = [0=>[],1=>[]];
	foreach($opcodes as $v){
		$is_type_spec = isset($v['is_type_spec']) && $v['is_type_spec'] ?1:0;
		foreach($v as $k2=>$v2){
			if($k2=='code')
				$t[$is_type_spec][$k2] = 0;
			else{
				if(!isset($t[$is_type_spec][$k2]))
					$t[$is_type_spec][$k2] = [];
				$t[$is_type_spec][$k2][json_encode($v2)] = 0;
			}
			
		}
	}
	// var_dump($t);die;
	
	// 统计助手参数
	$t = [];
	foreach($helpers as $v){
		foreach($v as $k2=>$v2){
			if($k2=='code')
				$t[$k2] = 0;
			else{
				if(!isset($t[$k2]))
					$t[$k2] = [];
				$t[$k2][json_encode($v2)] = 0;
			}
			
		}
	}
	// var_dump($t);die;
	// var_dump($list);die;
	
	//var_dump($opcodes,count($opcodes));die;

	// 2. 为操作码处理器查找 那些被其他操作码用过的处理器
    // Search for opcode handlers those are used by other opcode handlers
	// 遍历， 操作码编号索引的 处理器列表
    foreach ($opcodes as $dsc) {
		// 遍历所有操作码
        if (preg_match("/^\s*{\s*ZEND_VM_DISPATCH_TO_HANDLER\(\s*([A-Z_]*)\s*\)\s*;\s*}\s*/", $dsc["code"], $m)) {
			// 第一段 [A-Z_]*
            $op = $m[1];			
			// 
            $opcodes[$opnames[$dsc['op']]]['alias'] = $op;			
		// 
        } else if (preg_match_all("/ZEND_VM_DISPATCH_TO_HANDLER\(\s*([A-Z_]*)\s*\)/m", $dsc["code"], $mm, PREG_SET_ORDER)) {
            foreach ($mm as $m) {
                $op = $m[1];
                $code = $opnames[$op];
                $opcodes[$code]['use'] = 1;
            }
        }
    }
	// var_dump($opcodes,count($opcodes));die;

	// 3. 生成操作码定义 zend_vm_opcodes.h
    // Generate opcode #defines (zend_vm_opcodes.h)
    $str = gen_vm_opcodes_header($opcodes, $max_opcode, $max_opcode_len, $vm_op_flags);
	// 写入文件 zend_vm_opcodes.h
    write_file_if_changed(__DIR__ . "/zend_vm_opcodes_2.h", $str);
	// 生成操作码完毕
    echo "zend_vm_opcodes.h generated successfully.\n";

    // 开始创建文件，此后无复杂逻辑
	// 4. 写入 zend_vm_opcodes.c
    $f = fopen(__DIR__ . "/zend_vm_opcodes_2.c", "w+") or die("ERROR: Cannot create zend_vm_opcodes.c\n");

	// 插入头部内容
	// Insert header
    out($f, HEADER_TEXT);
    fputs($f,"#include <stdio.h>\n");
    fputs($f,"#include <zend.h>\n");
    fputs($f,"#include <zend_vm_opcodes.h>\n\n");

    fputs($f,"static const char *zend_vm_opcodes_names[".($max_opcode + 1)."] = {\n");
    for ($i = 0; $i <= $max_opcode; $i++) {
        fputs($f,"\t".(isset($opcodes[$i]["op"])?'"'.$opcodes[$i]["op"].'"':"NULL").",\n");
    }
    fputs($f, "};\n\n");

    fputs($f,"static uint32_t zend_vm_opcodes_flags[".($max_opcode + 1)."] = {\n");
    for ($i = 0; $i <= $max_opcode; $i++) {
        fprintf($f, "\t0x%08x,\n", isset($opcodes[$i]["flags"]) ? $opcodes[$i]["flags"] : 0);
    }
    fputs($f, "};\n\n");

    fputs($f, "ZEND_API const char* ZEND_FASTCALL zend_get_opcode_name(zend_uchar opcode) {\n");
    fputs($f, "\tif (UNEXPECTED(opcode > ZEND_VM_LAST_OPCODE)) {\n");
    fputs($f, "\t\treturn NULL;\n");
    fputs($f, "\t}\n");
    fputs($f, "\treturn zend_vm_opcodes_names[opcode];\n");
    fputs($f, "}\n");

    fputs($f, "ZEND_API uint32_t ZEND_FASTCALL zend_get_opcode_flags(zend_uchar opcode) {\n");
    fputs($f, "\tif (UNEXPECTED(opcode > ZEND_VM_LAST_OPCODE)) {\n");
    fputs($f, "\t\topcode = ZEND_NOP;\n");
    fputs($f, "\t}\n");
    fputs($f, "\treturn zend_vm_opcodes_flags[opcode];\n");
    fputs($f, "}\n");

    fputs($f, "ZEND_API zend_uchar zend_get_opcode_id(const char *name, size_t length) {\n");
    fputs($f, "\tzend_uchar opcode;\n");
    fputs($f, "\tfor (opcode = 0; opcode < (sizeof(zend_vm_opcodes_names) / sizeof(zend_vm_opcodes_names[0])) - 1; opcode++) {\n");
    fputs($f, "\t\tconst char *opcode_name = zend_vm_opcodes_names[opcode];\n");
    fputs($f, "\t\tif (opcode_name && strncmp(opcode_name, name, length) == 0) {\n");
    fputs($f, "\t\t\treturn opcode;\n");
    fputs($f, "\t\t}\n");
    fputs($f, "\t}\n");
    fputs($f, "\treturn ZEND_VM_LAST_OPCODE + 1;\n");
    fputs($f, "}\n");

    fclose($f);
	// zend_vm_opcodes.c 生成完毕
    echo "zend_vm_opcodes.c generated successfully.\n";

	// 5. 生成 zend_vm_execute.h （大货）
    // Generate zend_vm_execute.h
    // 打开文件
	$f = fopen(__DIR__ . "/zend_vm_execute_2.h", "w+") or die("ERROR: Cannot create zend_vm_execute.h\n");
	// 执行器文件绝对路径
    $executor_file = realpath(__DIR__ . "/zend_vm_execute_2.h");

	// 5.1插入头部内容
    // Insert header
    out($f, HEADER_TEXT);

    out($f, "#ifdef ZEND_WIN32\n");
	// 在 windows 里抑制 free_op1 警告
    // Suppress free_op1 warnings on Windows
    out($f, "# pragma warning(disable : 4101)\n");
    if (ZEND_VM_SPEC) {
        // Suppress (<non-zero constant> || <expression>) warnings on windows
        out($f, "# pragma warning(once : 6235)\n");
        // Suppress (<zero> && <expression>) warnings on windows
        out($f, "# pragma warning(once : 6237)\n");
        // Suppress (<non-zero constant> && <expression>) warnings on windows
        out($f, "# pragma warning(once : 6239)\n");
        // Suppress (<expression> && <non-zero constant>) warnings on windows
        out($f, "# pragma warning(once : 6240)\n");
        // Suppress (<non-zero constant> || <non-zero constant>) warnings on windows
        out($f, "# pragma warning(once : 6285)\n");
        // Suppress (<non-zero constant> || <expression>) warnings on windows
        out($f, "# pragma warning(once : 6286)\n");
        // Suppress constant with constant comparison warnings on windows
        out($f, "# pragma warning(once : 6326)\n");
    }
    out($f, "#endif\n");

	// 支持 ZEND_USER_OPCODE
    // Support for ZEND_USER_OPCODE
    out($f, "static user_opcode_handler_t zend_user_opcode_handlers[256] = {\n");
    for ($i = 0; $i < 255; ++$i) {
        out($f, "\t(user_opcode_handler_t)NULL,\n");
    }
    out($f, "\t(user_opcode_handler_t)NULL\n};\n\n");

    out($f, "static zend_uchar zend_user_opcodes[256] = {");
    for ($i = 0; $i < 255; ++$i) {
        if ($i % 16 == 1) out($f, "\n\t");
        out($f, "$i,");
    }
    out($f, "255\n};\n\n");

	// 5.2 生成专用执行器
    // Generate specialized executor
	// 参数：zend_vm_execute.h文件句柄，zend_vm_execute.skl文件内容，ZEND_VM_SPEC=1, ZEND_VM_KIND, "execute", "zend_vm_init"
    gen_executor($f, $skl, ZEND_VM_SPEC, ZEND_VM_KIND, "execute", "zend_vm_init");
    out($f, "\n");

	// 以下生成5个特殊函数
	// 5.3 生成 特殊函数1: zend_vm_get_opcode_handler 函数 
    // Generate zend_vm_get_opcode_handler() function
	// 获取操作码处理器序号，两个参数 
	// var_dump($used_extra_spec);die;
    out($f, "static uint32_t ZEND_FASTCALL zend_vm_get_opcode_handler_idx(uint32_t spec, const zend_op* op)\n");
    out($f, "{\n");
    if (!ZEND_VM_SPEC) {
        out($f, "\treturn spec;\n");
    } else {
        out($f, "\tstatic const int zend_vm_decode[] = {\n");
        out($f, "\t\t_UNUSED_CODE, /* 0 = IS_UNUSED  */\n");
        out($f, "\t\t_CONST_CODE,  /* 1 = IS_CONST   */\n");
        out($f, "\t\t_TMP_CODE,    /* 2 = IS_TMP_VAR */\n");
        out($f, "\t\t_UNUSED_CODE, /* 3              */\n");
        out($f, "\t\t_VAR_CODE,    /* 4 = IS_VAR     */\n");
        out($f, "\t\t_UNUSED_CODE, /* 5              */\n");
        out($f, "\t\t_UNUSED_CODE, /* 6              */\n");
        out($f, "\t\t_UNUSED_CODE, /* 7              */\n");
        out($f, "\t\t_CV_CODE      /* 8 = IS_CV      */\n");
        out($f, "\t};\n");
        out($f, "\tuint32_t offset = 0;\n");
        out($f, "\tif (spec & SPEC_RULE_OP1) offset = offset * 5 + zend_vm_decode[op->op1_type];\n");
        out($f, "\tif (spec & SPEC_RULE_OP2) offset = offset * 5 + zend_vm_decode[op->op2_type];\n");

        if (isset($used_extra_spec["OP_DATA"]) ||
            isset($used_extra_spec["RETVAL"]) ||
            isset($used_extra_spec["QUICK_ARG"]) ||
            isset($used_extra_spec["SMART_BRANCH"]) ||
            isset($used_extra_spec["ISSET"]) ||
            isset($used_extra_spec["OBSERVER"])) {

            $else = "";
            out($f, "\tif (spec & SPEC_EXTRA_MASK) {\n");

            if (isset($used_extra_spec["RETVAL"])) {
                out($f, "\t\t{$else}if (spec & SPEC_RULE_RETVAL) {\n");
                out($f, "\t\t\toffset = offset * 2 + (op->result_type != IS_UNUSED);\n");
                out($f, "\t\t\tif ((spec & SPEC_RULE_OBSERVER) && ZEND_OBSERVER_ENABLED) {\n");
                out($f,	"\t\t\t\toffset += 2;\n");
                out($f, "\t\t\t}\n");
                $else = "} else ";
            }
            if (isset($used_extra_spec["QUICK_ARG"])) {
                out($f, "\t\t{$else}if (spec & SPEC_RULE_QUICK_ARG) {\n");
                out($f, "\t\t\toffset = offset * 2 + (op->op2.num <= MAX_ARG_FLAG_NUM);\n");
                $else = "} else ";
            }
            if (isset($used_extra_spec["OP_DATA"])) {
                out($f, "\t\t{$else}if (spec & SPEC_RULE_OP_DATA) {\n");
                out($f, "\t\t\toffset = offset * 5 + zend_vm_decode[(op + 1)->op1_type];\n");
                $else = "} else ";
            }
            if (isset($used_extra_spec["ISSET"])) {
                out($f, "\t\t{$else}if (spec & SPEC_RULE_ISSET) {\n");
                out($f, "\t\t\toffset = offset * 2 + (op->extended_value & ZEND_ISEMPTY);\n");
                $else = "} else ";
            }
            if (isset($used_extra_spec["SMART_BRANCH"])) {
                out($f, "\t\t{$else}if (spec & SPEC_RULE_SMART_BRANCH) {\n");
                out($f,	"\t\t\toffset = offset * 3;\n");
                out($f, "\t\t\tif (op->result_type == (IS_SMART_BRANCH_JMPZ|IS_TMP_VAR)) {\n");
                out($f,	"\t\t\t\toffset += 1;\n");
                out($f, "\t\t\t} else if (op->result_type == (IS_SMART_BRANCH_JMPNZ|IS_TMP_VAR)) {\n");
                out($f,	"\t\t\t\toffset += 2;\n");
                out($f, "\t\t\t}\n");
                $else = "} else ";
            }
            if (isset($used_extra_spec["OBSERVER"])) {
                out($f, "\t\t{$else}if (spec & SPEC_RULE_OBSERVER) {\n");
                out($f,	"\t\t\toffset = offset * 2;\n");
                out($f, "\t\t\tif (ZEND_OBSERVER_ENABLED) {\n");
                out($f,	"\t\t\t\toffset += 1;\n");
                out($f, "\t\t\t}\n");
                $else = "} else ";
            }
            if ($else !== "") {
                out($f, "\t\t}\n");
            }
            out($f, "\t}\n");
        }
        out($f, "\treturn (spec & SPEC_START_MASK) + offset;\n");
    }
    out($f, "}\n\n");
    out($f, "#if (ZEND_VM_KIND != ZEND_VM_KIND_HYBRID) || !ZEND_VM_SPEC\n");
    out($f, "static const void *zend_vm_get_opcode_handler(zend_uchar opcode, const zend_op* op)\n");
    out($f, "{\n");
    if (!ZEND_VM_SPEC) {
        out($f, "\treturn zend_opcode_handlers[zend_vm_get_opcode_handler_idx(opcode, op)];\n");
    } else {
        out($f, "\treturn zend_opcode_handlers[zend_vm_get_opcode_handler_idx(zend_spec_handlers[opcode], op)];\n");
    }
    out($f, "}\n");
    out($f, "#endif\n\n");

	// 如果用 ZEND_VM_KIND_HYBRID 模式生成
    if (ZEND_VM_KIND == ZEND_VM_KIND_HYBRID) {
		// 5.4 生成 特殊函数2:  zend_vm_get_opcode_handler_func 函数
        // Generate zend_vm_get_opcode_handler_func() function
		// 如果运行时是 ZEND_VM_KIND_HYBRID 模式
        out($f, "#if ZEND_VM_KIND == ZEND_VM_KIND_HYBRID\n");
        out($f,"static const void *zend_vm_get_opcode_handler_func(zend_uchar opcode, const zend_op* op)\n");
        out($f, "{\n");
        out($f, "\tuint32_t spec = zend_spec_handlers[opcode];\n");
        if (!ZEND_VM_SPEC) {
            out($f, "\treturn zend_opcode_handler_funcs[spec];\n");
        } else {
            out($f, "\treturn zend_opcode_handler_funcs[zend_vm_get_opcode_handler_idx(spec, op)];\n");
        }
        out($f, "}\n\n");
        out($f, "#endif\n\n");
    }

	// 5.5 生成 特殊函数3:  zend_vm_set_opcode_handler 函数 
    // Generate zend_vm_get_opcode_handler() function
    out($f, "ZEND_API void ZEND_FASTCALL zend_vm_set_opcode_handler(zend_op* op)\n");
    out($f, "{\n");
    out($f, "\tzend_uchar opcode = zend_user_opcodes[op->opcode];\n");
    if (!ZEND_VM_SPEC) {
        out($f, "\top->handler = zend_opcode_handlers[zend_vm_get_opcode_handler_idx(opcode, op)];\n");
    } else {
        out($f, "\n");
        out($f, "\tif (zend_spec_handlers[op->opcode] & SPEC_RULE_COMMUTATIVE) {\n");
        out($f, "\t\tif (op->op1_type < op->op2_type) {\n");
        out($f, "\t\t\tzend_swap_operands(op);\n");
        out($f, "\t\t}\n");
        out($f, "\t}\n");
        out($f, "\top->handler = zend_opcode_handlers[zend_vm_get_opcode_handler_idx(zend_spec_handlers[opcode], op)];\n");
    }
    out($f, "}\n\n");

	// 5.6 生成 特殊函数4:  zend_vm_set_opcode_handler_ex 函数 
    // Generate zend_vm_set_opcode_handler_ex() function
    out($f, "ZEND_API void ZEND_FASTCALL zend_vm_set_opcode_handler_ex(zend_op* op, uint32_t op1_info, uint32_t op2_info, uint32_t res_info)\n");
    out($f, "{\n");
    out($f, "\tzend_uchar opcode = zend_user_opcodes[op->opcode];\n");
    if (!ZEND_VM_SPEC) {
        out($f, "\top->handler = zend_opcode_handlers[zend_vm_get_opcode_handler_idx(opcode, op)];\n");
    } else {
        out($f, "\tuint32_t spec = zend_spec_handlers[opcode];\n");
        if (isset($used_extra_spec["TYPE"])) {
            out($f, "\tswitch (opcode) {\n");
			// 遍历 操作码编号索引的 处理器列表
            foreach ($opcodes as $code => $dsc) {
				// 
                if (isset($dsc['type_spec'])) {
                    $orig_op = $dsc['op'];
                    out($f, "\t\tcase $orig_op:\n");
                    if (isset($dsc["spec"]["COMMUTATIVE"])) {
                        out($f, "\t\t\tif (op->op1_type < op->op2_type) {\n");
                        out($f, "\t\t\t\tzend_swap_operands(op);\n");
                        out($f, "\t\t\t}\n");
                    }
                    $first = true;
                    foreach ($dsc['type_spec'] as $code => $condition) {
                        $condition = format_condition($condition);
                        if ($first) {
                            out($f, "\t\t\tif $condition {\n");
                            $first = false;
                        } else {
                            out($f, "\t\t\t} else if $condition {\n");
                        }
                        $spec_dsc = $opcodes[$code];
                        if (isset($spec_dsc["spec"]["NO_CONST_CONST"])) {
                            out($f, "\t\t\t\tif (op->op1_type == IS_CONST && op->op2_type == IS_CONST) {\n");
                            out($f, "\t\t\t\t\tbreak;\n");
                            out($f, "\t\t\t\t}\n");
                        }
                        out($f, "\t\t\t\tspec = {$spec_dsc['spec_code']};\n");
                        if (isset($spec_dsc["spec"]["COMMUTATIVE"]) && !isset($dsc["spec"]["COMMUTATIVE"])) {
                            out($f, "\t\t\t\tif (op->op1_type < op->op2_type) {\n");
                            out($f, "\t\t\t\t\tzend_swap_operands(op);\n");
                            out($f, "\t\t\t\t}\n");
                        }
                    }
                    if (!$first) {
                        out($f, "\t\t\t}\n");
                    }
                    out($f, "\t\t\tbreak;\n");
                }
            }
            $has_commutative = false;
			// 遍历 操作码编号索引的 处理器列表
            foreach ($opcodes as $code => $dsc) {
                if (!isset($dsc['is_type_spec']) &&
                    !isset($dsc['type_spec']) &&
                    isset($dsc["spec"]["COMMUTATIVE"])) {
                    $orig_op = $dsc['op'];
                    out($f, "\t\tcase $orig_op:\n");
                    $has_commutative = true;
                }
            }
            if ($has_commutative) {
                out($f, "\t\t\tif (op->op1_type < op->op2_type) {\n");
                out($f, "\t\t\t\tzend_swap_operands(op);\n");
                out($f, "\t\t\t}\n");
                out($f, "\t\t\tbreak;\n");
                out($f, "\t\tcase ZEND_USER_OPCODE:\n");
                out($f, "\t\t\tif (zend_spec_handlers[op->opcode] & SPEC_RULE_COMMUTATIVE) {\n");
                out($f, "\t\t\t\tif (op->op1_type < op->op2_type) {\n");
                out($f, "\t\t\t\t\tzend_swap_operands(op);\n");
                out($f, "\t\t\t\t}\n");
                out($f, "\t\t\t}\n");
                out($f, "\t\t\tbreak;\n");
            }
            out($f, "\t\tdefault:\n");
            out($f, "\t\t\tbreak;\n");
            out($f, "\t}\n");
        }
        out($f, "\top->handler = zend_opcode_handlers[zend_vm_get_opcode_handler_idx(spec, op)];\n");
    }
    out($f, "}\n\n");

	// 5.7 生成 特殊函数5:  zend_vm_call_opcode_handler 函数 
    // Generate zend_vm_call_opcode_handler() function
	// 如果用 ZEND_VM_KIND_CALL 或 ZEND_VM_KIND_HYBRID 模式生成
    if (ZEND_VM_KIND == ZEND_VM_KIND_CALL || ZEND_VM_KIND == ZEND_VM_KIND_HYBRID) {
        out($f, "ZEND_API int ZEND_FASTCALL zend_vm_call_opcode_handler(zend_execute_data* ex)\n");
        out($f, "{\n");
		// 如果用 HYBRID 模式生成
        if (ZEND_VM_KIND == ZEND_VM_KIND_HYBRID) {
			// 如果运行时是 ZEND_VM_KIND_HYBRID 模式
            out($f,"#if (ZEND_VM_KIND == ZEND_VM_KIND_HYBRID)\n");
			// 声名 handler
            out($f, "\topcode_handler_t handler;\n");
            out($f,"#endif\n");
        }
		// 声名 ret
        out($f, "\tint ret;\n");
		// 如果有 ZEND_VM_IP_GLOBAL_REG
        out($f, "#ifdef ZEND_VM_IP_GLOBAL_REG\n");
        out($f, "\tconst zend_op *orig_opline = opline;\n");
        out($f, "#endif\n");
        out($f, "#ifdef ZEND_VM_FP_GLOBAL_REG\n");
        out($f, "\tzend_execute_data *orig_execute_data = execute_data;\n");
        out($f, "\texecute_data = ex;\n");
        out($f, "#else\n");
        out($f, "\tzend_execute_data *execute_data = ex;\n");
        out($f, "#endif\n");
        out($f, "\n");
        out($f, "\tLOAD_OPLINE();\n");
        out($f,"#if defined(ZEND_VM_FP_GLOBAL_REG) && defined(ZEND_VM_IP_GLOBAL_REG)\n");
		// 如果用 HYBRID 模式生成
        if (ZEND_VM_KIND == ZEND_VM_KIND_HYBRID) {
			// 如果运行时是 ZEND_VM_KIND_HYBRID 模式
            out($f,"#if (ZEND_VM_KIND == ZEND_VM_KIND_HYBRID)\n");
            out($f, "\thandler = (opcode_handler_t)zend_vm_get_opcode_handler_func(zend_user_opcodes[opline->opcode], opline);\n");
            out($f, "\thandler(ZEND_OPCODE_HANDLER_ARGS_PASSTHRU);\n");
            out($f, "\tif (EXPECTED(opline != &hybrid_halt_op)) {\n");
            out($f,"#else\n");
        }
        out($f, "\t((opcode_handler_t)OPLINE->handler)(ZEND_OPCODE_HANDLER_ARGS_PASSTHRU);\n");
		// 如果用 ZEND_VM_KIND_HYBRID 模式生成
        if (ZEND_VM_KIND == ZEND_VM_KIND_HYBRID) {
            out($f, "\tif (EXPECTED(opline)) {\n");
			// 宏结尾
            out($f,"#endif\n");
        } else {
            out($f, "\tif (EXPECTED(opline)) {\n");
        }
        out($f, "\t\tret = execute_data != ex ? (int)(execute_data->prev_execute_data != ex) + 1 : 0;\n");
        out($f, "\t\tSAVE_OPLINE();\n");
        out($f, "\t} else {\n");
        out($f, "\t\tret = -1;\n");
        out($f, "\t}\n");
        out($f, "#else\n");
        out($f, "\tret = ((opcode_handler_t)OPLINE->handler)(ZEND_OPCODE_HANDLER_ARGS_PASSTHRU);\n");
        out($f, "\tSAVE_OPLINE();\n");
        out($f, "#endif\n");
        out($f, "#ifdef ZEND_VM_FP_GLOBAL_REG\n");
        out($f, "\texecute_data = orig_execute_data;\n");
        out($f, "#endif\n");
        out($f, "#ifdef ZEND_VM_IP_GLOBAL_REG\n");
        out($f, "\topline = orig_opline;\n");
        out($f, "#endif\n");
        out($f, "\treturn ret;\n");
        out($f, "}\n\n");
	// 其他运行模式 ...
    } else {
        
    }
	// 关闭文件
    fclose($f);
	// zend_vm_execute.h 生成完毕
    echo "zend_vm_execute.h generated successfully.\n";
}

// clear, 如果有改变，重写文件：gen_vm 调用
function write_file_if_changed(string $filename, string $contents) {
    if (file_exists($filename)) {
        $orig_contents = file_get_contents($filename);
        if ($orig_contents === $contents) {
            // Unchanged, no need to write.
            return;
        }
    }

    file_put_contents($filename, $contents);
}

// ing4, 使用说明
function usage() {
    echo("\nUsage: php zend_vm_gen.php [options]\n".
         "\nOptions:".
		 // 线程模式参数，默认 HYBRID
         "\n  --with-vm-kind=CALL|SWITCH|GOTO|HYBRID - select threading model (default is HYBRID)".
		 // 不使用专用处理？
         "\n  --without-specializer                  - disable executor specialization".
		 // 开户 #line 指令
         "\n  --with-lines                           - enable #line directives".
         "\n\n");
}

// ing4, 解析参数
// Parse arguments
// 遍历所有命令行参数
for ($i = 1; $i < $argc; $i++) {
	// 如果是 --with-vm-kind=
    if (strpos($argv[$i],"--with-vm-kind=") === 0) {
		// 获取 模式字串
        $kind = substr($argv[$i], strlen("--with-vm-kind="));
		// 按不同模式处理
        switch ($kind) {
            case "CALL":
                define("ZEND_VM_KIND", ZEND_VM_KIND_CALL);
                break;
            case "SWITCH":
                define("ZEND_VM_KIND", ZEND_VM_KIND_SWITCH);
                break;
            case "GOTO":
                define("ZEND_VM_KIND", ZEND_VM_KIND_GOTO);
                break;
            case "HYBRID":
                define("ZEND_VM_KIND", ZEND_VM_KIND_HYBRID);
                break;			
        }
	// 如果是 --without-specializer
    } else if ($argv[$i] == "--without-specializer") {
		// 定义常量禁用 specialization
        // Disabling specialization
        define("ZEND_VM_SPEC", 0);
	// 如果是 --with-lines
    } else if ($argv[$i] == "--with-lines") {
		// 允许使用原 zend_vm_def.h 文件进行调试
        // Enabling debugging using original zend_vm_def.h
        define("ZEND_VM_LINES", 1);
	// 显示帮助信息
    } else if ($argv[$i] == "--help") {
        usage();
        exit();
    } 
}

// ing4, 默认模式
// Using defaults
if (!defined("ZEND_VM_KIND")) {
	// 默认使用 CALL 线程
    // Using CALL threading by default
    define("ZEND_VM_KIND", ZEND_VM_KIND_HYBRID);
}
if (!defined("ZEND_VM_SPEC")) {
	// 默认使用 专用执行器
    // Using specialized executor by default
    define("ZEND_VM_SPEC", 1);
}
if (!defined("ZEND_VM_LINES")) {
	// 禁止使用 #line 指令
    // Disabling #line directives
    define("ZEND_VM_LINES", 0);
}

// ing4, 虚拟机操作码文件，执行方法文件？
gen_vm(__DIR__ . "/zend_vm_def.h", __DIR__ . "/zend_vm_execute.skl");

// var_dump($list);
//var_dump($opnames[45]);