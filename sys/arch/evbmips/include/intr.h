/*	$NetBSD: intr.h,v 1.19.12.1 2017/12/03 11:36:09 jdolecek Exp $	*/

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

#ifndef _EVBMIPS_INTR_H_
#define	_EVBMIPS_INTR_H_

#include <sys/queue.h>
#include <mips/intr.h>

#ifdef	_KERNEL

struct evbmips_intrhand {
	LIST_ENTRY(evbmips_intrhand) ih_q;
	int (*ih_func)(void *);
	void *ih_arg;
	const void *ih_irqmap;			/* for algor */
	int ih_irq;
	int ih_ipl;
};

struct clockframe;

void	intr_init(void);
void	evbmips_intr_init(void);
void	evbmips_iointr(int, uint32_t, struct clockframe *);
void	*evbmips_intr_establish(int, int (*)(void *), void *);
void	evbmips_intr_disestablish(void *);

#endif /* _KERNEL */
#endif /* ! _EVBMIPS_INTR_H_ */
