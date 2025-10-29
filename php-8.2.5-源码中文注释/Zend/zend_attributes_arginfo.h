/* This is a generated file, edit the .stub.php file instead.
 * Stub hash: afb6a3f1d14099066d028b1579fff074359da293 */

// 创建参数信息列表 ，增加第一个参数（返回值）。
// 传入：变量名，_unused？，是否引用返回，必要的参数数
// ing3, __construct 方法，没有 _unused，不可引用返回，无必要参数
ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Attribute___construct, 0, 0, 0)
	// ing3, 有默认值的 参数信息：可引用传递，参数名称，类型，可以是null, 默认值
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, flags, IS_LONG, 0, "Attribute::TARGET_ALL")
	// 这个 Attribute::TARGET_ALL 在C语言中如何运行 ？
ZEND_END_ARG_INFO()

// 创建参数信息列表 ，增加第一个参数（返回值）。
// 传入：变量名，_unused？，是否引用返回，必要的参数数
// ing3, __construct 方法，没有 _unused，不可引用返回，无必要参数
ZEND_BEGIN_ARG_INFO_EX(arginfo_class_ReturnTypeWillChange___construct, 0, 0, 0)
ZEND_END_ARG_INFO()

// AllowDynamicProperties 与 ReturnTypeWillChange 的构造方法参数相同
#define arginfo_class_AllowDynamicProperties___construct arginfo_class_ReturnTypeWillChange___construct

// SensitiveParameter 与 ReturnTypeWillChange 的构造方法参数相同
#define arginfo_class_SensitiveParameter___construct arginfo_class_ReturnTypeWillChange___construct

// 创建参数信息列表 ，增加第一个参数（返回值）。
// 传入：变量名，_unused？，是否引用返回，必要的参数数
// ing3, __construct 方法，没有 _unused，不可引用返回，无必要参数
ZEND_BEGIN_ARG_INFO_EX(arginfo_class_SensitiveParameterValue___construct, 0, 0, 1)
	// ing3, 无默认值的 参数信息：可引用传递，参数名称，类型，可以是null
	ZEND_ARG_TYPE_INFO(0, value, IS_MIXED, 0)
ZEND_END_ARG_INFO()

// 创建 参数信息列表 ，增加第一个 返回类型不允null 的 参数（返回值）。
// 传入：变量名，是否引用返回，必要的参数数，类型，是否允null
// ing3, getValue 方法：不可引用返回，0个必要参数，返回类型 mixed，不允null
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_SensitiveParameterValue_getValue, 0, 0, IS_MIXED, 0)
ZEND_END_ARG_INFO()

// 创建 参数信息列表 ，增加第一个 返回类型不允null 的 参数（返回值）。
// 传入：变量名，是否引用返回，必要的参数数，类型，是否允null
// ing3, __debugInfo 方法：不可引用返回，0个必要参数，返回类型 array，不允null
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_SensitiveParameterValue___debugInfo, 0, 0, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

// Attribute 类的方法 映射的c函数
ZEND_METHOD(Attribute, __construct);
// ReturnTypeWillChange 类的方法 映射的c函数
ZEND_METHOD(ReturnTypeWillChange, __construct);
// AllowDynamicProperties 类的方法 映射的c函数
ZEND_METHOD(AllowDynamicProperties, __construct);
// SensitiveParameter 类的方法 映射的c函数
ZEND_METHOD(SensitiveParameter, __construct);
// SensitiveParameterValue 类的方法 映射的c函数
ZEND_METHOD(SensitiveParameterValue, __construct);
ZEND_METHOD(SensitiveParameterValue, getValue);
ZEND_METHOD(SensitiveParameterValue, __debugInfo);


// Attribute 类的方法列表 
static const zend_function_entry class_Attribute_methods[] = {
	// public __construct()
	ZEND_ME(Attribute, __construct, arginfo_class_Attribute___construct, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};

// ReturnTypeWillChange 类的方法列表 
static const zend_function_entry class_ReturnTypeWillChange_methods[] = {
	// public __construct()
	ZEND_ME(ReturnTypeWillChange, __construct, arginfo_class_ReturnTypeWillChange___construct, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};

// AllowDynamicProperties 类的方法列表 
static const zend_function_entry class_AllowDynamicProperties_methods[] = {
	// public __construct()
	ZEND_ME(AllowDynamicProperties, __construct, arginfo_class_AllowDynamicProperties___construct, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};

// SensitiveParameter 类的方法列表 
static const zend_function_entry class_SensitiveParameter_methods[] = {
	// public __construct()
	ZEND_ME(SensitiveParameter, __construct, arginfo_class_SensitiveParameter___construct, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};

// SensitiveParameterValue 类的方法列表 
static const zend_function_entry class_SensitiveParameterValue_methods[] = {
	// public __construct()
	ZEND_ME(SensitiveParameterValue, __construct, arginfo_class_SensitiveParameterValue___construct, ZEND_ACC_PUBLIC)
	// public getValue()
	ZEND_ME(SensitiveParameterValue, getValue, arginfo_class_SensitiveParameterValue_getValue, ZEND_ACC_PUBLIC)
	// public __debugInfo()
	ZEND_ME(SensitiveParameterValue, __debugInfo, arginfo_class_SensitiveParameterValue___debugInfo, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};

// ing3, 创建类 Attribute
static zend_class_entry *register_class_Attribute(void)
{
	zend_class_entry ce, *class_entry;
	// 创建类 Attribute
	INIT_CLASS_ENTRY(ce, "Attribute", class_Attribute_methods);
	// 注册成内部类，无继承
	class_entry = zend_register_internal_class_ex(&ce, NULL);
	// final 标记
	class_entry->ce_flags |= ZEND_ACC_FINAL;

	// 常量值
	zval const_TARGET_CLASS_value;
	ZVAL_LONG(&const_TARGET_CLASS_value, ZEND_ATTRIBUTE_TARGET_CLASS);
	// public TARGET_CLASS = (1<<0)
	zend_string *const_TARGET_CLASS_name = zend_string_init_interned("TARGET_CLASS", sizeof("TARGET_CLASS") - 1, 1);
	zend_declare_class_constant_ex(class_entry, const_TARGET_CLASS_name, &const_TARGET_CLASS_value, ZEND_ACC_PUBLIC, NULL);
	// 释放常量名
	zend_string_release(const_TARGET_CLASS_name);

	// 常量值
	zval const_TARGET_FUNCTION_value;
	ZVAL_LONG(&const_TARGET_FUNCTION_value, ZEND_ATTRIBUTE_TARGET_FUNCTION);
	// public TARGET_FUNCTION = (1<<1)
	zend_string *const_TARGET_FUNCTION_name = zend_string_init_interned("TARGET_FUNCTION", sizeof("TARGET_FUNCTION") - 1, 1);
	zend_declare_class_constant_ex(class_entry, const_TARGET_FUNCTION_name, &const_TARGET_FUNCTION_value, ZEND_ACC_PUBLIC, NULL);
	// 释放常量名
	zend_string_release(const_TARGET_FUNCTION_name);

	// 常量值
	zval const_TARGET_METHOD_value;
	ZVAL_LONG(&const_TARGET_METHOD_value, ZEND_ATTRIBUTE_TARGET_METHOD);
	// public const TARGET_METHOD=(1<<2)
	zend_string *const_TARGET_METHOD_name = zend_string_init_interned("TARGET_METHOD", sizeof("TARGET_METHOD") - 1, 1);
	zend_declare_class_constant_ex(class_entry, const_TARGET_METHOD_name, &const_TARGET_METHOD_value, ZEND_ACC_PUBLIC, NULL);
	zend_string_release(const_TARGET_METHOD_name);

	// 常量值
	zval const_TARGET_PROPERTY_value;
	ZVAL_LONG(&const_TARGET_PROPERTY_value, ZEND_ATTRIBUTE_TARGET_PROPERTY);
	// public const TARGET_PROPERTY=(1<<3)
	zend_string *const_TARGET_PROPERTY_name = zend_string_init_interned("TARGET_PROPERTY", sizeof("TARGET_PROPERTY") - 1, 1);
	zend_declare_class_constant_ex(class_entry, const_TARGET_PROPERTY_name, &const_TARGET_PROPERTY_value, ZEND_ACC_PUBLIC, NULL);
	zend_string_release(const_TARGET_PROPERTY_name);

	// 常量值
	zval const_TARGET_CLASS_CONSTANT_value;
	ZVAL_LONG(&const_TARGET_CLASS_CONSTANT_value, ZEND_ATTRIBUTE_TARGET_CLASS_CONST);
	// public const TARGET_CLASS_CONSTANT=(1<<4)
	zend_string *const_TARGET_CLASS_CONSTANT_name = zend_string_init_interned("TARGET_CLASS_CONSTANT", sizeof("TARGET_CLASS_CONSTANT") - 1, 1);
	zend_declare_class_constant_ex(class_entry, const_TARGET_CLASS_CONSTANT_name, &const_TARGET_CLASS_CONSTANT_value, ZEND_ACC_PUBLIC, NULL);
	zend_string_release(const_TARGET_CLASS_CONSTANT_name);

	// 常量值
	zval const_TARGET_PARAMETER_value;
	ZVAL_LONG(&const_TARGET_PARAMETER_value, ZEND_ATTRIBUTE_TARGET_PARAMETER);
	// public const TARGET_PARAMETER=(1<<5)
	zend_string *const_TARGET_PARAMETER_name = zend_string_init_interned("TARGET_PARAMETER", sizeof("TARGET_PARAMETER") - 1, 1);
	zend_declare_class_constant_ex(class_entry, const_TARGET_PARAMETER_name, &const_TARGET_PARAMETER_value, ZEND_ACC_PUBLIC, NULL);
	zend_string_release(const_TARGET_PARAMETER_name);

	// 常量值
	zval const_TARGET_ALL_value;
	ZVAL_LONG(&const_TARGET_ALL_value, ZEND_ATTRIBUTE_TARGET_ALL);
	// public const TARGET_ALL=((1<<6) - 1)
	zend_string *const_TARGET_ALL_name = zend_string_init_interned("TARGET_ALL", sizeof("TARGET_ALL") - 1, 1);
	zend_declare_class_constant_ex(class_entry, const_TARGET_ALL_name, &const_TARGET_ALL_value, ZEND_ACC_PUBLIC, NULL);
	zend_string_release(const_TARGET_ALL_name);

	// 常量值
	zval const_IS_REPEATABLE_value;
	ZVAL_LONG(&const_IS_REPEATABLE_value, ZEND_ATTRIBUTE_IS_REPEATABLE);
	// public const IS_REPEATABLE=(1<<6)
	zend_string *const_IS_REPEATABLE_name = zend_string_init_interned("IS_REPEATABLE", sizeof("IS_REPEATABLE") - 1, 1);
	zend_declare_class_constant_ex(class_entry, const_IS_REPEATABLE_name, &const_IS_REPEATABLE_value, ZEND_ACC_PUBLIC, NULL);
	zend_string_release(const_IS_REPEATABLE_name);

	// 属性值
	zval property_flags_default_value;
	ZVAL_UNDEF(&property_flags_default_value);
	// 属性名 flags
	zend_string *property_flags_name = zend_string_init("flags", sizeof("flags") - 1, 1);
	// public int $flags;
	zend_declare_typed_property(class_entry, property_flags_name, &property_flags_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release(property_flags_name);

	// #[Attribute()]
	// 修饰属性名 Attribute 
	zend_string *attribute_name_Attribute_class_Attribute = zend_string_init_interned("Attribute", sizeof("Attribute") - 1, 1);
	// 类添加修饰属性，1个参数
	zend_attribute *attribute_Attribute_class_Attribute = zend_add_class_attribute(class_entry, attribute_name_Attribute_class_Attribute, 1);
	//释放修饰属性名
	zend_string_release(attribute_name_Attribute_class_Attribute);
	
	zval attribute_Attribute_class_Attribute_arg0;
	// = (1<<0)
	ZVAL_LONG(&attribute_Attribute_class_Attribute_arg0, ZEND_ATTRIBUTE_TARGET_CLASS);
	// #[Attribute(1<<0)]
	ZVAL_COPY_VALUE(&attribute_Attribute_class_Attribute->args[0].value, &attribute_Attribute_class_Attribute_arg0);
	// 返回类入口
	return class_entry;
}

// ing3, 创建类 ReturnTypeWillChange
static zend_class_entry *register_class_ReturnTypeWillChange(void)
{
	zend_class_entry ce, *class_entry;
	// 创建类 ReturnTypeWillChange
	INIT_CLASS_ENTRY(ce, "ReturnTypeWillChange", class_ReturnTypeWillChange_methods);
	// 注册成内部内，无继承
	class_entry = zend_register_internal_class_ex(&ce, NULL);
	// 不可被继承
	class_entry->ce_flags |= ZEND_ACC_FINAL;

	// #[Attribute()]
	// 修饰属性名 Attribute 
	zend_string *attribute_name_Attribute_class_ReturnTypeWillChange = zend_string_init_interned("Attribute", sizeof("Attribute") - 1, 1);
	// 类添加修饰属性，1个参数
	zend_attribute *attribute_Attribute_class_ReturnTypeWillChange = zend_add_class_attribute(class_entry, attribute_name_Attribute_class_ReturnTypeWillChange, 1);
	//释放修饰属性名
	zend_string_release(attribute_name_Attribute_class_ReturnTypeWillChange);
	
	
	zval attribute_Attribute_class_ReturnTypeWillChange_arg0;
	// = (1<<2)
	ZVAL_LONG(&attribute_Attribute_class_ReturnTypeWillChange_arg0, ZEND_ATTRIBUTE_TARGET_METHOD);
	// #[Attribute(1<<2)]
	ZVAL_COPY_VALUE(&attribute_Attribute_class_ReturnTypeWillChange->args[0].value, &attribute_Attribute_class_ReturnTypeWillChange_arg0);
	// 返回类入口
	return class_entry;
}

// ing3, 创建类 AllowDynamicProperties
static zend_class_entry *register_class_AllowDynamicProperties(void)
{
	zend_class_entry ce, *class_entry;
	// 创建类 AllowDynamicProperties
	INIT_CLASS_ENTRY(ce, "AllowDynamicProperties", class_AllowDynamicProperties_methods);
	// 注册成内部内，无继承
	class_entry = zend_register_internal_class_ex(&ce, NULL);
	// 不可被继承
	class_entry->ce_flags |= ZEND_ACC_FINAL;

	// #[Attribute()]
	// 修饰属性名 Attribute 
	zend_string *attribute_name_Attribute_class_AllowDynamicProperties = zend_string_init_interned("Attribute", sizeof("Attribute") - 1, 1);
	// 类添加修饰属性，1个参数
	zend_attribute *attribute_Attribute_class_AllowDynamicProperties = zend_add_class_attribute(class_entry, attribute_name_Attribute_class_AllowDynamicProperties, 1);
	//释放修饰属性名
	zend_string_release(attribute_name_Attribute_class_AllowDynamicProperties);
	
	
	zval attribute_Attribute_class_AllowDynamicProperties_arg0;
	// = (1<<0)
	ZVAL_LONG(&attribute_Attribute_class_AllowDynamicProperties_arg0, ZEND_ATTRIBUTE_TARGET_CLASS);
	// #[Attribute(1<<0)]
	ZVAL_COPY_VALUE(&attribute_Attribute_class_AllowDynamicProperties->args[0].value, &attribute_Attribute_class_AllowDynamicProperties_arg0);
	// 返回类入口
	return class_entry;
}

// ing3, 创建类 SensitiveParameter
static zend_class_entry *register_class_SensitiveParameter(void)
{
	zend_class_entry ce, *class_entry;
	// 创建类 SensitiveParameter
	INIT_CLASS_ENTRY(ce, "SensitiveParameter", class_SensitiveParameter_methods);
	// 注册成内部内，无继承
	class_entry = zend_register_internal_class_ex(&ce, NULL);
	// 不可被继承，不支持动态属性
	class_entry->ce_flags |= ZEND_ACC_FINAL|ZEND_ACC_NO_DYNAMIC_PROPERTIES;
	
	// #[Attribute()]
	// 修饰属性名 Attribute 
	zend_string *attribute_name_Attribute_class_SensitiveParameter = zend_string_init_interned("Attribute", sizeof("Attribute") - 1, 1);
	// 类添加修饰属性，1个参数
	zend_attribute *attribute_Attribute_class_SensitiveParameter = zend_add_class_attribute(class_entry, attribute_name_Attribute_class_SensitiveParameter, 1);
	// 释放修饰属性名
	zend_string_release(attribute_name_Attribute_class_SensitiveParameter);
	
	// 第一个参数值
	zval attribute_Attribute_class_SensitiveParameter_arg0;
	// = (1<<5)
	ZVAL_LONG(&attribute_Attribute_class_SensitiveParameter_arg0, ZEND_ATTRIBUTE_TARGET_PARAMETER);
	// #[Attribute(1<<5)]
	ZVAL_COPY_VALUE(&attribute_Attribute_class_SensitiveParameter->args[0].value, &attribute_Attribute_class_SensitiveParameter_arg0);
	// 返回类入口
	return class_entry;
}

// ing4, 创建类 SensitiveParameterValue
static zend_class_entry *register_class_SensitiveParameterValue(void)
{
	zend_class_entry ce, *class_entry;
	// 创建类 SensitiveParameter
	INIT_CLASS_ENTRY(ce, "SensitiveParameterValue", class_SensitiveParameterValue_methods);
	// 注册成内部内，无继承
	class_entry = zend_register_internal_class_ex(&ce, NULL);
	// 不可被继承，不支持动态属性，不可序列化
	class_entry->ce_flags |= ZEND_ACC_FINAL|ZEND_ACC_NO_DYNAMIC_PROPERTIES|ZEND_ACC_NOT_SERIALIZABLE;

	// 属性值
	zval property_value_default_value;
	// 值为未定义
	ZVAL_UNDEF(&property_value_default_value);
	// 属性名 value
	zend_string *property_value_name = zend_string_init("value", sizeof("value") - 1, 1);
	// readonly private mixed $value;
	zend_declare_typed_property(class_entry, property_value_name, &property_value_default_value, ZEND_ACC_PRIVATE|ZEND_ACC_READONLY, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_ANY));
	// 释放属性名
	zend_string_release(property_value_name);
	// 返回类入口
	return class_entry;
}
