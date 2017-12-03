/*	$NetBSD: pm.c,v 1.11.6.2 2017/12/03 11:36:35 jdolecek Exp $	*/

/*-
 * Copyright (c) 2002, 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pm.c,v 1.11.6.2 2017/12/03 11:36:35 jdolecek Exp $");

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/intr.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/rasops/rasops.h>
#include <dev/wsfont/wsfont.h>

#include <pmax/pmax/kn01.h>

#include <pmax/ibus/ibusvar.h>
#include <pmax/ibus/pmreg.h>

#include <uvm/uvm_extern.h>

#define	CURSOR_MAX_SIZE	16

struct hwcmap256 {
	uint8_t r[256];
	uint8_t g[256];
	uint8_t b[256];
};

struct hwcursor64 {
	struct wsdisplay_curpos cc_pos;
	struct wsdisplay_curpos cc_hot;
	struct wsdisplay_curpos cc_size;
	uint8_t cc_color[6];

	/*
	 * Max cursor size is 16x16.  The X server pads bitmap scanlines to
	 * a word boundary.  We take the easy route and waste some space.
	 */
	u_short cc_image[32 + 32];
};

struct pm_softc {
	device_t		sc_dev;
	size_t			sc_cmap_size;
	size_t			sc_fb_size;
	int			sc_type;
	int			sc_blanked;
	int			sc_curenb;
	int			sc_changed;
	int			sc_nscreens;
	struct hwcursor64	sc_cursor;
	struct hwcmap256	sc_cmap;
};
#define	WSDISPLAY_CMAP_DOLUT	0x20

int	pm_match(device_t, cfdata_t, void *);
void	pm_attach(device_t, device_t, void *);
int	pm_ioctl(void *, void *, u_long, void *, int, struct lwp *);
paddr_t	pm_mmap(void *, void *, off_t, int);
int	pm_alloc_screen(void *, const struct wsscreen_descr *,
				void **, int *, int *, long *);
void	pm_free_screen(void *, void *);
int	pm_show_screen(void *, void *, int,
			       void (*) (void *, int, int), void *);
void	pm_cursor_off(void);
void	pm_cursor_on(struct pm_softc *);
int	pm_cnattach(void);
void	pm_common_init(void);
int	pm_flush(struct pm_softc *);
int	pm_get_cmap(struct pm_softc *, struct wsdisplay_cmap *);
int	pm_set_cmap(struct pm_softc *, struct wsdisplay_cmap *);
int	pm_set_cursor(struct pm_softc *, struct wsdisplay_cursor *);
int	pm_get_cursor(struct pm_softc *, struct wsdisplay_cursor *);
void	pm_set_curpos(struct pm_softc *, struct wsdisplay_curpos *);
void	pm_init_cmap(struct pm_softc *);

CFATTACH_DECL_NEW(pm, sizeof(struct pm_softc),
   pm_match, pm_attach, NULL, NULL);

struct rasops_info pm_ri;

struct wsscreen_descr pm_stdscreen = {
	"std", 0, 0,
	0, /* textops */
	0, 0,
	WSSCREEN_REVERSE
};

const struct wsscreen_descr *_pm_scrlist[] = {
	&pm_stdscreen,
};

const struct wsscreen_list pm_screenlist = {
	sizeof(_pm_scrlist) / sizeof(struct wsscreen_descr *), _pm_scrlist
};

const struct wsdisplay_accessops pm_accessops = {
	pm_ioctl,
	pm_mmap,
	pm_alloc_screen,
	pm_free_screen,
	pm_show_screen,
	0 /* load_font */
};

u_int	pm_creg;

int
pm_match(device_t parent, cfdata_t cf, void *aux)
{
	struct ibus_attach_args *ia;
	void *pmaddr;

	ia = aux;
	pmaddr = (void *)ia->ia_addr;

	if (strcmp(ia->ia_name, "pm") != 0)
		return (0);

	if (badaddr(pmaddr, 4))
		return (0);

	return (1);
}

void
pm_attach(device_t parent, device_t self, void *aux)
{
	struct pm_softc *sc;
	struct rasops_info *ri;
	struct wsemuldisplaydev_attach_args waa;
	int console;

	sc = device_private(self);
	sc->sc_dev = self;
	ri = &pm_ri;
	console = (ri->ri_bits != NULL);

	if (console) {
		sc->sc_nscreens = 1;
		ri->ri_flg &= ~RI_NO_AUTO;
	} else
		pm_common_init();

	printf(": %dx%d, %dbpp\n", ri->ri_width, ri->ri_height, ri->ri_depth);

	pm_init_cmap(sc);

	sc->sc_blanked = 0;
	sc->sc_curenb = 0;

	waa.console = console;
	waa.scrdata = &pm_screenlist;
	waa.accessops = &pm_accessops;
	waa.accesscookie = sc;

	config_found(self, &waa, wsemuldisplaydevprint);
}

void
pm_init_cmap(struct pm_softc *sc)
{
	struct hwcmap256 *cm;
	struct rasops_info *ri;
	const uint8_t *p;
	int index;

	cm = &sc->sc_cmap;
	ri = &pm_ri;

	if (ri->ri_depth == 8) {
		p = rasops_cmap;
		for (index = 0; index < 256; index++, p += 3) {
			cm->r[index] = p[0];
			cm->g[index] = p[1];
			cm->b[index] = p[2];
		}

		sc->sc_type = WSDISPLAY_TYPE_PM_COLOR;
		sc->sc_cmap_size = 256;
		sc->sc_fb_size = 0x100000;
	} else {
		cm->r[0] = 0x00;
		cm->g[0] = 0x00;
		cm->b[0] = 0x00;

		cm->r[1] = 0x00;
		cm->g[1] = 0xff;
		cm->b[1] = 0x00;

		sc->sc_type = WSDISPLAY_TYPE_PM_MONO;
		sc->sc_cmap_size = 2;
		sc->sc_fb_size = 0x40000;
	}
}

void
pm_common_init(void)
{
	struct rasops_info *ri;
	int cookie, bior, i;
	PCCRegs *pcc;
	VDACRegs *vdac;
	uint16_t kn01csr;

	kn01csr = *(volatile uint16_t *)MIPS_PHYS_TO_KSEG1(KN01_SYS_CSR);
	pcc = (void *)MIPS_PHYS_TO_KSEG1(KN01_SYS_PCC);
	vdac = (void *)MIPS_PHYS_TO_KSEG1(KN01_SYS_VDAC);
	ri = &pm_ri;

	ri->ri_flg = RI_CENTER;
	if (ri->ri_bits == NULL)
		ri->ri_flg |= RI_NO_AUTO;
	ri->ri_depth = ((kn01csr & KN01_CSR_MONO) != 0 ? 1 : 8);
	ri->ri_width = 1024;
	ri->ri_height = 864;
	ri->ri_stride = (ri->ri_depth == 8 ? 1024 : 2048 / 8);
	ri->ri_bits = (void *)MIPS_PHYS_TO_KSEG1(KN01_PHYS_FBUF_START);

	/*
	 * Clear the screen.
	 */
	memset(ri->ri_bits, 0, ri->ri_stride * ri->ri_height);

	/*
	 * Get a font to use.
	 */
	bior = (ri->ri_depth == 8 ? WSDISPLAY_FONTORDER_L2R :
	    WSDISPLAY_FONTORDER_R2L);

	wsfont_init();
	if (ri->ri_depth == 8)
		cookie = wsfont_find(NULL, 12, 0, 0, bior,
		    WSDISPLAY_FONTORDER_L2R, WSFONT_FIND_BITMAP);
	else
		cookie = wsfont_find(NULL, 8, 0, 0, bior,
		    WSDISPLAY_FONTORDER_L2R, WSFONT_FIND_BITMAP);
	if (cookie <= 0)
		cookie = wsfont_find(NULL, 0, 0, 0, bior,
		    WSDISPLAY_FONTORDER_L2R, WSFONT_FIND_BITMAP);
	if (cookie <= 0) {
		printf("pm: font table is empty\n");
		return;
	}

	if (wsfont_lock(cookie, &ri->ri_font)) {
		printf("pm: couldn't lock font\n");
		return;
	}
	ri->ri_wsfcookie = cookie;

	/*
	 * Set up the raster operations set.
	 */
	rasops_init(ri, 1000, 1000);

	pm_stdscreen.nrows = ri->ri_rows;
	pm_stdscreen.ncols = ri->ri_cols;
	pm_stdscreen.textops = &ri->ri_ops;
	pm_stdscreen.capabilities = ri->ri_caps;

	/*
	 * Initalize the VDAC.
	 */
	*(uint8_t *)MIPS_PHYS_TO_KSEG1(KN01_PHYS_COLMASK_START) = 0xff;
	wbflush();

	vdac->overWA = 0x04; wbflush();
	vdac->over = 0x00; wbflush();
	vdac->over = 0x00; wbflush();
	vdac->over = 0x00; wbflush();
	vdac->overWA = 0x08; wbflush();
	vdac->over = 0x00; wbflush();
	vdac->over = 0x00; wbflush();
	vdac->over = 0x7f; wbflush();
	vdac->overWA = 0x0c; wbflush();
	vdac->over = 0xff; wbflush();
	vdac->over = 0xff; wbflush();
	vdac->over = 0xff; wbflush();

	/*
	 * Set in the initial colormap.
	 */
	if (ri->ri_depth == 8) {
		vdac->mapWA = 0;
		wbflush();

		for (i = 0; i < 256 * 3; i += 3) {
			vdac->map = rasops_cmap[i];
			wbflush();
			vdac->map = rasops_cmap[i + 1];
			wbflush();
			vdac->map = rasops_cmap[i + 2];
			wbflush();
		}
	} else {
		vdac->mapWA = 0;
		wbflush();

		for (i = 0; i < 256; i++) {
			vdac->map = 0x00;
			wbflush();
			vdac->map = (i < 128 ? 0x00 : 0xff);
			wbflush();
			vdac->map = 0x00;
			wbflush();
		}
	}

	/*
	 * Turn off the hardware cursor sprite for text mode.
	 */
	pcc->cmdr = PCC_FOPB | PCC_VBHI;
	wbflush();
	pm_creg = 0;
	pm_cursor_off();
}

void
pm_cursor_off(void)
{
	PCCRegs *pcc;

	pcc = (void *)MIPS_PHYS_TO_KSEG1(KN01_SYS_PCC);
	pcc->cmdr = (pm_creg &= ~(PCC_ENPA | PCC_ENPB));
	wbflush();
}

void
pm_cursor_on(struct pm_softc *sc)
{
	PCCRegs *pcc;

	if (sc->sc_curenb) {
		pcc = (void *)MIPS_PHYS_TO_KSEG1(KN01_SYS_PCC);
		pcc->cmdr = (pm_creg |= (PCC_ENPA | PCC_ENPB));
		wbflush();
	}
}

int
pm_ioctl(void *v, void *vs, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct pm_softc *sc;
	struct rasops_info *ri;
	int turnoff, rv, i;
	PCCRegs *pcc;
	VDACRegs *vdac;

	sc = v;
	ri = &pm_ri;
	rv = 0;
	pcc = (void *)MIPS_PHYS_TO_KSEG1(KN01_SYS_PCC);
	vdac = (void *)MIPS_PHYS_TO_KSEG1(KN01_SYS_VDAC);

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = sc->sc_type;
		break;

	case WSDISPLAYIO_SMODE:
		if (*(u_int *)data == WSDISPLAYIO_MODE_EMUL) {
			pm_cursor_off();
			pm_init_cmap(sc);
			memset(ri->ri_bits, 0, ri->ri_stride * ri->ri_height); 
			sc->sc_curenb = 0;
			sc->sc_changed |= WSDISPLAY_CMAP_DOLUT;
		}
		break;

	case WSDISPLAYIO_GINFO:
#define	wsd_fbip ((struct wsdisplay_fbinfo *)data)
		wsd_fbip->height = ri->ri_height;
		wsd_fbip->width = ri->ri_width;
		wsd_fbip->depth = ri->ri_depth;
		wsd_fbip->cmsize = sc->sc_cmap_size;
#undef fbt
		break;

	case WSDISPLAYIO_GETCMAP:
		rv = pm_get_cmap(sc, (struct wsdisplay_cmap *)data);
		break;

	case WSDISPLAYIO_PUTCMAP:
		rv = pm_set_cmap(sc, (struct wsdisplay_cmap *)data);
		break;

	case WSDISPLAYIO_SVIDEO:
		turnoff = (*(int *)data == WSDISPLAYIO_VIDEO_OFF);
		if ((sc->sc_blanked == 0) ^ turnoff) {
			sc->sc_blanked = turnoff;
			if (turnoff == 0) {
				pcc->cmdr =
				    (pm_creg &= ~(PCC_FOPA | PCC_FOPB));
				wbflush();
				pm_cursor_on(sc);
				sc->sc_changed |= WSDISPLAY_CURSOR_DOCMAP;
			} else {
				pm_cursor_off();
				pcc->cmdr =
				    (pm_creg |= (PCC_FOPA | PCC_FOPB));
				wbflush();
				vdac->overWA = 0x0c;
				wbflush();
				for (i = 0; i < 3; i++) {
					vdac->over = 0;
					wbflush();
				}
			}
		}
		break;

	case WSDISPLAYIO_GVIDEO:
		*(u_int *)data = (sc->sc_blanked ?
		    WSDISPLAYIO_VIDEO_OFF : WSDISPLAYIO_VIDEO_ON);
		break;

	case WSDISPLAYIO_GCURPOS:
		*(struct wsdisplay_curpos *)data = sc->sc_cursor.cc_pos;
		break;

	case WSDISPLAYIO_SCURPOS:
		pm_set_curpos(sc, (struct wsdisplay_curpos *)data);
		sc->sc_changed |= WSDISPLAY_CURSOR_DOPOS;
		break;

	case WSDISPLAYIO_GCURMAX:
		((struct wsdisplay_curpos *)data)->x =
		((struct wsdisplay_curpos *)data)->y = CURSOR_MAX_SIZE;
		break;

	case WSDISPLAYIO_GCURSOR:
		rv = pm_get_cursor(sc, (struct wsdisplay_cursor *)data);
		break;

	case WSDISPLAYIO_SCURSOR:
		rv = pm_set_cursor(sc, (struct wsdisplay_cursor *)data);
		break;

	default:
		rv = ENOTTY;
		break;
	}

	pm_flush(sc);
	return (rv);
}

paddr_t
pm_mmap(void *v, void *vs, off_t offset, int prot)
{
	struct pm_softc *sc;

	sc = v;

	if (offset >= sc->sc_fb_size || offset < 0)
		return (-1);

	return (mips_btop(KN01_PHYS_FBUF_START + offset));
}

int
pm_alloc_screen(void *v, const struct wsscreen_descr *type, void **cookiep,
		int *curxp, int *curyp, long *attrp)
{
	struct pm_softc *sc;
	struct rasops_info *ri;
	long defattr;

	sc = v;
	ri = &pm_ri;

	if (sc->sc_nscreens > 0)
		return (ENOMEM);

	*cookiep = ri;	 /* one and only for now */
	*curxp = 0;
	*curyp = 0;
	(*ri->ri_ops.allocattr)(ri, 0, 0, 0, &defattr);
	*attrp = defattr;
	sc->sc_nscreens++;
	return (0);
}

void
pm_free_screen(void *v, void *cookie)
{

	panic("pm_free_screen: console");
}

int
pm_show_screen(void *v, void *cookie, int waitok,
	       void (*cb)(void *, int, int), void *cbarg)
{

	return (0);
}

/* EXPORT */ int
pm_cnattach(void)
{
	struct rasops_info *ri;
	long defattr;

	ri = &pm_ri;

	pm_common_init();
	(*ri->ri_ops.allocattr)(ri, 0, 0, 0, &defattr);
	wsdisplay_cnattach(&pm_stdscreen, ri, 0, 0, defattr);
	return (1);
}

int
pm_flush(struct pm_softc *sc)
{
	VDACRegs *vdac;
	PCCRegs *pcc;
	uint8_t *cp;
	int v, i, x, y;
	u_short *p, *pe;
	struct hwcmap256 *cm;

	if (sc->sc_changed == 0)
		return (1);

	vdac = (void *)MIPS_PHYS_TO_KSEG1(KN01_SYS_VDAC);
	pcc = (void *)MIPS_PHYS_TO_KSEG1(KN01_SYS_PCC);
	v = sc->sc_changed;

	if ((v & WSDISPLAY_CURSOR_DOCUR) != 0) {
		if (sc->sc_curenb)
			pm_cursor_on(sc);
		else
			pm_cursor_off();
	}

	if ((v & (WSDISPLAY_CURSOR_DOPOS | WSDISPLAY_CURSOR_DOHOT)) != 0) {
		x = sc->sc_cursor.cc_pos.x - sc->sc_cursor.cc_hot.x;
		y = sc->sc_cursor.cc_pos.y - sc->sc_cursor.cc_hot.y;
		pcc->xpos = x + PCC_X_OFFSET;
		pcc->ypos = y + PCC_Y_OFFSET;
		wbflush();
	}
	if ((v & WSDISPLAY_CURSOR_DOCMAP) != 0) {
		cp = sc->sc_cursor.cc_color;

		vdac->overWA = 0x04;
		wbflush();
		for (i = 1; i < 6; i += 2) {
			vdac->over = cp[i];
			wbflush();
		}

		vdac->overWA = 0x08;
		wbflush();
		vdac->over = 0x00;
		wbflush();
		vdac->over = 0x00;
		wbflush();
		vdac->over = 0x7f;
		wbflush();

		vdac->overWA = 0x0c;
		wbflush();
		for (i = 0; i < 6; i += 2) {
			vdac->over = cp[i];
			wbflush();
		}
	}
	if ((v & WSDISPLAY_CURSOR_DOSHAPE) != 0) {
		pcc->cmdr = (pm_creg | PCC_LODSA);
		wbflush();

		p = sc->sc_cursor.cc_image;
		x = 0xffff >> (16 - sc->sc_cursor.cc_size.x);
		for (pe = p + 64; p < pe; p += 2) {
			pcc->memory = *p & x;
			wbflush();
		}

		pcc->cmdr = (pm_creg &= ~PCC_LODSA);
		wbflush();
	}

	if ((v & WSDISPLAY_CMAP_DOLUT) != 0) {
		cm = &sc->sc_cmap;

		vdac->mapWA = 0;
		wbflush();

		if (sc->sc_cmap_size == 2) {
			for (i = 0; i < 128; i++) {
				vdac->map = 0;
				wbflush();
				vdac->map = cm->g[0];
				wbflush();
				vdac->map = 0;
				wbflush();
			}
			for (; i < 256; i++) {
				vdac->map = 0;
				wbflush();
				vdac->map = cm->g[1];
				wbflush();
				vdac->map = 0;
				wbflush();
			}
		} else {
			for (i = 0; i < sc->sc_cmap_size; i++) {
				vdac->map = cm->r[i];
				wbflush();
				vdac->map = cm->g[i];
				wbflush();
				vdac->map = cm->b[i];
				wbflush();
			}
		}
	}

	sc->sc_changed = 0;
	return (1);
}

int
pm_get_cmap(struct pm_softc *sc, struct wsdisplay_cmap *p)
{
	u_int index, count;
	int rv;

	index = p->index;
	count = p->count;

	if (index >= sc->sc_cmap_size || count > sc->sc_cmap_size - index)
		return (EINVAL);

	if ((rv = copyout(&sc->sc_cmap.r[index], p->red, count)) != 0)
		return (rv);
	if ((rv = copyout(&sc->sc_cmap.g[index], p->green, count)) != 0)
		return (rv);
	return (copyout(&sc->sc_cmap.b[index], p->blue, count));
}

int
pm_set_cmap(struct pm_softc *sc, struct wsdisplay_cmap *p)
{
	u_int index, count;
	int rv;

	index = p->index;
	count = p->count;

	if (index >= sc->sc_cmap_size || count > sc->sc_cmap_size - index)
		return (EINVAL);

	if ((rv = copyin(p->red, &sc->sc_cmap.r[index], count)) != 0)
		return (rv);
	if ((rv = copyin(p->green, &sc->sc_cmap.g[index], count)) != 0)
		return (rv);
	if ((rv = copyin(p->blue, &sc->sc_cmap.b[index], count)) != 0)
		return (rv);
	sc->sc_changed |= WSDISPLAY_CMAP_DOLUT;
	return (0);
}

int
pm_set_cursor(struct pm_softc *sc, struct wsdisplay_cursor *p)
{
	u_int v, index, count;
	struct hwcursor64 *cc;
	int rv;

	v = p->which;
	cc = &sc->sc_cursor;

	if ((v & WSDISPLAY_CURSOR_DOCUR) != 0)
		sc->sc_curenb = p->enable;
	if ((v & WSDISPLAY_CURSOR_DOPOS) != 0)
		pm_set_curpos(sc, &p->pos);
	if ((v & WSDISPLAY_CURSOR_DOHOT) != 0)
		cc->cc_hot = p->hot;
	if ((v & WSDISPLAY_CURSOR_DOCMAP) != 0) {
		index = p->cmap.index;
		count = p->cmap.count;
		if (index >= 2 || (index + count) > 2)
			return (EINVAL);

		rv = copyin(p->cmap.red, &cc->cc_color[index], count);
		if (rv != 0)
			return (rv);
		rv = copyin(p->cmap.green, &cc->cc_color[index + 2], count);
		if (rv != 0)
			return (rv);
		rv = copyin(p->cmap.blue, &cc->cc_color[index + 4], count);
		if (rv != 0)
			return (rv);
	}
	if ((v & WSDISPLAY_CURSOR_DOSHAPE) != 0) {
		if (p->size.x > CURSOR_MAX_SIZE ||
		    p->size.y > CURSOR_MAX_SIZE)
			return (EINVAL);

		cc->cc_size = p->size;
		memset(cc->cc_image, 0, sizeof(cc->cc_image));
		rv = copyin(p->image, cc->cc_image, p->size.y * 4);
		if (rv != 0)
			return (rv);
		rv = copyin(p->mask, cc->cc_image+32, p->size.y * 4);
		if (rv != 0)
			return (rv);
	}

	sc->sc_changed |= v;
	return (0);
}

int
pm_get_cursor(struct pm_softc *sc, struct wsdisplay_cursor *p)
{

	return (ENOTTY); /* XXX */
}

void
pm_set_curpos(struct pm_softc *sc, struct wsdisplay_curpos *curpos)
{
	struct rasops_info *ri;
	int x, y;

	ri = &pm_ri;
	x = curpos->x;
	y = curpos->y; 

	if (y < 0)
		y = 0;
	else if (y > ri->ri_height)
		y = ri->ri_height;
	if (x < 0)
		x = 0;
	else if (x > ri->ri_width)
		x = ri->ri_width;
	sc->sc_cursor.cc_pos.x = x;
	sc->sc_cursor.cc_pos.y = y;
}
