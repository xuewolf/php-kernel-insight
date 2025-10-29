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

#ifndef ZEND_OBJECTS_API_H
#define ZEND_OBJECTS_API_H

#include "zend.h"
#include "zend_compile.h"

// 无效对象标记
#define OBJ_BUCKET_INVALID			(1<<0)

// ing4, 对象是否有效（指针不包含无效标记）
#define IS_OBJ_VALID(o)				(!(((zend_uintptr_t)(o)) & OBJ_BUCKET_INVALID))

// ing4, 给对象指针添加无效标记：转成int，最后一位标记成1 再转回成 指针
#define SET_OBJ_INVALID(o)			((zend_object*)((((zend_uintptr_t)(o)) | OBJ_BUCKET_INVALID)))

// 全局只有 zend_objects_API.c : zend_objects_store_put 用到
// ing4, 把指针（用序号创建的假指针）转成 对象指针在对象容器中的序号
#define GET_OBJ_BUCKET_NUMBER(o)	(((zend_intptr_t)(o)) >> 1)

// ing4, 把要回收的序号n左移1位, 最后一位写成1。然后转成 zend_object 指针，写到 o 里面。
#define SET_OBJ_BUCKET_NUMBER(o, n)	do { \
		(o) = (zend_object*)((((zend_uintptr_t)(n)) << 1) | OBJ_BUCKET_INVALID); \
	} while (0)

// ing4, 回收一个 objects_store 里面的 元素指针
// 传入的 h( handle ) 是对象在全局对象容器 EG(objects_store).object_buckets 中的【索引号】，通过它可以直接在全局容器中获取指针
#define ZEND_OBJECTS_STORE_ADD_TO_FREE_LIST(h) do { \
		/* h是释放的指针元素索引号，让它指向原本的第一个空元素，让空元素形成一个串 */ \
		SET_OBJ_BUCKET_NUMBER(EG(objects_store).object_buckets[(h)], EG(objects_store).free_list_head); \
		/* 把新释放的元素序号放在最前面 */ \
		EG(objects_store).free_list_head = (h); \
	} while (0)

// ing4, 释放对象
#define OBJ_RELEASE(obj) zend_object_release(obj)

// 这个东西全局有一个，在 _zend_executor_globals 里面
typedef struct _zend_objects_store {
	zend_object **object_buckets;
	uint32_t top;
	uint32_t size;
	int free_list_head;
} zend_objects_store;

// 全局容器处理方法
/* Global store handling functions */
BEGIN_EXTERN_C()
ZEND_API void ZEND_FASTCALL zend_objects_store_init(zend_objects_store *objects, uint32_t init_size);
ZEND_API void ZEND_FASTCALL zend_objects_store_call_destructors(zend_objects_store *objects);
ZEND_API void ZEND_FASTCALL zend_objects_store_mark_destructed(zend_objects_store *objects);
ZEND_API void ZEND_FASTCALL zend_objects_store_free_object_storage(zend_objects_store *objects, bool fast_shutdown);
ZEND_API void ZEND_FASTCALL zend_objects_store_destroy(zend_objects_store *objects);

/* Store API functions */
// 全局只有 _zend_object_std_init 里调用，一般情况下，所有对象都进容器
ZEND_API void ZEND_FASTCALL zend_objects_store_put(zend_object *object);
// 这个主要是虚拟机执行的时候调用，调用的地方比较多。
ZEND_API void ZEND_FASTCALL zend_objects_store_del(zend_object *object);

// 当构造方法异常终止时调用
/* Called when the ctor was terminated by an exception */
// ing4, 给对象添加【已销毁】标记
static zend_always_inline void zend_object_store_ctor_failed(zend_object *obj)
{
	// 给对象添加【已销毁】标记
	GC_ADD_FLAGS(obj, IS_OBJ_DESTRUCTOR_CALLED);
}

END_EXTERN_C()

// ing3, 释放对象
static zend_always_inline void zend_object_release(zend_object *obj)
{
	// -1后引用次数为0
	if (GC_DELREF(obj) == 0) {
		// 调用对象销毁的方法，销毁并释放对象
		zend_objects_store_del(obj);
	// 还有引用，并且 可能泄露（是否带有 【不可回收】标记）
	} else if (UNEXPECTED(GC_MAY_LEAK((zend_refcounted*)obj))) {
		// 注册自动垃圾回收
		gc_possible_root((zend_refcounted*)obj);
	}
}

// ing3, 取得属性变量大小（数量*zval大小），没有魔术方法的类，数量 -1
static zend_always_inline size_t zend_object_properties_size(zend_class_entry *ce)
{
	// ZEND_ACC_USE_GUARDS 有魔术方法的类，带这个标记
	// 没有魔术方法的类，数量 -1
	return sizeof(zval) *
		(ce->default_properties_count -
			((ce->ce_flags & ZEND_ACC_USE_GUARDS) ? 0 : 1));
}
// 分配对象类型并填充成0，但不是标准的 对象 和 属性
// 标准对象必须用 zend_object_std_init 初始化
// 属性必须 使用 object_properties_init 初始化
/* Allocates object type and zeros it, but not the standard zend_object and properties.
 * Standard object MUST be initialized using zend_object_std_init().
 * Properties MUST be initialized using object_properties_init(). */
 
// ing4, 分配内存创建非标准对象。扩展中大量使用，核心中只有zend_weakrefs.c用到。其实这是个特别有用的方法。
static zend_always_inline void *zend_object_alloc(size_t obj_size, zend_class_entry *ce) {
	// 对象大小为，指定的大小 + 成员属性的大小
	void *obj = emalloc(obj_size + zend_object_properties_size(ce));
	// 全写成0
	memset(obj, 0, obj_size - sizeof(zend_object));
	// 返回指针
	return obj;
}

// ing4, 通过slot（它也是属性）指针计算slot的序号，返回此序号对应的属性信息
static inline zend_property_info *zend_get_property_info_for_slot(zend_object *obj, zval *slot)
{
	// 属性信息指针列表
	zend_property_info **table = obj->ce->properties_info_table;
	// 属性序号
	intptr_t prop_num = slot - obj->properties_table;
	// 属性糖数量必须大等于0 ，并小于 默认属性数量 
	ZEND_ASSERT(prop_num >= 0 && prop_num < obj->ce->default_properties_count);
	// 返回指定序号的属性信息
	return table[prop_num];
}

// 当我们只想得到有类型的属性的属性信息时，可以使用这个助手 
/* Helper for cases where we're only interested in property info of typed properties. */
// ing4, 获取指定属性的属性信息，p1:对象，p2:属性zval
static inline zend_property_info *zend_get_typed_property_info_for_slot(zend_object *obj, zval *slot)
{
	// 通过slot（它也是属性）指针计算slot的序号，返回此序号对应的属性信息
	zend_property_info *prop_info = zend_get_property_info_for_slot(obj, slot);
	// 如果找到属性信息并且 类型存在
	if (prop_info && ZEND_TYPE_IS_SET(prop_info->type)) {
		// 返回属性信息
		return prop_info;
	}
	// 返回空
	return NULL;
}


#endif /* ZEND_OBJECTS_H */
