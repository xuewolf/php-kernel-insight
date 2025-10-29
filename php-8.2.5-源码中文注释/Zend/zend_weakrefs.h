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

#ifndef ZEND_WEAKREFS_H
#define ZEND_WEAKREFS_H

#include "zend_alloc.h"

BEGIN_EXTERN_C()

extern ZEND_API zend_class_entry *zend_ce_weakref;

void zend_register_weakref_ce(void);

void zend_weakrefs_init(void);
void zend_weakrefs_shutdown(void);

ZEND_API void zend_weakrefs_notify(zend_object *object);

ZEND_API zval *zend_weakrefs_hash_add(HashTable *ht, zend_object *key, zval *pData);
ZEND_API zend_result zend_weakrefs_hash_del(HashTable *ht, zend_object *key);
// ing4, 弱引用哈希表中添加指针
static zend_always_inline void *zend_weakrefs_hash_add_ptr(HashTable *ht, zend_object *key, void *ptr) {
	zval tmp, *zv;
	// 创建一个zval把指针包起来
	ZVAL_PTR(&tmp, ptr);
	// 添加到哈希表中
	if ((zv = zend_weakrefs_hash_add(ht, key, &tmp))) {
		// 如果添加成功，返回指针
		return Z_PTR_P(zv);
	// 失败返回空
	} else {
		return NULL;
	}
}

// 因为php使用原生数字的哈希函数，原生指针会导致哈希冲突
// 这里使用了一个保证：指针的的最低 ZEND_MM_ALIGNED_OFFSET_LOG2 位是0

// 例如 在大多数64位平台上，指针是8bytes，所以最后3位是0，并且可以被丢弃。（这也可以说明，内存中最小的操作单位是byte不是bit）

// 提示：这个函数只给 EG(weakrefs) 和 zend_weakmap->ht使用
// 它不能用于带有 ZEND_WEAKREF_TAG_HT 标记的哈希表实例。（通过 zend_weakref_register 创建的哈希表，带有 ZEND_WEAKREF_ENCODE 标记）
// ZEND_WEAKREF_TAG_HT 实例用于消除 到同一个 zend_object 的多重弱引用 的歧义。

/* Because php uses the raw numbers as a hash function, raw pointers will lead to hash collisions.
 * We have a guarantee that the lowest ZEND_MM_ALIGNED_OFFSET_LOG2 bits of a pointer are zero.
 *
 * E.g. On most 64-bit platforms, pointers are aligned to 8 bytes, so the least significant 3 bits are always 0 and can be discarded.
 *
 * NOTE: This function is only used for EG(weakrefs) and zend_weakmap->ht.
 * It is not used for the HashTable instances associated with ZEND_WEAKREF_TAG_HT tags (created in zend_weakref_register, which uses ZEND_WEAKREF_ENCODE instead).
 * The ZEND_WEAKREF_TAG_HT instances are used to disambiguate between multiple weak references to the same zend_object.
 */
// ing4, 从对象指针获取弱引用key（直接右移3位）。所以这个key的算法，直接保证了哈希表的key不会有冲突，这是最重要的。
static zend_always_inline zend_ulong zend_object_to_weakref_key(const zend_object *object)
{
	// #define ZEND_MM_ALIGNMENT (size_t)8
	// 指必须对齐到 ZEND_MM_ALIGNMENT
	ZEND_ASSERT(((uintptr_t)object) % ZEND_MM_ALIGNMENT == 0);
	// #define ZEND_MM_ALIGNMENT_LOG2 (size_t)3
	// 右移3位
	return ((uintptr_t) object) >> ZEND_MM_ALIGNMENT_LOG2;
}

// ing4, key转换成对象指针
static zend_always_inline zend_object *zend_weakref_key_to_object(zend_ulong key)
{
	// 左移3位
	return (zend_object *) (((uintptr_t) key) << ZEND_MM_ALIGNMENT_LOG2);
}

END_EXTERN_C()

#endif

