/* $NetBSD: pm.c,v 1.1.2.5 1999/01/07 06:44:22 nisimura Exp $ */

/*
 * Copyright (c) 1998 Tohru Nishimura.  All rights reserved.
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

__KERNEL_RCSID(0, "$Id: pm.c,v 1.1.2.5 1999/01/07 06:44:22 nisimura Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/buf.h>
#include <sys/ioctl.h>
#include <vm/vm.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/rcons/raster.h>
#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wscons_raster.h>
#include <dev/wscons/wsdisplayvar.h>
#include <machine/autoconf.h>

#include <pmax/ibus/ibusvar.h>

#include "opt_uvm.h"
#if defined(UVM)
#include <uvm/uvm_extern.h>
#define useracc uvm_useracc
#endif

extern void kn01_wbflush __P((void));

struct pccreg {
#define	_WORD_(_x_)	u_int16_t _x_; unsigned : 16
	_WORD_(pcc_cmdr);	/* cursor command register */
	_WORD_(pcc_xpos);	/* cursor X position */
	_WORD_(pcc_ypos);	/* cursor Y position */
	_WORD_(pcc_xmin1);	/* region 1 top edge */
	_WORD_(pcc_xmax1);	/* region 1 bottom edge */
	_WORD_(pcc_ymin1);	/* region 1 top edge */
	_WORD_(pcc_ymax1);	/* region 1 bottom edge */
	unsigned : 32;		/* unused */
	unsigned : 32;		/* unused */
	unsigned : 32;		/* unused */
	unsigned : 32;		/* unused */
	_WORD_(pcc_xmin2);	/* region 2 top edge */
	_WORD_(pcc_xmax2);	/* region 2 bottom edge */
	_WORD_(pcc_ymin2);	/* region 2 top edge */
	_WORD_(pcc_ymax2);	/* region 2 bottom edge */
	_WORD_(pcc_memory);	/* cursor sprite pattern load */
#undef	_WORD_
};

#define	PCC_ENPA	0000001
#define	PCC_FOPA	0000002
#define	PCC_ENPB	0000004
#define	PCC_FOPB	0000010
#define	PCC_XHAIR	0000020
#define	PCC_XHCLP	0000040
#define	PCC_XHCL1	0000100
#define	PCC_XHWID	0000200
#define	PCC_ENRG1	0000400
#define	PCC_FORG1	0001000
#define	PCC_ENRG2	0002000
#define	PCC_FORG2	0004000
#define	PCC_LODSA	0010000
#define	PCC_VBHI	0020000
#define	PCC_HSHI	0040000
#define	PCC_TEST	0100000

struct bt478reg {
#define	_BYTE_(_y_)	u_int8_t _y_; unsigned : 24
	_BYTE_(bt_mapWA);	/* address register (color map write) */
	_BYTE_(bt_map);		/* color map */
	_BYTE_(bt_mask);	/* pixel read mask */
	_BYTE_(bt_mapRA);	/* address register (color map read) */
	_BYTE_(bt_overWA);	/* address register (overlay map write) */
	_BYTE_(bt_over);	/* overlay map */
	unsigned : 32;		/* unused */
	_BYTE_(bt_overRA);	/* address register (overlay map read) */
#undef	_BYTE_
};

struct fb_devconfig {
	vaddr_t dc_vaddr;		/* memory space virtual base address */
	paddr_t dc_paddr;		/* memory space physical base address */
	vsize_t dc_size;		/* size of slot memory */
	int     dc_wid;			/* width of frame buffer */
	int     dc_ht;			/* height of frame buffer */
	int     dc_depth;		/* depth, bits per pixel */
	int     dc_rowbytes;		/* bytes in a FB scan line */
	vaddr_t	dc_videobase;		/* base of flat frame buffer */
	struct raster	dc_raster;	/* raster description */
	struct rcons	dc_rcons;	/* raster blitter control info */
	int	    dc_blanked;		/* currently has video disabled */
};

struct hwcmap {
#define	CMAP_SIZE	256	/* 256 R/G/B entries */
	u_int8_t r[CMAP_SIZE];
	u_int8_t g[CMAP_SIZE];
	u_int8_t b[CMAP_SIZE];
};

struct hwcursor {
	struct wsdisplay_curpos cc_pos;
	struct wsdisplay_curpos cc_hot;
	struct wsdisplay_curpos cc_size;
#define	CURSOR_MAX_SIZE	16
	u_int8_t cc_color[6];
	u_int16_t cc_image[16 + 16];
};

struct pm_softc {
	struct device sc_dev;
	struct fb_devconfig *sc_dc;	/* device configuration */
	struct hwcmap sc_cmap;		/* software copy of colormap */
	struct hwcursor sc_cursor;	/* software copy of cursor */
	/* no sc_change field because pm does not emit interrupt */
	int nscreens;
	short magic_x, magic_y;		/* cursor location offset */
#define	PCC_MAGIC_X 212
#define	PCC_MAGIC_Y 34

	struct pccreg *sc_pcc;
	struct bt478reg *sc_vdac;
	u_int16_t sc_pcccmdr;		/* software copy of PCC cmdr */
};

int  pmmatch __P((struct device *, struct cfdata *, void *));
void pmattach __P((struct device *, struct device *, void *));

struct cfattach pm_ca = {
	sizeof(struct pm_softc), pmmatch, pmattach,
};

void pm_getdevconfig __P((tc_addr_t, struct fb_devconfig *));
struct fb_devconfig pm_console_dc;
tc_addr_t pm_consaddr;

struct wsdisplay_emulops pm_emulops = {
	rcons_cursor,
	rcons_mapchar,
	rcons_putchar,
	rcons_copycols,
	rcons_erasecols,
	rcons_copyrows,
	rcons_eraserows,
	rcons_alloc_attr
};

struct wsscreen_descr pm_stdscreen = {
	"std",
	0, 0,	/* will be filled in -- XXX shouldn't, it's global */
	&pm_emulops,
	0, 0,
	0
};

const struct wsscreen_descr *_pm_scrlist[] = {
	&pm_stdscreen,
};

struct wsscreen_list pm_screenlist = {
	sizeof(_pm_scrlist) / sizeof(struct wsscreen_descr *), _pm_scrlist
};

int	pmioctl __P((void *, u_long, caddr_t, int, struct proc *));
int	pmmmap __P((void *, off_t, int));

int	pm_alloc_screen __P((void *, const struct wsscreen_descr *,
				      void **, int *, int *, long *));
void	pm_free_screen __P((void *, void *));
void	pm_show_screen __P((void *, void *));
int	pm_load_font __P((void *, void *, int, int, int, void *));

struct wsdisplay_accessops pm_accessops = {
	pmioctl,
	pmmmap,
	pm_alloc_screen,
	pm_free_screen,
	pm_show_screen,
	pm_load_font
};

int  pm_cnattach __P((tc_addr_t));
void pminit __P((struct fb_devconfig *));
void pm_screenblank __P((struct pm_softc *));

static int  set_cmap __P((struct pm_softc *, struct wsdisplay_cmap *));
static int  get_cmap __P((struct pm_softc *, struct wsdisplay_cmap *));
static int  set_cursor __P((struct pm_softc *, struct wsdisplay_cursor *));
static int  get_cursor __P((struct pm_softc *, struct wsdisplay_cursor *));
static void set_curpos __P((struct pm_softc *, struct wsdisplay_curpos *));
void bt478_loadcmap __P((struct pm_softc *));
void bt478_load_curcmap __P((struct pm_softc *));
void pcc_load_curshape __P((struct pm_softc *));
void pcc_set_curpos __P((struct pm_softc *));
void pcc_show_cursor __P((struct pm_softc *, int));

#define	KN01_SYS_PCC	0x11100000
#define	KN01_SYS_BT478	0x11200000
#define	KN01_SYS_PMASK	0x10000000
#define	KN01_SYS_CSR	0x1e000000
#define	KN01_CSR_MONO	    0x0800

int
pmmatch(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct ibus_attach_args *ia = aux;

	if (strcmp("pm", ia->ia_name))
		return (0);
	if (badaddr((char *)ia->ia_addr, 4))
		return (0);
	return (1);
}

void
pm_getdevconfig(dense_addr, dc)
	tc_addr_t dense_addr;
	struct fb_devconfig *dc;
{
	struct raster *rap;
	struct rcons *rcp;
	u_int16_t *p = (void *)MIPS_PHYS_TO_KSEG1(KN01_SYS_CSR);
	int i;

	dc->dc_wid = 1024;
	dc->dc_ht = 864;
	dc->dc_depth = (*p & KN01_CSR_MONO) ? 1 : 8;
	dc->dc_rowbytes = (dc->dc_depth == 8) ? 1024 : 1024 / 8;
	dc->dc_videobase = dense_addr;
	dc->dc_blanked = 0;

	dc->dc_vaddr = dense_addr;
	dc->dc_paddr = MIPS_KSEG1_TO_PHYS(dc->dc_vaddr);
	dc->dc_size = (dc->dc_depth == 8) ? 0x100000 : 0x40000;

	/* initialize colormap and cursor resource */
	pminit(dc);

	/* clear the screen */
	for (i = 0; i < dc->dc_ht * dc->dc_rowbytes; i += sizeof(u_int32_t))
		*(u_int32_t *)(dc->dc_videobase + i) = 0;

	/* initialize the raster */
	rap = &dc->dc_raster;
	rap->width = dc->dc_wid;
	rap->height = dc->dc_ht;
	rap->depth = dc->dc_depth;
	rap->linelongs = dc->dc_rowbytes / sizeof(u_int32_t);
	rap->pixels = (u_int32_t *)dc->dc_videobase;

	/* initialize the raster console blitter */
	rcp = &dc->dc_rcons;
	rcp->rc_sp = rap;
	rcp->rc_crow = rcp->rc_ccol = -1;
	rcp->rc_crowp = &rcp->rc_crow;
	rcp->rc_ccolp = &rcp->rc_ccol;
	rcons_init(rcp, 34, 80);

	pm_stdscreen.nrows = dc->dc_rcons.rc_maxrow;
	pm_stdscreen.ncols = dc->dc_rcons.rc_maxcol;
}

void
pmattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct pm_softc *sc = (struct pm_softc *)self;
	struct ibus_attach_args *ia = aux;
	struct wsemuldisplaydev_attach_args waa;
	struct hwcmap *cm;
	int console, i;

	console = (ia->ia_addr == pm_consaddr);
	if (console) {
		sc->sc_dc = &pm_console_dc;
		sc->nscreens = 1;
	}
	else {
		sc->sc_dc = (struct fb_devconfig *)
		    malloc(sizeof(struct fb_devconfig), M_DEVBUF, M_WAITOK);
		pm_getdevconfig(ia->ia_addr, sc->sc_dc);
	}
	printf(": %d x %d, %dbpp\n", sc->sc_dc->dc_wid, sc->sc_dc->dc_ht,
	    sc->sc_dc->dc_depth);

	cm = &sc->sc_cmap;
	cm->r[0] = cm->g[0] = cm->b[0] = 0;
	for (i = 1; i < CMAP_SIZE; i++) {
		cm->r[i] = cm->g[i] = cm->b[i] = 0xff;
	}
	sc->magic_x = PCC_MAGIC_X;
	sc->magic_y = PCC_MAGIC_Y;
	sc->sc_pcc = (void *)MIPS_PHYS_TO_KSEG1(KN01_SYS_PCC);
	sc->sc_vdac = (void *)MIPS_PHYS_TO_KSEG1(KN01_SYS_BT478);
	sc->sc_pcccmdr = 0;

	/* pm emits no interrupt */

	waa.console = console;
	waa.scrdata = &pm_screenlist;
	waa.accessops = &pm_accessops;
	waa.accesscookie = sc;

	config_found(self, &waa, wsemuldisplaydevprint);
}

int
pmioctl(v, cmd, data, flag, p)
	void *v;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	struct pm_softc *sc = v;
	struct fb_devconfig *dc = sc->sc_dc;
	int turnoff, error;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = (dc->dc_depth == 8)
			? WSDISPLAY_TYPE_PM_COLOR : WSDISPLAY_TYPE_PM_MONO;
		return (0);

	case WSDISPLAYIO_GINFO:
#define	wsd_fbip ((struct wsdisplay_fbinfo *)data)
		wsd_fbip->height = sc->sc_dc->dc_ht;
		wsd_fbip->width = sc->sc_dc->dc_wid;
		wsd_fbip->depth = sc->sc_dc->dc_depth;
		wsd_fbip->cmsize = CMAP_SIZE;
#undef fbt
		return (0);

	case WSDISPLAYIO_GETCMAP:
		return get_cmap(sc, (struct wsdisplay_cmap *)data);

	case WSDISPLAYIO_PUTCMAP:
		error = set_cmap(sc, (struct wsdisplay_cmap *)data);
		if (error == 0)
			bt478_loadcmap(sc);
		return (error);

	case WSDISPLAYIO_SVIDEO:
		turnoff = *(int *)data == WSDISPLAYIO_VIDEO_OFF;
		if ((dc->dc_blanked == 0) ^ turnoff) {
			dc->dc_blanked = turnoff;
			pm_screenblank(sc);
		}
		return (0);

	case WSDISPLAYIO_GVIDEO:
		*(u_int *)data = dc->dc_blanked ?
		    WSDISPLAYIO_VIDEO_OFF : WSDISPLAYIO_VIDEO_ON;
		return (0);

	case WSDISPLAYIO_GCURPOS:
		*(struct wsdisplay_curpos *)data = sc->sc_cursor.cc_pos;
		return (0);

	case WSDISPLAYIO_SCURPOS:
		set_curpos(sc, (struct wsdisplay_curpos *)data);
		pcc_set_curpos(sc);
		return (0);

	case WSDISPLAYIO_GCURMAX:
		((struct wsdisplay_curpos *)data)->x =
		((struct wsdisplay_curpos *)data)->y = CURSOR_MAX_SIZE;
		return (0);

	case WSDISPLAYIO_GCURSOR:
		return get_cursor(sc, (struct wsdisplay_cursor *)data);

	case WSDISPLAYIO_SCURSOR:
		return set_cursor(sc, (struct wsdisplay_cursor *)data);
	}
	return ENOTTY;
}

int
pmmmap(v, offset, prot)
	void *v;
	off_t offset;
	int prot;
{
	struct pm_softc *sc = v;

	if (offset >= sc->sc_dc->dc_size)
		return -1;
	return mips_btop(sc->sc_dc->dc_paddr + offset);
}

int
pm_alloc_screen(v, type, cookiep, curxp, curyp, attrp)
	void *v;
	const struct wsscreen_descr *type;
	void **cookiep;
	int *curxp, *curyp;
	long *attrp;
{
	struct pm_softc *sc = v;
	long defattr;

	if (sc->nscreens > 0)
		return (ENOMEM);

	*cookiep = &sc->sc_dc->dc_rcons; /* one and only for now */
	*curxp = 0;
	*curyp = 0;
	rcons_alloc_attr(&sc->sc_dc->dc_rcons, 0, 0, 0, &defattr);
	*attrp = defattr;
	sc->nscreens++;
	return (0);
}

void
pm_free_screen(v, cookie)
	void *v;
	void *cookie;
{
	struct pm_softc *sc = v;

	if (sc->sc_dc == &pm_console_dc)
		panic("pm_free_screen: console");

	sc->nscreens--;
}

void
pm_show_screen(v, cookie)
	void *v;
	void *cookie;
{
}

int
pm_load_font(v, cookie, first, num, stride, data)
	void *v;
	void *cookie;
	int first, num, stride;
	void *data;
{
	return (EINVAL);
}

int
pm_cnattach(addr)
	tc_addr_t addr;
{
        struct fb_devconfig *dcp = &pm_console_dc;
        long defattr;

        pm_getdevconfig(addr, dcp);
 
        rcons_alloc_attr(&dcp->dc_rcons, 0, 0, 0, &defattr);

        wsdisplay_cnattach(&pm_stdscreen, &dcp->dc_rcons,
                           0, 0, defattr);
        pm_consaddr = addr;
        return (0);
}

void
pminit(dc)
	struct fb_devconfig *dc;
{
	int i;
	volatile struct bt478reg *vdac =
		(void *)MIPS_PHYS_TO_KSEG1(KN01_SYS_BT478);
	volatile struct pccreg *pcc =
		(void *)MIPS_PHYS_TO_KSEG1(KN01_SYS_PCC);

	pcc->pcc_cmdr = PCC_FOPB | PCC_VBHI;

	*(volatile u_int8_t *)MIPS_PHYS_TO_KSEG1(KN01_SYS_PMASK) = 0xff;

	vdac->bt_mapWA = 0;	kn01_wbflush();
	vdac->bt_map = 0;	kn01_wbflush();
	vdac->bt_map = 0;	kn01_wbflush();
	vdac->bt_map = 0;	kn01_wbflush();
	for (i = 1; i < CMAP_SIZE; i++) {
		vdac->bt_mapWA = i;	kn01_wbflush();
		vdac->bt_map = 0xff;	kn01_wbflush();
		vdac->bt_map = 0xff;	kn01_wbflush();
		vdac->bt_map = 0xff;	kn01_wbflush();
	}
}

void
pm_screenblank(sc)
	struct pm_softc *sc;
{
#if 0 /* XXX later XXX */
	struct fb_devconfig *dc = sc->sc_dc;

	if (dc->dc_blanked) {
		/* blank screen */
		/* turnoff hardware cursor */
	}
	else {
		/* restore current colormap */
		/* turnon hardware cursor */
	}
#endif
}

static int
get_cmap(sc, p)
	struct pm_softc *sc;
	struct wsdisplay_cmap *p;
{
	u_int index = p->index, count = p->count;

	if (index >= CMAP_SIZE || (index + count) > CMAP_SIZE)
		return (EINVAL);

	if (!useracc(p->red, count, B_WRITE) ||
	    !useracc(p->green, count, B_WRITE) ||
	    !useracc(p->blue, count, B_WRITE))
		return (EFAULT);

	copyout(&sc->sc_cmap.r[index], p->red, count);
	copyout(&sc->sc_cmap.r[index], p->green, count);
	copyout(&sc->sc_cmap.r[index], p->blue, count);

	return (0);
}

static int
set_cmap(sc, p)
	struct pm_softc *sc;
	struct wsdisplay_cmap *p;
{
	u_int index = p->index, count = p->count;

	if (index >= CMAP_SIZE || (index + count) > CMAP_SIZE)
		return (EINVAL);

	if (!useracc(p->red, count, B_READ) ||
	    !useracc(p->green, count, B_READ) ||
	    !useracc(p->blue, count, B_READ))
		return (EFAULT);

	copyin(p->red, &sc->sc_cmap.r[index], count);
	copyin(p->green, &sc->sc_cmap.g[index], count);
	copyin(p->blue, &sc->sc_cmap.b[index], count);

	return (0);
}

static int
set_cursor(sc, p)
	struct pm_softc *sc;
	struct wsdisplay_cursor *p;
{
#define	cc (&sc->sc_cursor)
	int v, index, count;

	v = p->which;
	if (v & WSDISPLAY_CURSOR_DOCMAP) {
		index = p->cmap.index;
		count = p->cmap.count;
		if (index >= 2 || (index + count) > 2)
			return (EINVAL);
		if (!useracc(p->cmap.red, count, B_READ) ||
		    !useracc(p->cmap.green, count, B_READ) ||
		    !useracc(p->cmap.blue, count, B_READ))
			return (EFAULT);

		count = p->cmap.count;
		copyin(p->cmap.red, &cc->cc_color[index], count);
		copyin(p->cmap.green, &cc->cc_color[index + 2], count);
		copyin(p->cmap.blue, &cc->cc_color[index + 4], count);

		bt478_load_curcmap(sc);
	}
	if (v & WSDISPLAY_CURSOR_DOSHAPE) {
		if (p->size.x > CURSOR_MAX_SIZE || p->size.y > CURSOR_MAX_SIZE)
			return (EINVAL);
		count = (CURSOR_MAX_SIZE / NBBY) * p->size.y;
		if (!useracc(p->image, count, B_READ) ||
		    !useracc(p->mask, count, B_READ))
			return (EFAULT);
		cc->cc_size = p->size;
		memset(cc->cc_image, 0, sizeof cc->cc_image);
		copyin(p->image, (caddr_t)cc->cc_image, count);
		copyin(p->mask, (caddr_t)(cc->cc_image+CURSOR_MAX_SIZE), count);

		pcc_load_curshape(sc);
	}
	if (v & WSDISPLAY_CURSOR_DOCUR) {
		cc->cc_hot = p->hot;
		pcc_show_cursor(sc, p->enable);
	}
	if (v & WSDISPLAY_CURSOR_DOPOS) {
		set_curpos(sc, &p->pos);
		pcc_set_curpos(sc);
	}

	return (0);
#undef cc
}

static int
get_cursor(sc, p)
	struct pm_softc *sc;
	struct wsdisplay_cursor *p;
{
	return (ENOTTY); /* XXX */
}

void
set_curpos(sc, curpos)
	struct pm_softc *sc;
	struct wsdisplay_curpos *curpos;
{
	struct fb_devconfig *dc = sc->sc_dc;
	int x = curpos->x, y = curpos->y;

	if (y < 0)
		y = 0;
	else if (y > dc->dc_ht)
		y = dc->dc_ht;
	if (x < 0)
		x = 0;
	else if (x > dc->dc_wid)
		x = dc->dc_wid;
	sc->sc_cursor.cc_pos.x = x;
	sc->sc_cursor.cc_pos.y = y;
}

void
bt478_loadcmap(sc)
	struct pm_softc *sc;
{
	int i;
	struct hwcmap *cm = &sc->sc_cmap;
	volatile struct bt478reg *vdac = sc->sc_vdac;
	
	for (i = 0; i < CMAP_SIZE; i++) {
		vdac->bt_mapWA = i;	 kn01_wbflush();
		vdac->bt_map = cm->r[i]; kn01_wbflush();
		vdac->bt_map = cm->g[i]; kn01_wbflush();
		vdac->bt_map = cm->b[i]; kn01_wbflush();
	}
}

void
bt478_load_curcmap(sc)
	struct pm_softc *sc;
{
	u_int8_t *cp = sc->sc_cursor.cc_color;
	volatile struct bt478reg *vdac = sc->sc_vdac;

	vdac->bt_overWA = 0x04;	kn01_wbflush();
	vdac->bt_over = cp[1];	kn01_wbflush();
	vdac->bt_over = cp[3];	kn01_wbflush();
	vdac->bt_over = cp[5];	kn01_wbflush();

	vdac->bt_overWA = 0x08;	kn01_wbflush();
	vdac->bt_over = 0x00;	kn01_wbflush();
	vdac->bt_over = 0x00;	kn01_wbflush();
	vdac->bt_over = 0x7f;	kn01_wbflush();

	vdac->bt_overWA = 0x0c;	kn01_wbflush();
	vdac->bt_over = cp[0];	kn01_wbflush();
	vdac->bt_over = cp[2];	kn01_wbflush();
	vdac->bt_over = cp[4];	kn01_wbflush();
}

void
pcc_load_curshape(sc)
	struct pm_softc *sc;
{
	volatile struct pccreg *pcc = sc->sc_pcc;
	u_int16_t *bp;
	int i;

	pcc->pcc_cmdr = sc->sc_pcccmdr | PCC_LODSA;
	bp = sc->sc_cursor.cc_image;
	for (i = 0; i < sizeof(sc->sc_cursor.cc_image); i++) {
		pcc->pcc_memory = *bp++;
	}
	pcc->pcc_cmdr = sc->sc_pcccmdr;
}

void
pcc_set_curpos(sc)
	struct pm_softc *sc;
{
	volatile struct pccreg *pcc = sc->sc_pcc;

	pcc->pcc_xpos = sc->sc_cursor.cc_pos.x + sc->magic_x;
	pcc->pcc_ypos = sc->sc_cursor.cc_pos.y + sc->magic_y;
}

void
pcc_show_cursor(sc, enable)
	struct pm_softc *sc;
	int enable;
{
	volatile struct pccreg *pcc = sc->sc_pcc;

	if (enable)
		sc->sc_pcccmdr |=  (PCC_ENPA | PCC_ENPB);
	else
		sc->sc_pcccmdr &= ~(PCC_ENPA | PCC_ENPB);
	pcc->pcc_cmdr = sc->sc_pcccmdr;
}
