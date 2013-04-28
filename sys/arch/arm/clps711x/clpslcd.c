/*      $NetBSD: clpslcd.c,v 1.1 2013/04/28 11:57:13 kiyohara Exp $      */
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
__KERNEL_RCSID(0, "$NetBSD: clpslcd.c,v 1.1 2013/04/28 11:57:13 kiyohara Exp $");

#include "rnd.h"

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/errno.h>

#include <uvm/uvm_extern.h>

#include <arm/clps711x/clps711xreg.h>
#include <arm/clps711x/clpssocvar.h>

#include <dev/cons.h>
#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/rasops/rasops.h>

#include "locators.h"

#define CLPSLCD_DEFAULT_DEPTH	4

static int is_console;
struct clpslcd_softc {
	device_t sc_dev;

	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;

	vaddr_t sc_buffer;

	struct rasops_info sc_ri;
};

static int clpslcd_match(device_t, cfdata_t, void *);
static void clpslcd_attach(device_t, device_t, void *);

/* wsdisplay functions */
static int clpslcd_ioctl(void *, void *, u_long, void *, int, struct lwp *);
static paddr_t clpslcd_mmap(void *, void *, off_t, int);
static int clpslcd_alloc_screen(void *, const struct wsscreen_descr *, void **,
				int *, int *, long *);
static void clpslcd_free_screen(void *, void *);
static int clpslcd_show_screen(void *, void *, int,
			       void (*)(void *, int, int), void *);

CFATTACH_DECL_NEW(clpslcd, sizeof(struct clpslcd_softc),
    clpslcd_match, clpslcd_attach, NULL, NULL);


static struct wsscreen_descr clpslcd_descr = {
	.name = "clpslcd",
	.fontwidth = 8,
	.fontheight = 16,
	.capabilities = WSSCREEN_WSCOLORS,
};
static const struct wsscreen_descr *clpslcd_descrs[] = {
	&clpslcd_descr
};

static const struct wsscreen_list clpslcd_screen_list = {
	.nscreens = __arraycount(clpslcd_descrs),
	.screens = clpslcd_descrs,
};

struct wsdisplay_accessops clpslcd_accessops = {
	clpslcd_ioctl,
	clpslcd_mmap,
	clpslcd_alloc_screen,
	clpslcd_free_screen,
	clpslcd_show_screen,
	NULL,
	NULL,
	NULL,
};

/* ARGSUSED */
static int
clpslcd_match(device_t parent, cfdata_t match, void *aux)
{

	return 1;
}

/* ARGSUSED */
static void
clpslcd_attach(device_t parent, device_t self, void *aux)
{
	struct clpslcd_softc *sc = device_private(self);
	struct clpssoc_attach_args *aa = aux;
	struct wsemuldisplaydev_attach_args waa;
	prop_dictionary_t dict = device_properties(self);
	struct rasops_info *ri;
	uint32_t syscon, lcdcon, depth, width, height, addr;
	int i;

	aprint_naive("\n");
	aprint_normal("\n");

	sc->sc_dev = self;
	sc->sc_iot = aa->aa_iot;
	sc->sc_ioh = *aa->aa_ioh;

	if (!prop_dictionary_get_uint32(dict, "width", &width) ||
	    !prop_dictionary_get_uint32(dict, "height", &height) ||
	    !prop_dictionary_get_uint32(dict, "addr", &addr)) {
		aprint_error_dev(self, "can't get properties\n");
		return;
	}
	KASSERT(addr == 0xc0000000);

	sc->sc_buffer = addr;

	depth = CLPSLCD_DEFAULT_DEPTH;
	lcdcon =
	    LCDCON_GSEN |
	    LCDCON_ACP(13) |
	    LCDCON_PP(width * height) |
	    LCDCON_LL(width) |
	    LCDCON_VBS(width * height * depth);
	if (depth == 4)
		lcdcon |= LCDCON_GSMD;

	syscon = bus_space_read_4(sc->sc_iot, sc->sc_ioh, PS711X_SYSCON);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, PS711X_SYSCON,
	    syscon | SYSCON_LCDEN);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, PS711X_LCDCON, lcdcon);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, PS711X_PALLSW, 0x54321fc0);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, PS711X_PALMSW, 0xedba9876);

	aprint_normal_dev(self,
	    ": %dx%d pixels, %d bpp mono\n", width, height, depth);

	ri = &sc->sc_ri;
	ri->ri_depth = depth;
	ri->ri_bits = (void *)addr;
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
				ri->ri_devcmap[i] =
				    i | (i << 8) | (i << 16) | (i << 24);
			}
		}
		ri->ri_devcmap[15] = -1;

		clpslcd_descr.ncols = ri->ri_cols;
		clpslcd_descr.nrows = ri->ri_rows;
		clpslcd_descr.textops = &ri->ri_ops;
		clpslcd_descr.capabilities = ri->ri_caps;

		if ((ri->ri_ops.allocattr)(ri, 0, 0, 0, &defattr) != 0)
			panic("allocattr failed");
		wsdisplay_cnattach(&clpslcd_descr, ri, ri->ri_ccol, ri->ri_crow,
		    defattr);
	}

	waa.console = is_console;
	waa.scrdata = &clpslcd_screen_list;
	waa.accessops = &clpslcd_accessops;
	waa.accesscookie = sc;

	config_found(self, &waa, wsemuldisplaydevprint);
}

static int
clpslcd_ioctl(void *v, void *vs, u_long cmd, void *data, int flag,
	      struct lwp *l)
{
	struct clpslcd_softc *sc = v;
	struct wsdisplay_fbinfo *wsdisp_info;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(int *)data = WSDISPLAY_TYPE_CLPS711X;
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
clpslcd_mmap(void *v, void *vs, off_t off, int prot)
{
	struct clpslcd_softc *sc = v;

	if (off < 0 || sc->sc_ri.ri_stride * sc->sc_ri.ri_height <= off)
		return -1;

	return (paddr_t)sc->sc_ri.ri_bits + off;
}

static int
clpslcd_alloc_screen(void *v, const struct wsscreen_descr *scr, void **cookiep,
		   int *curxp, int *curyp, long *attrp)
{
printf("%s\n", __func__);
return -1;
}

static void
clpslcd_free_screen(void *v, void *cookie)
{
printf("%s\n", __func__);
}

static int
clpslcd_show_screen(void *v, void *cookie, int waitok,
		  void (*func)(void *, int, int), void *arg)
{
printf("%s\n", __func__);
return -1;
}

int
clpslcd_cnattach(void)
{

	is_console = 1;
	return 0;
}
