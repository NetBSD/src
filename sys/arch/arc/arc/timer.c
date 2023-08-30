/* $NetBSD: timer.c,v 1.12 2023/08/30 17:10:17 tsutsui Exp $ */
/* NetBSD: clock.c,v 1.31 2001/05/27 13:53:24 sommerfeld Exp  */

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
__KERNEL_RCSID(0, "$NetBSD: timer.c,v 1.12 2023/08/30 17:10:17 tsutsui Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <mips/locore.h>
#include <mips/mips3_clock.h>

#include <arc/arc/timervar.h>

device_t timerdev;
const struct timerfns *timerfns;
int timerinitted;
uint32_t last_cp0_count;

void
timerattach(device_t dev, const struct timerfns *fns)
{

	/*
	 * Just bookkeeping.
	 */

	if (timerfns != NULL)
		panic("timerattach: multiple timers");
	timerdev = dev;
	timerfns = fns;
}

/*
 * Machine-dependent clock routines.
 */

/*
 * Start the real-time and statistics clocks. Leave stathz 0 since there
 * are no other timers available.
 */
void
cpu_initclocks(void)
{

	if (timerfns == NULL)
		panic("cpu_initclocks: no timer attached");

	/*
	 * Get the clock started.
	 */
	(*timerfns->tf_init)(timerdev);

	/* init timecounter */
	mips3_init_tc();
}

/*
 * This does not need to do anything, as we don't use a timer for statclock
 * and profhz == stathz == hz.
 */
void
setstatclockrate(int newhz)
{

	/* nothing to do */
}
