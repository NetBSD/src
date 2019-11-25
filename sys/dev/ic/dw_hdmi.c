/* $NetBSD: dw_hdmi.c,v 1.1.6.2 2019/11/25 16:18:40 martin Exp $ */

/*-
 * Copyright (c) 2019 Jared D. McNeill <jmcneill@invisible.ca>
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: dw_hdmi.c,v 1.1.6.2 2019/11/25 16:18:40 martin Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/conf.h>

#include <dev/ic/dw_hdmi.h>

#include <dev/i2c/i2cvar.h>
#include <dev/i2c/ddcvar.h>
#include <dev/i2c/ddcreg.h>
#include <dev/videomode/videomode.h>
#include <dev/videomode/edidvar.h>

#include <dev/audio/audio_dai.h>

#include <drm/drmP.h>
#include <drm/drm_crtc.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_edid.h>

#define	HDMI_DESIGN_ID		0x0000
#define	HDMI_REVISION_ID	0x0001
#define	HDMI_CONFIG0_ID		0x0004
#define	 HDMI_CONFIG0_ID_AUDI2S			__BIT(4)
#define	HDMI_CONFIG2_ID		0x0006

#define	HDMI_IH_I2CM_STAT0	0x0105
#define	 HDMI_IH_I2CM_STAT0_DONE		__BIT(1)
#define	 HDMI_IH_I2CM_STAT0_ERROR		__BIT(0)
#define	HDMI_IH_MUTE		0x01ff
#define	 HDMI_IH_MUTE_WAKEUP_INTERRUPT		__BIT(1)
#define	 HDMI_IH_MUTE_ALL_INTERRUPT		__BIT(0)

#define	HDMI_TX_INVID0		0x0200
#define	 HDMI_TX_INVID0_VIDEO_MAPPING		__BITS(4,0)
#define	  HDMI_TX_INVID0_VIDEO_MAPPING_DEFAULT	1
#define	HDMI_TX_INSTUFFING	0x0201
#define	 HDMI_TX_INSTUFFING_BCBDATA_STUFFING	__BIT(2)
#define	 HDMI_TX_INSTUFFING_RCRDATA_STUFFING	__BIT(1)
#define	 HDMI_TX_INSTUFFING_GYDATA_STUFFING	__BIT(0)
#define	HDMI_TX_GYDATA0		0x0202
#define	HDMI_TX_GYDATA1		0x0203
#define	HDMI_TX_RCRDATA0	0x0204
#define	HDMI_TX_RCRDATA1	0x0205
#define	HDMI_TX_BCBDATA0	0x0206
#define	HDMI_TX_BCBDATA1	0x0207

#define	HDMI_VP_STATUS		0x0800
#define	HDMI_VP_PR_CD		0x0801
#define	 HDMI_VP_PR_CD_COLOR_DEPTH		__BITS(7,4)
#define	  HDMI_VP_PR_CD_COLOR_DEPTH_24		0
#define	 HDMI_VP_PR_CD_DESIRED_PR_FACTOR	__BITS(3,0)
#define	  HDMI_VP_PR_CD_DESIRED_PR_FACTOR_NONE	0
#define	HDMI_VP_STUFF		0x0802
#define	 HDMI_VP_STUFF_IDEFAULT_PHASE		__BIT(5)
#define	 HDMI_VP_STUFF_YCC422_STUFFING		__BIT(2)
#define	 HDMI_VP_STUFF_PP_STUFFING		__BIT(1)
#define	 HDMI_VP_STUFF_PR_STUFFING		__BIT(0)
#define	HDMI_VP_REMAP		0x0803
#define	 HDMI_VP_REMAP_YCC422_SIZE		__BITS(1,0)
#define	  HDMI_VP_REMAP_YCC422_SIZE_16		0
#define	HDMI_VP_CONF		0x0804
#define	 HDMI_VP_CONF_BYPASS_EN			__BIT(6)
#define	 HDMI_VP_CONF_BYPASS_SELECT		__BIT(2)
#define	 HDMI_VP_CONF_OUTPUT_SELECT		__BITS(1,0)
#define	  HDMI_VP_CONF_OUTPUT_SELECT_BYPASS	2
#define	HDMI_VP_STAT		0x0805
#define	HDMI_VP_INT		0x0806
#define	HDMI_VP_MASK		0x0807
#define	HDMI_VP_POL		0x0808

#define	HDMI_FC_INVIDCONF	0x1000
#define	 HDMI_FC_INVIDCONF_VSYNC_IN_POLARITY	__BIT(6)
#define	 HDMI_FC_INVIDCONF_HSYNC_IN_POLARITY	__BIT(5)
#define	 HDMI_FC_INVIDCONF_DE_IN_POLARITY	__BIT(4)
#define	 HDMI_FC_INVIDCONF_DVI_MODE		__BIT(3)
#define	 HDMI_FC_INVIDCONF_R_V_BLANK_IN_OSC	__BIT(1)
#define	 HDMI_FC_INVIDCONF_IN_I_P		__BIT(0)
#define	HDMI_FC_INHACTIV0	0x1001
#define	HDMI_FC_INHACTIV1	0x1002
#define	HDMI_FC_INHBLANK0	0x1003
#define	HDMI_FC_INHBLANK1	0x1004
#define	HDMI_FC_INVACTIV0	0x1005
#define	HDMI_FC_INVACTIV1	0x1006
#define	HDMI_FC_INVBLANK	0x1007
#define	HDMI_FC_HSYNCINDELAY0	0x1008
#define	HDMI_FC_HSYNCINDELAY1	0x1009
#define	HDMI_FC_HSYNCINWIDTH0	0x100a
#define	HDMI_FC_HSYNCINWIDTH1	0x100b
#define	HDMI_FC_VSYNCINDELAY	0x100c
#define	HDMI_FC_VSYNCINWIDTH	0x100d
#define	HDMI_FC_CTRLDUR		0x1011
#define	 HDMI_FC_CTRLDUR_DEFAULT		12
#define	HDMI_FC_EXCTRLDUR	0x1012
#define	 HDMI_FC_EXCTRLDUR_DEFAULT		32
#define	HDMI_FC_EXCTRLSPAC	0x1013
#define	 HDMI_FC_EXCTRLSPAC_DEFAULT		1
#define	HDMI_FC_CH0PREAM	0x1014
#define	 HDMI_FC_CH0PREAM_DEFAULT		0x0b
#define	HDMI_FC_CH1PREAM	0x1015
#define	 HDMI_FC_CH1PREAM_DEFAULT		0x16
#define	HDMI_FC_CH2PREAM	0x1016
#define	 HDMI_FC_CH2PREAM_DEFAULT		0x21
#define	HDMI_FC_AUDCONF0	0x1025
#define	HDMI_FC_AUDCONF1	0x1026
#define	HDMI_FC_AUDCONF2	0x1027
#define	HDMI_FC_AUDCONF3	0x1028

#define	HDMI_PHY_CONF0		0x3000
#define	 HDMI_PHY_CONF0_PDZ			__BIT(7)
#define	 HDMI_PHY_CONF0_ENTMDS			__BIT(6)
#define	 HDMI_PHY_CONF0_SVSRET			__BIT(5)
#define	 HDMI_PHY_CONF0_PDDQ			__BIT(4)
#define	 HDMI_PHY_CONF0_TXPWRON			__BIT(3)
#define	 HDMI_PHY_CONF0_ENHPDRXSENSE		__BIT(2)
#define	 HDMI_PHY_CONF0_SELDATAENPOL		__BIT(1)
#define	 HDMI_PHY_CONF0_SELDIPIF		__BIT(0)
#define	HDMI_PHY_STAT0		0x3004
#define	 HDMI_PHY_STAT0_RX_SENSE_3		__BIT(7)
#define	 HDMI_PHY_STAT0_RX_SENSE_2		__BIT(6)
#define	 HDMI_PHY_STAT0_RX_SENSE_1		__BIT(5)
#define	 HDMI_PHY_STAT0_RX_SENSE_0		__BIT(4)
#define	 HDMI_PHY_STAT0_HPD			__BIT(1)
#define	 HDMI_PHY_STAT0_TX_PHY_LOCK		__BIT(0)

#define	HDMI_AUD_CONF0		0x3100
#define	 HDMI_AUD_CONF0_SW_AUDIO_FIFO_RST	__BIT(7)
#define	 HDMI_AUD_CONF0_I2S_SELECT		__BIT(5)
#define	 HDMI_AUD_CONF0_I2S_IN_EN		__BITS(3,0)
#define	HDMI_AUD_CONF1		0x3101
#define	 HDMI_AUD_CONF1_I2S_WIDTH		__BITS(4,0)
#define	HDMI_AUD_INT		0x3102
#define	HDMI_AUD_CONF2		0x3103
#define	 HDMI_AUD_CONF2_INSERT_PCUV		__BIT(2)
#define	 HDMI_AUD_CONF2_NLPCM			__BIT(1)
#define	 HDMI_AUD_CONF2_HBR			__BIT(0)
#define	HDMI_AUD_INT1		0x3104

#define	HDMI_AUD_N1		0x3200
#define	HDMI_AUD_N2		0x3201
#define	HDMI_AUD_N3		0x3202
#define	HDMI_AUD_CTS1		0x3203
#define	HDMI_AUD_CTS2		0x3204
#define	HDMI_AUD_CTS3		0x3205
#define	HDMI_AUD_INPUTCLKFS	0x3206
#define	 HDMI_AUD_INPUTCLKFS_IFSFACTOR		__BITS(2,0)

#define	HDMI_MC_CLKDIS		0x4001
#define	 HDMI_MC_CLKDIS_HDCPCLK_DISABLE		__BIT(6)
#define	 HDMI_MC_CLKDIS_CECCLK_DISABLE		__BIT(5)
#define	 HDMI_MC_CLKDIS_CSCCLK_DISABLE		__BIT(4)
#define	 HDMI_MC_CLKDIS_AUDCLK_DISABLE		__BIT(3)
#define	 HDMI_MC_CLKDIS_PREPCLK_DISABLE		__BIT(2)
#define	 HDMI_MC_CLKDIS_TMDSCLK_DISABLE		__BIT(1)
#define	 HDMI_MC_CLKDIS_PIXELCLK_DISABLE	__BIT(0)
#define	HDMI_MC_SWRSTZREQ	0x4002
#define	 HDMI_MC_SWRSTZREQ_CECSWRST_REQ		__BIT(6)
#define	 HDMI_MC_SWRSTZREQ_PREPSWRST_REQ	__BIT(2)
#define	 HDMI_MC_SWRSTZREQ_TMDSSWRST_REQ	__BIT(1)
#define	 HDMI_MC_SWRSTZREQ_PIXELSWRST_REQ	__BIT(0)
#define	HDMI_MC_FLOWCTRL	0x4004
#define	HDMI_MC_PHYRSTZ		0x4005
#define	 HDMI_MC_PHYRSTZ_ASSERT			__BIT(0)
#define	 HDMI_MC_PHYRSTZ_DEASSERT		0
#define	HDMI_MC_LOCKONCLOCK	0x4006
#define	HDMI_MC_HEACPHY_RST	0x4007

#define	HDMI_I2CM_SLAVE		0x7e00
#define	HDMI_I2CM_ADDRESS	0x7e01
#define	HDMI_I2CM_DATAO		0x7e02
#define	HDMI_I2CM_DATAI		0x7e03
#define	HDMI_I2CM_OPERATION	0x7e04
#define	 HDMI_I2CM_OPERATION_WR			__BIT(4)
#define	 HDMI_I2CM_OPERATION_RD_EXT		__BIT(1)
#define	 HDMI_I2CM_OPERATION_RD			__BIT(0)
#define	HDMI_I2CM_INT		0x7e05
#define	 HDMI_I2CM_INT_DONE_POL			__BIT(3)
#define	 HDMI_I2CM_INT_DONE_MASK		__BIT(2)
#define	 HDMI_I2CM_INT_DONE_INTERRUPT		__BIT(1)
#define	 HDMI_I2CM_INT_DONE_STATUS		__BIT(0)
#define	 HDMI_I2CM_INT_DEFAULT			\
	(HDMI_I2CM_INT_DONE_POL|		\
	 HDMI_I2CM_INT_DONE_INTERRUPT|		\
	 HDMI_I2CM_INT_DONE_STATUS)
#define	HDMI_I2CM_CTLINT	0x7e06
#define	 HDMI_I2CM_CTLINT_NACK_POL		__BIT(7)
#define	 HDMI_I2CM_CTLINT_NACK_MASK		__BIT(6)
#define	 HDMI_I2CM_CTLINT_NACK_INTERRUPT	__BIT(5)
#define	 HDMI_I2CM_CTLINT_NACK_STATUS		__BIT(4)
#define	 HDMI_I2CM_CTLINT_ARB_POL		__BIT(3)
#define	 HDMI_I2CM_CTLINT_ARB_MASK		__BIT(2)
#define	 HDMI_I2CM_CTLINT_ARB_INTERRUPT		__BIT(1)
#define	 HDMI_I2CM_CTLINT_ARB_STATUS		__BIT(0)
#define	 HDMI_I2CM_CTLINT_DEFAULT		\
	(HDMI_I2CM_CTLINT_NACK_POL|		\
	 HDMI_I2CM_CTLINT_NACK_INTERRUPT|	\
	 HDMI_I2CM_CTLINT_NACK_STATUS|		\
	 HDMI_I2CM_CTLINT_ARB_POL|		\
	 HDMI_I2CM_CTLINT_ARB_INTERRUPT|	\
	 HDMI_I2CM_CTLINT_ARB_STATUS)
#define	HDMI_I2CM_DIV		0x7e07
#define	 HDMI_I2CM_DIV_FAST_STD_MODE		__BIT(3)
#define	HDMI_I2CM_SEGADDR	0x7e08
#define	 HDMI_I2CM_SEGADDR_SEGADDR		__BITS(6,0)
#define	HDMI_I2CM_SOFTRSTZ	0x7e09
#define	 HDMI_I2CM_SOFTRSTZ_I2C_SOFTRST		__BIT(0)
#define	HDMI_I2CM_SEGPTR	0x7e0a
#define	HDMI_I2CM_SS_SCL_HCNT_0_ADDR 0x730c
#define	HDMI_I2CM_SS_SCL_LCNT_0_ADDR 0x730e

enum dwhdmi_dai_mixer_ctrl {
	DWHDMI_DAI_OUTPUT_CLASS,
	DWHDMI_DAI_INPUT_CLASS,

	DWHDMI_DAI_OUTPUT_MASTER_VOLUME,
	DWHDMI_DAI_INPUT_DAC_VOLUME,

	DWHDMI_DAI_MIXER_CTRL_LAST
};

static int
dwhdmi_ddc_acquire_bus(void *priv, int flags)
{
	struct dwhdmi_softc * const sc = priv;

	mutex_enter(&sc->sc_ic_lock);

	return 0;
}

static void
dwhdmi_ddc_release_bus(void *priv, int flags)
{
	struct dwhdmi_softc * const sc = priv;

	mutex_exit(&sc->sc_ic_lock);
}

static int
dwhdmi_ddc_exec(void *priv, i2c_op_t op, i2c_addr_t addr,
    const void *cmdbuf, size_t cmdlen, void *buf, size_t len, int flags)
{
	struct dwhdmi_softc * const sc = priv;
	uint8_t block, operation, val;
	uint8_t *pbuf = buf;
	int off, n, retry;

	KASSERT(mutex_owned(&sc->sc_ic_lock));

	if (addr != DDC_ADDR || op != I2C_OP_READ_WITH_STOP || cmdlen == 0 || buf == NULL) {
		printf("dwhdmi_ddc_exec: bad args addr=%#x op=%#x cmdlen=%d buf=%p\n",
		    addr, op, (int)cmdlen, buf);
		return ENXIO;
	}
	if (len > 256) {
		printf("dwhdmi_ddc_exec: bad len %d\n", (int)len);
		return ERANGE;
	}

	dwhdmi_write(sc, HDMI_I2CM_SOFTRSTZ, 0);
	dwhdmi_write(sc, HDMI_IH_I2CM_STAT0, dwhdmi_read(sc, HDMI_IH_I2CM_STAT0));
	if (sc->sc_scl_hcnt)
		dwhdmi_write(sc, HDMI_I2CM_SS_SCL_HCNT_0_ADDR, sc->sc_scl_hcnt);
	if (sc->sc_scl_lcnt)
		dwhdmi_write(sc, HDMI_I2CM_SS_SCL_LCNT_0_ADDR, sc->sc_scl_lcnt);
	dwhdmi_write(sc, HDMI_I2CM_DIV, 0);
	dwhdmi_write(sc, HDMI_I2CM_SLAVE, DDC_ADDR);
	dwhdmi_write(sc, HDMI_I2CM_SEGADDR, DDC_SEGMENT_ADDR);

	block = *(const uint8_t *)cmdbuf;
	operation = block ? HDMI_I2CM_OPERATION_RD_EXT : HDMI_I2CM_OPERATION_RD;
	off = (block & 1) ? 128 : 0;

	dwhdmi_write(sc, HDMI_I2CM_SEGPTR, block >> 1);

	for (n = 0; n < len; n++) {
		dwhdmi_write(sc, HDMI_I2CM_ADDRESS, n + off);
		dwhdmi_write(sc, HDMI_I2CM_OPERATION, operation);
		for (retry = 10000; retry > 0; retry--) {
			val = dwhdmi_read(sc, HDMI_IH_I2CM_STAT0);
			if (val & HDMI_IH_I2CM_STAT0_ERROR) {
				return EIO;
			}
			if (val & HDMI_IH_I2CM_STAT0_DONE) {
				dwhdmi_write(sc, HDMI_IH_I2CM_STAT0, val);
				break;
			}
			delay(1);
		}
		if (retry == 0) {
			printf("dwhdmi_ddc_exec: timeout waiting for xfer, stat0=%#x\n", dwhdmi_read(sc, HDMI_IH_I2CM_STAT0));
			return ETIMEDOUT;
		}

		pbuf[n] = dwhdmi_read(sc, HDMI_I2CM_DATAI);
	}

	return 0;
}

uint8_t
dwhdmi_read(struct dwhdmi_softc *sc, bus_size_t reg)
{
	uint8_t val;

	switch (sc->sc_reg_width) {
	case 1:
		val = bus_space_read_1(sc->sc_bst, sc->sc_bsh, reg);
		break;
	case 4:
		val = bus_space_read_4(sc->sc_bst, sc->sc_bsh, reg * 4) & 0xff;
		break;
	default:
		val = 0;
		break;
	}

	return val;
}

void
dwhdmi_write(struct dwhdmi_softc *sc, bus_size_t reg, uint8_t val)
{
	switch (sc->sc_reg_width) {
	case 1:
		bus_space_write_1(sc->sc_bst, sc->sc_bsh, reg, val);
		break;
	case 4:
		bus_space_write_4(sc->sc_bst, sc->sc_bsh, reg * 4, val);
		break;
	}
}

static void
dwhdmi_vp_init(struct dwhdmi_softc *sc)
{
	uint8_t val;

	/* Select 24-bits per pixel video, 8-bit packing mode and disable pixel repetition */
	val = __SHIFTIN(HDMI_VP_PR_CD_COLOR_DEPTH_24, HDMI_VP_PR_CD_COLOR_DEPTH) |
	      __SHIFTIN(HDMI_VP_PR_CD_DESIRED_PR_FACTOR_NONE, HDMI_VP_PR_CD_DESIRED_PR_FACTOR);
	dwhdmi_write(sc, HDMI_VP_PR_CD, val);

	/* Configure stuffing */
	val = HDMI_VP_STUFF_IDEFAULT_PHASE |
	      HDMI_VP_STUFF_YCC422_STUFFING |
	      HDMI_VP_STUFF_PP_STUFFING |
	      HDMI_VP_STUFF_PR_STUFFING;
	dwhdmi_write(sc, HDMI_VP_STUFF, val);

	/* Set YCC422 remap to 16-bit input video */
	val = __SHIFTIN(HDMI_VP_REMAP_YCC422_SIZE_16, HDMI_VP_REMAP_YCC422_SIZE);
	dwhdmi_write(sc, HDMI_VP_REMAP, val);

	/* Configure video packetizer */
	val = HDMI_VP_CONF_BYPASS_EN |
	      HDMI_VP_CONF_BYPASS_SELECT |
	      __SHIFTIN(HDMI_VP_CONF_OUTPUT_SELECT_BYPASS, HDMI_VP_CONF_OUTPUT_SELECT);
	dwhdmi_write(sc, HDMI_VP_CONF, val);
}

static void
dwhdmi_tx_init(struct dwhdmi_softc *sc)
{
	uint8_t val;

	/* Disable internal data enable generator and set default video mapping */
	val = __SHIFTIN(HDMI_TX_INVID0_VIDEO_MAPPING_DEFAULT, HDMI_TX_INVID0_VIDEO_MAPPING);
	dwhdmi_write(sc, HDMI_TX_INVID0, val);

	/* Enable video sampler stuffing */
	val = HDMI_TX_INSTUFFING_BCBDATA_STUFFING |
	      HDMI_TX_INSTUFFING_RCRDATA_STUFFING |
	      HDMI_TX_INSTUFFING_GYDATA_STUFFING;
	dwhdmi_write(sc, HDMI_TX_INSTUFFING, val);
}

static bool
dwhdmi_cea_mode_uses_fractional_vblank(uint8_t vic)
{
	const uint8_t match[] = { 5, 6, 7, 10, 11, 20, 21, 22 };
	u_int n;

	for (n = 0; n < __arraycount(match); n++)
		if (match[n] == vic)
			return true;

	return false;
}

static void
dwhdmi_fc_init(struct dwhdmi_softc *sc, struct drm_display_mode *mode)
{
	struct dwhdmi_connector *dwhdmi_connector = &sc->sc_connector;
	uint8_t val;

	const uint8_t vic = drm_match_cea_mode(mode);
	const uint16_t inhactiv = mode->crtc_hdisplay;
	const uint16_t inhblank = mode->crtc_htotal - mode->crtc_hdisplay;
	const uint16_t invactiv = mode->crtc_vdisplay;
	const uint8_t invblank = mode->crtc_vtotal - mode->crtc_vdisplay;
	const uint16_t hsyncindelay = mode->crtc_hsync_start - mode->crtc_hdisplay;
	const uint16_t hsyncinwidth = mode->crtc_hsync_end - mode->crtc_hsync_start;
	const uint8_t vsyncindelay = mode->crtc_vsync_start - mode->crtc_vdisplay;
	const uint8_t vsyncinwidth = mode->crtc_vsync_end - mode->crtc_vsync_start;

	/* Input video configuration for frame composer */
	val = HDMI_FC_INVIDCONF_DE_IN_POLARITY;
	if ((mode->flags & DRM_MODE_FLAG_PVSYNC) != 0)
		val |= HDMI_FC_INVIDCONF_VSYNC_IN_POLARITY;
	if ((mode->flags & DRM_MODE_FLAG_PHSYNC) != 0)
		val |= HDMI_FC_INVIDCONF_HSYNC_IN_POLARITY;
	if ((mode->flags & DRM_MODE_FLAG_INTERLACE) != 0)
		val |= HDMI_FC_INVIDCONF_IN_I_P;
	if (dwhdmi_connector->hdmi_monitor)
		val |= HDMI_FC_INVIDCONF_DVI_MODE;
	if (dwhdmi_cea_mode_uses_fractional_vblank(vic))
		val |= HDMI_FC_INVIDCONF_R_V_BLANK_IN_OSC;
	dwhdmi_write(sc, HDMI_FC_INVIDCONF, val);

	/* Input video mode timings */
	dwhdmi_write(sc, HDMI_FC_INHACTIV0, inhactiv & 0xff);
	dwhdmi_write(sc, HDMI_FC_INHACTIV1, inhactiv >> 8);
	dwhdmi_write(sc, HDMI_FC_INHBLANK0, inhblank & 0xff);
	dwhdmi_write(sc, HDMI_FC_INHBLANK1, inhblank >> 8);
	dwhdmi_write(sc, HDMI_FC_INVACTIV0, invactiv & 0xff);
	dwhdmi_write(sc, HDMI_FC_INVACTIV1, invactiv >> 8);
	dwhdmi_write(sc, HDMI_FC_INVBLANK, invblank);
	dwhdmi_write(sc, HDMI_FC_HSYNCINDELAY0, hsyncindelay & 0xff);
	dwhdmi_write(sc, HDMI_FC_HSYNCINDELAY1, hsyncindelay >> 8);
	dwhdmi_write(sc, HDMI_FC_HSYNCINWIDTH0, hsyncinwidth & 0xff);
	dwhdmi_write(sc, HDMI_FC_HSYNCINWIDTH1, hsyncinwidth >> 8);
	dwhdmi_write(sc, HDMI_FC_VSYNCINDELAY, vsyncindelay);
	dwhdmi_write(sc, HDMI_FC_VSYNCINWIDTH, vsyncinwidth);

	/* Setup control period minimum durations */
	dwhdmi_write(sc, HDMI_FC_CTRLDUR, HDMI_FC_CTRLDUR_DEFAULT);
	dwhdmi_write(sc, HDMI_FC_EXCTRLDUR, HDMI_FC_EXCTRLDUR_DEFAULT);
	dwhdmi_write(sc, HDMI_FC_EXCTRLSPAC, HDMI_FC_EXCTRLSPAC_DEFAULT);

	/* Setup channel preamble filters */
	dwhdmi_write(sc, HDMI_FC_CH0PREAM, HDMI_FC_CH0PREAM_DEFAULT);
	dwhdmi_write(sc, HDMI_FC_CH1PREAM, HDMI_FC_CH1PREAM_DEFAULT);
	dwhdmi_write(sc, HDMI_FC_CH2PREAM, HDMI_FC_CH2PREAM_DEFAULT);
}

static void
dwhdmi_mc_init(struct dwhdmi_softc *sc)
{
	uint8_t val;
	u_int n, iter;

	/* Bypass colour space converter */
	dwhdmi_write(sc, HDMI_MC_FLOWCTRL, 0);

	/* Enable TMDS, pixel, and (if required) audio sampler clocks */
	val = HDMI_MC_CLKDIS_HDCPCLK_DISABLE |
	      HDMI_MC_CLKDIS_CECCLK_DISABLE |
	      HDMI_MC_CLKDIS_CSCCLK_DISABLE |
	      HDMI_MC_CLKDIS_PREPCLK_DISABLE;
	dwhdmi_write(sc, HDMI_MC_CLKDIS, val);

	/* Soft reset TMDS */
	val = 0xff & ~HDMI_MC_SWRSTZREQ_TMDSSWRST_REQ;
	dwhdmi_write(sc, HDMI_MC_SWRSTZREQ, val);

	iter = sc->sc_version == 0x130a ? 4 : 1;

	val = dwhdmi_read(sc, HDMI_FC_INVIDCONF);
	for (n = 0; n < iter; n++)
		dwhdmi_write(sc, HDMI_FC_INVIDCONF, val);
}

static void
dwhdmi_mc_disable(struct dwhdmi_softc *sc)
{
	/* Disable clocks */
	dwhdmi_write(sc, HDMI_MC_CLKDIS, 0xff);
}

static void
dwhdmi_audio_init(struct dwhdmi_softc *sc)
{
	uint8_t val;
	u_int n;

	/* The following values are for 48 kHz */
	switch (sc->sc_curmode.clock) {
	case 25170:
		n = 6864;
		break;
	case 74170:
		n = 11648;
		break;
	case 148350:
		n = 5824;
		break;
	default:
		n = 6144;
		break;
	}

	/* Use automatic CTS generation */
	dwhdmi_write(sc, HDMI_AUD_CTS1, 0);
	dwhdmi_write(sc, HDMI_AUD_CTS2, 0);
	dwhdmi_write(sc, HDMI_AUD_CTS3, 0);

	/* Set N factor for audio clock regeneration */
	dwhdmi_write(sc, HDMI_AUD_N1, n & 0xff);
	dwhdmi_write(sc, HDMI_AUD_N2, (n >> 8) & 0xff);
	dwhdmi_write(sc, HDMI_AUD_N3, (n >> 16) & 0xff);

	val = dwhdmi_read(sc, HDMI_AUD_CONF0);
	val |= HDMI_AUD_CONF0_I2S_SELECT;		/* XXX i2s mode */
	val &= ~HDMI_AUD_CONF0_I2S_IN_EN;
	val |= __SHIFTIN(1, HDMI_AUD_CONF0_I2S_IN_EN);	/* XXX 2ch */
	dwhdmi_write(sc, HDMI_AUD_CONF0, val);
	
	val = __SHIFTIN(16, HDMI_AUD_CONF1_I2S_WIDTH);
	dwhdmi_write(sc, HDMI_AUD_CONF1, val);

	dwhdmi_write(sc, HDMI_AUD_INPUTCLKFS, 4);	/* XXX 64 FS */

	dwhdmi_write(sc, HDMI_FC_AUDCONF0, 1 << 4);	/* XXX 2ch */
	dwhdmi_write(sc, HDMI_FC_AUDCONF1, 0);
	dwhdmi_write(sc, HDMI_FC_AUDCONF2, 0);
	dwhdmi_write(sc, HDMI_FC_AUDCONF3, 0);

	val = dwhdmi_read(sc, HDMI_MC_CLKDIS);
	val &= ~HDMI_MC_CLKDIS_PREPCLK_DISABLE;
	dwhdmi_write(sc, HDMI_MC_CLKDIS, val);
}

static enum drm_connector_status
dwhdmi_connector_detect(struct drm_connector *connector, bool force)
{
	struct dwhdmi_connector *dwhdmi_connector = to_dwhdmi_connector(connector);
	struct dwhdmi_softc * const sc = dwhdmi_connector->sc;

	if (sc->sc_detect != NULL)
		return sc->sc_detect(sc, force);

	return connector_status_connected;
}

static void
dwhdmi_connector_destroy(struct drm_connector *connector)
{
	drm_connector_unregister(connector);
	drm_connector_cleanup(connector);
}

static const struct drm_connector_funcs dwhdmi_connector_funcs = {
	.dpms = drm_helper_connector_dpms,
	.detect = dwhdmi_connector_detect,
	.fill_modes = drm_helper_probe_single_connector_modes,
	.destroy = dwhdmi_connector_destroy,
};

static int
dwhdmi_connector_get_modes(struct drm_connector *connector)
{
	struct dwhdmi_connector *dwhdmi_connector = to_dwhdmi_connector(connector);
	struct dwhdmi_softc * const sc = dwhdmi_connector->sc;
	char edid[EDID_LENGTH * 4];
	struct edid *pedid = NULL;
	int error, block;

	memset(edid, 0, sizeof(edid));
	for (block = 0; block < 4; block++) {
		error = ddc_read_edid_block(sc->sc_ic,
		    &edid[block * EDID_LENGTH], EDID_LENGTH, block);
		if (error != 0)
			break;
		if (block == 0) {
			pedid = (struct edid *)edid;
			if (edid[0x7e] == 0)
				break;
		}
	}

	if (pedid) {
		dwhdmi_connector->hdmi_monitor = drm_detect_hdmi_monitor(pedid);
		dwhdmi_connector->monitor_audio = drm_detect_monitor_audio(pedid);
	} else {
		dwhdmi_connector->hdmi_monitor = false;
		dwhdmi_connector->monitor_audio = false;
	}

	drm_mode_connector_update_edid_property(connector, pedid);
	if (pedid == NULL)
		return 0;

	error = drm_add_edid_modes(connector, pedid);
	drm_edid_to_eld(connector, pedid);

	return error;
}

static struct drm_encoder *
dwhdmi_connector_best_encoder(struct drm_connector *connector)
{
	int enc_id = connector->encoder_ids[0];
	struct drm_mode_object *obj;
	struct drm_encoder *encoder = NULL;

	if (enc_id) {
		obj = drm_mode_object_find(connector->dev, enc_id,
		    DRM_MODE_OBJECT_ENCODER);
		if (obj == NULL)
			return NULL;
		encoder = obj_to_encoder(obj);
	}

	return encoder;
}

static const struct drm_connector_helper_funcs dwhdmi_connector_helper_funcs = {
	.get_modes = dwhdmi_connector_get_modes,
	.best_encoder = dwhdmi_connector_best_encoder,
};

static int
dwhdmi_bridge_attach(struct drm_bridge *bridge)
{
	struct dwhdmi_softc * const sc = bridge->driver_private;
	struct dwhdmi_connector *dwhdmi_connector = &sc->sc_connector;
	struct drm_connector *connector = &dwhdmi_connector->base;
	int error;

	dwhdmi_connector->sc = sc;

	connector->polled = DRM_CONNECTOR_POLL_CONNECT | DRM_CONNECTOR_POLL_DISCONNECT;
	connector->interlace_allowed = 0;
	connector->doublescan_allowed = 0;

	drm_connector_init(bridge->dev, connector, &dwhdmi_connector_funcs,
	    DRM_MODE_CONNECTOR_HDMIA);
	drm_connector_helper_add(connector, &dwhdmi_connector_helper_funcs);

	error = drm_mode_connector_attach_encoder(connector, bridge->encoder);
	if (error != 0)
		return error;

	return drm_connector_register(connector);
}

static void
dwhdmi_bridge_enable(struct drm_bridge *bridge)
{
	struct dwhdmi_softc * const sc = bridge->driver_private;

	dwhdmi_vp_init(sc);
	dwhdmi_fc_init(sc, &sc->sc_curmode);

	if (sc->sc_enable)
		sc->sc_enable(sc);

	dwhdmi_tx_init(sc);
	dwhdmi_mc_init(sc);

	if (sc->sc_connector.monitor_audio)
		dwhdmi_audio_init(sc);
}

static void
dwhdmi_bridge_pre_enable(struct drm_bridge *bridge)
{
}

static void
dwhdmi_bridge_disable(struct drm_bridge *bridge)
{
	struct dwhdmi_softc * const sc = bridge->driver_private;

	if (sc->sc_disable)
		sc->sc_disable(sc);

	dwhdmi_mc_disable(sc);
}

static void
dwhdmi_bridge_post_disable(struct drm_bridge *bridge)
{
}

static void
dwhdmi_bridge_mode_set(struct drm_bridge *bridge,
    struct drm_display_mode *mode, struct drm_display_mode *adjusted_mode)
{
	struct dwhdmi_softc * const sc = bridge->driver_private;

	if (sc->sc_mode_set)
		sc->sc_mode_set(sc, mode, adjusted_mode);

	sc->sc_curmode = *adjusted_mode;
}

static bool
dwhdmi_bridge_mode_fixup(struct drm_bridge *bridge,
    const struct drm_display_mode *mode, struct drm_display_mode *adjusted_mode)
{
	return true;
}

static const struct drm_bridge_funcs dwhdmi_bridge_funcs = {
	.attach = dwhdmi_bridge_attach,
	.enable = dwhdmi_bridge_enable,
	.pre_enable = dwhdmi_bridge_pre_enable,
	.disable = dwhdmi_bridge_disable,
	.post_disable = dwhdmi_bridge_post_disable,
	.mode_set = dwhdmi_bridge_mode_set,
	.mode_fixup = dwhdmi_bridge_mode_fixup,
};

static int
dwhdmi_dai_set_format(audio_dai_tag_t dai, u_int format)
{
	return 0;
}

static int
dwhdmi_dai_add_device(audio_dai_tag_t dai, audio_dai_tag_t aux)
{
	/* Not supported */
	return 0;
}

static void
dwhdmi_audio_swvol_codec(audio_filter_arg_t *arg)
{
	struct dwhdmi_softc * const sc = arg->context;
	const aint_t *src;
	aint_t *dst;
	u_int sample_count;
	u_int i;

	src = arg->src;
	dst = arg->dst;
	sample_count = arg->count * arg->srcfmt->channels;
	for (i = 0; i < sample_count; i++) {
		aint2_t v = (aint2_t)(*src++);
		v = v * sc->sc_swvol / 255;
		*dst++ = (aint_t)v;
	}
}

static int
dwhdmi_audio_set_format(void *priv, int setmode,
    const audio_params_t *play, const audio_params_t *rec,
    audio_filter_reg_t *pfil, audio_filter_reg_t *rfil)
{
	struct dwhdmi_softc * const sc = priv;

	pfil->codec = dwhdmi_audio_swvol_codec;
	pfil->context = sc;

	return 0;
}

static int
dwhdmi_audio_set_port(void *priv, mixer_ctrl_t *mc)
{
	struct dwhdmi_softc * const sc = priv;

	switch (mc->dev) {
	case DWHDMI_DAI_OUTPUT_MASTER_VOLUME:
	case DWHDMI_DAI_INPUT_DAC_VOLUME:
		sc->sc_swvol = mc->un.value.level[AUDIO_MIXER_LEVEL_LEFT];
		return 0;
	default:
		return ENXIO;
	}
}

static int
dwhdmi_audio_get_port(void *priv, mixer_ctrl_t *mc)
{
	struct dwhdmi_softc * const sc = priv;

	switch (mc->dev) {
	case DWHDMI_DAI_OUTPUT_MASTER_VOLUME:
	case DWHDMI_DAI_INPUT_DAC_VOLUME:
		mc->un.value.level[AUDIO_MIXER_LEVEL_LEFT] = sc->sc_swvol;
		mc->un.value.level[AUDIO_MIXER_LEVEL_RIGHT] = sc->sc_swvol;
		return 0;
	default:
		return ENXIO;
	}
}

static int
dwhdmi_audio_query_devinfo(void *priv, mixer_devinfo_t *di)
{
	switch (di->index) {
	case DWHDMI_DAI_OUTPUT_CLASS:
		di->mixer_class = di->index;
		strcpy(di->label.name, AudioCoutputs);
		di->type = AUDIO_MIXER_CLASS;
		di->next = di->prev = AUDIO_MIXER_LAST;
		return 0;

	case DWHDMI_DAI_INPUT_CLASS:
		di->mixer_class = di->index;
		strcpy(di->label.name, AudioCinputs);
		di->type = AUDIO_MIXER_CLASS;
		di->next = di->prev = AUDIO_MIXER_LAST;
		return 0;

	case DWHDMI_DAI_OUTPUT_MASTER_VOLUME:
		di->mixer_class = DWHDMI_DAI_OUTPUT_CLASS;
		strcpy(di->label.name, AudioNmaster);
		di->un.v.delta = 1;
		di->un.v.num_channels = 2;
		strcpy(di->un.v.units.name, AudioNvolume);
		di->type = AUDIO_MIXER_VALUE;
		di->next = di->prev = AUDIO_MIXER_LAST;
		return 0;

	case DWHDMI_DAI_INPUT_DAC_VOLUME:
		di->mixer_class = DWHDMI_DAI_INPUT_CLASS;
		strcpy(di->label.name, AudioNdac);
		di->un.v.delta = 1;
		di->un.v.num_channels = 2;
		strcpy(di->un.v.units.name, AudioNvolume);
		di->type = AUDIO_MIXER_VALUE;
		di->next = di->prev = AUDIO_MIXER_LAST;
		return 0;

	default:
		return ENXIO;
	}
}

static const struct audio_hw_if dwhdmi_dai_hw_if = {
	.set_format = dwhdmi_audio_set_format,
	.set_port = dwhdmi_audio_set_port,
	.get_port = dwhdmi_audio_get_port,
	.query_devinfo = dwhdmi_audio_query_devinfo,
};

int
dwhdmi_attach(struct dwhdmi_softc *sc)
{
	uint8_t val;

	if (sc->sc_reg_width != 1 && sc->sc_reg_width != 4) {
		aprint_error_dev(sc->sc_dev, "unsupported register width %d\n", sc->sc_reg_width);
		return EINVAL;
	}

	mutex_init(&sc->sc_ic_lock, MUTEX_DEFAULT, IPL_NONE);

	sc->sc_version = dwhdmi_read(sc, HDMI_DESIGN_ID);
	sc->sc_version <<= 8;
	sc->sc_version |= dwhdmi_read(sc, HDMI_REVISION_ID);

	sc->sc_phytype = dwhdmi_read(sc, HDMI_CONFIG2_ID);

	aprint_normal_dev(sc->sc_dev, "version %x.%03x, phytype 0x%02x\n",
	    sc->sc_version >> 12, sc->sc_version & 0xfff,
	    sc->sc_phytype);

	sc->sc_swvol = 255;

	/*
	 * If a DDC i2c bus tag is provided by the caller, use it. Otherwise,
	 * use the I2C master built-in to DWC HDMI.
	 */
	if (sc->sc_ic == NULL) {
		struct i2c_controller *ic = &sc->sc_ic_builtin;
		ic->ic_cookie = sc;
		ic->ic_acquire_bus = dwhdmi_ddc_acquire_bus;
		ic->ic_release_bus = dwhdmi_ddc_release_bus;
		ic->ic_exec = dwhdmi_ddc_exec;
		sc->sc_ic = ic;
	}

	/*
	 * Enable HPD on internal PHY
	 */
	if ((sc->sc_flags & DWHDMI_USE_INTERNAL_PHY) != 0) {
		val = dwhdmi_read(sc, HDMI_PHY_CONF0);
		val |= HDMI_PHY_CONF0_ENHPDRXSENSE;
		dwhdmi_write(sc, HDMI_PHY_CONF0, val);
	}

	/*
	 * Initialize audio DAI
	 */
	sc->sc_dai.dai_set_format = dwhdmi_dai_set_format;
	sc->sc_dai.dai_add_device = dwhdmi_dai_add_device;
	sc->sc_dai.dai_hw_if = &dwhdmi_dai_hw_if;
	sc->sc_dai.dai_dev = sc->sc_dev;
	sc->sc_dai.dai_priv = sc;

	return 0;
}

int
dwhdmi_bind(struct dwhdmi_softc *sc, struct drm_encoder *encoder)
{
	int error;

	sc->sc_bridge.driver_private = sc;
	sc->sc_bridge.funcs = &dwhdmi_bridge_funcs;
	sc->sc_bridge.encoder = encoder;

	error = drm_bridge_attach(encoder->dev, &sc->sc_bridge);
	if (error != 0)
		return EIO;

	encoder->bridge = &sc->sc_bridge;

	return 0;
}
