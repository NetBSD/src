/*	$NetBSD: trap.c,v 1.36 2005/12/24 20:07:32 perry Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department and Ralph Campbell.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: Utah Hdr: trap.c 1.32 91/04/06
 *
 *	@(#)trap.c	8.5 (Berkeley) 1/11/94
 */
/*
 * Copyright (c) 1988 University of Utah.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department and Ralph Campbell.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: Utah Hdr: trap.c 1.32 91/04/06
 *
 *	@(#)trap.c	8.5 (Berkeley) 1/11/94
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: trap.c,v 1.36 2005/12/24 20:07:32 perry Exp $");

#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/systm.h>
#include <sys/sa.h>
#include <sys/savar.h>
#include <sys/userret.h>

#include <uvm/uvm_extern.h>

#include <sh5/fpu.h>

#include <machine/cpu.h>
#include <machine/trap.h>
#include <machine/pmap.h>

#ifdef DDB
#include <machine/db_machdep.h>
#endif


/* Used to trap faults while probing */
label_t *onfault;


#ifdef DEBUG
int sh5_trap_debug;
#endif

void
userret(struct lwp *l)
{

	/* Invoke MI userret code */
	mi_userret(l);

	l->l_md.md_flags &= ~MDP_FPSAVED;
	curcpu()->ci_schedstate.spc_curpriority = l->l_priority = l->l_usrpri;
}

/*
 * Handle synchronous exceptions
 */
void
trap(struct lwp *l, struct trapframe *tf)
{
	struct proc *p;
	u_int traptype;
	vaddr_t vaddr;
	caddr_t pcb_onfault;
	vm_prot_t ftype;
	ksiginfo_t ksi;

	uvmexp.traps++;

	traptype = tf->tf_state.sf_expevt;
	if (USERMODE(tf)) {
		KDASSERT(l != NULL);
		traptype |= T_USER;
		l->l_md.md_regs = tf;
	} else
	if (l == NULL)
		l = &lwp0;

	pcb_onfault = l->l_addr->u_pcb.pcb_onfault;
	l->l_addr->u_pcb.pcb_onfault = NULL;
	p = l->l_proc;
	vaddr = (vaddr_t) tf->tf_state.sf_tea;

	switch (traptype) {
	default:
	dopanic:
		(void) splhigh();
		{
			register_t sr;

			/* Make sure the FPU is enabled */
			__asm volatile("getcon sr, %0" : "=r"(sr));
			sr &= ~SH5_CONREG_SR_FD;
			__asm volatile("putcon %0, sr" :: "r"(sr));
		}
		printf("\ntrap: %s in %s mode\n",
		    trap_type(traptype), USERMODE(tf) ? "user" : "kernel");
		printf("SSR=0x%x, SPC=0x%lx, TEA=0x%lx, TRA=0x%x\n",
			(u_int)tf->tf_state.sf_ssr,
			(uintptr_t)tf->tf_state.sf_spc,
			(uintptr_t)tf->tf_state.sf_tea,
			(u_int)tf->tf_state.sf_tra);
		printf("pid=%d cmd=%s, usp=0x%lx ", p->p_pid, p->p_comm,
		    (uintptr_t)tf->tf_caller.r15);
		printf("ksp=0x%lx\n", (vaddr_t)tf);
		if (traptype & T_USER) {
			/* This shouldn't happen ... */
			KSI_INIT_TRAP(&ksi);
			ksi.ksi_signo = SIGILL;
			ksi.ksi_code = ILL_ILLTRP;
			ksi.ksi_addr = (void *)(uintptr_t)tf->tf_state.sf_tea;
			ksi.ksi_trap = (int) tf->tf_state.sf_expevt;
			break;
		}
#if defined(DDB)
		kdb_trap(traptype, tf);
#else
#ifdef DIAGNOSTIC
		dump_trapframe(printf, "\n", tf);
#endif
#endif
		panic("trap");
		/* NOTREACHED */

	case T_EXECPROT|T_USER:
	case T_READPROT|T_USER:
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_signo = SIGSEGV;
		ksi.ksi_code = SEGV_ACCERR;
		ksi.ksi_addr = (void *)(uintptr_t)tf->tf_state.sf_tea;
		ksi.ksi_trap = (int) tf->tf_state.sf_expevt;
		break;

	case T_IADDERR|T_USER:
	case T_RADDERR|T_USER:
	case T_WADDERR|T_USER:
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_signo = SIGBUS;
		ksi.ksi_code = BUS_ADRERR;
		ksi.ksi_addr = (void *)(uintptr_t)tf->tf_state.sf_tea;
		ksi.ksi_trap = (int) tf->tf_state.sf_expevt;
		break;

	case T_WRITEPROT:
	case T_WRITEPROT|T_USER:
		/*
		 * This could be caused by tracking page Modification
		 * status, where managed read/write pages are initially
		 * mapped read-only in the pmap module.
		 */
		if (pmap_write_trap(p, USERMODE(tf), vaddr)) {
			if (traptype & T_USER)
				userret(l);
			l->l_addr->u_pcb.pcb_onfault = pcb_onfault;
			return;
		}

		ftype = VM_PROT_WRITE;

		/*
		 * The page really is read-only. Let UVM deal with it.
		 */
		if (vaddr >= VM_MIN_KERNEL_ADDRESS)
			goto kernelfault;

		/*
		 * But catch the kernel writing to a read-only page
		 * outside of copyin/copyout and friends.
		 */
		if ((traptype & T_USER) == 0 && pcb_onfault == NULL)
			goto dopanic;
		goto pagefault;

	case T_RTLBMISS:
	case T_WTLBMISS:
	case T_ITLBMISS:
		ftype = (traptype == T_WTLBMISS) ? VM_PROT_WRITE :
		    ((traptype == T_ITLBMISS) ? VM_PROT_EXECUTE : VM_PROT_READ);
		if (vaddr >= VM_MIN_KERNEL_ADDRESS)
			goto kernelfault;

		if (pcb_onfault == NULL)
			goto dopanic;
		goto pagefault;

	case T_RTLBMISS|T_USER:
		ftype = VM_PROT_READ;
		goto pagefault;

	case T_ITLBMISS|T_USER:
		ftype = VM_PROT_EXECUTE;
		goto pagefault;

	case T_WTLBMISS|T_USER:
		ftype = VM_PROT_WRITE;
		/*FALLTHROUGH*/

	pagefault:
	    {
		vaddr_t va;
		struct vmspace *vm;
		struct vm_map *map;
		int rv;

		vm = p->p_vmspace;
		map = &vm->vm_map;
		va = trunc_page(vaddr);
		if (l->l_flag & L_SA) {
			l->l_savp->savp_faultaddr = (vaddr_t)vaddr;
			l->l_flag |= L_SA_PAGEFAULT;
		}
		rv = uvm_fault(map, va, 0, ftype);

		/*
		 * If this was a stack access we keep track of the maximum
		 * accessed stack size.  Also, if vm_fault gets a protection
		 * failure it is due to accessing the stack region outside
		 * the current limit and we need to reflect that as an access
		 * error.
		 */
		if ((caddr_t)va >= vm->vm_maxsaddr) {
			if (rv == 0)
				uvm_grow(p, va);
			else if (rv == EACCES)
				rv = EFAULT;
		}

		l->l_flag &= ~L_SA_PAGEFAULT;

		if (rv == 0) {
			if (traptype & T_USER)
				userret(l);
			l->l_addr->u_pcb.pcb_onfault = pcb_onfault;
			return;
		}

		if ((traptype & T_USER) == 0)
			goto copyfault;

		KASSERT(pcb_onfault == NULL);

		KSI_INIT_TRAP(&ksi);
		ksi.ksi_signo = SIGSEGV;
		ksi.ksi_code = SEGV_MAPERR;
		ksi.ksi_addr = (void *)(uintptr_t)tf->tf_state.sf_tea;
		ksi.ksi_trap = (int) tf->tf_state.sf_expevt;

		if (rv == ENOMEM) {
			printf("UVM: pid %d (%s), uid %d killed: out of swap\n",
			    p->p_pid, p->p_comm,
			    (p->p_cred && p->p_ucred) ?
			    p->p_ucred->cr_uid : -1);
			ksi.ksi_signo = SIGKILL;
		} else
		if (rv == EACCES)
			ksi.ksi_code = SEGV_ACCERR;
		break;
	    }

	kernelfault:
	    {
		vaddr_t va;
		int rv;

		va = trunc_page(vaddr);
		rv = uvm_fault(kernel_map, va, 0, ftype);
		if (rv == 0) {
			l->l_addr->u_pcb.pcb_onfault = pcb_onfault;
			return;
		}
		/*FALLTHROUGH*/
	    }

	case T_RADDERR:
	case T_WADDERR:
	case T_READPROT:
		if (onfault) {
			l->l_addr->u_pcb.pcb_onfault = pcb_onfault;
			longjmp(onfault);
		}
		/*FALLTHROUGH*/

	copyfault:
		if (pcb_onfault == NULL)
			goto dopanic;
		tf->tf_state.sf_spc = (register_t)(uintptr_t)pcb_onfault;
		l->l_addr->u_pcb.pcb_onfault = pcb_onfault;
		return;

	case T_BREAK|T_USER:
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_signo = SIGTRAP;
		ksi.ksi_code = TRAP_BRKPT;
		ksi.ksi_addr = (void *)(uintptr_t)tf->tf_state.sf_spc;
		ksi.ksi_trap = (int) tf->tf_state.sf_expevt;
		break;

	case T_RESINST|T_USER:
	case T_FPUDIS|T_USER:
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_signo = SIGILL;
		ksi.ksi_code = ILL_ILLOPC;
		ksi.ksi_addr = (void *)(uintptr_t)tf->tf_state.sf_spc;
		ksi.ksi_trap = (int) tf->tf_state.sf_expevt;
		break;

	case T_ILLSLOT|T_USER:
	case T_SLOTFPUDIS|T_USER:
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_signo = SIGILL;
		ksi.ksi_code = ILL_ILLOPC; /* XXX: Could do with ILL_DELAYSLOT */
		ksi.ksi_addr = (void *)(uintptr_t)tf->tf_state.sf_spc;
		ksi.ksi_trap = (int) tf->tf_state.sf_expevt;
		break;

	case T_FPUEXC|T_USER:
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_signo = SIGFPE;
		ksi.ksi_addr = (void *)(uintptr_t)tf->tf_state.sf_spc;
		ksi.ksi_trap = T_FPUEXC;

#ifdef DEBUG
		printf("trap: FPUEXC - fpscr = 0x%x\n", ksi.ksi_trap);
#endif

		switch (sh5_getfpscr() & SH5_FPSCR_CAUSE_MASK) {
		case SH5_FPSCR_CAUSE_I:
			ksi.ksi_code = FPE_FLTRES;
			break;

		case SH5_FPSCR_CAUSE_I | SH5_FPSCR_CAUSE_U:
			ksi.ksi_code = FPE_FLTUND;
			break;

		case SH5_FPSCR_CAUSE_I | SH5_FPSCR_CAUSE_O:
			ksi.ksi_code = FPE_FLTOVF;
			break;

		case SH5_FPSCR_CAUSE_Z:
			ksi.ksi_code = FPE_FLTDIV;
			break;

		case SH5_FPSCR_CAUSE_V:
		case SH5_FPSCR_CAUSE_E:
		default:
			ksi.ksi_code = FPE_FLTINV;
			break;
		}

		/*
		 * An FPUEXC leaves the PC pointing to the FP instruction
		 * which caused the exception. To avoid an endless loop if
		 * the user is ignoring or catching SIGFPE, adjust the saved
		 * PC to skip over the offending instruction.
		 *
		 * XXX: The whole business of supporting IEEE754 on Sh5 needs
		 * to be re-visited. We currently don't do anywhere near
		 * enough work in the kernel to deal with FPU exceptions.
		 */
		tf->tf_state.sf_spc += 4;
		break;

	case T_AST|T_USER:
		if (p->p_flag & P_OWEUPC) {
			p->p_flag &= ~P_OWEUPC;
			ADDUPROF(p);
		}
		if (curcpu()->ci_want_resched)
			preempt(0);
		userret(l);
		return;

	case T_TRAP|T_USER:
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_signo = SIGILL;
		ksi.ksi_code = ILL_ILLTRP;
		ksi.ksi_addr = (void *)(uintptr_t)tf->tf_state.sf_spc;
		ksi.ksi_trap = (int) tf->tf_state.sf_tra;	/* XXX */
		break;

	case T_DIVZERO|T_USER:
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_signo = SIGFPE;
		ksi.ksi_code = FPE_INTDIV;
		ksi.ksi_addr = (void *)(uintptr_t)tf->tf_state.sf_spc;
		ksi.ksi_trap = T_DIVZERO;
		break;

	case T_NMI:
	case T_NMI|T_USER:
		printf("trap: NMI detected\n");
		sh5_nmi_clear();
		/*FALLTHROUGH*/

#ifdef DDB
	case T_BREAK:
		if (kdb_trap(traptype, tf))
			return;
#endif
		goto dopanic;
	}

	(*l->l_proc->p_emul->e_trapsignal)(l, &ksi);
	userret(l);
}

/*
 * Handle "TRAPA"-induced synchronous exceptions
 */
void
trapa(struct lwp *l, struct trapframe *tf)
{

	uvmexp.traps++;
	l->l_md.md_regs = tf;

	switch (tf->tf_state.sf_tra) {
	case TRAPA_SYSCALL:
#ifdef DIAGNOSTIC
		if (!USERMODE(tf) || l == NULL) {
			const char *pstr;

			if (l == NULL)
				pstr = "trapa: syscall with NULL lwp!";
			else
				pstr = "trapa: syscall trap in kernel mode!";

			if (l != NULL)
				printf("pid=%d cmd=%s, usp=0x%lx ",
				    l->l_proc->p_pid, l->l_proc->p_comm,
				    (uintptr_t)tf->tf_caller.r15);
			else
				printf("curlwp == NULL ");
			printf("trapa: SPC=0x%lx, SSR=0x%x\n",
			    (uintptr_t)tf->tf_state.sf_spc,
			    (u_int)tf->tf_state.sf_ssr);
			dump_trapframe(printf, "\n", tf);
			panic(pstr);
			/*NOTREACHED*/
		}
#endif
		(l->l_proc->p_md.md_syscall)(l, tf);
		userret(l);
		break;

	case TRAPA_DIVZERO:
		tf->tf_state.sf_expevt = T_DIVZERO;
		/*FALLTHROUGH*/

	default:
		trap(l, tf);
		break;
	}
}

void
panic_trap(struct trapframe *tf, register_t ssr, register_t spc,
    register_t expevt)
{
	struct exc_scratch_frame excf;
	struct cpu_info *ci = curcpu();
	register_t tlbregs[9];

	excf = ci->ci_escratch;
	tlbregs[0] = ci->ci_tscratch.ts_r[0];
	tlbregs[1] = ci->ci_tscratch.ts_r[1];
	tlbregs[2] = ci->ci_tscratch.ts_r[2];
	tlbregs[3] = ci->ci_tscratch.ts_r[3];
	tlbregs[4] = ci->ci_tscratch.ts_r[4];
	tlbregs[5] = ci->ci_tscratch.ts_r[5];
	tlbregs[6] = ci->ci_tscratch.ts_r[6];
	tlbregs[7] = ci->ci_tscratch.ts_tr[0];
	tlbregs[8] = ci->ci_tscratch.ts_tr[1];

	{
		register_t sr;

		/* Make sure the FPU is enabled */
		__asm volatile("getcon sr, %0" : "=r"(sr));
		sr &= ~SH5_CONREG_SR_FD;
		__asm volatile("putcon %0, sr" :: "r"(sr));
	}

	/*
	 * Allow subsequent exceptions.
	 */
	ci->ci_escratch.es_critical = 0;

	printf("\n\nPANIC trap: %s in %s mode\n", trap_type((int)expevt),
	    USERMODE(tf) ? "user" : "kernel");

	printf(
	    " SSR=0x%x,  SPC=0x%lx,  EXPEVT=0x%04x, TEA=0x%lx, TRA=0x%x\n",
	    (u_int)ssr, (uintptr_t)spc, (u_int)expevt,
	    (uintptr_t)tf->tf_state.sf_tea, (u_int)tf->tf_state.sf_tra);

	printf("PSSR=0x%08x, PSPC=0x%lx, PEXPEVT=0x%04x\n",
	    (u_int)tf->tf_state.sf_ssr, (uintptr_t)tf->tf_state.sf_spc,
	    (u_int)tf->tf_state.sf_expevt);

	tf->tf_state.sf_ssr = ssr;
	tf->tf_state.sf_spc = spc;
	tf->tf_state.sf_expevt = expevt;

	printf("Exc Scratch Area:\n");
	printf("  CRIT: 0x%x\n", (u_int)excf.es_critical);
	printf("    R0: 0x%08x%08x\n",
	    (u_int)(excf.es_r[0] >> 32), (u_int)excf.es_r[0]);
	printf("    R1: 0x%08x%08x\n",
	    (u_int)(excf.es_r[1] >> 32), (u_int)excf.es_r[1]);
	printf("    R2: 0x%08x%08x\n",
	    (u_int)(excf.es_r[2] >> 32), (u_int)excf.es_r[2]);
	printf("   R15: 0x%08x%08x\n",
	    (u_int)(excf.es_r15 >> 32), (u_int)excf.es_r15);
	printf("   TR0: 0x%08x%08x\n",
	    (u_int)(excf.es_tr0 >> 32), (u_int)excf.es_tr0);
	printf("EXPEVT: 0x%04x\n", (u_int)excf.es_expevt);
	printf("INTEVT: 0x%04x\n", (u_int)excf.es_intevt);
	printf("   TEA: 0x%08x%08x\n",
	    (u_int)(excf.es_tea >> 32), (u_int)excf.es_tea);
	printf("   TRA: 0x%04x\n", (u_int)excf.es_tra);
	printf("   SPC: 0x%08x%08x\n",
	    (u_int)(excf.es_spc >> 32), (u_int)excf.es_spc);
	printf("   SSR: 0x%08x\n", (u_int)excf.es_ssr);
	printf("   USR: 0x%04x\n", (u_int)excf.es_usr);

	printf("\nTLB Scratch Area:\n");
	printf("    R0: 0x%08x%08x\n",
	    (u_int)(tlbregs[0] >> 32), (u_int)tlbregs[0]);
	printf("    R1: 0x%08x%08x\n",
	    (u_int)(tlbregs[1] >> 32), (u_int)tlbregs[1]);
	printf("    R2: 0x%08x%08x\n",
	    (u_int)(tlbregs[2] >> 32), (u_int)tlbregs[2]);
	printf("    R3: 0x%08x%08x\n",
	    (u_int)(tlbregs[3] >> 32), (u_int)tlbregs[3]);
	printf("    R4: 0x%08x%08x\n",
	    (u_int)(tlbregs[4] >> 32), (u_int)tlbregs[4]);
	printf("    R5: 0x%08x%08x\n",
	    (u_int)(tlbregs[5] >> 32), (u_int)tlbregs[5]);
	printf("    R6: 0x%08x%08x\n",
	    (u_int)(tlbregs[6] >> 32), (u_int)tlbregs[6]);
	printf("   TR0: 0x%08x%08x\n",
	    (u_int)(tlbregs[7] >> 32), (u_int)tlbregs[7]);
	printf("   TR1: 0x%08x%08x\n",
	    (u_int)(tlbregs[8] >> 32), (u_int)tlbregs[8]);

#ifdef DDB
	dump_trapframe(printf, "\n", tf);
	kdb_trap(0, tf);
#endif
	panic("panic trap");
}


#ifdef PORTMASTER
/*
 * Called from exception.S when we get an exception inside the critical
 * section of another exception.
 */
void panic_critical_fault(struct trapframe *, struct exc_scratch_frame *,
    void *, register_t);
void
panic_critical_fault(struct trapframe *tf, struct exc_scratch_frame *es,
    void *errloc, register_t ownerid)
{
	struct exc_scratch_frame excf;

	excf = *es;

	printf("\nFAULT IN CRITICAL SECTION!\n");

	printf("Detected at address: %p, Current owner: 0x%x\n",
	    errloc, (u_int)ownerid);

	printf("SSR=0x%08x, SPC=0x%lx, TEA=0x%lx, EXPEVT=0x%x, INTEVT=0x%x\n\n",
	    (u_int)tf->tf_state.sf_ssr, (uintptr_t)tf->tf_state.sf_spc,
	    (uintptr_t)tf->tf_state.sf_tea, (u_int)tf->tf_state.sf_expevt,
	    (u_int)tf->tf_state.sf_intevt);

	printf("Original Fault State: Exception ");

	if ((ownerid & CRIT_EXIT) == 0) {
		printf("entry.\n");
		printf("Exc Scratch Area:\n");
		printf("  CRIT: 0x%02x\n", (u_int)excf.es_critical);
		printf("    R0: 0x%08x%08x\n",
		    (u_int)(excf.es_r[0] >> 32), (u_int)excf.es_r[0]);
		printf("    R1: 0x%08x%08x\n",
		    (u_int)(excf.es_r[1] >> 32), (u_int)excf.es_r[1]);
		printf("    R2: 0x%08x%08x\n",
		    (u_int)(excf.es_r[2] >> 32), (u_int)excf.es_r[2]);
		printf("   R15: 0x%08x%08x\n",
		    (u_int)(excf.es_r15 >> 32), (u_int)excf.es_r15);
		printf("   TR0: 0x%08x%08x\n",
		    (u_int)(excf.es_tr0 >> 32), (u_int)excf.es_tr0);
		printf("EXPEVT: 0x%04x\n", (u_int)excf.es_expevt);
		printf("INTEVT: 0x%04x\n", (u_int)excf.es_intevt);
		printf("   TEA: 0x%08x%08x\n",
		    (u_int)(excf.es_tea >> 32), (u_int)excf.es_tea);
		printf("   TRA: 0x%04x\n", (u_int)excf.es_tra);
		printf("   SPC: 0x%08x%08x\n",
		    (u_int)(excf.es_spc >> 32), (u_int)excf.es_spc);
		printf("   SSR: 0x%08x\n", (u_int)excf.es_ssr);
		printf("   USR: 0x%04x\n\n", (u_int)excf.es_usr);
	} else
		printf("exit.\nNot much to show.\n");

#ifdef DIAGNOSTIC
	dump_trapframe(printf, "\n", tf);
#endif
#ifdef DDB
	kdb_trap(0, tf);
#endif

	panic("panic_critical_fault");
}
#endif	/* PORTMASTER */

const char *
trap_type(int traptype)
{
	const char *t;

	switch (traptype & ~T_USER) {
	case T_READPROT:
		t = "Read Data Protection Violation";
		break;
	case T_WRITEPROT:
		t = "Write Data Protection Violation";
		break;
	case T_RADDERR:
		t = "Read Data Address Error";
		break;
	case T_WADDERR:
		t = "Write Data Address Error";
		break;
	case T_FPUEXC:
		t = "FPU Exception";
		break;
	case T_TRAP:
		t = "Syscall Trap";
		break;
	case T_RESINST:
		t = "Illegal Instruction";
		break;
	case T_ILLSLOT:
		t = "Illegal Slot Exception";
		break;
	case T_FPUDIS:
		t = "FPU Disabled";
		break;
	case T_SLOTFPUDIS:
		t = "Delay Slot FPU Disabled";
		break;
	case T_EXECPROT:
		t = "Instruction Protection Violation";
		break;
	case T_IADDERR:
		t = "Instruction Address Error";
		break;
	case T_DEBUGIA:
		t = "Instruction Address Debug";
		break;
	case T_DEBUGIV:
		t = "Instruction Value Debug";
		break;
	case T_BREAK:
		t = "Breakpoint";
		break;
	case T_DEBUGOA:
		t = "Operand Address Debug";
		break;
	case T_DEBUGSS:
		t = "Single Step Debug";
		break;
	case T_RTLBMISS:
		t = "Data Read TLB Miss";
		break;
	case T_WTLBMISS:
		t = "Data Write TLB Miss";
		break;
	case T_ITLBMISS:
		t = "Instruction TLB Miss";
		break;
	case T_AST:
		t = "AST";
		break;
	case T_NMI:
		t = "NMI";
		break;
	case T_DIVZERO:
		t = "Integer Divide by Zero";
		break;
	default:
		t = "Unknown Exception";
		break;
	}

	return (t);
}

#if defined(DIAGNOSTIC) || defined(DDB)
void
dump_trapframe(void (*pr)(const char *, ...), const char *prefix,
    struct trapframe *tf)
{
	const char fmt[] = "%s=0x%08x%08x%s";
	const char comma[] = ", ";

	(*pr)(prefix);

#define	print_a_reg(reg, v, nl) (*pr)(fmt, (reg), \
	    (u_int)((v) >> 32), (u_int)(v), (nl) ? prefix : comma)

	print_a_reg(" r0", tf->tf_caller.r0, 0);
	print_a_reg(" r1", tf->tf_caller.r1, 0);
	print_a_reg(" r2", tf->tf_caller.r2, 1);

	print_a_reg(" r3", tf->tf_caller.r3, 0);
	print_a_reg(" r4", tf->tf_caller.r4, 0);
	print_a_reg(" r5", tf->tf_caller.r5, 1);

	print_a_reg(" r6", tf->tf_caller.r6, 0);
	print_a_reg(" r7", tf->tf_caller.r7, 0);
	print_a_reg(" r8", tf->tf_caller.r8, 1);

	print_a_reg(" r9", tf->tf_caller.r9, 0);
	print_a_reg("r10", tf->tf_caller.r10, 0);
	print_a_reg("r11", tf->tf_caller.r11, 1);

	print_a_reg("r12", tf->tf_caller.r12, 0);
	print_a_reg("r13", tf->tf_caller.r13, 0);
	print_a_reg("r14", tf->tf_caller.r14, 1);

	print_a_reg("r15", tf->tf_caller.r15, 0);
	print_a_reg("r16", tf->tf_caller.r16, 0);
	print_a_reg("r17", tf->tf_caller.r17, 1);

	print_a_reg("r18", tf->tf_caller.r18, 0);
	print_a_reg("r19", tf->tf_caller.r19, 0);
	print_a_reg("r20", tf->tf_caller.r20, 1);

	print_a_reg("r21", tf->tf_caller.r21, 0);
	print_a_reg("r22", tf->tf_caller.r22, 0);
	print_a_reg("r23", tf->tf_caller.r23, 1);

	print_a_reg("r24", (register_t)0, 0);
	print_a_reg("r25", tf->tf_caller.r25, 0);
	print_a_reg("r26", tf->tf_caller.r26, 1);

	print_a_reg("r27", tf->tf_caller.r27, 0);
	if (tf->tf_state.sf_flags & SF_FLAGS_CALLEE_SAVED) {
		print_a_reg("r28", tf->tf_callee.r28, 0);
		print_a_reg("r29", tf->tf_callee.r29, 1);

		print_a_reg("r30", tf->tf_callee.r30, 0);
		print_a_reg("r31", tf->tf_callee.r31, 0);
		print_a_reg("r32", tf->tf_callee.r32, 1);

		print_a_reg("r33", tf->tf_callee.r33, 0);
		print_a_reg("r34", tf->tf_callee.r34, 0);
		print_a_reg("r35", tf->tf_callee.r35, 1);
	} else
		(*pr)(prefix);

	print_a_reg("r36", tf->tf_caller.r36, 0);
	print_a_reg("r37", tf->tf_caller.r37, 0);
	print_a_reg("r38", tf->tf_caller.r38, 1);

	print_a_reg("r39", tf->tf_caller.r39, 0);
	print_a_reg("r40", tf->tf_caller.r40, 0);
	print_a_reg("r41", tf->tf_caller.r41, 1);

	print_a_reg("r42", tf->tf_caller.r42, 0);
	print_a_reg("r43", tf->tf_caller.r43, 0);
	if (tf->tf_state.sf_flags & SF_FLAGS_CALLEE_SAVED) {
		print_a_reg("r44", tf->tf_callee.r44, 1);

		print_a_reg("r45", tf->tf_callee.r45, 0);
		print_a_reg("r46", tf->tf_callee.r46, 0);
		print_a_reg("r47", tf->tf_callee.r47, 1);

		print_a_reg("r48", tf->tf_callee.r48, 0);
		print_a_reg("r49", tf->tf_callee.r49, 0);
		print_a_reg("r50", tf->tf_callee.r50, 1);

		print_a_reg("r51", tf->tf_callee.r51, 0);
		print_a_reg("r52", tf->tf_callee.r52, 0);
		print_a_reg("r53", tf->tf_callee.r53, 1);

		print_a_reg("r54", tf->tf_callee.r54, 0);
		print_a_reg("r55", tf->tf_callee.r55, 0);
		print_a_reg("r56", tf->tf_callee.r56, 1);

		print_a_reg("r57", tf->tf_callee.r57, 0);
		print_a_reg("r58", tf->tf_callee.r58, 0);
		print_a_reg("r59", tf->tf_callee.r59, 1);
	} else
		(*pr)(prefix);

	print_a_reg("r60", tf->tf_caller.r60, 0);
	print_a_reg("r61", tf->tf_caller.r61, 0);
	print_a_reg("r62", tf->tf_caller.r62, 1);

	(*pr)(prefix);

	print_a_reg("tr0", tf->tf_caller.tr0, 0);
	print_a_reg("tr1", tf->tf_caller.tr1, 0);
	print_a_reg("tr2", tf->tf_caller.tr2, 1);

	print_a_reg("tr3", tf->tf_caller.tr3, 0);
	print_a_reg("tr4", tf->tf_caller.tr4, 0);
	if (tf->tf_state.sf_flags & SF_FLAGS_CALLEE_SAVED) {
		print_a_reg("tr5", tf->tf_callee.tr5, 1);

		print_a_reg("tr6", tf->tf_callee.tr6, 0);
		print_a_reg("tr7", tf->tf_callee.tr7, 0);
	}

	(*pr)("\n");
#undef print_a_reg
}
#endif


#ifdef PORTMASTER
/*
 * Called from Ltrapexit in exception.S just before we restore the
 * trapframe in order to verify that the trapframe is valid enough
 * not to cause untold bogosity when we restore context from it.
 */
void validate_trapframe(struct trapframe *tf);
void
validate_trapframe(struct trapframe *tf)
{
	extern char etext[];

	if ((tf->tf_state.sf_ssr & SH5_CONREG_SR_FD) != 0) {
		printf("oops: trapframe with FPU off!\n");
		goto boom;
	}

	if ((tf->tf_state.sf_ssr & SH5_CONREG_SR_MMU) == 0) {
		printf("oops: trapframe with MMU off!\n");
		goto boom;
	}

	if ((tf->tf_state.sf_ssr & SH5_CONREG_SR_BL) != 0) {
		printf("oops: trapframe with Exceptions Blocked!\n");
		goto boom;
	}

	if ((tf->tf_state.sf_ssr & SH5_CONREG_SR_WATCH) != 0) {
		printf("oops: trapframe with watchpoint set!\n");
		goto boom;
	}

	if ((tf->tf_state.sf_ssr & SH5_CONREG_SR_STEP) != 0) {
		printf("oops: trapframe with single-step set!\n");
		goto boom;
	}

	if ((tf->tf_state.sf_spc & 3) != 1) {
		printf("oops: trapframe with non-SHmedia SPC!\n");
		goto boom;
	}

	if (USERMODE(tf)) {
		if ((tf->tf_state.sf_ssr & SH5_CONREG_SR_IMASK_ALL) != 0) {
			printf("oops: return to usermode with non-zero ipl\n");
			goto boom;
		}

		if (tf->tf_state.sf_spc >= SH5_KSEG0_BASE) {
			printf("oops: usermode PC is in kernel space!\n");
			goto boom;
		}
	} else {
		if (tf->tf_state.sf_spc < SH5_KSEG0_BASE) {
			printf("oops: kernelmode PC is in user space!\n");
			goto boom;
		}

		if ((u_long)tf->tf_state.sf_spc > (u_long)etext) {
			printf("oops: kernelmode PC outside text segment!\n");
			goto boom;
		}
	}

	return;

boom:
	printf("oops: current trapframe address: %p\n", tf);
	dump_trapframe(printf, "\n", tf);
#ifdef DDB
	kdb_trap(0, tf);
#endif
	panic("validate_trapframe");
}
#endif	/* PORTMASTER */
