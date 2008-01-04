/*	$NetBSD: intr.h,v 1.8 2008/01/04 22:03:26 ad Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe, UCHIYAMA Yasushi.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

#ifndef _PLAYSTATION2_INTR_H_
#define _PLAYSTATION2_INTR_H_
#ifdef _KERNEL

#include <mips/locore.h>

/* Interrupt sharing types. */
#define	IST_NONE		0	/* none */
#define	IST_PULSE		1	/* pulsed */
#define	IST_EDGE		2	/* edge-triggered */
#define	IST_LEVEL		3	/* level-triggered */

/* Interrupt priority levels */
#define	IPL_NONE		0	/* nothing */
#define IPL_SOFTCLOCK		1	/* timeouts */
#define	IPL_SOFTBIO		1	/* bio */
#define	IPL_SOFTNET		2	/* protocol stacks */
#define IPL_SOFTSERIAL		2	/* serial */
#define	IPL_VM			3	/* i/o */
#define	IPL_SCHED		4	/* clock */
#define	IPL_HIGH		4	/* everything */

#define	_IPL_NSOFT	4
#define	_IPL_N		5

/*
 * Hardware interrupt masks
 */
extern u_int32_t __icu_mask[_IPL_N];

#define splsoftclock()		splraise(__icu_mask[IPL_SOFTCLOCK])
#define	splsoftbio()		splraise(__icu_mask[IPL_SOFTBIO])
#define	splsoftnet()		splraise(__icu_mask[IPL_SOFTNET])
#define	splsoftserial()		splraise(__icu_mask[IPL_SOFTSERIAL])
#define	splvm()			splraise(__icu_mask[IPL_VM])
#define	splsched()		splraise(__icu_mask[IPL_SCHED])
#define	splx(s)			splset(s)

void	spllowersofthigh(void);

int splraise(int);
void splset(int);
void spl0(void);

/* R5900 EI/DI instruction */
int _intr_suspend(void);
void _intr_resume(int);

#endif /* _KERNEL */
#endif /* _PLAYSTATION2_INTR_H_ */
