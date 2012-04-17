/* $NetBSD: vncfb.c,v 1.12.6.2 2012/04/17 00:06:59 yamt Exp $ */

/*-
 * Copyright (c) 2011 Jared D. McNeill <jmcneill@invisible.ca>
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by Jared D. McNeill.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
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

#include "opt_wsemul.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vncfb.c,v 1.12.6.2 2012/04/17 00:06:59 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/kmem.h>

#include <uvm/uvm_extern.h>

#include <dev/wscons/wsconsio.h>

#include <dev/wscons/wsdisplayvar.h>
#include <dev/wsfont/wsfont.h>
#include <dev/rasops/rasops.h>
#include <dev/wscons/wsdisplay_vconsvar.h>

#include <dev/wscons/wskbdvar.h>
#include <dev/wscons/wsksymdef.h>
#include <dev/wscons/wsksymvar.h>

#include <dev/wscons/wsmousevar.h>

#include <machine/mainbus.h>
#include <machine/thunk.h>

#define VNCFB_REFRESH_INTERVAL	33	/* fb refresh interval when mapped */

struct vncfb_fbops {
	void	(*copycols)(void *, int, int, int, int);
	void	(*erasecols)(void *, int, int, int, long);
	void	(*copyrows)(void *, int, int, int);
	void	(*eraserows)(void *, int, int, long);
	void	(*putchar)(void *, int, int, u_int, long);
	void	(*cursor)(void *, int, int, int);
};

struct vncfb_softc {
	device_t		sc_dev;
	device_t		sc_wskbddev;
	device_t		sc_wsmousedev;
	thunk_rfb_t		sc_rfb;
	unsigned int		sc_width;
	unsigned int		sc_height;
	unsigned int		sc_depth;
	int			sc_mode;
	uint8_t *		sc_mem;
	size_t			sc_memsize;
	uint8_t *		sc_framebuf;
	size_t			sc_framebufsize;
	struct vcons_data	sc_vd;
	struct vncfb_fbops	sc_ops;

	int			sc_kbd_enable;
	int			sc_mouse_enable;

	void			*sc_ih;
	void			*sc_sih;

	callout_t		sc_callout;
	void			*sc_refresh_sih;
};

static int	vncfb_match(device_t, cfdata_t, void *);
static void	vncfb_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(vncfb, sizeof(struct vncfb_softc),
    vncfb_match, vncfb_attach, NULL, NULL);

static void	vncfb_putchar(void *, int, int, u_int, long);
static void	vncfb_copycols(void *, int, int, int, int);
static void	vncfb_erasecols(void *, int, int, int, long);
static void	vncfb_copyrows(void *, int, int, int);
static void	vncfb_eraserows(void *, int, int, long);
static void	vncfb_cursor(void *, int, int, int);

static int	vncfb_ioctl(void *, void *, u_long, void *, int, lwp_t *);
static paddr_t	vncfb_mmap(void *, void *, off_t, int);

static void	vncfb_init_screen(void *, struct vcons_screen *, int, long *);

static void	vncfb_update(struct vncfb_softc *, int, int, int, int);
static void	vncfb_copyrect(struct vncfb_softc *, int, int, int, int, int, int);
static void	vncfb_fillrect(struct vncfb_softc *, int, int, int, int, uint32_t);
static int	vncfb_intr(void *);
static void	vncfb_softintr(void *);
static void	vncfb_refresh(void *);
static void	vncfb_softrefresh(void *);

static int	vncfb_kbd_enable(void *, int);
static void	vncfb_kbd_set_leds(void *, int);
static int	vncfb_kbd_ioctl(void *, u_long, void *, int, lwp_t *);

static void	vncfb_kbd_cngetc(void *, u_int *, int *);
static void	vncfb_kbd_cnpollc(void *, int);
static void	vncfb_kbd_bell(void *, u_int, u_int, u_int);

static int	vncfb_mouse_enable(void *);
static int	vncfb_mouse_ioctl(void *, u_long, void *, int, lwp_t *);
static void	vncfb_mouse_disable(void *);

static struct vcons_screen vncfb_console_screen;

static struct wsscreen_descr vncfb_defaultscreen = {
	.name = "default",
	.fontwidth = 8,
	.fontheight = 16,
	.capabilities = WSSCREEN_WSCOLORS | WSSCREEN_HILIT,
};

static const struct wsscreen_descr *vncfb_screens[] = {
	&vncfb_defaultscreen,
};

static struct wsscreen_list vncfb_screenlist = {
	.screens = vncfb_screens,
	.nscreens = __arraycount(vncfb_screens),
};

static struct wsdisplay_accessops vncfb_accessops = {
	.ioctl = vncfb_ioctl,
	.mmap = vncfb_mmap,
};

extern const struct wscons_keydesc vnckbd_keydesctab[];

static const struct wskbd_mapdata vncfb_keymapdata = {
	vnckbd_keydesctab,
	KB_US,
};

static struct wskbd_accessops vncfb_kbd_accessops = {
	vncfb_kbd_enable,
	vncfb_kbd_set_leds,
	vncfb_kbd_ioctl,
};

static const struct wskbd_consops vncfb_kbd_consops = {
	vncfb_kbd_cngetc,
	vncfb_kbd_cnpollc,
	vncfb_kbd_bell,
};

static const struct wsmouse_accessops vncfb_mouse_accessops = {
	vncfb_mouse_enable,
	vncfb_mouse_ioctl,
	vncfb_mouse_disable,
};

static int
vncfb_match(device_t parent, cfdata_t match, void *priv)
{
	struct thunkbus_attach_args *taa = priv;

	return taa->taa_type == THUNKBUS_TYPE_VNCFB;
}

static void
vncfb_attach(device_t parent, device_t self, void *priv)
{
	struct vncfb_softc *sc = device_private(self);
	struct thunkbus_attach_args *taa = priv;
	struct wsemuldisplaydev_attach_args waa;
	struct wskbddev_attach_args kaa;
	struct wsmousedev_attach_args maa;
	struct rasops_info *ri;
	unsigned long defattr;

	sc->sc_dev = self;
	sc->sc_width = taa->u.vnc.width;
	sc->sc_height = taa->u.vnc.height;
	sc->sc_depth = 32;
	sc->sc_mode = WSDISPLAYIO_MODE_EMUL;
#if notyet
	sc->sc_sockfd = thunk_vnc_open_socket(taa->u.vnc.port);
	if (sc->sc_sockfd == -1)
		panic("couldn't open VNC socket");
#endif

	sc->sc_framebufsize = sc->sc_width * sc->sc_height * (sc->sc_depth / 8);
	sc->sc_memsize = sc->sc_framebufsize + PAGE_SIZE;

	sc->sc_mem = kmem_zalloc(sc->sc_memsize, KM_SLEEP);
	sc->sc_framebuf = (void *)round_page((vaddr_t)sc->sc_mem);

	aprint_naive("\n");
	aprint_normal(": %ux%u %ubpp (port %u)\n",
	    sc->sc_width, sc->sc_height, sc->sc_depth, taa->u.vnc.port);

	sc->sc_rfb.width = sc->sc_width;
	sc->sc_rfb.height = sc->sc_height;
	sc->sc_rfb.depth = sc->sc_depth;
	sc->sc_rfb.framebuf = sc->sc_framebuf;
	snprintf(sc->sc_rfb.name, sizeof(sc->sc_rfb.name),
	    "NetBSD/usermode %d.%d.%d",
	    __NetBSD_Version__ / 100000000,
	    (__NetBSD_Version__ / 1000000) % 100,
	    (__NetBSD_Version__ / 100) % 100);
	if (thunk_rfb_open(&sc->sc_rfb, taa->u.vnc.port) != 0)
		panic("couldn't open rfb server");

	sc->sc_sih = softint_establish(SOFTINT_SERIAL, vncfb_softintr, sc);
	sc->sc_ih = sigio_intr_establish(vncfb_intr, sc);

	sc->sc_refresh_sih = softint_establish(SOFTINT_SERIAL,
	    vncfb_softrefresh, sc);

	callout_init(&sc->sc_callout, 0);
	callout_setfunc(&sc->sc_callout, vncfb_refresh, sc);

	vcons_init(&sc->sc_vd, sc, &vncfb_defaultscreen, &vncfb_accessops);
	sc->sc_vd.init_screen = vncfb_init_screen;

	ri = &vncfb_console_screen.scr_ri;
	vcons_init_screen(&sc->sc_vd, &vncfb_console_screen, 1, &defattr);
	vncfb_console_screen.scr_flags |= VCONS_SCREEN_IS_STATIC;
	vncfb_defaultscreen.textops = &ri->ri_ops;
	vncfb_defaultscreen.capabilities = ri->ri_caps;
	vncfb_defaultscreen.nrows = ri->ri_rows;
	vncfb_defaultscreen.ncols = ri->ri_cols;
	wsdisplay_cnattach(&vncfb_defaultscreen, ri, 0, 0, defattr);

	vcons_replay_msgbuf(&vncfb_console_screen);

	waa.console = true;
	waa.scrdata = &vncfb_screenlist;
	waa.accessops = &vncfb_accessops;
	waa.accesscookie = &sc->sc_vd;

	config_found(self, &waa, wsemuldisplaydevprint);

	wskbd_cnattach(&vncfb_kbd_consops, sc, &vncfb_keymapdata);

	kaa.console = true;
	kaa.keymap = &vncfb_keymapdata;
	kaa.accessops = &vncfb_kbd_accessops;
	kaa.accesscookie = sc;

	sc->sc_wskbddev = config_found_ia(self, "wskbddev", &kaa,
	    wskbddevprint);

	maa.accessops = &vncfb_mouse_accessops;
	maa.accesscookie = sc;

	sc->sc_wsmousedev = config_found_ia(self, "wsmousedev", &maa,
	    wsmousedevprint);
}

static void
vncfb_init_screen(void *priv, struct vcons_screen *scr, int existing,
    long *defattr)
{
	struct vncfb_softc *sc = priv;
	struct vncfb_fbops *ops = &sc->sc_ops;
	struct rasops_info *ri = &scr->scr_ri;

	ri->ri_width = sc->sc_width;
	ri->ri_height = sc->sc_height;
	ri->ri_depth = sc->sc_depth;
	ri->ri_stride = sc->sc_width * ri->ri_depth / 8;
	ri->ri_bits = sc->sc_framebuf;
	ri->ri_flg = RI_CENTER | RI_FULLCLEAR;
	if (existing)
		ri->ri_flg |= RI_CLEAR;

	rasops_init(ri, sc->sc_height / 8, sc->sc_width / 8);
	ri->ri_caps = WSSCREEN_WSCOLORS;
	rasops_reconfig(ri, sc->sc_height / ri->ri_font->fontheight,
	    sc->sc_width / ri->ri_font->fontwidth);

	ri->ri_hw = scr;

	ops->putchar = ri->ri_ops.putchar;
	ops->copyrows = ri->ri_ops.copyrows;
	ops->eraserows = ri->ri_ops.eraserows;
	ops->copycols = ri->ri_ops.copycols;
	ops->erasecols = ri->ri_ops.erasecols;
	ops->cursor = ri->ri_ops.cursor;

	ri->ri_ops.copyrows = vncfb_copyrows;
	ri->ri_ops.copycols = vncfb_copycols;
	ri->ri_ops.eraserows = vncfb_eraserows;
	ri->ri_ops.erasecols = vncfb_erasecols;
	ri->ri_ops.putchar = vncfb_putchar;
	ri->ri_ops.cursor = vncfb_cursor;
}

static void
vncfb_putchar(void *priv, int row, int col, u_int c, long attr)
{
	struct rasops_info *ri = priv;
	struct vcons_screen *scr = ri->ri_hw;
	struct vncfb_softc *sc = scr->scr_cookie;
	struct vncfb_fbops *ops = &sc->sc_ops;
	int x, y, w, h;

	ops->putchar(ri, row, col, c, attr);

	x = ri->ri_xorigin + (col * ri->ri_font->fontwidth);
	y = ri->ri_yorigin + (row * ri->ri_font->fontheight);
	w = ri->ri_font->fontwidth;
	h = ri->ri_font->fontheight;

	vncfb_update(sc, x, y, w, h);
}

static void
vncfb_copycols(void *priv, int row, int srccol, int dstcol, int ncols)
{
	struct rasops_info *ri = priv;
	struct vcons_screen *scr = ri->ri_hw;
	struct vncfb_softc *sc = scr->scr_cookie;
	struct vncfb_fbops *ops = &sc->sc_ops;
	int x, y, w, h;

	ops->copycols(ri, row, srccol, dstcol, ncols);

	y = ri->ri_yorigin + (row * ri->ri_font->fontheight);
	h = ri->ri_font->fontheight;
	if (srccol < dstcol) {
		x = ri->ri_xorigin + (srccol * ri->ri_font->fontwidth);
		w = (dstcol - srccol) * ri->ri_font->fontwidth;
		    
	} else {
		x = ri->ri_xorigin + (dstcol * ri->ri_font->fontwidth);
		w = (srccol - dstcol) * ri->ri_font->fontwidth;
	}

	vncfb_update(sc, x, y, w, h);
}

static void
vncfb_erasecols(void *priv, int row, int startcol, int ncols, long fillattr)
{
	struct rasops_info *ri = priv;
	struct vcons_screen *scr = ri->ri_hw;
	struct vncfb_softc *sc = scr->scr_cookie;
	struct vncfb_fbops *ops = &sc->sc_ops;
	int x, y, w, h, c;

	ops->erasecols(ri, row, startcol, ncols, fillattr);

	y = ri->ri_yorigin + (row * ri->ri_font->fontheight);
	h = ri->ri_font->fontheight;
	x = ri->ri_xorigin + (startcol * ri->ri_font->fontwidth);
	w = ncols * ri->ri_font->fontwidth;
	c = ri->ri_devcmap[(fillattr >> 16) & 0xf] & 0xffffff;

	vncfb_fillrect(sc, x, y, w, h, c);
}

static void
vncfb_copyrows(void *priv, int srcrow, int dstrow, int nrows)
{
	struct rasops_info *ri = priv;
	struct vcons_screen *scr = ri->ri_hw;
	struct vncfb_softc *sc = scr->scr_cookie;
	struct vncfb_fbops *ops = &sc->sc_ops;
	int x, y, w, h, srcx, srcy;
	int fontheight;

	/* barrier */
	while (sc->sc_rfb.nupdates > 0)
		if (thunk_rfb_poll(&sc->sc_rfb, NULL) == -1)
			break;

	ops->copyrows(ri, srcrow, dstrow, nrows);

	fontheight = ri->ri_font->fontheight;
	x = ri->ri_xorigin;
	y = ri->ri_yorigin + dstrow * fontheight;
	w = ri->ri_width;
	h = nrows * fontheight;

	srcx = ri->ri_xorigin;
	srcy = ri->ri_yorigin + srcrow * fontheight;

	vncfb_copyrect(sc, x, y, w, h, srcx, srcy);
}

static void
vncfb_eraserows(void *priv, int row, int nrows, long fillattr)
{
	struct rasops_info *ri = priv;
	struct vcons_screen *scr = ri->ri_hw;
	struct vncfb_softc *sc = scr->scr_cookie;
	struct vncfb_fbops *ops = &sc->sc_ops;
	int x, y, w, h, c;

	ops->eraserows(ri, row, nrows, fillattr);

	y = ri->ri_yorigin + (row * ri->ri_font->fontheight);
	h = nrows * ri->ri_font->fontheight;
	x = ri->ri_xorigin;
	w = ri->ri_width;
	c = ri->ri_devcmap[(fillattr >> 16) & 0xf] & 0xffffff;

	vncfb_fillrect(sc, x, y, w, h, c);
}

static void
vncfb_cursor(void *priv, int on, int row, int col)
{
	struct rasops_info *ri = priv;
	struct vcons_screen *scr = ri->ri_hw;
	struct vncfb_softc *sc = scr->scr_cookie;
	struct vncfb_fbops *ops = &sc->sc_ops;
	int ox, oy, x, y, w, h;

	w = ri->ri_font->fontwidth;
	h = ri->ri_font->fontheight;

	ox = ri->ri_ccol * w + ri->ri_xorigin;
	oy = ri->ri_crow * h + ri->ri_yorigin;

	ops->cursor(ri, on, row, col);

	x = ri->ri_ccol * w + ri->ri_xorigin;
	y = ri->ri_crow * h + ri->ri_yorigin;

	vncfb_update(sc, ox, oy, w, h);
	vncfb_update(sc, x, y, w, h);
}

static int
vncfb_ioctl(void *v, void *vs, u_long cmd, void *data, int flag, lwp_t *l)
{
	struct vcons_data *vd = v;
	struct vncfb_softc *sc = vd->cookie;
	struct wsdisplay_fbinfo *wdf;
	struct vcons_screen *ms = vd->active;
	int new_mode;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = WSDISPLAY_TYPE_VNC;
		return 0;
	case WSDISPLAYIO_GINFO:
		wdf = data;
		wdf->height = ms->scr_ri.ri_height;
		wdf->width = ms->scr_ri.ri_width;
		wdf->depth = ms->scr_ri.ri_depth;
		wdf->cmsize = 256;
		return 0;
	case WSDISPLAYIO_LINEBYTES:
		*(u_int *)data = sc->sc_width * (sc->sc_depth / 8);
		return 0;
	case WSDISPLAYIO_SMODE:
		new_mode = *(int *)data;
		if (sc->sc_mode != new_mode) {
			sc->sc_mode = new_mode;
			if (new_mode == WSDISPLAYIO_MODE_EMUL) {
				callout_halt(&sc->sc_callout, NULL);
				vcons_redraw_screen(ms);
			} else {
				callout_schedule(&sc->sc_callout, 1);
			}
		}
		return 0;
	default:
		return EPASSTHROUGH;
	}
}

static paddr_t
vncfb_mmap(void *v, void *vs, off_t offset, int prot)
{
	struct vcons_data *vd = v;
	struct vncfb_softc *sc = vd->cookie;
	paddr_t pa;
	vaddr_t va;

	if (offset < 0 || offset >= sc->sc_framebufsize) {
		device_printf(sc->sc_dev, "mmap: offset 0x%x, fbsize 0x%x"
		    " out of range!\n",
		    (unsigned int)offset, (unsigned int)sc->sc_framebufsize);
		return -1;
	}

	va = trunc_page((vaddr_t)sc->sc_framebuf + offset);

	if (pmap_extract(pmap_kernel(), va, &pa) == false) {
		device_printf(sc->sc_dev, "mmap: pmap_extract failed!\n");
		return -1;
	}

	return atop(pa);
}

static void
vncfb_update(struct vncfb_softc *sc, int x, int y, int w, int h)
{
	thunk_rfb_update(&sc->sc_rfb, x, y, w, h);
	softint_schedule(sc->sc_sih);
}

static void
vncfb_copyrect(struct vncfb_softc *sc, int x, int y, int w, int h,
	int srcx, int srcy)
{
	thunk_rfb_copyrect(&sc->sc_rfb, x, y, w, h, srcx, srcy);
	softint_schedule(sc->sc_sih);
}

static void
vncfb_fillrect(struct vncfb_softc *sc, int x, int y, int w, int h, uint32_t c)
{

	thunk_rfb_fillrect(&sc->sc_rfb, x, y, w, h, (uint8_t *)&c);
	softint_schedule(sc->sc_sih);
}

static int
vncfb_intr(void *priv)
{
	struct vncfb_softc *sc = priv;

	softint_schedule(sc->sc_sih);

	return 0;
}

static void
vncfb_softintr(void *priv)
{
	struct vncfb_softc *sc = priv;
	thunk_rfb_event_t event;
	int s;

	while (thunk_rfb_poll(&sc->sc_rfb, &event) > 0) {
		switch (event.message_type) {
		case THUNK_RFB_KEY_EVENT:
			s = spltty();
			wskbd_input(sc->sc_wskbddev,
			    event.data.key_event.down_flag ?
			     WSCONS_EVENT_KEY_DOWN : WSCONS_EVENT_KEY_UP,
			    event.data.key_event.keysym & 0xfff);
			splx(s);
			break;
		case THUNK_RFB_POINTER_EVENT:
			s = spltty();
			wsmouse_input(sc->sc_wsmousedev,
			    event.data.pointer_event.button_mask,
			    event.data.pointer_event.absx,
			    event.data.pointer_event.absy,
			    0, 0, 
			    WSMOUSE_INPUT_ABSOLUTE_X|WSMOUSE_INPUT_ABSOLUTE_Y);
			splx(s);
		default:
			break;
		}
	}
}

static void
vncfb_refresh(void *priv)
{
	struct vncfb_softc *sc = priv;

	softint_schedule(sc->sc_refresh_sih);
}

static void
vncfb_softrefresh(void *priv)
{
	struct vncfb_softc *sc = priv;

	if (sc->sc_mode == WSDISPLAYIO_MODE_EMUL)
		return;

	/* update the screen */
	vncfb_update(sc, 0, 0, sc->sc_width, sc->sc_height);

	/* flush the pending drawing op */
	while (thunk_rfb_poll(&sc->sc_rfb, NULL) > 0)
		;

	callout_schedule(&sc->sc_callout, mstohz(VNCFB_REFRESH_INTERVAL));
}

static int
vncfb_kbd_enable(void *priv, int on)
{
	struct vncfb_softc *sc = priv;

	sc->sc_kbd_enable = on;

	return 0;
}

static void
vncfb_kbd_set_leds(void *priv, int leds)
{
}

static int
vncfb_kbd_ioctl(void *priv, u_long cmd, void *data, int flag, lwp_t *l)
{
	struct wskbd_bell_data *bd;

	switch (cmd) {
	case WSKBDIO_GTYPE:
		*(int *)data = WSKBD_TYPE_RFB;
		return 0;
	case WSKBDIO_COMPLEXBELL:
		bd = data;
		vncfb_kbd_bell(priv, bd->pitch, bd->period, bd->volume);
		return 0;
	default:
		return EPASSTHROUGH;
	}
}

static void
vncfb_kbd_cngetc(void *priv, u_int *type, int *data)
{
	struct vncfb_softc *sc = priv;
	thunk_rfb_event_t event;

	for (;;) {
		if (thunk_rfb_poll(&sc->sc_rfb, &event) > 0) {
			if (event.message_type == THUNK_RFB_KEY_EVENT) {
				*type = event.data.key_event.down_flag ?
				    WSCONS_EVENT_KEY_DOWN : WSCONS_EVENT_KEY_UP;
				*data = event.data.key_event.keysym & 0xfff;
				return;
			}
		}
	}
}

static void
vncfb_kbd_cnpollc(void *priv, int on)
{
	struct vncfb_softc *sc = priv;

	if (!on) {
		vncfb_intr(sc);
	}
}

static void
vncfb_kbd_bell(void *priv, u_int pitch, u_int period, u_int volume)
{
	struct vncfb_softc *sc = priv;

	thunk_rfb_bell(&sc->sc_rfb);
	softint_schedule(sc->sc_sih);
}

static int
vncfb_mouse_enable(void *priv)
{
	struct vncfb_softc *sc = priv;

	sc->sc_mouse_enable = 1;

	return 0;
}

static int
vncfb_mouse_ioctl(void *priv, u_long cmd, void *data, int flag, lwp_t *l)
{
	switch (cmd) {
	case WSMOUSEIO_GTYPE:
		*(u_int *)data = WSMOUSE_TYPE_PSEUDO;
		return 0;
	default:
		return EPASSTHROUGH;
	}
}

static void
vncfb_mouse_disable(void *priv)
{
	struct vncfb_softc *sc = priv;

	sc->sc_mouse_enable = 0;
}
