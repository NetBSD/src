/*	$NetBSD: psl.h,v 1.12.4.1 2008/01/09 01:45:18 matt Exp $	*/

/*
 * Copyright (c) 1995 Mark Brinicombe.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Mark Brinicombe
 *	for the NetBSD Project.
 * 4. The name of the company nor the name of the author may be used to
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
 *
 * RiscBSD kernel project
 *
 * psl.h
 *
 * spl prototypes.
 * Eventually this will become a set of defines.
 *
 * Created      : 21/07/95
 */

#ifndef _ARM_PSL_H_
#define _ARM_PSL_H_
#include <machine/intr.h>
#if (! defined(_LOCORE)) && (! defined(hpcarm))
#include <arm/softintr.h>
#endif

/*
 * These are the different SPL states
 *
 * Each state has an interrupt mask associated with it which
 * indicate which interrupts are allowed.
 */

#define _SPL_0		0
#ifdef __HAVE_FAST_SOFTINTS
#define _SPL_SOFTCLOCK	1
#define _SPL_SOFTBIO	2
#define _SPL_SOFTNET	3
#define _SPL_SOFTSERIAL	4
#define _SPL_VM		5
#define _SPL_SCHED	6
#define _SPL_HIGH	7
#define _SPL_LEVELS	8
#else
#define _SPL_SOFTCLOCK	_SPL_0
#define _SPL_SOFTBIO	_SPL_0
#define _SPL_SOFTNET	_SPL_0
#define _SPL_SOFTSERIAL	_SPL_0
#define _SPL_VM		1
#define _SPL_SCHED	2
#define _SPL_HIGH	3
#define _SPL_LEVELS	4
#endif

#define spl0()		splx(_SPL_0)
#ifdef __HAVE_FAST_SOFTINTS
#define splsoftclock()	raisespl(_SPL_SOFTCLOCK)
#define splsoftbio()	raisespl(_SPL_SOFTBIO)
#define splsoftnet()	raisespl(_SPL_SOFTNET)
#define splsoftserial()	raisespl(_SPL_SOFTSERIAL)
#else
#define splsoftclock()	spl0()
#define splsoftbio()	spl0()
#define splsoftnet()	spl0()
#define splsoftserial()	spl0()
#endif
#define splvm()		raisespl(_SPL_VM)
#define splsched()	raisespl(_SPL_SCHED)
#define splhigh()	raisespl(_SPL_HIGH)

#ifdef _KERNEL
#ifndef _LOCORE
int raisespl	(int);
int lowerspl	(int);
int splx	(int);

#ifdef __HAVE_FAST_SOFTINTS
void _setsoftintr	(int si);
#endif

extern int current_spl_level;

extern u_int spl_masks[_SPL_LEVELS + 1];

typedef uint8_t ipl_t;
typedef struct {
	uint8_t _spl;
} ipl_cookie_t;

int ipl_to_spl(ipl_t);

static inline ipl_cookie_t
makeiplcookie(ipl_t ipl)
{

	return (ipl_cookie_t){._spl = (uint8_t)ipl_to_spl(ipl)};
}

static inline int
splraiseipl(ipl_cookie_t icookie)
{

	return raisespl(icookie._spl);
}
#endif /* _LOCORE */
#endif /* _KERNEL */

#endif /* _ARM_PSL_H_ */
/* End of psl.h */
