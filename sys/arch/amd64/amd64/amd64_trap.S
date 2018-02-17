/*	$NetBSD: amd64_trap.S,v 1.25 2018/02/17 20:22:05 maxv Exp $	*/

/*
 * Copyright (c) 1998, 2007, 2008, 2017 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum, by Andrew Doran and by Maxime Villard.
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
 * Copyright (c) 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Frank van der Linden for Wasabi Systems, Inc.
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

#include <machine/asm.h>

#include "opt_xen.h"
#include "opt_dtrace.h"

#define ALIGN_TEXT	.align 16,0x90

#include <machine/frameasm.h>
#include <machine/segments.h>
#include <machine/trap.h>
#include <machine/specialreg.h>

#include "assym.h"

/*
 * Trap and fault vector routines
 *
 * On exit from the kernel to user mode, we always need to check for ASTs.  In
 * addition, we need to do this atomically; otherwise an interrupt may occur
 * which causes an AST, but it won't get processed until the next kernel entry
 * (possibly the next clock tick).  Thus, we disable interrupt before checking,
 * and only enable them again on the final `iret' or before calling the AST
 * handler.
 */

#ifdef	XEN
#define	PRE_TRAP	movq (%rsp),%rcx ; movq 8(%rsp),%r11 ; addq $0x10,%rsp
#else
#define	PRE_TRAP
#endif

#ifdef XEN
/*
 * I don't believe XEN generates in-kernel traps for the
 * equivalent of iret, if it does this code would be needed
 * in order to copy the user segment registers into the fault frame.
 */
#define check_swapgs alltraps
#endif

#define	TRAP_NJ(a)	PRE_TRAP ; pushq $(a)
#define	ZTRAP_NJ(a)	PRE_TRAP ; pushq $0 ; pushq $(a)
#define	TRAP(a)		TRAP_NJ(a) ;
#define	ZTRAP(a)	ZTRAP_NJ(a) ;

.macro	TRAP_ENTRY_POINT	name,code,is_ztrap
IDTVEC(\name)
	.if	\is_ztrap
		ZTRAP(\code)
	.else
		TRAP(\code)
	.endif
	INTRENTRY
	jmp	.Lalltraps_noentry
IDTVEC_END(\name)
.endm

.macro	TRAP_ENTRY_POINT_SWAPGS	name,code,is_ztrap
IDTVEC(\name)
	.if	\is_ztrap
		ZTRAP_NJ(\code)
	.else
		TRAP_NJ(\code)
	.endif
	jmp	check_swapgs
IDTVEC_END(\name)
.endm

	.text

	TEXT_USER_BEGIN

TRAP_ENTRY_POINT	trap00,T_DIVIDE,1
TRAP_ENTRY_POINT	trap01,T_TRCTRAP,1

/*
 * Non Maskable Interrupts are a special case: they can be triggered even
 * with interrupts disabled, and once triggered they block further NMIs
 * until an 'iret' instruction is executed.
 *
 * Therefore we don't enable interrupts, because the CPU could switch to
 * another LWP, call 'iret' and unintentionally leave the NMI mode.
 *
 * We need to be careful about %gs too, because it is possible that we were
 * running in kernel mode with a userland %gs.
 */
IDTVEC(trap02)
#if defined(XEN)
	ZTRAP(T_NMI)
#else
	ZTRAP_NJ(T_NMI)
	subq	$TF_REGSIZE,%rsp
	INTR_SAVE_GPRS
	SVS_ENTER_ALTSTACK
	cld
	SMAP_ENABLE
	movw	%gs,TF_GS(%rsp)
	movw	%fs,TF_FS(%rsp)
	movw	%es,TF_ES(%rsp)
	movw	%ds,TF_DS(%rsp)

	movl	$MSR_GSBASE,%ecx
	rdmsr
	cmpl	$VM_MIN_KERNEL_ADDRESS_HIGH32,%edx
	jae	.Lnoswapgs

	swapgs
	movq	%rsp,%rdi
	incq	CPUVAR(NTRAP)
	call	_C_LABEL(nmitrap)
	SVS_LEAVE_ALTSTACK
	swapgs
	jmp	.Lnmileave

.Lnoswapgs:
	movq	%rsp,%rdi
	incq	CPUVAR(NTRAP)
	call	_C_LABEL(nmitrap)
	SVS_LEAVE_ALTSTACK

.Lnmileave:
	INTR_RESTORE_GPRS
	addq	$TF_REGSIZE+16,%rsp
	iretq
#endif
IDTVEC_END(trap02)

IDTVEC(trap03)
#ifndef KDTRACE_HOOKS
	ZTRAP(T_BPTFLT)
#else
	ZTRAP_NJ(T_BPTFLT)
	INTRENTRY
	STI(si)
	/*
	 * DTrace Function Boundary Trace (fbt) probes are triggered
	 * by int3 (0xcc).
	 */
	/* Check if there is no DTrace hook registered. */
	cmpq	$0,dtrace_invop_jump_addr
	je	calltrap

	/*
	 * Set our jump address for the jump back in the event that
	 * the exception wasn't caused by DTrace at all.
	 */
	/* XXX: This doesn't look right for SMP - unless it is a
	 * constant - so why set it everytime. (dsl) */
	movq	$calltrap, dtrace_invop_calltrap_addr(%rip)

	/* Jump to the code hooked in by DTrace. */
	movq	dtrace_invop_jump_addr, %rax
	jmpq	*dtrace_invop_jump_addr
#endif
IDTVEC_END(trap03)

TRAP_ENTRY_POINT	trap04,T_OFLOW,1
TRAP_ENTRY_POINT	trap05,T_BOUND,1
TRAP_ENTRY_POINT	trap06,T_PRIVINFLT,1

IDTVEC(trap07)
	ZTRAP_NJ(T_DNA)
	INTRENTRY
#ifdef DIAGNOSTIC
	movl	CPUVAR(ILEVEL),%ebx
#endif
	movq	%rsp,%rdi
	call	_C_LABEL(fpudna)
	jmp	.Lalltraps_checkusr
IDTVEC_END(trap07)

/*
 * Double faults execute on a particular stack, and we must not jump out
 * of it. So don't enable interrupts.
 */
IDTVEC(trap08)
#if defined(XEN)
	TRAP(T_DOUBLEFLT)
#else
	TRAP_NJ(T_DOUBLEFLT)
	subq	$TF_REGSIZE,%rsp
	INTR_SAVE_GPRS
	SVS_ENTER_ALTSTACK
	testb	$SEL_UPL,TF_CS(%rsp)
	jz	1f
	swapgs
1:
	cld
	SMAP_ENABLE
	movw	%gs,TF_GS(%rsp)
	movw	%fs,TF_FS(%rsp)
	movw	%es,TF_ES(%rsp)
	movw	%ds,TF_DS(%rsp)

	movq	%rsp,%rdi
	incq	CPUVAR(NTRAP)
	call	_C_LABEL(doubletrap)

	SVS_LEAVE_ALTSTACK
	INTR_RESTORE_GPRS

	testb	$SEL_UPL,TF_CS(%rsp)
	jz	1f
	swapgs
1:
	addq	$TF_REGSIZE+16,%rsp
	iretq
#endif
IDTVEC_END(trap08)

TRAP_ENTRY_POINT	trap09,T_FPOPFLT,1
TRAP_ENTRY_POINT	trap10,T_TSSFLT,0
TRAP_ENTRY_POINT_SWAPGS	trap11,T_SEGNPFLT,0
TRAP_ENTRY_POINT_SWAPGS	trap12,T_STKFLT,0
TRAP_ENTRY_POINT_SWAPGS	trap13,T_PROTFLT,0
TRAP_ENTRY_POINT	trap14,T_PAGEFLT,0

IDTVEC(trap15)
	ZTRAP_NJ(T_ASTFLT)
	INTRENTRY
#ifdef DIAGNOSTIC
	movl	CPUVAR(ILEVEL),%ebx
#endif
	jmp	.Lalltraps_checkusr
IDTVEC_END(trap15)

IDTVEC(trap16)
	ZTRAP_NJ(T_ARITHTRAP)
.Ldo_fputrap:
	INTRENTRY
#ifdef DIAGNOSTIC
	movl	CPUVAR(ILEVEL),%ebx
#endif
	movq	%rsp,%rdi
	call	_C_LABEL(fputrap)
	jmp	.Lalltraps_checkusr
IDTVEC_END(trap16)

TRAP_ENTRY_POINT	trap17,T_ALIGNFLT,0
TRAP_ENTRY_POINT	trap18,T_MCA,1

IDTVEC(trap19)
	ZTRAP_NJ(T_XMM)
	jmp	.Ldo_fputrap
IDTVEC_END(trap19)

TRAP_ENTRY_POINT	trap20,T_RESERVED,1
TRAP_ENTRY_POINT	trap21,T_RESERVED,1
TRAP_ENTRY_POINT	trap22,T_RESERVED,1
TRAP_ENTRY_POINT	trap23,T_RESERVED,1
TRAP_ENTRY_POINT	trap24,T_RESERVED,1
TRAP_ENTRY_POINT	trap25,T_RESERVED,1
TRAP_ENTRY_POINT	trap26,T_RESERVED,1
TRAP_ENTRY_POINT	trap27,T_RESERVED,1
TRAP_ENTRY_POINT	trap28,T_RESERVED,1
TRAP_ENTRY_POINT	trap29,T_RESERVED,1
TRAP_ENTRY_POINT	trap30,T_RESERVED,1
TRAP_ENTRY_POINT	trap31,T_RESERVED,1

IDTVEC(intrspurious)
	ZTRAP_NJ(T_ASTFLT)
	INTRENTRY
#ifdef DIAGNOSTIC
	movl	CPUVAR(ILEVEL),%ebx
#endif
	jmp	.Lalltraps_checkusr
IDTVEC_END(intrspurious)

#ifndef check_swapgs
/*
 * We need to worry about traps in kernel mode while the kernel %gs isn't
 * loaded. These are either faults on iretq during return to user or loads to
 * %gs.
 *
 * When such traps happen, we have CPL=0 and %gs=userland, and we must perform
 * an additional swapgs to get %gs=kernel.
 */
NENTRY(check_swapgs)
	INTRENTRY_L(3f,1:)
2:
	sti
	jmp	calltrap
3:
	/*
	 * Trap in kernel mode.
	 */

	/* Case 1: fault on sysretq? */
	leaq	do_sysret(%rip),%rdi
	cmpq	%rdi,TF_RIP(%rsp)
	je	1b

	/* Case 2: fault on iretq? */
	leaq	do_iret(%rip),%rdi
	cmpq	%rdi,TF_RIP(%rsp)
	jne	5f
	movq	TF_RSP(%rsp),%rax	/* Must read %rsp, may be a pad word */
	testb	$SEL_UPL,8(%rax)	/* Check %cs of outer iret frame */
	je	2b			/* jump if iret was to kernel  */
	jmp	1b			/* to user - must restore %gs */
5:

	/* Case 3: move to %gs? */
	movq	TF_RIP(%rsp),%rax
	movw	(%rax),%ax
	andb	$070,%ah		/* mask mod/rm from mod/reg/rm */
	cmpw	$0x8e+050*256,%ax	/* Any move to %gs (reg 5) */
	jne	2b			/* No - normal kernel fault */
	jmp	1b			/* Yes - restore %gs */
END(check_swapgs)
#endif

	TEXT_USER_END

/*
 * trap() calls here when it detects a fault in INTRFASTEXIT (loading the
 * segment registers or during the iret itself). The address of the (possibly
 * reconstructed) user trap frame is passed as an argument.
 *
 * Typically the code will have raised a SIGSEGV which will be actioned
 * by the code below.
 */
	.type	_C_LABEL(trap_return_fault_return), @function
LABEL(trap_return_fault_return)
	mov	%rdi,%rsp		/* frame for user return */
#ifdef DIAGNOSTIC
	/* We can't recover the saved %rbx, so suppress warning */
	movl	CPUVAR(ILEVEL),%ebx
#endif
	jmp	.Lalltraps_checkusr
END(trap_return_fault_return)

/*
 * All traps go through here. Call the generic trap handler, and
 * check for ASTs afterwards.
 */
NENTRY(alltraps)
	INTRENTRY
.Lalltraps_noentry:
	STI(si)

calltrap:
#ifdef DIAGNOSTIC
	movl	CPUVAR(ILEVEL),%ebx
#endif
	movq	%rsp,%rdi
	incq	CPUVAR(NTRAP)
	call	_C_LABEL(trap)

.Lalltraps_checkusr:
	testb	$SEL_RPL,TF_CS(%rsp)
	jz	6f

.Lalltraps_checkast:
	movq	CPUVAR(CURLWP),%r14
	/* Check for ASTs on exit to user mode. */
	CLI(si)
	CHECK_ASTPENDING(%r14)
	je	3f
	CLEAR_ASTPENDING(%r14)
	STI(si)
	movl	$T_ASTFLT,TF_TRAPNO(%rsp)
	movq	%rsp,%rdi
	incq	CPUVAR(NTRAP)
	call	_C_LABEL(trap)
	jmp	.Lalltraps_checkast	/* re-check ASTs */
3:	CHECK_DEFERRED_SWITCH
	jnz	9f

6:
#ifdef DIAGNOSTIC
	cmpl	CPUVAR(ILEVEL),%ebx
	jne	.Lspl_error
#endif
	INTRFASTEXIT

9:	STI(si)
	call	_C_LABEL(do_pmap_load)
	jmp	.Lalltraps_checkast	/* re-check ASTs */

#ifdef DIAGNOSTIC
.Lspl_error:
	STI(si)
	movabsq	$4f,%rdi
	movl	CPUVAR(ILEVEL),%esi
	movl	%ebx,%edx
	xorq	%rax,%rax
	call	_C_LABEL(printf)
	movl	%ebx,%edi
	call	_C_LABEL(spllower)
	jmp	.Lalltraps_checkast
4:	.asciz	"WARNING: SPL NOT LOWERED ON TRAP EXIT %x %x\n"
#endif
END(alltraps)

#ifdef KDTRACE_HOOKS
	.bss
	.globl	dtrace_invop_jump_addr
	.align	8
	.type	dtrace_invop_jump_addr, @object
	.size	dtrace_invop_jump_addr, 8
dtrace_invop_jump_addr:
	.zero	8
	.globl	dtrace_invop_calltrap_addr
	.align	8
	.type	dtrace_invop_calltrap_addr, @object
	.size	dtrace_invop_calltrap_addr, 8
dtrace_invop_calltrap_addr:
	.zero	8
#endif

	.section .rodata

LABEL(x86_exceptions)
	.quad	_C_LABEL(Xtrap00), _C_LABEL(Xtrap01)
	.quad	_C_LABEL(Xtrap02), _C_LABEL(Xtrap03)
	.quad	_C_LABEL(Xtrap04), _C_LABEL(Xtrap05)
	.quad	_C_LABEL(Xtrap06), _C_LABEL(Xtrap07)
	.quad	_C_LABEL(Xtrap08), _C_LABEL(Xtrap09)
	.quad	_C_LABEL(Xtrap10), _C_LABEL(Xtrap11)
	.quad	_C_LABEL(Xtrap12), _C_LABEL(Xtrap13)
	.quad	_C_LABEL(Xtrap14), _C_LABEL(Xtrap15)
	.quad	_C_LABEL(Xtrap16), _C_LABEL(Xtrap17)
	.quad	_C_LABEL(Xtrap18), _C_LABEL(Xtrap19)
	.quad	_C_LABEL(Xtrap20), _C_LABEL(Xtrap21)
	.quad	_C_LABEL(Xtrap22), _C_LABEL(Xtrap23)
	.quad	_C_LABEL(Xtrap24), _C_LABEL(Xtrap25)
	.quad	_C_LABEL(Xtrap26), _C_LABEL(Xtrap27)
	.quad	_C_LABEL(Xtrap28), _C_LABEL(Xtrap29)
	.quad	_C_LABEL(Xtrap30), _C_LABEL(Xtrap31)
END(x86_exceptions)

