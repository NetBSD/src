/*	$NetBSD: grf_ul.c,v 1.50 2014/01/22 00:25:16 christos Exp $ */
#define UL_DEBUG

/*-
 * Copyright (c) 1995 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Ignatios Souvatzis.
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

#include "opt_amigacons.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: grf_ul.c,v 1.50 2014/01/22 00:25:16 christos Exp $");

#include "grful.h"
#include "ite.h"
#if NGRFUL > 0

/* Graphics routines for the University of Lowell A2410 board,
   using the TMS34010 processor. */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/syslog.h>

#include <machine/cpu.h>

#include <amiga/amiga/device.h>
#include <amiga/amiga/isr.h>
#include <amiga/dev/zbusvar.h>
#include <amiga/dev/grfioctl.h>
#include <amiga/dev/grfvar.h>
#include <amiga/dev/grf_ulreg.h>

extern u_int16_t tmscode[];

int ul_ioctl(struct grf_softc *, u_long, void *, dev_t);
int ul_getcmap(struct grf_softc *, struct grf_colormap *, dev_t);
int ul_putcmap(struct grf_softc *, struct grf_colormap *, dev_t);
int ul_bitblt(struct grf_softc *, struct grf_bitblt *, dev_t);
int ul_blank(struct grf_softc *, int *, dev_t);

static int ulisr(void *);
int ulowell_alive(struct grfvideo_mode *);
static void ul_load_code(struct grf_softc *);
static int ul_load_mon(struct grf_softc *, struct grfvideo_mode *);
static int ul_getvmode(struct grf_softc *, struct grfvideo_mode *);
static int ul_setvmode(struct grf_softc *, unsigned);
static inline void ul_setfb(struct grf_softc *, u_long);

/*
 * marked true early so that ulowell_cnprobe() can tell if we are alive.
 */
int ulowell_inited;

/* standard-palette definition */
u_int8_t ul_std_palette[] = {
	0,128,  0,128,   0,128,  0,128,  0,255,  0,255,   0,255,  0,255,
	0,  0,128,128,   0,  0,128,128,  0,  0,255,255,   0,  0,255,255,
	0,  0,  0,  0, 128,128,128,128,  0,  0,  0,  0, 255,255,255,255};

u_int8_t ul_ovl_palette[] = {
	128, 0, 0, 0,
	128, 0, 0, 0,
	128, 0, 0, 0};

struct grfvideo_mode ul_monitor_defs[] = {

 	/*
	 * We give all these values in MI units, that is:
	 * horizontal timings in units of pixels
	 * vertical timings in units of lines
	 * point of reference is blanking end.
	 *
	 * The ul_load_mon transforms these values right before loading
	 * them into the chip.
	 *
	 * This leaves us with a single point where things are transformed,
	 * which should make life easier if we ever change things again.
	 */

	/* 1024x768, 60Hz */
	{1,"1024x768", 66667000,	1024,768,8,	1024,1088,1296,1392,
		768,771,774,798, 0},
	/* 864x648, 70Hz */
	{2,"864x648", 50000000,		864,648,8,	864,928,992,1056,
		648,658,663,678, 0},
	/* 800x600, 60Hz */
	{3, "800x600", 36000000,	800,600,8,	800,864,928,992,
		600,610,615,630, 0},
	/* 640x400, 60 Hz, interlaced */
	{4, "640x400i", 14318000,	640,400,8,	640,768,832,912,
		200,223,203,240, 1},
	/* 1024x768, 65Hz interlaced, s.th. is strange */
	{5, "1024x768?i", 44980000,	1024,768,8,	1024,1072,1136,1280,
		488,509,512,534, 1},
	/* 1024x1024, 60Hz */
	{6, "1024x1024", 80000000,	1024,1024,8,	1024,1040,1120,1248,
		1024,1027,1030,1055, 0},
	/* 736x480, 60 Hz */
	{7, "736x480", 28636300,	736,480,8,	736,784,848,928,
		480,491,495,515, 0},
};

int ulowell_mon_max = sizeof (ul_monitor_defs)/sizeof (ul_monitor_defs[0]);

/* option settable */
#ifndef ULOWELL_OSC1
#define ULOWELL_OSC1 36000000
#endif

#ifndef ULOWELL_OSC2
#define ULOWELL_OSC2 66667000
#endif

#ifndef ULOWELL_DEFAULT_MON
#define ULOWELL_DEFAULT_MON 1
#endif

/* patchable */
int ulowell_default_mon = ULOWELL_DEFAULT_MON;
int ulowell_default_gfx = ULOWELL_DEFAULT_MON;

/*
 * yes, this should be per board. We don't pay service to multiple boards,
 * anyway.
 */

u_long ulowell_clock[2] = { ULOWELL_OSC2, ULOWELL_OSC1 };

static struct grfvideo_mode *current_mon;

/*
 * We dont use ints at the moment, but will need this later to avoid
 * busy_waiting in gsp_write, and we use it for spurious int warnings.
 */

static int
ulisr(void *arg)
{
	struct grf_softc *gp = arg;
	volatile struct gspregs *ba;
	u_int16_t	thebits;

	if (gp == NULL)
		return 0;

	ba = (volatile struct gspregs *)gp->g_regkva;

	if (ba == NULL)
		return 0;

	thebits = ba->ctrl;
	if (thebits & INTOUT) {
		log(LOG_INFO, "grf4: got interrupt, ctrl=0x%4x\n", thebits);
		/* clear int */
		ba->ctrl = thebits & ~INTOUT;
		return 1;
	}
	return 0;
}

/*
 * used to query the ulowell board to see if its alive.
 * for the moment, a NOP.
 */
int
ulowell_alive(struct grfvideo_mode *mdp)
{
	return 1;
}

/*
 * Load the (mostly) ite support code and the default colormaps.
 */
static void
ul_load_code(struct grf_softc *gp)
{
	struct grf_ul_softc *gup;
	volatile struct gspregs *ba;
	struct grfinfo *gi;
	int i,j;
#if 0
	struct grf_colormap gcm;
#endif

	gup = (struct grf_ul_softc *)gp;
	ba = (volatile struct gspregs *)gp->g_regkva;
	gi = &gp->g_display;

	gi->gd_regaddr	= ztwopa((volatile void *)ba);
	gi->gd_regsize	= sizeof(struct gspregs);
	gi->gd_fbaddr	= NULL;
	gi->gd_fbsize	= 0;
	gi->gd_fbwidth	= 1024;
	gi->gd_fbheight	= 1024;
	gi->gd_colors	= 256;

	ba->ctrl = (ba->ctrl & ~INCR) | (LBL | INCW);
	ba->hstadrh = 0xC000;
	ba->hstadrl = 0x0080;
	ba->data = 0x0;		/* disable screen refresh and video output */
	ba->data = 0xFFFC;	/* screen refresh base address */
	ba->data = 0xFFFF;	/* no display int possible */
	ba->data = 0x000C;	/* CAS before RAS refresh each 64 local clks */

	ba->ctrl = (ba->ctrl & ~INCW) | LBL;
	ba->hstadrh = 0xfe80;
	ba->hstadrl = 0;
	ba->data = 4;
	ba->hstadrl = 0x20;
	ba->data = 0xFF;	/* all color planes visible */

	ba->hstadrl = 0;
	ba->data = 5;
	ba->hstadrl = 0x20;
	ba->data = 0;		/* no color planes blinking */

	ba->hstadrl = 0;
	ba->data = 6;
	ba->hstadrl = 0x20;
	ba->data = gup->gus_ovslct = 0x43;
	/* overlay visible, no overlay blinking, overlay color 0 transparent */

	ba->hstadrl = 0;
	ba->data = 7;
	ba->hstadrl = 0x20;
	ba->data = 0;		/* voodoo */

	/* clear overlay planes */
	ba->ctrl |= INCW;
	ba->hstadrh = 0xff80;
	ba->hstadrl = 0x0000;
	for (i=0xff80000; i< 0xffa0000; ++i) {
		ba->data = 0;
	}

	/* download tms code */

	ba->ctrl = LBL | INCW | NMI | NMIM | HLT | CF;

	printf("\ndownloading TMS code");
	i=0;
	while ((j = tmscode[i++])) {
		printf(".");
		ba->hstadrh = tmscode[i++];
		ba->hstadrl = tmscode[i++];
		while (j-- > 0) {
			ba->data = tmscode[i++];
		}
	}

	/* font info was uploaded in ite_ul.c(ite_ulinit). */

#if 1
	/* XXX load image palette with some initial values, slightly hacky */

	ba->hstadrh = 0xfe80;
	ba->hstadrl = 0x0000;
	ba->ctrl |= INCW;
	ba->data = 0;
	ba->ctrl &= ~INCW;

	for (i=0; i<16; ++i) {
		ba->data = gup->gus_imcmap[i+  0] = ul_std_palette[i+ 0];
		ba->data = gup->gus_imcmap[i+256] = ul_std_palette[i+16];
		ba->data = gup->gus_imcmap[i+512] = ul_std_palette[i+32];
	}

	/*
	 * XXX load shadow overlay palette with what the TMS code will load
	 * into the real one some time after the TMS code is started below.
	 * This might be considered a rude hack.
	 */
	memcpy(gup->gus_ovcmap, ul_ovl_palette, 3*4);

	/*
	 * Unflush cache, unhalt CPU -> nmi starts to run. This MUST NOT BE
	 * DONE before the image color map initialization above, to guarantee
	 * the index register in the BT458 is not used by more than one CPU
	 * at once.
	 *
	 * XXX For the same reason, we'll have to rething ul_putcmap(). For
	 * details, look at comment there.
	 */
	ba->ctrl &= ~(HLT|CF);

#else
	/*
	 * XXX I wonder why this partially ever worked.
	 *
	 * This can't possibly work this way, as we are copyin()ing data in
	 * ul_putcmap.
	 *
	 * I guess this partially worked because SFC happened to point to
	 * to supervisor data space on 68030 machines coming from the old
	 * boot loader.
	 *
	 * While this looks more correct than the hack in the other part of the
	 * loop, we would have to do our own version of the loop through
	 * colormap entries, set up command buffer, and call gsp_write(), or
	 * factor out some code.
	 */

	/*
	 * XXX This version will work for the overlay, if our queue codes
	 * initial conditions are set at load time (not start time).
	 * It further assumes that ul_putcmap only uses the
	 * GRFIMDEV/GRFOVDEV bits of the dev parameter.
	 */


	/* unflush cache, unhalt CPU first -> nmi starts to run */
	ba->ctrl &= ~(HLT|CF);

	gcm.index = 0;
	gcm.count = 16;
	gcm.red   = ul_std_palette +  0;
	gcm.green = ul_std_palette + 16;
	gcm.blue  = ul_std_palette + 32;
	ul_putcmap(gp, &gcm, GRFIMDEV);

	gcm.index = 0;
	gcm.count = 4;
	gcm.red   = ul_ovl_palette + 0;
	gcm.green = ul_ovl_palette + 4;
	gcm.blue  = ul_ovl_palette + 8;
	ul_putcmap(gp, &gcm, GRFOVDEV);
#endif

}

static int
ul_load_mon(struct grf_softc *gp, struct grfvideo_mode *md)
{
	struct grfinfo *gi;
	volatile struct gspregs *ba;
	u_int16_t buf[8];

	gi = &gp->g_display;
	ba = (volatile struct gspregs *)gp->g_regkva;

	gi->gd_dyn.gdi_fbx	= 0;
	gi->gd_dyn.gdi_fby	= 0;
	gi->gd_dyn.gdi_dwidth	= md->disp_width;
	gi->gd_dyn.gdi_dheight	= md->disp_height;
	gi->gd_dyn.gdi_dx	= 0;
	gi->gd_dyn.gdi_dy	= 0;

	ba->ctrl = (ba->ctrl & ~INCR) | (LBL | INCW); /* XXX */

	ba->hstadrh = 0xC000;
	ba->hstadrl = 0x0000;

	ba->data = (md->hsync_stop - md->hsync_start)/16;
	ba->data = (md->htotal - md->hsync_start)/16 - 1;
	ba->data = (md->hblank_start + md->htotal - md->hsync_start)/16 - 1;
	ba->data = md->htotal/16 - 1;

	ba->data = md->vsync_stop - md->vsync_start;
	ba->data = md->vtotal - md->vsync_start - 1;
	ba->data = md->vblank_start + md->vtotal - md->vsync_start - 1;
	ba->data = md->vtotal - 1;

	ba->ctrl &= ~INCW;
	ba->hstadrh = 0xFE90;
	ba->hstadrl = 0x0000;

	if (abs(md->pixel_clock - ulowell_clock[0]) >
	    abs(md->pixel_clock - ulowell_clock[1])) {

		ba->data = (ba->data & 0xFC) | 2 | 1;
		md->pixel_clock = ulowell_clock[1];

	} else {
		ba->data = (ba->data & 0xFC) | 2 | 0;
		md->pixel_clock = ulowell_clock[0];
	}

	ba->ctrl |= LBL | INCW;
	ba->hstadrh = 0xC000;
	ba->hstadrl = 0x0080;
	ba->data = md->disp_flags & GRF_FLAGS_LACE ? 0xb020 : 0xf020;

	/* I guess this should be in the yet unimplemented mode select ioctl */
	/* Hm.. maybe not. We always put the console on overlay plane no 0. */
	/* Anyway, this _IS_ called in the mode select ioctl. */

	/* ite support code parameters: */
	buf[0] = GCMD_MCHG;
	buf[1] = md->disp_width;	/* display width */
	buf[2] = md->disp_height;	/* display height */
	buf[3] = 0;			/* LSW of frame buffer origin */
	buf[4] = 0xFF80;		/* MSW of frame buffer origin */
	buf[5] = gi->gd_fbwidth * 1;	/* frame buffer pitch */
	buf[6] = 1;			/* frame buffer depth */
	gsp_write(ba, buf, 7);

	return(1);
}

int ul_mode(struct grf_softc *, u_long, void *, u_long, int);

void grfulattach(device_t, device_t, void *);
int grfulprint(void *, const char *);
int grfulmatch(device_t, cfdata_t, void *);

CFATTACH_DECL_NEW(grful, sizeof(struct grf_ul_softc),
    grfulmatch, grfulattach, NULL, NULL);

/*
 * only used in console init
 */
static struct cfdata *cfdata;

/*
 * we make sure to only init things once.  this is somewhat
 * tricky regarding the console.
 */
int
grfulmatch(device_t parent, cfdata_t cf, void *aux)
{
#ifdef ULOWELLCONSOLE
	static int ulconunit = -1;
#endif
	struct zbus_args *zap;

	zap = aux;

	/*
	 * allow only one ulowell console
	 */
        if (amiga_realconfig == 0)
#ifdef ULOWELLCONSOLE
		if (ulconunit != -1)
#endif
			return(0);

	if (zap->manid != 1030 || zap->prodid != 0)
		return(0);

#ifdef ULOWELLCONSOLE
	if (amiga_realconfig == 0 || ulconunit != cf->cf_unit) {
#endif
		if ((unsigned)ulowell_default_mon > ulowell_mon_max)
			ulowell_default_mon = 1;

		current_mon = ul_monitor_defs + ulowell_default_mon - 1;
		if (ulowell_alive(current_mon) == 0)
			return(0);
#ifdef ULOWELLCONSOLE
		if (amiga_realconfig == 0) {
			ulconunit = cf->cf_unit;
			cfdata = cf;
		}
	}
#endif
	return(1);
}

/*
 * attach to the grfbus (zbus)
 */
void
grfulattach(device_t parent, device_t self, void *aux)
{
	static struct grf_ul_softc congrf;
	struct device temp;
	struct zbus_args *zap;
	struct grf_softc *gp;
	struct grf_ul_softc *gup;

	zap = aux;

	if (self == NULL) {
		gup = &congrf;
		gp = &gup->gus_sc;
		gp->g_device = &temp;
		temp.dv_private = gp;
	} else {
		gup = device_private(self);
		gp = &gup->gus_sc;
		gp->g_device = self;
	}

	if (self != NULL && congrf.gus_sc.g_regkva != 0) {
		/*
		 * inited earlier, just copy (not device struct)
		 */
		memcpy(&gp->g_display, &congrf.gus_sc.g_display,
		    (char *)&gup->gus_isr - (char *)&gp->g_display);

		/* ...and transfer the isr */
		gup->gus_isr.isr_ipl = 2;
		gup->gus_isr.isr_intr = ulisr;
		gup->gus_isr.isr_arg = (void *)gp;
		/*
		 * To make sure ints are always catched, first add new isr
		 * then remove old:
		 */
		add_isr(&gup->gus_isr);
		remove_isr(&congrf.gus_isr);
	} else {
		gp->g_regkva = (void *)zap->va;
		gp->g_fbkva = NULL;
		gp->g_unit = GRF_ULOWELL_UNIT;
		gp->g_flags = GF_ALIVE;
		gp->g_mode = ul_mode;
#if NITE > 0
		gp->g_conpri = grful_cnprobe();
#endif
		gp->g_data = NULL;

		gup->gus_isr.isr_ipl = 2;
		gup->gus_isr.isr_intr = ulisr;
		gup->gus_isr.isr_arg = (void *)gp;
		add_isr(&gup->gus_isr);

		(void)ul_load_code(gp);
		(void)ul_load_mon(gp, current_mon);
#if NITE > 0
		grful_iteinit(gp);
#endif
	}
	if (self != NULL)
		printf("\n");
	/*
	 * attach grf
	 */
	amiga_config_found(cfdata, gp->g_device, gp, grfulprint);
}

int
grfulprint(void *aux, const char *pnp)
{
	if (pnp)
		aprint_normal("grf%d at %s", ((struct grf_softc *)aux)->g_unit,
			pnp);
	return(UNCONF);
}

static int
ul_getvmode (struct grf_softc *gp, struct grfvideo_mode *vm)
{
	struct grfvideo_mode *md;

	if (vm->mode_num && vm->mode_num > ulowell_mon_max)
		return EINVAL;

	if (! vm->mode_num)
		vm->mode_num = current_mon - ul_monitor_defs + 1;

	md = ul_monitor_defs + vm->mode_num - 1;
	strncpy (vm->mode_descr, md->mode_descr,
		sizeof (vm->mode_descr));

	/* XXX should tell TMS to measure it */
	vm->pixel_clock  = md->pixel_clock;
	vm->disp_width   = md->disp_width;
	vm->disp_height  = md->disp_height;
	vm->depth        = md->depth;

	vm->hblank_start = md->hblank_start;
	vm->hsync_start  = md->hsync_start;
	vm->hsync_stop   = md->hsync_stop;
	vm->htotal       = md->htotal;

	vm->vblank_start = md->vblank_start;
	vm->vsync_start  = md->vsync_start;
	vm->vsync_stop   = md->vsync_stop;
	vm->vtotal       = md->vtotal;

	vm->disp_flags	 = md->disp_flags;
	return 0;
}


static int
ul_setvmode (struct grf_softc *gp, unsigned mode)
{
	int error;

	if (!mode || mode > ulowell_mon_max)
		return EINVAL;

	current_mon = ul_monitor_defs + mode - 1;

	error = ul_load_mon (gp, current_mon) ? 0 : EINVAL;

	return error;
}

/*
 * Set the frame buffer or overlay planes on or off.
 * Always succeeds.
 */

static inline void
ul_setfb(struct grf_softc *gp, u_long cmd)
{
	struct grf_ul_softc *gup;
	volatile struct gspregs *ba;

	gup = (struct grf_ul_softc *)gp;

	ba = (volatile struct gspregs *)gp->g_regkva;
	ba->ctrl = LBL;
	ba->hstadrh = 0xfe80;
	ba->hstadrl = 0x0000;
	ba->data = 6;
	ba->hstadrl = 0x0020;

	switch (cmd) {
	case GM_GRFON:
		gup->gus_ovslct |= 0x40;
		break;
	case GM_GRFOFF:
		gup->gus_ovslct &= ~0x40;
		break;
	case GM_GRFOVON:
		gup->gus_ovslct |= 3;
		break;
	case GM_GRFOVOFF:
		gup->gus_ovslct &= ~3;
		break;
	}
	ba->data = gup->gus_ovslct;
}

/*
 * Change the mode of the display.
 * Return a UNIX error number or 0 for success.
 */
int
ul_mode(struct grf_softc *gp, u_long cmd, void *arg, u_long a2, int a3)
{
	int i;
	struct grfdyninfo *gd;

	switch (cmd) {
	case GM_GRFON:
	case GM_GRFOFF:
	case GM_GRFOVON:
	case GM_GRFOVOFF:
		ul_setfb (gp, cmd);
		return 0;

	case GM_GRFCONFIG:
		gd = (struct grfdyninfo *)arg;
		for (i=0; i<ulowell_mon_max; ++i) {
			if (ul_monitor_defs[i].disp_width == gd->gdi_dwidth &&
			    ul_monitor_defs[i].disp_height == gd->gdi_dheight)
				return ul_setvmode(gp, i+1);
		}
		return EINVAL;

	case GM_GRFGETVMODE:
		return ul_getvmode (gp, (struct grfvideo_mode *) arg);

	case GM_GRFSETVMODE:
		return ul_setvmode (gp, *(unsigned *) arg);

	case GM_GRFGETNUMVM:
		*(int *)arg = ulowell_mon_max;
		return 0;

	case GM_GRFIOCTL:
		return ul_ioctl (gp, a2, arg, (dev_t)a3);

	default:
		break;
	}

	return EPASSTHROUGH;
}

int
ul_ioctl (register struct grf_softc *gp, u_long cmd, void *data, dev_t dev)
{
	switch (cmd) {
#if 0
	/*
	 * XXX we have no hardware sprites, but might implement them
	 * later in TMS code.
	 */

	case GRFIOCGSPRITEPOS:
		return ul_getspritepos (gp, (struct grf_position *) data);

	case GRFIOCSSPRITEPOS:
		return ul_setspritepos (gp, (struct grf_position *) data);

	case GRFIOCSSPRITEINF:
		return ul_setspriteinfo (gp, (struct grf_spriteinfo *) data);

	case GRFIOCGSPRITEINF:
		return ul_getspriteinfo (gp, (struct grf_spriteinfo *) data);

	case GRFIOCGSPRITEMAX:
		return ul_getspritemax (gp, (struct grf_position *) data);

#endif

	case GRFIOCGETCMAP:
		return ul_getcmap (gp, (struct grf_colormap *) data, dev);

	case GRFIOCPUTCMAP:
		return ul_putcmap (gp, (struct grf_colormap *) data, dev);

	case GRFIOCBITBLT:
		return ul_bitblt (gp, (struct grf_bitblt *) data, dev);

	case GRFIOCBLANK:
		return ul_blank (gp, (int *) data, dev);
	}

	return EPASSTHROUGH;
}

int
ul_getcmap (struct grf_softc *gp, struct grf_colormap *cmap, dev_t dev)
{
	struct grf_ul_softc *gup;
	u_int8_t *mymap;
	int mxidx, error;

	gup = (struct grf_ul_softc *)gp;

	if (minor(dev) & GRFIMDEV) {
		mxidx = 256;
		mymap = gup->gus_imcmap;
	} else {
		mxidx = 4;
		mymap = gup->gus_ovcmap;
	}

	if (cmap->count == 0 || cmap->index >= mxidx)
		return 0;

	if (cmap->count > mxidx - cmap->index)
		cmap->count = mxidx - cmap->index;

	/* just copyout from the shadow color map */

	if ((error = copyout(mymap + cmap->index, cmap->red, cmap->count))

	    || (error = copyout(mymap + mxidx + cmap->index, cmap->green,
		cmap->count))

	    || (error = copyout(mymap + mxidx * 2 + cmap->index, cmap->blue,
		cmap->count)))

		return(error);

	return(0);
}

int
ul_putcmap (struct grf_softc *gp, struct grf_colormap *cmap, dev_t dev)
{
	struct grf_ul_softc *gup;
	volatile struct gspregs *ba;
	u_int16_t cmd[8];
	int x, mxidx, error;
	u_int8_t *mymap;

	gup = (struct grf_ul_softc *)gp;

	if (minor(dev) & GRFIMDEV) {
		mxidx = 256;
		mymap = gup->gus_imcmap;
	} else {
		mxidx = 4;
		mymap = gup->gus_ovcmap;
	}

	if (cmap->count == 0 || cmap->index >= mxidx)
		return 0;

	if (cmap->count > mxidx - cmap->index)
		cmap->count = mxidx - cmap->index;

	/* first copyin to our shadow color map */

	if ((error = copyin(cmap->red, mymap + cmap->index, cmap->count))

	    || (error = copyin(cmap->green, mymap + cmap->index + mxidx,
		cmap->count))

	    || (error = copyin(cmap->blue,  mymap + cmap->index + mxidx*2,
		cmap->count)))

		return error;


	/* then write from there to the hardware */
	ba = (volatile struct gspregs *)gp->g_regkva;
	/*
	 * XXX This is a bad thing to do.
	 * We should always use the gsp call, or have a means to arbitrate
	 * the usage of the BT458 index register. Else there might be a
	 * race condition (when writing both colormaps at nearly the same
	 * time), where one CPU changes the index register when the other
	 * one has not finished using it.
	 */
	if (mxidx > 4) {
		/* image color map: we can write, with a hack, directly */
		ba->ctrl = LBL;
		ba->hstadrh = 0xfe80;
		ba->hstadrl = 0x0000;
		ba->ctrl |= INCW;
		ba->data = cmap->index;
		ba->ctrl &= ~INCW;

		for (x=cmap->index; x < cmap->index + cmap->count; ++x) {
			ba->data = (u_int16_t) mymap[x];
			ba->data = (u_int16_t) mymap[x + mxidx];
			ba->data = (u_int16_t) mymap[x + mxidx * 2];
		}
	} else {

		/* overlay planes color map: have to call tms to do it */
		cmd[0] = GCMD_CMAP;
		cmd[1] = 1;
		for (x=cmap->index; x < cmap->index + cmap->count; ++x) {
			cmd[2] = x;
			cmd[3] = mymap[x];
			cmd[4] = mymap[x + mxidx];
			cmd[5] = mymap[x + mxidx * 2];
			gsp_write(ba, cmd, 6);
		}
	}
	return 0;
}

int
ul_blank(struct grf_softc *gp, int *onoff, dev_t dev)
{
	volatile struct gspregs *gsp;

	gsp = (volatile struct gspregs *)gp->g_regkva;
	gsp->ctrl = (gsp->ctrl & ~(INCR | INCW)) | LBL;
	gsp->hstadrh = 0xC000;
	gsp->hstadrl = 0x0080;
	if (*onoff > 0)
		gsp->data |= 0x9000;
	else
		gsp->data &= ~0x9000;

	return 0;
}
/*
 * !!! THIS AREA UNDER CONSTRUCTION !!!
 */
int ul_BltOpMap[16] = {
	3, 1, 2, 0, 11,  9, 10, 8,
	7, 5, 6, 4, 15, 13, 14, 12
};

int
ul_bitblt (struct grf_softc *gp, struct grf_bitblt *bb, dev_t dev)
{
	/* XXX not yet implemented, but pretty trivial */
	return EPASSTHROUGH;
}

void
gsp_write(volatile struct gspregs *gsp, u_short *ptr, size_t size)
{
	u_short put, new_put, next, oc;
	u_long put_hi, oa;
	size_t n;

	if (size == 0 || size > 8)
	        return;

	n = size;

	oc = gsp->ctrl;
	oa = GSPGETHADRS(gsp);

	gsp->ctrl = (oc & ~INCR) | LBL | INCW;
	GSPSETHADRS(gsp, GSP_MODE_ADRS);
	gsp->data &= ~GMODE_FLUSH;

	GSPSETHADRS(gsp, PUT_HI_PTR_ADRS);
	put_hi = gsp->data << 16;

	GSPSETHADRS(gsp, PUT_PTR_ADRS);
	put = gsp->data;
	new_put = put + (8<<4);

	GSPSETHADRS(gsp, GET_PTR_ADRS);
	next = gsp->data;

	while (next == new_put) {
		/*
		 * we should use an intr. here. unfortunately, we already
		 * are called from an interrupt and can't use tsleep.
		 * so we do busy waiting, at least for the moment.
		 */

		GSPSETHADRS(gsp,GET_PTR_ADRS);
		next = gsp->data;
	}

	GSPSETHADRS(gsp,put|put_hi);
	gsp->data = *ptr++ | 8<<4;
	while ( --n > 0) {
		gsp->data = *ptr++;
	}

	GSPSETHADRS(gsp,PUT_PTR_ADRS);
	gsp->data = new_put;
	GSPSETHADRS(gsp,oa);
	gsp->ctrl = oc;

	return;
}

#endif	/* NGRF */
