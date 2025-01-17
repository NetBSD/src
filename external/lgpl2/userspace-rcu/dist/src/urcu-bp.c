// SPDX-FileCopyrightText: 2009 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
// SPDX-FileCopyrightText: 2009 Paul E. McKenney, IBM Corporation.
//
// SPDX-License-Identifier: LGPL-2.1-or-later

/*
 * Userspace RCU library, "bulletproof" version.
 *
 * IBM's contributions to this file may be relicensed under LGPLv2 or later.
 */

#define URCU_NO_COMPAT_IDENTIFIERS
#define _LGPL_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <poll.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/mman.h>

#include <urcu/annotate.h>
#include <urcu/assert.h>
#include <urcu/config.h>
#include <urcu/arch.h>
#include <urcu/wfcqueue.h>
#include <urcu/map/urcu-bp.h>
#include <urcu/static/urcu-bp.h>
#include <urcu/pointer.h>
#include <urcu/tls-compat.h>

#include "urcu-die.h"
#include "urcu-utils.h"

#define URCU_API_MAP
/* Do not #define _LGPL_SOURCE to ensure we can emit the wrapper symbols */
#undef _LGPL_SOURCE
#include <urcu/urcu-bp.h>
#define _LGPL_SOURCE

#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

#ifdef __linux__
static
void *mremap_wrapper(void *old_address, size_t old_size,
		size_t new_size, int flags)
{
	return mremap(old_address, old_size, new_size, flags);
}
#else

#define MREMAP_MAYMOVE	1
#define MREMAP_FIXED	2

/*
 * mremap wrapper for non-Linux systems not allowing MAYMOVE.
 * This is not generic.
*/
static
void *mremap_wrapper(void *old_address __attribute__((unused)),
		size_t old_size __attribute__((unused)),
		size_t new_size __attribute__((unused)),
		int flags)
{
	urcu_posix_assert(!(flags & MREMAP_MAYMOVE));

	return MAP_FAILED;
}
#endif

/* Sleep delay in ms */
#define RCU_SLEEP_DELAY_MS	10
#define INIT_READER_COUNT	8

/*
 * Active attempts to check for reader Q.S. before calling sleep().
 */
#define RCU_QS_ACTIVE_ATTEMPTS 100

static
int urcu_bp_refcount;

/* If the headers do not support membarrier system call, fall back smp_mb. */
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

static
void __attribute__((constructor)) _urcu_bp_init(void);
static
void urcu_bp_exit(void);
static
void __attribute__((destructor)) urcu_bp_exit_destructor(void);
static void urcu_call_rcu_exit(void);

#ifndef CONFIG_RCU_FORCE_SYS_MEMBARRIER
int urcu_bp_has_sys_membarrier;
#endif

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

static pthread_mutex_t init_lock = PTHREAD_MUTEX_INITIALIZER;
static int initialized;

static pthread_key_t urcu_bp_key;

struct urcu_bp_gp urcu_bp_gp = { .ctr = URCU_BP_GP_COUNT };

/*
 * Pointer to registry elements. Written to only by each individual reader. Read
 * by both the reader and the writers.
 */
DEFINE_URCU_TLS(struct urcu_bp_reader *, urcu_bp_reader);

static CDS_LIST_HEAD(registry);

struct registry_chunk {
	size_t capacity;		/* capacity of this chunk (in elements) */
	size_t used;			/* count of elements used */
	struct cds_list_head node;	/* chunk_list node */
	struct urcu_bp_reader readers[];
};

struct registry_arena {
	struct cds_list_head chunk_list;
};

static struct registry_arena registry_arena = {
	.chunk_list = CDS_LIST_HEAD_INIT(registry_arena.chunk_list),
};

/* Saved fork signal mask, protected by rcu_gp_lock */
static sigset_t saved_fork_signal_mask;

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
		poll(NULL,0,10);
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

static void smp_mb_master(void)
{
	if (caa_likely(urcu_bp_has_sys_membarrier)) {
		if (membarrier(MEMBARRIER_CMD_PRIVATE_EXPEDITED, 0))
			urcu_die(errno);
	} else {
		cmm_smp_mb();
	}
}

/* Get the size of a chunk's allocation from its capacity (an element count). */
static size_t chunk_allocation_size(size_t capacity)
{
	return (capacity * sizeof(struct urcu_bp_reader)) +
		sizeof(struct registry_chunk);
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
	struct urcu_bp_reader *index, *tmp;

	/*
	 * Wait for each thread URCU_TLS(urcu_bp_reader).ctr to either
	 * indicate quiescence (not nested), or observe the current
	 * rcu_gp.ctr value.
	 */
	for (;;) {
		if (wait_loops < RCU_QS_ACTIVE_ATTEMPTS)
			wait_loops++;

		cds_list_for_each_entry_safe(index, tmp, input_readers, node) {
			switch (urcu_bp_reader_state(&index->ctr, group)) {
			case URCU_BP_READER_ACTIVE_CURRENT:
				if (cur_snap_readers) {
					cds_list_move(&index->node,
						cur_snap_readers);
					break;
				}
				/* Fall-through */
			case URCU_BP_READER_INACTIVE:
				cds_list_move(&index->node, qsreaders);
				break;
			case URCU_BP_READER_ACTIVE_OLD:
				/*
				 * Old snapshot. Leaving node in
				 * input_readers will make us busy-loop
				 * until the snapshot becomes current or
				 * the reader becomes inactive.
				 */
				break;
			}
		}

		if (cds_list_empty(input_readers)) {
			break;
		} else {
			/* Temporarily unlock the registry lock. */
			mutex_unlock(&rcu_registry_lock);
			if (wait_loops >= RCU_QS_ACTIVE_ATTEMPTS)
				(void) poll(NULL, 0, RCU_SLEEP_DELAY_MS);
			else
				caa_cpu_relax();
			/* Re-lock the registry lock before the next loop. */
			mutex_lock(&rcu_registry_lock);
		}
	}
}

void urcu_bp_synchronize_rcu(void)
{
	cmm_annotate_define(acquire_group);
	cmm_annotate_define(release_group);
	CDS_LIST_HEAD(cur_snap_readers);
	CDS_LIST_HEAD(qsreaders);
	sigset_t newmask, oldmask;
	int ret;

	ret = sigfillset(&newmask);
	urcu_posix_assert(!ret);
	ret = pthread_sigmask(SIG_BLOCK, &newmask, &oldmask);
	urcu_posix_assert(!ret);

	mutex_lock(&rcu_gp_lock);

	mutex_lock(&rcu_registry_lock);

	if (cds_list_empty(&registry))
		goto out;

	/* All threads should read qparity before accessing data structure
	 * where new ptr points to. */
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
	 * Adding a cmm_smp_mb() which is _not_ formally required, but makes the
	 * model easier to understand. It does not have a big performance impact
	 * anyway, given this is the write-side.
	 */
	cmm_smp_mb();

	/* Switch parity: 0 -> 1, 1 -> 0 */
	cmm_annotate_group_mem_release(&release_group, &rcu_gp.ctr);
	uatomic_store(&rcu_gp.ctr, rcu_gp.ctr ^ URCU_BP_GP_CTR_PHASE, CMM_RELAXED);

	/*
	 * Must commit qparity update to memory before waiting for other parity
	 * quiescent state. Failure to do so could result in the writer waiting
	 * forever while new readers are always accessing data (no progress).
	 * Ensured by CMM_STORE_SHARED and CMM_LOAD_SHARED.
	 */

	/*
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
	 * Finish waiting for reader threads before letting the old ptr being
	 * freed.
	 */
	smp_mb_master();
	cmm_annotate_group_mb_acquire(&acquire_group);
out:
	mutex_unlock(&rcu_registry_lock);
	mutex_unlock(&rcu_gp_lock);
	ret = pthread_sigmask(SIG_SETMASK, &oldmask, NULL);
	urcu_posix_assert(!ret);
}

/*
 * library wrappers to be used by non-LGPL compatible source code.
 */

void urcu_bp_read_lock(void)
{
	_urcu_bp_read_lock();
}

void urcu_bp_read_unlock(void)
{
	_urcu_bp_read_unlock();
}

int urcu_bp_read_ongoing(void)
{
	return _urcu_bp_read_ongoing();
}

/*
 * Only grow for now. If empty, allocate a ARENA_INIT_ALLOC sized chunk.
 * Else, try expanding the last chunk. If this fails, allocate a new
 * chunk twice as big as the last chunk.
 * Memory used by chunks _never_ moves. A chunk could theoretically be
 * freed when all "used" slots are released, but we don't do it at this
 * point.
 */
static
void expand_arena(struct registry_arena *arena)
{
	struct registry_chunk *new_chunk, *last_chunk;
	size_t old_chunk_size_bytes, new_chunk_size_bytes, new_capacity;

	/* No chunk. */
	if (cds_list_empty(&arena->chunk_list)) {
		new_chunk_size_bytes = chunk_allocation_size(INIT_READER_COUNT);
		new_chunk = (struct registry_chunk *) mmap(NULL,
			new_chunk_size_bytes,
			PROT_READ | PROT_WRITE,
			MAP_ANONYMOUS | MAP_PRIVATE,
			-1, 0);
		if (new_chunk == MAP_FAILED)
			abort();
		memset(new_chunk, 0, new_chunk_size_bytes);
		new_chunk->capacity = INIT_READER_COUNT;
		cds_list_add_tail(&new_chunk->node, &arena->chunk_list);
		return;		/* We're done. */
	}

	/* Try expanding last chunk. */
	last_chunk = cds_list_entry(arena->chunk_list.prev,
		struct registry_chunk, node);
	old_chunk_size_bytes = chunk_allocation_size(last_chunk->capacity);
	new_capacity = last_chunk->capacity << 1;
	new_chunk_size_bytes = chunk_allocation_size(new_capacity);

	/* Don't allow memory mapping to move, just expand. */
	new_chunk = mremap_wrapper(last_chunk, old_chunk_size_bytes,
		new_chunk_size_bytes, 0);
	if (new_chunk != MAP_FAILED) {
		/* Should not have moved. */
		assert(new_chunk == last_chunk);
		memset((char *) last_chunk + old_chunk_size_bytes, 0,
			new_chunk_size_bytes - old_chunk_size_bytes);
		last_chunk->capacity = new_capacity;
		return;		/* We're done. */
	}

	/* Remap did not succeed, we need to add a new chunk. */
	new_chunk = (struct registry_chunk *) mmap(NULL,
		new_chunk_size_bytes,
		PROT_READ | PROT_WRITE,
		MAP_ANONYMOUS | MAP_PRIVATE,
		-1, 0);
	if (new_chunk == MAP_FAILED)
		abort();
	memset(new_chunk, 0, new_chunk_size_bytes);
	new_chunk->capacity = new_capacity;
	cds_list_add_tail(&new_chunk->node, &arena->chunk_list);
}

static
struct rcu_reader *arena_alloc(struct registry_arena *arena)
{
	struct registry_chunk *chunk;
	int expand_done = 0;	/* Only allow to expand once per alloc */

retry:
	cds_list_for_each_entry(chunk, &arena->chunk_list, node) {
		size_t spot_idx;

		/* Skip fully used chunks. */
		if (chunk->used == chunk->capacity) {
			continue;
		}

		/* Find a spot. */
		for (spot_idx = 0; spot_idx < chunk->capacity; spot_idx++) {
			if (!chunk->readers[spot_idx].alloc) {
				chunk->readers[spot_idx].alloc = 1;
				chunk->used++;
				return &chunk->readers[spot_idx];
			}
		}
	}

	if (!expand_done) {
		expand_arena(arena);
		expand_done = 1;
		goto retry;
	}

	return NULL;
}

/* Called with signals off and mutex locked */
static
void add_thread(void)
{
	struct rcu_reader *rcu_reader_reg;
	int ret;

	rcu_reader_reg = arena_alloc(&registry_arena);
	if (!rcu_reader_reg)
		abort();
	ret = pthread_setspecific(urcu_bp_key, rcu_reader_reg);
	if (ret)
		abort();

	/* Add to registry */
	rcu_reader_reg->tid = pthread_self();
	urcu_posix_assert(rcu_reader_reg->ctr == 0);
	cds_list_add(&rcu_reader_reg->node, &registry);
	/*
	 * Reader threads are pointing to the reader registry. This is
	 * why its memory should never be relocated.
	 */
	URCU_TLS(urcu_bp_reader) = rcu_reader_reg;
}

/* Called with mutex locked */
static
void cleanup_thread(struct registry_chunk *chunk,
		struct rcu_reader *rcu_reader_reg)
{
	rcu_reader_reg->ctr = 0;
	cds_list_del(&rcu_reader_reg->node);
	rcu_reader_reg->tid = 0;
	rcu_reader_reg->alloc = 0;
	chunk->used--;
}

static
struct registry_chunk *find_chunk(struct rcu_reader *rcu_reader_reg)
{
	struct registry_chunk *chunk;

	cds_list_for_each_entry(chunk, &registry_arena.chunk_list, node) {
		if (rcu_reader_reg < (struct urcu_bp_reader *) &chunk->readers[0])
			continue;
		if (rcu_reader_reg >= (struct urcu_bp_reader *) &chunk->readers[chunk->capacity])
			continue;
		return chunk;
	}
	return NULL;
}

/* Called with signals off and mutex locked */
static
void remove_thread(struct rcu_reader *rcu_reader_reg)
{
	cleanup_thread(find_chunk(rcu_reader_reg), rcu_reader_reg);
	URCU_TLS(urcu_bp_reader) = NULL;
}

/* Disable signals, take mutex, add to registry */
void urcu_bp_register(void)
{
	sigset_t newmask, oldmask;
	int ret;

	ret = sigfillset(&newmask);
	if (ret)
		abort();
	ret = pthread_sigmask(SIG_BLOCK, &newmask, &oldmask);
	if (ret)
		abort();

	/*
	 * Check if a signal concurrently registered our thread since
	 * the check in rcu_read_lock().
	 */
	if (URCU_TLS(urcu_bp_reader))
		goto end;

	/*
	 * Take care of early registration before urcu_bp constructor.
	 */
	_urcu_bp_init();

	mutex_lock(&rcu_registry_lock);
	add_thread();
	mutex_unlock(&rcu_registry_lock);
end:
	ret = pthread_sigmask(SIG_SETMASK, &oldmask, NULL);
	if (ret)
		abort();
}

void urcu_bp_register_thread(void)
{
	if (caa_unlikely(!URCU_TLS(urcu_bp_reader)))
		urcu_bp_register(); /* If not yet registered. */
}

/* Disable signals, take mutex, remove from registry */
static
void urcu_bp_unregister(struct rcu_reader *rcu_reader_reg)
{
	sigset_t newmask, oldmask;
	int ret;

	ret = sigfillset(&newmask);
	if (ret)
		abort();
	ret = pthread_sigmask(SIG_BLOCK, &newmask, &oldmask);
	if (ret)
		abort();

	mutex_lock(&rcu_registry_lock);
	remove_thread(rcu_reader_reg);
	mutex_unlock(&rcu_registry_lock);
	ret = pthread_sigmask(SIG_SETMASK, &oldmask, NULL);
	if (ret)
		abort();
	urcu_bp_exit();
}

/*
 * Remove thread from the registry when it exits, and flag it as
 * destroyed so garbage collection can take care of it.
 */
static
void urcu_bp_thread_exit_notifier(void *rcu_key)
{
	urcu_bp_unregister(rcu_key);
}

#ifdef CONFIG_RCU_FORCE_SYS_MEMBARRIER
static
void urcu_bp_sys_membarrier_status(bool available)
{
	if (!available)
		abort();
}
#else
static
void urcu_bp_sys_membarrier_status(bool available)
{
	if (!available)
		return;
	urcu_bp_has_sys_membarrier = 1;
}
#endif

static
void urcu_bp_sys_membarrier_init(void)
{
	bool available = false;
	int mask;

	mask = membarrier(MEMBARRIER_CMD_QUERY, 0);
	if (mask >= 0) {
		if (mask & MEMBARRIER_CMD_PRIVATE_EXPEDITED) {
			if (membarrier(MEMBARRIER_CMD_REGISTER_PRIVATE_EXPEDITED, 0))
				urcu_die(errno);
			available = true;
		}
	}
	urcu_bp_sys_membarrier_status(available);
}

static
void _urcu_bp_init(void)
{
	mutex_lock(&init_lock);
	if (!urcu_bp_refcount++) {
		int ret;

		ret = pthread_key_create(&urcu_bp_key,
				urcu_bp_thread_exit_notifier);
		if (ret)
			abort();
		urcu_bp_sys_membarrier_init();
		initialized = 1;
	}
	mutex_unlock(&init_lock);
}

static
void urcu_bp_exit(void)
{
	mutex_lock(&init_lock);
	if (!--urcu_bp_refcount) {
		struct registry_chunk *chunk, *tmp;
		int ret;

		cds_list_for_each_entry_safe(chunk, tmp,
				&registry_arena.chunk_list, node) {
			munmap((void *) chunk, chunk_allocation_size(chunk->capacity));
		}
		CDS_INIT_LIST_HEAD(&registry_arena.chunk_list);
		ret = pthread_key_delete(urcu_bp_key);
		if (ret)
			abort();
	}
	mutex_unlock(&init_lock);
}

static
void urcu_bp_exit_destructor(void)
{
	urcu_call_rcu_exit();
	urcu_bp_exit();
}

/*
 * Holding the rcu_gp_lock and rcu_registry_lock across fork will make
 * sure we fork() don't race with a concurrent thread executing with
 * any of those locks held. This ensures that the registry and data
 * protected by rcu_gp_lock are in a coherent state in the child.
 */
void urcu_bp_before_fork(void)
{
	sigset_t newmask, oldmask;
	int ret;

	ret = sigfillset(&newmask);
	urcu_posix_assert(!ret);
	ret = pthread_sigmask(SIG_BLOCK, &newmask, &oldmask);
	urcu_posix_assert(!ret);
	mutex_lock(&rcu_gp_lock);
	mutex_lock(&rcu_registry_lock);
	saved_fork_signal_mask = oldmask;
}

void urcu_bp_after_fork_parent(void)
{
	sigset_t oldmask;
	int ret;

	oldmask = saved_fork_signal_mask;
	mutex_unlock(&rcu_registry_lock);
	mutex_unlock(&rcu_gp_lock);
	ret = pthread_sigmask(SIG_SETMASK, &oldmask, NULL);
	urcu_posix_assert(!ret);
}

/*
 * Prune all entries from registry except our own thread. Fits the Linux
 * fork behavior. Called with rcu_gp_lock and rcu_registry_lock held.
 */
static
void urcu_bp_prune_registry(void)
{
	struct registry_chunk *chunk;

	cds_list_for_each_entry(chunk, &registry_arena.chunk_list, node) {
		size_t spot_idx;

		for (spot_idx = 0; spot_idx < chunk->capacity; spot_idx++) {
			struct urcu_bp_reader *reader = &chunk->readers[spot_idx];

			if (!reader->alloc)
				continue;
			if (reader->tid == pthread_self())
				continue;
			cleanup_thread(chunk, reader);
		}
	}
}

void urcu_bp_after_fork_child(void)
{
	sigset_t oldmask;
	int ret;

	urcu_bp_prune_registry();
	oldmask = saved_fork_signal_mask;
	mutex_unlock(&rcu_registry_lock);
	mutex_unlock(&rcu_gp_lock);
	ret = pthread_sigmask(SIG_SETMASK, &oldmask, NULL);
	urcu_posix_assert(!ret);
}

void *urcu_bp_dereference_sym(void *p)
{
	return _rcu_dereference(p);
}

void *urcu_bp_set_pointer_sym(void **p, void *v)
{
	cmm_wmb();
	uatomic_set(p, v);
	return v;
}

void *urcu_bp_xchg_pointer_sym(void **p, void *v)
{
	cmm_wmb();
	return uatomic_xchg(p, v);
}

void *urcu_bp_cmpxchg_pointer_sym(void **p, void *old, void *_new)
{
	cmm_wmb();
	return uatomic_cmpxchg(p, old, _new);
}

DEFINE_RCU_FLAVOR(rcu_flavor);

#include "urcu-call-rcu-impl.h"
#include "urcu-defer-impl.h"
#include "urcu-poll-impl.h"
