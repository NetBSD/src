/*	$NetBSD: kobj.c,v 1.1 2009/08/07 20:57:57 haad Exp $	*/

/*-
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
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

/*-
 * Copyright (c) 2007 Pawel Jakub Dawidek <pjd@FreeBSD.org>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
/* __FBSDID("$FreeBSD: src/sys/compat/opensolaris/kern/opensolaris_kobj.c,v 1.4 2007/05/31 11:51:49 kib Exp $"); */
__KERNEL_RCSID(0, "$NetBSD: kobj.c,v 1.1 2009/08/07 20:57:57 haad Exp $");

#include <sys/types.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/kthread.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/filedesc.h>
#include <sys/fcntl.h>
#include <sys/kobj.h>
#include <sys/namei.h>

void
kobj_free(void *address, size_t size)
{

	kmem_free(address, size);
}

void *
kobj_alloc(size_t size, int flag)
{

	return (kmem_alloc(size, (flag & KM_NOWAIT) ? KM_NOSLEEP : KM_SLEEP));
}

void *
kobj_zalloc(size_t size, int flag)
{

	return (kmem_zalloc(size, (flag & KM_NOWAIT) ? KM_NOSLEEP : KM_SLEEP));
}

static void *
kobj_open_file_vnode(const char *file)
{
	vnode_t *vp;

	if (vn_open(file, UIO_SYSSPACE, 0, 0, &vp, CRCREAT, 0) != 0) {
		return NULL;
	}
	return vp;
}

struct _buf *
kobj_open_file(const char *file)
{
	struct _buf *out;

	out = kmem_alloc(sizeof(*out), KM_SLEEP);
	out->mounted = 1;
	out->ptr = kobj_open_file_vnode(file);
	if (out->ptr == NULL) {
		kmem_free(out, sizeof(*out));
		return ((struct _buf *)-1);
	}
	return (out);
}

static int
kobj_get_filesize_vnode(struct _buf *file, uint64_t *size)
{
	struct vattr va;
	int error;

	error = VOP_GETATTR(file->ptr, &va, 0, kauth_cred_get(), NULL);
	if (error == 0)
		*size = (uint64_t)va.va_size;
	return (error);
}

int
kobj_get_filesize(struct _buf *file, uint64_t *size)
{

	return (kobj_get_filesize_vnode(file, size));
}

int
kobj_read_file_vnode(struct _buf *file, char *buf, unsigned size, unsigned off)
{
	int error;
	size_t resid;

	error = vn_rdwr(UIO_READ, file->ptr, buf, size, off, UIO_SYSSPACE, 0,
	    RLIM64_INFINITY, kauth_cred_get(), &resid);
	return (error != 0 ? -1 : size - resid);
}

int
kobj_read_file(struct _buf *file, char *buf, unsigned size, unsigned off)
{

	return (kobj_read_file_vnode(file, buf, size, off));
}

void
kobj_close_file(struct _buf *file)
{

	if (file->mounted) {
		vn_close(file->ptr, FREAD, kauth_cred_get());
	}
	kmem_free(file, sizeof(*file));
}
