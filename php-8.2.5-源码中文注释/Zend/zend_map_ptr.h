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

// ing3, 这个东东不难但有点绕

#ifndef ZEND_MAP_PTR_H
#define ZEND_MAP_PTR_H

#include "zend_portability.h"

#define ZEND_MAP_PTR_KIND_PTR           0
#define ZEND_MAP_PTR_KIND_PTR_OR_OFFSET 1

#define ZEND_MAP_PTR_KIND ZEND_MAP_PTR_KIND_PTR_OR_OFFSET

// ing4, 获得变量名，后面加个 __ptr,例如：ZEND_MAP_PTR(name) => name__ptr （测试过）
#define ZEND_MAP_PTR(ptr) \
	ptr ## __ptr
// ing4, 定义变量名，前面是类型，后面加个 __ptr,例如：ZEND_MAP_PTR_DEF(int,name) => int name__ptr;（测试过）
#define ZEND_MAP_PTR_DEF(type, name) \
	type ZEND_MAP_PTR(name)
	
// ing4, 根据偏移量取回指定次序的指针	
#define ZEND_MAP_PTR_OFFSET2PTR(offset) \
	((void**)((char*)CG(map_ptr_base) + offset))

// ing4, 返回指针相对于 CG(map_ptr_base) 偏移的byte数量。 （64位系统中，char占一个byte，指针 void(*) 占8个byte）
#define ZEND_MAP_PTR_PTR2OFFSET(ptr) \
	((void*)(((char*)(ptr)) - ((char*)CG(map_ptr_base))))
	
// ing4, 变量赋值，变量后面加个 __ptr,例如：ZEND_MAP_PTR_INIT(name,1) => name__ptr=1;（测试过）
#define ZEND_MAP_PTR_INIT(ptr, val) do { \
		ZEND_MAP_PTR(ptr) = (val); \
	} while (0)

// ing4, 声名新指针 ，并把创建好的指针偏移量存到指针中。这里返回的其实是个整数，用指针存是为了方便后面把这个整数转成指针使用。
// CG(map_ptr_real_base) 指针列表中 创建新的指针，并返回新指针相对于 CG(map_ptr_base) 的偏移量		
#define ZEND_MAP_PTR_NEW(ptr) do { \
		ZEND_MAP_PTR(ptr) = zend_map_ptr_new(); \
	} while (0)

// 默认情况下相等（上面定义成相等了）
#if ZEND_MAP_PTR_KIND == ZEND_MAP_PTR_KIND_PTR_OR_OFFSET

// ing4, CG(map_ptr_real_base) 指针列表中 创建新的指针，并返回新指针相对于 CG(map_ptr_base) 的偏移量(32位整数）
# define ZEND_MAP_PTR_NEW_OFFSET() \
	((uint32_t)(uintptr_t)zend_map_ptr_new())

// ing4, 检验是否是偏移量？为什么结尾一定是1。 
// 因为 CG(map_ptr_base) 作为基础值，有一个 -1，所以所有的偏移量最后一位都是1。见：ZEND_MAP_PTR_BIASED_BASE
# define ZEND_MAP_PTR_IS_OFFSET(ptr) \
	(((uintptr_t)ZEND_MAP_PTR(ptr)) & 1L)

// ing3, 自动适配，返回要求的指针
# define ZEND_MAP_PTR_GET(ptr) \
	/* 如果是偏移量 */ \
	((ZEND_MAP_PTR_IS_OFFSET(ptr) ? \
		/* 根据偏移量取回指定的指针 */ \
		ZEND_MAP_PTR_GET_IMM(ptr) : \
		/* 直接返回对应的指针元素 */ \
		((void*)(ZEND_MAP_PTR(ptr)))))

// ing3, 根据偏移量取回指定的指针
# define ZEND_MAP_PTR_GET_IMM(ptr) \
	(*ZEND_MAP_PTR_OFFSET2PTR((uintptr_t)ZEND_MAP_PTR(ptr)))
	
// ing3, 自动适配赋值
# define ZEND_MAP_PTR_SET(ptr, val) do { \
		/* 如果是偏移量 */ \
		if (ZEND_MAP_PTR_IS_OFFSET(ptr)) { \
			/* 通过偏移量给目标指针赋值 */ \
			ZEND_MAP_PTR_SET_IMM(ptr, val); \
		/* 如果不是偏移量 */ \
		} else { \
			/* 指针赋值 */ \
			ZEND_MAP_PTR_INIT(ptr, val); \
		} \
	} while (0)
		
// ing4, 通过偏移量给目标指针赋值（有点绕）
# define ZEND_MAP_PTR_SET_IMM(ptr, val) do { \
		/* 根据偏移量取回指定次序的指针 */ \
		void **__p = ZEND_MAP_PTR_OFFSET2PTR((uintptr_t)ZEND_MAP_PTR(ptr)); \
		/* 给指针的目标指针赋值 */ \
		*__p = (val); \
	} while (0)

// ing4, 把指针整数值-1，在计算offset时，所有的值最后都会+1。这个指针数值不能直接用。
// 测试过指针左移： char *t = "123";	t++;	char *t3 = (char *)ZEND_MAP_PTR_BIASED_BASE(t);
# define ZEND_MAP_PTR_BIASED_BASE(real_base) \
	((void*)(((uintptr_t)(real_base)) - 1))
	
// 如果不相等，报错（可能是编译器的特殊情况？）
#else
# error "Unknown ZEND_MAP_PTR_KIND"
#endif

BEGIN_EXTERN_C()

ZEND_API void  zend_map_ptr_reset(void);
ZEND_API void *zend_map_ptr_new(void);
ZEND_API void  zend_map_ptr_extend(size_t last);
ZEND_API void zend_alloc_ce_cache(zend_string *type_name);

END_EXTERN_C()

#endif /* ZEND_MAP_PTR_H */
