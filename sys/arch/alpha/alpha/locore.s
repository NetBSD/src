/*	$NetBSD: locore.s,v 1.4 1995/06/28 02:45:04 cgd Exp $	*/

/*
 * Copyright (c) 1994, 1995 Carnegie-Mellon University.
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

#define LOCORE

#include <machine/asm.h>
#include "assym.s"

	/* don't reorder instructions; paranoia. */
	.set noreorder

/*
 * bootstack: a temporary stack, for booting.
 *
 * Extends from 'start' down.
 */
bootstack:

/*
 * __start: Kernel start.
 *
 * Arguments:
 *	a0 is the first free page frame number (PFN)
 *	a1 is the page table base register (PTBR)
 *	a2 argc
 *	a3 argv
 *	a4 envp
 *
 * All arguments are passed to alpha_init().
 */
	.text
NESTED(__start,1,0,ra,0,0)
	br	pv,1f
1:	SETGP(pv)

	/* Save a0, used by pal_wrkgp. */
	or	a0,zero,s0

	/* Load KGP with current GP. */
	or	gp,zero,a0
	CALL(pal_wrkgp)

	/* Switch to the boot stack. */
	lda	sp,bootstack

	/*
	 * Call alpha_init() to do pre-main initialization.  Restore
	 * a0, and pass alpha_init the arguments we were called with.
	 */
	or	s0,zero,a0
	CALL(alpha_init)

	/* Set up the virtual page table pointer. */
	CONST(VPTBASE, a0)
	CALL(pal_wrvptptr)

	/*
	 * Switch to proc0's PCB, which is at U_PCB off of proc0paddr.
	 */
	lda	t0,proc0			/* get phys addr of pcb */
	ldq	a0,P_MD_PCBPADDR(t0)
	call_pal PAL_OSF1_swpctx
	CONST(-1, a0)
	call_pal PAL_OSF1_tbi

	/*
	 * put a fake RA (0 XXX) on the stack, to panic if anything
	 * ever tries to return off the end of the stack
	 */
	lda	sp,-8(sp)
	stq	zero,0(sp)

	/*
	 * Construct a fake trap frame, so execve() can work normally.
	 * Note that setregs() is responsible for setting its contents
	 * to 'reasonable' values.
	 */
	lda	sp,-(FRAMESIZE)(sp)		/* space for struct trapframe */

	mov	sp, a0				/* main()'s arg is frame ptr */
	CALL(main)				/* go to main()! */

	/*
	 * Call REI, to restore the faked up trap frame and return
	 * to proc 1 == init!
	 */
	mov	zero, a0
	JMP(rei)			/* "And that's all she wrote." */
	END(__start)

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

	.data
EXPORT(cold)
	.long 1			/* cold start flag (.long -> _4_ bytes) */
	.text

/**************************************************************************/

/*
 * Signal "trampoline" code. Invoked from RTE setup by sendsig().
 *
 * On entry, stack & registers look like:
 *
 *      a0	signal number
 *      a1	signal specific code
 *      a2	pointer to signal context frame (scp)
 *      a3	address of handler
 *      sp+0	saved hardware state
 *                      .
 *                      .
 *      scp+0	beginning of signal context frame
 */

NESTED(sigcode,0,0,ra,0,0)
	lda	sp, -16(sp)		/* save the sigcontext pointer */
	stq	a2, 0(sp)
	jsr	ra, (t12)		/* call the signal handler (t12==pv) */
	ldq	a0, 0(sp)		/* get the sigcontext pointer */
	lda	sp, 16(sp)
	CONST(SYS_sigreturn, v0)	/* and call sigreturn() with it. */
	call_pal PAL_OSF1_callsys
	mov	v0, a0			/* if that failed, get error code */
	CONST(SYS_exit, v0)		/* and call exit() with it. */
	call_pal PAL_OSF1_callsys
XNESTED(esigcode,0)
	END(sigcode)

/**************************************************************************/

/*
 * Memory barrier: get all of the stuff in write caches out to memory.
 */
LEAF(alpha_mb,0)
	mb
	RET
	END(alpha_mb)

/**************************************************************************/

/*
 * rei: pseudo-emulation of VAX REI.
 */

BSS(ssir, 8)
IMPORT(astpending, 8)

LEAF(rei, 1)					/* XXX should be NESTED */
	br	pv, 1f
1:	SETGP(pv)

	ldq	s1, TF_PS(sp)			/* get the saved PS */
	and	s1, PSL_IPL, t0			/* look at the saved IPL */
	bne	t0, Lrestoreregs		/* != 0: can't do AST or SIR */

	/* see if we can do an SIR */
	ldq	t1, ssir			/* SIR pending? */
	beq	t1, Lchkast			/* no, try an AST*/

	/* We've got a SIR. */
	CONST(PSL_IPL_SOFT, a0)			/* yes, lower IPL to soft */
	call_pal PAL_OSF1_swpipl
	CALL(do_sir)				/* do the SIR */

Lchkast:
	CONST(PSL_IPL_0, a0)			/* drop IPL to zero*/
	call_pal PAL_OSF1_swpipl

	and	s1, PSL_U, t0			/* are we returning to user? */
	beq	t0, Lrestoreregs		/* no: just return */

	ldq	t2, astpending			/* AST pending? */
	beq	t2, Lsetfpenable		/* no: return & deal with FP */

	/* we've got an AST.  call trap to handle it */
	CONST(T_ASTFLT, a0)			/* type = T_ASTFLT */
	mov	zero, a1			/* code = 0 */
	mov	zero, a2			/* v = 0 */
	mov	sp, a3				/* frame */
	CALL(trap)

Lsetfpenable:
	/* enable FPU based on whether the current proc is fpcurproc */
	ldq	t0, curproc
	ldq	t1, fpcurproc
	cmpeq	t0, t1, t0
	mov	zero, a0
	cmovne	t0, 1, a0
	call_pal PAL_OSF1_wrfen

Lrestoreregs:
	/* restore the USP and the registers, and return */
	ldq	a0,(FRAME_SP*8)(sp)
	call_pal PAL_OSF1_wrusp

	.set noat
	ldq	v0,(FRAME_V0*8)(sp)
	ldq	t0,(FRAME_T0*8)(sp)
	ldq	t1,(FRAME_T1*8)(sp)
	ldq	t2,(FRAME_T2*8)(sp)
	ldq	t3,(FRAME_T3*8)(sp)
	ldq	t4,(FRAME_T4*8)(sp)
	ldq	t5,(FRAME_T5*8)(sp)
	ldq	t6,(FRAME_T6*8)(sp)
	ldq	t7,(FRAME_T7*8)(sp)
	ldq	s0,(FRAME_S0*8)(sp)
	ldq	s1,(FRAME_S1*8)(sp)
	ldq	s2,(FRAME_S2*8)(sp)
	ldq	s3,(FRAME_S3*8)(sp)
	ldq	s4,(FRAME_S4*8)(sp)
	ldq	s5,(FRAME_S5*8)(sp)
	ldq	s6,(FRAME_S6*8)(sp)
	ldq	a3,(FRAME_A3*8)(sp)
	ldq	a4,(FRAME_A4*8)(sp)
	ldq	a5,(FRAME_A5*8)(sp)
	ldq	t8,(FRAME_T8*8)(sp)
	ldq	t9,(FRAME_T9*8)(sp)
	ldq	t10,(FRAME_T10*8)(sp)
	ldq	t11,(FRAME_T11*8)(sp)
	ldq	ra,(FRAME_RA*8)(sp)
	ldq	t12,(FRAME_T12*8)(sp)
	ldq	at_reg,(FRAME_AT*8)(sp)

	lda	sp,(FRAME_NSAVEREGS*8)(sp)
	call_pal PAL_OSF1_rti
	END(rei)

/**************************************************************************/

/*
 * XentArith:
 * System arithmetic trap entry point.
 */

LEAF(XentArith, 2)				/* XXX should be NESTED */
	.set noat
	lda	sp,-(FRAME_NSAVEREGS*8)(sp)
	stq	v0,(FRAME_V0*8)(sp)
	stq	t0,(FRAME_T0*8)(sp)
	stq	t1,(FRAME_T1*8)(sp)
	stq	t2,(FRAME_T2*8)(sp)
	stq	t3,(FRAME_T3*8)(sp)
	stq	t4,(FRAME_T4*8)(sp)
	stq	t5,(FRAME_T5*8)(sp)
	stq	t6,(FRAME_T6*8)(sp)
	stq	t7,(FRAME_T7*8)(sp)
	stq	s0,(FRAME_S0*8)(sp)
	stq	s1,(FRAME_S1*8)(sp)
	stq	s2,(FRAME_S2*8)(sp)
	mov	a0,s0
	stq	s3,(FRAME_S3*8)(sp)
	stq	s4,(FRAME_S4*8)(sp)
	stq	s5,(FRAME_S5*8)(sp)
	stq	s6,(FRAME_S6*8)(sp)
	mov	a1,s1
	stq	a3,(FRAME_A3*8)(sp)
	stq	a4,(FRAME_A4*8)(sp)
	stq	a5,(FRAME_A5*8)(sp)
	stq	t8,(FRAME_T8*8)(sp)
	stq	t9,(FRAME_T9*8)(sp)
	stq	t10,(FRAME_T10*8)(sp)
	stq	t11,(FRAME_T11*8)(sp)
	stq	ra,(FRAME_RA*8)(sp)
	stq	t12,(FRAME_T12*8)(sp)
	stq	at_reg,(FRAME_AT*8)(sp)

	call_pal PAL_OSF1_rdusp
	stq	v0,(FRAME_SP*8)(sp)

	.set at

	br	pv, 1f
1:	SETGP(pv)

	CONST(T_ARITHFLT, a0)			/* type = T_ARITHFLT */
	mov	s0, a1				/* code = "summary" */
	mov	s1, a2				/* v = "reguster mask" */
	mov	sp, a3				/* frame */
	CALL(trap)

	JMP(rei)
	END(XentArith)

/**************************************************************************/

/*
 * XentIF:
 * System instruction fault trap entry point.
 */

LEAF(XentIF, 1)					/* XXX should be NESTED */
	.set noat
	lda	sp,-(FRAME_NSAVEREGS*8)(sp)
	stq	v0,(FRAME_V0*8)(sp)
	stq	t0,(FRAME_T0*8)(sp)
	stq	t1,(FRAME_T1*8)(sp)
	stq	t2,(FRAME_T2*8)(sp)
	stq	t3,(FRAME_T3*8)(sp)
	stq	t4,(FRAME_T4*8)(sp)
	stq	t5,(FRAME_T5*8)(sp)
	stq	t6,(FRAME_T6*8)(sp)
	stq	t7,(FRAME_T7*8)(sp)
	stq	s0,(FRAME_S0*8)(sp)
	stq	s1,(FRAME_S1*8)(sp)
	stq	s2,(FRAME_S2*8)(sp)
	mov	a0,s0
	stq	s3,(FRAME_S3*8)(sp)
	stq	s4,(FRAME_S4*8)(sp)
	stq	s5,(FRAME_S5*8)(sp)
	stq	s6,(FRAME_S6*8)(sp)
	stq	a3,(FRAME_A3*8)(sp)
	stq	a4,(FRAME_A4*8)(sp)
	stq	a5,(FRAME_A5*8)(sp)
	stq	t8,(FRAME_T8*8)(sp)
	stq	t9,(FRAME_T9*8)(sp)
	stq	t10,(FRAME_T10*8)(sp)
	stq	t11,(FRAME_T11*8)(sp)
	stq	ra,(FRAME_RA*8)(sp)
	stq	t12,(FRAME_T12*8)(sp)
	stq	at_reg,(FRAME_AT*8)(sp)

	call_pal PAL_OSF1_rdusp
	stq	v0,(FRAME_SP*8)(sp)

	.set at

	br	pv, 1f
1:	SETGP(pv)

	or	s0, T_IFLT, a0			/* type = T_IFLT|type*/
	mov	s0, a1				/* code = type */
	ldq	a2, TF_PC(sp)			/* v = frame's pc */
	mov	sp, a3				/* frame */
	CALL(trap)

	JMP(rei)
	END(XentIF)

/**************************************************************************/

/*
 * XentInt:
 * System interrupt entry point.
 */

LEAF(XentInt, 2)				/* XXX should be NESTED */
	.set noat
	lda	sp,-(FRAME_NSAVEREGS*8)(sp)
	stq	v0,(FRAME_V0*8)(sp)
	stq	t0,(FRAME_T0*8)(sp)
	stq	t1,(FRAME_T1*8)(sp)
	stq	t2,(FRAME_T2*8)(sp)
	stq	t3,(FRAME_T3*8)(sp)
	stq	t4,(FRAME_T4*8)(sp)
	stq	t5,(FRAME_T5*8)(sp)
	stq	t6,(FRAME_T6*8)(sp)
	stq	t7,(FRAME_T7*8)(sp)
	stq	s0,(FRAME_S0*8)(sp)
	stq	s1,(FRAME_S1*8)(sp)
	stq	s2,(FRAME_S2*8)(sp)
	mov	a0,s0
	stq	s3,(FRAME_S3*8)(sp)
	stq	s4,(FRAME_S4*8)(sp)
	stq	s5,(FRAME_S5*8)(sp)
	stq	s6,(FRAME_S6*8)(sp)
	mov	a1,s1
	stq	a3,(FRAME_A3*8)(sp)
	stq	a4,(FRAME_A4*8)(sp)
	stq	a5,(FRAME_A5*8)(sp)
	stq	t8,(FRAME_T8*8)(sp)
	mov	a2,s2
	stq	t9,(FRAME_T9*8)(sp)
	stq	t10,(FRAME_T10*8)(sp)
	stq	t11,(FRAME_T11*8)(sp)
	stq	ra,(FRAME_RA*8)(sp)
	stq	t12,(FRAME_T12*8)(sp)
	stq	at_reg,(FRAME_AT*8)(sp)

	call_pal PAL_OSF1_rdusp
	stq	v0,(FRAME_SP*8)(sp)

	.set at

	br	pv, 1f
1:	SETGP(pv)

	mov	s2,a3
	mov	s1,a2
	mov	s0,a1
	mov	sp,a0
	CALL(interrupt)

	JMP(rei)
	END(XentInt)

/**************************************************************************/

/*
 * XentMM:
 * System memory management fault entry point.
 */

LEAF(XentMM, 3)					/* XXX should be NESTED */
	.set noat
	lda	sp,-(FRAME_NSAVEREGS*8)(sp)
	stq	v0,(FRAME_V0*8)(sp)
	stq	t0,(FRAME_T0*8)(sp)
	stq	t1,(FRAME_T1*8)(sp)
	stq	t2,(FRAME_T2*8)(sp)
	stq	t3,(FRAME_T3*8)(sp)
	stq	t4,(FRAME_T4*8)(sp)
	stq	t5,(FRAME_T5*8)(sp)
	stq	t6,(FRAME_T6*8)(sp)
	stq	t7,(FRAME_T7*8)(sp)
	stq	s0,(FRAME_S0*8)(sp)
	stq	s1,(FRAME_S1*8)(sp)
	stq	s2,(FRAME_S2*8)(sp)
	mov	a0,s0
	stq	s3,(FRAME_S3*8)(sp)
	stq	s4,(FRAME_S4*8)(sp)
	stq	s5,(FRAME_S5*8)(sp)
	stq	s6,(FRAME_S6*8)(sp)
	mov	a1,s1
	stq	a3,(FRAME_A3*8)(sp)
	stq	a4,(FRAME_A4*8)(sp)
	stq	a5,(FRAME_A5*8)(sp)
	stq	t8,(FRAME_T8*8)(sp)
	mov	a2,s2
	stq	t9,(FRAME_T9*8)(sp)
	stq	t10,(FRAME_T10*8)(sp)
	stq	t11,(FRAME_T11*8)(sp)
	stq	ra,(FRAME_RA*8)(sp)
	stq	t12,(FRAME_T12*8)(sp)
	stq	at_reg,(FRAME_AT*8)(sp)

	call_pal PAL_OSF1_rdusp
	stq	v0,(FRAME_SP*8)(sp)

	.set at

	br	pv, 1f
1:	SETGP(pv)

	or	s1, T_MMFLT, a0			/* type = T_MMFLT|MMCSR */
	mov	s2, a1				/* code = "cause" */
	mov	s0, a2				/* v = VA */
	mov	sp, a3				/* frame */
	CALL(trap)

	JMP(rei)
	END(XentMM)

/**************************************************************************/

/*
 * XentSys:
 * System call entry point.
 */

LEAF(XentSys, 0)				/* XXX should be NESTED */
	.set noat
	lda	sp,-(FRAME_NSAVEREGS*8)(sp)
	stq	v0,(FRAME_V0*8)(sp)		/* in case we need to restart */
	stq	s0,(FRAME_S0*8)(sp)
	stq	s1,(FRAME_S1*8)(sp)
	stq	s2,(FRAME_S2*8)(sp)
	stq	s3,(FRAME_S3*8)(sp)
	stq	s4,(FRAME_S4*8)(sp)
	stq	s5,(FRAME_S5*8)(sp)
	stq	s6,(FRAME_S6*8)(sp)
	stq	a0,TF_A0(sp)
	stq	a1,TF_A1(sp)
	stq	a2,TF_A2(sp)
	stq	a3,(FRAME_A3*8)(sp)
	stq	a4,(FRAME_A4*8)(sp)
	stq	a5,(FRAME_A5*8)(sp)
	stq	ra,(FRAME_RA*8)(sp)

	/* save syscall number, which was passed in v0. */
	mov	v0,s0

	call_pal PAL_OSF1_rdusp
	stq	v0,(FRAME_SP*8)(sp)

	.set at

	br	pv, 1f
1:	SETGP(pv)

	mov	s0,a0
	mov	sp,a1
	CALL(syscall)

	JMP(rei)
	END(XentSys)

/**************************************************************************/

/*
 * XentUna:
 * System unaligned access entry point.
 */

LEAF(XentUna, 3)				/* XXX should be NESTED */
	.set noat
	lda	sp,-(FRAME_NSAVEREGS*8)(sp)
	stq	v0,(FRAME_V0*8)(sp)
	stq	t0,(FRAME_T0*8)(sp)
	stq	t1,(FRAME_T1*8)(sp)
	stq	t2,(FRAME_T2*8)(sp)
	stq	t3,(FRAME_T3*8)(sp)
	stq	t4,(FRAME_T4*8)(sp)
	stq	t5,(FRAME_T5*8)(sp)
	stq	t6,(FRAME_T6*8)(sp)
	stq	t7,(FRAME_T7*8)(sp)
	stq	s0,(FRAME_S0*8)(sp)
	stq	s1,(FRAME_S1*8)(sp)
	stq	s2,(FRAME_S2*8)(sp)
	mov	a0,s0
	stq	s3,(FRAME_S3*8)(sp)
	stq	s4,(FRAME_S4*8)(sp)
	stq	s5,(FRAME_S5*8)(sp)
	stq	s6,(FRAME_S6*8)(sp)
	mov	a1,s1
	stq	a3,(FRAME_A3*8)(sp)
	stq	a4,(FRAME_A4*8)(sp)
	stq	a5,(FRAME_A5*8)(sp)
	stq	t8,(FRAME_T8*8)(sp)
	mov	a2,s2
	stq	t9,(FRAME_T9*8)(sp)
	stq	t10,(FRAME_T10*8)(sp)
	stq	t11,(FRAME_T11*8)(sp)
	stq	ra,(FRAME_RA*8)(sp)
	stq	t12,(FRAME_T12*8)(sp)
	stq	at_reg,(FRAME_AT*8)(sp)

	call_pal PAL_OSF1_rdusp
	stq	v0,(FRAME_SP*8)(sp)

	.set at

	br	pv, 1f
1:	SETGP(pv)

	CONST(T_UNAFLT, a0)			/* type = T_UNAFLT */
	mov	zero, a1			/* code = 0 */
	mov	zero, a2			/* v = 0 */
	mov	sp, a3				/* frame */
	CALL(trap)

	JMP(rei)
	END(XentUna)

/**************************************************************************/

/*
 * savefpstate: Save a process's floating point state.
 *
 * Arguments:
 *	a0	'struct fpstate *' to save into
 */

LEAF(savefpstate, 1)
	SETGP(pv)
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
	stt	$f28, (28 * 8)(t1)
	stt	$f29, (29 * 8)(t1)
	stt	$f30, (30 * 8)(t1)

	/*
	 * Then save the FPCR; note that the necessary 'trapb's are taken
	 * care of on kernel entry and exit.
	 */
	MF_FPCR(ft0)
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
	SETGP(pv)
	/*
	 * Restore the FPCR; note that the necessary 'trapb's are taken care of
	 * on kernel entry and exit.
	 */
	ldt	ft0, FPREG_FPR_CR(a0)	/* load from FPCR save area */
	MT_FPCR(ft0)

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
 * Note that savectx() only works for processes other than curproc,
 * since cpu_switch will copy over the info saved here.  (It _can_
 * sanely be used for curproc iff cpu_switch won't be called again, e.g.
 * from if called from boot().)
 *
 * Arguments:
 *	a0	'struct user *' of the process that needs its context saved
 *
 * Return:
 *	v0	0.  (note that for child processes, it seems
 *		like savectx() returns 1, because the return address
 *		in the PCB is set to the return address from savectx().)
 */

LEAF(savectx, 1)
	br	pv, 1f
1:	SETGP(pv)
	stq	sp, U_PCB_KSP(a0)		/* store sp */
	stq	s0, U_PCB_CONTEXT+(0 * 8)(a0)	/* store s0 - s6 */
	stq	s1, U_PCB_CONTEXT+(1 * 8)(a0)
	stq	s2, U_PCB_CONTEXT+(2 * 8)(a0)
	stq	s3, U_PCB_CONTEXT+(3 * 8)(a0)
	stq	s4, U_PCB_CONTEXT+(4 * 8)(a0)
	stq	s5, U_PCB_CONTEXT+(5 * 8)(a0)
	stq	s6, U_PCB_CONTEXT+(6 * 8)(a0)
	stq	ra, U_PCB_CONTEXT+(7 * 8)(a0)	/* store ra */
	call_pal PAL_OSF1_rdps			/* NOTE: doesn't kill a0 */
	stq	v0, U_PCB_CONTEXT+(8 * 8)(a0)	/* store ps, for ipl */

	mov	zero, v0
	RET
	END(savectx)

/**************************************************************************/

BSS(curpcb, 8)

IMPORT(whichqs, 4)
IMPORT(want_resched, 8)
IMPORT(Lev1map, 8)

/*
 * When no processes are on the runq, cpu_switch branches to idle
 * to wait for something to come ready.
 * Note: this is really a part of cpu_switch() but defined here for kernel
 * profiling.
 */
LEAF(idle, 0)
	br	pv, 1f
1:	SETGP(pv)
	stq	zero, curproc			/* curproc <- NULL for stats */
	mov	zero, a0			/* enable all interrupts */
	call_pal PAL_OSF1_swpipl
2:
	ldl	t0, whichqs			/* look for non-empty queue */
	beq	t0, 2b
	CONST(PSL_IPL_HIGH, a0)			/* disable all interrupts */
	call_pal PAL_OSF1_swpipl
	JMP(sw1)				/* jump back into the fray */
	END(idle)

/*
 * cpu_switch()
 * Find the highest priority process and resume it.
 * XXX should optimiize, and not do the switch if switching to curproc
 */
LEAF(cpu_switch, 0)
	SETGP(pv)
	/* do an inline savectx(), to save old context */
	ldq	a0, curproc
	ldq	a0, P_ADDR(a0)
	/* NOTE: ksp is stored by the swpctx */
	stq	s0, U_PCB_CONTEXT+(0 * 8)(a0)	/* store s0 - s6 */
	stq	s1, U_PCB_CONTEXT+(1 * 8)(a0)
	stq	s2, U_PCB_CONTEXT+(2 * 8)(a0)
	stq	s3, U_PCB_CONTEXT+(3 * 8)(a0)
	stq	s4, U_PCB_CONTEXT+(4 * 8)(a0)
	stq	s5, U_PCB_CONTEXT+(5 * 8)(a0)
	stq	s6, U_PCB_CONTEXT+(6 * 8)(a0)
	stq	ra, U_PCB_CONTEXT+(7 * 8)(a0)	/* store ra */
	call_pal PAL_OSF1_rdps			/* NOTE: doesn't kill a0 */
	stq	v0, U_PCB_CONTEXT+(8 * 8)(a0)	/* store ps, for ipl */

	ldl	t0, whichqs			/* look for non-empty queue */
	beq	t0, idle			/* and if none, go idle */

	CONST(PSL_IPL_HIGH, a0)			/* disable all interrupts */
	call_pal PAL_OSF1_swpipl
sw1:
	br	pv, 1f
1:	SETGP(pv)
	ldl	t0, whichqs			/* look for non-empty queue */
	beq	t0, idle			/* and if none, go idle */
	mov	t0, t3				/* t3 = saved whichqs */
	mov	zero, t2			/* t2 = lowest bit set */
	blbs	t0, 3f				/* if low bit set, done! */

2:	srl	t0, 1, t0			/* try next bit */
	addq	t2, 1, t2
	blbc	t0, 2b				/* if clear, try again */

3:
	/*
	 * Remove process from queue
	 */
	lda	t1, qs				/* get queues */
	sll	t2, 4, t0			/* queue head is 16 bytes */
	addq	t1, t0, t0			/* t0 = qp = &qs[firstbit] */

	ldq	t4, PH_LINK(t0)			/* t4 = p = highest pri proc */
	ldq	t5, P_FORW(t4)			/* t5 = p->p_forw */
	bne	t4, 4f				/* make sure p != NULL */
	PANIC("cpu_switch")			/* nothing in queue! */

4:
	stq	t5, PH_LINK(t0)			/* qp->ph_link = p->p_forw */
	stq	t0, P_BACK(t5)			/* p->p_forw->p_back = qp */
	stq	zero, P_BACK(t4)		/* firewall: p->p_back = NULL */
	cmpeq	t0, t5, t0			/* see if queue is empty */
	beq	t0, 5f				/* nope, it's not! */

	CONST(1, t0)				/* compute bit in whichqs */
	sll	t0, t2, t0
	xor	t3, t0, t3			/* clear bit in whichqs */
	stl	t3, whichqs

5:
	/*
	 * Switch to the new context
	 */

	/* mark the new curproc, and other globals */
	stq	zero, want_resched		/* we've rescheduled */
	/* XXX should allocate an ASN, rather than just flushing */
	stq	t4, curproc			/* curproc = p */
	ldq	t5, P_MD_PCBPADDR(t4)		/* t5 = p->p_md.md_pcbpaddr */
	stq	t5, curpcb			/* and store it in curpcb */

	/*
	 * Do the context swap, and invalidate old TLB entries (XXX).
	 * XXX should do the ASN thing, and therefore not have to invalidate.
	 */
	ldq	t2, P_VMSPACE(t4                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          