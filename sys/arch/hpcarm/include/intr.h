/* 	$NetBSD: intr.h,v 1.9 2007/07/01 16:04:57 toshii Exp $	*/

/*
 * Copyright (c) 1997 Mark Brinicombe.
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
 *	This product includes software developed by Mark Brinicombe
 *	for the NetBSD Project.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _HPCARM_INTR_H_
#define _HPCARM_INTR_H_

/* Define the various Interrupt Priority Levels */

/* Hardware Interrupt Priority Levels are not mutually exclusive. */

#define IPL_SOFTCLOCK	0
#define IPL_SOFTNET	1
#define IPL_BIO		2	/* block I/O */
#define IPL_NET		3	/* network */
#define IPL_SOFTSERIAL	4
#define IPL_TTY		5	/* terminal */
#define IPL_VM		6	/* memory allocation */
#define IPL_AUDIO	7	/* audio */
#define IPL_CLOCK	8	/* clock */
#define	IPL_STATCLOCK	9
#define IPL_HIGH	10	/*  */
#define	IPL_SCHED	IPL_HIGH
#define	IPL_LOCK	IPL_HIGH
#define IPL_SERIAL	11	/* serial */
#define IPL_NONE	12

#define NIPL		13

#define	IST_UNUSABLE	-1	/* interrupt cannot be used */
#define	IST_NONE	0	/* none (dummy) */
#define	IST_PULSE	1	/* pulsed */
#define	IST_EDGE	2	/* edge-triggered */
#define	IST_LEVEL	3	/* level-triggered */

/* Software interrupt priority levels */

#define SOFTIRQ_CLOCK	0
#define SOFTIRQ_NET	1
#define SOFTIRQ_SERIAL	2

#define SOFTIRQ_BIT(x)	(1 << x)

#include <machine/irqhandler.h>
#include <arm/arm32/psl.h>

#ifndef _LOCORE
void *softintr_establish(int, void (*)(void *), void *);
void softintr_disestablish(void *);
void softintr_schedule(void *);
void setsoftnet(void);
#endif

#endif	/* _HPCARM_INTR_H */
