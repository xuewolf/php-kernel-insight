/* This is a generated file, edit the .stub.php file instead.
 * Stub hash: e82bbc8e81fe98873a9a5697a4b38e63a24379da */

// 创建参数信息列表 ，增加第一个参数（返回值）。（大量引用）
// 传入：变量名，_unused？，是否引用返回，必要的参数数
// ing3, __construct 方法，没有 _unused，不可引用返回，无必要参数
ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Fiber___construct, 0, 0, 1)
	// ing3, 无默认值的 参数信息：不可引用传递，参数名称 callback，类型 callable，不可以是null
	ZEND_ARG_TYPE_INFO(0, callback, IS_CALLABLE, 0)
ZEND_END_ARG_INFO()

// 创建 参数信息列表 ，增加第一个 返回类型不允null 的 参数（返回值）。（大量引用）
// 传入：变量名，是否引用返回，必要的参数数，类型，是否允null
// ing3, start 方法：不可引用返回，0个必要参数，返回类型 mixed，不允null
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Fiber_start, 0, 0, IS_MIXED, 0)
	// ing3, 无默认值的 字典参数信息：可引用传递，参数名称，类型，可以是null
	ZEND_ARG_VARIADIC_TYPE_INFO(0, args, IS_MIXED, 0)
ZEND_END_ARG_INFO()

// 创建 参数信息列表 ，增加第一个 返回类型不允null 的 参数（返回值）。（大量引用）
// 传入：变量名，是否引用返回，必要的参数数，类型，是否允null
// ing3, resume 方法：不可引用返回，0个必要参数，返回类型 mixed，不允null
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Fiber_resume, 0, 0, IS_MIXED, 0)
	// ing3, 有默认值的 参数信息：可引用传递，参数名称，类型，不可以是null, 默认值null
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, value, IS_MIXED, 0, "null")
ZEND_END_ARG_INFO()

// 创建 参数信息列表 ，增加第一个 返回类型不允null 的 参数（返回值）。（大量引用）
// 传入：变量名，是否引用返回，必要的参数数，类型，是否允null
// ing3, throw 方法：不可引用返回，1个必要参数，返回类型 mixed，不允null
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Fiber_throw, 0, 1, IS_MIXED, 0)
	// ing3, 定义对象返回值。参数：可否引用传递, 参数名，类名，是否允null
	ZEND_ARG_OBJ_INFO(0, exception, Throwable, 0)
ZEND_END_ARG_INFO()

// 创建 参数信息列表 ，增加第一个 返回类型不允null 的 参数（返回值）。（大量引用）
// 传入：变量名，是否引用返回，必要的参数数，类型，是否允null
// ing3, isStarted 方法：不可引用返回，0个必要参数，返回类型 bool，不允null
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Fiber_isStarted, 0, 0, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

// isSuspended 方法的参数表与 isStarted 方法相同
#define arginfo_class_Fiber_isSuspended arginfo_class_Fiber_isStarted

// isRunning 方法的参数表与 isStarted 方法相同
#define arginfo_class_Fiber_isRunning arginfo_class_Fiber_isStarted

// isTerminated 方法的参数表与 isStarted 方法相同
#define arginfo_class_Fiber_isTerminated arginfo_class_Fiber_isStarted

// 创建 参数信息列表 ，增加第一个 返回类型不允null 的 参数（返回值）。（大量引用）
// 传入：变量名，是否引用返回，必要的参数数，类型，是否允null
// ing3, getReturn 方法：不可引用返回，0个必要参数，返回类型 mixed，不允null
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Fiber_getReturn, 0, 0, IS_MIXED, 0)
ZEND_END_ARG_INFO()

// 创建参数信息列表 ，增加第一个参数（返回值）：返回类型不允null。
// 传入参数名，是否引用返回，必要参数数，返回类型，是否允null
// ing3, getCurrent 方法：不可引用返回，0个必要参数，返回类型 Fiber，允null
ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_class_Fiber_getCurrent, 0, 0, Fiber, 1)
ZEND_END_ARG_INFO()

// suspend 方法的参数表与 resume 方法相同
#define arginfo_class_Fiber_suspend arginfo_class_Fiber_resume

// ing2, 创建参数信息列表 ，增加第一个参数（返回值）。（大量引用）
// 传入：变量名，_unused？，是否引用返回，必要的参数数
ZEND_BEGIN_ARG_INFO_EX(arginfo_class_FiberError___construct, 0, 0, 0)
ZEND_END_ARG_INFO()

// Fiber 类的方法 映射的c函数
ZEND_METHOD(Fiber, __construct);
ZEND_METHOD(Fiber, start);
ZEND_METHOD(Fiber, resume);
ZEND_METHOD(Fiber, throw);
ZEND_METHOD(Fiber, isStarted);
ZEND_METHOD(Fiber, isSuspended);
ZEND_METHOD(Fiber, isRunning);
ZEND_METHOD(Fiber, isTerminated);
ZEND_METHOD(Fiber, getReturn);
ZEND_METHOD(Fiber, getCurrent);
ZEND_METHOD(Fiber, suspend);

// FiberError 类的方法 映射的c函数
ZEND_METHOD(FiberError, __construct);

// Fiber 类的方法
static const zend_function_entry class_Fiber_methods[] = {
	ZEND_ME(Fiber, __construct, arginfo_class_Fiber___construct, ZEND_ACC_PUBLIC)
	ZEND_ME(Fiber, start, arginfo_class_Fiber_start, ZEND_ACC_PUBLIC)
	ZEND_ME(Fiber, resume, arginfo_class_Fiber_resume, ZEND_ACC_PUBLIC)
	ZEND_ME(Fiber, throw, arginfo_class_Fiber_throw, ZEND_ACC_PUBLIC)
	ZEND_ME(Fiber, isStarted, arginfo_class_Fiber_isStarted, ZEND_ACC_PUBLIC)
	ZEND_ME(Fiber, isSuspended, arginfo_class_Fiber_isSuspended, ZEND_ACC_PUBLIC)
	ZEND_ME(Fiber, isRunning, arginfo_class_Fiber_isRunning, ZEND_ACC_PUBLIC)
	ZEND_ME(Fiber, isTerminated, arginfo_class_Fiber_isTerminated, ZEND_ACC_PUBLIC)
	ZEND_ME(Fiber, getReturn, arginfo_class_Fiber_getReturn, ZEND_ACC_PUBLIC)
	ZEND_ME(Fiber, getCurrent, arginfo_class_Fiber_getCurrent, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	ZEND_ME(Fiber, suspend, arginfo_class_Fiber_suspend, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	ZEND_FE_END
};


// FiberError 类的方法
static const zend_function_entry class_FiberError_methods[] = {
	ZEND_ME(FiberError, __construct, arginfo_class_FiberError___construct, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};

// ing3, 注册 Fiber 类 
static zend_class_entry *register_class_Fiber(void)
{
	zend_class_entry ce, *class_entry;
	// 创建类 Fiber
	INIT_CLASS_ENTRY(ce, "Fiber", class_Fiber_methods);
	// 注册内部类，无父类
	class_entry = zend_register_internal_class_ex(&ce, NULL);
	// final（不可继承），不支持动态属性，不可序列化
	class_entry->ce_flags |= ZEND_ACC_FINAL|ZEND_ACC_NO_DYNAMIC_PROPERTIES|ZEND_ACC_NOT_SERIALIZABLE;

	return class_entry;
}

// ing3, 注册 FiberError 类 
static zend_class_entry *register_class_FiberError(zend_class_entry *class_entry_Error)
{
	zend_class_entry ce, *class_entry;
	// 创建类 Fiber
	INIT_CLASS_ENTRY(ce, "FiberError", class_FiberError_methods);
	// 注册内部类，继承 class_entry_Error
	class_entry = zend_register_internal_class_ex(&ce, class_entry_Error);
	// final（不可继承）
	class_entry->ce_flags |= ZEND_ACC_FINAL;

	return class_entry;
}
