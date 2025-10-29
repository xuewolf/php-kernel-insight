// HashTable *interned_strings 相关的方法有4个
static zend_always_inline void zend_init_interned_strings_ht(HashTable *interned_strings, bool permanent)
static zend_always_inline zend_string *zend_interned_string_ht_lookup_ex(zend_ulong h, const char *str, size_t size, HashTable *interned_strings)
static zend_always_inline zend_string *zend_interned_string_ht_lookup(zend_string *str, HashTable *interned_strings)
static zend_always_inline zend_string *zend_add_interned_string(zend_string *str, HashTable *interned_strings, uint32_t flags)

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

#include "zend.h"
#include "zend_globals.h"

#ifdef HAVE_VALGRIND
# include "valgrind/callgrind.h"
#endif

// 这几个是永久方法，只有 zend_interned_strings_init() 和 zend_interned_strings_switch_storage()赋值。
ZEND_API zend_new_interned_string_func_t zend_new_interned_string;
ZEND_API zend_string_init_interned_func_t zend_string_init_interned;
ZEND_API zend_string_init_existing_interned_func_t zend_string_init_existing_interned;

static zend_string* ZEND_FASTCALL zend_new_interned_string_permanent(zend_string *str);
static zend_string* ZEND_FASTCALL zend_new_interned_string_request(zend_string *str);
static zend_string* ZEND_FASTCALL zend_string_init_interned_permanent(const char *str, size_t size, bool permanent);
static zend_string* ZEND_FASTCALL zend_string_init_existing_interned_permanent(const char *str, size_t size, bool permanent);
static zend_string* ZEND_FASTCALL zend_string_init_interned_request(const char *str, size_t size, bool permanent);
static zend_string* ZEND_FASTCALL zend_string_init_existing_interned_request(const char *str, size_t size, bool permanent);

/* Any strings interned in the startup phase. Common to all the threads,
   won't be free'd until process exit. If we want an ability to
   add permanent strings even after startup, it would be still
   possible on costs of locking in the thread safe builds. */
static HashTable interned_strings_permanent;

// 这几个有初始值，zend_interned_strings_init()赋值，zend_interned_strings_set_request_storage_handlers()内核无调用
static zend_new_interned_string_func_t interned_string_request_handler = zend_new_interned_string_request;
static zend_string_init_interned_func_t interned_string_init_request_handler = zend_string_init_interned_request;
static zend_string_init_existing_interned_func_t interned_string_init_existing_request_handler = zend_string_init_existing_interned_request;

ZEND_API zend_string  *zend_empty_string = NULL;
ZEND_API zend_string  *zend_one_char_string[256];
ZEND_API zend_string **zend_known_strings = NULL;

// clear, 给 zend_string 计算哈希值
ZEND_API zend_ulong ZEND_FASTCALL zend_string_hash_func(zend_string *str)
{
	return ZSTR_H(str) = zend_hash_func(ZSTR_VAL(str), ZSTR_LEN(str));
}

// ing4, 哈希函数的别名
ZEND_API zend_ulong ZEND_FASTCALL zend_hash_func(const char *str, size_t len)
{
	// 计算作为数组key的哈希值
	return zend_inline_hash_func(str, len);
}

// ing4, 销毁保留字串的方法，留给hash表调用
static void _str_dtor(zval *zv)
{
	zend_string *str = Z_STR_P(zv);
	pefree(str, GC_FLAGS(str) & IS_STR_PERSISTENT);
}

// clear, 已知字串
// #define _ZEND_STR_DSC(id, str) str,
static const char *known_strings[] = {
#define _ZEND_STR_DSC(id, str) str,
ZEND_KNOWN_STRINGS(_ZEND_STR_DSC)
#undef _ZEND_STR_DSC
	NULL
};

// ing4, 初始化保留字哈希表
static zend_always_inline void zend_init_interned_strings_ht(HashTable *interned_strings, bool permanent)
{
	// 初始化哈希表
	zend_hash_init(interned_strings, 1024, NULL, _str_dtor, permanent);
	// 如果是永久保留字
	if (permanent) {
		// 初始化混合哈希表
		zend_hash_real_init_mixed(interned_strings);
	}
}

// ing4, 初始化保留字串
ZEND_API void zend_interned_strings_init(void)
{
	char s[2];
	unsigned int i;
	zend_string *str;
	// 临时保留字处理方法
	interned_string_request_handler = zend_new_interned_string_request;
	interned_string_init_request_handler = zend_string_init_interned_request;
	interned_string_init_existing_request_handler = zend_string_init_existing_interned_request;
	// 空字串
	zend_empty_string = NULL;
	// 已知字串
	zend_known_strings = NULL;

	// 初始化永久哈希表
	zend_init_interned_strings_ht(&interned_strings_permanent, 1);
	// 永久保留字处理方法
	zend_new_interned_string = zend_new_interned_string_permanent;
	zend_string_init_interned = zend_string_init_interned_permanent;
	zend_string_init_existing_interned = zend_string_init_existing_interned_permanent;

	// 空字串 保留字，先创建好
	/* interned empty string */
	str = zend_string_alloc(sizeof("")-1, 1);
	// 空字串
	ZSTR_VAL(str)[0] = '\000';
	// 创建成永久保留字
	zend_empty_string = zend_new_interned_string_permanent(str);

	s[1] = 0;
	// 256个单字节字符
	for (i = 0; i < 256; i++) {
		s[0] = i;
		// 创建成永久保留字
		zend_one_char_string[i] = zend_new_interned_string_permanent(zend_string_init(s, 1, 1));
	}

	/* known strings */
	// 已知字串
	zend_known_strings = pemalloc(sizeof(zend_string*) * ((sizeof(known_strings) / sizeof(known_strings[0]) - 1)), 1);
	// 全部创建成永久保留字
	for (i = 0; i < (sizeof(known_strings) / sizeof(known_strings[0])) - 1; i++) {
		str = zend_string_init(known_strings[i], strlen(known_strings[i]), 1);
		zend_known_strings[i] = zend_new_interned_string_permanent(str);
	}
}

// ing4, 销毁保留字列表
ZEND_API void zend_interned_strings_dtor(void)
{
	// 销毁保留字哈希表
	zend_hash_destroy(&interned_strings_permanent);
	// 销毁已知字串
	free(zend_known_strings);
	// 清空指针
	zend_known_strings = NULL;
}

// ing4, 从哈希表中，通过 key 获取 指定元素
static zend_always_inline zend_string *zend_interned_string_ht_lookup_ex(zend_ulong h, const char *str, size_t size, HashTable *interned_strings)
{
	uint32_t nIndex;
	uint32_t idx;
	Bucket *p;
	// 把哈希值换算成有效索引号
	nIndex = h | interned_strings->nTableMask;
	// 把索引号换算成存放位置
	idx = HT_HASH(interned_strings, nIndex);
	// 遍历整个bucket串
	while (idx != HT_INVALID_IDX) {
		// 获得bucket
		p = HT_HASH_TO_BUCKET(interned_strings, idx);
		// 如果 key 匹配无误
		if ((p->h == h) && zend_string_equals_cstr(p->key, str, size)) {
			// 返回 key
			return p->key;
		}
		// 下一个bucket
		idx = Z_NEXT(p->val);
	}
	// 匹配失败
	return NULL;
}

// ing4, 在指定哈希表中查找保留字串
static zend_always_inline zend_string *zend_interned_string_ht_lookup(zend_string *str, HashTable *interned_strings)
{
	// 取得哈希值
	zend_ulong h = ZSTR_H(str);
	uint32_t nIndex;
	uint32_t idx;
	Bucket *p;

	// 索引号
	nIndex = h | interned_strings->nTableMask;
	// 索引位置
	idx = HT_HASH(interned_strings, nIndex);
	// 如果idx有效
	while (idx != HT_INVALID_IDX) {
		// 取得对应的bucket
		p = HT_HASH_TO_BUCKET(interned_strings, idx);
		// 如果哈希值匹配，且key匹配
		if ((p->h == h) && zend_string_equal_content(p->key, str)) {
			// 返回这个key
			return p->key;
		}
		// 下一个bucket
		idx = Z_NEXT(p->val);
	}
	// 查找失败
	return NULL;
}

// 这个函数最终无法保证线程安全，因为它会修改 哈希表中以往的string值。保证在合适的上下文中使用它。
/* This function might be not thread safe at least because it would update the
   hash val in the passed string. Be sure it is called in the appropriate context. */
// ing3, 添加保留字串
static zend_always_inline zend_string *zend_add_interned_string(zend_string *str, HashTable *interned_strings, uint32_t flags)
{
	zval val;
	// str 设置引用次数
	GC_SET_REFCOUNT(str, 1);
	// 添加保留字串标记
	GC_ADD_FLAGS(str, IS_STR_INTERNED | flags);

	// 创建保留字串
	ZVAL_INTERNED_STR(&val, str);

	// 在保留字哈希表中添加新元素
	zend_hash_add_new(interned_strings, str, &val);

	return str;
}

ZEND_API zend_string* ZEND_FASTCALL zend_interned_string_find_permanent(zend_string *str)
{
	zend_string_hash_val(str);
	return zend_interned_string_ht_lookup(str, &interned_strings_permanent);
}

// ing4, 新建永久保留字，传入 zend_string
static zend_string* ZEND_FASTCALL zend_new_interned_string_permanent(zend_string *str)
{
	zend_string *ret;
	// 如果是保留字串，直接返回
	if (ZSTR_IS_INTERNED(str)) {
		return str;
	}

	// 计算哈希值
	zend_string_hash_val(str);
	// 在保留字列表中查找 
	ret = zend_interned_string_ht_lookup(str, &interned_strings_permanent);
	// 如果已经存在
	if (ret) {
		// 释放当前字串
		zend_string_release(str);
		// 返回创建好的保留字
		return ret;
	}
	// 如果未创建过保留字
	// 必须是永久保留字
	ZEND_ASSERT(GC_FLAGS(str) & GC_PERSISTENT);
	// 如果字串引用次数大于1
	if (GC_REFCOUNT(str) > 1) {
		// 计算哈希值
		zend_ulong h = ZSTR_H(str);
		// 减少引用次数
		zend_string_delref(str);
		// 创建副本
		str = zend_string_init(ZSTR_VAL(str), ZSTR_LEN(str), 1);
		// 写入哈希值
		ZSTR_H(str) = h;
	}
	// 把传入的字串（或者副本）添加到永久保留字中
	return zend_add_interned_string(str, &interned_strings_permanent, IS_STR_PERMANENT);
}

// ing4, 创建新的跟随请求的保留字
static zend_string* ZEND_FASTCALL zend_new_interned_string_request(zend_string *str)
{
	zend_string *ret;

	// 如果已经是保留字，直接返回
	if (ZSTR_IS_INTERNED(str)) {
		return str;
	}

	// 计算哈希值
	zend_string_hash_val(str);

	// 检查永久保留字串，在这个点上，表是只读的
	/* Check for permanent strings, the table is readonly at this point. */
	ret = zend_interned_string_ht_lookup(str, &interned_strings_permanent);
	// 如果找到
	if (ret) {
		// 把字串释放掉
		zend_string_release(str);
		// 返回哈希表里的字串
		return ret;
	}
	// 如果没找到
	// 检查非永久保留字串
	ret = zend_interned_string_ht_lookup(str, &CG(interned_strings));
	// 如果找到
	if (ret) {
		// 把字串释放掉
		zend_string_release(str);
		// 返回哈希表里的字串
		return ret;
	}

	// 调试用，创建一个短的活动保留字，请求后释放掉
	/* Create a short living interned, freed after the request. */
#if ZEND_RC_DEBUG
	if (zend_rc_debug) {
		/* PHP shouldn't create persistent interned string during request,
		 * but at least dl() may do this */
		ZEND_ASSERT(!(GC_FLAGS(str) & GC_PERSISTENT));
	}
#endif
	// 如果字串引用次数大于1 
	if (GC_REFCOUNT(str) > 1) {
		// 取出哈希值
		zend_ulong h = ZSTR_H(str);
		// 减少引用次数
		zend_string_delref(str);
		// 创建副本
		str = zend_string_init(ZSTR_VAL(str), ZSTR_LEN(str), 0);
		// 新版本的哈希值
		ZSTR_H(str) = h;
	}

	// 把str(或者副本）添加到非永久保留字中
	ret = zend_add_interned_string(str, &CG(interned_strings), 0);

	// 返回字串指针
	return ret;
}

// ing4, 创建永久保留字，p1:字串，p2:长度，p3:是否永久
static zend_string* ZEND_FASTCALL zend_string_init_interned_permanent(const char *str, size_t size, bool permanent)
{
	zend_string *ret;
	// 计算作为数组key的哈希值
	zend_ulong h = zend_inline_hash_func(str, size);
	// 从永久保留字中获取
	ret = zend_interned_string_ht_lookup_ex(h, str, size, &interned_strings_permanent);
	// 如果获取成功，返回
	if (ret) {
		return ret;
	}
	// 必须是 permanent 永久保留字
	ZEND_ASSERT(permanent);
	// 创建zend_string
	ret = zend_string_init(str, size, permanent);
	// 写入hash值
	ZSTR_H(ret) = h;
	// 添加到保留字列表中
	return zend_add_interned_string(ret, &interned_strings_permanent, IS_STR_PERMANENT);
}

// ing4, 查找已存在的永久保留字，如果不存在，创建 普通 zend_string 并返回
static zend_string* ZEND_FASTCALL zend_string_init_existing_interned_permanent(const char *str, size_t size, bool permanent)
{
	// 计算作为数组key的哈希值
	zend_ulong h = zend_inline_hash_func(str, size);
	// 从永久保留字中获取
	zend_string *ret = zend_interned_string_ht_lookup_ex(h, str, size, &interned_strings_permanent);
	// 如果获取成功，返回
	if (ret) {
		return ret;
	}
	// 必须是 permanent 永久保留字
	ZEND_ASSERT(permanent);
	// 创建zend_string
	ret = zend_string_init(str, size, permanent);
	// 写入hash值
	ZSTR_H(ret) = h;
	// 不创建保留字，直接返回
	return ret;
}

// ing4, 创建非永久保留字
static zend_string* ZEND_FASTCALL zend_string_init_interned_request(const char *str, size_t size, bool permanent)
{
	zend_string *ret;
	// 计算作为数组key的哈希值
	zend_ulong h = zend_inline_hash_func(str, size);

	// 检验永久字串，在这个点上列表是只读的？
	/* Check for permanent strings, the table is readonly at this point. */
	// 先在当前存储空间中查找
	ret = zend_interned_string_ht_lookup_ex(h, str, size, &interned_strings_permanent);
	if (ret) {
		return ret;
	}

	// 如果找不到，编译常量空间中查找 
	ret = zend_interned_string_ht_lookup_ex(h, str, size, &CG(interned_strings));
	if (ret) {
		return ret;
	}

#if ZEND_RC_DEBUG
	if (zend_rc_debug) {
		/* PHP shouldn't create persistent interned string during request,
		 * but at least dl() may do this */
		ZEND_ASSERT(!permanent);
	}
#endif
	// 如果都找不到，创建zend_string
	ret = zend_string_init(str, size, permanent);
	// 写入hash
	ZSTR_H(ret) = h;
	// 创建一个短期的保留字，请求处理完后释放掉
	/* Create a short living interned, freed after the request. */
	// 把新创建的zend_string 添加进非永久保留字列表
	return zend_add_interned_string(ret, &CG(interned_strings), 0);
}

// ing4, 在已有的临时保留字列表中查找，如果不存在，创建普通 zend_string 并返回
static zend_string* ZEND_FASTCALL zend_string_init_existing_interned_request(const char *str, size_t size, bool permanent)
{
	// 计算作为数组key的哈希值
	zend_ulong h = zend_inline_hash_func(str, size);
	// 在永久保留字列表中查找 
	zend_string *ret = zend_interned_string_ht_lookup_ex(h, str, size, &interned_strings_permanent);
	// 找到就返回
	if (ret) {
		return ret;
	}

	// 在临时保留字列表中查找，找到就返回
	ret = zend_interned_string_ht_lookup_ex(h, str, size, &CG(interned_strings));
	if (ret) {
		return ret;
	}
	// 如果都没找到
	// 必须是临时保留字
	ZEND_ASSERT(!permanent);
	
	// 创建普通 zend_string
	ret = zend_string_init(str, size, permanent);
	// 写入哈希值
	ZSTR_H(ret) = h;
	// 返回
	return ret;
}

// ing4, 激活临时保留字功能
ZEND_API void zend_interned_strings_activate(void)
{
	// 初始化非永久保留字哈希表
	zend_init_interned_strings_ht(&CG(interned_strings), 0);
}

// ing4, 关闭临时保留字功能
ZEND_API void zend_interned_strings_deactivate(void)
{
	zend_hash_destroy(&CG(interned_strings));
}

// ing4, 设置隨请求的保留字的处理方法。优化器 ZendAccelerator 中用到
ZEND_API void zend_interned_strings_set_request_storage_handlers(zend_new_interned_string_func_t handler, zend_string_init_interned_func_t init_handler, zend_string_init_existing_interned_func_t init_existing_handler)
{
	interned_string_request_handler = handler;
	interned_string_init_request_handler = init_handler;
	interned_string_init_existing_request_handler = init_existing_handler;
}

// ing4, 切换保留字的存储位置，在main.c中调用
ZEND_API void zend_interned_strings_switch_storage(bool request)
{
	// php_module_startup 调用
	if (request) {
		zend_new_interned_string = interned_string_request_handler;
		zend_string_init_interned = interned_string_init_request_handler;
		zend_string_init_existing_interned = interned_string_init_existing_request_handler;
	// php_module_shutdown 调用
	} else {
		zend_new_interned_string = zend_new_interned_string_permanent;
		zend_string_init_interned = zend_string_init_interned_permanent;
		zend_string_init_existing_interned = zend_string_init_existing_interned_permanent;
	}
}

// 只有当创建时不支持 valgrind，包含这个符号，让 valgrind 可用。
/* Even if we don't build with valgrind support, include the symbol so that valgrind available
 * only at runtime will not result in false positives. */
// ing4, 动态生成方法名
#ifndef I_REPLACE_SONAME_FNNAME_ZU
# define I_REPLACE_SONAME_FNNAME_ZU(soname, fnname) _vgr00000ZU_ ## soname ## _ ## fnname
#endif

// 这个是动态生成的函数名，全局无调用
// ing4, 两个zend_string 不相同返回 1 否则 0
ZEND_API bool ZEND_FASTCALL I_REPLACE_SONAME_FNNAME_ZU(NONE,zend_string_equal_val)(const zend_string *s1, const zend_string *s2)
{
	// 
	return !memcmp(ZSTR_VAL(s1), ZSTR_VAL(s2), ZSTR_LEN(s1));
}

// gnuc,32位
#if defined(__GNUC__) && defined(__i386__)
// ing1
ZEND_API bool ZEND_FASTCALL zend_string_equal_val(const zend_string *s1, const zend_string *s2)
{
	const char *ptr = ZSTR_VAL(s1);
	size_t delta = (const char*)s2 - (const char*)s1;
	size_t len = ZSTR_LEN(s1);
	zend_ulong ret;

	__asm__ (
		".LL0%=:\n\t"
		"movl (%2,%3), %0\n\t"
		"xorl (%2), %0\n\t"
		"jne .LL1%=\n\t"
		"addl $0x4, %2\n\t"
		"subl $0x4, %1\n\t"
		"ja .LL0%=\n\t"
		"movl $0x1, %0\n\t"
		"jmp .LL3%=\n\t"
		".LL1%=:\n\t"
		"cmpl $0x4,%1\n\t"
		"jb .LL2%=\n\t"
		"xorl %0, %0\n\t"
		"jmp .LL3%=\n\t"
		".LL2%=:\n\t"
		"negl %1\n\t"
		"lea 0x20(,%1,8), %1\n\t"
		"shll %b1, %0\n\t"
		"sete %b0\n\t"
		"movzbl %b0, %0\n\t"
		".LL3%=:\n"
		: "=&a"(ret),
		  "+c"(len),
		  "+r"(ptr)
		: "r"(delta)
		: "cc");
	return ret;
}
// gnuc,64位，没有 __ILP32__
#elif defined(__GNUC__) && defined(__x86_64__) && !defined(__ILP32__)
// ing1, windows sdk 里没有 __asm__
ZEND_API bool ZEND_FASTCALL zend_string_equal_val(const zend_string *s1, const zend_string *s2)
{
	const char *ptr = ZSTR_VAL(s1);
	size_t delta = (const char*)s2 - (const char*)s1;
	size_t len = ZSTR_LEN(s1);
	zend_ulong ret;

	__asm__ (
		".LL0%=:\n\t"
		"movq (%2,%3), %0\n\t"
		"xorq (%2), %0\n\t"
		"jne .LL1%=\n\t"
		"addq $0x8, %2\n\t"
		"subq $0x8, %1\n\t"
		"ja .LL0%=\n\t"
		"movq $0x1, %0\n\t"
		"jmp .LL3%=\n\t"
		".LL1%=:\n\t"
		"cmpq $0x8,%1\n\t"
		"jb .LL2%=\n\t"
		"xorq %0, %0\n\t"
		"jmp .LL3%=\n\t"
		".LL2%=:\n\t"
		"negq %1\n\t"
		"lea 0x40(,%1,8), %1\n\t"
		"shlq %b1, %0\n\t"
		"sete %b0\n\t"
		"movzbq %b0, %0\n\t"
		".LL3%=:\n"
		: "=&a"(ret),
		  "+c"(len),
		  "+r"(ptr)
		: "r"(delta)
		: "cc");
	return ret;
}
#endif

// ing4, 拼接2个字串
ZEND_API zend_string *zend_string_concat2(
		const char *str1, size_t str1_len,
		const char *str2, size_t str2_len)
{
	// 长度和
	size_t len = str1_len + str2_len;
	// 创建新字串
	zend_string *res = zend_string_alloc(len, 0);

	// 依次复制两个字串， memcpy 测试过
	memcpy(ZSTR_VAL(res), str1, str1_len);
	memcpy(ZSTR_VAL(res) + str1_len, str2, str2_len);
	ZSTR_VAL(res)[len] = '\0';

	return res;
}

// ing4， 拼接3个字串
ZEND_API zend_string *zend_string_concat3(
		const char *str1, size_t str1_len,
		const char *str2, size_t str2_len,
		const char *str3, size_t str3_len)
{
	// 长度和
	size_t len = str1_len + str2_len + str3_len;
	// 分配内存
	zend_string *res = zend_string_alloc(len, 0);

	// 依次赋值3个字串
	memcpy(ZSTR_VAL(res), str1, str1_len);
	memcpy(ZSTR_VAL(res) + str1_len, str2, str2_len);
	memcpy(ZSTR_VAL(res) + str1_len + str2_len, str3, str3_len);
	// 
	ZSTR_VAL(res)[len] = '\0';

	return res;
}
