//https://blog.csdn.net/zhanglei4214/article/details/6767288
//linux内核hlist分析
在Linux内核中，hlist（哈希链表）使用非常广泛。本文将对其数据结构和核心函数进行分析。
和hlist相关的数据结构有两个（1）hlist_head (2)hlist_node

//全局哈希表binder_procs(统计了所有的binder proc进程)
static HLIST_HEAD(binder_procs);
//#define HLIST_HEAD(name) struct hlist_head name = {  .first = NULL }
//展开它
static struct hlist_head binder_procs = {. first = NULL};
//http://gityuan.com/2015/11/01/binder-driver/
binder_procs表头和链表节点binder_proc之间的串接关系

//binder_proc是描述进程上下文信息的，每一个用户空间的进程都对应一个binder_proc结构体
struct binder_proc {
	struct hlist_node proc_node; 	//全局链表 binder_procs 的node之一 (连接件)
	struct rb_root threads; 	//binder_thread红黑树，存放指针，指向进程所有的binder_thread, 用于Server端
					//threads成员和binder_thread->rb_node关联到一棵红黑树，从而将binder_proc和binder_thread关联起来。
					
	struct rb_root nodes;   	//binder_node红黑树，存放指针，指向进程所有的binder 对象
					//nodes成员和binder_node->rb_node关联到一棵红黑树，从而将binder_proc和binder_node关联起来。
					
	struct rb_root refs_by_desc;	//binder_ref 红黑树，根据desc(service No) 查找对应的引用
					//refs_by_desc成员和binder_ref->rb_node_desc关联到一棵红黑树，从而将binder_proc和binder_ref关联起来。
					
	struct rb_root refs_by_node;	//binder_ref 红黑树，根据binder_node 指针查找对应的引用
					//refs_by_node成员和binder_ref->rb_node_node关联到一棵红黑树，从而将binder_proc和binder_ref关联起来。
	struct list_head waiting_threads;
	int pid;
	struct task_struct *tsk;
	struct files_struct *files;
	struct mutex files_lock;
	struct hlist_node deferred_work_node;
	int deferred_work;
	bool is_dead;

	struct list_head todo; //todo是该进程的待处理事务队列，而wait则是等待队列。它们的作用是实现进程的等待/唤醒,
				//当Server进程的wait等待队列为空时，Server就进入中断等待状态；当某Client向Server发送请求时，
				//就将该请求添加到Server的todo待处理事务队列中，并尝试唤醒Server等待队列上的线程。如果，
				//此时Server的待处理事务队列不为空，则Server被唤醒后；唤醒后，则取出待处理事务进行处理，处理完毕，则将结果返回给Client。
	struct binder_stats stats;
	struct list_head delivered_death;
	int max_threads;
	int requested_threads;
	int requested_threads_started;
	int tmp_ref;
	struct binder_priority default_priority;
	struct dentry *debugfs_entry;
	struct binder_alloc alloc;
	struct binder_context *context;
	spinlock_t inner_lock;
	spinlock_t outer_lock;
};

struct binder_thread {
	struct binder_proc *proc; // 线程所属的Binder进程
	struct rb_node rb_node; //binder_pro->threads成员和binder_thread->rb_node关联到一棵红黑树，从而将binder_proc和binder_thread关联起来。
	struct list_head waiting_thread_node; 
	int pid;
	int looper;              /* only modified by this thread */
	bool looper_need_return; /* can be written by other thread */
	struct binder_transaction *transaction_stack;
	struct list_head todo; // 待处理的事务链表
	bool process_todo;
	struct binder_error return_error;
	struct binder_error reply_error;
	wait_queue_head_t wait; // 等待队列
	struct binder_stats stats;
	atomic_t tmp_ref;
	bool is_dead;
	struct task_struct *task;
};

//binder_node是Binder实体对应的结构体，它是Server在Binder驱动中的体现。
/*
Binder实体，是各个Server以及ServiceManager在内核中的存在形式。
Binder实体实际上是内核中binder_node结构体的对象，它的作用是在内核中保存Server和ServiceManager的信息
(例如，Binder实体中保存了Server对象在用户空间的地址)。
简言之，Binder实体是Server在Binder驱动中的存在形式，内核通过Binder实体可以找到用户空间的Server对象。
*/
struct binder_node {
	int debug_id;
	spinlock_t lock;
	struct binder_work work;
	
	//rb_node和dead_node属于一个union。如果该Binder实体还在使用，则通过rb_node将该节点链接到proc->nodes红黑树中；
	//否则，则将该Binder实体通过dead_node链接到全局哈希表binder_dead_nodes中。
	union {
		struct rb_node rb_node; //binder_proc->nodes成员和binder_node->rb_node关联到一棵红黑树，从而将binder_proc和binder_node关联起来。
		struct hlist_node dead_node;
	};
	struct binder_proc *proc; // 该binder实体所属的Binder进程
	
	/*
	Binder实体中有一个Binder引用的哈希表，专门来存放该Binder实体的Binder引用。
	这也如我们之前所说，每个Binder实体则可以多个Binder引用，而每个Binder引用则都只对应一个Binder实体。
	*/
	struct hlist_head refs; // 该Binder实体的所有Binder引用所组成的链表
	int internal_strong_refs;
	int local_weak_refs;
	int local_strong_refs;
	int tmp_refs;
	binder_uintptr_t ptr;	 // Binder实体在用户空间的地址(为Binder实体对应的Server在用户空间的本地Binder的引用)
	binder_uintptr_t cookie; // Binder实体在用户空间的其他数据(为Binder实体对应的Server在用户空间的本地Binder自身)

	struct {
		/*
		 * bitfield elements protected by
		 * proc inner_lock
		 */
		u8 has_strong_ref:1;
		u8 pending_strong_ref:1;
		u8 has_weak_ref:1;
		u8 pending_weak_ref:1;
	};
	struct {
		/*
		 * invariant after initialization
		 */
		u8 sched_policy:2;
		u8 inherit_rt:1;
		u8 accept_fds:1;
		u8 txn_security_ctx:1;
		u8 min_priority;
	};
	bool has_async_transaction;
	struct list_head async_todo;
};

/*
说到Binder实体，就不得不说"Binder引用"。
所谓Binder引用，实际上是内核中binder_ref结构体的对象，它的作用是在表示"Binder实体"的引用。
换句话说，每一个Binder引用都是某一个Binder实体的引用，通过Binder引用可以在内核中找到它对应的Binder实体。
如果将Server看作是Binder实体的话，那么Client就好比Binder引用。
Client要和Server通信，它就是通过保存一个Server对象的Binder引用，
再通过该Binder引用在内核中找到对应的Binder实体，进而找到Server对象，然后将通信内容发送给Server对象。
Binder实体和Binder引用都是内核(即，Binder驱动)中的数据结构。
每一个Server在内核中就表现为一个Binder实体，而每一个Client则表现为一个Binder引用。
这样，每个Binder引用都对应一个Binder实体，而每个Binder实体则可以多个Binder引用。
*/
//binder_ref是Binder引用对应的结构体，它是Client在Binder驱动中的体现。
struct binder_ref {
	/* Lookups needed: */
	/*   node + proc => ref (transaction) */
	/*   desc + proc => ref (transaction, inc/dec ref) */
	/*   node => refs + procs (proc exit) */
	struct binder_ref_data data;
	unsigned int dummy;
	struct rb_node rb_node_desc; //binder_proc->refs_by_desc成员和binder_ref->rb_node_desc关联到一棵红黑树，从而将binder_proc和binder_ref关联起来。
	struct rb_node rb_node_node; //binder_proc->refs_by_node成员和binder_ref->rb_node_node关联到一棵红黑树，从而将binder_proc和binder_ref关联起来。
	struct hlist_node node_entry; // 关联到binder_node->refs哈希表中
	struct binder_proc *proc;  // 该Binder引用所属的Binder进程
	struct binder_node *node;  // 该Binder引用对应的Binder实体
	struct binder_ref_death *death;
};
