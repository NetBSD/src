/*	$NetBSD: lock_stub.c,v 1.8.4.3 2007/10/27 11:36:23 yamt Exp $	*/

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
#include <sys/mutex.h>
#include <sys/rwlock.h>

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

#define RW_UNLOCKED -1 /* XXX */

void
rw_init(krwlock_t *rw)
{

	rw->rw_locked = RW_UNLOCKED;
}

void
rw_destroy(krwlock_t *rw)
{

}

void
rw_enter(krwlock_t *rw, const krw_t op)
{

	KASSERT(rw->rw_locked == RW_UNLOCKED);
	rw->rw_locked = op;
}

void
rw_exit(krwlock_t *rw)
{

	KASSERT(rw->rw_locked != RW_UNLOCKED);
	rw->rw_locked = RW_UNLOCKED;
}

int
rw_lock_held(krwlock_t *rw)
{

	return rw->rw_locked != RW_UNLOCKED;
}

int
rw_tryupgrade(krwlock_t *rw)
{

	KASSERT(rw->rw_locked == RW_READER);
	rw->rw_locked = RW_WRITER;

	return 1;
}
