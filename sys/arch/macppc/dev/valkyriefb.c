/*	$NetBSD: valkyriefb.c,v 1.2.2.1 2014/08/20 00:03:11 tls Exp $	*/

/*
 * Copyright (c) 2012 Michael Lorenz
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
 * A console driver for Apple's Valkyrie onboard video controller, found in
 * for example the Performa 6360
 * This should be easy enough to adapt to mac68k but I don't have the hardware
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: valkyriefb.c,v 1.2.2.1 2014/08/20 00:03:11 tls Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <dev/ofw/openfirm.h>

#include <machine/autoconf.h>

#include <dev/wscons/wsdisplayvar.h>
#include <dev/wscons/wsconsio.h>
#include <dev/wsfont/wsfont.h>
#include <dev/rasops/rasops.h>
#include <dev/wscons/wsdisplay_vconsvar.h>

#include <dev/videomode/videomode.h>

#include <arch/macppc/dev/valkyriefbreg.h>
#include <arch/macppc/dev/videopllvar.h>

#include "opt_wsemul.h"
#include "opt_valkyriefb.h"

#include "videopll.h"
#if NVIDEOPLL < 1
#error "valkyriefb requires videopll at iic"
#endif

struct valkyriefb_softc {
	device_t sc_dev;
	int sc_node;
	uint8_t *sc_base;
	uint8_t *sc_fbaddr;

	int sc_depth;
	int sc_width, sc_height, sc_linebytes;
	const struct videomode *sc_videomode;
	uint8_t sc_modereg;

	int sc_mode;
	uint32_t sc_bg;
		
	u_char sc_cmap_red[256];
	u_char sc_cmap_green[256];
	u_char sc_cmap_blue[256];	

	struct vcons_data vd;
};

struct valkyriefb_mode {
	int width;
	int height;
	uint8_t mode;
};

/*
 * Translate screen resolutions to values for Valkyrie's mode register.
 * These numbers seem to roughly correspond to MacOS video mode numbers
 * there are many numbers that result in the same resolution which in MacOS
 * only differ by the display refresh rate, which isn't set by Valkyrie at
 * all, instead there's a PLL chip hooked to cuda's i2c bus. It doesn't seem
 * to matter which number we use as long as the resolution is right and we
 * program the PLL for the right pixel clock
 */
static struct valkyriefb_mode modetab[] = {
	{  640, 480, 6 },
	{  800, 600, 12 },
	{  832, 624, 9 },
	{ 1024, 768, 14},
};
	
static struct vcons_screen valkyriefb_console_screen;

static int	valkyriefb_match(device_t, cfdata_t, void *);
static void	valkyriefb_attach(device_t, device_t, void *);
static int	valkyriefb_init(device_t);
static int 	valkyriefb_set_mode(struct valkyriefb_softc *,
		    const struct videomode *, int);

CFATTACH_DECL_NEW(valkyriefb, sizeof(struct valkyriefb_softc),
    valkyriefb_match, valkyriefb_attach, NULL, NULL);

struct wsscreen_descr valkyriefb_defaultscreen = {
	"default",
	0, 0,
	NULL,
	8, 16,
	WSSCREEN_WSCOLORS | WSSCREEN_HILIT,
	NULL,
};

const struct wsscreen_descr *_valkyriefb_scrlist[] = {
	&valkyriefb_defaultscreen,
	/* XXX other formats, graphics screen? */
};

struct wsscreen_list valkyriefb_screenlist = {
	sizeof(_valkyriefb_scrlist) / sizeof(struct wsscreen_descr *),
	_valkyriefb_scrlist
};

static int	valkyriefb_ioctl(void *, void *, u_long, void *, int,
		    struct lwp *);
static paddr_t	valkyriefb_mmap(void *, void *, off_t, int);

static void	valkyriefb_init_screen(void *, struct vcons_screen *, int,
			    long *);


struct wsdisplay_accessops valkyriefb_accessops = {
	valkyriefb_ioctl,
	valkyriefb_mmap,
	NULL,
	NULL,
	NULL,
	NULL,	/* load_font */
	NULL,	/* polls */
	NULL,	/* scroll */
};

static inline void
valkyriefb_write_reg(struct valkyriefb_softc *sc, int reg, uint8_t val)
{
	*(sc->sc_base + VAL_REGS_OFFSET + reg) = val;
	__asm("eieio; sync;");
}

static inline uint8_t
valkyriefb_read_reg(struct valkyriefb_softc *sc, int reg)
{
	return *(sc->sc_base + VAL_REGS_OFFSET + reg);
}

static void
valkyriefb_write_cmap(struct valkyriefb_softc *sc,
    int reg, uint8_t r, uint8_t g, uint8_t b)
{
	*(sc->sc_base + VAL_CMAP_OFFSET + VAL_CMAP_ADDR) = reg;
	__asm("eieio; sync;");
	*(sc->sc_base + VAL_CMAP_OFFSET + VAL_CMAP_LUT) = r;
	__asm("eieio; sync;");
	*(sc->sc_base + VAL_CMAP_OFFSET + VAL_CMAP_LUT) = g;
	__asm("eieio; sync;");
	*(sc->sc_base + VAL_CMAP_OFFSET + VAL_CMAP_LUT) = b;
	__asm("eieio; sync;");
}

static int
valkyriefb_match(device_t parent, cfdata_t cf, void *aux)
{
	struct confargs *ca = aux;

	if (strcmp(ca->ca_name, "valkyrie") != 0)
		return 0;
	return 1;
}

static void
valkyriefb_attach(device_t parent, device_t self, void *aux)
{
	struct valkyriefb_softc *sc = device_private(self);
	struct confargs *ca = aux;

	sc->sc_dev = self;
	sc->sc_node = ca->ca_node;

	aprint_normal(" address 0x%08x\n", ca->ca_reg[0]);
	aprint_verbose_dev(sc->sc_dev, "waiting for videopll...\n");
	sc->sc_base = (uint8_t *)ca->ca_reg[0];
#ifdef VALKYRIEFB_DEBUG
	for (i = 0; i < 0x40; i += 8) {
		aprint_error_dev(sc->sc_dev, "%02x: %02x\n", i,
		    valkyriefb_read_reg(sc, i));
	}
#endif
	config_finalize_register(sc->sc_dev, valkyriefb_init);
	sc->sc_fbaddr = (uint8_t *)(sc->sc_base + 0x1000);
}

static int
valkyriefb_init(device_t self)
{
	struct valkyriefb_softc *sc = device_private(self);
	const struct videomode *mode;
	struct rasops_info *ri;
	prop_dictionary_t dict;
	struct wsemuldisplaydev_attach_args aa;
	bool console = FALSE;
	long defattr;

	mode = pick_mode_by_ref(800, 600, 60);
	if (mode == NULL)
		return 0;

	valkyriefb_set_mode(sc, mode, 8);

	vcons_init(&sc->vd, sc, &valkyriefb_defaultscreen,
	    &valkyriefb_accessops);
	sc->vd.init_screen = valkyriefb_init_screen;

	dict = device_properties(sc->sc_dev);
	prop_dictionary_get_bool(dict, "is_console", &console);

	ri = &valkyriefb_console_screen.scr_ri;
	vcons_init_screen(&sc->vd, &valkyriefb_console_screen, 1, &defattr);
	memset(sc->sc_base + 0x1000, ri->ri_devcmap[(defattr >> 16) & 0xf],
	    sc->sc_width * sc->sc_linebytes);
	valkyriefb_console_screen.scr_flags |= VCONS_SCREEN_IS_STATIC;

	valkyriefb_defaultscreen.textops = &ri->ri_ops;
	valkyriefb_defaultscreen.capabilities = ri->ri_caps;
	valkyriefb_defaultscreen.nrows = ri->ri_rows;
	valkyriefb_defaultscreen.ncols = ri->ri_cols;
	if (console) {
		wsdisplay_cnattach(&valkyriefb_defaultscreen, ri, 0, 0,
		    defattr);
		vcons_replay_msgbuf(&valkyriefb_console_screen);
	}
	aa.console = console;
	aa.scrdata = &valkyriefb_screenlist;
	aa.accessops = &valkyriefb_accessops;
	aa.accesscookie = &sc->vd;

	config_found(self, &aa, wsemuldisplaydevprint);

	return 0;
}

static int
valkyriefb_set_mode(struct valkyriefb_softc *sc,
    const struct videomode *mode, int depth)
{
	int i;
	uint8_t modereg, tmp;

	/* first find the parameter for the mode register */
	i = 0;
	while ((i < __arraycount(modetab)) && 
	       (modetab[i].width != mode->hdisplay) &&
	       (modetab[i].height != mode->vdisplay))
		i++;
	if (i >= __arraycount(modetab)) {
		aprint_error_dev(sc->sc_dev,
		    "Can't find a mode register value for %s\n", mode->name);
		return EINVAL;
	} else
		modereg = modetab[i].mode;

	/* check if we have enough video memory */
	if ((mode->hdisplay * mode->vdisplay * (depth >> 3)) > 0x100000) {
		aprint_error_dev(sc->sc_dev, "Not enough video RAM for %s\n",
		    mode->name);
		return EINVAL;
	}

	/* reject depths other than 8 or 16 */
	if ((depth != 8) && (depth != 16)) {
		aprint_error_dev(sc->sc_dev,
		    "Depth [%d] is unsupported for %s\n", depth, mode->name);
		return EINVAL;
	}

	/* now start programming the chip */
	valkyriefb_write_reg(sc, VAL_STATUS, 0);
	delay(100);
	valkyriefb_write_reg(sc, VAL_MODE, modereg | VAL_MODE_STOP);

	if (depth == 8) {
		sc->sc_depth = 8;
		valkyriefb_write_reg(sc, VAL_DEPTH, VAL_DEPTH_8);
		for (i = 0; i < 256; i++) {
			tmp = i & 0xe0;
			/*
			 * replicate bits so 0xe0 maps to a red value of 0xff
			 * in order to make white look actually white
			 */
			tmp |= (tmp >> 3) | (tmp >> 6);
			sc->sc_cmap_red[i] = tmp;

			tmp = (i & 0x1c) << 3;
			tmp |= (tmp >> 3) | (tmp >> 6);
			sc->sc_cmap_green[i] = tmp;

			tmp = (i & 0x03) << 6;
			tmp |= tmp >> 2;
			tmp |= tmp >> 4;
			sc->sc_cmap_blue[i] = tmp;

			valkyriefb_write_cmap(sc, i, sc->sc_cmap_red[i],
			    sc->sc_cmap_green[i], sc->sc_cmap_blue[i]);
		}
	} else if (depth == 16) {
		sc->sc_depth = 16;
		valkyriefb_write_reg(sc, VAL_DEPTH, VAL_DEPTH_16);
		/* does the palette have any effect in 16bit? */
	}

	videopll_set_freq(mode->dot_clock);
	delay(100);
	valkyriefb_write_reg(sc, VAL_MODE, modereg);

	sc->sc_modereg = modereg;
	sc->sc_videomode = mode;
	sc->sc_width = mode->hdisplay;
	sc->sc_height = mode->vdisplay;
	sc->sc_linebytes = mode->hdisplay * (sc->sc_depth >> 3);
	aprint_normal_dev(sc->sc_dev, "switched to %d x %d in %d bit colour\n",
	    sc->sc_width, sc->sc_height, sc->sc_depth);
	return 0;
}

static int
valkyriefb_ioctl(void *v, void *vs, u_long cmd, void *data, int flag,
	struct lwp *l)
{
	struct vcons_data *vd = v;
	struct valkyriefb_softc *sc = vd->cookie;
	struct wsdisplay_fbinfo *wdf;
	struct vcons_screen *ms = vd->active;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = WSDISPLAY_TYPE_VALKYRIE;
		return 0;

	case WSDISPLAYIO_GINFO:
		wdf = (void *)data;
		wdf->height = ms->scr_ri.ri_height;
		wdf->width = ms->scr_ri.ri_width;
		wdf->depth = ms->scr_ri.ri_depth;
		wdf->cmsize = 256;
		return 0;
#if 0	
	case WSDISPLAYIO_GETCMAP:
		return valkyriefb_getcmap(sc,
		    (struct wsdisplay_cmap *)data);

	case WSDISPLAYIO_PUTCMAP:
		return valkyriefb_putcmap(sc,
		    (struct wsdisplay_cmap *)data);
#endif

	case WSDISPLAYIO_SMODE: {
		int new_mode = *(int*)data;
		if (new_mode != sc->sc_mode) {
			sc->sc_mode = new_mode;
			if (new_mode == WSDISPLAYIO_MODE_EMUL) {
				vcons_redraw_screen(ms);
			}
		}
		}
		return 0;
	}
	return EPASSTHROUGH;
}

static paddr_t
valkyriefb_mmap(void *v, void *vs, off_t offset, int prot)
{
	struct vcons_data *vd = v;
	struct valkyriefb_softc *sc = vd->cookie;
	paddr_t pa;
		
	/* 'regular' framebuffer mmap()ing */
	if (offset < 0x100000) {
		pa = (paddr_t)(sc->sc_base + 0x1000 + offset);	
		return pa;
	}
	return -1;
}

static void
valkyriefb_init_screen(void *cookie, struct vcons_screen *scr,
    int existing, long *defattr)
{
	struct valkyriefb_softc *sc = cookie;
	struct rasops_info *ri = &scr->scr_ri;
	
	memset(ri, 0, sizeof(struct rasops_info));
	ri->ri_depth = sc->sc_depth;
	ri->ri_width = sc->sc_width;
	ri->ri_height = sc->sc_height;
	ri->ri_stride = sc->sc_linebytes;
	ri->ri_flg = RI_CENTER | RI_8BIT_IS_RGB | RI_ENABLE_ALPHA;
	ri->ri_bits = sc->sc_fbaddr;

	scr->scr_flags |= VCONS_DONT_READ;
	
	rasops_init(ri, 0, 0);
	ri->ri_caps = WSSCREEN_WSCOLORS;

	rasops_reconfig(ri, sc->sc_height / ri->ri_font->fontheight,
		    sc->sc_width / ri->ri_font->fontwidth);

	ri->ri_hw = scr;
}
