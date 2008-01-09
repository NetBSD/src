/*	$NetBSD: intr.h,v 1.18.18.1 2008/01/09 01:47:29 matt Exp $	*/

/*
 *
 * Copyright (c) 1998 NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Minoura Makoto and Jason R. Thorpe.
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
 *      This product includes software developed by The NetBSD Foundation
 *	Inc. and its contributers.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _NEWS68K_INTR_H_
#define	_NEWS68K_INTR_H_

#include <sys/device.h>
#include <sys/queue.h>
#include <machine/psl.h>
#include <m68k/asm_single.h>

#define	IPL_NONE	0
#define	IPL_SOFTCLOCK	1
#define	IPL_SOFTBIO	2
#define	IPL_SOFTNET	3
#define	IPL_SOFTSERIAL	4
#define	IPL_VM		5
#define	IPL_SCHED	6
#define	IPL_HIGH	6
#define	NIPL		7

typedef int ipl_t;
typedef struct {
	uint16_t _psl;
} ipl_cookie_t;

ipl_cookie_t makeiplcookie(ipl_t);

static inline int
splraiseipl(ipl_cookie_t icookie)
{

	return _splraise(icookie._psl);
}

static __inline void
splx(int sr)
{

	__asm volatile("movw %0,%%sr" : : "di" (sr));
}

/*
 * news68k can handle software interrupts by its own hardware
 * so has no need to check for any simulated interrupts, etc.
 */
#define	spl0()		_spl0()

#define	splsoftbio()	splraise2()
#define	splsoftclock()	splraise2()
#define	splsoftnet()	splraise2()
#define	splsoftserial()	splraise2()
#define	splvm()		splraise5()
#define	splhigh()	spl7()
#define	splsched()	spl7()

#endif /* _NEWS68K_INTR_H_ */
