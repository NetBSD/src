/*	$NetBSD: cgtwelve.c,v 1.1 2010/03/24 00:33:06 macallan Exp $ */

/*-
 * Copyright (c) 2010 Michael Lorenz
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

/* a console driver for the Sun CG12 / Matrox SG3 graphics board */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cgtwelve.c,v 1.1 2010/03/24 00:33:06 macallan Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/conf.h>

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

#include <dev/sbus/cgtwelvereg.h>

#include "opt_wsemul.h"
#include "opt_cgtwelve.h"


struct cgtwelve_softc {
	device_t	sc_dev;
	bus_space_tag_t sc_tag;
	void		*sc_fbaddr;
	int		sc_width;
	int		sc_height;
	int		sc_stride;
	int		sc_fbsize;
	int		sc_mode;
	struct vcons_data vd;
};

static int	cgtwelve_match(device_t, cfdata_t, void *);
static void	cgtwelve_attach(device_t, device_t, void *);
static int	cgtwelve_ioctl(void *, void *, u_long, void *, int,
				 struct lwp*);
static paddr_t	cgtwelve_mmap(void *, void *, off_t, int);
static void	cgtwelve_init_screen(void *, struct vcons_screen *, int,
				 long *);

CFATTACH_DECL_NEW(cgtwelve, sizeof(struct cgtwelve_softc),
    cgtwelve_match, cgtwelve_attach, NULL, NULL);

struct wsscreen_descr cgtwelve_defscreendesc = {
	"default",
	0, 0,
	NULL,
	8, 16,
	0,
};

static struct vcons_screen cgtwelve_console_screen;

const struct wsscreen_descr *_cgtwelve_scrlist[] = {
	&cgtwelve_defscreendesc,
	/* XXX other formats, graphics screen? */
};

struct wsscreen_list cgtwelve_screenlist = {
	sizeof(_cgtwelve_scrlist) / sizeof(struct wsscreen_descr *),
	_cgtwelve_scrlist
};

struct wsdisplay_accessops cgtwelve_accessops = {
	cgtwelve_ioctl,
	cgtwelve_mmap,
	NULL,	/* vcons_alloc_screen */
	NULL,	/* vcons_free_screen */
	NULL,	/* vcons_show_screen */
	NULL,	/* load_font */
	NULL,	/* polls */
	NULL,	/* scroll */
};

static int
cgtwelve_match(device_t parent, cfdata_t cf, void *aux)
{
	struct sbus_attach_args *sa = aux;

	if (strcmp("cgtwelve", sa->sa_name) == 0)
		return 100;
	return 0;
}

/*
 * Attach a display.  We need to notice if it is the console, too.
 */
static void
cgtwelve_attach(device_t parent, device_t self, void *args)
{
	struct cgtwelve_softc *sc = device_private(self);
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

	/* read geometry information from the device tree */
	sc->sc_width = prom_getpropint(sa->sa_node, "width", 1152);
	sc->sc_height = prom_getpropint(sa->sa_node, "height", 900);
	sc->sc_stride = (sc->sc_width + 7) >> 3;

	sc->sc_fbsize = sc->sc_height * sc->sc_stride;
	sc->sc_fbaddr = (void *)prom_getpropint(sa->sa_node, "address", 0);
	if (sc->sc_fbaddr == NULL) {
		if (sbus_bus_map(sa->sa_bustag,
			 sa->sa_slot,
			 sa->sa_offset + CG12_FB_MONO,
			 sc->sc_fbsize,
			 BUS_SPACE_MAP_LINEAR, &bh) != 0) {
			aprint_error_dev(self, "cannot map framebuffer\n");
			return;
		}
		sc->sc_fbaddr = bus_space_vaddr(sa->sa_bustag, bh);
	}
		
	aprint_normal_dev(self, "%d x %d\n", sc->sc_width, sc->sc_height);

	isconsole = fb_is_console(node);

	sc->sc_mode = WSDISPLAYIO_MODE_EMUL;
	wsfont_init();

	vcons_init(&sc->vd, sc, &cgtwelve_defscreendesc, &cgtwelve_accessops);
	sc->vd.init_screen = cgtwelve_init_screen;

	vcons_init_screen(&sc->vd, &cgtwelve_console_screen, 1, &defattr);
	cgtwelve_console_screen.scr_flags |= VCONS_SCREEN_IS_STATIC;

	memset(sc->sc_fbaddr, 0, sc->sc_fbsize);

	ri = &cgtwelve_console_screen.scr_ri;

	cgtwelve_defscreendesc.nrows = ri->ri_rows;
	cgtwelve_defscreendesc.ncols = ri->ri_cols;
	cgtwelve_defscreendesc.textops = &ri->ri_ops;
	cgtwelve_defscreendesc.capabilities = ri->ri_caps;

	if(isconsole) {
		wsdisplay_cnattach(&cgtwelve_defscreendesc, ri, 0, 0, defattr);
		vcons_replay_msgbuf(&cgtwelve_console_screen);
	}

	aa.console = isconsole;
	aa.scrdata = &cgtwelve_screenlist;
	aa.accessops = &cgtwelve_accessops;
	aa.accesscookie = &sc->vd;

	config_found(self, &aa, wsemuldisplaydevprint);

}


static void
cgtwelve_init_screen(void *cookie, struct vcons_screen *scr,
    int existing, long *defattr)
{
	struct cgtwelve_softc *sc = cookie;
	struct rasops_info *ri = &scr->scr_ri;

	ri->ri_depth = 1;
	ri->ri_width = sc->sc_width;
	ri->ri_height = sc->sc_height;
	ri->ri_stride = sc->sc_stride;
	ri->ri_flg = RI_CENTER;

	ri->ri_bits = sc->sc_fbaddr;

	rasops_init(ri, ri->ri_height/8, ri->ri_width/8);
	ri->ri_caps = WSSCREEN_REVERSE;
	rasops_reconfig(ri, ri->ri_height / ri->ri_font->fontheight,
		    ri->ri_width / ri->ri_font->fontwidth);
}

static int
cgtwelve_ioctl(void *v, void *vs, u_long cmd, void *data, int flag,
    struct lwp *l)
{

	switch (cmd) {
		case WSDISPLAYIO_GTYPE:
			*(u_int *)data = WSDISPLAY_TYPE_SUNCG12;
			return 0;
	}

	return EPASSTHROUGH;
}

static paddr_t
cgtwelve_mmap(void *v, void *vs, off_t offset, int prot)
{
	struct cgtwelve_softc *sc = v;

	/* regular fb mapping at 0 */
	if ((offset >= 0) && (offset < sc->sc_fbsize)) {
#if 0
		return bus_space_mmap(sc->sc_tag, sc->sc_paddr,
		    CG12_FB_MONO + offset, prot,
		    BUS_SPACE_MAP_LINEAR);
#endif
	}

	return -1;
}
