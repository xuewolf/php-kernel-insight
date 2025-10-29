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
   | Authors: Andi Gutmans <andi@php.net>                                 |
   |          Zeev Suraski <zeev@php.net>                                 |
   |          Dmitry Stogov <dmitry@php.net>                              |
   +----------------------------------------------------------------------+
*/

// zend_object 处理器

#include "zend.h"
#include "zend_globals.h"
#include "zend_variables.h"
#include "zend_API.h"
#include "zend_objects.h"
#include "zend_objects_API.h"
#include "zend_object_handlers.h"
#include "zend_interfaces.h"
#include "zend_exceptions.h"
#include "zend_closures.h"
#include "zend_compile.h"
#include "zend_hash.h"

#define DEBUG_OBJECT_HANDLERS 0

#define ZEND_WRONG_PROPERTY_OFFSET   0

// 保护标记，这几个是用来防止循环操作的
/* guard flags */
// 不可以调用 get 方法
#define IN_GET		(1<<0)
// 不可以调用 set 方法
#define IN_SET		(1<<1)
// 不可以调用 unset 方法
#define IN_UNSET	(1<<2)
// 不可以调用 isset 方法
#define IN_ISSET	(1<<3)


// __X 类型的访问解释：
// 如果有 __get 方法，并且访问不在类属性列表中的元素，会自动调用 __get 方法，如果失败，返回 uninitialized。
// 如果有 __set 方法，并且访问不在类属性列表中的元素，会自动调用 __set 方法，如果失败，不改变类属性列表。
// 对于上面的两个处理器，我们内置了 __get/__set 方法，不用显示调用 __get(),__set()方法，
// 这样可以防止无限递归 并且 允许访问者修改类属性列表
// 如果有 __call 方法，并且调用不在函数列表中的方法，会转到 __call 方法

/*
  __X accessors explanation:

  if we have __get and property that is not part of the properties array is
  requested, we call __get handler. If it fails, we return uninitialized.

  if we have __set and property that is not part of the properties array is
  set, we call __set handler. If it fails, we do not change the array.

  for both handlers above, when we are inside __get/__set, no further calls for
  __get/__set for this property of this object will be made, to prevent endless
  recursion and enable accessors to change properties array.

  if we have __call and method which is not part of the class function table is
  called, we cal __call handler.
*/

// ing4, 创建属性哈希表 ->properties，并添加默认属性 （如果没创建过的话）
ZEND_API void rebuild_object_properties(zend_object *zobj) /* {{{ */
{
	// 没有对象属性表时才执行
	if (!zobj->properties) {
		zend_property_info *prop_info;
		zend_class_entry *ce = zobj->ce;
		int i;
		// 创建属性列表（一开始是顺序哈希表）
		zobj->properties = zend_new_array(ce->default_properties_count);
		// 如果有默认的属性
		if (ce->default_properties_count) {
			// 初始化成 mixed 哈希表
			zend_hash_real_init_mixed(zobj->properties);
			// 遍历所有属性信息
			for (i = 0; i < ce->default_properties_count; i++) {
				// 属性信息
				prop_info = ce->properties_info_table[i];
				// 如果为空，跳过
				if (!prop_info) {
					continue;
				}
				// 如果这个属性信息的类型是 未定义
				if (UNEXPECTED(Z_TYPE_P(OBJ_PROP(zobj, prop_info->offset)) == IS_UNDEF)) {
					// 属性哈希表添加标记：存在空索引
					HT_FLAGS(zobj->properties) |= HASH_FLAG_HAS_EMPTY_IND;
				}
				// 属性哈希表中 参加默认元素
				_zend_hash_append_ind(zobj->properties, prop_info->name,
					OBJ_PROP(zobj, prop_info->offset));
			}
		}
	}
}
/* }}} */

// ing4, 创建属性哈希表并返回，把 properties_table 属性列表里已经存在的属性（指针）存放到哈希表中
// 这里构造的属性 哈希表 不是作为 properties, 而是反回了
ZEND_API HashTable *zend_std_build_object_properties_array(zend_object *zobj) /* {{{ */
{
	zend_property_info *prop_info;
	zend_class_entry *ce = zobj->ce;
	HashTable *ht;
	zval* prop;
	int i;
	// 属性哈希表必须为空
	ZEND_ASSERT(!zobj->properties);
	// 先创建成索引哈希表
	ht = zend_new_array(ce->default_properties_count);
	// 如果有默认的属性
	if (ce->default_properties_count) {
		// 初始化成mixed哈希表
		zend_hash_real_init_mixed(ht);
		// 遍历所有属性信息
		for (i = 0; i < ce->default_properties_count; i++) {
			// 属性信息
			prop_info = ce->properties_info_table[i];
			// 跳过空的
			if (!prop_info) {
				continue;
			}
			// 通过索引号获取属性变量
			prop = OBJ_PROP(zobj, prop_info->offset);
			// 如果是未定义，跳过
			if (UNEXPECTED(Z_TYPE_P(prop) == IS_UNDEF)) {
				continue;
			}
			// 如果属性变量类型是引用，并且被引用过1次
			if (Z_ISREF_P(prop) && Z_REFCOUNT_P(prop) == 1) {
				// 追踪到被引用的变量
				prop = Z_REFVAL_P(prop);
			}
			// 引用次数 +1 
			Z_TRY_ADDREF_P(prop);
			// 把属性添加到属性列表里
			_zend_hash_append(ht, prop_info->name, prop);
		}
	}
	// 返回属性列表
	return ht;
}
/* }}} */

// ing4, 获取 properties 属性列表
ZEND_API HashTable *zend_std_get_properties(zend_object *zobj) /* {{{ */
{
	// properties 如果不存在，创建它
	if (!zobj->properties) {
		// 创建属性哈希表 ->properties，并添加默认属性 （如果没创建过的话）
		rebuild_object_properties(zobj);
	}
	return zobj->properties;
}
/* }}} */

// ing3, 有动态属性表返回动态属性表（直接返回），否则空和默认属性表（引用返回），（多处调用 get_gc）
ZEND_API HashTable *zend_std_get_gc(zend_object *zobj, zval **table, int *n) /* {{{ */
{
	// 如果获取属性方法不是 标准方法
	if (zobj->handlers->get_properties != zend_std_get_properties) {
		// 返回表为null
		*table = NULL;
		// 返回个数为0
		*n = 0;
		// 调用 get_properties ，返回结果
		return zobj->handlers->get_properties(zobj);
	// 如果是标准方法 zend_std_get_properties
	} else {
		// 如果有动态属性表
		if (zobj->properties) {
			// 返回表为空
			*table = NULL;
			// 返回个数为0
			*n = 0;
			// 返回动态属性表
			return zobj->properties;
		// 没有属性表
		} else {
			// 返回列表为属性表
			*table = zobj->properties_table;
			// 返回个数
			*n = zobj->ce->default_properties_count;
			// 返回空
			return NULL;
		}
	}
}
/* }}} */

// ing3, 获取调试信息 p1:对象，p2:返回值是否是临时变量
ZEND_API HashTable *zend_std_get_debug_info(zend_object *object, int *is_temp) /* {{{ */
{
	zend_class_entry *ce = object->ce;
	zval retval;
	HashTable *ht;

	// 如果没有 __debugInfo 方法
	if (!ce->__debugInfo) {
		// 返回值不是临时变量
		*is_temp = 0;
		// zend_std_get_properties
		return object->handlers->get_properties(object);
	}

	// 调用 __debugInfo 方法
	zend_call_known_instance_method_with_0_params(ce->__debugInfo, object, &retval);
	// 如果返回值是 数组 
	if (Z_TYPE(retval) == IS_ARRAY) {
		// 返回值不是可计数类型
		if (!Z_REFCOUNTED(retval)) {
			// 返回值是临时变量
			*is_temp = 1;
			// 返回值数组创建副本，返回副本
			// 访问 zval中的 数组指针, 和 Z_ARR 一毛一样
			return zend_array_dup(Z_ARRVAL(retval));
		// 如果 返回值引用次数在 是1或以下
		} else if (Z_REFCOUNT(retval) <= 1) {
			// 返回值是临时变量
			*is_temp = 1;
			// 访问 zval中的 数组指针
			ht = Z_ARR(retval);
			// 返回数组
			return ht;
		// 其他情况
		} else {
			// 返回不值是临时变量
			*is_temp = 0;
			// 销毁返回值
			zval_ptr_dtor(&retval);
			// 还是把它作为数组返回 ？
			return Z_ARRVAL(retval);
		}
	// 如果返回值是 null
	} else if (Z_TYPE(retval) == IS_NULL) {
		// 返回值是临时变量
		*is_temp = 1;
		// 创建空哈希表
		ht = zend_new_array(0);
		// 返回新哈希表
		return ht;
	}

	// 报错，需要提供错误类型，错误信息格式、参数
	zend_error_noreturn(E_ERROR, ZEND_DEBUGINFO_FUNC_NAME "() must return an array");
	
	// 返回空。编译器不能报错，不要理解成 函数不需要反回值
	return NULL; /* Compilers are dumb and don't understand that noreturn means that the function does NOT need a return value... */
}
/* }}} */

// ing4, 调用 __get 方法
static void zend_std_call_getter(zend_object *zobj, zend_string *prop_name, zval *retval) /* {{{ */
{
	zval member;
	ZVAL_STR(&member, prop_name);
	// 调用已知实例的方法，并传入1个参数
	zend_call_known_instance_method_with_1_params(zobj->ce->__get, zobj, retval, &member);
}
/* }}} */

// ing4, 调用 __set 方法
static void zend_std_call_setter(zend_object *zobj, zend_string *prop_name, zval *value) /* {{{ */
{
	zval args[2];
	ZVAL_STR(&args[0], prop_name);
	ZVAL_COPY_VALUE(&args[1], value);
	// 调用已知实例的方法，并传入2个参数
	zend_call_known_instance_method(zobj->ce->__set, zobj, NULL, 2, args);
}
/* }}} */

// ing4, 调用 __unset 方法
static void zend_std_call_unsetter(zend_object *zobj, zend_string *prop_name) /* {{{ */
{
	zval member;
	ZVAL_STR(&member, prop_name);
	// 调用已知对象的方法，1个附加参数。p1:方法，p2:所属对象:p3，返回结果，p4:参数
	zend_call_known_instance_method_with_1_params(zobj->ce->__unset, zobj, NULL, &member);
}
/* }}} */

// ing4, 调用 __isset 方法
static void zend_std_call_issetter(zend_object *zobj, zend_string *prop_name, zval *retval) /* {{{ */
{
	zval member;
	ZVAL_STR(&member, prop_name);
	// 调用已知对象的方法，1个附加参数。p1:方法，p2:所属对象:p3，返回结果，p4:参数
	zend_call_known_instance_method_with_1_params(zobj->ce->__isset, zobj, retval, &member);
}
/* }}} */

// derive 起源
// ing4, 检验是否存在继承关系（单向）
static zend_always_inline bool is_derived_class(zend_class_entry *child_class, zend_class_entry *parent_class) /* {{{ */
{
	child_class = child_class->parent;
	while (child_class) {
		if (child_class == parent_class) {
			return 1;
		}
		child_class = child_class->parent;
	}

	return 0;
}
/* }}} */

// ing4, 检验是否存在继承关系（双向）
static zend_never_inline int is_protected_compatible_scope(zend_class_entry *ce, zend_class_entry *scope) /* {{{ */
{
	return scope &&
		(is_derived_class(ce, scope) || is_derived_class(scope, ce));
}
/* }}} */

// ing4，获取父类的私有属性信息，不是私有则返回null。ce必须是scope的子类。
static zend_never_inline zend_property_info *zend_get_parent_private_property(zend_class_entry *scope, zend_class_entry *ce, zend_string *member) /* {{{ */
{
	zval *zv;
	zend_property_info *prop_info;
	// 如果域不是当前类，并且域存在，并且ce是scope的子类
	if (scope != ce && scope && is_derived_class(ce, scope)) {
		// 通过名字获取指定域的属性
		zv = zend_hash_find(&scope->properties_info, member);
		// 如果获取到了
		if (zv != NULL) {
			// 获取属性信息
			prop_info = (zend_property_info*)Z_PTR_P(zv);
			// 如果它是私有的，并且属于当前类的
			if ((prop_info->flags & ZEND_ACC_PRIVATE)
			// 私有属性这个验证是必须的	
			 && prop_info->ce == scope) {
				// 返回属性信息
				return prop_info;
			}
		}
	}
	return NULL;
}
/* }}} */

// ing4, 访问属性失败，抛出异常
static ZEND_COLD zend_never_inline void zend_bad_property_access(zend_property_info *property_info, zend_class_entry *ce, zend_string *member) /* {{{ */
{
	zend_throw_error(NULL, "Cannot access %s property %s::$%s", zend_visibility_string(property_info->flags), ZSTR_VAL(ce->name), ZSTR_VAL(member));
}
/* }}} */

// ing4, 抛出异常，无效（空）的属性名（内部调用）
static ZEND_COLD zend_never_inline void zend_bad_property_name(void) /* {{{ */
{
	zend_throw_error(NULL, "Cannot access property starting with \"\\0\"");
}
/* }}} */

// ing4, 抛出异常，创建动态属性失败
static ZEND_COLD zend_never_inline void zend_forbidden_dynamic_property(
		zend_class_entry *ce, zend_string *member) {
	zend_throw_error(NULL, "Cannot create dynamic property %s::$%s",
		ZSTR_VAL(ce->name), ZSTR_VAL(member));
}

// ing4, 报错：弃用的动态属性操作，无引用时删除对象。返回对象是否还有引用
static ZEND_COLD zend_never_inline bool zend_deprecated_dynamic_property(
		zend_object *obj, zend_string *member) {
	GC_ADDREF(obj);
	// 报错：创建操作已弃用
	zend_error(E_DEPRECATED, "Creation of dynamic property %s::$%s is deprecated",
		ZSTR_VAL(obj->ce->name), ZSTR_VAL(member));
	// 如果对象无引用
	if (UNEXPECTED(GC_DELREF(obj) == 0)) {
		zend_class_entry *ce = obj->ce;
		// 删除对象，无引用时释放内存
		zend_objects_store_del(obj);
		// 如果没有异常
		if (!EG(exception)) {
			// 这里不能继续执行，必须要抛出一个异常
			/* We cannot continue execution and have to throw an exception */
			// 抛出异常 无法创建动态属性
			zend_throw_error(NULL, "Cannot create dynamic property %s::$%s",
				ZSTR_VAL(ce->name), ZSTR_VAL(member));
		}
		return 0;
	}
	// 如果对象有引用,返回1
	return 1;
}

// ing4, 抛出异常，无法对只读属性进行 operation 操作
static ZEND_COLD zend_never_inline void zend_readonly_property_modification_scope_error(
		zend_class_entry *ce, zend_string *member, zend_class_entry *scope, const char *operation) {
	zend_throw_error(NULL, "Cannot %s readonly property %s::$%s from %s%s",
		operation, ZSTR_VAL(ce->name), ZSTR_VAL(member),
		scope ? "scope " : "global scope", scope ? ZSTR_VAL(scope->name) : "");
}

// ing4, 抛出异常，无法删除只读属性
static ZEND_COLD zend_never_inline void zend_readonly_property_unset_error(
		zend_class_entry *ce, zend_string *member) {
	zend_throw_error(NULL, "Cannot unset readonly property %s::$%s",
		ZSTR_VAL(ce->name), ZSTR_VAL(member));
}

// ing3, 取得属性在属性列表里的序号。传入：类入口，属性名，是否静默，缓存位置，返回的属性信息指针
// 步骤3 好多疑问。这个方法非常重要，但和继承关系密切。
static zend_always_inline uintptr_t zend_get_property_offset(zend_class_entry *ce, zend_string *member, int silent, void **cache_slot, zend_property_info **info_ptr) /* {{{ */
{
	zval *zv;
	zend_property_info *property_info;
	uint32_t flags;
	zend_class_entry *scope;
	uintptr_t offset;
	
	// ing4, 步骤1，从缓存里快速获取属性信息
	// #define CACHED_PTR_EX(slot) 	(slot)[0]
	// 如果缓存位置存的是当前 类指针，说明 cache_slot 直接可用 （参见 CACHE_POLYMORPHIC_PTR_EX ）
	if (cache_slot && EXPECTED(ce == CACHED_PTR_EX(cache_slot))) {
		// #define CACHED_PTR_EX(slot) (slot)[0]
		// 向后移两个指针，得到属性信息
		*info_ptr = CACHED_PTR_EX(cache_slot + 2);
		// 后移1个，得到偏移量 （参见 CACHE_POLYMORPHIC_PTR_EX ）
		return (uintptr_t)CACHED_PTR_EX(cache_slot + 1);
	}

	// ing4, 步骤2，查询属性表
	// 如果属性哈希表是空的
	if (UNEXPECTED(zend_hash_num_elements(&ce->properties_info) == 0)
		// 或者哈希表里找不到这个成员
	 || UNEXPECTED((zv = zend_hash_find(&ce->properties_info, member)) == NULL)) {
		// 如果成员名字异常
		if (UNEXPECTED(ZSTR_VAL(member)[0] == '\0') && ZSTR_LEN(member) != 0) {
			// 如果没有要求静默
			if (!silent) {
				// 报错
				zend_bad_property_name();
			}
			// 如果要求静默，返回错误：属性偏移位置错误。
			return ZEND_WRONG_PROPERTY_OFFSET;
		}
		// properties_info 表中找不到的算动态属性
// 动态属性
dynamic:
		// 如果有缓存位置，重写缓存位置
		if (cache_slot) {
			// 写入，re指针和,偏移量
			CACHE_POLYMORPHIC_PTR_EX(cache_slot, ce, (void*)ZEND_DYNAMIC_PROPERTY_OFFSET);
			// 第三个位置写入null
			CACHE_PTR_EX(cache_slot + 2, NULL);
		}
		// 返回：动态属性的偏移量（动态属性标记：-1）
		return ZEND_DYNAMIC_PROPERTY_OFFSET;
	}
	
	// 如果正常获取到属性信息
	// 取得属性信息指针
	property_info = (zend_property_info*)Z_PTR_P(zv);
	// 属性信息附加标记
	flags = property_info->flags;

	// ing3, 步骤3，检查作用域
	// 继承时覆盖 或 私有 或 受保护 （ZEND_ACC_CHANGED 只在继承时产生）
	if (flags & (ZEND_ACC_CHANGED|ZEND_ACC_PRIVATE|ZEND_ACC_PROTECTED)) {
		// 获取正在执行的域，如果有fake_scope存在
		if (UNEXPECTED(EG(fake_scope))) {
			// 使用 fack_scope
			scope = EG(fake_scope);
		} else {
			// 从执行数据串中，获取执行时作用域
			scope = zend_get_executed_scope();
		}
		// 关键条件：如果属性所属类 不是 正在执行的域
		if (property_info->ce != scope) {
			// 步骤3.1，如果是覆盖父类的属性。
			if (flags & ZEND_ACC_CHANGED) {
				// 获取父类的私有属性信息，不是私有则返回null。ce必须是scope的子类。
				zend_property_info *p = zend_get_parent_private_property(scope, ce, member);

				// 如果取回的是一个 ce 中的 public/protected 属性，不要尝试 在作用域中使用私有静态属性。
				// 如果两个都是static, 在作用域中，优先使用静态属性。
				// 这样会抛出一个情态属性提示，而不是一个可见的 error。
				/* If there is a public/protected instance property on ce, don't try to use a
				 * private static property on scope. If both are static, prefer the static
				 * property on scope. This will throw a static property notice, rather than
				 * a visibility error. */
				
				// 这个判断是在做什么？
				// 父类中的属性有效 并且 （ 父类中的属性信息没有static修饰 或 当前属性有static修饰 ）
				if (p && (!(p->flags & ZEND_ACC_STATIC) || (flags & ZEND_ACC_STATIC))) {
					// 使用父类的属性信息
					property_info = p;
					// 使用父类的属性信息标记
					flags = property_info->flags;
					// 按找到有效属性操作
					goto found;
				// 其他情况， 如果当前属性有public标记
				} else if (flags & ZEND_ACC_PUBLIC) {
					// 按找到有效属性操作
					goto found;
				}
			}
			// 步骤3.2，如果是 private
			if (flags & ZEND_ACC_PRIVATE) {
				// 如果属性信息不属于 要求的类，（也不属于正在执行的作用域，相当于完全没查到）
				if (property_info->ce != ce) {
					// 按动态属性操作， 返回：动态属性标记：-1
					goto dynamic;
				// 如果属性信息属于 要求的类。但要求的类不是当前正在执行的作用域（参看上一层判断）。
				} else {
// 出错 
wrong:
					// 信息可用，但拒绝访问
					/* Information was available, but we were denied access.  Error out. */
					// 非静默
					if (!silent) {
						// 报错，不可以访问此属性
						zend_bad_property_access(property_info, ce, member);
					}
					// #define ZEND_WRONG_PROPERTY_OFFSET 0
					// 返回错误的 偏移量：0
					return ZEND_WRONG_PROPERTY_OFFSET;
				}
			// 步骤3.3，不是private，肯定是 protected了
			} else {
				// 必须是 protected
				ZEND_ASSERT(flags & ZEND_ACC_PROTECTED);
				// 为什么要检查双向？
				// 检验是否存在继承关系（双向），如果没有
				if (UNEXPECTED(!is_protected_compatible_scope(property_info->ce, scope))) {
					// 按出错处理
					goto wrong;
				}
			}
		}
		// 如果属性信息所属类 是 正在执行的域，直接到这里，按正常找到操作。
	}
// 正常找到
found:
	// ing4, 步骤4，最后的检验
	// 如果是静态属性
	if (UNEXPECTED(flags & ZEND_ACC_STATIC)) {
		// 如果没有要求静默，报错
		if (!silent) {
			// 报错：把静态属性当成非静态属性访问
			zend_error(E_NOTICE, "Accessing static property %s::$%s as non static", ZSTR_VAL(ce->name), ZSTR_VAL(member));
		}
		// 返回动态偏移量： -1
		return ZEND_DYNAMIC_PROPERTY_OFFSET;
	}
	
	// 偏移量
	offset = property_info->offset;
	// 如果不是有效的类型 （左侧25个都是0）
	if (EXPECTED(!ZEND_TYPE_IS_SET(property_info->type))) {
		// 属性信息为null
		property_info = NULL;
	// 如果是有效的类型
	} else {
		// 放到信息指针里返回
		*info_ptr = property_info;
	}
	
	// ing4, 步骤5，更新缓存指针
	// 如果有缓存位置。
	if (cache_slot) {
		// 缓存多态指针 (slot)[0] = (ce); (slot)[1] = (ptr);
		// 前两个放 ce 和 顺序号
		CACHE_POLYMORPHIC_PTR_EX(cache_slot, ce, (void*)(uintptr_t)offset);
		// 把 property_info 指针放到第三个位置
		CACHE_PTR_EX(cache_slot + 2, property_info);
	}
	// 返回offset
	return offset;
}
/* }}} */

// ing4, 通过 zend_get_property_offset 方法来报错， 内部调用
static ZEND_COLD void zend_wrong_offset(zend_class_entry *ce, zend_string *member) /* {{{ */
{
	zend_property_info *dummy;

	// 触发正确的错误
	/* Trigger the correct error */
	// 取得属性在属性列表里的序号。传入：类入口，属性名，是否静默，缓存位置，返回的属性信息指针
	zend_get_property_offset(ce, member, 0, NULL, &dummy);
}
/* }}} */

// ing3,获取属性信息 （zend_property_info，不是属性值）（逻辑比较复杂）
ZEND_API zend_property_info *zend_get_property_info(zend_class_entry *ce, zend_string *member, int silent) /* {{{ */
{
	zval *zv;
	zend_property_info *property_info;
	uint32_t flags;
	zend_class_entry *scope;
	// 如果属性信息列表为空
	if (UNEXPECTED(zend_hash_num_elements(&ce->properties_info) == 0)
	// 或者属性信息列表中不存在这个属性名
	 || EXPECTED((zv = zend_hash_find(&ce->properties_info, member)) == NULL)) {
		 // 如果属性名开头是 \0 并且长度不是0 (说明字串有问题)
		if (UNEXPECTED(ZSTR_VAL(member)[0] == '\0') && ZSTR_LEN(member) != 0) {
			// 如果没有要求静默
			if (!silent) {
				// 抛异常，属性名错误
				zend_bad_property_name();
			}
			// 返回错误信息 属性信息错误
			return ZEND_WRONG_PROPERTY_INFO;
		}
dynamic:
		return NULL;
	}
	// 列表中获取到的属性信息
	property_info = (zend_property_info*)Z_PTR_P(zv);
	// 附加标记
	flags = property_info->flags;
	// 如果有 修改(覆盖)，或私有 或 受保护 三个标记之一
	if (flags & (ZEND_ACC_CHANGED|ZEND_ACC_PRIVATE|ZEND_ACC_PROTECTED)) {
		// 如果有 fake_scope 优先使用 fake_scope，（这是什么）
		if (UNEXPECTED(EG(fake_scope))) {
			scope = EG(fake_scope);
		// 没有fake_scope，使用执行时的scope
		} else {
			// 从执行数据串中，获取执行时作用域
			scope = zend_get_executed_scope();
		}
		// 如果属性信息所属类，与当前作用域不同
		if (property_info->ce != scope) {
			// 如果有 修改(覆盖) 父类信息
			if (flags & ZEND_ACC_CHANGED) {
				// 获取父类的私有属性信息，不是私有则返回null。ce必须是scope的子类。
				zend_property_info *p = zend_get_parent_private_property(scope, ce, member);
				// 如果存在 父类私有属性信息
				if (p) {
					// 应用此私有属性信息
					property_info = p;
					// 应用此私有属性信息的标记
					flags = property_info->flags;
					// 按查找成功操作
					goto found;
				// 如果不存在 父类私有属性信息，并且属性信息有public标记
				} else if (flags & ZEND_ACC_PUBLIC) {
					// 按查找成功操作
					goto found;
				}
			}
			// 如果没有 修改(覆盖) 父类信息
			// 并且 有 private 标记
			if (flags & ZEND_ACC_PRIVATE) {
				// 如果属性信息不属于当前类
				if (property_info->ce != ce) {
					// 按动态属性信息处理
					goto dynamic;
				// 如果属性信息属于当前类
				} else {
wrong:
					// 属性信息
					/* Information was available, but we were denied access.  Error out. */
					// 如果没有要求静默
					if (!silent) {
						// 抛出异常
						zend_bad_property_access(property_info, ce, member);
					}
					// 返回错误状态
					return ZEND_WRONG_PROPERTY_INFO;
				}
			// 如果没有 private 标记
			} else {
				// 必须要有 protected标记
				ZEND_ASSERT(flags & ZEND_ACC_PROTECTED);
				// 如果双向都没有继承关系（protected标记要求有继承关系）
				if (UNEXPECTED(!is_protected_compatible_scope(property_info->ce, scope))) {
					// 报错
					goto wrong;
				}
			}
		}
	}

found:
	// 如果属性信息有 static 标记
	if (UNEXPECTED(flags & ZEND_ACC_STATIC)) {
		// 如果没有要求静默
		if (!silent) {
			// 报错：把静态属性当成非静态属性访问
			zend_error(E_NOTICE, "Accessing static property %s::$%s as non static", ZSTR_VAL(ce->name), ZSTR_VAL(member));
		}
	}
	// 返回属性信息
	return property_info;
}
/* }}} */

// ing3, 从属性信息 检验属性是否可访问 p1:对象，p2:属性名，p3:是否是动态
ZEND_API int zend_check_property_access(zend_object *zobj, zend_string *prop_info_name, bool is_dynamic) /* {{{ */
{
	zend_property_info *property_info;
	const char *class_name = NULL;
	const char *prop_name;
	zend_string *member;
	size_t prop_name_len;

	if (ZSTR_VAL(prop_info_name)[0] == 0) {
		if (is_dynamic) {
			return SUCCESS;
		}
		// 带类名的属性名
		zend_unmangle_property_name_ex(prop_info_name, &class_name, &prop_name, &prop_name_len);
		// 新字串
		member = zend_string_init(prop_name, prop_name_len, 0);
		// 获取属性信息 （zend_property_info，不是属性值）
		property_info = zend_get_property_info(zobj->ce, member, 1);
		// 释放属性名		
		zend_string_release_ex(member, 0);
		// 如果没有属性信息 或 属性信息错误
		if (property_info == NULL || property_info == ZEND_WRONG_PROPERTY_INFO) {
			// 返回失败
			return FAILURE;
		}

		// 如果类型不是 * 开头
		if (class_name[0] != '*') {
			// 如果属性不是 私有
			if (!(property_info->flags & ZEND_ACC_PRIVATE)) {
				// 需要私有属性但找到非私有属性
				/* we we're looking for a private prop but found a non private one of the same name */
				// 返回失败
				return FAILURE;
			// 如果属性名和属性信息名不相同
			} else if (strcmp(ZSTR_VAL(prop_info_name)+1, ZSTR_VAL(property_info->name)+1)) {
				// 查找私有属性，但找到其他类的私有属性
				/* we we're looking for a private prop but found a private one of the same name but another class */
				return FAILURE;
			}
		// 是*开头
		} else {
			ZEND_ASSERT(property_info->flags & ZEND_ACC_PROTECTED);
		}
		return SUCCESS;
	} else {
		// 获取属性信息 （zend_property_info，不是属性值）
		property_info = zend_get_property_info(zobj->ce, prop_info_name, 1);
		// 如果属性信息为空
		if (property_info == NULL) {
			// 必须是动态属性
			ZEND_ASSERT(is_dynamic);
			// 返回成功
			return SUCCESS;
		// 属性信息 为 错误
		} else if (property_info == ZEND_WRONG_PROPERTY_INFO) {
			// 返回错误
			return FAILURE;
		}
		// 如果属性信息是 public 返回成功，否则 失败
		return (property_info->flags & ZEND_ACC_PUBLIC) ? SUCCESS : FAILURE;
	}
}
/* }}} */

// ing3, 它是用来销毁写进哈希表里的 .u2.property_guard ，它是32位整数
// 这个方法是 zend_get_property_guard 放在哈希表里 用作哈希表销毁的方法
static void zend_property_guard_dtor(zval *el) /* {{{ */ {
	// 取得 zval中的指针
	uint32_t *ptr = (uint32_t*)Z_PTR_P(el);
	// 如果指针最后一位不是1（最后1位1是 zend_get_property_guard 里面加的特殊标记）
	if (EXPECTED(!(((zend_uintptr_t)ptr) & 1))) {
		// 释放这4个byte
		efree_size(ptr, sizeof(uint32_t));
	}
}
/* }}} */

// 编译配置选项里必须打开了 ZEND_COMPILE_GUARDS
// ing3, 获取属性保护信息 .u2.property_guard 
ZEND_API uint32_t *zend_get_property_guard(zend_object *zobj, zend_string *member) /* {{{ */
{
	HashTable *guards;
	zval *zv;
	uint32_t *ptr;
	// 类必须有 ZEND_ACC_USE_GUARDS 标记 ( 编译配置选项里必须打开了 ZEND_COMPILE_GUARDS )
	ZEND_ASSERT(zobj->ce->ce_flags & ZEND_ACC_USE_GUARDS);
	// 默认类属性的最后一个
	zv = zobj->properties_table + zobj->ce->default_properties_count;
	// 如果类型是 字串
	if (EXPECTED(Z_TYPE_P(zv) == IS_STRING)) {
		// 取得字串
		zend_string *str = Z_STR_P(zv);
		// 如果字串和 要求的成员名相符 或 内容相同
		if (EXPECTED(str == member) ||
			// 此处不需要为 member 计算好哈希值
		    /* str and member don't necessarily have a pre-calculated hash value here */
		    EXPECTED(zend_string_equal_content(str, member))) {
			// 返回 zv.u2.property_guard，32位整数
			return &Z_PROPERTY_GUARD_P(zv);
		// 如果名称不相符，并且 zv.u2.property_guard 为0 
		} else if (EXPECTED(Z_PROPERTY_GUARD_P(zv) == 0)) {
			// 销毁 zv
			zval_ptr_dtor_str(zv);
			// 使用 member
			ZVAL_STR_COPY(zv, member);
			// 返回 member.u2.property_guard
			return &Z_PROPERTY_GUARD_P(zv);
		// 如果名称不相符，并且 zv.u2.property_guard 不为0 
		} else {
			// 创建保护列表（哈希表）
			ALLOC_HASHTABLE(guards);
			// 初始化 ，并指定 销毁方法
			zend_hash_init(guards, 8, NULL, zend_property_guard_dtor, 0);
			// 使用低位把指针标记成 “特殊”
			/* mark pointer as "special" using low bit */
			// 把指针最后一位写成1，并添加进 保护列表，这里写入的 .u2.property_guard 是32位整数
			// 最后1位是1的，在 zend_property_guard_dtor 里不会删除，会跳过
			zend_hash_add_new_ptr(guards, str,
				(void*)(((zend_uintptr_t)&Z_PROPERTY_GUARD_P(zv)) | 1));
			// 销毁 zv
			zval_ptr_dtor_str(zv);
			// 把 保护列表的地址添加给 zv
			ZVAL_ARR(zv, guards);
		}
	// 如果类型是 数组 
	} else if (EXPECTED(Z_TYPE_P(zv) == IS_ARRAY)) {
		// 取出保护列表
		guards = Z_ARRVAL_P(zv);
		// 保护列表不可以是空
		ZEND_ASSERT(guards != NULL);
		// 在操保护列表里查找此成员
		zv = zend_hash_find(guards, member);
		// 如果找到了
		if (zv != NULL) {
			// 返回成员zval的 指针元素
			return (uint32_t*)(((zend_uintptr_t)Z_PTR_P(zv)) & ~1);
		}
	// 其他类型
	} else {
		// 必须是未定义类型
		ZEND_ASSERT(Z_TYPE_P(zv) == IS_UNDEF);
		// 使用member
		ZVAL_STR_COPY(zv, member);
		// 把 .u2.property_guard 设置成0
		Z_PROPERTY_GUARD_P(zv) = 0;
		// 返回 .u2.property_guard 的指针
		return &Z_PROPERTY_GUARD_P(zv);
	}
	// 必须分开分配 uint32_t,因为 ht->arData 可能已经被分配过了
	/* we have to allocate uint32_t separately because ht->arData may be reallocated */
	ptr = (uint32_t*)emalloc(sizeof(uint32_t));
	// 指针清0
	*ptr = 0;
	// 在保护列表中添加此成员
	return (uint32_t*)zend_hash_add_new_ptr(guards, member, ptr);
}
/* }}} */

// ing3, 读取属性。p1:对象，p2:属性名，p3:类型，p4:缓存位置，p5:返回值（副本）
ZEND_API zval *zend_std_read_property(zend_object *zobj, zend_string *name, int type, void **cache_slot, zval *rv) /* {{{ */
{
	zval *retval;
	uintptr_t property_offset;
	zend_property_info *prop_info = NULL;
	uint32_t *guard = NULL;
	zend_string *tmp_name = NULL;

#if DEBUG_OBJECT_HANDLERS
	fprintf(stderr, "Read object #%d property: %s\n", zobj->handle, ZSTR_VAL(name));
#endif

	// 如果有getter，让它保持静默，可能会用到它
	/* make zend_get_property_info silent if we have getter - we may want to use it */
	// 取得属性在属性列表里的序号。传入：类入口，属性名，是否静默，缓存位置，返回的属性信息指针
	property_offset = zend_get_property_offset(zobj->ce, name, (type == BP_VAR_IS) || (zobj->ce->__get != NULL), cache_slot, &prop_info);
	
	// 情况1，property_offset有效：大于0
	if (EXPECTED(IS_VALID_PROPERTY_OFFSET(property_offset))) {
		// 用顺序号获取对象属性
		retval = OBJ_PROP(zobj, property_offset);
		// 如果返回值有效
		if (EXPECTED(Z_TYPE_P(retval) != IS_UNDEF)) {
			// 如果 是只读属性 
			if (prop_info && UNEXPECTED(prop_info->flags & ZEND_ACC_READONLY)
					// 并且 要求写入 或 更新 或 删除
					&& (type == BP_VAR_W || type == BP_VAR_RW || type == BP_VAR_UNSET)) {
				// 如果返回值 类型是对象
				if (Z_TYPE_P(retval) == IS_OBJECT) {
					// 对于对象，写入，更新，删除 不可以真正修改对象
					// 类似于魔术方法 __get()，但返回值的副本，来保证不会有真正的修改。
					/* For objects, W/RW/UNSET fetch modes might not actually modify object.
					 * Similar as with magic __get() allow them, but return the value as a copy
					 * to make sure no actual modification is possible. */
					// 给返回值创建副本
					ZVAL_COPY(rv, retval);
					// 使用副本返回
					retval = rv;
				// 如果返回值类型不是对象
				} else {
					// 报错：不可以修改只读属性
					zend_readonly_property_modification_error(prop_info);
					// 返回值为未初始化的zval
					retval = &EG(uninitialized_zval);
				}
			}
			// 释放临时名称 并返回 retval 
			goto exit;
		// 如果返回值无效
		} else {
			// 如果 是只读属性 
			if (prop_info && UNEXPECTED(prop_info->flags & ZEND_ACC_READONLY)) {
				// 如果要求写入 或 更新
				if (type == BP_VAR_W || type == BP_VAR_RW) {
					// 报错：不可以修改只读属性
					zend_readonly_property_indirect_modification_error(prop_info);
					// 返回值为未初始化的zval
					retval = &EG(uninitialized_zval);
					// 释放临时名称 并返回 retval
					goto exit;
				// 如果要求删除只读属性（并没有真的删除）
				} else if (type == BP_VAR_UNSET) {
					// 返回 未初始化的zval
					retval = &EG(uninitialized_zval);
					// 释放临时名称 并返回 retval
					goto exit;
				}
			}
		}
		// 如果返回结果是未初始化的属性
		if (UNEXPECTED(Z_PROP_FLAG_P(retval) == IS_PROP_UNINIT)) {
			// 为未初始化类型的属性 跳过 __get()
			/* Skip __get() for uninitialized typed properties */
			// 报错，未初始化的属性
			goto uninit_error;
		}
	// 情况2，如果顺序号无效，并且使用动态顺序号
	} else if (EXPECTED(IS_DYNAMIC_PROPERTY_OFFSET(property_offset))) {
		// 如果对象有属性列表
		if (EXPECTED(zobj->properties != NULL)) {
			// 如果不是未知的动态属性编号（-1）
			if (!IS_UNKNOWN_DYNAMIC_PROPERTY_OFFSET(property_offset)) {
				// 解码成索引号
				uintptr_t idx = ZEND_DECODE_DYN_PROP_OFFSET(property_offset);
				// 如果索引号有效
				if (EXPECTED(idx < zobj->properties->nNumUsed * sizeof(Bucket))) {
					// 找到对应的键值对
					Bucket *p = (Bucket*)((char*)zobj->properties->arData + idx);
					// 如果名称是同一个 zend_string
					if (EXPECTED(p->key == name) ||
						// 或 （ 哈希相符 并且 key 不是 null 并且名称字串相同）
				        (EXPECTED(p->h == ZSTR_H(name)) &&
				         EXPECTED(p->key != NULL) &&
				         EXPECTED(zend_string_equal_content(p->key, name)))) {
						// 反思这个键值对的值
						retval = &p->val;
						// 释放临时名称 并返回 retval
						goto exit;
					}
				}
				// 如果索引号无效，把顺序号更新成动态（ -1）
				CACHE_PTR_EX(cache_slot + 1, (void*)ZEND_DYNAMIC_PROPERTY_OFFSET);
			}
			// 在属性表找到此属性并作为返回值
			retval = zend_hash_find(zobj->properties, name);
			// 如果返回值有效
			if (EXPECTED(retval)) {
				// 如果有 缓存点
				if (cache_slot) {
					// 索引号
					uintptr_t idx = (char*)retval - (char*)zobj->properties->arData;
					// 缓存编码后的动态索引号
					CACHE_PTR_EX(cache_slot + 1, (void*)ZEND_ENCODE_DYN_PROP_OFFSET(idx));
				}
				// 释放临时名称 并返回 retval
				goto exit;
			}
		}
	// 情况3，如果顺序号无效，也没有动态顺序号，并且有 exception
	} else if (UNEXPECTED(EG(exception))) {
		// 返回值是未初始化 zval
		retval = &EG(uninitialized_zval);
		// 释放临时名称 并返回 retval
		goto exit;
	}

	// 魔术方法 __isset。（BP_VAR_IS 对应isset）
	/* magic isset */
	if ((type == BP_VAR_IS) && zobj->ce->__isset) {
		zval tmp_result;
		// 获取属性保护信息 .u2.property_guard 
		guard = zend_get_property_guard(zobj, name);
		// #define IN_ISSET (1<<3)
		// 如果没有防 isset 标记
		if (!((*guard) & IN_ISSET)) {
			// 如果没有临时名称 并且 它不是动态字串
			if (!tmp_name && !ZSTR_IS_INTERNED(name)) {
				// 复制 名称 zend_string
				tmp_name = zend_string_copy(name);
			}
			// 对象引用数 +1
			GC_ADDREF(zobj);
			// 临时结果值为未定义
			ZVAL_UNDEF(&tmp_result);
			// 添加防 isset 标记
			*guard |= IN_ISSET;
			// 调用isset方法
			zend_std_call_issetter(zobj, name, &tmp_result);
			// 删除防 isset 标记（只在调用 zend_std_call_issetter 时标记生效）
			*guard &= ~IN_ISSET;
			// 如果返回值不是true
			if (!zend_is_true(&tmp_result)) {
				// 返回值是未初始化 zval
				retval = &EG(uninitialized_zval);
				// 释放 对象
				OBJ_RELEASE(zobj);
				// 销毁临时结果 
				zval_ptr_dtor(&tmp_result);
				// 释放临时名称 并返回 retval
				goto exit;
			}
			// 销毁临时结果 
			zval_ptr_dtor(&tmp_result);
			// 如果 有 __get 并没 没有 防 get 标记
			if (zobj->ce->__get && !((*guard) & IN_GET)) {
				// 调用 get逻辑
				goto call_getter;
			}
			// 释放对象
			OBJ_RELEASE(zobj);
		// 如果有 __get 并没没有防 get 标记
		} else if (zobj->ce->__get && !((*guard) & IN_GET)) {
			// 调用 获取地址逻辑
			goto call_getter_addref;
		}
	// 魔术方法 __get
	} else if (zobj->ce->__get) {
		/* magic get */
		// 获取属性保护信息 .u2.property_guard 
		guard = zend_get_property_guard(zobj, name);
		// 如果有防 get 标记
		if (!((*guard) & IN_GET)) {
			// 如果有 getter 试用它
			/* have getter - try with it! */
call_getter_addref:
			// 对象增加引用次数
			GC_ADDREF(zobj);
call_getter:
			// 添加防 get 标记。防止循环读取
			*guard |= IN_GET; /* prevent circular getting */
			// 调用标准get方法
			zend_std_call_getter(zobj, name, rv);
			// 删除防 get 标记
			*guard &= ~IN_GET;
			// 如果结果有效
			if (Z_TYPE_P(rv) != IS_UNDEF) {
				// 添加到返回结果
				retval = rv;
				// 如果是引用类型
				if (!Z_ISREF_P(rv) &&
					// 并且要求 写入，更新，删除
				    (type == BP_VAR_W || type == BP_VAR_RW  || type == BP_VAR_UNSET)) {
					// 如果返回值是对象
					if (UNEXPECTED(Z_TYPE_P(rv) != IS_OBJECT)) {
						// 报错：间接修改 超载的属性是无效的
						zend_error(E_NOTICE, "Indirect modification of overloaded property %s::$%s has no effect", ZSTR_VAL(zobj->ce->name), ZSTR_VAL(name));
					}
				}
			// 如果结果无效
			} else {
				// 返回值是未初始化 zval
				retval = &EG(uninitialized_zval);
			}
			// 如果有属性信息
			if (UNEXPECTED(prop_info)) {
				// zend_execute.c ，卡住了
				zend_verify_prop_assignable_by_ref(prop_info, retval, (zobj->ce->__get->common.fn_flags & ZEND_ACC_STRICT_TYPES) != 0);
			}
			// 释放对象
			OBJ_RELEASE(zobj);
			// 释放临时名称 并返回 retval
			goto exit;
		// 如果偏移量无效
		} else if (UNEXPECTED(IS_WRONG_PROPERTY_OFFSET(property_offset))) {
			/* Trigger the correct error */
			// 取得属性在属性列表里的序号。传入：类入口，属性名，是否静默，缓存位置，返回的属性信息指针
			zend_get_property_offset(zobj->ce, name, 0, NULL, &prop_info);
			ZEND_ASSERT(EG(exception));
			// 返回值是未初始化 zval
			retval = &EG(uninitialized_zval);
			// 释放临时名称 并返回 retval
			goto exit;
		}
	}

// 属性未初始化时的报错
uninit_error:
	// 如果不是isset
	if (type != BP_VAR_IS) {
		// 如果有属性信息
		if (UNEXPECTED(prop_info)) {
			// 报错：此类型不能在初始化前 被访问
			zend_throw_error(NULL, "Typed property %s::$%s must not be accessed before initialization",
				ZSTR_VAL(prop_info->ce->name),
				ZSTR_VAL(name));
		// 没有属性信息
		} else {
			// 报错，未定义的属性
			zend_error(E_WARNING, "Undefined property: %s::$%s", ZSTR_VAL(zobj->ce->name), ZSTR_VAL(name));
		}
	}
	// 返回值是未初始化 zval
	retval = &EG(uninitialized_zval);
// 释放临时名称 并返回 retval
exit:
	// 释放临时名称
	zend_tmp_string_release(tmp_name);
	// 正常返回
	return retval;
}
/* }}} */

// ing4, 验证，当前调用的函数 的参数是否使用严格类型限制
static zend_always_inline bool property_uses_strict_types(void) {
	// 当前执行数据
	zend_execute_data *execute_data = EG(current_execute_data);
	// 如果当前在函数中，并且有函数，并且
	return execute_data
		&& execute_data->func
		// ing4, 此调用函数 的参数是否使用严格类型限制
		&& ZEND_CALL_USES_STRICT_TYPES(EG(current_execute_data));
}

// ing3, 检验是否有权限初始化 只读属性（属性属于当前域 或 ce是当前域的子类）
static bool verify_readonly_initialization_access(
		zend_property_info *prop_info, zend_class_entry *ce,
		zend_string *name, const char *operation) {
	// 
	zend_class_entry *scope;
	// 如果有临时执行域
	if (UNEXPECTED(EG(fake_scope))) {
		// 使用临时执行域
		scope = EG(fake_scope);
	// 如果没有
	} else {
		// 从执行数据串中，获取执行时作用域
		scope = zend_get_executed_scope();
	}
	// 如果属性信息所属类 和当前域相同
	if (prop_info->ce == scope) {
		// 返回成功
		return true;
	}

	// 可能已经重新声名了父类属性。这种情况下，还是要允许父类初始化它
	/* We may have redeclared a parent property. In that case the parent should still be
	 * allowed to initialize it. */
	// 如果当前域有效 并且是 ce是当前域的子类
	if (scope && is_derived_class(ce, scope)) {
		// 取得属性信息
		zend_property_info *prop_info = zend_hash_find_ptr(&scope->properties_info, name);
		// 如果获取成功
		if (prop_info) {
			// 这个可以通过继承保证
			/* This should be ensured by inheritance. */
			// 属性必须是 只读
			ZEND_ASSERT(prop_info->flags & ZEND_ACC_READONLY);
			// 如果属性信息所属类 和当前域相同
			if (prop_info->ce == scope) {
				// 返回成功
				return true;
			}
		}
	}
	
	// 抛出异常，无法对只读属性进行 operation 操作
	zend_readonly_property_modification_scope_error(prop_info->ce, name, scope, operation);
	// 返回失败
	return false;
}

// 130行
// ing3, 写入属性，p1:对象，p2:属性名，p3:属性值，p4:缓存位置
ZEND_API zval *zend_std_write_property(zend_object *zobj, zend_string *name, zval *value, void **cache_slot) /* {{{ */
{
	// 临时变量
	zval *variable_ptr, tmp;
	// 属性序号
	uintptr_t property_offset;
	// 属性信息
	zend_property_info *prop_info = NULL;
	// 不可以是引用类型
	ZEND_ASSERT(!Z_ISREF_P(value));
	// 取得属性在属性列表里的序号。传入：类入口，属性名，是否静默，缓存位置，返回的属性信息指针
	property_offset = zend_get_property_offset(zobj->ce, name, (zobj->ce->__set != NULL), cache_slot, &prop_info);
	// 有效：大于0
	if (EXPECTED(IS_VALID_PROPERTY_OFFSET(property_offset))) {
		// 通过偏移量获取指定的类属性
		variable_ptr = OBJ_PROP(zobj, property_offset);
		// 如果值有效
		if (Z_TYPE_P(variable_ptr) != IS_UNDEF) {
			// 值增加引用次数
			Z_TRY_ADDREF_P(value);
			// 如果属性信息有效
			if (UNEXPECTED(prop_info)) {
				// 如果是只读属性
				if (UNEXPECTED(prop_info->flags & ZEND_ACC_READONLY)) {
					// 值减少引用次数
					Z_TRY_DELREF_P(value);
					// 报错：不可以修改只读属性
					zend_readonly_property_modification_error(prop_info);
					// 类型为 错误 的zval
					variable_ptr = &EG(error_zval);
					// 返回 variable_ptr
					goto exit;
				}
				// 不是只读属性，把值复制到临时变量中
				ZVAL_COPY_VALUE(&tmp, value);
				
				// 检查属性值类型 与 属性信息要求的类型是否匹配
				// 验证，当前调用的函数 的参数是否使用严格类型限制
				if (UNEXPECTED(!zend_verify_property_type(prop_info, &tmp, property_uses_strict_types()))) {
					// 减少值的引用次数
					Z_TRY_DELREF_P(value);
					// 结果为 错误
					variable_ptr = &EG(error_zval);
					// 返回 variable_ptr
					goto exit;
				}
				// 使用临时变量
				value = &tmp;
			}

found:
			// 验证，当前调用的函数 的参数是否使用严格类型限制
			variable_ptr = zend_assign_to_variable(
				variable_ptr, value, IS_TMP_VAR, property_uses_strict_types());
			// 返回 variable_ptr
			goto exit;
		}
		// 如果返回值没有初始化过
		if (Z_PROP_FLAG_P(variable_ptr) == IS_PROP_UNINIT) {
			// 通过 __set() 添加到未初始化类型的属性中
			/* Writes to uninitialized typed properties bypass __set(). */
			goto write_std_property;
		}
	// 如果是动态偏移量
	} else if (EXPECTED(IS_DYNAMIC_PROPERTY_OFFSET(property_offset))) {
		// 如果有附加属性表
		if (EXPECTED(zobj->properties != NULL)) {
			// 如果属性表引用次数 > 1
			if (UNEXPECTED(GC_REFCOUNT(zobj->properties) > 1)) {
				// 如果属性表可以更改
				if (EXPECTED(!(GC_FLAGS(zobj->properties) & IS_ARRAY_IMMUTABLE))) {
					// 减少引用次数
					GC_DELREF(zobj->properties);
				}
				// 属性表创建副本
				zobj->properties = zend_array_dup(zobj->properties);
			}
			// 如果附加属性表里找不到此名称的属性
			if ((variable_ptr = zend_hash_find(zobj->properties, name)) != NULL) {
				// 属性值增加引用次数
				Z_TRY_ADDREF_P(value);
				// 按找到操作
				goto found;
			}
		}
	// 如果有异常
	} else if (UNEXPECTED(EG(exception))) {
		// 返回 错误 zval
		variable_ptr = &EG(error_zval);
		// 返回 variable_ptr
		goto exit;
	}

	// 魔术方法
	/* magic set */
	// 如果有 __set 方法
	if (zobj->ce->__set) {
		// 获取属性保护信息 .u2.property_guard 
		uint32_t *guard = zend_get_property_guard(zobj, name);
		// 如果 允许调用 __set方法
		if (!((*guard) & IN_SET)) {
			// 增加引用次数
			GC_ADDREF(zobj);
			// 禁用 __set方法
			(*guard) |= IN_SET; /* prevent circular setting */
			// 调用 __set 方法
			zend_std_call_setter(zobj, name, value);
			// 取消禁用 __set 方法
			(*guard) &= ~IN_SET;
			// 释放对象
			OBJ_RELEASE(zobj);
			// 返回值
			variable_ptr = value;
		// 如果偏移量错误
		} else if (EXPECTED(!IS_WRONG_PROPERTY_OFFSET(property_offset))) {
			// 写入属性
			goto write_std_property;
		// 否则 
		} else {
			/* Trigger the correct error */
			// 通过 zend_get_property_offset 方法来报错
			zend_wrong_offset(zobj->ce, name);
			// 必须有 异常
			ZEND_ASSERT(EG(exception));
			// 返回值为错误 zval
			variable_ptr = &EG(error_zval);
			// 返回 variable_ptr
			goto exit;
		}
	// 没有 __set 方法
	} else {
		// 不能有偏移量错误
		ZEND_ASSERT(!IS_WRONG_PROPERTY_OFFSET(property_offset));
// 定入标准属性
write_std_property:
		// 如果偏移量有效：大于0
		if (EXPECTED(IS_VALID_PROPERTY_OFFSET(property_offset))) {
			// 读取属性，放入返回值中
			variable_ptr = OBJ_PROP(zobj, property_offset);
			// 值增加引用次数
			Z_TRY_ADDREF_P(value);
			// 如果有属性信息
			if (UNEXPECTED(prop_info)) {
				// 如果是只读属性 并且
				if (UNEXPECTED((prop_info->flags & ZEND_ACC_READONLY)
						// 检验是否有权限初始化 只读属性（属性属于当前域 或 ce是当前域的子类）
						&& !verify_readonly_initialization_access(prop_info, zobj->ce, name, "initialize"))) {
					// 尝试减少引用次数
					Z_TRY_DELREF_P(value);
					// 返回结果为错误 zval
					variable_ptr = &EG(error_zval);
					// 返回 variable_ptr
					goto exit;
				}

				ZVAL_COPY_VALUE(&tmp, value);
				// 检查属性值类型 与 属性信息要求的类型是否匹配
				// 验证，当前调用的函数 的参数是否使用严格类型限制
				if (UNEXPECTED(!zend_verify_property_type(prop_info, &tmp, property_uses_strict_types()))) {
					zval_ptr_dtor(value);
					// 返回 variable_ptr
					goto exit;
				}
				// 使用临时变量
				value = &tmp;
				// 清空 variable_ptr 中的标记
				Z_PROP_FLAG_P(variable_ptr) = 0;
				// 按找到操作
				goto found; /* might have been updated via e.g. __toString() */
			}
			// 复制到返回值
			ZVAL_COPY_VALUE(variable_ptr, value);
		// 偏移量无效
		} else {
			// 如果不支持动态属性
			if (UNEXPECTED(zobj->ce->ce_flags & ZEND_ACC_NO_DYNAMIC_PROPERTIES)) {
				// 抛出异常，创建动态属性失败
				zend_forbidden_dynamic_property(zobj->ce, name);
				// 返回值为 错误 zval
				variable_ptr = &EG(error_zval);
				// 返回 variable_ptr
				goto exit;
			}
			// 如果 没有允许动态属性
			if (UNEXPECTED(!(zobj->ce->ce_flags & ZEND_ACC_ALLOW_DYNAMIC_PROPERTIES))) {
				// 报错：弃用的动态属性操作，无引用时删除对象。返回对象是否还有引用
				if (UNEXPECTED(!zend_deprecated_dynamic_property(zobj, name))) {
					// 返回值为 错误 zval
					variable_ptr = &EG(error_zval);
					// 返回 variable_ptr
					goto exit;
				}
			}
			// 尝试增加引用次数
			Z_TRY_ADDREF_P(value);
			// 如果没有附加属性表
			if (!zobj->properties) {
				// 创建属性哈希表 ->properties，并添加默认属性 （如果没创建过的话）
				rebuild_object_properties(zobj);
			}
			// 属性添加进附加属性表
			variable_ptr = zend_hash_add_new(zobj->properties, name, value);
		}
	}
// 返回 variable_ptr
exit:
	return variable_ptr;
}
/* }}} */

// clear, 抛异常：不能把object当成array来访问
static ZEND_COLD zend_never_inline void zend_bad_array_access(zend_class_entry *ce) /* {{{ */
{
	zend_throw_error(NULL, "Cannot use object of type %s as array", ZSTR_VAL(ce->name));
}
/* }}} */

// ing4, 像数组一样读取对象,$c = $a['b']
ZEND_API zval *zend_std_read_dimension(zend_object *object, zval *offset, int type, zval *rv) /* {{{ */
{
	// 所属类
	zend_class_entry *ce = object->ce;
	// 临时偏移量
	zval tmp_offset;

	// 只有实现实了 zend_ce_arrayaccess 接口，才会有 arrayaccess_funcs_ptr
	/* arrayaccess_funcs_ptr is set if (and only if) the class implements zend_ce_arrayaccess */
	zend_class_arrayaccess_funcs *funcs = ce->arrayaccess_funcs_ptr;
	
	// 如果有实现 zend_ce_arrayaccess 接口
	if (EXPECTED(funcs)) {
		// 如果没传递偏移量
		if (offset == NULL) {
			// [] 形式
			/* [] construct */
			// 临时偏移量为null
			ZVAL_NULL(&tmp_offset);
		// 如果有传递偏移量
		} else {
			// 复制引用目标 和 附加信息，并增加引用计数
			ZVAL_COPY_DEREF(&tmp_offset, offset);
		}
		// 对象增加引用次数
		GC_ADDREF(object);
		// 如果操作类型是 isset。isset 操作也是要读取最终结果的，只是在中间加了验证，防止报错
		if (type == BP_VAR_IS) {
			// 重点在这里，先调用 zf_offsetexists 方法验证key是否存在
			// 调用已知对象的方法，1个附加参数。p1:方法，p2:所属对象:p3，返回结果，p4:参数
			zend_call_known_instance_method_with_1_params(funcs->zf_offsetexists, object, rv, &tmp_offset);
			// 如果结果是未定义
			if (UNEXPECTED(Z_ISUNDEF_P(rv))) {
				// 释放对象
				OBJ_RELEASE(object);
				// 销毁临时偏移量
				zval_ptr_dtor(&tmp_offset);
				// 返回空
				return NULL;
			}
			// 如果返回结果 不是 true
			if (!i_zend_is_true(rv)) {
				// 释放对象
				OBJ_RELEASE(object);
				// 销毁临时偏移量
				zval_ptr_dtor(&tmp_offset);
				// 释放查询结果
				zval_ptr_dtor(rv);
				// 返回未初始化 zval
				return &EG(uninitialized_zval);
			}
			// 释放查询结果
			zval_ptr_dtor(rv);
		}
		// 调用已知对象的方法，1个附加参数。p1:方法，p2:所属对象:p3，返回结果，p4:参数
		zend_call_known_instance_method_with_1_params(funcs->zf_offsetget, object, rv, &tmp_offset);
		// 释放对象
		OBJ_RELEASE(object);
		// 销毁临时偏移量
		zval_ptr_dtor(&tmp_offset);
		// 如果结果是 未定义
		if (UNEXPECTED(Z_TYPE_P(rv) == IS_UNDEF)) {
			// 如果没有异常
			if (UNEXPECTED(!EG(exception))) {
				// 报错：A 类型的对象作为数组使用时 此索引号不存在
				zend_throw_error(NULL, "Undefined offset for object of type %s used as array", ZSTR_VAL(ce->name));
			}
			// 返回空
			return NULL;
		}
		// 返回查询到的值
		return rv;
	// 不支持 zend_ce_arrayaccess 接口的都报错
	} else {
		// 抛异常：不能把object当成array来访问
	    zend_bad_array_access(ce);
		// 返回空
		return NULL;
	}
}
/* }}} */

// ing4, 像数组一样写入, $a['b'] = $c;
ZEND_API void zend_std_write_dimension(zend_object *object, zval *offset, zval *value) /* {{{ */
{
	zend_class_entry *ce = object->ce;
	// 临时索引号
	zval tmp_offset;

	// 只有实现实了 zend_ce_arrayaccess 接口，才会有 arrayaccess_funcs_ptr
	zend_class_arrayaccess_funcs *funcs = ce->arrayaccess_funcs_ptr;
	// 如果有实现 zend_ce_arrayaccess 接口
	if (EXPECTED(funcs)) {
		// 如果没有传入索引号
		if (!offset) {
			// 临时索引号为空
			ZVAL_NULL(&tmp_offset);
		} else {
			// 复制引用目标 和 附加信息，并增加引用计数
			ZVAL_COPY_DEREF(&tmp_offset, offset);
		}
		// 对象增加引用次数
		GC_ADDREF(object);
		// 调用已知对象的方法，2个附加参数。p1:方法，p2:所属对象:p3，返回结果，p4,p5:参数
		zend_call_known_instance_method_with_2_params(funcs->zf_offsetset, object, NULL, &tmp_offset, value);
		// 释放对象
		OBJ_RELEASE(object);
		// 释放临时 索引号
		zval_ptr_dtor(&tmp_offset);
	} else {
		// 抛异常：不能把object当成array来访问
	    zend_bad_array_access(ce);
	}
}
/* }}} */

// ing4, 像数组一样确认 key 是否存在，并按要求检查元素值。p1:对象，p2:索引号，p3:是否检查元素empty
ZEND_API int zend_std_has_dimension(zend_object *object, zval *offset, int check_empty) /* {{{ */
{
	// 所属类
	zend_class_entry *ce = object->ce;
	// 临时索引号
	zval retval, tmp_offset;
	// 结果
	int result;
	// 只有实现实了 zend_ce_arrayaccess 接口，才会有 arrayaccess_funcs_ptr
	zend_class_arrayaccess_funcs *funcs = ce->arrayaccess_funcs_ptr;
	// 如果有实现 zend_ce_arrayaccess 接口
	if (EXPECTED(funcs)) {
		// 复制引用目标 和 附加信息，并增加引用计数
		ZVAL_COPY_DEREF(&tmp_offset, offset);
		// 增加对象引用次数
		GC_ADDREF(object);
		// 调用已知对象的方法，1个附加参数。p1:方法，p2:所属对象:p3，返回结果，p4:参数
		zend_call_known_instance_method_with_1_params(funcs->zf_offsetexists, object, &retval, &tmp_offset);
		// 结果是否是true
		result = i_zend_is_true(&retval);
		// 销毁返回值
		zval_ptr_dtor(&retval);
		// 如果需要检验空，并且返回结果为true，并且没有异常
		if (check_empty && result && EXPECTED(!EG(exception))) {
			// 调用 zf_offsetget 方法取出原素
			// 调用已知对象的方法，1个附加参数。p1:方法，p2:所属对象:p3，返回结果，p4:参数
			zend_call_known_instance_method_with_1_params(funcs->zf_offsetget, object, &retval, &tmp_offset);
			// 验证元素是否是true
			result = i_zend_is_true(&retval);
			// 销毁返回值
			zval_ptr_dtor(&retval);
		}
		// 释放对象
		OBJ_RELEASE(object);
		// 销毁临时偏移量
		zval_ptr_dtor(&tmp_offset);
	} else {
		// 抛异常：不能把object当成array来访问
	    zend_bad_array_access(ce);
		// 返回 否
		return 0;
	}
	// 返回结果 
	return result;
}
/* }}} */


// ing3, 取得属性指针的指针，p1:对象，p2:属性名，p3:操作类型，p4:缓存位置
ZEND_API zval *zend_std_get_property_ptr_ptr(zend_object *zobj, zend_string *name, int type, void **cache_slot) /* {{{ */
{
	zval *retval = NULL;
	uintptr_t property_offset;
	zend_property_info *prop_info = NULL;

#if DEBUG_OBJECT_HANDLERS
	fprintf(stderr, "Ptr object #%d property: %s\n", zobj->handle, ZSTR_VAL(name));
#endif
	// 取得属性在属性列表里的序号。传入：类入口，属性名，是否静默，缓存位置，返回的属性信息指针
	property_offset = zend_get_property_offset(zobj->ce, name, (zobj->ce->__get != NULL), cache_slot, &prop_info);
	// 偏移量有效：大于0
	if (EXPECTED(IS_VALID_PROPERTY_OFFSET(property_offset))) {
		// 通过偏移量获取指定对象的类属性值
		retval = OBJ_PROP(zobj, property_offset);
		// 如果值为未定义
		if (UNEXPECTED(Z_TYPE_P(retval) == IS_UNDEF)) {
			// 如果没有 __get 方法 或 不允许调用 __get方法 或 （属性信息有效 并且 retval没有被初始化过）
			if (EXPECTED(!zobj->ce->__get) ||
			    UNEXPECTED((*zend_get_property_guard(zobj, name)) & IN_GET) ||
			    UNEXPECTED(prop_info && Z_PROP_FLAG_P(retval) == IS_PROP_UNINIT)) {
				// 如果操作是 更新 或 读取
				if (UNEXPECTED(type == BP_VAR_RW || type == BP_VAR_R)) {
					// 如果属性信息有效
					if (UNEXPECTED(prop_info)) {
						// 报错：有类型的属性，在访问前必须先初始化
						zend_throw_error(NULL,
							"Typed property %s::$%s must not be accessed before initialization",
							ZSTR_VAL(prop_info->ce->name),
							ZSTR_VAL(name));
						// 返回值为错误 zval
						retval = &EG(error_zval);
					// 属性信息无效
					} else {
						// 返回值为 null
						ZVAL_NULL(retval);
						// 报错：未定义的属性 
						zend_error(E_WARNING, "Undefined property: %s::$%s", ZSTR_VAL(zobj->ce->name), ZSTR_VAL(name));
					}
				// 如果属性是只读的
				} else if (prop_info && UNEXPECTED(prop_info->flags & ZEND_ACC_READONLY)) {
					// 只读属性，委派到 read_property + write_property
					/* Readonly property, delegate to read_property + write_property. */
					// 返回值为null
					retval = NULL;
				}
			// 如果 __get 方法可用
			} else {
				// 有 get方法，失败并且 让它重新尝试 get/set
				/* we do have getter - fail and let it try again with usual get/set */
				retval = NULL;
			}
		// 值为其他类型
		} else if (prop_info && UNEXPECTED(prop_info->flags & ZEND_ACC_READONLY)) {
			// 只读属性，委派到 read_property + write_property
			/* Readonly property, delegate to read_property + write_property. */
			retval = NULL;
		}
	// 动态偏移量
	} else if (EXPECTED(IS_DYNAMIC_PROPERTY_OFFSET(property_offset))) {
		// 如果有动态属性表
		if (EXPECTED(zobj->properties)) {
			// 如果引用数次 >1
			if (UNEXPECTED(GC_REFCOUNT(zobj->properties) > 1)) {
				// 如果不是不可改变
				if (EXPECTED(!(GC_FLAGS(zobj->properties) & IS_ARRAY_IMMUTABLE))) {
					// 减少引用次数
					GC_DELREF(zobj->properties);
				}
				// 附加属性表创建副本
				zobj->properties = zend_array_dup(zobj->properties);
			}
			// 查找属性
		    if (EXPECTED((retval = zend_hash_find(zobj->properties, name)) != NULL)) {
				// 找到了直接返回
				return retval;
		    }
		}
		// 如果没有 __get 方法或 此属性不允许用 __get 方法
		if (EXPECTED(!zobj->ce->__get) ||
		    UNEXPECTED((*zend_get_property_guard(zobj, name)) & IN_GET)) {
			// 如果类没有动态属性	
			if (UNEXPECTED(zobj->ce->ce_flags & ZEND_ACC_NO_DYNAMIC_PROPERTIES)) {
				// 抛出异常，创建动态属性失败
				zend_forbidden_dynamic_property(zobj->ce, name);
				// 返回错误 zval
				return &EG(error_zval);
			}
			// 如果类 允许动态属性
			if (UNEXPECTED(!(zobj->ce->ce_flags & ZEND_ACC_ALLOW_DYNAMIC_PROPERTIES))) {
				// 报错：弃用的动态属性操作，无引用时删除对象。返回对象是否还有引用
				if (UNEXPECTED(!zend_deprecated_dynamic_property(zobj, name))) {
					// 如果没有引用了，返回错误 zval
					return &EG(error_zval);
				}
			}
			// 如果没有动态属性表
			if (UNEXPECTED(!zobj->properties)) {
				// 创建属性哈希表 ->properties，并添加默认属性 （如果没创建过的话）
				rebuild_object_properties(zobj);
			}
			// 更新属性，值为未初始化变量
			retval = zend_hash_update(zobj->properties, name, &EG(uninitialized_zval));
			// 创建属性后抛出警告，避免其他 错误处理器 重写 EG(std_property_info)
			/* Notice is thrown after creation of the property, to avoid EG(std_property_info)
			 * being overwritten in an error handler. */
			// 如果操作类型为 更新或读取
			if (UNEXPECTED(type == BP_VAR_RW || type == BP_VAR_R)) {
				// 报错：未定义的属性
				zend_error(E_WARNING, "Undefined property: %s::$%s", ZSTR_VAL(zobj->ce->name), ZSTR_VAL(name));
			}
		}
	// 偏移量无效
	} else if (zobj->ce->__get == NULL) {
		// 返回值为 错误 zval
		retval = &EG(error_zval);
	}

	// 返回结果
	return retval;
}
/* }}} */

// ing3, 删除属性，p1:对象，p2:属性名，p3:缓存位置
ZEND_API void zend_std_unset_property(zend_object *zobj, zend_string *name, void **cache_slot) /* {{{ */
{
	uintptr_t property_offset;
	zend_property_info *prop_info = NULL;
	// 取得属性在属性列表里的序号。传入：类入口，属性名，是否静默，缓存位置，返回的属性信息指针
	property_offset = zend_get_property_offset(zobj->ce, name, (zobj->ce->__unset != NULL), cache_slot, &prop_info);
	// 如果偏移量有效：大于0
	if (EXPECTED(IS_VALID_PROPERTY_OFFSET(property_offset))) {
		// 取得对象的属性
		zval *slot = OBJ_PROP(zobj, property_offset);
		// 如果值不是未定义
		if (Z_TYPE_P(slot) != IS_UNDEF) {
			// 如果属性是只读的
			if (UNEXPECTED(prop_info && (prop_info->flags & ZEND_ACC_READONLY))) {
				// 抛出异常，无法删除只读属性
				zend_readonly_property_unset_error(prop_info->ce, name);
				// 中断
				return;
			}
			// 如果属性是引用类型 并且 （调试模式 或 检验引用目标是否有效，成功）
			if (UNEXPECTED(Z_ISREF_P(slot)) &&
					(ZEND_DEBUG || ZEND_REF_HAS_TYPE_SOURCES(Z_REF_P(slot)))) {
				// 如果有属性信息
				if (prop_info) {
					// 找到并删除源指针列表中 指向属性信息的指针。p1:源列表，p2:属性信息指针
					ZEND_REF_DEL_TYPE_SOURCE(Z_REF_P(slot), prop_info);
				}
			}
			zval tmp;
			// 属性复制到临时变量里
			ZVAL_COPY_VALUE(&tmp, slot);
			// 属性设置成未定义
			ZVAL_UNDEF(slot);
			// 销毁临时变量
			zval_ptr_dtor(&tmp);
			// 如果有附加属性表
			if (zobj->properties) {
				// 添加标记【包含空索引】
				HT_FLAGS(zobj->properties) |= HASH_FLAG_HAS_EMPTY_IND;
			}
			// 返回
			return;
		}
		// 如果属性未初始化过
		if (UNEXPECTED(Z_PROP_FLAG_P(slot) == IS_PROP_UNINIT)) {
			// 如果有属性信息 并且 属性信息是只读的 并且 
			if (UNEXPECTED(prop_info && (prop_info->flags & ZEND_ACC_READONLY)
					// 检验是否有权限初始化 只读属性（属性属于当前域 或 ce是当前域的子类）
					&& !verify_readonly_initialization_access(prop_info, zobj->ce, name, "unset"))) {
				// 返回
				return;
			}

			// 如果它存在并且是骑过__unset删除过， 重置 IS_PROP_UNINIT 标记
			/* Reset the IS_PROP_UNINIT flag, if it exists and bypass __unset(). */
			Z_PROP_FLAG_P(slot) = 0;
			// 返回
			return;
		}
	// 如果是动态属性 并且 附加属性表不是空
	} else if (EXPECTED(IS_DYNAMIC_PROPERTY_OFFSET(property_offset))
	 && EXPECTED(zobj->properties != NULL)) {
		 // 附加属性表引用数 > 1
		if (UNEXPECTED(GC_REFCOUNT(zobj->properties) > 1)) {
			// 如果附加属性表不是不可更改
			if (EXPECTED(!(GC_FLAGS(zobj->properties) & IS_ARRAY_IMMUTABLE))) {
				// 减少引用次数
				GC_DELREF(zobj->properties);
			}
			// 创建副本
			zobj->properties = zend_array_dup(zobj->properties);
		}
		// 在附加属性表中删除此属性
		if (EXPECTED(zend_hash_del(zobj->properties, name) != FAILURE)) {
			// 返回
			return;
		}
	// 如果有异常
	} else if (UNEXPECTED(EG(exception))) {
		// 返回
		return;
	}

	// 魔术方法 __unset
	/* magic unset */
	if (zobj->ce->__unset) {
		// 获取属性保护信息 .u2.property_guard 
		uint32_t *guard = zend_get_property_guard(zobj, name);
		// 如果没有禁用 unset
		if (!((*guard) & IN_UNSET)) {
			// 有删除器 尝试它
			/* have unseter - try with it! */
			// 添加禁止删除，防止循环删除
			(*guard) |= IN_UNSET; /* prevent circular unsetting */
			// 调用 __unset 方法
			zend_std_call_unsetter(zobj, name);
			// 删除禁用 unset 标记
			(*guard) &= ~IN_UNSET;
		} else if (UNEXPECTED(IS_WRONG_PROPERTY_OFFSET(property_offset))) {
			/* Trigger the correct error */
			// 通过 zend_get_property_offset 方法来报错
			zend_wrong_offset(zobj->ce, name);
			// 必须要有 异常
			ZEND_ASSERT(EG(exception));
			// 返回
			return;
		// 其他情况
		} else {
			// 属性已经不存在了，什么也不用做
			/* Nothing to do: The property already does not exist. */
		}
	}
}
/* }}} */

// ing3, 像数组一样删除,unset($a['b'])
ZEND_API void zend_std_unset_dimension(zend_object *object, zval *offset) /* {{{ */
{
	zend_class_entry *ce = object->ce;
	zval tmp_offset;

	// 只有实现实了 zend_ce_arrayaccess 接口，才会有 arrayaccess_funcs_ptr
	zend_class_arrayaccess_funcs *funcs = ce->arrayaccess_funcs_ptr;
	// 如果有实现 zend_ce_arrayaccess 接口
	if (EXPECTED(funcs)) {
		// 复制引用目标 和 附加信息，并增加引用计数
		ZVAL_COPY_DEREF(&tmp_offset, offset);
		// 添加引用计数
		GC_ADDREF(object);
		// 调用已知实例的方法：zf_offsetunset，并传入1个参数
		zend_call_known_instance_method_with_1_params(funcs->zf_offsetunset, object, NULL, &tmp_offset);
		// 减少object引用次数
		OBJ_RELEASE(object);
		// 删除临时offset
		zval_ptr_dtor(&tmp_offset);
	// 否则报错
	} else {
		// 抛异常：不能把object当成array来访问
	    zend_bad_array_access(ce);
	}
}
/* }}} */

// ing4, 获取父类的私有方法
static zend_never_inline zend_function *zend_get_parent_private_method(zend_class_entry *scope, zend_class_entry *ce, zend_string *function_name) /* {{{ */
{
	zval *func;
	zend_function *fbc;

	// 如果两个域不同，并且 scope 存在， 并且ce继承自scope
	if (scope != ce && scope && is_derived_class(ce, scope)) {
		// 在scope中获取函数
		func = zend_hash_find(&scope->function_table, function_name);
		// 如果获取到
		if (func != NULL) {
			// 取得变量里的 zend_function
			fbc = Z_FUNC_P(func);
			// 如果函数带 private标记， 并且它的作用域是 scope（也有可能继承自其他域）
			if ((fbc->common.fn_flags & ZEND_ACC_PRIVATE)
			 && fbc->common.scope == scope) {
				// 返回找到的私有方法
				return fbc;
			}
		}
	}
	// 返回空
	return NULL;
}
/* }}} */

// ing3, 保证调用可用的 受保护方法（实际上还是检验继承关系）
/* Ensures that we're allowed to call a protected method.
 */
ZEND_API int zend_check_protected(zend_class_entry *ce, zend_class_entry *scope) /* {{{ */
{
	zend_class_entry *fbc_scope = ce;

	// 验证 scope 是否继承自（或等于）ce
	/* Is the context that's calling the function, the same as one of
	 * the function's parents?
	 */
	// 如果域存在
	while (fbc_scope) {
		// 如果匹配，返回1
		if (fbc_scope==scope) {
			return 1;
		}
		// 追溯到 fbc_scope(ce) 父类
		fbc_scope = fbc_scope->parent;
	}
	// ce 不是继承自（或等于）scope

	// 验证 ce 是否继承自（或等于）scope
	/* Is the function's scope the same as our current object context,
	 * or any of the parents of our context?
	 */
	while (scope) {
		// 如果匹配，返回1
		if (scope==ce) {
			return 1;
		}
		// 追溯到 scope 父类
		scope = scope->parent;
	}
	return 0;
}
/* }}} */

// ing2, 创建并返回弹跳方法,静态和普通方法都从这里创建。p1:类，p2:方法名，p3:是否静态
ZEND_API zend_function *zend_get_call_trampoline_func(zend_class_entry *ce, zend_string *method_name, int is_static) /* {{{ */
{
	size_t mname_len;
	zend_op_array *func;
	// 按需要添加静态标记
	zend_function *fbc = is_static ? ce->__callstatic : ce->__call;
	// 使用非NULL值来避免 分配 无用的 运行时缓存 。
	// 低位必须是0，避免被解释成 MAP_PTR 偏移量 ？？
	/* We use non-NULL value to avoid useless run_time_cache allocation.
	 * The low bit must be zero, to not be interpreted as a MAP_PTR offset.
	 */
	// 偏移量为2的未知类型指针
	static const void *dummy = (void*)(intptr_t)2;
	// 空参数
	static const zend_arg_info arg_info[1] = {{0}};

	// 必须要有 fbc
	ZEND_ASSERT(fbc);

	// 如果没有弹跳函数名 （它会隨EG 创建，但什么时候初始化的呢？）
	if (EXPECTED(EG(trampoline).common.function_name == NULL)) {
		// 从操作码中查找函数名
		func = &EG(trampoline).op_array;
	// 有跳转函数名？？？？
	} else {
		// 创建新操作码
		func = ecalloc(1, sizeof(zend_op_array));
	}

	// 类型是用户定义函数 
	func->type = ZEND_USER_FUNCTION;
	// 3个参数标记是 0
	func->arg_flags[0] = 0;
	func->arg_flags[1] = 0;
	func->arg_flags[2] = 0;
	// 函数标记， 弹跳函数 | public | 支持字典参数？
	func->fn_flags = ZEND_ACC_CALL_VIA_TRAMPOLINE | ZEND_ACC_PUBLIC | ZEND_ACC_VARIADIC;
	// 如果是静态调用，添加标记
	if (is_static) {
		func->fn_flags |= ZEND_ACC_STATIC;
	}
	// 弹跳函数的操作码（单个操作码）
	func->opcodes = &EG(call_trampoline_op);
	// ？？
	ZEND_MAP_PTR_INIT(func->run_time_cache, (void**)dummy);
	// 作用域
	func->scope = fbc->common.scope;
	// 为参数、本地和临时变量 留出空间
	/* reserve space for arguments, local and temporary variables */
	// EG(trampoline) 可以在其他地方再使用，例如 在 FFI(例如 zend_ffi_cdata_get_closure()) 里当成内置函数使用。
	// 它可能使用非公共空间，导致修改 zend_op_array 中的特定数据，主要是 last_var。
	// 需要重置这个什，在引擎分配空间给下一个堆栈时， 让它不包含垃圾
	// 通过使用“幸运的”结构布局， 这个至今尚未发生过问题
	/* EG(trampoline) is reused from other places, like FFI (e.g. zend_ffi_cdata_get_closure()) where
	 * it is used as an internal function. It may set fields that don't belong to common, thus
	 * modifying zend_op_array specific data, most significantly last_var. We need to reset this
	 * value so that it doesn't contain garbage when the engine allocates space for the next stack
	 * frame. This didn't cause any issues until now due to "lucky" structure layout. */
	func->last_var = 0;
	// 临时变量数量 
	func->T = (fbc->type == ZEND_USER_FUNCTION)? MAX(fbc->op_array.last_var + fbc->op_array.T, 2) : 2;
	// 函数所属文件名
	func->filename = (fbc->type == ZEND_USER_FUNCTION)? fbc->op_array.filename : ZSTR_EMPTY_ALLOC();
	// 开始行号
	func->line_start = (fbc->type == ZEND_USER_FUNCTION)? fbc->op_array.line_start : 0;
	// 结束行号
	func->line_end = (fbc->type == ZEND_USER_FUNCTION)? fbc->op_array.line_end : 0;

	// 保持兼容 \0 字符
	//??? keep compatibility for "\0" characters
	//??? see: Zend/tests/bug46238.phpt
	// 如果 method_name 字串的长度有问题（字串长度不等于记录的长度）（是因为没初始化过吗？）
	if (UNEXPECTED((mname_len = strlen(ZSTR_VAL(method_name))) != ZSTR_LEN(method_name))) {
		// 重新初始化函数名 字串
		func->function_name = zend_string_init(ZSTR_VAL(method_name), mname_len, 0);
	} else {
		// 使用函数名字串创建副本
		func->function_name = zend_string_copy(method_name);
	}
	// 原型是null
	func->prototype = NULL;
	// 参数数量 0
	func->num_args = 0;
	// 必要参数数量 0
	func->required_num_args = 0;
	// 参数信息
	func->arg_info = (zend_arg_info *) arg_info;

	// 返回新创建的函数实例（zend_op_array）
	return (zend_function*)func;
}
/* }}} */

// ing4, 创建并返回 非静态弹跳方法。p1:类，p2:方法名
static zend_always_inline zend_function *zend_get_user_call_function(zend_class_entry *ce, zend_string *method_name) /* {{{ */
{
	// 创建并返回弹跳方法,静态和普通方法都从这里创建。p1:类，p2:方法名，p3:是否静态
	return zend_get_call_trampoline_func(ce, method_name, 0);
}
/* }}} */

// ing3, 抛出异常：调用方法出错 
static ZEND_COLD zend_never_inline void zend_bad_method_call(zend_function *fbc, zend_string *method_name, zend_class_entry *scope) /* {{{ */
{
	zend_throw_error(NULL, "Call to %s method %s::%s() from %s%s",
		zend_visibility_string(fbc->common.fn_flags), ZEND_FN_SCOPE_NAME(fbc), ZSTR_VAL(method_name),
		scope ? "scope " : "global scope",
		scope ? ZSTR_VAL(scope->name) : ""
	);
}
/* }}} */

// clear，抛出异常：无法调用抽象方法
static ZEND_COLD zend_never_inline void zend_abstract_method_call(zend_function *fbc) /* {{{ */
{
	zend_throw_error(NULL, "Cannot call abstract method %s::%s()",
		ZSTR_VAL(fbc->common.scope->name), ZSTR_VAL(fbc->common.function_name));
}
/* }}} */

// ing3, 获取对象的方法，p1:对象指针的指针，p2:方法名，p3:小写方法名
ZEND_API zend_function *zend_std_get_method(zend_object **obj_ptr, zend_string *method_name, const zval *key) /* {{{ */
{
	zend_object *zobj = *obj_ptr;
	zval *func;
	zend_function *fbc;
	zend_string *lc_method_name;
	zend_class_entry *scope;
	ALLOCA_FLAG(use_heap);

	// 优先使用key
	if (EXPECTED(key != NULL)) {
		// 小写方法名
		lc_method_name = Z_STR_P(key);
#ifdef ZEND_ALLOCA_MAX_SIZE
		use_heap = 0;
#endif
	// 没有key
	} else {
		// 创建小写方法名字串
		ZSTR_ALLOCA_ALLOC(lc_method_name, ZSTR_LEN(method_name), use_heap);
		// 复制小写 method_name
		zend_str_tolower_copy(ZSTR_VAL(lc_method_name), ZSTR_VAL(method_name), ZSTR_LEN(method_name));
	}

	// 获取方法，如果失败
	if (UNEXPECTED((func = zend_hash_find(&zobj->ce->function_table, lc_method_name)) == NULL)) {
		// 如果没有key
		if (UNEXPECTED(!key)) {
			// 释放方法名
			ZSTR_ALLOCA_FREE(lc_method_name, use_heap);
		}
		// 如果有 __call 方法
		if (zobj->ce->__call) {
			// 创建并返回 非静态弹跳方法。p1:类，p2:方法名
			return zend_get_user_call_function(zobj->ce, method_name);
		// 没有 __call 方法
		} else {
			// 返回空
			return NULL;
		}
	}

	// 取得方法
	fbc = Z_FUNC_P(func);

	// 检验访问级别
	/* Check access level */
	// 如果方法有更改 或是私有 或是受保护的
	if (fbc->op_array.fn_flags & (ZEND_ACC_CHANGED|ZEND_ACC_PRIVATE|ZEND_ACC_PROTECTED)) {
		// 从执行数据串中，获取执行时作用域
		scope = zend_get_executed_scope();
		// 如果方法不属于此域
		if (fbc->common.scope != scope) {
			// 如果方法被更改过
			if (fbc->op_array.fn_flags & ZEND_ACC_CHANGED) {
				// 获取父类的私有方法
				zend_function *updated_fbc = zend_get_parent_private_method(scope, zobj->ce, lc_method_name);
				// 如果获取成功
				if (EXPECTED(updated_fbc != NULL)) {
					// 使用此方法
					fbc = updated_fbc;
					// 返回找到的方法。抽象方法抛异常。
					goto exit;
				// 如果 获取失败， 原方法是public 
				} else if (fbc->op_array.fn_flags & ZEND_ACC_PUBLIC) {
					// 返回找到的方法。抽象方法抛异常。
					goto exit;
				}
			}
			// 如果方法是私有的 或
			if (UNEXPECTED(fbc->op_array.fn_flags & ZEND_ACC_PRIVATE)
			// 如果作用域不合法
			// 取得方法的根类：如果有原型（->common.prototype）返回原型的 作用域。否则使用本身的作用域
			 || UNEXPECTED(!zend_check_protected(zend_get_function_root_class(fbc), scope))) {
				// 如果有 __call 方法
				if (zobj->ce->__call) {
					// 创建并返回 非静态弹跳方法。p1:类，p2:方法名
					fbc = zend_get_user_call_function(zobj->ce, method_name);
				// 没有 __call 方法
				} else {
					// 抛出异常：调用方法出错 
					zend_bad_method_call(fbc, method_name, scope);
					// 清空方法
					fbc = NULL;
				}
			}
		}
	}

// 返回找到的方法。抽象方法抛异常。
exit:
	// 如果方法是抽象的
	if (fbc && UNEXPECTED(fbc->common.fn_flags & ZEND_ACC_ABSTRACT)) {
		// 抛出异常：无法调用抽象方法
		zend_abstract_method_call(fbc);
		// 返回方法
		fbc = NULL;
	}
	// 如果没有key
	if (UNEXPECTED(!key)) {
		// 释放小写方法名
		ZSTR_ALLOCA_FREE(lc_method_name, use_heap);
	}
	// 返回方法
	return fbc;
}
/* }}} */

// ing4, 创建并返回 静态弹跳方法。p1:类，p2:方法名
static zend_always_inline zend_function *zend_get_user_callstatic_function(zend_class_entry *ce, zend_string *method_name) /* {{{ */
{
	return zend_get_call_trampoline_func(ce, method_name, 1);
}
/* }}} */

// ing3, 调用不存在的方法，转到调用 __call 或  __callstatic
static zend_always_inline zend_function *get_static_method_fallback(
		zend_class_entry *ce, zend_string *function_name)
{
	zend_object *object;
	// 如果有 __call 方法 
	if (ce->__call &&
		// 并且 （ 存在当前正在执行的对象 并且 对象所属类是当前类 ）
		(object = zend_get_this_object(EG(current_execute_data))) != NULL &&
		instanceof_function(object->ce, ce)) {
		// 调用顶层 __call()
		/* Call the top-level defined __call().
		 * see: tests/classes/__call_004.phpt  */

		// 所属类必须有__call方法
		ZEND_ASSERT(object->ce->__call);
		// 取得用户定义的方法
		return zend_get_user_call_function(object->ce, function_name);
	// 如果有 __callstatic 方法
	} else if (ce->__callstatic) {
		// 创建并返回 静态弹跳方法。p1:类，p2:方法名
		return zend_get_user_callstatic_function(ce, function_name);
	// 如果两个都没有，返回NULL
	} else {
		return NULL;
	}
}

// ing3, 获取静态方法，p1:类，p2:方法名，p3:key
ZEND_API zend_function *zend_std_get_static_method(zend_class_entry *ce, zend_string *function_name, const zval *key) /* {{{ */
{
	// 
	zend_string *lc_function_name;
	// 如果有传入 key
	if (EXPECTED(key != NULL)) {
		// key当成方法名
		lc_function_name = Z_STR_P(key);
	// 没有传入key
	} else {
		// 使用小写方法名
		lc_function_name = zend_string_tolower(function_name);
	}

	// 返回值
	zend_function *fbc;
	// 在方法列表中查找要求的方法
	zval *func = zend_hash_find(&ce->function_table, lc_function_name);
	// 如果找到
	if (EXPECTED(func)) {
		// 取出方法
		fbc = Z_FUNC_P(func);
		// 如果不是 Public
		if (!(fbc->op_array.fn_flags & ZEND_ACC_PUBLIC)) {
			// 从执行数据串中，获取执行时作用域
			zend_class_entry *scope = zend_get_executed_scope();
			// 如果所属域不是当前域
			if (UNEXPECTED(fbc->common.scope != scope)) {
				// 如果 是 private
				if (UNEXPECTED(fbc->op_array.fn_flags & ZEND_ACC_PRIVATE)
				// 取得方法的根类：如果有原型（->common.prototype）返回原型的 作用域。否则使用本身的作用域
				 || UNEXPECTED(!zend_check_protected(zend_get_function_root_class(fbc), scope))) {
					// 调用不存在的方法，转到调用 __call 或  __callstatic 
					zend_function *fallback_fbc = get_static_method_fallback(ce, function_name);
					if (!fallback_fbc) {
						// 抛出异常：调用方法出错 
						zend_bad_method_call(fbc, function_name, scope);
					}
					// 如果有找到需要的方法，放到返回值里
					fbc = fallback_fbc;
				}
			}
		}
	// 如果没找到
	} else {
		// 调用不存在的方法，转到调用 __call 或  __callstatic
		fbc = get_static_method_fallback(ce, function_name);
	}

	// 如果没有传入key
	if (UNEXPECTED(!key)) {
		// 释放小写方法名
		zend_string_release_ex(lc_function_name, 0);
	}

	// 如果有返回方法
	if (EXPECTED(fbc)) {
		// 如果方法是抽象的
		if (UNEXPECTED(fbc->common.fn_flags & ZEND_ACC_ABSTRACT)) {
			// 抛出异常：无法调用抽象方法
			zend_abstract_method_call(fbc);
			// 返回方法为空
			fbc = NULL;
		// 如果方法属于trait
		} else if (UNEXPECTED(fbc->common.scope->ce_flags & ZEND_ACC_TRAIT)) {
			// 报错：调用静态的trait方法 已弃用
			zend_error(E_DEPRECATED,
				"Calling static trait method %s::%s is deprecated, "
				"it should only be called on a class using the trait",
				ZSTR_VAL(fbc->common.scope->name), ZSTR_VAL(fbc->common.function_name));
			// 如果有异常
			if (EG(exception)) {
				// 返回空
				return NULL;
			}
		}
	}
	// 返回找到的方法
	return fbc;
}
/* }}} */

// ing3, 初始化静态成员列表，并把父类的静态成员复制过来
ZEND_API void zend_class_init_statics(zend_class_entry *class_type) /* {{{ */
{
	int i;
	zval *p;
	// 如果有静态成员 并且 返回静态成员表【指针列表】无效
	if (class_type->default_static_members_count && !CE_STATIC_MEMBERS(class_type)) {
		// 如果有父类
		if (class_type->parent) {
			// 递归：初始化静态成员列表，并添加父类的成员
			zend_class_init_statics(class_type->parent);
		}
		// 创建 zval列表并关联到 类
		ZEND_MAP_PTR_SET(class_type->static_members_table, emalloc(sizeof(zval) * class_type->default_static_members_count));
		// 遍历所有静态成员
		for (i = 0; i < class_type->default_static_members_count; i++) {
			// 取出一个元素
			p = &class_type->default_static_members_table[i];
			// 如果是间接引用
			if (Z_TYPE_P(p) == IS_INDIRECT) {
				// 取得父类的此元素（初始化时只处理父类的元素，还没有自己的元素）
				zval *q = &CE_STATIC_MEMBERS(class_type->parent)[i];
				// 解引用
				ZVAL_DEINDIRECT(q);
				// 把此类的元素 更新为 间接引用
				ZVAL_INDIRECT(&CE_STATIC_MEMBERS(class_type)[i], q);
			// 不是间接引用
			} else {
				// 直接把父类的元素复制过来
				ZVAL_COPY_OR_DUP(&CE_STATIC_MEMBERS(class_type)[i], p);
			}
		}
	}
} /* }}} */

// ing3, 标准方法：获取静态成员 和 成员信息。p1:类，p2:属性名，p3:类型，p4:属性信息列表指针
ZEND_API zval *zend_std_get_static_property_with_info(zend_class_entry *ce, zend_string *property_name, int type, zend_property_info **property_info_ptr) /* {{{ */
{
	zval *ret;
	zend_class_entry *scope;
	// 在属性列表里查找属性信息
	zend_property_info *property_info = zend_hash_find_ptr(&ce->properties_info, property_name);
	// 返回属性信息
	*property_info_ptr = property_info;
	// 如果没找到属性信息
	if (UNEXPECTED(property_info == NULL)) {
		// 未定义的属性
		goto undeclared_property;
	}
	// 如果属性没有public标记
	if (!(property_info->flags & ZEND_ACC_PUBLIC)) {
		// 如果有当前域
		if (UNEXPECTED(EG(fake_scope))) {
			// 使用当前域
			scope = EG(fake_scope);
		// 如果没有当前域
		} else {
			// 从执行数据串中，获取执行时作用域
			scope = zend_get_executed_scope();
		}
		// 如果属性不属于此域
		if (property_info->ce != scope) {
			// 如果有 private 标记 或 双方没有继承关系
			if (UNEXPECTED(property_info->flags & ZEND_ACC_PRIVATE)
				// 检验是否存在继承关系（双向）
			 || UNEXPECTED(!is_protected_compatible_scope(property_info->ce, scope))) {
				// 如果不是isset操作
				if (type != BP_VAR_IS) {
					// 访问属性失败，抛出异常
					zend_bad_property_access(property_info, ce, property_name);
				}
				//
				return NULL;
			}
		}
	}
	// 如果属性信息无静态标记
	if (UNEXPECTED((property_info->flags & ZEND_ACC_STATIC) == 0)) {
// 未定义的属性
undeclared_property:
		// 如果操作不是isset
		if (type != BP_VAR_IS) {
			// 抛错： 访问未定义的静态属性
			zend_throw_error(NULL, "Access to undeclared static property %s::$%s", ZSTR_VAL(ce->name), ZSTR_VAL(property_name));
		}
		// 返回Null
		return NULL;
	}

	// 如果未更新常量
	if (UNEXPECTED(!(ce->ce_flags & ZEND_ACC_CONSTANTS_UPDATED))) {
		// 更新常量
		if (UNEXPECTED(zend_update_class_constants(ce)) != SUCCESS) {
			// 失败返回null
			return NULL;
		}
	}

	// 保证静态属性已初始化过
	/* Ensure static properties are initialized. */
	// 返回静态成员表【指针列表】，如果无效
	if (UNEXPECTED(CE_STATIC_MEMBERS(ce) == NULL)) {
		// 初始化静态成员
		zend_class_init_statics(ce);
	}
	// 找到这个静态成员
	ret = CE_STATIC_MEMBERS(ce) + property_info->offset;
	// 解引用
	ZVAL_DEINDIRECT(ret);
	// 如果是写或更新操作 并且 变量是未定义 并且 有定义类型
	if (UNEXPECTED((type == BP_VAR_R || type == BP_VAR_RW)
				&& Z_TYPE_P(ret) == IS_UNDEF && ZEND_TYPE_IS_SET(property_info->type))) {
		// 抛错：有类型的静态属性不可以在没有初始化就访问
		zend_throw_error(NULL, "Typed static property %s::$%s must not be accessed before initialization",
			ZSTR_VAL(property_info->ce->name), ZSTR_VAL(property_name));
		// 返回Null
		return NULL;
	}
	// 如果是trait
	if (UNEXPECTED(ce->ce_flags & ZEND_ACC_TRAIT)) {
		// 报错：访问trait的静态变量是弃用的，只能通过引用它的类访问它。
		zend_error(E_DEPRECATED,
			"Accessing static trait property %s::$%s is deprecated, "
			"it should only be accessed on a class using the trait",
			ZSTR_VAL(property_info->ce->name), ZSTR_VAL(property_name));
	}
	// 返回静态成员
	return ret;
}
/* }}} */

// ing4, 读取静态成员变量，不带成员信息
ZEND_API zval *zend_std_get_static_property(zend_class_entry *ce, zend_string *property_name, int type) /* {{{ */
{
	zend_property_info *prop_info;
	return zend_std_get_static_property_with_info(ce, property_name, type, &prop_info);
}

// clear, 报错: 静态属性不能删除
ZEND_API ZEND_COLD bool zend_std_unset_static_property(zend_class_entry *ce, zend_string *property_name) /* {{{ */
{
	// 直接报错（静态属性不能删除）
	zend_throw_error(NULL, "Attempt to unset static property %s::$%s", ZSTR_VAL(ce->name), ZSTR_VAL(property_name));
	return 0;
}
/* }}} */

// ing4, 报错，调用构造方法失败
static ZEND_COLD zend_never_inline void zend_bad_constructor_call(zend_function *constructor, zend_class_entry *scope) /* {{{ */
{
	// 如果有作用域：不可以在此作用域调用此方法
	if (scope) {
		zend_throw_error(NULL, "Call to %s %s::%s() from scope %s",
			// 获取访问权限 字串：public,private,protected
			zend_visibility_string(constructor->common.fn_flags), ZSTR_VAL(constructor->common.scope->name),
			ZSTR_VAL(constructor->common.function_name), ZSTR_VAL(scope->name)
		);
	// 如果没有作用域：不可以在全局作用域调用此方法
	} else {
		zend_throw_error(NULL, "Call to %s %s::%s() from global scope", zend_visibility_string(constructor->common.fn_flags), ZSTR_VAL(constructor->common.scope->name), ZSTR_VAL(constructor->common.function_name));
	}
}
/* }}} */

// ing3, 取得构造方法 ?
// （实际上构造方法 不能是protected 或privated 否则不能new，所以这里是什么情况 ）
ZEND_API zend_function *zend_std_get_constructor(zend_object *zobj) /* {{{ */
{
	// 对象所属类的构造方法
	zend_function *constructor = zobj->ce->constructor;
	zend_class_entry *scope;
	// 如果构造方法存在
	if (constructor) {
		// 如果不是Public
		if (UNEXPECTED(!(constructor->op_array.fn_flags & ZEND_ACC_PUBLIC))) {
			// 如果有 EG(fake_scope)
			if (UNEXPECTED(EG(fake_scope))) {
				// 使用 EG(fake_scope) 为作用域
				scope = EG(fake_scope);
			// 否则 
			} else {
				// 从执行数据串中，获取执行时作用域
				scope = zend_get_executed_scope();
			}
			// 如果作用域和 方法本身的作用域不同（子类或其他情况）
			if (UNEXPECTED(constructor->common.scope != scope)) {
				// 如果操作码带有 私有标记（实际上构造方法 不能是protected 或privated 否则不能new，所以这里是什么情况 ）
				if (UNEXPECTED(constructor->op_array.fn_flags & ZEND_ACC_PRIVATE)
				// 如果取得方法的根类失败（如果有原型（->common.prototype）返回原型的 作用域。否则使用本身的作用域）
				// 如果作用域不合法
				 || UNEXPECTED(!zend_check_protected(zend_get_function_root_class(constructor), scope))) {
					// 报错
					zend_bad_constructor_call(constructor, scope);
					// 构造方法为 null
					constructor = NULL;
				}
			}
		}
	}
	// 返回构造方法
	return constructor;
}
/* }}} */

// ing3, 比较两个对象
ZEND_API int zend_std_compare_objects(zval *o1, zval *o2) /* {{{ */
{
	zend_object *zobj1, *zobj2;

	// 如果两个类型不一样
	if (Z_TYPE_P(o1) != Z_TYPE_P(o2)) {
		/* Object and non-object */
		zval *object;
		zval *value;
		zval casted;
		bool object_lhs;
		// 如果 o1 是对象
		if (Z_TYPE_P(o1) == IS_OBJECT) {
			// o1 是对象 ,o2 是其他类型
			object = o1;
			value = o2;
			// 
			object_lhs = true;
		// op不是对象
		} else {
			// o2 是对象 ,o1 是其他类型
			object = o2;
			value = o1;
			object_lhs = false;
		}
		
		// value不可以是对象
		ZEND_ASSERT(Z_TYPE_P(value) != IS_OBJECT);
		// ( 值为 false 或 true ) ? bool型 : value的类型
		zend_uchar target_type = (Z_TYPE_P(value) == IS_FALSE || Z_TYPE_P(value) == IS_TRUE)
								 ? _IS_BOOL : Z_TYPE_P(value);
		
		// 把两个中是 对象类型的一个 转成另一个同样的类型
		// 默认是 zend_std_cast_object_tostring ，把对象转成 string 或 bool类型，如果转换失败
		if (Z_OBJ_HT_P(object)->cast_object(Z_OBJ_P(object), &casted, target_type) == FAILURE) {
			// TODO: Less crazy.
			// 如果目标类型是 整数 或 小数
			if (target_type == IS_LONG || target_type == IS_DOUBLE) {
				// 提示：A 类的对象不可以转成 B 类型
				zend_error(E_NOTICE, "Object of class %s could not be converted to %s",
						   ZSTR_VAL(Z_OBJCE_P(object)->name), zend_get_type_by_const(target_type));
				// 如果要转成整数
				if (target_type == IS_LONG) {
					// 转换结果为1
					ZVAL_LONG(&casted, 1);
				// 转成小数
				} else {
					// 转换结果为 1.0
					ZVAL_DOUBLE(&casted, 1.0);
				}
			// 转换目标不是整数或小数
			} else {
				// 哪个是对象哪个大，没有相等
				return object_lhs ? 1 : -1;
			}
		}
		// 用转换后的值做比较
		int ret = object_lhs ? zend_compare(&casted, value) : zend_compare(value, &casted);
		// 删除转换后的值
		zval_ptr_dtor(&casted);
		// 返回结果 
		return ret;
	}
	
	// 如果两个类型一样
	// 取出两个对象
	zobj1 = Z_OBJ_P(o1);
	zobj2 = Z_OBJ_P(o2);

	// 如果是同一个对象
	if (zobj1 == zobj2) {
		// 返回相同
		return 0; /* the same object */
	}
	// 不是同一个对象
	// 如果所属类不同
	if (zobj1->ce != zobj2->ce) {
		// 返回无法比较
		return ZEND_UNCOMPARABLE; /* different classes */
	}
	// 所属类相同
	// 两个都没有附加属性
	if (!zobj1->properties && !zobj2->properties) {
		zend_property_info *info;
		int i;
		// 如果第一个对象没有默认属性（第二个一定也没有）
		if (!zobj1->ce->default_properties_count) {
			// 返回相同
			return 0;
		}

		// 只要保护一个对象就够了
		// 第二个可能引用了第一个，这样会造成错误的递归检测
		/* It's enough to protect only one of the objects.
		 * The second one may be referenced from the first and this may cause
		 * false recursion detection.
		 */
		// 使用二进制运算 or 来创建只有一个 条件跳转
		/* use bitwise OR to make only one conditional jump */
		
		// 通过指针检查 zval.value.counted 是否有递归标记
		if (UNEXPECTED(Z_IS_RECURSIVE_P(o1))) {
			// 报错：嵌套层数太多，有递归依赖？
			zend_error_noreturn(E_ERROR, "Nesting level too deep - recursive dependency?");
		}
		// 通过指针给 zval.value.counted 添加递归标记（这样如果再一次进到这里，就会认出这个标记）
		Z_PROTECT_RECURSION_P(o1);
		// 遍历默认属性表
		for (i = 0; i < zobj1->ce->default_properties_count; i++) {
			//
			zval *p1, *p2;
			// 属性信息
			info = zobj1->ce->properties_info_table[i];
			// 如果属性信息无效
			if (!info) {
				// 跳过这个
				continue;
			}
			// 属性信息有效

			// 取得对象1的属性
			p1 = OBJ_PROP(zobj1, info->offset);
			// 取得对象2的属性
			p2 = OBJ_PROP(zobj2, info->offset);

			// 对象1的属性不是 未定义
			if (Z_TYPE_P(p1) != IS_UNDEF) {
				// 对象2的属性也不是 未定义
				if (Z_TYPE_P(p2) != IS_UNDEF) {
					int ret;
					// 比较两个属性值
					ret = zend_compare(p1, p2);
					// 如果不相同
					if (ret != 0) {
						// 通过指针给 zval.value.counted 删除递归标记
						Z_UNPROTECT_RECURSION_P(o1);
						// 返回结果
						return ret;
					}
				// 对象2的属性是未定义
				} else {
					// 通过指针给 zval.value.counted 删除递归标记
					Z_UNPROTECT_RECURSION_P(o1);
					// 返回 对象1更大
					return 1;
				}
			// 对象1的属性是 未定义
			} else {
				// 对象2的属性也不是 未定义
				if (Z_TYPE_P(p2) != IS_UNDEF) {
					// 通过指针给 zval.value.counted 删除递归标记
					Z_UNPROTECT_RECURSION_P(o1);
					// 返回 对象1更大
					return 1;
				}
			}
		}
		// 通过指针给 zval.value.counted 删除递归标记
		Z_UNPROTECT_RECURSION_P(o1);
		// 返回相同
		return 0;
	// 至少1个有附加属性
	} else {
		// 如果对象1没有附加属性表
		if (!zobj1->properties) {
			// 创建属性哈希表 ->properties，并添加默认属性 （如果没创建过的话）
			rebuild_object_properties(zobj1);
		}
		// 如果对象2没有附加属性表
		if (!zobj2->properties) {
			// 创建属性哈希表 ->properties，并添加默认属性 （如果没创建过的话）
			rebuild_object_properties(zobj2);
		}
		// 用 hash_zval_compare_function 函数比较两个哈希表
		return zend_compare_symbol_tables(zobj1->properties, zobj2->properties);
	}
}
/* }}} */

// clear, 返回常量：不可比较
ZEND_API int zend_objects_not_comparable(zval *o1, zval *o2)
{
	return ZEND_UNCOMPARABLE;
}

// ing3, 检查属性是否存在 p1:对象，p2:属性名，p3:检查类型，p4:缓存位置
// ZEND_PROPERTY_ISSET=0:属性存在并且不是null
// ZEND_PROPERTY_NOT_EMPTY=1:属性不是空
// ZEND_PROPERTY_EXISTS=2:属性存在
ZEND_API int zend_std_has_property(zend_object *zobj, zend_string *name, int has_set_exists, void **cache_slot) /* {{{ */
{
	int result;
	zval *value = NULL;
	uintptr_t property_offset;
	zend_property_info *prop_info = NULL;
	zend_string *tmp_name = NULL;
	// 取得属性在属性列表里的序号。传入：类入口，属性名，是否静默，缓存位置，返回的属性信息指针
	property_offset = zend_get_property_offset(zobj->ce, name, 1, cache_slot, &prop_info);
	// 偏移量有效：大于0
	if (EXPECTED(IS_VALID_PROPERTY_OFFSET(property_offset))) {
		// 取出对象属性
		value = OBJ_PROP(zobj, property_offset);
		// 如果属性有效
		if (Z_TYPE_P(value) != IS_UNDEF) {
			// 按找到操作
			goto found;
		}
		if (UNEXPECTED(Z_PROP_FLAG_P(value) == IS_PROP_UNINIT)) {
			/* Skip __isset() for uninitialized typed properties */
			result = 0;
			// 按找到操作
			goto exit;
		}
	// 是动态属性偏移量
	} else if (EXPECTED(IS_DYNAMIC_PROPERTY_OFFSET(property_offset))) {
		// 如果有附加属性表
		if (EXPECTED(zobj->properties != NULL)) {
			// 检验：是未知的动态的属性编号
			if (!IS_UNKNOWN_DYNAMIC_PROPERTY_OFFSET(property_offset)) {
				// 解码 动态属性号，转成负数再-2
				uintptr_t idx = ZEND_DECODE_DYN_PROP_OFFSET(property_offset);
				// 如果索引号有效
				if (EXPECTED(idx < zobj->properties->nNumUsed * sizeof(Bucket))) {
					// 在哈希表中找到这个 bucket
					Bucket *p = (Bucket*)((char*)zobj->properties->arData + idx);
					// 如果名称匹配 或 （哈希值匹配 并且没有 key 并且 名称匹配）
					if (EXPECTED(p->key == name) ||
				        (EXPECTED(p->h == ZSTR_H(name)) &&
				         EXPECTED(p->key != NULL) &&
				         EXPECTED(zend_string_equal_content(p->key, name)))) {
						// 取出属性值
						value = &p->val;
						// 按找到操作
						goto found;
					}
				}
				// 把指针缓存到slot当前位置
				CACHE_PTR_EX(cache_slot + 1, (void*)ZEND_DYNAMIC_PROPERTY_OFFSET);
			}
			// 在附加属性表加查找此属性
			value = zend_hash_find(zobj->properties, name);
			// 如果找到
			if (value) {
				// 如果有缓存位置
				if (cache_slot) {
					// 索引位置
					uintptr_t idx = (char*)value - (char*)zobj->properties->arData;
					// 把指针缓存到slot当前位置
					CACHE_PTR_EX(cache_slot + 1, (void*)ZEND_ENCODE_DYN_PROP_OFFSET(idx));
				}
found:
				// ZEND_PROPERTY_NOT_EMPTY=1:属性不是空
				if (has_set_exists == ZEND_PROPERTY_NOT_EMPTY) {
					// 结果转成 布尔型
					result = zend_is_true(value);
				// ZEND_PROPERTY_NOT_EMPTY=1:属性不是空
				} else if (has_set_exists < ZEND_PROPERTY_NOT_EMPTY) {
					// ZEND_PROPERTY_ISSET=0:属性存在并且不是null
					ZEND_ASSERT(has_set_exists == ZEND_PROPERTY_ISSET);
					// 减少引用次数
					ZVAL_DEREF(value);
					// 不为null就算找到
					result = (Z_TYPE_P(value) != IS_NULL);
				// 其他情况
				} else {
					// ZEND_PROPERTY_EXISTS=2:属性存在
					ZEND_ASSERT(has_set_exists == ZEND_PROPERTY_EXISTS);
					// 结果为 1
					result = 1;
				}
				// 释放临时名称, 返回 result
				goto exit;
			}
		}
	// 如果有异常
	} else if (UNEXPECTED(EG(exception))) {
		// 未找到
		result = 0;
		// 释放临时名称, 返回 result
		goto exit;
	}

	result = 0;
	// 如果 has_set_exists != 2 并且 有 __isset 方法
	// ZEND_PROPERTY_EXISTS=2:属性存在
	if ((has_set_exists != ZEND_PROPERTY_EXISTS) && zobj->ce->__isset) {
		// 获取属性保护信息 .u2.property_guard
		uint32_t *guard = zend_get_property_guard(zobj, name);
		// 如果没有禁用 __isset
		if (!((*guard) & IN_ISSET)) {
			// 
			zval rv;
			// 如果有 __isset方法，尝试它
			/* have issetter - try with it! */
			// 如果还没有名称 并且 名称不是内置字串
			if (!tmp_name && !ZSTR_IS_INTERNED(name)) {
				// 复制名称
				tmp_name = zend_string_copy(name);
			}
			// 对象增加引用次数
			GC_ADDREF(zobj);
			// 禁用 __isset, 防止循环操作
			(*guard) |= IN_ISSET; /* prevent circular getting */
			// 调用 __isset 方法
			zend_std_call_issetter(zobj, name, &rv);
			// 结果转成 bool 值
			result = zend_is_true(&rv);
			// 销毁 结果
			zval_ptr_dtor(&rv);
			// 如果 has_set_exists==1 并且结果是 true
			// ZEND_PROPERTY_NOT_EMPTY=1:属性不是空
			if (has_set_exists == ZEND_PROPERTY_NOT_EMPTY && result) {
				// 如果没有异常 并且 有 __get 方法并且没有禁用 __get 方法
				if (EXPECTED(!EG(exception)) && zobj->ce->__get && !((*guard) & IN_GET)) {
					// 禁用 __get 方法
					(*guard) |= IN_GET;
					// 调用 __get 方法
					zend_std_call_getter(zobj, name, &rv);
					// 取消禁用 __get 方法
					(*guard) &= ~IN_GET;
					// 结果转成 布尔型
					result = i_zend_is_true(&rv);
					// 销毁 结果 
					zval_ptr_dtor(&rv);
				// 如果没有或不能用 __get 方法
				} else {
					// 结果 0
					result = 0;
				}
			}
			// 解除禁用 __isset 方法
			(*guard) &= ~IN_ISSET;
			// 释放对象
			OBJ_RELEASE(zobj);
		}
	}

// 释放临时名称, 返回 result
exit:
	// 释放临时名称
	zend_tmp_string_release(tmp_name);
	// 返回 result
	return result;
}
/* }}} */

// ing4, 获取类名的副本
ZEND_API zend_string *zend_std_get_class_name(const zend_object *zobj) /* {{{ */
{
	return zend_string_copy(zobj->ce->name);
}
/* }}} */

// ing3, 把对象转成string, __toString() 方法，支持转成string 或 bool类型
ZEND_API zend_result zend_std_cast_object_tostring(zend_object *readobj, zval *writeobj, int type) /* {{{ */
{
	// 
	switch (type) {
		// string 类型
		case IS_STRING: {
			zend_class_entry *ce = readobj->ce;
			// 看所属类是否有 __tostring 方法，如果有
			if (ce->__tostring) {
				zval retval;
				// 增加引用次数
				GC_ADDREF(readobj);
				// 调用方法 __tostring
				zend_call_known_instance_method_with_0_params(ce->__tostring, readobj, &retval);
				// 释放（减少引用次数）
				zend_object_release(readobj);
				// 如果返回结果是string类型，转换成功
				if (EXPECTED(Z_TYPE(retval) == IS_STRING)) {
					// 把结果复制到 writeobj
					ZVAL_COPY_VALUE(writeobj, &retval);
					// 返回成功
					return SUCCESS;
				}
				// 转换失败
				// 析构retval
				zval_ptr_dtor(&retval);
				// 如果有exception
				if (!EG(exception)) {
					// 抛出异常：__tostring必须返回string 类型
					zend_throw_error(NULL, "Method %s::__toString() must return a string value", ZSTR_VAL(ce->name));
				}
			}
			// 如果没有__tostring方法报错
			return FAILURE;
		}
		// bool 类型
		case _IS_BOOL:
			// 写入 TRUE
			ZVAL_TRUE(writeobj);
			// 
			return SUCCESS;
		// 其他类型，报错
		default:
			return FAILURE;
	}
}
/* }}} */

// ing3, 取得闭包，这个是取得__invoke方法。p1:对象，p2:返回类指针，p3:返回方法指针，p4:返回对象指针，p5:仅检查
ZEND_API zend_result zend_std_get_closure(zend_object *obj, zend_class_entry **ce_ptr, zend_function **fptr_ptr, zend_object **obj_ptr, bool check_only) /* {{{ */
{
	zend_class_entry *ce = obj->ce;
	// 取得 __invoke 方法
	zval *func = zend_hash_find_known_hash(&ce->function_table, ZSTR_KNOWN(ZEND_STR_MAGIC_INVOKE));
	// 如果没有这个方法，返回错误
	if (func == NULL) {
		return FAILURE;
	}
	// 取得 zval 中的 func
	*fptr_ptr = Z_FUNC_P(func);

	// 作用域
	*ce_ptr = ce;
	// 如果是静态方法
	if ((*fptr_ptr)->common.fn_flags & ZEND_ACC_STATIC) {
		// 如果有所属对象，把所属对象设置成空
		if (obj_ptr) {
			*obj_ptr = NULL;
		}
	// 如果不是
	} else {
		// 如果有所属对象，所属对象设置为 obj
		if (obj_ptr) {
			*obj_ptr = obj;
		}
	}
	// 返回成功
	return SUCCESS;
}
/* }}} */

// ing3, 按要求返回属性列表
ZEND_API HashTable *zend_std_get_properties_for(zend_object *obj, zend_prop_purpose purpose) {
	HashTable *ht;
	// 根据目标进行操作
	switch (purpose) {
		// 调试
		case ZEND_PROP_PURPOSE_DEBUG:
			// 如果支持获取调试信息
			if (obj->handlers->get_debug_info) {
				int is_temp;
				// 获取调试信息
				ht = obj->handlers->get_debug_info(obj, &is_temp);
				// 如果不是临时对象，增加引用次数
				if (ht && !is_temp) {
					GC_TRY_ADDREF(ht);
				}
				// 返回调试信息
				return ht;
			}
			ZEND_FALLTHROUGH;
		// 转array 或 序列化 或 导出变量 或 json_encode
		case ZEND_PROP_PURPOSE_ARRAY_CAST:
		case ZEND_PROP_PURPOSE_SERIALIZE:
		case ZEND_PROP_PURPOSE_VAR_EXPORT:
		case ZEND_PROP_PURPOSE_JSON:
			// 获取并返回属性列表
			ht = obj->handlers->get_properties(obj);
			if (ht) {
				// 增加引用次数
				GC_TRY_ADDREF(ht);
			}
			return ht;
		// 不支持其他操作, 默认返回null
		default:
			ZEND_UNREACHABLE();
			return NULL;
	}
}

// ing4, 调用对象自带的方法，或标准方法，获取属性列表
ZEND_API HashTable *zend_get_properties_for(zval *obj, zend_prop_purpose purpose) {
	// 取得zval中的对象
	zend_object *zobj = Z_OBJ_P(obj);
	// 如果有 get_properties_for方法
	if (zobj->handlers->get_properties_for) {
		// 调用它获取属性列表
		return zobj->handlers->get_properties_for(zobj, purpose);
	}
	// 调用标准方法获取属性列表
	return zend_std_get_properties_for(zobj, purpose);
}

// 标准对象处理器
ZEND_API const zend_object_handlers std_object_handlers = {
	0,										/* offset */

	zend_object_std_dtor,					/* free_obj */
	zend_objects_destroy_object,			/* dtor_obj */
	zend_objects_clone_obj,					/* clone_obj */

	zend_std_read_property,					/* read_property */
	zend_std_write_property,				/* write_property */
	zend_std_read_dimension,				/* read_dimension */
	zend_std_write_dimension,				/* write_dimension */
	zend_std_get_property_ptr_ptr,			/* get_property_ptr_ptr */
	zend_std_has_property,					/* has_property */
	zend_std_unset_property,				/* unset_property */
	zend_std_has_dimension,					/* has_dimension */
	zend_std_unset_dimension,				/* unset_dimension */
	zend_std_get_properties,				/* get_properties */
	zend_std_get_method,					/* get_method */
	zend_std_get_constructor,				/* get_constructor */
	zend_std_get_class_name,				/* get_class_name */
	zend_std_cast_object_tostring,			/* cast_object */
	NULL,									/* count_elements */
	zend_std_get_debug_info,				/* get_debug_info */
	zend_std_get_closure,					/* get_closure */
	zend_std_get_gc,						/* get_gc */
	NULL,									/* do_operation */
	zend_std_compare_objects,				/* compare */
	NULL,									/* get_properties_for */
};
