/*	$NetBSD: intr.h,v 1.27.18.1 2008/01/09 01:47:05 matt Exp $	*/

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

#define	IPL_NONE	0
#define	IPL_SOFTCLOCK	1
#define	IPL_SOFTBIO	2
#define	IPL_SOFTNET	3
#define	IPL_SOFTSERIAL	4
#define	IPL_VM		5
#define	IPL_SCHED	6
#define	IPL_HIGH	7
#define	NIPL		8

/* These spl calls are _not_ to be used by machine-independent code. */
#define	splsoft()	splraise1()
#define	spladb()	_splraise(mac68k_ipls[IPL_ADB])
#define	splzs()		splserial()

/*
 * These should be used for:
 * 1) ensuring mutual exclusion (why use processor level?)
 * 2) allowing faster devices to take priority
 */

/* watch out for side effects */
#define splx(s)         ((s) & PSL_IPL ? _spl(s) : spl0())


typedef int ipl_t;
typedef struct {
	uint16_t _ipl;
} ipl_cookie_t;

static inline ipl_cookie_t
makeiplcookie(ipl_t ipl)
{

	return (ipl_cookie_t){._ipl = ipl};
}

static inline int
splraiseipl(ipl_cookie_t icookie)
{

	return _splraise(mac68k_ipls[icookie._ipl]);
}

#include <sys/spl.h>

/* intr.c */
void	intr_init(void);
void	intr_establish(int (*)(void *), void *, int);
void	intr_disestablish(int);
void	intr_dispatch(int);

/* locore.s */
int	spl0(void);
#endif /* _KERNEL */

#endif /* _MAC68K_INTR_H_ */
