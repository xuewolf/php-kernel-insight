有些东西只能等有c++环境后调试了，比如 ((uint32_t)(-((nTableSize) + (nTableSize)))) 到底是什么

# 关于哈希表
	# 没看到用红黑树，可能是以前的版本用过把，现在的怎么看也只是普通的哈希表。

# 宏（大写函数，macros）的一个重要的特点是逻辑简单，只能有一行代码，
# 多行要用 反斜线 (\) 连接, 这是C语言的规则，不只是宏的

# 宏名称加 _P 是指对指针(pointer)的操作

# 常用术语
# TSRM : Thread Safe Resource Manager
# dtor : destory
# CG : compiled globals
# EG : executed globals
	# define GLOBAL_CONSTANTS_TABLE		EG(zend_constants)
# 函数末尾 _ex : execute
# ptr : pointer
# SCCP : Sparse Conditional Constant Propagation
size_t 是整数吗？

# 常用结构体
# zend_types.h
zval : typedef struct _zval_struct     zval;

# 常用别名 /Zend/zend_execute.c	
# define ZEND_VM_GUARD(name) __asm__(\"#\" #name)
#define FC(member) (CG(file_context).member)
	# FC 是 compile globals 的 file_context

# 指针是内存地址，可以直接转成int使用，每+1 是向后 sizeof(struct) 个字节
	# 所以直接开辟内存空间做列表就很方便
# 指针做加减法的意思是，前移或者后移。加法后移，减法前移。

# 传递指针占用内存小，不需要频繁复制对象

// ZEND_API 
#if defined(__GNUC__) && __GNUC__ >= 4
# define ZEND_API __attribute__ ((visibility("default")))
# define ZEND_DLEXPORT __attribute__ ((visibility("default")))
#else
# define ZEND_API
# define ZEND_DLEXPORT
#endif

// 这些都是提供portablity的宏
ZEND_API zend_never_inline ZEND_COLD ZEND_NORETURN


php_request_startup是准备工作， 是关键入口，main.c 中定义
/sapi/cli/php_cli.c，/sapi/cgi/cgi_main.c，/sapi/litespeed/lsapi_main.c，/sapi/fpm/fpm/fpm_main.c，/sapi/phpdbg/phpdbg.c，/sapi/cli/php_cli_server.c 都调用它运行Php程序

main.c:php_request_startup
	-> php_output_activate();
	-> zend.c:zend_activate()
		-> init_compiler()
			-> 
		-> init_executor()
		-> startup_scanner();
		...
	-> sapi_activate();
	-> zend_signal_activate();

php_execute_script 是执行php 程序 /sapi/cli/php_cli.c， /sapi/fpm/fpm/fpm_main.c，/sapi/cli/php_cli_server.c 都调用它运行php程序


# 目标
	先大概看个全貌，从信息分析上看，确实有必要整体分析一下文件数量，入库统计一下
	
#源码无法直接在windows环境运行
#到底是c还是c++呢，没有class，应该是c吧？
	# 这里有很多static的方法

.c 和.h 是怎么分工的呢？.c是具体实现，.h是外部接口吧？因为.h是用来给外部引用的

# 配置入口 configure.ac

ZEND_API 是什么？

# 最主要的目录
	# main
	# ext，各种扩展全都在里面，包含Php98%的功能
	# zend ?
	
# 启动 cgi 模式一定是从 \sapi\litespeed\lsapi_main.c 开始运行
	因为 parse_opt 方法解析cgi参数，-b,-c等
	引用了 php.h 等一堆东西
	
	lsapi_execute_script 调用 php_execute_script(&file_handle); 运行php代码
	
	static int processReq(void)
	
	有 main 函数
		cli_main 不算主要业务逻辑，
		
	main函数是主要业务逻辑，监听也在这里启动
		if ( php_bind ) {
    bindFd = LSAPI_CreateListenSock( php_bind, 10 );
	
	slow_script_msec 记录慢查询
	
	后面调用 PHP_FUNCTION 调用 php函数 返回结果？
	zend_end_try{
	}
	
	zend_try在zend.h里定义，不是特别常用
		
	# main函数是很重要的，出现次数少，好定位
	
# php_cli.c
	: WinMain 有条件二选一 ，这里应该是开头了
	: main
	-> do_cli
	# 应该这句是执行脚本
	-> php_execute_script(&file_handle);
	
	# 果然，lsapi_main.c 里也用到了
	
	php_execute_script : main/main.c 和 main/php_main.h 里定义
	-> zend_execute_scripts : Zend\zend.c 和 Zend\zend_compile.h 里定义
		# op_array = zend_compile_file(file_handle, type);
			# zend_execute(op_array, retval);
			
	typedef enum {
	  SUCCESS =  0,
	  FAILURE = -1,		/* this MUST stay a negative number, or it may affect functions! */
	} ZEND_RESULT_CODE;
	
	-> zend_compile_file : Zend\zend.c
		zend_compile_file = dtrace_compile_file;
		zend_compile_file = compile_file;
		
	-> compile_file 的实现逻辑没找到，是抽象函数么？
		只有 zend_compile.h 里面有
		ZEND_API zend_op_array *compile_file(zend_file_handle *file_handle, int type);
		# 相关方法
		zend_compile_file 同样没有实现逻辑
		dtrace_compile_file 调用 compile_file
		
		opcache_compile_file 有实现逻辑，ext\opcache\ZendAccelerator.c
			Accelerator /ək'seləreɪtə/ 加速器
		compile_filename 同样没有实现逻辑
		persistent_compile_file 有实现逻辑，opcache\ZendAccelerator.c
		accelerator_orig_compile_file 有实现逻辑，opcache\ZendAccelerator.c
		preload_orig_compile_file ，opcache\ZendAccelerator.c
			preload_orig_compile_file = accelerator_orig_compile_file ?
		file_cache_compile_file	有实现逻辑，opcache\ZendAccelerator.c	
		
		# debug,phar 相关不在里面

		# 所以肯定是在 ext\opcache\ZendAccelerator.c 里调用咯
			# 刚好 ini.opcache 里相关配置生效
			opcache.file_cache_only=1
		
		# ZendAccelerator:3294
		/* Override compiler */
		accelerator_orig_compile_file = zend_compile_file;
		zend_compile_file = persistent_compile_file;
		
		persistent_compile_file 在opcache外部没有调用
	
	-> persistent_compile_file ,opcache\ZendAccelerator.c
		# zend_op_array *persistent_compile_file(zend_file_handle *file_handle, int type)
			接口和 compile_file() 一致
		主要是调用 file_cache_compile_file
	-> file_cache_compile_file ,opcache\ZendAccelerator.c
		# zend_op_array *file_cache_compile_file(zend_file_handle *file_handle, int type)
		
		persistent_script = zend_file_cache_script_load(file_handle);
		如果失败，
		# 这里到 compile 了
		persistent_script = opcache_compile_file(file_handle, type, &op_array);
		
		return zend_accel_load_script(persistent_script, 1);
	
	-> opcache_compile_file
		# static zend_persistent_script *opcache_compile_file(zend_file_handle *file_handle, int type, zend_op_array **op_array_p)
		
		主要逻辑
		*op_array_p = accelerator_orig_compile_file(file_handle, type);
		
	-> 	accelerator_orig_compile_file
		accelerator_orig_compile_file = zend_compile_file
		完了，死循环，这个太坑了！
		
		preload_compile_file		
			preload_orig_compile_file
				preload_orig_compile_file = accelerator_orig_compile_file;
					叕死循环
		死透了
		
	# 如果排除opcache呢，毕竟opcache只是可选扩展，没有它也能运行php
	

	# 线索方法 zend_compile_stmt，编译statement, 引用17次
		# 

	# 引用关系完全和想象一点也不一样哇，看起来全是循环
		
		
	# DTRACE_COMPILE_FILE_ENTRY 这种大写的函数是什么？没有定义直接使用？	
	
# 先大概看一下引用顺序
	#include "php.h"
	#include "php_globals.h"
	#include "php_variables.h"
	#include "php_ini_builder.h"
	#include "zend_hash.h"
	#include "zend_modules.h"
	#include "zend_interfaces.h"
	
# zend_ast.c
	# php 代码语法解析似乎都在这里了
	

# static ZEND_COLD void zend_ast_export_ex(smart_str *str, zend_ast *ast, int priority, int indent)
	这个方法有800多行
  只在文件内部递归调用
  
 * Operator Precedence
 这里有一个操作符优先顺序， include, include_once, eval, require, require_once 都算操作符~
 
<- \Zend\zend_compile.h
<- \Zend\zend_variables.c


# zend_types.h

typedef struct _zend_ast        zend_ast;

# zend_ast.h
_zend_ast 在 zend_ast.h 定义
struct _zend_ast {
	zend_ast_kind kind; /* Type of the node (ZEND_AST_* enum constant) */
	zend_ast_attr attr; /* Additional attribute, use depending on node type */
	uint32_t lineno;    /* Line number */
	zend_ast *child[1]; /* Array of children (using struct hack) */
}; 

zend_ast_kind int 型，_zend_ast_kind 枚举
zend_ast_attr int 型，


# zend_language_parser.h
	这个摆明了是语法分析，但这个文件并不存在，是内置模块
	有一个 zend_language_parser.y 应该是它咯
	里面确实有关键语句定义
	
	ast是词法分析咯？
	
ZEND_API zend_op_array *compile_file(zend_file_handle *file_handle, int type)
居然在 zend_language_scanner.l 里！
-> zend_compile(int type)  文件名怎么传的
	open_file_for_scanning 里传的吧？
-> zend_compile_top_stmt(CG(ast));
	# 从这里正是进入compile，ast是如何获得的？
	
	# compile是对zend_ast 进行compile！
	
# zend_compile.c
/* Same as compile_stmt, but with early binding */
void zend_compile_top_stmt(zend_ast *ast)
-> zend_compile_stmt
-> 

接下来的操作在 zend_compile.c

ast是什么呢？暂且认为是assignment ?

compile获得opcode 之后调用
 zend_execute(op_array, retval);
 
# 果然是在zend vm里，仅有这一处定义
 ZEND_API void zend_execute(zend_op_array *op_array, zval *return_value)
 
 zend_execute 被调用次数比较少，容易确定逻辑关系

#  然后整个调用过程就结束了

zend_vm_execute.h 是最复杂最重要的部分



ZEND_API 是一个在gnuc环境下用到的简单的东西

#if defined(__GNUC__) && __GNUC__ >= 4
# define ZEND_API __attribute__ ((visibility("default")))
# define ZEND_DLEXPORT __attribute__ ((visibility("default")))
#else
# define ZEND_API
# define ZEND_DLEXPORT
#endif

暂时重点放在compile
