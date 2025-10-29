/*
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.00 of the Zend license,     |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.zend.com/license/2_00.txt.                                |
   | If you did not receive a copy of the Zend license and are unable to  |
   | obtain it through the world-wide-web, please send a note to          |
   | license@zend.com so we can mail you a copy immediately.              |
   +----------------------------------------------------------------------+
   | Authors: Max Kellermann <max.kellermann@ionos.com>                   |
   +----------------------------------------------------------------------+
*/

#ifndef ZEND_MMAP_H
#define ZEND_MMAP_H

#include "zend_portability.h"

#ifdef __linux__
# include <sys/prctl.h>

// 后备定义，如果 libc 版本低于内核版本 时启用
/* fallback definitions if our libc is older than the kernel */
# ifndef PR_SET_VMA
#  define PR_SET_VMA 0x53564d41
# endif

# ifndef PR_SET_VMA_ANON_NAME
#  define PR_SET_VMA_ANON_NAME 0
# endif
#endif // __linux__

/**
   ing1, 给指定的内存区域设置名称
 * Set a name for the specified memory area.
 *
   这个 feature 要在linux5.7以上才能使用
 * This feature requires Linux 5.17.
 */
static zend_always_inline void zend_mmap_set_name(const void *start, size_t len, const char *name)
{
// 如果是linux操作系统 
#ifdef __linux__
	// ?
	prctl(PR_SET_VMA, PR_SET_VMA_ANON_NAME, (unsigned long)start, len, (unsigned long)name);
#endif
}

#endif /* ZEND_MMAP_H */
