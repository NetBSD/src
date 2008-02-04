/*	$NetBSD: intr.h,v 1.14.10.6 2008/02/04 09:21:53 yamt Exp $	*/

/*-
 * Copyright (c) 1996, 1997, 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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

#ifndef _HP300_INTR_H_
#define	_HP300_INTR_H_

#include <sys/device.h>
#include <sys/queue.h>
#include <machine/psl.h>

/*
 * Interrupt "levels".  These are a more abstract representation
 * of interrupt levels, and do not have the same meaning as m68k
 * CPU interrupt levels.  They serve the following purposes:
 *
 *	- properly order ISRs in the list for that CPU ipl
 *	- compute CPU PSL values for the spl*() calls.
 *	- used to create cookie for the splraiseipl().
 */
#define	IPL_NONE	0
#define	IPL_SOFTCLOCK	1
#define	IPL_SOFTBIO	2
#define	IPL_SOFTNET	3
#define	IPL_SOFTSERIAL	4
#define	IPL_VM		5
#define	IPL_SCHED	6
#define	IPL_HIGH	7
#define	NIPL		8

/*
 * Convert PSL values to m68k CPU IPLs and vice-versa.
 * Note: CPU IPL values are different from IPL_* used by splraiseipl().
 */
#define	PSLTOIPL(x)	(((x) >> 8) & 0xf)
#define	IPLTOPSL(x)	((((x) & 0xf) << 8) | PSL_S)

extern int idepth;
extern u_short hp300_ipl2psl[];

typedef int ipl_t;
typedef struct {
	uint16_t _psl;
} ipl_cookie_t;

static inline ipl_cookie_t
makeiplcookie(ipl_t ipl)
{

	return (ipl_cookie_t){._psl = hp300_ipl2psl[ipl]};
}

static inline int
splraiseipl(ipl_cookie_t icookie)
{

	return _splraise(icookie._psl);
}

/* These spl calls are _not_ to be used by machine-independent code. */
#define	splhil()	splraise1()
#define	splkbd()	splhil()

/* These spl calls are used by machine-independent code. */
/* spl0 requires checking for software interrupts */
#define	splsoftbio()	splraise1()
#define	splsoftclock()	splraise1()
#define	splsoftnet()	splraise1()
#define	splsoftserial()	splraise1()
#define	splvm()		_splraise(hp300_ipl2psl[IPL_VM])
#define	splsched()	spl6()
#define	splhigh()	spl7()

/* watch out for side effects */
#define	splx(s)		((s) & PSL_IPL ? _spl((s)) : spl0())

struct hp300_intrhand {
	LIST_ENTRY(hp300_intrhand) ih_q;
	int (*ih_fn)(void *);
	void *ih_arg;
	int ih_ipl;
	int ih_priority;
};

struct hp300_intr {
	LIST_HEAD(, hp300_intrhand) hi_q;
	struct evcnt hi_evcnt;
};

/* locore.s */
int	spl0(void);

/* intr.c */
void	intr_init(void);
void	*intr_establish(int (*)(void *), void *, int, int);
void	intr_disestablish(void *);
void	intr_dispatch(int);
void	intr_printlevels(void);

#endif /* _HP300_INTR_H_ */
