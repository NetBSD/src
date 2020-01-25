/* $NetBSD: acpi_simplefb.c,v 1.1.4.1 2020/01/25 22:38:37 ad Exp $ */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: acpi_simplefb.c,v 1.1.4.1 2020/01/25 22:38:37 ad Exp $");

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

#include <arm/acpi/acpi_simplefb.h>

#include <libfdt.h>

extern struct bus_space arm_generic_bs_tag;

static struct acpi_simplefb_softc {
	uint32_t	sc_width;
	uint32_t	sc_height;
	uint32_t	sc_stride;
	uint16_t	sc_depth;
	void		*sc_bits;
} acpi_simplefb_softc;

static struct wsscreen_descr acpi_simplefb_stdscreen = {
	.name = "std",
	.ncols = 0,
	.nrows = 0,
	.textops = NULL,
	.fontwidth = 0,
	.fontheight = 0,
	.capabilities = 0,
	.modecookie = NULL
};

static struct wsdisplay_accessops acpi_simplefb_accessops;
static struct vcons_data acpi_simplefb_vcons_data;
static struct vcons_screen acpi_simplefb_screen;

static int
acpi_simplefb_find_node(void)
{
	static const char * simplefb_compatible[] = { "simple-framebuffer", NULL };
	int chosen_phandle, child;

	chosen_phandle = OF_finddevice("/chosen");
	if (chosen_phandle == -1)
		return -1;

	for (child = OF_child(chosen_phandle); child; child = OF_peer(child)) {
		if (!fdtbus_status_okay(child))
			continue;
		if (!of_match_compatible(child, simplefb_compatible))
			continue;

		return child;
	}

	return -1;
}

static void
acpi_simplefb_init_screen(void *cookie, struct vcons_screen *scr,
    int existing, long *defattr)
{
	struct acpi_simplefb_softc * const sc = cookie;
	struct rasops_info *ri = &scr->scr_ri;

	ri->ri_width = sc->sc_width;
	ri->ri_height = sc->sc_height;
	ri->ri_depth = sc->sc_depth;
	ri->ri_stride = sc->sc_stride;
	ri->ri_bits = sc->sc_bits;
	ri->ri_flg = RI_CENTER | RI_FULLCLEAR | RI_CLEAR;

	scr->scr_flags |= VCONS_LOADFONT;
	scr->scr_flags |= VCONS_DONT_READ;

	rasops_init(ri, ri->ri_height / 8, ri->ri_width / 8);
	ri->ri_caps = WSSCREEN_WSCOLORS | WSSCREEN_HILIT | WSSCREEN_UNDERLINE;
	rasops_reconfig(ri, sc->sc_height / ri->ri_font->fontheight,
	    sc->sc_width / ri->ri_font->fontwidth);

	ri->ri_hw = scr;
}

static int
acpi_simplefb_ioctl(void *v, void *vs, u_long cmd, void *data, int flag, lwp_t *l)
{
	return EPASSTHROUGH;
}

static paddr_t
acpi_simplefb_mmap(void *v, void *vs, off_t offset, int prot)
{
	return -1;
}

static void
acpi_simplefb_pollc(void *v, int on)
{
}

void
acpi_simplefb_preattach(void)
{
	struct acpi_simplefb_softc * const sc = &acpi_simplefb_softc;
	struct rasops_info *ri = &acpi_simplefb_screen.scr_ri;
	bus_space_tag_t bst = &arm_generic_bs_tag;
	bus_space_handle_t bsh;
	uint32_t width, height, stride;
	const char *format;
	bus_addr_t addr;
	bus_size_t size;
	uint16_t depth;
	long defattr;

	const int phandle = acpi_simplefb_find_node();
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
	} else if (strcmp(format, "r5g6b5") == 0) {
		depth = 16;
	} else {
		return;
	}

	if (bus_space_map(bst, addr, size,
	    BUS_SPACE_MAP_LINEAR | BUS_SPACE_MAP_PREFETCHABLE, &bsh) != 0)
		return;

	sc->sc_width = width;
	sc->sc_height = height;
	sc->sc_depth = depth;
	sc->sc_stride = stride;
	sc->sc_bits = bus_space_vaddr(bst, bsh);

	wsfont_init();

	acpi_simplefb_accessops.ioctl = acpi_simplefb_ioctl;
	acpi_simplefb_accessops.mmap = acpi_simplefb_mmap;
	acpi_simplefb_accessops.pollc = acpi_simplefb_pollc;

	vcons_init(&acpi_simplefb_vcons_data, sc, &acpi_simplefb_stdscreen,
		&acpi_simplefb_accessops);
	acpi_simplefb_vcons_data.init_screen = acpi_simplefb_init_screen;
	acpi_simplefb_vcons_data.use_intr = 0;
	vcons_init_screen(&acpi_simplefb_vcons_data, &acpi_simplefb_screen, 1, &defattr);
	acpi_simplefb_screen.scr_flags |= VCONS_SCREEN_IS_STATIC;    

	if (ri->ri_rows < 1 || ri->ri_cols < 1)
		return;

	acpi_simplefb_stdscreen.nrows = ri->ri_rows;
	acpi_simplefb_stdscreen.ncols = ri->ri_cols;
	acpi_simplefb_stdscreen.textops = &ri->ri_ops;
	acpi_simplefb_stdscreen.capabilities = ri->ri_caps;

	wsdisplay_preattach(&acpi_simplefb_stdscreen, ri, 0, 0, defattr);

	vcons_replay_msgbuf(&acpi_simplefb_screen);
}
