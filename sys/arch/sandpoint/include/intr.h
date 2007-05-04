/*	$NetBSD: intr.h,v 1.12.14.1 2007/05/04 10:34:13 nisimura Exp $	*/

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

#ifndef _MACHINE_INTR_H_
#define _MACHINE_INTR_H_

/* Interrupt priority `levels'. */
#define IPL_NONE        0       /* nothing */
#define IPL_SOFTCLOCK   1       /* timeouts */
#define IPL_SOFTNET     2       /* protocol stacks */
#define IPL_BIO         3       /* block I/O */
#define IPL_NET         4       /* network */
#define IPL_SOFTSERIAL  5       /* serial */
#define IPL_AUDIO       6       /* audio */
#define IPL_TTY         7       /* terminal */
#define IPL_LPT         IPL_TTY
#define IPL_VM          8       /* memory allocation */
#define IPL_CLOCK       9
#define IPL_STATCLOCK   10      /* clock */
#define IPL_SCHED       11
#define IPL_SERIAL      12      /* serial */
#define IPL_LOCK        13
#define IPL_HIGH        14      /* everything */
#define NIPL            15

/* Interrupt sharing types. */
#define	IST_NONE	0	/* none */
#define	IST_PULSE	1	/* pulsed */
#define	IST_EDGE	2	/* edge-triggered */
#define	IST_LEVEL	3	/* level-triggered */

#ifndef _LOCORE
#include <powerpc/softintr.h>
#include <machine/cpu.h>

/*
 * Interrupt handler chains.  intr_establish() inserts a handler into
 * the list.  The handler is called with its (single) argument.
 */
struct intrhand {
	int	(*ih_fun)(void *);
	void	*ih_arg;
	struct	intrhand *ih_next;
	int	ih_level;
	int	ih_irq;
};

#include <sys/device.h>

int splraise(int);
int spllower(int);
void softintr(int);
void splx(int);

void do_pending_int(void);

void init_intr_ivr(void);
void init_intr_openpic(void);

void enable_intr(void);
void disable_intr(void);

void *intr_establish(int, int, int, int (*)(void *), void *);
void intr_disestablish(void *);

const char *intr_typename(int);

void softnet(int);
void softserial(void);
int isa_intr(void);
void isa_intr_mask(int);
void isa_intr_clr(int);
void isa_setirqstat(int, int, int);

extern int imen;
extern int imask[];
extern struct intrhand *intrhand[];

#define	ICU_LEN			64

#if 1 /* PIC_I8259 */
#define	IRQ_SLAVE		2
#define	LEGAL_IRQ(x)		((x) >= 0 && (x) < ICU_LEN && (x) != IRQ_SLAVE)
#define	I8259_INTR_NUM		16
#define	OPENPIC_INTR_NUM	((ICU_LEN)-(I8259_INTR_NUM))
#else
#define	LEGAL_IRQ(x)		((x) >= 0 && (x) < ICU_LEN)
#define	OPENPIC_INTR_NUM	(0)
#endif

/* Soft interrupt masks. */
#define SIR_CLOCK       28
#define SIR_NET         29
#define SIR_SERIAL      30
#define SPL_CLOCK       31

#define setsoftclock()  softintr(SIR_CLOCK);
#define setsoftnet()    softintr(SIR_NET);
#define setsoftserial() softintr(SIR_SERIAL);

/*#define	splx(x)		spllower(x)*/
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

#endif /* !_MACHINE_INTR_H_ */
