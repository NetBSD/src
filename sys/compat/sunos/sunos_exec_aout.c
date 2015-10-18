/*	$NetBSD: sunos_exec_aout.c,v 1.19 2015/10/18 16:59:19 maxv Exp $	*/

/*
 * Copyright (c) 1993 Theo de Raadt
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
 * 3. The name of the author may not be used to endorse or promote products
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
__KERNEL_RCSID(0, "$NetBSD: sunos_exec_aout.c,v 1.19 2015/10/18 16:59:19 maxv Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/filedesc.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/mount.h>
#include <sys/malloc.h>
#include <sys/namei.h>
#include <sys/signalvar.h>
#include <sys/vnode.h>
#include <sys/file.h>
#include <sys/exec.h>
#include <sys/exec_aout.h>
#include <sys/resourcevar.h>
#include <sys/wait.h>

#include <sys/mman.h>

#include <sys/cpu.h>
#include <machine/reg.h>

#include <compat/sunos/sunos.h>
#include <compat/sunos/sunos_exec.h>
#include <compat/sunos/sunos_syscall.h>

#ifdef __sparc__
#define	sunos_exec_aout_prep_zmagic exec_aout_prep_zmagic
#define	sunos_exec_aout_prep_nmagic exec_aout_prep_nmagic
#define	sunos_exec_aout_prep_omagic exec_aout_prep_omagic
#endif

int sunos_exec_aout_prep_zmagic(struct lwp *, struct exec_package *);
int sunos_exec_aout_prep_nmagic(struct lwp *, struct exec_package *);
int sunos_exec_aout_prep_omagic(struct lwp *, struct exec_package *);

int
exec_sunos_aout_makecmds(struct lwp *l, struct exec_package *epp)
{
	struct sunos_exec *sunmag = epp->ep_hdr;
	int error = ENOEXEC;

	if (epp->ep_hdrvalid < sizeof(struct sunos_exec))
		return ENOEXEC;
	if (!SUNOS_M_NATIVE(sunmag->a_machtype))
		return (ENOEXEC);

	switch (sunmag->a_magic) {
	case ZMAGIC:
		error = sunos_exec_aout_prep_zmagic(l, epp);
		break;
	case NMAGIC:
		error = sunos_exec_aout_prep_nmagic(l, epp);
		break;
	case OMAGIC:
		error = sunos_exec_aout_prep_omagic(l, epp);
		break;
	}
	return error;
}

/*
 * the code below is only needed for sun2/sun3 emulation.
 */
#ifndef __sparc__

/* suns keep data seg aligned to SEGSIZ because of sun custom mmu */
#define SEGSIZ		0x20000
#define SUNOS_N_TXTADDR(x,m)	0x2000
#define SUNOS_N_DATADDR(x,m)	(((m)==OMAGIC) ? \
	(SUNOS_N_TXTADDR(x,m) + (x).a_text) : \
	(SEGSIZ + ((SUNOS_N_TXTADDR(x,m) + (x).a_text - 1) & ~(SEGSIZ-1))))
#define SUNOS_N_BSSADDR(x,m)	(SUNOS_N_DATADDR(x,m)+(x).a_data)

#define SUNOS_N_TXTOFF(x,m)	((m)==ZMAGIC ? 0 : sizeof (struct exec))
#define SUNOS_N_DATOFF(x,m)	(SUNOS_N_TXTOFF(x,m) + (x).a_text)

/*
 * sunos_exec_aout_prep_zmagic(): Prepare a SunOS ZMAGIC binary's exec package
 *
 * First, set of the various offsets/lengths in the exec package.
 *
 * Then, mark the text image busy (so it can be demand paged) or error
 * out if this is not possible.  Finally, set up vmcmds for the
 * text, data, bss, and stack segments.
 */
int
sunos_exec_aout_prep_zmagic(struct lwp *l, struct exec_package *epp)
{
	struct exec *execp = epp->ep_hdr;
	int error;

	epp->ep_taddr = SUNOS_N_TXTADDR(*execp, ZMAGIC);
	epp->ep_tsize = execp->a_text;
	epp->ep_daddr = SUNOS_N_DATADDR(*execp, ZMAGIC);
	epp->ep_dsize = execp->a_data + execp->a_bss;
	epp->ep_entry = execp->a_entry;

	error = vn_marktext(epp->ep_vp);
	if (error)
		return (error);

	/* set up command for text segment */
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_pagedvn, execp->a_text,
	    epp->ep_taddr, epp->ep_vp, SUNOS_N_TXTOFF(*execp, ZMAGIC),
	    VM_PROT_READ|VM_PROT_EXECUTE);

	/* set up command for data segment */
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_pagedvn, execp->a_data,
	    epp->ep_daddr, epp->ep_vp, SUNOS_N_DATOFF(*execp, ZMAGIC),
	    VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);

	/* set up command for bss segment */
	if (execp->a_bss)
		NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_zero, execp->a_bss,
		    epp->ep_daddr + execp->a_data, NULLVP, 0,
		    VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);

	return (*epp->ep_esch->es_setup_stack)(l, epp);
}

/*
 * sunos_exec_aout_prep_nmagic(): Prepare a SunOS NMAGIC binary's exec package
 */
int
sunos_exec_aout_prep_nmagic(struct lwp *l, struct exec_package *epp)
{
	struct exec *execp = epp->ep_hdr;
	long bsize, baddr;

	epp->ep_taddr = SUNOS_N_TXTADDR(*execp, NMAGIC);
	epp->ep_tsize = execp->a_text;
	epp->ep_daddr = SUNOS_N_DATADDR(*execp, NMAGIC);
	epp->ep_dsize = execp->a_data + execp->a_bss;
	epp->ep_entry = execp->a_entry;

	/* set up command for text segment */
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_readvn, execp->a_text,
	    epp->ep_taddr, epp->ep_vp, SUNOS_N_TXTOFF(*execp, NMAGIC),
	    VM_PROT_READ|VM_PROT_EXECUTE);

	/* set up command for data segment */
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_readvn, execp->a_data,
	    epp->ep_daddr, epp->ep_vp, SUNOS_N_DATOFF(*execp, NMAGIC),
	    VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);

	/* set up command for bss segment */
	baddr = roundup(epp->ep_daddr + execp->a_data, PAGE_SIZE);
	bsize = epp->ep_daddr + epp->ep_dsize - baddr;
	if (bsize > 0)
		NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_zero, bsize, baddr,
		    NULLVP, 0, VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);

	return (*epp->ep_esch->es_setup_stack)(l, epp);
}

/*
 * sunos_exec_aout_prep_omagic(): Prepare a SunOS OMAGIC binary's exec package
 */
int
sunos_exec_aout_prep_omagic(struct lwp *l, struct exec_package *epp)
{
	struct exec *execp = epp->ep_hdr;
	long bsize, baddr;

	epp->ep_taddr = SUNOS_N_TXTADDR(*execp, OMAGIC);
	epp->ep_tsize = execp->a_text;
	epp->ep_daddr = SUNOS_N_DATADDR(*execp, OMAGIC);
	epp->ep_dsize = execp->a_data + execp->a_bss;
	epp->ep_entry = execp->a_entry;

	/* set up command for text and data segments */
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_readvn,
	    execp->a_text + execp->a_data, epp->ep_taddr, epp->ep_vp,
	    SUNOS_N_TXTOFF(*execp, OMAGIC), VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);

	/* set up command for bss segment */
	baddr = roundup(epp->ep_daddr + execp->a_data, PAGE_SIZE);
	bsize = epp->ep_daddr + epp->ep_dsize - baddr;
	if (bsize > 0)
		NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_zero, bsize, baddr,
		    NULLVP, 0, VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);

	return (*epp->ep_esch->es_setup_stack)(l, epp);
}
#endif /* !sparc */
