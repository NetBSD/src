/*      $NetBSD: clock.c,v 1.2 2026/08/07 23:37:46 thorpej Exp $	*/

/*-
 * Copyright (c) 2023, 2025 The NetBSD Foundation, Inc.
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

/*
 * Copyright (c) 1992, 1993
 *      The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *      This product includes software developed by the University of
 *      California, Lawrence Berkeley Laboratory.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *      @(#)clock.c     8.1 (Berkeley) 6/11/93
 */

#include "opt_fdt.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: clock.c,v 1.2 2026/08/07 23:37:46 thorpej Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/clockvar.h>
#include <machine/psl.h>
#include <machine/bus.h>

static struct clock_vars {
	const char *name;
	struct clock_if *clkif;
	struct evcnt counter;
} m68k_clocks[NCLOCKS] = {
	[CLOCK_HARDCLOCK] = {
		.name = "hardclock",
	},
	[CLOCK_STATCLOCK] = {
		.name = "statclock",
	},
};

static TAILQ_HEAD(, clock_if) clock_candidates =
    TAILQ_HEAD_INITIALIZER(clock_candidates);

/*
 * Statistics clock interval and variance, in usec.  Variance must be a
 * power of two.  Since this gives us an even number, not an odd number,
 * we discard one case and compensate.  That is, a variance of 1024 would 
 * give us offsets in [0..1023].  Instead, we take offsets in [1..1023].
 * This is symmetric about the point 512, or statvar/2, and thus averages
 * to that value (assuming uniform random numbers).
 */
/* XXX fix comment to match value */
int	clock_statvar = 8192;
int	clock_statmin;		/* statclock interval - (1/2 * variance) */

#include <dev/fdt/fdtvar.h>
#include <dev/ofw/openfirm.h>
#include <libfdt.h>

/*
 * Given a device phandle, see if it is one of the system clock
 * sources.
 */
static int
clock_from_phandle(int phandle)
{
	static const char * propnames[NCLOCKS] = {
		[CLOCK_HARDCLOCK] = "netbsd,hardclock",
		[CLOCK_STATCLOCK] = "netbsd,statclock",
	};
	const int off = fdt_path_offset(fdtbus_get_data(), "/chosen");
	const char *propval;

	if (off < 0) {
		return CLOCK_NONE;
	}

	for (int i = 0; i < NCLOCKS; i++) {
		propval = fdt_getprop(fdtbus_get_data(), off,
		    propnames[i], NULL);
		if (propval == NULL) {
			continue;
		}
		if (*propval != '/') {
			/* Alias */
			propval = fdt_get_alias(fdtbus_get_data(), propval);
			if (propval == NULL) {
				continue;
			}
		}
		if (OF_finddevice(propval) == phandle) {
			return i;
		}
	}
	return CLOCK_NONE;
}

static int
clock_hardclock_intr(void *v)
{
	struct clock_if * const clk = m68k_clocks[CLOCK_HARDCLOCK].clkif;

	/* ACK interrupt, re-load timer. */
	(*clk->clk_intr)(clk);

	m68k_clocks[CLOCK_HARDCLOCK].counter.ev_count++;
	hardclock((struct clockframe *)v);

	return 1;
}

static int
clock_statclock_intr(void *v)
{
	struct clock_if * const clk = m68k_clocks[CLOCK_STATCLOCK].clkif;

	/* ACK interrupt, re-load timer. */
	/* XXX add variance */
	(*clk->clk_intr)(clk);

	m68k_clocks[CLOCK_STATCLOCK].counter.ev_count++;
	statclock((struct clockframe *)v);

	return 1;
}

/*
 * Common parts of clock autoconfiguration.
 */
void
clock_attach(struct clock_if *clk)
{
	clk->clk_which =
	    clock_from_phandle(devhandle_to_of(device_handle(clk->clk_dev)));
	if (clk->clk_which == CLOCK_NONE) {
		TAILQ_INSERT_TAIL(&clock_candidates, clk, clk_list);
	} else {
		TAILQ_INSERT_HEAD(&clock_candidates, clk, clk_list);
	}
}

static const char *
clock_name(int which)
{
	KASSERT(which >= 0);
	KASSERT(which < NCLOCKS);

	return m68k_clocks[which].name;
}

/*
 * Set up the real-time and statistics clocks.  Leave stathz 0 only
 * if no alternative timer is available.
 *
 * The frequencies of these clocks must be an even number of microseconds.
 */
void
cpu_initclocks(void)
{
	int i, statint = 0, minint = 0;
	struct clock_if *clk;

	/*
	 * Assign the clocks.  All the clocks with pre-assigned roles
	 * are at the head of the list, and all of the unassigned clocks
	 * are at the end.
	 */
	TAILQ_FOREACH(clk, &clock_candidates, clk_list) {
		if (clk->clk_which != CLOCK_NONE) {
			KASSERT(m68k_clocks[clk->clk_which].clkif == NULL);
			m68k_clocks[clk->clk_which].clkif = clk;
		} else if (m68k_clocks[CLOCK_HARDCLOCK].clkif == NULL) {
			clk->clk_which = CLOCK_HARDCLOCK;
			m68k_clocks[CLOCK_HARDCLOCK].clkif = clk;
		} else if (m68k_clocks[CLOCK_STATCLOCK].clkif == NULL) {
			clk->clk_which = CLOCK_STATCLOCK;
			m68k_clocks[CLOCK_STATCLOCK].clkif = clk;
		}
	}

	if (m68k_clocks[CLOCK_HARDCLOCK].clkif == NULL) {
		panic("Clock not configured");
	}

	if (1000000 % hz) {
		aprint_error("Cannot get %d Hz clock; using 100 Hz\n", hz);
		hz = 100;
		tick = 1000000 / hz;
	}

	if (m68k_clocks[CLOCK_STATCLOCK].clkif == NULL) {
		aprint_normal("No statclock; using hardclock.\n");
		stathz = 0;
		statint = 0;
	} else if (stathz == 0) {
		stathz = hz;
		if (1000000 % stathz) {
			aprint_error("Cannot get %d Hz statclock; "
				     "using 100 Hz\n", stathz);
			stathz = 100;
		}
		profhz = stathz;	/* always */
		statint = 1000000 / stathz;
		minint = statint / 2 + 100;
		while (clock_statvar > minint)
			clock_statvar >>= 1;
		clock_statmin = statint - (clock_statvar >> 1);
	}

	for (i = 0; i < NCLOCKS; i++) {
		clk = m68k_clocks[i].clkif;
		unsigned int freq, interval;
		int (*func)(void *);
		int error;

		if (clk == NULL) {
			continue;
		}
		switch (i) {
		case CLOCK_HARDCLOCK:
			freq = hz;
			interval = tick;
			func = clock_hardclock_intr;
			break;

		case CLOCK_STATCLOCK:
			freq = stathz;
			interval = statint;
			func = clock_statclock_intr;
			break;

		default:
			KASSERT(0);
		}

		evcnt_attach_dynamic(&m68k_clocks[i].counter,
		    EVCNT_TYPE_INTR, clk->clk_parent_evcnt,
		    device_xname(clk->clk_dev), m68k_clocks[i].name);

		aprint_normal_dev(clk->clk_dev,
		    "%s: freq=%u Hz, interval=%u usec\n",
		    clock_name(i), freq, interval);
		error = (*clk->clk_init)(clk, func);
		if (error) {
			panic("Failed to initialize %s for %s (error %d)",
			    device_xname(clk->clk_dev),
			    clock_name(i), error);
		}
		error = (*clk->clk_arm)(clk, interval);
		if (error) {
			panic("Failed to arm %s for %s (error %d)",
			    device_xname(clk->clk_dev),
			    clock_name(i), error);
		}
	}
}

void
setstatclockrate(int newhz)
{

	/* XXX should we do something here? XXX */
}
