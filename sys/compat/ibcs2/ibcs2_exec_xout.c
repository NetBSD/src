/*	$NetBSD: ibcs2_exec_xout.c,v 1.11.6.1 2006/06/01 22:35:49 kardel Exp $	*/

/*
 * Copyright (c) 1994, 1995, 1998 Scott Bartram
 * Copyright (c) 1994 Adam Glass
 * Copyright (c) 1993, 1994 Christopher G. Demetriou
 * All rights reserved.
 *
 * originally from kern/exec_ecoff.c
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
 *      This product includes software developed by Scott Bartram.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ibcs2_exec_xout.c,v 1.11.6.1 2006/06/01 22:35:49 kardel Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/exec.h>
#include <sys/resourcevar.h>

#include <sys/mman.h>
#include <sys/sa.h>
#include <sys/syscallargs.h>

#include <machine/cpu.h>
#include <machine/reg.h>
#include <machine/ibcs2_machdep.h>

#include <compat/ibcs2/ibcs2_types.h>
#include <compat/ibcs2/ibcs2_exec.h>
#include <compat/ibcs2/ibcs2_errno.h>
#include <compat/ibcs2/ibcs2_util.h>
#include <compat/ibcs2/ibcs2_syscall.h>

int exec_ibcs2_xout_prep_nmagic __P((struct lwp *, struct exec_package *,
				     struct xexec *, struct xext *));
int exec_ibcs2_xout_prep_zmagic __P((struct lwp *, struct exec_package *,
				     struct xexec *, struct xext *));

int
exec_ibcs2_xout_makecmds(l, epp)
	struct lwp *l;
	struct exec_package *epp;
{
	int error;
	struct xexec *xp = epp->ep_hdr;
	struct xext *xep;

	if (epp->ep_hdrvalid < XOUT_HDR_SIZE)
		return ENOEXEC;

	if ((xp->x_magic != XOUT_MAGIC) || (xp->x_cpu != XC_386))
		return ENOEXEC;
	if ((xp->x_renv & (XE_ABS | XE_VMOD)) || !(xp->x_renv & XE_EXEC))
		return ENOEXEC;

	xep = (void *)((char *)epp->ep_hdr + sizeof(struct xexec));
#ifdef notyet
	if (xp->x_renv & XE_PURE)
		error = exec_ibcs2_xout_prep_zmagic(l, epp, xp, xep);
	else
#endif
		error = exec_ibcs2_xout_prep_nmagic(l, epp, xp, xep);

	if (error)
		kill_vmcmds(&epp->ep_vmcmds);

	return error;
}

/*
 * exec_ibcs2_xout_prep_nmagic(): Prepare a pure x.out binary's exec package
 *
 */

int
exec_ibcs2_xout_prep_nmagic(l, epp, xp, xep)
	struct lwp *l;
	struct exec_package *epp;
	struct xexec *xp;
	struct xext *xep;
{
	int error;
	size_t nseg, i;
	long baddr, bsize;
	struct xseg *xs;
	size_t resid;
	size_t segsize = (size_t)xep->xe_segsize;

	if (segsize > 16 * sizeof(*xs))
		return ENOEXEC;

	/* read in segment table */
	xs = (struct xseg *)malloc(segsize, M_TEMP, M_WAITOK);
	error = vn_rdwr(UIO_READ, epp->ep_vp, (caddr_t)xs,
			segsize, xep->xe_segpos,
			UIO_SYSSPACE, IO_NODELOCKED, l->l_proc->p_cred,
			&resid, NULL);
	if (error) {
		DPRINTF(("segment table read error %d\n", error));
		free(xs, M_TEMP);
		return ENOEXEC;
	}

	for (nseg = segsize / sizeof(*xs), i = 0; i < nseg; i++) {
		switch (xs[i].xs_type) {
		case XS_TTEXT:	/* text segment */

			DPRINTF(("text addr %lx psize %ld vsize %ld off %ld\n",
				 xs[i].xs_rbase, xs[i].xs_psize,
				 xs[i].xs_vsize, xs[i].xs_filpos));

			epp->ep_taddr = xs[i].xs_rbase;	/* XXX - align ??? */
			epp->ep_tsize = xs[i].xs_vsize;

			DPRINTF(("VMCMD: addr %lx size %ld offset %ld\n",
				 epp->ep_taddr, epp->ep_tsize,
				 xs[i].xs_filpos));
			NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_readvn,
				  epp->ep_tsize, epp->ep_taddr,
				  epp->ep_vp, xs[i].xs_filpos,
				  VM_PROT_READ|VM_PROT_EXECUTE);
			break;

		case XS_TDATA:	/* data segment */

			DPRINTF(("data addr %lx psize %ld vsize %ld off %ld\n",
				 xs[i].xs_rbase, xs[i].xs_psize,
				 xs[i].xs_vsize, xs[i].xs_filpos));

			epp->ep_daddr = xs[i].xs_rbase;	/* XXX - align ??? */
			epp->ep_dsize = xs[i].xs_vsize;

			DPRINTF(("VMCMD: addr %lx size %ld offset %ld\n",
				 epp->ep_daddr, xs[i].xs_psize,
				 xs[i].xs_filpos));
			NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_readvn,
				  xs[i].xs_psize, epp->ep_daddr,
				  epp->ep_vp, xs[i].xs_filpos,
				  VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);

			/* set up command for bss segment */
			baddr = round_page(epp->ep_daddr + xs[i].xs_psize);
			bsize = epp->ep_daddr + epp->ep_dsize - baddr;
			if (bsize > 0) {
				DPRINTF(("VMCMD: bss addr %lx size %ld off %d\n",
					 baddr, bsize, 0));
				NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_zero,
					  bsize, baddr, NULLVP, 0,
					  VM_PROT_READ|VM_PROT_WRITE|
					  VM_PROT_EXECUTE);
			}
			break;

		default:
			break;
		}
	}

	/* set up entry point */
	epp->ep_entry = xp->x_entry;

	DPRINTF(("text addr: %lx size: %ld data addr: %lx size: %ld entry: %lx\n",
		 epp->ep_taddr, epp->ep_tsize,
		 epp->ep_daddr, epp->ep_dsize,
		 epp->ep_entry));

	free(xs, M_TEMP);
	return (*epp->ep_esch->es_setup_stack)(l, epp);
}
