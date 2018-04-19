/*	$NetBSD: intr.h,v 1.13 2018/04/19 21:50:07 christos Exp $ */

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

#ifndef _LOCORE
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: intr.h,v 1.13 2018/04/19 21:50:07 christos Exp $");
#endif

#ifndef _POWERPC_INTR_MACHDEP_H_
#define _POWERPC_INTR_MACHDEP_H_

#define	__HAVE_FAST_SOFTINTS	1


/* Interrupt priority `levels'. */
#define	IPL_NONE		0	/* nothing */
#define	IPL_SOFTCLOCK		1	/* timeouts */
#define	IPL_SOFTBIO		2	/* block I/O */
#define	IPL_SOFTNET		3	/* protocol stacks */
#define	IPL_SOFTSERIAL		4	/* serial */
#define	IPL_VM			5	/* memory allocation */
#define	IPL_SCHED		6
#define	IPL_HIGH		7	/* everything */
#define	NIPL			8

/* Interrupt sharing types. */
#define	IST_NONE		0	/* none */
#define	IST_PULSE		1	/* pulsed */
#define	IST_EDGE		2	/* falling edge triggered */
#define	IST_LEVEL		3	/* low level triggered */

#define IST_EDGE_FALLING	IST_EDGE
#define IST_EDGE_RISING		4	/* rising edge triggered */
#define IST_LEVEL_LOW		IST_LEVEL
#define IST_LEVEL_HIGH		5	/* high level triggered */

#if !defined(_LOCORE)
void *	intr_establish(int, int, int, int (*)(void *), void *);
void *	intr_establish_xname(int, int, int, int (*)(void *), void *,
	    const char *);
void	intr_disestablish(void *);
const char *
	intr_typename(int);

int	splraise(int);
int	spllower(int);
void	splx(int);

#if !defined(_MODULE)

#include <powerpc/softint.h>

void	genppc_cpu_configure(void);

/*
 * Interrupt handler chains.  intr_establish() inserts a handler into
 * the list.  The handler is called with its (single) argument.
 */
struct intrhand {
	int	(*ih_fun)(void *);
	void	*ih_arg;
	struct	intrhand *ih_next;
	int	ih_ipl;
	int	ih_virq;
	char	ih_xname[INTRDEVNAMEBUF];
};

void softint_fast_dispatch(struct lwp *, int);

#define softint_init_md		powerpc_softint_init_md
#define softint_trigger		powerpc_softint_trigger

#ifdef __IMASK_T
typedef __IMASK_T imask_t;
#else
typedef uint32_t imask_t;
#endif

extern imask_t imask[];

#define NVIRQ		(sizeof(imask_t)*8)	/* 32 virtual IRQs */
#ifndef NIRQ
#define NIRQ		128	/* up to 128 HW IRQs */
#endif

#define HWIRQ_MAX       (NVIRQ - 1)
#define HWIRQ_MASK     	(~(imask_t)0 >> 1)

#define	PIC_VIRQ_TO_MASK(v)	__BIT(HWIRQ_MAX - (v))
#define PIC_VIRQ_MS_PENDING(p)	__builtin_clz(p)

#endif /* !_MODULE */

#define spl0()		spllower(0)

typedef int ipl_t;
typedef struct {
	ipl_t _ipl;
} ipl_cookie_t;

static __inline ipl_cookie_t
makeiplcookie(ipl_t ipl)
{

	return (ipl_cookie_t){._ipl = ipl};
}

static __inline int
splraiseipl(ipl_cookie_t icookie)
{

	return splraise(icookie._ipl);
}

#include <sys/spl.h>

#endif /* _LOCORE */

#endif /* _POWERPC_INTR_MACHDEP_H_ */
