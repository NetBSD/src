/*	$NetBSD: tx39clock.c,v 1.4 1999/12/23 16:58:48 uch Exp $ */

/*
 * Copyright (c) 1999, by UCHIYAMA Yasushi
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the developer may NOT be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
#include "opt_tx39_debug.h"

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/clock_subr.h>

#include <machine/bus.h>
#include <machine/clock_machdep.h>
#include <machine/cpu.h>

#include <hpcmips/tx/tx39var.h>
#include <hpcmips/tx/tx39icureg.h>
#include <hpcmips/tx/tx39clockreg.h>
#include <hpcmips/tx/tx39timerreg.h>

#include <dev/dec/clockvar.h>

#ifdef TX39CLKDEBUG
#define	DPRINTF(arg) printf arg
#else
#define	DPRINTF(arg)
#endif

#define ISSETPRINT(r, m) __is_set_print(r, TX39_CLOCK_EN##m##CLK, #m)

void	tx39clock_init __P((struct device*));
void	tx39clock_get __P((struct device*, time_t, struct clocktime*));
void	tx39clock_set __P((struct device*, struct clocktime*));

const struct clockfns tx39clockfns = {
	tx39clock_init, tx39clock_get, tx39clock_set,
};

struct txtime {
	u_int32_t t_hi;
	u_int32_t t_lo;
};

struct tx39clock_softc {
	struct	device sc_dev;
	tx_chipset_tag_t sc_tc;

	int sc_enabled;
	int sc_year;
	struct clocktime sc_epoch;
};

int	tx39clock_match __P((struct device*, struct cfdata*, void*));
void	tx39clock_attach __P((struct device*, struct device*, void*));
void	tx39clock_dump __P((tx_chipset_tag_t));

void	tx39clock_cpuspeed __P((int*, int*));

void	__tx39timer_rtcfreeze __P((tx_chipset_tag_t));
void	__tx39timer_rtcreset __P((tx_chipset_tag_t));
__inline void	__tx39timer_rtcget __P((struct txtime*));
__inline time_t	__tx39timer_rtc2sec __P((struct txtime*));

struct cfattach tx39clock_ca = {
	sizeof(struct tx39clock_softc), tx39clock_match, tx39clock_attach
};

int
tx39clock_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	return 2; /* 1st attach group of txsim */
}

void
tx39clock_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct txsim_attach_args *ta = aux;
	struct tx39clock_softc *sc = (void*)self;
	tx_chipset_tag_t tc;
	txreg_t reg;

	tc = sc->sc_tc = ta->ta_tc;

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

	clockattach(self, &tx39clockfns);	

#ifdef TX39CLKDEBUG
	tx39clock_dump(tc);
#endif /* TX39CLKDEBUG */
}

/*
 * cpuclock ... CPU clock (Hz)
 * cpuspeed ... instructions-per-microsecond
 */
void
tx39clock_cpuspeed(cpuclock, cpuspeed)
	int *cpuclock;
	int *cpuspeed;
{
	struct txtime t0, t1;
	int elapsed;
	
	__tx39timer_rtcget(&t0);
	__asm __volatile("
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
__tx39timer_rtcfreeze(tc)
	tx_chipset_tag_t tc;
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

__inline time_t
__tx39timer_rtc2sec(t)
	struct txtime *t;
{
	/* This rely on RTC is 32.768kHz */
	return (t->t_lo >> 15) | (t->t_hi << 17);
}

__inline void
__tx39timer_rtcget(t)
	struct txtime *t;
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
__tx39timer_rtcreset(tc)
 	tx_chipset_tag_t tc;	
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
tx39clock_init(dev)
	struct device *dev;
{
	tx_chipset_tag_t tc;
	txreg_t reg;
	int pcnt;

	tc = tx_conf_get_tag();

	/* 
	 * Setup periodic timer (interrupting hz times per second.) 
	 */
	pcnt = TX39_TIMERCLK / hz - 1;
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

	/* 
	 * number of microseconds between interrupts 
	 */
	tick = 1000000 / hz;
}

void
tx39clock_get(dev, base, ct)
	struct device *dev;
	time_t base;
	struct clocktime *ct;
{
	struct clock_ymdhms dt;
	struct tx39clock_softc *sc = (void*)dev;
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
		dt.dt_mon = sc->sc_epoch.mon;
		dt.dt_day = sc->sc_epoch.day;
		dt.dt_hour = sc->sc_epoch.hour;
		dt.dt_min = sc->sc_epoch.min;
		dt.dt_sec = sc->sc_epoch.sec;
		dt.dt_wday = sc->sc_epoch.dow;
		base = sec + clock_ymdhms_to_secs(&dt);
	}

	clock_secs_to_ymdhms(base, &dt);
		
	ct->year = dt.dt_year % 100;
	ct->mon = dt.dt_mon;
	ct->day = dt.dt_day;
	ct->hour = dt.dt_hour;
	ct->min = dt.dt_min;
	ct->sec = dt.dt_sec;
	ct->dow = dt.dt_wday;

	sc->sc_year = dt.dt_year;
}

void
tx39clock_set(dev, ct)
	struct device *dev;
	struct clocktime *ct;
{
	struct tx39clock_softc *sc = (void*)dev;

	if (sc->sc_enabled) {
		sc->sc_epoch = *ct;
		/* Reset RTC counter */
		__tx39timer_rtcreset(sc->sc_tc);
	}
}

void
tx39clock_dump(tc)
	tx_chipset_tag_t tc;
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
