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

#include "zend.h"
#include "zend_extensions.h"
#include "zend_modules.h"
#include "zend_constants.h"
#include "zend_list.h"
#include "zend_API.h"
#include "zend_exceptions.h"
#include "zend_builtin_functions.h"
#include "zend_ini.h"
#include "zend_vm.h"
#include "zend_dtrace.h"
#include "zend_virtual_cwd.h"
#include "zend_smart_str.h"
#include "zend_smart_string.h"
#include "zend_cpuinfo.h"
#include "zend_attributes.h"
#include "zend_observer.h"
#include "zend_fibers.h"
#include "zend_max_execution_timer.h"
#include "Optimizer/zend_optimizer.h"

static size_t global_map_ptr_last = 0;
static bool startup_done = false;
// 如果要求线程安全
#ifdef ZTS
ZEND_API int compiler_globals_id;
ZEND_API int executor_globals_id;
ZEND_API size_t compiler_globals_offset;
ZEND_API size_t executor_globals_offset;
static HashTable *global_function_table = NULL;
static HashTable *global_class_table = NULL;
static HashTable *global_constants_table = NULL;
static HashTable *global_auto_globals_table = NULL;
static HashTable *global_persistent_list = NULL;
TSRMLS_MAIN_CACHE_DEFINE()

// 每个线程使用各自的 公共变量表
# define GLOBAL_FUNCTION_TABLE		global_function_table
# define GLOBAL_CLASS_TABLE			global_class_table
# define GLOBAL_CONSTANTS_TABLE		global_constants_table
# define GLOBAL_AUTO_GLOBALS_TABLE	global_auto_globals_table

// 非线程安全
#else
// 所有线程使用公共的 公共变量表
# define GLOBAL_FUNCTION_TABLE		CG(function_table)
# define GLOBAL_CLASS_TABLE			CG(class_table)
# define GLOBAL_AUTO_GLOBALS_TABLE	CG(auto_globals)
# define GLOBAL_CONSTANTS_TABLE		EG(zend_constants)
#endif

// 
ZEND_API zend_utility_values zend_uv;
ZEND_API bool zend_dtrace_enabled;

/* version information */
// 版本信息
static char *zend_version_info;
// 版本信息长度
static uint32_t zend_version_info_length;
#define ZEND_CORE_VERSION_INFO	"Zend Engine v" ZEND_VERSION ", Copyright (c) Zend Technologies\n"
#define PRINT_ZVAL_INDENT 4

/* true multithread-shared globals */
ZEND_API zend_class_entry *zend_standard_class_def = NULL;
ZEND_API size_t (*zend_printf)(const char *format, ...);
ZEND_API zend_write_func_t zend_write;
ZEND_API FILE *(*zend_fopen)(zend_string *filename, zend_string **opened_path);
ZEND_API zend_result (*zend_stream_open_function)(zend_file_handle *handle);
ZEND_API void (*zend_ticks_function)(int ticks);
ZEND_API void (*zend_interrupt_function)(zend_execute_data *execute_data);
ZEND_API void (*zend_error_cb)(int type, zend_string *error_filename, const uint32_t error_lineno, zend_string *message);
void (*zend_printf_to_smart_string)(smart_string *buf, const char *format, va_list ap);
// 
void (*zend_printf_to_smart_str)(smart_str *buf, const char *format, va_list ap);
ZEND_API char *(*zend_getenv)(const char *name, size_t name_len);
ZEND_API zend_string *(*zend_resolve_path)(zend_string *filename);
ZEND_API zend_result (*zend_post_startup_cb)(void) = NULL;
ZEND_API void (*zend_post_shutdown_cb)(void) = NULL;

/* This callback must be signal handler safe! */
void (*zend_on_timeout)(int seconds);

static void (*zend_message_dispatcher_p)(zend_long message, const void *data);
static zval *(*zend_get_configuration_directive_p)(zend_string *name);

#if ZEND_RC_DEBUG
ZEND_API bool zend_rc_debug = 0;
#endif

// ing3, 事件：更新报错级别
// int OnUpdateErrorReporting(zend_ini_entry *entry, zend_string *new_value, void *mh_arg1, void *mh_arg2, void *mh_arg3, int stage)
static ZEND_INI_MH(OnUpdateErrorReporting) /* {{{ */
{
	// 新值为空
	if (!new_value) {
		// 显示全部
		EG(error_reporting) = E_ALL;
	// 不空空
	} else {
		// 转成int型
		EG(error_reporting) = atoi(ZSTR_VAL(new_value));
	}
	// 返回成功
	return SUCCESS;
}
/* }}} */

// ing3, 事件：开启/关闭 gc
// int OnUpdateGCEnabled(zend_ini_entry *entry, zend_string *new_value, void *mh_arg1, void *mh_arg2, void *mh_arg3, int stage)
static ZEND_INI_MH(OnUpdateGCEnabled) /* {{{ */
{
	bool val;
	// 值转成布尔型
	val = zend_ini_parse_bool(new_value);
	// 调用gc方法
	gc_enable(val);
	// 返回成功
	return SUCCESS;
}
/* }}} */

// ing3, 显示 gc 开启状态
// ZEND_COLD void zend_ini_boolean_displayer_cb(zend_ini_entry *ini_entry, int type)
static ZEND_INI_DISP(zend_gc_enabled_displayer_cb) /* {{{ */
{
	// 如果gc启用了
	if (gc_enabled()) {
		// 打印 On
		ZEND_PUTS("On");
	// 否则 
	} else {
		// 打印 Off
		ZEND_PUTS("Off");
	}
}
/* }}} */

// ？？？, 事件：
// int OnUpdateScriptEncoding(zend_ini_entry *entry, zend_string *new_value, void *mh_arg1, void *mh_arg2, void *mh_arg3, int stage)
static ZEND_INI_MH(OnUpdateScriptEncoding) /* {{{ */
{
	// 如果编译器不支持多字节
	if (!CG(multibyte)) {
		// 返回失败
		return FAILURE;
	}
	// 如果没有多字节函数
	if (!zend_multibyte_get_functions()) {
		// 返回成功
		return SUCCESS;
	}
	return zend_multibyte_set_script_encoding_by_string(new_value ? ZSTR_VAL(new_value) : NULL, new_value ? ZSTR_LEN(new_value) : 0);
}
/* }}} */

// ing3, 事件：更新断言 zend.assertions 条目
// int OnUpdateAssertions(zend_ini_entry *entry, zend_string *new_value, void *mh_arg1, void *mh_arg2, void *mh_arg3, int stage)
static ZEND_INI_MH(OnUpdateAssertions) /* {{{ */
{
	// 取得整数指针
	// 取得参数 mh_arg2 ，并把指针前进 mh_arg1 个字节
	zend_long *p = (zend_long *) ZEND_INI_GET_ADDR();

	// 获取配置项整数值。失败则报错。p1:数字字串，p2:配置名
	zend_long val = zend_ini_parse_quantity_warn(new_value, entry->name);

	// 如果不在 开开启 和关闭阶段 并且 当前配置值和传入值不同 并且 （两个中有一个小于0）
	if (stage != ZEND_INI_STAGE_STARTUP &&
	    stage != ZEND_INI_STAGE_SHUTDOWN &&
	    *p != val &&
	    (*p < 0 || val < 0)) {
		// 警告：zend.assertions 只能在 php.ini 中完全开启 或 关闭
		zend_error(E_WARNING, "zend.assertions may be completely enabled or disabled only in php.ini");
		// 返回失败
		return FAILURE;
	}

	// 返回原值
	*p = val;
	// 返回成功
	return SUCCESS;
}
/* }}} */

// ing3, 事件：更新异常字串参数最大长度
// int OnSetExceptionStringParamMaxLen(zend_ini_entry *entry, zend_string *new_value, void *mh_arg1, void *mh_arg2, void *mh_arg3, int stage)
static ZEND_INI_MH(OnSetExceptionStringParamMaxLen) /* {{{ */
{
	// 取得新值
	zend_long i = ZEND_ATOL(ZSTR_VAL(new_value));
	// 如果新值在有效范围内
	if (i >= 0 && i <= 1000000) {
		// 更新 运行时 全局变量
		EG(exception_string_param_max_len) = i;
		// 返回成功
		return SUCCESS;
	// 如果值无效
	} else {
		// 返回失败
		return FAILURE;
	}
}
/* }}} */

// ing3, 事件：更新 fiber堆栈大小
// int OnUpdateFiberStackSize(zend_ini_entry *entry, zend_string *new_value, void *mh_arg1, void *mh_arg2, void *mh_arg3, int stage)
static ZEND_INI_MH(OnUpdateFiberStackSize) /* {{{ */
{
	// 如果 新值 有效
	if (new_value) {
		// 更新执行时 全局变量
		// 获取配置项整数值。失败则报错。p1:数字字串，p2:配置名
		EG(fiber_stack_size) = zend_ini_parse_quantity_warn(new_value, entry->name);
	// 如果新值无效
	} else {
		// 更新执行时 全局变量 =  (4096 * (((sizeof(void *)) < 8) ? 256 : 512))
		EG(fiber_stack_size) = ZEND_FIBER_DEFAULT_C_STACK_SIZE;
	}
	// 返回成功
	return SUCCESS;
}
/* }}} */

#if ZEND_DEBUG
# define SIGNAL_CHECK_DEFAULT "1"
#else
# define SIGNAL_CHECK_DEFAULT "0"
#endif

ZEND_INI_BEGIN()
	ZEND_INI_ENTRY("error_reporting",				NULL,		ZEND_INI_ALL,		OnUpdateErrorReporting)
	STD_ZEND_INI_ENTRY("zend.assertions",				"1",    ZEND_INI_ALL,       OnUpdateAssertions,           assertions,   zend_executor_globals,  executor_globals)
	ZEND_INI_ENTRY3_EX("zend.enable_gc",				"1",	ZEND_INI_ALL,		OnUpdateGCEnabled, NULL, NULL, NULL, zend_gc_enabled_displayer_cb)
	STD_ZEND_INI_BOOLEAN("zend.multibyte", "0", ZEND_INI_PERDIR, OnUpdateBool, multibyte,      zend_compiler_globals, compiler_globals)
	ZEND_INI_ENTRY("zend.script_encoding",			NULL,		ZEND_INI_ALL,		OnUpdateScriptEncoding)
	STD_ZEND_INI_BOOLEAN("zend.detect_unicode",			"1",	ZEND_INI_ALL,		OnUpdateBool, detect_unicode, zend_compiler_globals, compiler_globals)
#ifdef ZEND_SIGNALS
	STD_ZEND_INI_BOOLEAN("zend.signal_check", SIGNAL_CHECK_DEFAULT, ZEND_INI_SYSTEM, OnUpdateBool, check, zend_signal_globals_t, zend_signal_globals)
#endif
	STD_ZEND_INI_BOOLEAN("zend.exception_ignore_args",	"0",	ZEND_INI_ALL,		OnUpdateBool, exception_ignore_args, zend_executor_globals, executor_globals)
	STD_ZEND_INI_ENTRY("zend.exception_string_param_max_len",	"15",	ZEND_INI_ALL,	OnSetExceptionStringParamMaxLen,	exception_string_param_max_len,		zend_executor_globals,	executor_globals)
	STD_ZEND_INI_ENTRY("fiber.stack_size",		NULL,			ZEND_INI_ALL,		OnUpdateFiberStackSize,		fiber_stack_size,	zend_executor_globals, 		executor_globals)

ZEND_INI_END()

// ing3, 按给出的格式打印到smart_string 并返回 下属字串
// 其实只要下属字串，但smart_string 方便中间处理。
ZEND_API size_t zend_vspprintf(char **pbuf, size_t max_len, const char *format, va_list ap) /* {{{ */
{
	// smart_string
	smart_string buf = {0};
	// 当没有检查null就调用(v)spprintf  ...
	/* since there are places where (v)spprintf called without checking for null,
	   a bit of defensive coding here */
	
	// pbuf为空的的话直接返回0
	if (!pbuf) {
		return 0;
	}
	// 实际调用 php_printf_to_smart_string ：main/sprintf.c
	// 把信息打印到 缓冲区
	zend_printf_to_smart_string(&buf, format, ap);
	// 如果字串超长
	if (max_len && buf.len > max_len) {
		// 使用最大长度
		buf.len = max_len;
	}
	// 添加 \0
	smart_string_0(&buf);
	// 如果下属字串有效
	if (buf.c) {
		// 返回下属字串
		*pbuf = buf.c;
		// 返回长度
		return buf.len;
	// 如果无效
	} else {
		// 返回空
		*pbuf = estrndup("", 0);
		// 返回长度0
		return 0;
	}
}
/* }}} */

// ing3, 按给出的格式组织 zend_string 并返回 smart_string下属字串 。传入最大长度，格式，参数列表
ZEND_API size_t zend_spprintf(char **message, size_t max_len, const char *format, ...) /* {{{ */
{
	va_list arg;
	size_t len;

	va_start(arg, format);
	// ing3, 按给出的格式打印到smart_string 并返回 smart_string下属字串
	len = zend_vspprintf(message, max_len, format, arg);
	va_end(arg);
	return len;
}
/* }}} */

// ing3, 和上面一样
ZEND_API size_t zend_spprintf_unchecked(char **message, size_t max_len, const char *format, ...) /* {{{ */
{
	va_list arg;
	size_t len;

	va_start(arg, format);
	// ing3, 按给出的格式打印到smart_string 并返回 smart_string下属字串
	len = zend_vspprintf(message, max_len, format, arg);
	va_end(arg);
	return len;
}
/* }}} */

// ing4, 通过format和参数列表，组织 zend_string
ZEND_API zend_string *zend_vstrpprintf(size_t max_len, const char *format, va_list ap) /* {{{ */
{
	// 创建临时的 smart_str
	smart_str buf = {0};

	// smart_str 中写入信息
	zend_printf_to_smart_str(&buf, format, ap);

	// 如果写入信息无效
	if (!buf.s) {
		// 返回空字串
		return ZSTR_EMPTY_ALLOC();
	}
	// 如果有长度，并且大于最大长度
	if (max_len && ZSTR_LEN(buf.s) > max_len) {
		// 使用最大长度
		ZSTR_LEN(buf.s) = max_len;
	}
	// 把zend_string 抽出并返回
	return smart_str_extract(&buf);
}
/* }}} */

// ing4, 通过format和参数列表，组织 zend_string
ZEND_API zend_string *zend_strpprintf(size_t max_len, const char *format, ...) /* {{{ */
{
	va_list arg;
	zend_string *str;

	va_start(arg, format);
	// ing4, 通过format和参数列表，组织 zend_string
	str = zend_vstrpprintf(max_len, format, arg);
	va_end(arg);
	return str;
}
/* }}} */

// ing4, 参数通过参数列表，构造并返回错误信息
ZEND_API zend_string *zend_strpprintf_unchecked(size_t max_len, const char *format, ...) /* {{{ */
{
	va_list arg;
	zend_string *str;
	// 开始读取列表参数
	va_start(arg, format);
	// ing4, 通过format和参数列表，组织 zend_string
	str = zend_vstrpprintf(max_len, format, arg);
	// 结束读取列表参数
	va_end(arg);
	// 返回字串
	return str;
}
/* }}} */

static void zend_print_zval_r_to_buf(smart_str *buf, zval *expr, int indent);

// ing3, 把哈希表打印进缓存， p1:缓存，p2:哈希表，p3:缩进，p4:元素是否是对象
static void print_hash(smart_str *buf, HashTable *ht, int indent, bool is_object) /* {{{ */
{
	zval *tmp;
	zend_string *string_key;
	zend_ulong num_key;
	int i;

	// 添加缩进
	for (i = 0; i < indent; i++) {
		smart_str_appendc(buf, ' ');
	}
	// 添加 （\n
	smart_str_appends(buf, "(\n");
	// PRINT_ZVAL_INDENT=4 （4个空格）
	indent += PRINT_ZVAL_INDENT;
	// 遍历哈希表
	ZEND_HASH_FOREACH_KEY_VAL_IND(ht, num_key, string_key, tmp) {
		// 添加缩进
		for (i = 0; i < indent; i++) {
			smart_str_appendc(buf, ' ');
		}
		// 添加 [
		smart_str_appendc(buf, '[');
		// 如果有字串 key
		if (string_key) {
			// 如果 元素 是对象
			if (is_object) {
				const char *prop_name, *class_name;
				size_t prop_len;
				// 把类名 和 属性名串起来，成为key
				int mangled = zend_unmangle_property_name_ex(string_key, &class_name, &prop_name, &prop_len);
				// 把属性名打印进缓存 
				smart_str_appendl(buf, prop_name, prop_len);
				// 如果有类名 和 串联好的属性名
				if (class_name && mangled == SUCCESS) {
					// 如果类名第一个字符为 *
					if (class_name[0] == '*') {
						// 添加 :protected 
						smart_str_appends(buf, ":protected");
					// 否则 
					} else {
						// 添加 :
						smart_str_appends(buf, ":");
						// 添加类名
						smart_str_appends(buf, class_name);
						// 添加 :private
						smart_str_appends(buf, ":private");
					}
				}
			// 如果不是对象
			} else {
				// 添加 key
				smart_str_append(buf, string_key);
			}
		// 如果没有字串key
		} else {
			// 添加哈希值
			smart_str_append_long(buf, num_key);
		}
		// 添加 ] => 
		smart_str_appends(buf, "] => ");
		// 把 zval 打印进缓存里。p1:缓存，p2:zval，p3:缩进
		zend_print_zval_r_to_buf(buf, tmp, indent+PRINT_ZVAL_INDENT);
		// 添加 "\n"
		smart_str_appends(buf, "\n");
	} ZEND_HASH_FOREACH_END();
	// 减少缩进
	indent -= PRINT_ZVAL_INDENT;
	// 添加缩进字串
	for (i = 0; i < indent; i++) {
		smart_str_appendc(buf, ' ');
	}
	// 添加 ")\n"
	smart_str_appends(buf, ")\n");
}
/* }}} */

// 非常常用的功能
// ing3, 打印数组到字串中, p1:返回内容, p2:要打印的数组
static void print_flat_hash(smart_str *buf, HashTable *ht) /* {{{ */
{
	zval *tmp;
	zend_string *string_key;
	zend_ulong num_key;
	int i = 0;
	// 遍历哈希表
	ZEND_HASH_FOREACH_KEY_VAL_IND(ht, num_key, string_key, tmp) {
		// 不是第一个
		if (i++ > 0) {
			// 要先加,
			smart_str_appendc(buf, ',');
		}
		// 把 key 用 [ ] => 包起来
		// 加[
		smart_str_appendc(buf, '[');
		// 如果有字串key
		if (string_key) {
			// 添加字串key
			smart_str_append(buf, string_key);
		// 否则
		} else {
			// 添加整数key
			smart_str_append_unsigned(buf, num_key);
		}
		// 添加 ] => 
		smart_str_appends(buf, "] => ");
		// 间接递归，处理哈希表里每个元素
		// 把 zval 打印到缓存里(遍历数组 或 对象)。 p1:缓存, p2:zval
		zend_print_flat_zval_r_to_buf(buf, tmp);
	} ZEND_HASH_FOREACH_END();
}
/* }}} */

// ing4, 如果不是字串，转成字串放在副本里返回。p1:表达式，p2:返回副本。
ZEND_API bool zend_make_printable_zval(zval *expr, zval *expr_copy) /* {{{ */
{
	// 如果是字串
	if (Z_TYPE_P(expr) == IS_STRING) {
		// 不需要转换
		return 0;
	// 如果不是字串
	} else {
		// 转成字串，放在副本里返回
		ZVAL_STR(expr_copy, zval_get_string_func(expr));
		// 需要转换
		return 1;
	}
}
/* }}} */

// ing4, 打印zval ,返回字串长度。 p2:缩进
ZEND_API size_t zend_print_zval(zval *expr, int indent) /* {{{ */
{
	zend_string *tmp_str;
	// 运算对象转成临时字串（本身是字串则返回字串指针），p1:运算对象，p2:接收返回字串（p1字串时返回null）
	zend_string *str = zval_get_tmp_string(expr, &tmp_str);
	// 长度
	size_t len = ZSTR_LEN(str);

	// 如果有字串
	if (len != 0) {
		// 打印 
		zend_write(ZSTR_VAL(str), len);
	}

	// 释放字串
	zend_tmp_string_release(tmp_str);
	// 返回长度
	return len;
}
/* }}} */

// ing4, 把 zval 打印到缓存里(遍历数组 或 对象)。 p1:缓存, p2:zval
void zend_print_flat_zval_r_to_buf(smart_str *buf, zval *expr) /* {{{ */
{
	// 根据 zval类型操作
	switch (Z_TYPE_P(expr)) {
		// 数组 
		case IS_ARRAY:
			// 开头是 Array (
			smart_str_appends(buf, "Array (");
			// 如果表达式 不是不可更改
			if (!(GC_FLAGS(Z_ARRVAL_P(expr)) & GC_IMMUTABLE)) {
				// 如果是递归
				if (GC_IS_RECURSIVE(Z_ARRVAL_P(expr))) {
					// 添加 *RECURSION* 递归标记
					smart_str_appends(buf, " *RECURSION*");
					// 跳出
					return;
				}
				// 不是递归，添加递归保护
				GC_PROTECT_RECURSION(Z_ARRVAL_P(expr));
			}
			// 打印数组到字串中, p1:返回内容, p2:要打印的数组
			print_flat_hash(buf, Z_ARRVAL_P(expr));
			// 添加 ）
			smart_str_appendc(buf, ')');
			// 解除递归保护
			GC_TRY_UNPROTECT_RECURSION(Z_ARRVAL_P(expr));
			// 跳出
			break;
		// 对象
		case IS_OBJECT:
		{
			HashTable *properties;
			// 调用 get_class_name 方法获取类名
			zend_string *class_name = Z_OBJ_HANDLER_P(expr, get_class_name)(Z_OBJ_P(expr));
			// 类名添加到字串中
			smart_str_append(buf, class_name);
			// 添加 Object (
			smart_str_appends(buf, " Object (");
			// 释放类名
			zend_string_release_ex(class_name, 0);

			// 如果有递归
			if (GC_IS_RECURSIVE(Z_COUNTED_P(expr))) {
				// 添加 *RECURSION* 递归标记
				smart_str_appends(buf, " *RECURSION*");
				// 跳出
				return;
			}
			// 取出对象的属性表
			properties = Z_OBJPROP_P(expr);
			// 如果有属性表
			if (properties) {
				// 添加递归保护
				GC_PROTECT_RECURSION(Z_OBJ_P(expr));
				// 打印数组到字串中, p1:返回内容, p2:要打印的数组
				print_flat_hash(buf, properties);
				// 解除递归保护
				GC_UNPROTECT_RECURSION(Z_OBJ_P(expr));
			}
			// 添加 ）
			smart_str_appendc(buf, ')');
			// 跳出
			break;
		}
		// 引用
		case IS_REFERENCE:
			// 把 zval 打印到缓存里(遍历数组 或 对象)。 p1:缓存, p2:zval
			zend_print_flat_zval_r_to_buf(buf, Z_REFVAL_P(expr));
			// 跳出
			break;
		// 字串
		case IS_STRING:
			// expr 转成字串添加进缓存
			smart_str_append(buf, Z_STR_P(expr));
			// 跳出
			break;
		// 默认情况
		default:
		{
			// expr 转成 zend_string
			zend_string *str = zval_get_string_func(expr);
			// 添加进缓存里
			smart_str_append(buf, str);
			// 释放 临时变量
			zend_string_release_ex(str, 0);
			// 跳出
			break;
		}
	}
}
/* }}} */

// ing3, 打印表达式
ZEND_API void zend_print_flat_zval_r(zval *expr)
{
	// 创建临时智能字串
	smart_str buf = {0};
	// 把 zval 打印到缓存里(遍历数组 或 对象)。 p1:缓存, p2:zval
	zend_print_flat_zval_r_to_buf(&buf, expr);
	// 添加 \0
	smart_str_0(&buf);
	// 打印
	zend_write(ZSTR_VAL(buf.s), ZSTR_LEN(buf.s));
	// 释放字串
	smart_str_free(&buf);
}

// 和 zend_print_flat_zval_r_to_buf 有什么不同
// ing3, 把 zval 打印进缓存里。p1:缓存，p2:zval，p3:缩进
static void zend_print_zval_r_to_buf(smart_str *buf, zval *expr, int indent) /* {{{ */
{
	// 按打印目标的类型操作
	switch (Z_TYPE_P(expr)) {
		// 数组
		case IS_ARRAY:
			// 添加 "Array\n"
			smart_str_appends(buf, "Array\n");
			// 如果不是不可更改
			if (!(GC_FLAGS(Z_ARRVAL_P(expr)) & GC_IMMUTABLE)) {
				// 如果有递归保护
				if (GC_IS_RECURSIVE(Z_ARRVAL_P(expr))) {
					// 添加 " *RECURSION*"
					smart_str_appends(buf, " *RECURSION*");
					// 返回
					return;
				}
				// 添加递归保护
				GC_PROTECT_RECURSION(Z_ARRVAL_P(expr));
			}
			// 打印哈希表（间接递归）
			print_hash(buf, Z_ARRVAL_P(expr), indent, 0);
			// 解除递归保护
			GC_TRY_UNPROTECT_RECURSION(Z_ARRVAL_P(expr));
			break;
		// 对象
		case IS_OBJECT:
			{
				HashTable *properties;
				// 取出对象
				zend_object *zobj = Z_OBJ_P(expr);
				// 调用 get_class_name 方法取出类名
				zend_string *class_name = Z_OBJ_HANDLER_P(expr, get_class_name)(zobj);
				// 添加类名
				smart_str_appends(buf, ZSTR_VAL(class_name));
				// 释放临时类名
				zend_string_release_ex(class_name, 0);

				// 如果不是 枚举类型
				if (!(zobj->ce->ce_flags & ZEND_ACC_ENUM)) {
					// 添加 " Object\n"
					smart_str_appends(buf, " Object\n");
				// 是枚举类型
				} else {
					// 添加 " Enum"
					smart_str_appends(buf, " Enum");
					// 如果枚举有备用类型
					if (zobj->ce->enum_backing_type != IS_UNDEF) {
						// 添加 ：
						smart_str_appendc(buf, ':');
						// 添加 类型字串
						smart_str_appends(buf, zend_get_type_by_const(zobj->ce->enum_backing_type));
					}
					// 添加 '\n'
					smart_str_appendc(buf, '\n');
				}

				// 如果有递归
				if (GC_IS_RECURSIVE(Z_OBJ_P(expr))) {
					// 添加 " *RECURSION*"
					smart_str_appends(buf, " *RECURSION*");
					// 返回
					return;
				}

				// 如果 操作目的 不是 调试
				if ((properties = zend_get_properties_for(expr, ZEND_PROP_PURPOSE_DEBUG)) == NULL) {
					跳出
					break;
				}

				// 添加递归保护
				GC_PROTECT_RECURSION(Z_OBJ_P(expr));
				// 打印哈希表（间接递归）
				print_hash(buf, properties, indent, 1);
				// 解除递归引用
				GC_UNPROTECT_RECURSION(Z_OBJ_P(expr));
				// 释放属性表
				zend_release_properties(properties);
				break;
			}
		// 整数
		case IS_LONG:
			// 整数 添加到 缓存里
			smart_str_append_long(buf, Z_LVAL_P(expr));
			// 跳出
			break;
		// 引用类型
		case IS_REFERENCE:
			// 递归处理引用目标
			zend_print_zval_r_to_buf(buf, Z_REFVAL_P(expr), indent);
			// 跳出
			break;
		// 字串
		case IS_STRING:
			// 把字串添加到缓存里
			smart_str_append(buf, Z_STR_P(expr));
			// 跳出
			break;
		// 其他情况
		default:
			{
				// 表达式转成字串
				zend_string *str = zval_get_string_func(expr);
				// 字串 添加进 缓存
				smart_str_append(buf, str);
				// 释放字串
				zend_string_release_ex(str, 0);
			}
			// 跳出
			break;
	}
}
/* }}} */

// ing4, 把表达式打印进字串里，返回 zend_string。p1:表达式，p2:缩进
ZEND_API zend_string *zend_print_zval_r_to_str(zval *expr, int indent) /* {{{ */
{
	// 智能字串
	smart_str buf = {0};
	// 把表达式打印进智能字串
	zend_print_zval_r_to_buf(&buf, expr, indent);
	// 添加\0
	smart_str_0(&buf);
	// 返回 zend_string
	return buf.s;
}
/* }}} */

// ing4, 打印表达式，p1:表达式，p2:缩进
ZEND_API void zend_print_zval_r(zval *expr, int indent) /* {{{ */
{
	// 把表达式打印进字串里，返回 zend_string。p1:表达式，p2:缩进
	zend_string *str = zend_print_zval_r_to_str(expr, indent);
	// 打印字串
	zend_write(ZSTR_VAL(str), ZSTR_LEN(str));
	// 释放字串
	zend_string_release_ex(str, 0);
}
/* }}} */

// ing4, 二进制只读模式 打开文件。p1:文件名，p2:返回文件名
static FILE *zend_fopen_wrapper(zend_string *filename, zend_string **opened_path) /* {{{ */
{
	// 如果有返回路径
	if (opened_path) {
		// 返回文件名副本
		*opened_path = zend_string_copy(filename);
	}
	// 打开文件，模式：二进制只读
	return fopen(ZSTR_VAL(filename), "rb");
}
/* }}} */

#ifdef ZTS
static bool short_tags_default      = 1;
static uint32_t compiler_options_default = ZEND_COMPILE_DEFAULT;
#else
# define short_tags_default			1
# define compiler_options_default	ZEND_COMPILE_DEFAULT
#endif

// ing3, 设置 默认的 编译时 变量。
static void zend_set_default_compile_time_values(void) /* {{{ */
{
	// 默认编译时间
	/* default compile-time values */
	// 默认1
	CG(short_tags) = short_tags_default;
	//  compiler_options_default = ZEND_COMPILE_DEFAULT = ZEND_COMPILE_HANDLE_OP_ARRAY (1<<2)
	CG(compiler_options) = compiler_options_default;
	// 0 这是什么？
	CG(rtd_key_counter) = 0;
}
/* }}} */

// windows
#ifdef ZEND_WIN32
// ing2, 获取windows版本信息。（测试过，不通，但逻辑没问题）
static void zend_get_windows_version_info(OSVERSIONINFOEX *osvi) /* {{{ */
{
	// OSVERSIONINFOEX 是windowst自带结构体（测试过）
	// 先把内存写成0
	ZeroMemory(osvi, sizeof(OSVERSIONINFOEX));
	// （测试过）
	osvi->dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	// GetVersionEx 是windows 系统函数 （测试不通）
	if(!GetVersionEx((OSVERSIONINFO *) osvi)) {
		ZEND_UNREACHABLE(); /* Should not happen as sizeof is used. */
	}
}
/* }}} */
#endif

// ing3, 初始化 在执行时全局变量中 的 3个 异常操作码
static void zend_init_exception_op(void) /* {{{ */
{
	// 运行时异常操作码 内存写成 0
	memset(EG(exception_op), 0, sizeof(EG(exception_op)));
	// 第一个操作码 ZEND_HANDLE_EXCEPTION
	EG(exception_op)[0].opcode = ZEND_HANDLE_EXCEPTION;
	// 给操作码链接它对应的处理器
	ZEND_VM_SET_OPCODE_HANDLER(EG(exception_op));
	// 第二个操作码 ZEND_HANDLE_EXCEPTION
	EG(exception_op)[1].opcode = ZEND_HANDLE_EXCEPTION;
	// 给操作码链接它对应的处理器
	ZEND_VM_SET_OPCODE_HANDLER(EG(exception_op)+1);
	// 第三个操作码 ZEND_HANDLE_EXCEPTION
	EG(exception_op)[2].opcode = ZEND_HANDLE_EXCEPTION;
	// 给操作码链接它对应的处理器
	ZEND_VM_SET_OPCODE_HANDLER(EG(exception_op)+2);
}
/* }}} */

// ing4, 初始化调用弹跳操作码
static void zend_init_call_trampoline_op(void) /* {{{ */
{
	// 清空弹跳操作码
	memset(&EG(call_trampoline_op), 0, sizeof(EG(call_trampoline_op)));
	// 操作码为 ZEND_CALL_TRAMPOLINE
	EG(call_trampoline_op).opcode = ZEND_CALL_TRAMPOLINE;
	// 给操作码链接它对应的处理器
	ZEND_VM_SET_OPCODE_HANDLER(&EG(call_trampoline_op));
}
/* }}} */

// ing4, 释放zv指向的目标
static void auto_global_dtor(zval *zv) /* {{{ */
{
	// 释放zv指向的目标
	free(Z_PTR_P(zv));
}
/* }}} */

// 线程安全
#ifdef ZTS

// 给函数创建副本真的的蛮复杂的
// ing3, 给zval指向 的函数创建 副本
static void function_copy_ctor(zval *zv) /* {{{ */
{
	// 取得zv里的函数
	zend_function *old_func = Z_FUNC_P(zv);
	zend_function *func;

	// 如果是用户定义函数
	if (old_func->type == ZEND_USER_FUNCTION) {
		// 必须是不可更改
		ZEND_ASSERT(old_func->op_array.fn_flags & ZEND_ACC_IMMUTABLE);
		// 返回
		return;
	}
	// 分配内存，创建新内置函数
	func = pemalloc(sizeof(zend_internal_function), 1);
	// zv关联到新函数
	Z_FUNC_P(zv) = func;
	// 把旧函数复制过来
	memcpy(func, old_func, sizeof(zend_internal_function));
	// 增加新函数引用次数
	function_add_ref(func);
	// 如果 （有 返回类型 或 有类型定义 ） 并且 有参数信息
	if ((old_func->common.fn_flags & (ZEND_ACC_HAS_RETURN_TYPE|ZEND_ACC_HAS_TYPE_HINTS))
	 && old_func->common.arg_info) {
		uint32_t i;
		// 参数数 +1
		uint32_t num_args = old_func->common.num_args + 1;
		// 参数信息 -1
		zend_arg_info *arg_info = old_func->common.arg_info - 1;
		// 
		zend_arg_info *new_arg_info;

		// 如果有字典参数
		if (old_func->common.fn_flags & ZEND_ACC_VARIADIC) {
			// 参数数+1
			num_args++;
		}
		// 分配内存创建参数信息表
		new_arg_info = pemalloc(sizeof(zend_arg_info) * num_args, 1);
		// 把参数信息表复制过来
		memcpy(new_arg_info, arg_info, sizeof(zend_arg_info) * num_args);
		// 遍历参数信息表
		for (i = 0 ; i < num_args; i++) {
			// 如果类型是 列表类型
			if (ZEND_TYPE_HAS_LIST(arg_info[i].type)) {
				// 原类型列表
				zend_type_list *old_list = ZEND_TYPE_LIST(arg_info[i].type);
				// 分配内存创建类型列表
				zend_type_list *new_list = pemalloc(ZEND_TYPE_LIST_SIZE(old_list->num_types), 1);
				// 复制类型列表
				memcpy(new_list, old_list, ZEND_TYPE_LIST_SIZE(old_list->num_types));
				// 更新p1的指针 ((t).ptr = (_ptr))
				ZEND_TYPE_SET_PTR(new_arg_info[i].type, new_list);

				zend_type *list_type;
				// 遍历 新列表
				ZEND_TYPE_LIST_FOREACH(new_list, list_type) {
					// 类型名创建副本
					zend_string *name = zend_string_dup(ZEND_TYPE_NAME(*list_type), 1);
					// 更新p1的指针 ((t).ptr = (_ptr))
					ZEND_TYPE_SET_PTR(*list_type, name);
				} ZEND_TYPE_LIST_FOREACH_END();
			// 如果不是列表类型，但类型有名称
			} else if (ZEND_TYPE_HAS_NAME(arg_info[i].type)) {
				// 复制类型名称
				zend_string *name = zend_string_dup(ZEND_TYPE_NAME(arg_info[i].type), 1);
				// 更新p1的指针 ((t).ptr = (_ptr))
				ZEND_TYPE_SET_PTR(new_arg_info[i].type, name);
			}
		}
		// 参数信息指针，跳过返回值
		func->common.arg_info = new_arg_info + 1;
	}
	// 如果有修饰属性表
	if (old_func->common.attributes) {
		// 
		zend_attribute *old_attr;

		// 清空修饰属性表
		func->common.attributes = NULL;

		// 遍历修饰属性
		ZEND_HASH_PACKED_FOREACH_PTR(old_func->common.attributes, old_attr) {
			uint32_t i;
			zend_attribute *attr;
			// 添加修饰属性，传入，哈希表指针的指针，属性名，参数数量，flags，序号，行号
			attr = zend_add_attribute(&func->common.attributes, old_attr->name, old_attr->argc, old_attr->flags, old_attr->offset, old_attr->lineno);
			// 遍历修饰属性参数表
			for (i = 0 ; i < old_attr->argc; i++) {
				// 复制每个参数
				ZVAL_DUP(&attr->args[i].value, &old_attr->args[i].value);
			}
		} ZEND_HASH_FOREACH_END();
	}
}
/* }}} */

// ing3, 创建自动全局变量副本并应用副本
static void auto_global_copy_ctor(zval *zv) /* {{{ */
{
	// zv里的 自动全局变量
	zend_auto_global *old_ag = (zend_auto_global *) Z_PTR_P(zv);
	// 分配内存创建新的 自动全局变量
	zend_auto_global *new_ag = pemalloc(sizeof(zend_auto_global), 1);

	// 复制名称
	new_ag->name = old_ag->name;
	// 复制回调函数
	new_ag->auto_global_callback = old_ag->auto_global_callback;
	// 复制 jit （opcache里用）
	new_ag->jit = old_ag->jit;
	// 指针指向新的 自动全局变量。（旧的没有销毁）
	Z_PTR_P(zv) = new_ag;
}
/* }}} */

// ing3, 初始化 编译器 全局变量 。p1: zend_compiler_globals 编译器 全局变
static void compiler_globals_ctor(zend_compiler_globals *compiler_globals) /* {{{ */
{
	// 文件名
	compiler_globals->compiled_filename = NULL;
	// 创建函数表
	compiler_globals->function_table = (HashTable *) malloc(sizeof(HashTable));
	// 初始化函数表
	zend_hash_init(compiler_globals->function_table, 1024, NULL, ZEND_FUNCTION_DTOR, 1);
	// 默认是空， static HashTable *global_function_table = NULL;（本文件中定义）
	// 复制全局函数表，使用 function_copy_ctor 给每个函数创建副本
	zend_hash_copy(compiler_globals->function_table, global_function_table, function_copy_ctor);

	// 类表
	compiler_globals->class_table = (HashTable *) malloc(sizeof(HashTable));
	// 初始化类表
	zend_hash_init(compiler_globals->class_table, 64, NULL, ZEND_CLASS_DTOR, 1);
	// 复制全局类表，使用 zend_class_add_ref 给每个类增加引用次数
	zend_hash_copy(compiler_globals->class_table, global_class_table, zend_class_add_ref);

	// 设置 默认的 编译时 变量。
	zend_set_default_compile_time_values();

	// 分配内存创建全局变量表
	compiler_globals->auto_globals = (HashTable *) malloc(sizeof(HashTable));
	// 初始货全局变量表
	zend_hash_init(compiler_globals->auto_globals, 8, NULL, auto_global_dtor, 1);
	// 复制 global_auto_globals_table 表并 使用 auto_global_copy_ctor 给每个全局变量创建副本
	zend_hash_copy(compiler_globals->auto_globals, global_auto_globals_table, auto_global_copy_ctor);

	// 脚本编译列表，为 null
	compiler_globals->script_encoding_list = NULL;
	// 当前链接类，为 null
	compiler_globals->current_linking_class = NULL;

	// 地区区域 会在 运行时 创建和调整大小
	/* Map region is going to be created and resized at run-time. */
	// 默认为null
	compiler_globals->map_ptr_real_base = NULL;
	// 把指针整数值-1，在计算offset时，所有的值最后都会+1。这个指针数值不能直接用。
	compiler_globals->map_ptr_base = ZEND_MAP_PTR_BIASED_BASE(NULL);
	// 默认尺寸是0
	compiler_globals->map_ptr_size = 0;
	// 更新最后位置 global_map_ptr_last
	compiler_globals->map_ptr_last = global_map_ptr_last;
	// 如果这个位置存在
	if (compiler_globals->map_ptr_last) {
		// 分配 map_ptr 表
		/* Allocate map_ptr table */
		// 大小，对齐到4096
		compiler_globals->map_ptr_size = ZEND_MM_ALIGNED_SIZE_EX(compiler_globals->map_ptr_last, 4096);
		// 分配内存创建 指针表
		void *base = pemalloc(compiler_globals->map_ptr_size * sizeof(void*), 1);
		// 把指针表关联到编译时全局变量
		compiler_globals->map_ptr_real_base = base;
		// 把指针整数值-1，在计算offset时，所有的值最后都会+1。这个指针数值不能直接用。
		compiler_globals->map_ptr_base = ZEND_MAP_PTR_BIASED_BASE(base);
		// 把新建的指针列表全置成 0
		memset(base, 0, compiler_globals->map_ptr_last * sizeof(void*));
	}
}
/* }}} */

// ing4, 销毁 编译全局变量 p1:全局变量实例 
static void compiler_globals_dtor(zend_compiler_globals *compiler_globals) /* {{{ */
{
	// 如果不是 global_function_table 。 （线程安全）
	// # define GLOBAL_FUNCTION_TABLE global_function_table
	if (compiler_globals->function_table != GLOBAL_FUNCTION_TABLE) {
		// 销毁 编译函数表
		zend_hash_destroy(compiler_globals->function_table);
		// 释放 编译函数表
		free(compiler_globals->function_table);
	}
	
	// 如果不是 global_class_table。（线程安全）
	// # define GLOBAL_CLASS_TABLE global_class_table
	if (compiler_globals->class_table != GLOBAL_CLASS_TABLE) {
		// 子类可能从父类复用结构，所以反序销毁
		/* Child classes may reuse structures from parent classes, so destroy in reverse order. */
		// 反序销毁 类表
		zend_hash_graceful_reverse_destroy(compiler_globals->class_table);
		// 释放类表
		free(compiler_globals->class_table);
	}
	
	// 如果不是用 global_auto_globals_table。（线程安全）
	// # define GLOBAL_AUTO_GLOBALS_TABLE global_auto_globals_table
	if (compiler_globals->auto_globals != GLOBAL_AUTO_GLOBALS_TABLE) {
		// 销毁哈希表
		zend_hash_destroy(compiler_globals->auto_globals);
		// 释放哈希表
		free(compiler_globals->auto_globals);
	}
	// 如果有 脚本编码列表
	if (compiler_globals->script_encoding_list) {
		// 释放 脚本编码列表
		pefree((char*)compiler_globals->script_encoding_list, 1);
	}
	// 有 map_ptr 指针列表
	if (compiler_globals->map_ptr_real_base) {
		// 释放 map_ptr 指针列表
		free(compiler_globals->map_ptr_real_base);
		// 列表指针为null
		compiler_globals->map_ptr_real_base = NULL;
		// 把指针整数值-1，在计算offset时，所有的值最后都会+1。这个指针数值不能直接用。
		compiler_globals->map_ptr_base = ZEND_MAP_PTR_BIASED_BASE(NULL);
		// 尺寸为 0
		compiler_globals->map_ptr_size = 0;
	}
}
/* }}} */

static void executor_globals_ctor(zend_executor_globals *executor_globals) /* {{{ */
{
	// 开启常量功能
	zend_startup_constants();
	// 复制整个常量哈希表
	zend_copy_constants(executor_globals->zend_constants, GLOBAL_CONSTANTS_TABLE);
	// 初始化资源列表
	zend_init_rsrc_plist();
	// 初始化异常操作码
	zend_init_exception_op();
	// 初始化 弹跳操作码
	zend_init_call_trampoline_op();
	// 清空弹跳函数
	memset(&executor_globals->trampoline, 0, sizeof(zend_op_array));
	// ？？
	executor_globals->capture_warnings_during_sccp = 0;
	ZVAL_UNDEF(&executor_globals->user_error_handler);
	ZVAL_UNDEF(&executor_globals->user_exception_handler);
	executor_globals->in_autoload = NULL;
	executor_globals->current_execute_data = NULL;
	executor_globals->current_module = NULL;
	executor_globals->exit_status = 0;
// 
#if XPFPA_HAVE_CW
	executor_globals->saved_fpu_cw = 0;
#endif
	executor_globals->saved_fpu_cw_ptr = NULL;
	executor_globals->active = 0;
	executor_globals->bailout = NULL;
	executor_globals->error_handling  = EH_NORMAL;
	executor_globals->exception_class = NULL;
	executor_globals->exception = NULL;
	executor_globals->objects_store.object_buckets = NULL;
	executor_globals->current_fiber_context = NULL;
	executor_globals->main_fiber_context = NULL;
	executor_globals->active_fiber = NULL;
// windows系统
#ifdef ZEND_WIN32
	// 获取windows版本信息。
	zend_get_windows_version_info(&executor_globals->windows_version_info);
#endif
	executor_globals->flags = EG_FLAGS_INITIAL;
	executor_globals->record_errors = false;
	executor_globals->num_errors = 0;
	executor_globals->errors = NULL;
// 
#ifdef ZEND_MAX_EXECUTION_TIMERS
	executor_globals->pid = 0;
	executor_globals->oldact = (struct sigaction){0};
#endif
}
/* }}} */

// ing4, 执行全局销毁. p1:执行时全局变量
static void executor_globals_dtor(zend_executor_globals *executor_globals) /* {{{ */
{
	// 销毁ini配置指令
	zend_ini_dtor(executor_globals->ini_directives);

	// 如果永久列表 不同于 global_persistent_list
	if (&executor_globals->persistent_list != global_persistent_list) {
		// 销毁资源列表 
		zend_destroy_rsrc_list(&executor_globals->persistent_list);
	}
	// 如果常量表不同于系统 常量表
	if (executor_globals->zend_constants != GLOBAL_CONSTANTS_TABLE) {
		// 销毁常量表
		zend_hash_destroy(executor_globals->zend_constants);
		// 释放常量表
		free(executor_globals->zend_constants);
	}
}
/* }}} */

// ing2, 新线程的最终处理
static void zend_new_thread_end_handler(THREAD_T thread_id) /* {{{ */
{
	// 创建 EG(ini_directives)表 并从 registered_zend_ini_directives表复制元素
	zend_copy_ini_directives();
	// 刷新所有选项（依次触发 on_modify 事件）
	zend_ini_refresh_caches(ZEND_INI_STAGE_STARTUP);
	// windows 好像没用？
	zend_max_execution_timer_init();
}
/* }}} */
#endif

#if defined(__FreeBSD__) || defined(__DragonFly__)
/* FreeBSD and DragonFly floating point precision fix */
#include <floatingpoint.h>
#endif

// ing4, 把 zend_ini_scanner_globals 内存 全写成0
static void ini_scanner_globals_ctor(zend_ini_scanner_globals *scanner_globals_p) /* {{{ */
{
	memset(scanner_globals_p, 0, sizeof(*scanner_globals_p));
}
/* }}} */

// ing4, 把 zend_php_scanner_globals 内存 全写成0
static void php_scanner_globals_ctor(zend_php_scanner_globals *scanner_globals_p) /* {{{ */
{
	memset(scanner_globals_p, 0, sizeof(*scanner_globals_p));
}
/* }}} */

// ing4, 销毁zval指向的模块
static void module_destructor_zval(zval *zv) /* {{{ */
{
	// 取出模块
	zend_module_entry *module = (zend_module_entry*)Z_PTR_P(zv);
	// 销毁模块
	module_destructor(module);
}
/* }}} */

// ing4, 实际上，不在这里访问 $GLOBALS
static bool php_auto_globals_create_globals(zend_string *name) /* {{{ */
{
	// 已经把 $GLOBALS 注册成自动全局变量，不需要再为它创建一个实际变量。
	// 对它的访问由编译器来作特殊处理
	/* While we keep registering $GLOBALS as an auto-global, we do not create an
	 * actual variable for it. Access to it handled specially by the compiler. */
	 
	 // 返回0
	return 0;
}
/* }}} */

// 160行
// 
void zend_startup(zend_utility_functions *utility_functions) /* {{{ */
{
// 线程安全
#ifdef ZTS
	// 
	zend_compiler_globals *compiler_globals;
	zend_executor_globals *executor_globals;
	extern ZEND_API ts_rsrc_id ini_scanner_globals_id;
	extern ZEND_API ts_rsrc_id language_scanner_globals_id;
// 无线程安全
#else
	extern zend_ini_scanner_globals ini_scanner_globals;
	extern zend_php_scanner_globals language_scanner_globals;
#endif
	// 这个有点太底层了？
	zend_cpu_startup();

// windows
#ifdef ZEND_WIN32
	php_win32_cp_set_by_id(65001);
#endif

	// 开始内存管理
	start_memory_manager();

	virtual_cwd_startup(); /* Could use shutdown to free the main cwd but it would just slow it down for CGI */

// freebsd 或 。。。
#if defined(__FreeBSD__) || defined(__DragonFly__)
	/* FreeBSD and DragonFly floating point precision fix */
	fpsetmask(0);
#endif

	zend_startup_strtod();
	zend_startup_extensions_mechanism();

	/* Set up utility functions and values */
	zend_error_cb = utility_functions->error_function;
	zend_printf = utility_functions->printf_function;
	zend_write = utility_functions->write_function;
	zend_fopen = utility_functions->fopen_function;
	if (!zend_fopen) {
		zend_fopen = zend_fopen_wrapper;
	}
	zend_stream_open_function = utility_functions->stream_open_function;
	zend_message_dispatcher_p = utility_functions->message_handler;
	zend_get_configuration_directive_p = utility_functions->get_configuration_directive;
	zend_ticks_function = utility_functions->ticks_function;
	zend_on_timeout = utility_functions->on_timeout;
	zend_printf_to_smart_string = utility_functions->printf_to_smart_string_function;
	zend_printf_to_smart_str = utility_functions->printf_to_smart_str_function;
	zend_getenv = utility_functions->getenv_function;
	zend_resolve_path = utility_functions->resolve_path_function;

	zend_interrupt_function = NULL;

#ifdef HAVE_DTRACE
/* build with dtrace support */
	{
		char *tmp = getenv("USE_ZEND_DTRACE");

		if (tmp && ZEND_ATOL(tmp)) {
			zend_dtrace_enabled = 1;
			zend_compile_file = dtrace_compile_file;
			zend_execute_ex = dtrace_execute_ex;
			zend_execute_internal = dtrace_execute_internal;

			zend_observer_error_register(dtrace_error_notify_cb);
		} else {
			zend_compile_file = compile_file;
			zend_execute_ex = execute_ex;
			zend_execute_internal = NULL;
		}
	}
#else
	zend_compile_file = compile_file;
	zend_execute_ex = execute_ex;
	zend_execute_internal = NULL;
#endif /* HAVE_DTRACE */
	zend_compile_string = compile_string;
	zend_throw_exception_hook = NULL;

	/* Set up the default garbage collection implementation. */
	gc_collect_cycles = zend_gc_collect_cycles;

	zend_vm_init();

	/* set up version */
	zend_version_info = strdup(ZEND_CORE_VERSION_INFO);
	zend_version_info_length = sizeof(ZEND_CORE_VERSION_INFO) - 1;

	GLOBAL_FUNCTION_TABLE = (HashTable *) malloc(sizeof(HashTable));
	GLOBAL_CLASS_TABLE = (HashTable *) malloc(sizeof(HashTable));
	GLOBAL_AUTO_GLOBALS_TABLE = (HashTable *) malloc(sizeof(HashTable));
	GLOBAL_CONSTANTS_TABLE = (HashTable *) malloc(sizeof(HashTable));

	zend_hash_init(GLOBAL_FUNCTION_TABLE, 1024, NULL, ZEND_FUNCTION_DTOR, 1);
	zend_hash_init(GLOBAL_CLASS_TABLE, 64, NULL, ZEND_CLASS_DTOR, 1);
	zend_hash_init(GLOBAL_AUTO_GLOBALS_TABLE, 8, NULL, auto_global_dtor, 1);
	zend_hash_init(GLOBAL_CONSTANTS_TABLE, 128, NULL, ZEND_CONSTANT_DTOR, 1);

	zend_hash_init(&module_registry, 32, NULL, module_destructor_zval, 1);
	zend_init_rsrc_list_dtors();

// 线程安全
#ifdef ZTS
	ts_allocate_fast_id(&compiler_globals_id, &compiler_globals_offset, sizeof(zend_compiler_globals), (ts_allocate_ctor) compiler_globals_ctor, (ts_allocate_dtor) compiler_globals_dtor);
	ts_allocate_fast_id(&executor_globals_id, &executor_globals_offset, sizeof(zend_executor_globals), (ts_allocate_ctor) executor_globals_ctor, (ts_allocate_dtor) executor_globals_dtor);
	ts_allocate_fast_id(&language_scanner_globals_id, &language_scanner_globals_offset, sizeof(zend_php_scanner_globals), (ts_allocate_ctor) php_scanner_globals_ctor, NULL);
	ts_allocate_fast_id(&ini_scanner_globals_id, &ini_scanner_globals_offset, sizeof(zend_ini_scanner_globals), (ts_allocate_ctor) ini_scanner_globals_ctor, NULL);
	compiler_globals = ts_resource(compiler_globals_id);
	executor_globals = ts_resource(executor_globals_id);

	compiler_globals_dtor(compiler_globals);
	compiler_globals->in_compilation = 0;
	compiler_globals->function_table = (HashTable *) malloc(sizeof(HashTable));
	compiler_globals->class_table = (HashTable *) malloc(sizeof(HashTable));

	*compiler_globals->function_table = *GLOBAL_FUNCTION_TABLE;
	*compiler_globals->class_table = *GLOBAL_CLASS_TABLE;
	compiler_globals->auto_globals = GLOBAL_AUTO_GLOBALS_TABLE;

	zend_hash_destroy(executor_globals->zend_constants);
	*executor_globals->zend_constants = *GLOBAL_CONSTANTS_TABLE;
#else
	ini_scanner_globals_ctor(&ini_scanner_globals);
	php_scanner_globals_ctor(&language_scanner_globals);
	// 设置 默认的 编译时 变量。
	zend_set_default_compile_time_values();
#ifdef ZEND_WIN32
	// 获取windows版本信息。
	zend_get_windows_version_info(&EG(windows_version_info));
#endif
	// map指针列表
	/* Map region is going to be created and resized at run-time. */
	CG(map_ptr_real_base) = NULL;
	CG(map_ptr_base) = ZEND_MAP_PTR_BIASED_BASE(NULL);
	CG(map_ptr_size) = 0;
	CG(map_ptr_last) = 0;
#endif
	EG(error_reporting) = E_ALL & ~E_NOTICE;

	zend_interned_strings_init();
	zend_startup_builtin_functions();
	zend_register_standard_constants();
	zend_register_auto_global(zend_string_init_interned("GLOBALS", sizeof("GLOBALS") - 1, 1), 1, php_auto_globals_create_globals);

// 线程安全
#ifndef ZTS
	// 初始化 资源列表
	zend_init_rsrc_plist();
	// 初始化异常操作码
	zend_init_exception_op();
	// 初始化调用弹跳操作码
	zend_init_call_trampoline_op();
#endif

	// 启动 ini配置
	zend_ini_startup();

#ifdef ZEND_WIN32
	/* Uses INI settings, so needs to be run after it. */
	php_win32_cp_setup();
#endif

	zend_optimizer_startup();

// 线程安全
#ifdef ZTS
	tsrm_set_new_thread_end_handler(zend_new_thread_end_handler);
	tsrm_set_shutdown_handler(zend_interned_strings_dtor);
#endif
}
/* }}} */

// ing4, 永久注册标准ini条目
void zend_register_standard_ini_entries(void) /* {{{ */
{
	// 注册ini条目。p1:定义的ini条目，p2:模块号，p3:模块类型
	zend_register_ini_entries_ex(ini_entries, 0, MODULE_PERSISTENT);
}
/* }}} */


// 删除 类/函数/常量表 的 全局的 (r/o) 副本。在启动线程中 使用新的 r/w 副本
/* Unlink the global (r/o) copies of the class, function and constant tables,
 * and use a fresh r/w copy for the startup thread
 */
// 这是启动后要做的一些操作 php_module_startup ：main/main.c 调用
zend_result zend_post_startup(void) /* {{{ */
{
	// 线程安全
#ifdef ZTS
	// 编码表
	zend_encoding **script_encoding_list;

	// /TSRM/TSRM.h
	// 
	zend_compiler_globals *compiler_globals = ts_resource(compiler_globals_id);
	zend_executor_globals *executor_globals = ts_resource(executor_globals_id);
#endif

	// 已启动
	startup_done = true;

	// 后启动回调
	if (zend_post_startup_cb) {
		zend_result (*cb)(void) = zend_post_startup_cb;

		zend_post_startup_cb = NULL;
		if (cb() != SUCCESS) {
			return FAILURE;
		}
	}

// 线程安全
#ifdef ZTS
	*GLOBAL_FUNCTION_TABLE = *compiler_globals->function_table;
	*GLOBAL_CLASS_TABLE = *compiler_globals->class_table;
	*GLOBAL_CONSTANTS_TABLE = *executor_globals->zend_constants;
	global_map_ptr_last = compiler_globals->map_ptr_last;

	short_tags_default = CG(short_tags);
	compiler_options_default = CG(compiler_options);

	zend_destroy_rsrc_list(&EG(persistent_list));
	free(compiler_globals->function_table);
	compiler_globals->function_table = NULL;
	free(compiler_globals->class_table);
	compiler_globals->class_table = NULL;
	if (compiler_globals->map_ptr_real_base) {
		free(compiler_globals->map_ptr_real_base);
	}
	compiler_globals->map_ptr_real_base = NULL;
	compiler_globals->map_ptr_base = ZEND_MAP_PTR_BIASED_BASE(NULL);
	if ((script_encoding_list = (zend_encoding **)compiler_globals->script_encoding_list)) {
		compiler_globals_ctor(compiler_globals);
		compiler_globals->script_encoding_list = (const zend_encoding **)script_encoding_list;
	} else {
		compiler_globals_ctor(compiler_globals);
	}
	free(EG(zend_constants));
	EG(zend_constants) = NULL;

	executor_globals_ctor(executor_globals);
	global_persistent_list = &EG(persistent_list);
	zend_copy_ini_directives();
#else
	// 
	global_map_ptr_last = CG(map_ptr_last);
#endif

	return SUCCESS;
}
/* }}} */

void zend_shutdown(void) /* {{{ */
{
	// 销毁虚拟机
	zend_vm_dtor();
	// 销毁资源 列表
	zend_destroy_rsrc_list(&EG(persistent_list));
	// 销毁模块
	zend_destroy_modules();

	// 反激活 ？？？
	virtual_cwd_deactivate();
	virtual_cwd_shutdown();

	// 销毁全局函数表
	zend_hash_destroy(GLOBAL_FUNCTION_TABLE);
	// 子类可能从父类复用结构，所以反序销毁
	/* Child classes may reuse structures from parent classes, so destroy in reverse order. */
	// 反序销毁 全局类表
	zend_hash_graceful_reverse_destroy(GLOBAL_CLASS_TABLE);

	// 销毁自动全局变量表
	zend_hash_destroy(GLOBAL_AUTO_GLOBALS_TABLE);
	// 释放自动全局变量表
	free(GLOBAL_AUTO_GLOBALS_TABLE);

	// 关闭扩展
	zend_shutdown_extensions();
	// 释放版本信息
	free(zend_version_info);
	// 释放全局函数表
	free(GLOBAL_FUNCTION_TABLE);
	// 释放全局类表
	free(GLOBAL_CLASS_TABLE);

	// 销毁全局常量表
	zend_hash_destroy(GLOBAL_CONSTANTS_TABLE);
	// 释放全局常量表
	free(GLOBAL_CONSTANTS_TABLE);
	// 关闭 字串转小数 ？
	zend_shutdown_strtod();
	// 关闭修饰属性
	zend_attributes_shutdown();

// 线程安全
#ifdef ZTS
	GLOBAL_FUNCTION_TABLE = NULL;
	GLOBAL_CLASS_TABLE = NULL;
	GLOBAL_AUTO_GLOBALS_TABLE = NULL;
	GLOBAL_CONSTANTS_TABLE = NULL;
	ts_free_id(executor_globals_id);
	ts_free_id(compiler_globals_id);
// 非线程安全
#else
	if (CG(map_ptr_real_base)) {
		free(CG(map_ptr_real_base));
		CG(map_ptr_real_base) = NULL;
		CG(map_ptr_base) = ZEND_MAP_PTR_BIASED_BASE(NULL);
		CG(map_ptr_size) = 0;
	}
	// 如果有编码列表
	if (CG(script_encoding_list)) {
		free(ZEND_VOIDP(CG(script_encoding_list)));
		CG(script_encoding_list) = NULL;
		CG(script_encoding_list_size) = 0;
	}
#endif
	// 销毁资源列表
	zend_destroy_rsrc_list_dtors();

	// 关闭优化器 ？
	zend_optimizer_shutdown();
	// 未启动
	startup_done = false;
}
/* }}} */

// ing4, 设置全局变量 zend_uv （ zend_utility_values 结构）
void zend_set_utility_values(zend_utility_values *utility_values) /* {{{ */
{
	zend_uv = *utility_values;
}
/* }}} */

/* this should be compatible with the standard zenderror */
// ing4, 抛出异常，第一个有效
ZEND_COLD void zenderror(const char *error) /* {{{ */
{
	CG(parse_error) = 0;
	// 如果已经有exception了，就不再抛错了
	if (EG(exception)) {
		// 有一个抛出的exception ，不要让解析器再抛另一个
		/* An exception was thrown in the lexer, don't throw another in the parser. */
		return;
	}
	//没有异常的话，这里抛出。
	zend_throw_exception(zend_ce_parse_error, error, 0);
}
/* }}} */

// ing2, 跳伞，p1:文件名，p2:行号
ZEND_API ZEND_COLD ZEND_NORETURN void _zend_bailout(const char *filename, uint32_t lineno) /* {{{ */
{
	// 执行时没有 bailout
	if (!EG(bailout)) {
		// 输出调试信息 ，没有跳伞地址的跳伞
		zend_output_debug_string(1, "%s(%d) : Bailed out without a bailout address!", filename, lineno);
		exit(-1);
	}
	// 开启垃圾回收保护
	gc_protect(1);
	// 标记，未清理的停止
	CG(unclean_shutdown) = 1;
	// 清空 当前活跃的类入口
	CG(active_class_entry) = NULL;
	// 标记，已完成
	CG(in_compilation) = 0;
	// 清空 当前执行数据
	EG(current_execute_data) = NULL;
	//  longjmp 系统函数是做什么的 ？
	LONGJMP(*EG(bailout), FAILURE);
}
/* }}} */

// ing3, windows 系统返回4096
ZEND_API size_t zend_get_page_size(void)
{
// 如果是windows，从system_info里获取大小，值为4096（测试过）
#ifdef _WIN32
	SYSTEM_INFO system_info;
	GetSystemInfo(&system_info);
	return system_info.dwPageSize;
// freebsd
#elif defined(__FreeBSD__)
	/* This returns the value obtained from
	 * the auxv vector, avoiding a syscall. */
	return getpagesize();
// 其他操作系统 
#else
	return (size_t) sysconf(_SC_PAGESIZE);
#endif
}

// ing4, 添加扩展信息
ZEND_API void zend_append_version_info(const zend_extension *extension) /* {{{ */
{
	char *new_info;
	uint32_t new_info_length;
	// 计算信息长度
	new_info_length = (uint32_t)(sizeof("    with  v, , by \n")
						+ strlen(extension->name)
						+ strlen(extension->version)
						+ strlen(extension->copyright)
						+ strlen(extension->author));
	// 分配内存
	new_info = (char *) malloc(new_info_length + 1);
	// 把信息打印到缓冲区里
	snprintf(new_info, new_info_length, "    with %s v%s, %s, by %s\n", extension->name, extension->version, extension->copyright, extension->author);
	// 改变 zend_version_info 实例大小
	zend_version_info = (char *) realloc(zend_version_info, zend_version_info_length+new_info_length + 1);
	// 把新的信息添加进去
	strncat(zend_version_info, new_info, new_info_length);
	// 改变长度
	zend_version_info_length += new_info_length;
	// 释放临时缓冲区
	free(new_info);
}
/* }}} */

// ing4, 返回版本信息
ZEND_API const char *get_zend_version(void) /* {{{ */
{
	return zend_version_info;
}
/* }}} */

// ing3, 激活
ZEND_API void zend_activate(void) /* {{{ */
{
// 线程安全
#ifdef ZTS
	// 激活 virtual_cwd？
	virtual_cwd_activate();
#endif
	// 重置垃圾回收
	gc_reset();
	// 初始化编译器
	init_compiler();
	// 初始化执行器
	init_executor();
	// 开启扫描器
	startup_scanner();
	// 如果有用到指针列表
	if (CG(map_ptr_last)) {
		// 指针列表全都清0
		memset(CG(map_ptr_real_base), 0, CG(map_ptr_last) * sizeof(void*));
	}
	// 给所有内置函数添加运行时缓存（指针表）
	zend_init_internal_run_time_cache();
	// 激活观察者
	zend_observer_activate();
}
/* }}} */

// ing4,调用所有销毁器
void zend_call_destructors(void) /* {{{ */
{
	zend_try {
		// Zend/zend_execute_API.c : 250
		// 关闭销毁器。销毁符号表和对象仓库
		shutdown_destructors();
	} zend_end_try();
}
/* }}} */

// ing3, 反激活
ZEND_API void zend_deactivate(void) /* {{{ */
{
	// 不再执行任何内容
	/* we're no longer executing anything */
	// 清空执行上下文
	EG(current_execute_data) = NULL;
	// 
	zend_try {
		// 关闭扫描器
		shutdown_scanner();
	} zend_end_try();

	// shutdown_executor() 处理它自己的跳伞操作
	/* shutdown_executor() takes care of its own bailout handling */
	// 关闭执行器
	shutdown_executor();

	zend_try {
		// 反激活ini解析功能
		zend_ini_deactivate();
	} zend_end_try();

	zend_try {
		// 关闭编译器
		shutdown_compiler();
	} zend_end_try();
	// 销毁资源指针列表
	zend_destroy_rsrc_list(&EG(regular_list));

	/* See GH-8646: https://github.com/php/php-src/issues/8646
	 *
	 保存类入口的内置字串会 有一个对应的 在 map_ptr 中的位置，来存放 类入口缓存
	 * Interned strings that hold class entries can get a corresponding slot in map_ptr for the CE cache.
	 ？？
	 * map_ptr works like a bump allocator: there is a counter which increases to allocate the next slot in the map.
	 *
	 对于没有操作码缓存的类名：
	 * For class name strings in non-opcache we have:
	 启动时：使用永久+内置 字串
	 *   - on startup: permanent + interned
	 请求时：使用内置字串
	 *   - on request: interned
	 
	 对于有操作码缓存的类名：
	 * For class name strings in opcache we have:
	 启动时：使用永久+内置 字串
	 *   - on startup: permanent + interned
	 请求时：？
	 *   - on request: either not interned at all, which we can ignore because they won't get a CE cache entry
	 *                 or they were already permanent + interned
	 *                 or we get a new permanent + interned string in the opcache persistence code
	 *
	 注意，map_ptr 的布局总会先保证永久字串，然后才是请求字串。
	 没有操作码缓存时，请求字串会在 map_ptr 中占有一个位置，这个请求字串会在请求结束时销毁。
	 对应的  map_ptr 位置就不能再用了。这会让 map_ptr 越来越大。
	 * Notice that the map_ptr layout always has the permanent strings first, and the request strings after.
	 * In non-opcache, a request string may get a slot in map_ptr, and that interned request string
	 * gets destroyed at the end of the request. The corresponding map_ptr slot can thereafter never be used again.
	 * This causes map_ptr to keep reallocating to larger and larger sizes.
	 *
	 这样解决这个问题：
	 * We solve it as follows:
	 可以检查是否有内置请求字串，它们只会在 没有操作码缓存 时产生。
	 * We can check whether we had any interned request strings, which only happens in non-opcache.
	 如果有，把 map_ptr 复位到最后的 永久字串。
	 * If we have any, we reset map_ptr to the last permanent string.
	 由于 map_ptr 的布局，不会丢失任何 永久字串
	 * We can't lose any permanent strings because of map_ptr's layout.
	 */
	// 如果有内部字串
	if (zend_hash_num_elements(&CG(interned_strings)) > 0) {
		// 重置 指针列表
		zend_map_ptr_reset();
	}
// 默认是0 
#if GC_BENCH
	// 向 stderr 输出 GC 统计数据
	fprintf(stderr, "GC Statistics\n");
	fprintf(stderr, "-------------\n");
	fprintf(stderr, "Runs:               %d\n", GC_G(gc_runs));
	fprintf(stderr, "Collected:          %d\n", GC_G(collected));
	fprintf(stderr, "Root buffer length: %d\n", GC_G(root_buf_length));
	fprintf(stderr, "Root buffer peak:   %d\n\n", GC_G(root_buf_peak));
	fprintf(stderr, "      Possible            Remove from  Marked\n");
	fprintf(stderr, "        Root    Buffered     buffer     grey\n");
	fprintf(stderr, "      --------  --------  -----------  ------\n");
	fprintf(stderr, "ZVAL  %8d  %8d  %9d  %8d\n", GC_G(zval_possible_root), GC_G(zval_buffered), GC_G(zval_remove_from_buffer), GC_G(zval_marked_grey));
#endif
}
/* }}} */

// ing3, 分派消息
ZEND_API void zend_message_dispatcher(zend_long message, const void *data) /* {{{ */
{
	// 如果有分配函数，最终调用 php_message_handler_for_zend ：main.c
	if (zend_message_dispatcher_p) {
		// 调用分派函数 
		zend_message_dispatcher_p(message, data);
	}
}
/* }}} */

// ing3, 反激活配置信息
ZEND_API zval *zend_get_configuration_directive(zend_string *name) /* {{{ */
{
	// 如果有反激活函数，最终调用 php_get_configuration_directive_for_zend：main.c
	if (zend_get_configuration_directive_p) {
		// 调用函数 
		return zend_get_configuration_directive_p(name);
	} else {
		return NULL;
	}
}
/* }}} */

// ing4, 把编译时堆栈里的数据存储到当前堆栈，再把编译时堆栈置空
#define SAVE_STACK(stack) do { \
		/* 如果 编译时 堆栈 里有元素 */ \
		if (CG(stack).top) { \
			/* 把编译时堆栈里的数据复制到 当前 堆栈 */ \
			memcpy(&stack, &CG(stack), sizeof(zend_stack)); \
			/* 清空 编译时堆栈 */ \
			CG(stack).top = CG(stack).max = 0; \
			/* 清空元素指针 */ \
			CG(stack).elements = NULL; \
		/* 里面没有元素 */ \
		} else { \
			/* 新堆栈标记成空 */ \
			stack.top = 0; \
		} \
	} while (0)

// ing4, 恢复堆栈数据到 编译时堆栈 CG(stack)
#define RESTORE_STACK(stack) do { \
		/* 如果堆栈有元素，才有必要恢复 */ \
		if (stack.top) { \
			/* 清空编译时堆栈 */ \
			zend_stack_destroy(&CG(stack)); \
			/* 把数据复制到 编译时堆栈 */ \
			memcpy(&CG(stack), &stack, sizeof(zend_stack)); \
		} \
	} while (0)

// 170行
// ing3, 尝试用用户的方法来处理错误，如果失败，使用内置方法处理。p1:错误码，p2:文件名，p3:错误行号，p4:报错信息
ZEND_API ZEND_COLD void zend_error_zstr_at(
		int orig_type, zend_string *error_filename, uint32_t error_lineno, zend_string *message)
{
	// 4个参数
	zval params[4];
	zval retval;
	zval orig_user_error_handler;
	bool in_compilation;
	zend_class_entry *saved_class_entry;
	zend_stack loop_var_stack;
	zend_stack delayed_oplines_stack;
	// 取得系统支持的错误类型
	int type = orig_type & E_ALL;
	bool orig_record_errors;
	uint32_t orig_num_errors;
	zend_error_info **orig_errors;
	zend_result res;

	// 如果我们通过SCCP执行一个函数，要对所有发出的警告计数，但不调用其他的异常处理
	/* If we're executing a function during SCCP, count any warnings that may be emitted,
	 * but don't perform any other error handling. */
	 
	 // 如果 有sccp 中的警告
	if (EG(capture_warnings_during_sccp)) {
		// 类型不可以是 致使错误
		ZEND_ASSERT(!(type & E_FATAL_ERRORS) && "Fatal error during SCCP");
		// 警告数量 +1;
		EG(capture_warnings_during_sccp)++;
		// 
		return;
	}

	// 如果开启了 记录错误
	if (EG(record_errors)) {
		// 分配内存创建错误信息
		zend_error_info *info = emalloc(sizeof(zend_error_info));
		// 类型
		info->type = type;
		// 行号
		info->lineno = error_lineno;
		// 文件名
		info->filename = zend_string_copy(error_filename);
		// 错误消息
		info->message = zend_string_copy(message);

		// 对于大量错误，这样是不够的，可以使用 pow2 计算分配大小 来解决这个问题
		/* This is very inefficient for a large number of errors.
		 * Use pow2 realloc if it becomes a problem. */
		// 数量 +1
		EG(num_errors)++;
		// 分配一个指针
		EG(errors) = erealloc(EG(errors), sizeof(zend_error_info*) * EG(num_errors));
		// 把错误信息指针放进去
		EG(errors)[EG(num_errors)-1] = info;
	}

	// 在发生致命错误时，报告未捕获异常
	/* Report about uncaught exception in case of fatal errors */
	// 如果有异常
	if (EG(exception)) {
		zend_execute_data *ex;
		const zend_op *opline;
		
		// 如果是致命错误
		if (type & E_FATAL_ERRORS) {
			// 当前执行数据
			ex = EG(current_execute_data);
			// 操作码为空
			opline = NULL;
			// 如果执行数据有效 并且 （没有函数 或有用户函数）
			while (ex && (!ex->func || !ZEND_USER_CODE(ex->func->type))) {
				// 找到前一个执行数据
				ex = ex->prev_execute_data;
			}
			// 如果执行数据有效 并且 操作码是 ZEND_HANDLE_EXCEPTION 并且 有记录抛错前的操作码
			if (ex && ex->opline->opcode == ZEND_HANDLE_EXCEPTION &&
			    EG(opline_before_exception)) {
				// 取出记录的 操作码
				opline = EG(opline_before_exception);
			}
			// 报错，类型为警告
			zend_exception_error(EG(exception), E_WARNING);
			// 清空当前异常指针
			EG(exception) = NULL;
			// 如果有抛错前的操作码
			if (opline) {
				// 让它作为当前操作码
				ex->opline = opline;
			}
		}
	}

	// 依次调用每个 出错回调 函数
	zend_observer_error_notify(type, error_filename, error_lineno, message);

	// 如果没有用户定义的 处理器 
	/* if we don't have a user defined error handler */
	// 没用用户定义的处理器
	if (Z_TYPE(EG(user_error_handler)) == IS_UNDEF
		// 或 用户处理的错误没有此类型 ???
		|| !(EG(user_error_handler_error_reporting) & type)
		// 或 不用默认方法处理（需要抛出错误） ???
		|| EG(error_handling) != EH_NORMAL) {
		// 使用错误回调方法
		zend_error_cb(orig_type, error_filename, error_lineno, message);
	// 否则 ，根据错误类型处理
	} else switch (type) {
		// 错误，解析错误，内核错误，内核警告，编译错误，编译警告
		case E_ERROR:
		case E_PARSE:
		case E_CORE_ERROR:
		case E_CORE_WARNING:
		case E_COMPILE_ERROR:
		case E_COMPILE_WARNING:
			// 在用户空间里处理错误可能不安全
			/* The error may not be safe to handle in user-space */
			// 使用错误回调方法
			zend_error_cb(orig_type, error_filename, error_lineno, message);
			// 报错
			break;
		// 默认情况
		default:
			// 在用户空间里处理错误
			/* Handle the error in user space */
			// 错误信息复制到第二个参数中
			ZVAL_STR_COPY(&params[1], message);
			// 第一个参数放类型
			ZVAL_LONG(&params[0], type);

			// 如果有 报错文件名
			if (error_filename) {
				// 参数3为文件名
				ZVAL_STR_COPY(&params[2], error_filename);
			// 没有文件名
			} else {
				// 参数3为null
				ZVAL_NULL(&params[2]);
			}

			// 参数4为行号
			ZVAL_LONG(&params[3], error_lineno);

			// 先把户的处理函数复制到 临时变量里
			ZVAL_COPY_VALUE(&orig_user_error_handler, &EG(user_error_handler));
			// 清空全局变量里的  户的处理函数
			ZVAL_UNDEF(&EG(user_error_handler));

			// 用户的错误处理方法可能 会 include() 附加的php文件。
			// 如果在编译过程中出错，PHP会递归编译这个文件，但一些 CG() 变量会不一致。
			/* User error handler may include() additional PHP files.
			 * If an error was generated during compilation PHP will compile
			 * such scripts recursively, but some CG() variables may be
			 * inconsistent. */

			// 当前是否完成
			in_compilation = CG(in_compilation);
			// 如果完成
			if (in_compilation) {
				// 取出 当前活动的类入口
				saved_class_entry = CG(active_class_entry);
				// 清空 当前活动的类入口
				CG(active_class_entry) = NULL;
				// 循环变量堆栈
				// 把编译时堆栈里的数据存储到当前堆栈，再把编译时堆栈置空
				SAVE_STACK(loop_var_stack);
				// 延时操作码堆栈
				// 把编译时堆栈里的数据存储到当前堆栈，再把编译时堆栈置空
				SAVE_STACK(delayed_oplines_stack);
				// 已完成
				CG(in_compilation) = 0;
			}

			// 原错误记录状态
			orig_record_errors = EG(record_errors);
			// 原错误数
			orig_num_errors = EG(num_errors);
			// 原错误
			orig_errors = EG(errors);
			// 不记录错误
			EG(record_errors) = false;
			// 错误数 归0
			EG(num_errors) = 0;
			// 错误列表指针为 null
			EG(errors) = NULL;

			// **调用用户处理器处理错误
			// 调用用户函数，无命名参数。p1:没用到，p2:对象，p3:函数名，p4:返回值，p5:参数数量，p6:参数列表
			res = call_user_function(CG(function_table), NULL, &orig_user_error_handler, &retval, 4, params);

			// 恢复原来的状态
			EG(record_errors) = orig_record_errors;
			// 恢复错误数量
			EG(num_errors) = orig_num_errors;
			// 恢复错误列表
			EG(errors) = orig_errors;

			// 如果调用结果为成功
			if (res == SUCCESS) {
				// 如果返回值有效
				if (Z_TYPE(retval) != IS_UNDEF) {
					// 如果返回值是 false
					if (Z_TYPE(retval) == IS_FALSE) {
						zend_error_cb(orig_type, error_filename, error_lineno, message);
					}
					// 使用错误回调方法
					zval_ptr_dtor(&retval);
				}
			// 如果没有异常
			} else if (!EG(exception)) {
				// 用户处理器出错 ，使用内置错误处理器
				/* The user error handler failed, use built-in error handler */
				// 使用错误回调方法
				zend_error_cb(orig_type, error_filename, error_lineno, message);
			}

			// 如果没有完成
			if (in_compilation) {
				// 恢复原类入口
				CG(active_class_entry) = saved_class_entry;
				// 恢复 循环变量堆栈
				RESTORE_STACK(loop_var_stack);
				// 恢复 延时操作码堆栈
				RESTORE_STACK(delayed_oplines_stack);
				// 完成
				CG(in_compilation) = 1;
			}

			// 销毁参数 3，2
			zval_ptr_dtor(&params[2]);
			zval_ptr_dtor(&params[1]);

			// 如果用户处理方法未定义
			if (Z_TYPE(EG(user_error_handler)) == IS_UNDEF) {
				// 从临时变量中恢复
				ZVAL_COPY_VALUE(&EG(user_error_handler), &orig_user_error_handler);
			// 如果已经存在
			} else {
				// 销毁临时变量
				zval_ptr_dtor(&orig_user_error_handler);
			}
			break;
	}
	// 如果是解析
	if (type == E_PARSE) {
		// eval() 错误不会影响 exit_status
		/* eval() errors do not affect exit_status */
		// 如果没有当前执行数据 
		if (!(EG(current_execute_data) &&
			// 并且当前在函数中
			EG(current_execute_data)->func &&
			// 并且函数是用户定义
			ZEND_USER_CODE(EG(current_execute_data)->func->type) &&
			// 并且操作码是 
			EG(current_execute_data)->opline->opcode == ZEND_INCLUDE_OR_EVAL &&
			// 并且 扩展操作码是 ZEND_EVAL
			EG(current_execute_data)->opline->extended_value == ZEND_EVAL)) {
			// 退出状态为 255
			EG(exit_status) = 255;
		}
	}
}
/* }}} */

// ing3, 通过参数列表抛错，主要是通过参数列表获取错误信息，内部5次调用
static ZEND_COLD void zend_error_va_list(
		int orig_type, zend_string *error_filename, uint32_t error_lineno,
		const char *format, va_list args)
{
	// 获取错误信息，参数数量根据format 格式需要来定
	zend_string *message = zend_vstrpprintf(0, format, args);
	// 尝试用用户的方法来处理错误，如果失败，使用内置方法处理。p1:错误码，p2:文件名，p3:错误行号，p4:报错信息
	zend_error_zstr_at(orig_type, error_filename, error_lineno, message);
	// 释放字串
	zend_string_release(message);
}

// ing3, 取得相关的文件名和行号，通过指针返回。p1:错误类型，p2:返回文件名，p3:返回行号
static ZEND_COLD void get_filename_lineno(int type, zend_string **filename, uint32_t *lineno) {
	/* Obtain relevant filename and lineno */
	// 错误类型
	switch (type) {
		// 内核错误，无文件名和行号
		case E_CORE_ERROR:
		case E_CORE_WARNING:
			*filename = NULL;
			*lineno = 0;
			break;
		case E_PARSE:
		case E_COMPILE_ERROR:
		case E_COMPILE_WARNING:
		case E_ERROR:
		case E_NOTICE:
		case E_STRICT:
		case E_DEPRECATED:
		case E_WARNING:
		case E_USER_ERROR:
		case E_USER_WARNING:
		case E_USER_NOTICE:
		case E_USER_DEPRECATED:
		case E_RECOVERABLE_ERROR:
			// 如果正在编译
			if (zend_is_compiling()) {
				// return CG(compiled_filename);
				*filename = zend_get_compiled_filename();
				
				// return CG(zend_lineno);
				*lineno = zend_get_compiled_lineno();
			// 如果正在执行
			} else if (zend_is_executing()) {
				// 正在执行的文件名
				*filename = zend_get_executed_filename_ex();
				
				// 正在执行的行号
				*lineno = zend_get_executed_lineno();
			// 其它情况，无行号
			} else {
				*filename = NULL;
				*lineno = 0;
			}
			break;
		// 其它情况，无文件名和行号
		default:
			*filename = NULL;
			*lineno = 0;
			break;
	}
	// 如果没有获取到文件名
	if (!*filename) {
		// UnKnown, #define ZSTR_KNOWN(idx) zend_known_strings[idx]
		*filename = ZSTR_KNOWN(ZEND_STR_UNKNOWN_CAPITALIZED);
	}
}

// ing3, 报错，需要提供错误类型，文件名，行号，错误信息格式、参数
ZEND_API ZEND_COLD void zend_error_at(
		int type, zend_string *filename, uint32_t lineno, const char *format, ...) {
	va_list args;
	// 如果没有文件名
	if (!filename) {
		uint32_t dummy_lineno;
		// 取得相关的文件名和行号，通过指针返回。p1:错误类型，p2:返回文件名，p3:返回行号
		get_filename_lineno(type, &filename, &dummy_lineno);
	}

	va_start(args, format);
	// 通过参数列表抛错，主要是通过参数列表获取错误信息，
	zend_error_va_list(type, filename, lineno, format, args);
	va_end(args);
}

// ing4, 报错，需要提供错误类型，错误信息格式、参数列表
ZEND_API ZEND_COLD void zend_error(int type, const char *format, ...) {
	zend_string *filename;
	uint32_t lineno;
	va_list args;
	// 取得相关的文件名和行号，通过指针返回。p1:错误类型，p2:返回文件名，p3:返回行号
	get_filename_lineno(type, &filename, &lineno);
	va_start(args, format);
	// 通过参数列表抛错，主要是通过参数列表获取错误信息，
	zend_error_va_list(type, filename, lineno, format, args);
	va_end(args);
}

// 和上面的一毛一样。
// ing3, 报错，需要提供错误类型，错误信息格式、参数
ZEND_API ZEND_COLD void zend_error_unchecked(int type, const char *format, ...) {
	zend_string *filename;
	uint32_t lineno;
	va_list args;
	// 取得相关的文件名和行号，通过指针返回。p1:错误类型，p2:返回文件名，p3:返回行号
	get_filename_lineno(type, &filename, &lineno);
	va_start(args, format);
	// 通过参数列表来抛错
	zend_error_va_list(type, filename, lineno, format, args);
	va_end(args);
}

// 比 zend_error_at 只多一个 abort();
// ing3, 报错，需要提供错误类型，文件名，行号，错误信息格式、参数列表
ZEND_API ZEND_COLD ZEND_NORETURN void zend_error_at_noreturn(
		int type, zend_string *filename, uint32_t lineno, const char *format, ...)
{
	va_list args;

	// 如果没有文件名
	if (!filename) {
		// 用不到行号
		uint32_t dummy_lineno;
		// 用不到行号但还是要用文件名
		// 取得相关的文件名和行号，通过指针返回。p1:错误类型，p2:返回文件名，p3:返回行号
		get_filename_lineno(type, &filename, &dummy_lineno);
	}
	// 获取列表参数
	va_start(args, format);
	// 通过参数列表抛错，主要是通过参数列表获取错误信息，
	zend_error_va_list(type, filename, lineno, format, args);
	// 停止获取列表参数
	va_end(args);
	// 应该不会执行到这里
	/* Should never reach this. */
	
	// 强行中断
	abort();
}

// ing4, 报错，需要提供错误类型，错误信息格式、参数
ZEND_API ZEND_COLD ZEND_NORETURN void zend_error_noreturn(int type, const char *format, ...)
{
	zend_string *filename;
	uint32_t lineno;
	va_list args;
	// 取得类型，文件名，行号
	get_filename_lineno(type, &filename, &lineno);
	// 开始读取列表参数
	va_start(args, format);
	// 通过参数列表抛错，主要是通过参数列表获取错误信息，
	zend_error_va_list(type, filename, lineno, format, args);
	// 结束读取列表参数
	va_end(args);
	/* Should never reach this. */
	abort();
}

// ing3, 报错，p1:类型，p2:错误码，p3:错误信息
ZEND_API ZEND_COLD ZEND_NORETURN void zend_strerror_noreturn(int type, int errn, const char *message)
{
// 如果有 strerror_r 函数。windows 没有它。
#ifdef HAVE_STR_ERROR_R
	char buf[1024];	
	strerror_r(errn, buf, sizeof(buf));
// 否则 strerror 是系统方法，windows有它
#else
	// 取得错误字串
	char *buf = strerror(errn);
#endif

	// 报错
	zend_error_noreturn(type, "%s: %s (%d)", message, buf, errn);
}

// ing4, 报错。p1:类型，p2:错误信息
ZEND_API ZEND_COLD void zend_error_zstr(int type, zend_string *message) {
	// 文件名
	zend_string *filename;
	// 行号
	uint32_t lineno;
	// 取得相关的文件名和行号，通过指针返回。p1:错误类型，p2:返回文件名，p3:返回行号
	get_filename_lineno(type, &filename, &lineno);
	// 尝试用用户的方法来处理错误，如果失败，使用内置方法处理。p1:错误码，p2:文件名，p3:错误行号，p4:报错信息
	zend_error_zstr_at(type, filename, lineno, message);
}

// ing3, 开启记录错误信息
ZEND_API void zend_begin_record_errors(void)
{
	ZEND_ASSERT(!EG(record_errors) && "Error recording already enabled");
	// 开启记录错误信息
	EG(record_errors) = true;
	// 数量为0
	EG(num_errors) = 0;
	// 错误列表指针为空
	EG(errors) = NULL;
}

// ing3, 报错：记录中的错误
ZEND_API void zend_emit_recorded_errors(void)
{
	// 不再记录错误
	EG(record_errors) = false;
	// 遍历每一个错误
	for (uint32_t i = 0; i < EG(num_errors); i++) {
		// 取得错误信息
		zend_error_info *error = EG(errors)[i];
		// 尝试用用户的方法来处理错误，如果失败，使用内置方法处理。p1:错误码，p2:文件名，p3:错误行号，p4:报错信息
		zend_error_zstr_at(error->type, error->filename, error->lineno, error->message);
	}
}

// ing3, 释放 error 记录
ZEND_API void zend_free_recorded_errors(void)
{
	// 如果没有，直接返回
	if (!EG(num_errors)) {
		return;
	}
	// 遍历每一个
	for (uint32_t i = 0; i < EG(num_errors); i++) {
		zend_error_info *info = EG(errors)[i];
		// 释放文件名
		zend_string_release(info->filename);
		// 释放信息
		zend_string_release(info->message);
		// 释放整个实例
		efree(info);
	}
	// 释放指针列表(验证过）
	efree(EG(errors));
	// 指针为空
	EG(errors) = NULL;
	// 计数清0
	EG(num_errors) = 0;
}

// ing4, 抛错，大量引用
ZEND_API ZEND_COLD void zend_throw_error(zend_class_entry *exception_ce, const char *format, ...) /* {{{ */
{
	va_list va;
	char *message = NULL;
	
	// 如果没有 异常类，使用zend异常类
	if (!exception_ce) {
		exception_ce = zend_ce_error;
	}

	// 这个标记用于 禁止在预载入时产生错误 
	/* Marker used to disable exception generation during preloading. */
	// 如果是 -1（预载入中）
	if (EG(exception) == (void*)(uintptr_t)-1) {
		// 什么也不做，返回
		return;
	}
	// va_start 还没完全测试明白？
	va_start(va, format);
	// ing3, 按给出的格式打印到smart_string 并返回 下属字串
	zend_vspprintf(&message, 0, format, va);

	//TODO: we can't convert compile-time errors to exceptions yet???
	// 如果有执行上下文 并且 不在编译中
	if (EG(current_execute_data) && !CG(in_compilation)) {
		// 抛错
		zend_throw_exception(exception_ce, message, 0);
	// 如果没有执行上下文，或在编译中
	} else {
		// 报错
		zend_error(E_ERROR, "%s", message);
	}
	// 释放错误信息
	efree(message);
	va_end(va);
}
/* }}} */

// ing3, 按给出的格式和参数列表组织错误信息，并抛 zend_ce_type_error 异常（大量引用）
ZEND_API ZEND_COLD void zend_type_error(const char *format, ...) /* {{{ */
{
	va_list va;
	char *message = NULL;

	va_start(va, format);
	// ing3, 按给出的格式打印到smart_string 并返回 下属字串
	zend_vspprintf(&message, 0, format, va);
	// 异常类型为 zend_ce_type_error
	zend_throw_exception(zend_ce_type_error, message, 0);
	// 释放异常信息
	efree(message);
	va_end(va);
} /* }}} */

// ing3, 按给出的格式和参数列表组织错误信息，并抛 zend_ce_argument_count_error 异常（大量引用）
ZEND_API ZEND_COLD void zend_argument_count_error(const char *format, ...) /* {{{ */
{
	va_list va;
	char *message = NULL;

	va_start(va, format);
	// ing3, 按给出的格式打印到smart_string 并返回 下属字串
	zend_vspprintf(&message, 0, format, va);
	// 异常类型为 zend_ce_argument_count_error
	zend_throw_exception(zend_ce_argument_count_error, message, 0);
	efree(message);

	va_end(va);
} /* }}} */

// ing3, 按给出的格式和参数列表组织错误信息，并抛 zend_ce_value_error 异常（大量引用）
ZEND_API ZEND_COLD void zend_value_error(const char *format, ...) /* {{{ */
{
	va_list va;
	char *message = NULL;

	va_start(va, format);
	// ing3, 按给出的格式打印到smart_string 并返回 下属字串
	zend_vspprintf(&message, 0, format, va);
	// 异常类型为 zend_ce_value_error
	zend_throw_exception(zend_ce_value_error, message, 0);
	efree(message);
	va_end(va);
} /* }}} */

// ing1, 调试用
ZEND_API ZEND_COLD void zend_output_debug_string(bool trigger_break, const char *format, ...) /* {{{ */
{
// 调试用
#if ZEND_DEBUG
	va_list args;

	va_start(args, format);
// windows
#	ifdef ZEND_WIN32
	{
		char output_buf[1024];

		vsnprintf(output_buf, 1024, format, args);
		OutputDebugString(output_buf);
		OutputDebugString("\n");
		if (trigger_break && IsDebuggerPresent()) {
			DebugBreak();
		}
	}
// 其他平台
#	else
	vfprintf(stderr, format, args);
	fprintf(stderr, "\n");
#	endif
	va_end(args);
#endif
}
/* }}} */

// ing3, 绕个大圈来调用 EG(user_exception_handler) 函数
ZEND_API ZEND_COLD void zend_user_exception_handler(void) /* {{{ */
{
	zval orig_user_exception_handler;
	zval params[1], retval2;
	zend_object *old_exception;
	// 如果异常是 unwind_exit
	if (zend_is_unwind_exit(EG(exception))) {
		// 返回
		return;
	}

	// 取出 异常
	old_exception = EG(exception);
	// 清空指针
	EG(exception) = NULL;
	// 异常放到zval里
	ZVAL_OBJ(&params[0], old_exception);
	// 复制用户的异常处理方法
	ZVAL_COPY_VALUE(&orig_user_exception_handler, &EG(user_exception_handler));

	// 调用用户函数，无命名参数。p1:没用到，p2:对象，p3:函数名，p4:返回值，p5:参数数量，p6:参数列表
	// 如果成功
	if (call_user_function(CG(function_table), NULL, &orig_user_exception_handler, &retval2, 1, params) == SUCCESS) {
		// 销毁返回值
		zval_ptr_dtor(&retval2);
		// 如果有异常
		if (EG(exception)) {
			// 释放当前异常
			OBJ_RELEASE(EG(exception));
			// 清空当前异常指针
			EG(exception) = NULL;
		}
		// 释放原异常
		OBJ_RELEASE(old_exception);
	// 如果失败
	} else {
		// 恢复原异常
		EG(exception) = old_exception;
	}
} /* }}} */

// ing3, 执行脚本（多个）
ZEND_API zend_result zend_execute_scripts(int type, zval *retval, int file_count, ...) /* {{{ */
{
	va_list files;
	int i;
	zend_file_handle *file_handle;
	zend_op_array *op_array;
	zend_result ret = SUCCESS;
	// 获取文件名列表
	va_start(files, file_count);
	// 遍历列表
	for (i = 0; i < file_count; i++) {
		// 每个文件句柄
		file_handle = va_arg(files, zend_file_handle *);
		// 如果为空，跳过
		if (!file_handle) {
			continue;
		}
		// 有一个出错 ，后面都跳过
		if (ret == FAILURE) {
			// ret会一直是 FAILURE，把所有参数都过一遍
			continue;
		}
		// 先编译
		op_array = zend_compile_file(file_handle, type);
		// 如果有路径
		if (file_handle->opened_path) {
			// 添加进 EG(included_files) 包含文件列表里。
			zend_hash_add_empty_element(&EG(included_files), file_handle->opened_path);
		}
		// 如果有操作码
		if (op_array) {
			// 执行操作码
			zend_execute(op_array, retval);
			// 存储异常
			zend_exception_restore();
			// 如果有异常
			if (UNEXPECTED(EG(exception))) {
				// 如果有处理器
				if (Z_TYPE(EG(user_exception_handler)) != IS_UNDEF) {
					// 调用异常处理器
					zend_user_exception_handler();
				}
				// 如果还有异常
				if (EG(exception)) {
					// 报错
					ret = zend_exception_error(EG(exception), E_ERROR);
				}
			}
			// 销毁静态变量表
			zend_destroy_static_vars(op_array);
			// 销毁操作码表
			destroy_op_array(op_array);
			// 销毁操作码组
			efree_size(op_array, sizeof(zend_op_array));
		// 如使用了 require 函数
		} else if (type==ZEND_REQUIRE) {
			// 结果为失败
			ret = FAILURE;
		}
	}
	// 读取参数列表结束 
	va_end(files);

	// 返回结果 
	return ret;
}
/* }}} */

#define COMPILED_STRING_DESCRIPTION_FORMAT "%s(%d) : %s"

// ing4, 组织字串信息 文件名（行号） ：传入的name
ZEND_API char *zend_make_compiled_string_description(const char *name) /* {{{ */
{
	const char *cur_filename;
	int cur_lineno;
	char *compiled_string_description;
	// 如果正在编译
	if (zend_is_compiling()) {
		// 文件名
		cur_filename = ZSTR_VAL(zend_get_compiled_filename());
		// 行号
		cur_lineno = zend_get_compiled_lineno();
	// 如果正在执行
	} else if (zend_is_executing()) {
		// 文件名
		cur_filename = zend_get_executed_filename();
		// 行号
		cur_lineno = zend_get_executed_lineno();
	} else {
		cur_filename = "Unknown";
		cur_lineno = 0;
	}
	// 文件名（行号） ：传入的name
	zend_spprintf(&compiled_string_description, 0, COMPILED_STRING_DESCRIPTION_FORMAT, cur_filename, cur_lineno, name);
	// 返回组装的字串
	return compiled_string_description;
}
/* }}} */

// ing3, 根据指针，删除指定字串
void free_estring(char **str_p) /* {{{ */
{
	efree(*str_p);
}
/* }}} */

// ing4, 重置map指针，数量清0
ZEND_API void zend_map_ptr_reset(void)
{
	// 默认是0，  static size_t global_map_ptr_last = 0;
	CG(map_ptr_last) = global_map_ptr_last;
}

// ing3, CG(map_ptr_real_base) 指针列表中 创建新的指针，并返回新指针相对于 CG(map_ptr_base) 的偏移量
ZEND_API void *zend_map_ptr_new(void)
{
	void **ptr;
	// 如果使用满了
	if (CG(map_ptr_last) >= CG(map_ptr_size)) {
		// 扩大map指针表
		/* Grow map_ptr table */
		// 大小，对齐到4096
		CG(map_ptr_size) = ZEND_MM_ALIGNED_SIZE_EX(CG(map_ptr_last) + 1, 4096);
		// 重新分配内存
		CG(map_ptr_real_base) = perealloc(CG(map_ptr_real_base), CG(map_ptr_size) * sizeof(void*), 1);
		// 计数起点位置：得到 CG(map_ptr_real_base) 左移一个byte的位置指针
		CG(map_ptr_base) = ZEND_MAP_PTR_BIASED_BASE(CG(map_ptr_real_base));
	}
	// 找到指针列表的最后一个位置
	ptr = (void**)CG(map_ptr_real_base) + CG(map_ptr_last);
	// 把这个指针赋值成 null
	*ptr = NULL;
	// 使用数量  + 1
	CG(map_ptr_last)++;
	// 返回指针相对于 CG(map_ptr_base) 偏移的byte数量。
	return ZEND_MAP_PTR_PTR2OFFSET(ptr);
}

//ing3，把 CG(map_ptr_real_base) 指针列表扩展到指定长度
ZEND_API void zend_map_ptr_extend(size_t last)
{
	// 长度大于已有长度（指针个数）才操作
	if (last > CG(map_ptr_last)) {
		void **ptr;
		// 如果当前空间不够大
		if (last >= CG(map_ptr_size)) {
			/* Grow map_ptr table */
			// 重新计算列表大小
			CG(map_ptr_size) = ZEND_MM_ALIGNED_SIZE_EX(last, 4096);
			// 分配内存
			CG(map_ptr_real_base) = perealloc(CG(map_ptr_real_base), CG(map_ptr_size) * sizeof(void*), 1);
			// 计数起点位置：CG(map_ptr_real_base) 左移一个byte的位置指针
			CG(map_ptr_base) = ZEND_MAP_PTR_BIASED_BASE(CG(map_ptr_real_base));
		}
		// 最后一个有效指针后面的位置
		ptr = (void**)CG(map_ptr_real_base) + CG(map_ptr_last);
		// 把后面的内存全置成0（防止脏读？）
		memset(ptr, 0, (last - CG(map_ptr_last)) * sizeof(void*));
		// 最大有效长度（指针个数）
		CG(map_ptr_last) = last;
	}
}

// ing3, 分配类入口缓存。新建一个 map_ptr位置给type_name,把新map_ptr的偏移位置（序号）放进type_name的引用次数里。
ZEND_API void zend_alloc_ce_cache(zend_string *type_name)
{
	// 如果已经有这个类型的缓存 或类型名不是内置类型
	if (ZSTR_HAS_CE_CACHE(type_name) || !ZSTR_IS_INTERNED(type_name)) {
		// 直接返回
		return;
	}
	// 如果是永久的 并且 在开始时运行过.直接返回。
	if ((GC_FLAGS(type_name) & IS_STR_PERMANENT) && startup_done) {
		// 不要在模块开始以外的地方，分配位置给永久内置字串
		// 缓存位置在下一次请求时会变得不可用
		/* Don't allocate slot on permanent interned string outside module startup.
		 * The cache slot would no longer be valid on the next request. */
		return;
	}
	// 如果类名是 self 或 parent
	if (zend_string_equals_literal_ci(type_name, "self")
			|| zend_string_equals_literal_ci(type_name, "parent")) {
		// 直接返回
		return;
	}

	// 使用引用计数来对应类型的 map_ptr （哇擦，另类！）
	/* We use the refcount to keep map_ptr of corresponding type */
	uint32_t ret;
	// 
	do {
		// 取得新的 map_ptr 位置
		ret = ZEND_MAP_PTR_NEW_OFFSET();
	// 一定要大于2
	} while (ret <= 2);
	// 添加标记 IS_STR_CLASS_NAME_MAP_PTR，给类名添加了map_ptr
	GC_ADD_FLAGS(type_name, IS_STR_CLASS_NAME_MAP_PTR);
	// 把map_ptr序号放进引用次数里
	GC_SET_REFCOUNT(type_name, ret);
}
