/*	$NetBSD: intr.h,v 1.7.2.2 2002/02/11 20:07:43 jdolecek Exp $	*/

/*
 * Copyright (c) 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef	_EVBARM_INTR_H_
#define	_EVBARM_INTR_H_

#ifdef _KERNEL

/* Interrupt priority "levels". */
#define	IPL_NONE	0	/* nothing */
#define	IPL_SOFT	1	/* generic software interrupts */
#define	IPL_SOFTCLOCK	2	/* software clock interrupt */
#define	IPL_SOFTNET	3	/* software network interrupt */
#define	IPL_BIO		4	/* block I/O */
#define	IPL_NET		5	/* network */
#define	IPL_SOFTSERIAL	6	/* software serial interrupt */
#define	IPL_TTY		7	/* terminals */
#define	IPL_IMP		8	/* memory allocation */
#define	IPL_AUDIO	9	/* audio device */
#define	IPL_CLOCK	10	/* clock interrupt */
#define	IPL_STATCLOCK	11	/* statistics clock interrupt */
#define	IPL_HIGH	12	/* everything */
#define	IPL_SERIAL	13	/* serial device */

#define	NIPL		14

/* Interrupt sharing types. */
#define	IST_NONE	0	/* none */
#define	IST_PULSE	1	/* pulsed */
#define	IST_EDGE	2	/* edge-triggered */
#define	IST_LEVEL	3	/* level-triggered */

#ifdef __OLD_INTERRUPT_CODE	/* XXX XXX XXX */

/* Software interrupt priority levels */

#define SOFTIRQ_CLOCK   0
#define SOFTIRQ_NET     1
#define SOFTIRQ_SERIAL  2

#define SOFTIRQ_BIT(x)  (1 << x)

#include <arm/arm32/psl.h>

#else /* ! __OLD_INTERRUPT_CODE */

#define	__NEWINTR	/* enables new hooks in cpu_fork()/cpu_switch() */

#ifndef _LOCORE

#include <sys/device.h>
#include <sys/queue.h>

int	_splraise(int);		/* provided by BSP */
int	_spllower(int);		/* provided by BSP */
void	splx(int);		/* provided by BSP */
void	_setsoftintr(int);	/* provided by BSP */

#define	splhigh()	_splraise(IPL_HIGH)
#define	splsoft()	_splraise(IPL_SOFT)
#define	splsoftclock()	_splraise(IPL_SOFTCLOCK)
#define	splsoftnet()	_splraise(IPL_SOFTNET)
#define	splbio()	_splraise(IPL_BIO)
#define	splnet()	_splraise(IPL_NET)
#define	spltty()	_splraise(IPL_TTY)
#define	splvm()		_splraise(IPL_IMP)
#define	splaudio()	_splraise(IPL_AUDIO)
#define	splclock()	_splraise(IPL_CLOCK)
#define	splstatclock()	_splraise(IPL_STATCLOCK)
#define	splserial()	_splraise(IPL_SERIAL)

#define	spl0()		_spllower(IPL_NONE)
#define	spllowersoftclock() _spllower(IPL_SOFTCLOCK)

#define	splsched()	splhigh()
#define	spllock()	splhigh()

/* Use generic software interrupt support. */
#include <arm/softintr.h>

#endif /* _LOCORE */

#endif /* __OLD_INTERRUPT_CODE */

#endif /* _KERNEL */

#endif	/* _EVBARM_INTR_H_ */
