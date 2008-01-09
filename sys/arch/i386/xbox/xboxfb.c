/*	$NetBSD: xboxfb.c,v 1.10.20.1 2008/01/09 01:46:52 matt Exp $	*/

/*
 * Copyright (c) 2007 Jared D. McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: xboxfb.c,v 1.10.20.1 2008/01/09 01:46:52 matt Exp $");

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

#define SCREEN_WIDTH_SDTV		640
#define SCREEN_HEIGHT_SDTV		480
#define SCREEN_WIDTH_SDTVWS_CONEXANT	1024
#define SCREEN_HEIGHT_SDTVWS_CONEXANT	576
#define SCREEN_WIDTH_HDTV		720
#define SCREEN_HEIGHT_HDTV		480
#define SCREEN_WIDTH_VGA		800
#define SCREEN_HEIGHT_VGA		600
#define SCREEN_BPP			32

/*
 * Define a safe area border for TV displays.
 */
#define XBOXFB_SAFE_AREA_SDTV_LEFT 40
#define XBOXFB_SAFE_AREA_SDTV_TOP 40
#define XBOXFB_SAFE_AREA_HDTV_LEFT 72
#define XBOXFB_SAFE_AREA_HDTV_TOP 16

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

	void *sc_ih;

	size_t memsize;

	int sc_mode;
	uint32_t sc_bg;
};

static bus_space_handle_t xboxfb_console_memh;
static struct vcons_screen xboxfb_console_screen;
static uint8_t *xboxfb_console_bits;
static int xboxfb_console_width;
static int xboxfb_console_height;

static int	xboxfb_match(struct device *, struct cfdata *, void *);
static void	xboxfb_attach(struct device *, struct device *, void *);

static uint8_t	xboxfb_get_avpack(void);
static void	xboxfb_clear_fb(struct xboxfb_softc *);

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

static int	xboxfb_ioctl(void *, void *, u_long, void *, int,
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

	aprint_normal(": %dx%d, %d bit framebuffer console\n",
	    xboxfb_console_width, xboxfb_console_height, ri->ri_depth);

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
xboxfb_ioctl(void *v, void*vs, u_long cmd, void *data, int flag,
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
			wdf->height = xboxfb_console_height;
			wdf->width = xboxfb_console_width;
			wdf->depth = ms->scr_ri.ri_depth;
			wdf->cmsize = 256;
			return 0;

		case WSDISPLAYIO_GETCMAP:
			return EINVAL;

		case WSDISPLAYIO_PUTCMAP:
			return EINVAL;

		case WSDISPLAYIO_LINEBYTES:
			*(u_int *)data = ms->scr_ri.ri_stride;
			return 0;

		case WSDISPLAYIO_SMODE:
			{
				int new_mode = *(int *)data;
				if (new_mode != sc->sc_mode) {
					sc->sc_mode = new_mode;
					if (new_mode == WSDISPLAYIO_MODE_EMUL) {
						xboxfb_clear_fb(sc);
						vcons_redraw_screen(vd->active);
					}
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
	/*struct xboxfb_softc *sc = cookie;*/
	struct rasops_info *ri = &scr->scr_ri;
	struct rasops_info *console_ri;

	if (scr == &xboxfb_console_screen)
		return;
	console_ri = &xboxfb_console_screen.scr_ri;

	ri->ri_depth = console_ri->ri_depth;
	ri->ri_width = console_ri->ri_width;
	ri->ri_height = console_ri->ri_height;
	ri->ri_stride = console_ri->ri_stride;
	ri->ri_flg = 0;

	ri->ri_hwbits = console_ri->ri_hwbits;
	ri->ri_bits = console_ri->ri_bits;

	if (existing)
		ri->ri_flg |= RI_CLEAR;

	rasops_init(ri, ri->ri_height / 8, ri->ri_width / 8);
	ri->ri_caps = WSSCREEN_WSCOLORS;

	rasops_reconfig(ri, ri->ri_height / ri->ri_font->fontheight,
		ri->ri_width / ri->ri_font->fontwidth);

	ri->ri_hw = scr;
}

static void
xboxfb_clear_fb(struct xboxfb_softc *sc)
{
	struct rasops_info *ri;
	uint32_t fbsize;

	ri = &xboxfb_console_screen.scr_ri;
	fbsize = ri->ri_height * ri->ri_stride;

	memset(xboxfb_console_bits, 0, fbsize);
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
xboxfb_smbus_read(bus_space_tag_t t, bus_space_handle_t h, uint8_t addr,
    uint8_t cmd)
{
	uint8_t val;

	bus_space_write_1(t, h, 0x04, (addr << 1) | 1);
	bus_space_write_1(t, h, 0x08, cmd);
	bus_space_write_2(t, h, 0x00, bus_space_read_2(t, h, 0x00));
	bus_space_write_1(t, h, 0x02, 0x0a);
	while (((val = bus_space_read_1(t, h, 0x00)) & 0x36) == 0)
		;
	if (((val & 0x10) == 0) || (val & 0x24))
		return 0xff;
	return bus_space_read_1(t, h, 0x06);
}

static uint8_t
xboxfb_smbus_pic_read(bus_space_tag_t t, bus_space_handle_t h, uint8_t cmd)
{
	return xboxfb_smbus_read(t, h, XBOX_PIC_ADDR, cmd);
}

/*
 * Detect the TV encoder type; used to help determine which mode has been
 * setup by the bootloader.
 */
#define XBOXFB_ENCODER_CONEXANT	1
#define XBOXFB_ENCODER_FOCUS	2
#define XBOXFB_ENCODER_XCALIBUR	3

static uint8_t
xboxfb_get_encoder(void)
{
	bus_space_tag_t t = X86_BUS_SPACE_IO;
	bus_space_handle_t h;
	uint8_t rv;

	rv = bus_space_map(t, XBOX_SMBUS_BA, 16, 0, &h);
	if (rv)
		return XBOXFB_ENCODER_XCALIBUR; /* shouldn't happen */

	if (xboxfb_smbus_read(t, h, 0x45, 0x00) != 0xff)
		rv = XBOXFB_ENCODER_CONEXANT;
	else if (xboxfb_smbus_read(t, h, 0x6a, 0x00) != 0xff)
		rv = XBOXFB_ENCODER_FOCUS;
	else
		rv = XBOXFB_ENCODER_XCALIBUR;

	bus_space_unmap(t, h, 16);

	return rv;
}

/*
 * Detect widescreen settings from the EEPROM
 */
static uint8_t
xboxfb_is_widescreen(void)
{
	bus_space_tag_t t = X86_BUS_SPACE_IO;
	bus_space_handle_t h;
	uint8_t rv;

	rv = bus_space_map(t, XBOX_SMBUS_BA, 16, 0, &h);
	if (rv)
		return 0;

	rv = xboxfb_smbus_read(t, h, 0x54, 0x96) & 1;

	bus_space_unmap(t, h, 16);

	return rv;
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

	rv = xboxfb_smbus_pic_read(t, h, PIC16LC_REG_AVPACK);

	bus_space_unmap(t, h, 16);

	return rv;
}

int
xboxfb_cnattach(void)
{
	static int ncalls = 0;
	struct rasops_info *ri = &xboxfb_console_screen.scr_ri;
	uint8_t *xboxfb_console_shadowbits = NULL;
	long defattr;
	uint8_t avpack;
	uint32_t fbsize;
	uint32_t sa_top, sa_left;

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
		sa_top = XBOXFB_SAFE_AREA_HDTV_TOP;
		sa_left = XBOXFB_SAFE_AREA_HDTV_LEFT;
		break;
	case PIC16LC_REG_AVPACK_VGA:
	case PIC16LC_REG_AVPACK_VGA_SOG:
		ri->ri_width = SCREEN_WIDTH_VGA;
		ri->ri_height = SCREEN_HEIGHT_VGA;
		sa_top = sa_left = 0;
		break;
	default:
		/* Ugh, Cromwell puts Xboxes w/ Conexant encoders that are
		 * configured for widescreen mode into a different resolution.
		 * compensate for that here.
		 */
		if (xboxfb_is_widescreen() &&
		    xboxfb_get_encoder() == XBOXFB_ENCODER_CONEXANT) {
			ri->ri_width = SCREEN_WIDTH_SDTVWS_CONEXANT;
			ri->ri_height = SCREEN_HEIGHT_SDTVWS_CONEXANT;
		} else {
			ri->ri_width = SCREEN_WIDTH_SDTV;
			ri->ri_height = SCREEN_HEIGHT_SDTV;
		}
		sa_top = XBOXFB_SAFE_AREA_SDTV_TOP;
		sa_left = XBOXFB_SAFE_AREA_SDTV_LEFT;
		break;
	}

	fbsize = ri->ri_width * ri->ri_height * 4;

	xboxfb_console_shadowbits = malloc(fbsize, M_XBOXFB, M_NOWAIT);

	if (xboxfb_console_shadowbits == NULL)
		aprint_error("xboxfb_cnattach: failed to allocate shadowfb\n");

	if (bus_space_map(X86_BUS_SPACE_MEM, XBOX_FB_START, fbsize,
	    BUS_SPACE_MAP_LINEAR, &xboxfb_console_memh)) {
		aprint_error("xboxfb_cnattach: failed to map memory.\n");
		return 1;
	}

	ri->ri_depth = SCREEN_BPP;
	ri->ri_stride = ri->ri_width * ri->ri_depth / 8;
	ri->ri_flg = 0; /* RI_CENTER does not work with shadowfb */
	if (xboxfb_console_shadowbits) {
		ri->ri_bits = xboxfb_console_shadowbits;
		ri->ri_hwbits = bus_space_vaddr(X86_BUS_SPACE_MEM,
		    xboxfb_console_memh);
		xboxfb_console_bits = ri->ri_hwbits;
	} else {
		ri->ri_bits = bus_space_vaddr(X86_BUS_SPACE_MEM,
		    xboxfb_console_memh);
		ri->ri_hwbits = NULL;
		xboxfb_console_bits = ri->ri_bits;
	}

	/* clear screen */
	if (ri->ri_bits != NULL)
		memset(ri->ri_bits, 0, fbsize);
	if (ri->ri_hwbits != NULL)
		memset(ri->ri_hwbits, 0, fbsize);


	xboxfb_console_width = ri->ri_width;
	xboxfb_console_height = ri->ri_height;

	/* Define a TV safe area where applicable */
	if (sa_left > 0) {
		ri->ri_hwbits += (sa_left * 4);
		ri->ri_width -= (sa_left * 2);
	}
	if (sa_top > 0) {
		ri->ri_hwbits += (ri->ri_stride * sa_top);
		ri->ri_height -= (sa_top * 2);
	}

	rasops_init(ri, ri->ri_height / 8, ri->ri_width / 8);
	ri->ri_caps = WSSCREEN_WSCOLORS;
	rasops_reconfig(ri,
	    ri->ri_height / ri->ri_font->fontheight,
	    ri->ri_width / ri->ri_font->fontwidth);

	xboxfb_defaultscreen.nrows = ri->ri_rows;
	xboxfb_defaultscreen.ncols = ri->ri_cols;
	xboxfb_defaultscreen.textops = &ri->ri_ops;
	xboxfb_defaultscreen.capabilities = ri->ri_caps;
	ri->ri_ops.allocattr(ri, 0, 0, 0, &defattr);

	wsdisplay_preattach(&xboxfb_defaultscreen, ri, 0, 0, defattr);

	return 0;
}
