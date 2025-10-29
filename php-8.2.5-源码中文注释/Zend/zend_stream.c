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
   | Authors: Wez Furlong <wez@thebrainroom.com>                          |
   |          Scott MacVicar <scottmac@php.net>                           |
   |          Nuno Lopes <nlopess@php.net>                                |
   |          Marcus Boerger <helly@php.net>                              |
   +----------------------------------------------------------------------+
*/

#include "zend.h"
#include "zend_compile.h"
#include "zend_stream.h"

// 引用外部函数（windows系统有这个函数）
ZEND_DLIMPORT int isatty(int fd);

// ing3, 读取文件开头的指定长度 ？
static ssize_t zend_stream_stdio_reader(void *handle, char *buf, size_t len) /* {{{ */
{
	// 从上次读取位置开始，读取指定长度
	return fread(buf, 1, len, (FILE*)handle);
} /* }}} */

// ing3, 关联文件指针
static void zend_stream_stdio_closer(void *handle) /* {{{ */
{
	// 如果文件指针存在 并且不是 stdin
	if (handle && (FILE*)handle != stdin) {
		// 关闭文件指针
		fclose((FILE*)handle);
	}
} /* }}} */

// ing3, 计算文件大小
static size_t zend_stream_stdio_fsizer(void *handle) /* {{{ */
{
	zend_stat_t buf = {0};
	// 如果获取文件状态成功
	if (handle && zend_fstat(fileno((FILE*)handle), &buf) == 0) {
#ifdef S_ISREG
		// 如果缓存模式不是 S_IFREG ：zend_virtual_cwd.h 
		if (!S_ISREG(buf.st_mode)) {
			// 返回0
			return 0;
		}
#endif
		// 返回文件大小
		return buf.st_size;
	}
	// 获取失败
	return -1;
} /* }}} */

// ing2, 读取文件大小
static size_t zend_stream_fsize(zend_file_handle *file_handle) /* {{{ */
{
	// 类型是文件流指针
	ZEND_ASSERT(file_handle->type == ZEND_HANDLE_STREAM);
	// ？？
	if (file_handle->handle.stream.isatty) {
		return 0;
	}
	// 调用文件流实例的获取大小方法
	return file_handle->handle.stream.fsizer(file_handle->handle.stream.handle);
} /* }}} */

// ing4, 初始化文件指针
ZEND_API void zend_stream_init_fp(zend_file_handle *handle, FILE *fp, const char *filename) {
	// 先把文件句柄清空
	memset(handle, 0, sizeof(zend_file_handle));
	// 类型为使用文件指针
	handle->type = ZEND_HANDLE_FP;
	// 复制指针
	handle->handle.fp = fp;
	// 复制文件名，把char * 转成 zend_string
	handle->filename = filename ? zend_string_init(filename, strlen(filename), 0) : NULL;
}

// ing4, 初始化文件句柄（通过 char *）
ZEND_API void zend_stream_init_filename(zend_file_handle *handle, const char *filename) {
	// 先把文件句柄清空
	memset(handle, 0, sizeof(zend_file_handle));
	// 类型为使用文件名
	handle->type = ZEND_HANDLE_FILENAME;
	// 复制文件名，把char * 转成 zend_string
	handle->filename = filename ? zend_string_init(filename, strlen(filename), 0) : NULL;
}

// ing4, 初始化文件句柄（通过zend_string）
ZEND_API void zend_stream_init_filename_ex(zend_file_handle *handle, zend_string *filename) {
	// 先把文件句柄清空
	memset(handle, 0, sizeof(zend_file_handle));
	// 类型为使用文件名
	handle->type = ZEND_HANDLE_FILENAME;
	// 设置文件名,复制字串
	handle->filename = zend_string_copy(filename);
}

// ing3, 打开文件流
ZEND_API zend_result zend_stream_open(zend_file_handle *handle) /* {{{ */
{
	zend_string *opened_path;
	// 类型必须是文件名
	ZEND_ASSERT(handle->type == ZEND_HANDLE_FILENAME);
	// 如果有打开文件流用的方法，调用它：php_stream_open_for_zend：main.c
	if (zend_stream_open_function) {
		return zend_stream_open_function(handle);
	}
	// 如果没有，使用 zend_open : php_fopen_wrapper_for_zend: main.c
	handle->handle.fp = zend_fopen(handle->filename, &opened_path);
	// 如果打开失败
	if (!handle->handle.fp) {
		// 返回失败
		return FAILURE;
	}
	// 类型改变成使用文件指针
	handle->type = ZEND_HANDLE_FP;
	// 返回成功
	return SUCCESS;
} /* }}} */

// ing3, 从文件中读取一个字节
static int zend_stream_getc(zend_file_handle *file_handle) /* {{{ */
{
	char buf;
	// 调用文件流读取方法，读取内容到缓冲区（ sizeof(buf) 一次一字节）
	if (file_handle->handle.stream.reader(file_handle->handle.stream.handle, &buf, sizeof(buf))) {
		return (int)buf;
	}
	// 如果失败，返回：文件结尾
	return EOF;
} /* }}} */

// ing3, 文件里读取一行，放到缓冲区里
static ssize_t zend_stream_read(zend_file_handle *file_handle, char *buf, size_t len) /* {{{ */
{
	// ？
	if (file_handle->handle.stream.isatty) {
		int c = '*';
		size_t n;
		// 长度未超限且未到文件结尾, 且未到换行符
		for (n = 0; n < len && (c = zend_stream_getc(file_handle)) != EOF && c != '\n'; ++n)  {
			// 一次一个byte放进缓冲区
			buf[n] = (char)c;
		}
		// 如果碰到换行
		if (c == '\n') {
			// 把换行加进来
			buf[n++] = (char)c;
		}
		// 返回读取长度
		return n;
	}
	// 调用文件流的读取方法
	return file_handle->handle.stream.reader(file_handle->handle.stream.handle, buf, len);
} /* }}} */

// ing3, 读取【整个文件】，返回缓冲区指针，读取长度
ZEND_API zend_result zend_stream_fixup(zend_file_handle *file_handle, char **buf, size_t *len) /* {{{ */
{
	size_t file_size;
	// 如果文件句柄缓冲区有效
	if (file_handle->buf) {
		// 直接返回缓冲区指针和长度
		*buf = file_handle->buf;
		*len = file_handle->len;
		return SUCCESS;
	}
	// 文件名类型
	if (file_handle->type == ZEND_HANDLE_FILENAME) {
		// 如果打开文件流失败，返回：失败
		if (zend_stream_open(file_handle) == FAILURE) {
			return FAILURE;
		}
	}
	// 文件指针类型
	if (file_handle->type == ZEND_HANDLE_FP) {
		// 如果没有文件指针，返回：失败
		if (!file_handle->handle.fp) {
			return FAILURE;
		}
		// 类型是文件流指针
		file_handle->type = ZEND_HANDLE_STREAM;
		// 写入指针
		file_handle->handle.stream.handle = file_handle->handle.fp;
		// isatty ？
		file_handle->handle.stream.isatty = isatty(fileno((FILE *)file_handle->handle.stream.handle));
		// 读取文件流函数
		file_handle->handle.stream.reader = (zend_stream_reader_t)zend_stream_stdio_reader;
		// 关闭文件流指针
		file_handle->handle.stream.closer = (zend_stream_closer_t)zend_stream_stdio_closer;
		// 获取文件大小
		file_handle->handle.stream.fsizer = (zend_stream_fsizer_t)zend_stream_stdio_fsizer;
	}
	// 读取文件大小
	file_size = zend_stream_fsize(file_handle);
	// 如果读取失败
	if (file_size == (size_t)-1) {
		// 返回：失败
		return FAILURE;
	}
	// 如果大小有效
	if (file_size) {
		ssize_t read;
		size_t size = 0;
		// 分配缓冲区，大小是整个文件大小，对齐到 32 Byte
		*buf = safe_emalloc(1, file_size, ZEND_MMAP_AHEAD);
		// 一次一行，读取，放在缓冲区里
		while ((read = zend_stream_read(file_handle, *buf + size, file_size - size)) > 0) {
			// 读取数量变大
			size += read;
		}
		// 如果读取过程中出错 
		if (read < 0) {
			// 释放缓冲区
			efree(*buf);
			// 返回失败
			return FAILURE;
		}
		// 更新buf指针
		file_handle->buf = *buf;
		// 更新size ( 缓冲区大小）
		file_handle->len = size;
	// 如果大小无效：一边读一边分配内存，先多分配一些，读取完后再截短。
	} else {
		// 大小0，缓冲区剩余大小4096
		size_t size = 0, remain = 4*1024;
		ssize_t read;
		// 分配缓冲区
		*buf = emalloc(remain);
		// 从上次读取的位置继续读
		while ((read = zend_stream_read(file_handle, *buf + size, remain)) > 0) {
			// 读取大小累加
			size   += read;
			// 缓冲区剩余大小累减
			remain -= read;
			// 如果缓冲区用完了
			if (remain == 0) {
				// 开辟内存大小为 nmemb*size+offset
				*buf   = safe_erealloc(*buf, size, 2, 0);
				// 缓冲区剩余大小
				remain = size;
			}
		}
		// 如果读取失败
		if (read < 0) {
			// 释放缓冲区
			efree(*buf);
			// 返回失败
			return FAILURE;
		}
		// 更新大小
		file_handle->len = size;
		// #define ZEND_MMAP_AHEAD 32
		// 如果size有效且 剩余缓冲区大小小于头大小
		if (size && remain < ZEND_MMAP_AHEAD) {
			// 缓冲区增加一点点，对齐到 ZEND_MMAP_AHEAD
			*buf = safe_erealloc(*buf, size, 1, ZEND_MMAP_AHEAD);
		}
		// 缓冲区连接到文件句柄
		file_handle->buf = *buf;
	}
	// 如果什么也没读取到
	if (file_handle->len == 0) {
		// 减小缓冲区
		*buf = erealloc(*buf, ZEND_MMAP_AHEAD);
		// 缓冲区关联到 文件句柄
		file_handle->buf = *buf;
	}
	// 结尾没有用到的部分，设置成0
	memset(file_handle->buf + file_handle->len, 0, ZEND_MMAP_AHEAD);
	// 返回缓冲区指针
	*buf = file_handle->buf;
	// 返回长度
	*len = file_handle->len;
	// 成功
	return SUCCESS;
} /* }}} */

// ing3, 文件句柄 销毁器
static void zend_file_handle_dtor(zend_file_handle *fh) /* {{{ */
{
	// 根据类型操作
	switch (fh->type) {
		// 文件指针类型
		case ZEND_HANDLE_FP:
			// 如果指针有效
			if (fh->handle.fp) {
				// 关闭文件
				fclose(fh->handle.fp);
				// 清空指针
				fh->handle.fp = NULL;
			}
			break;
		// 文件流指针类型
		case ZEND_HANDLE_STREAM:
			// 如果文件流实例有关闭方法 并且指针存在
			if (fh->handle.stream.closer && fh->handle.stream.handle) {
				// 调用文件流的关闭方法 关闭指针
				fh->handle.stream.closer(fh->handle.stream.handle);
			}
			// 清空文件流指针
			fh->handle.stream.handle = NULL;
			break;
		// 文件名类型：并没有真正打开文件。
		case ZEND_HANDLE_FILENAME:
			// 我们只假设：当销毁used_files哈希时会走到这里，这个哈希并没真正包含打开的文件，只是引用了文件名和路径。
			/* We're only supposed to get here when destructing the used_files hash,
			 * which doesn't really contain open files, but references to their names/paths
			 */
			break;
	}
	// 如果有路径
	if (fh->opened_path) {
		// 删除路径字串
		zend_string_release_ex(fh->opened_path, 0);
		fh->opened_path = NULL;
	}
	// 如果有缓冲区
	if (fh->buf) {
		// 释放缓冲区
		efree(fh->buf);
		fh->buf = NULL;
	}
	// 如果有文件名
	if (fh->filename) {
		// 释放文件名
		zend_string_release(fh->filename);
		fh->filename = NULL;
	}
}
/* }}} */

// 返回int型 以便和 Zend linked list 的API兼容。
/* return int to be compatible with Zend linked list API */
// ing4, 比较两个文件句柄是否相同
static int zend_compare_file_handles(zend_file_handle *fh1, zend_file_handle *fh2) /* {{{ */
{
	// type不同，返回false
	if (fh1->type != fh2->type) {
		return 0;
	}
	// 按type处理
	switch (fh1->type) {
		// 如果有文件名
		case ZEND_HANDLE_FILENAME:
			// 比较文件名是否相同
			return zend_string_equals(fh1->filename, fh2->filename);
		// 如果有文件指针
		case ZEND_HANDLE_FP:
			// 比较指针是否相同
			return fh1->handle.fp == fh2->handle.fp;
		// 如果有文件流指针
		case ZEND_HANDLE_STREAM:
			// 比较文件流指针是否相同
			return fh1->handle.stream.handle == fh2->handle.stream.handle;
		default:
			// 默认返回否
			return 0;
	}
	// 其他情况返回否
	return 0;
} /* }}} */

// ing3, 销毁文件句柄
ZEND_API void zend_destroy_file_handle(zend_file_handle *file_handle) /* {{{ */
{
	// 如果文件句柄在 CG(open_files) 列表中
	if (file_handle->in_list) {
		// 在列表中删除它
		zend_llist_del_element(&CG(open_files), file_handle, (int (*)(void *, void *)) zend_compare_file_handles);
		// zend_file_handle_dtor() 在副本上操作，所以要在这里清空原始数据
		/* zend_file_handle_dtor() operates on the copy, so we have to NULLify the original here */
		// 清空路径
		file_handle->opened_path = NULL;
		// 清空文件名
		file_handle->filename = NULL;
	// 如果文件句柄 不在 CG(open_files) 列表中
	} else {
		// 销毁文件句柄
		zend_file_handle_dtor(file_handle);
	}
} /* }}} */

// ing4, 初始化文件流列表
void zend_stream_init(void) /* {{{ */
{
	// 初始货打开的文件链表， 总尺寸 sizeof(zend_file_handle) ，销毁方法 zend_file_handle_dtor ，非持久
	zend_llist_init(&CG(open_files), sizeof(zend_file_handle), (void (*)(void *)) zend_file_handle_dtor, 0);
} /* }}} */

// ing4, 销毁文件链表，关闭所有文件
void zend_stream_shutdown(void) /* {{{ */
{
	zend_llist_destroy(&CG(open_files));
} /* }}} */
