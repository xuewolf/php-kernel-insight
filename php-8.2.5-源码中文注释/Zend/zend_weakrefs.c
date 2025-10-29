/*
   +----------------------------------------------------------------------+
   | Copyright (c) The PHP Group                                          |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.00 of the Zend license,     |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.zend.com/license/2_00.txt.                                |
   | If you did not receive a copy of the Zend license and are unable to  |
   | obtain it through the world-wide-web, please send a note to          |
   | license@zend.com so we can mail you a copy immediately.              |
   +----------------------------------------------------------------------+
   | Authors: krakjoe@php.net                                             |
   +----------------------------------------------------------------------+
*/

#include "zend.h"
#include "zend_interfaces.h"
#include "zend_objects_API.h"
#include "zend_weakrefs.h"
#include "zend_weakrefs_arginfo.h"

// WeakReference，所以对象从一开始就要被创建成 zend_weakref
typedef struct _zend_weakref {
	zend_object *referent;
	zend_object std;
} zend_weakref;

// WeakMap
typedef struct _zend_weakmap {
	HashTable ht;
	zend_object std;
} zend_weakmap;

// weakmap 迭代器
typedef struct _zend_weakmap_iterator {
	zend_object_iterator it;
	uint32_t ht_iter;
} zend_weakmap_iterator;

// EG(weakrefs) 是一组映射关系（HashTable），从【一个对应到一个 zend_object 指针的键】 到 【所有相关的 WeakReference 和 WeakMap 实体】
/* EG(weakrefs) is a map from a key corresponding to a zend_object pointer to all the WeakReference and/or WeakMap entries relating to that pointer.
 *
 1. 对于单个 WeakReference ，哈希表的对应值标记是 ZEND_WEAKREF_TAG_REF ，指针是一个孤立的 WeakReference 实例(zend_weakref *)
 * 1. For a single WeakReference,
 *    the HashTable's corresponding value's tag is a ZEND_WEAKREF_TAG_REF and the pointer is a singleton WeakReference instance (zend_weakref *) for that zend_object pointer (from WeakReference::create()).
 
 对于单独的 WeakMap ，哈希表的对应值的标记是 ZEND_WEAKREF_TAG_MAP ，指针是一个 WeakMap 实例
 * 2. For a single WeakMap, the HashTable's corresponding value's tag is a ZEND_WEAKREF_TAG_MAP and the pointer is a WeakMap instance (zend_weakmap *).
 
 对于关系到同一个对象指针的多个值，哈希表实体的tag是ZEND_WEAKREF_TAG_HT 和一个 哈希映射。
 * 3. For multiple values associated with the same zend_object pointer, the HashTable entry's tag is a ZEND_WEAKREF_TAG_HT with a HashTable mapping
 *    tagged pointers of at most 1 WeakReference and 1 or more WeakMaps to the same tagged pointer.
 *
 
 ZEND_MM_ALIGNED_OFFSET_LOG2 在支持的结构上是至少是2。（即使在32位系统中，指向对象的指针也对齐到4bytes）
 * ZEND_MM_ALIGNED_OFFSET_LOG2 is at least 2 on supported architectures (pointers to the objects in question are aligned to 4 bytes (1<<2) even on 32-bit systems),
 
 例如 指针的最后的两个独立位 可以用来存放tag
 * i.e. the least two significant bits of the pointer can be used as a tag (ZEND_WEAKREF_TAG_*). */
// 映射到单个 WeakReference
#define ZEND_WEAKREF_TAG_REF 0
// 映射到单个 WeakMap
#define ZEND_WEAKREF_TAG_MAP 1
// 映射到哈希表
#define ZEND_WEAKREF_TAG_HT  2
// ing4, 取回指针中的弱引用tag（最后2位）
#define ZEND_WEAKREF_GET_TAG(p) (((uintptr_t) (p)) & 3)
// ing4, 取回有效指针（最后2位是0）
#define ZEND_WEAKREF_GET_PTR(p) ((void *) (((uintptr_t) (p)) & ~3))
// ing3, 用t给指针加码。其实就是添加 tag。
#define ZEND_WEAKREF_ENCODE(p, t) ((void *) (((uintptr_t) (p)) | (t)))

zend_class_entry *zend_ce_weakref;
zend_class_entry *zend_ce_weakmap;
static zend_object_handlers zend_weakref_handlers;
static zend_object_handlers zend_weakmap_handlers;

// ing4, 从对象指针获取指定对象的弱引用 ，从对象指针，向左移动到 整个 _zend_weakref 的开头
#define zend_weakref_from(o) ((zend_weakref*)(((char*) o) - XtOffsetOf(zend_weakref, std)))
// ing4, 从 zval 获取指定对象的弱引用
#define zend_weakref_fetch(z) zend_weakref_from(Z_OBJ_P(z))

// ing4, 从对象指针获取指定对象的弱映射 ，从对象指针，向左移动到 整个 _zend_weakmap 的开头
#define zend_weakmap_from(o) ((zend_weakmap*)(((char*) o) - XtOffsetOf(zend_weakmap, std)))
// ing4, 从 zval 获取指定对象的弱映射
#define zend_weakmap_fetch(z) zend_weakmap_from(Z_OBJ_P(z))

// ing3, 取消引用单个的对象
static inline void zend_weakref_unref_single(
		void *ptr, uintptr_t tag, zend_object *object)
{
	// 如果tag是 单个引用
	if (tag == ZEND_WEAKREF_TAG_REF) {
		/* Unreferencing WeakReference (at ptr) singleton that pointed to object. */
		zend_weakref *wr = ptr;
		// 引用对象置空
		wr->referent = NULL;
	// 如果目标不是单个引用，是 哈希表
	} else {
		// 必须保证是哈希表
		/* unreferencing WeakMap entry (at ptr) with a key of object. */
		ZEND_ASSERT(tag == ZEND_WEAKREF_TAG_MAP);
		// 从对象指针获取弱引用key（直接右移3位）
		// 删除指定键值
		zend_hash_index_del((HashTable *) ptr, zend_object_to_weakref_key(object));
	}
}

// ing3, 撤消弱引用, p1:引用目标，p2:弱引用实例
static void zend_weakref_unref(zend_object *object, void *tagged_ptr) {
	// 取回有效指针（最后2位是0）
	void *ptr = ZEND_WEAKREF_GET_PTR(tagged_ptr);
	// 取回指针中的弱引用tag（最后2位）
	uintptr_t tag = ZEND_WEAKREF_GET_TAG(tagged_ptr);
	// 如果类型是 哈希表
	if (tag == ZEND_WEAKREF_TAG_HT) {
		HashTable *ht = ptr;
		// 遍历哈希表
		ZEND_HASH_MAP_FOREACH_PTR(ht, tagged_ptr) {
			// 取消引用单个的对象
			// 取回有效指针（最后2位是0）
			// 取回指针中的弱引用tag（最后2位）
			zend_weakref_unref_single(
				ZEND_WEAKREF_GET_PTR(tagged_ptr), ZEND_WEAKREF_GET_TAG(tagged_ptr), object);
		} ZEND_HASH_FOREACH_END();
		// 销毁哈希表 
		zend_hash_destroy(ht);
		// 释放哈希表
		FREE_HASHTABLE(ht);
	} else {
		// 取消引用单个的对象
		zend_weakref_unref_single(ptr, tag, object);
	}
}

// ing3, 注册新的弱引用。
	// 如果 EG(weakrefs) 中的映射对象是单个元素，创建哈希表把它和新元素放进去。
	// 如果是哈希表，直接把新元素放进去。
	// 如果是空元素，把新元素放进去。
static void zend_weakref_register(zend_object *object, void *payload) {
	// 对象添加 弱引用标记
	GC_ADD_FLAGS(object, IS_OBJ_WEAKLY_REFERENCED);
	// 从对象指针获取弱引用key（直接右移3位）
	zend_ulong obj_key = zend_object_to_weakref_key(object);
	// 先查找key
	zval *zv = zend_hash_index_lookup(&EG(weakrefs), obj_key);
	// 情况1。如果找到的类型是 NULL
	if (Z_TYPE_P(zv) == IS_NULL) {
		// 让此元素指向 新对象
		ZVAL_PTR(zv, payload);
		return;
	}
	// 如果找到的不是NULL

	// 取带标记的的指针
	void *tagged_ptr = Z_PTR_P(zv);
	// 情况2。如果标记是 ZEND_WEAKREF_TAG_HT，（映射目标是哈希表）
	// 取回指针中的弱引用tag（最后2位）
	if (ZEND_WEAKREF_GET_TAG(tagged_ptr) == ZEND_WEAKREF_TAG_HT) {
		// 取回有效指针（最后2位是0）
		HashTable *ht = ZEND_WEAKREF_GET_PTR(tagged_ptr);
		// 添加新指针元素 payload，键名是它的指针的整数值
		zend_hash_index_add_new_ptr(ht, (zend_ulong) payload, payload);
		return;
	}
	// 情况3。如果映射目标不是哈希表
	
	// 把简单指针转成哈希表
	/* Convert simple pointer to hashtable. */
	// 创建哈希表
	HashTable *ht = emalloc(sizeof(HashTable));
	// 初始化成顺序哈希表
	zend_hash_init(ht, 0, NULL, NULL, 0);
	// 添加新元素 tagged_ptr
	zend_hash_index_add_new_ptr(ht, (zend_ulong) tagged_ptr, tagged_ptr);
	// 添加新元素 payload
	zend_hash_index_add_new_ptr(ht, (zend_ulong) payload, payload);
	// 把新哈希表的指针添加 ZEND_WEAKREF_TAG_HT 标记，然后把指针写回到 zv中去
	/* Replace the single WeakMap or WeakReference entry in EG(weakrefs) with a HashTable with 2 entries in place. */
	// 用t给指针加码。其实就是添加 tag。
	ZVAL_PTR(zv, ZEND_WEAKREF_ENCODE(ht, ZEND_WEAKREF_TAG_HT));
}

// ing4, 反注册弱映射，在 EG(weakrefs) 中删除元素
static void zend_weakref_unregister(zend_object *object, void *payload, bool weakref_free) {
	// 从对象指针获取弱引用key（直接右移3位）
	zend_ulong obj_key = zend_object_to_weakref_key(object);
	// 取得带标记的指针
	void *tagged_ptr = zend_hash_index_find_ptr(&EG(weakrefs), obj_key);
	// 指针必须存在
	ZEND_ASSERT(tagged_ptr && "Weakref not registered?");
	// 取回有效指针（最后2位是0）
	void *ptr = ZEND_WEAKREF_GET_PTR(tagged_ptr);
	// 取回指针中的弱引用tag（最后2位）
	uintptr_t tag = ZEND_WEAKREF_GET_TAG(tagged_ptr);
	// 情况1， 如果目标不是哈希表
	if (tag != ZEND_WEAKREF_TAG_HT) {
		// 取回的指针必须是 此对象的指针
		ZEND_ASSERT(tagged_ptr == payload);
		// 在弱引用表里删除这个键
		zend_hash_index_del(&EG(weakrefs), obj_key);
		// 对象删除 弱引用标记
		GC_DEL_FLAGS(object, IS_OBJ_WEAKLY_REFERENCED);

		// 最后做这一步，它可能销毁对象
		/* Do this last, as it may destroy the object. */
		// 如果要释放对象
		if (weakref_free) {
			// 取消引用单个的对象
			zend_weakref_unref_single(ptr, tag, object);
		// 如果不要释放对象
		} else {
			// 跳过未定义元素的优化只用在 WeakMap 的销毁中
			/* The optimization of skipping unref is only used in the destructor of WeakMap */
			// 取回指针中的弱引用tag（最后2位）
			ZEND_ASSERT(ZEND_WEAKREF_GET_TAG(payload) == ZEND_WEAKREF_TAG_MAP);
		}
		return;
	}
	// 情况2。如果目标是哈希表

	HashTable *ht = ptr;
// 调试用
#if ZEND_DEBUG
	void *old_payload = zend_hash_index_find_ptr(ht, (zend_ulong) payload);
	ZEND_ASSERT(old_payload && "Weakref not registered?");
	ZEND_ASSERT(old_payload == payload);
#endif

	// 在哈希表中骑过 数字索引 删除元素
	zend_hash_index_del(ht, (zend_ulong) payload);
	// 如果元素数量是0
	if (zend_hash_num_elements(ht) == 0) {
		// 对象指针删除 弱引用 标记
		GC_DEL_FLAGS(object, IS_OBJ_WEAKLY_REFERENCED);
		// 销毁哈希表
		zend_hash_destroy(ht);
		// 释放哈希表
		FREE_HASHTABLE(ht);
		// 弱引用表里删除指针元素
		zend_hash_index_del(&EG(weakrefs), obj_key);
	}
	// 最后销毁对象
	/* Do this last, as it may destroy the object. */
	if (weakref_free)  {
		// 取消引用单个的对象
		// 取回指针中的弱引用tag（最后2位）
		zend_weakref_unref_single(
			ZEND_WEAKREF_GET_PTR(payload), ZEND_WEAKREF_GET_TAG(payload), object);
	} else {
		/* The optimization of skipping unref is only used in the destructor of WeakMap */
		// // 取回指针中的弱引用tag（最后2位）
		ZEND_ASSERT(ZEND_WEAKREF_GET_TAG(payload) == ZEND_WEAKREF_TAG_MAP);
	}
}

// ing3, 把对象和映射对象添加到哈希表中，并注册弱引用
ZEND_API zval *zend_weakrefs_hash_add(HashTable *ht, zend_object *key, zval *pData) {
	// 从对象指针获取弱引用key（直接右移3位）
	// 把zval指针添加到顺序哈希表中, 返回包含 zend_weakref 的zval
	zval *zv = zend_hash_index_add(ht, zend_object_to_weakref_key(key), pData);
	// 如果添加成功
	if (zv) {
		// 注册弱引用 ，使用标记 【单独的弱引用】
		// 用t给指针加码。其实就是添加 tag。
		zend_weakref_register(key, ZEND_WEAKREF_ENCODE(ht, ZEND_WEAKREF_TAG_MAP));
	}
	return zv;
}

// ing3, 从哈希表中取出弱引用对象，并反注册它
ZEND_API zend_result zend_weakrefs_hash_del(HashTable *ht, zend_object *key) {
	// 从对象指针获取弱引用key（直接右移3位）
	// 从哈希表中取出要求的元素
	zval *zv = zend_hash_index_find(ht, zend_object_to_weakref_key(key));
	// 如果获取成功
	if (zv) {
		// 反注册弱引用
		// 用t给指针加码。其实就是添加 tag。
		zend_weakref_unregister(key, ZEND_WEAKREF_ENCODE(ht, ZEND_WEAKREF_TAG_MAP), 1);
		return SUCCESS;
	}
	return FAILURE;
}

// ing4, 初始化弱引用
void zend_weakrefs_init(void) {
	// 初始化弱引用哈希表。此哈希表在execute global中
	zend_hash_init(&EG(weakrefs), 8, NULL, NULL, 0);
}

// 当对象被垃圾回收时调用，用来删除所有关联到此对象的 弱引用和 弱映射实体
/* This is called when the object is garbage collected
 * to remove all WeakReference and WeakMap entries weakly referencing that object. */
// ing3, 删除此对象的引线引用
void zend_weakrefs_notify(zend_object *object) {
	// 烦人的是，这里不能使用 哈希表销毁器，因为需要访问 作为键的对象地址，它不会被传给销毁器
	/* Annoyingly we can't use the HT destructor here, because we need access to the key (which
	 * is the object address), which is not provided to the dtor. */
	// 从对象指针获取弱引用key（直接右移3位） 
	const zend_ulong obj_key = zend_object_to_weakref_key(object);
	// 取回弱引用实例指针
	void *tagged_ptr = zend_hash_index_find_ptr(&EG(weakrefs), obj_key);
#if ZEND_DEBUG
	ZEND_ASSERT(tagged_ptr && "Tracking of the IS_OBJ_WEAKLY_REFERENCE flag should be precise");
#endif
	// 如果指针有效
	if (tagged_ptr) {
		// 撤消弱引用, p1:引用目标，p2:弱引用实例
		zend_weakref_unref(object, tagged_ptr);
		// 在弱引用表中删除这个对象的引用
		zend_hash_index_del(&EG(weakrefs), obj_key);
	}
}

// ing4, 关闭弱引用
void zend_weakrefs_shutdown(void) {
	// 销毁弱引用哈希表的元素内容
	zend_hash_destroy(&EG(weakrefs));
}

// ing4, 创建新的 WeakReference 并返回它下属对象 的指针
// ce没有用到，只是为了满足 ->create_object 创建方法的格式
static zend_object* zend_weakref_new(zend_class_entry *ce) {
	// 创建一个 WeakReference 类 对象
	zend_weakref *wr = zend_object_alloc(sizeof(zend_weakref), zend_ce_weakref);
	// 初始化对象
	zend_object_std_init(&wr->std, zend_ce_weakref);
	// 添加处理器
	wr->std.handlers = &zend_weakref_handlers;
	// 返回 zend_weakref的对象元素 指针
	return &wr->std;
}

// ing3, 通过对象指针获取 映射目标的结果
static zend_always_inline bool zend_weakref_find(zend_object *referent, zval *return_value) {
	// 从对象指针获取弱引用key（直接右移3位）
	void *tagged_ptr = zend_hash_index_find_ptr(&EG(weakrefs), zend_object_to_weakref_key(referent));
	if (!tagged_ptr) {
		return 0;
	}
	// 取回有效指针（最后2位是0）
	void *ptr = ZEND_WEAKREF_GET_PTR(tagged_ptr);
	// 取回指针中的弱引用tag（最后2位）
	uintptr_t tag = ZEND_WEAKREF_GET_TAG(tagged_ptr);
	// 如果tag是，单个引用
	if (tag == ZEND_WEAKREF_TAG_REF) {
		zend_weakref *wr;
found_weakref:
		wr = ptr;
		// zend_weakref 里的对象复制给返回结果 
		RETVAL_OBJ_COPY(&wr->std);
		return 1;
	}
	// 如果 tag是，弱映射表
	if (tag == ZEND_WEAKREF_TAG_HT) {
		// 遍历弱映射表
		ZEND_HASH_MAP_FOREACH_PTR(ptr, tagged_ptr) {
			// 如果 key是 单个引用。找到第一个有 【单个引用标记】 的元素。
			// 取回指针中的弱引用tag（最后2位）
			if (ZEND_WEAKREF_GET_TAG(tagged_ptr) == ZEND_WEAKREF_TAG_REF) {
				// 取回有效指针（最后2位是0）
				ptr = ZEND_WEAKREF_GET_PTR(tagged_ptr);
				// 按查找成功操作
				goto found_weakref;
			}
		} ZEND_HASH_FOREACH_END();
	}

	return 0;
}

// ing3, 为指定对象 创建弱引用， 并把 新创建的 zend_weakref 关联到 return_value 中返回
static zend_always_inline void zend_weakref_create(zend_object *referent, zval *return_value) {
	zend_weakref *wr;
	// 创建对象并关联到 return_value
	object_init_ex(return_value, zend_ce_weakref);
	// 从 zval 获取指定对象的弱引用
	wr = zend_weakref_fetch(return_value);
	// 添加引用目标
	wr->referent = referent;
	// 把引用目标和 zend_weakref 注册到弱引用表里
	// 用t给指针加码。其实就是添加 tag。
	zend_weakref_register(wr->referent, ZEND_WEAKREF_ENCODE(wr, ZEND_WEAKREF_TAG_REF));
}

// ing4, 从 zval 获取 引用目标 
static zend_always_inline void zend_weakref_get(zval *weakref, zval *return_value) {
	// 从 zval 获取指定对象的弱引用
	zend_weakref *wr = zend_weakref_fetch(weakref);
	// 如果有映射目标
	if (wr->referent) {
		// 把目标对象拷贝到 返回结果里
		RETVAL_OBJ_COPY(wr->referent);
	}
}

// ing3, 释放指定对象的弱引用
static void zend_weakref_free(zend_object *zo) {
	// 获取单个引用
	zend_weakref *wr = zend_weakref_from(zo);
	// 如果有引用对象
	if (wr->referent) {
		// 先反注册弱引用
		// 用t给指针加码。其实就是添加 tag。
		zend_weakref_unregister(wr->referent, ZEND_WEAKREF_ENCODE(wr, ZEND_WEAKREF_TAG_REF), 1);
	}
	// 使用标准销毁器销毁 引用实例中的对象
	zend_object_std_dtor(&wr->std);
}

// ing4, 构造方法
ZEND_COLD ZEND_METHOD(WeakReference, __construct)
{
	// 报错：不可以直接实例化 WeakReference，使用 WeakReference::create 来替代
	zend_throw_error(NULL, "Direct instantiation of WeakReference is not allowed, use WeakReference::create instead");
}

// ing3, 给对象创建弱引用
ZEND_METHOD(WeakReference, create)
{
	zend_object *referent;
	// 验证参数数量是否合法并取出参数列表, 传入：最小数量，最大数量 
	ZEND_PARSE_PARAMETERS_START(1,1)
		// 获取传入的对象
		Z_PARAM_OBJ(referent)
	ZEND_PARSE_PARAMETERS_END();
	// 通过对象指针获取 映射目标的结果
	if (zend_weakref_find(referent, return_value)) {
		// 如果失败，中断
	    return;
	}
	// 为指定对象 创建弱引用， 并把 新创建的 zend_weakref 关联到 return_value 中返回
	zend_weakref_create(referent, return_value);
}

// ing3, 获取弱引用对象
ZEND_METHOD(WeakReference, get)
{
	// 不可以传入参数
	ZEND_PARSE_PARAMETERS_NONE();
	// 从 zval 获取指定对象的弱引用
	zend_weakref_get(getThis(), return_value);
}

// ing4, 创建新的映射表对象
static zend_object *zend_weakmap_create_object(zend_class_entry *ce)
{
	// 创建新的 弱映射表实例，类为 ce
	zend_weakmap *wm = zend_object_alloc(sizeof(zend_weakmap), ce);
	// 初始化对象
	zend_object_std_init(&wm->std, ce);
	// 对象的标准处理器使用 弱映射表的处理器
	wm->std.handlers = &zend_weakmap_handlers;
	// 初始化哈希表
	zend_hash_init(&wm->ht, 0, NULL, ZVAL_PTR_DTOR, 0);
	// 返回映射表中的对象
	return &wm->std;
}

// ing3, 释放弱引用表中的对象
static void zend_weakmap_free_obj(zend_object *object)
{
	// 从对象指针获取指定对象的弱映射 ，从对象指针，向左移动到 整个 _zend_weakmap 的开头
	zend_weakmap *wm = zend_weakmap_from(object);
	zend_ulong obj_key;
	// 遍历引用哈希表
	ZEND_HASH_MAP_FOREACH_NUM_KEY(&wm->ht, obj_key) {
		// 优化：反注册时，不调用 zend_weakref_unref_single 来释放单个 弱引用哈希表中的 实体
		// （那样会在哈希表中查找，调用 zend_hash_index_del 并且跳过任何 bucket 冲突）
		// 先释放 zend_hash_destroy 中应该释放的值，依次销毁对象
		// 当弱引用表很大，本地缓存性能不佳时，这样做会大副提升效率
		/* Optimization: Don't call zend_weakref_unref_single to free individual entries from wm->ht when unregistering (which would do a hash table lookup, call zend_hash_index_del, and skip over any bucket collisions).
		 * Let freeing the corresponding values for WeakMap entries be done in zend_hash_destroy, freeing objects sequentially.
		 * The performance difference is notable for larger WeakMaps with worse cache locality. */
		// 反注册弱映射，在 EG(weakrefs) 中删除元素
		// key转换成对象指针
		// 用t给指针加码。其实就是添加 tag。
		zend_weakref_unregister(
			zend_weakref_key_to_object(obj_key), ZEND_WEAKREF_ENCODE(&wm->ht, ZEND_WEAKREF_TAG_MAP), 0);
	} ZEND_HASH_FOREACH_END();
	// 销毁哈希表
	zend_hash_destroy(&wm->ht);
	// 使用标准方法 销毁对象
	zend_object_std_dtor(&wm->std);
}

// ing3, 像数组一样读取，p1:被引用对象，p2:偏移量，p3:操作类型，p4:完全没用到
static zval *zend_weakmap_read_dimension(zend_object *object, zval *offset, int type, zval *rv)
{
	// 如果 offset 不存在
	if (offset == NULL) {
		// 报错：无法添加到 WeakMap 里
		zend_throw_error(NULL, "Cannot append to WeakMap");
		return NULL;
	}
	
	// 减少 offset 的引用次数
	ZVAL_DEREF(offset);
	// 如果 offset 类型不是object
	if (Z_TYPE_P(offset) != IS_OBJECT) {
		// 报错，弱引用映射的 键必须是 对象
		zend_type_error("WeakMap key must be an object");
		return NULL;
	}
	// 获取对象的弱映射表
	zend_weakmap *wm = zend_weakmap_from(object);
	// 取得offset中的对象
	zend_object *obj_addr = Z_OBJ_P(offset);
	// 从对象指针获取弱引用key（直接右移3位）
	zval *zv = zend_hash_index_find(&wm->ht, zend_object_to_weakref_key(obj_addr));
	// 如果没找到对应的对象
	if (zv == NULL) {
		// 如果不是isset操作
		if (type != BP_VAR_IS) {
			// 报错：此对象没有包含在弱引用中
			zend_throw_error(NULL,
				"Object %s#%d not contained in WeakMap", ZSTR_VAL(obj_addr->ce->name), obj_addr->handle);
			// 返回null
			return NULL;
		}
		// 返回null
		return NULL;
	}

	// 如果操作是 创建或更新
	if (type == BP_VAR_W || type == BP_VAR_RW) {
		// 返回值转成引用类型
		ZVAL_MAKE_REF(zv);
	}
	// 返回引用 zval
	return zv;
}

// ing2, 像数组一样写入 映射表
static void zend_weakmap_write_dimension(zend_object *object, zval *offset, zval *value)
{
	// 如果没有 offset ，报错
	if (offset == NULL) {
		zend_throw_error(NULL, "Cannot append to WeakMap");
		return;
	}
	// 减少引用次数
	ZVAL_DEREF(offset);
	// 如果 offset 不是对象，报错
	if (Z_TYPE_P(offset) != IS_OBJECT) {
		zend_type_error("WeakMap key must be an object");
		return;
	}
	// 通过对象取得映射表
	zend_weakmap *wm = zend_weakmap_from(object);
	// 取得offset中的对象指针
	zend_object *obj_addr = Z_OBJ_P(offset);
	// 从对象指针获取弱引用key（直接右移3位）
	zend_ulong obj_key = zend_object_to_weakref_key(obj_addr);
	// 增加value引用次数
	Z_TRY_ADDREF_P(value);
	// 从映射表里获取 映射目标
	zval *zv = zend_hash_index_find(&wm->ht, obj_key);
	// 如果找到
	if (zv) {
		// 因为销毁器会有单方面的影响，比如 调整大小 或 rehashing 哈希表。只能在覆盖写入后释放 zval。
		/* Because the destructors can have side effects such as resizing or rehashing the WeakMap storage,
		 * free the zval only after overwriting the original value. */
		zval zv_orig;
		// 先用把zv存到临时变量里
		ZVAL_COPY_VALUE(&zv_orig, zv);
		// 覆盖zv
		ZVAL_COPY_VALUE(zv, value);
		// 销毁 临时变量（旧的zv）
		zval_ptr_dtor(&zv_orig);
		return;
	}
	// 这个还要注册进 EG(weakrefs) 里面，成一个单个弱引用对象？？
	zend_weakref_register(obj_addr, ZEND_WEAKREF_ENCODE(&wm->ht, ZEND_WEAKREF_TAG_MAP));
	// 映射表中
	zend_hash_index_add_new(&wm->ht, obj_key, value);
}

// ing3, 查检映射目标是否存在
/* int return and check_empty due to Object Handler API */
static int zend_weakmap_has_dimension(zend_object *object, zval *offset, int check_empty)
{
	// 减少 offset 的引用次数
	ZVAL_DEREF(offset);
	// 如果 offset 类型不是object
	if (Z_TYPE_P(offset) != IS_OBJECT) {
		// 报错，弱引用映射的 键必须是 对象
		zend_type_error("WeakMap key must be an object");
		return 0;
	}
	// 获取对象的弱映射表
	zend_weakmap *wm = zend_weakmap_from(object);
	// 从对象指针获取弱引用key（直接右移3位）
	// 从哈希表里查找对应的元素
	zval *zv = zend_hash_index_find(&wm->ht, zend_object_to_weakref_key(Z_OBJ_P(offset)));
	// 获取失败
	if (!zv) {
		// 返回false
		return 0;
	}
	// 如果需要检测空
	if (check_empty) {
		// 返回真假
		return i_zend_is_true(zv);
	}
	// 返回类型是否是NULL
	return Z_TYPE_P(zv) != IS_NULL;
}

// ing3, 像数组一样删除弱映射中的元素
static void zend_weakmap_unset_dimension(zend_object *object, zval *offset)
{
	// 减少 offset 的引用次数
	ZVAL_DEREF(offset);
	// 如果 offset 类型不是object
	if (Z_TYPE_P(offset) != IS_OBJECT) {
		// 报错，弱引用映射的 键必须是 对象
		zend_type_error("WeakMap key must be an object");
		return;
	}
	// 获取对象的弱映射表
	zend_weakmap *wm = zend_weakmap_from(object);
	// 取得offset中的对象
	zend_object *obj_addr = Z_OBJ_P(offset);
	// 从对象指针获取弱引用key（直接右移3位）
	// 如果没找到对应的对象
	if (!zend_hash_index_exists(&wm->ht, zend_object_to_weakref_key(obj_addr))) {
		// 如果对象不在弱映射表里，什么也不做
		/* Object not in WeakMap, do nothing. */
		return;
	}
	// 如果有对应的对象
	// 反注册弱映射，并释放目标对象
	zend_weakref_unregister(obj_addr, ZEND_WEAKREF_ENCODE(&wm->ht, ZEND_WEAKREF_TAG_MAP), 1);
}

// ing4, 获取指定对象的 弱映射中的元素个数
static zend_result zend_weakmap_count_elements(zend_object *object, zend_long *count)
{
	zend_weakmap *wm = zend_weakmap_from(object);
	*count = zend_hash_num_elements(&wm->ht);
	return SUCCESS;
}

// ing3, 创建一个新的哈希表，把 object 的弱引用哈希表中的元素全 导到新哈希表里，返回新哈希表
static HashTable *zend_weakmap_get_properties_for(zend_object *object, zend_prop_purpose purpose)
{
	// 使用目的不可以是 调试
	if (purpose != ZEND_PROP_PURPOSE_DEBUG) {
		return NULL;
	}
	// 取得对象的弱映射
	zend_weakmap *wm = zend_weakmap_from(object);
	// 创建并初始化新哈希表
	HashTable *ht;
	ALLOC_HASHTABLE(ht);
	// 销毁器是 ZVAL_PTR_DTOR
	zend_hash_init(ht, zend_hash_num_elements(&wm->ht), NULL, ZVAL_PTR_DTOR, 0);
	//
	zend_ulong obj_key;
	zval *val;
	// 遍历弱引用映射表
	ZEND_HASH_MAP_FOREACH_NUM_KEY_VAL(&wm->ht, obj_key, val) {
		// 取得每一个对象, key转换成对象指针
		zend_object *obj = zend_weakref_key_to_object(obj_key);
		zval pair;
		// #define array_init(arg) ZVAL_ARR((arg), zend_new_array(0))
		// 给pair 创建新哈希表
		array_init(&pair);
		// 对象添加引用次数
		GC_ADDREF(obj);
		// 添加两个元素,obj 的键为 key
		add_assoc_object(&pair, "key", obj);
		// val 添加引用次数
		Z_TRY_ADDREF_P(val);
		// 添加两个元素,val 的键为 value
		add_assoc_zval(&pair, "value", val);
		// 把这个键键对添加到新哈希表里
		zend_hash_next_index_insert_new(ht, &pair);
	} ZEND_HASH_FOREACH_END();
	// 返回新哈希表
	return ht;
}

// ing4, 把对象的 映射表中 所有元素添加到 gc，并返回gc 缓冲区的开头位置和偏移量
static HashTable *zend_weakmap_get_gc(zend_object *object, zval **table, int *n)
{
	// 获取弱映射表
	zend_weakmap *wm = zend_weakmap_from(object);
	// 获取 EG(get_gc_buffer)，游标指向开头
	zend_get_gc_buffer *gc_buffer = zend_get_gc_buffer_create();
	zval *val;
	// 遍历 弱映射表
	ZEND_HASH_MAP_FOREACH_VAL(&wm->ht, val) {
		// 向 gc 回收周期中添加所有元素
		zend_get_gc_buffer_add_zval(gc_buffer, val);
	} ZEND_HASH_FOREACH_END();
	// 返回 EG(get_gc_buffer) 的开始位置（给table）和 游标偏移量（给n）
	zend_get_gc_buffer_use(gc_buffer, table, n);
	return NULL;
}

// ing3, 克隆 对象的弱引用映射表
static zend_object *zend_weakmap_clone_obj(zend_object *old_object)
{
	// 创建新的映射表对象
	zend_object *new_object = zend_weakmap_create_object(zend_ce_weakmap);
	// 从对象指针获取指定对象的弱映射 ，从对象指针，向左移动到 整个 _zend_weakmap 的开头
	zend_weakmap *old_wm = zend_weakmap_from(old_object);
	// 从对象指针获取指定对象的弱映射 ，从对象指针，向左移动到 整个 _zend_weakmap 的开头
	zend_weakmap *new_wm = zend_weakmap_from(new_object);
	// 复制哈希表
	zend_hash_copy(&new_wm->ht, &old_wm->ht, NULL);

	zend_ulong obj_key;
	zval *val;
	// 遍历新哈希表
	ZEND_HASH_MAP_FOREACH_NUM_KEY_VAL(&new_wm->ht, obj_key, val) {
		// 注册弱引用
		// key转换成对象指针
		// 用t给指针加码。其实就是添加 tag。
		zend_weakref_register(
			zend_weakref_key_to_object(obj_key), ZEND_WEAKREF_ENCODE(new_wm, ZEND_WEAKREF_TAG_MAP));
		// 增加引用次数
		zval_add_ref(val);
	} ZEND_HASH_FOREACH_END();
	// 返回 新创建的映射表对象
	return new_object;
}

// ing3, 取得此迭代器的位置指针
static HashPosition *zend_weakmap_iterator_get_pos_ptr(zend_weakmap_iterator *iter) {
	ZEND_ASSERT(iter->ht_iter != (uint32_t) -1);
	// 从运行时迭代器代表中获取指定序号迭代器的pos属性
	return &EG(ht_iterators)[iter->ht_iter].pos;
}

// ing3, 销毁弱引用表迭代器
static void zend_weakmap_iterator_dtor(zend_object_iterator *obj_iter)
{
	// 指针转成 zend_weakmap_iterator
	zend_weakmap_iterator *iter = (zend_weakmap_iterator *) obj_iter;
	// 删除 指定序号的 哈希表迭代器 /Zend/zend_hash.c
	zend_hash_iterator_del(iter->ht_iter);
	// 销毁迭代器的附加数据
	zval_ptr_dtor(&iter->it.data);
}

// ing3, 检查迭代器是否有效。此位置上是否有元素
static int zend_weakmap_iterator_valid(zend_object_iterator *obj_iter)
{
	zend_weakmap_iterator *iter = (zend_weakmap_iterator *) obj_iter;
	// 从 zval 获取指定对象的弱映射
	zend_weakmap *wm = zend_weakmap_fetch(&iter->it.data);
	// 取得此迭代器的位置指针
	HashPosition *pos = zend_weakmap_iterator_get_pos_ptr(iter);
	// 检查此位置上是否有元素
	return zend_hash_has_more_elements_ex(&wm->ht, pos);
}

// ing3, 取得迭代器指向的当前对象
static zval *zend_weakmap_iterator_get_current_data(zend_object_iterator *obj_iter)
{
	zend_weakmap_iterator *iter = (zend_weakmap_iterator *) obj_iter;
	// 从 zval 获取指定对象的弱映射
	zend_weakmap *wm = zend_weakmap_fetch(&iter->it.data);
	// 取得此迭代器的位置指针
	HashPosition *pos = zend_weakmap_iterator_get_pos_ptr(iter);
	// 获取右侧第一个有效元素的值 zend_hash.c
	return zend_hash_get_current_data_ex(&wm->ht, pos);
}

// ing3, 取得迭代器当前指向的key, p1:迭代器，p2:返回值key
static void zend_weakmap_iterator_get_current_key(zend_object_iterator *obj_iter, zval *key)
{
	zend_weakmap_iterator *iter = (zend_weakmap_iterator *) obj_iter;
	// 从 zval 获取指定对象的弱映射
	zend_weakmap *wm = zend_weakmap_fetch(&iter->it.data);
	// 取得此迭代器的位置指针
	HashPosition *pos = zend_weakmap_iterator_get_pos_ptr(iter);

	zend_string *string_key;
	zend_ulong num_key;
	// 获取右侧第一个有效键，根据类型，把键返回在 str_index 或 num_index 变量里
	int key_type = zend_hash_get_current_key_ex(&wm->ht, &string_key, &num_key, pos);
	// key的类型不可以是整数
	if (key_type != HASH_KEY_IS_LONG) {
		ZEND_ASSERT(0 && "Must have integer key");
	}
	// num_key 转换成对象指针，返回给key变量
	ZVAL_OBJ_COPY(key, zend_weakref_key_to_object(num_key));
}

// ing3, 迭代器指向到下一个位置
static void zend_weakmap_iterator_move_forward(zend_object_iterator *obj_iter)
{
	zend_weakmap_iterator *iter = (zend_weakmap_iterator *) obj_iter;
	// 从 zval 获取指定对象的弱映射
	zend_weakmap *wm = zend_weakmap_fetch(&iter->it.data);
	// 取得此迭代器的位置指针
	HashPosition *pos = zend_weakmap_iterator_get_pos_ptr(iter);
	// 向右查找下一个有效位置
	zend_hash_move_forward_ex(&wm->ht, pos);
}

// ing3, 迭代器复位到第一个有效位置
static void zend_weakmap_iterator_rewind(zend_object_iterator *obj_iter)
{
	zend_weakmap_iterator *iter = (zend_weakmap_iterator *) obj_iter;
	// 从 zval 获取指定对象的弱映射
	zend_weakmap *wm = zend_weakmap_fetch(&iter->it.data);
	// 取得此迭代器的位置指针
	HashPosition *pos = zend_weakmap_iterator_get_pos_ptr(iter);
	// 获取第一个有效元素的位置
	zend_hash_internal_pointer_reset_ex(&wm->ht, pos);
}

// ing3, 弱映射哈希表迭代器相关方法
static const zend_object_iterator_funcs zend_weakmap_iterator_funcs = {
	zend_weakmap_iterator_dtor,
	zend_weakmap_iterator_valid,
	zend_weakmap_iterator_get_current_data,
	zend_weakmap_iterator_get_current_key,
	zend_weakmap_iterator_move_forward,
	zend_weakmap_iterator_rewind,
	NULL,
	NULL, /* get_gc */
};

// 通过 迭代器api ，by_ref 是int类型
/* by_ref is int due to Iterator API */
// ing3, 创建弱引用迭代器
static zend_object_iterator *zend_weakmap_get_iterator(
		zend_class_entry *ce, zval *object, int by_ref)
{
	// 从 zval 获取指定对象的弱映射
	zend_weakmap *wm = zend_weakmap_fetch(object);
	// 分配内存创建迭代器
	zend_weakmap_iterator *iter = emalloc(sizeof(zend_weakmap_iterator));
	// 初始化迭代器
	zend_iterator_init(&iter->it);
	// 绑定替代器方法
	iter->it.funcs = &zend_weakmap_iterator_funcs;
	// 把对象复制到迭代器中
	ZVAL_COPY(&iter->it.data, object);
	// 迭代器序号
	iter->ht_iter = zend_hash_iterator_add(&wm->ht, 0);
	// 返回迭代器（它在 zend_weakmap_iterator 开头）
	return &iter->it;
}

// ing3, 骑过偏移量读取
ZEND_METHOD(WeakMap, offsetGet)
{
	zval *key;
	// 解析传入的参数（包含可变参数）p1:参数数量，p2:格式列表，p+:参数列表
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "z", &key) == FAILURE) {
		return;
	}
	// 读取这个键上的元素
	zval *zv = zend_weakmap_read_dimension(Z_OBJ_P(ZEND_THIS), key, BP_VAR_R, NULL);
	// 如果读取失败
	if (!zv) {
		// 中断
		return;
	}
	// 复制 给返回 结果 
	ZVAL_COPY(return_value, zv);
}

// ing3, 通过偏移量写入
ZEND_METHOD(WeakMap, offsetSet)
{
	zval *key, *value;
	// 解析传入的参数（包含可变参数）p1:参数数量，p2:格式列表，p+:参数列表
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "zz", &key, &value) == FAILURE) {
		// 如果失败，中断
		return;
	}
	// 通过偏移量写入
	zend_weakmap_write_dimension(Z_OBJ_P(ZEND_THIS), key, value);
}

// ing3, 检查key是否存在
ZEND_METHOD(WeakMap, offsetExists)
{
	zval *key;
	// 解析传入的参数（包含可变参数）p1:参数数量，p2:格式列表，p+:参数列表
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "z", &key) == FAILURE) {
		// 如果失败，中断
		return;
	}
	// 读取这个键上的元素，只检查存在
	RETURN_BOOL(zend_weakmap_has_dimension(Z_OBJ_P(ZEND_THIS), key, /* check_empty */ 0));
}

// ing3, 删除指定元素
ZEND_METHOD(WeakMap, offsetUnset)
{
	zval *key;
	// 解析传入的参数（包含可变参数）p1:参数数量，p2:格式列表，p+:参数列表
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "z", &key) == FAILURE) {
		// 如果失败，中断
		return;
	}
	// 通过key删除指定元素
	zend_weakmap_unset_dimension(Z_OBJ_P(ZEND_THIS), key);
}

// ing3, 返回表中的元素数量
ZEND_METHOD(WeakMap, count)
{
	// 不可以传入参数
	if (zend_parse_parameters_none() == FAILURE) {
		// 否则直接返回
		return;
	}

	zend_long count;
	// 获取指定对象的 弱映射中的元素个数
	zend_weakmap_count_elements(Z_OBJ_P(ZEND_THIS), &count);
	// 返回整数 count
	RETURN_LONG(count);
}

// ing3, 取回哈希表的迭代器
ZEND_METHOD(WeakMap, getIterator)
{
	// 不可以传入参数
	if (zend_parse_parameters_none() == FAILURE) {
		// 否则直接返回
		return;
	}

	// /Zend/zend_interfaces.c
	// 创建内置迭代器，并关联到对象自己的迭代器
	zend_create_internal_iterator_zval(return_value, ZEND_THIS);
}

// ing4, 注册 WeakReference 和 WeakMap 两个类
void zend_register_weakref_ce(void) /* {{{ */
{
	// 创建 WeakReference 类
	zend_ce_weakref = register_class_WeakReference();
	// 创建对象方法
	zend_ce_weakref->create_object = zend_weakref_new;
	// 先复制标准对象处理器
	memcpy(&zend_weakref_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	// zend_weakref 中的对象 偏移量
	zend_weakref_handlers.offset = XtOffsetOf(zend_weakref, std);
	// 释放方法
	zend_weakref_handlers.free_obj = zend_weakref_free;
	// 没有克隆方法
	zend_weakref_handlers.clone_obj = NULL;
	
	// 注册弱映射表类
	zend_ce_weakmap = register_class_WeakMap(zend_ce_arrayaccess, zend_ce_countable, zend_ce_aggregate);
	// 创建方法
	zend_ce_weakmap->create_object = zend_weakmap_create_object;
	// 获取迭代器方法
	zend_ce_weakmap->get_iterator = zend_weakmap_get_iterator;
	// 使用标准对象处理器作为 弱映射表的处理器
	memcpy(&zend_weakmap_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	// 弱映射表 中对象的偏移器
	zend_weakmap_handlers.offset = XtOffsetOf(zend_weakmap, std);
	// 释放对象方法
	zend_weakmap_handlers.free_obj = zend_weakmap_free_obj;
	// 像数组一样读取的方法
	zend_weakmap_handlers.read_dimension = zend_weakmap_read_dimension;
	// 像数组一样写入的方法
	zend_weakmap_handlers.write_dimension = zend_weakmap_write_dimension;
	// 像数组一样 检查 的方法
	zend_weakmap_handlers.has_dimension = zend_weakmap_has_dimension;
	// 像数组一样 删除 的方法
	zend_weakmap_handlers.unset_dimension = zend_weakmap_unset_dimension;
	// 像数组一样 计数 的方法
	zend_weakmap_handlers.count_elements = zend_weakmap_count_elements;
	// 创建一个新的哈希表，把 object 的弱引用哈希表中的元素全 导到新哈希表里，返回新哈希表
	zend_weakmap_handlers.get_properties_for = zend_weakmap_get_properties_for;
	// 把对象的 映射表中 所有元素添加到 gc，并返回gc 缓冲区的开头位置和偏移量
	zend_weakmap_handlers.get_gc = zend_weakmap_get_gc;
	// 克隆方法
	zend_weakmap_handlers.clone_obj = zend_weakmap_clone_obj;
}
/* }}} */

