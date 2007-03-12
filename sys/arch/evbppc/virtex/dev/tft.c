/* 	$NetBSD: tft.c,v 1.1.8.1 2007/03/12 05:47:40 rmind Exp $ */

/*
 * Copyright (c) 2006 Jachym Holecek
 * All rights reserved.
 *
 * Written for DFC Design, s.r.o.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tft.c,v 1.1.8.1 2007/03/12 05:47:40 rmind Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/kernel.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/device.h>
#include <sys/queue.h>

#include <uvm/uvm_extern.h>

#include <machine/bus.h>

#include <dev/wscons/wsdisplayvar.h>
#include <dev/wscons/wsconsio.h>
#include <dev/wsfont/wsfont.h>
#include <dev/rasops/rasops.h>
#include <dev/splash/splash.h>
#include <dev/wscons/wsdisplay_vconsvar.h>

#include <evbppc/virtex/dev/tftreg.h>
#include <evbppc/virtex/dev/tftvar.h>


/* Template. */
static struct wsscreen_descr tft_screen = {
	.name 			= "fb",
	.fontwidth 		= 8,
	.fontheight 		= 16,
	.capabilities 		= (WSSCREEN_WSCOLORS | WSSCREEN_HILIT |
				   WSSCREEN_UNDERLINE | WSSCREEN_REVERSE),
};

static void tft_init_screen(void *, struct vcons_screen *, int, long *);


void
tft_attach(device_t self, struct wsdisplay_accessops *accessops)
{
	struct wsemuldisplaydev_attach_args waa;
	struct tft_softc 	*sc = device_private(self);
	struct rasops_info 	*ri;
	long 			defattr;

	KASSERT(accessops->mmap);

	if (accessops->ioctl == NULL)
		accessops->ioctl = tft_ioctl;

	printf("%s: %ux%ux%ub\n", device_xname(self), sc->sc_width,
	    sc->sc_height, sc->sc_bpp);
	printf("%s: video memory va %p size %uB stride %uB\n",
	    device_xname(self), sc->sc_image, sc->sc_size, sc->sc_stride);

	memset(sc->sc_image, 0xf0, sc->sc_size);

	/* Setup descr template, make up list. */
	sc->sc_ws_descr_storage[0] = tft_screen; 	/* struct copy */
	sc->sc_ws_descr = sc->sc_ws_descr_storage;
	sc->sc_ws_scrlist.nscreens = 1;
	sc->sc_ws_scrlist.screens =
	    (const struct wsscreen_descr **)&sc->sc_ws_descr;

	vcons_init(&sc->sc_vc_data, self, sc->sc_ws_descr, accessops);

	sc->sc_vc_screen.scr_flags |= VCONS_SCREEN_IS_STATIC;
	sc->sc_vc_data.init_screen = tft_init_screen;
	sc->sc_vc_data.cookie = sc;

	vcons_init_screen(&sc->sc_vc_data, &sc->sc_vc_screen, 1, &defattr);

	/* Patch descr. */
	ri = &sc->sc_vc_screen.scr_ri;
	sc->sc_ws_descr->textops = &ri->ri_ops;
	sc->sc_ws_descr->capabilities = ri->ri_caps;
	sc->sc_ws_descr->nrows = ri->ri_rows;
	sc->sc_ws_descr->ncols = ri->ri_cols;

#ifdef	SPLASHSCREEN
	sc->sc_sp_info.si_depth = ri->ri_depth;
	sc->sc_sp_info.si_bits = ri->ri_bits;
	sc->sc_sp_info.si_hwbits = ri->ri_hwbits;
	sc->sc_sp_info.si_width = ri->ri_width;
	sc->sc_sp_info.si_height = ri->ri_height;
	sc->sc_sp_info.si_stride = ri->ri_stride;
	sc->sc_sp_info.si_fillrect = NULL;

	splash_render(&sc->sc_sp_info, SPLASH_F_CENTER | SPLASH_F_FILL);
#endif

#ifdef	SPLASHSCREEN_PROGRESS
	sc->sc_sp_progress.sp_top = (sc->sc_height / 8) * 7;
	sc->sc_sp_progress.sp_width = (sc->sc_width / 4) * 3;
	sc->sc_sp_progress.sp_left = (sc->sc_width -
	    sc->sc_sp_progress.sp_width) / 2;
	sc->sc_sp_progress.sp_height = 20;
	sc->sc_sp_progress.sp_state = -1;
	sc->sc_sp_progress.sp_si = &sc->sc_sp_info;

	splash_progress_init(&sc->sc_sp_progress);
	SCREEN_DISABLE_DRAWING(&sc->sc_vc_screen);
#endif

	if (sc->sc_sdhook == NULL) {
		sc->sc_sdhook = shutdownhook_establish(tft_shutdown, self);
		if (sc->sc_sdhook == NULL)
			printf("%s: WARNING: unable to establish shutdown "
			    "hook\n", device_xname(self));
	}

	waa.console = 0; 			/* XXX */
	waa.scrdata = &sc->sc_ws_scrlist;
	waa.accessops = accessops;
	waa.accesscookie = &sc->sc_vc_data;

	config_found(self, &waa, wsemuldisplaydevprint);
}

static void
tft_init_screen(void *cookie, struct vcons_screen *scr, int existing,
    long *defattr)
{
	struct tft_softc 	*sc = cookie;
	struct rasops_info 	*ri = &scr->scr_ri;

	/* initialize font subsystem */
	wsfont_init();

	ri->ri_depth = sc->sc_bpp;
	ri->ri_width = sc->sc_width;
	ri->ri_height = sc->sc_height;
	ri->ri_stride = sc->sc_stride;
	ri->ri_flg = /*RI_CENTER |*/ RI_CLEAR;
	ri->ri_bits = (void *)sc->sc_image;
	ri->ri_caps = WSSCREEN_WSCOLORS;

	ri->ri_rnum = 8;
	ri->ri_gnum = 8;
	ri->ri_bnum = 8;
	ri->ri_rpos = 16;
	ri->ri_gpos = 8;
	ri->ri_bpos = 0;

	rasops_init(ri, ri->ri_height / 16, ri->ri_width / 8);

	/* we'd override rasops methods now if we had acceleration */
}

void
tft_shutdown(void *arg)
{
	struct tft_softc 	*sc = arg;

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, TFT_CTRL, CTRL_RESET);
}

int
tft_mode(device_t dev)
{
	struct tft_softc 		*sc = device_private(dev);
	prop_number_t 			pn;

	/* Defaults for tft core generics. */
	sc->sc_width 	= 640;
	sc->sc_height 	= 480;
	sc->sc_stride 	= 1024 * 4;
	sc->sc_bpp 	= 32; 		/* 24bit colour, not packed */

	/* We can make all these mandatory if it becomes practical... */
	pn = prop_dictionary_get(device_properties(dev), "x-resolution");
	if (pn != NULL)
		sc->sc_width = (u_int)prop_number_integer_value(pn);

	pn = prop_dictionary_get(device_properties(dev), "y-resolution");
	if (pn != NULL)
		sc->sc_height = (u_int)prop_number_integer_value(pn);

	pn = prop_dictionary_get(device_properties(dev), "stride-bytes");
	if (pn != NULL)
		sc->sc_stride = (u_int)prop_number_integer_value(pn);

	pn = prop_dictionary_get(device_properties(dev), "bits-per-pixel");
	if (pn != NULL)
		sc->sc_bpp = (u_int)prop_number_integer_value(pn);

	sc->sc_size = sc->sc_stride * sc->sc_height;
	return (0);
}

int
tft_ioctl(void *arg, void *scr, u_long cmd, void *data, int flag,
    struct lwp *l)
{
	struct vcons_data 	*vd = arg;
	struct tft_softc 	*sc = vd->cookie;
	struct wsdisplay_fbinfo *info;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:	
		*(u_int *)data = WSDISPLAY_TYPE_XILFB;
		return (0);

	case WSDISPLAYIO_GINFO:
		info = (struct wsdisplay_fbinfo *)data;

		info->height = sc->sc_height;
		info->width = sc->sc_width;
		info->depth = sc->sc_bpp;
		info->cmsize = 0;

		return (0);
	case WSDISPLAYIO_LINEBYTES:
		*(u_int *)data = sc->sc_stride;

		return (0);
	}

	return (EPASSTHROUGH);
}
