/*	$NetBSD: fstrans_stub.c,v 1.3.2.1 2007/12/28 21:43:16 ad Exp $	*/

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

#include <sys/mount.h>
#include <sys/fstrans.h>

int
fstrans_setstate(struct mount *mp, enum fstrans_state new_state)
{

	return 0;
}

enum fstrans_state
fstrans_getstate(struct mount *mp)
{

	return FSTRANS_NORMAL;
}

int
_fstrans_start(struct mount *mp, enum fstrans_lock_type lock_type, int wait)
{

	return 0;
}

void
fstrans_done(struct mount *mp)
{

	return;
}

int
fstrans_is_owner(struct mount *mp)
{

	return 1;
}

int
fscow_establish(struct mount *mp, int (*func)(void *, struct buf *, bool),
    void *arg)
{

	return 0;
}

int
fscow_disestablish(struct mount *mp, int (*func)(void *, struct buf *, bool),
    void *arg)
{

	return 0;
}

int
fscow_run(struct buf *bp, bool data_valid)
{

	return 0;
}

int
fstrans_mount(struct mount *mp)
{

	return 0;
}

void
fstrans_unmount(struct mount *mp)
{

}
