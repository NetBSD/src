/*	$NetBSD: clock_hb.c,v 1.5 2000/10/03 13:49:25 tsutsui Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <uvm/uvm_extern.h>

#include <dev/clock_subr.h>

#include <machine/cpu.h>

#include <news68k/news68k/isr.h>
#include <news68k/news68k/clockvar.h>

#include <news68k/dev/hbvar.h>

/*
 * interrupt level for clock
 */

#define CLOCK_LEVEL 6

#if 1
/* XXX We will switch MI mk48txx driver after eventual bus_space(9) support */
struct clockreg {
	volatile u_char	cl_csr;		/* control register */
#define	CLK_WRITE	0x80		/* want to write */
#define	CLK_READ	0x40		/* want to read (freeze clock) */
	volatile u_char	cl_sec;		/* seconds (0..59; BCD) */
	volatile u_char	cl_min;		/* minutes (0..59; BCD) */
	volatile u_char	cl_hour;	/* hour (0..23; BCD) */
	volatile u_char	cl_wday;	/* weekday (1..7) */
	volatile u_char	cl_mday;	/* day in month (1..31; BCD) */
	volatile u_char	cl_month;	/* month (1..12; BCD) */
	volatile u_char	cl_year;	/* year (0..99; BCD) */
};

#define MK48T02_SIZE	(2*1024)
#define MK48T08_SIZE	(8*1024)

struct mk48txx {
	struct clockreg	*mk_reg;	/* XXX register address */
	u_int	mk_nvramsz;		/* Size of NVRAM on the chip */
	u_int	mk_clkoffset;		/* Offset in NVRAM to clock bits */
	u_int	mk_year0;		/* What year is represented on the system
					   by the chip's year counter at 0 */
};

static	int mk48txx_gettime(todr_chip_handle_t, struct timeval *);
static	int mk48txx_settime(todr_chip_handle_t, struct timeval *);
static	int mk48txx_getcal(todr_chip_handle_t, int *);
static	int mk48txx_setcal(todr_chip_handle_t, int);
#endif

int	clock_hb_match __P((struct device *, struct cfdata  *, void *));
void	clock_hb_attach __P((struct device *, struct device *, void *));
void	clock_hb_initclocks __P((int, int));
void	clock_intr __P((struct clockframe *));

static void leds_intr __P((void));

extern void _isr_clock __P((void));	/* locore.s */

struct cfattach clock_hb_ca = {
	sizeof(struct device), clock_hb_match, clock_hb_attach
};

extern volatile u_char *ctrl_timer, *ctrl_led;
extern struct cfdriver clock_cd;

int
clock_hb_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct hb_attach_args *ha = aux;
	static int clock_hb_matched;

	/* Only one clock, please. */
	if (clock_hb_matched)
		return (0);

	if (strcmp(ha->ha_name, clock_cd.cd_name))
		return (0);

	if (ha->ha_ipl == -1)
		ha->ha_ipl = CLOCK_LEVEL;

	clock_hb_matched = 1;

	return 1;
}

void
clock_hb_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct hb_attach_args *ha = aux;
	caddr_t nvram, clockregs;
	int nvramsize;
	todr_chip_handle_t handle;
#if 1
	struct mk48txx *mk;
	int sz;
#endif
	extern int delay_divisor;	/* from machdep.c */

	if (ha->ha_ipl != CLOCK_LEVEL)
		panic("clock_hb_attach: wrong interrupt level");

	nvram = (caddr_t)(IIOV(ha->ha_address) - 0x7f8); /* XXX */
	nvramsize = MK48T02_SIZE;
	clockregs = (caddr_t)IIOV(ha->ha_address);

#if 1
	sz = ALIGN(sizeof(struct todr_chip_handle)) + sizeof(struct mk48txx);
	handle = malloc(sz, M_DEVBUF, M_NOWAIT);
	mk = (struct mk48txx *)((u_long)handle +
				 ALIGN(sizeof(struct todr_chip_handle)));

	handle->cookie = mk;
	handle->todr_gettime = mk48txx_gettime;
	handle->todr_settime = mk48txx_settime;
	handle->todr_getcal = mk48txx_getcal;
	handle->todr_setcal = mk48txx_setcal;

	mk->mk_reg = (struct clockreg *)clockregs;
	mk->mk_nvramsz = nvramsize;
	mk->mk_clkoffset = 0x7f8;
	mk->mk_year0 = 1900;
#else /* XXX eventually */
	if ((handle = mk48txx_attach(...)) == NULL)
		panic("Can't attach tod clock");
#endif

	handle->bus_cookie = NULL;
	handle->todr_setwen = NULL;

#if 1
	/* Print info about the clock. */
	printf(": Mostek MK48T0%d, %d bytes of NVRAM\n",
	    (nvramsize / 1024), nvramsize);
#endif
	printf("%s: delay_divisor %d\n", self->dv_xname, delay_divisor);

        clock_config(handle, clock_hb_initclocks);

#if 0 /* XXX this will be done in clock_hb_initclocks() */
	isrlink_custom(ha->ha_ipl, (void *)_isr_clock);
#endif
}

/*
 * Set up the real-time clock (enable clock interrupts).
 * Leave stathz 0 since there is no secondary clock available.
 * Note that clock interrupts MUST STAY DISABLED until here.
 */
void
clock_hb_initclocks(tick, statint)
	int tick, statint;
{
	int s;

	s = splhigh();

	/* Install isr (in locore.s) that calls clock_intr(). */
	isrlink_custom(CLOCK_LEVEL, (void *)_isr_clock);

	/* enable the clock */
	*ctrl_timer = 1;
	splx(s);
}

/*
 * Clock interrupt handler for Mostek.
 * This is is called by the "custom" interrupt handler.
 *
 * from sun3/sun3x/clock.c -tsutsui
 */
void
clock_intr(cf)
	struct clockframe *cf;
{
#ifdef	LED_IDLE_CHECK 
	extern char _Idle[];	/* locore.s */
#endif

	/* Pulse the clock intr. enable low. */
	*ctrl_timer = 0;
	*ctrl_timer = 1;
	intrcnt[CLOCK_LEVEL]++;

	{
	/* Entertainment! */
#ifdef	LED_IDLE_CHECK
	if (cf.cf_pc == (long)_Idle)
#endif
		leds_intr();
	}

	/* Call common clock interrupt handler. */
	hardclock(cf);
	uvmexp.intrs++;
}

/* heartbeat LED */
static u_char led_countdown = 0;
static u_char led_stat = 0;

#define LED0	0x01
#define LED1	0x02

static void
leds_intr()
{
	u_char i;

	if (led_countdown) {
		led_countdown--;
		return;
	}

	i = led_stat ^ LED0;
	*ctrl_led = i;
	led_stat = i;
	led_countdown = hz;

	return;
}

#if 1
/* XXX We will switch MI mk48txx driver after eventual bus_space(9) */
static int
mk48txx_gettime(handle, tv)
	todr_chip_handle_t handle;
	struct timeval *tv;
{
	struct mk48txx *mk = handle->cookie;
        struct clockreg *cl = mk->mk_reg;
	struct clock_ymdhms dt;
	int year;

	todr_wenable(handle, 1);

        cl->cl_csr |= CLK_READ;         /* enable read (stop time) */
        dt.dt_sec  = FROMBCD(cl->cl_sec);
        dt.dt_min  = FROMBCD(cl->cl_min);
        dt.dt_hour = FROMBCD(cl->cl_hour);
        dt.dt_day  = FROMBCD(cl->cl_mday);
        dt.dt_mon  = FROMBCD(cl->cl_month);
        year = FROMBCD(cl->cl_year);

	year += mk->mk_year0;
	if (year < POSIX_BASE_YEAR)
		year += 100;

	dt.dt_year = year;

	/* time wears on */
        cl->cl_csr &= ~CLK_READ;        /* time wears on */
	todr_wenable(handle, 0);

	/* simple sanity checks */
	if (dt.dt_mon < 1 || dt.dt_mon > 12 || dt.dt_day < 1 || dt.dt_day > 31)
		return (1);

	tv->tv_sec = clock_ymdhms_to_secs(&dt);
	tv->tv_usec = 0;
	return (0);
}

static int
mk48txx_settime(handle, tv)
	todr_chip_handle_t handle;
	struct timeval *tv;
{
	struct mk48txx *mk = handle->cookie;
        struct clockreg *cl = mk->mk_reg;
	struct clock_ymdhms dt;
	int year;

	/* Note: we ignore `tv_usec' */
	clock_secs_to_ymdhms(tv->tv_sec, &dt);

	year = dt.dt_year - mk->mk_year0;
	if (year > 99)
		year -= 100;

	todr_wenable(handle, 1);
	/* enable write */
	cl->cl_csr |= CLK_WRITE;
	cl->cl_sec   = TOBCD(dt.dt_sec);
	cl->cl_min   = TOBCD(dt.dt_min);
	cl->cl_hour  = TOBCD(dt.dt_hour);
	cl->cl_wday  = TOBCD(dt.dt_wday);
	cl->cl_mday  = TOBCD(dt.dt_day);
	cl->cl_month = TOBCD(dt.dt_mon);
	cl->cl_year  = TOBCD(year);

	/* load them up */
	cl->cl_csr &= ~CLK_WRITE;

	todr_wenable(handle, 0);
	return (0);
}

static int
mk48txx_getcal(handle, vp)
	todr_chip_handle_t handle;
	int *vp;
{
	return (EOPNOTSUPP);
}

static int
mk48txx_setcal(handle, v)
	todr_chip_handle_t handle;
	int v;
{
	return (EOPNOTSUPP);
}
#endif
