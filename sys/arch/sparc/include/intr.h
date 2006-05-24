/*	$NetBSD: intr.h,v 1.8.2.1 2006/05/24 10:57:14 yamt Exp $ */

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg.
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

/*
 * Device class interrupt levels
 * Note: sun4 and sun4c hardware only has software interrupt available
 *	 on level 1, 4 or 6. This limits the choice of the various
 * 	 IPL_SOFT* symbols to one of those three values.
 */
#define IPL_NONE	0	/* nothing */
#define IPL_SOFTCLOCK	1	/* timeouts */
#define IPL_SOFTNET	1	/* protocol stack */
#define IPL_SOFTAUDIO	4	/* second-level audio */
#define IPL_SOFTFDC	4	/* second-level floppy */
#define IPL_BIO		5	/* block I/O */
#define IPL_TTY		6	/* terminal */
#define IPL_SOFTSERIAL	6	/* serial */
#define IPL_VM		7	/* memory allocation */
#define IPL_NET		7	/* network */
#define IPL_CLOCK	10	/* clock */
#define IPL_SCHED	11	/* scheduler */
#define IPL_AUDIO	13	/* audio */
#define IPL_SERIAL	13	/* serial */
#define IPL_STATCLOCK	14	/* statclock */
#define IPL_HIGH	15	/* everything */
#define	IPL_LOCK	IPL_HIGH

/*
 * fd hardware, ts102, and tadpole microcontoller interrupts are at level 11
 */

#define	IPL_FD		11
#define	IPL_TS102	11

/*
 * zs hardware interrupts are at level 12
 * su (com) hardware interrupts are at level 13
 * IPL_SERIAL must protect them all.
 */

#define	IPL_ZS		12

#if defined(_KERNEL) && !defined(_LOCORE)
void *
softintr_establish(int level, void (*fun)(void *), void *arg);

void
softintr_disestablish(void *cookie);

/*
 * NB that softintr_schedule() casts the cookie to an int *.
 * This is to get the sic_pilreq member of the softintr_cookie
 * structure, which is otherwise internal to intr.c.
 */
#if defined(SUN4M) || defined(SUN4D)
extern void	raise(int, int);
#if !(defined(SUN4) || defined(SUN4C))
#define softintr_schedule(cookie)	raise(0, *((int *) (cookie)))
#else /* both defined */
#define softintr_schedule(cookie) do {		\
	if (CPU_ISSUN4M || CPU_ISSUN4D)		\
		raise(0, *((int *)(cookie)));	\
	else					\
		ienab_bis(*((int *)(cookie)));	\
} while (0)
#endif	/* SUN4  || SUN4C */
#else	/* SUN4M || SUN4D */
#define softintr_schedule(cookie)	ienab_bis(*((int *) (cookie)))
#endif	/* SUN4M || SUN4D */

#if 0
void softintr_schedule(void *cookie);
#endif
#endif /* KERNEL && !_LOCORE */
