/*	$NetBSD: intr.h,v 1.19.2.1 2009/05/13 17:16:30 jym Exp $	*/

/*-
 * Copyright (c) 1997, 2007 The NetBSD Foundation, Inc.
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

#ifndef _ATARI_INTR_H_
#define _ATARI_INTR_H_

#define	IPL_NONE	0		    /* disable no interrupts	    */
#define	IPL_SOFTCLOCK	1
#define	IPL_SOFTBIO	2
#define	IPL_SOFTNET	3
#define	IPL_SOFTSERIAL	4
#define	IPL_VM		5
#define	IPL_SCHED	6
#define	IPL_HIGH	7
#define	NIPL		8

#define	IST_UNUSABLE	-1	/* interrupt cannot be used	*/
#define	IST_NONE	0	/* none (dummy)			*/
#define	IST_PULSE	1	/* pulsed			*/
#define	IST_EDGE	2	/* edge-triggered		*/
#define	IST_LEVEL	3	/* level-triggered		*/

/*
 * spl functions; all but spl0 are done in-line
 */
#include <machine/psl.h>

/* spl0 requires checking for software interrupts */

#define splsoftclock()		splraise1()
#define splsoftbio()		splraise1()
#define splsoftnet()		splraise1()
#define splsoftserial()		splraise1()
#define splvm()			splraise4()
#define splsched()		splraise6()
#define splhigh()		spl7()
#define splx(s)			((s) & PSL_IPL ? _spl(s) : spl0())

#ifdef _KERNEL
int spl0(void);

extern const uint16_t ipl2psl_table[NIPL];
extern int idepth;

typedef int ipl_t;
typedef struct {
	uint16_t _psl;
} ipl_cookie_t;

static inline ipl_cookie_t
makeiplcookie(ipl_t ipl)
{

	return (ipl_cookie_t){._psl = ipl2psl_table[ipl]};
}

static inline int
splraiseipl(ipl_cookie_t icookie)
{

	return _splraise(icookie._psl);
}

#endif /* _KERNEL */

#endif /* _ATARI_INTR_H_ */
