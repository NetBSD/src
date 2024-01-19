/*	$NetBSD: intr.h,v 1.39 2024/01/19 03:09:04 thorpej Exp $	*/

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

#ifndef _HP300_INTR_H_
#define _HP300_INTR_H_

#ifdef _KERNEL

#include <m68k/psl.h>

#define	MACHINE_PSL_IPL_SOFTCLOCK	PSL_IPL1
#define	MACHINE_PSL_IPL_SOFTBIO		PSL_IPL1
#define	MACHINE_PSL_IPL_SOFTNET		PSL_IPL1
#define	MACHINE_PSL_IPL_SOFTSERIAL	PSL_IPL1
#define	MACHINE_PSL_IPL_VM		PSL_IPL5
#define	MACHINE_PSL_IPL_SCHED		PSL_IPL6

#define	MACHINE_INTREVCNT_NAMES						\
	{ "spurious", "lev1", "lev2", "lev3", "lev4", "lev5", "clock", "nmi" }

#if defined(_M68K_INTR_PRIVATE) && defined(_KERNEL_OPT)
#include "audio.h"
#if NAUDIO > 0
/*
 * Audio interrupts also come in on level 6, and those are dispatched
 * from a custom stub which handles the clock inline before calling the
 * general interrupt dispatch.  We want to suppress warning about stray
 * level 6 interrupts in the common code in case the clock interrupted
 * but audio did not.
 */
#define	MACHINE_AUTOVEC_IGNORE_STRAY(x)	((x) == 6)
#endif
#endif

#endif /* _KERNEL */

#include <m68k/intr.h>

#ifdef _KERNEL

#ifdef _M68K_INTR_PRIVATE
struct hp300_intrhand {
	struct m68k_intrhand       ih_super;
	LIST_ENTRY(hp300_intrhand) ih_dio_link;
};
#endif

/* These spl calls are _not_ to be used by machine-independent code. */
#define	splhil()	splraise1()
#define	splkbd()	splhil()

/*
 * Interface wrappers.
 */

static inline void *
intr_establish(int (*func)(void *), void *arg, int ipl, int isrpri)
{
	return m68k_intr_establish(func, arg, (void *)0, 0, ipl, isrpri, 0);
}

static inline void
intr_disestablish(void *ih)
{
	m68k_intr_disestablish(ih);
}

#endif /* _KERNEL */

#endif	/* _HP300_INTR_H */
