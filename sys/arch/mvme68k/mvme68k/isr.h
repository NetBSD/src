/*	$NetBSD: isr.h,v 1.13 2024/01/16 01:26:34 thorpej Exp $	*/

/*-
 * Copyright (c) 2024 The NetBSD Foundation, Inc.
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

#ifndef _MVME68K_ISR_H_
#define	_MVME68K_ISR_H_

#include <sys/intr.h>

/*
 * Aliases for the legacy mvme68k ISR routines.
 */

static inline void
isrinit(void)
{
	extern int nmihand(void *);

	m68k_intr_init(NULL);
	m68k_intr_establish(nmihand, NULL, NULL, 0, 7, 0, 0);
}

static inline void
isrlink_autovec(int (*func)(void *), void *arg, int ipl, int isrpri,
    struct evcnt *ev)
{
	/* XXX leaks interrupt handle. */
	m68k_intr_establish(func, arg, ev, 0, ipl, isrpri, 0);
}

static inline void
isrlink_vectored(int (*func)(void *), void *arg, int ipl, int vec,
    struct evcnt *ev)
{
	/* XXX leaks interrupt handle. */
	m68k_intr_establish(func, arg, ev, vec, ipl, 0, 0);
}

static inline struct evcnt *
isrlink_evcnt(int ipl)
{
	KASSERT(ipl >= 0 && ipl <= 7);
	return &m68k_intr_evcnt[ipl];
}

static inline void
isrunlink_vectored(int vec)
{
	/* XXX isrlink_vectored() should return a handle. */
	void *ih = m68k_intrvec_intrhand(vec);
	if (ih != NULL) {
		m68k_intr_disestablish(ih);
	}
}

#endif /* _MVME68K_ISR_H_ */
