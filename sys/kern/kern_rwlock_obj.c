/*	$NetBSD: kern_rwlock_obj.c,v 1.13 2023/10/02 21:03:55 ad Exp $	*/

/*-
 * Copyright (c) 2008, 2009, 2019, 2023 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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
__KERNEL_RCSID(0, "$NetBSD: kern_rwlock_obj.c,v 1.13 2023/10/02 21:03:55 ad Exp $");

#include <sys/param.h>
#include <sys/atomic.h>
#include <sys/kmem.h>
#include <sys/rwlock.h>

/* Mutex cache */
#define	RW_OBJ_MAGIC	0x85d3c85d
struct krwobj {
	krwlock_t	ro_lock;
	u_int		ro_magic;
	u_int		ro_refcnt;
	uint8_t		mo_pad[COHERENCY_UNIT - sizeof(krwlock_t) -
	    sizeof(u_int) * 2];
};

/*
 * rw_obj_alloc:
 *
 *	Allocate a single lock object, waiting for memory if needed.
 */
krwlock_t *
rw_obj_alloc(void)
{
	struct krwobj *ro;

	ro = kmem_intr_alloc(sizeof(*ro), KM_SLEEP);
	KASSERT(ALIGNED_POINTER(ro, coherency_unit));
	_rw_init(&ro->ro_lock, (uintptr_t)__builtin_return_address(0));
	ro->ro_magic = RW_OBJ_MAGIC;
	ro->ro_refcnt = 1;

	return (krwlock_t *)ro;
}

/*
 * rw_obj_tryalloc:
 *
 *	Allocate a single lock object, but fail if no memory is available.
 */
krwlock_t *
rw_obj_tryalloc(void)
{
	struct krwobj *ro;

	ro = kmem_intr_alloc(sizeof(*ro), KM_NOSLEEP);
	KASSERT(ALIGNED_POINTER(ro, coherency_unit));
	if (__predict_true(ro != NULL)) {
		_rw_init(&ro->ro_lock, (uintptr_t)__builtin_return_address(0));
		ro->ro_magic = RW_OBJ_MAGIC;
		ro->ro_refcnt = 1;
	}

	return (krwlock_t *)ro;
}

/*
 * rw_obj_hold:
 *
 *	Add a single reference to a lock object.  A reference to the object
 *	must already be held, and must be held across this call.
 */
void
rw_obj_hold(krwlock_t *lock)
{
	struct krwobj *ro = (struct krwobj *)lock;

	KASSERT(ro->ro_magic == RW_OBJ_MAGIC);
	KASSERT(ro->ro_refcnt > 0);

	atomic_inc_uint(&ro->ro_refcnt);
}

/*
 * rw_obj_free:
 *
 *	Drop a reference from a lock object.  If the last reference is being
 *	dropped, free the object and return true.  Otherwise, return false.
 */
bool
rw_obj_free(krwlock_t *lock)
{
	struct krwobj *ro = (struct krwobj *)lock;

	KASSERT(ro->ro_magic == RW_OBJ_MAGIC);
	KASSERT(ro->ro_refcnt > 0);

	membar_release();
	if (atomic_dec_uint_nv(&ro->ro_refcnt) > 0) {
		return false;
	}
	membar_acquire();
	rw_destroy(&ro->ro_lock);
	kmem_intr_free(ro, sizeof(*ro));
	return true;
}

/*
 * rw_obj_refcnt:
 *
 *	Return the reference count for a lock object.
 */
u_int
rw_obj_refcnt(krwlock_t *lock)
{
	struct krwobj *ro = (struct krwobj *)lock;

	return ro->ro_refcnt;
}
