/* $NetBSD: except.c,v 1.1 2002/03/24 15:46:45 bjh21 Exp $ */
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

__KERNEL_RCSID(0, "$NetBSD: except.c,v 1.1 2002/03/24 15:46:45 bjh21 Exp $");

#include "opt_cputypes.h"
#include "opt_ddb.h"
#include "opt_ktrace.h"

#include <sys/errno.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <sys/systm.h>
#include <sys/user.h>

#include <uvm/uvm_extern.h>

#include <arm/armreg.h>
#include <machine/intr.h>
#include <machine/machdep.h>
#include <machine/pcb.h>

#ifdef DEBUG
#include <arch/arm/arm/disassem.h>
#endif

#ifdef DDB
#include <machine/db_machdep.h>
#endif

#ifdef KTRACE
#include <sys/ktrace.h>
#endif

void syscall(struct trapframe *);
static void do_fault(struct trapframe *, struct proc *, struct vm_map *,
    vaddr_t, vm_prot_t);
static void data_abort_fixup(struct trapframe *);
static vaddr_t data_abort_address(struct trapframe *, vsize_t *);
static vm_prot_t data_abort_atype(struct trapframe *);
static boolean_t data_abort_usrmode(struct trapframe *);
#ifdef DEBUG
static void printregs(struct trapframe *tf);
#endif
#ifdef DIAGNOSTIC
void checkvectors(void);
#endif

int want_resched;

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
prefetch_abort_handler(struct trapframe *tf)
{
	vaddr_t pc;
	struct proc *p;

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

	do_fault(tf, p, &p->p_vmspace->vm_map, pc, VM_PROT_EXECUTE);

	userret(p);
}

void
data_abort_handler(struct trapframe *tf)
{
	vaddr_t pc, va;
	vsize_t asize;
	struct proc *p;
	vm_prot_t atype;
	boolean_t usrmode, twopages;
	struct vm_map *map;

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
	pc = tf->tf_r15 & R15_PC;
	data_abort_fixup(tf);
	va = data_abort_address(tf, &asize);
	atype = data_abort_atype(tf);
	usrmode = data_abort_usrmode(tf);
	twopages = (trunc_page(va) != round_page(va + asize) - PAGE_SIZE);
	if (!usrmode && va >= VM_MIN_KERNEL_ADDRESS)
		map = kernel_map;
	else
		map = &p->p_vmspace->vm_map;
	do_fault(tf, p, map, va, atype);
	if (twopages)
		do_fault(tf, p, map, va + asize - 4, atype);

	if ((tf->tf_r15 & R15_MODE) == R15_MODE_USR)
		userret(p);
}

/*
 * General page fault handler.
 */
void
do_fault(struct trapframe *tf, struct proc *p,
    struct vm_map *map, vaddr_t va, vm_prot_t atype)
{
	int error;
	struct pcb *curpcb;

	if (pmap_fault(map->pmap, va, atype))
		return;

	KASSERT(current_intr_depth == 0);

	for (;;) {
		error = uvm_fault(map, va, 0, atype);
		if (error != ENOMEM)
			break;
		log(LOG_WARNING, "pid %d: VM shortage, sleeping\n", p->p_pid);
		tsleep(&lbolt, PVM, "abtretry", 0);
	}

	if (error != 0) {
		curpcb = &p->p_addr->u_pcb;
		if (curpcb->pcb_onfault != NULL) {
			tf->tf_r0 = error;
			tf->tf_r15 = (tf->tf_r15 & ~R15_PC) |
			    (register_t)curpcb->pcb_onfault;
			return;
		}
		trapsignal(p, SIGSEGV, va);
	}
}

/*
 * In order for the following macro to work, any function using it
 * must ensure that tf->r15 is copied into getreg(15).  This is safe
 * with the current trapframe layout on arm26, but be careful.
 */
#define getreg(r) (((register_t *)&tf->tf_r0)[r])

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

	getreg(15) = tf->tf_r15;
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
data_abort_address(struct trapframe *tf, vsize_t *vsp)
{
	register_t insn;
	int rn, rm, offset, shift, p, i, u;
	vaddr_t base;

	getreg(15) = tf->tf_r15;
	/* Get the faulting instruction */
       	insn = *(register_t *)(tf->tf_r15 & R15_PC);
	if ((insn & 0x0c000000) == 0x04000000) {
		/* Single data transfer */
		*vsp = 1; /* or 4, but it doesn't really matter */
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
		int loop, count;

		/* LDM/STM */
		rn = (insn >> 16) & 0x0f;
		p = insn & 1 << 24;
		u = insn & 1 << 23;
		/* Count registers transferred */
		count = 0;
		for (loop = 0; loop < 16; ++loop)
			if (insn & (1<<loop))
				++count;
		*vsp = count * 4;
		/* Need to find both ends of the range being transferred. */
		if (u == 0) /* up/down bit */
			if (p == 0) /* pre/post bit */
				return getreg(rn) - *vsp + 4;	/* ...DA */
			else
				return getreg(rn) - *vsp;	/* ...DB */
		else
			if (p == 0)
				return getreg(rn);		/* ...IA */
			else
				return getreg(rn) + 4;		/* ...IB */
#if defined(CPU_ARM250) || defined(CPU_ARM3)
	} else if ((insn & 0x0fb00ff0) == 0x01000090) {
		/* SWP */
		*vsp = 1; /* or 4, but who cares? */
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
		return VM_PROT_WRITE;
	/* STM or CDT instruction ? */
	else if ((insn & 0x0a100000) == 0x08000000)
		return VM_PROT_WRITE;
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

	trapsignal(p, SIGBUS, pc);
	userret(p);
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
