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

// 
#ifndef ZEND_ALLOC_SIZES_H
#define ZEND_ALLOC_SIZES_H

// CHUNK 2M
#define ZEND_MM_CHUNK_SIZE ((size_t) (2 * 1024 * 1024))    /* 2 MB  */
// page 4K
#define ZEND_MM_PAGE_SIZE  (4 * 1024)                      /* 4 KB  */
// 一个 CHUNK 可分成512个 page
#define ZEND_MM_PAGES      (ZEND_MM_CHUNK_SIZE / ZEND_MM_PAGE_SIZE)  /* 512 */
// 第一个page的编号 1
#define ZEND_MM_FIRST_PAGE (1)

// 小尺寸最小 8 byte
#define ZEND_MM_MIN_SMALL_SIZE		8
// 大尺寸最小 3272 byte
#define ZEND_MM_MAX_SMALL_SIZE      3072
// 大尺寸最小 2m - 4k
#define ZEND_MM_MAX_LARGE_SIZE      (ZEND_MM_CHUNK_SIZE - (ZEND_MM_PAGE_SIZE * ZEND_MM_FIRST_PAGE))

// 第一列，序号
// 第二列，分配内存的大小，向上取最接近的数单位 Byte
/* num, size, count, pages */
// size 是大小，count 是每次开辟数量， pages 是每次占用 pages 数量, 这样计划最节省内存
/*
大小	数量	大小	pages数量	实际占用page数量
8       512     4096    1       1
16      256     4096    1       1
24      170     4080    0.99609375      1
32      128     4096    1       1
40      102     4080    0.99609375      1
48      85      4080    0.99609375      1
56      73      4088    0.998046875     1
64      64      4096    1       1
80      51      4080    0.99609375      1
96      42      4032    0.984375        1
112     36      4032    0.984375        1
128     32      4096    1       1
160     25      4000    0.9765625       1
192     21      4032    0.984375        1
224     18      4032    0.984375        1
256     16      4096    1       1
320     64      20480   5       5
384     32      12288   3       3
448     9       4032    0.984375        1
512     8       4096    1       1
640     32      20480   5       5
768     16      12288   3       3
896     9       8064    1.96875 2       // 这个是最不经济的，使用率也有 98.44 %
1024    8       8192    2       2
1280    16      20480   5       5
1536    8       12288   3       3
1792    16      28672   7       7
2048    8       16384   4       4
2560    8       20480   5       5
3072    4       12288   3       3
*/
// 比如 80 大小的空间每次开辟 51个 ，占用 1 个page
// 比如 896 大小的空间每次开辟 9 ，占用 2 个page
#define ZEND_MM_BINS_INFO(_, x, y) \
	_( 0,    8,  512, 1, x, y) \
	_( 1,   16,  256, 1, x, y) \
	_( 2,   24,  170, 1, x, y) \
	_( 3,   32,  128, 1, x, y) \
	_( 4,   40,  102, 1, x, y) \
	_( 5,   48,   85, 1, x, y) \
	_( 6,   56,   73, 1, x, y) \
	_( 7,   64,   64, 1, x, y) \
	_( 8,   80,   51, 1, x, y) \
	_( 9,   96,   42, 1, x, y) \
	_(10,  112,   36, 1, x, y) \
	_(11,  128,   32, 1, x, y) \
	_(12,  160,   25, 1, x, y) \
	_(13,  192,   21, 1, x, y) \
	_(14,  224,   18, 1, x, y) \
	_(15,  256,   16, 1, x, y) \
	_(16,  320,   64, 5, x, y) \
	_(17,  384,   32, 3, x, y) \
	_(18,  448,    9, 1, x, y) \
	_(19,  512,    8, 1, x, y) \
	_(20,  640,   32, 5, x, y) \
	_(21,  768,   16, 3, x, y) \
	_(22,  896,    9, 2, x, y) \
	_(23, 1024,    8, 2, x, y) \
	_(24, 1280,   16, 5, x, y) \
	_(25, 1536,    8, 3, x, y) \
	_(26, 1792,   16, 7, x, y) \
	_(27, 2048,    8, 4, x, y) \
	_(28, 2560,    8, 5, x, y) \
	_(29, 3072,    4, 3, x, y)

#endif /* ZEND_ALLOC_SIZES_H */
