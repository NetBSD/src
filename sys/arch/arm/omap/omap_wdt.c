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
#include <sys/bus.h>
#include <dev/sysmon/sysmonvar.h>

#include <arm/omap/omap_wdtvar.h>
#include <arm/omap/omap_wdtreg.h>

struct omapwdt32k_softc *omapwdt32k_sc;
int omapwdt_sysconfig;

static void
omapwdt32k_sync(void)
{
	/* The WCLR, WCRR, WLDR, WTGR and WSPR registers are
	 * synchronized to the 32kHz clock.  Each has an
	 * associated status bit that's set when there's
	 * a pending write.  This function will wait for
	 * all of those status bits to become clear so
	 * the caller can safely write to any of the registers.
	 */
	while (bus_space_read_4(omapwdt32k_sc->sc_iot, omapwdt32k_sc->sc_ioh,
				WWPS) &
	       (W_PEND_WSPR | W_PEND_WTGR | W_PEND_WLDR | W_PEND_WCRR |
		W_PEND_WCLR))
		delay(10);
}

static void
omapwdt32k_start(void)
{
	omapwdt32k_sync();
	bus_space_write_4(omapwdt32k_sc->sc_iot, omapwdt32k_sc->sc_ioh, WSPR,
			  WD_ENABLE_WORD1);
	omapwdt32k_sync();
	bus_space_write_4(omapwdt32k_sc->sc_iot, omapwdt32k_sc->sc_ioh, WSPR,
			  WD_ENABLE_WORD2);
}

static void
omapwdt32k_stop(void)
{
	omapwdt32k_sync();
	bus_space_write_4(omapwdt32k_sc->sc_iot, omapwdt32k_sc->sc_ioh, WSPR,
			  WD_DISABLE_WORD1);
	omapwdt32k_sync();
	bus_space_write_4(omapwdt32k_sc->sc_iot, omapwdt32k_sc->sc_ioh, WSPR,
			  WD_DISABLE_WORD2);
}

static void
omapwdt32k_do_set_timeout(void)
{
	if (omapwdt32k_sc->sc_armed) {
		/*
		 * The watchdog must be disabled before writing to
		 * the WCRR, WCLR:PTV, or WLDR registers.
		 */
		omapwdt32k_stop();
	}

	/* Make sure WLDR, WCRR, and WCLR are ready to be written to.
	 */
	omapwdt32k_sync();

	/* Make sure that the prescaler is set.
	 */
	bus_space_write_4(omapwdt32k_sc->sc_iot, omapwdt32k_sc->sc_ioh, WCLR,
			  (WCLR_PTV(PTV) | WCLR_PRE(PRE)));

	/* Write the new count value to the load and
	 * counter registers.
	 */

	bus_space_write_4(omapwdt32k_sc->sc_iot, omapwdt32k_sc->sc_ioh, WLDR,
			  omapwdt32k_sc->sc_smw.smw_period ?
			  WATCHDOG_COUNT(omapwdt32k_sc->sc_smw.smw_period)
			  : 0xffffffff);
	bus_space_write_4(omapwdt32k_sc->sc_iot, omapwdt32k_sc->sc_ioh, WCRR,
			  omapwdt32k_sc->sc_smw.smw_period ?
			  WATCHDOG_COUNT(omapwdt32k_sc->sc_smw.smw_period)
			  : 0xffffffff);

	/* Wait for the pending writes to go through.
	 */
	omapwdt32k_sync();

	/* Resume the watchdog if we had stopped it.
	 */
	if (omapwdt32k_sc->sc_armed)
		omapwdt32k_start();
}

void
omapwdt32k_set_timeout(unsigned int period)
{
	int s = splhigh();

	if (period != omapwdt32k_sc->sc_smw.smw_period) {
		omapwdt32k_sc->sc_smw.smw_period = period;
		omapwdt32k_do_set_timeout();
	}

	splx(s);
}

int
omapwdt32k_enable(int enable)
{
	int s;
	int prev_state;

	/* Just return if ddb is entered before the watchdog driver starts. */
	if (omapwdt32k_sc == NULL)
		return (0);

	prev_state = omapwdt32k_sc->sc_armed;

	/* Normalize the int to a boolean so we can compare values directly.
	 */
	enable = !!enable;

	s = splhigh();

	if (enable != omapwdt32k_sc->sc_armed) {
		if (enable) {
			/* Make sure that the watchdog timeout is up to date.
			 */
			omapwdt32k_do_set_timeout();
			omapwdt32k_start();
		} else {
			omapwdt32k_stop();
		}
		omapwdt32k_sc->sc_armed = enable;
	}

	splx(s);
	return prev_state;
}

int
omapwdt32k_setmode(struct sysmon_wdog *smw)
{
	struct omapwdt32k_softc *sc = smw->smw_cookie;
	int error = 0;

	if ((smw->smw_mode & WDOG_MODE_MASK) == WDOG_MODE_DISARMED) {
		omapwdt32k_enable(0);
	} else {
		if (smw->smw_period == WDOG_PERIOD_DEFAULT)
			sc->sc_smw.smw_period = OMAPWDT32K_DEFAULT_PERIOD;
		else
			sc->sc_smw.smw_period = smw->smw_period;
		omapwdt32k_set_timeout(sc->sc_smw.smw_period);
		omapwdt32k_enable(1);
		if ((smw->smw_mode & WDOG_MODE_MASK) == WDOG_MODE_KTICKLE)
			omapwdt32k_tickle(smw);
	}
	return error;
}

int
omapwdt32k_tickle(struct sysmon_wdog *smw)
{
	int s = splhigh();

	/*
	 *     To reload the timer counter and reset the prescaler counter
	 *     values without reaching overflow, a reload command can be
	 *     executed by accessing the watchdog trigger register (WTGR)
	 *     using a specific reload sequence.
	 *
	 *     The specific reload sequence is performed when the written
	 *     value on the watchdog trigger register (WTGR) is different
	 *     from its previous value. In this case, reload is executed in
	 *     the same way as overflow autoreload, without a reset pulse
	 *     generation. The timer is loaded with the watchdog load register
	 *     (WLDR) value and the prescaler counter is reset.
	 *
	 * Write a new value into WTGR to reload WCRR (counter register)
	 * with the value in WLDR (load register), thereby resetting the
	 * watchdog.
	 */
	omapwdt32k_sync();
	bus_space_write_4(omapwdt32k_sc->sc_iot, omapwdt32k_sc->sc_ioh, WTGR,
			  ~bus_space_read_4(omapwdt32k_sc->sc_iot,
					    omapwdt32k_sc->sc_ioh, WTGR));

	splx(s);
	return 0;
}

void
omapwdt32k_reboot(void)
{
	if (omapwdt32k_sc == NULL)
		return;

	const int s = splhigh();

	omapwdt32k_set_timeout(0);
	omapwdt32k_start();
	delay(100);
	splx(s);
}
