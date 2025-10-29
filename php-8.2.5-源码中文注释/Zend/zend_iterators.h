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
   | Author: Wez Furlong <wez@thebrainroom.com>                           |
   |         Marcus Boerger <helly@php.net>                               |
   +----------------------------------------------------------------------+
*/

// 这些迭代器设计用来在 引擎提供的 foreach() 语法结构中使用
// 但可以扩展成和其他迭代引擎操作码一起使用。
// 这些方法 和zend_hash接口 有相似的语义和相似的名字。
/* These iterators were designed to operate within the foreach()
 * structures provided by the engine, but could be extended for use
 * with other iterative engine opcodes.
 * These methods have similar semantics to the zend_hash API functions
 * with similar names.
 * */

typedef struct _zend_object_iterator zend_object_iterator;

// 迭代器方法
typedef struct _zend_object_iterator_funcs {
	// 销毁与此迭代器相关的所有资源
	/* release all resources associated with this iterator instance */
	void (*dtor)(zend_object_iterator *iter);
	
	// 检验是否迭代到结尾
	/* check for end of iteration (FAILURE or SUCCESS if data is valid) */
	int (*valid)(zend_object_iterator *iter);

	/* fetch the item data for the current element */
	zval *(*get_current_data)(zend_object_iterator *iter);

	/* fetch the key for the current element (optional, may be NULL). The key
	 * should be written into the provided zval* using the ZVAL_* macros. If
	 * this handler is not provided auto-incrementing integer keys will be
	 * used. */
	void (*get_current_key)(zend_object_iterator *iter, zval *key);
	
	// 找到下一个元素
	/* step forwards to next element */
	void (*move_forward)(zend_object_iterator *iter);

	// 游标复位 ?
	/* rewind to start of data (optional, may be NULL) */
	void (*rewind)(zend_object_iterator *iter);
	
	// 使当前的key无效？
	/* invalidate current value/key (optional, may be NULL) */
	void (*invalidate_current)(zend_object_iterator *iter);

	/* Expose owned values to GC.
	 * This has the same semantics as the corresponding object handler. */
	HashTable *(*get_gc)(zend_object_iterator *iter, zval **table, int *n);
} zend_object_iterator_funcs;

// 对象迭代器
struct _zend_object_iterator {
	// 迭代器对象
	zend_object std;
	//
	zval data;
	// 方法列表
	const zend_object_iterator_funcs *funcs;
	// 对 fe_reset，fe_fetch 操作码 私有
	zend_ulong index; /* private to fe_reset/fe_fetch opcodes */
};

// 类实现【循环迭代】需要提供的方法
typedef struct _zend_class_iterator_funcs {
	// 创建迭代器
	zend_function *zf_new_iterator;
	// 验证有效
	zend_function *zf_valid;
	// 获取当前值
	zend_function *zf_current;
	// 获取当前key
	zend_function *zf_key;
	// 游标后移
	zend_function *zf_next;
	// ？
	zend_function *zf_rewind;
} zend_class_iterator_funcs;

// clear, 类实现【类似数组操作】需要提供的方法
typedef struct _zend_class_arrayaccess_funcs {
	// 读取
	zend_function *zf_offsetget;
	// 检验是否存在
	zend_function *zf_offsetexists;
	// 创建
	zend_function *zf_offsetset;
	// 删除
	zend_function *zf_offsetunset;
} zend_class_arrayaccess_funcs;

BEGIN_EXTERN_C()
/* given a zval, returns stuff that can be used to iterate it. */
ZEND_API zend_object_iterator* zend_iterator_unwrap(zval *array_ptr);

/* given an iterator, wrap it up as a zval for use by the engine opcodes */
ZEND_API void zend_iterator_init(zend_object_iterator *iter);
ZEND_API void zend_iterator_dtor(zend_object_iterator *iter);

ZEND_API void zend_register_iterator_wrapper(void);
END_EXTERN_C()
