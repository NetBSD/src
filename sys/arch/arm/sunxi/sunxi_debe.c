/* $NetBSD: sunxi_debe.c,v 1.2.2.1 2018/04/07 04:12:12 pgoyette Exp $ */

/*-
 * Copyright (c) 2018 Manuel Bouyer <bouyer@antioche.eu.org>
 * All rights reserved.
 *
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

#include "genfb.h"

#ifndef SUNXI_DEBE_VIDEOMEM
#define SUNXI_DEBE_VIDEOMEM	(16 * 1024 * 1024)
#endif

#define SUNXI_DEBE_CURMAX	64

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sunxi_debe.c,v 1.2.2.1 2018/04/07 04:12:12 pgoyette Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/mutex.h>
#include <sys/condvar.h>

#include <dev/fdt/fdtvar.h>
#include <dev/fdt/fdt_port.h>

#include <dev/videomode/videomode.h>
#include <dev/wscons/wsconsio.h>
#include <dev/wsfb/genfbvar.h>

#include <arm/sunxi/sunxi_debereg.h>
#include <arm/sunxi/sunxi_display.h>

enum sunxi_debe_type {
	DEBE_A10 = 1,
};

struct sunxi_debe_softc {
	device_t sc_dev;
	device_t sc_fbdev;
	enum sunxi_debe_type sc_type;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	bus_dma_tag_t sc_dmat;

	struct clk *sc_clk_ahb;
	struct clk *sc_clk_mod;
	struct clk *sc_clk_ram;

	bus_dma_segment_t sc_dmasegs[1];
	bus_size_t sc_dmasize;
	bus_dmamap_t sc_dmamap;
	void *sc_dmap;

	bool sc_cursor_enable;
	int sc_cursor_x, sc_cursor_y;
	int sc_hot_x, sc_hot_y;
	uint8_t sc_cursor_bitmap[8 * SUNXI_DEBE_CURMAX];
	uint8_t sc_cursor_mask[8 * SUNXI_DEBE_CURMAX];

	int	sc_phandle;
	struct fdt_device_ports sc_ports;
	struct fdt_endpoint *sc_out_ep;
	int sc_unit; /* debe0 or debe1 */
};

#define DEBE_READ(sc, reg) \
    bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define DEBE_WRITE(sc, reg, val) \
    bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static const struct of_compat_data compat_data[] = {
	{"allwinner,sun4i-a10-display-backend", DEBE_A10},
	{"allwinner,sun7i-a20-display-backend", DEBE_A10},
	{NULL}
};

struct sunxifb_attach_args {
	void *afb_fb;
	uint32_t afb_width;
	uint32_t afb_height;
	bus_dma_tag_t afb_dmat;
	bus_dma_segment_t *afb_dmasegs;
	int afb_ndmasegs;
};

static void	sunxi_debe_ep_connect(device_t, struct fdt_endpoint *, bool);
static int	sunxi_debe_ep_enable(device_t, struct fdt_endpoint *, bool);
static int	sunxi_debe_match(device_t, cfdata_t, void *);
static void	sunxi_debe_attach(device_t, device_t, void *);

static int	sunxi_debe_alloc_videomem(struct sunxi_debe_softc *);
static void	sunxi_debe_setup_fbdev(struct sunxi_debe_softc *,
				      const struct videomode *);

static int	sunxi_debe_set_curpos(struct sunxi_debe_softc *, int, int);
static int	sunxi_debe_set_cursor(struct sunxi_debe_softc *,
				     struct wsdisplay_cursor *);
static int	sunxi_debe_ioctl(device_t, u_long, void *);
static void	sunxi_befb_set_videomode(device_t, u_int, u_int);
void sunxi_debe_dump_regs(int);

CFATTACH_DECL_NEW(sunxi_debe, sizeof(struct sunxi_debe_softc),
	sunxi_debe_match, sunxi_debe_attach, NULL, NULL);

static int
sunxi_debe_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compat_data(faa->faa_phandle, compat_data);
}

static void
sunxi_debe_attach(device_t parent, device_t self, void *aux)
{
	struct sunxi_debe_softc *sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;
	struct fdtbus_reset *rst;
	int error;

	sc->sc_dev = self;
	sc->sc_phandle = phandle;
	sc->sc_bst = faa->faa_bst;
	sc->sc_dmat = faa->faa_dmat;
	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
	}
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	sc->sc_clk_ahb = fdtbus_clock_get(phandle, "ahb");
	sc->sc_clk_mod = fdtbus_clock_get(phandle, "mod");
	sc->sc_clk_ram = fdtbus_clock_get(phandle, "ram");

	rst = fdtbus_reset_get_index(phandle, 0);
	if (rst == NULL) {
		aprint_error(": couldn't get reset\n");
		return;
	}
	if (fdtbus_reset_assert(rst) != 0) {
		aprint_error(": couldn't assert reset\n");
		return;
	}
	delay(1);
	if (fdtbus_reset_deassert(rst) != 0) {
		aprint_error(": couldn't de-assert reset\n");
		return;
	}

	if (sc->sc_clk_ahb == NULL || sc->sc_clk_mod == NULL
	    || sc->sc_clk_ram == NULL) {
		aprint_error(": couldn't get clocks\n");
		aprint_debug_dev(self, "clk ahb %s mod %s ram %s\n",
		    sc->sc_clk_ahb == NULL ? "missing" : "present",
		    sc->sc_clk_mod == NULL ? "missing" : "present",
		    sc->sc_clk_ram == NULL ? "missing" : "present");
		return;
	}

	error = clk_set_rate(sc->sc_clk_mod, 300000000);
	if (error) {
		aprint_error("couln't set mod clock rate (%d)\n", error);
		return;
	}

	if (clk_enable(sc->sc_clk_ahb) != 0 ||
	    clk_enable(sc->sc_clk_mod) != 0) {
		aprint_error(": couldn't enable clocks\n");
		return;
	}
	if (clk_disable(sc->sc_clk_ram) != 0) {
		aprint_error(": couldn't disable ram clock\n");
	}

	sc->sc_type = of_search_compatible(faa->faa_phandle, compat_data)->data;

	aprint_naive("\n");
	aprint_normal(": Display Engine Backend (%s)\n",
	    fdtbus_get_string(phandle, "name"));


	for (unsigned int reg = 0x800; reg < 0x1000; reg += 4) {
		DEBE_WRITE(sc, reg, 0);
	}

	DEBE_WRITE(sc, SUNXI_DEBE_MODCTL_REG, SUNXI_DEBE_MODCTL_EN);

	sc->sc_dmasize = SUNXI_DEBE_VIDEOMEM;

	DEBE_WRITE(sc, SUNXI_DEBE_HWC_PALETTE_TABLE, 0);

	error = sunxi_debe_alloc_videomem(sc);
	if (error) {
		aprint_error_dev(sc->sc_dev,
		    "couldn't allocate video memory, error = %d\n", error);
		return;
	}

	sc->sc_unit = -1;
	sc->sc_ports.dp_ep_connect = sunxi_debe_ep_connect;
	sc->sc_ports.dp_ep_enable = sunxi_debe_ep_enable;
	fdt_ports_register(&sc->sc_ports, self, phandle, EP_OTHER);

	if (clk_disable(sc->sc_clk_ahb) != 0 ||
	    clk_disable(sc->sc_clk_mod) != 0) {
		aprint_error(": couldn't disable clocks\n");
		return;
	}
}



static void
sunxi_debe_ep_connect(device_t self, struct fdt_endpoint *ep, bool connect)
{
	struct sunxi_debe_softc *sc = device_private(self);
	struct fdt_endpoint *rep = fdt_endpoint_remote(ep);
	int rep_idx = fdt_endpoint_index(rep);

	KASSERT(device_is_a(self, "sunxidebe"));
	if (!connect) {
		aprint_error_dev(self, "endpoint disconnect not supported\n");
		return;
	}

	if (fdt_endpoint_port_index(ep) == 1) {
		bool do_print = (sc->sc_unit == -1);
		/*
		 * one of our output endpoints has been connected.
		 * the remote id is our unit number
		 */
		if (sc->sc_unit != -1 && rep_idx != -1 &&
		    sc->sc_unit != rep_idx) {
			aprint_error_dev(self, ": remote id %d doens't match"
			    " discovered unit number %d\n",
			    rep_idx, sc->sc_unit);
			return;
		}
		if (!device_is_a(fdt_endpoint_device(rep), "sunxitcon")) {
			aprint_error_dev(self,
			    ": output %d connected to unknown device\n",
			    fdt_endpoint_index(ep));
			return;
		}
		if (rep_idx != -1)
			sc->sc_unit = rep_idx;
		else {
			/* assume only one debe */
			sc->sc_unit = 0;
		}
		if (do_print)
			aprint_verbose_dev(self, "debe unit %d\n", sc->sc_unit);
	}
}

static int
sunxi_debe_alloc_videomem(struct sunxi_debe_softc *sc)
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
sunxi_debe_setup_fbdev(struct sunxi_debe_softc *sc, const struct videomode *mode)
{
	if (mode == NULL)
		return;

	const u_int interlace_p = !!(mode->flags & VID_INTERLACE);
	const u_int fb_width = mode->hdisplay;
	const u_int fb_height = (mode->vdisplay << interlace_p);

	if (mode && sc->sc_fbdev == NULL) {
		struct sunxifb_attach_args afb = {
			.afb_fb = sc->sc_dmap,
			.afb_width = fb_width,
			.afb_height = fb_height,
			.afb_dmat = sc->sc_dmat,
			.afb_dmasegs = sc->sc_dmasegs,
			.afb_ndmasegs = 1
		};
		sc->sc_fbdev = config_found_ia(sc->sc_dev, "sunxidebe",
		    &afb, NULL);
	} else if (sc->sc_fbdev != NULL) {
		sunxi_befb_set_videomode(sc->sc_fbdev, fb_width, fb_height);
	}
}

static int
sunxi_debe_set_curpos(struct sunxi_debe_softc *sc, int x, int y)
{
	int xx, yy;
	u_int yoff, xoff;

	xoff = yoff = 0;
	xx = x - sc->sc_hot_x;
	yy = y - sc->sc_hot_y;
	if (xx < 0) {
		xoff -= xx;
		xx = 0;
	}
	if (yy < 0) {
		yoff -= yy;
		yy = 0;
	}

	DEBE_WRITE(sc, SUNXI_DEBE_HWCCTL_REG,
	    __SHIFTIN(yy, SUNXI_DEBE_HWCCTL_YCOOR) |
	    __SHIFTIN(xx, SUNXI_DEBE_HWCCTL_XCOOR));
	DEBE_WRITE(sc, SUNXI_DEBE_HWCFBCTL_REG,
#if SUNXI_DEBE_CURMAX == 32
	    __SHIFTIN(SUNXI_DEBE_HWCFBCTL_YSIZE_32, SUNXI_DEBE_HWCFBCTL_YSIZE) |
	    __SHIFTIN(SUNXI_DEBE_HWCFBCTL_XSIZE_32, SUNXI_DEBE_HWCFBCTL_XSIZE) |
#else
	    __SHIFTIN(SUNXI_DEBE_HWCFBCTL_YSIZE_64, SUNXI_DEBE_HWCFBCTL_YSIZE) |
	    __SHIFTIN(SUNXI_DEBE_HWCFBCTL_XSIZE_64, SUNXI_DEBE_HWCFBCTL_XSIZE) |
#endif
	    __SHIFTIN(SUNXI_DEBE_HWCFBCTL_FBFMT_2BPP, SUNXI_DEBE_HWCFBCTL_FBFMT) |
	    __SHIFTIN(yoff, SUNXI_DEBE_HWCFBCTL_YCOOROFF) |
	    __SHIFTIN(xoff, SUNXI_DEBE_HWCFBCTL_XCOOROFF));

	return 0;
}

static int
sunxi_debe_set_cursor(struct sunxi_debe_softc *sc, struct wsdisplay_cursor *cur)
{
	uint32_t val;
	uint8_t r[4], g[4], b[4];
	u_int index, count, shift, off, pcnt;
	int i, j, idx, error;
	uint8_t mask;

	if (cur->which & WSDISPLAY_CURSOR_DOCUR) {
		val = DEBE_READ(sc, SUNXI_DEBE_MODCTL_REG);
		if (cur->enable)
			val |= SUNXI_DEBE_MODCTL_HWC_EN;
		else
			val &= ~SUNXI_DEBE_MODCTL_HWC_EN;
		DEBE_WRITE(sc, SUNXI_DEBE_MODCTL_REG, val);

		sc->sc_cursor_enable = cur->enable;
	}

	if (cur->which & WSDISPLAY_CURSOR_DOHOT) {
		sc->sc_hot_x = cur->hot.x;
		sc->sc_hot_y = cur->hot.y;
		cur->which |= WSDISPLAY_CURSOR_DOPOS;
	}

	if (cur->which & WSDISPLAY_CURSOR_DOPOS) {
		sunxi_debe_set_curpos(sc, cur->pos.x, cur->pos.y);
	}

	if (cur->which & WSDISPLAY_CURSOR_DOCMAP) {
		index = cur->cmap.index;
		count = cur->cmap.count;
		if (index >= 2 || count > 2 - index)
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
			    SUNXI_DEBE_HWC_PALETTE_TABLE + (4 * (i + 2)),
			    (r[i] << 16) | (g[i] << 8) | b[i] | 0xff000000);
		}
	}

	if (cur->which & WSDISPLAY_CURSOR_DOSHAPE) {
		error = copyin(cur->mask, sc->sc_cursor_mask,
		    SUNXI_DEBE_CURMAX * 8);
		if (error)
			return error;
		error = copyin(cur->image, sc->sc_cursor_bitmap,
		    SUNXI_DEBE_CURMAX * 8);
		if (error)
			return error;
	}

	if (cur->which & (WSDISPLAY_CURSOR_DOCMAP|WSDISPLAY_CURSOR_DOSHAPE)) {
		for (i = 0, pcnt = 0; i < SUNXI_DEBE_CURMAX * 8; i++) {
			for (j = 0, mask = 1; j < 8; j++, mask <<= 1, pcnt++) {
				idx = ((sc->sc_cursor_mask[i] & mask) ? 2 : 0) |
				    ((sc->sc_cursor_bitmap[i] & mask) ? 1 : 0);
				off = (pcnt >> 4) * 4;
				shift = (pcnt & 0xf) * 2;
				val = DEBE_READ(sc,
				    SUNXI_DEBE_HWC_PATTERN_BLOCK + off);
				val &= ~(3 << shift);
				val |= (idx << shift);
				DEBE_WRITE(sc,
				    SUNXI_DEBE_HWC_PATTERN_BLOCK + off, val);
			}
		}
	}

	return 0;
}

static int
sunxi_debe_ep_enable(device_t dev, struct fdt_endpoint *ep, bool enable)
{
	struct sunxi_debe_softc *sc;
	uint32_t val;

	KASSERT(device_is_a(dev, "sunxidebe"));
	sc = device_private(dev);

	if (enable) {
		if (clk_enable(sc->sc_clk_ram) != 0) {
			device_printf(dev,
			    ": warning: failed to enable ram clock\n");
		}
		val = DEBE_READ(sc, SUNXI_DEBE_REGBUFFCTL_REG);
		val |= SUNXI_DEBE_REGBUFFCTL_REGLOADCTL;
		DEBE_WRITE(sc, SUNXI_DEBE_REGBUFFCTL_REG, val);

		val = DEBE_READ(sc, SUNXI_DEBE_MODCTL_REG);
		val |= SUNXI_DEBE_MODCTL_START_CTL;
		DEBE_WRITE(sc, SUNXI_DEBE_MODCTL_REG, val);
#ifdef SUNXI_DEBE_DEBUG
		sunxi_debe_dump_regs(sc->sc_unit);
#endif
	} else {
		val = DEBE_READ(sc, SUNXI_DEBE_MODCTL_REG);
		val &= ~SUNXI_DEBE_MODCTL_START_CTL;
		DEBE_WRITE(sc, SUNXI_DEBE_MODCTL_REG, val);
		if (clk_disable(sc->sc_clk_ram) != 0) {
			device_printf(dev,
			    ": warning: failed to disable ram clock\n");
		}
	}
#if 0
	for (int i = 0; i < 0x1000; i += 4) {
		printf("DEBE 0x%04x: 0x%08x\n", i, DEBE_READ(sc, i));
	}
#endif
	return 0;
}

void
sunxi_debe_set_videomode(device_t dev, const struct videomode *mode)
{
	struct sunxi_debe_softc *sc;
	uint32_t val;

	KASSERT(device_is_a(dev, "sunxidebe"));
	sc = device_private(dev);

	if (mode) {
		const u_int interlace_p = !!(mode->flags & VID_INTERLACE);
		const u_int width = mode->hdisplay;
		const u_int height = (mode->vdisplay << interlace_p);
		const u_int fb_width = width;
		const u_int fb_height = height;
		uint32_t vmem = width * height * 4;

		if (vmem > sc->sc_dmasize) {
			device_printf(sc->sc_dev,
			    "not enough memory for %ux%u fb (req %u have %u)\n",
			    width, height, vmem, (unsigned int)sc->sc_dmasize);
			return;
		}

		paddr_t pa = sc->sc_dmamap->dm_segs[0].ds_addr;
#if !defined(ALLWINNER_A80) && 0
#define SUNXI_SDRAM_PBASE-0 0x40000000
		/*
		 * On 2GB systems, we need to subtract AWIN_SDRAM_PBASE from
		 * the phys addr.
		 */
		if (pa >= SUNXI_SDRAM_PBASE)
			pa -= SUNXI_SDRAM_PBASE;
#endif

		/* notify fb */
		sunxi_debe_setup_fbdev(sc, mode);

		DEBE_WRITE(sc, SUNXI_DEBE_DISSIZE_REG,
		    ((height - 1) << 16) | (width - 1));
		DEBE_WRITE(sc, SUNXI_DEBE_LAYSIZE_REG,
		    ((fb_height - 1) << 16) | (fb_width - 1));
		DEBE_WRITE(sc, SUNXI_DEBE_LAYCOOR_REG, 0);
		DEBE_WRITE(sc, SUNXI_DEBE_LAYLINEWIDTH_REG, (fb_width << 5));
		DEBE_WRITE(sc, SUNXI_DEBE_LAYFB_L32ADD_REG, pa << 3);
		DEBE_WRITE(sc, SUNXI_DEBE_LAYFB_H4ADD_REG, pa >> 29);

		val = DEBE_READ(sc, SUNXI_DEBE_ATTCTL1_REG);
		val &= ~SUNXI_DEBE_ATTCTL1_LAY_FBFMT;
		val |= __SHIFTIN(SUNXI_DEBE_ATTCTL1_LAY_FBFMT_XRGB8888,
				 SUNXI_DEBE_ATTCTL1_LAY_FBFMT);
		val &= ~SUNXI_DEBE_ATTCTL1_LAY_BRSWAPEN;
		val &= ~SUNXI_DEBE_ATTCTL1_LAY_FBPS;
#if __ARMEB__
		val |= __SHIFTIN(SUNXI_DEBE_ATTCTL1_LAY_FBPS_32BPP_BGRA,
				 SUNXI_DEBE_ATTCTL1_LAY_FBPS);
#else
		val |= __SHIFTIN(SUNXI_DEBE_ATTCTL1_LAY_FBPS_32BPP_ARGB,
				 SUNXI_DEBE_ATTCTL1_LAY_FBPS);
#endif
		DEBE_WRITE(sc, SUNXI_DEBE_ATTCTL1_REG, val);

		val = DEBE_READ(sc, SUNXI_DEBE_MODCTL_REG);
		val |= SUNXI_DEBE_MODCTL_LAY0_EN;
		if (interlace_p) {
			val |= SUNXI_DEBE_MODCTL_ITLMOD_EN;
		} else {
			val &= ~SUNXI_DEBE_MODCTL_ITLMOD_EN;
		}
		val &= ~SUNXI_DEBE_MODCTL_OUT_SEL;
		if (sc->sc_unit == 1) {
			val |= __SHIFTIN(SUNXI_DEBE_MODCTL_OUT_SEL_LCD1,
			    SUNXI_DEBE_MODCTL_OUT_SEL);
		}
		DEBE_WRITE(sc, SUNXI_DEBE_MODCTL_REG, val);
	} else {
		/* disable */
		val = DEBE_READ(sc, SUNXI_DEBE_MODCTL_REG);
		val &= ~SUNXI_DEBE_MODCTL_LAY0_EN;
		val &= ~SUNXI_DEBE_MODCTL_START_CTL;
		DEBE_WRITE(sc, SUNXI_DEBE_MODCTL_REG, val);

		/* notify fb */
		sunxi_debe_setup_fbdev(sc, mode);
	}
}

static int
sunxi_debe_ioctl(device_t self, u_long cmd, void *data)
{
	struct sunxi_debe_softc *sc = device_private(self);
	struct wsdisplay_curpos *cp;
	uint32_t val;
	int enable;

	switch (cmd) {
	case WSDISPLAYIO_SVIDEO:
		enable = *(int *)data;
		val = DEBE_READ(sc, SUNXI_DEBE_MODCTL_REG);
		if (enable) {
			if (val & SUNXI_DEBE_MODCTL_START_CTL) {
				/* already enabled */
				return 0;
			}
		} else {
			if ((val & SUNXI_DEBE_MODCTL_START_CTL) == 0) {
				/* already disabled */
				return 0;
			}
		}
		return fdt_endpoint_enable(sc->sc_out_ep, enable);
	case WSDISPLAYIO_GVIDEO:
		val = DEBE_READ(sc, SUNXI_DEBE_MODCTL_REG);
		*(int *)data = !!(val & SUNXI_DEBE_MODCTL_LAY0_EN);
		return 0;
	case WSDISPLAYIO_GCURPOS:
		cp = data;
		cp->x = sc->sc_cursor_x;
		cp->y = sc->sc_cursor_y;
		return 0;
	case WSDISPLAYIO_SCURPOS:
		cp = data;
		return sunxi_debe_set_curpos(sc, cp->x, cp->y);
	case WSDISPLAYIO_GCURMAX:
		cp = data;
		cp->x = SUNXI_DEBE_CURMAX;
		cp->y = SUNXI_DEBE_CURMAX;
		return 0;
	case WSDISPLAYIO_SCURSOR:
		return sunxi_debe_set_cursor(sc, data);
	}

	return EPASSTHROUGH;
}

/* genfb attachement */

struct sunxi_befb_softc {
	struct genfb_softc sc_gen;
	device_t sc_debedev;

	bus_dma_tag_t sc_dmat;
	bus_dma_segment_t *sc_dmasegs;
	int sc_ndmasegs;
};

static device_t	sunxi_befb_consoledev = NULL;

static int	sunxi_befb_match(device_t, cfdata_t, void *);
static void	sunxi_befb_attach(device_t, device_t, void *);

static int	sunxi_befb_ioctl(void *, void *, u_long, void *, int, lwp_t *);
static paddr_t	sunxi_befb_mmap(void *, void *, off_t, int);
static bool	sunxi_befb_shutdown(device_t, int);

CFATTACH_DECL_NEW(sunxi_befb, sizeof(struct sunxi_befb_softc),
	sunxi_befb_match, sunxi_befb_attach, NULL, NULL);

static int
sunxi_befb_match(device_t parent, cfdata_t cf, void *aux)
{
	return 1;
}

static void
sunxi_befb_attach(device_t parent, device_t self, void *aux)
{
	struct sunxi_befb_softc *sc = device_private(self);
	struct sunxifb_attach_args * const afb = aux;
	prop_dictionary_t cfg = device_properties(self);
	struct genfb_ops ops;

	if (sunxi_befb_consoledev == NULL)
		sunxi_befb_consoledev = self;

	sc->sc_gen.sc_dev = self;
	sc->sc_debedev = parent;
	sc->sc_dmat = afb->afb_dmat;
	sc->sc_dmasegs = afb->afb_dmasegs;
	sc->sc_ndmasegs = afb->afb_ndmasegs;

	prop_dictionary_set_uint32(cfg, "width", afb->afb_width);
	prop_dictionary_set_uint32(cfg, "height", afb->afb_height);
	prop_dictionary_set_uint8(cfg, "depth", 32);
	prop_dictionary_set_uint16(cfg, "linebytes", afb->afb_width * 4);
	prop_dictionary_set_uint32(cfg, "address", 0);
	prop_dictionary_set_uint32(cfg, "virtual_address",
	    (uintptr_t)afb->afb_fb);

	genfb_init(&sc->sc_gen);

	if (sc->sc_gen.sc_width == 0 || sc->sc_gen.sc_fbsize == 0) {
		aprint_normal(": disabled\n");
		return;
	}

	pmf_device_register1(self, NULL, NULL, sunxi_befb_shutdown);

	memset(&ops, 0, sizeof(ops));
	ops.genfb_ioctl = sunxi_befb_ioctl;
	ops.genfb_mmap = sunxi_befb_mmap;

	aprint_naive("\n");

	bool is_console = false;
	prop_dictionary_set_bool(cfg, "is_console", is_console);

	if (is_console)
		aprint_normal(": switching to framebuffer console\n");
	else
		aprint_normal("\n");

	genfb_attach(&sc->sc_gen, &ops);
}

static int
sunxi_befb_ioctl(void *v, void *vs, u_long cmd, void *data, int flag, lwp_t *l)
{
	struct sunxi_befb_softc *sc = v;
	struct wsdisplayio_bus_id *busid;
	struct wsdisplayio_fbinfo *fbi;
	struct rasops_info *ri;
	int error;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = WSDISPLAY_TYPE_ALLWINNER;
		return 0;
	case WSDISPLAYIO_GET_BUSID:
		busid = data;
		busid->bus_type = WSDISPLAYIO_BUS_SOC;
		return 0;
	case WSDISPLAYIO_GET_FBINFO:
		fbi = data;
		ri = &sc->sc_gen.vd.active->scr_ri;
		error = wsdisplayio_get_fbinfo(ri, fbi);
		if (error == 0) {
			fbi->fbi_flags |= WSFB_VRAM_IS_RAM;
			fbi->fbi_fbsize = sc->sc_dmasegs[0].ds_len;
		}
		return error;
	case WSDISPLAYIO_SVIDEO:
	case WSDISPLAYIO_GVIDEO:
	case WSDISPLAYIO_GCURPOS:
	case WSDISPLAYIO_SCURPOS:
	case WSDISPLAYIO_GCURMAX:
	case WSDISPLAYIO_SCURSOR:
		return sunxi_debe_ioctl(sc->sc_debedev, cmd, data);
	default:
		return EPASSTHROUGH;
	}
}

static paddr_t
sunxi_befb_mmap(void *v, void *vs, off_t off, int prot)
{
	struct sunxi_befb_softc *sc = v;

	if (off < 0 || off >= sc->sc_dmasegs[0].ds_len)
		return -1;

	return bus_dmamem_mmap(sc->sc_dmat, sc->sc_dmasegs, sc->sc_ndmasegs,
	    off, prot, BUS_DMA_PREFETCHABLE);
}

static bool
sunxi_befb_shutdown(device_t self, int flags)
{
	genfb_enable_polling(self);
	return true;
}

static void
sunxi_befb_set_videomode(device_t dev, u_int width, u_int height)
{
	struct sunxi_befb_softc *sc = device_private(dev);

	if (sc->sc_gen.sc_width != width || sc->sc_gen.sc_height != height) {
		device_printf(sc->sc_gen.sc_dev,
		    "mode switching not yet supported\n");
	}
}

int
sunxi_debe_pipeline(int phandle, bool active)
{
	device_t dev;
	struct sunxi_debe_softc *sc;
	struct fdt_endpoint *ep;
	int i, error;

	if (!active)
		return EOPNOTSUPP;

	for (i = 0;;i++) {
		dev = device_find_by_driver_unit("sunxidebe", i);
		if (dev == NULL)
			return ENODEV;
		sc = device_private(dev);
		if (sc->sc_phandle == phandle)
			break;
	}
	aprint_normal("activate %s\n", device_xname(dev));
	if (clk_enable(sc->sc_clk_ahb) != 0 ||
	    clk_enable(sc->sc_clk_mod) != 0) {
		aprint_error_dev(dev, "couldn't enable clocks\n");
		return EIO;
	}
	/* connect debd0 to tcon0, debe1 to tcon1 */
	ep = fdt_endpoint_get_from_index(&sc->sc_ports, SUNXI_PORT_OUTPUT,
	    sc->sc_unit);
	if (ep == NULL) {
		aprint_error_dev(dev, "no output endpoint for %d\n",
		    sc->sc_unit);
		return ENODEV;
	}
	error = fdt_endpoint_activate(ep, true);
	if (error == 0) {
		sc->sc_out_ep = ep;
		fdt_endpoint_enable(ep, true);
	}
	return error;
}

#if defined(SUNXI_DEBE_DEBUG)
void
sunxi_debe_dump_regs(int u)
{
	static const struct {
		const char *name;
		uint16_t reg;
	} regs[] = {
		{ "SUNXI_DEBE_MODCTL_REG", SUNXI_DEBE_MODCTL_REG},
		{ "SUNXI_DEBE_BACKCOLOR_REG", SUNXI_DEBE_BACKCOLOR_REG},
		{ "SUNXI_DEBE_DISSIZE_REG", SUNXI_DEBE_DISSIZE_REG},
		{ "SUNXI_DEBE_LAYSIZE_REG", SUNXI_DEBE_LAYSIZE_REG},
		{ "SUNXI_DEBE_LAYCOOR_REG", SUNXI_DEBE_LAYCOOR_REG},
		{ "SUNXI_DEBE_LAYLINEWIDTH_REG", SUNXI_DEBE_LAYLINEWIDTH_REG},
		{ "SUNXI_DEBE_LAYFB_L32ADD_REG", SUNXI_DEBE_LAYFB_L32ADD_REG},
		{ "SUNXI_DEBE_LAYFB_H4ADD_REG", SUNXI_DEBE_LAYFB_H4ADD_REG},
		{ "SUNXI_DEBE_REGBUFFCTL_REG", SUNXI_DEBE_REGBUFFCTL_REG},
		{ "SUNXI_DEBE_CKMAX_REG", SUNXI_DEBE_CKMAX_REG},
		{ "SUNXI_DEBE_CKMIN_REG", SUNXI_DEBE_CKMIN_REG},
		{ "SUNXI_DEBE_CKCFG_REG", SUNXI_DEBE_CKCFG_REG},
		{ "SUNXI_DEBE_ATTCTL0_REG", SUNXI_DEBE_ATTCTL0_REG},
		{ "SUNXI_DEBE_ATTCTL1_REG", SUNXI_DEBE_ATTCTL1_REG},
		{ "SUNXI_DEBE_HWCCTL_REG", SUNXI_DEBE_HWCCTL_REG},
		{ "SUNXI_DEBE_HWCFBCTL_REG", SUNXI_DEBE_HWCFBCTL_REG},
		{ "SUNXI_DEBE_WBCTL_REG", SUNXI_DEBE_WBCTL_REG},
		{ "SUNXI_DEBE_WBADD_REG", SUNXI_DEBE_WBADD_REG},
		{ "SUNXI_DEBE_WBLINEWIDTH_REG", SUNXI_DEBE_WBLINEWIDTH_REG},
		{ "SUNXI_DEBE_IYUVCTL_REG", SUNXI_DEBE_IYUVCTL_REG},
		{ "SUNXI_DEBE_IYUVADD_REG", SUNXI_DEBE_IYUVADD_REG},
		{ "SUNXI_DEBE_IYUVLINEWIDTH_REG", SUNXI_DEBE_IYUVLINEWIDTH_REG},
		{ "SUNXI_DEBE_YGCOEF_REG", SUNXI_DEBE_YGCOEF_REG},
		{ "SUNXI_DEBE_YGCONS_REG", SUNXI_DEBE_YGCONS_REG},
		{ "SUNXI_DEBE_URCOEF_REG", SUNXI_DEBE_URCOEF_REG},
		{ "SUNXI_DEBE_URCONS_REG", SUNXI_DEBE_URCONS_REG},
		{ "SUNXI_DEBE_VBCOEF_REG", SUNXI_DEBE_VBCOEF_REG},
		{ "SUNXI_DEBE_VBCONS_REG", SUNXI_DEBE_VBCONS_REG},
		{ "SUNXI_DEBE_OCCTL_REG", SUNXI_DEBE_OCCTL_REG},
		{ "SUNXI_DEBE_OCRCOEF_REG", SUNXI_DEBE_OCRCOEF_REG},
		{ "SUNXI_DEBE_OCRCONS_REG", SUNXI_DEBE_OCRCONS_REG},
		{ "SUNXI_DEBE_OCGCOEF_REG", SUNXI_DEBE_OCGCOEF_REG},
		{ "SUNXI_DEBE_OCGCONS_REG", SUNXI_DEBE_OCGCONS_REG},
		{ "SUNXI_DEBE_OCBCOEF_REG", SUNXI_DEBE_OCBCOEF_REG},
		{ "SUNXI_DEBE_OCBCONS_REG", SUNXI_DEBE_OCBCONS_REG},
	};
	struct sunxi_debe_softc *sc;
	device_t dev;

	dev = device_find_by_driver_unit("sunxidebe", u);
	if (dev == NULL)
		return;
	sc = device_private(dev);

	for (int i = 0; i < __arraycount(regs); i++) {
		printf("%s: 0x%08x\n", regs[i].name,
		    DEBE_READ(sc, regs[i].reg));
	}
}
#endif
