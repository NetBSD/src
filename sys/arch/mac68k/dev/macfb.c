/* $NetBSD: macfb.c,v 1.1.2.5 1999/11/15 23:31:16 scottr Exp $ */
/*
 * Copyright (c) 1998 Matt DeBergalis
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
 *      This product includes software developed by Matt DeBergalis
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

#include "opt_wsdisplay_compat.h"
#include "grf.h"

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/cpu.h>
#include <machine/bus.h>

#include <machine/grfioctl.h>
#include <mac68k/nubus/nubus.h>
#include <mac68k/dev/grfvar.h>
#include <mac68k/dev/macfbvar.h>
#include <dev/wscons/wsconsio.h>

#ifdef WSDISPLAY_COMPAT_ITE
#include <machine/iteioctl.h>
#endif /* WSDISPLAY_COMPAT_ITE */

#include <dev/rcons/raster.h>
#include <dev/wscons/wscons_raster.h>
#include <dev/wscons/wsdisplayvar.h>

int macfb_match __P((struct device *, struct cfdata *, void *));
void macfb_attach __P((struct device *, struct device *, void *));

struct cfattach macfb_ca = {
	sizeof(struct macfb_softc), 
	macfb_match,
	macfb_attach,
};

const struct wsdisplay_emulops macfb_emulops = {
	rcons_cursor,
	rcons_mapchar,
	rcons_putchar,
	rcons_copycols,
	rcons_erasecols,
	rcons_copyrows,
	rcons_eraserows,
	rcons_alloc_attr
};

struct wsscreen_descr macfb_stdscreen = {
	"std",
	0, 0, /* will be filled in -- XXX shouldn't, it's global */
	&macfb_emulops,
	0, 0,
	WSSCREEN_REVERSE
};

const struct wsscreen_descr *_macfb_scrlist[] = {
	&macfb_stdscreen,
};

const struct wsscreen_list macfb_screenlist = {
	sizeof(_macfb_scrlist) / sizeof(struct wsscreen_descr *),
	_macfb_scrlist
};

static int	macfb_ioctl __P((void *, u_long, caddr_t, int, struct proc *));
static int	macfb_mmap __P((void *, off_t, int));
static int	macfb_alloc_screen __P((void *, const struct wsscreen_descr *,
		void **, int *, int *, long *));
static void	macfb_free_screen __P((void *, void *));
static void	macfb_show_screen __P((void *, void *));

const struct wsdisplay_accessops macfb_accessops = {
	macfb_ioctl,
	macfb_mmap,
	macfb_alloc_screen,
	macfb_free_screen,
	macfb_show_screen,
	0 /* load_font */
};

void macfb_init __P((struct macfb_devconfig *));

paddr_t macfb_consaddr;
static int macfb_is_console __P((paddr_t addr));
#ifdef WSDISPLAY_COMPAT_ITEFONT
static void	init_itefont __P((void));
#endif /* WSDISPLAY_COMPAT_ITEFONT */

static struct macfb_devconfig macfb_console_dc;

/* From Booter via locore */
extern long		videoaddr;
extern long		videorowbytes;
extern long		videobitdepth;
extern u_long		videosize;
extern u_int32_t	mac68k_vidlog;
extern u_int32_t	mac68k_vidphys;
extern u_int32_t	mac68k_vidlen;

static int
macfb_is_console(paddr_t addr)
{
	return ((mac68k_machine.serial_console & 0x03) == 0
	    && (addr == macfb_consaddr));
}

void
macfb_init(dc)
	struct macfb_devconfig *dc;
{
	struct raster *rap;
	struct rcons *rcp;
	int i;

	/* clear the display */
	for (i = 0; i < (dc->dc_ht * dc->dc_rowbytes); i += dc->dc_rowbytes)
		bzero((u_char *)dc->dc_vaddr + dc->dc_offset + i,
		    dc->dc_rowbytes);

#ifdef WSDISPLAY_COMPAT_ITEFONT
	init_itefont();
#endif /* WSDISPLAY_COMPAT_ITEFONT */

	rap = &dc->dc_raster;
	rap->width = dc->dc_wid;
	rap->height = dc->dc_ht;
	rap->depth = dc->dc_depth;
	rap->linelongs = dc->dc_rowbytes / sizeof(u_int32_t);
	rap->pixels = (u_int32_t *)dc->dc_vaddr + dc->dc_offset;

	/* initialize the raster console blitter */
	rcp = &dc->dc_rcons;
	rcp->rc_sp = rap;
	rcp->rc_crow = rcp->rc_ccol = -1;
	rcp->rc_crowp = &rcp->rc_crow;
	rcp->rc_ccolp = &rcp->rc_ccol;
	rcons_init(rcp, 128, 128);

	macfb_stdscreen.nrows = dc->dc_rcons.rc_maxrow;
	macfb_stdscreen.ncols = dc->dc_rcons.rc_maxcol;
}

int
macfb_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	return (1);
}

void
macfb_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct grfbus_attach_args *ga = aux;
	struct grfmode *gm = ga->ga_grfmode;
	struct macfb_softc *sc;
	struct wsemuldisplaydev_attach_args waa;
	int isconsole;

	sc = (struct macfb_softc *)self;

	printf("\n");

	isconsole = macfb_is_console(ga->ga_phys + ga->ga_grfmode->fboff);

	if (isconsole) {
		sc->sc_dc = &macfb_console_dc;
		sc->nscreens = 1;
	} else {
		sc->sc_dc = malloc(sizeof(struct macfb_devconfig), M_DEVBUF, M_WAITOK);
		sc->sc_dc->dc_vaddr = (vaddr_t)gm->fbbase;
		sc->sc_dc->dc_paddr = ga->ga_phys;
		sc->sc_dc->dc_size = gm->fbsize;

		sc->sc_dc->dc_wid = gm->width;
		sc->sc_dc->dc_ht = gm->height;
		sc->sc_dc->dc_depth = gm->psize;
		sc->sc_dc->dc_rowbytes = gm->rowbytes;

		sc->sc_dc->dc_offset = gm->fboff;

		macfb_init(sc->sc_dc);

		sc->nscreens = 1;
	}

	/* initialize the raster */
	waa.console = isconsole;
	waa.scrdata = &macfb_screenlist;
	waa.accessops = &macfb_accessops;
	waa.accesscookie = sc;

	config_found(self, &waa, wsemuldisplaydevprint);

#if NGRF > 0
	grf_attach(sc, self->dv_unit);
#endif
}


int
macfb_ioctl(v, cmd, data, flag, p)
	void *v;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	struct macfb_softc *sc = v;
	struct macfb_devconfig *dc = sc->sc_dc;
	struct wsdisplay_fbinfo *wdf;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(int *)data = dc->dc_type;
		return 0;

	case WSDISPLAYIO_GINFO:
		wdf = (struct wsdisplay_fbinfo *)data;
		wdf->height = dc->dc_raster.height;
		wdf->width = dc->dc_raster.width;
		wdf->depth = dc->dc_raster.depth;
		wdf->cmsize = 256;
		return 0;

	case WSDISPLAYIO_GCURMAX:
	case WSDISPLAYIO_GCURPOS:
	case WSDISPLAYIO_GCURSOR:
	case WSDISPLAYIO_GETCMAP:
	case WSDISPLAYIO_GVIDEO:
	case WSDISPLAYIO_PUTCMAP:
	case WSDISPLAYIO_SCURPOS:
	case WSDISPLAYIO_SCURSOR:
	case WSDISPLAYIO_SVIDEO:
		/* NONE of these operations are supported. */
		return ENOTTY;

#ifdef WSDISPLAY_COMPAT_ITE
	case ITEIOC_GETBELL:
	case ITEIOC_SETBELL:
	case ITEIOC_RINGBELL:
		/* NONE of these operations are supported. */
		return ENOTTY;

#endif /* WSDISPLAY_COMPAT_ITE */
	}

	return -1;
}

static int
macfb_mmap(v, offset, prot)
	void *v;
	off_t offset;
	int prot;
{
	struct macfb_softc *sc = v;
	struct macfb_devconfig *dc = sc->sc_dc;
	u_long addr;

	if (offset >= 0 &&
	    offset < m68k_round_page(dc->dc_rowbytes * dc->dc_ht))
		addr = m68k_btop(dc->dc_paddr + dc->dc_offset + offset);
	else
		addr = (-1);	/* XXX bogus */

	return (int)addr;
}

int
macfb_alloc_screen(v, type, cookiep, curxp, curyp, defattrp)
	void *v;
	const struct wsscreen_descr *type;
	void **cookiep;
	int *curxp, *curyp;
	long *defattrp;
{
	struct macfb_softc *sc = v;
	long defattr;

	if (sc->nscreens > 0)
		return (ENOMEM);

	*cookiep = &sc->sc_dc->dc_rcons; /* one and only for now */
	*curxp = 0;
	*curyp = 0;
	rcons_alloc_attr(&sc->sc_dc->dc_rcons, 0, 0, 0, &defattr);
	*defattrp = defattr;
	sc->nscreens++;
	return (0);
}

void
macfb_free_screen(v, cookie)
	void *v;
	void *cookie;
{
	struct macfb_softc *sc = v;

	if (sc->sc_dc == &macfb_console_dc)
		panic("cfb_free_screen: console");

	sc->nscreens--;
}

void
macfb_show_screen(v, cookie)
	void *v;
	void *cookie;
{
}

int
macfb_cnattach(addr)
	paddr_t addr;
{
	struct macfb_devconfig *dc = &macfb_console_dc;
	long defattr;

	dc->dc_vaddr = videoaddr;
	dc->dc_paddr = mac68k_vidphys;

	dc->dc_wid = videosize & 0xffff;
	dc->dc_ht = (videosize >> 16) & 0xffff;
	dc->dc_depth = videobitdepth;
	dc->dc_rowbytes = videorowbytes;

	dc->dc_size = (mac68k_vidlen > 0) ?
	    mac68k_vidlen : dc->dc_ht * dc->dc_rowbytes;
	dc->dc_offset = 0;

	/* set up the display */
	macfb_init(&macfb_console_dc);

	rcons_alloc_attr(&dc->dc_rcons, 0, 0, 0, &defattr);

	wsdisplay_cnattach(&macfb_stdscreen, &dc->dc_rcons,
			0, 0, defattr);

	macfb_consaddr = addr;
	dc->isconsole = 1;
	return (0);
}

#ifdef WSDISPLAY_COMPAT_ITEFONT
#include <mac68k/dev/6x10.h>

void
init_itefont()
{
	static int itefont_initted = 0;
	int i, j;

	extern struct raster_font gallant19;		/* XXX */

	if (itefont_initted)
		return;
	itefont_initted = 1;

	/* XXX but we cannot use malloc here... */
	gallant19.width = 6;
	gallant19.height = 10;
	gallant19.ascent = 0;

	for (i = 32; i < 128; i++) {
		u_int *p;

		if (gallant19.chars[i].r == NULL)
			continue;

		gallant19.chars[i].r->width = 6;
		gallant19.chars[i].r->height = 10;
		p = gallant19.chars[i].r->pixels;

		for (j = 0; j < 10; j++)
			*p++ = Font6x10[i * 10 + j] << 26;
	}
}
#endif /* WSDISPLAY_COMPAT_ITEFONT */
