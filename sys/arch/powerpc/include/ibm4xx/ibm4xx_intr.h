/*	$NetBSD: ibm4xx_intr.h,v 1.11 2005/12/31 14:07:32 hannken Exp $	*/

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

#ifndef _EVBPPC_INTR_H_
#define _EVBPPC_INTR_H_

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

#ifndef _LOCORE

#define	CLKF_BASEPRI(frame)	((frame)->pri == 0)
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

void setsoftclock(void);
void setsoftnet(void);

void do_pending_int(void);
void ext_intr(void);
void *intr_establish(int, int, int, int (*)(void *), void *);
void intr_disestablish(void *);
void intr_init(void);

static inline int splraise(int);
static inline int spllower(int);
static inline void splx(int);
static inline void set_sint(int);

extern volatile int cpl, ipending, astpending;
extern u_long imask[];
extern u_long intrcnt[];

/*
 * Reorder protection in the following inline functions is
 * achived with the "eieio" instruction which the assembler
 * seems to detect and then doesn't move instructions past....
 */
static inline int
splraise(newcpl)
	int newcpl;
{
	int oldcpl;

	__asm volatile("sync; eieio\n");	/* don't reorder.... */
	oldcpl = cpl;
	cpl = oldcpl | newcpl;
	__asm volatile("sync; eieio\n");	/* reorder protect */
	return(oldcpl);
}

static inline void
splx(newcpl)
	int newcpl;
{
	__asm volatile("sync; eieio\n");	/* reorder protect */
	cpl = newcpl;
	if (ipending & ~newcpl)
		do_pending_int();
	__asm volatile("sync; eieio\n");	/* reorder protect */
}

static inline int
spllower(newcpl)
	int newcpl;
{
	int oldcpl;

	__asm volatile("sync; eieio\n");	/* reorder protect */
	oldcpl = cpl;
	cpl = newcpl;
	if (ipending & ~newcpl)
		do_pending_int();
	__asm volatile("sync; eieio\n");	/* reorder protect */
	return(oldcpl);
}

/* Following code should be implemented with lwarx/stwcx to avoid
 * the disable/enable. i need to read the manual once more.... */
static inline void
set_sint(pending)
	int	pending;
{
	int	msrsave;

	__asm ("mfmsr %0" : "=r"(msrsave));
	__asm volatile ("mtmsr %0" :: "r"(msrsave & ~PSL_EE));
	ipending |= pending;
	__asm volatile ("mtmsr %0" :: "r"(msrsave));
}

#define	ICU_LEN		32
#define	LEGAL_IRQ(x)	((x) >= 0 && (x) < ICU_LEN)
#define	IRQ_TO_MASK(x)	(0x80000000UL >> (x))

/*
 * XXX 405GP specific.
 * Interrupt bits 0-18 and 25-31 are used by hardware.  This 
 * leaves us bits 19-24 for software.
 */
#define	HWINT_MASK	~0x1fc0

/* Assign these to unused (reserved) interrupt bits: */
#define	CNT_SINT_NET	19
#define	CNT_SINT_CLOCK	20
#define	CNT_SINT_SERIAL	21
#define	CNT_CLOCK       22
#define	CNT_STATCLOCK	23

#define	SINT_NET	IRQ_TO_MASK(CNT_SINT_NET)
#define	SINT_CLOCK	IRQ_TO_MASK(CNT_SINT_CLOCK)
#define	SINT_SERIAL	IRQ_TO_MASK(CNT_SINT_SERIAL)
#define	SPL_CLOCK	IRQ_TO_MASK(CNT_CLOCK)
#define	SINT_MASK	(SINT_CLOCK|SINT_NET|SINT_SERIAL)

#define	spllowersoftclock() spllower(imask[IPL_SOFTCLOCK])

#define	splraiseipl(x)	splraise(imask[x])

#include <sys/spl.h>

#define	setsoftclock()	set_sint(SINT_CLOCK);
#define	setsoftnet()	set_sint(SINT_NET);
#define	setsoftserial()	set_sint(SINT_SERIAL);

#define	spl0()		spllower(0)

void softnet(void);
void softserial(void);

#endif /* !_LOCORE */

#endif /* !_EVBPPC_INTR_H_ */
