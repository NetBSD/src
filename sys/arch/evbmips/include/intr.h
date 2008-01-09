/*	$NetBSD: intr.h,v 1.12.10.2 2008/01/09 01:45:50 matt Exp $	*/

/*-
 * Copyright (c) 2000, 2001 The NetBSD Foundation, Inc.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

#ifndef _EVBMIPS_INTR_H_
#define	_EVBMIPS_INTR_H_

#include <sys/queue.h>

#define	IPL_NONE	0	/* disable only this interrupt */
#define	IPL_SOFTCLOCK	1	/* software interrupts */
#define	IPL_SOFTBIO	1	/* software interrupts */
#define	IPL_SOFTNET	2	/* software interrupts */
#define	IPL_SOFTSERIAL	2	/* software interrupts */
#define	IPL_VM		3
#define	IPL_SCHED	4
#define	IPL_HIGH	4

#define	_IPL_N		5	/* max IPL + 1 */

#define	_IPL_SI0_FIRST	IPL_SOFTCLOCK
#define	_IPL_SI0_LAST	IPL_SOFTBIO

#define	_IPL_SI1_FIRST	IPL_SOFTNET
#define	_IPL_SI1_LAST	IPL_SOFTSERIAL

#define	IST_UNUSABLE	-1	/* interrupt cannot be used */
#define	IST_NONE	0	/* none (dummy) */
#define	IST_PULSE	1	/* pulsed */
#define	IST_EDGE	2	/* edge-triggered */
#define	IST_LEVEL	3	/* level-triggered */
#define	IST_LEVEL_HIGH	4	/* level triggered, active high */
#define	IST_LEVEL_LOW	5       /* level triggered, active low */

#ifdef	_KERNEL

#include <mips/locore.h>

extern const uint32_t ipl_sr_bits[_IPL_N];

#define	spl0()		(void) _spllower(0)
#define	splx(s)		(void) _splset(s)

#define	splsoft()	_splraise(ipl_sr_bits[IPL_SOFT])

typedef int ipl_t;
typedef struct {
	ipl_t _sr;
} ipl_cookie_t;

static inline ipl_cookie_t
makeiplcookie(ipl_t ipl)
{

	return (ipl_cookie_t){._sr = ipl_sr_bits[ipl]};
}

static inline int
splraiseipl(ipl_cookie_t icookie)
{

	return _splraise(icookie._sr);
}

#include <sys/spl.h>

struct evbmips_intrhand {
	LIST_ENTRY(evbmips_intrhand) ih_q;
	int (*ih_func)(void *);
	void *ih_arg;
	int ih_irq;
};

#include <mips/softintr.h>

void	evbmips_intr_init(void);
void	intr_init(void);
void	evbmips_iointr(uint32_t, uint32_t, uint32_t, uint32_t);
void	*evbmips_intr_establish(int, int (*)(void *), void *);
void	evbmips_intr_disestablish(void *);
#endif /* _KERNEL */
#endif /* ! _EVBMIPS_INTR_H_ */
