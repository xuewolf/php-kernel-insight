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

#if defined(__aarch64__) || defined(_M_ARM64)
# include <arm_neon.h>
#endif

#ifdef __SSE2__
# include <mmintrin.h>
# include <emmintrin.h>
#endif

// debug才用到
#if ZEND_DEBUG
// ing3, 检查：表达式为真 或 哈希表有 HASH_FLAG_ALLOW_COW_VIOLATION 标记，调试用。p1:哈希表，p2:表达式
# define HT_ASSERT(ht, expr) \
	/* 表达式为真或 包含 HASH_FLAG_ALLOW_COW_VIOLATION 标记 */ \
	ZEND_ASSERT((expr) || (HT_FLAGS(ht) & HASH_FLAG_ALLOW_COW_VIOLATION))
// 正式环境
#else
// ing4, 空
# define HT_ASSERT(ht, expr)
#endif

// ing3, 断言，哈希表的引用次数为1 或 ALLOW_COW_VIOLATION ，调试用
#define HT_ASSERT_RC1(ht) HT_ASSERT(ht, GC_REFCOUNT(ht) == 1)

// ing4, 有毒的哈希表指针。（地址是-1，访问它肯定会报错）
#define HT_POISONED_PTR ((HashTable *) (intptr_t) -1)

// 调试用
#if ZEND_DEBUG

// 正常
#define HT_OK					0x00
// 正在销毁
#define HT_IS_DESTROYING		0x01
// 已销毁
#define HT_DESTROYED			0x02
// 正在清理
#define HT_CLEANING				0x03

// zend_hash.h 里定义，不一致状态的mask
// #define HASH_FLAG_CONSISTENCY      ((1<<0) | (1<<1))

// ing3, 检查哈希表是否不一致。有问题直接抛错
static void _zend_is_inconsistent(const HashTable *ht, const char *file, int line)
{
	// 如果没有一致性标记
	if ((HT_FLAGS(ht) & HASH_FLAG_CONSISTENCY) == HT_OK) {
		return;
	}
	// 按标记报错
	switch (HT_FLAGS(ht) & HASH_FLAG_CONSISTENCY) {
		// 正在销毁
		case HT_IS_DESTROYING:
			zend_output_debug_string(1, "%s(%d) : ht=%p is being destroyed", file, line, ht);
			break;
		// 已销毁
		case HT_DESTROYED:
			zend_output_debug_string(1, "%s(%d) : ht=%p is already destroyed", file, line, ht);
			break;
		// 正在清理
		case HT_CLEANING:
			zend_output_debug_string(1, "%s(%d) : ht=%p is being cleaned", file, line, ht);
			break;
		// 报错，哈希表不一致
		default:
			zend_output_debug_string(1, "%s(%d) : ht=%p is inconsistent", file, line, ht);
			break;
	}
	// # define ZEND_UNREACHABLE() ZEND_ASSUME(0)
	ZEND_UNREACHABLE();
}

// ing4, 检查哈希表是否不一致。有问题直接抛错。p1:哈希表
#define IS_CONSISTENT(a) _zend_is_inconsistent(a, __FILE__, __LINE__);

// ing4, 给哈希表 ht 添加不一致状态，p1:状态码
#define SET_INCONSISTENT(n) do { \
		/* 先清除不一致状态，再重新添加状态 */ \
		HT_FLAGS(ht) = (HT_FLAGS(ht) & ~HASH_FLAG_CONSISTENCY) | (n); \
	} while (0)

// 正式环境这两个东西是空的
#else
#define IS_CONSISTENT(a)
#define SET_INCONSISTENT(n)
#endif

// ing4, 重新设置哈希表大小
#define ZEND_HASH_IF_FULL_DO_RESIZE(ht)				\
	// 如果已用数量大于等于哈希表大小
	if ((ht)->nNumUsed >= (ht)->nTableSize) {		\
		// 调整大小，开辟新空间
		zend_hash_do_resize(ht);					\
	}

// ing3, 忽略大小写查找指定键
ZEND_API void *zend_hash_str_find_ptr_lc(const HashTable *ht, const char *str, size_t len) {
	void *result;
	char *lc_str;

	// 使用堆栈分配小字串来提升性能
	/* Stack allocate small strings to improve performance */
	ALLOCA_FLAG(use_heap)

	// # define do_alloca(p, use_heap)		emalloc(p)
	// 小写的key 
	lc_str = zend_str_tolower_copy(do_alloca(len + 1, use_heap), str, len);
	// 查找结果
	result = zend_hash_str_find_ptr(ht, lc_str, len);
	// efree, 释放小写键名
	free_alloca(lc_str, use_heap);

	return result;
}

// ing3, key转成小写，返回指针
ZEND_API void *zend_hash_find_ptr_lc(const HashTable *ht, zend_string *key) {
	void *result;
	// key 转成小写
	zend_string *lc_key = zend_string_tolower(key);
	// 在哈希表里查找
	result = zend_hash_find_ptr(ht, lc_key);
	zend_string_release(lc_key);
	return result;
}

static void ZEND_FASTCALL zend_hash_do_resize(HashTable *ht);

// ing4, 32位整数p1向上取整到2的幂
static zend_always_inline uint32_t zend_hash_check_size(uint32_t nSize)
{
// windows
#ifdef ZEND_WIN32
	unsigned long index;
#endif
	// nSize 用一个足够大的2的幂
	/* Use big enough power of 2 */
	// 数量在 HT_MIN_SIZE:8，HT_MAX_SIZE（32位：32M，64位：1G） 之间
	/* size should be between HT_MIN_SIZE and HT_MAX_SIZE */
	
	// 过小
	if (nSize <= HT_MIN_SIZE) {
		 // 给出的数量过小，（如果初始值是0，会走这里）
		return HT_MIN_SIZE;
	// 过大
	} else if (UNEXPECTED(nSize >= HT_MAX_SIZE)) {
		// 报错：内存不够分配 nSize+1 个 Bucket
		zend_error_noreturn(E_ERROR, "Possible integer overflow in memory allocation (%u * %zu + %zu)", nSize, sizeof(Bucket), sizeof(Bucket));
	}

// windows走这里
#ifdef ZEND_WIN32
	if (BitScanReverse(&index, nSize - 1)) {
		return 0x2u << ((31 - index) ^ 0x1f);
	} else {
		/* nSize is ensured to be in the valid range, fall back to it
		   rather than using an undefined bis scan result. */
		return nSize;
	}
// 。。。。
#elif (defined(__GNUC__) || __has_builtin(__builtin_clz))  && defined(PHP_HAVE_BUILTIN_CLZ)
	return 0x2u << (__builtin_clz(nSize - 1) ^ 0x1f);
// 其他情况，这里最能看出业务逻辑
#else
	nSize -= 1; // 左侧一定有1位是1（初始值是1才会没有1，，这种情况在前面过滤了）	
	nSize |= (nSize >> 1); // 运算后，左侧最多有2个连续的1	
	nSize |= (nSize >> 2); // 运算后，左侧最多有4个连续的1	
	nSize |= (nSize >> 4); // 运算后，左侧最多有8个连续的1	
	nSize |= (nSize >> 8); // 运算后，左侧最多有16个连续的1	
	// 运算后，左侧最多有31个连续的1（初始值是0才会有32个1，这种情况在前面过滤了）	
	nSize |= (nSize >> 16); 
	return nSize + 1; // 最后加1，变成2的幂
#endif
}

// ing3, 初始化顺序数组 
static zend_always_inline void zend_hash_real_init_packed_ex(HashTable *ht)
{
	// 数据指针
	void *data;
	
	// HT_MIN_MASK -2, HT_MIN_SIZE 8
	// 如果有持久标记，用 pemalloc 创建数据，大小为 ht->nTableSize
	if (UNEXPECTED(GC_FLAGS(ht) & IS_ARRAY_PERSISTENT)) {
		// 计算顺序数组的 数据大小: zval列表 大小 + 哈希->索引列表大小。p1:元素数，p2:mask
		data = pemalloc(HT_PACKED_SIZE_EX(ht->nTableSize, HT_MIN_MASK), 1);
	// 如果未指定大小 HT_MIN_SIZE 8
	} else if (EXPECTED(ht->nTableSize == HT_MIN_SIZE)) {
		/* Use specialized API with constant allocation amount for a particularly common case. */
		// 计算顺序数组的 数据大小: zval列表 大小 + 哈希->索引列表大小。p1:元素数，p2:mask
		data = emalloc(HT_PACKED_SIZE_EX(HT_MIN_SIZE, HT_MIN_MASK));
	// 如果指定了大小
	} else {
		// 计算顺序数组的 数据大小: zval列表 大小 + 哈希->索引列表大小。p1:元素数，p2:mask
		data = emalloc(HT_PACKED_SIZE_EX(ht->nTableSize, HT_MIN_MASK));
	}
	// 设置 p1->arData 到 Bucket 列表开头。p1:哈希表，p2: 内存块开头位置
	HT_SET_DATA_ADDR(ht, data);
	// 添加标记，不要覆盖迭代器数量
	/* Don't overwrite iterator count. */
	ht->u.v.flags = HASH_FLAG_PACKED | HASH_FLAG_STATIC_KEYS;
	// 顺序数组的 (p1)->arHash 哈希->索引列表 里有2个元素，都设置成 -1
	HT_HASH_RESET_PACKED(ht);
}

// ing3, 初始化混合哈希表，主要是：1.计算内存大小。2.分配内存，
// 3.计算 nTableMask , 4.添加 HASH_FLAG_STATIC_KEYS 标记，5. 设置 arHash
static zend_always_inline void zend_hash_real_init_mixed_ex(HashTable *ht)
{
	void *data;
	// 本身带有 nTableSize
	uint32_t nSize = ht->nTableSize;
	// mask必须有效
	// 通过哈希表大小 计算 mask = (uint32_t)(-2 * nTableSize)
	ZEND_ASSERT(HT_SIZE_TO_MASK(nSize));

	// 如果是持久数组
	if (UNEXPECTED(GC_FLAGS(ht) & IS_ARRAY_PERSISTENT)) {
		// 计算 哈希表 数据大小：Bucket 列表大小 + 哈希->索引 列表大小 。p1:哈希表元素数, p2:mask
		// 通过哈希表大小 计算 mask = (uint32_t)(-2 * nTableSize)
		data = pemalloc(HT_SIZE_EX(nSize, HT_SIZE_TO_MASK(nSize)), 1);
	// 否则，如果尺寸是最小尺寸, （这个分支自己return ）
	} else if (EXPECTED(nSize == HT_MIN_SIZE)) {
		// 计算 哈希表 数据大小：Bucket 列表大小 + 哈希->索引 列表大小 。p1:哈希表元素数, p2:mask
		// 通过哈希表大小 计算 mask = (uint32_t)(-2 * nTableSize)
		data = emalloc(HT_SIZE_EX(HT_MIN_SIZE, HT_SIZE_TO_MASK(HT_MIN_SIZE)));
		// 记录mask
		// 通过哈希表大小 计算 mask = (uint32_t)(-2 * nTableSize)
		ht->nTableMask = HT_SIZE_TO_MASK(HT_MIN_SIZE);
		// 设置 p1->arData 到 Bucket 列表开头。p1:哈希表，p2: 内存块开头位置
		HT_SET_DATA_ADDR(ht, data);
		// 不要覆盖迭代器数量
		/* Don't overwrite iterator count. */
		// 哈希表默认是 HASH_FLAG_STATIC_KEYS 状态,每个分支都有它
		ht->u.v.flags = HASH_FLAG_STATIC_KEYS;
		// ??? 这一段不影响主要逻辑
#ifdef __SSE2__
		do {
			// __m128i 是什么
			__m128i xmm0 = _mm_setzero_si128();
			xmm0 = _mm_cmpeq_epi8(xmm0, xmm0);
			_mm_storeu_si128((__m128i*)&HT_HASH_EX(data,  0), xmm0);
			_mm_storeu_si128((__m128i*)&HT_HASH_EX(data,  4), xmm0);
			_mm_storeu_si128((__m128i*)&HT_HASH_EX(data,  8), xmm0);
			_mm_storeu_si128((__m128i*)&HT_HASH_EX(data, 12), xmm0);
		} while (0);
#elif defined(__aarch64__) || defined(_M_ARM64)
		do {
			// 32*4位=128位。 vdupq_n_s32，一共只有这里用到，无定义
			int32x4_t t = vdupq_n_s32(-1);
			vst1q_s32((int32_t*)&HT_HASH_EX(data,  0), t);
			vst1q_s32((int32_t*)&HT_HASH_EX(data,  4), t);
			vst1q_s32((int32_t*)&HT_HASH_EX(data,  8), t);
			vst1q_s32((int32_t*)&HT_HASH_EX(data, 12), t);
		} while (0);
#else
		// 添加16个空元素
		HT_HASH_EX(data,  0) = -1;
		HT_HASH_EX(data,  1) = -1;
		HT_HASH_EX(data,  2) = -1;
		HT_HASH_EX(data,  3) = -1;
		HT_HASH_EX(data,  4) = -1;
		HT_HASH_EX(data,  5) = -1;
		HT_HASH_EX(data,  6) = -1;
		HT_HASH_EX(data,  7) = -1;
		HT_HASH_EX(data,  8) = -1;
		HT_HASH_EX(data,  9) = -1;
		HT_HASH_EX(data, 10) = -1;
		HT_HASH_EX(data, 11) = -1;
		HT_HASH_EX(data, 12) = -1;
		HT_HASH_EX(data, 13) = -1;
		HT_HASH_EX(data, 14) = -1;
		HT_HASH_EX(data, 15) = -1;
#endif
		// 这个分支自己return，不调用 HT_HASH_RESET
		return;
	} else {
		// 计算 哈希表 数据大小：Bucket 列表大小 + 哈希->索引 列表大小 。p1:哈希表元素数, p2:mask
		// 通过哈希表大小 计算 mask = (uint32_t)(-2 * nTableSize)
		data = emalloc(HT_SIZE_EX(nSize, HT_SIZE_TO_MASK(nSize)));
	}
	// 如果 UNEXPECTED(GC_FLAGS(ht) & IS_ARRAY_PERSISTENT) 或 nSize ！= HT_MIN_SIZE 会走这里
	// 计算 mask = -2 * nSize
	// 通过哈希表大小 计算 mask = (uint32_t)(-2 * nTableSize)
	ht->nTableMask = HT_SIZE_TO_MASK(nSize);
	// 设置 p1->arData 到 Bucket 列表开头。p1:哈希表，p2: 内存块开头位置
	HT_SET_DATA_ADDR(ht, data);
	// 添加flag HASH_FLAG_STATIC_KEYS，哈希表默认是 HASH_FLAG_STATIC_KEYS 状态, 每个分支都有它
	HT_FLAGS(ht) = HASH_FLAG_STATIC_KEYS;
	// 设置 arHash , 这里是最重要的
	// memset(&HT_HASH(ht, (ht)->nTableMask), HT_INVALID_IDX, HT_HASH_SIZE((ht)->nTableMask))
	HT_HASH_RESET(ht);
}

// ing4, 初始化哈希表
static zend_always_inline void zend_hash_real_init_ex(HashTable *ht, bool packed)
{
	// 断言，哈希表的引用次数为1 或 ALLOW_COW_VIOLATION ，调试用
	HT_ASSERT_RC1(ht);
	// 断言，必须没初始化过
	ZEND_ASSERT(HT_FLAGS(ht) & HASH_FLAG_UNINITIALIZED);
	// 顺序数组
	if (packed) {
		// 初始化顺序数组
		zend_hash_real_init_packed_ex(ht);
	// 哈希表
	} else {
		// 初始化哈希表
		zend_hash_real_init_mixed_ex(ht);
	}
}

// ing1，空bucket 两个无效索引号， HT_MIN_MASK：((uint32_t) -2)
static const uint32_t uninitialized_bucket[-HT_MIN_MASK] =
	{HT_INVALID_IDX, HT_INVALID_IDX};

// ing3, 空哈希表，这样创建对象不错~ arHash 没用到
ZEND_API const HashTable zend_empty_array = {
	// 2次引用
	.gc.refcount = 2,
	// 可转换，数组
	.gc.u.type_info = IS_ARRAY | (GC_IMMUTABLE << GC_FLAGS_SHIFT),
	// 初始化过
	.u.flags = HASH_FLAG_UNINITIALIZED,
	// ((uint32_t) -2)
	.nTableMask = HT_MIN_MASK,
	// uninitialized_bucket 结尾
	.arData = (Bucket*)&uninitialized_bucket[2],
	//
	.nNumUsed = 0,
	//
	.nNumOfElements = 0,
	//  8
	.nTableSize = HT_MIN_SIZE,
	// 内部指针
	.nInternalPointer = 0,
	// 下一个空元素
	.nNextFreeElement = 0,
	// 销毁器
	.pDestructor = ZVAL_PTR_DTOR
};

// int应该是intern ?
// ing4, 初始化哈希表，只设置基础属性，未创建元素空间。p1:哈希表，p2:元素空间数量，p3:元素销毁器，p4:是否永久
static zend_always_inline void _zend_hash_init_int(HashTable *ht, uint32_t nSize, dtor_func_t pDestructor, bool persistent)
{
	// 引用 1 次
	GC_SET_REFCOUNT(ht, 1);
	// 类型信息，GC_ARRAY（IS_ARRAY） + （持久？持久+不可收集 ：0）
	GC_TYPE_INFO(ht) = GC_ARRAY | (persistent ? ((GC_PERSISTENT|GC_NOT_COLLECTABLE) << GC_FLAGS_SHIFT) : 0);
	// 标记成未初始化过
	HT_FLAGS(ht) = HASH_FLAG_UNINITIALIZED;
	// #define HT_MIN_MASK ((uint32_t) -2)
	ht->nTableMask = HT_MIN_MASK;
	// 设置 p1->arData 到 Bucket 列表开头。p1:哈希表，p2: 内存块开头位置
	HT_SET_DATA_ADDR(ht, &uninitialized_bucket);
	// 使用元素数
	ht->nNumUsed = 0;
	// 有效元素数
	ht->nNumOfElements = 0;
	// 内部指针
	ht->nInternalPointer = 0;
	// 下一个空元素
	ht->nNextFreeElement = ZEND_LONG_MIN;
	// 销毁器
	ht->pDestructor = pDestructor;
	// 根据操作系统设置大小
	ht->nTableSize = zend_hash_check_size(nSize);
}

// ing4, 初始化哈希表，只设置基础属性，未创建元素空间。p1:哈希表，p2:元素空间数量，p3:元素销毁器，p4:是否永久
ZEND_API void ZEND_FASTCALL _zend_hash_init(HashTable *ht, uint32_t nSize, dtor_func_t pDestructor, bool persistent)
{
	// 初始化哈希表，只设置基础属性，未创建元素空间。p1:哈希表，p2:元素空间数量，p3:元素销毁器，p4:是否永久
	_zend_hash_init_int(ht, nSize, pDestructor, persistent);
}

// ing3, 分配内存，创建 最小的 非持久哈希表
ZEND_API HashTable* ZEND_FASTCALL _zend_new_array_0(void)
{
	// 分配内存创建哈希表
	HashTable *ht = emalloc(sizeof(HashTable));
	// 初始化哈希表，只设置基础属性，未创建元素空间。p1:哈希表，p2:元素空间数量，p3:元素销毁器，p4:是否永久
	_zend_hash_init_int(ht, HT_MIN_SIZE, ZVAL_PTR_DTOR, 0);
	return ht;
}

// ing3, 分配内存，创建 nSize大小 非持久哈希表
ZEND_API HashTable* ZEND_FASTCALL _zend_new_array(uint32_t nSize)
{
	// 分配内存创建哈希表
	HashTable *ht = emalloc(sizeof(HashTable));
	// 初始化哈希表，只设置基础属性，未创建元素空间。p1:哈希表，p2:元素空间数量，p3:元素销毁器，p4:是否永久
	_zend_hash_init_int(ht, nSize, ZVAL_PTR_DTOR, 0);
	return ht;
}

// ing3, 创建一个包含两个元素的顺序数组
ZEND_API HashTable* ZEND_FASTCALL zend_new_pair(zval *val1, zval *val2)
{
	zval *zv;
	// 分配内存创建哈希表
	HashTable *ht = emalloc(sizeof(HashTable));
	// 初始化哈希表，只设置基础属性，未创建元素空间。p1:哈希表，p2:元素空间数量，p3:元素销毁器，p4:是否永久
	_zend_hash_init_int(ht, HT_MIN_SIZE, ZVAL_PTR_DTOR, 0);
	ht->nNumUsed = ht->nNumOfElements = ht->nNextFreeElement = 2;
	// 
	zend_hash_real_init_packed_ex(ht);
	//
	zv = ht->arPacked;
	ZVAL_COPY_VALUE(zv, val1);
	zv++;
	ZVAL_COPY_VALUE(zv, val2);
	return ht;
}

// ing4, 顺序数组重新分配内存，让数据空间翻倍。
	// 这里是按顺序数组计算大小的，但调用 HT_SET_DATA_ADDR 数据挂在 arData （bucket指针）上
ZEND_API void ZEND_FASTCALL zend_hash_packed_grow(HashTable *ht)
{
	// 断言，哈希表的引用次数为1 或 ALLOW_COW_VIOLATION ，调试用
	HT_ASSERT_RC1(ht);
	// 如果哈希表大小达到最大大小
	if (ht->nTableSize >= HT_MAX_SIZE) {
		// 报错: 内存溢出
		zend_error_noreturn(E_ERROR, "Possible integer overflow in memory allocation (%u * %zu + %zu)", ht->nTableSize * 2, sizeof(Bucket), sizeof(Bucket));
	}
	// 大小 翻倍
	ht->nTableSize += ht->nTableSize;
	// 设置 p1->arData 到 Bucket 列表开头。p1:哈希表，p2: 内存块开头位置
		// HT_GET_DATA_ADDR：找到原开头位置
		// HT_PACKED_SIZE_EX: 计算顺序数组的 数据大小: zval列表 大小 + 哈希->索引列表大小。p1:元素数，p2:mask
			// HT_MIN_MASK： 这里给哈希值列表 分配的是最小的大小。因为顺序数组用不到它。
		// HT_PACKED_USED_SIZE：顺序数组的已使用部分大小，哈希->索引列表 部分算已使用。p1:哈希表
	HT_SET_DATA_ADDR(ht, perealloc2(HT_GET_DATA_ADDR(ht), HT_PACKED_SIZE_EX(ht->nTableSize, HT_MIN_MASK), HT_PACKED_USED_SIZE(ht), GC_FLAGS(ht) & IS_ARRAY_PERSISTENT));
}

// ing4, 初始化哈希表：zend_hash_real_init_packed_ex(ht); 或 zend_hash_real_init_mixed_ex(ht);
ZEND_API void ZEND_FASTCALL zend_hash_real_init(HashTable *ht, bool packed)
{
	// 检查一致性，调试用
	IS_CONSISTENT(ht);

	// 断言，哈希表的引用次数为1 或 ALLOW_COW_VIOLATION ，调试用
	HT_ASSERT_RC1(ht);
	zend_hash_real_init_ex(ht, packed);
}

// ing3, 初始化顺序数组
ZEND_API void ZEND_FASTCALL zend_hash_real_init_packed(HashTable *ht)
{
	// 检查一致性，调试用
	IS_CONSISTENT(ht);
	// 断言，哈希表的引用次数为1 或 ALLOW_COW_VIOLATION ，调试用
	HT_ASSERT_RC1(ht);
	zend_hash_real_init_packed_ex(ht);
}

// ing3, 初始化哈希表
ZEND_API void ZEND_FASTCALL zend_hash_real_init_mixed(HashTable *ht)
{
	// 检查一致性，调试用
	IS_CONSISTENT(ht);
	// 断言，哈希表的引用次数为1 或 ALLOW_COW_VIOLATION ，调试用
	HT_ASSERT_RC1(ht);
	zend_hash_real_init_mixed_ex(ht);
}

// ing3， 顺序数组转哈希表。这个转换 删除 了顺序数组标记 HASH_FLAG_PACKED
ZEND_API void ZEND_FASTCALL zend_hash_packed_to_hash(HashTable *ht)
{
	// 键值对 指针
	void *new_data, *old_data = HT_GET_DATA_ADDR(ht);
	// 顺序数组指针
	zval *src = ht->arPacked;
	// 目标位置
	Bucket *dst;
	uint32_t i;
	// array大小
	uint32_t nSize = ht->nTableSize;
	// mask 必须有效
	// 通过哈希表大小 计算 mask = (uint32_t)(-2 * nTableSize)
	ZEND_ASSERT(HT_SIZE_TO_MASK(nSize));
	// 断言，哈希表的引用次数为1 或 ALLOW_COW_VIOLATION ，调试用
	HT_ASSERT_RC1(ht);
	// 删除顺序数组标记 HASH_FLAG_PACKED
	HT_FLAGS(ht) &= ~HASH_FLAG_PACKED;
	// 计算 哈希表 数据大小：Bucket 列表大小 + 哈希->索引 列表大小 。p1:哈希表元素数, p2:mask
	// 通过哈希表大小 计算 mask = (uint32_t)(-2 * nTableSize)
	new_data = pemalloc(HT_SIZE_EX(nSize, HT_SIZE_TO_MASK(nSize)), GC_FLAGS(ht) & IS_ARRAY_PERSISTENT);
	// 通过哈希表大小 计算 mask = (uint32_t)(-2 * nTableSize)
	ht->nTableMask = HT_SIZE_TO_MASK(ht->nTableSize);
	// 设置 p1->arData 到 Bucket 列表开头。p1:哈希表，p2: 内存块开头位置
	HT_SET_DATA_ADDR(ht, new_data);
	// 目标位置
	dst = ht->arData;
	// 遍历所有使用过的元素
	for (i = 0; i < ht->nNumUsed; i++) {
		// 复制值. .next 应该也在这里面了
		ZVAL_COPY_VALUE(&dst->val, src);
		// 哈希值设置成顺序号
		dst->h = i;
		// 没有键
		dst->key = NULL;
		// 下一个
		dst++;
		src++;
	}
	// 上面为什么没操作 arHash 呢
	// 删除旧的顺序数组元素
	pefree(old_data, GC_FLAGS(ht) & IS_ARRAY_PERSISTENT);
	// 整理元素，填补中间的空位，然后给所有元素更新 编号，哈希->顺序表，next指针。p1: 哈希表
	zend_hash_rehash(ht);
}

// ing3, 哈希表转顺序数组
ZEND_API void ZEND_FASTCALL zend_hash_to_packed(HashTable *ht)
{
	void *new_data, *old_data = HT_GET_DATA_ADDR(ht);
	Bucket *src = ht->arData;
	zval *dst;
	uint32_t i;
	// 断言，哈希表的引用次数为1 或 ALLOW_COW_VIOLATION ，调试用
	HT_ASSERT_RC1(ht);
	// 分配内存，创建顺序数据对象
	// 计算顺序数组的 数据大小: zval列表 大小 + 哈希->索引列表大小。p1:元素数，p2:mask
	new_data = pemalloc(HT_PACKED_SIZE_EX(ht->nTableSize, HT_MIN_MASK), GC_FLAGS(ht) & IS_ARRAY_PERSISTENT);
	// 添加标记，顺序数组，内置变量数组
	HT_FLAGS(ht) |= HASH_FLAG_PACKED | HASH_FLAG_STATIC_KEYS;
	// 顺序数组应该用不到 mask，因为mask是和哈希一起用的
	ht->nTableMask = HT_MIN_MASK;
	// 设置 p1->arData 到 Bucket 列表开头。p1:哈希表，p2: 内存块开头位置
	HT_SET_DATA_ADDR(ht, new_data);
	// 顺序数组的 (p1)->arHash 哈希->索引列表 里有2个元素，都设置成 -1
	HT_HASH_RESET_PACKED(ht);
	// 遍历原数据
	dst = ht->arPacked;
	// 把 键值对的值依次添加到顺序数组中（这里没检查空和IS_UNDEF）
	for (i = 0; i < ht->nNumUsed; i++) {
		ZVAL_COPY_VALUE(dst, &src->val);
		dst++;
		src++;
	}
	// 删除哈希表数据
	pefree(old_data, GC_FLAGS(ht) & IS_ARRAY_PERSISTENT);
}

// ing3, 把哈希表容量扩充到指定大小。p1:哈希表，p2:目标大小，p3：是否顺序数组
// 为什么顺序数组直接调整数据块大小，哈希表要重新分配内存呢？
ZEND_API void ZEND_FASTCALL zend_hash_extend(HashTable *ht, uint32_t nSize, bool packed)
{
	// 断言，哈希表的引用次数为1 或 ALLOW_COW_VIOLATION ，调试用
	HT_ASSERT_RC1(ht);

	// 如果目标大小是0 ，返回
	if (nSize == 0) return;
	
	// mask 必须有效
	// 通过哈希表大小 计算 mask = (uint32_t)(-2 * nTableSize)
	ZEND_ASSERT(HT_SIZE_TO_MASK(nSize));
	// 情况1：
	// 如果哈希表已经初始化过
	if (UNEXPECTED(HT_FLAGS(ht) & HASH_FLAG_UNINITIALIZED)) {
		// 如果目标大小 大于 现有大小
		if (nSize > ht->nTableSize) {
			// 根据操作系统类型返回合适的大小
			ht->nTableSize = zend_hash_check_size(nSize);
		}
		// 初始化
		zend_hash_real_init(ht, packed);
	// 情况2：
	// 没有初始化过
	} else {
		// 情况2.1：
		// 顺序数组
		if (packed) {
			// 必须是顺序数组
			ZEND_ASSERT(HT_IS_PACKED(ht));
			// 如果目标大小大于 现有大小
			if (nSize > ht->nTableSize) {
				// 根据操作系统类型返回合适的大小
				ht->nTableSize = zend_hash_check_size(nSize);
				// 设置 p1->arData 到 Bucket 列表开头。p1:哈希表，p2: 内存块开头位置
				// 只要 调整原数据块大小：
					// 计算顺序数组的 数据大小: zval列表 大小 + 哈希->索引列表大小。p1:元素数，p2:mask
					// 顺序数组的已使用部分大小，哈希->索引列表 部分算已使用。p1:哈希表
				HT_SET_DATA_ADDR(ht, perealloc2(HT_GET_DATA_ADDR(ht), HT_PACKED_SIZE_EX(ht->nTableSize, HT_MIN_MASK), HT_PACKED_USED_SIZE(ht), GC_FLAGS(ht) & IS_ARRAY_PERSISTENT));
			}
			
		// 情况2.1：
		// 哈希表
		} else {
			ZEND_ASSERT(!HT_IS_PACKED(ht));
			// 如果目标大小大于 现有大小
			if (nSize > ht->nTableSize) {
				// 准备释放的旧数据
				void *new_data, *old_data = HT_GET_DATA_ADDR(ht);
				// 原bucket表开头
				Bucket *old_buckets = ht->arData;
				// 根据操作系统类型返回合适的大小
				nSize = zend_hash_check_size(nSize);
				// 更新已分配元素数量
				ht->nTableSize = nSize;
				// 计算 哈希表 数据大小：Bucket 列表大小 + 哈希->索引 列表大小 。p1:哈希表元素数, p2:mask
				// 通过哈希表大小 计算 mask = (uint32_t)(-2 * nTableSize)
				new_data = pemalloc(HT_SIZE_EX(nSize, HT_SIZE_TO_MASK(nSize)), GC_FLAGS(ht) & IS_ARRAY_PERSISTENT);
				// 通过哈希表大小 计算 mask = (uint32_t)(-2 * nTableSize)
				ht->nTableMask = HT_SIZE_TO_MASK(ht->nTableSize);
				// 设置 p1->arData 到 Bucket 列表开头。p1:哈希表，p2: 内存块开头位置
				HT_SET_DATA_ADDR(ht, new_data);
				// 把旧 bucket表复制过来
				memcpy(ht->arData, old_buckets, sizeof(Bucket) * ht->nNumUsed);
				// 释放旧数据
				pefree(old_data, GC_FLAGS(ht) & IS_ARRAY_PERSISTENT);
				// 整理元素，填补中间的空位，然后给所有元素更新 编号，哈希->顺序表，next指针。p1: 哈希表
				zend_hash_rehash(ht);
			}
		}
	}
}

// ing3, 把数组截短到目标长度。p1:哈希表，p2:目标长度
ZEND_API void ZEND_FASTCALL zend_hash_discard(HashTable *ht, uint32_t nNumUsed)
{
	Bucket *p, *end, *arData;
	// 索引号。无符号整数
	uint32_t nIndex;
	// 不可以是顺序数组
	ZEND_ASSERT(!HT_IS_PACKED(ht));
	arData = ht->arData;
	// 最后一个元素
	p = arData + ht->nNumUsed;
	// 要求的结尾位置
	end = arData + nNumUsed;
	// 更新结尾位置
	ht->nNumUsed = nNumUsed;
	
	// 从右到左遍历全部
	while (p != end) {
		p--;
		// 跳过无效元素
		if (UNEXPECTED(Z_TYPE(p->val) == IS_UNDEF)) continue;
		// 减少元素数
		ht->nNumOfElements--;
		// 冲突指针总是指向从高到低的键值对
		/* Collision pointers always directed from higher to lower buckets */
// 废弃
#if 0
		if (!(Z_NEXT(p->val) == HT_INVALID_IDX || HT_HASH_TO_BUCKET_EX(arData, Z_NEXT(p->val)) < p)) {
			abort();
		}
#endif
		// 利用哈希值 和 mask 计算索引号位置（倒序，例如共16个位置，第5个位置算出 -11）
		nIndex = p->h | ht->nTableMask;
		// p的下一个元素放到指定位置
		// 找到哈希->索引号列表 中的指定索引号。p1:列表结尾位置，p2:顺序号（和 tableMask 做过或运算的哈希值, 负数）
		HT_HASH_EX(arData, nIndex) = Z_NEXT(p->val);
	}
}

// ing3, 取得非空元素数量，过滤掉内部元素中的未定义元素
static uint32_t zend_array_recalc_elements(HashTable *ht)
{
	zval *val;
	// 总数
	uint32_t num = ht->nNumOfElements;

	// 遍历所有元素
	ZEND_HASH_MAP_FOREACH_VAL(ht, val) {
		// 内部元素
		if (Z_TYPE_P(val) == IS_INDIRECT) {
			// 如果变量类型是未定义，总数减1
			if (UNEXPECTED(Z_TYPE_P(Z_INDIRECT_P(val)) == IS_UNDEF)) {
				num--;
			}
		}
	} ZEND_HASH_FOREACH_END();
	// ？ 从这里看，宏是先于c语言执行，ZEND_HASH_FOREACH_END() 调用后才拼接好逻辑
	return num;
}
/* }}} */

// ing3, 计算哈希表元素数
ZEND_API uint32_t zend_array_count(HashTable *ht)
{
	uint32_t num;
	// 如果可能包含空索引号
	if (UNEXPECTED(HT_FLAGS(ht) & HASH_FLAG_HAS_EMPTY_IND)) {
		// 获取有效元素数量
		num = zend_array_recalc_elements(ht);
		// 设置元素数量为有效元素数量
		if (UNEXPECTED(ht->nNumOfElements == num)) {
			// 删除标记
			HT_FLAGS(ht) &= ~HASH_FLAG_HAS_EMPTY_IND;
		}
	// 如果是 excute globals.symbol_table 
	} else if (UNEXPECTED(ht == &EG(symbol_table))) {
		// 获取有效元素数量
		num = zend_array_recalc_elements(ht);

	// 其他情况直接取计算好的数量
	} else {
		num = zend_hash_num_elements(ht);
	}
	return num;
}
/* }}} */

// ing3, 获得哈希表 pos右面第一个有效位置
static zend_always_inline HashPosition _zend_hash_get_valid_pos(const HashTable *ht, HashPosition pos)
{
	// 如果是顺序数组
	if (HT_IS_PACKED(ht)) {
		// 如果在有效范围内，发现无效元素，向后移动一个位置
		while (pos < ht->nNumUsed && Z_ISUNDEF(ht->arPacked[pos])) {
			pos++;
		}
	// 混合数组
	} else {
		// 如果在有效范围内，发现无效元素，向后移动一个位置
		while (pos < ht->nNumUsed && Z_ISUNDEF(ht->arData[pos].val)) {
			pos++;
		}
	}
	return pos;
}

// ing3, 取得哈希表的当前位置（右面第一个有效位置）
static zend_always_inline HashPosition _zend_hash_get_current_pos(const HashTable *ht)
{
	return _zend_hash_get_valid_pos(ht, ht->nInternalPointer);
}

// ing3, 取得哈希表的当前位置（右面第一个有效位置）
ZEND_API HashPosition ZEND_FASTCALL zend_hash_get_current_pos(const HashTable *ht)
{
	return _zend_hash_get_current_pos(ht);
}

// ing3, 如果没有闲置，分配8个。在新迭代器上记录 哈希表和pos，更新使用数量。p1:哈希表，p2:记录位置
// 一开始用的应该是 EG(ht_iterators_slots)
ZEND_API uint32_t ZEND_FASTCALL zend_hash_iterator_add(HashTable *ht, HashPosition pos)
{
	// 执行时 迭代器列表 指针。迭代器是运行时创建的。
	HashTableIterator *iter = EG(ht_iterators);
	// 迭代器末尾
	HashTableIterator *end  = iter + EG(ht_iterators_count);
	// 
	uint32_t idx;

	// 如果没溢出
	if (EXPECTED(!HT_ITERATORS_OVERFLOW(ht))) {
		// 迭代器count +1
		HT_INC_ITERATORS_COUNT(ht);
	}
	// 遍历所有迭代器
	while (iter != end) {
		// 如果是闲置的（没有关联哈希表）
		if (iter->ht == NULL) {
			// 关联到当前哈希表，设置位置
			iter->ht = ht;
			// 迭代器记录当前位置
			iter->pos = pos;
			// 迭代器 顺序号
			idx = iter - EG(ht_iterators);
			// 如果大于使用过的最大编号
			if (idx + 1 > EG(ht_iterators_used)) {
				// 更新使用过的最大编号 
				EG(ht_iterators_used) = idx + 1;
			}
			// 返回迭代器序号
			return idx;
		}
		// 下一个迭代器
		iter++;
	}
	
	// 如果没有闲置迭代器
	// 如果使用的是 EG(ht_iterators_slots) 迭代器列表（16个位置）
	if (EG(ht_iterators) == EG(ht_iterators_slots)) {
		// 重新创建列表，n+8个迭代器
		EG(ht_iterators) = emalloc(sizeof(HashTableIterator) * (EG(ht_iterators_count) + 8));
		// 把 原有列表复制过来
		memcpy(EG(ht_iterators), EG(ht_iterators_slots), sizeof(HashTableIterator) * EG(ht_iterators_count));
	// 
	} else {
		// 调整原列表内存大小，增加8个迭代器
		EG(ht_iterators) = erealloc(EG(ht_iterators), sizeof(HashTableIterator) * (EG(ht_iterators_count) + 8));
	}
	// 当前使用的迭代器
	iter = EG(ht_iterators) + EG(ht_iterators_count);
	// 添加8个迭代器
	EG(ht_iterators_count) += 8;
	// 关联哈希表
	iter->ht = ht;
	// 记录位置
	iter->pos = pos;
	// 后面的7个迭代器都是空
	memset(iter + 1, 0, sizeof(HashTableIterator) * 7);
	// 计算顺序号
	idx = iter - EG(ht_iterators);
	// 使用迭代器数量 +1
	EG(ht_iterators_used) = idx + 1;
	// 返回顺序号
	return idx;
}

// ing3, 获取指定 序号 迭代器的指向位置，如果迭代器和哈希表不匹配，更新迭代器
ZEND_API HashPosition ZEND_FASTCALL zend_hash_iterator_pos(uint32_t idx, HashTable *ht)
{
	HashTableIterator *iter = EG(ht_iterators) + idx;
	// 序号有效
	ZEND_ASSERT(idx != (uint32_t)-1);
	// 如果迭代器和哈希表不匹配
	if (UNEXPECTED(iter->ht != ht)) {
		// 如果迭代器有关联哈希表 并且关联的哈希表没有被删除 并且原哈希表迭代器数量没有满
		if (EXPECTED(iter->ht) && EXPECTED(iter->ht != HT_POISONED_PTR)
				&& EXPECTED(!HT_ITERATORS_OVERFLOW(iter->ht))) {
			// 原哈希表迭代器数量 -1
			HT_DEC_ITERATORS_COUNT(iter->ht);
		}
		// 如果哈希表迭代其数量没有满
		if (EXPECTED(!HT_ITERATORS_OVERFLOW(ht))) {
			// 迭代器数量 +1
			HT_INC_ITERATORS_COUNT(ht);
		}
		// 迭代器关联到哈希表，位置取哈希表当前位置
		iter->ht = ht;
		iter->pos = _zend_hash_get_current_pos(ht);
	}
	return iter->pos;
}

// ing3, 哈希表的迭代器位置。如果迭代器指向的数组不是当前数组，更新迭代器
ZEND_API HashPosition ZEND_FASTCALL zend_hash_iterator_pos_ex(uint32_t idx, zval *array)
{
	// 泛类型变量中获取数组
	HashTable *ht = Z_ARRVAL_P(array);
	// EG里第 idx 个迭代器
	HashTableIterator *iter = EG(ht_iterators) + idx;

	// 迭代器序号必须有效
	ZEND_ASSERT(idx != (uint32_t)-1);
	
	// 如果迭代器指向的数组不是当前数组
	if (UNEXPECTED(iter->ht != ht)) {
		// 如果迭代器有指定数组 并且 指定的数组没有被删除
		if (EXPECTED(iter->ht) && EXPECTED(iter->ht != HT_POISONED_PTR)
				// 并且 当前数组没有溢出
				&& EXPECTED(!HT_ITERATORS_OVERFLOW(ht))) {
			// 迭代器指向数组的 迭代器数量 -1
			HT_DEC_ITERATORS_COUNT(iter->ht);
		}
		// 分割数组
		SEPARATE_ARRAY(array);
		// 哈希表
		ht = Z_ARRVAL_P(array);
		// 如果迭代器数量没有满
		if (EXPECTED(!HT_ITERATORS_OVERFLOW(ht))) {
			// 迭代器数量+1
			HT_INC_ITERATORS_COUNT(ht);
		}
		// 迭代器指向当前数组
		iter->ht = ht;
		// 迭代器位置
		iter->pos = _zend_hash_get_current_pos(ht);
	}
	// 
	return iter->pos;
}

// ing3, 删除 指定序号的 哈希表迭代器
ZEND_API void ZEND_FASTCALL zend_hash_iterator_del(uint32_t idx)
{
	// 获取指定迭代器
	HashTableIterator *iter = EG(ht_iterators) + idx;
	// 序号有效
	ZEND_ASSERT(idx != (uint32_t)-1);
	// 如果迭代器有关联哈希表 ，指向的哈希表不可以正在删除， 哈希表迭代器数量不可以溢出
	if (EXPECTED(iter->ht) && EXPECTED(iter->ht != HT_POISONED_PTR)
			&& EXPECTED(!HT_ITERATORS_OVERFLOW(iter->ht))) {
		// 哈希表迭代器数量不可以是0
		ZEND_ASSERT(HT_ITERATORS_COUNT(iter->ht) != 0);
		// 减少哈希表迭代器数量
		HT_DEC_ITERATORS_COUNT(iter->ht);
	}
	// 解除关联
	iter->ht = NULL;

	// 如果是最后一个迭代器
	if (idx == EG(ht_iterators_used) - 1) {
		// 忽略末尾的空迭代器
		while (idx > 0 && EG(ht_iterators)[idx - 1].ht == NULL) {
			idx--;
		}
		// 更新迭代器数量
		EG(ht_iterators_used) = idx;
	}
}

// ing4, 删除迭代器（只是解除关联，没有销毁迭代器）
static zend_never_inline void ZEND_FASTCALL _zend_hash_iterators_remove(HashTable *ht)
{
	// 遍历所有迭代器
	HashTableIterator *iter = EG(ht_iterators);
	HashTableIterator *end  = iter + EG(ht_iterators_used);

	while (iter != end) {
		// 让所有指向本哈希表的迭代器指向 HT_POISONED_PTR
		if (iter->ht == ht) {
			iter->ht = HT_POISONED_PTR;
		}
		iter++;
	}
}

// ing3, 删除迭代器
static zend_always_inline void zend_hash_iterators_remove(HashTable *ht)
{
	// 有迭代器存在
	if (UNEXPECTED(HT_HAS_ITERATORS(ht))) {
		_zend_hash_iterators_remove(ht);
	}
}

// ing4, 查找start以后，所有迭代器中，最小的迭代位置， 返回int
ZEND_API HashPosition ZEND_FASTCALL zend_hash_iterators_lower_pos(HashTable *ht, HashPosition start)
{
	// 遍历全部迭代器
	HashTableIterator *iter = EG(ht_iterators);
	HashTableIterator *end  = iter + EG(ht_iterators_used);
	// 默认返回哈希表的 nNumUsed 
	HashPosition res = ht->nNumUsed;

	// 
	while (iter != end) {
		// 此哈希表的迭代器
		if (iter->ht == ht) {
			// 迭代指针在有效范围内
			if (iter->pos >= start && iter->pos < res) {
				// 使用更小的迭代指针
				res = iter->pos;
			}
		}
		// 下一个迭代器
		iter++;
	}
	// 
	return res;
}

// ing4, 更新哈希表的迭代器位置
ZEND_API void ZEND_FASTCALL _zend_hash_iterators_update(HashTable *ht, HashPosition from, HashPosition to)
{
	// 遍历全部迭代器
	HashTableIterator *iter = EG(ht_iterators);
	HashTableIterator *end  = iter + EG(ht_iterators_used);
	//
	while (iter != end) {
		// 如果是本哈希表的迭代器，并且 位置正在 from
		if (iter->ht == ht && iter->pos == from) {
			// 把位置更新到 to
			iter->pos = to;
		}
		iter++;
	}
}

// ing4, 所有属于当前哈希表的迭代器，位置右移 +step
ZEND_API void ZEND_FASTCALL zend_hash_iterators_advance(HashTable *ht, HashPosition step)
{
	// 遍历所有迭代器
	HashTableIterator *iter = EG(ht_iterators);
	HashTableIterator *end  = iter + EG(ht_iterators_used);

	while (iter != end) {
		// 属于当前哈希表的迭代器
		if (iter->ht == ht) {
			// 位置右移
			iter->pos += step;
		}
		iter++;
	}
}

// key的哈希值必须已经预先计算好！
/* Hash must be known and precomputed before */
// ing3, 用 zend_string 型的键名查找哈希表里的键值对
static zend_always_inline Bucket *zend_hash_find_bucket(const HashTable *ht, const zend_string *key)
{
	uint32_t nIndex;
	uint32_t idx;
	Bucket *p, *arData;

	// 	#define ZSTR_H(zstr) (zstr)->h
	// key 的哈希值， 哈希必须是已知的。所以这个查找里没用到哈希算法。
	ZEND_ASSERT(ZSTR_H(key) != 0 && "Hash must be known");

	// 数据
	arData = ht->arData;
	// 利用哈希值 和 mask 计算索引号位置（倒序，例如共16个位置，第5个位置算出 -11）
	nIndex = ZSTR_H(key) | ht->nTableMask;
	
	// 找到哈希->索引号列表 中的指定索引号。p1:列表结尾位置，p2:顺序号（和 tableMask 做过或运算的哈希值, 负数）
	idx = HT_HASH_EX(arData, nIndex);

	// 如果 idx 无效 #define HT_INVALID_IDX ((uint32_t) -1)
	if (UNEXPECTED(idx == HT_INVALID_IDX)) {
		return NULL;
	}
	// 获取索引位置的bucket
	p = HT_HASH_TO_BUCKET_EX(arData, idx);
	// 成功条件1：如果 p->key 和 key 指向同一个变量，直接返回。检查相同的内置字串。两个都是zend_string 指针。如何比较的？
	if (EXPECTED(p->key == key)) { /* check for the same interned string */
		return p;
	}

	// 如果Key没匹配，一直循环，循环哈希
	while (1) {
		// 成功条件2：如果P的哈希和 key的哈希匹配，并且，p->key 存在 并且 p->key 和 key 相同（大小写敏感）
		if (p->h == ZSTR_H(key) &&
		    EXPECTED(p->key) &&
		    zend_string_equal_content(p->key, key)) {
			// 匹配成功
			return p;
		}
		// 两个条件都不匹配，找同样哈希值的下一个位置
		idx = Z_NEXT(p->val);
		// 如果索引无效，返回 Null
		if (idx == HT_INVALID_IDX) {
			return NULL;
		}
		// # define HT_HASH_TO_BUCKET_EX(data, idx)  ((data) + (idx))
		// 获取索引位置的bucket
		p = HT_HASH_TO_BUCKET_EX(arData, idx);
		// 成功条件1：如果 p和key 指向同一个变量，直接返回
		if (p->key == key) { /* check for the same interned string */
			return p;
		}
	}
}

// ing3, 用char和长度做键名，查询哈希表，内部引用
static zend_always_inline Bucket *zend_hash_str_find_bucket(const HashTable *ht, const char *str, size_t len, zend_ulong h)
{
	uint32_t nIndex;
	uint32_t idx;
	Bucket *p, *arData;

	arData = ht->arData;
	// 利用哈希值 和 mask 计算索引号位置（倒序，例如共16个位置，第5个位置算出 -11）
	nIndex = h | ht->nTableMask;
	// 找到哈希->索引号列表 中的指定索引号。p1:列表结尾位置，p2:顺序号（和 tableMask 做过或运算的哈希值, 负数）
	idx = HT_HASH_EX(arData, nIndex);
	// 索引有效
	while (idx != HT_INVALID_IDX) {
		// 索引必须小于哈希表当前大小
		// 64位系统中 index 不用转换
		ZEND_ASSERT(idx < HT_IDX_TO_HASH(ht->nTableSize));
		// 取回键值对
		p = HT_HASH_TO_BUCKET_EX(arData, idx);
		// 哈希值相同， 有key ，key里面的string匹配
		if ((p->h == h)
			 && p->key
			 && zend_string_equals_cstr(p->key, str, len)) {
			return p;
		}
		// 查询同样哈希的下一个
		idx = Z_NEXT(p->val);
	}
	return NULL;
}

// ing3, 使用 哈希值(zend_ulong) 获取哈希表里【没有key，只有哈希值的键值对】
static zend_always_inline Bucket *zend_hash_index_find_bucket(const HashTable *ht, zend_ulong h)
{
	uint32_t nIndex;
	uint32_t idx;
	Bucket *p, *arData;

	arData = ht->arData;
	// 利用哈希值 和 mask 计算索引号位置（倒序，例如共16个位置，第5个位置算出 -11）
	nIndex = h | ht->nTableMask;
	
	// 找到哈希->索引号列表 中的指定索引号。p1:列表结尾位置，p2:顺序号（和 tableMask 做过或运算的哈希值, 负数）
	idx = HT_HASH_EX(arData, nIndex);
	// 如果有效，idx!=-1 , #define HT_INVALID_IDX ((uint32_t) -1)
	while (idx != HT_INVALID_IDX) {
		// 不可以取超了
		// 64位系统中 index 不用转换
		ZEND_ASSERT(idx < HT_IDX_TO_HASH(ht->nTableSize));
		// # define HT_HASH_TO_BUCKET_EX(data, idx) ((data) + (idx))
		// 取得第 nIndex 个键值对的指针 （无校验）
		p = HT_HASH_TO_BUCKET_EX(arData, idx);
		// 条件：如果哈希值匹配，【并且没有Key】, 正常返回
		if (p->h == h && !p->key) {
			return p;
		}
		// 找同样哈希值的下一个位置
		idx = Z_NEXT(p->val);
	}
	// 找不到返回null
	return NULL;
}

/* 组合操作类型如下


// 插入元素，有旧原素中断并返回null
_zend_hash_add_or_update_i(ht, key, pData, HASH_ADD);

// 不管找不到到原素，都更新指定位置的元素
_zend_hash_add_or_update_i(ht, key, pData, HASH_UPDATE);

// 不管找不到到原素，都更新指定位置的元素。如果原素是间接引用，解引用。
_zend_hash_add_or_update_i(ht, key, pData, HASH_UPDATE | HASH_UPDATE_INDIRECT);
// 不查找，直写入指定位置元素
_zend_hash_add_or_update_i(ht, key, pData, HASH_ADD_NEW);
// 找到位置，有旧原素返回，没有旧元素更新成null，返回
_zend_hash_add_or_update_i(ht, key, NULL, HASH_LOOKUP); 

// 插入元素，有旧原素 检查间接引用目标，如果有效，中断并返回null
_zend_hash_add_or_update_i(target, p->key, s, HASH_ADD | HASH_UPDATE_INDIRECT);

重复的
_zend_hash_add_or_update_i(target, p->key, s, HASH_UPDATE | HASH_UPDATE_INDIRECT);

HASH_UPDATE_INDIRECT 是附加标记只和 HASH_UPDATE，HASH_ADD 公用
HASH_ADD 可选加 HASH_UPDATE_INDIRECT
HASH_UPDATE 可选加 HASH_UPDATE_INDIRECT
HASH_ADD_NEW 单独使用
HASH_LOOKUP 单独使用且无 pData

*/
// ing4，（强转哈希表）增/改/查 元素, p1:哈希表，p2:key，p3:值，p4:flag（操作类型）
// 内部调用
static zend_always_inline zval *_zend_hash_add_or_update_i(HashTable *ht, zend_string *key, zval *pData, uint32_t flag)
{
	// 哈希值
	zend_ulong h;
	uint32_t nIndex;
	uint32_t idx;
	Bucket *p, *arData;

	// 检查一致性，调试用
	IS_CONSISTENT(ht);
	// 断言，哈希表的引用次数为1 或 ALLOW_COW_VIOLATION ，调试用
	HT_ASSERT_RC1(ht);
	// 给key添加哈希值
	zend_string_hash_val(key);

	// 情况1：
	// 如果哈希表未初始化，或者是 顺序数组
	if (UNEXPECTED(HT_FLAGS(ht) & (HASH_FLAG_UNINITIALIZED|HASH_FLAG_PACKED))) {
		// 情况1.1：
		// 如果哈希表 未初始化
		if (EXPECTED(HT_FLAGS(ht) & HASH_FLAG_UNINITIALIZED)) {
			// 初始化混合哈希表
			zend_hash_real_init_mixed(ht);
			// 向哈希表中添加
			goto add_to_hash;
		// 情况1.2：
		// 初始化过的顺序数组
		} else {
			// 顺序数组转哈希表
			zend_hash_packed_to_hash(ht);
		}
	// 情况2：
	// 已初始化过的【哈希表】
	// 标记不是新建，或者当前处于调试模式
	} else if ((flag & HASH_ADD_NEW) == 0 || ZEND_DEBUG) {
		// 获取已有键值对
		p = zend_hash_find_bucket(ht, key);
		// 情况2.1：
		// 如果有旧原素		
		if (p) {
			zval *data;
			// 不可以是新建
			ZEND_ASSERT((flag & HASH_ADD_NEW) == 0);
			// 情况2.1.1：
			// 如果是查找，直接返回
			if (flag & HASH_LOOKUP) {
				return &p->val;
				
			// 情况2.1.2：
			// 如果是添加
			} else if (flag & HASH_ADD) {
				// 情况2.1.2.1：
				// ? 如果不是更新内部元素
				// HASH_UPDATE_INDIRECT 是附加标记只和 HASH_UPDATE，HASH_ADD 公用
				if (!(flag & HASH_UPDATE_INDIRECT)) {
					// 返回 null
					return NULL;
				}
				// 情况2.1.2.2：
					// 有 HASH_UPDATE_INDIRECT 和 HASH_ADD
					
				// 跟新值不可以和旧值相同
				ZEND_ASSERT(&p->val != pData);
				data = &p->val;
				// 情况2.1.2.2.1：
				// 如果新值是间接引用
				if (Z_TYPE_P(data) == IS_INDIRECT) {
					// 追踪引用目标
					data = Z_INDIRECT_P(data);
					// 如果类型不是未定义，返回null 
					if (Z_TYPE_P(data) != IS_UNDEF) {
						// 返回 null
						return NULL;
					}
				// 情况2.1.2.2.2：
				// 不是间接引用
				} else {
					// 返回 null
					return NULL;
				}
			// 情况2.1.3：
			// 其他情况
			} else {
				// 
				ZEND_ASSERT(&p->val != pData);
				// 取出键值对的值
				data = &p->val;
				// 情况2.1.3.1：
				// HASH_UPDATE_INDIRECT 是附加标记只和 HASH_UPDATE，HASH_ADD 公用
				if ((flag & HASH_UPDATE_INDIRECT) && Z_TYPE_P(data) == IS_INDIRECT) {
					// 追踪引用目标
					data = Z_INDIRECT_P(data);
				}
				// 情况2.1.3.2：
					// 没有 HASH_UPDATE_INDIRECT 不处理间接引用
			}
			// 如果有销毁器
			if (ht->pDestructor) {
				// 销毁 data
				ht->pDestructor(data);
			}
			// 把数据 复制到副本中
			ZVAL_COPY_VALUE(data, pData);
			// 所有有旧原素的分支都返回了
			// 返回副本
			return data;
		}
		// 情况2.2：
		// 没找到已有原素 
	}
	// 情况3：
		// 已初始化过的【哈希表】，HASH_ADD_NEW


	// 可能到这里的情况：
		// 情况1：如果哈希表未初始化，或者是 顺序数组
		// 情况2.2：已初始化过的【哈希表】，不是HASH_ADD_NEW，没找到已有原素
		// 情况3：已初始化过的【哈希表】，HASH_ADD_NEW
			// 这种情况key重复怎么办？
	
	// 如果哈希表满了，重新设置大小
	ZEND_HASH_IF_FULL_DO_RESIZE(ht);		/* If the Hash table is full, resize it */

// 跳转点：添加元素
add_to_hash:
	// 如果是内部字串
	if (!ZSTR_IS_INTERNED(key)) {
		// 增加引用次数
		zend_string_addref(key);
		// 给哈希表添加标记： 静态key
		HT_FLAGS(ht) &= ~HASH_FLAG_STATIC_KEYS;
	}
	// 使用数+1
	idx = ht->nNumUsed++;
	// 元素数+1
	ht->nNumOfElements++;
	// 数据表开头
	arData = ht->arData;
	// 第 idx 个元素的指针
	p = arData + idx;
	// 添加key
	p->key = key;
	// 复制key的哈希值到键值对
	p->h = h = ZSTR_H(key);
	// 利用哈希值 和 mask 计算索引号位置（倒序，例如共16个位置，第5个位置算出 -11）
	nIndex = h | ht->nTableMask;
	// 设置下一个元素的指针
	// 找到哈希->索引号列表 中的指定索引号。p1:列表结尾位置，p2:顺序号（和 tableMask 做过或运算的哈希值, 负数）
	Z_NEXT(p->val) = HT_HASH_EX(arData, nIndex);
	
	// 哈希位置写入当前元素序号
	// 找到哈希->索引号列表 中的指定索引号。p1:列表结尾位置，p2:顺序号（和 tableMask 做过或运算的哈希值, 负数）
	// 64位系统中 index 不用转换
	HT_HASH_EX(arData, nIndex) = HT_IDX_TO_HASH(idx);
	// 如果要查询
	if (flag & HASH_LOOKUP) {
		// 新哈希对的值写成null
		ZVAL_NULL(&p->val);
	// 否则
	} else {
		// 把 pData 复制给 元素值
		ZVAL_COPY_VALUE(&p->val, pData);
	}

	// 返回新键值对的值
	return &p->val;
}

/*

有旧原素时替换旧原素，没有旧原素写入新元素
_zend_hash_str_add_or_update_i(ht, str, len, h, pData, HASH_UPDATE);

有旧原素时，追踪引用目标，替换旧原素。没有旧原素写入新元素
_zend_hash_str_add_or_update_i(ht, str, len, h, pData, HASH_UPDATE | HASH_UPDATE_INDIRECT);

碰到已有原素返回null。否则添加。
_zend_hash_str_add_or_update_i(ht, str, len, h, pData, HASH_ADD);
不检查已有，直接找到位置，写入原素。
_zend_hash_str_add_or_update_i(ht, str, len, h, pData, HASH_ADD_NEW);

可能的情况比较简单:
	HASH_UPDATE 可选 + HASH_UPDATE_INDIRECT
	HASH_ADD，HASH_ADD_NEW 单独使用
	没有带 HASH_LOOKUP 的引用
*/
// ing4, （强转哈希表）增/改 元素 。p1:哈希表，p2:key, p3:key长度，p4:哈希值，p5:新值，p6:操作类型
// 有查询功能但无调用
static zend_always_inline zval *_zend_hash_str_add_or_update_i(HashTable *ht, const char *str, size_t len, zend_ulong h, zval *pData, uint32_t flag)
{
	// 哈希键
	zend_string *key;
	// 数字索引号
	uint32_t nIndex;
	// 哈希值
	uint32_t idx;
	// 键值对
	Bucket *p;
	// 检查一致性，调试用
	IS_CONSISTENT(ht);
	// 断言，哈希表的引用次数为1 或 ALLOW_COW_VIOLATION ，调试用
	HT_ASSERT_RC1(ht);

	// 情况1：
	// 未初始化过 或者是 顺序数组。
	if (UNEXPECTED(HT_FLAGS(ht) & (HASH_FLAG_UNINITIALIZED|HASH_FLAG_PACKED))) {
		// 情况1.1：
		// 如果没有初始化
		if (EXPECTED(HT_FLAGS(ht) & HASH_FLAG_UNINITIALIZED)) {
			// 初始化成哈希表。
			zend_hash_real_init_mixed(ht);
			// 添加元素
			goto add_to_hash;
		// 情况1.2：
		// 如果未初始化，只能是顺序数组
		} else {
			// 顺序数组转哈希表	
			zend_hash_packed_to_hash(ht);
		}
		
	// 情况2：
	// HASH_ADD_NEW 和 HASH_ADD 的区别是 HASH_ADD 不走这里
	// 如果不是添加新对象。
	} else if ((flag & HASH_ADD_NEW) == 0) {
		// 查询哈希表（本函数无查询功能）
		p = zend_hash_str_find_bucket(ht, str, len, h);
		// 情况2.1：
		// 如果找到key，才执行操作。HASH_ADD_NEW 需要对已有元素做处理
		if (p) {
			zval *data;
			// 情况2.1.1：
			// 如果是查询，直接返回
			if (flag & HASH_LOOKUP) {
				return &p->val;
				
			// 情况2.1.2：
			// 如果 ！HASH_LOOKUP 并且 HASH_ADD
			} else if (flag & HASH_ADD) {
				// 如果不是更新间接引用，返回
				// 情况2.1.2.1：
				// 没有 HASH_UPDATE_INDIRECT
				if (!(flag & HASH_UPDATE_INDIRECT)) {
					return NULL;
				}
				
				// 情况2.1.2.2：
					// 此方法 HASH_UPDATE_INDIRECT 和 HASH_ADD 不会同时使用
					// 应该到不是了这个地方（全局搜索过）
					
				// 新值和旧值不可以相同
				ZEND_ASSERT(&p->val != pData);
				// 
				data = &p->val;
				
				// 情况2.1.2.2.1：
				// 如果是间接引用
				if (Z_TYPE_P(data) == IS_INDIRECT) {
					// 找到引用对象
					data = Z_INDIRECT_P(data);
					// 情况2.1.2.2.1.1：
					// 如果对象未定义，返回null
					if (Z_TYPE_P(data) != IS_UNDEF) {
						return NULL;
					}
					// 情况2.1.2.2.1.2：
					// 如果引用目标有效
						// 替换元素
						
				// 情况2.1.2.2.2：
				// 不是间接引用
				} else {
					return NULL;
				}
				
			// 情况2.1.3：
			// 不是 HASH_ADD
			} else {
				ZEND_ASSERT(&p->val != pData);
				data = &p->val;
				// 情况2.1.3.1：
				// 有 HASH_UPDATE_INDIRECT 并且是间接引用
				if ((flag & HASH_UPDATE_INDIRECT) && Z_TYPE_P(data) == IS_INDIRECT) {
					data = Z_INDIRECT_P(data);
				}
			}
			
			// 销毁器
			if (ht->pDestructor) {
				// 销毁旧
				ht->pDestructor(data);
			}
			ZVAL_COPY_VALUE(data, pData);
			// 有旧原素，所有分支都在这里返回了
			return data;
		}
		// 情况2.2：
		// 没有旧原素
	}
	
	// 情况3：
		// 已初始化过的哈希表，有 HASH_ADD_NEW
			// 这种情况下没加验证，有可能导致一个哈希多个值吗？
	
	// 可能到这里的情况：
		// 情况1：未初始化过 或者是 顺序数组。
			// 情况1.1。 goto add_to_hash
		// 情况2.2：没有旧原素 
		// 情况3：已初始化过的哈希表，有 HASH_ADD_NEW
	
	// 如果满了，扩充大小
	ZEND_HASH_IF_FULL_DO_RESIZE(ht);		/* If the Hash table is full, resize it */

// 向哈希表里添加元素
add_to_hash:
	// 已使用数量 +1
	idx = ht->nNumUsed++;
	// 元素数量 +1
	ht->nNumOfElements++;
	// 新键值对
	p = ht->arData + idx;
	// 更新key 和 key的哈希。 key创建副本，带 持久标记
	p->key = key = zend_string_init(str, len, GC_FLAGS(ht) & IS_ARRAY_PERSISTENT);
	p->h = ZSTR_H(key) = h;
	// 哈希表删除 HASH_FLAG_STATIC_KEYS
	HT_FLAGS(ht) &= ~HASH_FLAG_STATIC_KEYS;
	// 如果要求查询
	if (flag & HASH_LOOKUP) {
		ZVAL_NULL(&p->val);
	// 复制
	} else {
		ZVAL_COPY_VALUE(&p->val, pData);
	}
	// 利用哈希值 和 mask 计算索引号位置（倒序，例如共16个位置，第5个位置算出 -11）
	nIndex = h | ht->nTableMask;
	// 找到 (p1->arHash) 哈希->索引列表 中的指定索引号。p1:哈希表，p2:顺序号（和mask做过或运算, 负数）
	Z_NEXT(p->val) = HT_HASH(ht, nIndex);
	// 64位系统中 index 不用转换
	// 找到 (p1->arHash) 哈希->索引列表 中的指定索引号。p1:哈希表，p2:顺序号（和mask做过或运算, 负数）
	HT_HASH(ht, nIndex) = HT_IDX_TO_HASH(idx);
	// 
	return &p->val;
}

// ing4, （强转哈希表）增/改 元素 , p1:哈希表，p2:key，p3:值，p4:flag（操作类型）
ZEND_API zval* ZEND_FASTCALL zend_hash_add_or_update(HashTable *ht, zend_string *key, zval *pData, uint32_t flag)
{
	//
	if (flag == HASH_ADD) {
		// 插入元素，有旧原素中断并返回null
		return zend_hash_add(ht, key, pData);
	//
	} else if (flag == HASH_ADD_NEW) {
		// 不查检，直写入指定位置元素
		return zend_hash_add_new(ht, key, pData);
	//
	} else if (flag == HASH_UPDATE) {
		// 不管找不到到原素，都更新指定位置的元素
		return zend_hash_update(ht, key, pData);
	// 
	} else {
		// 操作必须是 HASH_UPDATE|HASH_UPDATE_INDIRECT
		ZEND_ASSERT(flag == (HASH_UPDATE|HASH_UPDATE_INDIRECT));
		// 不管找不到到原素，都更新指定位置的元素。如果原素是间接引用，解引用。
		return zend_hash_update_ind(ht, key, pData);
	}
}

// ing4,（强转哈希表）插入元素，有旧原素中断并返回null。p1:哈希表，p2:key，p3:值
ZEND_API zval* ZEND_FASTCALL zend_hash_add(HashTable *ht, zend_string *key, zval *pData)
{
	//（强转哈希表）增/改/查 元素, p1:哈希表，p2:key，p3:值，p4:flag（操作类型）
	return _zend_hash_add_or_update_i(ht, key, pData, HASH_ADD);
}

// ing4,（强转哈希表）不管找不到到原素，都更新指定位置的元素。p1:哈希表，p2:key，p3:值
ZEND_API zval* ZEND_FASTCALL zend_hash_update(HashTable *ht, zend_string *key, zval *pData)
{
	//（强转哈希表）增/改/查 元素, p1:哈希表，p2:key，p3:值，p4:flag（操作类型）
	return _zend_hash_add_or_update_i(ht, key, pData, HASH_UPDATE);
}

// ing4,（强转哈希表）不管找不到到原素，都更新指定位置的元素。如果原素是间接引用，解引用。p1:哈希表，p2:key，p3:值
ZEND_API zval* ZEND_FASTCALL zend_hash_update_ind(HashTable *ht, zend_string *key, zval *pData)
{
	//（强转哈希表）增/改/查 元素, p1:哈希表，p2:key，p3:值，p4:flag（操作类型）
	return _zend_hash_add_or_update_i(ht, key, pData, HASH_UPDATE | HASH_UPDATE_INDIRECT);
}

// ing4,（强转哈希表）不查检，直写入指定位置元素。p1:哈希表，p2:key，p3:值
ZEND_API zval* ZEND_FASTCALL zend_hash_add_new(HashTable *ht, zend_string *key, zval *pData)
{
	//（强转哈希表）增/改/查 元素, p1:哈希表，p2:key，p3:值，p4:flag（操作类型）
	return _zend_hash_add_or_update_i(ht, key, pData, HASH_ADD_NEW);
}

// ing4,（强转哈希表）找到位置，有旧原素返回，没有旧元素更新成null，返回。p1:哈希表，p2:key，p3:值
ZEND_API zval* ZEND_FASTCALL zend_hash_lookup(HashTable *ht, zend_string *key)
{
	//（强转哈希表）增/改/查 元素, p1:哈希表，p2:key，p3:值，p4:flag（操作类型）
	return _zend_hash_add_or_update_i(ht, key, NULL, HASH_LOOKUP);
}

// ing4,（强转哈希表）增/改 元素。p1:哈希表，p2:key, p3:key长度, p4:新值，p5:操作
ZEND_API zval* ZEND_FASTCALL zend_hash_str_add_or_update(HashTable *ht, const char *str, size_t len, zval *pData, uint32_t flag)
{
	// 
	if (flag == HASH_ADD) {
		// （强转哈希表）添加，碰到已有原素返回null。否则添加。p1:哈希表，p2:key, p3:key长度, p4:新值
		return zend_hash_str_add(ht, str, len, pData);
	//
	} else if (flag == HASH_ADD_NEW) {
		// （强转哈希表）添加，不检查已有，直接找到位置，写入元素。p1:哈希表，p2:key, p3:key长度, p4:新值
		return zend_hash_str_add_new(ht, str, len, pData);
	//
	} else if (flag == HASH_UPDATE) {
		// （强转哈希表）更新，有旧原素时替换旧原素，否则写入新元素。p1:哈希表，p2:key, p3:key长度, p4:新值
		return zend_hash_str_update(ht, str, len, pData);
	//
	} else {
		// （强转哈希表）更新，有旧原素时，追踪引用目标，替换旧原素。没有旧原素写入新元素
		// p1:哈希表，p2:key, p3:key长度, p4:新值
		ZEND_ASSERT(flag == (HASH_UPDATE|HASH_UPDATE_INDIRECT));
		//（强转哈希表）更新，有旧原素时，追踪引用目标，替换旧原素。没有旧原素写入新元素
		// p1:哈希表，p2:key, p3:key长度, p4:新值
		return zend_hash_str_update_ind(ht, str, len, pData);
	}
}

// ing4, （强转哈希表）更新，有旧原素时替换旧原素，否则写入新元素。p1:哈希表，p2:key, p3:key长度, p4:新值
ZEND_API zval* ZEND_FASTCALL zend_hash_str_update(HashTable *ht, const char *str, size_t len, zval *pData)
{
	// 计算作为数组key的哈希值
	zend_ulong h = zend_hash_func(str, len);

	// （强转哈希表）增/改 元素 。p1:哈希表，p2:key, p3:key长度，p4:哈希值，p5:新值，p6:操作类型
	return _zend_hash_str_add_or_update_i(ht, str, len, h, pData, HASH_UPDATE);
}

// ing4, （强转哈希表）更新，有旧原素时，追踪引用目标，替换旧原素。没有旧原素写入新元素
// p1:哈希表，p2:key, p3:key长度, p4:新值
ZEND_API zval* ZEND_FASTCALL zend_hash_str_update_ind(HashTable *ht, const char *str, size_t len, zval *pData)
{
	// 计算作为数组key的哈希值
	zend_ulong h = zend_hash_func(str, len);

	// （强转哈希表）增/改 元素 。p1:哈希表，p2:key, p3:key长度，p4:哈希值，p5:新值，p6:操作类型
	return _zend_hash_str_add_or_update_i(ht, str, len, h, pData, HASH_UPDATE | HASH_UPDATE_INDIRECT);
}

// ing4, （强转哈希表）添加，碰到已有原素返回null。否则添加。p1:哈希表，p2:key, p3:key长度, p4:新值
ZEND_API zval* ZEND_FASTCALL zend_hash_str_add(HashTable *ht, const char *str, size_t len, zval *pData)
{
	// 计算作为数组key的哈希值
	zend_ulong h = zend_hash_func(str, len);

	// （强转哈希表）增/改 元素 。p1:哈希表，p2:key, p3:key长度，p4:哈希值，p5:新值，p6:操作类型
	return _zend_hash_str_add_or_update_i(ht, str, len, h, pData, HASH_ADD);
}

// ing4, （强转哈希表）添加，不检查已有，直接找到位置，写入元素。p1:哈希表，p2:key, p3:key长度, p4:新值
ZEND_API zval* ZEND_FASTCALL zend_hash_str_add_new(HashTable *ht, const char *str, size_t len, zval *pData)
{
	// 计算作为数组key的哈希值
	zend_ulong h = zend_hash_func(str, len);

	// （强转哈希表）增/改 元素 。p1:哈希表，p2:key, p3:key长度，p4:哈希值，p5:新值，p6:操作类型
	return _zend_hash_str_add_or_update_i(ht, str, len, h, pData, HASH_ADD_NEW);
}

// ing4, 数组中添加值为 null的元素。p1:哈希表，p2:哈希值
ZEND_API zval* ZEND_FASTCALL zend_hash_index_add_empty_element(HashTable *ht, zend_ulong h)
{
	zval dummy;

	ZVAL_NULL(&dummy);
	// 顺序数组中找到旧原素直接中断，在哈希表中找到旧原素会替换掉。p1:哈希表，p2:哈希值，p3:元素
	return zend_hash_index_add(ht, h, &dummy);
}

// ing4, 数组中添加值为 null的元素。p1:哈希表，p2:key（zend_string）
ZEND_API zval* ZEND_FASTCALL zend_hash_add_empty_element(HashTable *ht, zend_string *key)
{
	zval dummy;

	ZVAL_NULL(&dummy);
	// （强转哈希表）插入元素，有旧原素中断并返回null。p1:哈希表，p2:key，p3:值
	return zend_hash_add(ht, key, &dummy);
}

// ing4, 数组中添加值为 null的元素。p1:哈希表，p2:key, p3:key长度
ZEND_API zval* ZEND_FASTCALL zend_hash_str_add_empty_element(HashTable *ht, const char *str, size_t len)
{
	zval dummy;

	ZVAL_NULL(&dummy);
	// （强转哈希表）添加，碰到已有原素返回null。否则添加。p1:哈希表，p2:key, p3:key长度, p4:新值
	return zend_hash_str_add(ht, str, len, &dummy);
}

/*
具体逻辑流程见附表 zend_hash.xlsx 
 
HASH_LOOKUP 只能单独使用。用哈希值，查询。
HASH_UPDATE 只能单独使用。更新，必须传入哈希值。在业务逻辑里没加判断。
HASH_ADD_NEXT 辅助标记，不单独使用。是指不传入哈希值，在哈希表下一个位置上操作，前面一定有 HASH_ADD。

HASH_ADD 顺序数组中，碰到已有元素会中断操作并返回null，
HASH_ADD_NEW 辅助标记，不单独使用。只能添加新元素，不能更新旧元素。这是最麻烦的一个标记

HASH_ADD 和 HASH_ADD | HASH_ADD_NEW 和 HASH_UPDATE 和 HASH_ADD | HASH_ADD_NEXT	这 4 个操作的差异很小
	HASH_ADD：顺序数组中找到旧原素直接中断，在哈希表中找到旧原素会替换掉
	HASH_ADD | HASH_ADD_NEW：顺序数组，碰到旧的新增。哈希表不查找旧原素，直接添加新元素。
	HASH_UPDATE：找到旧原素都更新。找不到旧原素都添加，数组会转成哈希表。
	HASH_ADD | HASH_ADD_NEXT: 比 HASH_ADD 只多一个，哈希值预处理
	HASH_ADD | HASH_ADD_NEW | HASH_ADD_NEXT : 在顺序数组和哈希表中都直接添加，不查旧原素
	HASH_LOOKUP：顺序数组和哈希表，找到位置有元素就返回，无元素初始化成null
	

其实比较麻烦的是 HASH_UPDATE 因为这个标记不在代码里出现 ，要自己分析它的流程。
但这几个方法从外面看还都 算比较直观。
	思路可以是 先看 HASH_ADD_NEXT，因为相关逻辑少，再看 HASH_ADD_NEW ，会不会更简单一点？
	但细节上不做成表格还是很难比较的
*/

// 170 行
// ing4, 通过哈希值(zend_ulong)，增/改/查 指定元素。p1:哈希表，p2:哈希值，p3:新元素，p4:操作类型
// 代码量并不大但同时考虑6种操作，情况很是复杂（参见附表）。有一点比较特殊，查找操作 也会初化 空目标元素。
static zend_always_inline zval *_zend_hash_index_add_or_update_i(HashTable *ht, zend_ulong h, zval *pData, uint32_t flag)
{
	// 序号
	uint32_t nIndex;
	// 索引
	uint32_t idx;
	// 键值对
	Bucket *p;
	// 键值对 -> 值
	zval *zv;

	// 检查一致性，调试用
	IS_CONSISTENT(ht);
	// 断言，哈希表的引用次数为1 或 ALLOW_COW_VIOLATION ，调试用
	HT_ASSERT_RC1(ht);

	// 这个判断是针对未初始化数组的
	// 如果在下一个位置添加，并且 位置是 最小整数
	if ((flag & HASH_ADD_NEXT) && h == ZEND_LONG_MIN) {
		// 位置是 0
		h = 0;
	}

	// 看似复杂，重点是情况 1 和情况 3，哈希表和顺序数组分开处理，互相独立，只有replace一个跳转点交叉。
	
	// ing3, 情况1：如果是顺序数组
	if (HT_IS_PACKED(ht)) {
		// ing4, 情况1.1 ：
		// 如果 不是在新位置添加 HASH_ADD_NEW|HASH_ADD_NEXT ( 必定也有 HASH_ADD ）
			// 所以6个子方法里只有 zend_hash_next_index_insert_new 一个一定不走这里。
		// 并且指向使用过的位置
		if ((flag & (HASH_ADD_NEW|HASH_ADD_NEXT)) != (HASH_ADD_NEW|HASH_ADD_NEXT)
		 && h < ht->nNumUsed) {
			// 顺序数组zval列表中，找到指定元素
			zv = ht->arPacked + h;
			// ing4, 情况1.1.1 ：
			// 如果位置有有效元素（不在新位置添加）
			if (Z_TYPE_P(zv) != IS_UNDEF) {
				// ing4, 情况1.1.1.1:
				// 如果是查找操作（说明没有 HASH_ADD_NEW 和 HASH_ADD_NEXT）
				if (flag & HASH_LOOKUP) {
					// 返回元素
					return zv;
				}
				
// 覆盖操作跳转点，这里只更新zval，一次引用（哈希表需要更新元素时）
replace:
				// ing4, 情况1.1.1.2:
				// 如果是添加操作
				if (flag & HASH_ADD) {
					// 顺序数组不允许在同一个位置添加两个元素
					// 返回null（添加失败）
					return NULL;
				}
				
				// ing4, 情况1.1.1.3:
				// HASH_UPDATE (不是 HASH_LOOKUP 和 HASH_ADD，只能是 HASH_UPDATE ）
				// 有销毁器
				if (ht->pDestructor) {
					// 销毁此元素上的变量
					ht->pDestructor(zv);
				}
				// 把新元素添加到指定位置
				ZVAL_COPY_VALUE(zv, pData);
				// 返回新元素
				return zv;
				
			// ing4, 情况1.1.2 ：
			// 如果不是按顺序使用的。必须要保持这个顺序。
				// （确定不是顺序数组了，即使是查询也要先转换成哈希表。）
			} else { /* we have to keep the order :( */
				// 转成哈希表，为了保持顺序
				goto convert_to_hash;
			}
			
			
		// ing3, 情况1.2 ：
		// 顺序数组，HASH_ADD_NEW|HASH_ADD_NEXT ( 必定也有 HASH_ADD ）或 访问未使用过的序号
		// 如果位置在已有大小内
		} else if (EXPECTED(h < ht->nTableSize)) {
// 添加到顺序数组，跳转点。2次引用（改变大小 或 初始化后）
add_to_packed:
			// 转到指定元素
			zv = ht->arPacked + h;
			
			// 新增的 空 Buckets 的初始化操作（由于是顺序操作，创建位置前面可能有空元素）
			/* incremental initialization of empty Buckets */
			
			// 如果 不是 HASH_ADD_NEW|HASH_ADD_NEXT( 必定也有 HASH_ADD ）
				// 那就是因为访问未使用过的编号才进来的
			if ((flag & (HASH_ADD_NEW|HASH_ADD_NEXT)) != (HASH_ADD_NEW|HASH_ADD_NEXT)) {
				// 如果位置超过了当前已使用位置
				if (h > ht->nNumUsed) {
					// 转到 已用元素最后 位置
					zval *q = ht->arPacked + ht->nNumUsed;
					// 遍历最后一个元素到当前元素之间（不包含当前元素）
					while (q != zv) {
						// 这一段 元素设置成 未定义
						ZVAL_UNDEF(q);
						// 移动到下一个元素
						q++;
					}
					// 但这里并没有把数组转成哈希表，要下一次查询或添加元素时才转换
				}
				// 如果顺序刚好接上（ h == ht->nNumUsed ），直接到这里，不需要调整位置
			}
			
			// 下一个空元素编号 和 已使用元素数  = h+1 （h从0开始）
				// 中间那段空的也算已使用了
			ht->nNextFreeElement = ht->nNumUsed = h + 1;
			
			// 有效元素数 +1
			ht->nNumOfElements++;
			// 情况1.2.1:
			// 如果是查找
			if (flag & HASH_LOOKUP) {
				// 新元素设置为null（就算是查询也把上面一堆事儿都干了）???
					// （经测试 $c = $a[10];empty($a[9]); 操作不会触发这个，这个lookup主要哪里用呢）
				ZVAL_NULL(zv);
			// 情况1.2.2:
			// 其他操作
			} else {
				// 把新元素添加到指定位置
				ZVAL_COPY_VALUE(zv, pData);
			}
			// 返回当前元素
			return zv;
			
			
		// ing4, 情况1.3 ：
		// 顺序数组，HASH_ADD_NEW|HASH_ADD_NEXT ( 必定也有 HASH_ADD ）或 访问未使用过的序号
		// 如果， 哈希值/2 小于 tablesize 并且 tablesize/2 小于元素数 
			// 翻倍后 序号可以直接覆盖要求的哈希值
			// 并且 哈希表已经使用一半以上
		} else if ((h >> 1) < ht->nTableSize &&
		           (ht->nTableSize >> 1) < ht->nNumOfElements) {
			// 大小翻倍
			zend_hash_packed_grow(ht);
			// 重新尝试添加. 并且返回
			goto add_to_packed;
			
			
		// ing4, 情况1.4 ：
		// 顺序数组，HASH_ADD_NEW|HASH_ADD_NEXT ( 必定也有 HASH_ADD ）或 访问未使用过的序号
		// 其他情况，序号大于元素数，又不适合翻倍，转成哈希表。
		} else {
			// 如果已使用大小比初始大小大
			if (ht->nNumUsed >= ht->nTableSize) {
				// 大小翻倍
				ht->nTableSize += ht->nTableSize;
			}
// 转成哈希表，跳转点, 1次引用
convert_to_hash:
			// 顺序数组转哈希表。这个转换 删除 了顺序数组标记 HASH_FLAG_PACKED
			zend_hash_packed_to_hash(ht);
			// 再进行 增改查操作
		}
		
		
	// ing4, 情况2 ：
	// 如果没初始化过。（既不是 哈希表 也不是 顺序数组）
	} else if (HT_FLAGS(ht) & HASH_FLAG_UNINITIALIZED) {
		// 情况2.1:
		// 哈希值可以作为索引号，没有越界，当成顺序数组处理
		if (h < ht->nTableSize) {
			// 初始化顺序数组
			zend_hash_real_init_packed_ex(ht);
			// 重新尝试添加, 并返回
			goto add_to_packed;
		}
		// 情况2.2:
		// 哈希值越界，当成哈希表处理
		// 初始化哈希表
		zend_hash_real_init_mixed(ht);
		
		
	// ing3, 情况3 ：
	// 如果 是哈希表（初始化过）。 只有这里单独用到 HASH_ADD_NEW
	} else {
		// ing4, 情况3.1:
		// 如果不是 HASH_ADD_NEW 添加新元素，那就是 【查找】 或 【更新】
			// 或者是调试模式（不相干）
		if ((flag & HASH_ADD_NEW) == 0 || ZEND_DEBUG) {
			// 【查找】 或 【更新】都要先查找
			// 使用 哈希值(zend_ulong) 获取哈希表里【没有key，只有哈希值的键值对】
			p = zend_hash_index_find_bucket(ht, h);
			// ing4, 情况3.1.1:
			// 如果能查到旧元素
			if (p) {
				// 如果是【查找】
				if (flag & HASH_LOOKUP) {
					// 直接返回
					return &p->val;
				}
				// 不是 HASH_LOOKUP，那就 只能是 【更新】
				// 断言：不可以是新建元素，
				ZEND_ASSERT((flag & HASH_ADD_NEW) == 0);
				// 取出键值对的值 指针
				zv = &p->val;
				// 转到替换，并返回
				goto replace;
			}
			// ing4, 情况3.1.2:
			// 【查找】 或 【更新】时，没有旧值，往下走。
				// 【查找】应返回空元素
				// 【更新】应插入新元素
		}
		
		// ing4, 情况3.2:
		// HASH_ADD_NEW
			// 【查找】不可能出现。 HASH_ADD_NEW 和 HASH_LOOKUP 不同时使用
			// 【更新】应插入新元素（HASH_ADD | HASH_ADD_NEW，或 HASH_ADD | HASH_ADD_NEW | HASH_ADD_NEXT)
				// 哈希表有没有 HASH_ADD_NEXT 没区别。
		
		// 如果哈希表满了，改变大小
		ZEND_HASH_IF_FULL_DO_RESIZE(ht);		/* If the Hash table is full, resize it */
	}
	
	// 会到达这里的情况 （一定是哈希表操作）
		// 情况1.1.2：顺序数组，不是按顺序使用的（当前元素在已用编号内，但值为未定义），转成哈希表。
		// 情况1.4：是按顺序新增，序号大于元素数，又不适合翻倍，转成哈希表。
		// 情况2.2：未初始化的哈希表，访问序号大于元素数（无法直接当成顺序数组使用），初始化成哈希表。
		// 情况3.1.2：已初始化的哈希表，【查找】 或 【更新】时，没有旧值
		// 情况3.2：哈希表中插入新元素
		
	
	// 索引号，自增
	idx = ht->nNumUsed++;
	// 利用哈希值 和 mask 计算索引号位置（倒序，例如共16个位置，第5个位置算出 -11）
	nIndex = h | ht->nTableMask;
	// 找到指定的 bucket
	p = ht->arData + idx;
	
	// val.u2.next 指向 同样哈希值的下一个元素, 存放 元素序号
	// 找到 (p1->arHash) 哈希->索引列表 中的指定索引号。p1:哈希表，p2:顺序号（和mask做过或运算, 负数）
	Z_NEXT(p->val) = HT_HASH(ht, nIndex);
	
	// 找到 (p1->arHash) 哈希->索引列表 中的指定索引号。p1:哈希表，p2:顺序号（和mask做过或运算, 负数）
	// 64位系统中 index 不用转换
	HT_HASH(ht, nIndex) = HT_IDX_TO_HASH(idx);
	// 如果哈希值大于 下一个空元素？
	if ((zend_long)h >= ht->nNextFreeElement) {
		// 更新下一个空元素序号，序号是 h+1, 防止超长。
		ht->nNextFreeElement = (zend_long)h < ZEND_LONG_MAX ? h + 1 : ZEND_LONG_MAX;
	}
	// 元素数量+1
	ht->nNumOfElements++;
	// 更新哈希值
	p->h = h;
	// key 为null
	p->key = NULL;
	// 如果是查询（查询也先把val更新了）
	if (flag & HASH_LOOKUP) {
		// 值为null
		ZVAL_NULL(&p->val);
	// 不是查询
	} else {
		// 更新键值对的值
		ZVAL_COPY_VALUE(&p->val, pData);
	}

	// 返回 键值对的值
	return &p->val;
}

// 此函数全局无调用
// ing4, 通过哈希值(zend_ulong)，增/改 指定元素。p1:哈希表，p2:哈希值，p3:新元素，p4:操作类型
// 比直接调用 _zend_hash_index_add_or_update_i 少一个 HASH_LOOKUP 操作
ZEND_API zval* ZEND_FASTCALL zend_hash_index_add_or_update(HashTable *ht, zend_ulong h, zval *pData, uint32_t flag)
{
	if (flag == HASH_ADD) {
		// 顺序数组中找到旧原素直接中断，在哈希表中找到旧原素会替换掉。p1:哈希表，p2:哈希值，p3:元素
		return zend_hash_index_add(ht, h, pData);
	} else if (flag == (HASH_ADD|HASH_ADD_NEW)) {
		// 顺序数组，碰到旧的新增。哈希表不查找旧原素，直接添加新元素。p1:哈希表，p2:哈希值，p3:元素
		return zend_hash_index_add_new(ht, h, pData);
	} else if (flag == (HASH_ADD|HASH_ADD_NEXT)) {
		ZEND_ASSERT(h == ht->nNextFreeElement);
		// （哈希值是最小整数时，归0）顺序数组中找到旧原素直接中断，在哈希表中找到旧原素会替换掉。p1:哈希表，p2:元素
		return zend_hash_next_index_insert(ht, pData);
	} else if (flag == (HASH_ADD|HASH_ADD_NEW|HASH_ADD_NEXT)) {
		ZEND_ASSERT(h == ht->nNextFreeElement);
		// （哈希值是最小整数时，归0）在顺序数组和哈希表中都直接添加，不查旧原素。p1:哈希表，p2:元素
		return zend_hash_next_index_insert_new(ht, pData);
	} else {
		ZEND_ASSERT(flag == HASH_UPDATE);
		// 找到旧原素都更新。找不到旧原素都添加，数组会转成哈希表。p1:哈希表，p2:哈希值，p3:元素
		return zend_hash_index_update(ht, h, pData);
	}
}


// ing4, 顺序数组中找到旧原素直接中断，在哈希表中找到旧原素会替换掉。p1:哈希表，p2:哈希值，p3:元素
ZEND_API zval* ZEND_FASTCALL zend_hash_index_add(HashTable *ht, zend_ulong h, zval *pData)
{
	// 通过哈希值(zend_ulong)，增/改/查 指定元素。p1:哈希表，p2:哈希值，p3:新元素，p4:操作类型
	return _zend_hash_index_add_or_update_i(ht, h, pData, HASH_ADD);
}

// ing4, 顺序数组，碰到旧的新增。哈希表不查找旧原素，直接添加新元素。p1:哈希表，p2:哈希值，p3:元素
ZEND_API zval* ZEND_FASTCALL zend_hash_index_add_new(HashTable *ht, zend_ulong h, zval *pData)
{
	// 通过哈希值(zend_ulong)，增/改/查 指定元素。p1:哈希表，p2:哈希值，p3:新元素，p4:操作类型
	return _zend_hash_index_add_or_update_i(ht, h, pData, HASH_ADD | HASH_ADD_NEW);
}

// ing4, 找到旧原素都更新。找不到旧原素都添加，数组会转成哈希表。p1:哈希表，p2:哈希值，p3:元素
ZEND_API zval* ZEND_FASTCALL zend_hash_index_update(HashTable *ht, zend_ulong h, zval *pData)
{
	// 通过哈希值(zend_ulong)，增/改/查 指定元素。p1:哈希表，p2:哈希值，p3:新元素，p4:操作类型
	return _zend_hash_index_add_or_update_i(ht, h, pData, HASH_UPDATE);
}

// 只有这两个用到 HASH_ADD_NEXT，
// ing4,（哈希值是最小整数时，归0）顺序数组中找到旧原素直接中断，在哈希表中找到旧原素会替换掉。p1:哈希表，p2:元素
ZEND_API zval* ZEND_FASTCALL zend_hash_next_index_insert(HashTable *ht, zval *pData)
{
	// 通过哈希值(zend_ulong)，增/改/查 指定元素。p1:哈希表，p2:哈希值，p3:新元素，p4:操作类型
	return _zend_hash_index_add_or_update_i(ht, ht->nNextFreeElement, pData, HASH_ADD | HASH_ADD_NEXT);
}

// 唯一用到 HASH_ADD | HASH_ADD_NEW | HASH_ADD_NEXT 个操作的
// ing4, （哈希值是最小整数时，归0）在顺序数组和哈希表中都直接添加，不查旧原素。p1:哈希表，p2:元素
ZEND_API zval* ZEND_FASTCALL zend_hash_next_index_insert_new(HashTable *ht, zval *pData)
{
	// 通过哈希值(zend_ulong)，增/改/查 指定元素。p1:哈希表，p2:哈希值，p3:新元素，p4:操作类型
	return _zend_hash_index_add_or_update_i(ht, ht->nNextFreeElement, pData, HASH_ADD | HASH_ADD_NEW | HASH_ADD_NEXT);
}

// 唯一用到 HASH_LOOKUP 的
// ing4, 通过哈希值查找原素。p1:哈希表，p2:哈希值
ZEND_API zval* ZEND_FASTCALL zend_hash_index_lookup(HashTable *ht, zend_ulong h)
{
	// 通过哈希值(zend_ulong)，增/改/查 指定元素。p1:哈希表，p2:哈希值，p3:新元素，p4:操作类型
	return _zend_hash_index_add_or_update_i(ht, h, NULL, HASH_LOOKUP);
}

// ing3, 给 Bucket 更换 key。不会改变bucket 在内存中的顺序。但会改变它的哈希对应的变量串
ZEND_API zval* ZEND_FASTCALL zend_hash_set_bucket_key(HashTable *ht, Bucket *b, zend_string *key)
{
	uint32_t nIndex;
	uint32_t idx, i;
	Bucket *p, *arData;

	// 检查一致性，调试用
	IS_CONSISTENT(ht);
	// 断言，哈希表的引用次数为1 或 ALLOW_COW_VIOLATION ，调试用
	HT_ASSERT_RC1(ht);
	ZEND_ASSERT(!HT_IS_PACKED(ht));
	// 计算key得哈希值
	(void)zend_string_hash_val(key);
	// 找到key对应的键值对
	p = zend_hash_find_bucket(ht, key);
	// 如果存在。不更新，直接返回
	if (UNEXPECTED(p)) {
		// 如果就是当前键值对，返回里面的变量。否则null
		return (p == b) ? &p->val : NULL;
	}
	// 如果 key 不存在，继续。key 不可以相同，单哈希值可以相同。这是关键。
	
	// 如果不是保留字
	if (!ZSTR_IS_INTERNED(key)) {
		// 添加key的引用次数
		zend_string_addref(key);
		// 删除标记 HASH_FLAG_STATIC_KEYS
		HT_FLAGS(ht) &= ~HASH_FLAG_STATIC_KEYS;
	}

	arData = ht->arData;
	// 从哈希表里删除
	/* del from hash */
	
	// 获取 Bucket 的顺序号
	// 64位系统中 index 不用转换
	idx = HT_IDX_TO_HASH(b - arData);
	// 利用哈希值 和 mask 计算索引号位置（倒序，例如共16个位置，第5个位置算出 -11）
	nIndex = b->h | ht->nTableMask;
	// 找到哈希->索引号列表 中的指定索引号。p1:列表结尾位置，p2:顺序号（和 tableMask 做过或运算的哈希值, 负数）
	i = HT_HASH_EX(arData, nIndex);
	// 如果两个序号相同，说明当前键值对是一串的第一个
	if (i == idx) {
		// 让哈希位置指向下一个元素（把当前元素摘除）
		// 找到哈希->索引号列表 中的指定索引号。p1:列表结尾位置，p2:顺序号（和 tableMask 做过或运算的哈希值, 负数）
		HT_HASH_EX(arData, nIndex) = Z_NEXT(b->val);
	// 
	} else {
		// 从头向后找，找到当前元素的前一个
		p = HT_HASH_TO_BUCKET_EX(arData, i);
		while (Z_NEXT(p->val) != idx) {
			i = Z_NEXT(p->val);
			p = HT_HASH_TO_BUCKET_EX(arData, i);
		}
		// 前一个的next 指向下一个（把当前元素摘除）
		Z_NEXT(p->val) = Z_NEXT(b->val);
	}
	// 删除当前元素的key
	zend_string_release(b->key);

	// 再添加回哈希表
	/* add to hash */
	// 顺序号， HT_IDX_TO_HASH 在下面
	idx = b - arData;
	// 新的key 和 哈希值
	b->key = key;
	b->h = ZSTR_H(key);
	// 利用哈希值 和 mask 计算索引号位置（倒序，例如共16个位置，第5个位置算出 -11）
	nIndex = b->h | ht->nTableMask;
	// 新的顺序号
	// 64位系统中 index 不用转换
	idx = HT_IDX_TO_HASH(idx);
	// 取得哈希位置上原来的顺序号
	// 找到哈希->索引号列表 中的指定索引号。p1:列表结尾位置，p2:顺序号（和 tableMask 做过或运算的哈希值, 负数）
	i = HT_HASH_EX(arData, nIndex);
	// 如果是 -1 或大于当前顺序号。把自己串在最前面
	if (i == HT_INVALID_IDX || i < idx) {
		// next 指针指向原来的 第一个
		Z_NEXT(b->val) = i;
		// 更新哈希位置，把自己的序号写入
		// 找到哈希->索引号列表 中的指定索引号。p1:列表结尾位置，p2:顺序号（和 tableMask 做过或运算的哈希值, 负数）
		HT_HASH_EX(arData, nIndex) = idx;
	// 如果有比自己顺序小的有效元素
	} else {
		// 找到可以插入的位置。 把自己插在p后面。i: 原有元素的顺序号，p: 原有元素的指针
		p = HT_HASH_TO_BUCKET_EX(arData, i);
		while (Z_NEXT(p->val) != HT_INVALID_IDX && Z_NEXT(p->val) > idx) {
			i = Z_NEXT(p->val);
			p = HT_HASH_TO_BUCKET_EX(arData, i);
		}
		// 自己的next 指向 p的next
		Z_NEXT(b->val) = Z_NEXT(p->val);
		// p 的next 指向自己
		Z_NEXT(p->val) = idx;
	}
	// 返回元素的值
	return &b->val;
}

// ing3, 重置哈希表大小。当使用率高于 97% 的时候，大小翻倍。p1:哈希表
static void ZEND_FASTCALL zend_hash_do_resize(HashTable *ht)
{
	// 检查一致性，调试用
	IS_CONSISTENT(ht);
	// 断言，哈希表的引用次数为1 或 ALLOW_COW_VIOLATION ，调试用
	HT_ASSERT_RC1(ht);

	// 不可以是顺序型
	ZEND_ASSERT(!HT_IS_PACKED(ht));
	// 如果 使用数 > 元素数 + 元素数/32。使用数小于 空间还足够，不重新分配。
	// 使用数 * 32/33 > 元素数。使用率小于 97%
	// 使用附加 term 来摊销压缩成本
	if (ht->nNumUsed > ht->nNumOfElements + (ht->nNumOfElements >> 5)) { /* additional term is there to amortize the cost of compaction */
		// 整理元素，填补中间的空位，然后给所有元素更新 编号，哈希->顺序表，next指针。p1: 哈希表
		zend_hash_rehash(ht);
	// 如果大小小于最大值，让哈希表大小翻倍
	// 这里又重新分配内存，更不是更改已有内存大小
	} else if (ht->nTableSize < HT_MAX_SIZE) {	/* Let's double the table size */
		// 旧数据块指针
		void *new_data, *old_data = HT_GET_DATA_ADDR(ht);
		// 新大小
		uint32_t nSize = ht->nTableSize + ht->nTableSize;
		// 旧元素指针
		Bucket *old_buckets = ht->arData;
		// mask必须有效
		// 通过哈希表大小 计算 mask = (uint32_t)(-2 * nTableSize)
		ZEND_ASSERT(HT_SIZE_TO_MASK(nSize));
		// 更新大小
		ht->nTableSize = nSize;
		// 计算 哈希表 数据大小：Bucket 列表大小 + 哈希->索引 列表大小 。p1:哈希表元素数, p2:mask
		// 通过哈希表大小 计算 mask = (uint32_t)(-2 * nTableSize)
		new_data = pemalloc(HT_SIZE_EX(nSize, HT_SIZE_TO_MASK(nSize)), GC_FLAGS(ht) & IS_ARRAY_PERSISTENT);
		// 更新 nTableMask
		// 通过哈希表大小 计算 mask = (uint32_t)(-2 * nTableSize)
		ht->nTableMask = HT_SIZE_TO_MASK(ht->nTableSize);
		// 设置 p1->arData 到 Bucket 列表开头。p1:哈希表，p2: 内存块开头位置
		HT_SET_DATA_ADDR(ht, new_data);
		// 把旧元素复制过来
		memcpy(ht->arData, old_buckets, sizeof(Bucket) * ht->nNumUsed);
		// 释放旧数据块
		pefree(old_data, GC_FLAGS(ht) & IS_ARRAY_PERSISTENT);
		// 整理元素，填补中间的空位，然后给所有元素更新 编号，哈希->顺序表，next指针。p1: 哈希表
		zend_hash_rehash(ht);
		
	// 大小达到最大值
	} else {
		// 报错：内存分配时溢出
		zend_error_noreturn(E_ERROR, "Possible integer overflow in memory allocation (%u * %zu + %zu)", ht->nTableSize * 2, sizeof(Bucket) + sizeof(uint32_t), sizeof(Bucket));
	}
}

// 120 行
// ing3, 整理元素，填补中间的空位，然后给所有元素更新 编号，哈希->顺序表，next指针。p1: 哈希表
ZEND_API void ZEND_FASTCALL zend_hash_rehash(HashTable *ht)
{
	Bucket *p;
	uint32_t nIndex, i;
	// 检查一致性，调试用
	IS_CONSISTENT(ht);
	// ing4, 情况1：
	// 如果里面没有元素
	if (UNEXPECTED(ht->nNumOfElements == 0)) {
		// 如果不是 未初始化状态
		if (!(HT_FLAGS(ht) & HASH_FLAG_UNINITIALIZED)) {
			// 报废已用
			ht->nNumUsed = 0;
			// 重置 (p1)->arHash 哈希->索引列表。 所有元素 都设置成 -1。p1:哈希表
			HT_HASH_RESET(ht);
		}
		// 中断
		return;
	}
	
	// 重置 (p1)->arHash 哈希->索引列表。 所有元素 都设置成 -1。p1:哈希表
	HT_HASH_RESET(ht);
	// 临时变量
	i = 0;
	// bucket 列表开着
	p = ht->arData;
	// 情况2：
	// 如果 有元素 且 中间没有空元素
	if (HT_IS_WITHOUT_HOLES(ht)) {
		// 遍历全部元素，更新所有的序号和 next
		do {
			// 利用哈希值 和 mask 计算索引号位置（倒序，例如共16个位置，第5个位置算出 -11）
			nIndex = p->h | ht->nTableMask;
			// 找到 (p1->arHash) 哈希->索引列表 中的指定索引号。p1:哈希表，p2:顺序号（和mask做过或运算, 负数）
			Z_NEXT(p->val) = HT_HASH(ht, nIndex);
			// 找到 (p1->arHash) 哈希->索引列表 中的指定索引号。p1:哈希表，p2:顺序号（和mask做过或运算, 负数）
			// 64位系统中 index 不用转换
			HT_HASH(ht, nIndex) = HT_IDX_TO_HASH(i);
			// 下一个
			p++;
		// 下一个
		} while (++i < ht->nNumUsed);
		
	// 情况3：（直到结尾）
	// 有元素且 中间没有空元素
	} else {
		// 原使用数量
		uint32_t old_num_used = ht->nNumUsed;
		// 结尾： while (++i < ht->nNumUsed); 
		// 遍历使用过的元素
		do {
			// 子元素情况3.1：
			// 如果元素未定义。需要把后面的元素移过来填充空位
			if (UNEXPECTED(Z_TYPE(p->val) == IS_UNDEF)) {
				// 这里i应该是 0
				uint32_t j = i;
				// bucket 表开头
				Bucket *q = p;
				
				// 子元素情况3.1.1：
				// 如果哈希表 没有迭代器
				if (EXPECTED(!HT_HAS_ITERATORS(ht))) {
					// 遍历后半段 每一个已用元素
					while (++i < ht->nNumUsed) {
						// 下一个
						p++;
						// 如果 元素有效，把有效的元素移过来填充空位
						if (EXPECTED(Z_TYPE_INFO(p->val) != IS_UNDEF)) {
							// 自己给自己么？
							ZVAL_COPY_VALUE(&q->val, &p->val);
							// 复制哈希值
							q->h = p->h;
							// 利用哈希值 和 mask 计算索引号位置（倒序，例如共16个位置，第5个位置算出 -11）
							nIndex = q->h | ht->nTableMask;
							// 复制key
							q->key = p->key;
							// 处理 arHash
							// 找到 (p1->arHash) 哈希->索引列表 中的指定索引号。p1:哈希表，p2:顺序号（和mask做过或运算, 负数）
							Z_NEXT(q->val) = HT_HASH(ht, nIndex);
							// 找到 (p1->arHash) 哈希->索引列表 中的指定索引号。p1:哈希表，p2:顺序号（和mask做过或运算, 负数）
							// 64位系统中 index 不用转换
							HT_HASH(ht, nIndex) = HT_IDX_TO_HASH(j);
							// 如果内部指针指向本元素
							if (UNEXPECTED(ht->nInternalPointer == i)) {
								// 更新内部的指针位置
								ht->nInternalPointer = j;
							}
							// 下一个 bucket
							q++;
							// 下一个编号
							j++;
						}
					}
				// 子元素情况3.1.2：
				// 有迭代器
				} else {
					// 所有迭代器中最小的迭代位置
					uint32_t iter_pos = zend_hash_iterators_lower_pos(ht, 0);
					// 遍历所有有效元素
					while (++i < ht->nNumUsed) {
						//
						p++;
						// 如果元素有效，把有效的元素移过来填充空位
						if (EXPECTED(Z_TYPE_INFO(p->val) != IS_UNDEF)) {
							// 复制元素值
							ZVAL_COPY_VALUE(&q->val, &p->val);
							// 复制 哈希值
							q->h = p->h;
							// 利用哈希值 和 mask 计算索引号位置（倒序，例如共16个位置，第5个位置算出 -11）
							nIndex = q->h | ht->nTableMask;
							// 复制key
							q->key = p->key;
							// 处理 arHash
							// 找到 (p1->arHash) 哈希->索引列表 中的指定索引号。p1:哈希表，p2:顺序号（和mask做过或运算, 负数）
							Z_NEXT(q->val) = HT_HASH(ht, nIndex);
							// 找到 (p1->arHash) 哈希->索引列表 中的指定索引号。p1:哈希表，p2:顺序号（和mask做过或运算, 负数）
							// 64位系统中 index 不用转换
							HT_HASH(ht, nIndex) = HT_IDX_TO_HASH(j);
							// 如果内部指针指向本元素
							if (UNEXPECTED(ht->nInternalPointer == i)) {
								// 更新内部指针
								ht->nInternalPointer = j;
							}
							// 如果当前位置在迭代位置之后
							if (UNEXPECTED(i >= iter_pos)) {
								do {
									// 更新全部迭代器的位置从 iter_pos 到 j
									zend_hash_iterators_update(ht, iter_pos, j);
									// 所有迭代器中最小的迭代位置
									iter_pos = zend_hash_iterators_lower_pos(ht, iter_pos + 1);
								} while (iter_pos < i);
							}
							// 下一个
							q++;
							// 
							j++;
						}
					}
				}
				// 更新已用元素数
				ht->nNumUsed = j;
				// 整理完后面的元素后，直接跳出循环
				break;
			}
			
			// 子元素情况3.2：
				// 处理正常元素，更新它的序号和next指针
			
			// 利用哈希值 和 mask 计算索引号位置（倒序，例如共16个位置，第5个位置算出 -11）
			nIndex = p->h | ht->nTableMask;
			// 找到 (p1->arHash) 哈希->索引列表 中的指定索引号。p1:哈希表，p2:顺序号（和mask做过或运算, 负数）
			Z_NEXT(p->val) = HT_HASH(ht, nIndex);
			// 找到 (p1->arHash) 哈希->索引列表 中的指定索引号。p1:哈希表，p2:顺序号（和mask做过或运算, 负数）
			// 64位系统中 index 不用转换
			HT_HASH(ht, nIndex) = HT_IDX_TO_HASH(i);
			// 后移
			p++;
		// 遍历使用过的元素
		} while (++i < ht->nNumUsed);

		// 移动指针 从结尾后一个到 结尾后新的一个 ，这样新插入的元素就可以正确获取了
		/* Migrate pointer to one past the end of the array to the new one past the end, so that
		 * newly inserted elements are picked up correctly. */
		// 如果哈希表有迭代器
		if (UNEXPECTED(HT_HAS_ITERATORS(ht))) {
			// 更新哈希表的迭代器
			_zend_hash_iterators_update(ht, old_num_used, ht->nNumUsed);
		}
	}
}

// ing3, 顺序数组删除指定 序号 的元素。删除元素主要靠销毁器(pDestructor)完成。p1:哈希表，p2:索引号，p3:元素zval指针
static zend_always_inline void _zend_hash_packed_del_val(HashTable *ht, uint32_t idx, zval *zv)
{
	// 64位系统中 index 不用转换
	idx = HT_HASH_TO_IDX(idx);
	// 先减元素数量
	ht->nNumOfElements--;
	// 如果删除内部指针指向的元素，或哈希表有迭代器。这情况太常见了。
	if (ht->nInternalPointer == idx || UNEXPECTED(HT_HAS_ITERATORS(ht))) {
		// 临时变量
		uint32_t new_idx;
		// 新顺序号，从这里向右查找有效元素
		new_idx = idx;
		// 从 idx 开始往后遍历
		while (1) {
			// 指针后移
			new_idx++;
			// 到头
			if (new_idx >= ht->nNumUsed) {
				// 跳出
				break;
			// 找到有效元素，退出
			} else if (Z_TYPE(ht->arPacked[new_idx]) != IS_UNDEF) {
				// 跳出
				break;
			}
		}
		// 碰到内部指针
		if (ht->nInternalPointer == idx) {
			// 更新内部指针
			ht->nInternalPointer = new_idx;
		}
		// 更新全部迭代器
		zend_hash_iterators_update(ht, idx, new_idx);
	}
	// 如果删除最后一个元素
	if (ht->nNumUsed - 1 == idx) {
		// 清理右侧无效元素（只减使用数）
		do {
			// 指针左移一个位置
			ht->nNumUsed--;
		//  ？
		} while (ht->nNumUsed > 0 && (UNEXPECTED(Z_TYPE(ht->arPacked[ht->nNumUsed-1]) == IS_UNDEF)));
		// 如果内部指针溢出，让它指向有效元素末尾
		ht->nInternalPointer = MIN(ht->nInternalPointer, ht->nNumUsed);
	}
	// 如果有销毁器，调用销毁器销毁
	if (ht->pDestructor) {
		zval tmp;
		// 先 复制到临时变量中
		ZVAL_COPY_VALUE(&tmp, zv);
		// 再 把元素设置成无效，
		ZVAL_UNDEF(zv);
		// 再 删除指向的对象
		ht->pDestructor(&tmp);
	} else {
		// 直接把元素设置成无效
		ZVAL_UNDEF(zv);
	}
}

// ing3,删除元素 ，这里只处理值，key在外面处理。p 是要删掉的元素,prev 是它的前一个元素。内部多次引用
static zend_always_inline void _zend_hash_del_el_ex(HashTable *ht, uint32_t idx, Bucket *p, Bucket *prev)
{
	// 如果有前一个,让前一个的next跳过本元素
	if (prev) {
		Z_NEXT(prev->val) = Z_NEXT(p->val);
	// 没有前一个
	} else {
		// 当前元素的 arHash 指向当前元素的下一个元素
		// 找到 (p1->arHash) 哈希->索引列表 中的指定索引号。p1:哈希表，p2:顺序号（和mask做过或运算, 负数）
		HT_HASH(ht, p->h | ht->nTableMask) = Z_NEXT(p->val);
	}
	// 64位系统中 index 不用转换
	idx = HT_HASH_TO_IDX(idx);
	// 元素数量 -1
	ht->nNumOfElements--;
	// 如果内部指针正指向 idx, 并且哈希表有迭代器
	if (ht->nInternalPointer == idx || UNEXPECTED(HT_HAS_ITERATORS(ht))) {
		uint32_t new_idx;

		new_idx = idx;
		// 找到后面的第一个有效元素
		while (1) {
			// 从下一个元素开始找
			new_idx++;
			// 找到最后一个已用元素为止
			if (new_idx >= ht->nNumUsed) {
				break;
			// 找到第一个有效元素
			} else if (Z_TYPE(ht->arData[new_idx].val) != IS_UNDEF) {
				break;
			}
		}
		// 更新内部指针
		if (ht->nInternalPointer == idx) {
			ht->nInternalPointer = new_idx;
		}
		// 更新迭代器, 把位置是 idx的，更新到 new_idx
		zend_hash_iterators_update(ht, idx, new_idx);
	}
	// 如果当前元素是最后一个
	if (ht->nNumUsed - 1 == idx) {
		// 向右找
		do {
			ht->nNumUsed--;
		// 如果元素存在，并且是 IS_UNDEF
		} while (ht->nNumUsed > 0 && (UNEXPECTED(Z_TYPE(ht->arData[ht->nNumUsed-1].val) == IS_UNDEF)));
		// 更新内部指针指向
		ht->nInternalPointer = MIN(ht->nInternalPointer, ht->nNumUsed);
	}
	// 如果有销毁器
	if (ht->pDestructor) {
		zval tmp;
		// 先把要销毁的变量复制出来
		ZVAL_COPY_VALUE(&tmp, &p->val);
		// 把当前元素设置成 IS_UNDEF
		ZVAL_UNDEF(&p->val);
		// 删除旧变量
		ht->pDestructor(&tmp);
	// 没有销毁器，把当前元素设置成 IS_UNDEF
	} else {
		ZVAL_UNDEF(&p->val);
	}
}

// ing3, 哈希表中删除指定 序号的 bucket。p1:哈希表，p2:序号，p3:bucket指针
// 其实最后这个元素的内存也在，不能单独释放。还可以复用。
static zend_always_inline void _zend_hash_del_el(HashTable *ht, uint32_t idx, Bucket *p)
{
	Bucket *prev = NULL;
	uint32_t nIndex;
	uint32_t i;
	// 利用哈希值 和 mask 计算索引号位置（倒序，例如共16个位置，第5个位置算出 -11）
	nIndex = p->h | ht->nTableMask;
	// 找到 (p1->arHash) 哈希->索引列表 中的指定索引号。p1:哈希表，p2:顺序号（和mask做过或运算, 负数）
	i = HT_HASH(ht, nIndex);

	// 如果元素序号和传入序号不符，说明这里有 一串元素，而且p 不是第一个。
	// 要先找到目标元素的前一个元素
	if (i != idx) {
		// 相同哈希的第一个元素
		// 找到哈希表中第 idx 个bucket。p1:哈希表，p2:bucket序号
		prev = HT_HASH_TO_BUCKET(ht, i);
		// 找到目标元素的前一个元素
		while (Z_NEXT(prev->val) != idx) {
			i = Z_NEXT(prev->val);
			// 找到哈希表中第 idx 个bucket。p1:哈希表，p2:bucket序号
			prev = HT_HASH_TO_BUCKET(ht, i);
		}
	}

	// 先删除key
	if (p->key) {
		zend_string_release(p->key);
		p->key = NULL;
	}
	// 删除值。
	_zend_hash_del_el_ex(ht, idx, p, prev);
}

// ing3, 顺序数组中删除指定元素
ZEND_API void ZEND_FASTCALL zend_hash_packed_del_val(HashTable *ht, zval *zv)
{
	// 检查一致性，调试用
	IS_CONSISTENT(ht);
	// 断言，哈希表的引用次数为1 或 ALLOW_COW_VIOLATION ，调试用
	HT_ASSERT_RC1(ht);
	ZEND_ASSERT(HT_IS_PACKED(ht));
	// 顺序数组删除指定 序号 的元素。删除元素主要靠销毁器(pDestructor)完成。
	// 64位系统中 index 不用转换
	_zend_hash_packed_del_val(ht, HT_IDX_TO_HASH(zv - ht->arPacked), zv);
}

// ing3, 哈希表中删除指定键值对
ZEND_API void ZEND_FASTCALL zend_hash_del_bucket(HashTable *ht, Bucket *p)
{
	// 检查一致性，调试用
	IS_CONSISTENT(ht);
	// 断言，哈希表的引用次数为1 或 ALLOW_COW_VIOLATION ，调试用
	HT_ASSERT_RC1(ht);
	// 不可以是顺序数组
	ZEND_ASSERT(!HT_IS_PACKED(ht));
	// 哈希表中删除指定 序号的 bucket。p1:哈希表，p2:序号，p3:bucket指针
	// 64位系统中 index 不用转换
	_zend_hash_del_el(ht, HT_IDX_TO_HASH(p - ht->arData), p);
}

// ing3, 通过key，删除哈希表中指定元素
ZEND_API zend_result ZEND_FASTCALL zend_hash_del(HashTable *ht, zend_string *key)
{
	zend_ulong h;
	uint32_t nIndex;
	uint32_t idx;
	Bucket *p;
	Bucket *prev = NULL;

	// 检查一致性，调试用
	IS_CONSISTENT(ht);
	// 断言，哈希表的引用次数为1 或 ALLOW_COW_VIOLATION ，调试用
	HT_ASSERT_RC1(ht);
	// 如果需要，计算哈希值
	h = zend_string_hash_val(key);
	// 利用哈希值 和 mask 计算索引号位置（倒序，例如共16个位置，第5个位置算出 -11）
	nIndex = h | ht->nTableMask;

	// 找到 (p1->arHash) 哈希->索引列表 中的指定索引号。p1:哈希表，p2:顺序号（和mask做过或运算, 负数）
	idx = HT_HASH(ht, nIndex);
	// 如果顺序号有效，一次获取元素串
	while (idx != HT_INVALID_IDX) {
		// 找到哈希表中第 idx 个bucket。p1:哈希表，p2:bucket序号
		p = HT_HASH_TO_BUCKET(ht, idx);
		// 如果key是同一个对象，（或 key存在并且 key和哈希值都比配）
		if ((p->key == key) ||
			(p->h == h &&
		     p->key &&
		     zend_string_equal_content(p->key, key))) {
			// 删除key
			zend_string_release(p->key);
			// 指针置空
			p->key = NULL;
			// 删除键值对的值
			_zend_hash_del_el_ex(ht, idx, p, prev);
			// 返回成功
			return SUCCESS;
		}
		// 如果没找到，继续往下找
		prev = p;
		// 下一个元素的序号
		idx = Z_NEXT(p->val);
	}
	// 返回失败
	return FAILURE;
}

// ing2, 删除哈希表中的元素，包含处理间接引用（附加 HASH_FLAG_HAS_EMPTY_IND 标记）。p1:哈希表，p2:key
ZEND_API zend_result ZEND_FASTCALL zend_hash_del_ind(HashTable *ht, zend_string *key)
{
	zend_ulong h;
	uint32_t nIndex;
	uint32_t idx;
	Bucket *p;
	Bucket *prev = NULL;

	// 检查一致性，调试用
	IS_CONSISTENT(ht);
	// 断言，哈希表的引用次数为1 或 ALLOW_COW_VIOLATION ，调试用
	HT_ASSERT_RC1(ht);
	// 哈希值
	h = zend_string_hash_val(key);
	// 利用哈希值 和 mask 计算索引号位置（倒序，例如共16个位置，第5个位置算出 -11）
	nIndex = h | ht->nTableMask;
	// 找到 (p1->arHash) 哈希->索引列表 中的指定索引号。p1:哈希表，p2:顺序号（和mask做过或运算, 负数）
	idx = HT_HASH(ht, nIndex);
	// 遍历一串元素
	while (idx != HT_INVALID_IDX) {
		// 找到哈希表中第 idx 个bucket。p1:哈希表，p2:bucket序号
		p = HT_HASH_TO_BUCKET(ht, idx);
		// key对象相同。或 （有key，哈希值相同，key相同）
		if ((p->key == key) ||
			(p->h == h &&
		     p->key &&
		     zend_string_equal_content(p->key, key))) {
			// 如果只间接引用
			if (Z_TYPE(p->val) == IS_INDIRECT) {
				// 追踪间接引用对象
				zval *data = Z_INDIRECT(p->val);
				// 如果无效
				if (UNEXPECTED(Z_TYPE_P(data) == IS_UNDEF)) {
					// 返回 失败
					return FAILURE;
				// 元素有效
				} else {
					// 销毁引用对象
					if (ht->pDestructor) {
						zval tmp;
						ZVAL_COPY_VALUE(&tmp, data);
						// 先把引用对象设置成无效，再调用销毁器销毁
						ZVAL_UNDEF(data);
						ht->pDestructor(&tmp);
					// 无销毁器，只把引用对象标记成无效
					} else {
						ZVAL_UNDEF(data);
					}
					// 添加标记，哈希表包含空的间接引用元素
					HT_FLAGS(ht) |= HASH_FLAG_HAS_EMPTY_IND;
				}
				// ? 间接引用元素不调用 _zend_hash_del_el_ex , 因为交给销毁器了
			// 普通元素
			} else {
				// 删除key
				zend_string_release(p->key);
				p->key = NULL;
				// 删除键值对的值
				_zend_hash_del_el_ex(ht, idx, p, prev);
			}
			// 返回成功
			return SUCCESS;
		}
		// 没找到，下一个
		prev = p;
		// 找到下一个元素的序号
		idx = Z_NEXT(p->val);
	}
	// 没找到要删除的元素，返回失败
	return FAILURE;
}

// ing3, 删除元素并处理间接引用。p1:哈希表，p2:key，p3:长度
ZEND_API zend_result ZEND_FASTCALL zend_hash_str_del_ind(HashTable *ht, const char *str, size_t len)
{
	zend_ulong h;
	uint32_t nIndex;
	uint32_t idx;
	Bucket *p;
	Bucket *prev = NULL;

	// 检查一致性，调试用
	IS_CONSISTENT(ht);
	// 断言，哈希表的引用次数为1 或 ALLOW_COW_VIOLATION ，调试用
	HT_ASSERT_RC1(ht);
	
	// 计算作为数组key的哈希值
	h = zend_inline_hash_func(str, len);
	// 利用哈希值 和 mask 计算索引号位置（倒序，例如共16个位置，第5个位置算出 -11）
	nIndex = h | ht->nTableMask;

	// 找到 (p1->arHash) 哈希->索引列表 中的指定索引号。p1:哈希表，p2:顺序号（和mask做过或运算, 负数）
	idx = HT_HASH(ht, nIndex);
	// 遍历相同哈希的一串元素
	while (idx != HT_INVALID_IDX) {
		// 找到哈希表中第 idx 个bucket。p1:哈希表，p2:bucket序号
		p = HT_HASH_TO_BUCKET(ht, idx);
		// 如果找到：哈希相同 且有key 且 key 匹配
		if ((p->h == h)
			 && p->key
			 && zend_string_equals_cstr(p->key, str, len)) {
			// 如果是间接引用
			if (Z_TYPE(p->val) == IS_INDIRECT) {
				// 追踪对象
				zval *data = Z_INDIRECT(p->val);
				// 对象无效，报错
				if (UNEXPECTED(Z_TYPE_P(data) == IS_UNDEF)) {
					return FAILURE;
				// 有效，调用销毁器销毁
				} else {
					if (ht->pDestructor) {
						ht->pDestructor(data);
					}
					// 对象标记成无效
					ZVAL_UNDEF(data);
					// 添加标记，包含空间接引用
					HT_FLAGS(ht) |= HASH_FLAG_HAS_EMPTY_IND;
				}
			// 非间接引用
			} else {
				// 删除key 
				zend_string_release(p->key);
				p->key = NULL;
				// 删除键值对的值
				_zend_hash_del_el_ex(ht, idx, p, prev);
			}
			return SUCCESS;
		}
		prev = p;
		idx = Z_NEXT(p->val);
	}
	return FAILURE;
}

// ing3, 哈希表中删除指定键。p1:哈希表，p2:key，p3:长度
ZEND_API zend_result ZEND_FASTCALL zend_hash_str_del(HashTable *ht, const char *str, size_t len)
{
	zend_ulong h;
	uint32_t nIndex;
	uint32_t idx;
	Bucket *p;
	Bucket *prev = NULL;

	// 检查一致性，调试用
	IS_CONSISTENT(ht);
	// 断言，哈希表的引用次数为1 或 ALLOW_COW_VIOLATION ，调试用
	HT_ASSERT_RC1(ht);

	// 计算作为数组key的哈希值
	h = zend_inline_hash_func(str, len);
	// 利用哈希值 和 mask 计算索引号位置（倒序，例如共16个位置，第5个位置算出 -11）
	nIndex = h | ht->nTableMask;

	// 找到 (p1->arHash) 哈希->索引列表 中的指定索引号。p1:哈希表，p2:顺序号（和mask做过或运算, 负数）
	idx = HT_HASH(ht, nIndex);
	
	// 如果 序号有效，继续查找
	while (idx != HT_INVALID_IDX) {
		// 找到哈希表中第 idx 个bucket。p1:哈希表，p2:bucket序号
		p = HT_HASH_TO_BUCKET(ht, idx);
		if ((p->h == h)
			 && p->key
			 && zend_string_equals_cstr(p->key, str, len)) {
			// 释放key字串
			zend_string_release(p->key);
			// 指针置空
			p->key = NULL;
			// 删除键值对的值
			_zend_hash_del_el_ex(ht, idx, p, prev);
			// 返回成功
			return SUCCESS;
		}
		// 记录成上一个元素，用于更新删除后元素的指针
		prev = p;
		// 找到下一个元素的序号
		idx = Z_NEXT(p->val);
	}
	// 返回失败
	return FAILURE;
}

// ing3, 删除指定（哈希值）的键值对。p1:哈希表，p2:哈希值
ZEND_API zend_result ZEND_FASTCALL zend_hash_index_del(HashTable *ht, zend_ulong h)
{
	uint32_t nIndex;
	uint32_t idx;
	Bucket *p;
	Bucket *prev = NULL;

	// 检查一致性，调试用
	IS_CONSISTENT(ht);
	// 断言，哈希表的引用次数为1 或 ALLOW_COW_VIOLATION ，调试用
	HT_ASSERT_RC1(ht);

	// 如果是顺序数组
	if (HT_IS_PACKED(ht)) {
		// 查找位置在已使用部分
		if (h < ht->nNumUsed) {
			// 找到元素
			zval *zv = ht->arPacked + h;
			// 如果元素是已使用
			if (Z_TYPE_P(zv) != IS_UNDEF) {
				// 顺序数组删除指定 序号 的元素。删除元素主要靠销毁器(pDestructor)完成。
				// 64位系统中 index 不用转换
				_zend_hash_packed_del_val(ht, HT_IDX_TO_HASH(h), zv);
				// 返回成功
				return SUCCESS;
			}
			// 如果未使用
		}
		// 如果 位置不在已使用范围内
		
		// 返回失败
		return FAILURE;
	}
	
	// 是哈希表
	// 利用哈希值 和 mask 计算索引号位置（倒序，例如共16个位置，第5个位置算出 -11）
	nIndex = h | ht->nTableMask;

	// 找到 (p1->arHash) 哈希->索引列表 中的指定索引号。p1:哈希表，p2:顺序号（和mask做过或运算, 负数）
	idx = HT_HASH(ht, nIndex);
	while (idx != HT_INVALID_IDX) {
		// 找到哈希表中第 idx 个bucket。p1:哈希表，p2:bucket序号
		p = HT_HASH_TO_BUCKET(ht, idx);
		// 如果哈希值相同，key是null
		if ((p->h == h) && (p->key == NULL)) {
			// 删除键值对的值
			_zend_hash_del_el_ex(ht, idx, p, prev);
			// 返回成功
			return SUCCESS;
		}
		// 记录成上一个元素
		prev = p;
		// 下一个元素序号
		idx = Z_NEXT(p->val);
	}
	// 返回失败
	return FAILURE;
}

// ing3, 销毁哈希表：销毁哈希表数据, 这里没有释放 ht 内存，需要在外面释放
ZEND_API void ZEND_FASTCALL zend_hash_destroy(HashTable *ht)
{
	// 检查一致性，调试用
	IS_CONSISTENT(ht);
	// 调试用
	HT_ASSERT(ht, GC_REFCOUNT(ht) <= 1);

	// 如果有效
	if (ht->nNumUsed) {
		// 分支1 : 如果是顺序数组
		if (HT_IS_PACKED(ht)) {
			// 有销毁器
			if (ht->pDestructor) {
				zval *zv = ht->arPacked;
				zval *end = zv + ht->nNumUsed;
				// 设置不一致状态 销毁中
				SET_INCONSISTENT(HT_IS_DESTROYING);
				// 没有空元素，遍历销毁每一个元素
				if (HT_IS_WITHOUT_HOLES(ht)) {
					do {
						ht->pDestructor(zv);
					} while (++zv != end);
				// 有空元素
				} else {
					do {
						// 只销毁所有有效元素
						if (EXPECTED(Z_TYPE_P(zv) != IS_UNDEF)) {
							ht->pDestructor(zv);
						}
					} while (++zv != end);
				}
				// 设置不一致状态 已销毁
				SET_INCONSISTENT(HT_DESTROYED);
			}
			// 删除迭代器（解除关联）
			zend_hash_iterators_remove(ht);
		// 分支2 ： 哈希表
		} else {
			Bucket *p = ht->arData;
			Bucket *end = p + ht->nNumUsed;
			// 分支2.1 : 有销毁器
			if (ht->pDestructor) {
				// 设置不一致状态 销毁中
				SET_INCONSISTENT(HT_IS_DESTROYING);
				// 2.1.1 只包含保留字 key。这个分支里没有销毁 key
				// 是否只包含静态key: 是索引数组 或者 所有key都是保留字
				if (HT_HAS_STATIC_KEYS_ONLY(ht)) {
					// 如果没有空元素
					if (HT_IS_WITHOUT_HOLES(ht)) {
						// 用销毁器删除每一个元素
						do {
							ht->pDestructor(&p->val);
						} while (++p != end);
					// 有空元素
					} else {
						// 用销毁器删除有效元素
						do {
							if (EXPECTED(Z_TYPE(p->val) != IS_UNDEF)) {
								ht->pDestructor(&p->val);
							}
						} while (++p != end);
					}
				// 2.1.2 不只包含保留字 key, 没有空元素
				} else if (HT_IS_WITHOUT_HOLES(ht)) {
					do {
						// 调用 pDestructor 删除每一个 值
						ht->pDestructor(&p->val);
						// 如果有key ，删除 key
						if (EXPECTED(p->key)) {
							zend_string_release(p->key);
						}
					} while (++p != end);
				// 2.1.3 不只包含保留字 key, 有空元素
				} else {
					do {
						// 只操作有效元素
						if (EXPECTED(Z_TYPE(p->val) != IS_UNDEF)) {
							// 调用 pDestructor 删除每一个 值
							ht->pDestructor(&p->val);
							// 如果有key ，删除 key
							if (EXPECTED(p->key)) {
								zend_string_release(p->key);
							}
						}
					} while (++p != end);
				}
				// 设置不一致状态 已销毁
				SET_INCONSISTENT(HT_DESTROYED);
			// 分支2.2 : 没有销毁器
			} else {
				// 是否只包含静态key: 是索引数组 或者 所有key都是保留字
				if (!HT_HAS_STATIC_KEYS_ONLY(ht)) {
					// 释放每一个key
					do {
						if (EXPECTED(p->key)) {
							zend_string_release(p->key);
						}
					} while (++p != end);
				}
			}
			// 解除迭代器关联
			zend_hash_iterators_remove(ht);
		}
	// 如果初始化过
	} else if (EXPECTED(HT_FLAGS(ht) & HASH_FLAG_UNINITIALIZED)) {
		return;
	}
	// 销毁哈希表数据块，包括 哈希->索引列表，zval列表，bucket列表。这里没有释放 ht 内存，需要在外面释放。
	// 从p1->arData找到哈希表数据块开头位置。p1:哈希表。
	pefree(HT_GET_DATA_ADDR(ht), GC_FLAGS(ht) & IS_ARRAY_PERSISTENT);
}

// ing2, 销毁并释放哈希表。p1:哈希表
ZEND_API void ZEND_FASTCALL zend_array_destroy(HashTable *ht)
{
	// 检查一致性，调试用
	IS_CONSISTENT(ht);
	// 调试用
	HT_ASSERT(ht, GC_REFCOUNT(ht) <= 1);

	// 从可能的循环中删除
	/* break possible cycles */
	GC_REMOVE_FROM_BUFFER(ht);
	// ？
	GC_TYPE_INFO(ht) = GC_NULL /*???| (GC_WHITE << 16)*/;
	
	// 如果有有在用的元素
	if (ht->nNumUsed) {
		// 在一些少见的 case 里，普通 array 的销毁器可能会被更改
		/* In some rare cases destructors of regular arrays may be changed */
		
		// #define ZVAL_PTR_DTOR zval_ptr_dtor
		// 如果销毁器不是 zval_ptr_dtor
		if (UNEXPECTED(ht->pDestructor != ZVAL_PTR_DTOR)) {
			// 销毁哈希表 
			zend_hash_destroy(ht);
			goto free_ht;
		}

		// 一致性状态：销毁中
		SET_INCONSISTENT(HT_IS_DESTROYING);

		// 顺序数组
		if (HT_IS_PACKED(ht)) {
			zval *zv = ht->arPacked;
			zval *end = zv + ht->nNumUsed;

			do {
				i_zval_ptr_dtor(zv);
			} while (++zv != end);
		// 哈希表
		} else {
			Bucket *p = ht->arData;
			Bucket *end = p + ht->nNumUsed;
			// 是否只包含静态key: 是索引数组 或者 所有key都是保留字
			if (HT_HAS_STATIC_KEYS_ONLY(ht)) {
				do {
					i_zval_ptr_dtor(&p->val);
				} while (++p != end);
			// 如果 中间没有空元素
			} else if (HT_IS_WITHOUT_HOLES(ht)) {
				do {
					i_zval_ptr_dtor(&p->val);
					// 如果有key
					if (EXPECTED(p->key)) {
						// 释放key
						zend_string_release_ex(p->key, 0);
					}
				} while (++p != end);
			// 中间有空元素
			} else {
				do {
					// 如果元素有效
					if (EXPECTED(Z_TYPE(p->val) != IS_UNDEF)) {
						// 
						i_zval_ptr_dtor(&p->val);
						// 如果有key
						if (EXPECTED(p->key)) {
							// 释放key
							zend_string_release_ex(p->key, 0);
						}
					}
				} while (++p != end);
			}
		}
	// 如果没有在用的元素，但是初始化过
	} else if (EXPECTED(HT_FLAGS(ht) & HASH_FLAG_UNINITIALIZED)) {
		// 跳到释放内存。 跳过销毁数据，连状态也不更新吗？
		goto free_ht;
	}
	// 如果没有在用的元素也没有初始化过，直接到这里 ？
	// 一致性状态：标记成已销毁
	SET_INCONSISTENT(HT_DESTROYED);
	// 销毁数据 arData
	// 从p1->arData找到哈希表数据块开头位置。p1:哈希表。
	efree(HT_GET_DATA_ADDR(ht));
free_ht:
	// 删除此哈希表的迭代器
	zend_hash_iterators_remove(ht);
	// 释放哈希表
	FREE_HASHTABLE(ht);
}

// ing3, 销毁每个元素并 重置哈希表
ZEND_API void ZEND_FASTCALL zend_hash_clean(HashTable *ht)
{
	// 检查一致性，调试用
	IS_CONSISTENT(ht);
	// 断言，哈希表的引用次数为1 或 ALLOW_COW_VIOLATION ，调试用
	HT_ASSERT_RC1(ht);

	// 有已使用元素才需要清理
	if (ht->nNumUsed) {
		// 顺序数组
		if (HT_IS_PACKED(ht)) {
			// 顺序元素开头
			zval *zv = ht->arPacked;
			// 顺序元素结尾
			zval *end = zv + ht->nNumUsed;
			// 如果有销毁器
			if (ht->pDestructor) {
				// 是否只包含静态key: 是索引数组 或者 所有key都是保留字
				if (HT_HAS_STATIC_KEYS_ONLY(ht)) {
					// 如果每个元素都用上了
					if (HT_IS_WITHOUT_HOLES(ht)) {
						// 遍历每个元素
						do {
							// 销毁每个元素
							ht->pDestructor(zv);
						} while (++zv != end);
					// 如果有空元素
					} else {
						// 遍历每个元素
						do {
							// 销毁已使用元素
							if (EXPECTED(Z_TYPE_P(zv) != IS_UNDEF)) {
								ht->pDestructor(zv);
							}
						} while (++zv != end);
					}
				}
			}
		// 哈希表
		} else {
			Bucket *p = ht->arData;
			Bucket *end = p + ht->nNumUsed;
			// 如果有销毁器 
			if (ht->pDestructor) {
				// 是否只包含静态key: 是索引数组 或者 所有key都是保留字
				if (HT_HAS_STATIC_KEYS_ONLY(ht)) {
					// 没有空元素，少一个判断
					if (HT_IS_WITHOUT_HOLES(ht)) {
						// 遍历每个元素
						do {
							// 销毁每个元素
							ht->pDestructor(&p->val);
						} while (++p != end);
					} else {
						// 遍历每个元素
						do {
							// 销毁每个有效元素
							if (EXPECTED(Z_TYPE(p->val) != IS_UNDEF)) {
								ht->pDestructor(&p->val);
							}
						} while (++p != end);
					}
				// 普通哈希表，如果没有空元素，少一个判断
				// 调用销毁器清理所有元素
				} else if (HT_IS_WITHOUT_HOLES(ht)) {
					// 遍历每一个元素
					do {
						// 销毁元素
						ht->pDestructor(&p->val);
						// 如果有字串key
						if (EXPECTED(p->key)) {
							// 释放key
							zend_string_release(p->key);
						}
					} while (++p != end);
				// 有空元素
				} else {
					// 遍历每一个元素
					do {
						// 销毁每个有效元素
						if (EXPECTED(Z_TYPE(p->val) != IS_UNDEF)) {
							ht->pDestructor(&p->val);
							// 如果有字串key
							if (EXPECTED(p->key)) {
								// 释放key
								zend_string_release(p->key);
							}
						}
					} while (++p != end);
				}
			// 哈希表 没有销毁器
			} else {
				// 是否只包含静态key: 是索引数组 或者 所有key都是保留字
				if (!HT_HAS_STATIC_KEYS_ONLY(ht)) {
					// 遍历销毁每一个key
					do {
						// 如果有字串key
						if (EXPECTED(p->key)) {
							// 释放字串key
							zend_string_release(p->key);
						}
					} while (++p != end);
				}
			}
			// 重置 (p1)->arHash 哈希->索引列表。 所有元素 都设置成 -1。p1:哈希表
			HT_HASH_RESET(ht);
		}
	}
	// 使用元素为0
	ht->nNumUsed = 0;
	// 元素数为0
	ht->nNumOfElements = 0;
	// 空闲元素数
	ht->nNextFreeElement = ZEND_LONG_MIN;
	// 内部指针
	ht->nInternalPointer = 0;
}

// 全局一次调用 zend_execute.c
// ing3, 清空符号表, 销毁所有元素
ZEND_API void ZEND_FASTCALL zend_symtable_clean(HashTable *ht)
{
	Bucket *p, *end;

	// 检查一致性，调试用
	IS_CONSISTENT(ht);
	// 断言，哈希表的引用次数为1 或 ALLOW_COW_VIOLATION ，调试用
	HT_ASSERT_RC1(ht);

	// 使用元素数 不是0
	if (ht->nNumUsed) {
		// 不可以是顺序数组
		ZEND_ASSERT(!HT_IS_PACKED(ht));
		// 顺序元素开头
		p = ht->arData;
		// 顺序元素结尾
		end = p + ht->nNumUsed;
		// 如果 只包含静态key: 是索引数组 或者 所有key都是保留字
		if (HT_HAS_STATIC_KEYS_ONLY(ht)) {
			// 遍历
			do {
				// 依次销毁 每个元素
				i_zval_ptr_dtor(&p->val);
			} while (++p != end);
		// 如果 没有空元素
		} else if (HT_IS_WITHOUT_HOLES(ht)) {
			// 遍历
			do {
				// 依次销毁 每个元素
				i_zval_ptr_dtor(&p->val);
				// 如果key有效
				if (EXPECTED(p->key)) {
					// 要释放key字串
					zend_string_release(p->key);
				}
			} while (++p != end);
		// 其他情况
		} else {
			// 遍历
			do {
				// 只清理类型为 未定义的元素
				if (EXPECTED(Z_TYPE(p->val) != IS_UNDEF)) {
					// 销毁原素
					i_zval_ptr_dtor(&p->val);
					// 如果有key
					if (EXPECTED(p->key)) {
						// 释放字串
						zend_string_release(p->key);
					}
				}
			} while (++p != end);
		}
		// 重置 (p1)->arHash 哈希->索引列表。 所有元素 都设置成 -1。p1:哈希表
		HT_HASH_RESET(ht);
	}
	// 使用数0
	ht->nNumUsed = 0;
	// 元素数0
	ht->nNumOfElements = 0;
	// 下一个可用，指向无效位置
	ht->nNextFreeElement = ZEND_LONG_MIN;
	// 内部指针 0
	ht->nInternalPointer = 0;
}

// ing3, 优雅地销毁哈希表，循环逐个销毁元素，全局无调用
ZEND_API void ZEND_FASTCALL zend_hash_graceful_destroy(HashTable *ht)
{
	uint32_t idx;

	// 检查一致性，调试用
	IS_CONSISTENT(ht);
	// 断言，哈希表的引用次数为1 或 ALLOW_COW_VIOLATION ，调试用
	HT_ASSERT_RC1(ht);

	// 如果是顺序数组
	if (HT_IS_PACKED(ht)) {
		// zval列表开头
		zval *zv = ht->arPacked;
		// 遍历zval列表
		for (idx = 0; idx < ht->nNumUsed; idx++, zv++) {
			// 如果 碰到未定义， 跳过
			if (UNEXPECTED(Z_TYPE_P(zv) == IS_UNDEF)) continue;
			
			// 顺序数组删除指定 序号 的元素。删除元素主要靠销毁器(pDestructor)完成。
			// 64位系统中 index 不用转换
			_zend_hash_packed_del_val(ht, HT_IDX_TO_HASH(idx), zv);
		}
	} else {
		// 开头 bucket
		Bucket *p = ht->arData;

		// 遍历所有 bucket
		for (idx = 0; idx < ht->nNumUsed; idx++, p++) {
			// 如果是未使用 ，跳过
			if (UNEXPECTED(Z_TYPE(p->val) == IS_UNDEF)) continue;
			// 哈希表中删除指定 序号的 bucket。p1:哈希表，p2:序号，p3:bucket指针
			// 64位系统中 index 不用转换
			_zend_hash_del_el(ht, HT_IDX_TO_HASH(idx), p);
		}
	}
	// 如果数组不是未初始化。说明有数据块
	if (!(HT_FLAGS(ht) & HASH_FLAG_UNINITIALIZED)) {
		// 释放哈希表数据块
		// 从p1->arData找到哈希表数据块开头位置。p1:哈希表。
		pefree(HT_GET_DATA_ADDR(ht), GC_FLAGS(ht) & IS_ARRAY_PERSISTENT);
	}
	// 一致性状态，已销毁
	SET_INCONSISTENT(HT_DESTROYED);
}

// ing3, 优雅地逆序销毁。蛮多次调用
ZEND_API void ZEND_FASTCALL zend_hash_graceful_reverse_destroy(HashTable *ht)
{
	uint32_t idx;

	// 检查一致性，调试用
	IS_CONSISTENT(ht);
	// 断言，哈希表的引用次数为1 或 ALLOW_COW_VIOLATION ，调试用
	HT_ASSERT_RC1(ht);

	idx = ht->nNumUsed;
	// 顺序数组
	if (HT_IS_PACKED(ht)) {
		// 指向内存末尾
		zval *zv = ht->arPacked + ht->nNumUsed;
		// 逆序遍历
		while (idx > 0) {
			// 序号自减
			idx--;
			// 指针左移
			zv--;
			// 跳过无效元素
			if (UNEXPECTED(Z_TYPE_P(zv) == IS_UNDEF)) continue;
			// 顺序数组删除指定 序号 的元素。删除元素主要靠销毁器(pDestructor)完成。
			// 64位系统中 index 不用转换
			_zend_hash_packed_del_val(ht, HT_IDX_TO_HASH(idx), zv);
		}
	// 哈希表
	} else {
		// 指向内存末尾
		Bucket *p = ht->arData + ht->nNumUsed;
		// 逆序遍历
		while (idx > 0) {
			// 指针和序号左移
			idx--;
			// Bucket 指针左移
			p--;
			// 跳过无效元素
			if (UNEXPECTED(Z_TYPE(p->val) == IS_UNDEF)) continue;
			// 哈希表中删除指定 序号的 bucket。p1:哈希表，p2:序号，p3:bucket指针
			// 64位系统中 index 不用转换
			_zend_hash_del_el(ht, HT_IDX_TO_HASH(idx), p);
		}
	}

	// 如果哈希表没有初始化过
	if (!(HT_FLAGS(ht) & HASH_FLAG_UNINITIALIZED)) {
		// 释放哈希表数据块
		// 从p1->arData找到哈希表数据块开头位置。p1:哈希表。
		pefree(HT_GET_DATA_ADDR(ht), GC_FLAGS(ht) & IS_ARRAY_PERSISTENT);
	}
	// 一致性状态，已销毁
	SET_INCONSISTENT(HT_DESTROYED);
}

/* This is used to recurse elements and selectively delete certain entries
 * from a hashtable. apply_func() receives the data and decides if the entry
 * should be deleted or recursion should be stopped. The following three
 * return codes are possible:
 * ZEND_HASH_APPLY_KEEP   - continue
 * ZEND_HASH_APPLY_STOP   - stop iteration
 * ZEND_HASH_APPLY_REMOVE - delete the element, combinable with the former
 */
// ing3, 对每个元素应用指定方法，无参数。与下面的方法类似
ZEND_API void ZEND_FASTCALL zend_hash_apply(HashTable *ht, apply_func_t apply_func)
{
	uint32_t idx;
	int result;

	// 检查一致性，调试用
	IS_CONSISTENT(ht);
	if (HT_IS_PACKED(ht)) {
		for (idx = 0; idx < ht->nNumUsed; idx++) {
			zval *zv = ht->arPacked + idx;

			if (UNEXPECTED(Z_TYPE_P(zv) == IS_UNDEF)) continue;
			result = apply_func(zv);

			if (result & ZEND_HASH_APPLY_REMOVE) {
				// 断言，哈希表的引用次数为1 或 ALLOW_COW_VIOLATION ，调试用
				HT_ASSERT_RC1(ht);
				// 顺序数组删除指定 序号 的元素。删除元素主要靠销毁器(pDestructor)完成。
				// 64位系统中 index 不用转换
				_zend_hash_packed_del_val(ht, HT_IDX_TO_HASH(idx), zv);
			}
			if (result & ZEND_HASH_APPLY_STOP) {
				break;
			}
		}
	} else {
		for (idx = 0; idx < ht->nNumUsed; idx++) {
			Bucket *p = ht->arData + idx;

			if (UNEXPECTED(Z_TYPE(p->val) == IS_UNDEF)) continue;
			result = apply_func(&p->val);

			if (result & ZEND_HASH_APPLY_REMOVE) {
				// 断言，哈希表的引用次数为1 或 ALLOW_COW_VIOLATION ，调试用
				HT_ASSERT_RC1(ht);
				// 哈希表中删除指定 序号的 bucket。p1:哈希表，p2:序号，p3:bucket指针
				// 64位系统中 index 不用转换
				_zend_hash_del_el(ht, HT_IDX_TO_HASH(idx), p);
			}
			if (result & ZEND_HASH_APPLY_STOP) {
				break;
			}
		}
	}
}

// ing3, 对每个元素应用指定方法，按返回结果的要求决定是否删除元素，附加参数。与下面的方法类似
ZEND_API void ZEND_FASTCALL zend_hash_apply_with_argument(HashTable *ht, apply_func_arg_t apply_func, void *argument)
{
	uint32_t idx;
	int result;

	// 检查一致性，调试用
	IS_CONSISTENT(ht);
	// 顺序数组
	if (HT_IS_PACKED(ht)) {
		// 正序遍历，跳过无效，按要求删除或停止
		for (idx = 0; idx < ht->nNumUsed; idx++) {
			zval *zv = ht->arPacked + idx;
			if (UNEXPECTED(Z_TYPE_P(zv) == IS_UNDEF)) continue;
			result = apply_func(zv, argument);

			if (result & ZEND_HASH_APPLY_REMOVE) {
				// 断言，哈希表的引用次数为1 或 ALLOW_COW_VIOLATION ，调试用
				HT_ASSERT_RC1(ht);
				// 顺序数组删除指定 序号 的元素。删除元素主要靠销毁器(pDestructor)完成。
				// 64位系统中 index 不用转换
				_zend_hash_packed_del_val(ht, HT_IDX_TO_HASH(idx), zv);
			}
			if (result & ZEND_HASH_APPLY_STOP) {
				break;
			}
		}
	// 哈希表
	} else {
		for (idx = 0; idx < ht->nNumUsed; idx++) {
			Bucket *p = ht->arData + idx;
			if (UNEXPECTED(Z_TYPE(p->val) == IS_UNDEF)) continue;
			result = apply_func(&p->val, argument);

			if (result & ZEND_HASH_APPLY_REMOVE) {
				// 断言，哈希表的引用次数为1 或 ALLOW_COW_VIOLATION ，调试用
				HT_ASSERT_RC1(ht);
				// 哈希表中删除指定 序号的 bucket。p1:哈希表，p2:序号，p3:bucket指针
				// 64位系统中 index 不用转换
				_zend_hash_del_el(ht, HT_IDX_TO_HASH(idx), p);
			}
			if (result & ZEND_HASH_APPLY_STOP) {
				break;
			}
		}
	}
}

// ing3, 对每个元素应用指定方法，按返回结果的要求决定是否删除元素，附加参数列表
ZEND_API void zend_hash_apply_with_arguments(HashTable *ht, apply_func_args_t apply_func, int num_args, ...)
{
	uint32_t idx;
	va_list args;
	// 这个key 是为了传给指定函数的时候格式统一
	zend_hash_key hash_key;
	int result;

	// 检查一致性，调试用
	IS_CONSISTENT(ht);
	// 顺序数组
	if (HT_IS_PACKED(ht)) {
		// 正序遍历
		for (idx = 0; idx < ht->nNumUsed; idx++) {
			zval *zv = ht->arPacked + idx;
			// 跳过无效
			if (UNEXPECTED(Z_TYPE_P(zv) == IS_UNDEF)) continue;
			// 获取参数列表，为什么写在循环里呢
			va_start(args, num_args);
			// 函数参数，key
			hash_key.h = idx;
			hash_key.key = NULL;
			// 调用用户函数
			result = apply_func(zv, num_args, args, &hash_key);
			// 如果返回了“删除”命令
			if (result & ZEND_HASH_APPLY_REMOVE) {
				// 断言，哈希表的引用次数为1 或 ALLOW_COW_VIOLATION ，调试用
				HT_ASSERT_RC1(ht);
				// 顺序数组删除指定 序号 的元素。删除元素主要靠销毁器(pDestructor)完成。
				// 64位系统中 index 不用转换
				_zend_hash_packed_del_val(ht, HT_IDX_TO_HASH(idx), zv);
			}
			// 如果返回了 “停止”命令
			if (result & ZEND_HASH_APPLY_STOP) {
				va_end(args);
				break;
			}
			// ？ 停止获取参数
			va_end(args);
		}
	// 哈希表
	} else {
		// 正序遍历
		for (idx = 0; idx < ht->nNumUsed; idx++) {
			Bucket *p = ht->arData + idx;
			// 跳过无效
			if (UNEXPECTED(Z_TYPE(p->val) == IS_UNDEF)) continue;
			// 获取参数列表，为什么写在循环里呢
			va_start(args, num_args);
			// 函数参数，key
			hash_key.h = p->h;
			hash_key.key = p->key;
			// 调用用户函数
			result = apply_func(&p->val, num_args, args, &hash_key);
			// 如果返回了“删除”命令
			if (result & ZEND_HASH_APPLY_REMOVE) {
				// 断言，哈希表的引用次数为1 或 ALLOW_COW_VIOLATION ，调试用
				HT_ASSERT_RC1(ht);
				// 哈希表中删除指定 序号的 bucket。p1:哈希表，p2:序号，p3:bucket指针
				// 64位系统中 index 不用转换
				_zend_hash_del_el(ht, HT_IDX_TO_HASH(idx), p);
			}
			// 如果返回了 “停止”命令
			if (result & ZEND_HASH_APPLY_STOP) {
				va_end(args);
				break;
			}
			va_end(args);
		}
	}
}

// ing3, 倒序遍历，逐个调用 apply_func
ZEND_API void ZEND_FASTCALL zend_hash_reverse_apply(HashTable *ht, apply_func_t apply_func)
{
	uint32_t idx;
	int result;

	// 检查一致性，调试用
	IS_CONSISTENT(ht);

	idx = ht->nNumUsed;
	// 顺序数组
	if (HT_IS_PACKED(ht)) {
		zval *zv;
		// 倒着遍历
		while (idx > 0) {
			idx--;
			zv = ht->arPacked + idx;
			// 跳过无效
			if (UNEXPECTED(Z_TYPE_P(zv) == IS_UNDEF)) continue;
			// 调用传入的方法
			result = apply_func(zv);
	
			// 如果返回了“删除”命令
			if (result & ZEND_HASH_APPLY_REMOVE) {
				// 断言，哈希表的引用次数为1 或 ALLOW_COW_VIOLATION ，调试用
				HT_ASSERT_RC1(ht);
				// 顺序数组删除指定 序号 的元素。删除元素主要靠销毁器(pDestructor)完成。
				// 64位系统中 index 不用转换
				_zend_hash_packed_del_val(ht, HT_IDX_TO_HASH(idx), zv);
			} 
			// 如果返回了 “停止”命令
			if (result & ZEND_HASH_APPLY_STOP) {
				// 
				break;
			}
		}
	// 哈希表
	} else {
		Bucket *p;
		
		// 倒着遍历
		while (idx > 0) {
			idx--;
			p = ht->arData + idx;
			// 跳过无效
			if (UNEXPECTED(Z_TYPE(p->val) == IS_UNDEF)) continue;
			// 调用传入的方法
			result = apply_func(&p->val);
			// 如果返回了“删除”命令
			if (result & ZEND_HASH_APPLY_REMOVE) {
				// 断言，哈希表的引用次数为1 或 ALLOW_COW_VIOLATION ，调试用
				HT_ASSERT_RC1(ht);
				// 哈希表中删除指定 序号的 bucket。p1:哈希表，p2:序号，p3:bucket指针
				// 64位系统中 index 不用转换
				_zend_hash_del_el(ht, HT_IDX_TO_HASH(idx), p);
			}
			// 如果返回了 “停止”命令
			if (result & ZEND_HASH_APPLY_STOP) {
				// 
				break;
			}
		}
	}
}

// ing3, 复制哈希表到目标哈希表
ZEND_API void ZEND_FASTCALL zend_hash_copy(HashTable *target, HashTable *source, copy_ctor_func_t pCopyConstructor)
{
	uint32_t idx;
	zval *new_entry, *data;
	// 两个哈希表的一致性检查，测试用
	IS_CONSISTENT(source);
	IS_CONSISTENT(target);
	// 断言，哈希表的引用次数为1 或 ALLOW_COW_VIOLATION ，调试用
	HT_ASSERT_RC1(target);
	// 顺序数组
	if (HT_IS_PACKED(source)) {
		// 遍历所有元素
		for (idx = 0; idx < source->nNumUsed; idx++) {
			// 取得元素
			zval *zv = source->arPacked + idx;
			// 跳过无效
			if (UNEXPECTED(Z_TYPE_P(zv) == IS_UNDEF)) continue;
			// 找到旧原素都更新。找不到旧原素都添加，数组会转成哈希表。p1:哈希表，p2:哈希值，p3:元素
			new_entry = zend_hash_index_update(target, idx, zv);
			// 
			if (pCopyConstructor) {
				pCopyConstructor(new_entry);
			}
		}
		return;
	}
	// 哈希表,遍历
	for (idx = 0; idx < source->nNumUsed; idx++) {
		
		Bucket *p = source->arData + idx;
		// 跳过无效
		if (UNEXPECTED(Z_TYPE(p->val) == IS_UNDEF)) continue;

		// 间接元素 指向 未定义快照？
		/* INDIRECT element may point to UNDEF-ined slots */
		data = &p->val;
		// 如果是间接引用
		if (Z_TYPE_P(data) == IS_INDIRECT) {
			// 追踪到目标对象
			data = Z_INDIRECT_P(data);
			// 跳过无效
			if (UNEXPECTED(Z_TYPE_P(data) == IS_UNDEF)) {
				continue;
			}
		}
		// 如果有 key
		if (p->key) {
			// （强转哈希表）不管找不到到原素，都更新指定位置的元素。p1:哈希表，p2:key，p3:值
			new_entry = zend_hash_update(target, p->key, data);
		// 没有key只有哈希值
		} else {
			// 找到旧原素都更新。找不到旧原素都添加，数组会转成哈希表。p1:哈希表，p2:哈希值，p3:元素
			new_entry = zend_hash_index_update(target, p->h, data);
		}
		// 
		if (pCopyConstructor) {
			pCopyConstructor(new_entry);
		}
	}
}

// ing2, 把哈希表中的指定元素 复制给另一个哈希表 ，【data指向的对象不可以是源哈希表】
// 传入两个哈希表，两个变量指针，bool packed 是否顺序数组, bool with_holes 是否包含空元素
static zend_always_inline bool zend_array_dup_value(HashTable *source, HashTable *target, zval *data, zval *dest, bool packed, bool with_holes)
{
	// 如果有空元素
	if (with_holes) {
		// 如果不是顺序数组 并且 源数据是 间接引用
		if (!packed && Z_TYPE_INFO_P(data) == IS_INDIRECT) {
			data = Z_INDIRECT_P(data); // 追踪引用对象
		}
		// 如果源数据无效
		if (UNEXPECTED(Z_TYPE_INFO_P(data) == IS_UNDEF)) {
			return 0; // 返回失败
		}
	} else if (!packed) { // 如果是哈希表
		if (Z_TYPE_INFO_P(data) == IS_INDIRECT) { // 如果是间接引用
			data = Z_INDIRECT_P(data);
			// 如果引用对象无效
			if (UNEXPECTED(Z_TYPE_INFO_P(data) == IS_UNDEF)) {
				return 0; // 返回失败
			}
		}
	}

	do {
		if (Z_OPT_REFCOUNTED_P(data)) { // 元素可引用计数
			// 如果类型是引用 并且引用数量是1  并且（引用对象类型不是数组 或 引用对象不是源哈希表）
			if (Z_ISREF_P(data) && Z_REFCOUNT_P(data) == 1 &&
				(Z_TYPE_P(Z_REFVAL_P(data)) != IS_ARRAY ||
			      Z_ARRVAL_P(Z_REFVAL_P(data)) != source)) {
				data = Z_REFVAL_P(data); // 追踪引用目标
				// 如果引用对象不可计数
				if (!Z_OPT_REFCOUNTED_P(data)) {
					break; // 如果类型是引用 并且引用数量是1  并且  引用对象不是源哈希表 并且 引用目标不可计数
				}
			}
			// 如果data不是引用类型 或 引用次数不是1 或 （引用对象类型是数组 并且是 源哈希表）
			Z_ADDREF_P(data); // 添加引用计数。 break 可以跳过这个。
		}
	} while (0); // while(0) 的目的是，为了用 break
	ZVAL_COPY_VALUE(dest, data); // 复制元素zval

	return 1;
}

// ing3, 复制单个哈希表元素，只有内部调用 ：zend_array_dup_elements。packed 参数固定是 0
static zend_always_inline bool zend_array_dup_element(HashTable *source, HashTable *target, uint32_t idx, Bucket *p, Bucket *q, bool packed, bool static_keys, bool with_holes)
{
	// 如果复制元素失败，返回false
	if (!zend_array_dup_value(source, target, &p->val, &q->val, packed, with_holes)) {
		return 0;
	}

	// 如果不是顺序数组
	if (!packed) {
		uint32_t nIndex;
		// 复制键名和哈希值
		q->h = p->h;
		q->key = p->key;
		// 如果key存在，并且不是 内置变量数组
		if (!static_keys && q->key) {
			// 增加引用次数
			zend_string_addref(q->key);
		}
		// 更新 arHash
		nIndex = q->h | target->nTableMask;
		Z_NEXT(q->val) = HT_HASH(target, nIndex);
		// 更新指定 哈希->索引 表中的索引号
		// 找到 (p1->arHash) 哈希->索引列表 中的指定索引号。p1:哈希表，p2:顺序号（和mask做过或运算, 负数）
		// 64位系统中 index 不用转换
		HT_HASH(target, nIndex) = HT_IDX_TO_HASH(idx);
	}
	return 1;
}

// ing3, 复制顺序数组数据，防交叉
static zend_always_inline void zend_array_dup_packed_elements(HashTable *source, HashTable *target, bool with_holes)
{
	zval *p = source->arPacked;
	zval *q = target->arPacked;
	zval *end = p + source->nNumUsed;

	// 依次遍历顺序数组
	do {
		// 如果复制失败，并且可以包含无效元素。 1：顺序数组标记
		if (!zend_array_dup_value(source, target, p, q, 1, with_holes)) {
			if (with_holes) {
				// 目标元素设置成 IS_UNDEF
				ZVAL_UNDEF(q);
			}
		}
		p++; q++;
	} while (p != end);
}

// ing3, 复制 哈希表 数据，static_keys 是否内置数组, with_holes 是否包含空元素
static zend_always_inline uint32_t zend_array_dup_elements(HashTable *source, HashTable *target, bool static_keys, bool with_holes)
{
	// 遍历两个哈希表
	uint32_t idx = 0;
	Bucket *p = source->arData;
	Bucket *q = target->arData;
	Bucket *end = p + source->nNumUsed;

	// 遍历两个哈希表
	do {
		// 如果赋值单个元素失败。
		if (!zend_array_dup_element(source, target, idx, p, q, 0, static_keys, with_holes)) {
			// 只要有一个元素复制失败，就走里面的逻辑了，因为两个哈希表的顺序号对不上了。
			// 为什么不只用里面的逻辑呢？防止比对内部指针消耗资源？这优化可够狠的！
			uint32_t target_idx = idx;

			idx++; p++;
			// 遍历源数据从 p到最后的元素
			while (p != end) {
				// 依次复制给目标哈希表，如果复制成功
				if (zend_array_dup_element(source, target, target_idx, p, q, 0, static_keys, with_holes)) {
					// 如果碰到 源数据内部指针指向的元素
					if (source->nInternalPointer == idx) {
						// 目标哈希表内部指针也跟着指向这个元素
						target->nInternalPointer = target_idx;
					}
					// 复制成功后移动目标哈希表的序号和当前元素指针
					target_idx++; q++;
				}
				// 无论是否成功，移动源哈希表的序号和当前元素指针
				idx++; p++;
			}
			// 返回目标哈希表当前序号
			return target_idx;
		}
		// 如果没出错，序号和两个哈希表的元素指针都向前移
		idx++; p++; q++;
	} while (p != end);
	// 返回当前顺序号
	return idx;
}

// ing3(大量调用）, 把哈希表所有元素导到新哈希表里。新旧哈希表不会交叉，新哈希表不会有指针指向 source
ZEND_API HashTable* ZEND_FASTCALL zend_array_dup(HashTable *source)
{
	uint32_t idx;
	HashTable *target;

	// 检查一致性，调试用
	IS_CONSISTENT(source);

	ALLOC_HASHTABLE(target);
	GC_SET_REFCOUNT(target, 1);
	GC_TYPE_INFO(target) = GC_ARRAY;

	target->pDestructor = ZVAL_PTR_DTOR;

	// 情况1： 如果源哈希表里没有元素
	if (source->nNumOfElements == 0) {
		// 设置新哈希表默认属性
		HT_FLAGS(target) = HASH_FLAG_UNINITIALIZED;
		target->nTableMask = HT_MIN_MASK;
		target->nNumUsed = 0;
		target->nNumOfElements = 0;
		target->nNextFreeElement = source->nNextFreeElement;
		target->nInternalPointer = 0;
		target->nTableSize = HT_MIN_SIZE;
		// 设置 p1->arData 到 Bucket 列表开头。p1:哈希表，p2: 内存块开头位置
		HT_SET_DATA_ADDR(target, &uninitialized_bucket);
	// 情况2： 源哈希表是内置元素表
	} else if (GC_FLAGS(source) & IS_ARRAY_IMMUTABLE) {
		// 复制属性到新哈希表
		// 过滤掉无效标记
		HT_FLAGS(target) = HT_FLAGS(source) & HASH_FLAG_MASK;
		target->nTableMask = source->nTableMask;
		target->nNumUsed = source->nNumUsed;
		target->nNumOfElements = source->nNumOfElements;
		target->nNextFreeElement = source->nNextFreeElement;
		target->nTableSize = source->nTableSize;
		// 分配内存，更新内部指针，复制数据
		// 顺序数组
		if (HT_IS_PACKED(source)) {
			// 设置 p1->arData 到 Bucket 列表开头。p1:哈希表，p2: 内存块开头位置
			// 计算顺序数组的 数据大小: 顺序数据大小 + 哈希->索引列表大小。p1:哈希表
			HT_SET_DATA_ADDR(target, emalloc(HT_PACKED_SIZE(target)));
			target->nInternalPointer = source->nInternalPointer;
			// 复制数据
			// 从p1->arData找到哈希表数据块开头位置。p1:哈希表。
			// 顺序数组的已使用部分大小，哈希->索引列表 部分算已使用。p1:哈希表
			memcpy(HT_GET_DATA_ADDR(target), HT_GET_DATA_ADDR(source), HT_PACKED_USED_SIZE(source));
		} else {
			// 设置 p1->arData 到 Bucket 列表开头。p1:哈希表，p2: 内存块开头位置
			// 计算 哈希表 数据大小：Bucket 列表大小 + 哈希->索引 列表大小。p1:哈希表
			HT_SET_DATA_ADDR(target, emalloc(HT_SIZE(target)));
			target->nInternalPointer = source->nInternalPointer;
			// 从p1->arData找到哈希表数据块开头位置。p1:哈希表。
			memcpy(HT_GET_DATA_ADDR(target), HT_GET_DATA_ADDR(source), HT_USED_SIZE(source));
		}
	// 情况3： 源哈希表是普通顺序数组 -> zend_array_dup_packed_elements
	} else if (HT_IS_PACKED(source)) {
		// 复制属性到新哈希表
		// 过滤掉无效标记
		HT_FLAGS(target) = HT_FLAGS(source) & HASH_FLAG_MASK;
		target->nTableMask = HT_MIN_MASK;
		target->nNumUsed = source->nNumUsed;
		target->nNumOfElements = source->nNumOfElements;
		target->nNextFreeElement = source->nNextFreeElement;
		target->nTableSize = source->nTableSize;
		// 设置 p1->arData 到 Bucket 列表开头。p1:哈希表，p2: 内存块开头位置
		// 计算顺序数组的 数据大小: zval列表 大小 + 哈希->索引列表大小。p1:元素数，p2:mask
		HT_SET_DATA_ADDR(target, emalloc(HT_PACKED_SIZE_EX(target->nTableSize, HT_MIN_MASK)));
		// 内部指针，如果源数组内部指针比元素数大，内部指针写 0
		target->nInternalPointer =
			(source->nInternalPointer < source->nNumUsed) ?
				source->nInternalPointer : 0;

		// 顺序数组的 (p1)->arHash 哈希->索引列表 里有2个元素，都设置成 -1
		HT_HASH_RESET_PACKED(target);
		
		// 复制顺序数组数据
		if (HT_IS_WITHOUT_HOLES(target)) {
			zend_array_dup_packed_elements(source, target, 0);
		} else {
			zend_array_dup_packed_elements(source, target, 1);
		}
	// 情况4： 源哈希表是普通 哈希表 -> zend_array_dup_elements
	} else {
		// 复制属性到新哈希表
		// 过滤掉无效标记
		HT_FLAGS(target) = HT_FLAGS(source) & HASH_FLAG_MASK;
		target->nTableMask = source->nTableMask;
		target->nNextFreeElement = source->nNextFreeElement;
		// 内部指针，如果源数组内部指针比元素数大，内部指针写 0
		target->nInternalPointer =
			(source->nInternalPointer < source->nNumUsed) ?
				source->nInternalPointer : 0;

		target->nTableSize = source->nTableSize;
		// 设置 p1->arData 到 Bucket 列表开头。p1:哈希表，p2: 内存块开头位置
		// 计算 哈希表 数据大小：Bucket 列表大小 + 哈希->索引 列表大小。p1:哈希表
		HT_SET_DATA_ADDR(target, emalloc(HT_SIZE(target)));
		// 重置 (p1)->arHash 哈希->索引列表。 所有元素 都设置成 -1。p1:哈希表
		HT_HASH_RESET(target);
		
		// 是否只包含静态key: 是索引数组 或者 所有key都是保留字
		if (HT_HAS_STATIC_KEYS_ONLY(target)) {
			if (HT_IS_WITHOUT_HOLES(source)) {
				idx = zend_array_dup_elements(source, target, 1, 0);
			} else {
				idx = zend_array_dup_elements(source, target, 1, 1);
			}
		// 普通哈希表
		} else {
			if (HT_IS_WITHOUT_HOLES(source)) {
				idx = zend_array_dup_elements(source, target, 0, 0);
			} else {
				idx = zend_array_dup_elements(source, target, 0, 1);
			}
		}
		target->nNumUsed = idx;
		target->nNumOfElements = idx;
	}
	return target;
}

// ing4, 把哈希表的元素添加到新建顺序数组中。p1:源哈希表
ZEND_API HashTable* zend_array_to_list(HashTable *source)
{
	// 创建新数组
	HashTable *result = _zend_new_array(zend_hash_num_elements(source));
	// 初始化成顺序数组
	zend_hash_real_init_packed(result);

	// 准备填充哈希表
	ZEND_HASH_FILL_PACKED(result) {
		zval *entry;
		// 遍历原哈希表
		ZEND_HASH_FOREACH_VAL(source, entry) {
			// 如果元素是引用类型 并且引用次数是1
			if (UNEXPECTED(Z_ISREF_P(entry) && Z_REFCOUNT_P(entry) == 1)) {
				// 解引用
				entry = Z_REFVAL_P(entry);
			}
			// 元素增加引用次数
			Z_TRY_ADDREF_P(entry);
			// 填充到数组
			ZEND_HASH_FILL_ADD(entry);
		// 遍历原表结束
		} ZEND_HASH_FOREACH_END();
	// 填充结束
	} ZEND_HASH_FILL_END();
	
	// 返回 新哈希表指针
	return result;
}

// ing3, 合并哈希表， pCopyConstructor 对更新过的每个元素执行构造方法，overwrite 覆盖
ZEND_API void ZEND_FASTCALL zend_hash_merge(HashTable *target, HashTable *source, copy_ctor_func_t pCopyConstructor, bool overwrite)
{
	uint32_t idx;
	Bucket *p;
	zval *t, *s;
	// 检查一致性，调试用
	IS_CONSISTENT(source);
	IS_CONSISTENT(target);
	// 断言，哈希表的引用次数为1 或 ALLOW_COW_VIOLATION ，调试用
	HT_ASSERT_RC1(target);

	// 可以覆盖
	if (overwrite) {
		// 源数组是顺序数组
		if (HT_IS_PACKED(source)) {
			// 遍历所有可用元素
			for (idx = 0; idx < source->nNumUsed; idx++) {
				// 元素
				s = source->arPacked + idx;
				// 跳过未定义
				if (UNEXPECTED(Z_TYPE_P(s) == IS_UNDEF)) {
					continue;
				}
				// 找到旧原素都更新。找不到旧原素都添加，数组会转成哈希表。p1:哈希表，p2:哈希值，p3:元素
				t = zend_hash_index_update(target, idx, s);
				// 如果有构造方法，对更新过的元素执行
				if (pCopyConstructor) {
					pCopyConstructor(t);
				}
			}
			return;
		}
		// 源数组是哈希表
		for (idx = 0; idx < source->nNumUsed; idx++) {
			// 键值对
			p = source->arData + idx;
			// 值
			s = &p->val;
			// 如果是间接引用，追踪到被引用对象
			if (UNEXPECTED(Z_TYPE_P(s) == IS_INDIRECT)) {
				s = Z_INDIRECT_P(s);
			}
			// 跳过未定义
			if (UNEXPECTED(Z_TYPE_P(s) == IS_UNDEF)) {
				continue;
			}
			// 如果有key
			if (p->key) {
				// 按key 更新目标数组（更新或新建）
				//（强转哈希表）增/改/查 元素, p1:哈希表，p2:key名，p3:值，p4:flag（操作类型）
				t = _zend_hash_add_or_update_i(target, p->key, s, HASH_UPDATE | HASH_UPDATE_INDIRECT);
				// 
				if (pCopyConstructor) {
					pCopyConstructor(t);
				}
			// 没有key
			} else {
				// 找到旧原素都更新。找不到旧原素都添加，数组会转成哈希表。p1:哈希表，p2:哈希值，p3:元素
				t = zend_hash_index_update(target, p->h, s);
				// 可以放后面去
				if (pCopyConstructor) {
					pCopyConstructor(t);
				}
			}
		}
	// 不可以覆盖
	} else {
		// 顺序数组
		if (HT_IS_PACKED(source)) {
			// 遍历源数组
			for (idx = 0; idx < source->nNumUsed; idx++) {
				// 每个元素
				s = source->arPacked + idx;
				// 跳过未定义
				if (UNEXPECTED(Z_TYPE_P(s) == IS_UNDEF)) {
					continue;
				}
				// 调用添加方法
				t = zend_hash_index_add(target, idx, s);
				// 添加成功后，调用构造方法
				if (t && pCopyConstructor) {
					pCopyConstructor(t);
				}
			}
			return;
		}
		// 哈希表
		for (idx = 0; idx < source->nNumUsed; idx++) {
			// 键值对
			p = source->arData + idx;
			// 值
			s = &p->val;
			// 间接引用，追踪引用对象
			if (UNEXPECTED(Z_TYPE_P(s) == IS_INDIRECT)) {
				s = Z_INDIRECT_P(s);
			}
			// 跳过未定义
			if (UNEXPECTED(Z_TYPE_P(s) == IS_UNDEF)) {
				continue;
			}
			// 有key，按key 更新。
			if (p->key) {
				// 插入元素，有旧原素 检查间接引用目标，如果有效，中断并返回null
				t = _zend_hash_add_or_update_i(target, p->key, s, HASH_ADD | HASH_UPDATE_INDIRECT);
				//
				if (t && pCopyConstructor) {
					pCopyConstructor(t);
				}
			// 没有key，按哈希值
			} else {
				t = zend_hash_index_add(target, p->h, s);
				// 
				if (t && pCopyConstructor) {
					pCopyConstructor(t);
				}
			}
		}
	}
}

// ing3, 调用自定义方法检查是否可以 合并。
// p1:哈希表，p2:源数据，p3:哈希值，p4:key（zend_string），p5:参数指针，p6:自定义检验方法
static bool ZEND_FASTCALL zend_hash_replace_checker_wrapper(HashTable *target, zval *source_data, zend_ulong h, zend_string *key, void *pParam, merge_checker_func_t merge_checker_func)
{
	// 临时 hash_key 实例
	zend_hash_key hash_key;
	// 哈希值
	hash_key.h = h;
	// key
	hash_key.key = key;
	// 调用自定义方法检查是否可以 merge  p1:哈希表，p2:源数据，p3: hash_key实例，p4:参数指针
	return merge_checker_func(target, source_data, &hash_key, pParam);
}

// ing3, 合并哈希表。p1:目标哈希表，p2:源哈希表，p3:复制元素构造方法，p4:合并检验函数，p5:合并检验用的 参数
ZEND_API void ZEND_FASTCALL zend_hash_merge_ex(HashTable *target, HashTable *source, copy_ctor_func_t pCopyConstructor, merge_checker_func_t pMergeSource, void *pParam)
{
	uint32_t idx;
	Bucket *p;
	zval *t;

	// 检查一致性，调试用
	IS_CONSISTENT(source);
	IS_CONSISTENT(target);
	// 断言，哈希表的引用次数为1 或 ALLOW_COW_VIOLATION ，调试用
	HT_ASSERT_RC1(target);
	// 不可以是顺序数组
	ZEND_ASSERT(!HT_IS_PACKED(source));
	// 遍历所有键值对
	for (idx = 0; idx < source->nNumUsed; idx++) {
		// 找到 bucket
		p = source->arData + idx;
		// 跳过未定义
		if (UNEXPECTED(Z_TYPE(p->val) == IS_UNDEF)) continue;
		// 调用自定义方法检查是否可以 合并。
		// p1:哈希表，p2:源数据，p3:哈希值，p4:key（zend_string），p5:参数指针，p6:自定义检验方法
		// 如果可以合并
		if (zend_hash_replace_checker_wrapper(target, &p->val, p->h, p->key, pParam, pMergeSource)) {
			// （强转哈希表）不管找不到到原素，都更新指定位置的元素。p1:哈希表，p2:key，p3:值
			t = zend_hash_update(target, p->key, &p->val);
			// 如果有复制 构造方法
			if (pCopyConstructor) {
				// 对新 zval 调用此方法
				pCopyConstructor(t);
			}
		}
	}
}

// 找到返回哈希表数据，找不到返回null
/* Returns the hash table data if found and NULL if not. */
// ing4, 使用 key 获取哈希表的 bucket值。p1:哈希表，p2:key(zend_string)
ZEND_API zval* ZEND_FASTCALL zend_hash_find(const HashTable *ht, zend_string *key)
{
	Bucket *p;
	
	// 检查一致性，调试用
	IS_CONSISTENT(ht);

	(void)zend_string_hash_val(key);
	p = zend_hash_find_bucket(ht, key);
	return p ? &p->val : NULL;
}

// ing4, 使用 有哈希值的键(zend_string) 获取哈希表的 值
ZEND_API zval* ZEND_FASTCALL zend_hash_find_known_hash(const HashTable *ht, const zend_string *key)
{
	Bucket *p;

	// 检查一致性，调试用
	IS_CONSISTENT(ht);

	p = zend_hash_find_bucket(ht, key);
	return p ? &p->val : NULL;
}

// ing4, 使用 键(string) 获取哈希表的 值。p1:哈希表，p2:key(char *),p3:长度
ZEND_API zval* ZEND_FASTCALL zend_hash_str_find(const HashTable *ht, const char *str, size_t len)
{
	zend_ulong h;
	Bucket *p;

	// 检查一致性，调试用
	IS_CONSISTENT(ht);

	// 计算数组key的哈希值
	h = zend_inline_hash_func(str, len);
	// 查找键值对
	p = zend_hash_str_find_bucket(ht, str, len, h);
	return p ? &p->val : NULL;
}

// ing4, 按哈希值查询，兼容顺序数组。p1:哈希表，p2:哈希值
// 大量引用
ZEND_API zval* ZEND_FASTCALL zend_hash_index_find(const HashTable *ht, zend_ulong h)
{
	Bucket *p;

	// 检查一致性，调试用
	IS_CONSISTENT(ht);
	// 如果是顺序数组
	if (HT_IS_PACKED(ht)) {
		// 顺序号有效
		if (h < ht->nNumUsed) {
			// 元素位置
			zval *zv = ht->arPacked + h;
			// 如果有效，返回元素
			if (Z_TYPE_P(zv) != IS_UNDEF) {
				return zv;
			}
		}
		return NULL;
	}
	// 查找哈希表，返回结果。使用 哈希值(zend_ulong) 获取哈希表里【没有key，只有哈希值的键值对】
	p = zend_hash_index_find_bucket(ht, h);
	// 如果元素有效，返回元素里的变量
	return p ? &p->val : NULL;
}

// ing3, 使用 哈希值(zend_ulong) 获取哈希表里【没有key，只有哈希值的键值对】里的变量
ZEND_API zval* ZEND_FASTCALL _zend_hash_index_find(const HashTable *ht, zend_ulong h)
{
	Bucket *p;

	// 检查一致性，调试用
	IS_CONSISTENT(ht);
	ZEND_ASSERT(!HT_IS_PACKED(ht));
	// 使用 哈希值(zend_ulong) 获取哈希表里【没有key，只有哈希值的键值对】
	p = zend_hash_index_find_bucket(ht, h);
	return p ? &p->val : NULL;
}

// ing3, 获取第一个有效元素的位置. 外部大量调用。p1:哈希表，p2:位置（返回）
ZEND_API void ZEND_FASTCALL zend_hash_internal_pointer_reset_ex(HashTable *ht, HashPosition *pos)
{
	// 检查一致性，调试用
	IS_CONSISTENT(ht);
	// 调试用
	HT_ASSERT(ht, &ht->nInternalPointer != pos || GC_REFCOUNT(ht) == 1);
	// 返回右面第一个有效位置
	*pos = _zend_hash_get_valid_pos(ht, 0);
}


// 通过记录列表结尾，可以极大地优化这个函数
/* This function will be extremely optimized by remembering
 * the end of the list
 */
// ing3, 查找最后一个有效元素的位置。p1:哈希表，p2:位置（返回）
ZEND_API void ZEND_FASTCALL zend_hash_internal_pointer_end_ex(HashTable *ht, HashPosition *pos)
{
	uint32_t idx;

	// 检查一致性，调试用
	IS_CONSISTENT(ht);
	// 调试用
	HT_ASSERT(ht, &ht->nInternalPointer != pos || GC_REFCOUNT(ht) == 1);

	// 使用数，也是最后一个位置
	idx = ht->nNumUsed;
	// 如果是顺序数组
	if (HT_IS_PACKED(ht)) {
		// 倒着遍历
		while (idx > 0) {
			// 序号 -1
			idx--;
			// 如果元素有效
			if (Z_TYPE(ht->arPacked[idx]) != IS_UNDEF) {
				// 返回编号
				*pos = idx;
				// 结束
				return;
			}
		}
	// 哈希表
	} else {
		// 倒着遍历
		while (idx > 0) {
			// 序号 -1
			idx--;
			// 如果元素有效
			if (Z_TYPE(ht->arData[idx].val) != IS_UNDEF) {
				// 返回编号
				*pos = idx;
				// 结束
				return;
			}
		}
	}
	// 如果找不到，返回最后一个元素位置
	*pos = ht->nNumUsed;
}

// ing3, 向右查找下一个有效位置。p1:哈希表，p2:位置, 也在这里返回找到的编号
ZEND_API zend_result ZEND_FASTCALL zend_hash_move_forward_ex(HashTable *ht, HashPosition *pos)
{
	uint32_t idx;

	// 检查一致性，调试用
	IS_CONSISTENT(ht);
	// 调试用
	HT_ASSERT(ht, &ht->nInternalPointer != pos || GC_REFCOUNT(ht) == 1);
	// pos下一个有效位置
	idx = _zend_hash_get_valid_pos(ht, *pos);
	// 如果位置已用
	if (idx < ht->nNumUsed) {
		// 如果是顺序数组
		if (HT_IS_PACKED(ht)) {
			// 向右查找
			while (1) {
				idx++;
				// 如果到头了，返回最后一个位置
				if (idx >= ht->nNumUsed) {
					*pos = ht->nNumUsed; // 这个位置应该是无效的？
					return SUCCESS;
				}
				// 如果元素有效，返回元素序号
				if (Z_TYPE(ht->arPacked[idx]) != IS_UNDEF) {
					*pos = idx;
					return SUCCESS;
				}
			}
		// 哈希表
		} else {
			// 向右查找
			while (1) {
				idx++;
				// 如果到头了，返回最后一个位置
				if (idx >= ht->nNumUsed) {
					*pos = ht->nNumUsed;
					return SUCCESS;
				}
				// 如果元素有效，返回元素序号
				if (Z_TYPE(ht->arData[idx].val) != IS_UNDEF) {
					*pos = idx;
					return SUCCESS;
				}
			}
		}
	// 否则报错
	} else {
		return FAILURE;
	}
}

// ing4, 找到 p1 左边第一个有效元素的位置。p1:哈希表，p2:给出位置, 也在这里返回找到的编号
ZEND_API zend_result ZEND_FASTCALL zend_hash_move_backwards_ex(HashTable *ht, HashPosition *pos)
{
	uint32_t idx = *pos;

	// 检查一致性，调试用
	IS_CONSISTENT(ht);
	// 内部指针未指到 目标位置 或 引用数是1
	HT_ASSERT(ht, &ht->nInternalPointer != pos || GC_REFCOUNT(ht) == 1);

	// 目标序号在已使用范围内
	if (idx < ht->nNumUsed) {
		// 顺序数组
		if (HT_IS_PACKED(ht)) {
			// 倒序遍历
			while (idx > 0) {
				// 指针向左移动
				idx--;
				// 如果元素有效
				if (Z_TYPE(ht->arPacked[idx]) != IS_UNDEF) {
					// 使用这个序号
					*pos = idx;
					// 返回成功
					return SUCCESS;
				}
			}
		// 哈希表
		} else {
			// 倒序遍历
			while (idx > 0) {
				// 指针向左移动
				idx--;
				// 如果元素有效
				if (Z_TYPE(ht->arData[idx].val) != IS_UNDEF) {
					// 使用这个序号
					*pos = idx;
					// 返回成功
					return SUCCESS;
				}
			}
		}
		// 如果没找到，指针放在最后一个元素上
		*pos = ht->nNumUsed;
		// 返回成功
		return SUCCESS;
	// 目标序号 超出已使用范围
	} else {
		// 返回失败
		return FAILURE;
	}
}

// 这个函数应该保证二进制安全
/* This function should be made binary safe  */
// ing3, 获取右侧第一个有效键，根据类型，把键返回在 str_index 或 num_index 变量里。
// p1:哈希表，p2:key（返回值），p3:哈希值（返回值），p4:位置
ZEND_API int ZEND_FASTCALL zend_hash_get_current_key_ex(const HashTable *ht, zend_string **str_index, zend_ulong *num_index, const HashPosition *pos)
{
	uint32_t idx;
	Bucket *p;

	// 检查一致性，调试用	
	IS_CONSISTENT(ht);
	// 右侧第一个有效位置
	idx = _zend_hash_get_valid_pos(ht, *pos);
	// 如果有效
	if (idx < ht->nNumUsed) {
		// 顺序数组
		if (HT_IS_PACKED(ht)) {
			*num_index = idx;
			return HASH_KEY_IS_LONG;
		}
		// 哈希表, 取得键值对
		p = ht->arData + idx;
		if (p->key) {
			*str_index = p->key;
			return HASH_KEY_IS_STRING;
		} else {
			*num_index = p->h;
			return HASH_KEY_IS_LONG;
		}
	}
	return HASH_KEY_NON_EXISTENT;
}

// ing3, 获取右侧第一个有效键。p1:哈希表，p2:返回key，p3:位置
ZEND_API void ZEND_FASTCALL zend_hash_get_current_key_zval_ex(const HashTable *ht, zval *key, const HashPosition *pos)
{
	uint32_t idx;
	Bucket *p;

	// 检查一致性，调试用
	IS_CONSISTENT(ht);
	// 下一个有效位置
	idx = _zend_hash_get_valid_pos(ht, *pos);
	// 位置无效，key设置成Null
	if (idx >= ht->nNumUsed) {
		ZVAL_NULL(key);
	// 位置有效
	} else {
		// 顺序数组
		if (HT_IS_PACKED(ht)) {
			// 返回顺序号
			ZVAL_LONG(key, idx);
			return;
		}
		// 哈希表，取得键值对
		p = ht->arData + idx;
		// 如果有key , 返回string
		if (p->key) {
			ZVAL_STR_COPY(key, p->key);
		// 没有 key 返回 哈希值
		} else {
			ZVAL_LONG(key, p->h);
		}
	}
}

// ing4, 获取位置右侧第一个元素的 key 的类型（数字/字串/不存在，只有哈希值算数字）。p1:哈希表，p2:位置
ZEND_API int ZEND_FASTCALL zend_hash_get_current_key_type_ex(HashTable *ht, HashPosition *pos)
{
	uint32_t idx;
	Bucket *p;

	// 检查一致性，调试用
	IS_CONSISTENT(ht);
	// 下一个有效位置
	idx = _zend_hash_get_valid_pos(ht, *pos);
	// 位置有效
	if (idx < ht->nNumUsed) {
		// 如果是顺序数组，返回 HASH_KEY_IS_LONG
		if (HT_IS_PACKED(ht)) {
			return HASH_KEY_IS_LONG;
		}
		//哈希表， 找到键值对
		p = ht->arData + idx;
		// 如果有键，返回 HASH_KEY_IS_STRING
		if (p->key) {
			return HASH_KEY_IS_STRING;
		// 否则还是 HASH_KEY_IS_LONG
		} else {
			return HASH_KEY_IS_LONG;
		}
	}
	// key 不存在
	return HASH_KEY_NON_EXISTENT;
}

// ing3, 获取右侧第一个有效元素的值。p1:哈希表，p2:位置
ZEND_API zval* ZEND_FASTCALL zend_hash_get_current_data_ex(HashTable *ht, HashPosition *pos)
{
	uint32_t idx;
	Bucket *p;

	// 检查一致性，调试用
	IS_CONSISTENT(ht);
	// 右侧第一个有效位置
	idx = _zend_hash_get_valid_pos(ht, *pos);
	// 如果位置有效
	if (idx < ht->nNumUsed) {
		// 如果是顺序数组
		if (HT_IS_PACKED(ht)) {
			// 返回第 idx+1 个元素
			return &ht->arPacked[idx];
		}
		// 哈希表，第 idx+1 个键值对的 值
		p = ht->arData + idx;
		return &p->val;
	} else {
		return NULL;
	}
}

// ing4, 交换两个键值对的 key，值和哈希
ZEND_API void zend_hash_bucket_swap(Bucket *p, Bucket *q)
{
	zval val;
	zend_ulong h;
	zend_string *key;

	val = p->val;
	h = p->h;
	key = p->key;

	p->val = q->val;
	p->h = q->h;
	p->key = q->key;

	q->val = val;
	q->h = h;
	q->key = key;
}

// ing4, 交换两个键值对的值，没有交换key和哈希值
ZEND_API void zend_hash_bucket_renum_swap(Bucket *p, Bucket *q)
{
	zval val;

	val = p->val;
	p->val = q->val;
	q->val = val;
}

// ing4, 交换两个Bucket元素的（值和哈希值），没有交换Key
ZEND_API void zend_hash_bucket_packed_swap(Bucket *p, Bucket *q)
{
	zval val;
	zend_ulong h;

	val = p->val;
	h = p->h;

	p->val = q->val;
	p->h = q->h;

	q->val = val;
	q->h = h;
}

// 100行
// ing3, 转成哈希表，记录原始序号，使用给定的方法比较，排序。如果需要重排序号，转成顺序数组，否则转成哈希表。
// p1:哈希表，p2:排序函数，p3:比较函数，p4:是否需要重新排序
ZEND_API void ZEND_FASTCALL zend_hash_sort_ex(HashTable *ht, sort_func_t sort, bucket_compare_func_t compar, bool renumber)
{
	Bucket *p;
	uint32_t i, j;
	// 检查一致性，调试用
	IS_CONSISTENT(ht);
	// 断言，哈希表的引用次数为1 或 ALLOW_COW_VIOLATION ，调试用
	HT_ASSERT_RC1(ht);

	// 有renumber 时，一个元素也要排序
	// 元素数量<=1 并且 ！（重新排序 并且 元素数>0 ）
	if (!(ht->nNumOfElements>1) && !(renumber && ht->nNumOfElements>0)) {
		// 1个元素或空。不需要重新排序
		/* Doesn't require sorting */
		return;
	}

	// 步骤1：转成哈希表
	// 顺序数组转成哈希表。都转成哈希表了为什么下面还要判断顺序数组？
	if (HT_IS_PACKED(ht)) {		
		zend_hash_packed_to_hash(ht); // TODO: ???
	}

	// 步骤2：跳过空元素并记录原始序号	
	// 如果没有空元素
	if (HT_IS_WITHOUT_HOLES(ht)) {
		// 把原顺序号存放在 变量的 extra 里， 允许稳定排序
		/* Store original order of elements in extra space to allow stable sorting. */
		// 遍历使用过的元素
		for (i = 0; i < ht->nNumUsed; i++) {
			// 记录原编号
			Z_EXTRA(ht->arData[i].val) = i;
		}
	// 有空元素
	} else {
		// 删除空元素并保存原顺序号，经过这个操作 arHash 就无效了
		/* Remove holes and store original order. */
		// 遍历所有的元素
		for (j = 0, i = 0; j < ht->nNumUsed; j++) {
			// 找到元素
			p = ht->arData + j;
			// 跳过未定义
			if (UNEXPECTED(Z_TYPE(p->val) == IS_UNDEF)) continue;
			// 如果出现过空元素
			if (i != j) {
				// 当前元素向右移，跳过空元素
				ht->arData[i] = *p;
			}
			// 记录原顺序号（空元素不计）
			Z_EXTRA(ht->arData[i].val) = i;
			// 下一个 bucket
			i++;
		}
		// 有效元素数量
		ht->nNumUsed = i;
	}

	// 步骤2：
	
	// 如果不是顺序数组
	if (!(HT_FLAGS(ht) & HASH_FLAG_PACKED)) {
		// 打乱哈希 colisions 链，用 Z_EXTRA() 覆盖  Z_NEXT() （就是上面那波操作）
		// 同时重置 哈希头表，避免 递归结构上可能出现的不一致访问。（下面这一行）
		/* We broke the hash colisions chains overriding Z_NEXT() by Z_EXTRA().
		 * Reset the hash headers table as well to avoid possilbe inconsistent
		 * access on recursive data structures.
	     *
	     * See Zend/tests/bug63882_2.phpt
		 */
		// 重置 (p1)->arHash 哈希->索引列表。 所有元素 都设置成 -1。p1:哈希表
		HT_HASH_RESET(ht);
	}
	// 排序 ht->arData
	// 使用 compar 函数作比较，
	// 交换函数的选择：renumber（重排序号） ? zend_hash_bucket_renum_swap (交换两个键值对的值，没有交换key和哈希值) 
		// ：顺序数组 ? zend_hash_bucket_packed_swap （交换两个Bucket元素的（值和哈希值），没有交换Key）
		//	: zend_hash_bucket_swap(交换两个键值对的 key，值和哈希)
	sort((void *)ht->arData, ht->nNumUsed, sizeof(Bucket), (compare_func_t) compar,
			(swap_func_t)(renumber? zend_hash_bucket_renum_swap :
				// 顺序数组和哈希表，用不同的交换方法
				(HT_IS_PACKED(ht) ? zend_hash_bucket_packed_swap : zend_hash_bucket_swap)));

	// 内部指针复位
	ht->nInternalPointer = 0;

	// 如果要求 renumber 重排序号（强转成纯顺序数组）
	if (renumber) {
		// 遍历所有 有效元素
		for (j = 0; j < i; j++) {
			// 找到元素
			p = ht->arData + j;
			// 哈希值为顺序号
			p->h = j;
			// 如果有key
			if (p->key) {
				// 释放key
				zend_string_release(p->key);
				// 清空key指针
				p->key = NULL;
			}
		}
		// 下一个有效元素
		ht->nNextFreeElement = i;
	}
	// 如果是顺序数组
	if (HT_IS_PACKED(ht)) {
		// 如果不用重新排序
		if (!renumber) {
			// 转成哈希表
			zend_hash_packed_to_hash(ht);
		}
	// 哈希表
	} else {
		// 如果要求 renumber 重新排序
		if (renumber) {
			// 从p1->arData找到哈希表数据块开头位置。p1:哈希表。
			void *new_data, *old_data = HT_GET_DATA_ADDR(ht);
			// 旧buckets开头
			Bucket *old_buckets = ht->arData;
			// 临时变量
			zval *zv;
			// 分配内存创建数据块
			// 计算顺序数组的 数据大小: zval列表 大小 + 哈希->索引列表大小。p1:元素数，p2:mask
			new_data = pemalloc(HT_PACKED_SIZE_EX(ht->nTableSize, HT_MIN_MASK), (GC_FLAGS(ht) & IS_ARRAY_PERSISTENT));
			// 添加顺序数组标记 和 使用静态key 标记
			HT_FLAGS(ht) |= HASH_FLAG_PACKED | HASH_FLAG_STATIC_KEYS;
			// 顺序数组 使用 HT_MIN_MASK
			ht->nTableMask = HT_MIN_MASK;
			// 设置 p1->arData 到 Bucket 列表开头。p1:哈希表，p2: 内存块开头位置
			HT_SET_DATA_ADDR(ht, new_data);
			// 旧buckets开头
			p = old_buckets;
			// 顺序元素开头
			zv = ht->arPacked;
			// 遍历所有元素
			for (i = 0; i < ht->nTableSize; i++) {
				// 把bucket的值复制到 顺序列表里
				ZVAL_COPY_VALUE(zv, &p->val);
				// 下一个顺序 zval
				zv++;
				// 下一个bucket
				p++;
			}
			// 释放旧数据块
			pefree(old_data, GC_FLAGS(ht) & IS_ARRAY_PERSISTENT);
			// 顺序数组的 (p1)->arHash 哈希->索引列表 里有2个元素，都设置成 -1
			HT_HASH_RESET_PACKED(ht);
		// 不重新排序
		} else {
			// 整理元素，填补中间的空位，然后给所有元素更新 编号，哈希->顺序表，next指针。p1: 哈希表
			zend_hash_rehash(ht);
		}
	}
}

// ing3, 比较两个哈希表, 规则里面key的比对，有些出乎意料。impl => implement 实现
static zend_always_inline int zend_hash_compare_impl(HashTable *ht1, HashTable *ht2, compare_func_t compar, bool ordered) {
	// 顺序号
	uint32_t idx1, idx2;
	// 键
	zend_string *key1, *key2;
	// 键的哈希值
	zend_ulong h1, h2;
	// 数据
	zval *pData1, *pData2;;
	int result;

	// 这里吧无效元素也算进来了吗？nNumOfElements, nNumUsed 区别？nNumUsed里有空元素，它只是序号而已。元素是否有效它都不能变的。
	// 【先比较元素数量，数量多的大。】
	if (ht1->nNumOfElements != ht2->nNumOfElements) {
		return ht1->nNumOfElements > ht2->nNumOfElements ? 1 : -1;
	}
	// 遍历所有元素
	for (idx1 = 0, idx2 = 0; idx1 < ht1->nNumUsed; idx1++) {
		// 顺序数组
		if (HT_IS_PACKED(ht1)) {
			pData1 = ht1->arPacked + idx1;
			h1 = idx1;
			key1 = NULL;
		// 哈希表
		} else {
			Bucket *p = ht1->arData + idx1;
			pData1 = &p->val;
			h1 = p->h;
			key1 = p->key;
		}

		// 第一个元素未定义，跳过。这里先找到了数组 1 的有效元素
		if (Z_TYPE_P(pData1) == IS_UNDEF) continue;
		
		// 如果要求按顺序比较
		if (ordered) {
			// 找到数组2 的下一个有效元素
			// 如果第二个数组是顺序数组
			if (HT_IS_PACKED(ht2)) {
				// 找到下一个有效元素
				while (1) {
					// 不可以溢出
					ZEND_ASSERT(idx2 != ht2->nNumUsed);
					// 
					pData2 = ht2->arPacked + idx2;
					h2 = idx2;
					key2 = NULL;
					// 跳过无效元素 
					if (Z_TYPE_P(pData2) != IS_UNDEF) break;
					idx2++;
				}
			// 第二个数组是哈希表
			} else {
				// 找到下一个有效元素
				while (1) {
					Bucket *p;
					// 不可以溢出
					ZEND_ASSERT(idx2 != ht2->nNumUsed);
					p = ht2->arData + idx2;
					//
					pData2 = &p->val;
					h2 = p->h;
					key2 = p->key;
					// 跳过无效元素
					if (Z_TYPE_P(pData2) != IS_UNDEF) break;
					idx2++;
				}
			}
			// 两个都没有key
			if (key1 == NULL && key2 == NULL) { /* numeric indices */
				// 【比较哈希值，返回结果】（这样也行）
				if (h1 != h2) {
					return h1 > h2 ? 1 : -1;
				}
			// 两个都有key
			} else if (key1 != NULL && key2 != NULL) { /* string indices */
				// 【先比较key的长度，返回结果】
				if (ZSTR_LEN(key1) != ZSTR_LEN(key2)) {
					// 再比较key的大小
					return ZSTR_LEN(key1) > ZSTR_LEN(key2) ? 1 : -1;
				}
				// 【比较字串，key1,key2，返回结果】
				result = memcmp(ZSTR_VAL(key1), ZSTR_VAL(key2), ZSTR_LEN(key1));
				if (result != 0) {
					return result;
				}
			// 【一个有key，一个没有。有key的大。】
			} else {
				/* Mixed key types: A string key is considered as larger */
				return key1 != NULL ? 1 : -1;
			}
			// 数组2，可以转到下一个了
			idx2++;
		// 如果没要求按顺序. 那就是按键比对
		} else {
			// 没有key1。查找 哈希值 相同的元素
			if (key1 == NULL) { /* numeric index */
				// 【并且pData2 是null , 算第一个大】，否则比较两个值
				pData2 = zend_hash_index_find(ht2, h1);
				if (pData2 == NULL) {
					return 1;
				}
			// 有key1。查找 key 相同的元素
			} else { /* string index */
				// 【如果 pData2 是null，算第一个大】，否则比较两个值
				pData2 = zend_hash_find(ht2, key1);
				if (pData2 == NULL) {
					return 1;
				}
			}
			// 这个逻辑里不操作 idx2
		}
		
		// 以上都是为了找出两个可比较的元素

		// 追踪间接引用对象
		if (Z_TYPE_P(pData1) == IS_INDIRECT) {
			pData1 = Z_INDIRECT_P(pData1);
		}
		if (Z_TYPE_P(pData2) == IS_INDIRECT) {
			pData2 = Z_INDIRECT_P(pData2);
		}
		// 有效元素大于未定义元素
		// 第一个未定义
		if (Z_TYPE_P(pData1) == IS_UNDEF) {
			// 【第二个不是未定义，返回 -1】
			if (Z_TYPE_P(pData2) != IS_UNDEF) {
				return -1;
			}
		// 【第一个有效，第二个未定义，返回 1】
		} else if (Z_TYPE_P(pData2) == IS_UNDEF) {
			return 1;
		// 两个都有效
		} else {
			// 【比对】。这里才是真正的比对，绝大部分逻辑都是在处理特殊情况。
			result = compar(pData1, pData2);
			// 如果不相等，返回结果
			if (result != 0) {
				return result;
			}
		}
	}
	// 返回相同
	return 0;
}

// ing3, 比较两个哈希表
ZEND_API int zend_hash_compare(HashTable *ht1, HashTable *ht2, compare_func_t compar, bool ordered)
{
	int result;
	// 检查一致性，调试用
	IS_CONSISTENT(ht1);
	IS_CONSISTENT(ht2);
	// 如果两个指针指向同一个哈希表
	if (ht1 == ht2) {
		return 0;
	}

	// 保护一个数组就够了。 第二个如果在第一个里被引用，会导致递归侦测失败
	/* It's enough to protect only one of the arrays.
	 * The second one may be referenced from the first and this may cause
	 * false recursion detection.
	 */
	// 如果有递归标记，报错
	if (UNEXPECTED(GC_IS_RECURSIVE(ht1))) {
		zend_error_noreturn(E_ERROR, "Nesting level too deep - recursive dependency?");
	}

	// 尝试添加递归保护
	GC_TRY_PROTECT_RECURSION(ht1);
	// 比较两个哈希表, 规则里面key的比对，有些出乎意料
	result = zend_hash_compare_impl(ht1, ht2, compar, ordered);
	// 尝试删除递归保护
	GC_TRY_UNPROTECT_RECURSION(ht1);

	return result;
}

// ing4, 获取最大值 | 最小值
ZEND_API zval* ZEND_FASTCALL zend_hash_minmax(const HashTable *ht, compare_func_t compar, uint32_t flag)
{
	uint32_t idx;
	zval *res;

	// 检查一致性，调试用
	IS_CONSISTENT(ht);

	// 空哈希表
	if (ht->nNumOfElements == 0 ) {
		return NULL;
	}

	// 顺序数组
	if (HT_IS_PACKED(ht)) {
		zval *zv;

		idx = 0;
		// 跳过未定义元素，如果所有元素都是未定义，返回null
		while (1) {
			if (idx == ht->nNumUsed) {
				return NULL;
			}
			if (Z_TYPE(ht->arPacked[idx]) != IS_UNDEF) break;
			idx++;
		}
		// 默认第一个有效元素
		res = ht->arPacked + idx;
		// 遍历剩下的元素，取出最大|最小 元素
		for (; idx < ht->nNumUsed; idx++) {
			zv = ht->arPacked + idx;
			// 跳过未定义
			if (UNEXPECTED(Z_TYPE_P(zv) == IS_UNDEF)) continue;
			// 比大小
			if (flag) {
				if (compar(res, zv) < 0) { /* max */
					res = zv;
				}
			} else {
				if (compar(res, zv) > 0) { /* min */
					res = zv;
				}
			}
		}
	// 哈希表, 逻辑与上面类似
	} else {
		Bucket *p;

		idx = 0;
		// 跳过未定义元素，如果所有元素都是未定义，返回null
		while (1) {
			if (idx == ht->nNumUsed) {
				return NULL;
			}
			if (Z_TYPE(ht->arData[idx].val) != IS_UNDEF) break;
			idx++;
		}
		// 默认第一个有效元素
		res = &ht->arData[idx].val;
		// // 遍历剩下的元素，取出最大|最小 元素
		for (; idx < ht->nNumUsed; idx++) {
			p = ht->arData + idx;
			// 跳过未定义
			if (UNEXPECTED(Z_TYPE(p->val) == IS_UNDEF)) continue;
			// 比大小
			if (flag) {
				if (compar(res, &p->val) < 0) { /* max */
					res = &p->val;
				}
			} else {
				if (compar(res, &p->val) > 0) { /* min */
					res = &p->val;
				}
			}
		}
	}
	return res;
}

// ing4, 检查是否是纯数字(0-9)字串，不允许0开头。p1:字串，p2:字串长度，p3:返回转换好的整数 
ZEND_API bool ZEND_FASTCALL _zend_handle_numeric_str_ex(const char *key, size_t length, zend_ulong *idx)
{
	const char *tmp = key;

	const char *end = key + length;
	
	// 允许 负号
	if (*tmp == '-') {
		tmp++;
	}
	// 如果0开头，或超长，或
	if ((*tmp == '0' && length > 1) /* numbers with leading zeros */
	 || (end - tmp > MAX_LENGTH_OF_LONG - 1) /* number too long */
	 // 32位机器 ， 长度满， 开头字符大于2（最大值是 2147483647）。认为溢出
	 || (SIZEOF_ZEND_LONG == 4 &&
	     end - tmp == MAX_LENGTH_OF_LONG - 1 &&
	     *tmp > '2')) { /* overflow */
		return 0;
	}
	// char 的减法。【通过和0的ascii 位置差异，算出数值】
	*idx = (*tmp - '0');
	while (1) {
		// 指针右移
		++tmp;
		// 如果最后一个字符
		if (tmp == end) {
			// 是负号
			if (*key == '-') {
				// 负数可以比 ZEND_LONG_MAX 大1 如果-1 还大于 ZEND_LONG_MAX，就会无法转换
				// 这样更好懂一些 if (*idx > ZEND_LONG_MAX +1 ) 
				if (*idx-1 > ZEND_LONG_MAX) { /* overflow */
					return 0;
				}
				// 负数
				*idx = 0 - *idx;
			// 正数不可以大于最大值
			} else if (*idx > ZEND_LONG_MAX) { /* overflow */
				return 0;
			}
			// 无特殊情况，返回 true
			return 1;
		}
		// 如果字符合法
		if (*tmp <= '9' && *tmp >= '0') {
			// 原数值进一位 + 新字符转换成数值
			*idx = (*idx * 10) + (*tmp - '0');
		// 字符不合法，返回 false
		} else {
			return 0;
		}
	}
}

// symtable 哈希表中包含 整型 和非数字的 字串 key，把它转换到一个 proptable 中（只包含 字串Key）
// 如果 symtable 不需要复制，它的引用数-1
/* Takes a "symtable" hashtable (contains integer and non-numeric string keys)
 * and converts it to a "proptable" (contains only string keys).
 * If the symtable didn't need duplicating, its refcount is incremented.
 */

// 符号表是 一个 只包含 字串Key 的哈希表。表中不可以包含只有哈希值没有字串key的元素。
// ing4, 把哈希表转换为符号表（只包含 字串Key 的哈希表）
ZEND_API HashTable* ZEND_FASTCALL zend_symtable_to_proptable(HashTable *ht)
{
	zend_ulong num_key;
	zend_string *str_key;
	zval *zv;
	// 情况1：
	// 如果是顺序数组
	if (UNEXPECTED(HT_IS_PACKED(ht))) {
		// 转换
		goto convert;
	}

	// 情况2：
	// 遍历哈希表中的key
	ZEND_HASH_MAP_FOREACH_STR_KEY(ht, str_key) {
		// 如果没有key
		if (!str_key) {
			// 转换
			goto convert;
		}
	} ZEND_HASH_FOREACH_END();
	
	// 情况3：
	// 如果是【哈希表】 并且 全都有key

	// 如果不是不可更改
	if (!(GC_FLAGS(ht) & IS_ARRAY_IMMUTABLE)) {
		// 增加引用次数
		GC_ADDREF(ht);
	}

	// 返回哈希表
	return ht;

// 转换
convert:
	{
		// 创建新数组
		HashTable *new_ht = zend_new_array(zend_hash_num_elements(ht));

		// 遍历哈希表
		ZEND_HASH_FOREACH_KEY_VAL(ht, num_key, str_key, zv) {
			// 如果 没有key
			if (!str_key) {
				//  哈希值 转字串
				str_key = zend_long_to_str(num_key);
				// 新字串减少引用次数
				zend_string_delref(str_key);
			}
			// 为了break
			do {
				// 可计数类型
				if (Z_OPT_REFCOUNTED_P(zv)) {
					// 引用类型 并且 引用数是1
					if (Z_ISREF_P(zv) && Z_REFCOUNT_P(zv) == 1) {
						// 解引用
						zv = Z_REFVAL_P(zv);
						// 目标值 不是可计数类型
						if (!Z_OPT_REFCOUNTED_P(zv)) {
							// 跳出
							break;
						}
					}
					// 可计数，增加引用次数
					Z_ADDREF_P(zv);
				}
			} while (0);
			// （强转哈希表）不管找不到到原素，都更新指定位置的元素。p1:哈希表，p2:key，p3:值
			zend_hash_update(new_ht, str_key, zv);
		// 
		} ZEND_HASH_FOREACH_END();
		// 返回新哈希表
		return new_ht;
	}
}

// 获取一个 属性表 （只包含字串key）然后把它转换成 符号表（包含数字和非数字key）
// 如果属性表不需要复制，增加引用数。
/* Takes a "proptable" hashtable (contains only string keys) and converts it to
 * a "symtable" (contains integer and non-numeric string keys).
 * If the proptable didn't need duplicating, its refcount is incremented.
 */
// ing3, 把属性表（只包含字串key），转成符号表 （包含数字和非数字key）。p1：哈希表，p2:总是返回副本
ZEND_API HashTable* ZEND_FASTCALL zend_proptable_to_symtable(HashTable *ht, bool always_duplicate)
{
	zend_ulong num_key;
	zend_string *str_key;
	zval *zv;
	// 如果不是顺序数组
	if (!HT_IS_PACKED(ht)) {
		// 遍历哈希表
		ZEND_HASH_MAP_FOREACH_STR_KEY(ht, str_key) {
			// `str_key &&` 看似是多余的，因为属性表只有字串key。不幸的是，实际情况并非如此，
			// 因为 最新的 ArrayObject 会在属性表的位置 存储一个符号表（符号表里有非字串键）
			/* The `str_key &&` here might seem redundant: property tables should
			 * only have string keys. Unfortunately, this isn't true, at the very
			 * least because of ArrayObject, which stores a symtable where the
			 * property table should be.
			 */
			// 有字串key 并且 字串是数字字串
			if (str_key && ZEND_HANDLE_NUMERIC(str_key, num_key)) {
				// 转换
				goto convert;
			}
		} ZEND_HASH_FOREACH_END();
	}
	// 如果总是需要复制
	if (always_duplicate) {
		// 返回副本
		return zend_array_dup(ht);
	}
	// 如果ht不是不可转换的
	if (EXPECTED(!(GC_FLAGS(ht) & IS_ARRAY_IMMUTABLE))) {
		// 增加引用数
		GC_ADDREF(ht);
	}
	// 返回ht（可能是副本）
	return ht;

convert:
	{
		// 创建新数组
		HashTable *new_ht = zend_new_array(zend_hash_num_elements(ht));
		// 遍历ht
		ZEND_HASH_MAP_FOREACH_KEY_VAL_IND(ht, num_key, str_key, zv) {
			do {
				// 如果值是可计数的
				if (Z_OPT_REFCOUNTED_P(zv)) {
					// 如果值是引用类型 并且 引用数是1
					if (Z_ISREF_P(zv) && Z_REFCOUNT_P(zv) == 1) {
						// 追踪到引用目标
						zv = Z_REFVAL_P(zv);
						// 如果目标不可计数
						if (!Z_OPT_REFCOUNTED_P(zv)) {
							// 跳过计数
							break;
						}
					}
					// 值增加引用数
					Z_ADDREF_P(zv);
				}
			} while (0);
			// 又是因为 ArrayObject
			/* Again, thank ArrayObject for `!str_key ||`. */
			// 如果没有字串键 或 使用了数字字串键
			if (!str_key || ZEND_HANDLE_NUMERIC(str_key, num_key)) {
				// 找到旧原素都更新。找不到旧原素都添加，数组会转成哈希表。p1:哈希表，p2:哈希值，p3:元素
				zend_hash_index_update(new_ht, num_key, zv);
			// 其他情况
			} else {
				// 按字串key更新元素
				zend_hash_update(new_ht, str_key, zv);
			}
		} ZEND_HASH_FOREACH_END();
		// 返回新表
		return new_ht;
	}
}
