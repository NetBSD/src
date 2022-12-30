/*	$NetBSD: rtc.c,v 1.2 2022/12/30 21:40:20 jakllsch Exp $	*/

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz and Don Ahn.
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
 *	@(#)clock.c	7.2 (Berkeley) 5/12/91
 */
/*-
 * Copyright (c) 1993, 1994 Charles M. Hannum.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz and Don Ahn.
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
 *	@(#)clock.c	7.2 (Berkeley) 5/12/91
 */
/* 
 * Mach Operating System
 * Copyright (c) 1991,1990,1989 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/*
  Copyright 1988, 1989 by Intel Corporation, Santa Clara, California.

		All Rights Reserved

Permission to use, copy, modify, and distribute this software and
its documentation for any purpose and without fee is hereby
granted, provided that the above copyright notice appears in all
copies and that both the copyright notice and this permission notice
appear in supporting documentation, and that the name of Intel
not be used in advertising or publicity pertaining to distribution
of the software without specific, written prior permission.

INTEL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS,
IN NO EVENT SHALL INTEL BE LIABLE FOR ANY SPECIAL, INDIRECT, OR
CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT,
NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

/*
 * Primitive RTC chip routines.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rtc.c,v 1.2 2022/12/30 21:40:20 jakllsch Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/timetc.h>
#include <sys/kernel.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>
#include <i386/isa/nvram.h>

#include <machine/pio.h>

#include <dev/ic/mc146818reg.h>
#include <x86/rtc.h>

#ifndef __x86_64__
#include "mca.h"
#endif
#if NMCA > 0
#include <machine/mca_machdep.h>	/* for MCA_system */
#endif

#include "acpica.h"
#if NACPICA > 0
#include <dev/acpi/acpivar.h>
#endif

static void	rtcinit(void);
static int	rtcget(mc_todregs *);
static void	rtcput(mc_todregs *);
static int	cmoscheck(void);
static int	clock_expandyear(int);

/* XXX use sc? */
u_int
mc146818_read(void *sc, u_int reg)
{

	outb(IO_RTC, reg);
	return (inb(IO_RTC+1));
}

void
mc146818_write(void *sc, u_int reg, u_int datum)
{

	outb(IO_RTC, reg);
	outb(IO_RTC+1, datum);
}

static void
rtcinit(void)
{
	static int first_rtcopen_ever = 1;

	if (!first_rtcopen_ever)
		return;
	first_rtcopen_ever = 0;

	mc146818_write(NULL, MC_REGA,			/* XXX softc */
	    MC_BASE_32_KHz | MC_RATE_1024_Hz);
	mc146818_write(NULL, MC_REGB, MC_REGB_24HR);	/* XXX softc */
}

static int
rtcget(mc_todregs *regs)
{

	rtcinit();
	if ((mc146818_read(NULL, MC_REGD) & MC_REGD_VRT) == 0) /* XXX softc */
		return (-1);
	MC146818_GETTOD(NULL, regs);			/* XXX softc */
	return (0);
}	

static void
rtcput(mc_todregs *regs)
{

	rtcinit();
	MC146818_PUTTOD(NULL, regs);			/* XXX softc */
}

/*
 * check whether the CMOS layout is "standard"-like (ie, not PS/2-like),
 * to be called at splclock()
 */
static int
cmoscheck(void)
{
	int i;
	unsigned short cksum = 0;

	for (i = 0x10; i <= 0x2d; i++)
		cksum += mc146818_read(NULL, i); /* XXX softc */

	return (cksum == (mc146818_read(NULL, 0x2e) << 8)
			  + mc146818_read(NULL, 0x2f));
}

#if NMCA > 0
/*
 * Check whether the CMOS layout is PS/2 like, to be called at splclock().
 */
static int cmoscheckps2(void);
static int
cmoscheckps2(void)
{
#if 0
	/* Disabled until I find out the CRC checksum algorithm IBM uses */
	int i;
	unsigned short cksum = 0;

	for (i = 0x10; i <= 0x31; i++)
		cksum += mc146818_read(NULL, i); /* XXX softc */

	return (cksum == (mc146818_read(NULL, 0x32) << 8)
			  + mc146818_read(NULL, 0x33));
#else
	/* Check 'incorrect checksum' bit of IBM PS/2 Diagnostic Status Byte */
	return ((mc146818_read(NULL, NVRAM_DIAG) & (1<<6)) == 0);
#endif
}
#endif /* NMCA > 0 */

/*
 * patchable to control century byte handling:
 * 1: always update
 * -1: never touch
 * 0: try to figure out itself
 */
int rtc_update_century = 0;

/*
 * Expand a two-digit year as read from the clock chip
 * into full width.
 * Being here, deal with the CMOS century byte.
 */
static int centb = NVRAM_CENTURY;
static int
clock_expandyear(int clockyear)
{
	int s, clockcentury, cmoscentury;

	clockcentury = (clockyear < 70) ? 20 : 19;
	clockyear += 100 * clockcentury;

	if (rtc_update_century < 0)
		return (clockyear);

	s = splclock();
#if NACPICA > 0
	if (acpi_active)
		cmoscentury = mc146818_read(NULL,
		    (centb = AcpiGbl_FADT.Century));
	else
#endif
	if (cmoscheck())
		cmoscentury = mc146818_read(NULL, NVRAM_CENTURY);
#if NMCA > 0
	else if (MCA_system && cmoscheckps2())
		cmoscentury = mc146818_read(NULL, (centb = 0x37));
#endif
	else
		cmoscentury = 0;
	splx(s);
	if (!cmoscentury) {
#ifdef DIAGNOSTIC
		printf("clock: unknown CMOS layout\n");
#endif
		return (clockyear);
	}
	cmoscentury = bcdtobin(cmoscentury);

	if (cmoscentury != clockcentury) {
		/* XXX note: saying "century is 20" might confuse the naive. */
		printf("WARNING: NVRAM century is %d but RTC year is %d\n",
		       cmoscentury, clockyear);

		/* Kludge to roll over century. */
		if ((rtc_update_century > 0) ||
		    ((cmoscentury == 19) && (clockcentury == 20) &&
		     (clockyear == 2000))) {
			printf("WARNING: Setting NVRAM century to %d\n",
			       clockcentury);
			s = splclock();
			mc146818_write(NULL, centb, bintobcd(clockcentury));
			splx(s);
		}
	} else if (cmoscentury == 19 && rtc_update_century == 0)
		rtc_update_century = 1; /* will update later in resettodr() */

	return (clockyear);
}

int
rtc_get_ymdhms(todr_chip_handle_t tch, struct clock_ymdhms *dt)
{
	int s;
	mc_todregs rtclk;

	s = splclock();
	if (rtcget(&rtclk)) {
		splx(s);
		return -1;
	}
	splx(s);

	dt->dt_sec = bcdtobin(rtclk[MC_SEC]);
	dt->dt_min = bcdtobin(rtclk[MC_MIN]);
	dt->dt_hour = bcdtobin(rtclk[MC_HOUR]);
	dt->dt_day = bcdtobin(rtclk[MC_DOM]);
	dt->dt_mon = bcdtobin(rtclk[MC_MONTH]);
	dt->dt_year = clock_expandyear(bcdtobin(rtclk[MC_YEAR]));

	return 0;
}

int
rtc_set_ymdhms(todr_chip_handle_t tch, struct clock_ymdhms *dt)
{
	mc_todregs rtclk;
	int century;
	int s;

	s = splclock();
	if (rtcget(&rtclk))
		memset(&rtclk, 0, sizeof(rtclk));
	splx(s);

	rtclk[MC_SEC] = bintobcd(dt->dt_sec);
	rtclk[MC_MIN] = bintobcd(dt->dt_min);
	rtclk[MC_HOUR] = bintobcd(dt->dt_hour);
	rtclk[MC_DOW] = dt->dt_wday + 1;
	rtclk[MC_YEAR] = bintobcd(dt->dt_year % 100);
	rtclk[MC_MONTH] = bintobcd(dt->dt_mon);
	rtclk[MC_DOM] = bintobcd(dt->dt_day);

#ifdef DEBUG_CLOCK
	printf("setclock: %x/%x/%x %x:%x:%x\n", rtclk[MC_YEAR], rtclk[MC_MONTH],
	   rtclk[MC_DOM], rtclk[MC_HOUR], rtclk[MC_MIN], rtclk[MC_SEC]);
#endif
	s = splclock();
	rtcput(&rtclk);
	if (rtc_update_century > 0) {
		century = bintobcd(dt->dt_year / 100);
		mc146818_write(NULL, centb, century); /* XXX softc */
	}
	splx(s);
	return 0;

}

void
rtc_register(void)
{
	static struct todr_chip_handle	tch;
	tch.todr_gettime_ymdhms = rtc_get_ymdhms;
	tch.todr_settime_ymdhms = rtc_set_ymdhms;
	tch.todr_setwen = NULL;

	todr_attach(&tch);
}

