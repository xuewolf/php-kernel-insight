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

// malloc 主要是在创建hashTable的时候用到
// emalloc 在代码中出现的也不是特别多


// 叕是个含金量极高的东东
/*
   zend_alloc 是为PHP设计的，对现代cpu缓存友好的内存管理工具。大部分构思来源于 jemalloc 和 tcmalloc 的实现。
   
 * zend_alloc is designed to be a modern CPU cache friendly memory manager
 * for PHP. Most ideas are taken from jemalloc and tcmalloc implementations.
 *
   所有的分配被划分成3种
   
 * All allocations are split into 3 categories:
 *
   巨大  - 尺寸比 CHUNK（默认2M）更大，使用 mmap() 来进行分配。分配结果在内存中对齐到2M（大小是2M的倍数）。
   
 * Huge  - the size is greater than CHUNK size (~2M by default), allocation is
 *         performed using mmap(). The result is aligned on 2M boundary.
 *
   大的  - 每个 CHUNK 包含一串 4k 大小的page。大块的内存块总是对齐到 page（内存页）边界（大小是page的倍数）。
   （这里应该是 4096B，参见 _zend_mm_heap 上面的注释）
 * Large - a number of 4096K pages inside a CHUNK. Large blocks
 *         are always aligned on page boundary.
 *
   小的  - 小于page大小的3/4（3k）。小尺寸向上获取最接近的预定义数字的值。（在 ZEND_MM_BINS_INFO 里共有30个预定义的数字：8, 16, 24, 32, ... 3072。）
		小块内存是通过RUN分配的。每个RUN被分配成一个单独的或一些连续的page。
		   每个RUN里的空间被分配成一串空间的元素链表，占用的总空间是8 byte的倍数。
		   
 * Small - less than 3/4 of page size. Small sizes are rounded up to nearest
 *         greater predefined small size (there are 30 predefined sizes:
 *         8, 16, 24, 32, ... 3072). Small blocks are allocated from
 *         RUNs. Each RUN is allocated as a single or few following pages.
 *         Allocation inside RUNs implemented using linked list of free
 *         elements. The result is aligned to 8 bytes.
 *
   zend_alloc 通过CHUNK从操作系统中分配内存。这些CHUNK和巨大的内存块总是与CHUNK对齐（大小是CHUNK的倍数）。
   所以定位包含特定指针的 CHUNK 总是很方便。普通CHUNK的开头保留了一个用于途特殊用途的单独page。
   用于存放空闲page的bitset、让预定义的小尺寸有效运行的bitset、保存CHIUNK中每个page使用信息的 page 映射表等等。
   
 * zend_alloc allocates memory from OS by CHUNKs, these CHUNKs and huge memory
 * blocks are always aligned to CHUNK boundary. So it's very easy to determine
 * the CHUNK owning the certain pointer. Regular CHUNKs reserve a single
 * page at start for special purpose. It contains bitset of free pages,
 * few bitset for available runs of predefined small sizes, map of pages that
 * keeps information about usage of each page in this CHUNK, etc.
 *
   zend_alloc 提供类似 emalloc/efree/erealloc 的API，
   但它还提供了专用的和优化的程序来分配预定义大小的内存块。
   （例如 emalloc_2(), emallc_4(), ..., emalloc_large() 等）
   当需要的内存大小已知时，这个类库使用 C 语言预处理器技术和更多的专用方法 来代替调用 emalloc() 。
   
 * zend_alloc provides familiar emalloc/efree/erealloc API, but in addition it
 * provides specialized and optimized routines to allocate blocks of predefined
 * sizes (e.g. emalloc_2(), emallc_4(), ..., emalloc_large(), etc)
 * The library uses C preprocessor tricks that substitute calls to emalloc()
 * with more specialized routines when the requested size is known.
 */

#include "zend.h"
#include "zend_alloc.h"
#include "zend_globals.h"
#include "zend_operators.h"
#include "zend_multiply.h"
#include "zend_bitset.h"
#include "zend_mmap.h"
#include <signal.h>

// 定义 usleep方法
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

// 如果是window操作系统 
#ifdef ZEND_WIN32
# include <wincrypt.h>
# include <process.h>
# include "win32/winutil.h"
#endif

// c语言标准库
#include <stdio.h>
#include <stdlib.h>
// string 库
#include <string.h>

//
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <fcntl.h>
#include <errno.h>

// 如果不是windows系统 
#ifndef _WIN32
# include <sys/mman.h>
	// 如果有 MAP_ANON
# ifndef MAP_ANON
		// 如果有 MAP_ANONYMOUS
#  ifdef MAP_ANONYMOUS
			// 
#   define MAP_ANON MAP_ANONYMOUS
#  endif
# endif

	// 如果 没有 MAP_FAILED
# ifndef MAP_FAILED
		// 返回错误的内存地址
#  define MAP_FAILED ((void*)-1)
# endif

	// ?
# ifndef MAP_POPULATE
#  define MAP_POPULATE 0
# endif

		// ?
#  if defined(_SC_PAGESIZE) || (_SC_PAGE_SIZE)
#    define REAL_PAGE_SIZE _real_page_size
static size_t _real_page_size = ZEND_MM_PAGE_SIZE;
#  endif
# ifdef MAP_ALIGNED_SUPER
#    define MAP_HUGETLB MAP_ALIGNED_SUPER
# endif
#endif

// #define ZEND_MM_PAGE_SIZE  (4 * 1024) 
#ifndef REAL_PAGE_SIZE
# define REAL_PAGE_SIZE ZEND_MM_PAGE_SIZE
#endif

// NetBSD系统有一个 mremap 函数，有一个与linux不兼容的信号（哈哈），所以伪装这个信号，不退出
/* NetBSD has an mremap() function with a signature that is incompatible with Linux (WTF?),
 * so pretend it doesn't exist. */
// 如果不是linux操作系统 
#ifndef __linux__
# undef HAVE_MREMAP
#endif

// 如果不是苹果操作系统
#ifndef __APPLE__
# define ZEND_MM_FD -1
// 是苹果操作系统
#else
# include <mach/vm_statistics.h>
// Mac操作系统允许 追踪匿名page,通过 每个Tag id的 vmmap
// 用户的应用程序？允许使用 240-225这些 ID
/* Mac allows to track anonymous page via vmmap per TAG id.
 * user land applications are allowed to take from 240 to 255.
 */
# define ZEND_MM_FD VM_MAKE_TAG(250U)
#endif


#ifndef ZEND_MM_STAT
// 追踪当前和高峰时内存使用情况
# define ZEND_MM_STAT 1    /* track current and peak memory usage            */
#endif

#ifndef ZEND_MM_LIMIT
// 支持用户定义内存上限
# define ZEND_MM_LIMIT 1   /* support for user-defined memory limit          */
#endif

#ifndef ZEND_MM_CUSTOM
// 支持自定义分配内存
# define ZEND_MM_CUSTOM 1  /* support for custom memory allocator            */
							// 当 USE_ZEND_ALLOC=0 时，使用系统分配内存 malloc
                           /* USE_ZEND_ALLOC=0 may switch to system malloc() */
#endif

#ifndef ZEND_MM_STORAGE
// 支持自定义内存存储
# define ZEND_MM_STORAGE 1 /* support for custom memory storage              */
#endif

#ifndef ZEND_MM_ERROR
// 上报系统异常
# define ZEND_MM_ERROR 1   /* report system errors                           */
#endif

#ifndef ZEND_MM_CHECK
// ing4, 内存异常检测方法，如果不符合条件，报内存异常并退出 
// 写do是为了要这个括号吗？因为宏定义没有括号，不方便阅读
# define ZEND_MM_CHECK(condition, message)  do { \
		if (UNEXPECTED(!(condition))) { \
			zend_mm_panic(message); \
		} \
	} while (0)
#endif
// 32位整数 
typedef uint32_t   zend_mm_page_info; /* 4-byte integer */
// 32或64位整数 
typedef zend_ulong zend_mm_bitset;    /* 4-byte or 8-byte integer */

// ing4,  计算 size（通常是指针） 到指定块大小的偏移量。 内部大量调用
// size_t 是把指针转成整数吗？ 为什么不用模（%）运算呢？想当然是因为位运算更快
#define ZEND_MM_ALIGNED_OFFSET(size, alignment) \
	(((size_t)(size)) & ((alignment) - 1))
// size % alignment

// ing4, 把 size 向左对齐到 alignment，也是取回所在 alignment 的指针
#define ZEND_MM_ALIGNED_BASE(size, alignment) \
	(((size_t)(size)) & ~((alignment) - 1))

// clear, 计算一个 size 是多少个 alignment，除不尽的向前进1
#define ZEND_MM_SIZE_TO_NUM(size, alignment) \
	(((size_t)(size) + ((alignment) - 1)) / (alignment))

// BITSET 的长度 64
#define ZEND_MM_BITSET_LEN		(sizeof(zend_mm_bitset) * 8)       /* 32 or 64 */
// PAGE_MAP 的长度 512 / 64 = 8
#define ZEND_MM_PAGE_MAP_LEN	(ZEND_MM_PAGES / ZEND_MM_BITSET_LEN) /* 16 or 8 */

// 8Byte * 8 = 64Bytes, 512个bit
typedef zend_mm_bitset zend_mm_page_map[ZEND_MM_PAGE_MAP_LEN];     /* 64B */

// free 标记，没有用到
#define ZEND_MM_IS_FRUN                  0x00000000
// large 块标记，0100 + 4*7=28 个0
#define ZEND_MM_IS_LRUN                  0x40000000
// small 块标记，1000 + 4*7=28 个0
#define ZEND_MM_IS_SRUN                  0x80000000

// large mask, 右侧 10个1, 最大1023。因为实际最大值是511，9个1值为511，不够用
// （first_page 必须是1，如果是0，就无法分辨它是第一个，还是不带序号）
#define ZEND_MM_LRUN_PAGES_MASK          0x000003ff
// 
#define ZEND_MM_LRUN_PAGES_OFFSET        0

// small mask, 右侧 5个1，最大31
#define ZEND_MM_SRUN_BIN_NUM_MASK        0x0000001f
// 成串的 pages 中存储块号的位置，默认在最后，最大值是7
#define ZEND_MM_SRUN_BIN_NUM_OFFSET      0

/* 
关于掩码到底是做什么用的，它的用途是过滤掉可能产生干扰的部分，只留下有用的部分
例如:
  掩码 0011 ，传入变量 1001 ，被过滤（0011 & 1001）后变成 0001  
*/

// 00000001 11111111 00000000 00000000，最大511
#define ZEND_MM_SRUN_FREE_COUNTER_MASK   0x01ff0000
// COUNTER 转成有效数字的偏移位数
#define ZEND_MM_SRUN_FREE_COUNTER_OFFSET 16

// 00000001 11111111 00000000 00000000，最大511
#define ZEND_MM_NRUN_OFFSET_MASK         0x01ff0000
// OFFSET 转成有效数字的偏移位数
#define ZEND_MM_NRUN_OFFSET_OFFSET       16

// ZEND_MM_LRUN_PAGES_OFFSET = 0
// ing4, 取回large块信息
#define ZEND_MM_LRUN_PAGES(info)         (((info) & ZEND_MM_LRUN_PAGES_MASK) >> ZEND_MM_LRUN_PAGES_OFFSET)
// clear, 取得当前 page 在 ZEND_MM_BINS_INFO 中的配置编号，最大31( 实际上最大30）
#define ZEND_MM_SRUN_BIN_NUM(info)       (((info) & ZEND_MM_SRUN_BIN_NUM_MASK) >> ZEND_MM_SRUN_BIN_NUM_OFFSET)
// ing4, 取回 COUNTER 数值（内存回收时临时用到，用完就清空了）
#define ZEND_MM_SRUN_FREE_COUNTER(info)  (((info) & ZEND_MM_SRUN_FREE_COUNTER_MASK) >> ZEND_MM_SRUN_FREE_COUNTER_OFFSET)
// clear, 一组small 块组成的large块（page数肯定大于1），反回page在组里的序号
#define ZEND_MM_NRUN_OFFSET(info)        (((info) & ZEND_MM_NRUN_OFFSET_MASK) >> ZEND_MM_NRUN_OFFSET_OFFSET)

// clear, 返回 0。这个宏全局没有用到
#define ZEND_MM_FRUN()                   ZEND_MM_IS_FRUN
// ing4, 添加 large 标记 ： 0x40000000 | count。 count 是pages 在large块里的序号
#define ZEND_MM_LRUN(count)              (ZEND_MM_IS_LRUN | ((count) << ZEND_MM_LRUN_PAGES_OFFSET))
// ing4, 记录配置编号 并 添加 small, SRUN 标记 ： 0x80000000 | bin_num 
#define ZEND_MM_SRUN(bin_num)            (ZEND_MM_IS_SRUN | ((bin_num) << ZEND_MM_SRUN_BIN_NUM_OFFSET))
// clear, small 标记，bin_num 是这组块在 ZEND_MM_BINS_INFO 中的配置序号 ,（内存回收时临时用到，用完就清空了）
#define ZEND_MM_SRUN_EX(bin_num, count)  (ZEND_MM_IS_SRUN | ((bin_num) << ZEND_MM_SRUN_BIN_NUM_OFFSET) | ((count) << ZEND_MM_SRUN_FREE_COUNTER_OFFSET))
// clear, 添加 large，small 标记 ：0x80000000 | 0x40000000 | bin_num | offset << 16
// bin_num 是这组块在 ZEND_MM_BINS_INFO 中的配置序号， offset 这个page在这组page中的序号。
// 成串的 存放 small 块 pages，即是 small 又是 large。 适用于分配 ZEND_MM_BINS_INFO 中 pages 数大于1的块。
#define ZEND_MM_NRUN(bin_num, offset)    (ZEND_MM_IS_SRUN | ZEND_MM_IS_LRUN | ((bin_num) << ZEND_MM_SRUN_BIN_NUM_OFFSET) | ((offset) << ZEND_MM_NRUN_OFFSET_OFFSET))

// 默认内存块数量 
#define ZEND_MM_BINS 30

typedef struct  _zend_mm_page      zend_mm_page;
typedef struct  _zend_mm_bin       zend_mm_bin;
typedef struct  _zend_mm_free_slot zend_mm_free_slot;
typedef struct  _zend_mm_chunk     zend_mm_chunk;
typedef struct  _zend_mm_huge_list zend_mm_huge_list;

// 不使用 巨大的page
static bool zend_mm_use_huge_pages = false;

/*
   内存在操作系统中被修正成 2M 大小的 chunk。
   内部 chunk 不被划分成4096B 的page。所以每个chunk由512个page组成
   每个 chunk 的第一个 page 保留用来当作 chunk 头信息，里面包含所有page的服务信息。
 * Memory is retrieved from OS by chunks of fixed size 2MB.
 * Inside chunk it's managed by pages of fixed size 4096B.
 * So each chunk consists from 512 pages.
 * The first page of each chunk is reserved for chunk header.
 * It contains service information about all pages.
 *
   free_pages - 这个chunk里当前的空闲page数量 
 * free_pages - current number of free pages in this chunk
 *
   free_tail  - 结尾处的连续空闲page
 * free_tail  - number of continuous free pages at the end of chunk
 *
   free_map   - 字节集（每个page一个字节）. 记录对应的page是否被分配了。
                分配器处理“大尺寸”（超过1个page可以叫做大尺寸了？）时，通过查找 0 位，
				可以容易找到空闲的page（或一组连续的page）。
				
 * free_map   - bitset (a bit for each page). The bit is set if the corresponding
 *              page is allocated. Allocator for "large sizes" may easily find a
 *              free page (or a continuous number of pages) searching for zero
 *              bits.
 *
   map		  - 包含每个page的服务信息（每page32 位）
 * map        - contains service information for each page. (32-bits for each
 *              page).
      使用说明：
 *    usage:
				占 2 个位
 *				(2 bits)
				FRUN - 空闲 page
 * 				FRUN - free page,

				LRUN - 大page 的第一 “large” page
 *              LRUN - first page of "large" allocation

				SRUN - 一用于存放 “small” 的块的第一个page
 *              SRUN - first page of a bin used for "small" allocation
 *
 *    lrun_pages:
				(10个bit) 已分配的page
 *              (10 bits) number of allocated pages
 *
 *    srun_bin_num:
				(5 位) 块 （例如 ... 参考 zend_alloc_sizes.h）
 *              (5 bits) bin number (e.g. 0 for sizes 0-2, 1 for 3-4,
 *               2 for 5-8, 3 for 9-16 etc) see zend_alloc_sizes.h
 */

// 内存堆, 主要在分配内存时使用，调试时少量使用。
struct _zend_mm_heap {
#if ZEND_MM_CUSTOM
	//
	int                use_custom_heap;
#endif

//
#if ZEND_MM_STORAGE
	//
	zend_mm_storage   *storage;
#endif

//
#if ZEND_MM_STAT
	// 当前内存使用量
	size_t             size;                    /* current memory usage */
	// 峰值内存使用量
	size_t             peak;                    /* peak memory usage */
#endif
	// 小尺寸用的空闲列表，默认30个 , ZEND_MM_BINS=30
	// 这里的 30 对应 ZEND_MM_BINS_INFO 的 30 个配置
	zend_mm_free_slot *free_slot[ZEND_MM_BINS]; /* free lists for small sizes */
#if ZEND_MM_STAT || ZEND_MM_LIMIT
	// 当前分配的pages数量
	size_t             real_size;               /* current size of allocated pages */
#endif
#if ZEND_MM_STAT
	// 分配的pages最大数量 
	size_t             real_peak;               /* peak size of allocated pages */
#endif
#if ZEND_MM_LIMIT
	// 内存限制
	size_t             limit;                   /* memory limit */
	// 溢出标记
	int                overflow;                /* memory overflow flag */
#endif

	// huge块有个单独的列表
	zend_mm_huge_list *huge_list;               /* list of huge allocated blocks */
	// large,small 都在 chunk里
	zend_mm_chunk     *main_chunk;
	// 使用的 chunks 列表
	zend_mm_chunk     *cached_chunks;			/* list of unused chunks */
	// 已分配的 chunks 数量 
	int                chunks_count;			/* number of allocated chunks */
	// 当前请求使用的chunks最大数量
	int                peak_chunks_count;		/* peak number of allocated chunks for current request */
	// 缓存的 chunks 数量 
	int                cached_chunks_count;		/* number of cached chunks */
	// 每个request 平均使用的 chunks 数量 
	double             avg_chunks_count;		/* average number of chunks allocated per request */
	// 最后一次删除后的chunk数量 
	int                last_chunks_delete_boundary; /* number of chunks after last deletion */
	// 删除的超出最后一条边界的 chunks 数量 
	int                last_chunks_delete_count;    /* number of deletion over the last boundary */
#if ZEND_MM_CUSTOM
	union {
		// 正式环境
		struct {
			// 分配内存方法
			void      *(*_malloc)(size_t);
			// 释放内存方法
			void       (*_free)(void*);
			// 调整内存大小方法
			void      *(*_realloc)(void*, size_t);
		} std;
		// 调试环境副加的
		struct {
			// 
			void      *(*_malloc)(size_t ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC);
			//
			void       (*_free)(void*  ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC);
			// 
			void      *(*_realloc)(void*, size_t  ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC);
		} debug;
	} custom_heap;
	// 
	HashTable *tracked_allocs;
#endif
};

// 内存 chunk。根也是一个chunk。结构体信息都保存在chunk的第一个page里
struct _zend_mm_chunk {
	// 所属的 heap
	zend_mm_heap      *heap;
	// 下一个 chunk
	zend_mm_chunk     *next;
	// 上一个 chunk
	zend_mm_chunk     *prev;
	// 空 pages
	uint32_t           free_pages;				/* number of free pages */
	// chunk末尾的空 pages串的开头页码 ，不是数量（这个注释写的不太清楚）
	uint32_t           free_tail;               /* number of free pages at the end of chunk */
	// chunk的创建序号
	uint32_t           num;
	// 这个没有用到
	char               reserve[64 - (sizeof(void*) * 3 + sizeof(uint32_t) * 3)];
	// 这里自带一个 zend_mm_heap，而不是指针。 只有在根chunk里会用到
	zend_mm_heap       heap_slot;               /* used only in main chunk */
	// 512 bits 等于 64 bytes。
	zend_mm_page_map   free_map;                /* 512 bits or 64 bytes */
	// 地图有 512 页, 每页是一个32位整数，4Byte，共2K
	zend_mm_page_info  map[ZEND_MM_PAGES];      /* 2 KB = 512 * 4 */
};

// 内存 page
struct _zend_mm_page {
	// 4k
	char               bytes[ZEND_MM_PAGE_SIZE];
};

/*
   一个bin 是一个或一组连续的pages（最多8个?）用于分配一个特定的 “small size”
 * bin - is one or few continuous pages (up to 8) used for allocation of
 * a particular "small size".
 */
struct _zend_mm_bin {
	// 4k*8
	char               bytes[ZEND_MM_PAGE_SIZE * 8];
};

// 空闲内存位置， 多包装一层 是为了方便地指向同类原素 
struct _zend_mm_free_slot {
	zend_mm_free_slot *next_free_slot;
};

// huge块列表（串）
struct _zend_mm_huge_list {
	void              *ptr;
	size_t             size;
	// 指向下一个
	zend_mm_huge_list *next;
// 调试模式
#if ZEND_DEBUG
	// 调试信息
	zend_mm_debug_info dbg;
#endif
};

// ing4, 找到 chunk 中指定序号的 page
#define ZEND_MM_PAGE_ADDR(chunk, page_num) \
	((void*)(((zend_mm_page*)(chunk)) + (page_num)))

// clear, 取得 ZEND_MM_BINS_INFO 的 size列（第二列）
#define _BIN_DATA_SIZE(num, size, elements, pages, x, y) size,
// ZEND_MM_BINS_INFO中的 size列 
static const uint32_t bin_data_size[] = {
	ZEND_MM_BINS_INFO(_BIN_DATA_SIZE, x, y)
};

// clear, 取得 ZEND_MM_BINS_INFO 的 elements列（第三列）
#define _BIN_DATA_ELEMENTS(num, size, elements, pages, x, y) elements,
// ZEND_MM_BINS_INFO中的 elements列 
static const uint32_t bin_elements[] = {
	ZEND_MM_BINS_INFO(_BIN_DATA_ELEMENTS, x, y)
};

// clear, 取得 ZEND_MM_BINS_INFO 的 pages列（第四列）
#define _BIN_DATA_PAGES(num, size, elements, pages, x, y) pages,
// ZEND_MM_BINS_INFO中的 pages列 
static const uint32_t bin_pages[] = {
	ZEND_MM_BINS_INFO(_BIN_DATA_PAGES, x, y)
};

// 调试模式
#if ZEND_DEBUG
// ing4, 输出内存中的内容
ZEND_COLD void zend_debug_alloc_output(char *format, ...)
{
	char output_buf[256];
	va_list args;

	// 遍历后面的参数并输出
	va_start(args, format);
	vsprintf(output_buf, format, args);
	va_end(args);

// window操作系统 用 OutputDebugString，其他操作系统用 fprintf 输入文本
#ifdef ZEND_WIN32
	OutputDebugString(output_buf);
#else
	fprintf(stderr, "%s", output_buf);
#endif
}
#endif

// ing3, 致命问题，中断执行。ZEND_MM_CHECK 一次调用
static ZEND_COLD ZEND_NORETURN void zend_mm_panic(const char *message)
{
	// 打印内容
	fprintf(stderr, "%s\n", message);
/* See http://support.microsoft.com/kb/190351 */

// 如果是windows系统
#ifdef ZEND_WIN32
	fflush(stderr);
#endif

// 如果是调试模式 并且 定义了 HAVE_KILL 和 HAVE_GETPID
#if ZEND_DEBUG && defined(HAVE_KILL) && defined(HAVE_GETPID)
	// 杀死进程 _getpid
	kill(getpid(), SIGSEGV);
#endif
	// 中断执行
	abort();
}

// ing3, 安全异常
static ZEND_COLD ZEND_NORETURN void zend_mm_safe_error(zend_mm_heap *heap,
	const char *format,
	size_t limit,
// 调试模式, 多两个参数，文件名和行
#if ZEND_DEBUG
	const char *filename,
	uint32_t lineno,
#endif
	size_t size)
{

	heap->overflow = 1;
	// 
	zend_try {
		// 报错
		zend_error_noreturn(E_ERROR,
			format,
			limit,
#if ZEND_DEBUG
			filename,
			lineno,
#endif
			size);
	} zend_catch {
		// 忽略其他异常
	}  zend_end_try();
	// 没有溢出
	heap->overflow = 0;
	// _zend_bailout：zend.c
	zend_bailout();
	// 退出程序 
	exit(1);
}

// 如果是windows系统 
#ifdef _WIN32
// ing3, 用stderr 输出最后一个 error
void
stderr_last_error(char *msg)
{
	
	DWORD err = GetLastError();
	// 
	char *buf = php_win32_error_to_msg(err);

	if (!buf[0]) {
		fprintf(stderr, "\n%s: [0x%08lx]\n", msg, err);
	}
	else {
		fprintf(stderr, "\n%s: [0x%08lx] %s\n", msg, err, buf);
	}

	php_win32_error_msg_free(buf);
}
#endif

// 操作系统分配
/*****************/
/* OS Allocation */
/*****************/

// ing3, 释放内存, 传入地址和大小。addr 必须是一块内存的起始位置
static void zend_mm_munmap(void *addr, size_t size)
{
// win32 系统，条件层数 0
#ifdef _WIN32
	// 释放虚拟内存
	if (VirtualFree(addr, 0, MEM_RELEASE) == 0) {
		// 如果addr不是某一块的起始地址，应该抛异常 ERROR_INVALID_ADDRESS
		/** ERROR_INVALID_ADDRESS is expected when addr is not range start address */
		if (GetLastError() != ERROR_INVALID_ADDRESS) {
// define ZEND_MM_ERROR 1 /* report system errors */
#if ZEND_MM_ERROR
			// stderr 输出错误信息
			stderr_last_error("VirtualFree() failed");
#endif
			return;
		}
		// 清空系统异常？
		SetLastError(0);

		MEMORY_BASIC_INFORMATION mbi;
		// 如果查询不到这块内存
		if (VirtualQuery(addr, &mbi, sizeof(mbi)) == 0) {
#if ZEND_MM_ERROR
			// 报错 
			stderr_last_error("VirtualQuery() failed");
#endif
			// 中断
			return;
		}
		// 
		addr = mbi.AllocationBase;

		// 再次尝试释放，如果失败
		if (VirtualFree(addr, 0, MEM_RELEASE) == 0) {
#if ZEND_MM_ERROR
			// 报错
			stderr_last_error("VirtualFree() failed");
#endif
		}
	}
// 如果不是window操作系统 ，条件层数 0
#else
	// 如果调用 munmap() 释放内存失败
	if (munmap(addr, size) != 0) {
#if ZEND_MM_ERROR
		// 报错
		fprintf(stderr, "\nmunmap() failed: [%d] %s\n", errno, strerror(errno));
#endif
	}
// 条件层数 0
#endif
}

#ifndef HAVE_MREMAP
// ing3, 调整内存大小
static void *zend_mm_mmap_fixed(void *addr, size_t size)
{
// 如果是window操作系统
#ifdef _WIN32
	// 尝试调整内存大小
	void *ptr = VirtualAlloc(addr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

	// 如果分调整失败
	if (ptr == NULL) {
		// 如果这块地址不是空的，应该抛异常 ERROR_INVALID_ADDRESS
		/** ERROR_INVALID_ADDRESS is expected when fixed addr range is not free */
		if (GetLastError() != ERROR_INVALID_ADDRESS) {
#if ZEND_MM_ERROR
			// 报错
			stderr_last_error("VirtualAlloc() fixed failed");
#endif
		}
		// 清空系统异常？
		SetLastError(0);
		return NULL;
	}
	// 调整前后的地址要相同
	ZEND_ASSERT(ptr == addr);
	// 返回地址
	return ptr;
// 如果不是window系统 
#else
	int flags = MAP_PRIVATE | MAP_ANON;
// 系统常量
#if defined(MAP_EXCL)
	flags |= MAP_FIXED | MAP_EXCL;
// 系统常量
#elif defined(MAP_TRYFIXED)
	flags |= MAP_TRYFIXED;
#endif
	// MAP_FIXED 模式会抛弃原来的映射，所以不能用
	/* MAP_FIXED leads to discarding of the old mapping, so it can't be used. */
	// 调整
	void *ptr = mmap(addr, size, PROT_READ | PROT_WRITE, flags /*| MAP_POPULATE | MAP_HUGETLB*/, ZEND_MM_FD, 0);

	// 如果调整失败
	if (ptr == MAP_FAILED) {
// 如果有定义（默认有） ZEND_MM_ERROR 并且没有 MAP_EXCL，MAP_TRYFIXED
#if ZEND_MM_ERROR && !defined(MAP_EXCL) && !defined(MAP_TRYFIXED)
		// 输出错误信息
		fprintf(stderr, "\nmmap() fixed failed: [%d] %s\n", errno, strerror(errno));
#endif
		return NULL;
	// 如果新旧地址不同
	} else if (ptr != addr) {
		// 释放新分配的内存
		zend_mm_munmap(ptr, size);
		// 返回空
		return NULL;
	}
	// 如果没有失败并且新旧地址相同
	return ptr;
#endif
}
#endif

// ing3, 分配新的内存
static void *zend_mm_mmap(size_t size)
{
// 如果是windows系统
#ifdef _WIN32
	// 分配指定大小的内存 
	void *ptr = VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	// 如果分配失败
	if (ptr == NULL) {
#if ZEND_MM_ERROR
		// 报错
		stderr_last_error("VirtualAlloc() failed");
#endif
		return NULL;
	}
	return ptr;
// 如果不是windows系统 
#else
	void *ptr;

#if defined(MAP_HUGETLB) || defined(VM_FLAGS_SUPERPAGE_SIZE_2MB)
	// 如果开始了 zend_mm_use_huge_pages 并且 大小是 CHUNK 大小
	if (zend_mm_use_huge_pages && size == ZEND_MM_CHUNK_SIZE) {
		int fd = -1;
		int mflags = MAP_PRIVATE | MAP_ANON;
// # define MAP_HUGETLB MAP_ALIGNED_SUPER，MAP_ALIGNED_SUPER是系统变量？
#if defined(MAP_HUGETLB)
		mflags |= MAP_HUGETLB;
#else
		fd = VM_FLAGS_SUPERPAGE_SIZE_2MB;
#endif
		// 分配新内存空间，mflags，fd 是什么？
		ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, mflags, fd, 0);
		// 如果分配成功
		if (ptr != MAP_FAILED) {
			// 给内存设置名称
			zend_mmap_set_name(ptr, size, "zend_alloc");
			// 返回
			return ptr;
		}
		// 如果失败下面会再尝试
	}

#endif

	// 分配新内存空间
	ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, ZEND_MM_FD, 0);
	// 如果分配失败
	if (ptr == MAP_FAILED) {
#if ZEND_MM_ERROR
		// 报错分配失败
		fprintf(stderr, "\nmmap() failed: [%d] %s\n", errno, strerror(errno));
#endif
		return NULL;
	}
	// 给内存设置名称
	zend_mmap_set_name(ptr, size, "zend_alloc");
	return ptr;
#endif
}

// bit 集合操作
/***********/
/* Bitmask */
/***********/

// ing3, 计算zend_mm_bitset右侧连续的值为1的bit的数量 
// zend_mm_bitset， 32或64位整数，typedef zend_ulong zend_mm_bitset
/* number of trailing set (1) bits */
static zend_always_inline int zend_mm_bitset_nts(zend_mm_bitset bitset)
{
// gunc 采用内置函数操作
#if (defined(__GNUC__) || __has_builtin(__builtin_ctzl)) && SIZEOF_ZEND_LONG == SIZEOF_LONG && defined(PHP_HAVE_BUILTIN_CTZL)
	return __builtin_ctzl(~bitset);
// 情况2 gunc 采用内置函数操作
#elif (defined(__GNUC__) || __has_builtin(__builtin_ctzll)) && defined(PHP_HAVE_BUILTIN_CTZLL)
	return __builtin_ctzll(~bitset);
// 情况3 window操作系统
#elif defined(_WIN32)
	unsigned long index;

// 64位windows
#if defined(_WIN64)
	//  扫描bitset 64位
	if (!BitScanForward64(&index, ~bitset)) {
// 其他操作系统 
#else
	// 扫描bitset
	if (!BitScanForward(&index, ~bitset)) {
#endif
		// 未定义行为？
		/* undefined behavior */
		return 32;
	}
	// 结果强转成int型
	return (int)index;
	
// 情况4， 不是window操作系统 ，这里有具体的业务逻辑
#else
	int n;

	// 如果是64个1，返回 ZEND_MM_BITSET_LEN，32 或 64
	if (bitset == (zend_mm_bitset)-1) return ZEND_MM_BITSET_LEN;

	n = 0;
	
	// 如果是64位系统
#if SIZEOF_ZEND_LONG == 8
	// 2分法计算，如果是64位，要加这个业务逻辑，和下面的32位操作完全一致
	if (sizeof(zend_mm_bitset) == 8) {
		// 如果结尾是32个1, 先右移32位
		if ((bitset & 0xffffffff) == 0xffffffff) {n += 32; bitset = bitset >> Z_UL(32);}
		// 这里n已经是 32了
	}	
#endif

	// 最后 16个1 // 1111 1111 1111 1111	
	if ((bitset & 0x0000ffff) == 0x0000ffff) {n += 16; bitset = bitset >> 16;}
	// 最后 8个1 
	if ((bitset & 0x000000ff) == 0x000000ff) {n +=  8; bitset = bitset >>  8;}
	// 最后 4个1
	if ((bitset & 0x0000000f) == 0x0000000f) {n +=  4; bitset = bitset >>  4;}
	// 最后 2个1
	if ((bitset & 0x00000003) == 0x00000003) {n +=  2; bitset = bitset >>  2;}
	// 最后 1个1 ，// 最大返回63
	return n + (bitset & 1);
#endif
}

// clear, 验证某一个位是否是 1
static zend_always_inline int zend_mm_bitset_is_set(zend_mm_bitset *bitset, int bit)
{
	return ZEND_BIT_TEST(bitset, bit);
}

// clear, 把某一位设置成 1 
static zend_always_inline void zend_mm_bitset_set_bit(zend_mm_bitset *bitset, int bit)
{
	// ZEND_MM_BITSET_LEN 32或64, Z_UL 转成无符号 long型
	bitset[bit / ZEND_MM_BITSET_LEN] |= (Z_UL(1) << (bit & (ZEND_MM_BITSET_LEN-1)));
	// 以64位操作系统为例
	// bitset[指定的块] |= 1 << bit & 63
	// 必须先找到指定的块再更新，因为 或(|)运算的操作对象是 长整形，固定8字节。
	// bit 是位数，所以用位移操作是最方便的。
}

// clear, 把某一位设置成 0 
static zend_always_inline void zend_mm_bitset_reset_bit(zend_mm_bitset *bitset, int bit)
{
	// ZEND_MM_BITSET_LEN 32或64
	bitset[bit / ZEND_MM_BITSET_LEN] &= ~(Z_UL(1) << (bit & (ZEND_MM_BITSET_LEN-1)));
	// bitset[指定的块] &= ~(1 << bit % 64)
	// 二进制反(~)操作 把 1000 换成 0111，再用并(&)操作 ，厉害！
}

// clear, 把 bitset的一个区域设置成 1
static zend_always_inline void zend_mm_bitset_set_range(zend_mm_bitset *bitset, int start, int len)
{
	// 如果长度是1
	if (len == 1) {
		zend_mm_bitset_set_bit(bitset, start);
	// 如果长度不是1
	} else {
		// 取得要标记的 块 的位置，每个块 ZEND_MM_BITSET_LEN 32或64
		int pos = start / ZEND_MM_BITSET_LEN;
		// 结束的 块 位置
		int end = (start + len - 1) / ZEND_MM_BITSET_LEN;
		// start % 64
		int bit = start & (ZEND_MM_BITSET_LEN - 1);
		// 
		zend_mm_bitset tmp;

		// 开始位置和结束位置不在同个块里
		if (pos != end) {
			// 设置从 bit 到 本块结束 这段位置
			/* set bits from "bit" to ZEND_MM_BITSET_LEN-1 */
			// 64个1， 左移bit个位置。这个是起始块，块右边的0和本次运算没关系，左边多出来的1，就是要设置的位置
			// 因为高位在左边低位在右边，位置也是从右向左数的，不是从左向右数，这一点千万要注意！
			tmp = (zend_mm_bitset)-1 << bit;
			// 设置开始段
			bitset[pos++] |= tmp;
			// 设置中间的块
			while (pos != end) {
				// 逐个设置所有的 块
				/* set all bits */
				// 每个值都是 64个1 
				bitset[pos++] = (zend_mm_bitset)-1;
			}
			// (start + len - 1) % 64 ，取得结束位置在 本块中的位置
			end = (start + len - 1) & (ZEND_MM_BITSET_LEN - 1);
			// 设置从0到结束位置
			/* set bits from "0" to "end" */
			// 64个1 右移 63-1-end 次， 块左边的0和本次运算没关系，右边多出来的1，就是要设置的位置
			tmp = (zend_mm_bitset)-1 >> ((ZEND_MM_BITSET_LEN - 1) - end);
			// 设置结束段
			bitset[pos] |= tmp;
		// 开始和结束位置在同一个块里
		} else {
			// (start + len - 1) % 64
			end = (start + len - 1) & (ZEND_MM_BITSET_LEN - 1);
			// 设置从开始到结束这一段的bit
			/* set bits from "bit" to "end" */
			// 右边多出来的位置设置成0
			tmp = (zend_mm_bitset)-1 << bit;
			// 左边多出来的位置设置成0。 这种计算碰到实际应用还要多练习
			tmp &= (zend_mm_bitset)-1 >> ((ZEND_MM_BITSET_LEN - 1) - end);
			// 写入到块中
			bitset[pos] |= tmp;
		}
	}
}

// clear, 把 bitset的一个区域设置成 0
static zend_always_inline void zend_mm_bitset_reset_range(zend_mm_bitset *bitset, int start, int len)
{
	if (len == 1) {
		zend_mm_bitset_reset_bit(bitset, start);
	} else {
		// 计算位置逻辑和上面相同
		// ZEND_MM_BITSET_LEN 32或64
		int pos = start / ZEND_MM_BITSET_LEN;
		int end = (start + len - 1) / ZEND_MM_BITSET_LEN;
		// start % 64
		int bit = start & (ZEND_MM_BITSET_LEN - 1);
		zend_mm_bitset tmp;

		if (pos != end) {
			// 重置从 bit 到 块结束这一块
			/* reset bits from "bit" to ZEND_MM_BITSET_LEN-1 */
			// 因为高位在左边低位在右边，位置也是从右向左数的，不是从左向右数，这一点千万要注意！
			tmp = ~((Z_UL(1) << bit) - 1);
			bitset[pos++] &= ~tmp;
			while (pos != end) {
				// 所有位都是 0
				/* set all bits */
				bitset[pos++] = 0;
			}
			end = (start + len - 1) & (ZEND_MM_BITSET_LEN - 1);
			/* reset bits from "0" to "end" */
			tmp = (zend_mm_bitset)-1 >> ((ZEND_MM_BITSET_LEN - 1) - end);
			bitset[pos] &= ~tmp;
		} else {
			end = (start + len - 1) & (ZEND_MM_BITSET_LEN - 1);
			/* reset bits from "bit" to "end" */
			tmp = (zend_mm_bitset)-1 << bit;
			tmp &= (zend_mm_bitset)-1 >> ((ZEND_MM_BITSET_LEN - 1) - end);
			bitset[pos] &= ~tmp;
		}
	}
}

// clear， 检验 bitset 的一个区域 是否是空的
static zend_always_inline int zend_mm_bitset_is_free_range(zend_mm_bitset *bitset, int start, int len)
{
	if (len == 1) {
		return !zend_mm_bitset_is_set(bitset, start);
	} else {
		// 业务逻辑与上面相似
		// ZEND_MM_BITSET_LEN 32或64
		int pos = start / ZEND_MM_BITSET_LEN;
		int end = (start + len - 1) / ZEND_MM_BITSET_LEN;
		int bit = start & (ZEND_MM_BITSET_LEN - 1);
		zend_mm_bitset tmp;
		// 如果开始和结束不在一个块里
		if (pos != end) {
			// 从bit到块结尾
			/* set bits from "bit" to ZEND_MM_BITSET_LEN-1 */
			tmp = (zend_mm_bitset)-1 << bit;
			// 如果有一个1，返回false
			if ((bitset[pos++] & tmp) != 0) {
				return 0;
			}
			// 检查中间每一个块
			while (pos != end) {
				// 如果有任何一个 bit 不是 0
				/* set all bits */
				if (bitset[pos++] != 0) {
					// 返回false
					return 0;
				}
			}
			// 检查结尾块
			end = (start + len - 1) & (ZEND_MM_BITSET_LEN - 1);
			/* set bits from "0" to "end" */
			tmp = (zend_mm_bitset)-1 >> ((ZEND_MM_BITSET_LEN - 1) - end);
			// 返回 是否全是 0
			return (bitset[pos] & tmp) == 0;
			
		// 如果开始和结束在一个块里
		} else {
			end = (start + len - 1) & (ZEND_MM_BITSET_LEN - 1);
			/* set bits from "bit" to "end" */
			tmp = (zend_mm_bitset)-1 << bit;
			tmp &= (zend_mm_bitset)-1 >> ((ZEND_MM_BITSET_LEN - 1) - end);
			// 返回 是否全是 0
			return (bitset[pos] & tmp) == 0;
		}
	}
}

/**********/
/* Chunks */
/**********/

// 检验平台是否支持 huge_pages？
static zend_always_inline void zend_mm_hugepage(void* ptr, size_t size)
{
// 环境变量
#if defined(MADV_HUGEPAGE)
	// what is it doing ?
	(void)madvise(ptr, size, MADV_HUGEPAGE);
// 
#elif defined(HAVE_MEMCNTL)
	struct memcntl_mha m = {.mha_cmd = MHA_MAPSIZE_VA, .mha_pagesize = ZEND_MM_CHUNK_SIZE, .mha_flags = 0};
	(void)memcntl(ptr, size, MC_HAT_ADVISE, (char *)&m, 0, 0);
//
#elif !defined(VM_FLAGS_SUPERPAGE_SIZE_2MB) && !defined(MAP_ALIGNED_SUPER)
	// 平台不支持 thp
	zend_error_noreturn(E_WARNING, "huge_pages: thp unsupported on this platform");
#endif
}

// ing3, 分配一个 chunk, 与 alignment 对齐的 chunk
static void *zend_mm_chunk_alloc_int(size_t size, size_t alignment)
{
	// 先分配一块 指定大小的 内存
	void *ptr = zend_mm_mmap(size);

	// 分配失败
	if (ptr == NULL) {
		return NULL;
	// 如果 偏移量为0（ptr 已经对齐到 alignment）
	} else if (ZEND_MM_ALIGNED_OFFSET(ptr, alignment) == 0) {
		// 如果使用 huge_pages
		if (zend_mm_use_huge_pages) {
			// 转成 huge_page ?
			zend_mm_hugepage(ptr, size);
		}
		return ptr;
	// 如果没有对齐
	} else {
		size_t offset;

		// chunk 必须要对齐
		/* chunk has to be aligned */
		// 释放这块内存
		zend_mm_munmap(ptr, size);
		// 重新分配一块内存，大小是：+alignment 是为了把最后一块补齐，然后减掉1个 page 
		// 这块分配的内存会比预期的 小最多 1个page
			// 这次分配的内存在windows里会再释放掉重新分配，这次分配只是为了获取一个可用的指针
			// 在其他操作系统
		ptr = zend_mm_mmap(size + alignment - REAL_PAGE_SIZE);
// 如果是 windows 系统 
#ifdef _WIN32
		// 第二次计算偏移量
		offset = ZEND_MM_ALIGNED_OFFSET(ptr, alignment);
		// 如果还有偏移
		if (offset != 0) {
			// 计算开头的偏移位置
			offset = alignment - offset;
		}
		// 释放这块内存
		zend_mm_munmap(ptr, size + alignment - REAL_PAGE_SIZE);
		// 调整内存指针，让它对齐 alignment。然后重新分配 size 大小的内存
		ptr = zend_mm_mmap_fixed((void*)((char*)ptr + offset), size);
		// 如果分配失败，这块内存如果被其他人用，这里会分配失败！
		if (ptr == NULL) { // fix GH-9650, fixed addr range is not free
			// 重新分配内存大小是： 
			ptr = zend_mm_mmap(size + alignment - REAL_PAGE_SIZE);
			// 分配失败，返回null
			if (ptr == NULL) {
				return NULL;
			}
			// 找到偏移位置
			offset = ZEND_MM_ALIGNED_OFFSET(ptr, alignment);
			// 存在偏移
			if (offset != 0) {
				// 移动指针到对齐的位置（god! 那前面岂不是浪费一块）
				// 要释放的时候怎么办呢，浪费的内存如何回收呢？
				ptr = (void*)((char*)ptr + alignment - offset);
			}
		}
		// 
		return ptr;
// 如果不是windows系统 
#else
		// 计算内存偏移量
		offset = ZEND_MM_ALIGNED_OFFSET(ptr, alignment);
		// 如果不是0
		if (offset != 0) {
			// 计算偏移差值
			offset = alignment - offset;
			// 释放这块内存里没有对齐的部分
			zend_mm_munmap(ptr, offset);
			// 把指针移动到对齐的位置上
			ptr = (char*)ptr + offset;
			// 
			alignment -= offset;
		}
		// 如果后面有多出来的部分
		if (alignment > REAL_PAGE_SIZE) {
			// 释放后面多出来的部分这块内存
			zend_mm_munmap((char*)ptr + size, alignment - REAL_PAGE_SIZE);
		}
		// 
		if (zend_mm_use_huge_pages) {
			zend_mm_hugepage(ptr, size);
		}
#endif
		return ptr;
	}
}

// ing3, 分配 chunk
static void *zend_mm_chunk_alloc(zend_mm_heap *heap, size_t size, size_t alignment)
{
// 
#if ZEND_MM_STORAGE
	// 如果有自带 storage（默认没有）
	if (UNEXPECTED(heap->storage)) {
		void *ptr = heap->storage->handlers.chunk_alloc(heap->storage, size, alignment);
		ZEND_ASSERT(((zend_uintptr_t)((char*)ptr + (alignment-1)) & (alignment-1)) == (zend_uintptr_t)ptr);
		return ptr;
	}
#endif
	// 分配一个 chunk
	return zend_mm_chunk_alloc_int(size, alignment);
}

// ing3, 释放 chunk
static void zend_mm_chunk_free(zend_mm_heap *heap, void *addr, size_t size)
{
#if ZEND_MM_STORAGE
	// 如果有自带 storage（默认没有）
	if (UNEXPECTED(heap->storage)) {
		heap->storage->handlers.chunk_free(heap->storage, addr, size);
		return;
	}
#endif
	// 直接释放内存
	zend_mm_munmap(addr, size);
}

// ing3, 截短 chunk
static int zend_mm_chunk_truncate(zend_mm_heap *heap, void *addr, size_t old_size, size_t new_size)
{
#if ZEND_MM_STORAGE
	// 如果有自带 storage（默认没有）
	if (UNEXPECTED(heap->storage)) {
		if (heap->storage->handlers.chunk_truncate) {
			return heap->storage->handlers.chunk_truncate(heap->storage, addr, old_size, new_size);
		} else {
			return 0;
		}
	}
#endif

#ifndef _WIN32
	// window 环境下才有清空功能 ？
	// 释放一分配内存 old_size 到 new_size
	zend_mm_munmap((char*)addr + new_size, old_size - new_size);
	return 1;
#else
	return 0;
#endif
}

// ing3, 扩大 chunk 
static int zend_mm_chunk_extend(zend_mm_heap *heap, void *addr, size_t old_size, size_t new_size)
{
#if ZEND_MM_STORAGE
	// 如果有自带 storage（默认没有）
	if (UNEXPECTED(heap->storage)) {
		if (heap->storage->handlers.chunk_extend) {
			return heap->storage->handlers.chunk_extend(heap->storage, addr, old_size, new_size);
		} else {
			return 0;
		}
	}
#endif

// 默认没有 HAVE_MREMAP
#ifdef HAVE_MREMAP
	/* We don't use MREMAP_MAYMOVE due to alignment requirements. */
	void *ptr = mremap(addr, old_size, new_size, 0);
	if (ptr == MAP_FAILED) {
		return 0;
	}
	/* Sanity check: The mapping shouldn't have moved. */
	ZEND_ASSERT(ptr == addr);
	return 1;
// 如果不是windows操作系统
#elif !defined(_WIN32)
	// 
	return (zend_mm_mmap_fixed((char*)addr + old_size, new_size - old_size) != NULL);
// ??? windows 走这个分支, 什么也不做
#else
	return 0;
#endif
}

// ing3, 初始化 chunk ， main_chunk 没有 ->next ,只有 ->priv 指向最新创建的元素
static zend_always_inline void zend_mm_chunk_init(zend_mm_heap *heap, zend_mm_chunk *chunk)
{
	// 关联到 heap
	chunk->heap = heap;
	// 把原来的 main 放在自己后面
	chunk->next = heap->main_chunk;
	// main的前一个，放在自己前面
	chunk->prev = heap->main_chunk->prev;
	// chunk->prev->next 原本是main_chunk , 现在把自己插到 原来的priv 和 main_chunk 之间
	chunk->prev->next = chunk;
	// main_chunk的前一个是自己
	chunk->next->prev = chunk;
	// 把 第一个pages 标记成 已分配
	/* mark first pages as allocated */
	// 空闲 pages 数量 ZEND_MM_FIRST_PAGE=1
	chunk->free_pages = ZEND_MM_PAGES - ZEND_MM_FIRST_PAGE;
	// 1，结尾连续空闲page的第一个，下标是1
	chunk->free_tail = ZEND_MM_FIRST_PAGE;
	// chunk编号 ，比前一个chunk使用更大的序号
	/* the younger chunks have bigger number */
	chunk->num = chunk->prev->num + 1;
	// 第一个page 标记成已使用 
	/* mark first pages as allocated */
	// 1<<2 -1 ，利用 ZEND_MM_FIRST_PAGE 找到第一个元素，为什么不直接用1呢？
	chunk->free_map[0] = (1L << ZEND_MM_FIRST_PAGE) - 1;
	// 添加page串结束标记：LRUN 标记
	//  #define ZEND_MM_LRUN(count) (ZEND_MM_IS_LRUN | ((count) << ZEND_MM_LRUN_PAGES_OFFSET))
	chunk->map[0] = ZEND_MM_LRUN(ZEND_MM_FIRST_PAGE);
}

// 
/***********************/
/* Huge Runs (forward) */
/***********************/

static size_t zend_mm_get_huge_block_size(zend_mm_heap *heap, void *ptr ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC);
static void *zend_mm_alloc_huge(zend_mm_heap *heap, size_t size ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC);
static void zend_mm_free_huge(zend_mm_heap *heap, void *ptr ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC);

#if ZEND_DEBUG
static void zend_mm_change_huge_block_size(zend_mm_heap *heap, void *ptr, size_t size, size_t dbg_size ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC);
#else
static void zend_mm_change_huge_block_size(zend_mm_heap *heap, void *ptr, size_t size ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC);
#endif

/**************/
/* Large Runs */
/**************/
// 调试模式
#if ZEND_DEBUG

// ing3, 分配 page, 200+ 行代码
static void *zend_mm_alloc_pages(zend_mm_heap *heap, uint32_t pages_count, size_t size ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC)
#else
static void *zend_mm_alloc_pages(zend_mm_heap *heap, uint32_t pages_count ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC)
#endif
{
	// 取得 main_chunk 
	zend_mm_chunk *chunk = heap->main_chunk;
	//
	uint32_t page_num, len;
	// 
	int steps = 0;

	while (1) {
		// 如果 pages不够用
		if (UNEXPECTED(chunk->free_pages < pages_count)) {
			// 按没找到操作 1
			goto not_found;
// 未启用逻辑
#if 0
		} else if (UNEXPECTED(chunk->free_pages + chunk->free_tail == ZEND_MM_PAGES)) {
			if (UNEXPECTED(ZEND_MM_PAGES - chunk->free_tail < pages_count)) {
				goto not_found;
			} else {
				page_num = chunk->free_tail;
				goto found;
			}
		} else if (0) {
			/* First-Fit Search */
			int free_tail = chunk->free_tail;
			zend_mm_bitset *bitset = chunk->free_map;
			zend_mm_bitset tmp = *(bitset++);
			int i = 0;

			while (1) {
				/* skip allocated blocks */
				while (tmp == (zend_mm_bitset)-1) {
					// ZEND_MM_BITSET_LEN 32或64
					i += ZEND_MM_BITSET_LEN;
					if (i == ZEND_MM_PAGES) {
						goto not_found;
					}
					tmp = *(bitset++);
				}
				// 找到第一个 0 位
				/* find first 0 bit */
				// 找到空闲的page编号，zend_mm_bitset_nts 结尾连续的值为1的bit的数量
				page_num = i + zend_mm_bitset_nts(tmp);
				// 把 bits 从 0 重置成 bit
				/* reset bits from 0 to "bit" */
				tmp &= tmp + 1;
				// 跳过空闲的块
				/* skip free blocks */
				while (tmp == 0) {
					// ZEND_MM_BITSET_LEN 32或64
					i += ZEND_MM_BITSET_LEN;
					len = i - page_num;
					if (len >= pages_count) {
						goto found;
					} else if (i >= free_tail) {
						goto not_found;
					}
					tmp = *(bitset++);
				}
				/* find first 1 bit */
				len = (i + zend_ulong_ntz(tmp)) - page_num;
				if (len >= pages_count) {
					goto found;
				}
				/* set bits from 0 to "bit" */
				tmp |= tmp - 1;
			}
#endif
		// 如果 page 够用
		} else {
			// 最佳位置 的 页码（page_num）
			/* Best-Fit Search */
			// 初始状态为无效
			int best = -1;
			// best_len 是能满足要求的最小空间
			uint32_t best_len = ZEND_MM_PAGES;
			// 块结尾的空闲page数
			uint32_t free_tail = chunk->free_tail;
			// chunk 地图
			zend_mm_bitset *bitset = chunk->free_map;
			
			// 跳过第一个page，因为第一个page是特殊用途
			// tmp 是 ulong 类型，这里是数值，不是指针
			// typedef zend_ulong zend_mm_bitset;
			zend_mm_bitset tmp = *(bitset++);
			//
			uint32_t i = 0;

			// n（>=0）个连续的用过的+m（>=0）个连续没用过的，为一段，一个chunk被划分成多个段来检查
			// 遍历每一个段
			while (1) {
				// 步骤1：
				// 跳过开头已分配完的页
				/* skip allocated blocks */
				// 如果 tmp 是64个 1，全满。要跳过开头所有全满的页
				while (tmp == (zend_mm_bitset)-1) {
					// 碰到全满页，翻页
					// ZEND_MM_BITSET_LEN 32或64
					i += ZEND_MM_BITSET_LEN;
					// 如果到最后一页，可以结束循环了。
					if (i == ZEND_MM_PAGES) {
						// 如果有最佳位置
						if (best > 0) {
							// 页码到最佳位置
							page_num = best;
							// 按找到空间操作 1
							goto found;
						// 如果没有最佳位置
						} else {
							// 按没找到空间操作 2
							goto not_found;
						}
					}
					// 下一页 
					tmp = *(bitset++);
				}
				// 如果 tmp 不是 64个1，在这一页里查找
				
				// 找到第一个0，bitset里是从右向左排列的，所以这里是从右向左查找
				/* find first 0 bit */
				// 结尾连续的值为1的bit的数量
				// page_num实际上是找到的可用位置的开头
				page_num = i + zend_mm_bitset_nts(tmp);
				
				// 把右侧n个位置全设置成0
				// 如果左面全是0，右面全是1，中间没有乱排的。tmp会变成0
				/* reset bits from 0 to "bit" */
				tmp &= tmp + 1;
				
				// 跳过连续的空闲块，累加可用page数量
				/* skip free blocks */
				// 如果全是0，说明所有bit都是连续使用的，中间没有空
				while (tmp == 0) {
					// 找到空闲块，翻页计数 ，ZEND_MM_BITSET_LEN 32或64
					i += ZEND_MM_BITSET_LEN;
					// 如果比 free_tail 大 (说明后面全是空page)
					// 或 等于  ZEND_MM_PAGES=512，说明已经到达结尾
					// 这两种情况都表示到达最后一个段，可以结束循环了。
					if (i >= free_tail || i == ZEND_MM_PAGES) {
						// 剩余空间
						len = ZEND_MM_PAGES - page_num;
						// 如果剩余空间够用 并且比 best_len 小 ，说明在最后一个段中分配是最佳方案。
						if (len >= pages_count && len < best_len) {
							// 找到合适的空间，第一时间先把这一段标记成占用。
							// 把 free_tail 的位置后移 pages_count 个
							chunk->free_tail = page_num + pages_count;
							// 按找到空间操作 2
							goto found;
						// 如果剩余空间不足 或者 比 best_len 大
						} else {
							// 设置正确值。为什么要在这个地方更新 free_tail 呢？
							/* set accurate value */
							chunk->free_tail = page_num;
							// 如果已经找到最佳位置
							if (best > 0) {
								// 到最佳位置进行操作
								page_num = best;
								// 按找到空间操作 3
								goto found;
							// 没找到最佳位置
							} else {
								// 按没找到操作 3
								goto not_found;
							}
						}
					}
					// 地图下一页
					tmp = *(bitset++);
				}
				
				// 到这里，说明找到这一段的结尾位置，后面还有其他段
				// 找到第一个1bit（段的结尾位置），用它减掉 page_num（空闲page的开头）得到空闲page的长度
				/* find first 1 bit */
				// zend_ulong_ntz：获取右侧连续0的数量
				len = i + zend_ulong_ntz(tmp) - page_num;
				
				// 如果这一段的长度够长
				if (len >= pages_count) {
					// 如果找到的空间刚好和要求的一样大
					if (len == pages_count) {
						// 按找到空间操作 4
						goto found;
					// 如果找到的空间比当前 best_len 要小
					} else if (len < best_len) {
						// 当前位置代替 best_len
						best_len = len;
						// 最佳位置页码
						best = page_num;
					}
				}
				// 把右侧n个位置设置成1
				// 如果左面全是1，右面全是0，中间没有乱排的。tmp会变成-1（每个位都是1）
				/* set bits from 0 to "bit" */
				tmp |= tmp - 1;
				
				
				// tmp &= tmp + 1; 和 tmp |= tmp - 1; 可以很好地处理 一个bitsit中有多个段
				// 如，初始bitset为: 10101011，开始序号0，结尾序号3，找到 1 个空闲位置
				// tmp &= tmp + 1 => 10101000
				// tmp |= tmp - 1 => 10101111，开始序号0，结尾序号5，找到 1 个空闲位置
				// tmp &= tmp + 1 => 10100000
				// tmp |= tmp - 1 => 10111111，开始序号0，结尾序号7，找到 1 个空闲位置
				// tmp &= tmp + 1 => 10000000
				// tmp |= tmp - 1 => 11111111，跳过此页到下一页
				// 就这样把开头位置一直向左推
			}
		}

// 如果 page 不够用

// 按没找到操作 4
// 默认情况走 not_found
not_found:
		// 如果当前 chunk 的下一个是 main_chunk
		if (chunk->next == heap->main_chunk) {
// 查找chunk 1 
get_chunk:
			// 如果有缓存chunks，先使用缓存chunk
			if (heap->cached_chunks) {
				// 缓存 chunk 数 -1
				heap->cached_chunks_count--;
				// 找到缓存chunk
				chunk = heap->cached_chunks;
				// 缓存chunk指向下一个
				heap->cached_chunks = chunk->next;
			// 如果没有缓存 chunks
			} else {
// 如果有内存限制 
#if ZEND_MM_LIMIT
				// 如果可用内存小于 ZEND_MM_CHUNK_SIZE，内存不足
				if (UNEXPECTED(ZEND_MM_CHUNK_SIZE > heap->limit - heap->real_size)) {
					// 回收空闲内存
					if (zend_mm_gc(heap)) {
						// 查找chunk 2 
						goto get_chunk;
					// 内存未溢出
					} else if (heap->overflow == 0) {
// 调试
#if ZEND_DEBUG
						zend_mm_safe_error(heap, "Allowed memory size of %zu bytes exhausted at %s:%d (tried to allocate %zu bytes)", heap->limit, __zend_filename, __zend_lineno, size);
// 正式业务
#else
						// 报错：内存耗尽
						zend_mm_safe_error(heap, "Allowed memory size of %zu bytes exhausted (tried to allocate %zu bytes)", heap->limit, ZEND_MM_PAGE_SIZE * pages_count);
#endif
						return NULL;
					}
				}
#endif
				// 如果内存充足，分配一个新chunk
				chunk = (zend_mm_chunk*)zend_mm_chunk_alloc(heap, ZEND_MM_CHUNK_SIZE, ZEND_MM_CHUNK_SIZE);
				// 如果分配失败
				if (UNEXPECTED(chunk == NULL)) {
					// 内存不足
					/* insufficient memory */
					// 回收空闲内存，再次分配，如果分配成功，继续后面的逻辑
					if (zend_mm_gc(heap) &&
					    (chunk = (zend_mm_chunk*)zend_mm_chunk_alloc(heap, ZEND_MM_CHUNK_SIZE, ZEND_MM_CHUNK_SIZE)) != NULL) {
						/* pass */
					// 如果还是分配失败，报错
					} else {
// 没有限制 ，直接报内存不足
#if !ZEND_MM_LIMIT
						zend_mm_safe_error(heap, "Out of memory");
// 调试状态，更详细一些
#elif ZEND_DEBUG
						zend_mm_safe_error(heap, "Out of memory (allocated %zu bytes) at %s:%d (tried to allocate %zu bytes)", heap->real_size, __zend_filename, __zend_lineno, size);
// 其他情况
#else
						zend_mm_safe_error(heap, "Out of memory (allocated %zu bytes) (tried to allocate %zu bytes)", heap->real_size, ZEND_MM_PAGE_SIZE * pages_count);
#endif
						return NULL;
					}
				}
// 统计内存信息
#if ZEND_MM_STAT
				do {
					size_t size = heap->real_size + ZEND_MM_CHUNK_SIZE;
					size_t peak = MAX(heap->real_peak, size);
					heap->real_size = size;
					heap->real_peak = peak;
				} while (0);
// 如果有内存限制 
#elif ZEND_MM_LIMIT
				// 记录当前使用量
				heap->real_size += ZEND_MM_CHUNK_SIZE;

#endif
			}
			// chunks 数量 +1
			heap->chunks_count++;
			// 如果创新记录了
			if (heap->chunks_count > heap->peak_chunks_count) {
				// 记录最大 chunks数
				heap->peak_chunks_count = heap->chunks_count;
			}
			// 初始化 chunk
			zend_mm_chunk_init(heap, chunk);
			// 地图页码更新到 1
			page_num = ZEND_MM_FIRST_PAGE;
			// 可用页数 512 - 1 
			len = ZEND_MM_PAGES - ZEND_MM_FIRST_PAGE;
			// 按找到空间操作 5
			goto found;
		// 如果当前 chunk 的下一个不是 main_chunk
		} else {
			// 切换到下一个 chunk
			chunk = chunk->next;
			// 记录前进 1 步
			steps++;
		}
	}

// 按找到空间操作 6
found:
	// 如果移动了2次以上 并且 需要的pages 少于8个（是小块内存分配）。
	// 针对于小块内存多次分配，把chunk放在前面提升效率
	if (steps > 2 && pages_count < 8) {
		// 把这个chunk 放到 chunk 串的头上
		/* move chunk into the head of the linked-list */
		// 先把自己摘出来，切断和前后 chunk 的关联 
		chunk->prev->next = chunk->next;
		chunk->next->prev = chunk->prev;
		// 更新自己的两个指针，把自己放到开头，紧跟在main_chunk的后面
		chunk->next = heap->main_chunk->next;
		chunk->prev = heap->main_chunk;
		// 更新自己前后原素的指针
		// main_chunk 指向自己
		chunk->prev->next = chunk;
		// 下一个chunk 指向自己
		chunk->next->prev = chunk;
	}
	// 把page 标记成已分配
	/* mark run as allocated */
	chunk->free_pages -= pages_count;
	// 更新 map
	zend_mm_bitset_set_range(chunk->free_map, page_num, pages_count);
	// 添加 LRUN 标记
	chunk->map[page_num] = ZEND_MM_LRUN(pages_count);
	// 如果page数刚好和chunk里剩余的相等
	if (page_num == chunk->free_tail) {
		// 最后的可用页码后移
		chunk->free_tail = page_num + pages_count;
	}
	// 返回page地址
	return ZEND_MM_PAGE_ADDR(chunk, page_num);
}

// ing3, 分配 large
static zend_always_inline void *zend_mm_alloc_large_ex(zend_mm_heap *heap, size_t size ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC)
{
	// 计算需要多少个 page
	int pages_count = (int)ZEND_MM_SIZE_TO_NUM(size, ZEND_MM_PAGE_SIZE);
// 如果是调试模式
#if ZEND_DEBUG
	void *ptr = zend_mm_alloc_pages(heap, pages_count, size ZEND_FILE_LINE_RELAY_CC ZEND_FILE_LINE_ORIG_RELAY_CC);
#else
	// 分配page
	void *ptr = zend_mm_alloc_pages(heap, pages_count ZEND_FILE_LINE_RELAY_CC ZEND_FILE_LINE_ORIG_RELAY_CC);
#endif
#if ZEND_MM_STAT
	do {
		size_t size = heap->size + pages_count * ZEND_MM_PAGE_SIZE;
		size_t peak = MAX(heap->peak, size);
		heap->size = size;
		heap->peak = peak;
	} while (0);
#endif
	// 返回指针
	return ptr;
}

// 如果是调试模式
#if ZEND_DEBUG
static zend_never_inline void *zend_mm_alloc_large(zend_mm_heap *heap, size_t size ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC)
{
	return zend_mm_alloc_large_ex(heap, size ZEND_FILE_LINE_RELAY_CC ZEND_FILE_LINE_ORIG_RELAY_CC);
}
// 正式
#else
	
// ing4, 创建large块
static zend_never_inline void *zend_mm_alloc_large(zend_mm_heap *heap, size_t size ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC)
{
	return zend_mm_alloc_large_ex(heap, size ZEND_FILE_LINE_RELAY_CC ZEND_FILE_LINE_ORIG_RELAY_CC);
}
#endif

// ing3, 删除chunk
static zend_always_inline void zend_mm_delete_chunk(zend_mm_heap *heap, zend_mm_chunk *chunk)
{
	// 先把自己从链条上摘出来
	chunk->next->prev = chunk->prev;
	chunk->prev->next = chunk->next;
	// chunk数 -1
	heap->chunks_count--;
	// 如果chunk数和缓存chunk数加起来，小于平均使用chunk数
	if (heap->chunks_count + heap->cached_chunks_count < heap->avg_chunks_count + 0.1
	// 或者 （ chunk数达到删除阀值 并且 最后一次删除时chunk数>=4个 ）
	 || (heap->chunks_count == heap->last_chunks_delete_boundary
	  && heap->last_chunks_delete_count >= 4)) {
		// 延时删除
		/* delay deletion */
		// 缓存chunk链表串中的chunk数加+1
		heap->cached_chunks_count++;
		// next指针指向cache链表的第一个元素
		chunk->next = heap->cached_chunks;
		// 让本chunk作为cache链表的第一个元素
		heap->cached_chunks = chunk;
	// 大于平均使用chunk数，触发清理
	} else {
#if ZEND_MM_STAT || ZEND_MM_LIMIT
		heap->real_size -= ZEND_MM_CHUNK_SIZE;
#endif
		// 如果没有缓存chunk
		if (!heap->cached_chunks) {
			// 如果当前 chunk 数不等于上次清理临界值
			if (heap->chunks_count != heap->last_chunks_delete_boundary) {
				// 更新临界值
				heap->last_chunks_delete_boundary = heap->chunks_count;
				// 清理数量归0
				heap->last_chunks_delete_count = 0;
			// 等于上次清理临界值
			} else {
				// 清理数+1
				heap->last_chunks_delete_count++;
			}
		}
		// 如果没有缓存 chunk 或 当前 chunk 序号大于 缓存 chunk 序号
		if (!heap->cached_chunks || chunk->num > heap->cached_chunks->num) {
			// 把这个chunk 释放掉
			zend_mm_chunk_free(heap, chunk, ZEND_MM_CHUNK_SIZE);
		// 如果有缓存 并且 当前chunk序号小于缓存 chunk
		} else {
//TODO: select the best chunk to delete???
			// chunk 添加到缓存中
			chunk->next = heap->cached_chunks->next;
			// 删除第一个缓存chunk
			zend_mm_chunk_free(heap, heap->cached_chunks, ZEND_MM_CHUNK_SIZE);
			// 当前chunk排到缓存第一个
			heap->cached_chunks = chunk;
		}
	}
}

// ing4，释放pages
static zend_always_inline void zend_mm_free_pages_ex(zend_mm_heap *heap, zend_mm_chunk *chunk, uint32_t page_num, uint32_t pages_count, int free_chunk)
{
	// 增加可用page数
	chunk->free_pages += pages_count;
	// 更新bitset地图，把相应的page标记成空闲
	zend_mm_bitset_reset_range(chunk->free_map, page_num, pages_count);
	// 重置地图信息
	chunk->map[page_num] = 0;
	// 如果被删除的page串的结尾 == 所有已用page的结尾（后面全是空闲page）
	if (chunk->free_tail == page_num + pages_count) {
		// 这个设置可能不精确 
		/* this setting may be not accurate */
		// 更新结尾空闲page序号
		chunk->free_tail = page_num;
	}
	// 否则就不更新 free_tail 了，因为pages_count没有接到结尾的free page串上。
		// chunk->free_tail < page_num + pages_count 不可能发生
		// chunk->free_tail > page_num + pages_count 中间留有使用过的page, free_tail位置没变
	
	// 如果要求释放chunk 并且chunk不是main_chunk 并且 chunk是空的
	if (free_chunk && chunk != heap->main_chunk && chunk->free_pages == ZEND_MM_PAGES - ZEND_MM_FIRST_PAGE) {
		// 删掉这个chunk
		zend_mm_delete_chunk(heap, chunk);
	}
}

// ing4,  释放page
static zend_never_inline void zend_mm_free_pages(zend_mm_heap *heap, zend_mm_chunk *chunk, int page_num, int pages_count)
{
	zend_mm_free_pages_ex(heap, chunk, page_num, pages_count, 1);
}

// ing4, 释放 large
static zend_always_inline void zend_mm_free_large(zend_mm_heap *heap, zend_mm_chunk *chunk, int page_num, int pages_count)
{
#if ZEND_MM_STAT
	// 减掉删除的 page
	heap->size -= pages_count * ZEND_MM_PAGE_SIZE;
#endif
	// 
	zend_mm_free_pages(heap, chunk, page_num, pages_count);
}

/**************/
/* Small Runs */
/**************/

// ing3, 返回表示这个数用到的bit位数， size最大65536，再大也是16
/* higher set bit number (0->N/A, 1->1, 2->2, 4->3, 8->4, 127->7, 128->8 etc) */
static zend_always_inline int zend_mm_small_size_to_bit(int size)
{

// 如果用的 gnuc ，windows 肯定不是这个
#if (defined(__GNUC__) || __has_builtin(__builtin_clz))  && defined(PHP_HAVE_BUILTIN_CLZ)
	return (__builtin_clz(size) ^ 0x1f) + 1;
// 如果是window 操作系统 
#elif defined(_WIN32)
	unsigned long index;
	// 如果是 0，直接返回
	if (!BitScanReverse(&index, (unsigned long)size)) {
		// 未定义行为
		/* undefined behavior */
		return 64;
	}

	// 抑或 1^2^4=7 ,  1^2^7 = 4
	return (((31 - (int)index) ^ 0x1f) + 1);
// 最大支持16位
#else
	// 返回值是1到16,这样写的好处是： 1，每次减半，可保证不遗漏。2，这样计算需要的运算次数最少？
	// 思路是用二分法，从左向右，每次切掉一半。
		// 0000 0000 0000 0001 -> 0000 0001 -> 0001 -> 01 -> 1
	int n = 16;
	// 是否可以删掉开头8位
	if (size <= 0x00ff) {n -= 8; size = size << 8;}
	// 是否可以删掉开头4位
	if (size <= 0x0fff) {n -= 4; size = size << 4;}
	// 是否可以删掉开头2位
	if (size <= 0x3fff) {n -= 2; size = size << 2;}
	// 是否可以删掉开头1位
	if (size <= 0x7fff) {n -= 1;}
	// 
	return n;
#endif
}

// clear, 取两个数中的大数
#ifndef MAX
# define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

// clear, 取两个数中的小数
#ifndef MIN
# define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

// ing3, 计算 一个 size 在 ZEND_MM_BINS_INFO 数组里的序号，size 最大是3072
static zend_always_inline int zend_mm_small_size_to_bin(size_t size)
{
// 无效逻辑
#if 0
	int n;
                            /*0,  1,  2,  3,  4,  5,  6,  7,  8,  9  10, 11, 12*/
	static const int f1[] = { 3,  3,  3,  3,  3,  3,  3,  4,  5,  6,  7,  8,  9};
	static const int f2[] = { 0,  0,  0,  0,  0,  0,  0,  4,  8, 12, 16, 20, 24};

	if (UNEXPECTED(size <= 2)) return 0;
	// 返回表示这个数用到的bit位数
	n = zend_mm_small_size_to_bit(size - 1);
	return ((size-1) >> f1[n]) + f2[n];
// 有用逻辑
#else
	unsigned int t1, t2;

	// 小于等于64，5位
	if (size <= 64) {
		// 支持 0
		/* we need to support size == 0 ... */
		// ( size - 0或1 ) / 8。8以下都是0咯
		return (size - !!size) >> 3;
	// 大于 64	
	} else {
		//
		t1 = size - 1;
		// 返回表示这个数用到的bit位数
		t2 = zend_mm_small_size_to_bit(t1) - 3;
		// 这样不是正好缩没了么？
		t1 = t1 >> t2;
		// 1 + (t2-3) *4
		t2 = t2 - 3;
		//
		t2 = t2 << 2;
		return (int)(t1 + t2);
	}
#endif
}

// ing4, 计算 size 在 ZEND_MM_BINS_INFO 数组里的序号，size 最大是3072
#define ZEND_MM_SMALL_SIZE_TO_BIN(size)  zend_mm_small_size_to_bin(size)

// ing4, 分配pages，初始化 small 块， 
// bin_num 是 ZEND_MM_BINS_INFO 中的块配置的序号, 有了它方便获取优化好的 块信息
static zend_never_inline void *zend_mm_alloc_small_slow(zend_mm_heap *heap, uint32_t bin_num ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC)
{
	zend_mm_chunk *chunk;
	int page_num;
	zend_mm_bin *bin;
	zend_mm_free_slot *p, *end;

// 调试
#if ZEND_DEBUG
	// bin_data_size : ZEND_MM_BINS_INFO中的 size列
	bin = (zend_mm_bin*)zend_mm_alloc_pages(heap, bin_pages[bin_num], bin_data_size[bin_num] ZEND_FILE_LINE_RELAY_CC ZEND_FILE_LINE_ORIG_RELAY_CC);
// 正式运行
#else
	// 根据 ZEND_MM_BINS_INFO 里的配置， 分配一组 page （最多7个）
	bin = (zend_mm_bin*)zend_mm_alloc_pages(heap, bin_pages[bin_num] ZEND_FILE_LINE_RELAY_CC ZEND_FILE_LINE_ORIG_RELAY_CC);
#endif
	// 如果分配失败
	if (UNEXPECTED(bin == NULL)) {
		// 内存不足
		/* insufficient memory */
		return NULL;
	}
	// 取回所在chunk的指针
	chunk = (zend_mm_chunk*)ZEND_MM_ALIGNED_BASE(bin, ZEND_MM_CHUNK_SIZE);
	// 找到地图页码
	page_num = ZEND_MM_ALIGNED_OFFSET(bin, ZEND_MM_CHUNK_SIZE) / ZEND_MM_PAGE_SIZE;
	// 更新这一页地图，添加 SRUN 标记，SRUN是开头块，start run的意思？ 
	chunk->map[page_num] = ZEND_MM_SRUN(bin_num);
	// 如果pages数不是1
	if (bin_pages[bin_num] > 1) {
		uint32_t i = 1;
		do {
			// 成串的 存放 small 块 pages，即是 small 又是 large
			// 后面每个 page 里添加：LRUN，SRUN  标记 + 这个page在这组page中的序号
			chunk->map[page_num+i] = ZEND_MM_NRUN(bin_num, i);
			i++;
		} while (i < bin_pages[bin_num]);
	}

	// 创建一串元素，从1到最后
	/* create a linked list of elements from 1 to last */
	// bin_data_size : ZEND_MM_BINS_INFO中的 size列
	// 取得 一串里的最后一个元素。转成char指针可以方便地按字节移动
	end = (zend_mm_free_slot*)((char*)bin + (bin_data_size[bin_num] * (bin_elements[bin_num] - 1)));
	// heap->free_slot[bin_num] 本身就是第一个元素，所以它里面的指针要指向第二个元素
	// 这个地方还要加强理解 ： 这里是创建了一个 zend_mm_free_slot 结构体的实例，并把指针返回回来。
		// zend_mm_free_slot 里面有一个指针
	heap->free_slot[bin_num] = p = (zend_mm_free_slot*)((char*)bin + bin_data_size[bin_num]);
	// 把元素串成串
	do {
		// bin_data_size : ZEND_MM_BINS_INFO中的 size列
		// 每个元素被初始化成指向下一个元素的指针，这样用的时候不用算数了，比较方便
		// 这里和上面一样，也是创建了一个 zend_mm_free_slot 结构体的实例，并把指针返回回来。
		p->next_free_slot = (zend_mm_free_slot*)((char*)p + bin_data_size[bin_num]);
// 调试
#if ZEND_DEBUG
		do {
			// bin_data_size : ZEND_MM_BINS_INFO中的 size列
			zend_mm_debug_info *dbg = (zend_mm_debug_info*)((char*)p + bin_data_size[bin_num] - ZEND_MM_ALIGNED_SIZE(sizeof(zend_mm_debug_info)));
			dbg->size = 0;
		} while (0);
#endif
		// 跳到下一个位置， bin_data_size : ZEND_MM_BINS_INFO中的 size列
		p = (zend_mm_free_slot*)((char*)p + bin_data_size[bin_num]);
	// 这个位置不是结尾，就继续操作
	} while (p != end);

	// 最后一个元素指向null
	/* terminate list using NULL */
	p->next_free_slot = NULL;
#if ZEND_DEBUG
		do {
			// bin_data_size : ZEND_MM_BINS_INFO 中的 size列
			zend_mm_debug_info *dbg = (zend_mm_debug_info*)((char*)p + bin_data_size[bin_num] - ZEND_MM_ALIGNED_SIZE(sizeof(zend_mm_debug_info)));
			dbg->size = 0;
		} while (0);
#endif

	/* return first element */
	// 返回第一个元素的指针
	return bin;
}

// ing3, 分配 small 块
static zend_always_inline void *zend_mm_alloc_small(zend_mm_heap *heap, int bin_num ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC)
{
// 跟踪当前内存情况和峰值
#if ZEND_MM_STAT
	do {
		// bin_data_size : ZEND_MM_BINS_INFO中的 size列
		size_t size = heap->size + bin_data_size[bin_num];
		size_t peak = MAX(heap->peak, size);
		heap->size = size;
		heap->peak = peak;
	} while (0);
#endif

	// 如果有空闲的位置 可用（ heap->free_slot[bin_num] 每个元素下面有一串空闲的 small 块）
	if (EXPECTED(heap->free_slot[bin_num] != NULL)) {
		// 直接使用这个位置
		zend_mm_free_slot *p = heap->free_slot[bin_num];
		// 把空间slot 指向下一个位置
		heap->free_slot[bin_num] = p->next_free_slot;
		return p;
	} else {
		// 分配 page 创建并初始化small 块,返回一串small 块的开头
		return zend_mm_alloc_small_slow(heap, bin_num ZEND_FILE_LINE_RELAY_CC ZEND_FILE_LINE_ORIG_RELAY_CC);
	}
}

// ing3，释放 small 块
static zend_always_inline void zend_mm_free_small(zend_mm_heap *heap, void *ptr, int bin_num)
{
	zend_mm_free_slot *p;

// 内存统计
#if ZEND_MM_STAT
	// bin_data_size : ZEND_MM_BINS_INFO中的 size列
	heap->size -= bin_data_size[bin_num];
#endif

// 调试
#if ZEND_DEBUG
	do {
		// bin_data_size : ZEND_MM_BINS_INFO中的 size列
		zend_mm_debug_info *dbg = (zend_mm_debug_info*)((char*)ptr + bin_data_size[bin_num] - ZEND_MM_ALIGNED_SIZE(sizeof(zend_mm_debug_info)));
		dbg->size = 0;
	} while (0);
#endif
	// 指针转成 zend_mm_free_slot
	p = (zend_mm_free_slot*)ptr;
	// 指向目前的空闲块
	p->next_free_slot = heap->free_slot[bin_num];
	// 把可用块指向这个块
	heap->free_slot[bin_num] = p;
}

// 堆
/********/
/* Heap */
/********/

// 调试模式
#if ZEND_DEBUG
// suspend, 获取堆的调试信息
static zend_always_inline zend_mm_debug_info *zend_mm_get_debug_info(zend_mm_heap *heap, void *ptr)
{
	size_t page_offset = ZEND_MM_ALIGNED_OFFSET(ptr, ZEND_MM_CHUNK_SIZE);
	zend_mm_chunk *chunk;
	int page_num;
	zend_mm_page_info info;

	ZEND_MM_CHECK(page_offset != 0, "zend_mm_heap corrupted");
	// 取回所在chunk的指针
	chunk = (zend_mm_chunk*)ZEND_MM_ALIGNED_BASE(ptr, ZEND_MM_CHUNK_SIZE);
	page_num = (int)(page_offset / ZEND_MM_PAGE_SIZE);
	info = chunk->map[page_num];
	ZEND_MM_CHECK(chunk->heap == heap, "zend_mm_heap corrupted");
	if (EXPECTED(info & ZEND_MM_IS_SRUN)) {
		// 取回 ZEND_MM_BINS_INFO 中的配置编号
		int bin_num = ZEND_MM_SRUN_BIN_NUM(info);
		// bin_data_size : ZEND_MM_BINS_INFO中的 size列
		return (zend_mm_debug_info*)((char*)ptr + bin_data_size[bin_num] - ZEND_MM_ALIGNED_SIZE(sizeof(zend_mm_debug_info)));
	} else /* if (info & ZEND_MM_IS_LRUN) */ {
		int pages_count = ZEND_MM_LRUN_PAGES(info);

		return (zend_mm_debug_info*)((char*)ptr + ZEND_MM_PAGE_SIZE * pages_count - ZEND_MM_ALIGNED_SIZE(sizeof(zend_mm_debug_info)));
	}
}
#endif

// ing3, 分配堆, 无论huge, large, small 都从这里分配
static zend_always_inline void *zend_mm_alloc_heap(zend_mm_heap *heap, size_t size ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC)
{
	void *ptr;
// 调试用
#if ZEND_DEBUG
	size_t real_size = size;
	zend_mm_debug_info *dbg;

	/* special handling for zero-size allocation */
	size = MAX(size, 1);
	// 大小对齐到8byte
	size = ZEND_MM_ALIGNED_SIZE(size) + ZEND_MM_ALIGNED_SIZE(sizeof(zend_mm_debug_info));
	if (UNEXPECTED(size < real_size)) {
		zend_error_noreturn(E_ERROR, "Possible integer overflow in memory allocation (%zu + %zu)", ZEND_MM_ALIGNED_SIZE(real_size), ZEND_MM_ALIGNED_SIZE(sizeof(zend_mm_debug_info)));
		return NULL;
	}
#endif
	// 如果大小在 ZEND_MM_MAX_SMALL_SIZE=3072 （4096*3/4） 内
	if (EXPECTED(size <= ZEND_MM_MAX_SMALL_SIZE)) {
		// 分配 small 块
		// 计算 size 在 ZEND_MM_BINS_INFO 数组里的序号，size 最大是3072
		ptr = zend_mm_alloc_small(heap, ZEND_MM_SMALL_SIZE_TO_BIN(size) ZEND_FILE_LINE_RELAY_CC ZEND_FILE_LINE_ORIG_RELAY_CC);
// 调试用
#if ZEND_DEBUG
		dbg = zend_mm_get_debug_info(heap, ptr);
		dbg->size = real_size;
		dbg->filename = __zend_filename;
		dbg->orig_filename = __zend_orig_filename;
		dbg->lineno = __zend_lineno;
		dbg->orig_lineno = __zend_orig_lineno;
#endif
		return ptr;
	// 大小在 ZEND_MM_MAX_LARGE_SIZE 内
	} else if (EXPECTED(size <= ZEND_MM_MAX_LARGE_SIZE)) {
		// 分配 large 块
		ptr = zend_mm_alloc_large(heap, size ZEND_FILE_LINE_RELAY_CC ZEND_FILE_LINE_ORIG_RELAY_CC);
// 调试用
#if ZEND_DEBUG
		dbg = zend_mm_get_debug_info(heap, ptr);
		dbg->size = real_size;
		dbg->filename = __zend_filename;
		dbg->orig_filename = __zend_orig_filename;
		dbg->lineno = __zend_lineno;
		dbg->orig_lineno = __zend_orig_lineno;
#endif
		return ptr;
	// 大小大于 ZEND_MM_MAX_LARGE_SIZE
	} else {
// 调试用
#if ZEND_DEBUG
		size = real_size;
#endif
		// 分配 huge 块
		return zend_mm_alloc_huge(heap, size ZEND_FILE_LINE_RELAY_CC ZEND_FILE_LINE_ORIG_RELAY_CC);
	}
}

// ing4, 释放一块内存
static zend_always_inline void zend_mm_free_heap(zend_mm_heap *heap, void *ptr ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC)
{
	// 计算这个指针相对 ZEND_MM_CHUNK_SIZE 的偏移量
	size_t page_offset = ZEND_MM_ALIGNED_OFFSET(ptr, ZEND_MM_CHUNK_SIZE);
	// 如果没有偏移量，当成huge
	if (UNEXPECTED(page_offset == 0)) {
		// 并且指针存在
		if (ptr != NULL) {
			// 释放这个chunk
			zend_mm_free_huge(heap, ptr ZEND_FILE_LINE_RELAY_CC ZEND_FILE_LINE_ORIG_RELAY_CC);
		}
	// 有偏移量, large 或 small 都不会占用chunk里的第一个page,所以一定有偏移量
	} else {
		// 取回所在chunk的指针
		zend_mm_chunk *chunk = (zend_mm_chunk*)ZEND_MM_ALIGNED_BASE(ptr, ZEND_MM_CHUNK_SIZE);
		// page 编号
		int page_num = (int)(page_offset / ZEND_MM_PAGE_SIZE);
		// 找到地图信息
		zend_mm_page_info info = chunk->map[page_num];

		// chunk必须属于当前 heap
		ZEND_MM_CHECK(chunk->heap == heap, "zend_mm_heap corrupted");
		// clear, 如果是小块
		if (EXPECTED(info & ZEND_MM_IS_SRUN)) {
			// 释放小块 ，第三个参数： ZEND_MM_BINS_INFO 中的配置编号
			zend_mm_free_small(heap, ptr, ZEND_MM_SRUN_BIN_NUM(info));
		// 不是小块，当成large 块
		} else /* if (info & ZEND_MM_IS_LRUN) */ {
			// 取得连续的page 数量 
			int pages_count = ZEND_MM_LRUN_PAGES(info);
			// page 的偏移量相对对 page大小必须是0，否则报致使错误
			ZEND_MM_CHECK(ZEND_MM_ALIGNED_OFFSET(page_offset, ZEND_MM_PAGE_SIZE) == 0, "zend_mm_heap corrupted");
			// 释放large块
			zend_mm_free_large(heap, chunk, page_num, pages_count);
		}
	}
}

// ing3, 获取此块内存的大小（每块内存是带标记的，再结合chunk中的信息进行计算）
static size_t zend_mm_size(zend_mm_heap *heap, void *ptr ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC)
{
	// 找到 ptr 相对 ZEND_MM_CHUNK_SIZE 的偏移量
	size_t page_offset = ZEND_MM_ALIGNED_OFFSET(ptr, ZEND_MM_CHUNK_SIZE);

	// 如果没有偏移,说明是 huge 块
	if (UNEXPECTED(page_offset == 0)) {
		//
		return zend_mm_get_huge_block_size(heap, ptr ZEND_FILE_LINE_RELAY_CC ZEND_FILE_LINE_ORIG_RELAY_CC);
	} else {
		zend_mm_chunk *chunk;
// 调试，已禁用
#if 0 && ZEND_DEBUG
		zend_mm_debug_info *dbg = zend_mm_get_debug_info(heap, ptr);
		return dbg->size;
// 正常情况
#else
		int page_num;
		zend_mm_page_info info;
		// 取回所在chunk的指针
		chunk = (zend_mm_chunk*)ZEND_MM_ALIGNED_BASE(ptr, ZEND_MM_CHUNK_SIZE);
		// 看偏移量所在的 pages 编号 
		page_num = (int)(page_offset / ZEND_MM_PAGE_SIZE);
		// 找到这个 page 的 map
		info = chunk->map[page_num];
		// 对齐后的 chunk 必须是 heap
		ZEND_MM_CHECK(chunk->heap == heap, "zend_mm_heap corrupted");
		// 如果 map 有 small, SRUN 标记，是small 块
		if (EXPECTED(info & ZEND_MM_IS_SRUN)) {
			// 返回small 块的大小，// 取回 ZEND_MM_BINS_INFO 中的配置编号
			return bin_data_size[ZEND_MM_SRUN_BIN_NUM(info)];
		// map 没有 SRUN 标记， 是 large 块
		} else /* if (info & ZEND_MM_IS_LARGE_RUN) */ {
			// 页数 * 每页大小
			return ZEND_MM_LRUN_PAGES(info) * ZEND_MM_PAGE_SIZE;
		}
#endif
	}
}

// ing4, 缓慢的 重新分配
static zend_never_inline void *zend_mm_realloc_slow(zend_mm_heap *heap, void *ptr, size_t size, size_t copy_size ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC)
{
	void *ret;

// 内存统计
#if ZEND_MM_STAT
	do {
		size_t orig_peak = heap->peak;
#endif
		// 分配内存
		ret = zend_mm_alloc_heap(heap, size ZEND_FILE_LINE_RELAY_CC ZEND_FILE_LINE_ORIG_RELAY_CC);
		// 把原有内容copy过来
		memcpy(ret, ptr, copy_size);
		// 释放原有内容
		zend_mm_free_heap(heap, ptr ZEND_FILE_LINE_RELAY_CC ZEND_FILE_LINE_ORIG_RELAY_CC);
// 内存统计
#if ZEND_MM_STAT
		// 内存峰值
		heap->peak = MAX(orig_peak, heap->size);
	// 这个 do-while 有什么用
	} while (0);
#endif
	// 返回新内存地址
	return ret;
}

// ing3, 重新分配 huge
static zend_never_inline void *zend_mm_realloc_huge(zend_mm_heap *heap, void *ptr, size_t size, size_t copy_size ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC)
{
	size_t old_size;
	size_t new_size;
//
#if ZEND_DEBUG
	size_t real_size;
#endif
	// 先获取原来的大小
	old_size = zend_mm_get_huge_block_size(heap, ptr ZEND_FILE_LINE_RELAY_CC ZEND_FILE_LINE_ORIG_RELAY_CC);
#if ZEND_DEBUG
	real_size = size;
	size = ZEND_MM_ALIGNED_SIZE(size) + ZEND_MM_ALIGNED_SIZE(sizeof(zend_mm_debug_info));
#endif
	// 	#define ZEND_MM_MAX_LARGE_SIZE (ZEND_MM_CHUNK_SIZE - (ZEND_MM_PAGE_SIZE * ZEND_MM_FIRST_PAGE)) = （2m-4Byte(4096bit) ）
	// 如果标的大小超限，只给最大限制的大小（2m-4Byte(4096bit) ），实际上只是更新了一下size, 但一大堆分支判断
	//（2044K测试过）
	if (size > ZEND_MM_MAX_LARGE_SIZE) {
#if ZEND_DEBUG
		size = real_size;
#endif

// 如果是windows系统 
#ifdef ZEND_WIN32
		// 在windows系统中无法随时扩大huge块的空间
		// 用2M的块来代替，避免扩大small 块的时候重新分配多次。
		/* On Windows we don't have ability to extend huge blocks in-place.
		 * We allocate them with 2MB size granularity, to avoid many
		 * reallocations when they are extended by small pieces
		 */
		// 向后对齐 alignment
		new_size = ZEND_MM_ALIGNED_SIZE_EX(size, MAX(REAL_PAGE_SIZE, ZEND_MM_CHUNK_SIZE));
// 其他系统 
#else
		// 向后对齐 到 REAL_PAGE_SIZE
		new_size = ZEND_MM_ALIGNED_SIZE_EX(size, REAL_PAGE_SIZE);
#endif
		// 情况1，如果调整前后大小一样，为什么还要调整？
		if (new_size == old_size) {
#if ZEND_DEBUG
			zend_mm_change_huge_block_size(heap, ptr, new_size, real_size ZEND_FILE_LINE_RELAY_CC ZEND_FILE_LINE_ORIG_RELAY_CC);
// 正式
#else
			// 更新 huge 块的 size 属性
			zend_mm_change_huge_block_size(heap, ptr, new_size ZEND_FILE_LINE_RELAY_CC ZEND_FILE_LINE_ORIG_RELAY_CC);
#endif
			return ptr;
		// 情况2，要求减少空间
		} else if (new_size < old_size) {
			/* unmup tail */
			// 减少空间，如果成功
			if (zend_mm_chunk_truncate(heap, ptr, old_size, new_size)) {
#if ZEND_MM_STAT || ZEND_MM_LIMIT
				heap->real_size -= old_size - new_size;
#endif
#if ZEND_MM_STAT
				heap->size -= old_size - new_size;
#endif
#if ZEND_DEBUG
				zend_mm_change_huge_block_size(heap, ptr, new_size, real_size ZEND_FILE_LINE_RELAY_CC ZEND_FILE_LINE_ORIG_RELAY_CC);
#else
				// 更新 huge 块的 size 属性
				zend_mm_change_huge_block_size(heap, ptr, new_size ZEND_FILE_LINE_RELAY_CC ZEND_FILE_LINE_ORIG_RELAY_CC);
#endif
				return ptr;
			}
			// 如果 zend_mm_chunk_truncate 失败，要调用 zend_mm_realloc_slow()方法重新分配
			
		// 情况3 要求 增加空间
		} else /* if (new_size > old_size) */ {
#if ZEND_MM_LIMIT
			if (UNEXPECTED(new_size - old_size > heap->limit - heap->real_size)) {
				// 回收闲置内存
				if (zend_mm_gc(heap) && new_size - old_size <= heap->limit - heap->real_size) {
					/* pass */
				// 如果内存溢出了，报错
				} else if (heap->overflow == 0) {
#if ZEND_DEBUG
					zend_mm_safe_error(heap, "Allowed memory size of %zu bytes exhausted at %s:%d (tried to allocate %zu bytes)", heap->limit, __zend_filename, __zend_lineno, size);
#else
					zend_mm_safe_error(heap, "Allowed memory size of %zu bytes exhausted (tried to allocate %zu bytes)", heap->limit, size);
#endif
					return NULL;
				}
			}
#endif
			/* try to map tail right after this block */
			// 增加空间，如果成功
			if (zend_mm_chunk_extend(heap, ptr, old_size, new_size)) {
#if ZEND_MM_STAT || ZEND_MM_LIMIT
				heap->real_size += new_size - old_size;
#endif
#if ZEND_MM_STAT
				heap->real_peak = MAX(heap->real_peak, heap->real_size);
				heap->size += new_size - old_size;
				heap->peak = MAX(heap->peak, heap->size);
#endif
#if ZEND_DEBUG
				zend_mm_change_huge_block_size(heap, ptr, new_size, real_size ZEND_FILE_LINE_RELAY_CC ZEND_FILE_LINE_ORIG_RELAY_CC);
// 正式
#else
				// 更新 huge 块的 size 属性
				zend_mm_change_huge_block_size(heap, ptr, new_size ZEND_FILE_LINE_RELAY_CC ZEND_FILE_LINE_ORIG_RELAY_CC);
#endif
				return ptr;
			}
			// 如果 zend_mm_chunk_extend 失败，要调用 zend_mm_realloc_slow()方法重新分配

		}
	}
	// 可能到这里的情况：要求的大小小于 2044K 或 前面调整内存大小失败
	return zend_mm_realloc_slow(heap, ptr, size, MIN(old_size, copy_size) ZEND_FILE_LINE_RELAY_CC ZEND_FILE_LINE_ORIG_RELAY_CC);
}

// ing3, 重新分配 内存, 它根据是否使用 copysize 会产生两个分支方法
static zend_always_inline void *zend_mm_realloc_heap(zend_mm_heap *heap, void *ptr, size_t size, bool use_copy_size, size_t copy_size ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC)
{
	size_t page_offset;
	size_t old_size;
	size_t new_size;
	void *ret;
//	
#if ZEND_DEBUG
	zend_mm_debug_info *dbg;
#endif
	// 找到 相对于 ZEND_MM_CHUNK_SIZE 的偏移量
	page_offset = ZEND_MM_ALIGNED_OFFSET(ptr, ZEND_MM_CHUNK_SIZE);
	// 无偏移，是huge 块
	if (UNEXPECTED(page_offset == 0)) {
		// 如果指针不存在
		if (EXPECTED(ptr == NULL)) {
			// 直接开辟新内存
			return _zend_mm_alloc(heap, size ZEND_FILE_LINE_RELAY_CC ZEND_FILE_LINE_ORIG_RELAY_CC);
		} else {
			// 调整huge大小
			return zend_mm_realloc_huge(heap, ptr, size, copy_size ZEND_FILE_LINE_RELAY_CC ZEND_FILE_LINE_ORIG_RELAY_CC);
		}
	// 有偏移，是small 或 large
	} else {
		// 取回所在chunk的指针
		zend_mm_chunk *chunk = (zend_mm_chunk*)ZEND_MM_ALIGNED_BASE(ptr, ZEND_MM_CHUNK_SIZE);
		// 取回page页码
		int page_num = (int)(page_offset / ZEND_MM_PAGE_SIZE);
		// 地图信息
		zend_mm_page_info info = chunk->map[page_num];
#if ZEND_DEBUG
		size_t real_size = size;

		size = ZEND_MM_ALIGNED_SIZE(size) + ZEND_MM_ALIGNED_SIZE(sizeof(zend_mm_debug_info));
#endif
		// chunk 必须从属于当前内存heap
		ZEND_MM_CHECK(chunk->heap == heap, "zend_mm_heap corrupted");
		// 如果是small 块
		if (info & ZEND_MM_IS_SRUN) {
			// 取回 ZEND_MM_BINS_INFO 中的配置编号
			int old_bin_num = ZEND_MM_SRUN_BIN_NUM(info);

			do {
				// bin_data_size : ZEND_MM_BINS_INFO中的 size列
				old_size = bin_data_size[old_bin_num];

				// 检验 请求的大小是否符合当前 数据块
				/* Check if requested size fits into current bin */
				
				// 如果想把内存划分得更小
				if (size <= old_size) {
					// 检验是否有必要截短
					/* Check if truncation is necessary */
					
					// 旧的序号>0 并且尺寸小于 旧序号的前一个序号。
					// 必须满足这个要求才可以把内存划得更小
					if (old_bin_num > 0 && size < bin_data_size[old_bin_num - 1]) {
						/* truncation */
						// 计算 size 在 ZEND_MM_BINS_INFO 数组里的序号，size 最大是3072
						ret = zend_mm_alloc_small(heap, ZEND_MM_SMALL_SIZE_TO_BIN(size) ZEND_FILE_LINE_RELAY_CC ZEND_FILE_LINE_ORIG_RELAY_CC);
						// 要复制的内容大小
						copy_size = use_copy_size ? MIN(size, copy_size) : size;
						// 复制内容到新地址
						memcpy(ret, ptr, copy_size);
						// 释放原来的 small 块
						zend_mm_free_small(heap, ptr, old_bin_num);
					// 如果不能把内存变得更小，就不重新分配
					} else {
						/* reallocation in-place */
						ret = ptr;
					}
				// size <= 3072，如果是分配 small 块
				} else if (size <= ZEND_MM_MAX_SMALL_SIZE) {
					// small 块的扩大
					/* small extension */
// 内存统计
#if ZEND_MM_STAT
					// 这里加个do-while到底是干嘛呢？为了快速回收内存吗
					// 只要用到临时变量，就要加do-while，这做法确实够严谨的
					do {
						size_t orig_peak = heap->peak;
#endif
						// 分配 small 块
						// 计算 size 在 ZEND_MM_BINS_INFO 数组里的序号，size 最大是3072
						ret = zend_mm_alloc_small(heap, ZEND_MM_SMALL_SIZE_TO_BIN(size) ZEND_FILE_LINE_RELAY_CC ZEND_FILE_LINE_ORIG_RELAY_CC);
						// 确定要copy的大小
						copy_size = use_copy_size ? MIN(old_size, copy_size) : old_size;
						// 复制内容
						memcpy(ret, ptr, copy_size);
						// 释放原来的small 块
						zend_mm_free_small(heap, ptr, old_bin_num);
// 内存统计						
#if ZEND_MM_STAT
						heap->peak = MAX(orig_peak, heap->size);
					} while (0);
#endif
				// 需要的 尺寸大于 small 块，从small 块转到large或huge块
				} else {
					// 走 zend_mm_realloc_slow
					/* slow reallocation */
					break;
				}

#if ZEND_DEBUG
				dbg = zend_mm_get_debug_info(heap, ret);
				dbg->size = real_size;
				dbg->filename = __zend_filename;
				dbg->orig_filename = __zend_orig_filename;
				dbg->lineno = __zend_lineno;
				dbg->orig_lineno = __zend_orig_lineno;
#endif
				return ret;
			}  while (0);
		// 如果是 large 块
		} else /* if (info & ZEND_MM_IS_LARGE_RUN) */ {
			// large 块必须对齐到 page大小
			ZEND_MM_CHECK(ZEND_MM_ALIGNED_OFFSET(page_offset, ZEND_MM_PAGE_SIZE) == 0, "zend_mm_heap corrupted");
			// 一串page的总大小
			old_size = ZEND_MM_LRUN_PAGES(info) * ZEND_MM_PAGE_SIZE;
			// 如果需要的尺寸是 large 大小
			if (size > ZEND_MM_MAX_SMALL_SIZE && size <= ZEND_MM_MAX_LARGE_SIZE) {
				// 向后对齐 alignment
				new_size = ZEND_MM_ALIGNED_SIZE_EX(size, ZEND_MM_PAGE_SIZE);
				// 如果新的和旧的大小一样
				if (new_size == old_size) {
#if ZEND_DEBUG
					dbg = zend_mm_get_debug_info(heap, ptr);
					dbg->size = real_size;
					dbg->filename = __zend_filename;
					dbg->orig_filename = __zend_orig_filename;
					dbg->lineno = __zend_lineno;
					dbg->orig_lineno = __zend_orig_lineno;
#endif
					// 直接返回，不用重新分配了
					return ptr;
				// 如果要分配得更小，large 块直接截短就行，不需要像small 块去考虑配置
				} else if (new_size < old_size) {
					// 
					/* free tail pages */
					// 需要的 page 数量 
					int new_pages_count = (int)(new_size / ZEND_MM_PAGE_SIZE);
					// 新旧相差的 page 数量 。
					int rest_pages_count = (int)((old_size - new_size) / ZEND_MM_PAGE_SIZE);

#if ZEND_MM_STAT
					heap->size -= rest_pages_count * ZEND_MM_PAGE_SIZE;
#endif
					// 更新这个配置的地图信息，large块的page数量
					chunk->map[page_num] = ZEND_MM_LRUN(new_pages_count);
					// 把多出来page加到空闲中
					chunk->free_pages += rest_pages_count;
					// 结尾这一段标记成空闲
					zend_mm_bitset_reset_range(chunk->free_map, page_num + new_pages_count, rest_pages_count);
#if ZEND_DEBUG
					dbg = zend_mm_get_debug_info(heap, ptr);
					dbg->size = real_size;
					dbg->filename = __zend_filename;
					dbg->orig_filename = __zend_orig_filename;
					dbg->lineno = __zend_lineno;
					dbg->orig_lineno = __zend_orig_lineno;
#endif
					// 返回
					return ptr;
					
				// 如果要分配得更大	，large 块想要多少个page 都可以，只要有那么多空闲，不像small 块有固定的一串多少个page
				} else /* if (new_size > old_size) */ {
					// 新的页数
					int new_pages_count = (int)(new_size / ZEND_MM_PAGE_SIZE);
					// 旧的页数 
					int old_pages_count = (int)(old_size / ZEND_MM_PAGE_SIZE);

					// 先尝试从 本块 后面的空闲page 里分配
					/* try to allocate tail pages after this block */
					// 如果后面有足够多的page 
					if (page_num + new_pages_count <= ZEND_MM_PAGES &&
						// 并且 这些 page 全是空闲的
					    zend_mm_bitset_is_free_range(chunk->free_map, page_num + old_pages_count, new_pages_count - old_pages_count)) {
// 内存统计
#if ZEND_MM_STAT
						do {
							size_t size = heap->size + (new_size - old_size);
							size_t peak = MAX(heap->peak, size);
							heap->size = size;
							heap->peak = peak;
						} while (0);
#endif
						// 减少空闲page
						chunk->free_pages -= new_pages_count - old_pages_count;
						// 把后面的page标记成使用
						zend_mm_bitset_set_range(chunk->free_map, page_num + old_pages_count, new_pages_count - old_pages_count);
						// 更新地图，把新的page 数写进进去（只更新page串中第一个page的地图）
						chunk->map[page_num] = ZEND_MM_LRUN(new_pages_count);
#if ZEND_DEBUG
						dbg = zend_mm_get_debug_info(heap, ptr);
						dbg->size = real_size;
						dbg->filename = __zend_filename;
						dbg->orig_filename = __zend_orig_filename;
						dbg->lineno = __zend_lineno;
						dbg->orig_lineno = __zend_orig_lineno;
#endif
						// 返回指针
						return ptr;
					}
					// 如果不够，走 zend_mm_realloc_slow
				}
			}
			// 如果需要的尺寸不在 large 块范围内，走 zend_mm_realloc_slow
		}
#if ZEND_DEBUG
		size = real_size;
#endif
	}

	// 要复制的大小
	copy_size = MIN(old_size, copy_size);
	// 慢速分配
	return zend_mm_realloc_slow(heap, ptr, size, copy_size ZEND_FILE_LINE_RELAY_CC ZEND_FILE_LINE_ORIG_RELAY_CC);
}

// ？again ?
/*********************/
/* Huge Runs (again) */
/*********************/

// ing4, 添加 huge 块
#if ZEND_DEBUG
static void zend_mm_add_huge_block(zend_mm_heap *heap, void *ptr, size_t size, size_t dbg_size ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC)
#else
static void zend_mm_add_huge_block(zend_mm_heap *heap, void *ptr, size_t size ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC)
#endif
{
	// 这里添加不是这块内存，而指向这块内存的指针的封装
	// 创建一个节点
	zend_mm_huge_list *list = (zend_mm_huge_list*)zend_mm_alloc_heap(heap, sizeof(zend_mm_huge_list) ZEND_FILE_LINE_RELAY_CC ZEND_FILE_LINE_ORIG_RELAY_CC);
	// 内存指针
	list->ptr = ptr;
	// 大小
	list->size = size;
	// 下一个huge块指到列表开头
	list->next = heap->huge_list;
#if ZEND_DEBUG
	list->dbg.size = dbg_size;
	list->dbg.filename = __zend_filename;
	list->dbg.orig_filename = __zend_orig_filename;
	list->dbg.lineno = __zend_lineno;
	list->dbg.orig_lineno = __zend_orig_lineno;
#endif
	// 本块（的指针封装）作为huge块开头
	heap->huge_list = list;
}

// ing4, 删除huge块
static size_t zend_mm_del_huge_block(zend_mm_heap *heap, void *ptr ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC)
{
	// prev，list 都是临时变量
	zend_mm_huge_list *prev = NULL;
	zend_mm_huge_list *list = heap->huge_list;
	// 遍历所huge有块
	while (list != NULL) {
		// 找到要删除的块
		if (list->ptr == ptr) {
			size_t size;
			// 如果有前一个
			if (prev) {
				// 把自己摘出来 
				prev->next = list->next;
			// 没有前一个
			} else {
				// 把huge_list 指到下一个
				heap->huge_list = list->next;
			}
			// 大小
			size = list->size;
			// 释放这块内存
			zend_mm_free_heap(heap, list ZEND_FILE_LINE_RELAY_CC ZEND_FILE_LINE_ORIG_RELAY_CC);
			// 返回大小
			return size;
		}
		// 切到下一个
		prev = list;
		// 
		list = list->next;
	}
	// 如果没找到，报错 zend_mm_heap 毁坏
	ZEND_MM_CHECK(0, "zend_mm_heap corrupted");
	return 0;
}

// clear, huge 块的大小
static size_t zend_mm_get_huge_block_size(zend_mm_heap *heap, void *ptr ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC)
{
	// 
	zend_mm_huge_list *list = heap->huge_list;
	// 遍历所有的huge块
	while (list != NULL) {
		// 如果是查找的块
		if (list->ptr == ptr) {
			// 返回size
			return list->size;
		}
		// 下一个
		list = list->next;
	}
	// 报错 zend_mm_heap 毁坏
	ZEND_MM_CHECK(0, "zend_mm_heap corrupted");
	// 没找到
	return 0;
}

//
#if ZEND_DEBUG
// 
static void zend_mm_change_huge_block_size(zend_mm_heap *heap, void *ptr, size_t size, size_t dbg_size ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC)
#else
// clear, 改变huge块尺寸
static void zend_mm_change_huge_block_size(zend_mm_heap *heap, void *ptr, size_t size ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC)
#endif
{
	zend_mm_huge_list *list = heap->huge_list;
	// 遍历列表
	while (list != NULL) {
		// 找到这个块
		if (list->ptr == ptr) {
			// 写入size
			list->size = size;
// 调试
#if ZEND_DEBUG
			list->dbg.size = dbg_size;
			list->dbg.filename = __zend_filename;
			list->dbg.orig_filename = __zend_orig_filename;
			list->dbg.lineno = __zend_lineno;
			list->dbg.orig_lineno = __zend_orig_lineno;
#endif
			return;
		}
		// 没找到，下一个
		list = list->next;
	}
}

// ing3, 分配 huge 块：分配一个大的chunk
static void *zend_mm_alloc_huge(zend_mm_heap *heap, size_t size ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC)
{
// windows系统 
#ifdef ZEND_WIN32
	/* On Windows we don't have ability to extend huge blocks in-place.
	 * We allocate them with 2MB size granularity, to avoid many
	 * reallocations when they are extended by small pieces
	 */
	size_t alignment = MAX(REAL_PAGE_SIZE, ZEND_MM_CHUNK_SIZE);
// 不是 windows 系统 
#else
	size_t alignment = REAL_PAGE_SIZE;
#endif
	// 向后对齐 alignment
	size_t new_size = ZEND_MM_ALIGNED_SIZE_EX(size, alignment);
	void *ptr;

	// 新size 肯定会更大，否则就是内存位置算超了
	if (UNEXPECTED(new_size < size)) {
		zend_error_noreturn(E_ERROR, "Possible integer overflow in memory allocation (%zu + %zu)", size, alignment);
	}
// 内存限制 
#if ZEND_MM_LIMIT
	if (UNEXPECTED(new_size > heap->limit - heap->real_size)) {
		// 回收闲置内存
		if (zend_mm_gc(heap) && new_size <= heap->limit - heap->real_size) {
			/* pass */
		} else if (heap->overflow == 0) {
#if ZEND_DEBUG
			zend_mm_safe_error(heap, "Allowed memory size of %zu bytes exhausted at %s:%d (tried to allocate %zu bytes)", heap->limit, __zend_filename, __zend_lineno, size);
#else
			zend_mm_safe_error(heap, "Allowed memory size of %zu bytes exhausted (tried to allocate %zu bytes)", heap->limit, size);
#endif
			return NULL;
		}
	}
#endif
	// 分配大chunk
	ptr = zend_mm_chunk_alloc(heap, new_size, ZEND_MM_CHUNK_SIZE);
	// 如果分配失败
	if (UNEXPECTED(ptr == NULL)) {
		// 内存不足
		/* insufficient memory */
		// 回收闲置内存
		if (zend_mm_gc(heap) &&
		    (ptr = zend_mm_chunk_alloc(heap, new_size, ZEND_MM_CHUNK_SIZE)) != NULL) {
			/* pass */
		} else {
// 内存限制 
#if !ZEND_MM_LIMIT
			zend_mm_safe_error(heap, "Out of memory");
#elif ZEND_DEBUG
			zend_mm_safe_error(heap, "Out of memory (allocated %zu bytes) at %s:%d (tried to allocate %zu bytes)", heap->real_size, __zend_filename, __zend_lineno, size);
#else
			zend_mm_safe_error(heap, "Out of memory (allocated %zu bytes) (tried to allocate %zu bytes)", heap->real_size, size);
#endif
			return NULL;
		}
	}

// 调试
#if ZEND_DEBUG
	zend_mm_add_huge_block(heap, ptr, new_size, size ZEND_FILE_LINE_RELAY_CC ZEND_FILE_LINE_ORIG_RELAY_CC);
#else
	zend_mm_add_huge_block(heap, ptr, new_size ZEND_FILE_LINE_RELAY_CC ZEND_FILE_LINE_ORIG_RELAY_CC);
#endif

// 内存统计
#if ZEND_MM_STAT
	do {
		size_t size = heap->real_size + new_size;
		size_t peak = MAX(heap->real_peak, size);
		heap->real_size = size;
		heap->real_peak = peak;
	} while (0);
	do {
		size_t size = heap->size + new_size;
		size_t peak = MAX(heap->peak, size);
		heap->size = size;
		heap->peak = peak;
	} while (0);
// 内存限制 ，逻辑少很多
#elif ZEND_MM_LIMIT
	heap->real_size += new_size;
#endif
	return ptr;
}

// ing4, 释放 huge
static void zend_mm_free_huge(zend_mm_heap *heap, void *ptr ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC)
{
	size_t size;
	// huge 块必须对齐到 ZEND_MM_CHUNK_SIZE
	ZEND_MM_CHECK(ZEND_MM_ALIGNED_OFFSET(ptr, ZEND_MM_CHUNK_SIZE) == 0, "zend_mm_heap corrupted");
	// 找到并删除 zend_mm_huge_list链表元素，返回元素中存储的内存大小
	size = zend_mm_del_huge_block(heap, ptr ZEND_FILE_LINE_RELAY_CC ZEND_FILE_LINE_ORIG_RELAY_CC);
	// 这里才是真正的释放内存
	zend_mm_chunk_free(heap, ptr, size);
#if ZEND_MM_STAT || ZEND_MM_LIMIT
	heap->real_size -= size;
#endif
#if ZEND_MM_STAT
	heap->size -= size;
#endif
}

// 初始化
/******************/
/* Initialization */
/******************/

// ing4, 初始化 zend 内存管理, 只有 alloc_globals_ctor 调用，禁用zend内存管理时调用
static zend_mm_heap *zend_mm_init(void)
{
	// 分配一个对齐到 ZEND_MM_CHUNK_SIZE 的 chunk，大小是2M。
	zend_mm_chunk *chunk = (zend_mm_chunk*)zend_mm_chunk_alloc_int(ZEND_MM_CHUNK_SIZE, ZEND_MM_CHUNK_SIZE);
	// 
	zend_mm_heap *heap;

	// 如果分配失败，中断执行
	if (UNEXPECTED(chunk == NULL)) {
#if ZEND_MM_ERROR
		fprintf(stderr, "Can't initialize heap\n");
#endif
		return NULL;
	}
	// 下面这些东西都在chunk的第一个page 里
	// chunk 里自带了一个 heap 而不是指针
	heap = &chunk->heap_slot;
	// 所属 heap
	chunk->heap = heap;
	// next ,prev 指向自己
	chunk->next = chunk;
	chunk->prev = chunk;
	// -1 去掉头page
	chunk->free_pages = ZEND_MM_PAGES - ZEND_MM_FIRST_PAGE;
	// 结尾空闲页码
	chunk->free_tail = ZEND_MM_FIRST_PAGE;
	// chunk序号，main_chunk是 0
	chunk->num = 0;
	// bitmap里把第一个page标记成已使用
	chunk->free_map[0] = (Z_L(1) << ZEND_MM_FIRST_PAGE) - 1;
	// 第一个page的地图信息
	chunk->map[0] = ZEND_MM_LRUN(ZEND_MM_FIRST_PAGE);
	// 根 chunk，main_chunk 是不会被替换掉的，它从一开始创建就是这一个
	heap->main_chunk = chunk;
	// 缓存 chunks
	heap->cached_chunks = NULL;
	// chunks 数量
	heap->chunks_count = 1;
	// 峰值chunks数量
	heap->peak_chunks_count = 1;
	// 缓存chunks数量 
	heap->cached_chunks_count = 0;
	// 平均chunks数量
	heap->avg_chunks_count = 1.0;
	// 自动清理阀值
	heap->last_chunks_delete_boundary = 0;
	// 自动清理次数
	heap->last_chunks_delete_count = 0;
#if ZEND_MM_STAT || ZEND_MM_LIMIT
	// 2M
	heap->real_size = ZEND_MM_CHUNK_SIZE;
#endif
#if ZEND_MM_STAT
	// 2M
	heap->real_peak = ZEND_MM_CHUNK_SIZE;
	// 当前使用内存大小
	heap->size = 0;
	// 内存使用峰值
	heap->peak = 0;
#endif
#if ZEND_MM_LIMIT
	// 这是在获取有符号整数的最大值
	heap->limit = ((size_t)Z_L(-1) >> (size_t)Z_L(1));
	// 无溢出
	heap->overflow = 0;
#endif
#if ZEND_MM_CUSTOM
	// 0，使用用户heap
	heap->use_custom_heap = ZEND_MM_CUSTOM_HEAP_NONE;
#endif
#if ZEND_MM_STORAGE
	// 这里设置成 null了，后面什么时候初始化它呢？
	heap->storage = NULL;
#endif
	// huge块列表
	heap->huge_list = NULL;
	return heap;
}

// ing3,棒棒哒内存回收机制，回收空闲的page和chunk, 内部5次外部1次
ZEND_API size_t zend_mm_gc(zend_mm_heap *heap)
{
	zend_mm_free_slot *p, **q;
	zend_mm_chunk *chunk;
	size_t page_offset;
	int page_num;
	zend_mm_page_info info;
	uint32_t i, free_counter;
	bool has_free_pages;
	size_t collected = 0;

#if ZEND_MM_CUSTOM
	if (heap->use_custom_heap) {
		return 0;
	}
#endif
	// 遍历每一组 small 块 ，	// ZEND_MM_BINS = 30，一共30组
	for (i = 0; i < ZEND_MM_BINS; i++) {
		// 无空闲page
		has_free_pages = false;
		// 第 i 串空闲 small 块
		p = heap->free_slot[i];
		// 如果 p 存在。这里要遍历一串所有的small块。
		while (p != NULL) {
			// 取回所在chunk的指针
			chunk = (zend_mm_chunk*)ZEND_MM_ALIGNED_BASE(p, ZEND_MM_CHUNK_SIZE);
			// 检验内存 ,找到的heap 必须是 当前heap
			ZEND_MM_CHECK(chunk->heap == heap, "zend_mm_heap corrupted");
			// 找到 p 相对于 ZEND_MM_CHUNK_SIZE 的偏移量
			page_offset = ZEND_MM_ALIGNED_OFFSET(p, ZEND_MM_CHUNK_SIZE);
			// 不可以没有偏移，因为chunk的开头是状态数据 （这只是个简单的检验）
			ZEND_ASSERT(page_offset != 0);
			// 找到偏移的 pages 数量 
			page_num = (int)(page_offset / ZEND_MM_PAGE_SIZE);
			// 找到这一页地图
			info = chunk->map[page_num];
			// small 块，必有有 SRUN 标记
			ZEND_ASSERT(info & ZEND_MM_IS_SRUN);
			// 如果有 LRUN 标记。说明是横跨多个 page 的串
			if (info & ZEND_MM_IS_LRUN) {
				// 找到这个串的第一个page
				page_num -= ZEND_MM_NRUN_OFFSET(info);
				// 找到这一页地图
				info = chunk->map[page_num];
				// 必须有 SRUN 标记
				ZEND_ASSERT(info & ZEND_MM_IS_SRUN);
				// 必须不能有 LRUN 标记，（第一个page没有，后面才有）
				ZEND_ASSERT(!(info & ZEND_MM_IS_LRUN));
			}
			// 取回 配置编号, 块信息里存放的页号必须正确
			ZEND_ASSERT(ZEND_MM_SRUN_BIN_NUM(info) == i);
			
			// 每个串的 map 不是同一个，所以每个串开始 是1
			free_counter = ZEND_MM_SRUN_FREE_COUNTER(info) + 1;
			// 连续的 小块 数量 == 配置中连续的的小块数量
			if (free_counter == bin_elements[i]) {
				// 如果空闲的 small 块和配置里一样，所以所有的small 块都是空闲的
				// 标记成有空闲页（整串都空闲）
				has_free_pages = true;
			}
			// map中记录空闲small 块数量 。ZEND_MM_SRUN_EX 全局 1 次应用
			chunk->map[page_num] = ZEND_MM_SRUN_EX(i, free_counter);
			// 找到一串中的下一个 small 块
			p = p->next_free_slot;
		}

		// 如果没有空page
		if (!has_free_pages) {
			// 跳过后面的逻辑
			continue;
		}

		// 如果有空间page, 处理空闲page。
		
		// q是指针的指针
		q = &heap->free_slot[i];
		// 
		p = *q;
		// 如果 p 有效
		while (p != NULL) {
			// 取回所在chunk的指针
			chunk = (zend_mm_chunk*)ZEND_MM_ALIGNED_BASE(p, ZEND_MM_CHUNK_SIZE);
			// chunk必须在当前 heap 里
			ZEND_MM_CHECK(chunk->heap == heap, "zend_mm_heap corrupted");
			// p 相对于 ZEND_MM_CHUNK_SIZE 的偏移量
			page_offset = ZEND_MM_ALIGNED_OFFSET(p, ZEND_MM_CHUNK_SIZE);
			// 因为是small块，偏移量不可以是 0
			ZEND_ASSERT(page_offset != 0);
			// 找到地图页码
			page_num = (int)(page_offset / ZEND_MM_PAGE_SIZE);
			// 获取这一页地图
			info = chunk->map[page_num];
			// small 块 必须有 SRUN 标记
			ZEND_ASSERT(info & ZEND_MM_IS_SRUN);
			// 如果有 LRUN 标记，是一串page
			if (info & ZEND_MM_IS_LRUN) {
				// 找到一串中的第一个 page
				page_num -= ZEND_MM_NRUN_OFFSET(info);
				// 找到这个 page 的地图
				info = chunk->map[page_num];
				// 必须有small 块标记
				ZEND_ASSERT(info & ZEND_MM_IS_SRUN);
				// 必须 没有large块标记
				ZEND_ASSERT(!(info & ZEND_MM_IS_LRUN));
			}
			// 取回 配置编号 , 编号必须一致
			ZEND_ASSERT(ZEND_MM_SRUN_BIN_NUM(info) == i);
			// 如果整串page都空闲 
			if (ZEND_MM_SRUN_FREE_COUNTER(info) == bin_elements[i]) {
				// 从缓存中删除？？？？？
				/* remove from cache */
				p = p->next_free_slot;
				*q = p;
			// 如果不是整串配置都空闲
			} else {
				q = &p->next_free_slot;
				p = *q;
			}
		}
	}

	// 从main_chunk开始
	chunk = heap->main_chunk;
	do {
		// 从第一页开始
		i = ZEND_MM_FIRST_PAGE;
		// 遍历到最后一个使用过的page（遍历检查所有page）
		while (i < chunk->free_tail) {
			// 如果这个page 已使用
			if (zend_mm_bitset_is_set(chunk->free_map, i)) {
				// 地图信息
				info = chunk->map[i];
				// 如果是 SRUN
				if (info & ZEND_MM_IS_SRUN) {
					// 取回 ZEND_MM_BINS_INFO 中的配置编号
					int bin_num = ZEND_MM_SRUN_BIN_NUM(info);
					// 使用的连续page数量
					int pages_count = bin_pages[bin_num];
					// 如果全部空闲（空闲的数量 = 配置数量）
					if (ZEND_MM_SRUN_FREE_COUNTER(info) == bin_elements[bin_num]) {
						/* all elements are free */
						// 回收，释放这一串 page
						zend_mm_free_pages_ex(heap, chunk, i, pages_count, 0);
						// 记录回收数量 
						collected += pages_count;
					} else {
						// 重置 counter
						/* reset counter */
						// 重新记录 bin_num ，这里清空了 COUNTER 记录
						chunk->map[i] = ZEND_MM_SRUN(bin_num);
					}
					// 向后跳 n 个page 
					i += bin_pages[bin_num];
				// 如果不是 SRUN， 那就只能是large 块
				} else /* if (info & ZEND_MM_IS_LRUN) */ {
					// 取得 LRUN 块的信息 ，跳过整个large块
					i += ZEND_MM_LRUN_PAGES(info);
				}
			// 如果没使用，下一个
			} else {
				i++;
			}
		}
		// 如果整个chunk都空闲了
		if (chunk->free_pages == ZEND_MM_PAGES - ZEND_MM_FIRST_PAGE) {
			// 转到下一个chunk
			zend_mm_chunk *next_chunk = chunk->next;
			// 回收这个chunk
			zend_mm_delete_chunk(heap, chunk);
			// 下个chunk
			chunk = next_chunk;
		// chunk 还在使用
		} else {
			// 下一个chunk
			chunk = chunk->next;
		}
	// 没到main_chunk就继续找
	} while (chunk != heap->main_chunk);

	// 返回收数量 
	return collected * ZEND_MM_PAGE_SIZE;
}

// 调试模式，泄露侦察
#if ZEND_DEBUG
/******************/
/* Leak detection */
/******************/

// suspend, 调试相关
static zend_long zend_mm_find_leaks_small(zend_mm_chunk *p, uint32_t i, uint32_t j, zend_leak_info *leak)
{
	bool empty = true;
	zend_long count = 0;
	// 取回 ZEND_MM_BINS_INFO 中的配置编号
	int bin_num = ZEND_MM_SRUN_BIN_NUM(p->map[i]);
	// bin_data_size : ZEND_MM_BINS_INFO中的 size列
	zend_mm_debug_info *dbg = (zend_mm_debug_info*)((char*)p + ZEND_MM_PAGE_SIZE * i + bin_data_size[bin_num] * (j + 1) - ZEND_MM_ALIGNED_SIZE(sizeof(zend_mm_debug_info)));

	while (j < bin_elements[bin_num]) {
		if (dbg->size != 0) {
			if (dbg->filename == leak->filename && dbg->lineno == leak->lineno) {
				count++;
				dbg->size = 0;
				dbg->filename = NULL;
				dbg->lineno = 0;
			} else {
				empty = false;
			}
		}
		j++;
		// bin_data_size : ZEND_MM_BINS_INFO中的 size列
		dbg = (zend_mm_debug_info*)((char*)dbg + bin_data_size[bin_num]);
	}
	if (empty) {
		zend_mm_bitset_reset_range(p->free_map, i, bin_pages[bin_num]);
	}
	return count;
}

// suspend, 调试相关
static zend_long zend_mm_find_leaks(zend_mm_heap *heap, zend_mm_chunk *p, uint32_t i, zend_leak_info *leak)
{
	zend_long count = 0;

	do {
		while (i < p->free_tail) {
			if (zend_mm_bitset_is_set(p->free_map, i)) {
				if (p->map[i] & ZEND_MM_IS_SRUN) {
					// 取回 ZEND_MM_BINS_INFO 中的配置编号
					int bin_num = ZEND_MM_SRUN_BIN_NUM(p->map[i]);
					count += zend_mm_find_leaks_small(p, i, 0, leak);
					i += bin_pages[bin_num];
				} else /* if (p->map[i] & ZEND_MM_IS_LRUN) */ {
					int pages_count = ZEND_MM_LRUN_PAGES(p->map[i]);
					zend_mm_debug_info *dbg = (zend_mm_debug_info*)((char*)p + ZEND_MM_PAGE_SIZE * (i + pages_count) - ZEND_MM_ALIGNED_SIZE(sizeof(zend_mm_debug_info)));

					if (dbg->filename == leak->filename && dbg->lineno == leak->lineno) {
						count++;
					}
					zend_mm_bitset_reset_range(p->free_map, i, pages_count);
					i += pages_count;
				}
			} else {
				i++;
			}
		}
		p = p->next;
		i = ZEND_MM_FIRST_PAGE;
	} while (p != heap->main_chunk);
	return count;
}

// suspend, 调试相关
static zend_long zend_mm_find_leaks_huge(zend_mm_heap *heap, zend_mm_huge_list *list)
{
	zend_long count = 0;
	zend_mm_huge_list *prev = list;
	zend_mm_huge_list *p = list->next;

	while (p) {
		if (p->dbg.filename == list->dbg.filename && p->dbg.lineno == list->dbg.lineno) {
			prev->next = p->next;
			zend_mm_chunk_free(heap, p->ptr, p->size);
			zend_mm_free_heap(heap, p, NULL, 0, NULL, 0);
			count++;
		} else {
			prev = p;
		}
		p = prev->next;
	}

	return count;
}

// suspend, 调试相关
static void zend_mm_check_leaks(zend_mm_heap *heap)
{
	zend_mm_huge_list *list;
	zend_mm_chunk *p;
	zend_leak_info leak;
	zend_long repeated = 0;
	uint32_t total = 0;
	uint32_t i, j;

	/* find leaked huge blocks and free them */
	list = heap->huge_list;
	while (list) {
		zend_mm_huge_list *q = list;

		leak.addr = list->ptr;
		leak.size = list->dbg.size;
		leak.filename = list->dbg.filename;
		leak.orig_filename = list->dbg.orig_filename;
		leak.lineno = list->dbg.lineno;
		leak.orig_lineno = list->dbg.orig_lineno;

		zend_message_dispatcher(ZMSG_LOG_SCRIPT_NAME, NULL);
		zend_message_dispatcher(ZMSG_MEMORY_LEAK_DETECTED, &leak);
		repeated = zend_mm_find_leaks_huge(heap, list);
		total += 1 + repeated;
		if (repeated) {
			zend_message_dispatcher(ZMSG_MEMORY_LEAK_REPEATED, (void *)(zend_uintptr_t)repeated);
		}

		heap->huge_list = list = list->next;
		zend_mm_chunk_free(heap, q->ptr, q->size);
		zend_mm_free_heap(heap, q, NULL, 0, NULL, 0);
	}

	/* for each chunk */
	p = heap->main_chunk;
	do {
		i = ZEND_MM_FIRST_PAGE;
		while (i < p->free_tail) {
			if (zend_mm_bitset_is_set(p->free_map, i)) {
				if (p->map[i] & ZEND_MM_IS_SRUN) {
					// 取回 ZEND_MM_BINS_INFO 中的配置编号
					int bin_num = ZEND_MM_SRUN_BIN_NUM(p->map[i]);
					// bin_data_size : ZEND_MM_BINS_INFO中的 size列
					zend_mm_debug_info *dbg = (zend_mm_debug_info*)((char*)p + ZEND_MM_PAGE_SIZE * i + bin_data_size[bin_num] - ZEND_MM_ALIGNED_SIZE(sizeof(zend_mm_debug_info)));

					j = 0;
					while (j < bin_elements[bin_num]) {
						if (dbg->size != 0) {
							// bin_data_size : ZEND_MM_BINS_INFO中的 size列
							leak.addr = (zend_mm_debug_info*)((char*)p + ZEND_MM_PAGE_SIZE * i + bin_data_size[bin_num] * j);
							leak.size = dbg->size;
							leak.filename = dbg->filename;
							leak.orig_filename = dbg->orig_filename;
							leak.lineno = dbg->lineno;
							leak.orig_lineno = dbg->orig_lineno;

							zend_message_dispatcher(ZMSG_LOG_SCRIPT_NAME, NULL);
							zend_message_dispatcher(ZMSG_MEMORY_LEAK_DETECTED, &leak);

							dbg->size = 0;
							dbg->filename = NULL;
							dbg->lineno = 0;

							repeated = zend_mm_find_leaks_small(p, i, j + 1, &leak) +
							           zend_mm_find_leaks(heap, p, i + bin_pages[bin_num], &leak);
							total += 1 + repeated;
							if (repeated) {
								zend_message_dispatcher(ZMSG_MEMORY_LEAK_REPEATED, (void *)(zend_uintptr_t)repeated);
							}
						}
						// bin_data_size : ZEND_MM_BINS_INFO中的 size列
						dbg = (zend_mm_debug_info*)((char*)dbg + bin_data_size[bin_num]);
						j++;
					}
					i += bin_pages[bin_num];
				} else /* if (p->map[i] & ZEND_MM_IS_LRUN) */ {
					int pages_count = ZEND_MM_LRUN_PAGES(p->map[i]);
					zend_mm_debug_info *dbg = (zend_mm_debug_info*)((char*)p + ZEND_MM_PAGE_SIZE * (i + pages_count) - ZEND_MM_ALIGNED_SIZE(sizeof(zend_mm_debug_info)));

					leak.addr = (void*)((char*)p + ZEND_MM_PAGE_SIZE * i);
					leak.size = dbg->size;
					leak.filename = dbg->filename;
					leak.orig_filename = dbg->orig_filename;
					leak.lineno = dbg->lineno;
					leak.orig_lineno = dbg->orig_lineno;

					zend_message_dispatcher(ZMSG_LOG_SCRIPT_NAME, NULL);
					zend_message_dispatcher(ZMSG_MEMORY_LEAK_DETECTED, &leak);

					zend_mm_bitset_reset_range(p->free_map, i, pages_count);

					repeated = zend_mm_find_leaks(heap, p, i + pages_count, &leak);
					total += 1 + repeated;
					if (repeated) {
						zend_message_dispatcher(ZMSG_MEMORY_LEAK_REPEATED, (void *)(zend_uintptr_t)repeated);
					}
					i += pages_count;
				}
			} else {
				i++;
			}
		}
		p = p->next;
	} while (p != heap->main_chunk);
	if (total) {
		zend_message_dispatcher(ZMSG_MEMORY_LEAKS_GRAND_TOTAL, &total);
	}
}
#endif

// 1
#if ZEND_MM_CUSTOM
static void *tracked_malloc(size_t size);
static void tracked_free_all(void);
#endif

// ing3, 关闭内存管理
void zend_mm_shutdown(zend_mm_heap *heap, bool full, bool silent)
{
	zend_mm_chunk *p;
	zend_mm_huge_list *list;

// 1
#if ZEND_MM_CUSTOM
	// 如果使用客户定义的内存管理
	if (heap->use_custom_heap) {
		// 如果用 tracked_malloc 方法分配内存
		if (heap->custom_heap.std._malloc == tracked_malloc) {
			// 如果要求静默
			if (silent) {
				// 释放所有
				tracked_free_all();
			}
			// 清空哈希表
			zend_hash_clean(heap->tracked_allocs);
			// 要求全部销毁
			if (full) {
				// 销毁哈希表
				zend_hash_destroy(heap->tracked_allocs);
				// 释放分配的内存
				free(heap->tracked_allocs);
				// 保证下面的释放内存，没有用到 tracked_free
				/* Make sure the heap free below does not use tracked_free(). */
				heap->custom_heap.std._free = free;
			}
			// 大小设置为0
			heap->size = 0;
		}

		// 全部销毁
		if (full) {
			// 调试模式
			if (ZEND_DEBUG && heap->use_custom_heap == ZEND_MM_CUSTOM_HEAP_DEBUG) {
				heap->custom_heap.debug._free(heap ZEND_FILE_LINE_CC ZEND_FILE_LINE_EMPTY_CC);
			// 正式模式
			} else {
				// 释放整个heap
				heap->custom_heap.std._free(heap);
			}
		}
		return;
	}
#endif

#if ZEND_DEBUG
	// 
	if (!silent) {
		zend_mm_check_leaks(heap);
	}
#endif

	// 删除 huge 块
	/* free huge blocks */
	list = heap->huge_list;
	heap->huge_list = NULL;
	// 依次遍历
	while (list) {
		zend_mm_huge_list *q = list;
		list = list->next;
		// 删除每个块，这里没有用free_large什么的，因为可以直接删掉，不管其他了
		zend_mm_chunk_free(heap, q->ptr, q->size);
	}

	// 除了第一个chunk，其他全都放到cache里
	/* move all chunks except of the first one into the cache */
	p = heap->main_chunk->next;
	// main_chunk 不操作
	while (p != heap->main_chunk) {
		// 先取得下一块的指针，防止丢失
		zend_mm_chunk *q = p->next;
		// 把p接到cache chunks 开头
		p->next = heap->cached_chunks;
		// p作为cache串开头
		heap->cached_chunks = p;
		// 下一个
		p = q;
		//chunk数 -1
		heap->chunks_count--;
		//缓存chunk数 +1
		heap->cached_chunks_count++;
	}

	// 如果要求全部清理掉
	if (full) {
		// 释放所有缓存chunk
		/* free all cached chunks */
		while (heap->cached_chunks) {
			p = heap->cached_chunks;
			heap->cached_chunks = p->next;
			// 依次释放内存
			zend_mm_chunk_free(heap, p, ZEND_MM_CHUNK_SIZE);
		}
		// 释放main_chunk
		/* free the first chunk */
		zend_mm_chunk_free(heap, heap->main_chunk, ZEND_MM_CHUNK_SIZE);
	// 如果不要全部清理掉
	} else {
		// 删除部分缓存 chunk，保留平均使用数量
		/* free some cached chunks to keep average count */
		// 平均数 = (旧平均数+峰值数)/2 , (666!)
		heap->avg_chunks_count = (heap->avg_chunks_count + (double)heap->peak_chunks_count) / 2.0;
		// 如果缓存chunk数 + 0.9(进1）大于平均使用chunk数 ，并且有缓存 chunk存在
		while ((double)heap->cached_chunks_count + 0.9 > heap->avg_chunks_count &&
		       heap->cached_chunks) {
			// 删除部分chunk，留下符合要求的数量 
			p = heap->cached_chunks;
			heap->cached_chunks = p->next;
			zend_mm_chunk_free(heap, p, ZEND_MM_CHUNK_SIZE);
			heap->cached_chunks_count--;
		}
		// 清空 缓存 chunk
		/* clear cached chunks */
		p = heap->cached_chunks;
		// 依次遍历所有缓存 chunk
		while (p != NULL) {
			zend_mm_chunk *q = p->next;
			// 全部用 0 填充
			memset(p, 0, sizeof(zend_mm_chunk));
			p->next = q;
			p = q;
		}

		// 重新初始化第一个chunk和heap
		/* reinitialize the first chunk and heap */
		// 获取main_chunk
		p = heap->main_chunk;
		// help指针指向自己的 heap_slot 属性
		p->heap = &p->heap_slot;
		// 下一个指向自己
		p->next = p;
		//  前一个指向自己
		p->prev = p;
		// 重置free_page 数量，511
		p->free_pages = ZEND_MM_PAGES - ZEND_MM_FIRST_PAGE;
		// 空闲page位置为 1
		p->free_tail = ZEND_MM_FIRST_PAGE;
		// 序号为 0
		p->num = 0;

#if ZEND_MM_STAT
		// 清空内存大小
		heap->size = heap->peak = 0;
#endif
		// 重置 free_slot
		memset(heap->free_slot, 0, sizeof(heap->free_slot));
#if ZEND_MM_STAT || ZEND_MM_LIMIT
		// 内存使用量
		heap->real_size = (heap->cached_chunks_count + 1) * ZEND_MM_CHUNK_SIZE;
#endif
#if ZEND_MM_STAT
		// 内存峰值
		heap->real_peak = (heap->cached_chunks_count + 1) * ZEND_MM_CHUNK_SIZE;
#endif
		// chunk数为 1
		heap->chunks_count = 1;
		// chunk数峰值为 1
		heap->peak_chunks_count = 1;
		// 自动清理边界值为0
		heap->last_chunks_delete_boundary = 0;
		// 自动清理次数为0
		heap->last_chunks_delete_count = 0;

		// 空闲清空地图和地图信息，两块内存挨着，所以一起清理了
		memset(p->free_map, 0, sizeof(p->free_map) + sizeof(p->map));
		// 1<<1 -1 = 1，只有第一个page标记成使用中
		p->free_map[0] = (1L << ZEND_MM_FIRST_PAGE) - 1;
		// 地图信息标记成 large 块（soga!）
		p->map[0] = ZEND_MM_LRUN(ZEND_MM_FIRST_PAGE);
	}
}

// 开放接口
/**************/
/* PUBLIC API */
/**************/

// clear, 分配内存
ZEND_API void* ZEND_FASTCALL _zend_mm_alloc(zend_mm_heap *heap, size_t size ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC)
{
	return zend_mm_alloc_heap(heap, size ZEND_FILE_LINE_RELAY_CC ZEND_FILE_LINE_ORIG_RELAY_CC);
}

// clear, 释放内存
ZEND_API void ZEND_FASTCALL _zend_mm_free(zend_mm_heap *heap, void *ptr ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC)
{
	zend_mm_free_heap(heap, ptr ZEND_FILE_LINE_RELAY_CC ZEND_FILE_LINE_ORIG_RELAY_CC);
}

// clear, 调整内存大小，不使用 copysize 
void* ZEND_FASTCALL _zend_mm_realloc(zend_mm_heap *heap, void *ptr, size_t size ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC)
{
	// 不使用 copysize 
	return zend_mm_realloc_heap(heap, ptr, size, 0, size ZEND_FILE_LINE_RELAY_CC ZEND_FILE_LINE_ORIG_RELAY_CC);
}

// clear, 调整内存大小，使用 copysize 
void* ZEND_FASTCALL _zend_mm_realloc2(zend_mm_heap *heap, void *ptr, size_t size, size_t copy_size ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC)
{
	// 使用 copysize 
	return zend_mm_realloc_heap(heap, ptr, size, 1, copy_size ZEND_FILE_LINE_RELAY_CC ZEND_FILE_LINE_ORIG_RELAY_CC);
}

// clear, 返回内存块大小
ZEND_API size_t ZEND_FASTCALL _zend_mm_block_size(zend_mm_heap *heap, void *ptr ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC)
{
	return zend_mm_size(heap, ptr ZEND_FILE_LINE_RELAY_CC ZEND_FILE_LINE_ORIG_RELAY_CC);
}

// 分配管理器
/**********************/
/* Allocation Manager */
/**********************/

// AG : alloc_globals
// 全局的内存分配节点 里面有一个指向内存heap的指针
typedef struct _zend_alloc_globals {
	zend_mm_heap *mm_heap;
} zend_alloc_globals;

// 如果要求线程安全
#ifdef ZTS
static int alloc_globals_id;
static size_t alloc_globals_offset;
//
# define AG(v) ZEND_TSRMG_FAST(alloc_globals_offset, zend_alloc_globals *, v)
// 如果不要求线程安全
#else
// ing4, 用于全局访问内存管理数据 
# define AG(v) (alloc_globals.v)
// 外部变量 alloc_globals
static zend_alloc_globals alloc_globals;
#endif

// clear 是否使用zend内存分配 
ZEND_API bool is_zend_mm(void)
{
//
#if ZEND_MM_CUSTOM
	// 没有用户自定义的内存
	return !AG(mm_heap)->use_custom_heap;
#else
	return 1;
#endif
}

// ing4, 是否是内存指针
ZEND_API bool is_zend_ptr(const void *ptr)
{
// 如果没有用 zend_heap ,一定不是zend_ptr
#if ZEND_MM_CUSTOM
	if (AG(mm_heap)->use_custom_heap) {
		return 0;
	}
#endif

	// 检查所有 chunk
	if (AG(mm_heap)->main_chunk) {
		// 从 main_chunk 开始
		zend_mm_chunk *chunk = AG(mm_heap)->main_chunk;

		do {
			// 如果指针在当前 chunk范围内，就是zend指针
			if (ptr >= (void*)chunk
			 && ptr < (void*)((char*)chunk + ZEND_MM_CHUNK_SIZE)) {
				return 1;
			}
			// 下一个
			chunk = chunk->next;
		// 如果没指回到 main_chunk
		} while (chunk != AG(mm_heap)->main_chunk);
	}

	// 检查所有 huge
	if (AG(mm_heap)->huge_list) {
		// 从 huge_list 开始
		zend_mm_huge_list *block = AG(mm_heap)->huge_list;

		do {
			// 如果指针在当前 block范围内，就是zend指针
			if (ptr >= (void*)block
			 && ptr < (void*)((char*)block + block->size)) {
				return 1;
			}
			// 下一个block
			block = block->next;
		// 如果没指回到 huge_list
		} while (block != AG(mm_heap)->huge_list);
	}
	// 没找到，返回 false 
	return 0;
}

#if ZEND_MM_CUSTOM

// ing4, 调用客户自定义的 分配内存方法
static ZEND_COLD void* ZEND_FASTCALL _malloc_custom(size_t size ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC)
{
	if (ZEND_DEBUG && AG(mm_heap)->use_custom_heap == ZEND_MM_CUSTOM_HEAP_DEBUG) {
		return AG(mm_heap)->custom_heap.debug._malloc(size ZEND_FILE_LINE_RELAY_CC ZEND_FILE_LINE_ORIG_RELAY_CC);
	} else {
		return AG(mm_heap)->custom_heap.std._malloc(size);
	}
}

// ing4, 调用客户自定义的 内存释放方法
static ZEND_COLD void ZEND_FASTCALL _efree_custom(void *ptr ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC)
{
	if (ZEND_DEBUG && AG(mm_heap)->use_custom_heap == ZEND_MM_CUSTOM_HEAP_DEBUG) {
		AG(mm_heap)->custom_heap.debug._free(ptr ZEND_FILE_LINE_RELAY_CC ZEND_FILE_LINE_ORIG_RELAY_CC);
	} else {
		AG(mm_heap)->custom_heap.std._free(ptr);
	}
}

// ing4, 调用客户自定义的 重新分配内存方法
static ZEND_COLD void* ZEND_FASTCALL _realloc_custom(void *ptr, size_t size ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC)
{
	if (ZEND_DEBUG && AG(mm_heap)->use_custom_heap == ZEND_MM_CUSTOM_HEAP_DEBUG) {
		return AG(mm_heap)->custom_heap.debug._realloc(ptr, size ZEND_FILE_LINE_RELAY_CC ZEND_FILE_LINE_ORIG_RELAY_CC);
	} else {
		return AG(mm_heap)->custom_heap.std._realloc(ptr, size);
	}
}
#endif

#if !ZEND_DEBUG && defined(HAVE_BUILTIN_CONSTANT_P)
#undef _emalloc

// 1
#if ZEND_MM_CUSTOM
// ing3，如果指定了客户自定义内存 ，调用自定义方法分配内存
# define ZEND_MM_CUSTOM_ALLOCATOR(size) do { \
		if (UNEXPECTED(AG(mm_heap)->use_custom_heap)) { \
			return _malloc_custom(size ZEND_FILE_LINE_RELAY_CC ZEND_FILE_LINE_ORIG_RELAY_CC); \
		} \
	} while (0)
// ing3, 如果指定了客户自定义内存 ，调用自定义方法释放内存
# define ZEND_MM_CUSTOM_DEALLOCATOR(ptr) do { \
		if (UNEXPECTED(AG(mm_heap)->use_custom_heap)) { \
			_efree_custom(ptr ZEND_FILE_LINE_RELAY_CC ZEND_FILE_LINE_ORIG_RELAY_CC); \
			return; \
		} \
	} while (0)
//
#else
# define ZEND_MM_CUSTOM_ALLOCATOR(size)
# define ZEND_MM_CUSTOM_DEALLOCATOR(ptr)
#endif

// ing4, 生成一个 _emalloc_ ## _size 方法，用来快速分配指定size 的内存
# define _ZEND_BIN_ALLOCATOR(_num, _size, _elements, _pages, x, y) \
	ZEND_API void* ZEND_FASTCALL _emalloc_ ## _size(void) { \
		ZEND_MM_CUSTOM_ALLOCATOR(_size); \
		return zend_mm_alloc_small(AG(mm_heap), _num ZEND_FILE_LINE_RELAY_CC ZEND_FILE_LINE_ORIG_RELAY_CC); \
	}

// 为每个尺寸的 small 块定义一个单独的 alloc方法
ZEND_MM_BINS_INFO(_ZEND_BIN_ALLOCATOR, x, y)

// ing3, 分配large块
ZEND_API void* ZEND_FASTCALL _emalloc_large(size_t size ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC)
{
	// 如果有用户自定义的，直接调用，这里面有return 语句
	ZEND_MM_CUSTOM_ALLOCATOR(size);
	return zend_mm_alloc_large_ex(AG(mm_heap), size ZEND_FILE_LINE_RELAY_CC ZEND_FILE_LINE_ORIG_RELAY_CC);
}

// ing3, 分配small块
ZEND_API void* ZEND_FASTCALL _emalloc_huge(size_t size)
{
	// 如果有用户自定义的，直接调用，这里面有return 语句
	ZEND_MM_CUSTOM_ALLOCATOR(size);
	return zend_mm_alloc_huge(AG(mm_heap), size);
}

// 调试用
#if ZEND_DEBUG
# define _ZEND_BIN_FREE(_num, _size, _elements, _pages, x, y) \
	ZEND_API void ZEND_FASTCALL _efree_ ## _size(void *ptr) { \
		ZEND_MM_CUSTOM_DEALLOCATOR(ptr); \
		{ \
			size_t page_offset = ZEND_MM_ALIGNED_OFFSET(ptr, ZEND_MM_CHUNK_SIZE); \
			zend_mm_chunk *chunk = (zend_mm_chunk*)ZEND_MM_ALIGNED_BASE(ptr, ZEND_MM_CHUNK_SIZE); \
			int page_num = page_offset / ZEND_MM_PAGE_SIZE; \
			ZEND_MM_CHECK(chunk->heap == AG(mm_heap), "zend_mm_heap corrupted"); \
			ZEND_ASSERT(chunk->map[page_num] & ZEND_MM_IS_SRUN); \
			ZEND_ASSERT(ZEND_MM_SRUN_BIN_NUM(chunk->map[page_num]) == _num); \
			zend_mm_free_small(AG(mm_heap), ptr, _num); \
		} \
	}
// 正式用
#else
// ing4, 定义一个释放 特定尺寸的 small 块的方法
# define _ZEND_BIN_FREE(_num, _size, _elements, _pages, x, y) \
	ZEND_API void ZEND_FASTCALL _efree_ ## _size(void *ptr) { \
		ZEND_MM_CUSTOM_DEALLOCATOR(ptr); \
		{ \
			zend_mm_chunk *chunk = (zend_mm_chunk*)ZEND_MM_ALIGNED_BASE(ptr, ZEND_MM_CHUNK_SIZE); \
			ZEND_MM_CHECK(chunk->heap == AG(mm_heap), "zend_mm_heap corrupted"); \
			zend_mm_free_small(AG(mm_heap), ptr, _num); \
		} \
	}
#endif

ZEND_MM_BINS_INFO(_ZEND_BIN_FREE, x, y)

// ing4, 释放large块
ZEND_API void ZEND_FASTCALL _efree_large(void *ptr, size_t size)
{
	// 调用客户方法，释放内存
	ZEND_MM_CUSTOM_DEALLOCATOR(ptr);
	{
		// 计算到 ZEND_MM_CHUNK_SIZE 的偏移量
		size_t page_offset = ZEND_MM_ALIGNED_OFFSET(ptr, ZEND_MM_CHUNK_SIZE);
		// 取回所在chunk的指针
		zend_mm_chunk *chunk = (zend_mm_chunk*)ZEND_MM_ALIGNED_BASE(ptr, ZEND_MM_CHUNK_SIZE);
		// 计算当前 page 页码
		int page_num = page_offset / ZEND_MM_PAGE_SIZE;
		// 向后对齐 alignment
		uint32_t pages_count = ZEND_MM_ALIGNED_SIZE_EX(size, ZEND_MM_PAGE_SIZE) / ZEND_MM_PAGE_SIZE;
		// chunk必须从属于 AG(mm_heap)， 
		// 并且 page_offset 到 ZEND_MM_PAGE_SIZE 的偏移量必须是0 （ptr指向page开头）
		ZEND_MM_CHECK(chunk->heap == AG(mm_heap) && ZEND_MM_ALIGNED_OFFSET(page_offset, ZEND_MM_PAGE_SIZE) == 0, "zend_mm_heap corrupted");
		// 地图信息里必须有large标记
		ZEND_ASSERT(chunk->map[page_num] & ZEND_MM_IS_LRUN);
		// 要释放的尺寸必须和map信息里的尺寸一致（不传size不行么？）
		ZEND_ASSERT(ZEND_MM_LRUN_PAGES(chunk->map[page_num]) == pages_count);
		// 释放large 块
		zend_mm_free_large(AG(mm_heap), chunk, page_num, pages_count);
	}
}

// ing4, 释放huge块
ZEND_API void ZEND_FASTCALL _efree_huge(void *ptr, size_t size)
{
	// 客户自定义方法
	ZEND_MM_CUSTOM_DEALLOCATOR(ptr);
	// 释放huge块
	zend_mm_free_huge(AG(mm_heap), ptr);
}
#endif

// ing4, 分配内存
ZEND_API void* ZEND_FASTCALL _emalloc(size_t size ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC)
{
#if ZEND_MM_CUSTOM
	if (UNEXPECTED(AG(mm_heap)->use_custom_heap)) {
		return _malloc_custom(size ZEND_FILE_LINE_RELAY_CC ZEND_FILE_LINE_ORIG_RELAY_CC);
	}
#endif
	return zend_mm_alloc_heap(AG(mm_heap), size ZEND_FILE_LINE_RELAY_CC ZEND_FILE_LINE_ORIG_RELAY_CC);
}

// ing4, 释放一块内存
ZEND_API void ZEND_FASTCALL _efree(void *ptr ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC)
{
#if ZEND_MM_CUSTOM
	if (UNEXPECTED(AG(mm_heap)->use_custom_heap)) {
		_efree_custom(ptr ZEND_FILE_LINE_RELAY_CC ZEND_FILE_LINE_ORIG_RELAY_CC);
		return;
	}
#endif
	zend_mm_free_heap(AG(mm_heap), ptr ZEND_FILE_LINE_RELAY_CC ZEND_FILE_LINE_ORIG_RELAY_CC);
}

// ing4, 调整内存大小
ZEND_API void* ZEND_FASTCALL _erealloc(void *ptr, size_t size ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC)
{
#if ZEND_MM_CUSTOM
	if (UNEXPECTED(AG(mm_heap)->use_custom_heap)) {
		return _realloc_custom(ptr, size ZEND_FILE_LINE_RELAY_CC ZEND_FILE_LINE_ORIG_RELAY_CC);
	}
#endif
	// 调整内存大小, 不使用copysize 
	return zend_mm_realloc_heap(AG(mm_heap), ptr, size, 0, size ZEND_FILE_LINE_RELAY_CC ZEND_FILE_LINE_ORIG_RELAY_CC);
}

// ing4, 调整内存大小
ZEND_API void* ZEND_FASTCALL _erealloc2(void *ptr, size_t size, size_t copy_size ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC)
{
// 1
#if ZEND_MM_CUSTOM
	// 客户自定义方法
	if (UNEXPECTED(AG(mm_heap)->use_custom_heap)) {
		return _realloc_custom(ptr, size ZEND_FILE_LINE_RELAY_CC ZEND_FILE_LINE_ORIG_RELAY_CC);
	}
#endif
	// 重新分配内存, 使用copysize 
	return zend_mm_realloc_heap(AG(mm_heap), ptr, size, 1, copy_size ZEND_FILE_LINE_RELAY_CC ZEND_FILE_LINE_ORIG_RELAY_CC);
}

// ing3, 返回内存块的大小
ZEND_API size_t ZEND_FASTCALL _zend_mem_block_size(void *ptr ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC)
{
#if ZEND_MM_CUSTOM
	if (UNEXPECTED(AG(mm_heap)->use_custom_heap)) {
		return 0;
	}
#endif
	return zend_mm_size(AG(mm_heap), ptr ZEND_FILE_LINE_RELAY_CC ZEND_FILE_LINE_ORIG_RELAY_CC);
}

// ing4, 先检查分配大小会不会溢出，再分配非持久内存。分配大小 ： nmemb*size+offset
ZEND_API void* ZEND_FASTCALL _safe_emalloc(size_t nmemb, size_t size, size_t offset ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC)
{
	return _emalloc(zend_safe_address_guarded(nmemb, size, offset) ZEND_FILE_LINE_RELAY_CC ZEND_FILE_LINE_ORIG_RELAY_CC);
}

// ing4, 先检查分配大小会不会溢出，再分配持久内存。分配大小 ： nmemb*size+offset
ZEND_API void* ZEND_FASTCALL _safe_malloc(size_t nmemb, size_t size, size_t offset)
{
	return pemalloc(zend_safe_address_guarded(nmemb, size, offset), 1);
}

// ing4, 先检查分配大小会不会溢出，再调整非持久内存大小。分配大小 ： nmemb*size+offset
ZEND_API void* ZEND_FASTCALL _safe_erealloc(void *ptr, size_t nmemb, size_t size, size_t offset ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC)
{
	return _erealloc(ptr, zend_safe_address_guarded(nmemb, size, offset) ZEND_FILE_LINE_RELAY_CC ZEND_FILE_LINE_ORIG_RELAY_CC);
}

// ing4, 先检查分配大小会不会溢出，再调整持久内存大小。分配大小 ： nmemb*size+offset
ZEND_API void* ZEND_FASTCALL _safe_realloc(void *ptr, size_t nmemb, size_t size, size_t offset)
{
	return perealloc(ptr, zend_safe_address_guarded(nmemb, size, offset), 1);
}

// ing4, 先检查分配大小会不会溢出（比前面少了个offset），再分配非持久内存。分配大小 ： nmemb*size
ZEND_API void* ZEND_FASTCALL _ecalloc(size_t nmemb, size_t size ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC)
{
	void *p;

	size = zend_safe_address_guarded(nmemb, size, 0);
	p = _emalloc(size ZEND_FILE_LINE_RELAY_CC ZEND_FILE_LINE_ORIG_RELAY_CC);
	// 缓冲区全写成0
	memset(p, 0, size);
	return p;
}

// ing4, (e-string-dump)调用zend方法分配内存，复制完整字串
ZEND_API char* ZEND_FASTCALL _estrdup(const char *s ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC)
{
	size_t length;
	char *p;

	length = strlen(s);
	// 如果长度+1 出错 ，说明整数溢出
	if (UNEXPECTED(length + 1 == 0)) {
		zend_error_noreturn(E_ERROR, "Possible integer overflow in memory allocation (1 * %zu + 1)", length);
	}
	// 分配内存
	p = (char *) _emalloc(length + 1 ZEND_FILE_LINE_RELAY_CC ZEND_FILE_LINE_ORIG_RELAY_CC);
	// 复制内容
	memcpy(p, s, length+1);
	return p;
}

// ing4, (e-string-ndump)调用zend方法分配内存，复制字串，自定义长度
ZEND_API char* ZEND_FASTCALL _estrndup(const char *s, size_t length ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC)
{
	char *p;
	// 如果长度+1 出错 ，说明整数溢出
	if (UNEXPECTED(length + 1 == 0)) {
		zend_error_noreturn(E_ERROR, "Possible integer overflow in memory allocation (1 * %zu + 1)", length);
	}
	p = (char *) _emalloc(length + 1 ZEND_FILE_LINE_RELAY_CC ZEND_FILE_LINE_ORIG_RELAY_CC);
	memcpy(p, s, length);
	// 0 就是 \0
	p[length] = 0;
	return p;
}

static ZEND_COLD ZEND_NORETURN void zend_out_of_memory(void);

// ing4, 创建内存，把内容copy到新内存里
ZEND_API char* ZEND_FASTCALL zend_strndup(const char *s, size_t length)
{
	char *p;
	// 如果 length 是 -1
	if (UNEXPECTED(length + 1 == 0)) {
		// 报错，可能已经溢出
		zend_error_noreturn(E_ERROR, "Possible integer overflow in memory allocation (1 * %zu + 1)", length);
	}
	// 用原生方法分配要求的长度，最后多1位
	p = (char *) malloc(length + 1);
	// 如果分配失败
	if (UNEXPECTED(p == NULL)) {
		zend_out_of_memory();
	}
	// 把原来的内容copy 过来
	if (EXPECTED(length)) {
		memcpy(p, s, length);
	}
	// 最后一位是0
	p[length] = 0;
	return p;
}

// ing4, 设置内存限制 
ZEND_API zend_result zend_set_memory_limit(size_t memory_limit)
{
// 如果开启了内存限制功能
#if ZEND_MM_LIMIT
	zend_mm_heap *heap = AG(mm_heap);

	// 如果限制数少于当下使用量
	if (UNEXPECTED(memory_limit < heap->real_size)) {
		// 如果删除缓存 chunk 后内存不超限制 
		if (memory_limit >= heap->real_size - heap->cached_chunks_count * ZEND_MM_CHUNK_SIZE) {
			// 释放一些缓存 chunk 来适应新的内存限制 
			/* free some cached chunks to fit into new memory limit */
			do {
				zend_mm_chunk *p = heap->cached_chunks;
				heap->cached_chunks = p->next;
				// 释放chunk
				zend_mm_chunk_free(heap, p, ZEND_MM_CHUNK_SIZE);
				heap->cached_chunks_count--;
				heap->real_size -= ZEND_MM_CHUNK_SIZE;
				// 逐个释放缓存chunk直到满足要求
			} while (memory_limit < heap->real_size);
			return SUCCESS;
		}
		// 无法满足要求，报错
		return FAILURE;
	}
	// 应用限制 
	AG(mm_heap)->limit = memory_limit;
#endif
	return SUCCESS;
}

// ing3, 查看是否分配是否存在溢出
ZEND_API bool zend_alloc_in_memory_limit_error_reporting(void)
{
// 如果开启了内存限制功能
#if ZEND_MM_LIMIT
	return AG(mm_heap)->overflow;
// 未开启内存限制功能
#else
	return false;
#endif
}

// ing3, 返回内存使用情况
ZEND_API size_t zend_memory_usage(bool real_usage)
{
// 如果记录的内存状态
#if ZEND_MM_STAT
	// 返回 AG(mm_heap) 中的 real_size 或 size ，两个有什么区别？
	if (real_usage) {
		return AG(mm_heap)->real_size;
	} else {
		size_t usage = AG(mm_heap)->size;
		return usage;
	}
#endif
	return 0;
}

// ing3, 返回内存使用峰值
ZEND_API size_t zend_memory_peak_usage(bool real_usage)
{
#if ZEND_MM_STAT
	// 返回 AG(mm_heap) 中的 real_peak 或 peak ，两个有什么区别？
	if (real_usage) {
		return AG(mm_heap)->real_peak;
	} else {
		return AG(mm_heap)->peak;
	}
#endif
	return 0;
}

// clear, 重置内存峰值，设置成当前用量
ZEND_API void zend_memory_reset_peak_usage(void)
{
#if ZEND_MM_STAT
	AG(mm_heap)->real_peak = AG(mm_heap)->real_size;
	AG(mm_heap)->peak = AG(mm_heap)->size;
#endif
}

// ing4, 关闭内存管理器, 为什么会好几个地方调用呢？
ZEND_API void shutdown_memory_manager(bool silent, bool full_shutdown)
{
	zend_mm_shutdown(AG(mm_heap), full_shutdown, silent);
}

// clear, 报错，内存溢出
static ZEND_COLD ZEND_NORETURN void zend_out_of_memory(void)
{
	fprintf(stderr, "Out of memory\n");
	exit(1);
}

// 1
#if ZEND_MM_CUSTOM
// suspend, 这几个东东是给custom_heap 用的
static zend_always_inline void tracked_add(zend_mm_heap *heap, void *ptr, size_t size) {
	zval size_zv;
	zend_ulong h = ((uintptr_t) ptr) >> ZEND_MM_ALIGNMENT_LOG2;
	ZEND_ASSERT((void *) (uintptr_t) (h << ZEND_MM_ALIGNMENT_LOG2) == ptr);
	ZVAL_LONG(&size_zv, size);
	zend_hash_index_add_new(heap->tracked_allocs, h, &size_zv);
}
// suspend, 这几个东东是给custom_heap 用的
static zend_always_inline zval *tracked_get_size_zv(zend_mm_heap *heap, void *ptr) {
	zend_ulong h = ((uintptr_t) ptr) >> ZEND_MM_ALIGNMENT_LOG2;
	zval *size_zv = zend_hash_index_find(heap->tracked_allocs, h);
	ZEND_ASSERT(size_zv && "Trying to free pointer not allocated through ZendMM");
	return size_zv;
}

// ing4, 检验内存是否够用
static zend_always_inline void tracked_check_limit(zend_mm_heap *heap, size_t add_size) {
	// 如果内存不足并且没有溢出
	if (add_size > heap->limit - heap->size && !heap->overflow) {
#if ZEND_DEBUG
		zend_mm_safe_error(heap,
			"Allowed memory size of %zu bytes exhausted at %s:%d (tried to allocate %zu bytes)",
			heap->limit, "file", 0, add_size);
#else
		// 报错，内存溢出
		zend_mm_safe_error(heap,
			"Allowed memory size of %zu bytes exhausted (tried to allocate %zu bytes)",
			heap->limit, add_size);
#endif
	}
}

// suspend, 这几个东东是给custom_heap 用的
static void *tracked_malloc(size_t size)
{
	zend_mm_heap *heap = AG(mm_heap);
	tracked_check_limit(heap, size);

	void *ptr = malloc(size);
	if (!ptr) {
		zend_out_of_memory();
	}

	tracked_add(heap, ptr, size);
	heap->size += size;
	return ptr;
}

// suspend, 这几个东东是给custom_heap 用的
static void tracked_free(void *ptr) {
	// 指针无效，返回
	if (!ptr) {
		return;
	}

	zend_mm_heap *heap = AG(mm_heap);
	// 
	zval *size_zv = tracked_get_size_zv(heap, ptr);
	heap->size -= Z_LVAL_P(size_zv);
	zend_hash_del_bucket(heap->tracked_allocs, (Bucket *) size_zv);
	free(ptr);
}

// suspend, 这几个东东是给custom_heap 用的
static void *tracked_realloc(void *ptr, size_t new_size) {
	zend_mm_heap *heap = AG(mm_heap);
	zval *old_size_zv = NULL;
	size_t old_size = 0;
	if (ptr) {
		old_size_zv = tracked_get_size_zv(heap, ptr);
		old_size = Z_LVAL_P(old_size_zv);
	}

	if (new_size > old_size) {
		tracked_check_limit(heap, new_size - old_size);
	}

	/* Delete information about old allocation only after checking the memory limit. */
	if (old_size_zv) {
		zend_hash_del_bucket(heap->tracked_allocs, (Bucket *) old_size_zv);
	}

	ptr = __zend_realloc(ptr, new_size);
	tracked_add(heap, ptr, new_size);
	heap->size += new_size - old_size;
	return ptr;
}

// suspend, 这几个东东是给custom_heap 用的
static void tracked_free_all(void) {
	HashTable *tracked_allocs = AG(mm_heap)->tracked_allocs;
	zend_ulong h;
	ZEND_HASH_FOREACH_NUM_KEY(tracked_allocs, h) {
		void *ptr = (void *) (uintptr_t) (h << ZEND_MM_ALIGNMENT_LOG2);
		free(ptr);
	} ZEND_HASH_FOREACH_END();
}
#endif

// ing3, 全局只有一个heap， alloc_globals就是外部定义的 alloc_globals
static void alloc_globals_ctor(zend_alloc_globals *alloc_globals)
{
	char *tmp;

#if ZEND_MM_CUSTOM
	// getenv 是系统方法吗？
	tmp = getenv("USE_ZEND_ALLOC");
	// 如果有 USE_ZEND_ALLOC， 并且是关闭状态, 默认应该是没有 USE_ZEND_ALLOC
	// 这个情况是不使用zend分配
	if (tmp && !ZEND_ATOL(tmp)) {
		// #  define ZEND_ATOL(s) _atoi64((s))
		bool tracked = (tmp = getenv("USE_TRACKED_ALLOC")) && ZEND_ATOL(tmp);
		// 使用系统自带方法 malloc 创建  zend_mm_heap 
		zend_mm_heap *mm_heap = alloc_globals->mm_heap = malloc(sizeof(zend_mm_heap));
		// 重置指针位置 ？
		memset(mm_heap, 0, sizeof(zend_mm_heap));
		// 1
		mm_heap->use_custom_heap = ZEND_MM_CUSTOM_HEAP_STD;
		// 64个1 >> 1
		mm_heap->limit = ((size_t)Z_L(-1) >> (size_t)Z_L(1));
		// 未溢出
		mm_heap->overflow = 0;

		// 如果不追踪
		if (!tracked) {
			// 使用系统分配器
			/* Use system allocator. */
			mm_heap->custom_heap.std._malloc = __zend_malloc;
			mm_heap->custom_heap.std._free = free;
			mm_heap->custom_heap.std._realloc = __zend_realloc;
		// 如果要求追踪
		} else {
			// 使用系统分配器并 追踪分配过程，准备自动释放
			/* Use system allocator and track allocations for auto-free. */
			mm_heap->custom_heap.std._malloc = tracked_malloc;
			mm_heap->custom_heap.std._free = tracked_free;
			mm_heap->custom_heap.std._realloc = tracked_realloc;
			// 追踪器哈希表
			mm_heap->tracked_allocs = malloc(sizeof(HashTable));
			zend_hash_init(mm_heap->tracked_allocs, 1024, NULL, NULL, 1);
		}
		// 
		return;
	}
#endif
	// 如果开启了环境变量 USE_ZEND_ALLOC_HUGE_PAGES
	tmp = getenv("USE_ZEND_ALLOC_HUGE_PAGES");
	/// 如果定义了这个环境变量
	if (tmp && ZEND_ATOL(tmp)) {
		// 开启巨大的page 
		zend_mm_use_huge_pages = true;
	}
	// 初始化内存，走这里（测试过）
	alloc_globals->mm_heap = zend_mm_init();
}

// ing4, 如果要求线程安全,全局销毁器
#ifdef ZTS
static void alloc_globals_dtor(zend_alloc_globals *alloc_globals)
{
	zend_mm_shutdown(alloc_globals->mm_heap, 1, 1);
}
#endif

// ing2, 开始内存管理 /Zend/zend.c 调用, 未了解线程安全
ZEND_API void start_memory_manager(void)
{
// 如果要求线程安全
#ifdef ZTS
	// 这种情况下自己创建 zend_alloc_globals 用的不是外面的 alloc_globals
	ts_allocate_fast_id(&alloc_globals_id, &alloc_globals_offset, sizeof(zend_alloc_globals), (ts_allocate_ctor) alloc_globals_ctor, (ts_allocate_dtor) alloc_globals_dtor);
// 非线程安全
#else
	alloc_globals_ctor(&alloc_globals);
#endif

// 如果不是windows系统 ，获取系统设置中的 PAGE_SIZE
#ifndef _WIN32
#  if defined(_SC_PAGESIZE)
	REAL_PAGE_SIZE = sysconf(_SC_PAGESIZE);
#  elif defined(_SC_PAGE_SIZE)
	REAL_PAGE_SIZE = sysconf(_SC_PAGE_SIZE);
#  endif
#endif

}

// suspend, 调试用
ZEND_API zend_mm_heap *zend_mm_set_heap(zend_mm_heap *new_heap)
{
	zend_mm_heap *old_heap;

	old_heap = AG(mm_heap);
	AG(mm_heap) = (zend_mm_heap*)new_heap;
	return (zend_mm_heap*)old_heap;
}

// clear, 返回内存管理heap
ZEND_API zend_mm_heap *zend_mm_get_heap(void)
{
	return AG(mm_heap);
}

// clear, 是否是客户自定义heap
ZEND_API bool zend_mm_is_custom_heap(zend_mm_heap *new_heap)
{
#if ZEND_MM_CUSTOM
	return AG(mm_heap)->use_custom_heap;
#else
	return 0;
#endif
}

// clear, 设置客户自定义内存管理方法
ZEND_API void zend_mm_set_custom_handlers(zend_mm_heap *heap,
                                          void* (*_malloc)(size_t),
                                          void  (*_free)(void*),
                                          void* (*_realloc)(void*, size_t))
{
#if ZEND_MM_CUSTOM
	zend_mm_heap *_heap = (zend_mm_heap*)heap;

	// 如果3个方法都不存在
	if (!_malloc && !_free && !_realloc) {
		_heap->use_custom_heap = ZEND_MM_CUSTOM_HEAP_NONE;
	// 
	} else {
		_heap->use_custom_heap = ZEND_MM_CUSTOM_HEAP_STD;
		_heap->custom_heap.std._malloc = _malloc;
		_heap->custom_heap.std._free = _free;
		_heap->custom_heap.std._realloc = _realloc;
	}
#endif
}

// clear, 取回用户自定义的内存分配方法
ZEND_API void zend_mm_get_custom_handlers(zend_mm_heap *heap,
                                          void* (**_malloc)(size_t),
                                          void  (**_free)(void*),
                                          void* (**_realloc)(void*, size_t))
{
// 1
#if ZEND_MM_CUSTOM
	zend_mm_heap *_heap = (zend_mm_heap*)heap;
	// 如果使用客户自定义内存分配，返回分配方法
	if (heap->use_custom_heap) {
		*_malloc = _heap->custom_heap.std._malloc;
		*_free = _heap->custom_heap.std._free;
		*_realloc = _heap->custom_heap.std._realloc;
	} else {
		*_malloc = NULL;
		*_free = NULL;
		*_realloc = NULL;
	}
#else
	*_malloc = NULL;
	*_free = NULL;
	*_realloc = NULL;
#endif
}

#if ZEND_DEBUG
//  clear, 设置客户自定义调试用的内存管理方法，分配，释放，重新分配，3个方法
ZEND_API void zend_mm_set_custom_debug_handlers(zend_mm_heap *heap,
                                                void* (*_malloc)(size_t ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC),
                                                void  (*_free)(void* ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC),
                                                void* (*_realloc)(void*, size_t ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC))
{
// 1
#if ZEND_MM_CUSTOM
	zend_mm_heap *_heap = (zend_mm_heap*)heap;

	_heap->use_custom_heap = ZEND_MM_CUSTOM_HEAP_DEBUG;
	_heap->custom_heap.debug._malloc = _malloc;
	_heap->custom_heap.debug._free = _free;
	_heap->custom_heap.debug._realloc = _realloc;
#endif
}
#endif

// clear, 获取 内存storage
ZEND_API zend_mm_storage *zend_mm_get_storage(zend_mm_heap *heap)
{
#if ZEND_MM_STORAGE
	return heap->storage;
#else
	return NULL
#endif
}

// ing4, 这个方法只有 \sapi\phpdbg\phpdbg_sigsafe.c 调试用到
ZEND_API zend_mm_heap *zend_mm_startup(void)
{
	return zend_mm_init();
}

// suspend, 这个方法是调试用的，正常情况不会用到
ZEND_API zend_mm_heap *zend_mm_startup_ex(const zend_mm_handlers *handlers, void *data, size_t data_size)
{
#if ZEND_MM_STORAGE
	zend_mm_storage tmp_storage, *storage;
	zend_mm_chunk *chunk;
	zend_mm_heap *heap;

	memcpy((zend_mm_handlers*)&tmp_storage.handlers, handlers, sizeof(zend_mm_handlers));
	tmp_storage.data = data;
	chunk = (zend_mm_chunk*)handlers->chunk_alloc(&tmp_storage, ZEND_MM_CHUNK_SIZE, ZEND_MM_CHUNK_SIZE);
	if (UNEXPECTED(chunk == NULL)) {
#if ZEND_MM_ERROR
		fprintf(stderr, "Can't initialize heap\n");
#endif
		return NULL;
	}
	heap = &chunk->heap_slot;
	chunk->heap = heap;
	chunk->next = chunk;
	chunk->prev = chunk;
	chunk->free_pages = ZEND_MM_PAGES - ZEND_MM_FIRST_PAGE;
	chunk->free_tail = ZEND_MM_FIRST_PAGE;
	chunk->num = 0;
	chunk->free_map[0] = (Z_L(1) << ZEND_MM_FIRST_PAGE) - 1;
	chunk->map[0] = ZEND_MM_LRUN(ZEND_MM_FIRST_PAGE);
	heap->main_chunk = chunk;
	heap->cached_chunks = NULL;
	heap->chunks_count = 1;
	heap->peak_chunks_count = 1;
	heap->cached_chunks_count = 0;
	heap->avg_chunks_count = 1.0;
	heap->last_chunks_delete_boundary = 0;
	heap->last_chunks_delete_count = 0;
#if ZEND_MM_STAT || ZEND_MM_LIMIT
	heap->real_size = ZEND_MM_CHUNK_SIZE;
#endif
#if ZEND_MM_STAT
	heap->real_peak = ZEND_MM_CHUNK_SIZE;
	heap->size = 0;
	heap->peak = 0;
#endif
#if ZEND_MM_LIMIT
	heap->limit = (Z_L(-1) >> Z_L(1));
	heap->overflow = 0;
#endif
#if ZEND_MM_CUSTOM
	heap->use_custom_heap = 0;
#endif
	heap->storage = &tmp_storage;
	heap->huge_list = NULL;
	memset(heap->free_slot, 0, sizeof(heap->free_slot));
	storage = _zend_mm_alloc(heap, sizeof(zend_mm_storage) + data_size ZEND_FILE_LINE_CC ZEND_FILE_LINE_CC);
	if (!storage) {
		handlers->chunk_free(&tmp_storage, chunk, ZEND_MM_CHUNK_SIZE);
#if ZEND_MM_ERROR
		fprintf(stderr, "Can't initialize heap\n");
#endif
		return NULL;
	}
	memcpy(storage, &tmp_storage, sizeof(zend_mm_storage));
	if (data) {
		storage->data = (void*)(((char*)storage + sizeof(zend_mm_storage)));
		memcpy(storage->data, data, data_size);
	}
	heap->storage = storage;
	return heap;
#else
	return NULL;
#endif
}

// ing2, 调用原生方法分配内存 memory allocate
ZEND_API void * __zend_malloc(size_t len)
{
	void *tmp = malloc(len);
	// 如果内存溢出，len会变空 ??
	if (EXPECTED(tmp || !len)) {
		return tmp;
	}
	zend_out_of_memory();
}

// ing3，分配内存并全设置成0
ZEND_API void * __zend_calloc(size_t nmemb, size_t len)
{
	void *tmp;
	// 返回一个检查过溢出的安全数字
	len = zend_safe_address_guarded(nmemb, len, 0);
	// 调用系统函数分配内存
	tmp = __zend_malloc(len);
	// 缓冲区全设置成
	memset(tmp, 0, len);
	return tmp;
}

// ing4, 重新分配内存 reallocate
ZEND_API void * __zend_realloc(void *p, size_t len)
{
	// 调整
	p = realloc(p, len);
	// 如果分配成功，或len无效（len无效时可以不成功）
	if (EXPECTED(p || !len)) {
		// 返回
		return p;
	}
	zend_out_of_memory();
}

// ing4, 复制内存
ZEND_API char * __zend_strdup(const char *s)
{
	// 调用原生的 strdup 函数 
	char *tmp = strdup(s);
	if (EXPECTED(tmp)) {
		return tmp;
	}
	zend_out_of_memory();
}

// 如果要求线程安全
#ifdef ZTS
// ing2, 返回 zend_alloc_globals 的大小
size_t zend_mm_globals_size(void)
{
	// zend_alloc_globals 里面只有一个heap指针，返回的应该就是一个指针的大小
	return sizeof(zend_alloc_globals);
}
#endif
