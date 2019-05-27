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
	union {
		struct rb_node rb_node;
		struct hlist_node dead_node;
	};
	struct binder_proc *proc;
	
	/*
	Binder实体中有一个Binder引用的哈希表，专门来存放该Binder实体的Binder引用。
	这也如我们之前所说，每个Binder实体则可以多个Binder引用，而每个Binder引用则都只对应一个Binder实体。
	*/
	struct hlist_head refs;
	int internal_strong_refs;
	int local_weak_refs;
	int local_strong_refs;
	int tmp_refs;
	binder_uintptr_t ptr;
	binder_uintptr_t cookie;
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
struct binder_ref {
	/* Lookups needed: */
	/*   node + proc => ref (transaction) */
	/*   desc + proc => ref (transaction, inc/dec ref) */
	/*   node => refs + procs (proc exit) */
	struct binder_ref_data data;
	unsigned int dummy;
	struct rb_node rb_node_desc;
	struct rb_node rb_node_node;
	struct hlist_node node_entry;
	struct binder_proc *proc;
	struct binder_node *node;
	struct binder_ref_death *death;
};
