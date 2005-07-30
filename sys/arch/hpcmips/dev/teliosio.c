/*	$NetBSD: teliosio.c,v 1.1 2005/07/30 22:40:33 nakayama Exp $	*/

/*
 * Copyright (c) 2005 Takeshi Nakayama.
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Sharp Telios machine dependent I/O (LCD backlight/Battery unit) driver
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: teliosio.c,v 1.1 2005/07/30 22:40:33 nakayama Exp $");

#include "apmdev.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/config_hook.h>

#include <hpcmips/tx/tx39var.h>
#include <hpcmips/tx/txcsbusvar.h>

#if NAPMDEV > 0
#include <dev/apm/apmbios.h>
#define __TX39IO_PRIVATE
#include <hpcmips/tx/tx39ioreg.h>
#include <hpcmips/tx/tx39timerreg.h>
#endif

#include <hpcmips/dev/teliosioreg.h>

struct teliosio_softc {
	struct device sc_dev;
	tx_chipset_tag_t sc_tc;
	bus_space_tag_t sc_regt;
	bus_space_handle_t sc_regh;

	int sc_backlight;
	int sc_brightness;
#define MAX_BRIGHTNESS 9
#define BRIGHTNESSVAL(n) \
	((n) <= 0 ? 0 : ((n) <= MAX_BRIGHTNESS ? (n) : MAX_BRIGHTNESS) * 9 - 1)
};

static int teliosio_match(struct device *, struct cfdata *, void *);
static void teliosio_attach(struct device *, struct device *, void *);
static int teliosio_event(void *, int, long, void *);

/* LCD backlight control */
static void teliosio_backlight(struct teliosio_softc *, int);
static void teliosio_brightness(struct teliosio_softc *, int);

/* Battery unit control */
#if NAPMDEV > 0
static int teliosio_ac_state(struct teliosio_softc *);
static int teliosio_mbu_state(struct teliosio_softc *);
static void teliosio_mbu_write(tx_chipset_tag_t, int);
static int teliosio_mbu_read(tx_chipset_tag_t);
#endif

CFATTACH_DECL(teliosio, sizeof(struct teliosio_softc),
	      teliosio_match, teliosio_attach, NULL, NULL);

static int
teliosio_match(struct device *parent, struct cfdata *cf, void *aux)
{

	return 2;	/* attach before plumvideo */
}

static void
teliosio_attach(struct device *parent, struct device *self, void *aux)
{
	struct cs_attach_args *ca = aux;
	struct teliosio_softc *sc = (void *)self;

	sc->sc_tc = ca->ca_tc;
	sc->sc_regt = ca->ca_csreg.cstag;

	if (bus_space_map(sc->sc_regt, TELIOSIO_REGBASE, TELIOSIO_REGSIZE,
			  0, &sc->sc_regh)) {
		printf(": register map failed\n");
		return;
	}
	printf("\n");

	sc->sc_backlight = 1;
	sc->sc_brightness = MAX_BRIGHTNESS;

	config_hook(CONFIG_HOOK_SET, CONFIG_HOOK_POWER_LCDLIGHT,
		    CONFIG_HOOK_SHARE, teliosio_event, sc);
	config_hook(CONFIG_HOOK_GET, CONFIG_HOOK_POWER_LCDLIGHT,
		    CONFIG_HOOK_SHARE, teliosio_event, sc);
	config_hook(CONFIG_HOOK_SET, CONFIG_HOOK_BRIGHTNESS,
		    CONFIG_HOOK_SHARE, teliosio_event, sc);
	config_hook(CONFIG_HOOK_GET, CONFIG_HOOK_BRIGHTNESS,
		    CONFIG_HOOK_SHARE, teliosio_event, sc);
	config_hook(CONFIG_HOOK_GET, CONFIG_HOOK_BRIGHTNESS_MAX,
		    CONFIG_HOOK_SHARE, teliosio_event, sc);
#if NAPMDEV > 0
	config_hook(CONFIG_HOOK_GET, CONFIG_HOOK_ACADAPTER,
		    CONFIG_HOOK_SHARE, teliosio_event, sc);
	config_hook(CONFIG_HOOK_GET, CONFIG_HOOK_BATTERYVAL,
		    CONFIG_HOOK_SHARE, teliosio_event, sc);
#endif
}

static int
teliosio_event(void *ctx, int type, long id, void *msg)
{
	struct teliosio_softc *sc = ctx;

	switch (type) {
	case CONFIG_HOOK_SET:
		switch (id) {
		case CONFIG_HOOK_POWER_LCDLIGHT:
			teliosio_backlight(sc, (int)msg);
			break;
		case CONFIG_HOOK_BRIGHTNESS:
			teliosio_brightness(sc, *(int *)msg + 1);
			break;
		default:
			return 1;
		}
		break;
	case CONFIG_HOOK_GET:
		switch (id) {
		case CONFIG_HOOK_POWER_LCDLIGHT:
			*(int *)msg = sc->sc_backlight;
			break;
		case CONFIG_HOOK_BRIGHTNESS:
			*(int *)msg = sc->sc_brightness - 1;
			break;
		case CONFIG_HOOK_BRIGHTNESS_MAX:
			*(int *)msg = MAX_BRIGHTNESS - 1;
			break;
#if NAPMDEV > 0
		case CONFIG_HOOK_ACADAPTER:
			*(int *)msg = teliosio_ac_state(sc);
			break;
		case CONFIG_HOOK_BATTERYVAL:
			*(int *)msg = teliosio_mbu_state(sc) / 2;
			break;
#endif
		default:
			return 1;
		}
		break;
	default:
		return 1;
	}

	return 0;
}

/*
 * LCD backlight control
 */
static void
teliosio_backlight(struct teliosio_softc *sc, int on)
{
	int brightness;

	sc->sc_backlight = on;
	if (on) {
		teliosio_brightness(sc, sc->sc_brightness);
	} else {
		brightness = sc->sc_brightness;
		teliosio_brightness(sc, 0);
		sc->sc_brightness = brightness;
	}
}

static void
teliosio_brightness(struct teliosio_softc *sc, int val)
{
	bus_space_tag_t regt = sc->sc_regt;
	bus_space_handle_t regh = sc->sc_regh;
	u_int16_t period;

	if (val < 0)
		val = 0;
	else if (val > MAX_BRIGHTNESS)
		val = MAX_BRIGHTNESS;
	sc->sc_brightness = val;

	period = bus_space_read_2(regt, regh, TELIOSIO_BACKLIGHT_PERIOD);
	if (val)
		period |= TELIOSIO_BACKLIGHT_PERIOD_EN;
	else
		period &= ~TELIOSIO_BACKLIGHT_PERIOD_EN;
	bus_space_write_2(regt, regh, TELIOSIO_BACKLIGHT_PERIOD, period);
	bus_space_write_2(regt, regh, TELIOSIO_BACKLIGHT_RESET,
			  BRIGHTNESSVAL(val));
}

/*
 * Battery unit control
 */
#if NAPMDEV > 0
static int
teliosio_ac_state(struct teliosio_softc *sc)
{
	tx_chipset_tag_t tc = sc->sc_tc;
	txreg_t reg;

	reg = tx_conf_read(tc, TX392X_IODATAINOUT_REG);
	return (reg & TELIOSIO_AC_STATE) ? APM_AC_OFF : APM_AC_ON;
}

static int
teliosio_mbu_state(struct teliosio_softc *sc)
{
	tx_chipset_tag_t tc = sc->sc_tc;
	int val;

	teliosio_mbu_write(tc, 0x00);
	delay(7000);
	val = teliosio_mbu_read(tc);
	delay(7000);

	return val;
}

#define WAIT (1000000 / TELIOSIO_BMU_CLOCK_FREQ / 2)

static void
teliosio_mbu_write(tx_chipset_tag_t tc, int val)
{
	txreg_t reg;
	int s;
	int mask;

	for (mask = 0x80; mask != 0; mask >>= 1) {
		s = splhigh();
		reg = tx_conf_read(tc, TX392X_IODATAINOUT_REG);
		reg |= TELIOSIO_BMU_CLOCK;
		if (val & mask)
			reg |= TELIOSIO_BMU_DATAOUT;
		else
			reg &= ~TELIOSIO_BMU_DATAOUT;
		tx_conf_write(tc, TX392X_IODATAINOUT_REG, reg);
		splx(s);
		delay(WAIT);

		s = splhigh();
		reg = tx_conf_read(tc, TX392X_IODATAINOUT_REG);
		reg &= ~TELIOSIO_BMU_CLOCK;
		tx_conf_write(tc, TX392X_IODATAINOUT_REG, reg);
		splx(s);
		delay(WAIT);
	}
}

static int
teliosio_mbu_read(tx_chipset_tag_t tc)
{
	txreg_t reg;
	int s;
	int mask, val;

	val = 0;
	for (mask = 0x80; mask != 0; mask >>= 1) {
		s = splhigh();
		reg = tx_conf_read(tc, TX392X_IODATAINOUT_REG);
		reg |= TELIOSIO_BMU_CLOCK;
		tx_conf_write(tc, TX392X_IODATAINOUT_REG, reg);
		splx(s);
		delay(WAIT);

		s = splhigh();
		reg = tx_conf_read(tc, TX392X_IODATAINOUT_REG);
		if (!(reg & TELIOSIO_BMU_DATAIN))
			val |= mask;
		reg &= ~TELIOSIO_BMU_CLOCK;
		tx_conf_write(tc, TX392X_IODATAINOUT_REG, reg);
		splx(s);
		delay(WAIT);
	}

	return val;
}
#endif /* NAPMDEV */
