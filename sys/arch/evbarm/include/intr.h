/* 	$NetBSD: intr.h,v 1.1 2001/09/05 04:53:40 matt Exp $	*/

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

#ifndef _EVBARM_INTR_H_
#define _EVBARM_INTR_H_

/* Define the various Interrupt Priority Levels */

/* Interrupt Priority Levels are mutually exclusive. */

#define	IPL_NONE	0	/* no interrupts blocked */
#define	IPL_SOFT	1	/* generic soft interrupts */
#define	IPL_SOFTCLOCK	2	/* clock soft interrupts */
#define	IPL_SOFTNET	3	/* network soft interrupts */
#define	IPL_SOFTSERIAL	4	/* serial soft interrupts */
#define IPL_BIO		5	/* block I/O */
#define IPL_NET		6	/* network */
#define IPL_TTY		7	/* terminal */
#define IPL_IMP		8	/* memory allocation */
#define IPL_AUDIO	9	/* audio */
#define IPL_CLOCK	10	/* clock */
#define IPL_SERIAL	11	/* serial */
#define IPL_PERF	12	/* peformance monitoring unit */
#define IPL_HIGH	13	/* blocks all interrupts */

#define IPL_LEVELS	14

#define	IST_UNUSABLE	-1	/* interrupt cannot be used */
#define	IST_NONE	0	/* none (dummy) */
#define	IST_PULSE	1	/* pulsed */
#define	IST_EDGE	2	/* edge-triggered */
#define	IST_LEVEL	3	/* level-triggered */

#if defined (_KERNEL) && !defined(_LOCORE)
#include <sys/queue.h>

extern int _splraise(int);
extern int _spllower(int);
extern int _splget(int);
extern int _splset(int);
extern int _splnone(void);
extern int _softintrset(int);
extern int _softintrclr(int);

#define	splsoftclock()		_splraise(IPL_SOFTCLOCK)
#define	splsoftnet()		_splraise(IPL_SOFTNET)
#define	splsoftserial()		_splraise(IPL_SOFTSERIAL)
#define	splbio()		_splraise(IPL_BIO)
#define	splnet()		_splraise(IPL_NET)
#define	spltty()		_splraise(IPL_TTY)
#define	splvm()			_splraise(IPL_IMP)
#define	splaudio()		_splraise(IPL_AUDIO)
#define	splclock()		_splraise(IPL_CLOCK)
#define	splserial()		_splraise(IPL_SERIAL)
#define	splhigh()		_splraise(IPL_HIGH)
#define	spl0()			(void) _splnone()
#define	splx(s)			(void) _splset(s)

#define	spllock()		splhigh()
#define	splsched()		splclock()
#define	splstatclock()		splclock()

#define	spllowersoftclock()	_spllower(IPL_SOFTCLOCK)

#define	setsoftclock()		_softintrset(IPL_SOFTCLOCK)
#define	setsoftnet()		_softintrset(IPL_SOFTNET)
#define	setsoftserial()		_softintrset(IPL_SOFTSERIAL)

struct intrsource {
	void *is_cookie;
	LIST_ENTRY(evbarm_intrsource) is_link;
	void *(*is_establish)(void *, int, int, int (*)(void *), void *);
	void (*is_disestablish)(void *, void *);

	void (*is_setmask)(int);
};

#define	intr_establish(src, irq, type, func, arg) \
	(((src)->is_establish)((src)->is_cookie, irq, type, func, arg))
#define	intr_disestablish(src, ih) \
	(((src)->is_disestablish)((src)->is_cookie, ih))
#endif	/* _KERNEL */

#endif	/* _EVBARM_INTR_H */
