/*	$NetBSD: hpcfb.c,v 1.1 2000/03/12 05:04:46 takemura Exp $	*/

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
#define FBDEBUG
static const char _copyright[] __attribute__ ((unused)) =
    "Copyright (c) 1999 Shin Takemura.  All rights reserved.";
static const char _rcsid[] __attribute__ ((unused)) =
    "$Id: hpcfb.c,v 1.1 2000/03/12 05:04:46 takemura Exp $";


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

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>

#include <dev/wsfont/wsfont.h>
#include <dev/rasops/rasops.h>

#include <arch/hpcmips/dev/hpcfbvar.h>
#include <arch/hpcmips/dev/hpcfbio.h>

#include "bivideo.h"
#if NBIVIDEO > 0
#include <arch/hpcmips/dev/bivideovar.h>
#endif

#ifdef FBDEBUG
int	hpcfb_debug = 0;
#define	DPRINTF(arg) if (hpcfb_debug) printf arg;
#else
#define	DPRINTF(arg)
#endif

struct hpcfb_devconfig {
	struct rasops_info	dc_rinfo;	/* rasops infomation */

	int		dc_blanked;	/* currently had video disabled */
};
	
struct hpcfb_softc {
	struct	device sc_dev;
	struct	hpcfb_devconfig *sc_dc;	/* device configuration */
	const struct hpcfb_accessops	*sc_accessops;
	void *sc_accessctx;
	int nscreens;
};

/*
 *  function prototypes
 */
int	hpcfbmatch __P((struct device *, struct cfdata *, void *));
void	hpcfbattach __P((struct device *, struct device *, void *));
int	hpcfbprint __P((void *aux, const char *pnp));

int	hpcfb_ioctl __P((void *, u_long, caddr_t, int, struct proc *));
int	hpcfb_mmap __P((void *, off_t, int));
static int	hpcfb_init __P((struct hpcfb_fbconf *fbconf,
				struct hpcfb_devconfig *dc));
static int	hpcfb_alloc_screen __P((void *, const struct wsscreen_descr *,
				     void **, int *, int *, long *));
static void	hpcfb_free_screen __P((void *, void *));
static int	hpcfb_show_screen __P((void *, void *, int,
				    void (*) (void *, int, int), void *));

static int	pow __P((int, int));

/*
 *  static variables
 */
struct cfattach hpcfb_ca = {
	sizeof(struct hpcfb_softc), hpcfbmatch, hpcfbattach,
};

struct wsscreen_descr hpcfb_stdscreen = {
	"std",
	0, 0,	/* will be filled in -- XXX shouldn't, it's global */
	0,
	0, 0,
	WSSCREEN_REVERSE
};

const struct wsscreen_descr *_hpcfb_scrlist[] = {
	&hpcfb_stdscreen,
	/* XXX other formats, graphics screen? */
};

struct wsscreen_list hpcfb_screenlist = {
	sizeof(_hpcfb_scrlist) / sizeof(struct wsscreen_descr *),
	_hpcfb_scrlist
};

struct wsdisplay_accessops hpcfb_accessops = {
	hpcfb_ioctl,
	hpcfb_mmap,
	hpcfb_alloc_screen,
	hpcfb_free_screen,
	hpcfb_show_screen,
	0 /* load_font */
};

static int hpcfbconsole, hpcfb_console_type;
struct hpcfb_devconfig hpcfb_console_dc;
struct wsscreen_descr hpcfb_console_screen;

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
hpcfbmatch(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
#if 0
	struct hpcfb_attach_args *fap = aux;
#endif

	return (1);
}

void
hpcfbattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct hpcfb_softc *sc = (struct hpcfb_softc *)self;
	struct hpcfb_attach_args *ha = aux;
	struct wsemuldisplaydev_attach_args wa;

	sc->sc_accessops = ha->ha_accessops;
	sc->sc_accessctx = ha->ha_accessctx;

	if (hpcfbconsole) {
		sc->sc_dc = &hpcfb_console_dc;
		sc->nscreens = 1;
	} else {
		sc->sc_dc = (struct hpcfb_devconfig *)
		    malloc(sizeof(struct hpcfb_devconfig), M_DEVBUF, M_WAITOK);
		bzero(sc->sc_dc, sizeof(struct hpcfb_devconfig));
		if (hpcfb_init(&ha->ha_fbconflist[0], sc->sc_dc) != 0) {
			return;
		}
	}

	hpcfb_stdscreen.textops = &sc->sc_dc->dc_rinfo.ri_ops;
	hpcfb_stdscreen.nrows = sc->sc_dc->dc_rinfo.ri_rows;
        hpcfb_stdscreen.ncols = sc->sc_dc->dc_rinfo.ri_cols;
	hpcfb_stdscreen.capabilities = sc->sc_dc->dc_rinfo.ri_caps;
	printf(": rasops %dx%d pixels, %d colors, %dx%d chars",
	       sc->sc_dc->dc_rinfo.ri_width,
	       sc->sc_dc->dc_rinfo.ri_height,
	       pow(2, sc->sc_dc->dc_rinfo.ri_depth),
	       sc->sc_dc->dc_rinfo.ri_cols,
	       sc->sc_dc->dc_rinfo.ri_rows);
	printf("\n");

	wa.console = hpcfbconsole;
	wa.scrdata = &hpcfb_screenlist;
	wa.accessops = &hpcfb_accessops;
	wa.accesscookie = sc;

	config_found(self, &wa, wsemuldisplaydevprint);
}

/* Print function (for parent devices). */
int
hpcfbprint(aux, pnp)
	void *aux;
	const char *pnp;
{
#if 0
	struct hpchpcfb_attach_args *fap = aux;
#endif

	if (pnp)
		printf("hpcfb at %s", pnp);

	return (UNCONF);
}

int
hpcfb_cnattach(iot, iobase, type, check)
	bus_space_tag_t iot;
	int iobase;
	int type, check;
{
	long defattr;
	int found;
	struct hpcfb_fbconf fbconf;

	found = 0;
	bzero(&fbconf, sizeof(struct hpcfb_fbconf));

#if NBIVIDEO > 0
	if (!found) found = (bivideo_getcnfb(&fbconf) == 0);
#endif

	bzero(&hpcfb_console_dc, sizeof(struct hpcfb_devconfig));
	if (!found || hpcfb_init(&fbconf, &hpcfb_console_dc) != 0) {
		return (ENXIO);
	}

	hpcfb_console_screen = hpcfb_stdscreen;
	hpcfb_console_screen.textops = &hpcfb_console_dc.dc_rinfo.ri_ops;
	hpcfb_console_screen.nrows = hpcfb_console_dc.dc_rinfo.ri_rows;
	hpcfb_console_screen.ncols = hpcfb_console_dc.dc_rinfo.ri_cols;
	hpcfb_console_screen.capabilities = hpcfb_console_dc.dc_rinfo.ri_caps;
	hpcfb_console_dc.dc_rinfo.ri_ops.alloc_attr(&hpcfb_console_dc.dc_rinfo,
						 7, 0, 0, &defattr);
	wsdisplay_cnattach(&hpcfb_console_screen, &hpcfb_console_dc.dc_rinfo,
			   0, 0, defattr);

	hpcfbconsole = 1;
	hpcfb_console_type = type;

	return (0);
}

int
hpcfb_init(fbconf, dc)
	struct hpcfb_fbconf *fbconf;
	struct hpcfb_devconfig *dc;
{
	int i;
	int32_t fg, bg;
	struct rasops_info *ri;
	vaddr_t fbaddr;

	fbaddr = (vaddr_t)fbconf->hf_baseaddr + fbconf->hf_offset;

	/*
	 * Set forground and background so that the screen 
	 * looks black on white.
	 * Normally, black = 00 and white = ff.
	 * HPCFB_ACCESS_REVERSE means black = ff and white = 00.
	 */
	if (fbconf->hf_access_flags & HPCFB_ACCESS_REVERSE) {
		bg = 0;
		fg = ~0;
	} else {
		bg = ~0;
		fg = 0;
	}

	/* clear the screen */
	for (i = 0;
	     i < fbconf->hf_height * fbconf->hf_bytes_per_line;
	     i += sizeof(u_int32_t)) {
		*(u_int32_t *)(fbaddr + i) = bg;
	}

	ri = &dc->dc_rinfo;
	bzero(ri, sizeof(struct rasops_info));
	ri->ri_depth = fbconf->hf_pixel_width;
	ri->ri_bits = (caddr_t)fbaddr;
	ri->ri_width = fbconf->hf_width;
	ri->ri_height = fbconf->hf_height;
	ri->ri_stride = fbconf->hf_bytes_per_line;
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

	return (0);
}

int
hpcfb_ioctl(v, cmd, data, flag, p)
	void *v;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	struct hpcfb_softc *sc = v;
	struct hpcfb_devconfig *dc = sc->sc_dc;
	struct wsdisplay_fbinfo *wdf;

	switch (cmd) {
	case WSKBDIO_BELL:
		return (0);
		break;

	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = WSDISPLAY_TYPE_HPCFB;
		return 0;

	case WSDISPLAYIO_GINFO:
		wdf = (void *)data;
		wdf->height = dc->dc_rinfo.ri_height;
		wdf->width = dc->dc_rinfo.ri_width;
		wdf->depth = dc->dc_rinfo.ri_depth;
		wdf->cmsize = 256;	/* XXXX */
		return 0;		
		
	case HPCFBIO_GCONF:
	case HPCFBIO_SCONF:
	case HPCFBIO_GDSPCONF:
	case HPCFBIO_SDSPCONF:
	case HPCFBIO_GOP:
	case HPCFBIO_SOP:
		return (*sc->sc_accessops->ioctl)(sc->sc_accessctx,
						  cmd, data, flag, p);

	default:
		if (IOCGROUP(cmd) != 't')
			DPRINTF(("%s(%d): hpcfb_ioctl(%lx, %lx) grp=%c num=%ld\n",
			 __FILE__, __LINE__,
			 cmd, (u_long)data, (char)IOCGROUP(cmd), cmd&0xff));
		break;
	}

	return (ENOTTY); /* Inappropriate ioctl for device */
}

int
hpcfb_mmap(v, offset, prot)
	void *v;
	off_t offset;
	int prot;
{
	struct hpcfb_softc *sc = v;

	return (*sc->sc_accessops->mmap)(sc->sc_accessctx, offset, prot);
}

int
hpcfb_alloc_screen(v, type, cookiep, curxp, curyp, attrp)
	void *v;
	const struct wsscreen_descr *type;
	void **cookiep;
	int *curxp, *curyp;
	long *attrp;
{
	struct hpcfb_softc *sc = v;

	DPRINTF(("%s(%d): hpcfb_alloc_screen()\n", __FILE__, __LINE__));

	if (sc->nscreens > 0)
		return (ENOMEM);

	*curxp = 0;
	*curyp = 0;
	*cookiep = &sc->sc_dc->dc_rinfo;
	sc->sc_dc->dc_rinfo.ri_ops.alloc_attr(&sc->sc_dc->dc_rinfo,
					      7, 0, 0, attrp);
	sc->nscreens++;
	return (0);
}

void
hpcfb_free_screen(v, cookie)
	void *v;
	void *cookie;
{
	struct hpcfb_softc *sc = v;

	if (sc->sc_dc == &hpcfb_console_dc)
		panic("hpcfb_free_screen: console");

	sc->nscreens--;
}

int
hpcfb_show_screen(v, cookie, waitok, cb, cbarg)
	void *v;
	void *cookie;
	int waitok;
	void (*cb) __P((void *, int, int));
	void *cbarg;
{
	DPRINTF(("%s(%d): hpcfb_show_screen()\n", __FILE__, __LINE__));
	return (0);
}
