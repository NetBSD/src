/*	$NetBSD: clock.c,v 1.17 2006/09/05 01:38:59 rumble Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department, Ralph Campbell, and Kazumasa Utashiro of
 * Software Research Associates, Inc.
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
 * from: Utah $Hdr: clock.c 1.18 91/01/21$
 *
 *	@(#)clock.c	8.1 (Berkeley) 6/11/93
 */
/*
 * Copyright (c) 1988 University of Utah.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department, Ralph Campbell, and Kazumasa Utashiro of
 * Software Research Associates, Inc.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 * from: Utah $Hdr: clock.c 1.18 91/01/21$
 *
 *	@(#)clock.c	8.1 (Berkeley) 6/11/93
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: clock.c,v 1.17 2006/09/05 01:38:59 rumble Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <machine/sysconf.h>

#include <mips/locore.h>
#include <dev/clock_subr.h>
#include <dev/ic/i8253reg.h>

#include <machine/machtype.h>
#include <sgimips/sgimips/clockvar.h>

u_int32_t next_clk_intr;
u_int32_t missed_clk_intrs;
unsigned long last_clk_intr;

void mips1_clock_intr(u_int32_t, u_int32_t, u_int32_t, u_int32_t);
void mips3_clock_intr(u_int32_t, u_int32_t, u_int32_t, u_int32_t);
unsigned long mips1_clkread(void);
unsigned long mips3_clkread(void);

/*
 * Machine-dependent clock routines.
 */

/*
 * We assume newhz is either stathz or profhz, and that neither will
 * change after being set up above.  Could recalculate intervals here
 * but that would be a drag.
 */
void
setstatclockrate(int newhz)
{

	/* do something? */
}

/*
 * Set up the real-time and statistics clocks.  Leave stathz 0 only if
 * no alternative timer is available.
 */
void
cpu_initclocks()
{

#if defined(MIPS3)
	if (mach_type != MACH_SGI_IP12) {
		next_clk_intr = mips3_cp0_count_read()
		    + curcpu()->ci_cycles_per_hz;
		mips3_cp0_compare_write(next_clk_intr);

		/* number of microseconds between interrupts */
		tick = 1000000 / hz;
		tickfix = 1000000 - (hz * tick);
		if (tickfix) {
			int ftp;

			ftp = min(ffs(tickfix), ffs(hz));
			tickfix >>= (ftp - 1);
			tickfixinterval = hz >> (ftp - 1);
		}
	}
#endif /* MIPS3 */
}

void
mips1_clock_intr(u_int32_t status, u_int32_t cause, u_int32_t pc,
		 u_int32_t ipending)
{
	struct clockframe cf;

	cf.pc = pc;
	cf.sr = status;

	hardclock(&cf);

	switch (mach_type) {
	case MACH_SGI_IP12:
		/* XXX - we need to strobe on ip12. abstract me?! */
		*(volatile u_int32_t *)MIPS_PHYS_TO_KSEG1(0x1fb801e0) = 1;
		break;
	}
}

unsigned long
mips1_clkread()
{

	return 0;
}

#if defined(MIPS3)
void
mips3_clock_intr(u_int32_t status, u_int32_t cause, u_int32_t pc,
		 u_int32_t ipending)
{
        u_int32_t newcnt;
	struct clockframe cf;

	last_clk_intr = mips3_cp0_count_read();

	next_clk_intr += curcpu()->ci_cycles_per_hz;
	mips3_cp0_compare_write(next_clk_intr);
	newcnt = mips3_cp0_count_read();

	/*
	 * Missed one or more clock interrupts, so let's start
	 * counting again from the current value.
	 */
	if ((next_clk_intr - newcnt) & 0x80000000) {
		missed_clk_intrs++;

		next_clk_intr = newcnt + curcpu()->ci_cycles_per_hz;
		mips3_cp0_compare_write(next_clk_intr);
	}

	cf.pc = pc;
	cf.sr = status;

	hardclock(&cf);
}

unsigned long
mips3_clkread()
{
	uint32_t res, count;

	count = mips3_cp0_count_read() - last_clk_intr;
	MIPS_COUNT_TO_MHZ(curcpu(), count, res);
	return (res);
}
#endif /* MIPS3 */
