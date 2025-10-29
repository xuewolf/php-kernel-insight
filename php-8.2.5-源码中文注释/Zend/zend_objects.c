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

#include "zend.h"
#include "zend_globals.h"
#include "zend_variables.h"
#include "zend_API.h"
#include "zend_interfaces.h"
#include "zend_exceptions.h"
#include "zend_weakrefs.h"

// 一共7个方法
// ing3, 标准初始化方法
static zend_always_inline void _zend_object_std_init(zend_object *object, zend_class_entry *ce)
{
	// 引用次数为1
	GC_SET_REFCOUNT(object, 1);
	// gc.u.type_info 为 GC_OBJECT
	GC_TYPE_INFO(object) = GC_OBJECT;
	// 类入口
	object->ce = ce;
	// 先不创建【附加属性】哈希表
	object->properties = NULL;
	// 对象关联到全局容器
	zend_objects_store_put(object);
	// 如果有用到魔术方法
	if (UNEXPECTED(ce->ce_flags & ZEND_ACC_USE_GUARDS)) {
		// 结尾元素设置成 IS_UNDEF
		ZVAL_UNDEF(object->properties_table + object->ce->default_properties_count);
	}
}

// ing4, zend 对象标准初始化
ZEND_API void ZEND_FASTCALL zend_object_std_init(zend_object *object, zend_class_entry *ce)
{
	_zend_object_std_init(object, ce);
}

// ing2, 标准析构器
ZEND_API void zend_object_std_dtor(zend_object *object)
{
	zval *p, *end;
	// 如果对象有属性列表
	if (object->properties) {
		// 如果属性列表是不可变数组
		if (EXPECTED(!(GC_FLAGS(object->properties) & IS_ARRAY_IMMUTABLE))) {
			// 如果属性列表引用数 -1 后为0
			if (EXPECTED(GC_DELREF(object->properties) == 0)
					// 并且属性列表不是 NULL
					&& EXPECTED(GC_TYPE(object->properties) != IS_NULL)) {
				// 销毁属性列表
				zend_array_destroy(object->properties);
			}
		}
	}
	// 属性列表指针
	p = object->properties_table;
	// 如果有属性数量
	if (EXPECTED(object->ce->default_properties_count)) {
		// 尾元素指针
		end = p + object->ce->default_properties_count;
		// 遍历每一个
		do {
			// 如果是可计数对象
			if (Z_REFCOUNTED_P(p)) {
				// 如果p是引用类型
				if (UNEXPECTED(Z_ISREF_P(p)) &&
						// 并且 是调试模式 或 有引用源
						(ZEND_DEBUG || ZEND_REF_HAS_TYPE_SOURCES(Z_REF_P(p)))) {
					// 通过slot（p）指针计算slot的序号，返回此序号对应的属性信息
					zend_property_info *prop_info = zend_get_property_info_for_slot(object, p);
					// 如果类型有效
					if (ZEND_TYPE_IS_SET(prop_info->type)) {
						// ??
						ZEND_REF_DEL_TYPE_SOURCE(Z_REF_P(p), prop_info);
					}
				}
				// 销毁p或添加到gc列表
				i_zval_ptr_dtor(p);
			}
			// 下一个类属性
			p++;
		} while (p != end);
	}

	// 如果有使用保护 ？
	if (UNEXPECTED(object->ce->ce_flags & ZEND_ACC_USE_GUARDS)) {
		// 如果属性类型是 字串
		if (EXPECTED(Z_TYPE_P(p) == IS_STRING)) {
			// 删除字串
			zval_ptr_dtor_str(p);
		// 如果属性类型是 数组
		} else if (Z_TYPE_P(p) == IS_ARRAY) {
			HashTable *guards;
			// 从参数中取出数组
			guards = Z_ARRVAL_P(p);
			// 数组不能为空
			ZEND_ASSERT(guards != NULL);
			// 销毁数组
			zend_hash_destroy(guards);
			// 释放数组内存
			FREE_HASHTABLE(guards);
		}
	}

	// 如果对象是弱引用
	if (UNEXPECTED(GC_FLAGS(object) & IS_OBJ_WEAKLY_REFERENCED)) {
		// ？？
		zend_weakrefs_notify(object);
	}
}

// ing2, 销毁对象
ZEND_API void zend_objects_destroy_object(zend_object *object)
{
	// 析构方法
	zend_function *destructor = object->ce->destructor;
	// 如果有析构方法
	if (destructor) {
		zend_object *old_exception;
		const zend_op *old_opline_before_exception;
		// 如果析构方法是 ZEND_ACC_PRIVATE 或 ZEND_ACC_PROTECTED
		if (destructor->op_array.fn_flags & (ZEND_ACC_PRIVATE|ZEND_ACC_PROTECTED)) {
			// 如果是 private 
			if (destructor->op_array.fn_flags & ZEND_ACC_PRIVATE) {
				// 保证如果我们调用私有方法，需要符合权限验证
				/* Ensure that if we're calling a private function, we're allowed to do so.
				 */
				// 如果有执行上下文
				if (EG(current_execute_data)) {
					// 取得作用域
					zend_class_entry *scope = zend_get_executed_scope();
					// 如果和当前对象不符
					if (object->ce != scope) {
						// 报错，不可以在**域中 调用此析构方法
						zend_throw_error(NULL,
							"Call to private %s::__destruct() from %s%s",
							ZSTR_VAL(object->ce->name),
							scope ? "scope " : "global scope",
							scope ? ZSTR_VAL(scope->name) : ""
						);
						return;
					}
				// 如果没有执行上下文
				} else {
					// 报错：不可以在全局作用域调用私有的析构方法
					zend_error(E_WARNING,
						"Call to private %s::__destruct() from global scope during shutdown ignored",
						ZSTR_VAL(object->ce->name));
					return;
				}
			// 如果是protected
			} else {
				// 保证如果我们调用私有方法，需要符合权限验证
				/* Ensure that if we're calling a protected function, we're allowed to do so.
				 */
				// 如果有执行上下文
				if (EG(current_execute_data)) {
					zend_class_entry *scope = zend_get_executed_scope();
					// 取得方法的根类：如果有原型（->common.prototype）返回原型的 作用域。否则使用本身的作用域
					// 如果保护检验未通过
					if (!zend_check_protected(zend_get_function_root_class(destructor), scope)) {
						// 报错： 不可以在**域中 调用此析构方法
						zend_throw_error(NULL,
							"Call to protected %s::__destruct() from %s%s",
							ZSTR_VAL(object->ce->name),
							scope ? "scope " : "global scope",
							scope ? ZSTR_VAL(scope->name) : ""
						);
						return;
					}
				// 如果没有执行上下文
				} else {
					// 报错：不可以在全局作用域调用私有的析构方法
					zend_error(E_WARNING,
						"Call to protected %s::__destruct() from global scope during shutdown ignored",
						ZSTR_VAL(object->ce->name));
					return;
				}
			}
		}
		// 对象添加引用次数
		GC_ADDREF(object);

		// 保证析构方法在前面抛出的异常中受保护，例如，如果一个函数抛了异常
		/* Make sure that destructors are protected from previously thrown exceptions.
		 * For example, if an exception was thrown in a function and when the function's
		 * local variable destruction results in a destructor being called.
		 */
		old_exception = NULL;
		// 如果已经有异常
		if (EG(exception)) {
			// 如果常量就是当前对象
			if (EG(exception) == object) {
				// 报错：不可销毁待处理的异常
				zend_error_noreturn(E_CORE_ERROR, "Attempt to destruct pending exception");
			// 如果异常不是当前对象
			} else {
				// 如果有执行上下文
				if (EG(current_execute_data)
				// 并且上下文在函数中
				 && EG(current_execute_data)->func
				// 并且函数是用户定义的
				 && ZEND_USER_CODE(EG(current_execute_data)->func->common.type)) {
					// 重新抛异常 ？
					zend_rethrow_exception(EG(current_execute_data));
				}
				// 旧异常
				old_exception = EG(exception);
				// 
				old_opline_before_exception = EG(opline_before_exception);
				EG(exception) = NULL;
			}
		}
		// 调用 destructor
		zend_call_known_instance_method_with_0_params(destructor, object, NULL);

		// 如果有旧的异常
		if (old_exception) {
			// 如果上下文没变化 
			EG(opline_before_exception) = old_opline_before_exception;
			// 如果有异常
			if (EG(exception)) {
				// 
				zend_exception_set_previous(EG(exception), old_exception);
			// 如果没有异常
			} else {
				// 使用旧的异常
				EG(exception) = old_exception;
			}
		}
		// 释放对象内存
		OBJ_RELEASE(object);
	}
	// 没有析构方法，什么也不做，也不free
}

// ing4, 创建新对象，并初始化它
ZEND_API zend_object* ZEND_FASTCALL zend_objects_new(zend_class_entry *ce)
{
	// 对象大小是 对象本身 + 属性变量大小（个数*zval大小）
	zend_object *object = emalloc(sizeof(zend_object) + zend_object_properties_size(ce));
	// 初始化标准对象
	_zend_object_std_init(object, ce);
	// 处理器（销毁方法）
	object->handlers = &std_object_handlers;
	// 返回对象
	return object;
}

// ing2, 克隆对象成员变量
ZEND_API void ZEND_FASTCALL zend_objects_clone_members(zend_object *new_object, zend_object *old_object)
{
	// 如果旧的成员有属性
	if (old_object->ce->default_properties_count) {
		// 源列表
		zval *src = old_object->properties_table;
		// 目标列表
		zval *dst = new_object->properties_table;
		// 源列表结尾元素
		zval *end = src + old_object->ce->default_properties_count;
		// 遍历所有源元素
		do {
			// 销毁dst，如果有引用次数，添加到gc列表中
			i_zval_ptr_dtor(dst);
			// 把 src 复制到 dst
			ZVAL_COPY_VALUE_PROP(dst, src);
			// dst 增加引用次数
			zval_add_ref(dst);
			// 如果dst是引用类型
			if (UNEXPECTED(Z_ISREF_P(dst)) &&
					// 如果是调试模式 或 引用目标不是NULL
					(ZEND_DEBUG || ZEND_REF_HAS_TYPE_SOURCES(Z_REF_P(dst)))) {
				// 通过slot（它也是属性）指针计算slot的序号，返回此序号对应的属性信息
				zend_property_info *prop_info = zend_get_property_info_for_slot(new_object, dst);
				// 如果类型有效
				if (ZEND_TYPE_IS_SET(prop_info->type)) {
					// 向zend_property_info_source_list指向的 zend_property_info 列表中添加 zend_property_info
					// 添加类型信息
					ZEND_REF_ADD_TYPE_SOURCE(Z_REF_P(dst), prop_info);
				}
			}
			// 下一个源元素
			src++;
			// 下一个目标元素
			dst++;
		// 直到源结尾
		} while (src != end);
	// 如果旧成员有属性列表 并且 旧成员没有 clone方法
	} else if (old_object->properties && !old_object->ce->clone) {
		// 快速复制
		/* fast copy */
		// 如果处理器列表 就是 标记处理器列表
		if (EXPECTED(old_object->handlers == &std_object_handlers)) {
			// 如果 properties 不是不能变的数组
			if (EXPECTED(!(GC_FLAGS(old_object->properties) & IS_ARRAY_IMMUTABLE))) {
				// 引用次数 +1
				GC_ADDREF(old_object->properties);
			}
			// 使用新的属性列表
			new_object->properties = old_object->properties;
			return;
		}
	}

	// 如果有属性列表 
	if (old_object->properties &&
		// 并且 列表中有元素
	    EXPECTED(zend_hash_num_elements(old_object->properties))) {
		zval *prop, new_prop;
		zend_ulong num_key;
		zend_string *key;

		// 如果新对象没有属性列表
		if (!new_object->properties) {
			// 创建一个元素数量相同的新数组
			new_object->properties = zend_new_array(zend_hash_num_elements(old_object->properties));
			// 初始化成哈希表
			zend_hash_real_init_mixed(new_object->properties);
		// 如果已经有了属性列表
		} else {
			// 扩展哈希表，第三个参数：类型不是顺序数组 
			zend_hash_extend(new_object->properties, new_object->properties->nNumUsed + zend_hash_num_elements(old_object->properties), 0);
		}
		// 属性哈希表添加标记：有空索引号
		HT_FLAGS(new_object->properties) |=
			HT_FLAGS(old_object->properties) & HASH_FLAG_HAS_EMPTY_IND;

		// 遍历 properties => prop
		ZEND_HASH_MAP_FOREACH_KEY_VAL(old_object->properties, num_key, key, prop) {
			// 如果 prop 是间接引用
			if (Z_TYPE_P(prop) == IS_INDIRECT) {
				// new_prop.zv 添加指针, 并标记成 间接引用 类型 
				// ？？？
				ZVAL_INDIRECT(&new_prop, new_object->properties_table + (Z_INDIRECT_P(prop) - old_object->properties_table));
			// 其他情况
			} else {
				// 直接复制值
				ZVAL_COPY_VALUE(&new_prop, prop);
				// 新 prop 增加引用次数
				zval_add_ref(&new_prop);
			}
			// 如果有 key 
			if (EXPECTED(key)) {
				// 新属性添加进哈希表里
				_zend_hash_append(new_object->properties, key, &new_prop);
			// 没有key
			} else {
				// 按顺序号添加进哈希表里 num_key？
				zend_hash_index_add_new(new_object->properties, num_key, &new_prop);
			}
		} ZEND_HASH_FOREACH_END();
	}
	

	// 如果 旧成员有克隆方法
	if (old_object->ce->clone) {
		// 新成员引用数量  +1 
		GC_ADDREF(new_object);
		// 调用新成员的clone方法（所以 __clone 方法是克隆后的附加处理，没有它对象也能正常克隆，测试过）
		zend_call_known_instance_method_with_0_params(new_object->ce->clone, new_object, NULL);
		// 释放 new_object
		OBJ_RELEASE(new_object);
	}
}

// ing3， 克隆对象
ZEND_API zend_object *zend_objects_clone_obj(zend_object *old_object)
{
	zend_object *new_object;

	// 假设创建没有被重写，当克隆依赖被重写的对象，时自己也要被重写
	/* assume that create isn't overwritten, so when clone depends on the
	 * overwritten one then it must itself be overwritten */
	// 使用原对象的所属类，创建新对象
	new_object = zend_objects_new(old_object->ce);

	// 
	/* zend_objects_clone_members() expect the properties to be initialized. */
	// 如果新对象有属性
	if (new_object->ce->default_properties_count) {
		// 属性表开头
		zval *p = new_object->properties_table;
		// 属性表结尾元素
		zval *end = p + new_object->ce->default_properties_count;
		// 遍历所有属性
		do {
			// 每一个属性设置成 IS_UNDEF
			ZVAL_UNDEF(p);
			// 下一个属性
			p++;
		// 直到结尾元素
		} while (p != end);
	}

	// 克隆对象成员变量（上面已经设置成 IS_UNDEF ）
	zend_objects_clone_members(new_object, old_object);

	return new_object;
}
