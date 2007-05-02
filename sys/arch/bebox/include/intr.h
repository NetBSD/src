/*	$NetBSD: intr.h,v 1.25.14.1 2007/05/02 16:24:45 matt Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
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

#ifndef _BEBOX_INTR_H_
#define _BEBOX_INTR_H_

/* Interrupt priority `levels'. */
#define	IPL_NONE	9	/* nothing */
#define	IPL_SOFTCLOCK	8	/* software clock interrupt */
#define	IPL_SOFTNET	7	/* software network interrupt */
#define	IPL_BIO		6	/* block I/O */
#define	IPL_NET		5	/* network */
#define	IPL_SOFTSERIAL	4	/* software serial interrupt */
#define	IPL_TTY		3	/* terminal */
#define	IPL_LPT		IPL_TTY
#define	IPL_VM		3	/* memory allocation */
#define	IPL_AUDIO	2	/* audio */
#define	IPL_CLOCK	1	/* clock */
#define	IPL_STATCLOCK	IPL_CLOCK
#define	IPL_HIGH	1	/* everything */
#define	IPL_SCHED	IPL_HIGH
#define	IPL_LOCK	IPL_HIGH
#define	IPL_SERIAL	0	/* serial */
#define	NIPL		10

/* Interrupt sharing types. */
#define	IST_NONE	0	/* none */
#define	IST_PULSE	1	/* pulsed */
#define	IST_EDGE	2	/* edge-triggered */
#define	IST_LEVEL	3	/* level-triggered */

#include <machine/cpu.h>

#ifndef _LOCORE
#include <powerpc/softintr.h>

/*
 * Interrupt handler chains.  intr_establish() inserts a handler into
 * the list.  The handler is called with its (single) argument.
 */
struct intrhand {
	int	(*ih_fun)(void *);
	void	*ih_arg;
	u_long	ih_count;
	struct	intrhand *ih_next;
	int	ih_level;
	int	ih_irq;
};

void do_pending_int __P((void));

void ext_intr __P((void));
void *intr_establish __P((int, int, int, int (*)(void *), void *));
void intr_disestablish __P((void *));
void intr_calculatemasks __P((void));
int  isa_intr __P((void));
void isa_intr_mask __P((int));
void isa_intr_clr __P((int));

void enable_intr __P((void));
void disable_intr __P((void));

int splraise(int newcpl);
void splx(int newcpl);
int spllower(int newcpl);
void set_sint(int pending);

extern int imask[];

#define	ICU_LEN		32
#define	IRQ_SLAVE	2
#define	LEGAL_IRQ(x)	((x) >= 0 && (x) < ICU_LEN && (x) != IRQ_SLAVE)

#define	MOTHER_BOARD_REG	0x7ffff000
#define	CPU0_INT_MASK	0x0f0
#define	CPU1_INT_MASK	0x1f0
#define	INT_STATE_REG	0x2f0

#define	SINT_CLOCK	0x20000000
#define	SINT_NET	0x40000000
#define	SINT_SERIAL	0x80000000
#define	SPL_CLOCK	0x00000001
#define	SINT_MASK	(SINT_CLOCK|SINT_NET|SINT_SERIAL)

#define	CNT_SINT_NET	29
#define	CNT_SINT_CLOCK	30
#define	CNT_SINT_SERIAL	31
#define	CNT_CLOCK	0

#define	setsoftclock()	set_sint(SINT_CLOCK);
#define	setsoftnet()	set_sint(SINT_NET);
#define	setsoftserial()	set_sint(SINT_SERIAL);

#define	spl0()		spllower(0)

typedef int ipl_t;
typedef struct {
	ipl_t _ipl;
} ipl_cookie_t;

static inline ipl_cookie_t
makeiplcookie(ipl_t ipl)
{

	return (ipl_cookie_t){._ipl = ipl};
}

static inline int
splraiseipl(ipl_cookie_t icookie)
{

	return splraise(imask[icookie._ipl]);
}

#include <sys/spl.h>

#endif /* !_LOCORE */

#endif /* !_BEBOX_INTR_H_ */
