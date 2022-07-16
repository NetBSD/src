/* $NetBSD: genfb_machdep.c,v 1.17 2022/07/16 06:27:24 mlelstv Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: genfb_machdep.c,v 1.17 2022/07/16 06:27:24 mlelstv Exp $");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/lwp.h>

#include <sys/bus.h>
#include <machine/bootinfo.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/rasops/rasops.h>
#include <dev/wsfont/wsfont.h>
#include <dev/wscons/wsdisplay_vconsvar.h>

#include <dev/wsfb/genfbvar.h>
#include <arch/x86/include/genfb_machdep.h>

#include "wsdisplay.h"
#include "genfb.h"
#include "acpica.h"

#if NWSDISPLAY > 0 && NGENFB > 0
struct vcons_screen x86_genfb_console_screen;
bool x86_genfb_use_shadowfb = true;

#if NACPICA > 0
extern int acpi_md_vesa_modenum;
#endif

static device_t x86_genfb_console_dev = NULL;

static struct wsscreen_descr x86_genfb_stdscreen = {
	"std",
	0, 0,
	0,
	0, 0,
	0,
	NULL
};

void
x86_genfb_set_console_dev(device_t dev)
{
	KASSERT(x86_genfb_console_dev == NULL);
	x86_genfb_console_dev = dev;
}

void
x86_genfb_ddb_trap_callback(int where)
{
	if (x86_genfb_console_dev == NULL)
		return;

	if (where) {
		genfb_enable_polling(x86_genfb_console_dev);
	} else {
		genfb_disable_polling(x86_genfb_console_dev);
	}
}

int
x86_genfb_init(void)
{
	static int inited, attached;
	struct rasops_info *ri = &x86_genfb_console_screen.scr_ri;
	const struct btinfo_framebuffer *fbinfo;
	bus_space_tag_t t = x86_bus_space_mem;
	bus_space_handle_t h;
	void *bits;
	int err;

	if (inited)
		return attached;
	inited = 1;

	memset(&x86_genfb_console_screen, 0, sizeof(x86_genfb_console_screen));

	fbinfo = lookup_bootinfo(BTINFO_FRAMEBUFFER);
	if (fbinfo == NULL || fbinfo->physaddr == 0)
		return 0;

	err = _x86_memio_map(t, (bus_addr_t)fbinfo->physaddr,
	    fbinfo->height * fbinfo->stride,
	    BUS_SPACE_MAP_LINEAR | BUS_SPACE_MAP_PREFETCHABLE, &h);
	if (err) {
		aprint_error("x86_genfb_cnattach: couldn't map framebuffer\n");
		return 0;
	}

	bits = bus_space_vaddr(t, h);
	if (bits == NULL) {
		aprint_error("x86_genfb_cnattach: couldn't get fb vaddr\n");
		return 0;
	}

#if NACPICA > 0
	acpi_md_vesa_modenum = fbinfo->vbemode;
#endif

	wsfont_init();

	ri->ri_width = fbinfo->width;
	ri->ri_height = fbinfo->height;
	ri->ri_depth = fbinfo->depth;
	ri->ri_stride = fbinfo->stride;
	ri->ri_rnum = fbinfo->rnum;
	ri->ri_gnum = fbinfo->gnum;
	ri->ri_bnum = fbinfo->bnum;
	ri->ri_rpos = fbinfo->rpos;
	ri->ri_gpos = fbinfo->gpos;
	ri->ri_bpos = fbinfo->bpos;
	if (x86_genfb_use_shadowfb && lookup_bootinfo(BTINFO_EFI) != NULL) {
		/* XXX The allocated memory is never released... */
		ri->ri_bits = kmem_alloc(ri->ri_stride * ri->ri_height,
		    KM_SLEEP);
		ri->ri_hwbits = bits;
	} else
		ri->ri_bits = bits;
	ri->ri_flg = RI_CENTER | RI_FULLCLEAR | RI_CLEAR;
	rasops_init(ri, ri->ri_height / 8, ri->ri_width / 8);
	ri->ri_caps = WSSCREEN_WSCOLORS;
	rasops_reconfig(ri, ri->ri_height / ri->ri_font->fontheight,
	    ri->ri_width / ri->ri_font->fontwidth);

	x86_genfb_stdscreen.nrows = ri->ri_rows;
	x86_genfb_stdscreen.ncols = ri->ri_cols;
	x86_genfb_stdscreen.textops = &ri->ri_ops;
	x86_genfb_stdscreen.capabilities = ri->ri_caps;

	attached = 1;
	return 1;
}

int
x86_genfb_cnattach(void)
{
	static int ncalls = 0;
	struct rasops_info *ri = &x86_genfb_console_screen.scr_ri;
	long defattr;

	/* XXX jmcneill
	 *  Defer console initialization until UVM is initialized
	 */
	++ncalls;
	if (ncalls < 3)
		return -1;

	if (!x86_genfb_init())
		return 0;

	ri->ri_ops.allocattr(ri, 0, 0, 0, &defattr);
	wsdisplay_preattach(&x86_genfb_stdscreen, ri, 0, 0, defattr);

	return 1;
}
#else	/* NWSDISPLAY > 0 && NGENFB > 0 */
int
x86_genfb_init(void)
{
	return 0;
}

int
x86_genfb_cnattach(void)
{
	return 0;
}
#endif
