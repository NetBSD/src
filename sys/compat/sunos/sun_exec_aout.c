/*
 * Copyright (c) 1993 Christopher G. Demetriou
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
 *    derived from this software withough specific prior written permission
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
 *
 * from: Id: exec_aout.c,v 1.1.2.5 1993/10/17 04:29:26 deraadt Exp
 * $Id: sun_exec_aout.c,v 1.2 1993/11/20 03:05:41 deraadt Exp $
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/filedesc.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/mount.h>
#include <sys/malloc.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/file.h>
#include <sys/exec.h>
#include <sys/resourcevar.h>
#include <sys/wait.h>

#include <machine/cpu.h>
#include <machine/reg.h>
#include <machine/exec.h>

#include <sys/mman.h>
#include <vm/vm.h>
#include <sys/vm/vm_param.h>
#include <sys/vm/vm_map.h>
#include <sys/vm/vm_kern.h>
#include <sys/vm/vm_pager.h>

/*
 * define some macros usually in <a.out.h> that come in handy later.
 *
 * NOTE: This is the only thing different than what's in 
 *       kern/exec_aout.c ! Perhaps the two files could merge somehow?
 */

/* suns keep data seg aligned to SEGSIZ because of sun custom mmu */
#define SEGSIZ 0x20000
#define N_TXTADDR(x,m) __LDPGSZ
#define N_DATADDR(x,m) (((m)==OMAGIC) ? (N_TXTADDR(x,m) + (x).a_text) \
			: (SEGSIZ + ((N_TXTADDR(x,m) + (x).a_text - 1) \
				       & ~(SEGSIZ-1))))
#define N_BSSADDR(x,m) (N_DATADDR(x,m)+(x).a_data)

#define N_TXTOFF(x,m) ((m)==ZMAGIC ? 0 : sizeof (struct exec))
#define N_DATOFF(x,m) (N_TXTOFF(x,m) + (x).a_text)

/*
 * sun_exec_aout_prep_zmagic(): Prepare a SunOS ZMAGIC binary's exec package
 *
 * First, set of the various offsets/lengths in the exec package.
 *
 * Then, mark the text image busy (so it can be demand paged) or error
 * out if this is not possible.  Finally, set up vmcmds for the
 * text, data, bss, and stack segments.
 */

int
sun_exec_aout_prep_zmagic(p, epp)
	struct proc *p;
	struct exec_package *epp;
{
	struct exec *execp = epp->ep_execp;
	struct exec_vmcmd *ccmdp;

	epp->ep_taddr = N_TXTADDR (*execp, ZMAGIC);
	epp->ep_tsize = execp->a_text;
	epp->ep_daddr = N_DATADDR (*execp, ZMAGIC);
	epp->ep_dsize = execp->a_data + execp->a_bss;
	epp->ep_entry = execp->a_entry;

	/*
	 * check if vnode is in open for writing, because we want to
	 * demand-page out of it.  if it is, don't do it, for various
	 * reasons
	 */
	if ((execp->a_text != 0 || execp->a_data != 0) &&
	    (epp->ep_vp->v_flag & VTEXT) == 0 && epp->ep_vp->v_writecount != 0) {
#ifdef DIAGNOSTIC
		if (epp->ep_vp->v_flag & VTEXT)
			panic("exec: a VTEXT vnode has writecount != 0\n");
#endif
		epp->ep_vcp = NULL;
		return ETXTBSY;
	}
	epp->ep_vp->v_flag |= VTEXT;

	/* set up command for text segment */
	epp->ep_vcp = new_vmcmd(vmcmd_map_pagedvn,
	    execp->a_text,
	    epp->ep_taddr,
	    epp->ep_vp,
	    N_TXTOFF(*execp, ZMAGIC),
	    VM_PROT_READ | VM_PROT_EXECUTE);
	ccmdp = epp->ep_vcp;

	/* set up command for data segment */
	ccmdp->ev_next = new_vmcmd(vmcmd_map_pagedvn,
	    execp->a_data,
	    epp->ep_daddr,
	    epp->ep_vp,
	    N_DATOFF(*execp, ZMAGIC),
	    VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE);
	ccmdp = ccmdp->ev_next;

	/* set up command for bss segment */
	ccmdp->ev_next = new_vmcmd(vmcmd_map_zero,
	    execp->a_bss,
	    epp->ep_daddr + execp->a_data,
	    0,
	    0,
	    VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE);
	ccmdp = ccmdp->ev_next;

	return exec_aout_setup_stack(p, epp, ccmdp);
}

/*
 * sun_exec_aout_prep_nmagic(): Prepare a SunOS NMAGIC binary's exec package
 */

int
sun_exec_aout_prep_nmagic(p, epp)
	struct proc *p;
	struct exec_package *epp;
{
	struct exec *execp = epp->ep_execp;
	struct exec_vmcmd *ccmdp;
	long bsize, baddr;

	epp->ep_taddr = N_TXTADDR(*execp, NMAGIC);
	epp->ep_tsize = execp->a_text;
	epp->ep_daddr = N_DATADDR(*execp, NMAGIC);
	epp->ep_dsize = execp->a_data + execp->a_bss;
	epp->ep_entry = execp->a_entry;

	/* set up command for text segment */
	epp->ep_vcp = new_vmcmd(vmcmd_map_readvn,
	    execp->a_text,
	    epp->ep_taddr,
	    epp->ep_vp,
	    N_TXTOFF(*execp, NMAGIC),
	    VM_PROT_READ | VM_PROT_EXECUTE);
	ccmdp = epp->ep_vcp;

	/* set up command for data segment */
	ccmdp->ev_next = new_vmcmd(vmcmd_map_readvn,
	    execp->a_data,
	    epp->ep_daddr,
	    epp->ep_vp,
	    N_DATOFF(*execp, NMAGIC),
	    VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE);
	ccmdp = ccmdp->ev_next;

	/* set up command for bss segment */
	baddr = roundup(epp->ep_daddr + execp->a_data, NBPG);
	bsize = epp->ep_daddr + epp->ep_dsize - baddr;
	if (bsize > 0) {
		ccmdp->ev_next = new_vmcmd(vmcmd_map_zero, bsize, baddr,
		    0, 0, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE);
		ccmdp = ccmdp->ev_next;
	}

	return exec_aout_setup_stack(p, epp, ccmdp);
}

/*
 * sun_exec_aout_prep_omagic(): Prepare a SunOS OMAGIC binary's exec package
 */

int
sun_exec_aout_prep_omagic(p, epp)
	struct proc *p;
	struct exec_package *epp;
{
	struct exec *execp = epp->ep_execp;
	struct exec_vmcmd *ccmdp;
	long bsize, baddr;

	epp->ep_taddr = N_TXTADDR(*execp, OMAGIC);
	epp->ep_tsize = execp->a_text;
	epp->ep_daddr = N_DATADDR(*execp, OMAGIC);
	epp->ep_dsize = execp->a_data + execp->a_bss;
	epp->ep_entry = execp->a_entry;

	/* set up command for text and data segments */
	epp->ep_vcp = new_vmcmd(vmcmd_map_readvn,
	    execp->a_text + execp->a_data,
	    epp->ep_taddr,
	    epp->ep_vp,
	    N_TXTOFF(*execp, OMAGIC),
	    VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE);
	ccmdp = epp->ep_vcp;

	/* set up command for bss segment */
	baddr = roundup(epp->ep_daddr + execp->a_data, __LDPGSZ);
	bsize = epp->ep_daddr + epp->ep_dsize - baddr;
	if (bsize > 0) {
		ccmdp->ev_next = new_vmcmd(vmcmd_map_zero, bsize, baddr,
		    0, 0, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE);
		ccmdp = ccmdp->ev_next;
	}

	return exec_aout_setup_stack(p, epp, ccmdp);
}
