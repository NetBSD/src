/* $NetBSD: pcdisplay_subr.c,v 1.1 1998/05/28 16:48:40 drochner Exp $ */

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

#include <dev/isa/isavar.h>
#include <dev/isa/isareg.h>

#include <dev/ic/mc6845reg.h>
#include <dev/ic/pcdisplayvar.h>

#include <dev/wscons/wsdisplayvar.h>

void
pcdisplay_cursor(id, on, row, col)
	void *id;
	int on, row, col;
{
	struct pcdisplayscreen *scr = id;
	int pos;

#if 0
	printf("pcdisplay_cursor: %d %d\n", row, col);
#endif
	scr->vc_crow = row;
	scr->vc_ccol = col;
	scr->cursoron = on;

	if (scr->active) {
		if (!on) {
			/* XXX disable cursor how??? */
			row = col = -1;
		}

		pos = row * scr->type->ncols + col;

		pcdisplay_6845_write(scr->hdl, cursorh, pos >> 8);
		pcdisplay_6845_write(scr->hdl, cursorl, pos);
	}
}

void
pcdisplay_putstr(id, row, col, cp, len, attr)
	void *id;
	int row, col;
	char *cp;
	int len;
	long attr;
{
	struct pcdisplayscreen *scr = id;
	bus_space_tag_t memt = scr->hdl->ph_memt;
	bus_space_handle_t memh = scr->hdl->ph_memh;
	int i, off;

	off = row * scr->type->ncols + col;

	if (scr->active) {
		off *= 2;

		for (i = 0; i < len; i++, cp++, off += 2)
			bus_space_write_2(memt, memh, off, (attr << 8) | *cp);
	} else {
		u_int16_t *m = &scr->mem[off];

		for (i = 0; i < len; i++, cp++)
			*m++ = *cp | (attr << 8);
	}
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
		bus_space_copy_region_2(memt, memh, srcoff * 2,
					memh, dstoff * 2, ncols);
	else
		bcopy(&scr->mem[srcoff], &scr->mem[dstoff], ncols * 2);
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
		bus_space_set_region_2(memt, memh, off * 2, val, ncols);
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
		bus_space_copy_region_2(memt, memh, srcoff * 2,
					memh, dstoff * 2, nrows * ncols);
	else
		bcopy(&scr->mem[srcoff], &scr->mem[dstoff],
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
		bus_space_set_region_2(memt, memh, off * 2, val, count);
	else
		for (i = 0; i < count; i++)
			scr->mem[off + i] = val;
}
