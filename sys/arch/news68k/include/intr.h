/*	$NetBSD: intr.h,v 1.11 2005/12/24 20:07:20 perry Exp $	*/

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
 *      This product includes software developed by The NetBSD Foundation
 *	Inc. and its contributers.
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

#ifndef _NEWS68K_INTR_H_
#define	_NEWS68K_INTR_H_

#include <machine/psl.h>
#include <m68k/asm_single.h>

#ifdef _KERNEL
/*
 * news68k can handle software interrupts by its own hardware
 * so has no need to check for any simulated interrupts, etc.
 */
#define	spl0()		_spl0()

#define	spllowersoftclock()	spl2()
#define	splsoft()	splraise2()
#define	splsoftclock()	splsoft()
#define	splsoftnet()	splsoft()
#define	splbio()	splraise4()
#define	splnet()	splraise4()
#define	spltty()	splraise5()
#define	splvm()		splraise5()
#define	splserial()     splraise5()
#define	splclock()	splraise6()
#define	splstatclock()	splclock()
#define	splhigh()	spl7()
#define	splsched()	spl7()
#define	spllock()	spl7()

static inline void
splx(int sr)
{

	__asm volatile("movw %0,%%sr" : : "di" (sr));
}

/*
 * simulated software interrupt register
 */
extern u_char ssir;
extern volatile u_char *ctrl_int2;

#define	NSIR		(sizeof(ssir) * 8)
#define	SIR_NET		0
#define	SIR_CLOCK	1
#define	NEXT_SIR	2

#define	siron(x)	single_inst_bset_b((ssir), (x))
#define	siroff(x)	single_inst_bclr_b((ssir), (x))
#define	setsoftint(x)	do {				\
				siron(x);		\
				*ctrl_int2 = 0xff;	\
			} while (0)
#define	setsoftnet()	setsoftint(1 << SIR_NET)
#define	setsoftclock()	setsoftint(1 << SIR_CLOCK)

u_char allocate_sir(void (*)(void *), void *);
void init_sir(void);
#endif /* _KERNEL */

#endif /* _NEWS68K_INTR_H_ */
