/*	$NetBSD: intr.h,v 1.6.4.1 2006/12/01 20:27:59 yamt Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _SH5_INTR_H
#define _SH5_INTR_H

#include <sys/evcnt.h>

/* Interrupt sharing types */
#define	IST_NONE	0	/* none */
#define	IST_PULSE	1	/* pulsed */
#define	IST_EDGE	2	/* edge-triggered */
#define	IST_LEVEL	3	/* level-triggered */

/* Interrupt priority levels */
#define	NIPL		16
#define	_IPL_NSOFT	4

#define	IPL_NONE	0	/* nothing */
#define	IPL_SOFT	1	/* miscellaneous soft interrupts */
#define	IPL_SOFTCLOCK	2	/* timeouts */
#define	IPL_SOFTNET	3	/* protocol stacks */
/* 4 - 7 available for misc hardware interrupts */
#define	IPL_BIO		8	/* Block I/O */
#define	IPL_NET		9	/* Network */
#define	IPL_SOFTSERIAL	10	/* serial */
#define	IPL_TTY		11	/* tty subsystem */
#define	IPL_VM		11	/* memory allocation */
#define	IPL_SERIAL	12	/* tty subsystem */
#define	IPL_CLOCK	14	/* Hard clock */
#define	IPL_STATCLOCK	IPL_CLOCK
#define	IPL_SCHED	IPL_CLOCK
#define	IPL_HIGH	15
#define	IPL_LOCK	IPL_HIGH

#define	spl0()			splx(IPL_NONE)

#define	spllowersoftclock()	splx(IPL_SOFTCLOCK);

#ifndef _LOCORE
extern void	splx(int);
extern int	splraise(int);

extern void	*softintr_establish(int, void (*)(void *), void *);
extern void	softintr_disestablish(void *);
extern void	softintr_schedule(void *);

/* XXX For legacy software interrupts */
extern void	*softnet_cookie;
#define setsoftnet()	softintr_schedule(softnet_cookie)

extern void	*sh5_intr_establish(int, int, int, int (*)(void *), void *);
extern void	sh5_intr_disestablish(void *);
extern struct evcnt *sh5_intr_evcnt(void *);


/* Internal use only */
extern void	softintr_dispatch(u_int, u_int);
extern void	softintr_init(void);
extern void	sh5_intr_init(void (*)(void *, u_int, int, int),
		    void (*)(void *, u_int), void *);
extern void	*sh5_intr_alloc_handle(size_t);
extern void	sh5_intr_free_handle(void *);
extern struct evcnt _sh5_intr_events[];
extern void	_cpu_intr_set(u_int);
extern void	_cpu_intr_resume(u_int);


/* Supplied by board-specific code to clear down an NMI source */
extern void	sh5_nmi_clear(void);

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

	return splraise(icookie._ipl);
}

#include <sys/spl.h>

#endif /* !_LOCORE */

#endif /* _SH5_INTR_H */
