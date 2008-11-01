/*	$NetBSD: gemini_wdt.c,v 1.2 2008/11/01 07:43:19 cliff Exp $	*/

/*
 * OMAP watchdog timers, common code
 *
 * Copyright (c) 2007 Microsoft
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
 *	This product includes software developed by Microsoft
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTERS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/callout.h>
#include <sys/cdefs.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/wdog.h>

#include <machine/param.h>
#include <machine/bus.h>
#include <dev/sysmon/sysmonvar.h>

#include <arm/gemini/gemini_wdtvar.h>
#include <arm/gemini/gemini_reg.h>

geminiwdt_softc_t *geminiwdt_sc;

#define WATCHDOG_COUNT(nsec)	\
		(GEMINI_WDT_CLOCK_FREQ * (nsec))


static void
geminiwdt_start(void)
{
	geminiwdt_softc_t *sc = geminiwdt_sc;
	uint32_t r;

	r = bus_space_read_4(sc->sc_iot, sc->sc_ioh, GEMINI_WDT_WDCR);
	r |= (WDT_WDCR_RESET_ENB
	     |WDT_WDCR_ENB);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, GEMINI_WDT_WDCR, r);
}

static void
geminiwdt_stop(void)
{
	geminiwdt_softc_t *sc = geminiwdt_sc;
	uint32_t r;

	r = bus_space_read_4(sc->sc_iot, sc->sc_ioh, GEMINI_WDT_WDCR);
	r &= ~(WDT_WDCR_EXTSIG_ENB
	      |WDT_WDCR_RESET_ENB
	      |WDT_WDCR_INTR_ENB
	      |WDT_WDCR_ENB);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, GEMINI_WDT_WDCR, r);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh,
		GEMINI_WDT_WDCLEAR, WDT_WDCLEAR_CLEAR);
}

static void
geminiwdt_do_set_timeout(void)
{
	geminiwdt_softc_t *sc = geminiwdt_sc;
	uint32_t r;

	/*
	 * Disable the watchdog timer
	 */
	if (sc->sc_armed)
		geminiwdt_stop();

	/*
	 * Set WdLoad register
	 */
	r = (sc->sc_smw.smw_period != 0) ?
		WATCHDOG_COUNT(sc->sc_smw.smw_period) : WDT_WDLOAD_DFLT;
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, GEMINI_WDT_WDLOAD, r);

	/*
	 * feed MAGIC treat to dog
	 */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh,
		GEMINI_WDT_WDRESTART, WDT_WDRESTART_MAGIC);

	/*
	 * Select PCLK clock source
	 */
	r = bus_space_read_4(sc->sc_iot, sc->sc_ioh, GEMINI_WDT_WDCR);
	r &= ~WDCR_CLKSRC_5MHZ;
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, GEMINI_WDT_WDCR, r);

	/*
	 * Enable the timer
	 */
	if (sc->sc_armed)
		geminiwdt_start();
}

void
geminiwdt_set_timeout(unsigned int period)
{
	geminiwdt_softc_t *sc = geminiwdt_sc;
	int s = splhigh();

	if (period != sc->sc_smw.smw_period) {
		sc->sc_smw.smw_period = period;
		geminiwdt_do_set_timeout();
	}

	splx(s);
}

int
geminiwdt_enable(int enable)
{
	geminiwdt_softc_t *sc = geminiwdt_sc;
	int s;
	int prev_state = geminiwdt_sc->sc_armed;

	/* Normalize the int to a boolean so we can compare values directly.
	 */
	enable = !!enable;

	s = splhigh();

	if (enable != sc->sc_armed) {
		if (enable) {
			/* Make sure that the watchdog timeout is up to date.
			 */
			geminiwdt_do_set_timeout();
			geminiwdt_start();
		} else {
			geminiwdt_stop();
		}
		sc->sc_armed = enable;
	}

	splx(s);
	return prev_state;
}

int
geminiwdt_setmode(struct sysmon_wdog *smw)
{
	geminiwdt_softc_t *sc = smw->smw_cookie;
	int error = 0;

	if ((smw->smw_mode & WDOG_MODE_MASK) == WDOG_MODE_DISARMED) {
		geminiwdt_enable(0);
	} else {
		if (smw->smw_period == WDOG_PERIOD_DEFAULT)
			sc->sc_smw.smw_period = WDT_WDLOAD_DFLT;
		else
			sc->sc_smw.smw_period = smw->smw_period;
		geminiwdt_set_timeout(sc->sc_smw.smw_period);
		geminiwdt_enable(1);
	}
	return error;
}

int
geminiwdt_tickle(struct sysmon_wdog *smw)
{
	geminiwdt_softc_t *sc = geminiwdt_sc;
	int s = splhigh();

	/*
	 * feed the dog a MAGIC treat
	 */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh,
		GEMINI_WDT_WDRESTART, WDT_WDRESTART_MAGIC);

	splx(s);
	return 0;
}
