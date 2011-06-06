/*	$NetBSD: kgdb_machdep.c,v 1.14.6.1 2011/06/06 09:06:53 jruoho Exp $ */
/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
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
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgements:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
 *
 * 	This product includes software developed by Harvard University.
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
 *	@(#)kgdb_stub.c	8.1 (Berkeley) 6/11/93
 */

/*
 * Copyright (c) 1995
 * 	The President and Fellows of Harvard College. All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgements:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
 *
 * 	This product includes software developed by Harvard University.
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
 *	@(#)kgdb_stub.c	8.1 (Berkeley) 6/11/93
 */

/*
 * Machine dependent routines needed by kern/kgdb_stub.c
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kgdb_machdep.c,v 1.14.6.1 2011/06/06 09:06:53 jruoho Exp $");

#include "opt_kgdb.h"
#include "opt_multiprocessor.h"
#include "opt_sparc_arch.h"

#ifdef KGDB

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/kgdb.h>

#include <machine/ctlreg.h>
#include <machine/psl.h>
#include <machine/reg.h>
#include <machine/trap.h>
#include <machine/cpu.h>

#include <sparc/sparc/asm.h>

extern int64_t pseg_get(struct pmap *, vaddr_t);

static inline void kgdb_copy(register char *, register char *, register int);
static inline void kgdb_zero(register char *, register int);

/*
 * This little routine exists simply so that bcopy() can be debugged.
 */
static inline void
kgdb_copy(register char *src, register char *dst, register int len)
{

	while (--len >= 0)
		*dst++ = *src++;
}

/* ditto for bzero */
static inline void
kgdb_zero(register char *ptr, register int len)
{
	while (--len >= 0)
		*ptr++ = (char) 0;
}

/*
 * Deal with KGDB in a MP environment. XXX need to have "mach cpu" equiv.
 */
#ifdef MULTIPROCESSOR

#define NOCPU -1

static int kgdb_suspend_others(void);
static void kgdb_resume_others(void);
static void kgdb_suspend(void);

__cpu_simple_lock_t kgdb_lock;
int kgdb_cpu = NOCPU;

static int
kgdb_suspend_others(void)
{
	int cpu_me = cpu_number();
	int win;

	if (cpus == NULL)
		return 1;

	__cpu_simple_lock(&kgdb_lock);
	if (kgdb_cpu == NOCPU)
		kgdb_cpu = cpu_me;
	win = (kgdb_cpu == cpu_me);
	__cpu_simple_unlock(&kgdb_lock);

	if (win)
		mp_pause_cpus();

	return win;
}

static void
kgdb_resume_others(void)
{

	mp_resume_cpus();

	__cpu_simple_lock(&kgdb_lock);
	kgdb_cpu = NOCPU;
	__cpu_simple_unlock(&kgdb_lock);
}

static void
kgdb_suspend(void)
{

	sparc64_ipi_pause_thiscpu(NULL);
}
#endif	/* MULTIPROCESSOR */

/*
 * Trap into kgdb to wait for debugger to connect,
 * noting on the console why nothing else is going on.
 */
void
kgdb_connect(int verbose)
{

	if (kgdb_dev == NODEV)
		return;
#if NFB > 0
	fb_unblank();
#endif
#ifdef MULTIPROCESSOR
	/* While we're in the debugger, pause all other CPUs */
	if (!kgdb_suspend_others()) {
		kgdb_suspend();
	} else {
#endif	/* MULTIPROCESSOR */
		if (verbose)
			printf("kgdb waiting...");
		__asm("ta %0" :: "n" (T_KGDB_EXEC));	/* trap into kgdb */

		kgdb_debug_panic = 1;

#ifdef MULTIPROCESSOR
		/* Other CPUs can continue now */
		kgdb_resume_others();
	}
#endif	/* MULTIPROCESSOR */
}

/*
 * Decide what to do on panic.
 */
void
kgdb_panic(void)
{

	if (kgdb_dev != NODEV && kgdb_debug_panic)
		kgdb_connect(kgdb_active == 0);
}

/*
 * Translate a trap number into a unix compatible signal value.
 * (gdb only understands unix signal numbers).
 * XXX should this be done at the other end?
 */
int
kgdb_signal(int type)
{
	int sigval;

	switch (type) {

	case T_AST:
		sigval = SIGINT;
		break;

	case T_TEXTFAULT:
	case T_DATAFAULT:
		sigval = SIGSEGV;
		break;

	case T_ALIGN:
		sigval = SIGBUS;
		break;

	case T_ILLINST:
	case T_PRIVINST:
	case T_DIV0:
		sigval = SIGILL;
		break;

	case T_FP_IEEE_754:
	case T_FP_OTHER:
		sigval = SIGFPE;
		break;

	case T_BREAKPOINT:
		sigval = SIGTRAP;
		break;

	case T_KGDB_EXEC:
		sigval = SIGIOT;
		break;

	default:
		sigval = SIGEMT;
		break;
	}
	return (sigval);
}

/*
 * Definitions exported from gdb (& then made prettier).
 * (see gnu/dist/toolchain/gdb/config/sparc/tm-sp64.h)
 */
#define	GDB_G0		0
#define	GDB_O0		8
#define	GDB_L0		16
#define	GDB_I0		24
#define	GDB_FP0		32
#define GDB_PC		80
#define GDB_NPC		81
#define GDB_CCR		82
#define	GDB_FSR		83
#define	GDB_FPRS	84
#define	GDB_Y		85
#define	GDB_ASI		86

#define REGISTER_BYTES		(KGDB_NUMREGS * 8)
#define REGISTER_BYTE(n)	((n) * 8)

/*
 * Translate the values stored in the kernel regs struct to the format
 * understood by gdb.
 */
void
kgdb_getregs(db_regs_t *regs, kgdb_reg_t *gdb_regs)
{
	struct trapframe64 *tf = &regs->db_tf;

	/* %g0..%g7 and %o0..%o7: from trapframe */
	gdb_regs[0] = 0;
	kgdb_copy((void *)&tf->tf_global[1], (void *)&gdb_regs[1], 15 * 8);

	/* %l0..%l7 and %i0..%i7: from stack */
	kgdb_copy((void *)(long)tf->tf_out[6], (void *)&gdb_regs[GDB_L0], 16 * 8);

	/* %f0..%f31 -- fake, kernel does not use FP */
	kgdb_zero((void *)&gdb_regs[GDB_FP0], 32 * 8);

	/* %y, %psr, %wim, %tbr, %pc, %npc, %fsr, %csr */
	gdb_regs[GDB_PC] = tf->tf_pc;
	gdb_regs[GDB_NPC] = tf->tf_npc;
}

/*
 * Reverse the above.
 */
void
kgdb_setregs(db_regs_t *regs, kgdb_reg_t *gdb_regs)
{
	struct trapframe64 *tf = &regs->db_tf;

	kgdb_copy((void *)&gdb_regs[1], (void *)&tf->tf_global[1], 15 * 8);
	kgdb_copy((void *)&gdb_regs[GDB_L0], (void *)(long)tf->tf_out[6], 16 * 8);
	tf->tf_pc = gdb_regs[GDB_PC];
	tf->tf_npc = gdb_regs[GDB_NPC];
}

/*
 * Determine if memory at [va..(va+len)] is valid.
 */
int
kgdb_acc(vaddr_t va, size_t len)
{
	int64_t data;
	vaddr_t eva;
	struct pmap *pm = &kernel_pmap_;

	eva = round_page(va + len);
	va = trunc_page(va);

	mutex_enter(&pm->pm_lock);
	for (; va < eva; va += PAGE_SIZE) {
		data = pseg_get(pm, va);
		if ((data & TLB_V) == 0) {
			mutex_exit(&pm->pm_lock);
			return 0;
		}
	}
	mutex_exit(&pm->pm_lock);

	return (1);
}
#endif
