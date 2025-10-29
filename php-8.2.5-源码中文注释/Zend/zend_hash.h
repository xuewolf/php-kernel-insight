// 数组确实是php里非常复杂也是非常经典的部分
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

#ifndef ZEND_HASH_H
#define ZEND_HASH_H

#include "zend.h"
#include "zend_sort.h"

//
#define HASH_KEY_IS_STRING 1
#define HASH_KEY_IS_LONG 2
#define HASH_KEY_NON_EXISTENT 3

// 6种操作类型：每次操作可以有多个
// 更新
#define HASH_UPDATE 			(1<<0)
// 添加
#define HASH_ADD				(1<<1)
// 更新间接元素
#define HASH_UPDATE_INDIRECT	(1<<2)
// 添加新元素
#define HASH_ADD_NEW			(1<<3)
// ? 添加next
#define HASH_ADD_NEXT			(1<<4)
// 查找
#define HASH_LOOKUP				(1<<5)

// 哈希标记 
// 冲突
#define HASH_FLAG_CONSISTENCY      ((1<<0) | (1<<1))
// 顺序数组
#define HASH_FLAG_PACKED           (1<<2)
// 未初始化
#define HASH_FLAG_UNINITIALIZED    (1<<3)
// static keys, 整型或内部string。没有非内置字串的字串做key。如果有的话就不是静态。
#define HASH_FLAG_STATIC_KEYS      (1<<4) /* long and interned strings */
// 存在空内部元素
#define HASH_FLAG_HAS_EMPTY_IND    (1<<5)
// 调试模式使用。关闭 HT_ASSERT 校验
#define HASH_FLAG_ALLOW_COW_VIOLATION (1<<6)

// 只有低字节是 real flags ，256。8个比特位
/* Only the low byte are real flags */
#define HASH_FLAG_MASK 0xff

// ing4, 获取哈希表标记
#define HT_FLAGS(ht) (ht)->u.flags

// ing4, 哈希表标记成未初始化
#define HT_INVALIDATE(ht) do { \
		HT_FLAGS(ht) = HASH_FLAG_UNINITIALIZED; \
	} while (0)

// ing4, 哈希表是否初始化过
#define HT_IS_INITIALIZED(ht) \
	((HT_FLAGS(ht) & HASH_FLAG_UNINITIALIZED) == 0)

// ing4, 是否是数字索引数组（flags 里面包含 HASH_FLAG_PACKED）
#define HT_IS_PACKED(ht) \
	((HT_FLAGS(ht) & HASH_FLAG_PACKED) != 0)

// ing4, 是否每个元素都用上了（不包含无效元素）
#define HT_IS_WITHOUT_HOLES(ht) \
	((ht)->nNumUsed == (ht)->nNumOfElements)

// ing4, 是否只包含静态key: 是索引数组 或者 所有key都是保留字
#define HT_HAS_STATIC_KEYS_ONLY(ht) \
	((HT_FLAGS(ht) & (HASH_FLAG_PACKED|HASH_FLAG_STATIC_KEYS)) != 0)

// 调试模式下添加  HASH_FLAG_ALLOW_COW_VIOLATION 标记
#if ZEND_DEBUG
// ing4, 添加标记 HASH_FLAG_ALLOW_COW_VIOLATION
# define HT_ALLOW_COW_VIOLATION(ht) HT_FLAGS(ht) |= HASH_FLAG_ALLOW_COW_VIOLATION
#else
// ing4
# define HT_ALLOW_COW_VIOLATION(ht)
#endif

// ing4, 获取哈希表迭代器数量
#define HT_ITERATORS_COUNT(ht) (ht)->u.v.nIteratorsCount
// ing4, 迭代器数量是否溢出（等于255个）
#define HT_ITERATORS_OVERFLOW(ht) (HT_ITERATORS_COUNT(ht) == 0xff)
// ing4, 哈希表是否包含迭代器
#define HT_HAS_ITERATORS(ht) (HT_ITERATORS_COUNT(ht) != 0)

// ing4, 设置哈希表的迭代器数量
#define HT_SET_ITERATORS_COUNT(ht, iters) \
	do { HT_ITERATORS_COUNT(ht) = (iters); } while (0)

// ing4, 哈希表的迭代器数量 +1
#define HT_INC_ITERATORS_COUNT(ht) \
	HT_SET_ITERATORS_COUNT(ht, HT_ITERATORS_COUNT(ht) + 1)
// ing4, 哈希表的迭代器数量 -1
#define HT_DEC_ITERATORS_COUNT(ht) \
	HT_SET_ITERATORS_COUNT(ht, HT_ITERATORS_COUNT(ht) - 1)

// 空数组
extern ZEND_API const HashTable zend_empty_array;

// ing4, zval设置成空数组
#define ZVAL_EMPTY_ARRAY(z) do {						\
		zval *__z = (z);								\
		Z_ARR_P(__z) = (zend_array*)&zend_empty_array;	\
		Z_TYPE_INFO_P(__z) = IS_ARRAY; \
	} while (0)

// 少量应用
typedef struct _zend_hash_key {
	zend_ulong h;
	zend_string *key;
} zend_hash_key;

typedef bool (*merge_checker_func_t)(HashTable *target_ht, zval *source_data, zend_hash_key *hash_key, void *pParam);

BEGIN_EXTERN_C()

/* startup/shutdown */
ZEND_API void ZEND_FASTCALL _zend_hash_init(HashTable *ht, uint32_t nSize, dtor_func_t pDestructor, bool persistent);
ZEND_API void ZEND_FASTCALL zend_hash_destroy(HashTable *ht);
ZEND_API void ZEND_FASTCALL zend_hash_clean(HashTable *ht);

// ing4, 初始化哈希表，只设置基础属性，未创建元素空间。
// p1:哈希表，p2:元素空间数量，p3:没用到，p4:元素销毁器，p5:是否永久
#define zend_hash_init(ht, nSize, pHashFunction, pDestructor, persistent) \
	/* 初始化哈希表，只设置基础属性，未创建元素空间。p1:哈希表，p2:元素空间数量，p3:元素销毁器，p4:是否永久 */ \
	_zend_hash_init((ht), (nSize), (pDestructor), (persistent))

ZEND_API void ZEND_FASTCALL zend_hash_real_init(HashTable *ht, bool packed);
ZEND_API void ZEND_FASTCALL zend_hash_real_init_packed(HashTable *ht);
ZEND_API void ZEND_FASTCALL zend_hash_real_init_mixed(HashTable *ht);
ZEND_API void ZEND_FASTCALL zend_hash_packed_to_hash(HashTable *ht);
ZEND_API void ZEND_FASTCALL zend_hash_to_packed(HashTable *ht);
ZEND_API void ZEND_FASTCALL zend_hash_extend(HashTable *ht, uint32_t nSize, bool packed);
ZEND_API void ZEND_FASTCALL zend_hash_discard(HashTable *ht, uint32_t nNumUsed);
ZEND_API void ZEND_FASTCALL zend_hash_packed_grow(HashTable *ht);

/* additions/updates/changes */
ZEND_API zval* ZEND_FASTCALL zend_hash_add_or_update(HashTable *ht, zend_string *key, zval *pData, uint32_t flag);
ZEND_API zval* ZEND_FASTCALL zend_hash_update(HashTable *ht, zend_string *key,zval *pData);
ZEND_API zval* ZEND_FASTCALL zend_hash_update_ind(HashTable *ht, zend_string *key,zval *pData);
ZEND_API zval* ZEND_FASTCALL zend_hash_add(HashTable *ht, zend_string *key,zval *pData);
ZEND_API zval* ZEND_FASTCALL zend_hash_add_new(HashTable *ht, zend_string *key,zval *pData);

ZEND_API zval* ZEND_FASTCALL zend_hash_str_add_or_update(HashTable *ht, const char *key, size_t len, zval *pData, uint32_t flag);
ZEND_API zval* ZEND_FASTCALL zend_hash_str_update(HashTable *ht, const char *key, size_t len, zval *pData);
ZEND_API zval* ZEND_FASTCALL zend_hash_str_update_ind(HashTable *ht, const char *key, size_t len, zval *pData);
ZEND_API zval* ZEND_FASTCALL zend_hash_str_add(HashTable *ht, const char *key, size_t len, zval *pData);
ZEND_API zval* ZEND_FASTCALL zend_hash_str_add_new(HashTable *ht, const char *key, size_t len, zval *pData);

ZEND_API zval* ZEND_FASTCALL zend_hash_index_add_or_update(HashTable *ht, zend_ulong h, zval *pData, uint32_t flag);
ZEND_API zval* ZEND_FASTCALL zend_hash_index_add(HashTable *ht, zend_ulong h, zval *pData);
ZEND_API zval* ZEND_FASTCALL zend_hash_index_add_new(HashTable *ht, zend_ulong h, zval *pData);
ZEND_API zval* ZEND_FASTCALL zend_hash_index_update(HashTable *ht, zend_ulong h, zval *pData);
ZEND_API zval* ZEND_FASTCALL zend_hash_next_index_insert(HashTable *ht, zval *pData);
ZEND_API zval* ZEND_FASTCALL zend_hash_next_index_insert_new(HashTable *ht, zval *pData);

ZEND_API zval* ZEND_FASTCALL zend_hash_index_add_empty_element(HashTable *ht, zend_ulong h);
ZEND_API zval* ZEND_FASTCALL zend_hash_add_empty_element(HashTable *ht, zend_string *key);
ZEND_API zval* ZEND_FASTCALL zend_hash_str_add_empty_element(HashTable *ht, const char *key, size_t len);

ZEND_API zval* ZEND_FASTCALL zend_hash_set_bucket_key(HashTable *ht, Bucket *p, zend_string *key);

// 循环时，删除元素
#define ZEND_HASH_APPLY_KEEP				0
// 循环时，保留元素
#define ZEND_HASH_APPLY_REMOVE				1<<0
// 停止循环
#define ZEND_HASH_APPLY_STOP				1<<1

typedef int (*apply_func_t)(zval *pDest);
typedef int (*apply_func_arg_t)(zval *pDest, void *argument);
typedef int (*apply_func_args_t)(zval *pDest, int num_args, va_list args, zend_hash_key *hash_key);

ZEND_API void ZEND_FASTCALL zend_hash_graceful_destroy(HashTable *ht);
ZEND_API void ZEND_FASTCALL zend_hash_graceful_reverse_destroy(HashTable *ht);
ZEND_API void ZEND_FASTCALL zend_hash_apply(HashTable *ht, apply_func_t apply_func);
ZEND_API void ZEND_FASTCALL zend_hash_apply_with_argument(HashTable *ht, apply_func_arg_t apply_func, void *);
ZEND_API void zend_hash_apply_with_arguments(HashTable *ht, apply_func_args_t apply_func, int, ...);

// 这个函数要小心使用（最好别用）。
/* This function should be used with special care (in other words,
 * it should usually not be used).  When used with the ZEND_HASH_APPLY_STOP
 * return value, it assumes things about the order of the elements in the hash.
 * Also, it does not provide the same kind of reentrancy protection that
 * the standard apply functions do.
 */
ZEND_API void ZEND_FASTCALL zend_hash_reverse_apply(HashTable *ht, apply_func_t apply_func);


/* Deletes */
ZEND_API zend_result ZEND_FASTCALL zend_hash_del(HashTable *ht, zend_string *key);
ZEND_API zend_result ZEND_FASTCALL zend_hash_del_ind(HashTable *ht, zend_string *key);
ZEND_API zend_result ZEND_FASTCALL zend_hash_str_del(HashTable *ht, const char *key, size_t len);
ZEND_API zend_result ZEND_FASTCALL zend_hash_str_del_ind(HashTable *ht, const char *key, size_t len);
ZEND_API zend_result ZEND_FASTCALL zend_hash_index_del(HashTable *ht, zend_ulong h);
ZEND_API void ZEND_FASTCALL zend_hash_del_bucket(HashTable *ht, Bucket *p);
ZEND_API void ZEND_FASTCALL zend_hash_packed_del_val(HashTable *ht, zval *zv);

/* Data retrieval */
ZEND_API zval* ZEND_FASTCALL zend_hash_find(const HashTable *ht, zend_string *key);
ZEND_API zval* ZEND_FASTCALL zend_hash_str_find(const HashTable *ht, const char *key, size_t len);
ZEND_API zval* ZEND_FASTCALL zend_hash_index_find(const HashTable *ht, zend_ulong h);
ZEND_API zval* ZEND_FASTCALL _zend_hash_index_find(const HashTable *ht, zend_ulong h);

/* The same as zend_hash_find(), but hash value of the key must be already calculated. */
ZEND_API zval* ZEND_FASTCALL zend_hash_find_known_hash(const HashTable *ht, const zend_string *key);

// ing4, 哈希表里用 key 查找元素。p1:哈希表，p2:key，p3：key上是否有哈希值
static zend_always_inline zval *zend_hash_find_ex(const HashTable *ht, zend_string *key, bool known_hash)
{
	// 如果 key 上带哈希值
	if (known_hash) {
		return zend_hash_find_known_hash(ht, key);
	// 
	} else {
		// 使用 key 获取哈希表的 bucket值。p1:哈希表，p2:key(zend_string)
		return zend_hash_find(ht, key);
	}
}

// ing3, 用数字索引号在哈希表里查找，找不到就 goto p4
#define ZEND_HASH_INDEX_FIND(_ht, _h, _ret, _not_found) do { \
		/* 如果是顺序数组 */ \
		if (EXPECTED(HT_FLAGS(_ht) & HASH_FLAG_PACKED)) { \
			/* 索引号有效 */ \
			if (EXPECTED((zend_ulong)(_h) < (zend_ulong)(_ht)->nNumUsed)) { \
				/* 准备返回这个元素 */ \
				_ret = &_ht->arPacked[_h]; \
				/* 这个元素是 undef, 也算没找到 */ \
				if (UNEXPECTED(Z_TYPE_P(_ret) == IS_UNDEF)) { \
					goto _not_found; \
				} \
			/* 索引号无效，算没找到 */ \
			} else { \
				goto _not_found; \
			} \
		/* 是哈希表 */ \
		} else { \
			/* 用哈希值查找 */ \
			_ret = _zend_hash_index_find(_ht, _h); \
			/* 如果没找到 */ \
			if (UNEXPECTED(_ret == NULL)) { \
				goto _not_found; \
			} \
		} \
	} while (0)


/* Find or add NULL, if doesn't exist */
ZEND_API zval* ZEND_FASTCALL zend_hash_lookup(HashTable *ht, zend_string *key);
ZEND_API zval* ZEND_FASTCALL zend_hash_index_lookup(HashTable *ht, zend_ulong h);

// ing4, 通过哈希值查找元素，先按顺序，再按哈希表。p1:哈希表，p2:哈希值，p3:返回值
#define ZEND_HASH_INDEX_LOOKUP(_ht, _h, _ret) do { \
		/* 如果是顺序数组 */ \
		if (EXPECTED(HT_FLAGS(_ht) & HASH_FLAG_PACKED)) { \
			/* 哈希值 小于 使用数 */ \
			if (EXPECTED((zend_ulong)(_h) < (zend_ulong)(_ht)->nNumUsed)) { \
				/* 顺序数组中的元素 */ \
				_ret = &_ht->arPacked[_h]; \
				/* 如果元素 有效 */ \
				if (EXPECTED(Z_TYPE_P(_ret) != IS_UNDEF)) { \
					/* 找到了，跳出 */ \
					break; \
				} \
			} \
		} \
		/* 没找到 */ \
		/* 通过哈希值查找原素。p1:哈希表，p2:哈希值 */ \
		_ret = zend_hash_index_lookup(_ht, _h); \
	} while (0)

/* Misc */
// ing4, 检验 key 是否存在哈希表中。p1:哈希表，p2:key(zend_string)
static zend_always_inline bool zend_hash_exists(const HashTable *ht, zend_string *key)
{
	// 使用 key 获取哈希表的 bucket值。p1:哈希表，p2:key(zend_string)
	return zend_hash_find(ht, key) != NULL;
}

// ing4, 检验 key 是否在哈希表中。p1:哈希表，p2:key(char *),p3:长度
static zend_always_inline bool zend_hash_str_exists(const HashTable *ht, const char *str, size_t len)
{
	// 使用 键(string) 获取哈希表的 值。p1:哈希表，p2:key(char *),p3:长度
	return zend_hash_str_find(ht, str, len) != NULL;
}

// ing4, 检验 哈希值 是否存在。p1:哈希表，p2:哈希值
static zend_always_inline bool zend_hash_index_exists(const HashTable *ht, zend_ulong h)
{
	// 按哈希值查询，兼容顺序数组。p1:哈希表，p2:哈希值
	return zend_hash_index_find(ht, h) != NULL;
}

/* traversing */
ZEND_API HashPosition ZEND_FASTCALL zend_hash_get_current_pos(const HashTable *ht);

ZEND_API zend_result   ZEND_FASTCALL zend_hash_move_forward_ex(HashTable *ht, HashPosition *pos);
ZEND_API zend_result   ZEND_FASTCALL zend_hash_move_backwards_ex(HashTable *ht, HashPosition *pos);
ZEND_API int   ZEND_FASTCALL zend_hash_get_current_key_ex(const HashTable *ht, zend_string **str_index, zend_ulong *num_index, const HashPosition *pos);
ZEND_API void  ZEND_FASTCALL zend_hash_get_current_key_zval_ex(const HashTable *ht, zval *key, const HashPosition *pos);
ZEND_API int   ZEND_FASTCALL zend_hash_get_current_key_type_ex(HashTable *ht, HashPosition *pos);
ZEND_API zval* ZEND_FASTCALL zend_hash_get_current_data_ex(HashTable *ht, HashPosition *pos);
ZEND_API void  ZEND_FASTCALL zend_hash_internal_pointer_reset_ex(HashTable *ht, HashPosition *pos);
ZEND_API void  ZEND_FASTCALL zend_hash_internal_pointer_end_ex(HashTable *ht, HashPosition *pos);

// 以下是给外部调用的封装
// ing4, 检查此位置后面是否还有元素 。p1:哈希表，p2:位置
static zend_always_inline zend_result zend_hash_has_more_elements_ex(HashTable *ht, HashPosition *pos) {
	// 获取位置右侧第一个元素的 key 的类型（数字/字串/不存在，只有哈希值算数字）。p1:哈希表，p2:位置
	return (zend_hash_get_current_key_type_ex(ht, pos) == HASH_KEY_NON_EXISTENT ? FAILURE : SUCCESS);
}

// ing4, 检查是否 内置指针位置后面是否还有元素。p1:哈希表
static zend_always_inline zend_result zend_hash_has_more_elements(HashTable *ht) {
	// 检查此位置后面是否还有元素 。p1:哈希表，p2:位置, 也在这里返回找到的编号
	return zend_hash_has_more_elements_ex(ht, &ht->nInternalPointer);
}

// ing4, 向右查找 内置指针后面 下一个有效位置。p1:哈希表
static zend_always_inline zend_result zend_hash_move_forward(HashTable *ht) {
	// 向右查找下一个有效位置。p1:哈希表，p2:位置
	return zend_hash_move_forward_ex(ht, &ht->nInternalPointer);
}

// ing4, 找到 内置指针 左边第一个有效元素的位置。p1:哈希表
static zend_always_inline zend_result zend_hash_move_backwards(HashTable *ht) {
	// 找到 p1 左边第一个有效元素的位置。p1:哈希表，p2:给出位置, 也在这里返回找到的编号
	return zend_hash_move_backwards_ex(ht, &ht->nInternalPointer);
}

// ing4, 获取 内置指针 右侧第一个有效键，根据类型，把键返回在 str_index 或 num_index 变量里。
// p1:哈希表，p2:key（返回值），p3:哈希值（返回值）
static zend_always_inline int zend_hash_get_current_key(const HashTable *ht, zend_string **str_index, zend_ulong *num_index) {
	// 获取右侧第一个有效键，根据类型，把键返回在 str_index 或 num_index 变量里。
	// p1:哈希表，p2:key（返回值），p3:哈希值（返回值），p4:位置
	return zend_hash_get_current_key_ex(ht, str_index, num_index, &ht->nInternalPointer);
}

// ing4, 获取右侧第一个有效键。p1:哈希表，p2:返回key
static zend_always_inline void zend_hash_get_current_key_zval(const HashTable *ht, zval *key) {
	// 获取右侧第一个有效键。p1:哈希表，p2:返回key，p3:位置
	zend_hash_get_current_key_zval_ex(ht, key, &ht->nInternalPointer);
}

// ing4, 获取 内置指针 右侧第一个元素的 key 的类型（数字/字串/不存在，只有哈希值算数字）。p1:哈希表
static zend_always_inline int zend_hash_get_current_key_type(HashTable *ht) {
	// 获取位置右侧第一个元素的 key 的类型（数字/字串/不存在，只有哈希值算数字）。p1:哈希表，p2:位置
	return zend_hash_get_current_key_type_ex(ht, &ht->nInternalPointer);
}

// ing4, 获取右侧第一个有效元素的值。p1:哈希表
static zend_always_inline zval* zend_hash_get_current_data(HashTable *ht) {
	// 获取右侧第一个有效元素的值。p1:哈希表，p2:位置
	return zend_hash_get_current_data_ex(ht, &ht->nInternalPointer);
}

// ing4, 获取第一个有效元素的位置。p1:哈希表
static zend_always_inline void zend_hash_internal_pointer_reset(HashTable *ht) {
	// 获取第一个有效元素的位置。p1:哈希表，p2:位置（返回）
	zend_hash_internal_pointer_reset_ex(ht, &ht->nInternalPointer);
}

// ing4, 查找最后一个有效元素的位置。p1:哈希表
static zend_always_inline void zend_hash_internal_pointer_end(HashTable *ht) {
	// 查找最后一个有效元素的位置。p1:哈希表，p2:位置（返回）
	zend_hash_internal_pointer_end_ex(ht, &ht->nInternalPointer);
}

/* Copying, merging and sorting */
ZEND_API void  ZEND_FASTCALL zend_hash_copy(HashTable *target, HashTable *source, copy_ctor_func_t pCopyConstructor);
ZEND_API void  ZEND_FASTCALL zend_hash_merge(HashTable *target, HashTable *source, copy_ctor_func_t pCopyConstructor, bool overwrite);
ZEND_API void  ZEND_FASTCALL zend_hash_merge_ex(HashTable *target, HashTable *source, copy_ctor_func_t pCopyConstructor, merge_checker_func_t pMergeSource, void *pParam);
ZEND_API void  zend_hash_bucket_swap(Bucket *p, Bucket *q);
ZEND_API void  zend_hash_bucket_renum_swap(Bucket *p, Bucket *q);
ZEND_API void  zend_hash_bucket_packed_swap(Bucket *p, Bucket *q);

typedef int (*bucket_compare_func_t)(Bucket *a, Bucket *b);
ZEND_API int   zend_hash_compare(HashTable *ht1, HashTable *ht2, compare_func_t compar, bool ordered);
ZEND_API void  ZEND_FASTCALL zend_hash_sort_ex(HashTable *ht, sort_func_t sort_func, bucket_compare_func_t compare_func, bool renumber);
ZEND_API zval* ZEND_FASTCALL zend_hash_minmax(const HashTable *ht, compare_func_t compar, uint32_t flag);

// ing4，转成哈希表，记录原始序号，使用给定的方法比较，排序。如果需要重排序号，转成顺序数组，否则转成哈希表。
// p1:哈希表，p2:比较函数，p3:是否需要重新排序
static zend_always_inline void ZEND_FASTCALL zend_hash_sort(HashTable *ht, bucket_compare_func_t compare_func, zend_bool renumber) {
	// 转成哈希表，记录原始序号，使用给定的方法比较，排序。如果需要重排序号，转成顺序数组，否则转成哈希表。
	// p1:哈希表，p2:排序函数，p3:比较函数，p4:是否需要重新排序
	zend_hash_sort_ex(ht, zend_sort, compare_func, renumber);
}

// clear, 返回 哈希表元素数量
static zend_always_inline uint32_t zend_hash_num_elements(const HashTable *ht) {
	return ht->nNumOfElements;
}

// clear, 返回 哈希表下一个空元素指针
static zend_always_inline zend_long zend_hash_next_free_element(const HashTable *ht) {
	return ht->nNextFreeElement;
}

ZEND_API void ZEND_FASTCALL zend_hash_rehash(HashTable *ht);

// ？ 如果不是调试状态，并且有内置常量指针。主要是调用 _zend_new_array
#if !ZEND_DEBUG && defined(HAVE_BUILTIN_CONSTANT_P)
// ing2
# define zend_new_array(size) \
	(__builtin_constant_p(size) ? \
		((((uint32_t)(size)) <= HT_MIN_SIZE) ? \
			_zend_new_array_0() \
		: \
			_zend_new_array((size)) \
		) \
	: \
		_zend_new_array((size)) \
	)
// 
#else
// clear
# define zend_new_array(size) \
	_zend_new_array(size)
#endif

ZEND_API HashTable* ZEND_FASTCALL _zend_new_array_0(void);
ZEND_API HashTable* ZEND_FASTCALL _zend_new_array(uint32_t size);
ZEND_API HashTable* ZEND_FASTCALL zend_new_pair(zval *val1, zval *val2);
ZEND_API uint32_t zend_array_count(HashTable *ht);
ZEND_API HashTable* ZEND_FASTCALL zend_array_dup(HashTable *source);
ZEND_API void ZEND_FASTCALL zend_array_destroy(HashTable *ht);
ZEND_API HashTable* zend_array_to_list(HashTable *source);
ZEND_API void ZEND_FASTCALL zend_symtable_clean(HashTable *ht);
ZEND_API HashTable* ZEND_FASTCALL zend_symtable_to_proptable(HashTable *ht);
ZEND_API HashTable* ZEND_FASTCALL zend_proptable_to_symtable(HashTable *ht, bool always_duplicate);

ZEND_API bool ZEND_FASTCALL _zend_handle_numeric_str_ex(const char *key, size_t length, zend_ulong *idx);

ZEND_API uint32_t     ZEND_FASTCALL zend_hash_iterator_add(HashTable *ht, HashPosition pos);
ZEND_API HashPosition ZEND_FASTCALL zend_hash_iterator_pos(uint32_t idx, HashTable *ht);
ZEND_API HashPosition ZEND_FASTCALL zend_hash_iterator_pos_ex(uint32_t idx, zval *array);
ZEND_API void         ZEND_FASTCALL zend_hash_iterator_del(uint32_t idx);
ZEND_API HashPosition ZEND_FASTCALL zend_hash_iterators_lower_pos(HashTable *ht, HashPosition start);
ZEND_API void         ZEND_FASTCALL _zend_hash_iterators_update(HashTable *ht, HashPosition from, HashPosition to);
ZEND_API void         ZEND_FASTCALL zend_hash_iterators_advance(HashTable *ht, HashPosition step);

// ing3, 更新哈希表的迭代器
static zend_always_inline void zend_hash_iterators_update(HashTable *ht, HashPosition from, HashPosition to)
{
	// 检查是否有迭代器存在
	if (UNEXPECTED(HT_HAS_ITERATORS(ht))) {
		// 更新哈希表的迭代器
		_zend_hash_iterators_update(ht, from, to);
	}
}

// ing3, 删除顺序数组
/* For regular arrays (non-persistent, storing zvals). */
static zend_always_inline void zend_array_release(zend_array *array)
{
	if (!(GC_FLAGS(array) & IS_ARRAY_IMMUTABLE)) {
		if (GC_DELREF(array) == 0) {
			zend_array_destroy(array);
		}
	}
	// 这里没有销毁 array 
}

// ing3, 针对一般的哈希表, 销毁哈希表
/* For general hashes (possibly persistent, storing any kind of value). */
static zend_always_inline void zend_hash_release(zend_array *array)
{
	// 如果不是内置变量数组（ IS_ARRAY_IMMUTABLE 属于特殊类型）
	if (!(GC_FLAGS(array) & IS_ARRAY_IMMUTABLE)) {
		// 先删除引用次数，如果次数为0
		if (GC_DELREF(array) == 0) {
			// 删除 哈希表
			zend_hash_destroy(array);
			// 释放内存
			pefree(array, GC_FLAGS(array) & IS_ARRAY_PERSISTENT);
		}
	}
}

END_EXTERN_C()

// ing4, 初始化 8个元素，非永久 哈希表，只设置基础属性，未创建元素空间。p1:哈希表
#define ZEND_INIT_SYMTABLE(ht)								\
	/* 初始化哈希表，只设置基础属性，未创建元素空间。p1:哈希表，p2:元素空间数量，p3:是否永久 */ \
	ZEND_INIT_SYMTABLE_EX(ht, 8, 0)

// ing4, 初始化哈希表，只设置基础属性，未创建元素空间。p1:哈希表，p2:元素空间数量，p3:是否永久
#define ZEND_INIT_SYMTABLE_EX(ht, n, persistent)			\
	/* 初始化哈希表，只设置基础属性，未创建元素空间。*/ \
	/* p1:哈希表，p2:元素空间数量，p3:没用到，p4:元素销毁器，p5:是否永久 */ \
	zend_hash_init(ht, n, NULL, ZVAL_PTR_DTOR, persistent)

// ing4, 检查是否是数字字串。p1:字串，p2:字串长度，p3:返回转换好的整数
static zend_always_inline bool _zend_handle_numeric_str(const char *key, size_t length, zend_ulong *idx)
{
	// 先进行简单检查
	const char *tmp = key;
	// 如果第一个字符太大，返回false
	if (EXPECTED(*tmp > '9')) {
		return 0;
	// 第一个字符太小
	} else if (*tmp < '0') {
		// 如果不是负号，返回false
		if (*tmp != '-') {
			return 0;
		}
		// 下一个字符
		tmp++;
		// 非法，返回 false
		if (*tmp > '9' || *tmp < '0') {
			return 0;
		}
	}
	// 通过简单检查
	// 检查是否是纯数字(0-9)字串，不允许0开头。p1:字串，p2:字串长度，p3:返回转换好的整数
	return _zend_handle_numeric_str_ex(key, length, idx);
}

// ing4, 检查是否是数字字串。p1:字串，p2:字串长度，p3:返回转换好的整数
#define ZEND_HANDLE_NUMERIC_STR(key, length, idx) \
	/* 检查是否是数字字串。p1:字串，p2:字串长度，p3:返回转换好的整数 */ \
	_zend_handle_numeric_str(key, length, &idx)

// ing4, 检查是否是数字字串。p1:字串，p2:返回转换好的整数
#define ZEND_HANDLE_NUMERIC(key, idx) \
	/* 检查是否是数字字串。p1:字串，p2:字串长度，p3:返回转换好的整数 */ \
	ZEND_HANDLE_NUMERIC_STR(ZSTR_VAL(key), ZSTR_LEN(key), idx)

// ing3, 查找并追踪间接引用。p1:哈希表，p2:key
static zend_always_inline zval *zend_hash_find_ind(const HashTable *ht, zend_string *key)
{
	zval *zv;
	// 使用 key 获取哈希表的 bucket值。p1:哈希表，p2:key(zend_string)
	zv = zend_hash_find(ht, key);
	return (zv && Z_TYPE_P(zv) == IS_INDIRECT) ?
		((Z_TYPE_P(Z_INDIRECT_P(zv)) != IS_UNDEF) ? Z_INDIRECT_P(zv) : NULL) : zv;
}

// ing4, 查找间接引用元素
static zend_always_inline zval *zend_hash_find_ex_ind(const HashTable *ht, zend_string *key, bool known_hash)
{
	zval *zv;
	// 哈希表里用 key 查找元素。p1:哈希表，p2:key，p3：key上是否有哈希值
	zv = zend_hash_find_ex(ht, key, known_hash);
	// 如果元素是间接引用, 返回引用对象，否则返回当前元素
	return (zv && Z_TYPE_P(zv) == IS_INDIRECT) ?
		((Z_TYPE_P(Z_INDIRECT_P(zv)) != IS_UNDEF) ? Z_INDIRECT_P(zv) : NULL) : zv;
}

// ing4, 检验间接引用的元素是否有效。p1:哈希表，p2:key
static zend_always_inline bool zend_hash_exists_ind(const HashTable *ht, zend_string *key)
{
	zval *zv;
	// 使用 key 获取哈希表的 bucket值。p1:哈希表，p2:key(zend_string)
	zv = zend_hash_find(ht, key);
	// zv 一定要有效，（ zv不是间接引用，或者间接引用对象不是IS_UNDEF  ）
	return zv && (Z_TYPE_P(zv) != IS_INDIRECT ||
			Z_TYPE_P(Z_INDIRECT_P(zv)) != IS_UNDEF);
}

// ing3 查找并处理间接引用
static zend_always_inline zval *zend_hash_str_find_ind(const HashTable *ht, const char *str, size_t len)
{
	zval *zv;
	// 使用 键(string) 获取哈希表的 值。p1:哈希表，p2:key(char *),p3:长度
	zv = zend_hash_str_find(ht, str, len);
	// 如果返回变量是间接引用
	return (zv && Z_TYPE_P(zv) == IS_INDIRECT) ?
		// 如果引用对象存在，返回引用对象，否则 null
		((Z_TYPE_P(Z_INDIRECT_P(zv)) != IS_UNDEF) ? Z_INDIRECT_P(zv) : NULL) : zv;
}

// ing3, 检查key是否存在，如果是间接引用，检查引用对象是否是 IS_UNDEF
static zend_always_inline bool zend_hash_str_exists_ind(const HashTable *ht, const char *str, size_t len)
{
	zval *zv;
	// 使用 键(string) 获取哈希表的 值。p1:哈希表，p2:key(char *),p3:长度
	zv = zend_hash_str_find(ht, str, len);
	return zv && (Z_TYPE_P(zv) != IS_INDIRECT ||
			Z_TYPE_P(Z_INDIRECT_P(zv)) != IS_UNDEF);
}

// ing4, symtable 中添加新元素
static zend_always_inline zval *zend_symtable_add_new(HashTable *ht, zend_string *key, zval *pData)
{
	zend_ulong idx;
	// 检查是否是数字字串。p1:字串，p2:返回转换好的整数
	if (ZEND_HANDLE_NUMERIC(key, idx)) {
		// 顺序数组，碰到旧的新增。哈希表不查找旧原素，直接添加新元素。p1:哈希表，p2:哈希值，p3:元素
		return zend_hash_index_add_new(ht, idx, pData);
	// 哈希表
	} else {
		// （强转哈希表）不查检，直写入指定位置元素。p1:哈希表，p2:key，p3:值
		return zend_hash_add_new(ht, key, pData);
	}
}

// ing3, 更新哈希表，数字字串当成数字
static zend_always_inline zval *zend_symtable_update(HashTable *ht, zend_string *key, zval *pData)
{
	zend_ulong idx;
	// 检查是否是数字字串。p1:字串，p2:返回转换好的整数
	if (ZEND_HANDLE_NUMERIC(key, idx)) {
		// 找到旧原素都更新。找不到旧原素都添加，数组会转成哈希表。p1:哈希表，p2:哈希值，p3:元素
		return zend_hash_index_update(ht, idx, pData);
	} else {
		// （强转哈希表）不管找不到到原素，都更新指定位置的元素。p1:哈希表，p2:key，p3:值
		return zend_hash_update(ht, key, pData);
	}
}

// ing4, 更新哈希表，数字字串当成数字。p1:哈希表，p2:key（数字时当成哈希值），p3:值
static zend_always_inline zval *zend_symtable_update_ind(HashTable *ht, zend_string *key, zval *pData)
{
	zend_ulong idx;
	// 如果key是数字,当成哈希值
	// 检查是否是数字字串。p1:字串，p2:返回转换好的整数
	if (ZEND_HANDLE_NUMERIC(key, idx)) {
		// 找到旧原素都更新。找不到旧原素都添加，数组会转成哈希表。p1:哈希表，p2:哈希值，p3:元素
		return zend_hash_index_update(ht, idx, pData);
	// 用key更新
	} else {
		// （强转哈希表）不管找不到到原素，都更新指定位置的元素。如果原素是间接引用，解引用。p1:哈希表，p2:key，p3:值
		return zend_hash_update_ind(ht, key, pData);
	}
}

// ing4, 删除指定的键值对。p1:哈希表，p2:key（数字字串当成哈希值）
static zend_always_inline zend_result zend_symtable_del(HashTable *ht, zend_string *key)
{
	zend_ulong idx;
	// 检查是否是数字字串。p1:字串，p2:返回转换好的整数
	if (ZEND_HANDLE_NUMERIC(key, idx)) {
		// 删除指定（哈希值）的键值对。p1:哈希表，p2:哈希值
		return zend_hash_index_del(ht, idx);
	} else {
		// 通过key，删除哈希表中指定元素
		return zend_hash_del(ht, key);
	}
}

// ing4, 删除指定的键值对。p1:哈希表，p2:key（数字字串当成哈希值）
static zend_always_inline zend_result zend_symtable_del_ind(HashTable *ht, zend_string *key)
{
	zend_ulong idx;
	// 检查是否是数字字串。p1:字串，p2:返回转换好的整数
	if (ZEND_HANDLE_NUMERIC(key, idx)) {
		// 删除指定（哈希值）的键值对。p1:哈希表，p2:哈希值
		return zend_hash_index_del(ht, idx);
	} else {
		// 删除哈希表中的元素，包含处理间接引用（附加 HASH_FLAG_HAS_EMPTY_IND 标记）。p1:哈希表，p2:key
		return zend_hash_del_ind(ht, key);
	}
}

// ing4, 按哈希值查询，兼容顺序数组。p1:哈希表，p2:key（数字字串当成哈希值）
static zend_always_inline zval *zend_symtable_find(const HashTable *ht, zend_string *key)
{
	zend_ulong idx;
	// 检查是否是数字字串。p1:字串，p2:返回转换好的整数
	if (ZEND_HANDLE_NUMERIC(key, idx)) {
		// 按哈希值查询，兼容顺序数组。p1:哈希表，p2:哈希值
		return zend_hash_index_find(ht, idx);
	} else {
		// 使用 key 获取哈希表的 bucket值。p1:哈希表，p2:key(zend_string)
		return zend_hash_find(ht, key);
	}
}

// ing4, 按哈希值查询，兼容顺序数组（【哈希表】追踪间接引用）。p1:哈希表，p2:key（数字字串当成哈希值）
static zend_always_inline zval *zend_symtable_find_ind(const HashTable *ht, zend_string *key)
{
	zend_ulong idx;
	// 检查是否是数字字串。p1:字串，p2:返回转换好的整数
	if (ZEND_HANDLE_NUMERIC(key, idx)) {
		// 按哈希值查询，兼容顺序数组。p1:哈希表，p2:哈希值
		return zend_hash_index_find(ht, idx);
	} else {
		// 查找并追踪间接引用。p1:哈希表，p2:key
		return zend_hash_find_ind(ht, key);
	}
}

// ing4, 检验 哈希值 是否存在。p1:哈希表，p2:key（数字字串当成哈希值）
static zend_always_inline bool zend_symtable_exists(HashTable *ht, zend_string *key)
{
	zend_ulong idx;
	// 检查是否是数字字串。p1:字串，p2:返回转换好的整数
	if (ZEND_HANDLE_NUMERIC(key, idx)) {
		// 检验 哈希值 是否存在。p1:哈希表，p2:哈希值
		return zend_hash_index_exists(ht, idx);
	} else {
		// 检验 key 是否存在哈希表中。p1:哈希表，p2:key(zend_string)
		return zend_hash_exists(ht, key);
	}
}

// ing4, 检验 key 是否存在。p1:哈希表，p2:key（数字字串当成哈希值）
static zend_always_inline bool zend_symtable_exists_ind(HashTable *ht, zend_string *key)
{
	zend_ulong idx;
	// 检查是否是数字字串。p1:字串，p2:返回转换好的整数
	if (ZEND_HANDLE_NUMERIC(key, idx)) {
		// 检验 哈希值 是否存在。p1:哈希表，p2:哈希值
		return zend_hash_index_exists(ht, idx);
	} else {
		// 检验间接引用的元素是否有效。p1:哈希表，p2:key
		return zend_hash_exists_ind(ht, key);
	}
}

// ing4, 更新符号表。p1:哈希表，p2:key（数字字串当成哈希值）, p3:key长度, p4:新值
static zend_always_inline zval *zend_symtable_str_update(HashTable *ht, const char *str, size_t len, zval *pData)
{
	zend_ulong idx;
	// 如果是数字字串
	if (ZEND_HANDLE_NUMERIC_STR(str, len, idx)) {
		// 找到旧原素都更新。找不到旧原素都添加，数组会转成哈希表。p1:哈希表，p2:哈希值，p3:元素
		return zend_hash_index_update(ht, idx, pData);
	} else {
		// （强转哈希表）更新，有旧原素时替换旧原素，否则写入新元素。p1:哈希表，p2:key, p3:key长度, p4:新值
		return zend_hash_str_update(ht, str, len, pData);
	}
}

// ing4, 更新符号表（【哈希表】追踪间接引用）。p1:哈希表，p2:key（数字字串当成哈希值）, p3:key长度, p4:新值
static zend_always_inline zval *zend_symtable_str_update_ind(HashTable *ht, const char *str, size_t len, zval *pData)
{
	zend_ulong idx;
	// 如果是数字字串
	if (ZEND_HANDLE_NUMERIC_STR(str, len, idx)) {
		// 找到旧原素都更新。找不到旧原素都添加，数组会转成哈希表。p1:哈希表，p2:哈希值，p3:元素
		return zend_hash_index_update(ht, idx, pData);
	} else {
		//（强转哈希表）更新，有旧原素时，追踪引用目标，替换旧原素。没有旧原素写入新元素
		// p1:哈希表，p2:key, p3:key长度, p4:新值
		return zend_hash_str_update_ind(ht, str, len, pData);
	}
}

// ing4, 删除哈希表指定元素。p1:哈希表，p2:key（数字字串当成哈希值），p3:长度
static zend_always_inline zend_result zend_symtable_str_del(HashTable *ht, const char *str, size_t len)
{
	zend_ulong idx;
	// 如果是数字字串
	if (ZEND_HANDLE_NUMERIC_STR(str, len, idx)) {
		// 删除指定（哈希值）的键值对。p1:哈希表，p2:哈希值
		return zend_hash_index_del(ht, idx);
	} else {
		// 哈希表中删除指定键。p1:哈希表，p2:key，p3:长度
		return zend_hash_str_del(ht, str, len);
	}
}

// ing4, 删除元素（【哈希表】追踪间接引用）。p1:哈希表，p2:key（数字字串当成哈希值），p3:长度
static zend_always_inline zend_result zend_symtable_str_del_ind(HashTable *ht, const char *str, size_t len)
{
	zend_ulong idx;
	// 如果是数字字串
	if (ZEND_HANDLE_NUMERIC_STR(str, len, idx)) {
		// 删除指定（哈希值）的键值对。p1:哈希表，p2:哈希值
		return zend_hash_index_del(ht, idx);
	} else {
		// 删除元素并处理间接引用。p1:哈希表，p2:key，p3:长度
		return zend_hash_str_del_ind(ht, str, len);
	}
}

// ing4, 在符号表中查找。p1:哈希表，p2:key（数字字串当成哈希值）,p3:长度
static zend_always_inline zval *zend_symtable_str_find(HashTable *ht, const char *str, size_t len)
{
	zend_ulong idx;
	// 如果是数字字串
	if (ZEND_HANDLE_NUMERIC_STR(str, len, idx)) {
		// 按哈希值查询，兼容顺序数组。p1:哈希表，p2:哈希值
		return zend_hash_index_find(ht, idx);
	} else {
		// 使用 键(string) 获取哈希表的 值。p1:哈希表，p2:key(char *),p3:长度
		return zend_hash_str_find(ht, str, len);
	}
}

// ing4, 检验 key 是否在哈希表中。p1:哈希表，p2:key（数字字串当成哈希值）,p3:长度
static zend_always_inline bool zend_symtable_str_exists(HashTable *ht, const char *str, size_t len)
{
	zend_ulong idx;
	// 如果是数字字串
	if (ZEND_HANDLE_NUMERIC_STR(str, len, idx)) {
		// 检验 哈希值 是否存在。p1:哈希表，p2:哈希值
		return zend_hash_index_exists(ht, idx);
	} else {
		// 检验 key 是否在哈希表中。p1:哈希表，p2:key(char *),p3:长度
		return zend_hash_str_exists(ht, str, len);
	}
}

// ing4, 哈希表中添加指针元素。p1:哈希表，p2:key，p3:指针
static zend_always_inline void *zend_hash_add_ptr(HashTable *ht, zend_string *key, void *pData)
{
	zval tmp, *zv;
	// 指针复制到 zval里
	ZVAL_PTR(&tmp, pData);
	// 添加到哈希表中
	zv = zend_hash_add(ht, key, &tmp);
	// 如果添加成功
	if (zv) {
		ZEND_ASSUME(Z_PTR_P(zv));
		// 返回新添加的指针
		return Z_PTR_P(zv);
	// 失败
	} else {
		// 返回null
		return NULL;
	}
}

// ing4,（强转哈希表）不查检，直写入指定位置元素，返回插入的原素。p1:哈希表，p2:key，p3:值
static zend_always_inline void *zend_hash_add_new_ptr(HashTable *ht, zend_string *key, void *pData)
{
	zval tmp, *zv;

	ZVAL_PTR(&tmp, pData);
	// （强转哈希表）不查检，直写入指定位置元素。p1:哈希表，p2:key，p3:值
	zv = zend_hash_add_new(ht, key, &tmp);
	if (zv) {
		ZEND_ASSUME(Z_PTR_P(zv));
		return Z_PTR_P(zv);
	} else {
		return NULL;
	}
}

// ing4,（强转哈希表）添加，返回插入的原素，碰到已有原素返回null。否则添加。p1:哈希表，p2:key, p3:key长度, p4:新值
static zend_always_inline void *zend_hash_str_add_ptr(HashTable *ht, const char *str, size_t len, void *pData)
{
	zval tmp, *zv;

	ZVAL_PTR(&tmp, pData);
	// （强转哈希表）添加，碰到已有原素返回null。否则添加。p1:哈希表，p2:key, p3:key长度, p4:新值
	zv = zend_hash_str_add(ht, str, len, &tmp);
	if (zv) {
		ZEND_ASSUME(Z_PTR_P(zv));
		return Z_PTR_P(zv);
	} else {
		return NULL;
	}
}

// ing4,（强转哈希表）添加，不检查已有，直接找到位置，写入元素，返回插入的原素，碰到已有原素返回null。
// p1:哈希表，p2:key, p3:key长度, p4:新值
static zend_always_inline void *zend_hash_str_add_new_ptr(HashTable *ht, const char *str, size_t len, void *pData)
{
	zval tmp, *zv;

	ZVAL_PTR(&tmp, pData);
	//（强转哈希表）添加，不检查已有，直接找到位置，写入元素。p1:哈希表，p2:key, p3:key长度, p4:新值
	zv = zend_hash_str_add_new(ht, str, len, &tmp);
	if (zv) {
		ZEND_ASSUME(Z_PTR_P(zv));
		return Z_PTR_P(zv);
	} else {
		return NULL;
	}
}

// ing4,（强转哈希表）不管找不到到原素，都更新指定位置的元素，返回更新结果。p1:哈希表，p2:key，p3:值
static zend_always_inline void *zend_hash_update_ptr(HashTable *ht, zend_string *key, void *pData)
{
	zval tmp, *zv;

	ZVAL_PTR(&tmp, pData);
	// （强转哈希表）不管找不到到原素，都更新指定位置的元素。p1:哈希表，p2:key，p3:值
	zv = zend_hash_update(ht, key, &tmp);
	ZEND_ASSUME(Z_PTR_P(zv));
	// 返回 zval 中的指针
	return Z_PTR_P(zv);
}

// ing4，（强转哈希表）更新，有旧原素时替换旧原素，否则写入新元素。返回更新结果。
// p1:哈希表，p2:key, p3:key长度, p4:新值
static zend_always_inline void *zend_hash_str_update_ptr(HashTable *ht, const char *str, size_t len, void *pData)
{
	zval tmp, *zv;

	ZVAL_PTR(&tmp, pData);
	// （强转哈希表）更新，有旧原素时替换旧原素，否则写入新元素。p1:哈希表，p2:key, p3:key长度, p4:新值
	zv = zend_hash_str_update(ht, str, len, &tmp);
	ZEND_ASSUME(Z_PTR_P(zv));
	return Z_PTR_P(zv);
}

// ing4, 向hash表中添加内存 副本，返回副本指针。p1:哈希表，p2:key，p3:内存指针，p4:内存大小
static zend_always_inline void *zend_hash_add_mem(HashTable *ht, zend_string *key, void *pData, size_t size)
{
	zval tmp, *zv;

	ZVAL_PTR(&tmp, NULL);
	// 向hash表中添加 空指针元素
	if ((zv = zend_hash_add(ht, key, &tmp))) {
		// 创建新 副本内存
		Z_PTR_P(zv) = pemalloc(size, GC_FLAGS(ht) & IS_ARRAY_PERSISTENT);
		// 把数据复制到新内存中
		memcpy(Z_PTR_P(zv), pData, size);
		// 返回值的指针
		return Z_PTR_P(zv);
	}
	// 如果添加失败，返回null
	return NULL;
}

// ing4,（强转哈希表）不查检，直写入指定位置元素。p1:哈希表，p2:key，p3:值。元素是内存副本。
static zend_always_inline void *zend_hash_add_new_mem(HashTable *ht, zend_string *key, void *pData, size_t size)
{
	zval tmp, *zv;

	ZVAL_PTR(&tmp, NULL);
	// （强转哈希表）不查检，直写入指定位置元素。p1:哈希表，p2:key，p3:值
	if ((zv = zend_hash_add_new(ht, key, &tmp))) {
		// 创建新 副本内存
		Z_PTR_P(zv) = pemalloc(size, GC_FLAGS(ht) & IS_ARRAY_PERSISTENT);
		// 把数据复制到新内存中
		memcpy(Z_PTR_P(zv), pData, size);
		// 返回更新后的指针
		return Z_PTR_P(zv);
	}
	return NULL;
}

// ing4,（强转哈希表）添加，碰到已有原素返回null。否则添加。元素是内存副本。返回更新后的指针。
// p1:哈希表，p2:key, p3:key长度, p4:新值
static zend_always_inline void *zend_hash_str_add_mem(HashTable *ht, const char *str, size_t len, void *pData, size_t size)
{
	zval tmp, *zv;

	ZVAL_PTR(&tmp, NULL);
	// （强转哈希表）添加，碰到已有原素返回null。否则添加。p1:哈希表，p2:key, p3:key长度, p4:新值
	if ((zv = zend_hash_str_add(ht, str, len, &tmp))) {
		// 创建新 副本内存
		Z_PTR_P(zv) = pemalloc(size, GC_FLAGS(ht) & IS_ARRAY_PERSISTENT);
		// 把数据复制到新内存中
		memcpy(Z_PTR_P(zv), pData, size);
		// 返回更新后的指针
		return Z_PTR_P(zv);
	}
	return NULL;
}

// ing4,（强转哈希表）添加，不检查已有，直接找到位置，写入元素。元素是内存副本。返回副本指针。
// p1:哈希表，p2:key, p3:key长度, p4:新值
// 全局无调用
static zend_always_inline void *zend_hash_str_add_new_mem(HashTable *ht, const char *str, size_t len, void *pData, size_t size)
{
	zval tmp, *zv;

	ZVAL_PTR(&tmp, NULL);
	// （强转哈希表）添加，不检查已有，直接找到位置，写入元素。p1:哈希表，p2:key, p3:key长度, p4:新值
	if ((zv = zend_hash_str_add_new(ht, str, len, &tmp))) {
		// 创建新 副本内存
		Z_PTR_P(zv) = pemalloc(size, GC_FLAGS(ht) & IS_ARRAY_PERSISTENT);
		// 把数据复制到新内存中
		memcpy(Z_PTR_P(zv), pData, size);
		// 返回更新后的指针
		return Z_PTR_P(zv);
	}
	return NULL;
}

// ing4,（强转哈希表）不管找不到到原素，都更新指定位置的元素，返回更新结果。元素是内存副本
// p1:哈希表，p2:key，p3:指针，p4:内存大小
static zend_always_inline void *zend_hash_update_mem(HashTable *ht, zend_string *key, void *pData, size_t size)
{
	void *p;
	// 创建新 副本内存
	p = pemalloc(size, GC_FLAGS(ht) & IS_ARRAY_PERSISTENT);
	// 把数据复制到新内存中
	memcpy(p, pData, size);
	// （强转哈希表）不管找不到到原素，都更新指定位置的元素，返回更新结果。p1:哈希表，p2:key，p3:值
	return zend_hash_update_ptr(ht, key, p);
}

// ing4,（强转哈希表）更新，有旧原素时替换旧原素，否则写入新元素。返回更新结果。元素是内存副本
// p1:哈希表，p2:key, p3:key长度, p4:指针，p5:内存大小
static zend_always_inline void *zend_hash_str_update_mem(HashTable *ht, const char *str, size_t len, void *pData, size_t size)
{
	void *p;
	// 创建新 副本内存
	p = pemalloc(size, GC_FLAGS(ht) & IS_ARRAY_PERSISTENT);
	// 把数据复制到新内存中
	memcpy(p, pData, size);
	//（强转哈希表）更新，有旧原素时替换旧原素，否则写入新元素。返回更新结果。
	// p1:哈希表，p2:key, p3:key长度, p4:新值
	return zend_hash_str_update_ptr(ht, str, len, p);
}

// ing4, 顺序数组中找到旧原素直接中断，在哈希表中找到旧原素会替换掉。p1:哈希表，p2:哈希值，p3:指针元素
static zend_always_inline void *zend_hash_index_add_ptr(HashTable *ht, zend_ulong h, void *pData)
{
	zval tmp, *zv;

	ZVAL_PTR(&tmp, pData);
	// 顺序数组中找到旧原素直接中断，在哈希表中找到旧原素会替换掉。p1:哈希表，p2:哈希值，p3:元素
	zv = zend_hash_index_add(ht, h, &tmp);
	return zv ? Z_PTR_P(zv) : NULL;
}

// ing4, 顺序数组，碰到旧的新增。哈希表不查找旧原素，直接添加新元素。p1:哈希表，p2:哈希值，p3:指针元素
static zend_always_inline void *zend_hash_index_add_new_ptr(HashTable *ht, zend_ulong h, void *pData)
{
	zval tmp, *zv;

	ZVAL_PTR(&tmp, pData);
	// 顺序数组，碰到旧的新增。哈希表不查找旧原素，直接添加新元素。p1:哈希表，p2:哈希值，p3:元素
	zv = zend_hash_index_add_new(ht, h, &tmp);
	return zv ? Z_PTR_P(zv) : NULL;
}

// ing4, 找到旧原素都更新。找不到旧原素都添加，数组会转成哈希表。p1:哈希表，p2:哈希值，p3:指针元素
static zend_always_inline void *zend_hash_index_update_ptr(HashTable *ht, zend_ulong h, void *pData)
{
	zval tmp, *zv;

	ZVAL_PTR(&tmp, pData);
	// 找到旧原素都更新。找不到旧原素都添加，数组会转成哈希表。p1:哈希表，p2:哈希值，p3:元素
	zv = zend_hash_index_update(ht, h, &tmp);
	ZEND_ASSUME(Z_PTR_P(zv));
	return Z_PTR_P(zv);
}

// ing4, 顺序数组中找到旧原素直接中断，在哈希表中找到旧原素会替换掉。原素是内存副本。
// p1:哈希表，p2:哈希值，p3:指针，p4:内存大小
static zend_always_inline void *zend_hash_index_add_mem(HashTable *ht, zend_ulong h, void *pData, size_t size)
{
	zval tmp, *zv;

	ZVAL_PTR(&tmp, NULL);
	// 顺序数组中找到旧原素直接中断，在哈希表中找到旧原素会替换掉。p1:哈希表，p2:哈希值，p3:元素
	if ((zv = zend_hash_index_add(ht, h, &tmp))) {
		
		Z_PTR_P(zv) = pemalloc(size, GC_FLAGS(ht) & IS_ARRAY_PERSISTENT);
		// 把数据复制到新内存中
		memcpy(Z_PTR_P(zv), pData, size);
		return Z_PTR_P(zv);
	}
	return NULL;
}

// ing4,（哈希值是最小整数时，归0）顺序数组中找到旧原素直接中断，在哈希表中找到旧原素会替换掉。原素是内存副本。
// p1:哈希表，p2:元素
static zend_always_inline void *zend_hash_next_index_insert_ptr(HashTable *ht, void *pData)
{
	zval tmp, *zv;

	ZVAL_PTR(&tmp, pData);
	// （哈希值是最小整数时，归0）顺序数组中找到旧原素直接中断，在哈希表中找到旧原素会替换掉。p1:哈希表，p2:元素
	zv = zend_hash_next_index_insert(ht, &tmp);
	if (zv) {
		ZEND_ASSUME(Z_PTR_P(zv));
		return Z_PTR_P(zv);
	} else {
		return NULL;
	}
}

// ing4, 找到旧原素都更新。找不到旧原素都添加，数组会转成哈希表。原素是内存副本。
// p1:哈希表，p2:哈希值，p3:指针，p4:内存大小
static zend_always_inline void *zend_hash_index_update_mem(HashTable *ht, zend_ulong h, void *pData, size_t size)
{
	void *p;
	// 分配内存创建哈希表数据
	p = pemalloc(size, GC_FLAGS(ht) & IS_ARRAY_PERSISTENT);
	// 把数据复制到新内存中
	memcpy(p, pData, size);
	// 找到旧原素都更新。找不到旧原素都添加，数组会转成哈希表。p1:哈希表，p2:哈希值，p3:元素
	return zend_hash_index_update_ptr(ht, h, p);
}

// ing4, 插入内存副本，（哈希值是最小整数时，归0）顺序数组中找到旧原素直接中断，在哈希表中找到旧原素会替换掉。
// p1:哈希表，p2:指针，p3:内存大小
static zend_always_inline void *zend_hash_next_index_insert_mem(HashTable *ht, void *pData, size_t size)
{
	zval tmp, *zv;

	ZVAL_PTR(&tmp, NULL);
	// （哈希值是最小整数时，归0）顺序数组中找到旧原素直接中断，在哈希表中找到旧原素会替换掉。p1:哈希表，p2:元素
	if ((zv = zend_hash_next_index_insert(ht, &tmp))) {
		// 分配内存创建哈希表数据
		Z_PTR_P(zv) = pemalloc(size, GC_FLAGS(ht) & IS_ARRAY_PERSISTENT);
		// 把数据复制到新内存中
		memcpy(Z_PTR_P(zv), pData, size);
		// 返回数据块指针
		return Z_PTR_P(zv);
	}
	return NULL;
}

// 光是查指针变量就有好多个重载
// ing3， 哈希表中查 找指针元素
static zend_always_inline void *zend_hash_find_ptr(const HashTable *ht, zend_string *key)
{
	zval *zv;

	// 使用 key 获取哈希表的 bucket值。p1:哈希表，p2:key(zend_string)
	zv = zend_hash_find(ht, key);
	// 如果有，返回指针，没有返回null
	if (zv) {
		// (zval)->value.ptr 必须存在
		ZEND_ASSUME(Z_PTR_P(zv));
		return Z_PTR_P(zv);
	} else {
		return NULL;
	}
}

// ing3, 哈希表中，查找指针元素，并返回指针
static zend_always_inline void *zend_hash_find_ex_ptr(const HashTable *ht, zend_string *key, bool known_hash)
{
	zval *zv;
	// 哈希表里用 key 查找元素。p1:哈希表，p2:key，p3：key上是否有哈希值
	zv = zend_hash_find_ex(ht, key, known_hash);
	if (zv) {
		ZEND_ASSUME(Z_PTR_P(zv));
		return Z_PTR_P(zv);
	} else {
		return NULL;
	}
}

// ing3, 查找指针元素（指定类型）
static zend_always_inline void *zend_hash_str_find_ptr(const HashTable *ht, const char *str, size_t len)
{
	zval *zv;
	// 使用 键(string) 获取哈希表的 值。p1:哈希表，p2:key(char *),p3:长度
	zv = zend_hash_str_find(ht, str, len);
	if (zv) {
		// 类型必须是指针
		ZEND_ASSUME(Z_PTR_P(zv));
		return Z_PTR_P(zv);
	} else {
		return NULL;
	}
}

// 会把字串转成小写，只有不再用到此字串时，使用此方法。
// 如果已经有了小写字串，使用 zend_hash_str_find_ptr
/* Will lowercase the str; use only if you don't need the lowercased string for
 * anything else. If you have a lowered string, use zend_hash_str_find_ptr. */
ZEND_API void *zend_hash_str_find_ptr_lc(const HashTable *ht, const char *str, size_t len);

// 会把字串转成小写，只有不再用到此字串时，使用此方法。
// 如果已经有了小写字串，使用 zend_hash_find_ptr
/* Will lowercase the str; use only if you don't need the lowercased string for
 * anything else. If you have a lowered string, use zend_hash_find_ptr. */
ZEND_API void *zend_hash_find_ptr_lc(const HashTable *ht, zend_string *key);

// ing3, 通过哈希值或顺序号 查找指针变量
static zend_always_inline void *zend_hash_index_find_ptr(const HashTable *ht, zend_ulong h)
{
	zval *zv;
	// 按哈希值查询，兼容顺序数组。p1:哈希表，p2:哈希值
	zv = zend_hash_index_find(ht, h);
	if (zv) {
		ZEND_ASSUME(Z_PTR_P(zv));
		return Z_PTR_P(zv);
	} else {
		return NULL;
	}
}

// ing3, 用哈希值查找 元素 并减少值的引用次数
static zend_always_inline zval *zend_hash_index_find_deref(HashTable *ht, zend_ulong h)
{
	// 按哈希值查询，兼容顺序数组。p1:哈希表，p2:哈希值
	zval *zv = zend_hash_index_find(ht, h);
	// 如果找到元素
	if (zv) {
		// 减少引用次数
		ZVAL_DEREF(zv);
	}
	return zv;
}

// ing3, 减少 键值对 值的引用次数
static zend_always_inline zval *zend_hash_find_deref(HashTable *ht, zend_string *str)
{
	// 使用 key 获取哈希表的 bucket值。p1:哈希表，p2:key(zend_string)
	zval *zv = zend_hash_find(ht, str);
	if (zv) {
		ZVAL_DEREF(zv);
	}
	return zv;
}

// ing3, 减少 键值对 值的引用次数
static zend_always_inline zval *zend_hash_str_find_deref(HashTable *ht, const char *str, size_t len)
{
	// 使用 键(string) 获取哈希表的 值。p1:哈希表，p2:key(char *),p3:长度
	zval *zv = zend_hash_str_find(ht, str, len);
	if (zv) {
		// 减少引用数
		ZVAL_DEREF(zv);
	}
	return zv;
}

// ing3, 根据传入key的类型自动适配查找指针变量。 全局无调用。
static zend_always_inline void *zend_symtable_str_find_ptr(HashTable *ht, const char *str, size_t len)
{
	zend_ulong idx;
	// 如果是数字字串
	if (ZEND_HANDLE_NUMERIC_STR(str, len, idx)) {
		return zend_hash_index_find_ptr(ht, idx);
	} else {
		return zend_hash_str_find_ptr(ht, str, len);
	}
}

// ing3, 获取 位置 右侧第一个有效元素，返回值中的 指针 ptr。p1:哈希表，p2:位置
static zend_always_inline void *zend_hash_get_current_data_ptr_ex(HashTable *ht, HashPosition *pos)
{
	zval *zv;
	// 获取右侧第一个有效元素的值
	zv = zend_hash_get_current_data_ex(ht, pos);
	// 必须是指针，返回指针
	if (zv) {
		ZEND_ASSUME(Z_PTR_P(zv));
		return Z_PTR_P(zv);
	} else {
		return NULL;
	}
}

// ing4, 获取 内置指针 右侧第一个有效元素，返回值中的 指针 ptr。p1:哈希表，p2:位置
#define zend_hash_get_current_data_ptr(ht) \
	zend_hash_get_current_data_ptr_ex(ht, &(ht)->nInternalPointer)

// 公用的 哈希表/数组 迭代器
/* Common hash/packed array iterators */

// 获取元素大小
#if 0
// ing4, 获取数组中单个元素大小
# define ZEND_HASH_ELEMENT_SIZE(__ht) \
	(HT_IS_PACKED(__ht) ? sizeof(zval) : sizeof(Bucket))
// 优化版
#else /* optimized version */

// ing3, 获取数组中单个元素大小。这样避免调用 HT_IS_PACKED 宏
	// (~HT_FLAGS(__ht) & HASH_FLAG_PACKED) 如果是顺序数组会是0。是哈希表最后把乘的东西除掉，只把差额加上。
	// (HT_FLAGS(__ht) & HASH_FLAG_PACKED) == HASH_FLAG_PACKED ? sizeof(zval) : sizeof(Bucket) 这样好不好？
# define ZEND_HASH_ELEMENT_SIZE(__ht) \
	(sizeof(zval) + (~HT_FLAGS(__ht) & HASH_FLAG_PACKED) * ((sizeof(Bucket)-sizeof(zval))/HASH_FLAG_PACKED))
#endif

// ing3, 找到顺序数组中第n个元素。p1:哈希表，p2:元素序号，p3:元素大小
#define ZEND_HASH_ELEMENT_EX(__ht, _idx, _size) \
	((zval*)(((char*)(__ht)->arPacked) + ((_idx) * (_size))))

// ing4, 找到顺序数组中第n个元素。p1:哈希表，p2:元素序号
#define ZEND_HASH_ELEMENT(__ht, _idx) \
	/* 找到顺序数组中第n个元素。p1:哈希表，p2:元素序号，p3:元素大小 */ \
	ZEND_HASH_ELEMENT_EX(__ht, _idx, ZEND_HASH_ELEMENT_SIZE(__ht))

// ing4, 找到数组的下一个元素，p1:元素列表指针，p2:元素大小
#define ZEND_HASH_NEXT_ELEMENT(_el, _size) \
	((zval*)(((char*)(_el)) + (_size)))

// ing4, 找到数组的上一个元素，p1:元素列表指针，p2:元素大小
#define ZEND_HASH_PREV_ELEMENT(_el, _size) \
	((zval*)(((char*)(_el)) - (_size)))

// ing4, 遍历哈希表 ->arPacked 。p1:哈希表指针
#define _ZEND_HASH_FOREACH_VAL(_ht) do { \
		/* 哈希表指针 */ \
		HashTable *__ht = (_ht); \
		/* 使用元素数 */ \
		uint32_t _count = __ht->nNumUsed; \
		/* 获取数组中单个元素大小 */ \
		size_t _size = ZEND_HASH_ELEMENT_SIZE(__ht); \
		/* 顺序数组元素开头 */ \
		zval *_z = __ht->arPacked; \
		/* 遍历每个元素 */ \
		/* 找到数组的下一个元素，p1:元素列表指针，p2:元素大小 */ \
		for (; _count > 0; _z = ZEND_HASH_NEXT_ELEMENT(_z, _size), _count--) { \
			/* 跳过未定义的元素 */ \
			if (UNEXPECTED(Z_TYPE_P(_z) == IS_UNDEF)) continue;

// ing4, 倒序遍历哈希表 ->arPacked 。p1:哈希表指针
#define _ZEND_HASH_REVERSE_FOREACH_VAL(_ht) do { \
		/* 哈希表指针 */ \
		HashTable *__ht = (_ht); \
		/* 从最大序号开始 */ \
		uint32_t _idx = __ht->nNumUsed; \
		/* 获取数组中单个元素大小 */ \
		size_t _size = ZEND_HASH_ELEMENT_SIZE(__ht); \
		/* 找到顺序数组中第n个元素。p1:哈希表，p2:元素序号，p3:元素大小 */ \
		zval *_z = ZEND_HASH_ELEMENT_EX(__ht, _idx, _size); \
		/* 倒序遍历 */ \
		for (;_idx > 0; _idx--) { \
			/* 找到数组的上一个元素，p1:元素列表指针，p2:元素大小 */ \
			_z = ZEND_HASH_PREV_ELEMENT(_z, _size); \
			/* 路过无效 */ \
			if (UNEXPECTED(Z_TYPE_P(_z) == IS_UNDEF)) continue;

// ing3, 从第n个元素开始遍历哈希表，p1:哈希表，p2:是否跳过引用目标值无效的【bucket】，p3:开始序号
#define ZEND_HASH_FOREACH_FROM(_ht, indirect, _from) do { \
		/* 哈希表指针 */ \
		HashTable *__ht = (_ht); \
		/* 哈希值 */ \
		zend_ulong __h; \
		/* key默认是null */ \
		zend_string *__key = NULL; \
		/* 开始序号 */ \
		uint32_t _idx = (_from); \
		/* 获取数组中单个元素大小 */ \
		size_t _size = ZEND_HASH_ELEMENT_SIZE(__ht); \
		/* 找到顺序数组中第n个元素。p1:哈希表，p2:元素序号，p3:元素大小 */ \
		zval *__z = ZEND_HASH_ELEMENT_EX(__ht, _idx, _size); \
		/* 需要遍历的数量 = 使用数量-当前序号 */ \
		uint32_t _count = __ht->nNumUsed - _idx; \
		/* 遍历每个元素 */ \
		for (;_count > 0; _count--) { \
			/* 临时变量 */ \
			zval *_z = __z; \
			/* 顺序数组 */ \
			if (HT_IS_PACKED(__ht)) { \
				/* 下一个元素 */ \
				__z++; \
				/* 序号 作为 哈希值 */ \
				__h = _idx; \
				/* 序号+1 */ \
				_idx++; \
			/* 哈希表 */ \
			} else { \
				/* 临时变量，转为bucket */ \
				Bucket *_p = (Bucket*)__z; \
				/* Bucket 中的 zval */ \
				__z = &(_p + 1)->val; \
				/* Bucket 中的 哈希值 */ \
				__h = _p->h; \
				/* Bucket 中的 key */ \
				__key = _p->key; \
				/* 如果值是间接引用 */ \
				if (indirect && Z_TYPE_P(_z) == IS_INDIRECT) { \
					/* 解间接引用 */ \
					_z = Z_INDIRECT_P(_z); \
				} \
			} \
			/* 经测试，这样写没什么作用，可能是window的原因？ */ \
			(void) __h; (void) __key; (void) _idx; \
			/* 如果元素无效 或 间接引用目标无效，跳过 */ \
			if (UNEXPECTED(Z_TYPE_P(_z) == IS_UNDEF)) continue;

// ing4, 遍历整个哈希表，p1:哈希表，p2:是否跳过引用目标值无效的【bucket】
// 从第n个元素开始遍历哈希表，p1:哈希表，p2:间接引用检查（是否跳过引用目标值无效的【bucket】），p3:开始序号
#define ZEND_HASH_FOREACH(_ht, indirect) ZEND_HASH_FOREACH_FROM(_ht, indirect, 0)

// ing3, 倒序遍历哈希表，p1:哈希表，p2:是否跳过引用目标值无效的【bucket】
#define ZEND_HASH_REVERSE_FOREACH(_ht, indirect) do { \
		HashTable *__ht = (_ht); \
		uint32_t _idx = __ht->nNumUsed; \
		zval *_z; \
		zend_ulong __h; \
		zend_string *__key = NULL; \
		/* 获取数组中单个元素大小 */ \
		size_t _size = ZEND_HASH_ELEMENT_SIZE(__ht); \
		/* 找到顺序数组中第n个元素。p1:哈希表，p2:元素序号，p3:元素大小 */ \
		zval *__z = ZEND_HASH_ELEMENT_EX(__ht, _idx, _size); \
		/* 倒序遍历 */ \
		for (;_idx > 0; _idx--) { \
			/* 顺序数组 */ \
			if (HT_IS_PACKED(__ht)) { \
				/* 前一个元素 */ \
				__z--; \
				/* 临时变量 */ \
				_z = __z; \
				/* 顺序号 作为 哈希值 */ \
				__h = _idx - 1; \
			/* 哈希表 */ \
			} else { \
				/* 元素转成 Bucket */ \
				Bucket *_p = (Bucket*)__z; \
				/* 前一个元素 */ \
				_p--; \
				/* 指向bucket 中的 zval */ \
				__z = &_p->val; \
				/* 临时变量，用于检查间接引用 */ \
				_z = __z; \
				/* 指向bucket 中的 哈希表 */ \
				__h = _p->h; \
				/* 指向bucket 中的 key */ \
				__key = _p->key; \
				/* 如果需要解引用 并且 碰到间接引用 */ \
				if (indirect && Z_TYPE_P(_z) == IS_INDIRECT) { \
					/* 解引用 */ \
					_z = Z_INDIRECT_P(_z); \
				} \
			} \
			/* 如果元素无效 或 间接引用目标无效，跳过 */ \
			(void) __h; (void) __key; (void) __z; \
			/* 如果元素无效 或 间接引用目标无效，跳过 */ \
			if (UNEXPECTED(Z_TYPE_P(_z) == IS_UNDEF)) continue;

// ing4， 数组循环结束
#define ZEND_HASH_FOREACH_END() \
		} \
	} while (0)

// ing4, 遍历【哈希表】之后，依次删除每个Bucket（不支持顺序数组），并更新相同哈希值下的元素链关系
#define ZEND_HASH_FOREACH_END_DEL() \
	/* 遍历【哈希表】之后，依次删除每个Bucket（不支持顺序数组），并更新相同哈希值下的元素链关系 */ \
	ZEND_HASH_MAP_FOREACH_END_DEL()

// ing4, 遍历哈希表（不支持顺序数组）,不跳过引用目标值无效的【bucket】。
// p1:哈希表，p2:访问的 bucket
#define ZEND_HASH_FOREACH_BUCKET(ht, _bucket) \
	/* 遍历哈希表（不支持顺序数组）,不跳过引用目标值无效的【bucket】。*/ \
	/* p1:哈希表，p2:访问的 bucket */ \
	ZEND_HASH_MAP_FOREACH_BUCKET(ht, _bucket)

// ing4, 从指定位置开始遍历哈希表（不支持顺序数组）,不跳过引用目标值无效的【bucket】。
// p1:哈希表，p2:访问的 bucket, p3:开始位置
#define ZEND_HASH_FOREACH_BUCKET_FROM(ht, _bucket, _from) \
	/* 从指定位置开始遍历哈希表（不支持顺序数组）,不跳过引用目标值无效的【bucket】。*/ \
	/* p1:哈希表，p2:访问的 bucket, p3:开始位置 */ \
	ZEND_HASH_MAP_FOREACH_BUCKET_FROM(ht, _bucket, _from)

// ing4, 倒序遍历哈希表（不支持顺序数组）,不跳过引用目标值无效的【bucket】。
// p1:哈希表，p2:访问的 bucket
#define ZEND_HASH_REVERSE_FOREACH_BUCKET(ht, _bucket) \
	/* 倒序遍历哈希表（不支持顺序数组）,不跳过引用目标值无效的【bucket】。 */ \
	/* p1:哈希表，p2:访问的 bucket */ \
	ZEND_HASH_MAP_REVERSE_FOREACH_BUCKET(ht, _bucket)

// ing4, 遍历哈希表 p1->arPacked。p1:哈希表，p2:访问的元素
#define ZEND_HASH_FOREACH_VAL(ht, _val) \
	/* 遍历哈希表 ->arPacked。p1:哈希表指针 */ \
	_ZEND_HASH_FOREACH_VAL(ht); \
	/* 供外部访问 */ \
	_val = _z;

// ing4, 倒序遍历哈希表 p1->arPacked。p1:哈希表，p2:访问的元素
#define ZEND_HASH_REVERSE_FOREACH_VAL(ht, _val) \
	/* 倒序遍历哈希表 ->arPacked 。p1:哈希表指针 */ \
	_ZEND_HASH_REVERSE_FOREACH_VAL(ht); \
	/* 供外部访问 */ \
	_val = _z;

// ing4, 遍历整个哈希表，跳过引用目标值无效的【bucket】。p1:哈希表，p2:访问的元素
#define ZEND_HASH_FOREACH_VAL_IND(ht, _val) \
	/* 遍历整个哈希表，p1:哈希表，p2:是否跳过引用目标值无效的【bucket】 */ \
	ZEND_HASH_FOREACH(ht, 1); \
	/* 供外部访问 */ \
	_val = _z;

// ing4, 倒序遍历哈希表，跳过引用目标值无效的【bucket】。p1:哈希表，p2:访问的元素
#define ZEND_HASH_REVERSE_FOREACH_VAL_IND(ht, _val) \
	/* 倒序遍历哈希表，p1:哈希表，p2:是否跳过引用目标值无效的【bucket】 */ \
	ZEND_HASH_REVERSE_FOREACH(ht, 1); \
	/* 供外部访问 */ \
	_val = _z;

// ing4, 遍历哈希表 ->arPacked。p1:哈希表指针，p2:访问的指针元素
#define ZEND_HASH_FOREACH_PTR(ht, _ptr) \
	/* 遍历哈希表 ->arPacked。p1:哈希表指针 */ \
	_ZEND_HASH_FOREACH_VAL(ht); \
	/* 供外部访问 */ \
	_ptr = Z_PTR_P(_z);

// ing4, 从第n个元素开始遍历哈希表，p1:哈希表，p2:访问的指针元素，p3:开始序号
#define ZEND_HASH_FOREACH_PTR_FROM(ht, _ptr, _from) \
	/* 从第n个元素开始遍历哈希表，p1:哈希表，p2:是否跳过引用目标值无效的【bucket】，p3:开始序号 */ \
	ZEND_HASH_FOREACH_FROM(ht, 0, _from); \
	/* 供外部访问 */ \
	_ptr = Z_PTR_P(_z);

// ing4, 倒序遍历哈希表 ->arPacked 。p1:哈希表，p2:访问的指针元素
#define ZEND_HASH_REVERSE_FOREACH_PTR(ht, _ptr) \
	/* 倒序遍历哈希表 ->arPacked 。p1:哈希表 */ \
	_ZEND_HASH_REVERSE_FOREACH_VAL(ht); \
	/* 供外部访问 */ \
	_ptr = Z_PTR_P(_z);

// ing4, 遍历整个哈希表，不跳过引用目标值无效的【bucket】。p1:哈希表，p2:访问的哈希值
#define ZEND_HASH_FOREACH_NUM_KEY(ht, _h) \
	/* 遍历整个哈希表，p1:哈希表，p2:是否跳过引用目标值无效的【bucket】 */ \
	ZEND_HASH_FOREACH(ht, 0); \
	/* 供外部访问 */ \
	_h = __h;

// ing4, 倒序遍历哈希表，不跳过引用目标值无效的【bucket】。p1:哈希表，p2:访问的哈希值
#define ZEND_HASH_REVERSE_FOREACH_NUM_KEY(ht, _h) \
	/* 倒序遍历哈希表，p1:哈希表，p2:是否跳过引用目标值无效的【bucket】 */ \
	ZEND_HASH_REVERSE_FOREACH(ht, 0); \
	/* 供外部访问 */ \
	_h = __h;

// ing4, 遍历整个哈希表，不跳过引用目标值无效的【bucket】。p1:哈希表，p2:访问的key
#define ZEND_HASH_FOREACH_STR_KEY(ht, _key) \
	/* 遍历整个哈希表，p1:哈希表，p2:是否跳过引用目标值无效的【bucket】 */ \
	ZEND_HASH_FOREACH(ht, 0); \
	/* 供外部访问 */ \
	_key = __key;

// ing4, 倒序遍历哈希表，不跳过引用目标值无效的【bucket】。p1:哈希表，p2:访问的key
#define ZEND_HASH_REVERSE_FOREACH_STR_KEY(ht, _key) \
	/* 倒序遍历哈希表，p1:哈希表，p2:是否跳过引用目标值无效的【bucket】 */ \
	ZEND_HASH_REVERSE_FOREACH(ht, 0); \
	/* 供外部访问 */ \
	_key = __key;

// ing4, 遍历整个哈希表，不跳过引用目标值无效的【bucket】。p1:哈希表，p2:访问的哈希值，p3:访问的key
#define ZEND_HASH_FOREACH_KEY(ht, _h, _key) \
	/* 遍历整个哈希表，p1:哈希表，p2:是否跳过引用目标值无效的【bucket】 */ \
	ZEND_HASH_FOREACH(ht, 0); \
	/* 供外部访问 */ \
	_h = __h; \
	_key = __key;

// ing4, 倒序遍历哈希表，不跳过引用目标值无效的【bucket】。p1:哈希表，p2:访问的哈希值，p3:访问的key
#define ZEND_HASH_REVERSE_FOREACH_KEY(ht, _h, _key) \
	/* 倒序遍历哈希表，p1:哈希表，p2:是否跳过引用目标值无效的【bucket】 */ \
	ZEND_HASH_REVERSE_FOREACH(ht, 0); \
	/* 供外部访问 */ \
	_h = __h; \
	_key = __key;

// ing4, 遍历整个哈希表，不跳过引用目标值无效的【bucket】。p1:哈希表，p2:访问的哈希值, p3:访问的元素值
#define ZEND_HASH_FOREACH_NUM_KEY_VAL(ht, _h, _val) \
	/* 遍历整个哈希表，p1:哈希表，p2:是否跳过引用目标值无效的【bucket】 */ \
	ZEND_HASH_FOREACH(ht, 0); \
	/* 供外部访问 */ \
	_h = __h; \
	_val = _z;

// ing4, 倒序遍历哈希表，不跳过引用目标值无效的【bucket】。p1:哈希表，p2:访问的哈希值, p3:访问的元素值
#define ZEND_HASH_REVERSE_FOREACH_NUM_KEY_VAL(ht, _h, _val) \
	/* 倒序遍历哈希表，p1:哈希表，p2:是否跳过引用目标值无效的【bucket】 */ \
	ZEND_HASH_REVERSE_FOREACH(ht, 0); \
	/* 供外部访问 */ \
	_h = __h; \
	_val = _z;

// ing4, 遍历整个哈希表，不跳过引用目标值无效的【bucket】。p1:哈希表，p2:访问的key, p3:访问的元素值
#define ZEND_HASH_FOREACH_STR_KEY_VAL(ht, _key, _val) \
	/* 遍历整个哈希表，p1:哈希表，p2:是否跳过引用目标值无效的【bucket】 */ \
	ZEND_HASH_FOREACH(ht, 0); \
	/* 供外部访问 */ \
	_key = __key; \
	_val = _z;

// ing4, 从第n个元素开始遍历哈希表，p1:哈希表，p2:访问的key, p3:访问的元素值, p4:开始位置
#define ZEND_HASH_FOREACH_STR_KEY_VAL_FROM(ht, _key, _val, _from) \
	/* 从第n个元素开始遍历哈希表，p1:哈希表，p2:是否跳过引用目标值无效的【bucket】，p3:开始序号 */ \
	ZEND_HASH_FOREACH_FROM(ht, 0, _from); \
	/* 供外部访问 */ \
	_key = __key; \
	_val = _z;

// ing4, 倒序遍历哈希表，不跳过引用目标值无效的【bucket】。p1:哈希表，p2:访问的key, p3:访问的元素值
#define ZEND_HASH_REVERSE_FOREACH_STR_KEY_VAL(ht, _key, _val) \
	/* 倒序遍历哈希表，p1:哈希表，p2:是否跳过引用目标值无效的【bucket】 */ \
	ZEND_HASH_REVERSE_FOREACH(ht, 0); \
	/* 供外部访问 */ \
	_key = __key; \
	_val = _z;

// ing4, 遍历整个哈希表，不跳过引用目标值无效的【bucket】。p1:哈希表，p2:访问的哈希值，p3:访问的key, p4:访问的元素值
#define ZEND_HASH_FOREACH_KEY_VAL(ht, _h, _key, _val) \
	/* 遍历整个哈希表，p1:哈希表，p2:是否跳过引用目标值无效的【bucket】 */ \
	ZEND_HASH_FOREACH(ht, 0); \
	/* 供外部访问 */ \
	_h = __h; \
	_key = __key; \
	_val = _z;

// ing4, 倒序遍历哈希表，不跳过引用目标值无效的【bucket】。p1:哈希表，p2:访问的哈希值，p3:访问的key, p4:访问的元素值
#define ZEND_HASH_REVERSE_FOREACH_KEY_VAL(ht, _h, _key, _val) \
	/* 倒序遍历哈希表，p1:哈希表，p2:是否跳过引用目标值无效的【bucket】 */ \
	ZEND_HASH_REVERSE_FOREACH(ht, 0); \
	/* 供外部访问 */ \
	_h = __h; \
	_key = __key; \
	_val = _z;

// ing4, 遍历整个哈希表，跳过引用目标值无效的【bucket】。p1:哈希表，p2:访问的key, p3:访问的元素值
#define ZEND_HASH_FOREACH_STR_KEY_VAL_IND(ht, _key, _val) \
	/* 遍历整个哈希表，p1:哈希表，p2:是否跳过引用目标值无效的【bucket】 */ \
	ZEND_HASH_FOREACH(ht, 1); \
	/* 供外部访问 */ \
	_key = __key; \
	_val = _z;

// ing4, 倒序遍历哈希表，跳过引用目标值无效的【bucket】。p1:哈希表，p2:访问的key, p3:访问的元素值
#define ZEND_HASH_REVERSE_FOREACH_STR_KEY_VAL_IND(ht, _key, _val) \
	/* 倒序遍历哈希表，p1:哈希表，p2:是否跳过引用目标值无效的【bucket】 */ \
	ZEND_HASH_REVERSE_FOREACH(ht, 1); \
	/* 供外部访问 */ \
	_key = __key; \
	_val = _z;

// ing4, 遍历整个哈希表，跳过引用目标值无效的【bucket】。p1:哈希表，p2:访问的哈希值，p3:访问的key, p4:访问的元素值
#define ZEND_HASH_FOREACH_KEY_VAL_IND(ht, _h, _key, _val) \
	/* 遍历整个哈希表，p1:哈希表，p2:是否跳过引用目标值无效的【bucket】 */ \
	ZEND_HASH_FOREACH(ht, 1); \
	/* 供外部访问 */ \
	_h = __h; \
	_key = __key; \
	_val = _z;

// ing4, 倒序遍历哈希表，跳过引用目标值无效的【bucket】。p1:哈希表，p2:访问的哈希值，p3:访问的key, p4:访问的元素值
#define ZEND_HASH_REVERSE_FOREACH_KEY_VAL_IND(ht, _h, _key, _val) \
	/* 倒序遍历哈希表，p1:哈希表，p2:是否跳过引用目标值无效的【bucket】 */ \
	ZEND_HASH_REVERSE_FOREACH(ht, 1); \
	/* 供外部访问 */ \
	_h = __h; \
	_key = __key; \
	_val = _z;

// ing4, 遍历整个哈希表，不跳过引用目标值无效的【bucket】。p1:哈希表，p2:访问的哈希值，p3:访问的指针元素
#define ZEND_HASH_FOREACH_NUM_KEY_PTR(ht, _h, _ptr) \
	/* 遍历整个哈希表，p1:哈希表，p2:是否跳过引用目标值无效的【bucket】 */ \
	ZEND_HASH_FOREACH(ht, 0); \
	/* 供外部访问 */ \
	_h = __h; \
	_ptr = Z_PTR_P(_z);

// ing4, 倒序遍历哈希表，不跳过引用目标值无效的【bucket】。p1:哈希表，p2:访问的哈希值，p3:访问的指针元素
#define ZEND_HASH_REVERSE_FOREACH_NUM_KEY_PTR(ht, _h, _ptr) \
	/* 倒序遍历哈希表，p1:哈希表，p2:是否跳过引用目标值无效的【bucket】 */ \
	ZEND_HASH_REVERSE_FOREACH(ht, 0); \
	/* 供外部访问 */ \
	_h = __h; \
	_ptr = Z_PTR_P(_z);

// ing4, 遍历整个哈希表，不跳过引用目标值无效的【bucket】。p1:哈希表，p2:访问的key, p3:访问的元素值
#define ZEND_HASH_FOREACH_STR_KEY_PTR(ht, _key, _ptr) \
	/* 遍历整个哈希表，p1:哈希表，p2:是否跳过引用目标值无效的【bucket】 */ \
	ZEND_HASH_FOREACH(ht, 0); \
	/* 供外部访问 */ \
	_key = __key; \
	_ptr = Z_PTR_P(_z);

// ing4, 倒序遍历哈希表，不跳过引用目标值无效的【bucket】。p1:哈希表，p2:访问的key, p3:访问的指针元素值
#define ZEND_HASH_REVERSE_FOREACH_STR_KEY_PTR(ht, _key, _ptr) \
	/* 倒序遍历哈希表，p1:哈希表，p2:是否跳过引用目标值无效的【bucket】 */ \
	ZEND_HASH_REVERSE_FOREACH(ht, 0); \
	_key = __key; \
	_ptr = Z_PTR_P(_z);

// ing4, 遍历整个哈希表，不跳过引用目标值无效的【bucket】。p1:哈希表，p2:访问的哈希值，p3:访问的key, p4:访问的元素值
#define ZEND_HASH_FOREACH_KEY_PTR(ht, _h, _key, _ptr) \
	/* 遍历整个哈希表，p1:哈希表，p2:是否跳过引用目标值无效的【bucket】 */ \
	ZEND_HASH_FOREACH(ht, 0); \
	/* 供外部访问 */ \
	_h = __h; \
	_key = __key; \
	_ptr = Z_PTR_P(_z);

// ing4, 倒序遍历哈希表，不跳过引用目标值无效的【bucket】。p1:哈希表，p2:访问的哈希值，p3:访问的key, p4:访问的元素值
#define ZEND_HASH_REVERSE_FOREACH_KEY_PTR(ht, _h, _key, _ptr) \
	/* 倒序遍历哈希表，p1:哈希表，p2:是否跳过引用目标值无效的【bucket】 */ \
	ZEND_HASH_REVERSE_FOREACH(ht, 0); \
	/* 供外部访问 */ \
	_h = __h; \
	_key = __key; \
	_ptr = Z_PTR_P(_z);

// ing4, 从指定位置开始遍历哈希表（不支持顺序数组）。p1:哈希表，p2:是否跳过引用目标值无效的【bucket】，p3:开始序号
/* Hash array iterators */
#define ZEND_HASH_MAP_FOREACH_FROM(_ht, indirect, _from) do { \
		/* 哈希表数组 */ \
		HashTable *__ht = (_ht); \
		/* 开始位置：如果有自定义的起始位置，从自定义的起始位置开始 */ \
		Bucket *_p = __ht->arData + (_from); \
		/* 末尾 */ \
		Bucket *_end = __ht->arData + __ht->nNumUsed; \
		/* 必须不是数字索引数组 */ \
		ZEND_ASSERT(!HT_IS_PACKED(__ht)); \
		/* 遍历每一个 Bucket；条件：指针没到末尾；指针后移 */ \
		for (; _p != _end; _p++) { \
			/* 取得值的指针 */ \
			zval *_z = &_p->val; \
			/* 如果要检查间接引用， 当前对象 切换到引用对象 */ \
			if (indirect && Z_TYPE_P(_z) == IS_INDIRECT) { \
				/* 追踪引用目标 */ \
				_z = Z_INDIRECT_P(_z); \
			} \
			/* 如果 当前对象是未定义 跳过 */ \
			if (UNEXPECTED(Z_TYPE_P(_z) == IS_UNDEF)) continue;

// ing4, 遍历哈希表（不支持顺序数组）。p1:哈希表，p2:是否跳过引用目标值无效的【bucket】
// 从指定位置开始遍历哈希表（不支持顺序数组）。p1:哈希表，p2:是否跳过引用目标值无效的【bucket】，p3:开始序号
#define ZEND_HASH_MAP_FOREACH(_ht, indirect) ZEND_HASH_MAP_FOREACH_FROM(_ht, indirect, 0)

// ing4, 倒序遍历哈希表（不支持顺序数组）。p1:哈希表，p2:是否跳过引用目标值无效的【bucket】
#define ZEND_HASH_MAP_REVERSE_FOREACH(_ht, indirect) do { \
		/* 哈希表数组 */ \
		HashTable *__ht = (_ht); \
		/* 已使用数量 */ \
		uint32_t _idx = __ht->nNumUsed; \
		/* 已使用列表结尾 */ \
		Bucket *_p = __ht->arData + _idx; \
		/* 临时变量 */ \
		zval *_z; \
		/* 不支持顺序数组 */ \
		ZEND_ASSERT(!HT_IS_PACKED(__ht)); \
		/* 倒序遍历 */ \
		for (_idx = __ht->nNumUsed; _idx > 0; _idx--) { \
			/* 指针左移1个 */ \
			_p--; \
			/* 取出 bucket里的值 */ \
			_z = &_p->val; \
			/* 如果是间接引用 并且 需要解引用 */ \
			if (indirect && Z_TYPE_P(_z) == IS_INDIRECT) { \
				/* 解引用 */ \
				_z = Z_INDIRECT_P(_z); \
			} \
			/* 跳过无效元素 */ \
			if (UNEXPECTED(Z_TYPE_P(_z) == IS_UNDEF)) continue;

// ing3, 遍历【哈希表】之后，依次删除每个Bucket（不支持顺序数组），并更新相同哈希值下的元素链关系
#define ZEND_HASH_MAP_FOREACH_END_DEL() \
			/* 不支持顺序数组 */ \
			ZEND_ASSERT(!HT_IS_PACKED(__ht)); \
			/* 删除1个元素（计数减1） */ \
			__ht->nNumOfElements--; \
			do { \
				/* 64位系统中 index 不用转换 */ \
				uint32_t j = HT_IDX_TO_HASH(_idx - 1); \
				/* 哈希值序号 */ \
				uint32_t nIndex = _p->h | __ht->nTableMask; \
				/* 找到 (p1->arHash) 哈希->索引列表 中的指定索引号。p1:哈希表，p2:顺序号（和mask做过或运算, 负数） */ \
				uint32_t i = HT_HASH(__ht, nIndex); \
				/* 如果 哈希->索引 没有指向当前元素序号 */ \
				if (UNEXPECTED(j != i)) { \
					/* 找到哈希表中第 idx 个bucket。p1:哈希表，p2:bucket序号 */ \
					Bucket *prev = HT_HASH_TO_BUCKET(__ht, i); \
					/* 更新哈希值相同的一串元素的 指向 */ \
					/* 向前找，直到找到 这个编号 */ \
					while (Z_NEXT(prev->val) != j) { \
						/* 取出这个编号 */ \
						i = Z_NEXT(prev->val); \
						/* 找到哈希表中第 idx 个bucket。p1:哈希表，p2:bucket序号 */ \
						prev = HT_HASH_TO_BUCKET(__ht, i); \
					} \
					/* 更新前一个元素的 值 */ \
					Z_NEXT(prev->val) = Z_NEXT(_p->val); \
				/* 如果 哈希->索引 已经指向当前元素序号 */ \
				} else { \
					/* 找到 哈希->索引列表 中的指定索引号。p1:哈希表，p2:顺序号（和mask做过或运算, 负数） */ \
					HT_HASH(__ht, nIndex) = Z_NEXT(_p->val); \
				} \
			} while (0); \
		} \
		/* 更新使用数量 */ \
		__ht->nNumUsed = _idx; \
	} while (0)

// ing4, 遍历哈希表（不支持顺序数组）,不跳过引用目标值无效的【bucket】。
// p1:哈希表，p2:访问的 bucket
#define ZEND_HASH_MAP_FOREACH_BUCKET(ht, _bucket) \
	/* 遍历哈希表（不支持顺序数组）。p1:哈希表，p2:是否跳过引用目标值无效的【bucket】 */ \
	ZEND_HASH_MAP_FOREACH(ht, 0); \
	/* 供外部访问 */ \
	_bucket = _p;

// ing4, 从指定位置开始遍历哈希表（不支持顺序数组）,不跳过引用目标值无效的【bucket】。
// p1:哈希表，p2:访问的 bucket, p3:开始位置
#define ZEND_HASH_MAP_FOREACH_BUCKET_FROM(ht, _bucket, _from) \
	/* 从指定位置开始遍历哈希表（不支持顺序数组）。p1:哈希表，p2:是否跳过引用目标值无效的【bucket】，p3:开始序号 */ \
	ZEND_HASH_MAP_FOREACH_FROM(ht, 0, _from); \
	/* 供外部访问 */ \
	_bucket = _p;

// ing4, 倒序遍历哈希表（不支持顺序数组）,不跳过引用目标值无效的【bucket】。
// p1:哈希表，p2:访问的 bucket
#define ZEND_HASH_MAP_REVERSE_FOREACH_BUCKET(ht, _bucket) \
	/* 倒序遍历哈希表（不支持顺序数组）。p1:哈希表，p2:是否跳过引用目标值无效的【bucket】 */ \
	ZEND_HASH_MAP_REVERSE_FOREACH(ht, 0); \
	/* 供外部访问 */ \
	_bucket = _p;

// ing4, 遍历哈希表（不支持顺序数组）,不跳过引用目标值无效的【bucket】。
// p1:哈希表，p2:访问的元素
#define ZEND_HASH_MAP_FOREACH_VAL(ht, _val) \
	/* 遍历哈希表（不支持顺序数组）。p1:哈希表，p2:是否跳过引用目标值无效的【bucket】 */ \
	ZEND_HASH_MAP_FOREACH(ht, 0); \
	/* 供外部访问 */ \
	_val = _z;

// ing4, 倒序遍历哈希表（不支持顺序数组）,不跳过引用目标值无效的【bucket】。
// p1:哈希表，p2:访问的元素
#define ZEND_HASH_MAP_REVERSE_FOREACH_VAL(ht, _val) \
	/* 倒序遍历哈希表（不支持顺序数组）。p1:哈希表，p2:是否跳过引用目标值无效的【bucket】 */ \
	ZEND_HASH_MAP_REVERSE_FOREACH(ht, 0); \
	/* 供外部访问 */ \
	_val = _z;

// ing4, 遍历哈希表（不支持顺序数组）,跳过引用目标值无效的【bucket】。
// p1:哈希表，p2:访问的元素
#define ZEND_HASH_MAP_FOREACH_VAL_IND(ht, _val) \
	/* 遍历哈希表（不支持顺序数组）。p1:哈希表，p2:是否跳过引用目标值无效的【bucket】 */ \
	ZEND_HASH_MAP_FOREACH(ht, 1); \
	/* 供外部访问 */ \
	_val = _z;

// ing4, 倒序遍历哈希表（不支持顺序数组）,跳过引用目标值无效的【bucket】。
// p1:哈希表，p2:访问的元素
#define ZEND_HASH_MAP_REVERSE_FOREACH_VAL_IND(ht, _val) \
	/* 倒序遍历哈希表（不支持顺序数组）。p1:哈希表，p2:是否跳过引用目标值无效的【bucket】 */ \
	ZEND_HASH_MAP_REVERSE_FOREACH(ht, 1); \
	/* 供外部访问 */ \
	_val = _z;

// ing4, 遍历哈希表（不支持顺序数组）,不跳过引用目标值无效的【bucket】。
// p1:哈希表，p2:访问的指针元素
#define ZEND_HASH_MAP_FOREACH_PTR(ht, _ptr) \
	/* 遍历哈希表（不支持顺序数组）。p1:哈希表，p2:是否跳过引用目标值无效的【bucket】 */ \
	ZEND_HASH_MAP_FOREACH(ht, 0); \
	/* 供外部访问 */ \
	_ptr = Z_PTR_P(_z);

// ing4, 从指定位置开始遍历哈希表（不支持顺序数组）,不跳过引用目标值无效的【bucket】。
// p1:哈希表，p2:访问的指针元素，p3:开始位置
#define ZEND_HASH_MAP_FOREACH_PTR_FROM(ht, _ptr, _from) \
	/* 从指定位置开始遍历哈希表（不支持顺序数组）。p1:哈希表，p2:是否跳过引用目标值无效的【bucket】，p3:开始序号 */ \
	ZEND_HASH_MAP_FOREACH_FROM(ht, 0, _from); \
	/* 供外部访问 */ \
	_ptr = Z_PTR_P(_z);

// ing4, 遍历哈希表（不支持顺序数组）,不跳过引用目标值无效的【bucket】。
// p1:哈希表，p2:访问的指针元素
#define ZEND_HASH_MAP_REVERSE_FOREACH_PTR(ht, _ptr) \
	/* 倒序遍历哈希表（不支持顺序数组）。p1:哈希表，p2:是否跳过引用目标值无效的【bucket】 */ \
	ZEND_HASH_MAP_REVERSE_FOREACH(ht, 0); \
	/* 供外部访问 */ \
	_ptr = Z_PTR_P(_z);

// ing4, 遍历哈希表（不支持顺序数组）,不跳过引用目标值无效的【bucket】。
// p1:哈希表，p2:访问的哈希值
#define ZEND_HASH_MAP_FOREACH_NUM_KEY(ht, _h) \
	/* 遍历哈希表（不支持顺序数组）。p1:哈希表，p2:是否跳过引用目标值无效的【bucket】 */ \
	ZEND_HASH_MAP_FOREACH(ht, 0); \
	/* 供外部访问 */ \
	_h = _p->h;

// ing4, 倒序遍历哈希表（不支持顺序数组）,不跳过引用目标值无效的【bucket】。
// p1:哈希表，p2:访问的哈希值
#define ZEND_HASH_MAP_REVERSE_FOREACH_NUM_KEY(ht, _h) \
	/* 倒序遍历哈希表（不支持顺序数组）。p1:哈希表，p2:是否跳过引用目标值无效的【bucket】 */ \
	ZEND_HASH_MAP_REVERSE_FOREACH(ht, 0); \
	/* 供外部访问 */ \
	_h = _p->h;

// ing4, 遍历哈希表（不支持顺序数组）,不跳过引用目标值无效的【bucket】。
// p1:哈希表，p2:访问的key
#define ZEND_HASH_MAP_FOREACH_STR_KEY(ht, _key) \
	/* 遍历哈希表（不支持顺序数组）。p1:哈希表，p2:是否跳过引用目标值无效的【bucket】 */ \
	ZEND_HASH_MAP_FOREACH(ht, 0); \
	/* 供外部访问 */ \
	_key = _p->key;

// ing4, 倒序遍历哈希表（不支持顺序数组）,不跳过引用目标值无效的【bucket】。
// p1:哈希表，p2:访问的key
#define ZEND_HASH_MAP_REVERSE_FOREACH_STR_KEY(ht, _key) \
	/* 倒序遍历哈希表（不支持顺序数组）。p1:哈希表，p2:是否跳过引用目标值无效的【bucket】 */ \
	ZEND_HASH_MAP_REVERSE_FOREACH(ht, 0); \
	/* 供外部访问 */ \
	_key = _p->key;

// ing4, 遍历哈希表（不支持顺序数组）,不跳过引用目标值无效的【bucket】。
// p1:哈希表，p2:访问的哈希值, p3:访问的key
#define ZEND_HASH_MAP_FOREACH_KEY(ht, _h, _key) \
	/* 遍历哈希表（不支持顺序数组）。p1:哈希表，p2:是否跳过引用目标值无效的【bucket】 */ \
	ZEND_HASH_MAP_FOREACH(ht, 0); \
	/* 供外部访问 */ \
	_h = _p->h; \
	_key = _p->key;

// ing4, 倒序遍历哈希表（不支持顺序数组）,不跳过引用目标值无效的【bucket】。
// p1:哈希表，p2:访问的哈希值, p3:访问的key
#define ZEND_HASH_MAP_REVERSE_FOREACH_KEY(ht, _h, _key) \
	/* 倒序遍历哈希表（不支持顺序数组）。p1:哈希表，p2:是否跳过引用目标值无效的【bucket】 */ \
	ZEND_HASH_MAP_REVERSE_FOREACH(ht, 0); \
	/* 供外部访问 */ \
	_h = _p->h; \
	_key = _p->key;

// ing4, 遍历哈希表（不支持顺序数组）,不跳过引用目标值无效的【bucket】。
// p1:哈希表，p2:访问的哈希值, p3:访问的元素值
#define ZEND_HASH_MAP_FOREACH_NUM_KEY_VAL(ht, _h, _val) \
	/* 遍历哈希表（不支持顺序数组）。p1:哈希表，p2:是否跳过引用目标值无效的【bucket】 */ \
	ZEND_HASH_MAP_FOREACH(ht, 0); \
	/* 供外部访问 */ \
	_h = _p->h; \
	_val = _z;

// ing4, 倒序遍历哈希表（不支持顺序数组）,不跳过引用目标值无效的【bucket】。
// p1:哈希表，p2:访问的哈希值, p3:访问的元素值
#define ZEND_HASH_MAP_REVERSE_FOREACH_NUM_KEY_VAL(ht, _h, _val) \
	/* 倒序遍历哈希表（不支持顺序数组）。p1:哈希表，p2:是否跳过引用目标值无效的【bucket】 */ \
	ZEND_HASH_MAP_REVERSE_FOREACH(ht, 0); \
	/* 供外部访问 */ \
	_h = _p->h; \
	_val = _z;

// ing4, 遍历哈希表（不支持顺序数组）,不跳过引用目标值无效的【bucket】。
// p1:哈希表，p2:访问的key, p3:访问的元素值
#define ZEND_HASH_MAP_FOREACH_STR_KEY_VAL(ht, _key, _val) \
	/* 遍历哈希表（不支持顺序数组）。p1:哈希表，p2:是否跳过引用目标值无效的【bucket】 */ \
	ZEND_HASH_MAP_FOREACH(ht, 0); \
	/* 供外部访问 */ \
	_key = _p->key; \
	_val = _z;

// ing4, 从指定位置开始遍历哈希表（不支持顺序数组）,不跳过引用目标值无效的【bucket】。
// p1:哈希表，p2:访问的key, p3:访问的元素值, p4:开始位置
#define ZEND_HASH_MAP_FOREACH_STR_KEY_VAL_FROM(ht, _key, _val, _from) \
	/* 从指定位置开始遍历哈希表（不支持顺序数组）。p1:哈希表，p2:是否跳过引用目标值无效的【bucket】，p3:开始序号 */ \
	ZEND_HASH_MAP_FOREACH_FROM(ht, 0, _from); \
	/* 供外部访问 */ \
	_key = _p->key; \
	_val = _z;

// ing4, 倒序遍历哈希表（不支持顺序数组）,不跳过引用目标值无效的【bucket】。
// p1:哈希表，p2:访问的key, p3:访问的元素值
#define ZEND_HASH_MAP_REVERSE_FOREACH_STR_KEY_VAL(ht, _key, _val) \
	/* 倒序遍历哈希表（不支持顺序数组）。p1:哈希表，p2:是否跳过引用目标值无效的【bucket】 */ \
	ZEND_HASH_MAP_REVERSE_FOREACH(ht, 0); \
	/* 供外部访问 */ \
	_key = _p->key; \
	_val = _z;

// ing4, 遍历哈希表（不支持顺序数组）,不跳过引用目标值无效的【bucket】。
// p1:哈希表，p2:访问的key，p3:访问的key, p4:访问的指针元素
#define ZEND_HASH_MAP_FOREACH_KEY_VAL(ht, _h, _key, _val) \
	/* 遍历哈希表（不支持顺序数组）。p1:哈希表，p2:是否跳过引用目标值无效的【bucket】 */ \
	ZEND_HASH_MAP_FOREACH(ht, 0); \
	/* 供外部访问 */ \
	_h = _p->h; \
	_key = _p->key; \
	_val = _z;

// ing4, 倒序遍历哈希表（不支持顺序数组）,不跳过引用目标值无效的【bucket】。
// p1:哈希表，p2:访问的哈希值，p3:访问的key, p4:访问的元素
#define ZEND_HASH_MAP_REVERSE_FOREACH_KEY_VAL(ht, _h, _key, _val) \
	/* 倒序遍历哈希表（不支持顺序数组）。p1:哈希表，p2:是否跳过引用目标值无效的【bucket】 */ \
	ZEND_HASH_MAP_REVERSE_FOREACH(ht, 0); \
	/* 供外部访问 */ \
	_h = _p->h; \
	_key = _p->key; \
	_val = _z;

// ing4, 遍历哈希表（不支持顺序数组）,跳过引用目标值无效的【bucket】。
// p1:哈希表，p2:访问的key, p3:访问的元素值
#define ZEND_HASH_MAP_FOREACH_STR_KEY_VAL_IND(ht, _key, _val) \
	/* 遍历哈希表（不支持顺序数组）。p1:哈希表，p2:是否跳过引用目标值无效的【bucket】 */ \
	ZEND_HASH_MAP_FOREACH(ht, 1); \
	/* 供外部访问 */ \
	_key = _p->key; \
	_val = _z;

// ing4, 倒序遍历哈希表（不支持顺序数组）,跳过引用目标值无效的【bucket】。
// p1:哈希表，p2:访问的key, p3:访问的元素值
#define ZEND_HASH_MAP_REVERSE_FOREACH_STR_KEY_VAL_IND(ht, _key, _val) \
	/* 倒序遍历哈希表（不支持顺序数组）。p1:哈希表，p2:是否跳过引用目标值无效的【bucket】 */ \
	ZEND_HASH_MAP_REVERSE_FOREACH(ht, 1); \
	/* 供外部访问 */ \
	_key = _p->key; \
	_val = _z;

// ing4, 遍历哈希表（不支持顺序数组）,跳过引用目标值无效的【bucket】。
// p1:哈希表，p2:访问的哈希值，p3:访问的key, p4:访问的指针元素
#define ZEND_HASH_MAP_FOREACH_KEY_VAL_IND(ht, _h, _key, _val) \
	/* 遍历哈希表（不支持顺序数组）。p1:哈希表，p2:是否跳过引用目标值无效的【bucket】 */ \
	ZEND_HASH_MAP_FOREACH(ht, 1); \
	/* 供外部访问 */ \
	_h = _p->h; \
	_key = _p->key; \
	_val = _z;

// ing4, 倒序遍历哈希表（不支持顺序数组）,跳过引用目标值无效的【bucket】。
// p1:哈希表，p2:访问的哈希值，p3:访问的key, p4:访问的指针元素
#define ZEND_HASH_MAP_REVERSE_FOREACH_KEY_VAL_IND(ht, _h, _key, _val) \
	/* 倒序遍历哈希表（不支持顺序数组）。p1:哈希表，p2:是否跳过引用目标值无效的【bucket】 */ \
	ZEND_HASH_MAP_REVERSE_FOREACH(ht, 1); \
	/* 供外部访问 */ \
	_h = _p->h; \
	_key = _p->key; \
	_val = _z;

// ing4, 遍历哈希表（不支持顺序数组）,不跳过引用目标值无效的【bucket】。
// p1:哈希表，p2:访问的哈希值, p3:访问的指针元素
#define ZEND_HASH_MAP_FOREACH_NUM_KEY_PTR(ht, _h, _ptr) \
	/* 遍历哈希表（不支持顺序数组）。p1:哈希表，p2:是否跳过引用目标值无效的【bucket】 */ \
	ZEND_HASH_MAP_FOREACH(ht, 0); \
	/* 供外部访问 */ \
	_h = _p->h; \
	_ptr = Z_PTR_P(_z);

// ing4, 倒序遍历哈希表（不支持顺序数组）,不跳过引用目标值无效的【bucket】。
// p1:哈希表，p2:访问的哈希值, p3:访问的指针元素
#define ZEND_HASH_MAP_REVERSE_FOREACH_NUM_KEY_PTR(ht, _h, _ptr) \
	/* 倒序遍历哈希表（不支持顺序数组）。p1:哈希表，p2:是否跳过引用目标值无效的【bucket】 */ \
	ZEND_HASH_MAP_REVERSE_FOREACH(ht, 0); \
	/* 供外部访问 */ \
	_h = _p->h; \
	_ptr = Z_PTR_P(_z);

// ing4, 遍历哈希表（不支持顺序数组）,不跳过引用目标值无效的【bucket】。
// p1:哈希表，p2:访问的key, p3:访问的指针元素
#define ZEND_HASH_MAP_FOREACH_STR_KEY_PTR(ht, _key, _ptr) \
	/* 遍历哈希表（不支持顺序数组）。p1:哈希表，p2:是否跳过引用目标值无效的【bucket】 */ \
	ZEND_HASH_MAP_FOREACH(ht, 0); \
	/* 供外部访问 */ \
	_key = _p->key; \
	_ptr = Z_PTR_P(_z);

// ing4, 倒序遍历哈希表（不支持顺序数组）,不跳过引用目标值无效的【bucket】。
// p1:哈希表，p2:访问的key, p3:访问的指针元素
#define ZEND_HASH_MAP_REVERSE_FOREACH_STR_KEY_PTR(ht, _key, _ptr) \
	/* 倒序遍历哈希表（不支持顺序数组）。p1:哈希表，p2:是否跳过引用目标值无效的【bucket】 */ \
	ZEND_HASH_MAP_REVERSE_FOREACH(ht, 0); \
	/* 供外部访问 */ \
	_key = _p->key; \
	_ptr = Z_PTR_P(_z);

// ing4, 遍历哈希表（不支持顺序数组）,不跳过引用目标值无效的【bucket】。
// p1:哈希表，p2:访问的key，p3:访问的key, p4:访问的指针元素
#define ZEND_HASH_MAP_FOREACH_KEY_PTR(ht, _h, _key, _ptr) \
	/* 遍历哈希表（不支持顺序数组）。p1:哈希表，p2:是否跳过引用目标值无效的【bucket】 */ \
	ZEND_HASH_MAP_FOREACH(ht, 0); \
	/* 供外部访问 */ \
	_h = _p->h; \
	_key = _p->key; \
	_ptr = Z_PTR_P(_z);

// ing4, 倒序遍历哈希表（不支持顺序数组）,不跳过引用目标值无效的【bucket】。
// p1:哈希表，p2:访问的哈希值，p3:访问的key, p4:访问的指针元素
#define ZEND_HASH_MAP_REVERSE_FOREACH_KEY_PTR(ht, _h, _key, _ptr) \
	/* 倒序遍历哈希表（不支持顺序数组）。p1:哈希表，p2:是否跳过引用目标值无效的【bucket】 */ \
	ZEND_HASH_MAP_REVERSE_FOREACH(ht, 0); \
	/* 供外部访问 */ \
	_h = _p->h; \
	_key = _p->key; \
	_ptr = Z_PTR_P(_z);

// ing3, 从指定位置开始遍历 顺序数组-> arPacked 。p1:哈希表，p2:指定位置
/* Packed array iterators */
#define ZEND_HASH_PACKED_FOREACH_FROM(_ht, _from) do { \
		/* 哈希表指针 */ \
		HashTable *__ht = (_ht); \
		/* 开始序号 */ \
		zend_ulong _idx = (_from); \
		/* 开始位置 */ \
		zval *_z = __ht->arPacked + (_from); \
		/* 结束位置 */ \
		zval *_end = __ht->arPacked + __ht->nNumUsed; \
		/* 必须是顺序数组 */ \
		ZEND_ASSERT(HT_IS_PACKED(__ht)); \
		/* 依次遍历每个元素 */ \
		for (;_z != _end; _z++, _idx++) { \
			/* ？？ */ \
			(void) _idx; \
			/* 如果元素是 未定义，跳过 */ \
			if (UNEXPECTED(Z_TYPE_P(_z) == IS_UNDEF)) continue;

// ing4, 遍历 顺序数组-> arPacked 。p1:哈希表
// 从指定位置开始遍历 顺序数组-> arPacked 。p1:哈希表，p2:指定位置
#define ZEND_HASH_PACKED_FOREACH(_ht) ZEND_HASH_PACKED_FOREACH_FROM(_ht, 0)


// ing3, 倒序遍历数组。p1:哈希表
#define ZEND_HASH_PACKED_REVERSE_FOREACH(_ht) do { \
		/* 哈希表指针 */ \
		HashTable *__ht = (_ht); \
		/* 结束位置 */ \
		zend_ulong _idx = __ht->nNumUsed; \
		/* 开始位置 */ \
		zval *_z = __ht->arPacked + _idx; \
		/* 必须是顺序数组 */ \
		ZEND_ASSERT(HT_IS_PACKED(__ht)); \
		/* 倒序遍历每个元素 */ \
		while (_idx > 0) { \
			/* 元素指针左移 */ \
			_z--; \
			/* 序号-1 */ \
			_idx--; \
			/* ？？ */ \
			(void) _idx; \
			/* 如果元素是 未定义，跳过 */ \
			if (UNEXPECTED(Z_TYPE_P(_z) == IS_UNDEF)) continue;

// ing4, 遍历 顺序数组-> arPacked。p1:哈希表，p2:访问的元素值
#define ZEND_HASH_PACKED_FOREACH_VAL(ht, _val) \
	/* 遍历 顺序数组-> arPacked 。p1:哈希表 */ \
	ZEND_HASH_PACKED_FOREACH(ht); \
	/* 供外部访问 */ \
	_val = _z;

// ing4, 倒序遍历数组。p1:哈希表，p2:访问的元素值
#define ZEND_HASH_PACKED_REVERSE_FOREACH_VAL(ht, _val) \
	/* 倒序遍历数组。p1:哈希表 */ \
	ZEND_HASH_PACKED_REVERSE_FOREACH(ht); \
	/* 供外部访问 */ \
	_val = _z;

// ing4, 遍历 顺序数组-> arPacked。p1:哈希表，p2:访问的指针元素值
#define ZEND_HASH_PACKED_FOREACH_PTR(ht, _ptr) \
	/* 遍历 顺序数组-> arPacked 。p1:哈希表 */ \
	ZEND_HASH_PACKED_FOREACH(ht); \
	/* 供外部访问 */ \
	_ptr = Z_PTR_P(_z);

// ing4, 倒序遍历数组。p1:哈希表，p2:访问的指针元素值
#define ZEND_HASH_PACKED_REVERSE_FOREACH_PTR(ht, _ptr) \
	/* 倒序遍历数组。p1:哈希表 */ \
	ZEND_HASH_PACKED_REVERSE_FOREACH(ht); \
	/* 供外部访问 */ \
	_ptr = Z_PTR_P(_z);

// ing4, 遍历 顺序数组-> arPacked。p1:哈希表，p2:访问的哈希值
#define ZEND_HASH_PACKED_FOREACH_KEY(ht, _h) \
	/* 遍历 顺序数组-> arPacked。p1:哈希表 */ \
	ZEND_HASH_PACKED_FOREACH(ht); \
	/* 供外部访问 */ \
	_h = _idx;

// ing4, 倒序遍历数组。p1:哈希表，p2:访问的哈希值
#define ZEND_HASH_PACKED_REVERSE_FOREACH_KEY(ht, _h) \
	/* 倒序遍历数组。p1:哈希表 */ \
	ZEND_HASH_PACKED_REVERSE_FOREACH(ht); \
	/* 供外部访问 */ \
	_h = _idx;

// ing4, 遍历 顺序数组-> arPacked。p1:哈希表，p2:访问的哈希值，p2:访问的元素值
#define ZEND_HASH_PACKED_FOREACH_KEY_VAL(ht, _h, _val) \
	/* 遍历 顺序数组-> arPacked 。p1:哈希表 */ \
	ZEND_HASH_PACKED_FOREACH(ht); \
	/* 供外部访问 */ \
	_h = _idx; \
	_val = _z;

// ing4, 倒序遍历数组。p1:哈希表，p2:访问的哈希值，p2:访问的元素值
#define ZEND_HASH_PACKED_REVERSE_FOREACH_KEY_VAL(ht, _h, _val) \
	/* 倒序遍历数组。p1:哈希表 */ \
	ZEND_HASH_PACKED_REVERSE_FOREACH(ht); \
	/* 供外部访问 */ \
	_h = _idx; \
	_val = _z;

// ing4, 遍历 顺序数组-> arPacked。p1:哈希表，p2:访问的哈希值，p2:访问的指针元素值
#define ZEND_HASH_PACKED_FOREACH_KEY_PTR(ht, _h, _ptr) \
	/* 遍历 顺序数组-> arPacked 。p1:哈希表 */ \
	ZEND_HASH_PACKED_FOREACH(ht); \
	/* 供外部访问 */ \
	_h = _idx; \
	_ptr = Z_PTR_P(_z);

// ing4, 倒序遍历数组。p1:哈希表，p2:访问的哈希值，p2:访问的指针元素值
#define ZEND_HASH_PACKED_REVERSE_FOREACH_KEY_PTR(ht, _h, _ptr) \
	/* 倒序遍历数组。p1:哈希表 */ \
	ZEND_HASH_PACKED_REVERSE_FOREACH(ht); \
	/* 供外部访问 */ \
	_h = _idx; \
	_ptr = Z_PTR_P(_z);

// 给数组 添加 一串新元素时 下面的宏会非常有用。
// 这可以替代 一系列的 zend_hash_next_index_insert_new() 操作（哈希表里必须e 足够的空闲 buckets）
/* The following macros are useful to insert a sequence of new elements
 * of packed array. They may be used instead of series of
 * zend_hash_next_index_insert_new()
 * (HashTable must have enough free buckets).
 */
 
// ing4, 准备填充哈希表
#define ZEND_HASH_FILL_PACKED(ht) do { \
		/* 哈希表 */ \
		HashTable *__fill_ht = (ht); \
		/* 最后一个元素 zval */ \
		zval *__fill_val = __fill_ht->arPacked + __fill_ht->nNumUsed; \
		/* 序号为现有元素数 */ \
		uint32_t __fill_idx = __fill_ht->nNumUsed; \
		/* 必须是顺序数组 */ \
		ZEND_ASSERT(HT_FLAGS(__fill_ht) & HASH_FLAG_PACKED);

// ing4, 如果空间不够，给数组增加空间
#define ZEND_HASH_FILL_GROW() do { \
		/* 如果要操作的序号在分配大小外 */ \
		if (UNEXPECTED(__fill_idx >= __fill_ht->nTableSize)) { \
			/* 更新已使用数量（有些可能是未定义元素） */ \
			__fill_ht->nNumUsed = __fill_idx; \
			/* 有效元素数 */ \
			__fill_ht->nNumOfElements = __fill_idx; \
			/* 下一个空闲位置 */ \
			__fill_ht->nNextFreeElement = __fill_idx; \
			/* 哈希表增加空间 */ \
			zend_hash_packed_grow(__fill_ht); \
			/* 最后一个元素 */ \
			__fill_val = __fill_ht->arPacked + __fill_idx; \
		} \
	} while (0);

// ing4, 设置填充元素。p1:填充元素
#define ZEND_HASH_FILL_SET(_val) \
		/* 把值复制到临时变量 */ \
		ZVAL_COPY_VALUE(__fill_val, _val)

// ing4, 设置填充元素，值为null
#define ZEND_HASH_FILL_SET_NULL() \
		ZVAL_NULL(__fill_val)

// ing4, 设置填充元素，值为整数
#define ZEND_HASH_FILL_SET_LONG(_val) \
		ZVAL_LONG(__fill_val, _val)

// ing4, 设置填充元素，值为小数
#define ZEND_HASH_FILL_SET_DOUBLE(_val) \
		ZVAL_DOUBLE(__fill_val, _val)

// ing4, 设置填充元素，值为字串
#define ZEND_HASH_FILL_SET_STR(_val) \
		ZVAL_STR(__fill_val, _val)

// ing4, 设置填充元素，值为字串副本
#define ZEND_HASH_FILL_SET_STR_COPY(_val) \
		ZVAL_STR_COPY(__fill_val, _val)

// ing4, 设置填充元素，值为内置字串
#define ZEND_HASH_FILL_SET_INTERNED_STR(_val) \
		ZVAL_INTERNED_STR(__fill_val, _val)

// ing4, 切换找到下一个填充位置和索引号
#define ZEND_HASH_FILL_NEXT() do {\
		/* 下一个zval */ \
		__fill_val++; \
		/* 下一个编号 */ \
		__fill_idx++; \
	} while (0)

// ing4, 用给出的值填充，并切换到下一个元素。p1:填充值
#define ZEND_HASH_FILL_ADD(_val) do { \
		/* 填充值 */ \
		ZEND_HASH_FILL_SET(_val); \
		/* 切换到下一个位置 */ \
		ZEND_HASH_FILL_NEXT(); \
	} while (0)

// ing4, 填充完毕
#define ZEND_HASH_FILL_FINISH() do { \
		/* 使用数 = 最后使用编号 */ \
		__fill_ht->nNumUsed = __fill_idx; \
		/* 有效元素数 = 最后使用编号 */ \
		__fill_ht->nNumOfElements = __fill_idx; \
		/* 下一个可以元素编号 = 最后使用编号 */ \
		__fill_ht->nNextFreeElement = __fill_idx; \
		/* 内置指针指向开头 */ \
		__fill_ht->nInternalPointer = 0; \
	} while (0)

// ing4, 填充完毕，ZEND_HASH_FILL_FINISH 外面再加一个 } while (0)
#define ZEND_HASH_FILL_END() \
		ZEND_HASH_FILL_FINISH(); \
	} while (0)

// ing4, 检查是否是list，无元素或者序号是从0开始并且连续的。p1:哈希表
/* Check if an array is a list */
static zend_always_inline bool zend_array_is_list(zend_array *array)
{
	zend_ulong expected_idx = 0;
	zend_ulong num_idx;
	zend_string* str_idx;
	// 空哈希表是list
	/* Empty arrays are lists */
	if (zend_hash_num_elements(array) == 0) {
		// 返回是
		return 1;
	}

	// 顺序数组，如果没有空元素，是list
	/* Packed arrays are lists */
	if (HT_IS_PACKED(array)) {
		// 里面没有空元素，返回 true
		if (HT_IS_WITHOUT_HOLES(array)) {
			// 返回是
			return 1;
		}
		// 检查哈希表在理论上是否可以 repack
		/* Check if the list could theoretically be repacked */
		// 当成顺序数组来迭代 -> ZEND_HASH_PACKED_FOREACH_FROM 
		ZEND_HASH_PACKED_FOREACH_KEY(array, num_idx) {
			// 如果 num_idx 不是从0开始并且连续的
			if (num_idx != expected_idx++) {
				// 返回否
				return 0;
			}
		} ZEND_HASH_FOREACH_END();
	// 哈希表
	} else {
		// 检查哈希表在理论上是否可以 repack
		/* Check if the list could theoretically be repacked */
		ZEND_HASH_MAP_FOREACH_KEY(array, num_idx, str_idx) {
			// 如果 num_idx 不是从0开始并且连续的
			if (str_idx != NULL || num_idx != expected_idx++) {
				// 返回否
				return 0;
			}
		} ZEND_HASH_FOREACH_END();
	}

	// 返回是
	return 1;
}

// ing3, hash 表中成对添加 key 和 zval
static zend_always_inline zval *_zend_hash_append_ex(HashTable *ht, zend_string *key, zval *zv, bool interned)
{
	uint32_t idx = ht->nNumUsed++; // 使用数+1
	uint32_t nIndex;
	Bucket *p = ht->arData + idx; // 新元素指针
	ZVAL_COPY_VALUE(&p->val, zv); // 复制 zval 给 Bucket
	// 如果 不是保留字调用，key 也不是保留字
	if (!interned && !ZSTR_IS_INTERNED(key)) {
		HT_FLAGS(ht) &= ~HASH_FLAG_STATIC_KEYS; // 删除 HASH_FLAG_STATIC_KEYS 标记
		zend_string_addref(key); // key 添加引用数，hash
		zend_string_hash_val(key); // key 计算哈希值
	}
	p->key = key; // key 和 hash 复制给新元素p
	p->h = ZSTR_H(key);
	nIndex = (uint32_t)p->h | ht->nTableMask; // 使用取得哈希索引表中的元素序号
	// 新元素的next指向 位置上的旧序号
	// 找到 (p1->arHash) 哈希->索引列表 中的指定索引号。p1:哈希表，p2:顺序号（和mask做过或运算, 负数）
	Z_NEXT(p->val) = HT_HASH(ht, nIndex);
	// 新序号存到位置上
	// 找到 (p1->arHash) 哈希->索引列表 中的指定索引号。p1:哈希表，p2:顺序号（和mask做过或运算, 负数）
	// 64位系统中 index 不用转换
	HT_HASH(ht, nIndex) = HT_IDX_TO_HASH(idx);
	ht->nNumOfElements++; // 有效元素数 +1
	return &p->val;
}

// ing4, hash 表中成对添加 key 和 zval
static zend_always_inline zval *_zend_hash_append(HashTable *ht, zend_string *key, zval *zv)
{
	return _zend_hash_append_ex(ht, key, zv, 0);
}

// ing3, 添加指针对象
static zend_always_inline zval *_zend_hash_append_ptr_ex(HashTable *ht, zend_string *key, void *ptr, bool interned)
{
	// 使用数+1
	uint32_t idx = ht->nNumUsed++;
	//
	uint32_t nIndex;
	// 取得对应的 Bucket
	Bucket *p = ht->arData + idx;

	// 设置 ptr
	ZVAL_PTR(&p->val, ptr);
	// 如果key 和value 都不是保留的
	if (!interned && !ZSTR_IS_INTERNED(key)) {
		// 删除静态key标记
		HT_FLAGS(ht) &= ~HASH_FLAG_STATIC_KEYS;
		// 添加引用次数
		zend_string_addref(key);
		// 创建哈希值
		zend_string_hash_val(key);
	}
	// Bucket 添加key
	p->key = key;
	// Bucket 添加哈希值
	p->h = ZSTR_H(key);
	// ? 任何数和 ht->nTableMask 做 | 运算都会取得一个很大的整数
	// 这个地方需要调试验证？
	nIndex = (uint32_t)p->h | ht->nTableMask;
	// p的next指向 这个 arHash 位置 原有的值
	// 找到 (p1->arHash) 哈希->索引列表 中的指定索引号。p1:哈希表，p2:顺序号（和mask做过或运算, 负数）
	Z_NEXT(p->val) = HT_HASH(ht, nIndex);
	// arHash 位置 指向当前元素索引号
	// 找到 (p1->arHash) 哈希->索引列表 中的指定索引号。p1:哈希表，p2:顺序号（和mask做过或运算, 负数）
	// 64位系统中 index 不用转换
	HT_HASH(ht, nIndex) = HT_IDX_TO_HASH(idx);
	// 元素数++
	ht->nNumOfElements++;
	// 返回添加成功的 值的 zval 对象
	return &p->val;
}

// ing3, 添加指针对象
static zend_always_inline zval *_zend_hash_append_ptr(HashTable *ht, zend_string *key, void *ptr)
{
	return _zend_hash_append_ptr_ex(ht, key, ptr, 0);
}

// ing3, 添加间接引用
static zend_always_inline void _zend_hash_append_ind(HashTable *ht, zend_string *key, zval *ptr)
{
	// 使用数+1
	uint32_t idx = ht->nNumUsed++;
	uint32_t nIndex;
	// 找到元素
	Bucket *p = ht->arData + idx;

	// 标记成 IS_INDIRECT，并添加指针 ptr
	ZVAL_INDIRECT(&p->val, ptr);
	// 如果key不是保留字 ?
	if (!ZSTR_IS_INTERNED(key)) {
		// 当前哈希表删除 HASH_FLAG_STATIC_KEYS 标记
		HT_FLAGS(ht) &= ~HASH_FLAG_STATIC_KEYS;
		// key 添加引用数，哈希
		zend_string_addref(key);
		zend_string_hash_val(key);
	}
	// 设置key, hash
	p->key = key;
	p->h = ZSTR_H(key);
	// 获取 arHash 位置
	nIndex = (uint32_t)p->h | ht->nTableMask;
	// 当前元素的next 指向位置上原有的序号
	// 找到 (p1->arHash) 哈希->索引列表 中的指定索引号。p1:哈希表，p2:顺序号（和mask做过或运算, 负数）
	Z_NEXT(p->val) = HT_HASH(ht, nIndex);
	// 找到 (p1->arHash) 哈希->索引列表 中的指定索引号。p1:哈希表，p2:顺序号（和mask做过或运算, 负数）
	// 64位系统中 index 不用转换
	HT_HASH(ht, nIndex) = HT_IDX_TO_HASH(idx);
	// 元素数+1
	ht->nNumOfElements++;
}

#endif							/* ZEND_HASH_H */
