/*	$NetBSD: pvr.c,v 1.5.2.2 2001/03/12 13:28:07 bouyer Exp $	*/

/*-
 * Copyright (c) 2001 Marcus Comstedt.
 * Copyright (c) 2001 Jason R. Thorpe.
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
 *	This product includes software developed by Marcus Comstedt.
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

/*
 * Copyright (c) 1998, 1999 Tohru Nishimura.  All rights reserved.
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
 *      This product includes software developed by Tohru Nishimura
 *	for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: pvr.c,v 1.5.2.2 2001/03/12 13:28:07 bouyer Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h> 
#include <sys/buf.h>
#include <sys/ioctl.h>

#include <machine/cpu.h>
#include <machine/bus.h>

#include <dev/cons.h> 

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h> 

#include <dev/wscons/wscons_callbacks.h>
 
#include <dev/rasops/rasops.h>
#include <dev/wsfont/wsfont.h>

#include <dreamcast/dev/pvrvar.h>
#include <dreamcast/dev/maple/mkbdvar.h>

#include <sh3/shbvar.h>

#include "mkbd.h"

struct fb_devconfig {
	vaddr_t dc_vaddr;		/* framebuffer virtual address */
	vaddr_t dc_paddr;		/* framebuffer physical address */
	int	dc_wid;			/* width of frame buffer */
	int	dc_ht;			/* height of frame buffer */
	int	dc_depth;		/* depth, bits per pixel */
	int	dc_rowbytes;		/* bytes in a FB scan line */
	vaddr_t	dc_videobase;		/* base of flat frame buffer */
	int	dc_blanked;		/* currently has video disabled */
	int	dc_dispflags;		/* display flags */
	int	dc_tvsystem;		/* TV broadcast system */

	struct rasops_info rinfo;
};

#define	PVR_RGBMODE	0x01		/* RGB or composite */
#define	PVR_VGAMODE	0x02		/* VGA */

struct pvr_softc {
	struct device sc_dev;
	struct fb_devconfig *sc_dc;	/* device configuration */
	int nscreens;
};

int	pvr_match(struct device *, struct cfdata *, void *);
void	pvr_attach(struct device *, struct device *, void *);

struct cfattach pvr_ca = {
	sizeof(struct pvr_softc), pvr_match, pvr_attach,
};

void	pvr_getdevconfig(struct fb_devconfig *);

struct fb_devconfig pvr_console_dc;

char pvr_stdscreen_textgeom[32] = { "std" };	/* XXX yuck */

struct wsscreen_descr pvr_stdscreen = {
	pvr_stdscreen_textgeom, 0, 0,
	0, /* textops */
	0, 0,
	WSSCREEN_WSCOLORS,
};

const struct wsscreen_descr *_pvr_scrlist[] = {
	&pvr_stdscreen,
};

const struct wsscreen_list pvr_screenlist = {
	sizeof(_pvr_scrlist) / sizeof(struct wsscreen_descr *), _pvr_scrlist
};

int	pvrioctl(void *, u_long, caddr_t, int, struct proc *);
paddr_t	pvrmmap(void *, off_t, int);

int	pvr_alloc_screen(void *, const struct wsscreen_descr *,
	    void **, int *, int *, long *);
void	pvr_free_screen(void *, void *);
int	pvr_show_screen(void *, void *, int,
	    void (*)(void *, int, int), void *);

const struct wsdisplay_accessops pvr_accessops = {
	pvrioctl,
	pvrmmap,
	pvr_alloc_screen,
	pvr_free_screen,
	pvr_show_screen,
	NULL, /* load_font */
};

void	pvrinit(struct fb_devconfig *);

int	pvr_is_console;

int
pvr_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct shb_attach_args *sa = aux;

	if (strcmp("pvr", match->cf_driver->cd_name) != 0)
		return (0);

	sa->ia_iosize = 0; /* 0x1400 */;
	return (1);
}

void
pvr_getdevconfig(struct fb_devconfig *dc)
{
	int i, cookie;

	dc->dc_paddr = 0x05000000;
	dc->dc_vaddr = SH3_PHYS_TO_P2SEG(dc->dc_paddr);

	dc->dc_wid = 640;
	dc->dc_ht = 480;
	dc->dc_depth = 16;
	dc->dc_rowbytes = dc->dc_wid * (dc->dc_depth / 8);
	dc->dc_videobase = dc->dc_vaddr;
	dc->dc_blanked = 0;
	dc->dc_dispflags = 0;

	/* Clear the screen. */
	for (i = 0; i < dc->dc_ht * dc->dc_rowbytes; i += sizeof(u_int32_t))
		*(u_int32_t *)(dc->dc_videobase + i) = 0x0;

	/* Initialize the device. */
	pvrinit(dc);

	dc->rinfo.ri_flg = 0;
	dc->rinfo.ri_depth = dc->dc_depth;
	dc->rinfo.ri_bits = (void *) dc->dc_videobase;
	dc->rinfo.ri_width = dc->dc_wid;
	dc->rinfo.ri_height = dc->dc_ht;
	dc->rinfo.ri_stride = dc->dc_rowbytes;

	wsfont_init();
	/* prefer 8 pixel wide font */
	if ((cookie = wsfont_find(NULL, 8, 0, 0)) <= 0)
		cookie = wsfont_find(NULL, 0, 0, 0);
	if (cookie <= 0) {
		printf("pvr: font table is empty\n");
		return;
	}

	if (wsfont_lock(cookie, &dc->rinfo.ri_font,
	    WSDISPLAY_FONTORDER_L2R, WSDISPLAY_FONTORDER_L2R) <= 0) {
		printf("pvr: unable to lock font\n");
		return;
	}
	dc->rinfo.ri_wsfcookie = cookie;

	rasops_init(&dc->rinfo, 500, 500);

	/* XXX shouldn't be global */
	pvr_stdscreen.nrows = dc->rinfo.ri_rows;
	pvr_stdscreen.ncols = dc->rinfo.ri_cols;
	pvr_stdscreen.textops = &dc->rinfo.ri_ops;
	pvr_stdscreen.capabilities = dc->rinfo.ri_caps;

	/* XXX yuck */
	sprintf(pvr_stdscreen_textgeom, "%dx%d", pvr_stdscreen.ncols,
	    pvr_stdscreen.nrows);
}

void
pvr_attach(struct device *parent, struct device *self, void *aux)
{
	struct pvr_softc *sc = (void *) self;
	struct wsemuldisplaydev_attach_args waa;
	int console;
	static const char *tvsystem_name[4] =
		{ "NTSC", "PAL", "PAL-M", "PAL-N" };

	console = pvr_is_console;
	if (console) {
		sc->sc_dc = &pvr_console_dc;
		sc->nscreens = 1;
	} else {
		sc->sc_dc = malloc(sizeof(struct fb_devconfig), M_DEVBUF,
		    M_WAITOK);
		pvr_getdevconfig(sc->sc_dc);
	}
	printf(": %d x %d, %dbpp, %s, %s\n", sc->sc_dc->dc_wid,
	    sc->sc_dc->dc_ht, sc->sc_dc->dc_depth,
	    (sc->sc_dc->dc_dispflags & PVR_VGAMODE) ? "VGA" :
	       tvsystem_name[sc->sc_dc->dc_tvsystem],
	    (sc->sc_dc->dc_dispflags & PVR_RGBMODE) ? "RGB" : "composite");

	/* XXX Colormap initialization? */

	waa.console = console;
	waa.scrdata = &pvr_screenlist;
	waa.accessops = &pvr_accessops;
	waa.accesscookie = sc;

	(void) config_found(self, &waa, wsemuldisplaydevprint);
}

int
pvrioctl(void *v, u_long cmd, caddr_t data, int flag, struct proc *p)
{
	struct pvr_softc *sc = v;
	struct fb_devconfig *dc = sc->sc_dc;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = WSDISPLAY_TYPE_DCPVR;
		return (0);

	case WSDISPLAYIO_GINFO:
#define	wsd_fbip ((struct wsdisplay_fbinfo *)data)
		wsd_fbip->height = sc->sc_dc->dc_ht;
		wsd_fbip->width = sc->sc_dc->dc_wid;
		wsd_fbip->depth = sc->sc_dc->dc_depth;
		wsd_fbip->cmsize = 0;	/* XXX Colormap */
#undef wsd_fbip
		return (0);

	case WSDISPLAYIO_GETCMAP:
	case WSDISPLAYIO_PUTCMAP:
		return (ENOTTY);	/* XXX Colormap */

	case WSDISPLAYIO_SVIDEO:
		return (ENOTTY);	/* XXX */

	case WSDISPLAYIO_GVIDEO:
		*(u_int *)data = dc->dc_blanked ?
		    WSDISPLAYIO_VIDEO_OFF : WSDISPLAYIO_VIDEO_ON;
		return (0);

	case WSDISPLAYIO_GCURPOS:
	case WSDISPLAYIO_SCURPOS:
	case WSDISPLAYIO_GCURMAX:
	case WSDISPLAYIO_GCURSOR:
	case WSDISPLAYIO_SCURSOR:
		return (ENOTTY);	/* XXX */
	}

	return (ENOTTY);
}

paddr_t
pvrmmap(void *v, off_t offset, int prot)
{

	/*
	 * XXX This should be easy to support -- just need to define
	 * XXX offsets for the contol regs, etc.
	 */

	struct pvr_softc *sc = v;
	struct fb_devconfig *dc = sc->sc_dc;
	paddr_t addr;

	if (offset >= 0 &&
	    offset < sh3_round_page(dc->dc_rowbytes * dc->dc_ht))
		addr = sh3_btop(dc->dc_paddr + offset);
	else
		addr = (-1);	/* XXX bogus */

	return addr;
}

int
pvr_alloc_screen(void *v, const struct wsscreen_descr *type,
    void **cookiep, int *curxp, int *curyp, long *attrp)
{
	struct pvr_softc *sc = v;
	long defattr;

	if (sc->nscreens > 0)
		return (ENOMEM);

	*cookiep = &sc->sc_dc->rinfo; /* one and only for now */
	*curxp = 0;
	*curyp = 0;
	(*sc->sc_dc->rinfo.ri_ops.alloc_attr)(&sc->sc_dc->rinfo, 0, 0, 0,
	    &defattr);
	*attrp = defattr;
	sc->nscreens++;
	return (0);
}

void
pvr_free_screen(void *v, void *cookie)
{
	struct pvr_softc *sc = v;

	if (sc->sc_dc == &pvr_console_dc)
		panic("pvr_free_screen: console");

	sc->nscreens--;
}

int
pvr_show_screen(void *v, void *cookie, int waitok,
    void (*cb)(void *, int, int), void *cbarg)
{

	return (0);
}

static void
pvr_check_cable(struct fb_devconfig *dc)
{
	__volatile u_int32_t *porta =
	    (__volatile u_int32_t *)0xff80002c;
	u_int16_t v;

	/* PORT8 and PORT9 is input */
	*porta = (*porta & ~0xf0000) | 0xa0000;

	/* Read PORT8 and PORT9 */
	v = ((*(__volatile u_int16_t *)(porta + 1)) >> 8) & 3;

	if ((v & 2) == 0)
		dc->dc_dispflags |= PVR_VGAMODE|PVR_RGBMODE;
	else if ((v & 1) == 0)
		dc->dc_dispflags |= PVR_RGBMODE;
}

static void
pvr_check_tvsys(struct fb_devconfig *dc)
{
	/* XXX should use flashmem device when one exists */
	dc->dc_tvsystem = (*(__volatile u_int8_t *)0xa021a004) & 3;
}

void
pvrinit(struct fb_devconfig *dc)
{
	__volatile u_int32_t *pvr = (__volatile u_int32_t *)
	    SH3_PHYS_TO_P2SEG(0x005f8000);
	int display_lines_per_field = 240;
	int v_absolute_size = 525;
	int h_absolute_size = 857;
	int modulo = 1, voffset, hoffset = 164, border = (126 << 16) | 837;

	pvr_check_cable(dc);
	pvr_check_tvsys(dc);

	pvr[8/4] = 0;		/* reset */
	pvr[0x40/4] = 0;	/* black border */

	if (dc->dc_dispflags & PVR_VGAMODE) {
		pvr[0x44/4] = 0x800004;	/* 31kHz, RGB565 */
		pvr[0xd0/4] = 0x100;	/* video output */
		display_lines_per_field = 480;
		voffset = 36;
	} else {
		if(dc->dc_tvsystem & 1) {
			/* 50 Hz */
			v_absolute_size = 625;
			h_absolute_size = 863;
			hoffset = 174;
			border = (116 << 16) | 843;
		}
		pvr[0x44/4] = 0x000004;	/* 15kHz, RGB565 */
		/* video output, PAL/NTSC, interlace */
		pvr[0xd0/4] = 0x110|(dc->dc_tvsystem<<6);
		modulo += 640 * 2 / 4;	/* interlace -> skip every other line */
		voffset = 18;
	}

	pvr[0x50/4] = 0;	/* video base address, long field */
	pvr[0x54/4] = 640 * 2;	/* video base address, short field */

	pvr[0x5c/4] = (modulo << 20) | ((display_lines_per_field - 1) << 10) |
	    (640 * 2 / 4 - 1);

	voffset = (voffset << 16) | voffset;

	pvr[0xf0/4] = voffset;				/* V start */
	pvr[0xdc/4] = voffset + display_lines_per_field;/* V border */
	pvr[0xec/4] = hoffset;				/* H start */
	pvr[0xd8/4] = (v_absolute_size<<16) | h_absolute_size; /* HV counter */
	pvr[0xd4/4] = border;				/* H border */
	pvr[0xe8/4] = 22 << 16;

	/* RGB / composite */
	*(__volatile u_int32_t *)
	    SH3_PHYS_TO_P2SEG(0x00702c00) =
	    ((dc->dc_dispflags & PVR_RGBMODE) ? 0 : 3) << 8;

	pvr[0x44/4] |= 1;	/* display on */
}

/* Console support. */

void	pvrcnprobe(struct consdev *);
void	pvrcninit(struct consdev *);

void
pvrcninit(struct consdev *cndev)
{
	struct fb_devconfig *dcp = &pvr_console_dc;
	long defattr;

	pvr_getdevconfig(dcp);
	(*dcp->rinfo.ri_ops.alloc_attr)(&dcp->rinfo, 0, 0, 0, &defattr);
	wsdisplay_cnattach(&pvr_stdscreen, &dcp->rinfo, 0, 0, defattr);

	pvr_is_console = 1;

	cn_tab->cn_pri = CN_INTERNAL;

#if NMKBD > 0
	mkbd_cnattach();	/* connect keyboard and screen together */
#endif
}

void
pvrcnprobe(struct consdev *cndev)
{
#if NWSDISPLAY > 0
	int maj, unit;
#endif
	cndev->cn_dev = NODEV;
	cndev->cn_pri = CN_NORMAL;

#if NWSDISPLAY > 0
	unit = 0;
	for (maj = 0; maj < nchrdev; maj++) {
		if (cdevsw[maj].d_open == wsdisplayopen)
			break;
	}
	if (maj != nchrdev) {
		cndev->cn_pri = CN_INTERNAL;
		cndev->cn_dev = makedev(maj, unit);
	}
#endif
}
