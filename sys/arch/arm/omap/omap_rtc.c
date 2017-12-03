/*	$NetBSD: omap_rtc.c,v 1.5.12.1 2017/12/03 11:35:55 jdolecek Exp $	*/

/*
 * OMAP RTC driver, based on i80321_timer.c.
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
__KERNEL_RCSID(0, "$NetBSD: omap_rtc.c,v 1.5.12.1 2017/12/03 11:35:55 jdolecek Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/time.h>
#include <sys/device.h>
#include <dev/clock_subr.h>

#include <sys/bus.h>
#include <machine/intr.h>

#include <arm/omap/omap_reg.h>
#include <arm/omap/omap_tipb.h>

/* RTC year values relative to this year */

#define	BASEYEAR		2000

/* Register offsets and bit fields. */

#define SECONDS_REG		0x00
#define MINUTES_REG		0x04
#define HOURS_REG		0x08
#define DAYS_REG		0x0c
#define MONTHS_REG		0x10
#define YEARS_REG		0x14
#define WEEKS_REG		0x18
#define ALARM_SECONDS_REG	0x20
#define ALARM_MINUTES_REG	0x24
#define ALARM_HOURS_REG		0x28
#define ALARM_DAYS_REG		0x2c
#define ALARM_MONTHS_REG	0x30
#define ALARM_YEARS_REG		0x34

#define RTC_CTRL_REG		0x40
#define STOP_RTC		0

#define RTC_STATUS_REG		0x44
#define ALARM			6
#define BUSY			0

#define RTC_INTERRUPTS_REG	0x48
#define IT_ALARM		3

static int	omaprtc_match(device_t, cfdata_t, void *);
static void	omaprtc_attach(device_t, device_t, void *);

struct omaprtc_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	int			sc_intr;
	void			*sc_irqcookie;
	struct todr_chip_handle	sc_todr;
};

CFATTACH_DECL_NEW(omaprtc, sizeof(struct omaprtc_softc),
    omaprtc_match, omaprtc_attach, NULL, NULL);

static int omaprtc_gettime(todr_chip_handle_t, struct clock_ymdhms *);
static int omaprtc_settime(todr_chip_handle_t, struct clock_ymdhms *);

#define rtc_is_busy()	(bus_space_read_1(sc->sc_iot, sc->sc_ioh, \
					  RTC_STATUS_REG) & 1<<BUSY)

static int
omaprtc_gettime(todr_chip_handle_t tch, struct clock_ymdhms *dt)
{
	struct omaprtc_softc *sc = tch->cookie;
	int s;

	/* Wait for RTC_STATUS_REG:BUSY to go low to
	 * guarantee our read is correct.  BUSY will
	 * only be high for one 32kHz period (30.5us)
	 * each second, so we'll usually pass right
	 * through.
	 *
	 * Watch RTC_CTRL_REG:STOP_RTC as well so
	 * we don't spin forever if someone disables the RTC.
	 *
	 * Turn interrupts off, because we are only allowed
	 * to read/write the registers for 1/2 of a 32kHz
	 * clock period (= 15us) after detecting that BUSY
	 * is low.
	 */

	s = disable_interrupts(I32_bit);

	while (rtc_is_busy()) {
		;
	}

	dt->dt_year =
		bcdtobin(bus_space_read_1(sc->sc_iot,
					 sc->sc_ioh,
					 YEARS_REG)) + BASEYEAR;
	dt->dt_mon =
		bcdtobin(bus_space_read_1(sc->sc_iot,
					 sc->sc_ioh,
					 MONTHS_REG));
	dt->dt_wday =
		bcdtobin(bus_space_read_1(sc->sc_iot,
					 sc->sc_ioh,
					 WEEKS_REG) & 0x0f);
	dt->dt_day =
		bcdtobin(bus_space_read_1(sc->sc_iot,
					 sc->sc_ioh,
					 DAYS_REG));
	dt->dt_sec =
		bcdtobin(bus_space_read_1(sc->sc_iot,
					 sc->sc_ioh,
					 SECONDS_REG));
	dt->dt_hour =
		bcdtobin(bus_space_read_1(sc->sc_iot,
					 sc->sc_ioh,
					 HOURS_REG));
	dt->dt_min =
		bcdtobin(bus_space_read_1(sc->sc_iot,
					 sc->sc_ioh,
					 MINUTES_REG));
	restore_interrupts(s);
        return 0;
}

static int
omaprtc_settime(todr_chip_handle_t tch, struct clock_ymdhms *dt)
{
	struct omaprtc_softc *sc = tch->cookie;
	int s;

	s = disable_interrupts(I32_bit);

	while (rtc_is_busy()) {
		;
	}

	/* It's ok to write these without stopping the
	 * RTC, because the BUSY mechanism lets us guarantee
	 * that we're not in the middle of, e.g., rolling
	 * seconds into minutes.
	 */

	bus_space_write_1(sc->sc_iot, sc->sc_ioh,
			  YEARS_REG, bintobcd(dt->dt_year - BASEYEAR));
	bus_space_write_1(sc->sc_iot, sc->sc_ioh,
			  MONTHS_REG, bintobcd(dt->dt_mon));
	bus_space_write_1(sc->sc_iot, sc->sc_ioh,
			  WEEKS_REG, bintobcd(dt->dt_wday & 0x0f));
	bus_space_write_1(sc->sc_iot, sc->sc_ioh,
			  DAYS_REG, bintobcd(dt->dt_day));
	bus_space_write_1(sc->sc_iot, sc->sc_ioh,
			  SECONDS_REG, bintobcd(dt->dt_sec));
	bus_space_write_1(sc->sc_iot, sc->sc_ioh,
			  HOURS_REG, bintobcd(dt->dt_hour));
	bus_space_write_1(sc->sc_iot, sc->sc_ioh,
			  MINUTES_REG, bintobcd(dt->dt_min));
	restore_interrupts(s);
        return 0;
}

static int
omaprtc_match(device_t parent, cfdata_t match, void *aux)
{
	struct tipb_attach_args *tipb = aux;

	if (tipb->tipb_addr == -1)
	    panic("omaprtc must have addr specified in config.");

	if (tipb->tipb_size == 0)
		tipb->tipb_size = 2048;	/* Per the OMAP TRM. */

	/* We implicitly trust the config file. */
	return (1);
}

void
omaprtc_attach(device_t parent, device_t self, void *aux)
{
	struct omaprtc_softc *sc = device_private(self);
	struct tipb_attach_args *tipb = aux;

	sc->sc_iot = tipb->tipb_iot;
	sc->sc_intr = tipb->tipb_intr;

	if (bus_space_map(tipb->tipb_iot, tipb->tipb_addr, tipb->tipb_size, 0,
			 &sc->sc_ioh))
		panic("%s: Cannot map registers", device_xname(self));

	aprint_normal(": OMAP RTC\n");
	aprint_naive("\n");

	/*
	 * Start RTC (STOP_RTC=1 starts it, go figure), 24 hour mode,
	 * no autocompensation, no rounding, 32KHz clock enabled,
	 * cannot use split power, no test mode.
	 */

	bus_space_write_1(sc->sc_iot, sc->sc_ioh, RTC_CTRL_REG,
			  1 << STOP_RTC);

	sc->sc_todr.cookie = sc;
	sc->sc_todr.todr_gettime_ymdhms = omaprtc_gettime;
	sc->sc_todr.todr_settime_ymdhms = omaprtc_settime;
	todr_attach(&sc->sc_todr);
}
