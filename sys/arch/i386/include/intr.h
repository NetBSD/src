/*	$NetBSD: intr.h,v 1.12.10.9 2000/12/31 17:45:51 thorpej Exp $	*/

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

#ifndef _I386_INTR_H_
#define _I386_INTR_H_

/*
 * Interrupt priority levels.
 * 
 * There are tty, network and disk drivers that use free() at interrupt
 * time, so imp > (tty | net | bio).
 *
 * Since run queues may be manipulated by both the statclock and tty,
 * network, and disk drivers, clock > imp.
 *
 * IPL_HIGH must block everything that can manipulate a run queue.
 *
 * We need serial drivers to run at the absolute highest priority to
 * avoid overruns, so serial > high.
 */
#define	IPL_NONE	0x00	/* nothing */
#define	IPL_SOFTCLOCK	0x50	/* timeouts */
#define	IPL_SOFTNET	0x60	/* protocol stacks */
#define	IPL_BIO		0x70	/* block I/O */
#define	IPL_NET		0x80	/* network */
#define	IPL_SOFTSERIAL	0x90	/* serial */
#define	IPL_TTY		0xa0	/* terminal */
#define	IPL_IMP		0xb0	/* memory allocation */
#define	IPL_AUDIO	0xc0	/* audio */
#define	IPL_CLOCK	0xd0	/* clock */
#define	IPL_HIGH	0xd0	/* everything */
#define	IPL_SERIAL	0xe0	/* serial */
#define IPL_IPI		0xe0	/* inter-processor interrupts */
#define	NIPL		16

/* Interrupt sharing types. */
#define	IST_NONE	0	/* none */
#define	IST_PULSE	1	/* pulsed */
#define	IST_EDGE	2	/* edge-triggered */
#define	IST_LEVEL	3	/* level-triggered */

/* Soft interrupt masks. */
#define	SIR_CLOCK	31
#define	SIR_NET		30
#define	SIR_SERIAL	29

/* Hack for CLKF_INTR(). */
#define	IPL_TAGINTR	28

#ifndef _LOCORE

#ifdef MULTIPROCESSOR
#include <machine/i82489reg.h>
#include <machine/i82489var.h>
#endif

extern volatile u_int32_t lapic_tpr;
volatile u_int32_t ipending;

int imasks[NIPL];
int iunmask[NIPL];

#define CPSHIFT 4
#define IMASK(level) imasks[(level)>>CPSHIFT]
#define IUNMASK(level) iunmask[(level)>>CPSHIFT]

extern void Xspllower __P((void));

static __inline int splraise __P((int));
static __inline void spllower __P((int));
static __inline void softintr __P((int, int));

/*
 * Add a mask to cpl, and return the old value of cpl.
 */
static __inline int
splraise(ncpl)
	register int ncpl;
{
	register int ocpl = lapic_tpr;

	if (ncpl > ocpl)
		lapic_tpr = ncpl;
	return (ocpl);
}

/*
 * Restore a value to cpl (unmasking interrupts).  If any unmasked
 * interrupts are pending, call Xspllower() to process them.
 */
static __inline void
spllower(ncpl)
	register int ncpl;
{
	register int cmask;

	lapic_tpr = ncpl;
	cmask = IUNMASK(ncpl);
	if (ipending & cmask)
		Xspllower();
}

/*
 * Hardware interrupt masks
 */
#define	splbio()	splraise(IPL_BIO)
#define	splnet()	splraise(IPL_NET)
#define	spltty()	splraise(IPL_TTY)
#define	splaudio()	splraise(IPL_AUDIO)
#define	splclock()	splraise(IPL_CLOCK)
#define	splstatclock()	splclock()
#define	splserial()	splraise(IPL_SERIAL)
#define spllock() 	splraise(IPL_SERIAL) /* XXX XXX XXX XXX */
#define splsched()	splraise(IPL_HIGH)
#define splipi()	splraise(IPL_IPI)

#define spllpt()	spltty()

#define SPL_ASSERT_ATMOST(x) KDASSERT(lapic_tpr <= (x))

/*
 * Software interrupt masks
 *
 * NOTE: splsoftclock() is used by hardclock() to lower the priority from
 * clock to softclock before it calls softclock().
 */
#define	spllowersoftclock() spllower(IPL_SOFTCLOCK)

#define	splsoftclock()	splraise(IPL_SOFTCLOCK)
#define	splsoftnet()	splraise(IPL_SOFTNET)
#define	splsoftserial()	splraise(IPL_SOFTSERIAL)

/*
 * Miscellaneous
 */
#define	splimp()	splraise(IPL_IMP)
#define	splhigh()	splraise(IPL_HIGH)
#define	spl0()		spllower(IPL_NONE)
#define	splx(x)		spllower(x)

/*
 * Software interrupt registration
 *
 * We hand-code this to ensure that it's atomic.
 */
static __inline void
softintr(sir, vec)
	register int sir;
	register int vec;
{
	__asm __volatile("orl %1, %0" : "=m"(ipending) : "ir" (1 << sir));
#ifdef MULTIPROCESSOR
	i82489_writereg(LAPIC_ICRLO,
	    vec | LAPIC_DLMODE_FIXED | LAPIC_LVL_ASSERT | LAPIC_DEST_SELF);
#endif
}

#define	setsoftclock()	softintr(SIR_CLOCK,IPL_SOFTCLOCK)
#define	setsoftnet()	softintr(SIR_NET,IPL_SOFTNET)
#define	setsoftserial()	softintr(SIR_SERIAL,IPL_SOFTSERIAL)


#define I386_IPI_HALT			0x00000001
#define I386_IPI_FLUSH_FPU		0x00000002
#define I386_IPI_SYNCH_FPU		0x00000004
#define I386_IPI_TLB			0x00000008
#define I386_IPI_MTRR			0x00000010

#define I386_NIPI		5

#ifdef MULTIPROCESSOR
void i386_send_ipi (struct cpu_info *, int);
void i386_broadcast_ipi (int);
void i386_multicast_ipi (int, int);
void i386_ipi_handler (void);
#endif

#endif /* !_LOCORE */

#endif /* !_I386_INTR_H_ */
