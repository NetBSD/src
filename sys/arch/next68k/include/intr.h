/*	$NetBSD: intr.h,v 1.15 2005/12/24 20:07:24 perry Exp $	*/

/*
 * Copyright (C) 1997 Scott Reynolds
 * Copyright (C) 1998 Darrin Jewell
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
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

#ifndef _NEXT68K_INTR_H_
#define _NEXT68K_INTR_H_

#include <machine/psl.h>

/* Probably want to dealwith IPL's here @@@ */

#ifdef _KERNEL

/* spl0 requires checking for software interrupts */

/* watch out for side effects */
#define splx(s)         ((s) & PSL_IPL ? _spl(s) : spl0())

/****************************************************************/

#define splhigh()       spl7()
#define splserial()     _splraise(PSL_S|PSL_IPL5)
#define splsched()      spl7()
#define spllock()	spl7()
#define splclock()      _splraise(PSL_S|PSL_IPL3)
#define splstatclock()  splclock()
#define splvm()         _splraise(PSL_S|PSL_IPL6)
#define spltty()        _splraise(PSL_S|PSL_IPL3)
#define splbio()        _splraise(PSL_S|PSL_IPL3)
#define splnet()        _splraise(PSL_S|PSL_IPL3)
#define splsoftnet()    _splraise(PSL_S|PSL_IPL2)
#define	splsoftclock()	splraise1()
#define spllowersoftclock() spl1()

#define spldma()        _splraise(PSL_S|PSL_IPL6)

/****************************************************************/

/*
 * simulated software interrupt register
 */
extern volatile u_int8_t ssir;

#define	SIR_NET		0x01
#define	SIR_CLOCK	0x02
#define	SIR_SERIAL	0x04
#define SIR_DTMGR	0x08
#define SIR_ADB		0x10

#define	siron(mask)	\
	__asm volatile ( "orb %1,%0" : "=m" (ssir) : "i" (mask))
#define	siroff(mask)	\
	__asm volatile ( "andb %1,%0" : "=m" (ssir) : "ir" (~(mask)));

#define	setsoftnet()	siron(SIR_NET)
#define	setsoftclock()	siron(SIR_CLOCK)
#define	setsoftserial()	siron(SIR_SERIAL)
#define	setsoftdtmgr()	siron(SIR_DTMGR)
#define	setsoftadb()	siron(SIR_ADB)

extern u_long allocate_sir(void (*)(void *),void *);
extern void init_sir(void);

/* locore.s */
int	spl0(void);

extern volatile u_long *intrstat;
extern volatile u_long *intrmask;
#define INTR_SETMASK(x)		(*intrmask = (x))
#define INTR_ENABLE(x)		(*intrmask |= NEXT_I_BIT(x))
#define INTR_DISABLE(x)		(*intrmask &= (~NEXT_I_BIT(x)))
#define INTR_OCCURRED(x)	(*intrstat & NEXT_I_BIT(x))

#endif /* _KERNEL */

#endif /* _NEXT68K_INTR_H_ */
