/* $NetBSD: except.c,v 1.21 2000/12/27 16:57:09 bjh21 Exp $ */
/*-
 * Copyright (c) 1998, 1999, 2000 Ben Harris
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
 *    derived from this software without specific prior written permission.
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
/* This file is part of NetBSD/arm26 -- a port of NetBSD to ARM2/3 machines. */
/*
 * except.c -- ARM exception handling.
 */

#include <sys/param.h>

__KERNEL_RCSID(0, "$NetBSD: except.c,v 1.21 2000/12/27 16:57:09 bjh21 Exp $");

#include "opt_cputypes.h"
#include "opt_ddb.h"
#include "opt_ktrace.h"
#include "opt_syscall_debug.h"

#include <sys/kernel.h>
#include <sys/syscall.h>
#include <sys/syslog.h>
#include <sys/systm.h>
#include <sys/user.h>

#include <uvm/uvm_extern.h>

#include <machine/armreg.h>
#include <machine/intr.h>
#include <machine/machdep.h>
#include <machine/pcb.h>

#ifdef DEBUG
#include <arm32/arm32/disassem.h>
#endif

#ifdef DDB
#include <machine/db_machdep.h>
#endif

#ifdef KTRACE
#include <sys/ktrace.h>
#endif

void syscall(struct trapframe *);
static void data_abort_fixup(struct trapframe *);
static vaddr_t data_abort_address(struct trapframe *);
static vm_prot_t data_abort_atype(struct trapframe *);
static boolean_t data_abort_usrmode(struct trapframe *);
#ifdef DEBUG
static void printregs(struct trapframe *tf);
#endif
#ifdef DIAGNOSTIC
void checkvectors(void);
#endif

int want_resched;

/*
 * userret() is called just before any return to userland after a trap.
 * It's all boilerplate stuff.
 */

static void
userret(struct proc *p, vaddr_t pc, u_quad_t oticks)
{
	int sig;

	/* take pending signals */
	while ((sig = CURSIG(p)) != 0)
		postsig(sig);
	p->p_priority = p->p_usrpri;
	if (want_resched) {
		/*
		 * We are being preempted.
		 */
		preempt(NULL);
		while ((sig = CURSIG(p)) != 0)
			postsig(sig);
	}

	/*
	 * If profiling, charge system time to the trapped pc.
	 */
	if (p->p_flag & P_PROFIL) {
		extern int psratio;

		addupc_task(p, pc, (int)(p->p_sticks - oticks) * psratio);
	}
	curcpu()->ci_schedstate.spc_curpriority = p->p_priority;
#ifdef DIAGNOSTIC
	/* Mark trapframe as invalid. */
	p->p_addr->u_pcb.pcb_tf = (void *)-1;
	checkvectors();
#endif
}

#ifdef DIAGNOSTIC
void
checkvectors()
{
	u_int32_t *ptr;

	/* Check that the vectors are valid */
	for (ptr = (u_int32_t *)0; ptr < (u_int32_t *)0x1c; ptr++)
		if (*ptr != 0xe59ff114)
			panic("CPU vectors mangled");
}
#endif


void
undefined_handler(struct trapframe *tf)
{
	u_quad_t sticks;
	struct proc *p;
	u_int32_t insn;
	vaddr_t pc;

	pc = tf->tf_r15 & R15_PC;
	insn =  *(register_t *)pc;
#ifdef CPU_ARM2
	/*
	 * Check if the aborted instruction was a SWI (ARM2 bug --
	 * ARM3 data sheet p87) and call SWI handler if so.
	 */
	if ((insn & 0x0f000000) == 0x0f000000) {
		swi_handler(tf);
		return;
	}
#endif
	/* Enable interrupts if they were enabled before the trap. */
	if ((tf->tf_r15 & R15_IRQ_DISABLE) == 0)
		int_on();
	uvmexp.traps++;
	p = curproc;
	if (p == NULL)
		p = &proc0;
	if (p->p_addr->u_pcb.pcb_onundef_lj != NULL)
		longjmp(p->p_addr->u_pcb.pcb_onundef_lj);
	if ((tf->tf_r15 & R15_MODE) != R15_MODE_USR) {
#ifdef DDB
		if (insn == 0xe7ffffff) {
			kdb_trap(T_BREAKPOINT, (db_regs_t *)tf);
			return;
		}
#endif
#ifdef DEBUG
		printf("Undefined instruction:\n");
		printregs(tf);
		printf("pc -> ");
		disassemble(tf->tf_r15 & R15_PC);
#endif
		panic("undefined instruction in kernel mode");
	} else {
		p->p_addr->u_pcb.pcb_tf = tf;
		sticks = p->p_sticks;
		trapsignal(p, SIGILL, insn);
		userret(p, pc, sticks);
	}
}

void
swi_handler(struct trapframe *tf)
{

	/* XXX The type of e_syscall is all wrong. */
	(*(void (*)(struct trapframe *))(curproc->p_emul->e_syscall))(tf);
}

void
syscall(struct trapframe *tf)
{
	u_quad_t sticks;
	struct proc *p;
	vaddr_t pc;
	int code, nargs, nregargs, nextreg, nstkargs;
	const struct sysent *sy;
	register_t args[8]; /* XXX just enough for mmap... */
	register_t rval[2], or15;
	int error;

	/* Enable interrupts if they were enabled before the trap. */
	if ((tf->tf_r15 & R15_IRQ_DISABLE) == 0)
		int_on();
	uvmexp.traps++;
	uvmexp.syscalls++;
	p = curproc;
	if (p == NULL)
		p = &proc0;
	sticks = p->p_sticks;
	if ((tf->tf_r15 & R15_MODE) == R15_MODE_USR)
		p->p_addr->u_pcb.pcb_tf = tf;

	if ((tf->tf_r15 & R15_MODE) != R15_MODE_USR) {
#ifdef DEBUG
		printf("SWI:\n");
		printregs(tf);
		printf("pc -> ");
		disassemble(tf->tf_r15 & R15_PC);
#endif
		panic("SWI in kernel mode");
	}

	pc = tf->tf_r15 & R15_PC;

#ifdef DIAGNOSTIC
	if ((*(u_int32_t *)pc & 0x0f000000) != 0x0f000000) {
		disassemble(pc);
		panic("SWI on non-SWI instruction");
	}
#endif

	/* Look up the system call and see if it's magic. */
	code = *(register_t *)pc & 0x00ffffff;
	switch (code) {
	case SYS_syscall: /* Indirect system call.  First arg is new code */
		code = tf->tf_r0;
		nregargs = 3; nextreg = 1;
		break;
	case SYS___syscall: /* As above, but quad_t arg */
		if (p->p_emul->e_sysent == sysent) { /* NetBSD emulation */
			code = tf->tf_r0; /* XXX assume little-endian */
			nregargs = 2; nextreg = 2;
			break;
		}
		/* FALLTHROUGH */
	default:
		nregargs = 4; nextreg = 0;
	}
	if (code > p->p_emul->e_nsysent)
		sy = p->p_emul->e_sysent + p->p_emul->e_nosys;
	else
		sy = p->p_emul->e_sysent + code;

	nargs = sy->sy_argsize / sizeof(register_t);
	nregargs = min(nregargs, nargs);
	nstkargs = nargs - nregargs;

	if (nregargs > 0)
		bcopy((register_t *)tf + nextreg, args,
		    nregargs * sizeof(register_t));

	if (nstkargs > 0) {
		error = copyin((caddr_t)tf->tf_r13, args + nregargs,
			       nstkargs * sizeof(register_t));
		if (error) {
#ifdef SYSCALL_DEBUG
			scdebug_call(p, code, (register_t *)args);
#endif
			goto bad;
		}
	}

#ifdef SYSCALL_DEBUG
	scdebug_call(p, code, args);
#endif
#ifdef KTRACE
	if (KTRPOINT(p, KTR_SYSCALL))
		ktrsyscall(p, code, nargs * sizeof(register_t), args);
#endif

	rval[0] = 0;
	rval[1] = tf->tf_r1;

	/* Set return address */
	or15 = tf->tf_r15;
	tf->tf_r15 += 4;

	error = sy->sy_call(p, args, rval);

	switch (error) {
	case 0:
		tf->tf_r0 = rval[0];
		tf->tf_r1 = rval[1];
		tf->tf_r15 &= ~R15_FLAG_C;
		break;
	case ERESTART:
		tf->tf_r15 = or15;
		break;
	case EJUSTRETURN:
		/* nothing to do */
		break;
	default:
	bad:
		tf->tf_r0 = rval[0] = error;
		tf->tf_r15 |= R15_FLAG_C;
		break;
	}
#ifdef SYSCALL_DEBUG
	scdebug_ret(p, code, error, rval);
#endif
	userret(p, pc, sticks);
#ifdef KTRACE
	if (KTRPOINT(p, KTR_SYSRET))
		ktrsysret(p, code, error, rval[0]);
#endif
}

/*
 * Return from fork(2) in the child.  This is effectively the tail end of
 * a normal successful syscall return.
 */
void
child_return(void *arg)
{
	struct proc *p = arg;
	struct trapframe *tf;

	tf = p->p_addr->u_pcb.pcb_tf;
	tf->tf_r0 = 0;
	tf->tf_r15 &= ~R15_FLAG_C;

#ifdef SYSCALL_DEBUG
	scdebug_ret(p, SYS_fork /* XXX */, 0, &tf->tf_r0);
#endif
	userret(p, tf->tf_r15 & R15_PC, 0);
#ifdef KTRACE
        if (KTRPOINT(p, KTR_SYSRET))
                ktrsysret(p, SYS_fork /* XXX */, 0, tf->tf_r0);
#endif
}

void
prefetch_abort_handler(struct trapframe *tf)
{
	u_quad_t sticks;
	vaddr_t pc;
	struct proc *p;
	int ret;

	/* Enable interrupts if they were enabled before the trap. */
	if ((tf->tf_r15 & R15_IRQ_DISABLE) == 0)
		int_on();

	/*
	 * XXX Not done yet:
	 * Check if the page being requested is already present.  If
	 * so, call the undefined instruction handler instead (ARM3 ds
	 * p15).
	 */

	uvmexp.traps++;
	p = curproc;
	if (p == NULL)
		p = &proc0;
	sticks = p->p_sticks;

	if ((tf->tf_r15 & R15_MODE) == R15_MODE_USR)
		p->p_addr->u_pcb.pcb_tf = tf;

	if ((tf->tf_r15 & R15_MODE) != R15_MODE_USR) {
#ifdef DEBUG
		printf("Prefetch abort:\n");
		printregs(tf);
#endif
		panic("prefetch abort in kernel mode");
	}

	/* User-mode prefetch abort */
	pc = tf->tf_r15 & R15_PC;

	if (pmap_fault(p->p_vmspace->vm_map.pmap, pc, VM_PROT_EXECUTE))
		goto out;
	for (;;) {
		ret = uvm_fault(&p->p_vmspace->vm_map, pc, 0, VM_PROT_EXECUTE);
		if (ret != KERN_RESOURCE_SHORTAGE)
			break;
		log(LOG_WARNING, "pid %d: VM shortage, sleeping\n", p->p_pid);
		tsleep(&lbolt, PVM, "abtretry", 0);
	}

	if (ret != KERN_SUCCESS) {
#ifdef DEBUG
		printf("unhandled fault at %p (ret = %d)\n", (void *)pc, ret);
		printf("Prefetch abort:\n");
		printregs(tf);
#ifdef DDB
		Debugger();
#endif
#endif
		trapsignal(p, SIGSEGV, pc);
	}
out:
	userret(p, pc, sticks);
}

void
data_abort_handler(struct trapframe *tf)
{
	u_quad_t sticks;
	vaddr_t pc, va;
	int ret;
	struct proc *p;
	vm_prot_t atype;
	boolean_t usrmode;
	vm_map_t map;
	struct pcb *curpcb;

	/*
	 * Data aborts in kernel mode are possible (copyout etc), so
	 * we hope the compiler (or programmer) has ensured that
	 * R14_svc gets saved.
	 *
	 * We may need to fix up an STM or LDM instruction.  This
	 * involves seeing if the base was being written back, and if
	 * so resetting it (by counting the number of registers being
	 * transferred) before retrying (ARM 2 ds pp 10 & 33).
	 */

	/* Enable interrupts if they were enabled before the trap. */
	if ((tf->tf_r15 & R15_IRQ_DISABLE) == 0)
		int_on();
	uvmexp.traps++;
	p = curproc;
	if (p == NULL)
		p = &proc0;
	if ((tf->tf_r15 & R15_MODE) == R15_MODE_USR)
		p->p_addr->u_pcb.pcb_tf = tf;
	sticks = p->p_sticks;
	pc = tf->tf_r15 & R15_PC;
	data_abort_fixup(tf);
	va = data_abort_address(tf);
	atype = data_abort_atype(tf);
	usrmode = data_abort_usrmode(tf);
	if (!usrmode && va >= VM_MIN_KERNEL_ADDRESS)
		map = kernel_map;
	else
		map = &p->p_vmspace->vm_map;
	if (pmap_fault(map->pmap, va, atype))
		goto out;
	for (;;) {
		ret = uvm_fault(map, va, 0, atype);
		if (ret != KERN_RESOURCE_SHORTAGE)
			break;
		log(LOG_WARNING, "pid %d: VM shortage, sleeping\n", p->p_pid);
		tsleep(&lbolt, PVM, "abtretry", 0);
	}

	if (ret != KERN_SUCCESS) {
#ifdef DEBUG
		printf("unhandled fault at %p (ret = %d)\n", (void *)va, ret);
		printf("Data abort:\n");
		printregs(tf);
		printf("pc -> ");
		disassemble(tf->tf_r15 & R15_PC);
#ifdef DDB
		Debugger();
#endif
#endif
		curpcb = &p->p_addr->u_pcb;
		if (curpcb->pcb_onfault != NULL) {
			tf->tf_r15 = (tf->tf_r15 & ~R15_PC) |
			    (register_t)curpcb->pcb_onfault;
			return;
		}
		if (curpcb->pcb_onfault_lj != NULL)
			longjmp(curpcb->pcb_onfault_lj);
		trapsignal(p, SIGSEGV, va);
	}

out:
	if ((tf->tf_r15 & R15_MODE) == R15_MODE_USR)
		userret(p, pc, sticks);
}

#define getreg(r) (((register_t *)tf)[r])

/*
 * Undo any effects of the aborted instruction that need to be undone
 * in order for us to restart it.  This is just a case of spotting
 * aborted LDMs and STMs and reversing any base writeback.  This code
 * is derived loosely from the arm32 late-abort fixup.
 */
static void
data_abort_fixup(struct trapframe *tf)
{
	register_t insn;
	int rn, count, loop;

	/* Get the faulting instruction */
       	insn = *(register_t *)(tf->tf_r15 & R15_PC);
	if ((insn & 0x0e000000) == 0x08000000 &&
	    (insn & 1 << 21)) {
		/* LDM/STM with writeback*/
		rn = (insn >> 16) & 0x0f;
		if (rn == 15)
			return; /* No writeback on R15 */
		/* Count registers transferred */
		count = 0;
		for (loop = 0; loop < 16; ++loop) {
			if (insn & (1<<loop))
				++count;
		}
		if (insn & (1 << 23)) /* up/down bit -- we reverse it. */
			getreg(rn) -= count * 4;
		else
			getreg(rn) += count * 4;
	}
}

/*
 * Work out where a data abort happened.  This involves emulating the
 * faulting instruction.  Some of this code is derived from the arm32
 * abort fixup stuff too.
 */
static vaddr_t
data_abort_address(struct trapframe *tf)
{
	register_t insn;
	int rn, rm, offset, shift, p, i, u;
	vaddr_t base;

	/* Get the faulting instruction */
       	insn = *(register_t *)(tf->tf_r15 & R15_PC);
	if ((insn & 0x0c000000) == 0x04000000) {
		/* Single data transfer */
		rn = (insn & 0x000f0000) >> 16;
		base = getreg(rn);
		if (rn == 15)
			base = (base & R15_PC) + 8;
		p = insn & 1 << 24;
		if (p == 0)
			/* Post-indexed, so offset doesn't concern us */
			return base;
		u = insn & 1 << 23;
		i = insn & 1 << 25;
		if (i == 0) {
			/* Immediate offset (str r0, [r1, #42]) */
			offset = insn & 0x00000fff;
			if (u == 0)
				return base - offset;
			else
				return base + offset;
		}
		rm = insn & 0x0000000f;
		offset = getreg(rm);
		if (rm == 15)
			offset += 8;
		if ((insn & 1 << 4) == 0)
			/* immediate shift */
			shift = (insn & 0x00000f80) >> 7;
		else
			goto croak; /* register shifts can't happen in ARMv2 */
		switch ((insn & 0x00000060) >> 5) {
		case 0: /* Logical left */
			offset = (int)(((u_int)offset) << shift);
			break;
		case 1: /* Logical Right */
			if (shift == 0) shift = 32;
			offset = (int)(((u_int)offset) >> shift);
			break;
		case 2: /* Arithmetic Right */
			if (shift == 0) shift = 32;
			offset = (int)(((int)offset) >> shift);
			break;
		case 3: /* Rotate right -- FIXME support this*/
		default: /* help GCC */
			goto croak;
		}
		if (u == 0)
			return base - offset;
		else
			return base + offset;
	} else if ((insn & 0x0e000000) == 0x08000000) {
		vaddr_t sva, eva;
		int loop, count;

		/* LDM/STM */
		rn = (insn >> 16) & 0x0f;
		/* Need to find both ends of the range being transferred. */
		p = insn & 1 << 24;
		u = insn & 1 << 23;
		if (p == 0)
			sva = getreg(rn);
		else
			if (u == 0)
				sva = getreg(rn) - 4;
			else
				sva = getreg(rn) + 4;
		/*
		 * This is currently bogus.  We should check the
		 * lowest address first.  I doubt they'll notice.
		 */
		if (pmap_confess(sva, data_abort_atype(tf)))
			return sva;
		/* Count registers transferred */
		count = 0;
		for (loop = 0; loop < 16; ++loop) {
			if (insn & (1<<loop))
				++count;
		}
		if (u == 0) /* up/down bit */
			eva = sva - count * 4;
		else
			eva = sva + count * 4;
		return eva;
#if defined(CPU_ARM250) || defined(CPU_ARM3)
	} else if ((insn & 0x0fb00ff0) == 0x01000090) {
		/* SWP */
		rm = insn & 0x0000000f;
		base = getreg(rm);
		if (rm == 15)
			return base + 8;
		else
			return base;
#endif
	}
croak:
#ifdef DEBUG
	printf("Data abort:\n");
	printregs(tf);
	printf("pc -> ");
	disassemble(tf->tf_r15 & R15_PC);
#endif
	panic("data_abort_address");
}

/*
 * We need to know whether the page should be mapped as R or R/W.  We
 * need to disassemble the instruction responsible and determine if it
 * was a read or write instruction.  This code is based on the arm32
 * version.
 */
static vm_prot_t
data_abort_atype(struct trapframe *tf)
{
	register_t insn;

	insn = *(register_t *)(tf->tf_r15 & R15_PC);
	/* STR instruction ? */
	if ((insn & 0x0c100000) == 0x04000000)
		return VM_PROT_READ | VM_PROT_WRITE;
	/* STM or CDT instruction ? */
	else if ((insn & 0x0a100000) == 0x08000000)
		return VM_PROT_READ | VM_PROT_WRITE;
#if defined(CPU_ARM250) || defined(CPU_ARM3)
	/* SWP instruction ? */
	else if ((insn & 0x0fb00ff0) == 0x01000090)
		return VM_PROT_READ | VM_PROT_WRITE;
#endif
	return VM_PROT_READ;
}

/*
 * Work out what effective mode was in use when a data abort occurred.
 */
static boolean_t
data_abort_usrmode(struct trapframe *tf)
{
	register_t insn;

	if ((tf->tf_r15 & R15_MODE) == R15_MODE_USR)
		return TRUE;
	insn = *(register_t *)(tf->tf_r15 & R15_PC);
	if ((insn & 0x0d200000) == 0x04200000)
		/* LDR[B]T and STR[B]T */
		return TRUE;
	return FALSE;
}

void
address_exception_handler(struct trapframe *tf)
{
	u_quad_t sticks;
	struct proc *p;
	vaddr_t pc;

	/* Enable interrupts if they were enabled before the trap. */
	if ((tf->tf_r15 & R15_IRQ_DISABLE) == 0)
		int_on();
	uvmexp.traps++;
	p = curproc;
	if (p == NULL)
		p = &proc0;
	if ((tf->tf_r15 & R15_MODE) == R15_MODE_USR)
		p->p_addr->u_pcb.pcb_tf = tf;
	
	pc = tf->tf_r15 & R15_PC;

	if ((tf->tf_r15 & R15_MODE) != R15_MODE_USR) {
#ifdef DEBUG
		printf("Address exception:\n");
		printregs(tf);
		printf("pc -> ");
		disassemble(pc);
#endif
		panic("address exception in kernel mode");
	}

#ifdef DEBUG
	printf("Address exception:\n");
	printregs(tf);
	printf("pc -> ");
	disassemble(pc);
#ifdef DDB
	Debugger();
#endif
#endif
	sticks = p->p_sticks;
	trapsignal(p, SIGBUS, pc);
	userret(p, pc, sticks);
}

/*
 * An AST isn't a real hardware trap, but locore.S makes it look like one
 * for consistency.
 */
int astpending;

void
ast_handler(struct trapframe *tf)
{
	u_quad_t sticks;
	vaddr_t pc;
	struct proc *p;

	/* Enable interrupts if they were enabled before the trap. */
	if ((tf->tf_r15 & R15_IRQ_DISABLE) == 0)
		int_on();
	astpending = 0;
	p = curproc;
	if (p == NULL)
		p = &proc0;
	sticks = p->p_sticks;
	if ((tf->tf_r15 & R15_MODE) == R15_MODE_USR)
		p->p_addr->u_pcb.pcb_tf = tf;
	pc = tf->tf_r15 & R15_PC;

	if (p->p_flag & P_OWEUPC) {
		p->p_flag &= ~P_OWEUPC;
		ADDUPROF(p);
	}

	userret(p, pc, sticks);
}

#ifdef DEBUG
static void
printregs(struct trapframe *tf)
{

	printf("R0  = 0x%08x  R1  = 0x%08x  R2  = 0x%08x  R3  = 0x%08x\n"
	       "R4  = 0x%08x  R5  = 0x%08x  R6  = 0x%08x  R7  = 0x%08x\n"
	       "R8  = 0x%08x  R9  = 0x%08x  R10 = 0x%08x  R11 = 0x%08x\n"
	       "R12 = 0x%08x  R13 = 0x%08x  R14 = 0x%08x  R15 = 0x%08x\n",
	       tf->tf_r0, tf->tf_r1, tf->tf_r2, tf->tf_r3,
	       tf->tf_r4, tf->tf_r5, tf->tf_r6, tf->tf_r7,
	       tf->tf_r8, tf->tf_r9, tf->tf_r10, tf->tf_r11,
	       tf->tf_r12, tf->tf_r13, tf->tf_r14, tf->tf_r15);
}
#endif

/* irq_handler is over in irq.c */
