/*	$NetBSD: mgx.c,v 1.1 2014/12/16 21:01:34 macallan Exp $ */

/*-
 * Copyright (c) 2014 Michael Lorenz
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

/* a console driver for the SSB 4096V-MGX graphics card */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mgx.c,v 1.1 2014/12/16 21:01:34 macallan Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/conf.h>
#include <sys/kmem.h>

#include <sys/bus.h>
#include <machine/autoconf.h>

#include <dev/sbus/sbusvar.h>
#include <dev/sun/fbio.h>
#include <dev/sun/fbvar.h>

#include <dev/wscons/wsdisplayvar.h>
#include <dev/wscons/wsconsio.h>
#include <dev/wsfont/wsfont.h>
#include <dev/rasops/rasops.h>

#include <dev/wscons/wsdisplay_vconsvar.h>

#include <dev/ic/vgareg.h>
#include <dev/sbus/mgxreg.h>

#include "opt_wsemul.h"


struct mgx_softc {
	device_t	sc_dev;
	bus_space_tag_t sc_tag;
	bus_space_handle_t sc_blith;
	bus_space_handle_t sc_vgah;
	bus_addr_t	sc_paddr;
	void		*sc_fbaddr;
	int		sc_width;
	int		sc_height;
	int		sc_stride;
	int		sc_fbsize;
	int		sc_mode;
	u_char		sc_cmap_red[256];
	u_char		sc_cmap_green[256];
	u_char		sc_cmap_blue[256];
	struct vcons_screen sc_console_screen;
	struct wsscreen_descr sc_defaultscreen_descr;
	const struct wsscreen_descr *sc_screens[1];
	struct wsscreen_list sc_screenlist;
	struct vcons_data vd;
};

static int	mgx_match(device_t, cfdata_t, void *);
static void	mgx_attach(device_t, device_t, void *);
static int	mgx_ioctl(void *, void *, u_long, void *, int,
				 struct lwp*);
static paddr_t	mgx_mmap(void *, void *, off_t, int);
static void	mgx_init_screen(void *, struct vcons_screen *, int,
				 long *);
static void	mgx_write_dac(struct mgx_softc *, int, int, int, int);
static void	mgx_setup(struct mgx_softc *, int);
static void	mgx_init_palette(struct mgx_softc *);

CFATTACH_DECL_NEW(mgx, sizeof(struct mgx_softc),
    mgx_match, mgx_attach, NULL, NULL);

struct wsdisplay_accessops mgx_accessops = {
	mgx_ioctl,
	mgx_mmap,
	NULL,	/* vcons_alloc_screen */
	NULL,	/* vcons_free_screen */
	NULL,	/* vcons_show_screen */
	NULL,	/* load_font */
	NULL,	/* polls */
	NULL,	/* scroll */
};

static int
mgx_match(device_t parent, cfdata_t cf, void *aux)
{
	struct sbus_attach_args *sa = aux;

	if (strcmp("SMSI,mgx", sa->sa_name) == 0)
		return 100;
	return 0;
}

/*
 * Attach a display.  We need to notice if it is the console, too.
 */
static void
mgx_attach(device_t parent, device_t self, void *args)
{
	struct mgx_softc *sc = device_private(self);
	struct sbus_attach_args *sa = args;
	struct wsemuldisplaydev_attach_args aa;
	struct rasops_info *ri;
	unsigned long defattr;
	bus_space_handle_t bh;
	int node = sa->sa_node;
	int isconsole;

	aprint_normal("\n");
	sc->sc_dev = self;
	sc->sc_tag = sa->sa_bustag;

	sc->sc_paddr = sbus_bus_addr(sa->sa_bustag, sa->sa_slot,
	    sa->sa_reg[8].oa_base);

	/* read geometry information from the device tree */
	sc->sc_width = prom_getpropint(sa->sa_node, "width", 1152);
	sc->sc_height = prom_getpropint(sa->sa_node, "height", 900);
	sc->sc_stride = prom_getpropint(sa->sa_node, "linebytes", 900);
	sc->sc_fbsize = sc->sc_height * sc->sc_stride;
	sc->sc_fbaddr = NULL; //(void *)(unsigned long)prom_getpropint(sa->sa_node, "address", 0);
	if (sc->sc_fbaddr == NULL) {
		if (sbus_bus_map(sa->sa_bustag,
			 sa->sa_slot,
			 sa->sa_reg[8].oa_base,
			 sc->sc_fbsize,
			 BUS_SPACE_MAP_LINEAR, &bh) != 0) {
			aprint_error_dev(self, "cannot map framebuffer\n");
			return;
		}
		sc->sc_fbaddr = bus_space_vaddr(sa->sa_bustag, bh);
	}
		
	aprint_normal_dev(self, "%d x %d\n", sc->sc_width, sc->sc_height);

	if (sbus_bus_map(sa->sa_bustag,
			 sa->sa_slot,
			 sa->sa_reg[4].oa_base, 0x1000, 0,
			 &sc->sc_vgah) != 0) {
		aprint_error("%s: couldn't map VGA registers\n", 
		    device_xname(sc->sc_dev));
		return;
	}

	if (sbus_bus_map(sa->sa_bustag,
			 sa->sa_slot,
			 sa->sa_reg[5].oa_base, 0x1000, 0, 
			 &sc->sc_blith) != 0) {
		aprint_error("%s: couldn't map blitter registers\n", 
		    device_xname(sc->sc_dev));
		return;
	}

	mgx_setup(sc, 8);

	sc->sc_defaultscreen_descr = (struct wsscreen_descr) {
		"default",
		0, 0,
		NULL,
		8, 16,
		WSSCREEN_WSCOLORS | WSSCREEN_HILIT,
		NULL
	};
	sc->sc_screens[0] = &sc->sc_defaultscreen_descr;
	sc->sc_screenlist = (struct wsscreen_list){1, sc->sc_screens};

	isconsole = fb_is_console(node);

	sc->sc_mode = WSDISPLAYIO_MODE_EMUL;
	wsfont_init();

	vcons_init(&sc->vd, sc, &sc->sc_defaultscreen_descr, &mgx_accessops);
	sc->vd.init_screen = mgx_init_screen;

	vcons_init_screen(&sc->vd, &sc->sc_console_screen, 1, &defattr);
	sc->sc_console_screen.scr_flags |= VCONS_SCREEN_IS_STATIC;

	ri = &sc->sc_console_screen.scr_ri;

	sc->sc_defaultscreen_descr.nrows = ri->ri_rows;
	sc->sc_defaultscreen_descr.ncols = ri->ri_cols;
	sc->sc_defaultscreen_descr.textops = &ri->ri_ops;
	sc->sc_defaultscreen_descr.capabilities = ri->ri_caps;

	mgx_init_palette(sc);

	if(isconsole) {
		wsdisplay_cnattach(&sc->sc_defaultscreen_descr, ri, 0, 0, defattr);
		vcons_replay_msgbuf(&sc->sc_console_screen);
	}

	aa.console = isconsole;
	aa.scrdata = &sc->sc_screenlist;
	aa.accessops = &mgx_accessops;
	aa.accesscookie = &sc->vd;

	config_found(self, &aa, wsemuldisplaydevprint);
}

static void
mgx_write_vga(struct mgx_softc *sc, uint32_t reg, uint8_t val)
{
	bus_space_write_1(sc->sc_tag, sc->sc_vgah, reg ^ 3, val);
}

static void
mgx_write_dac(struct mgx_softc *sc, int idx, int r, int g, int b)
{
	mgx_write_vga(sc, VGA_BASE + VGA_DAC_ADDRW, idx);
	mgx_write_vga(sc, VGA_BASE + VGA_DAC_PALETTE, r);
	mgx_write_vga(sc, VGA_BASE + VGA_DAC_PALETTE, g);
	mgx_write_vga(sc, VGA_BASE + VGA_DAC_PALETTE, b);
}

static void
mgx_init_palette(struct mgx_softc *sc)
{
	struct rasops_info *ri = &sc->sc_console_screen.scr_ri;
	int i, j = 0;
	uint8_t cmap[768];

	rasops_get_cmap(ri, cmap, sizeof(cmap));
	for (i = 0; i < 256; i++) {
		sc->sc_cmap_red[i] = cmap[j];
		sc->sc_cmap_green[i] = cmap[j + 1];
		sc->sc_cmap_blue[i] = cmap[j + 2];
		mgx_write_dac(sc, i, cmap[j], cmap[j + 1], cmap[j + 2]);
		j += 3;
	}
}

static void
mgx_setup(struct mgx_softc *sc, int depth)
{
}

static void
mgx_init_screen(void *cookie, struct vcons_screen *scr,
    int existing, long *defattr)
{
	struct mgx_softc *sc = cookie;
	struct rasops_info *ri = &scr->scr_ri;

	ri->ri_depth = 8;
	ri->ri_width = sc->sc_width;
	ri->ri_height = sc->sc_height;
	ri->ri_stride = sc->sc_stride;
	ri->ri_flg = RI_CENTER;

#if _LP64
	/*
	 * XXX
	 * Assuming all 64bit SPARCs are fast enough to render anti-aliased
	 * text on the fly. Matters only as long as we don't have acceleration
	 * and glyphcache. 
	 */
	if (ri->ri_depth == 8)
		ri->ri_flg |= RI_8BIT_IS_RGB | RI_ENABLE_ALPHA;
#endif

	ri->ri_bits = sc->sc_fbaddr;
	scr->scr_flags |= VCONS_DONT_READ;

	rasops_init(ri, 0, 0);

	ri->ri_caps = WSSCREEN_REVERSE | WSSCREEN_WSCOLORS;

	rasops_reconfig(ri, ri->ri_height / ri->ri_font->fontheight,
		    ri->ri_width / ri->ri_font->fontwidth);
}

static int
mgx_ioctl(void *v, void *vs, u_long cmd, void *data, int flag,
    struct lwp *l)
{
	struct vcons_data *vd = v;
	struct mgx_softc *sc = vd->cookie;
	struct wsdisplay_fbinfo *wdf;
	struct vcons_screen *ms = vd->active;

	switch (cmd) {
		case WSDISPLAYIO_GTYPE:
			*(u_int *)data = WSDISPLAY_TYPE_MGX;
			return 0;

		case WSDISPLAYIO_GINFO:
			wdf = (void *)data;
			wdf->height = sc->sc_height;
			wdf->width = sc->sc_width;
			wdf->depth = 8;
			wdf->cmsize = 256;
			return 0;

		case FBIOGVIDEO:
		case WSDISPLAYIO_GVIDEO:
			*(int *)data = 1;
			return 0;

		case WSDISPLAYIO_SVIDEO:
		case FBIOSVIDEO:
			return 0;

		case WSDISPLAYIO_LINEBYTES:
			{
				int *ret = (int *)data;
				*ret = sc->sc_stride;
			}
			return 0;

		case WSDISPLAYIO_SMODE:
			{
				int new_mode = *(int*)data;
				if (new_mode != sc->sc_mode)
				{
					sc->sc_mode = new_mode;
					if (new_mode == WSDISPLAYIO_MODE_EMUL)
					{
						mgx_setup(sc, 8);
						vcons_redraw_screen(ms);
					} else {
						mgx_setup(sc, 32);
					}
				}
			}
	}

	return EPASSTHROUGH;
}

static paddr_t
mgx_mmap(void *v, void *vs, off_t offset, int prot)
{
	struct vcons_data *vd = v;
	struct mgx_softc *sc = vd->cookie;

	/* regular fb mapping at 0 */
	if ((offset >= 0) && (offset < 0x400000)) {
		return bus_space_mmap(sc->sc_tag, sc->sc_paddr,
		    offset, prot, BUS_SPACE_MAP_LINEAR);
	}

	return -1;
}
