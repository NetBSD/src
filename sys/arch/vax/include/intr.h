/* 	$NetBSD: intr.h,v 1.29 2009/05/13 03:38:08 mhitch Exp $	*/

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

/* Hardware interrupt levels are 16 (0x10) thru 31 (0x1f) */
#define IPL_HIGH	0x1f	/* high -- blocks all interrupts */
#define IPL_SCHED	0x18	/* clock */
#define IPL_VM		0x17	/* memory allocation */

/* Software interrupt levels are 0 (0x00) thru 15 (0x0f) */
#define IPL_SOFTDDB	0x0f	/* used by DDB on VAX */
#define IPL_SOFTSERIAL	0x0d	/* soft serial */
#define IPL_SOFTNET	0x0c	/* soft network */
#define IPL_SOFTBIO	0x0b	/* soft bio */
#define IPL_SOFTCLOCK	0x08
#define IPL_NONE	0x00

/* vax weirdness */
#define IPL_UBA		IPL_VM	/* unibus adapters */
#define IPL_CONSMEDIA	IPL_VM	/* console media */

/* Misc */
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


#define spl0()		_splset(IPL_NONE)		/* IPL00 */
#define splddb()	splraiseipl(makeiplcookie(IPL_SOFTDDB)) /* IPL0F */
#define splconsmedia()	splraiseipl(makeiplcookie(IPL_CONSMEDIA)) /* IPL17 */

#include <sys/spl.h>

/* These are better to use when playing with VAX buses */
#define	spluba()	splraiseipl(makeiplcookie(IPL_UBA)) /* IPL17 */
#define spl7()		splvm()

/* schedule software interrupts
 */
#define setsoftddb()	((void)mtpr(IPL_SOFTDDB, PR_SIRR))

#if !defined(_LOCORE)

#if defined(__HAVE_FAST_SOFTINTS)
static inline void
softint_trigger(uintptr_t machdep)
{
	mtpr(machdep, PR_SIRR);
}
#endif /* __HAVE_FAST_SOFTINTS */
#endif /* !_LOCORE */
#endif /* _KERNEL */
#endif	/* _VAX_INTR_H */
