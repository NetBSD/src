/*	$NetBSD: intr.h,v 1.13.2.2 2007/03/24 14:54:35 yamt Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
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

#ifndef _ATARI_INTR_H_
#define _ATARI_INTR_H_

#define	IPL_NONE	0		    /* disable no interrupts	    */
#define	IPL_SOFTCLOCK	1
#define	IPL_SOFTNET	2
#define	IPL_SOFTSERIAL	3
#define	IPL_SOFT	4
#define	IPL_BIO		5		    /* disable block I/O interrupts */
#define	IPL_NET		6		    /* disable network interrupts   */
#define	IPL_TTY		7		    /* disable terminal interrupts  */
#define	IPL_LPT		IPL_TTY
#define	IPL_VM		8
#define	IPL_SERIAL	9
#define	IPL_CLOCK	10		    /* disable clock interrupts	    */
#define	IPL_STATCLOCK	IPL_CLOCK
#define	IPL_HIGH	11		    /* disable all interrupts	    */
#define	IPL_SCHED	IPL_HIGH
#define	IPL_LOCK	IPL_HIGH
#define	NIPL		12

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

#define splnone()		spl0()

#define splsoft()		splraise1()
#define splsoftclock()		splsoft()
#define splsoftnet()		splsoft()
#define splsoftserial()		splsoft()

#define splbio()		splraise3()
#define splnet()		splraise3()
#define spltty()		splraise4()
#define splvm()			splraise4()

#define spllpt()		spltty()

#define splclock()		splraise6()
#define splstatclock()		splraise6()
#define splhigh()		spl7()
#define splsched()		spl7()
#define spllock()		spl7()

#define splx(s)			((s) & PSL_IPL ? _spl(s) : spl0())

#ifdef _KERNEL
int spl0 __P((void));

typedef int ipl_t;
typedef struct {
	uint16_t _psl;
} ipl_cookie_t;

ipl_cookie_t makeiplcookie(ipl_t);

static inline int
splraiseipl(ipl_cookie_t icookie)
{

	return _splraise(icookie._psl);
}

#include <m68k/softintr.h>

#endif /* _KERNEL */

#endif /* _ATARI_INTR_H_ */
