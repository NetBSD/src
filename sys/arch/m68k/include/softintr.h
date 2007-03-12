/*	$NetBSD: softintr.h,v 1.2.4.2 2007/03/12 06:14:50 rmind Exp $	*/

/*-
 * Copyright (c) 1996, 1997, 1999 The NetBSD Foundation, Inc.
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

#ifndef _M68K_SOFTINTR_H_
#define	_M68K_SOFTINTR_H_

#include <sys/device.h>
#include <sys/queue.h>
#include <machine/psl.h>

#define	SI_SOFTSERIAL	0	/* serial software interrupts */
#define	SI_SOFTNET	1	/* network software interrupts */
#define	SI_SOFTCLOCK	2	/* clock software interrupts */
#define	SI_SOFT		3	/* other software interrupts */

#define	SI_NQUEUES	4

#define	SI_QUEUENAMES {							\
	"serial",							\
	"net",								\
	"clock",							\
	"misc",								\
}

struct m68k_soft_intrhand {
	LIST_ENTRY(m68k_soft_intrhand) sih_q;
	struct m68k_soft_intr *sih_intrhead;
	void (*sih_func)(void *);
	void *sih_arg;
	volatile int sih_pending;
};

struct m68k_soft_intr {
	LIST_HEAD(, m68k_soft_intrhand) msi_q;
	struct evcnt msi_evcnt;
	uint8_t msi_ipl;
};

void *softintr_establish(int, void (*)(void *), void *);
void softintr_disestablish(void *);
void softintr_init(void);
void softintr_dispatch(void);

extern volatile uint8_t ssir;
#define setsoft(x)	ssir |= (1 << (x))

#define softintr_schedule(arg)				\
do {							\
	struct m68k_soft_intrhand *__sih = (arg);	\
	__sih->sih_pending = 1;				\
	setsoft(__sih->sih_intrhead->msi_ipl);		\
} while (/* CONSTCOND */0)

/* XXX For legacy software interrupts */
extern struct m68k_soft_intrhand *softnet_intrhand;
#define setsoftnet()	softintr_schedule(softnet_intrhand)

#endif /* _M68K_SOFTINTR_H_ */
