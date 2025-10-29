// 特殊
	// ZEND_AST_DIM -> $a['a'] 或 $a{'a'} 但后一种方法已启用，所以只是获取数组元素

// 引用特别多的语句成分
	// name: 非变量的名称，不包含保留字，包含命名空间
		// 包括 T_NAME_QUALIFIED,T_NAME_FULLY_QUALIFIED,T_NAME_RELATIVE
	// identifier: 非变量的名称，保留字，半保留字
		// 不包括 T_NAME_QUALIFIED,T_NAME_FULLY_QUALIFIED,T_NAME_RELATIVE
	// class_name:  用于引用的类名：static 或 name , 这个不和声明类 T_CLASS 共用
	// use_type: function | const 
	
	// reserved_non_modifiers: 保留字
	// semi_reserved: 半保留字，包含保留字

	// namespace_name: 用来声名的命名空间，不是 \ 和 namespace 开头的 
	// legacy_namespace_name: 用于use的命名空间，增加一个绝对路径，不包含 namespace开头的

	// backup_doc_comment:, backup_lex_pos: 无语句，用来触发内部业务逻辑
	// case_separator: 分隔符 : ;
	// constant: name + 魔术常量
	// T_VARIABLE: 内置变量 ，由 $+变量名组成，不可以加 ->, []
	// simple_variable: 简单变量：由内置变量 ，${表达式}，n个$$开头（如$$a, $$$b）3种情况
	// is_reference: 可选 &
	// is_variadic: 可选 ...
	// possible_comma: 可选的, 这个明显是为了人性化设计，
		// 因为编程时经常会多写个,或者忘了删除，但实际上并不影响程序逻辑

	// variable: 非常复杂的一个东西
	// 难点：
		// 引号里面加变量，又加offset的写法，
			// 这个其实不重要，因为可以在引号外面先存到简单变量中
		// 普通变量
	
	// statement + expr 几乎涵盖了一切了，最基本的还是 statement，因为有些东西不在expr里
		// 所以语法最基本的结构是 statement 递归引用自己
		
	// 最主要的思路： 脚本语言的语法并不是从解析开始就都定义完整了，解析只是定义个大概框架，有很多异常是需要编译和执行时处理的

// 原生方法调用次数		
$$ = zend_ast_create	120
$$ = zend_ast_create_list	39
$$ = zend_ast_list_add	38
$$ = zend_ast_create_ex	29
$$ = zend_ast_create_binary_op	20
$$ = zend_ast_create_assign_op	12
$$ = zend_ast_create_decl	9
$$ = zend_ast_create_cast	7
$$ = zend_ast_with_attributes	6
$$ = zend_ast_create_zval_from_str	2
$$ = zend_ast_create_zval	2
$$ = zend_ast_create_class_const_or_name	2
$$ = zend_add_member_modifier	2
$$ = zend_negate_num_string	1
$$ = zend_ast_list_rtrim	1
$$ = zend_ast_create_zval_ex	1
$$ = zend_ast_create_concat_op	1
$$ = zend_add_class_modifier	1
总计	293

		
%require "3.0"
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
   |          Nikita Popov <nikic@php.net>                                |
   +----------------------------------------------------------------------+
*/

%code top {
#include "zend.h"
#include "zend_list.h"
#include "zend_globals.h"
#include "zend_API.h"
#include "zend_constants.h"
#include "zend_language_scanner.h"
#include "zend_exceptions.h"

#define YYSIZE_T size_t
#define yytnamerr zend_yytnamerr
static YYSIZE_T zend_yytnamerr(char*, const char*);

#ifdef _MSC_VER
#define YYMALLOC malloc
#define YYFREE free
#endif
}

%code requires {
#include "zend_compile.h"
}

%define api.prefix {zend}
%define api.pure full
%define api.value.type {zend_parser_stack_elem}
%define parse.error verbose
%expect 0

// ? 销毁器
%destructor { zend_ast_destroy($$); } <ast>
%destructor { if ($$) zend_string_release_ex($$, 0); } <str>

// ?
%precedence T_THROW
%precedence PREC_ARROW_FUNCTION
%precedence T_INCLUDE T_INCLUDE_ONCE T_REQUIRE T_REQUIRE_ONCE
%left T_LOGICAL_OR
%left T_LOGICAL_XOR
%left T_LOGICAL_AND
%precedence T_PRINT
%precedence T_YIELD
%precedence T_DOUBLE_ARROW
%precedence T_YIELD_FROM
%precedence '=' T_PLUS_EQUAL T_MINUS_EQUAL T_MUL_EQUAL T_DIV_EQUAL T_CONCAT_EQUAL T_MOD_EQUAL T_AND_EQUAL T_OR_EQUAL T_XOR_EQUAL T_SL_EQUAL T_SR_EQUAL T_POW_EQUAL T_COALESCE_EQUAL
%left '?' ':'
%right T_COALESCE
%left T_BOOLEAN_OR
%left T_BOOLEAN_AND
%left '|'
%left '^'
%left T_AMPERSAND_NOT_FOLLOWED_BY_VAR_OR_VARARG T_AMPERSAND_FOLLOWED_BY_VAR_OR_VARARG
%nonassoc T_IS_EQUAL T_IS_NOT_EQUAL T_IS_IDENTICAL T_IS_NOT_IDENTICAL T_SPACESHIP
%nonassoc '<' T_IS_SMALLER_OR_EQUAL '>' T_IS_GREATER_OR_EQUAL
%left '.'
%left T_SL T_SR
%left '+' '-'
%left '*' '/' '%'
%precedence '!'
%precedence T_INSTANCEOF
%precedence '~' T_INT_CAST T_DOUBLE_CAST T_STRING_CAST T_ARRAY_CAST T_OBJECT_CAST T_BOOL_CAST T_UNSET_CAST '@'
%right T_POW
%precedence T_CLONE

/* Resolve danging else conflict */
// 结束，什么也没有
%precedence T_NOELSE
// 下面有定义
%precedence T_ELSEIF
%precedence T_ELSE

// 这些居然都是在php脚本里可以访问的
	// 分析利器 echo token_name(token_get_all('<?php 0177777777777777777777787')[1][0]), "\n";
	
// 这些token的类型都是 ast，不是ident
	// %token <ast>  开头的都是类型，不是字串

// 这些是bison的内置规则???

// clear, 匹配：整型
%token <ast> T_LNUMBER   "integer"
// clear, 匹配：双字节数字，长整型或float型
%token <ast> T_DNUMBER   "floating-point number"
// clear, 匹配：所有不在引号里的字符串(中间不能有\，否则被认成命名空间)
%token <ast> T_STRING    "identifier"

// clear, 匹配：\ 开头的字符, 带命名空间的完整引用
%token <ast> T_NAME_FULLY_QUALIFIED "fully qualified name"

// clear, 匹配： namespace\ 开头的无引号字符串 ( 在Php脚本里，这是非法字符 )
	// 命名空间名字的第一节不可以是namespace，不区分大小写	
	// 应该是为了防止解析出错 ，先把这个情况排除
%token <ast> T_NAME_RELATIVE "namespace-relative name"

// clear, 匹配： 不是 \ 开头但中间有 \ 的字符
%token <ast> T_NAME_QUALIFIED "namespaced name"

// clear, 匹配：所有$开头的变量名
%token <ast> T_VARIABLE  "variable"

// clear, 匹配：不在php标签里的html内容
%token <ast> T_INLINE_HTML

// ing3, 匹配：引号中的字串里面有需要解析的变量，先把字串拆开
	// "11{\$a['abc']}22"; 中的 11 ， 22 是 T_ENCAPSED_AND_WHITESPACE
	// "\$a['abc']";
	// 要重点关注一下
%token <ast> T_ENCAPSED_AND_WHITESPACE  "string content"

// clear, 匹配：所有括在引号里的字符
%token <ast> T_CONSTANT_ENCAPSED_STRING "quoted string"

// clear, 字符串变量名, 在字串中引用变量的另一种写法
	// $abcd = '99'; echo "${abcd}";
	// 变量名不是数字开头，才能匹配上。 如果是数字会匹配到 T_LNUMBER
%token <ast> T_STRING_VARNAME "variable name"

// clear, 在字串中引用变量，并用[]取得子元素时，匹配[]中的元素索引号
	$a = "123"; echo "$a[1]";
%token <ast> T_NUM_STRING "number"

// ing3，php 关键字, 有些是字串，有些是预定义规则
	
%token <ident> T_INCLUDE       "'include'"
%token <ident> T_INCLUDE_ONCE  "'include_once'"
%token <ident> T_EVAL          "'eval'"
%token <ident> T_REQUIRE       "'require'"
%token <ident> T_REQUIRE_ONCE  "'require_once'"
%token <ident> T_LOGICAL_OR    "'or'"
%token <ident> T_LOGICAL_XOR   "'xor'"
%token <ident> T_LOGICAL_AND   "'and'"
%token <ident> T_PRINT         "'print'"
%token <ident> T_YIELD         "'yield'"
%token <ident> T_YIELD_FROM    "'yield from'"
%token <ident> T_INSTANCEOF    "'instanceof'"
%token <ident> T_NEW           "'new'"
%token <ident> T_CLONE         "'clone'"
%token <ident> T_EXIT          "'exit'"
%token <ident> T_IF            "'if'"
%token <ident> T_ELSEIF        "'elseif'"
%token <ident> T_ELSE          "'else'"
%token <ident> T_ENDIF         "'endif'"
%token <ident> T_ECHO          "'echo'"
%token <ident> T_DO            "'do'"
%token <ident> T_WHILE         "'while'"
%token <ident> T_ENDWHILE      "'endwhile'"
%token <ident> T_FOR           "'for'"
%token <ident> T_ENDFOR        "'endfor'"
%token <ident> T_FOREACH       "'foreach'"
%token <ident> T_ENDFOREACH    "'endforeach'"
%token <ident> T_DECLARE       "'declare'"
%token <ident> T_ENDDECLARE    "'enddeclare'"
%token <ident> T_AS            "'as'"
%token <ident> T_SWITCH        "'switch'"
%token <ident> T_ENDSWITCH     "'endswitch'"
%token <ident> T_CASE          "'case'"
%token <ident> T_DEFAULT       "'default'"
%token <ident> T_MATCH         "'match'"
%token <ident> T_BREAK         "'break'"
%token <ident> T_CONTINUE      "'continue'"
%token <ident> T_GOTO          "'goto'"
%token <ident> T_FUNCTION      "'function'"

// clear, 可以用这种方法声名一个一行代码的函数，
	// 而且它不需要use就可以引用上下文中的变量，真是太变态了！
	$a = 1;$b = fn() => $a;var_dump($b());
	
%token <ident> T_FN            "'fn'"

%token <ident> T_CONST         "'const'"
%token <ident> T_RETURN        "'return'"
%token <ident> T_TRY           "'try'"
%token <ident> T_CATCH         "'catch'"
%token <ident> T_FINALLY       "'finally'"
%token <ident> T_THROW         "'throw'"
%token <ident> T_USE           "'use'"
%token <ident> T_INSTEADOF     "'insteadof'"
%token <ident> T_GLOBAL        "'global'"
%token <ident> T_STATIC        "'static'"
%token <ident> T_ABSTRACT      "'abstract'"
%token <ident> T_FINAL         "'final'"
%token <ident> T_PRIVATE       "'private'"
%token <ident> T_PROTECTED     "'protected'"
%token <ident> T_PUBLIC        "'public'"
%token <ident> T_READONLY      "'readonly'"
%token <ident> T_VAR           "'var'"
%token <ident> T_UNSET         "'unset'"
%token <ident> T_ISSET         "'isset'"
%token <ident> T_EMPTY         "'empty'"

// clear, __halt_compiler(); 只有这一种调用方法，脚本只编译到这里，后面不编译了，
	// 所以后面即使有语法错误也不影响运行
	// 这东东只能在最外层用，一个 {} 也不能加，否则
	// Fatal error: __HALT_COMPILER() can only be used from the outermost scope
	// 也不能带if条件，否则报错
	// Parse error: syntax error, unexpected token "__halt_compiler"
%token <ident> T_HALT_COMPILER "'__halt_compiler'"
%token <ident> T_CLASS         "'class'"
%token <ident> T_TRAIT         "'trait'"
%token <ident> T_INTERFACE     "'interface'"
%token <ident> T_ENUM          "'enum'"
%token <ident> T_EXTENDS       "'extends'"
%token <ident> T_IMPLEMENTS    "'implements'"
%token <ident> T_NAMESPACE     "'namespace'"
%token <ident> T_LIST            "'list'"
%token <ident> T_ARRAY           "'array'"

// clear, 这个是用来规定参数类型的
	function enumCharTypes(callable $callback): void {}
%token <ident> T_CALLABLE        "'callable'"

// clear, 魔术常量
%token <ident> T_LINE            "'__LINE__'"
%token <ident> T_FILE            "'__FILE__'"
%token <ident> T_DIR             "'__DIR__'"
%token <ident> T_CLASS_C         "'__CLASS__'"
%token <ident> T_TRAIT_C         "'__TRAIT__'"
%token <ident> T_METHOD_C        "'__METHOD__'"
%token <ident> T_FUNC_C          "'__FUNCTION__'"
// clear, 测试过
%token <ident> T_NS_C            "'__NAMESPACE__'"

// clear, 整个脚本最后的结束位置
%token END 0 "end of file"
// clear, 属性修饰符, 1处引用
%token T_ATTRIBUTE    "'#['" 
	
// ing3 一元运算符，注意：只能用在变量上，不能用在常量上！
%token T_PLUS_EQUAL   "'+='"
%token T_MINUS_EQUAL  "'-='"
%token T_MUL_EQUAL    "'*='"
%token T_DIV_EQUAL    "'/='"
%token T_CONCAT_EQUAL "'.='"
%token T_MOD_EQUAL    "'%='"
%token T_AND_EQUAL    "'&='"
%token T_OR_EQUAL     "'|='"
%token T_XOR_EQUAL    "'^='"
%token T_SL_EQUAL     "'<<='"
%token T_SR_EQUAL     "'>>='"

// clear, ??= , 当变量不存在或者值为null的时候赋值，其他时候不赋值。
	$c = 1; $c=null; $c ??= 2;
%token T_COALESCE_EQUAL "'??='"

%token T_BOOLEAN_OR   "'||'"
%token T_BOOLEAN_AND  "'&&'"
%token T_IS_EQUAL     "'=='"
%token T_IS_NOT_EQUAL "'!='"
%token T_IS_IDENTICAL "'==='"
%token T_IS_NOT_IDENTICAL "'!=='"
%token T_IS_SMALLER_OR_EQUAL "'<='"
%token T_IS_GREATER_OR_EQUAL "'>='"
// clear, 比大小 ，大于返回1，等于返回0，小于返回-1
%token T_SPACESHIP "'<=>'"
%token T_SL "'<<'"
%token T_SR "'>>'"
%token T_INC "'++'"
%token T_DEC "'--'"
%token T_INT_CAST    "'(int)'"
%token T_DOUBLE_CAST "'(double)'"
%token T_STRING_CAST "'(string)'"
%token T_ARRAY_CAST  "'(array)'"
%token T_OBJECT_CAST "'(object)'"
%token T_BOOL_CAST   "'(bool)'"
// clear, 弃用的操作，Fatal error: The (unset) cast is no longer supported
%token T_UNSET_CAST  "'(unset)'"

%token T_OBJECT_OPERATOR "'->'"
// clear, 对像是null的时候用 ?-> 调用不会报错
%token T_NULLSAFE_OBJECT_OPERATOR "'?->'"
%token T_DOUBLE_ARROW    "'=>'"

// clear, 普通注释 // 或 # 或 /* */ 里面包的
%token T_COMMENT         "comment"
// clear, doc注释 /** */ 里面包的
%token T_DOC_COMMENT     "doc comment"
// clear, php 开始标签 <?php 或其他
%token T_OPEN_TAG        "open tag"
// clear, 脚本开始标签可以自己定义，但快速打印标签只能是这个~
%token T_OPEN_TAG_WITH_ECHO "'<?='"
%token T_CLOSE_TAG       "'?>'"
// 匹配 换行，空格，tab等空白的东西
%token T_WHITESPACE      "whitespace"
// clear, 大字符串开始
%token T_START_HEREDOC   "heredoc start"
// clear, 大字符串结束
%token T_END_HEREDOC     "heredoc end"
// clear, 带大括号的变量名，也是普通变量名
	// 这这主要是在引号里面用的
%token T_DOLLAR_OPEN_CURLY_BRACES "'${'"
// clear, 字串中引用变量的写法
%token T_CURLY_OPEN      "'{$'"

// ing3, 介是什么破名字？
%token T_PAAMAYIM_NEKUDOTAYIM "'::'"
%token T_NS_SEPARATOR    "'\\'"
// clear, 可包含任意多个参数
%token T_ELLIPSIS        "'...'"

// clear, 当变量不存在或者值为null的时候取后面的值
	$c = $c ?? 100;
%token T_COALESCE        "'??'"

// clear, 幂运算：这个不是一元操作符！
%token T_POW             "'**'"
// clear, 一元运算符，求幂
%token T_POW_EQUAL       "'**='"
/* We need to split the & token in two to avoid a shift/reduce conflict. For T1&$v and T1&T2,
 * with only one token lookahead, bison does not know whether to reduce T1 as a complete type,
 * or shift to continue parsing an intersection type. */
%token T_AMPERSAND_FOLLOWED_BY_VAR_OR_VARARG     "'&'"
/* Bison warns on duplicate token literals, so use a different dummy value here.
 * It will be fixed up by zend_yytnamerr() later. */
// ing1， php测试结果 T_STRING -> amp , amp 根本不会被识别成token
%token T_AMPERSAND_NOT_FOLLOWED_BY_VAR_OR_VARARG "amp"

// ing1, 在解析时用的，parse 阶段用不到
%token T_BAD_CHARACTER   "invalid character"

// 这里上面的常量都是php脚本可以访问的！！

/* Token used to force a parse error from the lexer */
// ing1, 在解析时用的，parse 阶段用不到
%token T_ERROR

// ing3, php 语句，每个语句都返回ast 对象, 下面有每个语句的定义
	// 这个同时也表明了解析顺序吗？
%type <ast> top_statement namespace_name name statement function_declaration_statement
%type <ast> class_declaration_statement trait_declaration_statement legacy_namespace_name
%type <ast> interface_declaration_statement interface_extends_list
%type <ast> group_use_declaration inline_use_declarations inline_use_declaration
%type <ast> mixed_group_use_declaration use_declaration unprefixed_use_declaration
%type <ast> unprefixed_use_declarations const_decl inner_statement
%type <ast> expr optional_expr while_statement for_statement foreach_variable
%type <ast> foreach_statement declare_statement finally_statement unset_variable variable
%type <ast> extends_from parameter optional_type_without_static argument global_var
%type <ast> static_var class_statement trait_adaptation trait_precedence trait_alias
%type <ast> absolute_trait_method_reference trait_method_reference property echo_expr
%type <ast> new_expr anonymous_class class_name class_name_reference simple_variable
%type <ast> internal_functions_in_yacc
%type <ast> exit_expr scalar backticks_expr lexical_var function_call member_name property_name
%type <ast> variable_class_name dereferenceable_scalar constant class_constant
%type <ast> fully_dereferenceable array_object_dereferenceable
%type <ast> callable_expr callable_variable static_member new_variable
%type <ast> encaps_var encaps_var_offset isset_variables
%type <ast> top_statement_list use_declarations const_list inner_statement_list if_stmt
%type <ast> alt_if_stmt for_exprs switch_case_list global_var_list static_var_list
%type <ast> echo_expr_list unset_variables catch_name_list catch_list optional_variable parameter_list class_statement_list
%type <ast> implements_list case_list if_stmt_without_else
%type <ast> non_empty_parameter_list argument_list non_empty_argument_list property_list
%type <ast> class_const_list class_const_decl class_name_list trait_adaptations method_body non_empty_for_exprs
%type <ast> ctor_arguments alt_if_stmt_without_else trait_adaptation_list lexical_vars
%type <ast> lexical_var_list encaps_list
%type <ast> array_pair non_empty_array_pair_list array_pair_list possible_array_pair
%type <ast> isset_variable type return_type type_expr type_without_static
%type <ast> identifier type_expr_without_static union_type_without_static_element union_type_without_static intersection_type_without_static
%type <ast> inline_function union_type_element union_type intersection_type
%type <ast> attributed_statement attributed_class_statement attributed_parameter
%type <ast> attribute_decl attribute attributes attribute_group namespace_declaration_name
%type <ast> match match_arm_list non_empty_match_arm_list match_arm match_arm_cond_list
%type <ast> enum_declaration_statement enum_backing_type enum_case enum_case_expr
%type <ast> function_name

// ? 返回number的语句?
%type <num> returns_ref function fn is_reference is_variadic variable_modifiers
%type <num> method_modifiers non_empty_member_modifiers member_modifier
%type <num> optional_property_modifiers property_modifier
%type <num> class_modifiers class_modifier use_type backup_fn_flags

// ? ptr ?
%type <ptr> backup_lex_pos

// ?
%type <str> backup_doc_comment

// 关键字，包括保留和半保留, ident是什么
%type <ident> reserved_non_modifiers semi_reserved


// 下面是规则，那上面是规则名咯
	// 思路是先把关键词分类，然后用关键词和类目一起组成语句
	// 也可以用语句组成语句，或者递归组成语句
	// 然后应用语句规则去解析脚本
// $$ 因该是匹配的全文咯，类似正则的$0,然后$1,$2,$3,$4就是匹配到的结果
	// 而且，每个匹配结果本身都是一个ast对象！
// %empty 应该是指空的代码块，如 {%empty}	
	// 常用来创建列表
	
%% /* Rules */

// ?这个是自动的?，无引用
	// CG(ast) 是在这里赋值的，它相当于dom树根节点咯~
	// 所以这个start:应该是自动的，从它开始执行	
start:
	top_statement_list	{ CG(ast) = $1; (void) zendnerrs; }
;

// clear, php 保留字
	// 验证过：它和半保留字的区别是，它可以做 trait 引用时的别名，半保留字不可以
reserved_non_modifiers:
	  T_INCLUDE | T_INCLUDE_ONCE | T_EVAL | T_REQUIRE | T_REQUIRE_ONCE | T_LOGICAL_OR | T_LOGICAL_XOR | T_LOGICAL_AND
	| T_INSTANCEOF | T_NEW | T_CLONE | T_EXIT | T_IF | T_ELSEIF | T_ELSE | T_ENDIF | T_ECHO | T_DO | T_WHILE | T_ENDWHILE
	| T_FOR | T_ENDFOR | T_FOREACH | T_ENDFOREACH | T_DECLARE | T_ENDDECLARE | T_AS | T_TRY | T_CATCH | T_FINALLY
	| T_THROW | T_USE | T_INSTEADOF | T_GLOBAL | T_VAR | T_UNSET | T_ISSET | T_EMPTY | T_CONTINUE | T_GOTO
	| T_FUNCTION | T_CONST | T_RETURN | T_PRINT | T_YIELD | T_LIST | T_SWITCH | T_ENDSWITCH | T_CASE | T_DEFAULT | T_BREAK
	| T_ARRAY | T_CALLABLE | T_EXTENDS | T_IMPLEMENTS | T_NAMESPACE | T_TRAIT | T_INTERFACE | T_CLASS
	| T_CLASS_C | T_TRAIT_C | T_FUNC_C | T_METHOD_C | T_LINE | T_FILE | T_DIR | T_NS_C | T_FN | T_MATCH | T_ENUM
;

// clear, 半保留字，包含保留字，有引用
semi_reserved:
	  reserved_non_modifiers
	| T_STATIC | T_ABSTRACT | T_FINAL | T_PRIVATE | T_PROTECTED | T_PUBLIC | T_READONLY
;

// ing3,  /'æmpəsænd/，& 符号 或 amp， amp 这个全写是怎么用的？
ampersand:
		T_AMPERSAND_FOLLOWED_BY_VAR_OR_VARARG
	|	T_AMPERSAND_NOT_FOLLOWED_BY_VAR_OR_VARARG
;

// 看起来 $$ 是变量也是返回值
// clear, 标识符, 保留字和半保留字, 除了带 \ 的命名空间，什么都包括了
identifier:
	// 不在号中的字符
		T_STRING { $$ = $1; }
	// 半保留字，包含保留字
	| 	semi_reserved  {
			zval zv;
			if (zend_lex_tstring(&zv, $1) == FAILURE) { YYABORT; }
			$$ = zend_ast_create_zval(&zv);
		}
;

// clear, 顶层语句列表，顶层是指最外层或 namespace{}里面， 任意多个 top_statement，start引用
top_statement_list:
		top_statement_list top_statement { $$ = zend_ast_list_add($1, $2); }
	|	%empty { $$ = zend_ast_create_list(0, ZEND_AST_STMT_LIST); }
;

// clear, 命名空间定义，只在声名时引用
	// 不包含 T_NAME_FULLY_QUALIFIED ，声名它会报语法错误（测试过）
/* Name usable in a namespace declaration. */
namespace_declaration_name:
	// 非变量的名称，保留字，半保留字
		identifier								{ $$ = $1; }
	// 前面不能加\ ，但命名空间在声名的时候是绝对地址
	|	T_NAME_QUALIFIED						{ $$ = $1; }
;

// clear, 将要被引用的命名空间，这里不能包含保留字？
	// 保留字可以声名成命名空间，但不能被use ，只能用绝对路径引用 
	<?php use public;
	Parse error: syntax error, unexpected token "public"
	<?php public();
	Parse error: syntax error, unexpected token "public"
	<?php \public();
	Fatal error: Uncaught Error: Call to undefined function public()
	
// clear, 用于声名的命名空间，不包含 namespace 开头和 \ 开头
/* Name usable in "use" declarations (leading separator forbidden). */
namespace_name:
	// 不带\的字符
		T_STRING								{ $$ = $1; }
	// 相对路径
	|	T_NAME_QUALIFIED						{ $$ = $1; }
;

// clear, 用于use的命名空间，可以\开头，不包含 namespace开头
	// 相关规则不是很多
/* Name usable in "use" declarations (leading separator allowed). */
legacy_namespace_name:
		namespace_name							{ $$ = $1; }
	|	T_NAME_FULLY_QUALIFIED					{ $$ = $1; }
;

// clear, 非变量的名称， 不包含保留字，包含命名空间
name:
	// 不在引号内的字串
		T_STRING									{ $$ = $1; $$->attr = ZEND_NAME_NOT_FQ; }
	// 相对路径引用命名空间中的成员
	|	T_NAME_QUALIFIED							{ $$ = $1; $$->attr = ZEND_NAME_NOT_FQ; }
	// 绝对路径引用命名空间中的成员
	|	T_NAME_FULLY_QUALIFIED						{ $$ = $1; $$->attr = ZEND_NAME_FQ; }
	// namespace为第一节的命名空间名字
	|	T_NAME_RELATIVE								{ $$ = $1; $$->attr = ZEND_NAME_RELATIVE; }
;

// clear, 修饰属性的定义, 修饰属性必须是类名，（或满足类名规则的修饰符？）
attribute_decl:
		class_name
			{ $$ = zend_ast_create(ZEND_AST_ATTRIBUTE, $1, NULL); }
	|	class_name argument_list
			{ $$ = zend_ast_create(ZEND_AST_ATTRIBUTE, $1, $2); }
;

// clear, 多个修饰属性
attribute_group:
		attribute_decl
			{ $$ = zend_ast_create_list(1, ZEND_AST_ATTRIBUTE_GROUP, $1); }
	|	attribute_group ',' attribute_decl
			{ $$ = zend_ast_list_add($1, $3); }
;

// ing3, 完整的修饰属性（属性修饰符） #[, 末尾可以多个,
	// 这东西是干什么的需要单独整理，不只是类型那么简单！
attribute:
		T_ATTRIBUTE attribute_group possible_comma ']'	{ $$ = $2; }
;

// clear, 多个修饰属性( 好几个地方引用 ) 测试过可以加多个
	// 后面可以跟 
	//	attributed_statement: function , class, trait, interface, enum
	//	parameter,
	//	attributed_class_statement,
	//	anonymous_class,
	//	inline_function: function(), fn() => 
		// ?
	//	T_STATIC inline_function: static function(), fn() => 
attributes:
		attribute				{ $$ = zend_ast_create_list(1, ZEND_AST_ATTRIBUTE_LIST, $1); }
	|	attributes attribute	{ $$ = zend_ast_list_add($1, $2); }
;

// clear, 属性语句，只有 top_statement ，inner_statement 两个地方引用
	// 定义 function , class, trait, interface, enum , 这些前面可以加 attributes
attributed_statement:
		function_declaration_statement		{ $$ = $1; }
	|	class_declaration_statement			{ $$ = $1; }
	|	trait_declaration_statement			{ $$ = $1; }
	|	interface_declaration_statement		{ $$ = $1; }
	|	enum_declaration_statement			{ $$ = $1; }
;

// clear, 可以放在最外层的语句，只有 top_statement_list 引用
	// 除了普通 statement 主要是添加 namespace 和 use 语句！ 
top_statement:
// clear
		statement							{ $$ = $1; }
// clear，无修饰属性的 function , class, trait, interface, enum
	|	attributed_statement					{ $$ = $1; }
// clear，有修饰属性的 function , class, trait, interface, enum
	|	attributes attributed_statement		{ $$ = zend_ast_with_attributes($2, $1); }
// clear， __halt_compiler
	|	T_HALT_COMPILER '(' ')' ';'
			{ $$ = zend_ast_create(ZEND_AST_HALT_COMPILER,
			      zend_ast_create_zval_from_long(zend_get_scanned_file_offset()));
			  zend_stop_lexing(); }
// clear, 定义当前文档的命名空间 ; 结尾。经测试，这样就不能声名多个命名空间了
	//  { RESET_DOC_COMMENT(); } 大概是内置方法，开启新文档的意思 ？
	|	T_NAMESPACE namespace_declaration_name ';'
			{ $$ = zend_ast_create(ZEND_AST_NAMESPACE, $2, NULL);
			  RESET_DOC_COMMENT(); }
// clear, 定义有名字的命名空间，加代码块。棒！
	// 经测试，有代码块时，可以声名多个命名空间，但命名空间外面不能有代码了！
	// 运行报错 Fatal error: No code may exist outside of namespace
	|	T_NAMESPACE namespace_declaration_name { RESET_DOC_COMMENT(); }
		'{' top_statement_list '}'
			{ $$ = zend_ast_create(ZEND_AST_NAMESPACE, $2, $5); }
// clear, 定义无名字的命名空间，加代码块。
	|	T_NAMESPACE { RESET_DOC_COMMENT(); }
		'{' top_statement_list '}'
			{ $$ = zend_ast_create(ZEND_AST_NAMESPACE, NULL, $4); }
// clear, 批量混合引用 子命名空间 或 function 或 const
	|	T_USE mixed_group_use_declaration ';'		{ $$ = $2; }
// clear, 批量引用子 function 或 const，二选一
	|	T_USE use_type group_use_declaration ';'	{ $$ = $3; $$->attr = $2; }
// clear, use 引用命名空间
	|	T_USE use_declarations ';'					{ $$ = $2; $$->attr = ZEND_SYMBOL_CLASS; }
// clear, use function|const + 命名空间。这里只能加一个 use_type，不能多余逗号 测试过
	// 这个地方不能用只有一段的地址，否则会报错: use function a;
	// Warning: The use statement with non-compound name 'a' has no effect
	|	T_USE use_type use_declarations ';'			{ $$ = $3; $$->attr = $2; }
// clear const 声明一个或多个常量
	|	T_CONST const_list ';'						{ $$ = $2; }
;

// clear, function | const ,用于指定被 use 引用的元素
use_type:
	 	T_FUNCTION 		{ $$ = ZEND_SYMBOL_FUNCTION; }
	| 	T_CONST 		{ $$ = ZEND_SYMBOL_CONST; }
;

// clear, 批量引用 function 或 const，这个规则前面必须加 use_type 
	// 不存在的也可以引用，运行时不调用就不报错
group_use_declaration:
		legacy_namespace_name T_NS_SEPARATOR '{' unprefixed_use_declarations possible_comma '}'
			{ $$ = zend_ast_create(ZEND_AST_GROUP_USE, $1, $4); }
;

// clear, 混合引用一堆子命名空间 或 下属 function|const
	// 不存在的也可以引用，运行时不调用就不报错
mixed_group_use_declaration:
		legacy_namespace_name T_NS_SEPARATOR '{' inline_use_declarations possible_comma '}'
			{ $$ = zend_ast_create(ZEND_AST_GROUP_USE, $1, $4);}
;

// clear, 可选逗号
possible_comma:
		%empty
	|	','
;

// clear, 多个将要被use的子命名空间，,分隔， 可增加 as 别名 和 function|const 前缀
	// 只有 mixed_group_use_declaration 引用
inline_use_declarations:
		inline_use_declarations ',' inline_use_declaration
			{ $$ = zend_ast_list_add($1, $3); }
	|	inline_use_declaration
			{ $$ = zend_ast_create_list(1, ZEND_AST_USE, $1); }
;

// clear, 多个将要被use的子命名空间，,分隔， 可增加 as 别名 
	// 只有 group_use_declaration 引用
unprefixed_use_declarations:
		unprefixed_use_declarations ',' unprefixed_use_declaration
			{ $$ = zend_ast_list_add($1, $3); }
	|	unprefixed_use_declaration
			{ $$ = zend_ast_create_list(1, ZEND_AST_USE, $1); }
;

// clear, 一个或多个可带别名的完整有效命名空间, 只直接在use语句里用
use_declarations:
		use_declarations ',' use_declaration
			{ $$ = zend_ast_list_add($1, $3); }
	|	use_declaration
			{ $$ = zend_ast_create_list(1, ZEND_AST_USE, $1); }
;

// clear, 将要被use的子命名空间， 可增加 as 别名 和 function|const 前缀
inline_use_declaration:
		unprefixed_use_declaration { $$ = $1; $$->attr = ZEND_SYMBOL_CLASS; }
	|	use_type unprefixed_use_declaration { $$ = $2; $$->attr = $1; }
;

// clear, 将要被use的子命名空间（在某命名空间下）， 可增加 as 别名
unprefixed_use_declaration:
		namespace_name
			{ $$ = zend_ast_create(ZEND_AST_USE_ELEM, $1, NULL); }
	|	namespace_name T_AS T_STRING
			{ $$ = zend_ast_create(ZEND_AST_USE_ELEM, $1, $3); }
;
// clear, 可带别名的完整有效命名空间，只有 use_declarations 引用，, 只在use语句里用
use_declaration:
		legacy_namespace_name
			{ $$ = zend_ast_create(ZEND_AST_USE_ELEM, $1, NULL); }
// clear,  可选 + as 别名，别名不能带\ 不能是保留字 (测试过)
	|	legacy_namespace_name T_AS T_STRING
			{ $$ = zend_ast_create(ZEND_AST_USE_ELEM, $1, $3); }
;

// clear, 赋值一个或多个常量内容，用在 declare() 里或者 const 语句
const_list:
		const_list ',' const_decl { $$ = zend_ast_list_add($1, $3); }	
	|	const_decl { $$ = zend_ast_create_list(1, ZEND_AST_CONST_DECL, $1); }
;

// clear,  普通语句或，{代码块}， function, class, trait, interface, enum 的组合
	// 大量引用，循环引用，inner_statement_list -> inner_statement ->  statement -> inner_statement_list
	// 存在很多类似的递归嵌套，因为脚本语句本身就是这样嵌套的
inner_statement_list:
		inner_statement_list inner_statement
			{ $$ = zend_ast_list_add($1, $2); }
	|	%empty
			{ $$ = zend_ast_create_list(0, ZEND_AST_STMT_LIST); }
;

// clear, 普通语句或，{代码块}， function, class, trait, interface, enum
	// 只有 inner_statement_list 引用
inner_statement:
// clear, 普通语句或{}代码块！
		statement { $$ = $1; }
// clear, 可加修饰属性的 attributed_statement: function , class, trait, interface, enum
	|	attributed_statement					{ $$ = $1; }
	|	attributes attributed_statement		{ $$ = zend_ast_with_attributes($2, $1); }
// ing4，可能这里是多余的, __halt_compiler 只能用在最外层（参看了测试代码）
// 报错 Fatal error: __HALT_COMPILER() can only be used from the outermost scope
	|	T_HALT_COMPILER '(' ')' ';'
			{ $$ = NULL; zend_throw_exception(zend_ce_compile_error,
			      "__HALT_COMPILER() can only be used from the outermost scope", 0); YYERROR; }
;

// clear，程序语句，最后的;都是这里规定的！！
	// 带 :T_*END（for|while|foreach|if|switch|declare） 语句共6种
statement:
// clear, 带 {} 的代码块也算语句！
		'{' inner_statement_list '}' { $$ = $2; }
// clear, if 语句, 两种写法，{}和: endwhile
	|	if_stmt { $$ = $1; }
	|	alt_if_stmt { $$ = $1; }
// clear,while 语句 ，两种写法，{}和: endwhile
	|	T_WHILE '(' expr ')' while_statement
			{ $$ = zend_ast_create(ZEND_AST_WHILE, $3, $5); }
// clear, do-while 语句
	|	T_DO statement T_WHILE '(' expr ')' ';'
			{ $$ = zend_ast_create(ZEND_AST_DO_WHILE, $2, $5); }
// clear, for 循环，两种写法，{}和: endfor
	|	T_FOR '(' for_exprs ';' for_exprs ';' for_exprs ')' for_statement
			{ $$ = zend_ast_create(ZEND_AST_FOR, $3, $5, $7, $9); }
// clear, switch 语句，两种写法，{}和: endswitch
	|	T_SWITCH '(' expr ')' switch_case_list
			{ $$ = zend_ast_create(ZEND_AST_SWITCH, $3, $5); }
// clear, break, continue,return 后面可选加表达式
	|	T_BREAK optional_expr ';'		{ $$ = zend_ast_create(ZEND_AST_BREAK, $2); }
	|	T_CONTINUE optional_expr ';'	{ $$ = zend_ast_create(ZEND_AST_CONTINUE, $2); }
	|	T_RETURN optional_expr ';'		{ $$ = zend_ast_create(ZEND_AST_RETURN, $2); }
// clear,global 语句 , 声明 global 时不可以赋值！ global $a=1; 是不行的
	|	T_GLOBAL global_var_list ';'	{ $$ = $2; }
// clear, static 开头，声名静态变量
	|	T_STATIC static_var_list ';'	{ $$ = $2; }
// clear, echo 语句
	|	T_ECHO echo_expr_list ';'		{ $$ = $2; }
// clear, 不在 <?php ?> 标签内的内容
	|	T_INLINE_HTML { $$ = zend_ast_create(ZEND_AST_ECHO, $1); }
// clear, 表达式最后面的 ;
	|	expr ';' { $$ = $1; }
// clear
	|	T_UNSET '(' unset_variables possible_comma ')' ';' { $$ = $3; }
// clear, foreach 语句 （带 =>） , 两种写法，{}和: endforeach
	|	T_FOREACH '(' expr T_AS foreach_variable ')' foreach_statement
			{ $$ = zend_ast_create(ZEND_AST_FOREACH, $3, $5, NULL, $7); }
	|	T_FOREACH '(' expr T_AS foreach_variable T_DOUBLE_ARROW foreach_variable ')'
		foreach_statement
			{ $$ = zend_ast_create(ZEND_AST_FOREACH, $3, $7, $5, $9); }
// clear, 定义内置常量，如 declare (strict_types = 1); 
	|	T_DECLARE '(' const_list ')'
			{ if (!zend_handle_encoding_declaration($3)) { YYERROR; } }
	// 后面加代码块，示例里主要用来声名字符集, 两种写法，{}和: enddeclare
	// declare(encoding="UTF-8") { echo "ok\n"; }
		declare_statement
			{ $$ = zend_ast_create(ZEND_AST_DECLARE, $3, $6); }
// clear, 空语句
	|	';'	/* empty statement */ { $$ = NULL; }
// clear, try catch final 语句
	|	T_TRY '{' inner_statement_list '}' catch_list finally_statement
			{ $$ = zend_ast_create(ZEND_AST_TRY, $3, $5, $6); }
// clear, goto 语句 跳转点
	|	T_GOTO T_STRING ';' { $$ = zend_ast_create(ZEND_AST_GOTO, $2); }
// clear, 声名跳转点
	|	T_STRING ':' { $$ = zend_ast_create(ZEND_AST_LABEL, $1); }
;

// clear, catch 语句， 一个try后面可以跟任意多个catch	
catch_list:
// 一个catch也没有
		%empty
			{ $$ = zend_ast_create_list(0, ZEND_AST_CATCH_LIST); }
// 一个或多catch ， catch() 里面类名至少一个，
	// 变量名可以不写，写的话在所有类名之后
	|	catch_list T_CATCH '(' catch_name_list optional_variable ')' '{' inner_statement_list '}'
			{ $$ = zend_ast_list_add($1, zend_ast_create(ZEND_AST_CATCH, $4, $5, $8)); }
;

// clear, catch 类名, 只在catch 语句中用到，可以是一个类名，
	// 或用|分隔的多个类名
catch_name_list:
		class_name { $$ = zend_ast_create_list(1, ZEND_AST_NAME_LIST, $1); }
	|	catch_name_list '|' class_name { $$ = zend_ast_list_add($1, $3); }
;

// clear, 可写可不写的变量, 只在catch_list 中用到
optional_variable:
		%empty { $$ = NULL; }
	|	T_VARIABLE { $$ = $1; }
;

// clear, 可选的 finally 语句, 只在try中引用
finally_statement:
		%empty { $$ = NULL; }
	|	T_FINALLY '{' inner_statement_list '}' { $$ = $3; }
;

// clear, unset 函数
unset_variables:
		unset_variable { $$ = zend_ast_create_list(1, ZEND_AST_STMT_LIST, $1); }
	|	unset_variables ',' unset_variable { $$ = zend_ast_list_add($1, $3); }
;

// clear,用于unset的变量， ast 对象与普通变量不同， kind: ZEND_AST_UNSET
unset_variable:
		variable { $$ = zend_ast_create(ZEND_AST_UNSET, $1); }
;

// clear, 函数名，保留字里只有 readonly 可以做函数名，
	// 只有 function_declaration_statement 引用
function_name:
		T_STRING { $$ = $1; }
	|	T_READONLY {
			zval zv;
			if (zend_lex_tstring(&zv, $1) == FAILURE) { YYABORT; }
			$$ = zend_ast_create_zval(&zv);
		}
;

// clear, 定义函数语句， backup_fn_flags 是空语法
function_declaration_statement:
	function returns_ref function_name backup_doc_comment '(' parameter_list ')' return_type
	backup_fn_flags '{' inner_statement_list '}' backup_fn_flags
		{ $$ = zend_ast_create_decl(ZEND_AST_FUNC_DECL, $2 | $13, $1, $4,
		      zend_ast_get_str($3), $6, NULL, $11, $8, NULL); CG(extra_fn_flags) = $9; }
;

// clear, 可选 &
is_reference:
		%empty	{ $$ = 0; }
	|	T_AMPERSAND_FOLLOWED_BY_VAR_OR_VARARG	{ $$ = ZEND_PARAM_REF; }
;

// clear, 可选 ...
is_variadic:
		%empty { $$ = 0; }
	|	T_ELLIPSIS  { $$ = ZEND_PARAM_VARIADIC; }
;

// clear, 声明类，可加修饰符
class_declaration_statement:
// 修饰符 + 类名 + 可选：extends  + 可选：implements + 实现逻辑 
		class_modifiers T_CLASS { $<num>$ = CG(zend_lineno); }
		T_STRING extends_from implements_list backup_doc_comment '{' class_statement_list '}'
			{ $$ = zend_ast_create_decl(ZEND_AST_CLASS, $1, $<num>3, $7, zend_ast_get_str($4), $5, $6, $9, NULL, NULL); }
//  类名 + 可选：extends  + 可选：implements + 实现逻辑
	|	T_CLASS { $<num>$ = CG(zend_lineno); }
		T_STRING extends_from implements_list backup_doc_comment '{' class_statement_list '}'
			{ $$ = zend_ast_create_decl(ZEND_AST_CLASS, 0, $<num>2, $6, zend_ast_get_str($3), $4, $5, $8, NULL, NULL); }
;

// clear, 一个或多个类修饰符，只有 class_declaration_statement 引用
class_modifiers:
		class_modifier 					{ $$ = $1; }
	|	class_modifiers class_modifier
			{ $$ = zend_add_class_modifier($1, $2); if (!$$) { YYERROR; } }
;

// clear, class 修饰符，abstract,final,readonly, 只有 class_modifiers 引用
class_modifier:
		T_ABSTRACT 		{ $$ = ZEND_ACC_EXPLICIT_ABSTRACT_CLASS; }
	|	T_FINAL 		{ $$ = ZEND_ACC_FINAL; }
	|	T_READONLY 		{ $$ = ZEND_ACC_READONLY_CLASS|ZEND_ACC_NO_DYNAMIC_PROPERTIES; }
;

// clear, trait 的定义比较简单 trait + 名字 + { 业务逻辑 }
trait_declaration_statement:
		T_TRAIT { $<num>$ = CG(zend_lineno); }
		T_STRING backup_doc_comment '{' class_statement_list '}'
			{ $$ = zend_ast_create_decl(ZEND_AST_CLASS, ZEND_ACC_TRAIT, $<num>2, $4, zend_ast_get_str($3), NULL, NULL, $6, NULL, NULL); }
;

// clear, 声明接口 interface + 名字 + extends + { 业务逻辑 }
interface_declaration_statement:
		T_INTERFACE { $<num>$ = CG(zend_lineno); }
		T_STRING interface_extends_list backup_doc_comment '{' class_statement_list '}'
			{ $$ = zend_ast_create_decl(ZEND_AST_CLASS, ZEND_ACC_INTERFACE, $<num>2, $5, zend_ast_get_str($3), NULL, $4, $7, NULL, NULL); }
;

// clear, 声明枚举类型 emnu + 名字 + ( :类型 ) + implements + { 业务逻辑 }
enum_declaration_statement:
		T_ENUM { $<num>$ = CG(zend_lineno); }
		T_STRING enum_backing_type implements_list backup_doc_comment '{' class_statement_list '}'
			{ $$ = zend_ast_create_decl(ZEND_AST_CLASS, ZEND_ACC_ENUM|ZEND_ACC_FINAL, $<num>2, $6, zend_ast_get_str($3), NULL, $5, $8, NULL, $4); }
;
// clear, 枚举类型后面的类型 :type
enum_backing_type:
		%empty	{ $$ = NULL; }
	|	':' type_expr { $$ = $2; }
;
// clear, enum中定义case,只有 attributed_class_statement 用到
enum_case:
		T_CASE backup_doc_comment identifier enum_case_expr ';'
			{ $$ = zend_ast_create(ZEND_AST_ENUM_CASE, $3, $4, ($2 ? zend_ast_create_zval_from_str($2) : NULL), NULL); }
;

// clear, enum的case可以不赋值,只有 enum_case 引用
enum_case_expr:
		%empty	{ $$ = NULL; }
	|	'=' expr { $$ = $2; }
;

// clear, 可选， extends 语句, 只有声明类时用到
extends_from:
		%empty				{ $$ = NULL; }
	|	T_EXTENDS class_name	{ $$ = $2; }
;

// clear, 可选，接口的继承，多重继承，只有声明接口用到
interface_extends_list:
		%empty			        { $$ = NULL; }
	|	T_EXTENDS class_name_list	{ $$ = $2; }
;

// clear, 可选，类声明中的实现接口语句，用于声明类和 enum !
implements_list:
		%empty		        		{ $$ = NULL; }
	|	T_IMPLEMENTS class_name_list	{ $$ = $2; }
;

// ing, foreach 语句 as 后面的部分， => 前后都是它
foreach_variable:
// => 前面key只能用
		variable			{ $$ = $1; }
	|	ampersand variable	{ $$ = zend_ast_create(ZEND_AST_REF, $2); }
	|	T_LIST '(' array_pair_list ')' { $$ = $3; $$->attr = ZEND_ARRAY_SYNTAX_LIST; }
	|	'[' array_pair_list ']' { $$ = $2; $$->attr = ZEND_ARRAY_SYNTAX_SHORT; }
;

// clear, for循环的业务逻辑部分，两种写法
for_statement:
		statement { $$ = $1; }
	|	':' inner_statement_list T_ENDFOR ';' { $$ = $2; }
;

// clear, foreach循环的业务逻辑部分，两种写法
foreach_statement:
		statement { $$ = $1; }
	|	':' inner_statement_list T_ENDFOREACH ';' { $$ = $2; }
;

// clear, declare的业务逻辑部分，两种写法
declare_statement:
		statement { $$ = $1; }
	|	':' inner_statement_list T_ENDDECLARE ';' { $$ = $2; }
;

// clear, switch的业务逻辑部分，两种写法
	// 最前面可以加个; （测试过）
switch_case_list:
		'{' case_list '}'					{ $$ = $2; }
	|	'{' ';' case_list '}'				{ $$ = $3; }
	|	':' case_list T_ENDSWITCH ';'		{ $$ = $2; }
	|	':' ';' case_list T_ENDSWITCH ';'	{ $$ = $3; }
;

// clear, 任意多个case语句
case_list:
// 留空
		%empty { $$ = zend_ast_create_list(0, ZEND_AST_SWITCH_LIST); }
// clear case 语句 ，可以写成 case 1; 或者case 1:;
	|	case_list T_CASE expr case_separator inner_statement_list
			{ $$ = zend_ast_list_add($1, zend_ast_create(ZEND_AST_SWITCH_CASE, $3, $5)); }
// clear default 语句  ，可以写成 default; 或者 default:;
	// default 在语法上可以有多个，运行时只能有一个
	// Fatal error: Switch statements may only contain one default clause
	|	case_list T_DEFAULT case_separator inner_statement_list
			{ $$ = zend_ast_list_add($1, zend_ast_create(ZEND_AST_SWITCH_CASE, NULL, $4)); }
;

// clear , 分隔符 : ;
case_separator:
		':'
	|	';'
;

// clear, match 语句
match:
		T_MATCH '(' expr ')' '{' match_arm_list '}'
			{ $$ = zend_ast_create(ZEND_AST_MATCH, $3, $6); };
;

// clear, 任意多个键值对，只有match 引用
match_arm_list:
		%empty { $$ = zend_ast_create_list(0, ZEND_AST_MATCH_ARM_LIST); }
	|	non_empty_match_arm_list possible_comma { $$ = $1; }
;

// clear, 一个或多个match键值对, 只有 match_arm_list 引用
non_empty_match_arm_list:
		match_arm { $$ = zend_ast_create_list(1, ZEND_AST_MATCH_ARM_LIST, $1); }
	|	non_empty_match_arm_list ',' match_arm { $$ = zend_ast_list_add($1, $3); }
;

// clear, match 键值对，键可以是 default， 只有 non_empty_match_arm_list 引用
	// 这里为什么要加个,呢？
match_arm:
		match_arm_cond_list possible_comma T_DOUBLE_ARROW expr
			{ $$ = zend_ast_create(ZEND_AST_MATCH_ARM, $1, $4); }
	|	T_DEFAULT possible_comma T_DOUBLE_ARROW expr
			{ $$ = zend_ast_create(ZEND_AST_MATCH_ARM, NULL, $4); }
;

// clear, arm 的条件列表 由一个或多个表达式组成， 只有 match_arm 引用
	// match_arm 是键值对 条件（一个或多个expr） => 值（expr）
match_arm_cond_list:
		expr { $$ = zend_ast_create_list(1, ZEND_AST_EXPR_LIST, $1); }
	|	match_arm_cond_list ',' expr { $$ = zend_ast_list_add($1, $3); }
;

// clear, while 语法的业务逻辑部分，两种写法，{}和:
while_statement:
		statement { $$ = $1; }
	|	':' inner_statement_list T_ENDWHILE ';' { $$ = $2; }
;

// clear, if 语句，加或不加else，只有 if_stmt 引用，引用后才是完整的
if_stmt_without_else:
// clear if(条件) + 语句(块)
		T_IF '(' expr ')' statement
			{ $$ = zend_ast_create_list(1, ZEND_AST_IF,
			      zend_ast_create(ZEND_AST_IF_ELEM, $3, $5)); }
// clear 后面添加任意多个 elseif(条件) + 语句(块)
	|	if_stmt_without_else T_ELSEIF '(' expr ')' statement
			{ $$ = zend_ast_list_add($1,
			      zend_ast_create(ZEND_AST_IF_ELEM, $4, $6)); }
;

// clear, 完整的if语句, 独立 statement，有 else 和没有 else 都在这里定义了, 
	// %prec是什么
if_stmt:
// if + 任意多个 elseif
		if_stmt_without_else %prec T_NOELSE { $$ = $1; }
// 最后最多加一个 else + 代码块
	// else后面如果有elseif，报解析错误 Parse error: syntax error, unexpected token "elseif"
	|	if_stmt_without_else T_ELSE statement
			{ $$ = zend_ast_list_add($1, zend_ast_create(ZEND_AST_IF_ELEM, NULL, $3)); }
;

// clear, :格式的if 语句，无结尾
alt_if_stmt_without_else:
// if(expr) 语句开头 : 不是 {
		T_IF '(' expr ')' ':' inner_statement_list
			{ $$ = zend_ast_create_list(1, ZEND_AST_IF,
			      zend_ast_create(ZEND_AST_IF_ELEM, $3, $6)); }
// elseif(expr) :
	|	alt_if_stmt_without_else T_ELSEIF '(' expr ')' ':' inner_statement_list
			{ $$ = zend_ast_list_add($1,
			      zend_ast_create(ZEND_AST_IF_ELEM, $4, $7)); }
;

// clear, 完整的:格式的if语句（if语句 + 结尾），独立 statement
alt_if_stmt:
// if: + endif ;
		alt_if_stmt_without_else T_ENDIF ';' { $$ = $1; }
// if: + else: + endif ;
	|	alt_if_stmt_without_else T_ELSE ':' inner_statement_list T_ENDIF ';'
			{ $$ = zend_ast_list_add($1,
			      zend_ast_create(ZEND_AST_IF_ELEM, NULL, $4)); }
;

// clear, 任意多个形式参数，只在定义function 或 fn() 时用到
	// 这是形式参数列表，参数只能用基础变量
parameter_list:
		non_empty_parameter_list possible_comma { $$ = $1; }
	|	%empty	{ $$ = zend_ast_create_list(0, ZEND_AST_PARAM_LIST); }
;

// clear, 一个或多个形式参数，可加attributes ,只有 parameter_list 引用
non_empty_parameter_list:
		attributed_parameter
			{ $$ = zend_ast_create_list(1, ZEND_AST_PARAM_LIST, $1); }
	|	non_empty_parameter_list ',' attributed_parameter
			{ $$ = zend_ast_list_add($1, $3); }
;

// clear, 可加 attributes 的单个形式参数，只有 non_empty_parameter_list 引用
attributed_parameter:
		attributes parameter	{ $$ = zend_ast_with_attributes($2, $1); }
	|	parameter				{ $$ = $1; }
;

// clear, 任意个类成员修饰符
optional_property_modifiers:
		%empty					{ $$ = 0; }
	|	optional_property_modifiers property_modifier
			{ $$ = zend_add_member_modifier($1, $2); if (!$$) { YYERROR; } }

// clear, 类成员修饰符 ，只有 optional_property_modifiers 引用
	// readonly 可以和其他的一起用
property_modifier:
		T_PUBLIC				{ $$ = ZEND_ACC_PUBLIC; }
	|	T_PROTECTED				{ $$ = ZEND_ACC_PROTECTED; }
	|	T_PRIVATE				{ $$ = ZEND_ACC_PRIVATE; }
	|	T_READONLY				{ $$ = ZEND_ACC_READONLY; }
;

// clear, 单个形式参数，只有 attributed_parameter 引用，最终是在function()定义时用的
parameter:
// clear, 修饰符 + 可选，不带static的类型名 + 可选& + 可选... + 基础变量
		optional_property_modifiers optional_type_without_static
		is_reference is_variadic T_VARIABLE backup_doc_comment
			{ $$ = zend_ast_create_ex(ZEND_AST_PARAM, $1 | $3 | $4, $2, $5, NULL,
					NULL, $6 ? zend_ast_create_zval_from_str($6) : NULL); }
// clear, 修饰符 + 可选，不带static的类型名 + 可选& + 可选... + 基础变量 + = 表达式赋值
	// 测试过，传实参时可以用赋值表达式 print($a=1);
	|	optional_property_modifiers optional_type_without_static
		is_reference is_variadic T_VARIABLE backup_doc_comment '=' expr
			{ $$ = zend_ast_create_ex(ZEND_AST_PARAM, $1 | $3 | $4, $2, $5, $8,
					NULL, $6 ? zend_ast_create_zval_from_str($6) : NULL); }
;

// clear, 可选，不带static的参数类型
optional_type_without_static:
		%empty	{ $$ = NULL; }
	|	type_expr_without_static	{ $$ = $1; }
;

// clear, 指定类型（包含array, callable）
	// 只有 enum_backing_type 和 return_type 引用
type_expr:
		type				{ $$ = $1; }
// clear, 加 ? 后可以赋值成 null
	|	'?' type			{ $$ = $2; $$->attr |= ZEND_TYPE_NULLABLE; }
	|	union_type			{ $$ = $1; }
	|	intersection_type	{ $$ = $1; }
;

// clear, 类型或static，这里两个不能共用，实际上可以两个都用（经测试）
	// attributed_class_statement 引用了修饰符 T_STATIC,所以 T_STATIC 会被识别成 type 吗？
		
	// 所以这里加个static也是为了 ZEND_AST_TYPE咯
type:
		type_without_static	{ $$ = $1; }
	|	T_STATIC			{ $$ = zend_ast_create_ex(ZEND_AST_TYPE, IS_STATIC); }
;

// clear, 一个 type 或  加括号的 ( intersection_type ) ，只有 union_type 引用
	// intersection_type 和 union_type 在一起时，intersection_type 一定要加括号
union_type_element:
                type { $$ = $1; }
        |        '(' intersection_type ')' { $$ = $2; }
;

// clear , 多个type或(intersection_type)用 | 连接 （满足任何一个类型即可）,
	// 只有 type_expr 引用
union_type:
		union_type_element '|' union_type_element
			{ $$ = zend_ast_create_list(2, ZEND_AST_TYPE_UNION, $1, $3); }
	|	union_type '|' union_type_element
			{ $$ = zend_ast_list_add($1, $3); }
;

// clear , 多个type用&连接，最终是为了给 type_expr 引用
	// 同时从属多个类或接口，指一个类型实现了多个接口 或 型继承了其他类型
intersection_type:
		type T_AMPERSAND_NOT_FOLLOWED_BY_VAR_OR_VARARG type       { $$ = zend_ast_create_list(2, ZEND_AST_TYPE_INTERSECTION, $1, $3); }
	|	intersection_type T_AMPERSAND_NOT_FOLLOWED_BY_VAR_OR_VARARG type { $$ = zend_ast_list_add($1, $3); }
;

/* Duplicate the type rules without "static",
 * to avoid conflicts with "static" modifier for properties. */
// clear, 不带static的参数类型， 只有 optional_type_without_static 引用 
type_expr_without_static:
		type_without_static			{ $$ = $1; }
// clear, 加？的变量可以设置成 Null
	|	'?' type_without_static		{ $$ = $2; $$->attr |= ZEND_TYPE_NULLABLE; }
	|	union_type_without_static	{ $$ = $1; }
	|	intersection_type_without_static	{ $$ = $1; }
;

// clear, 变量类型或方法的返回类型，不加static，多处引用 
type_without_static:
		T_ARRAY		{ $$ = zend_ast_create_ex(ZEND_AST_TYPE, IS_ARRAY); }
	|	T_CALLABLE	{ $$ = zend_ast_create_ex(ZEND_AST_TYPE, IS_CALLABLE); }
	|	name		{ $$ = $1; }
;
// clear, 两个非static类型用 | 连接 
union_type_without_static_element:
                type_without_static { $$ = $1; }
        |        '(' intersection_type_without_static ')' { $$ = $2; }
;
// clear, 多个非static类型用 | 连接
union_type_without_static:
		union_type_without_static_element '|' union_type_without_static_element
			{ $$ = zend_ast_create_list(2, ZEND_AST_TYPE_UNION, $1, $3); }
	|	union_type_without_static '|' union_type_without_static_element
			{ $$ = zend_ast_list_add($1, $3); }
;
// clear， 多个非static类型用 & 连接 
intersection_type_without_static:
		type_without_static T_AMPERSAND_NOT_FOLLOWED_BY_VAR_OR_VARARG type_without_static
			{ $$ = zend_ast_create_list(2, ZEND_AST_TYPE_INTERSECTION, $1, $3); }
	|	intersection_type_without_static T_AMPERSAND_NOT_FOLLOWED_BY_VAR_OR_VARARG type_without_static
			{ $$ = zend_ast_list_add($1, $3); }
;

// clear, 声明函数或 方法返回类型, 空或type_expr 
	// 类里的function method():static {} 是可以声名返回类型为static的！
	// 普通函数也不报语法错误，但运行报错
return_type:
		%empty	{ $$ = NULL; }
	|	':' type_expr	{ $$ = $2; }
;

// clear, 带括号的参数表 (里面任意多个参数), 或者(...)
	// 果然，根据这个规则，括号里的参数最后可以加个, 不影响解析
	// 这里才加上括号的！
argument_list:
		'(' ')'	{ $$ = zend_ast_create_list(0, ZEND_AST_ARG_LIST); }
	|	'(' non_empty_argument_list possible_comma ')' { $$ = $2; }
// clear, (...)
	|	'(' T_ELLIPSIS ')' { $$ = zend_ast_create(ZEND_AST_CALLABLE_CONVERT); }
;

// clear , 不带括号的多个参数, 连接（这里最后还不能是,）
	// 两处引用
non_empty_argument_list:
// 
		argument
			{ $$ = zend_ast_create_list(1, ZEND_AST_ARG_LIST, $1); }
// clear, 多个参数递归调用 zend_ast_list_add 添加进参数表
	|	non_empty_argument_list ',' argument
			{ $$ = zend_ast_list_add($1, $3); }
;

// clear, 不带括号的单个参数。看起来声明和调用函数，解析参数列表走同一个逻辑
argument:
// clear, 表达式
		expr				{ $$ = $1; }
// clear, 保留字(public 之类的也可以用，不会报解析错，但运行报错）：表达式
	// Fatal error: Cannot declare promoted property outside a constructor
	// 带名字的参数 function a($a=1,$b=2){var_dump($a,$b);} a(b:1);
	|	identifier ':' expr
			{ $$ = zend_ast_create(ZEND_AST_NAMED_ARG, $1, $3); }
// clear, 用表达式回传多个值，可以把返回结果数组解开(ZEND_AST_UNPACK)
	// function a($a,$b,$c,$d,$e,$f){ var_dump($a,$b,$c,$d,$e,$f,$g);} a(1,2,...[1,2,3]);
	// Fatal error: Uncaught ArgumentCountError: Too few arguments to function a(), 5 passed in D:\www\docs\test.php on line 2 and exactly 6 expected
	// function a($a,$b,$c,$d,...$e){ var_dump($a,$b,$c,$d,$e,$f,$g);} a(...[1,2],...[1,2,3]);
	// 用 T_ELLIPSIS 可以用多次
	|	T_ELLIPSIS expr	{ $$ = zend_ast_create(ZEND_AST_UNPACK, $2); }
;

// clear, global 一次声名多个变量,只有在 global 语句用到
	// 声明 global 时不可以赋值！ global $a=1; 是不行的
global_var_list:
		global_var_list ',' global_var { $$ = zend_ast_list_add($1, $3); }
	|	global_var { $$ = zend_ast_create_list(1, ZEND_AST_STMT_LIST, $1); }
;

// clear, global 修饰的变量，可以是 simple_variable （测试过 global ${'a'}; )	
global_var:
	simple_variable
		{ $$ = zend_ast_create(ZEND_AST_GLOBAL, zend_ast_create(ZEND_AST_VAR, $1)); }
;

// clear, 一次声名多个static变量，只在声名 static 变量时用到
static_var_list:
		static_var_list ',' static_var { $$ = zend_ast_list_add($1, $3); }
	|	static_var { $$ = zend_ast_create_list(1, ZEND_AST_STMT_LIST, $1); }
;

// clear, static 修饰的变量只能是 基本变量，不能用 simple_variable, ${'a'}
static_var:
		T_VARIABLE			{ $$ = zend_ast_create(ZEND_AST_STATIC, $1, NULL); }
	|	T_VARIABLE '=' expr	{ $$ = zend_ast_create(ZEND_AST_STATIC, $1, $3); }
;

// clear, 任意多个 class_statement, 虽然叫 class_statement，但用途蛮多
	// 用于 1.类声明，2.匿名类声明，3.trait声明，4.接口声明，5.enum声明 的业务逻辑
class_statement_list:
		class_statement_list class_statement
			{ $$ = zend_ast_list_add($1, $2); }
	|	%empty
			{ $$ = zend_ast_create_list(0, ZEND_AST_STMT_LIST); }
;

// clear, 只有 class_statement 用到
attributed_class_statement:
// clear，可带修饰符、static的 类属性列表
		variable_modifiers optional_type_without_static property_list ';'
			{ $$ = zend_ast_create(ZEND_AST_PROP_GROUP, $2, $3, NULL);
			  $$->attr = $1; }
			  
// clear, 常量也是可以用修饰符的，测试过，用readonly会报错
	// Fatal error: Cannot use 'readonly' as constant modifier
	|	method_modifiers T_CONST class_const_list ';'
			{ $$ = zend_ast_create(ZEND_AST_CLASS_CONST_GROUP, $3, NULL);
			  $$->attr = $1; }
// clear, 定义类方法
	|	method_modifiers function returns_ref identifier backup_doc_comment '(' parameter_list ')'
		return_type backup_fn_flags method_body backup_fn_flags
			{ $$ = zend_ast_create_decl(ZEND_AST_METHOD, $3 | $1 | $12, $2, $5,
				  zend_ast_get_str($4), $7, NULL, $11, $9, NULL); CG(extra_fn_flags) = $10; }
// clear, enum 中定义 case 实际上只能用在enum中, 但用在类里没有语法错误(syntax error)
	// 只有一个编译错误 Fatal error: Case can only be used in enums
	|	enum_case { $$ = $1; }
;

// clear, 只有 class_statement_list 引用
class_statement:
// clear, attributed_class_statement 前面可以加或不加 attributes
		attributed_class_statement { $$ = $1; }
	|	attributes attributed_class_statement { $$ = zend_ast_with_attributes($2, $1); }
// clear, 在class 中引用 trait
	|	T_USE class_name_list trait_adaptations
			{ $$ = zend_ast_create(ZEND_AST_USE_TRAIT, $2, $3); }
;

// clear, 一个或多个类名
class_name_list:
		class_name { $$ = zend_ast_create_list(1, ZEND_AST_NAME_LIST, $1); }
	|	class_name_list ',' class_name { $$ = zend_ast_list_add($1, $3); }
;

// clear，引用trait时可以加或不加{trait_adaptation_list}, 只有 class_statement 引用
trait_adaptations:
		';'								{ $$ = NULL; }
	|	'{' '}'							{ $$ = NULL; }
	|	'{' trait_adaptation_list '}'	{ $$ = $2; }
;

// clear, 一个或多个 trait_adaptation 只有 trait_adaptations 引用
trait_adaptation_list:
		trait_adaptation
			{ $$ = zend_ast_create_list(1, ZEND_AST_TRAIT_ADAPTATIONS, $1); }
	|	trait_adaptation_list trait_adaptation
			{ $$ = zend_ast_list_add($1, $2); }
;

// clear 只有，trait_adaptation_list 引用
trait_adaptation:
// clear, 确定trait内方法的优先关系
		trait_precedence ';'	{ $$ = $1; }
// clear, 可以给trait 里的方法设置别名或更改访问权限
	|	trait_alias ';'			{ $$ = $1; }
;

// clear, 当引用多个trait时，用 insteadof 确定哪个方法优先
	// 只有 trait_adaptation 引用，T_INSTEADOF 只有这里用到
	// 示例：class MyClass { use C, A, B { B::foo insteadof A, C;   }}
trait_precedence:
	absolute_trait_method_reference T_INSTEADOF class_name_list
		{ $$ = zend_ast_create(ZEND_AST_TRAIT_PRECEDENCE, $1, $3); }
;

// clear,  引用trait时指定trait下的方法并创建别名，只有 trait_adaptation 引用
trait_alias:
// clear, 普通字串做别名
		trait_method_reference T_AS T_STRING
			{ $$ = zend_ast_create(ZEND_AST_TRAIT_ALIAS, $1, $3); }
// clear, 保留字和半保留字做别名（除了readonly，语法无报错，编译报错），测试过
	|	trait_method_reference T_AS reserved_non_modifiers
			{ zval zv;
			  if (zend_lex_tstring(&zv, $3) == FAILURE) { YYABORT; }
			  $$ = zend_ast_create(ZEND_AST_TRAIT_ALIAS, $1, zend_ast_create_zval(&zv)); }
// clear, 可以改变方法的访问权限 ，同时设置别名，测试过
	|	trait_method_reference T_AS member_modifier identifier
			{ $$ = zend_ast_create_ex(ZEND_AST_TRAIT_ALIAS, $3, $1, $4); }
// clear, 可以改变方法的访问权限 ，不设置别名，测试过
	|	trait_method_reference T_AS member_modifier
			{ $$ = zend_ast_create_ex(ZEND_AST_TRAIT_ALIAS, $3, $1, NULL); }
;

// clear, 引用trait 时，可以指定trait下的方法名，相对或绝对路径
trait_method_reference:
		identifier
			{ $$ = zend_ast_create(ZEND_AST_METHOD_REFERENCE, NULL, $1); }
	|	absolute_trait_method_reference { $$ = $1; }
;

// clear, 引用 trait 里面的方法，可以写绝对名称（名空间\trait名::方法名）测试过	
absolute_trait_method_reference:
	class_name T_PAAMAYIM_NEKUDOTAYIM identifier
		{ $$ = zend_ast_create(ZEND_AST_METHOD_REFERENCE, $1, $3); }
;

// clear, {代码块} 或 ; ，只有 attributed_class_statement 用到
method_body:
// 主要是为了这个，添加抽象方法
		';' /* abstract method */		{ $$ = NULL; }
	|	'{' inner_statement_list '}'	{ $$ = $2; }
;

// clear, 类成员变量修饰符，比方法修饰符多一个var，只在 attributed_class_statement 引用
variable_modifiers:
		non_empty_member_modifiers		{ $$ = $1; }
	|	T_VAR							{ $$ = ZEND_ACC_PUBLIC; }
;

// clear, 任意多个类成员方法修饰符，只在 attributed_class_statement 引用
method_modifiers:
		%empty						{ $$ = ZEND_ACC_PUBLIC; }
	|	non_empty_member_modifiers
			{ $$ = $1; if (!($$ & ZEND_ACC_PPP_MASK)) { $$ |= ZEND_ACC_PUBLIC; } }
;

// clear 一个或多个类成员方法修饰符
non_empty_member_modifiers:
		member_modifier			{ $$ = $1; }
	|	non_empty_member_modifiers member_modifier
			{ $$ = zend_add_member_modifier($1, $2); if (!$$) { YYERROR; } }
;

// clear, 类成员方法修饰符，有引用，被non_empty_member_modifiers，trait_alias引用
member_modifier:
		T_PUBLIC				{ $$ = ZEND_ACC_PUBLIC; }
	|	T_PROTECTED				{ $$ = ZEND_ACC_PROTECTED; }
	|	T_PRIVATE				{ $$ = ZEND_ACC_PRIVATE; }
	|	T_STATIC				{ $$ = ZEND_ACC_STATIC; }
	|	T_ABSTRACT				{ $$ = ZEND_ACC_ABSTRACT; }
	|	T_FINAL					{ $$ = ZEND_ACC_FINAL; }
	|	T_READONLY				{ $$ = ZEND_ACC_READONLY; }
;

// clear，多个property可以用,隔开，只有 attributed_class_statement 引用 
property_list:
		property_list ',' property { $$ = zend_ast_list_add($1, $3); }
	|	property { $$ = zend_ast_create_list(1, ZEND_AST_PROP_DECL, $1); }
;

// clear，只有 property_list 引用 ，声名类property可以赋值或不赋值
property:
		T_VARIABLE backup_doc_comment
			{ $$ = zend_ast_create(ZEND_AST_PROP_ELEM, $1, NULL, ($2 ? zend_ast_create_zval_from_str($2) : NULL)); }
	|	T_VARIABLE '=' expr backup_doc_comment
			{ $$ = zend_ast_create(ZEND_AST_PROP_ELEM, $1, $3, ($4 ? zend_ast_create_zval_from_str($4) : NULL)); }
;

// clear,一次声名多个类常量
class_const_list:
		class_const_list ',' class_const_decl { $$ = zend_ast_list_add($1, $3); }
	|	class_const_decl { $$ = zend_ast_create_list(1, ZEND_AST_CLASS_CONST_DECL, $1); }
;

// clear,类常量，里面可以用保留字，只有 class_const_list 引用 
class_const_decl:
	identifier '=' expr backup_doc_comment { $$ = zend_ast_create(ZEND_AST_CONST_ELEM, $1, $3, ($4 ? zend_ast_create_zval_from_str($4) : NULL)); }
;

// clear, 单个常量内容，不可以用保留字，只有 const_list 引用
	// 用const声名常量，表达式里不可以有变量。用define声名就可以有变量。
const_decl:
	// 常量名 + = + 表达式 + 注释，这个注释明显是反射用的
	T_STRING '=' expr backup_doc_comment { $$ = zend_ast_create(ZEND_AST_CONST_ELEM, $1, $3, ($4 ? /($4) : NULL)); }
;

// clear, 一个或多个 echo_expr, echo 后的语句会被特殊处理
echo_expr_list:
		echo_expr_list ',' echo_expr { $$ = zend_ast_list_add($1, $3); }
	|	echo_expr { $$ = zend_ast_create_list(1, ZEND_AST_STMT_LIST, $1); }
;

// clear, 普通表达式，只有 echo_expr_list 引用 , 声名是为了 ZEND_AST_ECHO
echo_expr:
	expr { $$ = zend_ast_create(ZEND_AST_ECHO, $1); }
;

// clear, for循环里用的，用; 隔开的三段语句，每段可以是用逗号隔开的一个或多个公示
	// 这个是其中一段，这里加上可以为空了
	// 只有在for 循环中用到
for_exprs:
		%empty			{ $$ = NULL; }
	|	non_empty_for_exprs	{ $$ = $1; }
;

// clear, for循环里用的，用; 隔开的三段语句，每段可以是用逗号隔开的一个或多个公示
non_empty_for_exprs:
		non_empty_for_exprs ',' expr { $$ = zend_ast_list_add($1, $3); }
// 最少一个公式（这里不能为空）
	|	expr { $$ = zend_ast_create_list(1, ZEND_AST_EXPR_LIST, $1); }
;

// clear, 匿名类  只能直接new 
	// 示例 $a = new class(1,2){ function __construct($a,$b){var_dump($a,$b);} };
anonymous_class:
        T_CLASS { $<num>$ = CG(zend_lineno); } ctor_arguments
		extends_from implements_list backup_doc_comment '{' class_statement_list '}' {
			zend_ast *decl = zend_ast_create_decl(
				ZEND_AST_CLASS, ZEND_ACC_ANON_CLASS, $<num>2, $6, NULL,
				$4, $5, $8, NULL, NULL);
			$$ = zend_ast_create(ZEND_AST_NEW, decl, $3);
		}
;

// clear, new 开头的语句，独立表达式（expr）规则, 针对类，匿名类（可带修饰属性）
new_expr:
	// 引用的类名，可以直接是类名，是变量，或者是表达式
		T_NEW class_name_reference ctor_arguments
			{ $$ = zend_ast_create(ZEND_AST_NEW, $2, $3); }
	|	T_NEW anonymous_class
			{ $$ = $2; }
	|	T_NEW attributes anonymous_class
			{ zend_ast_with_attributes($3->child[0], $2); $$ = $3; }
;

// ing2, 常用表达式(expression),  这个是最多的了 89 条规则
expr:
// 普通变量
		variable
			{ $$ = $1; }
// clear, 解压并赋值
	// 示例：list($a,$b,$c) = (fn()=>[1,2,3])();var_dump($a,$b,$c);
	|	T_LIST '(' array_pair_list ')' '=' expr
			{ $3->attr = ZEND_ARRAY_SYNTAX_LIST; $$ = zend_ast_create(ZEND_AST_ASSIGN, $3, $6); }
	// 示例： [$a,$b,$c] = (fn()=>[1,2,3])();var_dump($a,$b,$c);
	|	'[' array_pair_list ']' '=' expr
			{ $2->attr = ZEND_ARRAY_SYNTAX_SHORT; $$ = zend_ast_create(ZEND_AST_ASSIGN, $2, $5); }
// clear, 普通赋值给变量
	|	variable '=' expr
			{ $$ = zend_ast_create(ZEND_AST_ASSIGN, $1, $3); }
// clear, 地址赋值
	|	variable '=' ampersand variable
			{ $$ = zend_ast_create(ZEND_AST_ASSIGN_REF, $1, $4); }
// clear, clone语句
	|	T_CLONE expr { $$ = zend_ast_create(ZEND_AST_CLONE, $2); }
// clear, 变量后面加 (操作符和= ) 再加表达式
	|	variable T_PLUS_EQUAL expr
			{ $$ = zend_ast_create_assign_op(ZEND_ADD, $1, $3); }
	|	variable T_MINUS_EQUAL expr
			{ $$ = zend_ast_create_assign_op(ZEND_SUB, $1, $3); }
	|	variable T_MUL_EQUAL expr
			{ $$ = zend_ast_create_assign_op(ZEND_MUL, $1, $3); }
	|	variable T_POW_EQUAL expr
			{ $$ = zend_ast_create_assign_op(ZEND_POW, $1, $3); }
	|	variable T_DIV_EQUAL expr
			{ $$ = zend_ast_create_assign_op(ZEND_DIV, $1, $3); }
	|	variable T_CONCAT_EQUAL expr
			{ $$ = zend_ast_create_assign_op(ZEND_CONCAT, $1, $3); }
	|	variable T_MOD_EQUAL expr
			{ $$ = zend_ast_create_assign_op(ZEND_MOD, $1, $3); }
	|	variable T_AND_EQUAL expr
			{ $$ = zend_ast_create_assign_op(ZEND_BW_AND, $1, $3); }
	|	variable T_OR_EQUAL expr
			{ $$ = zend_ast_create_assign_op(ZEND_BW_OR, $1, $3); }
	|	variable T_XOR_EQUAL expr
			{ $$ = zend_ast_create_assign_op(ZEND_BW_XOR, $1, $3); }
	|	variable T_SL_EQUAL expr
			{ $$ = zend_ast_create_assign_op(ZEND_SL, $1, $3); }
	|	variable T_SR_EQUAL expr
			{ $$ = zend_ast_create_assign_op(ZEND_SR, $1, $3); }
	|	variable T_COALESCE_EQUAL expr
			{ $$ = zend_ast_create(ZEND_AST_ASSIGN_COALESCE, $1, $3); }
// clear, ++ -- 在变量前或后（不是表达式前后）
	|	variable T_INC { $$ = zend_ast_create(ZEND_AST_POST_INC, $1); }
	|	T_INC variable { $$ = zend_ast_create(ZEND_AST_PRE_INC, $2); }
	|	variable T_DEC { $$ = zend_ast_create(ZEND_AST_POST_DEC, $1); }
	|	T_DEC variable { $$ = zend_ast_create(ZEND_AST_PRE_DEC, $2); }
// clear, 布尔运算和逻辑运算符连接两个表达式 （||,&&,or,and,xor） 
	|	expr T_BOOLEAN_OR expr
			{ $$ = zend_ast_create(ZEND_AST_OR, $1, $3); }
	|	expr T_BOOLEAN_AND expr
			{ $$ = zend_ast_create(ZEND_AST_AND, $1, $3); }
	|	expr T_LOGICAL_OR expr
			{ $$ = zend_ast_create(ZEND_AST_OR, $1, $3); }
	|	expr T_LOGICAL_AND expr
			{ $$ = zend_ast_create(ZEND_AST_AND, $1, $3); }
	|	expr T_LOGICAL_XOR expr
			{ $$ = zend_ast_create_binary_op(ZEND_BOOL_XOR, $1, $3); }
// clear, 位运算符 |,&,amp,^
	|	expr '|' expr	{ $$ = zend_ast_create_binary_op(ZEND_BW_OR, $1, $3); }
// ing1, amp 怎么用的？
	|	expr T_AMPERSAND_NOT_FOLLOWED_BY_VAR_OR_VARARG expr	{ $$ = zend_ast_create_binary_op(ZEND_BW_AND, $1, $3); }
// clear, 操作符连接两个表达式
	|	expr T_AMPERSAND_FOLLOWED_BY_VAR_OR_VARARG expr	{ $$ = zend_ast_create_binary_op(ZEND_BW_AND, $1, $3); }
	|	expr '^' expr	{ $$ = zend_ast_create_binary_op(ZEND_BW_XOR, $1, $3); }
	|	expr '.' expr 	{ $$ = zend_ast_create_concat_op($1, $3); }
	|	expr '+' expr 	{ $$ = zend_ast_create_binary_op(ZEND_ADD, $1, $3); }
	|	expr '-' expr 	{ $$ = zend_ast_create_binary_op(ZEND_SUB, $1, $3); }
	|	expr '*' expr	{ $$ = zend_ast_create_binary_op(ZEND_MUL, $1, $3); }
	|	expr T_POW expr	{ $$ = zend_ast_create_binary_op(ZEND_POW, $1, $3); }
	|	expr '/' expr	{ $$ = zend_ast_create_binary_op(ZEND_DIV, $1, $3); }
	|	expr '%' expr 	{ $$ = zend_ast_create_binary_op(ZEND_MOD, $1, $3); }
// clear, 位运算符连接两个表达式
	| 	expr T_SL expr	{ $$ = zend_ast_create_binary_op(ZEND_SL, $1, $3); }
	|	expr T_SR expr	{ $$ = zend_ast_create_binary_op(ZEND_SR, $1, $3); }
// ? %prec
	|	'+' expr %prec '~' { $$ = zend_ast_create(ZEND_AST_UNARY_PLUS, $2); }
	|	'-' expr %prec '~' { $$ = zend_ast_create(ZEND_AST_UNARY_MINUS, $2); }
// clear, !,~ 加表达式（~这个东西是有符号数字的正负反转 ~0 == -1 )
	|	'!' expr { $$ = zend_ast_create_ex(ZEND_AST_UNARY_OP, ZEND_BOOL_NOT, $2); }
	|	'~' expr { $$ = zend_ast_create_ex(ZEND_AST_UNARY_OP, ZEND_BW_NOT, $2); }
// clear, 是否绝对相等 （=== , !==）
	|	expr T_IS_IDENTICAL expr
			{ $$ = zend_ast_create_binary_op(ZEND_IS_IDENTICAL, $1, $3); }
	|	expr T_IS_NOT_IDENTICAL expr
			{ $$ = zend_ast_create_binary_op(ZEND_IS_NOT_IDENTICAL, $1, $3); }
// clear, 两个表达式比大小
	|	expr T_IS_EQUAL expr
			{ $$ = zend_ast_create_binary_op(ZEND_IS_EQUAL, $1, $3); }
	|	expr T_IS_NOT_EQUAL expr
			{ $$ = zend_ast_create_binary_op(ZEND_IS_NOT_EQUAL, $1, $3); }
	|	expr '<' expr
			{ $$ = zend_ast_create_binary_op(ZEND_IS_SMALLER, $1, $3); }
	|	expr T_IS_SMALLER_OR_EQUAL expr
			{ $$ = zend_ast_create_binary_op(ZEND_IS_SMALLER_OR_EQUAL, $1, $3); }
	|	expr '>' expr
			{ $$ = zend_ast_create(ZEND_AST_GREATER, $1, $3); }
	|	expr T_IS_GREATER_OR_EQUAL expr
			{ $$ = zend_ast_create(ZEND_AST_GREATER_EQUAL, $1, $3); }
// clear, <=>
	|	expr T_SPACESHIP expr
			{ $$ = zend_ast_create_binary_op(ZEND_SPACESHIP, $1, $3); }
// clear, instance of 语句，前面是表达式，后面是引用类名
	|	expr T_INSTANCEOF class_name_reference
			{ $$ = zend_ast_create(ZEND_AST_INSTANCEOF, $1, $3); }
// clear, 表达式直接放在括号里
	|	'(' expr ')' {
			// 直接，解包了
			$$ = $2;
			// 如果是三元操作符， 设置 ZEND_PARENTHESIZED_CONDITIONAL
			if ($$->kind == ZEND_AST_CONDITIONAL) $$->attr = ZEND_PARENTHESIZED_CONDITIONAL;
		}
	|	new_expr { $$ = $1; }
// clear, 三元操作符
	|	expr '?' expr ':' expr
			{ $$ = zend_ast_create(ZEND_AST_CONDITIONAL, $1, $3, $5); }
	|	expr '?' ':' expr
			{ $$ = zend_ast_create(ZEND_AST_CONDITIONAL, $1, NULL, $4); }
// clear, ?? 连接两个表达式
	|	expr T_COALESCE expr
			{ $$ = zend_ast_create(ZEND_AST_COALESCE, $1, $3); }
// clear, 内部函数：isset, empty, include, include_once, eval, require, require_once
	|	internal_functions_in_yacc { $$ = $1; }
// clear, 表达式前面加强制类型转换 例如(int)	
	|	T_INT_CAST expr		{ $$ = zend_ast_create_cast(IS_LONG, $2); }
	|	T_DOUBLE_CAST expr	{ $$ = zend_ast_create_cast(IS_DOUBLE, $2); }
	|	T_STRING_CAST expr	{ $$ = zend_ast_create_cast(IS_STRING, $2); }
	|	T_ARRAY_CAST expr	{ $$ = zend_ast_create_cast(IS_ARRAY, $2); }
	|	T_OBJECT_CAST expr	{ $$ = zend_ast_create_cast(IS_OBJECT, $2); }
	|	T_BOOL_CAST expr	{ $$ = zend_ast_create_cast(_IS_BOOL, $2); }
	|	T_UNSET_CAST expr	{ $$ = zend_ast_create_cast(IS_NULL, $2); }
// clear, exit 语句，后面可以加参数或者不加
	|	T_EXIT exit_expr	{ $$ = zend_ast_create(ZEND_AST_EXIT, $2); }
// clear, 表达式前面加错误抑制符 @	
	|	'@' expr			{ $$ = zend_ast_create(ZEND_AST_SILENCE, $2); }
//
	|	scalar { $$ = $1; }
// clear, 反引号引用的shell命令
	|	'`' backticks_expr '`' { $$ = zend_ast_create(ZEND_AST_SHELL_EXEC, $2); }
// clear, print 语句
	|	T_PRINT expr { $$ = zend_ast_create(ZEND_AST_PRINT, $2); }
// ？, yield 语句
	|	T_YIELD { $$ = zend_ast_create(ZEND_AST_YIELD, NULL, NULL); CG(extra_fn_flags) |= ZEND_ACC_GENERATOR; }
	|	T_YIELD expr { $$ = zend_ast_create(ZEND_AST_YIELD, $2, NULL); CG(extra_fn_flags) |= ZEND_ACC_GENERATOR; }
	|	T_YIELD expr T_DOUBLE_ARROW expr { $$ = zend_ast_create(ZEND_AST_YIELD, $4, $2); CG(extra_fn_flags) |= ZEND_ACC_GENERATOR; }
	|	T_YIELD_FROM expr { $$ = zend_ast_create(ZEND_AST_YIELD_FROM, $2); CG(extra_fn_flags) |= ZEND_ACC_GENERATOR; }
// clear, throw 语句
	|	T_THROW expr { $$ = zend_ast_create(ZEND_AST_THROW, $2); }
// clear, 闭包函数, 可以加 attributes 和 static
	|	inline_function { $$ = $1; }
	|	attributes inline_function { $$ = zend_ast_with_attributes($2, $1); }
// ?, 加static有什么用
	|	T_STATIC inline_function { $$ = $2; ((zend_ast_decl *) $$)->flags |= ZEND_ACC_STATIC; }
	|	attributes T_STATIC inline_function
			{ $$ = zend_ast_with_attributes($3, $1); ((zend_ast_decl *) $$)->flags |= ZEND_ACC_STATIC; }
// clear, match 语句
	|	match { $$ = $1; }
;

// clear, 闭包函数，最大的特点是没有函数名，可以不用变量接收，只有 expr 引用
inline_function:
// clear, 用 function 语句声名，可以用use 语句
		function returns_ref backup_doc_comment '(' parameter_list ')' lexical_vars return_type
		backup_fn_flags '{' inner_statement_list '}' backup_fn_flags
			{ $$ = zend_ast_create_decl(ZEND_AST_CLOSURE, $2 | $13, $1, $3,
				  zend_string_init("{closure}", sizeof("{closure}") - 1, 0),
				  $5, $7, $11, $8, NULL); CG(extra_fn_flags) = $9; }
// clear, 用 fn() 语句 声明， 只能有一个表达式的逻辑, 不能用 use 语句
	// backup_lex_pos 触发内部逻辑
	|	fn returns_ref backup_doc_comment '(' parameter_list ')' return_type
		T_DOUBLE_ARROW backup_fn_flags backup_lex_pos expr backup_fn_flags
			{ $$ = zend_ast_create_decl(ZEND_AST_ARROW_FUNC, $2 | $12, $1, $3,
				  zend_string_init("{closure}", sizeof("{closure}") - 1, 0), $5, NULL, $11, $7, NULL);
				  ((zend_ast_decl *) $$)->lex_pos = $10;
				  CG(extra_fn_flags) = $9; }
;

// clear, fn() ，触发内部逻辑
fn:
	T_FN { $$ = CG(zend_lineno); }
;

// clear, function 关键字，单独做个规则，触发内部逻辑
function:
	T_FUNCTION { $$ = CG(zend_lineno); }
;

// clear, 无语句，为了触发内部逻辑
backup_doc_comment:
	%empty { $$ = CG(doc_comment); CG(doc_comment) = NULL; }
;

// ing1, 在定义 function 时都会用到，触发内部逻辑 
backup_fn_flags:
	%prec PREC_ARROW_FUNCTION %empty { $$ = CG(extra_fn_flags); CG(extra_fn_flags) = 0; }
;

// clear, 无语句，为了触发内部逻辑, 只有 fn() 语句声名闭包时触发
backup_lex_pos:
	%empty { $$ = LANG_SCNG(yy_text); }
;

// clear, 函数或方法声名成返回地址，只用在function 或 fn() 关键字后面
returns_ref:
		%empty	{ $$ = 0; }
	|	ampersand	{ $$ = ZEND_ACC_RETURN_REFERENCE; }
;

// clear, 可选，声名闭包函数时的 use 语句，
	// 里面只能用 T_VARIABLE 或地址，其他 simple_variable 都不能用
	// 只有 inline_function 用到，不是闭包函数不可以用这个
lexical_vars:
		%empty { $$ = NULL; }
	|	T_USE '(' lexical_var_list possible_comma ')' { $$ = $3; }
;

// clear, 一个或多个(基本变量或变量地址), 只有 lexical_vars 引用
lexical_var_list:
		lexical_var_list ',' lexical_var { $$ = zend_ast_list_add($1, $3); }
	|	lexical_var { $$ = zend_ast_create_list(1, ZEND_AST_CLOSURE_USES, $1); }
;

// clear, 基本变量或基本变量地址， 只有 lexical_var_list 引用
lexical_var:
		T_VARIABLE		{ $$ = $1; }
	|	ampersand T_VARIABLE	{ $$ = $2; $$->attr = ZEND_BIND_REF; }
;

// ing3, 可以当成函数调用的东东，只有 callable_variable 引用
function_call:
// clear, 调用普通函数
		name argument_list
			{ $$ = zend_ast_create(ZEND_AST_CALL, $1, $2); }
// clear, 保留字中，只有 readonly 可以作方法名和函数名
	|	T_READONLY argument_list {
			zval zv;
			if (zend_lex_tstring(&zv, $1) == FAILURE) { YYABORT; }
			$$ = zend_ast_create(ZEND_AST_CALL, zend_ast_create_zval(&zv), $2);
		}
// clear, 类的静态方法
	|	class_name T_PAAMAYIM_NEKUDOTAYIM member_name argument_list
			{ $$ = zend_ast_create(ZEND_AST_STATIC_CALL, $1, $3, $4); }
// clear, 变量类名调用静态方法
	|	variable_class_name T_PAAMAYIM_NEKUDOTAYIM member_name argument_list
			{ $$ = zend_ast_create(ZEND_AST_STATIC_CALL, $1, $3, $4); }
// 
	|	callable_expr { $<num>$ = CG(zend_lineno); } argument_list { 
			$$ = zend_ast_create(ZEND_AST_CALL, $1, $3); 
			$$->lineno = $<num>2;
		}
;

// ing4, 这不是声名类，这是用 static 或 类名 引用类~
class_name:
// 用 static 引用本类
	// 里面的业务逻辑是什么？
		T_STATIC
			{ zval zv; ZVAL_INTERNED_STR(&zv, ZSTR_KNOWN(ZEND_STR_STATIC));
			  $$ = zend_ast_create_zval_ex(&zv, ZEND_NAME_NOT_FQ); }
// 引用指定类
	|	name { $$ = $1; }
;

// 引用的类名，可以直接是类名，是变量，或者是表达式
class_name_reference:
		class_name		{ $$ = $1; }
	|	new_variable	{ $$ = $1; }
	|	'(' expr ')'	{ $$ = $2; }
;

// clear,  exit 后面可以加参数（当函数调用）也可以不加
	// 只有在exit 后面用到
exit_expr:
		%empty				{ $$ = NULL; }
	|	'(' optional_expr ')'	{ $$ = $2; }
;

// clear, 反引号, 里面的内容和双引号一样解析
backticks_expr:
		%empty
			{ $$ = zend_ast_create_zval_from_str(ZSTR_EMPTY_ALLOC()); }
	|	T_ENCAPSED_AND_WHITESPACE { $$ = $1; }
	|	encaps_list { $$ = $1; }
;

// clear, 可留空的参数列表
ctor_arguments:
		%empty	{ $$ = zend_ast_create_list(0, ZEND_AST_ARG_LIST); }
	|	argument_list { $$ = $1; }
;

// ing4，最常的字串和数组, 3处引用，这个名称是什么意思 ？
	// scalar, fully_dereferenceable, callable_expr
dereferenceable_scalar:
// clear, array() 数组
		T_ARRAY '(' array_pair_list ')'	{ $$ = $3; $$->attr = ZEND_ARRAY_SYNTAX_LONG; }
// clear, [] 数组
	|	'[' array_pair_list ']'			{ $$ = $2; $$->attr = ZEND_ARRAY_SYNTAX_SHORT; }
// clear 字串常量
	|	T_CONSTANT_ENCAPSED_STRING		{ $$ = $1; }
// clear, 双引号中放字串和变量的 混合
	|	'"' encaps_list '"'			 	{ $$ = $2; }
;

// clear, 标量：clear，最常用的数字，字串、大字串（含变量），常量，类常量
scalar:
// clear, 短整形
		T_LNUMBER 	{ $$ = $1; }
// clear, 长整形，小数
	|	T_DNUMBER 	{ $$ = $1; }
// clear, 大字符串	
	|	T_START_HEREDOC T_ENCAPSED_AND_WHITESPACE T_END_HEREDOC { $$ = $2; }
// clear, 空的大字符串
	|	T_START_HEREDOC T_END_HEREDOC
			{ $$ = zend_ast_create_zval_from_str(ZSTR_EMPTY_ALLOC()); }
// clear, 大字符串中放
	|	T_START_HEREDOC encaps_list T_END_HEREDOC { $$ = $2; }
// clear, 最常的字串和数组
	|	dereferenceable_scalar	{ $$ = $1; }
// clear, 常量
	|	constant				{ $$ = $1; }
	|	class_constant			{ $$ = $1; }
;

// clear, 常量名， 两处引用 
constant:
		name		{ $$ = zend_ast_create(ZEND_AST_CONST, $1); }
// clear, 下面是魔术常量（MAGIC_CONST，俗称魔术变量）	
	|	T_LINE 		{ $$ = zend_ast_create_ex(ZEND_AST_MAGIC_CONST, T_LINE); }
	|	T_FILE 		{ $$ = zend_ast_create_ex(ZEND_AST_MAGIC_CONST, T_FILE); }
	|	T_DIR   	{ $$ = zend_ast_create_ex(ZEND_AST_MAGIC_CONST, T_DIR); }
	|	T_TRAIT_C	{ $$ = zend_ast_create_ex(ZEND_AST_MAGIC_CONST, T_TRAIT_C); }
	|	T_METHOD_C	{ $$ = zend_ast_create_ex(ZEND_AST_MAGIC_CONST, T_METHOD_C); }
	|	T_FUNC_C	{ $$ = zend_ast_create_ex(ZEND_AST_MAGIC_CONST, T_FUNC_C); }
	|	T_NS_C		{ $$ = zend_ast_create_ex(ZEND_AST_MAGIC_CONST, T_NS_C); }
	|	T_CLASS_C	{ $$ = zend_ast_create_ex(ZEND_AST_MAGIC_CONST, T_CLASS_C); }
;

// clear, 引用类中的常量
class_constant:
// 普通类名::常量名
		class_name T_PAAMAYIM_NEKUDOTAYIM identifier
			{ $$ = zend_ast_create_class_const_or_name($1, $3); }
// 变量作为类名
	|	variable_class_name T_PAAMAYIM_NEKUDOTAYIM identifier
			{ $$ = zend_ast_create_class_const_or_name($1, $3); }
;

// clear, 可留空表达式，为空或使用表达式
optional_expr:
		%empty	{ $$ = NULL; }
	|	expr		{ $$ = $1; }
;

variable_class_name:
		fully_dereferenceable { $$ = $1; }
;

// clear, 可当成对象引用的东东
fully_dereferenceable:
// 普通变量
		variable				{ $$ = $1; }
// 括起来的表达式
	|	'(' expr ')'			{ $$ = $2; }
// clear, 
	|	dereferenceable_scalar	{ $$ = $1; }
// clear 类中的常量
	|	class_constant			{ $$ = $1; }
;

// clear, 可当成对象引用的东东,包含常量
array_object_dereferenceable:
		fully_dereferenceable	{ $$ = $1; }
	|	constant				{ $$ = $1; }
;

// 可被直接调用的表达式 callable_expr -> callable_variable ->  function_call -> callable_expr
	// 只有 function_call 引用
callable_expr:
// clear, 可调用变量
		callable_variable		{ $$ = $1; }
// clear, 语句的返回值可被直接调用
	|	'(' expr ')'			{ $$ = $2; }
// clear, 字串可以直接当成函数名调用 $a = 'print';$a('1');
// 数组可以调用对象中的方法，或静态方法
	// $a = new class{	function a($a,$b,$c){ var_dump($a,$b,$c); }}; [$a,'a',](1,2,3);
	|	dereferenceable_scalar	{ $$ = $1; }

;

// clear, 可被当成函数调用的变量，和普通变量没什么区别，不过要生成不同的 ast,  variable 和 callable_expr 引用	
callable_variable:
// 简单变量
		simple_variable
			{ $$ = zend_ast_create(ZEND_AST_VAR, $1); }
// 数组风格 $a[1+1]();
	|	array_object_dereferenceable '[' optional_expr ']'
			{ $$ = zend_ast_create(ZEND_AST_DIM, $1, $3); }
// {}变量风格 $a{}(); 弃用
	|	array_object_dereferenceable '{' expr '}'
			{ $$ = zend_ast_create_ex(ZEND_AST_DIM, ZEND_DIM_ALTERNATIVE_SYNTAX, $1, $3); }
// $a->fff();
	|	array_object_dereferenceable T_OBJECT_OPERATOR property_name argument_list
			{ $$ = zend_ast_create(ZEND_AST_METHOD_CALL, $1, $3, $4); }
// $a?->fff();			
	|	array_object_dereferenceable T_NULLSAFE_OBJECT_OPERATOR property_name argument_list
			{ $$ = zend_ast_create(ZEND_AST_NULLSAFE_METHOD_CALL, $1, $3, $4); }
// 循环调用
	|	function_call { $$ = $1; }
;

// clear, 变量
variable:
// 又是循环引用
		callable_variable
			{ $$ = $1; }
// 静态成员变量
	|	static_member
			{ $$ = $1; }
// 引用对象属性名
	|	array_object_dereferenceable T_OBJECT_OPERATOR property_name
			{ $$ = zend_ast_create(ZEND_AST_PROP, $1, $3); }
// 引用对象属性名，允null
	|	array_object_dereferenceable T_NULLSAFE_OBJECT_OPERATOR property_name
			{ $$ = zend_ast_create(ZEND_AST_NULLSAFE_PROP, $1, $3); }
;

// clear，简单变量名由 $+变量名组成，如$name, ${name} $$name 不可以加 ->, [] 
simple_variable:
	// 普通变量，一个$开头，如：$a
		T_VARIABLE			{ $$ = $1; }
	// ${表达式} ，如${1+2}
	|	'$' '{' expr '}'	{ $$ = $3; }
	// n个$$开头，如$$a, $$$b
	|	'$' simple_variable	{ $$ = zend_ast_create(ZEND_AST_VAR, $2); }
;

// clear,类的静态变量
static_member:
// 直接写类名 :: 简单变量名
		class_name T_PAAMAYIM_NEKUDOTAYIM simple_variable
			{ $$ = zend_ast_create(ZEND_AST_STATIC_PROP, $1, $3); }
// 变量当成类名 :: 简单变量名
	|	variable_class_name T_PAAMAYIM_NEKUDOTAYIM simple_variable
			{ $$ = zend_ast_create(ZEND_AST_STATIC_PROP, $1, $3); }
;

// clear, 复合变量名
	// 包含简单变量，表达式变量，数组的子元素，对象的动态和静态成员，以及类的静态成员变量
	// 只被 
new_variable:
	// 前面可以是简单变量
		simple_variable
			{ $$ = zend_ast_create(ZEND_AST_VAR, $1); }
// clear, 后面加多个： [可选表达式]
	|	new_variable '[' optional_expr ']'
			{ $$ = zend_ast_create(ZEND_AST_DIM, $1, $3); }
// clear, 弃用，后面加多个：{}
	|	new_variable '{' expr '}'
			{ $$ = zend_ast_create_ex(ZEND_AST_DIM, ZEND_DIM_ALTERNATIVE_SYNTAX, $1, $3); }
// clear, 后面加多个：-> 属性名
	|	new_variable T_OBJECT_OPERATOR property_name
			{ $$ = zend_ast_create(ZEND_AST_PROP, $1, $3); }
// clear, 后面加多个：?-> 属性名
	|	new_variable T_NULLSAFE_OBJECT_OPERATOR property_name
			{ $$ = zend_ast_create(ZEND_AST_NULLSAFE_PROP, $1, $3); }
// clear, 用类名加 :: 加变量名 调用类的静态成员变量
	|	class_name T_PAAMAYIM_NEKUDOTAYIM simple_variable
			{ $$ = zend_ast_create(ZEND_AST_STATIC_PROP, $1, $3); }
// clear, 后面加多个：::	加变量名 调用类的静态成员变量
	|	new_variable T_PAAMAYIM_NEKUDOTAYIM simple_variable
			{ $$ = zend_ast_create(ZEND_AST_STATIC_PROP, $1, $3); }
;

// clear 类成方法员名,可以用保留字（仅指成员方法，成员变量是property_name）
	// 只有类相关引用
member_name:
	// 保留字
		identifier { $$ = $1; }
	// 表达式返回名字
	|	'{' expr '}'	{ $$ = $2; }
	// 变量
	|	simple_variable	{ $$ = zend_ast_create(ZEND_AST_VAR, $1); }
;

// clear, 类属性名，在不同场景下被调用，但都是作为类
property_name:
	// 直接用字串
		T_STRING { $$ = $1; }
	// 大括号用的表达式： echo $b?->{"c"}, 测试过
	|	'{' expr '}'	{ $$ = $2; }
	// 简单变量
	|	simple_variable	{ $$ = zend_ast_create(ZEND_AST_VAR, $1); }
;

// clear, 键值对 去掉右边的空项
array_pair_list:
		non_empty_array_pair_list
			{ /* allow single trailing comma */ $$ = zend_ast_list_rtrim($1); }
;

// clear, 可留空的键值对， 只有 non_empty_array_pair_list 引用
possible_array_pair:
		%empty { $$ = NULL; }
	|	array_pair  { $$ = $1; }
;
// ing3, 0-n 个键值对
	// 多个possible_array_pair拼接，怎么就成了non_empty了呢？ 
	// 只有 array_pair_list 引用
non_empty_array_pair_list:
	// 在 $1 里添加 子语句 $3
		non_empty_array_pair_list ',' possible_array_pair
			{ $$ = zend_ast_list_add($1, $3); }
	// ZEND_AST_ARRAY 第一个键值对，创建语句列表
	|	possible_array_pair
			{ $$ = zend_ast_create_list(1, ZEND_AST_ARRAY, $1); }
;

// clear, array 里的键值对，只有possible_array_pair引用
array_pair:
// 表达式 => 表达式 
		expr T_DOUBLE_ARROW expr
			{ $$ = zend_ast_create(ZEND_AST_ARRAY_ELEM, $3, $1); }
// 表达式
	|	expr
			{ $$ = zend_ast_create(ZEND_AST_ARRAY_ELEM, $1, NULL); }
// 表达式 => 变量地址
	|	expr T_DOUBLE_ARROW ampersand variable
			{ $$ = zend_ast_create_ex(ZEND_AST_ARRAY_ELEM, 1, $4, $1); }
// 引用变量地址
	|	ampersand variable
			{ $$ = zend_ast_create_ex(ZEND_AST_ARRAY_ELEM, 1, $2, NULL); }
// $a = [ ... expr ];  expr 必须返回数组，不能是一维对象
	// 报错：  Only arrays and Traversables can be unpacked
	|	T_ELLIPSIS expr
			{ $$ = zend_ast_create(ZEND_AST_UNPACK, $2); }			
// 示例 foreach(array(array('a'=>1,'b'=>2), array('a'=>3,'b'=>4)) as list('a'=>$a, 'b'=>$b)) {   var_dump($a . $b); }
	|	expr T_DOUBLE_ARROW T_LIST '(' array_pair_list ')'
			{ $5->attr = ZEND_ARRAY_SYNTAX_LIST;
			  $$ = zend_ast_create(ZEND_AST_ARRAY_ELEM, $5, $1); }
// 示例 foreach(array(array(1,2), array(3,4)) as list($a, $b)) {    var_dump($a . $b); }
// 可以嵌套，示例 foreach([] as list($a,list($b,$c))){}
	|	T_LIST '(' array_pair_list ')'
			{ $3->attr = ZEND_ARRAY_SYNTAX_LIST;
			  $$ = zend_ast_create(ZEND_AST_ARRAY_ELEM, $3, NULL); }
;

// clear, 字串常量混合 单个encaps_var 或 （连续或不连续的多个 encaps_var ）， 3处引用
	// 反引号，backticks_expr 
encaps_list:
		encaps_list encaps_var
			{ $$ = zend_ast_list_add($1, $2); }
	|	encaps_list T_ENCAPSED_AND_WHITESPACE
			{ $$ = zend_ast_list_add($1, $2); }
	|	encaps_var
			{ $$ = zend_ast_create_list(1, ZEND_AST_ENCAPS_LIST, $1); }
	|	T_ENCAPSED_AND_WHITESPACE encaps_var
			{ $$ = zend_ast_create_list(2, ZEND_AST_ENCAPS_LIST, $1, $2); }
;

// clear, 字串中插入变量, 只有 encaps_list 引用
encaps_var:
// clear, 基础变量
		T_VARIABLE
			{ $$ = zend_ast_create(ZEND_AST_VAR, $1); }
// clear, 变量+offset
	|	T_VARIABLE '[' encaps_var_offset ']'
			{ $$ = zend_ast_create(ZEND_AST_DIM,
			      zend_ast_create(ZEND_AST_VAR, $1), $3); }
// clear, ->
	|	T_VARIABLE T_OBJECT_OPERATOR T_STRING
			{ $$ = zend_ast_create(ZEND_AST_PROP,
			      zend_ast_create(ZEND_AST_VAR, $1), $3); }
// clear, ?->
	|	T_VARIABLE T_NULLSAFE_OBJECT_OPERATOR T_STRING
			{ $$ = zend_ast_create(ZEND_AST_NULLSAFE_PROP,
			      zend_ast_create(ZEND_AST_VAR, $1), $3); }
// clear, ${expr} 和普通变量一样，测试过 ${2} =1; echo "${1+1}","{$a->{1}}";
	// 只有这里用到 T_DOLLAR_OPEN_CURLY_BRACES 
	|	T_DOLLAR_OPEN_CURLY_BRACES expr '}'
			{ $$ = zend_ast_create_ex(ZEND_AST_VAR, ZEND_ENCAPS_VAR_DOLLAR_CURLY_VAR_VAR, $2); }
// clear, ${abc} 和普通变量一样，abc 会被识别成 T_STRING_VARNAME
	|	T_DOLLAR_OPEN_CURLY_BRACES T_STRING_VARNAME '}'
			{ $$ = zend_ast_create_ex(ZEND_AST_VAR, ZEND_ENCAPS_VAR_DOLLAR_CURLY, $2); }
// clear, ${abc}[expr] 原来这里也可以用表达式（测试过）	
	|	T_DOLLAR_OPEN_CURLY_BRACES T_STRING_VARNAME '[' expr ']' '}'
			{ $$ = zend_ast_create_ex(ZEND_AST_DIM, ZEND_ENCAPS_VAR_DOLLAR_CURLY,
			      zend_ast_create(ZEND_AST_VAR, $2), $4); }
// clear, {$ variable} : 例如"{$ $a}" 测试过
	|	T_CURLY_OPEN variable '}' { $$ = $2; }
;

// ing4, 字符串常量中的变量名的offset
	T_ENCAPSED_AND_WHITESPACE 也能识别，为什么呢？
	负数和数字居然要写两个规则，
	带引号的子串会怎么识别？？
	其他地方的负数也一样吗？
encaps_var_offset:
// 不带引号的字串
		T_STRING			{ $$ = $1; }
// 数字， T_NUM_STRING 只有这里用到
	|	T_NUM_STRING		{ $$ = $1; }
// 负数 
	|	'-' T_NUM_STRING 	{ $$ = zend_negate_num_string($2); }
// 基本变量 （测试过）
	|	T_VARIABLE			{ $$ = zend_ast_create(ZEND_AST_VAR, $1); }
;

// clear, 内部函数，原来如此！这7个是底层函数, 在脚本解析时直接识别
	// isset, empty, include, include_once, eval, require, require_once
	// 底层函数在传入参数不对时直接抛解析错误，而不是参数错误
		// Parse error: syntax error, unexpected token ","
		// Fatal error: Uncaught ArgumentCountError: array_key_exists() expects exactly 2 arguments
internal_functions_in_yacc:
// clear, isset 参数单独定义
		T_ISSET '(' isset_variables possible_comma ')' { $$ = $3; }
	|	T_EMPTY '(' expr ')' { $$ = zend_ast_create(ZEND_AST_EMPTY, $3); }
// clear, include 和 require 可以不加括号。
	|	T_INCLUDE expr
			{ $$ = zend_ast_create_ex(ZEND_AST_INCLUDE_OR_EVAL, ZEND_INCLUDE, $2); }
	|	T_INCLUDE_ONCE expr
			{ $$ = zend_ast_create_ex(ZEND_AST_INCLUDE_OR_EVAL, ZEND_INCLUDE_ONCE, $2); }
	|	T_EVAL '(' expr ')'
			{ $$ = zend_ast_create_ex(ZEND_AST_INCLUDE_OR_EVAL, ZEND_EVAL, $3); }
	|	T_REQUIRE expr
			{ $$ = zend_ast_create_ex(ZEND_AST_INCLUDE_OR_EVAL, ZEND_REQUIRE, $2); }
	|	T_REQUIRE_ONCE expr
			{ $$ = zend_ast_create_ex(ZEND_AST_INCLUDE_OR_EVAL, ZEND_REQUIRE_ONCE, $2); }
;

// clear, 多个expr 只在 isset() 函数参数中引用
isset_variables:
		isset_variable { $$ = $1; }
	// 多个时，递归调用
	|	isset_variables ',' isset_variable
			{ $$ = zend_ast_create(ZEND_AST_AND, $1, $3); }
;

// clear, expr触发内部逻辑, 只有 isset_variables 引用
isset_variable:
		expr { $$ = zend_ast_create(ZEND_AST_ISSET, $1); }
;

%%

// 优化bison，提供更好的token 描述
/* Over-ride Bison formatting routine to give better token descriptions.
   Copy to YYRES the contents of YYSTR for use in yyerror.
   YYSTR is taken from yytname, from the %token declaration.
   If YYRES is null, do not copy; instead, return the length of what
   the result would have been.  */
static YYSIZE_T zend_yytnamerr(char *yyres, const char *yystr)
{
	const char *toktype = yystr;
	size_t toktype_len = strlen(toktype);

	/* CG(parse_error) states:
	 * 0 => yyres = NULL, yystr is the unexpected token
	 * 1 => yyres = NULL, yystr is one of the expected tokens
	 * 2 => yyres != NULL, yystr is the unexpected token
	 * 3 => yyres != NULL, yystr is one of the expected tokens
	 */
	if (yyres && CG(parse_error) < 2) {
		CG(parse_error) = 2;
	}

	if (CG(parse_error) % 2 == 0) {
		/* The unexpected token */
		char buffer[120];
		const unsigned char *tokcontent, *tokcontent_end;
		size_t tokcontent_len;

		CG(parse_error)++;

		if (LANG_SCNG(yy_text)[0] == 0 &&
			LANG_SCNG(yy_leng) == 1 &&
			strcmp(toktype, "\"end of file\"") == 0) {
			if (yyres) {
				yystpcpy(yyres, "end of file");
			}
			return sizeof("end of file")-1;
		}

		/* Prevent the backslash getting doubled in the output (eugh) */
		if (strcmp(toktype, "\"'\\\\'\"") == 0) {
			if (yyres) {
				yystpcpy(yyres, "token \"\\\"");
			}
			return sizeof("token \"\\\"")-1;
		}

		/* We used "amp" as a dummy label to avoid a duplicate token literal warning. */
		if (strcmp(toktype, "\"amp\"") == 0) {
			if (yyres) {
				yystpcpy(yyres, "token \"&\"");
			}
			return sizeof("token \"&\"")-1;
		}

		/* Avoid unreadable """ */
		/* "'" would theoretically be just as bad, but is never currently parsed as a separate token */
		if (strcmp(toktype, "'\"'") == 0) {
			if (yyres) {
				yystpcpy(yyres, "double-quote mark");
			}
			return sizeof("double-quote mark")-1;
		}

		/* Strip off the outer quote marks */
		if (toktype_len >= 2 && *toktype == '"') {
			toktype++;
			toktype_len -= 2;
		}

		/* If the token always has one form, the %token line should have a single-quoted name */
		/* The parser rules also include single-character un-named tokens which will be single-quoted here */
		/* We re-format this with double quotes here to ensure everything's consistent */
		if (toktype_len > 0 && *toktype == '\'') {
			if (yyres) {
				snprintf(buffer, sizeof(buffer), "token \"%.*s\"", (int)toktype_len-2, toktype+1);
				yystpcpy(yyres, buffer);
			}
			return toktype_len + sizeof("token ")-1;
		}

		/* Fetch the content of the last seen token from global lexer state */
		tokcontent = LANG_SCNG(yy_text);
		tokcontent_len = LANG_SCNG(yy_leng);

		/* For T_BAD_CHARACTER, the content probably won't be a printable char */
		/* Also, "unexpected invalid character" sounds a bit redundant */
		if (tokcontent_len == 1 && strcmp(yystr, "\"invalid character\"") == 0) {
			if (yyres) {
				snprintf(buffer, sizeof(buffer), "character 0x%02hhX", *tokcontent);
				yystpcpy(yyres, buffer);
			}
			return sizeof("character 0x00")-1;
		}

		/* Truncate at line end to avoid messing up log formats */
		tokcontent_end = memchr(tokcontent, '\n', tokcontent_len);
		if (tokcontent_end != NULL) {
			tokcontent_len = (tokcontent_end - tokcontent);
		}

		/* Try to be helpful about what kind of string was found, before stripping the quotes */
		if (tokcontent_len > 0 && strcmp(yystr, "\"quoted string\"") == 0) {
			if (*tokcontent == '"') {
				toktype = "double-quoted string";
				toktype_len = sizeof("double-quoted string")-1;
			}
			else if (*tokcontent == '\'') {
				toktype = "single-quoted string";
				toktype_len = sizeof("single-quoted string")-1;
			}
		}

		/* For quoted strings, strip off another layer of quotes to avoid putting quotes inside quotes */
		if (tokcontent_len > 0 && (*tokcontent == '\'' || *tokcontent=='"'))  {
			tokcontent++;
			tokcontent_len--;
		}
		if (tokcontent_len > 0 && (tokcontent[tokcontent_len-1] == '\'' || tokcontent[tokcontent_len-1] == '"'))  {
			tokcontent_len--;
		}

		/* Truncate to 30 characters and add a ... */
		if (tokcontent_len > 30 + sizeof("...")-1) {
			if (yyres) {
				snprintf(buffer, sizeof(buffer), "%.*s \"%.*s...\"", (int)toktype_len, toktype, 30, tokcontent);
				yystpcpy(yyres, buffer);
			}
			return toktype_len + 30 + sizeof(" \"...\"")-1;
		}

		if (yyres) {
			snprintf(buffer, sizeof(buffer), "%.*s \"%.*s\"", (int)toktype_len, toktype, (int)tokcontent_len, tokcontent);
			yystpcpy(yyres, buffer);
		}
		return toktype_len + tokcontent_len + sizeof(" \"\"")-1;
	}

	/* One of the expected tokens */

	/* Prevent the backslash getting doubled in the output (eugh) */
	if (strcmp(toktype, "\"'\\\\'\"") == 0) {
		if (yyres) {
			yystpcpy(yyres, "\"\\\"");
		}
		return sizeof("\"\\\"")-1;
	}

	/* Strip off the outer quote marks */
	if (toktype_len >= 2 && *toktype == '"') {
		toktype++;
		toktype_len -= 2;
	}

	if (yyres) {
		YYSIZE_T yyn = 0;

		for (; yyn < toktype_len; ++yyn) {
			/* Replace single quotes with double for consistency */
			if (toktype[yyn] == '\'') {
				yyres[yyn] = '"';
			}
			else {
				yyres[yyn] = toktype[yyn];
			}
		}
		yyres[toktype_len] = '\0';
	}

	return toktype_len;
}
