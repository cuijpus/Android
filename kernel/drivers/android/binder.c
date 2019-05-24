/**
 * struct binder_node - binder node bookkeeping
 * @debug_id:             unique ID for debugging
 *                        (invariant after initialized)
 * @lock:                 lock for node fields
 * @work:                 worklist element for node work
 *                        (protected by @proc->inner_lock)
 * @rb_node:              element for proc->nodes tree
 *                        (protected by @proc->inner_lock)
 * @dead_node:            element for binder_dead_nodes list
 *                        (protected by binder_dead_nodes_lock)
 * @proc:                 binder_proc that owns this node
 *                        (invariant after initialized)
 * @refs:                 list of references on this node
 *                        (protected by @lock)
 * @internal_strong_refs: used to take strong references when
 *                        initiating a transaction
 *                        (protected by @proc->inner_lock if @proc
 *                        and by @lock)
 * @local_weak_refs:      weak user refs from local process
 *                        (protected by @proc->inner_lock if @proc
 *                        and by @lock)
 * @local_strong_refs:    strong user refs from local process
 *                        (protected by @proc->inner_lock if @proc
 *                        and by @lock)
 * @tmp_refs:             temporary kernel refs
 *                        (protected by @proc->inner_lock while @proc
 *                        is valid, and by binder_dead_nodes_lock
 *                        if @proc is NULL. During inc/dec and node release
 *                        it is also protected by @lock to provide safety
 *                        as the node dies and @proc becomes NULL)
 * @ptr:                  userspace pointer for node
 *                        (invariant, no lock needed)
 * @cookie:               userspace cookie for node
 *                        (invariant, no lock needed)
 * @has_strong_ref:       userspace notified of strong ref
 *                        (protected by @proc->inner_lock if @proc
 *                        and by @lock)
 * @pending_strong_ref:   userspace has acked notification of strong ref
 *                        (protected by @proc->inner_lock if @proc
 *                        and by @lock)
 * @has_weak_ref:         userspace notified of weak ref
 *                        (protected by @proc->inner_lock if @proc
 *                        and by @lock)
 * @pending_weak_ref:     userspace has acked notification of weak ref
 *                        (protected by @proc->inner_lock if @proc
 *                        and by @lock)
 * @has_async_transaction: async transaction to node in progress
 *                        (protected by @lock)
 * @sched_policy:         minimum scheduling policy for node
 *                        (invariant after initialized)
 * @accept_fds:           file descriptor operations supported for node
 *                        (invariant after initialized)
 * @min_priority:         minimum scheduling priority
 *                        (invariant after initialized)
 * @inherit_rt:           inherit RT scheduling policy from caller
 *                        (invariant after initialized)
 * @txn_security_ctx:     require sender's security context
 *                        (invariant after initialized)
 * @async_todo:           list of async work items
 *                        (protected by @proc->inner_lock)
 *
 * Bookkeeping structure for binder nodes.
 */
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
