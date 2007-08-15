/*	$NetBSD: lock_stub.c,v 1.5.2.2 2007/08/15 13:50:38 skrll Exp $	*/

/*
 * Copyright (c) 2007 Antti Kantee.  All Rights Reserved.
 *
 * Development of this software was supported by Google Summer of Code.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/lock.h>
#include <sys/mutex.h>
#include <sys/rwlock.h>

/* oh sweet crackmgr, what would I do without you? */
int
lockmgr(volatile struct lock *lock, u_int flags, struct simplelock *slock)
{
	u_int lktype = flags & LK_TYPE_MASK;

	switch (lktype) {
	case LK_SHARED:
	case LK_EXCLUSIVE:
		lock->lk_flags = lktype;
		break;

	case LK_RELEASE:
		lock->lk_flags = 0;
		break;

	case LK_UPGRADE:
	case LK_EXCLUPGRADE:
		assert(lock->lk_flags == LK_SHARED);
		lock->lk_flags = LK_EXCLUSIVE;
		break;

	case LK_DOWNGRADE:
		assert(lock->lk_flags == LK_EXCLUSIVE);
		lock->lk_flags = LK_SHARED;
		break;

	case LK_DRAIN:
		lock->lk_flags = LK_EXCLUSIVE;
		break;
	}

	return 0;
}

void
lockinit(struct lock *lock, pri_t prio, const char *wmesg, int timo,
	int flags)
{

	lock->lk_flags = 0;
}

int
lockstatus(struct lock *lock)
{

	return lock->lk_flags;
}

void
lockmgr_printinfo(volatile struct lock *lock)
{

	return;
}

void
transferlockers(struct lock *from, struct lock *to)
{

	return;
}

void
mutex_init(kmutex_t *mtx, kmutex_type_t type, int ipl)
{

	return;
}

void
mutex_destroy(kmutex_t *mtx)
{

	return;
}

void
mutex_enter(kmutex_t *mtx)
{

	return;
}

void
mutex_exit(kmutex_t *mtx)
{

	return;
}

int
mutex_owned(kmutex_t *mtx)
{

	return 1;
}

void
rw_init(krwlock_t *rw)
{

}

void
rw_destroy(krwlock_t *rw)
{

}

void
rw_enter(krwlock_t *rw, const krw_t op)
{

}

void
rw_exit(krwlock_t *rw)
{

}
