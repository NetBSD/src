/*	$NetBSD: softintr.h,v 1.1 2003/05/25 13:48:01 tsutsui Exp $	*/

/*-
 * Copyright (c) 2000, 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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

#ifndef _MIPS_SOFTINTR_H_
#define _MIPS_SOFTINTR_H_

#ifndef _LOCORE

#include <sys/device.h>
#include <sys/lock.h>

extern const u_int32_t mips_ipl_si_to_sr[_IPL_NSOFT];

#define setsoft(x)							\
do {									\
	_setsoftintr(mips_ipl_si_to_sr[(x) - IPL_SOFT]);		\
} while (/*CONSTCOND*/0)

struct mips_soft_intrhand {
	TAILQ_ENTRY(mips_soft_intrhand) sih_q;
	struct mips_soft_intr *sih_intrhead;
	void (*sih_func)(void *);
	void *sih_arg;
	int sih_pending;
};

struct mips_soft_intr {
	TAILQ_HEAD(,mips_soft_intrhand) softintr_q;
	struct evcnt softintr_evcnt;
	struct simplelock softintr_slock;
	unsigned long softintr_ipl;
};

void softintr_init(void);
void *softintr_establish(int, void (*)(void *), void *);
void softintr_disestablish(void *);
void softintr_dispatch(u_int32_t);

#define softintr_schedule(arg)						\
do {									\
	struct mips_soft_intrhand *__sih = (arg);			\
	struct mips_soft_intr *__msi = __sih->sih_intrhead;		\
	int __s;							\
									\
	__s = splhigh();						\
	simple_lock(&__msi->softintr_slock);				\
	if (__sih->sih_pending == 0) {					\
		TAILQ_INSERT_TAIL(&__msi->softintr_q, __sih, sih_q);	\
		__sih->sih_pending = 1;					\
		setsoft(__msi->softintr_ipl);				\
	}								\
	simple_unlock(&__msi->softintr_slock);				\
	splx(__s);							\
} while (/*CONSTCOND*/0)

/* XXX For legacy software interrupts. */
extern struct mips_soft_intrhand *softnet_intrhand;

#define setsoftnet()	softintr_schedule(softnet_intrhand)

#endif /* !_LOCORE */
#endif /* _MIPS_SOFTINTR_H_ */
