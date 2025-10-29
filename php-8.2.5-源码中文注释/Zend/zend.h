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
   +----------------------------------------------------------------------+
*/

#ifndef ZEND_H
#define ZEND_H

#define ZEND_VERSION "4.2.5"

#define ZEND_ENGINE_3

#include "zend_types.h"
#include "zend_map_ptr.h"
#include "zend_errors.h"
#include "zend_alloc.h"
#include "zend_llist.h"
#include "zend_string.h"
#include "zend_hash.h"
#include "zend_ast.h"
#include "zend_gc.h"
#include "zend_variables.h"
#include "zend_iterators.h"
#include "zend_stream.h"
#include "zend_smart_str_public.h"
#include "zend_smart_string_public.h"
#include "zend_signal.h"
#include "zend_max_execution_timer.h"

#define zend_sprintf sprintf

// /Zend/zend_signal.h	
#define HANDLE_BLOCK_INTERRUPTIONS()		ZEND_SIGNAL_BLOCK_INTERRUPTIONS()
#define HANDLE_UNBLOCK_INTERRUPTIONS()		ZEND_SIGNAL_UNBLOCK_INTERRUPTIONS()

// 内置函数参数，执行时常用
#define INTERNAL_FUNCTION_PARAMETERS zend_execute_data *execute_data, zval *return_value
// 执行数据 和 返回值
#define INTERNAL_FUNCTION_PARAM_PASSTHRU execute_data, return_value

// ing4, 验证：没有执行数据 或 是内置函数 或 有返回类型
#define USED_RET() \
	(!EX(prev_execute_data) || \
	 !ZEND_USER_CODE(EX(prev_execute_data)->func->common.type) || \
	 (EX(prev_execute_data)->opline->result_type != IS_UNUSED))

// 如果开始了静态 TSRMLS 缓存 
#ifdef ZEND_ENABLE_STATIC_TSRMLS_CACHE
#define ZEND_TSRMG TSRMG_STATIC
#define ZEND_TSRMG_FAST TSRMG_FAST_STATIC
// /TSRM/TSRM.h
#define ZEND_TSRMLS_CACHE_EXTERN() TSRMLS_CACHE_EXTERN()
// /TSRM/TSRM.h
#define ZEND_TSRMLS_CACHE_DEFINE() TSRMLS_CACHE_DEFINE()
// /TSRM/TSRM.h
#define ZEND_TSRMLS_CACHE_UPDATE() TSRMLS_CACHE_UPDATE()
#define ZEND_TSRMLS_CACHE TSRMLS_CACHE
// 如果没有开启
#else
#define ZEND_TSRMG TSRMG
#define ZEND_TSRMG_FAST TSRMG_FAST
#define ZEND_TSRMLS_CACHE_EXTERN()
#define ZEND_TSRMLS_CACHE_DEFINE()
#define ZEND_TSRMLS_CACHE_UPDATE()
#define ZEND_TSRMLS_CACHE
#endif

#ifndef ZEND_COMPILE_DL_EXT
TSRMLS_MAIN_CACHE_EXTERN()
#else
ZEND_TSRMLS_CACHE_EXTERN()
#endif

struct _zend_serialize_data;
struct _zend_unserialize_data;

// 序列化数据
typedef struct _zend_serialize_data zend_serialize_data;
typedef struct _zend_unserialize_data zend_unserialize_data;

// 类名
typedef struct _zend_class_name {
	// 类名
	zend_string *name;
	// 小写名称
	zend_string *lc_name;
} zend_class_name;

// trait 方法引用
typedef struct _zend_trait_method_reference {
	// 方法名
	zend_string *method_name;
	// 类名
	zend_string *class_name;
} zend_trait_method_reference;

// trait 优先
typedef struct _zend_trait_precedence {
	// trait 方法引用
	zend_trait_method_reference trait_method;
	// 排除数量
	uint32_t num_excludes;
	// 排除类名
	zend_string *exclude_class_names[1];
} zend_trait_precedence;

// trait 别名
typedef struct _zend_trait_alias {
	// trait 方法引用
	zend_trait_method_reference trait_method;

	// 被添加的方法
	/**
	* name for method to be added
	*/
	zend_string *alias;

	// 修饰符
	/**
	* modifiers to be set on trait method
	*/
	uint32_t modifiers;
} zend_trait_alias;

// 类的可转换数据？
typedef struct _zend_class_mutable_data {
	// 默认成员变量列表
	zval      *default_properties_table;
	// 常量列表
	HashTable *constants_table;
	// class entry 标记
	uint32_t   ce_flags;
	// ？
	HashTable *backed_enum_table;
} zend_class_mutable_data;

// 类依赖
typedef struct _zend_class_dependency {
	// 名称
	zend_string      *name;
	// 类入口
	zend_class_entry *ce;
} zend_class_dependency;

//
typedef struct _zend_inheritance_cache_entry zend_inheritance_cache_entry;

// 错误信息
typedef struct _zend_error_info {
	// 类型
	int type;
	// 行号
	uint32_t lineno;
	// 文件名
	zend_string *filename;
	// 错误信息
	zend_string *message;
} zend_error_info;

// 继承缓存入口？
struct _zend_inheritance_cache_entry {
	// 下一个缓存对象
	zend_inheritance_cache_entry *next;
	// 类入口
	zend_class_entry             *ce;
	// 父类入口
	zend_class_entry             *parent;
	// 依赖类
	zend_class_dependency        *dependencies;
	// 依赖类数量
	uint32_t                      dependencies_count;
	// 错误数
	uint32_t                      num_warnings;
	// 错误信息
	zend_error_info             **warnings;
	// 作用域
	zend_class_entry             *traits_and_interfaces[1];
};

// 类入口，作用域。php类在c语言中的呈现。
struct _zend_class_entry {
	// 类型
	char type;
	// 名称
	zend_string *name;
	/* class_entry or string depending on ZEND_ACC_LINKED */
	union {
		zend_class_entry *parent;
		zend_string *parent_name;
	};
	// 引用数
	int refcount;
	// class entry 标记
	uint32_t ce_flags;

	// 默认成员变量数量
	int default_properties_count;
	// 默认静态成员变量数量
	int default_static_members_count;
	// 默认成员变量列表
	zval *default_properties_table;
	// 默认静态成员变量列表
	zval *default_static_members_table;
	ZEND_MAP_PTR_DEF(zval *, static_members_table);
	// 成员方法列表
	HashTable function_table;
	// 成员变量信息
	HashTable properties_info;
	// 常量列表
	HashTable constants_table;

	ZEND_MAP_PTR_DEF(zend_class_mutable_data*, mutable_data);
	// 继承缓存入口？
	zend_inheritance_cache_entry *inheritance_cache;

	// **属性信息表 
	struct _zend_property_info **properties_info_table;

	// 构造方法
	zend_function *constructor;
	// 析构方法
	zend_function *destructor;
	// 克隆方法
	zend_function *clone;
	// as you know
	zend_function *__get;
	zend_function *__set;
	zend_function *__unset;
	zend_function *__isset;
	zend_function *__call;
	zend_function *__callstatic;
	zend_function *__tostring;
	// 调试信息
	zend_function *__debugInfo;
	// 序列化和反序列化
	zend_function *__serialize;
	zend_function *__unserialize;

	// 只有当类实现接口 Iterator 或 IteratorAggregate 时分配
	/* allocated only if class implements Iterator or IteratorAggregate interface */
	zend_class_iterator_funcs *iterator_funcs_ptr;
	// 只有当类实现接口 ArrayAccess 时分配
	/* allocated only if class implements ArrayAccess interface */
	zend_class_arrayaccess_funcs *arrayaccess_funcs_ptr;

	// 句柄
	/* handlers */
	union {
		// 这个是php里new 语句调用的的创建对象的方法
		zend_object* (*create_object)(zend_class_entry *class_type);
		//  ？
		int (*interface_gets_implemented)(zend_class_entry *iface, zend_class_entry *class_type); /* a class implements this interface */
	};
	// ？
	zend_object_iterator *(*get_iterator)(zend_class_entry *ce, zval *object, int by_ref);
	// 方法？
	zend_function *(*get_static_method)(zend_class_entry *ce, zend_string* method);

	/* serializer callbacks */
	int (*serialize)(zval *object, unsigned char **buffer, size_t *buf_len, zend_serialize_data *data);
	int (*unserialize)(zval *object, zend_class_entry *ce, const unsigned char *buf, size_t buf_len, zend_unserialize_data *data);

	// 接口数量
	uint32_t num_interfaces;
	// trait 数量
	uint32_t num_traits;

	// 
	/* class_entry or string(s) depending on ZEND_ACC_LINKED */
	union {
		zend_class_entry **interfaces;
		zend_class_name *interface_names;
	};

	// trait 名称列表
	zend_class_name *trait_names;
	// trait 别名列表
	zend_trait_alias **trait_aliases;
	// trait 优先列表
	zend_trait_precedence **trait_precedences;
	// 修饰属性
	HashTable *attributes;

	//
	uint32_t enum_backing_type;
	// 
	HashTable *backed_enum_table;

	// 信息
	union {
		struct {
			// 文件名
			zend_string *filename;
			// 开始行
			uint32_t line_start;
			// 结束行
			uint32_t line_end;
			// 注释文档
			zend_string *doc_comment;
		} user;
		struct {
			// 方法入口
			const struct _zend_function_entry *builtin_functions;
			// 模块入口
			struct _zend_module_entry *module;
		} internal;
	} info;
};

// 工具方法， /main/main.c	中设置每个方法
typedef struct _zend_utility_functions {
	// 报错方法
	void (*error_function)(int type, zend_string *error_filename, const uint32_t error_lineno, zend_string *message);
	// 打印方法
	size_t (*printf_function)(const char *format, ...) ZEND_ATTRIBUTE_PTR_FORMAT(printf, 1, 2);
	size_t (*write_function)(const char *str, size_t str_length);
	FILE *(*fopen_function)(zend_string *filename, zend_string **opened_path);
	void (*message_handler)(zend_long message, const void *data);
	zval *(*get_configuration_directive)(zend_string *name);
	void (*ticks_function)(int ticks);
	void (*on_timeout)(int seconds);
	zend_result (*stream_open_function)(zend_file_handle *handle);
	void (*printf_to_smart_string_function)(smart_string *buf, const char *format, va_list ap);
	void (*printf_to_smart_str_function)(smart_str *buf, const char *format, va_list ap);
	char *(*getenv_function)(const char *name, size_t name_len);
	zend_string *(*resolve_path_function)(zend_string *filename);
} zend_utility_functions;

// 
typedef struct _zend_utility_values {
	bool html_errors;
} zend_utility_values;

typedef size_t (*zend_write_func_t)(const char *str, size_t str_length);

// ing4, 跳伞
#define zend_bailout()		_zend_bailout(__FILE__, __LINE__)

// 模拟try
#define zend_try												\
	{															\
		/* 执行时的跳伞 */ \
		JMP_BUF *__orig_bailout = EG(bailout);					\
		JMP_BUF __bailout;										\
																\
		/* 执行时的跳伞，更新成临时变量 */ \
		EG(bailout) = &__bailout;								\
		/* 执行时的跳伞 */ \
		if (SETJMP(__bailout)==0) {

// 模拟cache
#define zend_catch												\
		} else {												\
			/* 还原跳伞 */ \
			EG(bailout) = __orig_bailout;

// ing4, 结束try, 还原跳伞
#define zend_end_try()											\
		}														\
		/* 还原跳伞 */ \
		EG(bailout) = __orig_bailout;							\
	}

// 
#define zend_first_try		EG(bailout)=NULL;	zend_try

BEGIN_EXTERN_C()
void zend_startup(zend_utility_functions *utility_functions);
void zend_shutdown(void);
void zend_register_standard_ini_entries(void);
zend_result zend_post_startup(void);
void zend_set_utility_values(zend_utility_values *utility_values);

ZEND_API ZEND_COLD ZEND_NORETURN void _zend_bailout(const char *filename, uint32_t lineno);
ZEND_API size_t zend_get_page_size(void);

ZEND_API size_t zend_vspprintf(char **pbuf, size_t max_len, const char *format, va_list ap);
ZEND_API size_t zend_spprintf(char **message, size_t max_len, const char *format, ...) ZEND_ATTRIBUTE_FORMAT(printf, 3, 4);
ZEND_API zend_string *zend_vstrpprintf(size_t max_len, const char *format, va_list ap);
ZEND_API zend_string *zend_strpprintf(size_t max_len, const char *format, ...) ZEND_ATTRIBUTE_FORMAT(printf, 2, 3);

/* Same as zend_spprintf and zend_strpprintf, without checking of format validity.
 * For use with custom printf specifiers such as %H. */
ZEND_API size_t zend_spprintf_unchecked(char **message, size_t max_len, const char *format, ...);
ZEND_API zend_string *zend_strpprintf_unchecked(size_t max_len, const char *format, ...);

ZEND_API const char *get_zend_version(void);
ZEND_API bool zend_make_printable_zval(zval *expr, zval *expr_copy);
ZEND_API size_t zend_print_zval(zval *expr, int indent);
ZEND_API void zend_print_zval_r(zval *expr, int indent);
ZEND_API zend_string *zend_print_zval_r_to_str(zval *expr, int indent);
ZEND_API void zend_print_flat_zval_r(zval *expr);
void zend_print_flat_zval_r_to_buf(smart_str *str, zval *expr);

// ing4, 打印变量, 无缩进
static zend_always_inline size_t zend_print_variable(zval *var) {
	// 打印zval ,返回字串长度。 p2:缩进
	return zend_print_zval(var, 0);
}

ZEND_API ZEND_COLD void zend_output_debug_string(bool trigger_break, const char *format, ...) ZEND_ATTRIBUTE_FORMAT(printf, 2, 3);

ZEND_API void zend_activate(void);
ZEND_API void zend_deactivate(void);
ZEND_API void zend_call_destructors(void);
ZEND_API void zend_activate_modules(void);
ZEND_API void zend_deactivate_modules(void);
ZEND_API void zend_post_deactivate_modules(void);

ZEND_API void free_estring(char **str_p);

END_EXTERN_C()

/* output support */
// ing4, zend_write 别名
#define ZEND_WRITE(str, str_len)		zend_write((str), (str_len))
// ing4, write_func 别名
#define ZEND_WRITE_EX(str, str_len)		write_func((str), (str_len))
// ing4, zend_write, 自动计算字串长度。p1: 要打印的char*
#define ZEND_PUTS(str)					zend_write((str), strlen((str)))
// ing4, write_func, 自动计算字串长度。p1: 要打印的char*
#define ZEND_PUTS_EX(str)				write_func((str), strlen((str)))
// ing4, zend_write 打印单个字符
#define ZEND_PUTC(c)					zend_write(&(c), 1)

BEGIN_EXTERN_C()
extern ZEND_API size_t (*zend_printf)(const char *format, ...) ZEND_ATTRIBUTE_PTR_FORMAT(printf, 1, 2);
extern ZEND_API zend_write_func_t zend_write;
extern ZEND_API FILE *(*zend_fopen)(zend_string *filename, zend_string **opened_path);
extern ZEND_API void (*zend_ticks_function)(int ticks);
extern ZEND_API void (*zend_interrupt_function)(zend_execute_data *execute_data);
extern ZEND_API void (*zend_error_cb)(int type, zend_string *error_filename, const uint32_t error_lineno, zend_string *message);
extern ZEND_API void (*zend_on_timeout)(int seconds);
extern ZEND_API zend_result (*zend_stream_open_function)(zend_file_handle *handle);
extern void (*zend_printf_to_smart_string)(smart_string *buf, const char *format, va_list ap);
extern void (*zend_printf_to_smart_str)(smart_str *buf, const char *format, va_list ap);
extern ZEND_API char *(*zend_getenv)(const char *name, size_t name_len);
// 	zend_resolve_path = utility_functions->resolve_path_function
extern ZEND_API zend_string *(*zend_resolve_path)(zend_string *filename);

/* These two callbacks are especially for opcache */
extern ZEND_API zend_result (*zend_post_startup_cb)(void);
extern ZEND_API void (*zend_post_shutdown_cb)(void);

ZEND_API ZEND_COLD void zend_error(int type, const char *format, ...) ZEND_ATTRIBUTE_FORMAT(printf, 2, 3);
ZEND_API ZEND_COLD ZEND_NORETURN void zend_error_noreturn(int type, const char *format, ...) ZEND_ATTRIBUTE_FORMAT(printf, 2, 3);
/* For custom format specifiers like H */
ZEND_API ZEND_COLD void zend_error_unchecked(int type, const char *format, ...);
/* If filename is NULL the default filename is used. */
ZEND_API ZEND_COLD void zend_error_at(int type, zend_string *filename, uint32_t lineno, const char *format, ...) ZEND_ATTRIBUTE_FORMAT(printf, 4, 5);
ZEND_API ZEND_COLD ZEND_NORETURN void zend_error_at_noreturn(int type, zend_string *filename, uint32_t lineno, const char *format, ...) ZEND_ATTRIBUTE_FORMAT(printf, 4, 5);
ZEND_API ZEND_COLD void zend_error_zstr(int type, zend_string *message);
ZEND_API ZEND_COLD void zend_error_zstr_at(int type, zend_string *filename, uint32_t lineno, zend_string *message);

ZEND_API ZEND_COLD void zend_throw_error(zend_class_entry *exception_ce, const char *format, ...) ZEND_ATTRIBUTE_FORMAT(printf, 2, 3);
ZEND_API ZEND_COLD void zend_type_error(const char *format, ...) ZEND_ATTRIBUTE_FORMAT(printf, 1, 2);
ZEND_API ZEND_COLD void zend_argument_count_error(const char *format, ...) ZEND_ATTRIBUTE_FORMAT(printf, 1, 2);
ZEND_API ZEND_COLD void zend_value_error(const char *format, ...) ZEND_ATTRIBUTE_FORMAT(printf, 1, 2);

ZEND_COLD void zenderror(const char *error);

/* For internal C errors */
ZEND_API ZEND_COLD ZEND_NORETURN void zend_strerror_noreturn(int type, int errn, const char *message);

/* The following #define is used for code duality in PHP for Engine 1 & 2 */
#define ZEND_STANDARD_CLASS_DEF_PTR zend_standard_class_def
extern ZEND_API zend_class_entry *zend_standard_class_def;
extern ZEND_API zend_utility_values zend_uv;

/* If DTrace is available and enabled */
extern ZEND_API bool zend_dtrace_enabled;
END_EXTERN_C()

// ing4, 返回 zend_uv.name
#define ZEND_UV(name) (zend_uv.name)

BEGIN_EXTERN_C()
ZEND_API void zend_message_dispatcher(zend_long message, const void *data);

ZEND_API zval *zend_get_configuration_directive(zend_string *name);
END_EXTERN_C()

// zend应用的消息
/* Messages for applications of Zend */
#define ZMSG_FAILED_INCLUDE_FOPEN		1L
#define ZMSG_FAILED_REQUIRE_FOPEN		2L
#define ZMSG_FAILED_HIGHLIGHT_FOPEN		3L
#define ZMSG_MEMORY_LEAK_DETECTED		4L
#define ZMSG_MEMORY_LEAK_REPEATED		5L
#define ZMSG_LOG_SCRIPT_NAME			6L
#define ZMSG_MEMORY_LEAKS_GRAND_TOTAL	7L

// 
typedef enum {
	EH_NORMAL = 0,
	EH_THROW
} zend_error_handling_t;

// 错误句柄
typedef struct {
	// 
	zend_error_handling_t  handling;
	// 错误类实体
	zend_class_entry       *exception;
} zend_error_handling;

BEGIN_EXTERN_C()
ZEND_API void zend_save_error_handling(zend_error_handling *current);
ZEND_API void zend_replace_error_handling(zend_error_handling_t error_handling, zend_class_entry *exception_class, zend_error_handling *current);
ZEND_API void zend_restore_error_handling(zend_error_handling *saved);
ZEND_API void zend_begin_record_errors(void);
ZEND_API void zend_emit_recorded_errors(void);
ZEND_API void zend_free_recorded_errors(void);
END_EXTERN_C()

#define DEBUG_BACKTRACE_PROVIDE_OBJECT (1<<0)
#define DEBUG_BACKTRACE_IGNORE_ARGS    (1<<1)

#include "zend_object_handlers.h"
#include "zend_operators.h"

#endif /* ZEND_H */
