/* 	$NetBSD: intr.h,v 1.13.12.1 2002/03/17 23:43:58 thorpej Exp $	*/

/*
 * Copyright (c) 1998 Matt Thomas.
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
 * 3. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _VAX_INTR_H_
#define _VAX_INTR_H_

#include <sys/queue.h>

/* Define the various Interrupt Priority Levels */

/* Interrupt Priority Levels are not mutually exclusive. */

/* Hardware interrupt levels are 16 (0x10) thru 31 (0x1f)
 */
#define IPL_HIGH	0x1f	/* high -- blocks all interrupts */
#define IPL_CLOCK	0x18	/* clock */
#define IPL_UBA		0x17	/* unibus adapters */
#define IPL_VM		0x17	/* memory allocation */
#define IPL_NET		0x16	/* network */
#define IPL_BIO		0x15	/* block I/O */
#define IPL_TTY		0x15	/* terminal */
#define IPL_AUDIO	0x15	/* audio */
#define IPL_IPI		0x14	/* interprocessor interrupt */
#define IPL_CONSMEDIA	0x14	/* console media */

/* Software interrupt level s are 0 (0x00) thru 15 (0x0f)
 */
#define IPL_SOFTDDB	0x0f	/* used by DDB on VAX */
#define IPL_SOFTSERIAL	0x0d	/* soft serial */
#define IPL_SOFTNET	0x0c	/* soft network */
#define IPL_SOFTCLOCK	0x08
#define IPL_NONE	0x00

#define IPL_LEVELS	32

#define IST_UNUSABLE	-1	/* interrupt cannot be used */
#define IST_NONE	0	/* none (dummy) */
#define IST_PULSE	1	/* pulsed */
#define IST_EDGE	2	/* edge-triggered */
#define IST_LEVEL	3	/* level-triggered */


#ifdef _KERNEL
#ifndef lint
#define splx(reg)						\
({								\
	register int val;					\
	__asm __volatile ("mfpr $0x12,%0;mtpr %1,$0x12"		\
				: "=&g" (val)			\
				: "g" (reg));			\
	val;							\
})

#define _splset(reg)						\
((void)({							\
	__asm __volatile ("mtpr %0,$0x12"			\
				: 				\
				: "g" (reg));			\
}))

#define _splraise(reg)						\
({								\
	register int val;					\
	__asm __volatile ("mfpr $0x12,%0"			\
				: "=&g" (val)			\
				: );				\
	if ((reg) > val) {					\
		_splset(reg);					\
	}							\
	val;							\
})

#define _setsirr(reg)						\
do {								\
	__asm __volatile ("mtpr %0,$0x14"			\
				:				\
				: "g" (reg));			\
} while (0)
#endif

#define spl0()		_splset(IPL_NONE)		/* IPL00 */
#define spllowersoftclock() _splset(IPL_SOFTCLOCK)	/* IPL08 */
#define splsoftclock()	_splraise(IPL_SOFTCLOCK)	/* IPL08 */
#define splsoftnet()	_splraise(IPL_SOFTNET)		/* IPL0C */
#define splsoftserial()	_splraise(IPL_SOFTSERIAL)	/* IPL0D */
#define splddb()	_splraise(IPL_SOFTDDB)		/* IPL0F */
#define splconsmedia()	_splraise(IPL_CONSMEDIA)	/* IPL14 */
#define	splipi()	_splraise(IPL_IPI)		/* IPL14 */
#define splbio()	_splraise(IPL_BIO)		/* IPL15 */
#define spltty()	_splraise(IPL_TTY)		/* IPL15 */
#define splnet()	_splraise(IPL_NET)		/* IPL16 */
#define splvm()		_splraise(IPL_VM)		/* IPL17 */
#define splclock()	_splraise(IPL_CLOCK)		/* IPL18 */
#define splhigh()	_splraise(IPL_HIGH)		/* IPL1F */
#define splstatclock()	splclock()

#define	splsched()	splhigh()
#define	spllock()	splhigh()

/* These are better to use when playing with VAX buses */
#define	spluba()	_splraise(IPL_UBA)		/* IPL17 */
#define spl4()		splx(0x14)
#define spl5()		splx(0x15)
#define spl6()		splx(0x16)
#define spl7()		splx(0x17)

/* schedule software interrupts
 */
#define setsoftddb()	_setsirr(IPL_SOFTDDB)
#define setsoftserial()	_setsirr(IPL_SOFTSERIAL)
#define setsoftnet()	_setsirr(IPL_SOFTNET)

#if !defined(_LOCORE)
LIST_HEAD(sh_head, softintr_handler);

struct softintr_head {
	int shd_ipl;
	struct sh_head shd_intrs;
};

struct softintr_handler {
	struct softintr_head *sh_head;
	LIST_ENTRY(softintr_handler) sh_link;
	void (*sh_func)(void *);
	void *sh_arg;
	int sh_pending;
};

extern void *softintr_establish(int, void (*)(void *), void *);
extern void softintr_disestablish(void *);

static __inline void
softintr_schedule(void *arg)
{
	struct softintr_handler * const sh = arg;
	sh->sh_pending = 1;
	_setsirr(sh->sh_head->shd_ipl);
}
#endif /* _LOCORE */
#endif /* _KERNEL */
#endif	/* _VAX_INTR_H */
