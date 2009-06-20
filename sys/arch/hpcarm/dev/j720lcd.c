/*	$NetBSD: j720lcd.c,v 1.3.64.2 2009/06/20 07:20:04 yamt Exp $	*/

/*-
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by IWAMOTO Toshihiro.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

/* Jornada 720 LCD screen driver. */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: j720lcd.c,v 1.3.64.2 2009/06/20 07:20:04 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>

#include <machine/config_hook.h>
#include <machine/platid.h>
#include <machine/platid_mask.h>

#include <arm/sa11x0/sa11x0_var.h>
#include <arm/sa11x0/sa11x0_gpioreg.h>
#include <arm/sa11x0/sa11x0_ppcreg.h>
#include <arm/sa11x0/sa11x0_sspreg.h>

#include <hpcarm/dev/j720sspvar.h>
#include <hpcarm/dev/sed1356var.h>

#ifdef DEBUG
#define DPRINTF(arg)	aprint_normal arg
#else
#define DPRINTF(arg)	/* nothing */
#endif

struct j720lcd_softc {
	device_t		sc_dev;

	struct j720ssp_softc	*sc_ssp;
};

static int	j720lcd_match(device_t, cfdata_t, void *);
static void	j720lcd_attach(device_t, device_t, void *);

static int	j720lcd_param(void *, int, long, void *);
int		j720lcd_power(void *, int, long, void *);

CFATTACH_DECL_NEW(j720lcd, sizeof(struct j720lcd_softc),
    j720lcd_match, j720lcd_attach, NULL, NULL);


static int
j720lcd_match(device_t parent, cfdata_t cf, void *aux)
{

	if (!platid_match(&platid, &platid_mask_MACH_HP_JORNADA_7XX))
		return 0;
	if (strcmp(cf->cf_name, "j720lcd") != 0)
		return 0;

	return 1;
}

static void
j720lcd_attach(device_t parent, device_t self, void *aux)
{
	struct j720lcd_softc *sc = device_private(self);
	int brightness, contrast;

	sc->sc_dev = self;
	sc->sc_ssp = device_private(parent);

	/* LCD brightness hooks. */
	config_hook(CONFIG_HOOK_SET, CONFIG_HOOK_BRIGHTNESS,
		    CONFIG_HOOK_SHARE, j720lcd_param, sc);
	config_hook(CONFIG_HOOK_GET, CONFIG_HOOK_BRIGHTNESS,
		    CONFIG_HOOK_SHARE, j720lcd_param, sc);
	config_hook(CONFIG_HOOK_GET, CONFIG_HOOK_BRIGHTNESS_MAX,
		    CONFIG_HOOK_SHARE, j720lcd_param, sc);

	/* LCD contrast hooks. */
	config_hook(CONFIG_HOOK_SET, CONFIG_HOOK_CONTRAST,
		    CONFIG_HOOK_SHARE, j720lcd_param, sc);
	config_hook(CONFIG_HOOK_GET, CONFIG_HOOK_CONTRAST,
		    CONFIG_HOOK_SHARE, j720lcd_param, sc);
	config_hook(CONFIG_HOOK_GET, CONFIG_HOOK_CONTRAST_MAX,
		    CONFIG_HOOK_SHARE, j720lcd_param, sc);

	/* LCD power hook. */
#if 0
	config_hook(CONFIG_HOOK_POWERCONTROL,
		    CONFIG_HOOK_POWERCONTROL_LCDLIGHT,
		    CONFIG_HOOK_SHARE, j720lcd_power, sc);
#endif

	/* Get default brightness/contrast values. */
	config_hook_call(CONFIG_HOOK_GET, CONFIG_HOOK_BRIGHTNESS, &brightness);
	config_hook_call(CONFIG_HOOK_GET, CONFIG_HOOK_CONTRAST, &contrast);

	aprint_normal(": brightness %d, contrast %d\n", brightness, contrast);
}

static int
j720lcd_param(void *ctx, int type, long id, void *msg)
{
	struct j720lcd_softc *sc = ctx;
	struct j720ssp_softc *ssp = sc->sc_ssp;
	uint32_t data[2], len;
	const int maxval = 255;
	int i, s;

	switch (type) {
	case CONFIG_HOOK_GET:
		switch (id) {
		case CONFIG_HOOK_BRIGHTNESS_MAX:
		case CONFIG_HOOK_CONTRAST_MAX:
			*(int *)msg = maxval;
			return 1;
		case CONFIG_HOOK_BRIGHTNESS:
			data[0] = 0xd6;
			data[1] = 0x11;
			len = 2;
			break;
		case CONFIG_HOOK_CONTRAST:
			data[0] = 0xd4;
			data[1] = 0x11;
			len = 2;
			break;
		default:
			return 0;
		}
		break;

	case CONFIG_HOOK_SET:
		switch (id) {
		case CONFIG_HOOK_BRIGHTNESS:
			if (*(int *)msg >= 0) {
				data[0] = 0xd3;
				data[1] = maxval - *(int *)msg;
				len = 2;
			} else {
				/* XXX hack */
				data[0] = 0xdf;
				len = 1;
			}
			break;
		case CONFIG_HOOK_CONTRAST:
			data[0] = 0xd1;
			data[1] = maxval - *(int *)msg;
			len = 2;
			break;
		default:
			return 0;
		}
		break;

	default:
		return 0;
	}

	s = splbio();
	bus_space_write_4(ssp->sc_iot, ssp->sc_gpioh, SAGPIO_PCR, 0x2000000);

	for (i = 0; i < len; i++) {
		if (j720ssp_readwrite(ssp, 1, data[i], &data[i], 500) < 0)
			goto out;
	}
	bus_space_write_4(ssp->sc_iot, ssp->sc_gpioh, SAGPIO_PSR, 0x2000000);
	splx(s);

	if (type == CONFIG_HOOK_SET)
		return 1;

	*(int *)msg = maxval - data[1];

	return 1;

out:
	bus_space_write_4(ssp->sc_iot, ssp->sc_gpioh, SAGPIO_PSR, 0x2000000);

	/* reset SSP */
	bus_space_write_4(ssp->sc_iot, ssp->sc_ssph, SASSP_CR0, 0x307);
	delay(100);
	bus_space_write_4(ssp->sc_iot, ssp->sc_ssph, SASSP_CR0, 0x387);

	splx(s);

	DPRINTF(("j720lcd_param: error %x %x\n", data[0], data[1]));
	return 0;

}

int
j720lcd_power(void *ctx, int type, long id, void *msg)
{
	struct sed1356_softc *sc = ctx;
	struct sa11x0_softc *psc = sc->sc_parent;
	uint32_t reg;
	int val;

	if (type != CONFIG_HOOK_POWERCONTROL ||
	    id != CONFIG_HOOK_POWERCONTROL_LCDLIGHT)
		return 0;

	sed1356_init_brightness(sc, 0);
	sed1356_init_contrast(sc, 0);

	if (msg) {
		bus_space_write_1(sc->sc_iot, sc->sc_regh, 0x1f0, 0);

		reg = bus_space_read_4(psc->sc_iot, psc->sc_ppch, SAPPC_PSR);
		reg |= 0x1;
		bus_space_write_4(psc->sc_iot, psc->sc_ppch, SAPPC_PSR, reg);
		delay(50000);

		val = sc->sc_contrast;
		config_hook_call(CONFIG_HOOK_SET, CONFIG_HOOK_CONTRAST, &val);
		delay(100000);

		reg = bus_space_read_4(psc->sc_iot, psc->sc_ppch, SAPPC_PSR);
		reg |= 0x4;
		bus_space_write_4(psc->sc_iot, psc->sc_ppch, SAPPC_PSR, reg);

		val = sc->sc_brightness;
		config_hook_call(CONFIG_HOOK_SET, CONFIG_HOOK_BRIGHTNESS, &val);

		reg = bus_space_read_4(psc->sc_iot, psc->sc_ppch, SAPPC_PSR);
		reg |= 0x2;
		bus_space_write_4(psc->sc_iot, psc->sc_ppch, SAPPC_PSR, reg);
	} else {
		reg = bus_space_read_4(psc->sc_iot, psc->sc_ppch, SAPPC_PSR);
		reg &= ~0x2;
		bus_space_write_4(psc->sc_iot, psc->sc_ppch, SAPPC_PSR, reg);
		reg &= ~0x4;
		bus_space_write_4(psc->sc_iot, psc->sc_ppch, SAPPC_PSR, reg);
		delay(100000);

		val = -2;
		config_hook_call(CONFIG_HOOK_SET, CONFIG_HOOK_BRIGHTNESS, &val);

		bus_space_write_1(sc->sc_iot, sc->sc_regh, 0x1f0, 1);

		delay(100000);
		reg = bus_space_read_4(psc->sc_iot, psc->sc_ppch, SAPPC_PSR);
		reg &= ~0x1;
		bus_space_write_4(psc->sc_iot, psc->sc_ppch, SAPPC_PSR, reg);
	}

	return 1;
}
