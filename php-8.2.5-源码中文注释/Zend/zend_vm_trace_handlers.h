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

#include "zend_sort.h"

#define VM_TRACE(op)     zend_vm_trace(#op, sizeof(#op)-1);
#define VM_TRACE_START() zend_vm_trace_init();
#define VM_TRACE_END()   zend_vm_trace_finish();

static HashTable vm_trace_ht;

// ing4, 记录两个操作码按固定顺序出现的次数
// 每当有新的 操作码 进来，把（旧的+ +新的）在哈希表里计数，最新的一个保存在 last中
static void zend_vm_trace(const char *op, size_t op_len)
{
	// 静态变量 last 和 last_len
	static const char *last = NULL;
	static size_t last_len = 0;
	// buf 最长256个字符 
	char buf[256];
	// buf的长度
	size_t len;
	zval tmp, *zv;
	// 如果 last 有效
	if (EXPECTED(last)) {
		// last的长度 + 1（空格的长度） +操作码长度
		len = last_len + 1 + op_len;
		// last 复制到 缓存里
		memcpy(buf, last, last_len);
		// 后面加一个空格
		buf[last_len] = ' ';
		// 再把操作码 op 复制进来，拼成一串
		memcpy(buf + last_len + 1, op, op_len + 1);
		// 在 trace 哈希表里用 buf 作为key来查找 
		zv = zend_hash_str_find(&vm_trace_ht, buf, len);
		// 如果找到
		if (EXPECTED(zv)) {
			// 如果整数 zv 有效
			if (EXPECTED(Z_LVAL_P(zv) < ZEND_LONG_MAX)) {
				// 整数 +1
				Z_LVAL_P(zv)++;
			}
		// 没找到
		} else {
			// tmp =1
			ZVAL_LONG(&tmp, 1);
			// 添加到trace哈希表中
			zend_hash_str_add_new(&vm_trace_ht, buf, len, &tmp);
		}
	}
	// 传入的 op 和 len 记录
	last = op;
	last_len = op_len;
}

// ing4, 比较两个 Bucket 的大小
static int zend_vm_trace_compare(const Bucket *p1, const Bucket *p2)
{
	// 第一个值小于第二个
	if (Z_LVAL(p1->val) < Z_LVAL(p2->val)) {
		//
		return 1;
	// 第一个值大于第二个
	} else if (Z_LVAL(p1->val) > Z_LVAL(p2->val)) {
		return -1;
	// 两个值相等
	} else {
		return 0;
	}
}

// ing4, 追踪完毕,把结果打印到文件里
static void zend_vm_trace_finish(void)
{
	zend_string *key;
	zval *val;
	FILE *f;
	// 打开文件， 追踪记录写进 zend_vm_trace.log
	f = fopen("zend_vm_trace.log", "w+");
	if (f) {
		// 先把哈希表按 操作码出现次数排序，升序
		zend_hash_sort(&vm_trace_ht, (compare_func_t)zend_vm_trace_compare, 0);
		// 遍历追踪哈希表
		ZEND_HASH_MAP_FOREACH_STR_KEY_VAL(&vm_trace_ht, key, val) {
			// 打印 key + value + 换行
			fprintf(f, "%s "ZEND_LONG_FMT"\n", ZSTR_VAL(key), Z_LVAL_P(val));
		} ZEND_HASH_FOREACH_END();
		// 关闭文件
		fclose(f);
	}
	// 销毁哈希表
	zend_hash_destroy(&vm_trace_ht);
}

// ing4, 初始化虚拟机跟踪。把旧的缓存文件载入进来
static void zend_vm_trace_init(void)
{
	FILE *f;
	// 初始化哈希表
	zend_hash_init(&vm_trace_ht, 0, NULL, NULL, 1);
	// 打开文件
	f = fopen("zend_vm_trace.log", "r");
	if (f) {
		char buf[256];
		size_t len;
		zval tmp;
		// 如果没到结尾
		while (!feof(f)) {
			// 读取256个字符 
			if (fgets(buf, sizeof(buf)-1, f)) {
				// 有效字符长度
				len = strlen(buf);
				// 倒着遍历，找到最后面所有 ascii编号 小于 32 的 控制字符，把它们替换成0
				while (len > 0 && buf[len-1] <= ' ') {
					len--;
					buf[len] = 0;
				}
				// 倒着继续遍历，跳过所有 0-9的字串（上文中写入的次数）
				while (len > 0 && buf[len-1] >= '0' && buf[len-1] <= '9') {
					len--;
				}
				// 如果还有数据。剩下的是两个操作码，中间一个空格
				if (len > 1) {
					// 最后一个不可用字串写成0
					buf[len-1] = 0;
					// 取出后面的一串数字，转成整数放进tmp里
					ZVAL_LONG(&tmp, ZEND_STRTOL(buf + len, NULL, 10));
					// 把 操作码串 和引用次数 插入哈希表里
					zend_hash_str_add(&vm_trace_ht, buf, len - 1, &tmp);
				}
			}
		}
		fclose(f);
	}
}
