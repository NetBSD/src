/*	$NetBSD: linux_dma_resv.c,v 1.22 2022/02/15 22:51:03 riastradh Exp $	*/

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: linux_dma_resv.c,v 1.22 2022/02/15 22:51:03 riastradh Exp $");

#include <sys/param.h>
#include <sys/poll.h>
#include <sys/select.h>

#include <linux/dma-fence.h>
#include <linux/dma-resv.h>
#include <linux/seqlock.h>
#include <linux/ww_mutex.h>

DEFINE_WW_CLASS(reservation_ww_class __cacheline_aligned);

static struct dma_resv_list *
objlist_tryalloc(uint32_t n)
{
	struct dma_resv_list *list;

	list = kmem_alloc(offsetof(typeof(*list), shared[n]), KM_NOSLEEP);
	if (list == NULL)
		return NULL;
	list->shared_max = n;

	return list;
}

static struct dma_resv_list *
objlist_alloc(uint32_t n)
{
	struct dma_resv_list *list;

	list = kmem_alloc(offsetof(typeof(*list), shared[n]), KM_SLEEP);
	list->shared_max = n;

	return list;
}

static void
objlist_free(struct dma_resv_list *list)
{
	uint32_t n = list->shared_max;

	kmem_free(list, offsetof(typeof(*list), shared[n]));
}

static void
objlist_free_cb(struct rcu_head *rcu)
{
	struct dma_resv_list *list = container_of(rcu,
	    struct dma_resv_list, rol_rcu);

	objlist_free(list);
}

static void
objlist_defer_free(struct dma_resv_list *list)
{

	call_rcu(&list->rol_rcu, objlist_free_cb);
}

/*
 * dma_resv_init(robj)
 *
 *	Initialize a reservation object.  Caller must later destroy it
 *	with dma_resv_fini.
 */
void
dma_resv_init(struct dma_resv *robj)
{

	ww_mutex_init(&robj->lock, &reservation_ww_class);
	seqcount_init(&robj->seq);
	robj->fence_excl = NULL;
	robj->fence = NULL;
	robj->robj_prealloc = NULL;
}

/*
 * dma_resv_fini(robj)
 *
 *	Destroy a reservation object, freeing any memory that had been
 *	allocated for it.  Caller must have exclusive access to it.
 */
void
dma_resv_fini(struct dma_resv *robj)
{
	unsigned i;

	if (robj->robj_prealloc) {
		objlist_free(robj->robj_prealloc);
		robj->robj_prealloc = NULL; /* paranoia */
	}
	if (robj->fence) {
		for (i = 0; i < robj->fence->shared_count; i++) {
			dma_fence_put(robj->fence->shared[i]);
			robj->fence->shared[i] = NULL; /* paranoia */
		}
		objlist_free(robj->fence);
		robj->fence = NULL; /* paranoia */
	}
	if (robj->fence_excl) {
		dma_fence_put(robj->fence_excl);
		robj->fence_excl = NULL; /* paranoia */
	}
	ww_mutex_destroy(&robj->lock);
}

/*
 * dma_resv_lock(robj, ctx)
 *
 *	Acquire a reservation object's lock.  Return 0 on success,
 *	-EALREADY if caller already holds it, -EDEADLK if a
 *	higher-priority owner holds it and the caller must back out and
 *	retry.
 */
int
dma_resv_lock(struct dma_resv *robj,
    struct ww_acquire_ctx *ctx)
{

	return ww_mutex_lock(&robj->lock, ctx);
}

/*
 * dma_resv_lock_slow(robj, ctx)
 *
 *	Acquire a reservation object's lock.  Caller must not hold
 *	this lock or any others -- this is to be used in slow paths
 *	after dma_resv_lock or dma_resv_lock_interruptible has failed
 *	and the caller has backed out all other locks.
 */
void
dma_resv_lock_slow(struct dma_resv *robj,
    struct ww_acquire_ctx *ctx)
{

	ww_mutex_lock_slow(&robj->lock, ctx);
}

/*
 * dma_resv_lock_interruptible(robj, ctx)
 *
 *	Acquire a reservation object's lock.  Return 0 on success,
 *	-EALREADY if caller already holds it, -EDEADLK if a
 *	higher-priority owner holds it and the caller must back out and
 *	retry, -EINTR if interrupted.
 */
int
dma_resv_lock_interruptible(struct dma_resv *robj,
    struct ww_acquire_ctx *ctx)
{

	return ww_mutex_lock_interruptible(&robj->lock, ctx);
}

/*
 * dma_resv_lock_slow_interruptible(robj, ctx)
 *
 *	Acquire a reservation object's lock.  Caller must not hold
 *	this lock or any others -- this is to be used in slow paths
 *	after dma_resv_lock or dma_resv_lock_interruptible has failed
 *	and the caller has backed out all other locks.  Return 0 on
 *	success, -EINTR if interrupted.
 */
int
dma_resv_lock_slow_interruptible(struct dma_resv *robj,
    struct ww_acquire_ctx *ctx)
{

	return ww_mutex_lock_slow_interruptible(&robj->lock, ctx);
}

/*
 * dma_resv_trylock(robj)
 *
 *	Try to acquire a reservation object's lock without blocking.
 *	Return true on success, false on failure.
 */
bool
dma_resv_trylock(struct dma_resv *robj)
{

	return ww_mutex_trylock(&robj->lock);
}

/*
 * dma_resv_locking_ctx(robj)
 *
 *	Return a pointer to the ww_acquire_ctx used by the owner of
 *	the reservation object's lock, or NULL if it is either not
 *	owned or if it is locked without context.
 */
struct ww_acquire_ctx *
dma_resv_locking_ctx(struct dma_resv *robj)
{

	return ww_mutex_locking_ctx(&robj->lock);
}

/*
 * dma_resv_unlock(robj)
 *
 *	Release a reservation object's lock.
 */
void
dma_resv_unlock(struct dma_resv *robj)
{

	return ww_mutex_unlock(&robj->lock);
}

/*
 * dma_resv_is_locked(robj)
 *
 *	True if robj is locked.
 */
bool
dma_resv_is_locked(struct dma_resv *robj)
{

	return ww_mutex_is_locked(&robj->lock);
}

/*
 * dma_resv_held(robj)
 *
 *	True if robj is locked.
 */
bool
dma_resv_held(struct dma_resv *robj)
{

	return ww_mutex_is_locked(&robj->lock);
}

/*
 * dma_resv_assert_held(robj)
 *
 *	Panic if robj is not held, in DIAGNOSTIC builds.
 */
void
dma_resv_assert_held(struct dma_resv *robj)
{

	KASSERT(dma_resv_held(robj));
}

/*
 * dma_resv_get_excl(robj)
 *
 *	Return a pointer to the exclusive fence of the reservation
 *	object robj.
 *
 *	Caller must have robj locked.
 */
struct dma_fence *
dma_resv_get_excl(struct dma_resv *robj)
{

	KASSERT(dma_resv_held(robj));
	return robj->fence_excl;
}

/*
 * dma_resv_get_list(robj)
 *
 *	Return a pointer to the shared fence list of the reservation
 *	object robj.
 *
 *	Caller must have robj locked.
 */
struct dma_resv_list *
dma_resv_get_list(struct dma_resv *robj)
{

	KASSERT(dma_resv_held(robj));
	return robj->fence;
}

/*
 * dma_resv_reserve_shared(robj, num_fences)
 *
 *	Reserve space in robj to add num_fences shared fences.  To be
 *	used only once before calling dma_resv_add_shared_fence.
 *
 *	Caller must have robj locked.
 *
 *	Internally, we start with room for four entries and double if
 *	we don't have enough.  This is not guaranteed.
 */
int
dma_resv_reserve_shared(struct dma_resv *robj, unsigned int num_fences)
{
	struct dma_resv_list *list, *prealloc;
	uint32_t n, nalloc;

	KASSERT(dma_resv_held(robj));

	list = robj->fence;
	prealloc = robj->robj_prealloc;

	/* If there's an existing list, check it for space.  */
	if (list) {
		/* If there's too many already, give up.  */
		if (list->shared_count > UINT32_MAX - num_fences)
			return -ENOMEM;

		/* Add some more. */
		n = list->shared_count + num_fences;

		/* If there's enough for one more, we're done.  */
		if (n <= list->shared_max)
			return 0;
	} else {
		/* No list already.  We need space for num_fences.  */
		n = num_fences;
	}

	/* If not, maybe there's a preallocated list ready.  */
	if (prealloc != NULL) {
		/* If there's enough room in it, stop here.  */
		if (n <= prealloc->shared_max)
			return 0;

		/* Try to double its capacity.  */
		nalloc = n > UINT32_MAX/2 ? UINT32_MAX : 2*n;
		prealloc = objlist_alloc(nalloc);

		/* Swap the new preallocated list and free the old one.  */
		objlist_free(robj->robj_prealloc);
		robj->robj_prealloc = prealloc;
	} else {
		/* Start with some spare.  */
		nalloc = n > UINT32_MAX/2 ? UINT32_MAX : MAX(2*n, 4);
		prealloc = objlist_alloc(nalloc);

		/* Save the new preallocated list.  */
		robj->robj_prealloc = prealloc;
	}

	/* Success!  */
	return 0;
}

struct dma_resv_write_ticket {
};

/*
 * dma_resv_write_begin(robj, ticket)
 *
 *	Begin an atomic batch of writes to robj, and initialize opaque
 *	ticket for it.  The ticket must be passed to
 *	dma_resv_write_commit to commit the writes.
 *
 *	Caller must have robj locked.
 *
 *	Implies membar_producer, i.e. store-before-store barrier.  Does
 *	NOT serve as an acquire operation, however.
 */
static void
dma_resv_write_begin(struct dma_resv *robj,
    struct dma_resv_write_ticket *ticket)
{

	KASSERT(dma_resv_held(robj));

	write_seqcount_begin(&robj->seq);
}

/*
 * dma_resv_write_commit(robj, ticket)
 *
 *	Commit an atomic batch of writes to robj begun with the call to
 *	dma_resv_write_begin that returned ticket.
 *
 *	Caller must have robj locked.
 *
 *	Implies membar_producer, i.e. store-before-store barrier.  Does
 *	NOT serve as a release operation, however.
 */
static void
dma_resv_write_commit(struct dma_resv *robj,
    struct dma_resv_write_ticket *ticket)
{

	KASSERT(dma_resv_held(robj));

	write_seqcount_end(&robj->seq);
}

struct dma_resv_read_ticket {
	unsigned version;
};

/*
 * dma_resv_read_begin(robj, ticket)
 *
 *	Begin a read section, and initialize opaque ticket for it.  The
 *	ticket must be passed to dma_resv_read_exit, and the
 *	caller must be prepared to retry reading if it fails.
 */
static void
dma_resv_read_begin(const struct dma_resv *robj,
    struct dma_resv_read_ticket *ticket)
{

	ticket->version = read_seqcount_begin(&robj->seq);
}

/*
 * dma_resv_read_valid(robj, ticket)
 *
 *	Test whether the read sections are valid.  Return true on
 *	success, or false on failure if the read ticket has been
 *	invalidated.
 */
static bool
dma_resv_read_valid(const struct dma_resv *robj,
    struct dma_resv_read_ticket *ticket)
{

	return !read_seqcount_retry(&robj->seq, ticket->version);
}

/*
 * dma_resv_get_shared_reader(robj, listp, shared_countp, ticket)
 *
 *	Set *listp and *shared_countp to a snapshot of the pointer to
 *	and length of the shared fence list of robj and return true, or
 *	set them to NULL/0 and return false if a writer intervened so
 *	the caller must start over.
 *
 *	Both *listp and *shared_countp are unconditionally initialized
 *	on return.  They may be NULL/0 even on success, if there is no
 *	shared list at the moment.  Does not take any fence references.
 */
static bool
dma_resv_get_shared_reader(const struct dma_resv *robj,
    const struct dma_resv_list **listp, unsigned *shared_countp,
    struct dma_resv_read_ticket *ticket)
{
	struct dma_resv_list *list;
	unsigned shared_count = 0;

	/*
	 * Get the list and, if it is present, its length.  If the list
	 * is present, it has a valid length.  The atomic_load_consume
	 * pairs with the membar_producer in dma_resv_write_begin.
	 */
	list = atomic_load_consume(&robj->fence);
	shared_count = list ? atomic_load_relaxed(&list->shared_count) : 0;

	/*
	 * We are done reading from robj and list.  Validate our
	 * parking ticket.  If it's invalid, do not pass go and do not
	 * collect $200.
	 */
	if (!dma_resv_read_valid(robj, ticket))
		goto fail;

	/* Success!  */
	*listp = list;
	*shared_countp = shared_count;
	return true;

fail:	*listp = NULL;
	*shared_countp = 0;
	return false;
}

/*
 * dma_resv_get_excl_reader(robj, fencep, ticket)
 *
 *	Set *fencep to the exclusive fence of robj and return true, or
 *	set it to NULL and return false if either
 *	(a) a writer intervened, or
 *	(b) the fence is scheduled to be destroyed after this RCU grace
 *	    period,
 *	in either case meaning the caller must restart.
 *
 *	The value of *fencep is unconditionally initialized on return.
 *	It may be NULL, if there is no exclusive fence at the moment.
 *	If nonnull, *fencep is referenced; caller must dma_fence_put.
 */
static bool
dma_resv_get_excl_reader(const struct dma_resv *robj,
    struct dma_fence **fencep,
    struct dma_resv_read_ticket *ticket)
{
	struct dma_fence *fence;

	/*
	 * Get the candidate fence pointer.  The atomic_load_consume
	 * pairs with the membar_consumer in dma_resv_write_begin.
	 */
	fence = atomic_load_consume(&robj->fence_excl);

	/*
	 * The load of robj->fence_excl is atomic, but the caller may
	 * have previously loaded the shared fence list and should
	 * restart if its view of the entire dma_resv object is not a
	 * consistent snapshot.
	 */
	if (!dma_resv_read_valid(robj, ticket))
		goto fail;

	/*
	 * If the fence is already scheduled to away after this RCU
	 * read section, give up.  Otherwise, take a reference so it
	 * won't go away until after dma_fence_put.
	 */
	if (fence != NULL &&
	    (fence = dma_fence_get_rcu(fence)) == NULL)
		goto fail;

	/* Success!  */
	*fencep = fence;
	return true;

fail:	*fencep = NULL;
	return false;
}

/*
 * dma_resv_add_excl_fence(robj, fence)
 *
 *	Empty and release all of robj's shared fences, and clear and
 *	release its exclusive fence.  If fence is nonnull, acquire a
 *	reference to it and save it as robj's exclusive fence.
 *
 *	Caller must have robj locked.
 */
void
dma_resv_add_excl_fence(struct dma_resv *robj,
    struct dma_fence *fence)
{
	struct dma_fence *old_fence = robj->fence_excl;
	struct dma_resv_list *old_list = robj->fence;
	uint32_t old_shared_count;
	struct dma_resv_write_ticket ticket;

	KASSERT(dma_resv_held(robj));

	/*
	 * If we are setting rather than just removing a fence, acquire
	 * a reference for ourselves.
	 */
	if (fence)
		(void)dma_fence_get(fence);

	/* If there are any shared fences, remember how many.  */
	if (old_list)
		old_shared_count = old_list->shared_count;

	/* Begin an update.  Implies membar_producer for fence.  */
	dma_resv_write_begin(robj, &ticket);

	/* Replace the fence and zero the shared count.  */
	atomic_store_relaxed(&robj->fence_excl, fence);
	if (old_list)
		old_list->shared_count = 0;

	/* Commit the update.  */
	dma_resv_write_commit(robj, &ticket);

	/* Release the old exclusive fence, if any.  */
	if (old_fence) {
		dma_fence_put(old_fence);
		old_fence = NULL; /* paranoia */
	}

	/* Release any old shared fences.  */
	if (old_list) {
		while (old_shared_count--) {
			dma_fence_put(old_list->shared[old_shared_count]);
			/* paranoia */
			old_list->shared[old_shared_count] = NULL;
		}
	}
}

/*
 * dma_resv_add_shared_fence(robj, fence)
 *
 *	Acquire a reference to fence and add it to robj's shared list.
 *	If any fence was already added with the same context number,
 *	release it and replace it by this one.
 *
 *	Caller must have robj locked, and must have preceded with a
 *	call to dma_resv_reserve_shared for each shared fence
 *	added.
 */
void
dma_resv_add_shared_fence(struct dma_resv *robj,
    struct dma_fence *fence)
{
	struct dma_resv_list *list = robj->fence;
	struct dma_resv_list *prealloc = robj->robj_prealloc;
	struct dma_resv_write_ticket ticket;
	struct dma_fence *replace = NULL;
	uint32_t i;

	KASSERT(dma_resv_held(robj));

	/* Acquire a reference to the fence.  */
	KASSERT(fence != NULL);
	(void)dma_fence_get(fence);

	/* Check for a preallocated replacement list.  */
	if (prealloc == NULL) {
		/*
		 * If there is no preallocated replacement list, then
		 * there must be room in the current list.
		 */
		KASSERT(list != NULL);
		KASSERT(list->shared_count < list->shared_max);

		/* Begin an update.  Implies membar_producer for fence.  */
		dma_resv_write_begin(robj, &ticket);

		/* Find a fence with the same context number.  */
		for (i = 0; i < list->shared_count; i++) {
			if (list->shared[i]->context == fence->context) {
				replace = list->shared[i];
				atomic_store_relaxed(&list->shared[i], fence);
				break;
			}
		}

		/* If we didn't find one, add it at the end.  */
		if (i == list->shared_count) {
			atomic_store_relaxed(&list->shared[list->shared_count],
			    fence);
			atomic_store_relaxed(&list->shared_count,
			    list->shared_count + 1);
		}

		/* Commit the update.  */
		dma_resv_write_commit(robj, &ticket);
	} else {
		/*
		 * There is a preallocated replacement list.  There may
		 * not be a current list.  If not, treat it as a zero-
		 * length list.
		 */
		uint32_t shared_count = (list == NULL? 0 : list->shared_count);

		/* There had better be room in the preallocated list.  */
		KASSERT(shared_count < prealloc->shared_max);

		/*
		 * Copy the fences over, but replace if we find one
		 * with the same context number.
		 */
		for (i = 0; i < shared_count; i++) {
			if (replace == NULL &&
			    list->shared[i]->context == fence->context) {
				replace = list->shared[i];
				prealloc->shared[i] = fence;
			} else {
				prealloc->shared[i] = list->shared[i];
			}
		}
		prealloc->shared_count = shared_count;

		/* If we didn't find one, add it at the end.  */
		if (replace == NULL) {
			KASSERT(prealloc->shared_count < prealloc->shared_max);
			prealloc->shared[prealloc->shared_count++] = fence;
		}

		/*
		 * Now ready to replace the list.  Begin an update.
		 * Implies membar_producer for fence and prealloc.
		 */
		dma_resv_write_begin(robj, &ticket);

		/* Replace the list.  */
		atomic_store_relaxed(&robj->fence, prealloc);
		robj->robj_prealloc = NULL;

		/* Commit the update.  */
		dma_resv_write_commit(robj, &ticket);

		/*
		 * If there is an old list, free it when convenient.
		 * (We are not in a position at this point to sleep
		 * waiting for activity on all CPUs.)
		 */
		if (list)
			objlist_defer_free(list);
	}

	/* Release a fence if we replaced it.  */
	if (replace) {
		dma_fence_put(replace);
		replace = NULL;	/* paranoia */
	}
}

/*
 * dma_resv_get_excl_rcu(robj)
 *
 *	Note: Caller need not call this from an RCU read section.
 */
struct dma_fence *
dma_resv_get_excl_rcu(const struct dma_resv *robj)
{
	struct dma_fence *fence;

	rcu_read_lock();
	fence = dma_fence_get_rcu_safe(&robj->fence_excl);
	rcu_read_unlock();

	return fence;
}

/*
 * dma_resv_get_fences_rcu(robj, fencep, nsharedp, sharedp)
 *
 *	Get a snapshot of the exclusive and shared fences of robj.  The
 *	shared fences are returned as a pointer *sharedp to an array,
 *	to be freed by the caller with kfree, of *nsharedp elements.
 *	If fencep is null, then add the exclusive fence, if any, at the
 *	end of the array instead.
 *
 *	Returns zero on success, negative (Linux-style) error code on
 *	failure.  On failure, *fencep, *nsharedp, and *sharedp are
 *	untouched.
 */
int
dma_resv_get_fences_rcu(const struct dma_resv *robj,
    struct dma_fence **fencep, unsigned *nsharedp, struct dma_fence ***sharedp)
{
	const struct dma_resv_list *list = NULL;
	struct dma_fence *fence = NULL;
	struct dma_fence **shared = NULL;
	unsigned shared_alloc = 0, shared_count, i;
	struct dma_resv_read_ticket ticket;

top:	KASSERT(fence == NULL);

	/* Enter an RCU read section and get a read ticket.  */
	rcu_read_lock();
	dma_resv_read_begin(robj, &ticket);

	/* If there is a shared list, grab it.  */
	if (!dma_resv_get_shared_reader(robj, &list, &shared_count, &ticket))
		goto restart;
	if (list != NULL) {

		/*
		 * Avoid arithmetic overflow with `+ 1' below.
		 * Strictly speaking we don't need this if the caller
		 * specified fencep or if there is no exclusive fence,
		 * but it is simpler to not have to consider those
		 * cases.
		 */
		KASSERT(shared_count <= list->shared_max);
		if (list->shared_max == UINT_MAX)
			return -ENOMEM;

		/* Check whether we have a buffer.  */
		if (shared == NULL) {
			/*
			 * We don't have a buffer yet.  Try to allocate
			 * one without waiting.
			 */
			shared_alloc = list->shared_max + 1;
			shared = kcalloc(shared_alloc, sizeof(shared[0]),
			    GFP_NOWAIT);
			if (shared == NULL) {
				/*
				 * Couldn't do it immediately.  Back
				 * out of RCU and allocate one with
				 * waiting.
				 */
				rcu_read_unlock();
				shared = kcalloc(shared_alloc,
				    sizeof(shared[0]), GFP_KERNEL);
				if (shared == NULL)
					return -ENOMEM;
				goto top;
			}
		} else if (shared_alloc < list->shared_max + 1) {
			/*
			 * We have a buffer but it's too small.  We're
			 * already racing in this case, so just back
			 * out and wait to allocate a bigger one.
			 */
			shared_alloc = list->shared_max + 1;
			rcu_read_unlock();
			kfree(shared);
			shared = kcalloc(shared_alloc, sizeof(shared[0]),
			    GFP_KERNEL);
			if (shared == NULL)
				return -ENOMEM;
			goto top;
		}

		/*
		 * We got a buffer large enough.  Copy into the buffer
		 * and record the number of elements.  Could safely use
		 * memcpy here, because even if we race with a writer
		 * it'll invalidate the read ticket and we'll start
		 * over, but atomic_load in a loop will pacify kcsan.
		 */
		for (i = 0; i < shared_count; i++)
			shared[i] = atomic_load_relaxed(&list->shared[i]);

		/* If anything changed while we were copying, restart.  */
		if (!dma_resv_read_valid(robj, &ticket))
			goto restart;
	}

	/* If there is an exclusive fence, grab it.  */
	KASSERT(fence == NULL);
	if (!dma_resv_get_excl_reader(robj, &fence, &ticket))
		goto restart;

	/*
	 * Try to get a reference to all of the shared fences.
	 */
	for (i = 0; i < shared_count; i++) {
		if (dma_fence_get_rcu(atomic_load_relaxed(&shared[i])) == NULL)
			goto put_restart;
	}

	/* Success!  */
	rcu_read_unlock();
	KASSERT(shared_count <= shared_alloc);
	KASSERT(shared_alloc == 0 || shared_count < shared_alloc);
	KASSERT(shared_alloc <= UINT_MAX);
	if (fencep) {
		*fencep = fence;
	} else if (fence) {
		if (shared_count) {
			shared[shared_count++] = fence;
		} else {
			shared = kmalloc(sizeof(shared[0]), GFP_KERNEL);
			shared[0] = fence;
			shared_count = 1;
		}
	}
	*nsharedp = shared_count;
	*sharedp = shared;
	return 0;

put_restart:
	/* Back out.  */
	while (i --> 0) {
		dma_fence_put(shared[i]);
		shared[i] = NULL; /* paranoia */
	}
	if (fence) {
		dma_fence_put(fence);
		fence = NULL;
	}

restart:
	KASSERT(fence == NULL);
	rcu_read_unlock();
	goto top;
}

/*
 * dma_resv_copy_fences(dst, src)
 *
 *	Copy the exclusive fence and all the shared fences from src to
 *	dst.
 *
 *	Caller must have dst locked.
 */
int
dma_resv_copy_fences(struct dma_resv *dst_robj,
    const struct dma_resv *src_robj)
{
	const struct dma_resv_list *src_list;
	struct dma_resv_list *dst_list = NULL;
	struct dma_resv_list *old_list;
	struct dma_fence *fence = NULL;
	struct dma_fence *old_fence;
	uint32_t shared_count, i;
	struct dma_resv_read_ticket read_ticket;
	struct dma_resv_write_ticket write_ticket;

	KASSERT(dma_resv_held(dst_robj));

top:	KASSERT(fence == NULL);

	/* Enter an RCU read section and get a read ticket.  */
	rcu_read_lock();
	dma_resv_read_begin(src_robj, &read_ticket);

	/* Get the shared list.  */
	if (!dma_resv_get_shared_reader(src_robj, &src_list, &shared_count,
		&read_ticket))
		goto restart;
	if (src_list) {
		/* Allocate a new list, if necessary.  */
		if (dst_list == NULL)
			dst_list = objlist_tryalloc(shared_count);
		if (dst_list == NULL || dst_list->shared_max < shared_count) {
			rcu_read_unlock();
			if (dst_list) {
				objlist_free(dst_list);
				dst_list = NULL;
			}
			dst_list = objlist_alloc(shared_count);
			dst_list->shared_count = 0; /* paranoia */
			goto top;
		}

		/* Copy over all fences that are not yet signalled.  */
		dst_list->shared_count = 0;
		for (i = 0; i < shared_count; i++) {
			KASSERT(fence == NULL);
			fence = atomic_load_relaxed(&src_list->shared[i]);
			if ((fence = dma_fence_get_rcu(fence)) == NULL)
				goto restart;
			if (dma_fence_is_signaled(fence)) {
				dma_fence_put(fence);
				fence = NULL;
				continue;
			}
			dst_list->shared[dst_list->shared_count++] = fence;
			fence = NULL;
		}

		/* If anything changed while we were copying, restart.  */
		if (!dma_resv_read_valid(src_robj, &read_ticket))
			goto restart;
	}

	/* Get the exclusive fence.  */
	KASSERT(fence == NULL);
	if (!dma_resv_get_excl_reader(src_robj, &fence, &read_ticket))
		goto restart;

	/* All done with src; exit the RCU read section.  */
	rcu_read_unlock();

	/*
	 * We now have a snapshot of the shared and exclusive fences of
	 * src_robj and we have acquired references to them so they
	 * won't go away.  Transfer them over to dst_robj, releasing
	 * references to any that were there.
	 */

	/* Get the old shared and exclusive fences, if any.  */
	old_list = dst_robj->fence;
	old_fence = dst_robj->fence_excl;

	/*
	 * Begin an update.  Implies membar_producer for dst_list and
	 * fence.
	 */
	dma_resv_write_begin(dst_robj, &write_ticket);

	/* Replace the fences.  */
	atomic_store_relaxed(&dst_robj->fence, dst_list);
	atomic_store_relaxed(&dst_robj->fence_excl, fence);

	/* Commit the update.  */
	dma_resv_write_commit(dst_robj, &write_ticket);

	/* Release the old exclusive fence, if any.  */
	if (old_fence) {
		dma_fence_put(old_fence);
		old_fence = NULL; /* paranoia */
	}

	/* Release any old shared fences.  */
	if (old_list) {
		for (i = old_list->shared_count; i --> 0;) {
			dma_fence_put(old_list->shared[i]);
			old_list->shared[i] = NULL; /* paranoia */
		}
		objlist_free(old_list);
		old_list = NULL; /* paranoia */
	}

	/* Success!  */
	return 0;

restart:
	KASSERT(fence == NULL);
	rcu_read_unlock();
	if (dst_list) {
		for (i = dst_list->shared_count; i --> 0;) {
			dma_fence_put(dst_list->shared[i]);
			dst_list->shared[i] = NULL; /* paranoia */
		}
		/* reuse dst_list allocation for the next attempt */
	}
	goto top;
}

/*
 * dma_resv_test_signaled_rcu(robj, shared)
 *
 *	If shared is true, test whether all of the shared fences are
 *	signalled, or if there are none, test whether the exclusive
 *	fence is signalled.  If shared is false, test only whether the
 *	exclusive fence is signalled.
 *
 *	XXX Why does this _not_ test the exclusive fence if shared is
 *	true only if there are no shared fences?  This makes no sense.
 */
bool
dma_resv_test_signaled_rcu(const struct dma_resv *robj,
    bool shared)
{
	struct dma_resv_read_ticket ticket;
	const struct dma_resv_list *list;
	struct dma_fence *fence = NULL;
	uint32_t i, shared_count;
	bool signaled = true;

top:	KASSERT(fence == NULL);

	/* Enter an RCU read section and get a read ticket.  */
	rcu_read_lock();
	dma_resv_read_begin(robj, &ticket);

	/* If shared is requested and there is a shared list, test it.  */
	if (shared) {
		if (!dma_resv_get_shared_reader(robj, &list, &shared_count,
			&ticket))
			goto restart;
	} else {
		list = NULL;
		shared_count = 0;
	}
	if (list != NULL) {
		/*
		 * For each fence, if it is going away, restart.
		 * Otherwise, acquire a reference to it to test whether
		 * it is signalled.  Stop if we find any that is not
		 * signalled.
		 */
		for (i = 0; i < shared_count; i++) {
			KASSERT(fence == NULL);
			fence = atomic_load_relaxed(&list->shared[i]);
			if ((fence = dma_fence_get_rcu(fence)) == NULL)
				goto restart;
			signaled &= dma_fence_is_signaled(fence);
			dma_fence_put(fence);
			fence = NULL;
			if (!signaled)
				goto out;
		}

		/* If anything changed while we were testing, restart.  */
		if (!dma_resv_read_valid(robj, &ticket))
			goto restart;
	}
	if (shared_count)
		goto out;

	/* If there is an exclusive fence, test it.  */
	KASSERT(fence == NULL);
	if (!dma_resv_get_excl_reader(robj, &fence, &ticket))
		goto restart;
	if (fence != NULL) {
		/* Test whether it is signalled.  If no, stop.  */
		signaled &= dma_fence_is_signaled(fence);
		dma_fence_put(fence);
		fence = NULL;
		if (!signaled)
			goto out;
	}

out:	KASSERT(fence == NULL);
	rcu_read_unlock();
	return signaled;

restart:
	KASSERT(fence == NULL);
	rcu_read_unlock();
	goto top;
}

/*
 * dma_resv_wait_timeout_rcu(robj, shared, intr, timeout)
 *
 *	If shared is true, wait for all of the shared fences to be
 *	signalled, or if there are none, wait for the exclusive fence
 *	to be signalled.  If shared is false, wait only for the
 *	exclusive fence to be signalled.  If timeout is zero, don't
 *	wait, only test.
 *
 *	XXX Why does this _not_ wait for the exclusive fence if shared
 *	is true only if there are no shared fences?  This makes no
 *	sense.
 */
long
dma_resv_wait_timeout_rcu(const struct dma_resv *robj,
    bool shared, bool intr, unsigned long timeout)
{
	struct dma_resv_read_ticket ticket;
	const struct dma_resv_list *list;
	struct dma_fence *fence = NULL;
	uint32_t i, shared_count;
	long ret;

	if (timeout == 0)
		return dma_resv_test_signaled_rcu(robj, shared);

top:	KASSERT(fence == NULL);

	/* Enter an RCU read section and get a read ticket.  */
	rcu_read_lock();
	dma_resv_read_begin(robj, &ticket);

	/* If shared is requested and there is a shared list, wait on it.  */
	if (shared) {
		if (!dma_resv_get_shared_reader(robj, &list, &shared_count,
			&ticket))
			goto restart;
	} else {
		list = NULL;
		shared_count = 0;
	}
	if (list != NULL) {
		/*
		 * For each fence, if it is going away, restart.
		 * Otherwise, acquire a reference to it to test whether
		 * it is signalled.  Stop and wait if we find any that
		 * is not signalled.
		 */
		for (i = 0; i < shared_count; i++) {
			KASSERT(fence == NULL);
			fence = atomic_load_relaxed(&list->shared[i]);
			if ((fence = dma_fence_get_rcu(fence)) == NULL)
				goto restart;
			if (!dma_fence_is_signaled(fence))
				goto wait;
			dma_fence_put(fence);
			fence = NULL;
		}

		/* If anything changed while we were testing, restart.  */
		if (!dma_resv_read_valid(robj, &ticket))
			goto restart;
	}
	if (shared_count)
		goto out;

	/* If there is an exclusive fence, test it.  */
	KASSERT(fence == NULL);
	if (!dma_resv_get_excl_reader(robj, &fence, &ticket))
		goto restart;
	if (fence != NULL) {
		/* Test whether it is signalled.  If no, wait.  */
		if (!dma_fence_is_signaled(fence))
			goto wait;
		dma_fence_put(fence);
		fence = NULL;
	}

out:	/* Success!  Return the number of ticks left.  */
	rcu_read_unlock();
	KASSERT(fence == NULL);
	return timeout;

restart:
	KASSERT(fence == NULL);
	rcu_read_unlock();
	goto top;

wait:
	/*
	 * Exit the RCU read section, wait for it, and release the
	 * fence when we're done.  If we time out or fail, bail.
	 * Otherwise, go back to the top.
	 */
	KASSERT(fence != NULL);
	rcu_read_unlock();
	ret = dma_fence_wait_timeout(fence, intr, timeout);
	dma_fence_put(fence);
	fence = NULL;
	if (ret <= 0)
		return ret;
	KASSERT(ret <= timeout);
	timeout = ret;
	goto top;
}

/*
 * dma_resv_poll_init(rpoll, lock)
 *
 *	Initialize reservation poll state.
 */
void
dma_resv_poll_init(struct dma_resv_poll *rpoll)
{

	mutex_init(&rpoll->rp_lock, MUTEX_DEFAULT, IPL_VM);
	selinit(&rpoll->rp_selq);
	rpoll->rp_claimed = 0;
}

/*
 * dma_resv_poll_fini(rpoll)
 *
 *	Release any resource associated with reservation poll state.
 */
void
dma_resv_poll_fini(struct dma_resv_poll *rpoll)
{

	KASSERT(rpoll->rp_claimed == 0);
	seldestroy(&rpoll->rp_selq);
	mutex_destroy(&rpoll->rp_lock);
}

/*
 * dma_resv_poll_cb(fence, fcb)
 *
 *	Callback to notify a reservation poll that a fence has
 *	completed.  Notify any waiters and allow the next poller to
 *	claim the callback.
 *
 *	If one thread is waiting for the exclusive fence only, and we
 *	spuriously notify them about a shared fence, tough.
 */
static void
dma_resv_poll_cb(struct dma_fence *fence, struct dma_fence_cb *fcb)
{
	struct dma_resv_poll *rpoll = container_of(fcb,
	    struct dma_resv_poll, rp_fcb);

	mutex_enter(&rpoll->rp_lock);
	selnotify(&rpoll->rp_selq, 0, NOTE_SUBMIT);
	rpoll->rp_claimed = 0;
	mutex_exit(&rpoll->rp_lock);
}

/*
 * dma_resv_do_poll(robj, events, rpoll)
 *
 *	Poll for reservation object events using the reservation poll
 *	state in rpoll:
 *
 *	- POLLOUT	wait for all fences shared and exclusive
 *	- POLLIN	wait for the exclusive fence
 *
 *	Return the subset of events in events that are ready.  If any
 *	are requested but not ready, arrange to be notified with
 *	selnotify when they are.
 */
int
dma_resv_do_poll(const struct dma_resv *robj, int events,
    struct dma_resv_poll *rpoll)
{
	struct dma_resv_read_ticket ticket;
	const struct dma_resv_list *list;
	struct dma_fence *fence = NULL;
	uint32_t i, shared_count;
	int revents;
	bool recorded = false;	/* curlwp is on the selq */
	bool claimed = false;	/* we claimed the callback */
	bool callback = false;	/* we requested a callback */

	/*
	 * Start with the maximal set of events that could be ready.
	 * We will eliminate the events that are definitely not ready
	 * as we go at the same time as we add callbacks to notify us
	 * that they may be ready.
	 */
	revents = events & (POLLIN|POLLOUT);
	if (revents == 0)
		return 0;

top:	KASSERT(fence == NULL);

	/* Enter an RCU read section and get a read ticket.  */
	rcu_read_lock();
	dma_resv_read_begin(robj, &ticket);

	/* If we want to wait for all fences, get the shared list.  */
	if (events & POLLOUT) {
		if (!dma_resv_get_shared_reader(robj, &list, &shared_count,
			&ticket))
			goto restart;
	} else {
		list = NULL;
		shared_count = 0;
	}
	if (list != NULL) do {
		/*
		 * For each fence, if it is going away, restart.
		 * Otherwise, acquire a reference to it to test whether
		 * it is signalled.  Stop and request a callback if we
		 * find any that is not signalled.
		 */
		for (i = 0; i < shared_count; i++) {
			KASSERT(fence == NULL);
			fence = atomic_load_relaxed(&list->shared[i]);
			if ((fence = dma_fence_get_rcu(fence)) == NULL)
				goto restart;
			if (!dma_fence_is_signaled(fence)) {
				dma_fence_put(fence);
				fence = NULL;
				break;
			}
			dma_fence_put(fence);
			fence = NULL;
		}

		/* If all shared fences have been signalled, move on.  */
		if (i == shared_count)
			break;

		/* Put ourselves on the selq if we haven't already.  */
		if (!recorded)
			goto record;

		/*
		 * If someone else claimed the callback, or we already
		 * requested it, we're guaranteed to be notified, so
		 * assume the event is not ready.
		 */
		if (!claimed || callback) {
			revents &= ~POLLOUT;
			break;
		}

		/*
		 * Otherwise, find the first fence that is not
		 * signalled, request the callback, and clear POLLOUT
		 * from the possible ready events.  If they are all
		 * signalled, leave POLLOUT set; we will simulate the
		 * callback later.
		 */
		for (i = 0; i < shared_count; i++) {
			KASSERT(fence == NULL);
			fence = atomic_load_relaxed(&list->shared[i]);
			if ((fence = dma_fence_get_rcu(fence)) == NULL)
				goto restart;
			if (!dma_fence_add_callback(fence, &rpoll->rp_fcb,
				dma_resv_poll_cb)) {
				dma_fence_put(fence);
				fence = NULL;
				revents &= ~POLLOUT;
				callback = true;
				break;
			}
			dma_fence_put(fence);
			fence = NULL;
		}
	} while (0);

	/* We always wait for at least the exclusive fence, so get it.  */
	KASSERT(fence == NULL);
	if (!dma_resv_get_excl_reader(robj, &fence, &ticket))
		goto restart;
	if (fence != NULL) do {
		/*
		 * Test whether it is signalled.  If not, stop and
		 * request a callback.
		 */
		if (dma_fence_is_signaled(fence))
			break;

		/* Put ourselves on the selq if we haven't already.  */
		if (!recorded) {
			dma_fence_put(fence);
			fence = NULL;
			goto record;
		}

		/*
		 * If someone else claimed the callback, or we already
		 * requested it, we're guaranteed to be notified, so
		 * assume the event is not ready.
		 */
		if (!claimed || callback) {
			revents = 0;
			break;
		}

		/*
		 * Otherwise, try to request the callback, and clear
		 * all possible ready events.  If the fence has been
		 * signalled in the interim, leave the events set; we
		 * will simulate the callback later.
		 */
		if (!dma_fence_add_callback(fence, &rpoll->rp_fcb,
			dma_resv_poll_cb)) {
			revents = 0;
			callback = true;
			break;
		}
	} while (0);
	if (fence != NULL) {
		dma_fence_put(fence);
		fence = NULL;
	}

	/* All done reading the fences.  */
	rcu_read_unlock();

	if (claimed && !callback) {
		/*
		 * We claimed the callback but we didn't actually
		 * request it because a fence was signalled while we
		 * were claiming it.  Call it ourselves now.  The
		 * callback doesn't use the fence nor rely on holding
		 * any of the fence locks, so this is safe.
		 */
		dma_resv_poll_cb(NULL, &rpoll->rp_fcb);
	}
	return revents;

restart:
	KASSERT(fence == NULL);
	rcu_read_unlock();
	goto top;

record:
	KASSERT(fence == NULL);
	rcu_read_unlock();
	mutex_enter(&rpoll->rp_lock);
	selrecord(curlwp, &rpoll->rp_selq);
	if (!rpoll->rp_claimed)
		claimed = rpoll->rp_claimed = true;
	mutex_exit(&rpoll->rp_lock);
	recorded = true;
	goto top;
}

/*
 * dma_resv_kqfilter(robj, kn, rpoll)
 *
 *	Kqueue filter for reservation objects.  Currently not
 *	implemented because the logic to implement it is nontrivial,
 *	and userland will presumably never use it, so it would be
 *	dangerous to add never-tested complex code paths to the kernel.
 */
int
dma_resv_kqfilter(const struct dma_resv *robj,
    struct knote *kn, struct dma_resv_poll *rpoll)
{

	return EINVAL;
}
