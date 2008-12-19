/*	$NetBSD: vfsops_stub.c,v 1.3 2008/12/19 00:57:24 pooka Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vfsops_stub.c,v 1.3 2008/12/19 00:57:24 pooka Exp $");

#include <sys/param.h>
#include <sys/errno.h>
#include <sys/lockf.h>
#include <sys/systm.h>
#include <sys/vnode.h>

#include <miscfs/specfs/specdev.h>
#include <miscfs/fifofs/fifo.h>
#include <miscfs/syncfs/syncfs.h>

int (*mountroot)(void);

#define VFSSTUB(name)							\
    int name(void *arg) {panic("%s: unimplemented vfs stub", __func__);}

/* specfs */
VFSSTUB(spec_lookup)
VFSSTUB(spec_open)
VFSSTUB(spec_close)
VFSSTUB(spec_ioctl)
VFSSTUB(spec_poll)
VFSSTUB(spec_kqfilter)
VFSSTUB(spec_mmap)
VFSSTUB(spec_bmap)
VFSSTUB(spec_strategy)
VFSSTUB(spec_pathconf)
VFSSTUB(spec_advlock)
VFSSTUB(spec_read)
VFSSTUB(spec_write)

int
spec_fsync(void *v)
{

	return 0;
}

/* fifo oops */
int (**fifo_vnodeop_p)(void *);
VFSSTUB(fifo_lookup)
VFSSTUB(fifo_open)
VFSSTUB(fifo_close)
VFSSTUB(fifo_ioctl)
VFSSTUB(fifo_poll)
VFSSTUB(fifo_kqfilter)
VFSSTUB(fifo_pathconf)
VFSSTUB(fifo_bmap)
VFSSTUB(fifo_read)
VFSSTUB(fifo_write)

int
vfs_allocate_syncvnode(struct mount *mp)
{

	panic("%s: unimplemented", __func__);
}

void
vfs_deallocate_syncvnode(struct mount *mp)
{

	panic("%s: unimplemented", __func__);
}

int
lf_advlock(struct vop_advlock_args *ap, struct lockf **head, off_t size)
{

	panic("%s: unimplemented", __func__);
}

void
fifo_printinfo(struct vnode *vp)
{

	return;
}
