/*	$NetBSD: procfs_fd.c,v 1.6.12.1 2006/05/24 15:50:43 tron Exp $	*/

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: procfs_fd.c,v 1.6.12.1 2006/05/24 15:50:43 tron Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <miscfs/procfs/procfs.h>

int
procfs_dofd(curl, p, pfs, uio)
	struct lwp *curl;
	struct proc *p;
	struct pfsnode *pfs;
	struct uio *uio;
{
	struct proc *curp = curl->l_proc;
	int error;
	struct file *fp;
	struct proc *pown;
	off_t offs;

	if ((error = procfs_getfp(pfs, &pown, &fp)) != 0)
		return error;

	FILE_USE(fp);

	offs = fp->f_offset;

	switch (uio->uio_rw) {
	case UIO_READ:
		error = (*fp->f_ops->fo_read)(fp, &offs, uio, curp->p_cred, 0);
		break;
	case UIO_WRITE:
		error = (*fp->f_ops->fo_write)(fp, &offs, uio, curp->p_cred,0);
		break;
	default:
		panic("bad uio op");
	}

	FILE_UNUSE(fp, proc_representative_lwp(pown));

	return (error);
}
