/*	$NetBSD: procfs_cmdline.c,v 1.4 1999/03/24 05:51:27 mrg Exp $	*/

/*
 * Copyright (c) 1999 Jaromir Dolecek <dolecek@ics.muni.cz>
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jaromir Dolecek.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

#include <sys/param.h>
#include <sys/types.h>
#include <sys/systm.h>
#include <sys/syslimits.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/exec.h>
#include <sys/malloc.h>
#include <miscfs/procfs/procfs.h>

#include <vm/vm.h>

#include <uvm/uvm_extern.h>

/*
 * code for returning process's command line arguments
 */
int
procfs_docmdline(curp, p, pfs, uio)
	struct proc *curp;
	struct proc *p;
	struct pfsnode *pfs;
	struct uio *uio;
{
	struct ps_strings pss;
	int xlen, len, count, error;
	struct uio auio;
	struct iovec aiov;
	vaddr_t argv;
	char *arg;

	/* Don't allow writing. */
	if (uio->uio_rw != UIO_READ)
		return (EOPNOTSUPP);

	/*
	 * Allocate a temporary buffer to hold the arguments.
	 *
	 * XXX THIS COULD BE HANDLED MUCH MORE INTELLIGENTLY,
	 * XXX WITHOUT REQUIRING A 256k TEMPORARY BUFFER!
	 */
	arg = malloc(ARG_MAX, M_TEMP, M_WAITOK);

	/*
	 * Zombies don't have a stack, so we can't read their psstrings.
	 * System processes also don't have a user stack.  This is what
	 * ps(1) would display.
	 */
	if (p->p_stat == SZOMB || (p->p_flag & P_SYSTEM) != 0) {
		snprintf(arg, sizeof(arg), "(%s)", p->p_comm);
		len = strlen(arg);	/* exclude last NUL */
		goto doio;
	}

	/*
	 * NOTE: Don't bother doing a procfs_checkioperm() here
	 * because the psstrings info is available by using ps(1),
	 * so it's not like there's anything to protect here.
	 */

	/*
	 * Lock the process down in memory.
	 */
	/* XXXCDC: how should locking work here? */
	if ((p->p_flag & P_WEXIT) || (p->p_vmspace->vm_refcnt < 1))
		return (EFAULT);
	PHOLD(p);
	p->p_vmspace->vm_refcnt++;	/* XXX */

	/*
	 * Read in the ps_strings structure.
	 */
	aiov.iov_base = &pss;
	aiov.iov_len = sizeof(pss);
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_offset = (vaddr_t)PS_STRINGS;
	auio.uio_resid = sizeof(pss);
	auio.uio_segflg = UIO_SYSSPACE;
	auio.uio_rw = UIO_READ;
	auio.uio_procp = NULL;
	error = uvm_io(&p->p_vmspace->vm_map, &auio);
	if (error)
		goto bad;

	/*
	 * Now read the address of the argument vector.
	 */
	aiov.iov_base = &argv;
	aiov.iov_len = sizeof(argv);
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_offset = (vaddr_t)pss.ps_argvstr;
	auio.uio_resid = sizeof(argv);
	auio.uio_segflg = UIO_SYSSPACE;
	auio.uio_rw = UIO_READ; 
	auio.uio_procp = NULL;
	error = uvm_io(&p->p_vmspace->vm_map, &auio);
	if (error)
		goto bad;

	/*
	 * Now copy in the actual argument vector, one byte at a time,
	 * since we don't know how long the vector is (though, we do
	 * know how many NUL-terminated strings are in the vector).
	 */
	len = 0;
	count = pss.ps_nargvstr;
	while (count && len < ARG_MAX) {
		aiov.iov_base = &arg[len];
		aiov.iov_len = 1;
		auio.uio_iov = &aiov;
		auio.uio_iovcnt = 1;
		auio.uio_offset = argv + len;
		auio.uio_resid = 1;
		auio.uio_segflg = UIO_SYSSPACE;
		auio.uio_rw = UIO_READ;
		auio.uio_procp = NULL;
		error = uvm_io(&p->p_vmspace->vm_map, &auio);
		if (error)
			goto bad;

		if (len > 0 && arg[len] == '\0')
			count--;	/* one full string */
		len++;
	}
	if (len > 0)
		len--;			/* exclude last NUL */

	/*
	 * Release the process.
	 */
	PRELE(p);
	uvmspace_free(p->p_vmspace);

 doio:
	xlen = len - uio->uio_offset;
	xlen = imin(xlen, uio->uio_resid);
	if (xlen <= 0) 
		error = 0;
	else
		error = uiomove(arg + uio->uio_offset, xlen, uio);

	free(arg, M_TEMP);
	return (error);

 bad:
	PRELE(p);
	uvmspace_free(p->p_vmspace);
	free(arg, M_TEMP);
	return (error);
}
