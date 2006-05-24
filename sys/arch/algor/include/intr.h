/*	$NetBSD: intr.h,v 1.4.58.1 2006/05/24 10:56:32 yamt Exp $	*/

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

#ifndef _ALGOR_INTR_H_
#define _ALGOR_INTR_H_

#include <sys/device.h>
#include <sys/lock.h>
#include <sys/queue.h>

#define	IPL_NONE	0	/* disable only this interrupt */

#define	IPL_SOFT	1	/* generic software interrupts (SI 0) */
#define	IPL_SOFTCLOCK	2	/* clock software interrupts (SI 0) */
#define	IPL_SOFTNET	3	/* network software interrupts (SI 1) */
#define	IPL_SOFTSERIAL	4	/* serial software interrupts (SI 1) */

#define	IPL_BIO		5	/* disable block I/O interrupts */
#define	IPL_NET		6	/* disable network interrupts */
#define	IPL_TTY		7	/* disable terminal interrupts */
#define	IPL_SERIAL	7	/* disable serial interrupts */
#define	IPL_CLOCK	8	/* disable clock interrupts */
#define	IPL_HIGH	8	/* disable all interrupts */

#define	_IPL_NSOFT	4
#define	_IPL_N		9

#define	_IPL_SI0_FIRST	IPL_SOFT
#define	_IPL_SI0_LAST	IPL_SOFTCLOCK

#define	_IPL_SI1_FIRST	IPL_SOFTNET
#define	_IPL_SI1_LAST	IPL_SOFTSERIAL

#define	IPL_SOFTNAMES {							\
	"misc",								\
	"clock",							\
	"net",								\
	"serial",							\
}

#define	IST_UNUSABLE	-1	/* interrupt cannot be used */
#define	IST_NONE	0	/* none (dummy) */
#define	IST_PULSE	1	/* pulsed */
#define	IST_EDGE	2	/* edge-triggered */
#define	IST_LEVEL	3	/* level-triggered */

#ifdef	_KERNEL

extern const u_int32_t ipl_sr_bits[_IPL_N];
extern const u_int32_t ipl_si_to_sr[_IPL_NSOFT];

extern int		_splraise(int);
extern int		_spllower(int);
extern int		_splset(int);
extern int		_splget(int);
extern int		_splnone(int);
extern int		_setsoftintr(int);
extern int		_clrsoftintr(int);

#define	splhigh()	_splraise(ipl_sr_bits[IPL_HIGH])
#define	spl0()		(void) _spllower(0)
#define	splx(s)		(void) _splset(s)
#define	splbio()	_splraise(ipl_sr_bits[IPL_BIO])
#define	splnet()	_splraise(ipl_sr_bits[IPL_NET])
#define	spltty()	_splraise(ipl_sr_bits[IPL_TTY])
#define	splserial()	_splraise(ipl_sr_bits[IPL_SERIAL])
#define	splvm()		spltty()
#define	splclock()	_splraise(ipl_sr_bits[IPL_CLOCK])
#define	splstatclock()	splclock()

#define	splsched()	splclock()
#define	spllock()	splhigh()
#define	spllpt()	spltty()

#define	splsoft()	_splraise(ipl_sr_bits[IPL_SOFT])
#define	splsoftclock()	_splraise(ipl_sr_bits[IPL_SOFTCLOCK])
#define	splsoftnet()	_splraise(ipl_sr_bits[IPL_SOFTNET])
#define	splsoftserial()	_splraise(ipl_sr_bits[IPL_SOFTSERIAL])

#define	spllowersoftclock() _spllower(ipl_sr_bits[IPL_SOFTCLOCK])

struct algor_intrhand {
	LIST_ENTRY(algor_intrhand) ih_q;
	int (*ih_func)(void *);
	void *ih_arg;
	int ih_irq;		/* mostly for ISA */
	const void *ih_irqmap;
};

#include <mips/softintr.h>

extern struct evcnt mips_int5_evcnt;

void	intr_init(void);
void	*(*algor_intr_establish)(int, int (*)(void *), void *);
void	(*algor_intr_disestablish)(void *);

#endif /* _KERNEL */
#endif /* ! _ALGOR_INTR_H_ */
