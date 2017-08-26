/* $NetBSD: sunxi_debe.c,v 1.1 2017/08/26 22:24:50 jmcneill Exp $ */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sunxi_debe.c,v 1.1 2017/08/26 22:24:50 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <dev/fdt/fdtvar.h>

#include <dev/wsfb/genfbvar.h>

#define	SUNXI_SDRAM_BASE		0x40000000

#define	DEBE_MAX_LAYER			4

#define	DEBE_MODCTL_REG			0x800
#define	 DEBE_MODCTL_LAYn_EN(n)		__BIT(8 + (n))
#define	 DEBE_MODCTL_EN			__BIT(0)
#define	DEBE_LAYSIZE_REG(n)		(0x810 + (n) * 0x10)
#define	 DEBE_LAYSIZE_HEIGHT		__BITS(28,16)
#define	 DEBE_LAYSIZE_WIDTH		__BITS(12,0)
#define	DEBE_LAYCOOR_REG(n)		(0x820 + (n) * 0x4)
#define	DEBE_LAYLINEWIDTH_REG(n)	(0x840 + (n) * 0x4)
#define	DEBE_L32ADD_REG(n)		(0x850 + (n) * 0x4)
#define	DEBE_H4ADD_REG			0x860
#define	 DEBE_H4ADD(n)			__BITS((n) * 8 + 3, (n) * 8)

static const char * const compatible[] = {
	"allwinner,sun5i-a13-display-backend",
	NULL
};

struct sunxi_debe_softc {
	struct genfb_softc sc_gen;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	int sc_phandle;

	bus_space_handle_t sc_fb_bsh;
	bus_addr_t sc_fb_paddr;
};

#define BE_READ(sc, reg) \
    bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define BE_WRITE(sc, reg, val) \
    bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static bool
sunxi_debe_shutdown(device_t self, int flags)
{
	genfb_enable_polling(self);
	return true;
}

static int
sunxi_debe_ioctl(void *v, void *vs, u_long cmd, void *data, int flag, lwp_t *l)
{
	struct sunxi_debe_softc * const sc = v;
	struct wsdisplayio_bus_id *busid;
	struct wsdisplayio_fbinfo *fbi;
	struct rasops_info *ri;
	int error;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = WSDISPLAY_TYPE_ALLWINNER;
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
sunxi_debe_mmap(void *v, void *vs, off_t off, int prot)
{
	struct sunxi_debe_softc * const sc = v;

	if (off < 0 || off >= sc->sc_gen.sc_fbsize)
		return -1;

	return bus_space_mmap(sc->sc_bst, sc->sc_fb_paddr, off, prot,
	    BUS_SPACE_MAP_LINEAR | BUS_SPACE_MAP_PREFETCHABLE);
}

static void
sunxi_debe_attach_layer(struct sunxi_debe_softc *sc, u_int lay)
{
	prop_dictionary_t dict = device_properties(sc->sc_gen.sc_dev);
	struct genfb_ops ops;

	const uint32_t size = BE_READ(sc, DEBE_LAYSIZE_REG(lay));
	const uint32_t linewidth = BE_READ(sc, DEBE_LAYLINEWIDTH_REG(lay));
	const uint32_t l32add = BE_READ(sc, DEBE_L32ADD_REG(lay));
	const uint32_t h4add = BE_READ(sc, DEBE_H4ADD_REG);

	const int width = __SHIFTOUT(size, DEBE_LAYSIZE_WIDTH) + 1;
	const int height = __SHIFTOUT(size, DEBE_LAYSIZE_HEIGHT) + 1;
	const int stride = linewidth / NBBY;
	const uint64_t fbaddr_bits =
	    l32add | (__SHIFTOUT(h4add, DEBE_H4ADD(lay)) << 32);
	const uint32_t fbaddr = SUNXI_SDRAM_BASE +
	    (uint32_t)(fbaddr_bits / NBBY);
	const int fbsize = height * stride;

	aprint_normal_dev(sc->sc_gen.sc_dev,
	    "attaching %dx%d fb to layer %d @ 0x%08x size 0x%x\n",
	    width, height, lay, fbaddr, fbsize);

	if (bus_space_map(sc->sc_bst, fbaddr, fbsize,
	    BUS_SPACE_MAP_LINEAR, &sc->sc_fb_bsh) != 0) {
		aprint_error_dev(sc->sc_gen.sc_dev, "failed to map fb\n");
		return;
	}

	sc->sc_fb_paddr = fbaddr;

	prop_dictionary_set_uint32(dict, "width", width);
	prop_dictionary_set_uint32(dict, "height", height);
	prop_dictionary_set_uint8(dict, "depth", 32);
	prop_dictionary_set_uint16(dict, "linebytes", stride);
	prop_dictionary_set_uint32(dict, "address", fbaddr);
	prop_dictionary_set_uint32(dict, "virtual_address",
	    (uintptr_t)bus_space_vaddr(sc->sc_bst, sc->sc_fb_bsh));

	genfb_init(&sc->sc_gen);

	if (sc->sc_gen.sc_width == 0 || sc->sc_gen.sc_fbsize == 0) {
		aprint_normal(": disabled\n");
		return;
	}

	pmf_device_register1(sc->sc_gen.sc_dev, NULL, NULL,
	    sunxi_debe_shutdown);

	memset(&ops, 0, sizeof(ops));
	ops.genfb_ioctl = sunxi_debe_ioctl;
	ops.genfb_mmap = sunxi_debe_mmap;

	bool is_console = false;
	prop_dictionary_get_bool(dict, "is_console", &is_console);
	prop_dictionary_set_bool(dict, "is_console", is_console);

	if (is_console)
		aprint_normal_dev(sc->sc_gen.sc_dev,
		    "switching to framebuffer console\n");

	genfb_attach(&sc->sc_gen, &ops);
}

static int
sunxi_debe_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
sunxi_debe_attach(device_t parent, device_t self, void *aux)
{
	struct sunxi_debe_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	struct fdtbus_reset *rst;
	struct clk *clk;
	bus_addr_t addr;
	bus_size_t size;
	u_int lay;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	if ((clk = fdtbus_clock_get(phandle, "ahb")) == NULL ||
	    clk_enable(clk) != 0) {
		aprint_error(": couldn't enable ahb clock\n");
		return;
	}
	if ((rst = fdtbus_reset_get_index(phandle, 0)) == NULL ||
	    fdtbus_reset_deassert(rst) != 0) {
		aprint_error(": couldn't de-assert reset\n");
		return;
	}

	sc->sc_gen.sc_dev = self;
	sc->sc_phandle = phandle;
	sc->sc_bst = faa->faa_bst;
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	aprint_naive("\n");
	aprint_normal(": Display Backend\n");

	aprint_normal_dev(self, "boot config:");
	const uint32_t modctl = BE_READ(sc, DEBE_MODCTL_REG);
	aprint_normal(" %08x\n", modctl);

	if ((modctl & DEBE_MODCTL_EN) == 0) {
		aprint_normal_dev(self, "not enabled by firmware\n");
		return;
	}

	for (lay = 0; lay < DEBE_MAX_LAYER; lay++) {
		if ((modctl & DEBE_MODCTL_LAYn_EN(lay)) == 0)
			continue;
		sunxi_debe_attach_layer(sc, lay);
		break;
	}
	if (lay == DEBE_MAX_LAYER) {
		aprint_error_dev(self, "no layers enabled\n");
		return;
	}
}

CFATTACH_DECL_NEW(sunxi_debe, sizeof(struct sunxi_debe_softc),
	sunxi_debe_match, sunxi_debe_attach, NULL, NULL);
