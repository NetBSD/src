/* $NetBSD: amlogic_genfb.c,v 1.5.18.2 2017/12/03 11:35:51 jdolecek Exp $ */

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

/*
 * Generic framebuffer console driver
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: amlogic_genfb.c,v 1.5.18.2 2017/12/03 11:35:51 jdolecek Exp $");

#include <sys/param.h>
#include <sys/types.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/bus.h>
#include <sys/kmem.h>
#include <sys/sysctl.h>

#include <arm/amlogic/amlogic_reg.h>
#include <arm/amlogic/amlogic_var.h>
#include <arm/amlogic/amlogic_canvasreg.h>
#include <arm/amlogic/amlogic_vpureg.h>
#include <arm/amlogic/amlogic_hdmireg.h>

#include <dev/wsfb/genfbvar.h>

#define AMLOGIC_GENFB_DEFAULT_DEPTH	16

/* Map CEA-861-D video code (VIC) to framebuffer dimensions */
static const struct amlogic_genfb_vic2mode {
	u_int vic;
	u_int width;
	u_int height;
	u_int flags;
#define INTERLACE __BIT(0)
#define DBLSCAN __BIT(1)
} amlogic_genfb_modes[] = {
	{ 1, 640, 480 },
	{ 2, 720, 480 },
	{ 3, 720, 480 },
	{ 4, 1280, 720 },
	{ 5, 1920, 1080, INTERLACE },
	{ 6, 720, 480, DBLSCAN | INTERLACE },
	{ 7, 720, 480, DBLSCAN | INTERLACE },
	{ 8, 720, 240, DBLSCAN },
	{ 9, 720, 240, DBLSCAN },
	{ 16, 1920, 1080 },
	{ 17, 720, 576 },
	{ 18, 720, 576 },
	{ 19, 1280, 720 },
	{ 20, 1920, 1080, INTERLACE },
	{ 31, 1920, 1080 },
	{ 32, 1920, 1080 },
	{ 33, 1920, 1080 },
	{ 34, 1920, 1080 },
	{ 39, 1920, 1080, INTERLACE },
};

struct amlogic_genfb_softc {
	struct genfb_softc	sc_gen;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_cav_bsh;
	bus_space_handle_t	sc_hdmi_bsh;
	bus_space_handle_t	sc_vpu_bsh;
	bus_dma_tag_t		sc_dmat;

	kmutex_t		sc_lock;

	u_int			sc_scale;

	bus_dma_segment_t	sc_dmasegs[1];
	bus_size_t		sc_dmasize;
	bus_dmamap_t		sc_dmamap;
	void			*sc_dmap;

	uint32_t		sc_wstype;

	struct sysctllog	*sc_sysctllog;
	int			sc_node_scale;
};

static int	amlogic_genfb_match(device_t, cfdata_t, void *);
static void	amlogic_genfb_attach(device_t, device_t, void *);

static int	amlogic_genfb_ioctl(void *, void *, u_long, void *, int, lwp_t *);
static paddr_t	amlogic_genfb_mmap(void *, void *, off_t, int);
static bool	amlogic_genfb_shutdown(device_t, int);

static void	amlogic_genfb_canvas_config(struct amlogic_genfb_softc *);
static void	amlogic_genfb_osd_config(struct amlogic_genfb_softc *);
static void	amlogic_genfb_scaler_config(struct amlogic_genfb_softc *);

static void	amlogic_genfb_init(struct amlogic_genfb_softc *);
static int	amlogic_genfb_alloc_videomem(struct amlogic_genfb_softc *);

static int	amlogic_genfb_scale_helper(SYSCTLFN_PROTO);

void		amlogic_genfb_set_console_dev(device_t);
void		amlogic_genfb_ddb_trap_callback(int);

static device_t amlogic_genfb_console_dev = NULL;

CFATTACH_DECL_NEW(amlogic_genfb, sizeof(struct amlogic_genfb_softc),
    amlogic_genfb_match, amlogic_genfb_attach, NULL, NULL);

static inline uint32_t
amlogic_genfb_hdmi_read_4(struct amlogic_genfb_softc *sc, uint32_t addr)
{
	bus_space_write_4(sc->sc_bst, sc->sc_hdmi_bsh, HDMI_ADDR_REG, addr);
	bus_space_write_4(sc->sc_bst, sc->sc_hdmi_bsh, HDMI_ADDR_REG, addr);
	return bus_space_read_4(sc->sc_bst, sc->sc_hdmi_bsh, HDMI_DATA_REG);
}

static __unused inline void
amlogic_genfb_hdmi_write_4(struct amlogic_genfb_softc *sc, uint32_t addr,
    uint32_t data)
{
	bus_space_write_4(sc->sc_bst, sc->sc_hdmi_bsh, HDMI_ADDR_REG, addr);
	bus_space_write_4(sc->sc_bst, sc->sc_hdmi_bsh, HDMI_ADDR_REG, addr);
	bus_space_write_4(sc->sc_bst, sc->sc_hdmi_bsh, HDMI_DATA_REG, data);
}

#define HDMI_READ	amlogic_genfb_hdmi_read_4
#define HDMI_WRITE	amlogic_genfb_hdmi_write_4

#define VPU_READ(sc, reg) \
    bus_space_read_4((sc)->sc_bst, (sc)->sc_vpu_bsh, (reg))
#define VPU_WRITE(sc, reg, val) \
    bus_space_write_4((sc)->sc_bst, (sc)->sc_vpu_bsh, (reg), (val))

#define CAV_READ(sc, reg) \
    bus_space_read_4((sc)->sc_bst, (sc)->sc_cav_bsh, (reg))
#define CAV_WRITE(sc, reg, val) \
    bus_space_write_4((sc)->sc_bst, (sc)->sc_cav_bsh, (reg), (val))

static int
amlogic_genfb_match(device_t parent, cfdata_t match, void *aux)
{
	return 1;
}

static void
amlogic_genfb_attach(device_t parent, device_t self, void *aux)
{
	struct amlogic_genfb_softc *sc = device_private(self);
	struct amlogicio_attach_args * const aio = aux;
	const struct amlogic_locators * const loc = &aio->aio_loc;
	prop_dictionary_t dict = device_properties(self);
	static const struct genfb_ops zero_ops;
	struct genfb_ops ops = zero_ops;
	bool is_console = false;

	sc->sc_gen.sc_dev = self;
	sc->sc_bst = aio->aio_core_bst;
	sc->sc_dmat = aio->aio_dmat;
	bus_space_subregion(aio->aio_core_bst, aio->aio_bsh,
	    loc->loc_offset, loc->loc_size, &sc->sc_cav_bsh);
	bus_space_subregion(aio->aio_core_bst, aio->aio_bsh,
	    AMLOGIC_HDMI_OFFSET, AMLOGIC_HDMI_SIZE, &sc->sc_hdmi_bsh);
	bus_space_subregion(aio->aio_core_bst, aio->aio_bsh,
	    AMLOGIC_VPU_OFFSET, AMLOGIC_VPU_SIZE, &sc->sc_vpu_bsh);
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_NONE);

	amlogic_genfb_init(sc);

	sc->sc_wstype = WSDISPLAY_TYPE_MESON;
	prop_dictionary_get_bool(dict, "is_console", &is_console);

	genfb_init(&sc->sc_gen);

	if (sc->sc_gen.sc_width == 0 ||
	    sc->sc_gen.sc_fbsize == 0) {
		aprint_normal(": disabled\n");
		return;
	}

	pmf_device_register1(self, NULL, NULL, amlogic_genfb_shutdown);

	memset(&ops, 0, sizeof(ops));
	ops.genfb_ioctl = amlogic_genfb_ioctl;
	ops.genfb_mmap = amlogic_genfb_mmap;

	aprint_naive("\n");

	if (is_console)
		aprint_normal(": switching to framebuffer console\n");
	else
		aprint_normal("\n");

	genfb_attach(&sc->sc_gen, &ops);
}

static int
amlogic_genfb_ioctl(void *v, void *vs, u_long cmd, void *data, int flag, lwp_t *l)
{
	struct amlogic_genfb_softc *sc = v;
	struct wsdisplayio_bus_id *busid;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = sc->sc_wstype;
		return 0;
	case WSDISPLAYIO_GET_BUSID:
		busid = data;
		busid->bus_type = WSDISPLAYIO_BUS_SOC;
		return 0;
	case WSDISPLAYIO_GET_FBINFO:
		{
			struct wsdisplayio_fbinfo *fbi = data;
			struct rasops_info *ri = &sc->sc_gen.vd.active->scr_ri;
			int ret;

			ret = wsdisplayio_get_fbinfo(ri, fbi);
			fbi->fbi_flags |= WSFB_VRAM_IS_RAM;
			return ret;
		}
	default:
		return EPASSTHROUGH;
	}
}

static paddr_t
amlogic_genfb_mmap(void *v, void *vs, off_t offset, int prot)
{
	struct amlogic_genfb_softc *sc = v;

	if (offset < 0 || offset >= sc->sc_dmasegs[0].ds_len)
		return -1;

	return bus_dmamem_mmap(sc->sc_dmat, sc->sc_dmasegs, 1,
	    offset, prot, BUS_DMA_PREFETCHABLE);
}

static bool
amlogic_genfb_shutdown(device_t self, int flags)
{
	genfb_enable_polling(self);
	return true;
}

static void
amlogic_genfb_canvas_config(struct amlogic_genfb_softc *sc)
{
	prop_dictionary_t cfg = device_properties(sc->sc_gen.sc_dev);
	const paddr_t pa = sc->sc_dmamap->dm_segs[0].ds_addr;
	uint32_t datal, datah, addr;
	u_int width, height, depth;

	prop_dictionary_get_uint32(cfg, "width", &width);
	prop_dictionary_get_uint32(cfg, "height", &height);
	prop_dictionary_get_uint32(cfg, "depth", &depth);

	const uint32_t w = (width * (depth/8)) >> 3;
	const uint32_t h = height;

	datal = CAV_READ(sc, DC_CAV_LUT_DATAL_REG);
	datah = CAV_READ(sc, DC_CAV_LUT_DATAH_REG);
	addr = CAV_READ(sc, DC_CAV_LUT_ADDR_REG);

	datal &= ~DC_CAV_LUT_DATAL_WIDTH_L;
	datal |= __SHIFTIN(w & 7, DC_CAV_LUT_DATAL_WIDTH_L);
	datal &= ~DC_CAV_LUT_DATAL_FBADDR;
	datal |= __SHIFTIN(pa >> 3, DC_CAV_LUT_DATAL_FBADDR);
	CAV_WRITE(sc, DC_CAV_LUT_DATAL_REG, datal);

	datah &= ~DC_CAV_LUT_DATAH_BLKMODE;
	datah |= __SHIFTIN(DC_CAV_LUT_DATAH_BLKMODE_LINEAR,
			   DC_CAV_LUT_DATAH_BLKMODE);
	datah &= ~DC_CAV_LUT_DATAH_WIDTH_H;
	datah |= __SHIFTIN(w >> 3, DC_CAV_LUT_DATAH_WIDTH_H);
	datah &= ~DC_CAV_LUT_DATAH_HEIGHT;
	datah |= __SHIFTIN(h, DC_CAV_LUT_DATAH_HEIGHT);
	CAV_WRITE(sc, DC_CAV_LUT_DATAH_REG, datah);

	addr |= DC_CAV_LUT_ADDR_WR_EN;
	CAV_WRITE(sc, DC_CAV_LUT_ADDR_REG, addr);
}

static void
amlogic_genfb_osd_config(struct amlogic_genfb_softc *sc)
{
	prop_dictionary_t cfg = device_properties(sc->sc_gen.sc_dev);
	uint32_t cs, tc, w0, w1, w2, w3, w4;
	u_int width, height, depth;
	bool interlace_p;

	prop_dictionary_get_uint32(cfg, "width", &width);
	prop_dictionary_get_uint32(cfg, "height", &height);
	prop_dictionary_get_uint32(cfg, "depth", &depth);
	prop_dictionary_get_bool(cfg, "interlace", &interlace_p);

	cs = VPU_READ(sc, VIU_OSD2_CTRL_STAT_REG);
	cs |= VIU_OSD_CTRL_STAT_ENABLE;
	cs &= ~VIU_OSD_CTRL_STAT_GLOBAL_ALPHA;
	cs |= __SHIFTIN(0xff, VIU_OSD_CTRL_STAT_GLOBAL_ALPHA);
	cs |= VIU_OSD_CTRL_STAT_BLK0_ENABLE;
	cs &= ~VIU_OSD_CTRL_STAT_BLK1_ENABLE;
	cs &= ~VIU_OSD_CTRL_STAT_BLK2_ENABLE;
	cs &= ~VIU_OSD_CTRL_STAT_BLK3_ENABLE;
	VPU_WRITE(sc, VIU_OSD2_CTRL_STAT_REG, cs);

	tc = __SHIFTIN(0, VIU_OSD_TCOLOR_R) |
	     __SHIFTIN(0, VIU_OSD_TCOLOR_G) |
	     __SHIFTIN(0, VIU_OSD_TCOLOR_B) |
	     __SHIFTIN(255, VIU_OSD_TCOLOR_A);
	VPU_WRITE(sc, VIU_OSD2_TCOLOR_AG0_REG, tc);

	w0 = VPU_READ(sc, VIU_OSD2_BLK0_CFG_W0_REG);
	w0 |= VIU_OSD_BLK_CFG_W0_RGB_EN;
	w0 &= ~VIU_OSD_BLK_CFG_W0_TC_ALPHA_EN;
	w0 &= ~VIU_OSD_BLK_CFG_W0_OSD_BLK_MODE;
	w0 &= ~VIU_OSD_BLK_CFG_W0_COLOR_MATRIX;
	switch (depth) {
	case 32:
		w0 |= __SHIFTIN(VIU_OSD_BLK_CFG_W0_OSD_BLK_MODE_32BPP,
				VIU_OSD_BLK_CFG_W0_OSD_BLK_MODE);
		w0 |= __SHIFTIN(VIU_OSD_BLK_CFG_W0_COLOR_MATRIX_ARGB,
				VIU_OSD_BLK_CFG_W0_COLOR_MATRIX);
		break;
	case 24:
		w0 |= __SHIFTIN(VIU_OSD_BLK_CFG_W0_OSD_BLK_MODE_24BPP,
				VIU_OSD_BLK_CFG_W0_OSD_BLK_MODE);
		w0 |= __SHIFTIN(VIU_OSD_BLK_CFG_W0_COLOR_MATRIX_RGB,
				VIU_OSD_BLK_CFG_W0_COLOR_MATRIX);
		break;
	case 16:
		w0 |= __SHIFTIN(VIU_OSD_BLK_CFG_W0_OSD_BLK_MODE_16BPP,
				VIU_OSD_BLK_CFG_W0_OSD_BLK_MODE);
		w0 |= __SHIFTIN(VIU_OSD_BLK_CFG_W0_COLOR_MATRIX_RGB565,
				VIU_OSD_BLK_CFG_W0_COLOR_MATRIX);
		break;
	}
	w0 |= VIU_OSD_BLK_CFG_W0_LITTLE_ENDIAN;
	w0 &= ~VIU_OSD_BLK_CFG_W0_RPT_Y;
	w0 &= ~VIU_OSD_BLK_CFG_W0_INTERP_CTRL;
	if (interlace_p) {
		w0 |= VIU_OSD_BLK_CFG_W0_INTERLACE_EN;
	} else {
		w0 &= ~VIU_OSD_BLK_CFG_W0_INTERLACE_EN;
	}
	VPU_WRITE(sc, VIU_OSD2_BLK0_CFG_W0_REG, w0);

	w1 = __SHIFTIN(width - 1, VIU_OSD_BLK_CFG_W1_X_END) |
	     __SHIFTIN(0, VIU_OSD_BLK_CFG_W1_X_START);
	w2 = __SHIFTIN(height - 1, VIU_OSD_BLK_CFG_W2_Y_END) |
	     __SHIFTIN(0, VIU_OSD_BLK_CFG_W2_Y_START);
	w3 = __SHIFTIN(width - 1, VIU_OSD_BLK_CFG_W3_H_END) |
	     __SHIFTIN(0, VIU_OSD_BLK_CFG_W3_H_START);
	w4 = __SHIFTIN(height - 1, VIU_OSD_BLK_CFG_W4_V_END) |
	     __SHIFTIN(0, VIU_OSD_BLK_CFG_W4_V_START);

	VPU_WRITE(sc, VIU_OSD2_BLK0_CFG_W1_REG, w1);
	VPU_WRITE(sc, VIU_OSD2_BLK0_CFG_W2_REG, w2);
	VPU_WRITE(sc, VIU_OSD2_BLK0_CFG_W3_REG, w3);
	VPU_WRITE(sc, VIU_OSD2_BLK0_CFG_W4_REG, w4);
}

static void
amlogic_genfb_scaler_config(struct amlogic_genfb_softc *sc)
{
	prop_dictionary_t cfg = device_properties(sc->sc_gen.sc_dev);
	uint32_t scctl, sci_wh, sco_h, sco_v, hsc, vsc, hps, vps, hip, vip;
	u_int width, height;
	bool interlace_p;

	prop_dictionary_get_uint32(cfg, "width", &width);
	prop_dictionary_get_uint32(cfg, "height", &height);
	prop_dictionary_get_bool(cfg, "interlace", &interlace_p);

	const u_int scale = sc->sc_scale;
	const u_int dst_w = (width * scale) / 100;
	const u_int dst_h = (height * scale) / 100;
	const u_int margin_w = (width - dst_w) / 2;
	const u_int margin_h = (height - dst_h) / 2;
	const bool scale_p = scale != 100;

	VPU_WRITE(sc, VPP_OSD_SC_DUMMY_DATA_REG, 0x00808000);

	scctl = VPU_READ(sc, VPP_OSD_SC_CTRL0_REG);
	scctl |= VPP_OSD_SC_CTRL0_OSD_SC_PATH_EN;
	scctl &= ~VPP_OSD_SC_CTRL0_OSD_SC_SEL;
	scctl |= __SHIFTIN(1, VPP_OSD_SC_CTRL0_OSD_SC_SEL); /* OSD2 */
	scctl &= ~VPP_OSD_SC_CTRL0_DEFAULT_ALPHA;
	scctl |= __SHIFTIN(0, VPP_OSD_SC_CTRL0_DEFAULT_ALPHA);
	VPU_WRITE(sc, VPP_OSD_SC_CTRL0_REG, scctl);

	sci_wh = __SHIFTIN(width - 1, VPP_OSD_SCI_WH_M1_WIDTH) |
		 __SHIFTIN((height >> interlace_p) - 1, VPP_OSD_SCI_WH_M1_HEIGHT);
	sco_h = __SHIFTIN(margin_w, VPP_OSD_SCO_H_START) |
		__SHIFTIN(width - margin_w - 1, VPP_OSD_SCO_H_END);
	sco_v = __SHIFTIN(margin_h >> interlace_p, VPP_OSD_SCO_V_START) |
		__SHIFTIN(((height - margin_h) >> interlace_p) - 1,
			  VPP_OSD_SCO_V_END);

	VPU_WRITE(sc, VPP_OSD_SCI_WH_M1_REG, sci_wh);
	VPU_WRITE(sc, VPP_OSD_SCO_H_REG, sco_h);
	VPU_WRITE(sc, VPP_OSD_SCO_V_REG, sco_v);

	/* horizontal scaling */
	hsc = VPU_READ(sc, VPP_OSD_HSC_CTRL0_REG);
	if (scale_p) {
		hsc &= ~VPP_OSD_HSC_CTRL0_BANK_LENGTH;
		hsc |= __SHIFTIN(4, VPP_OSD_HSC_CTRL0_BANK_LENGTH);
		hsc &= ~VPP_OSD_HSC_CTRL0_INI_RCV_NUM0;
		hsc |= __SHIFTIN(4, VPP_OSD_HSC_CTRL0_INI_RCV_NUM0);
		hsc &= ~VPP_OSD_HSC_CTRL0_RPT_P0_NUM0;
		hsc |= __SHIFTIN(1, VPP_OSD_HSC_CTRL0_RPT_P0_NUM0);
		hsc |= VPP_OSD_HSC_CTRL0_HSCALE_EN;
	} else {
		hsc &= ~VPP_OSD_HSC_CTRL0_HSCALE_EN;
	}
	VPU_WRITE(sc, VPP_OSD_HSC_CTRL0_REG, hsc);

	/* vertical scaling */
	vsc = VPU_READ(sc, VPP_OSD_VSC_CTRL0_REG);
	if (scale_p) {
		vsc &= ~VPP_OSD_VSC_CTRL0_BANK_LENGTH;
		vsc |= __SHIFTIN(4, VPP_OSD_VSC_CTRL0_BANK_LENGTH);
		vsc &= ~VPP_OSD_VSC_CTRL0_TOP_INI_RCV_NUM0;
		vsc |= __SHIFTIN(4, VPP_OSD_VSC_CTRL0_TOP_INI_RCV_NUM0);
		vsc &= ~VPP_OSD_VSC_CTRL0_TOP_RPT_P0_NUM0;
		vsc |= __SHIFTIN(1, VPP_OSD_VSC_CTRL0_TOP_RPT_P0_NUM0);
		vsc &= ~VPP_OSD_VSC_CTRL0_BOT_INI_RCV_NUM0;
		vsc &= ~VPP_OSD_VSC_CTRL0_BOT_RPT_P0_NUM0;
		vsc &= ~VPP_OSC_VSC_CTRL0_INTERLACE;
		if (interlace_p) {
			/* interlace */
			vsc |= VPP_OSC_VSC_CTRL0_INTERLACE;
			vsc |= __SHIFTIN(6, VPP_OSD_VSC_CTRL0_BOT_INI_RCV_NUM0);
			vsc |= __SHIFTIN(2, VPP_OSD_VSC_CTRL0_BOT_RPT_P0_NUM0);
		}
		vsc |= VPP_OSD_VSC_CTRL0_VSCALE_EN;
	} else {
		vsc &= ~VPP_OSD_VSC_CTRL0_VSCALE_EN;
	}
	VPU_WRITE(sc, VPP_OSD_VSC_CTRL0_REG, vsc);

	/* free scale enable */
	if (scale_p) {
		const u_int hf_phase_step = ((width << 18) / dst_w) << 6;
		const u_int vf_phase_step = ((height << 20) / dst_h) << 4;
		const u_int bot_ini_phase =
		    interlace_p ? ((vf_phase_step / 2) >> 8) : 0;

		hps = VPU_READ(sc, VPP_OSD_HSC_PHASE_STEP_REG);
		hps &= ~VPP_OSD_HSC_PHASE_STEP_FORMAT;
		hps |= __SHIFTIN(hf_phase_step, VPP_OSD_HSC_PHASE_STEP_FORMAT);
		VPU_WRITE(sc, VPP_OSD_HSC_PHASE_STEP_REG, hps);

		hip = VPU_READ(sc, VPP_OSD_HSC_INI_PHASE_REG);
		hip &= ~VPP_OSD_HSC_INI_PHASE_1;
		VPU_WRITE(sc, VPP_OSD_HSC_INI_PHASE_REG, hip);

		vps = VPU_READ(sc, VPP_OSD_VSC_PHASE_STEP_REG);
		vps &= ~VPP_OSD_VSC_PHASE_STEP_FORMAT;
		vps |= __SHIFTIN(hf_phase_step, VPP_OSD_VSC_PHASE_STEP_FORMAT);
		VPU_WRITE(sc, VPP_OSD_VSC_PHASE_STEP_REG, vps);

		vip = VPU_READ(sc, VPP_OSD_VSC_INI_PHASE_REG);
		vip &= ~VPP_OSD_VSC_INI_PHASE_1;
		vip |= __SHIFTIN(0, VPP_OSD_VSC_INI_PHASE_1);
		vip &= ~VPP_OSD_VSC_INI_PHASE_0;
		vip |= __SHIFTIN(bot_ini_phase, VPP_OSD_VSC_INI_PHASE_0);
		VPU_WRITE(sc, VPP_OSD_VSC_INI_PHASE_REG, vip);
	}
}

static void
amlogic_genfb_init(struct amlogic_genfb_softc *sc)
{
	prop_dictionary_t cfg = device_properties(sc->sc_gen.sc_dev);
	const struct sysctlnode *node, *devnode;
	u_int width = 0, height = 0, depth, flags, i, scale = 100;
	int error;

	/*
	 * Firmware has (maybe) setup HDMI TX for us. Read the VIC from
	 * the HDMI AVI InfoFrame (bits 6:0 in PB4) and map that to a
	 * framebuffer geometry.
	 */
	const uint32_t vic = HDMI_READ(sc, HDMITX_AVI_INFO_ADDR + 4) & 0x7f;
	for (i = 0; i < __arraycount(amlogic_genfb_modes); i++) {
		if (amlogic_genfb_modes[i].vic == vic) {
			aprint_debug(" [HDMI VIC %d]", vic);
			width = amlogic_genfb_modes[i].width;
			height = amlogic_genfb_modes[i].height;
			flags = amlogic_genfb_modes[i].flags;
			break;
		}
	}
	if (width == 0 || height == 0) {
		aprint_error(" [UNSUPPORTED HDMI VIC %d]", vic);
		return;
	}

	depth = AMLOGIC_GENFB_DEFAULT_DEPTH;
	prop_dictionary_get_uint32(cfg, "depth", &depth);
	switch (depth) {
	case 16:
	case 24:
		break;
	default:
		aprint_error_dev(sc->sc_gen.sc_dev,
		    "unsupported depth %d, using %d\n", depth,
		    AMLOGIC_GENFB_DEFAULT_DEPTH);
		depth = AMLOGIC_GENFB_DEFAULT_DEPTH;
		break;
	}
	prop_dictionary_set_uint8(cfg, "depth", depth);

	const uint32_t fbsize = width * height * (depth / 8);
	sc->sc_dmasize = (fbsize + 3) & ~3;

	error = amlogic_genfb_alloc_videomem(sc);
	if (error) {
		aprint_error_dev(sc->sc_gen.sc_dev,
		    "failed to allocate %u bytes of video memory: %d\n",
		    (u_int)sc->sc_dmasize, error);
		return;
	}

	prop_dictionary_get_uint32(cfg, "scale", &scale);
	if (scale > 100) {
		scale = 100;
	} else if (scale < 10) {
		scale = 10;
	}
	sc->sc_scale = scale;

	prop_dictionary_set_uint32(cfg, "width", width);
	prop_dictionary_set_uint32(cfg, "height", height);
	prop_dictionary_set_bool(cfg, "dblscan", !!(flags & DBLSCAN));
	prop_dictionary_set_bool(cfg, "interlace", !!(flags & INTERLACE));
	prop_dictionary_set_uint16(cfg, "linebytes", width * (depth / 8));
	prop_dictionary_set_uint32(cfg, "address", 0);
	prop_dictionary_set_uint32(cfg, "virtual_address",
	    (uintptr_t)sc->sc_dmap);

	amlogic_genfb_canvas_config(sc);
	amlogic_genfb_osd_config(sc);
	amlogic_genfb_scaler_config(sc);

	/* sysctl setup */
	error = sysctl_createv(&sc->sc_sysctllog, 0, NULL, &node,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "hw", NULL,
	    NULL, 0, NULL, 0, CTL_HW, CTL_EOL);
	if (error)
		goto sysctl_failed;
	error = sysctl_createv(&sc->sc_sysctllog, 0, &node, &devnode,
	    0, CTLTYPE_NODE, device_xname(sc->sc_gen.sc_dev), NULL,
	    NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL);
	if (error)
		goto sysctl_failed;
	error = sysctl_createv(&sc->sc_sysctllog, 0, &devnode, &node,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "scale", NULL,
	    amlogic_genfb_scale_helper, 0, (void *)sc, 0,
	    CTL_CREATE, CTL_EOL);
	if (error)
		goto sysctl_failed;
	sc->sc_node_scale = node->sysctl_num;

	return;

sysctl_failed:
	aprint_error_dev(sc->sc_gen.sc_dev,
	    "couldn't create sysctl nodes (%d)\n", error);
	sysctl_teardown(&sc->sc_sysctllog);
}

static int
amlogic_genfb_scale_helper(SYSCTLFN_ARGS)
{
	struct amlogic_genfb_softc *sc;
	struct sysctlnode node;
	int scale, oldscale, error;

	node = *rnode;
	sc = node.sysctl_data;
	scale = oldscale = sc->sc_scale;
	node.sysctl_data = &scale;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	if (scale > 100) {
		scale = 100;
	} else if (scale < 10) {
		scale = 10;
	}

	if (scale == oldscale) {
		return 0;
	}

	mutex_enter(&sc->sc_lock);
	sc->sc_scale = scale;
	amlogic_genfb_scaler_config(sc);
	mutex_exit(&sc->sc_lock);

	return 0;
}

static int
amlogic_genfb_alloc_videomem(struct amlogic_genfb_softc *sc)
{
	int error, nsegs;

	error = bus_dmamem_alloc(sc->sc_dmat, sc->sc_dmasize, 0x1000, 0,
	    sc->sc_dmasegs, 1, &nsegs, BUS_DMA_WAITOK);
	if (error)
		return error;
	error = bus_dmamem_map(sc->sc_dmat, sc->sc_dmasegs, nsegs,
	    sc->sc_dmasize, &sc->sc_dmap, BUS_DMA_WAITOK | BUS_DMA_COHERENT);
	if (error)
		goto free;
	error = bus_dmamap_create(sc->sc_dmat, sc->sc_dmasize, 1,
	    sc->sc_dmasize, 0, BUS_DMA_WAITOK, &sc->sc_dmamap);
	if (error)
		goto unmap;
	error = bus_dmamap_load(sc->sc_dmat, sc->sc_dmamap, sc->sc_dmap,
	    sc->sc_dmasize, NULL, BUS_DMA_WAITOK);
	if (error)
		goto destroy;

	memset(sc->sc_dmap, 0, sc->sc_dmasize);

	return 0;

destroy:
	bus_dmamap_destroy(sc->sc_dmat, sc->sc_dmamap);
unmap:
	bus_dmamem_unmap(sc->sc_dmat, sc->sc_dmap, sc->sc_dmasize);
free:
	bus_dmamem_free(sc->sc_dmat, sc->sc_dmasegs, nsegs);

	sc->sc_dmasize = 0;
	sc->sc_dmap = NULL;

	return error;
}

void
amlogic_genfb_set_console_dev(device_t dev)
{
	KASSERT(amlogic_genfb_console_dev == NULL);
	amlogic_genfb_console_dev = dev;
}

void
amlogic_genfb_ddb_trap_callback(int where)
{
	if (amlogic_genfb_console_dev == NULL)
		return;

	if (where) {
		genfb_enable_polling(amlogic_genfb_console_dev);
	} else {
		genfb_disable_polling(amlogic_genfb_console_dev);
	}
}
