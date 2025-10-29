
# zend_vm_opcodes.h 里面放了所有的opcode,一共 202个
	确实打印出来的opcode在这里都能找到了
	从 zend_ast 转换成 opline 应该是个很复杂的过程，就是compile
/*
    This script creates zend_vm_execute.h and zend_vm_opcodes.{h,c}
    from existing zend_vm_def.h and zend_vm_execute.skl
*/
zend_vm_gen.php 利用 zend_vm_def.h and zend_vm_execute.skl 生成 zend_vm_execute.h and zend_vm_opcodes.{h,c}


# compile
	->	
		zend_compile_ 开头的函数，语法分析在这里了		
		这些方法应该是做一些预处理工作
		
搜索 "static.+zend_compile" （2个文件中匹配到136次，总计查找1656次）
  D:\www\程序员\php-8.2.5-src\php-8.2.5-src\Zend\zend.c （匹配2次）
	行  700: static void compiler_globals_ctor(zend_compiler_globals *compiler_globals) /* {{{ */
	行  737: static void compiler_globals_dtor(zend_compiler_globals *compiler_globals) /* {{{ */
  D:\www\程序员\php-8.2.5-src\php-8.2.5-src\Zend\zend_compile.c （匹配134次）
	行    95: static zend_op *zend_compile_var(znode *result, zend_ast *ast, uint32_t type, bool by_ref);
	行    97: static void zend_compile_expr(znode *result, zend_ast *ast);
	行    98: static void zend_compile_stmt(zend_ast *ast);
	行    99: static void zend_compile_assign(znode *result, zend_ast *ast);
	行  2341: static void zend_compile_memoized_expr(znode *result, zend_ast *expr) /* {{{ */
	行  2596: static void zend_compile_class_ref(znode *result, zend_ast *name_ast, uint32_t fetch_flags) /* {{{ */
	行  2682: static zend_op *zend_compile_simple_var_no_cv(znode *result, zend_ast *ast, uint32_t type, bool delayed) /* {{{ */
	行  2748: static zend_op *zend_compile_simple_var(znode *result, zend_ast *ast, uint32_t type, bool delayed) /* {{{ */
	行  2862: static zend_op *zend_compile_dim(znode *result, zend_ast *ast, uint32_t type, bool by_ref) /* {{{ */
	行  2944: static zend_op *zend_compile_prop(znode *result, zend_ast *ast, uint32_t type, bool by_ref) /* {{{ */
	行  2955: static zend_op *zend_compile_static_prop(znode *result, zend_ast *ast, uint32_t type, bool by_ref, bool delayed) /* {{{ */
	行  3046: static void zend_compile_list_assign(
	行  3184: static void zend_compile_expr_with_potential_assign_to_self(
	行  3200: static void zend_compile_assign(znode *result, zend_ast *ast) /* {{{ */
	行  3295: static void zend_compile_assign_ref(znode *result, zend_ast *ast) /* {{{ */
	行  3369: static void zend_compile_compound_assign(znode *result, zend_ast *ast) /* {{{ */
	行  3464: static uint32_t zend_compile_args(
	行  3693: static bool zend_compile_call_common(znode *result, zend_ast *args_ast, zend_function *fbc, uint32_t lineno) /* {{{ */
	行  3736: static bool zend_compile_function_name(znode *name_node, zend_ast *name_ast) /* {{{ */
	行  3749: static void zend_compile_ns_call(znode *result, znode *name_node, zend_ast *args_ast, uint32_t lineno) /* {{{ */
	行  3762: static void zend_compile_dynamic_call(znode *result, znode *name_node, zend_ast *args_ast, uint32_t lineno) /* {{{ */
	行  3809: static zend_result zend_compile_func_strlen(znode *result, zend_ast_list *args) /* {{{ */
	行  3829: static zend_result zend_compile_func_typecheck(znode *result, zend_ast_list *args, uint32_t type) /* {{{ */
	行  3849: static zend_result zend_compile_func_is_scalar(znode *result, zend_ast_list *args) /* {{{ */
	行  3864: static zend_result zend_compile_func_cast(znode *result, zend_ast_list *args, uint32_t type) /* {{{ */
	行  3884: static zend_result zend_compile_func_defined(znode *result, zend_ast_list *args) /* {{{ */
	行  3916: static zend_result zend_compile_func_chr(znode *result, zend_ast_list *args) /* {{{ */
	行  3934: static zend_result zend_compile_func_ord(znode *result, zend_ast_list *args) /* {{{ */
	行  3990: static void zend_compile_init_user_func(zend_ast *name_ast, uint32_t num_args, zend_string *orig_func_name) /* {{{ */
	行  4009: static zend_result zend_compile_func_cufa(znode *result, zend_ast_list *args, zend_string *lcname) /* {{{ */
	行  4062: static zend_result zend_compile_func_cuf(znode *result, zend_ast_list *args, zend_string *lcname) /* {{{ */
	行  4088: static void zend_compile_assert(znode *result, zend_ast_list *args, zend_string *name, zend_function *fbc, uint32_t lineno) /* {{{ */
	行  4138: static zend_result zend_compile_func_in_array(znode *result, zend_ast_list *args) /* {{{ */
	行  4223: static zend_result zend_compile_func_count(znode *result, zend_ast_list *args, zend_string *lcname) /* {{{ */
	行  4240: static zend_result zend_compile_func_get_class(znode *result, zend_ast_list *args) /* {{{ */
	行  4258: static zend_result zend_compile_func_get_called_class(znode *result, zend_ast_list *args) /* {{{ */
	行  4269: static zend_result zend_compile_func_gettype(znode *result, zend_ast_list *args) /* {{{ */
	行  4283: static zend_result zend_compile_func_num_args(znode *result, zend_ast_list *args) /* {{{ */
	行  4294: static zend_result zend_compile_func_get_args(znode *result, zend_ast_list *args) /* {{{ */
	行  4305: static zend_result zend_compile_func_array_key_exists(znode *result, zend_ast_list *args) /* {{{ */
	行  4321: static zend_result zend_compile_func_array_slice(znode *result, zend_ast_list *args) /* {{{ */
	行  4440: static void zend_compile_call(znode *result, zend_ast *ast, uint32_t type) /* {{{ */
	行  4514: static void zend_compile_method_call(znode *result, zend_ast *ast, uint32_t type) /* {{{ */
	行  4606: static void zend_compile_static_call(znode *result, zend_ast *ast, uint32_t type) /* {{{ */
	行  4674: static void zend_compile_class_decl(znode *result, zend_ast *ast, bool toplevel);
	行  4676: static void zend_compile_new(znode *result, zend_ast *ast) /* {{{ */
	行  4707: static void zend_compile_clone(znode *result, zend_ast *ast) /* {{{ */
	行  4718: static void zend_compile_global_var(zend_ast *ast) /* {{{ */
	行  4755: static void zend_compile_static_var_common(zend_string *var_name, zval *value, uint32_t mode) /* {{{ */
	行  4778: static void zend_compile_static_var(zend_ast *ast) /* {{{ */
	行  4794: static void zend_compile_unset(zend_ast *ast) /* {{{ */
	行  4933: static void zend_compile_return(zend_ast *ast) /* {{{ */
	行  4992: static void zend_compile_echo(zend_ast *ast) /* {{{ */
	行  5005: static void zend_compile_throw(znode *result, zend_ast *ast) /* {{{ */
	行  5022: static void zend_compile_break_continue(zend_ast *ast) /* {{{ */
	行  5160: static void zend_compile_goto(zend_ast *ast) /* {{{ */
	行  5177: static void zend_compile_label(zend_ast *ast) /* {{{ */
	行  5196: static void zend_compile_while(zend_ast *ast) /* {{{ */
	行  5220: static void zend_compile_do_while(zend_ast *ast) /* {{{ */
	行  5242: static void zend_compile_expr_list(znode *result, zend_ast *ast) /* {{{ */
	行  5264: static void zend_compile_for(zend_ast *ast) /* {{{ */
	行  5298: static void zend_compile_foreach(zend_ast *ast) /* {{{ */
	行  5390: static void zend_compile_if(zend_ast *ast) /* {{{ */
	行  5495: static void zend_compile_switch(zend_ast *ast) /* {{{ */
	行  5673: static void zend_compile_match(znode *result, zend_ast *ast)
	行  5859: static void zend_compile_try(zend_ast *ast) /* {{{ */
	行  6107: static void zend_compile_declare(zend_ast *ast) /* {{{ */
	行  6171: static void zend_compile_stmt_list(zend_ast *ast) /* {{{ */
	行  6206: static zend_type zend_compile_single_typename(zend_ast *ast)
	行  6356: static zend_type zend_compile_typename(
	行  6593: static void zend_compile_attributes(
	行  6709: static void zend_compile_params(zend_ast *ast, zend_ast *return_type_ast, uint32_t fallback_return_type) /* {{{ */
	行  6988: static void zend_compile_closure_binding(znode *closure, zend_op_array *op_array, zend_ast *uses_ast) /* {{{ */
	行  7134: static void zend_compile_closure_uses(zend_ast *ast) /* {{{ */
	行  7168: static void zend_compile_implicit_closure_uses(closure_info *info)
	行  7326: static void zend_compile_func_decl(znode *result, zend_ast *ast, bool toplevel) /* {{{ */
	行  7463: static void zend_compile_prop_decl(zend_ast *ast, zend_ast *type_ast, uint32_t flags, zend_ast *attr_ast) /* {{{ */
	行  7576: static void zend_compile_prop_group(zend_ast *ast) /* {{{ */
	行  7600: static void zend_compile_class_const_decl(zend_ast *ast, uint32_t flags, zend_ast *attr_ast) /* {{{ */
	行  7637: static void zend_compile_class_const_group(zend_ast *ast) /* {{{ */
	行  7646: static void zend_compile_method_ref(zend_ast *ast, zend_trait_method_reference *method_ref) /* {{{ */
	行  7661: static void zend_compile_trait_precedence(zend_ast *ast) /* {{{ */
	行  7682: static void zend_compile_trait_alias(zend_ast *ast) /* {{{ */
	行  7706: static void zend_compile_use_trait(zend_ast *ast) /* {{{ */
	行  7749: static void zend_compile_implements(zend_ast *ast) /* {{{ */
	行  7790: static void zend_compile_enum_backing_type(zend_class_entry *ce, zend_ast *enum_backing_type_ast)
	行  7810: static void zend_compile_class_decl(znode *result, zend_ast *ast, bool toplevel) /* {{{ */
	行  8002: static void zend_compile_enum_case(zend_ast *ast)
	行  8110: static void zend_compile_use(zend_ast *ast) /* {{{ */
	行  8189: static void zend_compile_group_use(zend_ast *ast) /* {{{ */
	行  8209: static void zend_compile_const_decl(zend_ast *ast) /* {{{ */
	行  8252: static void zend_compile_namespace(zend_ast *ast) /* {{{ */
	行  8315: static void zend_compile_halt_compiler(zend_ast *ast) /* {{{ */
	行  8692: static void zend_compile_binary_op(znode *result, zend_ast *ast) /* {{{ */
	行  8785: static void zend_compile_greater(znode *result, zend_ast *ast) /* {{{ */
	行  8811: static void zend_compile_unary_op(znode *result, zend_ast *ast) /* {{{ */
	行  8830: static void zend_compile_unary_pm(znode *result, zend_ast *ast) /* {{{ */
	行  8852: static void zend_compile_short_circuiting(znode *result, zend_ast *ast) /* {{{ */
	行  8907: static void zend_compile_post_incdec(znode *result, zend_ast *ast) /* {{{ */
	行  8934: static void zend_compile_pre_incdec(znode *result, zend_ast *ast) /* {{{ */
	行  8963: static void zend_compile_cast(znode *result, zend_ast *ast) /* {{{ */
	行  8982: static void zend_compile_shorthand_conditional(znode *result, zend_ast *ast) /* {{{ */
	行  9007: static void zend_compile_conditional(znode *result, zend_ast *ast) /* {{{ */
	行  9067: static void zend_compile_coalesce(znode *result, zend_ast *ast) /* {{{ */
	行  9099: static void zend_compile_assign_coalesce(znode *result, zend_ast *ast) /* {{{ */
	行  9202: static void zend_compile_print(znode *result, zend_ast *ast) /* {{{ */
	行  9218: static void zend_compile_exit(znode *result, zend_ast *ast) /* {{{ */
	行  9239: static void zend_compile_yield(znode *result, zend_ast *ast) /* {{{ */
	行  9273: static void zend_compile_yield_from(znode *result, zend_ast *ast) /* {{{ */
	行  9290: static void zend_compile_instanceof(znode *result, zend_ast *ast) /* {{{ */
	行  9322: static void zend_compile_include_or_eval(znode *result, zend_ast *ast) /* {{{ */
	行  9338: static void zend_compile_isset_or_empty(znode *result, zend_ast *ast) /* {{{ */
	行  9418: static void zend_compile_silence(znode *result, zend_ast *ast) /* {{{ */
	行  9437: static void zend_compile_shell_exec(znode *result, zend_ast *ast) /* {{{ */
	行  9455: static void zend_compile_array(znode *result, zend_ast *ast) /* {{{ */
	行  9534: static void zend_compile_const(znode *result, zend_ast *ast) /* {{{ */
	行  9584: static void zend_compile_class_const(znode *result, zend_ast *ast) /* {{{ */
	行  9621: static void zend_compile_class_name(znode *result, zend_ast *ast) /* {{{ */
	行  9648: static zend_op *zend_compile_rope_add_ex(zend_op *opline, znode *result, uint32_t num, znode *elem_node) /* {{{ */
	行  9665: static zend_op *zend_compile_rope_add(znode *result, uint32_t num, znode *elem_node) /* {{{ */
	行  9684: static void zend_compile_encaps_list(znode *result, zend_ast *ast) /* {{{ */
	行  9809: static void zend_compile_magic_const(znode *result, zend_ast *ast) /* {{{ */
	行  9847: static void zend_compile_const_expr_class_const(zend_ast **ast_ptr) /* {{{ */
	行  9882: static void zend_compile_const_expr_class_name(zend_ast **ast_ptr) /* {{{ */
	行  9910: static void zend_compile_const_expr_const(zend_ast **ast_ptr) /* {{{ */
	行  9935: static void zend_compile_const_expr_magic_const(zend_ast **ast_ptr) /* {{{ */
	行  9947: static void zend_compile_const_expr_new(zend_ast **ast_ptr)
	行  9972: static void zend_compile_const_expr_args(zend_ast **ast_ptr)
	行  9999: static void zend_compile_const_expr(zend_ast **ast_ptr, void *context) /* {{{ */
	行 10091: static void zend_compile_stmt(zend_ast *ast) /* {{{ */
	行 10208: static void zend_compile_expr_inner(znode *result, zend_ast *ast) /* {{{ */
	行 10352: static void zend_compile_expr(znode *result, zend_ast *ast)
	行 10359: static zend_op *zend_compile_var_inner(znode *result, zend_ast *ast, uint32_t type, bool by_ref)
	行 10397: static zend_op *zend_compile_var(znode *result, zend_ast *ast, uint32_t type, bool by_ref) /* {{{ */


# 创建 zend_ast 的方法

zend_ast_create
zend_ast_create_znode
zend_ast_create_zval_int
zend_ast_create_constant
zend_ast_create_decl
zend_ast_create_from_va_list


zend_ast_create_0
zend_ast_create_1
zend_ast_create_2
zend_ast_create_3
zend_ast_create_4
zend_ast_create_5
# 其他方法虽然叫 zend_ast_create 但不直接创建 zend_ast

# zend_ast_create() 方法主要是在 zend_language_parser.y 里面定义的
	zend_ast_create_ex 方法类似
zend_language_parser.y

ZEND_API zend_ast *zend_ast_create_ex(zend_ast_kind kind, zend_ast_attr attr, ...)
ZEND_API zend_ast *zend_ast_create(zend_ast_kind kind, ...) {

# 相关创建方法都在zend_ast.h
	行 219: ZEND_API zend_ast * ZEND_FASTCALL zend_ast_create_zval_with_lineno(zval *zv, uint32_t lineno);
	行 220: ZEND_API zend_ast * ZEND_FASTCALL zend_ast_create_zval_ex(zval *zv, zend_ast_attr attr);
	行 221: ZEND_API zend_ast * ZEND_FASTCALL zend_ast_create_zval(zval *zv);
	行 222: ZEND_API zend_ast * ZEND_FASTCALL zend_ast_create_zval_from_str(zend_string *str);
	行 223: ZEND_API zend_ast * ZEND_FASTCALL zend_ast_create_zval_from_long(zend_long lval);
	行 225: ZEND_API zend_ast * ZEND_FASTCALL zend_ast_create_constant(zend_string *name, zend_ast_attr attr);
	行 226: ZEND_API zend_ast * ZEND_FASTCALL zend_ast_create_class_const_or_name(zend_ast *class_name, zend_ast *name);
	行 238: ZEND_API zend_ast * ZEND_FASTCALL zend_ast_create_0(zend_ast_kind kind);
	行 239: ZEND_API zend_ast * ZEND_FASTCALL zend_ast_create_1(zend_ast_kind kind, zend_ast *child);
	行 240: ZEND_API zend_ast * ZEND_FASTCALL zend_ast_create_2(zend_ast_kind kind, zend_ast *child1, zend_ast *child2);
	行 241: ZEND_API zend_ast * ZEND_FASTCALL zend_ast_create_3(zend_ast_kind kind, zend_ast *child1, zend_ast *child2, zend_ast *child3);
	行 242: ZEND_API zend_ast * ZEND_FASTCALL zend_ast_create_4(zend_ast_kind kind, zend_ast *child1, zend_ast *child2, zend_ast *child3, zend_ast *child4);
	行 243: ZEND_API zend_ast * ZEND_FASTCALL zend_ast_create_5(zend_ast_kind kind, zend_ast *child1, zend_ast *child2, zend_ast *child3, zend_ast *child4, zend_ast *child5);
	行 245: static zend_always_inline zend_ast * zend_ast_create_ex_0(zend_ast_kind kind, zend_ast_attr attr) {
	行 246: 	zend_ast *ast = zend_ast_create_0(kind);
	行 250: static zend_always_inline zend_ast * zend_ast_create_ex_1(zend_ast_kind kind, zend_ast_attr attr, zend_ast *child) {
	行 251: 	zend_ast *ast = zend_ast_create_1(kind, child);
	行 255: static zend_always_inline zend_ast * zend_ast_create_ex_2(zend_ast_kind kind, zend_ast_attr attr, zend_ast *child1, zend_ast *child2) {
	行 256: 	zend_ast *ast = zend_ast_create_2(kind, child1, child2);
	行 260: static zend_always_inline zend_ast * zend_ast_create_ex_3(zend_ast_kind kind, zend_ast_attr attr, zend_ast *child1, zend_ast *child2, zend_ast *child3) {
	行 261: 	zend_ast *ast = zend_ast_create_3(kind, child1, child2, child3);
	行 265: static zend_always_inline zend_ast * zend_ast_create_ex_4(zend_ast_kind kind, zend_ast_attr attr, zend_ast *child1, zend_ast *child2, zend_ast *child3, zend_ast *child4) {
	行 266: 	zend_ast *ast = zend_ast_create_4(kind, child1, child2, child3, child4);
	行 270: static zend_always_inline zend_ast * zend_ast_create_ex_5(zend_ast_kind kind, zend_ast_attr attr, zend_ast *child1, zend_ast *child2, zend_ast *child3, zend_ast *child4, zend_ast *child5) {
	行 271: 	zend_ast *ast = zend_ast_create_5(kind, child1, child2, child3, child4, child5);
	行 276: ZEND_API zend_ast * ZEND_FASTCALL zend_ast_create_list_0(zend_ast_kind kind);
	行 277: ZEND_API zend_ast * ZEND_FASTCALL zend_ast_create_list_1(zend_ast_kind kind, zend_ast *child);
	行 278: ZEND_API zend_ast * ZEND_FASTCALL zend_ast_create_list_2(zend_ast_kind kind, zend_ast *child1, zend_ast *child2);
	行 289: ZEND_API zend_ast *zend_ast_create_ex(zend_ast_kind kind, zend_ast_attr attr, ...);
	行 290: ZEND_API zend_ast *zend_ast_create_list(uint32_t init_children, zend_ast_kind kind, ...);
	行 295: ZEND_API zend_ast *zend_ast_create_decl(
	行 355: static zend_always_inline zend_ast *zend_ast_create_binary_op(uint32_t opcode, zend_ast *op0, zend_ast *op1) {
	行 359: zend_ast *zend_ast_create_concat_op(zend_ast *op0, zend_ast *op1);
	行 361: static zend_always_inline zend_ast *zend_ast_create_assign_op(uint32_t opcode, zend_ast *op0, zend_ast *op1) {
	行 364: static zend_always_inline zend_ast *zend_ast_create_cast(uint32_t type, zend_ast *op0) {
	

# 外部调用都在zend_compile.c	
	行  2789: 	zend_ast *assign_ast = zend_ast_create(ZEND_AST_ASSIGN, var_ast,
	行  2790: 		zend_ast_create_znode(value_node));
	行  3362: 	zend_ast *assign_ast = zend_ast_create(ZEND_AST_ASSIGN_REF, var_ast,
	行  3363: 		zend_ast_create_znode(value_node));
	行  4111: 			zend_ast *arg = zend_ast_create_zval_from_str(
	行  4116: 				zend_ast *name = zend_ast_create_zval_from_str(
	行  4118: 				arg = zend_ast_create(ZEND_AST_NAMED_ARG, name, arg);
	行  4748: 			zend_ast_create(ZEND_AST_VAR, zend_ast_create_znode(&name_node)),
	行  7427: 			stmt_ast = zend_ast_create(ZEND_AST_RETURN, stmt_ast);
	行  8014: 	zend_ast *class_name_ast = zend_ast_create_zval(&class_name_zval);
	行  8018: 	zend_ast *case_name_ast = zend_ast_create_zval(&case_name_zval);
	行  8033: 	zend_ast *const_enum_init_ast = zend_ast_create(ZEND_AST_CONST_ENUM_INIT, class_name_ast, case_name_ast, case_value_ast);
	行  8202: 		inline_use = zend_ast_create_list(1, ZEND_AST_USE, use);
	行  9350: 			zend_ast *not_ast = zend_ast_create_ex(ZEND_AST_UNARY_OP, ZEND_BOOL_NOT, var_ast);
	行  9445: 	name_ast = zend_ast_create_zval(&fn_name);
	行  9446: 	args_ast = zend_ast_create_list(1, ZEND_AST_ARG_LIST, expr_ast);
	行  9447: 	call_ast = zend_ast_create(ZEND_AST_CALL, name_ast, args_ast);
	行  9925: 		*ast_ptr = zend_ast_create_zval(&result);
	行  9930: 	*ast_ptr = zend_ast_create_constant(resolved_name,
	行  9943: 	*ast_ptr = zend_ast_create(ZEND_AST_CONSTANT_CLASS);
	行 10052: 		*ast_ptr = zend_ast_create_zval(&ast_zv);
	行 10717: 	*ast_ptr = zend_ast_create_zval(&result);
	
整个解析都围绕着 _zend_ast_kind 来做
	126 个值

# zend_compile.h 里面定义

#define ZEND_INTERNAL_FUNCTION		1
#define ZEND_USER_FUNCTION			2
#define ZEND_EVAL_CODE				4

文件打开之后最开始做了什么，如何把代码变成 zend_zst 的？

_zend_ast zend_ast
_zend_ast_list zend_ast_list
_zend_ast_zval zend_ast_zval
_zend_ast_decl zend_ast_decl

四个结构体

# 会返回 zend_ast 的方法
    D:\www\程序员\php-8.2.5-src\php-8.2.5-src\Zend\zend_ast.h （匹配23次）
	行 217: extern ZEND_API zend_ast_process_t zend_ast_process;
	行 219: ZEND_API zend_ast * ZEND_FASTCALL zend_ast_create_zval_with_lineno(zval *zv, uint32_t lineno);
	行 220: ZEND_API zend_ast * ZEND_FASTCALL zend_ast_create_zval_ex(zval *zv, zend_ast_attr attr);
	行 221: ZEND_API zend_ast * ZEND_FASTCALL zend_ast_create_zval(zval *zv);
	行 222: ZEND_API zend_ast * ZEND_FASTCALL zend_ast_create_zval_from_str(zend_string *str);
	行 223: ZEND_API zend_ast * ZEND_FASTCALL zend_ast_create_zval_from_long(zend_long lval);
	行 225: ZEND_API zend_ast * ZEND_FASTCALL zend_ast_create_constant(zend_string *name, zend_ast_attr attr);
	行 226: ZEND_API zend_ast * ZEND_FASTCALL zend_ast_create_class_const_or_name(zend_ast *class_name, zend_ast *name);
	行 238: ZEND_API zend_ast * ZEND_FASTCALL zend_ast_create_0(zend_ast_kind kind);
	行 239: ZEND_API zend_ast * ZEND_FASTCALL zend_ast_create_1(zend_ast_kind kind, zend_ast *child);
	行 240: ZEND_API zend_ast * ZEND_FASTCALL zend_ast_create_2(zend_ast_kind kind, zend_ast *child1, zend_ast *child2);
	行 241: ZEND_API zend_ast * ZEND_FASTCALL zend_ast_create_3(zend_ast_kind kind, zend_ast *child1, zend_ast *child2, zend_ast *child3);
	行 242: ZEND_API zend_ast * ZEND_FASTCALL zend_ast_create_4(zend_ast_kind kind, zend_ast *child1, zend_ast *child2, zend_ast *child3, zend_ast *child4);
	行 243: ZEND_API zend_ast * ZEND_FASTCALL zend_ast_create_5(zend_ast_kind kind, zend_ast *child1, zend_ast *child2, zend_ast *child3, zend_ast *child4, zend_ast *child5);
	行 276: ZEND_API zend_ast * ZEND_FASTCALL zend_ast_create_list_0(zend_ast_kind kind);
	行 277: ZEND_API zend_ast * ZEND_FASTCALL zend_ast_create_list_1(zend_ast_kind kind, zend_ast *child);
	行 278: ZEND_API zend_ast * ZEND_FASTCALL zend_ast_create_list_2(zend_ast_kind kind, zend_ast *child1, zend_ast *child2);
	行 288: ZEND_API zend_ast *zend_ast_create(zend_ast_kind kind, ...);
	行 289: ZEND_API zend_ast *zend_ast_create_ex(zend_ast_kind kind, zend_ast_attr attr, ...);
	行 290: ZEND_API zend_ast *zend_ast_create_list(uint32_t init_children, zend_ast_kind kind, ...);
	行 293: ZEND_API zend_ast * ZEND_FASTCALL zend_ast_list_add(zend_ast *list, zend_ast *op);
	行 295: ZEND_API zend_ast *zend_ast_create_decl(
	行 303: ZEND_API zend_ast_ref * ZEND_FASTCALL zend_ast_copy(zend_ast *ast);
  D:\www\程序员\php-8.2.5-src\php-8.2.5-src\Zend\zend_compile.h （匹配2次）
	行   98: ZEND_API zend_ast * ZEND_FASTCALL zend_ast_create_znode(znode *node);
	行  835: ZEND_API zend_ast *zend_compile_string_to_ast(
  D:\www\程序员\php-8.2.5-src\php-8.2.5-src\Zend\zend_language_scanner.l （匹配1次）
	行  660: ZEND_API zend_ast *zend_compile_string_to_ast(
	
	
# zend_language_parser.y	
	top_statement_list	{ CG(ast) = $1; (void) zendnerrs; }
	# $1 是什么？它很可能就是整个Php脚本
	
	CG(ast) 是什么，c global 吗？是指全局，还是说他是在外部声明的？
		暂时当成全局变量来理解吧
		
	CG(ast) 到底是如何创建陈zend_ast的？
	
	CG( 1178 次调用
	ZCG( 473 次调用
	
# zend_language_parser.c 和 zend_language_parser.h 
	是在make的时候用 zend_language_parser.y 生成的
	
	
	
zendparse 没有业务逻辑，假设它根本没有业务逻辑
zend_ast_process 又是什么鬼？这个更离谱了，根本没有定义


# 语句格式在 statement 里面定义的, 这个定义的方式不是正则但原理类似，比正则简单
T_FOREACH '(' expr T_AS foreach_variable ')' foreach_statement
			{ $$ = zend_ast_create(ZEND_AST_FOREACH, $3, $5, NULL, $7); }
			
T_FOREACH '(' expr T_AS foreach_variable T_DOUBLE_ARROW foreach_variable ')'
		foreach_statement
			{ $$ = zend_ast_create(ZEND_AST_FOREACH, $3, $7, $5, $9); }
			
原来ast有多个child指的是传入的参数，最多5个	, 上面这个有4个


# php 里ini文件的解释和php脚本的解析一样在底层定义的，
	毕竟用到了php.ini,没有这个不行
	
# 作为语句关键词的token一共有148个，
%token <ident> T_FOREACH       "'foreach'"
%token <ident> T_ENDFOREACH    "'endforeach'"	

# 有些没有用过
%token <ident> T_HALT_COMPILER "'__halt_compiler'"
%token T_UNSET_CAST  "'(unset)'"
%token T_NULLSAFE_OBJECT_OPERATOR "'?->'"

# %type <ast> 是什么,一共42个
	%type 是ast，也就是php语句，由%token 和其他表达式共同组成

# 似乎是用一个叫做 bison 的工具来进行语法分析的？
/* Bison warns on duplicate token literals, so use a different dummy value here.
 * It will be fixed up by zend_yytnamerr() later. */
 
 所以无法看到分析细节了，能看到分析规则也不错，毕竟底层分析想想也就那么回事

# bison 是编译Php必须的工具 
 For a minimal PHP build from Git, you will need autoconf, bison, and re2c. For
a default build, you will additionally need libxml2 and libsqlite3.


