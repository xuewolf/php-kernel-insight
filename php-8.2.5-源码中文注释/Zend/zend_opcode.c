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

#include <stdio.h>

#include "zend.h"
#include "zend_alloc.h"
#include "zend_compile.h"
#include "zend_extensions.h"
#include "zend_API.h"
#include "zend_sort.h"
#include "zend_constants.h"
#include "zend_observer.h"

#include "zend_vm.h"

// 有很多销毁方法

// ing3, 调用扩展模块的 操作码构造方法
static void zend_extension_op_array_ctor_handler(zend_extension *extension, zend_op_array *op_array)
{
	if (extension->op_array_ctor) {
		extension->op_array_ctor(op_array);
	}
}

// ing3, 调用扩展模块的 操作码析构方法
static void zend_extension_op_array_dtor_handler(zend_extension *extension, zend_op_array *op_array)
{
	if (extension->op_array_dtor) {
		extension->op_array_dtor(op_array);
	}
}

// ing3, 初始化操作码列表。p1:操作码列表，p2:类型，p3:操作码列表数量
void init_op_array(zend_op_array *op_array, zend_uchar type, int initial_ops_size)
{
	// 类型
	op_array->type = type;
	// 3个参数标记都是0
	op_array->arg_flags[0] = 0;
	op_array->arg_flags[1] = 0;
	op_array->arg_flags[2] = 0;
	// 引用数
	op_array->refcount = (uint32_t *) emalloc(sizeof(uint32_t));
	*op_array->refcount = 1;
	// 编译变量数 ？？
	op_array->last = 0;
	// 分配内存创建 操作码列表
	op_array->opcodes = emalloc(initial_ops_size * sizeof(zend_op));
	// 临时变量数量
	op_array->last_var = 0;
	// 变量数量
	op_array->vars = NULL;
	// 临时变量数
	op_array->T = 0;
	// 函数名
	op_array->function_name = NULL;
	// 文件名
	op_array->filename = zend_string_copy(zend_get_compiled_filename());
	// 专用注释
	op_array->doc_comment = NULL;
	// 修饰属性
	op_array->attributes = NULL;

	// 参数信息
	op_array->arg_info = NULL;
	// 参数个数
	op_array->num_args = 0;
	// 必须参数个数
	op_array->required_num_args = 0;

	// 域
	op_array->scope = NULL;
	// 原型
	op_array->prototype = NULL;
	// 活动区域
	op_array->live_range = NULL;
	// try/catch 列表
	op_array->try_catch_array = NULL;
	// 最后一个 活动区域序号
	op_array->last_live_range = 0;
	// 静态变量
	op_array->static_variables = NULL;
	// 初始化 静态变量指针列表
	ZEND_MAP_PTR_INIT(op_array->static_variables_ptr, NULL);
	// try/catch 数量
	op_array->last_try_catch = 0;
	// 函数标记
	op_array->fn_flags = 0;

	// literal 数量（也是最后一个 literal 编号）
	op_array->last_literal = 0;
	// 这个是作为 类型名，方法名 的字串列表
	op_array->literals = NULL;

	// 动态调用定义 数量
	op_array->num_dynamic_func_defs = 0;
	// 动态调用定义 列表
	op_array->dynamic_func_defs = NULL;

	// 初始化 运行时缓存 指针列表
	ZEND_MAP_PTR_INIT(op_array->run_time_cache, NULL);
	// 扩展处理器指针 缓存大小 ？？
	op_array->cache_size = zend_op_array_extension_handles * sizeof(void*);
	// 置空， 保留指针列表 ？？
	memset(op_array->reserved, 0, ZEND_MAX_RESERVED_RESOURCES * sizeof(void*));

	// 如果扩展里有操作码构造器
	if (zend_extension_flags & ZEND_EXTENSIONS_HAVE_OP_ARRAY_CTOR) {
		// 调用构造器
		zend_llist_apply_with_argument(&zend_extensions, (llist_apply_with_arg_func_t) zend_extension_op_array_ctor_handler, op_array);
	}
}

// ing3, 销毁函数
ZEND_API void destroy_zend_function(zend_function *function)
{
	zval tmp;
	// 先放进临时zval里
	ZVAL_PTR(&tmp, function);
	// 销毁 zval
	zend_function_dtor(&tmp);
}

// ing3, 释放类型
ZEND_API void zend_type_release(zend_type type, bool persistent) {
	// 如果是列表类型
	if (ZEND_TYPE_HAS_LIST(type)) {
		zend_type *list_type, *sublist_type;
		// 遍历列表
		ZEND_TYPE_LIST_FOREACH(ZEND_TYPE_LIST(type), list_type) {
			// 如此元素又是列表
			if (ZEND_TYPE_HAS_LIST(*list_type)) {
				// 遍历此列表
				ZEND_TYPE_LIST_FOREACH(ZEND_TYPE_LIST(*list_type), sublist_type) {
					// 如果此子类型有名称
					if (ZEND_TYPE_HAS_NAME(*sublist_type)) {
						// 释放此子类型的名称
						zend_string_release(ZEND_TYPE_NAME(*sublist_type));
					}
				} ZEND_TYPE_LIST_FOREACH_END();
			// 此元素不是列表
			} else if (ZEND_TYPE_HAS_NAME(*list_type)) {
				// 释放此类型的名称
				zend_string_release(ZEND_TYPE_NAME(*list_type));
			}
		} ZEND_TYPE_LIST_FOREACH_END();
		// 如果此类型不是使用 arena 分配内存
		if (!ZEND_TYPE_USES_ARENA(type)) {
			// 释放此列表类型
			pefree(ZEND_TYPE_LIST(type), persistent);
		}
		// 是arena分配的这里不释放 ，什么时候释放呢？
	// 不是列表类型
	} else if (ZEND_TYPE_HAS_NAME(type)) {
		// 直接释放类型名
		zend_string_release(ZEND_TYPE_NAME(type));
	}
}

// ing3, 释放内部参数信息（列表）
void zend_free_internal_arg_info(zend_internal_function *function) {
	// 如果 有返回类型或 参数类型信息 ，并且有参数信列表
	if ((function->fn_flags & (ZEND_ACC_HAS_RETURN_TYPE|ZEND_ACC_HAS_TYPE_HINTS)) &&
		function->arg_info) {
		//
		uint32_t i;
		// 参数数量 = 函数参数数量 +1
		uint32_t num_args = function->num_args + 1;
		// 参数信息列表，倒回去1个
		zend_internal_arg_info *arg_info = function->arg_info - 1;
		// 如果支持字典参数
		if (function->fn_flags & ZEND_ACC_VARIADIC) {
			// 参数数量 +1
			num_args++;
		}
		// 遍历每个参数信息
		for (i = 0 ; i < num_args; i++) {
			// 销毁，每个参数 类型
			zend_type_release(arg_info[i].type, /* persistent */ 1);
		}
		// 删除参数信息列表
		free(arg_info);
	}
}

// ing3, 销毁函数
ZEND_API void zend_function_dtor(zval *zv)
{
	zend_function *function = Z_PTR_P(zv);
	// 如果是用户定义函数
	if (function->type == ZEND_USER_FUNCTION) {
		// 必定有函数名（闭包是对象，不走这里）
		ZEND_ASSERT(function->common.function_name);
		// 销毁操作码
		destroy_op_array(&function->op_array);
		// 操作码是通过 arena 分配的，所以这里不用释放内存
		/* op_arrays are allocated on arena, so we don't have to free them */
	// 不是用户定义函数
	} else {
		// 必定是内置函数
		ZEND_ASSERT(function->type == ZEND_INTERNAL_FUNCTION);
		// 必定有函数名
		ZEND_ASSERT(function->common.function_name);
		// 释放函数名
		zend_string_release_ex(function->common.function_name, 1);
		// 对象类方法，需要显示调用
		/* For methods this will be called explicitly. */
		// 如果没有所属类
		if (!function->common.scope) {
			// 释放内置参数信息
			zend_free_internal_arg_info(&function->internal_function);
			// 如果有修饰属性
			if (function->common.attributes) {
				// 释放函数的修饰属性
				zend_hash_release(function->common.attributes);
				// 清空指针
				function->common.attributes = NULL;
			}
		}
		// 如果是通过 arena 分配的
		if (!(function->common.fn_flags & ZEND_ACC_ARENA_ALLOCATED)) {
			// 释放此函数
			pefree(function, 1);
		}
	}
}

// ing3, 清空内置类的数据（属性信息 和 static_members）
ZEND_API void zend_cleanup_internal_class_data(zend_class_entry *ce)
{
	// 如果有静态成员，并且有 静态成员表【指针列表】
	if (ZEND_MAP_PTR(ce->static_members_table) && CE_STATIC_MEMBERS(ce)) {
		// 取得静态成员列表
		zval *static_members = CE_STATIC_MEMBERS(ce);
		// 列表开头
		zval *p = static_members;
		// 列表结尾
		zval *end = p + ce->default_static_members_count;
		// 清空指针
		ZEND_MAP_PTR_SET(ce->static_members_table, NULL);
		// 遍历每个成员
		while (p != end) {
			// 如果是引用类型
			if (UNEXPECTED(Z_ISREF_P(p))) {
				zend_property_info *prop_info;
				// 追踪引用并遍历
				ZEND_REF_FOREACH_TYPE_SOURCES(Z_REF_P(p), prop_info) {
					// 如果属性信息属于此类 并且 prop_info 的序号正确
					if (prop_info->ce == ce && p - static_members == prop_info->offset) {
						// 找到并删除源指针列表中 指向属性信息的指针。p1:源列表，p2:属性信息指针
						ZEND_REF_DEL_TYPE_SOURCE(Z_REF_P(p), prop_info);
						// 跳出循环。在这里停止遍历，数组可能被重新分配了。
						break; /* stop iteration here, the array might be realloc()'ed */
					}
				} ZEND_REF_FOREACH_TYPE_SOURCES_END();
			}
			// 销毁此元素
			i_zval_ptr_dtor(p);
			// 下一个元素
			p++;
		}
		// 释放静态成员列表
		efree(static_members);
	}
}

// ing4, 销毁类中引用的trait信息。p1:类
static void _destroy_zend_class_traits_info(zend_class_entry *ce)
{
	uint32_t i;

	// 遍历所有trait
	for (i = 0; i < ce->num_traits; i++) {
		// 释放trait名
		zend_string_release_ex(ce->trait_names[i].name, 0);
		// 释放trait小写名
		zend_string_release_ex(ce->trait_names[i].lc_name, 0);
	}
	// 释放trait名列表
	efree(ce->trait_names);

	// 如果有别名
	if (ce->trait_aliases) {
		// 循环变量
		i = 0;
		// 遍历 所有别名
		while (ce->trait_aliases[i]) {
			// 如果有方法名
			if (ce->trait_aliases[i]->trait_method.method_name) {
				// 释放方法名
				zend_string_release_ex(ce->trait_aliases[i]->trait_method.method_name, 0);
			}
			// 如果有 方法所属类名
			if (ce->trait_aliases[i]->trait_method.class_name) {
				// 释放 方法所属类名
				zend_string_release_ex(ce->trait_aliases[i]->trait_method.class_name, 0);
			}
			// 如果有别名
			if (ce->trait_aliases[i]->alias) {
				// 释放别名
				zend_string_release_ex(ce->trait_aliases[i]->alias, 0);
			}
			// 释放这个trait别名
			efree(ce->trait_aliases[i]);
			// 下一个
			i++;
		}
		// 释放trait 别名列表
		efree(ce->trait_aliases);
	}

	// 如果有trait优先表
	if (ce->trait_precedences) {
		uint32_t j;
		// 循环变量
		i = 0;
		// 遍历 trait优先表
		while (ce->trait_precedences[i]) {
			// 释放方法名
			zend_string_release_ex(ce->trait_precedences[i]->trait_method.method_name, 0);
			// 释放类名
			zend_string_release_ex(ce->trait_precedences[i]->trait_method.class_name, 0);
			// 遍历排除列表
			for (j = 0; j < ce->trait_precedences[i]->num_excludes; j++) {
				// 释放排除类名
				zend_string_release_ex(ce->trait_precedences[i]->exclude_class_names[j], 0);
			}
			// 释放 trait优先表 中的元素
			efree(ce->trait_precedences[i]);
			// 下一个元素
			i++;
		}
		// 释放整个trait优先表
		efree(ce->trait_precedences);
	}
}

// ing3, 清空类的转换数据
ZEND_API void zend_cleanup_mutable_class_data(zend_class_entry *ce)
{
	// 取得类的转换数据
	zend_class_mutable_data *mutable_data = ZEND_MAP_PTR_GET_IMM(ce->mutable_data);
	// 如果有转换数据
	if (mutable_data) {
		HashTable *constants_table;
		zval *p;
		// 转换数据中的常量表
		constants_table = mutable_data->constants_table;
		// 如果和类常量表不同
		if (constants_table && constants_table != &ce->constants_table) {
			zend_class_constant *c;
			// 遍历此常量表
			ZEND_HASH_MAP_FOREACH_PTR(constants_table, c) {
				// 如果常量属于此类 并且常量值属于常量
				if (c->ce == ce || (Z_CONSTANT_FLAGS(c->value) & CONST_OWNED)) {
					// 销毁常量值
					zval_ptr_dtor_nogc(&c->value);
				}
			} ZEND_HASH_FOREACH_END();
			// 销毁常量表
			zend_hash_destroy(constants_table);
			// 清空指针
			mutable_data->constants_table = NULL;
		}
		// 如果转换信息里有属性表
		p = mutable_data->default_properties_table;
		// 如果属性表与此类的属性表不同
		if (p && p != ce->default_properties_table) {
			zval *end = p + ce->default_properties_count;
			// 遍历属性表
			while (p < end) {
				// 销毁此元素
				zval_ptr_dtor_nogc(p);
				// 下一个
				p++;
			}
			// 清空指针
			mutable_data->default_properties_table = NULL;
		}
		// 如果有备用枚举表
		if (mutable_data->backed_enum_table) {
			// 释放此哈希表
			zend_hash_release(mutable_data->backed_enum_table);
			// 清空指针
			mutable_data->backed_enum_table = NULL;
		}
		// 清空 ce->mutable_data__ptr 指针
		ZEND_MAP_PTR_SET_IMM(ce->mutable_data, NULL);
	}
}

// 200 行
// ing3, 销毁类
ZEND_API void destroy_zend_class(zval *zv)
{
	zend_property_info *prop_info;
	zend_class_entry *ce = Z_PTR_P(zv);
	zend_function *fn;
	// 如果是 不能变的，直接返回 ?
	if (ce->ce_flags & ZEND_ACC_IMMUTABLE) {
		return;
	}
	// 如果有文件缓存 ?
	if (ce->ce_flags & ZEND_ACC_FILE_CACHED) {
		zend_class_constant *c;
		zval *p, *end;
		// 遍历常量表
		ZEND_HASH_MAP_FOREACH_PTR(&ce->constants_table, c) {
			// 如果常量属于这个类
			if (c->ce == ce) {
				// 销毁常量（或放入回收队列）
				zval_ptr_dtor_nogc(&c->value);
			}
		} ZEND_HASH_FOREACH_END();
		// 默认属性列表
		if (ce->default_properties_table) {
			// 属性表开头
			p = ce->default_properties_table;
			// 列表结尾（没有元素）
			end = p + ce->default_properties_count;
			// 遍历默认属性表
			while (p < end) {
				// 一个个销毁（放入回收队列）
				zval_ptr_dtor_nogc(p);
				// 下一个
				p++;
			}
		}
		// 
		return;
	}
	// 如果 -1 后类还有引用
	if (--ce->refcount > 0) {
		// 中断
		return;
	}

	// 按类型操作
	switch (ce->type) {
		// 用户类
		case ZEND_USER_CLASS:
			// 如果没有缓存
			if (!(ce->ce_flags & ZEND_ACC_CACHED)) {
				// 如果有父类 并且 没有 实现父类 标记
				if (ce->parent_name && !(ce->ce_flags & ZEND_ACC_RESOLVED_PARENT)) {
					// 释放父类名
					zend_string_release_ex(ce->parent_name, 0);
				}
				// 释放类名
				zend_string_release_ex(ce->name, 0);
				// 释放文件名
				zend_string_release_ex(ce->info.user.filename, 0);
				// 如果有，释放注释文档
				if (ce->info.user.doc_comment) {					
					zend_string_release_ex(ce->info.user.doc_comment, 0);
				}
				// 如果有，释放修饰属性
				if (ce->attributes) {
					zend_hash_release(ce->attributes);
				}
				// 如果有接口列表 但没有实现接口标记
				if (ce->num_interfaces > 0 && !(ce->ce_flags & ZEND_ACC_RESOLVED_INTERFACES)) {
					uint32_t i;
					// 遍历每个接口
					for (i = 0; i < ce->num_interfaces; i++) {
						// 释放接口名
						zend_string_release_ex(ce->interface_names[i].name, 0);
						// 释放小写名
						zend_string_release_ex(ce->interface_names[i].lc_name, 0);
					}
					// 释放接口列表
					efree(ce->interface_names);
				}
				// 如果有引用trait
				if (ce->num_traits > 0) {
					// 销毁所有trait信息
					_destroy_zend_class_traits_info(ce);
				}
			}
			
			// 如果有默认属性表
			if (ce->default_properties_table) {
				zval *p = ce->default_properties_table;
				zval *end = p + ce->default_properties_count;
				// 遍历每个元素
				while (p != end) {
					// 销毁元素
					i_zval_ptr_dtor(p);
					// 下一个
					p++;
				}
				// 释放默认属性表
				efree(ce->default_properties_table);
			}
			// 如果有静态成员表
			if (ce->default_static_members_table) {
				zval *p = ce->default_static_members_table;
				zval *end = p + ce->default_static_members_count;
				// 遍历每个静态成员
				while (p != end) {
					// 可以是引用类型
					ZEND_ASSERT(!Z_ISREF_P(p));
					// 销毁此变量
					i_zval_ptr_dtor(p);
					// 下一个元素
					p++;
				}
				// 释放静态成员表
				efree(ce->default_static_members_table);
			}
			// 遍历属性信息
			ZEND_HASH_MAP_FOREACH_PTR(&ce->properties_info, prop_info) {
				// 如果属性信息属于此类（不是继承的）
				if (prop_info->ce == ce) {
					// 释放 属性信息名
					zend_string_release_ex(prop_info->name, 0);
					// 释放 属性信息中的 注释
					if (prop_info->doc_comment) {
						zend_string_release_ex(prop_info->doc_comment, 0);
					}
					// 释放 属性信息中的 修饰属性
					if (prop_info->attributes) {
						zend_hash_release(prop_info->attributes);
					}
					// 释放修饰属性的 类型
					zend_type_release(prop_info->type, /* persistent */ 0);
				}
			} ZEND_HASH_FOREACH_END();
			// 销毁属性信息表
			zend_hash_destroy(&ce->properties_info);
			// 销毁成员方法表
			zend_hash_destroy(&ce->function_table);
			// 如果有常量表
			if (zend_hash_num_elements(&ce->constants_table)) {
				zend_class_constant *c;
				// 遍历常量表
				ZEND_HASH_MAP_FOREACH_PTR(&ce->constants_table, c) {
					// 如果常量有所属类 并且常量值属于常量
					if (c->ce == ce || (Z_CONSTANT_FLAGS(c->value) & CONST_OWNED)) {
						// 销毁或进入垃圾回收
						zval_ptr_dtor_nogc(&c->value);
						// 如果有注释文档
						if (c->doc_comment) {
							// 释放注释文档
							zend_string_release_ex(c->doc_comment, 0);
						}
						// 如果有修饰属性
						if (c->attributes) {
							// 释放修饰属性
							zend_hash_release(c->attributes);
						}
					}
				} ZEND_HASH_FOREACH_END();
			}
			// 销毁常量表
			zend_hash_destroy(&ce->constants_table);
			// 如果有接口，并且 有实现接口标记
			if (ce->num_interfaces > 0 && (ce->ce_flags & ZEND_ACC_RESOLVED_INTERFACES)) {
				// 释放接口
				efree(ce->interfaces);
			}
			// 如果有备用枚举表
			if (ce->backed_enum_table) {
				// 释放 备用枚举表
				zend_hash_release(ce->backed_enum_table);
			}
			break;
		// 内部类
		case ZEND_INTERNAL_CLASS:
			// 如果有备用枚举表
			if (ce->backed_enum_table) {
				// 释放此枚举表
				zend_hash_release(ce->backed_enum_table);
			}
			// 如果有默认属性表
			if (ce->default_properties_table) {
				// 开始位置
				zval *p = ce->default_properties_table;
				// 末尾（无原素）
				zval *end = p + ce->default_properties_count;
				// 遍历每个元素
				while (p != end) {
					// 销毁内部指针
					zval_internal_ptr_dtor(p);
					// 下一个元素
					p++;
				}
				// 释放属性表
				free(ce->default_properties_table);
			}
			// 如果有静态成员表
			if (ce->default_static_members_table) {
				zval *p = ce->default_static_members_table;
				zval *end = p + ce->default_static_members_count;
				// 遍历 每个
				while (p != end) {
					// 销毁此成员
					zval_internal_ptr_dtor(p);
					// 下一个
					p++;
				}
				// 释放 静态成员表
				free(ce->default_static_members_table);
			}
			// 遍历每个属性信息
			ZEND_HASH_MAP_FOREACH_PTR(&ce->properties_info, prop_info) {
				// 如果属性信息属于此类
				if (prop_info->ce == ce) {
					// 释放属性名
					zend_string_release(prop_info->name);
					// 释放属性类型信息
					zend_type_release(prop_info->type, /* persistent */ 1);
					// 释放属性信息
					free(prop_info);
				}
			} ZEND_HASH_FOREACH_END();
			// 销毁属性表
			zend_hash_destroy(&ce->properties_info);
			// 释放类名（它是保留字）
			zend_string_release_ex(ce->name, 1);

			/* TODO: eliminate this loop for classes without functions with arg_info / attributes */
			// 遍历方法列表
			ZEND_HASH_MAP_FOREACH_PTR(&ce->function_table, fn) {
				// 如果方法属于此类（不是继承的）
				if (fn->common.scope == ce) {
					// 如果有返回类型 或 传入类型
					if (fn->common.fn_flags & (ZEND_ACC_HAS_RETURN_TYPE|ZEND_ACC_HAS_TYPE_HINTS)) {
						// 释放内部参数信息
						zend_free_internal_arg_info(&fn->internal_function);
					}
					// 如果有修饰属性
					if (fn->common.attributes) {
						// 释放修饰属性
						zend_hash_release(fn->common.attributes);
						// 清空指针
						fn->common.attributes = NULL;
					}
				}
			} ZEND_HASH_FOREACH_END();
			// 销毁方法表
			zend_hash_destroy(&ce->function_table);
			
			// 如果有常量表
			if (zend_hash_num_elements(&ce->constants_table)) {
				zend_class_constant *c;
				// 遍历 常量表
				ZEND_HASH_MAP_FOREACH_PTR(&ce->constants_table, c) {
					// 如果常量属于此类
					if (c->ce == ce) {
						// 如果常量来自表达式
						if (Z_TYPE(c->value) == IS_CONSTANT_AST) {
							// 标记成不可改变，但是当类销毁时，需要释放它
							/* We marked this as IMMUTABLE, but do need to free it when the
							 * class is destroyed. */
							ZEND_ASSERT(Z_ASTVAL(c->value)->kind == ZEND_AST_CONST_ENUM_INIT);
							// 释放常量值
							free(Z_AST(c->value));
						// 其他情况
						} else {
							// 销毁常量值
							zval_internal_ptr_dtor(&c->value);
						}
						// 如果有，释放注释
						if (c->doc_comment) {
							zend_string_release_ex(c->doc_comment, 1);
						}
						// 如果有，释放修饰属性
						if (c->attributes) {
							zend_hash_release(c->attributes);
						}
					}
					// 释放常量
					free(c);
				} ZEND_HASH_FOREACH_END();
				// 销毁常量表
				zend_hash_destroy(&ce->constants_table);
			}
			// 如果有迭代函数
			if (ce->iterator_funcs_ptr) {
				// 释放迭代函数
				free(ce->iterator_funcs_ptr);
			}
			// 如果有数组访问方法
			if (ce->arrayaccess_funcs_ptr) {
				// 释放这些方法
				free(ce->arrayaccess_funcs_ptr);
			}
			// 如果有实现接口
			if (ce->num_interfaces > 0) {
				// 释放这些接口
				free(ce->interfaces);
			}
			// 如果有属性信息表
			if (ce->properties_info_table) {
				// 释放属性信息表
				free(ce->properties_info_table);
			}
			// 如果有修饰属性表
			if (ce->attributes) {
				// 释放修饰属性表
				zend_hash_release(ce->attributes);
			}
			// 释放整个类
			free(ce);
			break;
	}
}

// ing4, 类增加引用次数
void zend_class_add_ref(zval *zv)
{
	zend_class_entry *ce = Z_PTR_P(zv);
	// 如果不是不可变的类
	if (!(ce->ce_flags & ZEND_ACC_IMMUTABLE)) {
		// 添加引用次数（zend_class_entry 没有用gc）
		ce->refcount++;
	}
}

// ing4, 销毁静态变量表
ZEND_API void zend_destroy_static_vars(zend_op_array *op_array)
{
	// 如果有静态变量哈希表
	if (ZEND_MAP_PTR(op_array->static_variables_ptr)) {
		// 获取此哈希表
		HashTable *ht = ZEND_MAP_PTR_GET(op_array->static_variables_ptr);
		// 如果哈希表有效
		if (ht) {
			// 销毁哈希表
			zend_array_destroy(ht);
			// 清空指针
			ZEND_MAP_PTR_SET(op_array->static_variables_ptr, NULL);
		}
	}
}

// 100行
// ing4， 销毁操作码组
ZEND_API void destroy_op_array(zend_op_array *op_array)
{
	uint32_t i;

	// 如果有独立缓存【ZEND_ACC_HEAP_RT_CACHE】 并且有 运行时缓存 
	if ((op_array->fn_flags & ZEND_ACC_HEAP_RT_CACHE)
	 && ZEND_MAP_PTR(op_array->run_time_cache)) {
		// 释放运行时缓存
		efree(ZEND_MAP_PTR(op_array->run_time_cache));
	}

	// 如果有函数名
	if (op_array->function_name) {
		// 释放函数名
		zend_string_release_ex(op_array->function_name, 0);
	}

	// 如果没有引用次数 或 引用-1后大于0
	if (!op_array->refcount || --(*op_array->refcount) > 0) {
		// 中断
		return;
	}

	// 释放引用计数器（整数）
	efree_size(op_array->refcount, sizeof(*(op_array->refcount)));

	// 如果有临时变量
	if (op_array->vars) {
		// 计数倒数
		i = op_array->last_var;
		// 遍历每一个
		while (i > 0) {
			// 序号
			i--;
			// 释放字串
			zend_string_release_ex(op_array->vars[i], 0);
		}
		// 释放列表
		efree(op_array->vars);
	}

	// 如果有 literals zval列表
	if (op_array->literals) {
		// 取得列表
		zval *literal = op_array->literals;
		// 结束位置
		zval *end = literal + op_array->last_literal;
		// 遍历每一个
	 	while (literal < end) {
			// 销毁 zval
			zval_ptr_dtor_nogc(literal);
			// 下一个
			literal++;
		}
		// 如果是32位地址 或 不通过pass_two 处理
		if (ZEND_USE_ABS_CONST_ADDR
		 || !(op_array->fn_flags & ZEND_ACC_DONE_PASS_TWO)) {
			// 释放 literals 列表
			efree(op_array->literals);
		}
	}
	// 释放操作码列表
	efree(op_array->opcodes);

	// 释放文件名
	zend_string_release_ex(op_array->filename, 0);
	// 如果有文档注释
	if (op_array->doc_comment) {
		// 释放文档注释
		zend_string_release_ex(op_array->doc_comment, 0);
	}
	// 如果有修饰属性 列表
	if (op_array->attributes) {
		// 释放修饰属性列表
		zend_hash_release(op_array->attributes);
	}
	// 如果有活动区域
	if (op_array->live_range) {
		// 释放活动区域
		efree(op_array->live_range);
	}
	// 如果有 try/catch 列表
	if (op_array->try_catch_array) {
		// 释放try/catch列表
		efree(op_array->try_catch_array);
	}
	// 如果扩展中有操作码组销毁器
	if (zend_extension_flags & ZEND_EXTENSIONS_HAVE_OP_ARRAY_DTOR) {
		// 如果通过 PASS_TWO 处理
		if (op_array->fn_flags & ZEND_ACC_DONE_PASS_TWO) {
			// 调用扩展的 销毁 方法，销毁操作码
			zend_llist_apply_with_argument(&zend_extensions, (llist_apply_with_arg_func_t) zend_extension_op_array_dtor_handler, op_array);
		}
	}
	// 如果有参数信息
	if (op_array->arg_info) {
		// 参数个数
		uint32_t num_args = op_array->num_args;
		// 参数信息列表开头
		zend_arg_info *arg_info = op_array->arg_info;

		// 如果有返回类型
		if (op_array->fn_flags & ZEND_ACC_HAS_RETURN_TYPE) {
			// 参数信息数量 -1
			arg_info--;
			// 参数数量 +1
			num_args++;
		}
		// 如果有字典参数
		if (op_array->fn_flags & ZEND_ACC_VARIADIC) {
			// 参数 +1
			num_args++;
		}
		// 遍历每一个参数
		for (i = 0 ; i < num_args; i++) {
			// 如果有参数名
			if (arg_info[i].name) {
				// 释放参数名
				zend_string_release_ex(arg_info[i].name, 0);
			}
			// 释放参数类型
			zend_type_release(arg_info[i].type, /* persistent */ 0);
		}
		// 释放参数信息
		efree(arg_info);
	}
	// 如果有静态变量表
	if (op_array->static_variables) {
		// 销毁静态变量表
		zend_array_destroy(op_array->static_variables);
	}
	// 如果有 动态调用定义
	if (op_array->num_dynamic_func_defs) {
		// 遍历动态调用定义
		for (i = 0; i < op_array->num_dynamic_func_defs; i++) {
			// 在副本中 闭包覆盖 静态变量列表
			// 如果原型函数（？）已经销毁了，保证销毁它们
			/* Closures overwrite static_variables in their copy.
			 * Make sure to destroy them when the prototype function is destroyed. */
			 
			// 如果有静态变量表 并且 当函数是闭包
			if (op_array->dynamic_func_defs[i]->static_variables
					&& (op_array->dynamic_func_defs[i]->fn_flags & ZEND_ACC_CLOSURE)) {
				// 销毁静态变量表
				zend_array_destroy(op_array->dynamic_func_defs[i]->static_variables);
				// 静态变量表指针 null
				op_array->dynamic_func_defs[i]->static_variables = NULL;
			}
			// 销毁这个动态调用的操作码组
			destroy_op_array(op_array->dynamic_func_defs[i]);
		}
		// 释放这个动态调用
		efree(op_array->dynamic_func_defs);
	}
}

// ing3, 更新扩展语句。p1:操作码组
static void zend_update_extended_stmts(zend_op_array *op_array)
{
	// 操作码开始行和结束行
	zend_op *opline = op_array->opcodes, *end=opline+op_array->last;
	// 遍历每一行。把这个操作码到最后
	while (opline<end) {
		// 如果操作码是 扩展语句
		if (opline->opcode == ZEND_EXT_STMT) {
			// 如果下一个不是最后一个操作码
			if (opline+1<end) {
				// 如果下一个操作码为 ZEND_EXT_STMT
				if ((opline+1)->opcode == ZEND_EXT_STMT) {
					// 此操作码为 ZEND_NOP
					opline->opcode = ZEND_NOP;
					// 下一个操作码
					opline++;
					// 继续
					continue;
				}
				// 如果没有到结尾
				if (opline+1<end) {
					// 把下一个操作码的 行号复制过来
					opline->lineno = (opline+1)->lineno;
				}
			// 如果是最后一个
			} else {
				// 操作码改成 ZEND_NOP
				opline->opcode = ZEND_NOP;
			}
		}
		// 下一个操作码
		opline++;
	}
}

// ing3, 调用扩展方法处理操作码。p1:扩展，p2:操作码组
static void zend_extension_op_array_handler(zend_extension *extension, zend_op_array *op_array)
{
	// 如果扩展有 op_array_handler 方法
	if (extension->op_array_handler) {
		// 调用方法处理操作码组
		extension->op_array_handler(op_array);
	}
}

// ing4, 验证，不可以跳进finally语句块里或 跳出finall语句块
static void zend_check_finally_breakout(zend_op_array *op_array, uint32_t op_num, uint32_t dst_num)
{
	int i;

	// 遍历所有的try/catch
	for (i = 0; i < op_array->last_try_catch; i++) {
		// 如果 操作码序号 不在finally中 （操作码在 finally 语句前 或 在finally结束后）
		// 并且 目标序号在 finally中（目标序号在 finally 语句后 和 finally结束前）
		if ((op_num < op_array->try_catch_array[i].finally_op ||
					op_num >= op_array->try_catch_array[i].finally_end)
				&& (dst_num >= op_array->try_catch_array[i].finally_op &&
					 dst_num <= op_array->try_catch_array[i].finally_end)) {
			// 未完成
			CG(in_compilation) = 1;
			// 当前操作码组 作为 活跃操作码组
			CG(active_op_array) = op_array;
			// 当前行号
			CG(zend_lineno) = op_array->opcodes[op_num].lineno;
			// 报错：不可以跳到finally 块中
			zend_error_noreturn(E_COMPILE_ERROR, "jump into a finally block is disallowed");
		// 如果 操作码序号 在finally中 （操作码在 finally 语句后 和 在finally结束前）
		// 并且 目标序号在 finally外（目标序号在 finally 语句前 或 finally结束后）
		} else if ((op_num >= op_array->try_catch_array[i].finally_op
					&& op_num <= op_array->try_catch_array[i].finally_end)
				&& (dst_num > op_array->try_catch_array[i].finally_end
					|| dst_num < op_array->try_catch_array[i].finally_op)) {
			// 未完成
			CG(in_compilation) = 1;
			// 当前操作码组 作为 活跃操作码组
			CG(active_op_array) = op_array;
			// 当前行号
			CG(zend_lineno) = op_array->opcodes[op_num].lineno;
			// 报错：不可以跳出 finally 块
			zend_error_noreturn(E_COMPILE_ERROR, "jump out of a finally block is disallowed");
		}
	}
}

// ing4, 返回 break/continue 的跳转目标。p1:操作码组，p2:操作码
static uint32_t zend_get_brk_cont_target(const zend_op_array *op_array, const zend_op *opline) {
	// 嵌套层数在op2里
	int nest_levels = opline->op2.num;
	// 偏移量在op1里
	int array_offset = opline->op1.num;
	// 跳转点
	zend_brk_cont_element *jmp_to;
	// 遍历每一层
	do {
		// 使用序号获取跳转点
		jmp_to = &CG(context).brk_cont_array[array_offset];
		// 如果嵌套次数大于1
		if (nest_levels > 1) {
			// 偏移量，上一层跳转点的序号
			array_offset = jmp_to->parent;
		}
	// 如果还有嵌套，再往出走
	} while (--nest_levels > 0);

	// 如果是 ZEND_BRK ，返回 break位置，否则 返回continue位置
	return opline->opcode == ZEND_BRK ? jmp_to->brk : jmp_to->cont;
}

// 内部调用多次
// ing3, 发布活动区域：给操作码组中最后一个活动区域区域设置 var（+类型）,start,end 3个元素。
// p1:操作码组，p2:变量编号，p3:区域类型，p4:开始位置，p5:结束位置
static void emit_live_range_raw(
		zend_op_array *op_array, uint32_t var_num, uint32_t kind, uint32_t start, uint32_t end) {
	// 
	zend_live_range *range;
	// 操作码包含区域数 +1
	op_array->last_live_range++;
	// 开辟内存创建 zend_live_range
	op_array->live_range = erealloc(op_array->live_range,
		sizeof(zend_live_range) * op_array->last_live_range);
	// 开始编号一定要小于结束编号
	ZEND_ASSERT(start < end);
	// 获得最后一个区域
	range = &op_array->live_range[op_array->last_live_range - 1];
	// 把 zval 序号 转成偏移量
	range->var = EX_NUM_TO_VAR(op_array->last_var + var_num);
	// 添加类型
	range->var |= kind;
	// 区域开始序号
	range->start = start;
	// 区域结束序号
	range->end = end;
}

// 140行
// ing3, 发布活动空间。p1:操作码组，p2:变量编号，p3:开始位置，p4:结束位置，p5:zend_needs_live_range_cb
static void emit_live_range(
		zend_op_array *op_array, uint32_t var_num, uint32_t start, uint32_t end,
		zend_needs_live_range_cb needs_live_range) {
	// 开始操作码指针
	zend_op *def_opline = &op_array->opcodes[start], *orig_def_opline = def_opline;
	// 结束操作码指针
	zend_op *use_opline = &op_array->opcodes[end];
	// 类型
	uint32_t kind;
	// 按操作码处理
	switch (def_opline->opcode) {
		// 这些不可能是第一个定义
		/* These should never be the first def. */
		case ZEND_ADD_ARRAY_ELEMENT:
		case ZEND_ADD_ARRAY_UNPACK:
		case ZEND_ROPE_ADD:
			// 不应该走到这里
			ZEND_UNREACHABLE();
			// 直接返回
			return;
			
		// 返回值是布尔型，不一定要销毁 
		/* Result is boolean, it doesn't have to be destroyed. */
		case ZEND_JMPZ_EX:
		case ZEND_JMPNZ_EX:
		case ZEND_BOOL:
		case ZEND_BOOL_NOT:
		// 类不一定要销毁 
		/* Classes don't have to be destroyed. */
		case ZEND_FETCH_CLASS:
		case ZEND_DECLARE_ANON_CLASS:
		// FAST_CALLs 不一定要销毁
		/* FAST_CALLs don't have to be destroyed. */
		case ZEND_FAST_CALL:
			// 直接返回
			return;
			
		//	开始静默
		case ZEND_BEGIN_SILENCE:
			// 类型为 静默区域
			kind = ZEND_LIVE_SILENCE;
			// 开始位置 +1
			start++;
			// 跳出
			break;
			
		// 初始化 rope
		case ZEND_ROPE_INIT:
			// rope区域
			kind = ZEND_LIVE_ROPE;
			// rope 活动区域包含 生成代码
			/* ROPE live ranges include the generating opcode. */
			
			// 找前一个操作码
			def_opline--;
			// 跳出
			break;
		
		// 重置循环？
		case ZEND_FE_RESET_R:
		case ZEND_FE_RESET_RW:
			// 循环区域
			kind = ZEND_LIVE_LOOP;
			// 开始位置 +1
			start++;
			// 跳出
			break;
			
		// 通过ZEND_NEW创建的对象，只能在DO_FCALL（调用构造方法）后被完全初始化。
		// 创建两个 live-ranges ：ZEND_LINE_NEW 用于未初始化的部分，ZEND_LIVE_TMPVAR 用于已初始化的部分
		/* Objects created via ZEND_NEW are only fully initialized 
		 * after the DO_FCALL (constructor call).
		 * We are creating two live-ranges: ZEND_LINE_NEW for uninitialized
		 * part, and ZEND_LIVE_TMPVAR for initialized.
		 */
		case ZEND_NEW:
		{
			int level = 0;
			// 原始开始位置
			uint32_t orig_start = start;
			// 遍历这一段操作码
			while (def_opline + 1 < use_opline) {
				// 找后一个操作码
				def_opline++;
				// 开始位置后移
				start++;
				// 按操作码操作
				switch (def_opline->opcode) {
					// 初始化调用，层数 +1
					case ZEND_INIT_FCALL:
					case ZEND_INIT_FCALL_BY_NAME:
					case ZEND_INIT_NS_FCALL_BY_NAME:
					case ZEND_INIT_DYNAMIC_CALL:
					case ZEND_INIT_USER_CALL:
					case ZEND_INIT_METHOD_CALL:
					case ZEND_INIT_STATIC_METHOD_CALL:
					case ZEND_NEW:
						// 层数 +1
						level++;
						// 跳出 switch
						break;
					// 执行调用，层数-1
					case ZEND_DO_FCALL:
					case ZEND_DO_FCALL_BY_NAME:
					case ZEND_DO_ICALL:
					case ZEND_DO_UCALL:
						// 最外层不用减
						if (level == 0) {
							// 完成
							goto done;
						}
						// 层数 -1
						level--;
						// 跳出 switch
						break;
				}
			}
// 完成
done:
			// 发布活动区域：给操作码组中最后一个活动区域区域设置 var（+类型）,start,end 3个元素。
			// p1:操作码组，p2:变量编号，p3:区域类型，p4:开始位置，p5:结束位置
			emit_live_range_raw(op_array, var_num, ZEND_LIVE_NEW, orig_start + 1, start + 1);
			// 如果已经到了最后
			if (start + 1 == end) {
				// 零碎的 live-range 可以不储存
				/* Trivial live-range, no need to store it. */
				return;
			}
		}
		// windows ((void)0)
		ZEND_FALLTHROUGH;
		
		// 默认情况
		default:
			// 开始位置 +1
			start++;
			// 类型为 临时变量 活动区域
			kind = ZEND_LIVE_TMPVAR;

			// 检查钩子来决定 活动区域是否有必要。例如：基于 type_info
			/* Check hook to determine whether a live range is necessary,
			 * e.g. based on type info. */
			// 如果不需要创建区域
			if (needs_live_range && !needs_live_range(op_array, orig_def_opline)) {
				// 直接返回
				return;
			}
			// 跳出
			break;
			
		// 复制临时变量
		case ZEND_COPY_TMP:
		{
			// COPY_TMP 有一个独立的活动区域：第一个从定义到在 "null" 分支中的使用
			// 第二个从"non-null"分支的开始到 FREE 操作码
			/* COPY_TMP has a split live-range: One from the definition until the use in
			 * "null" branch, and another from the start of the "non-null" branch to the
			 * FREE opcode. */
			// 取得序号
			uint32_t rt_var_num = EX_NUM_TO_VAR(op_array->last_var + var_num);
			// 如果不需要创建区域 或 创建活动区域失败
			if (needs_live_range && !needs_live_range(op_array, orig_def_opline)) {
				// 直接返回
				return;
			}
			// 类型为 临时变量 活动区域
			kind = ZEND_LIVE_TMPVAR;
			// 如果操作码不是free
			if (use_opline->opcode != ZEND_FREE) {
				// 如果一个合并分配被优化掉了，这个可能发生
				// 在这种情况，需要发布一个普通活动区域来替代它
				/* This can happen if one branch of the coalesce has been optimized away.
				 * In this case we should emit a normal live-range instead. */
				// 开始位置右移
				start++;
				// 跳出
				break;
			}
			
			// 块开始 操作对象
			zend_op *block_start_op = use_opline;
			// 只要前一个操作码是 ZEND_FREE
			while ((block_start_op-1)->opcode == ZEND_FREE) {
				// 一直向前找
				block_start_op--;
			}

			// 块开始位置的偏移量
			start = block_start_op - op_array->opcodes;
			// 如果，开始和结束中间有内容
			if (start != end) {
				// 发布活动区域：给操作码组中最后一个活动区域区域设置 var（+类型）,start,end 3个元素。
				// p1:操作码组，p2:变量编号，p3:区域类型，p4:开始位置，p5:结束位置
				emit_live_range_raw(op_array, var_num, kind, start, end);
			}
			// 
			do {
				// 操作码指针-1
				use_opline--;

				// 此处可以优化，这种情况会碰到 def
				/* The use might have been optimized away, in which case we will hit the def
				 * instead. */
				
				// 如果操作码是 ZEND_COPY_TMP 并且 结果变量序号 等于 rt_var_num
				if (use_opline->opcode == ZEND_COPY_TMP && use_opline->result.var == rt_var_num) {
					// def 开始位置
					start = def_opline + 1 - op_array->opcodes;
					// 发布活动区域：给操作码组中最后一个活动区域区域设置 var（+类型）,start,end 3个元素。
					// p1:操作码组，p2:变量编号，p3:区域类型，p4:开始位置，p5:结束位置
					emit_live_range_raw(op_array, var_num, kind, start, end);
					// 返回
					return;
				}
			// 继续条件：！（ 第一个操作对象是变量或临时变量 并且第一个操作对象变量序号是 rt_var_num 或
			// 第二个操作对象是变量或临时变量 并且第二个操作对象变量序号是 rt_var_num ）
			// 直到找到 rt_var_num 变量 为止
			} while (!(
				((use_opline->op1_type & (IS_TMP_VAR|IS_VAR)) && use_opline->op1.var == rt_var_num) ||
				((use_opline->op2_type & (IS_TMP_VAR|IS_VAR)) && use_opline->op2.var == rt_var_num)
			));
			// def 开始位置
			start = def_opline + 1 - op_array->opcodes;
			// 结束位置： 当前操作码位置
			end = use_opline - op_array->opcodes;
			// 发布活动区域：给操作码组中最后一个活动区域区域设置 var（+类型）,start,end 3个元素。
			// p1:操作码组，p2:变量编号，p3:区域类型，p4:开始位置，p5:结束位置
			emit_live_range_raw(op_array, var_num, kind, start, end);
			// 中断
			return;
		}
	}
	// 发布活动区域：给操作码组中最后一个活动区域区域设置 var（+类型）,start,end 3个元素。
	// p1:操作码组，p2:变量编号，p3:区域类型，p4:开始位置，p5:结束位置
	emit_live_range_raw(op_array, var_num, kind, start, end);
}

// ing3, 检验 ZEND_ROPE_ADD 或 ZEND_ADD_ARRAY_ELEMENT 或 ZEND_ADD_ARRAY_UNPACK
static bool is_fake_def(zend_op *opline) {
	/* These opcodes only modify the result, not create it. */
	return opline->opcode == ZEND_ROPE_ADD
		|| opline->opcode == ZEND_ADD_ARRAY_ELEMENT
		|| opline->opcode == ZEND_ADD_ARRAY_UNPACK;
}

// ing3, 类型转换，march, 列表读取，copy ,这几种情况返回1
static bool keeps_op1_alive(zend_op *opline) {
	// 这些操作码 不消耗它们的 op1操作对象，稍后会被其他操作销毁
	/* These opcodes don't consume their OP1 operand,
	 * it is later freed by something else. */
	// 类型转换，march, 列表读取，copy
	if (opline->opcode == ZEND_CASE
	 || opline->opcode == ZEND_CASE_STRICT
	 || opline->opcode == ZEND_SWITCH_LONG
	 || opline->opcode == ZEND_SWITCH_STRING
	 || opline->opcode == ZEND_MATCH
	 || opline->opcode == ZEND_FETCH_LIST_R
	 || opline->opcode == ZEND_COPY_TMP) {
		return 1;
	}
	ZEND_ASSERT(opline->opcode != ZEND_FE_FETCH_R
		&& opline->opcode != ZEND_FE_FETCH_RW
		&& opline->opcode != ZEND_FETCH_LIST_W
		&& opline->opcode != ZEND_VERIFY_RETURN_TYPE
		&& opline->opcode != ZEND_BIND_LEXICAL
		&& opline->opcode != ZEND_ROPE_ADD);
	return 0;
}

// 活跃区域 必须按增量的 start 排序
/* Live ranges must be sorted by increasing start opline */
// ing4, 比较两个 zend_live_range 的大小
static int cmp_live_range(const zend_live_range *a, const zend_live_range *b) {
	return a->start - b->start;
}

// ing4, 交换两个 zend_live_range
static void swap_live_range(zend_live_range *a, zend_live_range *b) {
	// 临时变量
	uint32_t tmp;
	// 先交换 ->var
	tmp = a->var;
	a->var = b->var;
	b->var = tmp;
	// 先交换 ->start
	tmp = a->start;
	a->start = b->start;
	b->start = tmp;
	// 先交换 ->end
	tmp = a->end;
	a->end = b->end;
	b->end = tmp;
}

// 100行，这个是关键
// ing3, 计算生成 活动区域。p1:操作码组，p2:zend_needs_live_range_cb
static void zend_calc_live_ranges(
		zend_op_array *op_array, f needs_live_range) {
	// 操作码数量
	uint32_t opnum = op_array->last;
	// 最后一个操作码
	zend_op *opline = &op_array->opcodes[opnum];
	// 
	ALLOCA_FLAG(use_heap)
	// 变量偏移量
	uint32_t var_offset = op_array->last_var;
	// 分配一串32位整数，和临时变量个数相同
	uint32_t *last_use = do_alloca(sizeof(uint32_t) * op_array->T, use_heap);
	// 把这串整数全写成-1（所有位都是1）
	memset(last_use, -1, sizeof(uint32_t) * op_array->T);
	// 必须没有 op_array->live_range 
	ZEND_ASSERT(!op_array->live_range);
	// 倒着遍历
	while (opnum > 0) {
		// 前一个操作码序号
		opnum--;
		// 前一个操作码
		opline--;
		// 如果结果类型是 IS_TMP_VAR|IS_VA。
		// 并且操作码不是 （ZEND_ROPE_ADD 或 ZEND_ADD_ARRAY_ELEMENT 或 ZEND_ADD_ARRAY_UNPACK）
		if ((opline->result_type & (IS_TMP_VAR|IS_VAR)) && !is_fake_def(opline)) {
			// 
			uint32_t var_num = EX_VAR_TO_NUM(opline->result.var) - var_offset;
			// 可能由于两点原因存在没使用的定义：结果是真的未使用（例如对一个未使用的bool型结果使用 FREE 操作码）
			// 或者因为多交定义操作码（例如 JMPZ_EX 和 QM_ASSIGN），最后这种情况会开始live range.
			// 所以这里可以简单地忽略错误使用
			/* Defs without uses can occur for two reasons: Either because the result is
			 * genuinely unused (e.g. omitted FREE opcode for an unused boolean result), or
			 * because there are multiple defining opcodes (e.g. JMPZ_EX and QM_ASSIGN), in
			 * which case the last one starts the live range. As such, we can simply ignore
			 * missing uses here. */
			if (EXPECTED(last_use[var_num] != (uint32_t) -1)) {
				/* Skip trivial live-range */
				if (opnum + 1 != last_use[var_num]) {
					// 临时变量
					uint32_t num;
// 只走这里
#if 1
					// 断言：OP_DATA 操作码 只使用了 op1
					/* OP_DATA uses only op1 operand */
					// 当前操作码不可以是 ZEND_OP_DATA
					ZEND_ASSERT(opline->opcode != ZEND_OP_DATA);
					// 暂存编号
					num = opnum;
// 不走这里
#else
					/* OP_DATA is really part of the previous opcode. */
					num = opnum - (opline->opcode == ZEND_OP_DATA);
#endif
					// 发布活动空间。p1:操作码组，p2:变量编号，p3:开始位置，p4:结束位置，p5:zend_needs_live_range_cb
					emit_live_range(op_array, var_num, num, last_use[var_num], needs_live_range);
				}
				// 最后使用编号 为 -1
				last_use[var_num] = (uint32_t) -1;
			}
		}

		// 如果第一个操作对象类型是 变量或临时变量
		if ((opline->op1_type & (IS_TMP_VAR|IS_VAR))) {
			// 取得这个变量的编号
			uint32_t var_num = EX_VAR_TO_NUM(opline->op1.var) - var_offset;
			// 如果最后使用是 -1
			if (EXPECTED(last_use[var_num] == (uint32_t) -1)) {
				// 类型转换，march, 列表读取，copy ,这几种情况返回1。 如果失败
				if (EXPECTED(!keeps_op1_alive(opline))) {
					// OP_DATA 已经是前面的操作码的一部分
					/* OP_DATA is really part of the previous opcode. */
					// 最后使用编号，如果是 ZEND_OP_DATA 操作码要 -1
					last_use[var_num] = opnum - (opline->opcode == ZEND_OP_DATA);
				}
			}
		}
		// 第二个运算对象类型是 临时变量 或 变量
		if (opline->op2_type & (IS_TMP_VAR|IS_VAR)) {
			// 计算变量序号
			uint32_t var_num = EX_VAR_TO_NUM(opline->op2.var) - var_offset;
			// 如果，操作码是 ZEND_FE_FETCH_R 或 ZEND_FE_FETCH_RW
			if (UNEXPECTED(opline->opcode == ZEND_FE_FETCH_R
					|| opline->opcode == ZEND_FE_FETCH_RW)) {
				// FE_FETCH 的 op2 必须一个定义，而不是一个使用
				/* OP2 of FE_FETCH is actually a def, not a use. */
				// 如果此位置值 不是 -1
				if (last_use[var_num] != (uint32_t) -1) {
					// 如果此位置值不是 操作码序号+1
					if (opnum + 1 != last_use[var_num]) {
						// 发布活动空间。p1:操作码组，p2:变量编号，p3:开始位置，p4:结束位置，p5:zend_needs_live_range_cb
						emit_live_range(
							op_array, var_num, opnum, last_use[var_num], needs_live_range);
					}
					// 最后使用编号为 -1
					last_use[var_num] = (uint32_t) -1;
				}
			// 如果此位置值为 -1
			} else if (EXPECTED(last_use[var_num] == (uint32_t) -1)) {
// 只走这里
#if 1
				// 只在op1 使用 OP_DATA，op2不可以有这个
				/* OP_DATA uses only op1 operand */
				ZEND_ASSERT(opline->opcode != ZEND_OP_DATA);
				// 记录变量编号对象的操作码编号
				last_use[var_num] = opnum;
// 不走这里
#else
				/* OP_DATA is really part of the previous opcode. */
				last_use[var_num] = opnum - (opline->opcode == ZEND_OP_DATA);
#endif
			}
		}
	}

	// 如果活动区域大于1个
	if (op_array->last_live_range > 1) {
		// 第一个活动区域
		zend_live_range *r1 = op_array->live_range;
		// 最后一个活动区域
		zend_live_range *r2 = r1 + op_array->last_live_range - 1;

		// 大部分情况下，只要翻转数组就可以了
		/* In most cases we need just revert the array */
		// 从两头到中间，两两比较
		while (r1 < r2) {
			// 把指针小的放在后面
			swap_live_range(r1, r2);
			// 两头向中间移一个
			// 左面的向右移
			r1++;
			// 右向的向左移
			r2--;
		}
		// 区域列表开头位置
		r1 = op_array->live_range;
		// 区域列表结尾位置
		r2 = r1 + op_array->last_live_range - 1;
		// 遍历这段操作码
		while (r1 < r2) {
			// 如果前面的区域操作码放在了后面
			if (r1->start > (r1+1)->start) {
				// 给所有区域排序（快排加插入）
				zend_sort(r1, r2 - r1 + 1, sizeof(zend_live_range),
					(compare_func_t) cmp_live_range, (swap_func_t) swap_live_range);
				// 排完跳出
				break;
			}
			// 检查下一个区域
			r1++;
		}
	}
	// 释放 last_use
	free_alloca(last_use, use_heap);
}

// ing3, 重新分配 活动区域 p1:操作码组，p2:zend_needs_live_range_cb
ZEND_API void zend_recalc_live_ranges(
		zend_op_array *op_array, zend_needs_live_range_cb needs_live_range) {
	// 假定：不在什么都没有的时候创建live-ranges，（所以 ->live_range 一定存在 ）
	/* We assume that we never create live-ranges where there were none before. */
	// 必须要有 ->live_range
	ZEND_ASSERT(op_array->live_range);
	// 释放 ->live_range
	efree(op_array->live_range);
	// 清空指针
	op_array->live_range = NULL;
	// 最后一个的编号为0
	op_array->last_live_range = 0;
	// 计算生成 活动区域。p1:操作码组，p2:zend_needs_live_range_cb
	zend_calc_live_ranges(op_array, needs_live_range);
}

// 160 行
// ing3, 把操作码中 把跳转目标从编译时转到运行时。还有一些其他操作
ZEND_API void pass_two(zend_op_array *op_array)
{
	zend_op *opline, *end;
	// 如果没有类型，直接返回
	if (!ZEND_USER_CODE(op_array->type)) {
		return;
	}
	// 如果编译配置允许 编译扩展语句
	if (CG(compiler_options) & ZEND_COMPILE_EXTENDED_STMT) {
		// 更新操作码扩展语句
		zend_update_extended_stmts(op_array);
	}
	// 如果如果编译配置允许 【调用扩展（extensions）的操作码处理器】
	if (CG(compiler_options) & ZEND_COMPILE_HANDLE_OP_ARRAY) {
		// 如果有扩展的操作码处理器
		if (zend_extension_flags & ZEND_EXTENSIONS_HAVE_OP_ARRAY_HANDLER) {
			// 调用扩展的 zend_extension_op_array_handler 方法
			zend_llist_apply_with_argument(&zend_extensions, (llist_apply_with_arg_func_t) zend_extension_op_array_handler, op_array);
		}
	}
	// 如果当前上下文中变量数量 不等于 操作码中编译变量的数量
	if (CG(context).vars_size != op_array->last_var) {
		// 分配内存创建string列表
		op_array->vars = (zend_string**) erealloc(op_array->vars, sizeof(zend_string*)*op_array->last_var);
		// 更新 编译变量的数量
		CG(context).vars_size = op_array->last_var;
	}

// 32位系统
#if ZEND_USE_ABS_CONST_ADDR
	// 
	if (CG(context).opcodes_size != op_array->last) {
		op_array->opcodes = (zend_op *) erealloc(op_array->opcodes, sizeof(zend_op)*op_array->last);
		CG(context).opcodes_size = op_array->last;
	}
	if (CG(context).literals_size != op_array->last_literal) {
		op_array->literals = (zval*)erealloc(op_array->literals, sizeof(zval) * op_array->last_literal);
		CG(context).literals_size = op_array->last_literal;
	}
// 64位操作系统
#else
	// 分配内存创建操作码列表
	op_array->opcodes = (zend_op *) erealloc(op_array->opcodes,
		ZEND_MM_ALIGNED_SIZE_EX(sizeof(zend_op) * op_array->last, 16) +
		sizeof(zval) * op_array->last_literal);
	// 如果有 literals
	if (op_array->literals) {
		// 复制 literals
		memcpy(((char*)op_array->opcodes) + ZEND_MM_ALIGNED_SIZE_EX(sizeof(zend_op) * op_array->last, 16),
			op_array->literals, sizeof(zval) * op_array->last_literal);
		// 释放原来的 literals 
		efree(op_array->literals);
		// 使用新 literals 列表
		op_array->literals = (zval*)(((char*)op_array->opcodes) + ZEND_MM_ALIGNED_SIZE_EX(sizeof(zend_op) * op_array->last, 16));
	}
	// 更新上下文中的操作码数量
	CG(context).opcodes_size = op_array->last;
	// 更新上下文中的literals 数量
	CG(context).literals_size = op_array->last_literal;
#endif

	// 如果开启了观察者，保存最后一个临时变量给观察者
    op_array->T += ZEND_OBSERVER_ENABLED; // reserve last temporary for observers if enabled

	// 需要在 opcode/literal 被重新分配以后直接设置，保证 当后继发生错误时，能够正确销毁
	/* Needs to be set directly after the opcode/literal reallocation, to ensure destruction
	 * happens correctly if any of the following fixups generate a fatal error. */
	op_array->fn_flags |= ZEND_ACC_DONE_PASS_TWO;

	// 操作码列表的开头
	opline = op_array->opcodes;
	// 操作码列表的结性
	end = opline + op_array->last;
	// 遍历每个操作码
	while (opline < end) {
		// 根据操作码操作
		switch (opline->opcode) {
			// 有默认值的参数
			case ZEND_RECV_INIT:
				{
					// 获取当前操作码列表中 第 (p1).constant 个 literal
					zval *val = CT_CONSTANT(opline->op2);
					// 如果类型是 表达式语句
					if (Z_TYPE_P(val) == IS_CONSTANT_AST) {
						// 缓存位置， 对齐到8
						uint32_t slot = ZEND_MM_ALIGNED_SIZE_EX(op_array->cache_size, 8);
						// 更新缓存位置
						// 获取 (zval).u2.cache_slot
						Z_CACHE_SLOT_P(val) = slot;
						// 缓存 大小增加，一个zval的大小
						op_array->cache_size += sizeof(zval);
					}
				}
				// 跳出 switch
				break;
			// finally 语句
			case ZEND_FAST_CALL:
				// finally_op 开始位置
				opline->op1.opline_num = op_array->try_catch_array[opline->op1.num].finally_op;
				// 把跳转目标从编译时转到运行时 。 (p3).jmp_addr = (p1)->opcodes + (p3).opline_num;
				ZEND_PASS_TWO_UPDATE_JMP_TARGET(op_array, opline, opline->op1);
				// 跳出 switch
				break;
			// break/continue
			case ZEND_BRK:
			case ZEND_CONT:
				{
					// 返回 break/continue 的跳转目标。p1:操作码组，p2:操作码
					uint32_t jmp_target = zend_get_brk_cont_target(op_array, opline);

					// 如果有 finally 块
					if (op_array->fn_flags & ZEND_ACC_HAS_FINALLY_BLOCK) {
						// 验证，不可以跳进finally语句块里或 跳出finall语句块
						zend_check_finally_breakout(op_array, opline - op_array->opcodes, jmp_target);
					}
					// 操作码 ZEND_JMP
					opline->opcode = ZEND_JMP;
					// 跳转目标操作码编号
					opline->op1.opline_num = jmp_target;
					// p2里存的序号是0
					opline->op2.num = 0;
					// 把跳转目标从编译时转到运行时 。 (p3).jmp_addr = (p1)->opcodes + (p3).opline_num;
					ZEND_PASS_TWO_UPDATE_JMP_TARGET(op_array, opline, opline->op1);
				}
				// 跳出 switch
				break;
			// goto 跳转
			case ZEND_GOTO:
				// 完成 goto 跳转到标签的编译
				zend_resolve_goto_label(op_array, opline);
				// 如果有 finally 块
				if (op_array->fn_flags & ZEND_ACC_HAS_FINALLY_BLOCK) {
					// 验证，不可以跳进finally语句块里或 跳出finall语句块
					zend_check_finally_breakout(op_array, opline - op_array->opcodes, opline->op1.opline_num);
				}
				// 
				ZEND_FALLTHROUGH;
				// 还要往下走
				
			// 跳转操作码
			case ZEND_JMP:
				// 把跳转目标从编译时转到运行时 。 (p3).jmp_addr = (p1)->opcodes + (p3).opline_num;
				ZEND_PASS_TWO_UPDATE_JMP_TARGET(op_array, opline, opline->op1);
				// 跳出 switch
				break;
			// 这一堆要仔细研究一下
			case ZEND_JMPZ:
			case ZEND_JMPNZ:
			case ZEND_JMPZ_EX:
			case ZEND_JMPNZ_EX:
			case ZEND_JMP_SET:
			case ZEND_COALESCE:
			case ZEND_FE_RESET_R:
			case ZEND_FE_RESET_RW:
			case ZEND_JMP_NULL:
				// 把跳转目标从编译时转到运行时 。 (p3).jmp_addr = (p1)->opcodes + (p3).opline_num;
				ZEND_PASS_TWO_UPDATE_JMP_TARGET(op_array, opline, opline->op2);
				// 跳出 switch
				break;
			// 检验断言
			case ZEND_ASSERT_CHECK:
			{
				// 如果断言结果是未使用 检验结果也是未使用
				/* If result of assert is unused, result of check is unused as well */
				// 取出这个操作码
				zend_op *call = &op_array->opcodes[opline->op2.opline_num - 1];
				// 如果 是 结束调用代码块
				if (call->opcode == ZEND_EXT_FCALL_END) {
					// 操作码指针-1
					call--;
				}
				// 如果操作码结果类型是未使用
				if (call->result_type == IS_UNUSED) {
					// 当前操作码 类型也标记未使用
					opline->result_type = IS_UNUSED;
				}
				// 把跳转目标从编译时转到运行时 。 (p3).jmp_addr = (p1)->opcodes + (p3).opline_num;
				ZEND_PASS_TWO_UPDATE_JMP_TARGET(op_array, opline, opline->op2);
				// 跳出 switch
				break;
			}
			// foreach 微循
			case ZEND_FE_FETCH_R:
			case ZEND_FE_FETCH_RW:
				// 绝对索引号 转成 相对偏移量
				/* absolute index to relative offset */
				// 更新扩展值
				// 计算 (p1)->opcodes[p3] 到p2 间的字节数
				opline->extended_value = ZEND_OPLINE_NUM_TO_OFFSET(op_array, opline, opline->extended_value);
				// 跳出 switch
				break;
			// catch
			case ZEND_CATCH:
				// 如果扩展值里没有 ZEND_LAST_CATCH 
				if (!(opline->extended_value & ZEND_LAST_CATCH)) {
					// 把跳转目标从编译时转到运行时 。 (p3).jmp_addr = (p1)->opcodes + (p3).opline_num;
					ZEND_PASS_TWO_UPDATE_JMP_TARGET(op_array, opline, opline->op2);
				}
				// 跳出 switch
				break;
			// 返回 和 引用返回
			case ZEND_RETURN:
			case ZEND_RETURN_BY_REF:
				// 如果在 访问生成器
				if (op_array->fn_flags & ZEND_ACC_GENERATOR) {
					// 操作码更新成 ZEND_GENERATOR_RETURN
					opline->opcode = ZEND_GENERATOR_RETURN;
				}
				// 跳出 switch
				break;
			// switch 和 match 语句
			case ZEND_SWITCH_LONG:
			case ZEND_SWITCH_STRING:
			case ZEND_MATCH:
			{
				// 绝对索引号 转成 相对偏移量
				/* absolute indexes to relative offsets */
				// 取出操作对象2中的跳转表
				// 获取当前操作码列表中 第 (p1).constant 个 literal
				HashTable *jumptable = Z_ARRVAL_P(CT_CONSTANT(opline->op2));
				// 临时变量
				zval *zv;
				// 遍历跳转表
				ZEND_HASH_FOREACH_VAL(jumptable, zv) {
					// 计算 (p1)->opcodes[p3] 到p2 间的字节数
					Z_LVAL_P(zv) = ZEND_OPLINE_NUM_TO_OFFSET(op_array, opline, Z_LVAL_P(zv));
				} ZEND_HASH_FOREACH_END();
				
				// 计算 (p1)->opcodes[p3] 到p2 间的字节数
				opline->extended_value = ZEND_OPLINE_NUM_TO_OFFSET(op_array, opline, opline->extended_value);
				// 跳出 switch
				break;
			}
		}
		// 如果 操作对象1 类型是常量
		if (opline->op1_type == IS_CONST) {
			// 把常量从编译时转到运行时: (p3).zv = p1->literals + p3.constant
			ZEND_PASS_TWO_UPDATE_CONSTANT(op_array, opline, opline->op1);
		// 如果 操作对象1 类型是 变量或临时变量
		} else if (opline->op1_type & (IS_VAR|IS_TMP_VAR)) {
			// 把 zval 序号 转成偏移量 （加上 ZEND_CALL_FRAME_SLOT）
			opline->op1.var = EX_NUM_TO_VAR(op_array->last_var + opline->op1.var);
		}
		// 如果 操作对象2 类型是常量
		if (opline->op2_type == IS_CONST) {
			// 把常量从编译时转到运行时: (p3).zv = p1->literals + p3.constant
			ZEND_PASS_TWO_UPDATE_CONSTANT(op_array, opline, opline->op2);
		// 如果 操作对象2 类型是 变量或临时变量
		} else if (opline->op2_type & (IS_VAR|IS_TMP_VAR)) {
			// 把 zval 序号 转成偏移量 （加上 ZEND_CALL_FRAME_SLOT）
			opline->op2.var = EX_NUM_TO_VAR(op_array->last_var + opline->op2.var);
		}
		// 如果 结果 类型是 变量或临时变量
		if (opline->result_type & (IS_VAR|IS_TMP_VAR)) {
			// 把 zval 序号 转成偏移量 （加上 ZEND_CALL_FRAME_SLOT）
			opline->result.var = EX_NUM_TO_VAR(op_array->last_var + opline->result.var);
		}
		// 给操作码链接它对应的处理器 zend_vm_set_opcode_handler : zend_vm_execute.h
		ZEND_VM_SET_OPCODE_HANDLER(opline);
		// 下一个操作码
		opline++;
	}

	// 计算生成 活动区域。p1:操作码组，p2:zend_needs_live_range_cb
	zend_calc_live_ranges(op_array, NULL);
	// 
	return;
}

// ing4, 一元操作符 ~(二进制位取补) 和 !
ZEND_API unary_op_type get_unary_op(int opcode)
{
	switch (opcode) {
		case ZEND_BW_NOT:
			return (unary_op_type) bitwise_not_function;
		case ZEND_BOOL_NOT:
			return (unary_op_type) boolean_not_function;
		default:
			return (unary_op_type) NULL;
	}
}

// ing4, 返回 二元操作符 对应的函数
ZEND_API binary_op_type get_binary_op(int opcode)
{
	switch (opcode) {
		// +
		case ZEND_ADD:
			return (binary_op_type) add_function;
		// -
		case ZEND_SUB:
			return (binary_op_type) sub_function;
		// *
		case ZEND_MUL:
			return (binary_op_type) mul_function;
		// 乘方
		case ZEND_POW:
			return (binary_op_type) pow_function;
		// 除法
		case ZEND_DIV:
			return (binary_op_type) div_function;
		// 区域
		case ZEND_MOD:
			return (binary_op_type) mod_function;
		// 左移
		case ZEND_SL:
			return (binary_op_type) shift_left_function;
		// 右移
		case ZEND_SR:
			return (binary_op_type) shift_right_function;
		// 连接，两边都是内置变量时 ZEND_FAST_CONCAT
		case ZEND_FAST_CONCAT:
		case ZEND_CONCAT:
			return (binary_op_type) concat_function;
		// === 或 match 中的 arm 严格匹配
		case ZEND_IS_IDENTICAL:
		case ZEND_CASE_STRICT:
			return (binary_op_type) is_identical_function;
		//
		case ZEND_IS_NOT_IDENTICAL:
			return (binary_op_type) is_not_identical_function;
		// == ， 或 match 中的 arm
		case ZEND_IS_EQUAL:
		case ZEND_CASE:
			return (binary_op_type) is_equal_function;
		// !=
		case ZEND_IS_NOT_EQUAL:
			return (binary_op_type) is_not_equal_function;
		// <
		case ZEND_IS_SMALLER:
			return (binary_op_type) is_smaller_function;
		// <=
		case ZEND_IS_SMALLER_OR_EQUAL:
			return (binary_op_type) is_smaller_or_equal_function;
		case ZEND_SPACESHIP:
			return (binary_op_type) compare_function;
		// | 这几个是位运算符
		case ZEND_BW_OR:
			return (binary_op_type) bitwise_or_function;
		// &
		case ZEND_BW_AND:
			return (binary_op_type) bitwise_and_function;
		// ^
		case ZEND_BW_XOR:
			return (binary_op_type) bitwise_xor_function;
		// xor 这个是逻辑运算符
		case ZEND_BOOL_XOR:
			return (binary_op_type) boolean_xor_function;
		default:
			ZEND_UNREACHABLE();
			return (binary_op_type) NULL;
	}
}
