/*	$NetBSD: trap.c,v 1.1 2002/07/05 13:32:07 scw Exp $	*/

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
 * Copyright (c) 1988 University of Utah.
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

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>

#include <machine/cpu.h>
#include <machine/trap.h>
#include <machine/pmap.h>


/* Used to trap faults while probing */
label_t *onfault;

void
userret(struct proc *p)
{
	int sig;

	while ((sig = CURSIG(p)) != 0)
		postsig(sig);
	p->p_priority = p->p_usrpri;
	if (curcpu()->ci_want_resched) {
		preempt(NULL);
		while ((sig = CURSIG(p)) != 0)
			postsig(sig);
	}

	curcpu()->ci_schedstate.spc_curpriority = p->p_priority;
}

/*
 * Handle synchronous exceptions
 */
void
trap(struct proc *p, struct trapframe *tf)
{
	u_int traptype;
	vaddr_t vaddr;
	vm_prot_t ftype;
	int sig = 0;
	u_long ucode = 0;

	uvmexp.traps++;

	traptype = tf->tf_state.sf_expevt;
	if (USERMODE(tf)) {
		traptype |= T_USER;
		p->p_md.md_regs = tf;
	}

	vaddr = (vaddr_t) tf->tf_state.sf_tea;

	switch (traptype) {
	default:
	dopanic:
		(void) splhigh();
		printf("trap: %s in %s mode\n",
		    trap_type(traptype), USERMODE(tf) ? "user" : "kernel");
		printf("SSR=0x%x, SPC=0x%lx, TEA=0x%lx, TRA=0x%x\n",
			(u_int)tf->tf_state.sf_ssr,
			(uintptr_t)tf->tf_state.sf_spc,
			(uintptr_t)tf->tf_state.sf_tea,
			(u_int)tf->tf_state.sf_tra);
		if (curproc != NULL)
			printf("pid=%d cmd=%s, usp=0x%lx ",
			    p->p_pid, p->p_comm, (uintptr_t)tf->tf_caller.r15);
		else
			printf("curproc == NULL ");
		printf("ksp=0x%lx\n", (vaddr_t)tf);
#if defined(DDB)
		kdb_trap(traptype, tf);
#else
		panic("trap");
#endif
		/* NOTREACHED */

	case T_EXECPROT|T_USER:
	case T_READPROT|T_USER:
		sig = SIGSEGV;
		ucode = vaddr;
		break;

	case T_IADDERR|T_USER:
	case T_RADDERR|T_USER:
	case T_WADDERR|T_USER:
		sig = SIGBUS;
		ucode = vaddr;
		break;

	case T_WRITEPROT:
	case T_WRITEPROT|T_USER:
		/*
		 * This could be caused by tracking page Modification
		 * status, where managed read/write pages are initially
		 * mapped read-only in the pmap module.
		 */
		if (pmap_write_trap(USERMODE(tf), vaddr)) {
			if (traptype & T_USER)
				userret(p);
			return;
		}

		ftype = VM_PROT_WRITE;

		/*
		 * The page really is read-only. Let UVM deal with it.
		 */
		if ((traptype & T_USER) == 0)
			goto kernelfault;
		goto pagefault;

	case T_RTLBMISS:
	case T_WTLBMISS:
	case T_ITLBMISS:
		ftype = (traptype == T_WTLBMISS) ? VM_PROT_WRITE : VM_PROT_READ;
		if (vaddr >= VM_MIN_KERNEL_ADDRESS)
			goto kernelfault;

		if (p->p_addr->u_pcb.pcb_onfault == NULL)
			goto dopanic;
		goto pagefault;

	case T_RTLBMISS|T_USER:
	case T_ITLBMISS|T_USER:
		ftype = VM_PROT_READ;
		goto pagefault;

	case T_WTLBMISS|T_USER:
		ftype = VM_PROT_WRITE;

	pagefault:
	    {
		vaddr_t va;
		struct vmspace *vm;
		struct vm_map *map;
		int rv;

		vm = p->p_vmspace;
		map = &vm->vm_map;
		va = trunc_page(vaddr);
		rv = uvm_fault(map, va, 0, ftype);

		/*
		 * If this was a stack access we keep track of the maximum
		 * accessed stack size.  Also, if vm_fault gets a protection
		 * failure it is due to accessing the stack region outside
		 * the current limit and we need to reflect that as an access
		 * error.
		 */
		if ((caddr_t)va >= vm->vm_maxsaddr) {
			if (rv == 0) {
				unsigned long nss;

				nss = btoc(USRSTACK - (unsigned long)va);
				if (nss > vm->vm_ssize)
					vm->vm_ssize = nss;
			} else
			if (rv == EACCES)
				rv = EFAULT;
		}

		if (rv == 0) {
			if (traptype && T_USER)
				userret(p);
			return;
		}

		if ((traptype & T_USER) == 0)
			goto copyfault;

		if (rv == ENOMEM) {
			printf("UVM: pid %d (%s), uid %d killed: out of swap\n",
			    p->p_pid, p->p_comm,
			    (p->p_cred && p->p_ucred) ?
			    p->p_ucred->cr_uid : -1);
			sig = SIGKILL;
		} else
			sig = (rv = EACCES) ? SIGBUS : SIGSEGV;
		ucode = vaddr;
		break;
	    }

	kernelfault:
	    {
		vaddr_t va;
		int rv;

		va = trunc_page(vaddr);
		rv = uvm_fault(kernel_map, va, 0, ftype);
		if (rv == 0)
			return;
		/*FALLTHROUGH*/
	    }

	case T_RADDERR:
	case T_WADDERR:
	case T_READPROT:
		if (onfault)
			longjmp(onfault);
		/*FALLTHROUGH*/

	copyfault:
		if (p->p_addr->u_pcb.pcb_onfault == NULL)
			goto dopanic;
		tf->tf_state.sf_spc =
		    (register_t)(uintptr_t)p->p_addr->u_pcb.pcb_onfault;
		return;

	case T_BREAK|T_USER:
	case T_RESINST|T_USER:
	case T_ILLSLOT|T_USER:
	case T_FPUDIS|T_USER:
	case T_SLOTFPUDIS|T_USER:
		sig = SIGILL;
		ucode = vaddr;
		break;

	case T_FPUEXC|T_USER:
		sig = SIGILL;
		ucode = vaddr;
		break;

	case T_AST|T_USER:
		if (p->p_flag & P_OWEUPC) {
			p->p_flag &= ~P_OWEUPC;
			ADDUPROF(p);
		}
		userret(p);
		return;
	}

	trapsignal(p, sig, ucode);
	userret(p);
}

/*
 * Handle "TRAPA"-induced syncronous exceptions
 */
void
trapa(struct proc *p, struct trapframe *tf)
{
	u_int trapcode;

#ifdef SIMULATED_CLOCK
	if (!USERMODE(tf) && tf->tf_state.sf_tra == 0) {
		hardclock((struct clockframe *)&tf->tf_state);
		tf->tf_state.sf_spc += 4;	/* Skip over the trapa */
		return;
	}
#endif

#ifdef DIAGNOSTIC
	const char *pstr;

	if (!USERMODE(tf)) {
		pstr = "trapa: TRAPA in kernel mode!";
trapa_panic:
		printf("trapa: SPC=0x%lx, SSR=0x%x, TRA=0x%x\n\n",
		    (uintptr_t)tf->tf_state.sf_spc,
		    (u_int)tf->tf_state.sf_ssr, (u_int)tf->tf_state.sf_tra);
		panic(pstr);
		/*NOTREACHED*/
	}

	if (p == NULL) {
		pstr = "trapa: NULL process!";
		goto trapa_panic;
	}
#endif

	uvmexp.traps++;

	p->p_md.md_regs = tf;
	trapcode = tf->tf_state.sf_tra;

	switch (trapcode) {
	case TRAPA_SYSCALL:
		(p->p_md.md_syscall)(p, tf);
		tf->tf_state.sf_spc += 4;	/* Skip over the trapa */
		break;

	default:
		trapsignal(p, SIGILL, (u_long) tf->tf_state.sf_spc);
		break;
	}

	userret(p);
}

/* We can't deal with these yet; they're invoked with the MMU disabled ... */
#if 0
void
panic_trap(struct cpu_info *ci, struct trapframe *tf)
{

	printf("PANIC trap: %s in %s mode\n",
	    trap_type((int)tf->tf_state.sf_expevt),
	    USERMODE(tf) ? "user" : "kernel");

	printf("SSR=0x%x, SPC=0x%lx, TEA=0x%lx, TRA=0x%x\n\n",
	    (u_int)tf->tf_state.sf_ssr, (uintptr_t)tf->tf_state.sf_spc,
	    (uintptr_t)tf->tf_state.sf_tea, (u_int)tf->tf_state.sf_tra);

	panic("panic_trap");
}
#endif

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
	default:
		t = "Unknown Exception";
		break;
	}

	return (t);
}
