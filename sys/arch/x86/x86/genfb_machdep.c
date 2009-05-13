/* $NetBSD: genfb_machdep.c,v 1.2.6.2 2009/05/13 17:18:45 jym Exp $ */

/*-
 * Copyright (c) 2009 Jared D. McNeill <jmcneill@invisible.ca>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
 * Early attach support for raster consoles
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: genfb_machdep.c,v 1.2.6.2 2009/05/13 17:18:45 jym Exp $");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>

#include <machine/bus.h>
#include <machine/bootinfo.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/rasops/rasops.h>
#include <dev/wsfont/wsfont.h>
#include <dev/wscons/wsdisplay_vconsvar.h>

#include <arch/x86/include/genfb_machdep.h>

#include "wsdisplay.h"
#include "genfb.h"

#if NWSDISPLAY > 0 && NGENFB > 0
struct vcons_screen x86_genfb_console_screen;

static struct wsscreen_descr x86_genfb_stdscreen = {
	"std",
	0, 0,
	0,
	0, 0,
	0,
	NULL
};

int
x86_genfb_cnattach(void)
{
	static int ncalls = 0;
	struct rasops_info *ri = &x86_genfb_console_screen.scr_ri;
	const struct btinfo_framebuffer *fbinfo;
	bus_space_tag_t t = X86_BUS_SPACE_MEM;
	bus_space_handle_t h;
	void *bits;
	long defattr;
	int err;

	/* XXX jmcneill
	 *  Defer console initialization until UVM is initialized
	 */
	++ncalls;
	if (ncalls < 3)
		return -1;

	memset(&x86_genfb_console_screen, 0, sizeof(x86_genfb_console_screen));

	fbinfo = lookup_bootinfo(BTINFO_FRAMEBUFFER);
	if (fbinfo == NULL || fbinfo->physaddr == 0)
		return 0;

	err = _x86_memio_map(t, (bus_addr_t)fbinfo->physaddr,
	    fbinfo->width * fbinfo->stride,
	    BUS_SPACE_MAP_LINEAR, &h);
	if (err) {
		aprint_error("x86_genfb_cnattach: couldn't map framebuffer\n");
		return 0;
	}

	bits = bus_space_vaddr(t, h);
	if (bits == NULL) {
		aprint_error("x86_genfb_cnattach: couldn't get fb vaddr\n");
		return 0;
	}

	wsfont_init();

	ri->ri_width = fbinfo->width;
	ri->ri_height = fbinfo->height;
	ri->ri_depth = fbinfo->depth;
	ri->ri_stride = fbinfo->stride;
	ri->ri_bits = bits;
	ri->ri_flg = RI_CENTER | RI_FULLCLEAR | RI_CLEAR;
	rasops_init(ri, ri->ri_width / 8, ri->ri_height / 8);
	ri->ri_caps = WSSCREEN_WSCOLORS;
	rasops_reconfig(ri, ri->ri_height / ri->ri_font->fontheight,
	    ri->ri_width / ri->ri_font->fontwidth);

	x86_genfb_stdscreen.nrows = ri->ri_rows;
	x86_genfb_stdscreen.ncols = ri->ri_cols;
	x86_genfb_stdscreen.textops = &ri->ri_ops;
	x86_genfb_stdscreen.capabilities = ri->ri_caps;

	ri->ri_ops.allocattr(ri, 0, 0, 0, &defattr);
	wsdisplay_preattach(&x86_genfb_stdscreen, ri, 0, 0, defattr);

	return 1;
}
#else	/* NWSDISPLAY > 0 && NGENFB > 0 */
int
x86_genfb_cnattach(void)
{
	return 0;
}
#endif
