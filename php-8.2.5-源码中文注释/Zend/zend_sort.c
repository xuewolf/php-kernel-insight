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
   | Authors: Xinchen Hui <laruence@php.net>                              |
   |          Sterling Hughes <sterling@php.net>                          |
   +----------------------------------------------------------------------+
*/

#include "zend.h"
#include "zend_sort.h"
#include <limits.h>

// ing4, 排序2个元素
static inline void zend_sort_2(void *a, void *b, compare_func_t cmp, swap_func_t swp) /* {{{ */ {
	// 如果第一个大于第二个
	if (cmp(a, b) > 0) {
		// 交换位置
		swp(a, b);
	}
}
/* }}} */

// ing4, 排序3个元素
static inline void zend_sort_3(void *a, void *b, void *c, compare_func_t cmp, swap_func_t swp) /* {{{ */ {
	// 第一个不大于第二个，前两个不用动
	if (!(cmp(a, b) > 0)) {
		// 第二个不大于第三个，后两个不用动，直接返回
		if (!(cmp(b, c) > 0)) {
			return;
		}
		// 第二个大于第三个，后两个交换
		swp(b, c);
		// 交换后第一个大于第二个
		if (cmp(a, b) > 0) {
			// 前两个交换
			swp(a, b);
		}
		// 完毕
		return;
	}
	
	// 第一个大于第二个，且 第二个大于等于第三个（一个比一个大）
	if (!(cmp(c, b) > 0)) {
		// 交 1，3 二个元素
		swp(a, c);
		return;
	}
	
	// 第一个大于第二个，且第二个小于第三个（后面两个不用换）
	// 先交换前两个
	swp(a, b);
	// 如果交换后第二个大于第三个
	if (cmp(b, c) > 0) {
		// 交换后两个
		swp(b, c);
	}
}
/* }}} */

// ing4, 4个元素
static void zend_sort_4(void *a, void *b, void *c, void *d, compare_func_t cmp, swap_func_t swp) /* {{{ */ {
	// 先排前3个
	zend_sort_3(a, b, c, cmp, swp);
	// 如果第三个大于第4个, 让第四个冒泡上去
	if (cmp(c, d) > 0) {
		swp(c, d);
		if (cmp(b, c) > 0) {
			swp(b, c);
			if (cmp(a, b) > 0) {
				swp(a, b);
			}
		}
	}
	// 否则什么也不用做
}
/* }}} */

// ing4, 5个元素
static void zend_sort_5(void *a, void *b, void *c, void *d, void *e, compare_func_t cmp, swap_func_t swp) /* {{{ */ {
	// 先排序前4个
	zend_sort_4(a, b, c, d, cmp, swp);
	// 如果第五个大于第四个，让第五个冒泡上去
	if (cmp(d, e) > 0) {
		swp(d, e);
		if (cmp(c, d) > 0) {
			swp(c, d);
			if (cmp(b, c) > 0) {
				swp(b, c);
				if (cmp(a, b) > 0) {
					swp(a, b);
				}
			}
		}
	}
}
/* }}} */


// ing4, 插入排序
ZEND_API void zend_insert_sort(void *base, size_t nmemb, size_t siz, compare_func_t cmp, swap_func_t swp) /* {{{ */{
	// 根据元素数量进行处理
	switch (nmemb) {
		//0 个，1个，不用排
		case 0:
		case 1:
			break;
		// 2个
		case 2:
			zend_sort_2(base, (char *)base + siz, cmp, swp);
			break;
		// 3个
		case 3:
			zend_sort_3(base, (char *)base + siz, (char *)base + siz + siz, cmp, swp);
			break;
		// 4个
		case 4:
			{
				size_t siz2 = siz + siz;
				zend_sort_4(base, (char *)base + siz, (char *)base + siz2, (char *)base + siz + siz2, cmp, swp);
			}
			break;
		// 5个
		case 5:
			{
				size_t siz2 = siz + siz;
				zend_sort_5(base, (char *)base + siz, (char *)base + siz2, (char *)base + siz + siz2, (char *)base + siz2 + siz2, cmp, swp);
			}
			break;
		// 5个以上
		default:
			{
				char *i, *j, *k;
				// 开始位置
				char *start = (char *)base;
				// 结束位置
				char *end = start + (nmemb * siz);
				// siz的2倍
				size_t siz2= siz + siz;
				// 哨兵 最少6个，哨兵是第6个位置
				char *sentry = start + (6 * siz);
				// 从第二个，到哨兵位置（前面这一段每次跳一个来比较）
				for (i = start + siz; i < sentry; i += siz) {
					// 前一个位置
					j = i - siz;
					// 如果前面的不大于后面的
					if (!(cmp(j, i) > 0)) {
						// 下一个
						continue;
					}
					// 如果前面的大于后面的，要找位置，让 i 冒泡上来
					// 从 j 开始往前找，一直找到开头
					while (j != start) {
						// 往前一个
						j -= siz;
						// 如果任何前面的元素 不大于 i元素
						if (!(cmp(j, i) > 0)) {
							// 找到了，j 后移，这就是插入位置
							j += siz;
							break;
						}
					}
					// 从i 到 j  
					for (k = i; k > j; k -= siz) {
						// 两两交换，让 i 冒泡上来
						swp(k, k - siz);
					}
				}
				
				// 从哨兵位置，到最后（哨兵后面这一段每次跳2个来比较）
				for (i = sentry; i < end; i += siz) {
					// 前面一个元素
					j = i - siz;
					// 如果前面的不大于后面的
					if (!(cmp(j, i) > 0)) {
						// 下一个 
						continue;
					}
					
					// 如果前面的大于后面的
					do {
						// 向前跳两个
						j -= siz2;
						// 前面的 不大于 i（ 这里是插入位置，要比较2个元素）
						if (!(cmp(j, i) > 0)) {
							// 后移一个
							j += siz;
							// 后一个还不大于 i
							if (!(cmp(j, i) > 0)) {
								// 再后移一个（因为前面跳了2个）
								j += siz;
							}
							// 跳出
							break;
						}
						// 如果在中间没找到位置						
						// 如果到开头
						if (j == start) {
							// 跳出
							break;
						}
						// 如果是开头第2个
						if (j == start + siz) {
							// 第一个（还是要和开头的比较一下，因为没比过）
							j -= siz;
							// i大于j, 不需要移到开头。
							if (cmp(i, j) > 0) {
								// 右移（插在1，2号元素中间）
								j += siz;
							}
							// 跳出
							break;
						}
					} while (1);
					// 从i 到 j 
					for (k = i; k > j; k -= siz) {
						// 两两交换，让i冒泡上去
						swp(k, k - siz);
					}
				}
			}
			break;
	}
}
/* }}} */

// 这里都是 license
/* {{{ ZEND_API void zend_sort(void *base, size_t nmemb, size_t siz, compare_func_t cmp, swap_func_t swp)
 *
 * Derived from LLVM's libc++ implementation of std::sort.
 *
 * ===========================================================================
 * libc++ License
 * ===========================================================================
 *
 * The libc++ library is dual licensed under both the University of Illinois
 * "BSD-Like" license and the MIT license. As a user of this code you may
 * choose to use it under either license. As a contributor, you agree to allow
 * your code to be used under both.
 *
 * Full text of the relevant licenses is included below.
 *
 * ===========================================================================
 *
 * University of Illinois/NCSA
 * Open Source License
 *
 * Copyright (c) 2009-2012 by the contributors listed at
 * http://llvm.org/svn/llvm-project/libcxx/trunk/CREDITS.TXT
 *
 * All rights reserved.
 *
 * Developed by:
 *
 *     LLVM Team
 *
 *     University of Illinois at Urbana-Champaign
 *
 *     http://llvm.org
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal with the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 *     * Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimers.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimers in the
 *       documentation and/or other materials provided with the distribution.
 *
 *     * Neither the names of the LLVM Team, University of Illinois at
 *       Urbana-Champaign, nor the names of its contributors may be used to
 *       endorse or promote products derived from this Software without
 *       specific prior written permission.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * WITH THE SOFTWARE.
 *
 * ===========================================================================
 *
 * Copyright (c) 2009-2012 by the contributors listed at
 * http://llvm.org/svn/llvm-project/libcxx/trunk/CREDITS.TXT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

// 快速排序思路：选位置在中间的数。
// 后面所有的数从两头开始找，左边大于它的第一个和右边小于它的最后一个，交换。 
// 循环下来会找到一个交叉点，它左边的都小于中间数，右边都大于中间数。同时也左边每一个都小于右边每一个。
// 然后递归排序左面一半和右面一半，直到每段只有2个元素为止。

// ing4, 这个快排加插排，可以有效减少递归次数，提高性能。
ZEND_API void zend_sort(void *base, size_t nmemb, size_t siz, compare_func_t cmp, swap_func_t swp)
{
	// 开头就是死循环
	while (1) {
		// 如果少于等于16个
		if (nmemb <= 16) {
			// 使用插入排序
			zend_insert_sort(base, nmemb, siz, cmp, swp);
			// 这里是唯一出口
			return;
		// 大于16个，采用快速排序 
		} else {
			char *i, *j;
			// 开始位置
			char *start = (char *)base;
			// 结束 位置（这里没有元素，end 不会直接使用）
			char *end = start + (nmemb * siz);
			// 偏移量 = 元素数量/2，向下取整
			size_t offset = (nmemb >> Z_L(1));
			// 中心点（如果17个元素，会指向第9个，18个元素，也指向第9个）
			char *pivot = start + (offset * siz);
			// 右移10位，如果还有效 （数量大于1024个元素，已测试）。采用5段比较。
			if ((nmemb >> Z_L(10))) {
				// 1/4位置
				size_t delta = (offset >> Z_L(1)) * siz;
				// 开始位置 1/4位置 1/2位置 3/4位置 结尾位置，比较这5个位置
				zend_sort_5(start, start + delta, pivot, pivot + delta, end - siz, cmp, swp);
			// 否则 采用3段比较。
			} else {
				// 开始位置  1/2位置 结尾位置，比较这3个位置
				zend_sort_3(start, pivot, end - siz, cmp, swp);
			}
			// 前面这一段是为了保证中间数尽量大小适中，这个很重要，可以有效提高快排效率。
			// 这样选出的中心点一定有一个元素比它大，也一定有一个元素比它小（或相等），它不需要被排到两端。
			
			// 交换第2个元素和中心点 （第一个不用动了，前面刚刚排序过了，一定比中心点小（或等））
			// 中心点先放在固定位置，最后移动到正确的位置。
			swp(start + siz, pivot);
			// 修正中心点指针（还是指向原来的那个元素）
			pivot = start + siz;
			
			// 这是真正需要排序的部分，第3个元素到最后一个。
			// 中心向右1个（第三个元素）
			i = pivot + siz;
			// 最后1个
			j = end - siz;
			
			// 死循环，只有goto才会跳出
			while (1) {
				// 跳过左边比中心点小的元素（这些元素不用移动）
				// 中点大于i
				while (cmp(pivot, i) > 0) {
					// i右移
					i += siz;
					// 位置交叉（不需要交换），不需要和结尾点比较，因为前面选中心点时保证了，结尾一定比中心点大。
					if (UNEXPECTED(i == j)) {
						// 完成
						goto done;
					}
				}
				// j左移
				j -= siz;
				// 位置交叉（不需要交换）
				if (UNEXPECTED(j == i)) {
					// 完成
					goto done;
				}
				// 跳过右边比中心点大的元素（这些元素不用移动）
				// j大于中点
				while (cmp(j, pivot) > 0) {
					// j左移
					j -= siz;
					// 位置交叉（不需要交换）
					if (UNEXPECTED(j == i)) {
						// 完成
						goto done;
					}
				}
				// 这时候找到了左边第一个比中心点大（等）的，和右边第一个比中心点小（等）的
				// 交换i,j
				swp(i, j);
				// i 右移
				i += siz;
				// 位置交叉（不需要交换）
				if (UNEXPECTED(i == j)) {
					// 完成
					goto done;
				}
			}
			// 这轮操作死循环产生的结果是，【最后交叉的这个点】的 左边没有比 pivot 大的，右边没有比 pivot 小的元素。
done:
			// 把交叉点左边一个元素存放中心点。中心点的位置一定要正确，这个很重要。
			// 因为i是先找的，这时候它要么是需要移位的元素，要么是结尾元素，动它左边的元素肯定不会有问题。
			// 把这个元素和pivot交换，是为了把pivot位置放对，这个元素存放的位置还只是临时位置，后面会继续排，
			// 但它肯定在交叉点左边，就不会错了。
			swp(pivot, i - siz);
			// 这里的思路是：交叉两边元素少的一边递归，多的一边继续快速排序。 
			
			// 这时候 i-siz 是中心点。
			// 如果中心点左边的元素 没有右边的多。中心点 - 开始 < 结尾 -i
			if ((i - siz) - start < end - i) {
				// 递归：排序左边一段
				zend_sort(start, (i - start)/siz - 1, siz, cmp, swp);
				// 右边一半继续循环排序
				base = i;
				// 数量 = 后面留下的部分，这部分要重新排。
				nmemb = (end - i)/siz;
			// 如果中心点左边的元素 比右边的多。中心点 - 开始 >= 结尾 -i
			} else {
				// 递归：排序右边一段
				zend_sort(i, (end - i)/siz, siz, cmp, swp);
				// 数量 = i -1（-1是因为 中心点 不用排了）
				nmemb = (i - start)/siz - 1;
			}
			// 还在外层死循环里
		}
	}
}
/* }}} */
