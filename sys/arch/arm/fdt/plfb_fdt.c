/* $NetBSD: plfb_fdt.c,v 1.2.6.2 2017/12/03 11:35:52 jdolecek Exp $ */

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
 * ARM PrimeCell PL111 framebuffer console driver
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: plfb_fdt.c,v 1.2.6.2 2017/12/03 11:35:52 jdolecek Exp $");

#include <sys/param.h>
#include <sys/types.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/bus.h>
#include <sys/kmem.h>
#include <sys/sysctl.h>

#include <dev/wsfb/genfbvar.h>

#include <dev/fdt/fdtvar.h>
#include <dev/fdt/display_timing.h>

#define	LCDTIMING0		0x000
#define	 LCDTIMING0_HBP		__BITS(31,24)
#define	 LCDTIMING0_HFP		__BITS(23,16)
#define	 LCDTIMING0_HSW		__BITS(15,8)
#define	 LCDTIMING0_PPL		__BITS(7,2)
#define	LCDTIMING1		0x004
#define	 LCDTIMING1_VBP		__BITS(31,24)
#define	 LCDTIMING1_VFP		__BITS(23,16)
#define	 LCDTIMING1_VSW		__BITS(15,10)
#define	 LCDTIMING1_LPP		__BITS(9,0)
#define	LCDUPBASE		0x010
#define	LCDLPBASE		0x014
#define	LCDCONTROL		0x018
#define	 LCDCONTROL_PWR		__BIT(11)
#define	 LCDCONTROL_BGR		__BIT(8)
#define	 LCDCONTROL_BPP		__BITS(3,1)
#define	  LCDCONTROL_BPP_24	__SHIFTIN(5, LCDCONTROL_BPP)
#define	 LCDCONTROL_EN		__BIT(0)

#define	PLFB_BPP		32

static int plfb_console_phandle = -1;

struct plfb_softc {
	struct genfb_softc	sc_gen;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	int			sc_phandle;

	bus_space_handle_t	sc_vram_bsh;
	bus_addr_t		sc_vram_addr;
	bus_size_t		sc_vram_size;
	uintptr_t		sc_vram;

	uint32_t		sc_wstype;
};

static int	plfb_match(device_t, cfdata_t, void *);
static void	plfb_attach(device_t, device_t, void *);

static int	plfb_ioctl(void *, void *, u_long, void *, int, lwp_t *);
static paddr_t	plfb_mmap(void *, void *, off_t, int);
static bool	plfb_shutdown(device_t, int);

static void	plfb_init(struct plfb_softc *);

static const char * const compatible[] = {
	"arm,pl111",
	NULL
};

CFATTACH_DECL_NEW(plfb_fdt, sizeof(struct plfb_softc),
    plfb_match, plfb_attach, NULL, NULL);

#define	FB_READ(sc, reg)	\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define	FB_WRITE(sc, reg, val)	\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static int
plfb_match(device_t parent, cfdata_t match, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
plfb_attach(device_t parent, device_t self, void *aux)
{
	struct plfb_softc *sc = device_private(self);
	prop_dictionary_t dict = device_properties(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	struct genfb_ops ops;
	struct clk *clk;
	bus_addr_t addr;
	bus_size_t size;

	sc->sc_gen.sc_dev = self;
	sc->sc_phandle = phandle;
	sc->sc_bst = faa->faa_bst;

	/* Enable clocks */
	for (int i = 0; (clk = fdtbus_clock_get_index(phandle, i)); i++)
		if (clk_enable(clk) != 0) {
			aprint_error(": couldn't enable clock #%d\n", i);
			return;
		}

	/* Map CLCD registers */
	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": missing 'reg' property\n");
		return;
	}
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh)) {
		aprint_error(": couldn't map device\n");
		return;
	}

	/* Map VRAM */
	const int vram_phandle = fdtbus_get_phandle(phandle, "memory-region");
	if (vram_phandle == -1) {
		/*
		 * The 'memory-region' property is optional. If
		 * absent, we can allocate FB from main RAM. (TODO)
		 */
		aprint_error(": missing 'memory-region' property\n");
		return;
	}
	if (fdtbus_get_reg(vram_phandle, 0, &sc->sc_vram_addr,
	    &sc->sc_vram_size) != 0) {
		aprint_error(": missing 'reg' property on memory-region\n");
		return;
	}
	if (bus_space_map(sc->sc_bst, sc->sc_vram_addr, sc->sc_vram_size,
	    BUS_SPACE_MAP_LINEAR, &sc->sc_vram_bsh)) {
		aprint_error(": couldn't map vram\n");
		return;
	}
	sc->sc_vram = (uintptr_t)bus_space_vaddr(sc->sc_bst, sc->sc_vram_bsh);

	plfb_init(sc);

	sc->sc_wstype = WSDISPLAY_TYPE_PLFB;
	prop_dictionary_set_bool(dict, "is_console",
	    phandle == plfb_console_phandle);

	genfb_init(&sc->sc_gen);

	if (sc->sc_gen.sc_width == 0 ||
	    sc->sc_gen.sc_fbsize == 0) {
		aprint_normal(": disabled\n");
		return;
	}

	pmf_device_register1(self, NULL, NULL, plfb_shutdown);

	memset(&ops, 0, sizeof(ops));
	ops.genfb_ioctl = plfb_ioctl;
	ops.genfb_mmap = plfb_mmap;

	aprint_naive("\n");
	aprint_normal("\n");

	genfb_attach(&sc->sc_gen, &ops);
}

static int
plfb_ioctl(void *v, void *vs, u_long cmd, void *data, int flag, lwp_t *l)
{
	struct plfb_softc *sc = v;
	struct wsdisplayio_bus_id *busid;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = sc->sc_wstype;
		return 0;
	case WSDISPLAYIO_GET_BUSID:
		busid = data;
		busid->bus_type = WSDISPLAYIO_BUS_SOC;
		return 0;
	case WSDISPLAYIO_GET_FBINFO:
		{
			struct wsdisplayio_fbinfo *fbi = data;
			struct rasops_info *ri = &sc->sc_gen.vd.active->scr_ri;

			return wsdisplayio_get_fbinfo(ri, fbi);
		}
	default:
		return EPASSTHROUGH;
	}
}

static paddr_t
plfb_mmap(void *v, void *vs, off_t offset, int prot)
{
	struct plfb_softc *sc = v;

	if (offset < 0 || offset >= sc->sc_vram_size)
		return -1;

	return bus_space_mmap(sc->sc_bst, sc->sc_vram_addr, offset, prot,
	    BUS_SPACE_MAP_LINEAR | BUS_SPACE_MAP_PREFETCHABLE);
}

static bool
plfb_shutdown(device_t self, int flags)
{
	genfb_enable_polling(self);
	return true;
}

static int
plfb_get_panel_timing(struct plfb_softc *sc, struct display_timing *timing)
{
	int panel, panel_timing;

	panel = of_find_firstchild_byname(sc->sc_phandle, "panel");
	if (panel <= 0)
		return ENOENT;
	panel_timing = of_find_firstchild_byname(panel, "panel-timing");
	if (panel_timing <= 0)
		return ENOENT;

	return display_timing_parse(panel_timing, timing);
}

static void
plfb_init(struct plfb_softc *sc)
{
	prop_dictionary_t dict = device_properties(sc->sc_gen.sc_dev);
	struct display_timing timing;

	if (plfb_get_panel_timing(sc, &timing) != 0) {
		aprint_error_dev(sc->sc_gen.sc_dev,
		    "couldn't get panel timings\n");
		return;
	}

	prop_dictionary_set_uint32(dict, "width", timing.hactive);
	prop_dictionary_set_uint32(dict, "height", timing.vactive);
	prop_dictionary_set_uint8(dict, "depth", PLFB_BPP);
	prop_dictionary_set_bool(dict, "dblscan", 0);
	prop_dictionary_set_bool(dict, "interlace", 0);
	prop_dictionary_set_uint16(dict, "linebytes", timing.hactive * (PLFB_BPP / 8));
	prop_dictionary_set_uint32(dict, "address", sc->sc_vram_addr);
	prop_dictionary_set_uint32(dict, "virtual_address", sc->sc_vram);

	/* FB base address */
	FB_WRITE(sc, LCDUPBASE, sc->sc_vram_addr);
	FB_WRITE(sc, LCDLPBASE, 0);

	/* CRTC timings */
	FB_WRITE(sc, LCDTIMING0,
	    __SHIFTIN(timing.hback_porch - 1, LCDTIMING0_HBP) |
	    __SHIFTIN(timing.hfront_porch - 1, LCDTIMING0_HFP) |
	    __SHIFTIN(timing.hsync_len - 1, LCDTIMING0_HSW) |
	    __SHIFTIN((timing.hactive / 16) - 1, LCDTIMING0_PPL));
	FB_WRITE(sc, LCDTIMING1,
	    __SHIFTIN(timing.vback_porch - 1, LCDTIMING1_VBP) |
	    __SHIFTIN(timing.vfront_porch - 1, LCDTIMING1_VFP) |
	    __SHIFTIN(timing.vsync_len - 1, LCDTIMING1_VSW) |
	    __SHIFTIN(timing.vactive - 1, LCDTIMING1_LPP));

	/* Configure and enable CLCD */
	FB_WRITE(sc, LCDCONTROL,
	    LCDCONTROL_PWR | LCDCONTROL_EN | LCDCONTROL_BPP_24 |
	    LCDCONTROL_BGR);
}

static int
plfb_console_match(int phandle)
{
	return of_match_compatible(phandle, compatible);
}

static void
plfb_console_consinit(struct fdt_attach_args *faa, u_int uart_freq)
{
	plfb_console_phandle = faa->faa_phandle;
	genfb_cnattach();
}

static const struct fdt_console plfb_fdt_console = {
	.match = plfb_console_match,
	.consinit = plfb_console_consinit
};

FDT_CONSOLE(plfb, &plfb_fdt_console);
