/*      $NetBSD: clock.c,v 1.2 2024/01/08 05:11:32 thorpej Exp $	*/

/*-
 * Copyright (c) 2023 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: clock.c,v 1.2 2024/01/08 05:11:32 thorpej Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/clockvar.h>
#include <machine/psl.h>
#include <machine/bus.h>

static struct clock_vars {
	struct clock_attach_args *args;
	struct evcnt counter;
} virt68k_clocks[NCLOCKS];

static struct clock_handlers {
	const char *name;
	void (*func)(struct clockframe *);
} virt68k_clock_handlers[] = {
[CLOCK_HARDCLOCK]	=	{ .name = "hardclock",
				  .func = hardclock },
[CLOCK_STATCLOCK]	=	{ .name = "statclock",
				  .func = statclock },
};

void	*clock_devices[NCLOCKS];

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

static void (*clock_delay_func)(device_t, unsigned int);
static device_t clock_delay_dev;

/*
 * Common parts of clock autoconfiguration.
 */
void
clock_attach(device_t dev, struct clock_attach_args *ca,
    void (*delay_func)(device_t, unsigned int))
{
	/* Hook up that which we need. */
	KASSERT(ca->ca_which >= 0);
	KASSERT(ca->ca_which < NCLOCKS);

	KASSERT(clock_devices[ca->ca_which] == NULL);
	KASSERT(virt68k_clocks[ca->ca_which].args == NULL);

	clock_devices[ca->ca_which] = ca->ca_arg;
	virt68k_clocks[ca->ca_which].args = ca;

	evcnt_attach_dynamic(&virt68k_clocks[ca->ca_which].counter,
	    EVCNT_TYPE_INTR, ca->ca_parent_evcnt,
	    device_xname(dev), virt68k_clock_handlers[ca->ca_which].name);

	if (delay_func != NULL && clock_delay_dev == NULL) {
		aprint_normal_dev(dev, "Using as delay() timer.\n");
		clock_delay_func = delay_func;
		clock_delay_dev = dev;
	}
}

void
delay(unsigned int usec)
{
	if (clock_delay_func != NULL) {
		(*clock_delay_func)(clock_delay_dev, usec);
	}
}

const char *
clock_name(int which)
{
	KASSERT(which >= 0);
	KASSERT(which < NCLOCKS);

	return virt68k_clock_handlers[which].name;
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

	if (virt68k_clocks[CLOCK_HARDCLOCK].args == NULL) {
		panic("Clock not configured");
	}

	if (1000000 % hz) {
		aprint_error("Cannot get %d Hz clock; using 100 Hz\n", hz);
		hz = 100;
		tick = 1000000 / hz;
	}

	if (virt68k_clocks[CLOCK_STATCLOCK].args == NULL) {
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
		struct clock_attach_args *ca = virt68k_clocks[i].args;
		unsigned int freq, interval;

		if (ca == NULL) {
			continue;
		}
		switch (i) {
		case CLOCK_HARDCLOCK:
			freq = hz;
			interval = tick;
			break;

		case CLOCK_STATCLOCK:
			freq = stathz;
			interval = statint;
			break;

		default:
			KASSERT(0);
		}

		aprint_normal("Initialzing %s: freq=%u Hz, interval=%u usec\n",
		    clock_name(i), freq, interval);
		ca->ca_initfunc(ca->ca_arg, interval,
		    &virt68k_clocks[i].counter,
		    virt68k_clock_handlers[i].func);
	}
}

void
setstatclockrate(int newhz)
{

	/* XXX should we do something here? XXX */
}
