/*	$NetBSD: xafb.c,v 1.17.6.1 2016/10/05 20:55:33 skrll Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: xafb.c,v 1.17.6.1 2016/10/05 20:55:33 skrll Exp $");

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>

#include <mips/locore.h>

#include <machine/adrsmap.h>
#include <machine/apcall.h>
#include <machine/wsconsio.h>

#include <dev/wscons/wsdisplayvar.h>
#include <dev/rasops/rasops.h>

#include <newsmips/apbus/apbusvar.h>

struct xafb_reg {
	volatile uint32_t r0;
	volatile uint32_t index;
	volatile uint32_t r2;
	volatile uint32_t zero;
	volatile uint32_t r4;
	volatile uint32_t r5;
	volatile uint32_t r6;
	volatile uint32_t rgb;
};

struct xafb_devconfig {
	uint8_t *dc_fbbase;		/* VRAM base address */
	paddr_t dc_fbpaddr;		/* VRAM physical address */
	struct xafb_reg *dc_reg;	/* register address */
	struct rasops_info dc_ri;
};

struct xafb_softc {
	device_t sc_dev;
	struct xafb_devconfig *sc_dc;
	int sc_nscreens;
	uint8_t sc_cmap_red[256];
	uint8_t sc_cmap_green[256];
	uint8_t sc_cmap_blue[256];
};

int xafb_match(device_t, cfdata_t, void *);
void xafb_attach(device_t, device_t, void *);

int xafb_common_init(struct xafb_devconfig *);
int xafb_is_console(void);

int xafb_ioctl(void *, void *, u_long, void *, int, struct lwp *);
paddr_t xafb_mmap(void *, void *, off_t, int);
int xafb_alloc_screen(void *, const struct wsscreen_descr *, void **, int *,
    int *, long *);
void xafb_free_screen(void *, void *);
int xafb_show_screen(void *, void *, int, void (*) (void *, int, int), void *);

int xafb_cnattach(void);
int xafb_getcmap(struct xafb_softc *, struct wsdisplay_cmap *);
int xafb_putcmap(struct xafb_softc *, struct wsdisplay_cmap *);

static inline void xafb_setcolor(struct xafb_devconfig *, int, int, int, int);

CFATTACH_DECL_NEW(xafb, sizeof(struct xafb_softc),
    xafb_match, xafb_attach, NULL, NULL);

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
	__arraycount(xafb_scrlist), xafb_scrlist
};

int
xafb_match(device_t parent, cfdata_t cf, void *aux)
{
	struct apbus_attach_args *apa = aux;

	if (strcmp(apa->apa_name, "xa") != 0)
		return 0;

	return 1;
}

void
xafb_attach(device_t parent, device_t self, void *aux)
{
	struct xafb_softc *sc = device_private(self);
	struct apbus_attach_args *apa = aux;
	struct wsemuldisplaydev_attach_args wsa;
	struct xafb_devconfig *dc;
	struct rasops_info *ri;
	int console, i;

	sc->sc_dev = self;
	console = xafb_is_console();

	if (console) {
		dc = &xafb_console_dc;
		ri = &dc->dc_ri;
		ri->ri_flg &= ~RI_NO_AUTO;
		sc->sc_nscreens = 1;
	} else {
		dc = malloc(sizeof(struct xafb_devconfig), M_DEVBUF,
		    M_WAITOK|M_ZERO);
		dc->dc_fbpaddr = (paddr_t)0x10000000;
		dc->dc_fbbase = (void *)MIPS_PHYS_TO_KSEG1(dc->dc_fbpaddr);
		dc->dc_reg = (void *)(apa->apa_hwbase + 0x3000);
		if (xafb_common_init(dc) != 0) {
			aprint_error(": couldn't initialize device\n");
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

	aprint_normal(": %d x %d, %dbpp\n",
	    ri->ri_width, ri->ri_height, ri->ri_depth);

	wsa.console = console;
	wsa.scrdata = &xafb_screenlist;
	wsa.accessops = &xafb_accessops;
	wsa.accesscookie = sc;

	config_found(self, &wsa, wsemuldisplaydevprint);
}

void
xafb_setcolor(struct xafb_devconfig *dc, int index, int r, int g, int b)
{
	volatile struct xafb_reg *reg = dc->dc_reg;

	reg->index = index;
	reg->zero = 0;
	reg->rgb = r;
	reg->rgb = g;
	reg->rgb = b;
}

int
xafb_common_init(struct xafb_devconfig *dc)
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
	if (dc == &xafb_console_dc)
		ri->ri_flg |= RI_NO_AUTO;

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
xafb_is_console(void)
{
	volatile uint32_t *dipsw = (void *)NEWS5000_DIP_SWITCH;

	if (*dipsw & 1)					/* XXX right? */
		return 1;

	return 0;
}

int
xafb_ioctl(void *v, void *vs, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct xafb_softc *sc = v;
	struct xafb_devconfig *dc = sc->sc_dc;
	struct newsmips_wsdisplay_fbinfo *nwdf = (void *)data;
	struct wsdisplay_fbinfo *wdf = (void *)data;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(int *)data = WSDISPLAY_TYPE_UNKNOWN;	/* XXX */
		return 0;

	case NEWSMIPS_WSDISPLAYIO_GINFO:
		nwdf->stride = dc->dc_ri.ri_stride;
		/* FALLTHROUGH */
	case WSDISPLAYIO_GINFO:
		wdf->height = dc->dc_ri.ri_height;
		wdf->width = dc->dc_ri.ri_width;
		wdf->depth = dc->dc_ri.ri_depth;
		wdf->cmsize = 256;
		return 0;

	case WSDISPLAYIO_LINEBYTES:
		*(u_int *)data = dc->dc_ri.ri_stride;
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
xafb_mmap(void *v, void *vs, off_t offset, int prot)
{
	struct xafb_softc *sc = v;
	struct xafb_devconfig *dc = sc->sc_dc;
	struct rasops_info *ri = &dc->dc_ri;

	if (offset >= (ri->ri_stride * ri->ri_height) || offset < 0)
		return -1;

	return mips_btop(dc->dc_fbpaddr + offset);
}

int
xafb_alloc_screen(void *v, const struct wsscreen_descr *scrdesc,
    void **cookiep, int *ccolp, int *crowp, long *attrp)
{
	struct xafb_softc *sc = v;
	struct rasops_info *ri = &sc->sc_dc->dc_ri;
	long defattr;

	if (sc->sc_nscreens > 0)
		return ENOMEM;

	*cookiep = ri;
	*ccolp = *crowp = 0;
	(*ri->ri_ops.allocattr)(ri, 0, 0, 0, &defattr);
	*attrp = defattr;
	sc->sc_nscreens++;

	return 0;
}

void
xafb_free_screen(void *v, void *cookie)
{
	struct xafb_softc *sc = v;

	if (sc->sc_dc == &xafb_console_dc)
		panic("xafb_free_screen: console");

	sc->sc_nscreens--;
}

int
xafb_show_screen(void *v, void *cookie, int waitok,
    void (*cb)(void *, int, int), void *cbarg)
{

	return 0;
}

int
xafb_cnattach(void)
{
	struct xafb_devconfig *dc = &xafb_console_dc;
	struct rasops_info *ri = &dc->dc_ri;
	long defattr;
	int crow = 0;

	if (!xafb_is_console())
		return -1;

	dc->dc_fbpaddr = (paddr_t)0x10000000;
	dc->dc_fbbase = (void *)MIPS_PHYS_TO_KSEG1(dc->dc_fbpaddr);
	dc->dc_reg = (void *)0xb4903000;			/* XXX */
	xafb_common_init(dc);

	crow = 0;			/* XXX current cursor pos */

	(*ri->ri_ops.allocattr)(ri, 0, 0, 0, &defattr);
	wsdisplay_cnattach(&xafb_stdscreen, &dc->dc_ri, 0, crow, defattr);

	return 0;
}

int
xafb_getcmap(struct xafb_softc *sc, struct wsdisplay_cmap *cm)
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
xafb_putcmap(struct xafb_softc *sc, struct wsdisplay_cmap *cm)
{
	u_int index = cm->index;
	u_int count = cm->count;
	int i, error;
	u_char rbuf[256], gbuf[256], bbuf[256];
	u_char *r, *g, *b;

	if (cm->index >= 256 || cm->count > 256 ||
	    (cm->index + cm->count) > 256)
		return EINVAL;
	error = copyin(cm->red, &rbuf[index], count);
	if (error)
		return error;
	error = copyin(cm->green, &gbuf[index], count);
	if (error)
		return error;
	error = copyin(cm->blue, &bbuf[index], count);
	if (error)
		return error;

	memcpy(&sc->sc_cmap_red[index], &rbuf[index], count);
	memcpy(&sc->sc_cmap_green[index], &gbuf[index], count);
	memcpy(&sc->sc_cmap_blue[index], &bbuf[index], count);

	r = &sc->sc_cmap_red[index];
	g = &sc->sc_cmap_green[index];
	b = &sc->sc_cmap_blue[index];
	for (i = 0; i < count; i++)
		xafb_setcolor(sc->sc_dc, index++, *r++, *g++, *b++);
	return 0;
}
