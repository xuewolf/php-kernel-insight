/*
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
   | Authors: Sammy Kaye Powers <sammyk@php.net>                          |
   |          Dmitry Stogov <dmitry@php.net>                              |
   +----------------------------------------------------------------------+
 */

#include "php.h"
#include "zend_system_id.h"
#include "zend_extensions.h"
#include "ext/standard/md5.h"
#include "ext/hash/php_hash.h"

// 创建的系统ID，32个16进制字符
ZEND_API char zend_system_id[32];

// md5上下文
static PHP_MD5_CTX context;
// 是否已结束
static int finalized = 0;

// ing3, 增加操作系统熵,在生成 zend_system_id 的md5上下文中添加内容
// p1:模块或，p2:钩子名，p3:数据，p4:数据大小
ZEND_API ZEND_RESULT_CODE zend_add_system_entropy(const char *module_name, const char *hook_name, const void *data, size_t size)
{
	// 如果未终结
	if (finalized == 0) {
		// ext\standard\md5.c 中定义
		// 更新md5上下文, 准备生成md5哈希值
		PHP_MD5Update(&context, module_name, strlen(module_name));
		// 更新md5上下文, 准备生成md5哈希值
		PHP_MD5Update(&context, hook_name, strlen(hook_name));
		// 
		if (size) {
			// 更新md5上下文, 准备生成md5哈希值
			PHP_MD5Update(&context, data, size);
		}
		// 返回成功
		return SUCCESS;
	}
	// 返回失败
	return FAILURE;
}

// 64位windows: "BIN_4488(size_t)8" (测试过）
#define ZEND_BIN_ID "BIN_" ZEND_TOSTR(SIZEOF_INT) ZEND_TOSTR(SIZEOF_LONG) ZEND_TOSTR(SIZEOF_SIZE_T) ZEND_TOSTR(SIZEOF_ZEND_LONG) ZEND_TOSTR(ZEND_MM_ALIGNMENT)

// ing3, 开始创建系统ID
void zend_startup_system_id(void)
{
	// 初始化md5上下文
	PHP_MD5Init(&context);
	// 更新md5上下文, 准备生成md5哈希值
	// 添加php版本信息
	PHP_MD5Update(&context, PHP_VERSION, sizeof(PHP_VERSION)-1);
	// 更新md5上下文, 准备生成md5哈希值
	// 添加 扩展DI
	PHP_MD5Update(&context, ZEND_EXTENSION_BUILD_ID, sizeof(ZEND_EXTENSION_BUILD_ID)-1);
	// 更新md5上下文, 准备生成md5哈希值
	PHP_MD5Update(&context, ZEND_BIN_ID, sizeof(ZEND_BIN_ID)-1);
	if (strstr(PHP_VERSION, "-dev") != 0) {
		// 
		/* Development versions may be changed from build to build */
		// 更新md5上下文, 准备生成md5哈希值
		PHP_MD5Update(&context, __DATE__, sizeof(__DATE__)-1);
		// 更新md5上下文, 准备生成md5哈希值
		PHP_MD5Update(&context, __TIME__, sizeof(__TIME__)-1);
	}
	// 一开始，是空字串
	zend_system_id[0] = '\0';
}

// 语句处理 钩子
#define ZEND_HOOK_AST_PROCESS      (1 << 0)
// 文件编译 钩子
#define ZEND_HOOK_COMPILE_FILE     (1 << 1)
// 执行 钩子
#define ZEND_HOOK_EXECUTE_EX       (1 << 2)
// 内部执行 钩子
#define ZEND_HOOK_EXECUTE_INTERNAL (1 << 3)

// ing3, 完成系统ID创建
void zend_finalize_system_id(void)
{
	// 16个字节 256个位
	unsigned char digest[16];
	// 钩子标记
	zend_uchar hooks = 0;

	// 判断这几个钩子方法是否存在
	// 有 语句处理 方法
	if (zend_ast_process) {
		// 有 语句处理 钩子
		hooks |= ZEND_HOOK_AST_PROCESS;
	}
	// 如果不是标准 文件编译方法
	if (zend_compile_file != compile_file) {
		// 有 文件编译 钩子
		hooks |= ZEND_HOOK_COMPILE_FILE;
	}
	// 如果不是标准执行方法
	if (zend_execute_ex != execute_ex) {
		// 有 执行 钩子
		hooks |= ZEND_HOOK_EXECUTE_EX;
	}
	// 如果不是标准内部执行方法
	if (zend_execute_internal) {
		// 有 内部执行 钩子
		hooks |= ZEND_HOOK_EXECUTE_INTERNAL;
	}
	// 更新md5上下文, 准备生成md5哈希值
	PHP_MD5Update(&context, &hooks, sizeof hooks);

	// 0到255
	for (int16_t i = 0; i < 256; i++) {
		// 获取用户操作码处理器，如果有
		if (zend_get_user_opcode_handler((zend_uchar) i) != NULL) {
			// 更新md5上下文, 准备生成md5哈希值
			PHP_MD5Update(&context, &i, sizeof i);
		}
	}

	// 使用上下文，生成md5值
	PHP_MD5Final(digest, &context);
	// md5值从2进制转成16进制，放进 zend_system_id
	php_hash_bin2hex(zend_system_id, digest, sizeof digest);
	// 已结束
	finalized = 1;
}
