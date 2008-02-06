/* $NetBSD: crmfb.c,v 1.13 2008/02/06 01:33:38 macallan Exp $ */

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by Jared D. McNeill.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: crmfb.c,v 1.13 2008/02/06 01:33:38 macallan Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#define _SGIMIPS_BUS_DMA_PRIVATE
#include <machine/autoconf.h>
#include <machine/bus.h>
#include <machine/machtype.h>
#include <machine/vmparam.h>

#include <dev/arcbios/arcbios.h>
#include <dev/arcbios/arcbiosvar.h>

#include <dev/wscons/wsdisplayvar.h>
#include <dev/wscons/wsconsio.h>
#include <dev/wsfont/wsfont.h>
#include <dev/rasops/rasops.h>
#include <dev/wscons/wsdisplay_vconsvar.h>

#include <arch/sgimips/dev/crmfbreg.h>

/* #define CRMFB_DEBUG */

struct wsscreen_descr crmfb_defaultscreen = {
	"default",
	0, 0,
	NULL,
	8, 16,
	WSSCREEN_WSCOLORS,
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

static int	crmfb_match(struct device *, struct cfdata *, void *);
static void	crmfb_attach(struct device *, struct device *, void *);
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
	struct device		sc_dev;
	struct vcons_data	sc_vd;

	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	bus_space_handle_t	sc_reh;

	bus_dma_tag_t		sc_dmat;

	struct crmfb_dma	sc_dma;
	struct crmfb_dma	sc_dmai;

	int			sc_width;
	int			sc_height;
	int			sc_depth;
	int			sc_tiles_x, sc_tiles_y;
	uint32_t		sc_fbsize;
	uint8_t			*sc_scratch;
	struct rasops_info	sc_rasops;
	int 			sc_cells;
	int			sc_current_cell;
	int			sc_wsmode;

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

#ifdef CRMFB_DEBUG
void crmfb_test_mte(struct crmfb_softc *);
#endif

static void crmfb_fill_rect(struct crmfb_softc *, int, int, int, int, uint32_t);
static void crmfb_bitblt(struct crmfb_softc *, int, int, int, int, int, int,
			 uint32_t);

static void	crmfb_copycols(void *, int, int, int, int);
static void	crmfb_erasecols(void *, int, int, int, long);
static void	crmfb_copyrows(void *, int, int, int);
static void	crmfb_eraserows(void *, int, int, long);
static void	crmfb_cursor(void *, int, int, int);
static void	crmfb_putchar(void *, int, int, u_int, long);

CFATTACH_DECL(crmfb, sizeof(struct crmfb_softc),
    crmfb_match, crmfb_attach, NULL, NULL);

static int
crmfb_match(struct device *parent, struct cfdata *cf, void *opaque)
{
	return crmfb_probe();
}

static void
crmfb_attach(struct device *parent, struct device *self, void *opaque)
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
	int rv, i;

	sc = (struct crmfb_softc *)self;
	ma = (struct mainbus_attach_args *)opaque;

	sc->sc_iot = SGIMIPS_BUS_SPACE_CRIME;
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
		printf("%s: device unusable if not setup by firmware\n",
		    sc->sc_dev.dv_xname);
		bus_space_unmap(sc->sc_iot, sc->sc_ioh, 0 /* XXX */);
		return;
	}

	printf("%s: initial resolution %dx%d\n",
	    sc->sc_dev.dv_xname, sc->sc_width, sc->sc_height);

	/*
	 * first determine how many tiles we need
	 * in 32bit each tile is 128x128 pixels
	 */
	sc->sc_tiles_x = (sc->sc_width + 127) >> 7;
	sc->sc_tiles_y = (sc->sc_height + 127) >> 7;
	sc->sc_fbsize = 0x10000 * sc->sc_tiles_x * sc->sc_tiles_y;
	printf("so we need %d x %d tiles -> %08x\n", sc->sc_tiles_x,
	    sc->sc_tiles_y, sc->sc_fbsize);

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

	/* allocate an extra tile for character drawing */
	sc->sc_dma.size = sc->sc_fbsize + 0x10000;
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
	sc->sc_scratch = (char *)KERNADDR(sc->sc_dma) + sc->sc_fbsize;

	printf("%s: allocated %d byte fb @ %p (%p)\n", sc->sc_dev.dv_xname,
	    sc->sc_fbsize, KERNADDR(sc->sc_dmai), KERNADDR(sc->sc_dma));

	sc->sc_current_cell = 0;

	ri = &crmfb_console_screen.scr_ri;
	memset(ri, 0, sizeof(struct rasops_info));

	vcons_init(&sc->sc_vd, sc, &crmfb_defaultscreen, &crmfb_accessops);
	sc->sc_vd.init_screen = crmfb_init_screen;
	crmfb_console_screen.scr_flags |= VCONS_SCREEN_IS_STATIC;
	vcons_init_screen(&sc->sc_vd, &crmfb_console_screen, 1, &defattr);

	crmfb_defaultscreen.ncols = ri->ri_cols;
	crmfb_defaultscreen.nrows = ri->ri_rows;
	crmfb_defaultscreen.textops = &ri->ri_ops;
	crmfb_defaultscreen.capabilities = ri->ri_caps;
	crmfb_defaultscreen.modecookie = NULL;

	crmfb_setup_video(sc, 8);
	crmfb_fill_rect(sc, 0, 0, sc->sc_width, sc->sc_height, 0xf);

	crmfb_setup_palette(sc);

	consdev = ARCBIOS->GetEnvironmentVariable("ConsoleOut");
	if (consdev != NULL && strcmp(consdev, "video()") == 0) {
		wsdisplay_cnattach(&crmfb_defaultscreen, ri, 0, 0, defattr);
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

#ifdef CRMFB_DEBUG
	crmfb_test_mte(sc);
#endif
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
	struct vcons_screen *ms;
	struct wsdisplay_fbinfo *wdf;
	int nmode;

	vd = (struct vcons_data *)v;
	sc = (struct crmfb_softc *)vd->cookie;
	ms = (struct vcons_screen *)vd->active;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		/* not really, but who cares? */
		/* wsfb does */
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
				crmfb_setup_video(sc, 8);
				crmfb_setup_palette(sc);
				vcons_redraw_screen(vd->active);
			} else {
				crmfb_setup_video(sc, 32);
			}
		}
		return 0;
	case WSDISPLAYIO_SVIDEO:
	case WSDISPLAYIO_GVIDEO:
		return ENODEV;	/* not supported yet */

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

#if 1
	if (offset >= 0 && offset < sc->sc_fbsize) {
		pa = bus_dmamem_mmap(sc->sc_dmat, sc->sc_dma.segs,
		    sc->sc_dma.nsegs, offset, prot,
		    BUS_DMA_WAITOK | BUS_DMA_COHERENT);
		return pa;
	}
#else
	if (offset >= 0 && offset < sc->sc_fbsize) {
		pa = bus_space_mmap(SGIMIPS_BUS_SPACE_NORMAL, sc->sc_fbh,
		    offset, prot, SGIMIPS_BUS_SPACE_NORMAL);
		printf("%s: %08llx -> %llx\n", __func__, offset, pa);
		return pa;
	}
#endif
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

	ri->ri_flg = RI_CENTER | RI_FULLCLEAR;
	ri->ri_depth = sc->sc_depth;
	ri->ri_width = sc->sc_width;
	ri->ri_height = sc->sc_height;
	ri->ri_stride = ri->ri_width * (ri->ri_depth / 8);

	switch (ri->ri_depth) {
	case 16:
		ri->ri_rnum = ri->ri_gnum = ri->ri_bnum = 5;
		ri->ri_rpos = 10;
		ri->ri_gpos = 5;
		ri->ri_bpos = 0;
		break;
	case 32:
		ri->ri_rnum = ri->ri_gnum = ri->ri_bnum = 8;
		ri->ri_rpos = 8;
		ri->ri_gpos = 16;
		ri->ri_bpos = 24;
		break;
	}

	ri->ri_bits = KERNADDR(sc->sc_dma);

	if (existing)
		ri->ri_flg |= RI_CLEAR;

	rasops_init(ri, ri->ri_height / 16, ri->ri_width / 8);
	ri->ri_caps = WSSCREEN_WSCOLORS;
	rasops_reconfig(ri, ri->ri_height / ri->ri_font->fontheight,
	    ri->ri_width / ri->ri_font->fontwidth);
	ri->ri_hw = scr;

	/* now make a fake rasops_info for drawing into the scratch tile */
	memcpy(&sc->sc_rasops, ri, sizeof(struct rasops_info));
	sc->sc_rasops.ri_width = 512;	/* assume we're always in 8bit here */
	sc->sc_rasops.ri_stride = 512;
	sc->sc_rasops.ri_height = 128;
	sc->sc_rasops.ri_xorigin = 0;
	sc->sc_rasops.ri_yorigin = 0;
	sc->sc_rasops.ri_bits = sc->sc_scratch;
	sc->sc_cells = 512 / ri->ri_font->fontwidth;

	ri->ri_ops.cursor    = crmfb_cursor;
	ri->ri_ops.copyrows  = crmfb_copyrows;
	ri->ri_ops.eraserows = crmfb_eraserows;
	ri->ri_ops.copycols  = crmfb_copycols;
	ri->ri_ops.erasecols = crmfb_erasecols;
	ri->ri_ops.putchar   = crmfb_putchar;

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

	val = (r << 24) | (g << 16) | (b << 8);
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
	uint32_t d, h, mode;
	int i, bail, tile_width, tlbptr, j, tx, shift;
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
	crmfb_write_reg(sc, CRMFB_DID_CONTROL, 0);
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
		h = CRMFB_MODE_TYP_I8;
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
	if ((sc->sc_width & (tile_width - 1)) != 0)
		d |= sc->sc_tiles_y;

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

	/* init framebuffer height, we use the trick that the Linux
	 * driver uses to fool the CRM out of tiled mode and into
	 * linear mode
	 */
	/*h = sc->sc_width * sc->sc_height / (512 / (depth >> 3));*/
	h = sc->sc_height;
	d = h << CRMFB_FRM_PIXSIZE_HEIGHT_SHIFT;
	crmfb_write_reg(sc, CRMFB_FRM_PIXSIZE, d);

	/* turn off firmware overlay and hardware cursor */
	crmfb_write_reg(sc, CRMFB_OVR_WIDTH_TILE, 0);
	crmfb_write_reg(sc, CRMFB_CURSOR_CONTROL, 0);

	/* enable drawing again */
	d = bus_space_read_4(sc->sc_iot, sc->sc_ioh, CRMFB_DOTCLOCK);
	d |= (1 << CRMFB_DOTCLOCK_CLKRUN_SHIFT);
	crmfb_write_reg(sc, CRMFB_DOTCLOCK, d);
	crmfb_write_reg(sc, CRMFB_VT_XY, 0);

	/* turn on DMA for the framebuffer */
	d = bus_space_read_4(sc->sc_iot, sc->sc_ioh, CRMFB_FRM_CONTROL);
	d |= (1 << CRMFB_FRM_CONTROL_DMAEN_SHIFT);
	crmfb_write_reg(sc, CRMFB_FRM_CONTROL, d);

	/* turn off sync-on-green */

	wantsync = ARCBIOS->GetEnvironmentVariable("SyncOnGreen");
	if ( (wantsync != NULL) && (wantsync[0] == 'n') ) {
		d = ( 1 << CRMFB_VT_FLAGS_SYNC_LOW_LSB) & CRMFB_REG_MASK(CRMFB_VT_FLAGS_SYNC_LOW_MSB, CRMFB_VT_FLAGS_SYNC_LOW_LSB);
		crmfb_write_reg(sc, CRMFB_VT_FLAGS, d);
	}

	sc->sc_depth = depth;

	/* finally set up the drawing engine's TLB A */
	v = (DMAADDR(sc->sc_dma) >> 16) & 0xffff;
	tlbptr = 0;
	tx = ((sc->sc_width + (tile_width - 1)) & ~(tile_width - 1)) / tile_width;
	for (i = 0; i < sc->sc_tiles_y; i++) {
		reg = 0;
		shift = 64;
		for (j = 0; j < tx; j++) {
			shift -= 16;
			reg |= (((uint64_t)(v | 0x8000)) << shift);
			if (shift == 0) {
				shift = 64;
				bus_space_write_8(sc->sc_iot, sc->sc_reh,
				    CRIME_RE_TLB_A + tlbptr + (j & 0xffc) * 2, reg);
			}
			v++;
		}
		if (shift != 64) {
			bus_space_write_8(sc->sc_iot, sc->sc_reh,
			    CRIME_RE_TLB_A + tlbptr + (j & 0xffc) * 2, reg);
		}
		tlbptr += 32;
	}

	/* put the scratch page into the lower left of the fb */
	reg = (((uint64_t)(DMAADDR(sc->sc_dma) + sc->sc_fbsize) | 0x80000000) &
	    0xffff0000) << 32;
	bus_space_write_8(sc->sc_iot, sc->sc_reh, CRIME_RE_TLB_A + 0x1e0, reg);

	wbflush();

#ifdef CRMFB_DEBUG
	for (i = 0; i < 0x80; i += 8)
		printf("%04x: %016llx\n", i, bus_space_read_8(sc->sc_iot, sc->sc_reh,
			    CRIME_RE_TLB_A + i));
#endif
	/* do some very basic engine setup */
	bus_space_write_4(sc->sc_iot, sc->sc_reh, CRIME_DE_CLIPMODE, 0);
	bus_space_write_4(sc->sc_iot, sc->sc_reh, CRIME_DE_WINOFFSET_SRC, 0);
	bus_space_write_4(sc->sc_iot, sc->sc_reh, CRIME_DE_WINOFFSET_DST, 0);
	bus_space_write_4(sc->sc_iot, sc->sc_reh, CRIME_DE_PLANEMASK, 0xffffffff);

	bus_space_write_8(sc->sc_iot, sc->sc_reh, 0x20, 0);
	bus_space_write_8(sc->sc_iot, sc->sc_reh, 0x28, 0);
	bus_space_write_8(sc->sc_iot, sc->sc_reh, 0x30, 0);
	bus_space_write_8(sc->sc_iot, sc->sc_reh, 0x38, 0);
	bus_space_write_8(sc->sc_iot, sc->sc_reh, 0x40, 0);
	
	switch (depth) {
		case 8:
			mode = DE_MODE_TLB_A | DE_MODE_BUFDEPTH_8 |
			    DE_MODE_TYPE_CI | DE_MODE_PIXDEPTH_8;
			break;
		case 16:
			mode = DE_MODE_TLB_A | DE_MODE_BUFDEPTH_16 |
			    DE_MODE_TYPE_RGB | DE_MODE_PIXDEPTH_16;
		case 32:
			mode = DE_MODE_TLB_A | DE_MODE_BUFDEPTH_32 |
			    DE_MODE_TYPE_RGBA | DE_MODE_PIXDEPTH_32;
		default:
			panic("%s: unsuported colour depth %d\n", __func__,
			    depth);
	}
	bus_space_write_4(sc->sc_iot, sc->sc_reh, CRIME_DE_MODE_DST, mode);
	bus_space_write_4(sc->sc_iot, sc->sc_reh, CRIME_DE_MODE_SRC, mode);
	bus_space_write_4(sc->sc_iot, sc->sc_reh, CRIME_DE_XFER_STEP_X, 1);
	bus_space_write_4(sc->sc_iot, sc->sc_reh, CRIME_DE_XFER_STEP_Y, 1);
	return 0;
}

static void
crmfb_setup_palette(struct crmfb_softc *sc)
{
	int i;

	for (i = 0; i < 256; i++) {
		crmfb_set_palette(sc, i, rasops_cmap[(i * 3) + 2],
		    rasops_cmap[(i * 3) + 1], rasops_cmap[(i * 3) + 0]);
		sc->sc_cmap_red[i] = rasops_cmap[(i * 3) + 2];
		sc->sc_cmap_green[i] = rasops_cmap[(i * 3) + 1];
		sc->sc_cmap_blue[i] = rasops_cmap[(i * 3) + 0];
	}
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
}

static void
crmfb_fill_rect(struct crmfb_softc *sc, int x, int y, int width, int height,
    uint32_t colour)
{
	crmfb_wait_idle(sc);
	bus_space_write_4(sc->sc_iot, sc->sc_reh, CRIME_DE_DRAWMODE,
	    DE_DRAWMODE_PLANEMASK | DE_DRAWMODE_BYTEMASK);
	bus_space_write_4(sc->sc_iot, sc->sc_reh, CRIME_DE_FG, colour);
	bus_space_write_4(sc->sc_iot, sc->sc_reh, CRIME_DE_PRIMITIVE,
		DE_PRIM_RECTANGLE | DE_PRIM_TB);
	bus_space_write_4(sc->sc_iot, sc->sc_reh, CRIME_DE_X_VERTEX_0,
	    (x << 16) | (y & 0xffff));
	bus_space_write_4(sc->sc_iot, sc->sc_reh,
	    CRIME_DE_X_VERTEX_1 | CRIME_DE_START,
	    ((x + width - 1) << 16) | ((y + height - 1) & 0xffff));
}

static void
crmfb_bitblt(struct crmfb_softc *sc, int xs, int ys, int xd, int yd,
    int wi, int he, uint32_t rop)
{
	uint32_t prim = DE_PRIM_RECTANGLE;
	int rxa, rya, rxe, rye, rxs, rys;
	crmfb_wait_idle(sc);
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

#ifdef CRMFB_DEBUG
void
crmfb_test_mte(struct crmfb_softc *sc)
{
	uint32_t status[10];
	int i;

	crmfb_fill_rect(sc, 1, 1, 100, 100, 1);
	crmfb_fill_rect(sc, 102, 1, 100, 100, 2);
	crmfb_fill_rect(sc, 203, 1, 100, 100, 3);
	crmfb_fill_rect(sc, 1024, 1, 100, 100, 4);

	crmfb_bitblt(sc, 10, 10, 400, 0, 300, 300, 12);
	crmfb_wait_idle(sc);
	printf("ROP: %08x\n", bus_space_read_4(sc->sc_iot, sc->sc_reh, CRIME_DE_ROP));
#if 0
	delay(4000000);

	bus_space_write_4(sc->sc_iot, sc->sc_reh, CRIME_MTE_NULL, 0x0);
	bus_space_write_4(sc->sc_iot, sc->sc_reh, CRIME_MTE_BG, 0x05050505);
	bus_space_write_4(sc->sc_iot, sc->sc_reh, CRIME_MTE_MODE,
	    MTE_MODE_DST_ECC |
	    (MTE_TLB_A << MTE_DST_TLB_SHIFT) |
	    (MTE_TLB_A << MTE_SRC_TLB_SHIFT) |
	    (MTE_DEPTH_8 << MTE_DEPTH_SHIFT) |
	    0/*MTE_MODE_COPY*/);
	bus_space_write_4(sc->sc_iot, sc->sc_reh, CRIME_MTE_DST_STRIDE, 1);
	bus_space_write_4(sc->sc_iot, sc->sc_reh, CRIME_MTE_SRC_STRIDE, 1);
	bus_space_write_4(sc->sc_iot, sc->sc_reh, CRIME_MTE_SRC0, 0x00000000);
	bus_space_write_4(sc->sc_iot, sc->sc_reh, CRIME_MTE_SRC1, 0x02000200);
	bus_space_write_4(sc->sc_iot, sc->sc_reh, CRIME_MTE_DST0, 0x03000000);
	bus_space_write_4(sc->sc_iot, sc->sc_reh, CRIME_MTE_DST1 | CRIME_DE_START, 0x02000200);
	status[9] = bus_space_read_4(sc->sc_iot, sc->sc_reh, CRIME_DE_STATUS);
#endif	
#if 1
	bus_space_write_4(sc->sc_iot, sc->sc_reh, CRIME_MTE_MODE,
	    MTE_MODE_DST_ECC |
	    (MTE_TLB_A << MTE_DST_TLB_SHIFT) |
	    (MTE_TLB_A << MTE_SRC_TLB_SHIFT) |
	    (MTE_DEPTH_8 << MTE_DEPTH_SHIFT) |
	    0/*MTE_MODE_COPY*/);
#endif
	delay(4000000);
#if 0
	printf("flush: %08x\n",
	    bus_space_read_4(sc->sc_iot, sc->sc_reh, CRIME_DE_FLUSH));
#endif

	for (i = 0; i < 10; i++)
		printf("%08x ", status[i]);
	printf("\n");
}
#endif /* CRMFB_DEBUG */

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
	crmfb_bitblt(scr->scr_cookie, x, ys, x, yd, width, height, 3);
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
	struct rasops_info *fri = &sc->sc_rasops;
	int bg;
	int x, y, wi, he, xs;

	wi = ri->ri_font->fontwidth;
	he = ri->ri_font->fontheight;

	x = ri->ri_xorigin + col * wi;
	y = ri->ri_yorigin + row * he;

	bg = (u_char)ri->ri_devcmap[(attr >> 16) & 0xff];
	if (c == 0x20) {
		crmfb_fill_rect(sc, x, y, wi, he, bg);
	} else {
		/*
		 * we rotate over all available character cells in the scratch
		 * tile. The idea is to have more cells than there's room for
		 * drawing commands in the engine's pipeline so we don't have
		 * to wait for the engine until we're done drawing the 
		 * character and ready to blit it into place
		 */
		fri->ri_ops.putchar(fri, 0, sc->sc_current_cell, c, attr);
		xs = sc->sc_current_cell * wi;
		sc->sc_current_cell++;
		if (sc->sc_current_cell >= sc->sc_cells)
			sc->sc_current_cell = 0;
		crmfb_bitblt(sc, xs, 2048-128, x, y, wi, he, 3);
	}
}
