/*	$NetBSD: clock.c,v 1.31 2006/09/07 15:56:01 simonb Exp $	*/

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
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: clock.c,v 1.31 2006/09/07 15:56:01 simonb Exp $");

#include <sys/param.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/timetc.h>

#include <dev/clock_subr.h>

#include <machine/autoconf.h>
#include <machine/icu.h>

#define ROM_ORIGIN	0xFFF00000	/* Mapped origin! */
static volatile u_char * const rom = (u_char *)ROM_ORIGIN;
static int divisor;
static int clock_attached;
static int rtc_attached;
static unsigned cpu_counter = 0;
static u_char rtc_magic[8] = {
	0xc5, 0x3a, 0xa3, 0x5c, 0xc5, 0x3a, 0xa3, 0x5c
};

static u_char	bcd2bin(u_char);
static u_char	bin2bcd(u_char);
static void	rw_rtc(struct clock_ymdhms *, int);
static void	write_rtc(u_char *);

static int  clock_match(struct device *, struct cfdata *, void *args);
static void clock_attach(struct device *, struct device *, void *);
static u_int icu_counter(struct timecounter *);

static int rtc_gettime(todr_chip_handle_t, volatile struct timeval *);
static int rtc_settime(todr_chip_handle_t, volatile struct timeval *);

CFATTACH_DECL(clock, sizeof(struct device),
    clock_match, clock_attach, NULL, NULL);

static int  rtc_match(struct device *, struct cfdata *, void *args);
static void rtc_attach(struct device *, struct device *, void *);

CFATTACH_DECL(rtc, sizeof(struct device),
    rtc_match, rtc_attach, NULL, NULL);

static int
clock_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct confargs *ca = aux;

	/* This driver only supports one unit. */
	if (clock_attached)
		return(0);

	if ((ca->ca_addr != -1 && ca->ca_addr != ICU_ADR) ||
	    (ca->ca_irq != -1 && ca->ca_irq != IR_CLK))
		return(0);

	ca->ca_addr = ICU_ADR;
	ca->ca_irq = IR_CLK;

	return(1);
}

static void
clock_intr(struct clockframe *arg)
{
	cpu_counter += divisor;
	hardclock(arg);
}

static void
clock_attach(struct device *parent, struct device *self, void *aux)
{
	static u_char icu_table[] = {
		CICTL,	0,		/* Disable clock interrupt for now. */
		CIPTR,	IR_CLK << 4,	/* Select clock interrupt vector. */
		OCASN,	0,		/* No clock output. */
		CCTL,	0x1c,		/* Two clocks, prescale, output zero */
					/* detect of L-counter, run both */
					/* clocks. */
		0xff
	};

	clock_attached = 1;

	printf("\n");
	icu_init(icu_table);

	/* Establish interrupt vector */
	intr_establish(IR_CLK, (void (*)(void *))clock_intr, NULL,
		       self->dv_xname, IPL_CLOCK, IPL_CLOCK, FALLING_EDGE);

	/* Write the timer values to the ICU. */
	divisor = ICU_CLK_HZ / hz;
	ICUW(HCSV) = divisor;
	ICUW(HCCV) = divisor;
}

void
cpu_initclocks(void)
{
	static struct timecounter tc = {
		icu_counter,		/* get_timecount */
		NULL,			/* no poll_pps */
		~0,			/* counter mask */
		ICU_CLK_HZ,		/* frequency */
		"ns32202",		/* name */
		100,			/* quality */
	};
	
	/* Enable the clock interrupt. */
	ICUB(CICTL) = 0x30;

	tc_init(&tc);
}

void
setstatclockrate(int arg)
{

	/* Nothing */
}

u_int
icu_counter(struct timecounter *tc)
{
	uint16_t count, ipend;
	uint32_t ccnt;

	di();
	ICUB(MCTL) |= 0x88;		/* freeze HCCV, IPND */
	count = ICUW(HCCV);
	ipend = ICUW(IPND);
	ccnt = cpu_counter;
	ICUB(MCTL) &= 0x77;		/* thaw ICU regs */
	ei();

	count = (divisor - count);
	/* check for timer overflow (ie; clock int disabled) */
	if ((count > 0) && (ipend & (1 << IR_CLK))) {
		count += divisor;
	}
	return (ccnt + count);
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
rtc_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct confargs *ca = aux;
	int rom_val, rom_cnt, i;

	/* This driver only supports one unit. */
	if (rtc_attached)
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
rtc_attach(struct device *parent, struct device *self, void *aux)
{
	static struct todr_chip_handle hdl;

	printf("\n");
	hdl.todr_gettime = rtc_gettime;
	hdl.todr_settime = rtc_settime;
	hdl.todr_setwen = NULL;
	todr_attach(&hdl);
	rtc_attached = 1;
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
bcd2bin(u_char n)
{

	return(((n >> 4) & 0x0f) * 10 + (n & 0x0f));
}

static u_char
bin2bcd(u_char n)
{

	return((char)(((n / 10) << 4) & 0xf0) | ((n % 10) & 0x0f));
}

static void
write_rtc(u_char *p)
{
	u_char *q;
	int i;

	for (q = p + 8; p < q; p++) {
		for (i = 0; i < 8; i++)
			(void) rom[(*p >> i) & 0x01];
	}
}

static void
rw_rtc(struct clock_ymdhms *dt, int rw)
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

static int
rtc_gettime(todr_chip_handle_t tch, volatile struct timeval *tvp)
{
	struct clock_ymdhms dt;

	if (rtc_attached == 0)
		return -1;

	rw_rtc(&dt, 0);
	if ((dt.dt_sec  > 59) ||
	    (dt.dt_min  > 59) ||
	    (dt.dt_hour > 23) ||
	    (dt.dt_day  > 31) ||
	    (dt.dt_mon  > 12) ||
	    (dt.dt_year > 99))
		return -1;

	if (dt.dt_year < 70)
		dt.dt_year += 100;

	dt.dt_year += CLOCK_BASE_YEAR;
	tvp->tv_sec = clock_ymdhms_to_secs(&dt);
	return 0;
}

static int
rtc_settime(todr_chip_handle_t tch, volatile struct timeval *tvp)
{
	struct clock_ymdhms dt;

	if (rtc_attached == 0)
		return -1;

	clock_secs_to_ymdhms(tvp->tv_sec, &dt);
	dt.dt_year -= CLOCK_BASE_YEAR;
	if (dt.dt_year >= 100)
		dt.dt_year -= 100;

	rw_rtc(&dt, 1);
	return 0;
}
