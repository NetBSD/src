/*	$NetBSD: netbsd32_exec.c,v 1.1 1998/08/26 10:20:35 mrg Exp $	*/
/*	from: NetBSD: exec_aout.c,v 1.15 1996/09/26 23:34:46 cgd Exp */

/*
 * Copyright (c) 1998 Matthew R. Green.
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


#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/vnode.h>
#include <sys/exec.h>
#include <sys/resourcevar.h>

#include <vm/vm.h>

#include <compat/sparc32/sparc32.h>
#include <compat/sparc32/sparc32_exec.h>
#include <compat/sparc32/sparc32_syscall.h>

extern struct sysent sparc32_sysent[];
#ifdef SYSCALL_DEBUG
extern char *sparc32_syscallnames[];
#endif
extern char sigcode[], esigcode[];
const char sparc32_emul_path[] = "/emul/sparc32";
extern void sparc32_sendsig __P((sig_t, int, int, u_long)); /* XXX error fix only */
void sparc32_setregs __P((struct proc *, struct exec_package *, u_long));

struct emul emul_sparc32 = {
	"sparc32",
	NULL,
	sparc32_sendsig,	/* XXX needs to be written */
	sparc32_SYS_syscall,
	sparc32_SYS_MAXSYSCALL,
	sparc32_sysent,
#ifdef SYSCALL_DEBUG
	sparc32_syscallnames,
#else
	NULL,
#endif
	0,
	copyargs,
	sparc32_setregs,	/* XXX needs to be written?? */
	sigcode,
	esigcode,
};

/*
 * exec_sparc32_makecmds(): Check if it's an sparc32 a.out format
 * executable.
 *
 * Given a proc pointer and an exec package pointer, see if the referent
 * of the epp is in sparc32 a.out format.  Check 'standard' magic
 * numbers for this architecture.
 *
 * This function, in the former case, or the hook, in the latter, is
 * responsible for creating a set of vmcmds which can be used to build
 * the process's vm space and inserting them into the exec package.
 */

int
exec_sparc32_makecmds(p, epp)
	struct proc *p;
	struct exec_package *epp;
{
	sparc32_u_long midmag, magic;
	u_short mid;
	int error;
	struct sparc32_exec *execp = epp->ep_hdr;

	if (epp->ep_hdrvalid < sizeof(struct sparc32_exec))
		return ENOEXEC;

	midmag = (sparc32_u_long)ntohl(execp->a_midmag);
	mid = (midmag >> 16) & 0x3ff;
	magic = midmag & 0xffff;

	midmag = mid << 16 | magic;

	switch (midmag) {
	case (MID_MACHINE << 16) | ZMAGIC:
		error = sparc32_exec_aout_prep_zmagic(p, epp);
		break;
	case (MID_MACHINE << 16) | NMAGIC:
		error = sparc32_exec_aout_prep_nmagic(p, epp);
		break;
	case (MID_MACHINE << 16) | OMAGIC:
		error = sparc32_exec_aout_prep_omagic(p, epp);
	}

	if (error)
		kill_vmcmds(&epp->ep_vmcmds);

	return error;
}

/*
 * sparc32_exec_aout_prep_zmagic(): Prepare a 'native' ZMAGIC binary's exec package
 *
 * First, set of the various offsets/lengths in the exec package.
 *
 * Then, mark the text image busy (so it can be demand paged) or error
 * out if this is not possible.  Finally, set up vmcmds for the
 * text, data, bss, and stack segments.
 */

int
sparc32_exec_aout_prep_zmagic(p, epp)
	struct proc *p;
	struct exec_package *epp;
{
	struct sparc32_exec *execp = epp->ep_hdr;

	epp->ep_taddr = USRTEXT;
	epp->ep_tsize = execp->a_text;
	epp->ep_daddr = epp->ep_taddr + execp->a_text;
	epp->ep_dsize = execp->a_data + execp->a_bss;
	epp->ep_entry = execp->a_entry;

	/*
	 * check if vnode is in open for writing, because we want to
	 * demand-page out of it.  if it is, don't do it, for various
	 * reasons
	 */
	if ((execp->a_text != 0 || execp->a_data != 0) &&
	    epp->ep_vp->v_writecount != 0) {
#ifdef DIAGNOSTIC
		if (epp->ep_vp->v_flag & VTEXT)
			panic("exec: a VTEXT vnode has writecount != 0\n");
#endif
		return ETXTBSY;
	}
	epp->ep_vp->v_flag |= VTEXT;

	/* set up command for text segment */
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_pagedvn, execp->a_text,
	    epp->ep_taddr, epp->ep_vp, 0, VM_PROT_READ|VM_PROT_EXECUTE);

	/* set up command for data segment */
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_pagedvn, execp->a_data,
	    epp->ep_daddr, epp->ep_vp, execp->a_text,
	    VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);

	/* set up command for bss segment */
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_zero, execp->a_bss,
	    epp->ep_daddr + execp->a_data, NULLVP, 0,
	    VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);

	return exec_aout_setup_stack(p, epp);
}

/*
 * sparc32_exec_aout_prep_nmagic(): Prepare a 'native' NMAGIC binary's exec package
 */

int
sparc32_exec_aout_prep_nmagic(p, epp)
	struct proc *p;
	struct exec_package *epp;
{
	struct sparc32_exec *execp = epp->ep_hdr;
	long bsize, baddr;

	epp->ep_taddr = USRTEXT;
	epp->ep_tsize = execp->a_text;
	epp->ep_daddr = roundup(epp->ep_taddr + execp->a_text, __LDPGSZ);
	epp->ep_dsize = execp->a_data + execp->a_bss;
	epp->ep_entry = execp->a_entry;

	/* set up command for text segment */
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_readvn, execp->a_text,
	    epp->ep_taddr, epp->ep_vp, sizeof(struct exec),
	    VM_PROT_READ|VM_PROT_EXECUTE);

	/* set up command for data segment */
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_readvn, execp->a_data,
	    epp->ep_daddr, epp->ep_vp, execp->a_text + sizeof(struct exec),
	    VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);

	/* set up command for bss segment */
	baddr = roundup(epp->ep_daddr + execp->a_data, NBPG);
	bsize = epp->ep_daddr + epp->ep_dsize - baddr;
	if (bsize > 0)
		NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_zero, bsize, baddr,
		    NULLVP, 0, VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);

	return exec_aout_setup_stack(p, epp);
}

/*
 * sparc32_exec_aout_prep_omagic(): Prepare a 'native' OMAGIC binary's exec package
 */

int
sparc32_exec_aout_prep_omagic(p, epp)
	struct proc *p;
	struct exec_package *epp;
{
	struct sparc32_exec *execp = epp->ep_hdr;
	long dsize, bsize, baddr;

	epp->ep_taddr = USRTEXT;
	epp->ep_tsize = execp->a_text;
	epp->ep_daddr = epp->ep_taddr + execp->a_text;
	epp->ep_dsize = execp->a_data + execp->a_bss;
	epp->ep_entry = execp->a_entry;

	/* set up command for text and data segments */
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_readvn,
	    execp->a_text + execp->a_data, epp->ep_taddr, epp->ep_vp,
	    sizeof(struct exec), VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);

	/* set up command for bss segment */
	baddr = roundup(epp->ep_daddr + execp->a_data, NBPG);
	bsize = epp->ep_daddr + epp->ep_dsize - baddr;
	if (bsize > 0)
		NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_zero, bsize, baddr,
		    NULLVP, 0, VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);

	/*
	 * Make sure (# of pages) mapped above equals (vm_tsize + vm_dsize);
	 * obreak(2) relies on this fact. Both `vm_tsize' and `vm_dsize' are
	 * computed (in execve(2)) by rounding *up* `ep_tsize' and `ep_dsize'
	 * respectively to page boundaries.
	 * Compensate `ep_dsize' for the amount of data covered by the last
	 * text page. 
	 */
	dsize = epp->ep_dsize + execp->a_text - roundup(execp->a_text, NBPG);
	epp->ep_dsize = (dsize > 0) ? dsize : 0;
	return exec_aout_setup_stack(p, epp);
}

/* XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX */
/*
 * this is from sys/arch/sparc/sparc/machdep.c:setregs()
 */
/* XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX */

/*
 * Set up registers on exec.
 *
 * XXX this entire mess must be fixed
 */
/* ARGSUSED */
void
sparc32_setregs(p, pack, stack)
	struct proc *p;
	struct exec_package *pack;
	u_long stack; /* XXX */
{
	struct trapframe32 *tf = (struct trapframe32 *)p->p_md.md_tf; /* XXX XXX XXX */
	struct fpstate *fs; /* XXX */
	int psr;

	/* Don't allow misaligned code by default */
	p->p_md.md_flags &= ~MDP_FIXALIGN;

	/*
	 * Set the registers to 0 except for:
	 *	%o6: stack pointer, built in exec())
	 *	%psr: (retain CWP and PSR_S bits)
	 *	%g1: address of PS_STRINGS (used by crt0)
	 *	%pc,%npc: entry point of program
	 */
	psr = tf->tf_psr & (PSR_S | PSR_CWP);
	if ((fs = p->p_md.md_fpstate) != NULL) {
		/*
		 * We hold an FPU state.  If we own *the* FPU chip state
		 * we must get rid of it, and the only way to do that is
		 * to save it.  In any case, get rid of our FPU state.
		 */
		if (p == fpproc) {
			savefpstate(fs);
			fpproc = NULL;
		}
		free((void *)fs, M_SUBPROC);
		p->p_md.md_fpstate = NULL;
	}
	bzero((caddr_t)tf, sizeof *tf);
	tf->tf_psr = psr;
	tf->tf_global[1] = (int)PS_STRINGS;
	tf->tf_pc = pack->ep_entry & ~3;
	tf->tf_npc = tf->tf_pc + 4;
	stack -= sizeof(struct rwindow32);
	tf->tf_out[6] = stack;
}
