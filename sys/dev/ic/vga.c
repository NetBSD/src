/* $NetBSD: vga.c,v 1.1 1998/03/22 15:11:49 drochner Exp $ */

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

#include <dev/wscons/wsdisplayvar.h>
#include <dev/wscons/wsconsio.h>

#include <dev/ic/vgareg.h>
#include <dev/ic/vgavar.h>

struct vgascreen {
	LIST_ENTRY(vgascreen) next;

	struct vga_config *cfg;

	const struct wsscreen_descr *type;

	u_int16_t *mem; /* backing store for contents */
	/* videostate */
	int fontset;
	/* font data */
	/* palette */

	int	cursoron;		/* cursor displayed? */
	int	vc_ccol, vc_crow;	/* current cursor position */

	char	vc_so;			/* in standout mode? */
	char	vc_at;			/* normal attributes */
	char	vc_so_at;		/* standout attributes */
};

struct vga_config {
	struct vga_handle hdl;

	int nscreens;
	LIST_HEAD(, vgascreen) screens;
	struct vgascreen *active; /* current display */
};

static int vgaconsole, vga_console_type, vga_console_attached;
static struct vgascreen vga_console_screen;
static struct vga_config vga_console_vc;

void vga_init_screen __P((struct vgascreen *, const struct wsscreen_descr *,
			  int));
void vga_init __P((struct vga_config *, bus_space_tag_t,
		   bus_space_tag_t));

static void	vga_cursor __P((void *, int, int, int));
static void	vga_putstr __P((void *, int, int, char *, int));
static void	vga_copycols __P((void *, int, int, int,int));
static void	vga_erasecols __P((void *, int, int, int));
static void	vga_copyrows __P((void *, int, int, int));
static void	vga_eraserows __P((void *, int, int));

const struct wsdisplay_emulops vga_emulops = {
	vga_cursor,
	vga_putstr,
	vga_copycols,
	vga_erasecols,
	vga_copyrows,
	vga_eraserows,
};

const struct wsscreen_descr vga_stdscreen = {
	"80x25", 80, 25,
	&vga_emulops,
	8, 16
};

const struct wsscreen_descr vga_50lscreen = {
	"80x50", 80, 50,
	&vga_emulops,
	8, 8
};

const struct wsscreen_descr *_scrlist[] = {
	&vga_stdscreen,
	&vga_50lscreen,
	/* XXX other formats, graphics screen? */
};

struct wsscreen_list vga_screenlist = {
	sizeof(_scrlist) / sizeof(struct wsscreen_descr *), _scrlist
};

static int	vga_ioctl __P((void *, u_long, caddr_t, int, struct proc *));
static int	vga_mmap __P((void *, off_t, int));
static int	vga_alloc_screen __P((void *, const struct wsscreen_descr *,
				      void **, int *, int *));
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
	u_int8_t mor;
	u_int16_t vgadata;
	int gotio_vga, gotio_6845, gotmem, mono, rv;
	int dispoffset;

	gotio_vga = gotio_6845 = gotmem = rv = 0;

	if (bus_space_map(iot, 0x3c0, 0x10, 0, &ioh_vga))
		goto bad;
	gotio_vga = 1;

	/* read "misc output register" */
	mor = bus_space_read_1(iot, ioh_vga, 0xc);
	mono = !(mor & 1);

	if (bus_space_map(iot, (mono ? 0x3b0 : 0x3d0), 0x10, 0, &ioh_6845))
		goto bad;
	gotio_6845 = 1;

	if (bus_space_map(memt, 0xa0000, 0x20000, 0, &memh))
		goto bad;
	gotmem = 1;

	dispoffset = (mono ? 0x10000 : 0x18000);

	vgadata = bus_space_read_2(memt, memh, dispoffset);
	bus_space_write_2(memt, memh, dispoffset, 0xa55a);
	rv = (bus_space_read_2(memt, memh, dispoffset) == 0xa55a);
	bus_space_write_2(memt, memh, dispoffset, vgadata);

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
vga_init_screen(scr, type, existing)
	struct vgascreen *scr;
	const struct wsscreen_descr *type;
	int existing;
{
	struct vga_config *vc = scr->cfg;
	int cpos = 0;

	scr->type = type;

	if (existing) {
		cpos = vga_6845_read(&vc->hdl, cursorh) << 8;
		cpos |= vga_6845_read(&vc->hdl, cursorl);

		/* make sure we have a valid cursor position */
		if (cpos < 0 || cpos >= type->nrows * type->ncols)
			cpos = 0;
	}

	scr->vc_crow = cpos / type->ncols;
	scr->vc_ccol = cpos % type->ncols;

	scr->vc_so = 0;
#ifdef __alpha__
	/*
	 * XXX DEC HAS SWITCHED THE CODES FOR BLUE AND RED!!!
	 * XXX Therefore, though the comments say "blue bg", the code uses
	 * XXX the value for a red background!
	 */
	scr->vc_at = 0x40 | 0x0f;		/* blue bg|white fg */
	scr->vc_so_at = 0x40 | 0x0f | 0x80;	/* blue bg|white fg|blink */
#else
	scr->vc_at = 0x00 | 0xf;		/* black bg|white fg */
	scr->vc_so_at = 0x00 | 0xf | 0x80;	/* black bg|white fg|blink */
#endif

	scr->mem = NULL;
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
	vh->mono = !(mor & 1);

	if (bus_space_map(vh->vh_iot, (vh->mono ? 0x3b0 : 0x3d0), 0x10, 0,
			  &vh->vh_ioh_6845))
                panic("vga_common_setup: couldn't map 6845 io");

        if (bus_space_map(vh->vh_memt, 0xa0000, 0x20000, 0, &vh->vh_allmemh))
                panic("vga_common_setup: couldn't map memory");

        if (bus_space_subregion(vh->vh_memt, vh->vh_allmemh,
				(vh->mono ? 0x10000 : 0x18000), 0x8000,
				&vh->vh_memh))
                panic("vga_common_setup: mem subrange failed");

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
	aa.scrdata = &vga_screenlist;
	aa.accessops = &vga_accessops;
	aa.accesscookie = vc;

        config_found(self, &aa, wsemuldisplaydevprint);
}

int
vga_cnattach(iot, memt, type, check)
	bus_space_tag_t iot, memt;
	int type, check;
{
	if (check && !vga_common_probe(iot, memt))
		return (ENXIO);

	/* set up bus-independent VGA configuration */
	vga_init(&vga_console_vc, iot, memt);
	vga_console_screen.cfg = &vga_console_vc;
	vga_init_screen(&vga_console_screen, &vga_stdscreen, 1);

	vga_console_vc.active = &vga_console_screen;

	wsdisplay_cnattach(&vga_stdscreen, &vga_console_screen,
			   vga_console_screen.vc_ccol,
			   vga_console_screen.vc_crow);

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
vga_alloc_screen(v, type, cookiep, curxp, curyp)
	void *v;
	const struct wsscreen_descr *type;
	void **cookiep;
	int *curxp, *curyp;
{
	struct vga_config *vc = v;
	struct vgascreen *scr;

	if (vc->nscreens == 1) {
		/*
		 * When allocating the second screen, get backing store
		 * for the first one too.
		 * XXX We could be more clever and use video RAM.
		 */
		vc->screens.lh_first->mem =
		  malloc(type->ncols * type->nrows * 2, M_DEVBUF, M_WAITOK);
	}

	scr = malloc(sizeof(struct vgascreen), M_DEVBUF, M_WAITOK);
	scr->cfg = vc;
	vga_init_screen(scr, type, vc->nscreens == 0);

	if (vc->nscreens == 1)
		vc->active = scr;
	else {
		scr->mem = malloc(type->ncols * type->nrows * 2,
				  M_DEVBUF, M_WAITOK);
		vga_eraserows(scr, 0, type->nrows);
	}

	*cookiep = scr;
	*curxp = scr->vc_ccol;
	*curyp = scr->vc_crow;

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
	struct vgascreen *scr = cookie;
	struct vga_config *vc = scr->cfg;
	struct vga_handle *vh = &vc->hdl;
	const struct wsscreen_descr *type = scr->type;
	int i, row, col, pos;

	if (scr == vc->active)
		return;

	for (i = 0; i < vc->active->type->ncols * vc->active->type->nrows; i++)
		vc->active->mem[i] = bus_space_read_2(vh->vh_memt, vh->vh_memh,
						      i * 2);

	if (vc->active->type != type)
		vga_setscreentype(&vc->hdl, type);
	if (vc->active->fontset != scr->fontset)
		vga_setfontset(&vc->hdl, scr->fontset);
	/* swich colours */

	for (i = 0; i < type->ncols * type->nrows; i++)
		bus_space_write_2(vh->vh_memt, vh->vh_memh, i * 2, scr->mem[i]);

	col = scr->vc_ccol;
	row = scr->vc_crow;

	if (!scr->cursoron) {
		/* XXX disable cursor how??? */
		row = col = -1;
	}

	pos = row * type->ncols + col;

	vga_6845_write(vh, cursorh, pos >> 8);
	vga_6845_write(vh, cursorl, pos);

	vc->active = scr;
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
		      scr->type->fontheight, data);
	return (0);
}

/*
 * The following functions implement the MI ANSI terminal emulation on
 * a VGA display.
 */
static void
vga_cursor(id, on, row, col)
	void *id;
	int on, row, col;
{
	struct vgascreen *scr = id;
	struct vga_config *vc = scr->cfg;
	int pos;

#if 0
	printf("vga_cursor: %d %d\n", row, col);
#endif
	scr->vc_crow = row;
	scr->vc_ccol = col;
	scr->cursoron = on;

	if (scr == vc->active) {
		if (!on) {
			/* XXX disable cursor how??? */
			row = col = -1;
		}

		pos = row * scr->type->ncols + col;

		vga_6845_write(&vc->hdl, cursorh, pos >> 8);
		vga_6845_write(&vc->hdl, cursorl, pos);
	}
}

static void
vga_putstr(id, row, col, cp, len)
	void *id;
	int row, col;
	char *cp;
	int len;
{
	struct vgascreen *scr = id;
	struct vga_config *vc = scr->cfg;
	bus_space_tag_t memt = vc->hdl.vh_memt;
	bus_space_handle_t memh = vc->hdl.vh_memh;
	int i, off;
	char attr;

	off = row * scr->type->ncols + col;
	attr = (scr->vc_so ? scr->vc_so_at : scr->vc_at);

	if (scr == vc->active) {
		off *= 2;

		for (i = 0; i < len; i++, cp++, off += 2) {
			bus_space_write_1(memt, memh, off, *cp);
			bus_space_write_1(memt, memh, off + 1, attr);
		}
	} else {
		u_int16_t *m = &scr->mem[off];

		for (i = 0; i < len; i++, cp++)
			*m++ = *cp | (attr << 8);
	}
}

static void
vga_copycols(id, row, srccol, dstcol, ncols)
	void *id;
	int row, srccol, dstcol, ncols;
{
	struct vgascreen *scr = id;
	struct vga_config *vc = scr->cfg;
	bus_space_tag_t memt = vc->hdl.vh_memt;
	bus_space_handle_t memh = vc->hdl.vh_memh;
	bus_size_t srcoff, dstoff;

	srcoff = dstoff = row * scr->type->ncols;
	srcoff += srccol;
	dstoff += dstcol;

	if (scr == vc->active)
		bus_space_copy_region_2(memt, memh, srcoff * 2,
					memh, dstoff * 2, ncols);
	else
		bcopy(&scr->mem[srcoff], &scr->mem[dstoff], ncols * 2);
}

static void
vga_erasecols(id, row, startcol, ncols)
	void *id;
	int row, startcol, ncols;
{
	struct vgascreen *scr = id;
	struct vga_config *vc = scr->cfg;
	bus_space_tag_t memt = vc->hdl.vh_memt;
	bus_space_handle_t memh = vc->hdl.vh_memh;
	bus_size_t off;
	u_int16_t val;
	int i;

	off = row * scr->type->ncols + startcol;

	val = (scr->vc_at << 8) | ' ';

	if (scr == vc->active)
		bus_space_set_region_2(memt, memh, off * 2, val, ncols);
	else
		for (i = 0; i < ncols; i++)
			scr->mem[off + i] = val;
}

static void
vga_copyrows(id, srcrow, dstrow, nrows)
	void *id;
	int srcrow, dstrow, nrows;
{
	struct vgascreen *scr = id;
	struct vga_config *vc = scr->cfg;
	bus_space_tag_t memt = vc->hdl.vh_memt;
	bus_space_handle_t memh = vc->hdl.vh_memh;
	const struct wsscreen_descr *type = scr->type;
	bus_size_t srcoff, dstoff;

	srcoff = srcrow * type->ncols + 0;
	dstoff = dstrow * type->ncols + 0;

	if (scr == vc->active)
		bus_space_copy_region_2(memt, memh, srcoff * 2,
					memh, dstoff * 2, nrows * type->ncols);
	else
		bcopy(&scr->mem[srcoff], &scr->mem[dstoff],
		      nrows * type->ncols * 2);
}

static void
vga_eraserows(id, startrow, nrows)
	void *id;
	int startrow, nrows;
{
	struct vgascreen *scr = id;
	struct vga_config *vc = scr->cfg;
	bus_space_tag_t memt = vc->hdl.vh_memt;
	bus_space_handle_t memh = vc->hdl.vh_memh;
	bus_size_t off, count;
	u_int16_t val;
	int i;

	off = startrow * scr->type->ncols;
	count = nrows * scr->type->ncols;

	val = (scr->vc_at << 8) | ' ';

	if (scr == vc->active)
		bus_space_set_region_2(memt, memh, off * 2, val, count);
	else
		for (i = 0; i < count; i++)
			scr->mem[off + i] = val;
}
