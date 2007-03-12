/*	$NetBSD: intr.h,v 1.17.2.1 2007/03/12 05:49:43 rmind Exp $	*/

/*
 * Copyright (C) 1997 Scott Reynolds
 * Copyright (C) 1998 Darrin Jewell
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

#ifndef _NEXT68K_INTR_H_
#define _NEXT68K_INTR_H_

#include <machine/psl.h>

/* Probably want to dealwith IPL's here @@@ */

#ifdef _KERNEL

/* spl0 requires checking for software interrupts */

/* watch out for side effects */
#define splx(s)         ((s) & PSL_IPL ? _spl(s) : spl0())

#define splsoft()	splraise1()
#define splsoftnet()    splsoft()
#define splsoftclock()	splsoft()
#define splsoftserial()	splsoft()
#define splbio()        splraise3()
#define splnet()        splraise3()
#define spltty()        splraise3()
#define splserial()     splraise5()
#define splvm()         splraise6()
#define splclock()      splraise3()	/* ??? */
#define splstatclock()  splclock()
#define splhigh()       spl7()
#define splsched()      spl7()
#define spllock()       spl7()

#define spldma()        splraise6()

/****************************************************************/

#define	IPL_NONE	0
#define	IPL_SOFTCLOCK	1
#define	IPL_SOFTNET	2
#define	IPL_SOFTSERIAL	3
#define	IPL_SOFT	4
#define	IPL_BIO		5
#define	IPL_NET		6
#define	IPL_TTY		7
#define	IPL_SERIAL	8
#define	IPL_VM		9
#define	IPL_CLOCK	10
#define	IPL_STATCLOCK	IPL_CLOCK
#define	IPL_HIGH	11
#define	IPL_LOCK	IPL_HIGH
#define	IPL_SCHED	IPL_HIGH
#define	NIPL		12

typedef int ipl_t;
typedef struct {
	uint16_t _psl;
} ipl_cookie_t;

extern const int ipl2psl_table[NIPL];

ipl_cookie_t makeiplcookie(ipl_t);

static inline int
splraiseipl(ipl_cookie_t icookie)
{

	return _splraise(icookie._psl);
}

/****************************************************************/

/* locore.s */
int	spl0(void);

extern volatile u_long *intrstat;
extern volatile u_long *intrmask;
#define INTR_SETMASK(x)		(*intrmask = (x))
#define INTR_ENABLE(x)		(*intrmask |= NEXT_I_BIT(x))
#define INTR_DISABLE(x)		(*intrmask &= (~NEXT_I_BIT(x)))
#define INTR_OCCURRED(x)	(*intrstat & NEXT_I_BIT(x))

#include <m68k/softintr.h>

#endif /* _KERNEL */

#endif /* _NEXT68K_INTR_H_ */
