/*	$NetBSD: intr.h,v 1.2 1997/04/14 06:25:32 scottr Exp $	*/

/*
 * Copyright (C) 1997 Scott Reynolds
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Scott Reynolds for
 *      the NetBSD Project.
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

#ifndef _MAC68K_INTR_H_
#define _MAC68K_INTR_H_

#ifdef _KERNEL
/*
 * spl functions; all but spl0 are done in-line
 */

#define _spl(s)								\
({									\
        register int _spl_r;						\
									\
        __asm __volatile ("clrl %0; movew sr,%0; movew %1,sr" :		\
                "&=d" (_spl_r) : "di" (s));				\
        _spl_r;								\
})

#define _splraise(s)							\
({									\
	register int _spl_r;						\
									\
	__asm __volatile ("clrl %0; movew sr,%0;" : "&=d" (_spl_r) : );	\
	if ((_spl_r & PSL_IPL) < ((s) & PSL_IPL))			\
		__asm __volatile ("movew %0,sr;" : : "di" (s));		\
	_spl_r;								\
})

/* spl0 requires checking for software interrupts */
#define spl1()  _spl(PSL_S|PSL_IPL1)
#define spl2()  _spl(PSL_S|PSL_IPL2)
#define spl3()  _spl(PSL_S|PSL_IPL3)
#define spl4()  _spl(PSL_S|PSL_IPL4)
#define spl5()  _spl(PSL_S|PSL_IPL5)
#define spl6()  _spl(PSL_S|PSL_IPL6)
#define spl7()  _spl(PSL_S|PSL_IPL7)

/*
 * These should be used for:
 * 1) ensuring mutual exclusion (why use processor level?)
 * 2) allowing faster devices to take priority
 *
 * Note that on the Mac, most things are masked at spl1, almost
 * everything at spl2, and everything but the panic switch and
 * power at spl4.
 */
#define	splsoftclock()	spl1()	/* disallow softclock */
#define	splsoftnet()	spl1()	/* disallow network */
#define	spltty()	spl1()	/* disallow tty (softserial & ADB) */
#define	splbio()	spl2()	/* disallow block I/O */
#define	splnet()	spl2()	/* disallow network */
#define	splimp()	spl2()	/* mutual exclusion for memory allocation */
#define	splclock()	spl2()	/* disallow clock (and other) interrupts */
#define	splstatclock()	spl2()	/* ditto */
#define	splzs()		spl4()	/* disallow serial hw interrupts */
#define	spladb()	spl7()	/* disallow adb interrupts */
#define	splhigh()	spl7()	/* disallow everything */
#define	splsched()	spl7()	/* disallow scheduling */

/* watch out for side effects */
#define splx(s)         ((s) & PSL_IPL ? _spl(s) : spl0())

/*
 * simulated software interrupt register
 */
extern volatile u_int8_t ssir;

#define	SIR_NET		0x01
#define	SIR_CLOCK	0x02
#define	SIR_SERIAL	0x04

#define	siron(mask)	\
	__asm __volatile ( "orb %0,_ssir" : : "i" (mask))
#define	siroff(mask)	\
	__asm __volatile ( "andb %0,_ssir" : : "ir" (~(mask)));

#define	setsoftnet()	siron(SIR_NET)
#define	setsoftclock()	siron(SIR_CLOCK)
#define	setsoftserial()	siron(SIR_SERIAL)

/* locore.s */
int	spl0 __P((void));
#endif /* _KERNEL */

#endif /* _MAC68K_INTR_H_ */
