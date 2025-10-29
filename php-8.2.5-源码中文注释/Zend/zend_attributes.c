// ？ 没有外部调用
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

#include "zend.h"
#include "zend_API.h"
#include "zend_attributes.h"
#include "zend_attributes_arginfo.h"
#include "zend_exceptions.h"
#include "zend_smart_str.h"

ZEND_API zend_class_entry *zend_ce_attribute;
ZEND_API zend_class_entry *zend_ce_return_type_will_change_attribute;
ZEND_API zend_class_entry *zend_ce_allow_dynamic_properties;
ZEND_API zend_class_entry *zend_ce_sensitive_parameter;
ZEND_API zend_class_entry *zend_ce_sensitive_parameter_value;

static zend_object_handlers attributes_object_handlers_sensitive_parameter_value;

static HashTable internal_attributes;

// ing4, 检验属性
void validate_attribute(zend_attribute *attr, uint32_t target, zend_class_entry *scope)
{
	// TODO: More proper signature validation: Too many args, incorrect arg names.
	// 如果有参数
	if (attr->argc > 0) {
		zval flags;
		// 在编译过程中运行时，不用指定scope，直接取出属性值即可
		// 类还没有完全链接好，这时会看到不一致的状态
		/* As this is run in the middle of compilation, fetch the attribute value without
		 * specifying a scope. The class is not fully linked yet, and we may seen an
		 * inconsistent state. */
		 
		// 如果查找不到此属性的值，直接返回
		if (FAILURE == zend_get_attribute_value(&flags, attr, 0, NULL)) {
			return;
		}
		// 如果返回值不是整数 
		if (Z_TYPE(flags) != IS_LONG) {
			// 报错： Attribute::__construct() 第一个参数必须是整数
			zend_error_noreturn(E_ERROR,
				"Attribute::__construct(): Argument #1 ($flags) must be of type int, %s given",
				zend_zval_type_name(&flags)
			);
		}
		// 如果返回值没有 ZEND_ATTRIBUTE_FLAGS 修饰属性标记
		if (Z_LVAL(flags) & ~ZEND_ATTRIBUTE_FLAGS) {
			// 报错：无效的修饰属性标记
			zend_error_noreturn(E_ERROR, "Invalid attribute flags specified");
		}
		// 验证通过，销毁返回值
		zval_ptr_dtor(&flags);
	}
}

// ing4, 不可以给 trait，interface，readonly class 添加 AllowDynamicProperties 修饰符（测试过）
static void validate_allow_dynamic_properties(
		zend_attribute *attr, uint32_t target, zend_class_entry *scope)
{
	// trait
	if (scope->ce_flags & ZEND_ACC_TRAIT) {
		zend_error_noreturn(E_ERROR, "Cannot apply #[AllowDynamicProperties] to trait");
	}
	// interface
	if (scope->ce_flags & ZEND_ACC_INTERFACE) {
		zend_error_noreturn(E_ERROR, "Cannot apply #[AllowDynamicProperties] to interface");
	}
	// 只读类
	if (scope->ce_flags & ZEND_ACC_READONLY_CLASS) {
		zend_error_noreturn(E_ERROR, "Cannot apply #[AllowDynamicProperties] to readonly class %s",
			ZSTR_VAL(scope->name)
		);
	}
	// 如果通过测试，添加 允许动态属性标记
	scope->ce_flags |= ZEND_ACC_ALLOW_DYNAMIC_PROPERTIES;
}

// ing3, Attribute::__construct
ZEND_METHOD(Attribute, __construct)
{	
	// #define ZEND_ATTRIBUTE_TARGET_ALL ((1<<6) - 1)
	zend_long flags = ZEND_ATTRIBUTE_TARGET_ALL;
	// 验证参数数量是否合法并取出参数列表, 传入：最小数量，最大数量 
	ZEND_PARSE_PARAMETERS_START(0, 1)
		// 标记成可选参数（这个参数有默认值）
		Z_PARAM_OPTIONAL
		// 获取整数值
		Z_PARAM_LONG(flags)
	ZEND_PARSE_PARAMETERS_END();
	// 更新 this 对象的第1个 属性 的值
	ZVAL_LONG(OBJ_PROP_NUM(Z_OBJ_P(ZEND_THIS), 0), flags);
}

// ing3, ReturnTypeWillChange::__construct
ZEND_METHOD(ReturnTypeWillChange, __construct)
{
	// ing4, 不可以有传入参数，有就报错
	ZEND_PARSE_PARAMETERS_NONE();
}

// ing3, AllowDynamicProperties::__construct
ZEND_METHOD(AllowDynamicProperties, __construct)
{
	// ing4, 不可以有传入参数，有就报错
	ZEND_PARSE_PARAMETERS_NONE();
}

// ing3, SensitiveParameter::__construct
ZEND_METHOD(SensitiveParameter, __construct)
{
	// ing4, 不可以有传入参数，有就报错
	ZEND_PARSE_PARAMETERS_NONE();
}

// ing3, SensitiveParameterValue::__construct
ZEND_METHOD(SensitiveParameterValue, __construct)
{
	zval *value;
	// 验证参数数量是否合法并取出参数列表, 传入：最小数量，最大数量 
	ZEND_PARSE_PARAMETERS_START(1, 1)
		// ing4, 取下一个参数，不检测null，返回什么就是什么
		Z_PARAM_ZVAL(value)
	ZEND_PARSE_PARAMETERS_END();
	// 更新 this  的value 属性，把刚刚获取到的参数写进去
	zend_update_property(zend_ce_sensitive_parameter_value, Z_OBJ_P(ZEND_THIS), "value", strlen("value"), value);
}

// ing3, SensitiveParameterValue::getValue
// 返回当前执行域的 第一个 属性
ZEND_METHOD(SensitiveParameterValue, getValue)
{
	// ing4, 不可以有传入参数，有就报错
	ZEND_PARSE_PARAMETERS_NONE();
	// 通过索引号获取指定对象的属性值，返回 *zval
	ZVAL_COPY(return_value, OBJ_PROP_NUM(Z_OBJ_P(ZEND_THIS), 0));
}

// ing3, SensitiveParameterValue::__debugInfo
ZEND_METHOD(SensitiveParameterValue, __debugInfo)
{
	// ing4, 不可以有传入参数，有就报错
	ZEND_PARSE_PARAMETERS_NONE();
	// 返回空数组
	RETURN_EMPTY_ARRAY();
}

// ing3, 创建一个新的对象作为 参数值
static zend_object *attributes_sensitive_parameter_value_new(zend_class_entry *ce)
{
	zend_object *object;
	// 用指定类创建对象
	object = zend_objects_new(ce);
	// 给对象添加处理器 attributes_object_handlers_sensitive_parameter_value = std_object_handlers
	// 添加的还是标准处理器
	object->handlers = &attributes_object_handlers_sensitive_parameter_value;
	// 初始化类属性
	object_properties_init(object, ce);

	return object;
}

// ing4, 固定返回 NULL
static HashTable *attributes_sensitive_parameter_value_get_properties_for(zend_object *zobj, zend_prop_purpose purpose)
{
	return NULL;
}

// ing4, 在属性表里查找名称和偏移量都相同的属性
static zend_attribute *get_attribute(HashTable *attributes, zend_string *lcname, uint32_t offset)
{
	// 如果属性哈希表有效
	if (attributes) {
		// 遍历用的元素指针
		zend_attribute *attr;
		// 遍历哈希表
		ZEND_HASH_PACKED_FOREACH_PTR(attributes, attr) {
			// 如果 此属性的偏移量与要求的相同 ， 此属性的名称也与要求的相同
			if (attr->offset == offset && zend_string_equals(attr->lcname, lcname)) {
				// 返回此属性
				return attr;
			}
		} ZEND_HASH_FOREACH_END();
	}
	// 查找失败返回null
	return NULL;
}

// ing4, 在属性表里查找名称和偏移量都相同的属性
static zend_attribute *get_attribute_str(HashTable *attributes, const char *str, size_t len, uint32_t offset)
{
	// 如果属性哈希表有效
	if (attributes) {
		// 遍历用的元素指针
		zend_attribute *attr;
		// 遍历哈希表
		ZEND_HASH_PACKED_FOREACH_PTR(attributes, attr) {
			// 如果 此属性的偏移量与要求的相同 ， 此属性的名称也与要求的相同
			if (attr->offset == offset && zend_string_equals_cstr(attr->lcname, str, len)) {
				// 返回此属性
				return attr;
			}
		} ZEND_HASH_FOREACH_END();
	}
	// 查找失败返回null
	return NULL;
}

// ing4, 在属性表里 通过键名查找属性, 偏移量为0
ZEND_API zend_attribute *zend_get_attribute(HashTable *attributes, zend_string *lcname)
{
	return get_attribute(attributes, lcname, 0);
}

// ing4, 在属性表里 通过 键名 和 长度 查找属性, 偏移量为0
ZEND_API zend_attribute *zend_get_attribute_str(HashTable *attributes, const char *str, size_t len)
{
	return get_attribute_str(attributes, str, len, 0);
}

// ing3, 在属性表里查找名称和偏移量都相同的属性，偏移量自动+1 （offsets 从1开始）
ZEND_API zend_attribute *zend_get_parameter_attribute(HashTable *attributes, zend_string *lcname, uint32_t offset)
{
	return get_attribute(attributes, lcname, offset + 1);
}

// ing3, 在属性表里查找名称和偏移量都相同的属性，偏移量自动+1 （offsets 从1开始）
ZEND_API zend_attribute *zend_get_parameter_attribute_str(HashTable *attributes, const char *str, size_t len, uint32_t offset)
{
	return get_attribute_str(attributes, str, len, offset + 1);
}

// ing3, 查找并返回 zend_attribute 的 指定序号 的参数，scope 用于更新常量
ZEND_API zend_result zend_get_attribute_value(zval *ret, zend_attribute *attr, uint32_t i, zend_class_entry *scope)
{
	// 如果要求的能数序号无效
	if (i >= attr->argc) {
		// 返回失败
		return FAILURE;
	}
	// 把指定序号的能数的 value 复制给 结果
	ZVAL_COPY_OR_DUP(ret, &attr->args[i].value);

	// 如果结果是常量表达式
	if (Z_TYPE_P(ret) == IS_CONSTANT_AST) {
		// 如果更新常量失败
		if (SUCCESS != zval_update_constant_ex(ret, scope)) {
			// 销毁结果
			zval_ptr_dtor(ret);
			// 返回失败
			return FAILURE;
		}
	}

	return SUCCESS;
}

// 目标名称，可以是类，函数，成员方法，类属性，类常量，参数
static const char *target_names[] = {
	"class",
	"function",
	"method",
	"property",
	"class constant",
	"parameter"
};

// ing2, 获取修饰属性的目标名称
ZEND_API zend_string *zend_get_attribute_target_names(uint32_t flags)
{
	// 新字串
	smart_str str = { 0 };
	// 遍历所有名称（测试过）
	for (uint32_t i = 0; i < (sizeof(target_names) / sizeof(char *)); i++) {
		// 如果flags里面有这个名称
		if (flags & (1 << i)) {
			// 如果 str 不空
			if (smart_str_get_len(&str)) {
				// 加个豆
				smart_str_appends(&str, ", ");
			}
			// 把名字拼在后面
			smart_str_appends(&str, target_names[i]);
		}
	}
	// 取出并返回里面的 zend_string
	return smart_str_extract(&str);
}

// ing4, 检查属性表里是否有 与此属性名称和偏移量一样 的 其他实例
ZEND_API bool zend_is_attribute_repeated(HashTable *attributes, zend_attribute *attr)
{
	zend_attribute *other;
	// 遍历 属性表
	ZEND_HASH_PACKED_FOREACH_PTR(attributes, other) {
		// 找到相同序号 的非同一个属性
		if (other != attr && other->offset == attr->offset) {
			// 如果名称也重复
			if (zend_string_equals(other->lcname, attr->lcname)) {
				// 返回：重复
				return 1;
			}
		}
	} ZEND_HASH_FOREACH_END();
	// 返回：否
	return 0;
}

// ing3, 释放属性实例
static void attr_free(zval *v)
{
	// 取出 zval里的属性对象
	zend_attribute *attr = Z_PTR_P(v);
	// 添加持久属性标记
	bool persistent = attr->flags & ZEND_ATTRIBUTE_PERSISTENT;
	// 释放属性名
	zend_string_release(attr->name);
	// 释放小写属性名
	zend_string_release(attr->lcname);
	// 遍历所有参数
	for (uint32_t i = 0; i < attr->argc; i++) {
		// 如果有参数名
		if (attr->args[i].name) {
			// 释放参数名
			zend_string_release(attr->args[i].name);
		}
		// 如果是持久存储
		if (persistent) {
			// 销毁内部指针
			zval_internal_ptr_dtor(&attr->args[i].value);
		// 非持久
		} else {
			// 销毁普通指针
			zval_ptr_dtor(&attr->args[i].value);
		}
	}
	// 释放整个属性实例
	pefree(attr, persistent);
}

// ing3, 添加修饰属性，传入，哈希表指针的指针，属性名，参数数量，flags，序号，行号
ZEND_API zend_attribute *zend_add_attribute(HashTable **attributes, zend_string *name, uint32_t argc, uint32_t flags, uint32_t offset, uint32_t lineno)
{
	// 添加永久属性标记
	bool persistent = flags & ZEND_ATTRIBUTE_PERSISTENT;
	// 如果哈希表无效
	if (*attributes == NULL) {
		// 分配内存创建哈希表
		*attributes = pemalloc(sizeof(HashTable), persistent);
		// 初始化哈希表
		zend_hash_init(*attributes, 8, NULL, attr_free, persistent);
	}
	// 创建包含有 argc 个参数的 zend_attribute
	zend_attribute *attr = pemalloc(ZEND_ATTRIBUTE_SIZE(argc), persistent);
	// 如果 name 是永久的
	if (persistent == ((GC_FLAGS(name) & IS_STR_PERSISTENT) != 0)) {
		// 使用copy, 只增加引用次数
		attr->name = zend_string_copy(name);
	// 否则使用dup
	} else {
		// 这个才是创建副本
		attr->name = zend_string_dup(name, persistent);
	}
	// 小写名称
	attr->lcname = zend_string_tolower_ex(attr->name, persistent);
	// 标记
	attr->flags = flags;
	// 行号
	attr->lineno = lineno;
	// 顺序号
	attr->offset = offset;
	// 包含参数数量 
	attr->argc = argc;
	// 初始化所有参数，避免部分初始化造成的致使错误
	/* Initialize arguments to avoid partial initialization in case of fatal errors. */
	for (uint32_t i = 0; i < argc; i++) {
		// 一开始没有名字
		attr->args[i].name = NULL;
		// 值为未定义
		ZVAL_UNDEF(&attr->args[i].value);
	}
	// 把属性插入到属性表里去
	zend_hash_next_index_insert_ptr(*attributes, attr);
	// 返回这个属性
	return attr;
}

// ing4, 释放内置属性
static void free_internal_attribute(zval *v)
{
	// 永久释放目标的内存
	pefree(Z_PTR_P(v), 1);
}

// ing3, 把指定的内部类，标记成内部属性
ZEND_API zend_internal_attribute *zend_mark_internal_attribute(zend_class_entry *ce)
{
	zend_internal_attribute *internal_attr;
	zend_attribute *attr;
	// 如果不是内置类
	if (ce->type != ZEND_INTERNAL_CLASS) {
		// 报错： 只有内置类可以注册成编译器属性
		zend_error_noreturn(E_ERROR, "Only internal classes can be registered as compiler attribute");
	}
	
	// 遍历类的修饰属性表
	ZEND_HASH_FOREACH_PTR(ce->attributes, attr) {
		// 找到属性名是 Attribute 的修饰属性
		if (zend_string_equals(attr->name, zend_ce_attribute->name)) {
			// 创建一个 内置修饰属性
			internal_attr = pemalloc(sizeof(zend_internal_attribute), 1);
			// 类入口指针
			internal_attr->ce = ce;
			// 添加标记， attr的第1个参数的 value
			internal_attr->flags = Z_LVAL(attr->args[0].value);
			// 无检证器
			internal_attr->validator = NULL;
			// 名字转成小写，永久字串
			zend_string *lcname = zend_string_tolower_ex(ce->name, 1);
			// 更新内置修饰属性表，把名称和属性更新进去
			zend_hash_update_ptr(&internal_attributes, lcname, internal_attr);
			// 释放小写名字
			zend_string_release(lcname);
			// 返回内置修饰属性
			return internal_attr;
		}
	} ZEND_HASH_FOREACH_END();
	
	// 如果类没有 名字是 Attribute 的修饰属性，
	// 报错： 类在注册成内置属性类前， 首标记成 修饰属性
	zend_error_noreturn(E_ERROR, "Classes must be first marked as attribute before being able to be registered as internal attribute class");
}

// ing3, 注册内部属性
ZEND_API zend_internal_attribute *zend_internal_attribute_register(zend_class_entry *ce, uint32_t flags)
{
	// 先给类添加修饰属性, Attribute, 1个参数
	zend_attribute *attr = zend_add_class_attribute(ce, zend_ce_attribute->name, 1);
	// flags 第入一个参数
	ZVAL_LONG(&attr->args[0].value, flags);
	// 创建内置修饰属性
	return zend_mark_internal_attribute(ce);
}

// ing4, 在内部属性表中按名称查找指定属性
ZEND_API zend_internal_attribute *zend_internal_attribute_get(zend_string *lcname)
{
	return zend_hash_find_ptr(&internal_attributes, lcname);
}

// ing3, 注册相关类
void zend_register_attribute_ce(void)
{
	zend_internal_attribute *attr;
	// 初始化哈希表 internal_attributes，长度8，持久，元素销毁方法 free_internal_attribute
	zend_hash_init(&internal_attributes, 8, NULL, free_internal_attribute, 1);

	// 注册类 Attribute
	zend_ce_attribute = register_class_Attribute();
	attr = zend_mark_internal_attribute(zend_ce_attribute);
	// 添加验证器
	attr->validator = validate_attribute;
	
	// 注册类 ReturnTypeWillChange
	zend_ce_return_type_will_change_attribute = register_class_ReturnTypeWillChange();
	zend_mark_internal_attribute(zend_ce_return_type_will_change_attribute);

	// 注册类 AllowDynamicProperties
	zend_ce_allow_dynamic_properties = register_class_AllowDynamicProperties();
	attr = zend_mark_internal_attribute(zend_ce_allow_dynamic_properties);
	// 添加验证器
	attr->validator = validate_allow_dynamic_properties;

	// 注册类 SensitiveParameter
	zend_ce_sensitive_parameter = register_class_SensitiveParameter();
	zend_mark_internal_attribute(zend_ce_sensitive_parameter);
	// attributes_object_handlers_sensitive_parameter_value = std_object_handlers
	memcpy(&attributes_object_handlers_sensitive_parameter_value, &std_object_handlers, sizeof(zend_object_handlers));
	
	// get_properties_for 函数 固定返回 NULL
	attributes_object_handlers_sensitive_parameter_value.get_properties_for = attributes_sensitive_parameter_value_get_properties_for;

	/* This is not an actual attribute, thus the zend_mark_internal_attribute() call is missing. */
	// 注册类 SensitiveParameterValue
	zend_ce_sensitive_parameter_value = register_class_SensitiveParameterValue();
	// 添加创建对象方法
	zend_ce_sensitive_parameter_value->create_object = attributes_sensitive_parameter_value_new;
}

// ing4, 关闭 修饰属性
void zend_attributes_shutdown(void)
{
	// 销毁哈希表 internal_attributes
	zend_hash_destroy(&internal_attributes);
}
