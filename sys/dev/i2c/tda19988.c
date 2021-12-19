/* $NetBSD: tda19988.c,v 1.8 2021/12/19 12:44:34 riastradh Exp $ */

/*-
 * Copyright (c) 2015 Oleksandr Tymoshenko <gonzo@freebsd.org>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tda19988.c,v 1.8 2021/12/19 12:44:34 riastradh Exp $");

/*
* NXP TDA19988 HDMI encoder
*/
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/time.h>
#include <sys/bus.h>
#include <sys/types.h>

#include <dev/i2c/i2cvar.h>
#include <dev/i2c/ddcvar.h>
#include <dev/i2c/ddcreg.h>

#include <dev/fdt/fdtvar.h>
#include <dev/fdt/fdt_port.h>

#include <dev/videomode/videomode.h>
#include <dev/videomode/edidvar.h>

#include <drm/drm_bridge.h>
#include <drm/drm_crtc.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_drv.h>
#include <drm/drm_edid.h>
#include <drm/drm_probe_helper.h>

enum {
	TDA19988_PORT_INPUT = 0
};

#define	MKREG(page, addr)	(((page) << 8) | (addr))

#define	REGPAGE(reg)		(((reg) >> 8) & 0xff)
#define	REGADDR(reg)		((reg) & 0xff)

#define TDA_VERSION		MKREG(0x00, 0x00)
#define TDA_MAIN_CNTRL0		MKREG(0x00, 0x01)
#define 	MAIN_CNTRL0_SR		(1 << 0)
#define TDA_VERSION_MSB		MKREG(0x00, 0x02)
#define	TDA_SOFTRESET		MKREG(0x00, 0x0a)
#define		SOFTRESET_I2C		(1 << 1)
#define		SOFTRESET_AUDIO		(1 << 0)
#define	TDA_DDC_CTRL		MKREG(0x00, 0x0b)
#define		DDC_ENABLE		0
#define	TDA_CCLK		MKREG(0x00, 0x0c)
#define		CCLK_ENABLE		1
#define	TDA_INT_FLAGS_2		MKREG(0x00, 0x11)
#define		INT_FLAGS_2_EDID_BLK_RD	(1 << 1)

#define	TDA_VIP_CNTRL_0		MKREG(0x00, 0x20)
#define	TDA_VIP_CNTRL_1		MKREG(0x00, 0x21)
#define	TDA_VIP_CNTRL_2		MKREG(0x00, 0x22)
#define	TDA_VIP_CNTRL_3		MKREG(0x00, 0x23)
#define		VIP_CNTRL_3_SYNC_HS	(2 << 4)
#define		VIP_CNTRL_3_V_TGL	(1 << 2)
#define		VIP_CNTRL_3_H_TGL	(1 << 1)

#define	TDA_VIP_CNTRL_4		MKREG(0x00, 0x24)
#define		VIP_CNTRL_4_BLANKIT_NDE		(0 << 2)
#define		VIP_CNTRL_4_BLANKIT_HS_VS	(1 << 2)
#define		VIP_CNTRL_4_BLANKIT_NHS_VS	(2 << 2)
#define		VIP_CNTRL_4_BLANKIT_HE_VE	(3 << 2)
#define		VIP_CNTRL_4_BLC_NONE		(0 << 0)
#define		VIP_CNTRL_4_BLC_RGB444		(1 << 0)
#define		VIP_CNTRL_4_BLC_YUV444		(2 << 0)
#define		VIP_CNTRL_4_BLC_YUV422		(3 << 0)
#define	TDA_VIP_CNTRL_5		MKREG(0x00, 0x25)
#define		VIP_CNTRL_5_SP_CNT(n)	(((n) & 3) << 1)
#define	TDA_MUX_VP_VIP_OUT	MKREG(0x00, 0x27)
#define TDA_MAT_CONTRL		MKREG(0x00, 0x80)
#define		MAT_CONTRL_MAT_BP	(1 << 2)
#define	TDA_VIDFORMAT		MKREG(0x00, 0xa0)
#define	TDA_REFPIX_MSB		MKREG(0x00, 0xa1)
#define	TDA_REFPIX_LSB		MKREG(0x00, 0xa2)
#define	TDA_REFLINE_MSB		MKREG(0x00, 0xa3)
#define	TDA_REFLINE_LSB		MKREG(0x00, 0xa4)
#define	TDA_NPIX_MSB		MKREG(0x00, 0xa5)
#define	TDA_NPIX_LSB		MKREG(0x00, 0xa6)
#define	TDA_NLINE_MSB		MKREG(0x00, 0xa7)
#define	TDA_NLINE_LSB		MKREG(0x00, 0xa8)
#define	TDA_VS_LINE_STRT_1_MSB	MKREG(0x00, 0xa9)
#define	TDA_VS_LINE_STRT_1_LSB	MKREG(0x00, 0xaa)
#define	TDA_VS_PIX_STRT_1_MSB	MKREG(0x00, 0xab)
#define	TDA_VS_PIX_STRT_1_LSB	MKREG(0x00, 0xac)
#define	TDA_VS_LINE_END_1_MSB	MKREG(0x00, 0xad)
#define	TDA_VS_LINE_END_1_LSB	MKREG(0x00, 0xae)
#define	TDA_VS_PIX_END_1_MSB	MKREG(0x00, 0xaf)
#define	TDA_VS_PIX_END_1_LSB	MKREG(0x00, 0xb0)
#define	TDA_VS_LINE_STRT_2_MSB	MKREG(0x00, 0xb1)
#define	TDA_VS_LINE_STRT_2_LSB	MKREG(0x00, 0xb2)
#define	TDA_VS_PIX_STRT_2_MSB	MKREG(0x00, 0xb3)
#define	TDA_VS_PIX_STRT_2_LSB	MKREG(0x00, 0xb4)
#define	TDA_VS_LINE_END_2_MSB	MKREG(0x00, 0xb5)
#define	TDA_VS_LINE_END_2_LSB	MKREG(0x00, 0xb6)
#define	TDA_VS_PIX_END_2_MSB	MKREG(0x00, 0xb7)
#define	TDA_VS_PIX_END_2_LSB	MKREG(0x00, 0xb8)
#define	TDA_HS_PIX_START_MSB	MKREG(0x00, 0xb9)
#define	TDA_HS_PIX_START_LSB	MKREG(0x00, 0xba)
#define	TDA_HS_PIX_STOP_MSB	MKREG(0x00, 0xbb)
#define	TDA_HS_PIX_STOP_LSB	MKREG(0x00, 0xbc)
#define	TDA_VWIN_START_1_MSB	MKREG(0x00, 0xbd)
#define	TDA_VWIN_START_1_LSB	MKREG(0x00, 0xbe)
#define	TDA_VWIN_END_1_MSB	MKREG(0x00, 0xbf)
#define	TDA_VWIN_END_1_LSB	MKREG(0x00, 0xc0)
#define	TDA_VWIN_START_2_MSB	MKREG(0x00, 0xc1)
#define	TDA_VWIN_START_2_LSB	MKREG(0x00, 0xc2)
#define	TDA_VWIN_END_2_MSB	MKREG(0x00, 0xc3)
#define	TDA_VWIN_END_2_LSB	MKREG(0x00, 0xc4)
#define	TDA_DE_START_MSB	MKREG(0x00, 0xc5)
#define	TDA_DE_START_LSB	MKREG(0x00, 0xc6)
#define	TDA_DE_STOP_MSB		MKREG(0x00, 0xc7)
#define	TDA_DE_STOP_LSB		MKREG(0x00, 0xc8)

#define	TDA_TBG_CNTRL_0		MKREG(0x00, 0xca)
#define		TBG_CNTRL_0_SYNC_ONCE	(1 << 7)
#define		TBG_CNTRL_0_SYNC_MTHD	(1 << 6)

#define	TDA_TBG_CNTRL_1		MKREG(0x00, 0xcb)
#define		TBG_CNTRL_1_DWIN_DIS	(1 << 6)
#define		TBG_CNTRL_1_TGL_EN	(1 << 2)
#define		TBG_CNTRL_1_V_TGL	(1 << 1)
#define		TBG_CNTRL_1_H_TGL	(1 << 0)

#define	TDA_HVF_CNTRL_0		MKREG(0x00, 0xe4)
#define		HVF_CNTRL_0_PREFIL_NONE		(0 << 2)
#define		HVF_CNTRL_0_INTPOL_BYPASS	(0 << 0)
#define	TDA_HVF_CNTRL_1		MKREG(0x00, 0xe5)
#define		HVF_CNTRL_1_VQR(x)	(((x) & 3) << 2)
#define		HVF_CNTRL_1_VQR_FULL	HVF_CNTRL_1_VQR(0)
#define	TDA_ENABLE_SPACE	MKREG(0x00, 0xd6)
#define	TDA_RPT_CNTRL		MKREG(0x00, 0xf0)

#define	TDA_PLL_SERIAL_1	MKREG(0x02, 0x00)
#define		PLL_SERIAL_1_SRL_MAN_IP	(1 << 6)
#define	TDA_PLL_SERIAL_2	MKREG(0x02, 0x01)
#define		PLL_SERIAL_2_SRL_PR(x)		(((x) & 0xf) << 4)
#define		PLL_SERIAL_2_SRL_NOSC(x)	(((x) & 0x3) << 0)
#define	TDA_PLL_SERIAL_3	MKREG(0x02, 0x02)
#define		PLL_SERIAL_3_SRL_PXIN_SEL	(1 << 4)
#define		PLL_SERIAL_3_SRL_DE		(1 << 2)
#define		PLL_SERIAL_3_SRL_CCIR		(1 << 0)
#define	TDA_SERIALIZER		MKREG(0x02, 0x03)
#define	TDA_BUFFER_OUT		MKREG(0x02, 0x04)
#define	TDA_PLL_SCG1		MKREG(0x02, 0x05)
#define	TDA_PLL_SCG2		MKREG(0x02, 0x06)
#define	TDA_PLL_SCGN1		MKREG(0x02, 0x07)
#define	TDA_PLL_SCGN2		MKREG(0x02, 0x08)
#define	TDA_PLL_SCGR1		MKREG(0x02, 0x09)
#define	TDA_PLL_SCGR2		MKREG(0x02, 0x0a)

#define	TDA_SEL_CLK		MKREG(0x02, 0x11)
#define		SEL_CLK_ENA_SC_CLK	(1 << 3)
#define		SEL_CLK_SEL_VRF_CLK(x)	(((x) & 3) << 1)
#define		SEL_CLK_SEL_CLK1	(1 << 0)
#define	TDA_ANA_GENERAL		MKREG(0x02, 0x12)

#define	TDA_EDID_DATA0		MKREG(0x09, 0x00)
#define	TDA_EDID_CTRL		MKREG(0x09, 0xfa)
#define	TDA_DDC_ADDR		MKREG(0x09, 0xfb)
#define	TDA_DDC_OFFS		MKREG(0x09, 0xfc)
#define	TDA_DDC_SEGM_ADDR	MKREG(0x09, 0xfd)
#define	TDA_DDC_SEGM		MKREG(0x09, 0xfe)

#define	TDA_IF_VSP		MKREG(0x10, 0x20)
#define	TDA_IF_AVI		MKREG(0x10, 0x40)
#define	TDA_IF_SPD		MKREG(0x10, 0x60)
#define	TDA_IF_AUD		MKREG(0x10, 0x80)
#define	TDA_IF_MPS		MKREG(0x10, 0xa0)

#define	TDA_ENC_CNTRL		MKREG(0x11, 0x0d)
#define		ENC_CNTRL_DVI_MODE	(0 << 2)
#define		ENC_CNTRL_HDMI_MODE	(1 << 2)
#define	TDA_DIP_IF_FLAGS	MKREG(0x11, 0x0f)
#define		DIP_IF_FLAGS_IF5	(1 << 5)
#define		DIP_IF_FLAGS_IF4	(1 << 4)
#define		DIP_IF_FLAGS_IF3	(1 << 3)
#define		DIP_IF_FLAGS_IF2	(1 << 2) /* AVI IF on page 10h */
#define		DIP_IF_FLAGS_IF1	(1 << 1)

#define	TDA_TX3			MKREG(0x12, 0x9a)
#define	TDA_TX4			MKREG(0x12, 0x9b)
#define		TX4_PD_RAM		(1 << 1)
#define	TDA_HDCP_TX33		MKREG(0x12, 0xb8)
#define		HDCP_TX33_HDMI		(1 << 1)

#define	TDA_CURPAGE_ADDR	0xff

#define	TDA_CEC_RXSHPDLEV	0xfe
#define		RXSHPDLEV_HPD	__BIT(1)

#define	TDA_CEC_ENAMODS		0xff
#define		ENAMODS_RXSENS		(1 << 2)
#define		ENAMODS_HDMI		(1 << 1)
#define	TDA_CEC_FRO_IM_CLK_CTRL	0xfb
#define		CEC_FRO_IM_CLK_CTRL_GHOST_DIS	(1 << 7)
#define		CEC_FRO_IM_CLK_CTRL_IMCLK_SEL	(1 << 1)

/* EDID reading */ 
#define	MAX_READ_ATTEMPTS	100

/* EDID fields */
#define	EDID_MODES0		35
#define	EDID_MODES1		36
#define	EDID_TIMING_START	38
#define	EDID_TIMING_END		54
#define	EDID_TIMING_X(v)	(((v) + 31) * 8)
#define	EDID_FREQ(v)		(((v) & 0x3f) + 60)
#define	EDID_RATIO(v)		(((v) >> 6) & 0x3)
#define	EDID_RATIO_10x16	0
#define	EDID_RATIO_3x4		1	
#define	EDID_RATIO_4x5		2	
#define	EDID_RATIO_9x16		3

#define	TDA19988		0x0301

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "nxp,tda998x" },
	DEVICE_COMPAT_EOL
};

struct tda19988_softc;

struct tda19988_connector {
	struct drm_connector	base;
	struct tda19988_softc	*sc;
};

struct tda19988_softc {
	device_t		sc_dev;
	int			sc_phandle;
	i2c_tag_t		sc_i2c;
	i2c_addr_t		sc_addr;
	uint32_t		sc_cec_addr;
	uint16_t		sc_version;
	int			sc_current_page;
	uint8_t			*sc_edid;
	uint32_t		sc_edid_len;
	bool			sc_edid_valid;

	struct drm_bridge	sc_bridge;
	struct tda19988_connector sc_connector;

	struct fdt_device_ports	sc_ports;

	enum drm_connector_status sc_last_status;
};

#define	to_tda_connector(x)	container_of(x, struct tda19988_connector, base)

static int
tda19988_set_page(struct tda19988_softc *sc, uint8_t page)
{
	uint8_t buf[2] = { TDA_CURPAGE_ADDR, page };
	int result;

	result = iic_exec(sc->sc_i2c, I2C_OP_WRITE_WITH_STOP, sc->sc_addr, buf, 2, NULL, 0, 0);
	if (result == 0)
		sc->sc_current_page = page;

	return result;
}

static int
tda19988_cec_read(struct tda19988_softc *sc, uint8_t addr, uint8_t *data)
{
	return iic_exec(sc->sc_i2c, I2C_OP_READ_WITH_STOP, sc->sc_cec_addr, &addr, 1, data, 1, 0);
}

static int 
tda19988_cec_write(struct tda19988_softc *sc, uint8_t addr, uint8_t data)
{
	uint8_t buf[2] = { addr, data };

	return iic_exec(sc->sc_i2c, I2C_OP_WRITE_WITH_STOP, sc->sc_cec_addr, buf, 2, NULL, 0, 0);
}

static int
tda19988_block_read(struct tda19988_softc *sc, uint16_t addr, uint8_t *data, int len)
{
	uint8_t reg;

	reg = REGADDR(addr);

	if (sc->sc_current_page != REGPAGE(addr))
		tda19988_set_page(sc, REGPAGE(addr));

	return iic_exec(sc->sc_i2c, I2C_OP_READ_WITH_STOP, sc->sc_addr, &reg, 1, data, len, 0);
}

static int
tda19988_reg_read(struct tda19988_softc *sc, uint16_t addr, uint8_t *data)
{
	uint8_t reg;

	reg = REGADDR(addr);

	if (sc->sc_current_page != REGPAGE(addr))
		tda19988_set_page(sc, REGPAGE(addr));

	return iic_exec(sc->sc_i2c, I2C_OP_READ_WITH_STOP, sc->sc_addr, &reg, 1, data, 1, 0);
}

static int
tda19988_reg_write(struct tda19988_softc *sc, uint16_t addr, uint8_t data)
{
	uint8_t buf[2] = { REGADDR(addr), data };

	if (sc->sc_current_page != REGPAGE(addr))
		tda19988_set_page(sc, REGPAGE(addr));

	return iic_exec(sc->sc_i2c, I2C_OP_WRITE_WITH_STOP, sc->sc_addr, buf, 2, NULL, 0, 0);
}

static int
tda19988_reg_write2(struct tda19988_softc *sc, uint16_t address, uint16_t data)
{
	uint8_t buf[3];

	buf[0] = REGADDR(address);
	buf[1] = (data >> 8);
	buf[2] = (data & 0xff);

	if (sc->sc_current_page != REGPAGE(address))
		tda19988_set_page(sc, REGPAGE(address));

	return iic_exec(sc->sc_i2c, I2C_OP_READ_WITH_STOP, sc->sc_addr, buf, 3, NULL, 0, 0);
}

static void
tda19988_reg_set(struct tda19988_softc *sc, uint16_t addr, uint8_t flags)
{
	uint8_t data;

	tda19988_reg_read(sc, addr, &data);
	data |= flags;
	tda19988_reg_write(sc, addr, data);
}

static void
tda19988_reg_clear(struct tda19988_softc *sc, uint16_t addr, uint8_t flags)
{
	uint8_t data;

	tda19988_reg_read(sc, addr, &data);
	data &= ~flags;
	tda19988_reg_write(sc, addr, data);
}

static int
tda19988_match(device_t parent, cfdata_t match, void *aux)
{
	struct i2c_attach_args * const ia = aux;
	int match_result;

	if (iic_use_direct_match(ia, match, compat_data, &match_result))
		return match_result;

	return 0;
}

static void
tda19988_init_encoder(struct tda19988_softc *sc, const struct drm_display_mode *mode)
{
	uint16_t ref_pix, ref_line, n_pix, n_line;
	uint16_t hs_pix_start, hs_pix_stop;
	uint16_t vs1_pix_start, vs1_pix_stop;
	uint16_t vs1_line_start, vs1_line_end;
	uint16_t vs2_pix_start, vs2_pix_stop;
	uint16_t vs2_line_start, vs2_line_end;
	uint16_t vwin1_line_start, vwin1_line_end;
	uint16_t vwin2_line_start, vwin2_line_end;
	uint16_t de_start, de_stop;
	uint8_t reg, div;

	n_pix = mode->crtc_htotal;
	n_line = mode->crtc_vtotal;

	hs_pix_stop = mode->crtc_hsync_end - mode->crtc_hdisplay;
	hs_pix_start = mode->crtc_hsync_start - mode->crtc_hdisplay;

	de_stop = mode->crtc_htotal;
	de_start = mode->crtc_htotal - mode->crtc_hdisplay;
	ref_pix = hs_pix_start + 3;

	if (mode->flags & DRM_MODE_FLAG_HSKEW)
		ref_pix += mode->crtc_hskew;

	if ((mode->flags & DRM_MODE_FLAG_INTERLACE) == 0) {
		ref_line = 1 + mode->crtc_vsync_start - mode->crtc_vdisplay;
		vwin1_line_start = mode->crtc_vtotal - mode->crtc_vdisplay - 1;
		vwin1_line_end = vwin1_line_start + mode->crtc_vdisplay;

		vs1_pix_start = vs1_pix_stop = hs_pix_start;
		vs1_line_start = mode->crtc_vsync_start - mode->crtc_vdisplay;
		vs1_line_end = vs1_line_start + mode->crtc_vsync_end - mode->crtc_vsync_start;

		vwin2_line_start = vwin2_line_end = 0;
		vs2_pix_start = vs2_pix_stop = 0;
		vs2_line_start = vs2_line_end = 0;
	} else {
		ref_line = 1 + (mode->crtc_vsync_start - mode->crtc_vdisplay)/2;
		vwin1_line_start = (mode->crtc_vtotal - mode->crtc_vdisplay)/2;
		vwin1_line_end = vwin1_line_start + mode->crtc_vdisplay/2;

		vs1_pix_start = vs1_pix_stop = hs_pix_start;
		vs1_line_start = (mode->crtc_vsync_start - mode->crtc_vdisplay)/2;
		vs1_line_end = vs1_line_start + (mode->crtc_vsync_end - mode->crtc_vsync_start)/2;

		vwin2_line_start = vwin1_line_start + mode->crtc_vtotal/2;
		vwin2_line_end = vwin2_line_start + mode->crtc_vdisplay/2;

		vs2_pix_start = vs2_pix_stop = hs_pix_start + mode->crtc_htotal/2;
		vs2_line_start = vs1_line_start + mode->crtc_vtotal/2 ;
		vs2_line_end = vs2_line_start + (mode->crtc_vsync_end - mode->crtc_vsync_start)/2;
	}

	div = 148500 / mode->crtc_clock;
	if (div != 0) {
		div--;
		if (div > 3)
			div = 3;
	}

	/* set HDMI HDCP mode off */
	tda19988_reg_set(sc, TDA_TBG_CNTRL_1, TBG_CNTRL_1_DWIN_DIS);
	tda19988_reg_clear(sc, TDA_HDCP_TX33, HDCP_TX33_HDMI);
	tda19988_reg_write(sc, TDA_ENC_CNTRL, ENC_CNTRL_DVI_MODE);

	/* no pre-filter or interpolator */
	tda19988_reg_write(sc, TDA_HVF_CNTRL_0,
	    HVF_CNTRL_0_INTPOL_BYPASS | HVF_CNTRL_0_PREFIL_NONE);
	tda19988_reg_write(sc, TDA_VIP_CNTRL_5, VIP_CNTRL_5_SP_CNT(0));
	tda19988_reg_write(sc, TDA_VIP_CNTRL_4,
	    VIP_CNTRL_4_BLANKIT_NDE | VIP_CNTRL_4_BLC_NONE);

	tda19988_reg_clear(sc, TDA_PLL_SERIAL_3, PLL_SERIAL_3_SRL_CCIR);
	tda19988_reg_clear(sc, TDA_PLL_SERIAL_1, PLL_SERIAL_1_SRL_MAN_IP);
	tda19988_reg_clear(sc, TDA_PLL_SERIAL_3, PLL_SERIAL_3_SRL_DE);
	tda19988_reg_write(sc, TDA_SERIALIZER, 0);
	tda19988_reg_write(sc, TDA_HVF_CNTRL_1, HVF_CNTRL_1_VQR_FULL);

	tda19988_reg_write(sc, TDA_RPT_CNTRL, 0);
	tda19988_reg_write(sc, TDA_SEL_CLK, SEL_CLK_SEL_VRF_CLK(0) |
			SEL_CLK_SEL_CLK1 | SEL_CLK_ENA_SC_CLK);

	tda19988_reg_write(sc, TDA_PLL_SERIAL_2, PLL_SERIAL_2_SRL_NOSC(div) |
			PLL_SERIAL_2_SRL_PR(0));

	tda19988_reg_set(sc, TDA_MAT_CONTRL, MAT_CONTRL_MAT_BP);

	tda19988_reg_write(sc, TDA_ANA_GENERAL, 0x09);

	tda19988_reg_clear(sc, TDA_TBG_CNTRL_0, TBG_CNTRL_0_SYNC_MTHD);

	/*
	 * Sync on rising HSYNC/VSYNC
	 */
	reg = VIP_CNTRL_3_SYNC_HS;
	if (mode->flags & DRM_MODE_FLAG_NHSYNC)
		reg |= VIP_CNTRL_3_H_TGL;
	if (mode->flags & DRM_MODE_FLAG_NVSYNC)
		reg |= VIP_CNTRL_3_V_TGL;
	tda19988_reg_write(sc, TDA_VIP_CNTRL_3, reg);

	reg = TBG_CNTRL_1_TGL_EN;
	if (mode->flags & DRM_MODE_FLAG_NHSYNC)
		reg |= TBG_CNTRL_1_H_TGL;
	if (mode->flags & DRM_MODE_FLAG_NVSYNC)
		reg |= TBG_CNTRL_1_V_TGL;
	tda19988_reg_write(sc, TDA_TBG_CNTRL_1, reg);

	/* Program timing */
	tda19988_reg_write(sc, TDA_VIDFORMAT, 0x00);

	tda19988_reg_write2(sc, TDA_REFPIX_MSB, ref_pix);
	tda19988_reg_write2(sc, TDA_REFLINE_MSB, ref_line);
	tda19988_reg_write2(sc, TDA_NPIX_MSB, n_pix);
	tda19988_reg_write2(sc, TDA_NLINE_MSB, n_line);

	tda19988_reg_write2(sc, TDA_VS_LINE_STRT_1_MSB, vs1_line_start);
	tda19988_reg_write2(sc, TDA_VS_PIX_STRT_1_MSB, vs1_pix_start);
	tda19988_reg_write2(sc, TDA_VS_LINE_END_1_MSB, vs1_line_end);
	tda19988_reg_write2(sc, TDA_VS_PIX_END_1_MSB, vs1_pix_stop);
	tda19988_reg_write2(sc, TDA_VS_LINE_STRT_2_MSB, vs2_line_start);
	tda19988_reg_write2(sc, TDA_VS_PIX_STRT_2_MSB, vs2_pix_start);
	tda19988_reg_write2(sc, TDA_VS_LINE_END_2_MSB, vs2_line_end);
	tda19988_reg_write2(sc, TDA_VS_PIX_END_2_MSB, vs2_pix_stop);
	tda19988_reg_write2(sc, TDA_HS_PIX_START_MSB, hs_pix_start);
	tda19988_reg_write2(sc, TDA_HS_PIX_STOP_MSB, hs_pix_stop);
	tda19988_reg_write2(sc, TDA_VWIN_START_1_MSB, vwin1_line_start);
	tda19988_reg_write2(sc, TDA_VWIN_END_1_MSB, vwin1_line_end);
	tda19988_reg_write2(sc, TDA_VWIN_START_2_MSB, vwin2_line_start);
	tda19988_reg_write2(sc, TDA_VWIN_END_2_MSB, vwin2_line_end);
	tda19988_reg_write2(sc, TDA_DE_START_MSB, de_start);
	tda19988_reg_write2(sc, TDA_DE_STOP_MSB, de_stop);

	if (sc->sc_version == TDA19988)
		tda19988_reg_write(sc, TDA_ENABLE_SPACE, 0x00);

	/* must be last register set */
	tda19988_reg_clear(sc, TDA_TBG_CNTRL_0, TBG_CNTRL_0_SYNC_ONCE);
}

static int
tda19988_read_edid_block(struct tda19988_softc *sc, uint8_t *buf, int block)
{
	int attempt, err;
	uint8_t data;

	err = 0;

	tda19988_reg_set(sc, TDA_INT_FLAGS_2, INT_FLAGS_2_EDID_BLK_RD);

	/* Block 0 */
	tda19988_reg_write(sc, TDA_DDC_ADDR, 0xa0);
	tda19988_reg_write(sc, TDA_DDC_OFFS, (block % 2) ? 128 : 0);
	tda19988_reg_write(sc, TDA_DDC_SEGM_ADDR, 0x60);
	tda19988_reg_write(sc, TDA_DDC_SEGM, block / 2);

	tda19988_reg_write(sc, TDA_EDID_CTRL, 1);
	tda19988_reg_write(sc, TDA_EDID_CTRL, 0);

	data = 0;
	for (attempt = 0; attempt < MAX_READ_ATTEMPTS; attempt++) {
		tda19988_reg_read(sc, TDA_INT_FLAGS_2, &data);
		if (data & INT_FLAGS_2_EDID_BLK_RD)
			break;
		delay(1000);
	}

	if (attempt == MAX_READ_ATTEMPTS) {
		err = -1;
		goto done;
	}

	if (tda19988_block_read(sc, TDA_EDID_DATA0, buf, EDID_LENGTH) != 0) {
		err = -1;
		goto done;
	}

done:
	tda19988_reg_clear(sc, TDA_INT_FLAGS_2, INT_FLAGS_2_EDID_BLK_RD);

	return (err);
}

static int
tda19988_read_edid(struct tda19988_softc *sc)
{
	int err;
	int blocks, i;
	uint8_t *buf, *edid;

	err = 0;
	if (sc->sc_version == TDA19988)
		tda19988_reg_clear(sc, TDA_TX4, TX4_PD_RAM);

	err = tda19988_read_edid_block(sc, sc->sc_edid, 0);
	if (err)
		goto done;

	blocks = sc->sc_edid[0x7e];
	if (blocks > 0) {
		if (sc->sc_edid_len != EDID_LENGTH*(blocks+1)) {
			edid = kmem_zalloc(EDID_LENGTH*(blocks+1), KM_SLEEP);
			memcpy(edid, sc->sc_edid, EDID_LENGTH);
			kmem_free(sc->sc_edid, sc->sc_edid_len);
			sc->sc_edid = edid;
			sc->sc_edid_len = EDID_LENGTH*(blocks+1);
		}
		for (i = 0; i < blocks; i++) {
			/* TODO: check validity */
			buf = sc->sc_edid + EDID_LENGTH*(i+1);
			err = tda19988_read_edid_block(sc, buf, i);
			if (err)
				goto done;
		}
	}

done:
	if (sc->sc_version == TDA19988)
		tda19988_reg_set(sc, TDA_TX4, TX4_PD_RAM);

	return (err);
}

static void
tda19988_start(struct tda19988_softc *sc)
{
	device_t dev;
	uint8_t data;
	uint16_t ver;

	dev = sc->sc_dev;
	
	tda19988_cec_write(sc, TDA_CEC_ENAMODS, ENAMODS_RXSENS | ENAMODS_HDMI);
	DELAY(1000);
	tda19988_cec_read(sc, TDA_CEC_RXSHPDLEV, &data);

	/* Reset core */
	tda19988_reg_set(sc, TDA_SOFTRESET, 3);
	DELAY(100);
	tda19988_reg_clear(sc, TDA_SOFTRESET, 3);
	DELAY(100);

	/* reset transmitter: */
	tda19988_reg_set(sc, TDA_MAIN_CNTRL0, MAIN_CNTRL0_SR);
	tda19988_reg_clear(sc, TDA_MAIN_CNTRL0, MAIN_CNTRL0_SR);

	/* PLL registers common configuration */
	tda19988_reg_write(sc, TDA_PLL_SERIAL_1, 0x00);
	tda19988_reg_write(sc, TDA_PLL_SERIAL_2, PLL_SERIAL_2_SRL_NOSC(1));
	tda19988_reg_write(sc, TDA_PLL_SERIAL_3, 0x00);
	tda19988_reg_write(sc, TDA_SERIALIZER, 0x00);
	tda19988_reg_write(sc, TDA_BUFFER_OUT, 0x00);
	tda19988_reg_write(sc, TDA_PLL_SCG1, 0x00);
	tda19988_reg_write(sc, TDA_SEL_CLK, SEL_CLK_SEL_CLK1 | SEL_CLK_ENA_SC_CLK);
	tda19988_reg_write(sc, TDA_PLL_SCGN1, 0xfa);
	tda19988_reg_write(sc, TDA_PLL_SCGN2, 0x00);
	tda19988_reg_write(sc, TDA_PLL_SCGR1, 0x5b);
	tda19988_reg_write(sc, TDA_PLL_SCGR2, 0x00);
	tda19988_reg_write(sc, TDA_PLL_SCG2, 0x10);

	/* Write the default value MUX register */
	tda19988_reg_write(sc, TDA_MUX_VP_VIP_OUT, 0x24);

	ver = 0;
	tda19988_reg_read(sc, TDA_VERSION, &data);
	ver |= data;
	tda19988_reg_read(sc, TDA_VERSION_MSB, &data);
	ver |= (data << 8);

	/* Clear feature bits */
	sc->sc_version = ver & ~0x30;
	switch (sc->sc_version) {
		case TDA19988:
			device_printf(dev, "TDA19988\n");
			break;
		default:
			device_printf(dev, "Unknown device: %04x\n", sc->sc_version);
			return;
	}

	tda19988_reg_write(sc, TDA_DDC_CTRL, DDC_ENABLE);
	tda19988_reg_write(sc, TDA_TX3, 39);

    	tda19988_cec_write(sc, TDA_CEC_FRO_IM_CLK_CTRL,
            CEC_FRO_IM_CLK_CTRL_GHOST_DIS | CEC_FRO_IM_CLK_CTRL_IMCLK_SEL);

	/* Default values for RGB 4:4:4 mapping */
	tda19988_reg_write(sc, TDA_VIP_CNTRL_0, 0x23);
	tda19988_reg_write(sc, TDA_VIP_CNTRL_1, 0x01);
	tda19988_reg_write(sc, TDA_VIP_CNTRL_2, 0x45);
}

static enum drm_connector_status
tda19988_connector_detect(struct drm_connector *connector, bool force)
{
	struct tda19988_connector *tda_connector = to_tda_connector(connector);
	struct tda19988_softc * const sc = tda_connector->sc;
	enum drm_connector_status status;
	uint8_t data = 0;

	iic_acquire_bus(sc->sc_i2c, 0);
	tda19988_cec_read(sc, TDA_CEC_RXSHPDLEV, &data);
	iic_release_bus(sc->sc_i2c, 0);

	status = (data & RXSHPDLEV_HPD) ?
	    connector_status_connected :
	    connector_status_disconnected;

	/* On connect, invalidate the last EDID */
	if (status == connector_status_connected &&
	    sc->sc_last_status != connector_status_connected)
		sc->sc_edid_valid = false;

	sc->sc_last_status = status;

	return status;
}

static void
tda19988_connector_destroy(struct drm_connector *connector)
{
	drm_connector_unregister(connector);
	drm_connector_cleanup(connector);
}

static const struct drm_connector_funcs tda19988_connector_funcs = {
	.dpms = drm_helper_connector_dpms,
	.detect = tda19988_connector_detect,
	.fill_modes = drm_helper_probe_single_connector_modes,
	.destroy = tda19988_connector_destroy,
};

static int
tda19988_connector_get_modes(struct drm_connector *connector)
{
	struct tda19988_connector *tda_connector = to_tda_connector(connector);
	struct tda19988_softc * const sc = tda_connector->sc;
	struct edid *pedid = NULL;

	if (sc->sc_edid_valid) {
		pedid = (struct edid *)sc->sc_edid;
	} else {
		iic_acquire_bus(sc->sc_i2c, 0);
		if (tda19988_read_edid(sc) == 0)
			pedid = (struct edid *)sc->sc_edid;
		iic_release_bus(sc->sc_i2c, 0);
		sc->sc_edid_valid = true;
	}

	drm_connector_update_edid_property(connector, pedid);
	if (pedid == NULL)
		return 0;

	return drm_add_edid_modes(connector, pedid);
}

static const struct drm_connector_helper_funcs tda19988_connector_helper_funcs = {
	.get_modes = tda19988_connector_get_modes,
};

static int
tda19988_bridge_attach(struct drm_bridge *bridge)
{
	struct tda19988_softc *sc = bridge->driver_private;
	struct tda19988_connector *tda_connector = &sc->sc_connector;
	struct drm_connector *connector = &tda_connector->base;
	int error;

	tda_connector->sc = sc;

	connector->polled = DRM_CONNECTOR_POLL_CONNECT | DRM_CONNECTOR_POLL_DISCONNECT;
	connector->interlace_allowed = 1;
	connector->doublescan_allowed = 0;

	drm_connector_init(bridge->dev, connector, &tda19988_connector_funcs,
	    DRM_MODE_CONNECTOR_HDMIA);
	drm_connector_helper_add(connector, &tda19988_connector_helper_funcs);

	error = drm_connector_attach_encoder(connector, bridge->encoder);
	if (error)
		return error;

	return drm_connector_register(connector);
}

static void
tda19988_bridge_enable(struct drm_bridge *bridge)
{
	struct tda19988_softc * const sc = bridge->driver_private;

	fdtbus_pinctrl_set_config(sc->sc_phandle, "default");
}

static void
tda19988_bridge_pre_enable(struct drm_bridge *bridge)
{
}

static void
tda19988_bridge_disable(struct drm_bridge *bridge)
{
	struct tda19988_softc * const sc = bridge->driver_private;

	fdtbus_pinctrl_set_config(sc->sc_phandle, "off");
}

static void
tda19988_bridge_post_disable(struct drm_bridge *bridge)
{
}

static void
tda19988_bridge_mode_set(struct drm_bridge *bridge,
    const struct drm_display_mode *mode,
    const struct drm_display_mode *adjusted_mode)
{
	struct tda19988_softc * const sc = bridge->driver_private;

	iic_acquire_bus(sc->sc_i2c, 0);
	tda19988_init_encoder(sc, adjusted_mode);
	iic_release_bus(sc->sc_i2c, 0);
}

static bool
tda19988_bridge_mode_fixup(struct drm_bridge *bridge,
    const struct drm_display_mode *mode, struct drm_display_mode *adjusted_mode)
{
	return true;
}

static const struct drm_bridge_funcs tda19988_bridge_funcs = {
	.attach = tda19988_bridge_attach,
	.enable = tda19988_bridge_enable,
	.pre_enable = tda19988_bridge_pre_enable,
	.disable = tda19988_bridge_disable,
	.post_disable = tda19988_bridge_post_disable,
	.mode_set = tda19988_bridge_mode_set,
	.mode_fixup = tda19988_bridge_mode_fixup,
};

static int
tda19988_ep_activate(device_t dev, struct fdt_endpoint *ep, bool activate)
{
	struct tda19988_softc *sc = device_private(dev);
	struct fdt_endpoint *in_ep = fdt_endpoint_remote(ep);
	struct drm_encoder *encoder;
	int error;

	if (!activate)
		return EINVAL;

	if (fdt_endpoint_port_index(ep) != TDA19988_PORT_INPUT)
		return EINVAL;

	switch (fdt_endpoint_type(in_ep)) {
	case EP_DRM_ENCODER:
		encoder = fdt_endpoint_get_data(in_ep);
		break;
	default:
		encoder = NULL;
		break;
	}

	if (encoder == NULL)
		return EINVAL;

	sc->sc_bridge.driver_private = sc;
	sc->sc_bridge.funcs = &tda19988_bridge_funcs;
	sc->sc_bridge.encoder = encoder;
	error = drm_bridge_attach(encoder, &sc->sc_bridge, NULL);
	if (error)
		return EIO;

	return 0;
}

static void *
tda19988_ep_get_data(device_t dev, struct fdt_endpoint *ep)
{
	struct tda19988_softc *sc = device_private(dev);

	return &sc->sc_bridge;
}

static void
tda19988_attach(device_t parent, device_t self, void *aux)
{
	struct tda19988_softc *sc = device_private(self);
	struct i2c_attach_args * const ia = aux;
	const int phandle = ia->ia_cookie;

	sc->sc_dev = self;
	sc->sc_phandle = phandle;
	sc->sc_i2c = ia->ia_tag;
	sc->sc_addr = ia->ia_addr;
	sc->sc_cec_addr = 0x34; /* hardcoded */
	sc->sc_current_page = 0xff;
	sc->sc_edid = kmem_zalloc(EDID_LENGTH, KM_SLEEP);
	sc->sc_edid_len = EDID_LENGTH;
	sc->sc_edid_valid = false;
	sc->sc_last_status = connector_status_unknown;

	aprint_naive("\n");
	aprint_normal(": NXP TDA19988 HDMI transmitter\n");

	iic_acquire_bus(sc->sc_i2c, 0);
	tda19988_start(sc);
	iic_release_bus(sc->sc_i2c, 0);

	sc->sc_ports.dp_ep_activate = tda19988_ep_activate;
	sc->sc_ports.dp_ep_get_data = tda19988_ep_get_data;
	fdt_ports_register(&sc->sc_ports, self, phandle, EP_DRM_ENCODER);
}

CFATTACH_DECL_NEW(tdahdmi, sizeof(struct tda19988_softc),
    tda19988_match, tda19988_attach, NULL, NULL);
