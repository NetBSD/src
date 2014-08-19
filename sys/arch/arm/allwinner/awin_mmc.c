/* $NetBSD: awin_mmc.c,v 1.3.12.2 2014/08/20 00:02:44 tls Exp $ */

/*-
 * Copyright (c) 2014 Jared D. McNeill <jmcneill@invisible.ca>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "locators.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: awin_mmc.c,v 1.3.12.2 2014/08/20 00:02:44 tls Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>

#include <dev/sdmmc/sdmmcvar.h>
#include <dev/sdmmc/sdmmcchip.h>
#include <dev/sdmmc/sdmmc_ioreg.h>

#include <arm/allwinner/awin_reg.h>
#include <arm/allwinner/awin_var.h>

static int	awin_mmc_match(device_t, cfdata_t, void *);
static void	awin_mmc_attach(device_t, device_t, void *);

static int	awin_mmc_host_reset(sdmmc_chipset_handle_t);
static uint32_t	awin_mmc_host_ocr(sdmmc_chipset_handle_t);
static int	awin_mmc_host_maxblklen(sdmmc_chipset_handle_t);
static int	awin_mmc_card_detect(sdmmc_chipset_handle_t);
static int	awin_mmc_write_protect(sdmmc_chipset_handle_t);
static int	awin_mmc_bus_power(sdmmc_chipset_handle_t, uint32_t);
static int	awin_mmc_bus_clock(sdmmc_chipset_handle_t, int);
static int	awin_mmc_bus_width(sdmmc_chipset_handle_t, int);
static int	awin_mmc_bus_rod(sdmmc_chipset_handle_t, int);
static void	awin_mmc_exec_command(sdmmc_chipset_handle_t,
				      struct sdmmc_command *);
static void	awin_mmc_card_enable_intr(sdmmc_chipset_handle_t, int);
static void	awin_mmc_card_intr_ack(sdmmc_chipset_handle_t);

static struct sdmmc_chip_functions awin_mmc_chip_functions = {
	.host_reset = awin_mmc_host_reset,
	.host_ocr = awin_mmc_host_ocr,
	.host_maxblklen = awin_mmc_host_maxblklen,
	.card_detect = awin_mmc_card_detect,
	.write_protect = awin_mmc_write_protect,
	.bus_power = awin_mmc_bus_power,
	.bus_clock = awin_mmc_bus_clock,
	.bus_width = awin_mmc_bus_width,
	.bus_rod = awin_mmc_bus_rod,
	.exec_command = awin_mmc_exec_command,
	.card_enable_intr = awin_mmc_card_enable_intr,
	.card_intr_ack = awin_mmc_card_intr_ack,
};

struct awin_mmc_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	bus_dma_tag_t sc_dmat;

	int sc_mmc_number;
	int sc_mmc_width;
	int sc_mmc_present;

	device_t sc_sdmmc_dev;
	unsigned int sc_pll5_freq;
	unsigned int sc_mod_clk;

	bool sc_has_gpio_detect;
	struct awin_gpio_pindata sc_gpio_detect;	/* card detect */
	bool sc_has_gpio_wp;
	struct awin_gpio_pindata sc_gpio_wp;		/* write protect */
	bool sc_has_gpio_led;
	struct awin_gpio_pindata sc_gpio_led;		/* LED */
};

CFATTACH_DECL_NEW(awin_mmc, sizeof(struct awin_mmc_softc),
	awin_mmc_match, awin_mmc_attach, NULL, NULL);

#define MMC_WRITE(sc, reg, val)	\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))
#define MMC_READ(sc, reg) \
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))

static int
awin_mmc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct awinio_attach_args * const aio = aux;
	const struct awin_locators * const loc = &aio->aio_loc;
	const int port = cf->cf_loc[AWINIOCF_PORT];

	if (strcmp(cf->cf_name, loc->loc_name))
		return 0;

	if (port != AWINIOCF_PORT_DEFAULT && port != loc->loc_port)
		return 0;

	return 1;
}

static void
awin_mmc_probe_clocks(struct awin_mmc_softc *sc, struct awinio_attach_args *aio)
{
	uint32_t val, freq;
	int n, k, p, div;

	val = bus_space_read_4(aio->aio_core_bst, aio->aio_ccm_bsh,
	    AWIN_PLL5_CFG_REG);

	n = (val >> 8) & 0x1f;
	k = ((val >> 4) & 3) + 1;
	p = 1 << ((val >> 16) & 3);

	freq = 24000000 * n * k / p;

	sc->sc_pll5_freq = freq;
	if (sc->sc_pll5_freq > 400000000) {
		div = 4;
	} else {
		div = 3;
	}
	sc->sc_mod_clk = sc->sc_pll5_freq / (div + 1);

	awin_reg_set_clear(aio->aio_core_bst, aio->aio_ccm_bsh,
	    AWIN_SD0_CLK_REG + (sc->sc_mmc_number * 8),
	    AWIN_PLL_CFG_ENABLE | AWIN_PLL_CFG_EXG_MODE | div, 0);

#ifdef AWIN_MMC_DEBUG
	aprint_normal_dev(sc->sc_dev, "PLL5 @ %u Hz\n", freq);
#endif
}

static void
awin_mmc_attach(device_t parent, device_t self, void *aux)
{
	struct awin_mmc_softc * const sc = device_private(self);
	struct awinio_attach_args * const aio = aux;
	const struct awin_locators * const loc = &aio->aio_loc;
	struct sdmmcbus_attach_args saa;
	prop_dictionary_t cfg = device_properties(self);
	const char *pin_name;

	sc->sc_dev = self;
	sc->sc_bst = aio->aio_core_bst;
	sc->sc_dmat = aio->aio_dmat;
	sc->sc_mmc_number = loc->loc_port;
	bus_space_subregion(sc->sc_bst, aio->aio_core_bsh,
	    loc->loc_offset, loc->loc_size, &sc->sc_bsh);

	aprint_naive("\n");
	aprint_normal(": SD/MMC interface\n");

	awin_mmc_probe_clocks(sc, aio);

	if (prop_dictionary_get_cstring_nocopy(cfg, "detect-gpio", &pin_name)) {
		if (!awin_gpio_pin_reserve(pin_name, &sc->sc_gpio_detect)) {
			aprint_error_dev(self,
			    "failed to reserve GPIO \"%s\"\n", pin_name);
		} else {
			sc->sc_has_gpio_detect = true;
		}
	}
	if (prop_dictionary_get_cstring_nocopy(cfg, "wp-gpio", &pin_name)) {
		if (!awin_gpio_pin_reserve(pin_name, &sc->sc_gpio_wp)) {
			aprint_error_dev(self,
			    "failed to reserve GPIO \"%s\"\n", pin_name);
		} else {
			sc->sc_has_gpio_wp = true;
		}
	}
	if (prop_dictionary_get_cstring_nocopy(cfg, "led-gpio", &pin_name)) {
		if (!awin_gpio_pin_reserve(pin_name, &sc->sc_gpio_led)) {
			aprint_error_dev(self,
			    "failed to reserve GPIO \"%s\"\n", pin_name);
		} else {
			sc->sc_has_gpio_led = true;
		}
	}

	awin_mmc_host_reset(sc);
	awin_mmc_bus_width(sc, 1);

	memset(&saa, 0, sizeof(saa));
	saa.saa_busname = "sdmmc";
	saa.saa_sct = &awin_mmc_chip_functions;
	saa.saa_sch = sc;
	saa.saa_clkmin = 400;
	saa.saa_clkmax = 52000;
	saa.saa_caps = SMC_CAPS_4BIT_MODE|
		       SMC_CAPS_8BIT_MODE|
		       SMC_CAPS_SD_HIGHSPEED|
		       SMC_CAPS_MMC_HIGHSPEED|
		       SMC_CAPS_AUTO_STOP;
	if (sc->sc_has_gpio_detect) {
		saa.saa_caps |= SMC_CAPS_POLL_CARD_DET;
	}

	sc->sc_sdmmc_dev = config_found(self, &saa, NULL);
}

static void
awin_mmc_led(struct awin_mmc_softc *sc, int on)
{
	if (!sc->sc_has_gpio_led)
		return;
	awin_gpio_pindata_write(&sc->sc_gpio_led, on);
}

static int
awin_mmc_host_reset(sdmmc_chipset_handle_t sch)
{
	struct awin_mmc_softc *sc = sch;

#ifdef AWIN_MMC_DEBUG
	aprint_normal_dev(sc->sc_dev, "host reset\n");
#endif

	MMC_WRITE(sc, AWIN_MMC_GCTRL,
	    MMC_READ(sc, AWIN_MMC_GCTRL) | __BITS(2,0));

	return 0;
}

static uint32_t
awin_mmc_host_ocr(sdmmc_chipset_handle_t sch)
{
	return MMC_OCR_3_2V_3_3V | MMC_OCR_3_3V_3_4V;
}

static int
awin_mmc_host_maxblklen(sdmmc_chipset_handle_t sch)
{
	return 4096;
}

static int
awin_mmc_card_detect(sdmmc_chipset_handle_t sch)
{
	struct awin_mmc_softc *sc = sch;

	if (sc->sc_has_gpio_detect == false) {
		return 1;	/* no card detect pin, assume present */
	} else {
		int v = 0, i;
		for (i = 0; i < 5; i++) {
			v += awin_gpio_pindata_read(&sc->sc_gpio_detect);
			delay(1000);
		}
		if (v == 5)
			sc->sc_mmc_present = 0;
		else if (v == 0)
			sc->sc_mmc_present = 1;
		return sc->sc_mmc_present;
	}
}

static int
awin_mmc_write_protect(sdmmc_chipset_handle_t sch)
{
	struct awin_mmc_softc *sc = sch;

	if (sc->sc_has_gpio_wp == false) {
		return 0;	/* no write protect pin, assume rw */
	} else {
		return awin_gpio_pindata_read(&sc->sc_gpio_wp);
	}
}

static int
awin_mmc_bus_power(sdmmc_chipset_handle_t sch, uint32_t ocr)
{
	return 0;
}

static int
awin_mmc_update_clock(struct awin_mmc_softc *sc)
{
	uint32_t cmd;
	int retry;

	cmd = __BIT(31) | __BIT(21) | __BIT(13);
	MMC_WRITE(sc, AWIN_MMC_CMD, cmd);
	retry = 0xfffff;
	while (--retry > 0) {
		if (!(MMC_READ(sc, AWIN_MMC_CMD) & __BIT(31)))
			break;
		delay(10);
	}
	if (retry == 0) {
		aprint_error_dev(sc->sc_dev, "timeout updating clk\n");
		return ETIMEDOUT;
	}
	MMC_WRITE(sc, AWIN_MMC_RINT, MMC_READ(sc, AWIN_MMC_RINT));
	return 0;
}

static int
awin_mmc_bus_clock(sdmmc_chipset_handle_t sch, int freq)
{
	struct awin_mmc_softc *sc = sch;
	unsigned int div, freq_hz;
	uint32_t clkcr;

#ifdef AWIN_MMC_DEBUG
	aprint_normal_dev(sc->sc_dev, "freq = %d\n", freq);
#endif

	clkcr = MMC_READ(sc, AWIN_MMC_CLKCR);
	clkcr &= ~__BIT(16);
	MMC_WRITE(sc, AWIN_MMC_CLKCR, clkcr);
	if (awin_mmc_update_clock(sc) != 0)
		return 1;

	if (freq) {
		freq_hz = freq * 1000;
		div = (sc->sc_mod_clk + (freq_hz >> 1)) / freq_hz / 2;

		clkcr &= ~__BITS(15,0);
		clkcr |= div;
		MMC_WRITE(sc, AWIN_MMC_CLKCR, clkcr);
		if (awin_mmc_update_clock(sc) != 0)
			return 1;

		clkcr |= __BIT(16);
		MMC_WRITE(sc, AWIN_MMC_CLKCR, clkcr);
		if (awin_mmc_update_clock(sc) != 0)
			return 1;
	}

	return 0;
}

static int
awin_mmc_bus_width(sdmmc_chipset_handle_t sch, int width)
{
	struct awin_mmc_softc *sc = sch;

#ifdef AWIN_MMC_DEBUG
	aprint_normal_dev(sc->sc_dev, "width = %d\n", width);
#endif

	switch (width) {
	case 1:
		MMC_WRITE(sc, AWIN_MMC_WIDTH, 0);
		break;
	case 4:
		MMC_WRITE(sc, AWIN_MMC_WIDTH, 1);
		break;
	case 8:
		MMC_WRITE(sc, AWIN_MMC_WIDTH, 2);
		break;
	default:
		return 1;
	}

	sc->sc_mmc_width = width;
	
	return 0;
}

static int
awin_mmc_bus_rod(sdmmc_chipset_handle_t sch, int on)
{
	return -1;
}

static int
awin_mmc_xfer_wait(struct awin_mmc_softc *sc, struct sdmmc_command *cmd)
{
	int retry = 0xfffff;
	uint32_t bit = (cmd->c_flags & SCF_CMD_READ) ? __BIT(2) : __BIT(3);

	while (--retry > 0) {
		uint32_t status = MMC_READ(sc, AWIN_MMC_STATUS);
		if (!(status & bit))
			return 0;
		delay(10);
	}

	return ETIMEDOUT;
}

static int
awin_mmc_xfer_data(struct awin_mmc_softc *sc, struct sdmmc_command *cmd)
{
	uint32_t *datap = (uint32_t *)cmd->c_buf;
	int i;

	for (i = 0; i < (cmd->c_resid >> 2); i++) {
		if (awin_mmc_xfer_wait(sc, cmd))
			return ETIMEDOUT;
		if (cmd->c_flags & SCF_CMD_READ) {
			datap[i] = MMC_READ(sc, AWIN_MMC_FIFO);
		} else {
			MMC_WRITE(sc, AWIN_MMC_FIFO, datap[i]);
		}
	}

	return 0;
}

static void
awin_mmc_exec_command(sdmmc_chipset_handle_t sch, struct sdmmc_command *cmd)
{
	struct awin_mmc_softc *sc = sch;
	uint32_t cmdval = __BIT(31);
	uint32_t status;
	int retry;

#ifdef AWIN_MMC_DEBUG
	aprint_normal_dev(sc->sc_dev,
	    "opcode %d flags 0x%x data %p datalen %d\n",
	    cmd->c_opcode, cmd->c_flags, cmd->c_data, cmd->c_datalen);
#endif

	if (cmd->c_opcode == 0)
		cmdval |= __BIT(15);
	if (cmd->c_flags & SCF_RSP_PRESENT)
		cmdval |= __BIT(6);
	if (cmd->c_flags & SCF_RSP_136)
		cmdval |= __BIT(7);
	if (cmd->c_flags & SCF_RSP_CRC)
		cmdval |= __BIT(8);

	if (cmd->c_datalen > 0) {
		unsigned int nblks;

		cmdval |= __BIT(9) | __BIT(13);
		if (!ISSET(cmd->c_flags, SCF_CMD_READ)) {
			cmdval |= __BIT(10);
		}

		nblks = cmd->c_datalen / cmd->c_blklen;
		if (nblks == 0 || (cmd->c_datalen % cmd->c_blklen) != 0)
			++nblks;

		if (nblks > 1) {
			cmdval |= __BIT(12);
		}

		MMC_WRITE(sc, AWIN_MMC_BLKSZ, cmd->c_blklen);
		MMC_WRITE(sc, AWIN_MMC_BYTECNT, nblks * cmd->c_blklen);
	}

	MMC_WRITE(sc, AWIN_MMC_ARG, cmd->c_arg);

#ifdef AWIN_MMC_DEBUG
	aprint_normal_dev(sc->sc_dev, "cmdval = %08x\n", cmdval);
#endif
	if (cmd->c_datalen == 0) {
		MMC_WRITE(sc, AWIN_MMC_CMD, cmdval | cmd->c_opcode);
	} else {
		MMC_WRITE(sc, AWIN_MMC_GCTRL,
		    MMC_READ(sc, AWIN_MMC_GCTRL) | __BIT(31));
		MMC_WRITE(sc, AWIN_MMC_CMD, cmdval | cmd->c_opcode);
		cmd->c_resid = cmd->c_datalen;
		cmd->c_buf = cmd->c_data;
		awin_mmc_led(sc, 0);
		cmd->c_error = awin_mmc_xfer_data(sc, cmd);
		awin_mmc_led(sc, 1);
		if (cmd->c_error) {
			aprint_error_dev(sc->sc_dev,
			    "xfer data timeout\n");
			goto done;
		}
	}

	retry = 0xfffff;
	while (--retry > 0) {
		status = MMC_READ(sc, AWIN_MMC_RINT);
		if (status & 0xbfc2) {
			retry = 0;
			break;
		}
		if (status & 0x4)
			break;
		delay(10);
	}
	if (retry == 0) {
#ifdef AWIN_MMC_DEBUG
		aprint_error_dev(sc->sc_dev,
		    "RINT (1) timeout, status = %08x\n", status);
#endif
		cmd->c_error = ETIMEDOUT;
		goto done;
	}
#ifdef AWIN_MMC_DEBUG
	aprint_normal_dev(sc->sc_dev, "status = %08x\n", status);
#endif

	if (cmd->c_datalen > 0) {
		retry = 0xffff;
		while (--retry > 0) {
			uint32_t done;
			status = MMC_READ(sc, AWIN_MMC_RINT);
			if (status & 0xbfc2) {
				retry = 0;
				break;
			}
			if (cmd->c_blklen < cmd->c_datalen)
				done = status & __BIT(14);
			else
				done = status & __BIT(3);
			if (done)
				break;
			delay(10);
		}
		if (retry == 0) {
#ifdef AWIN_MMC_DEBUG
			aprint_error_dev(sc->sc_dev,
			    "RINT (2) timeout, status = %08x\n", status);
#endif
			cmd->c_error = ETIMEDOUT;
			goto done;
		}
	}

	if (cmd->c_flags & SCF_RSP_BSY) {
		retry = 0xfffff;
		while (--retry > 0) {
			status = MMC_READ(sc, AWIN_MMC_STATUS);
			if (status & __BIT(9))
				break;
		}
		if (retry == 0) {
#ifdef AWIN_MMC_DEBUG
			aprint_error_dev(sc->sc_dev,
			    "RINT (3) timeout, status = %08x\n", status);
#endif
			cmd->c_error = ETIMEDOUT;
			goto done;
		}
	}

	if (cmd->c_flags & SCF_RSP_PRESENT) {
		if (cmd->c_flags & SCF_RSP_136) {
			cmd->c_resp[0] = MMC_READ(sc, AWIN_MMC_RESP0);
			cmd->c_resp[1] = MMC_READ(sc, AWIN_MMC_RESP1);
			cmd->c_resp[2] = MMC_READ(sc, AWIN_MMC_RESP2);
			cmd->c_resp[3] = MMC_READ(sc, AWIN_MMC_RESP3);
			if (cmd->c_flags & SCF_RSP_CRC) {
				cmd->c_resp[0] = (cmd->c_resp[0] >> 8) |
				    (cmd->c_resp[1] << 24);
				cmd->c_resp[1] = (cmd->c_resp[1] >> 8) |
				    (cmd->c_resp[2] << 24);
				cmd->c_resp[2] = (cmd->c_resp[2] >> 8) |
				    (cmd->c_resp[3] << 24);
				cmd->c_resp[3] = (cmd->c_resp[3] >> 8);
			}
		} else {
			cmd->c_resp[0] = MMC_READ(sc, AWIN_MMC_RESP0);
		}
	}

done:
	cmd->c_flags |= SCF_ITSDONE;

	if (cmd->c_error) {
		MMC_WRITE(sc, AWIN_MMC_GCTRL, __BITS(2,0));
		awin_mmc_update_clock(sc);
	}
	MMC_WRITE(sc, AWIN_MMC_RINT, __BITS(31,0));
	MMC_WRITE(sc, AWIN_MMC_GCTRL, MMC_READ(sc, AWIN_MMC_GCTRL) | __BIT(1));
}

static void
awin_mmc_card_enable_intr(sdmmc_chipset_handle_t sch, int enable)
{
}

static void
awin_mmc_card_intr_ack(sdmmc_chipset_handle_t sch)
{
}
