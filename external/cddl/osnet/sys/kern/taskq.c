/*	$NetBSD: taskq.c,v 1.6.12.1 2018/06/25 07:25:25 pgoyette Exp $	*/

/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*
 * Copyright 2015 Nexenta Systems, Inc.  All rights reserved.
 */

/*
 * Kernel task queues: general-purpose asynchronous task scheduling.
 *
 * A common problem in kernel programming is the need to schedule tasks
 * to be performed later, by another thread. There are several reasons
 * you may want or need to do this:
 *
 * (1) The task isn't time-critical, but your current code path is.
 *
 * (2) The task may require grabbing locks that you already hold.
 *
 * (3) The task may need to block (e.g. to wait for memory), but you
 *     cannot block in your current context.
 *
 * (4) Your code path can't complete because of some condition, but you can't
 *     sleep or fail, so you queue the task for later execution when condition
 *     disappears.
 *
 * (5) You just want a simple way to launch multiple tasks in parallel.
 *
 * Task queues provide such a facility. In its simplest form (used when
 * performance is not a critical consideration) a task queue consists of a
 * single list of tasks, together with one or more threads to service the
 * list. There are some cases when this simple queue is not sufficient:
 *
 * (1) The task queues are very hot and there is a need to avoid data and lock
 *	contention over global resources.
 *
 * (2) Some tasks may depend on other tasks to complete, so they can't be put in
 *	the same list managed by the same thread.
 *
 * (3) Some tasks may block for a long time, and this should not block other
 *	tasks in the queue.
 *
 * To provide useful service in such cases we define a "dynamic task queue"
 * which has an individual thread for each of the tasks. These threads are
 * dynamically created as they are needed and destroyed when they are not in
 * use. The API for managing task pools is the same as for managing task queues
 * with the exception of a taskq creation flag TASKQ_DYNAMIC which tells that
 * dynamic task pool behavior is desired.
 *
 * Dynamic task queues may also place tasks in the normal queue (called "backing
 * queue") when task pool runs out of resources. Users of task queues may
 * disallow such queued scheduling by specifying TQ_NOQUEUE in the dispatch
 * flags.
 *
 * The backing task queue is also used for scheduling internal tasks needed for
 * dynamic task queue maintenance.
 *
 * INTERFACES ==================================================================
 *
 * taskq_t *taskq_create(name, nthreads, pri, minalloc, maxalloc, flags);
 *
 *	Create a taskq with specified properties.
 *	Possible 'flags':
 *
 *	  TASKQ_DYNAMIC: Create task pool for task management. If this flag is
 *		specified, 'nthreads' specifies the maximum number of threads in
 *		the task queue. Task execution order for dynamic task queues is
 *		not predictable.
 *
 *		If this flag is not specified (default case) a
 *		single-list task queue is created with 'nthreads' threads
 *		servicing it. Entries in this queue are managed by
 *		taskq_ent_alloc() and taskq_ent_free() which try to keep the
 *		task population between 'minalloc' and 'maxalloc', but the
 *		latter limit is only advisory for TQ_SLEEP dispatches and the
 *		former limit is only advisory for TQ_NOALLOC dispatches. If
 *		TASKQ_PREPOPULATE is set in 'flags', the taskq will be
 *		prepopulated with 'minalloc' task structures.
 *
 *		Since non-DYNAMIC taskqs are queues, tasks are guaranteed to be
 *		executed in the order they are scheduled if nthreads == 1.
 *		If nthreads > 1, task execution order is not predictable.
 *
 *	  TASKQ_PREPOPULATE: Prepopulate task queue with threads.
 *		Also prepopulate the task queue with 'minalloc' task structures.
 *
 *	  TASKQ_THREADS_CPU_PCT: This flag specifies that 'nthreads' should be
 *		interpreted as a percentage of the # of online CPUs on the
 *		system.  The taskq subsystem will automatically adjust the
 *		number of threads in the taskq in response to CPU online
 *		and offline events, to keep the ratio.  nthreads must be in
 *		the range [0,100].
 *
 *		The calculation used is:
 *
 *			MAX((ncpus_online * percentage)/100, 1)
 *
 *		This flag is not supported for DYNAMIC task queues.
 *		This flag is not compatible with TASKQ_CPR_SAFE.
 *
 *	  TASKQ_CPR_SAFE: This flag specifies that users of the task queue will
 *		use their own protocol for handling CPR issues. This flag is not
 *		supported for DYNAMIC task queues.  This flag is not compatible
 *		with TASKQ_THREADS_CPU_PCT.
 *
 *	The 'pri' field specifies the default priority for the threads that
 *	service all scheduled tasks.
 *
 * taskq_t *taskq_create_instance(name, instance, nthreads, pri, minalloc,
 *    maxalloc, flags);
 *
 *	Like taskq_create(), but takes an instance number (or -1 to indicate
 *	no instance).
 *
 * taskq_t *taskq_create_proc(name, nthreads, pri, minalloc, maxalloc, proc,
 *    flags);
 *
 *	Like taskq_create(), but creates the taskq threads in the specified
 *	system process.  If proc != &p0, this must be called from a thread
 *	in that process.
 *
 * taskq_t *taskq_create_sysdc(name, nthreads, minalloc, maxalloc, proc,
 *    dc, flags);
 *
 *	Like taskq_create_proc(), but the taskq threads will use the
 *	System Duty Cycle (SDC) scheduling class with a duty cycle of dc.
 *
 * void taskq_destroy(tap):
 *
 *	Waits for any scheduled tasks to complete, then destroys the taskq.
 *	Caller should guarantee that no new tasks are scheduled in the closing
 *	taskq.
 *
 * taskqid_t taskq_dispatch(tq, func, arg, flags):
 *
 *	Dispatches the task "func(arg)" to taskq. The 'flags' indicates whether
 *	the caller is willing to block for memory.  The function returns an
 *	opaque value which is zero iff dispatch fails.  If flags is TQ_NOSLEEP
 *	or TQ_NOALLOC and the task can't be dispatched, taskq_dispatch() fails
 *	and returns (taskqid_t)0.
 *
 *	ASSUMES: func != NULL.
 *
 *	Possible flags:
 *	  TQ_NOSLEEP: Do not wait for resources; may fail.
 *
 *	  TQ_NOALLOC: Do not allocate memory; may fail.  May only be used with
 *		non-dynamic task queues.
 *
 *	  TQ_NOQUEUE: Do not enqueue a task if it can't dispatch it due to
 *		lack of available resources and fail. If this flag is not
 *		set, and the task pool is exhausted, the task may be scheduled
 *		in the backing queue. This flag may ONLY be used with dynamic
 *		task queues.
 *
 *		NOTE: This flag should always be used when a task queue is used
 *		for tasks that may depend on each other for completion.
 *		Enqueueing dependent tasks may create deadlocks.
 *
 *	  TQ_SLEEP:   May block waiting for resources. May still fail for
 *		dynamic task queues if TQ_NOQUEUE is also specified, otherwise
 *		always succeed.
 *
 *	  TQ_FRONT:   Puts the new task at the front of the queue.  Be careful.
 *
 *	NOTE: Dynamic task queues are much more likely to fail in
 *		taskq_dispatch() (especially if TQ_NOQUEUE was specified), so it
 *		is important to have backup strategies handling such failures.
 *
 * void taskq_dispatch_ent(tq, func, arg, flags, tqent)
 *
 *	This is a light-weight form of taskq_dispatch(), that uses a
 *	preallocated taskq_ent_t structure for scheduling.  As a
 *	result, it does not perform allocations and cannot ever fail.
 *	Note especially that it cannot be used with TASKQ_DYNAMIC
 *	taskqs.  The memory for the tqent must not be modified or used
 *	until the function (func) is called.  (However, func itself
 *	may safely modify or free this memory, once it is called.)
 *	Note that the taskq framework will NOT free this memory.
 *
 * void taskq_wait(tq):
 *
 *	Waits for all previously scheduled tasks to complete.
 *
 *	NOTE: It does not stop any new task dispatches.
 *	      Do NOT call taskq_wait() from a task: it will cause deadlock.
 *
 * void taskq_suspend(tq)
 *
 *	Suspend all task execution. Tasks already scheduled for a dynamic task
 *	queue will still be executed, but all new scheduled tasks will be
 *	suspended until taskq_resume() is called.
 *
 * int  taskq_suspended(tq)
 *
 *	Returns 1 if taskq is suspended and 0 otherwise. It is intended to
 *	ASSERT that the task queue is suspended.
 *
 * void taskq_resume(tq)
 *
 *	Resume task queue execution.
 *
 * int  taskq_member(tq, thread)
 *
 *	Returns 1 if 'thread' belongs to taskq 'tq' and 0 otherwise. The
 *	intended use is to ASSERT that a given function is called in taskq
 *	context only.
 *
 * system_taskq
 *
 *	Global system-wide dynamic task queue for common uses. It may be used by
 *	any subsystem that needs to schedule tasks and does not need to manage
 *	its own task queues. It is initialized quite early during system boot.
 *
 * IMPLEMENTATION ==============================================================
 *
 * This is schematic representation of the task queue structures.
 *
 *   taskq:
 *   +-------------+
 *   | tq_lock     | +---< taskq_ent_free()
 *   +-------------+ |
 *   |...          | | tqent:                  tqent:
 *   +-------------+ | +------------+          +------------+
 *   | tq_freelist |-->| tqent_next |--> ... ->| tqent_next |
 *   +-------------+   +------------+          +------------+
 *   |...          |   | ...        |          | ...        |
 *   +-------------+   +------------+          +------------+
 *   | tq_task     |    |
 *   |             |    +-------------->taskq_ent_alloc()
 * +--------------------------------------------------------------------------+
 * | |                     |            tqent                   tqent         |
 * | +---------------------+     +--> +------------+     +--> +------------+  |
 * | | ...		   |     |    | func, arg  |     |    | func, arg  |  |
 * +>+---------------------+ <---|-+  +------------+ <---|-+  +------------+  |
 *   | tq_taskq.tqent_next | ----+ |  | tqent_next | --->+ |  | tqent_next |--+
 *   +---------------------+	   |  +------------+     ^ |  +------------+
 * +-| tq_task.tqent_prev  |	   +--| tqent_prev |     | +--| tqent_prev |  ^
 * | +---------------------+	      +------------+     |    +------------+  |
 * | |...		   |	      | ...        |     |    | ...        |  |
 * | +---------------------+	      +------------+     |    +------------+  |
 * |                                      ^              |                    |
 * |                                      |              |                    |
 * +--------------------------------------+--------------+       TQ_APPEND() -+
 *   |             |                      |
 *   |...          |   taskq_thread()-----+
 *   +-------------+
 *   | tq_buckets  |--+-------> [ NULL ] (for regular task queues)
 *   +-------------+  |
 *                    |   DYNAMIC TASK QUEUES:
 *                    |
 *                    +-> taskq_bucket[nCPU]		taskq_bucket_dispatch()
 *                        +-------------------+                    ^
 *                   +--->| tqbucket_lock     |                    |
 *                   |    +-------------------+   +--------+      +--------+
 *                   |    | tqbucket_freelist |-->| tqent  |-->...| tqent  | ^
 *                   |    +-------------------+<--+--------+<--...+--------+ |
 *                   |    | ...               |   | thread |      | thread | |
 *                   |    +-------------------+   +--------+      +--------+ |
 *                   |    +-------------------+                              |
 * taskq_dispatch()--+--->| tqbucket_lock     |             TQ_APPEND()------+
 *      TQ_HASH()    |    +-------------------+   +--------+      +--------+
 *                   |    | tqbucket_freelist |-->| tqent  |-->...| tqent  |
 *                   |    +-------------------+<--+--------+<--...+--------+
 *                   |    | ...               |   | thread |      | thread |
 *                   |    +-------------------+   +--------+      +--------+
 *		     +--->	...
 *
 *
 * Task queues use tq_task field to link new entry in the queue. The queue is a
 * circular doubly-linked list. Entries are put in the end of the list with
 * TQ_APPEND() and processed from the front of the list by taskq_thread() in
 * FIFO order. Task queue entries are cached in the free list managed by
 * taskq_ent_alloc() and taskq_ent_free() functions.
 *
 *	All threads used by task queues mark t_taskq field of the thread to
 *	point to the task queue.
 *
 * Taskq Thread Management -----------------------------------------------------
 *
 * Taskq's non-dynamic threads are managed with several variables and flags:
 *
 *	* tq_nthreads	- The number of threads in taskq_thread() for the
 *			  taskq.
 *
 *	* tq_active	- The number of threads not waiting on a CV in
 *			  taskq_thread(); includes newly created threads
 *			  not yet counted in tq_nthreads.
 *
 *	* tq_nthreads_target
 *			- The number of threads desired for the taskq.
 *
 *	* tq_flags & TASKQ_CHANGING
 *			- Indicates that tq_nthreads != tq_nthreads_target.
 *
 *	* tq_flags & TASKQ_THREAD_CREATED
 *			- Indicates that a thread is being created in the taskq.
 *
 * During creation, tq_nthreads and tq_active are set to 0, and
 * tq_nthreads_target is set to the number of threads desired.  The
 * TASKQ_CHANGING flag is set, and taskq_thread_create() is called to
 * create the first thread. taskq_thread_create() increments tq_active,
 * sets TASKQ_THREAD_CREATED, and creates the new thread.
 *
 * Each thread starts in taskq_thread(), clears the TASKQ_THREAD_CREATED
 * flag, and increments tq_nthreads.  It stores the new value of
 * tq_nthreads as its "thread_id", and stores its thread pointer in the
 * tq_threadlist at the (thread_id - 1).  We keep the thread_id space
 * densely packed by requiring that only the largest thread_id can exit during
 * normal adjustment.   The exception is during the destruction of the
 * taskq; once tq_nthreads_target is set to zero, no new threads will be created
 * for the taskq queue, so every thread can exit without any ordering being
 * necessary.
 *
 * Threads will only process work if their thread id is <= tq_nthreads_target.
 *
 * When TASKQ_CHANGING is set, threads will check the current thread target
 * whenever they wake up, and do whatever they can to apply its effects.
 *
 * TASKQ_THREAD_CPU_PCT --------------------------------------------------------
 *
 * When a taskq is created with TASKQ_THREAD_CPU_PCT, we store their requested
 * percentage in tq_threads_ncpus_pct, start them off with the correct thread
 * target, and add them to the taskq_cpupct_list for later adjustment.
 *
 * We register taskq_cpu_setup() to be called whenever a CPU changes state.  It
 * walks the list of TASKQ_THREAD_CPU_PCT taskqs, adjusts their nthread_target
 * if need be, and wakes up all of the threads to process the change.
 *
 * Dynamic Task Queues Implementation ------------------------------------------
 *
 * For a dynamic task queues there is a 1-to-1 mapping between a thread and
 * taskq_ent_structure. Each entry is serviced by its own thread and each thread
 * is controlled by a single entry.
 *
 * Entries are distributed over a set of buckets. To avoid using modulo
 * arithmetics the number of buckets is 2^n and is determined as the nearest
 * power of two roundown of the number of CPUs in the system. Tunable
 * variable 'taskq_maxbuckets' limits the maximum number of buckets. Each entry
 * is attached to a bucket for its lifetime and can't migrate to other buckets.
 *
 * Entries that have scheduled tasks are not placed in any list. The dispatch
 * function sets their "func" and "arg" fields and signals the corresponding
 * thread to execute the task. Once the thread executes the task it clears the
 * "func" field and places an entry on the bucket cache of free entries pointed
 * by "tqbucket_freelist" field. ALL entries on the free list should have "func"
 * field equal to NULL. The free list is a circular doubly-linked list identical
 * in structure to the tq_task list above, but entries are taken from it in LIFO
 * order - the last freed entry is the first to be allocated. The
 * taskq_bucket_dispatch() function gets the most recently used entry from the
 * free list, sets its "func" and "arg" fields and signals a worker thread.
 *
 * After executing each task a per-entry thread taskq_d_thread() places its
 * entry on the bucket free list and goes to a timed sleep. If it wakes up
 * without getting new task it removes the entry from the free list and destroys
 * itself. The thread sleep time is controlled by a tunable variable
 * `taskq_thread_timeout'.
 *
 * There are various statistics kept in the bucket which allows for later
 * analysis of taskq usage patterns. Also, a global copy of taskq creation and
 * death statistics is kept in the global taskq data structure. Since thread
 * creation and death happen rarely, updating such global data does not present
 * a performance problem.
 *
 * NOTE: Threads are not bound to any CPU and there is absolutely no association
 *       between the bucket and actual thread CPU, so buckets are used only to
 *	 split resources and reduce resource contention. Having threads attached
 *	 to the CPU denoted by a bucket may reduce number of times the job
 *	 switches between CPUs.
 *
 *	 Current algorithm creates a thread whenever a bucket has no free
 *	 entries. It would be nice to know how many threads are in the running
 *	 state and don't create threads if all CPUs are busy with existing
 *	 tasks, but it is unclear how such strategy can be implemented.
 *
 *	 Currently buckets are created statically as an array attached to task
 *	 queue. On some system with nCPUs < max_ncpus it may waste system
 *	 memory. One solution may be allocation of buckets when they are first
 *	 touched, but it is not clear how useful it is.
 *
 * SUSPEND/RESUME implementation -----------------------------------------------
 *
 *	Before executing a task taskq_thread() (executing non-dynamic task
 *	queues) obtains taskq's thread lock as a reader. The taskq_suspend()
 *	function gets the same lock as a writer blocking all non-dynamic task
 *	execution. The taskq_resume() function releases the lock allowing
 *	taskq_thread to continue execution.
 *
 *	For dynamic task queues, each bucket is marked as TQBUCKET_SUSPEND by
 *	taskq_suspend() function. After that taskq_bucket_dispatch() always
 *	fails, so that taskq_dispatch() will either enqueue tasks for a
 *	suspended backing queue or fail if TQ_NOQUEUE is specified in dispatch
 *	flags.
 *
 *	NOTE: taskq_suspend() does not immediately block any tasks already
 *	      scheduled for dynamic task queues. It only suspends new tasks
 *	      scheduled after taskq_suspend() was called.
 *
 *	taskq_member() function works by comparing a thread t_taskq pointer with
 *	the passed thread pointer.
 *
 * LOCKS and LOCK Hierarchy ----------------------------------------------------
 *
 *   There are three locks used in task queues:
 *
 *   1) The taskq_t's tq_lock, protecting global task queue state.
 *
 *   2) Each per-CPU bucket has a lock for bucket management.
 *
 *   3) The global taskq_cpupct_lock, which protects the list of
 *      TASKQ_THREADS_CPU_PCT taskqs.
 *
 *   If both (1) and (2) are needed, tq_lock should be taken *after* the bucket
 *   lock.
 *
 *   If both (1) and (3) are needed, tq_lock should be taken *after*
 *   taskq_cpupct_lock.
 *
 * DEBUG FACILITIES ------------------------------------------------------------
 *
 * For DEBUG kernels it is possible to induce random failures to
 * taskq_dispatch() function when it is given TQ_NOSLEEP argument. The value of
 * taskq_dmtbf and taskq_smtbf tunables control the mean time between induced
 * failures for dynamic and static task queues respectively.
 *
 * Setting TASKQ_STATISTIC to 0 will disable per-bucket statistics.
 *
 * TUNABLES --------------------------------------------------------------------
 *
 *	system_taskq_size	- Size of the global system_taskq.
 *				  This value is multiplied by nCPUs to determine
 *				  actual size.
 *				  Default value: 64
 *
 *	taskq_minimum_nthreads_max
 *				- Minimum size of the thread list for a taskq.
 *				  Useful for testing different thread pool
 *				  sizes by overwriting tq_nthreads_target.
 *
 *	taskq_thread_timeout	- Maximum idle time for taskq_d_thread()
 *				  Default value: 5 minutes
 *
 *	taskq_maxbuckets	- Maximum number of buckets in any task queue
 *				  Default value: 128
 *
 *	taskq_search_depth	- Maximum # of buckets searched for a free entry
 *				  Default value: 4
 *
 *	taskq_dmtbf		- Mean time between induced dispatch failures
 *				  for dynamic task queues.
 *				  Default value: UINT_MAX (no induced failures)
 *
 *	taskq_smtbf		- Mean time between induced dispatch failures
 *				  for static task queues.
 *				  Default value: UINT_MAX (no induced failures)
 *
 * CONDITIONAL compilation -----------------------------------------------------
 *
 *    TASKQ_STATISTIC	- If set will enable bucket statistic (default).
 *
 */

#include <sys/kthread.h>
#include <sys/taskq_impl.h>
#include <sys/proc.h>
#include <sys/kmem.h>
#include <sys/callb.h>
#include <sys/systm.h>
#include <sys/cmn_err.h>
#include <sys/debug.h>
#include <sys/sysmacros.h>
#include <sys/sdt.h>
#include <sys/mutex.h>
#include <sys/kernel.h>
#include <sys/syslimits.h>

static kmem_cache_t *taskq_ent_cache, *taskq_cache;

/* Global system task queue for common use */
taskq_t *system_taskq;

/*
 * Maxmimum number of entries in global system taskq is
 *      system_taskq_size * max_ncpus
 */
#define SYSTEM_TASKQ_SIZE 1
int system_taskq_size = SYSTEM_TASKQ_SIZE;

/*
 * Minimum size for tq_nthreads_max; useful for those who want to play around
 * with increasing a taskq's tq_nthreads_target.
 */
int taskq_minimum_nthreads_max = 1;

/*
 * We want to ensure that when taskq_create() returns, there is at least
 * one thread ready to handle requests.  To guarantee this, we have to wait
 * for the second thread, since the first one cannot process requests until
 * the second thread has been created.
 */
#define	TASKQ_CREATE_ACTIVE_THREADS	2

/* Maximum percentage allowed for TASKQ_THREADS_CPU_PCT */
#define	TASKQ_CPUPCT_MAX_PERCENT	1000
int taskq_cpupct_max_percent = TASKQ_CPUPCT_MAX_PERCENT;

/*
 * Dynamic task queue threads that don't get any work within
 * taskq_thread_timeout destroy themselves
 */
#define	TASKQ_THREAD_TIMEOUT (60 * 5)
int taskq_thread_timeout = TASKQ_THREAD_TIMEOUT;

#define	TASKQ_MAXBUCKETS 128
int taskq_maxbuckets = TASKQ_MAXBUCKETS;

/*
 * When a bucket has no available entries another buckets are tried.
 * taskq_search_depth parameter limits the amount of buckets that we search
 * before failing. This is mostly useful in systems with many CPUs where we may
 * spend too much time scanning busy buckets.
 */
#define	TASKQ_SEARCH_DEPTH 4
int taskq_search_depth = TASKQ_SEARCH_DEPTH;

/*
 * Hashing function: mix various bits of x. May be pretty much anything.
 */
#define	TQ_HASH(x) ((x) ^ ((x) >> 11) ^ ((x) >> 17) ^ ((x) ^ 27))

/*
 * We do not create any new threads when the system is low on memory and start
 * throttling memory allocations. The following macro tries to estimate such
 * condition.
 */
#define	ENOUGH_MEMORY() (freemem > throttlefree)

/*
 * Static functions.
 */
static taskq_t	*taskq_create_common(const char *, int, int, pri_t, int,
    int, uint_t);
static void taskq_thread(void *);
static int  taskq_constructor(void *, void *, int);
static void taskq_destructor(void *, void *);
static int  taskq_ent_constructor(void *, void *, int);
static void taskq_ent_destructor(void *, void *);
static taskq_ent_t *taskq_ent_alloc(taskq_t *, int);
static void taskq_ent_free(taskq_t *, taskq_ent_t *);

void system_taskq_init(void);
void system_taskq_fini(void);

/*
 * List of all TASKQ_THREADS_CPU_PCT taskqs.
 */
static list_t taskq_cpupct_list;	/* protected by cpu_lock */

#ifdef __NetBSD__
#define cpu_lock taskq_cpu_lock
static kmutex_t cpu_lock;

typedef struct {
	int	cp_id;
	int	cp_ncpus;
} cpupart_t;
#endif

/*
 * Collect per-bucket statistic when TASKQ_STATISTIC is defined.
 */
#define	TASKQ_STATISTIC 1

#if TASKQ_STATISTIC
#define	TQ_STAT(b, x)	b->tqbucket_stat.x++
#else
#define	TQ_STAT(b, x)
#endif

/*
 * Random fault injection.
 */
uint_t taskq_random;
uint_t taskq_dmtbf = UINT_MAX;    /* mean time between injected failures */
uint_t taskq_smtbf = UINT_MAX;    /* mean time between injected failures */

/*
 * TQ_NOSLEEP dispatches on dynamic task queues are always allowed to fail.
 *
 * TQ_NOSLEEP dispatches on static task queues can't arbitrarily fail because
 * they could prepopulate the cache and make sure that they do not use more
 * then minalloc entries.  So, fault injection in this case insures that
 * either TASKQ_PREPOPULATE is not set or there are more entries allocated
 * than is specified by minalloc.  TQ_NOALLOC dispatches are always allowed
 * to fail, but for simplicity we treat them identically to TQ_NOSLEEP
 * dispatches.
 */
#ifdef DEBUG
#define	TASKQ_D_RANDOM_DISPATCH_FAILURE(tq, flag)		\
	taskq_random = (taskq_random * 2416 + 374441) % 1771875;\
	if ((flag & TQ_NOSLEEP) &&				\
	    taskq_random < 1771875 / taskq_dmtbf) {		\
		return (NULL);					\
	}

#define	TASKQ_S_RANDOM_DISPATCH_FAILURE(tq, flag)		\
	taskq_random = (taskq_random * 2416 + 374441) % 1771875;\
	if ((flag & (TQ_NOSLEEP | TQ_NOALLOC)) &&		\
	    (!(tq->tq_flags & TASKQ_PREPOPULATE) ||		\
	    (tq->tq_nalloc > tq->tq_minalloc)) &&		\
	    (taskq_random < (1771875 / taskq_smtbf))) {		\
		mutex_exit(&tq->tq_lock);			\
		return ((taskqid_t)0);				\
	}
#else
#define	TASKQ_S_RANDOM_DISPATCH_FAILURE(tq, flag)
#define	TASKQ_D_RANDOM_DISPATCH_FAILURE(tq, flag)
#endif

#define	IS_EMPTY(l) (((l).tqent_prev == (l).tqent_next) &&	\
	((l).tqent_prev == &(l)))

/*
 * Append `tqe' in the end of the doubly-linked list denoted by l.
 */
#define	TQ_APPEND(l, tqe) {					\
	tqe->tqent_next = &l;					\
	tqe->tqent_prev = l.tqent_prev;				\
	tqe->tqent_next->tqent_prev = tqe;			\
	tqe->tqent_prev->tqent_next = tqe;			\
}
/*
 * Prepend 'tqe' to the beginning of l
 */
#define	TQ_PREPEND(l, tqe) {					\
	tqe->tqent_next = l.tqent_next;				\
	tqe->tqent_prev = &l;					\
	tqe->tqent_next->tqent_prev = tqe;			\
	tqe->tqent_prev->tqent_next = tqe;			\
}

/*
 * Schedule a task specified by func and arg into the task queue entry tqe.
 */
#define	TQ_DO_ENQUEUE(tq, tqe, func, arg, front) {			\
	ASSERT(MUTEX_HELD(&tq->tq_lock));				\
	_NOTE(CONSTCOND)						\
	if (front) {							\
		TQ_PREPEND(tq->tq_task, tqe);				\
	} else {							\
		TQ_APPEND(tq->tq_task, tqe);				\
	}								\
	tqe->tqent_func = (func);					\
	tqe->tqent_arg = (arg);						\
	tq->tq_tasks++;							\
	if (tq->tq_tasks - tq->tq_executed > tq->tq_maxtasks)		\
		tq->tq_maxtasks = tq->tq_tasks - tq->tq_executed;	\
	cv_signal(&tq->tq_dispatch_cv);					\
	DTRACE_PROBE2(taskq__enqueue, taskq_t *, tq, taskq_ent_t *, tqe); \
}

#define	TQ_ENQUEUE(tq, tqe, func, arg)					\
	TQ_DO_ENQUEUE(tq, tqe, func, arg, 0)

#define	TQ_ENQUEUE_FRONT(tq, tqe, func, arg)				\
	TQ_DO_ENQUEUE(tq, tqe, func, arg, 1)

/*
 * Do-nothing task which may be used to prepopulate thread caches.
 */
/*ARGSUSED*/
void
nulltask(void *unused)
{
}


/*ARGSUSED*/
static int
taskq_constructor(void *arg, void *obj, int kmflags)
{
	taskq_t *tq = obj;

	memset(tq, 0, sizeof (taskq_t));

	mutex_init(&tq->tq_lock, NULL, MUTEX_DEFAULT, NULL);
	rw_init(&tq->tq_threadlock, NULL, RW_DEFAULT, NULL);
	cv_init(&tq->tq_dispatch_cv, NULL, CV_DEFAULT, NULL);
	cv_init(&tq->tq_wait_cv, NULL, CV_DEFAULT, NULL);
	cv_init(&tq->tq_exit_cv, NULL, CV_DEFAULT, NULL);

	tq->tq_task.tqent_next = &tq->tq_task;
	tq->tq_task.tqent_prev = &tq->tq_task;

	return (0);
}

/*ARGSUSED*/
static void
taskq_destructor(void *arg, void *obj)
{
	taskq_t *tq = obj;

	mutex_destroy(&tq->tq_lock);
	rw_destroy(&tq->tq_threadlock);
	cv_destroy(&tq->tq_dispatch_cv);
	cv_destroy(&tq->tq_wait_cv);
	cv_destroy(&tq->tq_exit_cv);
}

/*ARGSUSED*/
static int
taskq_ent_constructor(void *arg, void *obj, int kmflags)
{
	taskq_ent_t *tqe = obj;

	tqe->tqent_thread = NULL;
	cv_init(&tqe->tqent_cv, NULL, CV_DEFAULT, NULL);

	return (0);
}

/*ARGSUSED*/
static void
taskq_ent_destructor(void *arg, void *obj)
{
	taskq_ent_t *tqe = obj;

	ASSERT(tqe->tqent_thread == NULL);
	cv_destroy(&tqe->tqent_cv);
}

void
taskq_init(void)
{
	taskq_ent_cache = kmem_cache_create("taskq_ent_cache",
	    sizeof (taskq_ent_t), 0, taskq_ent_constructor,
	    taskq_ent_destructor, NULL, NULL, NULL, 0);
	taskq_cache = kmem_cache_create("taskq_cache", sizeof (taskq_t),
	    0, taskq_constructor, taskq_destructor, NULL, NULL, NULL, 0);

	list_create(&taskq_cpupct_list, sizeof (taskq_t),
	    offsetof(taskq_t, tq_cpupct_link));

#ifdef __NetBSD__
	system_taskq_init();
	mutex_init(&cpu_lock, NULL, MUTEX_DEFAULT, NULL);
#endif
}

void
taskq_fini(void)
{
	system_taskq_fini();
	mutex_destroy(&cpu_lock);
	kmem_cache_destroy(taskq_cache);
	kmem_cache_destroy(taskq_ent_cache);
}

static void
taskq_update_nthreads(taskq_t *tq, uint_t ncpus)
{
	uint_t newtarget = TASKQ_THREADS_PCT(ncpus, tq->tq_threads_ncpus_pct);

	ASSERT(MUTEX_HELD(&cpu_lock));
	ASSERT(MUTEX_HELD(&tq->tq_lock));

	/* We must be going from non-zero to non-zero; no exiting. */
	ASSERT3U(tq->tq_nthreads_target, !=, 0);
	ASSERT3U(newtarget, !=, 0);

	ASSERT3U(newtarget, <=, tq->tq_nthreads_max);
	if (newtarget != tq->tq_nthreads_target) {
		tq->tq_flags |= TASKQ_CHANGING;
		tq->tq_nthreads_target = newtarget;
		cv_broadcast(&tq->tq_dispatch_cv);
		cv_broadcast(&tq->tq_exit_cv);
	}
}

/* called during task queue creation */
static void
taskq_cpupct_install(taskq_t *tq, cpupart_t *cpup)
{
	ASSERT(tq->tq_flags & TASKQ_THREADS_CPU_PCT);

	mutex_enter(&cpu_lock);
	mutex_enter(&tq->tq_lock);
	tq->tq_cpupart = cpup->cp_id;
	taskq_update_nthreads(tq, cpup->cp_ncpus);
	mutex_exit(&tq->tq_lock);

	list_insert_tail(&taskq_cpupct_list, tq);
	mutex_exit(&cpu_lock);
}

static void
taskq_cpupct_remove(taskq_t *tq)
{
	ASSERT(tq->tq_flags & TASKQ_THREADS_CPU_PCT);

	mutex_enter(&cpu_lock);
	list_remove(&taskq_cpupct_list, tq);
	mutex_exit(&cpu_lock);
}

#ifdef illumos
/*ARGSUSED*/
static int
taskq_cpu_setup(cpu_setup_t what, int id, void *arg)
{
	taskq_t *tq;
	cpupart_t *cp = cpu[id]->cpu_part;
	uint_t ncpus = cp->cp_ncpus;

	ASSERT(MUTEX_HELD(&cpu_lock));
	ASSERT(ncpus > 0);

	switch (what) {
	case CPU_OFF:
	case CPU_CPUPART_OUT:
		/* offlines are called *before* the cpu is offlined. */
		if (ncpus > 1)
			ncpus--;
		break;

	case CPU_ON:
	case CPU_CPUPART_IN:
		break;

	default:
		return (0);		/* doesn't affect cpu count */
	}

	for (tq = list_head(&taskq_cpupct_list); tq != NULL;
	    tq = list_next(&taskq_cpupct_list, tq)) {

		mutex_enter(&tq->tq_lock);
		/*
		 * If the taskq is part of the cpuset which is changing,
		 * update its nthreads_target.
		 */
		if (tq->tq_cpupart == cp->cp_id) {
			taskq_update_nthreads(tq, ncpus);
		}
		mutex_exit(&tq->tq_lock);
	}
	return (0);
}

void
taskq_mp_init(void)
{
	mutex_enter(&cpu_lock);
	register_cpu_setup_func(taskq_cpu_setup, NULL);
	/*
	 * Make sure we're up to date.  At this point in boot, there is only
	 * one processor set, so we only have to update the current CPU.
	 */
	(void) taskq_cpu_setup(CPU_ON, CPU->cpu_id, NULL);
	mutex_exit(&cpu_lock);
}
#endif /* illumos */

/*
 * Create global system dynamic task queue.
 */
void
system_taskq_init(void)
{
	system_taskq = taskq_create_common("system_taskq", 0,
	    system_taskq_size * max_ncpus, minclsyspri, 4, 512,
	    TASKQ_PREPOPULATE);
}

void
system_taskq_fini(void)
{
	taskq_destroy(system_taskq);
}

/*
 * taskq_ent_alloc()
 *
 * Allocates a new taskq_ent_t structure either from the free list or from the
 * cache. Returns NULL if it can't be allocated.
 *
 * Assumes: tq->tq_lock is held.
 */
static taskq_ent_t *
taskq_ent_alloc(struct taskq *tq, int flags)
{
	int kmflags = (flags & TQ_NOSLEEP) ? KM_NOSLEEP : KM_SLEEP;
	taskq_ent_t *tqe;

	ASSERT(MUTEX_HELD(&tq->tq_lock));

	/*
	 * TQ_NOALLOC allocations are allowed to use the freelist, even if
	 * we are below tq_minalloc.
	 */
	if ((tqe = tq->tq_freelist) != NULL &&
	    ((flags & TQ_NOALLOC) || tq->tq_nalloc >= tq->tq_minalloc)) {
		tq->tq_freelist = tqe->tqent_next;
	} else {
		if (flags & TQ_NOALLOC)
			return (NULL);

		mutex_exit(&tq->tq_lock);
		if (tq->tq_nalloc >= tq->tq_maxalloc) {
			if (kmflags & KM_NOSLEEP) {
				mutex_enter(&tq->tq_lock);
				return (NULL);
			}
			/*
			 * We don't want to exceed tq_maxalloc, but we can't
			 * wait for other tasks to complete (and thus free up
			 * task structures) without risking deadlock with
			 * the caller.  So, we just delay for one second
			 * to throttle the allocation rate.
			 */
			delay(hz);
		}
		tqe = kmem_cache_alloc(taskq_ent_cache, kmflags);
		mutex_enter(&tq->tq_lock);
		if (tqe != NULL)
			tq->tq_nalloc++;
	}
	return (tqe);
}

/*
 * taskq_ent_free()
 *
 * Free taskq_ent_t structure by either putting it on the free list or freeing
 * it to the cache.
 *
 * Assumes: tq->tq_lock is held.
 */
static void
taskq_ent_free(taskq_t *tq, taskq_ent_t *tqe)
{
	ASSERT(MUTEX_HELD(&tq->tq_lock));

	if (tq->tq_nalloc <= tq->tq_minalloc) {
		tqe->tqent_next = tq->tq_freelist;
		tq->tq_freelist = tqe;
	} else {
		tq->tq_nalloc--;
		mutex_exit(&tq->tq_lock);
		kmem_cache_free(taskq_ent_cache, tqe);
		mutex_enter(&tq->tq_lock);
	}
}

/*
 * Dispatch a task.
 *
 * Assumes: func != NULL
 *
 * Returns: NULL if dispatch failed.
 *	    non-NULL if task dispatched successfully.
 *	    Actual return value is the pointer to taskq entry that was used to
 *	    dispatch a task. This is useful for debugging.
 */
/* ARGSUSED */
taskqid_t
taskq_dispatch(taskq_t *tq, task_func_t func, void *arg, uint_t flags)
{
	taskq_ent_t *tqe = NULL;

	ASSERT(tq != NULL);
	ASSERT(func != NULL);
	ASSERT(!(tq->tq_flags & TASKQ_DYNAMIC));

	if (!(tq->tq_flags & TASKQ_DYNAMIC)) {
		/*
		 * TQ_NOQUEUE flag can't be used with non-dynamic task queues.
		 */
		ASSERT(!(flags & TQ_NOQUEUE));
		/*
		 * Enqueue the task to the underlying queue.
		 */
		mutex_enter(&tq->tq_lock);

		TASKQ_S_RANDOM_DISPATCH_FAILURE(tq, flags);

		if ((tqe = taskq_ent_alloc(tq, flags)) == NULL) {
			mutex_exit(&tq->tq_lock);
			return (0);
		}
		/* Make sure we start without any flags */
		tqe->tqent_flags = 0;

		if (flags & TQ_FRONT) {
			TQ_ENQUEUE_FRONT(tq, tqe, func, arg);
		} else {
			TQ_ENQUEUE(tq, tqe, func, arg);
		}
		mutex_exit(&tq->tq_lock);
		return ((taskqid_t)tqe);
	}
	panic("taskq_dispatch TASKQ_DYNAMIC");
}

void
taskq_dispatch_ent(taskq_t *tq, task_func_t func, void *arg, uint_t flags,
    taskq_ent_t *tqe)
{
	ASSERT(func != NULL);
	ASSERT(!(tq->tq_flags & TASKQ_DYNAMIC));

	/*
	 * Mark it as a prealloc'd task.  This is important
	 * to ensure that we don't free it later.
	 */
	tqe->tqent_flags |= TQENT_FLAG_PREALLOC;
	/*
	 * Enqueue the task to the underlying queue.
	 */
	mutex_enter(&tq->tq_lock);

	if (flags & TQ_FRONT) {
		TQ_ENQUEUE_FRONT(tq, tqe, func, arg);
	} else {
		TQ_ENQUEUE(tq, tqe, func, arg);
	}
	mutex_exit(&tq->tq_lock);
}

/*
 * Wait for all pending tasks to complete.
 * Calling taskq_wait from a task will cause deadlock.
 */
void
taskq_wait(taskq_t *tq)
{

	mutex_enter(&tq->tq_lock);
	while (tq->tq_task.tqent_next != &tq->tq_task || tq->tq_active != 0)
		cv_wait(&tq->tq_wait_cv, &tq->tq_lock);
	mutex_exit(&tq->tq_lock);
}

/*
 * Suspend execution of tasks.
 *
 * Tasks in the queue part will be suspended immediately upon return from this
 * function. Pending tasks in the dynamic part will continue to execute, but all
 * new tasks will  be suspended.
 */
void
taskq_suspend(taskq_t *tq)
{
	rw_enter(&tq->tq_threadlock, RW_WRITER);

	/*
	 * Mark task queue as being suspended. Needed for taskq_suspended().
	 */
	mutex_enter(&tq->tq_lock);
	ASSERT(!(tq->tq_flags & TASKQ_SUSPENDED));
	tq->tq_flags |= TASKQ_SUSPENDED;
	mutex_exit(&tq->tq_lock);
}

/*
 * returns: 1 if tq is suspended, 0 otherwise.
 */
int
taskq_suspended(taskq_t *tq)
{
	return ((tq->tq_flags & TASKQ_SUSPENDED) != 0);
}

/*
 * Resume taskq execution.
 */
void
taskq_resume(taskq_t *tq)
{
	ASSERT(RW_WRITE_HELD(&tq->tq_threadlock));

	mutex_enter(&tq->tq_lock);
	ASSERT(tq->tq_flags & TASKQ_SUSPENDED);
	tq->tq_flags &= ~TASKQ_SUSPENDED;
	mutex_exit(&tq->tq_lock);

	rw_exit(&tq->tq_threadlock);
}

int
taskq_member(taskq_t *tq, kthread_t *thread)
{
	if (tq->tq_nthreads == 1)
		return (tq->tq_thread == thread);
	else {
		int i, found = 0;

		mutex_enter(&tq->tq_lock);
		for (i = 0; i < tq->tq_nthreads; i++) {
			if (tq->tq_threadlist[i] == thread) {
				found = 1;
				break;
			}
		}
		mutex_exit(&tq->tq_lock);
		return (found);
	}
}

/*
 * Creates a thread in the taskq.  We only allow one outstanding create at
 * a time.  We drop and reacquire the tq_lock in order to avoid blocking other
 * taskq activity while thread_create() or lwp_kernel_create() run.
 *
 * The first time we're called, we do some additional setup, and do not
 * return until there are enough threads to start servicing requests.
 */
static void
taskq_thread_create(taskq_t *tq)
{
	kthread_t	*t;
	const boolean_t	first = (tq->tq_nthreads == 0);

	ASSERT(MUTEX_HELD(&tq->tq_lock));
	ASSERT(tq->tq_flags & TASKQ_CHANGING);
	ASSERT(tq->tq_nthreads < tq->tq_nthreads_target);
	ASSERT(!(tq->tq_flags & TASKQ_THREAD_CREATED));


	tq->tq_flags |= TASKQ_THREAD_CREATED;
	tq->tq_active++;
	mutex_exit(&tq->tq_lock);

	/*
	 * With TASKQ_DUTY_CYCLE the new thread must have an LWP
	 * as explained in ../disp/sysdc.c (for the msacct data).
	 * Otherwise simple kthreads are preferred.
	 */
	if ((tq->tq_flags & TASKQ_DUTY_CYCLE) != 0) {
#ifdef illumos
		/* Enforced in taskq_create_common */
		ASSERT3P(tq->tq_proc, !=, &p0);
		t = lwp_kernel_create(tq->tq_proc, taskq_thread, tq, TS_RUN,
		    tq->tq_pri);
#endif
	} else {
		t = thread_create(NULL, 0, taskq_thread, tq, 0, tq->tq_proc,
		    TS_RUN, tq->tq_pri);
	}

	if (!first) {
		mutex_enter(&tq->tq_lock);
		return;
	}

	/*
	 * We know the thread cannot go away, since tq cannot be
	 * destroyed until creation has completed.  We can therefore
	 * safely dereference t.
	 */
	if (tq->tq_flags & TASKQ_THREADS_CPU_PCT) {
#ifdef __NetBSD__
		cpupart_t cpupart = { 0, ncpu };
		taskq_cpupct_install(tq, &cpupart);
#else
		taskq_cpupct_install(tq, t->t_cpupart);
#endif
	}
	mutex_enter(&tq->tq_lock);

	/* Wait until we can service requests. */
	while (tq->tq_nthreads != tq->tq_nthreads_target &&
	    tq->tq_nthreads < TASKQ_CREATE_ACTIVE_THREADS) {
		cv_wait(&tq->tq_wait_cv, &tq->tq_lock);
	}
}

/*
 * Common "sleep taskq thread" function, which handles CPR stuff, as well
 * as giving a nice common point for debuggers to find inactive threads.
 */
static clock_t
taskq_thread_wait(taskq_t *tq, kmutex_t *mx, kcondvar_t *cv,
    callb_cpr_t *cprinfo, clock_t timeout)
{
	clock_t ret = 0;

	if (!(tq->tq_flags & TASKQ_CPR_SAFE)) {
		CALLB_CPR_SAFE_BEGIN(cprinfo);
	}
	if (timeout < 0)
		cv_wait(cv, mx);
	else
		ret = cv_reltimedwait(cv, mx, timeout, TR_CLOCK_TICK);

	if (!(tq->tq_flags & TASKQ_CPR_SAFE)) {
		CALLB_CPR_SAFE_END(cprinfo, mx);
	}

	return (ret);
}

/*
 * Worker thread for processing task queue.
 */
static void
taskq_thread(void *arg)
{
	int thread_id;

	taskq_t *tq = arg;
	taskq_ent_t *tqe;
	callb_cpr_t cprinfo;
	hrtime_t start, end;
	boolean_t freeit;

	if (tq->tq_flags & TASKQ_CPR_SAFE) {
		CALLB_CPR_INIT_SAFE(curthread, tq->tq_name);
	} else {
		CALLB_CPR_INIT(&cprinfo, &tq->tq_lock, callb_generic_cpr,
		    tq->tq_name);
	}
	mutex_enter(&tq->tq_lock);
	thread_id = ++tq->tq_nthreads;
	ASSERT(tq->tq_flags & TASKQ_THREAD_CREATED);
	ASSERT(tq->tq_flags & TASKQ_CHANGING);
	tq->tq_flags &= ~TASKQ_THREAD_CREATED;

	VERIFY3S(thread_id, <=, tq->tq_nthreads_max);

	if (tq->tq_nthreads_max == 1)
		tq->tq_thread = curthread;
	else
		tq->tq_threadlist[thread_id - 1] = curthread;

	/* Allow taskq_create_common()'s taskq_thread_create() to return. */
	if (tq->tq_nthreads == TASKQ_CREATE_ACTIVE_THREADS)
		cv_broadcast(&tq->tq_wait_cv);

	for (;;) {
		if (tq->tq_flags & TASKQ_CHANGING) {
			/* See if we're no longer needed */
			if (thread_id > tq->tq_nthreads_target) {
				/*
				 * To preserve the one-to-one mapping between
				 * thread_id and thread, we must exit from
				 * highest thread ID to least.
				 *
				 * However, if everyone is exiting, the order
				 * doesn't matter, so just exit immediately.
				 * (this is safe, since you must wait for
				 * nthreads to reach 0 after setting
				 * tq_nthreads_target to 0)
				 */
				if (thread_id == tq->tq_nthreads ||
				    tq->tq_nthreads_target == 0)
					break;

				/* Wait for higher thread_ids to exit */
				(void) taskq_thread_wait(tq, &tq->tq_lock,
				    &tq->tq_exit_cv, &cprinfo, -1);
				continue;
			}

			/*
			 * If no thread is starting taskq_thread(), we can
			 * do some bookkeeping.
			 */
			if (!(tq->tq_flags & TASKQ_THREAD_CREATED)) {
				/* Check if we've reached our target */
				if (tq->tq_nthreads == tq->tq_nthreads_target) {
					tq->tq_flags &= ~TASKQ_CHANGING;
					cv_broadcast(&tq->tq_wait_cv);
				}
				/* Check if we need to create a thread */
				if (tq->tq_nthreads < tq->tq_nthreads_target) {
					taskq_thread_create(tq);
					continue; /* tq_lock was dropped */
				}
			}
		}
		if ((tqe = tq->tq_task.tqent_next) == &tq->tq_task) {
			if (--tq->tq_active == 0)
				cv_broadcast(&tq->tq_wait_cv);
			(void) taskq_thread_wait(tq, &tq->tq_lock,
			    &tq->tq_dispatch_cv, &cprinfo, -1);
			tq->tq_active++;
			continue;
		}

		tqe->tqent_prev->tqent_next = tqe->tqent_next;
		tqe->tqent_next->tqent_prev = tqe->tqent_prev;
		mutex_exit(&tq->tq_lock);

		/*
		 * For prealloc'd tasks, we don't free anything.  We
		 * have to check this now, because once we call the
		 * function for a prealloc'd taskq, we can't touch the
		 * tqent any longer (calling the function returns the
		 * ownershp of the tqent back to caller of
		 * taskq_dispatch.)
		 */
		if ((!(tq->tq_flags & TASKQ_DYNAMIC)) &&
		    (tqe->tqent_flags & TQENT_FLAG_PREALLOC)) {
			/* clear pointers to assist assertion checks */
			tqe->tqent_next = tqe->tqent_prev = NULL;
			freeit = B_FALSE;
		} else {
			freeit = B_TRUE;
		}
		rw_enter(&tq->tq_threadlock, RW_READER);
		start = gethrtime();
		DTRACE_PROBE2(taskq__exec__start, taskq_t *, tq,
		    taskq_ent_t *, tqe);
		tqe->tqent_func(tqe->tqent_arg);
		DTRACE_PROBE2(taskq__exec__end, taskq_t *, tq,
		    taskq_ent_t *, tqe);
		end = gethrtime();
		rw_exit(&tq->tq_threadlock);

		mutex_enter(&tq->tq_lock);
		tq->tq_totaltime += end - start;
		tq->tq_executed++;

		if (freeit)
			taskq_ent_free(tq, tqe);
	}
	if (tq->tq_nthreads_max == 1)
		tq->tq_thread = NULL;
	else
		tq->tq_threadlist[thread_id - 1] = NULL;

	/* We're exiting, and therefore no longer active */
	ASSERT(tq->tq_active > 0);
	tq->tq_active--;

	ASSERT(tq->tq_nthreads > 0);
	tq->tq_nthreads--;

	/* Wake up anyone waiting for us to exit */
	cv_broadcast(&tq->tq_exit_cv);
	if (tq->tq_nthreads == tq->tq_nthreads_target) {
		if (!(tq->tq_flags & TASKQ_THREAD_CREATED))
			tq->tq_flags &= ~TASKQ_CHANGING;

		cv_broadcast(&tq->tq_wait_cv);
	}

	ASSERT(!(tq->tq_flags & TASKQ_CPR_SAFE));
	CALLB_CPR_EXIT(&cprinfo);		/* drops tq->tq_lock */

	thread_exit();
}

/*
 * Taskq creation. May sleep for memory.
 * Always use automatically generated instances to avoid kstat name space
 * collisions.
 */

taskq_t *
taskq_create(const char *name, int nthreads, pri_t pri, int minalloc,
    int maxalloc, uint_t flags)
{
	return taskq_create_common(name, 0, nthreads, pri, minalloc,
	    maxalloc, flags | TASKQ_NOINSTANCE);
}

static taskq_t *
taskq_create_common(const char *name, int instance, int nthreads, pri_t pri,
    int minalloc, int maxalloc, uint_t flags)
{
	taskq_t *tq = kmem_cache_alloc(taskq_cache, KM_NOSLEEP);
	uint_t ncpus = ((boot_max_ncpus == -1) ? max_ncpus : boot_max_ncpus);
	uint_t bsize;	/* # of buckets - always power of 2 */
	int max_nthreads;

	ASSERT(instance == 0);
	ASSERT(!ISSET(flags, TASKQ_CPR_SAFE));
	ASSERT(!ISSET(flags, TASKQ_DYNAMIC));

	/*
	 * TASKQ_CPR_SAFE and TASKQ_DYNAMIC flags are mutually exclusive.
	 */
	ASSERT((flags & (TASKQ_DYNAMIC | TASKQ_CPR_SAFE)) !=
	    ((TASKQ_DYNAMIC | TASKQ_CPR_SAFE)));

	ASSERT(tq->tq_buckets == NULL);

	bsize = 1 << (highbit(ncpus) - 1);
	ASSERT(bsize >= 1);
	bsize = MIN(bsize, taskq_maxbuckets);

	ASSERT(!(flags & TASKQ_DYNAMIC));

	if (flags & TASKQ_THREADS_CPU_PCT) {
		uint_t pct;
		ASSERT3S(nthreads, >=, 0);
		pct = nthreads;

		if (pct > taskq_cpupct_max_percent)
			pct = taskq_cpupct_max_percent;

		/*
		 * If you're using THREADS_CPU_PCT, the process for the
		 * taskq threads must be curproc.  This allows any pset
		 * binding to be inherited correctly.  If proc is &p0,
		 * we won't be creating LWPs, so new threads will be assigned
		 * to the default processor set.
		 */
		tq->tq_threads_ncpus_pct = pct;
		nthreads = 1;		/* corrected in taskq_thread_create() */
		max_nthreads = TASKQ_THREADS_PCT(max_ncpus, pct);

	} else {
		ASSERT3S(nthreads, >=, 1);
		max_nthreads = nthreads;
	}

	if (max_nthreads < taskq_minimum_nthreads_max)
		max_nthreads = taskq_minimum_nthreads_max;

	/*
	 * Make sure the name is 0-terminated, and conforms to the rules for
	 * C indentifiers
	 */
	(void) strncpy(tq->tq_name, name, TASKQ_NAMELEN + 1);
	strident_canon(tq->tq_name, TASKQ_NAMELEN);

	tq->tq_flags = flags | TASKQ_CHANGING;
	tq->tq_active = 0;
	tq->tq_instance = instance;
	tq->tq_nthreads_target = nthreads;
	tq->tq_nthreads_max = max_nthreads;
	tq->tq_minalloc = minalloc;
	tq->tq_maxalloc = maxalloc;
	tq->tq_nbuckets = bsize;
	tq->tq_pri = pri;
	list_link_init(&tq->tq_cpupct_link);

	if (max_nthreads > 1)
		tq->tq_threadlist = kmem_alloc(
		    sizeof (kthread_t *) * max_nthreads, KM_SLEEP);

	mutex_enter(&tq->tq_lock);
	if (flags & TASKQ_PREPOPULATE) {
		while (minalloc-- > 0)
			taskq_ent_free(tq, taskq_ent_alloc(tq, TQ_SLEEP));
	}

	/*
	 * Create the first thread, which will create any other threads
	 * necessary.  taskq_thread_create will not return until we have
	 * enough threads to be able to process requests.
	 */
	taskq_thread_create(tq);
	mutex_exit(&tq->tq_lock);

	return (tq);
}

/*
 * taskq_destroy().
 *
 * Assumes: by the time taskq_destroy is called no one will use this task queue
 * in any way and no one will try to dispatch entries in it.
 */
void
taskq_destroy(taskq_t *tq)
{
	taskq_bucket_t *b = tq->tq_buckets;
	int bid = 0;

	ASSERT(! (tq->tq_flags & TASKQ_CPR_SAFE));

	/*
	 * Destroy kstats.
	 */
	if (tq->tq_kstat != NULL) {
		kstat_delete(tq->tq_kstat);
		tq->tq_kstat = NULL;
	}

	/*
	 * Unregister from the cpupct list.
	 */
	if (tq->tq_flags & TASKQ_THREADS_CPU_PCT) {
		taskq_cpupct_remove(tq);
	}

	/*
	 * Wait for any pending entries to complete.
	 */
	taskq_wait(tq);

	mutex_enter(&tq->tq_lock);
	ASSERT((tq->tq_task.tqent_next == &tq->tq_task) &&
	    (tq->tq_active == 0));

	/* notify all the threads that they need to exit */
	tq->tq_nthreads_target = 0;

	tq->tq_flags |= TASKQ_CHANGING;
	cv_broadcast(&tq->tq_dispatch_cv);
	cv_broadcast(&tq->tq_exit_cv);

	while (tq->tq_nthreads != 0)
		cv_wait(&tq->tq_wait_cv, &tq->tq_lock);

	if (tq->tq_nthreads_max != 1)
		kmem_free(tq->tq_threadlist, sizeof (kthread_t *) *
		    tq->tq_nthreads_max);

	tq->tq_minalloc = 0;
	while (tq->tq_nalloc != 0)
		taskq_ent_free(tq, taskq_ent_alloc(tq, TQ_SLEEP));

	mutex_exit(&tq->tq_lock);

	/*
	 * Mark each bucket as closing and wakeup all sleeping threads.
	 */
	for (; (b != NULL) && (bid < tq->tq_nbuckets); b++, bid++) {
		taskq_ent_t *tqe;

		mutex_enter(&b->tqbucket_lock);

		b->tqbucket_flags |= TQBUCKET_CLOSE;
		/* Wakeup all sleeping threads */

		for (tqe = b->tqbucket_freelist.tqent_next;
		    tqe != &b->tqbucket_freelist; tqe = tqe->tqent_next)
			cv_signal(&tqe->tqent_cv);

		ASSERT(b->tqbucket_nalloc == 0);

		/*
		 * At this point we waited for all pending jobs to complete (in
		 * both the task queue and the bucket and no new jobs should
		 * arrive. Wait for all threads to die.
		 */
		while (b->tqbucket_nfree > 0)
			cv_wait(&b->tqbucket_cv, &b->tqbucket_lock);
		mutex_exit(&b->tqbucket_lock);
		mutex_destroy(&b->tqbucket_lock);
		cv_destroy(&b->tqbucket_cv);
	}

	if (tq->tq_buckets != NULL) {
		ASSERT(tq->tq_flags & TASKQ_DYNAMIC);
		kmem_free(tq->tq_buckets,
		    sizeof (taskq_bucket_t) * tq->tq_nbuckets);

		/* Cleanup fields before returning tq to the cache */
		tq->tq_buckets = NULL;
		tq->tq_tcreates = 0;
		tq->tq_tdeaths = 0;
	} else {
		ASSERT(!(tq->tq_flags & TASKQ_DYNAMIC));
	}

	tq->tq_totaltime = 0;
	tq->tq_tasks = 0;
	tq->tq_maxtasks = 0;
	tq->tq_executed = 0;
	kmem_cache_free(taskq_cache, tq);
}
