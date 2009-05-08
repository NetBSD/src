/*	$NetBSD: locore.S,v 1.33 2009/05/08 09:33:58 skrll Exp $	*/
/*	$OpenBSD: locore.S,v 1.158 2008/07/28 19:08:46 miod Exp $	*/

/*
 * Copyright (c) 1998-2004 Michael Shalayeff
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

#include "opt_ddb.h"
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

#include "assym.h"

/* Some aliases for the macros in assym.h. */
#define	TRAPFRAME_SIZEOF	trapframe_SIZEOF

#define	ccr	cr10
#define	rctr	cr0

/*
 * Very crude debugging macros that write to com1.
 */

#if 1
#define	COM1_TX_REG	(0xffd00000 + 0x5000 + 0x800)
#else
#define	COM1_TX_REG	(0xf0823000 + 0x800)
#endif
#define _DEBUG_PUTCHAR(reg1, reg2)		! \
	ldil	L%COM1_TX_REG, %reg1		! \
	stb	%reg2, R%COM1_TX_REG(%sr1, %reg1) ! \
	ldil	L%10000000, %reg1		! \
	ldi	1, %reg2			! \
	comb,<>,n	%reg1, %r0, -8		! \
	sub	%reg1, %reg2, %reg1
#define DEBUG_PUTCHAR(reg1, reg2, ch)		! \
	ldi	ch, %reg2			! \
	_DEBUG_PUTCHAR(reg1,reg2)
#define _DEBUG_DUMPN(reg1, reg2, reg3, p)	! \
	extru	%reg3, p, 4, %reg2		! \
	comib,>>,n	10, %reg2, 0		! \
	addi	39, %reg2, %reg2		! \
	addi	48, %reg2, %reg2		! \
	_DEBUG_PUTCHAR(reg1,reg2)
#define DEBUG_DUMP32(reg1, reg2, reg3)		! \
	DEBUG_PUTCHAR(reg1,reg2,58)		! \
	_DEBUG_DUMPN(reg1, reg2, reg3, 3)	! \
	_DEBUG_DUMPN(reg1, reg2, reg3, 7)	! \
	_DEBUG_DUMPN(reg1, reg2, reg3, 11)	! \
	_DEBUG_DUMPN(reg1, reg2, reg3, 15)	! \
	_DEBUG_DUMPN(reg1, reg2, reg3, 19)	! \
	_DEBUG_DUMPN(reg1, reg2, reg3, 23)	! \
	_DEBUG_DUMPN(reg1, reg2, reg3, 27)	! \
	_DEBUG_DUMPN(reg1, reg2, reg3, 31)

#ifdef XXXNH
/*
 * hv-specific instructions
 */
#define	DR_PAGE0	diag (0x70 << 5)				/* XXXNH Different */
#define	DR_PAGE1	diag (0x72 << 5)				/* XXXNH Different */
#define	MTCPU_T(x,t)	diag ((t) << 21) | ((x) << 16) | (0xc0 << 5)	/* XXXNH Different */
#define	MFCPU_T(r,x)	diag ((r) << 21) | (0xa0 << 5) | (x)
#define	MTCPU_C(x,t)	diag ((t) << 21) | ((x) << 16) | (0x12 << 5)
#define	MFCPU_C(r,x)	diag ((r) << 21) | ((x) << 16) | (0x30 << 5)
#define	MFCPU_U(r,x)	.word	0x140008a0 | ((r) << 21) | ((x))
#define	MTCPU_U(x,r)	.word	0x14001840 | ((r) << 21) | ((x) << 16)

#else

#define	DR_PAGE0	.word	0x14001200
#define	DR_PAGE1	.word	0x14001240
#define	MTCPU_T(x,t)	.word	0x14001400 | ((t) << 21) | ((x) << 16)
#define	MFCPU_T(r,x)	.word	0x14001400 | ((r) << 21) | (x)
#define	MTCPU_C(x,t)	.word	0x14000240 | ((t) << 21) | ((x) << 16)
#define	MFCPU_C(r,x)	.word	0x14000600 | ((r) << 21) | ((x) << 16)
#define	MFCPU_U(r,x)	.word	0x140008a0 | ((r) << 21) | ((x))
#define	MTCPU_U(x,r)	.word	0x14001840 | ((r) << 21) | ((x) << 16)
#endif

	.import	$global$, data
	.import pdc, data
	.import	boothowto, data
	.import	bootdev, data
	.import	esym, data
	.import virtual_avail, data
	.import	lwp0, data
	.import	proc0paddr, data
	.import	kpsw, data
	.import	panic, code
	.import fpu_csw, data
	.import fpu_cur_uspace, data
	.import hp700_int_regs, data

#if 0
	BSS(pdc_stack, 4)	/* temp stack for PDC call */
	BSS(kernelmapped, 4)	/* set when kernel is mapped */
	BSS(hppa_vtop, 4)	/* a vtop translation table addr (pa=va) */
#else
	.section .bss

	.export	pdc_stack, data
pdc_stack:			/* temp stack for PDC call */
	.block	4

	.export	kernelmapped, data
kernelmapped:			/* set when kernel is mapped */
	.block	4

	.export hppa_vtop, data
hppa_vtop:			/* a vtop translation table addr (pa=va) */
	.block	4
#endif

	.text
	.import kernel_setup, entry

/*
 * This is the starting location for the kernel
 */
ENTRY_NOPROFILE(start,0)
/*
 *	bootapiver <= 2
 *		start(pdc, boothowto, bootdev, esym, bootapiver, argv, argc)
 *
 *	bootapiver == 3
 *		start(pdc, boothowto, bootdev, esym, bootapiver, bootinfo)
 *
 *	pdc - PDC entry point
 *	boothowto - boot flags (see "reboot.h")
 *	bootdev - boot device (index into bdevsw)
 *	esym - end of symbol table (or &end if not present)
 *	bootapiver - /boot API version
 *	argv - options block passed from /boot
 *	argc - the length of the block
 *	bootinfo - pointer to a struct bootinfo.
 */

	/*
	 * save the pdc, boothowto, bootdev and esym arguments
	 */
	ldil	L%pdc,%r1
	stw	%arg0,R%pdc(%r1)
	ldil	L%boothowto,%r1
	stw	%arg1,R%boothowto(%r1)
	ldil	L%bootdev,%r1
	stw	%arg2,R%bootdev(%r1)
	ldil	L%esym,%r1
	stw	%arg3,R%esym(%r1)

	/*
	 * Put page aligned %arg3 into %t3. It is the start of available
	 * memory.
	 */
	ldo	NBPG-1(%arg3), %t3
	dep	%r0, 31, PGSHIFT, %t3

	/* bootinfo struct address for hppa_init, if bootapiver is > 2 */
	ldw	HPPA_FRAME_ARG(4)(%sp), %t1
	ldw	HPPA_FRAME_ARG(5)(%sp), %r5
	comiclr,< 2, %t1, %r0
	copy	%r0, %r5

	/* assuming size being page-aligned */
#define STACK_ALLOC(n,s)		\
	ldil	L%(n), %t1		! \
	ldil	L%(s), %t2		! \
	stw	%t3, R%(n)(%t1)		! \
	add	%t3, %t2, %t3

	STACK_ALLOC(pdc_stack, PDC_STACKSIZE)

	/* zero fake trapframe and lwp0 u-area */
	/* XXX - we should create a real trapframe for lwp0 */
	copy	%t3, %t2
	ldi	NBPG+TRAPFRAME_SIZEOF, %t1
$start_zero_tf:
	stws,ma %r0, 4(%t2)
	addib,>= -8, %t1, $start_zero_tf
	stws,ma %r0, 4(%t2)	/* XXX could use ,bc here, but gas is broken */

	/*
	 * kernel stack starts a page and a trapframe above l_addr.
	 */
	ldo	NBPG+TRAPFRAME_SIZEOF(%t3), %sp
	mtctl	%t3, CR_UPADDR

	/* initialize the pcb */
	stw	%r0, U_PCB+PCB_ONFAULT(%t3)
	stw	%r0, U_PCB+PCB_SPACE(%t3)	/* XXX HPPA_SID_KERNEL == 0 */
	stw	%t3, U_PCB+PCB_UVA(%t3)

	/*
	 * Setup various pointers.
	 *
	 * First free memory is %t3 plus normal U space. The last page of
	 * USPACE is the redzone if DIAGNOSTIC (see param.h).
	 */
	ldil	L%USPACE, %r4
	add	%t3, %r4, %r4

	ldil	L%proc0paddr, %t1
	stw	%t3, R%proc0paddr(%t1)
	ldil	L%lwp0, %t2
	stw	%t3, R%lwp0+L_ADDR(%t2)
	ldo	NBPG(%t3), %t1
	stw	%t1, R%lwp0+L_MD_REGS(%t2)

	ldil	L%TFF_LAST, %t1
	stw	%t1, TF_FLAGS-TRAPFRAME_SIZEOF(%sp)
	stw	%t3, TF_CR30-TRAPFRAME_SIZEOF(%sp)

	/*
	 * disable all coprocessors
	 */
	mtctl	%r0, %ccr

	/*
	 * We need to set the Q bit so that we can take TLB misses after we
	 * turn on virtual memory.
	 */
	copy	%sp, %arg0
	ldil	L%$qisnowon, %rp
	ldo	R%$qisnowon(%rp), %rp

	b	kernel_setup
	ldi	PSW_Q|PSW_I, %arg1

$qisnowon:
	copy	%r4, %arg0
	copy	%r5, %arg1
	/*
	 * call C routine hppa_init() to initialize VM
	 */
	.import hppa_init, code
	CALL(hppa_init, %r1)

	/*
	 * Cannot change the queues or IPSW with the Q-bit on
	 */
	rsm	RESET_PSW, %r0
	nop ! nop ! nop ! nop ! nop ! nop ! nop

	/*
	 * We need to do an rfi to get the C bit set
	 */
	mtctl	%r0, %pcsq
	mtctl	%r0, %pcsq
	ldil	L%$virtual_mode, %t1
	ldo	R%$virtual_mode(%t1), %t1
	mtctl	%t1, %pcoq
	ldo	4(%t1), %t1
	mtctl	%t1, %pcoq
	ldil	L%kpsw, %t1
	ldw	R%kpsw(%t1), %t2
	mtctl	%t2, %ipsw
	rfi
	nop
	nop
	nop
	nop
	nop
	nop
	nop

$virtual_mode:

	ldil	L%kernelmapped, %t1
	stw	%t1, R%kernelmapped(%t1)

#ifdef DDB
	.import	Debugger, code
	/* have to call debugger from here, from virtual mode */
	ldil	L%boothowto, %r1
	ldw	R%boothowto(%r1), %r1
	bb,>=	%r1, 25, $noddb
	nop

	break	HPPA_BREAK_KERNEL, HPPA_BREAK_KGDB
	nop
$noddb:
#endif

	.import main,code
	CALL(main, %r1)
	/* should never return... */
	bv	(%rp)
	nop
EXIT(start)


/*
 * void kernel_setup(register_t sp, register_t psw)
 */
LEAF_ENTRY_NOPROFILE(kernel_setup)

	/*
	 * disable interrupts and turn off all bits in the psw so that
	 * we start in a known state.
	 */
	rsm	RESET_PSW, %r0
	nop ! nop ! nop ! nop ! nop ! nop

	/*
	 * go to virtual mode...
	 * get things ready for the kernel to run in virtual mode
	 */
	ldi	HPPA_PID_KERNEL, %r1
	mtctl	%r1, %pidr1
	mtctl	%r1, %pidr2
#if pbably_not_worth_it
	mtctl	%r0, %pidr3
	mtctl	%r0, %pidr4
#endif
	mtsp	%r0, %sr0
	mtsp	%r0, %sr1
	mtsp	%r0, %sr2
	mtsp	%r0, %sr3
	mtsp	%r0, %sr4
	mtsp	%r0, %sr5
	mtsp	%r0, %sr6
	mtsp	%r0, %sr7

	/*
	 * to keep the spl() routines consistent we need to put the correct
	 * spl level into eiem, and reset any pending interrupts
	 */
	ldi	-1, %r1
	mtctl	%r0, %eiem	/* IPL_NONE */
	mtctl	%r1, %eirr

	/*
	 * load address of interrupt vector table
	 */
	ldil	L%$ivaaddr,%t2
	ldo	R%$ivaaddr(%t2),%t2
	mtctl	%t2,%iva

	/*
	 * set up the dp pointer so that we can do quick references off of it
	 */
	ldil	L%$global$,%dp
	ldo	R%$global$(%dp),%dp

	/*
	 * Create a stack frame for us to call C with. Clear out the previous
	 * sp marker to mark that this is the first frame on the stack.
	 */
	copy	%arg0, %sp
	ldo	0(%arg0), %r3
	stw,ma	%r0, HPPA_FRAME_SIZE(%sp)
	stw	%r0, HPPA_FRAME_CRP(%sp)
	stw	%r0, HPPA_FRAME_PSP(%sp)

	/*
	 * We need to set the Q bit so that we can take TLB misses after we
	 * turn on virtual memory.
	 */

	mtctl	%r0, %pcsq
	mtctl	%r0, %pcsq
	mtctl	%rp, %pcoq
	ldo	4(%rp),%rp
	mtctl	%rp, %pcoq
	mtctl	%arg1, %ipsw
	rfi
	nop
	nop
EXIT(kernel_setup)

/*
 * int pdc_call(iodcio_t func,int pdc_flag, ...)
 */
ENTRY(pdc_call,160)

	mfctl	%eiem, %t1
	mtctl	%r0, %eiem
	stw	%rp, HPPA_FRAME_CRP(%sp)
	copy	%arg0, %r31
	copy	%sp, %ret1

	ldil	L%kernelmapped, %ret0
	ldw	R%kernelmapped(%ret0), %ret0
	comb,=	%r0, %ret0, pdc_call_unmapped1
	nop

	ldil	L%pdc_stack, %ret1
	ldw	R%pdc_stack(%ret1), %ret1

pdc_call_unmapped1:
	copy	%sp, %r1
	ldo	HPPA_FRAME_SIZE+24*4(%ret1), %sp

	stw	%r1, HPPA_FRAME_PSP(%sp)

	/* save kernelmapped and eiem */
	stw	%ret0, HPPA_FRAME_ARG(21)(%sp)
	stw	%t1, HPPA_FRAME_ARG(22)(%sp)

	/* copy arguments */
	copy	%arg2, %arg0
	copy	%arg3, %arg1
	ldw	HPPA_FRAME_ARG(4)(%r1), %arg2
	ldw	HPPA_FRAME_ARG(5)(%r1), %arg3
	ldw	HPPA_FRAME_ARG(6)(%r1), %t1
	ldw	HPPA_FRAME_ARG(7)(%r1), %t2
	ldw	HPPA_FRAME_ARG(8)(%r1), %t3
	ldw	HPPA_FRAME_ARG(9)(%r1), %t4
	stw	%t1, HPPA_FRAME_ARG(4)(%sp)	/* XXX can use ,bc */
	stw	%t2, HPPA_FRAME_ARG(5)(%sp)
	stw	%t3, HPPA_FRAME_ARG(6)(%sp)
	stw	%t4, HPPA_FRAME_ARG(7)(%sp)
	ldw	HPPA_FRAME_ARG(10)(%r1), %t1
	ldw	HPPA_FRAME_ARG(11)(%r1), %t2
	ldw	HPPA_FRAME_ARG(12)(%r1), %t3
	ldw	HPPA_FRAME_ARG(13)(%r1), %t4
	stw	%t1, HPPA_FRAME_ARG(8)(%sp)
	stw	%t2, HPPA_FRAME_ARG(9)(%sp)
	stw	%t3, HPPA_FRAME_ARG(10)(%sp)
	stw	%t4, HPPA_FRAME_ARG(11)(%sp)

	/* save temp control regs */
	mfctl	%cr24, %t1
	mfctl	%cr25, %t2
	mfctl	%cr26, %t3
	mfctl	%cr27, %t4
	stw	%t1, HPPA_FRAME_ARG(12)(%sp)	/* XXX can use ,bc */
	stw	%t2, HPPA_FRAME_ARG(13)(%sp)
	stw	%t3, HPPA_FRAME_ARG(14)(%sp)
	stw	%t4, HPPA_FRAME_ARG(15)(%sp)
	mfctl	%cr28, %t1
	mfctl	%cr29, %t2
	mfctl	%cr30, %t3
	mfctl	%cr31, %t4
	stw	%t1, HPPA_FRAME_ARG(16)(%sp)
	stw	%t2, HPPA_FRAME_ARG(17)(%sp)
	stw	%t3, HPPA_FRAME_ARG(18)(%sp)
	stw	%t4, HPPA_FRAME_ARG(19)(%sp)

	comb,=	%r0, %ret0, pdc_call_unmapped2
	nop

	copy	%arg0, %t4
	ldi	PSW_Q, %arg0 /* (!pdc_flag && args[0] == PDC_PIM)? PSW_M:0) */
	break	HPPA_BREAK_KERNEL, HPPA_BREAK_SET_PSW
	nop
	stw	%ret0, HPPA_FRAME_ARG(23)(%sp)
	copy	%t4, %arg0

pdc_call_unmapped2:
	.call
	blr	%r0, %rp
	bv,n	(%r31)
	nop

	/* load temp control regs */
	ldw	HPPA_FRAME_ARG(12)(%sp), %t1
	ldw	HPPA_FRAME_ARG(13)(%sp), %t2
	ldw	HPPA_FRAME_ARG(14)(%sp), %t3
	ldw	HPPA_FRAME_ARG(15)(%sp), %t4
	mtctl	%t1, %cr24
	mtctl	%t2, %cr25
	mtctl	%t3, %cr26
	mtctl	%t4, %cr27
	ldw	HPPA_FRAME_ARG(16)(%sp), %t1
	ldw	HPPA_FRAME_ARG(17)(%sp), %t2
	ldw	HPPA_FRAME_ARG(18)(%sp), %t3
	ldw	HPPA_FRAME_ARG(19)(%sp), %t4
	mtctl	%t1, %cr28
	mtctl	%t2, %cr29
	mtctl	%t3, %cr30
	mtctl	%t4, %cr31

	ldw	HPPA_FRAME_ARG(21)(%sp), %t1
	ldw	HPPA_FRAME_ARG(22)(%sp), %t2
	comb,=	%r0, %t1, pdc_call_unmapped3
	nop

	copy	%ret0, %t3
	ldw	HPPA_FRAME_ARG(23)(%sp), %arg0
	break	HPPA_BREAK_KERNEL, HPPA_BREAK_SET_PSW
	nop
	copy	%t3, %ret0

pdc_call_unmapped3:
	ldw	HPPA_FRAME_PSP(%sp), %sp
	ldw	HPPA_FRAME_CRP(%sp), %rp
	bv	%r0(%rp)
	mtctl	%t2, %eiem
EXIT(pdc_call)

/*
 * int spllower(int ncpl);
 */
ENTRY(spllower,64)
	ldil	L%ipending, %r1
	ldw	R%ipending(%r1), %r1	; load ipending
	ldil	L%cpl, %t1
	andcm,<> %r1, %arg0, %r1	; and with complement of new cpl
	bv	%r0(%rp)
	stw	%arg0, R%cpl(%t1)	; store new cpl

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
	
spllower_dispatch:
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
	
spllower_out:
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
ENTRY(hp700_intr_schedule,0)
	ldil	L%ipending, %t1
	ldil	L%cpl, %t2
	mfctl	%eiem, %arg1
	mtctl	%r0, %eiem			; disable interrupts
	ldw	R%ipending(%t1), %r1		; load ipending
	or	%r1, %arg0, %r1			; or in mask
	stw	%r1, R%ipending(%t1)		; store ipending
	ldw	R%cpl(%t2), %arg0		; load cpl
	andcm,= %r1, %arg0, %r1			; and ipending with ~cpl
	b,n	spllower_dispatch		; dispatch if we can
	bv	%r0(%rp)
	mtctl	%arg1, %eiem
EXIT(hp700_intr_schedule)

/*
 *
 * int hp700_intr_ipending_new(struct hp700_int_reg *int_reg, int int_req);
 *
 * This assembles the mask of new pending interrupts.
 *
 */
ENTRY(hp700_intr_ipending_new,HPPA_FRAME_SIZE)

	/* Start stack calling convention. */
	stw	%rp, HPPA_FRAME_CRP(%sp)
	copy	%r3, %r1
	copy	%sp, %r3
	stw,ma	%r1, HPPA_FRAME_SIZE(%sp)

	/*
	 * Get this interrupt register's interrupt bits map
	 * and start with the least significant bit and with
	 * a zero ipending_new value.
	 */
	ldo	INT_REG_BITS_MAP(%arg0), %arg0
	ldi	31, %arg2
	copy	%r0, %ret0

	/*
	 * The top of this loop finds the next set bit in
	 * the request register.  Note that if the bvb does
	 * not branch, the addib is nullified, and control
	 * falls out of the loop.  If the bvb does branch,
	 * the addib runs with the mtsar in its delay slot.
	 * If the addib branches, the mtsar is nullified.
	 */
$hp700_inew_loop:
	mtsar	%arg2
	bvb,>=,n %arg1, $hp700_inew_loop
	addib,<,n -1, %arg2, $hp700_inew_done

	/*
	 * If the map entry for this bit has INT_REG_BIT_REG
	 * set, branch to descend into the next interrupt
	 * register.  Otherwise, set the bits in our ipending_new
	 * value and loop.
	 */
	ldwx,s  %arg2(%arg0), %t1
	ldil	L%INT_REG_BIT_REG, %t2
	ldo	R%INT_REG_BIT_REG(%t2), %t2
	and	%t1, %t2, %t3
	combt,=,n	%t2, %t3, $hp700_inew_descend
	addib,>= -1, %arg2, $hp700_inew_loop
	or	%t1, %ret0, %ret0

$hp700_inew_done:

	/* End stack calling convention. */
	ldw	HPPA_FRAME_CRP(%r3), %rp
	ldo	HPPA_FRAME_SIZE(%r3), %sp
	bv	%r0(%rp)
	ldw,mb	-HPPA_FRAME_SIZE(%sp), %r3

$hp700_inew_descend:

	/*
	 * If the next interrupt register index is zero,
	 * this interrupt bit is unused.  (Index zero
	 * is the CPU interrupt register, which you can
	 * never descend into since it's the root.)
	 */
	andcm,<> %t1, %t2, %t1
	b,n	$hp700_inew_unused

	/* Save our state. */
	stw	%arg0, HPPA_FRAME_ARG(0)(%r3)
	stw	%arg1, HPPA_FRAME_ARG(1)(%r3)
	stw	%arg2, HPPA_FRAME_ARG(2)(%r3)
	stw	%ret0, HPPA_FRAME_ARG(3)(%r3)

	/* Get our new interrupt register. */
	ldil	L%hp700_int_regs, %arg0
	ldo	R%hp700_int_regs(%arg0), %arg0
	sh2add	%t1, %arg0, %arg0
	ldw	0(%arg0), %arg0

	/*
	 * Read the interrupt request register and make
	 * our recursive call.  The read also serves to
	 * acknowledge the interrupt to the I/O subsystem.
	 */
	ldw	INT_REG_REQ(%arg0), %arg1
	bl	hp700_intr_ipending_new, %rp
	ldw	0(%arg1), %arg1

	/* Restore our state. */
	ldw	HPPA_FRAME_ARG(0)(%r3), %arg0
	ldw	HPPA_FRAME_ARG(1)(%r3), %arg1
	ldw	HPPA_FRAME_ARG(2)(%r3), %arg2
	ldw	HPPA_FRAME_ARG(3)(%r3), %ret1
	or	%ret1, %ret0, %ret0

$hp700_inew_unused:
	addib,>= -1, %arg2, $hp700_inew_loop
	nop
	b,n	$hp700_inew_done
EXIT(hp700_intr_ipending_new)

/*
 * void cpu_die(void);
 */
LEAF_ENTRY_NOPROFILE(cpu_die)
	rsm	RESET_PSW, %r0
	nop
	nop
	mtsp	%r0, %sr0
	ldil	L%LBCAST_ADDR, %r25
	ldi	CMD_RESET, %r26
	stw	%r26, R%iomod_command(%r25)
forever:				; Loop until bus reset takes effect.
	b,n	forever
	nop
	nop
EXIT(cpu_die)

/* Include the system call and trap handling. */
#include <hppa/hppa/trap.S>

/* Include the userspace copyin/copyout functions. */
#include <hppa/hppa/copy.S>

/* Include the support functions. */
#include <hppa/hppa/support.S>

/*
 * struct lwp *
 * cpu_switchto(struct lwp *curl, struct lwp *newl)
 */
	.align	32
ENTRY(cpu_switchto,128)
	/* start stack calling convention */
	stw	%rp, HPPA_FRAME_CRP(%sp)
	copy	%r3, %r1
	copy	%sp, %r3
	stwm	%r1, HPPA_FRAME_SIZE+16*4(%sp)
					/* Frame marker and callee saves */
	stw	%r3, HPPA_FRAME_PSP(%sp)

#ifdef DIAGNOSTIC
	b,n	switch_diag

switch_error:
	copy	%t1, %arg1
	ldil	L%panic, %r1
	ldil	L%Lcspstr, %arg0
	ldo	R%panic(%r1), %r1
	ldo	R%Lcspstr(%arg0), %arg0
	.call
	blr	%r0, %rp
	bv,n	%r0(%r1)
	nop
Lcspstr:
	.asciz	"cpu_switchto: 0x%08x stack/len 0x%08x"
	.align	8

switch_diag:
	/*
	 * Either we must be switching to the same LWP, or
	 * the new LWP's kernel stack must be reasonable.
	 */
	comb,=,n %arg0, %arg1, kstack_ok

	/*
	 * cpu_lwp_fork sets the initial stack to a page above l_addr.
	 * Check that the stack is above this value for curl.
	 */
	ldw	L_ADDR(%arg1), %arg2
	ldw	U_PCB+PCB_KSP(%arg2), %t1	/* t1 for switch_error */
	ldo	NBPG(%arg2), %arg2
	comb,>>,n %arg2, %t1, switch_error
	nop

	/* make sure the stack hasn't grown too big (> USPACE) */
	sub	%t1, %arg2, %t1			/* t1 for switch_error */
	ldil	L%USPACE, %arg2
	ldo	R%USPACE(%arg2), %arg2
	comb,<<=,n %arg2, %t1, switch_error
	nop
kstack_ok:
#endif

	/* If old LWP exited, don't bother. */
	comb,=,n %r0, %arg0, switch_exited

	/*
	 * save old LWP context
	 *
	 * arg0: old LWP (curl)
	 * arg1: new LWP (newl)
	 */

	ldw	L_ADDR(%arg0), %t3	/* curl pcb */
	stw	%sp, U_PCB+PCB_KSP(%t3)
	fdc	%r0(%t3)		/* flush curl pcb  - surely fdc U_PCB+PCB_KSP(%t3) */

	/*
	 * Save the callee-save registers. We don't need to do
	 * r3 here as it was done during stack calling convention.
	 */
	stw	%r4,   1*4(%r3)
	stw	%r5,   2*4(%r3)
	stw	%r6,   3*4(%r3)
	stw	%r7,   4*4(%r3)
	stw	%r8,   5*4(%r3)
	stw	%r9,   6*4(%r3)
	stw	%r10,  7*4(%r3)
	stw	%r11,  8*4(%r3)
	stw	%r12,  9*4(%r3)
	stw	%r13, 10*4(%r3)
	stw	%r14, 11*4(%r3)
	stw	%r15, 12*4(%r3)
	stw	%r16, 13*4(%r3)
	stw	%r17, 14*4(%r3)
	stw	%r18, 15*4(%r3)

	/*
	 * restore new LWP context
	 *
	 * arg0: old LWP (curl)
	 * arg1: new LWP (newl)
	 */
switch_exited:
	ldw	L_MD(%arg1), %t1
	ldw	L_ADDR(%arg1), %t3
	ldw	U_PCB+PCB_KSP(%t3), %sp		/* restore stack of newl */

	fdc	%r0(%t3)			/* Flush newl PCB - why? */

#if 0
	ldw	TF_CR9(%t1), %t3		/* pmap_activate? */
	mtctl	%t3, %pidr2			/* pmap_activate? */
#endif
	ldw	TF_CR30(%t1), %t2		/* pmap_activate? */
	mtctl	%t2, CR_UPADDR			/* pmap_activate? */

	mtctl	%arg1, CR_CURLWP

#ifdef MULTIPROCESSOR
	ldw	L_CPU(%arg1),%t2		; %t2 = curcpu()
	stw	%arg1, CI_CURLWP(%t2)
#endif

	ldo	-(HPPA_FRAME_SIZE+16*4)(%sp), %r3

	ldw	 1*4(%r3), %r4
	ldw	 2*4(%r3), %r5
	ldw	 3*4(%r3), %r6
	ldw	 4*4(%r3), %r7
	ldw	 5*4(%r3), %r8
	ldw	 6*4(%r3), %r9
	ldw	 7*4(%r3), %r10
	ldw	 8*4(%r3), %r11
	ldw	 9*4(%r3), %r12
	ldw	10*4(%r3), %r13
	ldw	11*4(%r3), %r14
	ldw	12*4(%r3), %r15
	ldw	13*4(%r3), %r16
	ldw	14*4(%r3), %r17
	ldw	15*4(%r3), %r18

	/*
	 * Check for restartable atomic sequences (RAS)
	 */
	ldw	L_PROC(%arg1), %t1
	ldw	P_RASLIST(%t1), %t1
	comb,=,n %r0, %t1, noras

	/*
	 * Save some caller-saves we want to preserve.
	 *
	 * We save curl (%arg0) and newl (%arg1) for the benefit of
	 * lwp_trampoline() for when it calls lwp_startup().
	 *
	 * curl (%arg0) is saved as it's the return value
	 */
	stw	%arg0, HPPA_FRAME_ARG(0)(%r3)		/* curl */
	stw	%arg1, HPPA_FRAME_ARG(1)(%r3)		/* newl */

	copy	%arg1, %arg0

	.import	hppa_ras, code
	CALL(hppa_ras, %r1)

	/* restore caller-saves */
	ldw	HPPA_FRAME_ARG(1)(%r3), %arg1
	ldw	HPPA_FRAME_ARG(0)(%r3), %arg0

noras:
	/*
	 * As an optimization, hppa_fpu_bootstrap
	 * replaces this branch instruction with a
	 * nop if there is a hardware FPU.
	 */
ALTENTRY(hppa_fpu_nop1)
	b,n	switch_return

	/*
	 * We do have a hardware FPU.  If the LWP
	 * that we just switched to has its state in the
	 * FPU, enable the FPU, else disable it, so if
	 * the LWP does try to use the coprocessor
	 * we'll get an assist emulation trap to swap
	 * states.
	 */
	ldil	L%fpu_cur_uspace, %t1
	mfctl	%ccr, %r1
	mfctl	CR_UPADDR, %t2
	ldw	R%fpu_cur_uspace(%t1), %t1
	depi	0, 25, 2, %r1		; disables the FPU
	comb,<>,n %t1, %t2, 0		; nullify if LWPs different
	depi	3, 25, 2, %r1		; enables the FPU
	mtctl	%r1, %ccr

switch_return:
	copy	%arg0, %ret0

	ldw	HPPA_FRAME_CRP(%r3), %rp
	bv	0(%rp)
	ldwm	-(HPPA_FRAME_SIZE+16*4)(%sp), %r3
EXIT(cpu_switchto)

/*
 * This is the first code run in a new LWP after
 * cpu_switchto() has switched to it for the first time.
 *
 * This happens courtesy of the setup in cpu_lwp_fork() which
 * arranges for cpu_switchto() to call us with a frame containing
 * the first kernel function to call, and its argument.
 *
 * cpu_switchto() also makes sure that %arg0 and %arg1 are (still)
 * curl and newl respectively.
 */
ENTRY_NOPROFILE(lwp_trampoline,HPPA_FRAME_SIZE)
	/* no return point */
	stw	%r0, HPPA_FRAME_CRP(%sp)

	/* %arg0, %arg1 are still valid from cpu_switchto */
	.import	lwp_startup, code
	CALL(lwp_startup, %r1)

	/* get trampoline func (%t3) and arg (%arg0) */
	ldw	HPPA_FRAME_ARG(3)(%sp), %arg0
	ldw	HPPA_FRAME_ARG(2)(%sp), %t3

	/* call the first kernel function */
	.call
	blr	%r0, %rp
	bv,n	%r0(%t3)
	nop

	/*
	 * Since the first kernel function returned,
	 * this LWP was created by the fork()
	 * syscall, which we now return from.
	 */
	mfctl	CR_CURLWP, %t2
	.call
	b	$syscall_return
	ldw	L_MD(%t2), %t3
EXIT(lwp_trampoline)

/* Include the signal code. */
#include <hppa/hppa/sigcode.S>

	.end
