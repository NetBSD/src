/* $NetBSD: clock.c,v 1.47 2022/04/05 04:33:36 skrll Exp $ */

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department and Ralph Campbell.
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
 * from: Utah Hdr: clock.c 1.18 91/01/21
 *
 *	@(#)clock.c	8.1 (Berkeley) 6/10/93
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: clock.c,v 1.47 2022/04/05 04:33:36 skrll Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/lwp.h>

#include <machine/alpha.h>
#include <machine/autoconf.h>
#include <machine/cpuconf.h>
#include <machine/cpu_counter.h>

#include <alpha/alpha/clockvar.h>

#define PCC_QUAL	1000

void (*clock_init)(void *);
void *clockdev;

int	alpha_use_cctr;		/* != 0 if we're using the PCC timecounter */

void
clockattach(void (*fns)(void *), void *dev)
{

	/*
	 * Just bookkeeping.  We only allow one system clock.  If
	 * we're running on real hardware, enforce this.  If we're
	 * running under Qemu, anything after the first one.
	 */
	if (clock_init != NULL) {
		if (alpha_is_qemu) {
			return;
		}
		panic("clockattach: multiple clocks");
	}
	clock_init = fns;
	clockdev = dev;
}

/*
 * Start the real-time and statistics clocks. Leave stathz 0 since there
 * are no other timers available.
 */
void
cpu_initclocks(void)
{

	if (clock_init == NULL)
		panic("cpu_initclocks: no clock attached");

	/*
	 * Establish the clock interrupt; it's a special case.
	 *
	 * We establish the clock interrupt this late because if
	 * we do it at clock attach time, we may have never been at
	 * spl0() since taking over the system.  Some versions of
	 * PALcode save a clock interrupt, which would get delivered
	 * when we spl0() in autoconf.c.  If established the clock
	 * interrupt handler earlier, that interrupt would go to
	 * hardclock, which would then fall over because p->p_stats
	 * isn't set at that time.
	 */
	platform.clockintr = hardclock;
	schedhz = 16;

	/*
	 * Initialize PCC timecounter, unless we're running in Qemu
	 * (we will use a different timecounter in that case).
	 */
	if (!alpha_is_qemu) {
		const uint64_t pcc_freq = cpu_frequency(curcpu());
		cc_init(NULL, pcc_freq, "PCC", PCC_QUAL);
		alpha_use_cctr = 1;
	}

	/*
	 * Get the clock started.
	 */
	(*clock_init)(clockdev);
}

/*
 * Some platforms might have other per-cpu clock initialization.  This
 * is handled here.
 */
void
cpu_initclocks_secondary(void)
{
	(*clock_init)(clockdev);
}

/*
 * We assume newhz is either stathz or profhz, and that neither will
 * change after being set up above.  Could recalculate intervals here
 * but that would be a drag.
 */
void
setstatclockrate(int newhz)
{

	/* nothing we can do */
}
