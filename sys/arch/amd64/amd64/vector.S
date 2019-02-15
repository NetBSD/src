/*	$NetBSD: vector.S,v 1.69 2019/02/15 08:54:01 nonaka Exp $	*/

/*
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

#include <machine/asm.h>

#include "opt_ddb.h"
#include "opt_multiprocessor.h"
#include "opt_xen.h"
#include "opt_dtrace.h"

#define ALIGN_TEXT	.align 16,0x90

#include <machine/i8259.h>
#include <machine/i82093reg.h>
#include <machine/i82489reg.h>
#include <machine/frameasm.h>
#include <machine/segments.h>
#include <machine/trap.h>
#include <machine/specialreg.h>

#include "ioapic.h"
#include "lapic.h"
#include "assym.h"
#ifndef XEN
#include "hyperv.h"
#endif

	.text

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
 */

#if NLAPIC > 0
#ifdef MULTIPROCESSOR
IDTVEC(recurse_lapic_ipi)
	INTR_RECURSE_HWFRAME
	pushq	$0
	pushq	$T_ASTFLT
	INTR_RECURSE_ENTRY
	jmp	1f
IDTVEC_END(recurse_lapic_ipi)
IDTVEC(handle_x2apic_ipi)
	movl	$(MSR_X2APIC_BASE + MSR_X2APIC_EOI),%ecx
	xorl	%eax,%eax
	xorl	%edx,%edx
	wrmsr
	movl	CPUVAR(ILEVEL),%ebx
	cmpl	$IPL_HIGH,%ebx
	jae	2f
	jmp	1f
IDTVEC_END(handle_x2apic_ipi)
IDTVEC(handle_lapic_ipi)
	movq	_C_LABEL(local_apic_va),%rbx
	movl	$0,LAPIC_EOI(%rbx)
	movl	CPUVAR(ILEVEL),%ebx
	cmpl	$IPL_HIGH,%ebx
	jae	2f
	jmp	1f
IDTVEC_END(handle_lapic_ipi)
IDTVEC(resume_lapic_ipi)
1:
	incl	CPUVAR(IDEPTH)
	movl	$IPL_HIGH,CPUVAR(ILEVEL)
	sti
	pushq	%rbx
	call	_C_LABEL(x86_ipi_handler)
	jmp	_C_LABEL(Xdoreti)
2:
	orl	$(1 << LIR_IPI),CPUVAR(IPENDING)
	INTRFASTEXIT
IDTVEC_END(resume_lapic_ipi)

	TEXT_USER_BEGIN
IDTVEC(intr_x2apic_ipi)
	pushq	$0
	pushq	$T_ASTFLT
	INTRENTRY
	jmp	_C_LABEL(Xhandle_x2apic_ipi)
IDTVEC_END(intr_x2apic_ipi)
IDTVEC(intr_lapic_ipi)
	pushq	$0
	pushq	$T_ASTFLT
	INTRENTRY
	jmp	_C_LABEL(Xhandle_lapic_ipi)
IDTVEC_END(intr_lapic_ipi)
	TEXT_USER_END

#if defined(DDB)
IDTVEC(handle_ddbipi)
	movl	$0xf,%eax
	movq	%rax,%cr8
	movq	_C_LABEL(local_apic_va),%rbx
	movl	$0,LAPIC_EOI(%rbx)
	sti
	call	_C_LABEL(ddb_ipi)
	xorl	%eax,%eax
	movq	%rax,%cr8
	INTRFASTEXIT
IDTVEC_END(handle_ddbipi)
IDTVEC(handle_x2apic_ddbipi)
	movl	$0xf,%eax
	movq	%rax,%cr8
	movl	$(MSR_X2APIC_BASE + MSR_X2APIC_EOI),%ecx
	xorl	%eax,%eax
	xorl	%edx,%edx
	wrmsr
	sti
	call	_C_LABEL(ddb_ipi)
	xorl	%eax,%eax
	movq	%rax,%cr8
	INTRFASTEXIT
IDTVEC_END(handle_x2apic_ddbipi)

	TEXT_USER_BEGIN
IDTVEC(intr_ddbipi)
	pushq	$0
	pushq	$T_BPTFLT
	INTRENTRY
	jmp	_C_LABEL(Xhandle_ddbipi)
IDTVEC_END(intr_ddbipi)
IDTVEC(intr_x2apic_ddbipi)
	pushq	$0
	pushq	$T_BPTFLT
	INTRENTRY
	jmp	_C_LABEL(Xhandle_x2apic_ddbipi)
IDTVEC_END(intr_x2apic_ddbipi)
	TEXT_USER_END

#endif /* DDB */
#endif /* MULTIPROCESSOR */

	/*
	 * Interrupt from the local APIC timer.
	 */
IDTVEC(recurse_lapic_ltimer)
	INTR_RECURSE_HWFRAME
	pushq	$0
	pushq	$T_ASTFLT
	INTR_RECURSE_ENTRY
	jmp	1f
IDTVEC_END(recurse_lapic_ltimer)
IDTVEC(handle_x2apic_ltimer)
	movl	$(MSR_X2APIC_BASE + MSR_X2APIC_EOI),%ecx
	xorl	%eax,%eax
	xorl	%edx,%edx
	wrmsr
	movl	CPUVAR(ILEVEL),%ebx
	cmpl	$IPL_CLOCK,%ebx
	jae	2f
	jmp	1f
IDTVEC_END(handle_x2apic_ltimer)
IDTVEC(handle_lapic_ltimer)
	movq	_C_LABEL(local_apic_va),%rbx
	movl	$0,LAPIC_EOI(%rbx)
	movl	CPUVAR(ILEVEL),%ebx
	cmpl	$IPL_CLOCK,%ebx
	jae	2f
	jmp	1f
IDTVEC_END(handle_lapic_ltimer)
IDTVEC(resume_lapic_ltimer)
1:
	incl	CPUVAR(IDEPTH)
	movl	$IPL_CLOCK,CPUVAR(ILEVEL)
	sti
	pushq	%rbx
	movq	%rsp,%rsi
	xorq	%rdi,%rdi
	call	_C_LABEL(lapic_clockintr)
	jmp	_C_LABEL(Xdoreti)
2:
	orl	$(1 << LIR_TIMER),CPUVAR(IPENDING)
	INTRFASTEXIT
IDTVEC_END(resume_lapic_ltimer)

	TEXT_USER_BEGIN
IDTVEC(intr_x2apic_ltimer)
	pushq	$0
	pushq	$T_ASTFLT
	INTRENTRY
	jmp	_C_LABEL(Xhandle_x2apic_ltimer)
IDTVEC_END(intr_x2apic_ltimer)
IDTVEC(intr_lapic_ltimer)
	pushq	$0
	pushq	$T_ASTFLT
	INTRENTRY
	jmp	_C_LABEL(Xhandle_lapic_ltimer)
IDTVEC_END(intr_lapic_ltimer)
	TEXT_USER_END

#if NHYPERV > 0
	/*
	 * Hyper-V event channel upcall interrupt handler.
	 * Only used when the hypervisor supports direct vector callbacks.
	 */
IDTVEC(recurse_hyperv_hypercall)
	INTR_RECURSE_HWFRAME
	pushq	$0
	pushq	$T_ASTFLT
	INTR_RECURSE_ENTRY
	jmp	1f
IDTVEC_END(recurse_hyperv_hypercall)
NENTRY(handle_hyperv_hypercall)
	movl	CPUVAR(ILEVEL),%ebx
	cmpl	$IPL_NET,%ebx
	jae	2f
	jmp	1f
END(handle_hyperv_hypercall)
IDTVEC(resume_hyperv_hypercall)
1:
	incl	CPUVAR(IDEPTH)
	movl	$IPL_NET,CPUVAR(ILEVEL)
	sti
	pushq	%rbx
	movq	%rsp,%rsi
	call	_C_LABEL(hyperv_hypercall_intr)
	jmp	_C_LABEL(Xdoreti)
2:
	orl	$(1 << LIR_HV),CPUVAR(IPENDING)
	INTRFASTEXIT
IDTVEC_END(resume_hyperv_hypercall)

	TEXT_USER_BEGIN
IDTVEC(intr_hyperv_hypercall)
	pushq	$0
	pushq	$T_ASTFLT
	INTRENTRY
	jmp	_C_LABEL(handle_hyperv_hypercall)
IDTVEC_END(intr_hyperv_hypercall)
	TEXT_USER_END
#endif	/* NHYPERV > 0 */

#endif /* NLAPIC > 0 */

#ifndef XENPV
/*
 * TLB shootdown handler.
 */
IDTVEC(handle_lapic_tlb)
	movq	_C_LABEL(local_apic_va),%rax
	movl	$0,LAPIC_EOI(%rax)
	callq	_C_LABEL(pmap_tlb_intr)
	INTRFASTEXIT
IDTVEC_END(handle_lapic_tlb)
IDTVEC(handle_x2apic_tlb)
	movl	$(MSR_X2APIC_BASE + MSR_X2APIC_EOI),%ecx
	xorl	%eax,%eax
	xorl	%edx,%edx
	wrmsr
	callq	_C_LABEL(pmap_tlb_intr)
	INTRFASTEXIT
IDTVEC_END(handle_x2apic_tlb)

	TEXT_USER_BEGIN
IDTVEC(intr_lapic_tlb)
	pushq	$0
	pushq	$T_ASTFLT
	INTRENTRY
	jmp	_C_LABEL(Xhandle_lapic_tlb)
IDTVEC_END(intr_lapic_tlb)
IDTVEC(intr_x2apic_tlb)
	pushq	$0
	pushq	$T_ASTFLT
	INTRENTRY
	jmp	_C_LABEL(Xhandle_x2apic_tlb)
IDTVEC_END(intr_x2apic_tlb)
	TEXT_USER_END

#endif /* !XENPV */

#define voidop(num)

#ifndef XENPV

/*
 * This macro defines the generic stub code. Its arguments modify it
 * for specific PICs.
 */

#define	INTRSTUB(name, num, early_ack, late_ack, mask, unmask, level_mask) \
IDTVEC(recurse_ ## name ## num)						;\
	INTR_RECURSE_HWFRAME						;\
	subq	$8,%rsp							;\
	pushq	$T_ASTFLT		/* trap # for doing ASTs */	;\
	INTR_RECURSE_ENTRY						;\
	jmp	1f							;\
IDTVEC_END(recurse_ ## name ## num)					;\
IDTVEC(resume_ ## name ## num)						\
1:	movq	$IREENT_MAGIC,TF_ERR(%rsp)				;\
	movl	%ebx,%r13d						;\
	movq	CPUVAR(ISOURCES) + (num) * 8,%r14			;\
	movl	IS_MAXLEVEL(%r14),%ebx					;\
	jmp	1f							;\
IDTVEC_END(resume_ ## name ## num)					;\
IDTVEC(handle_ ## name ## num)						;\
	movq	CPUVAR(ISOURCES) + (num) * 8,%r14			;\
	mask(num)			/* mask it in hardware */	;\
	early_ack(num)			/* and allow other intrs */	;\
	testq	%r14,%r14						;\
	jz	9f			/* stray */			;\
	movl	IS_MAXLEVEL(%r14),%ebx					;\
	movl	CPUVAR(ILEVEL),%r13d					;\
	cmpl	%ebx,%r13d						;\
	jae	10f			/* currently masked; hold it */	;\
	incq	CPUVAR(NINTR)		/* statistical info */		;\
	incq	IS_EVCNT(%r14)						;\
1:									\
	pushq	%r13			/* save for Xdoreti */		;\
	movl	%ebx,CPUVAR(ILEVEL)					;\
	sti								;\
	incl	CPUVAR(IDEPTH)						;\
	movq	IS_HANDLERS(%r14),%rbx					;\
6:									\
	movl	IH_LEVEL(%rbx),%r12d					;\
	cmpl	%r13d,%r12d						;\
	jle	7f							;\
	movq	%rsp,%rsi						;\
	movq	IH_ARG(%rbx),%rdi					;\
	movl	%r12d,CPUVAR(ILEVEL)					;\
	call	*IH_FUN(%rbx)		/* call it */			;\
	movq	IH_NEXT(%rbx),%rbx	/* next handler in chain */	;\
	testq	%rbx,%rbx						;\
	jnz	6b							;\
5:									\
	cli								;\
	unmask(num)			/* unmask it in hardware */	;\
	late_ack(num)							;\
	sti								;\
	jmp	_C_LABEL(Xdoreti)	/* lower spl and do ASTs */	;\
7:									\
	cli								;\
	orl	$(1 << num),CPUVAR(IPENDING)				;\
	level_mask(num)							;\
	late_ack(num)							;\
	sti								;\
	jmp	_C_LABEL(Xdoreti)	/* lower spl and do ASTs */	;\
10:									\
	cli								;\
	orl	$(1 << num),CPUVAR(IPENDING)				;\
	level_mask(num)							;\
	late_ack(num)							;\
	INTRFASTEXIT							;\
9:									\
	unmask(num)							;\
	late_ack(num)							;\
	INTRFASTEXIT							;\
IDTVEC_END(handle_ ## name ## num)					;\
	TEXT_USER_BEGIN							;\
IDTVEC(intr_ ## name ## num)						;\
	pushq	$0			/* dummy error code */		;\
	pushq	$T_ASTFLT		/* trap # for doing ASTs */	;\
	INTRENTRY							;\
	jmp	_C_LABEL(Xhandle_ ## name ## num)			;\
IDTVEC_END(intr_ ## name ## num)					;\
	TEXT_USER_END

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

INTRSTUB(ioapic_edge,0,voidop,ioapic_asm_ack,voidop,voidop,voidop)
INTRSTUB(ioapic_edge,1,voidop,ioapic_asm_ack,voidop,voidop,voidop)
INTRSTUB(ioapic_edge,2,voidop,ioapic_asm_ack,voidop,voidop,voidop)
INTRSTUB(ioapic_edge,3,voidop,ioapic_asm_ack,voidop,voidop,voidop)
INTRSTUB(ioapic_edge,4,voidop,ioapic_asm_ack,voidop,voidop,voidop)
INTRSTUB(ioapic_edge,5,voidop,ioapic_asm_ack,voidop,voidop,voidop)
INTRSTUB(ioapic_edge,6,voidop,ioapic_asm_ack,voidop,voidop,voidop)
INTRSTUB(ioapic_edge,7,voidop,ioapic_asm_ack,voidop,voidop,voidop)
INTRSTUB(ioapic_edge,8,voidop,ioapic_asm_ack,voidop,voidop,voidop)
INTRSTUB(ioapic_edge,9,voidop,ioapic_asm_ack,voidop,voidop,voidop)
INTRSTUB(ioapic_edge,10,voidop,ioapic_asm_ack,voidop,voidop,voidop)
INTRSTUB(ioapic_edge,11,voidop,ioapic_asm_ack,voidop,voidop,voidop)
INTRSTUB(ioapic_edge,12,voidop,ioapic_asm_ack,voidop,voidop,voidop)
INTRSTUB(ioapic_edge,13,voidop,ioapic_asm_ack,voidop,voidop,voidop)
INTRSTUB(ioapic_edge,14,voidop,ioapic_asm_ack,voidop,voidop,voidop)
INTRSTUB(ioapic_edge,15,voidop,ioapic_asm_ack,voidop,voidop,voidop)
INTRSTUB(ioapic_edge,16,voidop,ioapic_asm_ack,voidop,voidop,voidop)
INTRSTUB(ioapic_edge,17,voidop,ioapic_asm_ack,voidop,voidop,voidop)
INTRSTUB(ioapic_edge,18,voidop,ioapic_asm_ack,voidop,voidop,voidop)
INTRSTUB(ioapic_edge,19,voidop,ioapic_asm_ack,voidop,voidop,voidop)
INTRSTUB(ioapic_edge,20,voidop,ioapic_asm_ack,voidop,voidop,voidop)
INTRSTUB(ioapic_edge,21,voidop,ioapic_asm_ack,voidop,voidop,voidop)
INTRSTUB(ioapic_edge,22,voidop,ioapic_asm_ack,voidop,voidop,voidop)
INTRSTUB(ioapic_edge,23,voidop,ioapic_asm_ack,voidop,voidop,voidop)
INTRSTUB(ioapic_edge,24,voidop,ioapic_asm_ack,voidop,voidop,voidop)
INTRSTUB(ioapic_edge,25,voidop,ioapic_asm_ack,voidop,voidop,voidop)
INTRSTUB(ioapic_edge,26,voidop,ioapic_asm_ack,voidop,voidop,voidop)
INTRSTUB(ioapic_edge,27,voidop,ioapic_asm_ack,voidop,voidop,voidop)
INTRSTUB(ioapic_edge,28,voidop,ioapic_asm_ack,voidop,voidop,voidop)
INTRSTUB(ioapic_edge,29,voidop,ioapic_asm_ack,voidop,voidop,voidop)
INTRSTUB(ioapic_edge,30,voidop,ioapic_asm_ack,voidop,voidop,voidop)
INTRSTUB(ioapic_edge,31,voidop,ioapic_asm_ack,voidop,voidop,voidop)

INTRSTUB(ioapic_level,0,voidop,ioapic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(ioapic_level,1,voidop,ioapic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(ioapic_level,2,voidop,ioapic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(ioapic_level,3,voidop,ioapic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(ioapic_level,4,voidop,ioapic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(ioapic_level,5,voidop,ioapic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(ioapic_level,6,voidop,ioapic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(ioapic_level,7,voidop,ioapic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(ioapic_level,8,voidop,ioapic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(ioapic_level,9,voidop,ioapic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(ioapic_level,10,voidop,ioapic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(ioapic_level,11,voidop,ioapic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(ioapic_level,12,voidop,ioapic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(ioapic_level,13,voidop,ioapic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(ioapic_level,14,voidop,ioapic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(ioapic_level,15,voidop,ioapic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(ioapic_level,16,voidop,ioapic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(ioapic_level,17,voidop,ioapic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(ioapic_level,18,voidop,ioapic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(ioapic_level,19,voidop,ioapic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(ioapic_level,20,voidop,ioapic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(ioapic_level,21,voidop,ioapic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(ioapic_level,22,voidop,ioapic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(ioapic_level,23,voidop,ioapic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(ioapic_level,24,voidop,ioapic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(ioapic_level,25,voidop,ioapic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(ioapic_level,26,voidop,ioapic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(ioapic_level,27,voidop,ioapic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(ioapic_level,28,voidop,ioapic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(ioapic_level,29,voidop,ioapic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(ioapic_level,30,voidop,ioapic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(ioapic_level,31,voidop,ioapic_asm_ack,voidop,ioapic_unmask,ioapic_mask)

INTRSTUB(x2apic_edge,0,voidop,x2apic_asm_ack,voidop,voidop,voidop)
INTRSTUB(x2apic_edge,1,voidop,x2apic_asm_ack,voidop,voidop,voidop)
INTRSTUB(x2apic_edge,2,voidop,x2apic_asm_ack,voidop,voidop,voidop)
INTRSTUB(x2apic_edge,3,voidop,x2apic_asm_ack,voidop,voidop,voidop)
INTRSTUB(x2apic_edge,4,voidop,x2apic_asm_ack,voidop,voidop,voidop)
INTRSTUB(x2apic_edge,5,voidop,x2apic_asm_ack,voidop,voidop,voidop)
INTRSTUB(x2apic_edge,6,voidop,x2apic_asm_ack,voidop,voidop,voidop)
INTRSTUB(x2apic_edge,7,voidop,x2apic_asm_ack,voidop,voidop,voidop)
INTRSTUB(x2apic_edge,8,voidop,x2apic_asm_ack,voidop,voidop,voidop)
INTRSTUB(x2apic_edge,9,voidop,x2apic_asm_ack,voidop,voidop,voidop)
INTRSTUB(x2apic_edge,10,voidop,x2apic_asm_ack,voidop,voidop,voidop)
INTRSTUB(x2apic_edge,11,voidop,x2apic_asm_ack,voidop,voidop,voidop)
INTRSTUB(x2apic_edge,12,voidop,x2apic_asm_ack,voidop,voidop,voidop)
INTRSTUB(x2apic_edge,13,voidop,x2apic_asm_ack,voidop,voidop,voidop)
INTRSTUB(x2apic_edge,14,voidop,x2apic_asm_ack,voidop,voidop,voidop)
INTRSTUB(x2apic_edge,15,voidop,x2apic_asm_ack,voidop,voidop,voidop)
INTRSTUB(x2apic_edge,16,voidop,x2apic_asm_ack,voidop,voidop,voidop)
INTRSTUB(x2apic_edge,17,voidop,x2apic_asm_ack,voidop,voidop,voidop)
INTRSTUB(x2apic_edge,18,voidop,x2apic_asm_ack,voidop,voidop,voidop)
INTRSTUB(x2apic_edge,19,voidop,x2apic_asm_ack,voidop,voidop,voidop)
INTRSTUB(x2apic_edge,20,voidop,x2apic_asm_ack,voidop,voidop,voidop)
INTRSTUB(x2apic_edge,21,voidop,x2apic_asm_ack,voidop,voidop,voidop)
INTRSTUB(x2apic_edge,22,voidop,x2apic_asm_ack,voidop,voidop,voidop)
INTRSTUB(x2apic_edge,23,voidop,x2apic_asm_ack,voidop,voidop,voidop)
INTRSTUB(x2apic_edge,24,voidop,x2apic_asm_ack,voidop,voidop,voidop)
INTRSTUB(x2apic_edge,25,voidop,x2apic_asm_ack,voidop,voidop,voidop)
INTRSTUB(x2apic_edge,26,voidop,x2apic_asm_ack,voidop,voidop,voidop)
INTRSTUB(x2apic_edge,27,voidop,x2apic_asm_ack,voidop,voidop,voidop)
INTRSTUB(x2apic_edge,28,voidop,x2apic_asm_ack,voidop,voidop,voidop)
INTRSTUB(x2apic_edge,29,voidop,x2apic_asm_ack,voidop,voidop,voidop)
INTRSTUB(x2apic_edge,30,voidop,x2apic_asm_ack,voidop,voidop,voidop)
INTRSTUB(x2apic_edge,31,voidop,x2apic_asm_ack,voidop,voidop,voidop)

INTRSTUB(x2apic_level,0,voidop,x2apic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(x2apic_level,1,voidop,x2apic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(x2apic_level,2,voidop,x2apic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(x2apic_level,3,voidop,x2apic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(x2apic_level,4,voidop,x2apic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(x2apic_level,5,voidop,x2apic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(x2apic_level,6,voidop,x2apic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(x2apic_level,7,voidop,x2apic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(x2apic_level,8,voidop,x2apic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(x2apic_level,9,voidop,x2apic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(x2apic_level,10,voidop,x2apic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(x2apic_level,11,voidop,x2apic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(x2apic_level,12,voidop,x2apic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(x2apic_level,13,voidop,x2apic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(x2apic_level,14,voidop,x2apic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(x2apic_level,15,voidop,x2apic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(x2apic_level,16,voidop,x2apic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(x2apic_level,17,voidop,x2apic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(x2apic_level,18,voidop,x2apic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(x2apic_level,19,voidop,x2apic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(x2apic_level,20,voidop,x2apic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(x2apic_level,21,voidop,x2apic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(x2apic_level,22,voidop,x2apic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(x2apic_level,23,voidop,x2apic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(x2apic_level,24,voidop,x2apic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(x2apic_level,25,voidop,x2apic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(x2apic_level,26,voidop,x2apic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(x2apic_level,27,voidop,x2apic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(x2apic_level,28,voidop,x2apic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(x2apic_level,29,voidop,x2apic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(x2apic_level,30,voidop,x2apic_asm_ack,voidop,ioapic_unmask,ioapic_mask)
INTRSTUB(x2apic_level,31,voidop,x2apic_asm_ack,voidop,ioapic_unmask,ioapic_mask)

#endif

/*
 * Create a struct intrstub.
 */
#define INTRSTUB_ENTRY(name) \
	.quad _C_LABEL(Xintr_ ## name ), _C_LABEL(Xrecurse_ ## name ) ; \
	.quad _C_LABEL(Xresume_ ## name ) ;

/*
 * Create an array of structs intrstub (16 entries).
 */
#define INTRSTUB_ARRAY_16(name) 		; \
	.type _C_LABEL(name ## _stubs), @object	; \
LABEL(name ## _stubs)				; \
	INTRSTUB_ENTRY(name ## 0)		; \
	INTRSTUB_ENTRY(name ## 1)		; \
	INTRSTUB_ENTRY(name ## 2)		; \
	INTRSTUB_ENTRY(name ## 3)		; \
	INTRSTUB_ENTRY(name ## 4)		; \
	INTRSTUB_ENTRY(name ## 5)		; \
	INTRSTUB_ENTRY(name ## 6)		; \
	INTRSTUB_ENTRY(name ## 7)		; \
	INTRSTUB_ENTRY(name ## 8)		; \
	INTRSTUB_ENTRY(name ## 9)		; \
	INTRSTUB_ENTRY(name ## 10)		; \
	INTRSTUB_ENTRY(name ## 11)		; \
	INTRSTUB_ENTRY(name ## 12)		; \
	INTRSTUB_ENTRY(name ## 13)		; \
	INTRSTUB_ENTRY(name ## 14)		; \
	INTRSTUB_ENTRY(name ## 15)		; \
END(name ## _stubs)

/*
 * Create an array of structs intrstub (32 entries).
 */
#define INTRSTUB_ARRAY_32(name) 		; \
	.type _C_LABEL(name ## _stubs), @object	; \
LABEL(name ## _stubs)				; \
	INTRSTUB_ENTRY(name ## 0)		; \
	INTRSTUB_ENTRY(name ## 1)		; \
	INTRSTUB_ENTRY(name ## 2)		; \
	INTRSTUB_ENTRY(name ## 3)		; \
	INTRSTUB_ENTRY(name ## 4)		; \
	INTRSTUB_ENTRY(name ## 5)		; \
	INTRSTUB_ENTRY(name ## 6)		; \
	INTRSTUB_ENTRY(name ## 7)		; \
	INTRSTUB_ENTRY(name ## 8)		; \
	INTRSTUB_ENTRY(name ## 9)		; \
	INTRSTUB_ENTRY(name ## 10)		; \
	INTRSTUB_ENTRY(name ## 11)		; \
	INTRSTUB_ENTRY(name ## 12)		; \
	INTRSTUB_ENTRY(name ## 13)		; \
	INTRSTUB_ENTRY(name ## 14)		; \
	INTRSTUB_ENTRY(name ## 15)		; \
	INTRSTUB_ENTRY(name ## 16)		; \
	INTRSTUB_ENTRY(name ## 17)		; \
	INTRSTUB_ENTRY(name ## 18)		; \
	INTRSTUB_ENTRY(name ## 19)		; \
	INTRSTUB_ENTRY(name ## 20)		; \
	INTRSTUB_ENTRY(name ## 21)		; \
	INTRSTUB_ENTRY(name ## 22)		; \
	INTRSTUB_ENTRY(name ## 23)		; \
	INTRSTUB_ENTRY(name ## 24)		; \
	INTRSTUB_ENTRY(name ## 25)		; \
	INTRSTUB_ENTRY(name ## 26)		; \
	INTRSTUB_ENTRY(name ## 27)		; \
	INTRSTUB_ENTRY(name ## 28)		; \
	INTRSTUB_ENTRY(name ## 29)		; \
	INTRSTUB_ENTRY(name ## 30)		; \
	INTRSTUB_ENTRY(name ## 31)		; \
END(name ## _stubs)

#endif /* !XENPV */

#if defined(XEN)
/* Resume/recurse procedures for spl() */
#define	XENINTRSTUB(name, num, early_ack, late_ack, mask, unmask, level_mask) \
IDTVEC(recurse_ ## name ## num)						;\
	INTR_RECURSE_HWFRAME						;\
	subq	$8,%rsp							;\
	pushq	$T_ASTFLT		/* trap # for doing ASTs */	;\
	INTR_RECURSE_ENTRY						;\
IDTVEC(resume_ ## name ## num)						\
	movq	$IREENT_MAGIC,TF_ERR(%rsp)				;\
	movl	%ebx,%r13d						;\
	movq	CPUVAR(XSOURCES) + (num) * 8,%r14			;\
1:									\
	pushq	%r13							;\
	movl	$num,CPUVAR(ILEVEL)					;\
	STI(si)								;\
	incl	CPUVAR(IDEPTH)						;\
	movq	IS_HANDLERS(%r14),%rbx					;\
6:									\
	movq	IH_ARG(%rbx),%rdi					;\
	movq	%rsp,%rsi						;\
	call	*IH_FUN(%rbx)		/* call it */			;\
	movq	IH_NEXT(%rbx),%rbx	/* next handler in chain */	;\
	testq	%rbx,%rbx						;\
	jnz	6b							;\
5:									\
	CLI(si)								;\
	unmask(num)			/* unmask it in hardware */	;\
	late_ack(num)							;\
	STI(si)								;\
	jmp	_C_LABEL(Xdoreti)	/* lower spl and do ASTs */	;\

/* The unmask func for Xen events */
#define hypervisor_asm_unmask(num)			\
	movq	$num,%rdi				;\
	call	_C_LABEL(hypervisor_enable_ipl)

XENINTRSTUB(xenev,0,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,1,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,2,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,3,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,4,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,5,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,6,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,7,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,8,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,9,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,10,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,11,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,12,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,13,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,14,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,15,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,16,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,17,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,18,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,19,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,20,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,21,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,22,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,23,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,24,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,25,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,26,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,27,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,28,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,29,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,30,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)
XENINTRSTUB(xenev,31,voidop,voidop,voidop,hypervisor_asm_unmask,voidop)

/* On Xen, the xenev_stubs are purely for spl entry, since there is no
 * vector based mechanism. We however provide the entrypoint to ensure
 * that native and Xen struct intrstub ; definitions are uniform.
 */
panicmsg:	.ascii "vector Xen event entry path entered."
LABEL(entry_xenev)
	movq $panicmsg, %rdi
	callq _C_LABEL(panic)
END(entry_xenev)

LABEL(xenev_stubs)
	.quad entry_xenev, _C_LABEL(Xrecurse_xenev0), _C_LABEL(Xresume_xenev0)
	.quad entry_xenev, _C_LABEL(Xrecurse_xenev1) ,_C_LABEL(Xresume_xenev1)
	.quad entry_xenev, _C_LABEL(Xrecurse_xenev2) ,_C_LABEL(Xresume_xenev2)
	.quad entry_xenev, _C_LABEL(Xrecurse_xenev3) ,_C_LABEL(Xresume_xenev3)
	.quad entry_xenev, _C_LABEL(Xrecurse_xenev4) ,_C_LABEL(Xresume_xenev4)
	.quad entry_xenev, _C_LABEL(Xrecurse_xenev5) ,_C_LABEL(Xresume_xenev5)
	.quad entry_xenev, _C_LABEL(Xrecurse_xenev6) ,_C_LABEL(Xresume_xenev6)
	.quad entry_xenev, _C_LABEL(Xrecurse_xenev7) ,_C_LABEL(Xresume_xenev7)
	.quad entry_xenev, _C_LABEL(Xrecurse_xenev8) ,_C_LABEL(Xresume_xenev8)
	.quad entry_xenev, _C_LABEL(Xrecurse_xenev9) ,_C_LABEL(Xresume_xenev9)
	.quad entry_xenev, _C_LABEL(Xrecurse_xenev10), _C_LABEL(Xresume_xenev10)
	.quad entry_xenev, _C_LABEL(Xrecurse_xenev11), _C_LABEL(Xresume_xenev11)
	.quad entry_xenev, _C_LABEL(Xrecurse_xenev12), _C_LABEL(Xresume_xenev12)
	.quad entry_xenev, _C_LABEL(Xrecurse_xenev13), _C_LABEL(Xresume_xenev13)
	.quad entry_xenev, _C_LABEL(Xrecurse_xenev14), _C_LABEL(Xresume_xenev14)
	.quad entry_xenev, _C_LABEL(Xrecurse_xenev15), _C_LABEL(Xresume_xenev15)
	.quad entry_xenev, _C_LABEL(Xrecurse_xenev16), _C_LABEL(Xresume_xenev16)
	.quad entry_xenev, _C_LABEL(Xrecurse_xenev17), _C_LABEL(Xresume_xenev17)
	.quad entry_xenev, _C_LABEL(Xrecurse_xenev18), _C_LABEL(Xresume_xenev18)
	.quad entry_xenev, _C_LABEL(Xrecurse_xenev19), _C_LABEL(Xresume_xenev19)
	.quad entry_xenev, _C_LABEL(Xrecurse_xenev20), _C_LABEL(Xresume_xenev20)
	.quad entry_xenev, _C_LABEL(Xrecurse_xenev21), _C_LABEL(Xresume_xenev21)
	.quad entry_xenev, _C_LABEL(Xrecurse_xenev22), _C_LABEL(Xresume_xenev22)
	.quad entry_xenev, _C_LABEL(Xrecurse_xenev23), _C_LABEL(Xresume_xenev23)
	.quad entry_xenev, _C_LABEL(Xrecurse_xenev24), _C_LABEL(Xresume_xenev24)
	.quad entry_xenev, _C_LABEL(Xrecurse_xenev25), _C_LABEL(Xresume_xenev25)
	.quad entry_xenev, _C_LABEL(Xrecurse_xenev26), _C_LABEL(Xresume_xenev26)
	.quad entry_xenev, _C_LABEL(Xrecurse_xenev27), _C_LABEL(Xresume_xenev27)
	.quad entry_xenev, _C_LABEL(Xrecurse_xenev28), _C_LABEL(Xresume_xenev28)
	.quad entry_xenev, _C_LABEL(Xrecurse_xenev29), _C_LABEL(Xresume_xenev29)
	.quad entry_xenev, _C_LABEL(Xrecurse_xenev30), _C_LABEL(Xresume_xenev30)
	.quad entry_xenev, _C_LABEL(Xrecurse_xenev31), _C_LABEL(Xresume_xenev31)
END(xenev_stubs)

/*
 * Xen callbacks
 */

/* Hypervisor callback */
ENTRY(hypervisor_callback)
	movq	(%rsp),%rcx
	movq	8(%rsp),%r11
	addq	$16,%rsp
IDTVEC(hypervisor_pvhvm_callback)
	pushq	$0		/* Dummy error code */
	pushq	$T_ASTFLT
	INTRENTRY
	/* sti?? */
	movq	%rsp,%rdi
	subq	$8,%rdi;	/* don't forget if_ppl */
	call	do_hypervisor_callback
	testb	$SEL_RPL,TF_CS(%rsp)
	jnz	doreti_checkast
1:
	INTRFASTEXIT
IDTVEC_END(hypervisor_pvhvm_callback)
END(hypervisor_callback)
#endif /* XEN */

#ifdef XENPV
/* Panic? */
ENTRY(failsafe_callback)
	movq	(%rsp),%rcx
	movq	8(%rsp),%r11
	addq	$16,%rsp
	pushq	$0
	pushq	$T_ASTFLT
	INTRENTRY
	movq	%rsp,%rdi
	subq	$8,%rdi;	/* don't forget if_ppl */
	call	xen_failsafe_handler
	INTRFASTEXIT
/*	jmp	HYPERVISOR_iret */
END(failsafe_callback)

#else	/* XENPV */

	.section .rodata

INTRSTUB_ARRAY_16(legacy)

#if NIOAPIC > 0
INTRSTUB_ARRAY_32(ioapic_edge)
INTRSTUB_ARRAY_32(ioapic_level)

INTRSTUB_ARRAY_32(x2apic_edge)
INTRSTUB_ARRAY_32(x2apic_level)
#endif
#endif /* !XENPV */
