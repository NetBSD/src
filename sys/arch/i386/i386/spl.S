/*	$NetBSD: spl.S,v 1.25 2008/01/10 20:14:11 ad Exp $	*/

/*
 * Copyright (c) 1998, 2007 The NetBSD Foundation, Inc.
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

#include <machine/asm.h>
__KERNEL_RCSID(0, "$NetBSD: spl.S,v 1.25 2008/01/10 20:14:11 ad Exp $");

#include "opt_vm86.h"
#include "opt_ddb.h"

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
	movl	CPUVAR(ILEVEL),%eax
	cmpl	%edx,%eax
	ja	1f
	movl	%edx,CPUVAR(ILEVEL)
1:
	ret

/*
 * void spllower(int s);
 *
 * spllower() for i486 and Pentium.  Must be the same size as
 * i686_spllower().  This must use pushf/cli/popf as it is used
 * early in boot where interrupts are disabled via eflags/IE.
 */
ENTRY(spllower)
	movl	4(%esp), %ecx
	cmpl	CPUVAR(ILEVEL), %ecx
	jae	1f
	movl	CPUVAR(IUNMASK)(,%ecx,4), %edx
	pushf
	cli
	testl	CPUVAR(IPENDING), %edx
	jnz	2f
	movl	%ecx, CPUVAR(ILEVEL)
	popf
1:
	ret
2:
	popf
	jmp	_C_LABEL(Xspllower)
	.align	32
LABEL(spllower_end)

/*
 * void	cx8_spllower(int s);
 *
 * spllower() optimized for Pentium Pro and later, which have long
 * pipelines that will be stalled by pushf/cli/popf.  Must be the
 * same size as spllower().  Does not need to restore eflags/IE as
 * is patched in once autoconf is underway.
 *
 * For cmpxchg8b, edx/ecx are the high words and eax/ebx the low.
 *
 * edx : eax = old level / old ipending 
 * ecx : ebx = new level / old ipending
 */
ENTRY(cx8_spllower)
	movl	4(%esp),%ecx
	movl	CPUVAR(ILEVEL),%edx
	cmpl	%edx,%ecx			/* new level is lower? */
	pushl	%ebx
	jae,pn	1f
0:
	movl	CPUVAR(IPENDING),%eax
	testl	%eax,CPUVAR(IUNMASK)(,%ecx,4)	/* deferred interrupts? */
	movl	%eax,%ebx
	jnz,pn	2f
	cmpxchg8b CPUVAR(ISTATE)		/* swap in new ilevel */
	jnz,pn	0b
1:
	popl	%ebx
	ret
2:
	popl	%ebx
LABEL(cx8_spllower_patch)
	jmp	_C_LABEL(Xspllower)
	.align	32
LABEL(cx8_spllower_end)

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
 * splipi() and defers it, lands in here via splx(), and handles
 * a lower-prio one first, which needs to take the kernel lock -->
 * the sending CPU will never see the that CPU accept the IPI
 * (see pmap_tlb_shootnow).
 */
	nop	/* Don't get confused with cx8_spllower_end */

IDTVEC(spllower)
#if defined(DDB) || defined(GPROF)
	pushl	%ebp
	movl	%esp,%ebp
	MCOUNT_ASM
#endif /* defined(DDB) || defined(GPROF) */
	pushl	%ebx
	pushl	%esi
	pushl	%edi
#if defined(DDB) || defined(GPROF)
	movl	8(%ebp),%ebx
#else /* defined(DDB) || defined(GPROF) */
	movl	16(%esp),%ebx
#endif /* defined(DDB) || defined(GPROF) */
	movl	$.Lspllower_resume,%esi		# address to resume loop at
	cli
.Lspllower_resume:
#if defined(DEBUG)
	pushf
	popl	%eax
	testl	$PSL_I,%eax
	jnz	.Lspllower_panic
#endif /* defined(DEBUG) */
	movl	%ebx,%eax		# get cpl
	movl	CPUVAR(IUNMASK)(,%eax,4),%eax
	andl	CPUVAR(IPENDING),%eax		# any non-masked bits left?
	jz	2f
	bsrl	%eax,%eax
	btrl	%eax,CPUVAR(IPENDING)
	movl	CPUVAR(ISOURCES)(,%eax,4),%eax
	jmp	*IS_RECURSE(%eax)
2:
	movl	%ebx,CPUVAR(ILEVEL)
	sti
	popl	%edi
	popl	%esi
	popl	%ebx
#if defined(DDB) || defined(GPROF)
	leave
#endif /* defined(DDB) || defined(GPROF) */
	ret
#if defined(DEBUG)
.Lspllower_panic:
	pushl	$1f
	call	_C_LABEL(panic)
1:	.asciz	"SPLLOWER: INTERRUPT ENABLED"
#endif /* defined(DEBUG) */

/*
 * Handle return from interrupt after device handler finishes.
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
	popl	%ebx			# get previous priority
	movl	$.Ldoreti_resume,%esi	# address to resume loop at
.Ldoreti_resume:
#if defined(DEBUG)
	pushf
	popl	%eax
	testl	$PSL_I,%eax
	jnz	.Ldoreti_panic
#endif /* defined(DEBUG) */
	movl	%ebx,%eax
	movl	CPUVAR(IUNMASK)(,%eax,4),%eax
	andl	CPUVAR(IPENDING),%eax
	jz	2f
	bsrl    %eax,%eax               # slow, but not worth optimizing
	btrl    %eax,CPUVAR(IPENDING)
	movl	CPUVAR(ISOURCES)(,%eax, 4),%eax
	jmp	*IS_RESUME(%eax)
2:	/* Check for ASTs on exit to user mode. */
	movl	%ebx,CPUVAR(ILEVEL)
5:
	testb   $SEL_RPL,TF_CS(%esp)
	jnz	.Ldoreti_checkast
#ifdef VM86
	testl	$PSL_VM,TF_EFLAGS(%esp)
	jz	6f
#else
	jmp	6f
#endif
.Ldoreti_checkast:
	CHECK_ASTPENDING(%eax)
	jz	3f
	CLEAR_ASTPENDING(%eax)
	sti
	movl	$T_ASTFLT,TF_TRAPNO(%esp)	/* XXX undo later.. */
	/* Pushed T_ASTFLT into tf_trapno on entry. */
	pushl	%esp
	call	_C_LABEL(trap)
	addl	$4,%esp
	cli
	jmp	5b
3:
	CHECK_DEFERRED_SWITCH
	jnz	9f
6:
	INTRFASTEXIT
9:
	sti
	call	_C_LABEL(pmap_load)
	cli
	jmp	.Ldoreti_checkast	/* recheck ASTs */
#if defined(DEBUG)
.Ldoreti_panic:
	pushl	$1f
	call	_C_LABEL(panic)
1:	.asciz	"DORETI: INTERRUPT ENABLED"
#endif /* defined(DEBUG) */

/*
 * softintr()
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
	movl	$IPL_HIGH,CPUVAR(ILEVEL)
	movl	CPUVAR(CURLWP),%esi
	movl	IS_LWP(%eax),%edi	/* switch to handler LWP */
	movl	%edi,CPUVAR(CURLWP)
	movl	L_ADDR(%edi),%edx
	movl	L_ADDR(%esi),%ecx
	movl	%esp,PCB_ESP(%ecx)
	movl	%ebp,PCB_EBP(%ecx)
	movl	PCB_ESP0(%edx),%esp	/* onto new stack */
	sti
	pushl	IS_MAXLEVEL(%eax)	/* ipl to run at */
	pushl	%esi
	call	_C_LABEL(softint_dispatch)/* run handlers */
	addl	$8,%esp
	movl	L_ADDR(%esi),%ecx
	movl	PCB_ESP(%ecx),%esp
	xchgl	%esi,CPUVAR(CURLWP)	/* must be globally visible */
	popl	%edi			/* unwind switchframe */
	popl	%esi
	addl	$8,%esp
	cli
	jmp	*%esi			/* back to splx/doreti */

/*
 * softintr_ret()
 *
 * Trampoline function that gets returned to by cpu_switchto() when
 * an interrupt handler blocks.  On entry:
 *
 *	%eax		prevlwp from cpu_switchto()
 */
NENTRY(softintr_ret)
	incl	CPUVAR(MTX_COUNT)	/* re-adjust after mi_switch */
	movl	$0, L_CTXSWTCH(%eax)	/* %eax from cpu_switchto */
	cli
	jmp	*%esi			/* back to splx/doreti */

/*
 * void softint_trigger(uintptr_t machdep);
 *
 * Software interrupt registration.
 */
NENTRY(softint_trigger)
	movl	4(%esp),%eax
	orl	%eax,CPUVAR(IPENDING)	/* atomic on local cpu */
	ret
