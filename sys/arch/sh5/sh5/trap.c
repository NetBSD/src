/*	$NetBSD: trap.c,v 1.10 2002/09/04 14:34:01 scw Exp $	*/

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

#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>

#include <machine/cpu.h>
#include <machine/trap.h>
#include <machine/pmap.h>

#ifdef DDB
#include <machine/db_machdep.h>
#endif

#ifdef DIAGNOSTIC
static void dump_trapframe(struct trapframe *);
static void print_a_reg(const char *, register_t, int);
#endif


/* Used to trap faults while probing */
label_t *onfault;


#ifdef DEBUG
int sh5_syscall_debug;
int sh5_trap_debug;
#endif

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

	p->p_md.md_flags &= ~MDP_FPSAVED;
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

#ifdef DEBUG
	static register_t last_tea, last_expevt;
	static int last_count;
#endif

	uvmexp.traps++;

	traptype = tf->tf_state.sf_expevt;
	if (USERMODE(tf)) {
		KDASSERT(p != NULL);
		traptype |= T_USER;
		p->p_md.md_regs = tf;
	} else
	if (p == NULL)
		p = &proc0;

	vaddr = (vaddr_t) tf->tf_state.sf_tea;

#ifdef DEBUG
	if (last_tea == tf->tf_state.sf_tea &&
	    last_expevt == tf->tf_state.sf_expevt) {
		if (last_count++ == 10) {
			printf("Repetitive fault. curvsid = 0x%x\n",
			    curcpu()->ci_curvsid);
			printf("\ntrap: %s in %s mode\n", trap_type(traptype),
			    USERMODE(tf) ? "user" : "kernel");
			printf("SSR=0x%x, SPC=0x%lx, TEA=0x%lx, TRA=0x%x\n",
				(u_int)tf->tf_state.sf_ssr,
				(uintptr_t)tf->tf_state.sf_spc,
				(uintptr_t)tf->tf_state.sf_tea,
				(u_int)tf->tf_state.sf_tra);
			kdb_trap(traptype, tf);
			last_count = 0;
		}
	} else
		last_count = 0;

	last_tea = tf->tf_state.sf_tea;
	last_expevt = tf->tf_state.sf_expevt;
#endif

	switch (traptype) {
	default:
	dopanic:
		(void) splhigh();
		printf("\ntrap: %s in %s mode\n",
		    trap_type(traptype), USERMODE(tf) ? "user" : "kernel");
		printf("SSR=0x%x, SPC=0x%lx, TEA=0x%lx, TRA=0x%x\n",
			(u_int)tf->tf_state.sf_ssr,
			(uintptr_t)tf->tf_state.sf_spc,
			(uintptr_t)tf->tf_state.sf_tea,
			(u_int)tf->tf_state.sf_tra);
		if (p != NULL)
			printf("pid=%d cmd=%s, usp=0x%lx ",
			    p->p_pid, p->p_comm, (uintptr_t)tf->tf_caller.r15);
		else
			printf("no process context ");
		printf("ksp=0x%lx\n", (vaddr_t)tf);
#if defined(DDB)
		kdb_trap(traptype, tf);
#else
#ifdef DIAGNOSTIC
		dump_trapframe(tf);
#endif
#endif
		panic("trap");
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
			if (traptype & T_USER)
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
#ifdef DIAGNOSTIC
	const char *pstr;
#endif

#ifdef DDB
	/*
	 * Kernel breakpoints use "trapa r63".
	 *
	 * XXX: May want to change this to "illegal" in order to avoid
	 * polluting the syscall() path.
	 */
	if (!USERMODE(tf) && tf->tf_state.sf_tra == 0) {
		if (kdb_trap(T_BREAK, tf))
			return;
	}
#endif

#ifdef DEBUG
	if (sh5_syscall_debug) {
		printf("trapa: TRAPA in %s mode ",
		    USERMODE(tf) ? "user" : "kernel");
		if (p != NULL)
			printf("pid=%d cmd=%s, usp=0x%lx\n",
			    p->p_pid, p->p_comm, (uintptr_t)tf->tf_caller.r15);
		else
			printf("curproc == NULL ");
		printf("trapa: SPC=0x%lx, SSR=0x%x, TRA=0x%x, R0=%d\n",
		    (uintptr_t)tf->tf_state.sf_spc, (u_int)tf->tf_state.sf_ssr,
		    (u_int)tf->tf_state.sf_tra, (u_int)tf->tf_caller.r0);
	}
#endif

#ifdef DIAGNOSTIC
	if (!USERMODE(tf)) {
		pstr = "trapa: TRAPA in kernel mode!";
trapa_panic:
		if (p != NULL)
			printf("pid=%d cmd=%s, usp=0x%lx ",
			    p->p_pid, p->p_comm, (uintptr_t)tf->tf_caller.r15);
		else
			printf("curproc == NULL ");
		printf("trapa: SPC=0x%lx, SSR=0x%x, TRA=0x%x\n\n",
		    (uintptr_t)tf->tf_state.sf_spc,
		    (u_int)tf->tf_state.sf_ssr, (u_int)tf->tf_state.sf_tra);
		dump_trapframe(tf);
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
		tf->tf_state.sf_spc += 4;	/* Skip over the trapa */
		(p->p_md.md_syscall)(p, tf);
		break;

	default:
		trapsignal(p, SIGILL, (u_long) tf->tf_state.sf_spc);
		break;
	}

	userret(p);
}

void
panic_trap(struct cpu_info *ci, struct trapframe *tf,
    register_t pssr, register_t pspc)
{

	printf("PANIC trap: %s in %s mode\n",
	    trap_type((int)tf->tf_state.sf_expevt),
	    USERMODE(tf) ? "user" : "kernel");

	printf("PSSR=0x%x, PSPC=0x%lx\n", (u_int)pssr, (u_int)pspc);
	printf("SSR=0x%x, SPC=0x%lx, TEA=0x%lx, TRA=0x%x\n\n",
	    (u_int)tf->tf_state.sf_ssr, (uintptr_t)tf->tf_state.sf_spc,
	    (uintptr_t)tf->tf_state.sf_tea, (u_int)tf->tf_state.sf_tra);

	panic("panic_trap");
}

/*
 * Called from exception.S when we get an exception inside the critical
 * section of another exception.
 */
void panic_critical_fault(struct trapframe *, struct exc_scratch_frame *);
void
panic_critical_fault(struct trapframe *tf, struct exc_scratch_frame *es)
{
	extern int sh5_vector_table;
	uintptr_t vector;

	printf("\nFAULT IN CRITICAL SECTION!\n");

	printf("Post Fault State: %s\n",
	    trap_type((int)tf->tf_state.sf_expevt));

	printf("SSR=0x%x, SPC=0x%lx, TEA=0x%lx\n\n",
	    (u_int)tf->tf_state.sf_ssr, (uintptr_t)tf->tf_state.sf_spc,
	    (uintptr_t)tf->tf_state.sf_tea);

	printf("Pre Fault State: ");

	vector = (uintptr_t)tf->tf_state.sf_spc - (uintptr_t)&sh5_vector_table;
	vector &= ~0xff;

	printf("vector: 0x%lx ", vector);

	switch (vector) {
	case 0x0:	printf("panic handler\n");	/* Can't happen */
			break;

	case 0x100:	printf("General exception handler\n");
			printf("\t%s in %s mode\n",
	    		    trap_type((int)es->es_expevt),
			    (es->es_ssr & SH5_CONREG_SR_MD)?"kernel":"user");
			break;

	case 0x200:	printf("debug interrupt handler\n"); /* Can't happen */
			break;

	case 0x600:	printf("H/W interrupt handler\n");
			break;

	default:	printf("Not sure. Probably Ltlbmiss_dotrap\n");
			break;
	}

	printf("STACKPTR=0x%x, SSR=0x%x, SPC=0x%lx\n",
	    (u_int)tf->tf_caller.r15, (u_int)es->es_ssr, (uintptr_t)es->es_spc);
	printf("TEA=0x%lx, TRA=0x%x, INTEVT=0x%lx\n\n",
	    (uintptr_t)es->es_tea, (u_int)es->es_tra, (u_int)es->es_intevt);

	printf("Exception temporaries:\n");
	printf("r0=0x%08x%08x, r1=0x%08x%08x, r2=0x%08x%08x\n",
	    (u_int)(tf->tf_caller.r0 >> 32), (u_int)tf->tf_caller.r0,
	    (u_int)(tf->tf_caller.r1 >> 32), (u_int)tf->tf_caller.r1,
	    (u_int)(tf->tf_caller.r2 >> 32), (u_int)tf->tf_caller.r2);

#ifdef DIAGNOSTIC
	printf("\nPre-exception Context:\n");
	tf->tf_caller.r0 = es->es_r[0];
	tf->tf_caller.r1 = es->es_r[1];
	tf->tf_caller.r2 = es->es_r[2];
	tf->tf_caller.r15 = es->es_r15;
	tf->tf_caller.tr0 = es->es_tr0;

	dump_trapframe(tf);
#endif

	panic("panic_critical_fault");
}

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
	default:
		t = "Unknown Exception";
		break;
	}

	return (t);
}

#ifdef DIAGNOSTIC
static void
dump_trapframe(struct trapframe *tf)
{
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

	print_a_reg("r24", 0, 0);
	print_a_reg("r25", tf->tf_caller.r25, 0);
	print_a_reg("r26", tf->tf_caller.r26, 1);

	print_a_reg("r27", tf->tf_caller.r27, 0);
	print_a_reg("r28", tf->tf_callee.r28, 0);
	print_a_reg("r29", tf->tf_callee.r29, 1);

	print_a_reg("r30", tf->tf_callee.r30, 0);
	print_a_reg("r31", tf->tf_callee.r31, 0);
	print_a_reg("r32", tf->tf_callee.r32, 1);

	print_a_reg("r33", tf->tf_callee.r33, 0);
	print_a_reg("r34", tf->tf_callee.r34, 0);
	print_a_reg("r35", tf->tf_callee.r35, 1);

	print_a_reg("r36", tf->tf_caller.r36, 0);
	print_a_reg("r37", tf->tf_caller.r37, 0);
	print_a_reg("r38", tf->tf_caller.r38, 1);

	print_a_reg("r39", tf->tf_caller.r39, 0);
	print_a_reg("r40", tf->tf_caller.r40, 0);
	print_a_reg("r41", tf->tf_caller.r41, 1);

	print_a_reg("r42", tf->tf_caller.r42, 0);
	print_a_reg("r43", tf->tf_caller.r43, 0);
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

	print_a_reg("r60", tf->tf_caller.r60, 0);
	print_a_reg("r61", tf->tf_caller.r61, 0);
	print_a_reg("r62", tf->tf_caller.r62, 1);

	print_a_reg("\ntr0", tf->tf_caller.tr0, 0);
	print_a_reg("tr1", tf->tf_caller.tr1, 0);
	print_a_reg("tr2", tf->tf_caller.tr2, 1);

	print_a_reg("tr3", tf->tf_caller.tr3, 0);
	print_a_reg("tr4", tf->tf_caller.tr4, 0);
	print_a_reg("tr5", tf->tf_callee.tr5, 1);

	print_a_reg("tr6", tf->tf_callee.tr6, 0);
	print_a_reg("tr7", tf->tf_callee.tr7, 1);
}

static void
print_a_reg(const char *reg, register_t v, int nl)
{
	printf("%s=0x%08x%08x%s", reg,
	    (u_int)(v >> 32), (u_int)v, nl ? "\n" : ", ");
}

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
	dump_trapframe(tf);
#ifdef DDB
	kdb_trap(0, tf);
#endif
	panic("validate_trapframe");
}
#endif
