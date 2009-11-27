/*	$NetBSD: subr_kobj_vfs.c,v 1.1 2009/11/27 17:54:11 pooka Exp $	*/

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software developed for The NetBSD Foundation
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
 * Copyright (c) 1998-2000 Doug Rabson
 * Copyright (c) 2004 Peter Wemm
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Kernel loader vfs routines.
 */

#include <sys/kobj_impl.h>
#include "opt_modular.h"

#ifdef MODULAR

#include <sys/param.h>
#include <sys/fcntl.h>
#include <sys/module.h>
#include <sys/namei.h>
#include <sys/vnode.h>

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: subr_kobj_vfs.c,v 1.1 2009/11/27 17:54:11 pooka Exp $");

static void
kobj_close_file(kobj_t ko)
{

	VOP_UNLOCK(ko->ko_source, 0);
	vn_close(ko->ko_source, FREAD, kauth_cred_get());
}

/*
 * kobj_read:
 *
 *	Utility function: read from the object.
 */
static int
kobj_read_file(kobj_t ko, void **basep, size_t size, off_t off,
	bool allocate)
{
	size_t resid;
	void *base;
	int error;

	KASSERT(ko->ko_source != NULL);

	if (allocate) {
		base = kmem_alloc(size, KM_SLEEP);
	} else {
		base = *basep;
		KASSERT((uintptr_t)base >= (uintptr_t)ko->ko_address);
		KASSERT((uintptr_t)base + size <=
		    (uintptr_t)ko->ko_address + ko->ko_size);
	}

	error = vn_rdwr(UIO_READ, ko->ko_source, base, size, off,
	    UIO_SYSSPACE, IO_NODELOCKED, curlwp->l_cred, &resid,
	    curlwp);

	if (error == 0 && resid != 0) {
		error = EINVAL;
	}

	if (allocate && error != 0) {
		kmem_free(base, size);
		base = NULL;
	}

	if (allocate)
		*basep = base;

	return error;
}

/*
 * kobj_load_file:
 *
 *	Load an object located in the file system.
 */
int
kobj_load_file(kobj_t *kop, const char *path, const bool nochroot)
{
	struct nameidata nd;
	kauth_cred_t cred;
	int error;
	kobj_t ko;

	cred = kauth_cred_get();

	ko = kmem_zalloc(sizeof(*ko), KM_SLEEP);
	if (ko == NULL) {
		return ENOMEM;
	}

	NDINIT(&nd, LOOKUP, FOLLOW | (nochroot ? NOCHROOT : 0),
	    UIO_SYSSPACE, path);
	error = vn_open(&nd, FREAD, 0);

 	if (error != 0) {
	 	kmem_free(ko, sizeof(*ko));
	 	return error;
	}

	ko->ko_type = KT_VNODE;
	ko->ko_source = nd.ni_vp;
	ko->ko_read = kobj_read_file;
	ko->ko_close = kobj_close_file;

	*kop = ko;
	return kobj_load(ko);
}

#else /* MODULAR */

int
kobj_load_file(kobj_t *kop, const char *path, const bool nochroot)
{

	return ENOSYS;
}

#endif
