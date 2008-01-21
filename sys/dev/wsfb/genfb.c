/*	$NetBSD: genfb.c,v 1.10.2.5 2008/01/21 09:44:54 yamt Exp $ */

/*-
 * Copyright (c) 2007 Michael Lorenz
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
 * 3. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: genfb.c,v 1.10.2.5 2008/01/21 09:44:54 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/mutex.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/malloc.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/rasops/rasops.h>
#include <dev/wsfont/wsfont.h>

#include <dev/wscons/wsdisplay_vconsvar.h>

#include <dev/wsfb/genfbvar.h>

#include "opt_genfb.h"
#include "opt_wsfb.h"

#ifdef GENFB_DEBUG
#define GPRINTF panic
#else
#define GPRINTF aprint_verbose
#endif

static int	genfb_ioctl(void *, void *, u_long, void *, int, struct lwp *);
static paddr_t	genfb_mmap(void *, void *, off_t, int);
static void	genfb_init_screen(void *, struct vcons_screen *, int, long *);

static int	genfb_putcmap(struct genfb_softc *, struct wsdisplay_cmap *);
static int 	genfb_getcmap(struct genfb_softc *, struct wsdisplay_cmap *);
static void	genfb_restore_palette(struct genfb_softc *);
static int 	genfb_putpalreg(struct genfb_softc *, uint8_t, uint8_t,
			    uint8_t, uint8_t);

extern const u_char rasops_cmap[768];

struct wsdisplay_accessops genfb_accessops = {
	genfb_ioctl,
	genfb_mmap,
	NULL,	/* alloc_screen */
	NULL,	/* free_screen */
	NULL,	/* show_screen */
	NULL, 	/* load_font */
	NULL,	/* pollc */
	NULL	/* scroll */
};

void
genfb_init(struct genfb_softc *sc)
{
	prop_dictionary_t dict;
	uint64_t cmap_cb;
	uint32_t fboffset;

	dict = device_properties(&sc->sc_dev);
#ifdef GENFB_DEBUG
	printf(prop_dictionary_externalize(dict));
#endif
	if (!prop_dictionary_get_uint32(dict, "width", &sc->sc_width)) {
		GPRINTF("no width property\n");
		return;
	}
	if (!prop_dictionary_get_uint32(dict, "height", &sc->sc_height)) {
		GPRINTF("no height property\n");
		return;
	}
	if (!prop_dictionary_get_uint32(dict, "depth", &sc->sc_depth)) {
		GPRINTF("no depth property\n");
		return;
	}

	/* XXX should be a 64bit value */
	if (!prop_dictionary_get_uint32(dict, "address", &fboffset)) {
		GPRINTF("no address property\n");
		return;
	}

	sc->sc_fboffset = fboffset;

	if (!prop_dictionary_get_uint32(dict, "linebytes", &sc->sc_stride))
		sc->sc_stride = (sc->sc_width * sc->sc_depth) >> 3;

	/*
	 * deal with a bug in the Raptor firmware which always sets
	 * stride = width even when depth != 8
	 */
	if (sc->sc_stride < sc->sc_width * (sc->sc_depth >> 3))
		sc->sc_stride = sc->sc_width * (sc->sc_depth >> 3);

	sc->sc_fbsize = sc->sc_height * sc->sc_stride;

	/* optional colour map callback */
	sc->sc_cmcb = NULL;
	if (prop_dictionary_get_uint64(dict, "cmap_callback", &cmap_cb)) {
		if (cmap_cb != 0)
			sc->sc_cmcb = (void *)(vaddr_t)cmap_cb;
	}
}

int
genfb_attach(struct genfb_softc *sc, struct genfb_ops *ops)
{
	struct wsemuldisplaydev_attach_args aa;
	prop_dictionary_t dict;
	struct rasops_info *ri;
	long defattr;
	int i, j;
	bool console;

	aprint_verbose("%s: framebuffer at %p, size %dx%d, depth %d, "
	    "stride %d\n", sc->sc_dev.dv_xname, sc->sc_fbaddr,
	    sc->sc_width, sc->sc_height, sc->sc_depth, sc->sc_stride);

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
	memcpy(&sc->sc_ops, ops, sizeof(struct genfb_ops));
	sc->sc_mode = WSDISPLAYIO_MODE_EMUL;

	sc->sc_shadowfb = malloc(sc->sc_fbsize, M_DEVBUF, M_WAITOK);

	dict = device_properties(&sc->sc_dev);

	prop_dictionary_get_bool(dict, "is_console", &console);

	vcons_init(&sc->vd, sc, &sc->sc_defaultscreen_descr,
	    &genfb_accessops);
	sc->vd.init_screen = genfb_init_screen;

	/* Do not print anything between this point and the screen
	 * clear operation below.  Otherwise it will be lost. */

	ri = &sc->sc_console_screen.scr_ri;

	if (console) {
		vcons_init_screen(&sc->vd, &sc->sc_console_screen, 1,
		    &defattr);
		sc->sc_console_screen.scr_flags |= VCONS_SCREEN_IS_STATIC;

		sc->sc_defaultscreen_descr.textops = &ri->ri_ops;
		sc->sc_defaultscreen_descr.capabilities = ri->ri_caps;
		sc->sc_defaultscreen_descr.nrows = ri->ri_rows;
		sc->sc_defaultscreen_descr.ncols = ri->ri_cols;
		wsdisplay_cnattach(&sc->sc_defaultscreen_descr, ri, 0, 0,
		    defattr);
	} else {
		/*
		 * since we're not the console we can postpone the rest
		 * until someone actually allocates a screen for us
		 */
		(*ri->ri_ops.allocattr)(ri, 0, 0, 0, &defattr);
	}

	/* Clear the whole screen to bring it to a known state. */
	(*ri->ri_ops.eraserows)(ri, 0, ri->ri_rows, defattr);

	j = 0;
	for (i = 0; i < (1 << sc->sc_depth); i++) {

		sc->sc_cmap_red[i] = rasops_cmap[j];
		sc->sc_cmap_green[i] = rasops_cmap[j + 1];
		sc->sc_cmap_blue[i] = rasops_cmap[j + 2];
		genfb_putpalreg(sc, i, rasops_cmap[j], rasops_cmap[j + 1],
		    rasops_cmap[j + 2]);
		j += 3;
	}

	aa.console = console;
	aa.scrdata = &sc->sc_screenlist;
	aa.accessops = &genfb_accessops;
	aa.accesscookie = &sc->vd;

	config_found(&sc->sc_dev, &aa, wsemuldisplaydevprint);

	return 0;
}

static int
genfb_ioctl(void *v, void *vs, u_long cmd, void *data, int flag,
	struct lwp *l)
{
	struct vcons_data *vd = v;
	struct genfb_softc *sc = vd->cookie;
	struct wsdisplay_fbinfo *wdf;
	struct vcons_screen *ms = vd->active;

	switch (cmd) {

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
			return genfb_getcmap(sc,
			    (struct wsdisplay_cmap *)data);

		case WSDISPLAYIO_PUTCMAP:
			return genfb_putcmap(sc,
			    (struct wsdisplay_cmap *)data);

		case WSDISPLAYIO_LINEBYTES:
			*(u_int *)data = sc->sc_stride;
			return 0;

		case WSDISPLAYIO_SMODE:
			{
				int new_mode = *(int*)data;

				/* notify the bus backend */
				if (sc->sc_ops.genfb_ioctl)
					return sc->sc_ops.genfb_ioctl(sc, vs,
					    cmd, data, flag, l);

				if (new_mode != sc->sc_mode) {
					sc->sc_mode = new_mode;
					if(new_mode == WSDISPLAYIO_MODE_EMUL) {
						genfb_restore_palette(sc);
						vcons_redraw_screen(ms);
					}
				}
			}
			return 0;
		default:
			if (sc->sc_ops.genfb_ioctl)
				return sc->sc_ops.genfb_ioctl(sc, vs, cmd,
				    data, flag, l);
	}
	return EPASSTHROUGH;
}

static paddr_t
genfb_mmap(void *v, void *vs, off_t offset, int prot)
{
	struct vcons_data *vd = v;
	struct genfb_softc *sc = vd->cookie;

	if (sc->sc_ops.genfb_mmap)
		return sc->sc_ops.genfb_mmap(sc, vs, offset, prot);

	return -1;
}

static void
genfb_init_screen(void *cookie, struct vcons_screen *scr,
    int existing, long *defattr)
{
	struct genfb_softc *sc = cookie;
	struct rasops_info *ri = &scr->scr_ri;

	ri->ri_depth = sc->sc_depth;
	ri->ri_width = sc->sc_width;
	ri->ri_height = sc->sc_height;
	ri->ri_stride = sc->sc_stride;
	ri->ri_flg = RI_CENTER | RI_FULLCLEAR;

	if (sc->sc_shadowfb != NULL) {

		ri->ri_hwbits = (char *)sc->sc_fbaddr;
		ri->ri_bits = (char *)sc->sc_shadowfb;
	} else
		ri->ri_bits = (char *)sc->sc_fbaddr;

	if (existing) {
		ri->ri_flg |= RI_CLEAR;
	}

	rasops_init(ri, sc->sc_height / 8, sc->sc_width / 8);
	ri->ri_caps = WSSCREEN_WSCOLORS;

	rasops_reconfig(ri, sc->sc_height / ri->ri_font->fontheight,
		    sc->sc_width / ri->ri_font->fontwidth);

	ri->ri_hw = scr;
}

static int
genfb_putcmap(struct genfb_softc *sc, struct wsdisplay_cmap *cm)
{
	u_char *r, *g, *b;
	u_int index = cm->index;
	u_int count = cm->count;
	int i, error;
	u_char rbuf[256], gbuf[256], bbuf[256];

#ifdef GENFB_DEBUG
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
		genfb_putpalreg(sc, index, *r, *g, *b);
		index++;
		r++, g++, b++;
	}
	return 0;
}

static int
genfb_getcmap(struct genfb_softc *sc, struct wsdisplay_cmap *cm)
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
genfb_restore_palette(struct genfb_softc *sc)
{
	int i;

	for (i = 0; i < (1 << sc->sc_depth); i++) {
		genfb_putpalreg(sc, i, sc->sc_cmap_red[i],
		    sc->sc_cmap_green[i], sc->sc_cmap_blue[i]);
	}
}

static int
genfb_putpalreg(struct genfb_softc *sc, uint8_t idx, uint8_t r, uint8_t g,
    uint8_t b)
{

	if (sc->sc_cmcb) {

		sc->sc_cmcb->gcc_set_mapreg(sc->sc_cmcb->gcc_cookie,
		    idx, r, g, b);
	}
	return 0;
}

