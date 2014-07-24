/* $NetBSD: lunafb.c,v 1.34 2014/07/24 14:09:09 tsutsui Exp $ */

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tohru Nishimura.
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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: lunafb.c,v 1.34 2014/07/24 14:09:09 tsutsui Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/kmem.h>
#include <sys/mman.h>
#include <sys/proc.h>
#include <sys/tty.h>
#include <sys/errno.h>
#include <sys/buf.h>

#include <uvm/uvm_extern.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/rasops/rasops.h>

#include <machine/cpu.h>
#include <machine/autoconf.h>

#include <arch/luna68k/dev/omrasopsvar.h>

#include "ioconf.h"

struct bt454 {
	volatile uint8_t bt_addr;	/* map address register */
	volatile uint8_t bt_cmap;	/* colormap data register */
};

struct bt458 {
	volatile uint8_t bt_addr;	/* map address register */
	uint8_t          pad0[3];
	volatile uint8_t bt_cmap;	/* colormap data register */
	uint8_t          pad1[3];
	volatile uint8_t bt_ctrl;	/* control register */
	uint8_t          pad2[3];
	volatile uint8_t bt_omap;	/* overlay (cursor) map register */
	uint8_t          pad3[3];
};

#define	OMFB_RFCNT	0xB1000000	/* video h-origin/v-origin */
#define	OMFB_PLANEMASK	0xB1040000	/* planemask register */
#define	OMFB_FB_WADDR	0xB1080008	/* common plane */
#define	OMFB_FB_RADDR	0xB10C0008	/* plane #0 */
#define	OMFB_ROPFUNC	0xB12C0000	/* ROP function code */
#define	OMFB_RAMDAC	0xC1100000	/* Bt454/Bt458 RAMDAC */
#define	OMFB_SIZE	(0xB1300000 - 0xB1080000 + PAGE_SIZE)

struct hwcmap {
#define CMAP_SIZE 256
	uint8_t r[CMAP_SIZE];
	uint8_t g[CMAP_SIZE];
	uint8_t b[CMAP_SIZE];
};

static const struct {
	uint8_t r;
	uint8_t g;
	uint8_t b;
} ansicmap[16] = {
	{    0,    0,    0},
	{ 0x80,    0,    0},
	{    0, 0x80,    0},
	{ 0x80, 0x80,    0},
	{    0,    0, 0x80},
	{ 0x80,    0, 0x80},
	{    0, 0x80, 0x80},
	{ 0xc0, 0xc0, 0xc0},
	{ 0x80, 0x80, 0x80},
	{ 0xff,    0,    0},
	{    0, 0xff,    0},
	{ 0xff, 0xff,    0},
	{    0,    0, 0xff},
	{ 0xff,    0, 0xff},
	{    0, 0xff, 0xff},
	{ 0xff, 0xff, 0xff},
};

struct om_hwdevconfig {
	int	dc_wid;			/* width of frame buffer */
	int	dc_ht;			/* height of frame buffer */
	int	dc_depth;		/* depth, bits per pixel */
	int	dc_rowbytes;		/* bytes in a FB scan line */
	int	dc_cmsize;		/* colormap size */
	struct hwcmap dc_cmap;		/* software copy of colormap */
	vaddr_t	dc_videobase;		/* base of flat frame buffer */
	struct rasops_info dc_ri;	/* raster blitter variables */
};

struct omfb_softc {
	device_t sc_dev;		/* base device */
	struct om_hwdevconfig *sc_dc;	/* device configuration */
	int sc_nscreens;
};

static int  omgetcmap(struct omfb_softc *, struct wsdisplay_cmap *);
static int  omsetcmap(struct omfb_softc *, struct wsdisplay_cmap *);

static struct om_hwdevconfig omfb_console_dc;
static void omfb_getdevconfig(paddr_t, struct om_hwdevconfig *);

static struct wsscreen_descr omfb_stdscreen = {
	.name = "std"
};

static const struct wsscreen_descr *_omfb_scrlist[] = {
	&omfb_stdscreen,
};

static const struct wsscreen_list omfb_screenlist = {
	sizeof(_omfb_scrlist) / sizeof(struct wsscreen_descr *), _omfb_scrlist
};

static int   omfbioctl(void *, void *, u_long, void *, int, struct lwp *);
static paddr_t omfbmmap(void *, void *, off_t, int);
static int   omfb_alloc_screen(void *, const struct wsscreen_descr *,
			       void **, int *, int *, long *);
static void  omfb_free_screen(void *, void *);
static int   omfb_show_screen(void *, void *, int,
			      void (*) (void *, int, int), void *);

static const struct wsdisplay_accessops omfb_accessops = {
	.ioctl        = omfbioctl,
	.mmap         = omfbmmap,
	.alloc_screen = omfb_alloc_screen,
	.free_screen  = omfb_free_screen,
	.show_screen  = omfb_show_screen,
	.load_font    = NULL,
	.pollc        = NULL,
	.scroll       = NULL
};

static int  omfbmatch(device_t, cfdata_t, void *);
static void omfbattach(device_t, device_t, void *);

CFATTACH_DECL_NEW(fb, sizeof(struct omfb_softc),
    omfbmatch, omfbattach, NULL, NULL);

extern int hwplanemask;	/* hardware planemask; retrieved at boot */

static int omfb_console;
int  omfb_cnattach(void);

static int
omfbmatch(device_t parent, cfdata_t cf, void *aux)
{
	struct mainbus_attach_args *ma = aux;

	if (strcmp(ma->ma_name, fb_cd.cd_name))
		return 0;
#if 0	/* XXX badaddr() bombs if no framebuffer is installed */
	if (badaddr((void *)ma->ma_addr, 4))
		return 0;
#else
	if (hwplanemask == 0)
		return 0;
#endif
	return 1;
}

static void
omfbattach(device_t parent, device_t self, void *args)
{
	struct omfb_softc *sc = device_private(self);
	struct wsemuldisplaydev_attach_args waa;

	sc->sc_dev = self;

	if (omfb_console) {
		sc->sc_dc = &omfb_console_dc;
		sc->sc_nscreens = 1;
	} else {
		sc->sc_dc = kmem_zalloc(sizeof(struct om_hwdevconfig),
		    KM_SLEEP);
		omfb_getdevconfig(OMFB_FB_WADDR, sc->sc_dc);
	}
	aprint_normal(": %d x %d, %dbpp\n", sc->sc_dc->dc_wid, sc->sc_dc->dc_ht,
	    sc->sc_dc->dc_depth);

	waa.console = omfb_console;
	waa.scrdata = &omfb_screenlist;
	waa.accessops = &omfb_accessops;
	waa.accesscookie = sc;

	config_found(self, &waa, wsemuldisplaydevprint);
}

/* EXPORT */ int
omfb_cnattach(void)
{
	struct om_hwdevconfig *dc = &omfb_console_dc;
	struct rasops_info *ri = &dc->dc_ri;
	long defattr;

	omfb_getdevconfig(OMFB_FB_WADDR, dc);
	(*ri->ri_ops.allocattr)(ri, 0, 0, 0, &defattr);
	wsdisplay_cnattach(&omfb_stdscreen, ri, 0, 0, defattr);
	omfb_console = 1;
	return 0;
}

static int
omfbioctl(void *v, void *vs, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct omfb_softc *sc = v;
	struct om_hwdevconfig *dc = sc->sc_dc;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = WSDISPLAY_TYPE_LUNA;
		return 0;

	case WSDISPLAYIO_GINFO:
#define	wsd_fbip ((struct wsdisplay_fbinfo *)data)
		wsd_fbip->height = dc->dc_ht;
		wsd_fbip->width = dc->dc_wid;
		wsd_fbip->depth = dc->dc_depth;
		wsd_fbip->cmsize = dc->dc_cmsize;
#undef fbt
		return 0;

	case WSDISPLAYIO_LINEBYTES:
		*(u_int *)data = dc->dc_rowbytes;
		return 0;

	case WSDISPLAYIO_GETCMAP:
		return omgetcmap(sc, (struct wsdisplay_cmap *)data);

	case WSDISPLAYIO_PUTCMAP:
		return omsetcmap(sc, (struct wsdisplay_cmap *)data);

	case WSDISPLAYIO_SVIDEO:
	case WSDISPLAYIO_GVIDEO:
	case WSDISPLAYIO_GCURPOS:
	case WSDISPLAYIO_SCURPOS:
	case WSDISPLAYIO_GCURMAX:
	case WSDISPLAYIO_GCURSOR:
	case WSDISPLAYIO_SCURSOR:
		break;
	}
	return EPASSTHROUGH;
}

/*
 * Return the address that would map the given device at the given
 * offset, allowing for the given protection, or return -1 for error.
 */
static paddr_t
omfbmmap(void *v, void *vs, off_t offset, int prot)
{
	struct omfb_softc *sc = v;
	struct om_hwdevconfig *dc = sc->sc_dc;
	paddr_t cookie = -1;

#if 0	/* XXX: quick workaround to make X.Org mono server work */
	if (offset >= 0 && offset < OMFB_SIZE)
		cookie = m68k_btop(m68k_trunc_page(dc->dc_videobase) + offset);
#else
	if (offset >= 0 && offset < dc->dc_rowbytes * dc->dc_ht * dc->dc_depth)
		cookie = m68k_btop(m68k_trunc_page(OMFB_FB_RADDR) + offset);
#endif

	return cookie;
}

static int
omgetcmap(struct omfb_softc *sc, struct wsdisplay_cmap *p)
{
	u_int index = p->index, count = p->count;
	int cmsize, error;

	cmsize = sc->sc_dc->dc_cmsize;
	if (index >= cmsize || count > cmsize - index)
		return EINVAL;

	error = copyout(&sc->sc_dc->dc_cmap.r[index], p->red, count);
	if (error)
		return error;
	error = copyout(&sc->sc_dc->dc_cmap.g[index], p->green, count);
	if (error)
		return error;
	error = copyout(&sc->sc_dc->dc_cmap.b[index], p->blue, count);
	return error;
}

static int
omsetcmap(struct omfb_softc *sc, struct wsdisplay_cmap *p)
{
	struct hwcmap cmap;
	u_int index = p->index, count = p->count;
	int cmsize, i, error;

	cmsize = sc->sc_dc->dc_cmsize;
	if (index >= cmsize || (index + count) > cmsize)
		return (EINVAL);

	error = copyin(p->red, &cmap.r[index], count);
	if (error)
		return error;
	error = copyin(p->green, &cmap.g[index], count);
	if (error)
		return error;
	error = copyin(p->blue, &cmap.b[index], count);
	if (error)
		return error;

	memcpy(&sc->sc_dc->dc_cmap.r[index], &cmap.r[index], count);
	memcpy(&sc->sc_dc->dc_cmap.g[index], &cmap.g[index], count);
	memcpy(&sc->sc_dc->dc_cmap.b[index], &cmap.b[index], count);
	if (hwplanemask == 0x0f) {
		struct bt454 *odac = (struct bt454 *)OMFB_RAMDAC;
		odac->bt_addr = index;
		for (i = index; i < index + count; i++) {
			odac->bt_cmap = sc->sc_dc->dc_cmap.r[i];
			odac->bt_cmap = sc->sc_dc->dc_cmap.g[i];
			odac->bt_cmap = sc->sc_dc->dc_cmap.b[i];
		}
	} else if (hwplanemask == 0xff) {
		struct bt458 *ndac = (struct bt458 *)OMFB_RAMDAC;
		ndac->bt_addr = index;
		for (i = index; i < index + count; i++) {
			ndac->bt_cmap = sc->sc_dc->dc_cmap.r[i];
			ndac->bt_cmap = sc->sc_dc->dc_cmap.g[i];
			ndac->bt_cmap = sc->sc_dc->dc_cmap.b[i];
		}
	}
	return 0;
}

static void
omfb_getdevconfig(paddr_t paddr, struct om_hwdevconfig *dc)
{
	int bpp, i;
	struct rasops_info *ri;
	union {
		struct { short h, v; } p;
		uint32_t u;
	} rfcnt;

	switch (hwplanemask) {
	case 0xff:
		bpp = 8;	/* XXX check monochrome bit in DIPSW */
		break;
	default:
	case 0x0f:
		bpp = 4;	/* XXX check monochrome bit in DIPSW */
		break;
	case 1:
		bpp = 1;
		break;
	}
	dc->dc_wid = 1280;
	dc->dc_ht = 1024;
	dc->dc_depth = bpp;
	dc->dc_rowbytes = 2048 / 8;
	dc->dc_cmsize = (bpp == 1) ? 0 : 1 << bpp;
	dc->dc_videobase = paddr;

	/* WHITE on BLACK */
	if (hwplanemask == 0x01) {
		struct bt454 *odac = (struct bt454 *)OMFB_RAMDAC;

		/*
		 * On 1bpp framebuffer, only plane P0 has framebuffer memory
		 * and other planes seems pulled up, i.e. always 1.
		 * Set white only for a palette (P0,P1,P2,P3) = (1,1,1,1).
		 */
		odac->bt_addr = 0;
		for (i = 0; i < 15; i++) {
			odac->bt_cmap = dc->dc_cmap.r[i] = 0;
			odac->bt_cmap = dc->dc_cmap.g[i] = 0;
			odac->bt_cmap = dc->dc_cmap.b[i] = 0;
		}
		/*
		 * The B/W video connector is connected to IOG of Bt454,
		 * and IOR and IOB are unused.
		 */
		odac->bt_cmap = dc->dc_cmap.r[15] = 0;
		odac->bt_cmap = dc->dc_cmap.g[15] = 255;
		odac->bt_cmap = dc->dc_cmap.b[15] = 0;
	} else if (hwplanemask == 0x0f) {
		struct bt454 *odac = (struct bt454 *)OMFB_RAMDAC;

		odac->bt_addr = 0;
		for (i = 0; i < 16; i++) {
			odac->bt_cmap = dc->dc_cmap.r[i] = ansicmap[i].r;
			odac->bt_cmap = dc->dc_cmap.g[i] = ansicmap[i].g;
			odac->bt_cmap = dc->dc_cmap.b[i] = ansicmap[i].b;
		}
	} else if (hwplanemask == 0xff) {
		struct bt458 *ndac = (struct bt458 *)OMFB_RAMDAC;

		/*
		 * Initialize the Bt458.  When we write to control registers,
		 * the address is not incremented automatically. So we specify
		 * it ourselves for each control register.
		 */
		ndac->bt_addr = 0x04;
		ndac->bt_ctrl = 0xff; /* all planes will be read */
		ndac->bt_addr = 0x05;
		ndac->bt_ctrl = 0x00; /* all planes have non-blink */
		ndac->bt_addr = 0x06;
		ndac->bt_ctrl = 0x40; /* pallete enabled, ovly plane disabled */
		ndac->bt_addr = 0x07;
		ndac->bt_ctrl = 0x00; /* no test mode */

		/*
		 * Set ANSI 16 colors.  We only supports 4bpp console right
		 * now, repeat 16 colors in 256 colormap.
		 */
		ndac->bt_addr = 0;
		for (i = 0; i < 256; i++) {
			ndac->bt_cmap = dc->dc_cmap.r[i] = ansicmap[i % 16].r;
			ndac->bt_cmap = dc->dc_cmap.g[i] = ansicmap[i % 16].g;
			ndac->bt_cmap = dc->dc_cmap.b[i] = ansicmap[i % 16].b;
		}
	}

	/* adjust h/v origin on screen */
	rfcnt.p.h = 7;
	rfcnt.p.v = -27;
	/* single write of 0x007ffe6 */
	*(volatile uint32_t *)OMFB_RFCNT = rfcnt.u;

	/* clear the screen */
	*(volatile uint32_t *)OMFB_PLANEMASK = 0xff;
	((volatile uint32_t *)OMFB_ROPFUNC)[5] = ~0;	/* ROP copy */
	for (i = 0; i < dc->dc_ht * dc->dc_rowbytes / sizeof(uint32_t); i++)
		*((volatile uint32_t *)dc->dc_videobase + i) = 0;
	*(volatile uint32_t *)OMFB_PLANEMASK = 0x01;

	/* initialize the raster */
	ri = &dc->dc_ri;
	ri->ri_width = dc->dc_wid;
	ri->ri_height = dc->dc_ht;
	ri->ri_depth = 1;       /* since planes are independently addressed */
	ri->ri_stride = dc->dc_rowbytes;
	ri->ri_bits = (void *)dc->dc_videobase;
	ri->ri_flg = RI_CENTER;
	if (dc == &omfb_console_dc)
		ri->ri_flg |= RI_NO_AUTO;
	ri->ri_hw = dc;

	if (bpp == 4 || bpp == 8)
		omrasops4_init(ri, 34, 80);
	else
		omrasops1_init(ri, 34, 80);

	omfb_stdscreen.nrows = ri->ri_rows;
	omfb_stdscreen.ncols = ri->ri_cols;
	omfb_stdscreen.textops = &ri->ri_ops;
	omfb_stdscreen.capabilities = ri->ri_caps;
	omfb_stdscreen.fontwidth = ri->ri_font->fontwidth;
	omfb_stdscreen.fontheight = ri->ri_font->fontheight;
}

static int
omfb_alloc_screen(void *v, const struct wsscreen_descr *type, void **cookiep,
    int *curxp, int *curyp, long *attrp)
{
	struct omfb_softc *sc = v;
	struct rasops_info *ri = &sc->sc_dc->dc_ri;

	if (sc->sc_nscreens > 0)
		return ENOMEM;

	*cookiep = ri;
	*curxp = 0;
	*curyp = 0;
	(*ri->ri_ops.allocattr)(ri, 0, 0, 0, attrp);
	sc->sc_nscreens++;
	return 0;
}

static void
omfb_free_screen(void *v, void *cookie)
{
	struct omfb_softc *sc = v;

	if (sc->sc_dc == &omfb_console_dc)
		panic("omfb_free_screen: console");

	sc->sc_nscreens--;
}

static int
omfb_show_screen(void *v, void *cookie, int waitok,
    void (*cb)(void *, int, int), void *cbarg)
{

	return 0;
}
