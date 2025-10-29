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
   | Authors: Andi Gutmans <andi@php.net>                                 |
   |          Zeev Suraski <zeev@php.net>                                 |
   +----------------------------------------------------------------------+
*/

#include "zend_extensions.h"
#include "zend_system_id.h"

ZEND_API zend_llist zend_extensions;
ZEND_API uint32_t zend_extension_flags = 0;
ZEND_API int zend_op_array_extension_handles = 0;
static int last_resource_number;

// ing2, 载入扩展
zend_result zend_load_extension(const char *path)
{
// 如果支持扩展
#if ZEND_EXTENSIONS_SUPPORT
	// void * 或 HMODULE（window环境）
	DL_HANDLE handle;
	// dlopen 或 LoadLibrary
	handle = DL_LOAD(path);
	// 如果载入失败
	if (!handle) {
// windows 操作系统
#ifndef ZEND_WIN32
		// 报错，载入失败
		fprintf(stderr, "Failed loading %s:  %s\n", path, DL_ERROR());
// 其他操作系统 
#else
		// 报错，载入失败
		fprintf(stderr, "Failed loading %s\n", path);
		/* See http://support.microsoft.com/kb/190351 */
		fflush(stderr);
#endif
		return FAILURE;
	}
// window 操作系统的附加操作
#ifdef ZEND_WIN32
	char *err;
	// 如果不兼容
	if (!php_win32_image_compatible(handle, &err)) {
		// 报错并返回false
		zend_error(E_CORE_WARNING, err);
		return FAILURE;
	}
#endif
	// 载入扩展
	return zend_load_extension_handle(handle, path);
// 如果不支持扩展
#else
	// 报错，不支持扩展
	fprintf(stderr, "Extensions are not supported on this platform.\n");
/* See http://support.microsoft.com/kb/190351 */
#ifdef ZEND_WIN32
	fflush(stderr);
#endif
	// 返回失败
	return FAILURE;
#endif
}

// ing1, 载入扩展
zend_result zend_load_extension_handle(DL_HANDLE handle, const char *path)
{
// 如果支持扩展
#if ZEND_EXTENSIONS_SUPPORT
	zend_extension *new_extension;
	zend_extension_version_info *extension_version_info;

	extension_version_info = (zend_extension_version_info *) DL_FETCH_SYMBOL(handle, "extension_version_info");
	if (!extension_version_info) {
		extension_version_info = (zend_extension_version_info *) DL_FETCH_SYMBOL(handle, "_extension_version_info");
	}
	new_extension = (zend_extension *) DL_FETCH_SYMBOL(handle, "zend_extension_entry");
	if (!new_extension) {
		new_extension = (zend_extension *) DL_FETCH_SYMBOL(handle, "_zend_extension_entry");
	}
	if (!extension_version_info || !new_extension) {
		fprintf(stderr, "%s doesn't appear to be a valid Zend extension\n", path);
/* See http://support.microsoft.com/kb/190351 */
#ifdef ZEND_WIN32
		fflush(stderr);
#endif
		DL_UNLOAD(handle);
		return FAILURE;
	}

	/* allow extension to proclaim compatibility with any Zend version */
	if (extension_version_info->zend_extension_api_no != ZEND_EXTENSION_API_NO &&(!new_extension->api_no_check || new_extension->api_no_check(ZEND_EXTENSION_API_NO) != SUCCESS)) {
		if (extension_version_info->zend_extension_api_no > ZEND_EXTENSION_API_NO) {
			fprintf(stderr, "%s requires Zend Engine API version %d.\n"
					"The Zend Engine API version %d which is installed, is outdated.\n\n",
					new_extension->name,
					extension_version_info->zend_extension_api_no,
					ZEND_EXTENSION_API_NO);
/* See http://support.microsoft.com/kb/190351 */
#ifdef ZEND_WIN32
			fflush(stderr);
#endif
			DL_UNLOAD(handle);
			return FAILURE;
		} else if (extension_version_info->zend_extension_api_no < ZEND_EXTENSION_API_NO) {
			fprintf(stderr, "%s requires Zend Engine API version %d.\n"
					"The Zend Engine API version %d which is installed, is newer.\n"
					"Contact %s at %s for a later version of %s.\n\n",
					new_extension->name,
					extension_version_info->zend_extension_api_no,
					ZEND_EXTENSION_API_NO,
					new_extension->author,
					new_extension->URL,
					new_extension->name);
/* See http://support.microsoft.com/kb/190351 */
#ifdef ZEND_WIN32
			fflush(stderr);
#endif
			DL_UNLOAD(handle);
			return FAILURE;
		}
	} else if (strcmp(ZEND_EXTENSION_BUILD_ID, extension_version_info->build_id) &&
	           (!new_extension->build_id_check || new_extension->build_id_check(ZEND_EXTENSION_BUILD_ID) != SUCCESS)) {
		fprintf(stderr, "Cannot load %s - it was built with configuration %s, whereas running engine is %s\n",
					new_extension->name, extension_version_info->build_id, ZEND_EXTENSION_BUILD_ID);
/* See http://support.microsoft.com/kb/190351 */
#ifdef ZEND_WIN32
		fflush(stderr);
#endif
		DL_UNLOAD(handle);
		return FAILURE;
	} else if (zend_get_extension(new_extension->name)) {
		fprintf(stderr, "Cannot load %s - it was already loaded\n", new_extension->name);
/* See http://support.microsoft.com/kb/190351 */
#ifdef ZEND_WIN32
		fflush(stderr);
#endif
		DL_UNLOAD(handle);
		return FAILURE;
	}

	zend_register_extension(new_extension, handle);
	return SUCCESS;
// 如果不支持扩展
#else
	// 报错，不支持扩展
	fprintf(stderr, "Extensions are not supported on this platform.\n");
/* See http://support.microsoft.com/kb/190351 */
#ifdef ZEND_WIN32
	fflush(stderr);
#endif
	return FAILURE;
#endif
}

// ing4, 注册一个扩展
void zend_register_extension(zend_extension *new_extension, DL_HANDLE handle)
{
// 支持扩展才有用
#if ZEND_EXTENSIONS_SUPPORT
	zend_extension extension;

	extension = *new_extension;
	// handle 是模块编号 
	extension.handle = handle;
	// 分发消息 ？
	zend_extension_dispatch_message(ZEND_EXTMSG_NEW_EXTENSION, &extension);
	// 添加到扩展列表里
	zend_llist_add_element(&zend_extensions, &extension);
	// 如果有 操作码构造器，添加标记
	if (extension.op_array_ctor) {
		zend_extension_flags |= ZEND_EXTENSIONS_HAVE_OP_ARRAY_CTOR;
	}
	// 如果有 操作码销毁器，添加标记
	if (extension.op_array_dtor) {
		zend_extension_flags |= ZEND_EXTENSIONS_HAVE_OP_ARRAY_DTOR;
	}
	// 如果有 操作码处理器，添加标记
	if (extension.op_array_handler) {
		zend_extension_flags |= ZEND_EXTENSIONS_HAVE_OP_ARRAY_HANDLER;
	}
	// 如果 需要操作码 持久计算，添加标记
	if (extension.op_array_persist_calc) {
		zend_extension_flags |= ZEND_EXTENSIONS_HAVE_OP_ARRAY_PERSIST_CALC;
	}
	// 如果 需要 持久化操作码 ，添加标记
	if (extension.op_array_persist) {
		zend_extension_flags |= ZEND_EXTENSIONS_HAVE_OP_ARRAY_PERSIST;
	}
	/*fprintf(stderr, "Loaded %s, version %s\n", extension.name, extension.version);*/
#endif
}


// ing4, 关闭一个扩展
static void zend_extension_shutdown(zend_extension *extension)
{
// 支持扩展的情况下才有用
#if ZEND_EXTENSIONS_SUPPORT
	// 如果有关闭方法，调用关闭方法
	if (extension->shutdown) {
		extension->shutdown(extension);
	}
#endif
}

/* int return due to zend linked list API */
// ing4, 启动一个扩展
static int zend_extension_startup(zend_extension *extension)
{
// 在支持扩展的情况下才有效
#if ZEND_EXTENSIONS_SUPPORT
	// 如果扩展有 startup 方法
	if (extension->startup) {
		// 执行这个方法，成功返回true
		if (extension->startup(extension)!=SUCCESS) {
			return 1;
		}
		// 不成功，添加版本信息
		zend_append_version_info(extension);
	}
#endif
	return 0;
}

// ing4, 初始化 扩展指针列表
void zend_startup_extensions_mechanism(void)
{
	/* Startup extensions mechanism */
	// 初始化扩展指针列表
	zend_llist_init(&zend_extensions, sizeof(zend_extension), (void (*)(void *)) zend_extension_dtor, 1);
	// 扩展处理器数量 ，0个
	zend_op_array_extension_handles = 0;
	// 最后一个资源编号 0
	last_resource_number = 0;
}

// ing4, 启动扩展
void zend_startup_extensions(void)
{
	// 对每个元素执行func，如果成功，删除这个元素
	zend_llist_apply_with_del(&zend_extensions, (int (*)(void *)) zend_extension_startup);
}

// ing4, 关闭扩展
void zend_shutdown_extensions(void)
{
	// 对每个元素执行func
	zend_llist_apply(&zend_extensions, (llist_apply_func_t) zend_extension_shutdown);
	// 销毁 zend_extensions 扩展列表
	zend_llist_destroy(&zend_extensions);
}

// ing3, 销毁一个扩展
void zend_extension_dtor(zend_extension *extension)
{
// 支持扩展，并且不是调试模式，才有用
#if ZEND_EXTENSIONS_SUPPORT && !ZEND_DEBUG
	// 如果模块编号 并且 没有 阻止卸载模块的选项
	if (extension->handle && !getenv("ZEND_DONT_UNLOAD_MODULES")) {
		// 卸载模块: dlclose 或 FreeLibrary。 handle是编号 
		DL_UNLOAD(extension->handle);
	}
#endif
}

// ing4, 给扩展发送消息
static void zend_extension_message_dispatcher(const zend_extension *extension, int num_args, va_list args)
{
	int message;
	void *arg;
	// 如果此扩展没有消息处理器或 参数数量不是2个
	if (!extension->message_handler || num_args!=2) {
		// 返回
		return;
	}
	// 获取消息码（第一个参数， 整数参数）
	message = va_arg(args, int);
	// 获取消息内容（第二个参数，指针）
	arg = va_arg(args, void *);
	// 调用扩展的消息处理器
	extension->message_handler(message, arg);
}


// ing4, 给每个扩展发送消息，带传入参数
ZEND_API void zend_extension_dispatch_message(int message, void *arg)
{
	// 给每个扩展调用 zend_extension_message_dispatcher 方法
	zend_llist_apply_with_arguments(&zend_extensions, (llist_apply_with_args_func_t) zend_extension_message_dispatcher, 2, message, arg);
}

// ing4, 获取 新的全局资源编号
ZEND_API int zend_get_resource_handle(const char *module_name)
{
	// 如果资源数没有超限
	if (last_resource_number<ZEND_MAX_RESERVED_RESOURCES) {
		// 增加操作系统熵,在生成 zend_system_id 的md5上下文中添加内容
		zend_add_system_entropy(module_name, "zend_get_resource_handle", &last_resource_number, sizeof(int));
		// 资源数量 +1
		return last_resource_number++;
	// 资源数量超限
	} else {
		// 返回 -1
		return -1;
	}
}

// ing4, 获取 新的 操作码扩展处理器 序号
ZEND_API int zend_get_op_array_extension_handle(const char *module_name)
{
	// 操作码扩展处理器 +1
	int handle = zend_op_array_extension_handles++;
	// 增加操作系统熵,在生成 zend_system_id 的md5上下文中添加内容
	zend_add_system_entropy(module_name, "zend_get_op_array_extension_handle", &zend_op_array_extension_handles, sizeof(int));
	// 返回新序号
	return handle;
}

// ing3, 传入模块名和处理器数量，增加处理器数量。返原有处理器数量。
ZEND_API int zend_get_op_array_extension_handles(const char *module_name, int handles)
{
	// 现有处理器数
	int handle = zend_op_array_extension_handles;
	// 处理器计数增加
	zend_op_array_extension_handles += handles;
	// 增加操作系统熵,在生成 zend_system_id 的md5上下文中添加内容
	zend_add_system_entropy(module_name, "zend_get_op_array_extension_handle", &zend_op_array_extension_handles, sizeof(int));
	// 返回 增加前，佣有处理器数
	return handle;
}

// ing4, 返回 n 个void * 指针大小
ZEND_API size_t zend_internal_run_time_cache_reserved_size(void) {
	return zend_op_array_extension_handles * sizeof(void *);
}

// ing3, 给所有内置函数添加运行时缓存（指针表）
ZEND_API void zend_init_internal_run_time_cache(void) {
	
	// 返回 n 个void * 指针大小
	size_t rt_size = zend_internal_run_time_cache_reserved_size();
	// 如果大小有效
	if (rt_size) {
		// 编译时 函数数量
		size_t functions = zend_hash_num_elements(CG(function_table));
		//
		zend_class_entry *ce;
		// 遍历类表
		ZEND_HASH_MAP_FOREACH_PTR(CG(class_table), ce) {
			// 把类方法计入 函数数量
			functions += zend_hash_num_elements(&ce->function_table);
		} ZEND_HASH_FOREACH_END();

		// 分配内存，创建指针列表
		char *ptr = zend_arena_calloc(&CG(arena), functions, rt_size);
		//
		zend_internal_function *zif;
		// 遍历 编译时 函数表
		ZEND_HASH_MAP_FOREACH_PTR(CG(function_table), zif) {
			// 如果不是用户函数 并且 没有 运行时缓存
			if (!ZEND_USER_CODE(zif->type) && ZEND_MAP_PTR_GET(zif->run_time_cache) == NULL)
			{
				// 添加新建的运行时缓存
				ZEND_MAP_PTR_SET(zif->run_time_cache, (void *)ptr);
				// 指针到下一段
				ptr += rt_size;
			}
		} ZEND_HASH_FOREACH_END();
		// 遍历编译时类表
		ZEND_HASH_MAP_FOREACH_PTR(CG(class_table), ce) {
			// 遍历类方法
			ZEND_HASH_MAP_FOREACH_PTR(&ce->function_table, zif) {
				// 如果不是用户函数 并且 没有 运行时缓存
				if (!ZEND_USER_CODE(zif->type) && ZEND_MAP_PTR_GET(zif->run_time_cache) == NULL)
				{
					// 添加新建的运行时缓存
					ZEND_MAP_PTR_SET(zif->run_time_cache, (void *)ptr);
					// 指针到下一段
					ptr += rt_size;
				}
			} ZEND_HASH_FOREACH_END();
		} ZEND_HASH_FOREACH_END();
	}
}

// ing4, 获取指定名称的扩展
ZEND_API zend_extension *zend_get_extension(const char *extension_name)
{
	zend_llist_element *element;
	// 遍历列表
	for (element = zend_extensions.head; element; element = element->next) {
		// 取得此扩展实例
		zend_extension *extension = (zend_extension *) element->data;
		// 如果传入的名称和 name 匹配
		if (!strcmp(extension->name, extension_name)) {
			// 返回此扩展
			return extension;
		}
	}
	// 查找失败，返回null
	return NULL;
}

// 扩展持久化数据
typedef struct _zend_extension_persist_data {
	// 操作码
	zend_op_array *op_array;
	// 操作码数量
	size_t         size;
	// 
	char          *mem;
} zend_extension_persist_data;

// ing3, 操作码持久化计算器
static void zend_extension_op_array_persist_calc_handler(zend_extension *extension, zend_extension_persist_data *data)
{
	// 调用扩展自己的操作码持久化计算器
	if (extension->op_array_persist_calc) {
		// 数据大小增加
		data->size += extension->op_array_persist_calc(data->op_array);
	}
}

// ing3, 持久化操作码处理器
static void zend_extension_op_array_persist_handler(zend_extension *extension, zend_extension_persist_data *data)
{
	// 如果此扩展 有持久化操作码方法
	if (extension->op_array_persist) {
		// 调用扩展息的 有持久化操作码方法，返回数量
		size_t size = extension->op_array_persist(data->op_array, data->mem);
		// 如果有数量
		if (size) {
			// 内存向右移 size 个byte
			data->mem = (void*)((char*)data->mem + size);
			// 增加大小
			data->size += size;
		}
	}
}

// ing2, 操作码持久化计算？
ZEND_API size_t zend_extensions_op_array_persist_calc(zend_op_array *op_array)
{
	// 如果需要计算持久化操作码
	if (zend_extension_flags & ZEND_EXTENSIONS_HAVE_OP_ARRAY_PERSIST_CALC) {
		zend_extension_persist_data data;

		data.op_array = op_array;
		data.size = 0;
		data.mem  = NULL;
		// 对每个扩展调用 zend_extension_op_array_persist_calc_handler
		zend_llist_apply_with_argument(&zend_extensions, (llist_apply_with_arg_func_t) zend_extension_op_array_persist_calc_handler, &data);
		// 返回数据大小
		return data.size;
	}
	return 0;
}

// ing3, 持久化操作码
ZEND_API size_t zend_extensions_op_array_persist(zend_op_array *op_array, void *mem)
{
	// 如有 持久化操作码 标记
	if (zend_extension_flags & ZEND_EXTENSIONS_HAVE_OP_ARRAY_PERSIST) {
		zend_extension_persist_data data;
		// 操作码关联到数据
		data.op_array = op_array;
		// 大小是0
		data.size = 0;
		// 内存关联到数据
		data.mem  = mem;
		// 对每个扩展调用 zend_extension_op_array_persist_handler， data作为附加参数
		zend_llist_apply_with_argument(&zend_extensions, (llist_apply_with_arg_func_t) zend_extension_op_array_persist_handler, &data);
		// 返回数量 
		return data.size;
	}
	// 没有持久化操作码标记，返回0
	return 0;
}
