/* $NetBSD: pcdisplay_subr.c,v 1.4 1998/06/20 21:55:05 drochner Exp $ */

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

static u_char iso2ibm437[] =
{
           0,     0,     0,     0,     0,     0,     0,     0,
           0,     0,     0,     0,     0,     0,     0,     0,
           0,     0,     0,     0,     0,     0,     0,     0,
           0,     0,     0,     0,     0,     0,     0,     0,
        0xff,  0xad,  0x9b,  0x9c,     0,  0x9d,     0,  0x40,
        0x6f,  0x63,  0x61,  0xae,     0,     0,     0,     0,
        0xf8,  0xf1,  0xfd,  0x33,     0,  0xe6,     0,  0xfa,
           0,  0x31,  0x6f,  0xaf,  0xac,  0xab,     0,  0xa8,
        0x41,  0x41,  0x41,  0x41,  0x8e,  0x8f,  0x92,  0x80,
        0x45,  0x90,  0x45,  0x45,  0x49,  0x49,  0x49,  0x49,
        0x81,  0xa5,  0x4f,  0x4f,  0x4f,  0x4f,  0x99,  0x4f,
        0x4f,  0x55,  0x55,  0x55,  0x9a,  0x59,     0,  0xe1,
        0x85,  0xa0,  0x83,  0x61,  0x84,  0x86,  0x91,  0x87,
        0x8a,  0x82,  0x88,  0x89,  0x8d,  0xa1,  0x8c,  0x8b,
           0,  0xa4,  0x95,  0xa2,  0x93,  0x6f,  0x94,  0x6f,
        0x6f,  0x97,  0xa3,  0x96,  0x81,  0x98,     0,     0
};

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
	u_char dc;
	int off;

	if (c < 128)
		dc = c;
	else if (c < 256)
		dc = iso2ibm437[c - 128];
	else
		return;

	off = row * scr->type->ncols + col;

	if (scr->active)
		bus_space_write_2(memt, memh, off * 2,
				  dc | (attr << 8));
	else
		scr->mem[off] = dc | (attr << 8);
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
