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
   |          Dmitry Stogov <dmitry@php.net>                              |
   +----------------------------------------------------------------------+
*/

#ifndef ZEND_VM_OPCODES_H
#define ZEND_VM_OPCODES_H

// vm 类型
#define ZEND_VM_SPEC		1
#define ZEND_VM_LINES		0
#define ZEND_VM_KIND_CALL	1
#define ZEND_VM_KIND_SWITCH	2
#define ZEND_VM_KIND_GOTO	3
#define ZEND_VM_KIND_HYBRID	4
/* HYBRID requires support for computed GOTO and global register variables*/
// 必须要有 __GNUC__ 和 HAVE_GCC_GLOBAL_REGS 才能用 HYBRID 模式
#if (defined(__GNUC__) && defined(HAVE_GCC_GLOBAL_REGS))
# define ZEND_VM_KIND		ZEND_VM_KIND_HYBRID
#else
# define ZEND_VM_KIND		ZEND_VM_KIND_CALL
#endif

#if (ZEND_VM_KIND == ZEND_VM_KIND_HYBRID) && !defined(__SANITIZE_ADDRESS__)
# if ((defined(i386) && !defined(__PIC__)) || defined(__x86_64__) || defined(_M_X64))
#  define ZEND_VM_HYBRID_JIT_RED_ZONE_SIZE 16
# endif
#endif

#define ZEND_VM_OP_SPEC          0x00000001
#define ZEND_VM_OP_CONST         0x00000002
#define ZEND_VM_OP_TMPVAR        0x00000004
#define ZEND_VM_OP_TMPVARCV      0x00000008
#define ZEND_VM_OP_MASK          0x000000f0
#define ZEND_VM_OP_NUM           0x00000010
#define ZEND_VM_OP_JMP_ADDR      0x00000020
#define ZEND_VM_OP_TRY_CATCH     0x00000030
#define ZEND_VM_OP_THIS          0x00000050
#define ZEND_VM_OP_NEXT          0x00000060
#define ZEND_VM_OP_CLASS_FETCH   0x00000070
#define ZEND_VM_OP_CONSTRUCTOR   0x00000080
#define ZEND_VM_OP_CONST_FETCH   0x00000090
#define ZEND_VM_OP_CACHE_SLOT    0x000000a0
#define ZEND_VM_EXT_VAR_FETCH    0x00010000
#define ZEND_VM_EXT_ISSET        0x00020000
#define ZEND_VM_EXT_CACHE_SLOT   0x00040000
#define ZEND_VM_EXT_ARRAY_INIT   0x00080000
#define ZEND_VM_EXT_REF          0x00100000
#define ZEND_VM_EXT_FETCH_REF    0x00200000
#define ZEND_VM_EXT_DIM_WRITE    0x00400000
#define ZEND_VM_EXT_MASK         0x0f000000
#define ZEND_VM_EXT_NUM          0x01000000
#define ZEND_VM_EXT_LAST_CATCH   0x02000000
#define ZEND_VM_EXT_JMP_ADDR     0x03000000
#define ZEND_VM_EXT_OP           0x04000000
#define ZEND_VM_EXT_TYPE         0x07000000
#define ZEND_VM_EXT_EVAL         0x08000000
#define ZEND_VM_EXT_TYPE_MASK    0x09000000
#define ZEND_VM_EXT_SRC          0x0b000000
#define ZEND_VM_NO_CONST_CONST   0x40000000
#define ZEND_VM_COMMUTATIVE      0x80000000
#define ZEND_VM_OP1_FLAGS(flags) (flags & 0xff)
#define ZEND_VM_OP2_FLAGS(flags) ((flags >> 8) & 0xff)

BEGIN_EXTERN_C()

ZEND_API const char* ZEND_FASTCALL zend_get_opcode_name(zend_uchar opcode);
ZEND_API uint32_t ZEND_FASTCALL zend_get_opcode_flags(zend_uchar opcode);
ZEND_API zend_uchar zend_get_opcode_id(const char *name, size_t length);

END_EXTERN_C()
// 无业务逻辑
#define ZEND_NOP                          0
// 加
#define ZEND_ADD                          1
// 减
#define ZEND_SUB                          2
// 乘
#define ZEND_MUL                          3
// 除
#define ZEND_DIV                          4
// 取模
#define ZEND_MOD                          5
// 左移
#define ZEND_SL                           6
// 右移
#define ZEND_SR                           7
// 连接
#define ZEND_CONCAT                       8
// 二进制操作码
#define ZEND_BW_OR                        9
#define ZEND_BW_AND                      10
#define ZEND_BW_XOR                      11
#define ZEND_POW                         12
#define ZEND_BW_NOT                      13
// 验证 != true 或 == false
#define ZEND_BOOL_NOT                    14
#define ZEND_BOOL_XOR                    15
// ===
#define ZEND_IS_IDENTICAL                16
// !==
#define ZEND_IS_NOT_IDENTICAL            17
// ==
#define ZEND_IS_EQUAL                    18
// !=
#define ZEND_IS_NOT_EQUAL                19
// <
#define ZEND_IS_SMALLER                  20
// <=
#define ZEND_IS_SMALLER_OR_EQUAL         21
// php 变量赋值
#define ZEND_ASSIGN                      22
// 数组元素赋值
#define ZEND_ASSIGN_DIM                  23
// 对象赋值
#define ZEND_ASSIGN_OBJ                  24
// 给类的静态属性赋值
#define ZEND_ASSIGN_STATIC_PROP          25
// 简单变量 自运算赋值操作 $a += 1;
#define ZEND_ASSIGN_OP                   26
// 数组元素 自运算赋值操作
#define ZEND_ASSIGN_DIM_OP               27
// 类属性 自运算赋值操作
#define ZEND_ASSIGN_OBJ_OP               28
// 类的静态属性赋值操作
#define ZEND_ASSIGN_STATIC_PROP_OP       29
// 给变量赋值，值是引用地址
#define ZEND_ASSIGN_REF                  30
#define ZEND_QM_ASSIGN                   31
// 给对象属性赋值，值是引用地址
#define ZEND_ASSIGN_OBJ_REF              32
// 给静态属性赋值，值是引用地址
#define ZEND_ASSIGN_STATIC_PROP_REF      33
// 变量前面的 ++ 或 --
#define ZEND_PRE_INC                     34
#define ZEND_PRE_DEC                     35
// 变量后面的 ++ 或 --
#define ZEND_POST_INC                    36
#define ZEND_POST_DEC                    37
// 类的静态属性前面的 ++ 或 --
#define ZEND_PRE_INC_STATIC_PROP         38
#define ZEND_PRE_DEC_STATIC_PROP         39
// 类的静态属性后面的 ++ 或 --
#define ZEND_POST_INC_STATIC_PROP        40
#define ZEND_POST_DEC_STATIC_PROP        41
// for,while,gotu 等逻辑的跳转，跳转到指定编号的操作码 
#define ZEND_JMP                         42
#define ZEND_JMPZ                        43
#define ZEND_JMPNZ                       44
#define ZEND_JMPZ_EX                     46
#define ZEND_JMPNZ_EX                    47
// 以上几个是跳转型操作码，区别应该在虚拟机里看
#define ZEND_CASE                        48
#define ZEND_CHECK_VAR                   49
// 编译函数参数时用到
#define ZEND_SEND_VAR_NO_REF_EX          50
// 类型转换操作码
#define ZEND_CAST                        51
// 验证 == true 或!= false
#define ZEND_BOOL                        52
// concat 的两个元素都是常量的时候会触发这个 快速连接
#define ZEND_FAST_CONCAT                 53
// 准备添加引号字串中的变量
#define ZEND_ROPE_INIT                   54
// 添加引号字串中的变量
#define ZEND_ROPE_ADD                    55
// 添加引号字串中的变量结束
#define ZEND_ROPE_END                    56
// 开始静默
#define ZEND_BEGIN_SILENCE               57
// 结束静默
#define ZEND_END_SILENCE                 58
// 通过函数名 调用函数
#define ZEND_INIT_FCALL_BY_NAME          59
// 调用函数 
#define ZEND_DO_FCALL                    60

// 初始化：函数和闭包调用，方法调用，静态方法调用，new class调构造方法
#define ZEND_INIT_FCALL                  61
// return 语句等地方用到
#define ZEND_RETURN                      62
// 函数的参数处理
#define ZEND_RECV                        63
// 函数参数的默认值处理
#define ZEND_RECV_INIT                   64
// 编译函数参数时用到
#define ZEND_SEND_VAL                    65
// 编译函数参数时用到
#define ZEND_SEND_VAR_EX                 66
// 编译函数参数时用到
#define ZEND_SEND_REF                    67
// new 创建类
#define ZEND_NEW                         68
// 带命名空间的函数调用
#define ZEND_INIT_NS_FCALL_BY_NAME       69
// 释放内存
#define ZEND_FREE                        70
// 初始化数组 
#define ZEND_INIT_ARRAY                  71
// 添加数组元素
#define ZEND_ADD_ARRAY_ELEMENT           72
// include 或 eval
#define ZEND_INCLUDE_OR_EVAL             73
// 删除变量
#define ZEND_UNSET_VAR                   74
// 删除数组元素
#define ZEND_UNSET_DIM                   75
// 删除对象属性
#define ZEND_UNSET_OBJ                   76
// 只有 for 循环中创建
#define ZEND_FE_RESET_R                  77
// 只有foreach循环中创建
#define ZEND_FE_FETCH_R                  78
// exit 退出
#define ZEND_EXIT                        79
// 读取普通变量和全局变量
#define ZEND_FETCH_R                     80
// 读取数组元素
#define ZEND_FETCH_DIM_R                 81
// 读取对象属性
#define ZEND_FETCH_OBJ_R                 82
// 操作全局变量
#define ZEND_FETCH_W                     83
// 创建数组元素
#define ZEND_FETCH_DIM_W                 84
// 创建对象属性
#define ZEND_FETCH_OBJ_W                 85
// ？没有创建
#define ZEND_FETCH_RW                    86
// 更新数组元素
#define ZEND_FETCH_DIM_RW                87
// 更新对象属性
#define ZEND_FETCH_OBJ_RW                88
// ？没有创建
#define ZEND_FETCH_IS                    89
// ？没有创建
#define ZEND_FETCH_DIM_IS                90
// ？没有创建
#define ZEND_FETCH_OBJ_IS                91
// ？没有创建
#define ZEND_FETCH_FUNC_ARG              92
// ？没有创建
#define ZEND_FETCH_DIM_FUNC_ARG          93
// ？没有创建
#define ZEND_FETCH_OBJ_FUNC_ARG          94
// ？没有创建
#define ZEND_FETCH_UNSET                 95
// 删除数组元素
#define ZEND_FETCH_DIM_UNSET             96
// 删除对象 property
#define ZEND_FETCH_OBJ_UNSET             97
// ？
#define ZEND_FETCH_LIST_R                98
// 编译常量
#define ZEND_FETCH_CONSTANT              99
// ？检查函数参数
#define ZEND_CHECK_FUNC_ARG             100
// if,for循环等语句的 代码块
#define ZEND_EXT_STMT                   101
// 开始调用代码块，if,函数声名 等用到
#define ZEND_EXT_FCALL_BEGIN            102
// 结束调用代码块
#define ZEND_EXT_FCALL_END              103
// 没有创建，无业务逻辑
#define ZEND_EXT_NOP                    104
// declare(ticks=...) 语句里面的 ticks
#define ZEND_TICKS                      105
// 编译函数参数时用到
#define ZEND_SEND_VAR_NO_REF            106
// catch
#define ZEND_CATCH                      107
// throw
#define ZEND_THROW                      108
// 计算类名
#define ZEND_FETCH_CLASS                109
// 克隆对象
#define ZEND_CLONE                      110
// 返回引用地址
#define ZEND_RETURN_BY_REF              111
// 初始化类方法调用
#define ZEND_INIT_METHOD_CALL           112
// 初始化类的静态方法
#define ZEND_INIT_STATIC_METHOD_CALL    113
// 用isset 检查 $this 和 编译变量以外的 简单变量 
#define ZEND_ISSET_ISEMPTY_VAR          114
// 用isset 检查 数组元素
#define ZEND_ISSET_ISEMPTY_DIM_OBJ      115
// 编译函数参数时用到
#define ZEND_SEND_VAL_EX                116
// 编译函数参数时用到
#define ZEND_SEND_VAR                   117
// 编译初始化用户函数
#define ZEND_INIT_USER_CALL             118
// 编译用户函数时用到
#define ZEND_SEND_ARRAY                 119
// 编译用户函数时用到，读取参数
#define ZEND_SEND_USER                  120
// strlen 函数 
#define ZEND_STRLEN                     121
// defined 函数 
#define ZEND_DEFINED                    122
// 类型检查操作码
#define ZEND_TYPE_CHECK                 123
// 检查返回值类型
#define ZEND_VERIFY_RETURN_TYPE         124
// for 循环中用到
#define ZEND_FE_RESET_RW                125
// 带写入的foreach ,例如 foreach($a as &$b)
#define ZEND_FE_FETCH_RW                126
// foreach 结束 ，释放临时变量
#define ZEND_FE_FREE                    127
// 函数的动态调用（需要动态计算函数名）
#define ZEND_INIT_DYNAMIC_CALL          128
// 各种已弃用函数 ，方法的调用
#define ZEND_DO_ICALL                   129
// 各种用户定义函数 ，方法的调用
#define ZEND_DO_UCALL                   130
// 各种内置函数 ，方法的调用
#define ZEND_DO_FCALL_BY_NAME           131
// 类属性前面的 ++ 和 --
#define ZEND_PRE_INC_OBJ                132
#define ZEND_PRE_DEC_OBJ                133
// 类属性后面的 ++ 和 --
#define ZEND_POST_INC_OBJ               134
#define ZEND_POST_DEC_OBJ               135
// 打印命令
#define ZEND_ECHO                       136
// 给数组元素，对象属性赋值时用到
#define ZEND_OP_DATA                    137
// instanceof 语句
#define ZEND_INSTANCEOF                 138
// yield 语句
#define ZEND_GENERATOR_CREATE           139
// 创建原变量的引用地址 $a = &$b
#define ZEND_MAKE_REF                   140
// 声名函数 
#define ZEND_DECLARE_FUNCTION           141
// 创建闭包
#define ZEND_DECLARE_LAMBDA_FUNCTION    142
// 声名常量
#define ZEND_DECLARE_CONST              143
// 声名类
#define ZEND_DECLARE_CLASS              144
// 延时定义类
#define ZEND_DECLARE_CLASS_DELAYED      145
// 定义匿名类
#define ZEND_DECLARE_ANON_CLASS         146
// 数组里添加 解包元素
#define ZEND_ADD_ARRAY_UNPACK           147
// isset 或 empty 检查对象属性
#define ZEND_ISSET_ISEMPTY_PROP_OBJ     148
// ？ zend.c里少量创建，应该是用来显示错误信息？
#define ZEND_HANDLE_EXCEPTION           149
// ？ /Zend/zend_execute.c 里1次用到
#define ZEND_USER_OPCODE                150
// ASSERT 语句
#define ZEND_ASSERT_CHECK               151
// 没有真值的三元操作符
#define ZEND_JMP_SET                    152
// 删除普通变量
#define ZEND_UNSET_CV                   153
// isset 或 empty 检查编译变量
#define ZEND_ISSET_ISEMPTY_CV           154
// ？ zend_compile.c 几处用到
#define ZEND_FETCH_LIST_W               155
// 数组、对象属性等 辅助操作码
// 如果op1中的变量是引用，并且引用次数是1，转到引用目标（解除引用）
#define ZEND_SEPARATE                   156
// 获取类名
#define ZEND_FETCH_CLASS_NAME           157
// 调用 __get,__set,__invoke 等方法
#define ZEND_CALL_TRAMPOLINE            158
// ？ try 的 finall 语句里用到
#define ZEND_DISCARD_EXCEPTION          159
// yield 语句
#define ZEND_YIELD                      160
// 引用返回时用到，参看 zend_opcode.c 只有这里创建
#define ZEND_GENERATOR_RETURN           161
// ？ try 的 finall 语句里用到
#define ZEND_FAST_CALL                  162
// ？ try 的 finall 语句里用到
#define ZEND_FAST_RET                   163
// 处理实参，变量字典
#define ZEND_RECV_VARIADIC              164
// 处理解压参数，例如 func(a3:99,...[11,22])
#define ZEND_SEND_UNPACK                165
// ? 生成器
#define ZEND_YIELD_FROM                 166
// ？ ZEND_MEMOIZE_COMPILE 模式用到的特殊操作码
#define ZEND_COPY_TMP                   167
// 绑定全局变量 $GLOBALS
#define ZEND_BIND_GLOBAL                168
// "??" 连接的两个表达式（默认值）
#define ZEND_COALESCE                   169
// 新的比大小运算符 <=> ，大于返回1，等于返回0，小于返回-1
#define ZEND_SPACESHIP                  170
// 函数内取得参数数量, 对应 php 函数 func_num_args
#define ZEND_FUNC_NUM_ARGS              171
// 函数内取得所有参数, 对应 php 函数 func_get_args
#define ZEND_FUNC_GET_ARGS              172
// 读取类的静态属性
#define ZEND_FETCH_STATIC_PROP_R        173
// 写入类的静态属性
#define ZEND_FETCH_STATIC_PROP_W        174
// 更新类的静态属性
#define ZEND_FETCH_STATIC_PROP_RW       175
// 创建类的静态属性
#define ZEND_FETCH_STATIC_PROP_IS       176
// ？优化器用到
#define ZEND_FETCH_STATIC_PROP_FUNC_ARG 177
// ？这个是给优化器用的
#define ZEND_FETCH_STATIC_PROP_UNSET    178
// 删除类的静态属性
#define ZEND_UNSET_STATIC_PROP          179
// isset 或 empty 检查对象静态属性
#define ZEND_ISSET_ISEMPTY_STATIC_PROP  180
// 获取类常量
#define ZEND_FETCH_CLASS_CONSTANT       181
// 闭包绑定 use()里面的参数
#define ZEND_BIND_LEXICAL               182
// 声名静态变量（可以是在函数中）
#define ZEND_BIND_STATIC                183
// 从 $this 中获取
#define ZEND_FETCH_THIS                 184
// 编译函数参数时用到
#define ZEND_SEND_FUNC_ARG              185
// isset 或 empty 检查 $this
#define ZEND_ISSET_ISEMPTY_THIS         186
// 用于php switch语句，匹配整数 
#define ZEND_SWITCH_LONG                187
// 用于php switch语句，匹配字串
#define ZEND_SWITCH_STRING              188
// 查看是否在array里， php有in_array函数 
#define ZEND_IN_ARRAY                   189
// 取回数量，php有count函数 
#define ZEND_COUNT                      190
// 获取指定对象的所属类，或者当前所在类名, php 有 get_class函数 
#define ZEND_GET_CLASS                  191
// 取得调用过的class，php 有 get_called_class 函数 
#define ZEND_GET_CALLED_CLASS           192
// 取得类型， php 有 gettype函数 
#define ZEND_GET_TYPE                   193
// 检查数组里是否有key，php有array_key_exists函数
#define ZEND_ARRAY_KEY_EXISTS           194
// match语法的匹配操作
#define ZEND_MATCH                      195
// 严格匹配相等（match() 语法用到，与它相对的是 ZEND_IS_IDENTICAL，同一对象）
#define ZEND_CASE_STRICT                196
// match() 语法里的，匹配失败报错
#define ZEND_MATCH_ERROR                197
// 短路时用到的，返回NULL
#define ZEND_JMP_NULL                   198
// 处理不在形参列表里的参数
#define ZEND_CHECK_UNDEF_ARGS           199
// 从 $GLOBALS 里获取元素
#define ZEND_FETCH_GLOBALS              200
// 检查类型是否是 IS_NEVER
#define ZEND_VERIFY_NEVER_TYPE          201
// 把已有函数转换成闭包，例如 a(...)
#define ZEND_CALLABLE_CONVERT           202

// 这个是哨兵，用来标记操作码的最大值，不会被创建成操作码
#define ZEND_VM_LAST_OPCODE             202

#endif


// compile.h 里有几个伪操作码 
// goto语句
// #define ZEND_GOTO  253
// break 语句
// #define ZEND_BRK   254
// continue语句
// #define ZEND_CONT  255