/* $NetBSD: simplefb.c,v 1.4 2018/04/01 04:35:05 ryo Exp $ */

/*-
 * Copyright (c) 2017 Jared McNeill <jmcneill@invisible.ca>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "opt_wsdisplay_compat.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: simplefb.c,v 1.4 2018/04/01 04:35:05 ryo Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <dev/fdt/fdtvar.h>

#include <dev/wsfb/genfbvar.h>

static const char * const compatible[] = {
	"simple-framebuffer",
	NULL
};

struct simplefb_softc {
	struct genfb_softc sc_gen;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	int sc_phandle;

	bus_addr_t sc_paddr;
};

static int simplefb_console_phandle = -1;

static bool
simplefb_shutdown(device_t self, int flags)
{
	genfb_enable_polling(self);
	return true;
}

static int
simplefb_ioctl(void *v, void *vs, u_long cmd, void *data, int flag, lwp_t *l)
{
	struct simplefb_softc * const sc = v;
	struct wsdisplayio_bus_id *busid;
	struct wsdisplayio_fbinfo *fbi;
	struct rasops_info *ri;
	int error;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = WSDISPLAY_TYPE_GENFB;
		return 0;
	case WSDISPLAYIO_GET_BUSID:
		busid = data;
		busid->bus_type = WSDISPLAYIO_BUS_SOC;
		return 0;
	case WSDISPLAYIO_GET_FBINFO:
		fbi = data;
		ri = &sc->sc_gen.vd.active->scr_ri;
		error = wsdisplayio_get_fbinfo(ri, fbi);
		if (error == 0)
			fbi->fbi_flags |= WSFB_VRAM_IS_RAM;
		return error;
	default:
		return EPASSTHROUGH;
	}
}

static paddr_t
simplefb_mmap(void *v, void *vs, off_t off, int prot)
{
	struct simplefb_softc * const sc = v;

	if (off < 0 || off >= sc->sc_gen.sc_fbsize)
		return -1;

	return bus_space_mmap(sc->sc_bst, sc->sc_paddr, off, prot,
	    BUS_SPACE_MAP_LINEAR | BUS_SPACE_MAP_PREFETCHABLE);
}

static int
simplefb_attach_genfb(struct simplefb_softc *sc)
{
	device_t dev = sc->sc_gen.sc_dev;
	prop_dictionary_t dict = device_properties(dev);
	const int phandle = sc->sc_phandle;
	struct genfb_ops ops;
	uint32_t width, height, stride;
	uint16_t depth;
	const char *format;
	bus_addr_t addr;
	bus_size_t size;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return ENXIO;
	}

	if (size == 0) {
		aprint_naive("\n");
		aprint_normal(": disabled\n");
		return ENXIO;
	}

	if (of_getprop_uint32(phandle, "width", &width) != 0 ||
	    of_getprop_uint32(phandle, "height", &height) != 0 ||
	    of_getprop_uint32(phandle, "stride", &stride) != 0 ||
	    (format = fdtbus_get_string(phandle, "format")) == NULL) {
		aprint_error(": missing property in DT\n");
		return ENXIO;
	}

	if (strcmp(format, "a8b8g8r8") == 0 ||
	    strcmp(format, "x8r8g8b8") == 0) {
		depth = 32;
	} else if (strcmp(format, "r5g6b5") == 0) {
		depth = 16;
	} else {
		aprint_error(": unsupported format '%s'\n", format);
		return ENXIO;
	}

	if (bus_space_map(sc->sc_bst, addr, size,
	    BUS_SPACE_MAP_LINEAR | BUS_SPACE_MAP_PREFETCHABLE,
	    &sc->sc_bsh) != 0) {
		aprint_error(": failed to map fb\n");
		return ENXIO;
	}

	sc->sc_paddr = addr;

	prop_dictionary_set_uint32(dict, "width", width);
	prop_dictionary_set_uint32(dict, "height", height);
	prop_dictionary_set_uint8(dict, "depth", depth);
	prop_dictionary_set_uint16(dict, "linebytes", stride);
	prop_dictionary_set_uint32(dict, "address", addr);
	prop_dictionary_set_uint64(dict, "virtual_address",
	    (uintptr_t)bus_space_vaddr(sc->sc_bst, sc->sc_bsh));

	genfb_init(&sc->sc_gen);

	if (sc->sc_gen.sc_width == 0 || sc->sc_gen.sc_fbsize == 0) {
		aprint_normal(": disabled\n");
		return ENXIO;
	}

	aprint_naive("\n");
	aprint_normal(": Simple Framebuffer (%ux%u %u-bpp @ 0x%" PRIxBUSADDR ")\n",
	    width, height, depth, addr);

	pmf_device_register1(dev, NULL, NULL, simplefb_shutdown);

	memset(&ops, 0, sizeof(ops));
	ops.genfb_ioctl = simplefb_ioctl;
	ops.genfb_mmap = simplefb_mmap;

#ifdef WSDISPLAY_MULTICONS
	const bool is_console = true;
#else
	const bool is_console = phandle == simplefb_console_phandle;
	if (is_console)
		aprint_normal_dev(sc->sc_gen.sc_dev,
		    "switching to framebuffer console\n");
#endif

	prop_dictionary_set_bool(dict, "is_console", is_console);

	genfb_attach(&sc->sc_gen, &ops);

	return 0;
}

static int
simplefb_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
simplefb_attach(device_t parent, device_t self, void *aux)
{
	struct simplefb_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;

	sc->sc_gen.sc_dev = self;
	sc->sc_phandle = phandle;
	sc->sc_bst = faa->faa_bst;

	if (simplefb_attach_genfb(sc) != 0)
		return;
}

CFATTACH_DECL_NEW(simplefb, sizeof(struct simplefb_softc),
	simplefb_match, simplefb_attach, NULL, NULL);

static int
simplefb_console_match(int phandle)
{
	return of_match_compatible(phandle, compatible);
}

static void
simplefb_console_consinit(struct fdt_attach_args *faa, u_int uart_freq)
{
	simplefb_console_phandle = faa->faa_phandle;
	genfb_cnattach();
}

static const struct fdt_console simplefb_fdt_console = {
	.match = simplefb_console_match,
	.consinit = simplefb_console_consinit
};

FDT_CONSOLE(simplefb, &simplefb_fdt_console);
