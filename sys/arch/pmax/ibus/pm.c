/*	$NetBSD: pm.c,v 1.1.16.1 2002/03/15 14:22:42 ad Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pm.c,v 1.1.16.1 2002/03/15 14:22:42 ad Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/buf.h>
#include <sys/ioctl.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/rasops/rasops.h>
#include <dev/wsfont/wsfont.h>

#include <pmax/pmax/kn01.h>

#include <pmax/ibus/ibusvar.h>
#include <pmax/ibus/pmreg.h>

#include <uvm/uvm_extern.h>

struct hwcmap256 {
	u_int8_t r[256];
	u_int8_t g[256];
	u_int8_t b[256];
};

struct hwcursor64 {
	struct wsdisplay_curpos cc_pos;
	struct wsdisplay_curpos cc_hot;
	struct wsdisplay_curpos cc_size;
#define	CURSOR_MAX_SIZE	16
	u_int8_t cc_color[6];
	u_short cc_image[16 + 16];
};

struct pm_softc {
	struct device		sc_dev;
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

int	pmmatch(struct device *, struct cfdata *, void *);
void	pmattach(struct device *, struct device *, void *);
int	pm_ioctl(void *, u_long, caddr_t, int, struct proc *);
paddr_t	pm_mmap(void *, off_t, int);
int	pm_alloc_screen(void *, const struct wsscreen_descr *,
				void **, int *, int *, long *);
void	pm_free_screen(void *, void *);
int	pm_show_screen(void *, void *, int,
			       void (*) (void *, int, int), void *);
void	pm_cursor_off(struct rasops_info *);
void	pm_cursor_on(struct pm_softc *);
int		pm_cnattach(void);
void	pm_common_init(struct rasops_info *);
int	pm_flush(struct pm_softc *);
int	pm_get_cmap(struct pm_softc *, struct wsdisplay_cmap *);
int	pm_set_cmap(struct pm_softc *, struct wsdisplay_cmap *);
int	pm_set_cursor(struct pm_softc *, struct wsdisplay_cursor *);
int	pm_get_cursor(struct pm_softc *, struct wsdisplay_cursor *);
void	pm_set_curpos(struct pm_softc *, struct wsdisplay_curpos *);
void	pm_init_cmap(struct pm_softc *);

const struct cfattach pm_ca = {
	sizeof(struct pm_softc), pmmatch, pmattach,
};

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
pmmatch(struct device *parent, struct cfdata *match, void *aux)
{
	struct ibus_attach_args *ia;
	caddr_t pmaddr;

	ia = aux;
	pmaddr = (caddr_t)ia->ia_addr;

	if (strcmp(ia->ia_name, "pm") != 0)
		return (0);

	if (badaddr(pmaddr, 4))
		return (0);

	return (1);
}

void
pmattach(struct device *parent, struct device *self, void *aux)
{
	struct pm_softc *sc;
	struct rasops_info *ri;
	struct wsemuldisplaydev_attach_args waa;
	int console;

	sc = (struct pm_softc *)self;
	ri = &pm_ri;
	console = (ri->ri_bits != NULL);

	if (console)
		sc->sc_nscreens = 1;
	else
		pm_common_init(ri);

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
	const u_int8_t *p;
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
		cm->r[0] = 0;
		cm->g[0] = 0;
		cm->b[0] = 0;
		for (index = 1; index < 256; index++) {
			cm->r[0] = 0xff;
			cm->g[0] = 0xff;
			cm->b[0] = 0xff;
		}

		sc->sc_type = WSDISPLAY_TYPE_PM_MONO;
		sc->sc_cmap_size = 2;
		sc->sc_fb_size = 0x20000;
	}
}

void
pm_common_init(struct rasops_info *ri)
{
	int cookie, bior;
	PCCRegs *pcc;
#if 0
	VDACRegs *vdac;
	int i;
#endif
	u_int16_t kn01csr;

	printf("pm_common_init: 0\n");

	kn01csr = *(volatile u_int16_t *)MIPS_PHYS_TO_KSEG1(KN01_SYS_CSR);

	ri->ri_flg = RI_CENTER;
	ri->ri_depth = ((kn01csr & KN01_CSR_MONO) != 0 ? 1 : 8);
	ri->ri_width = 1024;
	ri->ri_height = 864;
	ri->ri_stride = (ri->ri_depth == 8 ? 1024 : 2048 / 8);
	ri->ri_bits = (void *)MIPS_PHYS_TO_KSEG1(KN01_PHYS_FBUF_START);

	printf("pm_common_init: 1\n");

	/*
	 * Clear the screen.
	 */
#if 0
	memset(ri->ri_bits, 0, ri->ri_stride * ri->ri_height);
#endif

	/*
	 * Get a font to use.
	 */
	bior = (ri->ri_depth == 8 ? WSDISPLAY_FONTORDER_L2R :
	    WSDISPLAY_FONTORDER_R2L);

	printf("pm_common_init: 2\n");

	wsfont_init();
	cookie = wsfont_find(NULL, 8, 0, 0, bior, WSDISPLAY_FONTORDER_L2R);
	if (cookie <= 0)
		cookie = wsfont_find(NULL, 0, 0, 0, bior,
		    WSDISPLAY_FONTORDER_L2R);
	if (cookie <= 0) {
		printf("pm: font table is empty\n");
		return;
	}

	printf("pm_common_init: 3\n");

	if (wsfont_lock(cookie, &ri->ri_font)) {
		printf("pm: couldn't lock font\n");
		return;
	}
	ri->ri_wsfcookie = cookie;

	printf("pm_common_init: 4\n");

	/*
	 * Set up the raster operations set.
	 */
	rasops_init(ri, 34, 80);

	printf("pm_common_init: 5\n");

	pm_stdscreen.nrows = ri->ri_rows;
	pm_stdscreen.ncols = ri->ri_cols;
	pm_stdscreen.textops = &ri->ri_ops;
	pm_stdscreen.capabilities = ri->ri_caps;

	printf("pm_common_init: 6\n");

#if 0
	/*
	 * Initalize the VDAC.
	 */
	vdac = (void *)MIPS_PHYS_TO_KSEG1(KN01_SYS_VDAC);
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

		vdac->map = 0x00;
		wbflush();
		vdac->map = 0x00;
		wbflush();
		vdac->map = 0x00;
		wbflush();

		for (i = 1; i < 256; i++) {
			vdac->map = 0xff;
			wbflush();
			vdac->map = 0xff;
			wbflush();
			vdac->map = 0xff;
			wbflush();
		}
	}
#endif
	printf("pm_common_init: 7 - sleeping for 3 seconds\n");
	DELAY(1000*1000*3)
	
	/*
	 * Turn off the hardware cursor sprite for rcons text mode.
	 */
	pcc->cmdr = (pm_creg = PCC_FOPB | PCC_VBHI);
	wbflush();

	printf("pm_common_init: 8 - sleeping for 3 seconds\n");
	DELAY(1000*1000*3)

	pm_cursor_off(ri);

	printf("pm_common_init: 9 - sleeping for 3 seconds\n");
	DELAY(1000*1000*3)
}

void
pm_cursor_off(struct rasops_info *ri)
{
	PCCRegs *pcc;
	VDACRegs *vdac;
	int i;

	pcc = (void *)MIPS_PHYS_TO_KSEG1(KN01_SYS_PCC);
	vdac = (void *)MIPS_PHYS_TO_KSEG1(KN01_SYS_VDAC);
	pcc->cmdr = (pm_creg &= ~(PCC_ENPA | PCC_ENPB));
	wbflush();

	vdac->overWA = 0x0c;
	wbflush();

	for (i = 0; i < 3; i++) {
		vdac->over = 0;
		wbflush();
	}
}

void
pm_cursor_on(struct pm_softc *sc)
{
	PCCRegs *pcc;

	if (sc->sc_curenb) {
		pcc = (void *)MIPS_PHYS_TO_KSEG1(KN01_SYS_PCC);
		pcc->cmdr = (pm_creg |= (PCC_ENPA | PCC_ENPB));
		wbflush();
		sc->sc_changed |= WSDISPLAY_CURSOR_DOCMAP;
	}
}

int
pm_ioctl(void *v, u_long cmd, caddr_t data, int flag, struct proc *p)
{
	struct pm_softc *sc;
	struct rasops_info *ri;
	int turnoff, rv;
	PCCRegs *pcc;

	sc = v;
	ri = &pm_ri;
	pcc = (void *)MIPS_PHYS_TO_KSEG1(KN01_SYS_PCC);

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = sc->sc_type;
		rv = 0;
		break;

	case WSDISPLAYIO_SMODE:
		if (*(u_int *)data == WSDISPLAYIO_MODE_EMUL) {
			pm_cursor_off(ri);
			pm_init_cmap(sc);
			memset(ri->ri_bits, 0, ri->ri_stride * ri->ri_height); 
			sc->sc_changed |= WSDISPLAY_CMAP_DOLUT;
		}
		rv = 0;
		break;

	case WSDISPLAYIO_GINFO:
#define	wsd_fbip ((struct wsdisplay_fbinfo *)data)
		wsd_fbip->height = ri->ri_height;
		wsd_fbip->width = ri->ri_width;
		wsd_fbip->depth = ri->ri_depth;
		wsd_fbip->cmsize = sc->sc_cmap_size;
#undef fbt
		rv = 0;
		break;

	case WSDISPLAYIO_GETCMAP:
		rv = pm_get_cmap(sc, (struct wsdisplay_cmap *)data);
		break;

	case WSDISPLAYIO_PUTCMAP:
		rv = pm_set_cmap(sc, (struct wsdisplay_cmap *)data);
		break;

	case WSDISPLAYIO_SVIDEO:
		turnoff = *(int *)data == WSDISPLAYIO_VIDEO_OFF;
		if ((sc->sc_blanked == 0) ^ turnoff) {
			sc->sc_blanked = turnoff;
			if (turnoff == 1) {
				pcc->cmdr =
				    (pm_creg &= ~(PCC_FOPA | PCC_FOPB));
				pm_cursor_on(sc);
			} else {
				pcc->cmdr =
				    (pm_creg |= (PCC_FOPA | PCC_FOPB));
				pm_cursor_off(ri);
			}
		}
		rv = 0;
		break;

	case WSDISPLAYIO_GVIDEO:
		*(u_int *)data = sc->sc_blanked ?
		    WSDISPLAYIO_VIDEO_OFF : WSDISPLAYIO_VIDEO_ON;
		rv = 0;
		break;

	case WSDISPLAYIO_GCURPOS:
		*(struct wsdisplay_curpos *)data = sc->sc_cursor.cc_pos;
		rv = 0;
		break;

	case WSDISPLAYIO_SCURPOS:
		pm_set_curpos(sc, (struct wsdisplay_curpos *)data);
		rv = 0;
		break;

	case WSDISPLAYIO_GCURMAX:
		((struct wsdisplay_curpos *)data)->x =
		((struct wsdisplay_curpos *)data)->y = CURSOR_MAX_SIZE;
		rv = 0;
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
pm_mmap(void *v, off_t offset, int prot)
{
	struct pm_softc *sc;

	sc = v;

	if (offset >= sc->sc_fb_size || offset < 0)
		return (-1);

	return (mips_btop((caddr_t)pm_ri.ri_bits + offset));
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
	(*ri->ri_ops.alloc_attr)(ri, 0, 0, 0, &defattr);
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

	printf("pm_cnattach: 0\n");

	ri = &pm_ri;
	pm_common_init(ri);

	printf("pm_cnattach: 1\n");

	(*ri->ri_ops.alloc_attr)(ri, 0, 0, 0, &defattr);

	printf("pm_cnattach: 2\n");

	wsdisplay_cnattach(&pm_stdscreen, ri, 0, 0, defattr);

	printf("pm_cnattach: 3\n");

	return(0);
}

int
pm_flush(struct pm_softc *sc)
{
	VDACRegs *vdac;
	PCCRegs *pcc;
	u_int8_t *cp;
	int v, i, x, y;
	u_short *p, *pe;
	struct hwcmap256 *cm;
	struct rasops_info *ri;

	if (sc->sc_changed == 0)
		return (1);

	vdac = (void *)MIPS_PHYS_TO_KSEG1(KN01_SYS_VDAC);
	pcc = (void *)MIPS_PHYS_TO_KSEG1(KN01_SYS_PCC);
	ri = &pm_ri;

	if ((sc->sc_changed & WSDISPLAY_CURSOR_DOCUR) != 0) {
		if (sc->sc_curenb)
			pm_cursor_on(sc);
		else
			pm_cursor_off(ri);
	}

	/*
	 * sc_changed may be modified by pm_cursor_on().
	 */
	v = sc->sc_changed;

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
		for (i = 0; i < 6; i += 2) {
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
		for (i = 1; i < 6; i += 2) {
			vdac->over = cp[i];
			wbflush();
		}
	}
	if ((v & WSDISPLAY_CURSOR_DOSHAPE) != 0) {
		pcc->cmdr = (pm_creg | PCC_LODSA);
		wbflush();

		p = sc->sc_cursor.cc_image;
		pe = p + 32;
		while (p < pe) {
			pcc->memory = *p++;
			wbflush();
		}

		pcc->cmdr = (pm_creg &= ~PCC_LODSA);
		wbflush();
	}

	if ((v & WSDISPLAY_CMAP_DOLUT) != 0) {
		cm = &sc->sc_cmap;

		vdac->mapWA = 0;
		wbflush();

		for (i = 0; i < sc->sc_cmap_size; i++) {
			vdac->map = cm->r[i];
			wbflush();
			vdac->map = cm->g[i];
			wbflush();
			vdac->map = cm->b[i];
			wbflush();
		}
	}

	sc->sc_changed = 0;
	return (1);
}

int
pm_get_cmap(struct pm_softc *sc, struct wsdisplay_cmap *p)
{
	u_int index, count;

	index = p->index;
	count = p->count;

	if (index >= sc->sc_cmap_size || (index + count) > sc->sc_cmap_size)
		return (EINVAL);

	if (!uvm_useracc(p->red, count, B_WRITE) ||
	    !uvm_useracc(p->green, count, B_WRITE) ||
	    !uvm_useracc(p->blue, count, B_WRITE))
		return (EFAULT);

	copyout(&sc->sc_cmap.r[index], p->red, count);
	copyout(&sc->sc_cmap.g[index], p->green, count);
	copyout(&sc->sc_cmap.b[index], p->blue, count);

	return (0);
}

int
pm_set_cmap(struct pm_softc *sc, struct wsdisplay_cmap *p)
{
	u_int index, count;

	index = p->index;
	count = p->count;

	if (index >= sc->sc_cmap_size || (index + count) > sc->sc_cmap_size)
		return (EINVAL);

	if (!uvm_useracc(p->red, count, B_READ) ||
	    !uvm_useracc(p->green, count, B_READ) ||
	    !uvm_useracc(p->blue, count, B_READ))
		return (EFAULT);

	copyin(p->red, &sc->sc_cmap.r[index], count);
	copyin(p->green, &sc->sc_cmap.g[index], count);
	copyin(p->blue, &sc->sc_cmap.b[index], count);
	sc->sc_changed |= WSDISPLAY_CMAP_DOLUT;
	return (0);
}

int
pm_set_cursor(struct pm_softc *sc, struct wsdisplay_cursor *p)
{
#define	cc (&sc->sc_cursor)
	u_int v, index, count;

	v = p->which;

	if ((v & WSDISPLAY_CURSOR_DOCMAP) != 0) {
		index = p->cmap.index;
		count = p->cmap.count;
		if (index >= 2 || (index + count) > 2)
			return (EINVAL);
		if (!uvm_useracc(p->cmap.red, count, B_READ) ||
		    !uvm_useracc(p->cmap.green, count, B_READ) ||
		    !uvm_useracc(p->cmap.blue, count, B_READ))
			return (EFAULT);
	}
	if ((v & WSDISPLAY_CURSOR_DOSHAPE) != 0) {
		if (p->size.x > CURSOR_MAX_SIZE ||
		    p->size.y > CURSOR_MAX_SIZE ||
		    p->size.x < 9)
			return (EINVAL);
		if (!uvm_useracc(p->image, p->size.y * 2, B_READ) ||
		    !uvm_useracc(p->mask, p->size.y * 2, B_READ))
			return (EFAULT);
	}

	if ((v & WSDISPLAY_CURSOR_DOCUR) != 0)
		sc->sc_curenb = p->enable;
	if ((v & WSDISPLAY_CURSOR_DOPOS) != 0)
		pm_set_curpos(sc, &p->pos);
	if ((v & WSDISPLAY_CURSOR_DOHOT) != 0)
		cc->cc_hot = p->hot;
	if ((v & WSDISPLAY_CURSOR_DOCMAP) != 0) {
		copyin(p->cmap.red, &cc->cc_color[index], count);
		copyin(p->cmap.green, &cc->cc_color[index + 2], count);
		copyin(p->cmap.blue, &cc->cc_color[index + 4], count);
	}
	if ((v & WSDISPLAY_CURSOR_DOSHAPE) != 0) {
		cc->cc_size = p->size;
		memset(cc->cc_image, 0, sizeof(cc->cc_image));
		copyin(p->image, cc->cc_image, p->size.y * 2);
		copyin(p->mask, cc->cc_image+CURSOR_MAX_SIZE, p->size.y * 2);
	}
	sc->sc_changed |= v;

	return (0);
#undef cc
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
