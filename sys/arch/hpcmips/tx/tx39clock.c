/*	$NetBSD: tx39clock.c,v 1.10.4.2 2002/02/28 04:10:01 nathanw Exp $ */

/*-
 * Copyright (c) 1999-2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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

#include "opt_tx39clock_debug.h"

#include <sys/param.h>
#include <sys/systm.h>

#include <dev/clock_subr.h>

#include <machine/bus.h>
#include <machine/sysconf.h>

#include <hpcmips/tx/tx39var.h>
#include <hpcmips/tx/tx39icureg.h>
#include <hpcmips/tx/tx39clockvar.h>
#include <hpcmips/tx/tx39clockreg.h>
#include <hpcmips/tx/tx39timerreg.h>

#ifdef	TX39CLOCK_DEBUG
#define DPRINTF_ENABLE
#define DPRINTF_DEBUG	tx39clock_debug
#endif
#include <machine/debug.h>

#define ISSETPRINT(r, m)						\
	dbg_bitmask_print(r, TX39_CLOCK_EN ## m ## CLK, #m)

void	tx39clock_init(struct device *);
void	tx39clock_get(struct device *, time_t, struct clock_ymdhms *);
void	tx39clock_set(struct device *, struct clock_ymdhms *);

struct platform_clock tx39_clock = {
#define CLOCK_RATE	100
	CLOCK_RATE, tx39clock_init, tx39clock_get, tx39clock_set,
};

struct txtime {
	u_int32_t t_hi;
	u_int32_t t_lo;
};

struct tx39clock_softc {
	struct	device sc_dev;
	tx_chipset_tag_t sc_tc;

	int sc_alarm;
	int sc_enabled;
	int sc_year;
	struct clock_ymdhms sc_epoch;
};

int	tx39clock_match(struct device *, struct cfdata *, void *);
void	tx39clock_attach(struct device *, struct device *, void *);
#ifdef TX39CLOCK_DEBUG
void	tx39clock_dump(tx_chipset_tag_t);
#endif

void	tx39clock_cpuspeed(int *, int *);

void	__tx39timer_rtcfreeze(tx_chipset_tag_t);
void	__tx39timer_rtcreset(tx_chipset_tag_t);
__inline__ void	__tx39timer_rtcget(struct txtime *);
__inline__ time_t __tx39timer_rtc2sec(struct txtime *);

struct cfattach tx39clock_ca = {
	sizeof(struct tx39clock_softc), tx39clock_match, tx39clock_attach
};

int
tx39clock_match(struct device *parent, struct cfdata *cf, void *aux)
{

	return (ATTACH_FIRST);
}

void
tx39clock_attach(struct device *parent, struct device *self, void *aux)
{
	struct txsim_attach_args *ta = aux;
	struct tx39clock_softc *sc = (void*)self;
	tx_chipset_tag_t tc;
	txreg_t reg;

	tc = sc->sc_tc = ta->ta_tc;
	tx_conf_register_clock(tc, self);

	/* Reset timer module */
	tx_conf_write(tc, TX39_TIMERCONTROL_REG, 0);

	/* Enable periodic timer */
	reg = tx_conf_read(tc, TX39_TIMERCONTROL_REG);
	reg |= TX39_TIMERCONTROL_ENPERTIMER;
	tx_conf_write(tc, TX39_TIMERCONTROL_REG, reg);

	sc->sc_enabled = 0;
	/* 
	 * RTC and ALARM 
	 *    RTCINT    ... INTR5 bit 31  (roll over)
	 *    ALARMINT  ... INTR5 bit 30
	 *    PERINT    ... INTR5 bit 29
	 */

	platform_clock_attach(self, &tx39_clock);

#ifdef TX39CLOCK_DEBUG
	tx39clock_dump(tc);
#endif /* TX39CLOCK_DEBUG */
}

/*
 * cpuclock ... CPU clock (Hz)
 * cpuspeed ... instructions-per-microsecond
 */
void
tx39clock_cpuspeed(int *cpuclock, int *cpuspeed)
{
	struct txtime t0, t1;
	int elapsed;
	
	__tx39timer_rtcget(&t0);
	__asm__ __volatile__("
		.set	noreorder;
		li	$8, 10000000;
	1:	nop;
		nop;
		nop;
		nop;
		nop;
		nop;
		nop;
		add	$8, $8, -1;
		bnez	$8, 1b;
		nop;
		.set	reorder;
	");
	__tx39timer_rtcget(&t1);

	elapsed = t1.t_lo - t0.t_lo;

	*cpuclock = (100000000 / elapsed) * TX39_RTCLOCK;
	*cpuspeed = *cpuclock / 1000000;
}

void
__tx39timer_rtcfreeze(tx_chipset_tag_t tc)
{
	txreg_t reg;	

	reg = tx_conf_read(tc, TX39_TIMERCONTROL_REG);

	/* Freeze RTC */
	reg |= TX39_TIMERCONTROL_FREEZEPRE; /* Upper 8bit */
	reg |= TX39_TIMERCONTROL_FREEZERTC; /* Lower 32bit */

	/* Freeze periodic timer */
	reg |= TX39_TIMERCONTROL_FREEZETIMER;
	reg &= ~TX39_TIMERCONTROL_ENPERTIMER;
	tx_conf_write(tc, TX39_TIMERCONTROL_REG, reg);
}

__inline__ time_t
__tx39timer_rtc2sec(struct txtime *t)
{
	/* This rely on RTC is 32.768kHz */
	return ((t->t_lo >> 15) | (t->t_hi << 17));
}

__inline__ void
__tx39timer_rtcget(struct txtime *t)
{
	tx_chipset_tag_t tc;	
	txreg_t reghi, reglo, oreghi, oreglo;
	int retry;

	tc = tx_conf_get_tag();

	retry = 10;

	do {
		oreglo = tx_conf_read(tc, TX39_TIMERRTCLO_REG);
		reglo = tx_conf_read(tc, TX39_TIMERRTCLO_REG);

		oreghi = tx_conf_read(tc, TX39_TIMERRTCHI_REG);
		reghi = tx_conf_read(tc, TX39_TIMERRTCHI_REG);
	} while ((reghi != oreghi || reglo != oreglo) && (--retry > 0));

	if (retry < 0) {
		printf("RTC timer read error.\n");
	}

	t->t_hi = TX39_TIMERRTCHI(reghi);
	t->t_lo = reglo;
}

void
__tx39timer_rtcreset(tx_chipset_tag_t tc)
{
	txreg_t reg;
	
	reg = tx_conf_read(tc, TX39_TIMERCONTROL_REG);

	/* Reset counter and stop */
	reg |= TX39_TIMERCONTROL_RTCCLR;
	tx_conf_write(tc, TX39_TIMERCONTROL_REG, reg);

	/* Count again */
	reg &= ~TX39_TIMERCONTROL_RTCCLR;
	tx_conf_write(tc, TX39_TIMERCONTROL_REG, reg);
}

void
tx39clock_init(struct device *dev)
{
	struct tx39clock_softc *sc = (void*)dev;
	tx_chipset_tag_t tc = sc->sc_tc;
	txreg_t reg;
	int pcnt;

	/* 
	 * Setup periodic timer (interrupting hz times per second.) 
	 */
	pcnt = TX39_TIMERCLK / CLOCK_RATE - 1;
	reg = tx_conf_read(tc, TX39_TIMERPERIODIC_REG);
	TX39_TIMERPERIODIC_PERVAL_CLR(reg);
	reg = TX39_TIMERPERIODIC_PERVAL_SET(reg, pcnt);
	tx_conf_write(tc, TX39_TIMERPERIODIC_REG, reg);

	/* 
	 * Enable periodic timer 
	 */
	reg = tx_conf_read(tc, TX39_INTRENABLE6_REG);
	reg |= TX39_INTRPRI13_TIMER_PERIODIC_BIT; 	
	tx_conf_write(tc, TX39_INTRENABLE6_REG, reg); 
}

void
tx39clock_get(struct device *dev, time_t base, struct clock_ymdhms *t)
{
	struct tx39clock_softc *sc = (void *)dev;
	struct clock_ymdhms dt;
	struct txtime tt;
	time_t sec;

	__tx39timer_rtcget(&tt);		
	sec = __tx39timer_rtc2sec(&tt);

	if (!sc->sc_enabled) {
		DPRINTF(("bootstrap: %d sec from previous reboot\n", 
		    (int)sec));

		sc->sc_enabled = 1;
		base += sec;
	} else {
		dt.dt_year = sc->sc_year;
		dt.dt_mon = sc->sc_epoch.dt_mon;
		dt.dt_day = sc->sc_epoch.dt_day;
		dt.dt_hour = sc->sc_epoch.dt_hour;
		dt.dt_min = sc->sc_epoch.dt_min;
		dt.dt_sec = sc->sc_epoch.dt_sec;
		dt.dt_wday = sc->sc_epoch.dt_wday;
		base = sec + clock_ymdhms_to_secs(&dt);
	}

	clock_secs_to_ymdhms(base, &dt);

	t->dt_year = dt.dt_year % 100;
	t->dt_mon = dt.dt_mon;
	t->dt_day = dt.dt_day;
	t->dt_hour = dt.dt_hour;
	t->dt_min = dt.dt_min;
	t->dt_sec = dt.dt_sec;
	t->dt_wday = dt.dt_wday;

	sc->sc_year = dt.dt_year;
}

void
tx39clock_set(struct device *dev, struct clock_ymdhms *dt)
{
	struct tx39clock_softc *sc = (void *)dev;

	if (sc->sc_enabled) {
		sc->sc_epoch = *dt;
	}
}

int
tx39clock_alarm_set(tx_chipset_tag_t tc, int msec)
{
	struct tx39clock_softc *sc = tc->tc_clockt;

	sc->sc_alarm = TX39_MSEC2RTC(msec);
	tx39clock_alarm_refill(tc);

	return (0);
}

void
tx39clock_alarm_refill(tx_chipset_tag_t tc)
{
	struct tx39clock_softc *sc = tc->tc_clockt;
	struct txtime t;	
	u_int64_t time;
	
	__tx39timer_rtcget(&t);

	time = ((u_int64_t)t.t_hi << 32) | (u_int64_t)t.t_lo;
	time += (u_int64_t)sc->sc_alarm;

	t.t_hi = (u_int32_t)((time >> 32) & TX39_TIMERALARMHI_MASK);
	t.t_lo = (u_int32_t)(time & 0xffffffff);

	tx_conf_write(tc, TX39_TIMERALARMHI_REG, t.t_hi);
	tx_conf_write(tc, TX39_TIMERALARMLO_REG, t.t_lo);
}

#ifdef TX39CLOCK_DEBUG
void
tx39clock_dump(tx_chipset_tag_t tc)
{
	txreg_t reg;

	reg = tx_conf_read(tc, TX39_CLOCKCTRL_REG);

	printf(" ");
	ISSETPRINT(reg, CHIM);
#ifdef TX391X
	ISSETPRINT(reg, VID);
	ISSETPRINT(reg, MBUS);
#endif /* TX391X */
#ifdef TX392X
	ISSETPRINT(reg, IRDA);	
#endif /* TX392X */
	ISSETPRINT(reg, SPI);
	ISSETPRINT(reg, TIMER);
	ISSETPRINT(reg, FASTTIMER);
#ifdef TX392X
	ISSETPRINT(reg, C48MOUT);
#endif /* TX392X */
	ISSETPRINT(reg, SIBM);
	ISSETPRINT(reg, CSER);
	ISSETPRINT(reg, IR);
	ISSETPRINT(reg, UARTA);
	ISSETPRINT(reg, UARTB);
	printf("\n");
}
#endif /* TX39CLOCK_DEBUG */
