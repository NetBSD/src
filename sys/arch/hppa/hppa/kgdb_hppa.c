/*	$NetBSD: kgdb_hppa.c,v 1.3 2003/08/07 16:27:51 agc Exp $	*/

/*
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratories.
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
 *	@(#)kgdb_stub.c	8.4 (Berkeley) 1/12/94
 */

/*
 * Machine-dependent (hppa) part of the KGDB remote "stub"
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kgdb_hppa.c,v 1.3 2003/08/07 16:27:51 agc Exp $");

#include <sys/param.h>
#include <sys/kgdb.h>

#include <machine/frame.h>
#include <machine/trap.h>

/*
 * Determine if the memory at va..(va+len) is valid.
 */
int
kgdb_acc(va, ulen)
	vaddr_t va;
	size_t ulen;
{

	/* Just let the trap handler deal with it. */
	return (1);
}

/*
 * Translate a trap number into a unix compatible signal value.
 * (gdb only understands unix signal numbers).
 */
int 
kgdb_signal(type)
	int type;
{
	int sigval;

	switch (type) {

	case T_HPMC:
	case T_POWERFAIL:
	case T_LPMC:
	case T_INTERRUPT:
		sigval = SIGINT;
		break;

	case T_NONEXIST:
	case T_ILLEGAL:
	case T_PRIV_OP:
	case T_PRIV_REG:
	case T_IPROT:
		sigval = SIGILL;
		break;

	case T_IBREAK:
	case T_DBREAK:
	case T_TAKENBR:
	case T_RECOVERY:
		sigval = SIGTRAP;
		break;

	case T_EMULATION:
		sigval = SIGEMT;
		break;

	case T_DATALIGN:
		sigval = SIGBUS;
		break;

	case T_DATACC:
	case T_DATAPID:
	case T_ITLBMISS:
	case T_DTLBMISS:
	case T_ITLBMISSNA:
	case T_DTLBMISSNA:
	case T_DPROT:
		sigval = SIGSEGV;
		break;

#if 0
	case T_OVERFLOW:	/* overflow */
	case T_CONDITION:	/* conditional */
	case T_EXCEPTION:	/* assist exception */
	case T_TLB_DIRTY:	/* TLB dirty bit */ 
	case T_PAGEREF:		/* page reference */
	case T_HIGHERPL:	/* higher-privelege transfer */
	case T_LOWERPL:		/* lower-privilege transfer */
#endif
	default:
		sigval = SIGILL;
		break;
	}
	return (sigval);
}

/*
 * Definitions exported from gdb.
 */

/*
 * Translate the values stored in the kernel regs struct to/from
 * the format understood by gdb.
 *
 * When configured for the PA, GDB is set up to expect a buffer 
 * of registers in the HP/UX struct save_state format, described
 * in HP/UX's machine/save_state.h header.  The register order is 
 * very different from our struct trapframe, so we have to do some
 * moving around of values.  
 *
 * The constants in the macro below should correspond to the
 * register numbers in gdb's config/pa/tm-pa.h register macros.
 */
#define KGDB_MOVEREGS						\
	/* 0 is the "save state flags", which gdb doesn't use */	\
	KGDB_MOVEREG(1, tf_r1);					\
	KGDB_MOVEREG(2, tf_rp);          /* r2 */		\
	KGDB_MOVEREG(3, tf_r3);          /* frame pointer when -g */	\
	KGDB_MOVEREG(4, tf_r4);					\
	KGDB_MOVEREG(5, tf_r5);					\
	KGDB_MOVEREG(6, tf_r6);					\
	KGDB_MOVEREG(7, tf_r7);					\
	KGDB_MOVEREG(8, tf_r8);					\
	KGDB_MOVEREG(9, tf_r9);					\
	KGDB_MOVEREG(10, tf_r10);				\
	KGDB_MOVEREG(11, tf_r11);				\
	KGDB_MOVEREG(12, tf_r12);				\
	KGDB_MOVEREG(13, tf_r13);				\
	KGDB_MOVEREG(14, tf_r14);				\
	KGDB_MOVEREG(15, tf_r15);				\
	KGDB_MOVEREG(16, tf_r16);				\
	KGDB_MOVEREG(17, tf_r17);				\
	KGDB_MOVEREG(18, tf_r18);				\
	KGDB_MOVEREG(19, tf_t4);	/* r19 */		\
	KGDB_MOVEREG(20, tf_t3);	/* r20 */		\
	KGDB_MOVEREG(21, tf_t2);	/* r21 */		\
	KGDB_MOVEREG(22, tf_t1);	/* r22 */		\
	KGDB_MOVEREG(23, tf_arg3);	/* r23 */		\
	KGDB_MOVEREG(24, tf_arg2);	/* r24 */		\
	KGDB_MOVEREG(25, tf_arg1);	/* r25 */		\
	KGDB_MOVEREG(26, tf_arg0);	/* r26 */		\
	KGDB_MOVEREG(27, tf_dp);	/* r27 */		\
	KGDB_MOVEREG(28, tf_ret0);	/* r28 */		\
	KGDB_MOVEREG(29, tf_ret1);	/* r29 */		\
	KGDB_MOVEREG(30, tf_sp);	/* r30 */		\
	KGDB_MOVEREG(31, tf_r31);				\
	KGDB_MOVEREG(32, tf_sar);	/* cr11 */		\
	KGDB_MOVEREG(33, tf_iioq_head);	/* cr18 */		\
	KGDB_MOVEREG(34, tf_iisq_head);	/* cr17 */		\
	KGDB_MOVEREG(35, tf_iioq_tail);				\
	KGDB_MOVEREG(36, tf_iisq_tail);				\
	KGDB_MOVEREG(37, tf_eiem);	/* cr15 */		\
	KGDB_MOVEREG(38, tf_iir);	/* cr19 */		\
	KGDB_MOVEREG(39, tf_isr);	/* cr20 */		\
	KGDB_MOVEREG(40, tf_ior);	/* cr21 */		\
	KGDB_MOVEREG(41, tf_ipsw);	/* cr22 */		\
	/* 42 should be cr31, which we don't have available */	\
	KGDB_MOVEREG(43, tf_sr4);				\
	KGDB_MOVEREG(44, tf_sr0);				\
	KGDB_MOVEREG(45, tf_sr1);				\
	KGDB_MOVEREG(46, tf_sr2);				\
	KGDB_MOVEREG(47, tf_sr3);				\
	KGDB_MOVEREG(48, tf_sr5);				\
	KGDB_MOVEREG(49, tf_sr6);				\
	KGDB_MOVEREG(50, tf_sr7);				\
	KGDB_MOVEREG(51, tf_rctr);	/* cr0 */		\
	KGDB_MOVEREG(52, tf_pidr1);	/* cr8 */		\
	KGDB_MOVEREG(53, tf_pidr2);	/* cr9 */		\
	KGDB_MOVEREG(54, tf_ccr);	/* cr10 */		\
	KGDB_MOVEREG(55, tf_pidr3);	/* cr12 */		\
	KGDB_MOVEREG(56, tf_pidr4);	/* cr13 */		\
	KGDB_MOVEREG(57, tf_hptm);	/* cr24 - DDB */	\
	KGDB_MOVEREG(58, tf_vtop);	/* cr25 - DDB */	\
	/* 59 should be cr26, which we don't have available */	\
	/* 60 should be cr27, which we don't have available */	\
	KGDB_MOVEREG(61, tf_cr28);	/*      - DDB */	\
	/* 62 should be cr29, which we don't have available */	\
	KGDB_MOVEREG(63, tf_cr30)	/* uaddr */

void
kgdb_getregs(regs, gdb_regs)
	db_regs_t *regs;
	kgdb_reg_t *gdb_regs;
{
#define	KGDB_MOVEREG(i, f) gdb_regs[i] = regs->f
	KGDB_MOVEREGS;
#undef	KGDB_MOVEREG
}

void
kgdb_setregs(regs, gdb_regs)
	db_regs_t *regs;
	kgdb_reg_t *gdb_regs;
{
#define	KGDB_MOVEREG(i, f) regs->f = gdb_regs[i]
	KGDB_MOVEREGS;
#undef	KGDB_MOVEREG
}

/*
 * Trap into kgdb to wait for debugger to connect,
 * noting on the console why nothing else is going on.
 */
void
kgdb_connect(verbose)
	int verbose;
{

	if (kgdb_dev < 0)
		return;

	if (verbose)
		printf("kgdb waiting...");

	__asm __volatile ("break        %0, %1"
  		:: "i" (HPPA_BREAK_KERNEL), "i" (HPPA_BREAK_KGDB));

	if (verbose)
		printf("connected.\n");

	kgdb_debug_panic = 1;
}

/* 
 * Decide what to do on panic.
 * (This is called by panic, like Debugger())
 */
void
kgdb_panic()
{ 
	if (kgdb_dev >= 0 && kgdb_debug_panic) {
		printf("entering kgdb\n");
		kgdb_connect(kgdb_active == 0);
	}
}
