/*
   +----------------------------------------------------------------------+
   | Zend OPcache JIT                                                     |
   +----------------------------------------------------------------------+
   | Copyright (c) The PHP Group                                          |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | https://www.php.net/license/3_01.txt                                 |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Dmitry Stogov <dmitry@php.net>                              |
   +----------------------------------------------------------------------+
*/

#ifndef _ZEND_BITSET_H_
#define _ZEND_BITSET_H_

// 比特集，无符号整形目标 的 指针。64位系统占个字节（测试过）
// 它指向一串 无符号整形 数值。这样伸缩性好。
typedef zend_ulong *zend_bitset;

// zend_ulong 的大小 ，4或8字节
#define ZEND_BITSET_ELM_SIZE sizeof(zend_ulong)

// 如果是32位系统 
#if SIZEOF_ZEND_LONG == 4
// 需要元素数，n / 32
# define ZEND_BITSET_ELM_NUM(n)		((n) >> 5)
// 需要位 数，最后5位，最大32
# define ZEND_BITSET_BIT_NUM(n)		((zend_ulong)(n) & Z_UL(0x1f))
// 64位系统
#elif SIZEOF_ZEND_LONG == 8
// ing4, 所属块号,向下取整（从0开始），n / 64
# define ZEND_BITSET_ELM_NUM(n)		((n) >> 6)
// ing4, 在所属块号中的bit顺序号（从0开始），最后6位，最大63
# define ZEND_BITSET_BIT_NUM(n)		((zend_ulong)(n) & Z_UL(0x3f))
// 其他系统（？）
#else
// n / （整数位数*8）
# define ZEND_BITSET_ELM_NUM(n)		((n) / (sizeof(zend_long) * 8))
// n % （整数位数*8）
# define ZEND_BITSET_BIT_NUM(n)		((n) % (sizeof(zend_long) * 8))
#endif

// ing3, 分配内存创建比特集
#define ZEND_BITSET_ALLOCA(n, use_heap) \
	(zend_bitset)do_alloca((n) * ZEND_BITSET_ELM_SIZE, use_heap)

// ing3, 获取右侧连续0的数量（从0开始，右到左）
/* Number of trailing zero bits (0x01 -> 0; 0x40 -> 6; 0x00 -> LEN) */
static zend_always_inline int zend_ulong_ntz(zend_ulong num)
{
// gnuc 语言 或 __builtin_ctzl
#if (defined(__GNUC__) || __has_builtin(__builtin_ctzl)) \
	&& SIZEOF_ZEND_LONG == SIZEOF_LONG && defined(PHP_HAVE_BUILTIN_CTZL)
	return __builtin_ctzl(num);
#elif (defined(__GNUC__) || __has_builtin(__builtin_ctzll)) && defined(PHP_HAVE_BUILTIN_CTZLL)
	return __builtin_ctzll(num);
// 64位windows会走这里
#elif defined(_WIN32)
	unsigned long index;

// 64位windows会走这里
#if defined(_WIN64)
	if (!BitScanForward64(&index, num)) {
#else
	if (!BitScanForward(&index, num)) {
#endif
		/* undefined behavior */
		return SIZEOF_ZEND_LONG * 8;
	}

	return (int) index;
// 其他情况
#else
	int n;

	if (num == Z_UL(0)) return SIZEOF_ZEND_LONG * 8;

	n = 1;
// 如果是64位
#if SIZEOF_ZEND_LONG == 8
	// 最右32个0
	if ((num & 0xffffffff) == 0) {n += 32; num = num >> Z_UL(32);}
#endif
	// 最右16个0
	if ((num & 0x0000ffff) == 0) {n += 16; num = num >> 16;}
	// 最右8个0
	if ((num & 0x000000ff) == 0) {n +=  8; num = num >>  8;}
	// 最右4个0
	if ((num & 0x0000000f) == 0) {n +=  4; num = num >>  4;}
	// 最右2个0
	if ((num & 0x00000003) == 0) {n +=  2; num = num >>  2;}
	// 最右1个0
	return n - (num & 1);
#endif
}

// ing3, 开头的0的数量
/* Number of leading zero bits (Undefined for zero) */
static zend_always_inline int zend_ulong_nlz(zend_ulong num)
{
#if (defined(__GNUC__) || __has_builtin(__builtin_clzl)) \
	&& SIZEOF_ZEND_LONG == SIZEOF_LONG && defined(PHP_HAVE_BUILTIN_CLZL)
	return __builtin_clzl(num);
#elif (defined(__GNUC__) || __has_builtin(__builtin_clzll)) && defined(PHP_HAVE_BUILTIN_CLZLL)
	return __builtin_clzll(num);
#elif defined(_WIN32)
	unsigned long index;

#if defined(_WIN64)
	if (!BitScanReverse64(&index, num)) {
#else
	if (!BitScanReverse(&index, num)) {
#endif
		/* undefined behavior */
		return SIZEOF_ZEND_LONG * 8;
	}

	return (int) (SIZEOF_ZEND_LONG * 8 - 1)- index;
#else
	zend_ulong x;
	int n;

#if SIZEOF_ZEND_LONG == 8
	n = 64;
	// 左面32位有1，要减掉右面32位，它们是不是1都要减
	x = num >> 32; if (x != 0) {n -= 32; num = x;}
#else
	n = 32;
#endif
	// 左面16位有1，要减掉16，它们是不是1都要减
	x = num >> 16; if (x != 0) {n -= 16; num = x;}
	// 左面8位有1，要减掉8，它们是不是1都要减
	x = num >> 8;  if (x != 0) {n -=  8; num = x;}
	// 左面4位有1，要减掉4，它们是不是1都要减
	x = num >> 4;  if (x != 0) {n -=  4; num = x;}
	// 左面2位有1，要减掉2，它们是不是1都要减
	x = num >> 2;  if (x != 0) {n -=  2; num = x;}
	// 左面1位有1，要减掉1，它们是不是1都要减
	x = num >> 1;  if (x != 0) return n - 2;
	return n - num;
#endif
}

// ing4, 计算 n个bit长的串，需要的 bitset数
/* Returns the number of zend_ulong words needed to store a bitset that is N
   bits long.  */
static inline uint32_t zend_bitset_len(uint32_t n)
{
	// 增加不到1个bitset 。除后向下取整。
	return (n + ((sizeof(zend_long) * 8) - 1)) / (sizeof(zend_long) * 8);
}

// ing4, 验证某一个位是否是1
static inline bool zend_bitset_in(zend_bitset set, uint32_t n)
{
	// 验证某一个位是否是1
	return ZEND_BIT_TEST(set, n);
}

// ing4, 把1个块中的某个位设置成 1
static inline void zend_bitset_incl(zend_bitset set, uint32_t n)
{
	// ZEND_BITSET_ELM_NUM 所属块号,向下取整（从0开始）
	// ZEND_BITSET_BIT_NUM 在所属块号中的bit顺序号（从0开始）
	set[ZEND_BITSET_ELM_NUM(n)] |= Z_UL(1) << ZEND_BITSET_BIT_NUM(n);
}

// ing4, 把1个块中的某个位设置成 0
static inline void zend_bitset_excl(zend_bitset set, uint32_t n)
{
	// ZEND_BITSET_ELM_NUM 所属块号,向下取整（从0开始）
	// ZEND_BITSET_BIT_NUM 在所属块号中的bit顺序号（从0开始）
	set[ZEND_BITSET_ELM_NUM(n)] &= ~(Z_UL(1) << ZEND_BITSET_BIT_NUM(n));
}

// ing4, 清空bitset，len数量
static inline void zend_bitset_clear(zend_bitset set, uint32_t len)
{
	memset(set, 0, len * ZEND_BITSET_ELM_SIZE);
}

// ing4, 检查bitset是否为空
static inline bool zend_bitset_empty(zend_bitset set, uint32_t len)
{
	uint32_t i;
	for (i = 0; i < len; i++) {
		if (set[i]) {
			return 0;
		}
	}
	return 1;
}

// ing4, 所有位设置成1 （测试过,2个int）
static inline void zend_bitset_fill(zend_bitset set, uint32_t len)
{
	memset(set, 0xff, len * ZEND_BITSET_ELM_SIZE);
}

// ing4, set和set2是否相等，使用内存比较
static inline bool zend_bitset_equal(zend_bitset set1, zend_bitset set2, uint32_t len)
{
    return memcmp(set1, set2, len * ZEND_BITSET_ELM_SIZE) == 0;
}

// ing4, set2 复制到set1里
static inline void zend_bitset_copy(zend_bitset set1, zend_bitset set2, uint32_t len)
{
    memcpy(set1, set2, len * ZEND_BITSET_ELM_SIZE);
}

// ing4, 计算并集并返回到set1里
static inline void zend_bitset_intersection(zend_bitset set1, zend_bitset set2, uint32_t len)
{
    uint32_t i;

    for (i = 0; i < len; i++) {
		set1[i] &= set2[i];
	}
}

// ing4, 两个 bitset串 的并集（或运算），结果放在set1里
static inline void zend_bitset_union(zend_bitset set1, zend_bitset set2, uint32_t len)
{
	uint32_t i;

	for (i = 0; i < len; i++) {
		set1[i] |= set2[i];
	}
}

// ing4, 从set1里去掉set2里有的部分，结果放在set1里
static inline void zend_bitset_difference(zend_bitset set1, zend_bitset set2, uint32_t len)
{
	uint32_t i;

	for (i = 0; i < len; i++) {
		set1[i] = set1[i] & ~set2[i];
	}
}

// ing4, 计算 set2 并 （set3 交 set 4),结果放在set1里
static inline void zend_bitset_union_with_intersection(zend_bitset set1, zend_bitset set2, zend_bitset set3, zend_bitset set4, uint32_t len)
{
	uint32_t i;

	for (i = 0; i < len; i++) {
		set1[i] = set2[i] | (set3[i] & set4[i]);
	}
}

// ing4, 计算 set2 并 （set3 中删除 set 4),结果放在set1里
static inline void zend_bitset_union_with_difference(zend_bitset set1, zend_bitset set2, zend_bitset set3, zend_bitset set4, uint32_t len)
{
	uint32_t i;

	for (i = 0; i < len; i++) {
		set1[i] = set2[i] | (set3[i] & ~set4[i]);
	}
}

// ing4, 计算set2是否是set1的子集。如果set1 中 删除 set2后还有值，返回0。 否则返回1
static inline bool zend_bitset_subset(zend_bitset set1, zend_bitset set2, uint32_t len)
{
	uint32_t i;

	for (i = 0; i < len; i++) {
		if (set1[i] & ~set2[i]) {
			return 0;
		}
	}
	return 1;
}

// ing4, 取得右面第一个 1 的位置序号（从0开始）
static inline int zend_bitset_first(zend_bitset set, uint32_t len)
{
	uint32_t i;
	// 从右到左遍历每个块
	for (i = 0; i < len; i++) {
		// 如果块里有1
		if (set[i]) {
			// zend_ulong_ntz 结尾0的数量
			// 取得第一个是 1 的位置序号（从0开始，右到左）
			return ZEND_BITSET_ELM_SIZE * 8 * i + zend_ulong_ntz(set[i]);
		}
	}
	return -1; /* empty set */
}

// ing4, 获取左面第一个1的顺序号
static inline int zend_bitset_last(zend_bitset set, uint32_t len)
{
	uint32_t i = len;
	// 从左到右遍历每个块
	while (i > 0) {
		// 向右一个
		i--;
		// 如果 块 里有 1
		if (set[i]) {
			// 这个块的最小序号。
			// 初始是-1。因为0就是有效位置了。上面判断过了 set[i]有效，后面至少会加1次！
			int j = ZEND_BITSET_ELM_SIZE * 8 * i - 1;
			// 取出块
			zend_ulong x = set[i];
			// 只要还有1，就继续循环
			while (x != Z_UL(0)) {
				// 右移1位
				x = x >> Z_UL(1);
				// 序号+1
				j++;
			}
			// 
			return j;
		}
	}
	
	return -1; /* empty set */
}

// ing4, 遍历一串 bitset，跳过所有0，碰到1 执行自定义逻辑（开头部分）
// bit是返回的位序号，它返回最大的有效位序号，从右往左。
#define ZEND_BITSET_FOREACH(set, len, bit) do { \
	/* 临时变量 */ \
	zend_bitset _set = (set); \
	/* 临时变量 */ \
	uint32_t _i, _len = (len); \
	/* 遍历整个长度 */ \
	for (_i = 0; _i < _len; _i++) { \
		/* 第_i个整数（bitset） */ \
		zend_ulong _x = _set[_i]; \
		/* 如果它不是0 */ \
		if (_x) { \
			/* 要检查的位序号 */ \
			(bit) = ZEND_BITSET_ELM_SIZE * 8 * _i; \
			/* 只要左面还有1，就一直右移。bit记录最大的检查序号 */ \
			for (; _x != 0; _x >>= Z_UL(1), (bit)++) { \
				/* 如果位是0，下一个位。这样后面就可以插入有效情况的业务逻辑了 */ \
				if (!(_x & Z_UL(1))) continue;


// ing4, 遍历一串 bitset，跳过所有0，碰到1 执行自定义逻辑（开头部分）
#define ZEND_BITSET_REVERSE_FOREACH(set, len, bit) do { \
	/* 临时变量 */ \
	zend_bitset _set = (set); \
	/* 临时变量 */ \
	uint32_t _i = (len); \
	/* 要检测的位 */ \
	zend_ulong _test = Z_UL(1) << (ZEND_BITSET_ELM_SIZE * 8 - 1); \
	/* 从大到小，遍历每一个bitset */ \
	while (_i-- > 0) { \
		/* 第_i个整数（bitset） */ \
		zend_ulong _x = _set[_i]; \
		/* 如果它不是0 */ \
		if (_x) { \
			/* 要检查的位序号。块序号从0开始，所以要加1。最后-1也是因为位序号从0开始。 */ \
			(bit) = ZEND_BITSET_ELM_SIZE * 8 * (_i + 1) - 1; \
			/* 只要左面还有1，就一直右移。bit记录最小的检查序号 */ \
			for (; _x != 0; _x <<= Z_UL(1), (bit)--) { \
				/* 如果位是0，下一个位。这样后面就可以插入有效情况的业务逻辑了 */ \
				if (!(_x & _test)) continue; \

// ing4, 遍历 bitset 的结尾语句。这方法真是又傻又牛~
#define ZEND_BITSET_FOREACH_END() \
			} \
		} \
	} \
} while (0)

// ing4, 把最右一个1，设置成0，返回它的位置
static inline int zend_bitset_pop_first(zend_bitset set, uint32_t len) {
	// 找到最右一个1的位置
	int i = zend_bitset_first(set, len);
	// 如果有效（0也有效）
	if (i >= 0) {
		// 把它设置成0
		zend_bitset_excl(set, i);
	}
	// 返回这个位置
	return i;
}

#endif /* _ZEND_BITSET_H_ */
