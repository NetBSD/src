/*	$NetBSD: icu.s,v 1.61 1999/07/01 22:03:38 thorpej Exp $	*/

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

#include "opt_inet.h"
#include "opt_atalk.h"
#include "opt_ccitt.h"
#include "opt_iso.h"
#include "opt_ns.h"
#include "opt_vm86.h"

#include <net/netisr.h>

	.data
	.globl	_C_LABEL(imen),_C_LABEL(cpl),_C_LABEL(ipending)
	.globl	_C_LABEL(astpending),_C_LABEL(netisr)
_C_LABEL(imen):
	.long	0xffff		# interrupt mask enable (all off)

	.text

#if defined(PROF) || defined(GPROF)
	.globl	_C_LABEL(splhigh), _C_LABEL(splx)

	ALIGN_TEXT
_C_LABEL(splhigh):
	movl	$-1,%eax
	xchgl	%eax,_C_LABEL(cpl)
	ret

	ALIGN_TEXT
_C_LABEL(splx):
	movl	4(%esp),%eax
	movl	%eax,_C_LABEL(cpl)
	testl	%eax,%eax
	jnz	_C_LABEL(Xspllower)
	ret
#endif /* PROF || GPROF */
	
/*
 * Process pending interrupts.
 *
 * Important registers:
 *   ebx - cpl
 *   esi - address to resume loop at
 *   edi - scratch for Xsoftnet
 */
IDTVEC(spllower)
	pushl	%ebx
	pushl	%esi
	pushl	%edi
	movl	_C_LABEL(cpl),%ebx	# save priority
	movl	$1f,%esi		# address to resume loop at
1:	movl	%ebx,%eax
	notl	%eax
	andl	_C_LABEL(ipending),%eax
	jz	2f
	bsfl	%eax,%eax
	btrl	%eax,_C_LABEL(ipending)
	jnc	1b
	jmp	*_C_LABEL(Xrecurse)(,%eax,4)
2:	popl	%edi
	popl	%esi
	popl	%ebx
	ret

/*
 * Handle return from interrupt after device handler finishes.
 *
 * Important registers:
 *   ebx - cpl to restore
 *   esi - address to resume loop at
 *   edi - scratch for Xsoftnet
 */
IDTVEC(doreti)
	popl	%ebx			# get previous priority
	movl	%ebx,_C_LABEL(cpl)
	movl	$1f,%esi		# address to resume loop at
1:	movl	%ebx,%eax
	notl	%eax
	andl	_C_LABEL(ipending),%eax
	jz	2f
	bsfl    %eax,%eax               # slow, but not worth optimizing
	btrl    %eax,_C_LABEL(ipending)
	jnc     1b			# some intr cleared the in-memory bit
	jmp	*_C_LABEL(Xresume)(,%eax,4)
2:	/* Check for ASTs on exit to user mode. */
	cli
	cmpb	$0,_C_LABEL(astpending)
	je	3f
	testb   $SEL_RPL,TF_CS(%esp)
#ifdef VM86
	jnz	4f
	testl	$PSL_VM,TF_EFLAGS(%esp)
#endif
	jz	3f
4:	movb	$0,_C_LABEL(astpending)
	sti
	/* Pushed T_ASTFLT into tf_trapno on entry. */
	call	_C_LABEL(trap)
	jmp	2b
3:	INTRFASTEXIT


/*
 * Soft interrupt handlers
 */

IDTVEC(softserial)
	movl	_C_LABEL(imask) + IPL_SOFTSERIAL * 4,%eax
	movl	%eax,_C_LABEL(cpl)
#include "com.h"
#if NCOM > 0
	call	_C_LABEL(comsoft)
#endif
	movl	%ebx,_C_LABEL(cpl)
	jmp	%esi

#define DONET(s, c) \
	.globl  c		;\
	testl	$(1 << s),%edi	;\
	jz	1f		;\
	call	c		;\
1:

IDTVEC(softnet)
	movl	_C_LABEL(imask) + IPL_SOFTNET * 4,%eax
	movl	%eax,_C_LABEL(cpl)
	xorl	%edi,%edi
	xchgl	_C_LABEL(netisr),%edi
#ifdef INET
#include "arp.h"
#if NARP > 0
	DONET(NETISR_ARP, _C_LABEL(arpintr))
#endif
	DONET(NETISR_IP, _C_LABEL(ipintr))
#endif
#ifdef INET6
	DONET(NETISR_IPV6, _C_LABEL(ip6intr))
#endif
#ifdef IMP
	DONET(NETISR_IMP, _C_LABEL(impintr))
#endif
#ifdef NS
	DONET(NETISR_NS, _C_LABEL(nsintr))
#endif
#ifdef ISO
	DONET(NETISR_ISO, _C_LABEL(clnlintr))
#endif
#ifdef CCITT
	DONET(NETISR_CCITT, _C_LABEL(ccittintr))
#endif
#ifdef NATM
	DONET(NETISR_NATM, _C_LABEL(natmintr))
#endif
#ifdef NETATALK
	DONET(NETISR_ATALK, _C_LABEL(atintr))
#endif
#include "ppp.h"
#if NPPP > 0
	DONET(NETISR_PPP, _C_LABEL(pppintr))
#endif
	movl	%ebx,_C_LABEL(cpl)
	jmp	%esi

IDTVEC(softclock)
	movl	_C_LABEL(imask) + IPL_SOFTCLOCK * 4,%eax
	movl	%eax,_C_LABEL(cpl)
	call	_C_LABEL(softclock)
	movl	%ebx,_C_LABEL(cpl)
	jmp	%esi
