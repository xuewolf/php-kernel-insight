/* This is a generated file, edit the .stub.php file instead.
 * Stub hash: 0af5e8985dd4645bf23490b8cec312f8fd1fee2e */

// 创建 参数信息列表 ，增加第一个 返回类型不允null 的 参数（返回值）。
// 传入：变量名，是否引用返回，必要的参数数，类型，是否允null
// ing3, rewind 方法：不可引用返回，0个必要参数，返回类型 void，不可以是null
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Generator_rewind, 0, 0, IS_VOID, 0)
// 参数列结束
ZEND_END_ARG_INFO()

// 创建 参数信息列表 ，增加第一个 返回类型不允null 的 参数（返回值）。
// 传入：变量名，是否引用返回，必要的参数数，类型，是否允null
// ing3, valid 方法：不可引用返回，0个必要参数，返回类型 bool，不可以是null
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Generator_valid, 0, 0, _IS_BOOL, 0)
// 参数列结束
ZEND_END_ARG_INFO()

// 创建 参数信息列表 ，增加第一个 返回类型不允null 的 参数（返回值）。
// 传入：变量名，是否引用返回，必要的参数数，类型，是否允null
// ing3, current 方法：不可引用返回，0个必要参数，返回类型 mixed，不可以是null
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Generator_current, 0, 0, IS_MIXED, 0)
// 参数列结束
ZEND_END_ARG_INFO()

// key 方法的参数列表与 current 方法相同
#define arginfo_class_Generator_key arginfo_class_Generator_current

// next 方法的参数列表与 rewind 方法相同
#define arginfo_class_Generator_next arginfo_class_Generator_rewind

// 创建 参数信息列表 ，增加第一个 返回类型不允null 的 参数（返回值）。
// 传入：变量名，是否引用返回，必要的参数数，类型，是否允null
// ing3, send 方法：不可引用返回，1个必要参数，返回类型 mixed，不可以是null
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Generator_send, 0, 1, IS_MIXED, 0)
	// ing3, 无默认值的 参数信息：不可引用传递，参数名称 object，类型 object，不可以是null
	ZEND_ARG_TYPE_INFO(0, value, IS_MIXED, 0)
// 参数列结束
ZEND_END_ARG_INFO()

// 创建 参数信息列表 ，增加第一个 返回类型不允null 的 参数（返回值）。
// 传入：变量名，是否引用返回，必要的参数数，类型，是否允null
// ing3, throw 方法：不可引用返回，1个必要参数，返回类型 mixed，不可以是null
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Generator_throw, 0, 1, IS_MIXED, 0)
	// ing3, 定义对象类型参数。参数：可否引用传递, 参数名，类型名，是否允null
	ZEND_ARG_OBJ_INFO(0, exception, Throwable, 0)
// 参数列结束
ZEND_END_ARG_INFO()

// getReturn 方法的参数列表与 current 方法相同
#define arginfo_class_Generator_getReturn arginfo_class_Generator_current

// Generator 类的方法 映射的c函数
ZEND_METHOD(Generator, rewind);
ZEND_METHOD(Generator, valid);
ZEND_METHOD(Generator, current);
ZEND_METHOD(Generator, key);
ZEND_METHOD(Generator, next);
ZEND_METHOD(Generator, send);
ZEND_METHOD(Generator, throw);
ZEND_METHOD(Generator, getReturn);

// Generator 类的方法
static const zend_function_entry class_Generator_methods[] = {
	// public rewind()
	ZEND_ME(Generator, rewind, arginfo_class_Generator_rewind, ZEND_ACC_PUBLIC)
	// public valid
	ZEND_ME(Generator, valid, arginfo_class_Generator_valid, ZEND_ACC_PUBLIC)
	// public current
	ZEND_ME(Generator, current, arginfo_class_Generator_current, ZEND_ACC_PUBLIC)
	// public key
	ZEND_ME(Generator, key, arginfo_class_Generator_key, ZEND_ACC_PUBLIC)
	// public next
	ZEND_ME(Generator, next, arginfo_class_Generator_next, ZEND_ACC_PUBLIC)
	// public send
	ZEND_ME(Generator, send, arginfo_class_Generator_send, ZEND_ACC_PUBLIC)
	// public throw
	ZEND_ME(Generator, throw, arginfo_class_Generator_throw, ZEND_ACC_PUBLIC)
	// public getReturn
	ZEND_ME(Generator, getReturn, arginfo_class_Generator_getReturn, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};

// ClosedGeneratorException 类的方法（空）
static const zend_function_entry class_ClosedGeneratorException_methods[] = {
	ZEND_FE_END
};

// ing3, 注册类 Generator
static zend_class_entry *register_class_Generator(zend_class_entry *class_entry_Iterator)
{
	zend_class_entry ce, *class_entry;
	// 创建类 Generator
	INIT_CLASS_ENTRY(ce, "Generator", class_Generator_methods);
	// 注册成内部类，无父类
	class_entry = zend_register_internal_class_ex(&ce, NULL);
	// final, 不支持动态属性，不可序列化
	class_entry->ce_flags |= ZEND_ACC_FINAL|ZEND_ACC_NO_DYNAMIC_PROPERTIES|ZEND_ACC_NOT_SERIALIZABLE;
	// 实现接口 class_entry_Iterator
	zend_class_implements(class_entry, 1, class_entry_Iterator);
	// 
	return class_entry;
}

// ing3, 注册类 ClosedGeneratorException
static zend_class_entry *register_class_ClosedGeneratorException(zend_class_entry *class_entry_Exception)
{
	zend_class_entry ce, *class_entry;
	// 创建类 ClosedGeneratorException
	INIT_CLASS_ENTRY(ce, "ClosedGeneratorException", class_ClosedGeneratorException_methods);
	// 注册成内部类，继承类 class_entry_Exception
	class_entry = zend_register_internal_class_ex(&ce, class_entry_Exception);
	// 
	return class_entry;
}
