//********************* /zend/zend_types.h ****************************************************

typedef struct _zend_object_handlers zend_object_handlers;
typedef struct _zend_class_entry     zend_class_entry;
// /Zend/zend_compile.h	定义
typedef union  _zend_function        zend_function;
typedef struct _zend_execute_data    zend_execute_data;

// zval 内置变量
typedef struct _zval_struct     zval;

// 这些都是很常用的结构体
typedef struct _zend_refcounted zend_refcounted;
typedef struct _zend_string     zend_string;
typedef struct _zend_array      zend_array;
typedef struct _zend_object     zend_object;
typedef struct _zend_resource   zend_resource;
typedef struct _zend_reference  zend_reference;
typedef struct _zend_ast_ref    zend_ast_ref;
typedef struct _zend_ast        zend_ast;

// 比较函数的抽象形式
typedef int  (*compare_func_t)(const void *, const void *);
typedef void (*swap_func_t)(void *, void *);
typedef void (*sort_func_t)(void *, size_t, size_t, compare_func_t, swap_func_t);
typedef void (*dtor_func_t)(zval *pDest);
typedef void (*copy_ctor_func_t)(zval *pElement);

// 这是个蛮巧妙的东西，可以方便地计算各种类型的交并集
typedef struct {
	// 这里不用union，因为找不到一个同时兼容c和c++的初始化方法。那种初始化只有在c++20中支持
	/* Not using a union here, because there's no good way to initialize them
	 * in a way that is supported in both C and C++ (designated initializers
	 * are only supported since C++20). */
	// 一个泛指针
	void *ptr;
	// 类型掩码，32位无符号整数 
	uint32_t type_mask;
	/* TODO: We could use the extra 32-bit of padding on 64-bit systems. */
} zend_type;


// 类型列表，主要在编译和继承时用到
typedef struct {
	// 数量
	uint32_t num_types;
	// 列表
	zend_type types[1];
} zend_type_list;

// ing2, 所有基础类型都在里面了，所谓弱类型，主要是靠它支持咯？
typedef union _zend_value {
	// 长整型
	zend_long         lval;				/* long value */
	// 双精度
	double            dval;				/* double value */
	// 引用计数器. 每个 zend_value都有引用计数器，所以 zval也不用说了
	zend_refcounted  *counted;
	// 字串
	zend_string      *str;
	// 数组
	zend_array       *arr;
	// 对象
	zend_object      *obj;
	// 资源
	zend_resource    *res;
	// 地址引用
	zend_reference   *ref;
	// 基本的引用计数器，做什么用的？上面不是已经有一个计数器了么
	zend_ast_ref     *ast;
	// 内置变量，php程序不可访问。用到这个的，类型会是间接引用 IS_INDIRECT
	zval             *zv;
	// 指针
	void             *ptr;
	// 类入口
	zend_class_entry *ce;
	// 函数
	zend_function    *func;
	// 
	struct {
		// 全局没有用到
		uint32_t w1;
		// ZVAL_COPY_VALUE 用到
		uint32_t w2;
	} ww;
} zend_value;


// zval is the alias, 核心是 value ,这层包装主要是记录运行状态
struct _zval_struct {
	// 弱类型变量
	zend_value        value;			/* value */
	// 
	union {
		uint32_t type_info;
		// ？这里放个struct干嘛
		struct {
			// 高位或低位在前
			ZEND_ENDIAN_LOHI_3(
				// 
				zend_uchar    type,			/* active type */
				// 
				zend_uchar    type_flags,
				// ？这里放个union干嘛
				union {
					uint16_t  extra;        /* not further specified */
				} u)
		} v;
	} u1;
	// 全是 int。都是运行状态相关标记。
	union {
		// ing3, 哈希冲突链。哈希表里用它指向同样哈希值的下一个元素! 
		// 为了哈希表给每个内置变量加个指针，可见php对哈希表的重视。
		uint32_t     next;                 /* hash collision chain */
		//
		uint32_t     cache_slot;           /* cache slot (for RECV_INIT) */
		// 操作码编号，用于快速调用
		uint32_t     opline_num;           /* opline number (for FAST_CALL) */
		// 行号， 为了语句节点
		uint32_t     lineno;               /* line number (for ast nodes) */
		// 执行时用到的参数数量 ？
		uint32_t     num_args;             /* arguments number for EX(This) */
		// foreach 位置
		uint32_t     fe_pos;               /* foreach position */
		// foreach 迭代序号
		uint32_t     fe_iter_idx;          /* foreach iterator index */
		// 成员对象保护
		uint32_t     property_guard;       /* single property guard */
		// 常量标记
		uint32_t     constant_flags;       /* constant flags */
		// 额外数据，少量调用 （不再指定？）
		uint32_t     extra;                /* not further specified */
	} u2;
};


// ing4, gc计数器，由两个32位整数组成。这个结构体主要给内部引用，包装在其他 struct 里。所以这是一个头信息~
typedef struct _zend_refcounted_h {
	// 引用次数
	uint32_t         refcount;			/* reference counter 32-bit */
	// 类型信息存放在这里, 整型 gc.u.type_info( 参看 GC_TYPE_INFO(p) )
	// 为什么放个union ？
	union {
		// 用来做位标记，32位整数
			//分成4段使用：最左2位表示颜色，次左20位表示地址索引号，最右4位是type(类型),次右6位是flags
		uint32_t type_info;
	} u;
} zend_refcounted_h;

// 只封装一个 zend_refcounted_h 计数器，供外部调用
struct _zend_refcounted {
	zend_refcounted_h gc;
};

// 带引用计数器的 字串
struct _zend_string {
	// 引用计数器
	zend_refcounted_h gc;
	// 哈希值。为了用作数组key，给每个字串加个哈希值！
	zend_ulong        h;                /* hash value */
	// 长度,这是它的占用空间长度
	size_t            len;
	// 为什么不是 char *val? 因为如果是指针，就需要给指向位置再分配内存，那就不能包在一个 _zend_string 里面了，内存控制也更麻烦
	// 这个长度限制在 类型转换后会失效，可以写到 a->val[100] ，写到外面去，这个蛮危险的.（测试过，但还没搞太明白）
	char              val[1];
};

// 哈希表的一个键值对
typedef struct _Bucket {
	// 值
	zval              val;
	// 哈希值 或 数字索引号
	zend_ulong        h;                /* hash value (or numeric index)   */
	// 键：字串型key，数字型保存 NULL
	zend_string      *key;              /* string key or NULL for numerics */
} Bucket;

// hashTable 是 zend_array 的别名
typedef struct _zend_array HashTable;

// array 和 hashtable. 哈希表可能在很多地方被创建
struct _zend_array {
	// 引用计数器
	zend_refcounted_h gc;
	// 加了union之后这里只算一个指针的大小。有什么用？
	union {
		// 只有 struct 里面可以用 ZEND_ENDIAN_LOHI_4，所以这里定义 struct
		struct {
			// 唯一一处调用，只是改变一下顺序，四个元素同样类型 
			ZEND_ENDIAN_LOHI_4(
				zend_uchar    flags,
				zend_uchar    _unused,
				// 哈希表的迭代器数量
				zend_uchar    nIteratorsCount,
				zend_uchar    _unused2)
		} v;
		// int : 标记
		uint32_t flags;
	} u;
	// -(nTableSize*2)
	uint32_t          nTableMask;
	// 为什么要 union ?
	union {
		// #define HT_HASH(ht, idx) HT_HASH_EX((ht)->arHash, idx) // 全局只有这里用到
		// ( key 的哈希值 | mask) 得到在这各指针中的位置（负数），里面存着对应的元素序号。
		// 哈希值的列表的【结尾位置】哈希值表这个指针的前面。
		uint32_t     *arHash;   /* hash table (allocated above this pointer) */
		// 哈希表，bucket 键值对 列表
		Bucket       *arData;   /* array of hash buckets */
		// 顺序数组，zval列表
		zval         *arPacked; /* packed array of zvals */
	};
	// 已使用数量 包含 UNDEF
	uint32_t          nNumUsed;
	// 有效元素数 不包含 UNDEF
	uint32_t          nNumOfElements;
	// 大小
	uint32_t          nTableSize;
	// 内部指针
	uint32_t          nInternalPointer;
	// 下一个空元素
	zend_long         nNextFreeElement;
	// 销毁函数
	dtor_func_t       pDestructor;
};

// 迭代器。一般都是运行时创建，放在 EG(ht_iterators) 里
typedef struct _HashTableIterator {
	// 哈希表指针
	HashTable    *ht;
	// 位置，int型
	HashPosition  pos;
} HashTableIterator;


// ing2， php 对象
// zend_object 都是在 execute执行阶段，在虚拟机里创建的，怪不得编译阶段完全没有这个东东
struct _zend_object {
	// 引用计数
	zend_refcounted_h gc;
	// handle 是对象在全局对象容器 EG(objects_store).object_buckets 中的【索引号】，通过它可以直接在全局容器中获取指针
	uint32_t          handle; // TODO: may be removed ???
	// 所属类入口
	zend_class_entry *ce;
	// 对象操作句柄，只读
	const zend_object_handlers *handlers;
	// 对象的【动态属性】哈希表，通过__get,__set访问。默认属性也添加在里面，参见 rebuild_object_properties
	HashTable        *properties;
	// 默认属性表，这里可以放任意多个属性（zval），对象被创建的时候它就创建好了
	zval              properties_table[1];
};

// ing4， resource 对象
struct _zend_resource {
	// 引用次数
	zend_refcounted_h gc;
	// 可删除，实际上这个是有有用到的
	zend_long         handle; // TODO: may be removed ???
	// 类型
	int               type;
	// 指针
	void             *ptr;
};

// 类属性列表？
typedef struct {
	size_t num;
	size_t num_allocated;
	struct _zend_property_info *ptr[1];
} zend_property_info_list;

// ? 执行时少量调用
typedef union {
	struct _zend_property_info *ptr;
	uintptr_t list;
} zend_property_info_source_list;



// 引用
struct _zend_reference {
	// 引用次数
	zend_refcounted_h              gc;
	// 变量，引用结构体中包含一个内置变量，而不是变量指针，通过这个变量指向目标
	zval                           val;
	// 成员变量信息？
	zend_property_info_source_list sources;
};

// 只封装一个计数器，
// 所谓自动垃圾回收，就是引用清零后自动销毁对象？
struct _zend_ast_ref {
	zend_refcounted_h gc;
	// zend_ast 兼容 zend_ast_ref的结构
	/*zend_ast        ast; zend_ast follows the zend_ast_ref structure */
};

//********************* /zend/zend_compile.h ****************************************************

typedef struct _zend_op_array zend_op_array;
typedef struct _zend_op zend_op;

// znode_op, 指 operand，运算对象。 是最基础的结构体，没有引用其他结构体。
	// 只有几个外部文件直接引用它，但引用 zend_op 的非常多，都间接引用了它。
	// /Zend/zend_execute.c, /Zend/Optimizer/block_pass.c, /Zend/zend_execute.h, /Zend/Optimizer/zend_dump.c	
	// zval是内置类型吗？
typedef union _znode_op {
	uint32_t      constant; // 在 _zend_execute_data 中，此node 的常量的偏移量	
	uint32_t      var; // 在 _zend_execute_data 中 此node 的变量的偏移量
	uint32_t      num; // 创建过程中的序号，例如，表示这是第几个参数
	// 操作码行号？
	uint32_t      opline_num; /*  Needs to be signed */

#if ZEND_USE_ABS_JMP_ADDR // 32位系统 	
	zend_op       *jmp_addr; // 跳转地址，只有这一个是指针
#else // 64位系统	
	uint32_t      jmp_offset; // 跳转偏移量
#endif

#if ZEND_USE_ABS_CONST_ADDR // 32位系统有 zv ，64位用constant
	zval          *zv; // 常量
#endif
} znode_op;

// 只有编译过程中用到，常用来接收表达式结果
typedef struct _znode { /* used only during compilation */
	zend_uchar op_type;
	zend_uchar flag;
	// 放 union 的好处是什么？只是为了结构清晰吗？
	union {
		// 这里不是指针 通过 SET_NODE 把 （zend_op 下属的 znode_op） 复制过来
		// 或者通过 GET_NODE 把它传给 （zend_op 下属的 znode_op）
		znode_op op;
		// 同上，通过 SET_NODE 和 GET_NODE 传递
		zval constant; /* replaced by literal/zv */
	} u;
} znode;


// 临时定义，避免头部顺序问题，znode如何和zend_ast里的指针通用呢？
/* Temporarily defined here, to avoid header ordering issues */
typedef struct _zend_ast_znode {
	zend_ast_kind kind;
	zend_ast_attr attr;
	uint32_t lineno;
	znode node;
} zend_ast_znode;

ZEND_API zend_ast * ZEND_FASTCALL zend_ast_create_znode(znode *node);

// 直接
static zend_always_inline znode *zend_ast_get_znode(zend_ast *ast) {
	return &((zend_ast_znode *) ast)->node;
}

typedef struct _zend_declarables {
	zend_long ticks;
} zend_declarables;

// 编译上下文，每个文件一个，但是 可以通过 op arrays 共享
/* Compilation context that is different for each file, but shared between op arrays. */
typedef struct _zend_file_context {
	zend_declarables declarables;

	zend_string *current_namespace;
	bool in_namespace;
	bool has_bracketed_namespaces;

	HashTable *imports;
	HashTable *imports_function;
	HashTable *imports_const;

	HashTable seen_symbols;
} zend_file_context;

//
typedef union _zend_parser_stack_elem {
	zend_ast *ast;
	zend_string *str;
	zend_ulong num;
	unsigned char *ptr;
	unsigned char *ident;
} zend_parser_stack_elem;

void zend_compile_top_stmt(zend_ast *ast);
void zend_const_expr_to_zval(zval *result, zend_ast **ast_ptr, bool allow_dynamic);

typedef int (*user_opcode_handler_t) (zend_execute_data *execute_data);

// 这是一个操作，也是一个操作码.  程序里变量经常叫做op_line
struct _zend_op {
	// 处理器，只有这一个指针
	const void *handler;
	// 这三个都是znode_op,而且不是指针. op 是指 operand，计算对象
	znode_op op1;
	znode_op op2;
	znode_op result;
	// 扩展值
	uint32_t extended_value;
	// 行号
	uint32_t lineno;
	// 这里是opcode, 字串类型
	zend_uchar opcode;
	// 上面3个东东的类型
	zend_uchar op1_type;
	zend_uchar op2_type;
	zend_uchar result_type;
};

// ? break,continue 跳转点 只有内部调用
	// 编译中用到它的方法 zend_begin_loop，zend_end_loop，get_next_brk_cont_element
typedef struct _zend_brk_cont_element {
	
	int start;
	int cont;
	int brk;
	// 上一层跳转点
	int parent;
	// 是否用在switch中
	bool is_switch;
} zend_brk_cont_element;

// 跳转标签
typedef struct _zend_label {
	//
	int brk_cont;
	// 操作码序号
	uint32_t opline_num;
} zend_label;

//
typedef struct _zend_try_catch_element {
	uint32_t try_op;
	uint32_t catch_op;  /* ketchup! */
	uint32_t finally_op;
	uint32_t finally_end;
} zend_try_catch_element;


// 操作码活动区域，一般放在 op_array 中
typedef struct _zend_live_range {
	// 低位用来来存放变量类型 ZEND_LIVE_*
	uint32_t var; /* low bits are used for variable type (ZEND_LIVE_* macros) */
	uint32_t start;
	uint32_t end;
} zend_live_range;

// 操作码上下文，给每个操作码列表一个独立的上下文
/* Compilation context that is different for each op array. */
typedef struct _zend_oparray_context {
	uint32_t   opcodes_size;
	int        vars_size;
	int        literals_size;
	uint32_t   fast_call_var;
	uint32_t   try_catch_offset;
	// 这是个整数，是 _zend_brk_cont_element 在列表中的编号 ，不是_zend_brk_cont_element实例
	int        current_brk_cont;
	int        last_brk_cont;
	zend_brk_cont_element *brk_cont_array;
	// 跳转 label
	HashTable *labels;
} zend_oparray_context;


// 类属性信息
typedef struct _zend_property_info {
	// 普通属性的 offset 或 静态属性的 index
	uint32_t offset; /* property offset for object properties or
	                      property index for static properties */
	// 标记
	uint32_t flags;
	// 名称
	zend_string *name;
	// 文档
	zend_string *doc_comment;
	// 修饰属性
	HashTable *attributes;
	// 类入口
	zend_class_entry *ce;
	// 类型
	zend_type type;
} zend_property_info;


// 类常量
typedef struct _zend_class_constant {
	// 值，标记存放在 zval 的 u2 里
	zval value; /* flags are stored in u2 */
	// 文档
	zend_string *doc_comment;
	// 修饰属性
	HashTable *attributes;
	// 类入口
	zend_class_entry *ce;
} zend_class_constant;


// 内置函数的参数信息
/* arg_info for internal functions */
typedef struct _zend_internal_arg_info {
	// 参数名
	const char *name;
	// 类型
	zend_type type;
	// 默认值
	const char *default_value;
} zend_internal_arg_info;

// 用户定义函数的参数信息, 结构不同
/* arg_info for user functions */
typedef struct _zend_arg_info {
	// 参数名
	zend_string *name;
	// 类型
	zend_type type;
	// 默认值
	zend_string *default_value;
} zend_arg_info;

// 它重复了 zend_internal_arg_info 的结构，但意思不同。
// 它被当成 arg_info 数组的第一个元素，来定义内置函数的属性。
// 这个只能用于返回类型
/* the following structure repeats the layout of zend_internal_arg_info,
 * but its fields have different meaning. It's used as the first element of
 * arg_info array to define properties of internal functions.
 * It's also used for the return type.
 */
 // 内置函数信息，只能用于返回类型
typedef struct _zend_internal_function_info {
	// 必须参数数量
	zend_uintptr_t required_num_args;
	// 类型
	zend_type type;
	// 默认值
	const char *default_value;
} zend_internal_function_info;

// 每个 _zend_op_array 对应一个function 或者goto标签
	// 参看编译过程中的 CG(active_op_array) = 作用域切换
struct _zend_op_array {
	/* Common elements */
	zend_uchar type;
	// 引用传递的参数的 bitset
	zend_uchar arg_flags[3]; /* bitset of arg_info.pass_by_reference */
	// 标记
	uint32_t fn_flags;
	
	zend_string *function_name; // 所属函数名
	zend_class_entry *scope; // 所属类条目
	
	zend_function *prototype; // 函数原型
	
	uint32_t num_args; // 参数数量
	
	uint32_t required_num_args; // 需要的参数数量
	
	zend_arg_info *arg_info; // 参数信息列表
	
	HashTable *attributes; // 修饰属性
	// 临时变量数量
	uint32_t T;         /* number of temporary variables */
	
	ZEND_MAP_PTR_DEF(void **, run_time_cache); // 运行时缓存 指针列表
	/* END of common elements */
	// 以上是通用元素

	// 这里以前，与 _zend_function 结构一致
	// 缓存位置指针 数量 
	int cache_size;     /* number of run_time_cache_slots * sizeof(void*) */
	// 编译变量的数量
	int last_var;       /* number of CV variables */
	// 操作码数量
	uint32_t last;      /* number of opcodes */

	
	zend_op *opcodes; // 一串操作码的指针
	
	ZEND_MAP_PTR_DEF(HashTable *, static_variables_ptr); // 静态变量指针列表
	
	HashTable *static_variables; // 静态变量
	// 编译变量的变量名列表
	zend_string **vars; /* names of CV variables */

	
	uint32_t *refcount; // 引用次数。这里是个整数的指针

	// 最后一个活动区域
	int last_live_range;
	// try/catch 数量
	int last_try_catch;
	// 活动区域指针列表
	zend_live_range *live_range;
	// try_catch指针列表
	zend_try_catch_element *try_catch_array;

	
	zend_string *filename; // 文件名 
	
	uint32_t line_start; // 在文件中的开始行数
	
	uint32_t line_end; // 结束行数
	// 专属注释  /** 开头
	zend_string *doc_comment;

	// literal 数量
	int last_literal;
	// 闭包数量？
	uint32_t num_dynamic_func_defs;
	// ? 主要在 /Zend/Optimizer/compact_literals.c 和 /Zend/Optimizer/zend_optimizer.c 里面用到
	// 这个好像用来做opcache用的
	// 这个是作为 类型名，方法名 的字串列表。还有其他作用。
	zval *literals;

	// 代码块中的闭包
	/* Functions that are declared dynamically are stored here and
	 * referenced by index from opcodes. */
	zend_op_array **dynamic_func_defs;

	// 6
	void *reserved[ZEND_MAX_RESERVED_RESOURCES];
};



/* zend_internal_function_handler */
typedef void (ZEND_FASTCALL *zif_handler)(INTERNAL_FUNCTION_PARAMETERS);

// 内部函数
typedef struct _zend_internal_function {
	// 公共元素
	/* Common elements */
	// 类型
	zend_uchar type;
	// ？
	zend_uchar arg_flags[3]; /* bitset of arg_info.pass_by_reference */
	// ？
	uint32_t fn_flags;
	// 函数名
	zend_string* function_name;
	// 作用域
	zend_class_entry *scope;
	// php函数
	zend_function *prototype;
	// 参数数量
	uint32_t num_args;
	// 必须参数数量
	uint32_t required_num_args;
	// 参数信息列表
	zend_internal_arg_info *arg_info;
	// 修饰属性
	HashTable *attributes;
	// 临时变量数量
	uint32_t T;         /* number of temporary variables */
	// 
	ZEND_MAP_PTR_DEF(void **, run_time_cache);
	/* END of common elements */
	// 以上是通用元素

	// 处理器
	zif_handler handler;
	struct _zend_module_entry *module;
	// 	#define ZEND_MAX_RESERVED_RESOURCES 6
	// 保存的资源 
	void *reserved[ZEND_MAX_RESERVED_RESOURCES];
} zend_internal_function;



// php函数， 这两个结构体主体部分类似. union 和 struct 区别？为什么在 union 里定义 struct ?
union _zend_function {
	// 类型，必须是结构中第一个元素
	zend_uchar type;	/* MUST be the first element of this struct! */
	// 快速参数标记,最前面6个位空着，每个参数2个位。 标记内容是此参数的传递方式，是否引用传递。
	uint32_t   quick_arg_flags;

	struct {
		// 不会用到
		zend_uchar type;  /* never used */
		// 参数标记
		zend_uchar arg_flags[3]; /* bitset of arg_info.pass_by_reference */
		// 函数标记
		uint32_t fn_flags;
		// 函数名
		zend_string *function_name;
		// 所属类
		zend_class_entry *scope;
		// 原型，父函数？
		zend_function *prototype;
		// 参数数量
		uint32_t num_args;
		// 必须参数数量
		uint32_t required_num_args;
		// 参数信息，如果任何参数都可以，返回-1
		zend_arg_info *arg_info;  /* index -1 represents the return value info, if any */
		// 修饰属性
		HashTable   *attributes;
		// 临时变量数量
		uint32_t T;         /* number of temporary variables */
		// observer里用到，观察者函数列表。前面n个是begin函数 ，后面n个是end函数。
		ZEND_MAP_PTR_DEF(void **, run_time_cache);
	} common;

	// 这里以前，与 _zend_op_array 结构一致
	// 操作码
	zend_op_array op_array;
	// 内部函数
	zend_internal_function internal_function;
};

// 单个执行数据。最终被执行的不只是opcode而是这样打包好的一包数据 
struct _zend_execute_data {
	// 操作码执行过的操作码指针, （一个，而不是一堆）
	const zend_op       *opline;           /* executed opline                */
	// 执行数据，当前正在调用的执行数据。
	// 它可以调用多个执行数据，这些执行数据都指向它，但它只指向正在调用的这个。
	zend_execute_data   *call;             /* current call                   */
	
	zval                *return_value; // 返回值
	// 所属函数
	zend_function       *func;             /* executed function              */
	// $this 对象 （ this + call_info + num_args）
	zval                 This;             /* this + call_info + num_args    */
	// 前一个执行数据对象 
	zend_execute_data   *prev_execute_data;
	// 符号表
	zend_array          *symbol_table;
	// 运行时缓存
	void               **run_time_cache;   /* cache op_array->run_time_cache */
	// 扩展命名参数 （函数的命名参数）
	zend_array          *extra_named_params;
};


struct _zend_arena;

typedef struct _zend_auto_global {
	zend_string *name;
	zend_auto_global_callback auto_global_callback;
	// ？
	bool jit;
	// ？
	bool armed;
} zend_auto_global;


//********************* /Zend/zend.h ****************************************************


struct _zend_serialize_data;
struct _zend_unserialize_data;

typedef struct _zend_serialize_data zend_serialize_data;
typedef struct _zend_unserialize_data zend_unserialize_data;

// 类名：用在类实现接口列表上
typedef struct _zend_class_name {
	// 类名
	zend_string *name;
	// 小写名称
	zend_string *lc_name;
} zend_class_name;

// trait 方法引用
typedef struct _zend_trait_method_reference {
	// 方法名
	zend_string *method_name;
	// 类名
	zend_string *class_name;
} zend_trait_method_reference;

// trait 优先
typedef struct _zend_trait_precedence {
	// trait 方法引用
	zend_trait_method_reference trait_method;
	// 排除数量
	uint32_t num_excludes;
	// 排除类名
	zend_string *exclude_class_names[1];
} zend_trait_precedence;

// trait 别名
typedef struct _zend_trait_alias {
	// trait 方法引用
	zend_trait_method_reference trait_method;

	// 被添加的方法
	/**
	* name for method to be added
	*/
	zend_string *alias;

	// 修饰符
	/**
	* modifiers to be set on trait method
	*/
	uint32_t modifiers;
} zend_trait_alias;

// 类的可转换数据
typedef struct _zend_class_mutable_data {
	// 默认成员变量列表
	zval      *default_properties_table;
	// 常量列表
	HashTable *constants_table;
	// class entry 标记
	uint32_t   ce_flags;
	// 枚举数据
	HashTable *backed_enum_table;
} zend_class_mutable_data;

// 类依赖
typedef struct _zend_class_dependency {
	// 名称
	zend_string      *name;
	// 类入口
	zend_class_entry *ce;
} zend_class_dependency;

//
typedef struct _zend_inheritance_cache_entry zend_inheritance_cache_entry;

// ing4 错误信息
typedef struct _zend_error_info {
	// 类型
	int type;
	// 行号
	uint32_t lineno;
	// 文件名
	zend_string *filename;
	// 错误信息
	zend_string *message;
} zend_error_info;

// 继承缓存入口？
struct _zend_inheritance_cache_entry {
	// 下一个缓存对象
	zend_inheritance_cache_entry *next;
	// 类入口
	zend_class_entry             *ce;
	// 父类入口
	zend_class_entry             *parent;
	// 依赖类
	zend_class_dependency        *dependencies;
	// 依赖类数量
	uint32_t                      dependencies_count;
	// 错误数
	uint32_t                      num_warnings;
	// 错误信息
	zend_error_info             **warnings;
	// 作用域
	zend_class_entry             *traits_and_interfaces[1];
};

// ing3, 类入口，作用域。php类在c语言中的呈现。
struct _zend_class_entry {
	// 类型
	char type;
	// 名称
	zend_string *name;
	// 类入口或字串依赖 ZEND_ACC_LINKED
	/* class_entry or string depending on ZEND_ACC_LINKED */
	union {
		// 父类入口
		zend_class_entry *parent;
		// 父类名
		zend_string *parent_name;
	};
	// 引用数
	int refcount;
	// class entry 标记
	uint32_t ce_flags;

	// 属性这一段参见 zend_api.c : zend_declare_typed_property()
	// 默认动态成员变量数量
	int default_properties_count;
	// 默认静态成员变量数量
	int default_static_members_count;
	
	// 默认动态成员变量列表
	zval *default_properties_table;
	// 默认静态成员变量列表
	zval *default_static_members_table;
	
	// zval * static_members_table__ptr;
	ZEND_MAP_PTR_DEF(zval *, static_members_table);
	// 成员方法列表
	HashTable function_table;
	
	// 成员变量信息。不管是静态还是动态，属性信息都在这里面。
	HashTable properties_info;
	
	
	// 常量列表
	HashTable constants_table;

	// 【转换数据】的指针
	// zend_class_mutable_data* mutable_data__ptr;
	ZEND_MAP_PTR_DEF(zend_class_mutable_data*, mutable_data);
	// 继承缓存入口？
	zend_inheritance_cache_entry *inheritance_cache;

	// 动态属性信息表 ，静态的不在里面。
	struct _zend_property_info **properties_info_table;

	// 构造方法
	zend_function *constructor;
	// 析构方法
	zend_function *destructor;
	// 克隆方法
	zend_function *clone;
	// 常用魔术方法
	zend_function *__get;
	zend_function *__set;
	zend_function *__unset;
	zend_function *__isset;
	zend_function *__call;
	zend_function *__callstatic;
	zend_function *__tostring;
	// 调试信息
	zend_function *__debugInfo;
	// 序列化和反序列化
	zend_function *__serialize;
	zend_function *__unserialize;

	// 只有当类实现接口 Iterator 或 IteratorAggregate 时分配
	/* allocated only if class implements Iterator or IteratorAggregate interface */
	// 迭代方法列表
	zend_class_iterator_funcs *iterator_funcs_ptr;
	// 只有当类实现接口 ArrayAccess 时分配
	/* allocated only if class implements ArrayAccess interface */
	// 数组接口方法列表
	zend_class_arrayaccess_funcs *arrayaccess_funcs_ptr;
	// 迭代和数组访问是php的核心特色功能哇，所以单独放出来了。

	// 句柄
	/* handlers */
	union {
		// 这个是php里new 语句调用的的创建对象的方法
		zend_object* (*create_object)(zend_class_entry *class_type);
		// 获取实现此接口的类
		int (*interface_gets_implemented)(zend_class_entry *iface, zend_class_entry *class_type); /* a class implements this interface */
	};
	// 获取zend迭代器的方法
	zend_object_iterator *(*get_iterator)(zend_class_entry *ce, zval *object, int by_ref);
	// 方法获取静态方法的方法
	zend_function *(*get_static_method)(zend_class_entry *ce, zend_string* method);

	// 序列化回调
	/* serializer callbacks */
	// 序列化
	int (*serialize)(zval *object, unsigned char **buffer, size_t *buf_len, zend_serialize_data *data);
	// 反序列化
	int (*unserialize)(zval *object, zend_class_entry *ce, const unsigned char *buf, size_t buf_len, zend_unserialize_data *data);

	// 接口数量
	uint32_t num_interfaces;
	// trait 数量
	uint32_t num_traits;

	// 
	/* class_entry or string(s) depending on ZEND_ACC_LINKED */
	union {
		// 接口指针列表
		zend_class_entry **interfaces;
		// 接口名列表
		zend_class_name *interface_names;
	};

	// trait 名称列表
	zend_class_name *trait_names;
	// trait 别名（zend_trait_alias）指针列表
	zend_trait_alias **trait_aliases;
	// trait 优先列表
	zend_trait_precedence **trait_precedences;
	// 修饰属性表
	HashTable *attributes;

	// enum 的备用类型
	uint32_t enum_backing_type;
	// enum 备用表
	HashTable *backed_enum_table;

	// 信息
	union {
		struct {
			// 文件名
			zend_string *filename;
			// 开始行
			uint32_t line_start;
			// 结束行
			uint32_t line_end;
			// 注释文档
			zend_string *doc_comment;
		} user;
		struct {
			// 方法入口
			const struct _zend_function_entry *builtin_functions;
			// 模块入口
			struct _zend_module_entry *module;
		} internal;
	} info;
};

// 工具方法， /main/main.c	中设置每个方法
typedef struct _zend_utility_functions {
	// 报错方法
	void (*error_function)(int type, zend_string *error_filename, const uint32_t error_lineno, zend_string *message);
	// 打印方法
	size_t (*printf_function)(const char *format, ...) ZEND_ATTRIBUTE_PTR_FORMAT(printf, 1, 2);
	size_t (*write_function)(const char *str, size_t str_length);
	FILE *(*fopen_function)(zend_string *filename, zend_string **opened_path);
	void (*message_handler)(zend_long message, const void *data);
	zval *(*get_configuration_directive)(zend_string *name);
	void (*ticks_function)(int ticks);
	void (*on_timeout)(int seconds);
	zend_result (*stream_open_function)(zend_file_handle *handle);
	void (*printf_to_smart_string_function)(smart_string *buf, const char *format, va_list ap);
	void (*printf_to_smart_str_function)(smart_str *buf, const char *format, va_list ap);
	char *(*getenv_function)(const char *name, size_t name_len);
	zend_string *(*resolve_path_function)(zend_string *filename);
} zend_utility_functions;

// 
typedef struct _zend_utility_values {
	bool html_errors;
} zend_utility_values;

typedef size_t (*zend_write_func_t)(const char *str, size_t str_length);

typedef enum {
	EH_NORMAL = 0,
	EH_THROW
} zend_error_handling_t;

// 错误句柄
typedef struct {
	// 
	zend_error_handling_t  handling;
	// 错误类实体
	zend_class_entry       *exception;
} zend_error_handling;



//********************* /Zend/zend_alloc.h ****************************************************

typedef struct _zend_leak_info {
	void *addr;
	size_t size;
	const char *filename;
	const char *orig_filename;
	uint32_t lineno;
	uint32_t orig_lineno;
} zend_leak_info;

/* Heap functions */
typedef struct _zend_mm_heap zend_mm_heap;


typedef struct _zend_mm_storage zend_mm_storage;

// 这个相当于做了个接口，让其他地方来实现它
// 分配 chunk
typedef	void* (*zend_mm_chunk_alloc_t)(zend_mm_storage *storage, size_t size, size_t alignment);
// 释放 chunk
typedef void  (*zend_mm_chunk_free_t)(zend_mm_storage *storage, void *chunk, size_t size);
// 清空 chunk
typedef bool   (*zend_mm_chunk_truncate_t)(zend_mm_storage *storage, void *chunk, size_t old_size, size_t new_size);
// 扩大 chunk
typedef bool   (*zend_mm_chunk_extend_t)(zend_mm_storage *storage, void *chunk, size_t old_size, size_t new_size);

// 内存操作方法
typedef struct _zend_mm_handlers {
	// 分配chunk的方法
	zend_mm_chunk_alloc_t       chunk_alloc;
	// 释放chunk的方法
	zend_mm_chunk_free_t        chunk_free;
	// 清空chunk的方法
	zend_mm_chunk_truncate_t    chunk_truncate;
	// 扩大 chunk的方法
	zend_mm_chunk_extend_t      chunk_extend;
} zend_mm_handlers;

// ？
struct _zend_mm_storage {
	// 内存处理方法
	const zend_mm_handlers handlers;
	// 
	void *data;
};

//********************* /Zend/zend_alloc.c ****************************************************

typedef struct  _zend_mm_page      zend_mm_page;
typedef struct  _zend_mm_bin       zend_mm_bin;
typedef struct  _zend_mm_free_slot zend_mm_free_slot;
typedef struct  _zend_mm_chunk     zend_mm_chunk;
typedef struct  _zend_mm_huge_list zend_mm_huge_list;

struct _zend_mm_heap {
#if ZEND_MM_CUSTOM
	//
	int                use_custom_heap;
#endif

//
#if ZEND_MM_STORAGE
	//
	zend_mm_storage   *storage;
#endif

//
#if ZEND_MM_STAT
	// 当前内存使用量
	size_t             size;                    /* current memory usage */
	// 峰值内存使用量
	size_t             peak;                    /* peak memory usage */
#endif
	// 小尺寸用的空闲列表，默认30个 , ZEND_MM_BINS=30
	// 这里的 30 对应 ZEND_MM_BINS_INFO 的 30 个配置
	zend_mm_free_slot *free_slot[ZEND_MM_BINS]; /* free lists for small sizes */
#if ZEND_MM_STAT || ZEND_MM_LIMIT
	// 当前分配的pages数量
	size_t             real_size;               /* current size of allocated pages */
#endif
#if ZEND_MM_STAT
	// 分配的pages最大数量 
	size_t             real_peak;               /* peak size of allocated pages */
#endif
#if ZEND_MM_LIMIT
	// 内存限制
	size_t             limit;                   /* memory limit */
	// 溢出标记
	int                overflow;                /* memory overflow flag */
#endif

	// huge块有个单独的列表
	zend_mm_huge_list *huge_list;               /* list of huge allocated blocks */
	// large,small 都在 chunk里
	zend_mm_chunk     *main_chunk;
	// 使用的 chunks 列表
	zend_mm_chunk     *cached_chunks;			/* list of unused chunks */
	// 已分配的 chunks 数量 
	int                chunks_count;			/* number of allocated chunks */
	// 当前请求使用的chunks最大数量
	int                peak_chunks_count;		/* peak number of allocated chunks for current request */
	// 缓存的 chunks 数量 
	int                cached_chunks_count;		/* number of cached chunks */
	// 每个request 平均使用的 chunks 数量 
	double             avg_chunks_count;		/* average number of chunks allocated per request */
	// 最后一次删除后的chunk数量 
	int                last_chunks_delete_boundary; /* number of chunks after last deletion */
	// 删除的超出最后一条边界的 chunks 数量 
	int                last_chunks_delete_count;    /* number of deletion over the last boundary */
#if ZEND_MM_CUSTOM
	union {
		// 正式环境
		struct {
			// 分配内存方法
			void      *(*_malloc)(size_t);
			// 释放内存方法
			void       (*_free)(void*);
			// 调整内存大小方法
			void      *(*_realloc)(void*, size_t);
		} std;
		// 调试环境副加的
		struct {
			// 
			void      *(*_malloc)(size_t ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC);
			//
			void       (*_free)(void*  ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC);
			// 
			void      *(*_realloc)(void*, size_t  ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC);
		} debug;
	} custom_heap;
	// 
	HashTable *tracked_allocs;
#endif
};

// 内存 chunk。根也是一个chunk。结构体信息都保存在chunk的第一个page里
struct _zend_mm_chunk {
	// 所属的 heap
	zend_mm_heap      *heap;
	// 下一个 chunk
	zend_mm_chunk     *next;
	// 上一个 chunk
	zend_mm_chunk     *prev;
	// 空 pages
	uint32_t           free_pages;				/* number of free pages */
	// chunk末尾的空 pages串的开头页码 ，不是数量（这个注释写的不太清楚）
	uint32_t           free_tail;               /* number of free pages at the end of chunk */
	// chunk的创建序号
	uint32_t           num;
	// 这个没有用到
	char               reserve[64 - (sizeof(void*) * 3 + sizeof(uint32_t) * 3)];
	// 这里自带一个 zend_mm_heap，而不是指针。 只有在根chunk里会用到
	zend_mm_heap       heap_slot;               /* used only in main chunk */
	// 512 bits 等于 64 bytes。
	zend_mm_page_map   free_map;                /* 512 bits or 64 bytes */
	// 地图有 512 页, 每页是一个32位整数，4Byte，共2K
	zend_mm_page_info  map[ZEND_MM_PAGES];      /* 2 KB = 512 * 4 */
};

// 内存 page
struct _zend_mm_page {
	// 4k
	char               bytes[ZEND_MM_PAGE_SIZE];
};

/*
   一个bin 是一个或一组连续的pages（最多8个?）用于分配一个特定的 “small size”
 * bin - is one or few continuous pages (up to 8) used for allocation of
 * a particular "small size".
 */
struct _zend_mm_bin {
	// 4k*8
	char               bytes[ZEND_MM_PAGE_SIZE * 8];
};

// 空闲内存位置， 多包装一层 是为了方便地指向同类原素 
struct _zend_mm_free_slot {
	zend_mm_free_slot *next_free_slot;
};

// huge块列表（串）
struct _zend_mm_huge_list {
	void              *ptr;
	size_t             size;
	// 指向下一个
	zend_mm_huge_list *next;
// 调试模式
#if ZEND_DEBUG
	// 调试信息
	zend_mm_debug_info dbg;
#endif
};


// AG : alloc_globals
// 全局的内存分配节点 里面有一个指向内存heap的指针
typedef struct _zend_alloc_globals {
	zend_mm_heap *mm_heap;
} zend_alloc_globals;


//********************* /Zend/zend_globals.h ****************************************************


typedef struct _zend_vm_stack *zend_vm_stack;
typedef struct _zend_ini_entry zend_ini_entry;
typedef struct _zend_fiber_context zend_fiber_context;
typedef struct _zend_fiber zend_fiber;

// CG 函数是指 compiler globals
// 果然 zend_globals_macros.h # define CG(v) (compiler_globals.v)
struct _zend_compiler_globals {
	// 循环堆栈
	zend_stack loop_var_stack;

	// 当前类入口，当前所属类
	zend_class_entry *active_class_entry;

	// 正在编译的文件名
	zend_string *compiled_filename;

	// 行号
	int zend_lineno;

	// 当前上下文的 操作码列表
	zend_op_array *active_op_array;

	// 当前类的函数列表
	HashTable *function_table;	/* function symbol table */
	// 当前import 的类列表
	HashTable *class_table;		/* class table */

	// 自动全局变量 ，what is it ?
	HashTable *auto_globals;

	/* Refer to zend_yytnamerr() in zend_language_parser.y for meaning of values */
	zend_uchar parse_error;
	
	// 正在编译, 业务逻辑比较复杂， CG(in_compilation)
	bool in_compilation;
	
	// 短标签？
	bool short_tags;

	// ？
	bool unclean_shutdown;

	bool ini_parser_unbuffered_errors;

	zend_llist open_files;

	struct _zend_ini_parser_param *ini_parser_param;

	bool skip_shebang;
	bool increment_lineno;

	bool variable_width_locale;   /* UTF-8, Shift-JIS, Big5, ISO 2022, EUC, etc */
	bool ascii_compatible_locale; /* locale uses ASCII characters as singletons */
	                              /* and don't use them as lead/trail units     */
	// 文档
	zend_string *doc_comment;
	uint32_t extra_fn_flags;

	uint32_t compiler_options; /* set of ZEND_COMPILE_* constants */

	// 操作码上下文
	zend_oparray_context context;
	
	// 文件上下文， FC(name) 获取 file_context 的下属元素
	zend_file_context file_context;

	zend_arena *arena;

	HashTable interned_strings;

	const zend_encoding **script_encoding_list;
	size_t script_encoding_list_size;
	bool multibyte;
	bool detect_unicode;
	bool encoding_declared;

	zend_ast *ast;
	zend_arena *ast_arena;

	zend_stack delayed_oplines_stack;
	HashTable *memoized_exprs;
	int memoize_mode;

	void   *map_ptr_real_base;
	void   *map_ptr_base;
	size_t  map_ptr_size;
	size_t  map_ptr_last;

	HashTable *delayed_variance_obligations;
	// 延时自动加载？
	HashTable *delayed_autoloads;
	// 删除的 use?
	HashTable *unlinked_uses;
	
	// ?
	zend_class_entry *current_linking_class;

	// rtd ?
	uint32_t rtd_key_counter;

	// 短路堆栈深度
	zend_stack short_circuiting_opnums;
};

// 执行器的全局变量: 
struct _zend_executor_globals {
	zval uninitialized_zval;
	zval error_zval;

	// 符号表缓存
	/* symbol table cache */
	zend_array *symtable_cache[SYMTABLE_CACHE_SIZE];
	
	// 超过 symtable_cache 结尾1个位置
	/* Pointer to one past the end of the symtable_cache */
	zend_array **symtable_cache_limit;
	
	// 第一个未使用的符号缓存位置指针
	/* Pointer to first unused symtable_cache slot */
	zend_array **symtable_cache_ptr;

	// 主符号表
	zend_array symbol_table;		/* main symbol table */

	// 已经包含的文件
	HashTable included_files;	/* files already included */

	JMP_BUF *bailout;

	int error_reporting;
	int exit_status;

	// 函数表
	HashTable *function_table;	/* function symbol table */
	// 类列
	HashTable *class_table;		/* class table */
	// 常量表
	HashTable *zend_constants;	/* constants table */
	// 
	zval          *vm_stack_top;
	// 堆栈结尾
	zval          *vm_stack_end;
	// 虚拟机堆栈
	zend_vm_stack  vm_stack;
	// 虚拟机堆栈页大小
	size_t         vm_stack_page_size;

	// 当前执行数据
	struct _zend_execute_data *current_execute_data;
	// 获取成员属性等操作时用到的临时域
	zend_class_entry *fake_scope; /* used to avoid checks accessing properties */

	// 用于追踪 JIT 来引用当前运行的追踪
	uint32_t jit_trace_num; /* Used by tracing JIT to reference the currently running trace */

	// 小数位置
	zend_long precision;

	// 
	int ticks_count;

	// 持久常量数量
	uint32_t persistent_constants_count;
	// 持久函数数量
	uint32_t persistent_functions_count;
	// 持久类数量
	uint32_t persistent_classes_count;

	// 
	HashTable *in_autoload;
	// 是否一次性清除所有的 模块表，常量表，函数表。zend_API.c,zend_execute_API.c
	bool full_tables_cleanup;

	// 支持扩展信息
	/* for extended information support */
	bool no_extensions;

	// 原子布尔型，只起到布尔型的作用
	zend_atomic_bool vm_interrupt;
	zend_atomic_bool timed_out;
	
	// 
	zend_long hard_timeout;

// windows
#ifdef ZEND_WIN32
	// windows版本信息。 OSVERSIONINFOEX 是windows自带的结构体
	OSVERSIONINFOEX windows_version_info;
#endif

	// 持久列表，zend_list.c 等位置 用到
	HashTable regular_list;
	// 持久列表，zend_list.c 等位置用到
	HashTable persistent_list;

	int user_error_handler_error_reporting;
	zval user_error_handler;
	zval user_exception_handler;
	zend_stack user_error_handlers_error_reporting;
	zend_stack user_error_handlers;
	zend_stack user_exception_handlers;

	zend_error_handling_t  error_handling;
	zend_class_entry      *exception_class;

	/* timeout support */
	zend_long timeout_seconds;

	int capture_warnings_during_sccp;

	HashTable *ini_directives;
	HashTable *modified_ini_directives;
	zend_ini_entry *error_reporting_ini_entry;
	// 对象容器
	zend_objects_store objects_store;
	zend_object *exception, *prev_exception;
	const zend_op *opline_before_exception;
	zend_op exception_op[3];
	
	// 当下正在载入的模块
	struct _zend_module_entry *current_module;

	bool active;
	zend_uchar flags;

	zend_long assertions;

	uint32_t           ht_iterators_count;     /* number of allocated slots */
	uint32_t           ht_iterators_used;      /* number of used slots */
	HashTableIterator *ht_iterators;
/*
这个东西在全局只有读没有写
\Zend\zend_execute_API.c （匹配4次）
	行  183: 	EG(ht_iterators_count) = sizeof(EG(ht_iterators_slots)) / sizeof(HashTableIterator);
	行  185: 	EG(ht_iterators) = EG(ht_iterators_slots);
	行  186: 	memset(EG(ht_iterators), 0, sizeof(EG(ht_iterators_slots)));
	行  476: 		if (EG(ht_iterators) != EG(ht_iterators_slots)) {
\Zend\zend_globals.h （匹配1次）
	行 240: 	HashTableIterator  ht_iterators_slots[16];
\Zend\zend_hash.c （匹配2次）
	行  525: 	if (EG(ht_iterators) == EG(ht_iterators_slots)) {
	行  527: 		memcpy(EG(ht_iterators), EG(ht_iterators_slots), sizeof(HashTableIterator) * EG(ht_iterators_count));
*/	
	HashTableIterator  ht_iterators_slots[16];

	void *saved_fpu_cw_ptr;
#if XPFPA_HAVE_CW
	XPFPA_CW_DATATYPE saved_fpu_cw;
#endif
	// 弹跳方法 __call __callstatic ?
	zend_function trampoline;
	// 弹跳方法的操作码
	zend_op       call_trampoline_op;

	HashTable weakrefs;

	bool exception_ignore_args;
	zend_long exception_string_param_max_len;

	// 垃圾回收缓冲区
	zend_get_gc_buffer get_gc_buffer;

	zend_fiber_context *main_fiber_context;
	zend_fiber_context *current_fiber_context;

	/* Active instance of Fiber. */
	zend_fiber *active_fiber;

	/* Default fiber C stack size. */
	zend_long fiber_stack_size;

	/* If record_errors is enabled, all emitted diagnostics will be recorded,
	 * in addition to being processed as usual. */
	bool record_errors;
	uint32_t num_errors;
	zend_error_info **errors;

	/* Override filename or line number of thrown errors and exceptions */
	zend_string *filename_override;
	zend_long lineno_override;

#ifdef ZEND_MAX_EXECUTION_TIMERS
	timer_t max_execution_timer_timer;
	pid_t pid;
	struct sigaction oldact;
#endif

	void *reserved[ZEND_MAX_RESERVED_RESOURCES];
};

#define EG_FLAGS_INITIAL				(0)
#define EG_FLAGS_IN_SHUTDOWN			(1<<0)
#define EG_FLAGS_OBJECT_STORE_NO_REUSE	(1<<1)
#define EG_FLAGS_IN_RESOURCE_SHUTDOWN	(1<<2)

struct _zend_ini_scanner_globals {
	zend_file_handle *yy_in;
	zend_file_handle *yy_out;

	unsigned int yy_leng;
	unsigned char *yy_start;
	unsigned char *yy_text;
	unsigned char *yy_cursor;
	unsigned char *yy_marker;
	unsigned char *yy_limit;
	int yy_state;
	zend_stack state_stack;

	zend_string *filename;
	int lineno;

	/* Modes are: ZEND_INI_SCANNER_NORMAL, ZEND_INI_SCANNER_RAW, ZEND_INI_SCANNER_TYPED */
	int scanner_mode;
};

typedef enum {
	ON_TOKEN,
	ON_FEEDBACK,
	ON_STOP
} zend_php_scanner_event;

struct _zend_php_scanner_globals {
	zend_file_handle *yy_in;
	zend_file_handle *yy_out;

	unsigned int yy_leng;
	unsigned char *yy_start;
	unsigned char *yy_text;
	unsigned char *yy_cursor;
	unsigned char *yy_marker;
	unsigned char *yy_limit;
	int yy_state;
	zend_stack state_stack;
	zend_ptr_stack heredoc_label_stack;
	zend_stack nest_location_stack; /* for syntax error reporting */
	bool heredoc_scan_ahead;
	int heredoc_indentation;
	bool heredoc_indentation_uses_spaces;

	/* original (unfiltered) script */
	unsigned char *script_org;
	size_t script_org_size;

	/* filtered script */
	unsigned char *script_filtered;
	size_t script_filtered_size;

	/* input/output filters */
	zend_encoding_filter input_filter;
	zend_encoding_filter output_filter;
	const zend_encoding *script_encoding;

	/* initial string length after scanning to first variable */
	int scanned_string_len;

	/* hooks */
	void (*on_event)(
		zend_php_scanner_event event, int token, int line,
		const char *text, size_t length, void *context);
	void *on_event_context;
};

#endif /* ZEND_GLOBALS_H */



//********************* /Zend/zend_API.h ****************************************************

// 函数实体，包含5个元素
typedef struct _zend_function_entry {
	// 函数名
	const char *fname;
	// 这是个抽象函数，接收一个操作码上下文和一个返回值，两个参数
	zif_handler handler;
	
	// 参数信息指针，可以指向一个参数信息列表
	// 这种结构在打包创建实例时比较方便，
	// 参见 ZEND_RAW_FENTRY 和 zend_builtin_functions_arginfo.h 中的函数列表定义
	// 但加不加 struct 到底有什么区别呢？
	const struct _zend_internal_arg_info *arg_info;
	// 参数个数 
	uint32_t num_args;
	// 标记
	uint32_t flags;
} zend_function_entry;

// 函数调用信息
typedef struct _zend_fcall_info {
	size_t size;
	// 函数名 zval
	zval function_name;
	// 返回值指针
	zval *retval;
	// 参数列表指针
	zval *params;
	// 对象指针
	zend_object *object;
	// 参数数量
	uint32_t param_count;
	// 这个哈希表也可以用来存放位置序列参数，它们会被添加到普通的params[]j里。
	// 
	/* This hashtable can also contain positional arguments (with integer keys),
	 * which will be appended to the normal params[]. This makes it easier to
	 * integrate APIs like call_user_func_array(). The usual restriction that
	 * there may not be position arguments after named arguments applies. */
	 
	// 命名参数列表 指针
	HashTable *named_params;
} zend_fcall_info;

// 调用信息缓存 
typedef struct _zend_fcall_info_cache {
	// 函数指针
	zend_function *function_handler;
	// 所属类
	zend_class_entry *calling_scope;
	zend_class_entry *called_scope;
	zend_object *object;
} zend_fcall_info_cache;


//********************* /Zend/zend_compile.c ****************************************************


// 循环变量，用在循环堆栈里
typedef struct _zend_loop_var {
	// 操作码名
	zend_uchar opcode;
	// 变量类型
	zend_uchar var_type;
	// 变量数量
	uint32_t   var_num;
	// ？
	uint32_t   try_catch_offset;
} zend_loop_var;


// clear, 类名保留字
struct reserved_class_name {
	const char *name;
	size_t len;
};

// #define ZEND_STRL(str) (str), (sizeof(str)-1) 返回本身和长度
// clear, 保留的类名
static const struct reserved_class_name reserved_class_names[] = {
	{ZEND_STRL("bool")},
	{ZEND_STRL("false")},
	{ZEND_STRL("float")},
	{ZEND_STRL("int")},
	{ZEND_STRL("null")},
	{ZEND_STRL("parent")},
	{ZEND_STRL("self")},
	{ZEND_STRL("static")},
	{ZEND_STRL("string")},
	{ZEND_STRL("true")},
	{ZEND_STRL("void")},
	{ZEND_STRL("never")},
	{ZEND_STRL("iterable")},
	{ZEND_STRL("object")},
	{ZEND_STRL("mixed")},
	{NULL, 0}
};


// 内置类型信息
typedef struct _builtin_type_info {
	const char* name;
	const size_t name_len;
	const zend_uchar type;
} builtin_type_info;

// ing4, 内置类型列表。20种里的13种。参看 zend_types.h
static const builtin_type_info builtin_types[] = {
	{ZEND_STRL("null"), IS_NULL},
	{ZEND_STRL("true"), IS_TRUE},
	{ZEND_STRL("false"), IS_FALSE},
	{ZEND_STRL("int"), IS_LONG},
	{ZEND_STRL("float"), IS_DOUBLE},
	{ZEND_STRL("string"), IS_STRING},
	{ZEND_STRL("bool"), _IS_BOOL},
	{ZEND_STRL("void"), IS_VOID},
	{ZEND_STRL("never"), IS_NEVER},
	{ZEND_STRL("iterable"), IS_ITERABLE},
	{ZEND_STRL("object"), IS_OBJECT},
	{ZEND_STRL("mixed"), IS_MIXED},
	{NULL, 0, IS_UNDEF}
};

// ing4, 易混淆类型
typedef struct {
	const char *name;
	size_t name_len;
	const char *correct_name;
} confusable_type_info;

// ing4, 5种，resource 类型没有别名。Null 为什么也算呢？
static const confusable_type_info confusable_types[] = {
	{ZEND_STRL("boolean"), "bool"},
	{ZEND_STRL("integer"), "int"},
	{ZEND_STRL("double"), "float"},
	{ZEND_STRL("resource"), NULL},
	{NULL, 0, NULL},
};

// 闭包信息
typedef struct {
	HashTable uses;
	bool varvars_used;
} closure_info;

// 常量表达式上下文
typedef struct {
	/* Whether the value of this expression may differ on each evaluation. */
	bool allow_dynamic;
} const_expr_context;

//********************* /Zend/zend_fibers.c ****************************************************


/* Encapsulates the fiber C stack with extension for debugging tools. */
struct _zend_fiber_stack {
	void *pointer;
	size_t size;

#ifdef HAVE_VALGRIND
	unsigned int valgrind_stack_id;
#endif

#ifdef __SANITIZE_ADDRESS__
	const void *asan_pointer;
	size_t asan_size;
#endif

#ifdef ZEND_FIBER_UCONTEXT
	// 嵌入 ucontext 来避免非必要的内存分配
	/* Embedded ucontext to avoid unnecessary memory allocations. */
	ucontext_t ucontext;
#endif
};

// 在fiber上下文转换过程中， zend vm 状态需要被 捕获或 保存
/* Zend VM state that needs to be captured / restored during fiber context switch. */
typedef struct _zend_fiber_vm_state {
	zend_vm_stack vm_stack;
	zval *vm_stack_top;
	zval *vm_stack_end;
	size_t vm_stack_page_size;
	zend_execute_data *current_execute_data;
	int error_reporting;
	uint32_t jit_trace_num;
	JMP_BUF *bailout;
	zend_fiber *active_fiber;
} zend_fiber_vm_state;


typedef struct {
	void *handle;
	zend_fiber_transfer *transfer;
} boost_context_data;

//********************* /Zend/zend_modules.h ****************************************************

// 这是个不完整的结构体
struct _zend_ini_entry;
typedef struct _zend_module_entry zend_module_entry;
typedef struct _zend_module_dep zend_module_dep;

struct _zend_module_entry {
	unsigned short size;
	unsigned int zend_api;
	unsigned char zend_debug;
	unsigned char zts;
	const struct _zend_ini_entry *ini_entry;
	const struct _zend_module_dep *deps;
	const char *name;
	const struct _zend_function_entry *functions;
	zend_result (*module_startup_func)(INIT_FUNC_ARGS);
	zend_result (*module_shutdown_func)(SHUTDOWN_FUNC_ARGS);
	zend_result (*request_startup_func)(INIT_FUNC_ARGS);
	zend_result (*request_shutdown_func)(SHUTDOWN_FUNC_ARGS);
	void (*info_func)(ZEND_MODULE_INFO_FUNC_ARGS);
	const char *version;
	size_t globals_size;
#ifdef ZTS
	ts_rsrc_id* globals_id_ptr;
#else
	void* globals_ptr;
#endif
	void (*globals_ctor)(void *global);
	void (*globals_dtor)(void *global);
	zend_result (*post_deactivate_func)(void);
	int module_started;
	unsigned char type;
	void *handle;
	int module_number;
	const char *build_id;
};

struct _zend_module_dep {
	const char *name;		/* module name */
	const char *rel;		/* version relationship: NULL (exists), lt|le|eq|ge|gt (to given version) */
	const char *version;	/* version */
	unsigned char type;		/* dependency type */
};

//********************* /Zend/zend_signal.h ****************************************************


/* Signal structs */
typedef struct _zend_signal_entry_t {
	int   flags;          /* sigaction style flags */
	void* handler;      /* signal handler or context */
} zend_signal_entry_t;

typedef struct _zend_signal_t {
	int signo;
	siginfo_t *siginfo;
	void* context;
} zend_signal_t;

typedef struct _zend_signal_queue_t {
	zend_signal_t zend_signal;
	struct _zend_signal_queue_t *next;
} zend_signal_queue_t;

/* Signal Globals */
typedef struct _zend_signal_globals_t {
	int depth;
	int blocked;            /* 1==TRUE, 0==FALSE */
	int running;            /* in signal handler execution */
	int active;             /* internal signal handling is enabled */
	bool check;        /* check for replaced handlers on shutdown */
	bool reset;        /* reset signal handlers on each request */
	zend_signal_entry_t handlers[NSIG];
	zend_signal_queue_t pstorage[ZEND_SIGNAL_QUEUE_SIZE], *phead, *ptail, *pavail; /* pending queue */
} zend_signal_globals_t;


//********************* /Zend/zend_arena.h ****************************************************

typedef struct _zend_arena zend_arena;

// 自定义内存链。相关业务逻辑不是很多。
struct _zend_arena {
	// 内存开头和结束位置
	char		*ptr;
	char		*end;
	// 链表，指向前一个arena，它是个单向链
	zend_arena  *prev;
};


typedef struct _zend_arena zend_arena;

struct _zend_arena {
	void **ptr;
	void **end;
	struct _zend_arena *prev;
	void *ptrs[0];
};

//********************* /Zend/zend_ast.h ****************************************************


// 语句分3种（普通语句 zend_ast，列表型语句 zend_ast_list，声明型语句 zend_ast_decl）
// 扩展到 _zend_ast_znode，结构的前三个成员相同，可以通用，第四个不一样
// 普通语句，
struct _zend_ast {
	// 语句类型,16位整数  // typedef uint16_t zend_ast_kind;
	zend_ast_kind kind; /* Type of the node (ZEND_AST_* enum constant) */
	// 附加属性,16位整数 // typedef uint16_t zend_ast_attr;
	zend_ast_attr attr; /* Additional attribute, use depending on node type */
	// 行号
	uint32_t lineno;    /* Line number */
	// 前面一共8个字节, _zend_ast_ref 是两个32位整数，也是占8个字节，所以能兼容
	
	// 子元素，也是语句
	zend_ast *child[1]; /* Array of children (using struct hack) */
};

// 语句集合，比 zend_ast 增加了一个动态更新的children count
/* Same as zend_ast, but with children count, which is updated dynamically */
typedef struct _zend_ast_list {
	zend_ast_kind kind;
	zend_ast_attr attr;
	uint32_t lineno;
	// 最主要的，增加了子语句数量
	uint32_t children;
	zend_ast *child[1];
} zend_ast_list;

// 行号在 val变量里, 给变量添加了kind和类型，变成ast_zval
/* Lineno is stored in val.u2.lineno */
typedef struct _zend_ast_zval {
	zend_ast_kind kind;
	zend_ast_attr attr;
	zval val;
} zend_ast_zval;

// 声明型语句，当需要额外信息时，把function 和class的定义区分开，
// class 应该包含了 interface和enum?
/* Separate structure for function and class declaration, as they need extra information. */
typedef struct _zend_ast_decl {
	zend_ast_kind kind;
	// 没用到，只是为了结构兼容
	zend_ast_attr attr; /* Unused - for structure compatibility */
	// 起止行号
	uint32_t start_lineno;
	uint32_t end_lineno;
	// ?
	uint32_t flags;
	// 
	unsigned char *lex_pos;
	// 注释
	zend_string *doc_comment;
	// 类或function 的name
	zend_string *name;
	// 5个子元素
	zend_ast *child[5];
} zend_ast_decl;

typedef void (*zend_ast_process_t)(zend_ast *ast);


//********************* /Zend/zend_iterators.h ****************************************************


typedef struct _zend_object_iterator zend_object_iterator;

typedef struct _zend_object_iterator_funcs {
	/* release all resources associated with this iterator instance */
	void (*dtor)(zend_object_iterator *iter);

	/* check for end of iteration (FAILURE or SUCCESS if data is valid) */
	int (*valid)(zend_object_iterator *iter);

	/* fetch the item data for the current element */
	zval *(*get_current_data)(zend_object_iterator *iter);

	/* fetch the key for the current element (optional, may be NULL). The key
	 * should be written into the provided zval* using the ZVAL_* macros. If
	 * this handler is not provided auto-incrementing integer keys will be
	 * used. */
	void (*get_current_key)(zend_object_iterator *iter, zval *key);

	/* step forwards to next element */
	void (*move_forward)(zend_object_iterator *iter);

	/* rewind to start of data (optional, may be NULL) */
	void (*rewind)(zend_object_iterator *iter);

	/* invalidate current value/key (optional, may be NULL) */
	void (*invalidate_current)(zend_object_iterator *iter);

	/* Expose owned values to GC.
	 * This has the same semantics as the corresponding object handler. */
	HashTable *(*get_gc)(zend_object_iterator *iter, zval **table, int *n);
} zend_object_iterator_funcs;

struct _zend_object_iterator {
	zend_object std;
	// 
	zval data;
	const zend_object_iterator_funcs *funcs;
	// 指向 fe_reset/fe_fetch 的私有指针
	zend_ulong index; /* private to fe_reset/fe_fetch opcodes */
};

// 类实现【循环迭代】需要提供的方法
typedef struct _zend_class_iterator_funcs {
	zend_function *zf_new_iterator;
	zend_function *zf_valid;
	zend_function *zf_current;
	zend_function *zf_key;
	zend_function *zf_next;
	zend_function *zf_rewind;
} zend_class_iterator_funcs;

// clear, 类实现【类似数组操作】需要提供的方法
typedef struct _zend_class_arrayaccess_funcs {
	// 读取
	zend_function *zf_offsetget;
	// 检验是否存在
	zend_function *zf_offsetexists;
	// 定稿
	zend_function *zf_offsetset;
	// 删除
	zend_function *zf_offsetunset;
} zend_class_arrayaccess_funcs;


//********************* /Zend/zend_llist.h ****************************************************


// 列表元素
typedef struct _zend_llist_element {
	// 后一个元素指针
	struct _zend_llist_element *next;
	// 前一个元素指针
	struct _zend_llist_element *prev;
	// 字串数据，有必要保持作为结构体的最后一个元素（因为长度不固定）
	char data[1]; /* Needs to always be last in the struct */
} zend_llist_element;

typedef void (*llist_dtor_func_t)(void *);
typedef int (*llist_compare_func_t)(const zend_llist_element **, const zend_llist_element **);
typedef void (*llist_apply_with_args_func_t)(void *data, int num_args, va_list args);
typedef void (*llist_apply_with_arg_func_t)(void *data, void *arg);
typedef void (*llist_apply_func_t)(void *);

// 列表
typedef struct _zend_llist {
	// 头原素指针
	zend_llist_element *head;
	// 尾元素指针
	zend_llist_element *tail;
	// 元素数量
	size_t count;
	// list每个元素的数据空间 大小（元素大小是：元素基本大小 + 数据大小 -1）
	size_t size;
	// 销毁器指针
	llist_dtor_func_t dtor;
	// 永久 （这个是开辟内存时带的标记，释放内存时需要用到它）
	unsigned char persistent;
	// 游标？
	zend_llist_element *traverse_ptr;
} zend_llist;

// zend_llist_position 是指向 zend_llist_element 的指针
typedef zend_llist_element* zend_llist_position;


//********************* /Zend/zend_handlers.h ****************************************************


// 对象操作方法
struct _zend_object_handlers {
	/* offset of real object header (usually zero) */
	int										offset;
	// 操作方法
	/* object handlers */
	// 释放, 必须
	zend_object_free_obj_t					free_obj;             /* required */
	// 销毁
	zend_object_dtor_obj_t					dtor_obj;             /* required */
	// 克隆
	zend_object_clone_obj_t					clone_obj;            /* optional */
	// 读成员变量
	zend_object_read_property_t				read_property;        /* required */
	// 写成员变量
	zend_object_write_property_t			write_property;       /* required */
	// ? 尺寸？
	zend_object_read_dimension_t			read_dimension;       /* required */
	zend_object_write_dimension_t			write_dimension;      /* required */
	zend_object_get_property_ptr_ptr_t		get_property_ptr_ptr; /* required */
	// 属性是否存在
	zend_object_has_property_t				has_property;         /* required */
	// 删除属性
	zend_object_unset_property_t			unset_property;       /* required */
	// ？
	zend_object_has_dimension_t				has_dimension;        /* required */
	zend_object_unset_dimension_t			unset_dimension;      /* required */
	// 获取成员变量（多个）
	zend_object_get_properties_t			get_properties;       /* required */
	// 获取成员方法
	zend_object_get_method_t				get_method;           /* required */
	// 获取构造方法
	zend_object_get_constructor_t			get_constructor;      /* required */
	// 获取类名
	zend_object_get_class_name_t			get_class_name;       /* required */
	// 对象类型转换
	zend_object_cast_t						cast_object;          /* required */
	// 获取常量数量
	zend_object_count_elements_t			count_elements;       /* optional */
	// 获取调试信息
	zend_object_get_debug_info_t			get_debug_info;       /* optional */
	// 获取闭包？
	zend_object_get_closure_t				get_closure;          /* optional */
	// 获取引用次数
	zend_object_get_gc_t					get_gc;               /* required */
	// 执行操作
	zend_object_do_operation_t				do_operation;         /* optional */
	// 比较
	zend_object_compare_t					compare;              /* required */
	//  ?
	zend_object_get_properties_for_t		get_properties_for;   /* optional */
};

//********************* /Zend/zend_interfaces.h ****************************************************

// 用户迭代器
typedef struct _zend_user_iterator {
	// zend 迭代器
	zend_object_iterator     it;
	zend_class_entry         *ce;
	zval                     value;
} zend_user_iterator;
