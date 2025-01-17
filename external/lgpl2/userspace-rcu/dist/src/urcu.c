// SPDX-FileCopyrightText: 2009 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
// SPDX-FileCopyrightText: 2009 Paul E. McKenney, IBM Corporation.
//
// SPDX-License-Identifier: LGPL-2.1-or-later

/*
 * Userspace RCU library
 *
 * IBM's contributions to this file may be relicensed under LGPLv2 or later.
 */

#define URCU_NO_COMPAT_IDENTIFIERS
#define _BSD_SOURCE
#define _LGPL_SOURCE
#define _DEFAULT_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <poll.h>

#include <urcu/config.h>
#include <urcu/annotate.h>
#include <urcu/assert.h>
#include <urcu/arch.h>
#include <urcu/wfcqueue.h>
#include <urcu/map/urcu.h>
#include <urcu/static/urcu.h>
#include <urcu/pointer.h>
#include <urcu/tls-compat.h>

#include "urcu-die.h"
#include "urcu-wait.h"
#include "urcu-utils.h"

#define URCU_API_MAP
/* Do not #define _LGPL_SOURCE to ensure we can emit the wrapper symbols */
#undef _LGPL_SOURCE
#include <urcu/urcu.h>
#define _LGPL_SOURCE

/*
 * If a reader is really non-cooperative and refuses to commit its
 * rcu_active_readers count to memory (there is no barrier in the reader
 * per-se), kick it after 10 loops waiting for it.
 */
#define KICK_READER_LOOPS 	10

/*
 * Active attempts to check for reader Q.S. before calling futex().
 */
#define RCU_QS_ACTIVE_ATTEMPTS 100

/* If the headers do not support membarrier system call, fall back on RCU_MB */
#ifdef __NR_membarrier
# define membarrier(...)		syscall(__NR_membarrier, __VA_ARGS__)
#else
# define membarrier(...)		-ENOSYS
#endif

enum membarrier_cmd {
	MEMBARRIER_CMD_QUERY				= 0,
	MEMBARRIER_CMD_SHARED				= (1 << 0),
	/* reserved for MEMBARRIER_CMD_SHARED_EXPEDITED (1 << 1) */
	/* reserved for MEMBARRIER_CMD_PRIVATE (1 << 2) */
	MEMBARRIER_CMD_PRIVATE_EXPEDITED		= (1 << 3),
	MEMBARRIER_CMD_REGISTER_PRIVATE_EXPEDITED	= (1 << 4),
};

#ifdef RCU_MEMBARRIER
static int init_done;
static int urcu_memb_has_sys_membarrier_private_expedited;

#ifndef CONFIG_RCU_FORCE_SYS_MEMBARRIER
/*
 * Explicitly initialize to zero because we can't alias a non-static
 * uninitialized variable.
 */
int urcu_memb_has_sys_membarrier = 0;
#endif

void __attribute__((constructor)) rcu_init(void);
#endif

#if defined(RCU_MB)
void rcu_init(void)
{
}
#endif

void __attribute__((destructor)) rcu_exit(void);
static void urcu_call_rcu_exit(void);

/*
 * rcu_gp_lock ensures mutual exclusion between threads calling
 * synchronize_rcu().
 */
static pthread_mutex_t rcu_gp_lock = PTHREAD_MUTEX_INITIALIZER;
/*
 * rcu_registry_lock ensures mutual exclusion between threads
 * registering and unregistering themselves to/from the registry, and
 * with threads reading that registry from synchronize_rcu(). However,
 * this lock is not held all the way through the completion of awaiting
 * for the grace period. It is sporadically released between iterations
 * on the registry.
 * rcu_registry_lock may nest inside rcu_gp_lock.
 */
static pthread_mutex_t rcu_registry_lock = PTHREAD_MUTEX_INITIALIZER;
struct urcu_gp rcu_gp = { .ctr = URCU_GP_COUNT };

/*
 * Written to only by each individual reader. Read by both the reader and the
 * writers.
 */
DEFINE_URCU_TLS(struct urcu_reader, rcu_reader);

static CDS_LIST_HEAD(registry);

/*
 * Queue keeping threads awaiting to wait for a grace period. Contains
 * struct gp_waiters_thread objects.
 */
static DEFINE_URCU_WAIT_QUEUE(gp_waiters);

static void mutex_lock(pthread_mutex_t *mutex)
{
	int ret;

#ifndef DISTRUST_SIGNALS_EXTREME
	ret = pthread_mutex_lock(mutex);
	if (ret)
		urcu_die(ret);
#else /* #ifndef DISTRUST_SIGNALS_EXTREME */
	while ((ret = pthread_mutex_trylock(mutex)) != 0) {
		if (ret != EBUSY && ret != EINTR)
			urcu_die(ret);
		if (CMM_LOAD_SHARED(URCU_TLS(rcu_reader).need_mb)) {
			cmm_smp_mb();
			_CMM_STORE_SHARED(URCU_TLS(rcu_reader).need_mb, 0);
			cmm_smp_mb();
		}
		(void) poll(NULL, 0, 10);
	}
#endif /* #else #ifndef DISTRUST_SIGNALS_EXTREME */
}

static void mutex_unlock(pthread_mutex_t *mutex)
{
	int ret;

	ret = pthread_mutex_unlock(mutex);
	if (ret)
		urcu_die(ret);
}

#ifdef RCU_MEMBARRIER
static void smp_mb_master(void)
{
	if (caa_likely(urcu_memb_has_sys_membarrier)) {
		if (membarrier(urcu_memb_has_sys_membarrier_private_expedited ?
				MEMBARRIER_CMD_PRIVATE_EXPEDITED :
				MEMBARRIER_CMD_SHARED, 0))
			urcu_die(errno);
	} else {
		cmm_smp_mb();
	}
}
#endif

#if defined(RCU_MB)
static void smp_mb_master(void)
{
	cmm_smp_mb();
}
#endif

/*
 * synchronize_rcu() waiting. Single thread.
 * Always called with rcu_registry lock held. Releases this lock and
 * grabs it again. Holds the lock when it returns.
 */
static void wait_gp(void)
{
	/*
	 * Read reader_gp before read futex.
	 */
	smp_mb_master();
	/* Temporarily unlock the registry lock. */
	mutex_unlock(&rcu_registry_lock);
	while (uatomic_read(&rcu_gp.futex) == -1) {
		if (!futex_async(&rcu_gp.futex, FUTEX_WAIT, -1, NULL, NULL, 0)) {
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
			goto end;
		case EINTR:
			/* Retry if interrupted by signal. */
			break;	/* Get out of switch. Check again. */
		default:
			/* Unexpected error. */
			urcu_die(errno);
		}
	}
end:
	/*
	 * Re-lock the registry lock before the next loop.
	 */
	mutex_lock(&rcu_registry_lock);
}

/*
 * Always called with rcu_registry lock held. Releases this lock between
 * iterations and grabs it again. Holds the lock when it returns.
 */
static void wait_for_readers(struct cds_list_head *input_readers,
			struct cds_list_head *cur_snap_readers,
			struct cds_list_head *qsreaders,
			cmm_annotate_t *group)
{
	unsigned int wait_loops = 0;
	struct urcu_reader *index, *tmp;
#ifdef HAS_INCOHERENT_CACHES
	unsigned int wait_gp_loops = 0;
#endif /* HAS_INCOHERENT_CACHES */

	/*
	 * Wait for each thread URCU_TLS(rcu_reader).ctr to either
	 * indicate quiescence (not nested), or observe the current
	 * rcu_gp.ctr value.
	 */
	for (;;) {
		if (wait_loops < RCU_QS_ACTIVE_ATTEMPTS)
			wait_loops++;
		if (wait_loops >= RCU_QS_ACTIVE_ATTEMPTS) {
			uatomic_dec(&rcu_gp.futex);
			/* Write futex before read reader_gp */
			smp_mb_master();
		}

		cds_list_for_each_entry_safe(index, tmp, input_readers, node) {
			switch (urcu_common_reader_state(&rcu_gp, &index->ctr, group)) {
			case URCU_READER_ACTIVE_CURRENT:
				if (cur_snap_readers) {
					cds_list_move(&index->node,
						cur_snap_readers);
					break;
				}
				/* Fall-through */
			case URCU_READER_INACTIVE:
				cds_list_move(&index->node, qsreaders);
				break;
			case URCU_READER_ACTIVE_OLD:
				/*
				 * Old snapshot. Leaving node in
				 * input_readers will make us busy-loop
				 * until the snapshot becomes current or
				 * the reader becomes inactive.
				 */
				break;
			}
		}

#ifndef HAS_INCOHERENT_CACHES
		if (cds_list_empty(input_readers)) {
			if (wait_loops >= RCU_QS_ACTIVE_ATTEMPTS) {
				/* Read reader_gp before write futex */
				smp_mb_master();
				uatomic_set(&rcu_gp.futex, 0);
			}
			break;
		} else {
			if (wait_loops >= RCU_QS_ACTIVE_ATTEMPTS) {
				/* wait_gp unlocks/locks registry lock. */
				wait_gp();
			} else {
				/* Temporarily unlock the registry lock. */
				mutex_unlock(&rcu_registry_lock);
				caa_cpu_relax();
				/*
				 * Re-lock the registry lock before the
				 * next loop.
				 */
				mutex_lock(&rcu_registry_lock);
			}
		}
#else /* #ifndef HAS_INCOHERENT_CACHES */
		/*
		 * BUSY-LOOP. Force the reader thread to commit its
		 * URCU_TLS(rcu_reader).ctr update to memory if we wait
		 * for too long.
		 */
		if (cds_list_empty(input_readers)) {
			if (wait_loops >= RCU_QS_ACTIVE_ATTEMPTS) {
				/* Read reader_gp before write futex */
				smp_mb_master();
				uatomic_set(&rcu_gp.futex, 0);
			}
			break;
		} else {
			if (wait_gp_loops == KICK_READER_LOOPS) {
				smp_mb_master();
				wait_gp_loops = 0;
			}
			if (wait_loops >= RCU_QS_ACTIVE_ATTEMPTS) {
				/* wait_gp unlocks/locks registry lock. */
				wait_gp();
				wait_gp_loops++;
			} else {
				/* Temporarily unlock the registry lock. */
				mutex_unlock(&rcu_registry_lock);
				caa_cpu_relax();
				/*
				 * Re-lock the registry lock before the
				 * next loop.
				 */
				mutex_lock(&rcu_registry_lock);
			}
		}
#endif /* #else #ifndef HAS_INCOHERENT_CACHES */
	}
}

void synchronize_rcu(void)
{
	cmm_annotate_define(acquire_group);
	cmm_annotate_define(release_group);
	CDS_LIST_HEAD(cur_snap_readers);
	CDS_LIST_HEAD(qsreaders);
	DEFINE_URCU_WAIT_NODE(wait, URCU_WAIT_WAITING);
	struct urcu_waiters waiters;

	/*
	 * Add ourself to gp_waiters queue of threads awaiting to wait
	 * for a grace period. Proceed to perform the grace period only
	 * if we are the first thread added into the queue.
	 * The implicit memory barrier before urcu_wait_add()
	 * orders prior memory accesses of threads put into the wait
	 * queue before their insertion into the wait queue.
	 */
	if (urcu_wait_add(&gp_waiters, &wait) != 0) {
		/*
		 * Not first in queue: will be awakened by another thread.
		 * Implies a memory barrier after grace period.
		 */
		urcu_adaptative_busy_wait(&wait);
		return;
	}
	/* We won't need to wake ourself up */
	urcu_wait_set_state(&wait, URCU_WAIT_RUNNING);

	mutex_lock(&rcu_gp_lock);

	/*
	 * Move all waiters into our local queue.
	 */
	urcu_move_waiters(&waiters, &gp_waiters);

	mutex_lock(&rcu_registry_lock);

	if (cds_list_empty(&registry))
		goto out;

	/*
	 * All threads should read qparity before accessing data structure
	 * where new ptr points to. Must be done within rcu_registry_lock
	 * because it iterates on reader threads.
	 */
	/* Write new ptr before changing the qparity */
	smp_mb_master();
	cmm_annotate_group_mb_release(&release_group);

	/*
	 * Wait for readers to observe original parity or be quiescent.
	 * wait_for_readers() can release and grab again rcu_registry_lock
	 * internally.
	 */
	wait_for_readers(&registry, &cur_snap_readers, &qsreaders, &acquire_group);

	/*
	 * Must finish waiting for quiescent state for original parity before
	 * committing next rcu_gp.ctr update to memory. Failure to do so could
	 * result in the writer waiting forever while new readers are always
	 * accessing data (no progress).  Enforce compiler-order of load
	 * URCU_TLS(rcu_reader).ctr before store to rcu_gp.ctr.
	 */
	cmm_barrier();

	/*
	 * Adding a cmm_smp_mb() which is _not_ formally required, but makes the
	 * model easier to understand. It does not have a big performance impact
	 * anyway, given this is the write-side.
	 */
	cmm_smp_mb();

	/* Switch parity: 0 -> 1, 1 -> 0 */
	cmm_annotate_group_mem_release(&release_group, &rcu_gp.ctr);
	uatomic_store(&rcu_gp.ctr, rcu_gp.ctr ^ URCU_GP_CTR_PHASE, CMM_RELAXED);

	/*
	 * Must commit rcu_gp.ctr update to memory before waiting for quiescent
	 * state. Failure to do so could result in the writer waiting forever
	 * while new readers are always accessing data (no progress). Enforce
	 * compiler-order of store to rcu_gp.ctr before load rcu_reader ctr.
	 */
	cmm_barrier();

	/*
	 *
	 * Adding a cmm_smp_mb() which is _not_ formally required, but makes the
	 * model easier to understand. It does not have a big performance impact
	 * anyway, given this is the write-side.
	 */
	cmm_smp_mb();

	/*
	 * Wait for readers to observe new parity or be quiescent.
	 * wait_for_readers() can release and grab again rcu_registry_lock
	 * internally.
	 */
	wait_for_readers(&cur_snap_readers, NULL, &qsreaders, &acquire_group);

	/*
	 * Put quiescent reader list back into registry.
	 */
	cds_list_splice(&qsreaders, &registry);

	/*
	 * Finish waiting for reader threads before letting the old ptr
	 * being freed. Must be done within rcu_registry_lock because it
	 * iterates on reader threads.
	 */
	smp_mb_master();
	cmm_annotate_group_mb_acquire(&acquire_group);
out:
	mutex_unlock(&rcu_registry_lock);
	mutex_unlock(&rcu_gp_lock);

	/*
	 * Wakeup waiters only after we have completed the grace period
	 * and have ensured the memory barriers at the end of the grace
	 * period have been issued.
	 */
	urcu_wake_all_waiters(&waiters);
}

/*
 * library wrappers to be used by non-LGPL compatible source code.
 */

void rcu_read_lock(void)
{
	_rcu_read_lock();
}

void rcu_read_unlock(void)
{
	_rcu_read_unlock();
}

int rcu_read_ongoing(void)
{
	return _rcu_read_ongoing();
}

void rcu_register_thread(void)
{
	URCU_TLS(rcu_reader).tid = pthread_self();
	urcu_posix_assert(URCU_TLS(rcu_reader).need_mb == 0);
	urcu_posix_assert(!(URCU_TLS(rcu_reader).ctr & URCU_GP_CTR_NEST_MASK));

	mutex_lock(&rcu_registry_lock);
	urcu_posix_assert(!URCU_TLS(rcu_reader).registered);
	URCU_TLS(rcu_reader).registered = 1;
	rcu_init();	/* In case gcc does not support constructor attribute */
	cds_list_add(&URCU_TLS(rcu_reader).node, &registry);
	mutex_unlock(&rcu_registry_lock);
}

void rcu_unregister_thread(void)
{
	mutex_lock(&rcu_registry_lock);
	urcu_posix_assert(URCU_TLS(rcu_reader).registered);
	URCU_TLS(rcu_reader).registered = 0;
	cds_list_del(&URCU_TLS(rcu_reader).node);
	mutex_unlock(&rcu_registry_lock);
}

#ifdef RCU_MEMBARRIER

#ifdef CONFIG_RCU_FORCE_SYS_MEMBARRIER
static
void rcu_sys_membarrier_status(bool available)
{
	if (!available)
		abort();
}
#else
static
void rcu_sys_membarrier_status(bool available)
{
	if (!available)
		return;
	urcu_memb_has_sys_membarrier = 1;
}
#endif

static
void rcu_sys_membarrier_init(void)
{
	bool available = false;
	int mask;

	mask = membarrier(MEMBARRIER_CMD_QUERY, 0);
	if (mask >= 0) {
		if (mask & MEMBARRIER_CMD_PRIVATE_EXPEDITED) {
			if (membarrier(MEMBARRIER_CMD_REGISTER_PRIVATE_EXPEDITED, 0))
				urcu_die(errno);
			urcu_memb_has_sys_membarrier_private_expedited = 1;
			available = true;
		} else if (mask & MEMBARRIER_CMD_SHARED) {
			available = true;
		}
	}
	rcu_sys_membarrier_status(available);
}

void rcu_init(void)
{
	if (init_done)
		return;
	init_done = 1;
	rcu_sys_membarrier_init();
}
#endif

void rcu_exit(void)
{
	urcu_call_rcu_exit();
}

DEFINE_RCU_FLAVOR(rcu_flavor);

#include "urcu-call-rcu-impl.h"
#include "urcu-defer-impl.h"
#include "urcu-poll-impl.h"
