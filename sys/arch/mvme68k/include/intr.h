/*	$NetBSD: intr.h,v 1.2 2000/03/18 22:33:05 scw Exp $	*/

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe and Steve C. Woodford.
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

#ifndef _MVME68K_INTR_H
#define _MVME68K_INTR_H

#include <machine/psl.h>

/*
 * These are identical to the values used by hp300, but are not meaningful
 * to mvme68k code at this time.
 */
#define	IPL_NONE	0	/* disable only this interrupt */
#define	IPL_BIO		1	/* disable block I/O interrupts */
#define	IPL_NET		2	/* disable network interrupts */
#define	IPL_TTY		3	/* disable terminal interrupts */
#define	IPL_TTYNOBUF	4	/* IPL_TTY + higher ISR priority */
#define	IPL_CLOCK	5	/* disable clock interrupts */
#define	IPL_HIGH	6	/* disable all interrupts */

#ifdef _KERNEL
/* spl0 requires checking for software interrupts */

#define spllowersoftclock()	spl1()
#define splsoftclock()		splraise1()
#define splsoftnet()		splraise1()
#define splbio()		splraise2()
#define splnet()		splraise3()
#define spltty()		splraise3()
#define splimp()		splraise3()
#define splserial()		splraise4()
#define splclock()		splraise5()
#define splstatclock()		splraise5()
#define splvm()			splraise5()
#define splhigh()		spl7()
#define splsched()		spl7()

/* watch out for side effects */
#define splx(s)         (s & PSL_IPL ? _spl(s) : spl0())


#define SIR_NET		0x1
#define SIR_CLOCK	0x2

/* Following is from next68k/intr.h */
#define	siron(mask)	\
	__asm __volatile ( "orb %1,%0" : "=m" (ssir) : "ir" (mask))
#define	siroff(mask)	\
	__asm __volatile ( "andb %1,%0" : "=m" (ssir) : "ir" (~(mask)));

#define setsoftint(x)	siron(x)
#define setsoftnet()	siron(SIR_NET)
#define setsoftclock()	siron(SIR_CLOCK)


#ifndef _LOCORE
/*
 * simulated software interrupt register
 */
extern volatile unsigned char ssir;

extern void init_sir __P((void));
extern unsigned long allocate_sir __P((void (*)(void *), void *));
extern int spl0 __P((void));
#endif /* !_LOCORE */
#endif /* _KERNEL */

#endif /* _MVME68K_INTR_H */
