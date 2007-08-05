/*	$NetBSD: lock_stub.c,v 1.1.2.2 2007/08/05 22:28:09 pooka Exp $	*/

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

/* oh sweet crackmgr, what would I do without you? */
int
lockmgr(volatile struct lock *lock, u_int flags, struct simplelock *slock)
{

	return 0;
}

void
lockinit(struct lock *lock, pri_t prio, const char *wmesg, int timo,
	int flags)
{

	return;
}

int
lockstatus(struct lock *lock)
{

	return 0;
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
