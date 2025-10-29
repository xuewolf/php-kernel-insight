/* This is a generated file, edit the .stub.php file instead.
 * Stub hash: 4cf2c620393f468968a219b5bd12a2b5f6b03ecc */


// 创建 参数信息列表 ，增加第一个 is_tentative 为0 的 参数（返回值）。
// 传入：变量名，是否引用返回，必要的参数数，类型，是否允null
// ing3, 创建 Throwable::getMessage 方法的参数列表。返回值类型，字串, 无其他参数。
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Throwable_getMessage, 0, 0, IS_STRING, 0)
// 无其他参数。
ZEND_END_ARG_INFO()

// 创建参数信息列表 ，增加第一个参数（返回值）。（大量引用）
// 传入：变量名，_unused？，是否引用返回，必要的参数数
// ing2, 创建 Throwable::getCode 方法的参数列表。返回值类型，未定义 , 无其他参数。
ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Throwable_getCode, 0, 0, 0)
// 无其他参数。
ZEND_END_ARG_INFO()

#define arginfo_class_Throwable_getFile arginfo_class_Throwable_getMessage


// 创建 参数信息列表 ，增加第一个 is_tentative 为0 的 参数（返回值）。
// 传入：变量名，是否引用返回，必要的参数数，类型，是否允null
// ing3, 创建 Throwable::getLine 方法的参数列表。返回值类型，整数 。
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Throwable_getLine, 0, 0, IS_LONG, 0)
// 无其他参数。
ZEND_END_ARG_INFO()

// 创建 参数信息列表 ，增加第一个 is_tentative 为0 的 参数（返回值）。
// 传入：变量名，是否引用返回，必要的参数数，类型，是否允null
// ing3, 创建 Throwable::getTrace 方法的参数列表。返回值类型，数组 。 无其他参数。
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Throwable_getTrace, 0, 0, IS_ARRAY, 0)
// 无其他参数。
ZEND_END_ARG_INFO()

// 创建参数信息列表 ，增加第一个参数（返回值）。
// 传入参数名，是否引用返回，必要参数数量 ，返回类型，is_tentative_return_type
// ing3, 创建 Throwable::getPrevious 方法的参数列表。返回值类型，Throwable 。 无其他参数。
ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_class_Throwable_getPrevious, 0, 0, Throwable, 1)
// 无其他参数。
ZEND_END_ARG_INFO()

#define arginfo_class_Throwable_getTraceAsString arginfo_class_Throwable_getMessage

// 创建 参数信息列表 ，增加第一个 is_tentative 为0 的 参数（返回值）。
// 传入：变量名，是否引用返回，必要的参数数，类型，是否允null
// ing3, 创建 Exception::__clone 方法的参数列表。返回值类型void 。无其他参数。
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Exception___clone, 0, 0, IS_VOID, 0)
// 无其他参数。
ZEND_END_ARG_INFO()


// 创建参数信息列表 ，增加第一个参数（返回值）。
// 传入：变量名，_unused？，是否引用返回，必要的参数数
// ing2, 创建 Exception::__construct 方法的参数列表。返回值类型，未定义 , 无其他参数。
ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Exception___construct, 0, 0, 0)
	// 有默认值的 参数信息：可引用传递，参数名称，类型，可以是null, 有默认值
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, message, IS_STRING, 0, "\"\"")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, code, IS_LONG, 0, "0")
	ZEND_ARG_OBJ_INFO_WITH_DEFAULT_VALUE(0, previous, Throwable, 1, "null")
ZEND_END_ARG_INFO()

// 创建 参数信息列表 ，增加第一个 is_tentative 为1 的 参数（返回值）。
// 传入：变量名，是否引用返回，必要的参数数，类型，是否允null
// ing3, 创建 Exception::__wakeup 方法的参数列表。返回值类型void 。 无其他参数。
ZEND_BEGIN_ARG_WITH_TENTATIVE_RETURN_TYPE_INFO_EX(arginfo_class_Exception___wakeup, 0, 0, IS_VOID, 0)
// 无其他参数。
ZEND_END_ARG_INFO()

// 定义相似的参数列表
#define arginfo_class_Exception_getMessage arginfo_class_Throwable_getMessage

#define arginfo_class_Exception_getCode arginfo_class_Throwable_getCode

#define arginfo_class_Exception_getFile arginfo_class_Throwable_getMessage

#define arginfo_class_Exception_getLine arginfo_class_Throwable_getLine

#define arginfo_class_Exception_getTrace arginfo_class_Throwable_getTrace

#define arginfo_class_Exception_getPrevious arginfo_class_Throwable_getPrevious

#define arginfo_class_Exception_getTraceAsString arginfo_class_Throwable_getMessage

#define arginfo_class_Exception___toString arginfo_class_Throwable_getMessage

// 创建参数信息列表 ，增加第一个参数（返回值）。
// 传入：变量名，_unused？，是否引用返回，必要的参数数
// ing2, 创建 Exception::__construct 方法的参数列表。返回值类型，未定义 , 无其他参数。
ZEND_BEGIN_ARG_INFO_EX(arginfo_class_ErrorException___construct, 0, 0, 0)
	// 有默认值的 参数信息：可引用传递，参数名称，类型，可以是null, 有默认值
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, message, IS_STRING, 0, "\"\"")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, code, IS_LONG, 0, "0")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, severity, IS_LONG, 0, "E_ERROR")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, filename, IS_STRING, 1, "null")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, line, IS_LONG, 1, "null")
	// 定义有默认值的对象参数。传入参数：可否引用传递,参数名，类名，是否允null，默认值
	ZEND_ARG_OBJ_INFO_WITH_DEFAULT_VALUE(0, previous, Throwable, 1, "null")
ZEND_END_ARG_INFO()

// 定义相似的参数列表
#define arginfo_class_ErrorException_getSeverity arginfo_class_Throwable_getLine

#define arginfo_class_Error___clone arginfo_class_Exception___clone

#define arginfo_class_Error___construct arginfo_class_Exception___construct

#define arginfo_class_Error___wakeup arginfo_class_Exception___wakeup

#define arginfo_class_Error_getMessage arginfo_class_Throwable_getMessage

#define arginfo_class_Error_getCode arginfo_class_Throwable_getCode

#define arginfo_class_Error_getFile arginfo_class_Throwable_getMessage

#define arginfo_class_Error_getLine arginfo_class_Throwable_getLine

#define arginfo_class_Error_getTrace arginfo_class_Throwable_getTrace

#define arginfo_class_Error_getPrevious arginfo_class_Throwable_getPrevious

#define arginfo_class_Error_getTraceAsString arginfo_class_Throwable_getMessage

#define arginfo_class_Error___toString arginfo_class_Throwable_getMessage

// ing3, 定义内置方法。到这里都是抽象方法，因为不会不在子类中实现。
ZEND_METHOD(Exception, __clone);
ZEND_METHOD(Exception, __construct);
ZEND_METHOD(Exception, __wakeup);
ZEND_METHOD(Exception, getMessage);
ZEND_METHOD(Exception, getCode);
ZEND_METHOD(Exception, getFile);
ZEND_METHOD(Exception, getLine);
ZEND_METHOD(Exception, getTrace);
ZEND_METHOD(Exception, getPrevious);
ZEND_METHOD(Exception, getTraceAsString);
ZEND_METHOD(Exception, __toString);
ZEND_METHOD(ErrorException, __construct);
ZEND_METHOD(ErrorException, getSeverity);

// ing3, Throwable 类的方法：7个
static const zend_function_entry class_Throwable_methods[] = {
	// ing3, 创建抽象类方法，传入类名，方法名（对应的内置C函数名是NULL），参数信息，标识 参数信息 4个参数
	ZEND_ABSTRACT_ME_WITH_FLAGS(Throwable, getMessage, arginfo_class_Throwable_getMessage, ZEND_ACC_PUBLIC|ZEND_ACC_ABSTRACT)
	ZEND_ABSTRACT_ME_WITH_FLAGS(Throwable, getCode, arginfo_class_Throwable_getCode, ZEND_ACC_PUBLIC|ZEND_ACC_ABSTRACT)
	ZEND_ABSTRACT_ME_WITH_FLAGS(Throwable, getFile, arginfo_class_Throwable_getFile, ZEND_ACC_PUBLIC|ZEND_ACC_ABSTRACT)
	ZEND_ABSTRACT_ME_WITH_FLAGS(Throwable, getLine, arginfo_class_Throwable_getLine, ZEND_ACC_PUBLIC|ZEND_ACC_ABSTRACT)
	ZEND_ABSTRACT_ME_WITH_FLAGS(Throwable, getTrace, arginfo_class_Throwable_getTrace, ZEND_ACC_PUBLIC|ZEND_ACC_ABSTRACT)
	ZEND_ABSTRACT_ME_WITH_FLAGS(Throwable, getPrevious, arginfo_class_Throwable_getPrevious, ZEND_ACC_PUBLIC|ZEND_ACC_ABSTRACT)
	ZEND_ABSTRACT_ME_WITH_FLAGS(Throwable, getTraceAsString, arginfo_class_Throwable_getTraceAsString, ZEND_ACC_PUBLIC|ZEND_ACC_ABSTRACT)
	ZEND_FE_END
};

// Error 和 Exception有啥不一样？
// ing3, Exception 类的方法：10个
static const zend_function_entry class_Exception_methods[] = {
	// ing3, 创建类方法，传入类名，方法名（自动生成内置C函数名）， 参数信息，标识 4个参数
	ZEND_ME(Exception, __clone, arginfo_class_Exception___clone, ZEND_ACC_PRIVATE)
	ZEND_ME(Exception, __construct, arginfo_class_Exception___construct, ZEND_ACC_PUBLIC)
	ZEND_ME(Exception, __wakeup, arginfo_class_Exception___wakeup, ZEND_ACC_PUBLIC)
	ZEND_ME(Exception, getMessage, arginfo_class_Exception_getMessage, ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	ZEND_ME(Exception, getCode, arginfo_class_Exception_getCode, ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	ZEND_ME(Exception, getFile, arginfo_class_Exception_getFile, ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	ZEND_ME(Exception, getLine, arginfo_class_Exception_getLine, ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	ZEND_ME(Exception, getTrace, arginfo_class_Exception_getTrace, ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	ZEND_ME(Exception, getPrevious, arginfo_class_Exception_getPrevious, ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	ZEND_ME(Exception, getTraceAsString, arginfo_class_Exception_getTraceAsString, ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	ZEND_ME(Exception, __toString, arginfo_class_Exception___toString, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};

// ing3, ErrorException 类的方法：2个
static const zend_function_entry class_ErrorException_methods[] = {
	// ing3, 创建类方法，传入：类名，方法名（自动生成内置C函数名）， 参数信息，标识 4个参数
	ZEND_ME(ErrorException, __construct, arginfo_class_ErrorException___construct, ZEND_ACC_PUBLIC)
	ZEND_ME(ErrorException, getSeverity, arginfo_class_ErrorException_getSeverity, ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	ZEND_FE_END
};

// ing3, Error 类的方法：11个
static const zend_function_entry class_Error_methods[] = {
	// ing3, 创建类方法，传入：类名，方法别名（自动生成方法名），参数信息，标识 参数信息 4个参数
	ZEND_MALIAS(Exception, __clone, __clone, arginfo_class_Error___clone, ZEND_ACC_PRIVATE)
	ZEND_MALIAS(Exception, __construct, __construct, arginfo_class_Error___construct, ZEND_ACC_PUBLIC)
	// ？__wakeup
	ZEND_MALIAS(Exception, __wakeup, __wakeup, arginfo_class_Error___wakeup, ZEND_ACC_PUBLIC)
	ZEND_MALIAS(Exception, getMessage, getMessage, arginfo_class_Error_getMessage, ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	ZEND_MALIAS(Exception, getCode, getCode, arginfo_class_Error_getCode, ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	ZEND_MALIAS(Exception, getFile, getFile, arginfo_class_Error_getFile, ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	ZEND_MALIAS(Exception, getLine, getLine, arginfo_class_Error_getLine, ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	ZEND_MALIAS(Exception, getTrace, getTrace, arginfo_class_Error_getTrace, ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	ZEND_MALIAS(Exception, getPrevious, getPrevious, arginfo_class_Error_getPrevious, ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	ZEND_MALIAS(Exception, getTraceAsString, getTraceAsString, arginfo_class_Error_getTraceAsString, ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	ZEND_MALIAS(Exception, __toString, __toString, arginfo_class_Error___toString, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};

// ing3, CompileError 类的方法：空
static const zend_function_entry class_CompileError_methods[] = {
	ZEND_FE_END
};

// ing3, ParseError 类的方法：空
static const zend_function_entry class_ParseError_methods[] = {
	ZEND_FE_END
};

// ing3, TypeError 类的方法：空
static const zend_function_entry class_TypeError_methods[] = {
	ZEND_FE_END
};

// ing3, ArgumentCountError 类的方法：空
static const zend_function_entry class_ArgumentCountError_methods[] = {
	ZEND_FE_END
};

// ing3, ValueError 类的方法：空
static const zend_function_entry class_ValueError_methods[] = {
	ZEND_FE_END
};

// ing3, ArithmeticError 类的方法：空
static const zend_function_entry class_ArithmeticError_methods[] = {
	ZEND_FE_END
};


// ing3, DivisionByZeroError 类的方法：空
static const zend_function_entry class_DivisionByZeroError_methods[] = {
	ZEND_FE_END
};


// ing3, UnhandledMatchError 类的方法：空
static const zend_function_entry class_UnhandledMatchError_methods[] = {
	ZEND_FE_END
};

// ing3, 注册接口 Throwable
static zend_class_entry *register_class_Throwable(zend_class_entry *class_entry_Stringable)
{
	zend_class_entry ce, *class_entry;
	// 创建类入口 Throwable
	INIT_CLASS_ENTRY(ce, "Throwable", class_Throwable_methods);
	// 注册成内部 接口
	class_entry = zend_register_internal_interface(&ce);
	// 实现接口 Stringable
	zend_class_implements(class_entry, 1, class_entry_Stringable);

	return class_entry;
}

// ing3, 注册类 Exception
static zend_class_entry *register_class_Exception(zend_class_entry *class_entry_Throwable)
{
	zend_class_entry ce, *class_entry;
	// 创建类 实例
	INIT_CLASS_ENTRY(ce, "Exception", class_Exception_methods);
	// 注册内部类
	class_entry = zend_register_internal_class_ex(&ce, NULL);
	// 实现接口 Throwable
	zend_class_implements(class_entry, 1, class_entry_Throwable);
	// 默认消息为空
	zval property_message_default_value;
	ZVAL_EMPTY_STRING(&property_message_default_value);
	// 属性名 message
	zend_string *property_message_name = zend_string_init("message", sizeof("message") - 1, 1);
	// 添加属性 protected $message="";
	zend_declare_property_ex(class_entry, property_message_name, &property_message_default_value, ZEND_ACC_PROTECTED, NULL);
	// 释放属性名
	zend_string_release(property_message_name);

	// 默认string为空
	zval property_string_default_value;
	ZVAL_EMPTY_STRING(&property_string_default_value);
	// 属性名 string
	zend_string *property_string_name = zend_string_init("string", sizeof("string") - 1, 1);
	// 添加属性 private string $string="";
	zend_declare_typed_property(class_entry, property_string_name, &property_string_default_value, ZEND_ACC_PRIVATE, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_STRING));
	// 释放属性名
	zend_string_release(property_string_name);

	// 默认code为0
	zval property_code_default_value;
	ZVAL_LONG(&property_code_default_value, 0);
	// 属性名 code
	zend_string *property_code_name = zend_string_init("code", sizeof("code") - 1, 1);
	// 添加无类型属性 protected $code=0;
	zend_declare_property_ex(class_entry, property_code_name, &property_code_default_value, ZEND_ACC_PROTECTED, NULL);
	// 释放属性名
	zend_string_release(property_code_name);

	// 默认file为空
	zval property_file_default_value;
	ZVAL_EMPTY_STRING(&property_file_default_value);
	// 属性名 file
	zend_string *property_file_name = zend_string_init("file", sizeof("file") - 1, 1);
	// 添加属性 protected string $file="";
	zend_declare_typed_property(class_entry, property_file_name, &property_file_default_value, ZEND_ACC_PROTECTED, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_STRING));
	// 释放属性名
	zend_string_release(property_file_name);

	// 默认line为0
	zval property_line_default_value;
	ZVAL_LONG(&property_line_default_value, 0);
	zend_string *property_line_name = zend_string_init("line", sizeof("line") - 1, 1);
	// 添加有类型属性 protected int $line=0;
	zend_declare_typed_property(class_entry, property_line_name, &property_line_default_value, ZEND_ACC_PROTECTED, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release(property_line_name);

	// 默认trace为[]
	zval property_trace_default_value;
	ZVAL_EMPTY_ARRAY(&property_trace_default_value);
	zend_string *property_trace_name = zend_string_init("trace", sizeof("trace") - 1, 1);
	// 添加属性 protected array $trace=[];
	zend_declare_typed_property(class_entry, property_trace_name, &property_trace_default_value, ZEND_ACC_PRIVATE, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_ARRAY));
	zend_string_release(property_trace_name);

	// 类名 Throwable
	zend_string *property_previous_class_Throwable = zend_string_init("Throwable", sizeof("Throwable")-1, 1);
	zval property_previous_default_value;
	// previous 默认值null
	ZVAL_NULL(&property_previous_default_value);
	zend_string *property_previous_name = zend_string_init("previous", sizeof("previous") - 1, 1);
	// 添加属性 protected ?Throwable $previous=null; 
	zend_declare_typed_property(class_entry, property_previous_name, &property_previous_default_value, ZEND_ACC_PRIVATE, NULL, (zend_type) ZEND_TYPE_INIT_CLASS(property_previous_class_Throwable, 0, MAY_BE_NULL));
	// 释放名称字串 
	zend_string_release(property_previous_name);

	return class_entry;
}

// ing3 , 注册类 ErrorException，添加属性 severity 值为 1 
static zend_class_entry *register_class_ErrorException(zend_class_entry *class_entry_Exception)
{
	zend_class_entry ce, *class_entry;
	// 创建类入口 ErrorException
	INIT_CLASS_ENTRY(ce, "ErrorException", class_ErrorException_methods);
	// 注册成内部类
	class_entry = zend_register_internal_class_ex(&ce, class_entry_Exception);

	zval property_severity_default_value;
	// 强制默认值为1
	ZVAL_LONG(&property_severity_default_value, E_ERROR);
	// 属性名 severity
	zend_string *property_severity_name = zend_string_init("severity", sizeof("severity") - 1, 1);
	// 声名有类型的属性 previous ，默认值空，private，注释为空，类型 null
	zend_declare_typed_property(class_entry, property_severity_name, &property_severity_default_value, ZEND_ACC_PROTECTED, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	// 释放名称
	zend_string_release(property_severity_name);
	// 返回类实体
	return class_entry;
}

// ing4, 注册类 Error
static zend_class_entry *register_class_Error(zend_class_entry *class_entry_Throwable)
{
	zend_class_entry ce, *class_entry;
	// 初始化类入口  Error
	INIT_CLASS_ENTRY(ce, "Error", class_Error_methods);
	// 注册内部类
	class_entry = zend_register_internal_class_ex(&ce, NULL);
	// 此类实现接口 Throwable。1是接口数量
	zend_class_implements(class_entry, 1, class_entry_Throwable);
	
	// 添加属性 protected $message = "";
	zval property_message_default_value;
	// 消息为空
	ZVAL_EMPTY_STRING(&property_message_default_value);
	// 属性名为 message
	zend_string *property_message_name = zend_string_init("message", sizeof("message") - 1, 1);
	// protected $message = "";
	zend_declare_property_ex(class_entry, property_message_name, &property_message_default_value, ZEND_ACC_PROTECTED, NULL);	
	zend_string_release(property_message_name);

	// 添加属性 private string $string = "";
	zval property_string_default_value;
	ZVAL_EMPTY_STRING(&property_string_default_value);
	zend_string *property_string_name = zend_string_init("string", sizeof("string") - 1, 1);
	// 声名有类型的属性 previous ，默认值空，private，注释为空，类型 null
	zend_declare_typed_property(class_entry, property_string_name, &property_string_default_value, ZEND_ACC_PRIVATE, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_STRING));
	zend_string_release(property_string_name);

	// 添加属性 private $code = 0;
	zval property_code_default_value;
	ZVAL_LONG(&property_code_default_value, 0);
	zend_string *property_code_name = zend_string_init("code", sizeof("code") - 1, 1);
	zend_declare_property_ex(class_entry, property_code_name, &property_code_default_value, ZEND_ACC_PROTECTED, NULL);
	zend_string_release(property_code_name);

	/// 添加属性 private string $file = "";
	zval property_file_default_value;
	ZVAL_EMPTY_STRING(&property_file_default_value);
	zend_string *property_file_name = zend_string_init("file", sizeof("file") - 1, 1);
	// 声名有类型的属性 previous ，默认值空，private，注释为空，类型 null
	zend_declare_typed_property(class_entry, property_file_name, &property_file_default_value, ZEND_ACC_PROTECTED, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_STRING));
	zend_string_release(property_file_name);

	// 添加属性 private int $line = 0;
	zval property_line_default_value;
	ZVAL_UNDEF(&property_line_default_value);
	zend_string *property_line_name = zend_string_init("line", sizeof("line") - 1, 1);
	// 声名有类型的属性 previous ，默认值空，private，注释为空，类型 null
	zend_declare_typed_property(class_entry, property_line_name, &property_line_default_value, ZEND_ACC_PROTECTED, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release(property_line_name);

	// 添加属性 private array $trace = [];
	zval property_trace_default_value;
	ZVAL_EMPTY_ARRAY(&property_trace_default_value);
	zend_string *property_trace_name = zend_string_init("trace", sizeof("trace") - 1, 1);
	// 声名有类型的属性 previous ，默认值空，private，注释为空，类型 null
	zend_declare_typed_property(class_entry, property_trace_name, &property_trace_default_value, ZEND_ACC_PRIVATE, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_ARRAY));
	zend_string_release(property_trace_name);

	// 添加属性 private ?Throwable $previous=null; 
	zend_string *property_previous_class_Throwable = zend_string_init("Throwable", sizeof("Throwable")-1, 1);
	zval property_previous_default_value;
	ZVAL_NULL(&property_previous_default_value);
	zend_string *property_previous_name = zend_string_init("previous", sizeof("previous") - 1, 1);
	// 声名有类型的属性 previous ，默认值空，private，注释为空，类型 null
	zend_declare_typed_property(class_entry, property_previous_name, &property_previous_default_value, ZEND_ACC_PRIVATE, NULL, (zend_type) ZEND_TYPE_INIT_CLASS(property_previous_class_Throwable, 0, MAY_BE_NULL));
	zend_string_release(property_previous_name);

	return class_entry;
}

// ing4, 注册类 CompileError
static zend_class_entry *register_class_CompileError(zend_class_entry *class_entry_Error)
{
	zend_class_entry ce, *class_entry;
	// 创建类入口 CompileError
	INIT_CLASS_ENTRY(ce, "CompileError", class_CompileError_methods);
	class_entry = zend_register_internal_class_ex(&ce, class_entry_Error);

	return class_entry;
}

// ing4, 注册类 ParseError
static zend_class_entry *register_class_ParseError(zend_class_entry *class_entry_CompileError)
{
	zend_class_entry ce, *class_entry;
	// 创建类入口 ParseError
	INIT_CLASS_ENTRY(ce, "ParseError", class_ParseError_methods);
	// 注册成内部类
	class_entry = zend_register_internal_class_ex(&ce, class_entry_CompileError);

	return class_entry;
}

// ing4, 注册类 TypeError
static zend_class_entry *register_class_TypeError(zend_class_entry *class_entry_Error)
{
	zend_class_entry ce, *class_entry;
	// 创建类入口 TypeError
	INIT_CLASS_ENTRY(ce, "TypeError", class_TypeError_methods);
	// 注册成内部类
	class_entry = zend_register_internal_class_ex(&ce, class_entry_Error);

	return class_entry;
}

// ing4, 注册类 ArgumentCountError
static zend_class_entry *register_class_ArgumentCountError(zend_class_entry *class_entry_TypeError)
{
	zend_class_entry ce, *class_entry;
	// 创建类入口 ArgumentCountError
	INIT_CLASS_ENTRY(ce, "ArgumentCountError", class_ArgumentCountError_methods);
	// 注册成内部类
	class_entry = zend_register_internal_class_ex(&ce, class_entry_TypeError);

	return class_entry;
}

// ing4, 注册类 ValueError
static zend_class_entry *register_class_ValueError(zend_class_entry *class_entry_Error)
{
	zend_class_entry ce, *class_entry;
	// 创建类入口 ValueError
	INIT_CLASS_ENTRY(ce, "ValueError", class_ValueError_methods);
	// 注册成内部类
	class_entry = zend_register_internal_class_ex(&ce, class_entry_Error);

	return class_entry;
}

// ing4, 注册类 ArithmeticError
static zend_class_entry *register_class_ArithmeticError(zend_class_entry *class_entry_Error)
{
	zend_class_entry ce, *class_entry;
	// 创建类入口 ArithmeticError
	INIT_CLASS_ENTRY(ce, "ArithmeticError", class_ArithmeticError_methods);
	// 注册成内部类
	class_entry = zend_register_internal_class_ex(&ce, class_entry_Error);

	return class_entry;
}

// ing4, 注册类 DivisionByZeroError
static zend_class_entry *register_class_DivisionByZeroError(zend_class_entry *class_entry_ArithmeticError)
{
	zend_class_entry ce, *class_entry;
	// 创建类入口 DivisionByZeroError
	INIT_CLASS_ENTRY(ce, "DivisionByZeroError", class_DivisionByZeroError_methods);
	// 注册成内部类
	class_entry = zend_register_internal_class_ex(&ce, class_entry_ArithmeticError);

	return class_entry;
}

// ing4, 注册类 UnhandledMatchError
static zend_class_entry *register_class_UnhandledMatchError(zend_class_entry *class_entry_Error)
{
	zend_class_entry ce, *class_entry;
	// 创建类入口 UnhandledMatchError
	INIT_CLASS_ENTRY(ce, "UnhandledMatchError", class_UnhandledMatchError_methods);
	// 注册成内部类
	class_entry = zend_register_internal_class_ex(&ce, class_entry_Error);

	return class_entry;
}
