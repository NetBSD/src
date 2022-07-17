/* $NetBSD: arm_simplefb.c,v 1.12 2022/07/17 20:23:17 riastradh Exp $ */

/*-
 * Copyright (c) 2019 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jared McNeill <jmcneill@invisible.ca>.
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

#include "pci.h"
#include "opt_pci.h"
#include "opt_vcons.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: arm_simplefb.c,v 1.12 2022/07/17 20:23:17 riastradh Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>

#include <dev/fdt/fdtvar.h>
#include <arm/fdt/arm_fdtvar.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/rasops/rasops.h>
#include <dev/wsfont/wsfont.h>
#include <dev/wscons/wsdisplay_vconsvar.h>

#if NPCI > 0 && defined(PCI_NETBSD_CONFIGURE)
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pciconf.h>
#endif

#include <arm/fdt/arm_simplefb.h>

#include <libfdt.h>

extern struct bus_space arm_generic_bs_tag;

static struct arm_simplefb_softc {
	uint32_t	sc_width;
	uint32_t	sc_height;
	uint32_t	sc_stride;
	uint16_t	sc_depth;
	bool		sc_swapped;
	bool		sc_10bit;
	void		*sc_bits;
} arm_simplefb_softc;

static struct wsscreen_descr arm_simplefb_stdscreen = {
	.name = "std",
	.ncols = 0,
	.nrows = 0,
	.textops = NULL,
	.fontwidth = 0,
	.fontheight = 0,
	.capabilities = 0,
	.modecookie = NULL
};

static struct wsdisplay_accessops arm_simplefb_accessops;
static struct vcons_data arm_simplefb_vcons_data;
static struct vcons_screen arm_simplefb_screen;

static bus_addr_t arm_simplefb_addr;
static bus_size_t arm_simplefb_size;
static bus_space_handle_t arm_simplefb_bsh;

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "simple-framebuffer" },
	DEVICE_COMPAT_EOL
};

static int
arm_simplefb_find_node(void)
{
	int chosen_phandle, child;

	chosen_phandle = OF_finddevice("/chosen");
	if (chosen_phandle == -1)
		return -1;

	for (child = OF_child(chosen_phandle); child; child = OF_peer(child)) {
		if (!fdtbus_status_okay(child))
			continue;
		if (!of_compatible_match(child, compat_data))
			continue;

		return child;
	}

	return -1;
}

static void
arm_simplefb_init_screen(void *cookie, struct vcons_screen *scr,
    int existing, long *defattr)
{
	struct arm_simplefb_softc * const sc = cookie;
	struct rasops_info *ri = &scr->scr_ri;

	ri->ri_width = sc->sc_width;
	ri->ri_height = sc->sc_height;
	ri->ri_depth = sc->sc_depth;
	ri->ri_stride = sc->sc_stride;
	ri->ri_bits = sc->sc_bits;
	ri->ri_flg = RI_CENTER | RI_FULLCLEAR | RI_CLEAR;

	if (sc->sc_swapped) {
		KASSERT(ri->ri_depth == 32);
		ri->ri_rnum = ri->ri_gnum = ri->ri_bnum = 8;
		ri->ri_rpos =  8;
		ri->ri_gpos = 16;
		ri->ri_bpos = 24;
	} else if (sc->sc_10bit) {
		KASSERT(ri->ri_depth == 32);
		ri->ri_rnum = ri->ri_gnum = ri->ri_bnum = 10;
		ri->ri_rpos = 20;
		ri->ri_gpos = 10;
		ri->ri_bpos =  0;
	}

	scr->scr_flags |= VCONS_LOADFONT;
	scr->scr_flags |= VCONS_DONT_READ;

	rasops_init(ri, ri->ri_height / 8, ri->ri_width / 8);
	ri->ri_caps = WSSCREEN_WSCOLORS | WSSCREEN_HILIT | WSSCREEN_UNDERLINE;
	rasops_reconfig(ri, sc->sc_height / ri->ri_font->fontheight,
	    sc->sc_width / ri->ri_font->fontwidth);

	ri->ri_hw = scr;
}

static int
arm_simplefb_ioctl(void *v, void *vs, u_long cmd, void *data, int flag,
    lwp_t *l)
{
	return EPASSTHROUGH;
}

static paddr_t
arm_simplefb_mmap(void *v, void *vs, off_t offset, int prot)
{
	return -1;
}

static void
arm_simplefb_pollc(void *v, int on)
{
}

#if NPCI > 0 && defined(PCI_NETBSD_CONFIGURE)
static void
arm_simplefb_reconfig(void *arg, uint64_t new_addr)
{
	struct arm_simplefb_softc * const sc = &arm_simplefb_softc;
	struct rasops_info *ri = &arm_simplefb_screen.scr_ri;
	bus_space_tag_t bst = &arm_generic_bs_tag;

	bus_space_unmap(bst, arm_simplefb_bsh, arm_simplefb_size);
	bus_space_map(bst, new_addr, arm_simplefb_size,
	    BUS_SPACE_MAP_LINEAR | BUS_SPACE_MAP_PREFETCHABLE,
	    &arm_simplefb_bsh);

	sc->sc_bits = bus_space_vaddr(bst, arm_simplefb_bsh);
	ri->ri_bits = sc->sc_bits;

	arm_simplefb_addr = (bus_addr_t)new_addr;
}
#endif

uint64_t
arm_simplefb_physaddr(void)
{
	return arm_simplefb_addr;
}

void
arm_simplefb_preattach(void)
{
	struct arm_simplefb_softc * const sc = &arm_simplefb_softc;
	struct rasops_info *ri = &arm_simplefb_screen.scr_ri;
	bus_space_tag_t bst = &arm_generic_bs_tag;
	bus_space_handle_t bsh;
	uint32_t width, height, stride;
	const char *format;
	bus_addr_t addr;
	bus_size_t size;
	uint16_t depth;
	long defattr;
	bool swapped = false;
	bool is_10bit = false;

	const int phandle = arm_simplefb_find_node();
	if (phandle == -1)
		return;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0 || size == 0)
		return;

	if (of_getprop_uint32(phandle, "width", &width) != 0 ||
	    of_getprop_uint32(phandle, "height", &height) != 0 ||
	    of_getprop_uint32(phandle, "stride", &stride) != 0 ||
	    (format = fdtbus_get_string(phandle, "format")) == NULL)
		return;

	if (width == 0 || height == 0)
		return;

	if (strcmp(format, "a8b8g8r8") == 0 ||
	    strcmp(format, "x8r8g8b8") == 0) {
		depth = 32;
	} else if (strcmp(format, "r8g8b8a8") == 0 ||
		   strcmp(format, "b8g8r8x8") == 0) {
		depth = 32;
		swapped = true;
	} else if (strcmp(format, "x2r10g10b10") == 0) {
		depth = 32;
		is_10bit = true;
	} else if (strcmp(format, "r5g6b5") == 0) {
		depth = 16;
	} else {
		return;
	}

	/*
	 * Make sure that the size of the linear FB mapping is big enough
	 * to fit the requested screen dimensions.
	 */
	if (size < stride * height) {
		return;
	}

	if (bus_space_map(bst, addr, size,
	    BUS_SPACE_MAP_LINEAR | BUS_SPACE_MAP_PREFETCHABLE, &bsh) != 0)
		return;

	arm_simplefb_addr = addr;
	arm_simplefb_size = size;
	arm_simplefb_bsh = bsh;

	sc->sc_width = width;
	sc->sc_height = height;
	sc->sc_depth = depth;
	sc->sc_stride = stride;
	sc->sc_bits = bus_space_vaddr(bst, bsh);
	sc->sc_swapped = swapped;
	sc->sc_10bit = is_10bit;

	wsfont_init();

	arm_simplefb_accessops.ioctl = arm_simplefb_ioctl;
	arm_simplefb_accessops.mmap = arm_simplefb_mmap;
	arm_simplefb_accessops.pollc = arm_simplefb_pollc;

	vcons_earlyinit(&arm_simplefb_vcons_data, sc, &arm_simplefb_stdscreen,
	    &arm_simplefb_accessops);
	arm_simplefb_vcons_data.init_screen = arm_simplefb_init_screen;
	vcons_init_screen(&arm_simplefb_vcons_data, &arm_simplefb_screen, 1,
	    &defattr);
	arm_simplefb_screen.scr_flags |= VCONS_SCREEN_IS_STATIC;

	if (ri->ri_rows < 1 || ri->ri_cols < 1)
		return;

	arm_simplefb_stdscreen.nrows = ri->ri_rows;
	arm_simplefb_stdscreen.ncols = ri->ri_cols;
	arm_simplefb_stdscreen.textops = &ri->ri_ops;
	arm_simplefb_stdscreen.capabilities = ri->ri_caps;

	wsdisplay_preattach(&arm_simplefb_stdscreen, ri, 0, 0, defattr);

	vcons_replay_msgbuf(&arm_simplefb_screen);

#if NPCI > 0 && defined(PCI_NETBSD_CONFIGURE)
	/*
	 * Let the PCI resource allocator know about our framebuffer. This
	 * lets us know if the FB base address changes so we can remap the
	 * framebuffer if necessary.
	 */
	pciconf_resource_reserve(PCI_CONF_MAP_MEM, addr, size,
	    arm_simplefb_reconfig, NULL);
#endif
}
