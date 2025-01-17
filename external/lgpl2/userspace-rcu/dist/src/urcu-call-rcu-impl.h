// SPDX-FileCopyrightText: 2010 Paul E. McKenney <paulmck@linux.vnet.ibm.com>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

/*
 * Userspace RCU library - batch memory reclamation with kernel API
 */

#define _LGPL_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <poll.h>
#include <sys/time.h>
#include <unistd.h>
#include <sched.h>

#include "compat-getcpu.h"
#include <urcu/assert.h>
#include <urcu/wfcqueue.h>
#include <urcu/call-rcu.h>
#include <urcu/pointer.h>
#include <urcu/list.h>
#include <urcu/futex.h>
#include <urcu/tls-compat.h>
#include <urcu/ref.h>
#include "urcu-die.h"
#include "urcu-utils.h"
#include "compat-smp.h"

#define SET_AFFINITY_CHECK_PERIOD		(1U << 8)	/* 256 */
#define SET_AFFINITY_CHECK_PERIOD_MASK		(SET_AFFINITY_CHECK_PERIOD - 1)

/* Data structure that identifies a call_rcu thread. */

struct call_rcu_data {
	/*
	 * We do not align head on a different cache-line than tail
	 * mainly because call_rcu callback-invocation threads use
	 * batching ("splice") to get an entire list of callbacks, which
	 * effectively empties the queue, and requires to touch the tail
	 * anyway.
	 */
	struct cds_wfcq_tail cbs_tail;
	struct cds_wfcq_head cbs_head;
	unsigned long flags;
	int32_t futex;
	unsigned long qlen; /* maintained for debugging. */
	pthread_t tid;
	int cpu_affinity;
	unsigned long gp_count;
	struct cds_list_head list;
} __attribute__((aligned(CAA_CACHE_LINE_SIZE)));

struct call_rcu_completion {
	int barrier_count;
	int32_t futex;
	struct urcu_ref ref;
};

struct call_rcu_completion_work {
	struct rcu_head head;
	struct call_rcu_completion *completion;
};

enum crdf_flags {
	CRDF_FLAG_JOIN_THREAD = (1 << 0),
};

/*
 * List of all call_rcu_data structures to keep valgrind happy.
 * Protected by call_rcu_mutex.
 */

static CDS_LIST_HEAD(call_rcu_data_list);

/* Link a thread using call_rcu() to its call_rcu thread. */

static DEFINE_URCU_TLS(struct call_rcu_data *, thread_call_rcu_data);

/*
 * Guard call_rcu thread creation and atfork handlers.
 */
static pthread_mutex_t call_rcu_mutex = PTHREAD_MUTEX_INITIALIZER;

/* If a given thread does not have its own call_rcu thread, this is default. */

static struct call_rcu_data *default_call_rcu_data;

static struct urcu_atfork *registered_rculfhash_atfork;

/*
 * If the sched_getcpu() and sysconf(_SC_NPROCESSORS_CONF) calls are
 * available, then we can have call_rcu threads assigned to individual
 * CPUs rather than only to specific threads.
 */

#if defined(HAVE_SYSCONF) && (defined(HAVE_SCHED_GETCPU) || defined(HAVE_GETCPUID))

/*
 * Pointer to array of pointers to per-CPU call_rcu_data structures
 * and # CPUs. per_cpu_call_rcu_data is a RCU-protected pointer to an
 * array of RCU-protected pointers to call_rcu_data. call_rcu acts as a
 * RCU read-side and reads per_cpu_call_rcu_data and the per-cpu pointer
 * without mutex. The call_rcu_mutex protects updates.
 */

static struct call_rcu_data **per_cpu_call_rcu_data;
static long cpus_array_len;

static void cpus_array_len_reset(void)
{
	cpus_array_len = 0;
}

/* Allocate the array if it has not already been allocated. */

static void alloc_cpu_call_rcu_data(void)
{
	struct call_rcu_data **p;
	static int warned = 0;

	if (cpus_array_len != 0)
		return;
	cpus_array_len = get_possible_cpus_array_len();
	if (cpus_array_len <= 0) {
		return;
	}
	p = malloc(cpus_array_len * sizeof(*per_cpu_call_rcu_data));
	if (p != NULL) {
		memset(p, '\0', cpus_array_len * sizeof(*per_cpu_call_rcu_data));
		rcu_set_pointer(&per_cpu_call_rcu_data, p);
	} else {
		if (!warned) {
			fprintf(stderr, "[error] liburcu: unable to allocate per-CPU pointer array\n");
		}
		warned = 1;
	}
}

#else /* #if defined(HAVE_SYSCONF) && defined(HAVE_SCHED_GETCPU) */

/*
 * per_cpu_call_rcu_data should be constant, but some functions below, used both
 * for cases where cpu number is available and not available, assume it it not
 * constant.
 */
static struct call_rcu_data **per_cpu_call_rcu_data = NULL;
static const long cpus_array_len = -1;

static void cpus_array_len_reset(void)
{
}

static void alloc_cpu_call_rcu_data(void)
{
}

#endif /* #else #if defined(HAVE_SYSCONF) && defined(HAVE_SCHED_GETCPU) */

/* Acquire the specified pthread mutex. */

static void call_rcu_lock(pthread_mutex_t *pmp)
{
	int ret;

	ret = pthread_mutex_lock(pmp);
	if (ret)
		urcu_die(ret);
}

/* Release the specified pthread mutex. */

static void call_rcu_unlock(pthread_mutex_t *pmp)
{
	int ret;

	ret = pthread_mutex_unlock(pmp);
	if (ret)
		urcu_die(ret);
}

/*
 * Periodically retry setting CPU affinity if we migrate.
 * Losing affinity can be caused by CPU hotunplug/hotplug, or by
 * cpuset(7).
 */
#ifdef HAVE_SCHED_SETAFFINITY
static
int set_thread_cpu_affinity(struct call_rcu_data *crdp)
{
	cpu_set_t mask;
	int ret;

	if (crdp->cpu_affinity < 0)
		return 0;
	if (++crdp->gp_count & SET_AFFINITY_CHECK_PERIOD_MASK)
		return 0;
	if (urcu_sched_getcpu() == crdp->cpu_affinity)
		return 0;

	CPU_ZERO(&mask);
	CPU_SET(crdp->cpu_affinity, &mask);
	ret = sched_setaffinity(0, sizeof(mask), &mask);

	/*
	 * EINVAL is fine: can be caused by hotunplugged CPUs, or by
	 * cpuset(7). This is why we should always retry if we detect
	 * migration.
	 */
	if (ret && errno == EINVAL) {
		ret = 0;
		errno = 0;
	}
	return ret;
}
#else
static
int set_thread_cpu_affinity(struct call_rcu_data *crdp __attribute__((unused)))
{
	return 0;
}
#endif

static void call_rcu_wait(struct call_rcu_data *crdp)
{
	/* Read call_rcu list before read futex */
	cmm_smp_mb();
	while (uatomic_read(&crdp->futex) == -1) {
		if (!futex_async(&crdp->futex, FUTEX_WAIT, -1, NULL, NULL, 0)) {
			/*
			 * Prior queued wakeups queued by unrelated code
			 * using the same address can cause futex wait to
			 * return 0 even through the futex value is still
			 * -1 (spurious wakeups). Check the value again
			 * in user-space to validate whether it really
			 * differs from -1.
			 */
			continue;
		}
		switch (errno) {
		case EAGAIN:
			/* Value already changed. */
			return;
		case EINTR:
			/* Retry if interrupted by signal. */
			break;	/* Get out of switch. Check again. */
		default:
			/* Unexpected error. */
			urcu_die(errno);
		}
	}
}

static void call_rcu_wake_up(struct call_rcu_data *crdp)
{
	/* Write to call_rcu list before reading/writing futex */
	cmm_smp_mb();
	if (caa_unlikely(uatomic_read(&crdp->futex) == -1)) {
		uatomic_set(&crdp->futex, 0);
		if (futex_async(&crdp->futex, FUTEX_WAKE, 1,
				NULL, NULL, 0) < 0)
			urcu_die(errno);
	}
}

static void call_rcu_completion_wait(struct call_rcu_completion *completion)
{
	/* Read completion barrier count before read futex */
	cmm_smp_mb();
	while (uatomic_read(&completion->futex) == -1) {
		if (!futex_async(&completion->futex, FUTEX_WAIT, -1, NULL, NULL, 0)) {
			/*
			 * Prior queued wakeups queued by unrelated code
			 * using the same address can cause futex wait to
			 * return 0 even through the futex value is still
			 * -1 (spurious wakeups). Check the value again
			 * in user-space to validate whether it really
			 * differs from -1.
			 */
			continue;
		}
		switch (errno) {
		case EAGAIN:
			/* Value already changed. */
			return;
		case EINTR:
			/* Retry if interrupted by signal. */
			break;	/* Get out of switch. Check again. */
		default:
			/* Unexpected error. */
			urcu_die(errno);
		}
	}
}

static void call_rcu_completion_wake_up(struct call_rcu_completion *completion)
{
	/* Write to completion barrier count before reading/writing futex */
	cmm_smp_mb();
	if (caa_unlikely(uatomic_read(&completion->futex) == -1)) {
		uatomic_set(&completion->futex, 0);
		if (futex_async(&completion->futex, FUTEX_WAKE, 1,
				NULL, NULL, 0) < 0)
			urcu_die(errno);
	}
}

/* This is the code run by each call_rcu thread. */

static void *call_rcu_thread(void *arg)
{
	unsigned long cbcount;
	struct call_rcu_data *crdp = (struct call_rcu_data *) arg;
	int rt = !!(uatomic_read(&crdp->flags) & URCU_CALL_RCU_RT);

	if (set_thread_cpu_affinity(crdp))
		urcu_die(errno);

	/*
	 * If callbacks take a read-side lock, we need to be registered.
	 */
	rcu_register_thread();

	URCU_TLS(thread_call_rcu_data) = crdp;
	if (!rt) {
		uatomic_dec(&crdp->futex);
		/* Decrement futex before reading call_rcu list */
		cmm_smp_mb();
	}
	for (;;) {
		struct cds_wfcq_head cbs_tmp_head;
		struct cds_wfcq_tail cbs_tmp_tail;
		struct cds_wfcq_node *cbs, *cbs_tmp_n;
		enum cds_wfcq_ret splice_ret;

		if (set_thread_cpu_affinity(crdp))
			urcu_die(errno);

		if (uatomic_read(&crdp->flags) & URCU_CALL_RCU_PAUSE) {
			/*
			 * Pause requested. Become quiescent: remove
			 * ourself from all global lists, and don't
			 * process any callback. The callback lists may
			 * still be non-empty though.
			 */
			rcu_unregister_thread();
			cmm_smp_mb__before_uatomic_or();
			uatomic_or(&crdp->flags, URCU_CALL_RCU_PAUSED);
			while ((uatomic_read(&crdp->flags) & URCU_CALL_RCU_PAUSE) != 0)
				(void) poll(NULL, 0, 1);
			uatomic_and(&crdp->flags, ~URCU_CALL_RCU_PAUSED);
			cmm_smp_mb__after_uatomic_and();
			rcu_register_thread();
		}

		cds_wfcq_init(&cbs_tmp_head, &cbs_tmp_tail);
		splice_ret = __cds_wfcq_splice_blocking(&cbs_tmp_head,
			&cbs_tmp_tail, &crdp->cbs_head, &crdp->cbs_tail);
		urcu_posix_assert(splice_ret != CDS_WFCQ_RET_WOULDBLOCK);
		urcu_posix_assert(splice_ret != CDS_WFCQ_RET_DEST_NON_EMPTY);
		if (splice_ret != CDS_WFCQ_RET_SRC_EMPTY) {
			synchronize_rcu();
			cbcount = 0;
			__cds_wfcq_for_each_blocking_safe(&cbs_tmp_head,
					&cbs_tmp_tail, cbs, cbs_tmp_n) {
				struct rcu_head *rhp;

				rhp = caa_container_of(cbs,
					struct rcu_head, next);
				rhp->func(rhp);
				cbcount++;
			}
			uatomic_sub(&crdp->qlen, cbcount);
		}
		if (uatomic_read(&crdp->flags) & URCU_CALL_RCU_STOP)
			break;
		rcu_thread_offline();
		if (!rt) {
			if (cds_wfcq_empty(&crdp->cbs_head,
					&crdp->cbs_tail)) {
				call_rcu_wait(crdp);
				(void) poll(NULL, 0, 10);
				uatomic_dec(&crdp->futex);
				/*
				 * Decrement futex before reading
				 * call_rcu list.
				 */
				cmm_smp_mb();
			} else {
				(void) poll(NULL, 0, 10);
			}
		} else {
			(void) poll(NULL, 0, 10);
		}
		rcu_thread_online();
	}
	if (!rt) {
		/*
		 * Read call_rcu list before write futex.
		 */
		cmm_smp_mb();
		uatomic_set(&crdp->futex, 0);
	}
	uatomic_or(&crdp->flags, URCU_CALL_RCU_STOPPED);
	rcu_unregister_thread();
	return NULL;
}

/*
 * Create both a call_rcu thread and the corresponding call_rcu_data
 * structure, linking the structure in as specified.  Caller must hold
 * call_rcu_mutex.
 */

static void call_rcu_data_init(struct call_rcu_data **crdpp,
			       unsigned long flags,
			       int cpu_affinity)
{
	struct call_rcu_data *crdp;
	int ret;
	sigset_t newmask, oldmask;

	crdp = malloc(sizeof(*crdp));
	if (crdp == NULL)
		urcu_die(errno);
	memset(crdp, '\0', sizeof(*crdp));
	cds_wfcq_init(&crdp->cbs_head, &crdp->cbs_tail);
	crdp->qlen = 0;
	crdp->futex = 0;
	crdp->flags = flags;
	cds_list_add(&crdp->list, &call_rcu_data_list);
	crdp->cpu_affinity = cpu_affinity;
	crdp->gp_count = 0;
	rcu_set_pointer(crdpp, crdp);

	ret = sigfillset(&newmask);
	urcu_posix_assert(!ret);
	ret = pthread_sigmask(SIG_BLOCK, &newmask, &oldmask);
	urcu_posix_assert(!ret);

	ret = pthread_create(&crdp->tid, NULL, call_rcu_thread, crdp);
	if (ret)
		urcu_die(ret);

	ret = pthread_sigmask(SIG_SETMASK, &oldmask, NULL);
	urcu_posix_assert(!ret);
}

/*
 * Return a pointer to the call_rcu_data structure for the specified
 * CPU, returning NULL if there is none.  We cannot automatically
 * created it because the platform we are running on might not define
 * urcu_sched_getcpu().
 *
 * The call to this function and use of the returned call_rcu_data
 * should be protected by RCU read-side lock.
 */

struct call_rcu_data *get_cpu_call_rcu_data(int cpu)
{
	static int warned = 0;
	struct call_rcu_data **pcpu_crdp;

	pcpu_crdp = rcu_dereference(per_cpu_call_rcu_data);
	if (pcpu_crdp == NULL)
		return NULL;
	if (!warned && cpus_array_len > 0 && (cpu < 0 || cpus_array_len <= cpu)) {
		fprintf(stderr, "[error] liburcu: get CPU # out of range\n");
		warned = 1;
	}
	if (cpu < 0 || cpus_array_len <= cpu)
		return NULL;
	return rcu_dereference(pcpu_crdp[cpu]);
}

/*
 * Return the tid corresponding to the call_rcu thread whose
 * call_rcu_data structure is specified.
 */

pthread_t get_call_rcu_thread(struct call_rcu_data *crdp)
{
	return crdp->tid;
}

/*
 * Create a call_rcu_data structure (with thread) and return a pointer.
 */

static struct call_rcu_data *__create_call_rcu_data(unsigned long flags,
						    int cpu_affinity)
{
	struct call_rcu_data *crdp;

	call_rcu_data_init(&crdp, flags, cpu_affinity);
	return crdp;
}

struct call_rcu_data *create_call_rcu_data(unsigned long flags,
					   int cpu_affinity)
{
	struct call_rcu_data *crdp;

	call_rcu_lock(&call_rcu_mutex);
	crdp = __create_call_rcu_data(flags, cpu_affinity);
	call_rcu_unlock(&call_rcu_mutex);
	return crdp;
}

/*
 * Set the specified CPU to use the specified call_rcu_data structure.
 *
 * Use NULL to remove a CPU's call_rcu_data structure, but it is
 * the caller's responsibility to dispose of the removed structure.
 * Use get_cpu_call_rcu_data() to obtain a pointer to the old structure
 * (prior to NULLing it out, of course).
 *
 * The caller must wait for a grace-period to pass between return from
 * set_cpu_call_rcu_data() and call to call_rcu_data_free() passing the
 * previous call rcu data as argument.
 */

int set_cpu_call_rcu_data(int cpu, struct call_rcu_data *crdp)
{
	static int warned = 0;

	call_rcu_lock(&call_rcu_mutex);
	alloc_cpu_call_rcu_data();
	if (cpu < 0 || cpus_array_len <= cpu) {
		if (!warned) {
			fprintf(stderr, "[error] liburcu: set CPU # out of range\n");
			warned = 1;
		}
		call_rcu_unlock(&call_rcu_mutex);
		errno = EINVAL;
		return -EINVAL;
	}

	if (per_cpu_call_rcu_data == NULL) {
		call_rcu_unlock(&call_rcu_mutex);
		errno = ENOMEM;
		return -ENOMEM;
	}

	if (per_cpu_call_rcu_data[cpu] != NULL && crdp != NULL) {
		call_rcu_unlock(&call_rcu_mutex);
		errno = EEXIST;
		return -EEXIST;
	}

	rcu_set_pointer(&per_cpu_call_rcu_data[cpu], crdp);
	call_rcu_unlock(&call_rcu_mutex);
	return 0;
}

/*
 * Return a pointer to the default call_rcu_data structure, creating
 * one if need be.
 *
 * The call to this function with intent to use the returned
 * call_rcu_data should be protected by RCU read-side lock.
 */

struct call_rcu_data *get_default_call_rcu_data(void)
{
	struct call_rcu_data *crdp;

	crdp = rcu_dereference(default_call_rcu_data);
	if (crdp != NULL)
		return crdp;

	call_rcu_lock(&call_rcu_mutex);
	if (default_call_rcu_data == NULL)
		call_rcu_data_init(&default_call_rcu_data, 0, -1);
	crdp = default_call_rcu_data;
	call_rcu_unlock(&call_rcu_mutex);

	return crdp;
}

/*
 * Return the call_rcu_data structure that applies to the currently
 * running thread.  Any call_rcu_data structure assigned specifically
 * to this thread has first priority, followed by any call_rcu_data
 * structure assigned to the CPU on which the thread is running,
 * followed by the default call_rcu_data structure.  If there is not
 * yet a default call_rcu_data structure, one will be created.
 *
 * Calls to this function and use of the returned call_rcu_data should
 * be protected by RCU read-side lock.
 */
struct call_rcu_data *get_call_rcu_data(void)
{
	struct call_rcu_data *crd;

	if (URCU_TLS(thread_call_rcu_data) != NULL)
		return URCU_TLS(thread_call_rcu_data);

	if (cpus_array_len > 0) {
		crd = get_cpu_call_rcu_data(urcu_sched_getcpu());
		if (crd)
			return crd;
	}

	return get_default_call_rcu_data();
}

/*
 * Return a pointer to this task's call_rcu_data if there is one.
 */

struct call_rcu_data *get_thread_call_rcu_data(void)
{
	return URCU_TLS(thread_call_rcu_data);
}

/*
 * Set this task's call_rcu_data structure as specified, regardless
 * of whether or not this task already had one.  (This allows switching
 * to and from real-time call_rcu threads, for example.)
 *
 * Use NULL to remove a thread's call_rcu_data structure, but it is
 * the caller's responsibility to dispose of the removed structure.
 * Use get_thread_call_rcu_data() to obtain a pointer to the old structure
 * (prior to NULLing it out, of course).
 */

void set_thread_call_rcu_data(struct call_rcu_data *crdp)
{
	URCU_TLS(thread_call_rcu_data) = crdp;
}

/*
 * Create a separate call_rcu thread for each CPU.  This does not
 * replace a pre-existing call_rcu thread -- use the set_cpu_call_rcu_data()
 * function if you want that behavior. Should be paired with
 * free_all_cpu_call_rcu_data() to teardown these call_rcu worker
 * threads.
 */

int create_all_cpu_call_rcu_data(unsigned long flags)
{
	int i;
	struct call_rcu_data *crdp;
	int ret;

	call_rcu_lock(&call_rcu_mutex);
	alloc_cpu_call_rcu_data();
	call_rcu_unlock(&call_rcu_mutex);
	if (cpus_array_len <= 0) {
		errno = EINVAL;
		return -EINVAL;
	}
	if (per_cpu_call_rcu_data == NULL) {
		errno = ENOMEM;
		return -ENOMEM;
	}
	for (i = 0; i < cpus_array_len; i++) {
		call_rcu_lock(&call_rcu_mutex);
		if (get_cpu_call_rcu_data(i)) {
			call_rcu_unlock(&call_rcu_mutex);
			continue;
		}
		crdp = __create_call_rcu_data(flags, i);
		if (crdp == NULL) {
			call_rcu_unlock(&call_rcu_mutex);
			errno = ENOMEM;
			return -ENOMEM;
		}
		call_rcu_unlock(&call_rcu_mutex);
		if ((ret = set_cpu_call_rcu_data(i, crdp)) != 0) {
			call_rcu_data_free(crdp);

			/* it has been created by other thread */
			if (ret == -EEXIST)
				continue;

			return ret;
		}
	}
	return 0;
}

/*
 * Wake up the call_rcu thread corresponding to the specified
 * call_rcu_data structure.
 */
static void wake_call_rcu_thread(struct call_rcu_data *crdp)
{
	if (!(_CMM_LOAD_SHARED(crdp->flags) & URCU_CALL_RCU_RT))
		call_rcu_wake_up(crdp);
}

static void _call_rcu(struct rcu_head *head,
		      void (*func)(struct rcu_head *head),
		      struct call_rcu_data *crdp)
{
	cds_wfcq_node_init(&head->next);
	head->func = func;
	cds_wfcq_enqueue(&crdp->cbs_head, &crdp->cbs_tail, &head->next);
	uatomic_inc(&crdp->qlen);
	wake_call_rcu_thread(crdp);
}

/*
 * Schedule a function to be invoked after a following grace period.
 * This is the only function that must be called -- the others are
 * only present to allow applications to tune their use of RCU for
 * maximum performance.
 *
 * Note that unless a call_rcu thread has not already been created,
 * the first invocation of call_rcu() will create one.  So, if you
 * need the first invocation of call_rcu() to be fast, make sure
 * to create a call_rcu thread first.  One way to accomplish this is
 * "get_call_rcu_data();", and another is create_all_cpu_call_rcu_data().
 *
 * call_rcu must be called by registered RCU read-side threads.
 */
void call_rcu(struct rcu_head *head,
	      void (*func)(struct rcu_head *head))
{
	struct call_rcu_data *crdp;

	/* Holding rcu read-side lock across use of per-cpu crdp */
	_rcu_read_lock();
	crdp = get_call_rcu_data();
	_call_rcu(head, func, crdp);
	_rcu_read_unlock();
}

/*
 * Free up the specified call_rcu_data structure, terminating the
 * associated call_rcu thread.  The caller must have previously
 * removed the call_rcu_data structure from per-thread or per-CPU
 * usage.  For example, set_cpu_call_rcu_data(cpu, NULL) for per-CPU
 * call_rcu_data structures or set_thread_call_rcu_data(NULL) for
 * per-thread call_rcu_data structures.
 *
 * We silently refuse to free up the default call_rcu_data structure
 * because that is where we put any leftover callbacks.  Note that
 * the possibility of self-spawning callbacks makes it impossible
 * to execute all the callbacks in finite time without putting any
 * newly spawned callbacks somewhere else.  The "somewhere else" of
 * last resort is the default call_rcu_data structure.
 *
 * We also silently refuse to free NULL pointers.  This simplifies
 * the calling code.
 *
 * The caller must wait for a grace-period to pass between return from
 * set_cpu_call_rcu_data() and call to call_rcu_data_free() passing the
 * previous call rcu data as argument.
 *
 * Note: introducing __cds_wfcq_splice_blocking() in this function fixed
 * a list corruption bug in the 0.7.x series. The equivalent fix
 * appeared in 0.6.8 for the stable-0.6 branch.
 */
static
void _call_rcu_data_free(struct call_rcu_data *crdp, unsigned int flags)
{
	if (crdp == NULL || crdp == default_call_rcu_data) {
		return;
	}
	if ((uatomic_read(&crdp->flags) & URCU_CALL_RCU_STOPPED) == 0) {
		uatomic_or(&crdp->flags, URCU_CALL_RCU_STOP);
		wake_call_rcu_thread(crdp);
		while ((uatomic_read(&crdp->flags) & URCU_CALL_RCU_STOPPED) == 0)
			(void) poll(NULL, 0, 1);
	}
	call_rcu_lock(&call_rcu_mutex);
	if (!cds_wfcq_empty(&crdp->cbs_head, &crdp->cbs_tail)) {
		call_rcu_unlock(&call_rcu_mutex);
		/* Create default call rcu data if need be. */
		/* CBs queued here will be handed to the default list. */
		(void) get_default_call_rcu_data();
		call_rcu_lock(&call_rcu_mutex);
		__cds_wfcq_splice_blocking(&default_call_rcu_data->cbs_head,
			&default_call_rcu_data->cbs_tail,
			&crdp->cbs_head, &crdp->cbs_tail);
		uatomic_add(&default_call_rcu_data->qlen,
			    uatomic_read(&crdp->qlen));
		wake_call_rcu_thread(default_call_rcu_data);
	}

	cds_list_del(&crdp->list);
	call_rcu_unlock(&call_rcu_mutex);

	if (flags & CRDF_FLAG_JOIN_THREAD) {
		int ret;

		ret = pthread_join(get_call_rcu_thread(crdp), NULL);
		if (ret)
			urcu_die(ret);
	}
	free(crdp);
}

void call_rcu_data_free(struct call_rcu_data *crdp)
{
	_call_rcu_data_free(crdp, CRDF_FLAG_JOIN_THREAD);
}

/*
 * Clean up all the per-CPU call_rcu threads.
 */
void free_all_cpu_call_rcu_data(void)
{
	int cpu;
	struct call_rcu_data **crdp;
	static int warned = 0;

	if (cpus_array_len <= 0)
		return;

	crdp = malloc(sizeof(*crdp) * cpus_array_len);
	if (!crdp) {
		if (!warned) {
			fprintf(stderr, "[error] liburcu: unable to allocate per-CPU pointer array\n");
		}
		warned = 1;
		return;
	}

	for (cpu = 0; cpu < cpus_array_len; cpu++) {
		crdp[cpu] = get_cpu_call_rcu_data(cpu);
		if (crdp[cpu] == NULL)
			continue;
		set_cpu_call_rcu_data(cpu, NULL);
	}
	/*
	 * Wait for call_rcu sites acting as RCU readers of the
	 * call_rcu_data to become quiescent.
	 */
	synchronize_rcu();
	for (cpu = 0; cpu < cpus_array_len; cpu++) {
		if (crdp[cpu] == NULL)
			continue;
		call_rcu_data_free(crdp[cpu]);
	}
	free(crdp);
}

static
void free_completion(struct urcu_ref *ref)
{
	struct call_rcu_completion *completion;

	completion = caa_container_of(ref, struct call_rcu_completion, ref);
	free(completion);
}

static
void _rcu_barrier_complete(struct rcu_head *head)
{
	struct call_rcu_completion_work *work;
	struct call_rcu_completion *completion;

	work = caa_container_of(head, struct call_rcu_completion_work, head);
	completion = work->completion;
	if (!uatomic_sub_return(&completion->barrier_count, 1))
		call_rcu_completion_wake_up(completion);
	urcu_ref_put(&completion->ref, free_completion);
	free(work);
}

/*
 * Wait for all in-flight call_rcu callbacks to complete execution.
 */
void rcu_barrier(void)
{
	struct call_rcu_data *crdp;
	struct call_rcu_completion *completion;
	int count = 0;
	int was_online;

	/* Put in offline state in QSBR. */
	was_online = _rcu_read_ongoing();
	if (was_online)
		rcu_thread_offline();
	/*
	 * Calling a rcu_barrier() within a RCU read-side critical
	 * section is an error.
	 */
	if (_rcu_read_ongoing()) {
		static int warned = 0;

		if (!warned) {
			fprintf(stderr, "[error] liburcu: rcu_barrier() called from within RCU read-side critical section.\n");
		}
		warned = 1;
		goto online;
	}

	completion = calloc(1, sizeof(*completion));
	if (!completion)
		urcu_die(errno);

	call_rcu_lock(&call_rcu_mutex);
	cds_list_for_each_entry(crdp, &call_rcu_data_list, list)
		count++;

	/* Referenced by rcu_barrier() and each call_rcu thread. */
	urcu_ref_set(&completion->ref, count + 1);
	completion->barrier_count = count;

	cds_list_for_each_entry(crdp, &call_rcu_data_list, list) {
		struct call_rcu_completion_work *work;

		work = calloc(1, sizeof(*work));
		if (!work)
			urcu_die(errno);
		work->completion = completion;
		_call_rcu(&work->head, _rcu_barrier_complete, crdp);
	}
	call_rcu_unlock(&call_rcu_mutex);

	/* Wait for them */
	for (;;) {
		uatomic_dec(&completion->futex);
		/* Decrement futex before reading barrier_count */
		cmm_smp_mb();
		if (!uatomic_read(&completion->barrier_count))
			break;
		call_rcu_completion_wait(completion);
	}

	urcu_ref_put(&completion->ref, free_completion);

online:
	if (was_online)
		rcu_thread_online();
}

/*
 * Acquire the call_rcu_mutex in order to ensure that the child sees
 * all of the call_rcu() data structures in a consistent state. Ensure
 * that all call_rcu threads are in a quiescent state across fork.
 * Suitable for pthread_atfork() and friends.
 */
void call_rcu_before_fork(void)
{
	struct call_rcu_data *crdp;
	struct urcu_atfork *atfork;

	call_rcu_lock(&call_rcu_mutex);

	atfork = registered_rculfhash_atfork;
	if (atfork)
		atfork->before_fork(atfork->priv);

	cds_list_for_each_entry(crdp, &call_rcu_data_list, list) {
		uatomic_or(&crdp->flags, URCU_CALL_RCU_PAUSE);
		cmm_smp_mb__after_uatomic_or();
		wake_call_rcu_thread(crdp);
	}
	cds_list_for_each_entry(crdp, &call_rcu_data_list, list) {
		while ((uatomic_read(&crdp->flags) & URCU_CALL_RCU_PAUSED) == 0)
			(void) poll(NULL, 0, 1);
	}
}

/*
 * Clean up call_rcu data structures in the parent of a successful fork()
 * that is not followed by exec() in the child.  Suitable for
 * pthread_atfork() and friends.
 */
void call_rcu_after_fork_parent(void)
{
	struct call_rcu_data *crdp;
	struct urcu_atfork *atfork;

	cds_list_for_each_entry(crdp, &call_rcu_data_list, list)
		uatomic_and(&crdp->flags, ~URCU_CALL_RCU_PAUSE);
	cds_list_for_each_entry(crdp, &call_rcu_data_list, list) {
		while ((uatomic_read(&crdp->flags) & URCU_CALL_RCU_PAUSED) != 0)
			(void) poll(NULL, 0, 1);
	}
	atfork = registered_rculfhash_atfork;
	if (atfork)
		atfork->after_fork_parent(atfork->priv);
	call_rcu_unlock(&call_rcu_mutex);
}

/*
 * Clean up call_rcu data structures in the child of a successful fork()
 * that is not followed by exec().  Suitable for pthread_atfork() and
 * friends.
 */
void call_rcu_after_fork_child(void)
{
	struct call_rcu_data *crdp, *next;
	struct urcu_atfork *atfork;

	/* Release the mutex. */
	call_rcu_unlock(&call_rcu_mutex);

	atfork = registered_rculfhash_atfork;
	if (atfork)
		atfork->after_fork_child(atfork->priv);

	/* Do nothing when call_rcu() has not been used */
	if (cds_list_empty(&call_rcu_data_list))
		return;

	/*
	 * Allocate a new default call_rcu_data structure in order
	 * to get a working call_rcu thread to go with it.
	 */
	default_call_rcu_data = NULL;
	(void)get_default_call_rcu_data();

	/* Cleanup call_rcu_data pointers before use */
	cpus_array_len_reset();
	free(per_cpu_call_rcu_data);
	rcu_set_pointer(&per_cpu_call_rcu_data, NULL);
	URCU_TLS(thread_call_rcu_data) = NULL;

	/*
	 * Dispose of all of the rest of the call_rcu_data structures.
	 * Leftover call_rcu callbacks will be merged into the new
	 * default call_rcu thread queue.
	 */
	cds_list_for_each_entry_safe(crdp, next, &call_rcu_data_list, list) {
		if (crdp == default_call_rcu_data)
			continue;
		uatomic_set(&crdp->flags, URCU_CALL_RCU_STOPPED);
		/*
		 * Do not join the thread because it does not exist in
		 * the child.
		 */
		_call_rcu_data_free(crdp, 0);
	}
}

void urcu_register_rculfhash_atfork(struct urcu_atfork *atfork)
{
	if (CMM_LOAD_SHARED(registered_rculfhash_atfork))
		return;
	call_rcu_lock(&call_rcu_mutex);
	if (!registered_rculfhash_atfork)
		registered_rculfhash_atfork = atfork;
	call_rcu_unlock(&call_rcu_mutex);
}

/*
 * This unregistration function is deprecated, meant only for internal
 * use by rculfhash.
 */
__attribute__((__noreturn__))
void urcu_unregister_rculfhash_atfork(struct urcu_atfork *atfork __attribute__((unused)))
{
	urcu_die(EPERM);
}

/*
 * Teardown the default call_rcu worker thread if there are no queued
 * callbacks on process exit. This prevents leaking memory.
 *
 * Here is how an application can ensure graceful teardown of this
 * worker thread:
 *
 * - An application queuing call_rcu callbacks should invoke
 *   rcu_barrier() before it exits.
 * - When chaining call_rcu callbacks, the number of calls to
 *   rcu_barrier() on application exit must match at least the maximum
 *   number of chained callbacks.
 * - If an application chains callbacks endlessly, it would have to be
 *   modified to stop chaining callbacks when it detects an application
 *   exit (e.g. with a flag), and wait for quiescence with rcu_barrier()
 *   after setting that flag.
 * - The statements above apply to a library which queues call_rcu
 *   callbacks, only it needs to invoke rcu_barrier in its library
 *   destructor.
 *
 * Note that this function does not presume it is being called when the
 * application is single-threaded even though this is invoked from a
 * destructor: this function synchronizes against concurrent calls to
 * get_default_call_rcu_data().
 */
static void urcu_call_rcu_exit(void)
{
	struct call_rcu_data *crdp;
	bool teardown = true;

	if (default_call_rcu_data == NULL)
		return;
	call_rcu_lock(&call_rcu_mutex);
	/*
	 * If the application leaves callbacks in the default call_rcu
	 * worker queue, keep the default worker in place.
	 */
	crdp = default_call_rcu_data;
	if (!crdp) {
		teardown = false;
		goto unlock;
	}
	if (!cds_wfcq_empty(&crdp->cbs_head, &crdp->cbs_tail)) {
		teardown = false;
		goto unlock;
	}
	rcu_set_pointer(&default_call_rcu_data, NULL);
unlock:
	call_rcu_unlock(&call_rcu_mutex);
	if (teardown) {
		synchronize_rcu();
		call_rcu_data_free(crdp);
	}
}
