/* $NetBSD: wiiufb.c,v 1.5 2026/02/01 12:09:40 jmcneill Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: wiiufb.c,v 1.5 2026/02/01 12:09:40 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <machine/wii.h>
#include <machine/wiiu.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/rasops/rasops.h>
#include <dev/wsfont/wsfont.h>
#include <dev/wscons/wsdisplay_vconsvar.h>
#include <dev/wsfb/genfbvar.h>

#include "mainbus.h"

/* Set to true to output to Gamepad instead of TV */
static bool	wiiufb_drc;

#define WIIUFB_BASE		(wiiufb_drc ? WIIU_GFX_DRC_BASE : \
					      WIIU_GFX_TV_BASE)
#define WIIUFB_WIDTH		(wiiufb_drc ? 854 : 1280)
#define WIIUFB_HEIGHT		(wiiufb_drc ? 480 : 720)
#define WIIUFB_BPP		32
#define WIIUFB_STRIDE		((wiiufb_drc ? 896 : 1280) * WIIUFB_BPP / NBBY)
#define WIIUFB_SIZE		(WIIUFB_STRIDE * WIIUFB_HEIGHT)

#define WIIUFB_CURMAX		64
#define WIIUFB_CURSOR_SIZE	(WIIUFB_CURMAX * WIIUFB_CURMAX * 4)
#define WIIUFB_CURSOR_ALIGN	0x1000
#define WIIUFB_CURSOR_BITS	(WIIUFB_CURMAX * WIIUFB_CURMAX / NBBY)

#define REG_OFFSET(d, r)		((d) * 0x800 + (r))
#define DGRPH_SWAP_CNTL(d)		REG_OFFSET(d, 0x610c)
#define  DGRPH_SWAP_ENDIAN_SWAP		__BITS(1, 0)
#define  DGRPH_SWAP_ENDIAN_SWAP_8IN32	__SHIFTIN(2, DGRPH_SWAP_ENDIAN_SWAP)
#define DCRTC_BLANK_CONTROL(d)		REG_OFFSET(d, 0x6084)
#define  DCRTC_BLANK_DATA_EN		__BIT(8)
#define DCUR_CONTROL(d)			REG_OFFSET(d, 0x6400)
#define  DCUR_CONTROL_EN		__BIT(0)
#define  DCUR_CONTROL_MODE		__BITS(9, 8)
#define  DCUR_CONTROL_MODE_ARGB_PM	__SHIFTIN(2, DCUR_CONTROL_MODE)
#define DCUR_SURFACE_ADDRESS(d)		REG_OFFSET(d, 0x6408)
#define DCUR_SIZE(d)			REG_OFFSET(d, 0x6410)
#define  DCUR_SIZE_HEIGHT(h)		((h) - 1)
#define  DCUR_SIZE_WIDTH(w)		(((w) - 1) << 16)
#define DCUR_POSITION(d)		REG_OFFSET(d, 0x6414)
#define  DCUR_POSITION_Y(y)		(y)
#define  DCUR_POSITION_X(x)		((x) << 16)
#define DCUR_HOT_SPOT(d)		REG_OFFSET(d, 0x6418)
#define  DCUR_HOT_SPOT_Y(y)		(y)
#define  DCUR_HOT_SPOT_X(x)		((x) << 16)
#define DCUR_UPDATE(d)			REG_OFFSET(d, 0x6424)
#define  DCUR_UPDATE_LOCK		__BIT(16)

struct wiiufb_dma {
	bus_dmamap_t		dma_map;
	bus_dma_tag_t		dma_tag;
	bus_size_t		dma_size;
	bus_dma_segment_t	dma_segs[1];
	int			dma_nsegs;
	void			*dma_addr;
};

struct wiiufb_cursor {
	bool			c_enable;
	struct wsdisplay_curpos	c_pos;
	struct wsdisplay_curpos	c_hot;
	struct wsdisplay_curpos	c_size;
	uint32_t		c_cmap[2];
	uint8_t			c_image[WIIUFB_CURSOR_BITS];
	uint8_t			c_mask[WIIUFB_CURSOR_BITS];
};

struct wiiufb_softc {
	struct genfb_softc	sc_gen;

	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	bus_dma_tag_t		sc_dmat;

	uint32_t		sc_disp;

	struct wiiufb_cursor	sc_cursor;
	struct wiiufb_cursor	sc_tempcursor;
	struct wiiufb_dma	sc_curdma;
};

static struct wiiufb_softc *wiiufb_sc;

static int	wiiufb_match(device_t, cfdata_t, void *);
static void	wiiufb_attach(device_t, device_t, void *);

static bool	wiiufb_shutdown(device_t, int);
static int	wiiufb_ioctl(void *, void *, u_long, void *, int, lwp_t *);
static paddr_t	wiiufb_mmap(void *, void *, off_t, int);

static int	wiiufb_dma_alloc(struct wiiufb_softc *, bus_size_t, bus_size_t,
				 int, struct wiiufb_dma *);
static void	wiiufb_gpu_write(uint16_t, uint32_t);
static uint32_t	wiiufb_gpu_read(uint16_t);
static void	wiiufb_gpu_set(uint16_t, uint32_t);
static void	wiiufb_gpu_clear(uint16_t, uint32_t);

void		wiiufb_consinit(void);

static struct genfb_ops wiiufb_ops = {
	.genfb_ioctl = wiiufb_ioctl,
	.genfb_mmap = wiiufb_mmap,
};

struct vcons_screen wiiufb_console_screen;
static struct wsscreen_descr wiiufb_stdscreen = {
	"std",
	0, 0,
	0,
	0, 0,
	0,
	NULL
};

CFATTACH_DECL_NEW(wiiufb, sizeof(struct wiiufb_softc),
	wiiufb_match, wiiufb_attach, NULL, NULL);

static int
wiiufb_match(device_t parent, cfdata_t cf, void *aux)
{
	struct mainbus_attach_args *maa = aux;

	return wiiu_native && strcmp(maa->maa_name, "genfb") == 0;
}

static void
wiiufb_attach(device_t parent, device_t self, void *aux)
{
	struct wiiufb_softc *sc = device_private(self);
	prop_dictionary_t dict = device_properties(self);
	struct mainbus_attach_args *maa = aux;
	void *bits;
	int error;

	sc->sc_gen.sc_dev = self;
	sc->sc_bst = maa->maa_bst;
	error = bus_space_map(sc->sc_bst, maa->maa_addr, WIIU_GX2_SIZE, 0,
	    &sc->sc_bsh);
	if (error != 0) {
		panic("couldn't map registers");
	}
	sc->sc_dmat = maa->maa_dmat;
	sc->sc_disp = wiiufb_drc ? 1 : 0;

	wiiufb_sc = sc;

	/*
	 * powerpc bus_space_map will use the BAT mapping if present,
	 * ignoring the map flags passed in. Just use mapiodev directly
	 * to ensure that the FB is mapped by the kernel pmap.
	 */
	bits = mapiodev(WIIUFB_BASE, WIIUFB_SIZE, true);

	prop_dictionary_set_uint32(dict, "width", WIIUFB_WIDTH);
	prop_dictionary_set_uint32(dict, "height", WIIUFB_HEIGHT);
	prop_dictionary_set_uint8(dict, "depth", WIIUFB_BPP);
	prop_dictionary_set_uint16(dict, "linebytes", WIIUFB_STRIDE);
	prop_dictionary_set_uint32(dict, "address", WIIUFB_BASE);
	prop_dictionary_set_uint32(dict, "virtual_address", (uintptr_t)bits);

	genfb_init(&sc->sc_gen);

	aprint_naive("\n");
	aprint_normal(": Wii U %s framebuffer (%ux%u %u-bpp @ 0x%08x)\n",
	    sc->sc_disp ? "DRC" : "TV", WIIUFB_WIDTH, WIIUFB_HEIGHT,
	    WIIUFB_BPP, WIIUFB_BASE);

	pmf_device_register1(self, NULL, NULL, wiiufb_shutdown);

	error = wiiufb_dma_alloc(sc, WIIUFB_CURSOR_SIZE, WIIUFB_CURSOR_ALIGN,
	    BUS_DMA_NOCACHE, &sc->sc_curdma);
	if (error != 0) {
		panic("couldn't alloc hardware cursor");
	}
	wiiufb_gpu_set(DCUR_UPDATE(sc->sc_disp), DCUR_UPDATE_LOCK);
	wiiufb_gpu_set(DCUR_CONTROL(sc->sc_disp), 0);
	wiiufb_gpu_clear(DCUR_UPDATE(sc->sc_disp), DCUR_UPDATE_LOCK);

	genfb_cnattach();
	prop_dictionary_set_bool(dict, "is_console", true);
	genfb_attach(&sc->sc_gen, &wiiufb_ops);
}

static bool
wiiufb_shutdown(device_t self, int flags)
{
	genfb_enable_polling(self);
	return true;
}

static int
wiiufb_dma_alloc(struct wiiufb_softc *sc, bus_size_t size, bus_size_t align,
    int flags, struct wiiufb_dma *dma)
{
	bus_dma_tag_t dmat = sc->sc_dmat;
	int error;

	error = bus_dmamem_alloc(dmat, size, align, 0,
	    dma->dma_segs, 1, &dma->dma_nsegs, BUS_DMA_WAITOK);
	if (error)
		return error;

	error = bus_dmamem_map(dmat, dma->dma_segs, dma->dma_nsegs,
	    size, &dma->dma_addr, BUS_DMA_WAITOK | flags);
	if (error)
		goto free;

	error = bus_dmamap_create(dmat, size, dma->dma_nsegs,
	    size, 0, BUS_DMA_WAITOK, &dma->dma_map);
	if (error)
		goto unmap;

	error = bus_dmamap_load(dmat, dma->dma_map, dma->dma_addr,
	    size, NULL, BUS_DMA_WAITOK);
	if (error)
	    goto destroy;

	dma->dma_size = size;
	dma->dma_tag = dmat;

	memset(dma->dma_addr, 0, dma->dma_size);

	return 0;

destroy:
	bus_dmamap_destroy(dmat, dma->dma_map);
unmap:
	bus_dmamem_unmap(dmat, dma->dma_addr, dma->dma_size);
free:
	bus_dmamem_free(dmat, dma->dma_segs, dma->dma_nsegs);

	return error;
}

static void
wiiufb_cursor_shape(struct wiiufb_softc *sc)
{
	const uint8_t *msk = sc->sc_cursor.c_mask;
	const uint8_t *img = sc->sc_cursor.c_image;
	uint32_t *out = sc->sc_curdma.dma_addr;
	uint8_t bit;
	int i, j, px;

	for (i = 0; i < WIIUFB_CURMAX * NBBY; i++) {
		bit = 1;
		for (j = 0; j < 8; j++) {
			px = ((*msk & bit) ? 2 : 0) | ((*img & bit) ? 1 : 0);
			switch (px) {
			case 0:
			case 1:
				*out = 0;
				break;
			case 2:
			case 3:
				*out = htole32(
				    0xff000000 | sc->sc_cursor.c_cmap[px - 2]);
				break;
			}
			out++;
			bit <<= 1;
		}
		msk++;
		img++;
	}
}

static void
wiiufb_cursor_visible(struct wiiufb_softc *sc)
{
	uint32_t control;

	wiiufb_gpu_set(DCUR_UPDATE(sc->sc_disp), DCUR_UPDATE_LOCK);

	if (sc->sc_cursor.c_enable) {
		control = DCUR_CONTROL_EN | DCUR_CONTROL_MODE_ARGB_PM;
	} else {
		control = 0;
	}
	wiiufb_gpu_write(DCUR_CONTROL(sc->sc_disp), control);

	wiiufb_gpu_clear(DCUR_UPDATE(sc->sc_disp), DCUR_UPDATE_LOCK);
}

static void
wiiufb_cursor_position(struct wiiufb_softc *sc)
{
	int x, y;

	wiiufb_gpu_set(DCUR_UPDATE(sc->sc_disp), DCUR_UPDATE_LOCK);

	x = uimin(sc->sc_cursor.c_pos.x, WIIUFB_WIDTH - 1);
	y = uimin(sc->sc_cursor.c_pos.y, WIIUFB_HEIGHT - 1);

	wiiufb_gpu_write(DCUR_SIZE(sc->sc_disp),
	    DCUR_SIZE_WIDTH(WIIUFB_CURMAX) |
	    DCUR_SIZE_HEIGHT(WIIUFB_CURMAX));
	wiiufb_gpu_write(DCUR_SURFACE_ADDRESS(sc->sc_disp),
	    sc->sc_curdma.dma_segs[0].ds_addr);
	wiiufb_gpu_write(DCUR_POSITION(sc->sc_disp),
	    DCUR_POSITION_X(x) |
	    DCUR_POSITION_Y(y));
	wiiufb_gpu_write(DCUR_HOT_SPOT(sc->sc_disp),
	    DCUR_HOT_SPOT_X(sc->sc_cursor.c_hot.x) |
	    DCUR_HOT_SPOT_Y(sc->sc_cursor.c_hot.y));

	wiiufb_gpu_clear(DCUR_UPDATE(sc->sc_disp), DCUR_UPDATE_LOCK);
}

static void
wiiufb_cursor_update(struct wiiufb_softc *sc, unsigned which)
{
	if ((which & (WSDISPLAY_CURSOR_DOCMAP|WSDISPLAY_CURSOR_DOSHAPE)) != 0) {
		wiiufb_cursor_shape(sc);
	}

	if ((which & WSDISPLAY_CURSOR_DOCUR) != 0) {
		wiiufb_cursor_visible(sc);
	}

	wiiufb_cursor_position(sc);
}

static int
wiiufb_set_cursor(struct wiiufb_softc *sc, struct wsdisplay_cursor *wc)
{
	uint8_t r[2], g[2], b[2];
	unsigned which, index, count;
	int i, err, pitch, size;
	struct wiiufb_cursor *nc = &sc->sc_tempcursor;

	which = wc->which;
	*nc = sc->sc_cursor;

	if ((which & WSDISPLAY_CURSOR_DOCMAP) != 0) {
		index = wc->cmap.index;
		count = wc->cmap.count;

		if (index >= 2 || count > 2 - index) {
			return EINVAL;
		}

		err = copyin(wc->cmap.red, &r[index], count);
		if (err != 0) {
			return err;
		}
		err = copyin(wc->cmap.green, &g[index], count);
		if (err != 0) {
			return err;
		}
		err = copyin(wc->cmap.blue, &b[index], count);
		if (err != 0) {
			return err;
		}

		for (i = index; i < index + count; i++) {
			nc->c_cmap[i] =
			    (r[i] << 16) + (g[i] << 8) + (b[i] << 0);
		}
	}

	if ((which & WSDISPLAY_CURSOR_DOSHAPE) != 0) {
		if (wc->size.x > WIIUFB_CURMAX || wc->size.y > WIIUFB_CURMAX) {
			return EINVAL;
		}

		pitch = (wc->size.x + 7) / 8;
		size = pitch * wc->size.y;

		memset(nc->c_image, 0, sizeof(nc->c_image));
		memset(nc->c_mask, 0, sizeof(nc->c_mask));
		nc->c_size = wc->size;

		if ((err = copyin(wc->image, nc->c_image, size)) != 0) {
			return err;
		}
		if ((err = copyin(wc->mask, nc->c_mask, size)) != 0) {
			return err;
		}
	}

	if ((which & WSDISPLAY_CURSOR_DOHOT) != 0) {
		nc->c_hot = wc->hot;
		if (nc->c_hot.x >= nc->c_size.x) {
			nc->c_hot.x = nc->c_size.x - 1;
		}
		if (nc->c_hot.y >= nc->c_size.y) {
			nc->c_hot.y = nc->c_size.y - 1;
		}
	}

	if ((which & WSDISPLAY_CURSOR_DOPOS) != 0) {
		nc->c_pos = wc->pos;
		if (nc->c_pos.x >= WIIUFB_WIDTH) {
			nc->c_pos.x = WIIUFB_WIDTH - 1;
		}
		if (nc->c_pos.y >= WIIUFB_HEIGHT) {
			nc->c_pos.y = WIIUFB_HEIGHT - 1;
		}
	}

	if ((which & WSDISPLAY_CURSOR_DOCUR) != 0) {
		nc->c_enable = wc->enable;
	}

	sc->sc_cursor = *nc;
	wiiufb_cursor_update(sc, wc->which);

	return 0;
}

static int
wiiufb_set_curpos(struct wiiufb_softc *sc, struct wsdisplay_curpos *pos)
{
	sc->sc_cursor.c_pos.x = uimin(uimax(pos->x, 0), WIIUFB_WIDTH - 1);
	sc->sc_cursor.c_pos.y = uimin(uimax(pos->y, 0), WIIUFB_HEIGHT - 1);

	wiiufb_cursor_position(sc);

	return 0;
}

static int
wiiufb_ioctl(void *v, void *vs, u_long cmd, void *data, int flag, lwp_t *l)
{
	struct wiiufb_softc *sc = v;
	struct wsdisplayio_bus_id *busid;
	struct wsdisplayio_fbinfo *fbi;
	struct wsdisplay_curpos *cp;
	struct rasops_info *ri;
	u_int video;
	int error;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = WSDISPLAY_TYPE_GENFB;
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
		}
		return error;
	case WSDISPLAYIO_SVIDEO:
		video = *(u_int *)data;
		switch (video) {
		case WSDISPLAYIO_VIDEO_OFF:
			wiiufb_gpu_write(DCRTC_BLANK_CONTROL(sc->sc_disp),
			    DCRTC_BLANK_DATA_EN);
			break;
		case WSDISPLAYIO_VIDEO_ON:
			wiiufb_gpu_write(DCRTC_BLANK_CONTROL(sc->sc_disp), 0);
			break;
		default:
			return EINVAL;
		}
		return 0;
	case WSDISPLAYIO_GCURMAX:
		cp = data;
		cp->x = WIIUFB_CURMAX;
		cp->y = WIIUFB_CURMAX;
		return 0;
	case WSDISPLAYIO_GCURPOS:
		cp = data;
		cp->x = sc->sc_cursor.c_pos.x;
		cp->y = sc->sc_cursor.c_pos.y;
		return 0;
	case WSDISPLAYIO_SCURPOS:
		return wiiufb_set_curpos(sc, data);
	case WSDISPLAYIO_SCURSOR:
		return wiiufb_set_cursor(sc, data);
	}

	return EPASSTHROUGH;
}

static paddr_t
wiiufb_mmap(void *v, void *vs, off_t off, int prot)
{
	struct wiiufb_softc *sc = v;

	if (off < 0 || off >= WIIUFB_SIZE) {
		return -1;
	}

	return bus_space_mmap(sc->sc_bst, WIIUFB_BASE, off, prot,
	    BUS_SPACE_MAP_LINEAR | BUS_DMA_PREFETCHABLE);
}

static void
wiiufb_gpu_write(uint16_t reg, uint32_t data)
{
	if (wiiufb_sc != NULL) {
		bus_space_write_4(wiiufb_sc->sc_bst, wiiufb_sc->sc_bsh,
		    reg, data);
	} else {
		out32(LT_GPUINDADDR, LT_GPUINDADDR_REGSPACE_GPU | reg);
		out32(LT_GPUINDDATA, data);
		in32(LT_GPUINDDATA);
	}
}

static uint32_t
wiiufb_gpu_read(uint16_t reg)
{
	if (wiiufb_sc != NULL) {
		return bus_space_read_4(wiiufb_sc->sc_bst, wiiufb_sc->sc_bsh,
		    reg);
	} else {
		out32(LT_GPUINDADDR, LT_GPUINDADDR_REGSPACE_GPU | reg);
		return in32(LT_GPUINDDATA);
	}
}

static void
wiiufb_gpu_set(uint16_t reg, uint32_t mask)
{
	wiiufb_gpu_write(reg, wiiufb_gpu_read(reg) | mask);
}

static void
wiiufb_gpu_clear(uint16_t reg, uint32_t mask)
{
	wiiufb_gpu_write(reg, wiiufb_gpu_read(reg) & ~mask);
}

void
wiiufb_consinit(void)
{
	extern char wii_cmdline[];
	struct rasops_info *ri = &wiiufb_console_screen.scr_ri;
	long defattr;
	void *bits;
	const char *cmdline = wii_cmdline;

	memset(&wiiufb_console_screen, 0, sizeof(wiiufb_console_screen));

	while (*cmdline != '\0') {
		if (strcmp(cmdline, "video=drc") == 0) {
			/* Output to the gamepad instead of TV. */
			wiiufb_drc = true;
			break;
		}
		cmdline += strlen(cmdline) + 1;
	}

	/* Blank the CRTC we are not using. */
	wiiufb_gpu_write(DCRTC_BLANK_CONTROL(wiiufb_drc ? 0 : 1),
	    DCRTC_BLANK_DATA_EN);

	/* Ensure that the ARGB8888 framebuffer is in a sane state. */
	wiiufb_gpu_write(DGRPH_SWAP_CNTL(0), DGRPH_SWAP_ENDIAN_SWAP_8IN32);
	wiiufb_gpu_write(DGRPH_SWAP_CNTL(1), DGRPH_SWAP_ENDIAN_SWAP_8IN32);

	/*
	 * Need to use the BAT mapping here as pmap isn't initialized yet.
	 *
	 * Unfortunately, we have a single large (256MB) BAT mapping to cover
	 * both conventional memory and the framebuffer in MEM1, which means
	 * the early FB is mapped cacheable. Better than nothing (it's
	 * useful for debugging) and it's only like this until wiiufb is
	 * attached later on.
	 *
	 * This could be enhanced in the future to hook in to rasops and
	 * insert proper cache maintenance operations. Just don't flush the
	 * whole framebuffer every time something changes, it will be very
	 * slow.
	 */
	bits = (void *)WIIUFB_BASE;

	wsfont_init();

	ri->ri_width = WIIUFB_WIDTH;
	ri->ri_height = WIIUFB_HEIGHT;
	ri->ri_depth = WIIUFB_BPP;
	ri->ri_stride = WIIUFB_STRIDE;
	ri->ri_bits = bits;
	ri->ri_flg = RI_NO_AUTO | RI_CLEAR | RI_FULLCLEAR | RI_CENTER;
	rasops_init(ri, ri->ri_height / 8, ri->ri_width / 8);

	ri->ri_caps = WSSCREEN_WSCOLORS;
	rasops_reconfig(ri, ri->ri_height / ri->ri_font->fontheight,
	    ri->ri_width / ri->ri_font->fontwidth);

	wiiufb_stdscreen.nrows = ri->ri_rows;
	wiiufb_stdscreen.ncols = ri->ri_cols;
	wiiufb_stdscreen.textops = &ri->ri_ops;
	wiiufb_stdscreen.capabilities = ri->ri_caps;

	ri->ri_ops.allocattr(ri, 0, 0, 0, &defattr);

	wsdisplay_preattach(&wiiufb_stdscreen, ri, 0, 0, defattr);
}
