/*	$NetBSD: locore.S,v 1.1 2002/06/06 19:48:06 fredette Exp $	*/

/*	$OpenBSD: locore.S,v 1.46 2001/09/20 18:33:03 mickey Exp $	*/

/*
 * Copyright (c) 1998-2001 Michael Shalayeff
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
 *      This product includes software developed by Michael Shalayeff.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR HIS RELATIVES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF MIND, USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Portitions of this file are derived from other sources, see
 * the copyrights and acknowledgements below.
 */
/*
 * Copyright (c) 1990,1991,1992,1994 The University of Utah and
 * the Computer Systems Laboratory (CSL).  All rights reserved.
 *
 * THE UNIVERSITY OF UTAH AND CSL PROVIDE THIS SOFTWARE IN ITS "AS IS"
 * CONDITION, AND DISCLAIM ANY LIABILITY OF ANY KIND FOR ANY DAMAGES
 * WHATSOEVER RESULTING FROM ITS USE.
 *
 * CSL requests users of this software to return to csl-dist@cs.utah.edu any
 * improvements that they make and grant CSL redistribution rights.
 *
 *	Utah $Hdr: locore.s 1.62 94/12/15$
 */
/*
 *  (c) Copyright 1988 HEWLETT-PACKARD COMPANY
 *
 *  To anyone who acknowledges that this file is provided "AS IS"
 *  without any express or implied warranty:
 *      permission to use, copy, modify, and distribute this file
 *  for any purpose is hereby granted without fee, provided that
 *  the above copyright notice and this notice appears in all
 *  copies, and that the name of Hewlett-Packard Company not be
 *  used in advertising or publicity pertaining to distribution
 *  of the software without specific, written prior permission.
 *  Hewlett-Packard Company makes no representations about the
 *  suitability of this software for any purpose.
 */

#include "opt_kgdb.h"
#include "opt_lockdebug.h"

#include <sys/errno.h>
#include <machine/param.h>
#include <machine/asm.h>
#include <machine/psl.h>
#include <machine/trap.h>
#include <machine/iomod.h>
#include <machine/pdc.h>
#include <machine/intr.h>
#include <machine/frame.h>
#include <machine/reg.h>
#ifdef	GPROF
#include <machine/gprof.h>
#endif
#include "assym.h"

/* Some aliases for the macros in assym.h. */
#define	TRAPFRAME_SIZEOF	trapframe_SIZEOF

/*
 * Very crude debugging macros that write to com1.
 */
#define	COM1_TX_REG	(0xf0823000 + 0x800)
#define _DEBUG_PUTCHAR(reg1, reg2)	! \
	ldil	L%COM1_TX_REG, reg1	! \
	stb	reg2, R%COM1_TX_REG(sr1, reg1) ! \
	ldil	L%60000000, reg1	! \
	ldi	1, reg2			! \
	comb,<>,n	reg1, r0, -8	! \
	sub	reg1, reg2, reg1
#define DEBUG_PUTCHAR(reg1, reg2, ch)	! \
	ldi	ch, reg2		! \
	_DEBUG_PUTCHAR(reg1, reg2)
#define _DEBUG_DUMPN(reg1, reg2, reg3, p)	! \
	extru	reg3, p, 4, reg2		! \
	comib,>>,n	10, reg2, 0		! \
	addi	39, reg2, reg2			! \
	addi	48, reg2, reg2			! \
	_DEBUG_PUTCHAR(reg1, reg2)
#define DEBUG_DUMP32(reg1, reg2, reg3)		! \
	DEBUG_PUTCHAR(reg1, reg2, 58)		! \
	_DEBUG_DUMPN(reg1, reg2, reg3, 3)	! \
	_DEBUG_DUMPN(reg1, reg2, reg3, 7)	! \
	_DEBUG_DUMPN(reg1, reg2, reg3, 11)	! \
	_DEBUG_DUMPN(reg1, reg2, reg3, 15)	! \
	_DEBUG_DUMPN(reg1, reg2, reg3, 19)	! \
	_DEBUG_DUMPN(reg1, reg2, reg3, 23)	! \
	_DEBUG_DUMPN(reg1, reg2, reg3, 27)	! \
	_DEBUG_DUMPN(reg1, reg2, reg3, 31)

/*
 * hv-specific instructions
 */
#define	DR_PAGE0	diag (0x70 << 5)
#define	DR_PAGE1	diag (0x72 << 5)
#define	MTCPU_T(x,t)	diag ((t) << 21) | ((x) << 16) | (0xc0 << 5)
#define	MTCPU_C(x,t)	diag ((t) << 21) | ((x) << 16) | (0x12 << 5)
#define	MFCPU_T(r,x)	diag ((r) << 21) | (0xa0 << 5) | (x)
#define	MFCPU_C(r,x)	diag ((r) << 21) | ((x) << 16) | (0x30 << 5)

	.import	$global$, data
	.import pdc, data
	.import	boothowto, data
	.import	bootdev, data
	.import	esym, data
	.import	curproc, data
	.import	want_resched, data
	.import virtual_avail, data
	.import	proc0, data
	.import	proc0paddr, data
	.import	kpsw, data
	.import	panic, code
	.import fpu_csw, data
	.import fpu_cur_uspace, data

	.section .bss
	.export	pdc_stack, data
pdc_stack
	.comm	4*NBPG
exit_stack
	.comm	2*NBPG
kernelmapped			/* set when kernel is mapped */
	.comm	4

	.text

/*
 * This is the starting location for the kernel
 */
ENTRY($start,0)
/*
 *	start(pdc, boothowto, bootdev, esym, bootapiver, argv, argc)
 *
 *	pdc - PDC entry point (not used, HP-UX compatibility)
 *	boothowto - boot flags (see "reboot.h")
 *	bootdev - boot device (index into bdevsw)
 *	esym - end of symbol table (or &end if not present)
 *	bootapiver - /boot API version
 *	argv - options block passed from /boot
 *	argc - the length of the block
 */

	/*
	 * save the pdc, boothowto, bootdev and esym arguments
	 */
	ldil	L%pdc,r1
	stw	arg0,R%pdc(r1)
	ldil	L%boothowto,r1
	stw	arg1,R%boothowto(r1)
	ldil	L%bootdev,r1
	stw	arg2,R%bootdev(r1)
	ldil	L%esym,r1
	stw	arg3,R%esym(r1)

	/* Align arg3, which is the start of available memory */
	ldo	NBPG-1(arg3), arg3
	dep	r0, 31, PGSHIFT, arg3

	/*
	 * disable interrupts and turn off all bits in the psw so that
	 * we start in a known state.
	 */
	rsm	RESET_PSW, r0

	/*
	 * to keep the spl() routines consistent we need to put the correct
	 * spl level into eiem, and reset any pending interrupts
	 */
	ldi	-1, r1
	mtctl	r0, eiem	/* IPL_NONE */
	mtctl	r1, eirr

	/*
	 * set up the dp pointer so that we can do quick references off of it
	 */
	ldil	L%$global$,dp
	ldo	R%$global$(dp),dp

	/* zero fake trapframe and proc0 u-area */
	/* XXX - we should create a real trapframe for proc0 */
	copy	arg3, t2
	ldi	NBPG+TRAPFRAME_SIZEOF, t1
$start_zero_tf
	stws,ma r0, 4(t2)
	addib,>= -8, t1, $start_zero_tf
	stws,ma r0, 4(t2)	/* XXX could use ,bc here, but gas is broken */

	/*
	 * kernel stack lives here (arg3 is page-aligned esym)
	 * initialize the pcb
	 * arg0 will be available space for hppa_init()
	 */
	ldo	NBPG+TRAPFRAME_SIZEOF(arg3), sp
	mtctl	arg3, cr30
	stw	r0, U_PCB+PCB_ONFAULT(arg3)
	stw	r0, U_PCB+PCB_SPACE(arg3)	/* XXX HPPA_SID_KERNEL == 0 */
	stw	arg3, U_PCB+PCB_UVA(arg3)
	ldil	L%USPACE, arg0
	add	arg3, arg0, arg0
	ldil	L%proc0paddr, t1
	stw	arg3, R%proc0paddr(t1)
	ldil	L%proc0, t2
	stw	arg3, R%proc0+P_ADDR(t2)
	ldo	NBPG(arg3), t1
	stw	t1, R%proc0+P_MD_REGS(t2)

	ldil	L%TFF_LAST, t1
	stw	t1, TF_FLAGS-TRAPFRAME_SIZEOF(sp)
	stw	arg3, TF_CR30-TRAPFRAME_SIZEOF(sp)

	/*
	 * We need to set the Q bit so that we can take TLB misses after we
	 * turn on virtual memory.
	 */
	mtctl	r0, pcsq
	mtctl	r0, pcsq
	ldil	L%$qisnowon, t1
	ldo	R%$qisnowon(t1), t1
	mtctl	t1, pcoq
	ldo	4(t1),t1
	mtctl	t1, pcoq
	ldi	PSW_Q|PSW_I, t1
	mtctl	t1, ipsw
	rfi
	nop

$qisnowon
	/*
	 * load address of interrupt vector table
	 */
	ldil	L%$ivaaddr,t2
	ldo	R%$ivaaddr(t2),t2
	mtctl	t2,iva

	/*
	 * Create a stack frame for us to call C with. Clear out the previous
	 * sp marker to mark that this is the first frame on the stack.
	 */
	copy	sp, t1
	stwm	r0, HPPA_FRAME_SIZE(sp)
	copy	sp, r3
	stwm	t1, HPPA_FRAME_SIZE(sp)

	/*
	 * disable all coprocessors
	 */
	mtctl	r0, ccr

	/*
	 * call C routine hppa_init() to initialize VM
	 */
	.import hppa_init, code
	ldil	L%hppa_init, r1
	ldo	R%hppa_init(r1), r1
	.call
	blr	r0, rp
	bv,n	(r1)
	nop

	/*
	 * go to virtual mode...
	 * get things ready for the kernel to run in virtual mode
	 */
	ldi	HPPA_PID_KERNEL, r1
	mtctl	r1, pidr1
	mtctl	r1, pidr2
#if pbably_not_worth_it
	mtctl	r0, pidr3
	mtctl	r0, pidr4
#endif
	mtsp	r0, sr0
	mtsp	r0, sr1
	mtsp	r0, sr2
	mtsp	r0, sr3
	mtsp	r0, sr4
	mtsp	r0, sr5
	mtsp	r0, sr6
	mtsp	r0, sr7

	/*
	 * Cannot change the queues or IPSW with the Q-bit on
	 */
	rsm	RESET_PSW, r0

	/*
	 * We need to do an rfi to get the C bit set
	 */
	mtctl	r0, pcsq
	mtctl	r0, pcsq
	ldil	L%$virtual_mode, t1
	ldo	R%$virtual_mode(t1), t1
	mtctl	t1, pcoq
	ldo	4(t1), t1
	mtctl	t1, pcoq
	ldil	L%kpsw, t1
	ldw	R%kpsw(t1), t2
	mtctl	t2, ipsw
	rfi
	nop

$virtual_mode
	ldil	L%kernelmapped, t1
	stw	t1, R%kernelmapped(t1)

#ifdef DDB
	.import	Debugger, code
	/* have to call debugger from here, from virtual mode */
	ldil	L%boothowto, r1
	ldw	R%boothowto(r1), r1
	bb,>=	r1, 25, $noddb
	nop

	break	HPPA_BREAK_KERNEL, HPPA_BREAK_KGDB
	nop
$noddb
#endif

	.import main,code
	ldil	L%main, r1
	ldo	R%main(r1), r1
$callmain
	.call
	blr	r0, rp
	bv,n	(r1)
	nop

	/* should never return... */
	bv	(rp)
	nop
EXIT($start)

/* int
 * pdc_call(func, pdc_flag, ...)
 *	iodcio_t func;
 *	int pdc_flag;
 */
ENTRY(pdc_call,160)

	mfctl	eiem, t1
	mtctl	r0, eiem
	stw	rp, HPPA_FRAME_CRP(sp)
	copy	arg0, r31
	copy	sp, ret1

	ldil	L%kernelmapped, ret0
	ldw	R%kernelmapped(ret0), ret0
	comb,=	r0, ret0, pdc_call_unmapped1
	nop
	ldil	L%pdc_stack, ret1
	ldo	R%pdc_stack(ret1), ret1

pdc_call_unmapped1
	copy	sp, r1
	ldo	HPPA_FRAME_SIZE+24*4(ret1), sp

	stw	r1, HPPA_FRAME_PSP(sp)

	/* save kernelmapped and eiem */
	stw	ret0, HPPA_FRAME_ARG(21)(sp)
	stw	t1, HPPA_FRAME_ARG(22)(sp)

	/* copy arguments */
	copy	arg2, arg0
	copy	arg3, arg1
	ldw	HPPA_FRAME_ARG(4)(r1), arg2
	ldw	HPPA_FRAME_ARG(5)(r1), arg3
	ldw	HPPA_FRAME_ARG(6)(r1), t1
	ldw	HPPA_FRAME_ARG(7)(r1), t2
	ldw	HPPA_FRAME_ARG(8)(r1), t3
	ldw	HPPA_FRAME_ARG(9)(r1), t4
	stw	t1, HPPA_FRAME_ARG(4)(sp)	/* XXX can use ,bc */
	stw	t2, HPPA_FRAME_ARG(5)(sp)
	stw	t3, HPPA_FRAME_ARG(6)(sp)
	stw	t4, HPPA_FRAME_ARG(7)(sp)
	ldw	HPPA_FRAME_ARG(10)(r1), t1
	ldw	HPPA_FRAME_ARG(11)(r1), t2
	ldw	HPPA_FRAME_ARG(12)(r1), t3
	ldw	HPPA_FRAME_ARG(13)(r1), t4
	stw	t1, HPPA_FRAME_ARG(8)(sp)
	stw	t2, HPPA_FRAME_ARG(9)(sp)
	stw	t3, HPPA_FRAME_ARG(10)(sp)
	stw	t4, HPPA_FRAME_ARG(11)(sp)

	/* save temp control regs */
	mfctl	cr24, t1
	mfctl	cr25, t2
	mfctl	cr26, t3
	mfctl	cr27, t4
	stw	t1, HPPA_FRAME_ARG(12)(sp)	/* XXX can use ,bc */
	stw	t2, HPPA_FRAME_ARG(13)(sp)
	stw	t3, HPPA_FRAME_ARG(14)(sp)
	stw	t4, HPPA_FRAME_ARG(15)(sp)
	mfctl	cr28, t1
	mfctl	cr29, t2
	mfctl	cr30, t3
	mfctl	cr31, t4
	stw	t1, HPPA_FRAME_ARG(16)(sp)
	stw	t2, HPPA_FRAME_ARG(17)(sp)
	stw	t3, HPPA_FRAME_ARG(18)(sp)
	stw	t4, HPPA_FRAME_ARG(19)(sp)

	comb,=	r0, ret0, pdc_call_unmapped2
	nop

	copy	arg0, t4
	ldi	PSW_Q, arg0 /* (!pdc_flag && args[0] == PDC_PIM)? PSW_M:0) */
	break	HPPA_BREAK_KERNEL, HPPA_BREAK_SET_PSW
	nop
	stw	ret0, HPPA_FRAME_ARG(23)(sp)
	copy	t4, arg0

pdc_call_unmapped2
	.call
	blr	r0, rp
	bv,n	(r31)
	nop

	/* load temp control regs */
	ldw	HPPA_FRAME_ARG(12)(sp), t1
	ldw	HPPA_FRAME_ARG(13)(sp), t2
	ldw	HPPA_FRAME_ARG(14)(sp), t3
	ldw	HPPA_FRAME_ARG(15)(sp), t4
	mtctl	t1, cr24
	mtctl	t2, cr25
	mtctl	t3, cr26
	mtctl	t4, cr27
	ldw	HPPA_FRAME_ARG(16)(sp), t1
	ldw	HPPA_FRAME_ARG(17)(sp), t2
	ldw	HPPA_FRAME_ARG(18)(sp), t3
	ldw	HPPA_FRAME_ARG(19)(sp), t4
	mtctl	t1, cr28
	mtctl	t2, cr29
	mtctl	t3, cr30
	mtctl	t4, cr31

	ldw	HPPA_FRAME_ARG(21)(sp), t1
	ldw	HPPA_FRAME_ARG(22)(sp), t2
	comb,=	r0, t1, pdc_call_unmapped3
	nop

	copy	ret0, t3
	ldw	HPPA_FRAME_ARG(23)(sp), arg0
	break	HPPA_BREAK_KERNEL, HPPA_BREAK_SET_PSW
	nop
	copy	t3, ret0

pdc_call_unmapped3
	ldw	HPPA_FRAME_PSP(sp), sp
	ldw	HPPA_FRAME_CRP(sp), rp
	bv	r0(rp)
	mtctl	t2, eiem
EXIT(pdc_call)

/*
 * int spllower(int ncpl);
 */
ENTRY(spllower,64)
	ldil	L%ipending, %r1
	ldw	R%ipending(%r1), %r1	; load ipending
	ldil	L%cpl, t1
	andcm,<> %r1, %arg0, %r1	; and with complement of new cpl
	bv	%r0(%rp)
	stw	%arg0, R%cpl(t1)	; store new cpl

	/*
	 * Dispatch interrupts.  There's a chance
	 * that we may end up not dispatching anything;
	 * in between our load of ipending and this
	 * disabling of interrupts, something else may
	 * have come in and dispatched some or all
	 * of what we previously saw in ipending.
	 */
	mfctl	%eiem, %arg1
	mtctl	%r0, %eiem		; disable interrupts

	ldil	L%ipending, %r1
	ldw	R%ipending(%r1), %r1	; load ipending
	andcm,<> %r1, %arg0, %r1	; and with complement of new cpl
	b,n	spllower_out		; branch if we got beaten
	
spllower_dispatch
	/* start stack calling convention */
	stw	%rp, HPPA_FRAME_CRP(%sp)
	copy	%r3, %r1
	copy	%sp, %r3
	stw,ma	%r1, HPPA_FRAME_SIZE(%sp)

	/* save ncpl and %eiem */
	stw	%arg0, HPPA_FRAME_ARG(0)(%r3)
	stw	%arg1, HPPA_FRAME_ARG(1)(%r3)

	/* call hp700_intr_dispatch */
	ldil	L%hp700_intr_dispatch, %r1
	ldo	R%hp700_intr_dispatch(%r1), %r1
	blr	%r0, %rp
	.call
	bv	%r0(%r1)
	copy	%r0, %arg2		; call with a NULL frame
	
	/* restore %eiem, we don't need ncpl */
	ldw	HPPA_FRAME_ARG(1)(%r3), %arg1

	/* end stack calling convention */
	ldw	HPPA_FRAME_CRP(%r3), %rp
	ldo	HPPA_FRAME_SIZE(%r3), %sp
	ldw,mb	-HPPA_FRAME_SIZE(%sp), %r3
	
spllower_out
	/*
	 * Now return, storing %eiem in the delay slot.
	 * (hp700_intr_dispatch leaves it zero).  I think 
	 * doing this in the delay slot is important to 
	 * prevent recursion, but I might be being too 
	 * paranoid.
	 */
	bv	%r0(%rp)
	mtctl	%arg1, %eiem
EXIT(spllower)

/*
 * void hp700_intr_schedule(int mask);
 */
ENTRY(hp700_intr_schedule,64)
	ldil	L%ipending, t1
	ldil	L%cpl, t2
	mfctl	%eiem, %arg1
	mtctl	%r0, %eiem			; disable interrupts
	ldw	R%ipending(t1), %r1		; load ipending
	or	%r1, %arg0, %r1			; or in mask
	stw	%r1, R%ipending(t1)		; store ipending
	ldw	R%cpl(t2), %arg0		; load cpl
	andcm,= %r1, %arg0, %r1			; and ipending with ~cpl
	b,n	spllower_dispatch		; dispatch if we can
	bv	%r0(%rp)
	mtctl	%arg1, %eiem
EXIT(hp700_intr_schedule)

/*
 * void cpu_die(void);
 */
ENTRY(cpu_die, 0)
	rsm	RESET_PSW, r0
	nop
	nop
	mtsp	r0, sr0
	ldil	L%LBCAST_ADDR, %r25
	ldi	CMD_RESET, %r26
	stw	%r26, R%iomod_command(%r25)
forever					; Loop until bus reset takes effect.
	b,n	forever
	nop
	nop
EXIT(cpu_die)

/*
 * adjust the time value
 * XXX: do it the easy way, later we will calculate actual fuzz from itr
 */
LEAF_ENTRY(microtime)

	.import time, data
	ldil	L%-1000000, t3
	ldil	L%time, t1
	ldo	R%-1000000(t3), t3

	/* t4 = splhigh() */
	mfctl	eiem, t4
	mtctl	r0, eiem

	ldw	R%time+4(t1), t2
	ldw	R%time(t1), t1

	/* splx(t4) */
	mtctl	t4, eiem

	addi	1, t2, t2
	addb,<,n	t2, t3, microtime_no
	addi	1, t1, t1

	copy	t3, t2

microtime_no
	stwm	t1, 4(arg0)
	bv	(rp)
	stw	t2, 0(arg0)

EXIT(microtime)

/* Include the system call and trap handling. */
#include <hppa/hppa/trap.S>

/* Include the userspace copyin/copyout functions. */
#include <hppa/hppa/copy.S>

/* Include the process subroutines. */
#include <hppa/hppa/proc_subr.S>

/* Include the support functions. */
#include <hppa/hppa/support.S>

/*
 * cpu_switch()
 * Find the highest priority process and resume it.
 */
	.align	32
ENTRY(cpu_switch,128)

	/*
	 * Clear curproc so that we don't accumulate system time while idle.
	 */
	ldil	L%curproc, t1
	ldw	R%curproc(t1), arg2
	stw	r0, R%curproc(t1)
	/* remain on the old (curproc)'s stack until we have a better choice */

	/*
	 * arg3: spl
	 * t1:	&sched_whichqs
	 * t2:	old curproc
	 *
	 */

switch_search
	/* start stack calling convention */
	stw	%rp, HPPA_FRAME_CRP(%sp)
	copy	%r3, %r1
	copy	%sp, %r3
	stw,ma	%r1, HPPA_FRAME_SIZE(%sp)

	/* save cpl and old curproc */
	ldil	L%cpl, %arg0
	ldw	R%cpl(%arg0), %arg0
	stw	%arg0, HPPA_FRAME_ARG(0)(%r3)
	stw	%arg2, HPPA_FRAME_ARG(1)(%r3)

idle_loop
	/*
	 * XXX - We should be at splsched(), and as
	 * such we shouldn't have to disable interrupts.
	 */
	ldil	L%sched_whichqs, t1
	ldw	R%sched_whichqs(t1), t3

	comb,<>	r0, t3, gotprocs
	nop

#if defined(LOCKDEBUG)
	/* Release sched_lock */ 
	.call
	bl	sched_unlock_idle, %rp
	nop
#endif  

	/* spl0() */
	.call
	bl	spllower, %rp
	copy	%r0, %arg0
	
	/* XXX do idle work here */
	nop ! nop ! nop ! nop ! nop ! nop ! nop ! nop

	/* splsched() */
	.call
	bl	spllower, %rp
	ldw	HPPA_FRAME_ARG(0)(%r3), %arg0

#if defined(LOCKDEBUG)
	/* Acquire sched_lock */
	.call
	bl	sched_lock_idle, %rp
	nop
#endif

	b	idle_loop
	nop

gotprocs
	/* recover old curproc */
	ldw	HPPA_FRAME_ARG(1)(%r3), %arg2

#if 0
	/* XXX debugging - break if old curproc is NULL */
	comb,<>,n %arg2, %r0, curprocok
	nop
	break	0, 5
curprocok
#endif

	/* end stack calling convention */
	ldw	HPPA_FRAME_CRP(%r3), %rp
	ldo	HPPA_FRAME_SIZE(%r3), %sp
	ldw,mb	-HPPA_FRAME_SIZE(%sp), %r3

	ldi	0, t4
getbit
	mtsar	t4
	bvb,>=,n t3, getbit
	ldo	1(t4), t4

	ldil	L%sched_qs, t2
	ldo	R%sched_qs(t2), t2
	sh3add	t4, t2, t2

	ldw	P_FORW(t2), arg1
#ifdef DIAGNOSTIC
	comb,<>	t2, arg1, link_ok
	nop
switch_error
	copy	t4, arg1
	copy	t2, arg2
	ldil	L%panic, r1
	ldil	L%Lcspstr, arg0
	ldo	R%panic(r1), r1
	ldo	R%Lcspstr(arg0), arg0
	.call
	blr	%r0, rp
	bv,n	%r0(r1)
	nop
Lcspstr
	.asciz	"cpu_switch: bit=%x, q/p=%p"
	.align	8
link_ok
#endif
	ldw	P_FORW(arg1), arg0
	stw	arg0, P_FORW(t2)
	stw	t2, P_BACK(arg0)
	stw	r0, P_BACK(arg1)

	comb,<> arg0, t2, sw_qnempty
	nop

	vdepi	0, 1, t3
	stw	t3, R%sched_whichqs(t1)

	/* don't need &sched_whichqs (t1) starting here */
sw_qnempty
	ldil	L%want_resched, t3
	stw	r0, R%want_resched(t3)

#ifdef DIAGNOSTIC
	/* The new process must have no wait channel. */
	ldw	P_WCHAN(arg1), t1
	comb,<>,n r0, t1, switch_error
	copy	arg1, t2
	/* The new process' state must be SRUN. */
	ldb	P_STAT(arg1), t1
	comib,<>,n SRUN, t1, switch_error
	copy	arg1, t2
	/*
	 * Either we must be switching to the same process, or
	 * the the new process' kernel stack must be reasonable.
	 */
	comb,=,n arg1, arg2, kstack_ok
	ldw	P_MD_REGS(arg1), t1
	ldw	P_ADDR(arg1), arg0
	ldw	TF_R30(t1), t1
	ldo	NBPG(arg0), arg0
	comb,>>,n arg0, t1, switch_error
	copy	arg1, t2
	sub	t1, arg0, t1
	ldil	L%USPACE, arg0
	ldo	R%USPACE(arg0), arg0
	comb,<<=,n arg0, t1, switch_error
	copy	arg1, t2
kstack_ok
#endif
	ldi	SONPROC, t1
	stb	t1, P_STAT(arg1)
	ldil	L%curproc, t1
	/*
	 * Disable interrupts while we're doing the real
	 * switch.  Is this just being paranoid?
	 */
	mfctl	eiem, arg3
	mtctl	r0, eiem
	stw	arg1, R%curproc(t1)

	/* Skip context switch if same process. */
	comb,=,n arg1, arg2, switch_return

	/* If old process exited, don't bother. */
	comb,=,n r0, arg2, switch_exited

	/*
	 * 2. save old proc context
	 *
	 * arg2: old proc
	 */
	ldw	P_MD(arg2), t1
	copy	sp, t2
	ldo	HPPA_FRAME_SIZE+16*4(sp), sp
	ldw	TF_R30(t1), t3
	stw	t2, HPPA_FRAME_PSP(sp)
	stw	rp, HPPA_FRAME_CRP(sp)
	stw	t3, HPPA_FRAME_ARG(0)(sp)
	stw	sp, TF_R30(t1)
	fdc	r0(t1)
	/* save callee-save registers */
	stw	r3,   0*4(t2)
	stw	r4,   1*4(t2)
	stw	r5,   2*4(t2)
	stw	r6,   3*4(t2)
	stw	r7,   4*4(t2)
	stw	r8,   5*4(t2)
	stw	r9,   6*4(t2)
	stw	r10,  7*4(t2)
	stw	r11,  8*4(t2)
	stw	r12,  9*4(t2)
	stw	r13, 10*4(t2)
	stw	r14, 11*4(t2)
	stw	r15, 12*4(t2)
	stw	r16, 13*4(t2)
	stw	r17, 14*4(t2)
	stw	r18, 15*4(t2)

	/* don't need old curproc (arg2) starting from here */
switch_exited
	/*
	 * 3. restore new proc context
	 *
	 * arg1: new proc
	 */
	ldw	P_MD(arg1), t1
	ldw	TF_CR30(t1), t2
	ldw	TF_R30(t1), sp
	ldw	TF_CR9(t1), t3
	mtctl	t3, pidr2
	mtctl	t2, cr30
	ldw	HPPA_FRAME_ARG(0)(sp), t3
	ldw	HPPA_FRAME_CRP(sp), rp
	ldw	HPPA_FRAME_PSP(sp), t2
	stw	t3, TF_R30(t1)
	fdc	r0(t1)
	ldw	HPPA_FRAME_ARG(1)(sp), t3 /* in case we're in trampoline */
	ldw	HPPA_FRAME_ARG(2)(sp), arg0
	ldw	 0*4(t2), r3
	ldw	 1*4(t2), r4
	ldw	 2*4(t2), r5
	ldw	 3*4(t2), r6
	ldw	 4*4(t2), r7
	ldw	 5*4(t2), r8
	ldw	 6*4(t2), r9
	ldw	 7*4(t2), r10
	ldw	 8*4(t2), r11
	ldw	 9*4(t2), r12
	ldw	10*4(t2), r13
	ldw	11*4(t2), r14
	ldw	12*4(t2), r15
	ldw	13*4(t2), r16
	ldw	14*4(t2), r17
	ldw	15*4(t2), r18
	copy	t2, sp

	/*
	 * As an optimization, hppa_fpu_bootstrap
	 * replaces this branch instruction with a
	 * nop if there is a hardware FPU.
	 */
ALTENTRY(hppa_fpu_nop1)
	b,n	switch_return

	/*
	 * We do have a hardware FPU.  If the process 
	 * that we just switched to has its state in the
	 * FPU, enable the FPU, else disable it, so if 
	 * the process does try to use the coprocessor 
	 * we'll get an assist emulation trap to swap 
	 * states.
	 */
	ldil	L%fpu_cur_uspace, t1
	mfctl	ccr, %r1
	mfctl	cr30, t2
	ldw	R%fpu_cur_uspace(t1), t1
	depi	0, 25, 2, %r1		; disables the FPU
	comb,<>,n t1, t2, 0		; nullify if procs different
	depi	3, 25, 2, %r1		; enables the FPU
	mtctl	%r1, ccr

switch_return
	bv	0(rp)
	mtctl	arg3, eiem

EXIT(cpu_switch)

/*
 * switch_exit(struct proc *p)
 * restore proc0 context and go into cpu_switch to select the next runable
 * process.
 */
	.import	kernel_map, data
	.import	uvmspace_free, code
	.import	uvm_km_free, code
ENTRY(switch_exit,0)

	/* setup kernel context */
	mtctl	r0, sr0
	mtctl	r0, sr1
	mtctl	r0, sr2
	mtctl	r0, sr3
	mtctl	r0, sr4
	mtctl	r0, sr5
	mtctl	r0, sr6
	mtctl	r0, sr7

	ldi	HPPA_PID_KERNEL, t4
	mtctl	t4, pidr2

	/* switch onto the temporary stack */
	ldil	L%exit_stack, t4
	ldo	R%exit_stack(t4), t4
	ldo	HPPA_FRAME_SIZE(t4), t4
	stw	r0, HPPA_FRAME_PSP(t4)
	stw	r0, HPPA_FRAME_CRP(t4)
	copy	t4, sp

	/* start the stack frame for our callee */
	copy	sp, r3
	ldo	HPPA_FRAME_SIZE(sp), sp
	stw	r3, HPPA_FRAME_PSP(sp)

	/* arg0 -- oldproc */
	.import exit2, code
	ldil	L%exit2, t2
	ldo	R%exit2(t2), t2
	.call
	blr	%r0, rp
	bv,n	%r0(t2)
	nop

#if defined(LOCKDEBUG)
	/* Acquire sched_lock */
	.call
	bl	sched_lock_idle, %rp
	nop
#endif

	.call
	b	switch_search
	copy	r0, arg2	/* no old proc */
EXIT(switch_exit)

/*
 * This is the first code run in a new process after
 * cpu_switch() has switched to it for the first time.
 * This happens courtesy of the setup in cpu_fork(),
 * which also makes sure that %t3 is the address of
 * the first kernel function to call, and %arg0 is
 * its argument.
 */
ENTRY(switch_trampoline,64)

	/* no return point */
	copy	%r0, %rp

	/* start stack calling convention */
	stw	%rp, HPPA_FRAME_CRP(%sp)
	copy	%r3, %r1
	copy	%sp, %r3
	stw,ma	%r1, HPPA_FRAME_SIZE(%sp)

	/* save %arg0, %t3 */
	stw	%arg0, HPPA_FRAME_ARG(0)(%r3)
	stw	t3, HPPA_FRAME_ARG(1)(%r3)

	/* new children start at spl0 */
	.call
	bl	spllower, %rp
	copy	%r0, %arg0

	/* restore %arg0, %t3 */
	ldw	HPPA_FRAME_ARG(0)(%r3), %arg0
	ldw	HPPA_FRAME_ARG(1)(%r3), t3

	/* end stack calling convention */
	ldw	HPPA_FRAME_CRP(%r3), %rp
	ldo	HPPA_FRAME_SIZE(%r3), %sp
	ldw,mb	-HPPA_FRAME_SIZE(%sp), %r3

	/* call the first kernel function */
	.call
	blr	r0, rp
	bv,n	r0(t3)
	nop

	/*
	 * Since the first kernel function returned,
	 * this process was created by the fork()
	 * syscall, which we now return from.
	 */
	ldil	L%curproc, t1
	ldw	R%curproc(t1), t2
	.call
	b	$syscall_return
	ldw	P_MD(t2), t3
EXIT(switch_trampoline)

/* Include the signal code. */
#include <hppa/hppa/sigcode.S>

	.end
