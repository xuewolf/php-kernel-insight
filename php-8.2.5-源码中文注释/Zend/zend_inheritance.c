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
   +----------------------------------------------------------------------+
*/

#include "zend.h"
#include "zend_API.h"
#include "zend_compile.h"
#include "zend_execute.h"
#include "zend_inheritance.h"
#include "zend_interfaces.h"
#include "zend_smart_str.h"
#include "zend_operators.h"
#include "zend_exceptions.h"
#include "zend_enum.h"
#include "zend_attributes.h"
#include "zend_constants.h"
#include "zend_observer.h"

// 这里面最复杂的是类型检查


ZEND_API zend_class_entry* (*zend_inheritance_cache_get)(zend_class_entry *ce, zend_class_entry *parent, zend_class_entry **traits_and_interfaces) = NULL;
ZEND_API zend_class_entry* (*zend_inheritance_cache_add)(zend_class_entry *ce, zend_class_entry *proto, zend_class_entry *parent, zend_class_entry **traits_and_interfaces, HashTable *dependencies) = NULL;

// 未完成 表示  决定继承是否有效需要的类声名现在还不可用。在运行时 UNRESOLVED 可以被当成 错误来处理
/* Unresolved means that class declarations that are currently not available are needed to
 * determine whether the inheritance is valid or not. At runtime UNRESOLVED should be treated
 * as an ERROR. */
typedef enum {
	// 未完成
	INHERITANCE_UNRESOLVED = -1,
	// 错误
	INHERITANCE_ERROR = 0,
	// 警告
	INHERITANCE_WARNING = 1,
	// 成功
	INHERITANCE_SUCCESS = 2,
} inheritance_status;
// 因为 enum 实例无法作为返回值类型，所以这里要 typedef 做个别名，才能当成类型使用。

//
static void add_dependency_obligation(zend_class_entry *ce, zend_class_entry *dependency_ce);

//
static void add_compatibility_obligation(
		zend_class_entry *ce, const zend_function *child_fn, zend_class_entry *child_scope,
		const zend_function *parent_fn, zend_class_entry *parent_scope);

//
static void add_property_compatibility_obligation(
		zend_class_entry *ce, const zend_property_info *child_prop,
		const zend_property_info *parent_prop);

//
static void ZEND_COLD emit_incompatible_method_error(
		const zend_function *child, zend_class_entry *child_scope,
		const zend_function *parent, zend_class_entry *parent_scope,
		inheritance_status status);

// ing3, 复制类型，如果是列表才复制，不是列表只增加引用次数。列表的旧 zend_type_list 留给谁了呢？
static void zend_type_copy_ctor(zend_type *type, bool persistent) {
	// 如果是列表类型
	if (ZEND_TYPE_HAS_LIST(*type)) {
		// 取得列表
		zend_type_list *old_list = ZEND_TYPE_LIST(*type);
		// 取得列表大小
		size_t size = ZEND_TYPE_LIST_SIZE(old_list->num_types);
		// 分配新列表
		// 如果类型是 ARENA 分配的， 新列表也用 ARENA 分配 ，否则 普通分配
		zend_type_list *new_list = ZEND_TYPE_USES_ARENA(*type)
			? zend_arena_alloc(&CG(arena), size) : pemalloc(size, persistent);
		// 把旧列表中的元素 复制过来
		memcpy(new_list, old_list, ZEND_TYPE_LIST_SIZE(old_list->num_types));
		
		// 把新列表关联到原来的type上
		// #define ZEND_TYPE_SET_PTR(t, _ptr) ((t).ptr = (_ptr))
		// (*type).ptr = new_list
		ZEND_TYPE_SET_PTR(*type, new_list);

		zend_type *list_type;
		// 遍历新列表
		ZEND_TYPE_LIST_FOREACH(new_list, list_type) {
			// 每个元素必须要有名字
			ZEND_ASSERT(ZEND_TYPE_HAS_NAME(*list_type));
			// 给名字增加引用次数
			zend_string_addref(ZEND_TYPE_NAME(*list_type));
		} ZEND_TYPE_LIST_FOREACH_END();
	// 如果不是列表类型，但有名字
	} else if (ZEND_TYPE_HAS_NAME(*type)) {
		// 给名字增加引用次数
		zend_string_addref(ZEND_TYPE_NAME(*type));
	}
}

// ing3, 内置函数创建副本
static zend_function *zend_duplicate_internal_function(zend_function *func, zend_class_entry *ce) /* {{{ */
{
	zend_function *new_function;
	// 如果是内置类
	if (UNEXPECTED(ce->type & ZEND_INTERNAL_CLASS)) {
		// 创建新内置函数 
		new_function = pemalloc(sizeof(zend_internal_function), 1);
		// 把函数复制过来
		memcpy(new_function, func, sizeof(zend_internal_function));
	// 不是内置类，但仍然是内置函数
	} else {
		// arena 分配空间，大小是 内置函数大小
		new_function = zend_arena_alloc(&CG(arena), sizeof(zend_internal_function));
		// 按内置函数大小复制
		memcpy(new_function, func, sizeof(zend_internal_function));
		// 新函数添加标记 由 ARENA 分配
		new_function->common.fn_flags |= ZEND_ACC_ARENA_ALLOCATED;
	}
	// 新函数有函数名
	if (EXPECTED(new_function->common.function_name)) {
		// 函数名添加引用次数
		zend_string_addref(new_function->common.function_name);
	}
	// 返回新函数
	return new_function;
}
/* }}} */

// ing3, 为函数创建副本
static zend_always_inline zend_function *zend_duplicate_function(zend_function *func, zend_class_entry *ce) /* {{{ */
{
	// 如果是内置函数 
	if (UNEXPECTED(func->type == ZEND_INTERNAL_FUNCTION)) {
		// 内置函数创建副本
		return zend_duplicate_internal_function(func, ce);
	// 不是内置函数 
	} else {
		// 如果操作码有引用次数
		if (func->op_array.refcount) {
			// 增加引用次数
			(*func->op_array.refcount)++;
		}
		// 如果有函数名
		if (EXPECTED(func->op_array.function_name)) {
			// 函数名增加引用次数
			zend_string_addref(func->op_array.function_name);
		}
		// 返回原函数，实际上只增加引用次数，没有创建副本
		return func;
	}
}
/* }}} */

// ing4, 继承父类的构造方法
static void do_inherit_parent_constructor(zend_class_entry *ce) /* {{{ */
{
	zend_class_entry *parent = ce->parent;

	// 父类必须有效
	ZEND_ASSERT(parent != NULL);

	// create_object 方法不能改变。这个不是object_handler.c里的标准方法，是个性方法。
	/* You cannot change create_object */
	ce->create_object = parent->create_object;

	// 如果需要，继承特殊方法
	/* Inherit special functions if needed */
	
	// 这些方法在没有的时候才继承
	if (EXPECTED(!ce->get_iterator)) {
		ce->get_iterator = parent->get_iterator;
	}
	if (EXPECTED(!ce->__get)) {
		ce->__get = parent->__get;
	}
	if (EXPECTED(!ce->__set)) {
		ce->__set = parent->__set;
	}
	if (EXPECTED(!ce->__unset)) {
		ce->__unset = parent->__unset;
	}
	if (EXPECTED(!ce->__isset)) {
		ce->__isset = parent->__isset;
	}
	if (EXPECTED(!ce->__call)) {
		ce->__call = parent->__call;
	}
	if (EXPECTED(!ce->__callstatic)) {
		ce->__callstatic = parent->__callstatic;
	}
	if (EXPECTED(!ce->__tostring)) {
		ce->__tostring = parent->__tostring;
	}
	if (EXPECTED(!ce->clone)) {
		ce->clone = parent->clone;
	}
	if (EXPECTED(!ce->__serialize)) {
		ce->__serialize = parent->__serialize;
	}
	if (EXPECTED(!ce->__unserialize)) {
		ce->__unserialize = parent->__unserialize;
	}
	if (EXPECTED(!ce->serialize)) {
		ce->serialize = parent->serialize;
	}
	if (EXPECTED(!ce->unserialize)) {
		ce->unserialize = parent->unserialize;
	}
	if (!ce->destructor) {
		ce->destructor = parent->destructor;
	}
	if (EXPECTED(!ce->__debugInfo)) {
		ce->__debugInfo = parent->__debugInfo;
	}

	// 如果子类有构造方法
	if (ce->constructor) {
		// 如果父类有构造方法， 并且 构造方法有final标记
		if (parent->constructor && UNEXPECTED(parent->constructor->common.fn_flags & ZEND_ACC_FINAL)) {
			// 报错：不可以覆盖 final 方法
			zend_error_noreturn(E_ERROR, "Cannot override final %s::%s() with %s::%s()",
				ZSTR_VAL(parent->name), ZSTR_VAL(parent->constructor->common.function_name),
				ZSTR_VAL(ce->name), ZSTR_VAL(ce->constructor->common.function_name));
		}
		// 完成
		return;
	}
	// 如果子类没有构造方法，继承父类的
	ce->constructor = parent->constructor;
}
/* }}} */

// ing4, 获取可见性 字串：public,private,protected
char *zend_visibility_string(uint32_t fn_flags) /* {{{ */
{
	if (fn_flags & ZEND_ACC_PUBLIC) {
		return "public";
	} else if (fn_flags & ZEND_ACC_PRIVATE) {
		return "private";
	} else {
		ZEND_ASSERT(fn_flags & ZEND_ACC_PROTECTED);
		return "protected";
	}
}
/* }}} */

// ing3, 获取类名，先处理parent和self, 如果都不是，用当前域类名。p1:当前域
static zend_string *resolve_class_name(zend_class_entry *scope, zend_string *name) {
	// 域必须存在
	ZEND_ASSERT(scope);
	// 忽略大小写比较，如果name是 parent 并且 域有parent
	if (zend_string_equals_literal_ci(name, "parent") && scope->parent) {
		// 如果类有 带 parent 标记
		if (scope->ce_flags & ZEND_ACC_RESOLVED_PARENT) {
			// 返回父类的名称
			return scope->parent->name;
		// 没有标记，此类的父类名元素
		} else {
			// parent_name 是编译时增加的，->parent是 完成继承以后才有的，
			// 如果还没完成继承，可能没有 scope->parent->name 但有 scope->parent_name
			return scope->parent_name;
		}
	// 如果name是self
	} else if (zend_string_equals_literal_ci(name, "self")) {
		// 类名
		return scope->name;
	// 如果都不是，返回给出的备选名字
	} else {
		return name;
	}
}

// ing4, 此类是否可用
static bool class_visible(zend_class_entry *ce) {
	// 如果是内置类
	if (ce->type == ZEND_INTERNAL_CLASS) {
		// 如果编译选项没有要求忽略内置类，就可用
		return !(CG(compiler_options) & ZEND_COMPILE_IGNORE_INTERNAL_CLASSES);
	// 不是内置类
	} else {
		// 类型必须是用户类
		ZEND_ASSERT(ce->type == ZEND_USER_CLASS);
		// 如果编译选项没有要求忽略其他文件 或 此类属于当前文件。这样此类都可用。
		return !(CG(compiler_options) & ZEND_COMPILE_IGNORE_OTHER_FILES)
			|| ce->info.user.filename == CG(compiled_filename);
	}
}

// ing3, 注册未完成的类
static zend_always_inline void register_unresolved_class(zend_string *name) {
	// 稍后会自动载入此类，处理并延时 变异责任
	/* We'll autoload this class and process delayed variance obligations later. */
	// 如果没有延时加载表
	if (!CG(delayed_autoloads)) {
		// 创建延时加载哈希表
		ALLOC_HASHTABLE(CG(delayed_autoloads));
		// 初始化哈希表
		zend_hash_init(CG(delayed_autoloads), 0, NULL, NULL, 0);
	}
	// 把此类添加到列表里
	zend_hash_add_empty_element(CG(delayed_autoloads), name);
}

// ing3, 用类名查找类，p1:当前域，p2:类名，p3:是否注册延时加载
// 正在编译 或者有 要求预加载 时才可能用到备选类 
static zend_class_entry *lookup_class_ex(
		zend_class_entry *scope, zend_string *name, bool register_unresolved) {
	zend_class_entry *ce;
	
	// 如果编译选项有要求预加载
	bool in_preload = CG(compiler_options) & ZEND_COMPILE_PRELOAD;

	// 情况1，如果 不正在执行并且 没有预加载
	if (UNEXPECTED(!EG(active) && !in_preload)) {
		// 小写类名
		zend_string *lc_name = zend_string_tolower(name);
		// 编译时类表中查找此类
		ce = zend_hash_find_ptr(CG(class_table), lc_name);
		// 释放类名
		zend_string_release(lc_name);
		// 如果需要 register_unresolved 并且 类没有找到
		if (register_unresolved && !ce) {
			// 报错 ：要查找的类必须比 当前域更早注册
			zend_error_noreturn(
				E_COMPILE_ERROR, "%s must be registered before %s",
				ZSTR_VAL(name), ZSTR_VAL(scope->name));
	    }

		return ce;
	}
	
	// 如果 正在执行 或者 有预加载

	// 用名称查找类。附加要求：允许未链接，不自动加载
	ce = zend_lookup_class_ex(
	    name, NULL, ZEND_FETCH_CLASS_ALLOW_UNLINKED | ZEND_FETCH_CLASS_NO_AUTOLOAD);

	// 情况2，如果没在编译 并且 没有要求预加载
	if (!CG(in_compilation) || in_preload) {
		// 只要找到类就返回此类
		if (ce) {
			return ce;
		}
		// 没找到，添加到延时列表
		if (register_unresolved) {
			register_unresolved_class(name);
		}
	// 情况3，正在编译 或者有 要求预加载
	} else {
		// 有找到并且可见才返回此类
		if (ce && class_visible(ce)) {
			return ce;
		}
		// 没找到
		// 此类可能没有被注册，需要显示检验一下
		/* The current class may not be registered yet, so check for it explicitly. */
		// 如果 当前域 与 要求的名称 相符。为什么不先判断这个呢？
		if (zend_string_equals_ci(scope->name, name)) {
			// 返回当前域
			return scope;
		}
	}
	// 没找到，返回null
	return NULL;
}

// ing3, 用类名查找类，不延时加载。p1:当前域，p2:类名
static zend_class_entry *lookup_class(zend_class_entry *scope, zend_string *name) {
	// 用类名查找类，p1:当前域，p2:类名，p3:是否注册延时加载
	return lookup_class_ex(scope, name, /* register_unresolved */ false);
}

// ing3, 可以安全用在未链接的类上的 Instanceof 
/* Instanceof that's safe to use on unlinked classes. */
static bool unlinked_instanceof(zend_class_entry *ce1, zend_class_entry *ce2) {
	// 两个类指针相同，返回true
	if (ce1 == ce2) {
		return 1;
	}
	// ce1 链接过
	if (ce1->ce_flags & ZEND_ACC_LINKED) {
		// 检验继承或接口实现，单向 ： zend_operators.h
		return instanceof_function(ce1, ce2);
	}

	// ce1 没有链接过
	// ce1 有父类
	if (ce1->parent) {
		zend_class_entry *parent_ce;
		// ce1 处理过继承
		if (ce1->ce_flags & ZEND_ACC_RESOLVED_PARENT) {
			// 取出父类
			parent_ce = ce1->parent;
		// ce1 没有处理过继承
		} else {
			// 用父类名查找父类，允许未链接，不自动加载
			parent_ce = zend_lookup_class_ex(ce1->parent_name, NULL,
				ZEND_FETCH_CLASS_ALLOW_UNLINKED | ZEND_FETCH_CLASS_NO_AUTOLOAD);
		}

		// 只检查父类链是不够的，当父接口还没有复制过来时，需要一个完整的递归 检查 
		/* It's not sufficient to only check the parent chain itself, as need to do a full
		 * recursive instanceof in case the parent interfaces haven't been copied yet. */
		 
		// 如果有父类 并且 继承自 要求的类
		if (parent_ce && unlinked_instanceof(parent_ce, ce2)) {
			// 返回成功
			return 1;
		}
	}
	// 如果子类有实现接口
	if (ce1->num_interfaces) {
		uint32_t i;
		// 如果子类已完成接口复制
		if (ce1->ce_flags & ZEND_ACC_RESOLVED_INTERFACES) {
			// 不同于普通的 instanceof_function ，这里需要进行递归检查，因为接口可以没有完整复制
			/* Unlike the normal instanceof_function(), we have to perform a recursive
			 * check here, as the parent interfaces might not have been fully copied yet. */
			// 遍历所有接口
			for (i = 0; i < ce1->num_interfaces; i++) {
				// 如果接口继承自要求的父类
				if (unlinked_instanceof(ce1->interfaces[i], ce2)) {
					return 1;
				}
			}
		// 子类未完成接口复制
		} else {
			// 遍历所有接口
			for (i = 0; i < ce1->num_interfaces; i++) {
				// 查找接口。允许未链接，不自动加载
				zend_class_entry *ce = zend_lookup_class_ex(
					ce1->interface_names[i].name, ce1->interface_names[i].lc_name,
					ZEND_FETCH_CLASS_ALLOW_UNLINKED | ZEND_FETCH_CLASS_NO_AUTOLOAD);
				// 如果类实现自己，避免递归
				/* Avoid recursing if class implements itself. */
				// 如果接口有效，并且不是类本身 并且接口继承自要求的父类
				if (ce && ce != ce1 && unlinked_instanceof(ce, ce2)) {	
					// 返回真
					return 1;
				}
			}
		}
	}
	// 返回假
	return 0;
}

// ing3, 检验p3是否是p1的任何下属类型的子类，p2用于查找类名，p1:类型，p2:域，p3:self
static bool zend_type_permits_self(
		zend_type type, zend_class_entry *scope, zend_class_entry *self) {
			
	// 如果类型兼容任何对象，返回允许
	if (ZEND_TYPE_FULL_MASK(type) & MAY_BE_OBJECT) {
		return 1;
	}

	// 任何可能满足 self 的类型 必须已经加载好了，（用作 parent 或 接口）。所以这种情况不需要注册 延时变异责任
	/* Any types that may satisfy self must have already been loaded at this point
	 * (as a parent or interface), so we never need to register delayed variance obligations
	 * for this case. */
	zend_type *single_type;
	// 遍历子类型
	ZEND_TYPE_FOREACH(type, single_type) {
		// 如果下属类型有名称
		if (ZEND_TYPE_HAS_NAME(*single_type)) {
			// 获取类名，先处理parent和self, 如果都不是，用当前域类名。p1:当前域
			zend_string *name = resolve_class_name(scope, ZEND_TYPE_NAME(*single_type));
			// 用类名查找类，不延时加载。p1:当前域，p2:类名
			zend_class_entry *ce = lookup_class(self, name);
			// 如果当前域 是 ce 的子类，那它也是 type的子类
			if (ce && unlinked_instanceof(self, ce)) {
				// 返回允许
				return 1;
			}
		}
	} ZEND_TYPE_FOREACH_END();
	// 不允许
	return 0;
}

// ing3, 追踪类从属。步骤3是做什么用
static void track_class_dependency(zend_class_entry *ce, zend_string *class_name)
{
	HashTable *ht;
	// 必须有类名
	ZEND_ASSERT(class_name);
	// 步骤1，检查是否是当前域
	// 如果没有正在链接的类 或 此类就是正在链接的类
	if (!CG(current_linking_class) || ce == CG(current_linking_class)) {
		// 中断
		return;
	// 如果正在链接其他类。并且：当前类名是 self 或 parent（还是在操作当前类）
	} else if (zend_string_equals_literal_ci(class_name, "self")
	        || zend_string_equals_literal_ci(class_name, "parent")) {
		// 中断
		return;
	}

// 32位操作系统
#ifndef ZEND_WIN32
	// 非window操作系统中，内置类总是相同的，所以不需要显示追踪它们
	/* On non-Windows systems, internal classes are always the same,
	 * so there is no need to explicitly track them. */
	// 不是内置类才返回
	if (ce->type == ZEND_INTERNAL_CLASS) {
		return;
	}
#endif
	
	// 正在链接的类的 继承缓存
	ht = (HashTable*)CG(current_linking_class)->inheritance_cache;

	// 步骤2，检查类是否是 不可更改的
	if (!(ce->ce_flags & ZEND_ACC_IMMUTABLE)) {
		// TODO: dependency on not-immutable class ???
		
		// 如果有继承缓存
		if (ht) {
			// 销毁继承缓存
			zend_hash_destroy(ht);
			// 释放哈希表
			FREE_HASHTABLE(ht);
			// 清空继承缓存指针
			CG(current_linking_class)->inheritance_cache = NULL;
		}
		// 类标记成不可缓存
		CG(current_linking_class)->ce_flags &= ~ZEND_ACC_CACHEABLE;
		// 清空正在链接的类
		CG(current_linking_class) = NULL;
		return;
	}
	
	// 步骤3，
	// 如果类是不可更改的， 记从属关系
	/* Record dependency */
	// 如果继承缓存不存在
	if (!ht) {
		// 创建继承缓存哈希表
		ALLOC_HASHTABLE(ht);
		// 初始化哈希表
		zend_hash_init(ht, 0, NULL, NULL, 0);
		// 哈希表关联到正在连接的类上
		CG(current_linking_class)->inheritance_cache = (zend_inheritance_cache_entry*)ht;
	}
	// 继承缓存中添加此类
	zend_hash_add_ptr(ht, class_name, ce);
}

// ing3, 检查交叉类型 fe_type 里，是否有任何类 是 proto 的子类型。zend_is_intersection_subtype_of_type 调用。
/* Check whether any type in the fe_type intersection type is a subtype of the proto class. */
// 调用它的时候fe_type是定是单一或交叉类型，proto_class_name一定是单一类型。这个是最里层最简单的方法。
static inheritance_status zend_is_intersection_subtype_of_class(
		zend_class_entry *fe_scope, zend_type fe_type,
		zend_class_entry *proto_scope, zend_string *proto_class_name, zend_class_entry *proto_ce)
{
	// 必须是交叉类型
	ZEND_ASSERT(ZEND_TYPE_IS_INTERSECTION(fe_type));
	// 没有 未加载完成的类
	bool have_unresolved = false;
	// 遍历用的临时变量
	zend_type *single_type;
	
	// 遍历当前类型列表，检查：至少有一个类型是 父类型的下属类型（有交叉）
	/* Traverse the list of child types and check that at least one is
	 * a subtype of the parent type being checked */
	// 遍历子类型
	ZEND_TYPE_FOREACH(fe_type, single_type) {
		zend_class_entry *fe_ce;
		zend_string *fe_class_name = NULL;
		// 如果子类型有名称
		if (ZEND_TYPE_HAS_NAME(*single_type)) {
			// 获取类名，先处理parent和self, 如果都不是，用当前域类名。p1:当前域
			fe_class_name =
				resolve_class_name(fe_scope, ZEND_TYPE_NAME(*single_type));
			// 成功情况1：如果与 原型类 小写名称相同
			if (zend_string_equals_ci(fe_class_name, proto_class_name)) {
				// 返回：继承成功
				return INHERITANCE_SUCCESS;
			}
			// 用类名查找父类，不延时加载。p1:当前域，p2:类名
			if (!proto_ce) proto_ce = lookup_class(proto_scope, proto_class_name);
			
			// 用类名查找子类，不延时加载。p1:当前域，p2:类名
			fe_ce = lookup_class(fe_scope, fe_class_name);
			
		// 如果没有名称
		} else {
			// 标准类型不可以在交叉类型里，因为会触发 致命编译错误
			/* standard type in an intersection type is impossible,
			 * because it would be a fatal compile error */
			ZEND_UNREACHABLE();
			// 下一个类型
			continue;
		}
		// 如果类方法 和 原型方法都无效
		if (!fe_ce || !proto_ce) {
			// 有未加载完成的类
			have_unresolved = true;
			// 下一个
			continue;
		}
		// 成功情况2：如果两个函数的 所属类有 继承关系
		if (unlinked_instanceof(fe_ce, proto_ce)) {
			// 追踪类从属。上面没返回，应该就是为了这个了？
			track_class_dependency(fe_ce, fe_class_name);
			track_class_dependency(proto_ce, proto_class_name);
			// 返回继承成功
			return INHERITANCE_SUCCESS;
		}
	} ZEND_TYPE_FOREACH_END();

	// 有未加载完成的类 ？继承未完成 ： 继承出错
	return have_unresolved ? INHERITANCE_UNRESOLVED : INHERITANCE_ERROR;
}

// 检查一个单个 类原型 是一个潜在的复杂 fe_type的子类型
/* Check whether a single class proto type is a subtype of a potentially complex fe_type. */
// ing3, 检查子类型 是否被父类型兼容
static inheritance_status zend_is_class_subtype_of_type(
		zend_class_entry *fe_scope, zend_string *fe_class_name,
		zend_class_entry *proto_scope, zend_type proto_type) {
	zend_class_entry *fe_ce = NULL;
	bool have_unresolved = 0;

	// 如果父方法返回 object类型，任何 类 都满足 互转化测试
	/* If the parent has 'object' as a return type, any class satisfies the co-variant check */
	// 如果父类型允许 object
	if (ZEND_TYPE_FULL_MASK(proto_type) & MAY_BE_OBJECT) {
		// 目前，任何类都允许。还进行类查找是为了向前兼容的原因，未来可以会有不是类但有名称的类型（例如 typedef）
		/* Currently, any class name would be allowed here. We still perform a class lookup
		 * for forward-compatibility reasons, as we may have named types in the future that
		 * are not classes (such as typedefs). */
		 
		// 查找子类
		// 用类名查找类，不延时加载。p1:当前域，p2:类名
		if (!fe_ce) fe_ce = lookup_class(fe_scope, fe_class_name);
		// 如果子类不存在
		if (!fe_ce) {
			// 有未加载类
			have_unresolved = 1;
		// 如果存在
		} else {
			// 追踪类从属
			track_class_dependency(fe_ce, fe_class_name);
			// 返回成功
			return INHERITANCE_SUCCESS;
		}
	}

	// 循环变量
	zend_type *single_type;

	// 遍历父类型列表 并 检查 当前子类 是父类的子类型的 之一（union) 或 全部（交叉） 
	/* Traverse the list of parent types and check if the current child (FE)
	 * class is the subtype of at least one of them (union) or all of them (intersection). */
	 
	// 原型是否是交叉类型
	bool is_intersection = ZEND_TYPE_IS_INTERSECTION(proto_type);
	
	// 联合 类型有一个成就算成， 交叉 类型有一个失败就算失败
	// 所以过程中联合类型只可能报成功，交叉类型只可能报失败。
	// 联合类型的失败 和 交叉类型的成功，都要在遍历完成后才能得到（参看最后代码）。
	
	// 遍历父类型
	ZEND_TYPE_FOREACH(proto_type, single_type) {
		// 情况1， 如果下属类型是交叉类型
		if (ZEND_TYPE_IS_INTERSECTION(*single_type)) {
			// 递归
			inheritance_status subtype_status = zend_is_class_subtype_of_type(
				fe_scope, fe_class_name, proto_scope, *single_type);
			// 根据返回状态操作
			switch (subtype_status) {
				// 继承出错
				case INHERITANCE_ERROR:
					// 如果原型是交叉类型，只可能报出错。
					if (is_intersection) {
						// 返回继承出错 
						return INHERITANCE_ERROR;
					}
					// 如果不是交叉类型，下一个
					continue;
				// 继承未完成
				case INHERITANCE_UNRESOLVED:
					// 未完成
					have_unresolved = 1;
					// 下一个
					continue;
				// 继承成功
				case INHERITANCE_SUCCESS:
					// 如果原型不是交叉类型，才可能报成功。
					if (!is_intersection) {
						// 返回继承 成功
						return INHERITANCE_SUCCESS;
					}
					// 下一个
					continue;
				// 不会有其他状态，递归时所有的返回一定是这3个状态之一
				EMPTY_SWITCH_DEFAULT_CASE();
			}
		}
		
		// 情况2， 因为递归不会有第四个状态，所以到这里的一定是 联合，不是交叉类型	
		
		// 
		zend_class_entry *proto_ce;
		// 
		zend_string *proto_class_name = NULL;
		// 步骤2.1 如果类型有名称
		if (ZEND_TYPE_HAS_NAME(*single_type)) {
			// 获取类名，先处理parent和self, 如果都不是，用当前域类名。p1:当前域
			proto_class_name =
				resolve_class_name(proto_scope, ZEND_TYPE_NAME(*single_type));
			// 如果子类 和 父类名称相同
			if (zend_string_equals_ci(fe_class_name, proto_class_name)) {
				// 如果父不是交叉类型，才可能报成功。
				if (!is_intersection) {
					// 有一个匹配就可以，返回继承成功
					return INHERITANCE_SUCCESS;
				}
				// 是交叉类型，继续
				continue;
			}
			
			// 如果上刚刚没找到，还要再找，因为 track_class_dependency 时可能载入了
			// 用类名查找类，不延时加载。p1:当前域，p2:类名
			if (!fe_ce) fe_ce = lookup_class(fe_scope, fe_class_name);
			// 用类名查找类，不延时加载。p1:当前域，p2:类名
			proto_ce = lookup_class(proto_scope, proto_class_name);
			
		// 没有名称的是标准类型
		} else {
			// 标准类型，不可以有交叉。只能联合
			/* standard type */
			ZEND_ASSERT(!is_intersection);
			// 下一个
			continue;
		}
		
		// 步骤2.2， 如果父类 和 当前类 有一个没找到
		if (!fe_ce || !proto_ce) {
			// 未解决，有未加载的类
			have_unresolved = 1;
			// 继续
			continue;
		}
		
		// 步骤2.3， 如果子类是继承自父类
		if (unlinked_instanceof(fe_ce, proto_ce)) {
			// 追踪类从属
			track_class_dependency(fe_ce, fe_class_name);
			track_class_dependency(proto_ce, proto_class_name);
			// 如果不是交叉类型。才可能报成功。
			if (!is_intersection) {
				// 继承成功
				return INHERITANCE_SUCCESS;
			}
			// 交叉类型还要继续
		// 如果没有继承关系
		} else {
			// 如果是交叉类型。只可能报出错。
			if (is_intersection) {
				// 继承出错 
				return INHERITANCE_ERROR;
			}
		}
	} ZEND_TYPE_FOREACH_END();

	// 如果有未完成的
	if (have_unresolved) {
		// 返回继承未完成
		return INHERITANCE_UNRESOLVED;
	}
	// 如果是交叉类型 ？ 继承成功 ： 继承出错
	return is_intersection ? INHERITANCE_SUCCESS : INHERITANCE_ERROR;
}

// ing3, 从类型获取类名，p1:当前域, p2:类型
static zend_string *get_class_from_type(zend_class_entry *scope, zend_type single_type) {
	// 如果类型有名称
	if (ZEND_TYPE_HAS_NAME(single_type)) {
		// 获取类名，先处理parent和self, 如果都不是，用当前域类名。p1:当前域
		return resolve_class_name(scope, ZEND_TYPE_NAME(single_type));
	}
	// 类型没有名称，返回null
	return NULL;
}

// ing3, 查找并解决类型中的类，p1:当前域，p2:类型（或表）
static void register_unresolved_classes(zend_class_entry *scope, zend_type type) {
	zend_type *single_type;
	// 遍历子类型
	ZEND_TYPE_FOREACH(type, single_type) {
		// 如果子类型是列表
		if (ZEND_TYPE_HAS_LIST(*single_type)) {
			// 递归
			register_unresolved_classes(scope, *single_type);
			// 下一个子类型
			continue;
		}
		// 如果子类型有名称
		if (ZEND_TYPE_HAS_NAME(*single_type)) {
			// 获取类名，先处理parent和self, 如果都不是，用当前域类名。p1:当前域
			zend_string *class_name = resolve_class_name(scope, ZEND_TYPE_NAME(*single_type));
			// 用类名查找类，p1:当前域，p2:类名，p3:是否注册延时加载
			lookup_class_ex(scope, class_name, /* register_unresolved */ true);
		}
	} ZEND_TYPE_FOREACH_END();
}

// ing3, 检查 fe_type，是否是 proto_type 的子类型。 
// 这个调用的时候保证了 fe_type 是交叉类型，所以里面不用考虑它是联合类型。proto_type可能是交叉或联合类型。
static inheritance_status zend_is_intersection_subtype_of_type(
	zend_class_entry *fe_scope, zend_type fe_type,
	zend_class_entry *proto_scope, zend_type proto_type)
{
	bool have_unresolved = false;
	zend_type *single_type;
	// 17以下的纯类型
	uint32_t proto_type_mask = ZEND_TYPE_PURE_MASK(proto_type);

	// 当前, 对象类型兼容所有所有类， 为了向前兼容，还是要做一个类查找
	// 未来可能有命名的 非class类型（例如typedefs）
	/* Currently, for object type any class name would be allowed here.
	 * We still perform a class lookup for forward-compatibility reasons,
	 * as we may have named types in the future that are not classes
	 * (such as typedefs). */
	
	// 情况1，父类型包含对象，子类型只要是类就算成功
	// 如果原型类型 可以是对象
	if (proto_type_mask & MAY_BE_OBJECT) {
		// 遍历子类型 下属类型
		ZEND_TYPE_FOREACH(fe_type, single_type) {
			// 从类型获取类名，p1:当前域, p2:类型
			zend_string *fe_class_name = get_class_from_type(fe_scope, *single_type);
			// 如果找不到类名
			if (!fe_class_name) {
				// 下一个
				continue;
			}
			// 用类名查找类，不延时加载。p1:当前域，p2:类名
			zend_class_entry *fe_ce = lookup_class(fe_scope, fe_class_name);
			// 只要有一个子类型是类，就算成功
			if (fe_ce) {
				// 追踪类从属
				track_class_dependency(fe_ce, fe_class_name);
				// 继承成功
				return INHERITANCE_SUCCESS;
			// 没找到
			} else {
				// 未完成
				have_unresolved = true;
			}
		} ZEND_TYPE_FOREACH_END();
	}

	/* U_1&...&U_n < V_1&...&V_m if forall V_j. exists U_i. U_i < V_j.
	 * U_1&...&U_n < V_1|...|V_m if exists V_j. exists U_i. U_i < V_j.
	 * As such, we need to iterate over proto_type (V_j) first and use a different
	 * quantifier depending on whether fe_type is a union or an intersection. */
	
	// 可提前退出的状态。交叉类型有一个出错就可以退出，联合类型有一个成功就可以退出。
	// 。proto_type是交叉类型 ？ 继承出错 ： 继承成功
	inheritance_status early_exit_status =
		ZEND_TYPE_IS_INTERSECTION(proto_type) ? INHERITANCE_ERROR : INHERITANCE_SUCCESS;
	
	// 情况2，父类型不包括对象，或子类型暂时没找到对应类
	// 遍历原型中的每个子类型
	ZEND_TYPE_FOREACH(proto_type, single_type) {
		inheritance_status status;
		// 如果父类型的下属类型是交叉类型
		if (ZEND_TYPE_IS_INTERSECTION(*single_type)) {
			// 递归， 检查 fe_type，是否是 proto_type 的子类型
			status = zend_is_intersection_subtype_of_type(
				fe_scope, fe_type, proto_scope, *single_type);
				
		// 父类型的下属类型是单一 类型 
		// 下属类型里应该没有 联合类型了, 这里再联合就 和 上一层的联合作用一样了
		} else {
			// 从类型获取类名，p1:当前域, p2:类型
			zend_string *proto_class_name = get_class_from_type(proto_scope, *single_type);
			// 如果没有类名，下一个
			if (!proto_class_name) {
				continue;
			}

			zend_class_entry *proto_ce = NULL;
			// 检查 fe_type，是否满足联合类型 single_type
			status = zend_is_intersection_subtype_of_class(
				fe_scope, fe_type, proto_scope, proto_class_name, proto_ce);
		}
		
		// 可提前退出的状态。交叉类型有一个出错就可以退出，联合类型有一个成功就可以退出。
		// 当前状态 和 提前退出状态相同
		if (status == early_exit_status) {
			// 返回当前状态
			return status;
		}
		// 状态是 继承未完成
		if (status == INHERITANCE_UNRESOLVED) {
			// 有未加载的类
			have_unresolved = true;
		}
	} ZEND_TYPE_FOREACH_END();

	// 如果有未加载的类
	if (have_unresolved) {
		// 继承未完成
		return INHERITANCE_UNRESOLVED;
	}

	// 可提前退出的状态。交叉类型有一个出错就可以退出，联合类型有一个成功就可以退出。
	// 提前退出状态是 继承出错（父类型是交叉类型） ？ 继承成功 ：（父类型是联合类型）继承出错
	return early_exit_status == INHERITANCE_ERROR ? INHERITANCE_SUCCESS : INHERITANCE_ERROR;
}

// ing3, 检查 fe_type 是否可以作为 proto_scope 的子类型，两个都可以是复杂类型
static inheritance_status zend_perform_covariant_type_check(
		zend_class_entry *fe_scope, zend_type fe_type,
		zend_class_entry *proto_scope, zend_type proto_type)
{
	// 两个类型都不能是空的
	ZEND_ASSERT(ZEND_TYPE_IS_SET(fe_type) && ZEND_TYPE_IS_SET(proto_type));

	// 不同于void, 所有类型都可以兼容 mixed类型
	// 单独处理这种情况，来保证永远不需要加载类
	/* Apart from void, everything is trivially covariant to the mixed type.
	 * Handle this case separately to ensure it never requires class loading. */
	// 如果父类型允许所有 ,子类型不包含 void
	if (ZEND_TYPE_PURE_MASK(proto_type) == MAY_BE_ANY &&
			!ZEND_TYPE_CONTAINS_CODE(fe_type, IS_VOID)) {
		// 继承成功
		return INHERITANCE_SUCCESS;
	}

	// 内置类型可以被删除，但不能增加
	/* Builtin types may be removed, but not added */
	// 取得 纯类型
	uint32_t fe_type_mask = ZEND_TYPE_PURE_MASK(fe_type);
	uint32_t proto_type_mask = ZEND_TYPE_PURE_MASK(proto_type);
	// 追加类型 = 子类型 减掉 父类型
	uint32_t added_types = fe_type_mask & ~proto_type_mask;
	// 如果有追加类型
	if (added_types) {
		// 如果 类型 包含静态 并且 允许使用self
		if ((added_types & MAY_BE_STATIC)
				// 检验p3是否是p1的任何下属类型的子类，p2用于查找类名，p1:类型，p2:域，p3:self
				&& zend_type_permits_self(proto_type, proto_scope, fe_scope)) {
			
			// 把允许self的类型 替换成static是可以的
			/* Replacing type that accepts self with static is okay */
			
			// 删除 static 标记
			added_types &= ~MAY_BE_STATIC;
			// 这是在类里用的， class a{ function b():static {} } （测试过）
		}

		// 如果类型是never
		if (added_types == MAY_BE_NEVER) {
			// never 是最底类型
			/* never is the bottom type */
			// 返回继承成功
			return INHERITANCE_SUCCESS;
		}
		
		// 如果还有其他类型
		if (added_types) {
			// 覆盖新的类型 是非法的
			/* Otherwise adding new types is illegal */
			// 返回继承出错
			return INHERITANCE_ERROR;
		}
	}

	zend_type *single_type;
	inheritance_status early_exit_status;
	// 没有未加载类型
	bool have_unresolved = false;
	// 如果子类型是交叉类型。
	if (ZEND_TYPE_IS_INTERSECTION(fe_type)) {
		// 交叉类型实际上是一个类型，不用循环，一次验完
		// 原型是交叉类型，碰到错误马上返回错误。原型是联合类型，碰到成功马上返回成功。
		early_exit_status =
			ZEND_TYPE_IS_INTERSECTION(proto_type) ? INHERITANCE_ERROR : INHERITANCE_SUCCESS;
			
		// 这里已经保证了 fe_type 是交叉类型
		// 检查 fe_type，是否是 proto_type 的子类型
		inheritance_status status = zend_is_intersection_subtype_of_type(
			fe_scope, fe_type, proto_scope, proto_type);
			
		// 如果可以马上返回
		if (status == early_exit_status) {
			return status;
		}
		// 如果返回状态是未解决
		if (status == INHERITANCE_UNRESOLVED) {
			// 有未加载类
			have_unresolved = true;
		}
	// 子类型不是交叉类型，是联合类型或单个类型
	} else {
		/* U_1|...|U_n < V_1|...|V_m if forall U_i. exists V_j. U_i < V_j.
		 * U_1|...|U_n < V_1&...&V_m if forall U_i. forall V_j. U_i < V_j.
		 * We need to iterate over fe_type (U_i) first and the logic is independent of
		 * whether proto_type is a union or intersection (only the inner check differs). */
		
		// 子类型是联合类型，联合类型的每个下属类型都要兼容父类型，否则 继承出错
		early_exit_status = INHERITANCE_ERROR;
		// 遍历 子类型
		ZEND_TYPE_FOREACH(fe_type, single_type) {
			inheritance_status status;
			// 联合里 有交叉的下属类型
			/* Union has an intersection type as it's member */
			// 如果下属类型 是交叉类型
			if (ZEND_TYPE_IS_INTERSECTION(*single_type)) {
				// 这里也已经保证了 single_type 是交叉类型
				// 检查 检查 fe_type，是否是 proto_type 的子类型
				status = zend_is_intersection_subtype_of_type(
					fe_scope, *single_type, proto_scope, proto_type);
			// 下属类型不是交叉类型。是联合类型或单个类型。
			} else {
				// 从类型获取类名，p1:当前域, p2:类型
				zend_string *fe_class_name = get_class_from_type(fe_scope, *single_type);
				// 如果没找到此类，跳过
				if (!fe_class_name) {
					continue;
				}
				// 检查子类型 是否被父类型兼容
				status = zend_is_class_subtype_of_type(
					fe_scope, fe_class_name, proto_scope, proto_type);
			}
			
			// 如果可以返回
			if (status == early_exit_status) {
				// 返回当前状态
				return status;
			}
			// 如有1个未解决
			if (status == INHERITANCE_UNRESOLVED) {
				// 更新未解决标记
				have_unresolved = true;
			}
		} ZEND_TYPE_FOREACH_END();
	}
	// 如果没有未解决的
	if (!have_unresolved) {
		// 如果早先是 继承出错？返回 继承成功（因为没碰到出错），否则返回 继承出错（因为没碰到成功）
		return early_exit_status == INHERITANCE_ERROR ? INHERITANCE_SUCCESS : INHERITANCE_ERROR;
	}
	
	// 查找并解决类型中的类，p1:当前域，p2:类型（或表）
	register_unresolved_classes(fe_scope, fe_type);
	register_unresolved_classes(proto_scope, proto_type);
	// 返回继承未完成
	return INHERITANCE_UNRESOLVED;
}

// ing3, 检测子参数信息类型 是否可以继承父参数信息
static inheritance_status zend_do_perform_arg_type_hint_check(
		zend_class_entry *fe_scope, zend_arg_info *fe_arg_info,
		zend_class_entry *proto_scope, zend_arg_info *proto_arg_info) /* {{{ */
{
	// 定类型未定义 或者是 mixed ，直接返回成功
	if (!ZEND_TYPE_IS_SET(fe_arg_info->type) || ZEND_TYPE_PURE_MASK(fe_arg_info->type) == MAY_BE_ANY) {
		// 无类或mixed类型 永远是兼容的
		/* Child with no type or mixed type is always compatible */
		// 返回：继承成功
		return INHERITANCE_SUCCESS;
	}

	// 如果父类型不存在 ?? 这个要如何测试？
	if (!ZEND_TYPE_IS_SET(proto_arg_info->type)) {
		// 子属性有类型，父属性没有，违反 LSP
		/* Child defines a type, but parent doesn't, violates LSP */
		// 返回：继承出错
		return INHERITANCE_ERROR;
	}

	// 
	/* Contravariant type check is performed as a covariant type check with swapped
	 * argument order. */
	// 检查 fe_type 是否可以作为 proto_scope 的子类型，两个都可以是复杂类型
	return zend_perform_covariant_type_check(
		proto_scope, proto_arg_info->type, fe_scope, fe_arg_info->type);
}
/* }}} */

// 对于trait里的方法， fe_scope/proto_scope 可能与 fe/proto->common.scope 不同，
// self 会对应到引用 trait 的类，而不是定义方法的 trait
/* For trait methods, fe_scope/proto_scope may differ from fe/proto->common.scope,
 * as self will refer to the self of the class the trait is used in, not the trait
 * the method was declared in. */
 
 
// ing3, 方法实现检查，p1:方法，p2:作用域，p3:原型，p4:原型作用域
static inheritance_status zend_do_perform_implementation_check(
		const zend_function *fe, zend_class_entry *fe_scope,
		const zend_function *proto, zend_class_entry *proto_scope) /* {{{ */
{
	uint32_t i, num_args, proto_num_args, fe_num_args;
	inheritance_status status, local_status;
	bool proto_is_variadic, fe_is_variadic;

	// 步骤1，
	// 只有在接口中定义构造方法 或 者显式声名成抽象时，才检查构造方法
	/* Checks for constructors only if they are declared in an interface,
	 * or explicitly marked as abstract
	 */
	ZEND_ASSERT(!((fe->common.fn_flags & ZEND_ACC_CTOR)
		&& ((proto->common.scope->ce_flags & ZEND_ACC_INTERFACE) == 0
			&& (proto->common.fn_flags & ZEND_ACC_ABSTRACT) == 0)));

	// 步骤2，
	// 原型方法就是指父方法
	// 如果原型方法是私有 并且非抽象 ，不需要强制添加标记。私有抽象方法只有在 trait里才会存在。
	/* If the prototype method is private and not abstract, we do not enforce a signature.
	 * private abstract methods can only occur in traits. */
	// 必须 没有private 或 有abstract。（在普通类里定义私有抽象方法会直接报错，测试过）
	ZEND_ASSERT(!(proto->common.fn_flags & ZEND_ACC_PRIVATE)
			|| (proto->common.fn_flags & ZEND_ACC_ABSTRACT));

	// 步骤3，
	// 必须参数数量不能增加
	/* The number of required arguments cannot increase. */
	if (proto->common.required_num_args < fe->common.required_num_args) {
		// 返回：继承出错 
		return INHERITANCE_ERROR;
	}

	// 步骤4，
	// 如果父方法和子方法的引用返回限制要相同（测试过）
	/* by-ref constraints on return values are covariant */
	// 如果原型方法是引用返回 并且 当前方法不是引用返回
	if ((proto->common.fn_flags & ZEND_ACC_RETURN_REFERENCE)
		&& !(fe->common.fn_flags & ZEND_ACC_RETURN_REFERENCE)) {
		// 返回：继承出错 
		return INHERITANCE_ERROR;
	}

	// 步骤5，
	// 原型是否使用字典参数
	proto_is_variadic = (proto->common.fn_flags & ZEND_ACC_VARIADIC) != 0;
	// 子方法是否使用字典参数
	fe_is_variadic = (fe->common.fn_flags & ZEND_ACC_VARIADIC) != 0;

	// 父方法用字典参数，子方法也要用。否则报错
	/* A variadic function cannot become non-variadic */
	if (proto_is_variadic && !fe_is_variadic) {
		// 返回：继承错误
		return INHERITANCE_ERROR;
	}

	// 步骤6，
	// 字典参数不包含在参数数量里
	/* The variadic argument is not included in the stored argument count. */
	// 原型参数数量 + 字典参数
	proto_num_args = proto->common.num_args + proto_is_variadic;
	// 子方法参数数量 + 字典参数
	fe_num_args = fe->common.num_args + fe_is_variadic;
	// 两个里取大的一个
	num_args = MAX(proto_num_args, fe_num_args);

	// 默认继承成功
	status = INHERITANCE_SUCCESS;
	// 遍历每个参数
	for (i = 0; i < num_args; i++) {
		// 父方法参数信息。参数信息小于参数个数？使用参数信息：（原型是字典？使用返回值信息：null）
		zend_arg_info *proto_arg_info =
			i < proto_num_args ? &proto->common.arg_info[i] :
			proto_is_variadic ? &proto->common.arg_info[proto_num_args - 1] : NULL;
		// 子方法参数信息。参数信息小于参数个数？使用参数信息：（原型是字典？使用返回值信息：null）
		zend_arg_info *fe_arg_info =
			i < fe_num_args ? &fe->common.arg_info[i] :
			fe_is_variadic ? &fe->common.arg_info[fe_num_args - 1] : NULL;
		// 如果没有父方法参数信息
		if (!proto_arg_info) {
			// 添加一个新的可选参数 就好
			/* A new (optional) argument has been added, which is fine. */
			// 下一个参数
			continue;
		}
		// 如果没有子方法参数信息
		if (!fe_arg_info) {
			// 如果一个参数被删除了。这样的操作是非法的，因为 arity 检查工作基于一个这样的模型
			// 给函数传递多过定义数量的参数是错误的
			/* An argument has been removed. This is considered illegal, because arity checks
			 * work based on a model where passing more than the declared number of parameters
			 * to a function is an error. */
			// 返回：继承出错 
			return INHERITANCE_ERROR;
		}

		// 检测子参数信息类型 是否可以继承父参数信息
		local_status = zend_do_perform_arg_type_hint_check(
			fe_scope, fe_arg_info, proto_scope, proto_arg_info);

		// 如果状态不是 继承成功
		if (UNEXPECTED(local_status != INHERITANCE_SUCCESS)) {
			// 如果是继承出错
			if (UNEXPECTED(local_status == INHERITANCE_ERROR)) {
				// 返回继承出错
				return INHERITANCE_ERROR;
			}
			// 状态必须是继承未完成
			ZEND_ASSERT(local_status == INHERITANCE_UNRESOLVED);
			// 继承未完成（还要继续遍历）
			status = INHERITANCE_UNRESOLVED;
		}

		// 如果 参数的引用返回限制不一致（测试过，父类引用返回子类不是）
		/* by-ref constraints on arguments are invariant */
		if (ZEND_ARG_SEND_MODE(fe_arg_info) != ZEND_ARG_SEND_MODE(proto_arg_info)) {
			// 返回：继承出错 
			return INHERITANCE_ERROR;
		}
	}

	// 步骤7，
	// 检查返回类型兼容性，只有父方法有指定返回类型时才需要。新方法添加类型总是有效的。
	/* Check return type compatibility, but only if the prototype already specifies
	 * a return type. Adding a new return type is always valid. */
	 // 如果父方法有返回类型
	if (proto->common.fn_flags & ZEND_ACC_HAS_RETURN_TYPE) {
		// 不可以删除返回类型，除非父方法的返回类型是 tentative（测试过，null）
		/* Removing a return type is not valid, unless the parent return type is tentative. */
		
		// 如果子方法没有返回类型 （实际上这里不管父接口加不加允null，这里都报致使错误）
		if (!(fe->common.fn_flags & ZEND_ACC_HAS_RETURN_TYPE)) {
			// 如果父方法返回值不允null
			if (!ZEND_ARG_TYPE_IS_TENTATIVE(&proto->common.arg_info[-1])) {
				// 返回：继承出错
				return INHERITANCE_ERROR;
			}
			// 如果状态是 继承成功
			if (status == INHERITANCE_SUCCESS) {
				// 返回：继承警告
				return INHERITANCE_WARNING;
			}
			// 返回状态
			return status;
		}

		// 检查 fe_type 是否可以作为 proto_scope 的子类型，两个都可以是复杂类型
		local_status = zend_perform_covariant_type_check(
			fe_scope, fe->common.arg_info[-1].type, proto_scope, proto->common.arg_info[-1].type);

		// 如果本地状态不是：继承成功
		if (UNEXPECTED(local_status != INHERITANCE_SUCCESS)) {
			// 如果是继承出错 并且 返回值允null
			if (local_status == INHERITANCE_ERROR
					&& ZEND_ARG_TYPE_IS_TENTATIVE(&proto->common.arg_info[-1])) {
				// 状态：继承警告
				local_status = INHERITANCE_WARNING;
			}
			// 返回状态
			return local_status;
		}
	}
	// 返回状态
	return status;
}
/* }}} */


// ing3, 添加类型字串。p1:类型字串，p2:域（为self,parent），p3:参数信息，p4:是否是最后一个
static ZEND_COLD void zend_append_type_hint(
		smart_str *str, zend_class_entry *scope, zend_arg_info *arg_info, bool return_hint) /* {{{ */
{
	// 如果参数信息有类型
	if (ZEND_TYPE_IS_SET(arg_info->type)) {
		// 取回类型字串
		zend_string *type_str = zend_type_to_string_resolved(arg_info->type, scope);
		// 添加到str后面
		smart_str_append(str, type_str);
		// 释放类型字串
		zend_string_release(type_str);
		// 如果还没结束
		if (!return_hint) {
			// 再添加个空格
			smart_str_appendc(str, ' ');
		}
	}
}
/* }}} */

// 取得方法定义（php代码）
static ZEND_COLD zend_string *zend_get_function_declaration(
		const zend_function *fptr, zend_class_entry *scope) /* {{{ */
{
	smart_str str = {0};
	// 如果引用返回
	if (fptr->op_array.fn_flags & ZEND_ACC_RETURN_REFERENCE) {
		// 添加&
		smart_str_appends(&str, "& ");
	}
	// 有所属域
	if (fptr->common.scope) {
		// 属于匿名类
		if (fptr->common.scope->ce_flags & ZEND_ACC_ANON_CLASS) {
			/* cut off on NULL byte ... class@anonymous */
			smart_str_appends(&str, ZSTR_VAL(fptr->common.scope->name));
		// 不属于匿名类
		} else {
			smart_str_appendl(&str, ZSTR_VAL(fptr->common.scope->name), ZSTR_LEN(fptr->common.scope->name));
		}
		smart_str_appends(&str, "::");
	}

	smart_str_append(&str, fptr->common.function_name);
	smart_str_appendc(&str, '(');

	if (fptr->common.arg_info) {
		uint32_t i, num_args, required;
		zend_arg_info *arg_info = fptr->common.arg_info;

		required = fptr->common.required_num_args;
		num_args = fptr->common.num_args;
		if (fptr->common.fn_flags & ZEND_ACC_VARIADIC) {
			num_args++;
		}
		for (i = 0; i < num_args;) {
			// 添加类型字串。p1:类型字串，p2:域（为self,parent），p3:参数信息，p4:是否是最后一个
			zend_append_type_hint(&str, scope, arg_info, 0);

			if (ZEND_ARG_SEND_MODE(arg_info)) {
				smart_str_appendc(&str, '&');
			}

			if (ZEND_ARG_IS_VARIADIC(arg_info)) {
				smart_str_appends(&str, "...");
			}

			smart_str_appendc(&str, '$');
			if (fptr->type == ZEND_INTERNAL_FUNCTION) {
				smart_str_appends(&str, ((zend_internal_arg_info*)arg_info)->name);
			} else {
				smart_str_appendl(&str, ZSTR_VAL(arg_info->name), ZSTR_LEN(arg_info->name));
			}

			if (i >= required && !ZEND_ARG_IS_VARIADIC(arg_info)) {
				smart_str_appends(&str, " = ");

				if (fptr->type == ZEND_INTERNAL_FUNCTION) {
					if (((zend_internal_arg_info*)arg_info)->default_value) {
						smart_str_appends(&str, ((zend_internal_arg_info*)arg_info)->default_value);
					} else {
						smart_str_appends(&str, "<default>");
					}
				} else {
					zend_op *precv = NULL;
					{
						uint32_t idx  = i;
						zend_op *op = fptr->op_array.opcodes;
						zend_op *end = op + fptr->op_array.last;

						++idx;
						while (op < end) {
							if ((op->opcode == ZEND_RECV || op->opcode == ZEND_RECV_INIT)
									&& op->op1.num == (zend_ulong)idx)
							{
								precv = op;
							}
							++op;
						}
					}
					if (precv && precv->opcode == ZEND_RECV_INIT && precv->op2_type != IS_UNUSED) {
						zval *zv = RT_CONSTANT(precv, precv->op2);

						if (Z_TYPE_P(zv) == IS_FALSE) {
							smart_str_appends(&str, "false");
						} else if (Z_TYPE_P(zv) == IS_TRUE) {
							smart_str_appends(&str, "true");
						} else if (Z_TYPE_P(zv) == IS_NULL) {
							smart_str_appends(&str, "null");
						} else if (Z_TYPE_P(zv) == IS_STRING) {
							smart_str_appendc(&str, '\'');
							smart_str_appendl(&str, Z_STRVAL_P(zv), MIN(Z_STRLEN_P(zv), 10));
							if (Z_STRLEN_P(zv) > 10) {
								smart_str_appends(&str, "...");
							}
							smart_str_appendc(&str, '\'');
						} else if (Z_TYPE_P(zv) == IS_ARRAY) {
							if (zend_hash_num_elements(Z_ARRVAL_P(zv)) == 0) {
								smart_str_appends(&str, "[]");
							} else {
								smart_str_appends(&str, "[...]");
							}
						} else if (Z_TYPE_P(zv) == IS_CONSTANT_AST) {
							zend_ast *ast = Z_ASTVAL_P(zv);
							if (ast->kind == ZEND_AST_CONSTANT) {
								smart_str_append(&str, zend_ast_get_constant_name(ast));
							} else if (ast->kind == ZEND_AST_CLASS_CONST) {
								smart_str_append(&str, zend_ast_get_str(ast->child[0]));
								smart_str_appends(&str, "::");
								smart_str_append(&str, zend_ast_get_str(ast->child[1]));
							} else {
								smart_str_appends(&str, "<expression>");
							}
						} else {
							zend_string *tmp_zv_str;
							zend_string *zv_str = zval_get_tmp_string(zv, &tmp_zv_str);
							smart_str_append(&str, zv_str);
							zend_tmp_string_release(tmp_zv_str);
						}
					}
				}
			}

			if (++i < num_args) {
				smart_str_appends(&str, ", ");
			}
			arg_info++;
		}
	}

	smart_str_appendc(&str, ')');

	if (fptr->common.fn_flags & ZEND_ACC_HAS_RETURN_TYPE) {
		smart_str_appends(&str, ": ");
		// 添加类型字串。p1:类型字串，p2:域（为self,parent），p3:参数信息，p4:是否是最后一个
		zend_append_type_hint(&str, scope, fptr->common.arg_info - 1, 1);
	}
	smart_str_0(&str);

	return str.s;
}
/* }}} */

// ing3, 取得方法的文件名
static zend_always_inline zend_string *func_filename(const zend_function *fn) {
	// 如果是用户定义，返回文件名，否则返回空
	return fn->common.type == ZEND_USER_FUNCTION ? fn->op_array.filename : NULL;
}

// ing3, 取得方法的行号
static zend_always_inline uint32_t func_lineno(const zend_function *fn) {
	// 如果是用户定义，返回开始行号，否则返回0
	return fn->common.type == ZEND_USER_FUNCTION ? fn->op_array.line_start : 0;
}

// ing3, 报错：方法不兼容
static void ZEND_COLD emit_incompatible_method_error(
		const zend_function *child, zend_class_entry *child_scope,
		const zend_function *parent, zend_class_entry *parent_scope,
		inheritance_status status) {
	// 父方法的声名
	zend_string *parent_prototype = zend_get_function_declaration(parent, parent_scope);
	// 子方法的声名
	zend_string *child_prototype = zend_get_function_declaration(child, child_scope);
	
	// 如果状态为继承完成
	if (status == INHERITANCE_UNRESOLVED) {
		// TODO Improve error message if first unresolved class is present in child and parent?
		/* Fetch the first unresolved class from registered autoloads */
		//
		zend_string *unresolved_class = NULL;
		// 遍历延时加载，获得一个类名
		ZEND_HASH_MAP_FOREACH_STR_KEY(CG(delayed_autoloads), unresolved_class) {
			break;
		} ZEND_HASH_FOREACH_END();
		// 类名必须存在
		ZEND_ASSERT(unresolved_class);

		// 报错：不能查检兼容性，因为类 A 是无效的
		zend_error_at(E_COMPILE_ERROR, func_filename(child), func_lineno(child),
			"Could not check compatibility between %s and %s, because class %s is not available",
			ZSTR_VAL(child_prototype), ZSTR_VAL(parent_prototype), ZSTR_VAL(unresolved_class));
			
	// 如果状态为继承警告
	} else if (status == INHERITANCE_WARNING) {
		// 在属性表里 通过 键名 和 长度 查找属性, 偏移量为0
		zend_attribute *return_type_will_change_attribute = zend_get_attribute_str(
			child->common.attributes,
			"returntypewillchange",
			sizeof("returntypewillchange")-1
		);

		// 如果没找到修饰属性 returntypewillchange
		if (!return_type_will_change_attribute) {
			// 弃用 报错：返回类型需要和父类兼容 或 增加 #[\\ReturnTypeWillChange] 修饰属性，临时避免报错
			zend_error_at(E_DEPRECATED, func_filename(child), func_lineno(child),
				"Return type of %s should either be compatible with %s, "
				"or the #[\\ReturnTypeWillChange] attribute should be used to temporarily suppress the notice",
				ZSTR_VAL(child_prototype), ZSTR_VAL(parent_prototype));
			// 如果有异常
			if (EG(exception)) {
				// 在继承 A 类的过程中出错
				zend_exception_uncaught_error(
					"During inheritance of %s", ZSTR_VAL(parent_scope->name));
			}
		}
	// 其他情况
	} else {
		// 声名的子方法必须与父方法兼容
		zend_error_at(E_COMPILE_ERROR, func_filename(child), func_lineno(child),
			"Declaration of %s must be compatible with %s",
			ZSTR_VAL(child_prototype), ZSTR_VAL(parent_prototype));
	}
	// 释放定义字串
	zend_string_efree(child_prototype);
	zend_string_efree(parent_prototype);
}

// ing3, 允许延时实现 检查
static void perform_delayable_implementation_check(
		zend_class_entry *ce,
		const zend_function *fe, zend_class_entry *fe_scope,
		const zend_function *proto, zend_class_entry *proto_scope)
{
	// 方法实现检查，p1:方法，p2:作用域，p3:原型，p4:原型作用域
	inheritance_status status =
		zend_do_perform_implementation_check(fe, fe_scope, proto, proto_scope);
	// 如果未通过检查
	if (UNEXPECTED(status != INHERITANCE_SUCCESS)) {
		// 如果是 继承未完成
		if (EXPECTED(status == INHERITANCE_UNRESOLVED)) {
			// 添加兼容性责任
			add_compatibility_obligation(ce, fe, fe_scope, proto, proto_scope);
		// 如果是 继承出错
		} else {
			ZEND_ASSERT(status == INHERITANCE_ERROR || status == INHERITANCE_WARNING);
			// 报错：方法不兼容
			emit_incompatible_method_error(fe, fe_scope, proto, proto_scope, status);
		}
	}
}

// ing3, 对方法进行继承检查 。p1:子方法，p2:子类，p3:父方法，p4:父类，p5:?,p6:?,p7:检查可见性，p8,只检查，p9:检查
static zend_always_inline inheritance_status do_inheritance_check_on_method_ex(
		zend_function *child, zend_class_entry *child_scope,
		zend_function *parent, zend_class_entry *parent_scope,
		zend_class_entry *ce, zval *child_zv,
		bool check_visibility, bool check_only, bool checked) /* {{{ */
{
	uint32_t child_flags;
	uint32_t parent_flags = parent->common.fn_flags;
	zend_function *proto;

	// 步骤1，
	// 如果有 private 标记 并且 没有 abstract标记 并且 不是 构造方法
	if (UNEXPECTED((parent_flags & ZEND_ACC_PRIVATE) && !(parent_flags & ZEND_ACC_ABSTRACT) && !(parent_flags & ZEND_ACC_CTOR))) {
		// 如果不检验是否唯一？
		if (!check_only) {
			child->common.fn_flags |= ZEND_ACC_CHANGED;
		}
		// 父方法是私有的，并且不是 abstract方法，所以不用检验继承规则
		/* The parent method is private and not an abstract so we don't need to check any inheritance rules */
		// 继承成功
		return INHERITANCE_SUCCESS;
	}

	// 步骤2，
	// 如果不是检查 并且 父方法有 final 标记
	if (!checked && UNEXPECTED(parent_flags & ZEND_ACC_FINAL)) {
		// 
		if (check_only) {
			// 继承出错
			return INHERITANCE_ERROR;
		}
		// 不可以
		zend_error_at_noreturn(E_COMPILE_ERROR, func_filename(child), func_lineno(child),
			"Cannot override final method %s::%s()",
			ZEND_FN_SCOPE_NAME(parent), ZSTR_VAL(child->common.function_name));
	}

	// 步骤3，
	// 子方法标记
	child_flags	= child->common.fn_flags;
	/* You cannot change from static to non static and vice versa.
	 */
	// 如果不是检查 并且 父方法和子方法的静态标记不同
	if (!checked && UNEXPECTED((child_flags & ZEND_ACC_STATIC) != (parent_flags & ZEND_ACC_STATIC))) {
		// 如果只检查不报错
		if (check_only) {
			// 返回 继承出错
			return INHERITANCE_ERROR;
		}
		// 如果子方法有 static 标记
		if (child_flags & ZEND_ACC_STATIC) {
			// 不可以把非静态方法 变成静态方法
			zend_error_at_noreturn(E_COMPILE_ERROR, func_filename(child), func_lineno(child),
				"Cannot make non static method %s::%s() static in class %s",
				ZEND_FN_SCOPE_NAME(parent), ZSTR_VAL(child->common.function_name), ZEND_FN_SCOPE_NAME(child));
		// 如果没有
		} else {
			// 不可以把静态方法变成非静态方法
			zend_error_at_noreturn(E_COMPILE_ERROR, func_filename(child), func_lineno(child),
				"Cannot make static method %s::%s() non static in class %s",
				ZEND_FN_SCOPE_NAME(parent), ZSTR_VAL(child->common.function_name), ZEND_FN_SCOPE_NAME(child));
		}
	}

	// 步骤4，
	// 不可以继承后把方法变成abstract
	/* Disallow making an inherited method abstract. */
	// 如果不是检查 并且 子方法添加了 abstract
	if (!checked && UNEXPECTED((child_flags & ZEND_ACC_ABSTRACT) > (parent_flags & ZEND_ACC_ABSTRACT))) {
		// 如果只检查不报错
		if (check_only) {
			// 返回 继承出错
			return INHERITANCE_ERROR;
		}
		// 报错：不可以把非抽象 方法 变成抽象方法
		zend_error_at_noreturn(E_COMPILE_ERROR, func_filename(child), func_lineno(child),
			"Cannot make non abstract method %s::%s() abstract in class %s",
			ZEND_FN_SCOPE_NAME(parent), ZSTR_VAL(child->common.function_name), ZEND_FN_SCOPE_NAME(child));
	}

	// 步骤5，
	// 如果不是检查 如果 父方法是 private 或 变改变过
	if (!check_only && (parent_flags & (ZEND_ACC_PRIVATE|ZEND_ACC_CHANGED))) {
		child->common.fn_flags |= ZEND_ACC_CHANGED;
	}

	// 如果父方法有原型 ，使用原型，否则使用父方法本身
	proto = parent->common.prototype ?
		parent->common.prototype : parent;

	// 步骤6，
	// 如果父方法是构造方法
	if (parent_flags & ZEND_ACC_CTOR) {
		// 如果是抽象的， 构造方法只有工下原型。 这各情况，要检查对它的继承
		/* ctors only have a prototype if is abstract (or comes from an interface) */
		/* and if that is the case, we want to check inheritance against it */
		// 如果原型不是抽象方法
		if (!(proto->common.fn_flags & ZEND_ACC_ABSTRACT)) {
			// 继承成功
			return INHERITANCE_SUCCESS;
		}
		// 使用原型作为父方法
		parent = proto;
	}
	
	// 步骤7，创建子方法副本
	// 如果不是检查 
	if (!check_only && child->common.prototype != proto && child_zv) {
		// 执行1次
		do {
			// 如果子方法不属于当前类 并且 是用户定义的方法
			if (child->common.scope != ce && child->type == ZEND_USER_FUNCTION) {
				// 如果当前类是接口
				if (ce->ce_flags & ZEND_ACC_INTERFACE) {
					// 不同的父接口包含同样的方法名
					/* Few parent interfaces contain the same method */
					break;
				// 当前类不是接口
				} else {
					// op_array 还没被复制
					/* op_array wasn't duplicated yet */
					// 分配内存创建 op_array
					zend_function *new_function = zend_arena_alloc(&CG(arena), sizeof(zend_op_array));
					// 把子方法的 复制过来
					memcpy(new_function, child, sizeof(zend_op_array));
					// 
					Z_PTR_P(child_zv) = child = new_function;
				}
			}
			// 子方法的原型
			child->common.prototype = proto;
		} while (0);
	}

	// 步骤8，可见性检查
	// 阻止派生类 限制 父类中的有效访问
	/* Prevent derived classes from restricting access that was available in parent classes (except deriving from non-abstract ctors) */
	// 如果不是检查 并且需要 检查可见性 并且 子类的可见性大于父类
	if (!checked && check_visibility
			&& (child_flags & ZEND_ACC_PPP_MASK) > (parent_flags & ZEND_ACC_PPP_MASK)) {
		//  如果只检查 
		if (check_only) {
			// 返回：继承出错 
			return INHERITANCE_ERROR;
		}
		// 报错：子类的访问权限必须低于或等于父类
		zend_error_at_noreturn(E_COMPILE_ERROR, func_filename(child), func_lineno(child),
			"Access level to %s::%s() must be %s (as in class %s)%s",
			ZEND_FN_SCOPE_NAME(child), ZSTR_VAL(child->common.function_name), zend_visibility_string(parent_flags), ZEND_FN_SCOPE_NAME(parent), (parent_flags&ZEND_ACC_PUBLIC) ? "" : " or weaker");
	}
	
	// 步骤9，实现检查
	// 如果不是检查 
	if (!checked) {
		//  如果只检查不报错
		if (check_only) {
			// 方法实现检查，p1:方法，p2:作用域，p3:原型，p4:原型作用域
			return zend_do_perform_implementation_check(child, child_scope, parent, parent_scope);
		}
		// 允许延时实现 检查
		perform_delayable_implementation_check(ce, child, child_scope, parent, parent_scope);
	}
	return INHERITANCE_SUCCESS;
}
/* }}} */

// ing3, 对方法进行继承检查 。p1:子方法，p2:子类，p3:父方法，p4:父类，p5:?,p6:?,p7:检查可见性
static zend_never_inline void do_inheritance_check_on_method(
		zend_function *child, zend_class_entry *child_scope,
		zend_function *parent, zend_class_entry *parent_scope,
		zend_class_entry *ce, zval *child_zv, bool check_visibility)
{
	// 对方法进行继承检查 。p1:子方法，p2:子类，p3:父方法，p4:父类，p5:?,p6:?,p7:检查可见性，p8,只检查，p9:检查
	do_inheritance_check_on_method_ex(child, child_scope, parent, parent_scope, ce, child_zv, check_visibility, 0, 0);
}

// ing3, p1:方法名，p2:父方法，p3:此类，p4:接口，p5:是否检查
static zend_always_inline void do_inherit_method(zend_string *key, zend_function *parent, zend_class_entry *ce, bool is_interface, bool checked) /* {{{ */
{
	// 按方法名取出方法
	zval *child = zend_hash_find_known_hash(&ce->function_table, key);
	// 如果找到方法
	if (child) {
		// 取出此方法
		zend_function *func = (zend_function*)Z_PTR_P(child);
		// 如果是接口 并且 方法与父方法一致
		if (is_interface && UNEXPECTED(func == parent)) {
			// 接口中的同一个方法可能被 继承多次
			/* The same method in interface may be inherited few times */
			// 完成，返回
			return;
		}
		// 如果子方法与父方法不同，并且需要检查
		if (checked) {
			// 对方法进行继承检查 。p1:子方法，p2:子类，p3:父方法，p4:父类，p5:?,p6:?,p7:检查可见性，p8,只检查，p9:检查
			do_inheritance_check_on_method_ex(
				func, func->common.scope, parent, parent->common.scope, ce, child,
				/* check_visibility */ 1, 0, checked);
		// 如果不需要检查 
		} else {
			// 对方法进行继承检查 。p1:子方法，p2:子类，p3:父方法，p4:父类，p5:?,p6:?,p7:检查可见性
			do_inheritance_check_on_method(
				func, func->common.scope, parent, parent->common.scope, ce, child,
				/* check_visibility */ 1);
		}
	// 如果没找到
	} else {
		// 如果是接口 或 抽象类
		if (is_interface || (parent->common.fn_flags & (ZEND_ACC_ABSTRACT))) {
			// 类添加标记：隐式抽象类标记
			ce->ce_flags |= ZEND_ACC_IMPLICIT_ABSTRACT_CLASS;
		}
		// 为函数创建副本
		parent = zend_duplicate_function(parent, ce);
		// 如果不是接口
		if (!is_interface) {
			// 把父方法副本添加进函数 列表里
			_zend_hash_append_ptr(&ce->function_table, key, parent);
		// 如果是接口
		} else {
			// 添加新指针（有什么不同？）
			zend_hash_add_new_ptr(&ce->function_table, key, parent);
		}
	}
}
/* }}} */

// ing3, 检查属性类型是否兼容（双向可继承）
inheritance_status property_types_compatible(
		const zend_property_info *parent_info, const zend_property_info *child_info) {
	// 如果两个信息的纯类型码相同 并且 类型名称相同
	if (ZEND_TYPE_PURE_MASK(parent_info->type) == ZEND_TYPE_PURE_MASK(child_info->type)
			&& ZEND_TYPE_NAME(parent_info->type) == ZEND_TYPE_NAME(child_info->type)) {
		// 继承成功
		return INHERITANCE_SUCCESS;
	}
	
	// 如果一个有指定类型，一个没有
	if (ZEND_TYPE_IS_SET(parent_info->type) != ZEND_TYPE_IS_SET(child_info->type)) {
		// 继承出错
		return INHERITANCE_ERROR;
	}

	// 进行一个 双向类型检查 ，来决定 不变性
	/* Perform a covariant type check in both directions to determined invariance. */
	// 检查 fe_type 是否可以作为 proto_scope 的子类型，两个都可以是复杂类型
	inheritance_status status1 = zend_perform_covariant_type_check(
		child_info->ce, child_info->type, parent_info->ce, parent_info->type);
	inheritance_status status2 = zend_perform_covariant_type_check(
		parent_info->ce, parent_info->type, child_info->ce, child_info->type);
	
	// 如果双向都可以继承，返回继承成功
	if (status1 == INHERITANCE_SUCCESS && status2 == INHERITANCE_SUCCESS) {
		return INHERITANCE_SUCCESS;
	}
	// 如果有一个失败，返回失败
	if (status1 == INHERITANCE_ERROR || status2 == INHERITANCE_ERROR) {
		return INHERITANCE_ERROR;
	}
	// 其他情况，继承未完成
	ZEND_ASSERT(status1 == INHERITANCE_UNRESOLVED || status2 == INHERITANCE_UNRESOLVED);
	return INHERITANCE_UNRESOLVED;
}

// ing4, 报错：不兼容的属性
static void emit_incompatible_property_error(
		const zend_property_info *child, const zend_property_info *parent) {
	// 取得类型字串
	zend_string *type_str = zend_type_to_string_resolved(parent->type, parent->ce);
	// 编译错误：属性类型必须是 A （和在父类 B 一致）
	zend_error_noreturn(E_COMPILE_ERROR,
		"Type of %s::$%s must be %s (as in class %s)",
		ZSTR_VAL(child->ce->name),
		zend_get_unmangled_property_name(child->name),
		ZSTR_VAL(type_str),
		ZSTR_VAL(parent->ce->name));
}

// ing3, 继承属性信息
static void do_inherit_property(zend_property_info *parent_info, zend_string *key, zend_class_entry *ce) /* {{{ */
{
	// 子类属性信息表中查找此属性信息
	zval *child = zend_hash_find_known_hash(&ce->properties_info, key);
	// 
	zend_property_info *child_info;

	// 如果找到了
	if (UNEXPECTED(child)) {
		// 取出 属性信息
		child_info = Z_PTR_P(child);
		// 如果父属性有 private 或 【覆盖私有属性或方法】 标记
		if (parent_info->flags & (ZEND_ACC_PRIVATE|ZEND_ACC_CHANGED)) {
			// 子属性添加 【覆盖私有属性或方法】标记
			child_info->flags |= ZEND_ACC_CHANGED;
		}
		// 如果父属性信息没有私有标记
		if (!(parent_info->flags & ZEND_ACC_PRIVATE)) {
			// 如果父子属性信息的 static标记不同
			if (UNEXPECTED((parent_info->flags & ZEND_ACC_STATIC) != (child_info->flags & ZEND_ACC_STATIC))) {
				// 报错：不可以重新信息声名static标记不同的属性
				zend_error_noreturn(E_COMPILE_ERROR, "Cannot redeclare %s%s::$%s as %s%s::$%s",
					(parent_info->flags & ZEND_ACC_STATIC) ? "static " : "non static ", ZSTR_VAL(parent_info->ce->name), ZSTR_VAL(key),
					(child_info->flags & ZEND_ACC_STATIC) ? "static " : "non static ", ZSTR_VAL(ce->name), ZSTR_VAL(key));
			}
			// 如果父子属性信息的 readonly标记不同
			if (UNEXPECTED((child_info->flags & ZEND_ACC_READONLY) != (parent_info->flags & ZEND_ACC_READONLY))) {
				// 报错：不可以重新声名 readonly 标记不同的属性信息
				zend_error_noreturn(E_COMPILE_ERROR,
					"Cannot redeclare %s property %s::$%s as %s %s::$%s",
					parent_info->flags & ZEND_ACC_READONLY ? "readonly" : "non-readonly",
					ZSTR_VAL(parent_info->ce->name), ZSTR_VAL(key),
					child_info->flags & ZEND_ACC_READONLY ? "readonly" : "non-readonly",
					ZSTR_VAL(ce->name), ZSTR_VAL(key));
			}
			// 如果子类属性信息的 可见性 大于父类属性信息
			if (UNEXPECTED((child_info->flags & ZEND_ACC_PPP_MASK) > (parent_info->flags & ZEND_ACC_PPP_MASK))) {
				// 报错： 子类属性信息的 可见性 不可以大于父类属性信息
				zend_error_noreturn(E_COMPILE_ERROR, "Access level to %s::$%s must be %s (as in class %s)%s", ZSTR_VAL(ce->name), ZSTR_VAL(key), zend_visibility_string(parent_info->flags), ZSTR_VAL(parent_info->ce->name), (parent_info->flags&ZEND_ACC_PUBLIC) ? "" : " or weaker");
			// 可见性没问题， 如果子类没属性信息没有static标记
			} else if ((child_info->flags & ZEND_ACC_STATIC) == 0) {
				// 从偏移量获取 当前对象的(zval)序号
				int parent_num = OBJ_PROP_TO_NUM(parent_info->offset);
				int child_num = OBJ_PROP_TO_NUM(child_info->offset);
				
				// 不要把默认属性信信息保存在gc中（它们会被opcache释放掉）
				/* Don't keep default properties in GC (they may be freed by opcache) */
				// 销毁默认属性表中的父属性信息
				zval_ptr_dtor_nogc(&(ce->default_properties_table[parent_num]));
				// 把子属性信息添加到这个位置（子属性原来的位置没用了）
				ce->default_properties_table[parent_num] = ce->default_properties_table[child_num];
				// 清空原来的子属性信息位置
				ZVAL_UNDEF(&ce->default_properties_table[child_num]);
				// 子属性信息偏移量，复制原来的偏移量
				child_info->offset = parent_info->offset;
			}

			// 如果父类属性信息有类型
			if (UNEXPECTED(ZEND_TYPE_IS_SET(parent_info->type))) {
				// 检查属性类型是否兼容（双向可继承）
				inheritance_status status = property_types_compatible(parent_info, child_info);
				// 如果继承出错
				if (status == INHERITANCE_ERROR) {
					// 报错：不兼容的属性
					emit_incompatible_property_error(child_info, parent_info);
				}
				// 如果继承未解决
				if (status == INHERITANCE_UNRESOLVED) {
					// 添加 属性性信息兼容性责任。p1:所属类，p2:子属性信息，p3:父属性信息
					add_property_compatibility_obligation(ce, child_info, parent_info);
				}
			// 如果子属性有类型 并且 父属性无类型
			} else if (UNEXPECTED(ZEND_TYPE_IS_SET(child_info->type) && !ZEND_TYPE_IS_SET(parent_info->type))) {
				// 报错 属性不可定义成此类型（和父类一样）（测试过）
				zend_error_noreturn(E_COMPILE_ERROR,
						"Type of %s::$%s must not be defined (as in class %s)",
						ZSTR_VAL(ce->name),
						ZSTR_VAL(key),
						ZSTR_VAL(parent_info->ce->name));
			}
		}
	// 如果子类没有此属性信息
	} else {
		// 属性表里添加属性信息
		_zend_hash_append_ptr(&ce->properties_info, key, parent_info);
	}
}
/* }}} */

// ing3, 调用【接口本身】的实现方法进行处理
static inline void do_implement_interface(zend_class_entry *ce, zend_class_entry *iface) /* {{{ */
{
	// 当前类不是接口 并且 调用接口的检测方法检测是否可实现此接口
	if (!(ce->ce_flags & ZEND_ACC_INTERFACE) && iface->interface_gets_implemented && iface->interface_gets_implemented(iface, ce) == FAILURE) {
		// （trait、接口、枚举、类）不可以实现接口
		// 返回 首字母大写 对象类型（trait、接口、枚举、类）
		zend_error_noreturn(E_CORE_ERROR, "%s %s could not implement interface %s", zend_get_object_type_uc(ce), ZSTR_VAL(ce->name), ZSTR_VAL(iface->name));
	}
	// 这可能被类查找 逻辑 阻止
	/* This should be prevented by the class lookup logic. */
	ZEND_ASSERT(ce != iface);
}
/* }}} */

// ing3, 继承父类的接口
static void zend_do_inherit_interfaces(zend_class_entry *ce, const zend_class_entry *iface) /* {{{ */
{
	// 包含在当前类接口列表中的接口
	/* expects interface to be contained in ce's interface list already */
	// 父类的接口数量
	uint32_t i, ce_num, if_num = iface->num_interfaces;
	//
	zend_class_entry *entry;
	// 子类的接口数量
	ce_num = ce->num_interfaces;

	// 内置类
	if (ce->type == ZEND_INTERNAL_CLASS) {
		// 分配内存创建接口列表
		ce->interfaces = (zend_class_entry **) realloc(ce->interfaces, sizeof(zend_class_entry *) * (ce_num + if_num));
	// 用户类
	} else {
		// 调整接口列表大小
		ce->interfaces = (zend_class_entry **) erealloc(ce->interfaces, sizeof(zend_class_entry *) * (ce_num + if_num));
	}

	// 继承接口，如果它们还没被继承
	/* Inherit the interfaces, only if they're not already inherited by the class */
	// 倒着遍历
	while (if_num--) {
		// 取得接口
		entry = iface->interfaces[if_num];
		// 遍历已有接口
		for (i = 0; i < ce_num; i++) {
			// 如果找到了
			if (ce->interfaces[i] == entry) {
				// 跳出
				break;
			}
		}
		// 如果蹭没有跳出过
		if (i == ce_num) {
			// 把接口增加到列表最后
			ce->interfaces[ce->num_interfaces++] = entry;
		}
	}
	// 已解决接口列表
	ce->ce_flags |= ZEND_ACC_RESOLVED_INTERFACES;

	// 调用实现处理器
	/* and now call the implementing handlers */
	// 遍历每个接口
	while (ce_num < ce->num_interfaces) {
		// 调用【接口本身】的实现方法进行处理
		do_implement_interface(ce, ce->interfaces[ce_num++]);
	}
}
/* }}} */

// ing, 继承类常量
static void do_inherit_class_constant(zend_string *name, zend_class_constant *parent_const, zend_class_entry *ce) /* {{{ */
{
	// 从常量表中查找此常量
	zval *zv = zend_hash_find_known_hash(&ce->constants_table, name);
	zend_class_constant *c;

	// 如果找不到此常量
	if (zv != NULL) {
		// 常量
		c = (zend_class_constant*)Z_PTR_P(zv);
		// 如果子类常量的 可见性 大于父类
		if (UNEXPECTED((ZEND_CLASS_CONST_FLAGS(c) & ZEND_ACC_PPP_MASK) > (ZEND_CLASS_CONST_FLAGS(parent_const) & ZEND_ACC_PPP_MASK))) {
			// 报错：子类常量的访问级别必须小于父类
			zend_error_noreturn(E_COMPILE_ERROR, "Access level to %s::%s must be %s (as in class %s)%s",
				ZSTR_VAL(ce->name), ZSTR_VAL(name), zend_visibility_string(ZEND_CLASS_CONST_FLAGS(parent_const)), ZSTR_VAL(parent_const->ce->name), (ZEND_CLASS_CONST_FLAGS(parent_const) & ZEND_ACC_PUBLIC) ? "" : " or weaker");
		}
		// 如果父类常量有final标记
		if (UNEXPECTED((ZEND_CLASS_CONST_FLAGS(parent_const) & ZEND_ACC_FINAL))) {
			// 报错：不可以覆盖final常量
			zend_error_noreturn(
				E_COMPILE_ERROR, "%s::%s cannot override final constant %s::%s",
				ZSTR_VAL(ce->name), ZSTR_VAL(name), ZSTR_VAL(parent_const->ce->name), ZSTR_VAL(name)
			);
		}
	// 如果找到此常量，并且没有 private 标记
	} else if (!(ZEND_CLASS_CONST_FLAGS(parent_const) & ZEND_ACC_PRIVATE)) {
		// 如果父类常量是 常量表达式
		if (Z_TYPE(parent_const->value) == IS_CONSTANT_AST) {
			// 去掉 已更新常量 标记
			ce->ce_flags &= ~ZEND_ACC_CONSTANTS_UPDATED;
			// 添加，有表达式常量 标记
			ce->ce_flags |= ZEND_ACC_HAS_AST_CONSTANTS;
			// 如果此类不可更改
			if (ce->parent->ce_flags & ZEND_ACC_IMMUTABLE) {
				// arena 分配新常量
				c = zend_arena_alloc(&CG(arena), sizeof(zend_class_constant));
				// 复制父类常量
				memcpy(c, parent_const, sizeof(zend_class_constant));
				// 使用副本进行后继操作
				parent_const = c;
				// CONST_OWNED ：此常量应和类一起销毁 
				Z_CONSTANT_FLAGS(c->value) |= CONST_OWNED;
			}
		}
		// 如果是 final 类
		if (ce->type & ZEND_INTERNAL_CLASS) {
			// 分配内存创建常量
			c = pemalloc(sizeof(zend_class_constant), 1);
			// 复制父类常量
			memcpy(c, parent_const, sizeof(zend_class_constant));
			// 使用副本进行后继操作
			parent_const = c;
		}
		// 添加进哈希表里
		_zend_hash_append_ptr(&ce->constants_table, name, parent_const);
	}
}
/* }}} */

// ing3, 构造属性信息表
void zend_build_properties_info_table(zend_class_entry *ce)
{
	zend_property_info **table, *prop;
	size_t size;
	// 如果默认属性数量为0，直接返回
	if (ce->default_properties_count == 0) {
		return;
	}
	// 这时属性信息表还未创建
	ZEND_ASSERT(ce->properties_info_table == NULL);
	// 信息表大小 = 信息指针大小 * 默认属性个数
	size = sizeof(zend_property_info *) * ce->default_properties_count;
	// 如果是用户类
	if (ce->type == ZEND_USER_CLASS) {
		// 使用 arena 分配内存
		ce->properties_info_table = table = zend_arena_alloc(&CG(arena), size);
	// 内置类
	} else {
		// 默认方法分配内存，永久
		ce->properties_info_table = table = pemalloc(size, 1);
	}

	// 已删除的slots 可能还存在，所以继承时要清理干净
	/* Dead slots may be left behind during inheritance. Make sure these are NULLed out. */
	memset(table, 0, size);

	// 如果有父类 并且 父类的属性信息不是0
	if (ce->parent && ce->parent->default_properties_count != 0) {
		// 父类的属性信息表
		zend_property_info **parent_table = ce->parent->properties_info_table;
		// 先把父类的属性信息表复制过来
		memcpy(
			table, parent_table,
			sizeof(zend_property_info *) * ce->parent->default_properties_count
		);
		
		// 如果子类没有新加属性，这里就完成了
		/* Child did not add any new properties, we are done */
		if (ce->default_properties_count == ce->parent->default_properties_count) {
			return;
		}
	}

	// 遍历子类属性信息
	ZEND_HASH_MAP_FOREACH_PTR(&ce->properties_info, prop) {
		// 处理子类属性
		// 属性信息属于子类 并且 没有 static 标记
		if (prop->ce == ce && (prop->flags & ZEND_ACC_STATIC) == 0) {
			// 把属性信息添加进列表里
			// 从偏移量获取 当前对象的(zval)序号
			table[OBJ_PROP_TO_NUM(prop->offset)] = prop;
		}
	} ZEND_HASH_FOREACH_END();
}

// 大货，200行
// ing3, 继承
ZEND_API void zend_do_inheritance_ex(zend_class_entry *ce, zend_class_entry *parent_ce, bool checked) /* {{{ */
{
	zend_property_info *property_info;
	zend_function *func;
	zend_string *key;
	
	// ing4, 步骤1，先检验是否可继承
	// 如果是接口
	if (UNEXPECTED(ce->ce_flags & ZEND_ACC_INTERFACE)) {
		// 接口只能继承接口
		/* Interface can only inherit other interfaces */
		// 如果父类不是接口
		if (UNEXPECTED(!(parent_ce->ce_flags & ZEND_ACC_INTERFACE))) {
			// 报错：接口只能继承接口
			zend_error_noreturn(E_COMPILE_ERROR, "Interface %s cannot extend class %s", ZSTR_VAL(ce->name), ZSTR_VAL(parent_ce->name));
		}
	// 如果父类是 接口，trait，或有final标记
	} else if (UNEXPECTED(parent_ce->ce_flags & (ZEND_ACC_INTERFACE|ZEND_ACC_TRAIT|ZEND_ACC_FINAL))) {
		// 不可以继承final类
		/* Class must not extend a final class */
		// 如果类有final标记
		if (parent_ce->ce_flags & ZEND_ACC_FINAL) {
			// 报错：不可以继承final类
			zend_error_noreturn(E_COMPILE_ERROR, "Class %s cannot extend final class %s", ZSTR_VAL(ce->name), ZSTR_VAL(parent_ce->name));
		}
		
		// 类不可以继承 trait 或 接口
		/* Class declaration must not extend traits or interfaces */
		// 如果父类是接口 或 trait
		if ((parent_ce->ce_flags & ZEND_ACC_INTERFACE) || (parent_ce->ce_flags & ZEND_ACC_TRAIT)) {
			// 报错：不可以继承trait 或接口
			zend_error_noreturn(E_COMPILE_ERROR, "Class %s cannot extend %s %s",
				ZSTR_VAL(ce->name), parent_ce->ce_flags & ZEND_ACC_INTERFACE ? "interface" : "trait", ZSTR_VAL(parent_ce->name)
			);
		}
	}

	// 如果类的只读标记 与 父类 不同
	if (UNEXPECTED((ce->ce_flags & ZEND_ACC_READONLY_CLASS) != (parent_ce->ce_flags & ZEND_ACC_READONLY_CLASS))) {
		// 报错 只读类只能继承只读类，非只读类只能继承非只读类（测试过）
		zend_error_noreturn(E_COMPILE_ERROR, "%s class %s cannot extend %s class %s",
			ce->ce_flags & ZEND_ACC_READONLY_CLASS ? "Readonly" : "Non-readonly", ZSTR_VAL(ce->name),
			parent_ce->ce_flags & ZEND_ACC_READONLY_CLASS ? "readonly" : "non-readonly", ZSTR_VAL(parent_ce->name)
		);
	}
	// 如果类已经有父类名
	if (ce->parent_name) {
		// 释放掉
		zend_string_release_ex(ce->parent_name, 0);
	}
	// 关联父类
	ce->parent = parent_ce;
	// 添加标记【父类已创建】
	ce->ce_flags |= ZEND_ACC_RESOLVED_PARENT;

	// ing4, 步骤2，继承属性
	/* Inherit properties */
	if (parent_ce->default_properties_count) {
		zval *src, *dst, *end;
		// 如果有默认属性
		if (ce->default_properties_count) {
			// 创建属性表，大小是两个类的默认属性数量和。 内置类用持久内存。
			zval *table = pemalloc(sizeof(zval) * (ce->default_properties_count + parent_ce->default_properties_count), ce->type == ZEND_INTERNAL_CLASS);
			// 默认属性表的结尾
			src = ce->default_properties_table + ce->default_properties_count;
			// 新属性表，父属性结尾
			end = table + parent_ce->default_properties_count;
			// 新属性表结尾
			dst = end + ce->default_properties_count;
			// 应用新属性表（旧表解除关联）
			ce->default_properties_table = table;
			// 倒着遍历新属性表的子类属性部分(后半段）：从新属性表结尾 到 新属性表，父属性结尾
			do {
				// 左移
				dst--;
				src--;
				// 逐个把子类属性复制到新表里
				ZVAL_COPY_VALUE_PROP(dst, src);
			} while (dst != end);
			// 删除原属性表（已解除关联）
			pefree(src, ce->type == ZEND_INTERNAL_CLASS);
			// 结束位置为属性表的开头
			end = ce->default_properties_table;
			
		// 如果没有默认属性
		} else {
			// 按父类属性表大小分配内存，内置类永久分配
			end = pemalloc(sizeof(zval) * parent_ce->default_properties_count, ce->type == ZEND_INTERNAL_CLASS);
			// 复制开始位置，是此表的结束位置
			dst = end + parent_ce->default_properties_count;
			// 新属性表关联到类
			ce->default_properties_table = end;
		}
		
		// 两种情况都重新计算源位置，父类属性表的结尾位置
		src = parent_ce->default_properties_table + parent_ce->default_properties_count;
		// 父类的类型 不同于 新类的类型
		if (UNEXPECTED(parent_ce->type != ce->type)) {
			// 用户类继承内置类
			/* User class extends internal */
			// 同时倒序遍历新旧两个表，复制父类成员
			do {
				// 两个同时左移
				dst--;
				src--;
				// 复制每个类属性 给子类
				// 自动判断 ：持久对象增加引用次数，非持久对象创建副本。并复制 .u2.extra。
				ZVAL_COPY_OR_DUP_PROP(dst, src);
				// 如果此属性来自 常量表达式
				if (Z_OPT_TYPE_P(dst) == IS_CONSTANT_AST) {
					// 清除 【常量已更新】 标记
					ce->ce_flags &= ~ZEND_ACC_CONSTANTS_UPDATED;
					// 添加 【包含表达式属性】标记
					ce->ce_flags |= ZEND_ACC_HAS_AST_PROPERTIES;
				}
				// 这个continue 并没有什么用，因为它会跳到while语句，并不会造成死循环（测试过）
				continue;
			} while (dst != end);
		// 父类和子类的类型相同
		} else {
			do {
				// 两个同时左移
				dst--;
				src--;
				// 复制每个类属性 给子类
				ZVAL_COPY_PROP(dst, src);
				// 如果此属性来自 常量表达式
				if (Z_OPT_TYPE_P(dst) == IS_CONSTANT_AST) {
					// 清除 【常量已更新】 标记
					ce->ce_flags &= ~ZEND_ACC_CONSTANTS_UPDATED;
					// 添加 【包含表达式属性】标记
					ce->ce_flags |= ZEND_ACC_HAS_AST_PROPERTIES;
				}
				// 这个continue 并没有什么用，因为它会跳到while语句，并不会造成死循环（测试过）
				continue;
			} while (dst != end);
		}
		// 新类的属性数里，加上父类的属性数
		ce->default_properties_count += parent_ce->default_properties_count;
	}

	// ing4, 步骤3，继承静态属性
	// 如果父类有静态成员
	if (parent_ce->default_static_members_count) {
		zval *src, *dst, *end;
		// 如果子类有静态成员
		if (ce->default_static_members_count) {
			// 分配内存创建 zval表： 子类静态成员数量 + 父类静态成员数量 ，内置类永久分配
			zval *table = pemalloc(sizeof(zval) * (ce->default_static_members_count + parent_ce->default_static_members_count), ce->type == ZEND_INTERNAL_CLASS);
			// 旧表结尾处
			src = ce->default_static_members_table + ce->default_static_members_count;
			// 新表父类属性结尾处
			end = table + parent_ce->default_static_members_count;
			// 新表子类属性结尾处（后半段）
			dst = end + ce->default_static_members_count;
			// 使用新表（旧表解除关联）
			ce->default_static_members_table = table;
			// 同时倒序遍历新旧两个表，复制子类成员
			do {
				dst--;
				src--;
				// 旧表元素 复制给新表
				ZVAL_COPY_VALUE(dst, src);
			} while (dst != end);
			// 释放旧表
			pefree(src, ce->type == ZEND_INTERNAL_CLASS);
			// 新表的开始位置（也是倒序遍历的结束位置）
			end = ce->default_static_members_table;
		// 子类没有静态成员
		} else {
			// 分配内存创建 zval表： 父类静态成员数量，内置类永久分配
			end = pemalloc(sizeof(zval) * parent_ce->default_static_members_count, ce->type == ZEND_INTERNAL_CLASS);
			// 新表父元素结束位置（前半段）
			dst = end + parent_ce->default_static_members_count;
			// 使用新表（旧表解除关联）
			ce->default_static_members_table = end;
		}
		// 子类静态成员数，增加父类静态成员数
		src = parent_ce->default_static_members_table + parent_ce->default_static_members_count;
		// 同时倒序遍历新旧两个表，复制父类成员
		do {
			dst--;
			src--;
			// 如果是间接引用
			if (Z_TYPE_P(src) == IS_INDIRECT) {
				// 追踪到引用对象。再使用间接引用关联到新表
				ZVAL_INDIRECT(dst, Z_INDIRECT_P(src));
			// 不是间接引用
			} else {
				// 使用间接引用关联到新表
				ZVAL_INDIRECT(dst, src);
			}
			// 如果是常量表达式
			if (Z_TYPE_P(Z_INDIRECT_P(dst)) == IS_CONSTANT_AST) {
				// 清除 【常量已更新】 标记
				ce->ce_flags &= ~ZEND_ACC_CONSTANTS_UPDATED;
				// 添加 【包含静态表达式】标记
				ce->ce_flags |= ZEND_ACC_HAS_AST_STATICS;
			}
		} while (dst != end);
		// 更新 子类的静态成员数
		ce->default_static_members_count += parent_ce->default_static_members_count;
		// 如果没有静态成员指针列表
		if (!ZEND_MAP_PTR(ce->static_members_table)) {
			// 如果是静态类 并且 所属模块为持久模块
			if (ce->type == ZEND_INTERNAL_CLASS &&
					ce->info.internal.module->type == MODULE_PERSISTENT) {
				// 创建指针列表
				ZEND_MAP_PTR_NEW(ce->static_members_table);
			}
		}
	}

	// ing3, 步骤4，继承属性信息指针表
	// 遍历属性信息表
	ZEND_HASH_MAP_FOREACH_PTR(&ce->properties_info, property_info) {
		// 如果属性信息属于子类
		if (property_info->ce == ce) {
			// 如果属性信息有 static 标记
			if (property_info->flags & ZEND_ACC_STATIC) {
				// 计算此信息的偏移量，增加父类的静态信息数量
				property_info->offset += parent_ce->default_static_members_count;
			// 没有static标记
			} else {
				// 计算此信息的偏移量，增加父类的属性数量*zval大小
				property_info->offset += parent_ce->default_properties_count * sizeof(zval);
			}
		}
	} ZEND_HASH_FOREACH_END();

	// 步骤5，继承属性信息
	// 如果父类有属性信息
	if (zend_hash_num_elements(&parent_ce->properties_info)) {
		zend_hash_extend(&ce->properties_info,
			zend_hash_num_elements(&ce->properties_info) +
			zend_hash_num_elements(&parent_ce->properties_info), 0);
		// 遍历父类属性信息表
		ZEND_HASH_MAP_FOREACH_STR_KEY_PTR(&parent_ce->properties_info, key, property_info) {
			// 继承属性信息
			do_inherit_property(property_info, key, ce);
		} ZEND_HASH_FOREACH_END();
	}

	// 步骤6，继承常量表
	// 如果父类有常量表
	if (zend_hash_num_elements(&parent_ce->constants_table)) {
		zend_class_constant *c;
		// 把亲类常量表
		zend_hash_extend(&ce->constants_table,
			zend_hash_num_elements(&ce->constants_table) +
			zend_hash_num_elements(&parent_ce->constants_table), 0);
			
		// 遍历父类常量表
		ZEND_HASH_MAP_FOREACH_STR_KEY_PTR(&parent_ce->constants_table, key, c) {
			// 继承常量？
			do_inherit_class_constant(key, c, ce);
		} ZEND_HASH_FOREACH_END();
	}

	// 步骤7，继承方法表
	// 遍历父类方法表
	if (zend_hash_num_elements(&parent_ce->function_table)) {
		// 扩展新类的方法表 到 全部方法大小
		zend_hash_extend(&ce->function_table,
			zend_hash_num_elements(&ce->function_table) +
			zend_hash_num_elements(&parent_ce->function_table), 0);

		// 如果需要检查
		if (checked) {
			// 遍历父类方法表
			ZEND_HASH_MAP_FOREACH_STR_KEY_PTR(&parent_ce->function_table, key, func) {
				// 继承方法，需要检查
				do_inherit_method(key, func, ce, 0, 1);
			} ZEND_HASH_FOREACH_END();
		} else {
			// 遍历父类方法表
			ZEND_HASH_MAP_FOREACH_STR_KEY_PTR(&parent_ce->function_table, key, func) {
				// 继承方法，不需要检查
				do_inherit_method(key, func, ce, 0, 0);
			} ZEND_HASH_FOREACH_END();
		}
	}
	
	// 步骤8，继承构造方法
	// 继承父类的构造方法
	do_inherit_parent_constructor(ce);
	
	// 步骤9，继承接口
	// 如果子类是内置类
	if (ce->type == ZEND_INTERNAL_CLASS) {
		// 如果父类有实现接口
		if (parent_ce->num_interfaces) {
			// 继承父类的接口
			zend_do_inherit_interfaces(ce, parent_ce);
		}
		// 如果子类有 隐式抽象类标记
		if (ce->ce_flags & ZEND_ACC_IMPLICIT_ABSTRACT_CLASS) {
			// 子类 添加 显式抽象类标记
			ce->ce_flags |= ZEND_ACC_EXPLICIT_ABSTRACT_CLASS;
		}
	}
	
	// ing3, 步骤10，增加标记
	// 新类继承父类的类标记并添加 一些新标记 【？】
	// ZEND_HAS_STATIC_IN_METHODS：用户类有使用静态变量的成员方法
	// ZEND_ACC_HAS_TYPE_HINTS：函数有指定类型的参数 或 类有指定类型的属性
	// ZEND_ACC_USE_GUARDS：有魔术方法 __get/__set/__unset/__isset 的类，使用 guards
	// ZEND_ACC_NOT_SERIALIZABLE：类不可序列化或反序列化
	// ZEND_ACC_ALLOW_DYNAMIC_PROPERTIES：此类的实例可能有动态属性，不会触发弃用警告
	ce->ce_flags |= parent_ce->ce_flags & (ZEND_HAS_STATIC_IN_METHODS | ZEND_ACC_HAS_TYPE_HINTS | ZEND_ACC_USE_GUARDS | ZEND_ACC_NOT_SERIALIZABLE | ZEND_ACC_ALLOW_DYNAMIC_PROPERTIES);
}
/* }}} */

// ing3, 对值更新后，用===比较，值为对象的话必须指针相同
static zend_always_inline bool check_trait_property_or_constant_value_compatibility(zend_class_entry *ce, zval *op1, zval *op2) /* {{{ */
{
	bool is_compatible;
	zval op1_tmp, op2_tmp;

	// 如果值是常量，需要更新它
	/* if any of the values is a constant, we try to resolve it */
	// p1是常量表达式
	if (UNEXPECTED(Z_TYPE_P(op1) == IS_CONSTANT_AST)) {
		// 更新op1
		ZVAL_COPY_OR_DUP(&op1_tmp, op1);
		// 更新常量。p1:常量，p2:更新时用到的域
		zval_update_constant_ex(&op1_tmp, ce);
		// 使用更新后的常量
		op1 = &op1_tmp;
	}
	if (UNEXPECTED(Z_TYPE_P(op2) == IS_CONSTANT_AST)) {
		// 更新op2
		ZVAL_COPY_OR_DUP(&op2_tmp, op2);
		// 更新常量。p1:常量，p2:更新时用到的域
		zval_update_constant_ex(&op2_tmp, ce);
		// 使用更新后的常量
		op2 = &op2_tmp;
	}
	// op1，op2是否相同（===）
	is_compatible = fast_is_identical_function(op1, op2);

	// 如果op1没有创建副本
	if (op1 == &op1_tmp) {
		// 销毁 它
		zval_ptr_dtor_nogc(&op1_tmp);
	}
	// 如果op2没有创建副本
	if (op2 == &op2_tmp) {
		// 销毁 它
		zval_ptr_dtor_nogc(&op2_tmp);
	}
	// 返回是否兼容
	return is_compatible;
}
/* }}} */

// ing3, 继承常量前的检查，p1:类名，p2:父常量，p3:名称
static bool do_inherit_constant_check(
	zend_class_entry *ce, zend_class_constant *parent_constant, zend_string *name
) {
	// 从此类的常量表中按名称查找 
	zval *zv = zend_hash_find_known_hash(&ce->constants_table, name);
	// 如果没找到
	if (zv == NULL) {
		// 返回真，可以使用此名称
		return true;
	}

	// 如果找到，取出已有常量
	zend_class_constant *old_constant = Z_PTR_P(zv);
	// 如果父常量的所属类 和 已有常量不同 并且 父常量 有final标记
	if (parent_constant->ce != old_constant->ce && (ZEND_CLASS_CONST_FLAGS(parent_constant) & ZEND_ACC_FINAL)) {
		// 报错：不可以覆盖 有final 标记的 常量
		zend_error_noreturn(E_COMPILE_ERROR, "%s::%s cannot override final constant %s::%s",
			ZSTR_VAL(old_constant->ce->name), ZSTR_VAL(name),
			ZSTR_VAL(parent_constant->ce->name), ZSTR_VAL(name)
		);
	}
	// 如果已有常量所属类 不是父常量的所属类 并且 已有常量不属于此类
	if (old_constant->ce != parent_constant->ce && old_constant->ce != ce) {
		// 报错：冲突，有两个可继承的常量（可能来自接口和类，测试过）
		zend_error_noreturn(E_COMPILE_ERROR,
			"%s %s inherits both %s::%s and %s::%s, which is ambiguous",
			// 返回 首字母大写 对象类型（trait、接口、枚举、类）
			zend_get_object_type_uc(ce),
			// 
			ZSTR_VAL(ce->name),
			ZSTR_VAL(old_constant->ce->name), ZSTR_VAL(name),
			ZSTR_VAL(parent_constant->ce->name), ZSTR_VAL(name));
	}
	// 返回假
	return false;
}
/* }}} */

// ing3, 继承接口常量
static void do_inherit_iface_constant(zend_string *name, zend_class_constant *c, zend_class_entry *ce, zend_class_entry *iface) /* {{{ */
{
	// 继承常量前的检查，p1:类名，p2:父常量，p3:名称
	if (do_inherit_constant_check(ce, c, name)) {
		// 
		zend_class_constant *ct;
		// 如果值为常量表达式
		if (Z_TYPE(c->value) == IS_CONSTANT_AST) {
			// 类删除【已更新常量】标记
			ce->ce_flags &= ~ZEND_ACC_CONSTANTS_UPDATED;
			// 类添加【有常量】标记
			ce->ce_flags |= ZEND_ACC_HAS_AST_CONSTANTS;
			// 如果类不可更改
			if (iface->ce_flags & ZEND_ACC_IMMUTABLE) {
				// 分配内存创建新常量
				ct = zend_arena_alloc(&CG(arena), sizeof(zend_class_constant));
				// 把原常量复制过来
				memcpy(ct, c, sizeof(zend_class_constant));
				// 使用新常量
				c = ct;
				// 【常量跟随类一起销毁】
				Z_CONSTANT_FLAGS(c->value) |= CONST_OWNED;
			}
		}
		// 如果是内置类
		if (ce->type & ZEND_INTERNAL_CLASS) {
			// 持久分配常量
			ct = pemalloc(sizeof(zend_class_constant), 1);
			// 把原常量复制过来
			memcpy(ct, c, sizeof(zend_class_constant));
			// 使用新常量
			c = ct;
		}
		// 把新常量添加到常量表里
		zend_hash_update_ptr(&ce->constants_table, name, c);
	}
	// 不可继承的直接跳过了，不会报错
}
/* }}} */

// ing3, 接口实现
static void do_interface_implementation(zend_class_entry *ce, zend_class_entry *iface) /* {{{ */
{
	zend_function *func;
	zend_string *key;
	zend_class_constant *c;
	// 遍历此接口的常量表
	ZEND_HASH_MAP_FOREACH_STR_KEY_PTR(&iface->constants_table, key, c) {
		// 逐个继承常量
		do_inherit_iface_constant(key, c, ce, iface);
	} ZEND_HASH_FOREACH_END();

	// 遍历 此接口的方法表
	ZEND_HASH_MAP_FOREACH_STR_KEY_PTR(&iface->function_table, key, func) {
		// 逐个继承方法,p4:方法属于接口, 不需要检查
		do_inherit_method(key, func, ce, 1, 0);
	} ZEND_HASH_FOREACH_END();
	// 调用【接口本身】的实现方法进行处理
	do_implement_interface(ce, iface);
	// 如果有多个接口
	if (iface->num_interfaces) {
		// 继承父类的接口
		zend_do_inherit_interfaces(ce, iface);
	}
}
/* }}} */

// ing3, 实现单个接口
ZEND_API void zend_do_implement_interface(zend_class_entry *ce, zend_class_entry *iface) /* {{{ */
{
	uint32_t i, ignore = 0;
	uint32_t current_iface_num = ce->num_interfaces;
	uint32_t parent_iface_num  = ce->parent ? ce->parent->num_interfaces : 0;
	zend_string *key;
	zend_class_constant *c;

	ZEND_ASSERT(ce->ce_flags & ZEND_ACC_LINKED);
	// 遍历所有接口
	for (i = 0; i < ce->num_interfaces; i++) {
		// 如果无效
		if (ce->interfaces[i] == NULL) {
			// 接口列表左移一个元素，把空的填上
			memmove(ce->interfaces + i, ce->interfaces + i + 1, sizeof(zend_class_entry*) * (--ce->num_interfaces - i));
			// 再检查这个位置
			i--;
		// 如果接口有效，并且是要实现的接口
		} else if (ce->interfaces[i] == iface) {
			// 如果是父类的接口
			if (EXPECTED(i < parent_iface_num)) {
				// 忽略标记（父类的接口可以再实现一次,测试过）
				ignore = 1;
			// 不是父接口
			} else {
				// 报错 类 A 不能实现前面实现过的接口
				zend_error_noreturn(E_COMPILE_ERROR, "Class %s cannot implement previously implemented interface %s", ZSTR_VAL(ce->name), ZSTR_VAL(iface->name));
			}
		}
	}
	// 忽略（父类实现过的接口）
	if (ignore) {
		// 检查 尝试重新定义接口常量
		/* Check for attempt to redeclare interface constants */
		ZEND_HASH_MAP_FOREACH_STR_KEY_PTR(&iface->constants_table, key, c) {
			// 继承常量前的检查，p1:类名，p2:父常量，p3:名称
			do_inherit_constant_check(ce, c, key);
		} ZEND_HASH_FOREACH_END();
	// 不忽略
	} else {
		// 如果列表不够长了
		if (ce->num_interfaces >= current_iface_num) {
			// 如果 是内置类
			if (ce->type == ZEND_INTERNAL_CLASS) {
				// 原生方法，调整原接口列表大小
				ce->interfaces = (zend_class_entry **) realloc(ce->interfaces, sizeof(zend_class_entry *) * (++current_iface_num));
			// 用户类
			} else {
				// zend方法，调整原接口列表大小
				ce->interfaces = (zend_class_entry **) erealloc(ce->interfaces, sizeof(zend_class_entry *) * (++current_iface_num));
			}
		}
		// 把接口添加到列表里
		ce->interfaces[ce->num_interfaces++] = iface;
		// 实现接口 ~
		do_interface_implementation(ce, iface);
	}
}
/* }}} */

// ing3, 实现多个接口
static void zend_do_implement_interfaces(zend_class_entry *ce, zend_class_entry **interfaces) /* {{{ */
{
	zend_class_entry *iface;
	// 父类接口数量，这个数不动
	uint32_t num_parent_interfaces = ce->parent ? ce->parent->num_interfaces : 0;
	// 自己的接口数量，这个数会增加
	uint32_t num_interfaces = num_parent_interfaces;
	zend_string *key;
	zend_class_constant *c;
	uint32_t i, j;

	// 遍历每个接口
	for (i = 0; i < ce->num_interfaces; i++) {
		// 跳过父接口，获取一个自己实现的接口
		iface = interfaces[num_parent_interfaces + i];
		// 如果接口未链接
		if (!(iface->ce_flags & ZEND_ACC_LINKED)) {
			// 添加从属责任。（从属责任对象关关联到从属类）
			add_dependency_obligation(ce, iface);
		}
		// 如果找到的不是接口
		if (UNEXPECTED(!(iface->ce_flags & ZEND_ACC_INTERFACE))) {
			// 释放全部接口
			efree(interfaces);
			// 报错：不可实现它，它不是接口
			zend_error_noreturn(E_ERROR, "%s cannot implement %s - it is not an interface", ZSTR_VAL(ce->name), ZSTR_VAL(iface->name));
			// 中断
			return;
		}
		// 遍历所有前面的接口
		for (j = 0; j < num_interfaces; j++) {
			// 看前面是不是已经实现过了
			if (interfaces[j] == iface) {
				
				// 如果不是父类接口（是自己实现的接口）
				if (j >= num_parent_interfaces) {
					// 释放全部接口
					efree(interfaces);
					// 不可实现前面已经实现过的接口
					zend_error_noreturn(E_COMPILE_ERROR, "%s %s cannot implement previously implemented interface %s",
						// 返回 首字母大写 对象类型（trait、接口、枚举、类）
						zend_get_object_type_uc(ce),
						ZSTR_VAL(ce->name),
						ZSTR_VAL(iface->name));
					// 中断
					return;
				}
				
				// 是父类的接口，和 zend_do_implement_interface 方法一样，只更新常量
				
				// 跳过重复的
				/* skip duplications */
				// 遍历接口常量
				ZEND_HASH_MAP_FOREACH_STR_KEY_PTR(&iface->constants_table, key, c) {
					// 继承常量前的检查，p1:类名，p2:父常量，p3:名称
					do_inherit_constant_check(ce, c, key);
				} ZEND_HASH_FOREACH_END();
				// 清空此 重复的 接口
				iface = NULL;
				// 跳出
				break;
			}
		}
		// 如果接口存在
		if (iface) {
			// 放进接口列表里
			interfaces[num_interfaces] = iface;
			// 接口数量增加
			num_interfaces++;
		}
	}

	// 如果ce没被修改过
	if (!(ce->ce_flags & ZEND_ACC_CACHED)) {
		// 遍历当前类的所有接口
		for (i = 0; i < ce->num_interfaces; i++) {
			// 释放接口名
			zend_string_release_ex(ce->interface_names[i].name, 0);
			// 释放小写接口名
			zend_string_release_ex(ce->interface_names[i].lc_name, 0);
		}
		// 释放所有接口名
		efree(ce->interface_names);
	}

	// 更新接口数量 
	ce->num_interfaces = num_interfaces;
	// 更新接口列表
	ce->interfaces = interfaces;
	// 已完成接口实现
	ce->ce_flags |= ZEND_ACC_RESOLVED_INTERFACES;

	// 遍历所有父类接口
	for (i = 0; i < num_parent_interfaces; i++) {
		// 调用接口的检测方法检测是否可实现此接口
		do_implement_interface(ce, ce->interfaces[i]);
	}
	
	// 在循环继承中，可能加入新接口
	// 使用 num_interfaces 而不是 ce->num_interfaces 不要重新处理新接口。
	/* Note that new interfaces can be added during this loop due to interface inheritance.
	 * Use num_interfaces rather than ce->num_interfaces to not re-process the new ones. */
	 
	// 继续处理自己实现的接口
	for (; i < num_interfaces; i++) {
		// 接口实现
		do_interface_implementation(ce, ce->interfaces[i]);
	}
}
/* }}} */

// ing4, 返回fn的作用域，如果它属于trait，返回p2
static zend_class_entry *fixup_trait_scope(const zend_function *fn, zend_class_entry *ce)
{
	// trait中的self应被转换成引用它的类，而不是trait本身
	/* self in trait methods should be resolved to the using class, not the trait. */
	
	// 如果所属域是 trait ？使用ce ：使用所属域
	return fn->common.scope->ce_flags & ZEND_ACC_TRAIT ? ce : fn->common.scope;
}

// ing3, 添加trait中的方法， p1:类
static void zend_add_trait_method(zend_class_entry *ce, zend_string *name, zend_string *key, zend_function *fn) /* {{{ */
{
	zend_function *existing_fn = NULL;
	zend_function *new_fn;

	// 如果用key找到方法
	if ((existing_fn = zend_hash_find_ptr(&ce->function_table, key)) != NULL) {
		// 如果是同一个方法，并且有相同的可见性，并且没有被赋值所属类，无论它来自哪里，都没有冲突，也不需要再次添加它
		/* if it is the same function with the same visibility and has not been assigned a class scope yet, regardless
		 * of where it is coming from there is no conflict and we do not need to add it again */
		// 如果新旧方法的操作码是同一串，并且可见性相同 并且 方法已经有 trait 标记
		if (existing_fn->op_array.opcodes == fn->op_array.opcodes &&
			(existing_fn->common.fn_flags & ZEND_ACC_PPP_MASK) == (fn->common.fn_flags & ZEND_ACC_PPP_MASK) &&
			(existing_fn->common.scope->ce_flags & ZEND_ACC_TRAIT) == ZEND_ACC_TRAIT) {
			// 添加过了，直接返回
			return;
		}

		// trait里带的抽象方法标记 必须要满足
		/* Abstract method signatures from the trait must be satisfied. */
		if (fn->common.fn_flags & ZEND_ACC_ABSTRACT) {
			// trait里的 “抽象私有”方法 在 php8里不优先
			// 因此，“抽象受保护”有时用于指出trait要求，即使正在实现的方法是私有的
			// 不要检查可见性要求 来 保持 这个用法 向后兼容 
			/* "abstract private" methods in traits were not available prior to PHP 8.
			 * As such, "abstract protected" was sometimes used to indicate trait requirements,
			 * even though the "implementing" method was private. Do not check visibility
			 * requirements to maintain backwards-compatibility with such usage.
			 */
			// 对方法进行继承检查 。p1:子方法，p2:子类，p3:父方法，p4:父类，p5:?,p6:?,p7:检查可见性
			do_inheritance_check_on_method(
				// 返回fn的作用域，如果它属于trait，返回p2
				existing_fn, fixup_trait_scope(existing_fn, ce), fn, fixup_trait_scope(fn, ce),
				ce, NULL, /* check_visibility */ 0);
			// 抽象方法检查通过就好，不用做其他事
			return;
		}

		// 原有方法属于当前类
		if (existing_fn->common.scope == ce) {
			// 当前类方法覆盖trait方法
			/* members from the current class override trait methods */
			
			// 中断
			return;
		// 原有方法属于trait 并且 不是抽象的
		} else if (UNEXPECTED((existing_fn->common.scope->ce_flags & ZEND_ACC_TRAIT)
				&& !(existing_fn->common.fn_flags & ZEND_ACC_ABSTRACT))) {
			// 两个trait不可以定义同样名称的非抽象方法
			/* two traits can't define the same non-abstract method */
			
			// 报错：两个trait 用同名的方法，产生的冲突
			zend_error_noreturn(E_COMPILE_ERROR, "Trait method %s::%s has not been applied as %s::%s, because of collision with %s::%s",
				ZSTR_VAL(fn->common.scope->name), ZSTR_VAL(fn->common.function_name),
				ZSTR_VAL(ce->name), ZSTR_VAL(name),
				ZSTR_VAL(existing_fn->common.scope->name), ZSTR_VAL(existing_fn->common.function_name));
				
		// 如果是继承来的方法，要被trait方法覆盖
		} else {
			// 继承成员被traits插入的成员覆盖，检验trait方法是否满足继承要求（trait方法也要满足继承要求才行）
			/* Inherited members are overridden by members inserted by traits.
			 * Check whether the trait method fulfills the inheritance requirements. */
			 
			// 对方法进行继承检查 。p1:子方法，p2:子类，p3:父方法，p4:父类，p5:?,p6:?,p7:检查可见性
			do_inheritance_check_on_method(
				// 返回fn的作用域，如果它属于trait，返回p2
				fn, fixup_trait_scope(fn, ce), existing_fn, fixup_trait_scope(existing_fn, ce),
				ce, NULL, /* check_visibility */ 1);
		}
	}

	// 没有旧方法 或 通过了继承检验
	// 此方法是内置函数
	if (UNEXPECTED(fn->type == ZEND_INTERNAL_FUNCTION)) {
		// 创建新内置函数
		new_fn = zend_arena_alloc(&CG(arena), sizeof(zend_internal_function));
		// 复制此方法
		memcpy(new_fn, fn, sizeof(zend_internal_function));
		// 新方法添加 ARENA 标记
		new_fn->common.fn_flags |= ZEND_ACC_ARENA_ALLOCATED;
	} else {
		// 创建新内置函数
		new_fn = zend_arena_alloc(&CG(arena), sizeof(zend_op_array));
		// 复制此方法
		memcpy(new_fn, fn, sizeof(zend_op_array));
		// 新方法添加 【从trait复制】标记
		new_fn->op_array.fn_flags |= ZEND_ACC_TRAIT_CLONE;
		// 新方法删除“不可更改”标记
		new_fn->op_array.fn_flags &= ~ZEND_ACC_IMMUTABLE;
	}
	// 重新分配方法名，这时它是个别名
	/* Reassign method name, in case it is an alias. */
	// 更新新函数方法名
	new_fn->common.function_name = name;
	// 方法增加引用次数
	function_add_ref(new_fn);
	// 更新函数表中名称为 key 的方法
	fn = zend_hash_update_ptr(&ce->function_table, key, new_fn);
	// 添加魔术方法,13个，p1:类，p2:函数，p3:魔术方法名
	zend_add_magic_method(ce, fn, key);
}
/* }}} */

// ing3, 绑定 trait中的方法后，给类添加相应的标记
static void zend_fixup_trait_method(zend_function *fn, zend_class_entry *ce) /* {{{ */
{
	// 如果方法属于trait
	if ((fn->common.scope->ce_flags & ZEND_ACC_TRAIT) == ZEND_ACC_TRAIT) {
		// 如果所属域是当前类
		fn->common.scope = ce;
		// 如果是抽象方法
		if (fn->common.fn_flags & ZEND_ACC_ABSTRACT) {
			// 给类添加隐匿抽象标记
			ce->ce_flags |= ZEND_ACC_IMPLICIT_ABSTRACT_CLASS;
		}
		// 如果方法是用户定义 并且 里面有静态变量
		if (fn->type == ZEND_USER_FUNCTION && fn->op_array.static_variables) {
			// 类添加标记：方法中有静态变量
			ce->ce_flags |= ZEND_HAS_STATIC_IN_METHODS;
		}
	}
}
/* }}} */

// ing3, 复制trait里的方法 p1:方法名，p2:方法，p3:类，p4:排除列表，p5:trait别名列表
static void zend_traits_copy_functions(zend_string *fnname, zend_function *fn, zend_class_entry *ce, HashTable *exclude_table, zend_class_entry **aliases) /* {{{ */
{
	zend_trait_alias  *alias, **alias_ptr;
	zend_string       *lcname;
	zend_function      fn_copy;
	int                i;
	// 使用被类名限制的别名，不会出现歧义
	/* apply aliases which are qualified with a class name, there should not be any ambiguity */
	
	// 先处理trait别名
	// trait别名指针列表 （zend_trait_alias 指针列表）
	if (ce->trait_aliases) {
		// 别名指针列表
		alias_ptr = ce->trait_aliases;
		// 第一个别名 （zend_trait_alias 实例）
		alias = *alias_ptr;
		// 循环变量
		i = 0;
		// 遍历每个别名
		while (alias) {
			// scope 不存在或 与 方法的域相同 并且 别名应用到此方法
			/* Scope unset or equal to the function we compare to, and the alias applies to fn */
			
			// 如果有别名 并且 
			if (alias->alias != NULL
				// 方法的所属域 与此别名相等 
				&& fn->common.scope == aliases[i]
				// 别名中的 引用方法名 和此方法名相等
				&& zend_string_equals_ci(alias->trait_method.method_name, fnname)
			) {
				fn_copy = *fn;
				// 如果是0，没有修饰符被修改过
				/* if it is 0, no modifiers have been changed */
				if (alias->modifiers) {
					// 把原来的修饰符复制过来，不要可见性限制。
					fn_copy.common.fn_flags = alias->modifiers | (fn->common.fn_flags & ~ZEND_ACC_PPP_MASK);
				}
				// 小写别名
				lcname = zend_string_tolower(alias->alias);
				// 添加trait中的方法， p1:类
				zend_add_trait_method(ce, alias->alias, lcname, &fn_copy);
				// 释放小写名
				zend_string_release_ex(lcname, 0);
			}
			// 下一个别名指针
			alias_ptr++;
			// 别名
			alias = *alias_ptr;
			// 数量+1
			i++;
		}
	}

	// 排除的直接跳过了
	// 如果没有排除列表 或 排除列表里没有此方法
	if (exclude_table == NULL || zend_hash_find(exclude_table, fnname) == NULL) {
		// 不在哈希表中，函数不会被排除
		/* is not in hashtable, thus, function is not to be excluded */
		
		// 复制原 方法
		memcpy(&fn_copy, fn, fn->type == ZEND_USER_FUNCTION ? sizeof(zend_op_array) : sizeof(zend_internal_function));
		
		// 应用没有别名名称的引用，只设置可见性
		/* apply aliases which have not alias name, just setting visibility */
		
		// 如果有别名列表
		if (ce->trait_aliases) {
			// 列表开头
			alias_ptr = ce->trait_aliases;
			// 第一个别名指针
			alias = *alias_ptr;
			// 循环变量
			i = 0;
			// 如果别名存在
			while (alias) {
				// 删除域 或 与此方法的域相等 并且别名应用于此方法
				/* Scope unset or equal to the function we compare to, and the alias applies to fn */
				// 别名有效 并且 无修饰符 并且 函数的域与此域相同
				if (alias->alias == NULL && alias->modifiers != 0
					&& fn->common.scope == aliases[i]
					&& zend_string_equals_ci(alias->trait_method.method_name, fnname)
				) {
					// 清空 PPP 属性
					fn_copy.common.fn_flags = alias->modifiers | (fn->common.fn_flags & ~ZEND_ACC_PPP_MASK);
				}
				// 下一个别名指针
				alias_ptr++;
				// 取出指针
				alias = *alias_ptr;
				// 数量 +1
				i++;
			}
		}
		// 添加trait中的方法， p1:类
		zend_add_trait_method(ce, fn->common.function_name, fnname, &fn_copy);
	}
	// 如果在排除列表中，这一段都跳过了
}
/* }}} */

// ing3, trait 应用检查：trait是否有效 并且 被添加到类里
static uint32_t zend_check_trait_usage(zend_class_entry *ce, zend_class_entry *trait, zend_class_entry **traits) /* {{{ */
{
	uint32_t i;
	// 如果不是trait
	if (UNEXPECTED((trait->ce_flags & ZEND_ACC_TRAIT) != ZEND_ACC_TRAIT)) {
		// 报错：此类不是 trait， 只有trait可以在 as 和 insteadof 中使用
		zend_error_noreturn(E_COMPILE_ERROR, "Class %s is not a trait, Only traits may be used in 'as' and 'insteadof' statements", ZSTR_VAL(trait->name));
		// 返回 否
		return 0;
	}

	// 遍历所有traits
	for (i = 0; i < ce->num_traits; i++) {
		// 如果已经有了
		if (traits[i] == trait) {
			// 返回编号
			return i;
		}
	}
	// 报错：trait 还没有添加进类里
	zend_error_noreturn(E_COMPILE_ERROR, "Required Trait %s wasn't added to %s", ZSTR_VAL(trait->name), ZSTR_VAL(ce->name));
	return 0;
}
/* }}} */


// 150行
// ing2, 在类中完成 trait 初始化（创建并返回别名列表）,p1:类，p2:trait列表，p3:返回排除列表，p4:返回别名指针
static void zend_traits_init_trait_structures(zend_class_entry *ce, zend_class_entry **traits, HashTable ***exclude_tables_ptr, zend_class_entry ***aliases_ptr) /* {{{ */
{
	size_t i, j = 0;
	zend_trait_precedence **precedences;
	zend_trait_precedence *cur_precedence;
	zend_trait_method_reference *cur_method_ref;
	zend_string *lc_trait_name;
	zend_string *lcname;
	HashTable **exclude_tables = NULL;
	zend_class_entry **aliases = NULL;
	zend_class_entry *trait;

	// 解决类引用
	/* resolve class references */
	// 这个是编译时加进来的，trait优先列表（一串 zend_trait_precedence 实例）
	if (ce->trait_precedences) {
		// 创建排除列表，哈希表的列表
		exclude_tables = ecalloc(ce->num_traits, sizeof(HashTable*));
		// 循环变量
		i = 0;
		// 取出trait优先列表
		precedences = ce->trait_precedences;
		// 先清空原指针
		ce->trait_precedences = NULL;
		// 遍历trait优先列表（zend_trait_precedence 实例）
		while ((cur_precedence = precedences[i])) {
			// 先解决所有操作中用到的类
			/** Resolve classes for all precedence operations. */
			// 优先的方法
			cur_method_ref = &cur_precedence->trait_method;
			// trait名 转小写
			lc_trait_name = zend_string_tolower(cur_method_ref->class_name);
			// 在执行时类表里查找
			trait = zend_hash_find_ptr(EG(class_table), lc_trait_name);
			// 释放小定名称
			zend_string_release_ex(lc_trait_name, 0);
			// 如果没找到 或 trait未完成链接 
			if (!trait || !(trait->ce_flags & ZEND_ACC_LINKED)) {
				// 报错：找不到trait
				zend_error_noreturn(E_COMPILE_ERROR, "Could not find trait %s", ZSTR_VAL(cur_method_ref->class_name));
			}
			// trait 应用检查：trait是否有效 并且 被添加到类里
			zend_check_trait_usage(ce, trait, traits);

			// 保证优先的方法确实有效
			/** Ensure that the preferred method is actually available. */
			// 方法名小写
			lcname = zend_string_tolower(cur_method_ref->method_name);
			// 如果方法不存在
			if (!zend_hash_exists(&trait->function_table, lcname)) {
				// 报错：定义在优先规则中的方法不存在
				zend_error_noreturn(E_COMPILE_ERROR,
						   "A precedence rule was defined for %s::%s but this method does not exist",
						   ZSTR_VAL(trait->name),
						   ZSTR_VAL(cur_method_ref->method_name));
			}

			// 对于其他trait，比较宽松
			// 不对它们报错。在定义中也可以更 defensive 
			// 但 需要保证， insteadof 定义是在类里
			/** With the other traits, we are more permissive.
				We do not give errors for those. This allows to be more
				defensive in such definitions.
				However, we want to make sure that the insteadof declaration
				is consistent in itself.
			 */

			// 遍历优先列表中的排除列表
			for (j = 0; j < cur_precedence->num_excludes; j++) {
				// 排除的 trait 类名
				zend_string* class_name = cur_precedence->exclude_class_names[j];
				zend_class_entry *exclude_ce;
				uint32_t trait_num;
				// 排除的 trait 名小写
				lc_trait_name = zend_string_tolower(class_name);
				// 在执行时类表中查找 
				exclude_ce = zend_hash_find_ptr(EG(class_table), lc_trait_name);
				// 释放小写类名
				zend_string_release_ex(lc_trait_name, 0);
				// 如果没有找到 trait 或没有链接
				if (!exclude_ce || !(exclude_ce->ce_flags & ZEND_ACC_LINKED)) {
					// 报错：找不到trait
					zend_error_noreturn(E_COMPILE_ERROR, "Could not find trait %s", ZSTR_VAL(class_name));
				}
				// trait 应用检查：trait是否有效 并且 被添加到类里
				trait_num = zend_check_trait_usage(ce, exclude_ce, traits);
				// 如果排除列表中 找不到此哈希表 
				if (!exclude_tables[trait_num]) {
					// 创建哈希表
					ALLOC_HASHTABLE(exclude_tables[trait_num]);
					// 初始化哈希表
					zend_hash_init(exclude_tables[trait_num], 0, NULL, NULL, 0);
				}
				// 排除列表中添加此 小写 trait 名
				if (zend_hash_add_empty_element(exclude_tables[trait_num], lcname) == NULL) {
					// 添加失败，报错：无法计算优先trait。它里面的方法被排除了多次。
					zend_error_noreturn(E_COMPILE_ERROR, "Failed to evaluate a trait precedence (%s). Method of trait %s was defined to be excluded multiple times", ZSTR_VAL(precedences[i]->trait_method.method_name), ZSTR_VAL(exclude_ce->name));
				}

				// 为了一致性，保证trait该应 不是在 排除 trait列表中 定义的。
				/* make sure that the trait method is not from a class mentioned in
				 exclude_from_classes, for consistency */
				if (trait == exclude_ce) {
					zend_error_noreturn(E_COMPILE_ERROR,
							   "Inconsistent insteadof definition. "
							   "The method %s is to be used from %s, but %s is also on the exclude list",
							   ZSTR_VAL(cur_method_ref->method_name),
							   ZSTR_VAL(trait->name),
							   ZSTR_VAL(trait->name));
				}
			}
			// 释放小写trait名
			zend_string_release_ex(lcname, 0);
			// 下一个
			i++;
		}
		// 处理后把优先列表放回去
		ce->trait_precedences = precedences;
	}

	// 如果 trait 别名指针列表
	if (ce->trait_aliases) {
		i = 0;
		// 给所有有效的别名（zend_trait_alias）指针计数
		while (ce->trait_aliases[i]) {
			i++;
		}
		// 别名对应的类列表
		aliases = ecalloc(i, sizeof(zend_class_entry*));
		
		// 重置循环变量
		i = 0;
		// 遍历属性别名（zend_trait_alias）
		while (ce->trait_aliases[i]) {
			// 取出一个别名
			zend_trait_alias *cur_alias = ce->trait_aliases[i];
			// 别名指向的 方法引用
			cur_method_ref = &ce->trait_aliases[i]->trait_method;
			// 方法名小写
			lcname = zend_string_tolower(cur_method_ref->method_name);
			// 如果有所属类
			if (cur_method_ref->class_name) {
				// 对于所有有显式 trait 名的别名，先解决它们的 trait
				/* For all aliases with an explicit class name, resolve the class now. */
				// 小写trait名
				lc_trait_name = zend_string_tolower(cur_method_ref->class_name);
				// 执行时类表中获取 trait
				trait = zend_hash_find_ptr(EG(class_table), lc_trait_name);
				// 释放小写类名
				zend_string_release_ex(lc_trait_name, 0);
				// 如果没找到trait 或者它还没有链接完成
				if (!trait || !(trait->ce_flags & ZEND_ACC_LINKED)) {
					// 报错：无法找到trait
					zend_error_noreturn(E_COMPILE_ERROR, "Could not find trait %s", ZSTR_VAL(cur_method_ref->class_name));
				}
				// trait 应用检查：trait是否有效 并且 被添加到类里
				zend_check_trait_usage(ce, trait, traits);
				// trait放到别名列表里
				aliases[i] = trait;

				// 保证引用的方法是有效的
				/* And, ensure that the referenced method is resolvable, too. */
				// 如果方法名不在这个 trait 里
				if (!zend_hash_exists(&trait->function_table, lcname)) {
					// 报错：定义的方法不在trait中
					zend_error_noreturn(E_COMPILE_ERROR, "An alias was defined for %s::%s but this method does not exist", ZSTR_VAL(trait->name), ZSTR_VAL(cur_method_ref->method_name));
				}
			// 如果方法引用没有所属类
			// （这种情况怎么会出现呢？这要看类的编译过程）
			} else {
				// 先找到对于此方法优先的 trait
				/* Find out which trait this method refers to. */
				// 
				trait = NULL;
				// 遍历所有trait
				for (j = 0; j < ce->num_traits; j++) {
					// 如果元素有效
					if (traits[j]) {
						// 如果trait里能找到此方法
						if (zend_hash_exists(&traits[j]->function_table, lcname)) {
							// 如果trait还没找到
							if (!trait) {
								// 使用此trait
								trait = traits[j];
								// 继续找
								continue;
							}

							// 报错：有多个trait里包含此方法
							zend_error_noreturn(E_COMPILE_ERROR,
								"An alias was defined for method %s(), which exists in both %s and %s. Use %s::%s or %s::%s to resolve the ambiguity",
								ZSTR_VAL(cur_method_ref->method_name),
								ZSTR_VAL(trait->name), ZSTR_VAL(traits[j]->name),
								ZSTR_VAL(trait->name), ZSTR_VAL(cur_method_ref->method_name),
								ZSTR_VAL(traits[j]->name), ZSTR_VAL(cur_method_ref->method_name));
						}
					}
				}

				// 非绝对方法引用 引用了一个不存在的方法
				/* Non-absolute method reference refers to method that does not exist. */
				// 如果没找到可用的trait（如何测试这种情况？）
				if (!trait) {
					// 如果此别名有指定方法名
					if (cur_alias->alias) {
						// 报错：指定的方法不存在
						zend_error_noreturn(E_COMPILE_ERROR,
							"An alias (%s) was defined for method %s(), but this method does not exist",
							ZSTR_VAL(cur_alias->alias),
							ZSTR_VAL(cur_alias->trait_method.method_name));
					} else {
						// 报错：方法的修饰符有变化 ，但此方法不存在
						zend_error_noreturn(E_COMPILE_ERROR,
							"The modifiers of the trait method %s() are changed, but this method does not exist. Error",
							ZSTR_VAL(cur_alias->trait_method.method_name));
					}
				}
				// trait可用，添加要别名列表里
				aliases[i] = trait;
			}
			// 释放小写类名
			zend_string_release_ex(lcname, 0);
			// 下一个别名
			i++;
		}
	}

	// 返回排除列表
	*exclude_tables_ptr = exclude_tables;
	// 返回新创建的别名列表
	*aliases_ptr = aliases;
}
/* }}} */

// ing3, 绑定trait方法 p1:类，p2:trait列表，p3:排除列表，p4:trait别名列表
static void zend_do_traits_method_binding(zend_class_entry *ce, zend_class_entry **traits, HashTable **exclude_tables, zend_class_entry **aliases) /* {{{ */
{
	uint32_t i;
	zend_string *key;
	zend_function *fn;
	// 如果有排除列表
	if (exclude_tables) {
		// 遍历所有trait
		for (i = 0; i < ce->num_traits; i++) {
			// 如果trait有效
			if (traits[i]) {
				// 复制函数，应用定义的别名，排除使用过的 trait 方法
				/* copies functions, applies defined aliasing, and excludes unused trait methods */
				// 遍历方法列表
				ZEND_HASH_MAP_FOREACH_STR_KEY_PTR(&traits[i]->function_table, key, fn) {
					// 复制trait里的方法 p1:方法名，p2:方法，p3:类，p4:排除列表，p5:trait别名列表
					zend_traits_copy_functions(key, fn, ce, exclude_tables[i], aliases);
				} ZEND_HASH_FOREACH_END();

				// 如果排除表中有此哈希表
				if (exclude_tables[i]) {
					// 销毁此哈希表
					zend_hash_destroy(exclude_tables[i]);
					// 释放此哈希表
					FREE_HASHTABLE(exclude_tables[i]);
					// 指针清空
					exclude_tables[i] = NULL;
				}
			}
		}
	// 没有排除列表
	} else {
		// 和上面业务逻辑一样，只肖了个销毁哈希表
		for (i = 0; i < ce->num_traits; i++) {
			// 如果 trait 有效
			if (traits[i]) {
				// 遍历 trait 方法列表
				ZEND_HASH_MAP_FOREACH_STR_KEY_PTR(&traits[i]->function_table, key, fn) {
					// 复制trait里的方法 p1:方法名，p2:方法，p3:类，p4:排除列表，p5:trait别名列表
					zend_traits_copy_functions(key, fn, ce, NULL, aliases);
				} ZEND_HASH_FOREACH_END();
			}
		}
	}

	// 遍历方法表
	ZEND_HASH_MAP_FOREACH_PTR(&ce->function_table, fn) {
		// 绑定 trait中的方法后，给类添加相应的标记
		zend_fixup_trait_method(fn, ce);
	} ZEND_HASH_FOREACH_END();
}
/* }}} */

// ing3, 寻找常量的首次定义位置（在trait或当前类中）
static zend_class_entry* find_first_constant_definition(zend_class_entry *ce, zend_class_entry **traits, size_t current_trait, zend_string *constant_name, zend_class_entry *colliding_ce) /* {{{ */
{
	// 当有冲突时，这个方法用于在消息中 显示存在的冲突定义的位置
	// 当trait常量填充进正在组装的类的常量表，就失去了常量与trait的映射关系
	// 所以需要 这样的处理过程会 来定位常量在 trait中的首次定义位置。
	/* This function is used to show the place of the existing conflicting
	 * definition in error messages when conflicts occur. Since trait constants
	 * are flattened into the constants table of the composing class, and thus
	 * we lose information about which constant was defined in which trait, a
	 * process like this is needed to find the location of the first definition
	 * of the constant from traits.
	 */
	size_t i;
	// 如果冲突类就是当前类
	if (colliding_ce == ce) {
		// 遍历前面处理过的trait
		for (i = 0; i < current_trait; i++) {
			// 如果trait有效 并且 trait的常量表中有此常量
			if (traits[i]
				&& zend_hash_exists(&traits[i]->constants_table, constant_name)) {
				// 返回此trait
				return traits[i];
			}
		}
	}
	// trait里没有这个常量，正在组装的类里会有它
	/* Traits don't have it, then the composing class (or trait) itself has it. */
	return colliding_ce;
}
/* }}} */

// ing3, trait 常量检查，名称没有被占用，或类型兼容。p1:类，p2:trait常量，p3:常量名，p4:trait表，p5:当前trait序号
static bool do_trait_constant_check(zend_class_entry *ce, zend_class_constant *trait_constant, zend_string *name, zend_class_entry **traits, size_t current_trait) /* {{{ */
{
	// 初始标记掩码：PPP + final
	uint32_t flags_mask = ZEND_ACC_PPP_MASK | ZEND_ACC_FINAL;
	// 在类常量列表中用名称查找常量
	zval *zv = zend_hash_find_known_hash(&ce->constants_table, name);
	// 如果没找到
	if (zv == NULL) {
		// 没有存在同名常量，可以添加
		/* No existing constant of the same name, so this one can be added */
		// 返回真
		return true;
	}
	// 已有常量
	zend_class_constant *existing_constant = Z_PTR_P(zv);
	// 如果trait常量和 已存在的常量 的标记不同 或 trait属性或常量的值不兼容
	if ((ZEND_CLASS_CONST_FLAGS(trait_constant) & flags_mask) != (ZEND_CLASS_CONST_FLAGS(existing_constant) & flags_mask) ||
		// 对值更新后，用===比较，值为对象的话必须指针相同
	    !check_trait_property_or_constant_value_compatibility(ce, &trait_constant->value, &existing_constant->value)) {
		// 有存在的同名常量，它与新常量冲突，要抛出一个致命错误
		/* There is an existing constant of the same name, and it conflicts with the new one, so let's throw a fatal error */
		// 编译错误：trait A 和 B 定义了同样名称的常量，并且不兼容。
		zend_error_noreturn(E_COMPILE_ERROR,
			"%s and %s define the same constant (%s) in the composition of %s. However, the definition differs and is considered incompatible. Class was composed",
			// 寻找常量的首次定义位置（在trait或当前类中）
			ZSTR_VAL(find_first_constant_definition(ce, traits, current_trait, name, existing_constant->ce)->name),
			ZSTR_VAL(trait_constant->ce->name),
			ZSTR_VAL(name),
			ZSTR_VAL(ce->name));
	}

	// 存在与新常量兼容的常量，所以可以添加
	/* There is an existing constant which is compatible with the new one, so no need to add it */
	return false;
}
/* }}} */

// ing3, 绑定trait常量，p1:当前类，p2:trait表
static void zend_do_traits_constant_binding(zend_class_entry *ce, zend_class_entry **traits) /* {{{ */
{
	size_t i;
	// 遍历所有trait
	for (i = 0; i < ce->num_traits; i++) {
		zend_string *constant_name;
		zend_class_constant *constant;
		// 如果trait无效
		if (!traits[i]) {
			// 跳过
			continue;
		}

		// 遍历常量表
		ZEND_HASH_MAP_FOREACH_STR_KEY_PTR(&traits[i]->constants_table, constant_name, constant) {
			// trait 常量检查，名称没有被占用，或类型兼容。p1:类，p2:trait常量，p3:常量名，p4:trait表，p5:当前trait序号
			if (do_trait_constant_check(ce, constant, constant_name, traits, i)) {
				// 通过检验
				
				// 创建新类常量
				zend_class_constant *ct = NULL;
				ct = zend_arena_alloc(&CG(arena),sizeof(zend_class_constant));
				// 复制原类型常量
				memcpy(ct, constant, sizeof(zend_class_constant));
				// 使用新的类常量
				constant = ct;

				// 如果是表达式常量
				if (Z_TYPE(constant->value) == IS_CONSTANT_AST) {
					// 当前类去掉标记：已更新类常量
					ce->ce_flags &= ~ZEND_ACC_CONSTANTS_UPDATED;
					// 类添加标记：包含表达式常量
					ce->ce_flags |= ZEND_ACC_HAS_AST_CONSTANTS;
				}

				// 不同于接口实现和类继承，trait常量的访问控制 是通过正在组成的类的域 完成的。
				// 所以在这里替换类。
				/* Unlike interface implementations and class inheritances,
				 * access control of the trait constants is done by the scope
				 * of the composing class. So let's replace the ce here.
				 */
				// 常量使用新类
				constant->ce = ce;
				// 常量值增加引用次数
				Z_TRY_ADDREF(constant->value);
				// 常量有文档注释 ？创建注释副本 ： 空
				constant->doc_comment = constant->doc_comment ? zend_string_copy(constant->doc_comment) : NULL;
				// 常量有修饰属性 并且 修饰属性不是不可替代的
				if (constant->attributes && (!(GC_FLAGS(constant->attributes) & IS_ARRAY_IMMUTABLE))) {
					// 修饰添属性添加引用次数
					GC_ADDREF(constant->attributes);
				}
				// 更新常量表（后面的会覆盖前面的）
				zend_hash_update_ptr(&ce->constants_table, constant_name, constant);
			}
		} ZEND_HASH_FOREACH_END();
	}
}
/* }}} */

// ing3, 报错用，冲突时，找到属性信息的第一个定义trait。p1:当前类，p2:traits列表，p3:当前trait编号，p4:属性名，p5:产生冲突的类
static zend_class_entry* find_first_property_definition(zend_class_entry *ce, zend_class_entry **traits, size_t current_trait, zend_string *prop_name, zend_class_entry *colliding_ce) /* {{{ */
{
	size_t i;
	// 如果产生冲突的类就是当前类
	if (colliding_ce == ce) {
		// 检查前面用到的trait
		for (i = 0; i < current_trait; i++) {
			// 如果trait有效 并且 属性信息表里有 此属性名
			if (traits[i]
			 && zend_hash_exists(&traits[i]->properties_info, prop_name)) {
				// 返回此trait
				return traits[i];
			}
		}
	}
	// 产生冲突的类不是当前类 或 没找到属性信息的定义，返回产生冲突的类
	return colliding_ce;
}
/* }}} */

// ing3, 绑定 trait（列表） 属性
static void zend_do_traits_property_binding(zend_class_entry *ce, zend_class_entry **traits) /* {{{ */
{
	size_t i;
	zend_property_info *property_info;
	zend_property_info *colliding_prop;
	zend_property_info *new_prop;
	zend_string* prop_name;
	zval* prop_value;
	zend_string *doc_comment;

	// 在下面的步骤中，属性添加进属性表，为此进行了非常严格的处理
	// - 检查兼容性，如果和任何类中的属性不兼容 -> 致命报错
	// - 如果兼容，也有严格的提示信息
	/* In the following steps the properties are inserted into the property table
	 * for that, a very strict approach is applied:
	 * - check for compatibility, if not compatible with any property in class -> fatal
	 * - if compatible, then strict notice
	 */
	// 遍历每个trait
	for (i = 0; i < ce->num_traits; i++) {
		// 跳过无效的
		if (!traits[i]) {
			continue;
		}
		// 遍历trait的属性信息表
		ZEND_HASH_MAP_FOREACH_STR_KEY_PTR(&traits[i]->properties_info, prop_name, property_info) {
			// 属性信息标记
			uint32_t flags = property_info->flags;
			// 检查是否与当前类冲突
			/* next: check for conflicts with current class */
			// 用名称在类中查找属性信息，如果找到
			if ((colliding_prop = zend_hash_find_ptr(&ce->properties_info, prop_name)) != NULL) {
				// 如果属性信息是 private 并且不属于当前类
				if ((colliding_prop->flags & ZEND_ACC_PRIVATE) && colliding_prop->ce != ce) {
					// 类中删除此属性信息
					zend_hash_del(&ce->properties_info, prop_name);
					// 添加 修改过 标记
					flags |= ZEND_ACC_CHANGED;
				// 属性不是private 或 属于当前类
				} else {
					// 没查检通过都算不兼容
					bool is_compatible = false;
					// ppp + static + readonly 标记
					uint32_t flags_mask = ZEND_ACC_PPP_MASK | ZEND_ACC_STATIC | ZEND_ACC_READONLY;

					// 类属性信息与trait属性信息 flag相同 
					if ((colliding_prop->flags & flags_mask) == (flags & flags_mask) &&
						// 并且 检查属性类型是否兼容（双向可继承）
						property_types_compatible(property_info, colliding_prop) == INHERITANCE_SUCCESS
					) {
						// flags 一致，属性可能兼容
						/* the flags are identical, thus, the properties may be compatible */
						zval *op1, *op2;
						// 如果trait属性有 static标记
						if (flags & ZEND_ACC_STATIC) {
							// 取出两个静态成员
							op1 = &ce->default_static_members_table[colliding_prop->offset];
							op2 = &traits[i]->default_static_members_table[property_info->offset];
							// 两个静态成员解引用
							ZVAL_DEINDIRECT(op1);
							ZVAL_DEINDIRECT(op2);
						// 如果trait属性没有 static标记
						} else {
							// 取出两个普通成员
							op1 = &ce->default_properties_table[OBJ_PROP_TO_NUM(colliding_prop->offset)];
							op2 = &traits[i]->default_properties_table[OBJ_PROP_TO_NUM(property_info->offset)];
						}
						// 对值更新后，用===比较，值为对象的话必须指针相同
						is_compatible = check_trait_property_or_constant_value_compatibility(ce, op1, op2);
					}
					// 如果不兼容
					if (!is_compatible) {
						// 报错：编译错误：
						zend_error_noreturn(E_COMPILE_ERROR,
								"%s and %s define the same property ($%s) in the composition of %s. However, the definition differs and is considered incompatible. Class was composed",
								// 报错用，冲突时，找到属性信息的第一个定义trait。p1:当前类，p2:traits列表，p3:当前trait编号，p4:属性名，p5:产生冲突的类
								ZSTR_VAL(find_first_property_definition(ce, traits, i, prop_name, colliding_prop->ce)->name),
								ZSTR_VAL(property_info->ce->name),
								ZSTR_VAL(prop_name),
								ZSTR_VAL(ce->name));
					}
					continue;
				}
			}

			// 如果有只读标记 并且 属性没有只读标记
			if ((ce->ce_flags & ZEND_ACC_READONLY_CLASS) && !(property_info->flags & ZEND_ACC_READONLY)) {
				// 只读类不可引用 包含 非只读属性的 trait
				zend_error_noreturn(E_COMPILE_ERROR,
					"Readonly class %s cannot use trait with a non-readonly property %s::$%s",
					ZSTR_VAL(ce->name),
					ZSTR_VAL(property_info->ce->name),
					ZSTR_VAL(prop_name)
				);
			}

			// 属性未找到，进行添加
			/* property not found, so lets add it */
			// 如果有静态标记
			if (flags & ZEND_ACC_STATIC) {
				// 从静态成员列表中获取
				prop_value = &traits[i]->default_static_members_table[property_info->offset];
				// 值不可以是间接引用
				ZEND_ASSERT(Z_TYPE_P(prop_value) != IS_INDIRECT);
			// 没有静态标记
			} else {
				// 从默认成员列表中获取
				prop_value = &traits[i]->default_properties_table[OBJ_PROP_TO_NUM(property_info->offset)];
			}
			
			// 增加引用次数
			Z_TRY_ADDREF_P(prop_value);
			// 属性信息有文档注释，创建注释副本
			doc_comment = property_info->doc_comment ? zend_string_copy(property_info->doc_comment) : NULL;

			// 属性信息中定义的类型
			zend_type type = property_info->type;
			// 复制类型，如果是列表才复制，不是列表只增加引用次数。列表的旧 zend_type_list 留给谁了呢？
			zend_type_copy_ctor(&type, /* persistent */ 0);
			// 声名 类（不是对象）的静态和动态属性。返回属性信息。传入：类入口、属性名、属性值、权限、注释、类型（zend_type）
			new_prop = zend_declare_typed_property(ce, prop_name, prop_value, flags, doc_comment, type);

			// 如果有修饰属性
			if (property_info->attributes) {
				// 复制修饰属性
				new_prop->attributes = property_info->attributes;
				// 如果新属性不是不可更改
				if (!(GC_FLAGS(new_prop->attributes) & IS_ARRAY_IMMUTABLE)) {
					// 增加引用次数
					GC_ADDREF(new_prop->attributes);
				}
			}
		} ZEND_HASH_FOREACH_END();
	}
}
/* }}} */

// ing3, 绑定trait，p1:类，p2:trait列表
static void zend_do_bind_traits(zend_class_entry *ce, zend_class_entry **traits) /* {{{ */
{
	// 排除列表
	HashTable **exclude_tables;
	// 别名
	zend_class_entry **aliases;
	// trait 数 > 0
	ZEND_ASSERT(ce->num_traits > 0);

	// 在类中完成 trait 初始化
	/* complete initialization of trait structures in ce */
	zend_traits_init_trait_structures(ce, traits, &exclude_tables, &aliases);

	// 先把所有方法复制到类里
	/* first care about all methods to be flattened into the class */
	
	// 绑定trait方法 p1:类，p2:trait列表，p3:排除列表，p4:trait别名列表
	zend_do_traits_method_binding(ce, traits, exclude_tables, aliases);

	// 如果有别名列表
	if (aliases) {
		// 释放别名列表
		efree(aliases);
	}

	// 如果有排除列表
	if (exclude_tables) {
		// 释放排除列表
		efree(exclude_tables);
	}

	// 然后把trait中的 常量和 属性复制过来，当有问题时，提示开发者。
	/* then flatten the constants and properties into it, to, mostly to notify developer about problems */
	
	// 绑定trait常量，p1:当前类，p2:trait表
	zend_do_traits_constant_binding(ce, traits);
	// 绑定 trait（列表） 属性
	zend_do_traits_property_binding(ce, traits);
}
/* }}} */

#define MAX_ABSTRACT_INFO_CNT 3
// 抽象信息的打印格式
#define MAX_ABSTRACT_INFO_FMT "%s%s%s%s"
// clear, 辅助显示记录的类和方法名 ,只有 zend_verify_abstract_class 调用，
#define DISPLAY_ABSTRACT_FN(idx) \
	/* 有记录的话，显示类名（最多记录 MAX_ABSTRACT_INFO_CNT 个） */ \
	ai.afn[idx] ? ZEND_FN_SCOPE_NAME(ai.afn[idx]) : "", \
	/* 有记录的话，显示 :: */ \
	ai.afn[idx] ? "::" : "", \
	/* 有记录的话，显示 方法名 */ \
	ai.afn[idx] ? ZSTR_VAL(ai.afn[idx]->common.function_name) : "", \
	/* 下一个有效，结尾加“，”。 下一个无效，但还有记录数 结尾加 ... */ \
	ai.afn[idx] && ai.afn[idx + 1] ? ", " : (ai.afn[idx] && ai.cnt > MAX_ABSTRACT_INFO_CNT ? ", ..." : "")

// 抽象信息
typedef struct _zend_abstract_info {
	// 里面有4个方法指针
	zend_function *afn[MAX_ABSTRACT_INFO_CNT + 1];
	// 
	int cnt;
} zend_abstract_info;

// ing3, 检验抽象类方法
static void zend_verify_abstract_class_function(zend_function *fn, zend_abstract_info *ai) /* {{{ */
{
	// < 3，如果抽象信息还没满
	if (ai->cnt < MAX_ABSTRACT_INFO_CNT) {
		// 把新的方法放进去
		ai->afn[ai->cnt] = fn;
	}
	// 方法数量+1
	ai->cnt++;
}
/* }}} */

// ing3, 检验是否需要标记成抽象类，或实现某些抽象方法
void zend_verify_abstract_class(zend_class_entry *ce) /* {{{ */
{
	zend_function *func;
	zend_abstract_info ai;
	// 是否是显式抽象类
	bool is_explicit_abstract = (ce->ce_flags & ZEND_ACC_EXPLICIT_ABSTRACT_CLASS) != 0;
	// 不是枚举，就可以是抽象类
	bool can_be_abstract = (ce->ce_flags & ZEND_ACC_ENUM) == 0;
	// 清空临时抽象信息
	memset(&ai, 0, sizeof(ai));
	// 遍历方法表
	ZEND_HASH_MAP_FOREACH_PTR(&ce->function_table, func) {
		// 如果方法是抽象的
		if (func->common.fn_flags & ZEND_ACC_ABSTRACT) {
			// 如果类是显式抽象，只检验私有抽象方法，因为它们必须被定义在同一个类中。
			/* If the class is explicitly abstract, we only check private abstract methods,
			 * because only they must be declared in the same class. */
			 // 如果不是显式抽象 或 是private
			if (!is_explicit_abstract || (func->common.fn_flags & ZEND_ACC_PRIVATE)) {
				// 检验抽象类方法
				zend_verify_abstract_class_function(func, &ai);
			}
		}
	} ZEND_HASH_FOREACH_END();
	
	// 如果有抽象类方法
	if (ai.cnt) {
		// 报错：（不是显式抽象 并且 可以抽象）？
		zend_error_noreturn(E_ERROR, !is_explicit_abstract && can_be_abstract
			// （trait、接口、枚举、类）中含有 n 个 抽象方法， 所以它必须被定义成抽象的 或者 实现以下方法（前3个名称）
			? "%s %s contains %d abstract method%s and must therefore be declared abstract or implement the remaining methods (" MAX_ABSTRACT_INFO_FMT MAX_ABSTRACT_INFO_FMT MAX_ABSTRACT_INFO_FMT ")"
			// （trait、接口、枚举、类）必须实现 n 个抽象私有方法（前3个名称）
			: "%s %s must implement %d abstract private method%s (" MAX_ABSTRACT_INFO_FMT MAX_ABSTRACT_INFO_FMT MAX_ABSTRACT_INFO_FMT ")",
			// 返回 首字母大写 对象类型（trait、接口、枚举、类）
			zend_get_object_type_uc(ce),
			ZSTR_VAL(ce->name), ai.cnt,
			ai.cnt > 1 ? "s" : "",
			// 辅助显示记录的类和方法名
			DISPLAY_ABSTRACT_FN(0),
			DISPLAY_ABSTRACT_FN(1),
			DISPLAY_ABSTRACT_FN(2)
			);
	// 没有抽象类方法
	} else {
		// 现在一切就绪，隐式抽象类标记 可以删掉了
		/* now everything should be fine and an added ZEND_ACC_IMPLICIT_ABSTRACT_CLASS should be removed */
		// 删除 隐式抽象类标记
		ce->ce_flags &= ~ZEND_ACC_IMPLICIT_ABSTRACT_CLASS;
	}
}
/* }}} */

// 变异责任
typedef struct {
	enum {
		// 从属
		OBLIGATION_DEPENDENCY,
		// 兼容性
		OBLIGATION_COMPATIBILITY,
		// 属性兼容
		OBLIGATION_PROPERTY_COMPATIBILITY
	} type;
	// 
	union {
		// 从属类
		zend_class_entry *dependency_ce;
		// 
		struct {
			// 在继承检查过程中，trait 可能使用 临时的 堆栈函数 ，所以使用使用函数的副本比较好。
			/* Traits may use temporary on-stack functions during inheritance checks,
			 * so use copies of functions here as well. */
			// 父函数
			zend_function parent_fn;
			// 子函数
			zend_function child_fn;
			// 子域
			zend_class_entry *child_scope;
			// 父域
			zend_class_entry *parent_scope;
		};
		//
		struct {
			// 父类属性信息
			const zend_property_info *parent_prop;
			// 子类属性信息
			const zend_property_info *child_prop;
		};
	};
} variance_obligation;

// ing3, 变异责任 销毁器
static void variance_obligation_dtor(zval *zv) {
	efree(Z_PTR_P(zv));
}

// ing3, 变异责任哈希表 销毁器
static void variance_obligation_ht_dtor(zval *zv) {
	// 销毁并释放常量中的哈希表
	zend_hash_destroy(Z_PTR_P(zv));
	FREE_HASHTABLE(Z_PTR_P(zv));
}

// ing3, 为类获取或初始化 变异责任 哈希表
static HashTable *get_or_init_obligations_for_class(zend_class_entry *ce) {
	HashTable *ht;
	zend_ulong key;
	// 如果 编译时 没有 延时变异责任
	if (!CG(delayed_variance_obligations)) {
		// 创建 延时变异责任 哈希表 
		ALLOC_HASHTABLE(CG(delayed_variance_obligations));
		// 初始化 延时变异责任 哈希表
		zend_hash_init(CG(delayed_variance_obligations), 0, NULL, variance_obligation_ht_dtor, 0);
	}
	// 把类入口指针转成整数 
	key = (zend_ulong) (uintptr_t) ce;
	// 在 延时变异责任 哈希表中查找 此类的 哈希表
	ht = zend_hash_index_find_ptr(CG(delayed_variance_obligations), key);
	// 如果找到，直接返回
	if (ht) {
		return ht;
	}
	// 如果没找到，创建新哈希表
	ALLOC_HASHTABLE(ht);
	// 初始化新哈希表
	zend_hash_init(ht, 0, NULL, variance_obligation_dtor, 0);
	// 放到 延时变异责任 哈希表中
	zend_hash_index_add_new_ptr(CG(delayed_variance_obligations), key, ht);
	// 为类添加 【未完成变异】 标记
	ce->ce_flags |= ZEND_ACC_UNRESOLVED_VARIANCE;
	return ht;
}

// ing3, 添加从属责任。（从属责任对象关关联到从属类）
static void add_dependency_obligation(zend_class_entry *ce, zend_class_entry *dependency_ce) {
	// 为类获取或初始化 变异责任 哈希表
	HashTable *obligations = get_or_init_obligations_for_class(ce);
	// 创建变异责任对象
	variance_obligation *obligation = emalloc(sizeof(variance_obligation));
	// 类型为 从属
	obligation->type = OBLIGATION_DEPENDENCY;
	// 关联从属类
	obligation->dependency_ce = dependency_ce;
	// 添加到此类的 责任列表中
	zend_hash_next_index_insert_ptr(obligations, obligation);
}

// ing3, 添加兼容性责任
static void add_compatibility_obligation(
		zend_class_entry *ce,
		const zend_function *child_fn, zend_class_entry *child_scope,
		const zend_function *parent_fn, zend_class_entry *parent_scope) {
	// 为类获取或初始化 变异责任 哈希表
	HashTable *obligations = get_or_init_obligations_for_class(ce);
	// 创建变异责任
	variance_obligation *obligation = emalloc(sizeof(variance_obligation));
	// 类型为兼容性
	obligation->type = OBLIGATION_COMPATIBILITY;
	// 复制方法，因为它们在traits中会 分配在堆栈里
	/* Copy functions, because they may be stack-allocated in the case of traits. */
	
	// 如果子方法是内置方法
	if (child_fn->common.type == ZEND_INTERNAL_FUNCTION) {
		// 复制【内置方法】给 变异责任对象
		memcpy(&obligation->child_fn, child_fn, sizeof(zend_internal_function));
	// 不是内置方法
	} else {
		// 复制【操作码】给 变异责任对象（zend_op_array 与 zend_function 部分兼容）
		memcpy(&obligation->child_fn, child_fn, sizeof(zend_op_array));
	}
	// 如果父方法是内置方法
	if (parent_fn->common.type == ZEND_INTERNAL_FUNCTION) {
		// 复制【内置方法】给 变异责任对象
		memcpy(&obligation->parent_fn, parent_fn, sizeof(zend_internal_function));
	// 不是内置方法
	} else {
		// 复制【操作码】给 变异责任对象（zend_op_array 与 zend_function 部分兼容）
		memcpy(&obligation->parent_fn, parent_fn, sizeof(zend_op_array));
	}
	// 添加子类域
	obligation->child_scope = child_scope;
	// 添加父类域
	obligation->parent_scope = parent_scope;
	// 新建的责任，添加进责任哈希表里
	zend_hash_next_index_insert_ptr(obligations, obligation);
}

// ing3, 添加 属性性信息兼容性责任。p1:所属类，p2:子属性信息，p3:父属性信息
static void add_property_compatibility_obligation(
		zend_class_entry *ce, const zend_property_info *child_prop,
		const zend_property_info *parent_prop) {
	// 为类获取或初始化 变异责任 哈希表
	HashTable *obligations = get_or_init_obligations_for_class(ce);
	// 创建新的 变异责任
	variance_obligation *obligation = emalloc(sizeof(variance_obligation));
	// 类型为属性兼容性
	obligation->type = OBLIGATION_PROPERTY_COMPATIBILITY;
	// 关联子属性信息
	obligation->child_prop = child_prop;
	// 关联父属性信息
	obligation->parent_prop = parent_prop;
	// 把变异责任 添加到责任哈希表中
	zend_hash_next_index_insert_ptr(obligations, obligation);
}

static void resolve_delayed_variance_obligations(zend_class_entry *ce);

// ing3, 检查变异责任
static void check_variance_obligation(variance_obligation *obligation) {
	// 如果责任类型是 从属
	if (obligation->type == OBLIGATION_DEPENDENCY) {
		// 从属类
		zend_class_entry *dependency_ce = obligation->dependency_ce;
		// 如果类有 未完成变异 标记
		if (dependency_ce->ce_flags & ZEND_ACC_UNRESOLVED_VARIANCE) {
			// 原本正在链接的类
			zend_class_entry *orig_linking_class = CG(current_linking_class);
			// 如果从属类有 ZEND_ACC_CACHEABLE， 开始链接它
			CG(current_linking_class) =
				(dependency_ce->ce_flags & ZEND_ACC_CACHEABLE) ? dependency_ce : NULL;
			// 解决延时变异责任（主要是检查责任）
			resolve_delayed_variance_obligations(dependency_ce);
			// 换回原本正在链接的类
			CG(current_linking_class) = orig_linking_class;
		}
	// 责任类型是 兼容
	} else if (obligation->type == OBLIGATION_COMPATIBILITY) {
		// 方法实现检查，p1:方法，p2:作用域，p3:原型，p4:原型作用域
		inheritance_status status = zend_do_perform_implementation_check(
			&obligation->child_fn, obligation->child_scope,
			&obligation->parent_fn, obligation->parent_scope);
		// 如果检查不成功
		if (UNEXPECTED(status != INHERITANCE_SUCCESS)) {
			// 报错：方法不兼容
			emit_incompatible_method_error(
				&obligation->child_fn, obligation->child_scope,
				&obligation->parent_fn, obligation->parent_scope, status);
		}
		// 兼容性检查不成功时，要抛错
		/* Either the compatibility check was successful or only threw a warning. */
	// 其他情况
	} else {
		// 责任类型必须是 属性兼容
		ZEND_ASSERT(obligation->type == OBLIGATION_PROPERTY_COMPATIBILITY);
		// 检查属性类型是否兼容（双向可继承）
		inheritance_status status =
			property_types_compatible(obligation->parent_prop, obligation->child_prop);
			
		// 如果状态不是继承成功
		if (status != INHERITANCE_SUCCESS) {
			// 报错：不兼容的属性
			emit_incompatible_property_error(obligation->child_prop, obligation->parent_prop);
		}
	}
}

// ing3, 加载延时类：p1:正在继承的类，只有报错会用到
static void load_delayed_classes(zend_class_entry *ce) {
	// 延时自动加载列表
	HashTable *delayed_autoloads = CG(delayed_autoloads);
	// 如果列表无效，直接返回
	if (!delayed_autoloads) {
		return;
	}

	// 自动加载可以触发 其他类的链接操作，这个操作会注册新的延时载入
	// 因此，这里用到循环，弹出并加载表里的第一个元素。如果它触发了链接操作，其余的类会在新类链接时被载入。
	// 这个很重要，否则如果新类在继承中，是此类的子类。 必要的依赖可能不可用。
	/* Autoloading can trigger linking of another class, which may register new delayed autoloads.
	 * For that reason, this code uses a loop that pops and loads the first element of the HT. If
	 * this triggers linking, then the remaining classes may get loaded when linking the newly
	 * loaded class. This is important, as otherwise necessary dependencies may not be available
	 * if the new class is lower in the hierarchy than the current one. */
	// 表位置
	HashPosition pos = 0;
	// 返回字串key
	zend_string *name;
	// 返回索引号
	zend_ulong idx;
	// 如果当前key有效 （不是 HASH_KEY_NON_EXISTENT ）
	while (zend_hash_get_current_key_ex(delayed_autoloads, &name, &idx, &pos)
			!= HASH_KEY_NON_EXISTENT) {
		// 类名称增加引用次数
		zend_string_addref(name);
		// 通过类名称删除此元素
		zend_hash_del(delayed_autoloads, name);
		// 查找并加载此类
		zend_lookup_class(name);
		// 释放名称
		zend_string_release(name);
		// 如果有异常
		if (EG(exception)) {
			// 抛出异常：在继承此类是，自动加载 类 **
			zend_exception_uncaught_error(
				"During inheritance of %s, while autoloading %s",
				ZSTR_VAL(ce->name), ZSTR_VAL(name));
		}
	}
}

// ing3, 解决延时变异责任（主要是检查责任）
static void resolve_delayed_variance_obligations(zend_class_entry *ce) {
	// 取得编译时 中的 延时变异责任（哈希表）
	HashTable *all_obligations = CG(delayed_variance_obligations), *obligations;
	// 类指针转成整数 ，作为key
	zend_ulong num_key = (zend_ulong) (uintptr_t) ce;
	// 哈希表必须有效
	ZEND_ASSERT(all_obligations != NULL);
	// 根据 key 取出 此类的责任哈希表（又是一个哈希表）
	obligations = zend_hash_index_find_ptr(all_obligations, num_key);
	// 责任哈希表必须存在
	ZEND_ASSERT(obligations != NULL);

	variance_obligation *obligation;
	// 遍历责任哈希表
	ZEND_HASH_FOREACH_PTR(obligations, obligation) {
		// 检查变异责任
		check_variance_obligation(obligation);
	} ZEND_HASH_FOREACH_END();
	// 类删除标记【未完成变异】
	ce->ce_flags &= ~ZEND_ACC_UNRESOLVED_VARIANCE;
	// 类添加标记，已链接
	ce->ce_flags |= ZEND_ACC_LINKED;
	// 责任表中删除此类
	zend_hash_index_del(all_obligations, num_key);
}

// ing3, 检查 不可恢复的加载失败
static void check_unrecoverable_load_failure(zend_class_entry *ce) {
	// 如果此类在通过 变异责任 解除链接时 使用到，把它从列表里删除并抛错是非法的。
	// 因为已经有依赖 此指定类的 继承等级。
	// 所以如果不允许在上术情况中 抛错的话，要 退回到致命错误。
	/* If this class has been used while unlinked through a variance obligation, it is not legal
	 * to remove the class from the class table and throw an exception, because there is already
	 * a dependence on the inheritance hierarchy of this specific class. Instead we fall back to
	 * a fatal error, as would happen if we did not allow exceptions in the first place. */
	// 如果有 未链接的 uses
	if (CG(unlinked_uses)
			// 从未链接的 uses 中 删除此类
			&& zend_hash_index_del(CG(unlinked_uses), (zend_long)(zend_uintptr_t)ce) == SUCCESS) {
		// 抛错：在有变异从属的继承中
		zend_exception_uncaught_error(
			"During inheritance of %s with variance dependencies", ZSTR_VAL(ce->name));
	}
}

// ing3, 如果上面处理的是此方法，把处理器的操作码更新成（上文中）新建的副本
#define zend_update_inherited_handler(handler) do { \
		/* 如果操作码是此处理器 */ \
		if (ce->handler == (zend_function*)op_array) { \
			/* 使用新操作码 */ \
			ce->handler = (zend_function*)new_op_array; \
		} \
	} while (0)

// 120行
// ing3, 懒加载 类
static zend_class_entry *zend_lazy_class_load(zend_class_entry *pce)
{
	zend_class_entry *ce;
	Bucket *p, *end;
	// 分配内存创建 类入口
	ce = zend_arena_alloc(&CG(arena), sizeof(zend_class_entry));
	// 复制原类入口
	memcpy(ce, pce, sizeof(zend_class_entry));
	// 不可更改
	ce->ce_flags &= ~ZEND_ACC_IMMUTABLE;
	// 引用次数
	ce->refcount = 1;
	// 继承缓存 
	ce->inheritance_cache = NULL;
	// 如果允许预加载
	if (CG(compiler_options) & ZEND_COMPILE_PRELOAD) {
		// 创建 mutable_data 指针列表
		ZEND_MAP_PTR_NEW(ce->mutable_data);
	// 否则 
	} else {
		// 初始化 mutable_data 指针
		ZEND_MAP_PTR_INIT(ce->mutable_data, NULL);
	}

	// 复制 属性表
	/* properties */
	if (ce->default_properties_table) {
		// 创建属性表
		zval *dst = emalloc(sizeof(zval) * ce->default_properties_count);
		// 原属性表
		zval *src = ce->default_properties_table;
		// 原表结尾
		zval *end = src + ce->default_properties_count;
		// 使用新表
		ce->default_properties_table = dst;
		// 遍历原表
		for (; src != end; src++, dst++) {
			// 把元素 复制到新表里
			ZVAL_COPY_VALUE_PROP(dst, src);
		}
	}

	// 复制方法表
	/* methods */
	ce->function_table.pDestructor = ZEND_FUNCTION_DTOR;
	// 如果方法表没有初始化过
	if (!(HT_FLAGS(&ce->function_table) & HASH_FLAG_UNINITIALIZED)) {
		// 分配内存创建方法表
		p = emalloc(HT_SIZE(&ce->function_table));
		// 把原方法表的数据复制过来
		memcpy(p, HT_GET_DATA_ADDR(&ce->function_table), HT_USED_SIZE(&ce->function_table));
		// 使用新的方法表数据
		HT_SET_DATA_ADDR(&ce->function_table, p);
		// 指针到 数据段开始位置
		p = ce->function_table.arData;
		// 元素结束位置
		end = p + ce->function_table.nNumUsed;
		// 遍历每个方法
		for (; p != end; p++) {
			// 
			zend_op_array *op_array, *new_op_array;
			// 取出元素中的操作码列表指针
			op_array = Z_PTR(p->val);
			// 必须是用户定义函数 
			ZEND_ASSERT(op_array->type == ZEND_USER_FUNCTION);
			// 操作码必须属于此类
			ZEND_ASSERT(op_array->scope == pce);
			// 原型为空
			ZEND_ASSERT(op_array->prototype == NULL);
			// 分配内存创建新的操作码列表
			new_op_array = zend_arena_alloc(&CG(arena), sizeof(zend_op_array));
			// 使用新的操作码列表
			Z_PTR(p->val) = new_op_array;
			// 把操作码复制过来
			memcpy(new_op_array, op_array, sizeof(zend_op_array));
			// 删除不可更改标记
			new_op_array->fn_flags &= ~ZEND_ACC_IMMUTABLE;
			// 所属域为当前类
			new_op_array->scope = ce;
			// 初始化运行时缓存 指针表
			ZEND_MAP_PTR_INIT(new_op_array->run_time_cache, NULL);
			// 初始化 静态变量 指针表
			ZEND_MAP_PTR_INIT(new_op_array->static_variables_ptr, NULL);

			// 如果上面处理的是此方法，把处理器的操作码更新成（上文中）新建的副本
			zend_update_inherited_handler(constructor);
			zend_update_inherited_handler(destructor);
			zend_update_inherited_handler(clone);
			zend_update_inherited_handler(__get);
			zend_update_inherited_handler(__set);
			zend_update_inherited_handler(__call);
			zend_update_inherited_handler(__isset);
			zend_update_inherited_handler(__unset);
			zend_update_inherited_handler(__tostring);
			zend_update_inherited_handler(__callstatic);
			zend_update_inherited_handler(__debugInfo);
			zend_update_inherited_handler(__serialize);
			zend_update_inherited_handler(__unserialize);
		}
	}

	// 复制静态成员
	/* static members */
	if (ce->default_static_members_table) {
		// 分配内存创建新静态成员表
		zval *dst = emalloc(sizeof(zval) * ce->default_static_members_count);
		// 原静态成员表
		zval *src = ce->default_static_members_table;
		// 原静态成员表结尾
		zval *end = src + ce->default_static_members_count;
		// 使用新静态成员表
		ce->default_static_members_table = dst;
		// 遍历每个成员
		for (; src != end; src++, dst++) {
			// 复制到新成员表里
			ZVAL_COPY_VALUE(dst, src);
		}
	}
	// 初始化静态成员指针表
	ZEND_MAP_PTR_INIT(ce->static_members_table, NULL);

	// 复制属性信息
	/* properties_info */
	// 如果属性信息表 不是 未初始化
	if (!(HT_FLAGS(&ce->properties_info) & HASH_FLAG_UNINITIALIZED)) {
		// 创建新属性信息表（顺序哈希表）
		p = emalloc(HT_SIZE(&ce->properties_info));
		// 复制原表数据
		// 找到哈希表数据块开头位置。数据块在 arData 的前面，所以通过 arData可以找到数据块开头
		memcpy(p, HT_GET_DATA_ADDR(&ce->properties_info), HT_USED_SIZE(&ce->properties_info));
		// 设置Bucket 开头位置。 通过哈希表数据块开头位置，找到 Bucket 列表开头。
		HT_SET_DATA_ADDR(&ce->properties_info, p);
		// 属性信息 顺序数组数据 部分
		p = ce->properties_info.arData;
		// 属性信息 顺序数组数据 部分 结尾
		end = p + ce->properties_info.nNumUsed;
		// 遍历 所有属性信息
		for (; p != end; p++) {
			zend_property_info *prop_info, *new_prop_info;
			// 取出属性信息
			prop_info = Z_PTR(p->val);
			// 属性信息必须属于父类
			ZEND_ASSERT(prop_info->ce == pce);
			// 分配内存创建新的属性信息
			new_prop_info= zend_arena_alloc(&CG(arena), sizeof(zend_property_info));
			// 使用新创建的属性信息
			Z_PTR(p->val) = new_prop_info;
			// 把旧属性信息的内容复制给新 实例
			memcpy(new_prop_info, prop_info, sizeof(zend_property_info));
			// 所属类是新类
			new_prop_info->ce = ce;
			// 如果新属性信息有类型列表
			if (ZEND_TYPE_HAS_LIST(new_prop_info->type)) {
				zend_type_list *new_list;
				// 取出列表类型
				zend_type_list *list = ZEND_TYPE_LIST(new_prop_info->type);
				// 创建新类型列表
				new_list = zend_arena_alloc(&CG(arena), ZEND_TYPE_LIST_SIZE(list->num_types));
				// 把旧类型列表的数据复制过来
				memcpy(new_list, list, ZEND_TYPE_LIST_SIZE(list->num_types));
				// 新属性信息的类型列表，使用新建的列表
				// new_prop_info->type__prt = list
				ZEND_TYPE_SET_PTR(new_prop_info->type, list);
				// ZEND_TYPE_FULL_MASK ：返回zend_type 的类型码
				// 类型码中添加 _ZEND_TYPE_ARENA_BIT 从ARENA分配
				ZEND_TYPE_FULL_MASK(new_prop_info->type) |= _ZEND_TYPE_ARENA_BIT;
			}
		}
	}

	// 复制常量表
	/* constants table */
	if (!(HT_FLAGS(&ce->constants_table) & HASH_FLAG_UNINITIALIZED)) {
		// 创建新常量表
		p = emalloc(HT_SIZE(&ce->constants_table));
		// 复制原表数据
		// 找到哈希表数据块开头位置。数据块在 arData 的前面，所以通过 arData可以找到数据块开头
		memcpy(p, HT_GET_DATA_ADDR(&ce->constants_table), HT_USED_SIZE(&ce->constants_table));
		// 使用新常量表
		// 设置Bucket 开头位置。 通过哈希表数据块开头位置，找到 Bucket 列表开头。
		HT_SET_DATA_ADDR(&ce->constants_table, p);
		// 数据区指针
		p = ce->constants_table.arData;
		// 数据区结尾指针
		end = p + ce->constants_table.nNumUsed;
		// 遍历 
		for (; p != end; p++) {
			zend_class_constant *c, *new_c;
			// 取得元素值下面的 类常量指针
			c = Z_PTR(p->val);
			// 常量所属类必须为父类
			ZEND_ASSERT(c->ce == pce);
			// 创建类常量
			new_c = zend_arena_alloc(&CG(arena), sizeof(zend_class_constant));
			// 使用新类常量
			Z_PTR(p->val) = new_c;
			// 把原类常量的值复制过来
			memcpy(new_c, c, sizeof(zend_class_constant));
			// 新常量指向新类
			new_c->ce = ce;
		}
	}

	// 返回新类
	return ce;
}

// 32位
#ifndef ZEND_WIN32
# define UPDATE_IS_CACHEABLE(ce) do { \
			if ((ce)->type == ZEND_USER_CLASS) { \
				is_cacheable &= (ce)->ce_flags; \
			} \
		} while (0)
// 6其他
#else
// TODO: ASLR may cause different addresses in different workers ???
// ing3, is_cacheable &= (ce)->ce_flags;
# define UPDATE_IS_CACHEABLE(ce) do { \
			is_cacheable &= (ce)->ce_flags; \
		} while (0)
#endif

// 230行
// ing2, 链接类
ZEND_API zend_class_entry *zend_do_link_class(zend_class_entry *ce, zend_string *lc_parent_name, zend_string *key) /* {{{ */
{
	// 先加载 父类和接口 依赖项，这样可以优雅地中断连接并抛出异常 再从类表中移除类。
	// 只有 在加载过程中 没有 在当前类上没有 变异责任 被添加， 才会出现这种情况。
	/* Load parent/interface dependencies first, so we can still gracefully abort linking
	 * with an exception and remove the class from the class table. This is only possible
	 * if no variance obligations on the current class have been added during autoloading. */
	zend_class_entry *parent = NULL;
	zend_class_entry **traits_and_interfaces = NULL;
	zend_class_entry *proto = NULL;
	zend_class_entry *orig_linking_class;
	uint32_t is_cacheable = ce->ce_flags & ZEND_ACC_IMMUTABLE;
	uint32_t i, j;
	zval *zv;
	ALLOCA_FLAG(use_heap)

	SET_ALLOCA_FLAG(use_heap);
	ZEND_ASSERT(!(ce->ce_flags & ZEND_ACC_LINKED));
	
	// 如果有父类名
	if (ce->parent_name) {
		// 获取类，找不到会报错。p1:类名，p2:key，p3:flags
		// 用类名获取父类， 允许真正链接过的类 ，允许exception
		parent = zend_fetch_class_by_name(
			ce->parent_name, lc_parent_name,
			ZEND_FETCH_CLASS_ALLOW_NEARLY_LINKED | ZEND_FETCH_CLASS_EXCEPTION);
		// 如果父类无效
		if (!parent) {
			//  检查 不可恢复的加载失败
			check_unrecoverable_load_failure(ce);
			return NULL;
		}
		// is_cacheable &= (ce)->ce_flags;
		UPDATE_IS_CACHEABLE(parent);
	}

	// 如果有 trait 或接口
	if (ce->num_traits || ce->num_interfaces) {
		// 开辟内存创建 trait/接口列表
		traits_and_interfaces = do_alloca(sizeof(zend_class_entry*) * (ce->num_traits + ce->num_interfaces), use_heap);
		// 遍历 所有 trait
		for (i = 0; i < ce->num_traits; i++) {
			// 获取类，找不到会报错。p1:类名，p2:key，p3:flags
			zend_class_entry *trait = zend_fetch_class_by_name(ce->trait_names[i].name,
				ce->trait_names[i].lc_name, ZEND_FETCH_CLASS_TRAIT);
			// 如果没找到 此trait
			if (UNEXPECTED(trait == NULL)) {
				// 释放 临时变量
				free_alloca(traits_and_interfaces, use_heap);
				// 返回null
				return NULL;
			}
			// 如果 找到的不是trait
			if (UNEXPECTED(!(trait->ce_flags & ZEND_ACC_TRAIT))) {
				// 报错，不可以用，它不是trait
				zend_error_noreturn(E_ERROR, "%s cannot use %s - it is not a trait", ZSTR_VAL(ce->name), ZSTR_VAL(trait->name));
				// 释放 临时变量
				free_alloca(traits_and_interfaces, use_heap);
				/ 返回null
				return NULL;
			}
			// 从临时列表中查找 此 trait
			for (j = 0; j < i; j++) {
				// 如果找到
				if (traits_and_interfaces[j] == trait) {
					/* skip duplications */
					// 从临时列表里删除，避免重复
					trait = NULL;
					// 找到一个就跳出
					break;
				}
			}
			// 把 trait 加到临时列表中（只留最后一个）
			traits_and_interfaces[i] = trait;
			// 如果trait有效
			if (trait) {
				// 更新 缓存 标记
				// is_cacheable &= (ce)->ce_flags;
				UPDATE_IS_CACHEABLE(trait);
			}
		}
	}

	// 如果有接口
	if (ce->num_interfaces) {
		// 遍历 每个接口
		for (i = 0; i < ce->num_interfaces; i++) {
			// 获取类，找不到会报错。p1:类名，p2:key，p3:flags
			// 通过名称获取接口 标记：获取接口 | 允许真实链接的类 | 允许exception
			zend_class_entry *iface = zend_fetch_class_by_name(
				ce->interface_names[i].name, ce->interface_names[i].lc_name,
				ZEND_FETCH_CLASS_INTERFACE |
				ZEND_FETCH_CLASS_ALLOW_NEARLY_LINKED | ZEND_FETCH_CLASS_EXCEPTION);
			// 如果没找到接口
			if (!iface) {
				//  检查 不可恢复的加载失败
				check_unrecoverable_load_failure(ce);
				// 释放列表
				free_alloca(traits_and_interfaces, use_heap);
				return NULL;
			}
			// 把接口加入到trait和接口列表中
			traits_and_interfaces[ce->num_traits + i] = iface;
			// 如果接口有效
			if (iface) {
				// 更新 缓存 标记
				// is_cacheable &= (ce)->ce_flags;
				UPDATE_IS_CACHEABLE(iface);
			}
		}
	}

// 调试用
#ifndef ZEND_WIN32
	if (ce->ce_flags & ZEND_ACC_ENUM) {
		/* We will add internal methods. */
		is_cacheable = false;
	}
#endif

	// 原来是否需要记录错误
	bool orig_record_errors = EG(record_errors);

	// 如果类 是 不可更改 并且 可捕获错误
	if (ce->ce_flags & ZEND_ACC_IMMUTABLE && is_cacheable) {
		// 如果有 zend_inheritance_cache_get 和 zend_inheritance_cache_add 方法
		// 这两个方法在 \ext\opcache\ZendAccelerator.c 中定义，默认没有
		if (zend_inheritance_cache_get && zend_inheritance_cache_add) {
			// 取得继承缓存 
			zend_class_entry *ret = zend_inheritance_cache_get(ce, parent, traits_and_interfaces);
			// 如果获取成功
			if (ret) {
				// 如果有trait和接口列表
				if (traits_and_interfaces) {
					// 释放trait和接口列表
					free_alloca(traits_and_interfaces, use_heap);
				}
				// 从编译类表中取得此类
				zv = zend_hash_find_known_hash(CG(class_table), key);
				// 把zv的指针指向 刚获取到的缓存类
				Z_CE_P(zv) = ret;
				// 返回缓存类
				return ret;
			}

			// 保证继承中产生的警告（如弃用）会被记录进继承缓存 
			/* Make sure warnings (such as deprecations) thrown during inheritance
			 * will be recorded in the inheritance cache. */
			 
			// 开启记录错误信息
			zend_begin_record_errors();
		// 如果没有两个缓存方法
		} else {
			// 不可使用缓存
			is_cacheable = 0;
		}
		// ce作为原型
		proto = ce;
	}

	zend_try {
		// 如果是不可更改
		if (ce->ce_flags & ZEND_ACC_IMMUTABLE) {
			// 懒加载类
			/* Lazy class loading */
			// 懒加载 类
			ce = zend_lazy_class_load(ce);
			// 从编译类表中获取此类
			zv = zend_hash_find_known_hash(CG(class_table), key);
			// 更把类更新到列表元素中
			Z_CE_P(zv) = ce;
		// 如果不是不可更改，并且可使用文件缓存
		} else if (ce->ce_flags & ZEND_ACC_FILE_CACHED) {
			// 懒加载类
			/* Lazy class loading */
			// 懒加载 类
			ce = zend_lazy_class_load(ce);
			// 删除文件缓存标记
			ce->ce_flags &= ~ZEND_ACC_FILE_CACHED;
			// // 从编译类表中获取此类
			zv = zend_hash_find_known_hash(CG(class_table), key);
			// 更把类更新到列表元素中
			Z_CE_P(zv) = ce;
		}

		// 如果有未链接的 uses
		if (CG(unlinked_uses)) {
			// 从 未链接的 uses 中删除此类
			zend_hash_index_del(CG(unlinked_uses), (zend_long)(zend_uintptr_t) ce);
		}

		// 原本正在链接的类
		orig_linking_class = CG(current_linking_class);
		// 可缓存时，记录当前类，否则 null
		CG(current_linking_class) = is_cacheable ? ce : NULL;

		// 如果此类是 枚举
		if (ce->ce_flags & ZEND_ACC_ENUM) {
			// 继承过程中， 只注册内置枚举方法，来避免它们一直存在在 opcache里
			/* Only register builtin enum methods during inheritance to avoid persisting them in
			 * opcache. */
			zend_enum_register_funcs(ce);
		}

		// 如果有父类
		if (parent) {
			// 如果父类没有链接成功
			if (!(parent->ce_flags & ZEND_ACC_LINKED)) {
				// 添加从属责任。（从属责任对象关关联到从属类）
				add_dependency_obligation(ce, parent);
			}
			// 执行继承
			zend_do_inheritance(ce, parent);
		}
		// 如果有引用trait
		if (ce->num_traits) {
			// 绑定trait，p1:类，p2:trait列表
			zend_do_bind_traits(ce, traits_and_interfaces);
		}
		// 如果有实现接口
		if (ce->num_interfaces) {
			// 这里也要复制父类的接口，这样就不用后继再重新分配了
			/* Also copy the parent interfaces here, so we don't need to reallocate later. */
			// 父类接口数量 
			uint32_t num_parent_interfaces = parent ? parent->num_interfaces : 0;
			// 分配接口列表
			zend_class_entry **interfaces = emalloc(
					sizeof(zend_class_entry *) * (ce->num_interfaces + num_parent_interfaces));
			// 如果有父类接口
			if (num_parent_interfaces) {
				// 先把父类接口复制过来
				memcpy(interfaces, parent->interfaces,
					   sizeof(zend_class_entry *) * num_parent_interfaces);
			}
			// 再复制 traits_and_interfaces 中的接口
			memcpy(interfaces + num_parent_interfaces, traits_and_interfaces + ce->num_traits,
				   sizeof(zend_class_entry *) * ce->num_interfaces);
			// 实现接口列表中的接口
			zend_do_implement_interfaces(ce, interfaces);
		// 如果子类没有实现接口，并且父类有接口
		} else if (parent && parent->num_interfaces) {
			// 继承父类的接口
			zend_do_inherit_interfaces(ce, parent);
		}
		// （如果不是接口 也不是 trait） 并且 类有（显式或隐式）抽象标记
		if (!(ce->ce_flags & (ZEND_ACC_INTERFACE|ZEND_ACC_TRAIT))
			&& (ce->ce_flags & (ZEND_ACC_IMPLICIT_ABSTRACT_CLASS|ZEND_ACC_EXPLICIT_ABSTRACT_CLASS))
				) {
			// 检验是否需要标记成抽象类，或实现某些抽象方法
			zend_verify_abstract_class(ce);
		}
		// 如果类是枚举
		if (ce->ce_flags & ZEND_ACC_ENUM) {
			// 检验 枚举 （属性，魔术方法，接口）
			zend_verify_enum(ce);
		}

		// 通常 Stringable 是在编译中添加的。但是如果它来自于trait，需要显式添加 此接口
		/* Normally Stringable is added during compilation. However, if it is imported from a trait,
		 * we need to explicilty add the interface here. */
		 // 如果有__tostring 并且 不是trait 并且 没有实现 zend_ce_stringable 接口 
		if (ce->__tostring && !(ce->ce_flags & ZEND_ACC_TRAIT)
			&& !zend_class_implements_interface(ce, zend_ce_stringable)) {
			// __tostring方法必须来自trait
			ZEND_ASSERT(ce->__tostring->common.fn_flags & ZEND_ACC_TRAIT_CLONE);
			// 添加 已解决接口 标记
			ce->ce_flags |= ZEND_ACC_RESOLVED_INTERFACES;
			// 接口数量 +1
			ce->num_interfaces++;
			// 调整接口列表 内存大小
			ce->interfaces = perealloc(ce->interfaces,
									   sizeof(zend_class_entry *) * ce->num_interfaces, ce->type == ZEND_INTERNAL_CLASS);
			// 添加接口 zend_ce_stringable
			ce->interfaces[ce->num_interfaces - 1] = zend_ce_stringable;
			// 实现此接口
			do_interface_implementation(ce, zend_ce_stringable);
		}
		// // 构造属性信息表
		zend_build_properties_info_table(ce);
	} zend_catch {
		// 如果出错
		/* Do not leak recorded errors to the next linked class. */
		// 如果有原来的错误记录
		if (!orig_record_errors) {
			// 停止记录错误信息
			EG(record_errors) = false;
			// 释放错误记录
			zend_free_recorded_errors();
		}
		// 跳伞 ？
		zend_bailout();
	} zend_end_try();
	
	// 执行时记录 错误信息
	EG(record_errors) = orig_record_errors;

	// 如果 没有未完成的变异
	if (!(ce->ce_flags & ZEND_ACC_UNRESOLVED_VARIANCE)) {
		// 链接完毕
		ce->ce_flags |= ZEND_ACC_LINKED;
	// 有未完成的变异
	} else {
		// 添加链接完毕标记
		ce->ce_flags |= ZEND_ACC_NEARLY_LINKED;
		// 如果有正在链接的类
		if (CG(current_linking_class)) {
			// 添加 cacheable标记
			ce->ce_flags |= ZEND_ACC_CACHEABLE;
		}
		// 加载延时类
		load_delayed_classes(ce);
		// 如果有未完成的 变异
		if (ce->ce_flags & ZEND_ACC_UNRESOLVED_VARIANCE) {
			// 解决延时变异责任（主要是检查责任）
			resolve_delayed_variance_obligations(ce);
		}
		// 如果有 ZEND_ACC_CACHEABLE 标记
		if (ce->ce_flags & ZEND_ACC_CACHEABLE) {
			// 删除 ZEND_ACC_CACHEABLE 标记
			ce->ce_flags &= ~ZEND_ACC_CACHEABLE;
		// 否则 
		} else {
			// 清空当前正在链接的类
			CG(current_linking_class) = NULL;
		}
	}

	// 如果没有正在链接的类
	if (!CG(current_linking_class)) {
		// 不可cache
		is_cacheable = 0;
	}
	// 还原正在链接的类
	CG(current_linking_class) = orig_linking_class;
	
	// 如果可缓存 
	if (is_cacheable) {
		// 继承缓存表
		HashTable *ht = (HashTable*)ce->inheritance_cache;
		zend_class_entry *new_ce;
		// 初始为空
		ce->inheritance_cache = NULL;
		// 添加继承缓存
		new_ce = zend_inheritance_cache_add(ce, proto, parent, traits_and_interfaces, ht);
		// 如果添加缓存成功
		if (new_ce) {
			// 找到编译类表中的类
			zv = zend_hash_find_known_hash(CG(class_table), key);
			// 使用缓存返回的新实例
			ce = new_ce;
			// 更新类表中的元素
			Z_CE_P(zv) = ce;
		}
		// 如果有继承缓存表
		if (ht) {
			// 销毁继承缓存表
			zend_hash_destroy(ht);
			// 释放继承缓存表
			FREE_HASHTABLE(ht);
		}
	}
	
	// 如果原来不要求记录错误
	if (!orig_record_errors) {
		// 释放错误记录
		zend_free_recorded_errors();
	}
	
	// 如果有trait和接口列表
	if (traits_and_interfaces) {
		// 释放列表
		free_alloca(traits_and_interfaces, use_heap);
	}
	
	// 如果有类缓存 
	if (ZSTR_HAS_CE_CACHE(ce->name)) {
		// 更新类缓存
		ZSTR_SET_CE_CACHE(ce->name, ce);
	}

	return ce;
}
/* }}} */

// 通过在继承查检中的未完成的类型，检查早期绑定是否已经禁止
/* Check whether early binding is prevented due to unresolved types in inheritance checks. */
// ing3, 检查所有属性和方法，在继承上是否有问题
static inheritance_status zend_can_early_bind(zend_class_entry *ce, zend_class_entry *parent_ce) /* {{{ */
{
	zend_string *key;
	zend_function *parent_func;
	zend_property_info *parent_info;
	inheritance_status overall_status = INHERITANCE_SUCCESS;
	// 遍历 父类方法表
	ZEND_HASH_MAP_FOREACH_STR_KEY_PTR(&parent_ce->function_table, key, parent_func) {
		// 在子类方法表中查找此方法
		zval *zv = zend_hash_find_known_hash(&ce->function_table, key);
		// 如果找到
		if (zv) {
			zend_function *child_func = Z_FUNC_P(zv);
			// 对方法进行继承检查 。p1:子方法，p2:子类，p3:父方法，p4:父类，p5:?,p6:?,p7:检查可见性，p8,只检查，p9:检查
			inheritance_status status =
				do_inheritance_check_on_method_ex(
					child_func, child_func->common.scope,
					parent_func, parent_func->common.scope,
					ce, NULL, /* check_visibility */ 1, 1, 0);
			// 如果状态是 继承警告
			if (UNEXPECTED(status == INHERITANCE_WARNING)) {
				overall_status = INHERITANCE_WARNING;
			// 如果状态不是 继承成功
			} else if (UNEXPECTED(status != INHERITANCE_SUCCESS)) {
				return status;
			}
		}
	} ZEND_HASH_FOREACH_END();

	// 遍历所有属性信息
	ZEND_HASH_MAP_FOREACH_STR_KEY_PTR(&parent_ce->properties_info, key, parent_info) {
		zval *zv;
		// 如果父属性信息是 private 或 有规定类型
		if ((parent_info->flags & ZEND_ACC_PRIVATE) || !ZEND_TYPE_IS_SET(parent_info->type)) {
			// 下一个
			continue;
		}
		// 在子类属性信息表中查找这个 信息
		zv = zend_hash_find_known_hash(&ce->properties_info, key);
		// 如果找到
		if (zv) {
			// 取出子类属性信息
			zend_property_info *child_info = Z_PTR_P(zv);
			// 如果它有规定类型
			if (ZEND_TYPE_IS_SET(child_info->type)) {
				// 检查属性类型是否兼容（双向可继承）
				inheritance_status status = property_types_compatible(parent_info, child_info);
				// 状态不可以是：继承警告
				ZEND_ASSERT(status != INHERITANCE_WARNING);
				// 如果状态不是继承成功
				if (UNEXPECTED(status != INHERITANCE_SUCCESS)) {
					// 中断，返回此状态
					return status;
				}
			}
		}
	} ZEND_HASH_FOREACH_END();

	// 返回方法覆盖状态
	return overall_status;
}
/* }}} */

// ing3, 注册早期绑定类
static zend_always_inline bool register_early_bound_ce(zval *delayed_early_binding, zend_string *lcname, zend_class_entry *ce) {
	// 如果有延时早期绑定
	if (delayed_early_binding) {
		// 如果类没有预加载标记
		if (EXPECTED(!(ce->ce_flags & ZEND_ACC_PRELOADED))) {
			// 修改执行时类表中的类名
			if (zend_hash_set_bucket_key(EG(class_table), (Bucket *)delayed_early_binding, lcname) != NULL) {
				// 延时早期绑定中 
				Z_CE_P(delayed_early_binding) = ce;
				// 返回成功
				return true;
			}
			// 如果失败会走这里
			
		// 如果类有预加载标记
		} else {
			// 如果用了预加载 ，不要替换存在的bucket，添加新的
			/* If preloading is used, don't replace the existing bucket, add a new one. */
			// 执行时类表中，添加当前类
			if (zend_hash_add_ptr(EG(class_table), lcname, ce) != NULL) {
				// 返回成功
				return true;
			}
			// 如果已经有了会走这里
		}
		// 报错：不可定义类名 A 因为它已被使用
		zend_error_noreturn(E_COMPILE_ERROR, "Cannot declare %s %s, because the name is already in use", zend_get_object_type(ce), ZSTR_VAL(ce->name));
		// 返回失败
		return false;
	}
	// 如果没有 延时早期绑定（有的话已经在上面返回了）
	// 把类添加到编译时类表中
	if (zend_hash_add_ptr(CG(class_table), lcname, ce) != NULL) {
		// 返回成功
		return true;
	}
	// 返回失败
	return false;
}

// ing2, 尝试早期绑定
ZEND_API zend_class_entry *zend_try_early_bind(zend_class_entry *ce, zend_class_entry *parent_ce, zend_string *lcname, zval *delayed_early_binding) /* {{{ */
{
	inheritance_status status;
	zend_class_entry *proto = NULL;
	zend_class_entry *orig_linking_class;
	// 检查不可更改标记，不可更改的可以被缓存。
	uint32_t is_cacheable = ce->ce_flags & ZEND_ACC_IMMUTABLE;
	// is_cacheable &= (ce)->ce_flags;
	UPDATE_IS_CACHEABLE(parent_ce);
	// 如果可以缓存
	if (is_cacheable) {
		// 如果有缓存管理方法 ,默认没有。ext\opcache\ZendAccelerator.c 中添加
		if (zend_inheritance_cache_get && zend_inheritance_cache_add) {
			// 从缓存中获取类入口
			zend_class_entry *ret = zend_inheritance_cache_get(ce, parent_ce, NULL);
			// 如果找到了
			if (ret) {
				// 注册早期绑定类
				if (UNEXPECTED(!register_early_bound_ce(delayed_early_binding, lcname, ret))) {
					// 不成功返回null
					return NULL;
				}
				// 依次调用每个 链接回调 函数
				zend_observer_class_linked_notify(ret, lcname);
				// 返回缓存中的类入口
				return ret;
			}
		// 没有缓存管理方法
		} else {
			// 不能缓存 
			is_cacheable = 0;
		}
		// 缓存获取失败，ce作为原型
		proto = ce;
	}

	// 原本正在链接的类
	orig_linking_class = CG(current_linking_class);
	// 清空正在链接的类
	CG(current_linking_class) = NULL;
	// 检查所有属性和方法，在继承上是否有问题
	status = zend_can_early_bind(ce, parent_ce);
	// 恢复正在链接的类
	CG(current_linking_class) = orig_linking_class;
	// 如果状态不是未完成
	if (EXPECTED(status != INHERITANCE_UNRESOLVED)) {
		// 如果类是不可更改的
		if (ce->ce_flags & ZEND_ACC_IMMUTABLE) {
			// 懒加载
			/* Lazy class loading */
			// 懒加载 类
			ce = zend_lazy_class_load(ce);
		// 如果有文件缓存
		} else if (ce->ce_flags & ZEND_ACC_FILE_CACHED) {
			// 懒加载
			/* Lazy class loading */
			// 懒加载 类
			ce = zend_lazy_class_load(ce);
			// 去掉文件缓存标记
			ce->ce_flags &= ~ZEND_ACC_FILE_CACHED;
		}
		// 注册早期绑定类
		if (UNEXPECTED(!register_early_bound_ce(delayed_early_binding, lcname, ce))) {
			return NULL;
		}
		// 记录原本在链接的类
		orig_linking_class = CG(current_linking_class);
		// 如果有用到缓存，当前类记录 到正在链接
		CG(current_linking_class) = is_cacheable ? ce : NULL;

		zend_try{
			// 如果用到缓存 
			if (is_cacheable) {
				// 开始记录错误信息
				zend_begin_record_errors();
			}

			// 继承，追踪第三个参数？
			zend_do_inheritance_ex(ce, parent_ce, status == INHERITANCE_SUCCESS);
			// 如果父类有接口
			if (parent_ce && parent_ce->num_interfaces) {
				// 继承父类的接口
				zend_do_inherit_interfaces(ce, parent_ce);
			}
			// 构造属性信息表
			zend_build_properties_info_table(ce);
			// 如果此类是 是 隐式抽象类 不是 接口、trait、显式抽象类
			if ((ce->ce_flags & (ZEND_ACC_IMPLICIT_ABSTRACT_CLASS|ZEND_ACC_INTERFACE|ZEND_ACC_TRAIT|ZEND_ACC_EXPLICIT_ABSTRACT_CLASS)) == ZEND_ACC_IMPLICIT_ABSTRACT_CLASS) {
				// 检查抽象类
				zend_verify_abstract_class(ce);
			}
			// 不可以有未解决变异 标记
			ZEND_ASSERT(!(ce->ce_flags & ZEND_ACC_UNRESOLVED_VARIANCE));
			// 链接完成
			ce->ce_flags |= ZEND_ACC_LINKED;
			// 恢复正在链接的类
			CG(current_linking_class) = orig_linking_class;
			
		// 如果捕获到异常
		} zend_catch {
			// 不记录错误
			EG(record_errors) = false;
			// 释放错误记录
			zend_free_recorded_errors();
			// 跳伞 ？？
			zend_bailout();
		} zend_end_try();
		
		// 不记录错误
		EG(record_errors) = false;

		// 如果可缓存
		if (is_cacheable) {
			// 继承缓存哈希表
			HashTable *ht = (HashTable*)ce->inheritance_cache;
			// 临时变量
			zend_class_entry *new_ce;
			// 清空 继承缓存哈希表 指针
			ce->inheritance_cache = NULL;
			// 添加继承缓存
			new_ce = zend_inheritance_cache_add(ce, proto, parent_ce, NULL, ht);
			// 如果新类有效
			if (new_ce) {
				// 在类表中查找此类
				zval *zv = zend_hash_find_known_hash(CG(class_table), lcname);
				// 使用新类
				ce = new_ce;
				// 使用此新类替换旧的类
				Z_CE_P(zv) = ce;
			}
			// 如果 继承缓存哈希表 有效
			if (ht) {
				// 销毁此哈希表
				zend_hash_destroy(ht);
				// 释放此哈希表
				FREE_HASHTABLE(ht);
			}
		}
		
		// 如果此类有缓存 
		if (ZSTR_HAS_CE_CACHE(ce->name)) {
			// 更新缓存
			ZSTR_SET_CE_CACHE(ce->name, ce);
		}
		// 依次调用每个 链接回调 函数
		zend_observer_class_linked_notify(ce, lcname);
		// 返回刚刚加载的类
		return ce;
	}
	// 
	return NULL;
}
/* }}} */
