/*	$NetBSD: vga.c,v 1.1 1996/11/19 04:38:32 cgd Exp $	*/

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
#include <machine/bus.h>

#include <alpha/wscons/wsconsvar.h>
#include <alpha/common/vgavar.h>

#define	VGA_6845_ADDR	0x24
#define	VGA_6845_DATA	0x25

struct cfdriver vga_cd = {
	NULL, "vga", DV_DULL,
};

static void	vga_cursor __P((void *, int, int, int));
static void	vga_putstr __P((void *, int, int, char *, int));
static void	vga_copycols __P((void *, int, int, int,int));
static void	vga_erasecols __P((void *, int, int, int));
static void	vga_copyrows __P((void *, int, int, int));
static void	vga_eraserows __P((void *, int, int));

struct wscons_emulfuncs vga_emulfuncs = {
	vga_cursor,
	vga_putstr,
	vga_copycols,
	vga_erasecols,
	vga_copyrows,
	vga_eraserows,
};

static int	vgaprint __P((void *, const char *));
static int	vgaioctl __P((void *, u_long, caddr_t, int, struct proc *));
static int	vgammap __P((void *, off_t, int));

/*
 * The following functions implement back-end configuration grabbing
 * and attachment.
 */
void
vga_getconfig(vc)
	struct vga_config *vc;
{
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	int cpos;

	iot = vc->vc_iot;
	ioh = vc->vc_ioh;

	vc->vc_nrow = 25;
	vc->vc_ncol = 80;

	bus_space_write_1(iot, ioh, VGA_6845_ADDR, 14); 
	cpos = bus_space_read_1(iot, ioh, VGA_6845_DATA) << 8;
	bus_space_write_1(iot, ioh, VGA_6845_ADDR, 15);
	cpos |= bus_space_read_1(iot, ioh, VGA_6845_DATA);
	vc->vc_crow = cpos / vc->vc_ncol;
	vc->vc_ccol = cpos % vc->vc_ncol;

	vc->vc_so = 0;
#if 0
	vc->vc_at = 0x00 | 0xf;			/* black bg|white fg */
	vc->vc_so_at = 0x00 | 0xf | 0x80;	/* black bg|white fg|blink */

	/* clear screen, frob cursor, etc.? */
	pcivga_eraserows(vc, 0, vc->vc_nrow);
#endif
	/*
	 * XXX DEC HAS SWITCHED THE CODES FOR BLUE AND RED!!!
	 * XXX Therefore, though the comments say "blue bg", the code uses
	 * XXX the value for a red background!
	 */
	vc->vc_at = 0x40 | 0x0f;		/* blue bg|white fg */
	vc->vc_so_at = 0x40 | 0x0f | 0x80;	/* blue bg|white fg|blink */
}

void
vga_wscons_attach(parent, vc, console)
	struct device *parent;
	struct vga_config *vc;
	int console;
{
	struct wscons_attach_args waa;
	struct wscons_odev_spec *wo;

        waa.waa_isconsole = console;
        wo = &waa.waa_odev_spec;

        wo->wo_emulfuncs = &vga_emulfuncs;
	wo->wo_emulfuncs_cookie = vc;

        wo->wo_ioctl = vgaioctl;
        wo->wo_mmap = vgammap;
        wo->wo_miscfuncs_cookie = vc;

        wo->wo_nrows = vc->vc_nrow;
        wo->wo_ncols = vc->vc_ncol;
        wo->wo_crow = vc->vc_crow;
        wo->wo_ccol = vc->vc_ccol;
 
        config_found(parent, &waa, vgaprint);
}

void
vga_wscons_console(vc)
	struct vga_config *vc;
{
	struct wscons_odev_spec wo;

        wo.wo_emulfuncs = &vga_emulfuncs;
	wo.wo_emulfuncs_cookie = vc;

	/* ioctl and mmap are unused until real attachment. */

        wo.wo_nrows = vc->vc_nrow;
        wo.wo_ncols = vc->vc_ncol;
        wo.wo_crow = vc->vc_crow;
        wo.wo_ccol = vc->vc_ccol;
 
        wscons_attach_console(&wo);
}

static int
vgaprint(aux, pnp)
	void *aux;
	const char *pnp;
{

	if (pnp)
		printf("wscons at %s", pnp);
	return (UNCONF);
}

static int
vgaioctl(v, cmd, data, flag, p)
	void *v;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{

	/* XXX */
	return -1;
}

static int
vgammap(v, offset, prot)
	void *v;
	off_t offset;
	int prot;
{

	/* XXX */
	return -1;
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
	struct vga_config *vc = id;
	bus_space_tag_t iot = vc->vc_iot;
	bus_space_handle_t ioh = vc->vc_ioh;
	int pos;

#if 0
	printf("vga_cursor: %d %d\n", row, col);
#endif
	/* turn the cursor off */
	if (!on) {
		/* XXX disable cursor how??? */
		vc->vc_crow = vc->vc_ccol = -1;
	} else {
		vc->vc_crow = row;
		vc->vc_ccol = col;
	}

	pos = row * vc->vc_ncol + col;

	bus_space_write_1(iot, ioh, VGA_6845_ADDR, 14);
	bus_space_write_1(iot, ioh, VGA_6845_DATA, pos >> 8);
	bus_space_write_1(iot, ioh, VGA_6845_ADDR, 15);
	bus_space_write_1(iot, ioh, VGA_6845_DATA, pos);
}

static void
vga_putstr(id, row, col, cp, len)
	void *id;
	int row, col;
	char *cp;
	int len;
{
	struct vga_config *vc = id;
	bus_space_tag_t memt = vc->vc_memt;
	bus_space_handle_t memh = vc->vc_memh;
	int i, off;

	off = (row * vc->vc_ncol + col) * 2;
	for (i = 0; i < len; i++, cp++, off += 2) {
		bus_space_write_1(memt, memh, off, *cp);
		bus_space_write_1(memt, memh, off + 1,
		    vc->vc_so ? vc->vc_so_at : vc->vc_at);
	}
}

static void
vga_copycols(id, row, srccol, dstcol, ncols)
	void *id;
	int row, srccol, dstcol, ncols;
{
	struct vga_config *vc = id;
	bus_space_tag_t memt = vc->vc_memt;
	bus_space_handle_t memh = vc->vc_memh;
	bus_size_t srcoff, srcend, dstoff;

	/*
	 * YUCK.  Need bus copy functions.
	 */
	srcoff = (row * vc->vc_ncol + srccol) * 2;
	srcend = srcoff + ncols * 2;
	dstoff = (row * vc->vc_ncol + dstcol) * 2;

	for (; srcoff < srcend; srcoff += 2, dstoff += 2)
		bus_space_write_2(memt, memh, dstoff,
		    bus_space_read_2(memt, memh, srcoff));
}

static void
vga_erasecols(id, row, startcol, ncols)
	void *id;
	int row, startcol, ncols;
{
	struct vga_config *vc = id;
	bus_space_tag_t memt = vc->vc_memt;
	bus_space_handle_t memh = vc->vc_memh;
	bus_size_t off, endoff;
	u_int16_t val;

	/*
	 * YUCK.  Need bus 'set' functions.
	 */
	off = (row * vc->vc_ncol + startcol) * 2;
	endoff = off + ncols * 2;
	val = (vc->vc_at << 8) | ' ';

	for (; off < endoff; off += 2)
		bus_space_write_2(memt, memh, off, val);
}

static void
vga_copyrows(id, srcrow, dstrow, nrows)
	void *id;
	int srcrow, dstrow, nrows;
{
	struct vga_config *vc = id;
	bus_space_tag_t memt = vc->vc_memt;
	bus_space_handle_t memh = vc->vc_memh;
	bus_size_t srcoff, srcend, dstoff;

	/*
	 * YUCK.  Need bus copy functions.
	 */
	srcoff = (srcrow * vc->vc_ncol + 0) * 2;
	srcend = srcoff + (nrows * vc->vc_ncol * 2);
	dstoff = (dstrow * vc->vc_ncol + 0) * 2;

	for (; srcoff < srcend; srcoff += 2, dstoff += 2)
		bus_space_write_2(memt, memh, dstoff,
		    bus_space_read_2(memt, memh, srcoff));
}

static void
vga_eraserows(id, startrow, nrows)
	void *id;
	int startrow, nrows;
{
	struct vga_config *vc = id;
	bus_space_tag_t memt = vc->vc_memt;
	bus_space_handle_t memh = vc->vc_memh;
	bus_size_t off, endoff;
	u_int16_t val;

	/*
	 * YUCK.  Need bus 'set' functions.
	 */
	off = (startrow * vc->vc_ncol + 0) * 2;
	endoff = off + (nrows * vc->vc_ncol) * 2;
	val = (vc->vc_at << 8) | ' ';

	for (; off < endoff; off += 2)
		bus_space_write_2(memt, memh, off, val);
}
