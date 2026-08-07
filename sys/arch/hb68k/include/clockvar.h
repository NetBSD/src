/*	$NetBSD: clockvar.h,v 1.2 2026/08/07 23:37:46 thorpej Exp $	*/

/*-
 * Copyright (c) 1996, 2002, 2023 The NetBSD Foundation, Inc.
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

#ifndef _PG68K_CLOCKVAR_H_
#define _PG68K_CLOCKVAR_H_

#ifdef _KERNEL_OPT
#include "opt_fdt.h"
#endif

#include <sys/evcnt.h>
#include <sys/queue.h>
#include <dev/clock_subr.h>

extern	int clock_statvar;
extern	int clock_statmin;
struct clockframe;

#define	CLOCK_NONE		-1
#define	CLOCK_HARDCLOCK		0
#define	CLOCK_STATCLOCK		1
#define	NCLOCKS			2

struct clock_if {
	TAILQ_ENTRY(clock_if)	clk_list;

	/* Initialized by clock hardware driver */
	device_t		clk_dev;
	struct evcnt		*clk_parent_evcnt;
	void			(*clk_delaycal)(struct clock_if *,
						unsigned int *);
	int			(*clk_init)(struct clock_if *, int (*)(void *));
	int			(*clk_arm)(struct clock_if *, unsigned int);
	void			(*clk_intr)(struct clock_if *);
	void			(*clk_disarm)(struct clock_if *);

	/* Initialized by clock_attach() */
	int			clk_which;
};

void	clock_attach(struct clock_if *);

/*
 * Macro to compute a new randomized interval.  The intervals are
 * uniformly distributed on [statint - statvar / 2, statint + statvar / 2],
 * and therefore have mean statint, giving a stathz frequency clock.
 *
 * This is gratuitously stolen from sparc/sparc/clock.c
 */
#define CLOCK_NEWINT(statvar, statmin)	({				\
		u_long r, var = (statvar);				\
		do { r = random() & (var - 1); } while (r == 0);	\
		(statmin + r);						\
	})

#endif /* _PG68K_CLOCKVAR_H_ */
