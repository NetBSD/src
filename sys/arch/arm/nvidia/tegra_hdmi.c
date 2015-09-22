/* $NetBSD: tegra_hdmi.c,v 1.2.2.3 2015/09/22 12:05:38 skrll Exp $ */

/*-
 * Copyright (c) 2015 Jared D. McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: tegra_hdmi.c,v 1.2.2.3 2015/09/22 12:05:38 skrll Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <dev/i2c/i2cvar.h>
#include <dev/i2c/ddcvar.h>
#include <dev/videomode/videomode.h>
#include <dev/videomode/edidvar.h>

#include <arm/nvidia/tegra_reg.h>
#include <arm/nvidia/tegra_hdmireg.h>
#include <arm/nvidia/tegra_var.h>

#define PROP_PHYSICAL_ADDRESS	"physical-address"

static int	tegra_hdmi_match(device_t, cfdata_t, void *);
static void	tegra_hdmi_attach(device_t, device_t, void *);

static const struct tegra_hdmi_tmds_config {
	u_int		dot_clock;
	uint32_t	sor_pll0;
	uint32_t	sor_pll1;
	uint32_t	sor_lane_drive_current;
	uint32_t	pe_current;
	uint32_t	sor_io_peak_current;
	uint32_t	sor_pad_ctls0;
	uint32_t	car_plld_misc;	/* XXX unused? */
} tegra_hdmi_tmds_config[] = {
	/* 480p */
	{ 27000,      0x01003010, 0x00301b00, 0x1f1f1f1f,
	  0x00000000, 0x03030303, 0x800034bb, 0x40400820 },
	/* 720p / 1080i */
	{ 74250,      0x01003110, 0x00301500, 0x2c2c2c2c,
	  0x00000000, 0x07070707, 0x800034bb, 0x40400820 },
	/* 1080p */
	{ 148500,     0x01003310, 0x00301500, 0x2d2d2d2d,
	  0x00000000, 0x05050505, 0x800034bb, 0x40400820 },
	/* 2160p */
	{ 297000,     0x01003f10, 0x00300f00, 0x37373737,
	  0x00000000, 0x17171717, 0x800036bb, 0x40400f20 },
};

struct tegra_hdmi_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;

	device_t		sc_displaydev;
	device_t		sc_ddcdev;
	struct tegra_gpio_pin	*sc_pin_hpd;
	struct tegra_gpio_pin	*sc_pin_pll;
	struct tegra_gpio_pin	*sc_pin_power;

	bool			sc_connected;
	const struct videomode	*sc_curmode;
	bool			sc_hdmimode;
};

static void	tegra_hdmi_hpd(struct tegra_hdmi_softc *);
static void	tegra_hdmi_connect(struct tegra_hdmi_softc *);
static void	tegra_hdmi_disconnect(struct tegra_hdmi_softc *);
static void	tegra_hdmi_enable(struct tegra_hdmi_softc *, const uint8_t *);
static int	tegra_hdmi_sor_start(struct tegra_hdmi_softc *);
static bool	tegra_hdmi_is_hdmi(struct tegra_hdmi_softc *,
				   const struct edid_info *);
static void	tegra_hdmi_setup_audio_infoframe(struct tegra_hdmi_softc *);
static uint8_t	tegra_hdmi_infoframe_csum(const uint8_t *, size_t);

CFATTACH_DECL_NEW(tegra_hdmi, sizeof(struct tegra_hdmi_softc),
	tegra_hdmi_match, tegra_hdmi_attach, NULL, NULL);

#define HDMI_READ(sc, reg)			\
    bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define HDMI_WRITE(sc, reg, val)		\
    bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))
#define HDMI_SET_CLEAR(sc, reg, set, clr)	\
    tegra_reg_set_clear((sc)->sc_bst, (sc)->sc_bsh, (reg), (set), (clr))

static int
tegra_hdmi_match(device_t parent, cfdata_t cf, void *aux)
{
	return 1;
}

static void
tegra_hdmi_attach(device_t parent, device_t self, void *aux)
{
	struct tegra_hdmi_softc * const sc = device_private(self);
	struct tegraio_attach_args * const tio = aux;
	const struct tegra_locators * const loc = &tio->tio_loc;
	prop_dictionary_t prop = device_properties(self);
	const char *pin, *dev;

	sc->sc_dev = self;
	sc->sc_bst = tio->tio_bst;
	if (bus_space_map(sc->sc_bst, TEGRA_GHOST_BASE + loc->loc_offset,
	    loc->loc_size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map HDMI\n");
		return;
	}

	if (prop_dictionary_get_cstring_nocopy(prop, "hpd-gpio", &pin)) {
		sc->sc_pin_hpd = tegra_gpio_acquire(pin, GPIO_PIN_INPUT);
	}
	if (prop_dictionary_get_cstring_nocopy(prop, "pll-gpio", &pin)) {
		sc->sc_pin_pll = tegra_gpio_acquire(pin, GPIO_PIN_OUTPUT);
		if (sc->sc_pin_pll) {
			tegra_gpio_write(sc->sc_pin_pll, 0);
		} else {
			panic("couldn't get pll-gpio pin");
		}
	}
	if (prop_dictionary_get_cstring_nocopy(prop, "power-gpio", &pin)) {
		sc->sc_pin_power = tegra_gpio_acquire(pin, GPIO_PIN_OUTPUT);
		if (sc->sc_pin_power) {
			tegra_gpio_write(sc->sc_pin_power, 1);
		}
	}
	if (prop_dictionary_get_cstring_nocopy(prop, "ddc-device", &dev)) {
		sc->sc_ddcdev = device_find_by_xname(dev);
	}
	if (prop_dictionary_get_cstring_nocopy(prop, "display-device", &dev)) {
		sc->sc_displaydev = device_find_by_xname(dev);
	}

	if (sc->sc_displaydev == NULL) {
		aprint_error(": no display-device property\n");
		return;
	}

	aprint_naive("\n");
	aprint_normal(": HDMI\n");

	tegra_hdmi_hpd(sc);
}

static void
tegra_hdmi_hpd(struct tegra_hdmi_softc *sc)
{
	bool con;

	if (sc->sc_pin_hpd) {
		con = tegra_gpio_read(sc->sc_pin_hpd);
	} else {
		con = true;
	}

	if (sc->sc_connected == con)
		return;

	if (con) {
		device_printf(sc->sc_dev, "display connected\n");
		tegra_hdmi_connect(sc);
	} else {
		device_printf(sc->sc_dev, "display disconnected\n");
		tegra_hdmi_disconnect(sc);
	}

	sc->sc_connected = con;
}

static void
tegra_hdmi_connect(struct tegra_hdmi_softc *sc)
{
	const struct videomode *mode;
	char edid[128], *pedid = NULL;
	struct edid_info ei;
	int retry = 4, error;

	memset(&ei, 0, sizeof(ei));

	if (sc->sc_ddcdev) {
		memset(edid, 0, sizeof(edid));

		while (--retry > 0) {
			error = ddc_dev_read_edid(sc->sc_ddcdev, edid,
			    sizeof(edid));
			if (error == 0) {
				break;
			}
		}
		if (retry == 0) {
			device_printf(sc->sc_dev, "failed to read EDID (%d)\n",
			    error);
		} else {
			if (edid_parse(edid, &ei) != 0) {
				device_printf(sc->sc_dev,
				    "failed to parse EDID\n");
			} else {
#ifdef TEGRA_HDMI_DEBUG
				edid_print(&ei);
#endif
				pedid = edid;
			}
		}
	}

	mode = ei.edid_preferred_mode;
	if (mode == NULL) {
		mode = pick_mode_by_ref(640, 480, 60);
	}

	sc->sc_curmode = mode;
	sc->sc_hdmimode = tegra_hdmi_is_hdmi(sc, &ei);

	tegra_hdmi_enable(sc, pedid);
}

static void
tegra_hdmi_disconnect(struct tegra_hdmi_softc *sc)
{
	prop_dictionary_t cfg = device_properties(sc->sc_dev);

	prop_dictionary_remove(cfg, PROP_PHYSICAL_ADDRESS);
}

static void
tegra_hdmi_enable(struct tegra_hdmi_softc *sc, const uint8_t *edid)
{
	const struct tegra_hdmi_tmds_config *tmds = NULL;
	const struct videomode *mode = sc->sc_curmode;
	uint32_t input_ctrl;
	u_int i;

	KASSERT(sc->sc_curmode != NULL);
	tegra_pmc_hdmi_enable();

	tegra_car_hdmi_enable(mode->dot_clock * 1000);

	for (i = 0; i < __arraycount(tegra_hdmi_tmds_config); i++) {
		if (tegra_hdmi_tmds_config[i].dot_clock >= mode->dot_clock) {
			break;
		}
	}
	if (i < __arraycount(tegra_hdmi_tmds_config)) {
		tmds = &tegra_hdmi_tmds_config[i];
	} else {
		tmds = &tegra_hdmi_tmds_config[__arraycount(tegra_hdmi_tmds_config) - 1];
	}

	HDMI_WRITE(sc, HDMI_NV_PDISP_SOR_PLL0_REG, tmds->sor_pll0);
	HDMI_WRITE(sc, HDMI_NV_PDISP_SOR_PLL1_REG, tmds->sor_pll1);
	HDMI_WRITE(sc, HDMI_NV_PDISP_SOR_LANE_DRIVE_CURRENT_REG,
	    tmds->sor_lane_drive_current);
	HDMI_WRITE(sc, HDMI_NV_PDISP_PE_CURRENT_REG, tmds->pe_current);
	HDMI_WRITE(sc, HDMI_NV_PDISP_SOR_IO_PEAK_CURRENT_REG,
	    tmds->sor_io_peak_current);
	HDMI_WRITE(sc, HDMI_NV_PDISP_SOR_PAD_CTLS0_REG,
	    tmds->sor_pad_ctls0);

	tegra_dc_enable(sc->sc_displaydev, sc->sc_dev, mode, edid);

	const u_int div = (mode->dot_clock / 1000) * 4;
	HDMI_WRITE(sc, HDMI_NV_PDISP_SOR_REFCLK_REG,
	    __SHIFTIN(div >> 2, HDMI_NV_PDISP_SOR_REFCLK_DIV_INT) |
	    __SHIFTIN(div & 3, HDMI_NV_PDISP_SOR_REFCLK_DIV_FRAC));

	HDMI_SET_CLEAR(sc, HDMI_NV_PDISP_SOR_CSTM_REG,
	    __SHIFTIN(HDMI_NV_PDISP_SOR_CSTM_MODE_TMDS,
		      HDMI_NV_PDISP_SOR_CSTM_MODE) |
	    __SHIFTIN(2, HDMI_NV_PDISP_SOR_CSTM_ROTCLK) |
	    HDMI_NV_PDISP_SOR_CSTM_PLLDIV,
	    HDMI_NV_PDISP_SOR_CSTM_MODE |
	    HDMI_NV_PDISP_SOR_CSTM_ROTCLK |
	    HDMI_NV_PDISP_SOR_CSTM_LVDS_EN);

	const uint32_t inst =
	    HDMI_NV_PDISP_SOR_SEQ_INST_DRIVE_PWM_OUT_LO |
	    HDMI_NV_PDISP_SOR_SEQ_INST_HALT |
	    __SHIFTIN(2, HDMI_NV_PDISP_SOR_SEQ_INST_WAIT_UNITS) |
	    __SHIFTIN(1, HDMI_NV_PDISP_SOR_SEQ_INST_WAIT_TIME);
	HDMI_WRITE(sc, HDMI_NV_PDISP_SOR_SEQ_INST0_REG, inst);
	HDMI_WRITE(sc, HDMI_NV_PDISP_SOR_SEQ_INST8_REG, inst);

	input_ctrl = __SHIFTIN(tegra_dc_port(sc->sc_displaydev),
			       HDMI_NV_PDISP_INPUT_CONTROL_HDMI_SRC_SELECT);
	if (mode->hdisplay != 640 || mode->vdisplay != 480)
		input_ctrl |= HDMI_NV_PDISP_INPUT_CONTROL_ARM_VIDEO_RANGE;
	HDMI_WRITE(sc, HDMI_NV_PDISP_INPUT_CONTROL_REG, input_ctrl);

	if (tegra_hdmi_sor_start(sc) != 0)
		return;

	const u_int rekey = 56;
	const u_int hspw = mode->hsync_end - mode->hsync_start;
	const u_int hbp = mode->htotal - mode->hsync_end;
	const u_int hfp = mode->hsync_start - mode->hdisplay;
	const u_int max_ac_packet = (hspw + hbp + hfp - rekey - 18) / 32;
	uint32_t ctrl =
	    __SHIFTIN(rekey, HDMI_NV_PDISP_HDMI_CTRL_REKEY) |
	    __SHIFTIN(max_ac_packet, HDMI_NV_PDISP_HDMI_CTRL_MAX_AC_PACKET);
	if (sc->sc_hdmimode) {
		ctrl |= HDMI_NV_PDISP_HDMI_CTRL_ENABLE; /* HDMI ENABLE */
	}
	HDMI_WRITE(sc, HDMI_NV_PDISP_HDMI_CTRL_REG, ctrl);

	if (sc->sc_hdmimode) {
		const u_int n = 6144;	/* 48 kHz */
		const u_int cts = ((mode->dot_clock * 10) * (n / 128)) / 480;

		HDMI_WRITE(sc, HDMI_NV_PDISP_SOR_AUDIO_CNTRL0_REG,
		    __SHIFTIN(HDMI_NV_PDISP_SOR_AUDIO_CNTRL0_SOURCE_SELECT_AUTO,
			      HDMI_NV_PDISP_SOR_AUDIO_CNTRL0_SOURCE_SELECT) |
		    HDMI_NV_PDISP_SOR_AUDIO_CNTRL0_INJECT_NULLSMPL);
		HDMI_WRITE(sc, HDMI_NV_PDISP_AUDIO_N_REG,
		    HDMI_NV_PDISP_AUDIO_N_RESETF |
		    HDMI_NV_PDISP_AUDIO_N_GENERATE |
		    __SHIFTIN(n - 1, HDMI_NV_PDISP_AUDIO_N_VALUE));

		HDMI_WRITE(sc, HDMI_NV_PDISP_HDMI_SPARE_REG,
		    HDMI_NV_PDISP_HDMI_SPARE_HW_CTS |
		    HDMI_NV_PDISP_HDMI_SPARE_FORCE_SW_CTS |
		    __SHIFTIN(1, HDMI_NV_PDISP_HDMI_SPARE_CTS_RESET_VAL));

		/*
		 * When HW_CTS=1 and FORCE_SW_CTS=1, the CTS is programmed by
		 * software in the 44.1 kHz register regardless of chosen rate.
		 */
		HDMI_WRITE(sc, HDMI_NV_PDISP_HDMI_ACR_0441_SUBPACK_LOW_REG,
		    cts << 8);
		HDMI_WRITE(sc, HDMI_NV_PDISP_HDMI_ACR_0441_SUBPACK_HIGH_REG,
		    0x80000000 | n);

		HDMI_SET_CLEAR(sc, HDMI_NV_PDISP_AUDIO_N_REG, 0,
		    HDMI_NV_PDISP_AUDIO_N_RESETF);

		HDMI_WRITE(sc, 24000, HDMI_NV_PDISP_SOR_AUDIO_AVAL_0480_REG);

		tegra_hdmi_setup_audio_infoframe(sc);

		HDMI_WRITE(sc, HDMI_NV_PDISP_HDMI_GENERIC_CTRL_REG,
		    HDMI_NV_PDISP_HDMI_GENERIC_CTRL_AUDIO);
		HDMI_WRITE(sc, HDMI_NV_PDISP_HDMI_AVI_INFOFRAME_CTRL_REG, 0);
		HDMI_WRITE(sc, HDMI_NV_PDISP_HDMI_AUDIO_INFOFRAME_CTRL_REG,
		    HDMI_NV_PDISP_HDMI_AUDIO_INFOFRAME_CTRL_ENABLE);
		HDMI_WRITE(sc, HDMI_NV_PDISP_HDMI_ACR_CTRL_REG, 0);
	} else {
		HDMI_WRITE(sc, HDMI_NV_PDISP_HDMI_GENERIC_CTRL_REG, 0);
		HDMI_WRITE(sc, HDMI_NV_PDISP_HDMI_AVI_INFOFRAME_CTRL_REG, 0);
		HDMI_WRITE(sc, HDMI_NV_PDISP_HDMI_AUDIO_INFOFRAME_CTRL_REG, 0);
		HDMI_WRITE(sc, HDMI_NV_PDISP_HDMI_ACR_CTRL_REG, 0);
	}

	/* Start HDMI output */
	tegra_dc_hdmi_start(sc->sc_displaydev);
}

static int
tegra_hdmi_sor_start(struct tegra_hdmi_softc *sc)
{
	int retry;

	HDMI_SET_CLEAR(sc, HDMI_NV_PDISP_SOR_PLL0_REG,
	    0,
	    HDMI_NV_PDISP_SOR_PLL0_PWR |
	    HDMI_NV_PDISP_SOR_PLL0_VCOPD |
	    HDMI_NV_PDISP_SOR_PLL0_PULLDOWN);
	delay(10);
	HDMI_SET_CLEAR(sc, HDMI_NV_PDISP_SOR_PLL0_REG,
	    0,
	    HDMI_NV_PDISP_SOR_PLL0_PDBG);

	HDMI_WRITE(sc, HDMI_NV_PDISP_SOR_PWR_REG,
	    HDMI_NV_PDISP_SOR_PWR_NORMAL_STATE |
	    HDMI_NV_PDISP_SOR_PWR_SETTING_NEW);

	HDMI_WRITE(sc, HDMI_NV_PDISP_SOR_PWR_REG,
	    HDMI_NV_PDISP_SOR_PWR_NORMAL_STATE);

	for (retry = 10000; retry > 0; retry--) {
		const uint32_t pwr = HDMI_READ(sc, HDMI_NV_PDISP_SOR_PWR_REG);
		if ((pwr & HDMI_NV_PDISP_SOR_PWR_SETTING_NEW) == 0)
			break;
		delay(10);
	}
	if (retry == 0) {
		device_printf(sc->sc_dev, "timeout enabling SOR power\n");
		return ETIMEDOUT;
	}

	uint32_t state2 =
	    __SHIFTIN(1, HDMI_NV_PDISP_SOR_STATE2_ASY_OWNER) |
	    __SHIFTIN(3, HDMI_NV_PDISP_SOR_STATE2_ASY_SUBOWNER) |
	    __SHIFTIN(1, HDMI_NV_PDISP_SOR_STATE2_ASY_CRCMODE) |
	    __SHIFTIN(1, HDMI_NV_PDISP_SOR_STATE2_ASY_PROTOCOL);
	if (sc->sc_curmode->flags & VID_NHSYNC)
		state2 |= HDMI_NV_PDISP_SOR_STATE2_ASY_HSYNCPOL;
	if (sc->sc_curmode->flags & VID_NVSYNC)
		state2 |= HDMI_NV_PDISP_SOR_STATE2_ASY_VSYNCPOL;
	HDMI_WRITE(sc, HDMI_NV_PDISP_SOR_STATE2_REG, state2);

	HDMI_WRITE(sc, HDMI_NV_PDISP_SOR_STATE1_REG,
	    __SHIFTIN(HDMI_NV_PDISP_SOR_STATE1_ASY_HEAD_OPMODE_AWAKE,
		      HDMI_NV_PDISP_SOR_STATE1_ASY_HEAD_OPMODE) |
	    HDMI_NV_PDISP_SOR_STATE1_ASY_ORMODE);

	HDMI_WRITE(sc, HDMI_NV_PDISP_SOR_STATE0_REG, 0);

	HDMI_WRITE(sc, HDMI_NV_PDISP_SOR_STATE0_REG,
	    HDMI_NV_PDISP_SOR_STATE0_UPDATE);

	HDMI_SET_CLEAR(sc, HDMI_NV_PDISP_SOR_STATE1_REG,
	    HDMI_NV_PDISP_SOR_STATE1_ATTACHED, 0);

	HDMI_WRITE(sc, HDMI_NV_PDISP_SOR_STATE0_REG, 0);

	return 0;
}

static bool
tegra_hdmi_is_hdmi(struct tegra_hdmi_softc *sc, const struct edid_info *ei)
{
	prop_dictionary_t cfg = device_properties(sc->sc_dev);
	char edid[128];
	bool found_hdmi = false;
	unsigned int n, p;
	uint16_t phys_addr;

	/*
	 * Scan through extension blocks, looking for a CEA-861-D v3
	 * block. If an HDMI Vendor-Specific Data Block (HDMI VSDB) is
	 * found in that, assume HDMI mode.
	 */
	for (n = 1; n <= MIN(ei->edid_ext_block_count, 4); n++) {
		if (ddc_dev_read_edid_block(sc->sc_ddcdev, edid,
		    sizeof(edid), n)) {
			break;
		}

		const uint8_t tag = edid[0];
		const uint8_t rev = edid[1];
		const uint8_t off = edid[2];

		/* We are looking for a CEA-861-D tag (02h) with revision 3 */
		if (tag != 0x02 || rev != 3)
			continue;
		/*
		 * CEA data block collection starts at byte 4, so the
		 * DTD blocks must start after it.
		 */
		if (off <= 4)
			continue;

		/* Parse the CEA data blocks */
		for (p = 4; p < off;) {
			const uint8_t btag = (edid[p] >> 5) & 0x7;
			const uint8_t blen = edid[p] & 0x1f;

			/* Make sure the length is sane */
			if (p + blen + 1 > off)
				break;
			/* Looking for a VSDB tag */
			if (btag != 3)
				goto next_block;
			/* HDMI VSDB is at least 5 bytes long */
			if (blen < 5)
				goto next_block;

			/* HDMI 24-bit IEEE registration ID is 0x000C03 */
			if (memcmp(&edid[p + 1], "\x03\x0c\x00", 3) == 0) {
				found_hdmi = true;

				phys_addr = (edid[p + 4] << 8) | edid[p + 5];

				prop_dictionary_set_uint16(cfg,
				    PROP_PHYSICAL_ADDRESS, phys_addr);
			}

next_block:
			p += (1 + blen);
		}
	}

	return found_hdmi;
}

static void
tegra_hdmi_setup_audio_infoframe(struct tegra_hdmi_softc *sc)
{
	uint8_t data[10] = {
		0x84, 0x01, 0x0a,
		0x00,	/* PB0 (checksum) */
		0x01,	/* CT=0, CC=2ch */
		0x00,	/* SS=0, SF=0 */
		0x00,
		0x00,	/* CA=FR/FL */
		0x00,	/* LSV=0dB, DM_INH=permitted */
		0x00
	};

	data[3] = tegra_hdmi_infoframe_csum(data, sizeof(data));

	HDMI_WRITE(sc, HDMI_NV_PDISP_HDMI_AUDIO_INFOFRAME_HEADER_REG,
	    data[0] | (data[1] << 8) | (data[2] << 16));
	HDMI_WRITE(sc, HDMI_NV_PDISP_HDMI_AUDIO_INFOFRAME_SUBPACK0_LOW_REG,
	    data[3] | (data[4] << 8) | (data[5] << 16) | (data[6] << 24));
	HDMI_WRITE(sc, HDMI_NV_PDISP_HDMI_AUDIO_INFOFRAME_SUBPACK0_HIGH_REG,
	    data[7] | (data[8] << 8) | (data[9] << 16));
}

static uint8_t
tegra_hdmi_infoframe_csum(const uint8_t *data, size_t len)
{
	uint8_t csum = 0;
	u_int n;

	for (n = 0; n < len; n++)
		csum += data[n];
	if (csum)
		csum = 0x100 - csum;

	return csum;
}
