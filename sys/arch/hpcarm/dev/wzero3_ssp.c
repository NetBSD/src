/*	$NetBSD: wzero3_ssp.c,v 1.1 2010/05/09 10:40:00 nonaka Exp $	*/

/*
 * Copyright (c) 2010 NONAKA Kimihiro <nonaka@netbsd.org>
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: wzero3_ssp.c,v 1.1 2010/05/09 10:40:00 nonaka Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/mutex.h>
#include <sys/pmf.h>
#include <sys/bus.h>

#include <machine/bootinfo.h>
#include <machine/platid.h>
#include <machine/platid_mask.h>

#include <arm/xscale/pxa2x0reg.h>
#include <arm/xscale/pxa2x0var.h>
#include <arm/xscale/pxa2x0_gpio.h>

#include <hpcarm/dev/wzero3_reg.h>
#include <hpcarm/dev/wzero3_sspvar.h>

#define WS007SH_SSCR0_ADS7846	0x06ab	/* 12bit/Microwire/div by 7 */

struct wzero3ssp_softc {
	device_t sc_dev;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	kmutex_t sc_mtx;
};

static int	wzero3ssp_match(device_t, cfdata_t, void *);
static void	wzero3ssp_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(wzero3ssp, sizeof(struct wzero3ssp_softc),
	wzero3ssp_match, wzero3ssp_attach, NULL, NULL);

static void	wzero3ssp_init(struct wzero3ssp_softc *);
static bool	wzero3ssp_resume(device_t dv, const pmf_qual_t *);
static uint32_t	wzero3ssp_read_ads7846(uint32_t);

static struct wzero3ssp_softc *wzero3ssp_sc;

static const struct wzero3ssp_model {
	platid_mask_t *platid;
} wzero3ssp_table[] = {
#if 0
	/* WS003SH */
	{
		&platid_mask_MACH_SHARP_WZERO3_WS003SH,
	},
	/* WS004SH */
	{
		&platid_mask_MACH_SHARP_WZERO3_WS004SH,
	},
#endif
	/* WS007SH */
	{
		&platid_mask_MACH_SHARP_WZERO3_WS007SH,
	},
#if 0
	/* WS011SH */
	{
		&platid_mask_MACH_SHARP_WZERO3_WS011SH,
	},
	/* WS0020H */
	{
		&platid_mask_MACH_SHARP_WZERO3_WS020SH,
	},
#endif
	{
		NULL,
	},
};

static const struct wzero3ssp_model *
wzero3ssp_lookup(void)
{
	const struct wzero3ssp_model *model;

	for (model = wzero3ssp_table; model->platid != NULL; model++) {
		if (platid_match(&platid, model->platid)) {
			return model;
		}
	}
	return NULL;
}

static int
wzero3ssp_match(device_t parent, cfdata_t cf, void *aux)
{

	if (strcmp(cf->cf_name, "wzero3ssp") != 0)
		return 0;
	if (wzero3ssp_lookup() == NULL)
		return 0;
	if (wzero3ssp_sc != NULL)
		return 0;
	return 1;
}

static void
wzero3ssp_attach(device_t parent, device_t self, void *aux)
{
	struct wzero3ssp_softc *sc = device_private(self);

	sc->sc_dev = self;
	wzero3ssp_sc = sc;

	aprint_normal("\n");
	aprint_naive("\n");

	mutex_init(&sc->sc_mtx, MUTEX_DEFAULT, IPL_TTY);

	sc->sc_iot = &pxa2x0_bs_tag;
	if (bus_space_map(sc->sc_iot, PXA2X0_SSP1_BASE, PXA2X0_SSP_SIZE, 0,
	     &sc->sc_ioh)) {
		aprint_error_dev(sc->sc_dev, "can't map bus space\n");
		return;
	}

	if (!pmf_device_register(sc->sc_dev, NULL, wzero3ssp_resume))
		aprint_error_dev(sc->sc_dev,
		    "couldn't establish power handler\n");

	wzero3ssp_init(sc);
}

/*
 * Initialize the dedicated SSP unit and disable all chip selects.
 * This function is called with interrupts disabled.
 */
static void
wzero3ssp_init(struct wzero3ssp_softc *sc)
{

	pxa2x0_clkman_config(CKEN_SSP, 1);

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SSP_SSCR0, 0);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SSP_SSCR1, 0);

	pxa2x0_gpio_set_function(GPIO_WS007SH_ADS7846_CS, GPIO_OUT|GPIO_SET);
}

static bool
wzero3ssp_resume(device_t dv, const pmf_qual_t *qual)
{
	struct wzero3ssp_softc *sc = device_private(dv);

	mutex_enter(&sc->sc_mtx);
	wzero3ssp_init(sc);
	mutex_exit(&sc->sc_mtx);

	return true;
}

/*
 * Transmit a single data word to one of the ICs, keep the chip selected
 * afterwards, and don't wait for data to be returned in SSDR.  Interrupts
 * must be held off until wzero3ssp_ic_stop() gets called.
 */
void
wzero3ssp_ic_start(int ic, uint32_t cmd)
{
	struct wzero3ssp_softc *sc;

	KASSERT(wzero3ssp_sc != NULL);
	sc = wzero3ssp_sc;

	mutex_enter(&sc->sc_mtx);

	/* disable other ICs */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SSP_SSCR0, 0);
	if (ic != WZERO3_SSP_IC_ADS7846)
		pxa2x0_gpio_set_bit(GPIO_WS007SH_ADS7846_CS);

	/* activate the chosen one */
	switch (ic) {
	case WZERO3_SSP_IC_ADS7846:
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, SSP_SSCR0, 0);
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, SSP_SSCR0,
		    WS007SH_SSCR0_ADS7846);
		pxa2x0_gpio_clear_bit(GPIO_WS007SH_ADS7846_CS);
		bus_space_write_1(sc->sc_iot, sc->sc_ioh, SSP_SSDR, cmd);
		while ((bus_space_read_4(sc->sc_iot, sc->sc_ioh, SSP_SSSR)
		    & SSSR_TNF) != SSSR_TNF)
			continue;	/* poll */
		break;
	}
}

/*
 * Read the last value from SSDR and deactivate all chip-selects.
 */
uint32_t
wzero3ssp_ic_stop(int ic)
{
	struct wzero3ssp_softc *sc;
	uint32_t rv;

	KASSERT(wzero3ssp_sc != NULL);
	sc = wzero3ssp_sc;

	switch (ic) {
	case WZERO3_SSP_IC_ADS7846:
		/* read result of last command */
		while ((bus_space_read_4(sc->sc_iot, sc->sc_ioh, SSP_SSSR)
		    & SSSR_RNE) != SSSR_RNE)
			continue;	/* poll */
		rv = bus_space_read_4(sc->sc_iot, sc->sc_ioh, SSP_SSDR);
		break;
	default:
		rv = 0;
		break;
	}

	pxa2x0_gpio_set_bit(GPIO_WS007SH_ADS7846_CS);

	mutex_exit(&sc->sc_mtx);

	return rv;
}

/*
 * Activate one of the chip-select lines, transmit one word value in
 * each direction, and deactivate the chip-select again.
 */
uint32_t
wzero3ssp_ic_send(int ic, uint32_t data)
{

	switch (ic) {
	case WZERO3_SSP_IC_ADS7846:
		return wzero3ssp_read_ads7846(data);
	default:
		aprint_error("wzero3ssp: wzero3ssp_ic_send: invalid IC %d\n", ic);
		return 0;
	}
}

static uint32_t
wzero3ssp_read_ads7846(uint32_t cmd)
{
	struct wzero3ssp_softc *sc;
	uint32_t rv;

	if (wzero3ssp_sc == NULL) {
		printf("wzero3ssp_read_ads7846: not configured\n");
		return 0;
	}
	sc = wzero3ssp_sc;

	mutex_enter(&sc->sc_mtx);

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SSP_SSCR0, 0);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SSP_SSCR0,
	    WS007SH_SSCR0_ADS7846);

	pxa2x0_gpio_clear_bit(GPIO_WS007SH_ADS7846_CS);

	/* send cmd */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SSP_SSDR, cmd);
	while ((bus_space_read_4(sc->sc_iot, sc->sc_ioh, SSP_SSSR)
	    & SSSR_TNF) != SSSR_TNF)
		continue;	/* poll */
	delay(1);
	while ((bus_space_read_4(sc->sc_iot, sc->sc_ioh, SSP_SSSR)
	    & SSSR_RNE) != SSSR_RNE)
		continue;	/* poll */
	rv = bus_space_read_4(sc->sc_iot, sc->sc_ioh, SSP_SSDR);

	pxa2x0_gpio_set_bit(GPIO_WS007SH_ADS7846_CS);

	mutex_exit(&sc->sc_mtx);

	return rv;
}
