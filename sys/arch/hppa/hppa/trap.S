/*	$NetBSD: trap.S,v 1.35 2009/12/15 22:44:25 skrll Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matthew Fredette.
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
/*
 * Copyright (c) 1990,1991,1992,1994 The University of Utah and
 * the Computer Systems Laboratory (CSL).  All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software is hereby
 * granted provided that (1) source code retains these copyright, permission,
 * and disclaimer notices, and (2) redistributions including binaries
 * reproduce the notices in supporting documentation, and (3) all advertising
 * materials mentioning features or use of this software display the following
 * acknowledgement: ``This product includes software developed by the
 * Computer Systems Laboratory at the University of Utah.''
 *
 * THE UNIVERSITY OF UTAH AND CSL ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS
 * IS" CONDITION.  THE UNIVERSITY OF UTAH AND CSL DISCLAIM ANY LIABILITY OF
 * ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * CSL requests users of this software to return to csl-dist@cs.utah.edu any
 * improvements that they make and grant CSL redistribution rights.
 *
 *	Utah $Hdr: locore.s 1.63 95/01/20$
 */

#include "opt_compat_osf1.h"
#include "opt_cputype.h"

/* 
 * NOTICE: This is not a standalone file.  To use it, #include it in
 * your port's locore.S, like so:
 *
 *	#include <hppa/hppa/trap.S>
 */

	.section .data
	.align	64
L$trap_tmp_save:
	.block	TF_PHYS
	.size	L$trap_tmp_save, .-L$trap_tmp_save

	.align	64
	.export emergency_stack_start, data
emergency_stack_start:
	.block	32768		/* XXX must be aligned to 64 */
	.export emergency_stack_end, data
emergency_stack_end:
	.block	4

	.text

/*
 * Kernel Gateway Page (must be at known address)
 *	System Call Gate
 *
 * GATEway instructions have to be at a fixed known locations
 * because their addresses are hard coded in routines such as
 * those in the C library.
 */

	.align	NBPG
	.export	gateway_page, entry
gateway_page:
	nop				/* @ 0.C0000000 (Nothing)  */
	gate,n	bsd_syscall,%r0	/* @ 0.C0000004 (HPUX/BSD) */
#ifdef COMPAT_OSF1
	bl,n	osf_syscall,%r0
	bl,n	osf_syscall,%r0
#else
	nop				/* @ 0.C0000008 (HPOSF UNIX) */
	nop				/* @ 0.C000000C (HPOSF Mach) */
#endif
	nop
	nop
	nop
	nop

#ifdef COMPAT_OSF1
osf_syscall:
	/*
	 * Ripped screaming from OSF/MkLinux:
	 *
	 * Convert HPOSF system call to a BSD one by stashing arg4 and arg5
	 * back into the frame, and moving the system call number into r22.
	 * Fortunately, the HPOSF compiler has a bigger stack frame, which
	 * allows this horrible hack.
	 *
	 * We also need to save r29 (aka ret1) for the emulator since it may
	 * get clobbered between here and there.
	 */
	stw	%r22, HPPA_FRAME_ARG(4)(%sp)
	stw	%r21, HPPA_FRAME_ARG(5)(%sp)
	stw	%r29, HPPA_FRAME_SL(%sp)
	gate	bsd_syscall,%r0
	copy	%r1, %r22
#endif /* COMPAT_OSF1 */

bsd_syscall:
	/*
	 * set up a space register and a protection id so that
	 * we can access kernel memory
	 */
	mfctl	%eiem, %r1
	mtctl	%r0, %eiem
	mtsp	%r0, %sr1
	mfctl	%pidr1, %ret0
	ldi	HPPA_PID_KERNEL, %t2
	mtctl	%t2, %pidr1

	/*
	 * now call the syscall handler
	 */
	.import syscall_entry,code
	.call
	ldil	L%syscall_entry, %t2
	be	R%syscall_entry(%sr1, %t2)
	nop ! nop ! nop ! nop
	.size	gateway_page, .-gateway_page

	.align	NBPG
	.export	gateway_page_end, entry
gateway_page_end:

	.export syscall_entry,entry
	.proc
	.callinfo calls
	.entry
syscall_entry:
	/*
	 * %r1:		eiem
	 * %ret0:	process protection id
	 * %t1:		syscall number
	 * %sr0, %r31:	return address
	 * %sp:		user stack
	 *
	 * N.B. we are trying to rely on the fact that bottom of kernel
	 *	stack contains a print of some past trapframe, so
	 *	we do not save hard to get information, but do restore
	 *	the whole context later on return anyway.
	 * XXXXXX this is very bad. everything must be saved
	 *
	 * WHAT ARE WE RELYING ON?
	 *
	 */

	/* t2 = curlwp PCB */
	mfctl	CR_CURLWP, %t3 
	ldw	L_PCB(%sr1, %t3), %t2			/* XXX can use ,sl */

	/*
	 * NB: Even though t4 is a caller-saved register, we
	 * save it anyways, as a convenience to __vfork14 and
	 * any other syscalls that absolutely must have a 
	 * register that is saved for it.
	 */
	/* calculate kernel sp, load, create kernel stack frame */
	ldo	NBPG+TRAPFRAME_SIZEOF(%t2), %t3	/* see cpu_lwp_fork */

	stw	%t4, TF_R19 -TRAPFRAME_SIZEOF(%sr1, %t3)    /* t4 for vfork */
	stw	%t1, TF_R22 -TRAPFRAME_SIZEOF(%sr1, %t3)    /* syscall # */
	copy	%sp, %t4

	/* gotta save the args, in case we gonna restart */
	stw	%arg3, TF_R23 -TRAPFRAME_SIZEOF(%sr1, %t3)
	stw	%arg2, TF_R24 -TRAPFRAME_SIZEOF(%sr1, %t3)
	stw	%arg1, TF_R25 -TRAPFRAME_SIZEOF(%sr1, %t3)
	stw	%arg0, TF_R26 -TRAPFRAME_SIZEOF(%sr1, %t3)
	stw	%r27, TF_R27 -TRAPFRAME_SIZEOF(%sr1, %t3)	/* dp */
	stw	%sp, TF_R30 -TRAPFRAME_SIZEOF(%sr1, %t3)	/* user stack */
	copy	%t3, %sp
	/*
	 * Make space for the syscall arguments.
	 *
	 * Match the offset from %sp to the trapframe with the
	 * offset in TLABEL(all) for the benefit of ddb.
	 */
	stwm	%r0, HPPA_FRAME_SIZE+HPPA_FRAME_MAXARGS(%sr1, %sp)

	/* Align correctly */
	ldo	HPPA_FRAME_SIZE-1(%sp),%sp
	depi	0, 31, 6, %sp

	stw	%r0, HPPA_FRAME_CRP(%sr1, %sp)

	ldil	L%kpsw, %t1
	ldw	R%kpsw(%sr1, %t1), %t1
	stw	%r1, TF_CR15-TRAPFRAME_SIZEOF(%sr1, %t3)	/* eiem ,bc */
	stw	%t1, TF_CR22-TRAPFRAME_SIZEOF(%sr1, %t3)	/* ipsw */
 
	/* XXX moved here to be protected from interrupts - why? */
	mfsp	%sr3, %t1
	stw	%t1, TF_SR3-TRAPFRAME_SIZEOF(%sr1, %t3)
	stw	%ret0, TF_CR8-TRAPFRAME_SIZEOF(%sr1, %t3)	/* pidr1 */

	/* now we can allow interrupts to happen */
	mtctl	%r1, %eiem

	/*
	 * Normally, we only have to save the caller-saved registers,
	 * because the callee-saved registers will be naturally
	 * saved and restored by our callee(s).  However, see the
	 * longer comment in the trap handling code below for the
	 * reasons why we need to save and restore all of them.
	 */
	stw	%r2 , TF_R2 -TRAPFRAME_SIZEOF(%sr1, %t3)
	stw	%r3 , TF_R3 -TRAPFRAME_SIZEOF(%sr1, %t3)

	/* We can now set the frame pointer */
	copy	%t3, %r3

#if defined(DDB) || defined(KGDB) || defined(FPEMUL)
	stw	%r4 , TF_R4 -TRAPFRAME_SIZEOF(%sr1, %t3)
	stw	%r5 , TF_R5 -TRAPFRAME_SIZEOF(%sr1, %t3)
	stw	%r6 , TF_R6 -TRAPFRAME_SIZEOF(%sr1, %t3)
	stw	%r7 , TF_R7 -TRAPFRAME_SIZEOF(%sr1, %t3)
	stw	%r8 , TF_R8 -TRAPFRAME_SIZEOF(%sr1, %t3)
	stw	%r9 , TF_R9 -TRAPFRAME_SIZEOF(%sr1, %t3)
	stw	%r10, TF_R10-TRAPFRAME_SIZEOF(%sr1, %t3)
	stw	%r11, TF_R11-TRAPFRAME_SIZEOF(%sr1, %t3)
	stw	%r12, TF_R12-TRAPFRAME_SIZEOF(%sr1, %t3)
	stw	%r13, TF_R13-TRAPFRAME_SIZEOF(%sr1, %t3)
	stw	%r14, TF_R14-TRAPFRAME_SIZEOF(%sr1, %t3)
	stw	%r15, TF_R15-TRAPFRAME_SIZEOF(%sr1, %t3)
	stw	%r16, TF_R16-TRAPFRAME_SIZEOF(%sr1, %t3)
	stw	%r17, TF_R17-TRAPFRAME_SIZEOF(%sr1, %t3)
	stw	%r18, TF_R18-TRAPFRAME_SIZEOF(%sr1, %t3)
#endif /* DDB || KGDB || FPEMUL */
	stw	%r0, 0(%sr1, %t3)	/* terminate frame */
	copy	%r0 , %r3
	stw	%r0, HPPA_FRAME_PSP(%sr1, %sp)
	stw	%r0, HPPA_FRAME_CRP(%sr1, %sp)

	/*
	 * Is this copying required... OpenBSD doesn't do it!
	 *
	 * Copy Arguments
	 * unfortunately mmap() under bsd requires 7 words;
	 * linux is confined to 5, and hpux to 6.
	 * assuming the `long' syscall it gives us the maximum
	 * 9 words, which very much overkill for an average of 3.
	 * we keep it at 10, since bundling will keep it
	 * at the same speed as 9 anyway.
	 */
	/*
	 * XXX fredette - possible security hole here.
	 * What happens if the user hands us a stack
	 * that points to nowhere, or to data that they
	 * should not be reading?
	 */
	stw	%arg0, 1*4(%sr1, %t3)	/* XXX can use ,bc */
	stw	%arg1, 2*4(%sr1, %t3)
	stw	%arg2, 3*4(%sr1, %t3)
	stw	%arg3, 4*4(%sr1, %t3)
	ldw	HPPA_FRAME_ARG( 4)(%t4), %arg0
	ldw	HPPA_FRAME_ARG( 5)(%t4), %arg1
	ldw	HPPA_FRAME_ARG( 6)(%t4), %arg2
	ldw	HPPA_FRAME_ARG( 7)(%t4), %arg3
	stw	%arg0, 5*4(%sr1, %t3)
	stw	%arg1, 6*4(%sr1, %t3)
	stw	%arg2, 7*4(%sr1, %t3)
	stw	%arg3, 8*4(%sr1, %t3)
	ldw	HPPA_FRAME_ARG( 8)(%t4), %arg0
	ldw	HPPA_FRAME_ARG( 9)(%t4), %arg1
	stw	%arg0, 9*4(%sr1, %t3)
	stw	%arg1,10*4(%sr1, %t3)

	/*
	 * Save the rest of the CPU context
	 */

	/* XXXNH: Should do a be 0(%sr1, %r31) instead of rfi when possible */
	ldo	4(%r31), %arg1
	stw	%r31, TF_IIOQH-TRAPFRAME_SIZEOF(%sr1, %t3)
	stw	%arg1, TF_IIOQT-TRAPFRAME_SIZEOF(%sr1, %t3)

	mfsp	%sr0, %arg0
	stw	%arg0, TF_IISQH-TRAPFRAME_SIZEOF(%sr1, %t3)
	stw	%arg0, TF_IISQT-TRAPFRAME_SIZEOF(%sr1, %t3)

	stw	%arg0, TF_CR20-TRAPFRAME_SIZEOF(%sr1, %t3)
	stw	%r31, TF_CR21-TRAPFRAME_SIZEOF(%sr1, %t3)

	ldil	L%(TFF_LAST|TFF_SYS), %arg1
	stw	%r0, TF_CR19-TRAPFRAME_SIZEOF(%sr1, %t3)	/* iir */
	stw	%arg1, TF_FLAGS-TRAPFRAME_SIZEOF(%sr1, %t3)

	/* Already done above: mfsp	%sr0, %arg0 */
	mfsp	%sr2, %arg2
	mfsp	%sr4, %arg3
	stw	%arg0, TF_SR0-TRAPFRAME_SIZEOF(%sr1, %t3)
	stw	%arg0, TF_SR1-TRAPFRAME_SIZEOF(%sr1, %t3)
					/* we overwrote sr1 earlier */
	stw	%arg2, TF_SR2-TRAPFRAME_SIZEOF(%sr1, %t3)
	stw	%arg3, TF_SR4-TRAPFRAME_SIZEOF(%sr1, %t3)

	mfsp	%sr5, %arg0
	mfsp	%sr6, %arg1
	mfsp	%sr7, %arg2
	mfctl	%pidr2, %arg3
	stw	%arg0, TF_SR5-TRAPFRAME_SIZEOF(%sr1, %t3)
	stw	%arg1, TF_SR6-TRAPFRAME_SIZEOF(%sr1, %t3)
	stw	%arg2, TF_SR7-TRAPFRAME_SIZEOF(%sr1, %t3)
	stw	%arg3, TF_CR9-TRAPFRAME_SIZEOF(%sr1, %t3)

#if pbably_not_worth_it
	mfctl	%pidr3, %arg2
	mfctl	%pidr4, %arg3
	stw	%arg2, TF_CR12-TRAPFRAME_SIZEOF(%sr1, %t3)
	stw	%arg3, TF_CR13-TRAPFRAME_SIZEOF(%sr1, %t3)
#endif

#if defined(DDB) || defined(KGDB)
	/*
	 * Save v2p translation table pointer
	 */
	mfctl	%eirr, %arg0
	mfctl	CR_VTOP, %arg1
	stw	%arg0, TF_CR23-TRAPFRAME_SIZEOF(%sr1, %t3)
	stw	%arg1, TF_CR25-TRAPFRAME_SIZEOF(%sr1, %t3)

	mfctl	%cr28, %arg1
	stw	%arg1, TF_CR28-TRAPFRAME_SIZEOF(%sr1, %t3)
#endif

	/* setup kernel context */
	mtsp	%r0, %sr0
	mtsp	%r0, %sr1
	mtsp	%r0, %sr2
	mtsp	%r0, %sr3
	mtsp	%r0, %sr4
	mtsp	%r0, %sr5
	mtsp	%r0, %sr6
	mtsp	%r0, %sr7

	ldo	-TRAPFRAME_SIZEOF(%t3), %arg0
	ldo	4(%t3), %arg1

	ldil	L%$global$,%dp
	ldo	R%$global$(%dp),%dp

	/* do a syscall */
	.import	syscall,code
	CALL(syscall, %r1)

	/* load curlwp's trapframe pointer */
	mfctl	CR_CURLWP, %r1
	ldw	L_MD(%r1), %t3

	.exit
	.procend
	/* FALLTHROUGH */

	.export	syscall_return, entry
	.proc
	.callinfo no_calls
	.entry
syscall_return:
	/* t3 == VA trapframe */
	/* check for AST ? XXX */

	/* splhigh(), just in case */
	mtctl	%r0, %eiem

	/*
	 * 1a. Copy a `phys' part of the frame into temp store
	 *	(see a note for trapall)
	 *	hopefully no page fault would happen on or after the copy,
	 *	and interrupts are disabled.
	 */
	ldil	L%L$trap_tmp_save, %t2
	ldo	R%L$trap_tmp_save(%t2), %t2

	ldw  0(%t3), %r1 ! ldw  4(%t3), %t1 ! stw %r1,  0(%t2) ! stw %t1,  4(%t2)
	ldw  8(%t3), %r1 ! ldw 12(%t3), %t1 ! stw %r1,  8(%t2) ! stw %t1, 12(%t2)
	ldw 16(%t3), %r1 ! ldw 20(%t3), %t1 ! stw %r1, 16(%t2) ! stw %t1, 20(%t2)
	ldw 24(%t3), %r1 ! ldw 28(%t3), %t1 ! stw %r1, 24(%t2) ! stw %t1, 28(%t2)
	ldw 32(%t3), %r1 ! ldw 36(%t3), %t1 ! stw %r1, 32(%t2) ! stw %t1, 36(%t2)
	ldw 40(%t3), %r1 ! ldw 44(%t3), %t1 ! stw %r1, 40(%t2) ! stw %t1, 44(%t2)
	ldw 48(%t3), %r1 ! ldw 52(%t3), %t1 ! stw %r1, 48(%t2) ! stw %t1, 52(%t2)
	ldw 56(%t3), %r1 ! ldw 60(%t3), %t1 ! stw %r1, 56(%t2) ! stw %t1, 60(%t2)

	/* 1b. restore most of the general registers */
	ldw	TF_CR11(%t3), %t1
	mtctl	%t1, %sar
	ldw	TF_R1(%t3), %r1
	ldw	TF_R2(%t3), %r2
	ldw	TF_R3(%t3), %r3
	/*
	 * See the comment in the trap handling code below
	 * about why we need to save and restore all general
	 * registers under these cases.
	 */
#if defined(DDB) || defined(KGDB) || defined(FPEMUL)
	ldw	TF_R4(%t3), %r4
	ldw	TF_R5(%t3), %r5
	ldw	TF_R6(%t3), %r6
	ldw	TF_R7(%t3), %r7
	ldw	TF_R8(%t3), %r8
	ldw	TF_R9(%t3), %r9
	ldw	TF_R10(%t3), %r10
	ldw	TF_R11(%t3), %r11
	ldw	TF_R12(%t3), %r12
	ldw	TF_R13(%t3), %r13
	ldw	TF_R14(%t3), %r14
	ldw	TF_R15(%t3), %r15
	ldw	TF_R16(%t3), %r16
	ldw	TF_R17(%t3), %r17
	ldw	TF_R18(%t3), %r18
#endif /* DDB || KGDB || FPEMUL */
	ldw	TF_R19(%t3), %t4
	/*	%r20(%t3) is used as a temporary and will be restored later */
	/*	%r21(%t2) is used as a temporary and will be restored later */
	/*	%r22(%t1) is used as a temporary and will be restored later */
	ldw	TF_R23(%t3), %r23
	ldw	TF_R24(%t3), %r24
	ldw	TF_R25(%t3), %r25
	ldw	TF_R26(%t3), %r26
	ldw	TF_R27(%t3), %r27
	ldw	TF_R28(%t3), %r28
	ldw	TF_R29(%t3), %r29
	/*	%r30 (%sp) will be restored later */
	ldw	TF_R31(%t3), %r31

	/* 2. restore all the space regs and pid regs, except sr3, pidr1 */
	ldw	TF_SR0(%t3), %t1
	ldw	TF_SR1(%t3), %t2
	mtsp	%t1, %sr0
	mtsp	%t2, %sr1

	ldw	TF_SR2(%sr3, %t3), %t1
	ldw	TF_SR4(%sr3, %t3), %t2
	mtsp	%t1, %sr2
	mtsp	%t2, %sr4

	ldw	TF_SR5(%sr3, %t3), %t1
	ldw	TF_SR6(%sr3, %t3), %t2
	mtsp	%t1, %sr5
	mtsp	%t2, %sr6

	ldw	TF_SR7(%sr3, %t3), %t1
	ldw	TF_CR9(%sr3, %t3), %t2
	mtsp	%t1, %sr7
	mtctl	%t2, %pidr2

#if pbably_not_worth_it
	ldw	TF_CR12(%sr3, %t3), %t1
	ldw	TF_CR13(%sr3, %t3), %t2
	mtctl	%t1, %pidr3
	mtctl	%t2, %pidr4
#endif
	ldw	TF_CR0(%sr3, %t3), %t1
	mtctl	%t1, %rctr
	ldw	TF_CR30(%sr3, %t3), %t1
	mtctl	%t1, CR_UPADDR

	/*
	 * clear the system mask, this puts us back into physical mode.
	 * reload trapframe pointer w/ correspondent PA value.
	 * sp will be left in virtual until restored from trapframe,
	 * since we don't use it anyway.
	 */
	ssm	0, %r0
	ldil	L%L$trap_tmp_save, %t3
	ldo	R%L$trap_tmp_save(%t3), %t3
	nop ! nop ! nop ! nop ! nop
	rsm	RESET_PSW, %r0

	/* finally we can restore the space and offset queues and the ipsw */
	ldw	TF_IISQH(%t3), %t1
	ldw	TF_IISQT(%t3), %t2
	mtctl	%t1, %pcsq
	mtctl	%t2, %pcsq

	ldw	TF_IIOQH(%t3), %t1
	ldw	TF_IIOQT(%t3), %t2
	mtctl	%t1, %pcoq
	mtctl	%t2, %pcoq

	ldw	TF_CR15(%t3), %t1
	ldw	TF_CR22(%t3), %t2
	mtctl	%t1, %eiem
	mtctl	%t2, %ipsw

	ldw	TF_SR3(%t3), %t1
	ldw	TF_CR8(%t3), %t2
	mtsp	%t1, %sr3
	mtctl	%t2, %pidr1

	ldw	TF_R22(%t3), %t1
	ldw	TF_R21(%t3), %t2
	ldw	TF_R30(%t3), %sp
	ldw	TF_R20(%t3), %t3

	rfi
	nop
	.exit
	.procend
	.size	syscall_entry, .- syscall_entry

/*
 * interrupt vector table
 */
#define	TLABEL(name)		__CONCAT(trap_,name)
#define TRAPLABEL(name,num)	__CONCAT(TLABEL(name),num)
#define	TELABEL(num)		__CONCAT(trap_ep_,num)

#define TRAP(name,num) \
	mtctl	%r1, %tr7		! \
	.call				! \
	.import TLABEL(name), code	! \
	b	TLABEL(name)		! \
	ldi	num, %r1		! \
	.align	32

#define	ATRAP(name,num) \
	.export	TRAPLABEL(name,num), entry	! \
	.label	TRAPLABEL(name,num)		! \
	TRAP(all,num)				! \
	.size	TRAPLABEL(name,num), .-TRAPLABEL(name,num)

#define	CTRAP(name,num,pre) \
	.export	TRAPLABEL(name,num), entry	! \
	.label	TRAPLABEL(name,num)		! \
	pre				! \
	TRAP(name,num)			! \
	.size	TRAPLABEL(name,num), .-TRAPLABEL(name,num)

#define	STRAP(name,num,pre) \
	.export	TRAPLABEL(name,num), entry	! \
	.label	TRAPLABEL(name,num)		! \
	pre				! \
	mtctl	%r1, %tr7		! \
	.export	TELABEL(num), entry	! \
	.label	TELABEL(num)		! \
	ldil	0,%r1			! \
	ldo	0(%r1), %r1		! \
	.call				! \
	bv	0(%r1)			! \
	ldi	num, %r1		! \
	.align	32			! \
	.size	TRAPLABEL(name,num), .-TRAPLABEL(name,num)

#define	LDILDO(name)			! \
	.export	name, entry		! \
	.label	name			! \
	ldil	L%TLABEL(name),%r1	! \
	ldo	R%TLABEL(name)(%r1), %r1

#ifdef HP7000_CPU
LDILDO(itlb_x)
LDILDO(itlbna_x)
LDILDO(dtlb_x)
LDILDO(dtlbna_x)
LDILDO(tlbd_x)

LDILDO(itlb_s)
LDILDO(itlbna_s)
LDILDO(dtlb_s)
LDILDO(dtlbna_s)
LDILDO(tlbd_s)
#endif

#if defined(HP7100_CPU) || defined(HP7200_CPU)
LDILDO(itlb_t)
LDILDO(itlbna_t)
LDILDO(dtlb_t)
LDILDO(dtlbna_t)
LDILDO(tlbd_t)
#endif

#if defined(HP7100LC_CPU) || defined(HP7300LC_CPU)
LDILDO(itlb_l)
LDILDO(itlbna_l)
LDILDO(dtlb_l)
LDILDO(dtlbna_l)
LDILDO(tlbd_l)
#endif

#if defined(HP8000_CPU) || defined(HP8200_CPU) || \
    defined(HP8500_CPU) || defined(HP8600_CPU)
LDILDO(itlb_u)
LDILDO(itlbna_u)
LDILDO(dtlb_u)
LDILDO(dtlbna_u)
LDILDO(tlbd_u)
#endif

#define	ITLBPRE \
	mfctl	%pcoq,%r9		/* Offset */			! \
	mfctl	%pcsq,%r8		/* Space  */

#define	DTLBPRE \
	mfctl	%ior, %r9		/* Offset */			! \
	mfctl	%isr, %r8		/* Space  */
	/* CR28XXX according to a popular belief cr28 should be read here */

#define	HPMCPRE	\
	nop

	.align NBPG
	.export ivaaddr, entry
	.export os_hpmc, entry
ivaaddr:
	ATRAP(null,T_NONEXIST)		/*  0. invalid interrupt vector */
os_hpmc:
	CTRAP(hpmc,T_HPMC,HPMCPRE)	/*  1. high priority machine check */
	ATRAP(power,T_POWERFAIL)	/*  2. power failure */
	ATRAP(recnt,T_RECOVERY)		/*  3. recovery counter trap */
	ATRAP(intr,T_INTERRUPT)		/*  4. external interrupt */
	ATRAP(lpmc,T_LPMC)		/*  5. low-priority machine check */
	STRAP(itlb,T_ITLBMISS,ITLBPRE)	/*  6. instruction TLB miss fault */
	ATRAP(iprot,T_IPROT)		/*  7. instruction protection trap */
	ATRAP(ill,T_ILLEGAL)		/*  8. Illegal instruction trap */
	CTRAP(ibrk,T_IBREAK,)		/*  9. break instruction trap */
	ATRAP(privop,T_PRIV_OP)		/* 10. privileged operation trap */
	ATRAP(privr,T_PRIV_REG)		/* 11. privileged register trap */
	ATRAP(ovrfl,T_OVERFLOW)		/* 12. overflow trap */
	ATRAP(cond,T_CONDITION)		/* 13. conditional trap */
	ATRAP(excpt,T_EXCEPTION)	/* 14. assist exception trap */
	STRAP(dtlb,T_DTLBMISS,DTLBPRE)	/* 15. data TLB miss fault */
	STRAP(itlbna,T_ITLBMISSNA,DTLBPRE)/* 16. ITLB non-access miss fault */
	STRAP(dtlbna,T_DTLBMISSNA,DTLBPRE)/* 17. DTLB non-access miss fault */
	ATRAP(dprot,T_DPROT)		/* 18. data protection trap
					       unaligned data reference trap */
	ATRAP(dbrk,T_DBREAK)		/* 19. data break trap */
	STRAP(tlbd,T_TLB_DIRTY,DTLBPRE)	/* 20. TLB dirty bit trap */
	ATRAP(pgref,T_PAGEREF)		/* 21. page reference trap */
	CTRAP(emu,T_EMULATION,)		/* 22. assist emulation trap */
	ATRAP(hpl,T_HIGHERPL)		/* 23. higher-privelege transfer trap*/
	ATRAP(lpl,T_LOWERPL)		/* 24. lower-privilege transfer trap */
	ATRAP(tknbr,T_TAKENBR)		/* 25. taken branch trap */
	ATRAP(dacc,T_DATACC)		/* 26. data access rights trap */
	ATRAP(dpid,T_DATAPID)		/* 27. data protection ID trap */
	ATRAP(dalgn,T_DATALIGN)		/* 28. unaligned data ref trap */
	ATRAP(unk,29)
	ATRAP(unk,30)
	ATRAP(unk,31)
	ATRAP(unk,32)
	ATRAP(unk,33)
	ATRAP(unk,34)
	ATRAP(unk,35)
	ATRAP(unk,36)
	ATRAP(unk,37)
	ATRAP(unk,38)
	ATRAP(unk,39)
	ATRAP(unk,40)
	ATRAP(unk,41)
	ATRAP(unk,42)
	ATRAP(unk,43)
	ATRAP(unk,44)
	ATRAP(unk,45)
	ATRAP(unk,46)
	ATRAP(unk,47)
	ATRAP(unk,48)
	ATRAP(unk,49)
	ATRAP(unk,50)
	ATRAP(unk,51)
	ATRAP(unk,52)
	ATRAP(unk,53)
	ATRAP(unk,54)
	ATRAP(unk,55)
	ATRAP(unk,56)
	ATRAP(unk,57)
	ATRAP(unk,58)
	ATRAP(unk,59)
	ATRAP(unk,60)
	ATRAP(unk,61)
	ATRAP(unk,62)
	ATRAP(unk,63)
					/* 64 */
	.size	ivaaddr, .-ivaaddr

/*
 * This is the locore support for HPMC and TOC machine checks.
 * In the HPMC case, this is a continuation of the HPMC handler
 * that begins in the interrupt vector table.  In the TOC
 * case, this is the handler installed in page zero.
 *
 * Notable points about the CPU state for the OS_TOC handler:
 *
 * - The PSW Q bit is 1, all other PSW bits are 0.
 * - CR14 (IVA) does not point to our vector table.
 * - CR22 (IPSW) is valid.
 * - All other control registers HVERSION dependent.
 * - The TLB is initialized and invalid.
 * 
 * Notable points about the CPU state for the OS_HPMC handler:
 *
 * - The PSW M bit is 1, all other PSW bits are 0.
 * - CR14 (IVA) does point to our vector table.
 * - CR22 (IPSW) is valid.
 * - All other control registers HVERSION dependent.
 * - The TLB is unchanged.
 *
 * The TOC CPU state is actually trickier.  Whereas in the HPMC 
 * case, we can return to virtual mode right away, in the TOC 
 * case we can't return to virtual mode until the kernel mapping 
 * is reloaded into the BTLB.
 * 
 * Otherwise, we set up the kernel context, move onto the 
 * emergency stack, and call hppa_machine_check.
 */
ENTRY_NOPROFILE(os_toc, 0)
	/* This loads %arg0 and nullifies the next instruction. */
	addi,tr	T_INTERRUPT, %r0, %arg0
EXIT(os_toc)
ENTRY_NOPROFILE(TLABEL(hpmc),0)
ALTENTRY(os_hpmc_cont)
	ldi	T_HPMC, %arg0
	
	/* Disable interrupts. */
	mtctl	%r0, %eiem

	/* Load protection and space registers for the kernel. */
	ldi	HPPA_PID_KERNEL, %r1
	mtctl	%r1, %pidr1
	ldi	HPPA_SID_KERNEL, %r1
	mtsp	%r1, %sr0
	mtsp	%r1, %sr1
	mtsp	%r1, %sr2
	mtsp	%r1, %sr3
	mtsp	%r1, %sr4
	mtsp	%r1, %sr5
	mtsp	%r1, %sr6
	mtsp	%r1, %sr7

	/* Reload the Interruption Vector Address. */
	ldil	L%ivaaddr, %r1
	ldo	R%ivaaddr(%r1), %r1
	mtctl	%r1, %iva

	/* Reload the HPT base and mask. */
	ldil	L%hppa_vtop, %r1
	ldw	R%hppa_vtop(%r1), %r1
	mtctl	%r1, CR_VTOP

	/* Disable interrupts for the long haul. */
	ldil	L%kpsw, %t1
	ldw	R%kpsw(%t1), %r1
	depi	0, PSW_I_POS, 1, %r1
	stw	%r1, R%kpsw(%t1)

	/* Reload the global data pointer. */
	ldil	L%$global$, %dp
	ldo	R%$global$(%dp), %dp

	/* Move onto the emergency stack. */
	ldil	L%emergency_stack_start, %sp
	ldo	R%emergency_stack_start(%sp), %sp
	stwm	%r0, HPPA_FRAME_SIZE(%sp)
	copy	%sp, %r3

	/* Start stack calling convention. */
	stw	%r0, HPPA_FRAME_CRP(%sp)
	stw	%r0, HPPA_FRAME_PSP(%sp)
	copy	%r3, %r1
	copy	%sp, %r3
	stwm	%r1, HPPA_FRAME_SIZE(%sp)

	/* If this is a TOC, remap the kernel. */
	comib,<>,n T_INTERRUPT, %arg0, L$check_do_rfi

	/* Clear kernelmapped. */
	ldil	L%kernelmapped, %r1
	stw	%r0, R%kernelmapped(%r1)

	/* Call hppa_btlb_reload. */
	ldil	L%hppa_btlb_reload, %r1
	ldo	R%hppa_btlb_reload(%r1), %r1
	blr	0, %rp
	bv	%r0(%r1)
	nop

	/* Set kernelmapped. */
	ldil	L%kernelmapped, %r1
	stw	%r1, R%kernelmapped(%r1)

	/* Reload %arg0 (it may have been destroyed). */
	ldi	T_INTERRUPT, %arg0

	/* Disable the interrupt queues. */
	rsm	RESET_PSW, %r0

L$check_do_rfi:

	/* Load IPSW. */
	ldil	L%kpsw, %r1
	ldw	R%kpsw(%r1), %r1
	mtctl	%r1, %ipsw

	/* Get the address of hppa_machine_check. */
	ldil	L%hppa_machine_check, %r1
	ldo	R%hppa_machine_check(%r1), %r1

	/* Load the instruction address queues. */
	mtctl	%r1, %pcoq
	ldo	4(%r1), %r1
	mtctl	%r1, %pcoq
	ldi	HPPA_SID_KERNEL, %r1
	mtctl	%r1, %pcsq
	mtctl	%r1, %pcsq

	blr	0, %rp
	rfi
	nop
	nop
	nop
	nop
	nop
	nop
ALTENTRY(os_hpmc_cont_end)
	nop
ALTENTRY(os_toc_end)
EXIT(TLABEL(hpmc))

/*
 * This handles all assist emulation traps.  We break
 * these down into three categories: emulate special
 * function unit, emulate non-FPU coprocessor, and
 * emulate FPU coprocessor, and dispatch accordingly.
 */
	.export TLABEL(emu), entry
LEAF_ENTRY_NOPROFILE(TLABEL(emu))

	/*
	 * Save %arg0 and load it with the instruction
	 * that caused the emulation trap.
	 */
	mtctl	%arg0, %tr2
	mfctl	%iir, %arg0
	
	/*
	 * If the opcode field in the instruction is 4,
	 * indicating a special function unit SPOP
	 * instruction, branch to emulate an sfu.
	 * If the opcode field is 0xe, then it's an FPU instruction.
	 */
	extru	%arg0, 5, 6, %r1
	comib,=,n 4, %r1, L$emulate_sfu
	comib,=,n 0xe, %r1, hppa_fpu_nop0

	/*
	 * If the uid field in the instruction is not
	 * zero or one, indicating a coprocessor other
	 * than an FPU, branch to emulate a non-FPU
	 * coprocessor.
	 */
	extru	%arg0, 25, 3, %r1
	comib,<<,n 1, %r1, L$emulate_coproc

	/*
	 * If we're still here, this is a FPU 
	 * coprocessor instruction.  That we trapped
	 * to emulate it means one of three things.
	 *
	 * If we do not have a hardware FPU, we need
	 * to emulate this instruction.
	 *
	 * If we do have a hardware FPU but it is
	 * disabled, we trapped because the current
	 * process' state is not loaded into the
	 * FPU.  We load that state in, possibly
	 * swapping out another process' state first.
	 *
	 * If we do have a hardware FPU and it is
	 * enabled, we trapped because of an
	 * instruction that isn't supported by this
	 * FPU, and so we need to emulate it.
	 */

	/*
	 * As an optimization, hppa_fpu_bootstrap
	 * replaces this branch instruction with a
	 * nop if there is a hardware FPU.
	 *
	 * Otherwise, this is the branch to emulate
	 * an FPU coprocessor.
	 */
ALTENTRY(hppa_fpu_nop0)
	b,n	L$emulate_fpu

	/*
	 * We have a hardware FPU.  If it is enabled,
	 * branch to emulate the instruction.
	 */
	mfctl	%ccr, %arg0
	extru,= %arg0, 25, 2, %r1
	b,n	L$emulate_fpu

	/*
	 * The hardware FPU is disabled, so we need to swap 
	 * in the FPU state of the process whose uspace
	 * physical address in CR_UPADDR.  We may also need 
	 * to swap out the FPU state of any process whose 
	 * uspace physical address is in the fpu_cur_uspace 
	 * variable.
	 */
	
	/*
	 * So far, the CTRAP() macro has saved %r1 in
	 * %tr7, and the dispatching above has saved
	 * %arg0 in tr2.  Save the other registers that 
	 * we want to use.  hppa_fpu_swap deliberately 
	 * uses only these registers and %r1 and %arg0.
	 */
	mtctl	%arg1, %tr3
	mtctl	%rp, %tr5

	/*
	 * Call hppa_fpu_swap.
	 */
	ldil	L%fpu_cur_uspace, %arg0
	ldw	R%fpu_cur_uspace(%arg0), %arg0
	mfctl	CR_UPADDR, %arg1
	blr	0, %rp
	b	hppa_fpu_swap
	nop
	
	/* Restore registers and rfi. */
	mfctl	%tr5, %rp
	mfctl	%tr3, %arg1
	mfctl	%tr2, %arg0
	mfctl	%tr7, %r1
	rfi
	nop

	/*
	 * We branch here to emulate a special function
	 * unit instruction.  On entry, %r1 is saved in %tr7
	 * (courtesy of CTRAP), and %arg0 is saved in %tr2
	 * (courtesy of the sfu/coprocessor dispatcher).
	 */
L$emulate_sfu:
	/*
	 * Currently we just restore %arg0 and
	 * trap with an illegal instruction.
	 */
	mfctl	%tr2, %arg0
	b	TLABEL(all)
	ldi	T_ILLEGAL, %r1

	/*
	 * We branch here to emulate a non-FPU coprocessor
	 * instruction.  On entry, %r1 is saved in %tr7
	 * (courtesy of CTRAP), and %t1 is saved in %tr2
	 * (courtesy of the sfu/coprocessor dispatcher).
	 */
L$emulate_coproc:
	/*
	 * Currently we just restore %arg0 and
	 * trap with an illegal instruction.
	 */
	mfctl	%tr2, %arg0
	b	TLABEL(all)
	ldi	T_ILLEGAL, %r1

	/*
	 * We branch here to emulate an FPU coprocessor
	 * instruction.  On entry, %r1 is saved in %tr7
	 * (courtesy of CTRAP), and %t1 is saved in %tr2
	 * (courtesy of the sfu/coprocessor dispatcher).
	 */
L$emulate_fpu:
	/*
	 * We get back to C via the normal generic trap 
	 * mechanism, as opposed to switching to a special 
	 * stack, setting up a trapframe, etc., ourselves,
	 * for three reasons.
	 *
	 * One, I want to turn interrupts back on, since
	 * the emulation code might not be fast.  Two, 
	 * because the instruction to emulate might be
	 * a load or a store, I need to turn address
	 * translation back on (i.e., return to virtual
	 * mode.)  Third, doing both of those plus
	 * setting up a trapframe is a pain, and the
	 * generic trap handling already does it all.
	 *
	 * To relieve trap() from having to check for
	 * sfu and non-FPU instructions again, it assumes
	 * that these kinds of instructions have already 
	 * been translated into some other trap type (as 
	 * they have, by the above L$emulate_sfu and 
	 * L$emulate_coproc), and all T_EMULATION | T_USER 
	 * traps are FPU instructions that need emulating.
	 *
	 * So we just restore %arg0 and trap with
	 * T_EMULATION.
	 */
	mfctl	%tr2, %arg0
	b	TLABEL(all)
	ldi	T_EMULATION, %r1
EXIT(TLABEL(emu))

/*
 * void hppa_fpu_swap(paddr_t out, paddr_t in);
 * or
 * void hppa_fpu_swap(struct pcb *out, NULL);
 */
LEAF_ENTRY_NOPROFILE(hppa_fpu_swap)

	/*
	 * Note that this function must work in 
	 * physical mode as well as virtual mode,
	 * because it can be called by a trap
	 * handler.  This also further restricts 
	 * the registers we can use.  We can only
	 * use %arg0, %arg1, and %r1.
	 */

	/*
	 * Assuming that out and in aren't both NULL,
	 * we will have to run coprocessor instructions,
	 * so we'd better enable it.
	 * 
	 * Also, branch if there's no FPU state to swap out.
	 */
	mfctl	%ccr, %r1
	depi	3, 25, 2, %r1
	comb,=	%r0, %arg0, L$fpu_swap_in
	mtctl	%r1, %ccr

	/*
	 * Swap out the current FPU state.
	 */
	ldo	PCB_FPREGS(%arg0), %arg0
	fstds,ma %fr0 , 8(%arg0)	/* fr0 must be saved first */
	fstds,ma %fr1 , 8(%arg0)
	fstds,ma %fr2 , 8(%arg0)
	fstds,ma %fr3 , 8(%arg0)
	fstds,ma %fr4 , 8(%arg0)
	fstds,ma %fr5 , 8(%arg0)
	fstds,ma %fr6 , 8(%arg0)
	fstds,ma %fr7 , 8(%arg0)
	fstds,ma %fr8 , 8(%arg0)
	fstds,ma %fr9 , 8(%arg0)
	fstds,ma %fr10, 8(%arg0)
	fstds,ma %fr11, 8(%arg0)
	fstds,ma %fr12, 8(%arg0)
	fstds,ma %fr13, 8(%arg0)
	fstds,ma %fr14, 8(%arg0)
	fstds,ma %fr15, 8(%arg0)
	fstds,ma %fr16, 8(%arg0)
	fstds,ma %fr17, 8(%arg0)
	fstds,ma %fr18, 8(%arg0)
	fstds,ma %fr19, 8(%arg0)
	fstds,ma %fr20, 8(%arg0)
	fstds,ma %fr21, 8(%arg0)
	fstds,ma %fr22, 8(%arg0)
	fstds,ma %fr23, 8(%arg0)
	fstds,ma %fr24, 8(%arg0)
	fstds,ma %fr25, 8(%arg0)
	fstds,ma %fr26, 8(%arg0)
	fstds,ma %fr27, 8(%arg0)
	fstds,ma %fr28, 8(%arg0)
	fstds,ma %fr29, 8(%arg0)
	fstds,ma %fr30, 8(%arg0)
	fstds    %fr31, 0(%arg0)

	ldo	-248(%arg0), %arg0
	ldil	L%dcache_stride, %r1
	ldw	R%dcache_stride(%r1), %r1
	fdc,m	%r1(%arg0)
	fdc,m	%r1(%arg0)
	fdc,m	%r1(%arg0)
	fdc,m	%r1(%arg0)
	fdc,m	%r1(%arg0)
	fdc,m	%r1(%arg0)
	fdc,m	%r1(%arg0)
	fdc,m	%r1(%arg0)
	sync

L$fpu_swap_in:

	/*
	 * Stash the incoming user structure in
	 * fpu_cur_uspace.  Because this variable
	 * holds a physical address, this means 
	 * that hppa_fpu_swap can only be called 
	 * with a non-zero user_in from physical 
	 * mode (i.e., from the emulation assist
	 * trap handler).  And that's exactly
	 * what happens now.
	 *
	 * So stash fpu_cur_uspace, branching
	 * past the swap-in code if it is zero.
	 */
	ldil	L%fpu_cur_uspace, %r1
	comb,=	%r0, %arg1, L$fpu_no_swap_in
	stw	%arg1, R%fpu_cur_uspace(%r1)

	/*
	 * Swap in the new FPU state.
	 */
	ldo	PCB_FPREGS+31*8(%arg1), %arg1
	fldds,ma -8(%arg1), %fr31
	fldds,ma -8(%arg1), %fr30
	fldds,ma -8(%arg1), %fr29
	fldds,ma -8(%arg1), %fr28
	fldds,ma -8(%arg1), %fr27
	fldds,ma -8(%arg1), %fr26
	fldds,ma -8(%arg1), %fr25
	fldds,ma -8(%arg1), %fr24
	fldds,ma -8(%arg1), %fr23
	fldds,ma -8(%arg1), %fr22
	fldds,ma -8(%arg1), %fr21
	fldds,ma -8(%arg1), %fr20
	fldds,ma -8(%arg1), %fr19
	fldds,ma -8(%arg1), %fr18
	fldds,ma -8(%arg1), %fr17
	fldds,ma -8(%arg1), %fr16
	fldds,ma -8(%arg1), %fr15
	fldds,ma -8(%arg1), %fr14
	fldds,ma -8(%arg1), %fr13
	fldds,ma -8(%arg1), %fr12
	fldds,ma -8(%arg1), %fr11
	fldds,ma -8(%arg1), %fr10
	fldds,ma -8(%arg1), %fr9
	fldds,ma -8(%arg1), %fr8
	fldds,ma -8(%arg1), %fr7
	fldds,ma -8(%arg1), %fr6
	fldds,ma -8(%arg1), %fr5
	fldds,ma -8(%arg1), %fr4
	fldds,ma -8(%arg1), %fr3
	fldds,ma -8(%arg1), %fr2
	fldds,ma -8(%arg1), %fr1
	fldds     0(%arg1), %fr0	/* fr0 must be restored last */

	ldil	L%dcache_stride, %r1
	ldw	R%dcache_stride(%r1), %r1
	fdc,m	%r1(%arg1)
	fdc,m	%r1(%arg1)
	fdc,m	%r1(%arg1)
	fdc,m	%r1(%arg1)
	fdc,m	%r1(%arg1)
	fdc,m	%r1(%arg1)
	fdc,m	%r1(%arg1)
	fdc,m	%r1(%arg1)
	sync

L$fpu_swap_done:

	/* Increment the switch count and return. */
	ldil	L%fpu_csw, %r1
	ldw	R%fpu_csw(%r1), %arg0
	ldo	1(%arg0), %arg0
	bv	%r0(%rp)
	stw	%arg0, R%fpu_csw(%r1)

L$fpu_no_swap_in:

	/* We didn't swap any FPU state in, so disable the FPU. */
	mfctl	%ccr, %r1
	depi	0, 25, 2, %r1
	b	L$fpu_swap_done
	mtctl	%r1, %ccr
EXIT(hppa_fpu_swap)

	/* Construct the virtual address tag. */
	/* NB: it is legal for off and t to be the same. */
#define VTAG(sp,off,t) \
	shd	%r0, off, 1, t		/* t[1..15] = off[0..14] */	! \
	dep	sp, 31, 16, t		/* put in the space id */	! \
	depi	1, 0, 1, t		/* and set the valid bit */

#if 0
	.section .bss
	.export	dtlb_c, data
	.export	tlbd_c, data
	.export	itlb_c, data
#if 0
	BSS(dtlb_c, 8)
	BSS(tlbd_c, 8)
	BSS(itlb_c, 8)
#else
dtlb_c:
	.block 8
tlbd_c:
	.block 8
itlb_c:
	.block 8
#endif
	.text

	/* XXX this touches tr5, which it should not, perhaps */
#define	TLB_STATS_PRE(t) \
	mfctl	CR_ITMR, %r17			! \
	mtctl	%r17, %tr5

#define	TLB_STATS_AFT(t) \
	mfctl	CR_ITMR, %r16			! \
	mfctl	%tr5, %r17			! \
	ldil	L%__CONCAT(t,_c), %r25		! \
	ldo	R%__CONCAT(t,_c)(%r25), %r25	! \
	sub	%r16, %r17, %r16		! \
	ldw	0(%r25), %r24			! \
	ldw	4(%r25), %r17			! \
	ldo	1(%r24), %r24			! \
	ldo	-2(%r16), %r16 /* for mtctl */	! \
	add	%r16, %r17, %r17		! \
	stw	%r24, 0(%r25)			! \
	stw	%r17, 4(%r25)

#else
#define	TLB_STATS_PRE(t)	/**/
#define	TLB_STATS_AFT(t)	/**/
#endif

#if defined(HP7000_CPU) || defined(HP7100_CPU) || defined(HP7200_CPU)
#define	TLB_PULL(bits,lbl)						! \
	/* space:pgaddr -- %r8:%r9 */					! \
	mfctl	CR_VTOP, %r16						! \
	ldwax,s	%r8(%r16), %r17		/* space -> page directory */	! \
	extru	%r9, 9, 10, %r25	/* r25 = r9[31..22] */		! \
	combt,=,n %r0, %r17, lbl					! \
	ldwax,s	%r25(%r17), %r24	/* page -> page table */	! \
	extru	%r9, 19, 10, %r16	/* r16 = r9[22..12] */		! \
	combt,=,n %r0, %r24, lbl					! \
	ldwax,s	%r16(%r24), %r17	/* va -> pa:prot */		! \
	sh2addl	%r16, %r24, %r25	/* r25 = r16 << 2 + r24 */	! \
	combt,=,n %r0, %r17, lbl					! \
	copy	%r17, %r16						! \
	depi	(bits), 21+bits, 1+bits, %r17				! \
	sub,=	%r16, %r17, %r0		/* do not store if unchanged */	! \
	stwas	%r17, 0(%r25)		/* store back w/ the bits */	! \
	shd	%r17, %r0, 13, %r25					! \
	dep	%r8, 30, 15, %r25	/* mix0r the pid from the sid */! \
	dep	%r0, 31, 12, %r17	/* needed ? */			! \
	addi	2, %r25, %r25						! \
	extru	%r17, 24, 25, %r17

	.align	32

/*
 * This is a handler for interruption 20, "TLB dirty bit trap".  It is used on
 * the PA7000 (PCX), PA7000 (PCX-S), and PA7100 (PCX-T).
 * Only shadowed registers are available, and they are:
 *
 * %r1 = C trap number
 * %r8 = data address space identifier. Copied from %isr
 * %r9 = data address offset. Copied from %ior
 * %r16 = undefined
 * %r17 = undefined
 * %r24 = undefined
 * %r25 = undefined
 */
LEAF_ENTRY_NOPROFILE(TLABEL(tlbd_s))
ALTENTRY(TLABEL(tlbd_t))
ALTENTRY(TLABEL(tlbd_x))
	TLB_STATS_PRE(tlbd)
	TLB_PULL(1, TLABEL(all))
	mfsp	%sr1, %r16
	mtsp	%r8, %sr1
	idtlba	%r17,(%sr1, %r9)
	idtlbp	%r25,(%sr1, %r9)
	mtsp	%r16, %sr1
	TLB_STATS_AFT(tlbd)
	rfir
	nop
EXIT(TLABEL(tlbd_s))

/*
 * This is a handler for interruption 6, "Instruction TLB miss fault". It is
 * used on the PA7000 (PCX), PA7000 (PCX-S), PA7100 (PCX-T) and PA7200 (PCX-T')
 * Only shadowed registers are available, and they are:
 *
 * %r1 = C trap number
 * %r8 = instruction address space identifier. Copied from %pcsq
 * %r9 = instruction address offset. Copied from %pcoq
 * %r16 = undefined
 * %r17 = undefined
 * %r24 = undefined
 * %r25 = undefined
 */
LEAF_ENTRY_NOPROFILE(TLABEL(itlb_s))
ALTENTRY(TLABEL(itlb_t))
ALTENTRY(TLABEL(itlb_x))
	TLB_STATS_PRE(itlb)
	TLB_PULL(0, TLABEL(all))
	extru,=	%r25, 5, 1, %r0	/* gate needs a kernel pid */
	depi	0, 30, 15, %r25
	mfsp	%sr1, %r16
	mtsp	%r8, %sr1
	iitlba	%r17,(%sr1, %r9)
	iitlbp	%r25,(%sr1, %r9)
	mtsp	%r16, %sr1
	TLB_STATS_AFT(itlb)
	rfir
	nop
EXIT(TLABEL(itlb_s))

/*
 * This is a handler for interruption 15, "Data TLB miss fault".  It is used on
 * the PA7000 (PCX), PA7000 (PCX-S), PA7100 (PCX-T) and PA7200 (PCX-T').
 * Only shadowed registers are available, and they are:
 *
 * %r1 = C trap number
 * %r8 = data address space identifier.  Copied from %isr.
 * %r9 = data address offset.  Copied from %ior.
 * %r16 = undefined
 * %r17 = undefined
 * %r24 = undefined
 * %r25 = undefined
 */
LEAF_ENTRY_NOPROFILE(TLABEL(dtlb_s))
ALTENTRY(TLABEL(dtlb_t))
ALTENTRY(TLABEL(dtlb_x))
	TLB_STATS_PRE(dtlb)
	TLB_PULL(0, TLABEL(all))
	mfsp	%sr1, %r16
	mtsp	%r8, %sr1
	idtlba	%r17,(%sr1, %r9)
	idtlbp	%r25,(%sr1, %r9)
	mtsp	%r16, %sr1
	TLB_STATS_AFT(dtlb)
	rfir
	nop
EXIT(TLABEL(dtlb_s))

/*
 * This is a handler for interruption 16, "Non-access instruction TLB miss
 * fault", and interrupt 17, "Non-access data TLB miss fault / Non-access data
 * page fault".  It is used on the PA7000 (PCX), PA7000 (PCX-S), PA7100 (PCX-T)
 * and PA7200 (PCX-T').
 *
 * Only shadowed registers are available, and they are:
 *
 * %r1 = C trap number
 * %r8 = instruction address space identifier. Copied from %isr.
 * %r9 = instruction address offset. Copied from %ior.
 * %r16 = undefined
 * %r17 = undefined
 * %r24 = undefined
 * %r25 = undefined
 */
LEAF_ENTRY_NOPROFILE(TLABEL(dtlbna_s))
ALTENTRY(TLABEL(itlbna_s))
ALTENTRY(TLABEL(dtlbna_t))
ALTENTRY(TLABEL(itlbna_t))
ALTENTRY(TLABEL(dtlbna_x))
ALTENTRY(TLABEL(itlbna_x))
	TLB_STATS_PRE(dtlb)
	TLB_PULL(0, L$dtlbna_t_fake)
	mfsp	%sr1, %r16
	mtsp	%r8, %sr1
	idtlba	%r17,(%sr1, %r9)
	idtlbp	%r25,(%sr1, %r9)
	mtsp	%r16, %sr1
	TLB_STATS_AFT(dtlb)
	rfir
	nop
L$dtlbna_s_fake:
L$dtlbna_t_fake:
	/* parse prober/w insns, have to decent to trap() to set regs proper */
	mfctl	%iir, %r16
	extru	%r16, 6, 6, %r24
	comib,=,n 1, %r24, TLABEL(all)
	extru	%r16, 24, 6, %r24
	subi,<>	0x23, %r24, %r0
	b	TLABEL(all)
	/* otherwise generate a flush-only tlb entry */
	copy	%r0, %r17
	zdep	%r8, 30, 15, %r25
	depi	-13, 11, 7, %r25
	ldo	2(%r25), %r25   /* 3? */
	mfsp	%sr1, %r16
	mtsp	%r8, %sr1
	idtlba	%r17,(%sr1, %r9)
	idtlbp	%r25,(%sr1, %r9)
	mtsp	%r16, %sr1
	TLB_STATS_AFT(dtlb)
	rfir
	nop
EXIT(TLABEL(dtlbna_s))

#endif /* defined(HP7000_CPU) || defined(HP7100_CPU) || defined(HP7200_CPU) */


#if defined(HP7100LC_CPU) || defined(HP7300LC_CPU)

#define	IITLBAF(r)	.word	0x04000440 | ((r) << 16)
#define	IITLBPF(r)	.word	0x04000400 | ((r) << 16)
#define	IDTLBAF(r)	.word	0x04001440 | ((r) << 16)
#define	IDTLBPF(r)	.word	0x04001400 | ((r) << 16)

/*
 * possible optimizations:
 *	change pte to reduce number of shifts
 *	reorder to reduce stalls
 */
#define	TLB_PULL_L(bits,lbl)						! \
	/* space:pgaddr -- %r8:%r9 */					! \
	mfctl	CR_VTOP, %r16						! \
	ldwx,s	%r8(%r16), %r17		/* space -> page directory */	! \
	extru	%r9, 9, 10, %r25					! \
	combt,=,n %r0, %r17, lbl					! \
	ldwx,s	%r25(%r17), %r24	/* page -> page table */	! \
	extru	%r9, 19, 10, %r16					! \
	combt,=,n %r0, %r24, lbl					! \
	ldwx,s	%r16(%r24), %r17	/* va -> pa:prot */		! \
	sh2addl	%r16, %r24, %r25					! \
	combt,=,n %r0, %r17, lbl					! \
	copy	%r17, %r16						! \
	depi	(bits), 21+bits, 1+bits, %r17				! \
	sub,=	%r16, %r17, %r0		/* do not store if unchanged */	! \
	stws	%r17, 0(%r25)		/* store back w/ the bits */	! \
	shd	%r17, %r0, 13, %r25					! \
	dep	%r8, 30, 15, %r25	/* mix0r the pid from the sid */! \
	dep	%r0, 31, 12, %r17	/* needed ? */			! \
	addi	2, %r25, %r25						! \
	extru	%r17, 24, 25, %r17	/* tlbbtop(%r17) */		! \
	sync

	.align	32

/*
 * This is a handler for interruption 20, "TLB dirty bit trap".  It is used on
 * the PA7100LC (PCX-L), PA7300LC (PCX-L2).
 * Only shadowed registers are available, and they are:
 *
 * %r1 = C trap number
 * %r8 = data address space identifier.  Copied from %ior.
 * %r9 = data address offset  Copied from %isr.
 * %r16 = undefined
 * %r17 = undefined
 * %r24 = undefined
 * %r25 = undefined
 */

LEAF_ENTRY_NOPROFILE(TLABEL(tlbd_l))
	TLB_STATS_PRE(tlbd)
	TLB_PULL_L(1, TLABEL(all))
	IDTLBAF(17)
	IDTLBPF(25)
#ifdef USE_HPT
	/* invalidate instead of update */
	mfctl	%cr28, %r17
	ldw	0(%r17), %r24
	VTAG(%r8, %r9, %r16)
	sub,<>	%r16, %r24, %r0
	stw	%r0, 0(%r17)
#endif
	TLB_STATS_AFT(tlbd)
	rfir
	nop
EXIT(TLABEL(tlbd_l))

/*
 * This is a handler for interruption 6, "Instruction TLB miss fault".  It is
 * used on the PA7100LC (PCX-L), PA7300LC (PCX-L2).
 * Only shadowed registers are available, and they are:
 *
 * %r1 = C trap number
 * %r8 = instruction address space identifier. Copied from %pcsq.
 * %r9 = instruction address offset. Copied from %pcoq.
 * %r16 = undefined
 * %r17 = undefined
 * %r24 = undefined
 * %r25 = undefined
 */

/*
 * from 7100lc ers, pg.6:
 * we found a post-silicon bug that makes cr28
 * unreliable for the itlb miss handler
 */
LEAF_ENTRY_NOPROFILE(TLABEL(itlb_l))
	TLB_STATS_PRE(itlb)
	TLB_PULL_L(0, TLABEL(all))
	extru,=	%r25, 5, 1, %r0	/* gate needs a kernel pid */
	depi	0, 30, 15, %r25
	IITLBAF(17)
	IITLBPF(25)
	TLB_STATS_AFT(itlb)
	rfir
	nop
EXIT(TLABEL(itlb_l))

/*
 * This is a handler for interruption 16, "Non-access instruction TLB miss
 * fault", and interrupt 17, "Non-access data TLB miss fault / Non-access data
 * page fault".  It is used on the PA7100LC (PCX-L), PA7300LC (PCX-L2).
 * Only shadowed registers are available, and they are:
 *
 * %r1 = C trap number
 * %r8 = instruction address space identifier. Copied from %isr.
 * %r9 = instruction address offset. Copied from %ior.
 * %r16 = undefined
 * %r17 = undefined
 * %r24 = undefined
 * %r25 = undefined
 */
LEAF_ENTRY_NOPROFILE(TLABEL(dtlbna_l))
ALTENTRY(TLABEL(itlbna_l))
	TLB_STATS_PRE(dtlb)
	TLB_PULL_L(0, L$dtlbna_l_fake)
	IDTLBAF(17)
	IDTLBPF(25)
	TLB_STATS_AFT(dtlb)
	rfir
	nop
L$dtlbna_l_fake:
	/* parse prober/w insns, have to decent to trap() to set regs proper */
	mfctl	%iir, %r16
	extru	%r16, 6, 6, %r24
	comib,=,n 1, %r24, TLABEL(all)
	extru	%r16, 24, 6, %r24
	subi,<>	0x23, %r24, %r0
	b	TLABEL(all)
	/* otherwise generate a flush-only tlb entry */
	copy	%r0, %r17
	zdep	%r8, 30, 15, %r25
	depi	-13, 11, 7, %r25
	ldo	2(%r25), %r25   /* 3? */
	IDTLBAF(17)
	IDTLBPF(25)
	TLB_STATS_AFT(dtlb)
	rfir
	nop
EXIT(TLABEL(dtlbna_l))

/*
 * This is a handler for interruption 15, "Data TLB miss fault".  It is used on
 * the PA7100LC (PCX-L), PA7300LC (PCX-L2).
 * Only shadowed registers are available, and they are:
 *
 * %r1 = C trap number
 * %r8 = instruction address space identifier. Copied from %isr.
 * %r9 = instruction address offset. Copied from %ior.
 * %r16 = undefined
 * %r17 = undefined
 * %r24 = undefined
 * %r25 = undefined
 */
LEAF_ENTRY_NOPROFILE(TLABEL(dtlb_l))
	TLB_STATS_PRE(dtlb)
	TLB_PULL_L(0, TLABEL(all))
	IDTLBAF(17)
	IDTLBPF(25)
#ifdef USE_HPT
	/*
	 * cache the next page mapping in the hpt.
	 *
	 * mapping for a page at the end of each 128k is uncachable
	 * in the hvt since it'd be in the tlb itself and thus there
	 * is no reason to cache it!
	 * as a side effect this avoids recomputing hpt entry and
	 * retraversing the whole page table each time.
	 */

	ldo	PAGE_SIZE(%r9), %r9
	extru,<> %r9, 20, 5, %r0
	b,n	L$dtlb_done_l	/* skip if no simple advance */
	/* do not check the PT overlap since the above
	 * check already guaranties that */

	/* ripped from TLB_PULL_L(0) */
	extru	%r9, 19, 10, %r16		/* %r24 was loaded in the TLB_PULL_L */
	ldwx,s	%r16(%r24), %r17		/* va -> pa:prot */
	sh2addl	%r16, %r24, %r25
	combt,=,n %r0, %r17, L$dtlb_done_l
	copy	%r17, %r16
	depi	0, 21, 1, %r17
	sub,=	%r16, %r17, %r0			/* do not store if unchanged */
	stws	%r17, 0(%r25)			/* store back w/ the bits */
	shd	%r17, %r0, 13, %r25
	dep	%r8, 30, 15, %r25		/* mix0r the pid from the sid */
	dep	%r0, 31, 12, %r17		/* needed ? */
	addi	2, %r25, %r25
	extru	%r17, 24, 25, %r17
	sync

	mfctl	%cr28, %r24
	VTAG(%r8, %r9, %r16)
	ldo	16(%r24), %r24
	stw	%r16, 0(%r24)
	stw	%r25, 4(%r24)
	stw	%r17, 8(%r24)
L$dtlb_done_l:
#endif
	TLB_STATS_AFT(dtlb)
	rfir
	nop
EXIT(TLABEL(dtlb_l))
#endif /* defined(HP7100LC_CPU) || defined(HP7300LC_CPU) */

#if defined(HP8000_CPU) || defined(HP8200_CPU) || defined(HP8500_CPU)
	.level	2.0w

	/* xlate 32bit->64bit pte */
#define	TLB_PCX2PCXU \
	extrw,u	%r25, 14, 13, %r16		! \
	depdi	0, 31, 32, %r17			! \
		/* fix io mappings */		! \
	extrd,s	%r17, 42, 4, %r1		! \
	addi,<>	1, %r1, %r0			! \
	depdi	-1, 38, 32, %r17		! \
		/* fix prom mappings */		! \
	extrd,s	%r17, 46, 8, %r1		! \
	addi,<>	0x10, %r1, %r0			! \
	depdi	0, 38, 4, %r17			! \
		/* weak ordering, dyn bp */	! \
	depwi	1, 31, 2, %r16			! \
	depdi	0, 44, 30, %r25			! \
	depd	%r16, 14, 15, %r25

/*
 * This is a handler for interruption 20, "TLB dirty bit trap".  It is used by
 * the PA8000 (PCX-U), PA8200 (PCX-W), PA8500 (PCX-W), and PA8600 (PCX-W+).
 * Only shadowed registers are available, and they are:
 *
 * %r1 = C trap number
 * %r8 = data address space identifier.  Copied from %isr.
 * %r9 = data address offset.  Copied from %ior.
 * %r16 = undefined
 * %r17 = undefined
 * %r24 = undefined
 * %r25 = undefined
 */
LEAF_ENTRY_NOPROFILE(TLABEL(tlbd_u))
	TLB_STATS_PRE(tlbd)
	TLB_PULL_L(1, TLABEL(all))
	TLB_PCX2PCXU
	idtlbt	%r17, %r25
	TLB_STATS_AFT(tlbd)
	rfir
	nop
EXIT(TLABEL(tlbd_u))

/*
 * This is a handler for interruption 6, "Instruction TLB miss fault".  It is
 * the PA8000 (PCX-U), PA8200 (PCX-W), PA8500 (PCX-W), and PA8600 (PCX-W+).
 * Only shadowed registers are available, and they are:
 *
 * %r1 = C trap number
 * %r8 = data address space identifier.   Copied from %isr.
 * %r9 = data address offset.  Copied from %ior.
 * %r16 = undefined
 * %r17 = undefined
 * %r24 = undefined
 * %r25 = undefined
 */
LEAF_ENTRY_NOPROFILE(TLABEL(itlb_u))
	TLB_STATS_PRE(itlb)
	TLB_PULL_L(0, TLABEL(all))
	extru,=	%r25, 5, 1, %r0	/* gate needs a kernel pid */
	depi	0, 30, 15, %r25
	TLB_PCX2PCXU
	iitlbt	%r17, %r25
	TLB_STATS_AFT(itlb)
	rfir
	nop
EXIT(TLABEL(itlb_u))


/*
 * This is a handler for interruption 16, "Non-access instruction TLB miss
 * fault", and interrupt 17, "Non-access data TLB miss fault / Non-access data
 * page fault".  It is the PA8000 (PCX-U), PA8200 (PCX-W), PA8500 (PCX-W),
 * and PA8600 (PCX-W+).
 * Only shadowed registers are available, and they are:
 *
 * %r1 = C trap number
 * %r8 = instruction address space identifier. Copied from %isr.
 * %r9 = instruction address offset. Copied from %ior.
 * %r16 = undefined
 * %r17 = undefined
 * %r24 = undefined
 * %r25 = undefined
 */
LEAF_ENTRY_NOPROFILE(TLABEL(dtlbna_u))
ALTENTRY(TLABEL(itlbna_u))
	TLB_STATS_PRE(dtlb)
	TLB_PULL_L(0, L$dtlbna_u_fake)
	TLB_PCX2PCXU
	idtlbt	%r17, %r25
	TLB_STATS_AFT(dtlb)
	rfir
	nop
L$dtlbna_u_fake:
	/* parse prober/w insns, have to decent to trap() to set regs proper */
	mfctl	%iir, %r16
	extru	%r16, 6, 6, %r24
	comib,=,n 1, %r24, TLABEL(all)
	extru	%r16, 24, 6, %r24
	subi,<>	0x23, %r24, %r0
	b	TLABEL(all)
	/* otherwise generate a flush-only tlb entry */
	copy	%r0, %r17
	zdep	%r8, 30, 15, %r25
	depi	-13, 11, 7, %r25
	ldo	2(%r25), %r25   /* 3? */
	idtlbt	%r17, %r25
	TLB_STATS_AFT(dtlb)
	rfir
	nop
EXIT(TLABEL(dtlbna_u))

/*
 * This is a handler for interruption 15, "Data TLB miss fault".  It is the
 * PA8000 (PCX-U), PA8200 (PCX-W), PA8500 (PCX-W), and PA8600 (PCX-W+).
 * Only shadowed registers are available, and they are:
 *
 * %r1 = C trap number
 * %r8 = instruction address space identifier. Copied from %isr.
 * %r9 = instruction address offset. Copied from %ior.
 * %r16 = undefined
 * %r17 = undefined
 * %r24 = undefined
 * %r25 = undefined
 */
LEAF_ENTRY_NOPROFILE(TLABEL(dtlb_u))
	TLB_STATS_PRE(dtlb)
	TLB_PULL_L(0, TLABEL(all))
	TLB_PCX2PCXU
	idtlbt	%r17, %r25
	TLB_STATS_AFT(dtlb)
	rfir
	nop
EXIT(TLABEL(dtlb_u))

	.level	1.1
#endif /* HP8000_CPU */

#if defined(HP7000_CPU) || defined(HP7100_CPU)
/*
 * int desidhash_s(void)
 * int desidhash_x(void)
 */
LEAF_ENTRY_NOPROFILE(desidhash_s)
ALTENTRY(desidhash_x)
	sync
	MFCPU_T(DR_CPUCFG,22)	/* %t1 */
	MFCPU_T(DR_CPUCFG,22)
	nop
	nop
	depi	0, DR0_PCXS_DHE, 3, %t1	/* 3 4 DR0_PCXS_DOMAIN|DR0_PCXS_IHE */
	depi	1, DR0_PCXS_EQWSTO, 1, %t1
	depi	0, DR0_PCXS_DHPMC, 1, %t1
	depi	0, DR0_PCXS_ILPMC, 1, %t1
	sync
	MTCPU_T(22,DR_CPUCFG)
	MTCPU_T(22,DR_CPUCFG)
	nop
	nop
	bv	0(%rp)
	extru	%t1, 4, 5, %ret0	/* return chip revision */
EXIT(desidhash_s)
#endif /* defined(HP7000_CPU) || defined(HP7100_CPU) */

#if defined(HP7100_CPU) || defined(HP7200_CPU)
/*
 * void desidhash_t(void)
 */
LEAF_ENTRY_NOPROFILE(desidhash_t)
	sync
	MFCPU_T(DR_CPUCFG,22)	/* %t1 */
	MFCPU_T(DR_CPUCFG,22)
	nop
	nop
	depi	0, DR0_PCXT_IHE, 1, %t1
	depi	0, DR0_PCXT_DHE, 1, %t1
	depi	0, DR0_PCXT_DHPMC, 1, %t1
	depi	0, DR0_PCXT_ILPMC, 1, %t1
	sync
	MTCPU_T(22,DR_CPUCFG)
	MTCPU_T(22,DR_CPUCFG)
	nop
	nop
	bv	0(%rp)
	extru	%t1, 4, 5, %ret0	/* return chip revision */
EXIT(desidhash_t)
#endif /* defined(HP7100_CPU) || defined(HP7200_CPU) */

#if defined(HP7100LC_CPU) || defined(HP7300LC_CPU)
/*
 * int
 * ibtlb_l(int i, pa_space_t sp, vaddr_t va, paddr_t pa, vsize_t sz,
 *     u_int prot)
 */
LEAF_ENTRY_NOPROFILE(ibtlb_l)
	rsm	(PSW_R|PSW_I), %t4
	nop ! nop ! nop ! nop ! nop ! nop ! nop	/* XXXNH why? */

	bv	0(%rp)
	mtsm	%t4
EXIT(ibtlb_l)

/*
 * void
 * hpti_l(addr,size)
*/
LEAF_ENTRY_NOPROFILE(hpti_l)
	ldo	-1(%arg1), %arg1
	depi	0, 31, 12, %arg1
	ldi	0x1c0, %t1		/* cache size assumed 128k XXX */
	or	%arg0, %t1, %arg0
	sync
	MTCPU_C(26,DR0_PCXL2_HTLB_ADDR)
	MTCPU_C(25,DR0_PCXL2_HTLB_CFG)
	nop
	nop
	bv,n	%r0(%rp)
	nop
EXIT(hpti_l)

/*
 * int
 * pbtlb_l(int i)
 */
LEAF_ENTRY_NOPROFILE(pbtlb_l)
	; DR_PAGE0
	rsm	(PSW_R|PSW_I), %t4
	nop ! nop ! nop ! nop 
	ldil	L%0xc041, %t1
	ldo	R%0xc041(%t1), %t1
	dep	%arg0, 30, 3, %t1
	sync
	MTCPU_T(22,DR_DTLB)	/* t1 */
	nop
	nop
	mtsp	%r0, %sr1
	idtlba	%r0,(%sr1,%r0)
	idtlbp	%r0,(%sr1,%r0)
	zdepi	-1, 18, 1, %t1
	nop
	sync
	MTCPU_T(22,DR_DTLB)
	nop
	nop
	bv	0(%rp)
	mtsm	%t4
EXIT(pbtlb_l)

/*
 * int desidhash_l(void)
 */
LEAF_ENTRY_NOPROFILE(desidhash_l)
	MFCPU_C(DR_CPUCFG,22)	/* t1 */
	nop
	nop
	depi	0, DR0_PCXL_L2IHASH_EN, 2, %t1	/* + DR0_PCXL_L2DHASH_EN */
	depi	0, DR0_PCXL_L2IHPMC, 1, %t1	/* don't reset */
	depi	0, DR0_PCXL_L2DHPMC, 1, %t1	/* don't reset */
	depi	0, DR0_PCXL_L1IHPMC, 1, %t1	/* don't reset */
	depi	0, DR0_PCXL_L2PARERR,1, %t1	/* don't reset */
	sync
		/* set DR0_PCXL_L1ICACHE_EN ??? */
	MTCPU_C(22,DR_CPUCFG)
	nop
	nop
	bv	0(%rp)
	extru	%t1, 4, 5, %ret0	/* return chip revision */
EXIT(desidhash_l)

#endif /* defined(HP7100LC_CPU) || defined(HP7300LC_CPU) */

#if defined(HP8000_CPU) || defined(HP8200_CPU) || defined(HP8500_CPU)
	.level	2.0w
LEAF_ENTRY_NOPROFILE(desidhash_u)
	MFCPU_U(2,28)
	depdi	0, 54, 1, %r28
	MTCPU_U(28,2)
	bv	%r0(%rp)
	copy	%r0, %ret0	/* XXX dunno how to get chip rev */
EXIT(desidhash_u)

LEAF_ENTRY_NOPROFILE(ibtlb_u)
	/* TODO insert a locked large tlb entry */
	bv	0(%rp)
	nop
EXIT(ibtlb_u)

LEAF_ENTRY_NOPROFILE(pbtlb_u)
	/* TODO purge a locked tlb entry */
	bv	0(%rp)
	nop
EXIT(pbtlb_u)
	.level	1.1
#endif /* HP8000_CPU */

	.align	64
	.export	TLABEL(all), entry
ENTRY_NOPROFILE(TLABEL(all),0)
	/*
	 * at this point we have:
	 *  - psw copied into ipsw
	 *  - psw	E(default), M(1 if HPMC, else 0), all others 0, i.e.
	 *		interrupts masked, absolute accesses are enabled, etc.
	 *  - PL	0 (highest privilege)
	 *  - r1, r8, r9, r16, r17, r24, r25 shadowed (maybe)
	 *  - r1:	trap number
	 *  - tr7:	old r1
	 */

	/* do not overwrite %tr4(%cr28) it contains the contents of r24 */
	mtctl	%t3, %tr2

	ldil	L%L$trap_tmp_save, %t3
	ldo	R%L$trap_tmp_save(%t3), %t3
	stw	%t1, TF_R22(%t3)	/* use ,bc */
	stw	%t2, TF_R21(%t3)

	mfctl	%tr2, %t1
	stw	%sp, TF_R30(%t3)	/* sp */
	stw	%t1, TF_R20(%t3)	/* t3 */

	/*
	 * Now, save away other volatile state that prevents us from turning
	 * the PC queue back on, namely, the pc queue and ipsw, and the
	 * interrupt information.
	 */

	mfctl	%eiem, %t1
	mfctl	%ipsw, %t2
	stw	%t1, TF_CR15(%t3)	/* use ,bc */
	stw	%t2, TF_CR22(%t3)

	mfsp	%sr3, %t1
	mfctl	%pidr1, %t2
	stw	%t1, TF_SR3(%t3)
	stw	%t2, TF_CR8(%t3)

	/*
	 * Setup kernel context
	 */

	ldi	HPPA_PID_KERNEL,%t1
	mtctl	%t1, %pidr1
	mtsp	%r0, %sr3

	/* this will enable interrupts after `cold' */
	ldil	L%kpsw, %t1
	ldw	R%kpsw(%t1), %t2
	mtctl	%r0, %eiem
	mtctl	%t2, %ipsw

	/* save interruption instruction address space queue */
	mfctl	%pcsq, %t1
	mtctl	%r0, %pcsq
	mfctl	%pcsq, %t2
	stw	%t1, TF_IISQH(%t3)	/* use ,bc */
	stw	%t2, TF_IISQT(%t3)
	mtctl	%r0, %pcsq

	/*
	 * Set up the kernel stack pointer.  If the trap happened while we were
	 * in unprivileged code, or in privileged code in the SYSCALLGATE page,
	 * move to the kernel stack in curlwp's PCB; otherwise, start a new
	 * stack frame on whatever kernel stack we're already on.
	 *
	 * This used to check only for a trap while we were in unprivileged
	 * code, but this ignored the possibility that a trap could come in
	 * during the period between a gateway instruction to raise privilege
	 * and the disabling of interrupts.  During this period we're still on
	 * the user's stack, and we must move to the kernel stack.
	 */
	mfctl	%pcoq, %t1
	ldil	L%SYSCALLGATE, %t2
	/* Start aligning the assumed kernel sp. */
	ldo	HPPA_FRAME_SIZE-1(%sp), %sp
	/* This dep leaves t2 with SYSCALLGATE | (pcoqh & PAGE_MASK). */
	dep	%t1, 31, PGSHIFT, %t2
	/* Nullify if pcoqh & HPPA_PC_PRIV_MASK != 0. */
	dep,<>	%t1, 31, 2, %r0
	/* Branch if (pcoqh & ~PAGE_MASK) != SYSCALLGATE */
	comb,<> %t1, %t2, L$trap_from_kernel
	/* Finish aligning the assumed kernel sp. */
	dep	%r0, 31, 6, %sp

	mfctl	CR_UPADDR, %t2
	depi	1, T_USER_POS, 1, %r1
	depi	1, TFF_LAST_POS, 1, %r1
	ldw	PCB_UVA(%t2), %sp
#ifdef DIAGNOSTIC
	b	L$trap_have_stack
#endif
	ldo	NBPG(%sp), %sp

L$trap_from_kernel:
#ifdef DIAGNOSTIC
	/*
	 * Use the emergency stack if we have taken some kind
	 * of TLB or protection fault on the kernel stack.
	 */
	mtctl	%t1, %tr2
	ldw	TF_R30(%t3), %t1
	mfctl	%ior, %t2
	dep	%r0, 31, PGSHIFT, %t1
	dep	%r0, 31, PGSHIFT, %t2
	comb,=,n %t1, %t2, 0
	ldo	NBPG(%t1), %t1
	comb,<>	%t1, %t2, L$trap_have_stack
	mfctl	%tr2, %t1
	mfctl	%isr, %t2
	comib,<>,n HPPA_SID_KERNEL, %t2, L$trap_have_stack
#define _CHECK_TRAP_TYPE(tt) ldi tt, %t2 ! comb,= %r1, %t2, L$trap_kstack_fault
	_CHECK_TRAP_TYPE(T_ITLBMISS)
	_CHECK_TRAP_TYPE(T_DTLBMISS)
	_CHECK_TRAP_TYPE(T_ITLBMISSNA)
	_CHECK_TRAP_TYPE(T_DTLBMISSNA)
	_CHECK_TRAP_TYPE(T_DPROT)
	_CHECK_TRAP_TYPE(T_DATACC)
	_CHECK_TRAP_TYPE(T_DATAPID)
	ldi	T_DATALIGN, %t2
	comb,<>,n %r1, %t2, L$trap_have_stack
#undef _CHECK_TRAP_TYPE
L$trap_kstack_fault:
	ldil	L%emergency_stack_start, %sp
	ldo	R%emergency_stack_start(%sp), %sp
L$trap_have_stack:
#endif
	ldil	L%trapnowvirt, %t2
	ldo	R%trapnowvirt(%t2), %t2
	mtctl	%t2, %pcoq
	stw	%t1, TF_IIOQH(%t3)
	ldo	4(%t2), %t2
	mfctl	%pcoq, %t1
	stw	%t1, TF_IIOQT(%t3)
	mtctl	%t2, %pcoq

	/* save the interruption space and offset registers */
	mfctl	%isr, %t1
	mfctl	%ior, %t2
	stw	%t1, TF_CR20(%t3)	/* use ,bc */
	stw	%t2, TF_CR21(%t3)

	/* save the interruption instruction register */
	mfctl	%iir, %t2
	stw	%t2, TF_CR19(%t3)

	/* save the trap type and flags */
	stw	%r1, TF_FLAGS(%t3)

	/* XXX not in use... yet */
	/* XXX save %ccr here w/ rctr */
	mfctl	%rctr, %t1		/* gotta get it before R is up */

	copy	%sp, %t3
	ldo	HPPA_FRAME_SIZE+TRAPFRAME_SIZEOF(%sp), %sp

#if defined(DDB) || defined(KGDB)
	/*
	 * Match the offset from %sp for the trapframe with syscall_entry
	 */
	ldo	HPPA_FRAME_MAXARGS+HPPA_FRAME_SIZE-1(%sp),%sp
	depi	0, 31, 6, %sp
#endif
	rfir
	nop ! nop ! nop ! nop ! nop ! nop ! nop ! nop
trapnowvirt:
	/*
	 * t3 contains the virtual address of the trapframe
	 * sp is loaded w/ the right VA (we did not need it being physical)
	 */

	mfctl	%ccr, %t2
	stw	%t1, TF_CR0(%sr3, %t3)
	stw	%t2, TF_CR10(%sr3, %t3)

	mfsp	%sr0, %t1
	mfsp	%sr1, %t2
	stw	%t1, TF_SR0(%sr3, %t3)
	stw	%t2, TF_SR1(%sr3, %t3)

	mfsp	%sr2, %t1
	mfsp	%sr4, %t2
	stw	%t1, TF_SR2(%sr3, %t3)
	stw	%t2, TF_SR4(%sr3, %t3)

	mfsp	%sr5, %t2
	mfsp	%sr6, %t1
	stw	%t2, TF_SR5(%sr3, %t3)
	stw	%t1, TF_SR6(%sr3, %t3)

	mfsp	%sr7, %t1
	mfctl	%pidr2, %t2
	stw	%t1, TF_SR7(%sr3, %t3)
	stw	%t2, TF_CR9(%sr3, %t3)

	mtsp	%r0, %sr0
	mtsp	%r0, %sr1
	mtsp	%r0, %sr2
	mtsp	%r0, %sr4
	mtsp	%r0, %sr5
	mtsp	%r0, %sr6
	mtsp	%r0, %sr7

#if pbably_not_worth_it
	mfctl	%pidr3, %t1
	mfctl	%pidr4, %t2
	stw	%t1, TF_CR12(%t3)
	stw	%t2, TF_CR13(%t3)
#endif

	/*
	 * Save all general registers that we haven't saved already
	 */

	/* XXXNH check this!!! */
#if defined(DDB) || defined(KGDB)
	stw	%rp, HPPA_FRAME_CRP(%sp)
	stw	%r0, -HPPA_FRAME_SIZE(%sp)
#endif
	stw	%t3, -HPPA_FRAME_SIZE+4(%sp)

	mfctl	%sar, %t1		/* use ,bc each cache line */
	stw	%t1, TF_CR11(%t3)
	stw	%r1, TF_R1(%t3)
	stw	%r2, TF_R2(%t3)
	stw	%r3, TF_R3(%t3)

	/*
	 * Copy partially saved state from the store into the frame
	 */
	ldil	L%L$trap_tmp_save, %t2
	ldo	R%L$trap_tmp_save(%t2), %t2
	/* use ,bc each line */
	ldw  0(%t2), %r1 ! ldw  4(%t2), %t1 ! stw %r1,  0(%t3) ! stw %t1,  4(%t3)
	ldw  8(%t2), %r1 ! ldw 12(%t2), %t1 ! stw %r1,  8(%t3) ! stw %t1, 12(%t3)
	ldw 16(%t2), %r1 ! ldw 20(%t2), %t1 ! stw %r1, 16(%t3) ! stw %t1, 20(%t3)
	ldw 24(%t2), %r1 ! ldw 28(%t2), %t1 ! stw %r1, 24(%t3) ! stw %t1, 28(%t3)
	ldw 32(%t2), %r1 ! ldw 36(%t2), %t1 ! stw %r1, 32(%t3) ! stw %t1, 36(%t3)
	ldw 40(%t2), %r1 ! ldw 44(%t2), %t1 ! stw %r1, 40(%t3) ! stw %t1, 44(%t3)
	ldw 48(%t2), %r1 ! ldw 52(%t2), %t1 ! stw %r1, 48(%t3) ! stw %t1, 52(%t3)
	ldw 56(%t2), %r1 ! ldw 60(%t2), %t1 ! stw %r1, 56(%t3) ! stw %t1, 60(%t3)

	/*
	 * Normally, we'd only have to save and restore the caller-save
	 * registers, because the callee-save registers will be saved and
	 * restored automatically by our callee(s).
	 *
	 * However, in two cases we need to save and restore all of the general
	 * registers in the trapframe.  One, if we're running a debugger, we
	 * want the debugging person to be able to see and change any and all
	 * general register values at the trap site.  Two, if we have an FPU
	 * emulator, this trap may be to emulate an instruction that needs to
	 * read and write any and all general registers (for example, a load
	 * or store instruction with a modify completer).
	 *
	 * See similar #ifdefs in the syscall entry and exit code.
	 */
#if defined(DDB) || defined(KGDB) || defined(FPEMUL)
	stw	%r4, TF_R4(%t3)
	stw	%r5, TF_R5(%t3)
	stw	%r6, TF_R6(%t3)
	stw	%r7, TF_R7(%t3)
	stw	%r8, TF_R8(%t3)
	stw	%r9, TF_R9(%t3)
	stw	%r10, TF_R10(%t3)
	stw	%r11, TF_R11(%t3)
	stw	%r12, TF_R12(%t3)
	stw	%r13, TF_R13(%t3)
	stw	%r14, TF_R14(%t3)
	stw	%r15, TF_R15(%t3)
	stw	%r16, TF_R16(%t3)
	stw	%r17, TF_R17(%t3)
	stw	%r18, TF_R18(%t3)
#endif /* DDB || KGDB || FPEMUL */
	stw	%t4, TF_R19(%t3)
	stw	%r23,TF_R23(%t3)
	stw	%r24,TF_R24(%t3)
	stw	%r25,TF_R25(%t3)
	stw	%r26,TF_R26(%t3)
	stw	%r27,TF_R27(%t3)
	stw	%r28,TF_R28(%t3)
	stw	%r29,TF_R29(%t3)
	stw	%r31,TF_R31(%t3)

	/*
	 * Save the necessary control registers that have not already saved.
	 */
#if defined(DDB) || defined(KGDB)
	/*
	 * Save v2p translation table pointer
	 */
	mfctl	%eirr, %t1
	mfctl	CR_VTOP, %t2
	stw	%t1, TF_CR23(%t3)
	stw	%t2, TF_CR25(%t3)

	mfctl	%cr28, %t2
	stw	%t2, TF_CR28(%t3)
#endif
	mfctl	CR_UPADDR, %t1
	stw	%t1, TF_CR30(%t3)

	/*
	 * load the global pointer for the kernel
	 */

	ldil	L%$global$, %dp
	ldo	R%$global$(%dp), %dp

	/*
	 * call the C routine trap().
	 * form trap type in the first argument to trap()
	 */
	ldw	TF_FLAGS(%t3), %arg0
	dep	%r0, 24, 25, %arg0
	copy	%t3, %arg1

#if defined(DDB) || defined(KGDB)
	/* Mark frame pointer as NULL to indicate syscall/trap */
	copy	%r0, %r3
#endif

	.import	trap, code
	CALL(trap, %t1)

	ldw	-HPPA_FRAME_SIZE+4(%sp), %t3
	/* see if curlwp has changed */
	ldw	TF_FLAGS(%t3), %arg0
	bb,>=,n	%arg0, TFF_LAST_POS, L$trap_return
	nop

	mfctl	CR_CURLWP, %t2
	ldw	L_MD(%t2), %t3

L$trap_return:
	ldil	L%syscall_return, %t1
	ldo	R%syscall_return(%t1), %t1
	bv,n	%r0(%t1)
	nop

EXIT(TLABEL(all))

	.align	32
ENTRY_NOPROFILE(TLABEL(ibrk),0)
	/* If called by a user process then always pass it to trap() */
	mfctl	%pcoq, %r8
	extru,=	%r8, 31, 2, %r0
	b,n	L$ibrk_bad

	/* don't accept breaks from data segments */
	.import etext
	ldil	L%etext, %r9
	ldo	R%etext(%r9), %r9
	comb,>>=,n %r8, %r9, L$ibrk_bad

	mfctl	%iir, %r8
	extru	%r8, 31, 5, %r9
	comib,<>,n HPPA_BREAK_KERNEL, %r9, L$ibrk_bad

	/* now process all those `break' calls we make */
	extru	%r8, 18, 13, %r9
	comib,=,n HPPA_BREAK_GET_PSW, %r9, L$ibrk_getpsw
	comib,=,n HPPA_BREAK_SET_PSW, %r9, L$ibrk_setpsw

L$ibrk_bad:
	/* illegal (unimplemented) break entry point */
	b	TLABEL(all)
	nop

L$ibrk_getpsw:
	b	L$ibrk_exit
	mfctl	%ipsw, %ret0

L$ibrk_setpsw:
	mfctl	%ipsw, %ret0
	b	L$ibrk_exit
	mtctl	%arg0, %ipsw

	/* insert other fast breaks here */
	nop ! nop

L$ibrk_exit:
	/* skip the break */
	mtctl	%r0, %pcoq
	mfctl	%pcoq, %r9
	mtctl	%r9, %pcoq
	ldo	4(%r9), %r9
	mtctl	%r9, %pcoq
	rfir
	nop
EXIT(TLABEL(ibrk))
