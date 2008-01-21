/*	$NetBSD: tx39clock.c,v 1.17.2.3 2008/01/21 09:36:39 yamt Exp $ */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tx39clock.c,v 1.17.2.3 2008/01/21 09:36:39 yamt Exp $");

#include "opt_tx39clock_debug.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/timetc.h>
#include <sys/device.h>
#include <sys/bus.h>

#include <dev/clock_subr.h>

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

struct platform_clock tx39_clock = {
#define CLOCK_RATE	100
	CLOCK_RATE, tx39clock_init,
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
	struct timecounter sc_tcounter;
};

int	tx39clock_match(struct device *, struct cfdata *, void *);
void	tx39clock_attach(struct device *, struct device *, void *);
#ifdef TX39CLOCK_DEBUG
void	tx39clock_dump(tx_chipset_tag_t);
#endif

void	tx39clock_cpuspeed(int *, int *);

void	__tx39timer_rtcfreeze(tx_chipset_tag_t);
void	__tx39timer_rtcreset(tx_chipset_tag_t);
inline void	__tx39timer_rtcget(struct txtime *);
inline time_t __tx39timer_rtc2sec(struct txtime *);
uint32_t tx39_timecount(struct timecounter *);

CFATTACH_DECL(tx39clock, sizeof(struct tx39clock_softc),
    tx39clock_match, tx39clock_attach, NULL, NULL);

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
tx39clock_cpuspeed(int *cpuclock, int *cpu_speed)
{
	struct txtime t0, t1;
	int elapsed;
	
	__tx39timer_rtcget(&t0);
	__asm volatile(
		".set	noreorder;		\n\t"
		"li	$8, 10000000;		\n"
	"1:	nop;				\n\t"
		"nop;				\n\t"
		"nop;				\n\t"
		"nop;				\n\t"
		"nop;				\n\t"
		"nop;				\n\t"
		"nop;				\n\t"
		"add	$8, $8, -1;		\n\t"
		"bnez	$8, 1b;			\n\t"
		"nop;				\n\t"
		".set	reorder;");
	__tx39timer_rtcget(&t1);

	elapsed = t1.t_lo - t0.t_lo;

	*cpuclock = (100000000 / elapsed) * TX39_RTCLOCK;
	*cpu_speed = *cpuclock / 1000000;
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

inline void
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

uint32_t
tx39_timecount(struct timecounter *tch)
{
	tx_chipset_tag_t tc = tch->tc_priv;

	/*
	 * since we're only reading the low register, we don't care about
	 * if the chip increments it.  we assume that the single read will
	 * always be consistent.  This is much faster than the routine which
	 * has to get both values, improving the quality.
	 */
	return (tx_conf_read(tc, TX39_TIMERRTCLO_REG));
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

	sc->sc_tcounter.tc_name = "tx39rtc";
	sc->sc_tcounter.tc_get_timecount = tx39_timecount;
	sc->sc_tcounter.tc_priv = tc;
	sc->sc_tcounter.tc_counter_mask = 0xffffffff;
	sc->sc_tcounter.tc_frequency = TX39_RTCLOCK;
	sc->sc_tcounter.tc_quality = 100;
	tc_init(&sc->sc_tcounter);
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
	u_int64_t mytime;
	
	__tx39timer_rtcget(&t);

	mytime = ((u_int64_t)t.t_hi << 32) | (u_int64_t)t.t_lo;
	mytime += (u_int64_t)sc->sc_alarm;

	t.t_hi = (u_int32_t)((mytime >> 32) & TX39_TIMERALARMHI_MASK);
	t.t_lo = (u_int32_t)(mytime & 0xffffffff);

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
