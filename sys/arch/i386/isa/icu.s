/*-
 * Copyright (c) 1993 Charles Hannum.
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
 *	notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *	notice, this list of conditions and the following disclaimer in the
 *	documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *	must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *	may be used to endorse or promote products derived from this software
 *	without specific prior written permission.
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
 *	$Id: icu.s,v 1.19.4.14 1993/10/31 23:52:32 mycroft Exp $
 */

/*
 * AT/386
 * Vector interrupt control section
 */

#include <net/netisr.h>

/*
 * All spl levels include astmask; this forces cpl to be non-zero when
 * splx()ing from nested spl levels, and thus soft interrupts do not
 * get executed.  Logically, all spl levels are `above' soft interupts.
 */
	.data
	.globl	_cpl
_cpl:
	.long	-1			# current priority (all off)
	.globl  _ipending
_ipending:
	.long   0
	.globl	_imask
_imask:
	.long	0
	.globl	_ttymask
_ttymask:
	.long	0
	.globl	_biomask
_biomask:
	.long	0
	.globl	_netmask
_netmask:
	.long	0
	.globl	_impmask
_impmask:
	.long	0
	.globl	_astmask
_astmask:
	.long	0x80000000

vec:
	.long	INTRLOCAL(vec0), INTRLOCAL(vec1), INTRLOCAL(vec2)
	.long	INTRLOCAL(vec3), INTRLOCAL(vec4), INTRLOCAL(vec5)
	.long	INTRLOCAL(vec6), INTRLOCAL(vec7), INTRLOCAL(vec8)
	.long	INTRLOCAL(vec9), INTRLOCAL(vec10), INTRLOCAL(vec11)
	.long	INTRLOCAL(vec12), INTRLOCAL(vec13), INTRLOCAL(vec14)
	.long	INTRLOCAL(vec15)

#define	FASTSPL(mask) \
	movl	mask,_cpl

#define	FASTSPL_VARMASK(varmask) \
	movl	varmask,%eax ; \
	movl	%eax,_cpl

	.text

	ALIGN_TEXT
INTRLOCAL(unpend_v):
	COUNT_EVENT(_intrcnt_spl, 0)
	bsfl    %eax,%eax               # slow, but not worth optimizing
	btrl    %eax,_ipending
	jnc     INTRLOCAL(unpend_v_next) # some intr cleared the in-memory bit
	movl    _Xresume(,%eax,4),%eax
	jmp     %eax
  
	ALIGN_TEXT
INTRLOCAL(unpend_v_next):
	movl	_cpl,%eax
	movl	%eax,%edx
	notl	%eax
	andl	_ipending,%eax
	je	INTRLOCAL(none_to_unpend)
	jmp	INTRLOCAL(unpend_v)

/*
 * Handle return from interrupt after device handler finishes
 */
	ALIGN_TEXT
doreti:
	COUNT_EVENT(_intrcnt_spl, 1)
	popl	%eax			# get previous priority
/*
 * Now interrupt frame is a trap frame!
 *
 * XXX - setting up the interrupt frame to be almost a stack frame is mostly
 * a waste of time.
 */
	movl	%eax,_cpl
	movl	%eax,%edx
	notl	%eax
	andl	_ipending,%eax
	jnz	INTRLOCAL(unpend_v)
INTRLOCAL(none_to_unpend):
	testl	%edx,%edx		# returning to zero priority?
	jz	2f
	popal				# nope, going to non-zero priority
	popl	%es
	popl	%ds
	addl	$8,%esp
	iret


	.globl	_netisr
	.globl	_sir

#define DONET(s, c, event) ; \
	.globl  c ; \
	btrl	$s,_netisr ; \
	jnc	1f ; \
	call	c ; \
1:

	ALIGN_TEXT
2:
/*
 * XXX - might need extra locking while testing reg copy of netisr, but
 * interrupt routines setting it would not cause any new problems (since we
 * don't loop, fresh bits will not be processed until the next doreti or
 * splnone)
 */
	btrl	$SIR_NET,_sir
	jnc	test_clock
	FASTSPL_VARMASK(_netmask)
	DONET(NETISR_RAW, _rawintr, 5)
#ifdef INET
	DONET(NETISR_IP, _ipintr, 6)
#endif
#ifdef IMP
	DONET(NETISR_IMP, _impintr, 7)
#endif
#ifdef NS
	DONET(NETISR_NS, _nsintr, 8)
#endif
#ifdef ISO
	DONET(NETISR_ISO, _clnlintr, 25)
#endif
	FASTSPL($0)
test_clock:
	btrl	$SIR_CLOCK,_sir
	jnc	test_ast
	COUNT_EVENT(_intrcnt_spl, 9)
	FASTSPL_VARMASK(_astmask)
	call	_softclock
	FASTSPL($0)
test_ast:
	btrl	$SIR_GENERIC,_sir	# signal handling, rescheduling, ...
	jnc	2f
	testb   $SEL_RPL_MASK,TF_CS(%esp)
					# to non-kernel (i.e., user)?
	jz	2f			# nope, leave
	COUNT_EVENT(_intrcnt_spl, 10)
	call	_trap
2:
	COUNT_EVENT(_intrcnt_spl, 11)
	popal
	popl	%es
	popl	%ds
	addl	$8,%esp
	iret

/*
 * Interrupt priority mechanism
 *	-- soft splXX masks with group mechanism (cpl)
 *	-- h/w masks for currently active or unused interrupts (imen)
 *	-- ipending = active interrupts currently masked by cpl
 */

	.globl _splnone
	ALIGN_TEXT
_splnone:
	COUNT_EVENT(_intrcnt_spl, 19)
in_splnone:
	movl	_cpl,%eax
	pushl	%eax
	btrl	$SIR_NET,_sir
	jnc	INTRLOCAL(over_net_stuff_for_splnone)
	FASTSPL_VARMASK(_netmask)
	DONET(NETISR_RAW, _rawintr, 20)
#ifdef INET
	DONET(NETISR_IP, _ipintr, 21)
#endif
#ifdef IMP
	DONET(NETISR_IMP, _impintr, 26)
#endif
#ifdef NS
	DONET(NETISR_NS, _nsintr, 27)
#endif
#ifdef ISO
	DONET(NETISR_ISO, _clnlintr, 28)
#endif
INTRLOCAL(over_net_stuff_for_splnone):
	FASTSPL($0)
	movl	_ipending,%eax
	testl   %eax,%eax
	jne	INTRLOCAL(unpend_V)
	popl	%eax
	ret
	
	.globl _splx
	ALIGN_TEXT
_splx:
	COUNT_EVENT(_intrcnt_spl, 22)
	movl	4(%esp),%eax	# new priority
	testl	%eax,%eax
	jz	in_splnone	# going to "zero level" is special
	COUNT_EVENT(_intrcnt_spl, 23)
	pushl	_cpl
	movl	%eax,_cpl	# set new priority
	notl	%eax
	andl	_ipending,%eax
	jne	INTRLOCAL(unpend_V)
	popl	%eax
	ret

	ALIGN_TEXT
INTRLOCAL(unpend_V):
	COUNT_EVENT(_intrcnt_spl, 24)
	bsfl    %eax,%eax
	btrl    %eax,_ipending
	jnc     INTRLOCAL(unpend_V_next)
/*
 * We would prefer to call the intr handler directly here but that doesn't
 * work for badly behaved handlers that want the interrupt frame.
 *
 *	movl    _Xresume(,%eax,4),%edx
 */
	jmp     *vec(,%eax,4)

	ALIGN_TEXT
INTRLOCAL(unpend_V_next):
	movl	_cpl,%eax
	notl	%eax
	andl	_ipending,%eax
	jne	INTRLOCAL(unpend_V)
	popl	%eax
	ret

#define BUILD_VEC(irq_num) \
	ALIGN_TEXT ; \
INTRLOCAL(vec/**/irq_num): ; \
	int     $(ICU_OFFSET + irq_num) ; \
	jmp	INTRLOCAL(unpend_V_next)

	BUILD_VEC(0)
	BUILD_VEC(1)
	BUILD_VEC(2)
	BUILD_VEC(3)
	BUILD_VEC(4)
	BUILD_VEC(5)
	BUILD_VEC(6)
	BUILD_VEC(7)
	BUILD_VEC(8)
	BUILD_VEC(9)
	BUILD_VEC(10)
	BUILD_VEC(11)
	BUILD_VEC(12)
	BUILD_VEC(13)
	BUILD_VEC(14)
	BUILD_VEC(15)

