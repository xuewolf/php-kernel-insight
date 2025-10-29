# phpdbg 打印opcode的流程


## 从收到指令到打印opcode

# phpdbg.c
{'p', 2, "show opcodes"},
case 'p':
-> print_opline_func = php_optarg;
-> phpdbg_print_opcodes(print_opline_func)

# phpdbg_print.h 里面有两个函数
void phpdbg_print_opcodes(const char *function);
void phpdbg_print_opline(zend_execute_data *execute_data, bool ignore_flags);

# phpdbg_print.c 里面有一系列函数

# 指令参考文档
Examples

    prompt>  print class \my\class
    prompt>  p c \my\class
    Print the instructions for the methods in \my\class

    prompt>  print method \my\class::method
    prompt>  p m \my\class::method
    Print the instructions for \my\class::method

    prompt>  print func .getSomething
    prompt>  p f .getSomething
    Print the instructions for ::getSomething in the active scope

    prompt>  print func my_function
    prompt>  p f my_function
    Print the instructions for the global function my_function

    prompt>  print opline
    prompt>  p o
    Print the instruction for the current opline
	# 打印正在运行的这一行

    prompt>  print exec
    prompt>  p e
    Print the instructions for the execution context
	# 打印正在执行的上下文

    prompt>  print stack
    prompt>  p s
    Print the instructions for the current stack
	# 不带参数默认是这个，打印堆栈


# 主要入口 phpdbg_print_opcodes
如果沒有参数 -> phpdbg_print_opcodes_main
如果有一个参数，默认是函数名 ->  phpdbg_print_opcodes_function
phpdbg_print_opcodes_class
phpdbg_print_opcodes_method

phpdbg_print_opcodes_main 只有一行代码-> 
phpdbg_print_function_helper((zend_function *) PHPDBG_G(ops));

# phpdbg_print_function_helper 也在同一个文件中
-> Zend\Optimizer\zend_dump.c : zend_dump_op_array
	ZEND_API void zend_dump_op_array(const zend_op_array *op_array, uint32_t dump_flags, const char *msg, const void *data)
	# 居然还有其他地方也需要用到 zend_dump_op_array 打印操作码
	
-> 这里直接调用 fprintf 进行打印了（到头了）
	lines=%d, args=%d, vars=%d, tmps=%d 都在这里了

# zend_compile.h 中
#define ZEND_USER_FUNCTION			2


# 紧接上面，PHPDBG_G(ops) 的产生，追踪 PHPDBG_G(ops)
# phpdbg_prompt.c 
int phpdbg_compile_stdin(zend_string *code) {
PHPDBG_G(ops) = zend_compile_string(code, "Standard input code", ZEND_COMPILE_POSITION_AFTER_OPEN_TAG);

# phpdbg_compile_stdin 被调用
<- phpdbg.c:  phpdbg_compile_stdin(backup_phpdbg_compile);
	# 这个是入口，找到源头就在这里 

zend_op_array 在zend_compile.h中定义

---------------------------------------------------------------------
打印opcode 流程, 需要对照Opcode的生成
zend_dump_op_array
->zend_dump_op_line
->zend_dump_op

参考 zend_vm_opcodes.h 中的操作码定义可以看到，这里已经把确实的操作码打印出来了

---------------------------------------------------------------------


--

# 下面这个是二级指令p e，p o ,p s等

# phpdbg_print.c
# PHPDBG_PRINT(opline)

#define PHPDBG_PRINT(name) PHPDBG_COMMAND(print_##name) 
	# PHPDBG_PRINT 是 PHPDBG_COMMAND 的别名
它应该是在 phpdbg_prompt.c 里定义的 
PHPDBG_COMMAND_D(print,     "print something",                          'p', phpdbg_print_commands, "|*c", 0),

->
应该是调用 phpdbg_print.c ： phpdbg_command_t phpdbg_print_commands[]
	PHPDBG_PRINT_COMMAND_D(exec,       "print out the instructions in the main execution context", 'e', print_exec,   NULL, 0, PHPDBG_ASYNC_SAFE),

-> 再转发 到 PHPDBG_COMMAND_D_EXP
#define PHPDBG_PRINT_COMMAND_D(f, h, a, m, l, s, flags) \
	PHPDBG_COMMAND_D_EXP(f, h, a, m, l, s, &phpdbg_prompt_commands[8], flags)

-> phpdbg_cmd.h
#define PHPDBG_COMMAND_D_EXP(name, tip, alias, handler, children, args, parent, flags) \
	{PHPDBG_STRL(#name), tip, sizeof(tip)-1, alias, phpdbg_do_##handler, children, args, parent, flags}

这时候 handler 是前面的 print_exec 拼起来是 phpdbg_do_print_exec？
这个调用流程是怎么走的？




phpdbg_cmd.c 有命令对应的具体业务逻辑
