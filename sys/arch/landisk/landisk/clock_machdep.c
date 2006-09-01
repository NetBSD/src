/*	$NetBSD: clock_machdep.c,v 1.1 2006/09/01 21:26:18 uwe Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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
__KERNEL_RCSID(0, "$NetBSD: clock_machdep.c,v 1.1 2006/09/01 21:26:18 uwe Exp $");

#include <sys/param.h>
#include <sys/systm.h>

#include <dev/clock_subr.h>

#include <sh3/devreg.h>
#include <sh3/clock.h>
#include <sh3/scireg.h>

#include <landisk/landisk/landiskreg.h>
#include <landisk/dev/rs5c313reg.h>

void rs5c313_ops_init(void *cookie);
void rs5c313_ops_get(void *cookie, time_t base, struct clock_ymdhms *dt);
void rs5c313_ops_set(void *cookie, struct clock_ymdhms *dt);

static struct rtc_ops rs5c313_ops = {
	.init = rs5c313_ops_init,
	.get = rs5c313_ops_get,
	.set = rs5c313_ops_set,
};

void
machine_clock_init(void)
{

	sh_clock_init(SH_CLOCK_NORTC, &rs5c313_ops);
}

/*
 * RTC chip control
 */
static inline void rtc_ce(int onoff);
static inline void rtc_clk(int onoff);
static inline void rtc_dir(int output);
static void rtc_init(void);
static void rtc_do(int onoff);
static int rtc_di(void);

/* nanosec delay */
#define	ndelay(n)	delay(1)

/* control RTC chip enable */
static inline void
rtc_ce(int onoff)
{

	if (onoff) {
		_reg_write_1(LANDISK_PWRMNG, PWRMNG_RTC_CE);
	} else {
		_reg_write_1(LANDISK_PWRMNG, 0);
	}
	ndelay(600);
}

static inline void
rtc_clk(int onoff)
{

	if (onoff) {
		SHREG_SCSPTR |= SCSPTR_SPB0DT;
	} else {
		SHREG_SCSPTR &= ~SCSPTR_SPB0DT;
	}
}

static inline void
rtc_dir(int output)
{

	if (output) {
		SHREG_SCSPTR |= SCSPTR_SPB1IO;
	} else {
		SHREG_SCSPTR &= ~SCSPTR_SPB1IO;
	}
}

/* data-out */
static void
rtc_do(int onoff)
{

	if (onoff) {
		SHREG_SCSPTR |= SCSPTR_SPB1DT;
	} else {
		SHREG_SCSPTR &= ~SCSPTR_SPB1DT;
	}
	ndelay(300);

	rtc_clk(0);
	ndelay(300);
	rtc_clk(1);
}

/* data-in */
static int
rtc_di(void)
{
	int d;

	ndelay(300);
	d = (SHREG_SCSPTR & SCSPTR_SPB1DT) ? 1 : 0;

	rtc_clk(0);
	ndelay(300);
	rtc_clk(1);

	return d;
}

static void
rtc_init(void)
{

	SHREG_SCSPTR = SCSPTR_SPB1IO | SCSPTR_SPB1DT
	               | SCSPTR_SPB0IO | SCSPTR_SPB0DT;
	ndelay(100);
}

static uint8_t
rs5c313_read(uint8_t addr)
{
	int data;

	rtc_dir(1);		/* output */
	rtc_do(1);		/* Don't care */
	rtc_do(1);		/* R/#W = 1(READ) */
	rtc_do(1);		/* AD = 1 */
	rtc_do(0);		/* DT = 0 */
	rtc_do(addr & 0x8);	/* A3 */
	rtc_do(addr & 0x4);	/* A2 */
	rtc_do(addr & 0x2);	/* A1 */
	rtc_do(addr & 0x1);	/* A0 */

	rtc_dir(0);		/* input */
	(void)rtc_di();
	(void)rtc_di();
	(void)rtc_di();
	(void)rtc_di();
	data = rtc_di();	/* D3 */
	data <<= 1;
	data |= rtc_di();	/* D2 */
	data <<= 1;
	data |= rtc_di();	/* D1 */
	data <<= 1;
	data |= rtc_di();	/* D0 */

	return data & 0xf;
}

static void
rs5c313_write(uint8_t addr, uint8_t data)
{

	rtc_dir(1);		/* output */
	rtc_do(1);		/* Don't care */
	rtc_do(0);		/* R/#W = 0(WRITE) */
	rtc_do(1);		/* AD = 1 */
	rtc_do(0);		/* DT = 0 */
	rtc_do(addr & 0x8);	/* A3 */
	rtc_do(addr & 0x4);	/* A2 */
	rtc_do(addr & 0x2);	/* A1 */
	rtc_do(addr & 0x1);	/* A0 */

	rtc_do(1);		/* Don't care */
	rtc_do(0);		/* R/#W = 0(WRITE) */
	rtc_do(0);		/* AD = 0 */
	rtc_do(1);		/* DT = 1 */
	rtc_do(data & 0x8);	/* D3 */
	rtc_do(data & 0x4);	/* D2 */
	rtc_do(data & 0x2);	/* D1 */
	rtc_do(data & 0x1);	/* D0 */
}

void
rs5c313_ops_init(void *cookie)
{

	rtc_ce(0);

	rtc_init();
	rtc_ce(1);
	if (rs5c313_read(RS5C313_ADDR_CTRL) & CTRL_XSTP) {
		rs5c313_write(RS5C313_ADDR_TINT, 0);
		rs5c313_write(RS5C313_ADDR_CTRL, (CTRL_BASE | CTRL_ADJ));
		while (rs5c313_read(RS5C313_ADDR_CTRL) & CTRL_BSY) {
			delay(1);
		}
		rs5c313_write(RS5C313_ADDR_CTRL, CTRL_BASE);
	}
	rtc_ce(0);
}

void
rs5c313_ops_get(void *cookie, time_t base, struct clock_ymdhms *dt)
{
	int s;

	s = splhigh();
	rtc_init();
	for (;;) {
		rtc_ce(1);
		rs5c313_write(RS5C313_ADDR_CTRL, CTRL_BASE);
		if ((rs5c313_read(RS5C313_ADDR_CTRL) & CTRL_BSY) == 0) {
			break;
		}
		rtc_ce(0);
		delay(1);
	}

#define RTCGET(x, y) \
do { \
	int __t, __t1, __t10; \
	__t1 = rs5c313_read(RS5C313_ADDR_ ## y ## 1); \
	__t10 = rs5c313_read(RS5C313_ADDR_ ## y ## 10); \
	__t = __t1 | (__t10 << 4); \
	dt->dt_ ## x = FROMBCD(__t); \
} while (0)
	RTCGET(sec, SEC);
	RTCGET(min, MIN);
	RTCGET(hour, HOUR);
	RTCGET(day, DAY);
	RTCGET(mon, MON);
	RTCGET(year, YEAR);
#undef	RTCGET
	dt->dt_wday = rs5c313_read(RS5C313_ADDR_WDAY);

	rtc_ce(0);
	splx(s);

	dt->dt_year = (dt->dt_year % 100) + 1900;
	if (dt->dt_year < 1970) {
		dt->dt_year += 100;
	}
}

void
rs5c313_ops_set(void *cookie, struct clock_ymdhms *dt)
{
	int s;
	uint8_t t;

	s = splhigh();
	rtc_init();
	for (;;) {
		rtc_ce(1);
		rs5c313_write(RS5C313_ADDR_CTRL, CTRL_BASE);
		if ((rs5c313_read(RS5C313_ADDR_CTRL) & CTRL_BSY) == 0) {
			break;
		}
		rtc_ce(0);
		delay(1);
	}

#define	RTCSET(x, y) \
do { \
	t = TOBCD(dt->dt_ ## y) & 0xff; \
	rs5c313_write(RS5C313_ADDR_ ## x ## 1, t & 0x0f); \
	rs5c313_write(RS5C313_ADDR_ ## x ## 10, (t >> 4) & 0x0f); \
} while (0)
	RTCSET(SEC, sec);
	RTCSET(MIN, min);
	RTCSET(HOUR, hour);
	RTCSET(DAY, day);
	RTCSET(MON, mon);
#undef	RTCSET
	t = dt->dt_year % 100;
	t = TOBCD(t);
	rs5c313_write(RS5C313_ADDR_YEAR1, t & 0x0f);
	rs5c313_write(RS5C313_ADDR_YEAR10, (t >> 4) & 0x0f);
	rs5c313_write(RS5C313_ADDR_WDAY, dt->dt_wday);

	rtc_ce(0);
	splx(s);
}
