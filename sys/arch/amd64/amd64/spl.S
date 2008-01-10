/*	$NetBSD: spl.S,v 1.14.4.2 2008/01/10 23:43:08 bouyer Exp $	*/

/*
 * Copyright (c) 2003 Wasabi Systems, Inc.
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

#include "opt_ddb.h"

#define ALIGN_TEXT	.align 16,0x90

#include <machine/asm.h>
#include <machine/trap.h>
#include <machine/segments.h>
#include <machine/frameasm.h>

#include "assym.h"

	.text

#ifndef XEN
/*
 * softintr()
 *
 * Switch to the LWP assigned to handle interrupts from the given
 * source.  We borrow the VM context from the interrupted LWP.
 *
 * On entry:
 *
 *	%rax		intrsource
 *	%r13		address to return to
 */
IDTVEC(softintr)
	pushq	$_C_LABEL(softintr_ret)	/* set up struct switchframe */
	pushq	%rbx
	pushq	%r12
	pushq	%r13
	pushq	%r14
	pushq	%r15
	movl	$IPL_HIGH,CPUVAR(ILEVEL)
	movq	CPUVAR(CURLWP),%r15
	movq	IS_LWP(%rax),%rdi	/* switch to handler LWP */
	movq	L_ADDR(%rdi),%rdx
	movq	L_ADDR(%r15),%rcx
	movq	%rdi,CPUVAR(CURLWP)
	movq	%rsp,PCB_RSP(%rcx)
	movq	%rbp,PCB_RBP(%rcx)
	movq	PCB_RSP0(%rdx),%rsp	/* onto new stack */
	sti
	movq	%r15,%rdi		/* interrupted LWP */
	movl	IS_MAXLEVEL(%rax),%esi	/* ipl to run at */
	call	_C_LABEL(softint_dispatch)/* run handlers */
	movq	L_ADDR(%r15),%rcx
	movq	PCB_RSP(%rcx),%rsp
	xchgq	%r15,CPUVAR(CURLWP)	/* must be globally visible */
	popq	%r15			/* unwind switchframe */
	addq	$(5 * 8),%rsp
	cli
	jmp	*%r13			/* back to splx/doreti */

/*
 * softintr_ret()
 *
 * Trampoline function that gets returned to by cpu_switchto() when
 * an interrupt handler blocks.  On entry:
 *
 *	%rax		prevlwp from cpu_switchto()
 */
NENTRY(softintr_ret)
	incl	CPUVAR(MTX_COUNT)	/* re-adjust after mi_switch */
	movl	$0, L_CTXSWTCH(%rax)	/* %rax from cpu_switchto */
	cli
	jmp	*%r13			/* back to splx/doreti */

/*
 * void softint_trigger(uintptr_t machdep);
 *
 * Software interrupt registration.
 */
NENTRY(softint_trigger)
	orl	%edi,CPUVAR(IPENDING)	/* atomic on local cpu */
	ret

/*
 * int splraise(int s);
 */
ENTRY(splraise)
	movl	CPUVAR(ILEVEL),%eax
	cmpl	%edi,%eax
	cmoval	%eax,%edi
	movl	%edi,CPUVAR(ILEVEL)
	ret

/*
 * void spllower(int s);
 *
 * Must be the same size as i686_spllower().  This must use
 * pushf/cli/popf as it is used early in boot where interrupts
 * are disabled via eflags/IE.
 */
ENTRY(spllower)
	cmpl	CPUVAR(ILEVEL), %edi
	jae	1f
	movl	CPUVAR(IUNMASK)(,%edi,4), %edx
	pushf
	cli
	testl	CPUVAR(IPENDING), %edx
	jnz	2f
	movl	%edi, CPUVAR(ILEVEL)
	popf
1:
	ret
	ret
2:
	popf
	jmp	_C_LABEL(Xspllower)
	nop
	nop
	.align	16
LABEL(spllower_end)

#endif /* !XEN */

/*
 * void	cx8_spllower(int s);
 *
 * For cmpxchg8b, edx/ecx are the high words and eax/ebx the low.
 *
 * edx : eax = old level / old ipending 
 * ecx : ebx = new level / old ipending
 */
ENTRY(cx8_spllower)
	movl	CPUVAR(ILEVEL),%edx
	movq	%rbx,%r8
	cmpl	%edx,%edi			/* new level is lower? */
	jae,pn	1f
0:
	movl	CPUVAR(IPENDING),%eax
	movl	%edi,%ecx
	testl	%eax,CPUVAR(IUNMASK)(,%rcx,4)/* deferred interrupts? */
	movl	%eax,%ebx
	/*
	 * On the P4 this jump is cheaper than patching in junk
	 * using cmov.  Is cmpxchg expensive if it fails?
	 */
	jnz,pn	2f
	cmpxchg8b CPUVAR(ISTATE)		/* swap in new ilevel */
	jnz,pn	0b
1:
	movq	%r8,%rbx
	ret
2:
	movq	%r8,%rbx
LABEL(cx8_spllower_patch)
	jmp	_C_LABEL(Xspllower)

	.align	16
LABEL(cx8_spllower_end)

/*
 * void Xspllower(int s);
 * 
 * Process pending interrupts.
 *
 * Important registers:
 *   ebx - cpl
 *   r13 - address to resume loop at
 *
 * It is important that the bit scan instruction is bsr, it will get
 * the highest 2 bits (currently the IPI and clock handlers) first,
 * to avoid deadlocks where one CPU sends an IPI, another one is at
 * splipi() and defers it, lands in here via splx(), and handles
 * a lower-prio one first, which needs to take the kernel lock -->
 * the sending CPU will never see the that CPU accept the IPI
 * (see pmap_tlb_shootnow).
 */
	nop
	.align	4	/* Avoid confusion with cx8_spllower_end */

IDTVEC(spllower)
	pushq	%rbx
	pushq	%r13
	pushq	%r12
	movl	%edi,%ebx
	leaq	1f(%rip),%r13		# address to resume loop at
1:	movl	%ebx,%eax		# get cpl
	movl	CPUVAR(IUNMASK)(,%rax,4),%eax
	CLI(si)
	andl	CPUVAR(IPENDING),%eax		# any non-masked bits left?
	jz	2f
	bsrl	%eax,%eax
	btrl	%eax,CPUVAR(IPENDING)
	movq	CPUVAR(ISOURCES)(,%rax,8),%rax
	jmp	*IS_RECURSE(%rax)
2:
	movl	%ebx,CPUVAR(ILEVEL)
	STI(si)
	popq	%r12
	popq	%r13
	popq	%rbx
	ret

/*
 * Handle return from interrupt after device handler finishes.
 *
 * Important registers:
 *   ebx - cpl to restore
 *   r13 - address to resume loop at
 */
IDTVEC(doreti)
	popq	%rbx			# get previous priority
	decl	CPUVAR(IDEPTH)
	leaq	1f(%rip),%r13
1:	movl	%ebx,%eax
	movl	CPUVAR(IUNMASK)(,%rax,4),%eax
	CLI(si)
	andl	CPUVAR(IPENDING),%eax
	jz	2f
	bsrl    %eax,%eax               # slow, but not worth optimizing
	btrl    %eax,CPUVAR(IPENDING)
	movq	CPUVAR(ISOURCES)(,%rax, 8),%rax
	jmp	*IS_RESUME(%rax)
2:	/* Check for ASTs on exit to user mode. */
	movl	%ebx,CPUVAR(ILEVEL)
5:
	testb   $SEL_RPL,TF_CS(%rsp)
	jz	6f
	.globl doreti_checkast
doreti_checkast:
	movq	CPUVAR(CURLWP),%r14
	CHECK_ASTPENDING(%r14)
	je	3f
	CLEAR_ASTPENDING(%r14)
	STI(si)
	movl	$T_ASTFLT,TF_TRAPNO(%rsp)	/* XXX undo later.. */
	/* Pushed T_ASTFLT into tf_trapno on entry. */
	movq	%rsp,%rdi
	call	_C_LABEL(trap)
	CLI(si)
	jmp	doreti_checkast
3:
	CHECK_DEFERRED_SWITCH
	jnz	9f
6:
	INTRFASTEXIT
9:
	STI(si)
	call	_C_LABEL(do_pmap_load)
	CLI(si)
	jmp	doreti_checkast		/* recheck ASTs */

#ifdef XEN
NENTRY(call_evtchn_do_event)
	incl	CPUVAR(IDEPTH)
	call    _C_LABEL(evtchn_do_event)
	decl	CPUVAR(IDEPTH)
	ret
#endif
