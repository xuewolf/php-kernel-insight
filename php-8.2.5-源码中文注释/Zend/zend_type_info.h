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

#ifndef ZEND_TYPE_INFO_H
#define ZEND_TYPE_INFO_H

#include "zend_types.h"

// zend_type 结构体用到的类型常量

// 10种基本类型
// IS_UNDEF : 0
#define MAY_BE_UNDEF                (1 << IS_UNDEF)
// IS_NULL : 1
#define MAY_BE_NULL		            (1 << IS_NULL)
// IS_FALSE : 2
#define MAY_BE_FALSE	            (1 << IS_FALSE)
// IS_TRUE : 3
#define MAY_BE_TRUE		            (1 << IS_TRUE)
// 2+3
#define MAY_BE_BOOL                 (MAY_BE_FALSE|MAY_BE_TRUE)
// IS_LONG : 4
#define MAY_BE_LONG		            (1 << IS_LONG)
// IS_DOUBLE : 5
#define MAY_BE_DOUBLE	            (1 << IS_DOUBLE)
// IS_STRING : 6
#define MAY_BE_STRING	            (1 << IS_STRING)
// IS_ARRAY : 7
#define MAY_BE_ARRAY	            (1 << IS_ARRAY)
// IS_OBJECT : 8
#define MAY_BE_OBJECT	            (1 << IS_OBJECT)
// IS_RESOURCE : 9
#define MAY_BE_RESOURCE	            (1 << IS_RESOURCE)
// 9个1
#define MAY_BE_ANY                  (MAY_BE_NULL|MAY_BE_FALSE|MAY_BE_TRUE|MAY_BE_LONG|MAY_BE_DOUBLE|MAY_BE_STRING|MAY_BE_ARRAY|MAY_BE_OBJECT|MAY_BE_RESOURCE)
// IS_REFERENCE : 10
#define MAY_BE_REF                  (1 << IS_REFERENCE) /* may be reference */

// 这些在 zend_type中使用，但不能用于type推理
// 在推理过程中，它们可以交叠
/* These are used in zend_type, but not for type inference.
 * They are allowed to overlap with types used during inference. */
// IS_CALLABLE : 12
#define MAY_BE_CALLABLE             (1 << IS_CALLABLE)
// IS_VOID : 14
#define MAY_BE_VOID                 (1 << IS_VOID)
// IS_NEVER : 17
#define MAY_BE_NEVER                (1 << IS_NEVER)
// IS_STATIC : 15
#define MAY_BE_STATIC               (1 << IS_STATIC)


// IS_REFERENCE : 10 , 这个是给下面用的
#define MAY_BE_ARRAY_SHIFT          (IS_REFERENCE)

// 这些都是数组类型
// 1 << 1 << 10
#define MAY_BE_ARRAY_OF_NULL		(MAY_BE_NULL     << MAY_BE_ARRAY_SHIFT)
// 1 << 2 << 10
#define MAY_BE_ARRAY_OF_FALSE		(MAY_BE_FALSE    << MAY_BE_ARRAY_SHIFT)
// 1 << 3 << 10
#define MAY_BE_ARRAY_OF_TRUE		(MAY_BE_TRUE     << MAY_BE_ARRAY_SHIFT)
// 1 << 4 << 10
#define MAY_BE_ARRAY_OF_LONG		(MAY_BE_LONG     << MAY_BE_ARRAY_SHIFT)
// 1 << 5 << 10
#define MAY_BE_ARRAY_OF_DOUBLE		(MAY_BE_DOUBLE   << MAY_BE_ARRAY_SHIFT)
// 1 << 6 << 10
#define MAY_BE_ARRAY_OF_STRING		(MAY_BE_STRING   << MAY_BE_ARRAY_SHIFT)
// 1 << 7 << 10
#define MAY_BE_ARRAY_OF_ARRAY		(MAY_BE_ARRAY    << MAY_BE_ARRAY_SHIFT)
// 1 << 8 << 10
#define MAY_BE_ARRAY_OF_OBJECT		(MAY_BE_OBJECT   << MAY_BE_ARRAY_SHIFT)
// 1 << 9 << 10
#define MAY_BE_ARRAY_OF_RESOURCE	(MAY_BE_RESOURCE << MAY_BE_ARRAY_SHIFT)
// 9个1 << 10
#define MAY_BE_ARRAY_OF_ANY			(MAY_BE_ANY      << MAY_BE_ARRAY_SHIFT)
// 1 << 10 << 10
#define MAY_BE_ARRAY_OF_REF			(MAY_BE_REF      << MAY_BE_ARRAY_SHIFT)

// 顺序数组
#define MAY_BE_ARRAY_PACKED         (1<<21)
// 数字哈希表
#define MAY_BE_ARRAY_NUMERIC_HASH   (1<<22) /* hash with numeric keys */
// 字串哈希表
#define MAY_BE_ARRAY_STRING_HASH    (1<<23) /* hash with string keys */

// 顺序数组 或 数字哈希表
#define MAY_BE_ARRAY_KEY_LONG       (MAY_BE_ARRAY_PACKED | MAY_BE_ARRAY_NUMERIC_HASH)
// 字串哈希表 的别名
#define MAY_BE_ARRAY_KEY_STRING     MAY_BE_ARRAY_STRING_HASH
// 任何一种数组
#define MAY_BE_ARRAY_KEY_ANY        (MAY_BE_ARRAY_KEY_LONG | MAY_BE_ARRAY_KEY_STRING)

// 检查：是否是顺序数组
#define MAY_BE_PACKED(t)            ((t) & MAY_BE_ARRAY_PACKED)
// 检查：是 数字哈希 表 或字串哈希表
#define MAY_BE_HASH(t)              ((t) & (MAY_BE_ARRAY_NUMERIC_HASH | MAY_BE_ARRAY_KEY_STRING))
// 检查：只是顺序数组，不是数字或字串 哈希表
#define MAY_BE_PACKED_ONLY(t)       (MAY_BE_PACKED(t) && !MAY_BE_HASH(t))
// 检查：只是数字或字串 哈希表，不是顺序数组
#define MAY_BE_HASH_ONLY(t)         (MAY_BE_HASH(t) && !MAY_BE_PACKED(t))

// 数组以外的高级类型
// 类
#define MAY_BE_CLASS                (1<<24)
// 间接引用
#define MAY_BE_INDIRECT             (1<<25)
// 非引用类型并且引用次数1，RC = reference count
#define MAY_BE_RC1                  (1<<30) /* may be non-reference with refcount == 1 */
// 非引用类型并且引用次数>1
#define MAY_BE_RCN                  (1u<<31) /* may be non-reference with refcount > 1  */

// 只有这个稍微麻烦一点
// 可能是数组的类型 1<<17 | 1<<(22,23,24) | 9个1 << 10 | 1<<20
#define MAY_BE_ANY_ARRAY \
	(MAY_BE_ARRAY|MAY_BE_ARRAY_KEY_ANY|MAY_BE_ARRAY_OF_ANY|MAY_BE_ARRAY_OF_REF)

#endif /* ZEND_TYPE_INFO_H */
