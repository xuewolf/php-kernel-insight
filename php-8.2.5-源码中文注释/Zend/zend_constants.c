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

#include "zend.h"
#include "zend_constants.h"
#include "zend_exceptions.h"
#include "zend_execute.h"
#include "zend_variables.h"
#include "zend_operators.h"
#include "zend_globals.h"
#include "zend_API.h"
#include "zend_constants_arginfo.h"

// 保护：递归引用自身的类常量
/* Protection from recursive self-referencing class constants */
#define IS_CONSTANT_VISITED_MARK    0x80
// clear, 检验是否有 IS_CONSTANT_VISITED_MARK 标记
#define IS_CONSTANT_VISITED(zv)     (Z_CONSTANT_FLAGS_P(zv) & IS_CONSTANT_VISITED_MARK)
// clear, 添加 IS_CONSTANT_VISITED_MARK 标记
#define MARK_CONSTANT_VISITED(zv)   Z_CONSTANT_FLAGS_P(zv) |= IS_CONSTANT_VISITED_MARK
// clear, 删除 IS_CONSTANT_VISITED_MARK 标记
#define RESET_CONSTANT_VISITED(zv)  Z_CONSTANT_FLAGS_P(zv) &= ~IS_CONSTANT_VISITED_MARK

// 使用特殊常量 null,true,false
/* Use for special null/true/false constants. */
static zend_constant *null_const, *true_const, *false_const;

// ing3, 释放常量
void free_zend_constant(zval *zv)
{
	zend_constant *c = Z_PTR_P(zv);
	// 如果有持久标记
	if (!(ZEND_CONSTANT_FLAGS(c) & CONST_PERSISTENT)) {
		// 销毁 常量值
		zval_ptr_dtor_nogc(&c->value);
		// 如果有常量名
		if (c->name) {
			// 释放常量名
			zend_string_release_ex(c->name, 0);
		}
		// 释放常量实例
		efree(c);
	// 非持久
	} else {
		// 销毁内置变量（内置字串）
		zval_internal_ptr_dtor(&c->value);
		// 如果有常量名
		if (c->name) {
			// 释放常量名
			zend_string_release_ex(c->name, 1);
		}
		// 释放常量实例
		free(c);
	}
}

// 如果要求线程安全
#ifdef ZTS
// suspend
static void copy_zend_constant(zval *zv)
{
	zend_constant *c = Z_PTR_P(zv);

	ZEND_ASSERT(ZEND_CONSTANT_FLAGS(c) & CONST_PERSISTENT);
	Z_PTR_P(zv) = pemalloc(sizeof(zend_constant), 1);
	memcpy(Z_PTR_P(zv), c, sizeof(zend_constant));

	c = Z_PTR_P(zv);
	c->name = zend_string_copy(c->name);
	if (Z_TYPE(c->value) == IS_STRING) {
		Z_STR(c->value) = zend_string_dup(Z_STR(c->value), 1);
	}
}

// ing4, 复制整个常量哈希表
void zend_copy_constants(HashTable *target, HashTable *source)
{
	zend_hash_copy(target, source, copy_zend_constant);
}
#endif


// ing4, 删除模块常量。在 zend_hash_apply_with_argument 中调用，当模块相符时，返回删除常量指令。
static int clean_module_constant(zval *el, void *arg)
{
	zend_constant *c = (zend_constant *)Z_PTR_P(el);
	int module_number = *(int *)arg;

	// 如果模块编号相符
	if (ZEND_CONSTANT_MODULE_NUMBER(c) == module_number) {
		// 删除常量
		return ZEND_HASH_APPLY_REMOVE;
	} else {
		// 保留常量
		return ZEND_HASH_APPLY_KEEP;
	}
}


// ing4, 清除指定模块的常量
void clean_module_constants(int module_number)
{
	// 对每个元素调用 clean_module_constant 函数，按返回的要求删除元素
	zend_hash_apply_with_argument(EG(zend_constants), clean_module_constant, (void *) &module_number);
}

// ing4, 开启常量功能
void zend_startup_constants(void)
{
	// 创建哈希表
	EG(zend_constants) = (HashTable *) malloc(sizeof(HashTable));
	// 初始化哈希表
	zend_hash_init(EG(zend_constants), 128, NULL, ZEND_CONSTANT_DTOR, 1);
}


// ing3, 注册标准常量
void zend_register_standard_constants(void)
{
	// 注册错误类型 E_ALL (E_ERROR | E_WARNING | E_PARSE | E_NOTICE | E_CORE_ERROR | E_CORE_WARNING | E_COMPILE_ERROR | E_COMPILE_WARNING | E_USER_ERROR | E_USER_WARNING | E_USER_NOTICE | E_RECOVERABLE_ERROR | E_DEPRECATED | E_USER_DEPRECATED | E_STRICT)
	register_zend_constants_symbols(0);

	// 1
	REGISTER_MAIN_LONG_CONSTANT("DEBUG_BACKTRACE_PROVIDE_OBJECT", DEBUG_BACKTRACE_PROVIDE_OBJECT, CONST_PERSISTENT | CONST_CS);
	// 2
	REGISTER_MAIN_LONG_CONSTANT("DEBUG_BACKTRACE_IGNORE_ARGS", DEBUG_BACKTRACE_IGNORE_ARGS, CONST_PERSISTENT | CONST_CS);
	// 是否线程安全
	REGISTER_MAIN_BOOL_CONSTANT("ZEND_THREAD_SAFE", ZTS_V, CONST_PERSISTENT | CONST_CS);
	// 是否是调试模式
	REGISTER_MAIN_BOOL_CONSTANT("ZEND_DEBUG_BUILD", ZEND_DEBUG, CONST_PERSISTENT | CONST_CS);

	// 特殊常量 true/false/null
	/* Special constants true/false/null.  */
	REGISTER_MAIN_BOOL_CONSTANT("TRUE", 1, CONST_PERSISTENT);
	REGISTER_MAIN_BOOL_CONSTANT("FALSE", 0, CONST_PERSISTENT);
	REGISTER_MAIN_NULL_CONSTANT("NULL", CONST_PERSISTENT);

	// 让3个特殊常量可以全局访问
	true_const = zend_hash_str_find_ptr(EG(zend_constants), "TRUE", sizeof("TRUE")-1);
	false_const = zend_hash_str_find_ptr(EG(zend_constants), "FALSE", sizeof("FALSE")-1);
	null_const = zend_hash_str_find_ptr(EG(zend_constants), "NULL", sizeof("NULL")-1);
}

// ing4, 销毁所有常量
void zend_shutdown_constants(void)
{
	// 销毁哈希表里的数据 
	zend_hash_destroy(EG(zend_constants));
	// 释放哈希表
	free(EG(zend_constants));
}

// ing4, 注册值为null的常量
ZEND_API void zend_register_null_constant(const char *name, size_t name_len, int flags, int module_number)
{
	zend_constant c;
	// 值为null
	ZVAL_NULL(&c.value);
	// 添加标记
	ZEND_CONSTANT_SET_FLAGS(&c, flags, module_number);
	// 给常量名创建保留字
	c.name = zend_string_init_interned(name, name_len, flags & CONST_PERSISTENT);
	// 注册常量
	zend_register_constant(&c);
}

// ing4, 注册布尔型的常量
ZEND_API void zend_register_bool_constant(const char *name, size_t name_len, bool bval, int flags, int module_number)
{
	zend_constant c;
	// 添加布尔值
	ZVAL_BOOL(&c.value, bval);
	// 添加标记
	ZEND_CONSTANT_SET_FLAGS(&c, flags, module_number);
	// 给常量名创建保留字
	c.name = zend_string_init_interned(name, name_len, flags & CONST_PERSISTENT);
	// 注册常量
	zend_register_constant(&c);
}

// ing4, 注册整数型常量
ZEND_API void zend_register_long_constant(const char *name, size_t name_len, zend_long lval, int flags, int module_number)
{
	zend_constant c;
	// 添加整数值
	ZVAL_LONG(&c.value, lval);
	// 添加标记
	ZEND_CONSTANT_SET_FLAGS(&c, flags, module_number);
	// 给常量名创建保留字
	c.name = zend_string_init_interned(name, name_len, flags & CONST_PERSISTENT);
	// 注册常量
	zend_register_constant(&c);
}

// ing4, 注册型常量
ZEND_API void zend_register_double_constant(const char *name, size_t name_len, double dval, int flags, int module_number)
{
	zend_constant c;
	// 添加小数值
	ZVAL_DOUBLE(&c.value, dval);
	// 添加标记
	ZEND_CONSTANT_SET_FLAGS(&c, flags, module_number);
	// 给常量名创建保留字
	c.name = zend_string_init_interned(name, name_len, flags & CONST_PERSISTENT);
	// 注册常量
	zend_register_constant(&c);
}


// ing4, 注册字串型常量，传入长度
ZEND_API void zend_register_stringl_constant(const char *name, size_t name_len, const char *strval, size_t strlen, int flags, int module_number)
{
	zend_constant c;
	// 把字串值创建成保留字，然后添加给常量
	ZVAL_STR(&c.value, zend_string_init_interned(strval, strlen, flags & CONST_PERSISTENT));
	// 添加标记
	ZEND_CONSTANT_SET_FLAGS(&c, flags, module_number);
	// 给常量名创建保留字
	c.name = zend_string_init_interned(name, name_len, flags & CONST_PERSISTENT);
	// 注册常量
	zend_register_constant(&c);
}

// ing4, 注册字串型常量，不传入长度
ZEND_API void zend_register_string_constant(const char *name, size_t name_len, const char *strval, int flags, int module_number)
{
	zend_register_stringl_constant(name, name_len, strval, strlen(strval), flags, module_number);
}

// ing3, 获取常量 __COMPILER_HALT_OFFSET__ 的值，常量名后要拼接 执行文件名
static zend_constant *zend_get_halt_offset_constant(const char *name, size_t name_len)
{
	zend_constant *c;
	static const char haltoff[] = "__COMPILER_HALT_OFFSET__";

	// 如果没有执行上下文数据 
	if (!EG(current_execute_data)) {
		// 返回null
		return NULL;
	// 有执行上下文，并且常量名是 __COMPILER_HALT_OFFSET__
	} else if (name_len == sizeof("__COMPILER_HALT_OFFSET__")-1 &&
	          !memcmp(name, "__COMPILER_HALT_OFFSET__", sizeof("__COMPILER_HALT_OFFSET__")-1)) {
		const char *cfilename;
		zend_string *haltname;
		size_t clen;
		// 取得执行文件
		cfilename = zend_get_executed_filename();
		// 文件名长度
		clen = strlen(cfilename);
		// 检验 __COMPILER_HALT_OFFSET__
		/* check for __COMPILER_HALT_OFFSET__ */
		// 拼接常量名，可能存在多个 __COMPILER_HALT_OFFSET__，要和执行文件名连接起来防止冲突
			// 最后一个参数是，是否创建保留字
		haltname = zend_mangle_property_name(haltoff,
			sizeof("__COMPILER_HALT_OFFSET__") - 1, cfilename, clen, 0);
		// 从常量哈希表中获取  
		c = zend_hash_find_ptr(EG(zend_constants), haltname);
		// 释放常量名
		zend_string_efree(haltname);
		// 返回常量
		return c;
	// 没有执行上下文或 常量名不是 ...
	} else {
		return NULL;
	}
}

// ing4, 获取特殊常量的值 true,false,null 三个
ZEND_API zend_constant *_zend_get_special_const(const char *name, size_t len) /* {{{ */
{
	// 为什么不转成小写比较呢,这样性能更好吧
	if (len == 4) {
		// null
		if ((name[0] == 'n' || name[0] == 'N') &&
			(name[1] == 'u' || name[1] == 'U') &&
			(name[2] == 'l' || name[2] == 'L') &&
			(name[3] == 'l' || name[3] == 'L')
		) {
			return null_const;
		}
		// true
		if ((name[0] == 't' || name[0] == 'T') &&
			(name[1] == 'r' || name[1] == 'R') &&
			(name[2] == 'u' || name[2] == 'U') &&
			(name[3] == 'e' || name[3] == 'E')
		) {
			return true_const;
		}
	} else {
		// false
		if ((name[0] == 'f' || name[0] == 'F') &&
			(name[1] == 'a' || name[1] == 'A') &&
			(name[2] == 'l' || name[2] == 'L') &&
			(name[3] == 's' || name[3] == 'S') &&
			(name[4] == 'e' || name[4] == 'E')
		) {
			return false_const;
		}
	}
	return NULL;
}
/* }}} */

// ing3, 检验类常量的访问权限 
ZEND_API bool zend_verify_const_access(zend_class_constant *c, zend_class_entry *scope) /* {{{ */
{
	// public 返回 true
	if (ZEND_CLASS_CONST_FLAGS(c) & ZEND_ACC_PUBLIC) {
		return 1;
	// private 
	} else if (ZEND_CLASS_CONST_FLAGS(c) & ZEND_ACC_PRIVATE) {
		// 当前作用域和类入口相同时才可以访问
		return (c->ce == scope);
	// 其他情况
	} else {
		// 只能是 protected
		ZEND_ASSERT(ZEND_CLASS_CONST_FLAGS(c) & ZEND_ACC_PROTECTED);
		// 返回，验证 scope 是否继承自（或等于）ce
		return zend_check_protected(c->ce, scope);
	}
}
/* }}} */

// ing4, 根据常量名(char * )获取常量
static zend_constant *zend_get_constant_str_impl(const char *name, size_t name_len)
{
	// 先从哈希表里获取
	zend_constant *c = zend_hash_str_find_ptr(EG(zend_constants), name, name_len);
	if (c) {
		return c;
	}
	// 如果没有，获取常量 __COMPILER_HALT_OFFSET__ 的值，常量名后要拼接 执行文件名
	c = zend_get_halt_offset_constant(name, name_len);
	if (c) {
		return c;
	}
	// 如果还没有，获取特殊常量，true,false,null 三个
	return zend_get_special_const(name, name_len);
}

// ing4, 根据常量名(char * )和长度获取常量
ZEND_API zval *zend_get_constant_str(const char *name, size_t name_len)
{
	zend_constant *c = zend_get_constant_str_impl(name, name_len);
	if (c) {
		return &c->value;
	}
	return NULL;
}

// ing4, 根据常量名（zend_string）获取常量值
static zend_constant *zend_get_constant_impl(zend_string *name)
{
	// 从 EG(zend_constants) 哈希表里取得指针，然后转成常量
	zend_constant *c = zend_hash_find_ptr(EG(zend_constants), name);
	// 如果获取成功，返回
	if (c) {
		return c;
	}
	
	// 如果没有，获取常量 __COMPILER_HALT_OFFSET__ 的值，常量名后要拼接 执行文件名
	c = zend_get_halt_offset_constant(ZSTR_VAL(name), ZSTR_LEN(name));
	if (c) {
		return c;
	}
	
	// 如果没有，获取特殊常量，true,false,null 三个
	return zend_get_special_const(ZSTR_VAL(name), ZSTR_LEN(name));
}

// ing4, 根据常量名（zend_string）获取常量值
ZEND_API zval *zend_get_constant(zend_string *name)
{
	// 
	zend_constant *c = zend_get_constant_impl(name);
	if (c) {
		return &c->value;
	}
	// 失败返回null
	return NULL;
}

// ing3, 获取类常量
ZEND_API zval *zend_get_class_constant_ex(zend_string *class_name, zend_string *constant_name, zend_class_entry *scope, uint32_t flags)
{
	zend_class_entry *ce = NULL;
	zend_class_constant *c = NULL;
	zval *ret_constant = NULL;
	// 第1步，先计算作用域，比较复杂
	// 如果有缓存的类入口
	if (ZSTR_HAS_CE_CACHE(class_name)) {
		// 获取缓存中的类入口
		ce = ZSTR_GET_CE_CACHE(class_name);
		// 如果获取失败
		if (!ce) {
			// 通过类名重新获取
			ce = zend_fetch_class(class_name, flags);
		}
	// 如果是 self
	} else if (zend_string_equals_literal_ci(class_name, "self")) {
		// 如果没有作用域
		if (UNEXPECTED(!scope)) {
			// 报错：没有类作用域时无法访问 self
			zend_throw_error(NULL, "Cannot access \"self\" when no class scope is active");
			goto failure;
		}
		// 当前域作为类入口
		ce = scope;
	// 如果是 parent
	} else if (zend_string_equals_literal_ci(class_name, "parent")) {
		// 如果当前没有作用域
		if (UNEXPECTED(!scope)) {
			// 报错：没有类作用域时无法访问 parent
			zend_throw_error(NULL, "Cannot access \"parent\" when no class scope is active");
			goto failure;
		// 如果当前作用域没有 父类
		} else if (UNEXPECTED(!scope->parent)) {
			// 报错：当前作用域没有父类时无法访问 parent
			zend_throw_error(NULL, "Cannot access \"parent\" when current class scope has no parent");
			goto failure;
		// 父类作为类入口
		} else {
			ce = scope->parent;
		}
	// 如果是 static
	} else if (zend_string_equals_literal_ci(class_name, "static")) {
		// 取得正在调用的类入口
		ce = zend_get_called_scope(EG(current_execute_data));
		// 如果获取失败
		if (UNEXPECTED(!ce)) {
			// 报错：没有类作用域时，不可以访问 static
			zend_throw_error(NULL, "Cannot access \"static\" when no class scope is active");
			goto failure;
		}
	// 其他情况
	} else {
		// 通过类名获取类入口
		ce = zend_fetch_class(class_name, flags);
	}
	
	// 第2步，如果有类入口，按类常量处理
	if (ce) {
		// 在常量表里查找要求的常量
		c = zend_hash_find_ptr(CE_CONSTANTS_TABLE(ce), constant_name);
		// 如果查找失败
		if (c == NULL) {
			// 如果没有要求 静默
			if ((flags & ZEND_FETCH_CLASS_SILENT) == 0) {
				// 报错 未定义的常量 ****
				zend_throw_error(NULL, "Undefined constant %s::%s", ZSTR_VAL(class_name), ZSTR_VAL(constant_name));
				goto failure;
			}
			// 返回常量为null
			ret_constant = NULL;
		// 如果查找成功
		} else {
			// 检验是否是访问权限, 如果没有
			if (!zend_verify_const_access(c, scope)) {
				// 如果没有要求静默
				if ((flags & ZEND_FETCH_CLASS_SILENT) == 0) {
					// 报错 无法访问常量 **
					zend_throw_error(NULL, "Cannot access %s constant %s::%s", zend_visibility_string(ZEND_CLASS_CONST_FLAGS(c)), ZSTR_VAL(class_name), ZSTR_VAL(constant_name));
				}
				goto failure;
			}

			// 如果是在trait里
			if (UNEXPECTED(ce->ce_flags & ZEND_ACC_TRAIT)) {
				// 阻止直接 访问 trait里的常量，它们和 \defined() or \constant() 定义的常量不同。
				/** Prevent accessing trait constants directly on cases like \defined() or \constant(), etc. */
				// 如果没有要求静默
				if ((flags & ZEND_FETCH_CLASS_SILENT) == 0) {
					// 报错 无法访问常量 ** （测试过）
					zend_throw_error(NULL, "Cannot access trait constant %s::%s directly", ZSTR_VAL(class_name), ZSTR_VAL(constant_name));
				}
				goto failure;
			}

			ret_constant = &c->value;
		}
	}

	// 第3步，最麻烦的是这里。普通常量都走这里。
	// 如果返回的常量来自常量 表达式 语句
	if (ret_constant && Z_TYPE_P(ret_constant) == IS_CONSTANT_AST) {
		zend_result ret;
		// 如果有 IS_CONSTANT_VISITED_MARK 标记
		if (IS_CONSTANT_VISITED(ret_constant)) {
			// 报错，不可以重复声名 自引用常量
			zend_throw_error(NULL, "Cannot declare self-referencing constant %s::%s", ZSTR_VAL(class_name), ZSTR_VAL(constant_name));
			ret_constant = NULL;
			goto failure;
		}
		// 添加标记 IS_CONSTANT_VISITED_MARK，这里相当于加了个锁
		MARK_CONSTANT_VISITED(ret_constant);
		
		// zval_update_constant_ex会再调用本函数 ，但返回类型不会总是表达式语句，所以这个地方不会经常进来。
		// 不会死循环吗？这个暂时无法调试，因为没有语法解析。
		// 更新常量
		ret = zval_update_constant_ex(ret_constant, c->ce);
		// 删除标记 IS_CONSTANT_VISITED_MARK
		RESET_CONSTANT_VISITED(ret_constant);

		// 如果结果不是成功
		if (UNEXPECTED(ret != SUCCESS)) {
			// 清空常量
			ret_constant = NULL;
			goto failure;
		}
	}
failure:
	return ret_constant;
}

// ing3, 获取常量值
ZEND_API zval *zend_get_constant_ex(zend_string *cname, zend_class_entry *scope, uint32_t flags)
{
	zend_constant *c;
	// 在名称中查找 冒号
	const char *colon;
	// 名称字串
	const char *name = ZSTR_VAL(cname);
	// 名称长度
	size_t name_len = ZSTR_LEN(cname);

	// 跳过开头的 \ 
	/* Skip leading \\ */
	if (name[0] == '\\') {
		name += 1;
		name_len -= 1;
		// 如果有命名空间，常量名无效
		cname = NULL;
	}

	// （处理类常量）查找名称中 冒号 的位置，如果能找到
	if ((colon = zend_memrchr(name, ':', name_len)) &&
		// 并且不在开头 并且是连续两个 : 
	    colon > name && (*(colon - 1) == ':')) {
		// 类名长度
		int class_name_len = colon - name - 1;
		// 常量名长度
		size_t const_name_len = name_len - class_name_len - 2;
		// 常量名
		zend_string *constant_name = zend_string_init(colon + 1, const_name_len, 0);
		// 类名
		zend_string *class_name = zend_string_init_interned(name, class_name_len, 0);
		
		// 取得类常量
		zval *ret_constant = zend_get_class_constant_ex(class_name, constant_name, scope, flags);
		
		// 释放类名
		zend_string_release_ex(class_name, 0);
		// 释放常量名
		zend_string_efree(constant_name);
		// 返回
		return ret_constant;
/*
		zend_class_entry *ce = NULL;
		zend_class_constant *c = NULL;
		zval *ret_constant = NULL;

		if (zend_string_equals_literal_ci(class_name, "self")) {
			if (UNEXPECTED(!scope)) {
				zend_throw_error(NULL, "Cannot access \"self\" when no class scope is active");
				goto failure;
			}
			ce = scope;
		} else if (zend_string_equals_literal_ci(class_name, "parent")) {
			if (UNEXPECTED(!scope)) {
				zend_throw_error(NULL, "Cannot access \"parent\" when no class scope is active");
				goto failure;
			} else if (UNEXPECTED(!scope->parent)) {
				zend_throw_error(NULL, "Cannot access \"parent\" when current class scope has no parent");
				goto failure;
			} else {
				ce = scope->parent;
			}
		} else if (zend_string_equals_literal_ci(class_name, "static")) {
			ce = zend_get_called_scope(EG(current_execute_data));
			if (UNEXPECTED(!ce)) {
				zend_throw_error(NULL, "Cannot access \"static\" when no class scope is active");
				goto failure;
			}
		} else {
			ce = zend_fetch_class(class_name, flags);
		}
		if (ce) {
			c = zend_hash_find_ptr(CE_CONSTANTS_TABLE(ce), constant_name);
			if (c == NULL) {
				if ((flags & ZEND_FETCH_CLASS_SILENT) == 0) {
					zend_throw_error(NULL, "Undefined constant %s::%s", ZSTR_VAL(class_name), ZSTR_VAL(constant_name));
					goto failure;
				}
				ret_constant = NULL;
			} else {
				if (!zend_verify_const_access(c, scope)) {
					if ((flags & ZEND_FETCH_CLASS_SILENT) == 0) {
						zend_throw_error(NULL, "Cannot access %s constant %s::%s", zend_visibility_string(ZEND_CLASS_CONST_FLAGS(c)), ZSTR_VAL(class_name), ZSTR_VAL(constant_name));
					}
					goto failure;
				}
				ret_constant = &c->value;
			}
		}

		if (ret_constant && Z_TYPE_P(ret_constant) == IS_CONSTANT_AST) {
			zend_result ret;

			if (IS_CONSTANT_VISITED(ret_constant)) {
				zend_throw_error(NULL, "Cannot declare self-referencing constant %s::%s", ZSTR_VAL(class_name), ZSTR_VAL(constant_name));
				ret_constant = NULL;
				goto failure;
			}

			MARK_CONSTANT_VISITED(ret_constant);
			ret = zval_update_constant_ex(ret_constant, c->ce);
			RESET_CONSTANT_VISITED(ret_constant);

			if (UNEXPECTED(ret != SUCCESS)) {
				ret_constant = NULL;
				goto failure;
			}
		}
failure:
		zend_string_release_ex(class_name, 0);
		zend_string_efree(constant_name);
		return ret_constant;
*/
	}
	
	// 非类常量
	/* non-class constant */
	// 如果有命名空间
	if ((colon = zend_memrchr(name, '\\', name_len)) != NULL) {
		// 处理混合常量名
		/* compound constant name */
		// 前缀长度
		int prefix_len = colon - name;
		// 常量名长度，总长度 - 前缀 -1
		size_t const_name_len = name_len - prefix_len - 1;
		// 常量名
		const char *constant_name = colon + 1;
		// 小写完整常量名：小写命名空间 + \ + 常量名 拼成的字串
		char *lcname;
		// 
		size_t lcname_len;
		// # define ALLOCA_FLAG(name) bool name;
		// 声名一个bool型变量，没有赋值
		ALLOCA_FLAG(use_heap)

		// 命名空间转小写
		/* Lowercase the namespace portion */
		// 总长度
		lcname_len = prefix_len + 1 + const_name_len;
		
		// 分配内存 do_alloca 与兼容性有关，zend_portability.h
		lcname = do_alloca(lcname_len + 1, use_heap);
		
		// 复制小写前缀名称
		zend_str_tolower_copy(lcname, name, prefix_len);
		// 添加 \\ 
		lcname[prefix_len] = '\\';
		// 复制常量名
		memcpy(lcname + prefix_len + 1, constant_name, const_name_len + 1);
		// 这样 lcname 就成了 ： 小写命名空间 + \ + 常量名 拼成的字串
		
		// 用常量名取得常量
		c = zend_hash_str_find_ptr(EG(zend_constants), lcname, lcname_len);
		// 释放小写名称
		free_alloca(lcname, use_heap);
		// 如果未获取到常量
		if (!c) {
			// 如果 带有相对路径的命名空间
			if (flags & IS_CONSTANT_UNQUALIFIED_IN_NAMESPACE) {
				// 名称需要在运行时计算， 需要在无命名空间的常量中查找一遍
				/* name requires runtime resolution, need to check non-namespaced name */
				// 查找常量
				c = zend_get_constant_str_impl(constant_name, const_name_len);
			}
		}
		
	// 如果没有命名空间
	} else {
		// 如果有常量名
		if (cname) {
			c = zend_get_constant_impl(cname);
		// 没有常量名，用字串常量名
		} else {
			c = zend_get_constant_str_impl(name, name_len);
		}
	}

	// 如果获取常量失败
	if (!c) {
		// 如果没有静默符号
		if (!(flags & ZEND_FETCH_CLASS_SILENT)) {
			// 报错：未定义的常量
			zend_throw_error(NULL, "Undefined constant \"%s\"", name);
		}
		return NULL;
	}

	// 如果没有静默符号 并且 常量有弃用标记
	if (!(flags & ZEND_FETCH_CLASS_SILENT) && (ZEND_CONSTANT_FLAGS(c) & CONST_DEPRECATED)) {
		// 报错，此常量弃用
		zend_error(E_DEPRECATED, "Constant %s is deprecated", name);
	}
	return &c->value;
}

// ing3, 新建常量
static void* zend_hash_add_constant(HashTable *ht, zend_string *key, zend_constant *c)
{
	void *ret;
	// 分配内存新建常量
	zend_constant *copy = pemalloc(sizeof(zend_constant), ZEND_CONSTANT_FLAGS(c) & CONST_PERSISTENT);
	// 把 c 拷贝到新常量里
	memcpy(copy, c, sizeof(zend_constant));
	// 把新常量添加到 ht 里
	ret = zend_hash_add_ptr(ht, key, copy);
	// 如果添加失败
	if (!ret) {
		// 释放p
		pefree(copy, ZEND_CONSTANT_FLAGS(c) & CONST_PERSISTENT);
	}
	return ret;
}

// ing4, 注册常量，检验后添加到 EG(zend_constants) 中，只有 zend_register_constant 用到
ZEND_API zend_result zend_register_constant(zend_constant *c)
{
	zend_string *lowercase_name = NULL;
	zend_string *name;
	zend_result ret = SUCCESS;
	// 是否持久
	bool persistent = (ZEND_CONSTANT_FLAGS(c) & CONST_PERSISTENT) != 0;

#if 0
	printf("Registering constant for module %d\n", c->module_number);
#endif
	// 取回最后一个 \ 的位置，返回指针，还在原字串上，所以后面可以算减法
	const char *slash = strrchr(ZSTR_VAL(c->name), '\\');
	// 如果有命名空间	
	if (slash) {
		// 把名字复制出来 
		lowercase_name = zend_string_init(ZSTR_VAL(c->name), ZSTR_LEN(c->name), persistent);
		// 命名空间部分 转成小写（命名空间不区分大小写，测试过）
		// namespace a{const a=1;} namespace A{const A=1;const a=1;}
		zend_str_tolower(ZSTR_VAL(lowercase_name), slash - ZSTR_VAL(c->name));
		// 创建保留字
		lowercase_name = zend_new_interned_string(lowercase_name);
		// 保留字做名字
		name = lowercase_name;
	// 如果没有，直接用名字
	} else {
		name = c->name;
	}

	// 检验特殊常量名
	/* Check if the user is trying to define any special constant */
	// 如果使用了名称 __COMPILER_HALT_OFFSET__
	if (zend_string_equals_literal(name, "__COMPILER_HALT_OFFSET__")
		// 或 true ,false, null
		|| (!persistent && zend_get_special_const(ZSTR_VAL(name), ZSTR_LEN(name)))
		// 或 添加到全局常量表失败（添加的时候会创建副本，不直接用c）
		|| zend_hash_add_constant(EG(zend_constants), name, c) == NULL
	) {
		// 抛错，常量已定义过
		zend_error(E_WARNING, "Constant %s already defined", ZSTR_VAL(name));
		// 释放常量名
		zend_string_release(c->name);
		// 非持久
		if (!persistent) {
			// 删除常量值
			zval_ptr_dtor_nogc(&c->value);
		}
		// 返回状态：出错
		ret = FAILURE;
	}
	// 如果有小写名字
	if (lowercase_name) {
		// 释放小写名字
		zend_string_release(lowercase_name);
	}
	return ret;
}
