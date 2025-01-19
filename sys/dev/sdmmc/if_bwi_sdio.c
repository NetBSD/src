/* $NetBSD: if_bwi_sdio.c,v 1.1 2025/01/19 00:29:29 jmcneill Exp $ */

/*-
 * Copyright (c) 2025 Jared McNeill <jmcneill@invisible.ca>
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

__KERNEL_RCSID(0, "$NetBSD: if_bwi_sdio.c,v 1.1 2025/01/19 00:29:29 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/mutex.h>
#include <sys/systm.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#include <netinet/in.h>

#include <net80211/ieee80211_node.h>
#include <net80211/ieee80211_amrr.h>
#include <net80211/ieee80211_radiotap.h>
#include <net80211/ieee80211_var.h>

#include <dev/ic/bwireg.h>
#include <dev/ic/bwivar.h>

#include <dev/pcmcia/pcmciareg.h>

#include <dev/sdmmc/sdmmcdevs.h>
#include <dev/sdmmc/sdmmcvar.h>

#define BWI_SDIO_FUNC1_SBADDRLOW	0x1000a
#define BWI_SDIO_FUNC1_SBADDRMID	0x1000b
#define BWI_SDIO_FUNC1_SBADDRHI		0x1000c

#define BWI_CISTPL_VENDOR		0x80
#define BWI_VENDOR_SROMREV		0
#define BWI_VENDOR_ID			1
#define BWI_VENDOR_BOARDREV		2
#define BWI_VENDOR_PA			3
#define BWI_VENDOR_OEMNAME		4
#define BWI_VENDOR_CCODE		5
#define BWI_VENDOR_ANTENNA		6
#define BWI_VENDOR_ANTGAIN		7
#define BWI_VENDOR_BFLAGS		8
#define BWI_VENDOR_LEDS			9

#define BWI_SDIO_REG_OFFSET(ssc, reg)	\
	((reg) | ((ssc)->sc_sel_regwin & 0x7000))

#define BWI_SDIO_REG_32BIT_ACCESS	0x8000

static const struct bwi_sdio_product {
	uint16_t	vendor;
	uint16_t	product;
} bwi_sdio_products[] = {
	{ SDMMC_VENDOR_BROADCOM, SDMMC_PRODUCT_BROADCOM_NINTENDO_WII },
};

struct bwi_sdio_sprom {
	uint16_t pa_params[3];
	uint16_t board_vendor;
	uint16_t card_flags;
	uint8_t srom_rev;
	uint8_t board_rev;
	uint8_t idle_tssi;
	uint8_t max_txpwr;
	uint8_t country_code;
	uint8_t ant_avail;
	uint8_t ant_gain;
	uint8_t gpio[4];
};

struct bwi_sdio_softc {
	struct bwi_softc sc_base;

	struct sdmmc_function *sc_sf;
	struct bwi_sdio_sprom sc_sprom;
	uint32_t sc_sel_regwin;
	kmutex_t sc_lock;
};

static int bwi_sdio_match(device_t, cfdata_t, void *);
static void bwi_sdio_attach(device_t, device_t, void *);

static void bwi_sdio_parse_cis(struct bwi_sdio_softc *);

static int bwi_sdio_intr(void *);

static void bwi_sdio_conf_write(void *, uint32_t, uint32_t);
static uint32_t bwi_sdio_conf_read(void *, uint32_t);
static void bwi_sdio_reg_write_2(void *, uint32_t, uint16_t);
static uint16_t bwi_sdio_reg_read_2(void *, uint32_t);
static void bwi_sdio_reg_write_4(void *, uint32_t, uint32_t);
static uint32_t bwi_sdio_reg_read_4(void *, uint32_t);
static void bwi_sdio_reg_write_multi_4(void *, uint32_t, const uint32_t *,
    size_t);
static void bwi_sdio_reg_read_multi_4(void *, uint32_t, uint32_t *,
    size_t);

CFATTACH_DECL_NEW(bwi_sdio, sizeof(struct bwi_sdio_softc),
    bwi_sdio_match, bwi_sdio_attach, NULL, NULL);

static int
bwi_sdio_match(device_t parent, cfdata_t cf, void *aux)
{
	struct sdmmc_attach_args * const saa = aux;
	struct sdmmc_function *sf = saa->sf;
	struct sdmmc_cis *cis;
	u_int n;

	if (sf == NULL) {
		return 0;
	}
	cis = &sf->sc->sc_fn0->cis;

	for (n = 0; n < __arraycount(bwi_sdio_products); n++) {
		const struct bwi_sdio_product *bsp = &bwi_sdio_products[n];

		if (bsp->vendor == cis->manufacturer &&
		    bsp->product == cis->product) {
			return 1;
		}
	}

	return 0;
}

static void
bwi_sdio_attach(device_t parent, device_t self, void *aux)
{
	struct bwi_sdio_softc * const ssc = device_private(self);
	struct bwi_softc * const sc = &ssc->sc_base;
	struct sdmmc_attach_args * const saa = aux;
	struct sdmmc_function *sf = saa->sf;
	struct sdmmc_cis *cis = &sf->sc->sc_fn0->cis;
	int error;
	void *ih;

	aprint_naive("\n");
	aprint_normal(": Broadcom Wireless\n");

	sc->sc_dev = self;
	sc->sc_flags = BWI_F_SDIO | BWI_F_PIO;
	sc->sc_conf_write = bwi_sdio_conf_write;
	sc->sc_conf_read = bwi_sdio_conf_read;
	sc->sc_reg_write_multi_4 = bwi_sdio_reg_write_multi_4;
	sc->sc_reg_read_multi_4 = bwi_sdio_reg_read_multi_4;
	sc->sc_reg_write_2 = bwi_sdio_reg_write_2;
	sc->sc_reg_read_2 = bwi_sdio_reg_read_2;
	sc->sc_reg_write_4 = bwi_sdio_reg_write_4;
	sc->sc_reg_read_4 = bwi_sdio_reg_read_4;
	sc->sc_pci_revid = 0;	/* XXX can this come from CIS? */
	sc->sc_pci_did = cis->product;
	sc->sc_pci_subvid = cis->manufacturer;
	sc->sc_pci_subdid = cis->product;

	ssc->sc_sf = sf;
	mutex_init(&ssc->sc_lock, MUTEX_DEFAULT, IPL_NONE);

	sdmmc_io_set_blocklen(ssc->sc_sf, 64);
	if (sdmmc_io_function_enable(ssc->sc_sf) != 0) {
		aprint_error_dev(self, "couldn't enable function\n");
		return;
	}

	bwi_sdio_parse_cis(ssc);

	ih = sdmmc_intr_establish(parent, bwi_sdio_intr, ssc,
	    device_xname(self));
	if (ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt\n");
		return;
	}

	error = bwi_attach(sc);
	if (error != 0) {
		sdmmc_intr_disestablish(ih);
		return;
	}

	sdmmc_intr_enable(ssc->sc_sf);
}

static void
bwi_sdio_parse_cis(struct bwi_sdio_softc *ssc)
{
	struct sdmmc_function *sf0 = ssc->sc_sf->sc->sc_fn0;
	struct bwi_sdio_sprom *sprom = &ssc->sc_sprom;
	uint32_t reg;
	uint8_t tplcode, tpllen;

	reg = sdmmc_cisptr(ssc->sc_sf);
	for (;;) {
		tplcode = sdmmc_io_read_1(sf0, reg++);
		if (tplcode == PCMCIA_CISTPL_NULL) {
			continue;
		}
		tpllen = sdmmc_io_read_1(sf0, reg++);
		if (tplcode == PCMCIA_CISTPL_END || tpllen == 0) {
			break;
		}
		if (tplcode != BWI_CISTPL_VENDOR) {
			reg += tpllen;
			continue;
		}

		switch (sdmmc_io_read_1(sf0, reg)) {
		case BWI_VENDOR_SROMREV:
			sprom->srom_rev = sdmmc_io_read_1(sf0, reg + 1);
			break;
		case BWI_VENDOR_ID:
			sprom->board_vendor =
			    sdmmc_io_read_1(sf0, reg + 1) |
			    ((uint16_t)sdmmc_io_read_1(sf0, reg + 2) << 8);
			break;
		case BWI_VENDOR_BOARDREV:
			sprom->board_rev =
			    sdmmc_io_read_1(sf0, reg + 1);
			break;
		case BWI_VENDOR_PA:
			sprom->pa_params[0] =
			    sdmmc_io_read_1(sf0, reg + 1) |
			    ((uint16_t)sdmmc_io_read_1(sf0, reg + 2) << 8);
			sprom->pa_params[1] =
			    sdmmc_io_read_1(sf0, reg + 3) |
			    ((uint16_t)sdmmc_io_read_1(sf0, reg + 4) << 8);
			sprom->pa_params[2] =
			    sdmmc_io_read_1(sf0, reg + 5) |
			    ((uint16_t)sdmmc_io_read_1(sf0, reg + 6) << 8);
			sprom->idle_tssi =
			    sdmmc_io_read_1(sf0, reg + 7);
			sprom->max_txpwr =
			    sdmmc_io_read_1(sf0, reg + 8);
			break;
		case BWI_VENDOR_CCODE:
			sprom->country_code =
			    sdmmc_io_read_1(sf0, reg + 1);
			break;
		case BWI_VENDOR_ANTGAIN:
			sprom->ant_gain = sdmmc_io_read_1(sf0, reg + 1);
			break;
		case BWI_VENDOR_BFLAGS:
			sprom->card_flags =
			    sdmmc_io_read_1(sf0, reg + 1) |
			    ((uint16_t)sdmmc_io_read_1(sf0, reg + 2) << 8);
			break;
		case BWI_VENDOR_LEDS:
			sprom->gpio[0] = sdmmc_io_read_1(sf0, reg + 1);
			sprom->gpio[1] = sdmmc_io_read_1(sf0, reg + 2);
			sprom->gpio[2] = sdmmc_io_read_1(sf0, reg + 3);
			sprom->gpio[3] = sdmmc_io_read_1(sf0, reg + 4);
			break;
		}

		reg += tpllen;
	}
}

static int
bwi_sdio_intr(void *priv)
{
	struct bwi_sdio_softc * const ssc = priv;

	bwi_intr(&ssc->sc_base);

	return 1;
}

static void
bwi_sdio_conf_write(void *priv, uint32_t reg, uint32_t val)
{
	struct bwi_sdio_softc * const ssc = priv;

	KASSERT(reg == BWI_PCIR_SEL_REGWIN);

	mutex_enter(&ssc->sc_lock);
	if (reg == BWI_PCIR_SEL_REGWIN && ssc->sc_sel_regwin != val) {
		sdmmc_io_write_1(ssc->sc_sf, BWI_SDIO_FUNC1_SBADDRLOW,
		    (val >> 8) & 0x80);
		sdmmc_io_write_1(ssc->sc_sf, BWI_SDIO_FUNC1_SBADDRMID,
		    (val >> 16) & 0xff);
		sdmmc_io_write_1(ssc->sc_sf, BWI_SDIO_FUNC1_SBADDRHI,
		    (val >> 24) & 0xff);
		ssc->sc_sel_regwin = val;
	}
	mutex_exit(&ssc->sc_lock);
}

static uint32_t
bwi_sdio_conf_read(void *priv, uint32_t reg)
{
	struct bwi_sdio_softc * const ssc = priv;

	KASSERT(reg == BWI_PCIR_SEL_REGWIN);

	if (reg == BWI_PCIR_SEL_REGWIN) {
		return ssc->sc_sel_regwin;
	} else {
		return 0;
	}
}

static void
bwi_sdio_reg_write_multi_4(void *priv, uint32_t reg, const uint32_t *datap,
    size_t count)
{
	struct bwi_sdio_softc * const ssc = priv;

	mutex_enter(&ssc->sc_lock);
	sdmmc_io_write_multi_1(ssc->sc_sf,
	    BWI_SDIO_REG_OFFSET(ssc, reg) | BWI_SDIO_REG_32BIT_ACCESS,
	    (uint8_t *)__UNCONST(datap), count * sizeof(uint32_t)); 
	mutex_exit(&ssc->sc_lock);
}

static void
bwi_sdio_reg_read_multi_4(void *priv, uint32_t reg, uint32_t *datap,
    size_t count)
{
	struct bwi_sdio_softc * const ssc = priv;

	mutex_enter(&ssc->sc_lock);
	sdmmc_io_read_multi_1(ssc->sc_sf,
	    BWI_SDIO_REG_OFFSET(ssc, reg) | BWI_SDIO_REG_32BIT_ACCESS,
	    (uint8_t *)datap, count * sizeof(uint32_t)); 
	mutex_exit(&ssc->sc_lock);
}

static void
bwi_sdio_reg_write_2(void *priv, uint32_t reg, uint16_t val)
{
	struct bwi_sdio_softc * const ssc = priv;

	val = htole16(val);

	mutex_enter(&ssc->sc_lock);
	sdmmc_io_write_2(ssc->sc_sf, BWI_SDIO_REG_OFFSET(ssc, reg), val);
	mutex_exit(&ssc->sc_lock);
}

static uint16_t
bwi_sdio_reg_read_sprom(struct bwi_sdio_softc *ssc, uint32_t reg)
{
	struct bwi_sdio_sprom *sprom = &ssc->sc_sprom;
	struct sdmmc_cis *cis = &ssc->sc_sf->cis;

	switch (reg) {
	case BWI_SPROM_11BG_EADDR ... BWI_SPROM_11BG_EADDR + 4:
		return *(uint16_t *)&cis->lan_nid[reg - BWI_SPROM_11BG_EADDR];
	case BWI_SPROM_11A_EADDR ... BWI_SPROM_11A_EADDR + 4:
		return *(uint16_t *)&cis->lan_nid[reg - BWI_SPROM_11A_EADDR];
	case BWI_SPROM_CARD_INFO:
		return (uint16_t)sprom->country_code << 8;
	case BWI_SPROM_PA_PARAM_11BG ... BWI_SPROM_PA_PARAM_11BG + 4:
		return sprom->pa_params[(reg - BWI_SPROM_PA_PARAM_11BG) / 2];
	case BWI_SPROM_PA_PARAM_11A ... BWI_SPROM_PA_PARAM_11A + 4:
		return sprom->pa_params[(reg - BWI_SPROM_PA_PARAM_11A) / 2];
	case BWI_SPROM_GPIO01:
		return sprom->gpio[0] | ((uint16_t)sprom->gpio[1] << 8);
	case BWI_SPROM_GPIO23:
		return sprom->gpio[2] | ((uint16_t)sprom->gpio[3] << 8);
	case BWI_SPROM_MAX_TXPWR:
		return sprom->max_txpwr | ((uint16_t)sprom->max_txpwr << 8);
	case BWI_SPROM_IDLE_TSSI:
		return sprom->idle_tssi | ((uint16_t)sprom->idle_tssi << 8);
	case BWI_SPROM_CARD_FLAGS:
		return sprom->card_flags;
	case BWI_SPROM_ANT_GAIN:
		return sprom->ant_gain | ((uint16_t)sprom->ant_gain << 8);
	default:
		return 0xffff;
	}
}

static uint16_t
bwi_sdio_reg_read_2(void *priv, uint32_t reg)
{
	struct bwi_sdio_softc * const ssc = priv;
	uint16_t val;

	/* Emulate SPROM reads */
	if (reg >= BWI_SPROM_START &&
	    reg <= BWI_SPROM_START + BWI_SPROM_ANT_GAIN) {
		return bwi_sdio_reg_read_sprom(ssc, reg - BWI_SPROM_START);
	}

	mutex_enter(&ssc->sc_lock);
	val = sdmmc_io_read_2(ssc->sc_sf, BWI_SDIO_REG_OFFSET(ssc, reg));
	mutex_exit(&ssc->sc_lock);

	val = le16toh(val);

	return val;
}

static void
bwi_sdio_reg_write_4(void *priv, uint32_t reg, uint32_t val)
{
	struct bwi_sdio_softc * const ssc = priv;

	val = htole32(val);

	mutex_enter(&ssc->sc_lock);
	sdmmc_io_write_4(ssc->sc_sf,
	    BWI_SDIO_REG_OFFSET(ssc, reg) | BWI_SDIO_REG_32BIT_ACCESS, val);
	/* SDIO cards require a read after a 32-bit write */
	sdmmc_io_read_4(ssc->sc_sf, 0);
	mutex_exit(&ssc->sc_lock);
}

static uint32_t
bwi_sdio_reg_read_4(void *priv, uint32_t reg)
{
	struct bwi_sdio_softc * const ssc = priv;
	uint32_t val;

	mutex_enter(&ssc->sc_lock);
	val = sdmmc_io_read_4(ssc->sc_sf,
	    BWI_SDIO_REG_OFFSET(ssc, reg) | BWI_SDIO_REG_32BIT_ACCESS);
	mutex_exit(&ssc->sc_lock);

	val = le32toh(val);

	return val;
}
