/* $NetBSD: crmfb.c,v 1.44.2.1 2018/03/18 11:08:04 martin Exp $ */

/*-
 * Copyright (c) 2007 Jared D. McNeill <jmcneill@invisible.ca>
 *               2008 Michael Lorenz <macallan@netbsd.org>
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
 * SGI-CRM (O2) Framebuffer driver
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: crmfb.c,v 1.44.2.1 2018/03/18 11:08:04 martin Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/autoconf.h>
#include <sys/bus.h>
#include <machine/machtype.h>
#include <machine/vmparam.h>

#include <dev/arcbios/arcbios.h>
#include <dev/arcbios/arcbiosvar.h>

#include <dev/wscons/wsdisplayvar.h>
#include <dev/wscons/wsconsio.h>
#include <dev/wsfont/wsfont.h>
#include <dev/rasops/rasops.h>
#include <dev/wscons/wsdisplay_vconsvar.h>

#include <dev/i2c/i2cvar.h>
#include <dev/i2c/i2c_bitbang.h>
#include <dev/i2c/ddcvar.h>
#include <dev/videomode/videomode.h>
#include <dev/videomode/vesagtf.h>
#include <dev/videomode/edidvar.h>

#include <arch/sgimips/dev/crmfbreg.h>

#include "opt_crmfb.h"

#ifdef CRMFB_DEBUG
#define DPRINTF printf
#else
#define DPRINTF while (0) printf
#endif

struct wsscreen_descr crmfb_defaultscreen = {
	"default",
	0, 0,
	NULL,
	8, 16,
	WSSCREEN_WSCOLORS | WSSCREEN_RESIZE,
	NULL,
};

const struct wsscreen_descr *_crmfb_scrlist[] = {
	&crmfb_defaultscreen,
};

struct wsscreen_list crmfb_screenlist = {
	sizeof(_crmfb_scrlist) / sizeof(struct wsscreen_descr *),
	_crmfb_scrlist
};

static struct vcons_screen crmfb_console_screen;

static int	crmfb_ioctl(void *, void *, u_long, void *, int, struct lwp *);
static paddr_t	crmfb_mmap(void *, void *, off_t, int);
static void	crmfb_init_screen(void *, struct vcons_screen *, int, long *);

struct wsdisplay_accessops crmfb_accessops = {
	crmfb_ioctl,
	crmfb_mmap,
	NULL,	/* alloc_screen */
	NULL,	/* free_screen */
	NULL,	/* show_screen */
	NULL,	/* load_font */
	NULL,	/* pollc */
	NULL,	/* scroll */
};

/* Memory to allocate to SGI-CRM -- remember, this is stolen from
 * host memory!
 */
#define CRMFB_TILESIZE	(512*128)

static int	crmfb_match(device_t, struct cfdata *, void *);
static void	crmfb_attach(device_t, device_t, void *);
int		crmfb_probe(void);

#define KERNADDR(p)	((void *)((p).addr))
#define DMAADDR(p)	((p).map->dm_segs[0].ds_addr)

#define CRMFB_REG_MASK(msb, lsb) \
	( (((uint32_t) 1 << ((msb)-(lsb)+1)) - 1) << (lsb) )


struct crmfb_dma {
	bus_dmamap_t		map;
	void			*addr;
	bus_dma_segment_t	segs[1];
	int			nsegs;
	size_t			size;
};

struct crmfb_softc {
	device_t		sc_dev;
	struct vcons_data	sc_vd;
	struct i2c_controller	sc_i2c;
	int sc_dir;

	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	bus_space_handle_t	sc_reh;

	bus_dma_tag_t		sc_dmat;

	struct crmfb_dma	sc_dma;
	struct crmfb_dma	sc_dmai;

	int			sc_width;
	int			sc_height;
	int			sc_depth;
	int			sc_console_depth;
	int			sc_tiles_x, sc_tiles_y;
	uint32_t		sc_fbsize;
	int			sc_mte_direction;
	int			sc_mte_x_shift;
	uint32_t		sc_mte_mode;
	uint32_t		sc_de_mode;
	uint32_t		sc_src_mode;
	uint32_t		sc_dst_mode;
	int			sc_needs_sync;
	uint8_t			*sc_lptr;
	paddr_t			sc_linear;
	uint32_t		sc_vtflags;
	int			sc_wsmode, sc_video_on;
	uint8_t			sc_edid_data[128];
	struct edid_info 	sc_edid_info;

	/* cursor stuff */
	int			sc_cur_x;
	int			sc_cur_y;
	int			sc_hot_x;
	int			sc_hot_y;

	u_char			sc_cmap_red[256];
	u_char			sc_cmap_green[256];
	u_char			sc_cmap_blue[256];
};

static int	crmfb_putcmap(struct crmfb_softc *, struct wsdisplay_cmap *);
static int	crmfb_getcmap(struct crmfb_softc *, struct wsdisplay_cmap *);
static void	crmfb_set_palette(struct crmfb_softc *,
				  int, uint8_t, uint8_t, uint8_t);
static int	crmfb_set_curpos(struct crmfb_softc *, int, int);
static int	crmfb_gcursor(struct crmfb_softc *, struct wsdisplay_cursor *);
static int	crmfb_scursor(struct crmfb_softc *, struct wsdisplay_cursor *);
static inline void	crmfb_write_reg(struct crmfb_softc *, int, uint32_t);
static inline uint32_t	crmfb_read_reg(struct crmfb_softc *, int);
static int	crmfb_wait_dma_idle(struct crmfb_softc *);

/* setup video hw in given colour depth */
static int	crmfb_setup_video(struct crmfb_softc *, int);
static void	crmfb_setup_palette(struct crmfb_softc *);

static void crmfb_fill_rect(struct crmfb_softc *, int, int, int, int, uint32_t);
static void crmfb_bitblt(struct crmfb_softc *, int, int, int, int, int, int,
			 uint32_t);
static void crmfb_scroll(struct crmfb_softc *, int, int, int, int, int, int);

static void	crmfb_copycols(void *, int, int, int, int);
static void	crmfb_erasecols(void *, int, int, int, long);
static void	crmfb_copyrows(void *, int, int, int);
static void	crmfb_eraserows(void *, int, int, long);
static void	crmfb_cursor(void *, int, int, int);
static void	crmfb_putchar(void *, int, int, u_int, long);
static void	crmfb_putchar_aa(void *, int, int, u_int, long);

/* I2C glue */
static int crmfb_i2c_acquire_bus(void *, int);
static void crmfb_i2c_release_bus(void *, int);
static int crmfb_i2c_send_start(void *, int);
static int crmfb_i2c_send_stop(void *, int);
static int crmfb_i2c_initiate_xfer(void *, i2c_addr_t, int);
static int crmfb_i2c_read_byte(void *, uint8_t *, int);
static int crmfb_i2c_write_byte(void *, uint8_t, int);

/* I2C bitbang glue */
static void crmfb_i2cbb_set_bits(void *, uint32_t);
static void crmfb_i2cbb_set_dir(void *, uint32_t);
static uint32_t crmfb_i2cbb_read(void *);

static const struct i2c_bitbang_ops crmfb_i2cbb_ops = {
	crmfb_i2cbb_set_bits,
	crmfb_i2cbb_set_dir,
	crmfb_i2cbb_read,
	{
		CRMFB_I2C_SDA,
		CRMFB_I2C_SCL,
		0,
		1
	}
};
static void crmfb_setup_ddc(struct crmfb_softc *);

/* mode setting stuff */
static uint32_t calc_pll(int);	/* frequency in kHz */
static int crmfb_set_mode(struct crmfb_softc *, const struct videomode *);
static int crmfb_parse_mode(const char *, struct videomode *);

CFATTACH_DECL_NEW(crmfb, sizeof(struct crmfb_softc),
    crmfb_match, crmfb_attach, NULL, NULL);

static int
crmfb_match(device_t parent, struct cfdata *cf, void *opaque)
{
	return crmfb_probe();
}

static void
crmfb_attach(device_t parent, device_t self, void *opaque)
{
	struct mainbus_attach_args *ma;
	struct crmfb_softc *sc;
	struct rasops_info *ri;
	struct wsemuldisplaydev_attach_args aa;
	uint32_t d, h;
	uint16_t *p;
	unsigned long v;
	long defattr;
	const char *consdev;
	const char *modestr;
	struct videomode mode, *pmode;
	int rv, i;

	sc = device_private(self);
	sc->sc_dev = self;

	ma = (struct mainbus_attach_args *)opaque;

	sc->sc_iot = normal_memt;
	sc->sc_dmat = &sgimips_default_bus_dma_tag;
	sc->sc_wsmode = WSDISPLAYIO_MODE_EMUL;

	aprint_normal(": SGI CRIME Graphics Display Engine\n");
	rv = bus_space_map(sc->sc_iot, ma->ma_addr, 0 /* XXX */,
	    BUS_SPACE_MAP_LINEAR, &sc->sc_ioh);
	if (rv)
		panic("crmfb_attach: can't map I/O space");
	rv = bus_space_map(sc->sc_iot, 0x15000000, 0x6000, 0, &sc->sc_reh);
	if (rv)
		panic("crmfb_attach: can't map rendering engine");

	/* determine mode configured by firmware */
	d = bus_space_read_4(sc->sc_iot, sc->sc_ioh, CRMFB_VT_HCMAP);
	sc->sc_width = (d >> CRMFB_VT_HCMAP_ON_SHIFT) & 0xfff;
	d = bus_space_read_4(sc->sc_iot, sc->sc_ioh, CRMFB_VT_VCMAP);
	sc->sc_height = (d >> CRMFB_VT_VCMAP_ON_SHIFT) & 0xfff;
	d = bus_space_read_4(sc->sc_iot, sc->sc_ioh, CRMFB_FRM_TILESIZE);
	h = (d >> CRMFB_FRM_TILESIZE_DEPTH_SHIFT) & 0x3;
	if (h == 0)
		sc->sc_depth = 8;
	else if (h == 1)
		sc->sc_depth = 16;
	else
		sc->sc_depth = 32;

	if (sc->sc_width == 0 || sc->sc_height == 0) {
		/*
		 * XXX
		 * actually, these days we probably could
		 */
		aprint_error_dev(sc->sc_dev,
		    "device unusable if not setup by firmware\n");
		bus_space_unmap(sc->sc_iot, sc->sc_ioh, 0 /* XXX */);
		return;
	}

	aprint_normal_dev(sc->sc_dev, "initial resolution %dx%d\n",
	    sc->sc_width, sc->sc_height);

	sc->sc_console_depth = 8;

	crmfb_setup_ddc(sc);
	pmode = sc->sc_edid_info.edid_preferred_mode;

	modestr = arcbios_GetEnvironmentVariable("crmfb_mode");
	if (crmfb_parse_mode(modestr, &mode) == 0)
		pmode = &mode;

	if (pmode != NULL && crmfb_set_mode(sc, pmode))
		aprint_normal_dev(sc->sc_dev, "using %dx%d\n",
		    sc->sc_width, sc->sc_height);

	/*
	 * first determine how many tiles we need
	 * in 32bit each tile is 128x128 pixels
	 */
	sc->sc_tiles_x = (sc->sc_width + 127) >> 7;
	sc->sc_tiles_y = (sc->sc_height + 127) >> 7;
	sc->sc_fbsize = 0x10000 * sc->sc_tiles_x * sc->sc_tiles_y;

	sc->sc_dmai.size = 256 * sizeof(uint16_t);
	rv = bus_dmamem_alloc(sc->sc_dmat, sc->sc_dmai.size, 65536, 0,
	    sc->sc_dmai.segs,
	    sizeof(sc->sc_dmai.segs) / sizeof(sc->sc_dmai.segs[0]),
	    &sc->sc_dmai.nsegs, BUS_DMA_NOWAIT);
	if (rv)
		panic("crmfb_attach: can't allocate DMA memory");
	rv = bus_dmamem_map(sc->sc_dmat, sc->sc_dmai.segs, sc->sc_dmai.nsegs,
	    sc->sc_dmai.size, &sc->sc_dmai.addr,
	    BUS_DMA_NOWAIT);
	if (rv)
		panic("crmfb_attach: can't map DMA memory");
	rv = bus_dmamap_create(sc->sc_dmat, sc->sc_dmai.size, 1,
	    sc->sc_dmai.size, 0, BUS_DMA_NOWAIT, &sc->sc_dmai.map);
	if (rv)
		panic("crmfb_attach: can't create DMA map");
	rv = bus_dmamap_load(sc->sc_dmat, sc->sc_dmai.map, sc->sc_dmai.addr,
	    sc->sc_dmai.size, NULL, BUS_DMA_NOWAIT);
	if (rv)
		panic("crmfb_attach: can't load DMA map");

	/* allocate an extra 128Kb for a linear buffer */
	sc->sc_dma.size = 0x10000 * (16 * sc->sc_tiles_x + 2);
	rv = bus_dmamem_alloc(sc->sc_dmat, sc->sc_dma.size, 65536, 0,
	    sc->sc_dma.segs,
	    sizeof(sc->sc_dma.segs) / sizeof(sc->sc_dma.segs[0]),
	    &sc->sc_dma.nsegs, BUS_DMA_NOWAIT);
	if (rv)
		panic("crmfb_attach: can't allocate DMA memory");
	rv = bus_dmamem_map(sc->sc_dmat, sc->sc_dma.segs, sc->sc_dma.nsegs,
	    sc->sc_dma.size, &sc->sc_dma.addr,
	    BUS_DMA_NOWAIT | BUS_DMA_COHERENT);
	if (rv)
		panic("crmfb_attach: can't map DMA memory");
	rv = bus_dmamap_create(sc->sc_dmat, sc->sc_dma.size, 1,
	    sc->sc_dma.size, 0, BUS_DMA_NOWAIT, &sc->sc_dma.map);
	if (rv)
		panic("crmfb_attach: can't create DMA map");

	rv = bus_dmamap_load(sc->sc_dmat, sc->sc_dma.map, sc->sc_dma.addr,
	    sc->sc_dma.size, NULL, BUS_DMA_NOWAIT);
	if (rv)
		panic("crmfb_attach: can't load DMA map");

	p = KERNADDR(sc->sc_dmai);
	v = (unsigned long)DMAADDR(sc->sc_dma);
	for (i = 0; i < (sc->sc_tiles_x * sc->sc_tiles_y); i++) {
		p[i] = ((uint32_t)v >> 16) + i;
	}

	bus_dmamap_sync(sc->sc_dmat, sc->sc_dmai.map, 0, sc->sc_dmai.size,
	    BUS_DMASYNC_PREWRITE);

	sc->sc_linear = (paddr_t)DMAADDR(sc->sc_dma) + 0x100000 * sc->sc_tiles_x;
	sc->sc_lptr =  (char *)KERNADDR(sc->sc_dma) + (0x100000 * sc->sc_tiles_x);

	aprint_normal_dev(sc->sc_dev, "allocated %d byte fb @ %p (%p)\n", 
	    sc->sc_fbsize, KERNADDR(sc->sc_dmai), KERNADDR(sc->sc_dma));

	crmfb_setup_video(sc, sc->sc_console_depth);
	ri = &crmfb_console_screen.scr_ri;
	memset(ri, 0, sizeof(struct rasops_info));
	sc->sc_video_on = 1;

	vcons_init(&sc->sc_vd, sc, &crmfb_defaultscreen, &crmfb_accessops);
	sc->sc_vd.init_screen = crmfb_init_screen;
	crmfb_console_screen.scr_flags |= VCONS_SCREEN_IS_STATIC;
	vcons_init_screen(&sc->sc_vd, &crmfb_console_screen, 1, &defattr);

	crmfb_defaultscreen.ncols = ri->ri_cols;
	crmfb_defaultscreen.nrows = ri->ri_rows;
	crmfb_defaultscreen.textops = &ri->ri_ops;
	crmfb_defaultscreen.capabilities = ri->ri_caps;
	crmfb_defaultscreen.modecookie = NULL;

	crmfb_setup_palette(sc);
	crmfb_fill_rect(sc, 0, 0, sc->sc_width, sc->sc_height,
	    ri->ri_devcmap[(defattr >> 16) & 0xff]);

	consdev = arcbios_GetEnvironmentVariable("ConsoleOut");
	if (consdev != NULL && strcmp(consdev, "video()") == 0) {
		wsdisplay_cnattach(&crmfb_defaultscreen, ri, 0, 0, defattr);
		vcons_replay_msgbuf(&crmfb_console_screen);
		aa.console = 1;
	} else
		aa.console = 0;
	aa.scrdata = &crmfb_screenlist;
	aa.accessops = &crmfb_accessops;
	aa.accesscookie = &sc->sc_vd;

	config_found(self, &aa, wsemuldisplaydevprint);

	sc->sc_cur_x = 0;
	sc->sc_cur_y = 0;
	sc->sc_hot_x = 0;
	sc->sc_hot_y = 0;

	return;
}

int
crmfb_probe(void)
{

        if (mach_type != MACH_SGI_IP32)
                return 0;

	return 1;
}

static int
crmfb_ioctl(void *v, void *vs, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct vcons_data *vd;
	struct crmfb_softc *sc;
	struct wsdisplay_fbinfo *wdf;
	int nmode;

	vd = (struct vcons_data *)v;
	sc = (struct crmfb_softc *)vd->cookie;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		/* not really, but who cares? */
		/* xf86-video-crime does */
		*(u_int *)data = WSDISPLAY_TYPE_CRIME;
		return 0;
	case WSDISPLAYIO_GINFO:
		if (vd->active != NULL) {
			wdf = (void *)data;
			wdf->height = sc->sc_height;
			wdf->width = sc->sc_width;
			wdf->depth = 32;
			wdf->cmsize = 256;
			return 0;
		} else
			return ENODEV;
	case WSDISPLAYIO_GETCMAP:
		if (sc->sc_depth == 8)
			return crmfb_getcmap(sc, (struct wsdisplay_cmap *)data);
		else
			return EINVAL;
	case WSDISPLAYIO_PUTCMAP:
		if (sc->sc_depth == 8)
			return crmfb_putcmap(sc, (struct wsdisplay_cmap *)data);
		else
			return EINVAL;
	case WSDISPLAYIO_LINEBYTES:
		*(u_int *)data = sc->sc_width * sc->sc_depth / 8;
		return 0;
	case WSDISPLAYIO_SMODE:
		nmode = *(int *)data;
		if (nmode != sc->sc_wsmode) {
			sc->sc_wsmode = nmode;
			if (nmode == WSDISPLAYIO_MODE_EMUL) {
				crmfb_setup_video(sc, sc->sc_console_depth);
				crmfb_setup_palette(sc);
				vcons_redraw_screen(vd->active);
			} else {
				crmfb_setup_video(sc, 32);
			}
		}
		return 0;
	case WSDISPLAYIO_SVIDEO:
		{
			int d = *(int *)data;
			if (d == sc->sc_video_on)
				return 0;
			sc->sc_video_on = d;
			if (d == WSDISPLAYIO_VIDEO_ON) {
				crmfb_write_reg(sc,
				    CRMFB_VT_FLAGS, sc->sc_vtflags);
			} else {
				/* turn all SYNCs off */
				crmfb_write_reg(sc, CRMFB_VT_FLAGS,
				    sc->sc_vtflags | CRMFB_VT_FLAGS_VDRV_LOW |
				     CRMFB_VT_FLAGS_HDRV_LOW |
				     CRMFB_VT_FLAGS_SYNC_LOW);
			}
		}
		return 0;
					
	case WSDISPLAYIO_GVIDEO:
		*(int *)data = sc->sc_video_on;
		return 0;

	case WSDISPLAYIO_GCURPOS:
		{
			struct wsdisplay_curpos *pos;

			pos = (struct wsdisplay_curpos *)data;
			pos->x = sc->sc_cur_x;
			pos->y = sc->sc_cur_y;
		}
		return 0;
	case WSDISPLAYIO_SCURPOS:
		{
			struct wsdisplay_curpos *pos;

			pos = (struct wsdisplay_curpos *)data;
			crmfb_set_curpos(sc, pos->x, pos->y);
		}
		return 0;
	case WSDISPLAYIO_GCURMAX:
		{
			struct wsdisplay_curpos *pos;

			pos = (struct wsdisplay_curpos *)data;
			pos->x = 32;
			pos->y = 32;
		}
		return 0;
	case WSDISPLAYIO_GCURSOR:
		{
			struct wsdisplay_cursor *cu;

			cu = (struct wsdisplay_cursor *)data;
			return crmfb_gcursor(sc, cu);
		}
	case WSDISPLAYIO_SCURSOR:
		{
			struct wsdisplay_cursor *cu;

			cu = (struct wsdisplay_cursor *)data;
			return crmfb_scursor(sc, cu);
		}
	case WSDISPLAYIO_GET_EDID: {
		struct wsdisplayio_edid_info *d = data;

		d->data_size = 128;
		if (d->buffer_size < 128)
			return EAGAIN;
		if (sc->sc_edid_data[1] == 0)
			return ENODATA;
		return copyout(sc->sc_edid_data, d->edid_data, 128);
	}
	}
	return EPASSTHROUGH;
}

static paddr_t
crmfb_mmap(void *v, void *vs, off_t offset, int prot)
{
	struct vcons_data *vd;
	struct crmfb_softc *sc;
	paddr_t pa;

	vd = (struct vcons_data *)v;
	sc = (struct crmfb_softc *)vd->cookie;

	/* we probably shouldn't let anyone mmap the framebuffer */
#if 1
	if (offset >= 0 && offset < (0x100000 * sc->sc_tiles_x)) {
		pa = bus_dmamem_mmap(sc->sc_dmat, sc->sc_dma.segs,
		    sc->sc_dma.nsegs, offset, prot,
		    BUS_DMA_WAITOK | BUS_DMA_COHERENT | BUS_DMA_PREFETCHABLE);
		return pa;
	}
#endif
	/*
	 * here would the TLBs be but we don't want to show them to userland
	 * so we return the page containing the status register 
	 */
	if ((offset >= 0x15000000) && (offset < 0x15002000))
		return bus_space_mmap(sc->sc_iot, 0x15004000, 0, prot, 0);
	/* now the actual engine registers */
	if ((offset >= 0x15002000) && (offset < 0x15005000))
		return bus_space_mmap(sc->sc_iot, offset, 0, prot, 0);
	/* and now the linear area */
	if ((offset >= 0x15010000) && (offset < 0x15030000))
		return bus_dmamem_mmap(sc->sc_dmat, sc->sc_dma.segs,
		     sc->sc_dma.nsegs,
		     offset + (0x100000 * sc->sc_tiles_x) - 0x15010000, prot,
		     BUS_DMA_WAITOK | BUS_DMA_COHERENT | BUS_DMA_PREFETCHABLE);
	return -1;
}

static void
crmfb_init_screen(void *c, struct vcons_screen *scr, int existing,
    long *defattr)
{
	struct crmfb_softc *sc;
	struct rasops_info *ri;

	sc = (struct crmfb_softc *)c;
	ri = &scr->scr_ri;

	scr->scr_flags |= VCONS_LOADFONT;

	ri->ri_flg = RI_CENTER | RI_FULLCLEAR |
		     RI_ENABLE_ALPHA | RI_PREFER_ALPHA;
	ri->ri_depth = sc->sc_console_depth;
	ri->ri_width = sc->sc_width;
	ri->ri_height = sc->sc_height;
	ri->ri_stride = ri->ri_width * (ri->ri_depth / 8);

	switch (ri->ri_depth) {
	case 8:
		ri->ri_flg |= RI_8BIT_IS_RGB;
		break;
	case 16:
		ri->ri_rnum = ri->ri_gnum = ri->ri_bnum = 5;
		ri->ri_rpos = 11;
		ri->ri_gpos = 6;
		ri->ri_bpos = 1;
		break;
	case 32:
		ri->ri_rnum = ri->ri_gnum = ri->ri_bnum = 8;
		ri->ri_rpos = 8;
		ri->ri_gpos = 16;
		ri->ri_bpos = 24;
		break;
	}

	ri->ri_bits = NULL;

	rasops_init(ri, 0, 0);
	ri->ri_caps = WSSCREEN_WSCOLORS | WSSCREEN_RESIZE;
	rasops_reconfig(ri, ri->ri_height / ri->ri_font->fontheight,
	    ri->ri_width / ri->ri_font->fontwidth);
	ri->ri_hw = scr;

	ri->ri_ops.cursor    = crmfb_cursor;
	ri->ri_ops.copyrows  = crmfb_copyrows;
	ri->ri_ops.eraserows = crmfb_eraserows;
	ri->ri_ops.copycols  = crmfb_copycols;
	ri->ri_ops.erasecols = crmfb_erasecols;
	if (FONT_IS_ALPHA(ri->ri_font)) {
		ri->ri_ops.putchar   = crmfb_putchar_aa;
	} else {
		ri->ri_ops.putchar   = crmfb_putchar;
	}
	return;
}

static int
crmfb_putcmap(struct crmfb_softc *sc, struct wsdisplay_cmap *cm)
{
	u_int idx, cnt;
	u_char r[256], g[256], b[256];
	u_char *rp, *gp, *bp;
	int rv, i;

	idx = cm->index;
	cnt = cm->count;

	if (idx >= 255 || cnt > 256 || idx + cnt > 256)
		return EINVAL;

	rv = copyin(cm->red, &r[idx], cnt);
	if (rv)
		return rv;
	rv = copyin(cm->green, &g[idx], cnt);
	if (rv)
		return rv;
	rv = copyin(cm->blue, &b[idx], cnt);
	if (rv)
		return rv;

	memcpy(&sc->sc_cmap_red[idx], &r[idx], cnt);
	memcpy(&sc->sc_cmap_green[idx], &g[idx], cnt);
	memcpy(&sc->sc_cmap_blue[idx], &b[idx], cnt);

	rp = &sc->sc_cmap_red[idx];
	gp = &sc->sc_cmap_green[idx];
	bp = &sc->sc_cmap_blue[idx];

	for (i = 0; i < cnt; i++) {
		crmfb_set_palette(sc, idx, *rp, *gp, *bp);
		idx++;
		rp++, gp++, bp++;
	}

	return 0;
}

static int
crmfb_getcmap(struct crmfb_softc *sc, struct wsdisplay_cmap *cm)
{
	u_int idx, cnt;
	int rv;

	idx = cm->index;
	cnt = cm->count;

	if (idx >= 255 || cnt > 256 || idx + cnt > 256)
		return EINVAL;

	rv = copyout(&sc->sc_cmap_red[idx], cm->red, cnt);
	if (rv)
		return rv;
	rv = copyout(&sc->sc_cmap_green[idx], cm->green, cnt);
	if (rv)
		return rv;
	rv = copyout(&sc->sc_cmap_blue[idx], cm->blue, cnt);
	if (rv)
		return rv;

	return 0;
}

static void
crmfb_set_palette(struct crmfb_softc *sc, int reg, uint8_t r, uint8_t g,
    uint8_t b)
{
	uint32_t val;

	if (reg > 255 || sc->sc_depth != 8)
		return;

	while (bus_space_read_4(sc->sc_iot, sc->sc_ioh, CRMFB_CMAP_FIFO) >= 63)
		DELAY(10);

	val = (r << 8) | (g << 16) | (b << 24);
	crmfb_write_reg(sc, CRMFB_CMAP + (reg * 4), val);

	return;
}

static int
crmfb_set_curpos(struct crmfb_softc *sc, int x, int y)
{
	uint32_t val;

	sc->sc_cur_x = x;
	sc->sc_cur_y = y;

	val = ((x - sc->sc_hot_x) & 0xffff) | ((y - sc->sc_hot_y) << 16);
	crmfb_write_reg(sc, CRMFB_CURSOR_POS, val);

	return 0;
}

static int
crmfb_gcursor(struct crmfb_softc *sc, struct wsdisplay_cursor *cur)
{
	/* do nothing for now */
	return 0;
}

static int
crmfb_scursor(struct crmfb_softc *sc, struct wsdisplay_cursor *cur)
{
	if (cur->which & WSDISPLAY_CURSOR_DOCUR) {

		crmfb_write_reg(sc, CRMFB_CURSOR_CONTROL, cur->enable ? 1 : 0);
	}
	if (cur->which & WSDISPLAY_CURSOR_DOHOT) {

		sc->sc_hot_x = cur->hot.x;
		sc->sc_hot_y = cur->hot.y;
	}
	if (cur->which & WSDISPLAY_CURSOR_DOPOS) {

		crmfb_set_curpos(sc, cur->pos.x, cur->pos.y);
	}
	if (cur->which & WSDISPLAY_CURSOR_DOCMAP) {
		int i;
		uint32_t val;
	
		for (i = 0; i < cur->cmap.count; i++) {
			val = (cur->cmap.red[i] << 24) |
			      (cur->cmap.green[i] << 16) |
			      (cur->cmap.blue[i] << 8);
			crmfb_write_reg(sc, CRMFB_CURSOR_CMAP0 + 
			    ((i + cur->cmap.index) << 2), val);
		}
	}
	if (cur->which & WSDISPLAY_CURSOR_DOSHAPE) {

		int i, j, cnt = 0;
		uint32_t latch = 0, omask;
		uint8_t imask;
		for (i = 0; i < 64; i++) {
			omask = 0x80000000;
			imask = 0x01;
			cur->image[cnt] &= cur->mask[cnt];
			for (j = 0; j < 8; j++) {
				if (cur->image[cnt] & imask)
					latch |= omask;
				omask >>= 1;
				if (cur->mask[cnt] & imask)
					latch |= omask;
				omask >>= 1;
				imask <<= 1;
			}
			cnt++;
			imask = 0x01;
			cur->image[cnt] &= cur->mask[cnt];
			for (j = 0; j < 8; j++) {
				if (cur->image[cnt] & imask)
					latch |= omask;
				omask >>= 1;
				if (cur->mask[cnt] & imask)
					latch |= omask;
				omask >>= 1;
				imask <<= 1;
			}
			cnt++;
			crmfb_write_reg(sc, CRMFB_CURSOR_BITMAP + (i << 2),
			    latch);
			latch = 0;
		}				
	}
	return 0;
}

static inline void
crmfb_write_reg(struct crmfb_softc *sc, int offset, uint32_t val)
{

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, offset, val);
	wbflush();
}

static inline uint32_t
crmfb_read_reg(struct crmfb_softc *sc, int offset)
{

	return bus_space_read_4(sc->sc_iot, sc->sc_ioh, offset);
}

static inline void
crmfb_wait_idle(struct crmfb_softc *sc)
{
	int i = 0;

	do {
		i++;
	} while (((bus_space_read_4(sc->sc_iot, sc->sc_reh, CRIME_DE_STATUS) &
		   CRIME_DE_IDLE) == 0) && (i < 100000000));
	if (i >= 100000000)
		aprint_error("crmfb_wait_idle() timed out\n");
	sc->sc_needs_sync = 0;
}

/* writes to CRIME_DE_MODE_* only take effect when the engine is idle */

static inline void
crmfb_src_mode(struct crmfb_softc *sc, uint32_t mode)
{
	if (mode == sc->sc_src_mode)
		return;
	bus_space_write_4(sc->sc_iot, sc->sc_reh, CRIME_DE_MODE_SRC, mode);
	sc->sc_needs_sync = 1;
	sc->sc_src_mode = mode;
}

static inline void
crmfb_dst_mode(struct crmfb_softc *sc, uint32_t mode)
{
	if (mode == sc->sc_dst_mode)
		return;
	bus_space_write_4(sc->sc_iot, sc->sc_reh, CRIME_DE_MODE_DST, mode);
	sc->sc_needs_sync = 1;
	sc->sc_dst_mode = mode;
}

static inline void
crmfb_make_room(struct crmfb_softc *sc, int num)
{
	int i = 0, slots;
	uint32_t status;

	if (sc->sc_needs_sync != 0) {
		crmfb_wait_idle(sc);
		return;
	}

	do {
		i++;
		status = bus_space_read_4(sc->sc_iot, sc->sc_reh,
		    CRIME_DE_STATUS);
		slots = 60 - CRIME_PIPE_LEVEL(status);
	} while (slots <= num);
}

static int
crmfb_wait_dma_idle(struct crmfb_softc *sc)
{
	int bail = 100000, idle;

	do {
		idle = ((bus_space_read_4(sc->sc_iot, sc->sc_ioh,
		         CRMFB_OVR_CONTROL) & 1) == 0) &&
		       ((bus_space_read_4(sc->sc_iot, sc->sc_ioh,
		         CRMFB_FRM_CONTROL) & 1) == 0) &&
		       ((bus_space_read_4(sc->sc_iot, sc->sc_ioh,
		         CRMFB_DID_CONTROL) & 1) == 0);
		if (!idle)
			delay(10);
		bail--;
	} while ((!idle) && (bail > 0));
	return idle;
}

static int
crmfb_setup_video(struct crmfb_softc *sc, int depth)
{
	uint64_t reg;
	uint32_t d, h, page;
	int i, bail, tile_width, tlbptr, lptr, j, tx, shift, overhang;
	const char *wantsync;
	uint16_t v;

	/* disable DMA */
	d = bus_space_read_4(sc->sc_iot, sc->sc_ioh, CRMFB_OVR_CONTROL);
	d &= ~(1 << CRMFB_OVR_CONTROL_DMAEN_SHIFT);
	crmfb_write_reg(sc, CRMFB_OVR_CONTROL, d);
	DELAY(50000);
	d = bus_space_read_4(sc->sc_iot, sc->sc_ioh, CRMFB_FRM_CONTROL);
	d &= ~(1 << CRMFB_FRM_CONTROL_DMAEN_SHIFT);
	crmfb_write_reg(sc, CRMFB_FRM_CONTROL, d);
	DELAY(50000);
	crmfb_write_reg(sc, CRMFB_DID_CONTROL, d);
	DELAY(50000);

	if (!crmfb_wait_dma_idle(sc))
		aprint_error("crmfb: crmfb_wait_dma_idle timed out\n");
	
	/* ensure that CRM starts drawing at the top left of the screen
	 * when we re-enable DMA later
	 */
	d = (1 << CRMFB_VT_XY_FREEZE_SHIFT);
	crmfb_write_reg(sc, CRMFB_VT_XY, d);
	delay(1000);
	d = bus_space_read_4(sc->sc_iot, sc->sc_ioh, CRMFB_DOTCLOCK);
	d &= ~(1 << CRMFB_DOTCLOCK_CLKRUN_SHIFT);
	crmfb_write_reg(sc, CRMFB_DOTCLOCK, d);

	/* wait for dotclock to turn off */
	bail = 10000;
	while ((bus_space_read_4(sc->sc_iot, sc->sc_ioh, CRMFB_DOTCLOCK) &
	    (1 << CRMFB_DOTCLOCK_CLKRUN_SHIFT)) && (bail > 0)) {
		delay(10);
		bail--;
	}

	/* reset FIFO */
	d = bus_space_read_4(sc->sc_iot, sc->sc_ioh, CRMFB_FRM_TILESIZE);
	d |= (1 << CRMFB_FRM_TILESIZE_FIFOR_SHIFT);
	crmfb_write_reg(sc, CRMFB_FRM_TILESIZE, d);
	d &= ~(1 << CRMFB_FRM_TILESIZE_FIFOR_SHIFT);
	crmfb_write_reg(sc, CRMFB_FRM_TILESIZE, d);

	/* setup colour mode */
	switch (depth) {
	case 8:
		h = CRMFB_MODE_TYP_RG3B2;
		tile_width = 512;
		break;
	case 16:
		h = CRMFB_MODE_TYP_ARGB5;
		tile_width = 256;
		break;
	case 32:
		h = CRMFB_MODE_TYP_RGB8;
		tile_width = 128;
		break;
	default:
		panic("Unsupported depth");
	}
	d = h << CRMFB_MODE_TYP_SHIFT;
	d |= CRMFB_MODE_BUF_BOTH << CRMFB_MODE_BUF_SHIFT;
	for (i = 0; i < (32 * 4); i += 4)
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, CRMFB_MODE + i, d);
	wbflush();

	/* setup tile pointer, but don't turn on DMA yet! */
	h = DMAADDR(sc->sc_dmai);
	d = (h >> 9) << CRMFB_FRM_CONTROL_TILEPTR_SHIFT;
	crmfb_write_reg(sc, CRMFB_FRM_CONTROL, d);

	/* init framebuffer width and pixel size */
	/*d = (1 << CRMFB_FRM_TILESIZE_WIDTH_SHIFT);*/

	d = ((int)(sc->sc_width / tile_width)) << 
	    CRMFB_FRM_TILESIZE_WIDTH_SHIFT;
	overhang = sc->sc_width % tile_width;
	if (overhang != 0) {
		uint32_t val; 
		DPRINTF("tile width: %d\n", tile_width);
		DPRINTF("overhang: %d\n", overhang);
		val = (overhang * (depth >> 3)) >> 5;
		DPRINTF("reg: %08x\n", val);
		d |= (val & 0x1f);
		DPRINTF("d: %08x\n", d);
	}

	switch (depth) {
	case 8:
		h = CRMFB_FRM_TILESIZE_DEPTH_8;
		break;
	case 16:
		h = CRMFB_FRM_TILESIZE_DEPTH_16;
		break;
	case 32:
		h = CRMFB_FRM_TILESIZE_DEPTH_32;
		break;
	default:
		panic("Unsupported depth");
	}
	d |= (h << CRMFB_FRM_TILESIZE_DEPTH_SHIFT);
	crmfb_write_reg(sc, CRMFB_FRM_TILESIZE, d);

	/*h = sc->sc_width * sc->sc_height / (512 / (depth >> 3));*/
	h = sc->sc_height;
	d = h << CRMFB_FRM_PIXSIZE_HEIGHT_SHIFT;
	crmfb_write_reg(sc, CRMFB_FRM_PIXSIZE, d);

	/* turn off firmware overlay and hardware cursor */
	crmfb_write_reg(sc, CRMFB_OVR_WIDTH_TILE, 0);
	crmfb_write_reg(sc, CRMFB_CURSOR_CONTROL, 0);

	/* turn on DMA for the framebuffer */
	d = bus_space_read_4(sc->sc_iot, sc->sc_ioh, CRMFB_FRM_CONTROL);
	d |= (1 << CRMFB_FRM_CONTROL_DMAEN_SHIFT);
	crmfb_write_reg(sc, CRMFB_FRM_CONTROL, d);

	/* enable drawing again */
	crmfb_write_reg(sc, CRMFB_VT_XY, 0);
	d = bus_space_read_4(sc->sc_iot, sc->sc_ioh, CRMFB_DOTCLOCK);
	d |= (1 << CRMFB_DOTCLOCK_CLKRUN_SHIFT);
	crmfb_write_reg(sc, CRMFB_DOTCLOCK, d);

	/* turn off sync-on-green */

	wantsync = arcbios_GetEnvironmentVariable("SyncOnGreen");
	if ( (wantsync != NULL) && (wantsync[0] == 'n') ) {
		sc->sc_vtflags |= CRMFB_VT_FLAGS_SYNC_LOW;
		crmfb_write_reg(sc, CRMFB_VT_FLAGS, d);
	}

	sc->sc_depth = depth;

	/* finally set up the drawing engine's TLB A */
	v = (DMAADDR(sc->sc_dma) >> 16) & 0xffff;
	tlbptr = 0;
	tx = ((sc->sc_width + (tile_width - 1)) & ~(tile_width - 1)) / 
	    tile_width;

	DPRINTF("tx: %d\n", tx);

	for (i = 0; i < 16; i++) {
		reg = 0;
		shift = 64;
		lptr = 0;
		for (j = 0; j < tx; j++) {
			shift -= 16;
			reg |= (((uint64_t)(v | 0x8000)) << shift);
			if (shift == 0) {
				shift = 64;
				bus_space_write_8(sc->sc_iot, sc->sc_reh,
				    CRIME_RE_TLB_A + tlbptr + lptr, 
				    reg);
				DPRINTF("%04x: %016"PRIx64"\n", tlbptr + lptr, reg);
				reg = 0;
				lptr += 8;
			}
			v++;
		}
		if (shift != 64) {
			bus_space_write_8(sc->sc_iot, sc->sc_reh,
			    CRIME_RE_TLB_A + tlbptr + lptr, reg);
			DPRINTF("%04x: %016"PRIx64"\n", tlbptr + lptr, reg);
		}
		tlbptr += 32;
	}

	/* now put the last 128kB into the 1st linear TLB */
	page = (sc->sc_linear >> 12) | 0x80000000;
	tlbptr = 0;
	for (i = 0; i < 16; i++) {
		reg = ((uint64_t)page << 32) | (page + 1);
		bus_space_write_8(sc->sc_iot, sc->sc_reh,
		    CRIME_RE_LINEAR_A + tlbptr, reg);
		page += 2;
		tlbptr += 8;
	}
	wbflush();

	/* do some very basic engine setup */
	bus_space_write_4(sc->sc_iot, sc->sc_reh, CRIME_DE_CLIPMODE, 0);
	bus_space_write_4(sc->sc_iot, sc->sc_reh, CRIME_DE_WINOFFSET_SRC, 0);
	bus_space_write_4(sc->sc_iot, sc->sc_reh, CRIME_DE_WINOFFSET_DST, 0);
	bus_space_write_4(sc->sc_iot, sc->sc_reh, CRIME_DE_PLANEMASK, 
	    0xffffffff);

	bus_space_write_8(sc->sc_iot, sc->sc_reh, 0x20, 0);
	bus_space_write_8(sc->sc_iot, sc->sc_reh, 0x28, 0);
	bus_space_write_8(sc->sc_iot, sc->sc_reh, 0x30, 0);
	bus_space_write_8(sc->sc_iot, sc->sc_reh, 0x38, 0);
	bus_space_write_8(sc->sc_iot, sc->sc_reh, 0x40, 0);
	
	switch (depth) {
	case 8:
		sc->sc_de_mode = DE_MODE_TLB_A | DE_MODE_BUFDEPTH_8 |
		    DE_MODE_TYPE_CI | DE_MODE_PIXDEPTH_8;
		sc->sc_mte_mode = MTE_MODE_DST_ECC |
		    (MTE_TLB_A << MTE_DST_TLB_SHIFT) |
		    (MTE_TLB_A << MTE_SRC_TLB_SHIFT) |
		    (MTE_DEPTH_8 << MTE_DEPTH_SHIFT);
		sc->sc_mte_x_shift = 0;
		break;
	case 16:
		sc->sc_de_mode = DE_MODE_TLB_A | DE_MODE_BUFDEPTH_16 |
		    DE_MODE_TYPE_RGBA | DE_MODE_PIXDEPTH_16;
		sc->sc_mte_mode = MTE_MODE_DST_ECC |
		    (MTE_TLB_A << MTE_DST_TLB_SHIFT) |
		    (MTE_TLB_A << MTE_SRC_TLB_SHIFT) |
		    (MTE_DEPTH_16 << MTE_DEPTH_SHIFT);
		sc->sc_mte_x_shift = 1;
		break;
	case 32:
		sc->sc_de_mode = DE_MODE_TLB_A | DE_MODE_BUFDEPTH_32 |
		    DE_MODE_TYPE_RGBA | DE_MODE_PIXDEPTH_32;
		sc->sc_mte_mode = MTE_MODE_DST_ECC |
		    (MTE_TLB_A << MTE_DST_TLB_SHIFT) |
		    (MTE_TLB_A << MTE_SRC_TLB_SHIFT) |
		    (MTE_DEPTH_32 << MTE_DEPTH_SHIFT);
		sc->sc_mte_x_shift = 2;
		break;
	default:
		panic("%s: unsuported colour depth %d\n", __func__,
		    depth);
	}
	sc->sc_needs_sync = 0;
	sc->sc_src_mode = 0xffffffff;
	sc->sc_dst_mode = 0xffffffff;

	crmfb_src_mode(sc, sc->sc_de_mode);
	crmfb_dst_mode(sc, sc->sc_de_mode);

	bus_space_write_4(sc->sc_iot, sc->sc_reh, CRIME_DE_XFER_STEP_X, 1);
	bus_space_write_4(sc->sc_iot, sc->sc_reh, CRIME_DE_XFER_STEP_Y, 1);

	/* initialize memory transfer engine */
	bus_space_write_4(sc->sc_iot, sc->sc_reh, CRIME_MTE_MODE,
	    sc->sc_mte_mode | MTE_MODE_COPY);
	sc->sc_mte_direction = 1;
	bus_space_write_4(sc->sc_iot, sc->sc_reh, CRIME_MTE_DST_Y_STEP, 1);
	bus_space_write_4(sc->sc_iot, sc->sc_reh, CRIME_MTE_SRC_Y_STEP, 1);

	return 0;
}

static void
crmfb_set_mte_direction(struct crmfb_softc *sc, int dir)
{
	if (dir == sc->sc_mte_direction)
		return;

	crmfb_make_room(sc, 2);
	bus_space_write_4(sc->sc_iot, sc->sc_reh, CRIME_MTE_DST_Y_STEP, dir);
	bus_space_write_4(sc->sc_iot, sc->sc_reh, CRIME_MTE_SRC_Y_STEP, dir);
	sc->sc_mte_direction = dir;
}

static void
crmfb_setup_palette(struct crmfb_softc *sc)
{
	int i, j, x;
	uint32_t col;
	struct rasops_info *ri = &crmfb_console_screen.scr_ri;

	for (i = 0; i < 256; i++) {
		crmfb_set_palette(sc, i, rasops_cmap[(i * 3) + 2],
		    rasops_cmap[(i * 3) + 1], rasops_cmap[(i * 3) + 0]);
		sc->sc_cmap_red[i] = rasops_cmap[(i * 3) + 2];
		sc->sc_cmap_green[i] = rasops_cmap[(i * 3) + 1];
		sc->sc_cmap_blue[i] = rasops_cmap[(i * 3) + 0];
	}

	if (FONT_IS_ALPHA(ri->ri_font)) {	
		sc->sc_de_mode =
		    (sc->sc_de_mode & ~DE_MODE_TYPE_MASK) | DE_MODE_TYPE_RGB;
	}

	/* draw 16 character cells in 32bit RGBA for alpha blending */
	crmfb_make_room(sc, 3);
	crmfb_dst_mode(sc,
	    DE_MODE_TLB_A |
	    DE_MODE_BUFDEPTH_32 |
	    DE_MODE_TYPE_RGBA |
	    DE_MODE_PIXDEPTH_32);
	bus_space_write_4(sc->sc_iot, sc->sc_reh, CRIME_DE_DRAWMODE,
	    DE_DRAWMODE_PLANEMASK | DE_DRAWMODE_BYTEMASK);
	bus_space_write_4(sc->sc_iot, sc->sc_reh, CRIME_DE_PRIMITIVE,
	    DE_PRIM_RECTANGLE | DE_PRIM_TB);
	j = 0;
	x = 0;
	for (i = 0; i < 16; i++) {
		crmfb_make_room(sc, 2);
		col = (rasops_cmap[j] << 24) | 
		      (rasops_cmap[j + 1] << 16) | 
		      (rasops_cmap[j + 2] << 8);
		bus_space_write_4(sc->sc_iot, sc->sc_reh, CRIME_DE_FG, col);
		bus_space_write_4(sc->sc_iot, sc->sc_reh, CRIME_DE_X_VERTEX_0,
	    	    (x << 16) | ((sc->sc_height - 500) & 0xffff));
		bus_space_write_4(sc->sc_iot, sc->sc_reh,
	    	    CRIME_DE_X_VERTEX_1 | CRIME_DE_START,
		    ((x + ri->ri_font->fontwidth - 1)  << 16) |
		    ((sc->sc_height + ri->ri_font->fontheight - 1) & 0xffff));
		j += 3;
		x += ri->ri_font->fontwidth;
	}
	crmfb_dst_mode(sc, sc->sc_de_mode);
}

static void
crmfb_fill_rect(struct crmfb_softc *sc, int x, int y, int width, int height,
    uint32_t colour)
{
	int rxa, rxe;

	rxa = x << sc->sc_mte_x_shift;
	rxe = ((x + width) << sc->sc_mte_x_shift) - 1;
	crmfb_set_mte_direction(sc, 1);
	crmfb_make_room(sc, 4);
	bus_space_write_4(sc->sc_iot, sc->sc_reh, CRIME_MTE_MODE,
	    sc->sc_mte_mode | 0);
	bus_space_write_4(sc->sc_iot, sc->sc_reh, CRIME_MTE_BG, colour);
	bus_space_write_4(sc->sc_iot, sc->sc_reh, CRIME_MTE_DST0,
	    (rxa << 16) | (y & 0xffff));
	bus_space_write_4(sc->sc_iot, sc->sc_reh,
	    CRIME_MTE_DST1 | CRIME_DE_START,
	    (rxe << 16) | ((y + height - 1) & 0xffff));
}

static void
crmfb_bitblt(struct crmfb_softc *sc, int xs, int ys, int xd, int yd,
    int wi, int he, uint32_t rop)
{
	uint32_t prim = DE_PRIM_RECTANGLE;
	int rxa, rya, rxe, rye, rxs, rys;
	crmfb_make_room(sc, 2);
	crmfb_src_mode(sc, sc->sc_de_mode);
	crmfb_make_room(sc, 6);
	bus_space_write_4(sc->sc_iot, sc->sc_reh, CRIME_DE_DRAWMODE,
	    DE_DRAWMODE_PLANEMASK | DE_DRAWMODE_BYTEMASK | DE_DRAWMODE_ROP |
	    DE_DRAWMODE_XFER_EN);
	bus_space_write_4(sc->sc_iot, sc->sc_reh, CRIME_DE_ROP, rop);
	if (xs < xd) {
		prim |= DE_PRIM_RL;
		rxe = xd;
		rxa = xd + wi - 1;
		rxs = xs + wi - 1;
	} else {
		prim |= DE_PRIM_LR;
		rxe = xd + wi - 1;
		rxa = xd;
		rxs = xs;
	}
	if (ys < yd) {
		prim |= DE_PRIM_BT;
		rye = yd;
		rya = yd + he - 1;
		rys = ys + he - 1;
	} else {
		prim |= DE_PRIM_TB;
		rye = yd + he - 1;
		rya = yd;
		rys = ys;
	}
	bus_space_write_4(sc->sc_iot, sc->sc_reh, CRIME_DE_PRIMITIVE, prim);
	bus_space_write_4(sc->sc_iot, sc->sc_reh, CRIME_DE_XFER_ADDR_SRC,
	    (rxs << 16) | (rys & 0xffff));
	bus_space_write_4(sc->sc_iot, sc->sc_reh, CRIME_DE_X_VERTEX_0,
	    (rxa << 16) | (rya & 0xffff));
	bus_space_write_4(sc->sc_iot, sc->sc_reh,
	    CRIME_DE_X_VERTEX_1 | CRIME_DE_START,
	    (rxe << 16) | (rye & 0xffff));
}

static void
crmfb_scroll(struct crmfb_softc *sc, int xs, int ys, int xd, int yd,
    int wi, int he)
{
	int rxa, rya, rxe, rye, rxd, ryd, rxde, ryde;

	rxa = xs << sc->sc_mte_x_shift;
	rxd = xd << sc->sc_mte_x_shift;
	rxe = ((xs + wi) << sc->sc_mte_x_shift) - 1;
	rxde = ((xd + wi) << sc->sc_mte_x_shift) - 1;

	crmfb_make_room(sc, 1);

	bus_space_write_4(sc->sc_iot, sc->sc_reh, CRIME_MTE_MODE,
	    sc->sc_mte_mode | MTE_MODE_COPY);

	if (ys < yd) {
		/* bottom to top */
		rye = ys;
		rya = ys + he - 1;
		ryd = yd + he - 1;
		ryde = yd;
		crmfb_set_mte_direction(sc, -1);
	} else {
		/* top to bottom */
		rye = ys + he - 1;
		rya = ys;
		ryd = yd;
		ryde = yd + he - 1;
		crmfb_set_mte_direction(sc, 1);
	}
	crmfb_make_room(sc, 4);
	bus_space_write_4(sc->sc_iot, sc->sc_reh, CRIME_MTE_SRC0,
	    (rxa << 16) | rya);
	bus_space_write_4(sc->sc_iot, sc->sc_reh, CRIME_MTE_SRC1,
	    (rxe << 16) | rye);
	bus_space_write_4(sc->sc_iot, sc->sc_reh,
	    CRIME_MTE_DST0,
	    (rxd << 16) | ryd);
	bus_space_write_4(sc->sc_iot, sc->sc_reh, CRIME_MTE_DST1 | 
	    CRIME_DE_START,
	    (rxde << 16) | ryde);
}

static void
crmfb_copycols(void *cookie, int row, int srccol, int dstcol, int ncols)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	int32_t xs, xd, y, width, height;

	xs = ri->ri_xorigin + ri->ri_font->fontwidth * srccol;
	xd = ri->ri_xorigin + ri->ri_font->fontwidth * dstcol;
	y = ri->ri_yorigin + ri->ri_font->fontheight * row;
	width = ri->ri_font->fontwidth * ncols;
	height = ri->ri_font->fontheight;
	crmfb_bitblt(scr->scr_cookie, xs, y, xd, y, width, height, 3);
}

static void
crmfb_erasecols(void *cookie, int row, int startcol, int ncols, long fillattr)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	int32_t x, y, width, height, bg;

	x = ri->ri_xorigin + ri->ri_font->fontwidth * startcol;
	y = ri->ri_yorigin + ri->ri_font->fontheight * row;
	width = ri->ri_font->fontwidth * ncols;
	height = ri->ri_font->fontheight;
	bg = (uint32_t)ri->ri_devcmap[(fillattr >> 16) & 0xff];
	crmfb_fill_rect(scr->scr_cookie, x, y, width, height, bg);
}

static void
crmfb_copyrows(void *cookie, int srcrow, int dstrow, int nrows)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	int32_t x, ys, yd, width, height;

	x = ri->ri_xorigin;
	ys = ri->ri_yorigin + ri->ri_font->fontheight * srcrow;
	yd = ri->ri_yorigin + ri->ri_font->fontheight * dstrow;
	width = ri->ri_emuwidth;
	height = ri->ri_font->fontheight * nrows;

	crmfb_scroll(scr->scr_cookie, x, ys, x, yd, width, height);
}

static void
crmfb_eraserows(void *cookie, int row, int nrows, long fillattr)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	int32_t x, y, width, height, bg;

	if ((row == 0) && (nrows == ri->ri_rows)) {
		x = y = 0;
		width = ri->ri_width;
		height = ri->ri_height;
	} else {
		x = ri->ri_xorigin;
		y = ri->ri_yorigin + ri->ri_font->fontheight * row;
		width = ri->ri_emuwidth;
		height = ri->ri_font->fontheight * nrows;
	}
	bg = (uint32_t)ri->ri_devcmap[(fillattr >> 16) & 0xff];
	crmfb_fill_rect(scr->scr_cookie, x, y, width, height, bg);
}

static void
crmfb_cursor(void *cookie, int on, int row, int col)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct crmfb_softc *sc = scr->scr_cookie;
	int x, y, wi,he;

	wi = ri->ri_font->fontwidth;
	he = ri->ri_font->fontheight;

	if (ri->ri_flg & RI_CURSOR) {
		x = ri->ri_ccol * wi + ri->ri_xorigin;
		y = ri->ri_crow * he + ri->ri_yorigin;
		crmfb_bitblt(sc, x, y, x, y, wi, he, 12);
		ri->ri_flg &= ~RI_CURSOR;
	}

	ri->ri_crow = row;
	ri->ri_ccol = col;

	if (on)
	{
		x = ri->ri_ccol * wi + ri->ri_xorigin;
		y = ri->ri_crow * he + ri->ri_yorigin;
		crmfb_bitblt(sc, x, y, x, y, wi, he, 12);
		ri->ri_flg |= RI_CURSOR;
	}
}

static void
crmfb_putchar(void *cookie, int row, int col, u_int c, long attr)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct crmfb_softc *sc = scr->scr_cookie;
	struct wsdisplay_font *font = PICK_FONT(ri, c);
	uint32_t bg, fg;
	int x, y, wi, he, i, uc;
	uint8_t *fd8;
	uint16_t *fd16;
	void *fd;

	wi = font->fontwidth;
	he = font->fontheight;

	x = ri->ri_xorigin + col * wi;
	y = ri->ri_yorigin + row * he;

	bg = ri->ri_devcmap[(attr >> 16) & 0xff];
	fg = ri->ri_devcmap[(attr >> 24) & 0xff];
	uc = c - font->firstchar;
	fd = (uint8_t *)font->data + uc * ri->ri_fontscale;
	if (c == 0x20) {
		crmfb_fill_rect(sc, x, y, wi, he, bg);
	} else {
		crmfb_make_room(sc, 6);
		/* setup */
		bus_space_write_4(sc->sc_iot, sc->sc_reh, CRIME_DE_DRAWMODE,
		    DE_DRAWMODE_PLANEMASK | DE_DRAWMODE_BYTEMASK |
		    DE_DRAWMODE_ROP | 
		    DE_DRAWMODE_OPAQUE_STIP | DE_DRAWMODE_POLY_STIP);
		wbflush();
		bus_space_write_4(sc->sc_iot, sc->sc_reh, CRIME_DE_ROP, 3);
		bus_space_write_4(sc->sc_iot, sc->sc_reh, CRIME_DE_FG, fg);
		bus_space_write_4(sc->sc_iot, sc->sc_reh, CRIME_DE_BG, bg);
		bus_space_write_4(sc->sc_iot, sc->sc_reh, CRIME_DE_PRIMITIVE,
		    DE_PRIM_RECTANGLE | DE_PRIM_LR | DE_PRIM_TB);
		bus_space_write_4(sc->sc_iot, sc->sc_reh, CRIME_DE_STIPPLE_MODE,
		    0x001f0000);
		/* now let's feed the engine */
		crmfb_make_room(sc, 30);
		if (font->stride == 1) {
			/* shovel in 8 bit quantities */
			fd8 = fd;
			for (i = 0; i < he; i++) {
				if (i & 8)
					crmfb_make_room(sc, 30); 
				bus_space_write_4(sc->sc_iot, sc->sc_reh, 
				    CRIME_DE_STIPPLE_PAT, *fd8 << 24);
				bus_space_write_4(sc->sc_iot, sc->sc_reh,
				    CRIME_DE_X_VERTEX_0, (x << 16) | y);
				bus_space_write_4(sc->sc_iot, sc->sc_reh,
				    CRIME_DE_X_VERTEX_1 | CRIME_DE_START,
				    ((x + wi) << 16) | y);
				y++;
				fd8++;
			}
		} else if (font->stride == 2) {
			/* shovel in 16 bit quantities */
			fd16 = fd;
			for (i = 0; i < he; i++) {
				if (i & 8)
					crmfb_make_room(sc, 30); 
				bus_space_write_4(sc->sc_iot, sc->sc_reh, 
				    CRIME_DE_STIPPLE_PAT, *fd16 << 16);
				bus_space_write_4(sc->sc_iot, sc->sc_reh,
				    CRIME_DE_X_VERTEX_0, (x << 16) | y);
				bus_space_write_4(sc->sc_iot, sc->sc_reh,
				    CRIME_DE_X_VERTEX_1 | CRIME_DE_START,
				    ((x + wi) << 16) | y);
				y++;
				fd16++;
			}
		}
	}
}

static void
crmfb_putchar_aa(void *cookie, int row, int col, u_int c, long attr)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct crmfb_softc *sc = scr->scr_cookie;
	struct wsdisplay_font *font = PICK_FONT(ri, c);
	uint32_t bg, fg;
	int x, y, wi, he, uc, xx;
	void *fd;

	wi = font->fontwidth;
	he = font->fontheight;

	x = ri->ri_xorigin + col * wi;
	y = ri->ri_yorigin + row * he;

	bg = ri->ri_devcmap[(attr >> 16) & 0xff];
	fg = (attr >> 24);
	uc = c - font->firstchar;
	fd = (uint8_t *)font->data + uc * ri->ri_fontscale;

	/* fill the cell with the background colour */
	crmfb_fill_rect(sc, x, y, wi, he, bg);

	/* if all we draw is a space we're done */
	if (c == 0x20)
		return;

	/* copy the glyph into the linear buffer */
	memcpy(sc->sc_lptr, fd, ri->ri_fontscale);
	wbflush();

	/* now blit it on top of the requested fg colour cell */
	xx = fg * wi;
	crmfb_make_room(sc, 2);
	crmfb_src_mode(sc,
	    DE_MODE_LIN_A |
	    DE_MODE_BUFDEPTH_8 |
	    DE_MODE_TYPE_CI |
	    DE_MODE_PIXDEPTH_8);
	crmfb_dst_mode(sc,
	    DE_MODE_TLB_A |
	    DE_MODE_BUFDEPTH_32 |
	    DE_MODE_TYPE_CI |
	    DE_MODE_PIXDEPTH_8);

	crmfb_make_room(sc, 6);
	/* only write into the alpha channel */
	bus_space_write_4(sc->sc_iot, sc->sc_reh, CRIME_DE_DRAWMODE,
	    DE_DRAWMODE_PLANEMASK | 0x08 |
	    DE_DRAWMODE_XFER_EN);
	bus_space_write_4(sc->sc_iot, sc->sc_reh, CRIME_DE_PRIMITIVE,
	    DE_PRIM_RECTANGLE | DE_PRIM_TB);
	bus_space_write_4(sc->sc_iot, sc->sc_reh, CRIME_DE_XFER_STRD_SRC, 1);
	bus_space_write_4(sc->sc_iot, sc->sc_reh, CRIME_DE_XFER_ADDR_SRC, 0);
	bus_space_write_4(sc->sc_iot, sc->sc_reh, CRIME_DE_X_VERTEX_0,
	    (xx << 16) | (sc->sc_height & 0xffff));
	bus_space_write_4(sc->sc_iot, sc->sc_reh,
	    CRIME_DE_X_VERTEX_1 | CRIME_DE_START,
	    ((xx + wi - 1) << 16) | ((sc->sc_height + he - 1) & 0xffff));

	/* now draw the actual character */
	crmfb_make_room(sc, 2);
	crmfb_src_mode(sc,
	    DE_MODE_TLB_A |
	    DE_MODE_BUFDEPTH_32 |
	    DE_MODE_TYPE_RGBA |
	    DE_MODE_PIXDEPTH_32);
	crmfb_dst_mode(sc, sc->sc_de_mode);

	crmfb_make_room(sc, 6);
	bus_space_write_4(sc->sc_iot, sc->sc_reh, CRIME_DE_DRAWMODE,
	    DE_DRAWMODE_PLANEMASK | DE_DRAWMODE_BYTEMASK |
	    DE_DRAWMODE_ALPHA_BLEND |
	    DE_DRAWMODE_XFER_EN);
	bus_space_write_4(sc->sc_iot, sc->sc_reh, CRIME_DE_ALPHA_FUNC, 
	    DE_ALPHA_ADD |
	    (DE_ALPHA_OP_SRC_ALPHA << DE_ALPHA_OP_SRC_SHIFT) |
	    (DE_ALPHA_OP_1_MINUS_SRC_ALPHA << DE_ALPHA_OP_DST_SHIFT));
	bus_space_write_4(sc->sc_iot, sc->sc_reh, CRIME_DE_PRIMITIVE,
	    DE_PRIM_RECTANGLE | DE_PRIM_TB);
	bus_space_write_4(sc->sc_iot, sc->sc_reh, CRIME_DE_XFER_ADDR_SRC, 
	    (xx << 16) | (sc->sc_height & 0xffff));
	bus_space_write_4(sc->sc_iot, sc->sc_reh, CRIME_DE_X_VERTEX_0,
	    (x << 16) | (y & 0xffff));
	bus_space_write_4(sc->sc_iot, sc->sc_reh,
	    CRIME_DE_X_VERTEX_1 | CRIME_DE_START,
	    ((x + wi - 1) << 16) | ((y + he - 1) & 0xffff));
}

static void
crmfb_setup_ddc(struct crmfb_softc *sc)
{
	int i;

	memset(sc->sc_edid_data, 0, 128);
	sc->sc_i2c.ic_cookie = sc;
	sc->sc_i2c.ic_acquire_bus = crmfb_i2c_acquire_bus;
	sc->sc_i2c.ic_release_bus = crmfb_i2c_release_bus;
	sc->sc_i2c.ic_send_start = crmfb_i2c_send_start;
	sc->sc_i2c.ic_send_stop = crmfb_i2c_send_stop;
	sc->sc_i2c.ic_initiate_xfer = crmfb_i2c_initiate_xfer;
	sc->sc_i2c.ic_read_byte = crmfb_i2c_read_byte;
	sc->sc_i2c.ic_write_byte = crmfb_i2c_write_byte;
	sc->sc_i2c.ic_exec = NULL;
	i = 0;
	while (sc->sc_edid_data[1] == 0 && i++ < 10)
		ddc_read_edid(&sc->sc_i2c, sc->sc_edid_data, 128);
	if (i > 1)
		aprint_debug_dev(sc->sc_dev,
		    "had to try %d times to get EDID data\n", i);
	if (i < 11) {
		edid_parse(sc->sc_edid_data, &sc->sc_edid_info);
		edid_print(&sc->sc_edid_info);
	}
}

/* I2C bitbanging */
static void
crmfb_i2cbb_set_bits(void *cookie, uint32_t bits)
{
	struct crmfb_softc *sc = cookie;

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, CRMFB_I2C_VGA, bits ^ 3);
}

static void
crmfb_i2cbb_set_dir(void *cookie, uint32_t dir)
{

	/* Nothing to do */
}

static uint32_t
crmfb_i2cbb_read(void *cookie)
{
	struct crmfb_softc *sc = cookie;

	return bus_space_read_4(sc->sc_iot, sc->sc_ioh, CRMFB_I2C_VGA) ^ 3;
}

/* higher level I2C stuff */
static int
crmfb_i2c_acquire_bus(void *cookie, int flags)
{

	/* private bus */
	return 0;
}

static void
crmfb_i2c_release_bus(void *cookie, int flags)
{

	/* private bus */
}

static int
crmfb_i2c_send_start(void *cookie, int flags)
{

	return i2c_bitbang_send_start(cookie, flags, &crmfb_i2cbb_ops);
}

static int
crmfb_i2c_send_stop(void *cookie, int flags)
{

	return i2c_bitbang_send_stop(cookie, flags, &crmfb_i2cbb_ops);
}

static int
crmfb_i2c_initiate_xfer(void *cookie, i2c_addr_t addr, int flags)
{

	return i2c_bitbang_initiate_xfer(cookie, addr, flags, 
	    &crmfb_i2cbb_ops);
}

static int
crmfb_i2c_read_byte(void *cookie, uint8_t *valp, int flags)
{

	return i2c_bitbang_read_byte(cookie, valp, flags, &crmfb_i2cbb_ops);
}

static int
crmfb_i2c_write_byte(void *cookie, uint8_t val, int flags)
{

	return i2c_bitbang_write_byte(cookie, val, flags, &crmfb_i2cbb_ops);
}

/* mode setting stuff */
static uint32_t
calc_pll(int f_out)
{
	uint32_t ret;
	int f_in = 20000;	/* 20MHz in */
	int M, N, P;
	int error, div, best = 9999999;
	int ff1, ff2;
	int MM = 0, NN = 0, PP = 0, ff = 0;

	/* f_out = M * f_in / (N * (1 << P) */

	for (P = 0; P < 4; P++) {
		for (N = 64; N > 0; N--) {
			div = N * (1 << P);
			M = f_out * div / f_in;
			if ((M < 257) && (M > 100)) {
				ff1 = M * f_in / div;
				ff2 = (M + 1) * f_in / div;
				error = abs(ff1 - f_out);
				if (error < best) {
					MM = M;
					NN = N;
					PP = P;
					ff = ff1;
					best = error;
				}
				error = abs(ff2 - f_out);
				if ((error < best) && ( M < 256)){
					MM = M + 1;
					NN = N;
					PP = P;
					ff = ff2;
					best = error;
				}
			}
		}
	}
	DPRINTF("%d: M %d N %d P %d -> %d\n", f_out, MM, NN, PP, ff);
	/* now shove the parameters into the register's format */
	ret = (MM - 1) | ((NN - 1) << 8) | (P << 14);
	return ret;
}

static int
crmfb_set_mode(struct crmfb_softc *sc, const struct videomode *mode)
{
	uint32_t d, dc;
	int tmp, diff;

	switch (mode->hdisplay % 32) {
		case 0:
			sc->sc_console_depth = 8;
			break;
		case 16:
			sc->sc_console_depth = 16;
			break;
		case 8:
		case 24:
			sc->sc_console_depth = 32;
			break;
		default:
			aprint_error_dev(sc->sc_dev,
			    "hdisplay (%d) is not a multiple of 32\n",
			    mode->hdisplay);
			return FALSE;
	}
	if (mode->dot_clock > 150000) {
		aprint_error_dev(sc->sc_dev,
		    "requested dot clock is too high ( %d MHz )\n",
		    mode->dot_clock / 1000);
		return FALSE;
	}

	/* disable DMA */
	d = bus_space_read_4(sc->sc_iot, sc->sc_ioh, CRMFB_OVR_CONTROL);
	d &= ~(1 << CRMFB_OVR_CONTROL_DMAEN_SHIFT);
	crmfb_write_reg(sc, CRMFB_OVR_CONTROL, d);
	DELAY(50000);
	d = bus_space_read_4(sc->sc_iot, sc->sc_ioh, CRMFB_FRM_CONTROL);
	d &= ~(1 << CRMFB_FRM_CONTROL_DMAEN_SHIFT);
	crmfb_write_reg(sc, CRMFB_FRM_CONTROL, d);
	DELAY(50000);
	crmfb_write_reg(sc, CRMFB_DID_CONTROL, d);
	DELAY(50000);

	if (!crmfb_wait_dma_idle(sc))
		aprint_error("crmfb: crmfb_wait_dma_idle timed out\n");

	/* ok, now we're good to go */
	dc = calc_pll(mode->dot_clock);

	crmfb_write_reg(sc, CRMFB_VT_XY, 1 << CRMFB_VT_XY_FREEZE_SHIFT);
	delay(1000);

	/* set the dot clock pll but don't start it yet */
	crmfb_write_reg(sc, CRMFB_DOTCLOCK, dc);
	delay(10000);

	/* pixel counter */
	d = mode->htotal | (mode->vtotal << 12);
	crmfb_write_reg(sc, CRMFB_VT_XYMAX, d);

	/* video timings */
	d = mode->vsync_end | (mode->vsync_start << 12);
	crmfb_write_reg(sc, CRMFB_VT_VSYNC, d);

	d = mode->hsync_end | (mode->hsync_start << 12);
	crmfb_write_reg(sc, CRMFB_VT_HSYNC, d);

	d = mode->vtotal | (mode->vdisplay << 12);
	crmfb_write_reg(sc, CRMFB_VT_VBLANK, d);

	d = (mode->htotal - 5) | ((mode->hdisplay - 5) << 12);
	crmfb_write_reg(sc, CRMFB_VT_HBLANK, d);

	d = mode->vtotal | (mode->vdisplay << 12);
	crmfb_write_reg(sc, CRMFB_VT_VCMAP, d);
	d = mode->htotal | (mode->hdisplay << 12);
	crmfb_write_reg(sc, CRMFB_VT_HCMAP, d);

	d = 0;
	if (mode->flags & VID_NHSYNC) d |= CRMFB_VT_FLAGS_HDRV_INVERT;
	if (mode->flags & VID_NVSYNC) d |= CRMFB_VT_FLAGS_VDRV_INVERT;
	crmfb_write_reg(sc, CRMFB_VT_FLAGS, d);
	sc->sc_vtflags = d;

	diff = -abs(mode->vtotal - mode->vdisplay - 1);
	d = ((uint32_t)diff << 12) & 0x00fff000;
	d |= (mode->htotal - 20);
	crmfb_write_reg(sc, CRMFB_VT_DID_STARTXY, d);

	d = ((uint32_t)(diff + 1) << 12) & 0x00fff000;
	d |= (mode->htotal - 54);
	crmfb_write_reg(sc, CRMFB_VT_CRS_STARTXY, d);

	d = ((uint32_t)diff << 12) & 0x00fff000;
	d |= (mode->htotal - 4);
	crmfb_write_reg(sc, CRMFB_VT_VC_STARTXY, d);

	tmp = mode->htotal - 19;
	d = tmp << 12;
	d |= ((tmp + mode->hdisplay - 2) % mode->htotal);
	crmfb_write_reg(sc, CRMFB_VT_HPIX_EN, d);

	d = mode->vdisplay | (mode->vtotal << 12);
	crmfb_write_reg(sc, CRMFB_VT_VPIX_EN, d);

	sc->sc_width = mode->hdisplay;
	sc->sc_height = mode->vdisplay;

	return TRUE;
}

/*
 * Parse a mode string in the form WIDTHxHEIGHT[@REFRESH] and return
 * monitor timings generated using the VESA GTF formula.
 */
static int
crmfb_parse_mode(const char *modestr, struct videomode *mode)
{
	char *x, *at;
	int width, height, refresh;

	if (modestr == NULL)
		return EINVAL;

	x = strchr(modestr, 'x');
	at = strchr(modestr, '@');

	if (x == NULL || (at != NULL && at < x))
		return EINVAL;

	width = strtoul(modestr, NULL, 10);
	height = strtoul(x + 1, NULL, 10);
	refresh = at ? strtoul(at + 1, NULL, 10) : 60;

	if (width == 0 || height == 0 || refresh == 0)
		return EINVAL;

	vesagtf_mode(width, height, refresh, mode);

	return 0;
}
