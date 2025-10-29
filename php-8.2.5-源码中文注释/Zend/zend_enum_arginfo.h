/* This is a generated file, edit the .stub.php file instead.
 * Stub hash: 7092f1d4ba651f077cff37050899f090f00abf22 */

// 创建 cases 方法的参数信息列表 ，增加第一个 is_tentative 为0 的 参数（返回值）。
// 变量名:arginfo_class_UnitEnum_cases，非引用返回，非必须的参数数，类型:数组，非null
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_UnitEnum_cases, 0, 0, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

// 创建 参数信息列表 ，增加第一个 is_tentative 为0 的 参数（返回值）。
// 变量名:arginfo_class_BackedEnum_from，非引用返回，必须的参数数，类型:IS_STATIC，非null
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_BackedEnum_from, 0, 1, IS_STATIC, 0)
	// 有默认值、需要type_mask的 参数信息，可引用传递，参数名称，type_mask，默认值
	// 不可引用传递，参数名value,类型 MAY_BE_LONG|MAY_BE_STRING 无默认值
	ZEND_ARG_TYPE_MASK(0, value, MAY_BE_LONG|MAY_BE_STRING, NULL)
ZEND_END_ARG_INFO()

// 创建 参数信息列表 ，增加第一个 is_tentative 为0 的 参数（返回值）。
// 变量名:arginfo_class_BackedEnum_tryFrom，非引用返回，必须的参数数，类型:IS_STATIC，允许null
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_BackedEnum_tryFrom, 0, 1, IS_STATIC, 1)
	// 不可引用传递，参数名value,类型 MAY_BE_LONG|MAY_BE_STRING 无默认值
	ZEND_ARG_TYPE_MASK(0, value, MAY_BE_LONG|MAY_BE_STRING, NULL)
ZEND_END_ARG_INFO()



// ing3, UnitEnum 接口 的方法列表
static const zend_function_entry class_UnitEnum_methods[] = {
	// 抽象方法  static public abstract function cases()
	ZEND_ABSTRACT_ME_WITH_FLAGS(UnitEnum, cases, arginfo_class_UnitEnum_cases, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC|ZEND_ACC_ABSTRACT)
	ZEND_FE_END
};

// ing3, BackedEnum 接口 的方法
static const zend_function_entry class_BackedEnum_methods[] = {
	// 抽象方法 static public abstract function from(){}
	ZEND_ABSTRACT_ME_WITH_FLAGS(BackedEnum, from, arginfo_class_BackedEnum_from, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC|ZEND_ACC_ABSTRACT)
	// 抽象方法  static public abstract function tryFrom(){}
	ZEND_ABSTRACT_ME_WITH_FLAGS(BackedEnum, tryFrom, arginfo_class_BackedEnum_tryFrom, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC|ZEND_ACC_ABSTRACT)
	ZEND_FE_END
};

// ing3, 注册接口 UnitEnum
static zend_class_entry *register_class_UnitEnum(void)
{
	zend_class_entry ce, *class_entry;
	// 创建类入口 UnitEnum
	INIT_CLASS_ENTRY(ce, "UnitEnum", class_UnitEnum_methods);
	// 把它注册成内部接口
	class_entry = zend_register_internal_interface(&ce);
	// 返回类实例
	return class_entry;
}

// ing3, 注册类 BackedEnum
static zend_class_entry *register_class_BackedEnum(zend_class_entry *class_entry_UnitEnum)
{
	zend_class_entry ce, *class_entry;
	// 创建类入口 BackedEnum
	INIT_CLASS_ENTRY(ce, "BackedEnum", class_BackedEnum_methods);
	// 把它注册成内部接口
	class_entry = zend_register_internal_interface(&ce);
	// 此类实现1个接口 UnitEnum
	zend_class_implements(class_entry, 1, class_entry_UnitEnum);
	// 返回类实例
	return class_entry;
}
