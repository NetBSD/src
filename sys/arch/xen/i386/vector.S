/*	$NetBSD: vector.S,v 1.3 2004/05/07 13:52:37 cl Exp $	*/
/*	NetBSD: 1.13 2004/03/11 11:39:26 yamt Exp 	*/

/*
 * Copyright 2002 (c) Wasabi Systems, Inc.
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
#include "opt_ipkdb.h"
#include "opt_vm86.h"
#include "opt_xen.h"

#ifndef XEN
#include <machine/i8259.h>
#endif
#include <machine/i82093reg.h>
#include <machine/i82489reg.h>
#include <machine/asm.h>
#include <machine/frameasm.h>
#include <machine/segments.h>
#include <machine/trap.h>
#include <machine/intr.h>
#include <machine/psl.h>
#ifdef XEN
#include <machine/xen.h>
#endif

#include <net/netisr.h>

#include "ioapic.h"
#include "lapic.h"

#include "npx.h"
#include "assym.h"
/* XXX assym.h */
#define	EVENTS_MASK	4

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
 * The direct costs of setting up a trap frame are two pushl's (error code and
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
#define	XSTRAY(name,num)	Xstray_/**/name/**/num
#define XINTR_TSS(irq_num)	Xintr_tss_ ## irq_num
#else
#define	XINTR(name,num)		_Xintr_/**/name/**/num
#define	XSTRAY(name,num)	_Xstray_/**/name/**/num
#define XINTR_TSS(irq_num)	Xintr_tss_/**/irq_num
#endif

/*
 * Store address of TSS in %eax, given a selector in %eax.
 * Clobbers %eax, %ecx, %edx, but that's ok for its usage.
 * This is a bit complicated, but it's done to make as few
 * assumptions as possible about the validity of the environment.
 * The GDT and the current and previous TSS are known to be OK,
 * otherwise we would not be here. The only other thing that needs
 * to be OK is the cpu_info structure for the current CPU.
 */
#define GET_TSS \
	andl	$0xfff8,%eax				;\
	addl	CPUVAR(GDT),%eax			;\
	movl	2(%eax),%edx				;\
	andl	$0xffffff,%edx				;\
	movzbl	7(%eax),%eax				;\
	shl	$24,%eax				;\
	orl	%edx,%eax

#if NLAPIC > 0
#ifdef MULTIPROCESSOR
IDTVEC(recurse_lapic_ipi)
	pushfl
	pushl	%cs
	pushl	%esi
	pushl	$0		
	pushl	$T_ASTFLT
	INTRENTRY		
IDTVEC(resume_lapic_ipi)
	cli
	jmp	1f
IDTVEC(intr_lapic_ipi)
	pushl	$0		
	pushl	$T_ASTFLT
	INTRENTRY		
	movl	$0,_C_LABEL(local_apic)+LAPIC_EOI
	movl	CPUVAR(ILEVEL),%ebx
	cmpl	$IPL_IPI,%ebx
	jae	2f
1:
	incl	CPUVAR(IDEPTH)
	movl	$IPL_IPI,CPUVAR(ILEVEL)
        sti
	pushl	%ebx
	call	_C_LABEL(x86_ipi_handler)
	jmp	_C_LABEL(Xdoreti)
2:
	orl	$(1 << LIR_IPI),CPUVAR(IPENDING)
	sti
	INTRFASTEXIT

#if defined(DDB)
IDTVEC(intrddbipi)
1:
	str	%ax
	GET_TSS
	movzwl	(%eax),%eax
	GET_TSS
	pushl	%eax
	movl	$0xff,_C_LABEL(lapic_tpr)
	movl	$0,_C_LABEL(local_apic)+LAPIC_EOI
	sti
	call	_C_LABEL(ddb_ipi_tss)
	addl	$4,%esp
	movl	$0,_C_LABEL(lapic_tpr)
	iret
	jmp	1b
#endif /* DDB */
#endif /* MULTIPROCESSOR */
	
	/*
	 * Interrupt from the local APIC timer.
	 */
IDTVEC(recurse_lapic_ltimer)
	pushfl
	pushl	%cs
	pushl	%esi
	pushl	$0		
	pushl	$T_ASTFLT
	INTRENTRY		
IDTVEC(resume_lapic_ltimer)
	cli
	jmp	1f
IDTVEC(intr_lapic_ltimer)
	pushl	$0		
	pushl	$T_ASTFLT
	INTRENTRY		
	movl	$0,_C_LABEL(local_apic)+LAPIC_EOI
	movl	CPUVAR(ILEVEL),%ebx
	cmpl	$IPL_CLOCK,%ebx
	jae	2f
1:
	incl	CPUVAR(IDEPTH)
	movl	$IPL_CLOCK,CPUVAR(ILEVEL)
	sti
	pushl	%ebx
	pushl	$0
	call	_C_LABEL(lapic_clockintr)
	addl	$4,%esp		
	jmp	_C_LABEL(Xdoreti)
2:
	orl	$(1 << LIR_TIMER),CPUVAR(IPENDING)
	sti
	INTRFASTEXIT
#endif /* NLAPIC > 0 */

#ifdef MULTIPROCESSOR
#define LOCK_KERNEL	pushl %esp ; call _C_LABEL(x86_intlock) ; addl $4,%esp
#define UNLOCK_KERNEL	pushl %esp ; call _C_LABEL(x86_intunlock) ; addl $4,%esp
#else
#define LOCK_KERNEL
#define UNLOCK_KERNEL
#endif

#define voidop(num)


#define	XENINTRSTUB(name, num, early_ack, late_ack, mask, unmask, level_mask) \
IDTVEC(recurse_/**/name/**/num)						;\
	pushfl								;\
	pushl	%cs							;\
	pushl	%esi							;\
	subl	$4,%esp							;\
	pushl	$T_ASTFLT		/* trap # for doing ASTs */	;\
	INTRENTRY							;\
IDTVEC(resume_/**/name/**/num)						\
	/*movl	%esp,%ecx*/						;\
	movl	$IREENT_MAGIC,TF_ERR(%esp)				;\
	movl	%ebx,%esi						;\
	movl	CPUVAR(ISOURCES) + (num) * 4, %ebp			;\
	movl	IS_MAXLEVEL(%ebp),%ebx					;\
	jmp	1f							;\
IDTVEC(intr_/**/name/**/num)						;\
	pushl	$0			/* dummy error code */		;\
	pushl	$T_ASTFLT		/* trap # for doing ASTs */	;\
	INTRENTRY							;\
	/*movl	%esp,%ecx*/						;\
	movl	CPUVAR(ISOURCES) + (num) * 4, %ebp		;\
	mask(num)		/* mask it in hardware */	;\
	early_ack(num)			/* and allow other intrs */	;\
	testl	%ebp,%ebp						;\
	jz	9f			/* stray */			;\
	movl	IS_MAXLEVEL(%ebp),%ebx					;\
	movl	CPUVAR(ILEVEL),%esi					;\
	cmpl	%ebx,%esi						;\
	jae	10f			/* currently masked; hold it */	;\
	incl	MY_COUNT+V_INTR		/* statistical info */		;\
	addl	$1,IS_EVCNTLO(%ebp)	/* inc event counter */		;\
	adcl	$0,IS_EVCNTHI(%ebp)					;\
1:									\
	pushl	%esi							;\
	movl	%ebx,CPUVAR(ILEVEL)					;\
	STI(%eax)							;\
	incl	CPUVAR(IDEPTH)						;\
	movl	IS_HANDLERS(%ebp),%ebx					;\
	LOCK_KERNEL							;\
6:									\
	movl	IH_LEVEL(%ebx),%edi					;\
	cmpl	%esi,%edi						;\
	jle	7f							;\
	pushl	%esp							;\
	pushl	IH_ARG(%ebx)						;\
	movl	%edi,CPUVAR(ILEVEL)					;\
	call	*IH_FUN(%ebx)		/* call it */			;\
	addl	$8,%esp			/* toss the arg */		;\
	movl	IH_NEXT(%ebx),%ebx	/* next handler in chain */	;\
	testl	%ebx,%ebx						;\
	jnz	6b							;\
5:									\
	UNLOCK_KERNEL							;\
	CLI(%eax)							;\
	unmask(num)			/* unmask it in hardware */	;\
	late_ack(num)							;\
	STI(%eax)							;\
	jmp	_C_LABEL(Xdoreti)	/* lower spl and do ASTs */	;\
7:									\
	UNLOCK_KERNEL							;\
	CLI(%eax)							;\
	orl     $(1 << num),CPUVAR(IPENDING)				;\
	level_mask(num)							;\
	late_ack(num)							;\
	STI(%eax)							;\
	jmp	_C_LABEL(Xdoreti)	/* lower spl and do ASTs */	;\
10:									\
	CLI(%eax)							;\
	orl     $(1 << num),CPUVAR(IPENDING)				;\
	level_mask(num)							;\
	late_ack(num)							;\
	STIC(%eax)							;\
    	jz	4f  		; \
	call	_C_LABEL(stipending) ; \
	testl	%eax,%eax	; \
	jnz	1b		; \
4:	INTRFASTEXIT							;\
9:									\
	unmask(num)							;\
	late_ack(num)							;\
	STIC(%eax)							;\
    	jz	4f		; \
	call	_C_LABEL(stipending) ; \
	testl	%eax,%eax	; \
	jnz	1b		; \
4:	INTRFASTEXIT


/*
 * This macro defines the generic stub code. Its arguments modifiy it
 * for specific PICs.
 */

#define	INTRSTUB(name, num, early_ack, late_ack, mask, unmask, level_mask) \
IDTVEC(recurse_/**/name/**/num)						;\
	pushfl								;\
	pushl	%cs							;\
	pushl	%esi							;\
	subl	$4,%esp							;\
	pushl	$T_ASTFLT		/* trap # for doing ASTs */	;\
	INTRENTRY							;\
IDTVEC(resume_/**/name/**/num)						\
	movl	$IREENT_MAGIC,TF_ERR(%esp)				;\
	movl	%ebx,%esi						;\
	movl	CPUVAR(ISOURCES) + (num) * 4, %ebp			;\
	movl	IS_MAXLEVEL(%ebp),%ebx					;\
	jmp	1f							;\
IDTVEC(intr_/**/name/**/num)						;\
	pushl	$0			/* dummy error code */		;\
	pushl	$T_ASTFLT		/* trap # for doing ASTs */	;\
	INTRENTRY							;\
	movl	CPUVAR(ISOURCES) + (num) * 4, %ebp		;\
	mask(num)		/* mask it in hardware */	;\
	early_ack(num)			/* and allow other intrs */	;\
	testl	%ebp,%ebp						;\
	jz	9f			/* stray */			;\
	movl	IS_MAXLEVEL(%ebp),%ebx					;\
	movl	CPUVAR(ILEVEL),%esi					;\
	cmpl	%ebx,%esi						;\
	jae	10f			/* currently masked; hold it */	;\
	incl	MY_COUNT+V_INTR		/* statistical info */		;\
	addl	$1,IS_EVCNTLO(%ebp)	/* inc event counter */		;\
	adcl	$0,IS_EVCNTHI(%ebp)					;\
1:									\
	pushl	%esi							;\
	movl	%ebx,CPUVAR(ILEVEL)					;\
	STI(%eax)							;\
	incl	CPUVAR(IDEPTH)						;\
	movl	IS_HANDLERS(%ebp),%ebx					;\
	LOCK_KERNEL							;\
6:									\
	movl	IH_LEVEL(%ebx),%edi					;\
	cmpl	%esi,%edi						;\
	jle	7f							;\
	pushl	IH_ARG(%ebx)						;\
	movl	%edi,CPUVAR(ILEVEL)					;\
	call	*IH_FUN(%ebx)		/* call it */			;\
	addl	$4,%esp			/* toss the arg */		;\
	movl	IH_NEXT(%ebx),%ebx	/* next handler in chain */	;\
	testl	%ebx,%ebx						;\
	jnz	6b							;\
5:									\
	UNLOCK_KERNEL							;\
	CLI(%eax)							;\
	unmask(num)			/* unmask it in hardware */	;\
	late_ack(num)							;\
	STI(%eax)							;\
	jmp	_C_LABEL(Xdoreti)	/* lower spl and do ASTs */	;\
7:									\
	UNLOCK_KERNEL							;\
	CLI(%eax)							;\
	orl     $(1 << num),CPUVAR(IPENDING)				;\
	level_mask(num)							;\
	late_ack(num)							;\
	STI(%eax)							;\
	jmp	_C_LABEL(Xdoreti)	/* lower spl and do ASTs */	;\
10:									\
	CLI(%eax)							;\
	orl     $(1 << num),CPUVAR(IPENDING)				;\
	level_mask(num)							;\
	late_ack(num)							;\
	STIC(%eax)							;\
    	jz	4f		; \
	call	_C_LABEL(stipending) ; \
	testl	%eax,%eax	; \
	jnz	1b		; \
4:	INTRFASTEXIT							;\
9:									\
	unmask(num)							;\
	late_ack(num)							;\
	STIC(%eax)							;\
    	jz	4f  		; \
	call	_C_LABEL(stipending) ; \
	testl	%eax,%eax   	; \
	jnz	1b		; \
4:	INTRFASTEXIT

#define ICUADDR IO_ICU1

#ifndef XEN
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
#endif

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

#endif

XENINTRSTUB(xenev,0,hypervisor_asm_ack,voidop,voidop,voidop,voidop)
XENINTRSTUB(xenev,1,hypervisor_asm_ack,voidop,voidop,voidop,voidop)
XENINTRSTUB(xenev,2,hypervisor_asm_ack,voidop,voidop,voidop,voidop)
XENINTRSTUB(xenev,3,hypervisor_asm_ack,voidop,voidop,voidop,voidop)
XENINTRSTUB(xenev,4,hypervisor_asm_ack,voidop,voidop,voidop,voidop)
XENINTRSTUB(xenev,5,hypervisor_asm_ack,voidop,voidop,voidop,voidop)

.globl _C_LABEL(xenev_stubs)
_C_LABEL(xenev_stubs):
	.long _C_LABEL(Xintr_xenev0), _C_LABEL(Xrecurse_xenev0)
	.long _C_LABEL(Xresume_xenev0)
	.long _C_LABEL(Xintr_xenev1), _C_LABEL(Xrecurse_xenev1)
	.long _C_LABEL(Xresume_xenev1)
	.long _C_LABEL(Xintr_xenev2), _C_LABEL(Xrecurse_xenev2)
	.long _C_LABEL(Xresume_xenev2)
	.long _C_LABEL(Xintr_xenev3), _C_LABEL(Xrecurse_xenev3)
	.long _C_LABEL(Xresume_xenev3)
	.long _C_LABEL(Xintr_xenev4), _C_LABEL(Xrecurse_xenev4)
	.long _C_LABEL(Xresume_xenev4)
	.long _C_LABEL(Xintr_xenev5), _C_LABEL(Xrecurse_xenev5)
	.long _C_LABEL(Xresume_xenev5)
	
#ifndef XEN
.globl _C_LABEL(i8259_stubs)
_C_LABEL(i8259_stubs):
	.long _C_LABEL(Xintr_legacy0), _C_LABEL(Xrecurse_legacy0)
	.long _C_LABEL(Xresume_legacy0)
	.long _C_LABEL(Xintr_legacy1), _C_LABEL(Xrecurse_legacy1)
	.long _C_LABEL(Xresume_legacy1)
	.long _C_LABEL(Xintr_legacy2), _C_LABEL(Xrecurse_legacy2)
	.long _C_LABEL(Xresume_legacy2)
	.long _C_LABEL(Xintr_legacy3), _C_LABEL(Xrecurse_legacy3)
	.long _C_LABEL(Xresume_legacy3)
	.long _C_LABEL(Xintr_legacy4), _C_LABEL(Xrecurse_legacy4)
	.long _C_LABEL(Xresume_legacy4)
	.long _C_LABEL(Xintr_legacy5), _C_LABEL(Xrecurse_legacy5)
	.long _C_LABEL(Xresume_legacy5)
	.long _C_LABEL(Xintr_legacy6), _C_LABEL(Xrecurse_legacy6)
	.long _C_LABEL(Xresume_legacy6)
	.long _C_LABEL(Xintr_legacy7), _C_LABEL(Xrecurse_legacy7)
	.long _C_LABEL(Xresume_legacy7)
	.long _C_LABEL(Xintr_legacy8), _C_LABEL(Xrecurse_legacy8)
	.long _C_LABEL(Xresume_legacy8)
	.long _C_LABEL(Xintr_legacy9), _C_LABEL(Xrecurse_legacy9)
	.long _C_LABEL(Xresume_legacy9)
	.long _C_LABEL(Xintr_legacy10), _C_LABEL(Xrecurse_legacy10)
	.long _C_LABEL(Xresume_legacy10)
	.long _C_LABEL(Xintr_legacy11), _C_LABEL(Xrecurse_legacy11)
	.long _C_LABEL(Xresume_legacy11)
	.long _C_LABEL(Xintr_legacy12), _C_LABEL(Xrecurse_legacy12)
	.long _C_LABEL(Xresume_legacy12)
	.long _C_LABEL(Xintr_legacy13), _C_LABEL(Xrecurse_legacy13)
	.long _C_LABEL(Xresume_legacy13)
	.long _C_LABEL(Xintr_legacy14), _C_LABEL(Xrecurse_legacy14)
	.long _C_LABEL(Xresume_legacy14)
	.long _C_LABEL(Xintr_legacy15), _C_LABEL(Xrecurse_legacy15)
	.long _C_LABEL(Xresume_legacy15)
#endif

#if NIOAPIC > 0
.globl _C_LABEL(ioapic_edge_stubs)
_C_LABEL(ioapic_edge_stubs):
	.long _C_LABEL(Xintr_ioapic_edge0), _C_LABEL(Xrecurse_ioapic_edge0)
	.long _C_LABEL(Xresume_ioapic_edge0)
	.long _C_LABEL(Xintr_ioapic_edge1), _C_LABEL(Xrecurse_ioapic_edge1)
	.long _C_LABEL(Xresume_ioapic_edge1)
	.long _C_LABEL(Xintr_ioapic_edge2), _C_LABEL(Xrecurse_ioapic_edge2)
	.long _C_LABEL(Xresume_ioapic_edge2)
	.long _C_LABEL(Xintr_ioapic_edge3), _C_LABEL(Xrecurse_ioapic_edge3)
	.long _C_LABEL(Xresume_ioapic_edge3)
	.long _C_LABEL(Xintr_ioapic_edge4), _C_LABEL(Xrecurse_ioapic_edge4)
	.long _C_LABEL(Xresume_ioapic_edge4)
	.long _C_LABEL(Xintr_ioapic_edge5), _C_LABEL(Xrecurse_ioapic_edge5)
	.long _C_LABEL(Xresume_ioapic_edge5)
	.long _C_LABEL(Xintr_ioapic_edge6), _C_LABEL(Xrecurse_ioapic_edge6)
	.long _C_LABEL(Xresume_ioapic_edge6)
	.long _C_LABEL(Xintr_ioapic_edge7), _C_LABEL(Xrecurse_ioapic_edge7)
	.long _C_LABEL(Xresume_ioapic_edge7)
	.long _C_LABEL(Xintr_ioapic_edge8), _C_LABEL(Xrecurse_ioapic_edge8)
	.long _C_LABEL(Xresume_ioapic_edge8)
	.long _C_LABEL(Xintr_ioapic_edge9), _C_LABEL(Xrecurse_ioapic_edge9)
	.long _C_LABEL(Xresume_ioapic_edge9)
	.long _C_LABEL(Xintr_ioapic_edge10), _C_LABEL(Xrecurse_ioapic_edge10)
	.long _C_LABEL(Xresume_ioapic_edge10)
	.long _C_LABEL(Xintr_ioapic_edge11), _C_LABEL(Xrecurse_ioapic_edge11)
	.long _C_LABEL(Xresume_ioapic_edge11)
	.long _C_LABEL(Xintr_ioapic_edge12), _C_LABEL(Xrecurse_ioapic_edge12)
	.long _C_LABEL(Xresume_ioapic_edge12)
	.long _C_LABEL(Xintr_ioapic_edge13), _C_LABEL(Xrecurse_ioapic_edge13)
	.long _C_LABEL(Xresume_ioapic_edge13)
	.long _C_LABEL(Xintr_ioapic_edge14), _C_LABEL(Xrecurse_ioapic_edge14)
	.long _C_LABEL(Xresume_ioapic_edge14)
	.long _C_LABEL(Xintr_ioapic_edge15), _C_LABEL(Xrecurse_ioapic_edge15)
	.long _C_LABEL(Xresume_ioapic_edge15)
	.long _C_LABEL(Xintr_ioapic_edge16), _C_LABEL(Xrecurse_ioapic_edge16)
	.long _C_LABEL(Xresume_ioapic_edge16)
	.long _C_LABEL(Xintr_ioapic_edge17), _C_LABEL(Xrecurse_ioapic_edge17)
	.long _C_LABEL(Xresume_ioapic_edge17)
	.long _C_LABEL(Xintr_ioapic_edge18), _C_LABEL(Xrecurse_ioapic_edge18)
	.long _C_LABEL(Xresume_ioapic_edge18)
	.long _C_LABEL(Xintr_ioapic_edge19), _C_LABEL(Xrecurse_ioapic_edge19)
	.long _C_LABEL(Xresume_ioapic_edge19)
	.long _C_LABEL(Xintr_ioapic_edge20), _C_LABEL(Xrecurse_ioapic_edge20)
	.long _C_LABEL(Xresume_ioapic_edge20)
	.long _C_LABEL(Xintr_ioapic_edge21), _C_LABEL(Xrecurse_ioapic_edge21)
	.long _C_LABEL(Xresume_ioapic_edge21)
	.long _C_LABEL(Xintr_ioapic_edge22), _C_LABEL(Xrecurse_ioapic_edge22)
	.long _C_LABEL(Xresume_ioapic_edge22)
	.long _C_LABEL(Xintr_ioapic_edge23), _C_LABEL(Xrecurse_ioapic_edge23)
	.long _C_LABEL(Xresume_ioapic_edge23)
	.long _C_LABEL(Xintr_ioapic_edge24), _C_LABEL(Xrecurse_ioapic_edge24)
	.long _C_LABEL(Xresume_ioapic_edge24)
	.long _C_LABEL(Xintr_ioapic_edge25), _C_LABEL(Xrecurse_ioapic_edge25)
	.long _C_LABEL(Xresume_ioapic_edge25)
	.long _C_LABEL(Xintr_ioapic_edge26), _C_LABEL(Xrecurse_ioapic_edge26)
	.long _C_LABEL(Xresume_ioapic_edge26)
	.long _C_LABEL(Xintr_ioapic_edge27), _C_LABEL(Xrecurse_ioapic_edge27)
	.long _C_LABEL(Xresume_ioapic_edge27)
	.long _C_LABEL(Xintr_ioapic_edge28), _C_LABEL(Xrecurse_ioapic_edge28)
	.long _C_LABEL(Xresume_ioapic_edge28)
	.long _C_LABEL(Xintr_ioapic_edge29), _C_LABEL(Xrecurse_ioapic_edge29)
	.long _C_LABEL(Xresume_ioapic_edge29)
	.long _C_LABEL(Xintr_ioapic_edge30), _C_LABEL(Xrecurse_ioapic_edge30)
	.long _C_LABEL(Xresume_ioapic_edge30)
	.long _C_LABEL(Xintr_ioapic_edge31), _C_LABEL(Xrecurse_ioapic_edge31)
	.long _C_LABEL(Xresume_ioapic_edge31)

.globl _C_LABEL(ioapic_level_stubs)
_C_LABEL(ioapic_level_stubs):
	.long _C_LABEL(Xintr_ioapic_level0), _C_LABEL(Xrecurse_ioapic_level0)
	.long _C_LABEL(Xresume_ioapic_level0)
	.long _C_LABEL(Xintr_ioapic_level1), _C_LABEL(Xrecurse_ioapic_level1)
	.long _C_LABEL(Xresume_ioapic_level1)
	.long _C_LABEL(Xintr_ioapic_level2), _C_LABEL(Xrecurse_ioapic_level2)
	.long _C_LABEL(Xresume_ioapic_level2)
	.long _C_LABEL(Xintr_ioapic_level3), _C_LABEL(Xrecurse_ioapic_level3)
	.long _C_LABEL(Xresume_ioapic_level3)
	.long _C_LABEL(Xintr_ioapic_level4), _C_LABEL(Xrecurse_ioapic_level4)
	.long _C_LABEL(Xresume_ioapic_level4)
	.long _C_LABEL(Xintr_ioapic_level5), _C_LABEL(Xrecurse_ioapic_level5)
	.long _C_LABEL(Xresume_ioapic_level5)
	.long _C_LABEL(Xintr_ioapic_level6), _C_LABEL(Xrecurse_ioapic_level6)
	.long _C_LABEL(Xresume_ioapic_level6)
	.long _C_LABEL(Xintr_ioapic_level7), _C_LABEL(Xrecurse_ioapic_level7)
	.long _C_LABEL(Xresume_ioapic_level7)
	.long _C_LABEL(Xintr_ioapic_level8), _C_LABEL(Xrecurse_ioapic_level8)
	.long _C_LABEL(Xresume_ioapic_level8)
	.long _C_LABEL(Xintr_ioapic_level9), _C_LABEL(Xrecurse_ioapic_level9)
	.long _C_LABEL(Xresume_ioapic_level9)
	.long _C_LABEL(Xintr_ioapic_level10), _C_LABEL(Xrecurse_ioapic_level10)
	.long _C_LABEL(Xresume_ioapic_level10)
	.long _C_LABEL(Xintr_ioapic_level11), _C_LABEL(Xrecurse_ioapic_level11)
	.long _C_LABEL(Xresume_ioapic_level11)
	.long _C_LABEL(Xintr_ioapic_level12), _C_LABEL(Xrecurse_ioapic_level12)
	.long _C_LABEL(Xresume_ioapic_level12)
	.long _C_LABEL(Xintr_ioapic_level13), _C_LABEL(Xrecurse_ioapic_level13)
	.long _C_LABEL(Xresume_ioapic_level13)
	.long _C_LABEL(Xintr_ioapic_level14), _C_LABEL(Xrecurse_ioapic_level14)
	.long _C_LABEL(Xresume_ioapic_level14)
	.long _C_LABEL(Xintr_ioapic_level15), _C_LABEL(Xrecurse_ioapic_level15)
	.long _C_LABEL(Xresume_ioapic_level15)
	.long _C_LABEL(Xintr_ioapic_level16), _C_LABEL(Xrecurse_ioapic_level16)
	.long _C_LABEL(Xresume_ioapic_level16)
	.long _C_LABEL(Xintr_ioapic_level17), _C_LABEL(Xrecurse_ioapic_level17)
	.long _C_LABEL(Xresume_ioapic_level17)
	.long _C_LABEL(Xintr_ioapic_level18), _C_LABEL(Xrecurse_ioapic_level18)
	.long _C_LABEL(Xresume_ioapic_level18)
	.long _C_LABEL(Xintr_ioapic_level19), _C_LABEL(Xrecurse_ioapic_level19)
	.long _C_LABEL(Xresume_ioapic_level19)
	.long _C_LABEL(Xintr_ioapic_level20), _C_LABEL(Xrecurse_ioapic_level20)
	.long _C_LABEL(Xresume_ioapic_level20)
	.long _C_LABEL(Xintr_ioapic_level21), _C_LABEL(Xrecurse_ioapic_level21)
	.long _C_LABEL(Xresume_ioapic_level21)
	.long _C_LABEL(Xintr_ioapic_level22), _C_LABEL(Xrecurse_ioapic_level22)
	.long _C_LABEL(Xresume_ioapic_level22)
	.long _C_LABEL(Xintr_ioapic_level23), _C_LABEL(Xrecurse_ioapic_level23)
	.long _C_LABEL(Xresume_ioapic_level23)
	.long _C_LABEL(Xintr_ioapic_level24), _C_LABEL(Xrecurse_ioapic_level24)
	.long _C_LABEL(Xresume_ioapic_level24)
	.long _C_LABEL(Xintr_ioapic_level25), _C_LABEL(Xrecurse_ioapic_level25)
	.long _C_LABEL(Xresume_ioapic_level25)
	.long _C_LABEL(Xintr_ioapic_level26), _C_LABEL(Xrecurse_ioapic_level26)
	.long _C_LABEL(Xresume_ioapic_level26)
	.long _C_LABEL(Xintr_ioapic_level27), _C_LABEL(Xrecurse_ioapic_level27)
	.long _C_LABEL(Xresume_ioapic_level27)
	.long _C_LABEL(Xintr_ioapic_level28), _C_LABEL(Xrecurse_ioapic_level28)
	.long _C_LABEL(Xresume_ioapic_level28)
	.long _C_LABEL(Xintr_ioapic_level29), _C_LABEL(Xrecurse_ioapic_level29)
	.long _C_LABEL(Xresume_ioapic_level29)
	.long _C_LABEL(Xintr_ioapic_level30), _C_LABEL(Xrecurse_ioapic_level30)
	.long _C_LABEL(Xresume_ioapic_level30)
	.long _C_LABEL(Xintr_ioapic_level31), _C_LABEL(Xrecurse_ioapic_level31)
	.long _C_LABEL(Xresume_ioapic_level31)
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
	movl	CPUVAR(ISOURCES) + SIR_SERIAL * 4, %edi
	addl	$1,IS_EVCNTLO(%edi)
	adcl	$0,IS_EVCNTHI(%edi)
	pushl	$X86_SOFTINTR_SOFTSERIAL
	call	_C_LABEL(softintr_dispatch)
	addl	$4,%esp
#ifdef MULTIPROCESSOR	
	call	_C_LABEL(x86_softintunlock)
#endif
	decl	CPUVAR(IDEPTH)
	jmp	*%esi

IDTVEC(softnet)
	movl	$IPL_SOFTNET, CPUVAR(ILEVEL)
	incl	CPUVAR(IDEPTH)
#ifdef MULTIPROCESSOR	
	call	_C_LABEL(x86_softintlock)
#endif
	movl	CPUVAR(ISOURCES) + SIR_NET * 4, %edi
	addl	$1,IS_EVCNTLO(%edi)
	adcl	$0,IS_EVCNTHI(%edi)

	xorl	%edi,%edi
	xchgl	_C_LABEL(netisr),%edi

	/* XXX Do the legacy netisrs here for now. */
#define DONETISR(s, c) \
	.globl  _C_LABEL(c)	;\
	testl	$(1 << s),%edi	;\
	jz	1f		;\
	call	_C_LABEL(c)	;\
1:
#include <net/netisr_dispatch.h>
	
	pushl	$X86_SOFTINTR_SOFTNET
	call	_C_LABEL(softintr_dispatch)
	addl	$4,%esp
#ifdef MULTIPROCESSOR	
	call	_C_LABEL(x86_softintunlock)	
#endif
	decl	CPUVAR(IDEPTH)
	jmp	*%esi

IDTVEC(softclock)
	movl	$IPL_SOFTCLOCK, CPUVAR(ILEVEL)
	incl	CPUVAR(IDEPTH)
#ifdef MULTIPROCESSOR	
	call	_C_LABEL(x86_softintlock)
#endif
	movl	CPUVAR(ISOURCES) + SIR_CLOCK * 4, %edi
	addl	$1,IS_EVCNTLO(%edi)
	adcl	$0,IS_EVCNTHI(%edi)

	pushl	$X86_SOFTINTR_SOFTCLOCK
	call	_C_LABEL(softintr_dispatch)
	addl	$4,%esp
#ifdef MULTIPROCESSOR	
	call	_C_LABEL(x86_softintunlock)		
#endif
	decl	CPUVAR(IDEPTH)
	jmp	*%esi

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

#define TRAP(a)			pushl $(a) ; jmp _C_LABEL(alltraps)
#define ZTRAP(a)		pushl $0 ; TRAP(a)

#ifdef IPKDB
#define BPTTRAP(a)	pushl $0; pushl $(a); jmp _C_LABEL(bpttraps)
#else
#define BPTTRAP(a)	ZTRAP(a)
#endif


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
#if NNPX > 0
	pushl	$0			# dummy error code
	pushl	$T_DNA
	INTRENTRY
#ifdef XENDEBUG_LOW
	pushl	%esp
#endif
	pushl	CPUVAR(SELF)
	call	*_C_LABEL(npxdna_func)
	addl	$4,%esp
#ifdef XENDEBUG_LOW
	addl	$4,%esp
#endif
	testl	%eax,%eax
	jz	calltrap
	INTRFASTEXIT
#else
	ZTRAP(T_DNA)
#endif
IDTVEC(trap08)
	TRAP(T_DOUBLEFLT)
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
#ifndef XEN
IDTVEC(trap0e)
#ifndef I586_CPU
	TRAP(T_PAGEFLT)
#else
	pushl	$T_PAGEFLT
	INTRENTRY
	testb	$PGEX_U,TF_ERR(%esp)
	jnz	calltrap
	movl	%cr2,%eax
	subl	_C_LABEL(pentium_idt),%eax
	cmpl	$(6*8),%eax
	jne	calltrap
	movb	$T_PRIVINFLT,TF_TRAPNO(%esp)
	jmp	calltrap
#endif
#endif

IDTVEC(intrspurious)
IDTVEC(trap0f)
	/*
	 * The Pentium Pro local APIC may erroneously call this vector for a
	 * default IR7.  Just ignore it.
	 *
	 * (The local APIC does this when CPL is raised while it's on the 
	 * way to delivering an interrupt.. presumably enough has been set 
	 * up that it's inconvenient to abort delivery completely..)
	 */
	iret
	
IDTVEC(trap10)
#if NNPX > 0
	/*
	 * Handle like an interrupt so that we can call npxintr to clear the
	 * error.  It would be better to handle npx interrupts as traps but
	 * this is difficult for nested interrupts.
	 */
	pushl	$0			# dummy error code
	pushl	$T_ASTFLT
	INTRENTRY
	pushl	CPUVAR(ILEVEL)
	pushl	%esp
	incl	_C_LABEL(uvmexp)+V_TRAP
	call	_C_LABEL(npxintr)
	addl	$8,%esp
	INTRFASTEXIT
#else
	ZTRAP(T_ARITHTRAP)
#endif
IDTVEC(trap11)
	TRAP(T_ALIGNFLT)
IDTVEC(trap12)
IDTVEC(trap13)
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
	/* 18 - 31 reserved for future exp */
	ZTRAP(T_RESERVED)

IDTVEC(exceptions)
#ifndef XENDEBUG_LOW
	.long	_C_LABEL(Xtrap00), _C_LABEL(Xtrap01)
	.long	_C_LABEL(Xtrap02), _C_LABEL(Xtrap03)
	.long	_C_LABEL(Xtrap04), _C_LABEL(Xtrap05)
	.long	_C_LABEL(Xtrap06), _C_LABEL(Xtrap07)
	.long	_C_LABEL(Xtrap08), _C_LABEL(Xtrap09)
	.long	_C_LABEL(Xtrap0a), _C_LABEL(Xtrap0b)
	.long	_C_LABEL(Xtrap0c), _C_LABEL(Xtrap0d)
	.long	_C_LABEL(Xtrap0e), _C_LABEL(Xtrap0f)
	.long	_C_LABEL(Xtrap10), _C_LABEL(Xtrap11)
	.long	_C_LABEL(Xtrap12), _C_LABEL(Xtrap13)
	.long	_C_LABEL(Xtrap14), _C_LABEL(Xtrap15)
	.long	_C_LABEL(Xtrap16), _C_LABEL(Xtrap17)
	.long	_C_LABEL(Xtrap18), _C_LABEL(Xtrap19)
	.long	_C_LABEL(Xtrap1a), _C_LABEL(Xtrap1b)
	.long	_C_LABEL(Xtrap1c), _C_LABEL(Xtrap1d)
	.long	_C_LABEL(Xtrap1e), _C_LABEL(Xtrap1f)
#else
	.long	_C_LABEL(divide_error), _C_LABEL(debug)
	.long	_C_LABEL(Xtrap02), _C_LABEL(Xtrap03)	    #int3)
	.long	_C_LABEL(overflow), _C_LABEL(bounds)
	.long	_C_LABEL(invalid_op), _C_LABEL(device_not_available)
	.long	_C_LABEL(double_fault), _C_LABEL(coprocessor_segment_overrun)
	.long	_C_LABEL(invalid_TSS), _C_LABEL(segment_not_present)
	.long	_C_LABEL(stack_segment)
	#.long	_C_LABEL(general_protection)
        .long	_C_LABEL(Xtrap0d)
	#.long	_C_LABEL(page_fault)
        .long	_C_LABEL(Xtrap0e)
	.long	_C_LABEL(spurious_interrupt_bug)
	.long	_C_LABEL(coprocessor_error), _C_LABEL(alignment_check)
	.long	_C_LABEL(machine_check), _C_LABEL(simd_coprocessor_error)
	.long	_C_LABEL(Xtrap14), _C_LABEL(Xtrap15)
	.long	_C_LABEL(Xtrap16), _C_LABEL(Xtrap17)
	.long	_C_LABEL(Xtrap18), _C_LABEL(Xtrap19)
	.long	_C_LABEL(Xtrap1a), _C_LABEL(Xtrap1b)
	.long	_C_LABEL(Xtrap1c), _C_LABEL(Xtrap1d)
	.long	_C_LABEL(Xtrap1e), _C_LABEL(Xtrap1f)
#endif


IDTVEC(tss_trap08)
1:
	str	%ax
	GET_TSS
	movzwl	(%eax),%eax
	GET_TSS
	pushl	$T_DOUBLEFLT
	pushl	%eax
	call	_C_LABEL(trap_tss)
	addl	$12,%esp
	iret
	jmp	1b

/* LINTSTUB: Ignore */
NENTRY(alltraps)
	INTRENTRY
calltrap:
#ifdef DIAGNOSTIC
	movl	CPUVAR(ILEVEL),%ebx
#endif /* DIAGNOSTIC */
	pushl	%esp
	call	_C_LABEL(trap)
	addl	$4,%esp
	testb	$CHK_UPL,TF_CS(%esp)
	jnz	alltraps_checkast
#ifdef VM86
	testl	$PSL_VM,TF_EFLAGS(%esp)
	jz	6f
#else
	jmp	6f
#endif
alltraps_checkast:
	/* Check for ASTs on exit to user mode. */
	CLI(%eax)
	CHECK_ASTPENDING(%eax)
	jz	3f
5:	CLEAR_ASTPENDING(%eax)
	STI(%eax)
	movl	$T_ASTFLT,TF_TRAPNO(%esp)
	pushl	%esp
	call	_C_LABEL(trap)
	addl	$4,%esp
	jmp	alltraps_checkast	/* re-check ASTs */
3:	CHECK_DEFERRED_SWITCH(%eax)
	jnz	9f
6:	STIC(%eax)
    	jz	4f
	call	_C_LABEL(stipending)
	#testl	%eax,%eax		/* XXXcl */
	#jnz	1b
4:
#ifndef DIAGNOSTIC
	INTRFASTEXIT
#else
	cmpl	CPUVAR(ILEVEL),%ebx
	jne	3f
	INTRFASTEXIT
3:	pushl	$4f
	call	_C_LABEL(printf)
	addl	$4,%esp
#ifdef DDB
	int	$3
#endif /* DDB */
	movl	%ebx,CPUVAR(ILEVEL)
	jmp	alltraps_checkast	/* re-check ASTs */
4:	.asciz	"WARNING: SPL NOT LOWERED ON TRAP EXIT\n"
#endif /* DIAGNOSTIC */
9:	STI(%eax)
	call	_C_LABEL(pmap_load)
	jmp	alltraps_checkast	/* re-check ASTs */

/* LINTSTUB: Ignore */
IDTVEC(trap0e)
	INTRENTRY
	movl	TF_TRAPNO(%esp),%eax
	movl	$T_PAGEFLT,TF_TRAPNO(%esp)
#ifdef DIAGNOSTIC
	movl	CPUVAR(ILEVEL),%ebx
#endif /* DIAGNOSTIC */
	#pushl	%esp
	pushl	%eax
	movl	%esp,%eax
	addl	$4,%eax
	pushl	%eax
	call	_C_LABEL(trap)
	addl	$4,%esp
	addl	$4,%esp
	testb	$CHK_UPL,TF_CS(%esp)
	jnz	trap0e_checkast
#ifdef VM86
	testl	$PSL_VM,TF_EFLAGS(%esp)
	jz	6f
#else
	jmp	6f
#endif
trap0e_checkast:
	/* Check for ASTs on exit to user mode. */
	CLI(%eax)
	CHECK_ASTPENDING(%eax)
	jz	3f
5:	CLEAR_ASTPENDING(%eax)
	STI(%eax)
	movl	$T_ASTFLT,TF_TRAPNO(%esp)
	pushl	%esp
	call	_C_LABEL(trap)
	addl	$4,%esp
	jmp	trap0e_checkast		/* re-check ASTs */
3:	CHECK_DEFERRED_SWITCH(%eax)
	jnz	9f
6:	STIC(%eax)
    	jz	4f
	call	_C_LABEL(stipending)
	#testl	%eax,%eax		/* XXXcl */
	#jnz	1b
4:
#ifndef DIAGNOSTIC
	INTRFASTEXIT
#else
	cmpl	CPUVAR(ILEVEL),%ebx
	jne	3f
	INTRFASTEXIT
3:	pushl	$4f
	call	_C_LABEL(printf)
	addl	$4,%esp
#ifdef DDB
	int	$3
#endif /* DDB */
	movl	%ebx,CPUVAR(ILEVEL)
	jmp	trap0e_checkast		/* re-check ASTs */
4:	.asciz	"WARNING: SPL NOT LOWERED ON TRAP EXIT\n"
#endif /* DIAGNOSTIC */
9:	STI(%eax)
	call	_C_LABEL(pmap_load)
	jmp	trap0e_checkast		/* re-check ASTs */

#ifdef IPKDB
/* LINTSTUB: Ignore */
NENTRY(bpttraps)
	INTRENTRY
	call	_C_LABEL(ipkdb_trap_glue)
	testl	%eax,%eax
	jz	calltrap
	INTRFASTEXIT

ipkdbsetup:
	popl	%ecx

	/* Disable write protection: */
	movl	%cr0,%eax
	pushl	%eax
	andl	$~CR0_WP,%eax
	movl	%eax,%cr0

	/* Substitute Protection & Page Fault handlers: */
	movl	_C_LABEL(idt),%edx
	pushl	13*8(%edx)
	pushl	13*8+4(%edx)
	pushl	14*8(%edx)
	pushl	14*8+4(%edx)
	movl	$fault,%eax
	movw	%ax,13*8(%edx)
	movw	%ax,14*8(%edx)
	shrl	$16,%eax
	movw	%ax,13*8+6(%edx)
	movw	%ax,14*8+6(%edx)

	pushl	%ecx
	ret

ipkdbrestore:
	popl	%ecx

	/* Restore Protection & Page Fault handlers: */
	movl	_C_LABEL(idt),%edx
	popl	14*8+4(%edx)
	popl	14*8(%edx)
	popl	13*8+4(%edx)
	popl	13*8(%edx)

	/* Restore write protection: */
	popl	%edx
	movl	%edx,%cr0

	pushl	%ecx
	ret
#endif /* IPKDB */


/*
 * If an error is detected during trap, syscall, or interrupt exit, trap() will
 * change %eip to point to one of these labels.  We clean up the stack, if
 * necessary, and resume as if we were handling a general protection fault.
 * This will cause the process to get a SIGBUS.
 */
/* LINTSTUB: Var: char resume_iret[1]; */
NENTRY(resume_iret)
	ZTRAP(T_PROTFLT)
/* LINTSTUB: Var: char resume_pop_ds[1]; */
NENTRY(resume_pop_ds)
	movl	%es,TF_ES(%esp)
	movl	$GSEL(GDATA_SEL, SEL_KPL),%eax
	movw	%ax,%es
/* LINTSTUB: Var: char resume_pop_es[1]; */
NENTRY(resume_pop_es)
	movl	%fs,TF_FS(%esp)
	movl	$GSEL(GDATA_SEL, SEL_KPL),%eax
	movw	%ax,%fs
/* LINTSTUB: Var: char resume_pop_fs[1]; */
NENTRY(resume_pop_fs)
	movl	%gs,TF_GS(%esp)
	movl	$GSEL(GDATA_SEL, SEL_KPL),%eax
	movw	%ax,%gs
/* LINTSTUB: Var: char resume_pop_gs[1]; */
NENTRY(resume_pop_gs)
	movl	$T_PROTFLT,TF_TRAPNO(%esp)
	jmp	calltrap

#ifdef IPKDB
/* LINTSTUB: Func: int ipkdbfbyte(u_char *c) */
NENTRY(ipkdbfbyte)
	pushl	%ebp
	movl	%esp,%ebp
	call	ipkdbsetup
	movl	8(%ebp),%edx
	movzbl	(%edx),%eax
faultexit:
	call	ipkdbrestore
	popl	%ebp
	ret

/* LINTSTUB: Func: int ipkdbsbyte(u_char *c, int i) */
NENTRY(ipkdbsbyte)
	pushl	%ebp
	movl	%esp,%ebp
	call	ipkdbsetup
	movl	8(%ebp),%edx
	movl	12(%ebp),%eax
	movb	%al,(%edx)
	call	ipkdbrestore
	popl	%ebp
	ret

fault:
	popl	%eax		/* error code */
	movl	$faultexit,%eax
	movl	%eax,(%esp)
	movl	$-1,%eax
	iret
#endif	/* IPKDB */



# A note on the "critical region" in our callback handler.
# We want to avoid stacking callback handlers due to events occurring
# during handling of the last event. To do this, we keep events disabled
# until weve done all processing. HOWEVER, we must enable events before
# popping the stack frame (cant be done atomically) and so it would still
# be possible to get enough handler activations to overflow the stack.
# Although unlikely, bugs of that kind are hard to track down, so wed
# like to avoid the possibility.
# So, on entry to the handler we detect whether we interrupted an
# existing activation in its critical region -- if so, we pop the current
# activation and restart the handler using the previous one.
ENTRY(hypervisor_callback)
	pushl	$0			# dummy error code
	pushl	$T_ASTFLT
	INTRENTRY
        movl TF_EIP(%esp),%eax
        cmpl $scrit,%eax
        jb   11f
        cmpl $ecrit,%eax
        jb   critical_region_fixup
11:     push %esp
        call do_hypervisor_callback
        add  $4,%esp
        movl HYPERVISOR_shared_info,%esi
        xorl %eax,%eax
        movb TF_CS(%esp),%cl
        test $CHK_UPL,%cl          # slow return to ring 2 or 3
        je   safesti
        movl CPUVAR(ILEVEL),%ebx
        jmp  doreti_checkast
safesti:btsl $31,4(%esi)     # reenable event callbacks
scrit:  /**** START OF CRITICAL REGION ****/
        cmpl %eax,(%esi)
        jne  14f              # process more events if necessary...
        INTRFASTEXIT
critiret:
14:     btrl %eax,4(%esi)
        jmp  11b
ecrit:  /**** END OF CRITICAL REGION ****/
# [How we do the fixup]. We want to merge the current stack frame with the
# just-interrupted frame. How we do this depends on where in the critical
# region the interrupted handler was executing, and so how many saved
# registers are in each frame. We do this quickly using the lookup table
# 'critical_fixup_table'. For each byte offset in the critical region, it
# provides the number of bytes which have already been popped from the
# interrupted stack frame. 
critical_region_fixup:
        cmpl	$(critiret-1),%eax	    # eip points to iret?
	jne	1f
	movl	$(TF_PUSHSIZE+0x8),%eax
	jmp	2f
1:	xorl	%eax,%eax
2:
				# %eax contains num bytes popped
        mov  %esp,%esi
        add  %eax,%esi        # %esi points at end of src region
        mov  %esp,%edi
        add  $(TF_PUSHSIZE+0x8+0xC),%edi # %edi points at end of dst region
        mov  %eax,%ecx
        shr  $2,%ecx          # convert words to bytes
        je   16f              # skip loop if nothing to copy
15:     subl $4,%esi          # pre-decrementing copy loop
        subl $4,%edi
        movl (%esi),%eax
        movl %eax,(%edi)
        loop 15b
16:     movl %edi,%esp        # final %edi is top of merged stack
        jmp  11b


# Hypervisor uses this for application faults while it executes.
ENTRY(failsafe_callback)
	pop	%ds
	pop	%es
	pop	%fs
	pop	%gs
	call	_C_LABEL(xen_failsafe_handler)
	iret

#ifdef XENDEBUG_LOW

ES		= 0x20
ORIG_EAX	= 0x24
EIP		= 0x28
CS		= 0x2C

#define SAVE_ALL \
	cld; \
	pushl %es; \
	pushl %ds; \
	pushl %eax; \
	pushl %ebp; \
	pushl %edi; \
	pushl %esi; \
	pushl %edx; \
	pushl %ecx; \
	pushl %ebx; \
	movl $GSEL(GDATA_SEL, SEL_KPL),%edx; \
	movl %edx,%ds; \
	movl %edx,%es;

#define RESTORE_ALL	\
	popl %ebx;	\
	popl %ecx;	\
	popl %edx;	\
	popl %esi;	\
	popl %edi;	\
	popl %ebp;	\
	popl %eax;	\
	popl %ds;	\
	popl %es;	\
	addl $4,%esp;	\
	iret;		\

ret_from_exception:
        movb CS(%esp),%cl
	test $2,%cl          # slow return to ring 2 or 3
	jne  safesti
        RESTORE_ALL


ENTRY(divide_error)
	pushl $0		# no error code
	pushl $do_divide_error
do_exception:
	pushl %ds
	pushl %eax
	xorl %eax,%eax
	pushl %ebp
	pushl %edi
	pushl %esi
	pushl %edx
	decl %eax			# eax = -1
	pushl %ecx
	pushl %ebx
	cld
	movl %es,%ecx
	movl ORIG_EAX(%esp), %esi	# get the error code
	movl ES(%esp), %edi		# get the function address
	movl %eax, ORIG_EAX(%esp)
	movl %ecx, ES(%esp)
	movl %esp,%edx
	pushl %esi			# push the error code
	pushl %edx			# push the pt_regs pointer
	movl $(__KERNEL_DS),%edx
	movl %edx,%ds
	movl %edx,%es
	call *%edi
	addl $8,%esp
	jmp ret_from_exception

ENTRY(coprocessor_error)
	pushl $0
	pushl $do_coprocessor_error
	jmp do_exception

ENTRY(simd_coprocessor_error)
	pushl $0
	pushl $do_simd_coprocessor_error
	jmp do_exception

ENTRY(device_not_available)
        iret

ENTRY(debug)
	pushl $0
	pushl $do_debug
	jmp do_exception

ENTRY(int3)
	pushl $0
	pushl $do_int3
	jmp do_exception

ENTRY(overflow)
	pushl $0
	pushl $do_overflow
	jmp do_exception

ENTRY(bounds)
	pushl $0
	pushl $do_bounds
	jmp do_exception

ENTRY(invalid_op)
	pushl $0
	pushl $do_invalid_op
	jmp do_exception

ENTRY(coprocessor_segment_overrun)
	pushl $0
	pushl $do_coprocessor_segment_overrun
	jmp do_exception

ENTRY(double_fault)
	pushl $do_double_fault
	jmp do_exception

ENTRY(invalid_TSS)
	pushl $do_invalid_TSS
	jmp do_exception

ENTRY(segment_not_present)
	pushl $do_segment_not_present
	jmp do_exception

ENTRY(stack_segment)
	pushl $do_stack_segment
	jmp do_exception

ENTRY(general_protection)
	pushl $do_general_protection
	jmp do_exception

ENTRY(alignment_check)
	pushl $do_alignment_check
	jmp do_exception

# This handler is special, because it gets an extra value on its stack,
# which is the linear faulting address.
ENTRY(page_fault)
	pushl %ds
	pushl %eax
	xorl %eax,%eax
	pushl %ebp
	pushl %edi
	pushl %esi
	pushl %edx
	decl %eax			# eax = -1
	pushl %ecx
	pushl %ebx
	cld
	movl %es,%ecx
	movl ORIG_EAX(%esp), %esi	# get the error code
	movl ES(%esp), %edi		# get the faulting address
	movl %eax, ORIG_EAX(%esp)
	movl %ecx, ES(%esp)
	movl %esp,%edx
        pushl %edi                      # push the faulting address
	pushl %esi			# push the error code
	pushl %edx			# push the pt_regs pointer
	movl $(__KERNEL_DS),%edx
	movl %edx,%ds
	movl %edx,%es
	call do_page_fault
	addl $12,%esp
	jmp ret_from_exception

ENTRY(machine_check)
	pushl $0
	pushl $do_machine_check
	jmp do_exception

ENTRY(spurious_interrupt_bug)
	pushl $0
	pushl $do_spurious_interrupt_bug
	jmp do_exception
#endif
