/*	$NetBSD: genfbvar.h,v 1.27 2022/03/28 11:21:40 mlelstv Exp $ */

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

#ifndef GENFBVAR_H
#define GENFBVAR_H

#ifdef _KERNEL_OPT
#include "opt_splash.h"
#endif

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/device.h>

#include <sys/bus.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/rasops/rasops.h>

#include <dev/wscons/wsdisplay_vconsvar.h>
#ifdef _KERNEL_OPT
#include "opt_genfb.h"
#endif

#ifdef SPLASHSCREEN
#define GENFB_DISABLE_TEXT
#include <dev/splash/splash.h>
#endif

#if GENFB_GLYPHCACHE > 0
#include <dev/wscons/wsdisplay_glyphcachevar.h>
#endif

struct genfb_softc;

struct genfb_ops {
	int (*genfb_ioctl)(void *, void *, u_long, void *, int, struct lwp *);
	paddr_t	(*genfb_mmap)(void *, void *, off_t, int);
	int (*genfb_borrow)(void *, bus_addr_t, bus_space_handle_t *);
	int (*genfb_enable_polling)(void *);
	int (*genfb_disable_polling)(void *);
};

struct genfb_colormap_callback {
	void *gcc_cookie;
	void (*gcc_set_mapreg)(void *, int, int, int, int);
};

/*
 * Integer parameter provider.  Each callback shall return 0 on success,
 * and an error(2) number on failure.  The gpc_upd_parameter callback is
 * optional (i.e. it can be NULL).
 *
 * This structure is used for backlight and brightness control.  The
 * expected parameter range is:
 *
 *	[0, 1]		for backlight
 *	[0, 255]	for brightness
 */
struct genfb_parameter_callback {
	void *gpc_cookie;
	int (*gpc_get_parameter)(void *, int *);
	int (*gpc_set_parameter)(void *, int);
	int (*gpc_upd_parameter)(void *, int);
};

struct genfb_pmf_callback {
	bool (*gpc_suspend)(device_t, const pmf_qual_t *);
	bool (*gpc_resume)(device_t, const pmf_qual_t *);
};

struct genfb_mode_callback {
	bool (*gmc_setmode)(struct genfb_softc *, int);
};

struct genfb_softc {
	device_t sc_dev;
	struct vcons_data vd;
	struct genfb_ops sc_ops;
	struct vcons_screen sc_console_screen;
	struct wsscreen_descr sc_defaultscreen_descr;
	const struct wsscreen_descr *sc_screens[1];
	struct wsscreen_list sc_screenlist;
	struct genfb_colormap_callback *sc_cmcb;
	struct genfb_pmf_callback *sc_pmfcb;
	struct genfb_parameter_callback *sc_backlight;
	struct genfb_parameter_callback *sc_brightness;
	struct genfb_mode_callback *sc_modecb;
	int sc_backlight_level, sc_backlight_on;
	void *sc_fbaddr;	/* kva */
	void *sc_shadowfb;
	bool sc_enable_shadowfb;
	bus_addr_t sc_fboffset;	/* bus address */
	int sc_width, sc_height, sc_stride, sc_depth;
	size_t sc_fbsize;
	int sc_mode;
	u_char sc_cmap_red[256];
	u_char sc_cmap_green[256];
	u_char sc_cmap_blue[256];
	bool sc_want_clear;
#ifdef SPLASHSCREEN
	struct splash_info sc_splash;
#endif
	struct wsdisplay_accessops sc_accessops;
#if GENFB_GLYPHCACHE > 0
	/*
	 * The generic glyphcache code makes a bunch of assumptions that are
	 * true for most graphics hardware with a directly supported blitter.
	 * For example it assume that
	 * - VRAM access from the host is expensive
	 * - copying data around in VRAM is cheap and can happen in parallel
	 *   to the host CPU
	 * -> therefore we draw glyphs normally if we have to, so the ( assumed
	 *    to be hardware assisted ) driver supplied putchar() method doesn't
	 *    need to be glyphcache aware, then copy them away for later use
	 * for genfb things are a bit different. On most hardware:
	 * - VRAM access from the host is still expensive
	 * - copying data around in VRAM is also expensive since we don't have
	 *   a blitter and VRAM is mapped uncached
	 * - VRAM reads are usually slower than writes ( write combining and
	 *   such help writes but not reads, and VRAM might be behind an
	 *   asymmetric bus like AGP ) and must be avoided, both are much
	 *   slower than main memory
	 * -> therefore we cache glyphs in main memory, no reason to map it
	 *    uncached, we draw into the cache first and then copy the glyph
	 *    into video memory to avoid framebuffer reads and to allow more
	 *    efficient write accesses than putchar() would offer
	 * Because of this we can't use the generic code but we can recycle a
	 * few data structures.
	 */
	uint8_t *sc_cache;
	struct rasops_info sc_cache_ri;
	void (*sc_putchar)(void *, int, int, u_int, long);
	int sc_cache_cells;
	int sc_nbuckets;	/* buckets allocated */
	gc_bucket *sc_buckets;	/* we allocate as many as we can get into ram */
	int sc_attrmap[256];	/* mapping a colour attribute to a bucket */
#endif

};

void	genfb_cnattach(void);
int	genfb_cndetach(void);
void	genfb_disable(void);
int	genfb_is_console(void);
int	genfb_is_enabled(void);
void	genfb_init(struct genfb_softc *);
int	genfb_attach(struct genfb_softc *, struct genfb_ops *);
int	genfb_borrow(bus_addr_t, bus_space_handle_t *);
void	genfb_restore_palette(struct genfb_softc *);
void	genfb_enable_polling(device_t);
void	genfb_disable_polling(device_t);

#endif /* GENFBVAR_H */
