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
   | Authors: Ilija Tovilo <ilutov@php.net>                               |
   +----------------------------------------------------------------------+
*/

#include "zend.h"
#include "zend_API.h"
#include "zend_compile.h"
#include "zend_enum_arginfo.h"
#include "zend_interfaces.h"
#include "zend_enum.h"
#include "zend_extensions.h"
#include "zend_observer.h"

// ing4, 禁用魔术方法：方法名，魔术方法名
#define ZEND_ENUM_DISALLOW_MAGIC_METHOD(propertyName, methodName) \
	do { \
		/* 如果此方法存在 */ \
		if (ce->propertyName) { \
			/* 报错：不可以包含魔术方法：methodName */ \
			zend_error_noreturn(E_COMPILE_ERROR, "Enum %s cannot include magic method %s", ZSTR_VAL(ce->name), methodName); \
		} \
	} while (0);

ZEND_API zend_class_entry *zend_ce_unit_enum;
ZEND_API zend_class_entry *zend_ce_backed_enum;

static zend_object_handlers enum_handlers;

// ing3, 创建 enum 实例
zend_object *zend_enum_new(zval *result, zend_class_entry *ce, zend_string *case_name, zval *backing_value_zv)
{
	// 创建此类的实例
	zend_object *zobj = zend_objects_new(ce);
	// 实例关联到返回结果上
	ZVAL_OBJ(result, zobj);

	//  通过索引号获取指定的类属性，并把 case_name 赋值给它
	ZVAL_STR_COPY(OBJ_PROP_NUM(zobj, 0), case_name);
	
	// 如果有 backing_value_zv
	if (backing_value_zv != NULL) {
		// 通过索引号获取指定的类属性，并把 backing_value_zv 赋值给它
		ZVAL_COPY(OBJ_PROP_NUM(zobj, 1), backing_value_zv);
	}
	// 此实例的相关处理方法
	zobj->handlers = &enum_handlers;

	return zobj;
}

// ing3, 验证 enum 的 porperty 。只能是 name,value
static void zend_verify_enum_properties(zend_class_entry *ce)
{
	zend_property_info *property_info;
	// 遍历每一个 property_info
	ZEND_HASH_MAP_FOREACH_PTR(&ce->properties_info, property_info) {
		// 如果属性名是 name ，下一个
		if (zend_string_equals_literal(property_info->name, "name")) {
			continue;
		}
		if (
			// 如果 备用类型 不是 未定义，并且 属性名是 value
			ce->enum_backing_type != IS_UNDEF
			&& zend_string_equals_literal(property_info->name, "value")
		) {
			// 下一个
			continue;
		}
		// 如果属性名不是 name 和 value
		// FIXME: File/line number for traits?
		// 报错 enum 不可以包含此属性名 
		zend_error_noreturn(E_COMPILE_ERROR, "Enum %s cannot include properties",
			ZSTR_VAL(ce->name));
	} ZEND_HASH_FOREACH_END();
}

// ing4, enum的魔术方法验证
static void zend_verify_enum_magic_methods(zend_class_entry *ce)
{
	// 只有 __get,__call,__invoke 可用
	// Only __get, __call and __invoke are allowed
	
	// 禁用以下魔术方法，当编译时遇到它们，就报错
	ZEND_ENUM_DISALLOW_MAGIC_METHOD(constructor, "__construct");
	ZEND_ENUM_DISALLOW_MAGIC_METHOD(destructor, "__destruct");
	ZEND_ENUM_DISALLOW_MAGIC_METHOD(clone, "__clone");
	ZEND_ENUM_DISALLOW_MAGIC_METHOD(__get, "__get");
	ZEND_ENUM_DISALLOW_MAGIC_METHOD(__set, "__set");
	ZEND_ENUM_DISALLOW_MAGIC_METHOD(__unset, "__unset");
	ZEND_ENUM_DISALLOW_MAGIC_METHOD(__isset, "__isset");
	ZEND_ENUM_DISALLOW_MAGIC_METHOD(__tostring, "__toString");
	ZEND_ENUM_DISALLOW_MAGIC_METHOD(__debugInfo, "__debugInfo");
	ZEND_ENUM_DISALLOW_MAGIC_METHOD(__serialize, "__serialize");
	ZEND_ENUM_DISALLOW_MAGIC_METHOD(__unserialize, "__unserialize");

	// 禁用方法
	const char *forbidden_methods[] = {
		"__sleep",
		"__wakeup",
		"__set_state",
	};
	// 禁用方法数量
	uint32_t forbidden_methods_length = sizeof(forbidden_methods) / sizeof(forbidden_methods[0]);
	
	// 遍历每一个
	for (uint32_t i = 0; i < forbidden_methods_length; ++i) {
		// 取得方法名
		const char *forbidden_method = forbidden_methods[i];
		// 如果在方法列表里有它
		if (zend_hash_str_exists(&ce->function_table, forbidden_method, strlen(forbidden_method))) {
			// 报错，不可以使用此魔术方法
			zend_error_noreturn(E_COMPILE_ERROR, "Enum %s cannot include magic method %s", ZSTR_VAL(ce->name), forbidden_method);
		}
	}
}

// ing3, 检验 enum 接口, 不可以实现可序列化接口
static void zend_verify_enum_interfaces(zend_class_entry *ce)
{
	// 如果类实现了 可序列化接口
	if (zend_class_implements_interface(ce, zend_ce_serializable)) {
		// 报错，不可以实现可序列化接口
		zend_error_noreturn(E_COMPILE_ERROR,
			"Enum %s cannot implement the Serializable interface", ZSTR_VAL(ce->name));
	}
}

// ing4, 检验 enum （属性，魔术方法，接口）
void zend_verify_enum(zend_class_entry *ce)
{
	// 检验属性
	zend_verify_enum_properties(ce);
	// 检验魔术方法
	zend_verify_enum_magic_methods(ce);
	// 检验接口
	zend_verify_enum_interfaces(ce);
}

// ing4, 验证，只有 enum 类可以实现此接口
static int zend_implement_unit_enum(zend_class_entry *interface, zend_class_entry *class_type)
{
	// 如果类有 enum 标记
	if (class_type->ce_flags & ZEND_ACC_ENUM) {
		// 返回成功
		return SUCCESS;
	}
	// 报错 非 enum 类不可以实现 此接口
	zend_error_noreturn(E_ERROR, "Non-enum class %s cannot implement interface %s",
		ZSTR_VAL(class_type->name),
		ZSTR_VAL(interface->name));

	// 返回失败
	return FAILURE;
}

// ing4, 验证，只有 有备用类型的 enum 类可以实现此接口
static int zend_implement_backed_enum(zend_class_entry *interface, zend_class_entry *class_type)
{
	// 如果类没有 enum 标记
	if (!(class_type->ce_flags & ZEND_ACC_ENUM)) {
		// 报错 非 enum 类不可以实现 此接口
		zend_error_noreturn(E_ERROR, "Non-enum class %s cannot implement interface %s",
			ZSTR_VAL(class_type->name),
			ZSTR_VAL(interface->name));
		// 返回失败
		return FAILURE;
	}

	// 如果备用类型是 未定义
	if (class_type->enum_backing_type == IS_UNDEF) {
		// 无后备类型的 enum 不可以继承此接口
		zend_error_noreturn(E_ERROR, "Non-backed enum %s cannot implement interface %s",
			ZSTR_VAL(class_type->name),
			ZSTR_VAL(interface->name));
		return FAILURE;
	}

	return SUCCESS;
}

// ing4, 注册enum相关类
void zend_register_enum_ce(void)
{
	// 注册类 UnitEnum接口
	zend_ce_unit_enum = register_class_UnitEnum();
	// 实现此接口时的验证方法：只有enum类可以实现此接口
	zend_ce_unit_enum->interface_gets_implemented = zend_implement_unit_enum;

	// 注册 BackedEnum 接口。有备用类型的 enum
	zend_ce_backed_enum = register_class_BackedEnum(zend_ce_unit_enum);
	// 实现此接口时的验证方法：只有enum类可以实现此接口，并且 要有备用类型
	zend_ce_backed_enum->interface_gets_implemented = zend_implement_backed_enum;

	// enum 处理方法与 std_object_handlers 差不多，先直接复制过来。
	memcpy(&enum_handlers, &std_object_handlers, sizeof(zend_object_handlers));
	// 克隆方法为空
	enum_handlers.clone_obj = NULL;
	// 不可比较，返回 ZEND_UNCOMPARABLE
	enum_handlers.compare = zend_objects_not_comparable;
}

// ing4, 给类添加接口
void zend_enum_add_interfaces(zend_class_entry *ce)
{
	// 已有接口数量 
	uint32_t num_interfaces_before = ce->num_interfaces;
	// 接口数量 +1
	ce->num_interfaces++;
	// 如果有备用类型
	if (ce->enum_backing_type != IS_UNDEF) {
		// 接口数量 +1
		ce->num_interfaces++;
	}
	// 不可以有 ZEND_ACC_RESOLVED_INTERFACES 标记
	ZEND_ASSERT(!(ce->ce_flags & ZEND_ACC_RESOLVED_INTERFACES));
	// 扩展接口名 列表，添加新接口名
	ce->interface_names = erealloc(ce->interface_names, sizeof(zend_class_name) * ce->num_interfaces);
	// 新接口名：名称（UnitEnum接口）
	ce->interface_names[num_interfaces_before].name = zend_string_copy(zend_ce_unit_enum->name);
	// 新接口名：小写名称固定 unitenum
	ce->interface_names[num_interfaces_before].lc_name = zend_string_init("unitenum", sizeof("unitenum") - 1, 0);
	// 如果有备用类型
	if (ce->enum_backing_type != IS_UNDEF) {
		// 新接口名：名称（BackedEnum接口）
		ce->interface_names[num_interfaces_before + 1].name = zend_string_copy(zend_ce_backed_enum->name);
		// 新接口名：小写名称固定 backedenum
		ce->interface_names[num_interfaces_before + 1].lc_name = zend_string_init("backedenum", sizeof("backedenum") - 1, 0);	
	}
}

// ing4, 给类添加备用 enum 表（case值表）
zend_result zend_enum_build_backed_enum_table(zend_class_entry *ce)
{
	// 必须有 enum 标记
	ZEND_ASSERT(ce->ce_flags & ZEND_ACC_ENUM);
	// 类型必须是 用户类
	ZEND_ASSERT(ce->type == ZEND_USER_CLASS);
	// 备用类型
	uint32_t backing_type = ce->enum_backing_type;
	// 备用类型不可以空
	ZEND_ASSERT(backing_type != IS_UNDEF);
	// 备用 enum 表（哈希表）
	HashTable *backed_enum_table = emalloc(sizeof(HashTable));
	// 初始化哈希表
	zend_hash_init(backed_enum_table, 0, NULL, ZVAL_PTR_DTOR, 0);
	// 如果类入口有可转换数据，把备用表关联到可转换数据上。否则，把备用表关联到类入口上。
	zend_class_set_backed_enum_table(ce, backed_enum_table);

	zend_string *enum_class_name = ce->name;

	zend_string *name;
	zval *val;
	// 遍历常量列表
	ZEND_HASH_MAP_FOREACH_STR_KEY_VAL(CE_CONSTANTS_TABLE(ce), name, val) {
		zend_class_constant *c = Z_PTR_P(val);
		// 如果有 case 标记
		if ((ZEND_CLASS_CONST_FLAGS(c) & ZEND_CLASS_CONST_IS_CASE) == 0) {
			// 下一个
			continue;
		}

		zval *c_value = &c->value;
		// 获取 case 名
		zval *case_name = zend_enum_fetch_case_name(Z_OBJ_P(c_value));
		// 获取 case 值
		zval *case_value = zend_enum_fetch_case_value(Z_OBJ_P(c_value));
		// 如果备用类型 和当前 case 类型不同
		if (ce->enum_backing_type != Z_TYPE_P(case_value)) {
			// 报错 enum 类型与备用类型不符
			zend_type_error("Enum case type %s does not match enum backing type %s",
				zend_get_type_by_const(Z_TYPE_P(case_value)),
				zend_get_type_by_const(ce->enum_backing_type));
			// 返回失败
			goto failure;
		}
		// 如果备用类型是整数 （只支持整数 ，字串 两种类型）
		if (ce->enum_backing_type == IS_LONG) {
			// 获取case的整数 值
			zend_long long_key = Z_LVAL_P(case_value);
			// 查看case是否已经存在
			zval *existing_case_name = zend_hash_index_find(backed_enum_table, long_key);
			// 如果已经存在
			if (existing_case_name) {
				// 报错，重复的case值
				zend_throw_error(NULL, "Duplicate value in enum %s for cases %s and %s",
					ZSTR_VAL(enum_class_name),
					Z_STRVAL_P(existing_case_name),
					ZSTR_VAL(name));
				// 返回：失败
				goto failure;
			}
			// 如果没有重复值
			// 把 case 添加进去
			zend_hash_index_add_new(backed_enum_table, long_key, case_name);
		// 备用类型不是整数
		} else {
			// 备用类型必须是string
			ZEND_ASSERT(ce->enum_backing_type == IS_STRING);
			// 值也是key
			zend_string *string_key = Z_STR_P(case_value);
			// 获取重复项
			zval *existing_case_name = zend_hash_find(backed_enum_table, string_key);
			// 如果获取到，报错
			if (existing_case_name != NULL) {
				zend_throw_error(NULL, "Duplicate value in enum %s for cases %s and %s",
					ZSTR_VAL(enum_class_name),
					Z_STRVAL_P(existing_case_name),
					ZSTR_VAL(name));
				goto failure;
			}
			// 没有重复项，把case添加到备用列表中
			zend_hash_add_new(backed_enum_table, string_key, case_name);
		}
	} ZEND_HASH_FOREACH_END();
	// 返回成功
	return SUCCESS;

failure:
	// 释放备用列表
	zend_hash_release(backed_enum_table);
	// 把类的备用 enum列表设置成空
	zend_class_set_backed_enum_table(ce, NULL);
	// 返回失败
	return FAILURE;
}

// ing4, 方法，取回enum里的所有 case 
// void ZEND_FASTCALL zend_enum_cases_func(zend_execute_data *execute_data, zval *return_value)
static 	
{
	// 从操作上下文中获取
	zend_class_entry *ce = execute_data->func->common.scope;
	zend_class_constant *c;
	// 此方法必须0个参数
	ZEND_PARSE_PARAMETERS_NONE();
	// 初始化返回值
	array_init(return_value);
	// 遍历类常量表
	ZEND_HASH_MAP_FOREACH_PTR(CE_CONSTANTS_TABLE(ce), c) {
		// 如果此常量是 case 
		if (!(ZEND_CLASS_CONST_FLAGS(c) & ZEND_CLASS_CONST_IS_CASE)) {
			// 下一个
			continue;
		}
		// 如果不是case
		zval *zv = &c->value;
		// 如果是常量表达式
		if (Z_TYPE_P(zv) == IS_CONSTANT_AST) {
			// 如果更新常量失败
			if (zval_update_constant_ex(zv, c->ce) == FAILURE) {
				// 抛错
				RETURN_THROWS();
			}
		}
		// 增加引用次数
		Z_ADDREF_P(zv);
		// 返回值表增加新元素
		zend_hash_next_index_insert_new(Z_ARRVAL_P(return_value), zv);
	} ZEND_HASH_FOREACH_END();
}

// ing3, 根据key和备用类型信息，获取 case 值
ZEND_API zend_result zend_enum_get_case_by_value(zend_object **result, zend_class_entry *ce, zend_long long_key, zend_string *string_key, bool try)
{
	// 如果是用户定义类，并且 未更新常量
	if (ce->type == ZEND_USER_CLASS && !(ce->ce_flags & ZEND_ACC_CONSTANTS_UPDATED)) {
		// 更新常量
		if (zend_update_class_constants(ce) == FAILURE) {
			return FAILURE;
		}
	}
	// 返回 ce->mutable_data 或者 ce本身的 enum备用表
	HashTable *backed_enum_table = CE_BACKED_ENUM_TABLE(ce);
	// 如果没有 
	if (!backed_enum_table) {
		goto not_found;
	}
	// 如果找到
	
	zval *case_name_zv;
	// 如果备用类型是整数 
	if (ce->enum_backing_type == IS_LONG) {
		// 取得name
		case_name_zv = zend_hash_index_find(backed_enum_table, long_key);
	// 备用类型是 字串
	} else {
		// 备用类型必须是字串
		ZEND_ASSERT(ce->enum_backing_type == IS_STRING);
		// 必须存在
		ZEND_ASSERT(string_key != NULL);
		// 取出 name
		case_name_zv = zend_hash_find(backed_enum_table, string_key);
	}
	// 如果找不到name
	if (case_name_zv == NULL) {
not_found:
		// 如果用了try
		if (try) {
			// 结果为空
			*result = NULL;
			// 返回成功
			return SUCCESS;
		}
		// 没有用try
		// 备用类型是整数 
		if (ce->enum_backing_type == IS_LONG) {
			// 报错 不可用的备用类型
			zend_value_error(ZEND_LONG_FMT " is not a valid backing value for enum %s", long_key, ZSTR_VAL(ce->name));
		// 备用类型是 string
		} else {
			ZEND_ASSERT(ce->enum_backing_type == IS_STRING);
			// 报错 不可用的备用类型
			zend_value_error("\"%s\" is not a valid backing value for enum %s", ZSTR_VAL(string_key), ZSTR_VAL(ce->name));
		}
		// 返回失败
		return FAILURE;
	}

	// TODO: We might want to store pointers to constants in backed_enum_table instead of names,
	// to make this lookup more efficient.
	// name必须是字串
	ZEND_ASSERT(Z_TYPE_P(case_name_zv) == IS_STRING);
	// 用name获取类常量
	zend_class_constant *c = zend_hash_find_ptr(CE_CONSTANTS_TABLE(ce), Z_STR_P(case_name_zv));
	// 常量必须存在
	ZEND_ASSERT(c != NULL);
	// 获取值
	zval *case_zv = &c->value;
	// 如果是常量表达式
	if (Z_TYPE_P(case_zv) == IS_CONSTANT_AST) {
		// 更新常量
		if (zval_update_constant_ex(case_zv, c->ce) == FAILURE) {
			// 失败则返回 失败
			return FAILURE;
		}
	}
	// 值关联到结果
	*result = Z_OBJ_P(case_zv);
	// 返回成功
	return SUCCESS;
}

// ing3, 根据执行上下文中的 参数，从menu里取出 case 并返回。
// static void zend_enum_from_base(zend_execute_data *execute_data, zval *return_value, bool try)
static void zend_enum_from_base(INTERNAL_FUNCTION_PARAMETERS, bool try)
{
	// 执行上下文中的作用域
	zend_class_entry *ce = execute_data->func->common.scope;
	bool release_string = false;
	zend_string *string_key = NULL;
	zend_long long_key = 0;
	// 如果备用类型是 整数 
	if (ce->enum_backing_type == IS_LONG) {
		// 验证参数数量是否合法并取出参数列表, 传入：最小数量，最大数量 
		ZEND_PARSE_PARAMETERS_START(1, 1)
			// 获取第一个参数，值为整数
			Z_PARAM_LONG(long_key)
		// 获取参数的 最后验证，出错时 return返回。
		ZEND_PARSE_PARAMETERS_END();
	} else {
		ZEND_ASSERT(ce->enum_backing_type == IS_STRING);
		// 前一个执行数据所属函数 的参数使用严格类型限制
		if (ZEND_ARG_USES_STRICT_TYPES()) {
			// 验证参数数量是否合法并取出参数列表, 传入：最小数量，最大数量 
			ZEND_PARSE_PARAMETERS_START(1, 1)
				// 获取第一个参数，值为字串
				Z_PARAM_STR(string_key)
			// 获取参数的 最后验证，出错时 return返回。
			ZEND_PARSE_PARAMETERS_END();
		// 没有使用严格类型限制 
		} else {
			// coercion 强迫。这里允许整数key所以不会 强转成string。 
			// We allow long keys so that coercion to string doesn't happen implicitly. The JIT
			// skips deallocation of params that don't require it. In the case of from/tryFrom
			// passing int to from(int|string) looks like no coercion will happen, so the JIT
			// won't emit a dtor call. Thus we allocate/free the string manually.
			
			// 验证参数数量是否合法并取出参数列表, 传入：最小数量，最大数量 
			ZEND_PARSE_PARAMETERS_START(1, 1)
				// 获取第一个参数，值为字串 或 整数 
				Z_PARAM_STR_OR_LONG(string_key, long_key)
			// 获取参数的 最后验证，出错时 return返回。
			ZEND_PARSE_PARAMETERS_END();
			// 如果 key 是空
			if (string_key == NULL) {
				// key 为 true
				release_string = true;
				// 转成字串
				string_key = zend_long_to_str(long_key);
			}
		}
	}

	zend_object *case_obj;
	// 根据key和备用类型信息，获取 case 值
	if (zend_enum_get_case_by_value(&case_obj, ce, long_key, string_key, try) == FAILURE) {
		// 失败则抛错
		goto throw;
	}
	// 如果获取失败
	if (case_obj == NULL) {
		// 必须有 try
		ZEND_ASSERT(try);
		// 返回null
		goto return_null;
	}

	// 如果需要释放key
	if (release_string) {
		zend_string_release(string_key);
	}
	// 把 case_obj 的对象 copy 给 返回结果 并 return 
	RETURN_OBJ_COPY(case_obj);

throw:
	// 如果需要释放key
	if (release_string) {
		// 释放 key
		zend_string_release(string_key);
	}
	// 抛错并返回
	RETURN_THROWS();

return_null:
	// 如果需要释放key
	if (release_string) {
		// 释放 key
		zend_string_release(string_key);
	}
	// 返回 null
	RETURN_NULL();
}

static ZEND_NAMED_FUNCTION(zend_enum_from_func)
{
	zend_enum_from_base(INTERNAL_FUNCTION_PARAM_PASSTHRU, 0);
}

static ZEND_NAMED_FUNCTION(zend_enum_try_from_func)
{
	zend_enum_from_base(INTERNAL_FUNCTION_PARAM_PASSTHRU, 1);
}

// ing3, 给类添加方法
static void zend_enum_register_func(zend_class_entry *ce, zend_known_string_id name_id, zend_internal_function *zif) {
	zend_string *name = ZSTR_KNOWN(name_id);
	// 类型为内置函数 
	zif->type = ZEND_INTERNAL_FUNCTION;
	// 所属模块:当前模块
	zif->module = EG(current_module);
	// 所属域：当前类
	zif->scope = ce;
	// 临时变量数量 0 或 1
	zif->T = ZEND_OBSERVER_ENABLED;
	// 如果正在执行
    if (EG(active)) { // at run-time
		// 创建指针 zif->run_time_cache__ptr，并分配新的内存空间
		// zend_op_array_extension_handles * sizeof(void *)
		ZEND_MAP_PTR_INIT(zif->run_time_cache, zend_arena_calloc(&CG(arena), 1, zend_internal_run_time_cache_reserved_size()));
	// 如果没在执行
	} else {
		// 创建指针 zif->run_time_cache__ptr
		ZEND_MAP_PTR_NEW(zif->run_time_cache);
	}
	// 添加到方法表
	if (!zend_hash_add_ptr(&ce->function_table, name, zif)) {
		// 报错：方法已存在
		zend_error_noreturn(E_COMPILE_ERROR, "Cannot redeclare %s::%s()", ZSTR_VAL(ce->name), ZSTR_VAL(name));
	}
}

// ing4, 注册 cases 方法，如果有备用类型，再注册 from ,tryFrom 两个方法
void zend_enum_register_funcs(zend_class_entry *ce)
{
	// 默认标记：public, static，有返回类型，通过 arena 分配
	const uint32_t fn_flags =
		ZEND_ACC_PUBLIC|ZEND_ACC_STATIC|ZEND_ACC_HAS_RETURN_TYPE|ZEND_ACC_ARENA_ALLOCATED;
	// 分配内存创建永久内置函数
	zend_internal_function *cases_function = zend_arena_calloc(&CG(arena), sizeof(zend_internal_function), 1);
	// 处理方法 zend_enum_cases_func
	cases_function->handler = zend_enum_cases_func;
	// 函数名 cases
	cases_function->function_name = ZSTR_KNOWN(ZEND_STR_CASES);
	// 添加标记
	cases_function->fn_flags = fn_flags;
	// 参数信息
	cases_function->arg_info = (zend_internal_arg_info *) (arginfo_class_UnitEnum_cases + 1);
	// 给类添加 cass 方法
	zend_enum_register_func(ce, ZEND_STR_CASES, cases_function);
	// 如果有备用类型
	if (ce->enum_backing_type != IS_UNDEF) {
		// 创建内置方法
		zend_internal_function *from_function = zend_arena_calloc(&CG(arena), sizeof(zend_internal_function), 1);
		// 对应的处理函数 
		from_function->handler = zend_enum_from_func;
		// 方法名 from
		from_function->function_name = ZSTR_KNOWN(ZEND_STR_FROM);
		// 标记
		from_function->fn_flags = fn_flags;
		// 参数数量1
		from_function->num_args = 1;
		// 必须参数，1个
		from_function->required_num_args = 1;
		// 参数信息 
		from_function->arg_info = (zend_internal_arg_info *) (arginfo_class_BackedEnum_from + 1);
		// 注册 from 方法到类上
		zend_enum_register_func(ce, ZEND_STR_FROM, from_function);
		
		// 创建内置方法 2
		zend_internal_function *try_from_function = zend_arena_calloc(&CG(arena), sizeof(zend_internal_function), 1);
		// 处理函数 zend_enum_try_from_func
		try_from_function->handler = zend_enum_try_from_func;
		// 函数名 tryFrom
		try_from_function->function_name = ZSTR_KNOWN(ZEND_STR_TRYFROM);
		// 添加flag
		try_from_function->fn_flags = fn_flags;
		// 参数数 1个
		try_from_function->num_args = 1;
		// 必须参数数 1个
		try_from_function->required_num_args = 1;
		// 参数信息
		try_from_function->arg_info = (zend_internal_arg_info *) (arginfo_class_BackedEnum_tryFrom + 1);
		// 注册 tryFrom 方法到类上
		zend_enum_register_func(ce, ZEND_STR_TRYFROM_LOWERCASE, try_from_function);
	}
}

// ing3，给enum添加 默认值 0
void zend_enum_register_props(zend_class_entry *ce)
{
	// 添加标记 无动态属性
	ce->ce_flags |= ZEND_ACC_NO_DYNAMIC_PROPERTIES;

	zval name_default_value;
	// 设置成未定义
	ZVAL_UNDEF(&name_default_value);
	// 类型为字串，不可以是null，无 extra_flags
	zend_type name_type = ZEND_TYPE_INIT_CODE(IS_STRING, 0, 0);
	// 声名有类型的类属性，public, readonly
	zend_declare_typed_property(ce, ZSTR_KNOWN(ZEND_STR_NAME), &name_default_value, ZEND_ACC_PUBLIC | ZEND_ACC_READONLY, NULL, name_type);
	// 如果备用类型不是空
	if (ce->enum_backing_type != IS_UNDEF) {
		// 默认值
		zval value_default_value;
		// 设置为未定义
		ZVAL_UNDEF(&value_default_value);
		// 类型为 ce->enum_backing_type ，不可以是null，无 extra_flags
		zend_type value_type = ZEND_TYPE_INIT_CODE(ce->enum_backing_type, 0, 0);
		// 声名有类型的类属性 public, readonly
		zend_declare_typed_property(ce, ZSTR_KNOWN(ZEND_STR_VALUE), &value_default_value, ZEND_ACC_PUBLIC | ZEND_ACC_READONLY, NULL, value_type);
	}
}

// unitEnum 类添加方法 
static const zend_function_entry unit_enum_methods[] = {
	// cases 方法，对应函数  zend_enum_cases_func ，
	ZEND_NAMED_ME(cases, zend_enum_cases_func, arginfo_class_UnitEnum_cases, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	ZEND_FE_END
};

// backedEnum 类添加方法
static const zend_function_entry backed_enum_methods[] = {
	ZEND_NAMED_ME(cases, zend_enum_cases_func, arginfo_class_UnitEnum_cases, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	ZEND_NAMED_ME(from, zend_enum_from_func, arginfo_class_BackedEnum_from, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	ZEND_NAMED_ME(tryFrom, zend_enum_try_from_func, arginfo_class_BackedEnum_tryFrom, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	ZEND_FE_END
};

// ing3, 注册内置enum类
ZEND_API zend_class_entry *zend_register_internal_enum(
	const char *name, zend_uchar type, const zend_function_entry *functions)
{
	// 类型为未定义 或 整数 或 字串
	ZEND_ASSERT(type == IS_UNDEF || type == IS_LONG || type == IS_STRING);
	// 
	zend_class_entry tmp_ce;
	// 用给定的名字创建新类，并添加成员方法列表
	INIT_CLASS_ENTRY_EX(tmp_ce, name, strlen(name), functions);
	// 注册内置内置类
	zend_class_entry *ce = zend_register_internal_class(&tmp_ce);
	// 添加enum标记
	ce->ce_flags |= ZEND_ACC_ENUM;
	// 备用类型
	ce->enum_backing_type = type;
	// 如果有备用类型
	if (type != IS_UNDEF) {
		// 创建新哈希表，enum表
		HashTable *backed_enum_table = pemalloc(sizeof(HashTable), 1);
		// 初始化 enum表
		zend_hash_init(backed_enum_table, 0, NULL, ZVAL_PTR_DTOR, 1);
		// 把 enum表 关联到类上
		zend_class_set_backed_enum_table(ce, backed_enum_table);
	}
	// 给enum添加 默认值 0
	zend_enum_register_props(ce);
	// 如果类型是未定义
	if (type == IS_UNDEF) {
		// 添加方法 unit_enum_methods
		zend_register_functions(
			ce, unit_enum_methods, &ce->function_table, EG(current_module)->type);
		// 实现 unitEnum 接口
		zend_class_implements(ce, 1, zend_ce_unit_enum);
	// 如果类型不是未定义，添加带备用类型的处理方法
	} else {
		// 添加方法 backed_enum_methods
		zend_register_functions(
			ce, backed_enum_methods, &ce->function_table, EG(current_module)->type);
		// 实现 backedEnum 接口
		zend_class_implements(ce, 1, zend_ce_backed_enum);
	}

	return ce;
}

// 创建enum case语句实例（应该是反射用）
static zend_ast_ref *create_enum_case_ast(
		zend_string *class_name, zend_string *case_name, zval *value) {
	// TODO: Use custom node type for enum cases?
	// 总大小是 zend_ast_ref + （zend_ast - (zend_ast *) + (zend_ast *) * 3） + 2或3个 zend_ast_zval
	// 8+16-8+8*3+24*(2或3)
	// 88或112
	size_t size = sizeof(zend_ast_ref) + zend_ast_size(3)
		+ (value ? 3 : 2) * sizeof(zend_ast_zval);
	// 分配内存创建语句树
	char *p = pemalloc(size, 1);
	// ref是缓冲区开头位置，zend_ast_ref 实例在最前面。p向后移一个 zend_ast_ref 
	zend_ast_ref *ref = (zend_ast_ref *) p; p += sizeof(zend_ast_ref);
	// 引用次数为1
	GC_SET_REFCOUNT(ref, 1);
	// 类型为: 常量表达式，持久，不变
	GC_TYPE_INFO(ref) = GC_CONSTANT_AST | GC_PERSISTENT | GC_IMMUTABLE;
	// 主语句指针。后移3个？
	zend_ast *ast = (zend_ast *) p; p += zend_ast_size(3);
	// 语句类型为 初始化enum
	ast->kind = ZEND_AST_CONST_ENUM_INIT;
	// 附加属性 0
	ast->attr = 0;
	// 行号0
	ast->lineno = 0;
	// 第一个子语句
	ast->child[0] = (zend_ast *) p; p += sizeof(zend_ast_zval);
	ast->child[0]->kind = ZEND_AST_ZVAL;
	ast->child[0]->attr = 0;
	ZEND_ASSERT(ZSTR_IS_INTERNED(class_name));
	ZVAL_STR(zend_ast_get_zval(ast->child[0]), class_name);
	// 第二个子语句
	ast->child[1] = (zend_ast *) p; p += sizeof(zend_ast_zval);
	ast->child[1]->kind = ZEND_AST_ZVAL;
	ast->child[1]->attr = 0;
	ZEND_ASSERT(ZSTR_IS_INTERNED(case_name));
	ZVAL_STR(zend_ast_get_zval(ast->child[1]), case_name);
	// 第三个子语句
	if (value) {
		ast->child[2] = (zend_ast *) p; p += sizeof(zend_ast_zval);
		ast->child[2]->kind = ZEND_AST_ZVAL;
		ast->child[2]->attr = 0;
		ZEND_ASSERT(!Z_REFCOUNTED_P(value));
		ZVAL_COPY_VALUE(zend_ast_get_zval(ast->child[2]), value);
	} else {
		ast->child[2] = NULL;
	}

	return ref;
}

// ing3, 添加 case （zend_string）
ZEND_API void zend_enum_add_case(zend_class_entry *ce, zend_string *case_name, zval *value)
{
	// 如果值有效
	if (value) {
		// 值的类型 必须和备用类型相符
		ZEND_ASSERT(ce->enum_backing_type == Z_TYPE_P(value));
		// 如果值是 string 并且不是 保留字
		if (Z_TYPE_P(value) == IS_STRING && !ZSTR_IS_INTERNED(Z_STR_P(value))) {
			// 添加保留字。enum的case 字串都要添加保留字
			zval_make_interned_string(value);
		}
		// 返回 ce->mutable_data 或者 ce本身的 enum备用表
		HashTable *backed_enum_table = CE_BACKED_ENUM_TABLE(ce);

		zval case_name_zv;
		// 复制case名
		ZVAL_STR(&case_name_zv, case_name);
		// 如果值是 整数 
		if (Z_TYPE_P(value) == IS_LONG) {
			// 顺序数据中添加 新元素
			zend_hash_index_add_new(backed_enum_table, Z_LVAL_P(value), &case_name_zv);
		// 值不是整数 
		} else {
			// 哈希表中添加新元素
			zend_hash_add_new(backed_enum_table, Z_STR_P(value), &case_name_zv);
		}
	// 否则
	} else {
		// 备用类型必须未定义
		ZEND_ASSERT(ce->enum_backing_type == IS_UNDEF);
	}

	zval ast_zv;
	// 类型为常量表达式
	Z_TYPE_INFO(ast_zv) = IS_CONSTANT_AST;
	// 创建enum语句
	Z_AST(ast_zv) = create_enum_case_ast(ce->name, case_name, value);
	// 声名类常量 名称 case_name, 值 ast_zv ,public
	zend_class_constant *c = zend_declare_class_constant_ex(
		ce, case_name, &ast_zv, ZEND_ACC_PUBLIC, NULL);
	// 常量添加case 标记
	ZEND_CLASS_CONST_FLAGS(c) |= ZEND_CLASS_CONST_IS_CASE;
}

// ing3, 添加 case （char *）
ZEND_API void zend_enum_add_case_cstr(zend_class_entry *ce, const char *name, zval *value)
{
	// 创建 zend_string
	zend_string *name_str = zend_string_init_interned(name, strlen(name), 1);
	// 添加 case （zend_string）
	zend_enum_add_case(ce, name_str, value);
	// 释放string
	zend_string_release(name_str);
}

// ing3, 根据 case 名称(zend_string)获取 case 的值
ZEND_API zend_object *zend_enum_get_case(zend_class_entry *ce, zend_string *name) {
	// 类常量表里找常量名
	zend_class_constant *c = zend_hash_find_ptr(CE_CONSTANTS_TABLE(ce), name);
	// 找到的必须是 case
	ZEND_ASSERT(ZEND_CLASS_CONST_FLAGS(c) & ZEND_CLASS_CONST_IS_CASE);
	// 如果是常量表达式
	if (Z_TYPE(c->value) == IS_CONSTANT_AST) {
		// 更新常量
		if (zval_update_constant_ex(&c->value, c->ce) == FAILURE) {
			// 失败则报错
			ZEND_UNREACHABLE();
		}
	}
	// 值必须是对象
	ZEND_ASSERT(Z_TYPE(c->value) == IS_OBJECT);
	// 返回常量值
	return Z_OBJ(c->value);
}

// ing4, 根据 case 名称（char *）获取 case 的值
ZEND_API zend_object *zend_enum_get_case_cstr(zend_class_entry *ce, const char *name) {
	// 创建名称字串
	zend_string *name_str = zend_string_init(name, strlen(name), 0);
	// 根据名称获取case的值
	zend_object *result = zend_enum_get_case(ce, name_str);
	// 释放名称
	zend_string_release(name_str);
	return result;
}
