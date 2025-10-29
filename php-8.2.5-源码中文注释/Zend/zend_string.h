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
   | Authors: Dmitry Stogov <dmitry@php.net>                              |
   +----------------------------------------------------------------------+
*/

#ifndef ZEND_STRING_H
#define ZEND_STRING_H

#include "zend.h"

BEGIN_EXTERN_C()

typedef void (*zend_string_copy_storage_func_t)(void);
typedef zend_string *(ZEND_FASTCALL *zend_new_interned_string_func_t)(zend_string *str);
typedef zend_string *(ZEND_FASTCALL *zend_string_init_interned_func_t)(const char *str, size_t size, bool permanent);
typedef zend_string *(ZEND_FASTCALL *zend_string_init_existing_interned_func_t)(const char *str, size_t size, bool permanent);

// ing3, 新建永久保留字，传入 zend_string， 
// 默认 zend_new_interned_string_permanent 有时是 interned_string_request_handler
ZEND_API extern zend_new_interned_string_func_t zend_new_interned_string;

// ing4, 创建永久保留字，p1:字串，p2:长度，p3:是否永久
// 默认 zend_string_init_interned_permanent 有时是 interned_string_init_request_handler
ZEND_API extern zend_string_init_interned_func_t zend_string_init_interned;

// 把一个已经存在的字串初始化成保留字，但不创建新的
/* Init an interned string if it already exists, but do not create a new one if it does not. */

// ing4，查找已存在的永久保留字，如果不存在，创建 普通 zend_string 并返回
// 默认 zend_string_init_existing_interned_permanent 有时是 interned_string_init_existing_request_handler
ZEND_API extern zend_string_init_existing_interned_func_t zend_string_init_existing_interned;

ZEND_API zend_ulong ZEND_FASTCALL zend_string_hash_func(zend_string *str);
ZEND_API zend_ulong ZEND_FASTCALL zend_hash_func(const char *str, size_t len);
ZEND_API zend_string* ZEND_FASTCALL zend_interned_string_find_permanent(zend_string *str);

ZEND_API zend_string *zend_string_concat2(
	const char *str1, size_t str1_len,
	const char *str2, size_t str2_len);
ZEND_API zend_string *zend_string_concat3(
	const char *str1, size_t str1_len,
	const char *str2, size_t str2_len,
	const char *str3, size_t str3_len);

ZEND_API void zend_interned_strings_init(void);
ZEND_API void zend_interned_strings_dtor(void);
ZEND_API void zend_interned_strings_activate(void);
ZEND_API void zend_interned_strings_deactivate(void);
ZEND_API void zend_interned_strings_set_request_storage_handlers(
	zend_new_interned_string_func_t handler,
	zend_string_init_interned_func_t init_handler,
	zend_string_init_existing_interned_func_t init_existing_handler);
ZEND_API void zend_interned_strings_switch_storage(bool request);

ZEND_API extern zend_string  *zend_empty_string;
ZEND_API extern zend_string  *zend_one_char_string[256];
ZEND_API extern zend_string **zend_known_strings;

END_EXTERN_C()

/* Shortcuts */

// clear, 取回 zend_string 的 val 元素，这个是真正的字串数组（不是指针）
#define ZSTR_VAL(zstr)  (zstr)->val
// clear, 取回 zend_string 的 len 元素，长度是计算好的，直接取回就行了
#define ZSTR_LEN(zstr)  (zstr)->len
// clear, 取回 zend_string 的哈希值（整数）
#define ZSTR_H(zstr)    (zstr)->h
// clear, 取回字串哈希值（没有就计算），完全并没有用到，包括 别名Z_STRHASH(),Z_STRHASH_P()都没用
#define ZSTR_HASH(zstr) zend_string_hash_val(zstr)

// 兼容性宏
/* Compatibility macros */

// ing4, 是否是保留字串 ,intern(拘留，软禁), 貌似没有被调用
#define IS_INTERNED(s)	ZSTR_IS_INTERNED(s)
// clear, 返回 NULL
#define STR_EMPTY_ALLOC()	ZSTR_EMPTY_ALLOC()
// 
#define _STR_HEADER_SIZE _ZSTR_HEADER_SIZE
//
#define STR_ALLOCA_ALLOC(str, _len, use_heap) ZSTR_ALLOCA_ALLOC(str, _len, use_heap)
//
#define STR_ALLOCA_INIT(str, s, len, use_heap) ZSTR_ALLOCA_INIT(str, s, len, use_heap)
// ing4, ZSTR_ALLOCA_FREE 别名
#define STR_ALLOCA_FREE(str, use_heap) ZSTR_ALLOCA_FREE(str, use_heap)

/*---*/
// ing3, GC_FLAGS(s) 是否包含 IS_STR_INTERNED 标记
#define ZSTR_IS_INTERNED(s)					(GC_FLAGS(s) & IS_STR_INTERNED)

// ing4, 就是NULL, ZEND_API zend_string *zend_empty_string = NULL;
#define ZSTR_EMPTY_ALLOC() zend_empty_string

// ing4, 获取 256个单字节字符 的 zend_string 实例，zend_one_char_string 是 zend_string 数组 
#define ZSTR_CHAR(c) zend_one_char_string[c]
// ing4, 获取已知字串
#define ZSTR_KNOWN(idx) zend_known_strings[idx]

#define _ZSTR_HEADER_SIZE XtOffsetOf(zend_string, val)

#define _ZSTR_STRUCT_SIZE(len) (_ZSTR_HEADER_SIZE + len + 1)

#define ZSTR_MAX_OVERHEAD (ZEND_MM_ALIGNED_SIZE(_ZSTR_HEADER_SIZE + 1))

// 
#define ZSTR_MAX_LEN (SIZE_MAX - ZSTR_MAX_OVERHEAD)

// ing3, 创建指定长度（自动对齐）的字串
#define ZSTR_ALLOCA_ALLOC(str, _len, use_heap) do { \
	/* 分配内存创建新字串 */ \
	(str) = (zend_string *)do_alloca(ZEND_MM_ALIGNED_SIZE_EX(_ZSTR_STRUCT_SIZE(_len), 8), (use_heap)); \
	/* 引用次数1 */ \
	GC_SET_REFCOUNT(str, 1); \
	/* 类型为可回收字串 */ \
	GC_TYPE_INFO(str) = GC_STRING; \
	/* 哈希值为0 */ \
	ZSTR_H(str) = 0; \
	/* 长度 */ \
	ZSTR_LEN(str) = _len; \
} while (0)

// ing3, 创建指定长度（自动对齐）的字串，并复制给出的字串（带长度）
#define ZSTR_ALLOCA_INIT(str, s, len, use_heap) do { \
	/* 创建指定长度（自动对齐）的字串 */ \
	ZSTR_ALLOCA_ALLOC(str, len, use_heap); \
	/* 把字串s的内容复制过来 */ \
	memcpy(ZSTR_VAL(str), (s), (len)); \
	/* 添加结尾字符 */ \
	ZSTR_VAL(str)[(len)] = '\0'; \
} while (0)

// ing3, 如果 use_heap ，释放 str
// do { if (UNEXPECTED(use_heap)) efree(p); } while (0)
#define ZSTR_ALLOCA_FREE(str, use_heap) free_alloca(str, use_heap)

// ing4, 用字串常量创建 zend_string 实例，只有1次调用
#define ZSTR_INIT_LITERAL(s, persistent) (zend_string_init((s), strlen(s), (persistent)))

/*---*/

// clear, 如果 zend_string 没有哈希值，给它计算哈希值
static zend_always_inline zend_ulong zend_string_hash_val(zend_string *s)
{
	return ZSTR_H(s) ? ZSTR_H(s) : zend_string_hash_func(s);
}

// ing4, 删除 zend_string 的哈希值
static zend_always_inline void zend_string_forget_hash_val(zend_string *s)
{
	ZSTR_H(s) = 0;
	// 删除 IS_STR_VALID_UTF8 标记
	GC_DEL_FLAGS(s, IS_STR_VALID_UTF8);
}

// ing4, 获取 zend_string 的引用次数
static zend_always_inline uint32_t zend_string_refcount(const zend_string *s)
{
	// 不是保留字，获取内存中的引用次数
	if (!ZSTR_IS_INTERNED(s)) {
		// #define GC_REFCOUNT(p) zend_gc_refcount(&(p)->gc)
		return GC_REFCOUNT(s);
	}
	// 保留字永远是1，所以不会自动销毁
	return 1;
}

// ing4, 添加引用次数，保留字串返回1，其他返回添加后的引用次数
static zend_always_inline uint32_t zend_string_addref(zend_string *s)
{
	// 不是保留字，添加引用次数并返回次数
	if (!ZSTR_IS_INTERNED(s)) {
		return GC_ADDREF(s);
	}
	// 保留字永远是 1
	return 1;
}

// ing4, 减少引用次数，保留字返回1
static zend_always_inline uint32_t zend_string_delref(zend_string *s)
{
	if (!ZSTR_IS_INTERNED(s)) {
		// #define GC_DELREF(p) zend_gc_delref(&(p)->gc)
		return GC_DELREF(s);
	}
	return 1;
}

// ing4, 分配内存，创建zend_string
static zend_always_inline zend_string *zend_string_alloc(size_t len, bool persistent)
{
	// 先根据 len 计算合适的内存大小，然后对齐到 mm ，得到需要分配的内存大小
	// ZEND_MM_ALIGNMENT = (size_t)8 = 8 （测试过）
	// 分配内存，长度前面要加上头信息. _zend_string.val 是最后一个元素，所以后面的部分都可以存储字符
	zend_string *ret = (zend_string *)pemalloc(ZEND_MM_ALIGNED_SIZE(_ZSTR_STRUCT_SIZE(len)), persistent);

	// 添加引用次数
	GC_SET_REFCOUNT(ret, 1);
	// 添加 type_info， GC_STRING + 持久标记
	GC_TYPE_INFO(ret) = GC_STRING | ((persistent ? IS_STR_PERSISTENT : 0) << GC_FLAGS_SHIFT);
	// 初始哈希值为0
	ZSTR_H(ret) = 0;
	// 设置
	ZSTR_LEN(ret) = len;
	// 返回指针
	return ret;
}

// ing3, 用 safe_pemalloc 创建的 zend_string。（扩展中大量使用）
static zend_always_inline zend_string *zend_string_safe_alloc(size_t n, size_t m, size_t l, bool persistent)
{
	// safe_pemalloc 涉及汇编语言
	zend_string *ret = (zend_string *)safe_pemalloc(n, m, ZEND_MM_ALIGNED_SIZE(_ZSTR_STRUCT_SIZE(l)), persistent);
	// 引用次数1 
	GC_SET_REFCOUNT(ret, 1);
	// 按需要添加 IS_STR_PERSISTENT 标记
	GC_TYPE_INFO(ret) = GC_STRING | ((persistent ? IS_STR_PERSISTENT : 0) << GC_FLAGS_SHIFT);
	// 哈希值为0
	ZSTR_H(ret) = 0;
	// 设置长度
	ZSTR_LEN(ret) = (n * m) + l;
	// 返回 zend_string 指针
	return ret;
}

// ing4, 创建 zend_string，并初始化
static zend_always_inline zend_string *zend_string_init(const char *str, size_t len, bool persistent)
{
	zend_string *ret = zend_string_alloc(len, persistent);
	// 复制str
	memcpy(ZSTR_VAL(ret), str, len);
	// 添加结束字符（这里Len可以访问任意位置,测试过）
	ZSTR_VAL(ret)[len] = '\0';
	return ret;
}

// ing4, 快速初始化 zend_string
static zend_always_inline zend_string *zend_string_init_fast(const char *str, size_t len)
{
	// 长度一个以上用 zend_string_init
	if (len > 1) {
		return zend_string_init(str, len, 0);
	// 空字串，NULL
	} else if (len == 0) {
		return zend_empty_string;
	// 长度为1
	} else /* if (len == 1) */ {
		return ZSTR_CHAR((zend_uchar) *str);
	}
}

// ing3, 增加引用次数. 这个和 ZVAL_COPY 不同
// 这就是那个所谓的：php变量刚复制 ，还是同一个, 要到更新的时候才 separate
static zend_always_inline zend_string *zend_string_copy(zend_string *s)
{
	// 如果不是保留字
	if (!ZSTR_IS_INTERNED(s)) {
		// 增加引用次数+1
		GC_ADDREF(s);
	}
	return s;
}
// ing4, 把字串dump到新zend_string中，如果是保留字，直接返回
static zend_always_inline zend_string *zend_string_dup(zend_string *s, bool persistent)
{
	// 如果是保留字，直接返回
	if (ZSTR_IS_INTERNED(s)) {
		return s;
	// 不是保留字，返回副本
	} else {
		return zend_string_init(ZSTR_VAL(s), ZSTR_LEN(s), persistent);
	}
}

// ing4, 给字串创建副本，separate指的是版本分离
static zend_always_inline zend_string *zend_string_separate(zend_string *s, bool persistent)
{
	// 如果是保留字或引用次数大于1
	if (ZSTR_IS_INTERNED(s) || GC_REFCOUNT(s) > 1) {
		// 如果不是保留字
		if (!ZSTR_IS_INTERNED(s)) {
			// 原字串引用次数-1
			GC_DELREF(s);
		}
		// 创建并返回副本
		return zend_string_init(ZSTR_VAL(s), ZSTR_LEN(s), persistent);
	}
	// 不是保留字并且引用次数是1，只删除原始实例的哈希值
	zend_string_forget_hash_val(s);
	// 返回原始串（因为只引用了1次，没必要搞两个版本）
	return s;
}

// ing4, 调整内存大小
static zend_always_inline zend_string *zend_string_realloc(zend_string *s, size_t len, bool persistent)
{
	zend_string *ret;

	// 如果不是保留字串
	if (!ZSTR_IS_INTERNED(s)) {
		// 如果引用次数不是1 
		if (EXPECTED(GC_REFCOUNT(s) == 1)) {
			// 直接调整大小
			ret = (zend_string *)perealloc(s, ZEND_MM_ALIGNED_SIZE(_ZSTR_STRUCT_SIZE(len)), persistent);
			// 大小
			ZSTR_LEN(ret) = len;
			// 清空哈希
			zend_string_forget_hash_val(ret);
			// 返回指针
			return ret;
		}
	}
	// 是保留字或引用次数不是1 ， 创建新版本
	ret = zend_string_alloc(len, persistent);
	// 复制内存
	memcpy(ZSTR_VAL(ret), ZSTR_VAL(s), MIN(len, ZSTR_LEN(s)) + 1);
	// 不是保留字，旧版本减少引用次数
	if (!ZSTR_IS_INTERNED(s)) {
		GC_DELREF(s);
	}
	// 返回指针
	return ret;
}

// ing4, zend_string增加尺寸
static zend_always_inline zend_string *zend_string_extend(zend_string *s, size_t len, bool persistent)
{
	zend_string *ret;

	// 新的大小一定要大于原来的大小
	ZEND_ASSERT(len >= ZSTR_LEN(s));
	// 如果不是保留字
	if (!ZSTR_IS_INTERNED(s)) {
		// 如果引用次数是1
		if (EXPECTED(GC_REFCOUNT(s) == 1)) {
			// 调整 zend_string 大小
			ret = (zend_string *)perealloc(s, ZEND_MM_ALIGNED_SIZE(_ZSTR_STRUCT_SIZE(len)), persistent);
			// 设置长度
			ZSTR_LEN(ret) = len;
			// 清空哈希值
			zend_string_forget_hash_val(ret);
			return ret;
		}
	}
	// 如果是保留字，或引用次数不是 1
	// 分配内存创建新字串
	ret = zend_string_alloc(len, persistent);
	// 复制内容，引用次数也一起复制过来了
	memcpy(ZSTR_VAL(ret), ZSTR_VAL(s), ZSTR_LEN(s) + 1);
	// 不是保留字，旧版本减少引用次数
	if (!ZSTR_IS_INTERNED(s)) {
		// 原字串减少引用次数 -1
		GC_DELREF(s);
	}
	// 旧的引用次数不是1，就不能删掉，这里相当于创建了一个新版本
	// 返回新字串
	return ret;
}

// ing4, 截短字串
static zend_always_inline zend_string *zend_string_truncate(zend_string *s, size_t len, bool persistent)
{
	zend_string *ret;
	// 新长度必须比旧的小
	ZEND_ASSERT(len <= ZSTR_LEN(s));
	// 如果不是保留字
	if (!ZSTR_IS_INTERNED(s)) {
		// 如果引用次数是1
		if (EXPECTED(GC_REFCOUNT(s) == 1)) {
			// 调整内存大小
			ret = (zend_string *)perealloc(s, ZEND_MM_ALIGNED_SIZE(_ZSTR_STRUCT_SIZE(len)), persistent);
			// 重新设置长度
			ZSTR_LEN(ret) = len;
			// 清空哈希值
			zend_string_forget_hash_val(ret);
			// 返回指针
			return ret;
		}
	}
	// 如果是保留字，或引用次数是1
	// 创建新 zend_string
	ret = zend_string_alloc(len, persistent);
	// 复制内容
	memcpy(ZSTR_VAL(ret), ZSTR_VAL(s), len + 1);
	// 不是保留字，旧版本减少引用次数
	if (!ZSTR_IS_INTERNED(s)) {
		// 引用次数 -1
		GC_DELREF(s);
	}
	// 旧的引用次数不是1，就不能删掉，这里相当于创建了一个新版本
	// 返回指针
	return ret;
}

// ing4, 安全地重新分配内存
static zend_always_inline zend_string *zend_string_safe_realloc(zend_string *s, size_t n, size_t m, size_t l, bool persistent)
{
	zend_string *ret;
	// 不是保留字
	if (!ZSTR_IS_INTERNED(s)) {
		// 并且引用次数是1
		if (GC_REFCOUNT(s) == 1) {
			// 直接调整大小
			ret = (zend_string *)safe_perealloc(s, n, m, ZEND_MM_ALIGNED_SIZE(_ZSTR_STRUCT_SIZE(l)), persistent);
			// 设置长度，清空哈希，返回指针
			ZSTR_LEN(ret) = (n * m) + l;
			zend_string_forget_hash_val(ret);
			return ret;
		}
	}
	// 保留字或引用次数大于1，创建新版本
	ret = zend_string_safe_alloc(n, m, l, persistent);
	// 复制内存
	memcpy(ZSTR_VAL(ret), ZSTR_VAL(s), MIN((n * m) + l, ZSTR_LEN(s)) + 1);
	// 不是保留字，旧版本减少引用次数
	if (!ZSTR_IS_INTERNED(s)) {
		GC_DELREF(s);
	}
	// 反回指针
	return ret;
}

// ing2, 释放内存，删除string
static zend_always_inline void zend_string_free(zend_string *s)
{
	// 不是保留字串
	if (!ZSTR_IS_INTERNED(s)) {
		// 引用次数最大是1
		ZEND_ASSERT(GC_REFCOUNT(s) <= 1);
		// 删除
		pefree(s, GC_FLAGS(s) & IS_STR_PERSISTENT);
	}
}

// ing4, 释放内存，删除zend_string实例
static zend_always_inline void zend_string_efree(zend_string *s)
{
	// 必须不是保留字
	ZEND_ASSERT(!ZSTR_IS_INTERNED(s));
	// 引用次数最大是1
	ZEND_ASSERT(GC_REFCOUNT(s) <= 1);
	// 不可以有 IS_STR_PERSISTENT 标记
	ZEND_ASSERT(!(GC_FLAGS(s) & IS_STR_PERSISTENT));
	efree(s);
}

// ing4, 释放内存，删除string
static zend_always_inline void zend_string_release(zend_string *s)
{
	// 如果不是保留字串
	if (!ZSTR_IS_INTERNED(s)) {
		// 引用次数减 1 后等于0
		if (GC_DELREF(s) == 0) {
			// 释放字串
			pefree(s, GC_FLAGS(s) & IS_STR_PERSISTENT);
		}
	}
}

// ing4, 释放内存，删除 zend_string
static zend_always_inline void zend_string_release_ex(zend_string *s, bool persistent)
{
	// 只对非保留字进行操作
	if (!ZSTR_IS_INTERNED(s)) {
		// 如果减少引用次数后次数变成0（还是确认是保留字）
		if (GC_DELREF(s) == 0) {
			// 永久
			if (persistent) {
				// 必须带永久标记
				ZEND_ASSERT(GC_FLAGS(s) & IS_STR_PERSISTENT);
				// 释放内存
				free(s);
			// 非永久
			} else {
				// 必须不带永久标记
				ZEND_ASSERT(!(GC_FLAGS(s) & IS_STR_PERSISTENT));
				// 释放内存
				efree(s);
			}
		}
	}
}

// ing4, 比对一个zend_string和一个 字串常量 是否相等。大小写敏感。
static zend_always_inline bool zend_string_equals_cstr(const zend_string *s1, const char *s2, size_t s2_length)
{
	// 长度相等。 !memcmp，C语言里 -1 也是true，只有0是false，测试过。( php 里也一样，测试过）
	return ZSTR_LEN(s1) == s2_length && !memcmp(ZSTR_VAL(s1), s2, s2_length);
}

// 如果是GNUC编译器用i386机型 或 64位用ILP32 编译器？
#if defined(__GNUC__) && (defined(__i386__) || (defined(__x86_64__) && !defined(__ILP32__)))
BEGIN_EXTERN_C()
// GNUC的此方法定义在 zend_string.c中
ZEND_API bool ZEND_FASTCALL zend_string_equal_val(const zend_string *s1, const zend_string *s2);
END_EXTERN_C()
// 其他情况, windows 走这块
#else
// ing4, 比较两个zend_string 是否相同，大小写敏感
static zend_always_inline bool zend_string_equal_val(const zend_string *s1, const zend_string *s2)
{
	// 直接内存比对两块内存（两个 zend_string） ，大小写敏感。（memcmp 测试过）
	return !memcmp(ZSTR_VAL(s1), ZSTR_VAL(s2), ZSTR_LEN(s1));
}
#endif

// ing4, 比较两个zend_string 是否相同，先比较长度，更优化。
static zend_always_inline bool zend_string_equal_content(const zend_string *s1, const zend_string *s2)
{
	return ZSTR_LEN(s1) == ZSTR_LEN(s2) && zend_string_equal_val(s1, s2);
}

// ing4, 比较两个zend_string是否相同
static zend_always_inline bool zend_string_equals(const zend_string *s1, const zend_string *s2)
{
	return s1 == s2 || zend_string_equal_content(s1, s2);
}
// clear, ci : case insensitive
#define zend_string_equals_ci(s1, s2) \
	(ZSTR_LEN(s1) == ZSTR_LEN(s2) && !zend_binary_strcasecmp(ZSTR_VAL(s1), ZSTR_LEN(s1), ZSTR_VAL(s2), ZSTR_LEN(s2)))

// ing4, 比较字串，大小写不相同。这个方法的c参数是const char * 类型，临时声名的字串，所以用sizeof来获取长度。
#define zend_string_equals_literal_ci(str, c) \
	(ZSTR_LEN(str) == sizeof(c) - 1 && !zend_binary_strcasecmp(ZSTR_VAL(str), ZSTR_LEN(str), (c), sizeof(c) - 1))

// 比较比对一个zend_string和一个 字串常量是否相同，大小写敏感
// ing4, zend_string str, char *literal -> zend_string_equals_cstr
#define zend_string_equals_literal(str, literal) \
	// 关键是把 literal 转换成一个指针和一个长度，zend_string是自带长度的
	zend_string_equals_cstr(str, literal, strlen(literal))

// ing4, 校验zend_string是否是 prefix（char *类型） 开头
static zend_always_inline bool zend_string_starts_with_cstr(const zend_string *str, const char *prefix, size_t prefix_length)
{
	// 一定要比prefix长，且比较结果为0
	return ZSTR_LEN(str) >= prefix_length && !memcmp(ZSTR_VAL(str), prefix, prefix_length);
}

// ing4, 校验zend_string是否是 prefix（zend_string *类型） 开头
static zend_always_inline bool zend_string_starts_with(const zend_string *str, const zend_string *prefix)
{
	return zend_string_starts_with_cstr(str, ZSTR_VAL(prefix), ZSTR_LEN(prefix));
}

// ing4, 校验zend_string是否是 prefix（字串常量） 开头
#define zend_string_starts_with_literal(str, prefix) \
	zend_string_starts_with_cstr(str, prefix, strlen(prefix))

/*
 * DJBX33A (Daniel J. Bernstein, Times 33 with Addition)
 *
 * This is Daniel J. Bernstein's popular `times 33' hash function as
 * posted by him years ago on comp.lang.c. It basically uses a function
 * like ``hash(i) = hash(i-1) * 33 + str[i]''. This is one of the best
 * known hash functions for strings. Because it is both computed very
 * fast and distributes very well.
 - 这个方法基于  hash(i) = hash(i-1) * 33 + str[i] 
 - 这是已知的最好的 用字串做哈希表的方法
 - 这哈希方法不光快，而且分散的很好
 
 *
 * The magic of number 33, i.e. why it works better than many other
 * constants, prime or not, has never been adequately explained by
 * anyone. So I try an explanation: if one experimentally tests all
 * multipliers between 1 and 256 (as RSE did now) one detects that even
 * numbers are not usable at all. The remaining 128 odd numbers
 * (except for the number 1) work more or less all equally well. They
 * all distribute in an acceptable way and this way fill a hash table
 * with an average percent of approx. 86%.
 - 为什么 33这个数字有如此魔力，比其他常量运行的好，并没有一个充分的解释。
 - 经过测试，1到256之间的偶数都不能用。 128个奇数，除了1，都可以相同地良好运行。
 - 都可以用可接受的方式来分散哈希，大约可平均覆盖哈希表 86%。
 
 *
 * If one compares the Chi^2 values of the variants, the number 33 not
 * even has the best value. But the number 33 and a few other equally
 * good numbers like 17, 31, 63, 127 and 129 have nevertheless a great
 * advantage to the remaining numbers in the large set of possible
 * multipliers: their multiply operation can be replaced by a faster
 * operation based on just one shift plus either a single addition
 * or subtraction operation. And because a hash function has to both
 * distribute good _and_ has to be very fast to compute, those few
 * numbers should be preferred and seems to be the reason why Daniel J.
 * Bernstein also preferred it.
 - 如果比较 Chi^2 数值，33并不是最好的数。
 - 但是 33 和 17,31,63,127,129 这几个数 大集合乘数中，特别有优势：？？
 - 并且一个哈希函数必须分散良好又非常快，这几个数字表现更好。
 *
 *
 *                  -- Ralf S. Engelschall <rse@engelschall.com>
 */

// ing2, 内置哈希方法。这是把数组的key转换成哈希值的方法。这是个含金量很高的方法！参看上文。
// 64位windows系统中测试过。
static zend_always_inline zend_ulong zend_inline_hash_func(const char *str, size_t len)
{
	zend_ulong hash = Z_UL(5381);
// 如果是 win32 或 i386 或 x86_64 或 aarch64 操作系统
#if defined(_WIN32) || defined(__i386__) || defined(__x86_64__) || defined(__aarch64__)
	/* Version with multiplication works better on modern CPU */
	for (; len >= 8; len -= 8, str += 8) {
# if defined(__aarch64__) && !defined(WORDS_BIGENDIAN)
		/* On some architectures it is beneficial to load 8 bytes at a
		   time and extract each byte with a bit field extract instr. */
		uint64_t chunk;

		memcpy(&chunk, str, sizeof(chunk));
		hash =
			hash                        * 33 * 33 * 33 * 33 +
			((chunk >> (8 * 0)) & 0xff) * 33 * 33 * 33 +
			((chunk >> (8 * 1)) & 0xff) * 33 * 33 +
			((chunk >> (8 * 2)) & 0xff) * 33 +
			((chunk >> (8 * 3)) & 0xff);
		hash =
			hash                        * 33 * 33 * 33 * 33 +
			((chunk >> (8 * 4)) & 0xff) * 33 * 33 * 33 +
			((chunk >> (8 * 5)) & 0xff) * 33 * 33 +
			((chunk >> (8 * 6)) & 0xff) * 33 +
			((chunk >> (8 * 7)) & 0xff);
# else
		hash =
			hash   * Z_L(33 * 33 * 33 * 33) +
			str[0] * Z_L(33 * 33 * 33) +
			str[1] * Z_L(33 * 33) +
			str[2] * Z_L(33) +
			str[3];
		hash =
			hash   * Z_L(33 * 33 * 33 * 33) +
			str[4] * Z_L(33 * 33 * 33) +
			str[5] * Z_L(33 * 33) +
			str[6] * Z_L(33) +
			str[7];
# endif
	}
	if (len >= 4) {
		hash =
			hash   * Z_L(33 * 33 * 33 * 33) +
			str[0] * Z_L(33 * 33 * 33) +
			str[1] * Z_L(33 * 33) +
			str[2] * Z_L(33) +
			str[3];
		len -= 4;
		str += 4;
	}
	if (len >= 2) {
		if (len > 2) {
			hash =
				hash   * Z_L(33 * 33 * 33) +
				str[0] * Z_L(33 * 33) +
				str[1] * Z_L(33) +
				str[2];
		} else {
			hash =
				hash   * Z_L(33 * 33) +
				str[0] * Z_L(33) +
				str[1];
		}
	} else if (len != 0) {
		hash = hash * Z_L(33) + *str;
	}
#else
	/* variant with the hash unrolled eight times */
	for (; len >= 8; len -= 8) {
		hash = ((hash << 5) + hash) + *str++;
		hash = ((hash << 5) + hash) + *str++;
		hash = ((hash << 5) + hash) + *str++;
		hash = ((hash << 5) + hash) + *str++;
		hash = ((hash << 5) + hash) + *str++;
		hash = ((hash << 5) + hash) + *str++;
		hash = ((hash << 5) + hash) + *str++;
		hash = ((hash << 5) + hash) + *str++;
	}
	switch (len) {
		case 7: hash = ((hash << 5) + hash) + *str++; /* fallthrough... */
		case 6: hash = ((hash << 5) + hash) + *str++; /* fallthrough... */
		case 5: hash = ((hash << 5) + hash) + *str++; /* fallthrough... */
		case 4: hash = ((hash << 5) + hash) + *str++; /* fallthrough... */
		case 3: hash = ((hash << 5) + hash) + *str++; /* fallthrough... */
		case 2: hash = ((hash << 5) + hash) + *str++; /* fallthrough... */
		case 1: hash = ((hash << 5) + hash) + *str++; break;
		case 0: break;
EMPTY_SWITCH_DEFAULT_CASE()
	}
#endif

	/* hash 值不可以是0，所以先把高位填充好 */
	/* Hash value can't be zero, so we always set the high bit */
// 64位
#if SIZEOF_ZEND_LONG == 8
	return hash | Z_UL(0x8000000000000000);
// 32位
#elif SIZEOF_ZEND_LONG == 4
	return hash | Z_UL(0x80000000);
// 其他情况，报错
#else
# error "Unknown SIZEOF_ZEND_LONG"
#endif
}
/**
64位windows
static zend_always_inline zend_ulong zend_inline_hash_func(const char *str, size_t len) {
	zend_ulong hash = Z_UL(5381); // 初始哈希值 5381
	// 遍历字串，长度每次-8，指针每次向右跳8Bytes，直到最后不到8Bytes为止
	for (; len >= 8; len -= 8, str += 8) { 
		hash =
			hash   * Z_L(33 * 33 * 33 * 33) + // 哈希值 * pow(33,4)
			str[0] * Z_L(33 * 33 * 33) + // 第1个Byte * pow(33,3)
			str[1] * Z_L(33 * 33) + // 第2个Byte * pow(33,2)
			str[2] * Z_L(33) + // 第3个Byte *33
			str[3]; // 第4个Byte
		hash =
			hash   * Z_L(33 * 33 * 33 * 33) + // 哈希值 * pow(33,4)
			str[4] * Z_L(33 * 33 * 33) + // 第5个Byte * pow(33,3)
			str[5] * Z_L(33 * 33) + // 第6个Byte * pow(33,2)
			str[6] * Z_L(33) + // 第7个Byte * 33
			str[7]; // 第8个Byte
	}	
	if (len >= 4) { // 如果余下的大于等于4Bytes
		hash =
			hash   * Z_L(33 * 33 * 33 * 33) + // 哈希值 * pow(33,4)
			str[0] * Z_L(33 * 33 * 33) + // 第1个Byte * pow(33,3)
			str[1] * Z_L(33 * 33) + // 第2个Byte * pow(33,2)
			str[2] * Z_L(33) + // 第3个Byte *33
			str[3]; // 第4个Byte
		len -= 4; // 长度-4
		str += 4; // 指针向右跳4
	}	
	if (len >= 2) { // 如果余下的大于等于2Bytes
		if (len > 2) { // 如果余下的是3Bytes
			hash =
				hash   * Z_L(33 * 33 * 33) + // 哈希值 * pow(33,3)
				str[0] * Z_L(33 * 33) + // 第1个Byte * pow(33,2)
				str[1] * Z_L(33) + // 第2个Byte * 33
				str[2]; // 第3个Byte				
		} else { // 余下的是2Bytes
			hash =
				hash   * Z_L(33 * 33) + // 哈希值 * pow(33,2)
				str[0] * Z_L(33) + // 第1个Byte * 33
				str[1]; // 第2个Byte
		}	
	} else if (len != 0) { // 如果余下的等于1Byte
		hash = hash * Z_L(33) + *str; // 加最后一个Byte
	}	
	return hash | Z_UL(0x8000000000000000); // 第一个位写成1
}
*/

// ing4, 字串常量，不只是 zend_known_string_id 这里用到，其他报错之类的地方也会用到
#define ZEND_KNOWN_STRINGS(_) \
	//
	_(ZEND_STR_FILE,                   "file") \
	_(ZEND_STR_LINE,                   "line") \
	_(ZEND_STR_FUNCTION,               "function") \
	_(ZEND_STR_CLASS,                  "class") \
	_(ZEND_STR_OBJECT,                 "object") \
	_(ZEND_STR_TYPE,                   "type") \
	_(ZEND_STR_OBJECT_OPERATOR,        "->") \
	_(ZEND_STR_PAAMAYIM_NEKUDOTAYIM,   "::") \
	_(ZEND_STR_ARGS,                   "args") \
	_(ZEND_STR_UNKNOWN,                "unknown") \
	// 首字母大写的
	_(ZEND_STR_UNKNOWN_CAPITALIZED,    "Unknown") \
	_(ZEND_STR_EVAL,                   "eval") \
	_(ZEND_STR_INCLUDE,                "include") \
	_(ZEND_STR_REQUIRE,                "require") \
	_(ZEND_STR_INCLUDE_ONCE,           "include_once") \
	_(ZEND_STR_REQUIRE_ONCE,           "require_once") \
	_(ZEND_STR_SCALAR,                 "scalar") \
	_(ZEND_STR_ERROR_REPORTING,        "error_reporting") \
	_(ZEND_STR_STATIC,                 "static") \
	_(ZEND_STR_THIS,                   "this") \
	_(ZEND_STR_VALUE,                  "value") \
	_(ZEND_STR_KEY,                    "key") \
	_(ZEND_STR_MAGIC_INVOKE,           "__invoke") \
	_(ZEND_STR_PREVIOUS,               "previous") \
	_(ZEND_STR_CODE,                   "code") \
	_(ZEND_STR_MESSAGE,                "message") \
	_(ZEND_STR_SEVERITY,               "severity") \
	_(ZEND_STR_STRING,                 "string") \
	_(ZEND_STR_TRACE,                  "trace") \
	_(ZEND_STR_SCHEME,                 "scheme") \
	_(ZEND_STR_HOST,                   "host") \
	_(ZEND_STR_PORT,                   "port") \
	_(ZEND_STR_USER,                   "user") \
	_(ZEND_STR_PASS,                   "pass") \
	_(ZEND_STR_PATH,                   "path") \
	_(ZEND_STR_QUERY,                  "query") \
	_(ZEND_STR_FRAGMENT,               "fragment") \
	_(ZEND_STR_NULL,                   "NULL") \
	_(ZEND_STR_BOOLEAN,                "boolean") \
	_(ZEND_STR_INTEGER,                "integer") \
	_(ZEND_STR_DOUBLE,                 "double") \
	_(ZEND_STR_ARRAY,                  "array") \
	_(ZEND_STR_RESOURCE,               "resource") \
	_(ZEND_STR_CLOSED_RESOURCE,        "resource (closed)") \
	_(ZEND_STR_NAME,                   "name") \
	_(ZEND_STR_ARGV,                   "argv") \
	_(ZEND_STR_ARGC,                   "argc") \
	_(ZEND_STR_ARRAY_CAPITALIZED,      "Array") \
	_(ZEND_STR_BOOL,                   "bool") \
	_(ZEND_STR_INT,                    "int") \
	_(ZEND_STR_FLOAT,                  "float") \
	_(ZEND_STR_CALLABLE,               "callable") \
	_(ZEND_STR_ITERABLE,               "iterable") \
	_(ZEND_STR_VOID,                   "void") \
	_(ZEND_STR_NEVER,                  "never") \
	_(ZEND_STR_FALSE,                  "false") \
	_(ZEND_STR_TRUE,                   "true") \
	_(ZEND_STR_NULL_LOWERCASE,         "null") \
	_(ZEND_STR_MIXED,                  "mixed") \
	_(ZEND_STR_TRAVERSABLE,            "Traversable") \
	_(ZEND_STR_SLEEP,                  "__sleep") \
	_(ZEND_STR_WAKEUP,                 "__wakeup") \
	_(ZEND_STR_CASES,                  "cases") \
	_(ZEND_STR_FROM,                   "from") \
	_(ZEND_STR_TRYFROM,                "tryFrom") \
	_(ZEND_STR_TRYFROM_LOWERCASE,      "tryfrom") \
	_(ZEND_STR_AUTOGLOBAL_SERVER,      "_SERVER") \
	_(ZEND_STR_AUTOGLOBAL_ENV,         "_ENV") \
	_(ZEND_STR_AUTOGLOBAL_REQUEST,     "_REQUEST") \
	_(ZEND_STR_COUNT,                  "count") \
	_(ZEND_STR_SENSITIVEPARAMETER,     "SensitiveParameter") \


// /Zend/zend_enum.c 中用到 zend_known_string_id
// ing4, 已知字串，主要用于 包含上面 ZEND_KNOWN_STRINGS 里所有的字串名
typedef enum _zend_known_string_id {
// 定义一个宏，用完马上删掉。
// clear, 返回第一个参数，第二个丢掉
#define _ZEND_STR_ID(id, str) id,
// 用 上面这个 宏，把所有 ZEND_KNOWN_STRINGS 中的字串常量的第一个元素添加到 enum 中
ZEND_KNOWN_STRINGS(_ZEND_STR_ID)
// 删除宏
#undef _ZEND_STR_ID
	// 最后一个未知字串
	ZEND_STR_LAST_KNOWN
} zend_known_string_id;

#endif /* ZEND_STRING_H */
