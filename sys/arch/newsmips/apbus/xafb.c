/*	$NetBSD: xafb.c,v 1.1.10.1 2002/04/01 07:41:40 nathanw Exp $	*/

/*-
 * Copyright (c) 2000 Tsubai Masanari.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* "xa" frame buffer driver.  Currently supports 1280x1024x8 only. */

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>

#include <machine/adrsmap.h>
#include <machine/apcall.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/rasops/rasops.h>

#include <newsmips/apbus/apbusvar.h>

struct xafb_reg {
	volatile u_int r0;
	volatile u_int index;
	volatile u_int r2;
	volatile u_int zero;
	volatile u_int r4;
	volatile u_int r5;
	volatile u_int r6;
	volatile u_int rgb;
};

struct xafb_devconfig {
	volatile u_char *dc_fbbase;	/* VRAM base address */
	struct xafb_reg *dc_reg;	/* register address */
	struct rasops_info dc_ri;
};

struct xafb_softc {
	struct device sc_dev;
	struct xafb_devconfig *sc_dc;
	int sc_nscreens;
	u_char sc_cmap_red[256];
	u_char sc_cmap_green[256];
	u_char sc_cmap_blue[256];
};

int xafb_match __P((struct device *, struct cfdata *, void *));
void xafb_attach __P((struct device *, struct device *, void *));

int xafb_common_init __P((struct xafb_devconfig *));
int xafb_is_console __P((void));

int xafb_ioctl __P((void *, u_long, caddr_t, int, struct proc *));
paddr_t xafb_mmap __P((void *, off_t, int));
int xafb_alloc_screen __P((void *, const struct wsscreen_descr *,
			   void **, int *, int *, long *));
void xafb_free_screen __P((void *, void *));
int xafb_show_screen __P((void *, void *, int,
			  void (*) (void *, int, int), void *));

int xafb_cnattach __P((void));
int xafb_getcmap __P((struct xafb_softc *, struct wsdisplay_cmap *));
int xafb_putcmap __P((struct xafb_softc *, struct wsdisplay_cmap *));

static __inline void xafb_setcolor(struct xafb_devconfig *, int, int, int, int);

struct cfattach xafb_ca = {
	sizeof(struct xafb_softc), xafb_match, xafb_attach,
};

struct xafb_devconfig xafb_console_dc;

struct wsdisplay_accessops xafb_accessops = {
	xafb_ioctl,
	xafb_mmap,
	xafb_alloc_screen,
	xafb_free_screen,
	xafb_show_screen,
	NULL	/* load_font */
};

struct wsscreen_descr xafb_stdscreen = {
	"std",
	0, 0,
	0,
	0, 0,
	WSSCREEN_REVERSE
};

const struct wsscreen_descr *xafb_scrlist[] = {
	&xafb_stdscreen
};

struct wsscreen_list xafb_screenlist = {
	sizeof(xafb_scrlist) / sizeof(xafb_scrlist[0]), xafb_scrlist
};

int
xafb_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct apbus_attach_args *apa = aux;

	if (strcmp(apa->apa_name, "xa") != 0)
		return 0;

	return 1;
}

void
xafb_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct xafb_softc *sc = (void *)self;
	struct apbus_attach_args *apa = aux;
	struct wsemuldisplaydev_attach_args wsa;
	struct xafb_devconfig *dc;
	struct rasops_info *ri;
	int console, i;

	console = xafb_is_console();

	if (console) {
		dc = &xafb_console_dc;
		ri = &dc->dc_ri;
		sc->sc_nscreens = 1;
	} else {
		dc = malloc(sizeof(struct xafb_devconfig), M_DEVBUF, M_WAITOK);
		bzero(dc, sizeof(struct xafb_devconfig));

		dc->dc_fbbase = (void *)0xb0000000;		/* XXX */
		dc->dc_reg = (void *)(apa->apa_hwbase + 0x3000);
		if (xafb_common_init(dc) != 0) {
			printf(": couldn't initialize device\n");
			return;
		}

		ri = &dc->dc_ri;

		/* clear screen */
		(*ri->ri_ops.eraserows)(ri, 0, ri->ri_rows, 0);
	}
	sc->sc_dc = dc;

	for (i = 0; i < 256; i++) {
		sc->sc_cmap_red[i] = i;
		sc->sc_cmap_green[i] = i;
		sc->sc_cmap_blue[i] = i;
	}

	printf(": %d x %d, %dbpp\n", ri->ri_width, ri->ri_height, ri->ri_depth);

	wsa.console = console;
	wsa.scrdata = &xafb_screenlist;
	wsa.accessops = &xafb_accessops;
	wsa.accesscookie = sc;

	config_found(self, &wsa, wsemuldisplaydevprint);
}

void
xafb_setcolor(dc, index, r, g, b)
	struct xafb_devconfig *dc;
	int index, r, g, b;
{
	volatile struct xafb_reg *reg = dc->dc_reg;

	reg->index = index;
	reg->zero = 0;
	reg->rgb = r;
	reg->rgb = g;
	reg->rgb = b;
}

int
xafb_common_init(dc)
	struct xafb_devconfig *dc;
{
	struct rasops_info *ri = &dc->dc_ri;
	int i;

	for (i = 0; i < 256; i++)
		xafb_setcolor(dc, i, i, i, i);

	/* initialize rasops */
	ri->ri_width = 1280;
	ri->ri_height = 1024;
	ri->ri_depth = 8;
	ri->ri_stride = 2048;
	ri->ri_bits = (void *)dc->dc_fbbase;
	ri->ri_flg = RI_FORCEMONO | RI_FULLCLEAR;

	rasops_init(ri, 44, 100);

	/* mono */
	ri->ri_devcmap[0] = 0;				/* bg */
	ri->ri_devcmap[1] = 0xffffffff;			/* fg */

	xafb_stdscreen.nrows = ri->ri_rows;
	xafb_stdscreen.ncols = ri->ri_cols; 
	xafb_stdscreen.textops = &ri->ri_ops;
	xafb_stdscreen.capabilities = ri->ri_caps;

	return 0;
}

int
xafb_is_console()
{
	volatile u_int *dipsw = (void *)NEWS5000_DIP_SWITCH;

	if (*dipsw & 1)					/* XXX right? */
		return 1;

	return 0;
}

int
xafb_ioctl(v, cmd, data, flag, p)
	void *v;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	struct xafb_softc *sc = v;
	struct xafb_devconfig *dc = sc->sc_dc;
	struct wsdisplay_fbinfo *wdf;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(int *)data = WSDISPLAY_TYPE_UNKNOWN;	/* XXX */
		return 0;

	case WSDISPLAYIO_GINFO:
		wdf = (void *)data;
		wdf->height = dc->dc_ri.ri_height;
		wdf->width = dc->dc_ri.ri_width;
		wdf->depth = dc->dc_ri.ri_depth;
		wdf->cmsize = 256;
		return 0;

	case WSDISPLAYIO_GETCMAP:
		return xafb_getcmap(sc, (struct wsdisplay_cmap *)data);

	case WSDISPLAYIO_PUTCMAP:
		return xafb_putcmap(sc, (struct wsdisplay_cmap *)data);

	/* case WSDISPLAYIO_SVIDEO: */
	}
	return EPASSTHROUGH;
}

paddr_t
xafb_mmap(v, offset, prot)
	void *v;
	off_t offset;
	int prot;
{
	struct xafb_softc *sc = v;
	struct xafb_devconfig *dc = sc->sc_dc;
	struct rasops_info *ri = &dc->dc_ri;

	if (offset >= (ri->ri_stride * ri->ri_height) || offset < 0)
		return -1;

	return mips_btop((int)dc->dc_fbbase + offset);
}

int
xafb_alloc_screen(v, scrdesc, cookiep, ccolp, crowp, attrp)
	void *v;
	const struct wsscreen_descr *scrdesc;
	void **cookiep;
	int *ccolp, *crowp;
	long *attrp;
{
	struct xafb_softc *sc = v;
	struct rasops_info *ri = &sc->sc_dc->dc_ri;
	long defattr;

	if (sc->sc_nscreens > 0)
		return ENOMEM;

	*cookiep = ri;
	*ccolp = *crowp = 0;
	(*ri->ri_ops.alloc_attr)(ri, 0, 0, 0, &defattr);
	*attrp = defattr;
	sc->sc_nscreens++;

	return 0;
}

void
xafb_free_screen(v, cookie)
	void *v;
	void *cookie;
{
	struct xafb_softc *sc = v;

	if (sc->sc_dc == &xafb_console_dc)
		panic("xafb_free_screen: console");

	sc->sc_nscreens--;
}

int
xafb_show_screen(v, cookie, waitok, cb, cbarg)
	void *v;
	void *cookie;
	int waitok;
	void (*cb) __P((void *, int, int));
	void *cbarg;
{
	return 0;
}

int
xafb_cnattach()
{
	struct xafb_devconfig *dc = &xafb_console_dc;
	struct rasops_info *ri = &dc->dc_ri;
	long defattr;
	int crow = 0;

	if (!xafb_is_console())
		return -1;

	dc->dc_fbbase = (void *)0xb0000000;			/* XXX */
	dc->dc_reg = (void *)0xb4903000;			/* XXX */
	xafb_common_init(dc);

	crow = 0;			/* XXX current cursor pos */

	(*ri->ri_ops.alloc_attr)(ri, 0, 0, 0, &defattr);
	wsdisplay_cnattach(&xafb_stdscreen, &dc->dc_ri, 0, crow, defattr);

	return 0;
}

int
xafb_getcmap(sc, cm)
	struct xafb_softc *sc;
	struct wsdisplay_cmap *cm;
{
	u_int index = cm->index;
	u_int count = cm->count;
	int error;

	if (index >= 256 || count > 256 || index + count > 256)
		return EINVAL;

	error = copyout(&sc->sc_cmap_red[index],   cm->red,   count);
	if (error)
		return error;
	error = copyout(&sc->sc_cmap_green[index], cm->green, count);
	if (error)
		return error;
	error = copyout(&sc->sc_cmap_blue[index],  cm->blue,  count);
	if (error)
		return error;

	return 0;
}

int
xafb_putcmap(sc, cm)
	struct xafb_softc *sc;
	struct wsdisplay_cmap *cm;
{
	u_int index = cm->index;
	u_int count = cm->count;
	int i;
	u_char *r, *g, *b;

	if (index >= 256 || count > 256 || index + count > 256)
		return EINVAL;
	if (!uvm_useracc(cm->red,   count, B_READ) ||
	    !uvm_useracc(cm->green, count, B_READ) ||
	    !uvm_useracc(cm->blue,  count, B_READ))
		return EFAULT;
	copyin(cm->red,   &sc->sc_cmap_red[index],   count);
	copyin(cm->green, &sc->sc_cmap_green[index], count);
	copyin(cm->blue,  &sc->sc_cmap_blue[index],  count);

	r = &sc->sc_cmap_red[index];
	g = &sc->sc_cmap_green[index];
	b = &sc->sc_cmap_blue[index];

	for (i = 0; i < count; i++)
		xafb_setcolor(sc->sc_dc, index++, *r++, *g++, *b++);

	return 0;
}
