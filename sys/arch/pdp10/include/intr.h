/*	$NetBSD: intr.h,v 1.1 2003/08/19 10:53:05 ragge Exp $	*/
/*
 * Copyright (c) 2003 Anders Magnusson (ragge@ludd.luth.se).
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
 *    derived from this software without specific prior written permission
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

#ifndef _PDP10_INTR_H_
#define _PDP10_INTR_H_

/*
 * A KL10 has 8 interrupt levels, numbered from 0 (highest) to 7.
 * All devices get their interrupt level assigned by software.
 * The levels here are given as those numbers.
 * The hardware vectors used are 40-57 in EPT.
 */
#define	IPL_HIGH	0	/* In reality cannot be blocked */
#define	IPL_DDB		1	/* Enter DDB here */
#define	IPL_CLOCK	2	/* Interrupt timer */
#define	IPL_VM		3	/* Blocks everything except hardclock */
#define	IPL_NET		3	/* NIA20 */
#define	IPL_BIO		4	/* RH20 */
#define	IPL_TTY		5	/* Actually all frontend devices */
#define	IPL_AUDIO	5	/* Not likely... */
#define	IPL_SOFTSERIAL	6
#define	IPL_SOFTNET	6
#define	IPL_SOFTCLOCK	7	/* Process scheduling */

#define	MAKEIV(ipl) (040 + (ipl)*2)

/*
 * Constants used for altering the interrupt system.
 */
#define	PI_ON		0000200	/* Turn on PI system */
#define	PI_OFF		0000400	/* Turn off PI system */
#define	PI_LVLOFF	0001000	/* Turn off PI level */
#define	PI_LVLON	0002000	/* Turn on PI level */
#define	PI_INIT		0004000	/* Initiate interrupt on level */
#define	PI_CLEAR	0010000	/* Clear interrupt system */
#define	PI_DROP		0020000	/* Drop initiated interrupt on level */

/*
 * IPLs converted to PI bits
 */
#define	IPL2PI(x)	(1 << (7 - (x)))
#define	PI_CLOCK	IPL2PI(IPL_CLOCK)
#define	PI_BIO		IPL2PI(IPL_BIO)
#define	PI_NET		IPL2PI(IPL_NET)
#define	PI_TTY		IPL2PI(IPL_TTY)
#define	PI_SOFTNET	IPL2PI(IPL_SOFTNET)
#define	PI_SOFTCLOCK	IPL2PI(IPL_SOFTCLOCK)
#define	PI_ALL		0177

#ifndef _LOCORE
int splvm(void);
int splnet(void);
int splsched(void);
int splclock(void);
int spllowersoftclock(void);
int splbio(void);
int splstatclock(void);
int splhigh(void);
int splstatclock(void);
int splsoftnet(void);
int spltty(void);
int spl0(void);
int spllock(void);
int splx(int);
#if 0
extern void *softintr_establish(int, void (*)(void *), void *);
void softintr_schedule(void *arg);
#endif
void setsoftclock(void);
void setsoftnet(void);
#endif /* _LOCORE */

#endif	/* _PDP10_INTR_H */
