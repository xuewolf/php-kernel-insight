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
   | Authors: Benjamin Eberlei <kontakt@beberlei.de>                      |
   |          Martin Schröder <m.schroeder2007@gmail.com>                 |
   +----------------------------------------------------------------------+
*/

#ifndef ZEND_ATTRIBUTES_H
#define ZEND_ATTRIBUTES_H

// 修饰目标 类
#define ZEND_ATTRIBUTE_TARGET_CLASS			(1<<0)
// 修饰目标 函数
#define ZEND_ATTRIBUTE_TARGET_FUNCTION		(1<<1)
// 修饰目标 类方法
#define ZEND_ATTRIBUTE_TARGET_METHOD		(1<<2)
// 修饰目标 类属性
#define ZEND_ATTRIBUTE_TARGET_PROPERTY		(1<<3)
// 修饰目标 类常量
#define ZEND_ATTRIBUTE_TARGET_CLASS_CONST	(1<<4)
// 修饰目标 参数
#define ZEND_ATTRIBUTE_TARGET_PARAMETER		(1<<5)
// 修饰目标 全部 5个1, 这个相当于mask
#define ZEND_ATTRIBUTE_TARGET_ALL			((1<<6) - 1)
// 修饰目标 可重复
#define ZEND_ATTRIBUTE_IS_REPEATABLE		(1<<6)
// 修饰属性标记，6个1，这个相当于mask
#define ZEND_ATTRIBUTE_FLAGS				((1<<7) - 1)

/* Flags for zend_attribute.flags */
// 持久
#define ZEND_ATTRIBUTE_PERSISTENT   (1<<0)
// 严格类型
#define ZEND_ATTRIBUTE_STRICT_TYPES (1<<1)

// ing4, 计算n个参数的 zend_attribute 的大小
// 56 + 24 * argc - 24 ( 因为第一个 zend_attribute_arg 包含在 zend_attribute 里面了）
#define ZEND_ATTRIBUTE_SIZE(argc) \
	(sizeof(zend_attribute) + sizeof(zend_attribute_arg) * (argc) - sizeof(zend_attribute_arg))

BEGIN_EXTERN_C()

extern ZEND_API zend_class_entry *zend_ce_attribute;
extern ZEND_API zend_class_entry *zend_ce_allow_dynamic_properties;
extern ZEND_API zend_class_entry *zend_ce_sensitive_parameter;
extern ZEND_API zend_class_entry *zend_ce_sensitive_parameter_value;

// 修饰属性参数
typedef struct {
	// 参数名
	zend_string *name;
	// 值
	zval value;
} zend_attribute_arg;

// 修饰属性
typedef struct _zend_attribute {
	// 属性名
	zend_string *name;
	// 小写名
	zend_string *lcname;
	// 标记
	uint32_t flags;
	// 行号
	uint32_t lineno;
	// 参数的 offsets 从1开始，其他情况都是0 
	/* Parameter offsets start at 1, everything else uses 0. */
	uint32_t offset;
	// 传入参数
	uint32_t argc;
	// 能数列表
	zend_attribute_arg args[1];
} zend_attribute;

// 内部修饰属性
typedef struct _zend_internal_attribute {
	// 类入口指针
	zend_class_entry *ce;
	// 标记
	uint32_t flags;
	// validator 函数指针
	void (*validator)(zend_attribute *attr, uint32_t target, zend_class_entry *scope);
} zend_internal_attribute;

// 抽象函数
ZEND_API zend_attribute *zend_get_attribute(HashTable *attributes, zend_string *lcname);
ZEND_API zend_attribute *zend_get_attribute_str(HashTable *attributes, const char *str, size_t len);

ZEND_API zend_attribute *zend_get_parameter_attribute(HashTable *attributes, zend_string *lcname, uint32_t offset);
ZEND_API zend_attribute *zend_get_parameter_attribute_str(HashTable *attributes, const char *str, size_t len, uint32_t offset);

ZEND_API zend_result zend_get_attribute_value(zval *ret, zend_attribute *attr, uint32_t i, zend_class_entry *scope);

ZEND_API zend_string *zend_get_attribute_target_names(uint32_t targets);
ZEND_API bool zend_is_attribute_repeated(HashTable *attributes, zend_attribute *attr);

ZEND_API zend_internal_attribute *zend_mark_internal_attribute(zend_class_entry *ce);
ZEND_API zend_internal_attribute *zend_internal_attribute_register(zend_class_entry *ce, uint32_t flags);
ZEND_API zend_internal_attribute *zend_internal_attribute_get(zend_string *lcname);

// ing3, 添加修饰属性，传入，哈希表指针的指针，修饰属性名，参数数量，flags，序号，行号
ZEND_API zend_attribute *zend_add_attribute(
		HashTable **attributes, zend_string *name, uint32_t argc,
		uint32_t flags, uint32_t offset, uint32_t lineno);

END_EXTERN_C()

// ing3, 给类 添加修饰属性，传入：类入口，修饰属性名，参数数量
static zend_always_inline zend_attribute *zend_add_class_attribute(zend_class_entry *ce, zend_string *name, uint32_t argc)
{
	// 标记 。用户类：持久，其他情况：非持久
	uint32_t flags = ce->type != ZEND_USER_CLASS ? ZEND_ATTRIBUTE_PERSISTENT : 0;
	// ing3, 添加修饰属性，传入，哈希表指针的指针，修饰属性名，参数数量，flags，序号，行号
	return zend_add_attribute(&ce->attributes, name, argc, flags, 0, 0);
}

// ing3, 给函数 添加修饰属性，传入：函数实例指针，修饰属性名，参数数量
static zend_always_inline zend_attribute *zend_add_function_attribute(zend_function *func, zend_string *name, uint32_t argc)
{
	uint32_t flags = func->common.type != ZEND_USER_FUNCTION ? ZEND_ATTRIBUTE_PERSISTENT : 0;
	// ing3, 添加修饰属性，传入，哈希表指针的指针，修饰属性名，参数数量，flags，序号，行号
	return zend_add_attribute(&func->common.attributes, name, argc, flags, 0, 0);
}

// ing3, 给参数 添加修饰属性，传入：函数实例指针，偏移量，修饰属性名，参数数量
static zend_always_inline zend_attribute *zend_add_parameter_attribute(zend_function *func, uint32_t offset, zend_string *name, uint32_t argc)
{
	uint32_t flags = func->common.type != ZEND_USER_FUNCTION ? ZEND_ATTRIBUTE_PERSISTENT : 0;
	// ing3, 添加修饰属性，传入，哈希表指针的指针，修饰属性名，参数数量，flags，序号，行号
	return zend_add_attribute(&func->common.attributes, name, argc, flags, offset + 1, 0);
}

// ing3, 给类属性 添加修饰属性，传入：类入口，属性信息，修饰属性名，参数数量
static zend_always_inline zend_attribute *zend_add_property_attribute(zend_class_entry *ce, zend_property_info *info, zend_string *name, uint32_t argc)
{
	uint32_t flags = ce->type != ZEND_USER_CLASS ? ZEND_ATTRIBUTE_PERSISTENT : 0;
	// ing3, 添加修饰属性，传入，哈希表指针的指针，修饰属性名，参数数量，flags，序号，行号
	return zend_add_attribute(&info->attributes, name, argc, flags, 0, 0);
}

// ing3, 给类常量 添加修饰属性，传入：类入口，常量指针，修饰属性名，参数数量
static zend_always_inline zend_attribute *zend_add_class_constant_attribute(zend_class_entry *ce, zend_class_constant *c, zend_string *name, uint32_t argc)
{
	// 如果不是用户类，持久
	uint32_t flags = ce->type != ZEND_USER_CLASS ? ZEND_ATTRIBUTE_PERSISTENT : 0;
	// ing3, 添加修饰属性，传入，哈希表指针的指针，属性名，参数数量，flags，序号，行号
	return zend_add_attribute(&c->attributes, name, argc, flags, 0, 0);
}

// 
void zend_register_attribute_ce(void);
void zend_attributes_shutdown(void);

#endif
