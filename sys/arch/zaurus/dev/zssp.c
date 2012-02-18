/*	$NetBSD: zssp.c,v 1.11.6.1 2012/02/18 07:33:51 mrg Exp $	*/
/*	$OpenBSD: zaurus_ssp.c,v 1.6 2005/04/08 21:58:49 uwe Exp $	*/

/*
 * Copyright (c) 2005 Uwe Stuehler <uwe@bsdx.de>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: zssp.c,v 1.11.6.1 2012/02/18 07:33:51 mrg Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/bus.h>

#include <arm/xscale/pxa2x0reg.h>
#include <arm/xscale/pxa2x0var.h>
#include <arm/xscale/pxa2x0_gpio.h>

#include <zaurus/dev/zsspvar.h>
#include <zaurus/zaurus/zaurus_var.h>

#define GPIO_ADS7846_CS_C3000	14	/* SSP SFRM */
#define GPIO_MAX1111_CS_C3000	20
#define GPIO_TG_CS_C3000	53
#define GPIO_ADS7846_CS_C860	24	/* SSP SFRM */
#define GPIO_MAX1111_CS_C860	20
#define GPIO_TG_CS_C860		19

#define SSCR0_ADS7846_C3000	0x06ab /* 12bit/Microwire/div by 7 */
#define SSCR0_MAX1111		0x0387
#define	SSCR0_LZ9JG18		0x01ab
#define SSCR0_ADS7846_C860	0x00ab /* 12bit/Microwire/div by 7 */

struct zssp_ads7846 {
	u_int gpio;
	uint32_t sscr0;
};
struct zssp_max1111 {
	u_int gpio;
	uint32_t sscr0;
};
struct zssp_lz9jg18 {
	u_int gpio;
	uint32_t sscr0;
	int sclk_pin;
	int sfrm_pin;
	int txd_pin;
	int rxd_pin;
};

struct zssp_softc {
	device_t sc_dev;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	bus_addr_t sc_ssp;
	struct zssp_ads7846 ads7846;
	struct zssp_max1111 max1111;
	struct zssp_lz9jg18 lz9jg18;
};

static int	zssp_match(device_t, cfdata_t, void *);
static void	zssp_attach(device_t, device_t, void *);
static int	zssp_search(device_t, cfdata_t, const int *, void *);
static int	zssp_print(void *, const char *);

CFATTACH_DECL_NEW(zssp, sizeof(struct zssp_softc),
	zssp_match, zssp_attach, NULL, NULL);

static void	zssp_init(void);
static bool	zssp_resume(device_t dv, const pmf_qual_t *);

static struct zssp_softc *zssp_sc;

static int
zssp_match(device_t parent, cfdata_t cf, void *aux)
{

	if (zssp_sc != NULL)
		return 0;
	return 1;
}

static void
zssp_attach(device_t parent, device_t self, void *aux)
{
	struct zssp_softc *sc = device_private(self);

	sc->sc_dev = self;
	zssp_sc = sc;

	aprint_normal("\n");
	aprint_naive("\n");

	sc->sc_iot = &pxa2x0_bs_tag;
	if (ZAURUS_ISC1000 || ZAURUS_ISC3000) {
		sc->sc_ssp = PXA2X0_SSP1_BASE;
		sc->ads7846.gpio     = GPIO_ADS7846_CS_C3000;
		sc->ads7846.sscr0    = SSCR0_ADS7846_C3000;
		sc->max1111.gpio     = GPIO_MAX1111_CS_C3000;
		sc->max1111.sscr0    = SSCR0_MAX1111;
		sc->lz9jg18.gpio     = GPIO_TG_CS_C3000;
		sc->lz9jg18.sscr0    = SSCR0_LZ9JG18;
		sc->lz9jg18.sclk_pin = 19;
		sc->lz9jg18.sfrm_pin = 14;
		sc->lz9jg18.txd_pin  = 87;
		sc->lz9jg18.rxd_pin  = 86;
	} else {
		sc->sc_ssp = PXA2X0_SSP_BASE;
		sc->ads7846.gpio     = GPIO_ADS7846_CS_C860;
		sc->ads7846.sscr0    = SSCR0_ADS7846_C860;
		sc->max1111.gpio     = GPIO_MAX1111_CS_C860;
		sc->max1111.sscr0    = SSCR0_MAX1111;
		sc->lz9jg18.gpio     = GPIO_TG_CS_C860;
		sc->lz9jg18.sscr0    = SSCR0_LZ9JG18;
		sc->lz9jg18.sclk_pin = 23;
		sc->lz9jg18.sfrm_pin = 24;
		sc->lz9jg18.txd_pin  = 25;
		sc->lz9jg18.rxd_pin  = 26;
	}

	if (bus_space_map(sc->sc_iot, sc->sc_ssp, PXA2X0_SSP_SIZE,
	    0, &sc->sc_ioh)) {
		aprint_error_dev(sc->sc_dev, "can't map bus space\n");
		return;
	}

	if (!pmf_device_register(sc->sc_dev, NULL, zssp_resume))
		aprint_error_dev(sc->sc_dev,
		    "couldn't establish power handler\n");

	zssp_init();

	/* Attach all devices */
	config_search_ia(zssp_search, self, "zssp", sc);
}

static int
zssp_search(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{
	struct zssp_attach_args aa;

	aa.zaa_name = cf->cf_name;

	if (config_match(parent, cf, &aa))
		config_attach(parent, cf, &aa, zssp_print);

	return 0;
}

static int
zssp_print(void *aux, const char *name)
{

	return UNCONF;
}

/*
 * Initialize the dedicated SSP unit and disable all chip selects.
 * This function is called with interrupts disabled.
 */
static void
zssp_init(void)
{
	struct zssp_softc *sc;

	if (__predict_false(zssp_sc == NULL)) {
		aprint_error("%s: not configured.\n", __func__);
		return;
	}
	sc = zssp_sc;

	pxa2x0_clkman_config(CKEN_SSP, 1);

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SSP_SSCR0, sc->lz9jg18.sscr0);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SSP_SSCR1, 0);

	pxa2x0_gpio_set_function(sc->ads7846.gpio, GPIO_OUT|GPIO_SET);
	pxa2x0_gpio_set_function(sc->max1111.gpio, GPIO_OUT|GPIO_SET);
	pxa2x0_gpio_set_function(sc->lz9jg18.gpio, GPIO_OUT|GPIO_SET);
}

static bool
zssp_resume(device_t dv, const pmf_qual_t *qual)
{
	int s;

	s = splhigh();
	zssp_init();
	splx(s);

	return true;
}

/*
 * Transmit a single data word to one of the ICs, keep the chip selected
 * afterwards, and don't wait for data to be returned in SSDR.  Interrupts
 * must be held off until zssp_ic_stop() gets called.
 */
void
zssp_ic_start(int ic, uint32_t data)
{
	struct zssp_softc *sc;

	if (__predict_false(zssp_sc == NULL)) {
		aprint_error("%s: not configured.\n", __func__);
		return;
	}
	sc = zssp_sc;

	/* disable other ICs */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SSP_SSCR0, 0);
	if (ic != ZSSP_IC_ADS7846)
		pxa2x0_gpio_set_bit(sc->ads7846.gpio);
	if (ic != ZSSP_IC_LZ9JG18)
		pxa2x0_gpio_set_bit(sc->lz9jg18.gpio);
	if (ic != ZSSP_IC_MAX1111)
		pxa2x0_gpio_set_bit(sc->max1111.gpio);

	/* activate the chosen one */
	switch (ic) {
	case ZSSP_IC_ADS7846:
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, SSP_SSCR0,
		    sc->ads7846.sscr0);
		pxa2x0_gpio_clear_bit(sc->ads7846.gpio);
		delay(1);	/* ADS7846 Tcss = 100ns */
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, SSP_SSDR, data);
		while ((bus_space_read_4(sc->sc_iot, sc->sc_ioh, SSP_SSSR)
		    & SSSR_TNF) != SSSR_TNF)
			continue;	/* poll */
		break;
	case ZSSP_IC_LZ9JG18:
		pxa2x0_gpio_clear_bit(sc->lz9jg18.gpio);
		break;
	case ZSSP_IC_MAX1111:
		pxa2x0_gpio_clear_bit(sc->max1111.gpio);
		break;
	}
}

/*
 * Read the last value from SSDR and deactivate all chip-selects.
 */
uint32_t
zssp_ic_stop(int ic)
{
	struct zssp_softc *sc;
	uint32_t rv;

	if (__predict_false(zssp_sc == NULL)) {
		aprint_error("%s: not configured.\n", __func__);
		return 0;
	}
	sc = zssp_sc;

	switch (ic) {
	case ZSSP_IC_ADS7846:
		/* read result of last command */
		while ((bus_space_read_4(sc->sc_iot, sc->sc_ioh, SSP_SSSR)
		    & SSSR_RNE) != SSSR_RNE)
			continue;	/* poll */
		rv = bus_space_read_4(sc->sc_iot, sc->sc_ioh, SSP_SSDR);
		break;
	case ZSSP_IC_LZ9JG18:
	case ZSSP_IC_MAX1111:
		/* last value received is irrelevant or undefined */
	default:
		rv = 0;
		break;
	}

	pxa2x0_gpio_set_bit(sc->ads7846.gpio);
	pxa2x0_gpio_set_bit(sc->lz9jg18.gpio);
	pxa2x0_gpio_set_bit(sc->max1111.gpio);

	return rv;
}

/*
 * Activate one of the chip-select lines, transmit one word value in
 * each direction, and deactivate the chip-select again.
 */
uint32_t
zssp_ic_send(int ic, uint32_t data)
{

	switch (ic) {
	case ZSSP_IC_MAX1111:
		return (zssp_read_max1111(data));
	case ZSSP_IC_ADS7846:
		return (zssp_read_ads7846(data));
	case ZSSP_IC_LZ9JG18:
		zssp_write_lz9jg18(data);
		return 0;
	default:
		aprint_error("zssp: zssp_ic_send: invalid IC %d\n", ic);
		return 0;
	}
}

int
zssp_read_max1111(uint32_t cmd)
{
	struct zssp_softc *sc;
	int data[3];
	int voltage[3];	/* voltage[0]: dummy */
	int i;
	int s;

	if (__predict_false(zssp_sc == NULL)) {
		aprint_error("%s: not configured.\n", __func__);
		return 0;
	}
	sc = zssp_sc;

	s = splhigh();

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SSP_SSCR0, 0);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SSP_SSCR0, sc->max1111.sscr0);

	pxa2x0_gpio_set_bit(sc->lz9jg18.gpio);
	pxa2x0_gpio_set_bit(sc->ads7846.gpio);
	pxa2x0_gpio_clear_bit(sc->max1111.gpio);

	delay(1);

	memset(data, 0, sizeof(data));
	data[0] = cmd;
	for (i = 0; i < __arraycount(data); i++) {
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, SSP_SSDR, data[i]);
		while ((bus_space_read_4(sc->sc_iot, sc->sc_ioh, SSP_SSSR)
		    & SSSR_TNF) != SSSR_TNF)
			continue;	/* poll */
		/* XXX is this delay necessary? */
		delay(1);
		while ((bus_space_read_4(sc->sc_iot, sc->sc_ioh, SSP_SSSR)
		    & SSSR_RNE) != SSSR_RNE)
			continue;	/* poll */
		voltage[i] = bus_space_read_4(sc->sc_iot, sc->sc_ioh,
		    SSP_SSDR);
	}

	pxa2x0_gpio_set_bit(sc->lz9jg18.gpio);
	pxa2x0_gpio_set_bit(sc->ads7846.gpio);
	pxa2x0_gpio_set_bit(sc->max1111.gpio);

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SSP_SSCR0, 0);

	splx(s);

	/* XXX no idea what this means, but it's what Linux would do. */
	if ((voltage[1] & 0xc0) != 0 || (voltage[2] & 0x3f) != 0)
		return -1;
	return ((voltage[1] << 2) & 0xfc) | ((voltage[2] >> 6) & 0x03);
}

/* XXX - only does CS_ADS7846 */
uint32_t
zssp_read_ads7846(uint32_t cmd)
{
	struct zssp_softc *sc;
	uint32_t val;
	int s;

	if (__predict_false(zssp_sc == NULL)) {
		aprint_error("%s: not configured\n", __func__);
		return 0;
	}
	sc = zssp_sc;

	s = splhigh();

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SSP_SSCR0, 0);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SSP_SSCR0, sc->ads7846.sscr0);

	pxa2x0_gpio_set_bit(sc->lz9jg18.gpio);
	pxa2x0_gpio_set_bit(sc->max1111.gpio);
	pxa2x0_gpio_clear_bit(sc->ads7846.gpio);
	delay(1);	/* ADS7846 Tcss = 100ns */

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SSP_SSDR, cmd);

	while ((bus_space_read_4(sc->sc_iot, sc->sc_ioh, SSP_SSSR)
	    & SSSR_TNF) != SSSR_TNF)
		continue;	/* poll */

	delay(1);

	while ((bus_space_read_4(sc->sc_iot, sc->sc_ioh, SSP_SSSR)
	    & SSSR_RNE) != SSSR_RNE)
		continue;	/* poll */

	val = bus_space_read_4(sc->sc_iot, sc->sc_ioh, SSP_SSDR);

	pxa2x0_gpio_set_bit(sc->ads7846.gpio);

	splx(s);

	return val;
}

void
zssp_write_lz9jg18(uint32_t data)
{
	struct zssp_softc *sc;
	int sclk_fn;
	int sfrm_fn;
	int txd_fn;
	int rxd_fn;
	int i;
	int s;

	KASSERT(zssp_sc != NULL);
	sc = zssp_sc;

	/* XXX this creates a DAC command from a backlight duty value. */
	data = 0x40 | (data & 0x1f);

	s = splhigh();

	sclk_fn = pxa2x0_gpio_get_function(sc->lz9jg18.sclk_pin);
	sfrm_fn = pxa2x0_gpio_get_function(sc->lz9jg18.sfrm_pin);
	txd_fn = pxa2x0_gpio_get_function(sc->lz9jg18.txd_pin);
	rxd_fn = pxa2x0_gpio_get_function(sc->lz9jg18.rxd_pin);

	pxa2x0_gpio_set_function(sc->lz9jg18.sfrm_pin, GPIO_OUT | GPIO_SET);
	pxa2x0_gpio_set_function(sc->lz9jg18.sclk_pin, GPIO_OUT | GPIO_CLR);
	pxa2x0_gpio_set_function(sc->lz9jg18.txd_pin, GPIO_OUT | GPIO_CLR);
	pxa2x0_gpio_set_function(sc->lz9jg18.rxd_pin, GPIO_IN);

	pxa2x0_gpio_set_bit(sc->max1111.gpio);
	pxa2x0_gpio_set_bit(sc->ads7846.gpio);
	pxa2x0_gpio_clear_bit(sc->lz9jg18.gpio);

	delay(10);
	
	for (i = 0; i < 8; i++) {
		if (data & 0x80)
			pxa2x0_gpio_set_bit(sc->lz9jg18.txd_pin);
		else
			pxa2x0_gpio_clear_bit(sc->lz9jg18.txd_pin);
		delay(10);
		pxa2x0_gpio_set_bit(sc->lz9jg18.sclk_pin);
		delay(10);
		pxa2x0_gpio_clear_bit(sc->lz9jg18.sclk_pin);
		delay(10);
		data <<= 1;
	}

	pxa2x0_gpio_clear_bit(sc->lz9jg18.txd_pin);
	pxa2x0_gpio_set_bit(sc->lz9jg18.gpio);

	pxa2x0_gpio_set_function(sc->lz9jg18.sclk_pin, sclk_fn);
	pxa2x0_gpio_set_function(sc->lz9jg18.sfrm_pin, sfrm_fn);
	pxa2x0_gpio_set_function(sc->lz9jg18.txd_pin, txd_fn);
	pxa2x0_gpio_set_function(sc->lz9jg18.rxd_pin, rxd_fn);

	splx(s);
}
