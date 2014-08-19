/* $NetBSD: genfb_machdep.c,v 1.10.12.1 2014/08/20 00:03:29 tls Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: genfb_machdep.c,v 1.10.12.1 2014/08/20 00:03:29 tls Exp $");

#include "opt_mtrr.h"

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/lwp.h>

#include <sys/bus.h>
#include <machine/bootinfo.h>
#include <machine/mtrr.h>

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

void
x86_genfb_mtrr_init(uint64_t physaddr, uint32_t size)
{
#if notyet
#ifdef MTRR
	struct mtrr mtrr;
	int error, n;

	if (mtrr_funcs == NULL) {
		aprint_debug("%s: no mtrr funcs\n", __func__);
		return;
	}

	mtrr.base = physaddr;
	mtrr.len = size;
	mtrr.type = MTRR_TYPE_WC;
	mtrr.flags = MTRR_VALID;
	mtrr.owner = 0;

	aprint_debug("%s: 0x%" PRIx64 "-0x%" PRIx64 "\n", __func__,
	    mtrr.base, mtrr.base + mtrr.len - 1);

	n = 1;
	KERNEL_LOCK(1, NULL);
	error = mtrr_set(&mtrr, &n, curlwp->l_proc, MTRR_GETSET_KERNEL);
	if (n != 0)
		mtrr_commit();
	KERNEL_UNLOCK_ONE(NULL);

	aprint_debug("%s: mtrr_set returned %d\n", __func__, error);
#else
	aprint_debug("%s: kernel lacks MTRR option\n", __func__);
#endif
#endif
}

int
x86_genfb_cnattach(void)
{
	static int ncalls = 0;
	struct rasops_info *ri = &x86_genfb_console_screen.scr_ri;
	const struct btinfo_framebuffer *fbinfo;
	bus_space_tag_t t = x86_bus_space_mem;
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
