/*	$NetBSD: procfs_auxv.c,v 1.2.18.2 2017/12/03 11:38:48 jdolecek Exp $	*/

/*-
 * Copyright (c) 2017 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
__KERNEL_RCSID(0, "$NetBSD: procfs_auxv.c,v 1.2.18.2 2017/12/03 11:38:48 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/kmem.h>

#include <miscfs/procfs/procfs.h>

int
procfs_doauxv(struct lwp *curl, struct proc *p, struct pfsnode *pfs,
     struct uio *uio)
{
	int error;
	void *buffer;
	size_t bufsize;

	if (uio->uio_rw != UIO_READ)
		return EOPNOTSUPP;

	if ((error = proc_getauxv(p, &buffer, &bufsize)) != 0)
		return error;

	if (uio->uio_offset < bufsize)
		error = uiomove((char *)buffer + uio->uio_offset,
		    bufsize - uio->uio_offset, uio);
	else
		error = 0;

	kmem_free(buffer, bufsize);
	return error;
}

int
procfs_validauxv(struct lwp *l, struct mount *mp)
{
	return l != NULL && l->l_proc != NULL && l->l_proc->p_execsw != NULL;
}
