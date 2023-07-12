/*	$NetBSD: db_syncobj.c,v 1.2 2023/07/12 12:50:46 riastradh Exp $	*/

/*-
 * Copyright (c) 2023 The NetBSD Foundation, Inc.
 * All rights reserved.
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

#define	__MUTEX_PRIVATE
#define	__RWLOCK_PRIVATE

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: db_syncobj.c,v 1.2 2023/07/12 12:50:46 riastradh Exp $");

#include <sys/types.h>

#include <sys/mutex.h>
#include <sys/null.h>
#include <sys/rwlock.h>
#include <sys/syncobj.h>

#include <ddb/ddb.h>

struct lwp *
db_syncobj_owner(struct syncobj *sobj, wchan_t wchan)
{
	db_expr_t mutex_syncobj_;
	db_expr_t rw_syncobj_;

	if (db_value_of_name("mutex_syncobj", &mutex_syncobj_) &&
	    (db_expr_t)(uintptr_t)sobj == mutex_syncobj_) {
		volatile const struct kmutex *mtx = wchan;
		uintptr_t owner;

		db_read_bytes((db_addr_t)&mtx->mtx_owner, sizeof(owner),
		    (char *)&owner);
		return (struct lwp *)(owner & MUTEX_THREAD);

	} else if (db_value_of_name("rw_syncobj", &rw_syncobj_) &&
	    (db_expr_t)(uintptr_t)sobj == rw_syncobj_) {
		volatile const struct krwlock *rw = wchan;
		uintptr_t owner;

		db_read_bytes((db_addr_t)&rw->rw_owner, sizeof(owner),
		    (char *)&owner);
		if (owner & RW_WRITE_LOCKED)
			return (struct lwp *)(owner & RW_THREAD);
		return NULL;

	} else {
		return NULL;
	}
}
