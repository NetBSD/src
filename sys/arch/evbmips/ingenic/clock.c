/*	$NetBSD: clock.c,v 1.1 2014/11/22 15:17:02 macallan Exp $ */

/*-
 * Copyright (c) 2014 Michael Lorenz
 * All rights reserved.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: clock.c,v 1.1 2014/11/22 15:17:02 macallan Exp $");

#include <sys/param.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <mips/ingenic/ingenic_regs.h>

void
cpu_initclocks(void)
{

	/* timecounter setup and such */
}

/* shamelessly stolen from mips3_clock.c */
void
delay(int n)
{
	u_long divisor_delay;
	uint32_t cur, last, delta, usecs;

	last = readreg(JZ_OST_CNT_LO);
	delta = usecs = 0;

	divisor_delay = curcpu()->ci_divisor_delay;
	if (divisor_delay == 0) {
		/*
		 * Frequency values in curcpu() are not initialized.
		 * Assume faster frequency since longer delays are harmless.
		 * Note CPU_MIPS_DOUBLE_COUNT is ignored here.
		 */
#define FAST_FREQ	(300 * 1000 * 1000)	/* fast enough? */
		divisor_delay = FAST_FREQ / (1000 * 1000);
	}
	while (n > usecs) {
		cur = readreg(JZ_OST_CNT_LO);

		/*
		 * We setup the OS timer to always counts upto UINT32_MAX,
		 * so no need to check wrapped around case.
		 */
		delta += (cur - last);

		last = cur;

		while (delta >= divisor_delay) {
			/*
			 * delta is not so larger than divisor_delay here,
			 * and using DIV/DIVU ops could be much slower.
			 * (though longer delay may be harmless)
			 */
			usecs++;
			delta -= divisor_delay;
		}
	}
}

void
setstatclockrate(int r)
{
	/* nothing to see here */
}
