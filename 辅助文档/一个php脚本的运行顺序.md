# 执行程序 php-cgi ，最开始调用：/sapi/cli/cli_win32.c  只有两行代码：
	#define PHP_CLI_WIN32_NO_CONSOLE 1
	#include "php_cli.c"

main : /sapi/cli/php_cli.c
  sapi_module->startup = php_cli_startup : /sapi/cli/php_cli.c  
    php_module_startup : /main/main.c
      zend_startup : /Zend/zend.c
		# 1. 开始cpu管理
		zend_cpu_startup();
		# 2. 开始内存管理
		start_memory_manager : /Zend/zend_alloc.c
			# 创建全局内存管理heap
			alloc_globals_ctor ：/Zend/zend_alloc.c
				# 初始化内存管理
				zend_mm_init();
	    # n. 初始化虚拟机的流程
        zend_vm_init : /Zend/zend_vm_execute.h	
		  execute_ex : /Zend/zend_vm_execute.h 
		  
  do_cli : /sapi/cli/php_cli.c
    php_request_startup : /main/main.c
	  zend_activate : /Zend/zend.c
	    init_compiler : /Zend/zend_compile.c
	    init_executor : /Zend/zend_execute_API.c
		startup_scanner : /Zend/zend_language_scanner.l	
	  sapi_activate : /main/SAPI.c
	  zend_activate_modules : /Zend/zend_API.c
	# 返回bool型，所以没有运行结果，所有业务包括打印等都是在过程中完成的
    php_execute_script : /main/main.c
	  zend_execute_scripts : /Zend/zend.c
		# 相对于编译来说，语法分析是个特别简单的事儿
	    zend_compile_file = compile_file : /Zend/zend_language_scanner.l
		  # 这里是 编译相关的业务逻辑，返回opcode
		  zend_compile : /Zend/zend_language_scanner.l 
			zend_compile_top_stmt : /Zend/zend_compile.c
			pass_two(op_array)
		# 这个是执行，执行完后就卸载内存进行清理工作了,wow, 这里就用到虚拟机了
		zend_execute(op_array) : /Zend/zend_vm_execute.h
		  zend_vm_stack_push_call_frame(call_info,(zend_function*)op_array, 0, object_or_called_scope);
		  # 执行php脚本的流程， 64731-52443=12288行代码
		  zend_execute_ex = execute_ex : /Zend/zend_vm_execute.h 
		  
		  
zend_execute(op_array) : /Zend/zend_vm_execute.h
  execute_data = zend_vm_stack_push_call_frame : /Zend/zend_execute.h
  # 最后被执行的不是 opcode，是execute_data
  zend_execute_ex(execute_data) = execute_ex : /Zend/zend_vm_execute.h 


# zend_vm 并不如想象中的复杂，本以为里面会有特别多的优化处理，让人眼花缭乱，实际上并没有，只是分各种情况处理操作码而已。
	# VM 之所以叫 VM不是因为它特别复杂，有特别多的机器接口或底层优化，而是因为它执行操作码
	# 由于执行自己的指令集，所以才叫VM


$a = [];clone($a); 报错 Fatal error: Uncaught Error: __clone method called on non-object
__clone method called on non-object 这个报错只有在vm运行时才有，所以可以证明用到了vm

zend_vm_gen.php 用来生成 zend_vm_execute.h
操作码执行方式直接看 zend_vm_execute.h 也可以

zend_execute.c主要用来被VM调用，执行业务逻辑



宏里面用 do-while(0) 是为了用它的的括号，把代码括起来比较方便
c 语言程序里这样写是因为用到临时变量，方便临时变量快速释放掉？