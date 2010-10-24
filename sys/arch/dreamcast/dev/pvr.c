/*	$NetBSD: pvr.c,v 1.30 2010/10/24 13:34:27 tsutsui Exp $	*/

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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: pvr.c,v 1.30 2010/10/24 13:34:27 tsutsui Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h> 
#include <sys/buf.h>
#include <sys/ioctl.h>

#include <machine/vmparam.h>
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

#include "mkbd.h"

#define	PVRREG_FBSTART		0x05000000
#define	PVRREG_REGSTART		0x005f8000

#define	PVRREG_BRDCOLR		0x40
#define	BRDCOLR_BLUE(x)		((x) << 0)
#define	BRDCOLR_GREEN(x)	((x) << 8)
#define	BRDCOLR_RED(x)		((x) << 16)

#define	PVRREG_DIWMODE		0x44
#define	DIWMODE_DE		(1U << 0)	/* display enable */
#define	DIWMODE_SD		(1U << 1)	/* scan double enable */
#define	DIWMODE_COL(x)		((x) << 2)
#define	DIWMODE_COL_RGB555	DIWMODE_COL(0)	/* RGB555, 16-bit */
#define	DIWMODE_COL_RGB565	DIWMODE_COL(1)	/* RGB565, 16-bit */
#define	DIWMODE_COL_RGB888	DIWMODE_COL(2)	/* RGB888, 24-bit */
#define	DIWMODE_COL_ARGB888	DIWMODE_COL(3)	/* RGB888, 32-bit */
#define	DIWMODE_C		(1U << 23)	/* 2x clock enable (VGA) */

#define	PVRREG_DIWADDRL		0x50

#define	PVRREG_DIWADDRS		0x54

#define	PVRREG_DIWSIZE		0x5c
#define	DIWSIZE_DPL(x)		((x) << 0)	/* pixel data per line */
#define	DIWSIZE_LPF(x)		((x) << 10)	/* lines per field */
#define	DIWSIZE_MODULO(x)	((x) << 20)	/* words to skip + 1 */

#define	PVRREG_RASEVTPOS	0xcc
#define	RASEVTPOS_BOTTOM(x)	((x) << 0)
#define	RASEVTPOS_TOP(x)	((x) << 16)

#define	PVRREG_SYNCCONF		0xd0
#define	SYNCCONF_VP		(1U << 0)	/* V-sync polarity */
#define	SYNCCONF_HP		(1U << 1)	/* H-sync polarity */
#define	SYNCCONF_I		(1U << 4)	/* interlace */
#define	SYNCCONF_BC(x)		(1U << 6)	/* broadcast standard */
#define	SYNCCONF_VO		(1U << 8)	/* video output enable */

#define	PVRREG_BRDHORZ		0xd4
#define	BRDHORZ_STOP(x)		((x) << 0)
#define	BRDHORZ_START(x)	((x) << 16)

#define	PVRREG_SYNCSIZE		0xd8
#define	SYNCSIZE_H(x)		((x) << 0)
#define	SYNCSIZE_V(x)		((x) << 16)

#define	PVRREG_BRDVERT		0xdc
#define	BRDVERT_STOP(x)		((x) << 0)
#define	BRDVERT_START(x)	((x) << 16)

#define	PVRREG_DIWCONF		0xe8
#define	DIWCONF_LR		(1U << 8)	/* low-res */
#define	DIWCONF_MAGIC		(22 << 16)

#define	PVRREG_DIWHSTRT		0xec

#define	PVRREG_DIWVSTRT		0xf0
#define	DIWVSTRT_V1(x)		((x) << 0)
#define	DIWVSTRT_V2(x)		((x) << 16)

#define	PVR_REG_READ(dc, reg)						\
	((volatile uint32_t *)(dc)->dc_regvaddr)[(reg) >> 2]
#define	PVR_REG_WRITE(dc, reg, val)					\
	((volatile uint32_t *)(dc)->dc_regvaddr)[(reg) >> 2] = (val)

struct fb_devconfig {
	vaddr_t dc_vaddr;		/* framebuffer virtual address */
	vaddr_t dc_paddr;		/* framebuffer physical address */
	vaddr_t dc_regvaddr;		/* registers virtual address */
	vaddr_t dc_regpaddr;		/* registers physical address */
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
	device_t sc_dev;
	struct fb_devconfig *sc_dc;	/* device configuration */
	int nscreens;
};

static int	pvr_match(device_t, cfdata_t, void *);
static void	pvr_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(pvr, sizeof(struct pvr_softc),
    pvr_match, pvr_attach, NULL, NULL);

static void	pvr_getdevconfig(struct fb_devconfig *);

static struct fb_devconfig pvr_console_dc;

static char pvr_stdscreen_textgeom[32] = { "std" };	/* XXX yuck */

static struct wsscreen_descr pvr_stdscreen = {
	pvr_stdscreen_textgeom, 0, 0,
	0, /* textops */
	0, 0,
	WSSCREEN_WSCOLORS,
};

static const struct wsscreen_descr *_pvr_scrlist[] = {
	&pvr_stdscreen,
};

static const struct wsscreen_list pvr_screenlist = {
	sizeof(_pvr_scrlist) / sizeof(struct wsscreen_descr *), _pvr_scrlist
};

static int	pvrioctl(void *, void *, u_long, void *, int, struct lwp *);
static paddr_t	pvrmmap(void *, void *, off_t, int);

static int	pvr_alloc_screen(void *, const struct wsscreen_descr *,
		    void **, int *, int *, long *);
static void	pvr_free_screen(void *, void *);
static int	pvr_show_screen(void *, void *, int,
		    void (*)(void *, int, int), void *);

static const struct wsdisplay_accessops pvr_accessops = {
	pvrioctl,
	pvrmmap,
	pvr_alloc_screen,
	pvr_free_screen,
	pvr_show_screen,
	NULL, /* load_font */
};

static void	pvrinit(struct fb_devconfig *);

int	pvr_is_console;

int
pvr_match(device_t parent, cfdata_t cf, void *aux)
{

	return 1;
}

void
pvr_getdevconfig(struct fb_devconfig *dc)
{
	int i, cookie;

	dc->dc_paddr = PVRREG_FBSTART;
	dc->dc_vaddr = SH3_PHYS_TO_P2SEG(dc->dc_paddr);

	dc->dc_regpaddr = PVRREG_REGSTART;
	dc->dc_regvaddr = SH3_PHYS_TO_P2SEG(dc->dc_regpaddr);

	dc->dc_wid = 640;
	dc->dc_ht = 480;
	dc->dc_depth = 16;
	dc->dc_rowbytes = dc->dc_wid * (dc->dc_depth / 8);
	dc->dc_videobase = dc->dc_vaddr;
	dc->dc_blanked = 0;
	dc->dc_dispflags = 0;

	/* Clear the screen. */
	for (i = 0; i < dc->dc_ht * dc->dc_rowbytes; i += sizeof(uint32_t))
		*(uint32_t *)(dc->dc_videobase + i) = 0x0;

	/* Initialize the device. */
	pvrinit(dc);

	dc->rinfo.ri_flg = 0;
	if (dc == &pvr_console_dc)
		dc->rinfo.ri_flg |= RI_NO_AUTO;
	dc->rinfo.ri_depth = dc->dc_depth;
	dc->rinfo.ri_bits = (void *) dc->dc_videobase;
	dc->rinfo.ri_width = dc->dc_wid;
	dc->rinfo.ri_height = dc->dc_ht;
	dc->rinfo.ri_stride = dc->dc_rowbytes;

	wsfont_init();
	/* prefer 8 pixel wide font */
	cookie = wsfont_find(NULL, 8, 0, 0, WSDISPLAY_FONTORDER_L2R,
	    WSDISPLAY_FONTORDER_L2R);
	if (cookie <= 0)
		cookie = wsfont_find(NULL, 0, 0, 0, WSDISPLAY_FONTORDER_L2R,
		    WSDISPLAY_FONTORDER_L2R);
	if (cookie <= 0) {
		printf("pvr: font table is empty\n");
		return;
	}

	if (wsfont_lock(cookie, &dc->rinfo.ri_font)) {
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
pvr_attach(device_t parent, device_t self, void *aux)
{
	struct pvr_softc *sc = device_private(self);
	struct wsemuldisplaydev_attach_args waa;
	int console;
	static const char *tvsystem_name[4] =
		{ "NTSC", "PAL", "PAL-M", "PAL-N" };

	sc->sc_dev = self;
	console = pvr_is_console;
	if (console) {
		sc->sc_dc = &pvr_console_dc;
		sc->sc_dc->rinfo.ri_flg &= ~RI_NO_AUTO;
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
pvrioctl(void *v, void *vs, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct pvr_softc *sc = v;
	struct fb_devconfig *dc = sc->sc_dc;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = WSDISPLAY_TYPE_DCPVR;
		return 0;

	case WSDISPLAYIO_GINFO:
#define	wsd_fbip ((struct wsdisplay_fbinfo *)data)
		wsd_fbip->height = sc->sc_dc->dc_ht;
		wsd_fbip->width = sc->sc_dc->dc_wid;
		wsd_fbip->depth = sc->sc_dc->dc_depth;
		wsd_fbip->cmsize = 0;	/* XXX Colormap */
#undef wsd_fbip
		return 0;

	case WSDISPLAYIO_LINEBYTES:
		*(u_int *)data = sc->sc_dc->rinfo.ri_stride;
		return 0;

	case WSDISPLAYIO_GETCMAP:
	case WSDISPLAYIO_PUTCMAP:
		return EPASSTHROUGH;	/* XXX Colormap */

	case WSDISPLAYIO_SVIDEO:
		switch (*(u_int *)data) {
		case WSDISPLAYIO_VIDEO_OFF:
			if (!dc->dc_blanked) {
				dc->dc_blanked = 1;
				PVR_REG_WRITE(dc, PVRREG_DIWMODE,
				    PVR_REG_READ(dc, PVRREG_DIWMODE) &
				    ~DIWMODE_DE);
			}
			break;
		case WSDISPLAYIO_VIDEO_ON:
			if (dc->dc_blanked) {
				dc->dc_blanked = 0;
				PVR_REG_WRITE(dc, PVRREG_DIWMODE,
				    PVR_REG_READ(dc, PVRREG_DIWMODE) |
				    DIWMODE_DE);
			}
			break;
		default:
			return EPASSTHROUGH;	/* XXX */
		}
		return 0;

	case WSDISPLAYIO_GVIDEO:
		*(u_int *)data = dc->dc_blanked ?
		    WSDISPLAYIO_VIDEO_OFF : WSDISPLAYIO_VIDEO_ON;
		return 0;

	case WSDISPLAYIO_GCURPOS:
	case WSDISPLAYIO_SCURPOS:
	case WSDISPLAYIO_GCURMAX:
	case WSDISPLAYIO_GCURSOR:
	case WSDISPLAYIO_SCURSOR:
		return EPASSTHROUGH;	/* XXX */
	}

	return EPASSTHROUGH;
}

paddr_t
pvrmmap(void *v, void *vs, off_t offset, int prot)
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
		return ENOMEM;

	*cookiep = &sc->sc_dc->rinfo; /* one and only for now */
	*curxp = 0;
	*curyp = 0;
	(*sc->sc_dc->rinfo.ri_ops.allocattr)(&sc->sc_dc->rinfo, 0, 0, 0,
	    &defattr);
	*attrp = defattr;
	sc->nscreens++;
	return 0;
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

	return 0;
}

static void
pvr_check_cable(struct fb_devconfig *dc)
{
	volatile uint32_t *porta =
	    (volatile uint32_t *)0xff80002c;
	uint16_t v;

	/* PORT8 and PORT9 is input */
	*porta = (*porta & ~0xf0000) | 0xa0000;

	/* Read PORT8 and PORT9 */
	v = ((*(volatile uint16_t *)(porta + 1)) >> 8) & 3;

	if ((v & 2) == 0)
		dc->dc_dispflags |= PVR_VGAMODE|PVR_RGBMODE;
	else if ((v & 1) == 0)
		dc->dc_dispflags |= PVR_RGBMODE;
}

static void
pvr_check_tvsys(struct fb_devconfig *dc)
{

	/* XXX should use flashmem device when one exists */
	dc->dc_tvsystem = (*(volatile uint8_t *)0xa021a004) & 3;
}

void
pvrinit(struct fb_devconfig *dc)
{
	int display_lines_per_field;
	int v_absolute_size;
	int h_absolute_size;
	int vborder_start, vborder_stop;
	int hborder_start, hborder_stop;
	int modulo = 1, voffset, hoffset;

	pvr_check_cable(dc);
	pvr_check_tvsys(dc);

	PVR_REG_WRITE(dc, 8, 0);		/* reset */
	PVR_REG_WRITE(dc, PVRREG_BRDCOLR, 0);	/* black border */

	if (dc->dc_dispflags & PVR_VGAMODE) {
		v_absolute_size = 524;
		h_absolute_size = 857;

		display_lines_per_field = 480;
		hoffset = 164;
		voffset = 36;

		hborder_start = 126;
		hborder_stop = 837;

		vborder_start = 40;
		vborder_stop = 444;		/* XXX */

		/* 31kHz, RGB565 */
		PVR_REG_WRITE(dc, PVRREG_DIWMODE,
		    DIWMODE_C | DIWMODE_COL_RGB565);

		/* video output */
		PVR_REG_WRITE(dc, PVRREG_SYNCCONF, SYNCCONF_VO);
	} else {
		if (dc->dc_tvsystem & 1) {
			/* 50 Hz PAL */
			v_absolute_size = 624;
			h_absolute_size = 863;

			display_lines_per_field = 240;
			hoffset = 174;
			voffset = 18;

			hborder_start = 116;
			hborder_stop = 843;

			vborder_start = 44;
			vborder_stop = 536;	/* XXX */
		} else {
			/* 60 Hz NTSC */
			v_absolute_size = 524;
			h_absolute_size = 857; 

			display_lines_per_field = 240;
			hoffset = 170;
			voffset = 28;

			hborder_start = 126;
			hborder_stop = 837;

			vborder_start = 18;
			vborder_stop = 506;	/* XXX */
		}

		modulo += 640 * 2 / 4;	/* interlace -> skip every other line */

		/* 15kHz, RGB565 */
		PVR_REG_WRITE(dc, PVRREG_DIWMODE,
		    DIWMODE_COL_RGB565);

		/* video output, PAL/NTSC, interlace */
		PVR_REG_WRITE(dc, PVRREG_SYNCCONF,
		    SYNCCONF_VO | SYNCCONF_I | SYNCCONF_BC(dc->dc_tvsystem));
	}

	/* video base address, long field */
	PVR_REG_WRITE(dc, PVRREG_DIWADDRL, 0);

	/* video base address, short field */
	PVR_REG_WRITE(dc, PVRREG_DIWADDRS, 640 * 2);

	/* video size */
	PVR_REG_WRITE(dc, PVRREG_DIWSIZE, DIWSIZE_MODULO(modulo) |
	    DIWSIZE_LPF(display_lines_per_field - 1) |
	    DIWSIZE_DPL(640 * 2 / 4 - 1));

	PVR_REG_WRITE(dc, PVRREG_DIWVSTRT,		/* V start */
	    DIWVSTRT_V1(voffset) | DIWVSTRT_V2(voffset));
	PVR_REG_WRITE(dc, PVRREG_BRDVERT,		/* V border */
	    BRDVERT_START(vborder_start) | BRDVERT_STOP(vborder_stop));
	PVR_REG_WRITE(dc, PVRREG_DIWHSTRT, hoffset);	/* H start */
	PVR_REG_WRITE(dc, PVRREG_SYNCSIZE,		/* HV counter */
	    SYNCSIZE_V(v_absolute_size) | SYNCSIZE_H(h_absolute_size));
	PVR_REG_WRITE(dc, PVRREG_BRDHORZ,		/* H border */
	    BRDHORZ_START(hborder_start) | BRDHORZ_STOP(hborder_stop));
	PVR_REG_WRITE(dc, PVRREG_DIWCONF, DIWCONF_MAGIC);

	/* RGB / composite */
	*(volatile uint32_t *)
	    SH3_PHYS_TO_P2SEG(0x00702c00) =
	    ((dc->dc_dispflags & PVR_RGBMODE) ? 0 : 3) << 8;

	/* display on */
	PVR_REG_WRITE(dc, PVRREG_DIWMODE,
	    PVR_REG_READ(dc, PVRREG_DIWMODE) | DIWMODE_DE);
}

/* Console support. */

void
pvrcninit(struct consdev *cndev)
{
	struct fb_devconfig *dcp = &pvr_console_dc;
	long defattr;

	pvr_getdevconfig(dcp);
	(*dcp->rinfo.ri_ops.allocattr)(&dcp->rinfo, 0, 0, 0, &defattr);
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
	extern const struct cdevsw wsdisplay_cdevsw;
#endif
	cndev->cn_dev = NODEV;
	cndev->cn_pri = CN_NORMAL;

#if NWSDISPLAY > 0
	unit = 0;
	maj = cdevsw_lookup_major(&wsdisplay_cdevsw);
	if (maj != -1) {
		cndev->cn_pri = CN_INTERNAL;
		cndev->cn_dev = makedev(maj, unit);
	}
#endif
}
