/*	$NetBSD: clock.c,v 1.4 1999/02/15 04:38:06 sakamoto Exp $	*/

/*
 * Copyright (C) 1995, 1996 Wolfgang Solfrank.
 * Copyright (C) 1995, 1996 TooLs GmbH.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <dev/isa/isareg.h>
#include <dev/ic/i8253reg.h>

#define FIRST_GUESS	0x2000

static u_long ns_per_tick;

static inline u_quad_t
mftb()
{
	u_long scratch;
	u_quad_t tb;
	
	asm ("1: mftbu %0; mftb %0+1; mftbu %1; cmpw %0,%1; bne 1b"
	    : "=r"(tb), "=r"(scratch));
	return tb;
}

/*
 * Wait for about n microseconds (at least!).
 */
void
delay(n)
	int n;
{
	u_quad_t tb;
	u_long tbh, tbl, scratch;
	
	tb = mftb();
	tb += (n * 1000 + ns_per_tick - 1) / ns_per_tick;
	tbh = tb >> 32;
	tbl = tb;
	asm ("1: mftbu %0; cmpw %0,%1; blt 1b; bgt 2f; mftb %0; cmpw %0,%2; blt 1b; 2:"
	     :: "r"(scratch), "r"(tbh), "r"(tbl));
}

int
findcpuspeed()
{
	int i;
	int ticks_per_sec;
	u_short remainder;
	u_quad_t tstart, tend;

	/* Put counter in count down mode */
	outb(IO_TIMER1 + TIMER_MODE, TIMER_SEL0|TIMER_16BIT|TIMER_RATEGEN);
	outb(IO_TIMER1 + TIMER_CNTR0, 0xff);	/* lo */
	outb(IO_TIMER1 + TIMER_CNTR0, 0xff);	/* hi */
	for (i = FIRST_GUESS; i; i--)
		;
	/* Read the value left in the counter */
	outb(IO_TIMER1 + TIMER_MODE, TIMER_SEL0|TIMER_LATCH);
	remainder = inb(IO_TIMER1 + TIMER_CNTR0);
	remainder += (inb(IO_TIMER1 + TIMER_CNTR0) << 8);

	tstart = mftb();
	for (i = FIRST_GUESS; i; i--)
		;
	tend = mftb();
	if (tend > tstart)
		tend -= tstart;
	else
		tend += UQUAD_MAX - tstart;

	ticks_per_sec = (int)(tend * TIMER_FREQ / (0xffff - remainder));
	if (ticks_per_sec > 8000000)		/* XXX */
		ticks_per_sec = 33000000 / 4;
	else
		ticks_per_sec = 25000000 / 4;
	ns_per_tick = 1000000000 / ticks_per_sec;

	return (ticks_per_sec);
}
