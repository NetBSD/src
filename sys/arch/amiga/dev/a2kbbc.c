/*	$NetBSD: a2kbbc.c,v 1.22.2.1 2011/06/23 14:18:58 cherry Exp $ */

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1982, 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 *	@(#)clock.c	7.6 (Berkeley) 5/7/91
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: a2kbbc.c,v 1.22.2.1 2011/06/23 14:18:58 cherry Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <machine/psl.h>
#include <machine/cpu.h>
#include <amiga/amiga/device.h>
#include <amiga/amiga/custom.h>
#include <amiga/amiga/cia.h>
#include <amiga/dev/rtc.h>
#include <amiga/dev/zbusvar.h>

#include <dev/clock_subr.h>

int a2kbbc_match(device_t, cfdata_t, void *);
void a2kbbc_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(a2kbbc, 0,
    a2kbbc_match, a2kbbc_attach, NULL, NULL);

void *a2kclockaddr;
int a2kugettod(todr_chip_handle_t, struct clock_ymdhms *);
int a2kusettod(todr_chip_handle_t, struct clock_ymdhms *);
static struct todr_chip_handle a2ktodr;

int
a2kbbc_match(device_t pdp, cfdata_t cfp, void *auxp)
{
	struct clock_ymdhms dt;
	static int a2kbbc_matched = 0;

	if (!matchname("a2kbbc", auxp))
		return (0);

	/* Allow only one instance. */
	if (a2kbbc_matched)
		return (0);

	if (/* is_a1200() || */ is_a3000() || is_a4000()
#ifdef DRACO
	    || is_draco()
#endif
	    )
		return (0);

	a2kclockaddr = (void *)__UNVOLATILE(ztwomap(0xdc0000));
	if (a2kugettod(&a2ktodr, &dt) != 0)
		return (0);

	a2kbbc_matched = 1;
	return (1);
}

/*
 * Attach us to the rtc function pointers.
 */
void
a2kbbc_attach(device_t pdp, device_t dp, void *auxp)
{
	printf("\n");
	a2kclockaddr = (void *)__UNVOLATILE(ztwomap(0xdc0000));

	a2ktodr.cookie = a2kclockaddr;
	a2ktodr.todr_gettime_ymdhms = a2kugettod;
	a2ktodr.todr_settime_ymdhms = a2kusettod;
	todr_attach(&a2ktodr);
}

int
a2kugettod(todr_chip_handle_t h, struct clock_ymdhms *dt)
{
	struct rtclock2000 *rt;
	int i;

	rt = a2kclockaddr;

	/*
	 * hold clock
	 */
	rt->control1 |= A2CONTROL1_HOLD;
	i = 0x1000;
	while (rt->control1 & A2CONTROL1_BUSY && i--)
		;
	if (rt->control1 & A2CONTROL1_BUSY)
		return (ENXIO);		/* Give up and say it's not there */

	/* Copy the info.  Careful about the order! */
	dt->dt_sec   = rt->second1 * 10 + rt->second2;
	dt->dt_min   = rt->minute1 * 10 + rt->minute2;
	dt->dt_hour  = (rt->hour1 & 3) * 10 + rt->hour2;
	dt->dt_day   = rt->day1    * 10 + rt->day2;
	dt->dt_mon   = rt->month1  * 10 + rt->month2;
	dt->dt_year  = rt->year1   * 10 + rt->year2;
	dt->dt_wday  = rt->weekday;

	/*
	 * The oki clock chip has a register to put the clock into
	 * 12/24h mode.
	 *
	 *  clockmode   |    A2HOUR1_PM
	 *  24h   12h   |  am = 0, pm = 1
	 * ---------------------------------
	 *   0    12    |       0
	 *   1     1    |       0
	 *  ..    ..    |       0
	 *  11    11    |       0
	 *  12    12    |       1
	 *  13     1    |       1
	 *  ..    ..    |       1
	 *  23    11    |       1
	 *
	 */

	if ((rt->control3 & A2CONTROL3_24HMODE) == 0) {
		if ((rt->hour1 & A2HOUR1_PM) == 0 && dt->dt_hour == 12)
			dt->dt_hour = 0;
		else if ((rt->hour1 & A2HOUR1_PM) && dt->dt_hour != 12)
			dt->dt_hour += 12;
	}

	/*
	 * release the clock
	 */
	rt->control1 &= ~A2CONTROL1_HOLD;

	dt->dt_year += CLOCK_BASE_YEAR;
	if (dt->dt_year < STARTOFTIME)
		dt->dt_year += 100;

	/*
	 * Note that this check is redundant with one in kern_todr.c, but
	 * attach relies on it being here.
	 */
	if ((dt->dt_hour > 23) ||
	    (dt->dt_day  > 31) ||
	    (dt->dt_mon  > 12) ||
	    /* (dt->dt_year < STARTOFTIME) || */ (dt->dt_year > 2036))
		return (EINVAL);

	return (0);
}

int
a2kusettod(todr_chip_handle_t h, struct clock_ymdhms *dt)
{
	struct rtclock2000 *rt;
	int ampm, i;

	rt = a2kclockaddr;
	/*
	 * there seem to be problems with the bitfield addressing
	 * currently used..
	 */
	if (! rt)
		return (ENXIO);

	/*
	 * hold clock
	 */
	rt->control1 |= A2CONTROL1_HOLD;
	i = 0x1000;
	while (rt->control1 & A2CONTROL1_BUSY && i--)
		;
	if (rt->control1 & A2CONTROL1_BUSY)
		return (ENXIO);		/* Give up and say it's not there */

	ampm = 0;
	if ((rt->control3 & A2CONTROL3_24HMODE) == 0) {
		if (dt->dt_hour >= 12) {
			ampm = A2HOUR1_PM;
			if (dt->dt_hour != 12)
				dt->dt_hour -= 12;
		} else if (dt->dt_hour == 0) {
			dt->dt_hour = 12;
		}
	}
	rt->hour1   = (dt->dt_hour / 10) | ampm;
	rt->hour2   = dt->dt_hour % 10;
	rt->second1 = dt->dt_sec / 10;
	rt->second2 = dt->dt_sec % 10;
	rt->minute1 = dt->dt_min / 10;
	rt->minute2 = dt->dt_min % 10;
	rt->day1    = dt->dt_day / 10;
	rt->day2    = dt->dt_day % 10;
	rt->month1  = dt->dt_mon / 10;
	rt->month2  = dt->dt_mon % 10;
	rt->year1   = (dt->dt_year / 10) % 10;
	rt->year2   = dt->dt_year % 10;
	rt->weekday = dt->dt_wday;

	/*
	 * release the clock
	 */
	rt->control2 &= ~A2CONTROL1_HOLD;

	return (0);
}
