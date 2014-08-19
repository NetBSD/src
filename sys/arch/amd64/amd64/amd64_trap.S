/*	$NetBSD: amd64_trap.S,v 1.2.8.2 2014/08/20 00:02:42 tls Exp $	*/

/*-
 * Copyright (c) 1998, 2007, 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum and by Andrew Doran.
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

#if 0
#include <machine/asm.h>
__KERNEL_RCSID(0, "$NetBSD: amd64_trap.S,v 1.2.8.2 2014/08/20 00:02:42 tls Exp $");
#endif

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

/*****************************************************************************/

#ifdef	XEN
#define	PRE_TRAP	movq (%rsp),%rcx ; movq 8(%rsp),%r11 ; addq $0x10,%rsp 
#else
#define	PRE_TRAP
#endif

#define	TRAP_NJ(a)	PRE_TRAP ; pushq $(a)
#define	ZTRAP_NJ(a)	PRE_TRAP ; pushq $0 ; pushq $(a)
#define	TRAP(a)		TRAP_NJ(a) ; jmp _C_LABEL(alltraps)
#define	ZTRAP(a)	ZTRAP_NJ(a) ; jmp _C_LABEL(alltraps)

	.text

IDTVEC(trap00)
	ZTRAP(T_DIVIDE)
IDTVEC_END(trap00)

IDTVEC(trap01)
	ZTRAP(T_TRCTRAP)
IDTVEC_END(trap01)

IDTVEC(trap02)
#if defined(XEN)
	ZTRAP(T_NMI)
#else /* defined(XEN) */
	pushq $0
	pushq $T_NMI
	subq	$TF_REGSIZE,%rsp
	INTR_SAVE_GPRS
	movl	$MSR_GSBASE,%ecx
	rdmsr
	cmpl	$VM_MIN_KERNEL_ADDRESS_HIGH32,%edx
	jae	1f
	swapgs
	movw	%gs,TF_GS(%rsp)
	movw	%fs,TF_FS(%rsp)
	movw	%es,TF_ES(%rsp)
	movw	%ds,TF_DS(%rsp)
	movq	%rsp,%rdi
	incq	CPUVAR(NTRAP)
	call	_C_LABEL(trap)
	movw	TF_ES(%rsp),%es
	movw	TF_DS(%rsp),%ds
	swapgs
	jmp	2f
1:
	movq	%rsp,%rdi
	incq	CPUVAR(NTRAP)
	call	_C_LABEL(trap)
2:
	INTR_RESTORE_GPRS
	addq	$TF_REGSIZE+16,%rsp
	iretq
#endif /* defined(XEN) */
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
	.text
#endif
IDTVEC_END(trap03)

IDTVEC(trap04)
	ZTRAP(T_OFLOW)
IDTVEC_END(trap04)

IDTVEC(trap05)
	ZTRAP(T_BOUND)
IDTVEC_END(trap05)

IDTVEC(trap06)
	ZTRAP(T_PRIVINFLT)
IDTVEC_END(trap06)

IDTVEC(trap07)
	ZTRAP_NJ(T_DNA)
	INTRENTRY
#ifdef DIAGNOSTIC
	movl	CPUVAR(ILEVEL),%ebx
#endif /* DIAGNOSTIC */
	movq	%rsp,%rdi
	call	_C_LABEL(fpudna)
	jmp	.Lalltraps_checkusr
IDTVEC_END(trap07)

IDTVEC(trap08)
	TRAP(T_DOUBLEFLT)
IDTVEC_END(trap08)

IDTVEC(trap09)
	ZTRAP(T_FPOPFLT)
IDTVEC_END(trap09)

IDTVEC(trap0a)
	TRAP(T_TSSFLT)
IDTVEC_END(trap0a)

#ifdef XEN
/*
 * I don't believe XEN generates in-kernel traps for the
 * equivalent of iret, if it does this code would be needed
 * in order to copy the user segment registers into the fault frame.
 */
#define check_swapgs alltraps
#endif

IDTVEC(trap0b)		/* #NP() Segment not present */
	TRAP_NJ(T_SEGNPFLT)
	jmp	check_swapgs
IDTVEC_END(trap0b)		/* #NP() Segment not present */

IDTVEC(trap0c)		/* #SS() Stack exception */
	TRAP_NJ(T_STKFLT)
	jmp	check_swapgs
IDTVEC_END(trap0c)		/* #SS() Stack exception */

IDTVEC(trap0d)		/* #GP() General protection */
	TRAP_NJ(T_PROTFLT)
#ifdef check_swapgs
	jmp	check_swapgs
#else
/* We need to worry about traps while the kernel %gs_base isn't loaded.
 * These are either loads to %gs (only 32bit) or faults on iret during
 * return to user. */
check_swapgs:
	INTRENTRY_L(3f,1:)
2:	sti
	jmp	calltrap
3:
	/* Trap in kernel mode. */
	/* If faulting instruction is 'iret' we may need to do a 'swapgs'. */
	movq	TF_RIP(%rsp),%rax
	cmpw	$0xcf48,(%rax)		/* Faulting instruction is iretq ? */
	jne	5f			/* Jump if not */
	movq	TF_RSP(%rsp),%rax	/* Must read %rsp, may be a pad word */
	testb	$SEL_UPL,8(%rax)	/* Check %cs of outer iret frame */
	je	2b			/* jump if iret was to kernel  */
	jmp	1b			/* to user - must restore %gs */
5:
	/* Not 'iret', all moves to %gs also need a swapgs */
	movw	(%rax),%ax
	andb	$070,%ah		/* mask mod/rm from mod/reg/rm */
	cmpw	$0x8e+050*256,%ax	/* Any move to %gs (reg 5) */
	jne	2b			/* No - normal kernel fault */
	jmp	1b			/* Yes - restore %gs */
#endif
IDTVEC_END(trap0d)

IDTVEC(trap0e)
	TRAP(T_PAGEFLT)
IDTVEC_END(trap0e)

IDTVEC(intrspurious)
IDTVEC(trap0f)
	ZTRAP_NJ(T_ASTFLT)
	INTRENTRY
#ifdef DIAGNOSTIC
	movl	CPUVAR(ILEVEL),%ebx
#endif /* DIAGNOSTIC */
	jmp	.Lalltraps_checkusr
IDTVEC_END(trap0f)
IDTVEC_END(intrspurious)

IDTVEC(trap10)
	ZTRAP_NJ(T_ARITHTRAP)
.Ldo_fputrap:
	INTRENTRY
#ifdef DIAGNOSTIC
	movl	CPUVAR(ILEVEL),%ebx
#endif /* DIAGNOSTIC */
	movq	%rsp,%rdi
	call	_C_LABEL(fputrap)
	jmp	.Lalltraps_checkusr
IDTVEC_END(trap10)

IDTVEC(trap11)
	TRAP(T_ALIGNFLT)
IDTVEC_END(trap11)

IDTVEC(trap12)
	ZTRAP(T_MCA)
IDTVEC_END(trap12)

IDTVEC(trap13)
	ZTRAP_NJ(T_XMM)
	jmp	.Ldo_fputrap
IDTVEC_END(trap13)

IDTVEC(trap14)
IDTVEC(trap15)
IDTVEC(trap16)
IDTVEC(trap17)
IDTVEC(trap18)
IDTVEC(trap19)
IDTVEC(trap1a)
IDTVEC(trap1b)
IDTVEC(trap1c)
IDTVEC(trap1d)
IDTVEC(trap1e)
IDTVEC(trap1f)
	/* 20 - 31 reserved for future exp */
	ZTRAP(T_RESERVED)
IDTVEC_END(trap1f)
IDTVEC_END(trap1e)
IDTVEC_END(trap1d)
IDTVEC_END(trap1c)
IDTVEC_END(trap1b)
IDTVEC_END(trap1a)
IDTVEC_END(trap19)
IDTVEC_END(trap18)
IDTVEC_END(trap17)
IDTVEC_END(trap16)
IDTVEC_END(trap15)
IDTVEC_END(trap14)

IDTVEC(exceptions)
	.quad	_C_LABEL(Xtrap00), _C_LABEL(Xtrap01)
	.quad	_C_LABEL(Xtrap02), _C_LABEL(Xtrap03)
	.quad	_C_LABEL(Xtrap04), _C_LABEL(Xtrap05)
	.quad	_C_LABEL(Xtrap06), _C_LABEL(Xtrap07)
	.quad	_C_LABEL(Xtrap08), _C_LABEL(Xtrap09)
	.quad	_C_LABEL(Xtrap0a), _C_LABEL(Xtrap0b)
	.quad	_C_LABEL(Xtrap0c), _C_LABEL(Xtrap0d)
	.quad	_C_LABEL(Xtrap0e), _C_LABEL(Xtrap0f)
	.quad	_C_LABEL(Xtrap10), _C_LABEL(Xtrap11)
	.quad	_C_LABEL(Xtrap12), _C_LABEL(Xtrap13)
	.quad	_C_LABEL(Xtrap14), _C_LABEL(Xtrap15)
	.quad	_C_LABEL(Xtrap16), _C_LABEL(Xtrap17)
	.quad	_C_LABEL(Xtrap18), _C_LABEL(Xtrap19)
	.quad	_C_LABEL(Xtrap1a), _C_LABEL(Xtrap1b)
	.quad	_C_LABEL(Xtrap1c), _C_LABEL(Xtrap1d)
	.quad	_C_LABEL(Xtrap1e), _C_LABEL(Xtrap1f)
IDTVEC_END(exceptions)

/*
 * trap() calls here when it detects a fault in INTRFASTEXIT (loading the
 * segment registers or during the iret itself).
 * The address of the (possibly reconstructed) user trap frame is
 * passed as an argument.
 * Typically the code will have raised a SIGSEGV which will be actioned
 * by the code below.
 */
	.type	_C_LABEL(trap_return_fault_return), @function
LABEL(trap_return_fault_return)
	mov	%rdi,%rsp		/* frame for user return */
#ifdef DIAGNOSTIC
	/* We can't recover the saved %rbx, so suppress warning */
	movl	CPUVAR(ILEVEL),%ebx
#endif /* DIAGNOSTIC */
	jmp	.Lalltraps_checkusr
END(trap_return_fault_return)

/*
 * All traps go through here. Call the generic trap handler, and
 * check for ASTs afterwards.
 */
NENTRY(alltraps)
	INTRENTRY
  	STI(si)

calltrap:
#ifdef DIAGNOSTIC
	movl	CPUVAR(ILEVEL),%ebx
#endif /* DIAGNOSTIC */
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
#ifndef DIAGNOSTIC
6:	INTRFASTEXIT
#else /* DIAGNOSTIC */
6:	cmpl	CPUVAR(ILEVEL),%ebx
	jne	3f
	INTRFASTEXIT
3:  	STI(si)
	movabsq	$4f,%rdi
	movl	CPUVAR(ILEVEL),%esi
	movl	%ebx,%edx
	xorq	%rax,%rax
	call	_C_LABEL(printf)
	movl	%ebx,%edi
	call	_C_LABEL(spllower)
	jmp	.Lalltraps_checkast
4:	.asciz	"WARNING: SPL NOT LOWERED ON TRAP EXIT %x %x\n"
#endif /* DIAGNOSTIC */
9:	STI(si)
	call	_C_LABEL(do_pmap_load)
	jmp	.Lalltraps_checkast	/* re-check ASTs */
END(alltraps)
