/*	$NetBSD: spl.S,v 1.13 2008/01/09 23:45:34 bouyer Exp $	*/
/*	NetBSD: spl.S,v 1.11 2004/02/20 17:35:01 yamt Exp 	*/

/*
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

#include "opt_vm86.h"
#include "opt_ddb.h"
#include "opt_xen.h"
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
	movl	4(%esp),%eax
	movl	%eax,CPUVAR(ILEVEL)
	testl	%eax,%eax
	jnz	_C_LABEL(Xspllower)
	ret
#endif /* PROF || GPROF */
#endif
	
/*
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
IDTVEC(spllower)
#if defined(DDB) || defined(GPROF)
	pushl	%ebp
	movl	%esp,%ebp
	MCOUNT_ASM
#endif /* defined(DDB) || defined(GPROF) */
	pushl	%ebx
	pushl	%esi
	pushl	%edi
	pushl	%ecx
#if defined(DDB) || defined(GPROF)
	movl	8(%ebp),%ebx
#else /* defined(DDB) || defined(GPROF) */
	movl	20(%esp),%ebx
#endif /* defined(DDB) || defined(GPROF) */
	movl	$.Lspllower_resume,%esi	# address to resume loop at
1:
# because of the way Xen interrupts works *%esi will in fact be called
# from Xdoreti via iret. So we have to always disable interrupts here
# for Xen
.Lspllower_resume:
	CLI(%eax)
	movl	%ebx,%eax		# get cpl
	movl	CPUVAR(IUNMASK)(,%eax,4),%eax
	andl	CPUVAR(IPENDING),%eax	# any non-masked bits left?
	jz	2f
	bsrl	%eax,%eax
	btrl	%eax,CPUVAR(IPENDING)
	movl	CPUVAR(ISOURCES)(,%eax,4),%eax
	jmp	*IS_RECURSE(%eax)
2:
	movl	%ebx,CPUVAR(ILEVEL)
	STIC(%ecx)
    	jz	4f
	call	_C_LABEL(stipending)
	testl	%eax,%eax
	jnz	1b
4:	popl	%ecx
	popl	%edi
	popl	%esi
	popl	%ebx
#if defined(DDB) || defined(GPROF)
	leave
#endif /* defined(DDB) || defined(GPROF) */
	ret

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
.Ldoreti_resume:
	movl	$.Ldoreti_resume,%esi	# address to resume loop at
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
	testb   $CHK_UPL,TF_CS(%esp)
	jnz	doreti_checkast
#ifdef VM86
	testl	$PSL_VM,TF_EFLAGS(%esp)
	jz	6f
#else
	jmp	6f
#endif
	.globl doreti_checkast
doreti_checkast:
	movl	CPUVAR(CURLWP),%eax
	CHECK_ASTPENDING(%eax)
	jz	3f
	CLEAR_ASTPENDING(%eax)
	STI(%ecx)
	movl	$T_ASTFLT,TF_TRAPNO(%esp)	/* XXX undo later.. */
	/* Pushed T_ASTFLT into tf_trapno on entry. */
	pushl	%esp
	call	_C_LABEL(trap)
	addl	$4,%esp
	CLI(%ecx)
	jmp	.Ldoreti_resume
3:
	CHECK_DEFERRED_SWITCH
	jnz	9f
6:	STIC(%ecx)
    	jz	4f
	call	_C_LABEL(stipending)
	testl	%eax,%eax
	CLI(%eax)
	jnz	.Ldoreti_resume
4:
	INTRFASTEXIT
9:
	STI(%ecx)
	call	_C_LABEL(pmap_load)
	CLI(%ecx)
	jmp	doreti_checkast	/* recheck ASTs */

/*
 * void evtchn_do_event(int evtch, struct intrframe *regs)
 */

NENTRY(call_evtchn_do_event)
	IDEPTH_INCR
	/*
	 * IDEPTH_INCR leaves old %esp in %eax.
	 */
	pushl	8(%eax)	/* regs */
	pushl	4(%eax)	/* evtch */
	call	_C_LABEL(evtchn_do_event)
	addl	$8, %esp
	IDEPTH_DECR
	ret
