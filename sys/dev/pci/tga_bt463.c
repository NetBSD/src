/* $NetBSD: tga_bt463.c,v 1.3 1999/03/24 05:51:21 mrg Exp $ */

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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
#include <sys/buf.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <vm/vm.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/tgareg.h>
#include <dev/pci/tgavar.h>
#include <dev/ic/bt463reg.h>

#include <dev/wscons/wsconsio.h>

/*
 * Functions exported via the RAMDAC configuration table.
 */
void	tga_bt463_init __P((struct tga_devconfig *, int));
int	tga_bt463_intr __P((void *));
int	tga_bt463_set_cmap __P((struct tga_devconfig *,
	    struct wsdisplay_cmap *));
int	tga_bt463_get_cmap __P((struct tga_devconfig *,
	    struct wsdisplay_cmap *));

int	tga_bt463_check_curcmap __P((struct tga_devconfig *,
	    struct wsdisplay_cursor *cursorp));
void	tga_bt463_set_curcmap __P((struct tga_devconfig *,
	    struct wsdisplay_cursor *cursorp));
int	tga_bt463_get_curcmap __P((struct tga_devconfig *,
	    struct wsdisplay_cursor *cursorp));

const struct tga_ramdac_conf tga_ramdac_bt463 = {
	"Bt463",
	tga_bt463_init,
	tga_bt463_intr,
	tga_bt463_set_cmap,
	tga_bt463_get_cmap,
	tga_builtin_set_cursor,
	tga_builtin_get_cursor,
	tga_builtin_set_curpos,
	tga_builtin_get_curpos,
	tga_builtin_get_curmax,
	tga_bt463_check_curcmap,
	tga_bt463_set_curcmap,
	tga_bt463_get_curcmap,
};

/*
 * Private data.
 */
struct bt463data {
	int	changed;			/* what changed; see below */
	char curcmap_r[2];			/* cursor colormap */
	char curcmap_g[2];
	char curcmap_b[2];
	char cmap_r[BT463_NCMAP_ENTRIES];	/* colormap */
	char cmap_g[BT463_NCMAP_ENTRIES];
	char cmap_b[BT463_NCMAP_ENTRIES];
};

#define	DATA_CURCMAP_CHANGED	0x01	/* cursor colormap changed */
#define	DATA_CMAP_CHANGED	0x02	/* colormap changed */
#define	DATA_ALL_CHANGED	0x03

/*
 * Internal functions.
 */
inline void	tga_bt463_wr_d __P((volatile tga_reg_t *, u_int, u_int8_t));
inline u_int8_t tga_bt463_rd_d __P((volatile tga_reg_t *, u_int));
inline void	tga_bt463_wraddr __P((volatile tga_reg_t *, u_int16_t));
void	tga_bt463_update __P((struct tga_devconfig *, struct bt463data *));

#define	tga_bt463_sched_update(dc)					\
    ((dc)->dc_regs[TGA_REG_SISR] = 0x00010000)			/* XXX */

/*****************************************************************************/

/*
 * Functions exported via the RAMDAC configuration table.
 */

void
tga_bt463_init(dc, alloc)
	struct tga_devconfig *dc;
	int alloc;
{
	struct bt463data tmp, *data;
	int i;

	/*
	 * Init the BT463 for normal operation.
	 */

	/*
	 * Select:
	 *
	 * Overlay mapping: mapped to common palette.
	 * 14 window type entries.
	 */
	tga_bt463_wraddr(dc->dc_regs, BT463_IREG_COMMAND_1);
	tga_bt463_wr_d(dc->dc_regs, BT463_REG_IREG_DATA, 0x48);

	/*
	 * Initialize the read mask.
	 */
	tga_bt463_wraddr(dc->dc_regs, BT463_IREG_READ_MASK_P0_P7);
	for (i = 0; i < 3; i++)
		tga_bt463_wr_d(dc->dc_regs, BT463_REG_IREG_DATA, 0xff);

	/*
	 * Initialize the blink mask.
	 */
	tga_bt463_wraddr(dc->dc_regs, BT463_IREG_BLINK_MASK_P0_P7);
	for (i = 0; i < 3; i++)
		tga_bt463_wr_d(dc->dc_regs, BT463_REG_IREG_DATA, 0);

	/*
	 * If we should allocate a new private info struct, do so.
	 * Otherwise, use the one we have (if it's there), or
	 * use the temporary one on the stack.
	 */
	if (alloc) {
		if (dc->dc_ramdac_private != NULL)
			panic("tga_bt463_init: already have private struct");
		dc->dc_ramdac_private = malloc(sizeof *data, M_DEVBUF,
		    M_WAITOK);
	}
	if (dc->dc_ramdac_private != NULL)
		data = dc->dc_ramdac_private;
	else
		data = &tmp;

	/*
	 * Initalize the RAMDAC info struct to hold all of our
	 * data, and fill it in.
	 */
	data->changed = DATA_ALL_CHANGED;

	/* initial cursor colormap: 0 is black, 1 is white */
	data->curcmap_r[0] = data->curcmap_g[0] = data->curcmap_b[0] = 0;
	data->curcmap_r[1] = data->curcmap_g[1] = data->curcmap_b[1] = 0xff;

	/* Initial colormap: 0 is black, everything else is white */
	data->cmap_r[0] = data->cmap_g[0] = data->cmap_b[0] = 0;
	for (i = 1; i < 256; i++)
		data->cmap_r[i] = data->cmap_g[i] = data->cmap_b[i] = 255;

	tga_bt463_update(dc, data);

	dc->dc_regs[TGA_REG_SISR] = 0x00000001;			/* XXX */
}

int
tga_bt463_set_cmap(dc, cmapp)
	struct tga_devconfig *dc;
	struct wsdisplay_cmap *cmapp;
{
	struct bt463data *data = dc->dc_ramdac_private;
	int count, index, s;

	if ((u_int)cmapp->index >= BT463_NCMAP_ENTRIES ||
	    ((u_int)cmapp->index + (u_int)cmapp->count) > BT463_NCMAP_ENTRIES)
		return (EINVAL);
	if (!uvm_useracc(cmapp->red, cmapp->count, B_READ) ||
	    !uvm_useracc(cmapp->green, cmapp->count, B_READ) ||
	    !uvm_useracc(cmapp->blue, cmapp->count, B_READ))
		return (EFAULT);

	s = spltty();

	index = cmapp->index;
	count = cmapp->count;
	copyin(cmapp->red, &data->cmap_r[index], count);
	copyin(cmapp->green, &data->cmap_g[index], count);
	copyin(cmapp->blue, &data->cmap_b[index], count);

	data->changed |= DATA_CMAP_CHANGED;

	tga_bt463_sched_update(dc);
	splx(s);

	return (0);
}

int
tga_bt463_get_cmap(dc, cmapp)
	struct tga_devconfig *dc;
	struct wsdisplay_cmap *cmapp;
{
	struct bt463data *data = dc->dc_ramdac_private;
	int error, count, index;

	if ((u_int)cmapp->index >= BT463_NCMAP_ENTRIES ||
	    ((u_int)cmapp->index + (u_int)cmapp->count) > BT463_NCMAP_ENTRIES)
		return (EINVAL);

	count = cmapp->count;
	index = cmapp->index;

	error = copyout(&data->cmap_r[index], cmapp->red, count);
	if (error)
		return (error);
	error = copyout(&data->cmap_g[index], cmapp->green, count);
	if (error)
		return (error);
	error = copyout(&data->cmap_b[index], cmapp->blue, count);
	return (error);
}

int
tga_bt463_check_curcmap(dc, cursorp)
	struct tga_devconfig *dc;
	struct wsdisplay_cursor *cursorp;
{
	int count;

	if ((u_int)cursorp->cmap.index > 2 ||
	    ((u_int)cursorp->cmap.index +
	     (u_int)cursorp->cmap.count) > 2)
		return (EINVAL);
	count = cursorp->cmap.count; 
	if (!uvm_useracc(cursorp->cmap.red, count, B_READ) ||
	    !uvm_useracc(cursorp->cmap.green, count, B_READ) ||
	    !uvm_useracc(cursorp->cmap.blue, count, B_READ))
		return (EFAULT);
	return (0);
}

void
tga_bt463_set_curcmap(dc, cursorp)
	struct tga_devconfig *dc;
	struct wsdisplay_cursor *cursorp;
{
	struct bt463data *data = dc->dc_ramdac_private;
	int count, index;

	/* can't fail; parameters have already been checked. */
	count = cursorp->cmap.count;
	index = cursorp->cmap.index;
	copyin(cursorp->cmap.red, &data->curcmap_r[index], count);
	copyin(cursorp->cmap.green, &data->curcmap_g[index], count);
	copyin(cursorp->cmap.blue, &data->curcmap_b[index], count);
	data->changed |= DATA_CURCMAP_CHANGED;
	tga_bt463_sched_update(dc);
}

int
tga_bt463_get_curcmap(dc, cursorp)
	struct tga_devconfig *dc;
	struct wsdisplay_cursor *cursorp;
{
	struct bt463data *data = dc->dc_ramdac_private;
	int error;

	cursorp->cmap.index = 0;	/* DOCMAP */
	cursorp->cmap.count = 2;
	if (cursorp->cmap.red != NULL) {
		error = copyout(data->curcmap_r, cursorp->cmap.red, 2);
		if (error)
			return (error);
	}
	if (cursorp->cmap.green != NULL) {
		error = copyout(data->curcmap_g, cursorp->cmap.green, 2);
		if (error)
			return (error);
	}
	if (cursorp->cmap.blue != NULL) {
		error = copyout(data->curcmap_b, cursorp->cmap.blue, 2);
		if (error)
			return (error);
	}
	return (0);
}

int
tga_bt463_intr(v)
	void *v;
{
	struct tga_devconfig *dc = v;

	if ((dc->dc_regs[TGA_REG_SISR] & 0x00010001) != 0x00010001)
		return 0;
	tga_bt463_update(dc, dc->dc_ramdac_private);
	dc->dc_regs[TGA_REG_SISR] = 0x00000001;
	return (1);
}

/*****************************************************************************/

/*
 * Internal functions.
 */

inline void
tga_bt463_wr_d(tgaregs, btreg, val)
	volatile tga_reg_t *tgaregs;
	u_int btreg;
	u_int8_t val;
{

	if (btreg > BT463_REG_MAX)
		panic("tga_bt463_wr_d: reg %d out of range\n", btreg);

	/* XXX */
	tgaregs[TGA_REG_EPDR] = (btreg << 10) | (0 << 9) || (0 << 8) | val;
#ifdef __alpha__
	alpha_mb();
#endif
}

inline u_int8_t
tga_bt463_rd_d(tgaregs, btreg)
	volatile tga_reg_t *tgaregs;
	u_int btreg;
{
	tga_reg_t rdval;

	if (btreg > BT463_REG_MAX)
		panic("tga_bt463_rd_d: reg %d out of range\n", btreg);

	tgaregs[TGA_REG_EPSR] = (btreg << 2) | (0x1 << 1) | 0;	/* XXX */
#ifdef __alpha__
	alpha_mb();
#endif

	rdval = tgaregs[TGA_REG_EPDR];
	return (rdval >> 16) & 0xff;				/* XXX */
}

inline void
tga_bt463_wraddr(tgaregs, ireg)
	volatile tga_reg_t *tgaregs;
	u_int16_t ireg;
{

	tga_bt463_wr_d(tgaregs, BT463_REG_ADDR_LOW, ireg & 0xff);
	tga_bt463_wr_d(tgaregs, BT463_REG_ADDR_HIGH, (ireg >> 8) & 0xff);
}

void
tga_bt463_update(dc, data)
	struct tga_devconfig *dc;
	struct bt463data *data;
{
	int i, v;

	v = data->changed;
	data->changed = 0;

	if (v & DATA_CURCMAP_CHANGED) {
		tga_bt463_wraddr(dc->dc_regs, BT463_IREG_CURSOR_COLOR_0);
		/* spit out the cursor data */
		for (i = 0; i < 2; i++) {
                	tga_bt463_wr_d(dc->dc_regs, BT463_REG_IREG_DATA,
			    data->curcmap_r[i]);
                	tga_bt463_wr_d(dc->dc_regs, BT463_REG_IREG_DATA,
			    data->curcmap_g[i]);
                	tga_bt463_wr_d(dc->dc_regs, BT463_REG_IREG_DATA,
			    data->curcmap_b[i]);
		}
	}

	if (v & DATA_CMAP_CHANGED) {
		tga_bt463_wraddr(dc->dc_regs, BT463_IREG_CPALETTE_RAM);
		/* spit out the colormap data */
		for (i = 0; i < BT463_NCMAP_ENTRIES; i++) {
                	tga_bt463_wr_d(dc->dc_regs, BT463_REG_CMAP_DATA,
			    data->cmap_r[i]);
                	tga_bt463_wr_d(dc->dc_regs, BT463_REG_CMAP_DATA,
			    data->cmap_g[i]);
                	tga_bt463_wr_d(dc->dc_regs, BT463_REG_CMAP_DATA,
			    data->cmap_b[i]);
		}
	}
}
