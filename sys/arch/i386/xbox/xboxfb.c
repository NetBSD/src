/* $NetBSD: xboxfb.c,v 1.2 2007/01/05 02:09:13 jmcneill Exp $ */

/*
 * Copyright (c) 2006 Andrew Gillham
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 * A console driver for the Xbox.
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/callout.h>
#include <sys/lwp.h>
#include <sys/kauth.h>

#include <uvm/uvm_extern.h>
/* #include <machine/autoconf.h> */
#include <machine/bus.h>
#include <machine/xbox.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>
#include <dev/pci/pciio.h>

#include <dev/wscons/wsdisplayvar.h>
#include <dev/wscons/wsconsio.h>
#include <dev/wsfont/wsfont.h>
#include <dev/rasops/rasops.h>
#include <dev/wscons/wsdisplay_vconsvar.h>

#include "opt_wsemul.h"

MALLOC_DEFINE(M_XBOXFB, "xboxfb", "xboxfb shadow framebuffer");

#define SCREEN_WIDTH	640
#define SCREEN_HEIGHT	480
#define SCREEN_BPP	32
#define SCREEN_SIZE	(SCREEN_WIDTH*SCREEN_HEIGHT*SCREEN_BPP)

#define FONT_HEIGHT	16
#define FONT_WIDTH	8

#define CHAR_HEIGHT	16
#define CHAR_WIDTH	10

/*
#define XBOX_RAM_SIZE		(arch_i386_xbox_memsize * 1024 * 1024)
#define XBOX_FB_SIZE		(0x400000)
#define XBOX_FB_START		(0xF0000000 | (XBOX_RAM_SIZE - XBOX_FB_SIZE))
#define XBOX_FB_START_PTR	(0xFD600800)
*/

struct xboxfb_softc {
	struct device sc_dev;
	struct vcons_data vd;

	bus_space_tag_t sc_memt;
	bus_space_handle_t sc_memh;

	vaddr_t fbva;
	paddr_t fbpa;

	void *sc_ih;

	size_t memsize;

	int bits_per_pixel;
	int width, height, linebytes;

	int sc_mode;
	uint32_t sc_bg;

	char *sc_shadowbits;
};

static struct vcons_screen xboxfb_console_screen;

static int	xboxfb_match(struct device *, struct cfdata *, void *);
static void	xboxfb_attach(struct device *, struct device *, void *);

CFATTACH_DECL(xboxfb, sizeof(struct xboxfb_softc), xboxfb_match,
	xboxfb_attach, NULL, NULL);

/* static void	xboxfb_init(struct xboxfb_softc *); */

/* static void	xboxfb_cursor(void *, int, int, int); */
/* static void	xboxfb_copycols(void *, int, int, int, int); */
/* static void	xboxfb_erasecols(void *, int, int, int, long); */
/* static void	xboxfb_copyrows(void *, int, int, int); */
/* static void	xboxfb_eraserows(void *, int, int, long); */

struct wsscreen_descr xboxfb_defaultscreen = {
	"default",
	0, 0,
	NULL,
	8, 16,
	WSSCREEN_WSCOLORS,
	NULL,
};

const struct wsscreen_descr *_xboxfb_scrlist[] = {
	&xboxfb_defaultscreen,
};

struct wsscreen_list xboxfb_screenlist = {
	sizeof(_xboxfb_scrlist) / sizeof(struct wsscreen_descr *),
		_xboxfb_scrlist
};

static int	xboxfb_ioctl(void *, void *, u_long, caddr_t, int,
			struct lwp *);
static paddr_t	xboxfb_mmap(void *, void *, off_t, int);
static void	xboxfb_init_screen(void *, struct vcons_screen *, int,
			long *);

struct wsdisplay_accessops xboxfb_accessops = {
	xboxfb_ioctl,
	xboxfb_mmap,
	NULL,	/* alloc_screen */
	NULL,	/* free_screen */
	NULL,	/* show_screen */
	NULL,	/* load_font */
	NULL,	/* pollc */
	NULL,	/* scroll */
};

static int
xboxfb_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct pci_attach_args *pa = (struct pci_attach_args *)aux;

	/* Don't match on non-Xbox i386 */
	if (!arch_i386_is_xbox) {
		return (0);
	}

	if (PCI_CLASS(pa->pa_class) != PCI_CLASS_DISPLAY ||
 	    PCI_SUBCLASS(pa->pa_class) != PCI_SUBCLASS_DISPLAY_VGA)
		return 0;
	if ((PCI_VENDOR(pa->pa_id) == PCI_VENDOR_NVIDIA) &&
	    (PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_NVIDIA_XBOXFB))
		return 100;
	return 0;
};

static void
xboxfb_attach(struct device *parent, struct device *self, void *aux)
{
	struct xboxfb_softc *sc = (void *)self;
	struct pci_attach_args *pa = aux;
	struct wsemuldisplaydev_attach_args aa;
	struct rasops_info *ri;
	int console;
/*	int width, height; */
	ulong defattr = 0;
	uint32_t bg, fg, ul;

	sc->sc_memt = pa->pa_memt;
	sc->sc_mode = WSDISPLAYIO_MODE_EMUL;
	sc->width = SCREEN_WIDTH;
	sc->height = SCREEN_HEIGHT;
	sc->bits_per_pixel = SCREEN_BPP;
	sc->fbpa = XBOX_FB_START;

	aprint_normal(": %dx%d, %d bit framebuffer enabled.\n",
		sc->width, sc->height, sc->bits_per_pixel);

	if (bus_space_map(sc->sc_memt, XBOX_FB_START, XBOX_FB_SIZE,
		BUS_SPACE_MAP_LINEAR, &sc->sc_memh)) {
		aprint_error(": failed to map memory.\n");
		return;
	}

	sc->fbva = (vaddr_t)bus_space_vaddr(sc->sc_memt, sc->sc_memh);
	if (sc->fbva == 0)
		return;

	sc->sc_shadowbits = malloc(XBOX_FB_SIZE, M_XBOXFB, M_NOWAIT);
	if (sc->sc_shadowbits == NULL) {
		aprint_error(": unable to allocate %d bytes for shadowfb\n",
		    XBOX_FB_SIZE);
		return;
	}

	ri = &xboxfb_console_screen.scr_ri;
	memset(ri, 0, sizeof(struct rasops_info));

	vcons_init(&sc->vd, sc, &xboxfb_defaultscreen, &xboxfb_accessops);
	sc->vd.init_screen = xboxfb_init_screen;

	/* yes, we're the console */
	console = 1;

	if (console) {
		xboxfb_console_screen.scr_flags |= VCONS_SCREEN_IS_STATIC;
		vcons_init_screen(&sc->vd, &xboxfb_console_screen, 1,
			&defattr);
		vcons_redraw_screen(&xboxfb_console_screen);

		xboxfb_defaultscreen.textops = &ri->ri_ops;
		xboxfb_defaultscreen.capabilities = ri->ri_caps;
		xboxfb_defaultscreen.nrows = ri->ri_rows;
		xboxfb_defaultscreen.ncols = ri->ri_cols;
		xboxfb_defaultscreen.modecookie = NULL;
		wsdisplay_cnattach(&xboxfb_defaultscreen, ri, 0, 0, defattr);
	} 

	rasops_unpack_attr(defattr, &fg, &bg, &ul);
	sc->sc_bg = ri->ri_devcmap[bg];

	aa.console = console;
	aa.scrdata = &xboxfb_screenlist;
	aa.accessops = &xboxfb_accessops;
	aa.accesscookie = &sc->vd;

	config_found(self, &aa, wsemuldisplaydevprint);
}

/*
 * respond to ioctl requests
 */

static int
xboxfb_ioctl(void *v, void*vs, u_long cmd, caddr_t data, int flag,
	struct lwp *l)
{
	struct vcons_data *vd = v;
/*	struct xboxfb_softc *sc = vd->cookie; */
	struct wsdisplay_fbinfo *wdf;
	struct vcons_screen *ms = vd->active;

	switch (cmd) {
		case WSDISPLAYIO_GTYPE:
			*(u_int *)data = WSDISPLAY_TYPE_PCIMISC;
			return (0);

		case WSDISPLAYIO_GINFO:
			wdf = (void *)data;
			wdf->height = ms->scr_ri.ri_height;
			wdf->width = ms->scr_ri.ri_width;
			wdf->depth = ms->scr_ri.ri_depth;
			wdf->cmsize = 256;
			return (0);

		case WSDISPLAYIO_GETCMAP:
			return (0);

		case WSDISPLAYIO_PUTCMAP:
			return (0);

		case WSDISPLAYIO_SMODE:
			return (0);
	}
	return EPASSTHROUGH;
}

static paddr_t
xboxfb_mmap(void *v, void *vs, off_t offset, int prot)
{
	aprint_normal(": mmap called.");
	return (-1);
}

static void
xboxfb_init_screen(void *cookie, struct vcons_screen *scr,
	int existing, long *defattr)
{
	struct xboxfb_softc *sc = cookie;
	struct rasops_info *ri = &scr->scr_ri;

	ri->ri_depth = sc->bits_per_pixel;
	ri->ri_width = sc->width;
	ri->ri_height = sc->height;
	ri->ri_stride = sc->width * (sc->bits_per_pixel / 8);
	ri->ri_flg = RI_CENTER;

	ri->ri_hwbits = bus_space_vaddr(sc->sc_memt, sc->sc_memh);
	ri->ri_bits = sc->sc_shadowbits;

	if (existing) {
		ri->ri_flg |= RI_CLEAR;
	}

	rasops_init(ri, sc->height/8, sc->width/8);
	ri->ri_caps = WSSCREEN_WSCOLORS;

	rasops_reconfig(ri, sc->height / ri->ri_font->fontheight,
		sc->width / ri->ri_font->fontwidth);

	ri->ri_hw = scr;
}

int
xboxfb_cnattach(void)
{
	/* XXX: we should really setup the graphics console early. */
	return (0);
}
