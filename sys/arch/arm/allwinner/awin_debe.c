/* $NetBSD: awin_debe.c,v 1.20.16.2 2017/12/03 11:35:50 jdolecek Exp $ */

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

#include "opt_allwinner.h"
#include "genfb.h"
#include "awin_mp.h"
#include "awin_tcon.h"

#ifndef AWIN_DEBE_VIDEOMEM
#define AWIN_DEBE_VIDEOMEM	(16 * 1024 * 1024)
#endif

#define AWIN_DEBE_CURMAX	64

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: awin_debe.c,v 1.20.16.2 2017/12/03 11:35:50 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/mutex.h>
#include <sys/condvar.h>

#include <arm/allwinner/awin_reg.h>
#include <arm/allwinner/awin_var.h>

#include <dev/videomode/videomode.h>
#include <dev/wscons/wsconsio.h>

struct awin_debe_softc {
	device_t sc_dev;
	device_t sc_fbdev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	bus_space_handle_t sc_ccm_bsh;
	bus_dma_tag_t sc_dmat;
	unsigned int sc_port;

	bus_dma_segment_t sc_dmasegs[1];
	bus_size_t sc_dmasize;
	bus_dmamap_t sc_dmamap;
	void *sc_dmap;

	uint16_t sc_margin;

	bool sc_cursor_enable;
	int sc_cursor_x, sc_cursor_y;
	int sc_hot_x, sc_hot_y;
	uint8_t sc_cursor_bitmap[8 * AWIN_DEBE_CURMAX];
	uint8_t sc_cursor_mask[8 * AWIN_DEBE_CURMAX];
};

#define DEBE_READ(sc, reg) \
    bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define DEBE_WRITE(sc, reg, val) \
    bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static int	awin_debe_match(device_t, cfdata_t, void *);
static void	awin_debe_attach(device_t, device_t, void *);

static int	awin_debe_alloc_videomem(struct awin_debe_softc *);
static void	awin_debe_setup_fbdev(struct awin_debe_softc *,
				      const struct videomode *);

static int	awin_debe_set_curpos(struct awin_debe_softc *, int, int);
static int	awin_debe_set_cursor(struct awin_debe_softc *,
				     struct wsdisplay_cursor *);

CFATTACH_DECL_NEW(awin_debe, sizeof(struct awin_debe_softc),
	awin_debe_match, awin_debe_attach, NULL, NULL);

static int
awin_debe_match(device_t parent, cfdata_t cf, void *aux)
{
	struct awinio_attach_args * const aio = aux;
	const struct awin_locators * const loc = &aio->aio_loc;

	if (strcmp(cf->cf_name, loc->loc_name))
		return 0;

	return 1;
}

static void
awin_debe_attach(device_t parent, device_t self, void *aux)
{
	struct awin_debe_softc *sc = device_private(self);
	struct awinio_attach_args * const aio = aux;
	const struct awin_locators * const loc = &aio->aio_loc;
	prop_dictionary_t cfg = device_properties(self);
#if NAWIN_MP > 0
	device_t mpdev;
#endif
#ifdef AWIN_DEBE_FWINIT
	struct videomode mode;
#endif
	int error;

	sc->sc_dev = self;
	sc->sc_bst = aio->aio_core_bst;
	sc->sc_dmat = aio->aio_dmat;
	sc->sc_port = loc->loc_port;
	bus_space_subregion(sc->sc_bst, aio->aio_core_bsh,
	    loc->loc_offset, loc->loc_size, &sc->sc_bsh);
	bus_space_subregion(sc->sc_bst, aio->aio_ccm_bsh, 0, 0x1000,
	    &sc->sc_ccm_bsh);

	aprint_naive("\n");
	aprint_normal(": Display Engine Backend (BE%d)\n", loc->loc_port);

	prop_dictionary_get_uint16(cfg, "margin", &sc->sc_margin);

	if (awin_chip_id() == AWIN_CHIP_ID_A31) {
		awin_reg_set_clear(aio->aio_core_bst, aio->aio_ccm_bsh,
		    AWIN_A31_AHB_RESET1_REG,
		    AWIN_A31_AHB_RESET1_BE0_RST << loc->loc_port,
		    0);
	} else if (awin_chip_id() == AWIN_CHIP_ID_A20) {
		awin_reg_set_clear(aio->aio_core_bst, aio->aio_ccm_bsh,
		    AWIN_BE0_SCLK_CFG_REG + (loc->loc_port * 4),
		    AWIN_BEx_CLK_RST,
		    0);
	}

	if (awin_chip_id() == AWIN_CHIP_ID_A31) {
		uint32_t pll6_freq = awin_pll6_get_rate() * 2;
		unsigned int clk_div = (pll6_freq + 299999999) / 300000000;

#ifdef AWIN_DEBE_DEBUG
		device_printf(sc->sc_dev, "PLL6 @ %u Hz\n", pll6_freq);
		device_printf(sc->sc_dev, "div %d\n", clk_div);
#endif
		awin_reg_set_clear(aio->aio_core_bst, aio->aio_ccm_bsh,
		    AWIN_BE0_SCLK_CFG_REG + (loc->loc_port * 4),
		    __SHIFTIN(AWIN_A31_BEx_CLK_SRC_SEL_PLL6_2X,
			      AWIN_A31_BEx_CLK_SRC_SEL) |
		    __SHIFTIN(clk_div - 1, AWIN_BEx_CLK_DIV_RATIO_M),
		    AWIN_A31_BEx_CLK_SRC_SEL | AWIN_BEx_CLK_DIV_RATIO_M);
	} else if (awin_chip_id() == AWIN_CHIP_ID_A20) {
		uint32_t pll5x_freq = awin_pll5x_get_rate();
		unsigned int clk_div = (pll5x_freq + 299999999) / 300000000;

#ifdef AWIN_DEBE_DEBUG
		device_printf(sc->sc_dev, "PLL5x @ %u Hz\n", pll5x_freq);
		device_printf(sc->sc_dev, "div %d\n", clk_div);
#endif

		awin_reg_set_clear(aio->aio_core_bst, aio->aio_ccm_bsh,
		    AWIN_BE0_SCLK_CFG_REG + (loc->loc_port * 4),
		    __SHIFTIN(AWIN_BEx_CLK_SRC_SEL_PLL5, AWIN_BEx_CLK_SRC_SEL) |
		    __SHIFTIN(clk_div - 1, AWIN_BEx_CLK_DIV_RATIO_M),
		    AWIN_BEx_CLK_SRC_SEL | AWIN_BEx_CLK_DIV_RATIO_M);
	}

	if (awin_chip_id() == AWIN_CHIP_ID_A20 ||
	    awin_chip_id() == AWIN_CHIP_ID_A31) {
		awin_reg_set_clear(aio->aio_core_bst, aio->aio_ccm_bsh,
		    AWIN_AHB_GATING1_REG,
		    AWIN_AHB_GATING1_DE_BE0 << loc->loc_port, 0);

		awin_reg_set_clear(aio->aio_core_bst, aio->aio_ccm_bsh,
		    AWIN_DRAM_CLK_REG,
		    AWIN_DRAM_CLK_BE0_DCLK_ENABLE << loc->loc_port, 0);

		awin_reg_set_clear(aio->aio_core_bst, aio->aio_ccm_bsh,
		    AWIN_BE0_SCLK_CFG_REG + (loc->loc_port * 4),
		    AWIN_CLK_ENABLE, 0);
	}

#ifdef AWIN_DEBE_FWINIT
	const uint32_t modctl = DEBE_READ(sc, AWIN_DEBE_MODCTL_REG);
	const uint32_t dissize = DEBE_READ(sc, AWIN_DEBE_DISSIZE_REG);
	if ((modctl & AWIN_DEBE_MODCTL_EN) == 0) {
		aprint_error_dev(sc->sc_dev, "disabled\n");
		return;
	}
	if ((modctl & AWIN_DEBE_MODCTL_START_CTL) == 0) {
		aprint_error_dev(sc->sc_dev, "stopped\n");
		return;
	}
	memset(&mode, 0, sizeof(mode));
	mode.hdisplay = (dissize & 0xffff) + 1;
	mode.vdisplay = ((dissize >> 16) & 0xffff) + 1;

	if (mode.hdisplay == 1 || mode.vdisplay == 1) {
		aprint_error_dev(sc->sc_dev,
		    "couldn't determine video mode\n");
		return;
	}

	aprint_verbose_dev(sc->sc_dev, "using %dx%d mode from firmware\n",
	    mode.hdisplay, mode.vdisplay);

	sc->sc_dmasize = mode.hdisplay * mode.vdisplay * 4;
#else
	for (unsigned int reg = 0x800; reg < 0x1000; reg += 4) {
		DEBE_WRITE(sc, reg, 0);
	}

	DEBE_WRITE(sc, AWIN_DEBE_MODCTL_REG, AWIN_DEBE_MODCTL_EN);

	sc->sc_dmasize = AWIN_DEBE_VIDEOMEM;
#endif

	DEBE_WRITE(sc, AWIN_DEBE_HWC_PALETTE_TABLE, 0);

	error = awin_debe_alloc_videomem(sc);
	if (error) {
		aprint_error_dev(sc->sc_dev,
		    "couldn't allocate video memory, error = %d\n", error);
		return;
	}

#if NAWIN_MP > 0
	mpdev = device_find_by_driver_unit("awinmp", 0);
	if (mpdev) {
		paddr_t pa = sc->sc_dmamap->dm_segs[0].ds_addr;
		if (pa >= AWIN_SDRAM_PBASE)
			pa -= AWIN_SDRAM_PBASE;
		awin_mp_setbase(mpdev, pa, sc->sc_dmasize);
	}
#endif

#ifdef AWIN_DEBE_FWINIT
	awin_debe_set_videomode(device_unit(self), &mode);
	awin_debe_enable(device_unit(self), true);
#endif
}

static int
awin_debe_alloc_videomem(struct awin_debe_softc *sc)
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

static void
awin_debe_setup_fbdev(struct awin_debe_softc *sc, const struct videomode *mode)
{
	if (mode == NULL)
		return;

	const u_int interlace_p = !!(mode->flags & VID_INTERLACE);
	const u_int fb_width = mode->hdisplay - (sc->sc_margin * 2);
	const u_int fb_height = (mode->vdisplay << interlace_p) -
				(sc->sc_margin * 2);

	if (mode && sc->sc_fbdev == NULL) {
		struct awinfb_attach_args afb = {
			.afb_fb = sc->sc_dmap,
			.afb_width = fb_width,
			.afb_height = fb_height,
			.afb_dmat = sc->sc_dmat,
			.afb_dmasegs = sc->sc_dmasegs,
			.afb_ndmasegs = 1
		};
		sc->sc_fbdev = config_found_ia(sc->sc_dev, "awindebe",
		    &afb, NULL);
	}
#if NGENFB > 0
	else if (sc->sc_fbdev != NULL) {
		awin_fb_set_videomode(sc->sc_fbdev, fb_width, fb_height);
	}
#endif
}

static int
awin_debe_set_curpos(struct awin_debe_softc *sc, int x, int y)
{
	int xx, yy;
	u_int yoff, xoff;

	xoff = yoff = 0;
	xx = x - sc->sc_hot_x + sc->sc_margin;
	yy = y - sc->sc_hot_y + sc->sc_margin;
	if (xx < 0) {
		xoff -= xx;
		xx = 0;
	}
	if (yy < 0) {
		yoff -= yy;
		yy = 0;
	}

	DEBE_WRITE(sc, AWIN_DEBE_HWCCTL_REG,
	    __SHIFTIN(yy, AWIN_DEBE_HWCCTL_YCOOR) |
	    __SHIFTIN(xx, AWIN_DEBE_HWCCTL_XCOOR));
	DEBE_WRITE(sc, AWIN_DEBE_HWCFBCTL_REG,
#if AWIN_DEBE_CURMAX == 32
	    __SHIFTIN(AWIN_DEBE_HWCFBCTL_YSIZE_32, AWIN_DEBE_HWCFBCTL_YSIZE) |
	    __SHIFTIN(AWIN_DEBE_HWCFBCTL_XSIZE_32, AWIN_DEBE_HWCFBCTL_XSIZE) |
#else
	    __SHIFTIN(AWIN_DEBE_HWCFBCTL_YSIZE_64, AWIN_DEBE_HWCFBCTL_YSIZE) |
	    __SHIFTIN(AWIN_DEBE_HWCFBCTL_XSIZE_64, AWIN_DEBE_HWCFBCTL_XSIZE) |
#endif
	    __SHIFTIN(AWIN_DEBE_HWCFBCTL_FBFMT_2BPP, AWIN_DEBE_HWCFBCTL_FBFMT) |
	    __SHIFTIN(yoff, AWIN_DEBE_HWCFBCTL_YCOOROFF) |
	    __SHIFTIN(xoff, AWIN_DEBE_HWCFBCTL_XCOOROFF));

	return 0;
}

static int
awin_debe_set_cursor(struct awin_debe_softc *sc, struct wsdisplay_cursor *cur)
{
	uint32_t val;
	uint8_t r[4], g[4], b[4];
	u_int index, count, shift, off, pcnt;
	int i, j, idx, error;
	uint8_t mask;

	if (cur->which & WSDISPLAY_CURSOR_DOCUR) {
		val = DEBE_READ(sc, AWIN_DEBE_MODCTL_REG);
		if (cur->enable)
			val |= AWIN_DEBE_MODCTL_HWC_EN;
		else
			val &= ~AWIN_DEBE_MODCTL_HWC_EN;
		DEBE_WRITE(sc, AWIN_DEBE_MODCTL_REG, val);

		sc->sc_cursor_enable = cur->enable;
	}

	if (cur->which & WSDISPLAY_CURSOR_DOHOT) {
		sc->sc_hot_x = cur->hot.x;
		sc->sc_hot_y = cur->hot.y;
		cur->which |= WSDISPLAY_CURSOR_DOPOS;
	}

	if (cur->which & WSDISPLAY_CURSOR_DOPOS) {
		awin_debe_set_curpos(sc, cur->pos.x, cur->pos.y);
	}

	if (cur->which & WSDISPLAY_CURSOR_DOCMAP) {
		index = cur->cmap.index;
		count = cur->cmap.count;
		if (index >= 2 || (index + count) > 2)
			return EINVAL;
		error = copyin(cur->cmap.red, &r[index], count);
		if (error)
			return error;
		error = copyin(cur->cmap.green, &g[index], count);
		if (error)
			return error;
		error = copyin(cur->cmap.blue, &b[index], count);
		if (error)
			return error;

		for (i = index; i < (index + count); i++) {
			DEBE_WRITE(sc,
			    AWIN_DEBE_HWC_PALETTE_TABLE + (4 * (i + 2)),
			    (r[i] << 16) | (g[i] << 8) | b[i] | 0xff000000);
		}
	}

	if (cur->which & WSDISPLAY_CURSOR_DOSHAPE) {
		error = copyin(cur->mask, sc->sc_cursor_mask,
		    AWIN_DEBE_CURMAX * 8);
		if (error)
			return error;
		error = copyin(cur->image, sc->sc_cursor_bitmap,
		    AWIN_DEBE_CURMAX * 8);
		if (error)
			return error;
	}

	if (cur->which & (WSDISPLAY_CURSOR_DOCMAP|WSDISPLAY_CURSOR_DOSHAPE)) {
		for (i = 0, pcnt = 0; i < AWIN_DEBE_CURMAX * 8; i++) {
			for (j = 0, mask = 1; j < 8; j++, mask <<= 1, pcnt++) {
				idx = ((sc->sc_cursor_mask[i] & mask) ? 2 : 0) |
				    ((sc->sc_cursor_bitmap[i] & mask) ? 1 : 0);
				off = (pcnt >> 4) * 4;
				shift = (pcnt & 0xf) * 2;
				val = DEBE_READ(sc,
				    AWIN_DEBE_HWC_PATTERN_BLOCK + off);
				val &= ~(3 << shift);
				val |= (idx << shift);
				DEBE_WRITE(sc,
				    AWIN_DEBE_HWC_PATTERN_BLOCK + off, val);
			}
		}
	}

	return 0;
}

void
awin_debe_enable(int unit, bool enable)
{
	struct awin_debe_softc *sc;
	device_t dev;
	uint32_t val;

	dev = device_find_by_driver_unit("awindebe", unit);
	if (dev == NULL) {
		printf("DEBE%d: no driver found\n", unit);
		return;
	}
	sc = device_private(dev);
	KASSERT(device_unit(sc->sc_dev) == unit);

	if (enable) {
		val = DEBE_READ(sc, AWIN_DEBE_REGBUFFCTL_REG);
		val |= AWIN_DEBE_REGBUFFCTL_REGLOADCTL;
		DEBE_WRITE(sc, AWIN_DEBE_REGBUFFCTL_REG, val);

		val = DEBE_READ(sc, AWIN_DEBE_MODCTL_REG);
		val |= AWIN_DEBE_MODCTL_START_CTL;
		DEBE_WRITE(sc, AWIN_DEBE_MODCTL_REG, val);
	} else {
		val = DEBE_READ(sc, AWIN_DEBE_MODCTL_REG);
		val &= ~AWIN_DEBE_MODCTL_START_CTL;
		DEBE_WRITE(sc, AWIN_DEBE_MODCTL_REG, val);
	}
#if 0
	for (int i = 0; i < 0x1000; i += 4) {
		printf("DEBE 0x%04x: 0x%08x\n", i, DEBE_READ(sc, i));
	}
#endif
}

void
awin_debe_set_videomode(int unit, const struct videomode *mode)
{
	struct awin_debe_softc *sc;
	device_t dev;
	uint32_t val;

	dev = device_find_by_driver_unit("awindebe", unit);
	if (dev == NULL) {
		printf("DEBE%d: no driver found\n", unit);
		return;
	}
	sc = device_private(dev);
	KASSERT(device_unit(sc->sc_dev) == unit);

	if (mode) {
		const u_int interlace_p = !!(mode->flags & VID_INTERLACE);
		const u_int width = mode->hdisplay;
		const u_int height = (mode->vdisplay << interlace_p);
		const u_int fb_width = width - (sc->sc_margin * 2);
		const u_int fb_height = height - (sc->sc_margin * 2);
		uint32_t vmem = width * height * 4;

		if (vmem > sc->sc_dmasize) {
			device_printf(sc->sc_dev,
			    "not enough memory for %ux%u fb (req %u have %u)\n",
			    width, height, vmem, (unsigned int)sc->sc_dmasize);
			return;
		}

		paddr_t pa = sc->sc_dmamap->dm_segs[0].ds_addr;
#if !defined(ALLWINNER_A80)
		/*
		 * On 2GB systems, we need to subtract AWIN_SDRAM_PBASE from
		 * the phys addr.
		 */
		if (pa >= AWIN_SDRAM_PBASE)
			pa -= AWIN_SDRAM_PBASE;
#endif

		/* notify fb */
		awin_debe_setup_fbdev(sc, mode);

		DEBE_WRITE(sc, AWIN_DEBE_DISSIZE_REG,
		    ((height - 1) << 16) | (width - 1));
		DEBE_WRITE(sc, AWIN_DEBE_LAYSIZE_REG,
		    ((fb_height - 1) << 16) | (fb_width - 1));
		DEBE_WRITE(sc, AWIN_DEBE_LAYCOOR_REG,
		    (sc->sc_margin << 16) | sc->sc_margin);
		DEBE_WRITE(sc, AWIN_DEBE_LAYLINEWIDTH_REG, (fb_width << 5));
		DEBE_WRITE(sc, AWIN_DEBE_LAYFB_L32ADD_REG, pa << 3);
		DEBE_WRITE(sc, AWIN_DEBE_LAYFB_H4ADD_REG, pa >> 29);

		val = DEBE_READ(sc, AWIN_DEBE_ATTCTL1_REG);
		val &= ~AWIN_DEBE_ATTCTL1_LAY_FBFMT;
		val |= __SHIFTIN(AWIN_DEBE_ATTCTL1_LAY_FBFMT_XRGB8888,
				 AWIN_DEBE_ATTCTL1_LAY_FBFMT);
		val &= ~AWIN_DEBE_ATTCTL1_LAY_BRSWAPEN;
		val &= ~AWIN_DEBE_ATTCTL1_LAY_FBPS;
#if __ARMEB__
		val |= __SHIFTIN(AWIN_DEBE_ATTCTL1_LAY_FBPS_32BPP_BGRA,
				 AWIN_DEBE_ATTCTL1_LAY_FBPS);
#else
		val |= __SHIFTIN(AWIN_DEBE_ATTCTL1_LAY_FBPS_32BPP_ARGB,
				 AWIN_DEBE_ATTCTL1_LAY_FBPS);
#endif
		DEBE_WRITE(sc, AWIN_DEBE_ATTCTL1_REG, val);

		val = DEBE_READ(sc, AWIN_DEBE_MODCTL_REG);
		val |= AWIN_DEBE_MODCTL_LAY0_EN;
		if (interlace_p) {
			val |= AWIN_DEBE_MODCTL_ITLMOD_EN;
		} else {
			val &= ~AWIN_DEBE_MODCTL_ITLMOD_EN;
		}
		val &= ~AWIN_DEBE_MODCTL_OUT_SEL;
		if (device_unit(sc->sc_dev) == 1) {
			val |= __SHIFTIN(AWIN_DEBE_MODCTL_OUT_SEL_LCD1,
			    AWIN_DEBE_MODCTL_OUT_SEL);
		}
		DEBE_WRITE(sc, AWIN_DEBE_MODCTL_REG, val);
	} else {
		/* disable */
		val = DEBE_READ(sc, AWIN_DEBE_MODCTL_REG);
		val &= ~AWIN_DEBE_MODCTL_LAY0_EN;
		val &= ~AWIN_DEBE_MODCTL_START_CTL;
		DEBE_WRITE(sc, AWIN_DEBE_MODCTL_REG, val);

		/* notify fb */
		awin_debe_setup_fbdev(sc, mode);
	}
}

int
awin_debe_ioctl(device_t self, u_long cmd, void *data)
{
	struct awin_debe_softc *sc = device_private(self);
	struct wsdisplay_curpos *cp;
	uint32_t val;
	int enable;

	switch (cmd) {
	case WSDISPLAYIO_SVIDEO:
		enable = *(int *)data;
		val = DEBE_READ(sc, AWIN_DEBE_MODCTL_REG);
		if (enable) {
			if (val & AWIN_DEBE_MODCTL_LAY0_EN) {
				/* already enabled */
				return 0;
			}
			val |= AWIN_DEBE_MODCTL_LAY0_EN;
			if (sc->sc_cursor_enable) {
				val |= AWIN_DEBE_MODCTL_HWC_EN;
			} else {
				val &= ~AWIN_DEBE_MODCTL_HWC_EN;
			}
		} else {
			if ((val & AWIN_DEBE_MODCTL_LAY0_EN) == 0) {
				/* already disabled */
				return 0;
			}
			val &= ~AWIN_DEBE_MODCTL_LAY0_EN;
			val &= ~AWIN_DEBE_MODCTL_HWC_EN;
		}
		DEBE_WRITE(sc, AWIN_DEBE_MODCTL_REG, val);
#if NAWIN_TCON > 0
		/* debe0 always connected to tcon0, debe1 to tcon1*/
		awin_tcon_setvideo(device_unit(sc->sc_dev), enable);
#endif
		return 0;
	case WSDISPLAYIO_GVIDEO:
		val = DEBE_READ(sc, AWIN_DEBE_MODCTL_REG);
		*(int *)data = !!(val & AWIN_DEBE_MODCTL_LAY0_EN);
		return 0;
	case WSDISPLAYIO_GCURPOS:
		cp = data;
		cp->x = sc->sc_cursor_x;
		cp->y = sc->sc_cursor_y;
		return 0;
	case WSDISPLAYIO_SCURPOS:
		cp = data;
		return awin_debe_set_curpos(sc, cp->x, cp->y);
	case WSDISPLAYIO_GCURMAX:
		cp = data;
		cp->x = AWIN_DEBE_CURMAX;
		cp->y = AWIN_DEBE_CURMAX;
		return 0;
	case WSDISPLAYIO_SCURSOR:
		return awin_debe_set_cursor(sc, data);
	}

	return EPASSTHROUGH;
}

