/*	$NetBSD: kern_mutex_obj.c,v 1.12 2023/09/10 14:45:52 ad Exp $	*/

/*-
 * Copyright (c) 2008, 2019, 2023 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: kern_mutex_obj.c,v 1.12 2023/09/10 14:45:52 ad Exp $");

#include <sys/param.h>
#include <sys/atomic.h>
#include <sys/mutex.h>
#include <sys/kmem.h>

/* Mutex cache */
#define	MUTEX_OBJ_MAGIC	0x5aa3c85d
struct kmutexobj {
	kmutex_t	mo_lock;
	u_int		mo_magic;
	u_int		mo_refcnt;
	uint8_t		mo_pad[COHERENCY_UNIT - sizeof(kmutex_t) -
	    sizeof(u_int) * 2];
};

/*
 * mutex_obj_alloc:
 *
 *	Allocate a single lock object, waiting for memory if needed.
 */
kmutex_t *
mutex_obj_alloc(kmutex_type_t type, int ipl)
{
	struct kmutexobj *mo;

	mo = kmem_alloc(sizeof(*mo), KM_SLEEP);
	KASSERT(ALIGNED_POINTER(mo, coherency_unit));
	_mutex_init(&mo->mo_lock, type, ipl,
	    (uintptr_t)__builtin_return_address(0));
	mo->mo_magic = MUTEX_OBJ_MAGIC;
	mo->mo_refcnt = 1;

	return (kmutex_t *)mo;
}

/*
 * mutex_obj_alloc:
 *
 *	Allocate a single lock object, failing if no memory available.
 */
kmutex_t *
mutex_obj_tryalloc(kmutex_type_t type, int ipl)
{
	struct kmutexobj *mo;

	mo = kmem_alloc(sizeof(*mo), KM_NOSLEEP);
	KASSERT(ALIGNED_POINTER(mo, coherency_unit));
	if (__predict_true(mo != NULL)) {
		_mutex_init(&mo->mo_lock, type, ipl,
		    (uintptr_t)__builtin_return_address(0));
		mo->mo_magic = MUTEX_OBJ_MAGIC;
		mo->mo_refcnt = 1;
	}

	return (kmutex_t *)mo;
}

/*
 * mutex_obj_hold:
 *
 *	Add a single reference to a lock object.  A reference to the object
 *	must already be held, and must be held across this call.
 */
void
mutex_obj_hold(kmutex_t *lock)
{
	struct kmutexobj *mo = (struct kmutexobj *)lock;

	KASSERTMSG(mo->mo_magic == MUTEX_OBJ_MAGIC,
	    "%s: lock %p: mo->mo_magic (%#x) != MUTEX_OBJ_MAGIC (%#x)",
	     __func__, mo, mo->mo_magic, MUTEX_OBJ_MAGIC);
	KASSERTMSG(mo->mo_refcnt > 0,
	    "%s: lock %p: mo->mo_refcnt (%#x) == 0",
	     __func__, mo, mo->mo_refcnt);

	atomic_inc_uint(&mo->mo_refcnt);
}

/*
 * mutex_obj_free:
 *
 *	Drop a reference from a lock object.  If the last reference is being
 *	dropped, free the object and return true.  Otherwise, return false.
 */
bool
mutex_obj_free(kmutex_t *lock)
{
	struct kmutexobj *mo = (struct kmutexobj *)lock;

	KASSERTMSG(mo->mo_magic == MUTEX_OBJ_MAGIC,
	    "%s: lock %p: mo->mo_magic (%#x) != MUTEX_OBJ_MAGIC (%#x)",
	     __func__, mo, mo->mo_magic, MUTEX_OBJ_MAGIC);
	KASSERTMSG(mo->mo_refcnt > 0,
	    "%s: lock %p: mo->mo_refcnt (%#x) == 0",
	     __func__, mo, mo->mo_refcnt);

	membar_release();
	if (atomic_dec_uint_nv(&mo->mo_refcnt) > 0) {
		return false;
	}
	membar_acquire();
	mutex_destroy(&mo->mo_lock);
	kmem_free(mo, sizeof(*mo));
	return true;
}

/*
 * mutex_obj_refcnt:
 *
 *	Return the reference count on a lock object.
 */
u_int
mutex_obj_refcnt(kmutex_t *lock)
{
	struct kmutexobj *mo = (struct kmutexobj *)lock;

	return mo->mo_refcnt;
}
