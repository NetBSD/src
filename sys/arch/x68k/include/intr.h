/*	$NetBSD: intr.h,v 1.8 2001/04/13 23:30:07 thorpej Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
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

#ifndef _X68K_INTR_H_
#define	_X68K_INTR_H_

#include <machine/psl.h>

#if defined(_KERNEL) && !defined(_LOCORE)

/* spl0 requires checking for software interrupts */
void	spl0 __P((void));

#define splnone()       spl0()
#define spllowersoftclock()  spl1()  /* disallow softclock */
#define splsoftclock()  splraise1()  /* disallow softclock */
#define splsoftnet()	splraise1()	/* disallow softnet */
#define splnet()        _splraise(PSL_S|PSL_IPL4) /* disallow network */
#define splbio()        _splraise(PSL_S|PSL_IPL3) /* disallow block I/O */
#define spltty()        _splraise(PSL_S|PSL_IPL4) /* disallow tty interrupts */
#define splvm()         _splraise(PSL_S|PSL_IPL4) /* disallow vm */
#define splzs()         splraise5()	/* disallow serial interrupts */
#define splclock()      splraise6()	/* disallow clock interrupt */
#define splstatclock()  splraise6()	/* disallow clock interrupt */
#define splhigh()       spl7()	/* disallow everything */
#define splsched()      spl7()	/* disallow scheduling */
#define spllock()	spl7()	/* disallow scheduling */

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

#endif /* _KERNEL && ! _LOCORE */
#endif
