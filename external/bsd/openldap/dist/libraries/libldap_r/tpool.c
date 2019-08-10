/*	$NetBSD: tpool.c,v 1.1.1.6.6.1 2019/08/10 06:17:15 martin Exp $	*/

/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1998-2019 The OpenLDAP Foundation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: tpool.c,v 1.1.1.6.6.1 2019/08/10 06:17:15 martin Exp $");

#include "portable.h"

#include <stdio.h>

#include <ac/signal.h>
#include <ac/stdarg.h>
#include <ac/stdlib.h>
#include <ac/string.h>
#include <ac/time.h>
#include <ac/errno.h>

#include "ldap-int.h"
#include "ldap_pvt_thread.h" /* Get the thread interface */
#include "ldap_queue.h"
#define LDAP_THREAD_POOL_IMPLEMENTATION
#include "ldap_thr_debug.h"  /* May rename symbols defined below */

#ifndef LDAP_THREAD_HAVE_TPOOL

/* Thread-specific key with data and optional free function */
typedef struct ldap_int_tpool_key_s {
	void *ltk_key;
	void *ltk_data;
	ldap_pvt_thread_pool_keyfree_t *ltk_free;
} ldap_int_tpool_key_t;

/* Max number of thread-specific keys we store per thread.
 * We don't expect to use many...
 */
#define	MAXKEYS	32

/* Max number of threads */
#define	LDAP_MAXTHR	1024	/* must be a power of 2 */

/* (Theoretical) max number of pending requests */
#define MAX_PENDING (INT_MAX/2)	/* INT_MAX - (room to avoid overflow) */

/* pool->ltp_pause values */
enum { NOT_PAUSED = 0, WANT_PAUSE = 1, PAUSED = 2 };

/* Context: thread ID and thread-specific key/data pairs */
typedef struct ldap_int_thread_userctx_s {
	ldap_pvt_thread_t ltu_id;
	ldap_int_tpool_key_t ltu_key[MAXKEYS];
} ldap_int_thread_userctx_t;


/* Simple {thread ID -> context} hash table; key=ctx->ltu_id.
 * Protected by ldap_pvt_thread_pool_mutex except during pauses,
 * when it is read-only (used by pool_purgekey and pool_context).
 * Protected by tpool->ltp_mutex during pauses.
 */
static struct {
	ldap_int_thread_userctx_t *ctx;
	/* ctx is valid when not NULL or DELETED_THREAD_CTX */
#	define DELETED_THREAD_CTX (&ldap_int_main_thrctx + 1) /* dummy addr */
} thread_keys[LDAP_MAXTHR];

#define	TID_HASH(tid, hash) do { \
	unsigned const char *ptr_ = (unsigned const char *)&(tid); \
	unsigned i_; \
	for (i_ = 0, (hash) = ptr_[0]; ++i_ < sizeof(tid);) \
		(hash) += ((hash) << 5) ^ ptr_[i_]; \
} while(0)


/* Task for a thread to perform */
typedef struct ldap_int_thread_task_s {
	union {
		LDAP_STAILQ_ENTRY(ldap_int_thread_task_s) q;
		LDAP_SLIST_ENTRY(ldap_int_thread_task_s) l;
	} ltt_next;
	ldap_pvt_thread_start_t *ltt_start_routine;
	void *ltt_arg;
} ldap_int_thread_task_t;

typedef LDAP_STAILQ_HEAD(tcq, ldap_int_thread_task_s) ldap_int_tpool_plist_t;

struct ldap_int_thread_pool_s {
	LDAP_STAILQ_ENTRY(ldap_int_thread_pool_s) ltp_next;

	/* protect members below, and protect thread_keys[] during pauses */
	ldap_pvt_thread_mutex_t ltp_mutex;

	/* not paused and something to do for pool_<wrapper/pause/destroy>() */
	ldap_pvt_thread_cond_t ltp_cond;

	/* ltp_active_count <= 1 && ltp_pause */
	ldap_pvt_thread_cond_t ltp_pcond;

	/* ltp_pause == 0 ? &ltp_pending_list : &empty_pending_list,
	 * maintaned to reduce work for pool_wrapper()
	 */
	ldap_int_tpool_plist_t *ltp_work_list;

	/* pending tasks, and unused task objects */
	ldap_int_tpool_plist_t ltp_pending_list;
	LDAP_SLIST_HEAD(tcl, ldap_int_thread_task_s) ltp_free_list;

	/* The pool is finishing, waiting for its threads to close.
	 * They close when ltp_pending_list is done.  pool_submit()
	 * rejects new tasks.  ltp_max_pending = -(its old value).
	 */
	int ltp_finishing;

	/* Some active task needs to be the sole active task.
	 * Atomic variable so ldap_pvt_thread_pool_pausing() can read it.
	 * Note: Pauses adjust ltp_<open_count/vary_open_count/work_list>,
	 * so pool_<submit/wrapper>() mostly can avoid testing ltp_pause.
	 */
	volatile sig_atomic_t ltp_pause;

	/* Max number of threads in pool, or 0 for default (LDAP_MAXTHR) */
	int ltp_max_count;

	/* Max pending + paused + idle tasks, negated when ltp_finishing */
	int ltp_max_pending;

	int ltp_pending_count;		/* Pending + paused + idle tasks */
	int ltp_active_count;		/* Active, not paused/idle tasks */
	int ltp_open_count;			/* Number of threads, negated when ltp_pause */
	int ltp_starting;			/* Currenlty starting threads */

	/* >0 if paused or we may open a thread, <0 if we should close a thread.
	 * Updated when ltp_<finishing/pause/max_count/open_count> change.
	 * Maintained to reduce the time ltp_mutex must be locked in
	 * ldap_pvt_thread_pool_<submit/wrapper>().
	 */
	int ltp_vary_open_count;
#	define SET_VARY_OPEN_COUNT(pool)	\
		((pool)->ltp_vary_open_count =	\
		 (pool)->ltp_pause      ?  1 :	\
		 (pool)->ltp_finishing  ? -1 :	\
		 ((pool)->ltp_max_count ? (pool)->ltp_max_count : LDAP_MAXTHR) \
		 - (pool)->ltp_open_count)
};

static ldap_int_tpool_plist_t empty_pending_list =
	LDAP_STAILQ_HEAD_INITIALIZER(empty_pending_list);

static int ldap_int_has_thread_pool = 0;
static LDAP_STAILQ_HEAD(tpq, ldap_int_thread_pool_s)
	ldap_int_thread_pool_list =
	LDAP_STAILQ_HEAD_INITIALIZER(ldap_int_thread_pool_list);

static ldap_pvt_thread_mutex_t ldap_pvt_thread_pool_mutex;

static void *ldap_int_thread_pool_wrapper( void *pool );

static ldap_pvt_thread_key_t	ldap_tpool_key;

/* Context of the main thread */
static ldap_int_thread_userctx_t ldap_int_main_thrctx;

int
ldap_int_thread_pool_startup ( void )
{
	ldap_int_main_thrctx.ltu_id = ldap_pvt_thread_self();
	ldap_pvt_thread_key_create( &ldap_tpool_key );
	return ldap_pvt_thread_mutex_init(&ldap_pvt_thread_pool_mutex);
}

int
ldap_int_thread_pool_shutdown ( void )
{
	struct ldap_int_thread_pool_s *pool;

	while ((pool = LDAP_STAILQ_FIRST(&ldap_int_thread_pool_list)) != NULL) {
		(ldap_pvt_thread_pool_destroy)(&pool, 0); /* ignore thr_debug macro */
	}
	ldap_pvt_thread_mutex_destroy(&ldap_pvt_thread_pool_mutex);
	ldap_pvt_thread_key_destroy( ldap_tpool_key );
	return(0);
}


/* Create a thread pool */
int
ldap_pvt_thread_pool_init (
	ldap_pvt_thread_pool_t *tpool,
	int max_threads,
	int max_pending )
{
	ldap_pvt_thread_pool_t pool;
	int rc;

	/* multiple pools are currently not supported (ITS#4943) */
	assert(!ldap_int_has_thread_pool);

	if (! (0 <= max_threads && max_threads <= LDAP_MAXTHR))
		max_threads = 0;
	if (! (1 <= max_pending && max_pending <= MAX_PENDING))
		max_pending = MAX_PENDING;

	*tpool = NULL;
	pool = (ldap_pvt_thread_pool_t) LDAP_CALLOC(1,
		sizeof(struct ldap_int_thread_pool_s));

	if (pool == NULL) return(-1);

	rc = ldap_pvt_thread_mutex_init(&pool->ltp_mutex);
	if (rc != 0) {
fail1:
		LDAP_FREE(pool);
		return(rc);
	}
	rc = ldap_pvt_thread_cond_init(&pool->ltp_cond);
	if (rc != 0) {
fail2:
		ldap_pvt_thread_mutex_destroy(&pool->ltp_mutex);
		goto fail1;
	}
	rc = ldap_pvt_thread_cond_init(&pool->ltp_pcond);
	if (rc != 0) {
		ldap_pvt_thread_cond_destroy(&pool->ltp_cond);
		goto fail2;
	}

	ldap_int_has_thread_pool = 1;

	pool->ltp_max_count = max_threads;
	SET_VARY_OPEN_COUNT(pool);
	pool->ltp_max_pending = max_pending;

	LDAP_STAILQ_INIT(&pool->ltp_pending_list);
	pool->ltp_work_list = &pool->ltp_pending_list;
	LDAP_SLIST_INIT(&pool->ltp_free_list);

	ldap_pvt_thread_mutex_lock(&ldap_pvt_thread_pool_mutex);
	LDAP_STAILQ_INSERT_TAIL(&ldap_int_thread_pool_list, pool, ltp_next);
	ldap_pvt_thread_mutex_unlock(&ldap_pvt_thread_pool_mutex);

	/* Start no threads just yet.  That can break if the process forks
	 * later, as slapd does in order to daemonize.  On at least POSIX,
	 * only the forking thread would survive in the child.  Yet fork()
	 * can't unlock/clean up other threads' locks and data structures,
	 * unless pthread_atfork() handlers have been set up to do so.
	 */

	*tpool = pool;
	return(0);
}


/* Submit a task to be performed by the thread pool */
int
ldap_pvt_thread_pool_submit (
	ldap_pvt_thread_pool_t *tpool,
	ldap_pvt_thread_start_t *start_routine, void *arg )
{
	struct ldap_int_thread_pool_s *pool;
	ldap_int_thread_task_t *task;
	ldap_pvt_thread_t thr;

	if (tpool == NULL)
		return(-1);

	pool = *tpool;

	if (pool == NULL)
		return(-1);

	ldap_pvt_thread_mutex_lock(&pool->ltp_mutex);

	if (pool->ltp_pending_count >= pool->ltp_max_pending)
		goto failed;

	task = LDAP_SLIST_FIRST(&pool->ltp_free_list);
	if (task) {
		LDAP_SLIST_REMOVE_HEAD(&pool->ltp_free_list, ltt_next.l);
	} else {
		task = (ldap_int_thread_task_t *) LDAP_MALLOC(sizeof(*task));
		if (task == NULL)
			goto failed;
	}

	task->ltt_start_routine = start_routine;
	task->ltt_arg = arg;

	pool->ltp_pending_count++;
	LDAP_STAILQ_INSERT_TAIL(&pool->ltp_pending_list, task, ltt_next.q);

	/* true if ltp_pause != 0 or we should open (create) a thread */
	if (pool->ltp_vary_open_count > 0 &&
		pool->ltp_open_count < pool->ltp_active_count+pool->ltp_pending_count)
	{
		if (pool->ltp_pause)
			goto done;

		pool->ltp_starting++;
		pool->ltp_open_count++;
		SET_VARY_OPEN_COUNT(pool);

		if (0 != ldap_pvt_thread_create(
			&thr, 1, ldap_int_thread_pool_wrapper, pool))
		{
			/* couldn't create thread.  back out of
			 * ltp_open_count and check for even worse things.
			 */
			pool->ltp_starting--;
			pool->ltp_open_count--;
			SET_VARY_OPEN_COUNT(pool);

			if (pool->ltp_open_count == 0) {
				/* no open threads at all?!?
				 */
				ldap_int_thread_task_t *ptr;

				/* let pool_destroy know there are no more threads */
				ldap_pvt_thread_cond_signal(&pool->ltp_cond);

				LDAP_STAILQ_FOREACH(ptr, &pool->ltp_pending_list, ltt_next.q)
					if (ptr == task) break;
				if (ptr == task) {
					/* no open threads, task not handled, so
					 * back out of ltp_pending_count, free the task,
					 * report the error.
					 */
					pool->ltp_pending_count--;
					LDAP_STAILQ_REMOVE(&pool->ltp_pending_list, task,
						ldap_int_thread_task_s, ltt_next.q);
					LDAP_SLIST_INSERT_HEAD(&pool->ltp_free_list, task,
						ltt_next.l);
					goto failed;
				}
			}
			/* there is another open thread, so this
			 * task will be handled eventually.
			 */
		}
	}
	ldap_pvt_thread_cond_signal(&pool->ltp_cond);

 done:
	ldap_pvt_thread_mutex_unlock(&pool->ltp_mutex);
	return(0);

 failed:
	ldap_pvt_thread_mutex_unlock(&pool->ltp_mutex);
	return(-1);
}

static void *
no_task( void *ctx, void *arg )
{
	return NULL;
}

/* Cancel a pending task that was previously submitted.
 * Return 1 if the task was successfully cancelled, 0 if
 * not found, -1 for invalid parameters
 */
int
ldap_pvt_thread_pool_retract (
	ldap_pvt_thread_pool_t *tpool,
	ldap_pvt_thread_start_t *start_routine, void *arg )
{
	struct ldap_int_thread_pool_s *pool;
	ldap_int_thread_task_t *task;

	if (tpool == NULL)
		return(-1);

	pool = *tpool;

	if (pool == NULL)
		return(-1);

	ldap_pvt_thread_mutex_lock(&pool->ltp_mutex);
	LDAP_STAILQ_FOREACH(task, &pool->ltp_pending_list, ltt_next.q)
		if (task->ltt_start_routine == start_routine &&
			task->ltt_arg == arg) {
			/* Could LDAP_STAILQ_REMOVE the task, but that
			 * walks ltp_pending_list again to find it.
			 */
			task->ltt_start_routine = no_task;
			task->ltt_arg = NULL;
			break;
		}
	ldap_pvt_thread_mutex_unlock(&pool->ltp_mutex);
	return task != NULL;
}

/* Set max #threads.  value <= 0 means max supported #threads (LDAP_MAXTHR) */
int
ldap_pvt_thread_pool_maxthreads(
	ldap_pvt_thread_pool_t *tpool,
	int max_threads )
{
	struct ldap_int_thread_pool_s *pool;

	if (! (0 <= max_threads && max_threads <= LDAP_MAXTHR))
		max_threads = 0;

	if (tpool == NULL)
		return(-1);

	pool = *tpool;

	if (pool == NULL)
		return(-1);

	ldap_pvt_thread_mutex_lock(&pool->ltp_mutex);

	pool->ltp_max_count = max_threads;
	SET_VARY_OPEN_COUNT(pool);

	ldap_pvt_thread_mutex_unlock(&pool->ltp_mutex);
	return(0);
}

/* Inspect the pool */
int
ldap_pvt_thread_pool_query(
	ldap_pvt_thread_pool_t *tpool,
	ldap_pvt_thread_pool_param_t param,
	void *value )
{
	struct ldap_int_thread_pool_s	*pool;
	int				count = -1;

	if ( tpool == NULL || value == NULL ) {
		return -1;
	}

	pool = *tpool;

	if ( pool == NULL ) {
		return 0;
	}

	ldap_pvt_thread_mutex_lock(&pool->ltp_mutex);
	switch ( param ) {
	case LDAP_PVT_THREAD_POOL_PARAM_MAX:
		count = pool->ltp_max_count;
		break;

	case LDAP_PVT_THREAD_POOL_PARAM_MAX_PENDING:
		count = pool->ltp_max_pending;
		if (count < 0)
			count = -count;
		if (count == MAX_PENDING)
			count = 0;
		break;

	case LDAP_PVT_THREAD_POOL_PARAM_OPEN:
		count = pool->ltp_open_count;
		if (count < 0)
			count = -count;
		break;

	case LDAP_PVT_THREAD_POOL_PARAM_STARTING:
		count = pool->ltp_starting;
		break;

	case LDAP_PVT_THREAD_POOL_PARAM_ACTIVE:
		count = pool->ltp_active_count;
		break;

	case LDAP_PVT_THREAD_POOL_PARAM_PAUSING:
		count = (pool->ltp_pause != 0);
		break;

	case LDAP_PVT_THREAD_POOL_PARAM_PENDING:
		count = pool->ltp_pending_count;
		break;

	case LDAP_PVT_THREAD_POOL_PARAM_BACKLOAD:
		count = pool->ltp_pending_count + pool->ltp_active_count;
		break;

	case LDAP_PVT_THREAD_POOL_PARAM_ACTIVE_MAX:
		break;

	case LDAP_PVT_THREAD_POOL_PARAM_PENDING_MAX:
		break;

	case LDAP_PVT_THREAD_POOL_PARAM_BACKLOAD_MAX:
		break;

	case LDAP_PVT_THREAD_POOL_PARAM_STATE:
		*((char **)value) =
			pool->ltp_pause ? "pausing" :
			!pool->ltp_finishing ? "running" :
			pool->ltp_pending_count ? "finishing" : "stopping";
		break;

	case LDAP_PVT_THREAD_POOL_PARAM_UNKNOWN:
		break;
	}
	ldap_pvt_thread_mutex_unlock( &pool->ltp_mutex );

	if ( count > -1 ) {
		*((int *)value) = count;
	}

	return ( count == -1 ? -1 : 0 );
}

/*
 * true if pool is pausing; does not lock any mutex to check.
 * 0 if not pause, 1 if pause, -1 if error or no pool.
 */
int
ldap_pvt_thread_pool_pausing( ldap_pvt_thread_pool_t *tpool )
{
	int rc = -1;
	struct ldap_int_thread_pool_s *pool;

	if ( tpool != NULL && (pool = *tpool) != NULL ) {
		rc = (pool->ltp_pause != 0);
	}

	return rc;
}

/*
 * wrapper for ldap_pvt_thread_pool_query(), left around
 * for backwards compatibility
 */
int
ldap_pvt_thread_pool_backload ( ldap_pvt_thread_pool_t *tpool )
{
	int	rc, count;

	rc = ldap_pvt_thread_pool_query( tpool,
		LDAP_PVT_THREAD_POOL_PARAM_BACKLOAD, (void *)&count );

	if ( rc == 0 ) {
		return count;
	}

	return rc;
}

/* Destroy the pool after making its threads finish */
int
ldap_pvt_thread_pool_destroy ( ldap_pvt_thread_pool_t *tpool, int run_pending )
{
	struct ldap_int_thread_pool_s *pool, *pptr;
	ldap_int_thread_task_t *task;

	if (tpool == NULL)
		return(-1);

	pool = *tpool;

	if (pool == NULL) return(-1);

	ldap_pvt_thread_mutex_lock(&ldap_pvt_thread_pool_mutex);
	LDAP_STAILQ_FOREACH(pptr, &ldap_int_thread_pool_list, ltp_next)
		if (pptr == pool) break;
	if (pptr == pool)
		LDAP_STAILQ_REMOVE(&ldap_int_thread_pool_list, pool,
			ldap_int_thread_pool_s, ltp_next);
	ldap_pvt_thread_mutex_unlock(&ldap_pvt_thread_pool_mutex);

	if (pool != pptr) return(-1);

	ldap_pvt_thread_mutex_lock(&pool->ltp_mutex);

	pool->ltp_finishing = 1;
	SET_VARY_OPEN_COUNT(pool);
	if (pool->ltp_max_pending > 0)
		pool->ltp_max_pending = -pool->ltp_max_pending;

	if (!run_pending) {
		while ((task = LDAP_STAILQ_FIRST(&pool->ltp_pending_list)) != NULL) {
			LDAP_STAILQ_REMOVE_HEAD(&pool->ltp_pending_list, ltt_next.q);
			LDAP_FREE(task);
		}
		pool->ltp_pending_count = 0;
	}

	while (pool->ltp_open_count) {
		if (!pool->ltp_pause)
			ldap_pvt_thread_cond_broadcast(&pool->ltp_cond);
		ldap_pvt_thread_cond_wait(&pool->ltp_cond, &pool->ltp_mutex);
	}

	while ((task = LDAP_SLIST_FIRST(&pool->ltp_free_list)) != NULL)
	{
		LDAP_SLIST_REMOVE_HEAD(&pool->ltp_free_list, ltt_next.l);
		LDAP_FREE(task);
	}

	ldap_pvt_thread_mutex_unlock(&pool->ltp_mutex);
	ldap_pvt_thread_cond_destroy(&pool->ltp_pcond);
	ldap_pvt_thread_cond_destroy(&pool->ltp_cond);
	ldap_pvt_thread_mutex_destroy(&pool->ltp_mutex);
	LDAP_FREE(pool);
	*tpool = NULL;
	ldap_int_has_thread_pool = 0;
	return(0);
}

/* Thread loop.  Accept and handle submitted tasks. */
static void *
ldap_int_thread_pool_wrapper ( 
	void *xpool )
{
	struct ldap_int_thread_pool_s *pool = xpool;
	ldap_int_thread_task_t *task;
	ldap_int_tpool_plist_t *work_list;
	ldap_int_thread_userctx_t ctx, *kctx;
	unsigned i, keyslot, hash;

	assert(pool != NULL);

	for ( i=0; i<MAXKEYS; i++ ) {
		ctx.ltu_key[i].ltk_key = NULL;
	}

	ctx.ltu_id = ldap_pvt_thread_self();
	TID_HASH(ctx.ltu_id, hash);

	ldap_pvt_thread_key_setdata( ldap_tpool_key, &ctx );

	ldap_pvt_thread_mutex_lock(&pool->ltp_mutex);

	/* thread_keys[] is read-only when paused */
	while (pool->ltp_pause)
		ldap_pvt_thread_cond_wait(&pool->ltp_cond, &pool->ltp_mutex);

	/* find a key slot to give this thread ID and store a
	 * pointer to our keys there; start at the thread ID
	 * itself (mod LDAP_MAXTHR) and look for an empty slot.
	 */
	ldap_pvt_thread_mutex_lock(&ldap_pvt_thread_pool_mutex);
	for (keyslot = hash & (LDAP_MAXTHR-1);
		(kctx = thread_keys[keyslot].ctx) && kctx != DELETED_THREAD_CTX;
		keyslot = (keyslot+1) & (LDAP_MAXTHR-1));
	thread_keys[keyslot].ctx = &ctx;
	ldap_pvt_thread_mutex_unlock(&ldap_pvt_thread_pool_mutex);

	pool->ltp_starting--;
	pool->ltp_active_count++;

	for (;;) {
		work_list = pool->ltp_work_list; /* help the compiler a bit */
		task = LDAP_STAILQ_FIRST(work_list);
		if (task == NULL) {	/* paused or no pending tasks */
			if (--(pool->ltp_active_count) < 2) {
				/* Notify pool_pause it is the sole active thread. */
				ldap_pvt_thread_cond_signal(&pool->ltp_pcond);
			}

			do {
				if (pool->ltp_vary_open_count < 0) {
					/* Not paused, and either finishing or too many
					 * threads running (can happen if ltp_max_count
					 * was reduced).  Let this thread die.
					 */
					goto done;
				}

				/* We could check an idle timer here, and let the
				 * thread die if it has been inactive for a while.
				 * Only die if there are other open threads (i.e.,
				 * always have at least one thread open).
				 * The check should be like this:
				 *   if (pool->ltp_open_count>1 && pool->ltp_starting==0)
				 *       check timer, wait if ltp_pause, leave thread;
				 *
				 * Just use pthread_cond_timedwait() if we want to
				 * check idle time.
				 */
				ldap_pvt_thread_cond_wait(&pool->ltp_cond, &pool->ltp_mutex);

				work_list = pool->ltp_work_list;
				task = LDAP_STAILQ_FIRST(work_list);
			} while (task == NULL);

			pool->ltp_active_count++;
		}

		LDAP_STAILQ_REMOVE_HEAD(work_list, ltt_next.q);
		pool->ltp_pending_count--;
		ldap_pvt_thread_mutex_unlock(&pool->ltp_mutex);

		task->ltt_start_routine(&ctx, task->ltt_arg);

		ldap_pvt_thread_mutex_lock(&pool->ltp_mutex);
		LDAP_SLIST_INSERT_HEAD(&pool->ltp_free_list, task, ltt_next.l);
	}
 done:

	assert(!pool->ltp_pause); /* thread_keys writable, ltp_open_count >= 0 */

	/* The ltp_mutex lock protects ctx->ltu_key from pool_purgekey()
	 * during this call, since it prevents new pauses. */
	ldap_pvt_thread_pool_context_reset(&ctx);

	ldap_pvt_thread_mutex_lock(&ldap_pvt_thread_pool_mutex);
	thread_keys[keyslot].ctx = DELETED_THREAD_CTX;
	ldap_pvt_thread_mutex_unlock(&ldap_pvt_thread_pool_mutex);

	pool->ltp_open_count--;
	SET_VARY_OPEN_COUNT(pool);
	/* let pool_destroy know we're all done */
	if (pool->ltp_open_count == 0)
		ldap_pvt_thread_cond_signal(&pool->ltp_cond);

	ldap_pvt_thread_mutex_unlock(&pool->ltp_mutex);

	ldap_pvt_thread_exit(NULL);
	return(NULL);
}

/* Arguments > ltp_pause to handle_pause(,PAUSE_ARG()).  arg=PAUSE_ARG
 * ensures (arg-ltp_pause) sets GO_* at need and keeps DO_PAUSE/GO_*.
 */
#define GO_IDLE		8
#define GO_UNIDLE	16
#define CHECK_PAUSE	32	/* if ltp_pause: GO_IDLE; wait; GO_UNIDLE */
#define DO_PAUSE	64	/* CHECK_PAUSE; pause the pool */
#define PAUSE_ARG(a) \
		((a) | ((a) & (GO_IDLE|GO_UNIDLE) ? GO_IDLE-1 : CHECK_PAUSE))

static int
handle_pause( ldap_pvt_thread_pool_t *tpool, int pause_type )
{
	struct ldap_int_thread_pool_s *pool;
	int ret = 0, pause, max_ltp_pause;

	if (tpool == NULL)
		return(-1);

	pool = *tpool;

	if (pool == NULL)
		return(0);

	if (pause_type == CHECK_PAUSE && !pool->ltp_pause)
		return(0);

	/* Let pool_unidle() ignore requests for new pauses */
	max_ltp_pause = pause_type==PAUSE_ARG(GO_UNIDLE) ? WANT_PAUSE : NOT_PAUSED;

	ldap_pvt_thread_mutex_lock(&pool->ltp_mutex);

	pause = pool->ltp_pause;	/* NOT_PAUSED, WANT_PAUSE or PAUSED */

	/* If ltp_pause and not GO_IDLE|GO_UNIDLE: Set GO_IDLE,GO_UNIDLE */
	pause_type -= pause;

	if (pause_type & GO_IDLE) {
		pool->ltp_pending_count++;
		pool->ltp_active_count--;
		if (pause && pool->ltp_active_count < 2) {
			/* Tell the task waiting to DO_PAUSE it can proceed */
			ldap_pvt_thread_cond_signal(&pool->ltp_pcond);
		}
	}

	if (pause_type & GO_UNIDLE) {
		/* Wait out pause if any, then cancel GO_IDLE */
		if (pause > max_ltp_pause) {
			ret = 1;
			do {
				ldap_pvt_thread_cond_wait(&pool->ltp_cond, &pool->ltp_mutex);
			} while (pool->ltp_pause > max_ltp_pause);
		}
		pool->ltp_pending_count--;
		pool->ltp_active_count++;
	}

	if (pause_type & DO_PAUSE) {
		/* Tell everyone else to pause or finish, then await that */
		ret = 0;
		assert(!pool->ltp_pause);
		pool->ltp_pause = WANT_PAUSE;
		/* Let ldap_pvt_thread_pool_submit() through to its ltp_pause test,
		 * and do not finish threads in ldap_pvt_thread_pool_wrapper() */
		pool->ltp_open_count = -pool->ltp_open_count;
		SET_VARY_OPEN_COUNT(pool);
		/* Hide pending tasks from ldap_pvt_thread_pool_wrapper() */
		pool->ltp_work_list = &empty_pending_list;
		/* Wait for this task to become the sole active task */
		while (pool->ltp_active_count > 1) {
			ldap_pvt_thread_cond_wait(&pool->ltp_pcond, &pool->ltp_mutex);
		}
		assert(pool->ltp_pause == WANT_PAUSE);
		pool->ltp_pause = PAUSED;
	}

	ldap_pvt_thread_mutex_unlock(&pool->ltp_mutex);
	return(ret);
}

/* Consider this task idle: It will not block pool_pause() in other tasks. */
void
ldap_pvt_thread_pool_idle( ldap_pvt_thread_pool_t *tpool )
{
	handle_pause(tpool, PAUSE_ARG(GO_IDLE));
}

/* Cancel pool_idle(). If the pool is paused, wait it out first. */
void
ldap_pvt_thread_pool_unidle( ldap_pvt_thread_pool_t *tpool )
{
	handle_pause(tpool, PAUSE_ARG(GO_UNIDLE));
}

/*
 * If a pause was requested, wait for it.  If several threads
 * are waiting to pause, let through one or more pauses.
 * The calling task must be active, not idle.
 * Return 1 if we waited, 0 if not, -1 at parameter error.
 */
int
ldap_pvt_thread_pool_pausecheck( ldap_pvt_thread_pool_t *tpool )
{
	return handle_pause(tpool, PAUSE_ARG(CHECK_PAUSE));
}

/*
 * Pause the pool.  The calling task must be active, not idle.
 * Return when all other tasks are paused or idle.
 */
int
ldap_pvt_thread_pool_pause( ldap_pvt_thread_pool_t *tpool )
{
	return handle_pause(tpool, PAUSE_ARG(DO_PAUSE));
}

/* End a pause */
int
ldap_pvt_thread_pool_resume ( 
	ldap_pvt_thread_pool_t *tpool )
{
	struct ldap_int_thread_pool_s *pool;

	if (tpool == NULL)
		return(-1);

	pool = *tpool;

	if (pool == NULL)
		return(0);

	ldap_pvt_thread_mutex_lock(&pool->ltp_mutex);

	assert(pool->ltp_pause == PAUSED);
	pool->ltp_pause = 0;
	if (pool->ltp_open_count <= 0) /* true when paused, but be paranoid */
		pool->ltp_open_count = -pool->ltp_open_count;
	SET_VARY_OPEN_COUNT(pool);
	pool->ltp_work_list = &pool->ltp_pending_list;

	ldap_pvt_thread_cond_broadcast(&pool->ltp_cond);

	ldap_pvt_thread_mutex_unlock(&pool->ltp_mutex);
	return(0);
}

/*
 * Get the key's data and optionally free function in the given context.
 */
int ldap_pvt_thread_pool_getkey(
	void *xctx,
	void *key,
	void **data,
	ldap_pvt_thread_pool_keyfree_t **kfree )
{
	ldap_int_thread_userctx_t *ctx = xctx;
	int i;

	if ( !ctx || !key || !data ) return EINVAL;

	for ( i=0; i<MAXKEYS && ctx->ltu_key[i].ltk_key; i++ ) {
		if ( ctx->ltu_key[i].ltk_key == key ) {
			*data = ctx->ltu_key[i].ltk_data;
			if ( kfree ) *kfree = ctx->ltu_key[i].ltk_free;
			return 0;
		}
	}
	return ENOENT;
}

static void
clear_key_idx( ldap_int_thread_userctx_t *ctx, int i )
{
	for ( ; i < MAXKEYS-1 && ctx->ltu_key[i+1].ltk_key; i++ )
		ctx->ltu_key[i] = ctx->ltu_key[i+1];
	ctx->ltu_key[i].ltk_key = NULL;
}

/*
 * Set or remove data for the key in the given context.
 * key can be any unique pointer.
 * kfree() is an optional function to free the data (but not the key):
 *   pool_context_reset() and pool_purgekey() call kfree(key, data),
 *   but pool_setkey() does not.  For pool_setkey() it is the caller's
 *   responsibility to free any existing data with the same key.
 *   kfree() must not call functions taking a tpool argument.
 */
int ldap_pvt_thread_pool_setkey(
	void *xctx,
	void *key,
	void *data,
	ldap_pvt_thread_pool_keyfree_t *kfree,
	void **olddatap,
	ldap_pvt_thread_pool_keyfree_t **oldkfreep )
{
	ldap_int_thread_userctx_t *ctx = xctx;
	int i, found;

	if ( !ctx || !key ) return EINVAL;

	for ( i=found=0; i<MAXKEYS; i++ ) {
		if ( ctx->ltu_key[i].ltk_key == key ) {
			found = 1;
			break;
		} else if ( !ctx->ltu_key[i].ltk_key ) {
			break;
		}
	}

	if ( olddatap ) {
		if ( found ) {
			*olddatap = ctx->ltu_key[i].ltk_data;
		} else {
			*olddatap = NULL;
		}
	}

	if ( oldkfreep ) {
		if ( found ) {
			*oldkfreep = ctx->ltu_key[i].ltk_free;
		} else {
			*oldkfreep = 0;
		}
	}

	if ( data || kfree ) {
		if ( i>=MAXKEYS )
			return ENOMEM;
		ctx->ltu_key[i].ltk_key = key;
		ctx->ltu_key[i].ltk_data = data;
		ctx->ltu_key[i].ltk_free = kfree;
	} else if ( found ) {
		clear_key_idx( ctx, i );
	}

	return 0;
}

/* Free all elements with this key, no matter which thread they're in.
 * May only be called while the pool is paused.
 */
void ldap_pvt_thread_pool_purgekey( void *key )
{
	int i, j;
	ldap_int_thread_userctx_t *ctx;

	assert ( key != NULL );

	for ( i=0; i<LDAP_MAXTHR; i++ ) {
		ctx = thread_keys[i].ctx;
		if ( ctx && ctx != DELETED_THREAD_CTX ) {
			for ( j=0; j<MAXKEYS && ctx->ltu_key[j].ltk_key; j++ ) {
				if ( ctx->ltu_key[j].ltk_key == key ) {
					if (ctx->ltu_key[j].ltk_free)
						ctx->ltu_key[j].ltk_free( ctx->ltu_key[j].ltk_key,
						ctx->ltu_key[j].ltk_data );
					clear_key_idx( ctx, j );
					break;
				}
			}
		}
	}
}

/*
 * Find the context of the current thread.
 * This is necessary if the caller does not have access to the
 * thread context handle (for example, a slapd plugin calling
 * slapi_search_internal()). No doubt it is more efficient
 * for the application to keep track of the thread context
 * handles itself.
 */
void *ldap_pvt_thread_pool_context( )
{
	void *ctx = NULL;

	ldap_pvt_thread_key_getdata( ldap_tpool_key, &ctx );
	return ctx ? ctx : (void *) &ldap_int_main_thrctx;
}

/*
 * Free the context's keys.
 * Must not call functions taking a tpool argument (because this
 * thread already holds ltp_mutex when called from pool_wrapper()).
 */
void ldap_pvt_thread_pool_context_reset( void *vctx )
{
	ldap_int_thread_userctx_t *ctx = vctx;
	int i;

	for ( i=MAXKEYS-1; i>=0; i--) {
		if ( !ctx->ltu_key[i].ltk_key )
			continue;
		if ( ctx->ltu_key[i].ltk_free )
			ctx->ltu_key[i].ltk_free( ctx->ltu_key[i].ltk_key,
			ctx->ltu_key[i].ltk_data );
		ctx->ltu_key[i].ltk_key = NULL;
	}
}

ldap_pvt_thread_t ldap_pvt_thread_pool_tid( void *vctx )
{
	ldap_int_thread_userctx_t *ctx = vctx;

	return ctx->ltu_id;
}
#endif /* LDAP_THREAD_HAVE_TPOOL */
