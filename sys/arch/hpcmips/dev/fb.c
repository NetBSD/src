/*	$NetBSD: fb.c,v 1.11 1999/12/16 09:46:56 sato Exp $	*/

/*-
 * Copyright (c) 1999
 *         Shin Takemura and PocketBSD Project. All rights reserved.
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
 *	This product includes software developed by the PocketBSD project
 *	and its contributors.
 * 4. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
/*
 * Copyright (c) 1995, 1996 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 * 
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS" 
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND 
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */
#define FBDEBUG
/*
#define WSCONS_FONT_HACK
#define USE_RASTERCONS
 */
static const char _copyright[] __attribute__ ((unused)) =
    "Copyright (c) 1999 Shin Takemura.  All rights reserved.";
static const char _rcsid[] __attribute__ ((unused)) =
    "$Id: fb.c,v 1.11 1999/12/16 09:46:56 sato Exp $";


#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/malloc.h>
#include <sys/buf.h>
#include <sys/ioctl.h>

#include <vm/vm.h>

#include <machine/bus.h>
#include <machine/autoconf.h>
#include <machine/bootinfo.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>

#ifdef USE_RASTERCONS
#include <dev/rcons/raster.h>
#include <dev/wscons/wscons_raster.h>
#else /*  USE_RASTERCONS */
#include <dev/wsfont/wsfont.h>
#include <dev/rasops/rasops.h>
#endif /*  USE_RASTERCONS */

#include <arch/hpcmips/dev/fbvar.h>

#ifdef FBDEBUG
int	fb_debug = 1;
#define	DPRINTF(arg) if (fb_debug) printf arg;
#else
#define	DPRINTF(arg)
#endif

/*
 *  function prototypes
 */
int	fbmatch __P((struct device *, struct cfdata *, void *));
void	fbattach __P((struct device *, struct device *, void *));
int	fbprint __P((void *, const char *));

int	fb_ioctl __P((void *, u_long, caddr_t, int, struct proc *));
int	fb_mmap __P((void *, off_t, int));
static int	fb_getdevconfig __P((struct fb_devconfig *dc));
static int	fb_alloc_screen __P((void *, const struct wsscreen_descr *,
				     void **, int *, int *, long *));
static void	fb_free_screen __P((void *, void *));
static int	fb_show_screen __P((void *, void *, int,
				    void (*) (void *, int, int), void *));

static int	pow __P((int, int));
#if defined(USE_RASTERCONS) & defined(WSCONS_FONT_HACK)
#include <arch/hpcmips/dev/vt220l8x10.h>
static void	rcons_init2 __P((struct rcons *rc, int mrow, int mcol));
void	rcons_initfont __P((struct rcons *, struct raster_font *));
#endif

/*
 *  static variables
 */
struct cfattach fb_ca = {
	sizeof(struct fb_softc), fbmatch, fbattach,
};

#ifdef USE_RASTERCONS
struct wsdisplay_emulops fb_emulops = {
	rcons_cursor,
	rcons_mapchar,
	rcons_putchar,
	rcons_copycols,
	rcons_erasecols,
	rcons_copyrows,
	rcons_eraserows,
	rcons_alloc_attr
};
#endif /* USE_RASTERCONS */

struct wsscreen_descr fb_stdscreen = {
	"std",
	0, 0,	/* will be filled in -- XXX shouldn't, it's global */
#ifdef USE_RASTERCONS
	&fb_emulops,
#else
	0,
#endif /* USE_RASTERCONS */
	0, 0,
	WSSCREEN_REVERSE
};

const struct wsscreen_descr *_fb_scrlist[] = {
	&fb_stdscreen,
	/* XXX other formats, graphics screen? */
};

struct wsscreen_list fb_screenlist = {
	sizeof(_fb_scrlist) / sizeof(struct wsscreen_descr *), _fb_scrlist
};

struct wsdisplay_accessops fb_accessops = {
	fb_ioctl,
	fb_mmap,
	fb_alloc_screen,
	fb_free_screen,
	fb_show_screen,
	0 /* load_font */
};

static int fbconsole, fb_console_type;
struct fb_devconfig fb_console_dc;
struct wsscreen_descr fb_console_screen;

/*
 *  function bodies
 */
static int
pow(int x, int n)
{
	int res = 1;
	while (0 < n--) {
		res *= x;
	}
	return (res);
}

int
fbmatch(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct mainbus_attach_args *ma = aux;
    
	if (strcmp(ma->ma_name, match->cf_driver->cd_name))
		return 0;

	return (1);
}

void
fbattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct fb_softc *sc = (struct fb_softc *)self;
	struct mainbus_attach_args *ma = aux;
	struct wsemuldisplaydev_attach_args wa;

	/* avoid warning */
	ma->ma_iot = ma->ma_iot;

	if (fbconsole) {
		sc->sc_dc = &fb_console_dc;
		sc->nscreens = 1;
	} else {
		sc->sc_dc = (struct fb_devconfig *)
		    malloc(sizeof(struct fb_devconfig), M_DEVBUF, M_WAITOK);
		bzero(sc->sc_dc, sizeof(struct fb_devconfig));
		if (fb_getdevconfig(sc->sc_dc) != 0) {
			return;
		}
	}

#ifdef USE_RASTERCONS
	printf(": raster %dx%d pixels, %d colors, %dx%d chars",
	       sc->sc_dc->dc_raster.width,
	       sc->sc_dc->dc_raster.height,
	       pow(2, sc->sc_dc->dc_raster.depth),
	       sc->sc_dc->dc_rcons.rc_maxcol,
	       sc->sc_dc->dc_rcons.rc_maxrow);
#else /*  USE_RASTERCONS */
	fb_stdscreen.textops = &sc->sc_dc->dc_rinfo.ri_ops;
	fb_stdscreen.nrows = sc->sc_dc->dc_rinfo.ri_rows;
	fb_stdscreen.ncols = sc->sc_dc->dc_rinfo.ri_cols;
	fb_stdscreen.capabilities = sc->sc_dc->dc_rinfo.ri_caps;
	printf(": rasops %dx%d pixels, %d colors, %dx%d chars",
	       sc->sc_dc->dc_rinfo.ri_width,
	       sc->sc_dc->dc_rinfo.ri_height,
	       pow(2, sc->sc_dc->dc_rinfo.ri_depth),
	       sc->sc_dc->dc_rinfo.ri_cols,
	       sc->sc_dc->dc_rinfo.ri_rows);
#endif /*  USE_RASTERCONS */

	printf("\n");

	wa.console = fbconsole;
	wa.scrdata = &fb_screenlist;
	wa.accessops = &fb_accessops;
	wa.accesscookie = sc;

	config_found(self, &wa, wsemuldisplaydevprint);
}

int
fb_cnattach(iot, iobase, type, check)
	bus_space_tag_t iot;
	int iobase;
	int type, check;
{
	long defattr = 0;

	/*
	  We can't probe because we have no real device yet.

	if (check && ! fb_probe(iot, memt))
		return (ENXIO);
	*/

	bzero(&fb_console_dc, sizeof(struct fb_devconfig));
	if (fb_getdevconfig(&fb_console_dc) != 0) {
		return (ENXIO);
	}

#ifdef USE_RASTERCONS
	fb_console_screen = fb_stdscreen;
	fb_console_screen.nrows = fb_console_dc.dc_rcons.rc_maxrow;
	fb_console_screen.ncols = fb_console_dc.dc_rcons.rc_maxcol;
	wsdisplay_cnattach(&fb_console_screen, &fb_console_dc.dc_rcons,
			   0, 0, defattr);
#else /*  USE_RASTERCONS */
	fb_console_screen = fb_stdscreen;
	fb_console_screen.textops = &fb_console_dc.dc_rinfo.ri_ops;
	fb_console_screen.nrows = fb_console_dc.dc_rinfo.ri_rows;
	fb_console_screen.ncols = fb_console_dc.dc_rinfo.ri_cols;
	fb_console_screen.capabilities = fb_console_dc.dc_rinfo.ri_caps;
	fb_console_dc.dc_rinfo.ri_ops.alloc_attr(&fb_console_dc.dc_rinfo,
						 7, 0, 0, &defattr);
	wsdisplay_cnattach(&fb_console_screen, &fb_console_dc.dc_rinfo,
			   0, 0, defattr);
#endif /*  USE_RASTERCONS */

	fbconsole = 1;
	fb_console_type = type;

	return (0);
}

int
fb_getdevconfig(dc)
	struct fb_devconfig *dc;
{
	int i;
	int32_t fg, bg;
	int depth, reverse;
#ifdef USE_RASTERCONS
	struct rcons *rcp;
#else
	struct rasops_info *ri;
#endif

	if (bootinfo == NULL ||
	    bootinfo->fb_addr == 0 ||
	    bootinfo->fb_line_bytes == 0 ||
	    bootinfo->fb_width == 0 ||
	    bootinfo->fb_height == 0) {
		printf(": no frame buffer infomation.\n");
		return (-1);
	}

	dc->dc_width = bootinfo->fb_width;
	dc->dc_height = bootinfo->fb_height;
	dc->dc_rowbytes = bootinfo->fb_line_bytes;
	dc->dc_fbaddr = (vaddr_t)bootinfo->fb_addr;

	reverse = 0;
	switch (bootinfo->fb_type) {
	case BIFB_D2_M2L_0:
	case BIFB_D2_M2L_0x2:
		reverse = 1;
		depth = 2;
		break;
	case BIFB_D2_M2L_3:
	case BIFB_D2_M2L_3x2:
		depth = 2;
		break;
	case BIFB_D8_00:
		reverse = 1;
		depth = 8;
		break;
	case BIFB_D8_FF:
		depth = 8;
		break;
	case BIFB_D16_0000:
		reverse = 1;
		depth = 16;
		break;
	case BIFB_D16_FFFF:
		depth = 16;
		break;
	default:
		printf(": unknown type (=%d).\n", bootinfo->fb_type);
		return (-1);
		break;
	}

	if (!reverse) {
		bg = 0;
		fg = ~0;
	} else {
		bg = ~0;
		fg = 0;
	}

	/* clear the screen */
	for (i = 0;
	     i < dc->dc_height * dc->dc_rowbytes;
	     i += sizeof(u_int32_t)) {
		*(u_int32_t *)(dc->dc_fbaddr + i) = bg;
	}

#ifdef USE_RASTERCONS
	/* initialize the raster */
	dc->dc_raster.width = dc->dc_width;
	dc->dc_raster.height = dc->dc_height;
	dc->dc_raster.depth = depth;
	dc->dc_raster.linelongs = dc->dc_rowbytes / sizeof(u_int32_t);
	dc->dc_raster.pixels = (u_int32_t *)dc->dc_fbaddr;
	dc->dc_raster.data = (caddr_t)dc;

	/* initialize the raster console blitter */
	rcp = &dc->dc_rcons;
	rcp->rc_sp = &dc->dc_raster;
	rcp->rc_crow = rcp->rc_ccol = -1;
	rcp->rc_crowp = &rcp->rc_crow;
	rcp->rc_ccolp = &rcp->rc_ccol;
#ifdef WSCONS_FONT_HACK
	rcons_initfont(rcp, &vt220_8x10);
	rcons_init2(rcp, 34, 80);
#else /* WSCONS_FONT_HACK */
	rcons_init(rcp, 34, 80);
#endif /* WSCONS_FONT_HACK */
#else /* USE_RASTERCONS */
	ri = &dc->dc_rinfo;
	bzero(ri, sizeof(struct rasops_info));
	ri->ri_depth = depth;
	ri->ri_bits = (caddr_t)dc->dc_fbaddr;
	ri->ri_width = dc->dc_width;
	ri->ri_height = dc->dc_height;
	ri->ri_stride = dc->dc_rowbytes;
	ri->ri_flg = RI_FORCEMONO | RI_CURSOR;
	if (rasops_init(ri, 200, 200)) {
		panic("%s(%d): rasops_init() failed!", __FILE__, __LINE__);
	}

	/*
	 *  setup color map
	 *  overriding rasops.c: rasops_init_devcmap().
	 */
	ri->ri_devcmap[0] = bg;
	for (i = 1; i < 16; i++) {
		ri->ri_devcmap[i] = fg;
	}
#endif /* USE_RASTERCONS */
	return (0);
}

int
fb_ioctl(v, cmd, data, flag, p)
	void *v;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	struct fb_softc *sc = v;
	struct fb_devconfig *dc = sc->sc_dc;
	struct wsdisplay_fbinfo *wdf;

	switch (cmd) {
	case WSKBDIO_BELL:
		return (0);
		break;

	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = WSDISPLAY_TYPE_UNKNOWN;	/* XXX ? */
		return 0;

	case WSDISPLAYIO_GINFO:
		wdf = (void *)data;
#ifdef USE_RASTERCONS
		wdf->height = dc->dc_raster.height;
		wdf->width = dc->dc_raster.width;
		wdf->depth = dc->dc_raster.depth;
		wdf->cmsize = 256;	/* XXXX */
#else /* USE_RASTERCONS */
		wdf->height = dc->dc_rinfo.ri_height;
		wdf->width = dc->dc_rinfo.ri_width;
		wdf->depth = dc->dc_rinfo.ri_depth;
		wdf->cmsize = 256;	/* XXXX */
#endif /* USE_RASTERCONS */
		return 0;		

	default:
		if (IOCGROUP(cmd) != 't')
			DPRINTF(("%s(%d): fb_ioctl(%lx, %lx) grp=%c num=%ld\n",
			 __FILE__, __LINE__,
			 cmd, (u_long)data, (char)IOCGROUP(cmd), cmd&0xff));
		break;
	}

	return (-1);
}

int
fb_mmap(v, offset, prot)
	void *v;
	off_t offset;
	int prot;
{

	struct fb_softc *sc = v;
	struct fb_devconfig *dc = sc->sc_dc;

	if (offset >= (dc->dc_rowbytes * dc->dc_height) || offset < 0)
		return -1;

	return dc->dc_fbaddr + offset;
}

int
fb_alloc_screen(v, type, cookiep, curxp, curyp, attrp)
	void *v;
	const struct wsscreen_descr *type;
	void **cookiep;
	int *curxp, *curyp;
	long *attrp;
{
	struct fb_softc *sc = v;

	DPRINTF(("%s(%d): fb_alloc_screen()\n", __FILE__, __LINE__));

	if (sc->nscreens > 0)
		return (ENOMEM);

	*curxp = 0;
	*curyp = 0;
#ifdef USE_RASTERCONS
	*cookiep = &sc->sc_dc->dc_rcons; /* one and only for now */
	rcons_alloc_attr(&sc->sc_dc->dc_rcons, 0, 0, 0, attrp);
#else
	*cookiep = &sc->sc_dc->dc_rinfo;
	sc->sc_dc->dc_rinfo.ri_ops.alloc_attr(&sc->sc_dc->dc_rinfo,
					      7, 0, 0, attrp);
#endif
	sc->nscreens++;
	return (0);
}

void
fb_free_screen(v, cookie)
	void *v;
	void *cookie;
{
	struct fb_softc *sc = v;

	if (sc->sc_dc == &fb_console_dc)
		panic("fb_free_screen: console");

	sc->nscreens--;
}

int
fb_show_screen(v, cookie, waitok, cb, cbarg)
	void *v;
	void *cookie;
	int waitok;
	void (*cb) __P((void *, int, int));
	void *cbarg;
{
	DPRINTF(("%s(%d): fb_show_screen()\n", __FILE__, __LINE__));
	return (0);
}

#if defined(USE_RASTERCONS) & defined(WSCONS_FONT_HACK)
void
rcons_init2(rc, mrow, mcol)
	struct rcons *rc;
	int mrow, mcol;
{
	struct raster *rp = rc->rc_sp;
	int i;

	if (rc->rc_font == NULL) {
		/*rcons_initfont(rc, &gallant19);
		 */
	}

	i = rp->height / rc->rc_font->height;
	rc->rc_maxrow = min(i, mrow);

	i = rp->width / rc->rc_font->width;
	rc->rc_maxcol = min(i, mcol);

	/* Center emulator screen (but align x origin to 32 bits) */
	rc->rc_xorigin =
	    ((rp->width - rc->rc_maxcol * rc->rc_font->width) / 2) & ~0x1f;
	rc->rc_yorigin =
	    (rp->height - rc->rc_maxrow * rc->rc_font->height) / 2;

	/* Raster width used for row copies */
	rc->rc_raswidth = rc->rc_maxcol * rc->rc_font->width;
	if (rc->rc_raswidth & 0x1f) {
		/* Pad to 32 bits */
		i = (rc->rc_raswidth + 0x1f) & ~0x1f;
		/* Make sure width isn't too wide */
		if (rc->rc_xorigin + i <= rp->width)
			rc->rc_raswidth = i;
	}

	rc->rc_bits = 0;

	/* If cursor position given, assume it's there and drawn. */
	if (*rc->rc_crowp != -1 && *rc->rc_ccolp != -1)
		rc->rc_bits |= RC_CURSOR;
}
#endif /* USE_RASTERCONS & WSCONS_FONT_HACK */
