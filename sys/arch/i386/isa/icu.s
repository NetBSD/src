/*-
 * Copyright (c) 1993, 1994 Charles Hannum.
 * Copyright (c) 1989, 1990 William F. Jolitz.
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	from: @(#)icu.s	7.2 (Berkeley) 5/21/91
 *	$Id: icu.s,v 1.34.2.1 1994/08/14 09:26:07 mycroft Exp $
 */

/*
 * AT/386
 * Vector interrupt control section
 */

#include <net/netisr.h>

	.data
	.globl	_imen,_cpl,_ipending,_astpending,_netisr
_imen:
	.long	0xffff		# interrupt mask enable (all off)

	.text

#if defined(PROF) || defined(GPROF)
	.globl	_splhigh, _splx

	ALIGN_TEXT
_splhigh:
	movl	$-1,%eax
	xchgl	%eax,_cpl
	ret

	ALIGN_TEXT
_splx:
	movl	4(%esp),%eax
	movl	%eax,_cpl
	testl	%eax,%eax
	jnz	_Xspllower
	ret
#endif /* PROF || GPROF */
	
/*
 * Process pending interrupts.
 *
 * Important registers:
 *   ebx - cpl
 *   esi - address to resume loop
 *   edi - scratch for Xsoftnet
 */
ENTRY(spllower)
IDTVEC(spllower)
	pushl	%ebx
	pushl	%esi
	pushl	%edi
	movl	_cpl,%ebx
	movl	$1f,%esi
1:	movl	%ebx,%eax
	notl	%eax
	andl	_ipending,%eax
	jz	2f
	bsfl	%eax,%eax
	btrl	%eax,_ipending
	jnc	1b
	jmp	*_Xrecurse(,%eax,4)
2:	popl	%edi
	popl	%esi
	popl	%ebx
	ret

/*
 * Handle return from interrupt after device handler finishes.
 *
 * Important registers:
 *   ebx - cpl to restore
 *   esi - address to resume loop
 *   edi - scratch for Xsoftnet
 */
IDTVEC(doreti)
	popl	%ebx			# get previous priority
/*
 * Now interrupt frame is a trap frame!
 *
 * XXX
 * Setting up the interrupt frame to be almost a stack frame is mostly a waste
 * of time.
 */
	movl	%ebx,_cpl
	movl	$1f,%esi
1:	movl	%ebx,%eax
	notl	%eax
	andl	_ipending,%eax
	jz	2f
	bsfl    %eax,%eax               # slow, but not worth optimizing
	btrl    %eax,_ipending
	jnc     1b			# some intr cleared the in-memory bit
	jmp	*_Xresume(,%eax,4)
2:	/* Check for ASTs on exit to user mode. */
	cli
	testb   $SEL_RPL_MASK,TF_CS(%esp)
	jz	3f
	btrl	$0,_astpending
	jnc	3f
	sti
	/* Pushed T_ASTFLT into TF_TRAPNO on interrupt entry. */
	call	_trap
3:	INTRFASTEXIT


/*
 * Soft interrupt handlers
 */

IDTVEC(softtty)
	/* XXXX nothing for now */
	jmp	%esi

#define DONET(s, c) \
	.globl  c	;\
	btl	$s,%edi	;\
	jnc	1f	;\
	call	c	;\
1:

IDTVEC(softnet)
	leal	SIR_NETMASK(%ebx),%eax
	movl	%eax,_cpl
	xorl	%edi,%edi
	xchgl	_netisr,%edi
#ifdef INET
#include "ether.h"
#if NETHER > 0
	DONET(NETISR_ARP, _arpintr)
#endif
	DONET(NETISR_IP, _ipintr)
#endif
#ifdef IMP
	DONET(NETISR_IMP, _impintr)
#endif
#ifdef NS
	DONET(NETISR_NS, _nsintr)
#endif
#ifdef ISO
	DONET(NETISR_ISO, _clnlintr)
#endif
#ifdef CCITT
	DONET(NETISR_CCITT, _ccittintr)
#endif
	movl	%ebx,_cpl
	jmp	%esi

IDTVEC(softclock)
	leal	SIR_CLOCKMASK(%ebx),%eax
	movl	%eax,_cpl
	call	_softclock
	movl	%ebx,_cpl
	jmp	%esi
