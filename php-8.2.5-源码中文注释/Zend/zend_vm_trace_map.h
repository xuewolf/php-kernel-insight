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

#include "zend_vm_handlers.h"
#include "zend_sort.h"

// n是执行码号，name是执行码名，见 zend_vm_handlers.h
// ing4, 把执行码添加到哈希表里。sizeof可直接获取长度，测试过 sizeof("123")
#define GEN_MAP(n, name) do { \
		/* 把执行码号写到 tmp 里 */ \
		ZVAL_LONG(&tmp, (zend_long)(uintptr_t)zend_opcode_handlers[n]); \
		/* tmp 添加到哈希表里，#name是key, 后面是#name长度 */ \
		zend_hash_str_add(&vm_trace_ht, #name, sizeof(#name) - 1, &tmp); \
	} while (0);

// ing4, 开始追踪
#define VM_TRACE_START() do { \
		zval tmp; \
		/* 初始化哈希表 */ \
		zend_hash_init(&vm_trace_ht, 0, NULL, NULL, 1); \
		/* 遍历 1848 多个执行码 */ \
		VM_HANDLERS(GEN_MAP) \
		/* 初始化 vm 追踪器 */ \
		zend_vm_trace_init(); \
	} while (0)

// 64位系统 地址格式
#ifdef _WIN64
# define ADDR_FMT "%016I64x"
// 32位地址格式
#elif SIZEOF_SIZE_T == 4
# define ADDR_FMT "%08zx"
// 非64位系统 ，64位地址格式
#elif SIZEOF_SIZE_T == 8
# define ADDR_FMT "%016zx"
// 其他情况，报错
#else
# error "Unknown SIZEOF_SIZE_T"
#endif

static HashTable vm_trace_ht;

// ing4, 哈希表排序比较函数 
static int zend_vm_trace_compare(const Bucket *p1, const Bucket *p2)
{
	// 当成整数比较，第一个大于第二个
	if (Z_LVAL(p1->val) > Z_LVAL(p2->val)) {
		return 1;
	// 当成整数比较，第一个小于第二个
	} else if (Z_LVAL(p1->val) < Z_LVAL(p2->val)) {
		return -1;
	// 相等
	} else {
		return 0;
	}
}

// ing2, 初始化 vm 追踪器
static void zend_vm_trace_init(void)
{
	FILE *f;
	zend_string *key, *prev_key;
	zval *val;
	zend_long prev_addr;
	// 文件位置 zend_vm.map
	f = fopen("zend_vm.map", "w+");
	//
	if (f) {
		// 排序哈希表
		zend_hash_sort(&vm_trace_ht, (bucket_compare_func_t)zend_vm_trace_compare, 0);
		prev_key = NULL;
		// 遍历哈希表
		ZEND_HASH_MAP_FOREACH_STR_KEY_VAL(&vm_trace_ht, key, val) {
			// 如果有前一个key
			if (prev_key) {
				// 写入文件 prev_addr, Z_LVAL_P(val) - prev_addr, ZSTR_VAL(prev_key)
				fprintf(f, ADDR_FMT" "ADDR_FMT" t %s\n", prev_addr, Z_LVAL_P(val) - prev_addr, ZSTR_VAL(prev_key));
			}
			// 覆盖前一个key
			prev_key  = key;
			// 覆盖前一个地址
			prev_addr = Z_LVAL_P(val);
		} ZEND_HASH_FOREACH_END();
		// 如果有前一个key
		if (prev_key) {
			// 写入文件 prev_addr, Z_LVAL_P(val) - prev_addr, ZSTR_VAL(prev_key)
			fprintf(f, ADDR_FMT" "ADDR_FMT" t %s\n", prev_addr, 0, ZSTR_VAL(prev_key));
		}
		fclose(f);
	}
	// 销毁哈希表
	zend_hash_destroy(&vm_trace_ht);
}
