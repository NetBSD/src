/*	$NetBSD: wcfb.c,v 1.4 2010/03/09 23:17:12 macallan Exp $ */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: wcfb.c,v 1.4 2010/03/09 23:17:12 macallan Exp $");

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

#include <dev/wscons/wsdisplayvar.h>
#include <dev/wscons/wsconsio.h>
#include <dev/wsfont/wsfont.h>
#include <dev/rasops/rasops.h>
#include <dev/wscons/wsdisplay_vconsvar.h>

#include "opt_wsfb.h"
#include "opt_wcfb.h"

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

	bus_space_handle_t sc_fbh, sc_wtfh;
	bus_space_handle_t sc_regh;
	bus_addr_t sc_fb, sc_reg, sc_wtf;
	bus_size_t sc_fbsize, sc_regsize, sc_wtfsize;

	int sc_width, sc_height, sc_stride;
	int sc_locked;
	uint8_t *sc_fbaddr, *sc_fb0, *sc_fb1, *sc_shadow;
	struct vcons_screen sc_console_screen;
	struct wsscreen_descr sc_defaultscreen_descr;
	const struct wsscreen_descr *sc_screens[1];
	struct wsscreen_list sc_screenlist;
	struct vcons_data vd;
	int sc_mode;
	u_char sc_cmap_red[256];
	u_char sc_cmap_green[256];
	u_char sc_cmap_blue[256];
	uint32_t sc_fb0off;

	void (*copycols)(void *, int, int, int, int);
	void (*erasecols)(void *, int, int, int, long);
	void (*copyrows)(void *, int, int, int);
	void (*eraserows)(void *, int, int, long);
	void (*putchar)(void *, int, int, u_int, long);
	void (*cursor)(void *, int, int, int);
	
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

static void 	wcfb_putpalreg(struct wcfb_softc *, int, int, int, int);

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
	unsigned long		defattr;
	bool			is_console;
	char 			devinfo[256];
	void *wtf;

	sc->sc_dev = self;
	sc->putchar = NULL;
	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo, sizeof(devinfo));
	aprint_naive("\n");
	aprint_normal(": %s\n", devinfo);

	dict = device_properties(self);
	prop_dictionary_get_bool(dict, "is_console", &is_console);
	if(!is_console) return;

	sc->sc_memt = pa->pa_memt;
	sc->sc_iot = pa->pa_iot;	
	sc->sc_pc = pa->pa_pc;
	sc->sc_pcitag = pa->pa_tag;

	/* fill in parameters from properties */
	if (!prop_dictionary_get_uint32(dict, "width", &sc->sc_width)) {
		aprint_error("%s: no width property\n", device_xname(self));
		return;
	}
	if (!prop_dictionary_get_uint32(dict, "height", &sc->sc_height)) {
		aprint_error("%s: no height property\n", device_xname(self));
		return;
	}

	if (pci_mapreg_map(pa, 0x14, PCI_MAPREG_TYPE_MEM, 0,
	    &sc->sc_regt, &sc->sc_regh, &sc->sc_reg, &sc->sc_regsize)) {
		aprint_error("%s: failed to map registers.\n",
		    device_xname(sc->sc_dev));
	}

	if (pci_mapreg_map(pa, 0x10, PCI_MAPREG_TYPE_MEM, BUS_SPACE_MAP_LINEAR,
	    &sc->sc_memt, &sc->sc_fbh, &sc->sc_fb, &sc->sc_fbsize)) {
		aprint_error("%s: failed to map framebuffer.\n",
		    device_xname(sc->sc_dev));
	}

	if (pci_mapreg_map(pa, 0x18, PCI_MAPREG_TYPE_MEM, BUS_SPACE_MAP_LINEAR,
	    &sc->sc_wtft, &sc->sc_wtfh, &sc->sc_wtf, &sc->sc_wtfsize)) {
		aprint_error("%s: failed to map wtf.\n",
		    device_xname(sc->sc_dev));
	}
	wtf = bus_space_vaddr(sc->sc_wtft, sc->sc_wtfh);
	memset(wtf, 0, 0x100000);

	sc->sc_fbaddr = bus_space_vaddr(sc->sc_memt, sc->sc_fbh);

	sc->sc_fb0off = bus_space_read_4(sc->sc_regt, sc->sc_regh, 0x8080) -
	    sc->sc_fb;
	sc->sc_fb0 = sc->sc_fbaddr + sc->sc_fb0off;
	sc->sc_fb1 = sc->sc_fb0 + 0x200000;

	sc->sc_stride = 1 << 
	    ((bus_space_read_4(sc->sc_regt, sc->sc_regh, 0x8074) & 0x00ff0000) >> 16);
	printf("%s: %d x %d, %d\n", device_xname(sc->sc_dev), 
	    sc->sc_width, sc->sc_height, sc->sc_stride);

	sc->sc_shadow = kmem_alloc(sc->sc_stride * sc->sc_height, KM_SLEEP);
	if (sc->sc_shadow == NULL) {
		printf("%s: failed to allocate shadow buffer\n",
		    device_xname(self));
		return;
	}

	for (i = 0x40; i < 0x100; i += 16) {
		printf("%04x:", i);
		for (j = 0; j < 16; j += 4) {
			printf(" %08x", bus_space_read_4(sc->sc_regt,
			    sc->sc_regh, 0x8000 + i + j));
		}
		printf("\n");
	}

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
	wcfb_putpalreg(sc, 0, 0, 0, 0);
	wcfb_putpalreg(sc, 15, 0xff, 0xff, 0xff);

	if (is_console) {
		vcons_init_screen(&sc->vd, &sc->sc_console_screen, 1,
		    &defattr);
		sc->sc_console_screen.scr_flags |= VCONS_SCREEN_IS_STATIC;

#if 0
		wcfb_rectfill(sc, 0, 0, sc->sc_width, sc->sc_height,
		    ri->ri_devcmap[(defattr >> 16) & 0xff]);
#else
		/*memset(sc->sc_fbaddr + sc->sc_fb0off, 0, 0x400000);*/
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
	aa.accessops = &wcfb_accessops;
	aa.accesscookie = &sc->vd;

	config_found(sc->sc_dev, &aa, wsemuldisplaydevprint);
}

static int
wcfb_ioctl(void *v, void *vs, u_long cmd, void *data, int flag,
    struct lwp *l)
{
	struct wcfb_softc *sc = v;

	switch (cmd) {
		case WSDISPLAYIO_GTYPE:
			*(u_int *)data = WSDISPLAY_TYPE_PCIMISC;
			return 0;

		/* PCI config read/write passthrough. */
		case PCI_IOC_CFGREAD:
		case PCI_IOC_CFGWRITE:
			return (pci_devioctl(sc->sc_pc, sc->sc_pcitag,
			    cmd, data, flag, l));
		case WSDISPLAYIO_SMODE:
			{
				/*int new_mode = *(int*)data, i;*/
			}
			return 0;
	}

	return EPASSTHROUGH;
}

static paddr_t
wcfb_mmap(void *v, void *vs, off_t offset, int prot)
{
	struct wcfb_softc *sc = v;

	/* no point in allowing a wsfb map if we can't provide one */
	/*
	 * restrict all other mappings to processes with superuser privileges
	 * or the kernel itself
	 */
	if (kauth_authorize_generic(kauth_cred_get(), KAUTH_GENERIC_ISSUSER,
	    NULL) != 0) {
		aprint_normal_dev(sc->sc_dev, "mmap() rejected.\n");
		return -1;
	}

#ifdef WSFB_FAKE_VGA_FB
	if ((offset >= 0xa0000) && (offset < 0xbffff)) {

		return bus_space_mmap(sc->sc_memt, sc->sc_gen.sc_fboffset,
		   offset - 0xa0000, prot, BUS_SPACE_MAP_LINEAR);
	}
#endif

	/*
	 * XXX this should be generalized, let's just
	 * #define PCI_IOAREA_PADDR
	 * #define PCI_IOAREA_OFFSET
	 * #define PCI_IOAREA_SIZE
	 * somewhere in a MD header and compile this code only if all are
	 * present
	 */
	/*
	 * PCI_IOAREA_PADDR is useless, that's what the IO tag is for
	 * the address isn't guaranteed to be the same on each host bridge
	 * either, never mind the fact that it would be a bus address
	 */
#ifdef PCI_MAGIC_IO_RANGE
	/* allow to map our IO space */
	if ((offset >= PCI_MAGIC_IO_RANGE) &&
	    (offset < PCI_MAGIC_IO_RANGE + 0x10000)) {
		return bus_space_mmap(sc->sc_iot, offset - PCI_MAGIC_IO_RANGE,
		    0, prot, BUS_SPACE_MAP_LINEAR);	
	}
#endif
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

	ri->ri_bits = sc->sc_shadow;

	if (existing) {
		ri->ri_flg |= RI_CLEAR;
	}

	rasops_init(ri, sc->sc_height / 8, sc->sc_width / 8);
	ri->ri_caps = WSSCREEN_WSCOLORS;

	rasops_reconfig(ri, sc->sc_height / ri->ri_font->fontheight,
		    sc->sc_width / ri->ri_font->fontwidth);

	ri->ri_hw = scr;
	sc->putchar = ri->ri_ops.putchar;
	sc->copyrows = ri->ri_ops.copyrows;
	sc->eraserows = ri->ri_ops.eraserows;
	sc->copycols = ri->ri_ops.copycols;
	sc->erasecols = ri->ri_ops.erasecols;

	ri->ri_ops.copyrows = wcfb_copyrows;
	ri->ri_ops.copycols = wcfb_copycols;
	ri->ri_ops.eraserows = wcfb_eraserows;
	ri->ri_ops.erasecols = wcfb_erasecols;
	ri->ri_ops.putchar = wcfb_putchar;
	ri->ri_ops.cursor = wcfb_cursor;
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

	bus_space_write_4(sc->sc_regt, sc->sc_regh, 0x80bc, i);
	rgb = (b << 22) | (g << 12) | (r << 2);
	bus_space_write_4(sc->sc_regt, sc->sc_regh, 0x80c0, rgb);
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
