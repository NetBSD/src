/* $NetBSD: vesafb.c,v 1.22.12.1 2007/08/03 22:17:06 jmcneill Exp $ */

/*-
 * Copyright (c) 2006 Jared D. McNeill <jmcneill@invisible.ca>
 * All rights reserved.
 *
 * Hardware scrolling added in 2007 by Reinoud Zandijk <reinoud@NetBSD.org>
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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


#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vesafb.c,v 1.22.12.1 2007/08/03 22:17:06 jmcneill Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <uvm/uvm_extern.h>

#include <machine/frame.h>
#include <machine/kvm86.h>
#include <machine/bus.h>

#if notyet
#include <machine/bioscall.h>
#endif

#include "opt_vesafb.h"

#include <arch/i386/bios/vesabios.h>
#include <arch/i386/bios/vesabiosreg.h>
#include <arch/i386/bios/vesafbvar.h>

MALLOC_DEFINE(M_VESAFB, "vesafb", "vesafb shadow framebuffer");

static int vesafb_match(struct device *, struct cfdata *, void *);
static void vesafb_attach(struct device *, struct device *, void *);

struct wsscreen_descr vesafb_stdscreen = {
	"fb",
	0, 0,
	NULL,
	8, 16,
	0,	/* capabilities */
	NULL,	/* modecookie */
};

static int	vesafb_ioctl(void *, void *, u_long, void *, int,
		    struct lwp *);
static paddr_t	vesafb_mmap(void *, void *, off_t, int);
static void	vesafb_show_screen_cb(struct vcons_screen *);
static void	vesafb_init_screen(void *, struct vcons_screen *,
					int, long *);

static void	vesafb_init(struct vesafb_softc *, int);
static pnp_status_t	vesafb_power(device_t, pnp_request_t, void *);

static int	vesafb_svideo(struct vesafb_softc *, int);
static int	vesafb_gvideo(struct vesafb_softc *, u_int *);
static int	vesafb_putcmap(struct vesafb_softc *,
				    struct wsdisplay_cmap *);
static int	vesafb_getcmap(struct vesafb_softc *,
				    struct wsdisplay_cmap *);

static void	vesafb_set_palette(struct vesafb_softc *, int, struct paletteentry);

struct wsdisplay_accessops vesafb_accessops = {
	vesafb_ioctl,
	vesafb_mmap,
	NULL,	/* alloc_screen */
	NULL,	/* free_screen */
	NULL,	/* show_screen */
	NULL,	/* load_font */
	NULL,	/* pollc */
	NULL,	/* scroll */
};

static struct vcons_screen vesafb_console_screen;

const struct wsscreen_descr *_vesafb_scrlist[] = {
	&vesafb_stdscreen,
};

struct wsscreen_list vesafb_screenlist = {
	sizeof(_vesafb_scrlist) / sizeof(struct wsscreen_descr *),
	_vesafb_scrlist
};

CFATTACH_DECL(vesafb, sizeof(struct vesafb_softc),
    vesafb_match, vesafb_attach, NULL, NULL);

static int
vesafb_match(struct device *parent, struct cfdata *match,
	void *aux)
{
	struct vesabiosdev_attach_args *vaa = aux;

	if (strcmp(vaa->vbaa_type, "raster"))
		return (0);

	return (1);
}

static void
vesafb_attach(struct device *parent, struct device *dev, void *aux)
{
	struct vesafb_softc *sc = (struct vesafb_softc *)dev;
	struct vesabiosdev_attach_args *vaa = aux;
	unsigned char *buf;
	struct trapframe tf;
	int res, i, j;
	long defattr;
	struct modeinfoblock *mi;
	struct rasops_info *ri;
	struct wsemuldisplaydev_attach_args aa;
	bus_space_handle_t h;
	pnp_status_t stat;

	aprint_naive("\n");
	aprint_normal(": VESA frame buffer\n");

	buf = kvm86_bios_addpage(0x2000);
	if (!buf) {
		aprint_error("%s: kvm86_bios_addpage(0x2000) failed\n",
		    sc->sc_dev.dv_xname);
		return;
	}
	sc->sc_buf = buf;

	if (vaa->vbaa_nmodes == 0)
		goto out;

	sc->sc_nscreens = 1; /* XXX console */
	sc->sc_wsmode = WSDISPLAYIO_MODE_EMUL;
	sc->sc_mode = vaa->vbaa_modes[0]; /* XXX */
	sc->sc_pm = 0;
	sc->sc_pmstates = 0;
	sc->sc_isconsole = 0;
	mi = NULL;
	j = 0;

	/* XXX */
	for (i = 0; i < vaa->vbaa_nmodes; i++) {
		memset(&tf, 0, sizeof(struct trapframe));
		tf.tf_eax = 0x4f01; /* function code */
		tf.tf_ecx = vaa->vbaa_modes[i];
		tf.tf_vm86_es = 0;
		tf.tf_edi = 0x2000; /* buf ptr */

		res = kvm86_bioscall(0x10, &tf);
		if (res || (tf.tf_eax & 0xff) != 0x4f) {
			aprint_error("%s: vbecall: res=%d, ax=%x\n",
			    sc->sc_dev.dv_xname, res, tf.tf_eax);
			goto out;
		}
		mi = (struct modeinfoblock *)buf;

		if (mi->XResolution == VESAFB_WIDTH &&
		    mi->YResolution == VESAFB_HEIGHT &&
		    mi->BitsPerPixel == VESAFB_DEPTH) {
			sc->sc_mode = vaa->vbaa_modes[i];
			break;
		}
	}

	if (i == vaa->vbaa_nmodes) {
		aprint_error("%s: no supported mode found\n",
		    sc->sc_dev.dv_xname);
		goto out;
	}

	sc->sc_mi = *mi;

	/* Check for power management support */
	memset(&tf, 0, sizeof(struct trapframe));
	tf.tf_eax = 0x4f10; /* function code */
	tf.tf_ebx = 0;
	tf.tf_edi = 0x2000; /* buf ptr */

	res = kvm86_bioscall(0x10, &tf);
	if (res || (tf.tf_eax & 0xff) != 0x4f)
		sc->sc_pm = 0; /* power management not supported */
	else {
		sc->sc_pm = 1; /* power management is supported */
		sc->sc_pmver = tf.tf_ebx & 0xff;
		sc->sc_pmstates = (tf.tf_ebx >> 8);
	}

	ri = &vesafb_console_screen.scr_ri;
	memset(ri, 0, sizeof(struct rasops_info));

	vcons_init(&sc->sc_vd, sc, &vesafb_stdscreen,
	    &vesafb_accessops);
	sc->sc_vd.init_screen = vesafb_init_screen;
	sc->sc_vd.show_screen_cb = vesafb_show_screen_cb;

	aprint_normal("%s: fb %dx%dx%d @0x%x\n", sc->sc_dev.dv_xname,
	       mi->XResolution, mi->YResolution,
	       mi->BitsPerPixel, mi->PhysBasePtr);

	if (vaa->vbaa_vbeversion >= 0x300) {
		sc->sc_scrollscreens = mi->LinNumberOfImagePages;
	} else {
		sc->sc_scrollscreens = mi->NumberOfImagePages;
	}
	if (sc->sc_scrollscreens == 0)
		sc->sc_scrollscreens = 1;

	/* XXX disable hardware scrolling for now; kvm86_call() can trap */
	sc->sc_scrollscreens = 1;

	sc->sc_screensize = mi->YResolution * mi->BytesPerScanLine;
	sc->sc_fbsize = sc->sc_scrollscreens * sc->sc_screensize;

	aprint_normal("%s: %d Kb memory reported, %d screens possible\n",
	       sc->sc_dev.dv_xname,
	       sc->sc_fbsize / 1024,
	       sc->sc_scrollscreens);

	if (sc->sc_scrollscreens == 1)
		aprint_normal("%s: one screen, so hardware scrolling not "
			"possible\n", sc->sc_dev.dv_xname);

	if (sc->sc_pm) {
		aprint_normal("%s: VBE/PM %d.%d\n", sc->sc_dev.dv_xname,
		    (sc->sc_pmver >> 4), sc->sc_pmver & 0xf);
	}

	res = _x86_memio_map(X86_BUS_SPACE_MEM, mi->PhysBasePtr,
			      sc->sc_fbsize,		/* was sc_screensize */
			      BUS_SPACE_MAP_LINEAR | BUS_SPACE_MAP_PREFETCHABLE,
			      &h);
	if (res) {
		aprint_error("%s: framebuffer mapping failed\n",
		    sc->sc_dev.dv_xname);
		goto out;
	}
	sc->sc_fbstart = bus_space_vaddr(X86_BUS_SPACE_MEM, h);
	sc->sc_bits = sc->sc_fbstart;

#ifdef VESAFB_SHADOW_FB
	sc->sc_shadowbits = malloc(sc->sc_screensize,
				   M_VESAFB, M_NOWAIT);
	if (sc->sc_shadowbits == NULL) {
		aprint_error("%s: unable to allocate %d bytes for shadowfb\n",
		    sc->sc_dev.dv_xname, sc->sc_screensize);
		/* Not fatal; we'll just have to continue without shadowfb */
	}
#endif

	/* initialise our display wide settings */
#ifdef VESAFB_SHADOW_FB
	sc->sc_displ_bits   = sc->sc_shadowbits;
	sc->sc_displ_hwbits = sc->sc_bits;
#else
	sc->sc_displ_bits   = sc->sc_bits;
	sc->sc_displ_hwbits = NULL;
#endif /* !VESAFB_SHADOW_FB */

	vesafb_init(sc, 1);

	vesafb_console_screen.scr_flags |= VCONS_SCREEN_IS_STATIC;
	vcons_init_screen(&sc->sc_vd, &vesafb_console_screen, 1, &defattr);
#ifndef SPLASHSCREEN
	vcons_redraw_screen(&vesafb_console_screen);
#endif

	vesafb_stdscreen.ncols = ri->ri_cols;
	vesafb_stdscreen.nrows = ri->ri_rows;
	vesafb_stdscreen.textops = &ri->ri_ops;
	vesafb_stdscreen.capabilities = ri->ri_caps;
	vesafb_stdscreen.modecookie = NULL;

	wsdisplay_cnattach(&vesafb_stdscreen, ri, 0, 0, defattr);

#ifdef SPLASHSCREEN
	sc->sc_si.si_depth = ri->ri_depth;
	sc->sc_si.si_bits = ri->ri_bits;
	sc->sc_si.si_hwbits = ri->ri_hwbits;
	sc->sc_si.si_width = ri->ri_width;
	sc->sc_si.si_height = ri->ri_height;
	sc->sc_si.si_stride = ri->ri_stride;
	sc->sc_si.si_fillrect = NULL;
	splash_render(&sc->sc_si, SPLASH_F_CENTER|SPLASH_F_FILL);
#endif

#ifdef SPLASHSCREEN_PROGRESS
	sc->sc_sp.sp_top = (mi->YResolution / 8) * 7;
	sc->sc_sp.sp_width = (mi->XResolution / 4) * 3;
	sc->sc_sp.sp_left = (mi->XResolution - sc->sc_sp.sp_width) / 2;
	sc->sc_sp.sp_height = 20;
	sc->sc_sp.sp_state = -1;
	sc->sc_sp.sp_si = &sc->sc_si;
	splash_progress_init(&sc->sc_sp);
#endif

	aa.console = 1; /* XXX */
	aa.scrdata = &vesafb_screenlist;
	aa.accessops = &vesafb_accessops;
	aa.accesscookie = &sc->sc_vd;

#ifdef VESAFB_DISABLE_TEXT
	SCREEN_DISABLE_DRAWING(&vesafb_console_screen);
#endif /* !VESAFB_DISABLE_TEXT */

	sc->sc_isconsole = 1;

	stat = pnp_register(dev, vesafb_power);
	if (stat != PNP_STATUS_SUCCESS)
		aprint_error("%s: unable to establish power handler\n",
		    sc->sc_dev.dv_xname);

	config_found(dev, &aa, wsemuldisplaydevprint);

out:
	return;
}

static int
vesafb_ioctl(void *v, void *vs, u_long cmd, void *data,
    int flag, struct lwp *l)
{
	struct vcons_data *vd;
	struct vesafb_softc *sc;
	struct wsdisplay_fbinfo *fb;

	vd = (struct vcons_data *)v;
	sc = (struct vesafb_softc *)vd->cookie;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = WSDISPLAY_TYPE_VESA;
		return 0;
	case WSDISPLAYIO_GINFO:
		if (vd->active != NULL) {
			fb = (struct wsdisplay_fbinfo *)data;
			fb->width = sc->sc_mi.XResolution;
			fb->height = sc->sc_mi.YResolution;
			fb->depth = sc->sc_mi.BitsPerPixel;
			fb->cmsize = 256;
			return 0;
		} else
			return ENODEV;
	case WSDISPLAYIO_GVIDEO:
		if (sc->sc_pm)
			return vesafb_gvideo(sc, (u_int *)data);
		else
			return ENODEV;
	case WSDISPLAYIO_SVIDEO:
		if (sc->sc_pm) {
			u_int on = *(u_int *)data;
			if (on == WSDISPLAYIO_VIDEO_OFF)
				return vesafb_svideo(sc, 4);
			else
				return vesafb_svideo(sc, 0);
		} else
			return ENODEV;
	case WSDISPLAYIO_GETCMAP:
		if (sc->sc_mi.BitsPerPixel == 8)
			return vesafb_getcmap(sc,
			    (struct wsdisplay_cmap *)data);
		else
			return EINVAL;
	case WSDISPLAYIO_PUTCMAP:
		if (sc->sc_mi.BitsPerPixel == 8)
			return vesafb_putcmap(sc,
			    (struct wsdisplay_cmap *)data);
		else
			return EINVAL;
	case WSDISPLAYIO_LINEBYTES:
		*(u_int *)data = sc->sc_mi.BytesPerScanLine;
		return 0;
	case WSDISPLAYIO_SMODE:
		{
			int new_mode = *(int *)data;
			if (new_mode != sc->sc_wsmode) {
				sc->sc_wsmode = new_mode;
				if (new_mode == WSDISPLAYIO_MODE_EMUL) {
					/* vesafb_init(sc, 0); */
					vcons_redraw_screen(vd->active);
				}
			}
		}
		return 0;
	case WSDISPLAYIO_SSPLASH:
#if defined(SPLASHSCREEN)
		if (*(int *)data == 1) {
			SCREEN_DISABLE_DRAWING(&vesafb_console_screen);
			splash_render(&sc->sc_si, SPLASH_F_CENTER|SPLASH_F_FILL);
		} else
			SCREEN_ENABLE_DRAWING(&vesafb_console_screen);
		return 0;
#else
		return ENODEV;
#endif
	case WSDISPLAYIO_SPROGRESS:
#if defined(SPLASHSCREEN) && defined(SPLASHSCREEN_PROGRESS)
		sc->sc_sp.sp_force = 1;
		splash_progress_update(&sc->sc_sp);
		sc->sc_sp.sp_force = 0;
		return 0;
#else
		return ENODEV;
#endif
	}

	return EPASSTHROUGH;
}

static paddr_t
vesafb_mmap(void *v, void *vs, off_t offset, int prot)
{
	struct vcons_data *vd;
	struct vesafb_softc *sc;
	paddr_t pa;

	vd = (struct vcons_data *)v;
	sc = (struct vesafb_softc *)vd->cookie;

	/* XXX */
	if (offset >= 0 && offset <
	    sc->sc_mi.YResolution*sc->sc_mi.BytesPerScanLine) {
		pa = x86_memio_mmap(X86_BUS_SPACE_MEM, sc->sc_mi.PhysBasePtr,
		    offset, prot, BUS_SPACE_MAP_LINEAR);
		return pa;
	}

	return -1;
}

/* called back by vcons on screen change; needed for VT display sharing */
static void
vesafb_show_screen_cb(struct vcons_screen *scr)
{
	struct vesafb_softc *sc;
	struct rasops_info *ri;

	sc = (struct vesafb_softc *) scr->scr_cookie;
	ri = &scr->scr_ri;

	/* protect against roque values) */
	if ((sc == NULL) || (ri == NULL))
		return;

	/* set our rasops info to match the display's global state (VTs!) */
	ri->ri_bits   = sc->sc_displ_bits;
	ri->ri_hwbits = sc->sc_displ_hwbits;
}

static void
vv_copyrows(void *id, int srcrow, int dstrow, int nrows)
{
	static int working = 1;
	struct trapframe tf;
	struct rasops_info *ri = id;
	struct vcons_screen *scr = (struct vcons_screen *) ri->ri_hw;
	struct vesafb_softc *sc  = (struct vesafb_softc *) scr->scr_cookie;
	uint32_t displ_offset;
	uint8_t *src, *dst, *hwbits, *cur_hwbits, *hiwater, *lowater;
	int fontheight, offset, linesz, size, height;
	int scrollup, scrolldown, res;

	/* set our rasops info to match the display's global state (VTs!) */
	ri->ri_bits   = sc->sc_displ_bits;
	ri->ri_hwbits = sc->sc_displ_hwbits;

	/* fontheight = ri->ri_font->fontheight; */
	fontheight = 16;

	/* All movements are done in multiples of character heights */
	height = fontheight * nrows;
	size   = height * ri->ri_stride;
	linesz = fontheight * ri->ri_stride;
	offset = (srcrow - dstrow) * linesz;

	/* check if we are full screen scrolling */
	scrollup   = (srcrow + nrows >= ri->ri_rows);
	scrolldown = (dstrow + nrows >= ri->ri_rows);

	if (working && (scrollup || scrolldown))  {
		lowater = sc->sc_fbstart;
		hiwater = lowater + sc->sc_fbsize - sc->sc_screensize;
#ifdef VESAFB_SHADOW_FB
		hwbits = ri->ri_hwbits;
#else
		hwbits = ri->ri_bits;
#endif
		cur_hwbits = hwbits;
		hwbits += offset;
		if (hwbits > hiwater) {
			/* offset is positive */
			memmove(lowater, ri->ri_bits + offset,
				sc->sc_screensize - offset);
			hwbits = lowater;
		}
		if (hwbits < lowater) {
			/* offset is negative */
			memmove(hiwater - offset,
				ri->ri_bits,
				sc->sc_screensize + offset);
			hwbits = hiwater;
		}
		/* program VESA frame buffer start */
		displ_offset = (hwbits - sc->sc_fbstart);
		memset(&tf, 0, sizeof(struct trapframe));
		tf.tf_eax = 0x4f07; /* function code */
		tf.tf_ebx = 0x00;   /* set display immediately */
		tf.tf_ecx = 0;					/* hpixels */
		tf.tf_edx = displ_offset / ri->ri_stride;	/* lineno  */
		tf.tf_vm86_es = 0;

		res = kvm86_bioscall(0x10, &tf);
		if (res || (tf.tf_eax & 0xff) != 0x4f) {
			working = 0;
			aprint_error("%s: vbecall: res=%d, ax=%x\n",
			    sc->sc_dev.dv_xname, res, tf.tf_eax);
			hwbits = cur_hwbits;
			goto out;
		}
#ifdef VESAFB_SHADOW_FB
		ri->ri_hwbits = hwbits;
		src = ri->ri_bits + srcrow * fontheight * ri->ri_stride;
		dst = ri->ri_bits + dstrow * fontheight * ri->ri_stride;
		memmove(dst, src, size);
#else
		ri->ri_bits = hwbits;
#endif

#if 0
		/* wipe out remains of the screen if nessisary */
		if (ri->ri_emuheight != ri->ri_height)
			vv_eraserows(id, ri->ri_rows, 1, 0);
#endif
		/* remember display's global state (VTs!) */
		sc->sc_displ_bits   = ri->ri_bits;
		sc->sc_displ_hwbits = ri->ri_hwbits;
		return;
	}
out:
	/* just deligate to the origional routine */
	sc->sc_orig_copyrows(id, srcrow, dstrow, nrows);
}

static void
vesafb_init_screen(void *c, struct vcons_screen *scr, int existing,
    long *defattr)
{
	struct vesafb_softc *sc;
	struct rasops_info *ri;
	struct modeinfoblock *mi;

	sc = (struct vesafb_softc *)c;
	mi = &sc->sc_mi;
	ri = &scr->scr_ri;
	ri->ri_flg = RI_CENTER;
	ri->ri_depth = mi->BitsPerPixel;
	ri->ri_width = mi->XResolution;
	ri->ri_height = mi->YResolution;
	ri->ri_emuheight = ri->ri_height;	/* XXX always ? */
	ri->ri_stride = mi->BytesPerScanLine;

	/* set our rasops info to match the display's global state (VTs!) */
	ri->ri_bits   = sc->sc_displ_bits;
	ri->ri_hwbits = sc->sc_displ_hwbits;

	ri->ri_caps = WSSCREEN_WSCOLORS;
	ri->ri_rnum = mi->RedMaskSize;
	ri->ri_gnum = mi->GreenMaskSize;
	ri->ri_bnum = mi->BlueMaskSize;
	ri->ri_rpos = mi->RedFieldPosition;
	ri->ri_gpos = mi->GreenFieldPosition;
	ri->ri_bpos = mi->BlueFieldPosition;

	rasops_init(ri, mi->YResolution / 16, mi->XResolution / 8);

	if (sc->sc_scrollscreens > 1) {
		/* override copyrows but remember old one for delegation */
		sc->sc_orig_copyrows = ri->ri_ops.copyrows;
		ri->ri_ops.copyrows  = vv_copyrows;
	}

#ifdef VESA_DISABLE_TEXT
	if (scr == &vesafb_console_screen)
		SCREEN_DISABLE_DRAWING(&vesafb_console_screen);
#endif
}

static void
vesafb_init(struct vesafb_softc *sc, int setmode)
{
	struct bioscallregs regs;
	struct modeinfoblock *mi;
	struct paletteentry pe;
	int i, j, s;

	mi = &sc->sc_mi;

	s = splsched();

	if (setmode) {
		/* set mode */
		regs.EAX = 0x4f02;
		regs.EBX = sc->sc_mode | 0x4000;
		kvm86_bioscall_simple(0x10, &regs);
		if ((regs.EAX & 0xff) != 0x4f) {
			aprint_error("%s: bioscall failed\n",
			    sc->sc_dev.dv_xname);
			goto out;
		}
	}

	/* setup palette, not required in direct colour mode */
	if (mi->BitsPerPixel == 8) {
		memset(&regs, 0, sizeof(struct bioscallregs));
		regs.EAX = 0x4f08; /* function code */
		regs.EBX = 0x0600; /* we want a 6-bit palette */

		kvm86_bioscall_simple(0x10, &regs);
		if ((regs.EAX & 0xff) != 0x4f) {
			aprint_error("%s: vbecall failed: ax=%x\n",
			    sc->sc_dev.dv_xname, regs.EAX);
			goto out;
		}

		j = 0;

		/* setup ansi / rasops palette */
		for (i = 0; i < 256; i++) {
#ifndef SPLASHSCREEN
			pe.Blue = rasops_cmap[(i * 3) + 2];
			pe.Green = rasops_cmap[(i * 3) + 1];
			pe.Red = rasops_cmap[(i * 3) + 0];
			pe.Alignment = 0;
			vesafb_set_palette(sc, i, pe);
#else
			if (i >= SPLASH_CMAP_OFFSET &&
			    i < SPLASH_CMAP_OFFSET +
			    SPLASH_CMAP_SIZE) {
				pe.Blue = _splash_header_data_cmap[j][2];
				pe.Green = _splash_header_data_cmap[j][1];
				pe.Red = _splash_header_data_cmap[j][0];
				pe.Alignment = 0;
				j++;
				vesafb_set_palette(sc, i, pe);
			} else {
				pe.Blue = rasops_cmap[(i * 3) + 2];
				pe.Green = rasops_cmap[(i * 3) + 1];
				pe.Red = rasops_cmap[(i * 3) + 0];
				pe.Alignment = 0;
				vesafb_set_palette(sc, i, pe);
			}
#endif

			/* Fill in the softc colourmap arrays */
			sc->sc_cmap_red[i / 3] = rasops_cmap[i + 0];
			sc->sc_cmap_green[i / 3] = rasops_cmap[i + 1];
			sc->sc_cmap_blue[i / 3] = rasops_cmap[i + 2];
		}
	}

out:
	splx(s);
	return;
}

static pnp_status_t
vesafb_power(device_t dv, pnp_request_t req, void *opaque)
{
	struct vesafb_softc *sc;
	pnp_status_t stat;
	pnp_display_power_t *ppower;
	pnp_capabilities_t *pcaps;
	pnp_state_t *pstate;
	int bh;

	sc = (struct vesafb_softc *)dv;
	stat = PNP_STATUS_UNSUPPORTED;
	bh = 0;

	switch (req) {
	case PNP_REQUEST_GET_CAPABILITIES:
		pcaps = opaque;

		pcaps->display_power = PNP_DISPLAY_POWER_ON;
		if (sc->sc_pmstates & 1)
			pcaps->display_power |= PNP_DISPLAY_POWER_STANDBY;
		if (sc->sc_pmstates & 2)
			pcaps->display_power |= PNP_DISPLAY_POWER_SUSPEND;
		if (sc->sc_pmstates & 4)
			pcaps->display_power |= PNP_DISPLAY_POWER_OFF;
		if (sc->sc_pmstates & 8)
			pcaps->display_power |= PNP_DISPLAY_POWER_REDUCED;

		pcaps->state = PNP_STATE_D0 | PNP_STATE_D3;

		stat = PNP_STATUS_SUCCESS;
		break;
	case PNP_REQUEST_SET_DISPLAY_POWER:
		ppower = opaque;

		/* If we're not a normal console, don't handle PM requests;
		 * let the windowing system manage it for us.
		 */
		if (sc->sc_wsmode != WSDISPLAYIO_MODE_EMUL)
			return PNP_STATUS_BUSY;

		/* BH 0:on, 1:standby, 2:suspend, 4:off, 8:reduced_on */
		switch (*ppower) {
		case PNP_DISPLAY_POWER_ON:
			bh = 0;
			break;
		case PNP_DISPLAY_POWER_REDUCED:
			bh = 8;
			break;
		case PNP_DISPLAY_POWER_STANDBY:
			bh = 1;
			break;
		case PNP_DISPLAY_POWER_SUSPEND:
			bh = 2;
			break;
		case PNP_DISPLAY_POWER_OFF:
			bh = 4;
			break;
		}

		if (vesafb_svideo(sc, bh) == 0)
			stat = PNP_STATUS_SUCCESS;
		break;

	case PNP_REQUEST_GET_STATE:
		pstate = opaque;
		*pstate = PNP_STATE_D0;
		stat = PNP_STATUS_SUCCESS;
		break;

	case PNP_REQUEST_SET_STATE:
		pstate = opaque;
		stat = PNP_STATUS_SUCCESS;
		switch (*pstate) {
		case PNP_STATE_D0:
#if notyet
			vesafb_init(sc, 1);
			vesafb_svideo(sc, 0);
#endif
			break;
		case PNP_STATE_D3:
			break;
		default:
			break;
		}
		break;

	default:
		break;
	}

	return stat;
}

static void
vesafb_set_palette(struct vesafb_softc *sc, int reg,
    struct paletteentry pe)
{
	struct trapframe tf;
	int res;
	char *buf;

	buf = sc->sc_buf;

	/*
	 * this function takes 8 bit per palette as input, but we're
	 * working in 6 bit mode here
	 */
	pe.Red >>= 2;
	pe.Green >>= 2;
	pe.Blue >>= 2;

	memcpy(buf, &pe, sizeof(struct paletteentry));

	/* set palette */
	memset(&tf, 0, sizeof(struct trapframe));
	tf.tf_eax = 0x4f09; /* function code */
	tf.tf_ebx = 0x0600; /* 6 bit per primary, set format */
	tf.tf_ecx = 1;
	tf.tf_edx = reg;
	tf.tf_vm86_es = 0;
	tf.tf_edi = 0x2000;

	res = kvm86_bioscall(0x10, &tf);
	if (res || (tf.tf_eax & 0xff) != 0x4f)
		aprint_error("%s: vbecall: res=%d, ax=%x\n",
		    sc->sc_dev.dv_xname, res, tf.tf_eax);

	return;
}

static int
vesafb_svideo(struct vesafb_softc *sc, int bh)
{
#ifdef VESAFB_PM
	struct bioscallregs regs;
	int s;

	/* If the power management state is not supported, bail */
	if ((sc->sc_pmstates & bh) != bh)
		return ENODEV;

	memset(&regs, 0, sizeof(struct bioscallregs));
	regs.EAX = 0x4f10;
	regs.EBX = (bh << 8) | 1;

	s = splsched();
	kvm86_bioscall_simple(0x10, &regs);
	splx(s);

	return 0;
#else
	return ENODEV;
#endif
}

static int
vesafb_gvideo(struct vesafb_softc *sc, u_int *on)
{
#ifdef VESAFB_PM
	struct trapframe tf;
	int res, bh;

	/* get video pm state */
	memset(&tf, 0, sizeof(struct trapframe));
	tf.tf_eax = 0x4f10; /* function code */
	tf.tf_ebx = 2;

	res = kvm86_bioscall(0x10, &tf);
	if (res || (tf.tf_eax & 0xff) != 0x4f) {
		aprint_error("%s: unable to get power state (0x%04x)\n",
		    sc->sc_dev.dv_xname, (tf.tf_eax & 0xffff));
		return ENODEV;
	}

	bh = (tf.tf_ebx & 0xff00) >> 8;

	switch (bh) {
	case 0: /* 0:on */
		*on = WSDISPLAYIO_VIDEO_ON;
		break;
	default: /* 1:standby, 2:suspend, 4:off, 8:reduced_on */
		*on = WSDISPLAYIO_VIDEO_OFF;
		break;
	}

	return 0;
#else
	return ENODEV;
#endif
}

static int
vesafb_putcmap(struct vesafb_softc *sc,
		    struct wsdisplay_cmap *cm)
{
	struct paletteentry pe;
	u_int idx, cnt;
	u_char r[256], g[256], b[256];
	u_char *rp, *gp, *bp;
	int rv, i;

	idx = cm->index;
	cnt = cm->count;

	if (idx >= 255 || cnt > 256 || idx + cnt > 256)
		return EINVAL;

	rv = copyin(cm->red, &r[idx], cnt);
	if (rv)
		return rv;
	rv = copyin(cm->green, &g[idx], cnt);
	if (rv)
		return rv;
	rv = copyin(cm->blue, &b[idx], cnt);
	if (rv)
		return rv;

	memcpy(&sc->sc_cmap_red[idx], &r[idx], cnt);
	memcpy(&sc->sc_cmap_green[idx], &g[idx], cnt);
	memcpy(&sc->sc_cmap_blue[idx], &b[idx], cnt);

	rp = &sc->sc_cmap_red[idx];
	gp = &sc->sc_cmap_green[idx];
	bp = &sc->sc_cmap_blue[idx];

	for (i = 0; i < cnt; i++) {
		pe.Blue = *bp;
		pe.Green = *gp;
		pe.Red = *rp;
		pe.Alignment = 0;
		vesafb_set_palette(sc, idx, pe);
		idx++;
		rp++, gp++, bp++;
	}

	return 0;
}

static int
vesafb_getcmap(struct vesafb_softc *sc,
		    struct wsdisplay_cmap *cm)
{
	u_int idx, cnt;
	int rv;

	idx = cm->index;
	cnt = cm->count;

	if (idx >= 255 || cnt > 256 || idx + cnt > 256)
		return EINVAL;

	rv = copyout(&sc->sc_cmap_red[idx], cm->red, cnt);
	if (rv)
		return rv;
	rv = copyout(&sc->sc_cmap_green[idx], cm->green, cnt);
	if (rv)
		return rv;
	rv = copyout(&sc->sc_cmap_blue[idx], cm->blue, cnt);
	if (rv)
		return rv;

	return 0;
}

int
vesafb_cnattach(void)
{
	/* XXX i386 calls consinit too early for us to use
	 *     kvm86; assume that we've attached
	 */

	return 0;
}
