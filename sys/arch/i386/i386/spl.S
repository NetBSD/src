/*	$NetBSD: spl.S,v 1.15.8.1 2007/07/11 20:00:03 mjf Exp $	*/

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

#include "opt_vm86.h"
#include "opt_ddb.h"
#include <machine/asm.h>
#include <machine/trap.h>
#include <machine/segments.h>
#include <machine/frameasm.h>

#include "assym.h"

	.data
	.globl	_C_LABEL(netisr)
	.text

#if 0
#if defined(PROF) || defined(GPROF)
/*
 * XXXX TODO
 */
	.globl	_C_LABEL(splhigh), _C_LABEL(splx)

	ALIGN_TEXT
_C_LABEL(splhigh):
	movl	$IPL_HIGH,%eax
	xchgl	%eax,CPUVAR(ILEVEL)
	ret

	ALIGN_TEXT
_C_LABEL(splx):
	movl	4(%esp),%ecx
	SPLLOWER(_C_LABEL(Xspllower))
	/* ret */
#endif /* PROF || GPROF */
#endif

/*
 * int splraise(int s);
 */
	.align	32

ENTRY(splraise)
	movl	4(%esp),%edx
	movl	CPUVAR(ILEVEL),%eax
	cmpl	%edx,%eax
	ja	1f
	movl	%edx,CPUVAR(ILEVEL)
1:
	ret

/*
 * void softintr(int sir);
 *
 * Software interrupt registration.
 */
	.align	32

ENTRY(softintr)
	movl	4(%esp),%ecx
	movl	$1,%eax
	shll	%cl,%eax
	orl	%eax,CPUVAR(IPENDING)
	ret

/*
 * void spllower(int s);
 *
 * spllower() for i486 and Pentium.  Must be the same size as
 * i686_spllower().  This must use pushf/cli/popf as it is used
 * early in boot where interrupts are disabled via eflags/IE.
 */
	.align 64

ENTRY(spllower)
	movl	CPUVAR(SELF150), %eax
	movl	4(%esp), %ecx
	cmpl	(CPU_INFO_ILEVEL-0x150)(%eax), %ecx
	jae	1f
	movl	(CPU_INFO_IUNMASK-0x150)(%eax,%ecx,4), %edx
	pushf
	cli
	testl	(CPU_INFO_IPENDING-0x150)(%eax), %edx
	jnz	2f
	movl	%ecx, (CPU_INFO_ILEVEL-0x150)(%eax)
	popf
1:
	ret
2:
	popf
	jmp	_C_LABEL(Xspllower)
	.align	64
LABEL(spllower_end)

/*
 * void	i686_spllower(int s);
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
	.align	64

ENTRY(i686_spllower)
	movl	4(%esp),%ecx
	movl	CPUVAR(ILEVEL),%edx
	cmpl	%edx,%ecx			/* new level is lower? */
	pushl	%ebx
	jae,pn	1f
0:
	movl	CPUVAR(IPENDING),%eax
	testl	%eax,CPUVAR(IUNMASK)(,%ecx,4)	/* deferred interrupts? */
	movl	%eax,%ebx
	/*
	 * On the P4 this jump is cheaper than patching in junk
	 * using cmovnz.  Is cmpxchg expensive if it fails?
	 */
	jnz,pn	2f
	cmpxchg8b CPUVAR(ISTATE)		/* swap in new ilevel */
	jnz,pn	0b
1:
	popl	%ebx
	ret
2:
	popl	%ebx
LABEL(i686_spllower_patch)
	jmp	_C_LABEL(Xspllower)
	.align	64
LABEL(i686_spllower_end)

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
	nop
	.align	4	/* Don't get confused with i686_spllower_end */

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
4:	CLEAR_ASTPENDING(%eax)
	sti
	movl	$T_ASTFLT,TF_TRAPNO(%esp)	/* XXX undo later.. */
	/* Pushed T_ASTFLT into tf_trapno on entry. */
	pushl	%esp
	call	_C_LABEL(trap)
	addl	$4,%esp
	cli
	jmp	5b
3:
	CHECK_DEFERRED_SWITCH(%eax)
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
