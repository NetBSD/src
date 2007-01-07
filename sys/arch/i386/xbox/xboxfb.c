/* $NetBSD: xboxfb.c,v 1.5 2007/01/07 01:12:42 jmcneill Exp $ */

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

#include <dev/i2c/pic16lcreg.h>

#include "opt_wsemul.h"

MALLOC_DEFINE(M_XBOXFB, "xboxfb", "xboxfb shadow framebuffer");

#define SCREEN_WIDTH_SDTV	640
#define SCREEN_HEIGHT_SDTV	480
#define SCREEN_WIDTH_HDTV	720
#define SCREEN_HEIGHT_HDTV	480
#define SCREEN_WIDTH_VGA	800
#define SCREEN_HEIGHT_VGA	600
#define SCREEN_BPP		32
#define SCREEN_SIZE(sc)		((sc)->width * (sc)->height * SCREEN_BPP)

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

static char *xboxfb_console_shadowbits;
static bus_space_handle_t xboxfb_console_memh;

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

static uint8_t	xboxfb_get_avpack(void);

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
	struct wsemuldisplaydev_attach_args aa;
	struct rasops_info *ri;
	int console;
	ulong defattr = 0;
	uint32_t bg, fg, ul;

	ri = &xboxfb_console_screen.scr_ri;

	sc->sc_memt = X86_BUS_SPACE_MEM;
	sc->sc_memh = xboxfb_console_memh;
	sc->sc_mode = WSDISPLAYIO_MODE_EMUL;
	sc->width = ri->ri_width;
	sc->height = ri->ri_height;
	sc->bits_per_pixel = SCREEN_BPP;
	sc->fbpa = XBOX_FB_START;

	aprint_normal(": %dx%d, %d bit framebuffer console\n",
		sc->width, sc->height, sc->bits_per_pixel);

	sc->fbva = (vaddr_t)bus_space_vaddr(sc->sc_memt, sc->sc_memh);
	if (sc->fbva == 0)
		return;

	sc->sc_shadowbits = xboxfb_console_shadowbits;
	if (sc->sc_shadowbits == NULL) {
		aprint_error(": unable to allocate %d bytes for shadowfb\n",
		    XBOX_FB_SIZE);
		return;
	}

	vcons_init(&sc->vd, sc, &xboxfb_defaultscreen, &xboxfb_accessops);
	sc->vd.init_screen = xboxfb_init_screen;

	/* yes, we're the console */
	console = 1;

	if (console) {
		vcons_init_screen(&sc->vd, &xboxfb_console_screen, 1,
			&defattr);
		xboxfb_console_screen.scr_flags |= VCONS_SCREEN_IS_STATIC;
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
	struct xboxfb_softc *sc = vd->cookie;
	struct wsdisplay_fbinfo *wdf;
	struct vcons_screen *ms = vd->active;

	switch (cmd) {
		case WSDISPLAYIO_GTYPE:
			*(u_int *)data = WSDISPLAY_TYPE_PCIMISC;
			return 0;

		case WSDISPLAYIO_GINFO:
			wdf = (void *)data;
			wdf->height = ms->scr_ri.ri_height;
			wdf->width = ms->scr_ri.ri_width;
			wdf->depth = ms->scr_ri.ri_depth;
			wdf->cmsize = 256;
			return 0;

		case WSDISPLAYIO_GETCMAP:
			return EINVAL;

		case WSDISPLAYIO_PUTCMAP:
			return EINVAL;

		case WSDISPLAYIO_LINEBYTES:
			*(u_int *)data = sc->width * 4;
			return 0;

		case WSDISPLAYIO_SMODE:
			{
				int new_mode = *(int *)data;
				if (new_mode != sc->sc_mode) {
					sc->sc_mode = new_mode;
					if (new_mode == WSDISPLAYIO_MODE_EMUL)
						vcons_redraw_screen(vd->active);
				}
			}
			return 0;
	}
	return EPASSTHROUGH;
}

static paddr_t
xboxfb_mmap(void *v, void *vs, off_t offset, int prot)
{
	struct vcons_data *vd;
	struct xboxfb_softc *sc;
	paddr_t pa;

	vd = (struct vcons_data *)v;
	sc = (struct xboxfb_softc *)vd->cookie;

	if (offset >= 0 && offset < XBOX_FB_SIZE) {
		pa = bus_space_mmap(X86_BUS_SPACE_MEM, XBOX_FB_START,
		    offset, prot, BUS_SPACE_MAP_LINEAR);
		return pa;
	}
		    
	return (-1);
}

static void
xboxfb_init_screen(void *cookie, struct vcons_screen *scr,
	int existing, long *defattr)
{
	struct xboxfb_softc *sc = cookie;
	struct rasops_info *ri = &scr->scr_ri;

	if (scr == &xboxfb_console_screen)
		return;

	ri->ri_depth = sc->bits_per_pixel;
	ri->ri_width = sc->width;
	ri->ri_height = sc->height;
	ri->ri_stride = sc->width * (sc->bits_per_pixel / 8);
	ri->ri_flg = RI_CENTER;

	if (xboxfb_console_shadowbits != NULL) {
		ri->ri_hwbits = bus_space_vaddr(sc->sc_memt, sc->sc_memh);
		ri->ri_bits = sc->sc_shadowbits;
	} else
		ri->ri_bits = bus_space_vaddr(sc->sc_memt, sc->sc_memh);

	if (existing) {
		ri->ri_flg |= RI_CLEAR;
	}

	rasops_init(ri, sc->height/8, sc->width/8);
	ri->ri_caps = WSSCREEN_WSCOLORS;

	rasops_reconfig(ri, sc->height / ri->ri_font->fontheight,
		sc->width / ri->ri_font->fontwidth);

	ri->ri_hw = scr;
}

/*
 * Gross hack to determine the display resolution based on the type of
 * AV cable attached at boot. Since we don't have the capability of changing
 * the display resolution yet, we need to rely on Cromwell/Xromwell to
 * set this up for us.
 */
#define XBOX_SMBUS_BA	0xc000
#define XBOX_PIC_ADDR	0x10

static uint8_t
xboxfb_smbus_pic_read(bus_space_tag_t t, bus_space_handle_t h, uint8_t cmd)
{

	bus_space_write_1(t, h, 0x04, (XBOX_PIC_ADDR << 1) | 1);
	bus_space_write_1(t, h, 0x08, cmd);
	bus_space_write_2(t, h, 0x00, bus_space_read_2(t, h, 0x00));
	bus_space_write_1(t, h, 0x02, 0x0a);
	while ((bus_space_read_1(t, h, 0x00) & 0x36) == 0)
		;
	return bus_space_read_1(t, h, 0x06);
}

static uint8_t
xboxfb_get_avpack(void)
{
	bus_space_tag_t t;
	bus_space_handle_t h;
	uint8_t rv;

	t = X86_BUS_SPACE_IO;
	rv = bus_space_map(t, XBOX_SMBUS_BA, 16, 0, &h);
	if (rv)
		return PIC16LC_REG_AVPACK_COMPOSITE; /* shouldn't happen */

	xboxfb_smbus_pic_read(t, h, PIC16LC_REG_AVPACK);

	bus_space_unmap(t, h, 16);

	return rv;
}

int
xboxfb_cnattach(void)
{
	static int ncalls = 0;
	struct rasops_info *ri = &xboxfb_console_screen.scr_ri;
	long defattr;
	uint8_t avpack;

	/* We can't attach if we're not running on an Xbox... */
	if (!arch_i386_is_xbox)
		return 1;

	/* XXX jmcneill
	 *  Console initialization is called multiple times on i386; the first
	 *  two being too early for us to use malloc or bus_space_map. Defer
	 *  initialization under these subsystems are ready.
	 */
	++ncalls;
	if (ncalls < 3)
		return -1;

	xboxfb_console_shadowbits = malloc(XBOX_FB_SIZE, M_XBOXFB, M_NOWAIT);

	if (xboxfb_console_shadowbits == NULL)
		aprint_error("xboxfb_cnattach: failed to allocate shadowfb\n");

	if (bus_space_map(X86_BUS_SPACE_MEM, XBOX_FB_START, XBOX_FB_SIZE,
	    BUS_SPACE_MAP_LINEAR, &xboxfb_console_memh)) {
		aprint_error("xboxfb_cnattach: failed to map memory.\n");
		return 1;
	}

	wsfont_init();

	/*
	 * We need to ask the pic16lc for the avpack type to determine
	 * which video mode the loader has setup for us
	 */
	avpack = xboxfb_get_avpack();
	switch (avpack) {
	case PIC16LC_REG_AVPACK_HDTV:
		ri->ri_width = SCREEN_WIDTH_HDTV;
		ri->ri_height = SCREEN_HEIGHT_HDTV;
		break;
	case PIC16LC_REG_AVPACK_VGA:
	case PIC16LC_REG_AVPACK_VGA_SOG:
		ri->ri_width = SCREEN_WIDTH_VGA;
		ri->ri_height = SCREEN_HEIGHT_VGA;
		break;
	default:
		ri->ri_width = SCREEN_WIDTH_SDTV;
		ri->ri_height = SCREEN_HEIGHT_SDTV;
		break;
	}
	ri->ri_depth = SCREEN_BPP;
	ri->ri_stride = ri->ri_width * ri->ri_depth / 8;
	ri->ri_flg = RI_CENTER;
	ri->ri_bits = xboxfb_console_shadowbits;
	ri->ri_hwbits = bus_space_vaddr(X86_BUS_SPACE_MEM,
	    xboxfb_console_memh);
	if (ri->ri_hwbits == NULL) {
		aprint_error("xboxfb_cnattach: bus_space_vaddr failed\n");
		return 1;
	}

	/* clear screen */
	memset(ri->ri_bits, 0, XBOX_FB_SIZE);
	memset(ri->ri_hwbits, 0, XBOX_FB_SIZE);

	rasops_init(ri, ri->ri_height / 8, ri->ri_width / 8);
	ri->ri_caps = WSSCREEN_WSCOLORS;
	rasops_reconfig(ri, ri->ri_height / ri->ri_font->fontheight,
	    ri->ri_width / ri->ri_font->fontwidth);

	xboxfb_defaultscreen.nrows = ri->ri_rows;
	xboxfb_defaultscreen.ncols = ri->ri_cols;
	xboxfb_defaultscreen.textops = &ri->ri_ops;
	xboxfb_defaultscreen.capabilities = ri->ri_caps;
	ri->ri_ops.allocattr(ri, 0, 0, 0, &defattr);

	wsdisplay_preattach(&xboxfb_defaultscreen, ri, 0, 0, defattr);

	return 0;
}
