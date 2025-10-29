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

#include "zend.h"
#include "zend_sort.h"
#include "zend_API.h"
#include "zend_ini.h"
#include "zend_alloc.h"
#include "zend_operators.h"
#include "zend_strtod.h"
#include "zend_modules.h"
#include "zend_smart_str.h"
#include <ctype.h>

static HashTable *registered_zend_ini_directives;

#define NO_VALUE_PLAINTEXT		"no value"
#define NO_VALUE_HTML			"<i>no value</i>"

// ing4, 检验字符是否是空格
static inline bool zend_is_whitespace(char c) {
	return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f';
}

// 哈希应用方法
/*
 * hash_apply functions
 */
 
// ing4, 修配置项转移到指定模块 p1:配置项, p2:模块编号
static int zend_remove_ini_entries(zval *el, void *arg) /* {{{ */
{
	// 取出zval里的配置项
	zend_ini_entry *ini_entry = (zend_ini_entry *)Z_PTR_P(el);
	// 模块编号 
	int module_number = *(int *)arg;
	// 实体里的模块编号
	return ini_entry->module_number == module_number;
}
/* }}} */

// ing3, 还原 配置条目 回调。p1:配置条件，p2:阶段
static zend_result zend_restore_ini_entry_cb(zend_ini_entry *ini_entry, int stage) /* {{{ */
{
	// 默认是失败
	zend_result result = FAILURE;

	// 如果已修改
	if (ini_entry->modified) {
		// 如果有 on_modify 方法
		if (ini_entry->on_modify) {
			// 
			zend_try {
			// 即使 on_modify 中断，还是需要继续还原， 
			// 因为可能 已经分配了变量，变量会在 MM 关闭时释放
			// 在稍后的ini再次修改时，会造成内存污染
			/* even if on_modify bails out, we have to continue on with restoring,
				since there can be allocated variables that would be freed on MM shutdown
				and would lead to memory corruption later ini entry is modified again */
				// 触发 on_modify 事件
				result = ini_entry->on_modify(ini_entry, ini_entry->orig_value, ini_entry->mh_arg1, ini_entry->mh_arg2, ini_entry->mh_arg3, stage);
			} zend_end_try();
		}
		// 如果在运行阶段 并且 结果是失败
		if (stage == ZEND_INI_STAGE_RUNTIME && result == FAILURE) {
			// 运行时可以失败
			/* runtime failure is OK */
			// 返回失败
			return FAILURE;
		}
		// 如果现有值和原始值不同
		if (ini_entry->value != ini_entry->orig_value) {
			// 释放现有值
			zend_string_release(ini_entry->value);
		}
		// 恢复原始值
		ini_entry->value = ini_entry->orig_value;
		// 原始可修改状态
		ini_entry->modifiable = ini_entry->orig_modifiable;
		// 未修改
		ini_entry->modified = 0;
		// 原始值为空
		ini_entry->orig_value = NULL;
		// 原始可修改状态为空
		ini_entry->orig_modifiable = 0;
	}
	// 返回成功
	return SUCCESS;
}
/* }}} */

// ing4, 释放配置项. p1:zval
static void free_ini_entry(zval *zv) /* {{{ */
{
	// 取出配置项
	zend_ini_entry *entry = (zend_ini_entry*)Z_PTR_P(zv);

	// 释放名称
	zend_string_release_ex(entry->name, 1);
	// 如果有值
	if (entry->value) {
		// 释放值
		zend_string_release(entry->value);
	}
	// 如果有原始值
	if (entry->orig_value) {
		// 释放原始值
		zend_string_release_ex(entry->orig_value, 1);
	}
	// 释放配置项
	free(entry);
}
/* }}} */

/*
启动/关闭
 * Startup / shutdown
 */
// ing4, 启动
ZEND_API void zend_ini_startup(void) /* {{{ */
{
	// 分配内存创建哈希表
	registered_zend_ini_directives = (HashTable *) malloc(sizeof(HashTable));

	// 使用 registered_zend_ini_directives 作为执行时配置表
	EG(ini_directives) = registered_zend_ini_directives;
	// 修改后的执行时 配置信息为 null
	EG(modified_ini_directives) = NULL;
	// 报错级别配置条目为 null
	EG(error_reporting_ini_entry) = NULL;
	// 初始化 registered_zend_ini_directives 配置表，128个元素，永久
	zend_hash_init(registered_zend_ini_directives, 128, NULL, free_ini_entry, 1);
}
/* }}} */

// ing4, 关闭
ZEND_API void zend_ini_shutdown(void) /* {{{ */
{
	// 销毁 执行时 配置表
	zend_ini_dtor(EG(ini_directives));
}
/* }}} */

// ing4, 释放配置哈希表
ZEND_API void zend_ini_dtor(HashTable *ini_directives) /* {{{ */
{
	// 销毁 哈希表
	zend_hash_destroy(ini_directives);
	// 释放哈希表
	free(ini_directives);
}
/* }}} */

// ing4, 关闭全局配置
ZEND_API void zend_ini_global_shutdown(void) /* {{{ */
{
	// 销毁配置表
	zend_hash_destroy(registered_zend_ini_directives);
	// 释放配置表
	free(registered_zend_ini_directives);
}
/* }}} */

// ing4，反激活。释放 修改后的配置表
ZEND_API void zend_ini_deactivate(void) /* {{{ */
{
	// 如果有修改后的配置表
	if (EG(modified_ini_directives)) {
		//
		zend_ini_entry *ini_entry;
		// 遍历 修改后的配置表
		ZEND_HASH_MAP_FOREACH_PTR(EG(modified_ini_directives), ini_entry) {
			// 逐个条目调用 恢复回调
			zend_restore_ini_entry_cb(ini_entry, ZEND_INI_STAGE_DEACTIVATE);
		} ZEND_HASH_FOREACH_END();
		// 销毁  修改后的配置表
		zend_hash_destroy(EG(modified_ini_directives));
		// 释放 修改后的配置表
		FREE_HASHTABLE(EG(modified_ini_directives));
		// 指针修改为 null
		EG(modified_ini_directives) = NULL;
	}
}
/* }}} */
// 如果要求线程安全
#ifdef ZTS

// ing4, 创建配置项的 永久副本。原来的没有删掉
static void copy_ini_entry(zval *zv) /* {{{ */
{
	// 原配置项
	zend_ini_entry *old_entry = (zend_ini_entry*)Z_PTR_P(zv);
	// 分配内存创建永久配置项
	zend_ini_entry *new_entry = pemalloc(sizeof(zend_ini_entry), 1);

	// zv指向新配置项
	Z_PTR_P(zv) = new_entry;
	// 复制内容
	memcpy(new_entry, old_entry, sizeof(zend_ini_entry));
	// 如果旧的有名称
	if (old_entry->name) {
		// 创建名称副本，给新实例
		new_entry->name = zend_string_dup(old_entry->name, 1);
	}
	// 如果旧的有值
	if (old_entry->value) {
		// 创建值副本，给新实例
		new_entry->value = zend_string_dup(old_entry->value, 1);
	}
	// 如果旧的有原始值
	if (old_entry->orig_value) {
		// 创建原始值副本，给新实例
		new_entry->orig_value = zend_string_dup(old_entry->orig_value, 1);
	}
}
/* }}} */

// ing4, 创建 EG(ini_directives)表 并从 registered_zend_ini_directives表复制元素
ZEND_API void zend_copy_ini_directives(void) /* {{{ */
{
	// 修改后的配置为 null
	EG(modified_ini_directives) = NULL;
	// 报错级别 null
	EG(error_reporting_ini_entry) = NULL;
	// 分配内存创建哈希表
	EG(ini_directives) = (HashTable *) malloc(sizeof(HashTable));
	// 初始货哈希表 
	zend_hash_init(EG(ini_directives), registered_zend_ini_directives->nNumOfElements, NULL, free_ini_entry, 1);
	// 从 registered_zend_ini_directives 哈希表中复制元素
	zend_hash_copy(EG(ini_directives), registered_zend_ini_directives, copy_ini_entry);
}
/* }}} */
#endif

// ing4, 比较两个 Bucket 的key的大小
static int ini_key_compare(Bucket *f, Bucket *s) /* {{{ */
{
	// 如果两个都是整数哈希。没有key
	if (!f->key && !s->key) { /* both numeric */
		// 第一大
		if (f->h > s->h) {
			// 返回-1
			return -1;
		// 第二个大
		} else if (f->h < s->h) {
			// 返回1
			return 1;
		}
		// 相等返回 0
		return 0;
	// 第一个是整数
	} else if (!f->key) { /* f is numeric, s is not */
		// 返回-1
		return -1;
	// 第二个是整数
	} else if (!s->key) { /* s is numeric, f is not */
		// 返回1
		return 1;
	// 两个都不是整数 ，都是字串
	} else { /* both strings */
		// 按字串二进制比较
		return zend_binary_strcasecmp(ZSTR_VAL(f->key), ZSTR_LEN(f->key), ZSTR_VAL(s->key), ZSTR_LEN(s->key));
	}
}
/* }}} */

// ing4, 给配置项表排序
ZEND_API void zend_ini_sort_entries(void) /* {{{ */
{
	// 通过 ini_key_compare 方法给配置项排序 
	zend_hash_sort(EG(ini_directives), ini_key_compare, 0);
}
/* }}} */

/*
注册/反注册
 * Registration / unregistration
 */

// ing3, 注册ini条目。p1:定义的ini条目，p2:模块号，p3:模块类型
ZEND_API zend_result zend_register_ini_entries_ex(const zend_ini_entry_def *ini_entry, int module_number, int module_type) /* {{{ */
{
	zend_ini_entry *p;
	zval *default_value;
	// 使用注册的配置指令表
	HashTable *directives = registered_zend_ini_directives;

// 线程安全
#ifdef ZTS
	// 如果在请求过程中调用，例如：从dl()参数，不可以使用 全局 配置指令表，
	// 只能修改 （请求|线程）前的 版本
	// 这样可以解决两个问题：第一是 dl()过的扩展 的配置条目 会开始生效
	// 第二个是 从dl()中更新全局哈希是没有 互斥保护的，可能会导致崩溃
	/* if we are called during the request, eg: from dl(),
	 * then we should not touch the global directives table,
	 * and should update the per-(request|thread) version instead.
	 * This solves two problems: one is that ini entries for dl()'d
	 * extensions will now work, and the second is that updating the
	 * global hash here from dl() is not mutex protected and can
	 * lead to death.
	 */
	// 如果配置指令表 不是 运行时配置指令表
	if (directives != EG(ini_directives)) {
		// 必须是在临时模块中
		ZEND_ASSERT(module_type == MODULE_TEMPORARY);
		// 使用 运行时配置指令表
		directives = EG(ini_directives);
	// 否则 
	} else {
		// 必须是永久模块
		ZEND_ASSERT(module_type == MODULE_PERSISTENT);
	}
#endif

	while (ini_entry->name) {
		// 分配内存创建配置条目
		p = pemalloc(sizeof(zend_ini_entry), 1);
		// 给配置项名 创建内置字串
		p->name = zend_string_init_interned(ini_entry->name, ini_entry->name_length, 1);
		// 添加 on_modify 方法
		p->on_modify = ini_entry->on_modify;
		// on_modify 的3个参数
		p->mh_arg1 = ini_entry->mh_arg1;
		p->mh_arg2 = ini_entry->mh_arg2;
		p->mh_arg3 = ini_entry->mh_arg3;
		// 值为null
		p->value = NULL;
		// 原始值为null
		p->orig_value = NULL;
		// 更新 displayer
		p->displayer = ini_entry->displayer;
		// 更新 modifiable
		p->modifiable = ini_entry->modifiable;

		// 原始为不可 修改 为0
		p->orig_modifiable = 0;
		// 未修改
		p->modified = 0;
		// 模块编号
		p->module_number = module_number;
		
		// 添加配置条目，如果失败
		if (zend_hash_add_ptr(directives, p->name, (void*)p) == NULL) {
			// 如果有名称
			if (p->name) {
				// 释放名称
				zend_string_release_ex(p->name, 1);
			}
			// 反注册这个模块的条目
			zend_unregister_ini_entries_ex(module_number, module_type);
			// 返回失败
			return FAILURE;
		}
		
		// 反激活配置信息 成功 并且 （没有 on_modify 或 on_modify成功）
		if (((default_value = zend_get_configuration_directive(p->name)) != NULL) &&
		    (!p->on_modify || p->on_modify(p, Z_STR_P(default_value), p->mh_arg1, p->mh_arg2, p->mh_arg3, ZEND_INI_STAGE_STARTUP) == SUCCESS)) {
			// 使用默认值
			p->value = zend_new_interned_string(zend_string_copy(Z_STR_P(default_value)));
		// 如果失败
		} else {
			// 条目里有值 用它创建内置字串，否则 null
			p->value = ini_entry->value ?
				zend_string_init_interned(ini_entry->value, ini_entry->value_length, 1) : NULL;
			// 如果有 on_modify
			if (p->on_modify) {
				// 触发 on_modify。 启动阶段
				p->on_modify(p, p->value, p->mh_arg1, p->mh_arg2, p->mh_arg3, ZEND_INI_STAGE_STARTUP);
			}
		}
		// 下一个条目
		ini_entry++;
	}
	// 返回成功
	return SUCCESS;
}
/* }}} */

// ing4, 注册ini条目。p1:定义的ini条目，p2:模块编号
ZEND_API zend_result zend_register_ini_entries(const zend_ini_entry_def *ini_entry, int module_number) /* {{{ */
{
	zend_module_entry *module;

	// 模块看似是列表的最后一个
	/* Module is likely to be the last one in the list */
	ZEND_HASH_REVERSE_FOREACH_PTR(&module_registry, module) {
		// 如果编号一致
		if (module->module_number == module_number) {
			// 注册ini条目。p1:定义的ini条目，p2:模块号，p3:模块类型
			return zend_register_ini_entries_ex(ini_entry, module_number, module->type);
		}
	} ZEND_HASH_FOREACH_END();

	// 返回失败
	return FAILURE;
}
/* }}} */

// ing4, 删除模块相关的所有 配置指令。p1:模块号，p2:模块类型
ZEND_API void zend_unregister_ini_entries_ex(int module_number, int module_type) /* {{{ */
{
	static HashTable *ini_directives;

	// 如果是临时模块
	if (module_type == MODULE_TEMPORARY) {
		// 使用系统 配置指令表
		ini_directives = EG(ini_directives);
	// 永久模块
	} else {
		// 使用注册过的指令表
		ini_directives = registered_zend_ini_directives;
	}
	// 对每个元素调用 zend_remove_ini_entries 来删除它们。并传入模块号
	zend_hash_apply_with_argument(ini_directives, zend_remove_ini_entries, (void *) &module_number);
}
/* }}} */

// ing4, 删除模块相关的所有 配置指令。p1:模块号
ZEND_API void zend_unregister_ini_entries(int module_number) /* {{{ */
{
	zend_module_entry *module;

	// module 看起来像列表的最后一个
	/* Module is likely to be the last one in the list */
	// 倒序遍历所有模块
	ZEND_HASH_REVERSE_FOREACH_PTR(&module_registry, module) {
		// 如果找到目标模块
		if (module->module_number == module_number) {
			// 删除模块相关的所有 配置指令
			zend_unregister_ini_entries_ex(module_number, module->type);
			return;
		}
	} ZEND_HASH_FOREACH_END();
}
/* }}} */

#ifdef ZTS
// ing4, 刷新所有选项（依次触发 on_modify 事件）
ZEND_API void zend_ini_refresh_caches(int stage) /* {{{ */
{
	zend_ini_entry *p;
	// 遍历所有 配置条目
	ZEND_HASH_MAP_FOREACH_PTR(EG(ini_directives), p) {
		// 如果有 on_modify 方法
		if (p->on_modify) {
			// 触发 on_modify 事件
			p->on_modify(p, p->value, p->mh_arg1, p->mh_arg2, p->mh_arg3, stage);
		}
	} ZEND_HASH_FOREACH_END();
}
/* }}} */
#endif

// ing4, （不强制）修改配置条目 p1:名称，p2:值，p3:修改类型，p4:阶段
ZEND_API zend_result zend_alter_ini_entry(zend_string *name, zend_string *new_value, int modify_type, int stage) /* {{{ */
{
	// 修改配置条目 p1:名称，p2:值，p3:修改类型，p4:阶段，p5:是否强制修改
	return zend_alter_ini_entry_ex(name, new_value, modify_type, stage, 0);
}
/* }}} */

// ing4, (不强制)修改配置条目。p1:条目名称，p2:值，p3:值的长度，p4:修改类型，p4:阶段
ZEND_API zend_result zend_alter_ini_entry_chars(zend_string *name, const char *value, size_t value_length, int modify_type, int stage) /* {{{ */
{
	zend_result ret;
	zend_string *new_value;
	// 给值创建 zend_string
	new_value = zend_string_init(value, value_length, !(stage & ZEND_INI_STAGE_IN_REQUEST));
	// 修改配置条目 p1:名称，p2:值，p3:修改类型，p4:阶段，p5:是否强制修改
	ret = zend_alter_ini_entry_ex(name, new_value, modify_type, stage, 0);
	// 释放zend_string
	zend_string_release(new_value);
	return ret;
}
/* }}} */

// ing4, 修改配置条目。p1:条目名称，p2:值，p3:值的长度，p4:修改类型，p4:阶段，p5:是否改制改变
ZEND_API zend_result zend_alter_ini_entry_chars_ex(zend_string *name, const char *value, size_t value_length, int modify_type, int stage, int force_change) /* {{{ */
{
	zend_result ret;
	zend_string *new_value;
	// 给值创建 zend_string
	new_value = zend_string_init(value, value_length, !(stage & ZEND_INI_STAGE_IN_REQUEST));
	// 修改配置条目 p1:名称，p2:值，p3:修改类型，p4:阶段，p5:是否强制修改
	ret = zend_alter_ini_entry_ex(name, new_value, modify_type, stage, force_change);
	// 释放zend_string
	zend_string_release(new_value);
	return ret;
}
/* }}} */

// ing4, 修改配置条目 p1:名称，p2:值，p3:修改类型，p4:阶段，p5:是否强制修改
ZEND_API zend_result zend_alter_ini_entry_ex(zend_string *name, zend_string *new_value, int modify_type, int stage, bool force_change) /* {{{ */
{
	zend_ini_entry *ini_entry;
	zend_string *duplicate;
	uint8_t modifiable;
	bool modified;
	
	// 在配置指令表里查找 
	if ((ini_entry = zend_hash_find_ptr(EG(ini_directives), name)) == NULL) {
		// 找不到返回 失败
		return FAILURE;
	}

	// 是否可更改
	modifiable = ini_entry->modifiable;
	// 是否已更改
	modified = ini_entry->modified;

	// 处于激活阶段 或 修改类型为 系统类型
	if (stage == ZEND_INI_STAGE_ACTIVATE && modify_type == ZEND_INI_SYSTEM) {
		// 可修改 为 系统类型
		ini_entry->modifiable = ZEND_INI_SYSTEM;
	}

	// 如果要求强制改变
	if (!force_change) {
		// 如果没有 modify_type
		if (!(ini_entry->modifiable & modify_type)) {
			// 返回失败
			return FAILURE;
		}
	}

	// 如果没有修改后的 配置指令表
	if (!EG(modified_ini_directives)) {
		// 创建这个哈希表
		ALLOC_HASHTABLE(EG(modified_ini_directives));
		// 初始化这个哈希表
		zend_hash_init(EG(modified_ini_directives), 8, NULL, NULL, 0);
	}
	// 如果没有修改过
	if (!modified) {
		// 保存原始值
		ini_entry->orig_value = ini_entry->value;
		// 原始可修改状态
		ini_entry->orig_modifiable = modifiable;
		// 标记成已修改
		ini_entry->modified = 1;
		// 把条目添加到 修改后的 配置指令表
		zend_hash_add_ptr(EG(modified_ini_directives), ini_entry->name, ini_entry);
	}
	// 给值创建副本
	duplicate = zend_string_copy(new_value);

	// 如果没有 on_modify 方法或 on_modify 成功
	// p1:配置条目，p2:值副本，p3:参数1，p4:参数2，p5:参数3，p6:阶段
	if (!ini_entry->on_modify
		|| ini_entry->on_modify(ini_entry, duplicate, ini_entry->mh_arg1, ini_entry->mh_arg2, ini_entry->mh_arg3, stage) == SUCCESS) {
		// 如果修改过 并且 值不是原始值。已修改，释放原来的值
		if (modified && ini_entry->orig_value != ini_entry->value) { /* we already changed the value, free the changed value */
			// 释放原来的值
			zend_string_release(ini_entry->value);
		}
		// 使用新值
		ini_entry->value = duplicate;
	// 如果 on_modify 失败
	} else {
		// 释放新值副本
		zend_string_release(duplicate);
		// 返回失败
		return FAILURE;
	}

	// 成功
	return SUCCESS;
}
/* }}} */

// ing3, 恢复指定名称的条目。p1:名称，p2:阶段
ZEND_API zend_result zend_restore_ini_entry(zend_string *name, int stage) /* {{{ */
{
	zend_ini_entry *ini_entry;

	// 查找条目，如果失败 或 当前在不在运行时 或 modifiable 不是用户定义的
	if ((ini_entry = zend_hash_find_ptr(EG(ini_directives), name)) == NULL ||
		(stage == ZEND_INI_STAGE_RUNTIME && (ini_entry->modifiable & ZEND_INI_USER) == 0)) {
		// 反回失败
		return FAILURE;
	}

	// 如果有修改后的配置条目
	if (EG(modified_ini_directives)) {
		// 调用回调方法，如果返回0
		if (zend_restore_ini_entry_cb(ini_entry, stage) == 0) {
			// 在 修改后的配置条目 中表删除这个条目
			zend_hash_del(EG(modified_ini_directives), name);
		// 如果返回 非0
		} else {
			// 返回失败
			return FAILURE;
		}
	}

	// 返回成功
	return SUCCESS;
}
/* }}} */

// ing4, 更新 注册指令表 里配置选项的 displayer 属性。p1:配置名称，p2:名称长度，p3:displayer（zend_ini_entry*）
ZEND_API zend_result zend_ini_register_displayer(const char *name, uint32_t name_length, void (*displayer)(zend_ini_entry *ini_entry, int type)) /* {{{ */
{
	zend_ini_entry *ini_entry;
	// 在 注册指令表 里读取配置
	ini_entry = zend_hash_str_find_ptr(registered_zend_ini_directives, name, name_length);
	// 如果没找到
	if (ini_entry == NULL) {
		// 返回失败
		return FAILURE;
	}
	// 更新 displayer 元素
	ini_entry->displayer = displayer;
	// 返回成功
	return SUCCESS;
}
/* }}} */

/*
数据检索
 * Data retrieval
 */
// ing4, 查找条目，结果转成整数。p1:名称，p2:名称长度，p3:是否查找原始值
ZEND_API zend_long zend_ini_long(const char *name, size_t name_length, int orig) /* {{{ */
{
	zend_ini_entry *ini_entry;
	// 查找条目
	ini_entry = zend_hash_str_find_ptr(EG(ini_directives), name, name_length);
	// 如果找到条目
	if (ini_entry) {
		// 如果要原始值 并且 修改过
		if (orig && ini_entry->modified) {
			// 返回原始值 转成的整数 或 0
			return (ini_entry->orig_value ? ZEND_STRTOL(ZSTR_VAL(ini_entry->orig_value), NULL, 0) : 0);
		// 不要原始值 或没改过
		} else {
			// 返回当前值 转成的整数 或 0
			return (ini_entry->value      ? ZEND_STRTOL(ZSTR_VAL(ini_entry->value), NULL, 0)      : 0);
		}
	}
	// 返回0
	return 0;
}
/* }}} */

// ing4，在配置指令表里读取 小数 值。p1:配置名，p2:置名长度，p3:是否查找原始值
ZEND_API double zend_ini_double(const char *name, size_t name_length, int orig) /* {{{ */
{
	zend_ini_entry *ini_entry;

	// 在指令表里查找
	ini_entry = zend_hash_str_find_ptr(EG(ini_directives), name, name_length);
	// 如果存在
	if (ini_entry) {
		// 如果需要原始值 并且 修改过
		if (orig && ini_entry->modified) {
			// 有原始值返回原值 没有返回 0.0
			return (double) (ini_entry->orig_value ? zend_strtod(ZSTR_VAL(ini_entry->orig_value), NULL) : 0.0);
		// 不需要原始值 或 没修改过
		} else {
			// 有值返回值 没有返回 0.0
			return (double) (ini_entry->value      ? zend_strtod(ZSTR_VAL(ini_entry->value), NULL)      : 0.0);
		}
	}

	// 找不到返回0.0
	return 0.0;
}
/* }}} */

// ing3, 查找配置项的值。p1:配置名，p2:名称长度，p3:是否查找原始值，p4:是否检查存在
ZEND_API char *zend_ini_string_ex(const char *name, size_t name_length, int orig, bool *exists) /* {{{ */
{
	zend_ini_entry *ini_entry;
	// 查找 配置项条目
	ini_entry = zend_hash_str_find_ptr(EG(ini_directives), name, name_length);
	// 如果存在
	if (ini_entry) {
		// 如果要求检查存在
		if (exists) {
			// 返回，存在
			*exists = 1;
		}
		// 如果条目已经修改过
		if (orig && ini_entry->modified) {
			// 如果有原始值，返回原始值，否则 null
			return ini_entry->orig_value ? ZSTR_VAL(ini_entry->orig_value) : NULL;
		// 没有修改过
		} else {
			// 如果有新值
			return ini_entry->value ? ZSTR_VAL(ini_entry->value) : NULL;
		}
	// 不存在
	} else {
		// 如果要求检查存在
		if (exists) {
			// 返回，不存在
			*exists = 0;
		}
		// 返回 null
		return NULL;
	}
}
/* }}} */

// ing3, 查找配置项的值。p1:配置名，p2:名称长度，p3:是否查找原始值
ZEND_API char *zend_ini_string(const char *name, size_t name_length, int orig) /* {{{ */
{
	bool exists = 1;
	char *return_value;

	// 查找配置项的值。p1:配置名，p2:名称长度，p3:是否查找原始值，p4:是否检查存在
	return_value = zend_ini_string_ex(name, name_length, orig, &exists);
	// 如果不存在
	if (!exists) {
		// 返回null
		return NULL;
	// 如果标记为存在，并且实际不存在
	} else if (!return_value) {
		// 返回值为 空字串
		return_value = "";
	}
	// 返回
	return return_value;
}
/* }}} */

// ing4, 查找条目值，p1:配置名称
ZEND_API zend_string *zend_ini_get_value(zend_string *name) /* {{{ */
{
	zend_ini_entry *ini_entry;
	// 在配置表里查找条目
	ini_entry = zend_hash_find_ptr(EG(ini_directives), name);
	// 如果找到
	if (ini_entry) {
		// 返回条目值或 空字串
		return ini_entry->value ? ini_entry->value : ZSTR_EMPTY_ALLOC();
	// 没找到
	} else {
		// 返回null
		return NULL;
	}
}
/* }}} */

// ing4, 字串转布尔型
ZEND_API bool zend_ini_parse_bool(zend_string *str)
{
	// true,yes,on
	if (zend_string_equals_literal_ci(str, "true")
			|| zend_string_equals_literal_ci(str, "yes")
			|| zend_string_equals_literal_ci(str, "on")
	) {
		// 转成1
		return 1;
	// 其他转成整数，如果不是0，返回1，否则0
	} else {
		return atoi(ZSTR_VAL(str)) != 0;
	}
}

// 转换类型
typedef enum {
	// 有符号整数 
	ZEND_INI_PARSE_QUANTITY_SIGNED,
	// 无符号整数
	ZEND_INI_PARSE_QUANTITY_UNSIGNED,
} zend_ini_parse_quantity_signed_result_t;

// 230行
// ing3, 把数字字串转成整数，支持K/M/G的计算，2/8/10/16进制。p1:字串，p2:结果有无符号，p3:错误信息
static zend_ulong zend_ini_parse_quantity_internal(zend_string *value, zend_ini_parse_quantity_signed_result_t signed_result, zend_string **errstr) /* {{{ */
{
	char *digits_end = NULL;
	char *str = ZSTR_VAL(value);
	char *str_end = &str[ZSTR_LEN(value)];
	char *digits = str;
	bool overflow = false;
	zend_ulong factor;
	// 临时变量，报错时存放整个字串
	smart_str invalid = {0};
	// 临时变量，报错时存放有效部分
	smart_str interpreted = {0};
	// 临时变量，报错时存放后缀
	smart_str chr = {0};

	// 忽略开头的空格，ZEND_STRTOL()也会跳过开头的空格，但后续会需要第一个非空字符的位置编号
	/* Ignore leading whitespace. ZEND_STRTOL() also skips leading whitespaces,
	 * but we need the position of the first non-whitespace later. */
	 
	// 从头开始逐个字符遍历，碰到不是空格的跳出
	while (digits < str_end && zend_is_whitespace(*digits)) {++digits;}

	// 忽略结尾的空格
	/* Ignore trailing whitespace */
	// 从结尾开始 逐个字符倒序遍历，碰到不是空格的跳出
	while (digits < str_end && zend_is_whitespace(*(str_end-1))) {--str_end;}

	// 如果开头和结尾碰到一起了。说明空的
	if (digits == str_end) {
		// 返回错误信息为null
		*errstr = NULL;
		// 返回 0
		return 0;
	}

	// 默认不是负数 
	bool is_negative = false;
	// 如果第一个字符是 +
	if (digits[0] == '+') {
		// 找下一个字符 
		++digits;
	// 如果第一个字符是-
	} else if (digits[0] == '-') {
		// 是负数
		is_negative = true;
		// 找下一个字符 
		++digits;
	}

	// 如果 +/-后面没有数字
	/* if there is no digit after +/- */
	// 如果当前字符不是数字（isdigit是系统函数）
	if (!isdigit(digits[0])) {
		// 转换这个字串，来避免null字节 并且 让无法打印的字符可见
		/* Escape the string to avoid null bytes and to make non-printable chars
		 * visible */
		// 把内容拼接在字串里
		smart_str_append_escaped(&invalid, ZSTR_VAL(value), ZSTR_LEN(value));
		// 添加\0
		smart_str_0(&invalid);
		// 错误信息：无效的数量，没有数字开头。为了向后兼容， 解析成 0 
		*errstr = zend_strpprintf(0, "Invalid quantity \"%s\": no valid leading digits, interpreting as \"0\" for backwards compatibility",
						ZSTR_VAL(invalid.s));
		// 释放智能字串
		smart_str_free(&invalid);
		// 返回0
		return 0;
	}

	// 基数是 0，表示10进制 
	int base = 0;
	// 第一个字串是0 第二个不是数字
	if (digits[0] == '0' && !isdigit(digits[1])) {
		// 值为0
		/* Value is just 0 */
		// 如果只有一个0
		if ((digits+1) == str_end) {
			// 没有错误信息
			*errstr = NULL;
			// 返回0
			return 0;
		}
		// 按第一个 非0字符 操作
		switch (digits[1]) {
			// 多后缀。
			/* Multiplier suffixes */
			// 这些是用来计算大小的，G/M/K
			case 'g':
			case 'G':
			case 'm':
			case 'M':
			case 'k':
			case 'K':
				// 转到计算
				goto evaluation;
			// x/X 开头表示16进制
			case 'x':
			case 'X':
				base = 16;
				// 跳出
				break;
			// o/O 开头表示8进制
			case 'o':
			case 'O':
				base = 8;
				// 跳出
				break;
			// b/B 开头表示2进制
			case 'b':
			case 'B':
				base = 2;
				// 跳出
				break;
			// 其他情况
			default:
				// 报错：无效的前缀，为了向后兼容，解析成0
				*errstr = zend_strpprintf(0, "Invalid prefix \"0%c\", interpreting as \"0\" for backwards compatibility",
					digits[1]);
				// 返回 0
				return 0;
        }
		// 向后2个字符
        digits += 2;
		// 如果碰到结尾
		if (UNEXPECTED(digits == str_end)) {
			// 转换这个字串，来避免null字节 并且 让无法打印的字符可见
			/* Escape the string to avoid null bytes and to make non-printable chars
			 * visible */
			// 传入值添加到 invalid 
			smart_str_append_escaped(&invalid, ZSTR_VAL(value), ZSTR_LEN(value));
			// 添加\0
			smart_str_0(&invalid);

			// 错误信息：无效的数量，在基本前缀后面没有数字。为了向后兼容， 解析成 0
			*errstr = zend_strpprintf(0, "Invalid quantity \"%s\": no digits after base prefix, interpreting as \"0\" for backwards compatibility",
							ZSTR_VAL(invalid.s));
			// 释放临时变量
			smart_str_free(&invalid);
			// 返回0
			return 0;
		}
	}
// 计算
	evaluation:
	// 错误吗
	errno = 0;
	
	// 字串转成无符号整数，p1:数字字串，p2:返回 结束位置（字串可能没用完），p3:进制数。（测试过）
	zend_ulong retval = ZEND_STRTOUL(digits, &digits_end, base);

	// 如果错误码是 ERANGE
	if (errno == ERANGE) {
		// 有溢出
		overflow = true;
	// 如果要求结果是无符号
	} else if (signed_result == ZEND_INI_PARSE_QUANTITY_UNSIGNED) {
		// 如果是负数
		if (is_negative) {
			// 忽略-1，它通常当成最大值来用。例如 memory_limit=-1
			/* Ignore "-1" as it is commonly used as max value, for instance in memory_limit=-1. */
			// 如果值为1 并且 一个字符也没用到
			if (retval == 1 && digits_end == str_end) {
				// 返回-1
				retval = -1;
			// 值不为1 或 字串未到结尾
			} else {
				// 标记成溢出
				overflow = true;
			}
		}
	// 如果要求结果是有符号
	} else if (signed_result == ZEND_INI_PARSE_QUANTITY_SIGNED) {
		// 处理 PHP_INT_MIN
		/* Handle PHP_INT_MIN case */
		// 如果是负数 并且 结果值为 ZEND_LONG_MAX+1
		if (is_negative && retval == ((zend_ulong)ZEND_LONG_MAX +1)) {
			// 减法计算负数
			retval = 0u - retval;
		// 如果结果 < 0
		} else if ((zend_long) retval < 0) {
			// 标记成溢出
			overflow = true;
		// 如果 结果不是负数 但 有负数标记
		} else if (is_negative) {
			// 减法计算负数
			retval = 0u - retval;
		}
	}

	// 如果 一个字符也没用到
	if (UNEXPECTED(digits_end == digits)) {
		/* No leading digits */
		// 转换这个字串，来避免null字节 并且 让无法打印的字符可见
		/* Escape the string to avoid null bytes and to make non-printable chars
		 * visible */
		// 传入值添加到 invalid 
		smart_str_append_escaped(&invalid, ZSTR_VAL(value), ZSTR_LEN(value));
		// 添加 \0
		smart_str_0(&invalid);
		// 错误信息：无效的数量，无效的开头数字。为了向后兼容， 解析成 0
		*errstr = zend_strpprintf(0, "Invalid quantity \"%s\": no valid leading digits, interpreting as \"0\" for backwards compatibility",
						ZSTR_VAL(invalid.s));
		// 释放临时变量
		smart_str_free(&invalid);
		// 返回0
		return 0;
	}

	// 在数字和后缀字符间允许存在空格
	/* Allow for whitespace between integer portion and any suffix character */
	// 跳过数字后的空格
	while (digits_end < str_end && zend_is_whitespace(*digits_end)) ++digits_end;

	// 没有 后缀
	/* No exponent suffix. */
	if (digits_end == str_end) {
		// 结束
		goto end;
	}
	// 检查最后一个字符
	switch (*(str_end-1)) {
		// g/G
		case 'g':
		case 'G':
			// 按 2^30算
			factor = 1<<30;
			// 跳出
			break;
		// m/M
		case 'm':
		case 'M':
			// 按 2^20算
			factor = 1<<20;
			// 跳出
			break;
		case 'k':
		case 'K':
			// 按 1^10算
			factor = 1<<10;
			// 跳出
			break;
		// 默认情况
		default:
			// 未知后缀
			/* Unknown suffix */
			// 传入值添加到 invalid 
			smart_str_append_escaped(&invalid, ZSTR_VAL(value), ZSTR_LEN(value));
			// 添加\0
			smart_str_0(&invalid);
			// 字串添加到 interpreted
			smart_str_append_escaped(&interpreted, str, digits_end - str);
			// 添加\0
			smart_str_0(&interpreted);
			// 字串里最后一个字符，添加到 chr
			smart_str_append_escaped(&chr, str_end-1, 1);
			// 添加\0
			smart_str_0(&chr);
			// 错误信息：无效的数量，未知的修饰符 %s。为了向后兼容， 解析成 interpreted + chr
			*errstr = zend_strpprintf(0, "Invalid quantity \"%s\": unknown multiplier \"%s\", interpreting as \"%s\" for backwards compatibility",
						ZSTR_VAL(invalid.s), ZSTR_VAL(chr.s), ZSTR_VAL(interpreted.s));
			// 释放临时变量
			smart_str_free(&invalid);
			smart_str_free(&interpreted);
			smart_str_free(&chr);
			// 返回结果
			return retval;
	}

	// 如果没有溢出
	if (!overflow) {
		// 如果要求结果是有符号
		if (signed_result == ZEND_INI_PARSE_QUANTITY_SIGNED) {
			// 转成有符号整数 
			zend_long sretval = (zend_long)retval;
			// 如果是正数
			if (sretval > 0) {
				// 计算正数溢出，要除掉 k/m/g
				overflow = (zend_long)retval > ZEND_LONG_MAX / (zend_long)factor;
			// 负数
			} else {
				// 计算负数溢出，要除掉 k/m/g
				overflow = (zend_long)retval < ZEND_LONG_MIN / (zend_long)factor;
			}
		// 不要求有符号
		} else {
			// 计算无符号溢出 ，要除掉 k/m/g
			overflow = retval > ZEND_ULONG_MAX / factor;
		}
	}

	// 乘以倍数 K/M/G
	retval *= factor;

	// 如果数字结尾不是字串结尾
	if (UNEXPECTED(digits_end != str_end-1)) {
		// 结尾有1个以上的后缀
		/* More than one character in suffix */
		// 传入值添加到 invalid 
		smart_str_append_escaped(&invalid, ZSTR_VAL(value), ZSTR_LEN(value));
		// 添加\0
		smart_str_0(&invalid);
		// 字串添加到 interpreted
		smart_str_append_escaped(&interpreted, str, digits_end - str);
		// 添加\0
		smart_str_0(&interpreted);
		// 字串里最后一个字符，添加到 chr
		smart_str_append_escaped(&chr, str_end-1, 1);
		// 添加\0
		smart_str_0(&chr);
		// 错误信息：无效的数量。为了向后兼容， 解析成 interpreted + chr
		*errstr = zend_strpprintf(0, "Invalid quantity \"%s\", interpreting as \"%s%s\" for backwards compatibility",
						ZSTR_VAL(invalid.s), ZSTR_VAL(interpreted.s), ZSTR_VAL(chr.s));
		// 释放临时变量
		smart_str_free(&invalid);
		smart_str_free(&interpreted);
		smart_str_free(&chr);
		// 返回结果 
		return retval;
	}

// 结尾
end:
	// 如果有溢出
	if (UNEXPECTED(overflow)) {
		// 把value放进智能字串
		smart_str_append_escaped(&invalid, ZSTR_VAL(value), ZSTR_LEN(value));
		// 加\0
		smart_str_0(&invalid);
		// 这里没有指定最终结果，因为调用者可以会再进行转换。
		// 没有指定允许区域，因为调用者可能进行更严格的区域检查。
		/* Not specifying the resulting value here because the caller may make
		 * additional conversions. Not specifying the allowed range
		 * because the caller may do narrower range checks. */
		// 错误信息：无效的数量，值超出范围。为了向后兼容， 解析成 溢出值 
		*errstr = zend_strpprintf(0, "Invalid quantity \"%s\": value is out of range, using overflow result for backwards compatibility",
						ZSTR_VAL(invalid.s));

		// 释放临时变量
		smart_str_free(&invalid);
		smart_str_free(&interpreted);
		smart_str_free(&chr);

		// 返回结果
		return retval;
	}

	// 错误信息为空
	*errstr = NULL;
	// 返回结果
	return retval;
}
/* }}} */

// ing4, 把数字字串转成有符号整数，支持K/M/G的计算，2/8/10/16进制。p1:字串，p2:错误信息
ZEND_API zend_long zend_ini_parse_quantity(zend_string *value, zend_string **errstr) /* {{{ */
{
	// 把数字字串转成整数，支持K/M/G的计算，2/8/10/16进制。p1:字串，p2:结果有无符号，p3:错误信息
	return (zend_long) zend_ini_parse_quantity_internal(value, ZEND_INI_PARSE_QUANTITY_SIGNED, errstr);
}
/* }}} */

// ing4, 把数字字串转成无符号整数，支持K/M/G的计算，2/8/10/16进制。p1:字串，p2:错误信息
ZEND_API zend_ulong zend_ini_parse_uquantity(zend_string *value, zend_string **errstr) /* {{{ */
{
	// 把数字字串转成整数，支持K/M/G的计算，2/8/10/16进制。p1:字串，p2:结果有无符号，p3:错误信息
	return zend_ini_parse_quantity_internal(value, ZEND_INI_PARSE_QUANTITY_UNSIGNED, errstr);
}
/* }}} */

// ing4, 获取配置项整数值。失败则报错。p1:数字字串，p2:配置名
ZEND_API zend_long zend_ini_parse_quantity_warn(zend_string *value, zend_string *setting) /* {{{ */
{
	zend_string *errstr;
	// 把数字字串转成有符号整数，支持K/M/G的计算，2/8/10/16进制。p1:字串，p2:错误信息
	zend_long retval = zend_ini_parse_quantity(value, &errstr);

	// 如果有错误信息
	if (errstr) {
		// 报错无效的设置
		zend_error(E_WARNING, "Invalid \"%s\" setting. %s", ZSTR_VAL(setting), ZSTR_VAL(errstr));
		zend_string_release(errstr);
	}
	// 返回结果值
	return retval;
}
/* }}} */

// ing4, 获取配置项 无符号整数值。失败则报错。p1:数字字串，p2:配置名
ZEND_API zend_ulong zend_ini_parse_uquantity_warn(zend_string *value, zend_string *setting) /* {{{ */
{
	zend_string *errstr;
	// 把数字字串转成无符号整数，支持K/M/G的计算，2/8/10/16进制。p1:字串，p2:错误信息
	zend_ulong retval = zend_ini_parse_uquantity(value, &errstr);

	// 如果有错误信息
	if (errstr) {
		// 报错无效的设置
		zend_error(E_WARNING, "Invalid \"%s\" setting. %s", ZSTR_VAL(setting), ZSTR_VAL(errstr));
		zend_string_release(errstr);
	}
	// 返回结果值
	return retval;
}
/* }}} */

// ing3, 显示布尔型值 的回调
// ZEND_COLD void zend_ini_boolean_displayer_cb(zend_ini_entry *ini_entry, int type)
ZEND_INI_DISP(zend_ini_boolean_displayer_cb) /* {{{ */
{
	int value;
	zend_string *tmp_value;

	// 如果 要求显示原始值并且，条目被修改过
	// #define ZEND_INI_DISPLAY_ORIG	1
	if (type == ZEND_INI_DISPLAY_ORIG && ini_entry->modified) {
		// 原始值 或 null
		tmp_value = (ini_entry->orig_value ? ini_entry->orig_value : NULL );
	// 如果条目有值
	} else if (ini_entry->value) {
		// 使用条目值
		tmp_value = ini_entry->value;
	// 其他情况
	} else {
		// 值为null
		tmp_value = NULL;
	}

	// 如果值有效
	if (tmp_value) {
		// 转成bool型
		value = zend_ini_parse_bool(tmp_value);
	// 无效
	} else {
		// false
		value = 0;
	}

	// 如果是true
	if (value) {
		// 显示 On
		ZEND_PUTS("On");
	} else {
		// 显示 Off
		ZEND_PUTS("Off");
	}
}
/* }}} */

// ing3, 显示配置值，如果开启 zend_uv.html_errors ，可添加颜色
// ZEND_COLD void zend_ini_color_displayer_cb(zend_ini_entry *ini_entry, int type)
ZEND_INI_DISP(zend_ini_color_displayer_cb) /* {{{ */
{
	char *value;

	// 如果 要求显示原始值并且，条目被修改过
	// #define ZEND_INI_DISPLAY_ORIG	1
	if (type == ZEND_INI_DISPLAY_ORIG && ini_entry->modified) {
		// 原始值
		value = ZSTR_VAL(ini_entry->orig_value);
	// 如果条目有值
	} else if (ini_entry->value) {
		// 当前值
		value = ZSTR_VAL(ini_entry->value);
	// 其他情况
	} else {
		// null
		value = NULL;
	}
	
	// 如果值有效
	if (value) {
		// 如果使用 html_errors
		if (zend_uv.html_errors) {
			// 显示带颜色的字串
			zend_printf("<font style=\"color: %s\">%s</font>", value, value);
		// 普通显示
		} else {
			ZEND_PUTS(value);
		}
	// 值无效
	} else {
		// 如果使用 html_errors
		if (zend_uv.html_errors) {
			// 显示 <i>no value</i>
			ZEND_PUTS(NO_VALUE_HTML);
		// 普通显示
		} else {
			// 显示 no value
			ZEND_PUTS(NO_VALUE_PLAINTEXT);
		}
	}
}
/* }}} */

// ing3, 显示整数值 的回调
// ZEND_COLD void display_link_numbers(zend_ini_entry *ini_entry, int type)
ZEND_INI_DISP(display_link_numbers) /* {{{ */
{
	char *value;

	// 如果 要求显示原始值并且， 条目被修改过
	// #define ZEND_INI_DISPLAY_ORIG	1
	if (type == ZEND_INI_DISPLAY_ORIG && ini_entry->modified) {
		// 取出条原始目值，字串
		value = ZSTR_VAL(ini_entry->orig_value);
	// 如果条目有值
	} else if (ini_entry->value) {
		// 取出条目值，字串
		value = ZSTR_VAL(ini_entry->value);
	// 其他情况
	} else {
		// 值为null
		value = NULL;
	}

	// 如果值有效
	if (value) {
		// 如果转成整数是 -1
		if (atoi(value) == -1) {
			// 显示 Unlimited
			ZEND_PUTS("Unlimited");
		// 其他情况
		} else {
			// 打印值
			zend_printf("%s", value);
		}
	}
}
/* }}} */


// 标准信息处理
/* Standard message handlers */

// ing3, 事件：更新成布尔值
// int OnUpdateBool(zend_ini_entry *entry, zend_string *new_value, void *mh_arg1, void *mh_arg2, void *mh_arg3, int stage)
ZEND_API ZEND_INI_MH(OnUpdateBool) /* {{{ */
{
	// 取得 布尔型 指针
	// 取得参数 mh_arg2 ，并把指针前进 mh_arg1 个字节
	bool *p = (bool *) ZEND_INI_GET_ADDR();
	// 更新这个bool变量
	*p = zend_ini_parse_bool(new_value);
	// 返回成功
	return SUCCESS;
}
/* }}} */

// ing3, 事件：更新成整数
// int OnUpdateLong(zend_ini_entry *entry, zend_string *new_value, void *mh_arg1, void *mh_arg2, void *mh_arg3, int stage)
ZEND_API ZEND_INI_MH(OnUpdateLong) /* {{{ */
{
	// 取得 整数 指针
	// 取得参数 mh_arg2 ，并把指针前进 mh_arg1 个字节
	zend_long *p = (zend_long *) ZEND_INI_GET_ADDR();
	// 获取配置项整数值。失败则报错。p1:数字字串，p2:配置名
	*p = zend_ini_parse_quantity_warn(new_value, entry->name);
	// 返回成功
	return SUCCESS;
}
/* }}} */

// ing3, 事件：更新成0或正整数
// int OnUpdateLongGEZero(zend_ini_entry *entry, zend_string *new_value, void *mh_arg1, void *mh_arg2, void *mh_arg3, int stage)
ZEND_API ZEND_INI_MH(OnUpdateLongGEZero) /* {{{ */
{
	// 获取配置项整数值。失败则报错。p1:数字字串，p2:配置名
	zend_long tmp = zend_ini_parse_quantity_warn(new_value, entry->name);
	// 如果值小于0
	if (tmp < 0) {
		// 返回失败
		return FAILURE;
	}
	// 取得 整数 指针
	// 取得参数 mh_arg2 ，并把指针前进 mh_arg1 个字节
	zend_long *p = (zend_long *) ZEND_INI_GET_ADDR();
	// 更新目标整数
	*p = tmp;
	// 返回成功
	return SUCCESS;
}
/* }}} */

// ing3, 事件：更新成小数
// int OnUpdateReal(zend_ini_entry *entry, zend_string *new_value, void *mh_arg1, void *mh_arg2, void *mh_arg3, int stage)
ZEND_API ZEND_INI_MH(OnUpdateReal) /* {{{ */
{
	// 取得 小数 指针
	// 取得参数 mh_arg2 ，并把指针前进 mh_arg1 个字节
	double *p = (double *) ZEND_INI_GET_ADDR();
	// 值转成小数并更新到指定位置
	*p = zend_strtod(ZSTR_VAL(new_value), NULL);
	// 返回成功
	return SUCCESS;
}
/* }}} */

// ing3, 事件：更新成字串
// int OnUpdateString(zend_ini_entry *entry, zend_string *new_value, void *mh_arg1, void *mh_arg2, void *mh_arg3, int stage)
ZEND_API ZEND_INI_MH(OnUpdateString) /* {{{ */
{
	// 取得 字串 指针
	// 取得参数 mh_arg2 ，并把指针前进 mh_arg1 个字节
	char **p = (char **) ZEND_INI_GET_ADDR();
	// 字串更新到指定位置，如果无效更新成 null
	*p = new_value ? ZSTR_VAL(new_value) : NULL;
	// 返回成功
	return SUCCESS;
}
/* }}} */

// ing3, 事件：更新成非空 字串
// int OnUpdateStringUnempty(zend_ini_entry *entry, zend_string *new_value, void *mh_arg1, void *mh_arg2, void *mh_arg3, int stage)
ZEND_API ZEND_INI_MH(OnUpdateStringUnempty) /* {{{ */
{
	// 如果有字串 并且 第一个字符不存在
	if (new_value && !ZSTR_VAL(new_value)[0]) {
		// 返回失败
		return FAILURE;
	}
	// 取得 字串 指针
	// 取得参数 mh_arg2 ，并把指针前进 mh_arg1 个字节
	char **p = (char **) ZEND_INI_GET_ADDR();
	// 更新字串
	*p = new_value ? ZSTR_VAL(new_value) : NULL;
	// 返回成功
	return SUCCESS;
}
/* }}} */

// ing3, 事件：更新成 zend_string
// int OnUpdateStr(zend_ini_entry *entry, zend_string *new_value, void *mh_arg1, void *mh_arg2, void *mh_arg3, int stage)
ZEND_API ZEND_INI_MH(OnUpdateStr) /* {{{ */
{
	// 取得 zend_string 指针
	// 取得参数 mh_arg2 ，并把指针前进 mh_arg1 个字节
	zend_string **p = (zend_string **) ZEND_INI_GET_ADDR();
	// 更新 zend_string
	*p = new_value;
	// 返回成功
	return SUCCESS;
}
/* }}} */

// ing3, 事件：更新成非空 zend_string
// int OnUpdateStrNotEmpty(zend_ini_entry *entry, zend_string *new_value, void *mh_arg1, void *mh_arg2, void *mh_arg3, int stage)
ZEND_API ZEND_INI_MH(OnUpdateStrNotEmpty) /* {{{ */
{
	// 如果传入了新值，但新值无效
	if (new_value && ZSTR_LEN(new_value) == 0) {
		// 返回失败
		return FAILURE;
	}
	// 取得 zend_string 指针
	// 取得参数 mh_arg2 ，并把指针前进 mh_arg1 个字节
	zend_string **p = (zend_string **) ZEND_INI_GET_ADDR();
	// 更新 zend_string
	*p = new_value;
	// 返回成功
	return SUCCESS;
}
/* }}} */
