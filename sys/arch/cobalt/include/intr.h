/*	$NetBSD: intr.h,v 1.32.22.1 2011/06/06 09:05:14 jruoho Exp $	*/

/*
 * Copyright (c) 2000 Soren S. Jorvang.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef	_COBALT_INTR_H_
#define	_COBALT_INTR_H_

#include <mips/intr.h>

#ifdef _KERNEL
#ifndef _LOCORE

#include <sys/evcnt.h>
#include <mips/cpuregs.h>

#define NCPU_INT	6
#define NICU_INT	16

struct cobalt_intrhand {
	LIST_ENTRY(cobalt_intrhand) ih_q;
	int (*ih_func)(void *);
	void *ih_arg;
	int ih_irq;
	int ih_cookie_type;
#define	COBALT_COOKIE_TYPE_CPU	0x1
#define	COBALT_COOKIE_TYPE_ICU	0x2
};

void intr_init(void);
void *cpu_intr_establish(int, int, int (*)(void *), void *);
void *icu_intr_establish(int, int, int, int (*)(void *), void *);
void cpu_intr_disestablish(void *);
void icu_intr_disestablish(void *);

#endif /* !_LOCORE */
#endif /* _KERNEL */

#endif	/* !_COBALT_INTR_H_ */
