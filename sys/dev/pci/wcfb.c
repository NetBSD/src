/*	$NetBSD: wcfb.c,v 1.11.2.2 2017/12/03 11:37:29 jdolecek Exp $ */

/*
 * Copyright (c) 2007, 2008, 2009 Miodrag Vallat.
 *               2010 Michael Lorenz
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/* a driver for (some) 3DLabs Wildcat cards, based on OpenBSD's ifb driver */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: wcfb.c,v 1.11.2.2 2017/12/03 11:37:29 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/mutex.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/kauth.h>
#include <sys/kmem.h>

#include <dev/pci/pcidevs.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pciio.h>
#include <dev/pci/wcfbreg.h>

#include <dev/wscons/wsdisplayvar.h>
#include <dev/wscons/wsconsio.h>
#include <dev/wsfont/wsfont.h>
#include <dev/rasops/rasops.h>
#include <dev/wscons/wsdisplay_vconsvar.h>
#include <dev/pci/wsdisplay_pci.h>

#include "opt_wsfb.h"
#include "opt_wsdisplay_compat.h"

#ifdef WCFB_DEBUG
# define DPRINTF printf
#else
# define DPRINTF while (0) printf
#endif

static int	wcfb_match(device_t, cfdata_t, void *);
static void	wcfb_attach(device_t, device_t, void *);
static int	wcfb_ioctl(void *, void *, u_long, void *, int,
		    struct lwp *);
static paddr_t	wcfb_mmap(void *, void *, off_t, int);

struct wcfb_softc {
	device_t sc_dev;

	pci_chipset_tag_t sc_pc;
	pcitag_t sc_pcitag;

	bus_space_tag_t sc_memt;
	bus_space_tag_t sc_regt, sc_wtft;
	bus_space_tag_t sc_iot;

	bus_space_handle_t sc_fbh;
	bus_space_handle_t sc_regh;
	bus_addr_t sc_fb, sc_reg;
	bus_size_t sc_fbsize, sc_regsize;

	int sc_width, sc_height, sc_stride;
	int sc_locked;
	uint8_t *sc_fbaddr, *sc_fb0, *sc_fb1, *sc_shadow;
	struct vcons_screen sc_console_screen;
	struct wsscreen_descr sc_defaultscreen_descr;
	const struct wsscreen_descr *sc_screens[1];
	struct wsscreen_list sc_screenlist;
	struct vcons_data vd;
	int sc_mode, sc_dpms;
	u_char sc_cmap_red[256];
	u_char sc_cmap_green[256];
	u_char sc_cmap_blue[256];
	uint32_t sc_fb0off, sc_fb1off, sc_fb8size;

	void (*copycols)(void *, int, int, int, int);
	void (*erasecols)(void *, int, int, int, long);
	void (*copyrows)(void *, int, int, int);
	void (*eraserows)(void *, int, int, long);
	void (*putchar)(void *, int, int, u_int, long);
	void (*cursor)(void *, int, int, int);
	int sc_is_jfb;
};

static void	wcfb_init_screen(void *, struct vcons_screen *, int, long *);

CFATTACH_DECL_NEW(wcfb, sizeof(struct wcfb_softc),
    wcfb_match, wcfb_attach, NULL, NULL);

struct wsdisplay_accessops wcfb_accessops = {
	wcfb_ioctl,
	wcfb_mmap,
	NULL,	/* alloc_screen */
	NULL,	/* free_screen */
	NULL,	/* show_screen */
	NULL, 	/* load_font */
	NULL,	/* pollc */
	NULL	/* scroll */
};

static void	wcfb_putchar(void *, int, int, u_int, long);
static void	wcfb_cursor(void *, int, int, int);
static void	wcfb_copycols(void *, int, int, int, int);
static void	wcfb_erasecols(void *, int, int, int, long);
static void	wcfb_copyrows(void *, int, int, int);
static void	wcfb_eraserows(void *, int, int, long);

static void	wcfb_acc_putchar(void *, int, int, u_int, long);
static void	wcfb_acc_cursor(void *, int, int, int);
static void	wcfb_acc_copycols(void *, int, int, int, int);
static void	wcfb_acc_erasecols(void *, int, int, int, long);
static void	wcfb_acc_copyrows(void *, int, int, int);
static void	wcfb_acc_eraserows(void *, int, int, long);

static void 	wcfb_putpalreg(struct wcfb_softc *, int, int, int, int);

static void	wcfb_bitblt(struct wcfb_softc *, int, int, int, int, int,
			int, uint32_t);
static void	wcfb_rectfill(struct wcfb_softc *, int, int, int, int, int);
static void	wcfb_rop_common(struct wcfb_softc *, bus_addr_t, int, int, int,
			int, int, int, uint32_t, int32_t);
static void	wcfb_rop_jfb(struct wcfb_softc *, int, int, int, int, int, int,
			uint32_t, int32_t);
static int	wcfb_rop_wait(struct wcfb_softc *);

static int
wcfb_match(device_t parent, cfdata_t match, void *aux)
{
	struct pci_attach_args *pa = aux;

	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_3DLABS &&
	    PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_3DLABS_WILDCAT5110)
		return 100;

	return 0;
}

static void
wcfb_attach(device_t parent, device_t self, void *aux)
{
	struct wcfb_softc *sc = device_private(self);
	struct pci_attach_args *pa = aux;
	struct rasops_info	*ri;
	prop_dictionary_t	dict;
	struct wsemuldisplaydev_attach_args aa;
	int 			i, j;
	uint32_t		reg;
	unsigned long		defattr;
	bool			is_console = 0;
	uint32_t		sub;

	sc->sc_dev = self;
	sc->putchar = NULL;
	pci_aprint_devinfo(pa, NULL);

	dict = device_properties(self);
	prop_dictionary_get_bool(dict, "is_console", &is_console);
#ifndef WCFB_DEBUG
	if (!is_console) return;
#endif
	sc->sc_memt = pa->pa_memt;
	sc->sc_iot = pa->pa_iot;
	sc->sc_pc = pa->pa_pc;
	sc->sc_pcitag = pa->pa_tag;

	if (pci_mapreg_map(pa, 0x14, PCI_MAPREG_TYPE_MEM, 0,
	    &sc->sc_regt, &sc->sc_regh, &sc->sc_reg, &sc->sc_regsize)) {
		aprint_error("%s: failed to map registers.\n",
		    device_xname(sc->sc_dev));
	}

	if (pci_mapreg_map(pa, 0x10, PCI_MAPREG_TYPE_MEM,
	    BUS_SPACE_MAP_LINEAR | BUS_SPACE_MAP_PREFETCHABLE,
	    &sc->sc_memt, &sc->sc_fbh, &sc->sc_fb, &sc->sc_fbsize)) {
		aprint_error("%s: failed to map framebuffer.\n",
		    device_xname(sc->sc_dev));
	}

	sc->sc_fbaddr = bus_space_vaddr(sc->sc_memt, sc->sc_fbh);
#ifdef DEBUG
	memset(sc->sc_fbaddr, 0, sc->sc_fbsize);
#endif
	sc->sc_fb0off =
	    bus_space_read_4(sc->sc_regt, sc->sc_regh,
	        WC_FB8_ADDR0) - sc->sc_fb;
	sc->sc_fb0 = sc->sc_fbaddr + sc->sc_fb0off;
	sc->sc_fb1off =
	    bus_space_read_4(sc->sc_regt, sc->sc_regh,
	        WC_FB8_ADDR1) - sc->sc_fb;
	sc->sc_fb1 = sc->sc_fbaddr + sc->sc_fb1off;
	sc->sc_fb8size = 2 * (sc->sc_fb1off - sc->sc_fb0off);

	sub = pci_conf_read(sc->sc_pc, sc->sc_pcitag, PCI_SUBSYS_ID_REG);
	aprint_normal("subsys: %08x\n", sub);
	switch (sub) {
		case WC_XVR1200:
			sc->sc_is_jfb = 1;
			break;
		default:
			sc->sc_is_jfb = 0;
	}

	reg = bus_space_read_4(sc->sc_regt, sc->sc_regh, WC_RESOLUTION);
	sc->sc_height = (reg >> 16) + 1;
#ifdef WCFB_DEBUG
	sc->sc_height -= 200;
#endif
	sc->sc_width = (reg & 0xffff) + 1;
	sc->sc_stride = 1 <<
	    ((bus_space_read_4(sc->sc_regt, sc->sc_regh, WC_CONFIG) &
	      0x00ff0000) >> 16);
	aprint_normal_dev(self, "%d x %d, %d\n",
	    sc->sc_width, sc->sc_height, sc->sc_stride);

	if (sc->sc_is_jfb == 0) {
		sc->sc_shadow = kmem_alloc(sc->sc_stride * sc->sc_height,
		    KM_SLEEP);
	}

	for (i = 0x40; i < 0x100; i += 16) {
		aprint_normal("%04x:", i);
		for (j = 0; j < 16; j += 4) {
			aprint_normal(" %08x", bus_space_read_4(sc->sc_regt,
			    sc->sc_regh, 0x8000 + i + j));
		}
		aprint_normal("\n");
	}

	/* make sure video output is on */
	bus_space_write_4(sc->sc_regt, sc->sc_regh, WC_DPMS_STATE, WC_DPMS_ON);
	sc->sc_dpms = WSDISPLAYIO_VIDEO_ON;

#if 0
	/* testing & debugging voodoo */
	memset(sc->sc_fb0, 0x01, 0x00100000);
	memset(sc->sc_fb1, 0x00, 0x00100000);
	wcfb_rop_wait(sc);
	wcfb_rop_jfb(sc, 0, 0, 0, 0, 600, 600, WC_ROP_SET, 0xffffffff);
	wcfb_rop_wait(sc);
	delay(4000000);
	bus_space_write_4(sc->sc_regt, sc->sc_regh, WC_FB8_ADDR1,
	    bus_space_read_4(sc->sc_regt, sc->sc_regh, WC_FB8_ADDR0));
	delay(8000000);
#endif
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
	    &wcfb_accessops);
	sc->vd.init_screen = wcfb_init_screen;

	/* init engine here */
#if 0
	wcfb_init(sc);
#endif

	ri = &sc->sc_console_screen.scr_ri;

	j = 0;
	for (i = 0; i < 256; i++) {

		sc->sc_cmap_red[i] = rasops_cmap[j];
		sc->sc_cmap_green[i] = rasops_cmap[j + 1];
		sc->sc_cmap_blue[i] = rasops_cmap[j + 2];
		wcfb_putpalreg(sc, i, rasops_cmap[j], rasops_cmap[j + 1],
		    rasops_cmap[j + 2]);
		j += 3;
	}

	if (is_console) {
		vcons_init_screen(&sc->vd, &sc->sc_console_screen, 1,
		    &defattr);
		sc->sc_console_screen.scr_flags |= VCONS_SCREEN_IS_STATIC;

		if (sc->sc_is_jfb) {
			wcfb_rectfill(sc, 0, 0, sc->sc_width, sc->sc_height,
				ri->ri_devcmap[(defattr >> 16) & 0xff]);
		} else {
			memset(sc->sc_fb0,
			    ri->ri_devcmap[(defattr >> 16) & 0xff],
			    sc->sc_stride * sc->sc_height);
			memset(sc->sc_fb1,
			    ri->ri_devcmap[(defattr >> 16) & 0xff],
			    sc->sc_stride * sc->sc_height);
		}
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
		memset(sc->sc_fb0, WS_DEFAULT_BG,
		    sc->sc_stride * sc->sc_height);
		memset(sc->sc_fb1, WS_DEFAULT_BG,
		    sc->sc_stride * sc->sc_height);
		return;
	}

	aa.console = is_console;
	aa.scrdata = &sc->sc_screenlist;
	aa.accessops = &wcfb_accessops;
	aa.accesscookie = &sc->vd;

	config_found(sc->sc_dev, &aa, wsemuldisplaydevprint);
}

static int
wcfb_putcmap(struct wcfb_softc *sc, struct wsdisplay_cmap *cm)
{
	u_char *r, *g, *b;
	u_int index = cm->index;
	u_int count = cm->count;
	int i, error;
	u_char rbuf[256], gbuf[256], bbuf[256];

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
		wcfb_putpalreg(sc, index, *r, *g, *b);
		index++;
		r++, g++, b++;
	}
	return 0;
}

static int
wcfb_getcmap(struct wcfb_softc *sc, struct wsdisplay_cmap *cm)
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

static int
wcfb_ioctl(void *v, void *vs, u_long cmd, void *data, int flag,
    struct lwp *l)
{
	struct vcons_data *vd = v;
	struct wcfb_softc *sc = vd->cookie;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = WSDISPLAY_TYPE_PCIMISC;
		return 0;

	/* PCI config read/write passthrough. */
	case PCI_IOC_CFGREAD:
	case PCI_IOC_CFGWRITE:
		return pci_devioctl(sc->sc_pc, sc->sc_pcitag,
		    cmd, data, flag, l);

	case WSDISPLAYIO_GET_BUSID:
		return wsdisplayio_busid_pci(sc->sc_dev, sc->sc_pc,
		    sc->sc_pcitag, data);

	case WSDISPLAYIO_SVIDEO: {
		int new_mode = *(int*)data;
		if (new_mode != sc->sc_dpms) {
			sc->sc_dpms = new_mode;
			bus_space_write_4(sc->sc_regt, sc->sc_regh,
			     WC_DPMS_STATE,
			     (new_mode == WSDISPLAYIO_VIDEO_ON) ?
			      WC_DPMS_ON : WC_DPMS_STANDBY);
		}
		}
		return 0;

	case WSDISPLAYIO_GVIDEO:
		*(int*)data = sc->sc_dpms;
		return 0;

	case WSDISPLAYIO_GETCMAP:
		return wcfb_getcmap(sc,
		    (struct wsdisplay_cmap *)data);

	case WSDISPLAYIO_PUTCMAP:
		return wcfb_putcmap(sc,
		    (struct wsdisplay_cmap *)data);

	case WSDISPLAYIO_GET_FBINFO: {
		struct wsdisplayio_fbinfo *fbi = data;

		fbi->fbi_fbsize = sc->sc_fb8size;
		fbi->fbi_fboffset = 0;
		fbi->fbi_width = sc->sc_width;
		fbi->fbi_height = sc->sc_height;
		fbi->fbi_bitsperpixel = 8;
		fbi->fbi_stride = sc->sc_stride;
		fbi->fbi_pixeltype = WSFB_CI;
		fbi->fbi_subtype.fbi_cmapinfo.cmap_entries = 256;
		fbi->fbi_flags = WSFB_VRAM_IS_SPLIT;
		return 0;
		}
	}
	return EPASSTHROUGH;
}

static paddr_t
wcfb_mmap(void *v, void *vs, off_t offset, int prot)
{
	struct vcons_data *vd = v;
	struct wcfb_softc *sc = vd->cookie;

	/* XXX in theory the order is not fixed... */

	if (offset < sc->sc_fb8size)
		return bus_space_mmap(sc->sc_memt, sc->sc_fb + sc->sc_fb0off,
		    offset, prot,
		    BUS_SPACE_MAP_LINEAR | BUS_SPACE_MAP_PREFETCHABLE);
	/*
	 * restrict all other mappings to processes with superuser privileges
	 * or the kernel itself
	 */
	if (kauth_authorize_machdep(kauth_cred_get(),
	    KAUTH_MACHDEP_UNMANAGEDMEM,
	    NULL, NULL, NULL, NULL) != 0) {
		aprint_normal_dev(sc->sc_dev, "mmap() rejected.\n");
		return -1;
	}

	/* may want to mmap() registers at some point */

	return -1;
}

static void
wcfb_init_screen(void *cookie, struct vcons_screen *scr,
    int existing, long *defattr)
{
	struct wcfb_softc *sc = cookie;
	struct rasops_info *ri = &scr->scr_ri;

	ri->ri_depth = 8;
	ri->ri_width = sc->sc_width;
	ri->ri_height = sc->sc_height;
	ri->ri_stride = sc->sc_stride;
	ri->ri_flg = RI_CENTER /*| RI_FULLCLEAR*/;

	if (sc->sc_is_jfb) {
		ri->ri_bits = sc->sc_fb0;
	} else {
		ri->ri_bits = sc->sc_shadow;
	}
	if (existing) {
		ri->ri_flg |= RI_CLEAR;
	}

	rasops_init(ri, 0, 0);
	ri->ri_caps = WSSCREEN_WSCOLORS;

	rasops_reconfig(ri, sc->sc_height / ri->ri_font->fontheight,
		    sc->sc_width / ri->ri_font->fontwidth);

	ri->ri_hw = scr;
	sc->putchar = ri->ri_ops.putchar;
	sc->copyrows = ri->ri_ops.copyrows;
	sc->eraserows = ri->ri_ops.eraserows;
	sc->copycols = ri->ri_ops.copycols;
	sc->erasecols = ri->ri_ops.erasecols;

	if (sc->sc_is_jfb) {
		ri->ri_ops.copyrows = wcfb_acc_copyrows;
		ri->ri_ops.copycols = wcfb_acc_copycols;
		ri->ri_ops.eraserows = wcfb_acc_eraserows;
		ri->ri_ops.erasecols = wcfb_acc_erasecols;
		ri->ri_ops.putchar = wcfb_acc_putchar;
		ri->ri_ops.cursor = wcfb_acc_cursor;
	} else {
		ri->ri_ops.copyrows = wcfb_copyrows;
		ri->ri_ops.copycols = wcfb_copycols;
		ri->ri_ops.eraserows = wcfb_eraserows;
		ri->ri_ops.erasecols = wcfb_erasecols;
		ri->ri_ops.putchar = wcfb_putchar;
		ri->ri_ops.cursor = wcfb_cursor;
	}
}

static void
wcfb_putchar(void *cookie, int row, int col, u_int c, long attr)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct wcfb_softc *sc = scr->scr_cookie;
	int offset = (ri->ri_yorigin + row * ri->ri_font->fontheight) *
	    sc->sc_stride + ri->ri_xorigin + col * ri->ri_font->fontwidth;
	uint8_t *from, *to0, *to1;
	int i;

	sc->putchar(ri, row, col, c, attr);
	from = sc->sc_shadow + offset;
	to0 = sc->sc_fb0 + offset;
	to1 = sc->sc_fb1 + offset;
	for (i = 0; i < ri->ri_font->fontheight; i++) {
		memcpy(to0, from, ri->ri_font->fontwidth);
		memcpy(to1, from, ri->ri_font->fontwidth);
		to0 += sc->sc_stride;
		to1 += sc->sc_stride;
		from += sc->sc_stride;
	}
}

static void
wcfb_putpalreg(struct wcfb_softc *sc, int i, int r, int g, int b)
{
	uint32_t rgb;

	bus_space_write_4(sc->sc_regt, sc->sc_regh, WC_CMAP_INDEX, i);
	rgb = (b << 22) | (g << 12) | (r << 2);
	bus_space_write_4(sc->sc_regt, sc->sc_regh, WC_CMAP_DATA, rgb);
}

static void
wcfb_cursor(void *cookie, int on, int row, int col)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct wcfb_softc *sc = scr->scr_cookie;
	int coffset;

	if (sc->sc_mode == WSDISPLAYIO_MODE_EMUL) {

		if (ri->ri_flg & RI_CURSOR) {
			/* remove cursor */
			coffset = ri->ri_ccol + (ri->ri_crow * ri->ri_cols);
#ifdef WSDISPLAY_SCROLLSUPPORT
			coffset += scr->scr_offset_to_zero;
#endif
			wcfb_putchar(cookie, ri->ri_crow,
			    ri->ri_ccol, scr->scr_chars[coffset],
			    scr->scr_attrs[coffset]);
			ri->ri_flg &= ~RI_CURSOR;
		}
		ri->ri_crow = row;
		ri->ri_ccol = col;
		if (on) {
			long attr, revattr;
			coffset = col + (row * ri->ri_cols);
#ifdef WSDISPLAY_SCROLLSUPPORT
			coffset += scr->scr_offset_to_zero;
#endif
			attr = scr->scr_attrs[coffset];
			revattr = attr ^ 0xffff0000;

			wcfb_putchar(cookie, ri->ri_crow, ri->ri_ccol,
			    scr->scr_chars[coffset], revattr);
			ri->ri_flg |= RI_CURSOR;
		}
	} else {
		ri->ri_crow = row;
		ri->ri_ccol = col;
		ri->ri_flg &= ~RI_CURSOR;
	}
}

static void
wcfb_copycols(void *cookie, int row, int srccol, int dstcol, int ncols)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct wcfb_softc *sc = scr->scr_cookie;
	int offset = (ri->ri_yorigin + row * ri->ri_font->fontheight) *
	    sc->sc_stride + ri->ri_xorigin + dstcol * ri->ri_font->fontwidth;
	uint8_t *from, *to0, *to1;
	int i;

	sc->copycols(ri, row, srccol, dstcol, ncols);
	from = sc->sc_shadow + offset;
	to0 = sc->sc_fb0 + offset;
	to1 = sc->sc_fb1 + offset;
	for (i = 0; i < ri->ri_font->fontheight; i++) {
		memcpy(to0, from, ri->ri_font->fontwidth * ncols);
		memcpy(to1, from, ri->ri_font->fontwidth * ncols);
		to0 += sc->sc_stride;
		to1 += sc->sc_stride;
		from += sc->sc_stride;
	}
}

static void
wcfb_erasecols(void *cookie, int row, int startcol, int ncols, long fillattr)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct wcfb_softc *sc = scr->scr_cookie;
	int offset = (ri->ri_yorigin + row * ri->ri_font->fontheight) *
	    sc->sc_stride + ri->ri_xorigin + startcol * ri->ri_font->fontwidth;
	uint8_t *to0, *to1;
	int i;

	sc->erasecols(ri, row, startcol, ncols, fillattr);

	to0 = sc->sc_fb0 + offset;
	to1 = sc->sc_fb1 + offset;
	for (i = 0; i < ri->ri_font->fontheight; i++) {
		memset(to0, ri->ri_devcmap[(fillattr >> 16) & 0xff],
		    ri->ri_font->fontwidth * ncols);
		memset(to1, ri->ri_devcmap[(fillattr >> 16) & 0xff],
		    ri->ri_font->fontwidth * ncols);
		to0 += sc->sc_stride;
		to1 += sc->sc_stride;
	}
}

static void
wcfb_copyrows(void *cookie, int srcrow, int dstrow, int nrows)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct wcfb_softc *sc = scr->scr_cookie;
	int offset = (ri->ri_yorigin + dstrow * ri->ri_font->fontheight) *
	    sc->sc_stride + ri->ri_xorigin;
	uint8_t *from, *to0, *to1;
	int i;

	sc->copyrows(ri, srcrow, dstrow, nrows);

	from = sc->sc_shadow + offset;
	to0 = sc->sc_fb0 + offset;
	to1 = sc->sc_fb1 + offset;
	for (i = 0; i < ri->ri_font->fontheight * nrows; i++) {
		memcpy(to0, from, ri->ri_emuwidth);
		memcpy(to1, from, ri->ri_emuwidth);
		to0 += sc->sc_stride;
		to1 += sc->sc_stride;
		from += sc->sc_stride;
	}
}

static void
wcfb_eraserows(void *cookie, int row, int nrows, long fillattr)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct wcfb_softc *sc = scr->scr_cookie;
	int offset = (ri->ri_yorigin + row * ri->ri_font->fontheight) *
	    sc->sc_stride + ri->ri_xorigin;
	uint8_t *to0, *to1;
	int i;

	sc->eraserows(ri, row, nrows, fillattr);

	to0 = sc->sc_fb0 + offset;
	to1 = sc->sc_fb1 + offset;
	for (i = 0; i < ri->ri_font->fontheight * nrows; i++) {
		memset(to0, ri->ri_devcmap[(fillattr >> 16) & 0xff],
		    ri->ri_emuwidth);
		memset(to1, ri->ri_devcmap[(fillattr >> 16) & 0xff],
		    ri->ri_emuwidth);
		to0 += sc->sc_stride;
		to1 += sc->sc_stride;
	}
}

static void
wcfb_bitblt(struct wcfb_softc *sc, int sx, int sy, int dx, int dy, int w,
		 int h, uint32_t rop)
{
	wcfb_rop_wait(sc);
	wcfb_rop_jfb(sc, sx, sy, dx, dy, w, h, rop, 0x0f);
}

static void
wcfb_rectfill(struct wcfb_softc *sc, int x, int y, int w, int h, int bg)
{
	int32_t mask;

	/* clear everything just in case... */
	wcfb_rop_wait(sc);
	wcfb_rop_jfb(sc, x, y, x, y, w, h, WC_ROP_CLEAR, 0xffffffff);

	/* pixels to set... */
	mask = 0x0f & bg;
	if (mask != 0) {
		wcfb_rop_wait(sc);
		wcfb_rop_jfb(sc, x, y, x, y, w, h, WC_ROP_SET, mask);
	}

}

void
wcfb_rop_common(struct wcfb_softc *sc, bus_addr_t reg, int sx, int sy,
    int dx, int dy, int w, int h, uint32_t rop, int32_t planemask)
{
	int dir = 0;

	/*
	 * Compute rop direction. This only really matters for
	 * screen-to-screen copies.
	 */
	if (sy < dy /* && sy + h > dy */) {
		sy += h - 1;
		dy += h;
		dir |= WC_BLT_DIR_BACKWARDS_Y;
	}
	if (sx < dx /* && sx + w > dx */) {
		sx += w - 1;
		dx += w;
		dir |= WC_BLT_DIR_BACKWARDS_X;
	}

	/* Which one of those below is your magic number for today? */
	bus_space_write_4(sc->sc_regt, sc->sc_regh, reg, 0x61000001);
	bus_space_write_4(sc->sc_regt, sc->sc_regh, reg, 0);
	bus_space_write_4(sc->sc_regt, sc->sc_regh, reg, 0x6301c080);
	bus_space_write_4(sc->sc_regt, sc->sc_regh, reg, 0x80000000);
	bus_space_write_4(sc->sc_regt, sc->sc_regh, reg, rop);
	bus_space_write_4(sc->sc_regt, sc->sc_regh, reg, planemask);
	bus_space_write_4(sc->sc_regt, sc->sc_regh, reg, 0);
	bus_space_write_4(sc->sc_regt, sc->sc_regh, reg, 0x64000303);
	/*
	 * This value is a pixel offset within the destination area. It is
	 * probably used to define complex polygon shapes, with the
	 * last pixel in the list being back to (0,0).
	 */
	bus_space_write_4(sc->sc_regt, sc->sc_regh, reg, WCFB_COORDS(0, 0));
	bus_space_write_4(sc->sc_regt, sc->sc_regh, reg, 0);
	bus_space_write_4(sc->sc_regt, sc->sc_regh, reg, 0x00030000);
	bus_space_write_4(sc->sc_regt, sc->sc_regh, reg, 0x2200010d);

	bus_space_write_4(sc->sc_regt, sc->sc_regh, reg, 0x33f01000 | dir);
	bus_space_write_4(sc->sc_regt, sc->sc_regh, reg, WCFB_COORDS(dx, dy));
	bus_space_write_4(sc->sc_regt, sc->sc_regh, reg, WCFB_COORDS(w, h));
	bus_space_write_4(sc->sc_regt, sc->sc_regh, reg, WCFB_COORDS(sx, sy));
}


static void
wcfb_rop_jfb(struct wcfb_softc *sc, int sx, int sy, int dx, int dy,
             int w, int h, uint32_t rop, int32_t planemask)
{
	bus_addr_t reg = WC_JFB_ENGINE;
	uint32_t spr, splr;

#if 0
	/*
	 * Pick the current spr and splr values from the communication
	 * area if possible.
	 */
	if (sc->sc_comm != NULL) {
		spr = sc->sc_comm[IFB_SHARED_TERM8_SPR >> 2];
		splr = sc->sc_comm[IFB_SHARED_TERM8_SPLR >> 2];
	} else
#endif
	{
		/* supposedly sane defaults */
		spr = 0x54ff0303;
		splr = 0x5c0000ff;
	}

	bus_space_write_4(sc->sc_regt, sc->sc_regh, reg, 0x00400016);
	bus_space_write_4(sc->sc_regt, sc->sc_regh, reg, 0x5b000002);
	bus_space_write_4(sc->sc_regt, sc->sc_regh, reg, 0x5a000000);
	bus_space_write_4(sc->sc_regt, sc->sc_regh, reg, spr);
	bus_space_write_4(sc->sc_regt, sc->sc_regh, reg, splr);

	wcfb_rop_common(sc, reg, sx, sy, dx, dy, w, h, rop, planemask);

	bus_space_write_4(sc->sc_regt, sc->sc_regh, reg, 0x5a000001);
	bus_space_write_4(sc->sc_regt, sc->sc_regh, reg, 0x5b000001);
}

static int
wcfb_rop_wait(struct wcfb_softc *sc)
{
	int i;

	for (i = 1000000; i != 0; i--) {
		if (bus_space_read_4(sc->sc_regt, sc->sc_regh,
		    WC_STATUS) & WC_STATUS_DONE)
			break;
		delay(1);
	}

	return i;
}

static void
wcfb_acc_putchar(void *cookie, int row, int col, u_int c, long attr)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct wcfb_softc *sc = scr->scr_cookie;
	struct wsdisplay_font *font = PICK_FONT(ri, c);
	int x, y, wi, he;
	uint32_t bg;

	wi = font->fontwidth;
	he = font->fontheight;
	x = ri->ri_xorigin + col * wi;
	y = ri->ri_yorigin + row * he;
	bg = ri->ri_devcmap[(attr >> 16) & 0xf];
	if (c == 0x20) {
		wcfb_rectfill(sc, x, y, wi, he, bg);
		return;
	}
	/* we wait until the blitter is idle... */
	wcfb_rop_wait(sc);
	/* ... draw the character into buffer 0 ... */
	sc->putchar(ri, row, col, c, attr);
	/* ... and then blit it into buffer 1 */
	wcfb_bitblt(sc, x, y, x, y, wi, he, WC_ROP_COPY);
}

static void
wcfb_acc_cursor(void *cookie, int on, int row, int col)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct wcfb_softc *sc = scr->scr_cookie;
	int x, y, wi, he;

	wi = ri->ri_font->fontwidth;
	he = ri->ri_font->fontheight;

	if (sc->sc_mode == WSDISPLAYIO_MODE_EMUL) {
		x = ri->ri_ccol * wi + ri->ri_xorigin;
		y = ri->ri_crow * he + ri->ri_yorigin;
		if (ri->ri_flg & RI_CURSOR) {
			wcfb_bitblt(sc, x, y, x, y, wi, he, WC_ROP_XOR);
			ri->ri_flg &= ~RI_CURSOR;
		}
		ri->ri_crow = row;
		ri->ri_ccol = col;
		if (on) {
			x = ri->ri_ccol * wi + ri->ri_xorigin;
			y = ri->ri_crow * he + ri->ri_yorigin;
			wcfb_bitblt(sc, x, y, x, y, wi, he, WC_ROP_XOR);
			ri->ri_flg |= RI_CURSOR;
		}
	} else {
		scr->scr_ri.ri_crow = row;
		scr->scr_ri.ri_ccol = col;
		scr->scr_ri.ri_flg &= ~RI_CURSOR;
	}

}

static void
wcfb_acc_copycols(void *cookie, int row, int srccol, int dstcol, int ncols)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct wcfb_softc *sc = scr->scr_cookie;
	int32_t xs, xd, y, width, height;

	if ((sc->sc_locked == 0) && (sc->sc_mode == WSDISPLAYIO_MODE_EMUL)) {
		xs = ri->ri_xorigin + ri->ri_font->fontwidth * srccol;
		xd = ri->ri_xorigin + ri->ri_font->fontwidth * dstcol;
		y = ri->ri_yorigin + ri->ri_font->fontheight * row;
		width = ri->ri_font->fontwidth * ncols;
		height = ri->ri_font->fontheight;
		wcfb_bitblt(sc, xs, y, xd, y, width, height, WC_ROP_COPY);
	}
}

static void
wcfb_acc_erasecols(void *cookie, int row, int startcol, int ncols,
		long fillattr)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct wcfb_softc *sc = scr->scr_cookie;
	int32_t x, y, width, height, fg, bg, ul;

	if ((sc->sc_locked == 0) && (sc->sc_mode == WSDISPLAYIO_MODE_EMUL)) {
		x = ri->ri_xorigin + ri->ri_font->fontwidth * startcol;
		y = ri->ri_yorigin + ri->ri_font->fontheight * row;
		width = ri->ri_font->fontwidth * ncols;
		height = ri->ri_font->fontheight;
		rasops_unpack_attr(fillattr, &fg, &bg, &ul);

		wcfb_rectfill(sc, x, y, width, height, ri->ri_devcmap[bg]);
	}
}

static void
wcfb_acc_copyrows(void *cookie, int srcrow, int dstrow, int nrows)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct wcfb_softc *sc = scr->scr_cookie;
	int32_t x, ys, yd, width, height;

	if ((sc->sc_locked == 0) && (sc->sc_mode == WSDISPLAYIO_MODE_EMUL)) {
		x = ri->ri_xorigin;
		ys = ri->ri_yorigin + ri->ri_font->fontheight * srcrow;
		yd = ri->ri_yorigin + ri->ri_font->fontheight * dstrow;
		width = ri->ri_emuwidth;
		height = ri->ri_font->fontheight * nrows;
		wcfb_bitblt(sc, x, ys, x, yd, width, height, WC_ROP_COPY);
	}
}

static void
wcfb_acc_eraserows(void *cookie, int row, int nrows, long fillattr)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct wcfb_softc *sc = scr->scr_cookie;
	int32_t x, y, width, height, fg, bg, ul;

	if ((sc->sc_locked == 0) && (sc->sc_mode == WSDISPLAYIO_MODE_EMUL)) {
		x = ri->ri_xorigin;
		y = ri->ri_yorigin + ri->ri_font->fontheight * row;
		width = ri->ri_emuwidth;
		height = ri->ri_font->fontheight * nrows;
		rasops_unpack_attr(fillattr, &fg, &bg, &ul);

		wcfb_rectfill(sc, x, y, width, height, ri->ri_devcmap[bg]);
	}
}
