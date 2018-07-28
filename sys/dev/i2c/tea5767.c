/*	$NetBSD: tea5767.c,v 1.1.2.2 2018/07/28 04:37:44 pgoyette Exp $	*/
/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Karuna Grewal.
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
__KERNEL_RCSID(0, "$NetBSD: tea5767.c,v 1.1.2.2 2018/07/28 04:37:44 pgoyette Exp $");

#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/param.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/systm.h>

#include <dev/i2c/i2cvar.h>

#include <sys/ioctl.h>
#include <sys/radioio.h>
#include <dev/radio_if.h>

#include "tea5767reg.h"

struct tea5767_tune {
	int mute;
	uint32_t freq;
	int stereo;
	bool is_pll_set;
	bool is_xtal_set;
	bool is_force_srch;
	int fm_band;		/* Set = JAPAN. */
	int lock;
	int adc_stop_level;
};

struct tea5767_softc {
	device_t sc_dev;
	/* Tunable properties. */
	struct tea5767_tune tune;
	/* I2C bus controller. */
	i2c_tag_t sc_i2c_tag;
	/* Device addr on I2C. */
	i2c_addr_t sc_addr;
	/* Device capabilities. */
	uint32_t caps;
};

static int tea5767_get_info(void *, struct radio_info *);
static int tea5767_set_info(void *, struct radio_info *);
static int tea5767_search(void *, int);

static int tea5767_match(device_t, cfdata_t, void *);
static void tea5767_attach(device_t, device_t, void *);

static const struct radio_hw_if tea5767_hw_if = {
	NULL,
	NULL,
	tea5767_get_info,
	tea5767_set_info,
	tea5767_search
};

static const uint32_t tea5767_common_caps =
	RADIO_CAPS_DETECT_STEREO |
	RADIO_CAPS_SET_MONO |
	RADIO_CAPS_HW_SEARCH |
	RADIO_CAPS_LOCK_SENSITIVITY;

CFATTACH_DECL_NEW(tea5767radio, sizeof(struct tea5767_softc), tea5767_match,
	tea5767_attach, NULL, NULL);

static int
tea5767_match(device_t parent, cfdata_t cf, void *aux)
{
	struct i2c_attach_args *ia = aux;

	if (ia->ia_addr == TEA5767_ADDR)
		return I2C_MATCH_ADDRESS_ONLY;

	return 0;
}

static void
tea5767_attach(device_t parent, device_t self, void *aux)
{
	struct tea5767_softc *sc = device_private(self);
	int tea5767_flags = device_cfdata(self)->cf_flags;
	struct i2c_attach_args *ia = aux;

	aprint_normal(": Philips/NXP TEA5767 Radio\n");
	aprint_naive(": FM radio\n");

	sc->sc_dev = self;
	sc->tune.mute = 0;
	sc->tune.freq = MIN_FM_FREQ;
	sc->tune.stereo = 1;
	sc->tune.is_pll_set = false;
	sc->tune.is_xtal_set = false;
	sc->tune.is_force_srch = false;
	sc->sc_i2c_tag = ia->ia_tag;
	sc->sc_addr = ia->ia_addr;

	if (tea5767_flags & TEA5767_PLL_FLAG)
		sc->tune.is_pll_set = true;
	if (tea5767_flags & TEA5767_XTAL_FLAG)
		sc->tune.is_xtal_set = true;
	if (tea5767_flags & TEA5767_JAPAN_FM_FLAG)
		sc->tune.fm_band = 1;
	if (tea5767_flags & TEA5767_FORCE_SRCH_FLAG)
		sc->tune.is_force_srch = true;

	sc->caps = tea5767_common_caps;

	/* 
	 * Check the quality of crystal and disable hardware search if
	 * necessary.
	 */
	if (!tea5767_if_check(sc) && !(sc->tune.is_force_srch))
		sc->caps &= ~RADIO_CAPS_HW_SEARCH;

	radio_attach_mi(&tea5767_hw_if, sc, self);
}

static int
tea5767_write(struct tea5767_softc *sc, uint8_t *reg)
{
	int exec_result;

	if (iic_acquire_bus(sc->sc_i2c_tag, I2C_F_POLL)) {
		device_printf(sc->sc_dev, "bus acquiration failed.\n");
		return 1;
	}

	exec_result = iic_exec(sc->sc_i2c_tag, I2C_OP_WRITE_WITH_STOP,
	    sc->sc_addr, NULL, 0, reg, 5, I2C_F_POLL);

	if (exec_result) {
		iic_release_bus(sc->sc_i2c_tag, I2C_F_POLL);
		device_printf(sc->sc_dev, "write operation failed %d.\n",
		    exec_result);
		return 1;
	}

	iic_release_bus(sc->sc_i2c_tag, I2C_F_POLL);

	return 0;
}


static int
tea5767_read(struct tea5767_softc *sc, uint8_t *reg)
{
	int exec_result;

	if (iic_acquire_bus(sc->sc_i2c_tag, I2C_F_POLL)) {
		device_printf(sc->sc_dev, "bus acquiration failed.\n");
		return 1;
	}

	iic_exec(sc->sc_i2c_tag, I2C_OP_READ_WITH_STOP, sc->sc_addr,
	    NULL, 0, reg, 5, I2C_F_POLL);

	if (exec_result) {
		iic_release_bus(sc->sc_i2c_tag, I2C_F_POLL);
		device_printf(sc->sc_dev, "read operation failed.\n");
		return 1;
	}

	iic_release_bus(sc->sc_i2c_tag, I2C_F_POLL);
	return 0;
}

/*
 * Calculate the PLL word.
 * TODO: Extend this calculation to do high side injection as well
 * (add fields to tea5767_tune).
 */
static void
tea5767_freq_to_pll(struct tea5767_tune tune, uint8_t *buf)
{
	uint16_t pll_word = 0;

	if (!tune.is_pll_set && !tune.is_xtal_set)
		pll_word = 4 * (tune.freq  - 225) * 1000 / 50000;
	else if (!tune.is_pll_set && tune.is_xtal_set) {
		pll_word = 4 * (tune.freq - 225) * 1000 / 32768;
		buf[3] = TEA5767_XTAL;
	}
	else {
		pll_word = 4 * (tune.freq - 225) * 1000 / 50000;
		buf[4] |= TEA5767_PLLREF;
	}

	buf[1] = pll_word & 0xff;
	buf[0] = (pll_word>>8) & 0x3f;
}

static void
tea5767_set_properties(struct tea5767_softc *sc, uint8_t *reg)
{
	memset(reg,0,5);

	tea5767_freq_to_pll(sc->tune, reg);

	if (sc->tune.mute) {
		reg[0] |= TEA5767_MUTE;
		reg[2] |=TEA5767_MUTE_R | TEA5767_MUTE_L;
	}

	reg[3] |= TEA5767_SNC;

	if (sc->tune.stereo == 0)
		reg[2] |= TEA5767_MONO;
	if(sc->tune.fm_band)
		reg[3] |= TEA5767_FM_BAND;
}

static bool
tea5767_if_check(struct tea5767_soft *sc)
{
	uint8_t reg[5];

	tea5767_set_properties(sc, reg);
	tea5767_write(sc, reg);

	memset(reg,0,5);
	delay(1000);
	tea5767_read(sc,reg);

	if ((reg[2] & 0x7f) == 0x37)
		return true;
	return false;    
}

static void
tea5767_pll_to_freq(struct tea5767_tune *tune, uint8_t *buf)
{
	int pll_word = buf[1] | (buf[0] & 0x3f) << 8;
	if (!tune->is_pll_set && !tune->is_xtal_set)
		tune->freq = pll_word * 50 / 4 + 225;
	else if (!tune->is_pll_set && tune->is_xtal_set)
		tune->freq = pll_word * 32768 / 4000 + 225;
	else
		tune->freq = pll_word * 50 / 4 + 225;
}

static int
tea5767_get_info(void *v, struct radio_info *ri)
{
	struct tea5767_softc *sc = v;
	uint8_t reg[5];
	tea5767_read(sc, reg);

	tea5767_pll_to_freq(&sc->tune, reg);
	ri->mute = sc->tune.mute;
	ri->stereo = sc->tune.stereo;
	ri->freq = sc->tune.freq;
	ri->lock = sc->tune.lock;

	ri->caps = sc->caps;

	return 0;
}

static int
tea5767_set_info(void *v, struct radio_info *ri)
{
	struct tea5767_softc *sc = v;
	int adc_conversion;
	uint8_t reg[5];

	sc->tune.mute = ri->mute;
	sc->tune.stereo = ri->stereo;
	sc->tune.freq = ri->freq;
	sc->tune.lock = ri->lock;

	adc_conversion = (ri->lock-60) * 3 / 30;

	switch(adc_conversion) {
	case 0: sc->tune.adc_stop_level = TEA5767_SSL_3;
		break;
	case 1: sc->tune.adc_stop_level = TEA5767_SSL_2;
		break;
	case 2: sc->tune.adc_stop_level = TEA5767_SSL_1;
		break;
	}

	tea5767_set_properties(sc, reg);
	tea5767_write(sc, reg);
	return 0;
}

static int
tea5767_search(void *v, int dir)
{
	struct tea5767_softc *sc = v;
	uint8_t reg[5];

	/* Increment frequency to search for the next frequency. */
	sc->tune.freq = sc->tune.freq >= 107500 ? 87500 : sc->tune.freq + 100;
	tea5767_set_properties(sc, reg);

	/* Activate autonomous search. */
	reg[0] |= TEA5767_SEARCH;

	/*
	 * If dir 1 => search up, if dir 0 => search down.
	 */
	if (dir)
		reg[2] |= TEA5767_SUD;

	reg[2] |= sc->tune.adc_stop_level; /* Stop level for search. */

	tea5767_write(sc, reg);

	memset(reg, 0, 5);

	while (tea5767_read(sc, reg), !(reg[0] & TEA5767_READY_FLAG)) {
		kpause("teasrch", true, hz/100, NULL);
	}

	return 0;
}

