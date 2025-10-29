/* This is a generated file, edit the .stub.php file instead.
 * Stub hash: a9c915c11e5989d8c7cf2d704ada09ca765670c3 */

// 创建各个方法的形式参数列表
// 每个后面都要加一个结束符号 #define ZEND_END_ARG_INFO()		};


// 创建参数信息列表 ，增加第一个参数（返回值）。
// 传入参数名，是否引用返回，必要参数数量 ，返回类型，is_tentative_return_type
// ing3, getIterator方法的参数列表和第一个参数（返回值）。返回值类型是 Traversable。
ZEND_BEGIN_ARG_WITH_TENTATIVE_RETURN_OBJ_INFO_EX(arginfo_class_IteratorAggregate_getIterator, 0, 0, Traversable, 0)
ZEND_END_ARG_INFO()

// 创建 参数信息列表 ，增加第一个 is_tentative 为1 的 参数（返回值）。
// 传入：变量名，是否引用返回，必要的参数数，类型，是否允null
// ing3, current方法的参数列表和第一个参数（返回值）。返回值类型是 mixed。
ZEND_BEGIN_ARG_WITH_TENTATIVE_RETURN_TYPE_INFO_EX(arginfo_class_Iterator_current, 0, 0, IS_MIXED, 0)
ZEND_END_ARG_INFO()

// 创建 参数信息列表 ，增加第一个 is_tentative 为1 的 参数（返回值）。
// 传入：变量名，是否引用返回，必要的参数数，类型，是否允null
// ing3, next方法的参数列表和第一个参数（返回值）。无返回值类型。
ZEND_BEGIN_ARG_WITH_TENTATIVE_RETURN_TYPE_INFO_EX(arginfo_class_Iterator_next, 0, 0, IS_VOID, 0)
ZEND_END_ARG_INFO()

// Iterator 接口中 key 和 current 方法的参数信息相同
#define arginfo_class_Iterator_key arginfo_class_Iterator_current

// 创建 参数信息列表 ，增加第一个 is_tentative 为1 的 参数（返回值）。
// 传入：变量名，是否引用返回，必要的参数数，类型，是否允null
// ing3, valid方法的参数列表和第一个参数（返回值）。返回值类型是 bool。
ZEND_BEGIN_ARG_WITH_TENTATIVE_RETURN_TYPE_INFO_EX(arginfo_class_Iterator_valid, 0, 0, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

// Iterator 接口中 rewind 和 next 方法的参数信息相同
#define arginfo_class_Iterator_rewind arginfo_class_Iterator_next

// 创建 参数信息列表 ，增加第一个 is_tentative 为1 的 参数（返回值）。
// 传入：变量名，是否引用返回，必要的参数数，类型，是否允null
// ing3, offsetExists方法的参数列表和第一个参数（返回值）。返回值类型是 bool。
ZEND_BEGIN_ARG_WITH_TENTATIVE_RETURN_TYPE_INFO_EX(arginfo_class_ArrayAccess_offsetExists, 0, 1, _IS_BOOL, 0)
	//无默认值的 参数信息：可引用传递，参数名称，类型，可以是null
	// ing3, 第一个传入参数 offset,类型是mixed
	ZEND_ARG_TYPE_INFO(0, offset, IS_MIXED, 0)
ZEND_END_ARG_INFO()

// 创建 参数信息列表 ，增加第一个 is_tentative 为1 的 参数（返回值）。
// 传入：变量名，是否引用返回，必要的参数数，类型，是否允null
// ing3, offsetGet方法的参数列表和第一个参数（返回值）。返回值类型是 mixed。
ZEND_BEGIN_ARG_WITH_TENTATIVE_RETURN_TYPE_INFO_EX(arginfo_class_ArrayAccess_offsetGet, 0, 1, IS_MIXED, 0)
	//无默认值的 参数信息：可引用传递，参数名称，类型，可以是null
	// ing3, 第一个传入参数 offset,类型是mixed
	ZEND_ARG_TYPE_INFO(0, offset, IS_MIXED, 0)
ZEND_END_ARG_INFO()

// 创建 参数信息列表 ，增加第一个 is_tentative 为1 的 参数（返回值）。
// 传入：变量名，是否引用返回，必要的参数数，类型，是否允null
// ing3, offsetSet方法的参数列表和第一个参数（返回值）。返回void。
ZEND_BEGIN_ARG_WITH_TENTATIVE_RETURN_TYPE_INFO_EX(arginfo_class_ArrayAccess_offsetSet, 0, 2, IS_VOID, 0)
	//无默认值的 参数信息：可引用传递，参数名称，类型，可以是null
	// ing3, 第一个传入参数 offset,类型是mixed
	ZEND_ARG_TYPE_INFO(0, offset, IS_MIXED, 0)
	//无默认值的 参数信息：可引用传递，参数名称，类型，可以是null
	// ing3, 第二个传入参数 value,类型是mixed
	ZEND_ARG_TYPE_INFO(0, value, IS_MIXED, 0)
ZEND_END_ARG_INFO()

// 创建 参数信息列表 ，增加第一个 is_tentative 为1 的 参数（返回值）。
// 传入：变量名，是否引用返回，必要的参数数，类型，是否允null
// ing3, offsetUnset方法的参数列表和第一个参数（返回值）。返回void。
ZEND_BEGIN_ARG_WITH_TENTATIVE_RETURN_TYPE_INFO_EX(arginfo_class_ArrayAccess_offsetUnset, 0, 1, IS_VOID, 0)
	//无默认值的 参数信息：可引用传递，参数名称，类型，可以是null
	// ing3, 第一个传入参数 offset,类型是mixed
	ZEND_ARG_TYPE_INFO(0, offset, IS_MIXED, 0)
ZEND_END_ARG_INFO()

//  创建参数信息列表 ，增加第一个参数（返回值）。
// 传入：变量名，_unused？，是否引用返回，必要的参数数
// ing3, serialize方法的参数列表和第一个参数（返回值）,无规定类型。
ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Serializable_serialize, 0, 0, 0)
ZEND_END_ARG_INFO()


//  创建参数信息列表 ，增加第一个参数（返回值）。
// 传入：变量名，_unused？，是否引用返回，必要的参数数
// ing3, unserialize方法的参数列表和第一个参数（返回值）,无规定类型。
ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Serializable_unserialize, 0, 0, 1)
	//无默认值的 参数信息：可引用传递，参数名称，类型，可以是null
	// ing3, 第一个传入参数 data,类型是string
	ZEND_ARG_TYPE_INFO(0, data, IS_STRING, 0)
ZEND_END_ARG_INFO()

// 创建 参数信息列表 ，增加第一个 is_tentative 为1 的 参数（返回值）。
// 传入：变量名，是否引用返回，必要的参数数，类型，是否允null
// ing3, count方法的参数列表和第一个参数（返回值）。返回值是整数型，无传入参数。
ZEND_BEGIN_ARG_WITH_TENTATIVE_RETURN_TYPE_INFO_EX(arginfo_class_Countable_count, 0, 0, IS_LONG, 0)
ZEND_END_ARG_INFO()

// 创建 参数信息列表 ，增加第一个 is_tentative 为0 的 参数（返回值）。
// 传入：变量名，是否引用返回，必要的参数数，类型，是否允null
// ing3, __toString方法的参数列表和第一个参数（返回值）。返回值是string型，无传入参数。
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Stringable___toString, 0, 0, IS_STRING, 0)
ZEND_END_ARG_INFO()

// Iterator 接口中 __construct 和 _serialize 方法的参数信息相同
#define arginfo_class_InternalIterator___construct arginfo_class_Serializable_serialize


// 创建 参数信息列表 ，增加第一个 is_tentative 为0 的 参数（返回值）。
// 传入：变量名，是否引用返回，必要的参数数，类型，是否允null
// ing3, current方法的参数列表和第一个参数（返回值）。返回值是mixed型，无传入参数。
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_InternalIterator_current, 0, 0, IS_MIXED, 0)
ZEND_END_ARG_INFO()

// InternalIterator 接口中 key和current 方法的参数信息相同
#define arginfo_class_InternalIterator_key arginfo_class_InternalIterator_current

// 创建 参数信息列表 ，增加第一个 is_tentative 为0 的 参数（返回值）。
// 传入：变量名，是否引用返回，必要的参数数，类型，是否允null
// ing3, next方法的参数列表和第一个参数（返回值）。返回值是void型，无传入参数。
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_InternalIterator_next, 0, 0, IS_VOID, 0)
ZEND_END_ARG_INFO()

// 创建 参数信息列表 ，增加第一个 is_tentative 为0 的 参数（返回值）。
// 传入：变量名，是否引用返回，必要的参数数，类型，是否允null
// ing3, valid方法的参数列表和第一个参数（返回值）。返回值是bool型，无传入参数。
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_InternalIterator_valid, 0, 0, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

// InternalIterator 接口中 rewind和next 方法的参数信息相同
#define arginfo_class_InternalIterator_rewind arginfo_class_InternalIterator_next

// ing4, 注册 InternalIterator 类的 抽象成员方法
// #define ZEND_METHOD(classname, name) ZEND_NAMED_FUNCTION(zim_##classname##_##name)
// void ZEND_FASTCALL zim_InternalIterator___construct(zend_execute_data *execute_data, zval *return_value);
ZEND_METHOD(InternalIterator, __construct);
// void ZEND_FASTCALL zim_InternalIterator_current(zend_execute_data *execute_data, zval *return_value);
ZEND_METHOD(InternalIterator, current);
// void ZEND_FASTCALL zim_InternalIterator_key(zend_execute_data *execute_data, zval *return_value);
ZEND_METHOD(InternalIterator, key);
// void ZEND_FASTCALL zim_InternalIterator_next(zend_execute_data *execute_data, zval *return_value);
ZEND_METHOD(InternalIterator, next);
// void ZEND_FASTCALL zim_InternalIterator_valid(zend_execute_data *execute_data, zval *return_value);
ZEND_METHOD(InternalIterator, valid);
// void ZEND_FASTCALL zim_InternalIterator_rewind(zend_execute_data *execute_data, zval *return_value);
ZEND_METHOD(InternalIterator, rewind);

// ZEND_FE_END 表示一个空函数
// #define ZEND_FE_END { NULL, NULL, NULL, 0, 0 }

// ing4, Traversable 接口里一个有效方法也没有
static const zend_function_entry class_Traversable_methods[] = {
	// 一个空函数 
	ZEND_FE_END
};

// ing3, 迭代器聚合 接口中 
static const zend_function_entry class_IteratorAggregate_methods[] = {
	// 添加 带标记的抽象类方法
	ZEND_ABSTRACT_ME_WITH_FLAGS(IteratorAggregate, getIterator, arginfo_class_IteratorAggregate_getIterator, ZEND_ACC_PUBLIC|ZEND_ACC_ABSTRACT)
	// 一个空函数 
	ZEND_FE_END
};

// ing3, Iterator 接口添加方法
static const zend_function_entry class_Iterator_methods[] = {
	// 当前的 value
	ZEND_ABSTRACT_ME_WITH_FLAGS(Iterator, current, arginfo_class_Iterator_current, ZEND_ACC_PUBLIC|ZEND_ACC_ABSTRACT)
	// 下一个
	ZEND_ABSTRACT_ME_WITH_FLAGS(Iterator, next, arginfo_class_Iterator_next, ZEND_ACC_PUBLIC|ZEND_ACC_ABSTRACT)
	// 取得当前 key
	ZEND_ABSTRACT_ME_WITH_FLAGS(Iterator, key, arginfo_class_Iterator_key, ZEND_ACC_PUBLIC|ZEND_ACC_ABSTRACT)
	// 验证有效
	ZEND_ABSTRACT_ME_WITH_FLAGS(Iterator, valid, arginfo_class_Iterator_valid, ZEND_ACC_PUBLIC|ZEND_ACC_ABSTRACT)
	// 倒回
	ZEND_ABSTRACT_ME_WITH_FLAGS(Iterator, rewind, arginfo_class_Iterator_rewind, ZEND_ACC_PUBLIC|ZEND_ACC_ABSTRACT)
	// 一个空函数 
	ZEND_FE_END
};

// ing4, ArrayAccess 接口添加方法
static const zend_function_entry class_ArrayAccess_methods[] = {
	// 索引号是否存在
	ZEND_ABSTRACT_ME_WITH_FLAGS(ArrayAccess, offsetExists, arginfo_class_ArrayAccess_offsetExists, ZEND_ACC_PUBLIC|ZEND_ACC_ABSTRACT)
	// 读取
	ZEND_ABSTRACT_ME_WITH_FLAGS(ArrayAccess, offsetGet, arginfo_class_ArrayAccess_offsetGet, ZEND_ACC_PUBLIC|ZEND_ACC_ABSTRACT)
	// 定入
	ZEND_ABSTRACT_ME_WITH_FLAGS(ArrayAccess, offsetSet, arginfo_class_ArrayAccess_offsetSet, ZEND_ACC_PUBLIC|ZEND_ACC_ABSTRACT)
	// 删除
	ZEND_ABSTRACT_ME_WITH_FLAGS(ArrayAccess, offsetUnset, arginfo_class_ArrayAccess_offsetUnset, ZEND_ACC_PUBLIC|ZEND_ACC_ABSTRACT)
	// 一个空函数 
	ZEND_FE_END
};

// ing4, Serializable 接口添加方法
static const zend_function_entry class_Serializable_methods[] = {
	// 序列化
	ZEND_ABSTRACT_ME_WITH_FLAGS(Serializable, serialize, arginfo_class_Serializable_serialize, ZEND_ACC_PUBLIC|ZEND_ACC_ABSTRACT)
	// 反序列化
	ZEND_ABSTRACT_ME_WITH_FLAGS(Serializable, unserialize, arginfo_class_Serializable_unserialize, ZEND_ACC_PUBLIC|ZEND_ACC_ABSTRACT)
	// 一个空函数 
	ZEND_FE_END
};

// ing4, Countable 接口添加方法
static const zend_function_entry class_Countable_methods[] = {
	// 计数
	ZEND_ABSTRACT_ME_WITH_FLAGS(Countable, count, arginfo_class_Countable_count, ZEND_ACC_PUBLIC|ZEND_ACC_ABSTRACT)
	// 一个空函数 
	ZEND_FE_END
};

// ing4, Stringable 接口添加方法
static const zend_function_entry class_Stringable_methods[] = {
	// 转字串
	ZEND_ABSTRACT_ME_WITH_FLAGS(Stringable, __toString, arginfo_class_Stringable___toString, ZEND_ACC_PUBLIC|ZEND_ACC_ABSTRACT)
	// 一个空函数 
	ZEND_FE_END
};


// ing3, InternalIterator 接口添加方法
static const zend_function_entry class_InternalIterator_methods[] = {
	// 构造方法
	ZEND_ME(InternalIterator, __construct, arginfo_class_InternalIterator___construct, ZEND_ACC_PRIVATE)
	// 当前的value
	ZEND_ME(InternalIterator, current, arginfo_class_InternalIterator_current, ZEND_ACC_PUBLIC)
	// 键
	ZEND_ME(InternalIterator, key, arginfo_class_InternalIterator_key, ZEND_ACC_PUBLIC)
	// 下一个元素
	ZEND_ME(InternalIterator, next, arginfo_class_InternalIterator_next, ZEND_ACC_PUBLIC)
	// 是否可用
	ZEND_ME(InternalIterator, valid, arginfo_class_InternalIterator_valid, ZEND_ACC_PUBLIC)
	// 复位
	ZEND_ME(InternalIterator, rewind, arginfo_class_InternalIterator_rewind, ZEND_ACC_PUBLIC)
	// 一个空函数 
	ZEND_FE_END
};

// ing4, 注册 Traversable 接口, 并添加成员方法
static zend_class_entry *register_class_Traversable(void)
{
	zend_class_entry ce, *class_entry;
	// 用给定的名字创建新类，并添加成员方法列表
	INIT_CLASS_ENTRY(ce, "Traversable", class_Traversable_methods);
	class_entry = zend_register_internal_interface(&ce);

	return class_entry;
}

// ing3, 注册 IteratorAggregate 接口, 并添加成员方法
static zend_class_entry *register_class_IteratorAggregate(zend_class_entry *class_entry_Traversable)
{
	zend_class_entry ce, *class_entry;

	INIT_CLASS_ENTRY(ce, "IteratorAggregate", class_IteratorAggregate_methods);
	class_entry = zend_register_internal_interface(&ce);
	// 此接口实现 Traversable 接口
	zend_class_implements(class_entry, 1, class_entry_Traversable);

	return class_entry;
}

// ing3, 注册 Iterator 接口, 并添加成员方法
static zend_class_entry *register_class_Iterator(zend_class_entry *class_entry_Traversable)
{
	zend_class_entry ce, *class_entry;

	INIT_CLASS_ENTRY(ce, "Iterator", class_Iterator_methods);
	class_entry = zend_register_internal_interface(&ce);
	// 此接口实现 Traversable 接口
	zend_class_implements(class_entry, 1, class_entry_Traversable);

	return class_entry;
}

// ing3, 注册 ArrayAccess 接口, 并添加成员方法
static zend_class_entry *register_class_ArrayAccess(void)
{
	zend_class_entry ce, *class_entry;

	INIT_CLASS_ENTRY(ce, "ArrayAccess", class_ArrayAccess_methods);
	class_entry = zend_register_internal_interface(&ce);

	return class_entry;
}

// ing3, 注册 Serializable 接口, 并添加成员方法
static zend_class_entry *register_class_Serializable(void)
{
	zend_class_entry ce, *class_entry;

	INIT_CLASS_ENTRY(ce, "Serializable", class_Serializable_methods);
	class_entry = zend_register_internal_interface(&ce);

	return class_entry;
}

// ing3, 注册 Countable 接口, 并添加成员方法
static zend_class_entry *register_class_Countable(void)
{
	zend_class_entry ce, *class_entry;

	INIT_CLASS_ENTRY(ce, "Countable", class_Countable_methods);
	class_entry = zend_register_internal_interface(&ce);

	return class_entry;
}

// ing3, 注册 Stringable 接口, 并添加成员方法
static zend_class_entry *register_class_Stringable(void)
{
	zend_class_entry ce, *class_entry;

	INIT_CLASS_ENTRY(ce, "Stringable", class_Stringable_methods);
	class_entry = zend_register_internal_interface(&ce);

	return class_entry;
}

// ing3, 注册 InternalIterator 接口, 并添加成员方法
static zend_class_entry *register_class_InternalIterator(zend_class_entry *class_entry_Iterator)
{
	zend_class_entry ce, *class_entry;

	INIT_CLASS_ENTRY(ce, "InternalIterator", class_InternalIterator_methods);
	class_entry = zend_register_internal_class_ex(&ce, NULL);
	// 添加 final, 不可序列化，标记
	class_entry->ce_flags |= ZEND_ACC_FINAL|ZEND_ACC_NOT_SERIALIZABLE;
	// 此接口实现 class_entry_Iterator 接口
	zend_class_implements(class_entry, 1, class_entry_Iterator);

	return class_entry;
}
