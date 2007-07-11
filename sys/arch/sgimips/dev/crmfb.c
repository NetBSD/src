/* $NetBSD: crmfb.c,v 1.5.6.2 2007/07/11 20:01:41 mjf Exp $ */

/*-
 * Copyright (c) 2007 Jared D. McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: crmfb.c,v 1.5.6.2 2007/07/11 20:01:41 mjf Exp $");

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

#define CRMFB_SHADOWFB

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

#define CRMFB_DOTCLOCK		0x00000004
#define		CRMFB_DOTCLOCK_CLKRUN_SHIFT	20
#define CRMFB_VT_XY		0x00010000
#define		CRMFB_VT_XY_FREEZE_SHIFT	31
#define CRMFB_VT_INTR01		0x00010020
#define		CRMFB_VT_INTR01_EN		0xffffffff
#define CRMFB_VT_INTR23		0x00010024
#define		CRMFB_VT_INTR23_EN		0xffffffff
#define CRMFB_VT_VPIX_EN	0x00010038
#define		CRMFB_VT_VPIX_EN_OFF_SHIFT	0
#define CRMFB_VT_VCMAP		0x0001003c
#define		CRMFB_VT_VCMAP_ON_SHIFT		12
#define CRMFB_VT_HCMAP		0x00010040
#define		CRMFB_VT_HCMAP_ON_SHIFT		12
#define CRMFB_OVR_WIDTH_TILE	0x00020000
#define CRMFB_OVR_CONTROL	0x00020008
#define		CRMFB_OVR_CONTROL_DMAEN_SHIFT	0
#define CRMFB_FRM_TILESIZE	0x00030000
#define		CRMFB_FRM_TILESIZE_RHS_SHIFT	0
#define		CRMFB_FRM_TILESIZE_WIDTH_SHIFT	5
#define		CRMFB_FRM_TILESIZE_DEPTH_SHIFT	13
#define			CRMFB_FRM_TILESIZE_DEPTH_8	0
#define			CRMFB_FRM_TILESIZE_DEPTH_16	1
#define			CRMFB_FRM_TILESIZE_DEPTH_32	2
#define		CRMFB_FRM_TILESIZE_FIFOR_SHIFT	15
#define CRMFB_FRM_PIXSIZE	0x00030004
#define		CRMFB_FRM_PIXSIZE_HEIGHT_SHIFT	16
#define CRMFB_FRM_CONTROL	0x0003000c
#define		CRMFB_FRM_CONTROL_DMAEN_SHIFT	0
#define		CRMFB_FRM_CONTROL_LINEAR_SHIFT	1
#define		CRMFB_FRM_CONTROL_TILEPTR_SHIFT	9
#define CRMFB_DID_CONTROL	0x00040004
#define		CRMFB_DID_CONTROL_DMAEN_SHIFT	0
#define CRMFB_MODE		0x00048000
#define		CRMFB_MODE_TYP_SHIFT		2
#define			CRMFB_MODE_TYP_I8	0
#define			CRMFB_MODE_TYP_ARGB5	4
#define			CRMFB_MODE_TYP_RGB8	5
#define		CRMFB_MODE_BUF_SHIFT		0
#define			CRMFB_MODE_BUF_BOTH	3
#define CRMFB_CMAP		0x00050000
#define CRMFB_CMAP_FIFO		0x00058000
#define CRMFB_GMAP		0x00060000
#define CRMFB_CRS_CONTROL	0x00070004

static int	crmfb_match(struct device *, struct cfdata *, void *);
static void	crmfb_attach(struct device *, struct device *, void *);
int		crmfb_probe(void);

#define KERNADDR(p)	((void *)((p).addr))
#define DMAADDR(p)	((p).map->dm_segs[0].ds_addr)

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
	bus_dma_tag_t		sc_dmat;

	struct crmfb_dma	sc_dma;
	struct crmfb_dma	sc_dmai;

	int			sc_width;
	int			sc_height;
	int			sc_depth;
	uint32_t		sc_fbsize;
	uint8_t			*sc_shadowfb;

	int			sc_wsmode;
	u_char			sc_cmap_red[256];
	u_char			sc_cmap_green[256];
	u_char			sc_cmap_blue[256];
};

static int	crmfb_putcmap(struct crmfb_softc *, struct wsdisplay_cmap *);
static int	crmfb_getcmap(struct crmfb_softc *, struct wsdisplay_cmap *);
static void	crmfb_set_palette(struct crmfb_softc *,
				  int, uint8_t, uint8_t, uint8_t);

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

	/* determine mode configured by firmware */
	d = bus_space_read_4(sc->sc_iot, sc->sc_ioh, CRMFB_VT_VCMAP);
	sc->sc_width = (d >> CRMFB_VT_VCMAP_ON_SHIFT) & 0xfff;
	d = bus_space_read_4(sc->sc_iot, sc->sc_ioh, CRMFB_VT_HCMAP);
	sc->sc_height = (d >> CRMFB_VT_HCMAP_ON_SHIFT) & 0xfff;
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

	printf("%s: initial resolution %dx%d %d bpp\n",
	    sc->sc_dev.dv_xname, sc->sc_width, sc->sc_height, sc->sc_depth);

#if 0
	/* Changing the colour depth is easy... */
	sc->sc_depth = 32;
	printf("%s: using resolution %dx%d %d bpp\n",
	    sc->sc_dev.dv_xname, sc->sc_width, sc->sc_height, sc->sc_depth);
#endif

	sc->sc_fbsize = sc->sc_width * sc->sc_height * sc->sc_depth / 8;
	sc->sc_fbsize = (sc->sc_fbsize / CRMFB_TILESIZE) * CRMFB_TILESIZE;

	sc->sc_dmai.size = 128 * sizeof(uint16_t);
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

	sc->sc_dma.size = sc->sc_fbsize;
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
	for (i = 0; i < (sc->sc_fbsize >> 16); i++) {
		p[i] = ((uint32_t)v >> 16) + i;
	}

	memset(KERNADDR(sc->sc_dma), 0x00, sc->sc_fbsize);

	printf("%s: allocated %d byte fb @ %p (%p)\n", sc->sc_dev.dv_xname,
	    sc->sc_fbsize, KERNADDR(sc->sc_dmai), KERNADDR(sc->sc_dma));

#ifdef CRMFB_SHADOWFB
	/* setup shadow framebuffer */
	sc->sc_shadowfb = malloc(sc->sc_fbsize, M_DEVBUF, M_NOWAIT | M_ZERO);
	if (sc->sc_shadowfb == NULL)
		aprint_error("%s: WARNING: couldn't allocate shadow fb\n",
		    sc->sc_dev.dv_xname);
#endif

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

	/* disable DMA */
	d = bus_space_read_4(sc->sc_iot, sc->sc_ioh, CRMFB_OVR_CONTROL);
	d &= ~(1 << CRMFB_OVR_CONTROL_DMAEN_SHIFT);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, CRMFB_OVR_CONTROL, d);
	DELAY(10000);
	d = bus_space_read_4(sc->sc_iot, sc->sc_ioh, CRMFB_FRM_CONTROL);
	d &= ~(1 << CRMFB_FRM_CONTROL_DMAEN_SHIFT);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, CRMFB_FRM_CONTROL, d);
	DELAY(10000);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, CRMFB_DID_CONTROL, 0);
	DELAY(10000);

	/* ensure that CRM starts drawing at the top left of the screen
	 * when we re-enable DMA later
	 */
	d = (1 << CRMFB_VT_XY_FREEZE_SHIFT);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, CRMFB_VT_XY, d);
	d = bus_space_read_4(sc->sc_iot, sc->sc_ioh, CRMFB_DOTCLOCK);
	d &= ~(1 << CRMFB_DOTCLOCK_CLKRUN_SHIFT);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, CRMFB_DOTCLOCK, d);

	/* reset FIFO */
	d = bus_space_read_4(sc->sc_iot, sc->sc_ioh, CRMFB_FRM_TILESIZE);
	d |= (1 << CRMFB_FRM_TILESIZE_FIFOR_SHIFT);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, CRMFB_FRM_TILESIZE, d);
	d &= ~(1 << CRMFB_FRM_TILESIZE_FIFOR_SHIFT);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, CRMFB_FRM_TILESIZE, d);

	/* setup colour mode */
	switch (sc->sc_depth) {
	case 8:
		h = CRMFB_MODE_TYP_I8;
		break;
	case 16:
		h = CRMFB_MODE_TYP_ARGB5;
		break;
	case 32:
		h = CRMFB_MODE_TYP_RGB8;
		break;
	default:
		panic("Unsupported depth");
	}
	d = h << CRMFB_MODE_TYP_SHIFT;
	d |= CRMFB_MODE_BUF_BOTH << CRMFB_MODE_BUF_SHIFT;
	for (i = 0; i < (32 * 4); i += 4)
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, CRMFB_MODE + i, d);

	/* setup tile pointer, but don't turn on DMA yet! */
	h = DMAADDR(sc->sc_dmai);
	d = (h >> 9) << CRMFB_FRM_CONTROL_TILEPTR_SHIFT;
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, CRMFB_FRM_CONTROL, d);

	/* init framebuffer width and pixel size */
	d = (1 << CRMFB_FRM_TILESIZE_WIDTH_SHIFT);
	switch (sc->sc_depth) {
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
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, CRMFB_FRM_TILESIZE, d);

	/* init framebuffer height, we use the trick that the Linux
	 * driver uses to fool the CRM out of tiled mode and into
	 * linear mode
	 */
	h = sc->sc_width * sc->sc_height / (512 / (sc->sc_depth / 8));
	d = h << CRMFB_FRM_PIXSIZE_HEIGHT_SHIFT;
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, CRMFB_FRM_PIXSIZE, d);

	/* turn off firmware overlay and hardware cursor */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, CRMFB_OVR_WIDTH_TILE, 0);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, CRMFB_CRS_CONTROL, 0);

	/* enable drawing again */
	d = bus_space_read_4(sc->sc_iot, sc->sc_ioh, CRMFB_DOTCLOCK);
	d |= (1 << CRMFB_DOTCLOCK_CLKRUN_SHIFT);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, CRMFB_DOTCLOCK, d);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, CRMFB_VT_XY, 0);

	/* turn on DMA for the framebuffer */
	d = bus_space_read_4(sc->sc_iot, sc->sc_ioh, CRMFB_FRM_CONTROL);
	d |= (1 << CRMFB_FRM_CONTROL_DMAEN_SHIFT);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, CRMFB_FRM_CONTROL, d);

	for (i = 0; i < 256; i++) {
		crmfb_set_palette(sc, i, rasops_cmap[(i * 3) + 2],
		    rasops_cmap[(i * 3) + 1], rasops_cmap[(i * 3) + 0]);
		sc->sc_cmap_red[i] = rasops_cmap[(i * 3) + 2];
		sc->sc_cmap_green[i] = rasops_cmap[(i * 3) + 1];
		sc->sc_cmap_blue[i] = rasops_cmap[(i * 3) + 0];
	}

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
		*(u_int *)data = WSDISPLAY_TYPE_NEWPORT;
		return 0;
	case WSDISPLAYIO_GINFO:
		if (vd->active != NULL) {
			wdf = (void *)data;
			wdf->height = sc->sc_height;
			wdf->width = sc->sc_width;
			wdf->depth = sc->sc_depth;
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
			if (nmode == WSDISPLAYIO_MODE_EMUL)
				vcons_redraw_screen(vd->active);
		}
		return 0;
	case WSDISPLAYIO_SVIDEO:
	case WSDISPLAYIO_GVIDEO:
		return ENODEV;	/* not supported yet */
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

	if (offset >= 0 && offset < sc->sc_fbsize) {
		pa = bus_dmamem_mmap(sc->sc_dmat, sc->sc_dma.segs,
		    sc->sc_dma.nsegs, offset, prot,
		    BUS_DMA_WAITOK | BUS_DMA_COHERENT);
		return pa;
	}

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

	ri->ri_flg = RI_CENTER;
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

	if (sc->sc_shadowfb == NULL)
		ri->ri_bits = KERNADDR(sc->sc_dma);
	else {
		ri->ri_bits = sc->sc_shadowfb;
		ri->ri_hwbits = KERNADDR(sc->sc_dma);
	}

	if (existing)
		ri->ri_flg |= RI_CLEAR;

	rasops_init(ri, ri->ri_height / 16, ri->ri_width / 8);
	ri->ri_caps = WSSCREEN_WSCOLORS;
	rasops_reconfig(ri, ri->ri_height / ri->ri_font->fontheight,
	    ri->ri_width / ri->ri_font->fontwidth);
	ri->ri_hw = scr;

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
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, CRMFB_CMAP + (reg * 4), val);

	return;
}
