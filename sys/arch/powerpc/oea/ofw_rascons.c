/*	$NetBSD: ofw_rascons.c,v 1.11 2018/03/02 14:37:18 macallan Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: ofw_rascons.c,v 1.11 2018/03/02 14:37:18 macallan Exp $");

#include "wsdisplay.h"

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/bus.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <dev/ofw/openfirm.h>
#include <uvm/uvm_extern.h>

#include <machine/autoconf.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/rasops/rasops.h>
#include <dev/wscons/wsdisplay_vconsvar.h>
#include <dev/wsfont/wsfont.h>

#include <powerpc/oea/bat.h>
#include <powerpc/oea/cpufeat.h>
#include <powerpc/oea/ofw_rasconsvar.h>

/* we need a wsdisplay to do anything halfway useful */
#if NWSDISPLAY > 0

static int copy_rom_font(void);
static struct wsdisplay_font openfirm6x11;
static vaddr_t fbaddr;
static int romfont_loaded = 0;
static int needs_finalize = 0;

struct vcons_screen rascons_console_screen;

struct wsscreen_descr rascons_stdscreen = {
	"std",
	0, 0,	/* will be filled in -- XXX shouldn't, it's global */
	0,
	0, 0,
	WSSCREEN_REVERSE
};

int
rascons_cnattach(void)
{
	struct rasops_info *ri = &rascons_console_screen.scr_ri;
	long defattr;
	int crow = 0;

	/* get current cursor position */
	OF_interpret("line#", 0, 1, &crow);

	/* move (rom monitor) cursor to the lowest line - 1 */
	OF_interpret("#lines 2 - to line#", 0, 0);

	wsfont_init();
	if (copy_rom_font() == 0) {
#if !defined(OFWOEA_WSCONS_NO_ROM_FONT)
		romfont_loaded = 1;
#endif /* !OFWOEA_WSCONS_NO_ROM_FONT */
	}

	/* set up rasops */
	rascons_init_rasops(console_node, ri);

	/*
	 * no need to clear the screen here when we're mimicing firmware
	 * output anyway
	 */
#if 0
	if (ri->ri_width >= 1024 && ri->ri_height >= 768) {
		int i, screenbytes = ri->ri_stride * ri->ri_height;

		for (i = 0; i < screenbytes; i += sizeof(u_int32_t))
			*(u_int32_t *)(fbaddr + i) = 0xffffffff;
		crow = 0;
	}
#endif

	rascons_stdscreen.nrows = ri->ri_rows;
	rascons_stdscreen.ncols = ri->ri_cols;
	rascons_stdscreen.textops = &ri->ri_ops;
	rascons_stdscreen.capabilities = ri->ri_caps;

	if ((oeacpufeat & OEACPU_64_BRIDGE) != 0) {
		needs_finalize = 1;
	} else {
		ri->ri_ops.allocattr(ri, 0, 0, 0, &defattr);
		wsdisplay_preattach(&rascons_stdscreen, ri, 0, max(0,
		    min(crow, ri->ri_rows - 1)), defattr);
	}
#if notyet
	rascons_init_cmap(NULL);
#endif

	return 0;
}

void
rascons_finalize(void)
{
	struct rasops_info *ri = &rascons_console_screen.scr_ri;
	long defattr;

	if (needs_finalize == 0) return;
	
	ri->ri_ops.allocattr(ri, 0, 0, 0, &defattr);
	wsdisplay_preattach(&rascons_stdscreen, ri, 0, 0, defattr);
}

static int
copy_rom_font(void)
{
	u_char *romfont;
	int char_width, char_height;
	int chosen, mmu, m, e;

	/* Get ROM FONT address. */
	OF_interpret("font-adr", 0, 1, &romfont);
	if (romfont == NULL)
		return -1;

	chosen = OF_finddevice("/chosen");
	OF_getprop(chosen, "mmu", &mmu, 4);

	/*
	 * Convert to physcal address.  We cannot access to Open Firmware's
	 * virtual address space.
	 */
	OF_call_method("translate", mmu, 1, 3, romfont, &romfont, &m, &e);

	/* Get character size */
	OF_interpret("char-width", 0, 1, &char_width);
	OF_interpret("char-height", 0, 1, &char_height);

	openfirm6x11.name = "Open Firmware";
	openfirm6x11.firstchar = 32;
	openfirm6x11.numchars = 96;
	openfirm6x11.encoding = WSDISPLAY_FONTENC_ISO;
	openfirm6x11.fontwidth = char_width;
	openfirm6x11.fontheight = char_height;
	openfirm6x11.stride = 1;
	openfirm6x11.bitorder = WSDISPLAY_FONTORDER_L2R;
	openfirm6x11.byteorder = WSDISPLAY_FONTORDER_L2R;
	openfirm6x11.data = romfont;

	return 0;
}

int
rascons_init_rasops(int node, struct rasops_info *ri)
{
	int32_t width, height, linebytes, depth;

	/* XXX /chaos/control doesn't have "width", "height", ... */
	width = height = -1;
	if (OF_getprop(node, "width", &width, 4) != 4)
		OF_interpret("screen-width", 0, 1, &width);
	if (OF_getprop(node, "height", &height, 4) != 4)
		OF_interpret("screen-height", 0, 1, &height);
	if (OF_getprop(node, "linebytes", &linebytes, 4) != 4)
		linebytes = width;			/* XXX */
	if (OF_getprop(node, "depth", &depth, 4) != 4)
		depth = 8;				/* XXX */
	if (OF_getprop(node, "address", &fbaddr, 4) != 4)
		OF_interpret("frame-buffer-adr", 0, 1, &fbaddr);

	if (width == -1 || height == -1 || fbaddr == 0 || fbaddr == -1)
		return false;

	/* initialize rasops */
	ri->ri_width = width;
	ri->ri_height = height;
	ri->ri_depth = depth;
	ri->ri_stride = linebytes;
	ri->ri_bits = (char *)fbaddr;
	ri->ri_flg = RI_CENTER | RI_FULLCLEAR | RI_NO_AUTO;

	/* mimic firmware output if we can find the ROM font */
	if (romfont_loaded) {
		int cols, rows;

		/*
		 * XXX this assumes we're the console which may or may not
		 * be the case
		 */
		OF_interpret("#lines", 0, 1, &rows);
		OF_interpret("#columns", 0, 1, &cols);
		ri->ri_font = &openfirm6x11;
		ri->ri_wsfcookie = -1;		/* not using wsfont */
		rasops_init(ri, rows, cols);

		ri->ri_xorigin = (width - cols * ri->ri_font->fontwidth) >> 1;
		ri->ri_yorigin = (height - rows * ri->ri_font->fontheight)
		    >> 1;
		ri->ri_bits = (char *)fbaddr + ri->ri_xorigin +
			      ri->ri_stride * ri->ri_yorigin;
	} else {
		/* use as much of the screen as the font permits */
		rasops_init(ri, height/8, width/8);
		ri->ri_caps = WSSCREEN_WSCOLORS;
		rasops_reconfig(ri, height / ri->ri_font->fontheight,
		    width / ri->ri_font->fontwidth);
	}

	return true;
}
#else	/* NWSDISPLAY > 0 */
int
rascons_cnattach(void)
{
	return -1;
}
#endif
