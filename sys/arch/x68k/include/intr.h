/*	$NetBSD: intr.h,v 1.1 1998/12/13 15:04:01 minoura Exp $	*/

/*
 *
 * Copyright (c) 1998 NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Minoura Makoto and Jason R. Thorpe.
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
 *      This product includes software developed by Charles D. Cranor and
 *      Washington University.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _X68K_INTR_H_
#define	_X68K_INTR_H_

/*
 * spl functions; all but spl0 are done in-line
 */
#include <machine/psl.h>

#define _spl(s)								\
({									\
	register int _spl_r;						\
									\
	__asm __volatile ("clrl %0; movew sr,%0; movew %1,sr" :		\
	    "&=d" (_spl_r) : "di" (s));					\
	_spl_r;								\
})

#define	_splraise(s)							\
({									\
	int _spl_r;							\
									\
	__asm __volatile ("						\
		clrl	d0					;	\
		movw	sr,d0					;	\
		movl	d0,%0					;	\
		andw	#0x700,d0				;	\
		movw	%1,d1					;	\
		andw	#0x700,d1				;	\
		cmpw	d0,d1					;	\
		jle	1f					;	\
		movw	%1,sr					;	\
	    1:"							:	\
		    "&=d" (_spl_r)				:	\
		    "di" (s)					:	\
		    "d0", "d1");					\
	_spl_r;								\
})

/* spl0 requires checking for software interrupts */
void	spl0 __P((void));
#define spl1()  _spl(PSL_S|PSL_IPL1)
#define spl2()  _spl(PSL_S|PSL_IPL2)
#define spl3()  _spl(PSL_S|PSL_IPL3)
#define spl4()  _spl(PSL_S|PSL_IPL4)
#define spl5()  _spl(PSL_S|PSL_IPL5)
#define spl6()  _spl(PSL_S|PSL_IPL6)
#define spl7()  _spl(PSL_S|PSL_IPL7)

#define splnone()       spl0()
#define splsoftclock()  spl1()  /* disallow softclock */
#define splsoftnet()	spl1()	/* disallow softnet */
#define splnet()        _splraise(PSL_S|PSL_IPL4) /* disallow network */
#define splbio()        _splraise(PSL_S|PSL_IPL3) /* disallow block I/O */
#define splimp()        _splraise(PSL_S|PSL_IPL4) /* disallow imput */
#define spltty()        _splraise(PSL_S|PSL_IPL4) /* disallow tty interrupts */
#define splzs()         spl5()	/* disallow serial interrupts */
#define splclock()      spl6()	/* disallow clock interrupt */
#define splstatclock()  spl6()	/* disallow clock interrupt */
#define splvm()         _splraise(PSL_S|PSL_IPL4) /* disallow virtual memory operations */
#define splhigh()       spl7()	/* disallow everything */
#define splsched()      spl7()	/* disallow scheduling */

/* watch out for side effects */
#define splx(s)         ((s) & PSL_IPL ? _spl(s) : spl0())

/*
 * simulated software interrupt register
 */
extern unsigned char ssir;

#define SIR_NET		0x1
#define SIR_CLOCK	0x2
#define SIR_SERIAL	0x4
#define SIR_KBD		0x8

#define siroff(x)	ssir &= ~(x)
#define setsoftnet()	ssir |= SIR_NET
#define setsoftclock()	ssir |= SIR_CLOCK
#define setsoftserial() ssir |= SIR_SERIAL
#define setsoftkbd()    ssir |= SIR_KBD

#endif
