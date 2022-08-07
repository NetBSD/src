/* $NetBSD: locore.s,v 1.143 2022/08/07 10:12:19 andvar Exp $ */

/*-
 * Copyright (c) 1999, 2000, 2019 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
 * Copyright (c) 1994, 1995, 1996 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

.stabs	__FILE__,100,0,0,kernel_text

#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_multiprocessor.h"
#include "opt_lockdebug.h"
#include "opt_compat_netbsd.h"

#include <machine/asm.h>

__KERNEL_RCSID(0, "$NetBSD: locore.s,v 1.143 2022/08/07 10:12:19 andvar Exp $");

#include "assym.h"

.stabs	__FILE__,132,0,0,kernel_text

	/* don't reorder instructions; paranoia. */
	.set noreorder
	.text

	.macro	bfalse	reg, dst
	beq	\reg, \dst
	.endm

	.macro	btrue	reg, dst
	bne	\reg, \dst
	.endm

/*
 * This is for kvm_mkdb, and should be the address of the beginning
 * of the kernel text segment (not necessarily the same as kernbase).
 */
	EXPORT(kernel_text)
.loc	1 __LINE__
kernel_text:

/*
 * bootstack: a temporary stack, for booting.
 *
 * Extends from 'start' down.
 */
bootstack:

/*
 * locorestart: Kernel start. This is no longer the actual entry
 * point, although jumping to here (the first kernel address) will
 * in fact work just fine.
 *
 * Arguments:
 *	a0 is the first free page frame number (PFN)
 *	a1 is the page table base register (PTBR)
 *	a2 is the bootinfo magic number
 *	a3 is the pointer to the bootinfo structure
 *
 * All arguments are passed to alpha_init().
 */
IMPORT(prom_mapped, 4)
NESTED_NOPROFILE(locorestart,1,0,ra,0,0)
	br	pv,1f
1:	LDGP(pv)

	/* Switch to the boot stack. */
	lda	sp,bootstack

	/* Load KGP with current GP. */
	mov	a0, s0			/* save pfn */
	mov	gp, a0
	call_pal PAL_OSF1_wrkgp		/* clobbers a0, t0, t8-t11 */
	mov	s0, a0			/* restore pfn */

	/*
	 * Call alpha_init() to do pre-main initialization.
	 * alpha_init() gets the arguments we were called with,
	 * which are already in a0, a1, a2, a3, and a4.
	 */
	CALL(alpha_init)

	/* Set up the virtual page table pointer. */
	ldiq	a0, VPTBASE
	call_pal PAL_OSF1_wrvptptr	/* clobbers a0, t0, t8-t11 */

	/*
	 * Switch to lwp0's PCB.
	 */
	lda	a0, lwp0
	ldq	a0, L_MD_PCBPADDR(a0)		/* phys addr of PCB */
	call_pal PAL_OSF1_swpctx	/* clobbers a0, t0, t8-t11, a0 */

	/* PROM is no longer mapped. */
	lda	t0, prom_mapped
	stl	zero, 0(t0)

	/*
	 * We've switched to a new page table base, so invalidate the TLB
	 * and I-stream.  This happens automatically everywhere but here.
	 */
	ldiq	a0, -2				/* TBIA */
	call_pal PAL_OSF1_tbi
	call_pal PAL_imb

	/*
	 * All ready to go!  Call main()!
	 *
	 * We're going to play a little trick there, though.  We are
	 * going to fake our return address as the kthread backstop.
	 * Hitting the backstop will trigger a panic, and we want lwp0
	 * to work like other kthreads in that regard.  We will still
	 * keep the "main returned" backstop here in case something
	 * goes horribly wrong.
	 */
	lda	ra, alpha_kthread_backstop
	jsr	s0, main
	ldgp	gp, 0(s0)

	/* This should never happen. */
	PANIC("main() returned",Lmain_returned_pmsg)
	END(locorestart)

/**************************************************************************/

/*
 * Pull in the PROM interface routines; these are needed for
 * prom printf (while bootstrapping), and for determining the
 * boot device, etc.
 */
#include <alpha/alpha/prom_disp.s>

/**************************************************************************/

/*
 * Pull in the PALcode function stubs.
 */
#include <alpha/alpha/pal.s>

/**************************************************************************/

/**************************************************************************/

#if defined(MULTIPROCESSOR)
/*
 * Pull in the multiprocssor glue.
 */
#include <alpha/alpha/multiproc.s>
#endif /* MULTIPROCESSOR */

/**************************************************************************/

/**************************************************************************/

#if defined(DDB) || defined(KGDB)
/*
 * Pull in debugger glue.
 */
#include <alpha/alpha/debug.s>
#endif /* DDB || KGDB */

/**************************************************************************/

/**************************************************************************/

/*
 * Pull in optimized pmap subroutines.
 */
#include <alpha/alpha/pmap_subr.s>

/**************************************************************************/

/**************************************************************************/

	.text
.stabs	__FILE__,132,0,0,backtolocore1	/* done with includes */
.loc	1 __LINE__
backtolocore1:
/**************************************************************************/

#ifdef COMPAT_16
/*
 * Signal "trampoline" code.
 *
 * The kernel arranges for the handler to be invoked directly.  This
 * trampoline is used only to return from the signal.
 *
 * The stack pointer points to the saved sigcontext.
 */

NESTED_NOPROFILE(sigcode,0,0,ra,0,0)
	mov	sp, a0			/* get pointer to sigcontext */
	CALLSYS_NOERROR(compat_16___sigreturn14)	/* and call sigreturn() with it. */
	mov	v0, a0			/* if that failed, get error code */
	CALLSYS_NOERROR(exit)		/* and call exit() with it. */
XNESTED(esigcode,0)
	END(sigcode)
#endif /* COMPAT_16 */

/**************************************************************************/

/*
 * exception_return: return from trap, exception, or syscall
 */

LEAF(exception_return, 1)			/* XXX should be NESTED */
	br	pv, 1f
1:	LDGP(pv)

	ldq	s1, (FRAME_PS * 8)(sp)		/* s1 = new PSL */
	and	s1, ALPHA_PSL_IPL_MASK, s3	/* s3 = new ipl */

	/* --- BEGIN inline spllower() --- */

	cmpult	s3, ALPHA_PSL_IPL_SOFT_HI, t1	/* new IPL < SOFT_HI? */
	beq	t1, 5f				/* no, can't do AST or SI */
	/* yes */

	/* GET_CURLWP clobbers v0, t0, t8...t11. */
	GET_CURLWP
	mov	v0, s0				/* s0 = curlwp */

2:	/*
	 * Check to see if a soft interrupt is pending.  We need to only
	 * check for soft ints eligible to run at the new IPL.  We generate
	 * the mask of eligible soft ints to run by masking the ssir with:
	 *
	 *	(ALPHA_ALL_SOFTINTS << ((ipl) << 1))
	 *
	 * See alpha_softint_dispatch().
	 */
	ldq	t1, L_CPU(s0)			/* t1 = curlwp->l_cpu */
	ldiq	t2, ALPHA_ALL_SOFTINTS		/* t2 = ALPHA_ALL_SOFTINTS */
	ldq	t1, CPU_INFO_SSIR(t1)		/* t1 = t1->ci_ssir */
	sll	s3, 1, t3			/* t3 = ipl << 1 */
	sll	t2, t3, t2			/* t2 <<= t3 */
	and	t1, t2, t1			/* t1 &= t2 */
	bne	t1, 6f				/* yes */
	/* no */

	/* --- END inline spllower() --- */

	and	s1, ALPHA_PSL_USERMODE, t0	/* are we returning to user? */
	beq	t0, 5f				/* no: just return */
	/* yes */

	/* check for AST */
3:	ldl	t3, L_MD_ASTPENDING(s0)		/* AST pending? */
	bne	t3, 7f				/* yes */
	/* no: headed back to user space */

	/* Enable the FPU based on whether MDLWP_FPACTIVE is set. */
4:	ldq	t2, L_MD_FLAGS(s0)
	cmplt	t2, zero, a0
	call_pal PAL_OSF1_wrfen

	/* restore the registers, and return */
5:	bsr	ra, exception_restore_regs	/* jmp/CALL trashes pv/t12 */
	ldq	ra,(FRAME_RA*8)(sp)
	.set noat
	ldq	at_reg,(FRAME_AT*8)(sp)

	lda	sp,(FRAME_SW_SIZE*8)(sp)
	call_pal PAL_OSF1_rti
	.set at
	/* NOTREACHED */

	/* We've got a softint */
6:	ldiq	a0, ALPHA_PSL_IPL_HIGH
	call_pal PAL_OSF1_swpipl
	mov	v0, s2				/* remember old IPL */
	mov	s3, a0				/* pass new ipl */
	CALL(alpha_softint_dispatch)

	/* SI handled; restore IPL and check again */
	mov	s2, a0
	call_pal PAL_OSF1_swpipl
	br	2b

	/* We've got an AST */
7:	stl	zero, L_MD_ASTPENDING(s0)	/* no AST pending */

	ldiq	a0, ALPHA_PSL_IPL_0		/* drop IPL to zero */
	call_pal PAL_OSF1_swpipl
	mov	v0, s2				/* remember old IPL */

	mov	sp, a0				/* only arg is frame */
	CALL(ast)

	/* AST handled; restore IPL and check again */
	mov	s2, a0
	call_pal PAL_OSF1_swpipl
	br	3b

	END(exception_return)

LEAF(exception_save_regs, 0)
	stq	v0,(FRAME_V0*8)(sp)
	stq	a3,(FRAME_A3*8)(sp)
	stq	a4,(FRAME_A4*8)(sp)
	stq	a5,(FRAME_A5*8)(sp)
	stq	s0,(FRAME_S0*8)(sp)
	stq	s1,(FRAME_S1*8)(sp)
	stq	s2,(FRAME_S2*8)(sp)
	stq	s3,(FRAME_S3*8)(sp)
	stq	s4,(FRAME_S4*8)(sp)
	stq	s5,(FRAME_S5*8)(sp)
	stq	s6,(FRAME_S6*8)(sp)
	stq	t0,(FRAME_T0*8)(sp)
	stq	t1,(FRAME_T1*8)(sp)
	stq	t2,(FRAME_T2*8)(sp)
	stq	t3,(FRAME_T3*8)(sp)
	stq	t4,(FRAME_T4*8)(sp)
	stq	t5,(FRAME_T5*8)(sp)
	stq	t6,(FRAME_T6*8)(sp)
	stq	t7,(FRAME_T7*8)(sp)
	stq	t8,(FRAME_T8*8)(sp)
	stq	t9,(FRAME_T9*8)(sp)
	stq	t10,(FRAME_T10*8)(sp)
	stq	t11,(FRAME_T11*8)(sp)
	stq	t12,(FRAME_T12*8)(sp)
	RET
	END(exception_save_regs)

LEAF(exception_restore_regs, 0)
	ldq	v0,(FRAME_V0*8)(sp)
	ldq	a3,(FRAME_A3*8)(sp)
	ldq	a4,(FRAME_A4*8)(sp)
	ldq	a5,(FRAME_A5*8)(sp)
	ldq	s0,(FRAME_S0*8)(sp)
	ldq	s1,(FRAME_S1*8)(sp)
	ldq	s2,(FRAME_S2*8)(sp)
	ldq	s3,(FRAME_S3*8)(sp)
	ldq	s4,(FRAME_S4*8)(sp)
	ldq	s5,(FRAME_S5*8)(sp)
	ldq	s6,(FRAME_S6*8)(sp)
	ldq	t0,(FRAME_T0*8)(sp)
	ldq	t1,(FRAME_T1*8)(sp)
	ldq	t2,(FRAME_T2*8)(sp)
	ldq	t3,(FRAME_T3*8)(sp)
	ldq	t4,(FRAME_T4*8)(sp)
	ldq	t5,(FRAME_T5*8)(sp)
	ldq	t6,(FRAME_T6*8)(sp)
	ldq	t7,(FRAME_T7*8)(sp)
	ldq	t8,(FRAME_T8*8)(sp)
	ldq	t9,(FRAME_T9*8)(sp)
	ldq	t10,(FRAME_T10*8)(sp)
	ldq	t11,(FRAME_T11*8)(sp)
	ldq	t12,(FRAME_T12*8)(sp)
	RET
	END(exception_restore_regs)

/**************************************************************************/

/*
 * XentArith:
 * System arithmetic trap entry point.
 */

	PALVECT(XentArith)		/* setup frame, save registers */

	/* a0, a1, & a2 already set up */
	ldiq	a3, ALPHA_KENTRY_ARITH
	mov	sp, a4			; .loc 1 __LINE__
	CALL(trap)

	jmp	zero, exception_return
	END(XentArith)

/**************************************************************************/

/*
 * XentIF:
 * System instruction fault trap entry point.
 */

	PALVECT(XentIF)			/* setup frame, save registers */

	/* a0, a1, & a2 already set up */
	ldiq	a3, ALPHA_KENTRY_IF
	mov	sp, a4			; .loc 1 __LINE__
	CALL(trap)
	jmp	zero, exception_return	
	END(XentIF)

/**************************************************************************/

/*
 * XentInt:
 * System interrupt entry point.
 */

	PALVECT(XentInt)		/* setup frame, save registers */

	/* a0, a1, & a2 already set up */
	mov	sp, a3			; .loc 1 __LINE__
	CALL(interrupt)
	jmp	zero, exception_return
	END(XentInt)

/**************************************************************************/

/*
 * XentMM:
 * System memory management fault entry point.
 */

	PALVECT(XentMM)			/* setup frame, save registers */

	/* a0, a1, & a2 already set up */
	ldiq	a3, ALPHA_KENTRY_MM
	mov	sp, a4			; .loc 1 __LINE__
	CALL(trap)

	jmp	zero, exception_return
	END(XentMM)

/**************************************************************************/

/*
 * XentSys:
 * System call entry point.
 */

	ESETUP(XentSys)			; .loc 1 __LINE__

	stq	v0,(FRAME_V0*8)(sp)		/* in case we need to restart */
	stq	s0,(FRAME_S0*8)(sp)
	stq	s1,(FRAME_S1*8)(sp)
	stq	s2,(FRAME_S2*8)(sp)
	stq	s3,(FRAME_S3*8)(sp)
	stq	s4,(FRAME_S4*8)(sp)
	stq	s5,(FRAME_S5*8)(sp)
	stq	s6,(FRAME_S6*8)(sp)
	stq	a0,(FRAME_A0*8)(sp)
	stq	a1,(FRAME_A1*8)(sp)
	stq	a2,(FRAME_A2*8)(sp)
	stq	a3,(FRAME_A3*8)(sp)
	stq	a4,(FRAME_A4*8)(sp)
	stq	a5,(FRAME_A5*8)(sp)
	stq	ra,(FRAME_RA*8)(sp)

	/* syscall number, passed in v0, is first arg, frame pointer second */
	mov	v0,a1
	GET_CURLWP
	mov	v0,a0
	mov	sp,a2			; .loc 1 __LINE__
	ldq	t11,L_PROC(a0)
	ldq	t12,P_MD_SYSCALL(t11)
	CALL((t12))

	jmp	zero, exception_return
	END(XentSys)

/**************************************************************************/

/*
 * XentUna:
 * System unaligned access entry point.
 */

LEAF(XentUna, 3)				/* XXX should be NESTED */
	.set noat
	lda	sp,-(FRAME_SW_SIZE*8)(sp)
	stq	at_reg,(FRAME_AT*8)(sp)
	.set at
	stq	ra,(FRAME_RA*8)(sp)
	bsr	ra, exception_save_regs		/* jmp/CALL trashes pv/t12 */

	/* a0, a1, & a2 already set up */
	ldiq	a3, ALPHA_KENTRY_UNA
	mov	sp, a4			; .loc 1 __LINE__
	CALL(trap)

	jmp	zero, exception_return
	END(XentUna)

/**************************************************************************/

/*
 * savefpstate: Save a process's floating point state.
 *
 * Arguments:
 *	a0	'struct fpstate *' to save into
 */

LEAF(savefpstate, 1)
	LDGP(pv)
	/* save all of the FP registers */
	lda	t1, FPREG_FPR_REGS(a0)	/* get address of FP reg. save area */
	stt	$f0,   (0 * 8)(t1)	/* save first register, using hw name */
	stt	$f1,   (1 * 8)(t1)	/* etc. */
	stt	$f2,   (2 * 8)(t1)
	stt	$f3,   (3 * 8)(t1)
	stt	$f4,   (4 * 8)(t1)
	stt	$f5,   (5 * 8)(t1)
	stt	$f6,   (6 * 8)(t1)
	stt	$f7,   (7 * 8)(t1)
	stt	$f8,   (8 * 8)(t1)
	stt	$f9,   (9 * 8)(t1)
	stt	$f10, (10 * 8)(t1)
	stt	$f11, (11 * 8)(t1)
	stt	$f12, (12 * 8)(t1)
	stt	$f13, (13 * 8)(t1)
	stt	$f14, (14 * 8)(t1)
	stt	$f15, (15 * 8)(t1)
	stt	$f16, (16 * 8)(t1)
	stt	$f17, (17 * 8)(t1)
	stt	$f18, (18 * 8)(t1)
	stt	$f19, (19 * 8)(t1)
	stt	$f20, (20 * 8)(t1)
	stt	$f21, (21 * 8)(t1)
	stt	$f22, (22 * 8)(t1)
	stt	$f23, (23 * 8)(t1)
	stt	$f24, (24 * 8)(t1)
	stt	$f25, (25 * 8)(t1)
	stt	$f26, (26 * 8)(t1)
	stt	$f27, (27 * 8)(t1)
	.set noat
	stt	$f28, (28 * 8)(t1)
	.set at
	stt	$f29, (29 * 8)(t1)
	stt	$f30, (30 * 8)(t1)

	/*
	 * Then save the FPCR; note that the necessary 'trapb's are taken
	 * care of on kernel entry and exit.
	 */
	mf_fpcr	ft0
	stt	ft0, FPREG_FPR_CR(a0)	/* store to FPCR save area */

	RET
	END(savefpstate)

/**************************************************************************/

/*
 * restorefpstate: Restore a process's floating point state.
 *
 * Arguments:
 *	a0	'struct fpstate *' to restore from
 */

LEAF(restorefpstate, 1)
	LDGP(pv)
	/*
	 * Restore the FPCR; note that the necessary 'trapb's are taken care of
	 * on kernel entry and exit.
	 */
	ldt	ft0, FPREG_FPR_CR(a0)	/* load from FPCR save area */
	mt_fpcr	ft0

	/* Restore all of the FP registers. */
	lda	t1, FPREG_FPR_REGS(a0)	/* get address of FP reg. save area */
	ldt	$f0,   (0 * 8)(t1)	/* restore first reg., using hw name */
	ldt	$f1,   (1 * 8)(t1)	/* etc. */
	ldt	$f2,   (2 * 8)(t1)
	ldt	$f3,   (3 * 8)(t1)
	ldt	$f4,   (4 * 8)(t1)
	ldt	$f5,   (5 * 8)(t1)
	ldt	$f6,   (6 * 8)(t1)
	ldt	$f7,   (7 * 8)(t1)
	ldt	$f8,   (8 * 8)(t1)
	ldt	$f9,   (9 * 8)(t1)
	ldt	$f10, (10 * 8)(t1)
	ldt	$f11, (11 * 8)(t1)
	ldt	$f12, (12 * 8)(t1)
	ldt	$f13, (13 * 8)(t1)
	ldt	$f14, (14 * 8)(t1)
	ldt	$f15, (15 * 8)(t1)
	ldt	$f16, (16 * 8)(t1)
	ldt	$f17, (17 * 8)(t1)
	ldt	$f18, (18 * 8)(t1)
	ldt	$f19, (19 * 8)(t1)
	ldt	$f20, (20 * 8)(t1)
	ldt	$f21, (21 * 8)(t1)
	ldt	$f22, (22 * 8)(t1)
	ldt	$f23, (23 * 8)(t1)
	ldt	$f24, (24 * 8)(t1)
	ldt	$f25, (25 * 8)(t1)
	ldt	$f26, (26 * 8)(t1)
	ldt	$f27, (27 * 8)(t1)
	ldt	$f28, (28 * 8)(t1)
	ldt	$f29, (29 * 8)(t1)
	ldt	$f30, (30 * 8)(t1)

	RET
	END(restorefpstate)

/**************************************************************************/

/*
 * savectx: save process context, i.e. callee-saved registers
 *
 * Note that savectx() only works for processes other than curlwp,
 * since cpu_switchto will copy over the info saved here.  (It _can_
 * sanely be used for curlwp iff cpu_switchto won't be called again, e.g.
 * if called from boot().)
 *
 * N.B. this is actually only used by dumpsys().
 *
 * Arguments:
 *	a0	'struct pcb *' of the process that needs its context saved
 */

LEAF(savectx, 1)
	br	pv, 1f
1:	LDGP(pv)
	stq	sp, PCB_HWPCB_KSP(a0)		/* store sp */
	stq	s0, PCB_CONTEXT+(0 * 8)(a0)	/* store s0 - s6 */
	stq	s1, PCB_CONTEXT+(1 * 8)(a0)
	stq	s2, PCB_CONTEXT+(2 * 8)(a0)
	stq	s3, PCB_CONTEXT+(3 * 8)(a0)
	stq	s4, PCB_CONTEXT+(4 * 8)(a0)
	stq	s5, PCB_CONTEXT+(5 * 8)(a0)
	stq	s6, PCB_CONTEXT+(6 * 8)(a0)
	stq	ra, PCB_CONTEXT+(7 * 8)(a0)	/* store ra */
	RET
	END(savectx)

/**************************************************************************/

/*
 * void alpha_softint_switchto(struct lwp *current, int ipl, struct lwp *next)
 * Switch away from the current LWP to the specified softint LWP, and
 * dispatch to softint processing.
 * Aguments:
 *	a0	'struct lwp *' of the LWP to switch from
 *	a1	IPL that the softint will run at
 *	a2	'struct lwp *' of the LWP to switch to
 *
 * N.B. We have arranged that a0 and a1 are already set up correctly
 * for the call to softint_dispatch().
 */
NESTED_NOPROFILE(alpha_softint_switchto, 3, 16, ra, IM_RA, 0)
	LDGP(pv)

	ldq	a3, L_PCB(a0)			/* a3 = from->l_pcb */

	lda	sp, -16(sp)			/* set up stack frame */
	stq	ra, 0(sp)			/* save ra */

	/*
	 * Step 1: Save the current LWP's context.  We don't
	 * save the return address directly; instead, we arrange
	 * for it to bounce through a trampoline that fixes up
	 * the state in case the softint LWP blocks.
	 */
	stq	sp, PCB_HWPCB_KSP(a3)		/* store sp */
	stq	s0, PCB_CONTEXT+(0 * 8)(a3)	/* store s0 - s6 */
	stq	s1, PCB_CONTEXT+(1 * 8)(a3)
	stq	s2, PCB_CONTEXT+(2 * 8)(a3)
	stq	s3, PCB_CONTEXT+(3 * 8)(a3)
	stq	s4, PCB_CONTEXT+(4 * 8)(a3)
	stq	s5, PCB_CONTEXT+(5 * 8)(a3)
	stq	s6, PCB_CONTEXT+(6 * 8)(a3)

	/* Set the trampoline address in saved context. */
	lda	v0, alpha_softint_return
	stq	v0, PCB_CONTEXT+(7 * 8)(a3)	/* store ra */

	/*
	 * Step 2: Switch to the softint LWP's stack.
	 * We always start at the top of the stack (i.e.
	 * just below the trapframe).
	 *
	 * N.B. There is no need to restore any other registers
	 * from the softint LWP's context; we are starting from
	 * the root of the call graph.
	 */
	ldq	sp, L_MD_TF(a2)

	/*
	 * Step 3: Update curlwp.
	 *
	 * N.B. We save off the from-LWP argument that will be passed
	 * to softint_dispatch() in s0, which we'll need to restore
	 * before returning.  If we bounce through the trampoline, the
	 * context switch will restore it for us.
	 */
	mov	a0, s0			/* s0 = from LWP */
	SET_CURLWP(a2)			/* clobbers a0, v0, t0, t8..t11 */

	/*
	 * Step 4: Call softint_dispatch().
	 *
	 * N.B. a1 already has the IPL argument.
	 */
	mov	s0, a0			/* a0 = from LWP */
	CALL(softint_dispatch)

	/*
	 * Step 5: Restore everything and return.
	 */
	ldq	a3, L_PCB(s0)			/* a3 = from->l_pcb */
	SET_CURLWP(s0)			/* clobbers a0, v0, t0, t8..t11 */
	ldq	sp, PCB_HWPCB_KSP(a3)		/* restore sp */
	ldq	s0, PCB_CONTEXT+(0 * 8)(a3)	/* restore s0 */
	ldq	ra, 0(sp)			/* restore ra */
	lda	sp, 16(sp)			/* pop stack frame */
	RET
	END(alpha_softint_switchto)

LEAF_NOPROFILE(alpha_softint_return, 0)
	/*
	 * Step 1: Go to IPL_HIGH, which is what the alpha_softint_dispatch()
	 * expects.  We will have arrived here at IPL_SCHED.
	 */
	ldiq	a0, ALPHA_PSL_IPL_HIGH
	call_pal PAL_OSF1_swpipl

	/*
	 * Step 2: Re-adjust the mutex count after mi_switch().
	 */
	GET_CURLWP
	ldq	v0, L_CPU(v0)
	ldl	t0, CPU_INFO_MTX_COUNT(v0)
	addl	t0, 1, t0
	stl	t0, CPU_INFO_MTX_COUNT(v0)

	/*
	 * Step 3: Pop alpha_softint_switchto()'s stack frame
	 * and return.
	 */
	ldq	ra, 0(sp)			/* restore ra */
	lda	sp, 16(sp)			/* pop stack frame */
	RET
	END(alpha_softint_return)

/*
 * struct lwp *cpu_switchto(struct lwp *current, struct lwp *next,
 *                          bool returning)
 * Switch to the specified next LWP
 * Arguments:
 *	a0	'struct lwp *' of the LWP to switch from
 *	a1	'struct lwp *' of the LWP to switch to
 *	a2	non-zero if we're returning to an interrupted LWP
 *		from a soft interrupt
 */
LEAF(cpu_switchto, 0)
	LDGP(pv)

	/*
	 * do an inline savectx(), to save old context
	 */
	ldq	a3, L_PCB(a0)
	/* NOTE: ksp is stored by the swpctx */
	stq	s0, PCB_CONTEXT+(0 * 8)(a3)	/* store s0 - s6 */
	stq	s1, PCB_CONTEXT+(1 * 8)(a3)
	stq	s2, PCB_CONTEXT+(2 * 8)(a3)
	stq	s3, PCB_CONTEXT+(3 * 8)(a3)
	stq	s4, PCB_CONTEXT+(4 * 8)(a3)
	stq	s5, PCB_CONTEXT+(5 * 8)(a3)
	stq	s6, PCB_CONTEXT+(6 * 8)(a3)
	stq	ra, PCB_CONTEXT+(7 * 8)(a3)	/* store ra */

	mov	a0, s4				/* save old curlwp */
	mov	a1, s2				/* save new lwp */

	/*
	 * Check to see if we're doing a light-weight switch back to
	 * an interrupted LWP (referred to as the "pinned" LWP) from
	 * a softint LWP.  In this case we have been running on the
	 * pinned LWP's context -- swpctx was not used to get here --
	 * so we won't be using swpctx to go back, either.
	 */
	bne	a2, 3f			/* yes, go handle it */
	/* no, normal context switch */

	/* Switch to the new PCB. */
	ldq	a0, L_MD_PCBPADDR(s2)
	call_pal PAL_OSF1_swpctx	/* clobbers a0, t0, t8-t11, v0 */

1:	SET_CURLWP(s2)			/* curlwp = l */

	/*
	 * Now running on the new PCB.
	 */
	ldq	s0, L_PCB(s2)

	/*
	 * Check for restartable atomic sequences (RAS).
	 */
	ldq	a0, L_PROC(s2)			/* first ras_lookup() arg */
	ldq	t0, P_RASLIST(a0)		/* any RAS entries? */
	bne	t0, 4f				/* yes, go deal with it */
2:
	mov	s4, v0				/* return the old lwp */
	/*
	 * Restore registers and return.
	 * NOTE: ksp is restored by the swpctx.
	 */
	ldq	s1, PCB_CONTEXT+(1 * 8)(s0)		/* restore s1-s6 */
	ldq	s2, PCB_CONTEXT+(2 * 8)(s0)
	ldq	s3, PCB_CONTEXT+(3 * 8)(s0)
	ldq	s4, PCB_CONTEXT+(4 * 8)(s0)
	ldq	s5, PCB_CONTEXT+(5 * 8)(s0)
	ldq	s6, PCB_CONTEXT+(6 * 8)(s0)
	ldq	ra, PCB_CONTEXT+(7 * 8)(s0)		/* restore ra */
	ldq	s0, PCB_CONTEXT+(0 * 8)(s0)		/* restore s0 */

	RET

3:	/*
	 * Registers right now:
	 *
	 *	a0	old LWP
	 *	a1	new LWP
	 *	a3	old PCB
	 *
	 * What we need to do here is swap the stack, since we won't
	 * be getting that from swpctx.
	 */
	ldq	a2, L_PCB(a1)			/* a2 = new PCB */
	stq	sp, PCB_HWPCB_KSP(a3)		/* save old SP */
	ldq	sp, PCB_HWPCB_KSP(a2)		/* restore new SP */
	br	1b				/* finish up */

4:
	ldq	s1, L_MD_TF(s2)			/* s1 = l->l_md.md_tf */
	ldq	a1, (FRAME_PC*8)(s1)		/* second ras_lookup() arg */
	CALL(ras_lookup)			/* ras_lookup(p, PC) */
	addq	v0, 1, t0			/* -1 means "not in ras" */
	beq	t0, 2b				/* not in ras? return */
	stq	v0, (FRAME_PC*8)(s1)		/* in ras? fix up PC */
	br	2b				/* finish up */

	END(cpu_switchto)

/*
 * lwp_trampoline()
 *
 * Arrange for a function to be invoked neatly, after a cpu_lwp_fork(),
 * which has set up our pcb_context for us.  But we actually *get here*
 * via cpu_switchto(), which returns the LWP we switched away from in v0.
 *
 * Invokes the function specified by the s0 register with the return
 * address specified by the s1 register and with one argument specified
 * by the s2 register.
 */
LEAF_NOPROFILE(lwp_trampoline, 0)
	mov	v0, a0		/* a0 = prev_lwp (from cpu_switchto()) */
	mov	s3, a1		/* a1 = new_lwp (that's us!) */
	CALL(lwp_startup)	/* lwp_startup(prev_lwp, new_lwp); */
	mov	s0, pv		/* pv = func */
	mov	s1, ra		/* ra = (probably exception_return()) */
	mov	s2, a0		/* a0 = arg */
	jmp	zero, (pv)	/* func(arg) */
	END(lwp_trampoline)

/**************************************************************************/

/*
 * alpha_copystr(const void *from, void *to, size_t len, size_t *donep)
 */
	.arch	ev56
LEAF(alpha_copystr_bwx, 4)
	LDGP(pv)

	mov	a2, t0			/* t0 = i = len */
	beq	a2, 5f			/* if (len == 0), bail */

1:	ldbu	t1, 0(a0)		/* t1 = *from */
	subl	a2, 1, a2		/* len-- */
	addq	a0, 1, a0		/* from++ */
	stb	t1, 0(a1)		/* *to = t1 */
	beq	t1, 2f			/* if (t1 == '\0'), bail out */
	addq	a1, 1, a1		/* to++ */
	bne	a2, 1b			/* if (len != 0), copy more */

2:	beq	a3, 3f			/* if (lenp != NULL) */
	subl	t0, a2, t0		/* *lenp = (i - len) */
	stq	t0, 0(a3)
3:	bne	t1, 4f			/* *from != '\0'; leave in a huff */

	mov	zero, v0		/* return 0. */
	RET

4:	ldiq	v0, ENAMETOOLONG
	RET

5:	ldiq	t1, 1			/* fool the test above... */
	br	zero, 2b

	nop				/* pad to same length as... */
	nop				/* non-BWX version. */
	nop
	nop
	nop
	EXPORT(alpha_copystr_bwx_end)
	END(alpha_copystr_bwx)
	.arch	ev4

LEAF(alpha_copystr, 4)
	LDGP(pv)

	mov	a2, t0			/* t0 = i = len */
	beq	a2, 5f			/* if (len == 0), bail */

1:	ldq_u	t1, 0(a0)		/* t1 = *from */
	extbl	t1, a0, t1
	ldq_u	t3, 0(a1)		/* set up t2 with quad around *to */
	insbl	t1, a1, t2
	mskbl	t3, a1, t3
	or	t3, t2, t3		/* add *from to quad around *to */
	stq_u	t3, 0(a1)		/* write out that quad */

	subl	a2, 1, a2		/* len-- */
	beq	t1, 2f			/* if (*from == 0), bail out */
	addq	a1, 1, a1		/* to++ */
	addq	a0, 1, a0		/* from++ */
	bne	a2, 1b			/* if (len != 0) copy more */

2:	beq	a3, 3f			/* if (lenp != NULL) */
	subl	t0, a2, t0		/* *lenp = (i - len) */
	stq	t0, 0(a3)
3:	bne	t1, 4f			/* *from != '\0'; leave in a huff */

	mov	zero, v0		/* return 0. */
	RET

4:	ldiq	v0, ENAMETOOLONG
	RET

5:	ldiq	t1, 1			/* fool the test above... */
	br	zero, 2b
	EXPORT(alpha_copystr_end)
	END(alpha_copystr)

NESTED(copyinstr, 4, 16, ra, IM_RA|IM_S0, 0)
	LDGP(pv)
	lda	sp, -16(sp)			/* set up stack frame	     */
	stq	ra, (16-8)(sp)			/* save ra		     */
	stq	s0, (16-16)(sp)			/* save s0		     */
	ldiq	t0, VM_MAX_ADDRESS		/* make sure that src addr   */
	cmpult	a0, t0, t1			/* is in user space.	     */
	beq	t1, copyerr_efault		/* if it's not, error out.   */
	/* Note: GET_CURLWP clobbers v0, t0, t8...t11. */
	GET_CURLWP
	ldq	s0, L_PCB(v0)			/* s0 = pcb                  */
	lda	v0, copyerr			/* set up fault handler.     */
	stq	v0, PCB_ONFAULT(s0)
	CALL(alpha_copystr)			/* do the copy.		     */
	stq	zero, PCB_ONFAULT(s0)		/* kill the fault handler.   */
	ldq	ra, (16-8)(sp)			/* restore ra.		     */
	ldq	s0, (16-16)(sp)			/* restore s0.		     */
	lda	sp, 16(sp)			/* kill stack frame.	     */
	RET					/* v0 left over from copystr */
	END(copyinstr)

NESTED(copyoutstr, 4, 16, ra, IM_RA|IM_S0, 0)
	LDGP(pv)
	lda	sp, -16(sp)			/* set up stack frame	     */
	stq	ra, (16-8)(sp)			/* save ra		     */
	stq	s0, (16-16)(sp)			/* save s0		     */
	ldiq	t0, VM_MAX_ADDRESS		/* make sure that dest addr  */
	cmpult	a1, t0, t1			/* is in user space.	     */
	beq	t1, copyerr_efault		/* if it's not, error out.   */
	/* Note: GET_CURLWP clobbers v0, t0, t8...t11. */
	GET_CURLWP
	ldq	s0, L_PCB(v0)			/* s0 = pcb                  */
	lda	v0, copyerr			/* set up fault handler.     */
	stq	v0, PCB_ONFAULT(s0)
	CALL(alpha_copystr)			/* do the copy.		     */
	stq	zero, PCB_ONFAULT(s0)		/* kill the fault handler.   */
	ldq	ra, (16-8)(sp)			/* restore ra.		     */
	ldq	s0, (16-16)(sp)			/* restore s0.		     */
	lda	sp, 16(sp)			/* kill stack frame.	     */
	RET					/* v0 left over from copystr */
	END(copyoutstr)

/*
 * kcopy(const void *src, void *dst, size_t len);
 *
 * Copy len bytes from src to dst, aborting if we encounter a fatal
 * page fault.
 *
 * kcopy() _must_ save and restore the old fault handler since it is
 * called by uiomove(), which may be in the path of servicing a non-fatal
 * page fault.
 *
 * N.B. This implementation is a wrapper around memcpy(), which is
 * implemented in src/common/lib/libc/arch/alpha/string/bcopy.S.
 * This is safe ONLY because we know that, as implemented, it is
 * a LEAF function (and thus does not use any callee-saved registers).
 */
NESTED(kcopy, 3, 32, ra, IM_RA|IM_S0|IM_S1, 0)
	LDGP(pv)
	lda	sp, -32(sp)			/* set up stack frame	     */
	stq	ra, (32-8)(sp)			/* save ra		     */
	stq	s0, (32-16)(sp)			/* save s0		     */
	stq	s1, (32-24)(sp)			/* save s1		     */
	/* Swap a0, a1, for call to memcpy(). */
	mov	a1, v0
	mov	a0, a1
	mov	v0, a0
	/* Note: GET_CURLWP clobbers v0, t0, t8...t11. */
	GET_CURLWP
	ldq	s1, L_PCB(v0)			/* s1 = pcb                  */
	lda	v0, kcopyerr			/* set up fault handler.     */
	ldq	s0, PCB_ONFAULT(s1)		/* save old handler.	     */
	stq	v0, PCB_ONFAULT(s1)
	CALL(memcpy)				/* do the copy.		     */
	stq	s0, PCB_ONFAULT(s1)		/* restore the old handler.  */
	ldq	ra, (32-8)(sp)			/* restore ra.		     */
	ldq	s0, (32-16)(sp)			/* restore s0.		     */
	ldq	s1, (32-24)(sp)			/* restore s1.		     */
	lda	sp, 32(sp)			/* kill stack frame.	     */
	mov	zero, v0			/* return 0. */
	RET
	END(kcopy)

LEAF(kcopyerr, 0)
	LDGP(pv)
	stq	s0, PCB_ONFAULT(s1)		/* s1 == pcb (from above)    */
	ldq	ra, (32-8)(sp)			/* restore ra.		     */
	ldq	s0, (32-16)(sp)			/* restore s0.		     */
	ldq	s1, (32-24)(sp)			/* restore s1.		     */
	lda	sp, 32(sp)			/* kill stack frame.	     */
	RET
END(kcopyerr)

NESTED(copyin, 3, 16, ra, IM_RA|IM_S0, 0)
	LDGP(pv)
	lda	sp, -16(sp)			/* set up stack frame	     */
	stq	ra, (16-8)(sp)			/* save ra		     */
	stq	s0, (16-16)(sp)			/* save s0		     */
	ldiq	t0, VM_MAX_ADDRESS		/* make sure that src addr   */
	cmpult	a0, t0, t1			/* is in user space.	     */
	beq	t1, copyerr_efault		/* if it's not, error out.   */
	/* Swap a0, a1, for call to memcpy(). */
	mov	a1, v0
	mov	a0, a1
	mov	v0, a0
	/* Note: GET_CURLWP clobbers v0, t0, t8...t11. */
	GET_CURLWP
	ldq	s0, L_PCB(v0)			/* s = pcb                   */
	lda	v0, copyerr			/* set up fault handler.     */
	stq	v0, PCB_ONFAULT(s0)
	CALL(memcpy)				/* do the copy.		     */
	stq	zero, PCB_ONFAULT(s0)		/* kill the fault handler.   */
	ldq	ra, (16-8)(sp)			/* restore ra.		     */
	ldq	s0, (16-16)(sp)			/* restore s0.		     */
	lda	sp, 16(sp)			/* kill stack frame.	     */
	mov	zero, v0			/* return 0. */
	RET
	END(copyin)

NESTED(copyout, 3, 16, ra, IM_RA|IM_S0, 0)
	LDGP(pv)
	lda	sp, -16(sp)			/* set up stack frame	     */
	stq	ra, (16-8)(sp)			/* save ra		     */
	stq	s0, (16-16)(sp)			/* save s0		     */
	ldiq	t0, VM_MAX_ADDRESS		/* make sure that dest addr  */
	cmpult	a1, t0, t1			/* is in user space.	     */
	beq	t1, copyerr_efault		/* if it's not, error out.   */
	/* Swap a0, a1, for call to memcpy(). */
	mov	a1, v0
	mov	a0, a1
	mov	v0, a0
	/* Note: GET_CURLWP clobbers v0, t0, t8...t11. */
	GET_CURLWP
	ldq	s0, L_PCB(v0)			/* s0 = pcb                  */
	lda	v0, copyerr			/* set up fault handler.     */
	stq	v0, PCB_ONFAULT(s0)
	CALL(memcpy)				/* do the copy.		     */
	stq	zero, PCB_ONFAULT(s0)		/* kill the fault handler.   */
	ldq	ra, (16-8)(sp)			/* restore ra.		     */
	ldq	s0, (16-16)(sp)			/* restore s0.		     */
	lda	sp, 16(sp)			/* kill stack frame.	     */
	mov	zero, v0			/* return 0. */
	RET
	END(copyout)

LEAF(copyerr_efault, 0)
	ldiq	v0, EFAULT			/* return EFAULT.	     */
XLEAF(copyerr, 0)
	LDGP(pv)
	ldq	ra, (16-8)(sp)			/* restore ra.		     */
	ldq	s0, (16-16)(sp)			/* restore s0.		     */
	lda	sp, 16(sp)			/* kill stack frame.	     */
	RET
END(copyerr)

/**************************************************************************/

#define	UFETCHSTORE_PROLOGUE						 \
	br	pv, 1f							;\
1:	LDGP(pv)							;\
	ldiq	t0, VM_MAX_ADDRESS	/* make sure that addr */	;\
	cmpult	a0, t0, t1		/* is in user space. */		;\
	beq	t1, ufetchstoreerr_efault /* if it's not, error out. */

/* LINTSTUB: int _ufetch_8(const uint8_t *uaddr, uint8_t *valp); */
LEAF_NOPROFILE(_ufetch_8, 2)
	UFETCHSTORE_PROLOGUE
.L_ufetch_8_start:
	ldq_u	t0, 0(a0)	/* load quad containing byte */
.L_ufetch_8_end:
	extbl	t0, a0, a0	/* a0 = extracted byte */
	ldq_u	t0, 0(a1)	/* load dest quad */
	insbl	a0, a1, a0	/* a0 = byte in target position */
	mskbl	t0, a1, t0	/* clear target byte in destination */
	or	a0, t0, a0	/* or in byte to destination */
	stq_u	a0, 0(a1)	/* *a1 = fetched byte! */
	mov	zero, v0
	RET
	END(_ufetch_8)

/* LINTSTUB: int _ufetch_16(const uint16_t *uaddr, uint16_t *valp); */
LEAF_NOPROFILE(_ufetch_16, 2)
	UFETCHSTORE_PROLOGUE
.L_ufetch_16_start:
	ldq_u	t0, 0(a0)	/* load quad containing short */
.L_ufetch_16_end:
	extwl	t0, a0, a0	/* a0 = extracted short */
	ldq_u	t0, 0(a1)	/* load dest quad */
	inswl	a0, a1, a0	/* a0 = short in target position */
	mskwl	t0, a1, t0	/* clear target short in destination */
	or	a0, t0, a0	/* or in short to destination */
	stq_u	a0, 0(a1)	/* *a1 = fetched short! */
	mov	zero, v0
	RET
	END(_ufetch_16)

/* LINTSTUB: int _ufetch_32(const uint32_t *uaddr, uint32_t *valp); */
LEAF_NOPROFILE(_ufetch_32, 2)
	UFETCHSTORE_PROLOGUE
.L_ufetch_32_start:
	ldl	v0, 0(a0)
.L_ufetch_32_end:
	stl	v0, 0(a1)
	mov	zero, v0
	RET
	END(_ufetch_32)

/* LINTSTUB: int _ufetch_64(const uint64_t *uaddr, uint64_t *valp); */
LEAF_NOPROFILE(_ufetch_64, 2)
	UFETCHSTORE_PROLOGUE
.L_ufetch_64_start:
	ldq	v0, 0(a0)
.L_ufetch_64_end:
	stq	v0, 0(a1)
	mov	zero, v0
	RET
	END(_ufetch_64)

/* LINTSTUB: int _ustore_8(uint8_t *uaddr, uint8_t val); */
LEAF_NOPROFILE(_ustore_8, 2)
	UFETCHSTORE_PROLOGUE
	zap	a1, 0xfe, a1	/* kill arg's high bytes */
	insbl	a1, a0, a1	/* move it to the right spot */
.L_ustore_8_start:
	ldq_u	t0, 0(a0)	/* load quad around byte */
	mskbl	t0, a0, t0	/* kill the target byte */
	or	t0, a1, a1	/* put the result together */
	stq_u	a1, 0(a0)	/* and store it. */
.L_ustore_8_end:
	mov	zero, v0
	RET
	END(_ustore_8)

/* LINTSTUB: int _ustore_16(uint16_t *uaddr, uint16_t val); */
LEAF_NOPROFILE(_ustore_16, 2)
	UFETCHSTORE_PROLOGUE
	zap	a1, 0xfc, a1	/* kill arg's high bytes */
	inswl	a1, a0, a1	/* move it to the right spot */
.L_ustore_16_start:
	ldq_u	t0, 0(a0)	/* load quad around short */
	mskwl	t0, a0, t0	/* kill the target short */
	or	t0, a1, a1	/* put the result together */
	stq_u	a1, 0(a0)	/* and store it. */
.L_ustore_16_end:
	mov	zero, v0
	RET
	END(_ustore_16)

/* LINTSTUB: int _ustore_32(uint32_t *uaddr, uint32_t val); */
LEAF_NOPROFILE(_ustore_32, 2)
	UFETCHSTORE_PROLOGUE
.L_ustore_32_start:
	stl	a1, 0(a0)
.L_ustore_32_end:
	mov	zero, v0
	RET
	END(_ustore_32)

/* LINTSTUB: int _ustore_64(uint64_t *uaddr, uint64_t val); */
LEAF_NOPROFILE(_ustore_64, 2)
	UFETCHSTORE_PROLOGUE
.L_ustore_64_start:
	stq	a1, 0(a0)
.L_ustore_64_end:
	mov	zero, v0
	RET
	END(_ustore_64)

LEAF_NOPROFILE(ufetchstoreerr_efault, 0)
	ldiq	v0, EFAULT	/* return EFAULT. */
XLEAF(ufetchstoreerr, 0)
	LDGP(pv)
	RET
	END(ufetchstoreerr_efault)

/**************************************************************************/

/*
 * int _ucas_32(volatile uint32_t *uptr, uint32_t old, uint32_t new,
 *		uint32_t *ret);
 */
LEAF_NOPROFILE(_ucas_32, 4)
	UFETCHSTORE_PROLOGUE
3:
.Lucas_32_start:
	mov	a2, t2
	ldl_l	t0, 0(a0)			/* t0 = *uptr */
	cmpeq	t0, a1, t1			/* does t0 = old? */
	beq	t1, 1f				/* if not, skip */
	stl_c	t2, 0(a0)			/* *uptr ~= new */
.Lucas_32_end:
	beq	t1, 2f				/* did it work? */
1:
	stl	t0, 0(a3)			/* *ret = t0 */
	mov	zero, v0
	RET
2:
	br	3b
END(_ucas_32)

/*
 * int _ucas_64(volatile uint64_t *uptr, uint64_t old, uint64_t new,
 *		uint64_t *ret);
 */
LEAF_NOPROFILE(_ucas_64, 4)
	UFETCHSTORE_PROLOGUE
3:
.Lucas_64_start:
	mov	a2, t2
	ldq_l	t0, 0(a0)			/* t0 = *uptr */
	cmpeq	t0, a1, t1			/* does t0 = old? */
	beq	t1, 1f				/* if not, skip */
	stq_c	t2, 0(a0)			/* *uptr ~= new */
.Lucas_64_end:
	beq	t1, 2f				/* did it work? */
1:
	stq	t0, 0(a3)			/* *ret = t0 */
	mov	zero, v0
	RET
2:
	br	3b
END(_ucas_64)

/**************************************************************************/

/*
 * Fault table of user access functions for trap().
 */
	.section ".rodata"
	.globl	onfault_table
onfault_table:
	.quad	.L_ufetch_8_start
	.quad	.L_ufetch_8_end
	.quad	ufetchstoreerr

	.quad	.L_ufetch_16_start
	.quad	.L_ufetch_16_end
	.quad	ufetchstoreerr

	.quad	.L_ufetch_32_start
	.quad	.L_ufetch_32_end
	.quad	ufetchstoreerr

	.quad	.L_ufetch_64_start
	.quad	.L_ufetch_64_end
	.quad	ufetchstoreerr

	.quad	.L_ustore_8_start
	.quad	.L_ustore_8_end
	.quad	ufetchstoreerr

	.quad	.L_ustore_16_start
	.quad	.L_ustore_16_end
	.quad	ufetchstoreerr

	.quad	.L_ustore_32_start
	.quad	.L_ustore_32_end
	.quad	ufetchstoreerr

	.quad	.L_ustore_64_start
	.quad	.L_ustore_64_end
	.quad	ufetchstoreerr

	.quad	.Lucas_32_start
	.quad	.Lucas_32_end
	.quad	ufetchstoreerr

	.quad	.Lucas_64_start
	.quad	.Lucas_64_end
	.quad	ufetchstoreerr

	.quad	0

	.text

/**************************************************************************/

/*
 * console 'restart' routine to be placed in HWRPB.
 */
LEAF(XentRestart, 1)			/* XXX should be NESTED */
	.set noat
	lda	sp,-(FRAME_SIZE*8)(sp)
	stq	at_reg,(FRAME_AT*8)(sp)
	.set at
	stq	v0,(FRAME_V0*8)(sp)
	stq	a0,(FRAME_A0*8)(sp)
	stq	a1,(FRAME_A1*8)(sp)
	stq	a2,(FRAME_A2*8)(sp)
	stq	a3,(FRAME_A3*8)(sp)
	stq	a4,(FRAME_A4*8)(sp)
	stq	a5,(FRAME_A5*8)(sp)
	stq	s0,(FRAME_S0*8)(sp)
	stq	s1,(FRAME_S1*8)(sp)
	stq	s2,(FRAME_S2*8)(sp)
	stq	s3,(FRAME_S3*8)(sp)
	stq	s4,(FRAME_S4*8)(sp)
	stq	s5,(FRAME_S5*8)(sp)
	stq	s6,(FRAME_S6*8)(sp)
	stq	t0,(FRAME_T0*8)(sp)
	stq	t1,(FRAME_T1*8)(sp)
	stq	t2,(FRAME_T2*8)(sp)
	stq	t3,(FRAME_T3*8)(sp)
	stq	t4,(FRAME_T4*8)(sp)
	stq	t5,(FRAME_T5*8)(sp)
	stq	t6,(FRAME_T6*8)(sp)
	stq	t7,(FRAME_T7*8)(sp)
	stq	t8,(FRAME_T8*8)(sp)
	stq	t9,(FRAME_T9*8)(sp)
	stq	t10,(FRAME_T10*8)(sp)
	stq	t11,(FRAME_T11*8)(sp)
	stq	t12,(FRAME_T12*8)(sp)
	stq	ra,(FRAME_RA*8)(sp)

	br	pv,1f
1:	LDGP(pv)

	mov	sp,a0
	CALL(console_restart)

	call_pal PAL_halt
	END(XentRestart)

/**************************************************************************/

/*
 * Kernel setjmp and longjmp.  Rather minimalist.
 *
 *	longjmp(label_t *a)
 * will generate a "return (1)" from the last call to
 *	setjmp(label_t *a)
 * by restoring registers from the stack,
 */

	.set	noreorder

LEAF(setjmp, 1)
	LDGP(pv)

	stq	ra, (0 * 8)(a0)			/* return address */
	stq	s0, (1 * 8)(a0)			/* callee-saved registers */
	stq	s1, (2 * 8)(a0)
	stq	s2, (3 * 8)(a0)
	stq	s3, (4 * 8)(a0)
	stq	s4, (5 * 8)(a0)
	stq	s5, (6 * 8)(a0)
	stq	s6, (7 * 8)(a0)
	stq	sp, (8 * 8)(a0)

	ldiq	t0, 0xbeeffedadeadbabe		/* set magic number */
	stq	t0, (9 * 8)(a0)

	mov	zero, v0			/* return zero */
	RET
END(setjmp)

LEAF(longjmp, 1)
	LDGP(pv)

	ldiq	t0, 0xbeeffedadeadbabe		/* check magic number */
	ldq	t1, (9 * 8)(a0)
	cmpeq	t0, t1, t0
	beq	t0, longjmp_botch		/* if bad, punt */

	ldq	ra, (0 * 8)(a0)			/* return address */
	ldq	s0, (1 * 8)(a0)			/* callee-saved registers */
	ldq	s1, (2 * 8)(a0)
	ldq	s2, (3 * 8)(a0)
	ldq	s3, (4 * 8)(a0)
	ldq	s4, (5 * 8)(a0)
	ldq	s5, (6 * 8)(a0)
	ldq	s6, (7 * 8)(a0)
	ldq	sp, (8 * 8)(a0)

	ldiq	v0, 1
	RET

longjmp_botch:
	lda	a0, longjmp_botchmsg
	mov	ra, a1
	CALL(panic)
	call_pal PAL_bugchk

	.data
longjmp_botchmsg:
	.asciz	"longjmp botch from %p"
	.text
END(longjmp)

/*
 * void sts(int rn, u_int32_t *rval);
 * void stt(int rn, u_int64_t *rval);
 * void lds(int rn, u_int32_t *rval);
 * void ldt(int rn, u_int64_t *rval);
 */

.macro	make_freg_util name, op
	LEAF(alpha_\name, 2)
	and	a0, 0x1f, a0
	s8addq	a0, pv, pv
	addq	pv, 1f - alpha_\name, pv
	jmp	(pv)
1:
	rn = 0
	.rept	32
	\op	$f0 + rn, 0(a1)
	RET
	rn = rn + 1
	.endr
	END(alpha_\name)
.endm
/*
LEAF(alpha_sts, 2)
LEAF(alpha_stt, 2)
LEAF(alpha_lds, 2)
LEAF(alpha_ldt, 2)
 */
	make_freg_util sts, sts
	make_freg_util stt, stt
	make_freg_util lds, lds
	make_freg_util ldt, ldt

LEAF(alpha_read_fpcr, 0); f30save = 0; rettmp = 8; framesz = 16
	lda	sp, -framesz(sp)
	stt	$f30, f30save(sp)
	mf_fpcr	$f30
	stt	$f30, rettmp(sp)
	ldt	$f30, f30save(sp)
	ldq	v0, rettmp(sp)
	lda	sp, framesz(sp)
	RET
END(alpha_read_fpcr)

LEAF(alpha_write_fpcr, 1); f30save = 0; fpcrtmp = 8; framesz = 16
	lda	sp, -framesz(sp)
	stq	a0, fpcrtmp(sp)
	stt	$f30, f30save(sp)
	ldt	$f30, fpcrtmp(sp)
	mt_fpcr	$f30
	ldt	$f30, f30save(sp)
	lda	sp, framesz(sp)
	RET
END(alpha_write_fpcr)
