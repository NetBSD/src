/* $NetBSD: vga.c,v 1.10 1998/12/30 13:54:04 augustss Exp $ */

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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/queue.h>
#include <machine/bus.h>

#include <dev/isa/isavar.h>
#include <dev/isa/isareg.h>

#include <dev/ic/mc6845reg.h>
#include <dev/ic/pcdisplayvar.h>
#include <dev/ic/vgareg.h>
#include <dev/ic/vgavar.h>

#include <dev/wscons/wsdisplayvar.h>
#include <dev/wscons/wsconsio.h>

#include <dev/ic/pcdisplay.h>

struct vgascreen {
	struct pcdisplayscreen pcs;

	LIST_ENTRY(vgascreen) next;

	struct vga_config *cfg;

	/* videostate */
	int fontset;
	/* font data */
	/* palette */

	int mindispoffset, maxdispoffset;
};

struct vga_config {
	struct vga_handle hdl;

	int nscreens;
	LIST_HEAD(, vgascreen) screens;
	struct vgascreen *active; /* current display */

	int vc_biosmapped;
	bus_space_tag_t vc_biostag;
	bus_space_handle_t vc_bioshdl;
};

static int vgaconsole, vga_console_type, vga_console_attached;
static struct vgascreen vga_console_screen;
static struct vga_config vga_console_vc;

void vga_init_screen __P((struct vga_config *, struct vgascreen *,
			  const struct wsscreen_descr *,
			  int, long *));
void vga_init __P((struct vga_config *, bus_space_tag_t,
		   bus_space_tag_t));

static int	vga_alloc_attr __P((void *, int, int, int, long *));
void	vga_copyrows __P((void *, int, int, int));

const struct wsdisplay_emulops vga_emulops = {
	pcdisplay_cursor,
	pcdisplay_mapchar,
	pcdisplay_putchar,
	pcdisplay_copycols,
	pcdisplay_erasecols,
	vga_copyrows,
	pcdisplay_eraserows,
	vga_alloc_attr
};

/*
 * translate WS(=ANSI) color codes to standard pc ones
 */
static unsigned char fgansitopc[] = {
#ifdef __alpha__
	/*
	 * XXX DEC HAS SWITCHED THE CODES FOR BLUE AND RED!!!
	 * XXX We should probably not bother with this
	 * XXX (reinitialize the palette registers).
	 */
	FG_BLACK, FG_BLUE, FG_GREEN, FG_CYAN, FG_RED,
	FG_MAGENTA, FG_BROWN, FG_LIGHTGREY
#else
	FG_BLACK, FG_RED, FG_GREEN, FG_BROWN, FG_BLUE,
	FG_MAGENTA, FG_CYAN, FG_LIGHTGREY
#endif
}, bgansitopc[] = {
#ifdef __alpha__
	BG_BLACK, BG_BLUE, BG_GREEN, BG_CYAN, BG_RED,
	BG_MAGENTA, BG_BROWN, BG_LIGHTGREY
#else
	BG_BLACK, BG_RED, BG_GREEN, BG_BROWN, BG_BLUE,
	BG_MAGENTA, BG_CYAN, BG_LIGHTGREY
#endif
};

const struct wsscreen_descr vga_stdscreen = {
	"80x25", 80, 25,
	&vga_emulops,
	8, 16,
	WSSCREEN_WSCOLORS | WSSCREEN_HILIT | WSSCREEN_BLINK
}, vga_stdscreen_mono = {
	"80x25", 80, 25,
	&vga_emulops,
	8, 16,
	WSSCREEN_HILIT | WSSCREEN_UNDERLINE | WSSCREEN_BLINK | WSSCREEN_REVERSE
};

const struct wsscreen_descr vga_50lscreen = {
	"80x50", 80, 50,
	&vga_emulops,
	8, 8,
	WSSCREEN_WSCOLORS | WSSCREEN_HILIT | WSSCREEN_BLINK
}, vga_50lscreen_mono = {
	"80x50", 80, 50,
	&vga_emulops,
	8, 8,
	WSSCREEN_HILIT | WSSCREEN_UNDERLINE | WSSCREEN_BLINK | WSSCREEN_REVERSE
};

const struct wsscreen_descr *_vga_scrlist[] = {
	&vga_stdscreen,
	&vga_50lscreen,
	/* XXX other formats, graphics screen? */
}, *_vga_scrlist_mono[] = {
	&vga_stdscreen_mono,
	&vga_50lscreen_mono,
	/* XXX other formats, graphics screen? */
};

const struct wsscreen_list vga_screenlist = {
	sizeof(_vga_scrlist) / sizeof(struct wsscreen_descr *),
	_vga_scrlist
}, vga_screenlist_mono = {
	sizeof(_vga_scrlist_mono) / sizeof(struct wsscreen_descr *),
	_vga_scrlist_mono
};

static int	vga_ioctl __P((void *, u_long, caddr_t, int, struct proc *));
static int	vga_mmap __P((void *, off_t, int));
static int	vga_alloc_screen __P((void *, const struct wsscreen_descr *,
				      void **, int *, int *, long *));
static void	vga_free_screen __P((void *, void *));
static void	vga_show_screen __P((void *, void *));
static int	vga_load_font __P((void *, void *, int, int, int, void *));

const struct wsdisplay_accessops vga_accessops = {
	vga_ioctl,
	vga_mmap,
	vga_alloc_screen,
	vga_free_screen,
	vga_show_screen,
	vga_load_font
};

/*
 * The following functions implement back-end configuration grabbing
 * and attachment.
 */
int
vga_common_probe(iot, memt)
	bus_space_tag_t iot, memt;
{
	bus_space_handle_t ioh_vga, ioh_6845, memh;
	u_int8_t regval;
	u_int16_t vgadata;
	int gotio_vga, gotio_6845, gotmem, mono, rv;
	int dispoffset;

	gotio_vga = gotio_6845 = gotmem = rv = 0;

	if (bus_space_map(iot, 0x3c0, 0x10, 0, &ioh_vga))
		goto bad;
	gotio_vga = 1;

	/* read "misc output register" */
	regval = bus_space_read_1(iot, ioh_vga, 0xc);
	mono = !(regval & 1);

	if (bus_space_map(iot, (mono ? 0x3b0 : 0x3d0), 0x10, 0, &ioh_6845))
		goto bad;
	gotio_6845 = 1;

	if (bus_space_map(memt, 0xa0000, 0x20000, 0, &memh))
		goto bad;
	gotmem = 1;

	dispoffset = (mono ? 0x10000 : 0x18000);

	vgadata = bus_space_read_2(memt, memh, dispoffset);
	bus_space_write_2(memt, memh, dispoffset, 0xa55a);
	if (bus_space_read_2(memt, memh, dispoffset) != 0xa55a)
		goto bad;
	bus_space_write_2(memt, memh, dispoffset, vgadata);

	/*
	 * check if this is really a VGA
	 * (try to write "Color Select" register as XFree86 does)
	 * XXX check before if at least EGA?
	 */
	/* reset state */
	(void) bus_space_read_1(iot, ioh_6845, 10);
	bus_space_write_1(iot, ioh_vga, VGA_ATC_INDEX,
			  20 | 0x20); /* colselect | enable */
	regval = bus_space_read_1(iot, ioh_vga, VGA_ATC_DATAR);
	/* toggle the implemented bits */
	bus_space_write_1(iot, ioh_vga, VGA_ATC_DATAW, regval ^ 0x0f);
	bus_space_write_1(iot, ioh_vga, VGA_ATC_INDEX,
			  20 | 0x20);
	/* read back */
	if (bus_space_read_1(iot, ioh_vga, VGA_ATC_DATAR) != (regval ^ 0x0f))
		goto bad;
	/* restore contents */
	bus_space_write_1(iot, ioh_vga, VGA_ATC_DATAW, regval);

	rv = 1;
bad:
	if (gotio_vga)
		bus_space_unmap(iot, ioh_vga, 0x10);
	if (gotio_6845)
		bus_space_unmap(iot, ioh_6845, 0x10);
	if (gotmem)
		bus_space_unmap(memt, memh, 0x20000);

	return (rv);
}

void
vga_init_screen(vc, scr, type, existing, attrp)
	struct vga_config *vc;
	struct vgascreen *scr;
	const struct wsscreen_descr *type;
	int existing;
	long *attrp;
{
	int cpos;
	int res;

	scr->cfg = vc;
	scr->pcs.hdl = (struct pcdisplay_handle *)&vc->hdl;
	scr->pcs.type = type;
	scr->pcs.active = 0;
	scr->mindispoffset = 0;
	scr->maxdispoffset = 0x8000 - type->nrows * type->ncols * 2;

	if (existing) {
		cpos = vga_6845_read(&vc->hdl, cursorh) << 8;
		cpos |= vga_6845_read(&vc->hdl, cursorl);

		/* make sure we have a valid cursor position */
		if (cpos < 0 || cpos >= type->nrows * type->ncols)
			cpos = 0;

		scr->pcs.dispoffset = vga_6845_read(&vc->hdl, startadrh) << 9;
		scr->pcs.dispoffset |= vga_6845_read(&vc->hdl, startadrl) << 1;

		/* make sure we have a valid memory offset */
		if (scr->pcs.dispoffset < scr->mindispoffset ||
		    scr->pcs.dispoffset > scr->maxdispoffset)
			scr->pcs.dispoffset = scr->mindispoffset;
	} else {
		cpos = 0;
		scr->pcs.dispoffset = scr->mindispoffset;
	}

	scr->pcs.vc_crow = cpos / type->ncols;
	scr->pcs.vc_ccol = cpos % type->ncols;
	scr->pcs.cursoron = 1;

#ifdef __alpha__
	if (!vc->hdl.vh_mono)
		/*
		 * DEC firmware uses a blue background.
		 */
		res = vga_alloc_attr(scr, WSCOL_WHITE, WSCOL_BLUE,
				     WSATTR_WSCOLORS, attrp);
	else
#endif
	res = vga_alloc_attr(scr, 0, 0, 0, attrp);
#ifdef DIAGNOSTIC
	if (res)
		panic("vga_init_screen: attribute botch");
#endif

	scr->pcs.mem = NULL;
	if (type == &vga_stdscreen)	/* XXX do it better! */
		scr->fontset = 0;
	else
		scr->fontset = 1;

	vc->nscreens++;
	LIST_INSERT_HEAD(&vc->screens, scr, next);
}

void
vga_init(vc, iot, memt)
	struct vga_config *vc;
	bus_space_tag_t iot, memt;
{
	struct vga_handle *vh = &vc->hdl;
	u_int8_t mor;

        vh->vh_iot = iot;
        vh->vh_memt = memt;

        if (bus_space_map(vh->vh_iot, 0x3c0, 0x10, 0, &vh->vh_ioh_vga))
                panic("vga_common_setup: couldn't map vga io");

	/* read "misc output register" */
	mor = bus_space_read_1(vh->vh_iot, vh->vh_ioh_vga, 0xc);
	vh->vh_mono = !(mor & 1);

	if (bus_space_map(vh->vh_iot, (vh->vh_mono ? 0x3b0 : 0x3d0), 0x10, 0,
			  &vh->vh_ioh_6845))
                panic("vga_common_setup: couldn't map 6845 io");

        if (bus_space_map(vh->vh_memt, 0xa0000, 0x20000, 0, &vh->vh_allmemh))
                panic("vga_common_setup: couldn't map memory");

        if (bus_space_subregion(vh->vh_memt, vh->vh_allmemh,
				(vh->vh_mono ? 0x10000 : 0x18000), 0x8000,
				&vh->vh_memh))
                panic("vga_common_setup: mem subrange failed");

	/* should only reserve the space (no need to map - save KVM) */
	vc->vc_biostag = memt;
	if (bus_space_map(vc->vc_biostag, 0xc0000, 0x8000, 0,
			  &vc->vc_bioshdl))
		vc->vc_biosmapped = 0;
	else
		vc->vc_biosmapped = 1;

	vc->nscreens = 0;
	LIST_INIT(&vc->screens);
	vc->active = NULL;
}

void
vga_common_attach(self, iot, memt, type)
	struct device *self;
	bus_space_tag_t iot, memt;
	int type;
{
	int console;
	struct vga_config *vc;
	struct wsemuldisplaydev_attach_args aa;

	console = vga_is_console(iot, type);

	if (console) {
		vc = &vga_console_vc;
		vga_console_attached = 1;
	} else {
		vc = malloc(sizeof(struct vga_config), M_DEVBUF, M_WAITOK);
		vga_init(vc, iot, memt);
	}

	aa.console = console;
	aa.scrdata = (vc->hdl.vh_mono ? &vga_screenlist_mono : &vga_screenlist);
	aa.accessops = &vga_accessops;
	aa.accesscookie = vc;

        config_found(self, &aa, wsemuldisplaydevprint);
}

int
vga_cnattach(iot, memt, type, check)
	bus_space_tag_t iot, memt;
	int type, check;
{
	long defattr;
	const struct wsscreen_descr *scr;

	if (check && !vga_common_probe(iot, memt))
		return (ENXIO);

	/* set up bus-independent VGA configuration */
	vga_init(&vga_console_vc, iot, memt);
	scr = vga_console_vc.hdl.vh_mono ? &vga_stdscreen_mono : &vga_stdscreen;
	vga_init_screen(&vga_console_vc, &vga_console_screen, scr, 1, &defattr);

	vga_console_screen.pcs.active = 1;
	vga_console_vc.active = &vga_console_screen;

	wsdisplay_cnattach(scr, &vga_console_screen,
			   vga_console_screen.pcs.vc_ccol,
			   vga_console_screen.pcs.vc_crow,
			   defattr);

	vgaconsole = 1;
	vga_console_type = type;
	return (0);
}

int
vga_is_console(iot, type)
	bus_space_tag_t iot;
	int type;
{
	if (vgaconsole &&
	    !vga_console_attached &&
	    iot == vga_console_vc.hdl.vh_iot &&
	    (vga_console_type == -1 || (type == vga_console_type)))
		return (1);
	return (0);
}

int
vga_ioctl(v, cmd, data, flag, p)
	void *v;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
#if 0
	struct vga_config *vc = v;
#endif

	switch (cmd) {
#if 0
	case WSDISPLAYIO_GTYPE:
		*(int *)data = vc->vc_type;
		/* XXX should get detailed hardware information here */
		return 0;
#else
	case WSDISPLAYIO_GTYPE:
		*(int *)data = WSDISPLAY_TYPE_UNKNOWN;
		return 0;
#endif
	case WSDISPLAYIO_GINFO:
	case WSDISPLAYIO_GETCMAP:
	case WSDISPLAYIO_PUTCMAP:
	case WSDISPLAYIO_GVIDEO:
	case WSDISPLAYIO_SVIDEO:
	case WSDISPLAYIO_GCURPOS:
	case WSDISPLAYIO_SCURPOS:
	case WSDISPLAYIO_GCURMAX:
	case WSDISPLAYIO_GCURSOR:
	case WSDISPLAYIO_SCURSOR:
		/* NONE of these operations are by the generic VGA driver. */
		return ENOTTY;
	}
	return -1;
}

static int
vga_mmap(v, offset, prot)
	void *v;
	off_t offset;
	int prot;
{

	/* XXX */
	return -1;
}

int
vga_alloc_screen(v, type, cookiep, curxp, curyp, defattrp)
	void *v;
	const struct wsscreen_descr *type;
	void **cookiep;
	int *curxp, *curyp;
	long *defattrp;
{
	struct vga_config *vc = v;
	struct vgascreen *scr;

	if (vc->nscreens == 1) {
		/*
		 * When allocating the second screen, get backing store
		 * for the first one too.
		 * XXX We could be more clever and use video RAM.
		 */
		vc->screens.lh_first->pcs.mem =
		  malloc(type->ncols * type->nrows * 2, M_DEVBUF, M_WAITOK);
	}

	scr = malloc(sizeof(struct vgascreen), M_DEVBUF, M_WAITOK);
	vga_init_screen(vc, scr, type, vc->nscreens == 0, defattrp);

	if (vc->nscreens == 1) {
		scr->pcs.active = 1;
		vc->active = scr;
	} else {
		scr->pcs.mem = malloc(type->ncols * type->nrows * 2,
				      M_DEVBUF, M_WAITOK);
		pcdisplay_eraserows(&scr->pcs, 0, type->nrows, *defattrp);
	}

	*cookiep = scr;
	*curxp = scr->pcs.vc_ccol;
	*curyp = scr->pcs.vc_crow;

	return (0);
}

void
vga_free_screen(v, cookie)
	void *v;
	void *cookie;
{
	struct vgascreen *vs = cookie;

	LIST_REMOVE(vs, next);
	if (vs != &vga_console_screen)
		free(vs, M_DEVBUF);
	else
		panic("vga_free_screen: console");
}

void
vga_show_screen(v, cookie)
	void *v;
	void *cookie;
{
	struct vgascreen *scr = cookie, *oldscr;
	struct vga_config *vc = scr->cfg;
	struct vga_handle *vh = &vc->hdl;
	const struct wsscreen_descr *type = scr->pcs.type, *oldtype;
	int i;

	oldscr = vc->active;
	oldtype = oldscr->pcs.type;
#ifdef DIAGNOSTIC
	if (!oldscr->pcs.active)
		panic("vga_show_screen: not active");
#endif
	if (scr == oldscr) {
		return;
	}
#ifdef DIAGNOSTIC
	if (scr->pcs.active)
		panic("vga_show_screen: active");
#endif

	oldscr->pcs.active = 0;

	for (i = 0; i < oldtype->ncols * oldtype->nrows; i++)
		oldscr->pcs.mem[i] = bus_space_read_2(vh->vh_memt, vh->vh_memh,
						oldscr->pcs.dispoffset + i * 2);

	if (oldtype != type)
		vga_setscreentype(vh, type);
	if (oldscr->fontset != scr->fontset)
		vga_setfontset(vh, scr->fontset);
	/* swich colours */

	scr->pcs.dispoffset = scr->mindispoffset;
	if (scr->pcs.dispoffset != oldscr->pcs.dispoffset) {
		vga_6845_write(vh, startadrh, scr->pcs.dispoffset >> 9);
		vga_6845_write(vh, startadrl, scr->pcs.dispoffset >> 1);
	}

	for (i = 0; i < type->ncols * type->nrows; i++)
		bus_space_write_2(vh->vh_memt, vh->vh_memh,
				  scr->pcs.dispoffset + i * 2,
				  scr->pcs.mem[i]);

	scr->pcs.active = 1;
	vc->active = scr;

	pcdisplay_cursor(&scr->pcs, scr->pcs.cursoron,
			 scr->pcs.vc_crow, scr->pcs.vc_ccol);
}

static int
vga_load_font(v, cookie, first, num, stride, data)
	void *v;
	void *cookie;
	int first, num, stride;
	void *data;
{
	struct vgascreen *scr = cookie;
	struct vga_config *vc = scr->cfg;

	if (stride != 1)
		return (EINVAL); /* XXX 1 byte per line */

	vga_loadchars(&vc->hdl, scr->fontset, first, num,
		      scr->pcs.type->fontheight, data);
	return (0);
}

static int
vga_alloc_attr(id, fg, bg, flags, attrp)
	void *id;
	int fg, bg;
	int flags;
	long *attrp;
{
	struct vgascreen *scr = id;
	struct vga_config *vc = scr->cfg;

	if (vc->hdl.vh_mono) {
		if (flags & WSATTR_WSCOLORS)
			return (EINVAL);
		if (flags & WSATTR_REVERSE)
			*attrp = 0x70;
		else
			*attrp = 0x07;
		if (flags & WSATTR_UNDERLINE)
			*attrp |= FG_UNDERLINE;
		if (flags & WSATTR_HILIT)
			*attrp |= FG_INTENSE;
	} else {
		if (flags & (WSATTR_UNDERLINE | WSATTR_REVERSE))
			return (EINVAL);
		if (flags & WSATTR_WSCOLORS)
			*attrp = fgansitopc[fg] | bgansitopc[bg];
		else
			*attrp = 7;
		if (flags & WSATTR_HILIT)
			*attrp += 8;
	}
	if (flags & WSATTR_BLINK)
		*attrp |= FG_BLINK;
	return (0);
}

void
vga_copyrows(id, srcrow, dstrow, nrows)
	void *id;
	int srcrow, dstrow, nrows;
{
	struct vgascreen *scr = id;
	bus_space_tag_t memt = scr->pcs.hdl->ph_memt;
	bus_space_handle_t memh = scr->pcs.hdl->ph_memh;
	int ncols = scr->pcs.type->ncols;
	bus_size_t srcoff, dstoff;

	srcoff = srcrow * ncols + 0;
	dstoff = dstrow * ncols + 0;

	if (scr->pcs.active) {
		if (dstrow == 0 && (srcrow + nrows == scr->pcs.type->nrows)) {
			/* scroll up whole screen */
			if ((scr->pcs.dispoffset + srcrow * ncols * 2)
			    <= scr->maxdispoffset) {
				scr->pcs.dispoffset += srcrow * ncols * 2;
			} else {
				bus_space_copy_region_2(memt, memh,
					scr->pcs.dispoffset + srcoff * 2,
					memh, scr->mindispoffset,
					nrows * ncols);
				scr->pcs.dispoffset = scr->mindispoffset;
			}
			vga_6845_write(&scr->cfg->hdl, startadrh,
				       scr->pcs.dispoffset >> 9);
			vga_6845_write(&scr->cfg->hdl, startadrl,
				       scr->pcs.dispoffset >> 1);
		} else {
			bus_space_copy_region_2(memt, memh,
					scr->pcs.dispoffset + srcoff * 2,
					memh, scr->pcs.dispoffset + dstoff * 2,
					nrows * ncols);
		}
	} else
		bcopy(&scr->pcs.mem[srcoff], &scr->pcs.mem[dstoff],
		      nrows * ncols * 2);
}
