/*	$NetBSD: spl.S,v 1.58 2023/03/01 08:38:50 riastradh Exp $	*/

/*
 * Copyright (c) 1998, 2007, 2008, 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum and Andrew Doran.
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

#include <machine/asm.h>
__KERNEL_RCSID(0, "$NetBSD: spl.S,v 1.58 2023/03/01 08:38:50 riastradh Exp $");

#include "opt_ddb.h"
#include "opt_spldebug.h"
#include "opt_xen.h"

#include <machine/trap.h>
#include <machine/segments.h>
#include <machine/frameasm.h>

#include "assym.h"

	.text

/*
 * int splraise(int s);
 */
ENTRY(splraise)
	movl	4(%esp),%edx
	movzbl	CPUVAR(ILEVEL),%eax
	cmpl	%edx,%eax
	ja	1f
	movb	%dl,CPUVAR(ILEVEL)
1:
#ifdef SPLDEBUG
	pushl	%ebp
	movl	%esp,%ebp
	pushl	%eax
	pushl	%edx
	call	_C_LABEL(spldebug_raise)
	addl	$4,%esp
	popl	%eax
	popl	%ebp
#endif /* SPLDEBUG */
	ret
END(splraise)

#ifndef XENPV
/*
 * void spllower(int s);
 *
 * spllower() for i486 and Pentium. Must be the same size as cx8_spllower(),
 * that is, 96 bytes. This must use pushf/cli/popf as it is used early in boot
 * where interrupts are disabled via eflags/IE.
 */
ENTRY(spllower)
	HOTPATCH(HP_NAME_SPLLOWER, 96)
#ifdef SPLDEBUG
	movl	4(%esp),%ecx
	pushl	%ebp
	movl	%esp,%ebp
	pushl	%ecx
	call	_C_LABEL(spldebug_lower)
	addl	$4,%esp
	popl	%ebp
#endif /* SPLDEBUG */
	movl	4(%esp),%ecx
	cmpb	CPUVAR(ILEVEL),%cl
	jae	1f
	movl	CPUVAR(IUNMASK)(,%ecx,8),%edx
	movl	CPUVAR(IUNMASK)+4(,%ecx,8),%eax
	PUSHF(%eax)
	CLI(%eax)
	testl	CPUVAR(IPENDING),%edx
	jnz	2f
	testl	CPUVAR(IPENDING)+4,%eax
	jnz	2f
	movb	%cl,CPUVAR(ILEVEL)
	POPF(%eax)
1:
	ret
2:
	popf
	jmp	_C_LABEL(Xspllower)
	.align	32
END(spllower)
#else  /* XENPV */
STRONG_ALIAS(spllower, cx8_spllower)
#endif /* !XENPV */

/*
 * void	cx8_spllower(int s);
 *
 * spllower() optimized for Pentium Pro and later, which have long pipelines
 * that will be stalled by pushf/cli/popf.  Must be the same size as
 * spllower(), ie 96 bytes.  Does not need to restore eflags/IE as is patched
 * in once autoconf is underway.
 *
 * For cmpxchg8b, edx/ecx are the high words and eax/ebx the low.
 *
 * edx : eax = old level + high 24 bit old ipending / low 32 bit old ipending
 * ecx : ebx = new level + high 24 bit old ipending / low 32 bit old ipending
 */
ENTRY(cx8_spllower)
	movl	4(%esp),%ecx
	movzbl	CPUVAR(ILEVEL),%edx
	cmpl	%edx,%ecx			/* new level is lower? */
	jae	1f
	pushl	%ebx
	pushl	%esi
	pushl	%edi
	movl	%ecx,%esi
	movl	%ecx,%edi
	shll	$24,%edi
0:
	movl	CPUVAR(IPENDING),%eax
	testl	%eax,CPUVAR(IUNMASK)(,%esi,8)	/* deferred interrupts? */
	jnz	2f
	movl	CPUVAR(IPENDING)+4,%edx
	testl	%edx,CPUVAR(IUNMASK)+4(,%esi,8)
	jnz	2f
	movl	%eax,%ebx
	movl	%edx,%ecx
	andl	$0x00ffffff,%ecx
	orl	%edi,%ecx
	cmpxchg8b CPUVAR(ISTATE)		/* swap in new ilevel */
	jnz	0b
	popl	%edi
	popl	%esi
	popl	%ebx
1:
	ret
2:
	popl	%edi
	popl	%esi
	popl	%ebx

	/* The reference must be absolute, hence the indirect jump. */
	movl	$Xspllower,%eax
	jmp	*%eax

	.align	32, 0xCC
LABEL(cx8_spllower_end)
END(cx8_spllower)

/*
 * void Xspllower(int s);
 *
 * Process pending interrupts.
 *
 * Important registers:
 *   ebx - cpl
 *   esi - address to resume loop at
 *   edi - scratch for Xsoftnet
 *
 * It is important that the bit scan instruction is bsr, it will get
 * the highest 2 bits (currently the IPI and clock handlers) first,
 * to avoid deadlocks where one CPU sends an IPI, another one is at
 * splhigh() and defers it, lands in here via splx(), and handles
 * a lower-prio one first, which needs to take the kernel lock -->
 * the sending CPU will never see the that CPU accept the IPI
 * (see pmap_tlb_shootnow).
 */
	nop	/* Don't get confused with cx8_spllower_end */

IDTVEC(spllower)
	pushl	%ebp
	movl	%esp,%ebp
	MCOUNT_ASM
	pushl	%ebx
	pushl	%esi
	pushl	%edi
	movl	8(%ebp),%ebx
	movl	$.Lspllower_resume,%esi		/* address to resume loop at */
1:
	/*
	 * Because of the way Xen interrupts work *%esi will in fact be called
	 * from Xdoreti via iret. So we have to always disable interrupts here
	 * for Xen.
	 */
#ifndef XENPV
	CLI(%eax)
#endif
.Lspllower_resume:
#ifdef XENPV
	CLI(%eax)
#endif
#if defined(DEBUG)
#ifndef XENPV
	pushf
	popl	%eax
	testl	$PSL_I,%eax
	jnz	.Lspllower_panic
#else
	movl    CPUVAR(VCPU),%eax
	movb	EVTCHN_UPCALL_MASK(%eax),%al
	andb	%al,%al
	jz	.Lspllower_panic
#endif /* XENPV */
#endif /* defined(DEBUG) */
	movl	%ebx,%eax			/* get cpl */
	movl	CPUVAR(IUNMASK)+4(,%eax,8),%eax
	andl	CPUVAR(IPENDING)+4,%eax		/* any non-masked bits left? */
	jz	10f
	bsrl	%eax,%eax
	btrl	%eax,CPUVAR(IPENDING)+4
	addl	$32,%eax
	movl	CPUVAR(ISOURCES)(,%eax,4),%eax
	jmp	*IS_RECURSE(%eax)
10:
	movl	%ebx,%eax			/* get cpl */
	movl	CPUVAR(IUNMASK)(,%eax,8),%eax
	andl	CPUVAR(IPENDING),%eax		/* any non-masked bits left? */
	jz	2f
	bsrl	%eax,%eax
	btrl	%eax,CPUVAR(IPENDING)
	movl	CPUVAR(ISOURCES)(,%eax,4),%eax
	jmp	*IS_RECURSE(%eax)
2:
	movb	%bl,CPUVAR(ILEVEL)
#ifdef XENPV
	STIC(%eax)
	jz 4f
	call	_C_LABEL(stipending)
	testl	%eax,%eax
	jnz	1b
4:
#else
	STI(%eax)
#endif
	popl	%edi
	popl	%esi
	popl	%ebx
	leave
	ret
#if defined(DEBUG)
.Lspllower_panic:
	pushl	$1f
	call	_C_LABEL(panic)
1:	.asciz	"SPLLOWER: INTERRUPT ENABLED"
#endif
IDTVEC_END(spllower)

/*
 * Xdoreti: Handle return from interrupt after device handler finishes.
 *
 * Important registers:
 *   ebx - cpl to restore
 *   esi - address to resume loop at
 *   edi - scratch for Xsoftnet
 *
 * called with interrupt disabled.
 */
IDTVEC(doreti)
	IDEPTH_DECR
	popl	%ebx			/* get previous priority */
.Ldoreti_resume_stic:
	movl	$.Ldoreti_resume,%esi	/* address to resume loop at */
.Ldoreti_resume:

#if defined(DEBUG)
#ifndef XENPV
	pushf
	popl	%eax
	testl	$PSL_I,%eax
	jnz	.Ldoreti_panic
#else
	movl	CPUVAR(VCPU),%eax
	movb	EVTCHN_UPCALL_MASK(%eax),%al
	andb	%al,%al
	jz	.Ldoreti_panic
#endif /* XENPV */
#endif /* defined(DEBUG) */

	movl	%ebx,%eax
	movl	CPUVAR(IUNMASK)+4(,%eax,8),%eax
	andl	CPUVAR(IPENDING)+4,%eax
	jz	10f
	bsrl	%eax,%eax		/* slow, but not worth optimizing */
	btrl	%eax,CPUVAR(IPENDING)+4
	addl	$32,%eax
	movl	CPUVAR(ISOURCES)(,%eax, 4),%eax
	jmp	*IS_RESUME(%eax)
10:
	movl	%ebx,%eax
	movl	CPUVAR(IUNMASK)(,%eax,8),%eax
	andl	CPUVAR(IPENDING),%eax
	jz	2f
	bsrl	%eax,%eax		/* slow, but not worth optimizing */
	btrl	%eax,CPUVAR(IPENDING)
	movl	CPUVAR(ISOURCES)(,%eax, 4),%eax
	jmp	*IS_RESUME(%eax)
2:	/* Check for ASTs on exit to user mode. */
	movb	%bl,CPUVAR(ILEVEL)
5:
	testb	$CHK_UPL,TF_CS(%esp)
	jnz	doreti_checkast
	jmp	6f

	.type	_C_LABEL(doreti_checkast), @function
LABEL(doreti_checkast)
	CHECK_ASTPENDING(%eax)
	jz	3f
	CLEAR_ASTPENDING(%eax)
	STI(%eax)
	movl	$T_ASTFLT,TF_TRAPNO(%esp)	/* XXX undo later.. */
	/* Pushed T_ASTFLT into tf_trapno on entry. */
	pushl	%esp
	call	_C_LABEL(trap)
	addl	$4,%esp
	CLI(%eax)
	jmp	5b
END(doreti_checkast)

3:
	CHECK_DEFERRED_SWITCH
	jnz	9f
	HANDLE_DEFERRED_FPU
6:
#ifdef XENPV
	STIC(%eax)
	jz	4f
	call	_C_LABEL(stipending)
	testl	%eax,%eax
	jz	4f
	CLI(%eax)
	jmp	.Ldoreti_resume_stic
4:
#endif
	INTRFASTEXIT
9:
	STI(%eax)
	call	_C_LABEL(pmap_load)
	CLI(%eax)
	jmp	doreti_checkast	/* recheck ASTs */

#if defined(DEBUG)
.Ldoreti_panic:
	pushl	$1f
	call	_C_LABEL(panic)
1:	.asciz	"DORETI: INTERRUPT ENABLED"
#endif
IDTVEC_END(doreti)

/*
 * Xsoftintr()
 *
 * Switch to the LWP assigned to handle interrupts from the given
 * source.  We borrow the VM context from the interrupted LWP.
 *
 * On entry:
 *
 *	%eax		intrsource
 *	%esi		address to return to
 */
IDTVEC(softintr)
	pushl	$_C_LABEL(softintr_ret)	/* set up struct switchframe */
	pushl	%ebx
	pushl	%esi
	pushl	%edi
	movb	$IPL_HIGH,CPUVAR(ILEVEL)
	STI(%esi)
	movl	CPUVAR(CURLWP),%esi
	movl	IS_LWP(%eax),%edi	/* switch to handler LWP */
	/*
	 * Simple MOV to set curlwp to softlwp.  See below on ordering
	 * required to restore softlwp like cpu_switchto.
	 *
	 * 1. Don't need store-before-store barrier because x86 is TSO.
	 *
	 * 2. Don't need store-before-load barrier because when we
	 *    enter a softint lwp, it can't be holding any mutexes, so
	 *    it can't release any until after it has acquired them, so
	 *    we need not participate in the protocol with
	 *    mutex_vector_enter barriers here.
	 *
	 * Hence no need for XCHG or barriers around MOV.
	 */
	movl	%edi,CPUVAR(CURLWP)
	movl	L_PCB(%edi),%edx
	movl	L_PCB(%esi),%ecx
	movl	%esp,PCB_ESP(%ecx)
	movl	%ebp,PCB_EBP(%ecx)
	movl	PCB_ESP0(%edx),%esp	/* onto new stack */
	pushl	IS_MAXLEVEL(%eax)	/* ipl to run at */
	pushl	%esi
	call	_C_LABEL(softint_dispatch)/* run handlers */
	addl	$8,%esp
	CLI(%ecx)
	movl	L_PCB(%esi),%ecx
	movl	PCB_ESP(%ecx),%esp

	/*
	 * Use XCHG, not MOV, to coordinate mutex_exit on this CPU with
	 * mutex_vector_enter on another CPU.
	 *
	 * 1. Any prior mutex_exit by the softint must be visible to
	 *    other CPUs before we restore curlwp on this one,
	 *    requiring store-before-store ordering.
	 *
	 *    (This is always guaranteed by the x86 memory model, TSO,
	 *    but other architectures require a explicit barrier before
	 *    the store to ci->ci_curlwp.)
	 *
	 * 2. Restoring curlwp must be visible on all other CPUs before
	 *    any subsequent mutex_exit on this one can even test
	 *    whether there might be waiters, requiring
	 *    store-before-load ordering.
	 *
	 *    (This is the only ordering x86 TSO ever requires any kind
	 *    of barrier for -- in this case, we take advantage of the
	 *    sequential consistency implied by XCHG to obviate the
	 *    need for MFENCE or something.)
	 *
	 * See kern_mutex.c for details -- this is necessary for
	 * adaptive mutexes to detect whether the lwp is on the CPU in
	 * order to safely block without requiring atomic r/m/w in
	 * mutex_exit.  See also cpu_switchto.
	 */
	xchgl	%esi,CPUVAR(CURLWP)	/* restore ci_curlwp */
	popl	%edi			/* unwind switchframe */
	popl	%esi
	addl	$8,%esp
	jmp	*%esi			/* back to splx/doreti */
IDTVEC_END(softintr)

/*
 * softintr_ret()
 *
 * Trampoline function that gets returned to by cpu_switchto() when
 * an interrupt handler blocks.  On entry:
 *
 *	%eax		prevlwp from cpu_switchto()
 */
ENTRY(softintr_ret)
	incl	CPUVAR(MTX_COUNT)	/* re-adjust after mi_switch */
	CLI(%eax)
	jmp	*%esi			/* back to splx/doreti */
END(softintr_ret)

/*
 * void softint_trigger(uintptr_t machdep);
 *
 * Software interrupt registration.
 */
ENTRY(softint_trigger)
	movl	4(%esp),%eax
	orl	%eax,CPUVAR(IPENDING)	/* atomic on local cpu */
	ret
END(softint_trigger)

/*
 * Xrecurse_preempt()
 *
 * Handles preemption interrupts via Xspllower().
 */
IDTVEC(recurse_preempt)
	movb	$IPL_PREEMPT,CPUVAR(ILEVEL)
	STI(%eax)
	pushl	$0
	call	_C_LABEL(kpreempt)
	addl	$4,%esp
	CLI(%eax)
	jmp	*%esi
IDTVEC_END(recurse_preempt)

/*
 * Xresume_preempt()
 *
 * Handles preemption interrupts via Xdoreti().
 */
IDTVEC(resume_preempt)
	movb	$IPL_PREEMPT,CPUVAR(ILEVEL)
	STI(%eax)
	testb	$CHK_UPL,TF_CS(%esp)
	jnz	1f
	movl	TF_EIP(%esp),%eax
	pushl	%eax
	call	_C_LABEL(kpreempt)		/* from kernel */
	addl	$4,%esp
	CLI(%eax)
	jmp	*%esi
1:
	call	_C_LABEL(preempt)		/* from user */
	CLI(%eax)
	jmp	*%esi
IDTVEC_END(resume_preempt)
