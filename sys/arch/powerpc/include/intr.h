/*	$NetBSD: intr.h,v 1.2.8.3 2008/01/09 01:47:48 matt Exp $ */

/*-
 * Copyright (c) 2007 Michael Lorenz
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
 * 3. Neither the name of The NetBSD Foundation nor the names of its
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: intr.h,v 1.2.8.3 2008/01/09 01:47:48 matt Exp $");

#ifndef POWERPC_INTR_MACHDEP_H
#define POWERPC_INTR_MACHDEP_H

void *intr_establish(int, int, int, int (*)(void *), void *);
void intr_disestablish(void *);
const char *intr_typename(int);
void genppc_cpu_configure(void);

/* Interrupt priority `levels'. */
#define	IPL_NONE	0	/* nothing */
#define	IPL_SOFTCLOCK	1	/* timeouts */
#define	IPL_SOFTBIO	2	/* block I/O */
#define	IPL_SOFTNET	3	/* protocol stacks */
#define	IPL_SOFTSERIAL	4	/* serial */
#define	IPL_VM		5	/* memory allocation */
#define	IPL_SCHED	6
#define	IPL_HIGH	7	/* everything */
#define	NIPL		8

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
	int	(*ih_fun)(void *);
	void	*ih_arg;
	struct	intrhand *ih_next;
	int	ih_level;
	int	ih_irq;
};

int splraise(int);
int spllower(int);
void splx(int);
void softintr(int);

extern int imask[];

/* Soft interrupt masks. */
#define SIR_CLOCK	27
#define SIR_BIO		28
#define SIR_NET		29
#define SIR_SERIAL	30
#define SPL_CLOCK	31

#define setsoftclock()	softintr(SIR_CLOCK)
#define setsoftbio()	softintr(SIR_BIO)
#define setsoftnet()	softintr(SIR_NET)
#define setsoftserial()	softintr(SIR_SERIAL)

#define spl0()		spllower(0)

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

#endif /* _LOCORE */

#endif /* POWERPC_INTR_MACHDEP_H */
