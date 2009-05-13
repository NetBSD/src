/*	$NetBSD: vfsops_stub.c,v 1.6.6.1 2009/05/13 17:22:58 jym Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: vfsops_stub.c,v 1.6.6.1 2009/05/13 17:22:58 jym Exp $");

#include <sys/param.h>
#include <sys/errno.h>
#include <sys/lockf.h>
#include <sys/systm.h>
#include <sys/vnode.h>

#include <miscfs/fifofs/fifo.h>
#include <miscfs/syncfs/syncfs.h>

const char *rootfstype;

#define VFSSTUB(name)							\
    int name(void *arg) {panic("%s: unimplemented vfs stub", __func__);}

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

void
fifo_printinfo(struct vnode *vp)
{

	return;
}
