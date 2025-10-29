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

// 这个list 的操作对象是 zend_resource ，比如mysql返回结果列表？
/* resource lists */

#include "zend.h"
#include "zend_list.h"
#include "zend_API.h"
#include "zend_globals.h"

ZEND_API int le_index_ptr;

// rsrc = resource, 这个list是用来管理各种资源的，所以扩展里大量使用到它
/* true global */
// 【列表销毁器】列表，resource type 是它注册进list_destructors时的序号
static HashTable list_destructors;

// ing3, 向【普通列表】中插入用 给定指针 新创建的 zend_resource 元素 和 类型
ZEND_API zval* ZEND_FASTCALL zend_list_insert(void *ptr, int type)
{
	// 这个zv为什么用临时创建的，而不是分配内存创建呢？因为它只是暂时用来装一下ptr
	zval zv;
	// 取回哈希表的下一个空元素索引号
	zend_long index = zend_hash_next_free_element(&EG(regular_list));
	// 默认是1
	if (index == 0) {
		index = 1;
	// 如果已经溢出，报错
	} else if (index == ZEND_LONG_MAX) {
		zend_error_noreturn(E_ERROR, "Resource ID space overflow");
	}
	// 创建 zend_resource,并关联到 zv上
	ZVAL_NEW_RES(&zv, index, ptr, type);
	// 把zv添加到哈希表中
	return zend_hash_index_add_new(&EG(regular_list), index, &zv);
}

// ing4, EG(regular_list) 中删除 zend_resource
ZEND_API zend_result ZEND_FASTCALL zend_list_delete(zend_resource *res)
{
	// 如果引用次数 -1 后 <=0 才删除
	if (GC_DELREF(res) <= 0) {
		// res->handle 是索引号
		return zend_hash_index_del(&EG(regular_list), res->handle);
	// 否则直接返回成功
	} else {
		return SUCCESS;
	}
}

// ing4, 释放一个 zend_resource 元素
ZEND_API void ZEND_FASTCALL zend_list_free(zend_resource *res)
{
	// 保证没有其他引用
	ZEND_ASSERT(GC_REFCOUNT(res) == 0);
	// 在哈希表中删除，然后调用handle 销毁元素。// res->handle 是索引号
	zend_hash_index_del(&EG(regular_list), res->handle);
}

// ing4, 资源销毁器，销毁但不释放资源本身
static void zend_resource_dtor(zend_resource *res)
{
	zend_rsrc_list_dtors_entry *ld;
	// 要销毁的资源的副本
	zend_resource r = *res;
	// 清除类型
	res->type = -1;
	// 清除指针
	res->ptr = NULL;
	// 销毁器列表中找到 类型 对应的销毁器
	ld = zend_hash_index_find_ptr(&list_destructors, r.type);
	// 必须要能找到
	ZEND_ASSERT(ld && "Unknown list entry type");
	// 如果销毁器中有普通列表销毁器
	if (ld->list_dtor_ex) {
		// 调用它销毁当前资源 
		ld->list_dtor_ex(&r);
	}
}

// ing4, 按实际情况销毁或释放资源 
ZEND_API void ZEND_FASTCALL zend_list_close(zend_resource *res)
{
	// 如果没有引用
	if (GC_REFCOUNT(res) <= 0) {
		// 释放一个 zend_resource 元素
		zend_list_free(res);
	// 如果有引用 并且有类型
	} else if (res->type >= 0) {
		// 销毁一个资源 
		zend_resource_dtor(res);
	}
}

// ing4, 注册资源
ZEND_API zend_resource* zend_register_resource(void *rsrc_pointer, int rsrc_type)
{
	zval *zv;
	// 向【普通列表】中插入用 给定指针 新创建的 zend_resource 元素 和 类型
	zv = zend_list_insert(rsrc_pointer, rsrc_type);
	// 返回资源指针
	return Z_RES_P(zv);
}

// ing4, 两个类型只要有一个能对应上，就返回资源的指针元素
ZEND_API void *zend_fetch_resource2(zend_resource *res, const char *resource_type_name, int resource_type1, int resource_type2)
{
	// 如果资源有效
	if (res) {
		// 类型1可用，返回指针元素
		if (resource_type1 == res->type) {
			return res->ptr;
		}
		// 类型2可用，返回指针元素
		if (resource_type2 == res->type) {
			return res->ptr;
		}
	}
	// 如果没有类型名
	if (resource_type_name) {
		const char *space;
		const char *class_name = get_active_class_name(&space);
		// 报错，资源不可用
		zend_type_error("%s%s%s(): supplied resource is not a valid %s resource", class_name, space, get_active_function_name(), resource_type_name);
	}

	return NULL;
}

// ing4, 先验证资源类型，然后返回资源的指针元素。蛮简单的。
ZEND_API void *zend_fetch_resource(zend_resource *res, const char *resource_type_name, int resource_type)
{
	// 如果资源类型和要求的类型相符
	if (resource_type == res->type) {
		// 返回资源对象的指针元素
		return res->ptr;
	}
	// 如果类型不相符，如果有类型名
	if (resource_type_name) {
		const char *space;
		// 取得类名
		const char *class_name = get_active_class_name(&space);
		// 报错：传入的资源无效
		zend_type_error("%s%s%s(): supplied resource is not a valid %s resource", class_name, space, get_active_function_name(), resource_type_name);
	}

	return NULL;
}

// ing4, 主要是验证资源是否有效，有效则返回下属指针元素
ZEND_API void *zend_fetch_resource_ex(zval *res, const char *resource_type_name, int resource_type)
{
	const char *space, *class_name;
	// 如果res为空
	if (res == NULL) {
		// 如果有类型名
		if (resource_type_name) {
			// 获取当前活动类名
			class_name = get_active_class_name(&space);
			// 报错，无可用资源
			zend_type_error("%s%s%s(): no %s resource supplied", class_name, space, get_active_function_name(), resource_type_name);
		}
		return NULL;
	}
	// 如果 res 类型不是资源 
	if (Z_TYPE_P(res) != IS_RESOURCE) {
		// 如果有类型名
		if (resource_type_name) {
			// 获取当前活动类名
			class_name = get_active_class_name(&space);
			// 报错，传入的参数不是有效资源
			zend_type_error("%s%s%s(): supplied argument is not a valid %s resource", class_name, space, get_active_function_name(), resource_type_name);
		}
		return NULL;
	}

	// 先验证资源类型，然后返回资源的指针元素。蛮简单的。
	return zend_fetch_resource(Z_RES_P(res), resource_type_name, resource_type);
}

// ing4, 先验证资源类型，然后两个类型只要有一个能对应上，就返回资源的指针元素。蛮简单的。
ZEND_API void *zend_fetch_resource2_ex(zval *res, const char *resource_type_name, int resource_type1, int resource_type2)
{
	const char *space, *class_name;
	// 如果资源是空
	if (res == NULL) {
		if (resource_type_name) {
			class_name = get_active_class_name(&space);
			// 报错：无可用资源 
			zend_type_error("%s%s%s(): no %s resource supplied", class_name, space, get_active_function_name(), resource_type_name);
		}
		return NULL;
	}
	// 如果类型不是资源 
	if (Z_TYPE_P(res) != IS_RESOURCE) {
		if (resource_type_name) {
			class_name = get_active_class_name(&space);
			// 报错，传入的参数不是有效资源
			zend_type_error("%s%s%s(): supplied argument is not a valid %s resource", class_name, space, get_active_function_name(), resource_type_name);
		}
		return NULL;
	}
	// ing4, 两个类型只要有一个能对应上，就返回资源的指针元素
	return zend_fetch_resource2(Z_RES_P(res), resource_type_name, resource_type1, resource_type2);
}

// ing4, 销毁并释放一个持久资源
void list_entry_destructor(zval *zv)
{
	// 资源
	zend_resource *res = Z_RES_P(zv);
	// 清空zv
	ZVAL_UNDEF(zv);
	// 如果资源 类型有效
	if (res->type >= 0) {
		// 销毁资源 
		zend_resource_dtor(res);
	}
	// 释放资源内存
	efree_size(res, sizeof(zend_resource));
}

// ing4, 销毁并释放一个持久资源
void plist_entry_destructor(zval *zv)
{
	// 取得资源
	zend_resource *res = Z_RES_P(zv);
	// 如果类型有效
	if (res->type >= 0) {
		// 
		zend_rsrc_list_dtors_entry *ld;
		// 找到资源销毁器
		ld = zend_hash_index_find_ptr(&list_destructors, res->type);
		ZEND_ASSERT(ld && "Unknown list entry type");
		// 如果有持久销毁器
		if (ld->plist_dtor_ex) {
			// 销毁持久列表中的元素
			ld->plist_dtor_ex(res);
		}
	}
	// 释放资源
	free(res);
}

// ing4, 初始化【普通列表】，这个list是个哈希表
ZEND_API void zend_init_rsrc_list(void)
{
	zend_hash_init(&EG(regular_list), 8, NULL, list_entry_destructor, 0);
	EG(regular_list).nNextFreeElement = 0;
}


// ing4, 初始化【持久列表】，这个list是个哈希表
void zend_init_rsrc_plist(void)
{
	// 元素销毁方法是 plist_entry_destructor，持久分配
	zend_hash_init(&EG(persistent_list), 8, NULL, plist_entry_destructor, 1);
}

// ing4, 销毁一个列表里的所有资源元素 
void zend_close_rsrc_list(HashTable *ht)
{
	/* Reload ht->arData on each iteration, as it may be reallocated. */
	// 取得元素数量
	uint32_t i = ht->nNumUsed;
	// 倒着遍历
	while (i-- > 0) {
		// 取得元素
		zval *p = ZEND_HASH_ELEMENT(ht, i);
		// 如果不是未定义
		if (Z_TYPE_P(p) != IS_UNDEF) {
			// 取得资源 
			zend_resource *res = Z_PTR_P(p);
			// 如果类型有效
			if (res->type >= 0) {
				// 销毁资源 
				zend_resource_dtor(res);
			}
		}
	}
}

// ing4, 销毁资源列表
void zend_destroy_rsrc_list(HashTable *ht)
{
	zend_hash_graceful_reverse_destroy(ht);
}

// arg是从 哈希表 API 返回的
/* int return due to HashTable API */
// ing4, 检验资源类型是否是 arg
static int clean_module_resource(zval *zv, void *arg)
{
	// 第二个参数是资源id
	int resource_id = *(int *)arg;

	return Z_RES_TYPE_P(zv) == resource_id;
}

// int 通过 哈希表API返回
/* int return due to HashTable API */
// ing4, 删除此模块的所有持久资源（持久列表元素） ,arg，模块编号
static int zend_clean_module_rsrc_dtors_cb(zval *zv, void *arg)
{
	// 从zv中取出列表销毁器指针
	zend_rsrc_list_dtors_entry *ld = (zend_rsrc_list_dtors_entry *)Z_PTR_P(zv);
	// 模块编号
	int module_number = *(int *)arg;
	// 如果此销毁器属于此模块
	if (ld->module_number == module_number) {
		// 在持久元素列表中查找 并删除 此类型的资源 
		zend_hash_apply_with_argument(&EG(persistent_list), clean_module_resource, (void *) &(ld->resource_id));
		return 1;
	// 不属于此模块，跳过
	} else {
		return 0;
	}
}

// ing4, 销毁指定模块的所有持久资源 ，并删除 销毁器
void zend_clean_module_rsrc_dtors(int module_number)
{
	// 在【列表销毁器】列表中删除此模块的销毁器，并...
	zend_hash_apply_with_argument(&list_destructors, zend_clean_module_rsrc_dtors_cb, (void *) &module_number);
}

// 扩展中大量用到，
// ing4, 注册列表销毁器，传入，列表销毁器，持久列表销毁器，类型名，模块名
ZEND_API int zend_register_list_destructors_ex(rsrc_dtor_func_t ld, rsrc_dtor_func_t pld, const char *type_name, int module_number)
{
	zend_rsrc_list_dtors_entry *lde;
	zval zv;
	// 创建列表销毁器，每个销毁器对应一个资源类型
	lde = malloc(sizeof(zend_rsrc_list_dtors_entry));
	// 列表销毁器
	lde->list_dtor_ex = ld;
	// 持久列表销毁器
	lde->plist_dtor_ex = pld;
	// 模块号
	lde->module_number = module_number;
	// 资源类型 ID
	lde->resource_id = list_destructors.nNextFreeElement;
	// 类型名
	lde->type_name = type_name;
	// 把指针放到 zval里
	ZVAL_PTR(&zv, lde);
	// 插入到【列表销毁器】列表
	if (zend_hash_next_index_insert(&list_destructors, &zv) == NULL) {
		return FAILURE;
	}
	// 返回的 resource type 是上面插入元素的顺序号
	return list_destructors.nNextFreeElement-1;
}

// ing4, 通过类型名找到类型ID
ZEND_API int zend_fetch_list_dtor_id(const char *type_name)
{
	zend_rsrc_list_dtors_entry *lde;
	// 遍历销毁器列表
	ZEND_HASH_PACKED_FOREACH_PTR(&list_destructors, lde) {
		// 如果 type 相符
		if (lde->type_name && (strcmp(type_name, lde->type_name) == 0)) {
			// 返回ID
			return lde->resource_id;
		}
	} ZEND_HASH_FOREACH_END();
	// 返回0
	return 0;
}

// ing4, 释放zv.value.ptr 对应的内存
static void list_destructors_dtor(zval *zv)
{
	free(Z_PTR_P(zv));
}

// ing4, 初始化【列表销毁器】列表
void zend_init_rsrc_list_dtors(void)
{
	// 初始化持久哈希表
	zend_hash_init(&list_destructors, 64, NULL, list_destructors_dtor, 1);
	// resource type 不可以是0（所以resource type 是它注册进list_destructors时的序号）
	list_destructors.nNextFreeElement=1;	/* we don't want resource type 0 */
}


// ing4, 销毁整个【列表销毁器】列表
void zend_destroy_rsrc_list_dtors(void)
{
	zend_hash_destroy(&list_destructors);
}


// ing4, 取得资源列表类型
const char *zend_rsrc_list_get_rsrc_type(zend_resource *res)
{
	zend_rsrc_list_dtors_entry *lde;
	// 取得资源类型对应的销毁器
	lde = zend_hash_index_find_ptr(&list_destructors, res->type);
	// 如果获取成功，返回销毁器里的类型名
	if (lde) {
		return lde->type_name;
	} else {
		return NULL;
	}
}

// ing4, 注册持久资源
ZEND_API zend_resource* zend_register_persistent_resource_ex(zend_string *key, void *rsrc_pointer, int rsrc_type)
{
	zval *zv;
	zval tmp;
	// 创建持久resource, 初始 handle 为 -1
	ZVAL_NEW_PERSISTENT_RES(&tmp, -1, rsrc_pointer, rsrc_type);
	// 这两个高度用
	GC_MAKE_PERSISTENT_LOCAL(Z_COUNTED(tmp));
	GC_MAKE_PERSISTENT_LOCAL(key);
	// 更新【持久列表】中的指定资源
	zv = zend_hash_update(&EG(persistent_list), key, &tmp);
	// 返回资源指针
	return Z_RES_P(zv);
}

// ing4, 注册持久资源
ZEND_API zend_resource* zend_register_persistent_resource(const char *key, size_t key_len, void *rsrc_pointer, int rsrc_type)
{
	// 创建键名 zend_string
	zend_string *str = zend_string_init(key, key_len, 1);
	// 注册持久资源
	zend_resource *ret  = zend_register_persistent_resource_ex(str, rsrc_pointer, rsrc_type);
	// 释放键名
	zend_string_release_ex(str, 1);
	return ret;
}
