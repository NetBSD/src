/*	$NetBSD: vax1k_exec.c,v 1.2.4.1 2001/09/13 01:15:28 thorpej Exp $	*/

/*
 * Copyright (c) 1993, 1994 Christopher G. Demetriou
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Christopher G. Demetriou.
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

/*
 * Exec glue to provide compatibility with older NetBSD vax1k exectuables.
 *
 * Because NetBSD/vax now uses 4k page size, older binaries (that started
 * on an 1k boundary) cannot be mmap'ed. Therefore they are read in 
 * (via vn_rdwr) as OMAGIC binaries and executed. This will use a little
 * bit more memory, but otherwise won't affect the execution speed.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/vnode.h>
#include <sys/exec.h>
#include <sys/resourcevar.h>

#include <compat/vax1k/vax1k_exec.h>

#if defined(_KERNEL_OPT)
#include "opt_compat_43.h"
#else
#define COMPAT_43	/* enable 4.3BSD binaries for lkm */
#endif

int	exec_vax1k_prep_anymagic __P((struct proc *p, struct exec_package *epp,
				      int, int));

/*
 * exec_vax1k_makecmds(): Check if it's an a.out-format executable
 * with an vax1k magic number.
 *
 * Given a proc pointer and an exec package pointer, see if the referent
 * of the epp is in a.out format.  Just check 'standard' magic numbers for
 * this architecture.
 *
 * This function, in the former case, or the hook, in the latter, is
 * responsible for creating a set of vmcmds which can be used to build
 * the process's vm space and inserting them into the exec package.
 */

int
exec_vax1k_makecmds(p, epp)
	struct proc *p;
	struct exec_package *epp;
{
	u_long midmag, magic;
	u_short mid;
	int error;
	struct exec *execp = epp->ep_hdr;

	if (epp->ep_hdrvalid < sizeof(struct exec))
		return ENOEXEC;

	midmag = ntohl(execp->a_midmag);
	mid = (midmag >> 16) & 0x3ff;
	magic = midmag & 0xffff;

	midmag = mid << 16 | magic;

	switch (midmag) {
	case (MID_VAX1K << 16) | ZMAGIC:
		error = exec_vax1k_prep_anymagic(p, epp, 0, 0);
		goto done;

	case (MID_VAX1K << 16) | NMAGIC:
		error = exec_vax1k_prep_anymagic(p, epp,
						 sizeof(struct exec), 1);
		goto done;

	case (MID_VAX1K << 16) | OMAGIC:
		error = exec_vax1k_prep_anymagic(p, epp,
						 sizeof(struct exec), 0);
		goto done;
	}

#ifdef COMPAT_43
	/*
	 * 4.3BSD pre-dates cpu midmag (e.g. MID_VAX1K).   instead, we 
	 * expect a magic number in native byte order.
	 */
	switch (execp->a_midmag) {
	case ZMAGIC:
		error = exec_vax1k_prep_anymagic(p, epp, VAX1K_LDPGSZ, 0);
		goto done;
	
	case NMAGIC:
		error = exec_vax1k_prep_anymagic(p, epp, 
					 	sizeof(struct exec), 1);
		goto done;

	case OMAGIC:
		error = exec_vax1k_prep_anymagic(p, epp, 
					 	sizeof(struct exec), 0);
		goto done;
	}
#endif

	/* failed... */
	error = ENOEXEC;

done:
	if (error)
		kill_vmcmds(&epp->ep_vmcmds);

	return error;
}

/*
 * exec_vax1k_prep_anymagic(): Prepare an vax1k ?MAGIC binary's exec package
 *
 * First, set of the various offsets/lengths in the exec package.
 * Note that all code is mapped RW; no protection, but because it is
 * only used for compatibility it won't hurt.
 *
 */
int
exec_vax1k_prep_anymagic(p, epp, text_foffset, textpad)
	struct proc *p;
	struct exec_package *epp;
	int text_foffset, textpad;
{
        struct exec *execp = epp->ep_hdr;

	epp->ep_taddr = execp->a_entry & ~(VAX1K_USRTEXT - 1);
	epp->ep_tsize = execp->a_text;
	epp->ep_daddr = epp->ep_taddr + epp->ep_tsize;
	if (textpad)			/* pad for NMAGIC? */
		epp->ep_daddr = (epp->ep_daddr + (VAX1K_LDPGSZ - 1)) & 
						~(VAX1K_LDPGSZ - 1);
	epp->ep_dsize = execp->a_data;
	epp->ep_entry = execp->a_entry;

	/* first allocate memory for text+data+bss */
        NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_zero, 
		round_page(epp->ep_daddr + epp->ep_dsize + execp->a_bss) -
			trunc_page(epp->ep_taddr),	/* size */
		trunc_page(epp->ep_taddr), NULLVP,	/* addr, vnode */
		0, VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);

	/* then read the text in the area we just allocated */
       	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_readvn,
		epp->ep_tsize, epp->ep_taddr, epp->ep_vp, text_foffset,
    	VM_PROT_WRITE|VM_PROT_READ|VM_PROT_EXECUTE);

	/* next read the data */
	if (epp->ep_dsize) {
        	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_readvn,
			epp->ep_dsize, epp->ep_daddr, epp->ep_vp, 
			text_foffset + epp->ep_tsize,
			VM_PROT_WRITE|VM_PROT_READ|VM_PROT_EXECUTE);
	}

	/* now bump up the dsize to include the bss so that sbrk works */
	epp->ep_dsize += execp->a_bss;

	/* finally, setup the stack ... */
        return exec_aout_setup_stack(p, epp);
}
