/*	$NetBSD: intr.h,v 1.13.2.2 2002/03/16 15:57:01 jdolecek Exp $	*/

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
#define	IPL_IMP		3	/* memory allocation */
#define	IPL_AUDIO	2	/* audio */
#define	IPL_CLOCK	1	/* clock */
#define	IPL_HIGH	1	/* everything */
#define	IPL_SERIAL	0	/* serial */
#define	NIPL		10

/* Interrupt sharing types. */
#define	IST_NONE	0	/* none */
#define	IST_PULSE	1	/* pulsed */
#define	IST_EDGE	2	/* edge-triggered */
#define	IST_LEVEL	3	/* level-triggered */

#ifndef _LOCORE

/*
 * Interrupt handler chains.  intr_establish() inserts a handler into
 * the list.  The handler is called with its (single) argument.
 */
struct intrhand {
	int	(*ih_fun) __P((void *));
	void	*ih_arg;
	u_long	ih_count;
	struct	intrhand *ih_next;
	int	ih_level;
	int	ih_irq;
};

void setsoftclock __P((void));
void clearsoftclock __P((void));
int  splsoftclock __P((void));
void setsoftnet   __P((void));
void clearsoftnet __P((void));
int  splsoftnet   __P((void));

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

static __inline int splraise __P((int));
static __inline int spllower __P((int));
static __inline void splx __P((int));
static __inline void set_sint __P((int));

extern volatile int cpl, ipending, astpending, tickspending;
extern int imask[];
extern long intrcnt[];

/*
 *  Reorder protection in the following inline functions is
 * achieved with the "eieio" instruction which the assembler
 * seems to detect and then doesn't move instructions past....
 */
static __inline int
splraise(newcpl)
	int newcpl;
{
	int oldcpl;

	__asm__ volatile("sync; eieio\n");	/* don't reorder.... */
	oldcpl = cpl;
	cpl = oldcpl | newcpl;
	__asm__ volatile("sync; eieio\n");	/* reorder protect */
	return(oldcpl);
}

static __inline void
splx(newcpl)
	int newcpl;
{
	__asm__ volatile("sync; eieio\n");	/* reorder protect */
	cpl = newcpl;
	if(ipending & ~newcpl)
		do_pending_int();
	__asm__ volatile("sync; eieio\n");	/* reorder protect */
}

static __inline int
spllower(newcpl)
	int newcpl;
{
	int oldcpl;

	__asm__ volatile("sync; eieio\n");	/* reorder protect */
	oldcpl = cpl;
	cpl = newcpl;
	if(ipending & ~newcpl)
		do_pending_int();
	__asm__ volatile("sync; eieio\n");	/* reorder protect */
	return(oldcpl);
}

/* Following code should be implemented with lwarx/stwcx to avoid
 * the disable/enable. i need to read the manual once more.... */
static __inline void
set_sint(pending)
	int	pending;
{
	int	msrsave;

	__asm__ ("mfmsr %0" : "=r"(msrsave));
	__asm__ volatile ("mtmsr %0" :: "r"(msrsave & ~PSL_EE));
	ipending |= pending;
	__asm__ volatile ("mtmsr %0" :: "r"(msrsave));
}

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

#define splbio()	splraise(imask[IPL_BIO])
#define splnet()	splraise(imask[IPL_NET])
#define spltty()	splraise(imask[IPL_TTY])
#define splclock()	splraise(imask[IPL_CLOCK])
#define splvm()		splraise(imask[IPL_IMP])
#define	splserial()	splraise(imask[IPL_SERIAL])
#define splstatclock()	splclock()
#define	spllowersoftclock() spllower(imask[IPL_SOFTCLOCK])
#define	splsoftclock()	splraise(imask[IPL_SOFTCLOCK])
#define	splsoftnet()	splraise(imask[IPL_SOFTNET])
#define	splsoftserial()	splraise(imask[IPL_SOFTSERIAL])

#define spllpt()	spltty()

#define	setsoftclock()	set_sint(SINT_CLOCK);
#define	setsoftnet()	set_sint(SINT_NET);
#define	setsoftserial()	set_sint(SINT_SERIAL);

#define	splhigh()	splraise(imask[IPL_HIGH])
#define	spl0()		spllower(0)

#define	splsched()	splhigh()
#define	spllock()	splhigh()

#endif /* !_LOCORE */

#endif /* !_BEBOX_INTR_H_ */
