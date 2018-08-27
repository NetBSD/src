/*	$NetBSD: linux_reservation.c,v 1.3 2018/08/27 13:55:46 riastradh Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: linux_reservation.c,v 1.3 2018/08/27 13:55:46 riastradh Exp $");

#include <linux/fence.h>
#include <linux/reservation.h>

DEFINE_WW_CLASS(reservation_ww_class __cacheline_aligned);

static struct reservation_object_list *
objlist_tryalloc(uint32_t n)
{
	struct reservation_object_list *list;

	list = kmem_alloc(offsetof(typeof(*list), shared[n]), KM_NOSLEEP);
	if (list == NULL)
		return NULL;
	list->shared_max = n;

	return list;
}

static void
objlist_free(struct reservation_object_list *list)
{
	uint32_t n = list->shared_max;

	kmem_free(list, offsetof(typeof(*list), shared[n]));
}

static void
objlist_free_cb(struct rcu_head *rcu)
{
	struct reservation_object_list *list = container_of(rcu,
	    struct reservation_object_list, rol_rcu);

	objlist_free(list);
}

static void
objlist_defer_free(struct reservation_object_list *list)
{

	call_rcu(&list->rol_rcu, objlist_free_cb);
}

/*
 * reservation_object_init(robj)
 *
 *	Initialize a reservation object.  Caller must later destroy it
 *	with reservation_object_fini.
 */
void
reservation_object_init(struct reservation_object *robj)
{

	ww_mutex_init(&robj->lock, &reservation_ww_class);
	robj->robj_version = 0;
	robj->robj_fence = NULL;
	robj->robj_list = NULL;
	robj->robj_prealloc = NULL;
}

/*
 * reservation_object_fini(robj)
 *
 *	Destroy a reservation object, freeing any memory that had been
 *	allocated for it.  Caller must have exclusive access to it.
 */
void
reservation_object_fini(struct reservation_object *robj)
{
	unsigned i;

	if (robj->robj_prealloc)
		objlist_free(robj->robj_prealloc);
	if (robj->robj_list) {
		for (i = 0; i < robj->robj_list->shared_count; i++)
			fence_put(robj->robj_list->shared[i]);
		objlist_free(robj->robj_list);
	}
	if (robj->robj_fence)
		fence_put(robj->robj_fence);
	ww_mutex_destroy(&robj->lock);
}

/*
 * reservation_object_held(roj)
 *
 *	True if robj is locked.
 */
bool
reservation_object_held(struct reservation_object *robj)
{

	return ww_mutex_is_locked(&robj->lock);
}

/*
 * reservation_object_get_excl(robj)
 *
 *	Return a pointer to the exclusive fence of the reservation
 *	object robj.
 *
 *	Caller must have robj locked.
 */
struct fence *
reservation_object_get_excl(struct reservation_object *robj)
{

	KASSERT(reservation_object_held(robj));
	return robj->robj_fence;
}

/*
 * reservation_object_get_list(robj)
 *
 *	Return a pointer to the shared fence list of the reservation
 *	object robj.
 *
 *	Caller must have robj locked.
 */
struct reservation_object_list *
reservation_object_get_list(struct reservation_object *robj)
{

	KASSERT(reservation_object_held(robj));
	return robj->robj_list;
}

/*
 * reservation_object_reserve_shared(robj)
 *
 *	Reserve space in robj to add a shared fence.  To be used only
 *	once before calling reservation_object_add_shared_fence.
 *
 *	Caller must have robj locked.
 *
 *	Internally, we start with room for four entries and double if
 *	we don't have enough.  This is not guaranteed.
 */
int
reservation_object_reserve_shared(struct reservation_object *robj)
{
	struct reservation_object_list *list, *prealloc;
	uint32_t n, nalloc;

	KASSERT(reservation_object_held(robj));

	list = robj->robj_list;
	prealloc = robj->robj_prealloc;

	/* If there's an existing list, check it for space.  */
	if (list != NULL) {
		/* If there's too many already, give up.  */
		if (list->shared_count == UINT32_MAX)
			return -ENOMEM;

		/* Add one more. */
		n = list->shared_count + 1;

		/* If there's enough for one more, we're done.  */
		if (n <= list->shared_max)
			return 0;
	} else {
		/* No list already.  We need space for 1.  */
		n = 1;
	}

	/* If not, maybe there's a preallocated list ready.  */
	if (prealloc != NULL) {
		/* If there's enough room in it, stop here.  */
		if (n <= prealloc->shared_max)
			return 0;

		/* Try to double its capacity.  */
		nalloc = n > UINT32_MAX/2 ? UINT32_MAX : 2*n;
		prealloc = objlist_tryalloc(nalloc);
		if (prealloc == NULL)
			return -ENOMEM;

		/* Swap the new preallocated list and free the old one.  */
		objlist_free(robj->robj_prealloc);
		robj->robj_prealloc = prealloc;
	} else {
		/* Start with some spare.  */
		nalloc = n > UINT32_MAX/2 ? UINT32_MAX : MAX(2*n, 4);
		prealloc = objlist_tryalloc(nalloc);
		if (prealloc == NULL)
			return -ENOMEM;
		/* Save the new preallocated list.  */
		robj->robj_prealloc = prealloc;
	}

	/* Success!  */
	return 0;
}

struct reservation_object_write_ticket {
	unsigned version;
};

/*
 * reservation_object_write_begin(robj, ticket)
 *
 *	Begin an atomic batch of writes to robj, and initialize opaque
 *	ticket for it.  The ticket must be passed to
 *	reservation_object_write_commit to commit the writes.
 *
 *	Caller must have robj locked.
 */
static void
reservation_object_write_begin(struct reservation_object *robj,
    struct reservation_object_write_ticket *ticket)
{

	KASSERT(reservation_object_held(robj));

	ticket->version = robj->robj_version |= 1;
	membar_producer();
}

/*
 * reservation_object_write_commit(robj, ticket)
 *
 *	Commit an atomic batch of writes to robj begun with the call to
 *	reservation_object_write_begin that returned ticket.
 *
 *	Caller must have robj locked.
 */
static void
reservation_object_write_commit(struct reservation_object *robj,
    struct reservation_object_write_ticket *ticket)
{

	KASSERT(reservation_object_held(robj));
	KASSERT(ticket->version == robj->robj_version);
	KASSERT((ticket->version & 1) == 1);

	membar_producer();
	robj->robj_version = ticket->version + 1;
}

struct reservation_object_read_ticket {
	unsigned version;
};

/*
 * reservation_object_read_begin(robj, ticket)
 *
 *	Begin a read section, and initialize opaque ticket for it.  The
 *	ticket must be passed to reservation_object_read_exit, and the
 *	caller must be prepared to retry reading if it fails.
 */
static void
reservation_object_read_begin(struct reservation_object *robj,
    struct reservation_object_read_ticket *ticket)
{

	while ((ticket->version = robj->robj_version) & 1)
		SPINLOCK_BACKOFF_HOOK;
	membar_consumer();
}

/*
 * reservation_object_read_valid(robj, ticket)
 *
 *	Test whether the read sections are valid.  Return true on
 *	success, or false on failure if the read ticket has been
 *	invalidated.
 */
static bool
reservation_object_read_valid(struct reservation_object *robj,
    struct reservation_object_read_ticket *ticket)
{

	membar_consumer();
	return ticket->version == robj->robj_version;
}

/*
 * reservation_object_add_excl_fence(robj, fence)
 *
 *	Replace robj's exclusive fence, if any, by fence, and empty its
 *	list of shared fences.  The old exclusive fence and all old
 *	shared fences are released.
 *
 *	Caller must have robj locked.
 */
void
reservation_object_add_excl_fence(struct reservation_object *robj,
    struct fence *fence)
{
	struct fence *old_fence = robj->robj_fence;
	struct reservation_object_list *old_list = robj->robj_list;
	uint32_t old_shared_count;
	struct reservation_object_write_ticket ticket;

	KASSERT(reservation_object_held(robj));

	/* If there are any shared fences, remember how many.  */
	if (old_list)
		old_shared_count = old_list->shared_count;

	/* Begin an update.  */
	reservation_object_write_begin(robj, &ticket);

	/* Replace the fence and zero the shared count.  */
	robj->robj_fence = fence;
	if (old_list)
		old_list->shared_count = 0;

	/* Commit the update.  */
	reservation_object_write_commit(robj, &ticket);

	/* Release the old exclusive fence, if any.  */
	if (old_fence)
		fence_put(old_fence);

	/* Release any old shared fences.  */
	if (old_list) {
		while (old_shared_count--)
			fence_put(old_list->shared[old_shared_count]);
	}
}

/*
 * reservation_object_add_shared_fence(robj, fence)
 *
 *	Add a shared fence to robj.  If any fence was already added
 *	with the same context number, release it and replace it by this
 *	one.
 *
 *	Caller must have robj locked, and must have preceded with a
 *	call to reservation_object_reserve_shared for each shared fence
 *	added.
 */
void
reservation_object_add_shared_fence(struct reservation_object *robj,
    struct fence *fence)
{
	struct reservation_object_list *list = robj->robj_list;
	struct reservation_object_list *prealloc = robj->robj_prealloc;
	struct reservation_object_write_ticket ticket;
	struct fence *replace = NULL;
	uint32_t i;

	KASSERT(reservation_object_held(robj));

	/* Check for a preallocated replacement list.  */
	if (prealloc == NULL) {
		KASSERT(list->shared_count < list->shared_max);

		/* Begin an update.  */
		reservation_object_write_begin(robj, &ticket);

		/* Find a fence with the same context number.  */
		for (i = 0; i < list->shared_count; i++) {
			if (list->shared[i]->context == fence->context) {
				replace = list->shared[i];
				list->shared[i] = fence;
				break;
			}
		}

		/* If we didn't find one, add it at the end.  */
		if (i == list->shared_count)
			list->shared[list->shared_count++] = fence;

		/* Commit the update.  */
		reservation_object_write_commit(robj, &ticket);
	} else {
		KASSERT(list->shared_count < prealloc->shared_max);

		/*
		 * Copy the fences over, but replace if we find one
		 * with the same context number.
		 */
		for (i = 0; i < list->shared_count; i++) {
			if (replace == NULL &&
			    list->shared[i]->context == fence->context) {
				replace = list->shared[i];
				prealloc->shared[i] = fence;
			} else {
				prealloc->shared[i] = list->shared[i];
			}
		}
		prealloc->shared_count = list->shared_count;

		/* If we didn't find one, add it at the end.  */
		if (replace == NULL)
			prealloc->shared[prealloc->shared_count++] = fence;

		/* Now ready to replace the list.  Begin an update.  */
		reservation_object_write_begin(robj, &ticket);

		/* Replace the list.  */
		robj->robj_list = prealloc;
		robj->robj_prealloc = NULL;

		/* Commit the update.  */
		reservation_object_write_commit(robj, &ticket);

		/*
		 * Free the old list when it is convenient.  (We are
		 * not in a position at this point to sleep waiting for
		 * activity on all CPUs.)
		 */
		objlist_defer_free(list);
	}

	/* Release a fence if we replaced it.  */
	if (replace)
		fence_put(replace);
}

/*
 * reservation_object_test_signaled_rcu(robj, shared)
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
reservation_object_test_signaled_rcu(struct reservation_object *robj,
    bool shared)
{
	struct reservation_object_read_ticket ticket;
	struct reservation_object_list *list;
	struct fence *fence;
	uint32_t i, shared_count;
	bool signaled = true;

top:
	/* Enter an RCU read section and get a read ticket.  */
	rcu_read_lock();
	reservation_object_read_begin(robj, &ticket);

	/* If shared is requested and there is a shared list, test it.  */
	if (shared && (list = robj->robj_list) != NULL) {
		/* Make sure the content of the list has been published.  */
		membar_datadep_consumer();

		/* Find out how long it is.  */
		shared_count = list->shared_count;

		/*
		 * Make sure we saw a consistent snapshot of the list
		 * pointer and length.
		 */
		if (!reservation_object_read_valid(robj, &ticket))
			goto restart;

		/*
		 * For each fence, if it is going away, restart.
		 * Otherwise, acquire a reference to it to test whether
		 * it is signalled.  Stop if we find any that is not
		 * signalled.
		 */
		for (i = 0; i < shared_count; i++) {
			fence = fence_get_rcu(list->shared[i]);
			if (fence == NULL)
				goto restart;
			signaled &= fence_is_signaled(fence);
			fence_put(fence);
			if (!signaled)
				goto out;
		}
	}

	/* If there is an exclusive fence, test it.  */
	if ((fence = robj->robj_fence) != NULL) {
		/* Make sure the content of the fence has been published.  */
		membar_datadep_consumer();

		/*
		 * Make sure we saw a consistent snapshot of the fence.
		 *
		 * XXX I'm not actually sure this is necessary since
		 * pointer writes are supposed to be atomic.
		 */
		if (!reservation_object_read_valid(robj, &ticket))
			goto restart;

		/*
		 * If it is going away, restart.  Otherwise, acquire a
		 * reference to it to test whether it is signalled.
		 */
		if ((fence = fence_get_rcu(fence)) == NULL)
			goto restart;
		signaled &= fence_is_signaled(fence);
		fence_put(fence);
		if (!signaled)
			goto out;
	}

out:	rcu_read_unlock();
	return signaled;

restart:
	rcu_read_unlock();
	goto top;
}

/*
 * reservation_object_wait_timeout_rcu(robj, shared, intr, timeout)
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
reservation_object_wait_timeout_rcu(struct reservation_object *robj,
    bool shared, bool intr, unsigned long timeout)
{
	struct reservation_object_read_ticket ticket;
	struct reservation_object_list *list;
	struct fence *fence;
	uint32_t i, shared_count;

	if (timeout == 0)
		return reservation_object_test_signaled_rcu(robj, shared);

top:
	/* Enter an RCU read section and get a read ticket.  */
	rcu_read_lock();
	reservation_object_read_begin(robj, &ticket);

	/* If shared is requested and there is a shared list, wait on it.  */
	if (shared && (list = robj->robj_list) != NULL) {
		/* Make sure the content of the list has been published.  */
		membar_datadep_consumer();

		/* Find out how long it is.  */
		shared_count = list->shared_count;

		/*
		 * Make sure we saw a consistent snapshot of the list
		 * pointer and length.
		 */
		if (!reservation_object_read_valid(robj, &ticket))
			goto restart;

		/*
		 * For each fence, if it is going away, restart.
		 * Otherwise, acquire a reference to it to test whether
		 * it is signalled.  Stop and wait if we find any that
		 * is not signalled.
		 */
		for (i = 0; i < shared_count; i++) {
			fence = fence_get_rcu(list->shared[i]);
			if (fence == NULL)
				goto restart;
			if (!fence_is_signaled(fence))
				goto wait;
			fence_put(fence);
		}
	}

	/* If there is an exclusive fence, test it.  */
	if ((fence = robj->robj_fence) != NULL) {
		/* Make sure the content of the fence has been published.  */
		membar_datadep_consumer();

		/*
		 * Make sure we saw a consistent snapshot of the fence.
		 *
		 * XXX I'm not actually sure this is necessary since
		 * pointer writes are supposed to be atomic.
		 */
		if (!reservation_object_read_valid(robj, &ticket))
			goto restart;

		/*
		 * If it is going away, restart.  Otherwise, acquire a
		 * reference to it to test whether it is signalled.  If
		 * not, wait for it.
		 */
		if ((fence = fence_get_rcu(fence)) == NULL)
			goto restart;
		if (!fence_is_signaled(fence))
			goto wait;
		fence_put(fence);
	}

	/* Success!  Return the number of ticks left.  */
	rcu_read_unlock();
	return timeout;

restart:
	rcu_read_unlock();
	goto top;

wait:
	/*
	 * Exit the RCU read section and wait for it.  If we time out
	 * or fail, bail.  Otherwise, go back to the top.
	 */
	KASSERT(fence != NULL);
	rcu_read_unlock();
	timeout = fence_wait_timeout(fence, intr, timeout);
	if (timeout <= 0)
		return timeout;
	goto top;
}
