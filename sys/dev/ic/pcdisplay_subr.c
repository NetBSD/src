/* $NetBSD: pcdisplay_subr.c,v 1.19 2001/09/04 17:14:49 drochner Exp $ */

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
#include <sys/device.h>
#include <machine/bus.h>

#include <dev/ic/mc6845reg.h>
#include <dev/ic/pcdisplayvar.h>

#include <dev/wscons/wsdisplayvar.h>

void
pcdisplay_cursor_init(scr, existing)
	struct pcdisplayscreen *scr;
	int existing;
{
#ifdef PCDISPLAY_SOFTCURSOR
	bus_space_tag_t memt;
	bus_space_handle_t memh;
	int off;

	pcdisplay_6845_write(scr->hdl, curstart, 0x10);
	pcdisplay_6845_write(scr->hdl, curend, 0x10);
	
	if (existing) {
		/*
		 * This is the first screen. At this point, scr->mem is NULL
		 * (no backing store), so we can't use pcdisplay_cursor() to
		 * do this.
		 */
		memt = scr->hdl->ph_memt;
		memh = scr->hdl->ph_memh;
		off = (scr->vc_crow * scr->type->ncols + scr->vc_ccol) * 2 +
		    scr->dispoffset;

		scr->cursortmp = bus_space_read_2(memt, memh, off);
		bus_space_write_2(memt, memh, off, scr->cursortmp ^ 0x7700);
	} else
		scr->cursortmp = 0;
#else
	/*
	 * Firmware might not have initialized the cursor shape.  Make
	 * sure there's something we can see.
	 * Don't touch the hardware if this is not the first screen.
	 */
	if (existing) {
		pcdisplay_6845_write(scr->hdl, curstart,
				     scr->type->fontheight - 2);
		pcdisplay_6845_write(scr->hdl, curend,
				     scr->type->fontheight - 1);
	}
#endif
	scr->cursoron = 1;
}

void
pcdisplay_cursor(id, on, row, col)
	void *id;
	int on, row, col;
{
#ifdef PCDISPLAY_SOFTCURSOR
	struct pcdisplayscreen *scr = id;
	bus_space_tag_t memt = scr->hdl->ph_memt;
	bus_space_handle_t memh = scr->hdl->ph_memh;
	int off;

	/* Remove old cursor image */
	if (scr->cursoron) {
		off = scr->vc_crow * scr->type->ncols + scr->vc_ccol;
		if (scr->active)
			bus_space_write_2(memt, memh, scr->dispoffset + off * 2,
			    scr->cursortmp);
		else
			scr->mem[off] = scr->cursortmp;
	}
		
	scr->vc_crow = row;
	scr->vc_ccol = col;

	if ((scr->cursoron = on) == 0)
		return;

	off = (scr->vc_crow * scr->type->ncols + scr->vc_ccol);
	if (scr->active) {
		off = off * 2 + scr->dispoffset;
		scr->cursortmp = bus_space_read_2(memt, memh, off);
		bus_space_write_2(memt, memh, off, scr->cursortmp ^ 0x7700);
	} else {
		scr->cursortmp = scr->mem[off];
		scr->mem[off] = scr->cursortmp ^ 0x7700;
	}
#else 	/* PCDISPLAY_SOFTCURSOR */
	struct pcdisplayscreen *scr = id;
	int pos;

	scr->vc_crow = row;
	scr->vc_ccol = col;
	scr->cursoron = on;

	if (scr->active) {
		if (!on)
			pos = 0x3fff;
		else
			pos = scr->dispoffset / 2
				+ row * scr->type->ncols + col;

		pcdisplay_6845_write(scr->hdl, cursorh, pos >> 8);
		pcdisplay_6845_write(scr->hdl, cursorl, pos);
	}
#endif	/* PCDISPLAY_SOFTCURSOR */
}

#if 0
unsigned int
pcdisplay_mapchar_simple(id, uni)
	void *id;
	int uni;
{
	if (uni < 128)
		return (uni);

	return (1); /* XXX ??? smiley */
}
#endif

void
pcdisplay_putchar(id, row, col, c, attr)
	void *id;
	int row, col;
	u_int c;
	long attr;
{
	struct pcdisplayscreen *scr = id;
	bus_space_tag_t memt = scr->hdl->ph_memt;
	bus_space_handle_t memh = scr->hdl->ph_memh;
	int off;

	off = row * scr->type->ncols + col;

	if (scr->active)
		bus_space_write_2(memt, memh, scr->dispoffset + off * 2,
				  c | (attr << 8));
	else
		scr->mem[off] = c | (attr << 8);
}

void
pcdisplay_copycols(id, row, srccol, dstcol, ncols)
	void *id;
	int row, srccol, dstcol, ncols;
{
	struct pcdisplayscreen *scr = id;
	bus_space_tag_t memt = scr->hdl->ph_memt;
	bus_space_handle_t memh = scr->hdl->ph_memh;
	bus_size_t srcoff, dstoff;

	srcoff = dstoff = row * scr->type->ncols;
	srcoff += srccol;
	dstoff += dstcol;

	if (scr->active)
		bus_space_copy_region_2(memt, memh,
					scr->dispoffset + srcoff * 2,
					memh, scr->dispoffset + dstoff * 2,
					ncols);
	else
		memcpy(&scr->mem[dstoff], &scr->mem[srcoff], ncols * 2);
}

void
pcdisplay_erasecols(id, row, startcol, ncols, fillattr)
	void *id;
	int row, startcol, ncols;
	long fillattr;
{
	struct pcdisplayscreen *scr = id;
	bus_space_tag_t memt = scr->hdl->ph_memt;
	bus_space_handle_t memh = scr->hdl->ph_memh;
	bus_size_t off;
	u_int16_t val;
	int i;

	off = row * scr->type->ncols + startcol;

	val = (fillattr << 8) | ' ';

	if (scr->active)
		bus_space_set_region_2(memt, memh, scr->dispoffset + off * 2,
				       val, ncols);
	else
		for (i = 0; i < ncols; i++)
			scr->mem[off + i] = val;
}

void
pcdisplay_copyrows(id, srcrow, dstrow, nrows)
	void *id;
	int srcrow, dstrow, nrows;
{
	struct pcdisplayscreen *scr = id;
	bus_space_tag_t memt = scr->hdl->ph_memt;
	bus_space_handle_t memh = scr->hdl->ph_memh;
	int ncols = scr->type->ncols;
	bus_size_t srcoff, dstoff;

	srcoff = srcrow * ncols + 0;
	dstoff = dstrow * ncols + 0;

	if (scr->active)
		bus_space_copy_region_2(memt, memh,
					scr->dispoffset + srcoff * 2,
					memh, scr->dispoffset + dstoff * 2,
					nrows * ncols);
	else
		memcpy(&scr->mem[dstoff], &scr->mem[srcoff],
		      nrows * ncols * 2);
}

void
pcdisplay_eraserows(id, startrow, nrows, fillattr)
	void *id;
	int startrow, nrows;
	long fillattr;
{
	struct pcdisplayscreen *scr = id;
	bus_space_tag_t memt = scr->hdl->ph_memt;
	bus_space_handle_t memh = scr->hdl->ph_memh;
	bus_size_t off, count;
	u_int16_t val;
	int i;

	off = startrow * scr->type->ncols;
	count = nrows * scr->type->ncols;

	val = (fillattr << 8) | ' ';

	if (scr->active)
		bus_space_set_region_2(memt, memh, scr->dispoffset + off * 2,
				       val, count);
	else
		for (i = 0; i < count; i++)
			scr->mem[off + i] = val;
}
