/*	$NetBSD: intr.h,v 1.2 2000/06/29 15:36:48 soren Exp $	*/

/*
 * Copyright (c) 2000 Soren S. Jorvang
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
 *          This product includes software developed for the
 *          NetBSD Project.  See http://www.netbsd.org/ for
 *          information about NetBSD.
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

#define	IPL_NONE	0	/* Disable only this interrupt. */
#define	IPL_BIO		1	/* Disable block I/O interrupts. */
#define	IPL_NET		2	/* Disable network interrupts. */
#define	IPL_TTY		3	/* Disable terminal interrupts. */
#define	IPL_CLOCK	4	/* Disable clock interrupts. */
#define	IPL_STATCLOCK	5	/* Disable profiling interrupts. */
#ifndef __NO_SOFT_SERIAL_INTERRUPT
#define	IPL_SERIAL	6	/* Disable serial hardware interrupts. */
#endif
#define	IPL_HIGH	7	/* Disable all interrupts. */
#define NIPL		8

/* Interrupt sharing types. */
#define IST_NONE	0	/* none */
#define IST_PULSE	1	/* pulsed */
#define IST_EDGE	2	/* edge-triggered */
#define IST_LEVEL	3	/* level-triggered */

/* Soft interrupt masks. */
#define SIR_CLOCK	31
#define SIR_NET		30
#define SIR_CLOCKMASK	((1 << SIR_CLOCK))
#define SIR_NETMASK	((1 << SIR_NET) | SIR_CLOCKMASK)
#define SIR_ALLMASK	(SIR_CLOCKMASK | SIR_NETMASK)

#ifdef _KERNEL
#ifndef _LOCORE

#include <mips/cpuregs.h>

extern int		_splraise(int);
extern int		_spllower(int); 
extern int		_splset(int);
extern int		_splget(void);
extern void		_splnone(void); 
extern void		_setsoftintr(int);
extern void		_clrsoftintr(int);

#define setsoftclock()	_setsoftintr(MIPS_SOFT_INT_MASK_0)
#define setsoftnet()	_setsoftintr(MIPS_SOFT_INT_MASK_1)
#define clearsoftclock() _clrsoftintr(MIPS_SOFT_INT_MASK_0)
#define clearsoftnet()	_clrsoftintr(MIPS_SOFT_INT_MASK_1)  

extern u_int32_t 	biomask;
extern u_int32_t 	netmask;
extern u_int32_t 	ttymask;
extern u_int32_t 	clockmask;

#define splhigh()       _splraise(MIPS_INT_MASK)
#define spl0()          (void)_spllower(0)
#define splx(s)         (void)_splset(s)
#define splbio()        _splraise(biomask)
#define splnet()        _splraise(netmask)
#define spltty()        _splraise(ttymask)
#define splimp()        spltty()
#define splclock()      _splraise(clockmask)
#define splstatclock()  splclock()
#define spllowersoftclock() _spllower(MIPS_SOFT_INT_MASK_0)
#define splsoftclock()  _splraise(MIPS_SOFT_INT_MASK_0 | MIPS_SOFT_INT_MASK_1)
#define splsoftnet()    _splraise(MIPS_SOFT_INT_MASK_1)

#define spllpt()	spltty()

extern unsigned int	intrcnt[];
#define SOFTCLOCK_INTR	0
#define SOFTNET_INTR	1  

extern void *		cpu_intr_establish(int, int, int (*)(void *), void *);

#endif /* _LOCORE */
#endif /* _KERNEL */
