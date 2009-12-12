/*      $NetBSD: rtc.c,v 1.15 2009/12/12 14:44:09 tsutsui Exp $        */
/*
 * Copyright (c) 1998 Darrin Jewell
 * Copyright (c) 1997 Rolf Grossmann 
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
 *      This product includes software developed by Rolf Grossmann.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* These haven't been tested to see how they interact with NeXTstep's
 * interpretation of the rtc.
 */

/* Now using this in the kernel.  This should be turned into a device
 * Darrin B Jewell <jewell@mit.edu>  Tue Jan 27 20:59:25 1998
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rtc.c,v 1.15 2009/12/12 14:44:09 tsutsui Exp $");

#include <sys/param.h>
#include <sys/systm.h>          /* for panic */

#include <machine/bus.h>
#include <machine/cpu.h>

#include <dev/clock_subr.h>

#include <next68k/next68k/rtc.h>

#include <next68k/dev/clockreg.h>
#include <next68k/dev/intiovar.h>

/* #define RTC_DEBUG */

u_char new_clock;
volatile u_int *scr2 = (u_int *)NEXT_P_SCR2; /* will get memory mapped in rtc_init */

static int gettime_old(todr_chip_handle_t, struct clock_ymdhms *);
static int settime_old(todr_chip_handle_t, struct clock_ymdhms *);
static int gettime_new(todr_chip_handle_t, struct timeval *);
static int settime_new(todr_chip_handle_t, struct timeval *);

/*
 * NB: This code should probably be converted to a _true_ device, then this
 * initialization could happen in attach.  The printf could get fixed then,
 * too.
 */
void
rtc_init(void)
{
	static struct todr_chip_handle tch;
	u_char val;
	
	scr2 = (u_int *)IIOV(NEXT_P_SCR2);
	val = rtc_read(RTC_STATUS);
	new_clock = (val & RTC_NEW_CLOCK) ? 1 : 0;

	printf("Looks like a %s clock chip.\n",
			(new_clock?
					"MCS1850 (new style)":
					"MC68HC68T1 (old style)"));

#ifdef RTC_DEBUG
	rtc_print();
#endif

	if (new_clock) {
		tch.todr_gettime = gettime_new;
		tch.todr_settime = settime_new;
		tch.todr_gettime_ymdhms = NULL;
		tch.todr_settime_ymdhms = NULL;
	} else {
		tch.todr_gettime_ymdhms = gettime_old;
		tch.todr_settime_ymdhms = settime_old;
		tch.todr_gettime = NULL;
		tch.todr_settime = NULL;
	}
	tch.todr_setwen = NULL;

	todr_attach(&tch);
}

void
rtc_print(void)
{

#define RTC_PRINT(x)	printf("\t%16s= 0x%02x\n",#x, rtc_read(x))

	if (new_clock) {
		RTC_PRINT(RTC_RAM);
		RTC_PRINT(RTC_CNTR0);
		RTC_PRINT(RTC_CNTR1);
		RTC_PRINT(RTC_CNTR2);
		RTC_PRINT(RTC_CNTR3);
		RTC_PRINT(RTC_ALARM0);
		RTC_PRINT(RTC_ALARM1);
		RTC_PRINT(RTC_ALARM2);
		RTC_PRINT(RTC_ALARM3);
		RTC_PRINT(RTC_STATUS);
		RTC_PRINT(RTC_CONTROL);
	} else {
		RTC_PRINT(RTC_RAM);
		RTC_PRINT(RTC_SEC);
		RTC_PRINT(RTC_MIN);
		RTC_PRINT(RTC_HRS);
		RTC_PRINT(RTC_DAY);
		RTC_PRINT(RTC_DATE);
		RTC_PRINT(RTC_MON);
		RTC_PRINT(RTC_YR);
		RTC_PRINT(RTC_ALARM_SEC);
		RTC_PRINT(RTC_ALARM_MIN);
		RTC_PRINT(RTC_ALARM_HR);
		RTC_PRINT(RTC_STATUS);
		RTC_PRINT(RTC_CONTROL);
		RTC_PRINT(RTC_INTRCTL);
	}
}


u_char
rtc_read(u_char reg)
{
	int i;
	u_int tmp;
	u_char val;

	*scr2 = (*scr2 & ~(SCR2_RTDATA | SCR2_RTCLK)) | SCR2_RTCE;
	DELAY(1);

	val = reg;
	for (i=0; i<8; i++) {
		tmp = *scr2 & ~(SCR2_RTDATA | SCR2_RTCLK);
		if (val & 0x80)
			tmp |= SCR2_RTDATA;

		*scr2 = tmp;
		DELAY(1);
		*scr2 = tmp | SCR2_RTCLK;
		DELAY(1);
		*scr2 = tmp;
		DELAY(1);

		val <<= 1;
	}

	val = 0;			/* should be anyway */
	for (i=0; i<8; i++) {
		val <<= 1;
	
		tmp = *scr2 & ~(SCR2_RTDATA | SCR2_RTCLK);

		*scr2 = tmp | SCR2_RTCLK;
		DELAY(1);
		*scr2 = tmp;
		DELAY(1);

		if (*scr2 & SCR2_RTDATA)
			val |= 1;
	}

	*scr2 &= ~(SCR2_RTDATA|SCR2_RTCLK|SCR2_RTCE);
	DELAY(1);

	return val;
}

void
rtc_write(u_char reg, u_char v)
{
	int i;
	u_int tmp;
	u_char val;

	*scr2 = (*scr2 & ~(SCR2_RTDATA | SCR2_RTCLK)) | SCR2_RTCE;
	DELAY(1);

	val = reg|RTC_WRITE;

	for (i=0; i<8; i++) {
		tmp = *scr2 & ~(SCR2_RTDATA | SCR2_RTCLK);
		if (val & 0x80)
			tmp |= SCR2_RTDATA;

		*scr2 = tmp;
		DELAY(1);
		*scr2 = tmp | SCR2_RTCLK;
		DELAY(1);
		*scr2 = tmp;
		DELAY(1);

		val <<= 1;
	}

	DELAY(1);

	for (i=0; i<8; i++) {
		tmp = *scr2 & ~(SCR2_RTDATA | SCR2_RTCLK);
		if (v & 0x80)
			tmp |= SCR2_RTDATA;

		*scr2 = tmp;
		DELAY(1);
		*scr2 = tmp | SCR2_RTCLK;
		DELAY(1);
		*scr2 = tmp;
		DELAY(1);

		v <<= 1;
	}

	*scr2 &= ~(SCR2_RTDATA|SCR2_RTCLK|SCR2_RTCE);
	DELAY(1);
}

void
poweroff(void)
{
	int reg, t;

	if(new_clock) {
		reg = RTC_CNTR3;
	} else {
		reg = RTC_CNTR0;
	}

	t = rtc_read(reg);	/* seconds */
	/* wait for clock to tick */
	while(t == rtc_read(reg));

	DELAY(850000);	/* hardware bug workaround ? */

	if(new_clock) {
		reg = RTC_CONTROL;
	} else {
		reg = RTC_INTRCTL;
	}

	rtc_write(reg, rtc_read(reg)|(RTC_PDOWN));
	
	printf("....................."); /* @@@ work around some sort of bug. */

	panic("Failed to poweroff!");
}


int
gettime_old(todr_chip_handle_t tch, struct clock_ymdhms *dt)
{
	u_char h, y;
	
	struct clock_ymdhms val;
	y = FROMBCD(rtc_read(RTC_YR));
	if (y >= 69) {
		dt->dt_year = 1900+y;
	} else {
		dt->dt_year = 2000+y;
	}

	dt->dt_mon	= FROMBCD(rtc_read(RTC_MON)&0x1f);
	dt->dt_day	= FROMBCD(rtc_read(RTC_DATE)&0x3f);
	dt->dt_wday = FROMBCD(rtc_read(RTC_DAY)&0x7);

	h = rtc_read(RTC_HRS);
	if (h & 0x80) {			/* time is am/pm format */
		dt->dt_hour = FROMBCD(h&0x1f);
		if (h & 0x20) { /* pm */
			if (dt->dt_hour < 12) dt->dt_hour += 12;
		} else {  /* am */
			if (dt->dt_hour == 12) dt->dt_hour = 0;
		}
	} else {	/* time is 24 hour format */
		val.dt_hour = FROMBCD(h & 0x3f);
	}

	dt->dt_min	= FROMBCD(rtc_read(RTC_MIN)&0x7f);
	dt->dt_sec	= FROMBCD(rtc_read(RTC_SEC)&0x7f);

	return 0;
}

int
settime_old(todr_chip_handle_t tcr, struct clock_ymdhms *dt)
{
	u_char h;

	/* Stop the clock */
	rtc_write(RTC_CONTROL,rtc_read(RTC_CONTROL) & ~RTC_START);

#ifdef RTC_DEBUG
	printf("Setting RTC to 0x%08x.  Regs before:\n",secs);
	rtc_print();
#endif

	rtc_write(RTC_SEC,TOBCD(dt->dt_sec));
	rtc_write(RTC_MIN,TOBCD(dt->dt_min));
	h = rtc_read(RTC_HRS);
	if (h & 0x80) {		/* time is am/pm format */
		if (dt->dt_hour == 0) {
			rtc_write(RTC_HRS,TOBCD(12)|0x80);
		} else if (dt->dt_hour < 12) {	/* am */
			rtc_write(RTC_HRS,TOBCD(dt->dt_hour)|0x80);
		} else if (dt->dt_hour == 12) {
				rtc_write(RTC_HRS,TOBCD(12)|0x80|0x20);
		} else 		/* pm */
			rtc_write(RTC_HRS,TOBCD(dt->dt_hour-12)|0x80|0x20);
	} else {	/* time is 24 hour format */
			rtc_write(RTC_HRS,TOBCD(dt->dt_hour));
	}
	rtc_write(RTC_DAY,TOBCD(dt->dt_wday));
	rtc_write(RTC_DATE,TOBCD(dt->dt_day));
	rtc_write(RTC_MON,TOBCD(dt->dt_mon));
	rtc_write(RTC_YR,TOBCD(dt->dt_year%100));

#ifdef RTC_DEBUG
	printf("Regs after:\n",secs);
	rtc_print();
#endif

	/* restart the clock */
	rtc_write(RTC_CONTROL,rtc_read(RTC_CONTROL) | RTC_START);
	return 0;
}

int
gettime_new(todr_chip_handle_t tch, struct timeval *tvp)
{
	tvp->tv_sec = rtc_read(RTC_CNTR0) << 24 |
			rtc_read(RTC_CNTR1) << 16 |
			rtc_read(RTC_CNTR2) << 8	 |
			rtc_read(RTC_CNTR3);
	return 0;
}

int
settime_new(todr_chip_handle_t tch, struct timeval *tvp)
{

	/* Stop the clock */
	rtc_write(RTC_CONTROL,rtc_read(RTC_CONTROL) & ~RTC_START);

#ifdef RTC_DEBUG
	printf("Setting RTC to 0x%08x.  Regs before:\n",tvp->tv_sec);
	rtc_print();
#endif

	rtc_write(RTC_CNTR0, (tvp->tv_sec >> 24) & 0xff);
	rtc_write(RTC_CNTR1, (tvp->tv_sec >> 16) & 0xff);
	rtc_write(RTC_CNTR2, (tvp->tv_sec >> 8) & 0xff);
	rtc_write(RTC_CNTR3, (tvp->tv_sec) & 0xff);

#ifdef RTC_DEBUG
	printf("Regs after:\n",secs);
	rtc_print();
#endif

	/* restart the clock */
	rtc_write(RTC_CONTROL,rtc_read(RTC_CONTROL) | RTC_START);

	return 0;
}
