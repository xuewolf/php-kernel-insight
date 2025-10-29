/* This is a generated file, edit the .stub.php file instead.
 * Stub hash: e3b480674671a698814db282c5ea34d438fe519d */

// 创建参数信息列表 ，增加第一个参数（返回值）。（大量引用）
// 传入：变量名，_unused？，是否引用返回，必要的参数数
// ing3, __construct 方法，没有 _unused，不可引用返回，无必要参数
ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Closure___construct, 0, 0, 0)
// 参数列结束
ZEND_END_ARG_INFO()

// 创建参数信息列表 ，增加第一个参数（返回值）：返回类型不允null。
// 传入参数名，是否引用返回，必要参数数，返回类型，是否允null
// ing3, bind 方法：不可引用返回，2个必要参数，返回类型 Closure，可以是null
ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_class_Closure_bind, 0, 2, Closure, 1)
	// ing3, 定义对象返回值。参数：可否引用传递, 参数名，类名，是否允null
	ZEND_ARG_OBJ_INFO(0, closure, Closure, 0)
	// ing3, 无默认值的 参数信息：可引用传递，参数名称，类型，可以是null
	ZEND_ARG_TYPE_INFO(0, newThis, IS_OBJECT, 1)
	// ing3, 有默认值、需要type_mask的 参数信息，可引用传递，参数名称，type_mask，默认值
	ZEND_ARG_TYPE_MASK(0, newScope, MAY_BE_OBJECT|MAY_BE_STRING|MAY_BE_NULL, "\"static\"")
// 参数列结束
ZEND_END_ARG_INFO()

// 创建参数信息列表 ，增加第一个参数（返回值）：返回类型不允null。
// 传入参数名，是否引用返回，必要参数数，返回类型，是否允null
// ing3, bindTo 方法：不可引用返回，1个必要参数，返回类型 Closure，可以是null
ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_class_Closure_bindTo, 0, 1, Closure, 1)
	// ing3, 无默认值的 参数信息：可引用传递，参数名称，类型，可以是null
	ZEND_ARG_TYPE_INFO(0, newThis, IS_OBJECT, 1)
	// ing3, 有默认值、需要type_mask的 参数信息，可引用传递，参数名称，type_mask，默认值
	ZEND_ARG_TYPE_MASK(0, newScope, MAY_BE_OBJECT|MAY_BE_STRING|MAY_BE_NULL, "\"static\"")
// 参数列结束
ZEND_END_ARG_INFO()

// 创建 参数信息列表 ，增加第一个 返回类型不允null 的 参数（返回值）。
// 传入：变量名，是否引用返回，必要的参数数，类型，是否允null
// ing3, call 方法：不可引用返回，1个必要参数，返回类型 mixed，不允null
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Closure_call, 0, 1, IS_MIXED, 0)
	// ing3, 无默认值的 参数信息：可引用传递，参数名称，类型，可以是null
	ZEND_ARG_TYPE_INFO(0, newThis, IS_OBJECT, 0)
	// ing3, 无默认值的 字典参数信息：可引用传递，参数名称，类型，可以是null	
	ZEND_ARG_VARIADIC_TYPE_INFO(0, args, IS_MIXED, 0)
// 参数列结束
ZEND_END_ARG_INFO()

// 创建参数信息列表 ，增加第一个参数（返回值）：返回类型不允null。
// 传入参数名，是否引用返回，必要参数数，返回类型，是否允null
// ing3, fromCallable 方法：不可引用返回，1个必要参数，返回类型 Closure，不可以是null
ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_class_Closure_fromCallable, 0, 1, Closure, 0)
	// ing3, 无默认值的 参数信息：可引用传递，参数名称，类型，可以是null
	ZEND_ARG_TYPE_INFO(0, callback, IS_CALLABLE, 0)
// 参数列结束
ZEND_END_ARG_INFO()


// Closure类的5个方法，都是抽象函数
ZEND_METHOD(Closure, __construct);
ZEND_METHOD(Closure, bind);
ZEND_METHOD(Closure, bindTo);
ZEND_METHOD(Closure, call);
ZEND_METHOD(Closure, fromCallable);

// ing3, 创建类 Closure
static const zend_function_entry class_Closure_methods[] = {
	// private function function __construct => arginfo_class_Closure___construct
	ZEND_ME(Closure, __construct, arginfo_class_Closure___construct, ZEND_ACC_PRIVATE)
	// static public function bind => arginfo_class_Closure_bind
	ZEND_ME(Closure, bind, arginfo_class_Closure_bind, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	// public function bindTo => arginfo_class_Closure_bindTo
	ZEND_ME(Closure, bindTo, arginfo_class_Closure_bindTo, ZEND_ACC_PUBLIC)
	// public function call => arginfo_class_Closure_call
	ZEND_ME(Closure, call, arginfo_class_Closure_call, ZEND_ACC_PUBLIC)
	// static public function fromCallable => arginfo_class_Closure_fromCallable
	ZEND_ME(Closure, fromCallable, arginfo_class_Closure_fromCallable, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	ZEND_FE_END
};

// ing3, 创建类 Closure
static zend_class_entry *register_class_Closure(void)
{
	zend_class_entry ce, *class_entry;
	// 创建新类 Closure，并添加成员方法列表，所以闭包本身是php里面的类	
	INIT_CLASS_ENTRY(ce, "Closure", class_Closure_methods);
	// 注册内置类，第二个参数是父类
	class_entry = zend_register_internal_class_ex(&ce, NULL);
	// final, 无动态属性，不可序列化
	class_entry->ce_flags |= ZEND_ACC_FINAL|ZEND_ACC_NO_DYNAMIC_PROPERTIES|ZEND_ACC_NOT_SERIALIZABLE;
	// 返回类入口
	return class_entry;
}
