/* $NetBSD: tegra_dc.c,v 1.1.2.3 2015/09/22 12:05:38 skrll Exp $ */

/*-
 * Copyright (c) 2015 Jared D. McNeill <jmcneill@invisible.ca>
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

#include "locators.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tegra_dc.c,v 1.1.2.3 2015/09/22 12:05:38 skrll Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <dev/videomode/videomode.h>

#include <arm/nvidia/tegra_reg.h>
#include <arm/nvidia/tegra_dcreg.h>
#include <arm/nvidia/tegra_var.h>

#define TEGRA_DC_NPORTS		2
#define TEGRA_DC_DEPTH		32
#define TEGRA_DC_FBALIGN	PAGE_SIZE

static int	tegra_dc_match(device_t, cfdata_t, void *);
static void	tegra_dc_attach(device_t, device_t, void *);

struct tegra_dc_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	bus_dma_tag_t		sc_dmat;
	int			sc_port;

	bus_dma_segment_t	sc_dmasegs[1];
	bus_size_t		sc_dmasize;
	bus_dmamap_t		sc_dmamap;
	void			*sc_dmap;

	device_t		sc_fbdev;
};

static int	tegra_dc_print(void *, const char *);
static int	tegra_dc_allocmem(struct tegra_dc_softc *, bus_size_t);
static int	tegra_dc_init(struct tegra_dc_softc *,
			      const struct videomode *);
static void	tegra_dc_init_win(struct tegra_dc_softc *,
				  const struct videomode *);
static void	tegra_dc_init_disp(struct tegra_dc_softc *,
				   const struct videomode *);
static void	tegra_dc_update(struct tegra_dc_softc *);

CFATTACH_DECL_NEW(tegra_dc, sizeof(struct tegra_dc_softc),
	tegra_dc_match, tegra_dc_attach, NULL, NULL);

#define DC_READ(sc, reg)			\
    bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define DC_WRITE(sc, reg, val)			\
    bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))
#define DC_SET_CLEAR(sc, reg, set, clr)		\
    tegra_reg_set_clear((sc)->sc_bst, (sc)->sc_bsh, (reg), (set), (clr))

static int
tegra_dc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct tegraio_attach_args * const tio = aux;
	const struct tegra_locators * const loc = &tio->tio_loc;

	if (loc->loc_port == TEGRAIOCF_PORT_DEFAULT ||
	    loc->loc_port >= TEGRA_DC_NPORTS)
		return 0;

	return 1;
}

static void
tegra_dc_attach(device_t parent, device_t self, void *aux)
{
	struct tegra_dc_softc * const sc = device_private(self);
	struct tegraio_attach_args * const tio = aux;
	const struct tegra_locators * const loc = &tio->tio_loc;

	sc->sc_dev = self;
	sc->sc_port = loc->loc_port;
	sc->sc_bst = tio->tio_bst;
	sc->sc_dmat = tio->tio_dmat;
	if (bus_space_map(sc->sc_bst, TEGRA_GHOST_BASE + loc->loc_offset,
	    loc->loc_size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map DC\n");
		return;
	}

	aprint_naive("\n");
	aprint_normal(": DISPLAY%c\n", loc->loc_port + 'A');
}


static int
tegra_dc_allocmem(struct tegra_dc_softc *sc, bus_size_t size)
{
	int error, nsegs;

	error = bus_dmamem_alloc(sc->sc_dmat, size, TEGRA_DC_FBALIGN, 0,
	    sc->sc_dmasegs, 1, &nsegs, BUS_DMA_WAITOK);
	if (error)
		return error;
	error = bus_dmamem_map(sc->sc_dmat, sc->sc_dmasegs, nsegs, size,
	    &sc->sc_dmap, BUS_DMA_WAITOK | BUS_DMA_COHERENT);
	if (error)
		goto free;
	error = bus_dmamap_create(sc->sc_dmat, size, 1, size, 0,
	    BUS_DMA_WAITOK, &sc->sc_dmamap);
	if (error)
		goto unmap;
	error = bus_dmamap_load(sc->sc_dmat, sc->sc_dmamap, sc->sc_dmap,
	    size, NULL, BUS_DMA_WAITOK);
	if (error)
		goto destroy;

	sc->sc_dmasize = size;

	memset(sc->sc_dmap, 0, size);

	return 0;

destroy:
	bus_dmamap_destroy(sc->sc_dmat, sc->sc_dmamap);
unmap:
	bus_dmamem_unmap(sc->sc_dmat, sc->sc_dmap, size);
free:
	bus_dmamem_free(sc->sc_dmat, sc->sc_dmasegs, nsegs);

	sc->sc_dmasize = 0;
	sc->sc_dmap = NULL;

	return error;
}

static int
tegra_dc_print(void *aux, const char *pnp)
{
	const struct tegrafb_attach_args * const tfb = aux;

	aprint_normal(" output %s", device_xname(tfb->tfb_outputdev));

	return UNCONF;
}

static int
tegra_dc_init(struct tegra_dc_softc *sc, const struct videomode *mode)
{
	tegra_car_dc_enable(sc->sc_port);

	tegra_dc_init_win(sc, mode);
	tegra_dc_init_disp(sc, mode);
	tegra_dc_update(sc);
	return 0;
}


static void
tegra_dc_init_win(struct tegra_dc_softc *sc, const struct videomode *mode)
{
	/* Write access control */
	DC_WRITE(sc, DC_CMD_STATE_ACCESS_REG,
	    DC_CMD_STATE_ACCESS_WRITE_MUX | DC_CMD_STATE_ACCESS_READ_MUX);

	/* Enable window A programming */
	DC_WRITE(sc, DC_CMD_DISPLAY_WINDOW_HEADER_REG,
	    DC_CMD_DISPLAY_WINDOW_HEADER_WINDOW_A_SELECT);

	/* Set colour depth to ARGB8888 */
	DC_WRITE(sc, DC_WINC_A_COLOR_DEPTH_REG,
	    __SHIFTIN(DC_WINC_A_COLOR_DEPTH_DEPTH_T_A8R8G8B8,
		      DC_WINC_A_COLOR_DEPTH_DEPTH));

	/* Disable byte swapping */
	DC_WRITE(sc, DC_WINC_A_BYTE_SWAP_REG,
	    __SHIFTIN(DC_WINC_A_BYTE_SWAP_SWAP_NOSWAP,
		      DC_WINC_A_BYTE_SWAP_SWAP));

	/* Initial DDA */
	DC_WRITE(sc, DC_WINC_A_H_INITIAL_DDA_REG, 0);
	DC_WRITE(sc, DC_WINC_A_V_INITIAL_DDA_REG, 0);
	DC_WRITE(sc, DC_WINC_A_DDA_INCREMENT_REG, 0x10001000);

	/* Window position, size, stride */
	DC_WRITE(sc, DC_WINC_A_POSITION_REG,
	    __SHIFTIN(0, DC_WINC_A_POSITION_V) |
	    __SHIFTIN(0, DC_WINC_A_POSITION_H));
	DC_WRITE(sc, DC_WINC_A_SIZE_REG,
	    __SHIFTIN(mode->vdisplay, DC_WINC_A_SIZE_V) |
	    __SHIFTIN(mode->hdisplay, DC_WINC_A_SIZE_H));
	DC_WRITE(sc, DC_WINC_A_PRESCALED_SIZE_REG,
	    __SHIFTIN(mode->vdisplay, DC_WINC_A_PRESCALED_SIZE_V) |
	    __SHIFTIN(mode->hdisplay * (TEGRA_DC_DEPTH / 8),
		      DC_WINC_A_PRESCALED_SIZE_H));
	DC_WRITE(sc, DC_WINC_A_LINE_STRIDE_REG,
	    __SHIFTIN(mode->hdisplay * (TEGRA_DC_DEPTH / 8),
		      DC_WINC_A_LINE_STRIDE_LINE_STRIDE));

	/* Framebuffer start address */
	DC_WRITE(sc, DC_WINBUF_A_START_ADDR_REG,
	    (uint32_t)sc->sc_dmamap->dm_segs[0].ds_addr);

	/* Offsets */
	DC_WRITE(sc, DC_WINBUF_A_ADDR_H_OFFSET_REG, 0);
	DC_WRITE(sc, DC_WINBUF_A_ADDR_V_OFFSET_REG, 0);

	/* Surface kind */
	DC_WRITE(sc, DC_WINBUF_A_SURFACE_KIND_REG,
	    __SHIFTIN(DC_WINBUF_A_SURFACE_KIND_SURFACE_KIND_PITCH,
		      DC_WINBUF_A_SURFACE_KIND_SURFACE_KIND));

	/* Enable window A */
	DC_WRITE(sc, DC_WINC_A_WIN_OPTIONS_REG,
	    DC_WINC_A_WIN_OPTIONS_WIN_ENABLE);
}

static void
tegra_dc_init_disp(struct tegra_dc_softc *sc, const struct videomode *mode)
{
	const u_int hspw = mode->hsync_end - mode->hsync_start;
	const u_int hbp = mode->htotal - mode->hsync_end;
	const u_int hfp = mode->hsync_start - mode->hdisplay;
	const u_int vspw = mode->vsync_end - mode->vsync_start;
	const u_int vbp = mode->vtotal - mode->vsync_end;
	const u_int vfp = mode->vsync_start - mode->vdisplay;

	DC_WRITE(sc, DC_DISP_DISP_TIMING_OPTIONS_REG,
	    __SHIFTIN(1, DC_DISP_DISP_TIMING_OPTIONS_VSYNC_POS));
	DC_WRITE(sc, DC_DISP_DISP_COLOR_CONTROL_REG,
	    __SHIFTIN(DC_DISP_DISP_COLOR_CONTROL_BASE_COLOR_SIZE_888,
		      DC_DISP_DISP_COLOR_CONTROL_BASE_COLOR_SIZE));
	DC_WRITE(sc, DC_DISP_DISP_SIGNAL_OPTIONS0_REG,
	    DC_DISP_DISP_SIGNAL_OPTIONS0_H_PULSE2_ENABLE);
	DC_WRITE(sc, DC_DISP_H_PULSE2_CONTROL_REG,
	    __SHIFTIN(DC_DISP_H_PULSE2_CONTROL_V_QUAL_VACTIVE,
		      DC_DISP_H_PULSE2_CONTROL_V_QUAL) |
	    __SHIFTIN(DC_DISP_H_PULSE2_CONTROL_LAST_END_A,
		      DC_DISP_H_PULSE2_CONTROL_LAST));

	u_int pulse_start = 1 + hspw + hbp - 10;
	DC_WRITE(sc, DC_DISP_H_PULSE2_POSITION_A_REG,
	    __SHIFTIN(pulse_start, DC_DISP_H_PULSE2_POSITION_A_START) |
	    __SHIFTIN(pulse_start + 8, DC_DISP_H_PULSE2_POSITION_A_END));

	/* Pixel clock */
	const u_int div = (tegra_car_plld2_rate() * 2) / (mode->dot_clock * 1000) - 2;
	DC_WRITE(sc, DC_DISP_DISP_CLOCK_CONTROL_REG,
	    __SHIFTIN(0, DC_DISP_DISP_CLOCK_CONTROL_PIXEL_CLK_DIVIDER) |
	    __SHIFTIN(div, DC_DISP_DISP_CLOCK_CONTROL_SHIFT_CLK_DIVIDER));

	/* Mode timings */
	DC_WRITE(sc, DC_DISP_REF_TO_SYNC_REG,
	    __SHIFTIN(1, DC_DISP_REF_TO_SYNC_V) |
	    __SHIFTIN(1, DC_DISP_REF_TO_SYNC_H));
	DC_WRITE(sc, DC_DISP_SYNC_WIDTH_REG,
	    __SHIFTIN(vspw, DC_DISP_SYNC_WIDTH_V) |
	    __SHIFTIN(hspw, DC_DISP_SYNC_WIDTH_H));
	DC_WRITE(sc, DC_DISP_BACK_PORCH_REG,
	    __SHIFTIN(vbp, DC_DISP_BACK_PORCH_V) |
	    __SHIFTIN(hbp, DC_DISP_BACK_PORCH_H));
	DC_WRITE(sc, DC_DISP_FRONT_PORCH_REG,
	    __SHIFTIN(vfp, DC_DISP_FRONT_PORCH_V) |
	    __SHIFTIN(hfp, DC_DISP_FRONT_PORCH_H));
	DC_WRITE(sc, DC_DISP_DISP_ACTIVE_REG,
	    __SHIFTIN(mode->vdisplay, DC_DISP_DISP_ACTIVE_V) |
	    __SHIFTIN(mode->hdisplay, DC_DISP_DISP_ACTIVE_H));

	/* Enable continus display mode */
	DC_WRITE(sc, DC_CMD_DISPLAY_COMMAND_REG,
	    __SHIFTIN(DC_CMD_DISPLAY_COMMAND_DISPLAY_CTRL_MODE_C_DISPLAY,
		      DC_CMD_DISPLAY_COMMAND_DISPLAY_CTRL_MODE));

	/* Enable power */
	DC_SET_CLEAR(sc, DC_CMD_DISPLAY_POWER_CONTROL_REG,
	    DC_CMD_DISPLAY_POWER_CONTROL_PM1_ENABLE |
	    DC_CMD_DISPLAY_POWER_CONTROL_PM0_ENABLE |
	    DC_CMD_DISPLAY_POWER_CONTROL_PW4_ENABLE |
	    DC_CMD_DISPLAY_POWER_CONTROL_PW3_ENABLE |
	    DC_CMD_DISPLAY_POWER_CONTROL_PW2_ENABLE |
	    DC_CMD_DISPLAY_POWER_CONTROL_PW1_ENABLE |
	    DC_CMD_DISPLAY_POWER_CONTROL_PW0_ENABLE,
	    0);
}

static void
tegra_dc_update(struct tegra_dc_softc *sc)
{
	/* Commit settings */
	DC_WRITE(sc, DC_CMD_STATE_CONTROL_REG,
	    DC_CMD_STATE_CONTROL_GENERAL_UPDATE | DC_CMD_STATE_CONTROL_WIN_A_UPDATE);
	DC_WRITE(sc, DC_CMD_STATE_CONTROL_REG,
	    DC_CMD_STATE_CONTROL_GENERAL_ACT_REQ | DC_CMD_STATE_CONTROL_WIN_A_ACT_REQ);
}

int
tegra_dc_port(device_t dev)
{
	struct tegra_dc_softc * const sc = device_private(dev);

	return sc->sc_port;
}

int
tegra_dc_enable(device_t dev, device_t outputdev,
    const struct videomode *mode, const uint8_t *edid)
{
	struct tegra_dc_softc * const sc = device_private(dev);
	prop_dictionary_t prop = device_properties(dev);
	bool is_console = false;
	int error;

	KASSERT(sc->sc_fbdev == NULL);

	prop_dictionary_get_bool(prop, "is_console", &is_console);

	const uint32_t depth = TEGRA_DC_DEPTH;
	const uint32_t fbsize = mode->vdisplay * mode->hdisplay * (depth / 8);
	const bus_size_t dmasize = (fbsize + PAGE_MASK) & ~PAGE_MASK;
	error = tegra_dc_allocmem(sc, dmasize);
	if (error) {
		aprint_error_dev(dev,
		    "failed to allocate %u bytes of video memory: %d\n",
		    (u_int)dmasize, error);
		return error;
	}

	error = tegra_dc_init(sc, mode);
	if (error) {
		aprint_error_dev(dev,
		    "failed to initialize display controller: %d\n", error);
		return error;
	}

	struct tegrafb_attach_args tfb = {
		.tfb_console = is_console,
		.tfb_dmat = sc->sc_dmat,
		.tfb_dmamap = sc->sc_dmamap,
		.tfb_dmap = sc->sc_dmap,
		.tfb_width = mode->hdisplay,
		.tfb_height = mode->vdisplay,
		.tfb_depth = depth,
		.tfb_stride = mode->hdisplay * (depth / 8),
		.tfb_outputdev = outputdev,
	};

	sc->sc_fbdev = config_found(sc->sc_dev, &tfb, tegra_dc_print);

	if (sc->sc_fbdev != NULL && edid != NULL) {
		prop_data_t data = prop_data_create_data(edid, 128);
		prop_dictionary_set(device_properties(sc->sc_fbdev),
		    "EDID", data);
		prop_object_release(data);
	}

	return 0;
}

void
tegra_dc_hdmi_start(device_t dev)
{
	struct tegra_dc_softc * const sc = device_private(dev);

	DC_SET_CLEAR(sc, DC_DISP_DISP_WIN_OPTIONS_REG,
	    DC_DISP_DISP_WIN_OPTIONS_HDMI_ENABLE, 0);

	tegra_dc_update(sc);
}
