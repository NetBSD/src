/*	$NetBSD: pfb.c,v 1.13.8.1 2006/05/24 10:56:39 yamt Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pfb.c,v 1.13.8.1 2006/05/24 10:56:39 yamt Exp $");

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/systm.h>

#include <dev/pci/pcidevs.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include <dev/rcons/raster.h>
#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wscons_raster.h>
#include <dev/wscons/wsdisplayvar.h>

#include <machine/autoconf.h>
#include <machine/bus.h>

#include "pfbvar.h"

static int pfb_is_console = 0;

int	pfbmatch __P((struct device *, struct cfdata *, void *));
void	pfbattach __P((struct device *, struct device *, void *));
int	pfbprint __P((void *, const char *));

CFATTACH_DECL(pfb, sizeof(struct pfb_softc),
    pfbmatch, pfbattach, NULL, NULL);

struct pfb_devconfig pfb_console_dc;

struct wsdisplay_emulops pfb_emulops = {
	rcons_cursor,
	rcons_mapchar,
	rcons_putchar,
	rcons_copycols,
	rcons_erasecols,
	rcons_copyrows,
	rcons_eraserows,
	rcons_allocattr
};

struct wsscreen_descr pfb_stdscreen = {
	"std",
	0, 0,	/* will be filled in -- XXX shouldn't, it's global */
	&pfb_emulops,
	0, 0,
	WSSCREEN_REVERSE
};

const struct wsscreen_descr *_pfb_scrlist[] = {
	&pfb_stdscreen,
	/* XXX other formats, graphics screen? */
};

struct wsscreen_list pfb_screenlist = {
	sizeof(_pfb_scrlist) / sizeof(struct wsscreen_descr *), _pfb_scrlist
};

static int pfb_ioctl __P((void *, void *, u_long, caddr_t, int, struct lwp *));
static paddr_t pfb_mmap __P((void *, void *, off_t, int));
static int pfb_alloc_screen __P((void *, const struct wsscreen_descr *,
				void **, int *, int *, long *));
static void pfb_free_screen __P((void *, void *));
static int pfb_show_screen __P((void *, void *, int,
				void (*) (void *, int, int), void *));

struct wsdisplay_accessops pfb_accessops = {
	pfb_ioctl,
	pfb_mmap,
	pfb_alloc_screen,
	pfb_free_screen,
	pfb_show_screen,
	0 /* load_font */
};

static void pfb_common_init __P((int addr, struct pfb_devconfig *));

int
pfbmatch(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct pci_attach_args *pa = aux;

	if (PCI_CLASS(pa->pa_class) == PCI_CLASS_DISPLAY)
		return 1;

	return 0;
}

void
pfbattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct pfb_softc *sc = (struct pfb_softc *)self;
	/* struct pci_attach_args *pa = aux; */
	struct wsemuldisplaydev_attach_args a;
	struct pfb_devconfig *dc;

	if (pfb_is_console) {
		dc = &pfb_console_dc;
		sc->nscreens = 1;
	} else {
		return;
	}
	sc->sc_dc = dc;

	if (dc->dc_paddr == 0) {
		printf(": cannot map framebuffer\n");
		return;
	}
	dc->dc_raster.pixels = (u_int32_t *)mapiodev(dc->dc_paddr,
				dc->dc_linebytes * dc->dc_height);

	printf(": %d x %d, %dbpp\n",
	    dc->dc_raster.width, dc->dc_raster.height, dc->dc_raster.depth);

	a.console = pfb_is_console;
	a.scrdata = &pfb_screenlist;
	a.accessops = &pfb_accessops;
	a.accesscookie = sc;

	config_found(self, &a, wsemuldisplaydevprint);
}

void
pfb_common_init(addr, dc)
	int addr;
	struct pfb_devconfig *dc;
{
	struct raster *rap;
	struct rcons *rcp;
	int i;

	dc->dc_paddr = addr;	/* PA of the frame buffer */

	/* initialize the raster */
	rap = &dc->dc_raster;
	rap->width = 640;
	rap->height = 480;
	rap->depth = 8;
	rap->linelongs = rap->width / sizeof(u_int32_t);
	rap->pixels = (u_int32_t *)addr;

	/* initialize the raster console blitter */
	rcp = &dc->dc_rcons;
	rcp->rc_sp = rap;
	rcp->rc_crow = rcp->rc_ccol = -1;
	rcp->rc_crowp = &rcp->rc_crow;
	rcp->rc_ccolp = &rcp->rc_ccol;
	rcons_init(rcp, 128, 128);

	pfb_stdscreen.nrows = dc->dc_rcons.rc_maxrow;
	pfb_stdscreen.ncols = dc->dc_rcons.rc_maxcol;

	for (i = 0; i < rap->width * rap->height; i += sizeof(u_int32_t))
		*(u_int32_t *)(addr + i) = 0;
}

int
pfb_ioctl(v, vs, cmd, data, flag, l)
	void *v;
	void *vs;
	u_long cmd;
	caddr_t data;
	int flag;
	struct lwp *l;
{
	struct pfb_softc *sc = v;
	struct pfb_devconfig *dc = sc->sc_dc;
	struct wsdisplay_fbinfo *wdf;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = WSDISPLAY_TYPE_PCIMISC;	/* XXX ? */
		return 0;

	case WSDISPLAYIO_GINFO:
		wdf = (void *)data;
		wdf->height = dc->dc_raster.height;
		wdf->width = dc->dc_raster.width;
		wdf->depth = dc->dc_raster.depth;
		wdf->cmsize = 256;
		return 0;
	}
	return EPASSTHROUGH;
}

paddr_t
pfb_mmap(v, vs, offset, prot)
	void *v;
	void *vs;
	off_t offset;
	int prot;
{
	struct pfb_softc *sc = v;
	struct pfb_devconfig *dc = sc->sc_dc;

	if (offset > (dc->dc_linebytes * dc->dc_height) || offset < 0)
		return -1;

	return dc->dc_paddr + offset;
}

int
pfb_alloc_screen(v, type, cookiep, curxp, curyp, attrp)
	void *v;
	const struct wsscreen_descr *type;
	void **cookiep;
	int *curxp, *curyp;
	long *attrp;
{
	struct pfb_softc *sc = v;
	long defattr;

	if (sc->nscreens > 0)
		return (ENOMEM);

	*cookiep = &sc->sc_dc->dc_rcons; /* one and only for now */
	*curxp = 0;
	*curyp = 0;
	rcons_allocattr(&sc->sc_dc->dc_rcons, 0, 0, 0, &defattr);
	*attrp = defattr;
	sc->nscreens++;
	return 0;
}

void
pfb_free_screen(v, cookie)
	void *v;
	void *cookie;
{
	struct pfb_softc *sc = v;

	if (sc->sc_dc == &pfb_console_dc)
		panic("pfb_free_screen: console");

	sc->nscreens--;
}

int
pfb_show_screen(v, cookie, waitok, cb, cbarg)
	void *v;
	void *cookie;
	int waitok;
	void (*cb) __P((void *, int, int));
	void *cbarg;
{

	return (0);
}

int
pfb_cnattach(addr)
	int addr;
{
	struct pfb_devconfig *dc = &pfb_console_dc;
	long defattr;

	pfb_common_init(addr, dc);
	rcons_allocattr(&dc->dc_rcons, 0, 0, 0, &defattr);
	wsdisplay_cnattach(&pfb_stdscreen, &dc->dc_rcons, 0, 0, defattr);

	pfb_is_console = 1;

	return 0;
}
