/*	$NetBSD: gdium_genfb.c,v 1.1 2009/08/06 00:50:26 matt Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: gdium_genfb.c,v 1.1 2009/08/06 00:50:26 matt Exp $");

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/systm.h>

#include <machine/bus.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/rasops/rasops.h>
#include <dev/wsfont/wsfont.h>
#include <dev/wscons/wsdisplay_vconsvar.h>

#include <mips/bonito/bonitoreg.h>
#include <evbmips/gdium/gdiumvar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include "wsdisplay.h"

/* we need a wsdisplay to do anything halfway useful */
#if NWSDISPLAY > 0

static struct vcons_screen gdium_console_screen;

static struct wsscreen_descr gdium_stdscreen = {
	.name = "std",
};

int
gdium_cnattach(struct gdium_config *gc)
{
	struct rasops_info * const ri = &gdium_console_screen.scr_ri;
	long defattr;
	pcireg_t reg;
	
	wsfont_init();
	
	/* set up rasops */
	ri->ri_width = 1024;
	ri->ri_height = 600;
	ri->ri_depth = 16;
	ri->ri_stride = 0x800;
#if 1
	reg = 0x04000000;
#else
	/* read the mapping register for the frame buffer */
	reg = pci_conf_read(&gc->gc_pc, (14 << 11), PCI_MAPREG_START);
#endif
	ri->ri_bits = (char *)MIPS_PHYS_TO_KSEG1(BONITO_PCILO_BASE + reg);
	ri->ri_flg = RI_CENTER | RI_FULLCLEAR;

	/* use as much of the screen as the font permits */
	rasops_init(ri, 30, 80);

	rasops_reconfig(ri, ri->ri_height / ri->ri_font->fontheight,
	    ri->ri_width / ri->ri_font->fontwidth);

	gdium_stdscreen.nrows = ri->ri_rows;
	gdium_stdscreen.ncols = ri->ri_cols;
	gdium_stdscreen.textops = &ri->ri_ops;
	gdium_stdscreen.capabilities = ri->ri_caps;

	ri->ri_ops.allocattr(ri, 0, 0, 0, &defattr);

	wsdisplay_preattach(&gdium_stdscreen, ri, 0, 0, defattr);
	
	return 0;
}
#else	/* NWSDISPLAY > 0 */
int
gdium_cnattach(void)
{
	return -1;
}
#endif
