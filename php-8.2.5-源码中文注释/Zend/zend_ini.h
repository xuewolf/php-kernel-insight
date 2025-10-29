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
   | Author: Zeev Suraski <zeev@php.net>                                  |
   +----------------------------------------------------------------------+
*/

#ifndef ZEND_INI_H
#define ZEND_INI_H

// 用户条目
#define ZEND_INI_USER	(1<<0)
// 
#define ZEND_INI_PERDIR	(1<<1)
// 系统条目
#define ZEND_INI_SYSTEM	(1<<2)

// 所有条目
#define ZEND_INI_ALL (ZEND_INI_USER|ZEND_INI_PERDIR|ZEND_INI_SYSTEM)

// ing4, 定义事件。p1:函数名
// p1:配置条目，p2:新值，p3:参数1，p4:参数2，p5:参数3，p6:阶段
#define ZEND_INI_MH(name) int name(zend_ini_entry *entry, zend_string *new_value, void *mh_arg1, void *mh_arg2, void *mh_arg3, int stage)

// ing4, 定义显示配置值的函数，p1:函数名
// p1:配置条目，p2:类型（是否原始值）
#define ZEND_INI_DISP(name) ZEND_COLD void name(zend_ini_entry *ini_entry, int type)

// ini条目定义
typedef struct _zend_ini_entry_def {
	// 名称
	const char *name;
	// on_modify 事件
	ZEND_INI_MH((*on_modify));
	// on_modify 的3个参数
	void *mh_arg1;
	void *mh_arg2;
	void *mh_arg3;
	// 值
	const char *value;
	// displayer 函数原型
	void (*displayer)(zend_ini_entry *ini_entry, int type);

	// 值长度
	uint32_t value_length;
	// 名称长度
	uint16_t name_length;
	// 是否可更改
	uint8_t modifiable;
} zend_ini_entry_def;

// ini条目
struct _zend_ini_entry {
	// 名称
	zend_string *name;
	// on_modify 事件
	ZEND_INI_MH((*on_modify));
	// on_modify 的3个参数
	void *mh_arg1;
	void *mh_arg2;
	void *mh_arg3;
	// 值
	zend_string *value;
	// 原始值
	zend_string *orig_value;
	// displayer 函数原型
	void (*displayer)(zend_ini_entry *ini_entry, int type);

	// 模块编号
	int module_number;
	// 是否可更改
	uint8_t modifiable;
	// 原始是否可更改
	uint8_t orig_modifiable;
	// 是否已更改
	uint8_t modified;

};

BEGIN_EXTERN_C()
ZEND_API void zend_ini_startup(void);
ZEND_API void zend_ini_shutdown(void);
ZEND_API void zend_ini_global_shutdown(void);
ZEND_API void zend_ini_deactivate(void);
ZEND_API void zend_ini_dtor(HashTable *ini_directives);

ZEND_API void zend_copy_ini_directives(void);

ZEND_API void zend_ini_sort_entries(void);

ZEND_API zend_result zend_register_ini_entries(const zend_ini_entry_def *ini_entry, int module_number);
ZEND_API zend_result zend_register_ini_entries_ex(const zend_ini_entry_def *ini_entry, int module_number, int module_type);
ZEND_API void zend_unregister_ini_entries(int module_number);
ZEND_API void zend_unregister_ini_entries_ex(int module_number, int module_type);
ZEND_API void zend_ini_refresh_caches(int stage);
ZEND_API zend_result zend_alter_ini_entry(zend_string *name, zend_string *new_value, int modify_type, int stage);
ZEND_API zend_result zend_alter_ini_entry_ex(zend_string *name, zend_string *new_value, int modify_type, int stage, bool force_change);
ZEND_API zend_result zend_alter_ini_entry_chars(zend_string *name, const char *value, size_t value_length, int modify_type, int stage);
ZEND_API zend_result zend_alter_ini_entry_chars_ex(zend_string *name, const char *value, size_t value_length, int modify_type, int stage, int force_change);
ZEND_API zend_result zend_restore_ini_entry(zend_string *name, int stage);
ZEND_API void display_ini_entries(zend_module_entry *module);

ZEND_API zend_long zend_ini_long(const char *name, size_t name_length, int orig);
ZEND_API double zend_ini_double(const char *name, size_t name_length, int orig);
ZEND_API char *zend_ini_string(const char *name, size_t name_length, int orig);
ZEND_API char *zend_ini_string_ex(const char *name, size_t name_length, int orig, bool *exists);
ZEND_API zend_string *zend_ini_get_value(zend_string *name);
ZEND_API bool zend_ini_parse_bool(zend_string *str);

/**
解析一个ini数量 （见 zend_ini_parse_quantity_internal函数）
 * Parses an ini quantity
 *
参数值必须是下列格式的字串
 * The value parameter must be a string in the form
 *
正负号（可选） 数字 空格（任意多个） 修饰符（可选）
 *     sign? digits ws* multiplier?
 *
 * with
 *
正负号 0个或1个
 *     sign: [+-]
数字 1个
 *     digit: [0-9]
数字 n个
 *     digits: digit+
空格 n个 
 *     ws: [ \t\n\r\v\f]
乘方符号 KMG
 *     multiplier: [KMG]
 *
开头和结尾的空格会被忽略
 * Leading and trailing whitespaces are ignored.
 *
如果字串是空的话只有空格，会返回0
 * If the string is empty or consists only of only whitespaces, 0 is returned.
 *
数字会被解析成10进制，除非开头是0，这种情况下会被解析成 8进制。
 * Digits is parsed as decimal unless the first digit is '0', in which case
 * digits is parsed as octal.
 *
乘方符号 大小写不敏感。K/M/G 分配对象 2的 10次/20次/30次 幂
 * The multiplier is case-insensitive. K, M, and G multiply the quantity by
 * 2**10, 2**20, and 2**30, respectively.
 *
为了向后兼容，非法格式做如下处理：
 * For backwards compatibility, ill-formatted values are handled as follows:
没有开头字符，当成0来处理
 * - No leading digits: value is treated as '0'
乘方符号非法：忽略乘方符号
 * - Invalid multiplier: multiplier is ignored
在数字和乘方符号间有非法字符：忽略非法字符
 * - Invalid characters between digits and multiplier: invalid characters are
 *   ignored
整数溢出：返回溢出结果
 * - Integer overflow: The result of the overflow is returned
 *
在所有情况里错误信息会存储在 *errstr中（调用者必须自己释放它），没有错误它会是 Null
 * In any of these cases an error string is stored in *errstr (caller must
 * release it), otherwise *errstr is set to NULL.
 */
ZEND_API zend_long zend_ini_parse_quantity(zend_string *value, zend_string **errstr);

/**
zend_ini_parse_quantity 各种不同的无符号
 * Unsigned variant of zend_ini_parse_quantity
 */
ZEND_API zend_ulong zend_ini_parse_uquantity(zend_string *value, zend_string **errstr);

ZEND_API zend_long zend_ini_parse_quantity_warn(zend_string *value, zend_string *setting);

ZEND_API zend_ulong zend_ini_parse_uquantity_warn(zend_string *value, zend_string *setting);

ZEND_API zend_result zend_ini_register_displayer(const char *name, uint32_t name_length, void (*displayer)(zend_ini_entry *ini_entry, int type));

ZEND_API ZEND_INI_DISP(zend_ini_boolean_displayer_cb);
ZEND_API ZEND_INI_DISP(zend_ini_color_displayer_cb);
ZEND_API ZEND_INI_DISP(display_link_numbers);
END_EXTERN_C()

// ing3, 声名 zend_ini_entry_def 列表
#define ZEND_INI_BEGIN()		static const zend_ini_entry_def ini_entries[] = {
// ing3, zend_ini_entry_def 列表结尾，再加一个空但有效的元素
#define ZEND_INI_END()		{ NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0} };

// ing4, 快速声名 zend_ini_entry_def
#define ZEND_INI_ENTRY3_EX(name, default_value, modifiable, on_modify, arg1, arg2, arg3, displayer) \
	/* zend_ini_entry_def 有10个元素 */ \
	{ name, on_modify, arg1, arg2, arg3, default_value, displayer, sizeof(default_value)-1, sizeof(name)-1, modifiable },

// ing4, 快速声名 zend_ini_entry_def，displayer 为 null
#define ZEND_INI_ENTRY3(name, default_value, modifiable, on_modify, arg1, arg2, arg3) \
	ZEND_INI_ENTRY3_EX(name, default_value, modifiable, on_modify, arg1, arg2, arg3, NULL)

// ing4, 快速声名 zend_ini_entry_def，参数3 为 null
#define ZEND_INI_ENTRY2_EX(name, default_value, modifiable, on_modify, arg1, arg2, displayer) \
	ZEND_INI_ENTRY3_EX(name, default_value, modifiable, on_modify, arg1, arg2, NULL, displayer)

// ing4, 快速声名 zend_ini_entry_def，参数3 和 displayer 为 null
#define ZEND_INI_ENTRY2(name, default_value, modifiable, on_modify, arg1, arg2) \
	ZEND_INI_ENTRY2_EX(name, default_value, modifiable, on_modify, arg1, arg2, NULL)

// ing4, 快速声名 zend_ini_entry_def，参数2 和 参数3 为 null
#define ZEND_INI_ENTRY1_EX(name, default_value, modifiable, on_modify, arg1, displayer) \
	ZEND_INI_ENTRY3_EX(name, default_value, modifiable, on_modify, arg1, NULL, NULL, displayer)

// ing4, 快速声名 zend_ini_entry_def，参数2 和 参数3 和 displayer 为 null
#define ZEND_INI_ENTRY1(name, default_value, modifiable, on_modify, arg1) \
	ZEND_INI_ENTRY1_EX(name, default_value, modifiable, on_modify, arg1, NULL)

// ing4, 快速声名 zend_ini_entry_def，参数1 和 参数2 和 参数3 为 null
#define ZEND_INI_ENTRY_EX(name, default_value, modifiable, on_modify, displayer) \
	ZEND_INI_ENTRY3_EX(name, default_value, modifiable, on_modify, NULL, NULL, NULL, displayer)

// ing4, 快速声名 zend_ini_entry_def，参数1 和 参数2 和 参数3 和 displayer 为 null
#define ZEND_INI_ENTRY(name, default_value, modifiable, on_modify) \
	ZEND_INI_ENTRY_EX(name, default_value, modifiable, on_modify, NULL)
	
	
// 如果要求线程安全，每个 struct_ptr 换成 struct_ptr##_id
#ifdef ZTS

// ing3, 快速声名 zend_ini_entry_def，参数3 和 displayer 为 null，参数1是 struct_type 和 property_name 的偏移量
#define STD_ZEND_INI_ENTRY(name, default_value, modifiable, on_modify, property_name, struct_type, struct_ptr) \
	/* 快速声名 zend_ini_entry_def，参数3 和 displayer 为 null */ \
	ZEND_INI_ENTRY2(name, default_value, modifiable, on_modify, (void *) XtOffsetOf(struct_type, property_name), (void *) &struct_ptr##_id)

// ing3, 快速声名 zend_ini_entry_def，参数3 为 null，参数1是 struct_type 和 property_name 的偏移量
#define STD_ZEND_INI_ENTRY_EX(name, default_value, modifiable, on_modify, property_name, struct_type, struct_ptr, displayer) \
	/* 快速声名 zend_ini_entry_def，参数3 为 null */ \
	ZEND_INI_ENTRY2_EX(name, default_value, modifiable, on_modify, (void *) XtOffsetOf(struct_type, property_name), (void *) &struct_ptr##_id, displayer)

// ing3, 快速声名 zend_ini_entry_def，参数1是 struct_type 和 property_name 的偏移量
#define STD_ZEND_INI_BOOLEAN(name, default_value, modifiable, on_modify, property_name, struct_type, struct_ptr) \
	/* 快速声名 zend_ini_entry_def */ \
	ZEND_INI_ENTRY3_EX(name, default_value, modifiable, on_modify, (void *) XtOffsetOf(struct_type, property_name), (void *) &struct_ptr##_id, NULL, zend_ini_boolean_displayer_cb)

// 不要求线程安全
#else

// ing3, 快速声名 zend_ini_entry_def，参数3 和 displayer 为 null，参数1是 struct_type 和 property_name 的偏移量
#define STD_ZEND_INI_ENTRY(name, default_value, modifiable, on_modify, property_name, struct_type, struct_ptr) \
	/* 快速声名 zend_ini_entry_def，参数3 和 displayer 为 null */ \
	ZEND_INI_ENTRY2(name, default_value, modifiable, on_modify, (void *) XtOffsetOf(struct_type, property_name), (void *) &struct_ptr)

// ing3, 快速声名 zend_ini_entry_def，参数3 为 null，参数1是 struct_type 和 property_name 的偏移量
#define STD_ZEND_INI_ENTRY_EX(name, default_value, modifiable, on_modify, property_name, struct_type, struct_ptr, displayer) \
	/* 快速声名 zend_ini_entry_def，参数3 为 null */ \
	ZEND_INI_ENTRY2_EX(name, default_value, modifiable, on_modify, (void *) XtOffsetOf(struct_type, property_name), (void *) &struct_ptr, displayer)

// ing3, 快速声名 zend_ini_entry_def，参数1是 struct_type 和 property_name 的偏移量
#define STD_ZEND_INI_BOOLEAN(name, default_value, modifiable, on_modify, property_name, struct_type, struct_ptr) \
	/* 快速声名 zend_ini_entry_def */ \
	ZEND_INI_ENTRY3_EX(name, default_value, modifiable, on_modify, (void *) XtOffsetOf(struct_type, property_name), (void *) &struct_ptr, NULL, zend_ini_boolean_displayer_cb)

#endif


// ing4, 查找条目当前值，结果转成整数。p1:名称
#define INI_INT(name) zend_ini_long((name), strlen(name), 0)
// ing4, 查找条目当前值，结果转成小数。p1:名称
#define INI_FLT(name) zend_ini_double((name), strlen(name), 0)
// ing4, 查找条目当前值，结果转成字串。p1:名称
#define INI_STR(name) zend_ini_string_ex((name), strlen(name), 0, NULL)
// ing4, 查找条目当前值，结果转成布尔型。p1:名称
#define INI_BOOL(name) ((bool) INI_INT(name))

// ing4, 查找条目原始值，结果转成整数。p1:名称
#define INI_ORIG_INT(name)	zend_ini_long((name), strlen(name), 1)
// ing4, 查找条目原始值，结果转成小数。p1:名称
#define INI_ORIG_FLT(name)	zend_ini_double((name), strlen(name), 1)
// ing4, 查找条目原始值，结果转成字串。p1:名称
#define INI_ORIG_STR(name)	zend_ini_string((name), strlen(name), 1)
// ing4, 查找条目原始值，结果转成布尔型。p1:名称
#define INI_ORIG_BOOL(name) ((bool) INI_ORIG_INT(name))

// ing3, 注册ini条目。p1:定义的ini条目，p2:模块号，p3:模块类型
#define REGISTER_INI_ENTRIES() zend_register_ini_entries_ex(ini_entries, module_number, type)

// ing4, 删除模块相关的所有 配置指令。p1:模块号，p2:模块类型
#define UNREGISTER_INI_ENTRIES() zend_unregister_ini_entries_ex(module_number, type)

// abstract, main/php_ini.c 实现
#define DISPLAY_INI_ENTRIES() display_ini_entries(zend_module)

// ing4, 更新 注册指令表 里配置选项的 displayer 属性。p1:配置名称，p2:名称长度，p3:displayer（zend_ini_entry*）
#define REGISTER_INI_DISPLAYER(name, displayer) zend_ini_register_displayer((name), strlen(name), displayer)

// ing4, 更新 注册指令表 里配置选项的 displayer 属性。
#define REGISTER_INI_BOOLEAN(name) REGISTER_INI_DISPLAYER(name, zend_ini_boolean_displayer_cb)

/* Standard message handlers */
BEGIN_EXTERN_C()
ZEND_API ZEND_INI_MH(OnUpdateBool);
ZEND_API ZEND_INI_MH(OnUpdateLong);
ZEND_API ZEND_INI_MH(OnUpdateLongGEZero);
ZEND_API ZEND_INI_MH(OnUpdateReal);
/* char* versions */
ZEND_API ZEND_INI_MH(OnUpdateString);
ZEND_API ZEND_INI_MH(OnUpdateStringUnempty);
/* zend_string* versions */
ZEND_API ZEND_INI_MH(OnUpdateStr);
ZEND_API ZEND_INI_MH(OnUpdateStrNotEmpty);
END_EXTERN_C()

// 
#define ZEND_INI_DISPLAY_ORIG	1
// 
#define ZEND_INI_DISPLAY_ACTIVE	2

// 启动阶段
#define ZEND_INI_STAGE_STARTUP		(1<<0)
// 关闭阶段
#define ZEND_INI_STAGE_SHUTDOWN		(1<<1)
// 激活阶段
#define ZEND_INI_STAGE_ACTIVATE		(1<<2)
// 反激活阶段
#define ZEND_INI_STAGE_DEACTIVATE	(1<<3)
// 运行阶段
#define ZEND_INI_STAGE_RUNTIME		(1<<4)
// 全局没用到
#define ZEND_INI_STAGE_HTACCESS		(1<<5)

// 启动/关闭以外的阶段
#define ZEND_INI_STAGE_IN_REQUEST   (ZEND_INI_STAGE_ACTIVATE|ZEND_INI_STAGE_DEACTIVATE|ZEND_INI_STAGE_RUNTIME|ZEND_INI_STAGE_HTACCESS)

// ini解析引擎
/* INI parsing engine */
// 解析器回调函数 原型
typedef void (*zend_ini_parser_cb_t)(zval *arg1, zval *arg2, zval *arg3, int callback_type, void *arg);
BEGIN_EXTERN_C()
// 
ZEND_API int zend_parse_ini_file(zend_file_handle *fh, bool unbuffered_errors, int scanner_mode, zend_ini_parser_cb_t ini_parser_cb, void *arg);
ZEND_API int zend_parse_ini_string(char *str, bool unbuffered_errors, int scanner_mode, zend_ini_parser_cb_t ini_parser_cb, void *arg);
END_EXTERN_C()

// ini 条目类型，3各
/* INI entries */
// 普通条目：foo = bar
#define ZEND_INI_PARSER_ENTRY     1 /* Normal entry: foo = bar */
// 段: [foobar]
#define ZEND_INI_PARSER_SECTION	  2 /* Section: [foobar] */
// 数组： foo[] = bar
#define ZEND_INI_PARSER_POP_ENTRY 3 /* Offset entry: foo[] = bar */

// 解析器参数
typedef struct _zend_ini_parser_param {
	// 解析器回调函数
	zend_ini_parser_cb_t ini_parser_cb;
	// 未知类型指针
	void *arg;
} zend_ini_parser_param;

// 如果没有线程安全
#ifndef ZTS
// ing3, 取得事件触发的第二个参数
# define ZEND_INI_GET_BASE() ((char *) mh_arg2)

// 要求线程安全
#else
# define ZEND_INI_GET_BASE() ((char *) ts_resource(*((int *) mh_arg2)))
#endif

// ing3, 取得参数 mh_arg2 ，并把指针前进 mh_arg1 个字节
#define ZEND_INI_GET_ADDR() (ZEND_INI_GET_BASE() + (size_t) mh_arg1)

#endif /* ZEND_INI_H */
