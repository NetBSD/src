/* $NetBSD: amlogic_genfb.c,v 1.1 2015/03/21 01:17:00 jmcneill Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: amlogic_genfb.c,v 1.1 2015/03/21 01:17:00 jmcneill Exp $");

#include <sys/param.h>
#include <sys/types.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/bus.h>
#include <sys/kmem.h>

#include <arm/amlogic/amlogic_reg.h>
#include <arm/amlogic/amlogic_var.h>
#include <arm/amlogic/amlogic_canvasreg.h>
#include <arm/amlogic/amlogic_vpureg.h>
#include <arm/amlogic/amlogic_hdmireg.h>

#include <dev/wsfb/genfbvar.h>

/* Map CEA-861-D video code (VIC) to framebuffer dimensions */
static const struct amlogic_genfb_vic2mode {
	u_int vic;
	u_int width;
	u_int height;
} amlogic_genfb_modes[] = {
	{ 1, 640, 480 },
	{ 2, 720, 480 },
	{ 3, 720, 480 },
	{ 4, 1280, 720 },
	{ 5, 1920, 1080 },
	{ 16, 1920, 1080 },
	{ 17, 720, 576 },
	{ 18, 720, 576 },
	{ 19, 1280, 720 },
	{ 20, 1920, 1080 },
	{ 31, 1920, 1080 },
	{ 32, 1920, 1080 },
	{ 33, 1920, 1080 },
	{ 34, 1920, 1080 },
	{ 39, 1920, 1080 },
};

struct amlogic_genfb_softc {
	struct genfb_softc	sc_gen;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	bus_space_handle_t	sc_hdmi_bsh;
	bus_space_handle_t	sc_vpu_bsh;
	bus_dma_tag_t		sc_dmat;

	bus_dma_segment_t	sc_dmasegs[1];
	bus_size_t		sc_dmasize;
	bus_dmamap_t		sc_dmamap;
	void			*sc_dmap;

	uint32_t		sc_wstype;
};

static int	amlogic_genfb_match(device_t, cfdata_t, void *);
static void	amlogic_genfb_attach(device_t, device_t, void *);

static int	amlogic_genfb_ioctl(void *, void *, u_long, void *, int, lwp_t *);
static paddr_t	amlogic_genfb_mmap(void *, void *, off_t, int);
static bool	amlogic_genfb_shutdown(device_t, int);

static void	amlogic_genfb_probe(struct amlogic_genfb_softc *);
static int	amlogic_genfb_alloc_videomem(struct amlogic_genfb_softc *);

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

static inline void
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
	    loc->loc_offset, loc->loc_size, &sc->sc_bsh);
	bus_space_subregion(aio->aio_core_bst, aio->aio_bsh,
	    AMLOGIC_HDMI_OFFSET, AMLOGIC_HDMI_SIZE, &sc->sc_hdmi_bsh);
	bus_space_subregion(aio->aio_core_bst, aio->aio_bsh,
	    AMLOGIC_VPU_OFFSET, AMLOGIC_VPU_SIZE, &sc->sc_vpu_bsh);

	amlogic_genfb_probe(sc);

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
amlogic_genfb_probe(struct amlogic_genfb_softc *sc)
{
	prop_dictionary_t cfg = device_properties(sc->sc_gen.sc_dev);
	u_int width = 0, height = 0, i;
	uint32_t datal, datah, addr;
	uint32_t w1, w2, w3, w4;
	int error;

	datal = bus_space_read_4(sc->sc_bst, sc->sc_bsh, DC_CAV_LUT_DATAL_REG);
	datah = bus_space_read_4(sc->sc_bst, sc->sc_bsh, DC_CAV_LUT_DATAH_REG);
	addr = bus_space_read_4(sc->sc_bst, sc->sc_bsh, DC_CAV_LUT_ADDR_REG);

	w1 = VPU_READ(sc, VIU_OSD2_BLK0_CFG_W1_REG);
	w2 = VPU_READ(sc, VIU_OSD2_BLK0_CFG_W2_REG);
	w3 = VPU_READ(sc, VIU_OSD2_BLK0_CFG_W3_REG);
	w4 = VPU_READ(sc, VIU_OSD2_BLK0_CFG_W4_REG);

#if 0
	const u_int w = (__SHIFTOUT(datal, DC_CAV_LUT_DATAL_WIDTH_L) |
	    (__SHIFTOUT(datah, DC_CAV_LUT_DATAH_WIDTH_H) << 3)) << 3;
	const u_int h = __SHIFTOUT(datah, DC_CAV_LUT_DATAH_HEIGHT);
#endif

	/* VIC is in AVI InfoFrame PB4. */
	const uint32_t vic = HDMI_READ(sc, HDMITX_AVI_INFO_ADDR + 4) & 0x7f;

	for (i = 0; i < __arraycount(amlogic_genfb_modes); i++) {
		if (amlogic_genfb_modes[i].vic == vic) {
			aprint_debug(" [HDMI VIC %d]", vic);
			width = amlogic_genfb_modes[i].width;
			height = amlogic_genfb_modes[i].height;
		}
	}

	if (width == 0 || height == 0) {
		aprint_error(" [UNSUPPORTED HDMI VIC %d]", vic);
		return;
	}

	const uint32_t fbsize = width * height * 3;
	sc->sc_dmasize = (fbsize + 3) & ~3;

	error = amlogic_genfb_alloc_videomem(sc);
	if (error == 0) {
		const paddr_t pa = sc->sc_dmamap->dm_segs[0].ds_addr;
		const uint32_t w = width * 3;
		const uint32_t h = height;

		datal &= ~DC_CAV_LUT_DATAL_WIDTH_L;
		datal |= __SHIFTIN(w >> 3, DC_CAV_LUT_DATAL_WIDTH_L);
		datal &= ~DC_CAV_LUT_DATAL_FBADDR;
		datal |= __SHIFTIN(pa >> 3, DC_CAV_LUT_DATAL_FBADDR);
		bus_space_write_4(sc->sc_bst, sc->sc_bsh, DC_CAV_LUT_DATAL_REG,
		    datal);

		datah &= ~DC_CAV_LUT_DATAH_BLKMODE;
		datah |= __SHIFTIN(DC_CAV_LUT_DATAH_BLKMODE_LINEAR,
				   DC_CAV_LUT_DATAH_BLKMODE);
		datah &= ~DC_CAV_LUT_DATAH_WIDTH_H;
		datah |= __SHIFTIN(w >> 6, DC_CAV_LUT_DATAH_WIDTH_H);
		datah &= ~DC_CAV_LUT_DATAH_HEIGHT;
		datah |= __SHIFTIN(h, DC_CAV_LUT_DATAH_HEIGHT);
		bus_space_write_4(sc->sc_bst, sc->sc_bsh, DC_CAV_LUT_DATAH_REG,
		    datah);

		addr |= DC_CAV_LUT_ADDR_WR_EN;
		bus_space_write_4(sc->sc_bst, sc->sc_bsh, DC_CAV_LUT_ADDR_REG,
		    addr);

		w1 = __SHIFTIN(width - 1, VIU_OSD_BLK_CFG_W1_X_END);
		w2 = __SHIFTIN(height - 1, VIU_OSD_BLK_CFG_W2_Y_END);
		w3 = __SHIFTIN(width - 1, VIU_OSD_BLK_CFG_W3_H_END);
		w4 = __SHIFTIN(height - 1, VIU_OSD_BLK_CFG_W4_V_END);

		VPU_WRITE(sc, VIU_OSD2_BLK0_CFG_W1_REG, w1);
		VPU_WRITE(sc, VIU_OSD2_BLK0_CFG_W2_REG, w2);
		VPU_WRITE(sc, VIU_OSD2_BLK0_CFG_W3_REG, w3);
		VPU_WRITE(sc, VIU_OSD2_BLK0_CFG_W4_REG, w4);

		prop_dictionary_set_uint32(cfg, "width", width);
		prop_dictionary_set_uint32(cfg, "height", height);
		prop_dictionary_set_uint8(cfg, "depth", 24);
		prop_dictionary_set_uint16(cfg, "linebytes", width * 3);
		prop_dictionary_set_uint32(cfg, "address", 0);
		prop_dictionary_set_uint32(cfg, "virtual_address",
		    (uintptr_t)sc->sc_dmap);
	} else {
		aprint_error_dev(sc->sc_gen.sc_dev,
		    "failed to allocate %u bytes of video memory: %d\n",
		    (u_int)sc->sc_dmasize, error);
	}
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
