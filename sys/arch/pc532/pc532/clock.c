/*	$NetBSD: clock.c,v 1.19 1997/03/20 12:00:33 matthias Exp $	*/

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
 *
 */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/conf.h>

#include <dev/clock_subr.h>

#include <machine/autoconf.h>
#include <machine/icu.h>

#define ROM_ORIGIN	0xFFF00000	/* Mapped origin! */
static volatile u_char * const rom = (u_char *)ROM_ORIGIN;
static int divisor;
static int clockinitted;
static int rtc_attached;
static u_char rtc_magic[8] = {
	0xc5, 0x3a, 0xa3, 0x5c, 0xc5, 0x3a, 0xa3, 0x5c
};

static long	clk_get_secs __P((void));
static void	clk_set_secs __P((long));
static u_char	bcd2bin __P((u_char));
static u_char	bin2bcd __P((u_char));
static void	rw_rtc __P((struct clock_ymdhms *, int));
static void	write_rtc __P((u_char *));

static int  clock_match __P((struct device *, struct cfdata *, void *args));
static void clock_attach __P((struct device *, struct device *, void *));

struct cfattach clock_ca = {
	sizeof(struct device), clock_match, clock_attach
};

struct cfdriver clock_cd = {
	NULL, "clock", DV_DULL
};

static int  rtc_match __P((struct device *, struct cfdata *, void *args));
static void rtc_attach __P((struct device *, struct device *, void *));

struct cfattach rtc_ca = {
	sizeof(struct device), rtc_match, rtc_attach
};

struct cfdriver rtc_cd = {
	NULL, "rtc", DV_DULL
};

static int
clock_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct confargs *ca = aux;

	/* This driver only supports one unit. */
	if (cf->cf_unit != 0)
		return(0);

	if ((ca->ca_addr != -1 && ca->ca_addr != ICU_ADR) ||
	    (ca->ca_irq != -1 && ca->ca_irq != IR_CLK))
		return(0);

	ca->ca_addr = ICU_ADR;
	ca->ca_irq = IR_CLK;

	return(1);
}

static void
clock_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct confargs *ca = aux;

	printf("\n");
	divisor = ICU_CLK_HZ / hz;

	/* Disable clock interrupt for now. */
	ICUB(CICTL) = 0;

	/* Select clock interrupt vector. */
	ICUB(CIPTR) = ca->ca_irq << 4;

	/* Establish interrupt vector */
	intr_establish(IR_CLK, (void (*)(void *))hardclock, NULL, "clock",
		       IPL_CLOCK, IPL_CLOCK, FALLING_EDGE);

	/* No clock output. */
	ICUB(OCASN) = 0;

	/*
	 * Two clocks, prescale, output zero detect of L-counter,
	 * run both clocks.
	 */
	ICUB(CCTL) = 0x1c;

	/* Write the timer values to the ICU. */
	ICUW(HCSV) = divisor;
	ICUW(HCCV) = divisor;
}

void
cpu_initclocks()
{
	/* Enable the clock interrupt. */
	ICUB(CICTL) = 0x30;
}

void
setstatclockrate(arg)
	int arg;
{
	printf("setstatclockrate\n");
}

void
microtime(tvp)
	register struct timeval *tvp;
{
	u_short count, delta, ipend;

	di();
	ICUB(MCTL) |= 0x88;		/* freeze HCCV, IPND */
	count = ICUW(HCCV);
	ipend = ICUW(IPND);
	*tvp = time;
	ICUB(MCTL) &= 0x77;		/* thaw ICU regs */
	ei();

	/* yields value 0..9999 */
	delta = ((divisor - count) * (4096 * 1000000UL / ICU_CLK_HZ)) >> 12;
	tvp->tv_usec += delta;

	/* check for timer overflow (ie; clock int disabled) */
	if (count > 0 && (ipend & (1<<IR_CLK))) {
		tvp->tv_usec += tick;
		if (tvp->tv_usec > 1000000) {
			tvp->tv_usec -= 1000000;
			tvp->tv_sec++;
		}
	}
}

/*
 * Machine-dependent clock routines.
 *
 * Inittodr initializes the time of day hardware which provides
 * date functions.
 *
 * Resettodr restores the time of day hardware after a time change.
 */

static int
rtc_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct confargs *ca = aux;
	int rom_val, rom_cnt, i;

	/* This driver only supports one unit. */
	if (cf->cf_unit != 0)
		return(0);

	(void) rom[4];	/* Synchronize the comparison reg. */
	rom_val = rom[4];
	write_rtc(rtc_magic);

	for (i = rom_cnt = 0; i < 64; i++)
		if (rom[4] == rom_val)
			rom_cnt++;

	if (rom_cnt == 64)
		return(0);

	if ((ca->ca_addr != -1 && ca->ca_addr != ROM_ORIGIN) ||
	    ca->ca_irq != -1)
		return(0);

	ca->ca_addr = ROM_ORIGIN;
	ca->ca_irq = -1;

	return(1);
}

static void
rtc_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	printf("\n");
	rtc_attached = 1;
}

/*
 * Initialize the time of day register, based on the time base
 * which is, e.g. from a filesystem.
 */
void inittodr(fs_time)
	time_t fs_time;
{
	long diff, clk_time;
	long long_ago = (5 * SECYR);
	int clk_bad = 0;

	clockinitted = 1;

	/*
	 * Sanity check time from file system.
	 * If it is zero,assume filesystem time is just unknown
	 * instead of preposterous.  Don't bark.
	 */
	if (fs_time < long_ago) {
		/*
		 * If fs_time is zero, assume filesystem time is just
		 * unknown instead of preposterous.  Don't bark.
		 */
		if (fs_time != 0)
			printf("WARNING: preposterous time in file system\n");
		/* 1991/07/01  12:00:00 */
		fs_time = 21*SECYR + 186*SECDAY + SECDAY/2;
	}

	clk_time = clk_get_secs();

	/* Sanity check time from clock. */
	if (clk_time < long_ago) {
		printf("WARNING: bad date in battery clock");
		clk_bad = 1;
		clk_time = fs_time;
	} else {
		/* Does the clock time jive with the file system? */
		diff = clk_time - fs_time;
		if (diff < 0)
			diff = -diff;
		if (diff >= (SECDAY*2)) {
			printf("WARNING: clock %s %d days",
				   (clk_time < fs_time) ? "lost" : "gained",
				   (int) (diff / SECDAY));
			clk_bad = 1;
		}
	}
	if (clk_bad)
		printf(" -- CHECK AND RESET THE DATE!\n");
	time.tv_sec = clk_time;
}

/*
 * Resettodr restores the time of day hardware after a time change.
 */
void resettodr()
{
	/*
	 * We might have been called by boot() due to a crash early
	 * on.  Don't reset the clock chip in this case.
	 */
	if (clockinitted == 0)
		return;
	clk_set_secs(time.tv_sec);
}

/*
 * Now routines to get and set clock as POSIX time.
 * Our clock keeps "years since 1/1/1900".
 */
#define	CLOCK_BASE_YEAR 1900

/*
 * Do the actual reading and writing of the rtc.  We have to read
 * and write the entire contents at a time.
 *   rw = 0 => read,
 *   rw = 1 => write.
 */

static u_char
bcd2bin(n)
	u_char n;
{
	return(((n >> 4) & 0x0f) * 10 + (n & 0x0f));
}

static u_char
bin2bcd(n)
	u_char n;
{
	return((char)(((n / 10) << 4) & 0xf0) | ((n % 10) & 0x0f));
}

static void
write_rtc(p)
	u_char *p;
{
	u_char *q;
	int i;

	for (q = p + 8; p < q; p++) {
		for (i = 0; i < 8; i++)
			(void) rom[(*p >> i) & 0x01];
	}
}

static void
rw_rtc(dt, rw)
	struct clock_ymdhms *dt;
	int rw;
{
	struct {
		u_char rtc_csec;
		u_char rtc_sec;
		u_char rtc_min;
		u_char rtc_hour;
		u_char rtc_wday;
		u_char rtc_day;
		u_char rtc_mon;
		u_char rtc_year;
	} rtcdt[1];
	u_char *p;
	int i;

	/*
	 * Read or write to the real time chip. Address line A0 functions as
	 * data input, A2 is used as the /write signal. Accesses to the RTC
	 * are always done to one of the addresses (unmapped):
	 *
	 * 0x10000000  -  write a '0' bit
	 * 0x10000001  -  write a '1' bit
	 * 0x10000004  -  read a bit
	 *
	 * Data is output from the RTC using D0. To read or write time
	 * information, the chip has to be activated first, to distinguish
	 * clock accesses from normal ROM reads. This is done by writing,
	 * bit by bit, a magic pattern to the chip. Before that, a dummy read
	 * assures that the chip's pattern comparison register pointer is
	 * reset. The RTC register file is always read or written wholly,
	 * even if we are only interested in a part of it.
	 */

	/* Activate the real time chip */
	(void) rom[4];	/* Synchronize the comparison reg. */
	write_rtc(rtc_magic);

	if (rw == 0) {
		/* Read the time from the RTC. */
		for (p = (u_char *)rtcdt; p < (u_char *)(rtcdt + 1); p++) {
			for (i = 0; i < 8; i++) {
				*p >>= 1;
				*p |= ((rom[4] & 0x01) ? 0x80 : 0x00);
			}
		}
		dt->dt_sec  = bcd2bin(rtcdt->rtc_sec);
		dt->dt_min  = bcd2bin(rtcdt->rtc_min);
		dt->dt_hour = bcd2bin(rtcdt->rtc_hour);
		dt->dt_day  = bcd2bin(rtcdt->rtc_day);
		dt->dt_mon  = bcd2bin(rtcdt->rtc_mon);
		dt->dt_year = bcd2bin(rtcdt->rtc_year);
		dt->dt_wday = bcd2bin(rtcdt->rtc_wday);
	} else {
		/* Write the time to the RTC */
		rtcdt->rtc_sec  = bin2bcd(dt->dt_sec);
		rtcdt->rtc_min  = bin2bcd(dt->dt_min);
		rtcdt->rtc_hour = bin2bcd(dt->dt_hour);
		rtcdt->rtc_day  = bin2bcd(dt->dt_day);
		rtcdt->rtc_mon  = bin2bcd(dt->dt_mon);
		rtcdt->rtc_year = bin2bcd(dt->dt_year);
		rtcdt->rtc_wday = bin2bcd(dt->dt_wday);
		write_rtc((u_char *)rtcdt);
	}
}

static long
clk_get_secs()
{
	struct clock_ymdhms dt;
	long secs;

	if (rtc_attached == 0)
		return(0);

	rw_rtc(&dt, 0);
	if ((dt.dt_sec  > 59) ||
	    (dt.dt_min  > 59) ||
	    (dt.dt_hour > 23) ||
	    (dt.dt_day  > 31) ||
	    (dt.dt_mon  > 12) ||
	    (dt.dt_year > 99))
		return(0);

	if (dt.dt_year < 70)
		dt.dt_year += 100;

	dt.dt_year += CLOCK_BASE_YEAR;
	secs = clock_ymdhms_to_secs(&dt);
	return(secs);
}

static void
clk_set_secs(secs)
	long secs;
{
	struct clock_ymdhms dt;

	if (rtc_attached == 0)
		return;

	clock_secs_to_ymdhms(secs, &dt);
	dt.dt_year -= CLOCK_BASE_YEAR;
	if (dt.dt_year >= 100)
		dt.dt_year -= 100;

	rw_rtc(&dt, 1);
}
