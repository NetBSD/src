/*	$NetBSD: intr.h,v 1.23 2005/12/24 20:07:15 perry Exp $	*/

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

#ifndef _MAC68K_INTR_H_
#define _MAC68K_INTR_H_

#include <machine/psl.h>

#ifdef _KERNEL

/* spl0 requires checking for software interrupts */

/*
 * This array contains the appropriate PSL_S|PSL_IPL? values
 * to raise interrupt priority to the requested level.
 */
extern unsigned short mac68k_ipls[];

#define	MAC68K_IPL_NONE		0
#define	MAC68K_IPL_SOFT		1
#define	MAC68K_IPL_BIO		2
#define	MAC68K_IPL_NET		3
#define	MAC68K_IPL_TTY		4
#define	MAC68K_IPL_IMP		5
#define	MAC68K_IPL_AUDIO	6
#define	MAC68K_IPL_SERIAL	7
#define	MAC68K_IPL_ADB		8
#define	MAC68K_IPL_CLOCK	9
#define	MAC68K_IPL_STATCLOCK	10
#define	MAC68K_IPL_SCHED	11
#define	MAC68K_IPL_HIGH		12
#define	MAC68K_NIPLS		13

/* These spl calls are _not_ to be used by machine-independent code. */
#define	spladb()	_splraise(mac68k_ipls[MAC68K_IPL_ADB])
#define	splzs()		splserial()

/*
 * These should be used for:
 * 1) ensuring mutual exclusion (why use processor level?)
 * 2) allowing faster devices to take priority
 */
#define	spllowersoftclock() spl1()

/* watch out for side effects */
#define splx(s)         ((s) & PSL_IPL ? _spl(s) : spl0())

#define	IPL_NONE	MAC68K_IPL_NONE
#define	IPL_SOFTCLOCK	MAC68K_IPL_SOFT
#define	IPL_SOFTNET	MAC68K_IPL_SOFT
#define	IPL_BIO		MAC68K_IPL_BIO
#define	IPL_NET		MAC68K_IPL_NET
#define	IPL_TTY		MAC68K_IPL_TTY
#define	IPL_VM		MAC68K_IPL_IMP
#define	IPL_AUDIO	MAC68K_IPL_AUDIO
#define	IPL_CLOCK	MAC68K_IPL_CLOCK
#define	IPL_STATCLOCK	MAC68K_IPL_STATCLOCK
#define	IPL_SCHED	MAC68K_IPL_SCHED
#define	IPL_HIGH	MAC68K_IPL_HIGH
#define	IPL_LOCK	MAC68K_IPL_HIGH
#define	IPL_SERIAL	MAC68K_IPL_SERIAL

#define	splraiseipl(x)	_splraise(mac68k_ipls[x])

#include <sys/spl.h>

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

/* intr.c */
void	intr_init(void);
void	intr_establish(int (*)(void *), void *, int);
void	intr_disestablish(int);
void	intr_dispatch(int);

/* locore.s */
int	spl0(void);
#endif /* _KERNEL */

#endif /* _MAC68K_INTR_H_ */
