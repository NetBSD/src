/*	$NetBSD: vector.S,v 1.21 2003/03/06 22:17:44 fvdl Exp $	*/

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

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include "opt_ddb.h"
#include "opt_multiprocessor.h"

#define ALIGN_TEXT	.align 16,0x90

#include <machine/i8259.h>
#include <machine/i82093reg.h>
#include <machine/i82489reg.h>
#include <machine/asm.h>
#include <machine/frameasm.h>
#include <machine/segments.h>
#include <machine/trap.h>
#include <machine/intr.h>
#include <machine/psl.h>

#include <net/netisr.h>

#include "ioapic.h"
#include "lapic.h"
#include "assym.h"

/*****************************************************************************/

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

#define	TRAP(a)		pushq $(a) ; jmp _C_LABEL(alltraps)
#define	ZTRAP(a)	pushq $0 ; TRAP(a)

#define	BPTTRAP(a)	ZTRAP(a)

	.text
IDTVEC(trap00)
	ZTRAP(T_DIVIDE)
IDTVEC(trap01)
	BPTTRAP(T_TRCTRAP)
IDTVEC(trap02)
	ZTRAP(T_NMI)
IDTVEC(trap03)
	BPTTRAP(T_BPTFLT)
IDTVEC(trap04)
	ZTRAP(T_OFLOW)
IDTVEC(trap05)
	ZTRAP(T_BOUND)
IDTVEC(trap06)
	ZTRAP(T_PRIVINFLT)
IDTVEC(trap07)
	pushq	$0			# dummy error code
	pushq	$T_DNA
	INTRENTRY
	sti
	movq	CPUVAR(SELF),%rdi
	call	_C_LABEL(fpudna)
	INTRFASTEXIT
IDTVEC(trap08)
	ZTRAP(T_DOUBLEFLT)
IDTVEC(trap09)
	ZTRAP(T_FPOPFLT)
IDTVEC(trap0a)
	TRAP(T_TSSFLT)
IDTVEC(trap0b)
	TRAP(T_SEGNPFLT)
IDTVEC(trap0c)
	TRAP(T_STKFLT)
IDTVEC(trap0d)
	TRAP(T_PROTFLT)
IDTVEC(trap0e)
	TRAP(T_PAGEFLT)
IDTVEC(intrspurious)
IDTVEC(trap0f)
	iretq
IDTVEC(trap10)
	ZTRAP(T_ARITHTRAP)
IDTVEC(trap11)
	ZTRAP(T_ALIGNFLT)
IDTVEC(trap12)
	ZTRAP(T_MCA)
IDTVEC(trap13)
	ZTRAP(T_XMM)
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

/*
 * If an error is detected during trap, syscall, or interrupt exit, trap() will
 * change %eip to point to one of these labels.  We clean up the stack, if
 * necessary, and resume as if we were handling a general protection fault.
 * This will cause the process to get a SIGBUS.
 *
 * XXXfvdl currently unused, as pop %ds and pop %es are illegal in long
 * mode. However, if the x86-64 port is going to support USER_LDT, we
 * may need something like this after all.
 */
NENTRY(resume_iret)
	ZTRAP(T_PROTFLT)
#if 0
NENTRY(resume_pop_ds)
	movl	$GSEL(GDATA_SEL, SEL_KPL),%eax
	movl	%eax,%es
NENTRY(resume_pop_es)
	movl	$T_PROTFLT,TF_TRAPNO(%rsp)
	jmp	calltrap
#endif

/*
 * All traps go through here. Call the generic trap handler, and
 * check for ASTs afterwards.
 */
NENTRY(alltraps)
	INTRENTRY
	sti
calltrap:
#ifdef DIAGNOSTIC
	movl	CPUVAR(ILEVEL),%ebx
#endif /* DIAGNOSTIC */
	call	_C_LABEL(trap)
2:	/* Check for ASTs on exit to user mode. */
	cli
	CHECK_ASTPENDING(%r11)
	je	1f
	testb	$SEL_RPL,TF_CS(%rsp)
	jz	1f
5:	CLEAR_ASTPENDING(%r11)
	sti
	movl	$T_ASTFLT,TF_TRAPNO(%rsp)
	call	_C_LABEL(trap)
	jmp	2b
#ifndef DIAGNOSTIC
1:	INTRFASTEXIT
#else /* DIAGNOSTIC */
1:	cmpl	CPUVAR(ILEVEL),%ebx
	jne	3f
	INTRFASTEXIT
3:	sti
	movabsq	$4f,%rdi
	movl	CPUVAR(ILEVEL),%esi
	movl	%ebx,%edx
	xorq	%rax,%rax
	call	_C_LABEL(printf)
#ifdef DDB
	int	$3
#endif /* DDB */
	movl	%ebx,CPUVAR(ILEVEL)
	jmp	2b
4:	.asciz	"WARNING: SPL NOT LOWERED ON TRAP EXIT %x %x\n"
#endif /* DIAGNOSTIC */


#define __HAVE_GENERIC_SOFT_INTERRUPTS	/* XXX */


/*
 * Macros for interrupt entry, call to handler, and exit.
 *
 * XXX
 * The interrupt frame is set up to look like a trap frame.  This may be a
 * waste.  The only handler which needs a frame is the clock handler, and it
 * only needs a few bits.  Xdoreti() needs a trap frame for handling ASTs, but
 * it could easily convert the frame on demand.
 *
 * The direct costs of setting up a trap frame are two pushq's (error code and
 * trap number), an addl to get rid of these, and pushing and popping the
 * callee-saved registers %esi, %edi, %ebx, and %ebp twice.
 *
 * If the interrupt frame is made more flexible,  INTR can push %eax first and
 * decide the ipending case with less overhead, e.g., by avoiding loading the
 * segment registers.
 *
 */

#define MY_COUNT _C_LABEL(uvmexp)

/* XXX See comment in locore.s */
#ifdef __ELF__
#define	XINTR(name,num)		Xintr_/**/name/**/num
#else
#define	XINTR(name,num)		_Xintr_/**/name/**/num
#endif

#if NLAPIC > 0
#ifdef MULTIPROCESSOR
IDTVEC(recurse_lapic_ipi)
	INTR_RECURSE_HWFRAME
	pushq	$0		
	pushq	$T_ASTFLT
	INTRENTRY		
IDTVEC(resume_lapic_ipi)
	cli
	jmp	1f
IDTVEC(intr_lapic_ipi)
	pushq	$0		
	pushq	$T_ASTFLT
	INTRENTRY		
	movl	$0,_C_LABEL(local_apic)+LAPIC_EOI
	movl	CPUVAR(ILEVEL),%ebx
	cmpl	$IPL_IPI,%ebx
	jae	2f
1:
	incl	CPUVAR(IDEPTH)
	movl	$IPL_IPI,CPUVAR(ILEVEL)
        sti
	pushq	%rbx
	call	_C_LABEL(x86_ipi_handler)
	jmp	_C_LABEL(Xdoreti)
2:
	orl	$(1 << LIR_IPI),CPUVAR(IPENDING)
	sti
	INTRFASTEXIT

#if defined(DDB)
IDTVEC(intrddb)
1:
	pushq	$0
	pushq	$T_BPTFLT
	INTRENTRY
	movl	$0xf,%eax
	movq	%rax,%cr8
	movl	$0,_C_LABEL(local_apic)+LAPIC_EOI
	sti
	call	_C_LABEL(ddb_ipi)
	xorl	%eax,%eax
	movq	%rax,%cr8
	INTRFASTEXIT
#endif /* DDB */
#endif /* MULTIPROCESSOR */
	
	/*
	 * Interrupt from the local APIC timer.
	 */
IDTVEC(recurse_lapic_ltimer)
	INTR_RECURSE_HWFRAME
	pushq	$0		
	pushq	$T_ASTFLT
	INTRENTRY		
IDTVEC(resume_lapic_ltimer)
	cli
	jmp	1f
IDTVEC(intr_lapic_ltimer)
	pushq	$0		
	pushq	$T_ASTFLT
	INTRENTRY		
	movl	$0,_C_LABEL(local_apic)+LAPIC_EOI
	movl	CPUVAR(ILEVEL),%ebx
	cmpl	$IPL_CLOCK,%ebx
	jae	2f
1:
	incl	CPUVAR(IDEPTH)
	movl	$IPL_CLOCK,CPUVAR(ILEVEL)
	sti
	pushq	%rbx
	xorq	%rdi,%rdi
	call	_C_LABEL(lapic_clockintr)
	jmp	_C_LABEL(Xdoreti)
2:
	orl	$(1 << LIR_TIMER),CPUVAR(IPENDING)
	sti
	INTRFASTEXIT
#endif /* NLAPIC > 0 */

#ifdef MULTIPROCESSOR
#define LOCK_KERNEL	call _C_LABEL(x86_intlock)
#define UNLOCK_KERNEL	call _C_LABEL(x86_intunlock)
#else
#define LOCK_KERNEL
#define UNLOCK_KERNEL
#endif

#define voidop(num)


/*
 * This macro defines the generic stub code. Its arguments modifiy it
 * for specific PICs.
 */

#define	INTRSTUB(name, num, early_ack, late_ack, mask, unmask, level_check) \
IDTVEC(recurse_/**/name/**/num)						;\
	INTR_RECURSE_HWFRAME						;\
	pushq	$0			/* dummy error code */		;\
	pushq	$T_ASTFLT		/* trap # for doing ASTs */	;\
	INTRENTRY							;\
IDTVEC(resume_/**/name/**/num)						\
	movl	%ebx,%r13d						;\
	movq	CPUVAR(ISOURCES) + (num) * 8, %r14			;\
	movl	IS_MAXLEVEL(%r14),%ebx					;\
	jmp	1f							;\
IDTVEC(intr_/**/name/**/num)						;\
	pushq	$0			/* dummy error code */		;\
	pushq	$T_ASTFLT		/* trap # for doing ASTs */	;\
	INTRENTRY							;\
	movq	CPUVAR(ISOURCES) + (num) * 8, %r14		;\
	mask(num)		/* mask it in hardware */	;\
	early_ack(num)			/* and allow other intrs */	;\
	testq	%r14,%r14						;\
	jz	9f			/* stray */			;\
	movl	IS_MAXLEVEL(%r14),%ebx					;\
	movl	CPUVAR(ILEVEL),%r13d					;\
	cmpl	%ebx,%r13d						;\
	jae	10f			/* currently masked; hold it */	;\
	incl	MY_COUNT+V_INTR		/* statistical info */		;\
	incq	IS_EVCNT(%r14)						;\
1:									\
	pushq	%r13							;\
	movl	%ebx,CPUVAR(ILEVEL)					;\
	sti								;\
	incl	CPUVAR(IDEPTH)						;\
	movq	IS_HANDLERS(%r14),%rbx					;\
	LOCK_KERNEL							;\
6:									\
	movl	IH_LEVEL(%rbx),%r12d					;\
	cmpl	%r13d,%r12d						;\
	jle	7f							;\
	movq	IH_ARG(%rbx),%rdi					;\
	movl	%r12d,CPUVAR(ILEVEL)					;\
	call	*IH_FUN(%rbx)		/* call it */			;\
	level_check(num)						;\
	movq	IH_NEXT(%rbx),%rbx	/* next handler in chain */	;\
	testq	%rbx,%rbx						;\
	jnz	6b							;\
5:									\
	UNLOCK_KERNEL							;\
	unmask(num)			/* unmask it in hardware */	;\
	late_ack(num)							;\
	sti								;\
	jmp	_C_LABEL(Xdoreti)	/* lower spl and do ASTs */	;\
7:									\
	UNLOCK_KERNEL							;\
	orl     $(1 << num),CPUVAR(IPENDING)				;\
	late_ack(num)							;\
	sti								;\
	jmp	_C_LABEL(Xdoreti)	/* lower spl and do ASTs */	;\
10:									\
	orl     $(1 << num),CPUVAR(IPENDING)				;\
	late_ack(num)							;\
	sti								;\
	INTRFASTEXIT							;\
9:									\
	unmask(num)							;\
	late_ack(num)							;\
	sti								;\
	INTRFASTEXIT

#define ICUADDR IO_ICU1

INTRSTUB(legacy,0,i8259_asm_ack1,voidop,i8259_asm_mask,i8259_asm_unmask,
    voidop)
INTRSTUB(legacy,1,i8259_asm_ack1,voidop,i8259_asm_mask,i8259_asm_unmask,
    voidop)
INTRSTUB(legacy,2,i8259_asm_ack1,voidop,i8259_asm_mask,i8259_asm_unmask,
    voidop)
INTRSTUB(legacy,3,i8259_asm_ack1,voidop,i8259_asm_mask,i8259_asm_unmask,
    voidop)
INTRSTUB(legacy,4,i8259_asm_ack1,voidop,i8259_asm_mask,i8259_asm_unmask,
    voidop)
INTRSTUB(legacy,5,i8259_asm_ack1,voidop,i8259_asm_mask,i8259_asm_unmask,
    voidop)
INTRSTUB(legacy,6,i8259_asm_ack1,voidop,i8259_asm_mask,i8259_asm_unmask,
    voidop)
INTRSTUB(legacy,7,i8259_asm_ack1,voidop,i8259_asm_mask,i8259_asm_unmask,
    voidop)
#undef ICUADDR
#define ICUADDR IO_ICU2

INTRSTUB(legacy,8,i8259_asm_ack2,voidop,i8259_asm_mask,i8259_asm_unmask,
    voidop)
INTRSTUB(legacy,9,i8259_asm_ack2,voidop,i8259_asm_mask,i8259_asm_unmask,
    voidop)
INTRSTUB(legacy,10,i8259_asm_ack2,voidop,i8259_asm_mask,i8259_asm_unmask,
    voidop)
INTRSTUB(legacy,11,i8259_asm_ack2,voidop,i8259_asm_mask,i8259_asm_unmask,
    voidop)
INTRSTUB(legacy,12,i8259_asm_ack2,voidop,i8259_asm_mask,i8259_asm_unmask,
    voidop)
INTRSTUB(legacy,13,i8259_asm_ack2,voidop,i8259_asm_mask,i8259_asm_unmask,
    voidop)
INTRSTUB(legacy,14,i8259_asm_ack2,voidop,i8259_asm_mask,i8259_asm_unmask,
    voidop)
INTRSTUB(legacy,15,i8259_asm_ack2,voidop,i8259_asm_mask,i8259_asm_unmask,
    voidop)

#if NIOAPIC > 0
INTRSTUB(ioapic,0,ioapic_asm_ack,voidop,voidop,voidop,voidop)
INTRSTUB(ioapic,1,ioapic_asm_ack,voidop,voidop,voidop,voidop)
INTRSTUB(ioapic,2,ioapic_asm_ack,voidop,voidop,voidop,voidop)
INTRSTUB(ioapic,3,ioapic_asm_ack,voidop,voidop,voidop,voidop)
INTRSTUB(ioapic,4,ioapic_asm_ack,voidop,voidop,voidop,voidop)
INTRSTUB(ioapic,5,ioapic_asm_ack,voidop,voidop,voidop,voidop)
INTRSTUB(ioapic,6,ioapic_asm_ack,voidop,voidop,voidop,voidop)
INTRSTUB(ioapic,7,ioapic_asm_ack,voidop,voidop,voidop,voidop)
INTRSTUB(ioapic,8,ioapic_asm_ack,voidop,voidop,voidop,voidop)
INTRSTUB(ioapic,9,ioapic_asm_ack,voidop,voidop,voidop,voidop)
INTRSTUB(ioapic,10,ioapic_asm_ack,voidop,voidop,voidop,voidop)
INTRSTUB(ioapic,11,ioapic_asm_ack,voidop,voidop,voidop,voidop)
INTRSTUB(ioapic,12,ioapic_asm_ack,voidop,voidop,voidop,voidop)
INTRSTUB(ioapic,13,ioapic_asm_ack,voidop,voidop,voidop,voidop)
INTRSTUB(ioapic,14,ioapic_asm_ack,voidop,voidop,voidop,voidop)
INTRSTUB(ioapic,15,ioapic_asm_ack,voidop,voidop,voidop,voidop)
INTRSTUB(ioapic,16,ioapic_asm_ack,voidop,voidop,voidop,voidop)
INTRSTUB(ioapic,17,ioapic_asm_ack,voidop,voidop,voidop,voidop)
INTRSTUB(ioapic,18,ioapic_asm_ack,voidop,voidop,voidop,voidop)
INTRSTUB(ioapic,19,ioapic_asm_ack,voidop,voidop,voidop,voidop)
INTRSTUB(ioapic,20,ioapic_asm_ack,voidop,voidop,voidop,voidop)
INTRSTUB(ioapic,21,ioapic_asm_ack,voidop,voidop,voidop,voidop)
INTRSTUB(ioapic,22,ioapic_asm_ack,voidop,voidop,voidop,voidop)
INTRSTUB(ioapic,23,ioapic_asm_ack,voidop,voidop,voidop,voidop)
INTRSTUB(ioapic,24,ioapic_asm_ack,voidop,voidop,voidop,voidop)
INTRSTUB(ioapic,25,ioapic_asm_ack,voidop,voidop,voidop,voidop)
INTRSTUB(ioapic,26,ioapic_asm_ack,voidop,voidop,voidop,voidop)
INTRSTUB(ioapic,27,ioapic_asm_ack,voidop,voidop,voidop,voidop)
INTRSTUB(ioapic,28,ioapic_asm_ack,voidop,voidop,voidop,voidop)
INTRSTUB(ioapic,29,ioapic_asm_ack,voidop,voidop,voidop,voidop)
INTRSTUB(ioapic,30,ioapic_asm_ack,voidop,voidop,voidop,voidop)
INTRSTUB(ioapic,31,ioapic_asm_ack,voidop,voidop,voidop,voidop)
#endif

.globl _C_LABEL(i8259_stubs)
_C_LABEL(i8259_stubs):
	.quad _C_LABEL(Xintr_legacy0), _C_LABEL(Xrecurse_legacy0)
	.quad _C_LABEL(Xresume_legacy0)
	.quad _C_LABEL(Xintr_legacy1), _C_LABEL(Xrecurse_legacy1)
	.quad _C_LABEL(Xresume_legacy1)
	.quad _C_LABEL(Xintr_legacy2), _C_LABEL(Xrecurse_legacy2)
	.quad _C_LABEL(Xresume_legacy2)
	.quad _C_LABEL(Xintr_legacy3), _C_LABEL(Xrecurse_legacy3)
	.quad _C_LABEL(Xresume_legacy3)
	.quad _C_LABEL(Xintr_legacy4), _C_LABEL(Xrecurse_legacy4)
	.quad _C_LABEL(Xresume_legacy4)
	.quad _C_LABEL(Xintr_legacy5), _C_LABEL(Xrecurse_legacy5)
	.quad _C_LABEL(Xresume_legacy5)
	.quad _C_LABEL(Xintr_legacy6), _C_LABEL(Xrecurse_legacy6)
	.quad _C_LABEL(Xresume_legacy6)
	.quad _C_LABEL(Xintr_legacy7), _C_LABEL(Xrecurse_legacy7)
	.quad _C_LABEL(Xresume_legacy7)
	.quad _C_LABEL(Xintr_legacy8), _C_LABEL(Xrecurse_legacy8)
	.quad _C_LABEL(Xresume_legacy8)
	.quad _C_LABEL(Xintr_legacy9), _C_LABEL(Xrecurse_legacy9)
	.quad _C_LABEL(Xresume_legacy9)
	.quad _C_LABEL(Xintr_legacy10), _C_LABEL(Xrecurse_legacy10)
	.quad _C_LABEL(Xresume_legacy10)
	.quad _C_LABEL(Xintr_legacy11), _C_LABEL(Xrecurse_legacy11)
	.quad _C_LABEL(Xresume_legacy11)
	.quad _C_LABEL(Xintr_legacy12), _C_LABEL(Xrecurse_legacy12)
	.quad _C_LABEL(Xresume_legacy12)
	.quad _C_LABEL(Xintr_legacy13), _C_LABEL(Xrecurse_legacy13)
	.quad _C_LABEL(Xresume_legacy13)
	.quad _C_LABEL(Xintr_legacy14), _C_LABEL(Xrecurse_legacy14)
	.quad _C_LABEL(Xresume_legacy14)
	.quad _C_LABEL(Xintr_legacy15), _C_LABEL(Xrecurse_legacy15)
	.quad _C_LABEL(Xresume_legacy15)

#if NIOAPIC > 0
.globl _C_LABEL(ioapic_stubs)
_C_LABEL(ioapic_stubs):
	.quad _C_LABEL(Xintr_ioapic0), _C_LABEL(Xrecurse_ioapic0)
	.quad _C_LABEL(Xresume_ioapic0)
	.quad _C_LABEL(Xintr_ioapic1), _C_LABEL(Xrecurse_ioapic1)
	.quad _C_LABEL(Xresume_ioapic1)
	.quad _C_LABEL(Xintr_ioapic2), _C_LABEL(Xrecurse_ioapic2)
	.quad _C_LABEL(Xresume_ioapic2)
	.quad _C_LABEL(Xintr_ioapic3), _C_LABEL(Xrecurse_ioapic3)
	.quad _C_LABEL(Xresume_ioapic3)
	.quad _C_LABEL(Xintr_ioapic4), _C_LABEL(Xrecurse_ioapic4)
	.quad _C_LABEL(Xresume_ioapic4)
	.quad _C_LABEL(Xintr_ioapic5), _C_LABEL(Xrecurse_ioapic5)
	.quad _C_LABEL(Xresume_ioapic5)
	.quad _C_LABEL(Xintr_ioapic6), _C_LABEL(Xrecurse_ioapic6)
	.quad _C_LABEL(Xresume_ioapic6)
	.quad _C_LABEL(Xintr_ioapic7), _C_LABEL(Xrecurse_ioapic7)
	.quad _C_LABEL(Xresume_ioapic7)
	.quad _C_LABEL(Xintr_ioapic8), _C_LABEL(Xrecurse_ioapic8)
	.quad _C_LABEL(Xresume_ioapic8)
	.quad _C_LABEL(Xintr_ioapic9), _C_LABEL(Xrecurse_ioapic9)
	.quad _C_LABEL(Xresume_ioapic9)
	.quad _C_LABEL(Xintr_ioapic10), _C_LABEL(Xrecurse_ioapic10)
	.quad _C_LABEL(Xresume_ioapic10)
	.quad _C_LABEL(Xintr_ioapic11), _C_LABEL(Xrecurse_ioapic11)
	.quad _C_LABEL(Xresume_ioapic11)
	.quad _C_LABEL(Xintr_ioapic12), _C_LABEL(Xrecurse_ioapic12)
	.quad _C_LABEL(Xresume_ioapic12)
	.quad _C_LABEL(Xintr_ioapic13), _C_LABEL(Xrecurse_ioapic13)
	.quad _C_LABEL(Xresume_ioapic13)
	.quad _C_LABEL(Xintr_ioapic14), _C_LABEL(Xrecurse_ioapic14)
	.quad _C_LABEL(Xresume_ioapic14)
	.quad _C_LABEL(Xintr_ioapic15), _C_LABEL(Xrecurse_ioapic15)
	.quad _C_LABEL(Xresume_ioapic15)
	.quad _C_LABEL(Xintr_ioapic16), _C_LABEL(Xrecurse_ioapic16)
	.quad _C_LABEL(Xresume_ioapic16)
	.quad _C_LABEL(Xintr_ioapic17), _C_LABEL(Xrecurse_ioapic17)
	.quad _C_LABEL(Xresume_ioapic17)
	.quad _C_LABEL(Xintr_ioapic18), _C_LABEL(Xrecurse_ioapic18)
	.quad _C_LABEL(Xresume_ioapic18)
	.quad _C_LABEL(Xintr_ioapic19), _C_LABEL(Xrecurse_ioapic19)
	.quad _C_LABEL(Xresume_ioapic19)
	.quad _C_LABEL(Xintr_ioapic20), _C_LABEL(Xrecurse_ioapic20)
	.quad _C_LABEL(Xresume_ioapic20)
	.quad _C_LABEL(Xintr_ioapic21), _C_LABEL(Xrecurse_ioapic21)
	.quad _C_LABEL(Xresume_ioapic21)
	.quad _C_LABEL(Xintr_ioapic22), _C_LABEL(Xrecurse_ioapic22)
	.quad _C_LABEL(Xresume_ioapic22)
	.quad _C_LABEL(Xintr_ioapic23), _C_LABEL(Xrecurse_ioapic23)
	.quad _C_LABEL(Xresume_ioapic23)
	.quad _C_LABEL(Xintr_ioapic24), _C_LABEL(Xrecurse_ioapic24)
	.quad _C_LABEL(Xresume_ioapic24)
	.quad _C_LABEL(Xintr_ioapic25), _C_LABEL(Xrecurse_ioapic25)
	.quad _C_LABEL(Xresume_ioapic25)
	.quad _C_LABEL(Xintr_ioapic26), _C_LABEL(Xrecurse_ioapic26)
	.quad _C_LABEL(Xresume_ioapic26)
	.quad _C_LABEL(Xintr_ioapic27), _C_LABEL(Xrecurse_ioapic27)
	.quad _C_LABEL(Xresume_ioapic27)
	.quad _C_LABEL(Xintr_ioapic28), _C_LABEL(Xrecurse_ioapic28)
	.quad _C_LABEL(Xresume_ioapic28)
	.quad _C_LABEL(Xintr_ioapic29), _C_LABEL(Xrecurse_ioapic29)
	.quad _C_LABEL(Xresume_ioapic29)
	.quad _C_LABEL(Xintr_ioapic30), _C_LABEL(Xrecurse_ioapic30)
	.quad _C_LABEL(Xresume_ioapic30)
	.quad _C_LABEL(Xintr_ioapic31), _C_LABEL(Xrecurse_ioapic31)
	.quad _C_LABEL(Xresume_ioapic31)
#endif

/*
 * Symbols that vmstat -i wants, even though they're not used.
 */
.globl	_C_LABEL(intrnames)
_C_LABEL(intrnames):
.globl	_C_LABEL(eintrnames)
_C_LABEL(eintrnames):

.globl	_C_LABEL(intrcnt)
_C_LABEL(intrcnt):
.globl	_C_LABEL(eintrcnt)
_C_LABEL(eintrcnt):

/*
 * Soft interrupt handlers
 */

IDTVEC(softserial)
	movl	$IPL_SOFTSERIAL, CPUVAR(ILEVEL)
	incl	CPUVAR(IDEPTH)
#ifdef MULTIPROCESSOR
	call	_C_LABEL(x86_softintlock)
#endif
	movq	CPUVAR(ISOURCES) + SIR_SERIAL * 8, %r12
	incq	IS_EVCNT(%r12)
	movl	$X86_SOFTINTR_SOFTSERIAL,%edi
	call	_C_LABEL(softintr_dispatch)
#ifdef MULTIPROCESSOR	
	call	_C_LABEL(x86_softintunlock)
#endif
	decl	CPUVAR(IDEPTH)
	jmp	*%r13

IDTVEC(softnet)
	movl	$IPL_SOFTNET, CPUVAR(ILEVEL)
	incl	CPUVAR(IDEPTH)
#ifdef MULTIPROCESSOR	
	call	_C_LABEL(x86_softintlock)
#endif
	movq	CPUVAR(ISOURCES) + SIR_NET * 8, %r12
	incq    IS_EVCNT(%r12)

	xorq	%r12,%r12
	xchgl	_C_LABEL(netisr),%r12d

	/* XXX Do the legacy netisrs here for now. */
#define DONETISR(s, c) \
	.globl  _C_LABEL(c)	;\
	testl	$(1 << s),%r12d	;\
	jz	1f		;\
	call	_C_LABEL(c)	;\
1:
#include <net/netisr_dispatch.h>
	
	movl	$X86_SOFTINTR_SOFTNET,%edi
	call	_C_LABEL(softintr_dispatch)
#ifdef MULTIPROCESSOR	
	call	_C_LABEL(x86_softintunlock)	
#endif
	decl	CPUVAR(IDEPTH)
	jmp	*%r13

IDTVEC(softclock)
	movl	$IPL_SOFTCLOCK, CPUVAR(ILEVEL)
	incl	CPUVAR(IDEPTH)
#ifdef MULTIPROCESSOR	
	call	_C_LABEL(x86_softintlock)
#endif
	movq	CPUVAR(ISOURCES) + SIR_CLOCK * 8, %r12
	incq	IS_EVCNT(%r12)

	movl	$X86_SOFTINTR_SOFTCLOCK,%edi
	call	_C_LABEL(softintr_dispatch)
#ifdef MULTIPROCESSOR	
	call	_C_LABEL(x86_softintunlock)		
#endif
	decl	CPUVAR(IDEPTH)
	jmp	*%r13
