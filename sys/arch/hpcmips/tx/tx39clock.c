/*	$NetBSD: tx39clock.c,v 1.2 1999/12/07 17:08:10 uch Exp $ */

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
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/clock_machdep.h>
#include <machine/cpu.h>

#include <hpcmips/tx/tx39var.h>
#include <hpcmips/tx/tx39icureg.h> /* XXX */
#include <hpcmips/tx/tx39clockreg.h>
#include <hpcmips/tx/tx39timerreg.h>
#include <dev/dec/clockvar.h>

#define ISSETPRINT(r, m) __is_set_print(r, TX39_CLOCK_EN##m##CLK, #m)

void	clock_init __P((struct device*));
void	clock_get __P((struct device*, time_t, struct clocktime*));
void	clock_set __P((struct device*, struct clocktime*));

static const struct clockfns clockfns = {
	clock_init, clock_get, clock_set,
};

int	tx39clock_match __P((struct device*, struct cfdata*, void*));
void	tx39clock_attach __P((struct device*, struct device*, void*));
void	tx39clock_dump __P((tx_chipset_tag_t));

void	tx39timer_freeze __P((tx_chipset_tag_t));
void	tx39timer_rtcreset __P((tx_chipset_tag_t));

struct tx39clock_softc {
	struct	device sc_dev;
	tx_chipset_tag_t sc_tc;
};

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

	/* 
	 *	Enable periodic timer 
	 *	 but interrupt don't arise yet. see clock_init().
	 */
	reg = tx_conf_read(tc, TX39_TIMERCONTROL_REG);
	reg |= TX39_TIMERCONTROL_ENPERTIMER;
	tx_conf_write(tc, TX39_TIMERCONTROL_REG, reg);
	
	/* Set counter */
#if 0
	{
		int cnt = 0xffff; /* XXX the most slower. */
		reg = tx_conf_read(tc, TX39_TIMERPERIODIC_REG);
		reg = TX39_TIMERPERIODIC_PERVAL_SET(reg, cnt);
		tx_conf_write(tc, TX39_TIMERPERIODIC_REG, reg);
	}
#endif
	clockattach(self, &clockfns);	

#ifdef TX39CLKDEBUG
	tx39clock_dump(tc);
#endif /* TX39CLKDEBUG */
}

/* 
 * RTC and ALARM 
 *    RTCINT    ... INTR5 bit 31  (roll over)
 *    ALARMINT  ... INTR5 bit 30
 *    PERINT    ... INTR5 bit 29
 */
void
tx39timer_freeze(tc)
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

void
tx39timer_rtcreset(tc)
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
clock_init(dev)
	struct device *dev;
{
	tx_chipset_tag_t tc;
	txreg_t reg;

	tc = tx_conf_get_tag();
	/* Enable periodic timer */
	reg = tx_conf_read(tc, TX39_INTRENABLE6_REG);
	reg |= TX39_INTRPRI13_TIMER_PERIODIC_BIT; 	
	tx_conf_write(tc, TX39_INTRENABLE6_REG, reg); 

}

void
clock_get(dev, base, ct)
	struct device *dev;
	time_t base;
	struct clocktime *ct;
{
	tx_chipset_tag_t tc;	
	txreg_t reghi, reglo, oreghi, oreglo;
	int i;

	tc = tx_conf_get_tag();
	i = 10;
	do {
		reghi = tx_conf_read(tc, TX39_TIMERRTCHI_REG);
		reglo = tx_conf_read(tc, TX39_TIMERRTCLO_REG);
		oreghi = tx_conf_read(tc, TX39_TIMERRTCHI_REG);
		oreglo = tx_conf_read(tc, TX39_TIMERRTCLO_REG);
	} while ((reghi != oreghi || reglo != oreglo) && (--i > 0));
	if (i < 0) {
		panic("RTC timer read error.\n");
	}
	/* XXX not coded yet */
}

void
clock_set(dev, ct)
	struct device *dev;
	struct clocktime *ct;
{
	/* XXX not coded yet */
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




