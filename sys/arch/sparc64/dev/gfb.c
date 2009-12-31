/*	$NetBSD: gfb.c,v 1.2 2009/12/31 05:08:05 macallan Exp $	*/

/*
 * Copyright (c) 2009 Michael Lorenz
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * A console driver for Sun XVR-1000 graphics cards
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: gfb.c,v 1.2 2009/12/31 05:08:05 macallan Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/lwp.h>
#include <sys/kauth.h>
#include <sys/kmem.h>

#include <uvm/uvm_extern.h>

#include <dev/videomode/videomode.h>

#include <machine/bus.h>
#include <machine/autoconf.h>
#include <machine/openfirm.h>

#include <dev/wscons/wsdisplayvar.h>
#include <dev/wscons/wsconsio.h>
#include <dev/wsfont/wsfont.h>
#include <dev/rasops/rasops.h>
#include <dev/wscons/wsdisplay_vconsvar.h>


struct gfb_softc {
	device_t sc_dev;

	bus_space_tag_t sc_memt;

	bus_space_handle_t sc_fbh;
	bus_addr_t sc_fb_paddr;

	int sc_width, sc_height, sc_depth, sc_stride, sc_fblen;
	int sc_locked;
	void *sc_fbaddr, *sc_shadow;
	struct vcons_screen sc_console_screen;
	struct wsscreen_descr sc_defaultscreen_descr;
	const struct wsscreen_descr *sc_screens[1];
	struct wsscreen_list sc_screenlist;
	struct vcons_data vd;
	int sc_mode;
	int sc_node;
	u_char sc_cmap_red[256];
	u_char sc_cmap_green[256];
	u_char sc_cmap_blue[256];
};

static int	gfb_match(device_t, cfdata_t, void *);
static void	gfb_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(gfb, sizeof(struct gfb_softc),
    gfb_match, gfb_attach, NULL, NULL);

extern const u_char rasops_cmap[768];

static int	gfb_ioctl(void *, void *, u_long, void *, int,
			     struct lwp *);
static paddr_t	gfb_mmap(void *, void *, off_t, int);
static void	gfb_init_screen(void *, struct vcons_screen *, int, long *);

static int	gfb_putcmap(struct gfb_softc *, struct wsdisplay_cmap *);
static int 	gfb_getcmap(struct gfb_softc *, struct wsdisplay_cmap *);
static void	gfb_restore_palette(struct gfb_softc *);
static int 	gfb_putpalreg(struct gfb_softc *, uint8_t, uint8_t,
			    uint8_t, uint8_t);

struct wsdisplay_accessops gfb_accessops = {
	gfb_ioctl,
	gfb_mmap,
	NULL,	/* alloc_screen */
	NULL,	/* free_screen */
	NULL,	/* show_screen */
	NULL, 	/* load_font */
	NULL,	/* pollc */
	NULL	/* scroll */
};

extern int prom_stdout_node;

static int
gfb_match(device_t parent, cfdata_t match, void *aux)
{
	struct mainbus_attach_args *ma = aux;

	if (strcmp(ma->ma_name, "SUNW,gfb") == 0 )
		return 100;
	return 0;
}

static void
gfb_attach(device_t parent, device_t self, void *aux)
{
	struct gfb_softc	*sc = device_private(self);
	struct mainbus_attach_args	*ma = aux;
	struct rasops_info	*ri;
	struct wsemuldisplaydev_attach_args aa;
	unsigned long		defattr;
	bool			is_console;
	int i, j;

	sc->sc_memt = ma->ma_bustag;
	sc->sc_dev = self;
	sc->sc_node = ma->ma_node;

	if (ma->ma_nreg < 7) {
		aprint_error("%s: can't find the fb range\n",
		    device_xname(self));
		return;
	}

	is_console = (prom_stdout_node == ma->ma_node);

	aprint_normal(": Sun XVR-1000%s\n", is_console ? " (console)" : "");

	sc->sc_depth = 32;
	sc->sc_stride = 0x4000;
	sc->sc_height = prom_getpropint(sc->sc_node, "height", 0);
	sc->sc_width = prom_getpropint(sc->sc_node, "width", 0);
	sc->sc_fblen = sc->sc_stride * sc->sc_height;

	if (sc->sc_fblen == 0) {
		aprint_error("%s: not set up by firmware, can't continue.\n",
		    device_xname(self));
		return;
	}

	sc->sc_shadow = kmem_alloc(sc->sc_fblen, KM_SLEEP);
	sc->sc_locked = 0;
	sc->sc_mode = WSDISPLAYIO_MODE_EMUL;

	if (bus_space_map(sc->sc_memt, ma->ma_reg[6].ur_paddr,
	    sc->sc_fblen, BUS_SPACE_MAP_LINEAR, &sc->sc_fbh)) {
		printf(": failed to map the framebuffer\n");
		return;
	}
	sc->sc_fbaddr = bus_space_vaddr(sc->sc_memt, sc->sc_fbh);
	sc->sc_fb_paddr = ma->ma_reg[6].ur_paddr;

	sc->sc_defaultscreen_descr = (struct wsscreen_descr){
		"default",
		0, 0,
		NULL,
		8, 16,
		WSSCREEN_WSCOLORS | WSSCREEN_HILIT,
		NULL
	};
	sc->sc_screens[0] = &sc->sc_defaultscreen_descr;
	sc->sc_screenlist = (struct wsscreen_list){1, sc->sc_screens};
	sc->sc_mode = WSDISPLAYIO_MODE_EMUL;
	sc->sc_locked = 0;

	vcons_init(&sc->vd, sc, &sc->sc_defaultscreen_descr,
	    &gfb_accessops);
	sc->vd.init_screen = gfb_init_screen;

	ri = &sc->sc_console_screen.scr_ri;

	j = 0;
	for (i = 0; i < (1 << sc->sc_depth); i++) {

		sc->sc_cmap_red[i] = rasops_cmap[j];
		sc->sc_cmap_green[i] = rasops_cmap[j + 1];
		sc->sc_cmap_blue[i] = rasops_cmap[j + 2];
		gfb_putpalreg(sc, i, rasops_cmap[j], rasops_cmap[j + 1],
		    rasops_cmap[j + 2]);
		j += 3;
	}

	if (is_console) {
		vcons_init_screen(&sc->vd, &sc->sc_console_screen, 1,
		    &defattr);
		sc->sc_console_screen.scr_flags |= VCONS_SCREEN_IS_STATIC;

#if notyet
		gfb_rectfill(sc, 0, 0, sc->sc_width, sc->sc_height,
		    ri->ri_devcmap[(defattr >> 16) & 0xff]);
#endif
		sc->sc_defaultscreen_descr.textops = &ri->ri_ops;
		sc->sc_defaultscreen_descr.capabilities = ri->ri_caps;
		sc->sc_defaultscreen_descr.nrows = ri->ri_rows;
		sc->sc_defaultscreen_descr.ncols = ri->ri_cols;
		wsdisplay_cnattach(&sc->sc_defaultscreen_descr, ri, 0, 0,
		    defattr);
		vcons_replay_msgbuf(&sc->sc_console_screen);
	} else {
		/*
		 * since we're not the console we can postpone the rest
		 * until someone actually allocates a screen for us
		 */
		(*ri->ri_ops.allocattr)(ri, 0, 0, 0, &defattr);
	}

	aa.console = is_console;
	aa.scrdata = &sc->sc_screenlist;
	aa.accessops = &gfb_accessops;
	aa.accesscookie = &sc->vd;

	config_found(sc->sc_dev, &aa, wsemuldisplaydevprint);
}

static int
gfb_ioctl(void *v, void *vs, u_long cmd, void *data, int flag,
	struct lwp *l)
{
	struct vcons_data *vd = v;
	struct gfb_softc *sc = vd->cookie;
	struct wsdisplay_fbinfo *wdf;
	struct vcons_screen *ms = vd->active;

	switch (cmd) {

		case WSDISPLAYIO_GTYPE:
			*(u_int *)data = WSDISPLAY_TYPE_XVR1000;
			return 0;

		case WSDISPLAYIO_GINFO:
			if (ms == NULL)
				return ENODEV;
			wdf = (void *)data;
			wdf->height = ms->scr_ri.ri_height;
			wdf->width = ms->scr_ri.ri_width;
			wdf->depth = ms->scr_ri.ri_depth;
			wdf->cmsize = 256;
			return 0;

		case WSDISPLAYIO_GETCMAP:
			return gfb_getcmap(sc,
			    (struct wsdisplay_cmap *)data);

		case WSDISPLAYIO_PUTCMAP:
			return gfb_putcmap(sc,
			    (struct wsdisplay_cmap *)data);

		case WSDISPLAYIO_LINEBYTES:
			*(u_int *)data = sc->sc_stride;
			return 0;

		case WSDISPLAYIO_SMODE:
			{
				int new_mode = *(int*)data;

				/* notify the bus backend */
				if (new_mode != sc->sc_mode) {
					sc->sc_mode = new_mode;
					if(new_mode == WSDISPLAYIO_MODE_EMUL) {
						gfb_restore_palette(sc);
						vcons_redraw_screen(ms);
					}
				}
			}
			return 0;
	}
	return EPASSTHROUGH;
}

static paddr_t
gfb_mmap(void *v, void *vs, off_t offset, int prot)
{
	struct vcons_data *vd = v;
	struct gfb_softc *sc = vd->cookie;
	paddr_t pa;

	/* 'regular' framebuffer mmap()ing */
	if (offset < sc->sc_fblen) {
		pa = bus_space_mmap(sc->sc_memt, sc->sc_fb_paddr + offset, 0,
		    prot, BUS_SPACE_MAP_LINEAR);
		return pa;
	}

	/*
	 * restrict all other mappings to processes with superuser privileges
	 * or the kernel itself
	 */
	if (kauth_authorize_generic(kauth_cred_get(), KAUTH_GENERIC_ISSUSER,
	    NULL) != 0) {
		aprint_normal("%s: mmap() rejected.\n",
		    device_xname(sc->sc_dev));
		return -1;
	}

	/* let userland map other ranges */

	return -1;
}

static void
gfb_init_screen(void *cookie, struct vcons_screen *scr,
    int existing, long *defattr)
{
	struct gfb_softc *sc = cookie;
	struct rasops_info *ri = &scr->scr_ri;

	ri->ri_depth = sc->sc_depth;
	ri->ri_width = sc->sc_width;
	ri->ri_height = sc->sc_height;
	ri->ri_stride = sc->sc_stride;
	ri->ri_flg = RI_CENTER | RI_FULLCLEAR;

	ri->ri_bits = sc->sc_shadow;
	ri->ri_hwbits = (char *)sc->sc_fbaddr;

	if (existing) {
		ri->ri_flg |= RI_CLEAR;
	}

	rasops_init(ri, sc->sc_height / 8, sc->sc_width / 8);
	ri->ri_caps = WSSCREEN_WSCOLORS;

	rasops_reconfig(ri, sc->sc_height / ri->ri_font->fontheight,
		    sc->sc_width / ri->ri_font->fontwidth);

	ri->ri_hw = scr;
#if 0
	ri->ri_ops.copyrows = gfb_copyrows;
	ri->ri_ops.copycols = gfb_copycols;
	ri->ri_ops.cursor = gfb_cursor;
	ri->ri_ops.eraserows = gfb_eraserows;
	ri->ri_ops.erasecols = gfb_erasecols;
	ri->ri_ops.putchar = gfb_putchar;
#endif
}

static int
gfb_putcmap(struct gfb_softc *sc, struct wsdisplay_cmap *cm)
{
	u_char *r, *g, *b;
	u_int index = cm->index;
	u_int count = cm->count;
	int i, error;
	u_char rbuf[256], gbuf[256], bbuf[256];

#ifdef GFB_DEBUG
	aprint_debug("putcmap: %d %d\n",index, count);
#endif
	if (cm->index >= 256 || cm->count > 256 ||
	    (cm->index + cm->count) > 256)
		return EINVAL;
	error = copyin(cm->red, &rbuf[index], count);
	if (error)
		return error;
	error = copyin(cm->green, &gbuf[index], count);
	if (error)
		return error;
	error = copyin(cm->blue, &bbuf[index], count);
	if (error)
		return error;

	memcpy(&sc->sc_cmap_red[index], &rbuf[index], count);
	memcpy(&sc->sc_cmap_green[index], &gbuf[index], count);
	memcpy(&sc->sc_cmap_blue[index], &bbuf[index], count);

	r = &sc->sc_cmap_red[index];
	g = &sc->sc_cmap_green[index];
	b = &sc->sc_cmap_blue[index];

	for (i = 0; i < count; i++) {
		gfb_putpalreg(sc, index, *r, *g, *b);
		index++;
		r++, g++, b++;
	}
	return 0;
}

static int
gfb_getcmap(struct gfb_softc *sc, struct wsdisplay_cmap *cm)
{
	u_int index = cm->index;
	u_int count = cm->count;
	int error;

	if (index >= 255 || count > 256 || index + count > 256)
		return EINVAL;

	error = copyout(&sc->sc_cmap_red[index],   cm->red,   count);
	if (error)
		return error;
	error = copyout(&sc->sc_cmap_green[index], cm->green, count);
	if (error)
		return error;
	error = copyout(&sc->sc_cmap_blue[index],  cm->blue,  count);
	if (error)
		return error;

	return 0;
}

static void
gfb_restore_palette(struct gfb_softc *sc)
{
	int i;

	for (i = 0; i < (1 << sc->sc_depth); i++) {
		gfb_putpalreg(sc, i, sc->sc_cmap_red[i],
		    sc->sc_cmap_green[i], sc->sc_cmap_blue[i]);
	}
}

static int
gfb_putpalreg(struct gfb_softc *sc, uint8_t idx, uint8_t r, uint8_t g,
    uint8_t b)
{
	return 0;
}
