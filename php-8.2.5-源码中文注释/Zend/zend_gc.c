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
   | Authors: David Wang <planetbeing@gmail.com>                          |
   |          Dmitry Stogov <dmitry@php.net>                              |
   +----------------------------------------------------------------------+
*/

// gc是垃圾回收（garbage collect） 

/*
// 从自外部调用 gc_possible_root 和 gc_check_possible_root 来看，只有想要回收的垃圾才会被添加到这个回收周期里面来
	// 使用场景1： execute时会把 可能泄露的内存 (GC_MAY_LEAK(ref)) 通过gc_possible_root 添加进来 
	
// 其实整个gc是针对数组和对象的
gc_possible_root
	// 这里规定了 【计数器】类型只能是数 IS_ARRAY 组或对象 IS_OBJECT
	新添加的元素是 紫色 GC_PURPLE 
 
 // 黑色，使用中或空闲
 // 灰色 周期中的可用成员
 // 白色 垃圾回收周期的成员
 // 紫色 回收周期中的可用 root
zend_gc_collect_cycles
	// 只有这里用到
	gc_mark_roots
		// 遍历每一个
		// 如果是普通 root，没有添加过 GC_UNUSED、GC_GARBAGE、GC_DTOR_GARBAGE
			// 如果是紫色 
				// 标记成灰色
				gc_mark_grey
					// 递归标记成灰色
		
	// 只有这里用到
	gc_scan_roots
		// 如果是普通 root，没有添加过 GC_UNUSED、GC_GARBAGE、GC_DTOR_GARBAGE
		// 如果是灰色 
			// 标记成白色
		
			// 只有这里用到
			gc_scan
				// 跳过不是白色的
				
				// 如果引用数 > 0 且 不是黑色
					// 标记成黑色
					// 只有这里用到
					gc_scan_black
						// 遍历每一个
							// 如果不是黑色 标记成黑色
							
				// 遍历每一个
					// 如果是灰色 标记成白色
				
	// 只有这里用到
	gc_collect_roots
		// 遍历每个元素
			// 如果是黑色
				// 清空 type_info
				GC_REF_SET_INFO(current->ref, 0)
				gc_remove_from_roots
	
		// 遍历每个元素
			// 添加垃圾标记, 这是把buf里的所有对象都标记成垃圾了？
			GC_MAKE_GARBAGE(ref)
			// 如果元素是白色
				// 标记成黑色
				GC_REF_SET_BLACK
				// 只有这里用到
				gc_collect_white
					如果碰到数组和对象（递归处理）
						如果没有 GC_INFO(ref) ，
							// 只有这里用到（两处）
							gc_add_garbage 标记成垃圾
								// 给指针添加 GC_GARBAGE 标记
								buf->ref = GC_MAKE_GARBAGE(ref);
						对象：增加引用次数
						遍历里面的元素列表
							每个元素 如果是白色
								标记成黑色
								增加引用次数，压入栈里
								-> 从头开始
								
					如果是引用类型（没有递归处理）
						增加目标的引用次数
						如果是白色
							标记成黑色
							-> 从头开始
						
	
	// 前后半段的线索在 GC_MAKE_GARBAGE，后面就开始回收带 GC_IS_GARBAGE 标记的对象了。
	// 
	if (gc_flags & GC_HAS_DESTRUCTORS)
	
		// 遍历每一个
			if (GC_IS_GARBAGE(current->ref))		
				GC_REF_SET_COLOR(obj, GC_PURPLE);
				// 只有这里用到
				current->ref = GC_MAKE_DTOR_GARBAGE(obj);
				
		// 遍历每一个
			if (GC_IS_DTOR_GARBAGE(current->ref))
				// 只有这里用到
				// 递归所有元素，找到黑色元素，把它和root解除关联
				gc_remove_nested_data_from_buffer
				
		// 遍历每一个
			if (GC_IS_DTOR_GARBAGE(current->ref))
				obj->handlers->dtor_obj(obj);
	
	if (GC_G(gc_protected))
		zend_get_gc_buffer_release();
	
	// 销毁 zval ，root 缓存可能被重新分配	（数组和对象的内部数据都清理掉了）	
	// 遍历每一个
		// 如果是垃圾，
		if (GC_IS_GARBAGE(current->ref))
			if (GC_TYPE(p) == IS_OBJECT) 
				// 删除对象在全局列表中的指针
				// 清空type_info最后4位（类型）
				// 给指针添加 GC_GARBAGE 标记 
				current->ref = GC_MAKE_GARBAGE(ref)
				obj->handlers->free_obj(obj);
			else if (GC_TYPE(p) == IS_ARRAY)
				// 销毁哈希表里的数据
				zend_hash_destroy(arr);
	
	// 释放对象（释放内存，这是最后步骤了）	
	// 遍历每一个
		// 如果是垃圾，计数器指针带了【垃圾】标记
		if (GC_IS_GARBAGE(current->ref))
			// 清除标记取回【计数器】指针
			p = GC_GET_PTR(current->ref)
			// root标记成可用
			GC_LINK_UNUSED(current);
			// 这里连计数器都释放了
			efree(p)

	// 
	gc_compact()
	
	
	zend_get_gc_buffer_release
	zend_gc_root_tmpvars
	return total_count;
	
	


*/

/**
 * zend_gc_collect_cycles
 * ======================
 *
 * Colors and its meaning
 * ----------------------
 *
 // 黑色，使用中或空闲
 * BLACK  (GC_BLACK)   - In use or free.
 // 灰色 可能是垃圾的成员
 * GREY   (GC_GREY)    - Possible member of cycle.
 // 白色 确定是垃圾的成员
 * WHITE  (GC_WHITE)   - Member of garbage cycle.
 
 可用的 root。
	gc_possible_root_when_full，gc_possible_root 调用，刚刚使用的root，会标记成紫色。
	有销毁器的垃圾在销毁前是紫色
 * PURPLE (GC_PURPLE)  - Possible root of cycle.
 *
 // 在文档中介绍过，但没有使用的颜色
 * Colors described in the paper but not used
 * ------------------------------------------
 *
 // 绿色 红色 橙色
 * GREEN - Acyclic
 * RED   - Candidate cycle undergoing
 * ORANGE - Candidate cycle awaiting epoch boundary.
 *
 *
 * Flow
 * =====
 *
 垃圾回收周期开始于 gc_mark_roots, 它 遍历 可用的 root，
 然后 深度优先遍历紫色节点，并对它们调用 mark_grey 
 * The garbage collect cycle starts from 'gc_mark_roots', which traverses the
 * possible roots, and calls mark_grey for roots are marked purple with
 * depth-first traverse.
 *
 
 当所有的可能 root 被 遍历 并标记后，gc_scan_roots 会被调用，每个root 会被调用 gc_scan(root->ref)
 * After all possible roots are traversed and marked,
 * gc_scan_roots will be called, and each root will be called with
 * gc_scan(root->ref)
 *
 
 gc_scan 查检可用对象的颜色。
 如果元素 是 灰色并且引用数>0 ,会对元素调用 gc_scan_black 来扫描它的子元素
 如果引用 数=0 元素会被标记成白色
 * gc_scan checks the colors of possible members.
 *
 * If the node is marked as grey and the refcount > 0
 *    gc_scan_black will be called on that node to scan it's subgraph.
 * otherwise (refcount == 0), it marks the node white.
 *
 
 删除动作发生时，会向 可用 roots 添加一个元素 或者 
 当可用垃圾元素生成时，用调用 zend_assign_to_variable
 * A node MAY be added to possible roots when ZEND_UNSET_VAR happens or
 * zend_assign_to_variable is called only when possible garbage node is
 * produced.
 
 gc_possible_root 会把元素 添加到 可用的root里
 * gc_possible_root() will be called to add the nodes to possible roots.
 *
 *
 
 对于对象类型, 调用 get_gc 方法 （默认是 zend_std_get_gc ) 来取得 对象的属性列表，用于扫描。
 * For objects, we call their get_gc handler (by default 'zend_std_get_gc') to
 * get the object properties to scan.
 *
 *
 * @see http://researcher.watson.ibm.com/researcher/files/us-bacon/Bacon01Concurrent.pdf
 */
#include "zend.h"
#include "zend_API.h"
#include "zend_fibers.h"

#ifndef GC_BENCH
# define GC_BENCH 0
#endif

#ifndef ZEND_GC_DEBUG
# define ZEND_GC_DEBUG 0
#endif

/* GC_INFO layout */
// 掩码，右面20个位。作为地址索引号，1024*1024 = 1,048,576 最多支持这么多个对象。
// GC_MAX_UNCOMPRESSED (512 * 1024)，一定要小于这个数/2 ，这样就不会出错了
// 因为压缩过的地址在 GC_MAX_UNCOMPRESSED - GC_MAX_UNCOMPRESSED*2 之间.
#define GC_ADDRESS  0x0fffffu
// 21-22位，这2个位是颜色。和上面加起来共用了 22个位。
#define GC_COLOR    0x300000u

// 黑色：必须是0
#define GC_BLACK    0x000000u /* must be zero */
// 白色
#define GC_WHITE    0x100000u
// 灰色
#define GC_GREY     0x200000u
// 紫色
#define GC_PURPLE   0x300000u

// 默认是0
/* Debug tracing */
#if ZEND_GC_DEBUG > 1
# define GC_TRACE(format, ...) fprintf(stderr, format "\n", ##__VA_ARGS__);
# define GC_TRACE_REF(ref, format, ...) \
	do { \
		gc_trace_ref((zend_refcounted *) ref); \
		fprintf(stderr, format "\n", ##__VA_ARGS__); \
	} while (0)
# define GC_TRACE_SET_COLOR(ref, color) \
	GC_TRACE_REF(ref, "->%s", gc_color_name(color))
// 非调试模式，这几个无业务逻辑
#else
// suspend
# define GC_TRACE_REF(ref, format, ...)
// suspend
# define GC_TRACE_SET_COLOR(ref, new_color)
// suspend
# define GC_TRACE(str)
#endif

/* GC_INFO access */
// #define GC_INFO_SHIFT	10。 这10位的用法见 zend_types.h
// ing4, 获取的是索引号：32位type_info的第11-30位（共20位），右边有10位空着，左边还有两位空着。
#define GC_REF_ADDRESS(ref) \
	(((GC_TYPE_INFO(ref)) & (GC_ADDRESS << GC_INFO_SHIFT)) >> GC_INFO_SHIFT)

// ing4, 获取颜色，32位type_info的31-32两个位
#define GC_REF_COLOR(ref) \
	(((GC_TYPE_INFO(ref)) & (GC_COLOR << GC_INFO_SHIFT)) >> GC_INFO_SHIFT)

// ing4, 检验是否是某一种 color，黑、灰等
#define GC_REF_CHECK_COLOR(ref, color) \
	((GC_TYPE_INFO(ref) & (GC_COLOR << GC_INFO_SHIFT)) == ((color) << GC_INFO_SHIFT))

// ing4, 设置 type_info，左边22位
#define GC_REF_SET_INFO(ref, info) do { \
		GC_TYPE_INFO(ref) = \
			/* 取出 自己的右边10位 */ \
			(GC_TYPE_INFO(ref) & (GC_TYPE_MASK | GC_FLAGS_MASK)) | \
			/* 拼接 传入的左边22位 */ \
			((info) << GC_INFO_SHIFT); \
	} while (0)

// ing4, 设置颜色，
// 要保证传入的是颜色常量才行，要不然可能写错位置，好在只有内部调用且次数不多
#define GC_REF_SET_COLOR(ref, c) do { \
		/* 调试用 */ \
		GC_TRACE_SET_COLOR(ref, c); \
		GC_TYPE_INFO(ref) = \
			/* 先取出原信息，删除颜色 */ \
			(GC_TYPE_INFO(ref) & ~(GC_COLOR << GC_INFO_SHIFT)) | \
			/* 再把颜色拼进来，这里没用掩码过滤，有可能像上面的方法一样，把前面22位都改了 */
			((c) << GC_INFO_SHIFT); \
	} while (0)

// ing4, 设置成黑色，黑色就是0
#define GC_REF_SET_BLACK(ref) do { \
		/* 调试用 */ \
		GC_TRACE_SET_COLOR(ref, GC_BLACK); \
		GC_TYPE_INFO(ref) &= ~(GC_COLOR << GC_INFO_SHIFT); \
	} while (0)

// ing4, 设置成紫色，全局无调用
#define GC_REF_SET_PURPLE(ref) do { \
		/* 调试用 */ \
		GC_TRACE_SET_COLOR(ref, GC_PURPLE); \
		GC_TYPE_INFO(ref) |= (GC_COLOR << GC_INFO_SHIFT); \
	} while (0)
// 其他颜色也这样封装一下不是更好！

// gc_root_buffer.ref 的bit 标记
/* bit stealing tags for gc_root_buffer.ref */
// gc标记位，只有内部引用，这个相当于下面两个状态的mask
#define GC_BITS    0x3
// 可用的循环垃圾根位置
#define GC_ROOT    0x0 /* possible root of circular garbage     */
// 未使用的缓冲列表的一部分, 只有 GC_IDX2LIST 添加这个标记
#define GC_UNUSED  0x1 /* part of linked list of unused buffers */
// 将要销毁的垃圾，只有 GC_MAKE_GARBAGE 添加这个标记
#define GC_GARBAGE 0x2 /* garbage to delete                     */
// 在这个垃圾上，有销毁器可以被引用， 只有 GC_MAKE_DTOR_GARBAGE 调用
#define GC_DTOR_GARBAGE 0x3 /* garbage on which only the dtor should be invoked */

// ing4, 指针转成 int 再清空最后2个位，为什么这个可用呢，这是因为所有的指针都对齐到8，最后3个bit肯定是0
// 这也太狡猾了！！
#define GC_GET_PTR(ptr) \
	((void*)(((uintptr_t)(ptr)) & ~GC_BITS))
// ing4, 是否是普通 root，没有添加过 GC_UNUSED、GC_GARBAGE、GC_DTOR_GARBAGE
#define GC_IS_ROOT(ptr) \
	((((uintptr_t)(ptr)) & GC_BITS) == GC_ROOT)
// ing4, 指针是否是【未使用】
#define GC_IS_UNUSED(ptr) \
	((((uintptr_t)(ptr)) & GC_BITS) == GC_UNUSED)
// ing4, 指针是否是【垃圾】
#define GC_IS_GARBAGE(ptr) \
	((((uintptr_t)(ptr)) & GC_BITS) == GC_GARBAGE)
// ing4, 指针是否是【待销毁垃圾】
#define GC_IS_DTOR_GARBAGE(ptr) \
	((((uintptr_t)(ptr)) & GC_BITS) == GC_DTOR_GARBAGE)
// ing4, 添加【垃圾】标记
#define GC_MAKE_GARBAGE(ptr) \
	((void*)(((uintptr_t)(ptr)) | GC_GARBAGE))
// ing4, 添加【有销毁器的垃圾】标记
#define GC_MAKE_DTOR_GARBAGE(ptr) \
	((void*)(((uintptr_t)(ptr)) | GC_DTOR_GARBAGE))

// gc地址转换
/* GC address conversion */

// 这四个是针对 gc_root_buffer
// ing4, 用索引号 计算 内存指针
#define GC_IDX2PTR(idx)      (GC_G(buf) + (idx))
// ing4, 内存指针 计算 用索引号
#define GC_PTR2IDX(ptr)      ((ptr) - GC_G(buf))

// ing4, 把 idx 编码成一个 兼容指针类型的 整数 。很牛很狡猾！
// 这个根本不是为了解压地址，是为了把 idx 左移 2或3（根据操作系统位数）位。左面空出来的2-3位可以用来作标记位。
#define GC_IDX2LIST(idx)     ((void*)(uintptr_t)(((idx) * sizeof(void*)) | GC_UNUSED))

// ing4, 把编码后的 idx 换成原来的 idx，丢掉标记位。
#define GC_LIST2IDX(list)    (((uint32_t)(uintptr_t)(list)) / sizeof(void*))
// 这个搞了好久才弄明白，重点在于一直以为它真是在作指针转换。


/* GC buffers */
// gc不可用
#define GC_INVALID           0
// 
#define GC_FIRST_ROOT        1

// gc 默认缓存大小 16K
#define GC_DEFAULT_BUF_SIZE  (16 * 1024)
// gc 缓存每次增加大小 128K
#define GC_BUF_GROW_STEP     (128 * 1024)

// gc 压缩阀值 512K ：大于它会触发压缩操作
#define GC_MAX_UNCOMPRESSED  (512 * 1024)
// gc 最大缓存大小，64M
#define GC_MAX_BUF_SIZE      0x40000000

// 默认 gc 回收临界值： 10001
#define GC_THRESHOLD_DEFAULT (10000 + GC_FIRST_ROOT)
// 临界值增加步长
#define GC_THRESHOLD_STEP    10000
// 最大回收临界值
#define GC_THRESHOLD_MAX     1000000000
// 回收触发临界值
#define GC_THRESHOLD_TRIGGER 100

/* GC flags */
// gc 有销毁器
#define GC_HAS_DESTRUCTORS  (1<<0)

/* unused buffers */
// ing4, 有使用：使用数不为0
#define GC_HAS_UNUSED() \
	(GC_G(unused) != GC_INVALID)
	
// ing4, 取得可复用的 root
#define GC_FETCH_UNUSED() \
	gc_fetch_unused()
	
// ing4, 删除一个 gc_root_buffer （并没真的删除，只是标记成未使用）
#define GC_LINK_UNUSED(root) \
	gc_link_unused(root)

// ing4, 验证使用数量是否在阀值内
#define GC_HAS_NEXT_UNUSED_UNDER_THRESHOLD() \
	(GC_G(first_unused) < GC_G(gc_threshold))
// ing4, 验证buf没满
#define GC_HAS_NEXT_UNUSED() \
	(GC_G(first_unused) != GC_G(buf_size))
// ing4, 取得下一个没使用过的 root指针
#define GC_FETCH_NEXT_UNUSED() \
	gc_fetch_next_unused()
// 默认是 zend_gc_collect_cycles
ZEND_API int (*gc_collect_cycles)(void);

// gc 根缓冲位置，里面只有一个引用计数器指针，所以它是一个指针的大小，只有内部用到
// 这样 【root的指针】 和 【root.ref的指针】 是相同的（测试过）
// 但这里存的是 zend_refcounted 指针，所以 _gc_root_buffer 和 它指向的 zend_refcounted 地址不同，这个一定要搞清楚。
typedef struct _gc_root_buffer {
	// 计数器对象指针。
		// 使用时存放 zend_refcounted 指针
		// 不使用时存放 编码后的 _gc_root_buffer 序号，多么的狡猾！
	zend_refcounted  *ref;
} gc_root_buffer;

// gc 全局变量
typedef struct _zend_gc_globals {
	// 预先分配的缓存数组 ，它指向一堆 gc_root_buffer
	gc_root_buffer   *buf;				/* preallocated arrays of buffers   */
	// gc 可用
	bool         gc_enabled;
	// gc 正在回收中，禁止嵌套GC（禁止同时运行两个回收）
	bool         gc_active;        /* GC currently running, forbid nested GC */
	// gc 保护，禁止 root 附加
	bool         gc_protected;     /* GC protected, forbid root additions */
	// gc 已满
	bool         gc_full;

	// 未使用缓存链。（它是使用过叕回收回来的 gc_root_buffer 链的开头，数字是用指针换算成的地址）
	uint32_t          unused;			/* linked list of unused buffers    */
	// 新的未使用过的 root 的索引号
	uint32_t          first_unused;		/* first unused buffer              */
	// gc 回收 阀值
	uint32_t          gc_threshold;     /* GC collection threshold          */
	// gc缓冲区 大小
	uint32_t          buf_size;			/* size of the GC buffer            */
	// 已使用的 roots 数量
	uint32_t          num_roots;		/* number of roots in GC buffer     */
	// ? 回收次数
	uint32_t gc_runs;
	// ？回收大小
	uint32_t collected;
// 默认是0 
#if GC_BENCH
	uint32_t root_buf_length;
	uint32_t root_buf_peak;
	uint32_t zval_possible_root;
	uint32_t zval_buffered;
	uint32_t zval_remove_from_buffer;
	uint32_t zval_marked_grey;
#endif

} zend_gc_globals;

// 如果要求线程安全（默认没有定义 ZTS）
#ifdef ZTS
static int gc_globals_id;
static size_t gc_globals_offset;
#define GC_G(v) ZEND_TSRMG_FAST(gc_globals_offset, zend_gc_globals *, v)
// 默认走这里
#else
// 获取GC全局变量，主要是内部使用
#define GC_G(v) (gc_globals.v)
static zend_gc_globals gc_globals;
#endif

// 默认是0 
#if GC_BENCH
# define GC_BENCH_INC(counter) GC_G(counter)++
# define GC_BENCH_DEC(counter) GC_G(counter)--
# define GC_BENCH_PEAK(peak, counter) do {		\
		if (GC_G(counter) > GC_G(peak)) {		\
			GC_G(peak) = GC_G(counter);			\
		}										\
	} while (0)
// 这里有效
#else
// suspend
# define GC_BENCH_INC(counter)
// suspend，一次调用
# define GC_BENCH_DEC(counter)
// suspend
# define GC_BENCH_PEAK(peak, counter)
#endif


// #define ZEND_MM_OVERHEAD 0
// ((4096 - ZEND_MM_OVERHEAD) / sizeof(void*)) - 2
// (4096 / sizeof(void*)) - 2
// 64位： (4096 / 8) -2 = 512-2 = 510（测试过）
// 32位： (4096 / 4) -2 = 1024-2 = 1022
// ing4, gc堆栈片尺寸
#define GC_STACK_SEGMENT_SIZE (((4096 - ZEND_MM_OVERHEAD) / sizeof(void*)) - 2)

// gc 堆栈
typedef struct _gc_stack gc_stack;

// clear, gc堆栈
// 一般深度510，前面的-2是减掉这里的 prev和next两个指针，保证使用 4096 bytes 的内存
struct _gc_stack {
	gc_stack        *prev;
	gc_stack        *next;
	// 64位 510
	zend_refcounted *data[GC_STACK_SEGMENT_SIZE];
};

// 这三个方法是一组，和别的关联不大，用于遍历过程中的堆栈处理
// ing4, 声名堆栈，不影响其他业务逻辑
#define GC_STACK_DCL(init) \
	gc_stack *_stack = init; \
	size_t    _top = 0;

// ing4,向堆栈中压入ref计数器, _top是引用返回
#define GC_STACK_PUSH(ref) \
	gc_stack_push(&_stack, &_top, ref);

// ing4,从堆栈中弹出ref计数器, _top是引用返回
#define GC_STACK_POP() \
	gc_stack_pop(&_stack, &_top)

// ing4, 取得下一个堆栈，没有则创建
static zend_never_inline gc_stack* gc_stack_next(gc_stack *stack)
{
	// 如果没有下一个位置
	if (UNEXPECTED(!stack->next)) {
		// 分配一个新的堆栈
		gc_stack *segment = emalloc(sizeof(gc_stack));
		// 关联前一个堆栈
		segment->prev = stack;
		// 下一个为空
		segment->next = NULL;
		// 前一个堆栈关联到新堆栈
		stack->next = segment;
	}
	// 可以返回next了
	return stack->next;
}

// ing4, 向堆栈里压入 zend_refcounted 指针
static zend_always_inline void gc_stack_push(gc_stack **stack, size_t *top, zend_refcounted *ref)
{
	// 如果存满了
	if (UNEXPECTED(*top == GC_STACK_SEGMENT_SIZE)) {
		// 转到下一个堆栈
		(*stack) = gc_stack_next(*stack);
		// 从第一个元素开始
		(*top) = 0;
	}
	// 把 ref 存进来，高度增加
	(*stack)->data[(*top)++] = ref;
}

// ing4, 从堆栈里弹出，这里其实没删掉，只是指针把这个位置标记成可用了。
static zend_always_inline zend_refcounted* gc_stack_pop(gc_stack **stack, size_t *top)
{
	// 如果高度是0
	if (UNEXPECTED((*top) == 0)) {
		// 找到前一个堆栈，没有则返回null
		if (!(*stack)->prev) {
			return NULL;
		// 有前一个堆栈
		} else {
			// 切换到前一个堆栈
			(*stack) = (*stack)->prev;
			// 指针指向最后一个位置
			(*top) = GC_STACK_SEGMENT_SIZE - 1;
			// 返回最后一个元素
			return (*stack)->data[GC_STACK_SEGMENT_SIZE - 1];
		}
	// 高度不是0
	} else {
		// 先左移指针，然后返回指向的元素
		return (*stack)->data[--(*top)];
	}
}

// ing3, 释放 堆栈，传入的这个stack没有被释放，后面的一串全释放了
static void gc_stack_free(gc_stack *stack)
{
	// 下一个堆栈
	gc_stack *p = stack->next;

	// 如果下一个存在
	while (p) {
		// stack指向再下一个
		stack = p->next;
		// 删除下一个
		efree(p);
		// 指向存在的
		p = stack;
	}
}

// ing3, 压缩索引号
static zend_always_inline uint32_t gc_compress(uint32_t idx)
{
	// 	#define GC_MAX_UNCOMPRESSED (512 * 1024)
	// 没到压缩尺寸，直接返回
	if (EXPECTED(idx < GC_MAX_UNCOMPRESSED)) {
		return idx;
	}
	// 大小在 GC_MAX_UNCOMPRESSED 到 GC_MAX_UNCOMPRESSED*2 之间。
	return (idx % GC_MAX_UNCOMPRESSED) | GC_MAX_UNCOMPRESSED;
}

// ing4, 解压缩索引号，得到 gc_root_buffer 指针，内部1次调用
// 传入 zend_refcounted 指针 和它所属 gc_root_buffer 的 地址索引号
static zend_always_inline gc_root_buffer* gc_decompress(zend_refcounted *ref, uint32_t idx)
{
	// buf地址
	gc_root_buffer *root = GC_IDX2PTR(idx);

	// 如果计数器 和 根地址计数器匹配
	if (EXPECTED(GC_GET_PTR(root->ref) == ref)) {
		return root;
	}
	// 如果不属于前面的 buf
	
	// ** 由于idx是压缩过的，需要像哈希表一样查找碰撞
	while (1) {
		// 每次增加 GC_MAX_UNCOMPRESSED
		idx += GC_MAX_UNCOMPRESSED;
		// 不能超过未使用的实例,只在已使用的实例中查找 
		ZEND_ASSERT(idx < GC_G(first_unused));
		// idx转成指针
		root = GC_IDX2PTR(idx);
		// 如果找到
		if (GC_GET_PTR(root->ref) == ref) {
			// 返回 这个指针
			return root;
		}
	}
	// 这里假设一定能找到了，所以连个 return NULL 也无。
}

// ing3, 找到未使用的序号
static zend_always_inline uint32_t gc_fetch_unused(void)
{
	uint32_t idx;
	gc_root_buffer *root;
	// 必须有未使用root
	ZEND_ASSERT(GC_HAS_UNUSED());
	// 第一个 unused 实例 索引号
	idx = GC_G(unused);
	// 转成指针（这里为什么没检查碰撞？）
	root = GC_IDX2PTR(idx);
	// 保证 idx指向的实例带有 IS_UNUSED 标记
	ZEND_ASSERT(GC_IS_UNUSED(root->ref));
	// 反编码指针，得到下一个可用 root的 idx 
	GC_G(unused) = GC_LIST2IDX(root->ref);
	// 返回使用数量 
	return idx;
}

// ing2, 删除一个 gc_root_buffer : 把所有删掉的 gc_root_buffer 串成一个链，最后删除的会最先复用
// gc_root_buffer 是一次创建一堆，没法一个一个删除，并且复用率很高，这是个非常棒的方法。
static zend_always_inline void gc_link_unused(gc_root_buffer *root)
{
	// 把原来的 idx 编码后 当成指针，放在 ref 对象上
	root->ref = GC_IDX2LIST(GC_G(unused));
	// 把 当前gc_root_buffer的指针换算成 索引号，放到全局变量里（这样就串成了一个链）
	GC_G(unused) = GC_PTR2IDX(root);
	// 这里没有动到 first_unused
}

// ing3, 获取一个新的buf地址索引号（不是复用的）
static zend_always_inline uint32_t gc_fetch_next_unused(void)
{
	// GC_G(first_unused) 是最右一个使用的buf的序号。
	uint32_t idx;
	// 断言 GC_G(first_unused) 不是最后一个可用buf
	ZEND_ASSERT(GC_HAS_NEXT_UNUSED());
	// 取出 GC_G(first_unused)
	idx = GC_G(first_unused);
	// 向右推一个位置
	GC_G(first_unused) = GC_G(first_unused) + 1;
	return idx;
}

// 调试模式
#if ZEND_GC_DEBUG > 1
// ing4, 获取颜色英文字串
static const char *gc_color_name(uint32_t color) {
	switch (color) {
		case GC_BLACK: return "black";
		case GC_WHITE: return "white";
		case GC_GREY: return "grey";
		case GC_PURPLE: return "purple";
		default: return "unknown";
	}
}
// suspend
static void gc_trace_ref(zend_refcounted *ref) {
	if (GC_TYPE(ref) == IS_OBJECT) {
		zend_object *obj = (zend_object *) ref;
		fprintf(stderr, "[%p] rc=%d addr=%d %s object(%s)#%d ",
			ref, GC_REFCOUNT(ref), GC_REF_ADDRESS(ref),
			gc_color_name(GC_REF_COLOR(ref)),
			obj->ce->name->val, obj->handle);
	} else if (GC_TYPE(ref) == IS_ARRAY) {
		zend_array *arr = (zend_array *) ref;
		fprintf(stderr, "[%p] rc=%d addr=%d %s array(%d) ",
			ref, GC_REFCOUNT(ref), GC_REF_ADDRESS(ref),
			gc_color_name(GC_REF_COLOR(ref)),
			zend_hash_num_elements(arr));
	} else {
		fprintf(stderr, "[%p] rc=%d addr=%d %s %s ",
			ref, GC_REFCOUNT(ref), GC_REF_ADDRESS(ref),
			gc_color_name(GC_REF_COLOR(ref)),
			GC_TYPE(ref) == IS_REFERENCE
				? "reference" : zend_get_type_by_const(GC_TYPE(ref)));
	}
}
#endif
// 以上是调试模式

// ing4, 删除一个 gc_root_buffer
static void gc_remove_from_roots(gc_root_buffer *root) {
	GC_LINK_UNUSED(root); //  删除一个 gc_root_buffer （并没真的删除，只是标记成未使用）
	GC_G(num_roots)--; // 已用 root 数量减少（root总数是不会变的）
	// 调试用
	GC_BENCH_DEC(root_buf_length);
}

// ing4, 释放整个缓冲区
static void root_buffer_dtor(zend_gc_globals *gc_globals)
{
	// 如果创建了缓冲区，释放整个缓冲区
	if (gc_globals->buf) {
		free(gc_globals->buf);
		gc_globals->buf = NULL;
	}
}

// ing3, 初始化gc全局变量，只有内部用到
static void gc_globals_ctor_ex(zend_gc_globals *gc_globals)
{
	// 不可以
	gc_globals->gc_enabled = 0;
	// 不在回收中
	gc_globals->gc_active = 0;
	// 保护中
	gc_globals->gc_protected = 1;
	// 
	gc_globals->gc_full = 0;

	// 无缓存区
	gc_globals->buf = NULL;
	// 0
	gc_globals->unused = GC_INVALID;
	// 0
	gc_globals->first_unused = GC_INVALID;
	// 0
	gc_globals->gc_threshold = GC_INVALID;
	// 0
	gc_globals->buf_size = GC_INVALID;
	//
	gc_globals->num_roots = 0;
	// 
	gc_globals->gc_runs = 0;
	//
	gc_globals->collected = 0;
	
// 默认是0 
#if GC_BENCH
	gc_globals->root_buf_length = 0;
	gc_globals->root_buf_peak = 0;
	gc_globals->zval_possible_root = 0;
	gc_globals->zval_buffered = 0;
	gc_globals->zval_remove_from_buffer = 0;
	gc_globals->zval_marked_grey = 0;
#endif
}

// ing4, 初始化gc全局变量
void gc_globals_ctor(void)
{
// 线程安全
#ifdef ZTS
	ts_allocate_fast_id(&gc_globals_id, &gc_globals_offset, sizeof(zend_gc_globals), (ts_allocate_ctor) gc_globals_ctor_ex, (ts_allocate_dtor) root_buffer_dtor);
// 其他情况
#else
	// 初始化gc全局变量
	gc_globals_ctor_ex(&gc_globals);
#endif
}

// ing4 ,什么也没有
void gc_globals_dtor(void)
{
	// 默认是0
#ifndef ZTS
	// 释放整个缓冲区
	root_buffer_dtor(&gc_globals);
#endif
}

// ing4, 重置GC全局变量
void gc_reset(void)
{
	// 如果有缓冲区，才有必要重置
	if (GC_G(buf)) {
		GC_G(gc_active) = 0;
		GC_G(gc_protected) = 0;
		GC_G(gc_full) = 0;
		GC_G(unused) = GC_INVALID;
		GC_G(first_unused) = GC_FIRST_ROOT;
		GC_G(num_roots) = 0;

		GC_G(gc_runs) = 0;
		GC_G(collected) = 0;
// 默认是0 
#if GC_BENCH
		GC_G(root_buf_length) = 0;
		GC_G(root_buf_peak) = 0;
		GC_G(zval_possible_root) = 0;
		GC_G(zval_buffered) = 0;
		GC_G(zval_remove_from_buffer) = 0;
		GC_G(zval_marked_grey) = 0;
#endif
	}
}

// ing4, 开启或关闭GC功能
ZEND_API bool gc_enable(bool enable)
{
	// 旧状态
	bool old_enabled = GC_G(gc_enabled);
	// 更新状态
	GC_G(gc_enabled) = enable;
	// 如果从关闭转到开启，并且没有分配缓存区
	if (enable && !old_enabled && GC_G(buf) == NULL) {
		// 持久分配，大小等于 8*GC_DEFAULT_BUF_SIZE(16K) 个指针的大小，128K
		GC_G(buf) = (gc_root_buffer*) pemalloc(sizeof(gc_root_buffer) * GC_DEFAULT_BUF_SIZE, 1);
		// 第一个指针 空
		GC_G(buf)[0].ref = NULL;
		// 16K
		GC_G(buf_size) = GC_DEFAULT_BUF_SIZE;
		// 10001
		GC_G(gc_threshold) = GC_THRESHOLD_DEFAULT;
		// 重置
		gc_reset();
	}
	// 返回原状态
	return old_enabled;
}

// ing4, gc是否可用
ZEND_API bool gc_enabled(void)
{
	return GC_G(gc_enabled);
}

// ing4, 开启或关闭gc保护，并返回旧状态
ZEND_API bool gc_protect(bool protect)
{
	// 旧值
	bool old_protected = GC_G(gc_protected);
	GC_G(gc_protected) = protect;
	return old_protected;
}

// ing4, 返回gc保护状态
ZEND_API bool gc_protected(void)
{
	return GC_G(gc_protected);
}

// ing4, 根缓冲区增加内存
static void gc_grow_root_buffer(void)
{
	size_t new_size;
	// 如果缓存大小超限制 
	if (GC_G(buf_size) >= GC_MAX_BUF_SIZE) {
		// 如果没有满
		if (!GC_G(gc_full)) {
			// 警告：gc缓存溢出，gc已禁用（这样就不会再增加内存开销了，旧的还可以用）
			zend_error(E_WARNING, "GC buffer overflow (GC disabled)\n");
			// gc回收中（禁止再回收）
			GC_G(gc_active) = 1;
			// gc保护开启
			GC_G(gc_protected) = 1;
			// gc已满
			GC_G(gc_full) = 1;
			return;
		}
	}
	// 如果缓存大小小于增长步长
	if (GC_G(buf_size) < GC_BUF_GROW_STEP) {
		// 直接把空间翻倍
		new_size = GC_G(buf_size) * 2;
	// 否则 ，增长一个步长的大小
	} else {
		new_size = GC_G(buf_size) + GC_BUF_GROW_STEP;
	}
	// 如果内存超限
	if (new_size > GC_MAX_BUF_SIZE) {
		// 使用最大限制值
		new_size = GC_MAX_BUF_SIZE;
	}
	// 调整缓存区大小
	GC_G(buf) = perealloc(GC_G(buf), sizeof(gc_root_buffer) * new_size, 1);
	// 更新缓存数量 
	GC_G(buf_size) = new_size;
}

// ing4, 调整 阀值，只有一处调用，传入上次回收的数量 
// 如果回收数量很少，阀值调高，降低回收频率。如果数量很多，阀值调低，增加回收频率。
// 这样就根据无效数据的比率，自动调节可用buf数量 
static void gc_adjust_threshold(int count)
{
	uint32_t new_threshold;

	/* TODO Very simple heuristic for dynamic GC buffer resizing:
	 * If there are "too few" collections, increase the collection threshold
	 * by a fixed step */
	// 如果 数量 小于 GC_THRESHOLD_TRIGGER（100）
	// 如果上次回收数量小于 100
	if (count < GC_THRESHOLD_TRIGGER) {
		// 添加
		/* increase */
		// 如果 当前 阀值小于 GC_THRESHOLD_MAX （1000000000）
		if (GC_G(gc_threshold) < GC_THRESHOLD_MAX) {
			// + 10000
			new_threshold = GC_G(gc_threshold) + GC_THRESHOLD_STEP;
			// 如果阀值大于最大可用值
			if (new_threshold > GC_THRESHOLD_MAX) {
				// 使用最大可用值
				new_threshold = GC_THRESHOLD_MAX;
			}
			// 如果阀值大于 buf 数量 ： 这种情况并没有应用新阀值
			if (new_threshold > GC_G(buf_size)) {
				// 增加buf 空间，一次增加 GC_BUF_GROW_STEP（128K）
				// 并不会因为 new_threshold 很大就增加很多
				gc_grow_root_buffer();				
			}
			// 如果 阀值 小于等于 buf 数量 
			if (new_threshold <= GC_G(buf_size)) {
				// 直接应用
				GC_G(gc_threshold) = new_threshold;
			}
		}
	// 如果上次回收数量大于 100 。如果当前阀值大于 默认阀值 （10001）
	} else if (GC_G(gc_threshold) > GC_THRESHOLD_DEFAULT) {
		// 减少 10000
		new_threshold = GC_G(gc_threshold) - GC_THRESHOLD_STEP;
		// 如果小于默认值
		if (new_threshold < GC_THRESHOLD_DEFAULT) {
			// 使用默认值
			new_threshold = GC_THRESHOLD_DEFAULT;
		}
		// 应用新的 阀值
		GC_G(gc_threshold) = new_threshold;
	}
}

// ing3, 当buf满了的时候，用这个开辟内存创建新buf（或什么也不做）
static zend_never_inline void ZEND_FASTCALL gc_possible_root_when_full(zend_refcounted *ref)
{
	uint32_t idx;
	gc_root_buffer *newRoot;
	// 保证类型是 数组 或 对象
	ZEND_ASSERT(GC_TYPE(ref) == IS_ARRAY || GC_TYPE(ref) == IS_OBJECT);
	// 保证 地址和颜色为空
	ZEND_ASSERT(GC_INFO(ref) == 0);

	// 如果 gc 已启用但现在不在回收中
	if (GC_G(gc_enabled) && !GC_G(gc_active)) {
		// 引用数 +1
		GC_ADDREF(ref);
		// gc_collect_cycles: 默认是 zend_gc_collect_cycles()
		// buf满了的时候，先回收一轮，按回收数量调整 可以buf数量 阀值
		gc_adjust_threshold(gc_collect_cycles());
		// 如果 ref 引用娄 -1后为 0
		if (UNEXPECTED(GC_DELREF(ref)) == 0) {
			// 销毁 ref：按类型调用销毁器来销毁实例， zend_variables.c
			rc_dtor_func(ref);
			return;
		// 如果 ref 还有引用，并且有 gc_info
		} else if (UNEXPECTED(GC_INFO(ref))) {
			// 什么也不做
			return;
		}
	}

	// 如果有可复用root
	if (GC_HAS_UNUSED()) {
		// 获取第一个可复用root的索引号
		idx = GC_FETCH_UNUSED();
	// 否则：如果有可用的新root
	} else if (EXPECTED(GC_HAS_NEXT_UNUSED())) {
		// 获取第一个新root的索引号
		idx = GC_FETCH_NEXT_UNUSED();
	// 其他情况：没用可复用的，也没有新的
	} else {
		// 先增加buf
		gc_grow_root_buffer();
		// 如果增加失败
		if (UNEXPECTED(!GC_HAS_NEXT_UNUSED())) {
			// 直接返回，也不报错
			return;
		}
		// 获取新root
		idx = GC_FETCH_NEXT_UNUSED();
	}
	// 新root地址
	newRoot = GC_IDX2PTR(idx);
	// 把 ref添加进去
	newRoot->ref = ref; /* GC_ROOT tag is 0 */
	// 调试用
	GC_TRACE_SET_COLOR(ref, GC_PURPLE);
	// 压缩idx
	idx = gc_compress(idx);
	// 给ref标记成紫色
	GC_REF_SET_INFO(ref, idx | GC_PURPLE);
	// root数量增加
	GC_G(num_roots)++;
	
	// 下面3个调试用
	GC_BENCH_INC(zval_buffered);
	GC_BENCH_INC(root_buf_length);
	GC_BENCH_PEAK(root_buf_peak, root_buf_length);
}

// ing3, 找一个可用的 root 把 zend_refcounted 指针放进去
ZEND_API void ZEND_FASTCALL gc_possible_root(zend_refcounted *ref)
{
	uint32_t idx;
	gc_root_buffer *newRoot;
	// 如果开启了gc保护，直接返回
	if (UNEXPECTED(GC_G(gc_protected))) {
		return;
	}
	// 调试用
	GC_BENCH_INC(zval_possible_root);

	// 如果有可复用root
	if (EXPECTED(GC_HAS_UNUSED())) {
		// 取得可利用root索引号
		idx = GC_FETCH_UNUSED();
	// 没有可复用root，有新的可用root（且数量未超阀值）
	} else if (EXPECTED(GC_HAS_NEXT_UNUSED_UNDER_THRESHOLD())) {
		// 获取下一个(新的)可用 root 的索引号
		idx = GC_FETCH_NEXT_UNUSED();
	// 如果没有可用root了
	} else {
		// 按满的处理
		gc_possible_root_when_full(ref);
		return;
	}

	// 回收类型只能是数组或对象
	ZEND_ASSERT(GC_TYPE(ref) == IS_ARRAY || GC_TYPE(ref) == IS_OBJECT);
	// 必须无附加信息
	ZEND_ASSERT(GC_INFO(ref) == 0);
	// 把 idx换成指针
	newRoot = GC_IDX2PTR(idx);
	// 给root 设置 ref。 这时指针的 附加tag是0 ？（GC_ROOT：0，GC_UNUSED：1，GC_GARBAGE：2，GC_DTOR_GARBAGE：3）
	newRoot->ref = ref; /* GC_ROOT tag is 0 */
	// 调试用
	GC_TRACE_SET_COLOR(ref, GC_PURPLE);
	
	// 压缩 索引号
	idx = gc_compress(idx);
	// 给计数器 type_info 写入 root索引号和颜色
	GC_REF_SET_INFO(ref, idx | GC_PURPLE);
	// 已使用root + 1
	GC_G(num_roots)++;
	// 下面3个调试用
	GC_BENCH_INC(zval_buffered);
	GC_BENCH_INC(root_buf_length);
	GC_BENCH_PEAK(root_buf_peak, root_buf_length);
}


// ing4, 通过压缩索引号删除 gc_root_buffer
// 传入 zend_refcounted 指针 和 gc_root_buffer 地址索引号
static zend_never_inline void ZEND_FASTCALL gc_remove_compressed(zend_refcounted *ref, uint32_t idx)
{
	// 通过 ref和idx 获取到它所属的 gc_root_buffer 
	// 搞这么麻烦，为啥不建个反向指针呢，idx是反向指针，节省了一个指针的内存，但真够累的。
	gc_root_buffer *root = gc_decompress(ref, idx);
	// 在roots中删除 一个 gc_root_buffer
	gc_remove_from_roots(root);
}

// ing4, 把计数器和root解除关联（把它所属的 root也标记成 unused），并把计数器标为黑色。实际上并没有删除什么。
ZEND_API void ZEND_FASTCALL gc_remove_from_buffer(zend_refcounted *ref)
{
	gc_root_buffer *root;
	// 取出存在 type_info 中的 gc_root_buffer 地址索引号
	uint32_t idx = GC_REF_ADDRESS(ref);
	// 调试用
	GC_BENCH_INC(zval_remove_from_buffer);
	// 如果 ref 不是黑色
	if (!GC_REF_CHECK_COLOR(ref, GC_BLACK)) {
		// 测试用
		GC_TRACE_SET_COLOR(ref, GC_BLACK);
	}
	// 清空 【地址索引号】和【颜色】
	GC_REF_SET_INFO(ref, 0);

	// 只有buffers很大时才使用压缩
	/* Perform decompression only in case of large buffers */
	// 如果大于压缩阀值
	if (UNEXPECTED(GC_G(first_unused) >= GC_MAX_UNCOMPRESSED)) {
		// 通过压缩索引号，删除计数器和它所属的 root
		// 传入 zend_refcounted 指针 和 gc_root_buffer 地址索引号
		gc_remove_compressed(ref, idx);
		return;
	}
	// idx 一定要有效
	ZEND_ASSERT(idx);
	// 把idx 转换成 gc_root_buffer 指针
	root = GC_IDX2PTR(idx);
	// 删除这个root
	gc_remove_from_roots(root);
}

/*
如果碰到数组和对象（递归处理）
	对象：增加引用次数
	遍历里面的元素列表
		每个元素如果不是黑色
			标记成黑色
			增加引用次数，压入栈里
			-> 从头开始
如果是引用类型（没有递归处理）
	增加目标的引用次数
	如果不是黑色
		标记成黑色
		-> 从头开始
处理堆栈的下一个元素
*/
// ing4，把 ref 的元素 递归标记成黑色，并增加引用次数. gc_scan 里一处调用
static void gc_scan_black(zend_refcounted *ref, gc_stack *stack)
{
	HashTable *ht;
	Bucket *p;
	zval *zv;
	uint32_t n;
	// #define GC_STACK_DCL(init) 	gc_stack *_stack = init; size_t    _top = 0;
	GC_STACK_DCL(stack);

tail_call:
	// 如果类型是
	if (GC_TYPE(ref) == IS_OBJECT) {
		// 转成对象
		zend_object *obj = (zend_object*)ref;
		// 对象没有被释放
		if (EXPECTED(!(OBJ_FLAGS(ref) & IS_OBJ_FREE_CALLED))) {
			zval *table;
			int len;
			// 四种情况 zend_weakmap_get_gc,zend_closure_get_gc,zend_fiber_object_gc,zend_generator_get_gc
			ht = obj->handlers->get_gc(obj, &table, &len);
			n = len;
			zv = table;
			// 如果哈希表有效
			if (UNEXPECTED(ht)) {
				// 增加引用次数
				GC_ADDREF(ht);
				// 如果不是黑色
				if (!GC_REF_CHECK_COLOR(ht, GC_BLACK)) {
					// 设置成黑色，黑色就是0
					GC_REF_SET_BLACK(ht);
					// 遍历每个元素
					for (; n != 0; n--) {
						// 如果可计数
						if (Z_REFCOUNTED_P(zv)) {
							// 找到计数器
							ref = Z_COUNTED_P(zv);
							// 引用次数+1
							GC_ADDREF(ref);
							// 如果颜色不是黑色
							if (!GC_REF_CHECK_COLOR(ref, GC_BLACK)) {
								// 设置成黑色，黑色就是0
								GC_REF_SET_BLACK(ref);
								// 向堆栈中压入ref计数器, _top是引用返回
								GC_STACK_PUSH(ref);
							}
						}
						// 下一个zval
						zv++;
					}
					// 处理哈希表
					goto handle_ht;
				}
			}
// 处理zval列表
handle_zvals:
			// 遍历每一个
			for (; n != 0; n--) {
				// 如果是可计数类型
				if (Z_REFCOUNTED_P(zv)) {
					// 取得计数器
					ref = Z_COUNTED_P(zv);
					// 增加引用次数
					GC_ADDREF(ref);
					// 如果颜色不是黑色
					if (!GC_REF_CHECK_COLOR(ref, GC_BLACK)) {
						// 设置成黑色，黑色就是0
						GC_REF_SET_BLACK(ref);
						// 下一个zval
						zv++;
						// 后面每一个zval
						while (--n) {
							// 如果是可计数类型
							if (Z_REFCOUNTED_P(zv)) {
								// 取得计数器
								zend_refcounted *ref = Z_COUNTED_P(zv);
								// 引用次数+1
								GC_ADDREF(ref);
								// 如果颜色不是黑色
								if (!GC_REF_CHECK_COLOR(ref, GC_BLACK)) {
									// 设置成黑色，黑色就是0
									GC_REF_SET_BLACK(ref);
									// 向堆栈中压入ref计数器, _top是引用返回
									GC_STACK_PUSH(ref);
								}
							}
							// 下一个zval
							zv++;
						}
						// 从头再来
						goto tail_call;
					}
				}
				// 下一个zval
				zv++;
			}
		}
	// 如果类型是数组 
	} else if (GC_TYPE(ref) == IS_ARRAY) {
		// 不可以是 符号表
		ZEND_ASSERT((zend_array*)ref != &EG(symbol_table));
		ht = (zend_array*)ref;
handle_ht:
		n = ht->nNumUsed;
		zv = ht->arPacked;
		// 如果类型是顺序数组 
		if (HT_IS_PACKED(ht)) {
			// 处理zval列表
			goto handle_zvals;
		}
		// buckets
		p = (Bucket*)zv;
		// 遍历每个元素
		for (; n != 0; n--) {
			zv = &p->val;
			// 如果是间接引用类型
			if (Z_TYPE_P(zv) == IS_INDIRECT) {
				// 追踪到引用类型
				zv = Z_INDIRECT_P(zv);
			}
			// 如果是可计数类型
			if (Z_REFCOUNTED_P(zv)) {
				// 取得计数器
				ref = Z_COUNTED_P(zv);
				// 增加引用次数
				GC_ADDREF(ref);
				// 如果颜色不是黑色
				if (!GC_REF_CHECK_COLOR(ref, GC_BLACK)) {
					// 设置成黑色，黑色就是0
					GC_REF_SET_BLACK(ref);
					// 下一个bucket
					p++;
					// 遍历后面每一个bucket
					while (--n) {
						// val
						zv = &p->val;
						// 如果类型是间接引用
						if (Z_TYPE_P(zv) == IS_INDIRECT) {
							// 追踪到引用对象
							zv = Z_INDIRECT_P(zv);
						}
						// 如果是可计数类型
						if (Z_REFCOUNTED_P(zv)) {
							// 取得计数器
							zend_refcounted *ref = Z_COUNTED_P(zv);
							// 增加引用次数
							GC_ADDREF(ref);
							// 如果不是黑色
							if (!GC_REF_CHECK_COLOR(ref, GC_BLACK)) {
								// 设置成黑色，黑色就是0
								GC_REF_SET_BLACK(ref);
								// 向堆栈中压入ref计数器, _top是引用返回
								GC_STACK_PUSH(ref);
							}
						}
						// 下一个bucket
						p++;
					}
					// 从头再来
					goto tail_call;
				}
			}
			// 下一个bucket
			p++;
		}
	// 如果类型是引用
	} else if (GC_TYPE(ref) == IS_REFERENCE) {
		// 追踪到目标
		if (Z_REFCOUNTED(((zend_reference*)ref)->val)) {
			// 取得计数器
			ref = Z_COUNTED(((zend_reference*)ref)->val);
			// 添加引用次数
			GC_ADDREF(ref);
			// 如果颜色 不是黑色
			if (!GC_REF_CHECK_COLOR(ref, GC_BLACK)) {
				// 设置成黑色，黑色就是0
				GC_REF_SET_BLACK(ref);
				// 从头再来
				goto tail_call;
			}
		}
	}
	// 从堆栈中弹出ref计数器, _top是引用返回
	ref = GC_STACK_POP();
	if (ref) {
		goto tail_call;
	}
}

/*
如果碰到数组和对象（递归处理）
	对象：减少引用次数
	遍历里面的元素列表
		每个元素如果不是灰色
			标记成灰色
			减少引用次数，压入栈里
			-> 从头开始
如果是引用类型（没有递归处理）
	减少目标的引用次数
	如果不是灰色
		标记成灰色
		-> 从头开始
处理堆栈的下一个元素
*/
// ing4，把 ref 的元素 递归标记成灰色. gc_mark_roots 里一处调用
static void gc_mark_grey(zend_refcounted *ref, gc_stack *stack)
{
	HashTable *ht;
	Bucket *p;
	zval *zv;
	uint32_t n;
	// #define GC_STACK_DCL(init) 	gc_stack *_stack = init; size_t    _top = 0;
	GC_STACK_DCL(stack);

tail_call:
	// 调试用
	GC_BENCH_INC(zval_marked_grey);
	// 如果对象类型是 object
	if (GC_TYPE(ref) == IS_OBJECT) {
		// 计数器转成对象
		zend_object *obj = (zend_object*)ref;
		// 如果对象没有被释放
		if (EXPECTED(!(OBJ_FLAGS(ref) & IS_OBJ_FREE_CALLED))) {
			zval *table;
			int len;
			// 四种情况 zend_weakmap_get_gc,zend_closure_get_gc,zend_fiber_object_gc,zend_generator_get_gc
			ht = obj->handlers->get_gc(obj, &table, &len);
			n = len;
			zv = table;
			// 如果哈希表有效
			if (UNEXPECTED(ht)) {
				GC_DELREF(ht);
				// 如果不是灰色
				if (!GC_REF_CHECK_COLOR(ht, GC_GREY)) {
					// 标记成灰色
					GC_REF_SET_COLOR(ht, GC_GREY);
					// 检查后面所有元素
					for (; n != 0; n--) {
						// 如果是可计数类型
						if (Z_REFCOUNTED_P(zv)) {
							// 取得计数器
							ref = Z_COUNTED_P(zv);
							// 减少引用次数
							GC_DELREF(ref);
							// 如果不是灰色
							if (!GC_REF_CHECK_COLOR(ref, GC_GREY)) {
								// 标记成灰色
								GC_REF_SET_COLOR(ref, GC_GREY);
								// 向堆栈中压入ref计数器, _top是引用返回
								GC_STACK_PUSH(ref);
							}
						}
						// 下一个 zval
						zv++;
					}
					// 处理哈希表
					goto handle_ht;
				}
			}
// 处理 zval 列表
handle_zvals:
			// 
			for (; n != 0; n--) {
				// 如果是可计数
				if (Z_REFCOUNTED_P(zv)) {
					// 取得计数器
					ref = Z_COUNTED_P(zv);
					// 减少引用次数
					GC_DELREF(ref);
					// 如果颜色不是灰色：进这个里就有可能给堆栈添加元素，所以最后是 tail_call
					if (!GC_REF_CHECK_COLOR(ref, GC_GREY)) {
						// 标记成灰色
						GC_REF_SET_COLOR(ref, GC_GREY);
						// 第一个元素为什么不入栈呢，这是因为tail_call刚好就处理这个元素，不用入栈了
						// 下一个zv
						zv++;
						// 检查后面所有元素
						while (--n) {
							// 如果是可计数类型
							if (Z_REFCOUNTED_P(zv)) {
								// 取得计数器，这个ref 是重新声名的，只对子域有效，和外面的不一样（测试过）
								zend_refcounted *ref = Z_COUNTED_P(zv);
								// 减少引用次数
								GC_DELREF(ref);
								// 如果颜色不是灰色
								if (!GC_REF_CHECK_COLOR(ref, GC_GREY)) {
									// 标记成灰色
									GC_REF_SET_COLOR(ref, GC_GREY);
									// 向堆栈中压入ref计数器, _top是引用返回
									GC_STACK_PUSH(ref);
								}
							}
							// 下一个zval
							zv++;
						}
						// 从头开始
						goto tail_call;
					}
					// 如果一直走这里，不给堆栈添加元素，也不用 tail_call
				}
				// 下一个zval
				zv++;
			}
		}
	// 如果实例类型是 array
	} else if (GC_TYPE(ref) == IS_ARRAY) {
		// 不可以是 _zend_executor_globals.symbol_table
		ZEND_ASSERT(((zend_array*)ref) != &EG(symbol_table));
		// 计数器转成 数组
		ht = (zend_array*)ref;
handle_ht:
		// 使用数量
		n = ht->nNumUsed;
		// 如果 ht 是顺序数组
		if (HT_IS_PACKED(ht)) {
			// 顺序数组开头
            zv = ht->arPacked;
			// 处理 zvals 列表
            goto handle_zvals;
		}
		// 如果 ht不是顺序数组（是哈希表）
		// 找到bucket列表
		p = ht->arData;
		// 一个一个遍历
		for (; n != 0; n--) {
			// bucket 的值
			zv = &p->val;
			// 如果是间接引用
			if (Z_TYPE_P(zv) == IS_INDIRECT) {
				// 追踪到引用目标
				zv = Z_INDIRECT_P(zv);
			}
			// 如果是可计数类型
			if (Z_REFCOUNTED_P(zv)) {
				// 取得计数器
				ref = Z_COUNTED_P(zv);
				// 减少引用数量 
				GC_DELREF(ref);
				// 如果颜色不是灰色：检查后面的所有bucket，这时候就有可能压栈，所以最后是 tail_call
				if (!GC_REF_CHECK_COLOR(ref, GC_GREY)) {
					// 标记成灰色
					GC_REF_SET_COLOR(ref, GC_GREY);
					// 第一个元素为什么不入栈呢，这是因为tail_call刚好就处理这个元素，不用入栈了
					// 下一个bucket
					p++;
					// 检查后面的所有bucket
					while (--n) {
						// 取出值
						zv = &p->val;
						// 如果类型是间接引用
						if (Z_TYPE_P(zv) == IS_INDIRECT) {
							// 取得引用值
							zv = Z_INDIRECT_P(zv);
						}
						// 如果 zv 是可计数类型
						if (Z_REFCOUNTED_P(zv)) {
							// 取得计数器，这个ref 是重新声名的，只对子域有效，和外面的不一样
							zend_refcounted *ref = Z_COUNTED_P(zv);
							// 减少引用次数
							GC_DELREF(ref);
							// 如果颜色不是灰色
							if (!GC_REF_CHECK_COLOR(ref, GC_GREY)) {
								// 标记成灰色
								GC_REF_SET_COLOR(ref, GC_GREY);
								// 向堆栈中压入ref计数器, _top是引用返回
								GC_STACK_PUSH(ref);
							}
						}
						// 下一个bucket
						p++;
					}
					// 从头开始
					goto tail_call;
				}
				// 如果一直走这里，没有压栈，就不用 tail_call
			}
			// 下一个bucket
			p++;
		}
	// 如果实例类型是引用
	} else if (GC_TYPE(ref) == IS_REFERENCE) {
		// 如果目标是可计数的类型
		if (Z_REFCOUNTED(((zend_reference*)ref)->val)) {
			// 取回目标的计数器
			ref = Z_COUNTED(((zend_reference*)ref)->val);
			// 减少引用次数
			GC_DELREF(ref);
			// 如果颜色不是灰色
			if (!GC_REF_CHECK_COLOR(ref, GC_GREY)) {
				// 标记成灰色
				GC_REF_SET_COLOR(ref, GC_GREY);
				// 紧接着就处理这个，所以不用入栈了
				// 从头开始
				goto tail_call;
			}
		}
	}
	// 从堆栈中弹出ref计数器, _top是引用返回
	ref = GC_STACK_POP();
	// 如果 stack存在
	if (ref) {
		// 从头再来
		goto tail_call;
	}
}

// ing3，整理buf。 
// 二指压缩算法：从两头同时遍历。 找到左侧的空位，把最右侧的元素转移到左侧空位里。
// 直到两个指针交叉碰撞，列表里就没有空位了。
/* Two-Finger compaction algorithm */
static void gc_compact(void)
{
	// 如果使用root数 + 第一个root编号（1） != 最右一个未使用的 buf编号 
	if (GC_G(num_roots) + GC_FIRST_ROOT != GC_G(first_unused)) {
		// 一开始是没有 GC_G(num_roots) 的
		if (GC_G(num_roots)) {
			// 第一个root的指针
			gc_root_buffer *free = GC_IDX2PTR(GC_FIRST_ROOT);
			// 扫描起始 root的指针
			gc_root_buffer *scan = GC_IDX2PTR(GC_G(first_unused) - 1);
			// 最后一个 root的指针
			gc_root_buffer *end  = GC_IDX2PTR(GC_G(num_roots));
			//
			uint32_t idx;
			zend_refcounted *p;
			// 如果 scan 在使用过的范围内
			while (free < scan) {
				// 找到左侧的空位
				while (!GC_IS_UNUSED(free->ref)) {
					free++;
				}
				// 找到右侧的有效元素
				while (GC_IS_UNUSED(scan->ref)) {
					scan--;
				}
				// 如果 scan 在free 右侧，执行
				if (scan > free) {
					// 当前计数器
					p = scan->ref;
					// 前计数器放到左侧空位里
					free->ref = p;
					// 去掉指针里的颜色标记，获得真实指针
					p = GC_GET_PTR(p);
					// 把free的指针换算成 索引号，然后压缩
					idx = gc_compress(GC_PTR2IDX(free));
					// 把压缩好的索引号 和 颜色，存放到计数器 type_info里
					GC_REF_SET_INFO(p, idx | GC_REF_COLOR(p));
					// free 指针右移 
					free++;
					// scan 指针左移
					scan--;
					// 如果两个指针交叉或碰撞，结束交换
					if (scan <= end) {
						break;
					}
				}
			}
		}
		// 这样过一轮以后，没有空闲空间了
		GC_G(unused) = GC_INVALID;
		// 更新最右侧索引号
		GC_G(first_unused) = GC_G(num_roots) + GC_FIRST_ROOT;
	}
}

// ing4,把所有紫色root标记成灰色, 返回扫描过的节点数量。
// 因为扫描前先整理过，返回的数量一定是有用root数量，并且是总数，不会少。
// 全局只有 zend_gc_collect_cycles 调用 ，stack是新创建的空临时实例，这里没用到，传给gc_mark_grey
static void gc_mark_roots(gc_stack *stack)
{
	gc_root_buffer *current, *last;

	// 先整理buf
	gc_compact();

	// #define GC_FIRST_ROOT 1
	// 从第一个开始
	current = GC_IDX2PTR(GC_FIRST_ROOT);
	// 到最后一个
	last = GC_IDX2PTR(GC_G(first_unused));
	// 遍历每一个
	while (current != last) {
		// 如果是普通 root，没有添加过 GC_UNUSED、GC_GARBAGE、GC_DTOR_GARBAGE
		if (GC_IS_ROOT(current->ref)) {
			// 如果是紫色节点: 刚刚应用上的
			if (GC_REF_CHECK_COLOR(current->ref, GC_PURPLE)) {
				// 把root标记成灰色
				GC_REF_SET_COLOR(current->ref, GC_GREY);
				// 递归标记成灰色。 
				// stack 是给ref 遍历到的下级元素用的, 这里传入的stack一定是空的
				gc_mark_grey(current->ref, stack);
			}
		}
		// 扫描过的节点数量 
		current++;
	}
}

/*
跳过不是白色的元素

如果引用数>0
	如果 不是黑色
		标记成黑色
		扫描此黑色元素
	跳到下一个
	
如果 引用数是0 ，才会走到这里	

如果碰到数组和对象（递归处理）
	遍历里面的元素列表
		每个元素如果是灰色
			标记成白色
			压入栈里
			-> 从头开始

如果是引用类型（没有递归处理）
	如果是灰色
		标记成白色
		-> 从头开始

处理堆栈的下一个元素
*/
// ing3, 把ref的 所有灰色原素标记成白色，再把所有引用次数>0的白色元素 递归标记成黑色
// 传入的原素必须是白色，gc_scan_roots 调用时保证了。 gc_scan_roots 一处调用。
// 这样做和直接把灰色标记成黑色有什么区别呢？
static void gc_scan(zend_refcounted *ref, gc_stack *stack)
{
	HashTable *ht;
	Bucket *p;
	zval *zv;
	uint32_t n;
	// #define GC_STACK_DCL(init) 	gc_stack *_stack = init; size_t    _top = 0;
	GC_STACK_DCL(stack);

// 尾调用 
tail_call:
	// 如果 ref 不是白色
	if (!GC_REF_CHECK_COLOR(ref, GC_WHITE)) {
		// 转到下一个
		goto next;
	}
	// 如果是白色
	// 如果引用数 > 0
	if (GC_REFCOUNT(ref) > 0) {
		// 如果 ref 不是黑色（如果是黑色应该在上面跳走了，这里只能是白色）
		if (!GC_REF_CHECK_COLOR(ref, GC_BLACK)) {
			// 标记成黑色
			GC_REF_SET_BLACK(ref);
			// 如果没有下一个 stack
			if (UNEXPECTED(!_stack->next)) {
				// 创建新的 stack
				gc_stack_next(_stack);
			}
			// 拆分stack并复用尾元素
			/* Split stack and reuse the tail */
			// 先把新的 stack 孤立起来（为毛？？）
			_stack->next->prev = NULL;
			// 把 ref 的元素 递归标记成黑色（这里面没有用到 stack 的 prev , next 指针）
			gc_scan_black(ref, _stack->next);
			// 再把新的 stack 指向原 stack			
			_stack->next->prev = _stack;
			// 这时候新stack应该是空的，暂时闲着（？？）
		}
		// 下一个元素
		goto next;
	}
	
	// 如果 引用数是0 ，才会走到这里，不会死循环吗？
	// 每个元素只会入栈一次出栈一次，所以不用担心死循环
	
	// 如果类型是对象
	if (GC_TYPE(ref) == IS_OBJECT) {
		// 取得对象
		zend_object *obj = (zend_object*)ref;
		// 对象没有被释放
		if (EXPECTED(!(OBJ_FLAGS(ref) & IS_OBJ_FREE_CALLED))) {
			zval *table;
			int len;
			// 四种情况 zend_weakmap_get_gc,zend_closure_get_gc,zend_fiber_object_gc,zend_generator_get_gc
			ht = obj->handlers->get_gc(obj, &table, &len);
			n = len;
			zv = table;
			// 如果ht有效
			if (UNEXPECTED(ht)) {
				// 如果颜色是灰色
				if (GC_REF_CHECK_COLOR(ht, GC_GREY)) {
					// 标记成白色
					GC_REF_SET_COLOR(ht, GC_WHITE);
					// 向堆栈中压入ref计数器, _top是引用返回
					GC_STACK_PUSH((zend_refcounted *) ht);
					// 遍历每个元素
					for (; n != 0; n--) {
						// 如果是可计数类型
						if (Z_REFCOUNTED_P(zv)) {
							// 找到计数器
							ref = Z_COUNTED_P(zv);
							// 如果颜色是灰色
							if (GC_REF_CHECK_COLOR(ref, GC_GREY)) {
								// 标记成白色
								GC_REF_SET_COLOR(ref, GC_WHITE);
								// 向堆栈中压入ref计数器, _top是引用返回
								GC_STACK_PUSH(ref);
							}
						}
						// 下一个zval
						zv++;
					}
					// 处理 哈希表
					goto handle_ht;
				}
			}
// 处理zval列表
handle_zvals:
			// 遍历每一个
			for (; n != 0; n--) {
				// 如果是可计数类型
				if (Z_REFCOUNTED_P(zv)) {
					// 找到计数器
					ref = Z_COUNTED_P(zv);
					// 如果颜色是灰色
					if (GC_REF_CHECK_COLOR(ref, GC_GREY)) {
						// 标记成白色
						GC_REF_SET_COLOR(ref, GC_WHITE);
						// 这里下面有 tail_call 所以不用压栈
						// 下一个zval
						zv++;
						// 遍历后面所有元素
						while (--n) {
							// 如果是可计数类型
							if (Z_REFCOUNTED_P(zv)) {
								// 获取计数器
								zend_refcounted *ref = Z_COUNTED_P(zv);
								// 如果颜色是灰色
								if (GC_REF_CHECK_COLOR(ref, GC_GREY)) {
									// 标记成白色
									GC_REF_SET_COLOR(ref, GC_WHITE);
									// 向堆栈中压入ref计数器, _top是引用返回
									GC_STACK_PUSH(ref);
								}
							}
							// 下一个zval
							zv++;
						}
						// 从头再来
						goto tail_call;
					}
				}
				// 下一个zval
				zv++;
			}
		}
	// 如果是数组 
	} else if (GC_TYPE(ref) == IS_ARRAY) {
		// 转成哈希表
		ht = (HashTable *)ref;
		// 不可以是符号表
		ZEND_ASSERT(ht != &EG(symbol_table));
		
handle_ht:
		// 元素数量 
		n = ht->nNumUsed;
		// 如果是顺序数组
		if (HT_IS_PACKED(ht)) {
			// 找到zval列表
            zv = ht->arPacked;
			// 处理zval列表
            goto handle_zvals;
		}
		// 找到bucket列表
		p = ht->arData;
		// 遍历 
		for (; n != 0; n--) {
			// val
			zv = &p->val;
			// 如果类型是间接引用
			if (Z_TYPE_P(zv) == IS_INDIRECT) {
				// 追踪到引用目标
				zv = Z_INDIRECT_P(zv);
			}
			// 如果是可计数类型
			if (Z_REFCOUNTED_P(zv)) {
				// 取得计数器
				ref = Z_COUNTED_P(zv);
				// 如果颜色是秋色
				if (GC_REF_CHECK_COLOR(ref, GC_GREY)) {
					// 标记成白色
					GC_REF_SET_COLOR(ref, GC_WHITE);
					// 下一个bucket
					p++;
					// 遍历后面所有bucket
					while (--n) {
						// val
						zv = &p->val;
						// 如果是间接引用
						if (Z_TYPE_P(zv) == IS_INDIRECT) {
							// 追踪到引用目标
							zv = Z_INDIRECT_P(zv);
						}
						// 如果是可计数类型
						if (Z_REFCOUNTED_P(zv)) {
							// 计数器
							zend_refcounted *ref = Z_COUNTED_P(zv);
							// 如果颜色是灰色
							if (GC_REF_CHECK_COLOR(ref, GC_GREY)) {
								// 标记成白色
								GC_REF_SET_COLOR(ref, GC_WHITE);
								// 向堆栈中压入ref计数器, _top是引用返回
								GC_STACK_PUSH(ref);
							}
						}
						// 下一个bucket
						p++;
					}
					// 从头再来
					goto tail_call;
				}
			}
			// 下一个bucket
			p++;
		}
	// 如果是引用类型
	} else if (GC_TYPE(ref) == IS_REFERENCE) {
		// 如果是可计数类型
		if (Z_REFCOUNTED(((zend_reference*)ref)->val)) {
			// 取得计数器
			ref = Z_COUNTED(((zend_reference*)ref)->val);
			// 如果是灰色
			if (GC_REF_CHECK_COLOR(ref, GC_GREY)) {
				// 标记成白色
				GC_REF_SET_COLOR(ref, GC_WHITE);
				// 从头再来
				goto tail_call;
			}
		}
	}
// 下一个root
next:
	// 从堆栈中弹出ref计数器, _top是引用返回
	ref = GC_STACK_POP();
	if (ref) {
		goto tail_call;
	}
}

// ing3, 把所有普通root里的 灰色的计数器，转成白色，并 扫描这些计数器
static void gc_scan_roots(gc_stack *stack)
{
	// 第一个root
	gc_root_buffer *current = GC_IDX2PTR(GC_FIRST_ROOT);
	// 最后一个已用的root
	gc_root_buffer *last = GC_IDX2PTR(GC_G(first_unused));

	// 遍历每一个root
	while (current != last) {
		// 是否是普通 root，没有添加过 GC_UNUSED、GC_GARBAGE、GC_DTOR_GARBAGE
		if (GC_IS_ROOT(current->ref)) {
			// 如果 root 指向的计数器 是灰色
			if (GC_REF_CHECK_COLOR(current->ref, GC_GREY)) {
				// 变成白色
				GC_REF_SET_COLOR(current->ref, GC_WHITE);
				// 扫描计数器，传入的原素一定是白色
				gc_scan(current->ref, stack);
			}
		}
		// 下一个root
		current++;
	}
}

// ing3, 找一个复用或新的 root，把ref添加进去，并标记成黑色
static void gc_add_garbage(zend_refcounted *ref)
{
	uint32_t idx;
	gc_root_buffer *buf;
	// 如果buf有可复用空间
	if (GC_HAS_UNUSED()) {
		// 取得 索引号
		idx = GC_FETCH_UNUSED();
	// 如果有新空间
	} else if (GC_HAS_NEXT_UNUSED()) {
		// 取得新空间索引号
		idx = GC_FETCH_NEXT_UNUSED();
	// 都没有
	} else {
		// 增加buf
		gc_grow_root_buffer();
		// 如果增加失败，直接返回，不报错
		if (UNEXPECTED(!GC_HAS_NEXT_UNUSED())) {
			return;
		}
		// 新buf索引号
		idx = GC_FETCH_NEXT_UNUSED();
	}

	// 通过索引号取得root地址
	buf = GC_IDX2PTR(idx);
	// 给【计数器】指针添加 GC_GARBAGE 标记
	buf->ref = GC_MAKE_GARBAGE(ref);
	// 压缩索引号
	idx = gc_compress(idx);
	// 标记成黑色
	GC_REF_SET_INFO(ref, idx | GC_BLACK);
	// 使用root数量增加
	GC_G(num_roots)++;
}

/*
如果碰到数组和对象（递归处理）
	如果没有 GC_INFO(ref) ， 
		gc_add_garbage 标记成垃圾
	对象：增加引用次数
	遍历里面的元素列表
		每个元素 如果是白色
			标记成黑色
			增加引用次数，压入栈里
			-> 从头开始
			
如果是引用类型（没有递归处理）
	增加目标的引用次数
	如果是白色
		标记成黑色
		-> 从头开始
处理堆栈的下一个元素
*/
// ing4，把stack里的白色元素 递归标记成黑色。把所有没有 GC_INFO(ref) 标记成垃圾。 gc_collect_roots 里一处调用
// flags 用于返回 GC_HAS_DESTRUCTORS 标记：被回收的对象是否自带析构方法 
// 返回所有不是引用类型的元素数量 
static int gc_collect_white(zend_refcounted *ref, uint32_t *flags, gc_stack *stack)
{
	int count = 0;
	HashTable *ht;
	Bucket *p;
	zval *zv;
	uint32_t n;
	// #define GC_STACK_DCL(init) 	gc_stack *_stack = init; size_t    _top = 0;
	GC_STACK_DCL(stack);

// 
tail_call:
	// 为了兼容性，不把引用计数
	/* don't count references for compatibility ??? */
	if (GC_TYPE(ref) != IS_REFERENCE) {
		// 记数
		count++;
	}

	// 类型是对象
	if (GC_TYPE(ref) == IS_OBJECT) {
		zend_object *obj = (zend_object*)ref;
		// 对象没有被释放
		if (EXPECTED(!(OBJ_FLAGS(ref) & IS_OBJ_FREE_CALLED))) {
			int len;
			zval *table;

			/* optimization: color is GC_BLACK (0) */
			// 如果没有 type_info
			if (!GC_INFO(ref)) {
				// 添加到垃圾列表
				gc_add_garbage(ref);
			}
			// 如果对象还没有销毁
			if (!(OBJ_FLAGS(obj) & IS_OBJ_DESTRUCTOR_CALLED)
				// 并且 类或对象有 析构方法
			 && (obj->handlers->dtor_obj != zend_objects_destroy_object
			  || obj->ce->destructor != NULL)) {
				// 添加标记：包含析构方法
				*flags |= GC_HAS_DESTRUCTORS;
			}
			// 四种情况 zend_weakmap_get_gc,zend_closure_get_gc,zend_fiber_object_gc,zend_generator_get_gc
			ht = obj->handlers->get_gc(obj, &table, &len);
			n = len;
			zv = table;
			// ht有效
			if (UNEXPECTED(ht)) {
				// 先添加引用次数
				GC_ADDREF(ht);
				// 如果是白色
				if (GC_REF_CHECK_COLOR(ht, GC_WHITE)) {
					// 设置成黑色
					GC_REF_SET_BLACK(ht);
					// 遍历 zval 列表
					for (; n != 0; n--) {
						// 如果是可计数节点
						if (Z_REFCOUNTED_P(zv)) {
							// 找到计数器
							ref = Z_COUNTED_P(zv);
							// 添加引用次数
							GC_ADDREF(ref);
							// 如果是白色
							if (GC_REF_CHECK_COLOR(ref, GC_WHITE)) {
								// 设置成黑色
								GC_REF_SET_BLACK(ref);
								// 向堆栈中压入ref计数器, _top是引用返回
								GC_STACK_PUSH(ref);
							}
						}
						// 下一个zval
						zv++;
					}
					// 处理哈希表
					goto handle_ht;
				}
			}

// 处理zval列表
handle_zvals:
			// 遍历 zval列表
			for (; n != 0; n--) {
				// 如果是可计数类型
				if (Z_REFCOUNTED_P(zv)) {
					// 计数器
					ref = Z_COUNTED_P(zv);
					// 增加计数次数
					GC_ADDREF(ref);
					// 如果是白色
					if (GC_REF_CHECK_COLOR(ref, GC_WHITE)) {
						// 标记成黑色
						GC_REF_SET_BLACK(ref);
						// 下一个zval
						zv++;
						// 遍历后面的每个zval
						while (--n) {
							// 如果是可计数类型
							if (Z_REFCOUNTED_P(zv)) {
								// 取得计数器
								zend_refcounted *ref = Z_COUNTED_P(zv);
								// 引用次数 +1
								GC_ADDREF(ref);
								// 如果是白色
								if (GC_REF_CHECK_COLOR(ref, GC_WHITE)) {
									// 标记成黑色
									GC_REF_SET_BLACK(ref);
									// 向堆栈中压入ref计数器, _top是引用返回
									GC_STACK_PUSH(ref);
								}
							}
							// 下一个zval
							zv++;
						}
						// 从头开始
						goto tail_call;
					}
				}
				// 下一个zval
				zv++;
			}
		}
	// 如果类型是数组 
	} else if (GC_TYPE(ref) == IS_ARRAY) {
		/* optimization: color is GC_BLACK (0) */
		// 如果没有 type_info
		if (!GC_INFO(ref)) {
			// 添加垃圾
			gc_add_garbage(ref);
		}
		// 类型转成数组 
		ht = (zend_array*)ref;

handle_ht:
		// 数组元素数量 
		n = ht->nNumUsed;
		// 如果是顺序数组
		if (HT_IS_PACKED(ht)) {
			// 找到zval列表
			zv = ht->arPacked;
			// 处理列表
			goto handle_zvals;
		}

		// 找到buckets
		p = ht->arData;
		// 遍历 
		for (; n != 0; n--) {
			// val
			zv = &p->val;
			// 如果是间接引用
			if (Z_TYPE_P(zv) == IS_INDIRECT) {
				// 转到引用目标
				zv = Z_INDIRECT_P(zv);
			}
			// 如果是可计数类型
			if (Z_REFCOUNTED_P(zv)) {
				// 找到记数器
				ref = Z_COUNTED_P(zv);
				// 引用次数+1
				GC_ADDREF(ref);
				// 如果颜色是白色
				if (GC_REF_CHECK_COLOR(ref, GC_WHITE)) {
					// 标记成黑色
					GC_REF_SET_BLACK(ref);
					// 下一个bucket
					p++;
					// 遍历后面所有bucket
					while (--n) {
						// val
						zv = &p->val;
						// 如果是间接引用
						if (Z_TYPE_P(zv) == IS_INDIRECT) {
							// 追踪到引用目标
							zv = Z_INDIRECT_P(zv);
						}
						// 如果是可计数类型
						if (Z_REFCOUNTED_P(zv)) {
							// 找到记数器
							zend_refcounted *ref = Z_COUNTED_P(zv);
							// 增加引用次数
							GC_ADDREF(ref);
							// 如果颜色是白色
							if (GC_REF_CHECK_COLOR(ref, GC_WHITE)) {
								// 标记成黑色
								GC_REF_SET_BLACK(ref);
								// 向堆栈中压入ref计数器, _top是引用返回
								GC_STACK_PUSH(ref);
							}
						}
						// 下一个bucket
						p++;
					}
					// 从头开始
					goto tail_call;
				}
			}
			// 下一个bucket
			p++;
		}
	// 如果类型是引用
	} else if (GC_TYPE(ref) == IS_REFERENCE) {
		// 如果引用目标可计数
		if (Z_REFCOUNTED(((zend_reference*)ref)->val)) {
			// 找到计数器
			ref = Z_COUNTED(((zend_reference*)ref)->val);
			// 增加引用计数
			GC_ADDREF(ref);
			// 如果颜色是白色
			if (GC_REF_CHECK_COLOR(ref, GC_WHITE)) {
				// 标记成黑色
				GC_REF_SET_BLACK(ref);
				// 重来一次
				goto tail_call;
			}
		}
	}
	// 从堆栈中弹出ref计数器, _top是引用返回
	ref = GC_STACK_POP();
	if (ref) {
		goto tail_call;
	}

	return count;
}

/*
遍历所有元素
	把黑色元素的 type_info 清空，并回收所在root
	
整理后，再遍历所有元素（黑色的已经清除掉了）
	给每个计数器的 ref 指针添加 GC_GARBAGE（垃圾） 标记
	如果是白色节点
		标记成黑色
		gc_collect_white 递归标记成黑色，并计数
返回计数
*/

// ing3，
// flags 用于返回 GC_HAS_DESTRUCTORS 标记： 被回收的对象是否自带析构方法 
// 返回所有回收的非引用类型元素数量 ，只有zend_gc_collect_cycles调用
static int gc_collect_roots(uint32_t *flags, gc_stack *stack)
{
	uint32_t idx, end;
	zend_refcounted *ref;
	int count = 0;
	// 第一个 root
	gc_root_buffer *current = GC_IDX2PTR(GC_FIRST_ROOT);
	// 最后一个 root
	gc_root_buffer *last = GC_IDX2PTR(GC_G(first_unused));

	// 从列表中删除非垃圾元素
	/* remove non-garbage from the list */
	// 遍历每一个
	while (current != last) {
		// 如果是否是普通 root，没有添加过 GC_UNUSED、GC_GARBAGE、GC_DTOR_GARBAGE
		if (GC_IS_ROOT(current->ref)) {
			// 如果是黑色
			if (GC_REF_CHECK_COLOR(current->ref, GC_BLACK)) {
				// 清空 type_info 左边22位
				GC_REF_SET_INFO(current->ref, 0); /* reset GC_ADDRESS() and keep GC_BLACK */
				// 回收一个root
				gc_remove_from_roots(current);
			}
		}
		// 下一个
		current++;
	}
	// 回收完后，整理一下
	gc_compact();

	/* Root buffer might be reallocated during gc_collect_white,
	 * make sure to reload pointers. */
	// 第一个
	idx = GC_FIRST_ROOT;
	// 最后一个
	end = GC_G(first_unused);
	// 遍历 
	while (idx != end) {
		// 取得 root 指针
		current = GC_IDX2PTR(idx);
		// root 连接的计数器
		ref = current->ref;
		// 所有指针必须是普通 root，没有添加过 GC_UNUSED、GC_GARBAGE、GC_DTOR_GARBAGE
		ZEND_ASSERT(GC_IS_ROOT(ref));
		// 给【计数器】指针添加 GC_GARBAGE 标记, 这是把buf里的所有对象都标记成垃圾了？
		current->ref = GC_MAKE_GARBAGE(ref);
		// 如果是白色节点
		if (GC_REF_CHECK_COLOR(ref, GC_WHITE)) {
			// 标记成黑色
			GC_REF_SET_BLACK(ref);
			// 把stack里的白色元素 递归标记成黑色。把所有没有 GC_INFO(ref) 标记成垃圾。（返回所有不是引用类型的元素数量 ）
			count += gc_collect_white(ref, flags, stack);
		}
		// 下个buf
		idx++;
	}

	// 返回所有回收的非引用类型元素数量 
	return count;
}


/*
如果有传入root
	root=null
	数量+1， 这里相当于对所有的元素递归计数
	
否则 如果 ref 是黑色
	** 把计数器 ref 和root解除关联
	数量+1
	
否则 如果是引用类型
	转到目标
	从头开始
其他情况
	跳过

能到这里的一定是 有传入root 或 ref 是黑色

如果碰到数组和对象（递归处理）
	如果是对象：把属性哈希表 和 root 解除关联
	遍历里面的元素列表
		压入栈里
		-> 从头开始

处理堆栈的下一个元素
*/
// 这里明明可以用递归的偏要用堆栈，也是为了速度更快咯？
// 从buffer里删除嵌套的数据,zend_gc_collect_cycles 一处调用
// ing3, 递归所有元素，找到黑色元素，把它和root解除关联
static int gc_remove_nested_data_from_buffer(zend_refcounted *ref, gc_root_buffer *root, gc_stack *stack)
{
	HashTable *ht;
	Bucket *p;
	zval *zv;
	uint32_t n;
	int count = 0;
	// #define GC_STACK_DCL(init) 	gc_stack *_stack = init; size_t    _top = 0;
	GC_STACK_DCL(stack);

tail_call:
	// 如果root有效，这里相当于对所有的元素递归计数
	if (root) {
		// 置空?
		root = NULL;
		// 数量 +1
		count++;
	// 如果 ref 里有 root 地址
	} else if (GC_REF_ADDRESS(ref) != 0
	// 并且颜色是黑色
	 && GC_REF_CHECK_COLOR(ref, GC_BLACK)) {
		// 调试用
		GC_TRACE_REF(ref, "removing from buffer");
		// gc_remove_from_buffer : 把计数器和root解除关联（把它所属的 root也标记成 unused），并把计数器标为黑色
		GC_REMOVE_FROM_BUFFER(ref);
		// 数量 +1
		count++;
	// 如果是引用类型
	} else if (GC_TYPE(ref) == IS_REFERENCE) {
		// 如果目标是可计数类型
		if (Z_REFCOUNTED(((zend_reference*)ref)->val)) {
			// 转到目标
			ref = Z_COUNTED(((zend_reference*)ref)->val);
			// 从头开始
			goto tail_call;
		}
		// 如果不是可计数类型，下一个
		goto next;
	// 其他情况，下一个
	} else {
		goto next;
	}
	
	// 能到这里的一定是 有传入root 或 引用次数为0 或  ref不隶属于黑色的root
	// 每个元素只会入栈一次出栈一次，所以不用担心死循环
	
	// 如果类型是对象
	if (GC_TYPE(ref) == IS_OBJECT) {
		// 转成对象
		zend_object *obj = (zend_object*)ref;
		// 如果对象没有被释放过
		if (EXPECTED(!(OBJ_FLAGS(ref) & IS_OBJ_FREE_CALLED))) {
			int len;
			zval *table;
			// 四种情况 zend_weakmap_get_gc,zend_closure_get_gc,zend_fiber_object_gc,zend_generator_get_gc
			ht = obj->handlers->get_gc(obj, &table, &len);
			n = len;
			zv = table;
			// 如果哈希表存在
			if (UNEXPECTED(ht)) {
				// 遍历每一个zval
				for (; n != 0; n--) {
					// 如果是可计数类型
					if (Z_REFCOUNTED_P(zv)) {
						// 找到 zv的计数器
						ref = Z_COUNTED_P(zv);
						// 向堆栈中压入ref计数器, _top是引用返回
						GC_STACK_PUSH(ref);
					}
					// 下一个zv
					zv++;
				}
				// 如果 ht 指向的 root 不为空，并且 ht的颜色是黑色
				if (GC_REF_ADDRESS(ht) != 0 && GC_REF_CHECK_COLOR(ht, GC_BLACK)) {
					GC_TRACE_REF(ht, "removing from buffer");
					// gc_remove_from_buffer，把ht和root 解除关联，并把计数器标为黑色
					GC_REMOVE_FROM_BUFFER(ht);
				}
				// 处理哈希表
				goto handle_ht;
			}
// 处理zval列表
handle_zvals:
			// 遍历每一个zval
			for (; n != 0; n--) {
				// 如果是可计数类型
				if (Z_REFCOUNTED_P(zv)) {
					// 取得计数器
					ref = Z_COUNTED_P(zv);
					// 下一个zval
					zv++;
					// 遍历后面的每一个zval
					while (--n) {
						if (Z_REFCOUNTED_P(zv)) {
							// 取得计数器
							zend_refcounted *ref = Z_COUNTED_P(zv);
							// 向堆栈中压入ref计数器, _top是引用返回
							GC_STACK_PUSH(ref);
						}
						// 下一个zval
						zv++;
					}
					// 从头开始
					goto tail_call;
				}
				// 下一个zval
				zv++;
			}
		}
	// 如果类型是数组 
	} else if (GC_TYPE(ref) == IS_ARRAY) {
		// 转成array指针
		ht = (zend_array*)ref;
// 处理哈希表
handle_ht:
		n = ht->nNumUsed;
		// 如果是顺序数组
		if (HT_IS_PACKED(ht)) {
			// 取得元素列表开头
			zv = ht->arPacked;
			// 按zval列表处理
			goto handle_zvals;
		}
		// 如果不是顺序数组 
		// 取得buckets
		p = ht->arData;
		// 遍历
		for (; n != 0; n--) {
			zv = &p->val;
			// 如果类型是间接引用
			if (Z_TYPE_P(zv) == IS_INDIRECT) {
				// 追踪到引用目标
				zv = Z_INDIRECT_P(zv);
			}
			// 如果是可计数类型
			if (Z_REFCOUNTED_P(zv)) {
				// 找到计数器
				ref = Z_COUNTED_P(zv);
				// 下一个bucket
				p++;
				// 遍历后面所有
				while (--n) {
					// 取得val
					zv = &p->val;
					// 如果是间接引用
					if (Z_TYPE_P(zv) == IS_INDIRECT) {
						// 追踪到引用地址
						zv = Z_INDIRECT_P(zv);
					}
					// 如果是可计数类型
					if (Z_REFCOUNTED_P(zv)) {
						// 获得计数器
						zend_refcounted *ref = Z_COUNTED_P(zv);
						// 向堆栈中压入ref计数器, _top是引用返回
						GC_STACK_PUSH(ref);
					}
					// 下一个bucket
					p++;
				}
				// 从头开始
				goto tail_call;
			}
			// 下一个bucket
			p++;
		}
	}

next:
	// 从堆栈中弹出ref计数器, _top是引用返回
	ref = GC_STACK_POP();
	// 如果还有stack
	if (ref) {
		// 从头再来
		goto tail_call;
	}
	// 返回数量 
	return count;
}

static void zend_get_gc_buffer_release(void);
static void zend_gc_root_tmpvars(void);

// 回收周期
/*
	gc_possible_root_when_full->zend_gc_collect_cycles
		->gc_mark_roots->gc_mark_grey
		->gc_scan_roots->gc_scan->gc_scan_black
		->gc_collect_roots->gc_collect_white
		->gc_remove_nested_data_from_buffer
*/
ZEND_API int zend_gc_collect_cycles(void)
{
	int total_count = 0;
	bool should_rerun_gc = 0;
	bool did_rerun_gc = 0;

// 重复回收
rerun_gc:
	// 如果有root数量 
	if (GC_G(num_roots)) {
		int count;
		gc_root_buffer *current, *last;
		zend_refcounted *p;
		uint32_t gc_flags = 0;
		uint32_t idx, end;
		// 临时堆栈
		gc_stack stack;

		stack.prev = NULL;
		stack.next = NULL;
		// 如果 正在 回收中
		if (GC_G(gc_active)) {
			// 返回0
			return 0;
		}
		//
		GC_TRACE("Collecting cycles");
		// 回收次数 +1
		GC_G(gc_runs)++;
		// 状态更新成回收中
		GC_G(gc_active) = 1;

		GC_TRACE("Marking roots");
		// stack是刚创建的空临时对象,唯一一个调用
		// 把 ref 的元素 递归标记成灰色
		gc_mark_roots(&stack);
		GC_TRACE("Scanning roots");
		// 把ref的 所有灰色原素标记成白色，再把所有引用次数>0的白色元素 递归标记成黑色
		gc_scan_roots(&stack);

		GC_TRACE("Collecting roots");
		// 把黑色元素的 type_info 清空，并回收所在root
		// 给每个指针添加 垃圾 标记
		count = gc_collect_roots(&gc_flags, &stack);
		// 如果没有root在用
		if (!GC_G(num_roots)) {
			/* nothing to free */
			GC_TRACE("Nothing to free");
			// 释放stack
			gc_stack_free(&stack);
			// 完成回收，状态：不在回收中
			GC_G(gc_active) = 0;
			// 完成
			goto finish;
		}
		// ？
		zend_fiber_switch_block();
		// 最后一个已用原素 
		end = GC_G(first_unused);
		// 如果有析构方法，这里才是真的开始销毁对象，释放内存
		if (gc_flags & GC_HAS_DESTRUCTORS) {
			GC_TRACE("Calling destructors");

			/* During a destructor call, new externally visible references to nested data may
			 * be introduced. These references can be introduced in a way that does not
			 * modify any refcounts, so we have no real way to detect this situation
			 * short of rerunning full GC tracing. What we do instead is to only run
			 * destructors at this point and automatically re-run GC afterwards. */
			// 只要有析构方法就要重来
			should_rerun_gc = 1;

			/* Mark all roots for which a dtor will be invoked as DTOR_GARBAGE. Additionally
			 * color them purple. This serves a double purpose: First, they should be
			 * considered new potential roots for the next GC run. Second, it will prevent
			 * their removal from the root buffer by nested data removal. */
			// 第一个元素索引号
			idx = GC_FIRST_ROOT;
			// 第一个元素位置
			current = GC_IDX2PTR(GC_FIRST_ROOT);
			// 遍历每一个
			while (idx != end) {
				// 如果是垃圾
				if (GC_IS_GARBAGE(current->ref)) {
					// 取得 root 指针
					p = GC_GET_PTR(current->ref);
					//  如果是对象 并且 对象没有销毁
					if (GC_TYPE(p) == IS_OBJECT && !(OBJ_FLAGS(p) & IS_OBJ_DESTRUCTOR_CALLED)) {
						// 找到对象
						zend_object *obj = (zend_object *) p;
						// 如果有自己的或者所属类的析构方法
						if (obj->handlers->dtor_obj != zend_objects_destroy_object
							|| obj->ce->destructor) {
							// 添加 GC_DTOR_GARBAGE 标记
							current->ref = GC_MAKE_DTOR_GARBAGE(obj);
							// 颜色标记成紫色
							GC_REF_SET_COLOR(obj, GC_PURPLE);
						} else {
							// 对象已销毁
							GC_ADD_FLAGS(obj, IS_OBJ_DESTRUCTOR_CALLED);
						}
					}
				}
				// 下一个
				current++;
				// 序号下一个
				idx++;
			}

			// 删除将要 销毁的对象的 嵌套数据，并不会删除对象本身，它们已经被标记成紫色。 
			/* Remove nested data for objects on which a destructor will be called.
			 * This will not remove the objects themselves, as they have been colored
			 * purple. */
			// 第一个root的索引号
			idx = GC_FIRST_ROOT;
			// 第一个root的指针
			current = GC_IDX2PTR(GC_FIRST_ROOT);
			// 遍历每一个
			while (idx != end) {
				// 如果等待销毁 
				if (GC_IS_DTOR_GARBAGE(current->ref)) {
					// 取得root指针
					p = GC_GET_PTR(current->ref);
					// 递归所有元素，找到黑色元素，把它和root解除关联
					count -= gc_remove_nested_data_from_buffer(p, current, &stack);
				}
				// 下一个root
				current++;
				// 下一个索引号
				idx++;
			}

			// 真正开始销毁 
			// 在销毁过程中 root
			/* Actually call destructors.
			 *
			 * The root buffer might be reallocated during destructors calls,
			 * make sure to reload pointers as necessary. */
			// 从第一个开始
			idx = GC_FIRST_ROOT;
			// 遍历每一个
			while (idx != end) {
				// root指针
				current = GC_IDX2PTR(idx);
				// 如果 是 等待销毁 
				if (GC_IS_DTOR_GARBAGE(current->ref)) {
					// 取得指针
					p = GC_GET_PTR(current->ref);
					// 
					/* Mark this is as a normal root for the next GC run,
					 * it's no longer garbage for this run. */
					// 当前root 指向 这个root ? 
					current->ref = p;
					/* Double check that the destructor hasn't been called yet. It could have
					 * already been invoked indirectly by some other destructor. */
					 // 对象未销毁
					if (!(OBJ_FLAGS(p) & IS_OBJ_DESTRUCTOR_CALLED)) {
						zend_object *obj = (zend_object*)p;
						GC_TRACE_REF(obj, "calling destructor");
						// 标记成对象已销毁
						GC_ADD_FLAGS(obj, IS_OBJ_DESTRUCTOR_CALLED);
						// 增加引用次数
						GC_ADDREF(obj);
						// 调用销毁方法，销毁对象。dtor_obj 和 free_obj 有什么区别
						obj->handlers->dtor_obj(obj);
						// 减少引用次数
						GC_DELREF(obj);
					}
				}
				// 下一个root  
				idx++;
			}
			// 如果是受保护状态
			if (GC_G(gc_protected)) {
				/* something went wrong */
				// 释放 zval 列表，但不释放 _zend_executor_globals
				zend_get_gc_buffer_release();
				// ?
				zend_fiber_switch_unblock();
				return 0;
			}
		}
		// 释放 stack
		gc_stack_free(&stack);

		// 销毁 zval ，root 缓存可能被重新分配
		/* Destroy zvals. The root buffer may be reallocated. */
		GC_TRACE("Destroying zvals");
		// 从第一个开始
		idx = GC_FIRST_ROOT;
		// 遍历每一个
		while (idx != end) {
			// 取得root指针
			current = GC_IDX2PTR(idx);
			// 如果是垃圾
			if (GC_IS_GARBAGE(current->ref)) {
				// 找到对象指针
				p = GC_GET_PTR(current->ref);
				GC_TRACE_REF(p, "destroying");
				// 如果类型是 对象， 这个分支比较复杂
				if (GC_TYPE(p) == IS_OBJECT) {
					// 转成对象
					zend_object *obj = (zend_object*)p;
					// 删除对象在全局列表中的指针
					EG(objects_store).object_buckets[obj->handle] = SET_OBJ_INVALID(obj);
					// 清空type_info最后4位（类型）
					GC_TYPE_INFO(obj) = GC_NULL |
						(GC_TYPE_INFO(obj) & ~GC_TYPE_MASK);
					
					// 在 free_obj 前修改 current ( free_obj 会导致 root buffer （同current）被重新分配）
					/* Modify current before calling free_obj (bug #78811: free_obj() can cause the root buffer (with current) to be reallocated.) */
					
					// 给【计数器】指针添加 GC_GARBAGE 标记 ， offset 通常是0 参见：_zend_object_handlers 结构体
					current->ref = GC_MAKE_GARBAGE(((char*)obj) - obj->handlers->offset);
					// 如果对象没有被释放
					if (!(OBJ_FLAGS(obj) & IS_OBJ_FREE_CALLED)) {
						// 添加已释放标记
						GC_ADD_FLAGS(obj, IS_OBJ_FREE_CALLED);
						// 增加引用次数
						GC_ADDREF(obj);
						// 释放对象，这里也只是删除对象里面的数据，对象本身没有被free
						obj->handlers->free_obj(obj);
						// 减少引用次数
						GC_DELREF(obj);
					}
					// ?? 添加到释放列表
					ZEND_OBJECTS_STORE_ADD_TO_FREE_LIST(obj->handle);
				// 如果类型是数组 
				} else if (GC_TYPE(p) == IS_ARRAY) {
					// 转成数组 
					zend_array *arr = (zend_array*)p;
					// 清空type_info后4位（类型）
					GC_TYPE_INFO(arr) = GC_NULL |
						(GC_TYPE_INFO(arr) & ~GC_TYPE_MASK);
					// gc 可能销毁 rc>1的数组 。这个有效并且安全
					/* GC may destroy arrays with rc>1. This is valid and safe. */
					// 调试用
					HT_ALLOW_COW_VIOLATION(arr);
					// 销毁 哈希表（里面的内容）
					zend_hash_destroy(arr);
				}
			}
			// 下一个root
			idx++;
		}

		// 释放对象（释放内存，这是最后步骤了）
		/* Free objects */
		// 第一个root的指针
		current = GC_IDX2PTR(GC_FIRST_ROOT);
		// 最后一个root的指针
		last = GC_IDX2PTR(end);
		// 遍历每一个
		while (current != last) {
			// 如果是垃圾，计数器指针带了【垃圾】标记
			if (GC_IS_GARBAGE(current->ref)) {
				// 取得root指针
				p = GC_GET_PTR(current->ref);
				// 删除一个 gc_root_buffer （并没真的删除，只是标记成未使用）
				GC_LINK_UNUSED(current);
				// root数量 -1
				GC_G(num_roots)--;
				// 释放root
				efree(p);
			}
			// 下一个root
			current++;
		}
		// 
		zend_fiber_switch_unblock();

		GC_TRACE("Collection finished");
		// 回收数量增加
		GC_G(collected) += count;
		// 总回收数量 
		total_count += count;
		// 标记成回收完毕
		GC_G(gc_active) = 0;
	}
	// 没有 GC_G(num_roots) 的话直接到这里了

	// 整理buf
	gc_compact();

	/* Objects with destructors were removed from this GC run. Rerun GC right away to clean them
	 * up. We do this only once: If we encounter more destructors on the second run, we'll not
	 * run GC another time. */
	// 如果需要重复回收（只要有 GC_HAS_DESTRUCTORS（析构方法）就要重来）
	if (should_rerun_gc && !did_rerun_gc) {
		did_rerun_gc = 1;
		// 从头再来
		goto rerun_gc;
	}

// 完成
finish:
	// 释放stack指向的zval列表
	zend_get_gc_buffer_release();
	// ？？？
	zend_gc_root_tmpvars();
	return total_count;
}

// ing3, 获取 gc运行状态. 这个是线索
ZEND_API void zend_gc_get_status(zend_gc_status *status)
{
	// 回收次数
	status->runs = GC_G(gc_runs);
	// 已回收大小
	status->collected = GC_G(collected);
	// 回收临界值：
	status->threshold = GC_G(gc_threshold);
	// roots数量 
	status->num_roots = GC_G(num_roots);
}

// 这三个方法和主体逻辑关联不大，但在业务逻辑上关系密切，因为都用到buf
// 这个在迭代器中会用到
// ing4, 返回全局的 zend_get_gc_buffer
ZEND_API zend_get_gc_buffer *zend_get_gc_buffer_create(void) {
	// 同一时间只有一个get_gc()在运行，所以只需要一个buffer
	/* There can only be one get_gc() call active at a time,
	 * so there only needs to be one buffer. */
	// 全局只有一个 zend_get_gc_buffer，隨 _zend_executor_globals 一起创建。
	zend_get_gc_buffer *gc_buffer = &EG(get_gc_buffer);
	// 游标指向开始位置
	gc_buffer->cur = gc_buffer->start;
	// 返回实例
	return gc_buffer;
}

// ing4, zend_get_gc_buffer 增加内存，这里才创建需要的 zval 列表，一开始是只有指针组，没有zval列表的
ZEND_API void zend_get_gc_buffer_grow(zend_get_gc_buffer *gc_buffer) {
	// 旧容量
	size_t old_capacity = gc_buffer->end - gc_buffer->start;
	// 新容量，最小 64个zval大小，每次翻倍
	size_t new_capacity = old_capacity == 0 ? 64 : old_capacity * 2;
	// 调整内存大小
	gc_buffer->start = erealloc(gc_buffer->start, new_capacity * sizeof(zval));
	// 调整结束位置
	gc_buffer->end = gc_buffer->start + new_capacity;
	// 游标指向新的第一个zval
	gc_buffer->cur = gc_buffer->start + old_capacity;
}

// ing4, 释放 zval 列表，但不释放 _zend_executor_globals
static void zend_get_gc_buffer_release(void) {
	// 全局只有一个 zend_get_gc_buffer，隨 _zend_executor_globals 一起创建
	zend_get_gc_buffer *gc_buffer = &EG(get_gc_buffer);
	// 释放 zval 列表，但指针组还在，与 _zend_executor_globals 同在
	efree(gc_buffer->start);
	// 指针组置空
	gc_buffer->start = gc_buffer->end = gc_buffer->cur = NULL;
}

// zend_gc_collect_cycles 一处引用。
// TMPVAR 运算对象通过 zval_ptr_dtor_nogc 来销毁，因为它们通常不能包含回收周期。
// 但仍然会有一些少见的异常可能存在，在这样的用例里，依赖 producing code 来存放值。
// 如果 一个GC运行在 rooting 和消费中间，需要结束这样的泄露。为了避免这种情况发生，把所有 活跃 的 TMPVAR 值存放在这里。
/* TMPVAR operands are destroyed using zval_ptr_dtor_nogc(), because they usually cannot contain
 * cycles. However, there are some rare exceptions where this is possible, in which case we rely
 * on the producing code to root the value. If a GC run occurs between the rooting and consumption
 * of the value, we would end up leaking it. To avoid this, root all live TMPVAR values here. */
static void zend_gc_root_tmpvars(void) {
	// 当前的执行数据
	zend_execute_data *ex = EG(current_execute_data);
	// 只要ex有效就运行，运行完切换到下一个执行数据 
	for (; ex; ex = ex->prev_execute_data) {
		zend_function *func = ex->func;
		// 如果当前不在函数里 或 当前函数不是用户定义的
		if (!func || !ZEND_USER_CODE(func->type)) {
			// 下一行
			continue;
		}

		// 取得两个操作码之前的顺序差 
		// 当前 ex 的操作码，到当前函数的操作码列表开头，的偏移量
		uint32_t op_num = ex->opline - ex->func->op_array.opcodes;
		// last_live_range？
		for (uint32_t i = 0; i < func->op_array.last_live_range; i++) {
			// 依赖 zend_opcode.c
			const zend_live_range *range = &func->op_array.live_range[i];
			//
			if (range->start > op_num) {
				break;
			}
			//
			if (range->end <= op_num) {
				continue;
			}
			// 
			uint32_t kind = range->var & ZEND_LIVE_MASK;
			if (kind == ZEND_LIVE_TMPVAR) {
				uint32_t var_num = range->var & ~ZEND_LIVE_MASK;
				// ？
				zval *var = ZEND_CALL_VAR(ex, var_num);
				// 如果是可计数类型
				if (Z_REFCOUNTED_P(var)) {
					// 找个合适的root放进去
					gc_check_possible_root(Z_COUNTED_P(var));
				}
			}
		}
	}
}

#ifdef ZTS
// ing3, 这个为什么要写个封装
size_t zend_gc_globals_size(void)
{
	return sizeof(zend_gc_globals);
}
#endif
