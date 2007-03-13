/* 	$NetBSD: intr.h,v 1.23.6.1 2007/03/13 16:50:09 ad Exp $	*/

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
#include <machine/mtpr.h>

/* Define the various Interrupt Priority Levels */

/* Interrupt Priority Levels are not mutually exclusive. */

/* Hardware interrupt levels are 16 (0x10) thru 31 (0x1f)
 */
#define IPL_HIGH	0x1f	/* high -- blocks all interrupts */
#define IPL_CLOCK	0x18	/* clock */
#define IPL_STATCLOCK	IPL_CLOCK
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

/* Misc
 */

#define	IPL_SCHED	IPL_HIGH
#define	IPL_LOCK	IPL_HIGH

#define IPL_LEVELS	32

#define IST_UNUSABLE	-1	/* interrupt cannot be used */
#define IST_NONE	0	/* none (dummy) */
#define IST_PULSE	1	/* pulsed */
#define IST_EDGE	2	/* edge-triggered */
#define IST_LEVEL	3	/* level-triggered */


#ifdef _KERNEL
typedef int ipl_t;

static inline void
_splset(ipl_t ipl)
{
	mtpr(ipl, PR_IPL);
}

static inline ipl_t
_splget(void)
{
	return mfpr(PR_IPL);
}

static inline ipl_t
splx(ipl_t new_ipl)
{
	ipl_t old_ipl = _splget();
	_splset(new_ipl);
	return old_ipl;
}

typedef struct {
	uint8_t _ipl;
} ipl_cookie_t;

static inline ipl_cookie_t
makeiplcookie(ipl_t ipl)
{
	return (ipl_cookie_t){._ipl = (uint8_t)ipl};
}

static inline int
splraiseipl(ipl_cookie_t icookie)
{
	ipl_t newipl = icookie._ipl;
	ipl_t oldipl;

	oldipl = _splget();
	if (newipl > oldipl) {
		_splset(newipl);
	}
	return oldipl;
}

#define _setsirr(reg)	mtpr((reg), PR_SIRR)

#define spl0()		_splset(IPL_NONE)		/* IPL00 */
#define splddb()	splraiseipl(makeiplcookie(IPL_SOFTDDB)) /* IPL0F */
#define splconsmedia()	splraiseipl(makeiplcookie(IPL_CONSMEDIA)) /* IPL14 */

#include <sys/spl.h>

/* These are better to use when playing with VAX buses */
#define	spluba()	splraiseipl(makeiplcookie(IPL_UBA)) /* IPL17 */
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
