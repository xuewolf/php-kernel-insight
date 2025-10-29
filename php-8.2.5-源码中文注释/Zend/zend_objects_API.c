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
#include "zend_objects_API.h"
#include "zend_fibers.h"

// 8个方法，只有 zend_fiber_switch_block 暂不明白。
// 这个主要是对象容器的方法，对象容器存在的最主要的意义是在程序关闭时快速销毁所有对象。

// ing4，初始化 全局对象容器。zend_objects_store全局只有一个实例在，_zend_executor_globals 结构体中，参见 /Zend/zend_globals.h
ZEND_API void ZEND_FASTCALL zend_objects_store_init(zend_objects_store *objects, uint32_t init_size)
{
	// 大小 = 初始大小 * 对象指针的尺寸，类型为指针列表
	objects->object_buckets = (zend_object **) emalloc(init_size * sizeof(zend_object*));
	// 跳过 0，使 if 判断时为真
	objects->top = 1; /* Skip 0 so that handles are true */
	// 初始大小
	objects->size = init_size;
	// 初始值为 -1。没有可用的元素。
	objects->free_list_head = -1;
	// 第一个指针用 0 填充
	memset(&objects->object_buckets[0], 0, sizeof(zend_object*));
}

// ing4, 销毁 zend_objects_store
ZEND_API void ZEND_FASTCALL zend_objects_store_destroy(zend_objects_store *objects)
{
	// 销毁 object_buckets 对象指针列表
	efree(objects->object_buckets);
	// 指针为null
	objects->object_buckets = NULL;
}

// ing4，销毁每一个对象
ZEND_API void ZEND_FASTCALL zend_objects_store_call_destructors(zend_objects_store *objects)
{
	// 添加 【对象容器无复用】标记
	EG(flags) |= EG_FLAGS_OBJECT_STORE_NO_REUSE;
	// 如果仓库里有对象
	if (objects->top > 1) {
		// 锁定 fiber 
		zend_fiber_switch_block();

		uint32_t i;
		// 遍历每一个对象
		for (i = 1; i < objects->top; i++) {
			// 对象指针
			zend_object *obj = objects->object_buckets[i];
			// 如果对象有效
			if (IS_OBJ_VALID(obj)) {
				// 如果没有【已销毁】标记
				if (!(OBJ_FLAGS(obj) & IS_OBJ_DESTRUCTOR_CALLED)) {
					// 添加【已销毁】标记
					GC_ADD_FLAGS(obj, IS_OBJ_DESTRUCTOR_CALLED);
					// 如果有自己的销毁方法，或所属类有析构方法
					if (obj->handlers->dtor_obj != zend_objects_destroy_object
							|| obj->ce->destructor) {
						// 引用计数 +1
						GC_ADDREF(obj);
						// 调用自字义方法销毁对象
						obj->handlers->dtor_obj(obj);
						// 引用计数 -1
						GC_DELREF(obj);
					}
				}
			}
		}
		// 解锁 fiber 
		zend_fiber_switch_unblock();
	}
}

// ing4, 把仓库中的每一个对象标记成 已销毁 （并没有真正销毁）
ZEND_API void ZEND_FASTCALL zend_objects_store_mark_destructed(zend_objects_store *objects)
{
	// 如果 object_buckets 存在，并且有存入对象指针
	if (objects->object_buckets && objects->top > 1) {
		// 从第一个开始
		zend_object **obj_ptr = objects->object_buckets + 1;
		// 到最后一个
		zend_object **end = objects->object_buckets + objects->top;
		// 遍历每一个对象
		do {
			zend_object *obj = *obj_ptr;
			// 如果对象有效
			if (IS_OBJ_VALID(obj)) {
				// 添加 【已销毁】 标记
				GC_ADD_FLAGS(obj, IS_OBJ_DESTRUCTOR_CALLED);
			}
			// 下一个
			obj_ptr++;
		} while (obj_ptr != end);
	}
}

// ing4, 释放对象容器
ZEND_API void ZEND_FASTCALL zend_objects_store_free_object_storage(zend_objects_store *objects, bool fast_shutdown)
{
	zend_object **obj_ptr, **end, *obj;
	// 如果使用位置<1 ，说明已经释放过了，直接返回
	if (objects->top <= 1) {
		return;
	}

	// 释放对象内容，但不释放对象本身，所以看起来像内存泄露
	// 还要添加一个引用次数给所有对象，对象不会在随后被其他操作释放
	/* Free object contents, but don't free objects themselves, so they show up as leaks.
	 * Also add a ref to all objects, so the object can't be freed by something else later. */
	 
	// 结束位置，就是初始化空容器时的位置，开头留1个指针的空间
	end = objects->object_buckets + 1;
	// 找到容器的游标位置
	obj_ptr = objects->object_buckets + objects->top;

	// 如果要求快速关闭
	if (fast_shutdown) {
		do {
			// 游标退 1格
			obj_ptr--;
			// 对象指针
			obj = *obj_ptr;
			// 如果对象有效
			if (IS_OBJ_VALID(obj)) {
				// 如果对象没有【已释放】标记
				if (!(OBJ_FLAGS(obj) & IS_OBJ_FREE_CALLED)) {
					// 添加【已释放】标记
					GC_ADD_FLAGS(obj, IS_OBJ_FREE_CALLED);
					// 如果销毁方法不是 zend_object_std_dtor
					if (obj->handlers->free_obj != zend_object_std_dtor) {
						// 引用数 -1
						GC_ADDREF(obj);
						// 调用自己的销毁方法
						obj->handlers->free_obj(obj);
					}
					// 如果销毁方法是 zend_object_std_dtor，什么也不做
				}
			}
		// 直到开头位置
		} while (obj_ptr != end);
	// 不是快速关闭
	} else {
		do {
			// 游标退 1格
			obj_ptr--;
			// 对象指针
			obj = *obj_ptr;
			// 如果对象有效
			if (IS_OBJ_VALID(obj)) {
				// 如果对象没有【已释放】标记
				if (!(OBJ_FLAGS(obj) & IS_OBJ_FREE_CALLED)) {
					// 添加【已释放】标记
					GC_ADD_FLAGS(obj, IS_OBJ_FREE_CALLED);
					// 引用数 -1
					GC_ADDREF(obj);
					// 释放对象
					obj->handlers->free_obj(obj);
				}
			}
		// 直到开头位置
		} while (obj_ptr != end);
	}
}


// ing4, 增加容器空间，并把对象添加到 store 里
/* Store objects API */
static ZEND_COLD zend_never_inline void ZEND_FASTCALL zend_objects_store_put_cold(zend_object *object)
{
	int handle;
	// 每次重新分配，空间增加 1 倍
	uint32_t new_size = 2 * EG(objects_store).size;
	// 重新分配空间
	EG(objects_store).object_buckets = (zend_object **) erealloc(EG(objects_store).object_buckets, new_size * sizeof(zend_object*));
	// 万一(in case)失败了, 在 重新分配 后设置尺寸，
	/* Assign size after realloc, in case it fails */
	// 新的尺寸
	EG(objects_store).size = new_size;
	// 使用数量 +1
	handle = EG(objects_store).top++;
	// 对象的 handle元素 是对象在对象容器中的【索引号】
	object->handle = handle;
	// 把对象指针写入 容器
	EG(objects_store).object_buckets[handle] = object;
}

// ing4, 把对象指针添加进全局 对象容器 里
ZEND_API void ZEND_FASTCALL zend_objects_store_put(zend_object *object)
{
	int handle;
	// 当处于关闭队列中，不要重复使用前面释放过的 handles。
	// 保证新建对象的销毁器在 zend_objects_store_call_destructors() 循环中被调用。
	// zend_objects_store_call_destructors (): 销毁 zend_objects_store 中的每一个对象
	/* When in shutdown sequence - do not reuse previously freed handles, to make sure
	 * the dtors for newly created objects are called in zend_objects_store_call_destructors() loop
	 */
	// 如果 有可复用的元素 并且没有全局的 【对象容器不可复用】标记
	if (EG(objects_store).free_list_head != -1 && EXPECTED(!(EG(flags) & EG_FLAGS_OBJECT_STORE_NO_REUSE))) {
		// 第一个可复用元素
		handle = EG(objects_store).free_list_head;
		// 可利用链表的指针指向下一个元素
		EG(objects_store).free_list_head = GET_OBJ_BUCKET_NUMBER(EG(objects_store).object_buckets[handle]);
	// 如果对象指针仓库已经满了
	} else if (UNEXPECTED(EG(objects_store).top == EG(objects_store).size)) {
		// 开辟空间并存入新对象指针
		zend_objects_store_put_cold(object);
		return;
	// 如果有可用空间
	} else {
		// 使用新序号，objects_store 元素数 +1
		handle = EG(objects_store).top++;
	}
	// 元素添加仓储序号
	object->handle = handle;
	// 对应序号的指针 指向此对象
	EG(objects_store).object_buckets[handle] = object;
}

// ing2, 释放指定的对象，并更新 objects_store 中的指针
ZEND_API void ZEND_FASTCALL zend_objects_store_del(zend_object *object) /* {{{ */
{
	// 引用次数必须已经是0
	ZEND_ASSERT(GC_REFCOUNT(object) == 0);

	// 如果 GC_TYPE 已经是 null了，直接返回
	/* GC might have released this object already. */
	if (UNEXPECTED(GC_TYPE(object) == IS_NULL)) {
		return;
	}

	// 保证在销毁器调用过程中，有一个引用计数器。否则当销毁结束时，引用次数为0， 仓库会被清空
	/*	Make sure we hold a reference count during the destructor call
		otherwise, when the destructor ends the storage might be freed
		when the refcount reaches 0 a second time
	 */
	// 如果没有【销毁中】标记
	if (!(OBJ_FLAGS(object) & IS_OBJ_DESTRUCTOR_CALLED)) {
		// 添加 【销毁中】标记
		GC_ADD_FLAGS(object, IS_OBJ_DESTRUCTOR_CALLED);
		// 如果不用 zend_objects_destroy_object 方法销毁 或 类有自带的析构方法
		if (object->handlers->dtor_obj != zend_objects_destroy_object
				|| object->ce->destructor) {
			// ？？？？？
			zend_fiber_switch_block();
			// 引用数为1
			GC_SET_REFCOUNT(object, 1);
			// 调用处理器，销毁对象
			object->handlers->dtor_obj(object);
			// 引用次数 -1
			GC_DELREF(object);
			// 解除 fiber 锁定
			zend_fiber_switch_unblock();
		}
	}

	// 如果引用次数为0
	if (GC_REFCOUNT(object) == 0) {
		// handle 是对象在全局对象容器 EG(objects_store).object_buckets 中的【索引号】，通过它可以直接在全局容器中获取指针
		uint32_t handle = object->handle;
		void *ptr;
		// object_buckets 不可以是 NULL
		ZEND_ASSERT(EG(objects_store).object_buckets != NULL);
		// 处理器对象必须 存在并且有效
		ZEND_ASSERT(IS_OBJ_VALID(EG(objects_store).object_buckets[handle]));
		// 处理器对象设置成无效
		EG(objects_store).object_buckets[handle] = SET_OBJ_INVALID(object);
		// 如果object的不是【已释放】状态
		if (!(OBJ_FLAGS(object) & IS_OBJ_FREE_CALLED)) {
			// 添加 【已释放】状态
			GC_ADD_FLAGS(object, IS_OBJ_FREE_CALLED);
			// 引用次数为1
			GC_SET_REFCOUNT(object, 1);
			//  调用处理器释放 对象
			object->handlers->free_obj(object);
		}
		// 找到整个对象的开头指针，一般 offset是0
		ptr = ((char*)object) - object->handlers->offset;
		// 从gc缓存中删除
		GC_REMOVE_FROM_BUFFER(object);
		// **释放对象
		efree(ptr);
		// 回收 objects_store 里面的 元素指针
		ZEND_OBJECTS_STORE_ADD_TO_FREE_LIST(handle);
	}
}
/* }}} */
