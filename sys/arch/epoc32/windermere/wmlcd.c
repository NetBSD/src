/*      $NetBSD: wmlcd.c,v 1.1 2013/04/28 12:11:26 kiyohara Exp $      */
/*
 * Copyright (c) 2013 KIYOHARA Takashi
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: wmlcd.c,v 1.1 2013/04/28 12:11:26 kiyohara Exp $");

#include "rnd.h"

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/errno.h>

#include <uvm/uvm_extern.h>

#include <epoc32/windermere/windermerereg.h>
#include <epoc32/windermere/windermerevar.h>

#include <dev/cons.h>
#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/rasops/rasops.h>

#include "locators.h"

#define LCD_SIZE	0x100

static int is_console;
struct wmlcd_softc {
	device_t sc_dev;

	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;

	vaddr_t sc_buffer;

	struct rasops_info sc_ri;
};

static int wmlcd_match(device_t, cfdata_t, void *);
static void wmlcd_attach(device_t, device_t, void *);

/* wsdisplay functions */
static int wmlcd_ioctl(void *, void *, u_long, void *, int, struct lwp *);
static paddr_t wmlcd_mmap(void *, void *, off_t, int);
static int wmlcd_alloc_screen(void *, const struct wsscreen_descr *, void **,
			      int *, int *, long *);
static void wmlcd_free_screen(void *, void *);
static int wmlcd_show_screen(void *, void *, int,
			     void (*)(void *, int, int), void *);

CFATTACH_DECL_NEW(wmlcd, sizeof(struct wmlcd_softc),
    wmlcd_match, wmlcd_attach, NULL, NULL);


#define WMLCD_DEFAULT_DEPTH	4
/* Linux like palette data */
const static char palette_2bpp[] = {
	0x00, 0x00, 0x05, 0x00, 0x0a, 0x00, 0x0f, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
const static char palette_4bpp[] = {
	0x00, 0x00, 0x01, 0x00, 0x06, 0x00, 0x07, 0x00,
	0x03, 0x00, 0x04, 0x00, 0x06, 0x00, 0x0a, 0x00,
	0x05, 0x00, 0x06, 0x00, 0x0b, 0x00, 0x0c, 0x00,
	0x08, 0x00, 0x09, 0x00, 0x0e, 0x00, 0x0f, 0x00,
};

static struct wsscreen_descr wmlcd_descr = {
	.name = "wmlcd",
	.fontwidth = 8,
	.fontheight = 16,
	.capabilities = WSSCREEN_WSCOLORS,
};
static const struct wsscreen_descr *wmlcd_descrs[] = {
	&wmlcd_descr
};

static const struct wsscreen_list wmlcd_screen_list = {
	.nscreens = __arraycount(wmlcd_descrs),
	.screens = wmlcd_descrs,
};

struct wsdisplay_accessops wmlcd_accessops = {
	wmlcd_ioctl,
	wmlcd_mmap,
	wmlcd_alloc_screen,
	wmlcd_free_screen,
	wmlcd_show_screen,
	NULL,
	NULL,
	NULL,
};

/* ARGSUSED */
static int
wmlcd_match(device_t parent, cfdata_t match, void *aux)
{
	struct windermere_attach_args *aa = aux;

	/* Wildcard not accept */
	if (aa->aa_offset == WINDERMERECF_OFFSET_DEFAULT)
		return 0;

	aa->aa_size = LCD_SIZE;
	return 1;
}

/* ARGSUSED */
static void
wmlcd_attach(device_t parent, device_t self, void *aux)
{
	struct wmlcd_softc *sc = device_private(self);
	struct windermere_attach_args *aa = aux;
	struct wsemuldisplaydev_attach_args waa;
	prop_dictionary_t dict = device_properties(self);
	paddr_t paddr;
	struct rasops_info *ri;
	uint32_t lcdctl, width, height, addr, depth;
	int c, i;
	const char *palette;

	aprint_naive("\n");
	aprint_normal("\n");

	sc->sc_dev = self;
	if (windermere_bus_space_subregion(aa->aa_iot, *aa->aa_ioh,
				aa->aa_offset, aa->aa_size, &sc->sc_ioh) != 0) {
		aprint_error_dev(self, "can't map registers\n");
		return;
	}
	sc->sc_iot = aa->aa_iot;

	if (!prop_dictionary_get_uint32(dict, "width", &width) ||
	    !prop_dictionary_get_uint32(dict, "height", &height) ||
	    !prop_dictionary_get_uint32(dict, "addr", &addr)) {
		aprint_error_dev(self, "can't get properties\n");
		return;
	}
	sc->sc_buffer = addr;
	pmap_extract(pmap_kernel(), addr, &paddr);

	/* Setup palette data */
	depth = WMLCD_DEFAULT_DEPTH;
	if (depth == 2)
		palette = palette_2bpp;
	else
		palette = palette_4bpp;
	for (i = 0; i < LCD_PALETTE_SIZE; i++)
		*((uint8_t *)addr + i) = palette[i];
	*(uint16_t *)addr |= (depth >> 1) << 12;

	lcdctl = bus_space_read_4(sc->sc_iot, sc->sc_ioh, LCDCTL);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, LCDCTL, lcdctl & ~LCDCTL_EN);

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, LCDDBAR1, paddr);

	lcdctl = bus_space_read_4(sc->sc_iot, sc->sc_ioh, LCDCTL);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, LCDCTL,
	    lcdctl | LCDCTL_EN | LCDCTL_BW);

	aprint_normal_dev(self,
	    ": %dx%d pixels, %d bpp mono\n", width, height, 1 << depth);

	ri = &sc->sc_ri;
	ri->ri_depth = depth;
	ri->ri_bits = (void *)(addr + LCD_PALETTE_SIZE);
	ri->ri_width = width;
	ri->ri_height = height;
	ri->ri_stride = width * ri->ri_depth / 8/*bits*/;
	ri->ri_flg = RI_FORCEMONO | RI_CLEAR | RI_CENTER;

	if (is_console) {
		long defattr;

		if (rasops_init(ri, 0, 0) < 0)
			panic("rasops_init failed");

		if (ri->ri_depth == 4) {
			/* XXXXX: Create color map. */
			ri->ri_devcmap[0] = 0;
			for (i = 1; i < 15; i++) {
				c = (i + 0xc) & 0xf;
				ri->ri_devcmap[i] =
				    c | (c << 8) | (c << 16) | (c << 24);
			}
		}
		ri->ri_devcmap[15] = -1;

		wmlcd_descr.ncols = ri->ri_cols;
		wmlcd_descr.nrows = ri->ri_rows;
		wmlcd_descr.textops = &ri->ri_ops;
		wmlcd_descr.capabilities = ri->ri_caps;

		if ((ri->ri_ops.allocattr)(ri, 0, 0, 0, &defattr) != 0)
			panic("allocattr failed");
		wsdisplay_cnattach(&wmlcd_descr, ri, ri->ri_ccol, ri->ri_crow,
		    defattr);
	}

	waa.console = is_console;
	waa.scrdata = &wmlcd_screen_list;
	waa.accessops = &wmlcd_accessops;
	waa.accesscookie = sc;

	config_found(self, &waa, wsemuldisplaydevprint);
}

static int
wmlcd_ioctl(void *v, void *vs, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct wmlcd_softc *sc = v;
	struct wsdisplay_fbinfo *wsdisp_info;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(int *)data = WSDISPLAY_TYPE_WINDERMERE;
		return 0;

	case WSDISPLAYIO_GINFO:
		wsdisp_info = (struct wsdisplay_fbinfo *)data;
		wsdisp_info->height = sc->sc_ri.ri_height;
		wsdisp_info->width = sc->sc_ri.ri_width;
		wsdisp_info->depth = sc->sc_ri.ri_depth;
		wsdisp_info->cmsize = 0;
		return 0;

	case WSDISPLAYIO_GVIDEO:
		if (1)	/* XXXX */
			*(int *)data = WSDISPLAYIO_VIDEO_ON;
		else
			*(int *)data = WSDISPLAYIO_VIDEO_OFF;
		return 0;

	case WSDISPLAYIO_SVIDEO:
		if (*(int *)data == WSDISPLAYIO_VIDEO_ON) {
			/* XXXX: turn on */
		} else {
			/* XXXX: turn off */
		}
		return 0;

	case WSDISPLAYIO_LINEBYTES:
		*(int *)data = sc->sc_ri.ri_stride;
		return 0;
	}

	return EPASSTHROUGH;
}

static paddr_t
wmlcd_mmap(void *v, void *vs, off_t off, int prot)
{
	struct wmlcd_softc *sc = v;

	if (off < 0 || sc->sc_ri.ri_stride * sc->sc_ri.ri_height <= off)
		return -1;

	return (paddr_t)sc->sc_ri.ri_bits + off;
}

static int
wmlcd_alloc_screen(void *v, const struct wsscreen_descr *scr, void **cookiep,
		   int *curxp, int *curyp, long *attrp)
{
printf("%s\n", __func__);
return -1;
}

static void
wmlcd_free_screen(void *v, void *cookie)
{
printf("%s\n", __func__);
}

static int
wmlcd_show_screen(void *v, void *cookie, int waitok,
		  void (*func)(void *, int, int), void *arg)
{
printf("%s\n", __func__);
return -1;
}

int
wmlcd_cnattach(void)
{

	is_console = 1;
	return 0;
}
