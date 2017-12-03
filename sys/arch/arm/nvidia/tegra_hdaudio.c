/* $NetBSD: tegra_hdaudio.c,v 1.9.2.2 2017/12/03 11:35:54 jdolecek Exp $ */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tegra_hdaudio.c,v 1.9.2.2 2017/12/03 11:35:54 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <dev/hdaudio/hdaudioreg.h>
#include <dev/hdaudio/hdaudiovar.h>

#include <arm/nvidia/tegra_var.h>
#include <arm/nvidia/tegra_pmcreg.h>
#include <arm/nvidia/tegra_hdaudioreg.h>

#include <dev/fdt/fdtvar.h>

#define TEGRA_HDAUDIO_OFFSET	0x8000

#define TEGRA_HDA_IFPS_BAR0_REG		0x0080
#define TEGRA_HDA_IFPS_CONFIG_REG	0x0180
#define TEGRA_HDA_IFPS_INTR_REG		0x0188
#define TEGRA_HDA_CFG_CMD_REG		0x1004
#define TEGRA_HDA_CFG_BAR0_REG		0x1010

static int	tegra_hdaudio_match(device_t, cfdata_t, void *);
static void	tegra_hdaudio_attach(device_t, device_t, void *);
static int	tegra_hdaudio_detach(device_t, int);
static int	tegra_hdaudio_rescan(device_t, const char *, const int *);
static void	tegra_hdaudio_childdet(device_t, device_t);

static int	tegra_hdaudio_intr(void *);

struct tegra_hdaudio_softc {
	struct hdaudio_softc	sc;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	void			*sc_ih;
	int			sc_phandle;
	struct clk		*sc_clk_hda;
	struct clk		*sc_clk_hda2hdmi;
	struct clk		*sc_clk_hda2codec_2x;
	struct fdtbus_reset	*sc_rst_hda;
	struct fdtbus_reset	*sc_rst_hda2hdmi;
	struct fdtbus_reset	*sc_rst_hda2codec_2x;
};

static int	tegra_hdaudio_init_clocks(struct tegra_hdaudio_softc *);
static void	tegra_hdaudio_init(struct tegra_hdaudio_softc *);

CFATTACH_DECL2_NEW(tegra_hdaudio, sizeof(struct tegra_hdaudio_softc),
	tegra_hdaudio_match, tegra_hdaudio_attach, tegra_hdaudio_detach, NULL,
	tegra_hdaudio_rescan, tegra_hdaudio_childdet);

static int
tegra_hdaudio_match(device_t parent, cfdata_t cf, void *aux)
{
	const char * const compatible[] = {
		"nvidia,tegra210-hda",
		"nvidia,tegra124-hda",
		NULL
	};
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
tegra_hdaudio_attach(device_t parent, device_t self, void *aux)
{
	struct tegra_hdaudio_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	char intrstr[128];
	bus_addr_t addr;
	bus_size_t size;
	int error;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}
	sc->sc_clk_hda = fdtbus_clock_get(phandle, "hda");
	if (sc->sc_clk_hda == NULL) {
		aprint_error(": couldn't get clock hda\n");
		return;
	}
	sc->sc_clk_hda2hdmi = fdtbus_clock_get(phandle, "hda2hdmi");
	if (sc->sc_clk_hda2hdmi == NULL) {
		aprint_error(": couldn't get clock hda2hdmi\n");
		return;
	}
	sc->sc_clk_hda2codec_2x = fdtbus_clock_get(phandle, "hda2codec_2x");
	if (sc->sc_clk_hda2codec_2x == NULL) {
		aprint_error(": couldn't get clock hda2codec_2x\n");
		return;
	}
	sc->sc_rst_hda = fdtbus_reset_get(phandle, "hda");
	if (sc->sc_rst_hda == NULL) {
		aprint_error(": couldn't get reset hda\n");
		return;
	}
	sc->sc_rst_hda2hdmi = fdtbus_reset_get(phandle, "hda2hdmi");
	if (sc->sc_rst_hda2hdmi == NULL) {
		aprint_error(": couldn't get reset hda2hdmi\n");
		return;
	}
	sc->sc_rst_hda2codec_2x = fdtbus_reset_get(phandle, "hda2codec_2x");
	if (sc->sc_rst_hda2codec_2x == NULL) {
		aprint_error(": couldn't get reset hda2codec_2x\n");
		return;
	}

	sc->sc_phandle = phandle;
	sc->sc_bst = faa->faa_bst;
	error = bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh);
	if (error) {
		aprint_error(": couldn't map %#llx: %d", (uint64_t)addr, error);
		return;
	}

	sc->sc.sc_dev = self;
	sc->sc.sc_memt = faa->faa_bst;
	bus_space_subregion(sc->sc.sc_memt, sc->sc_bsh, TEGRA_HDAUDIO_OFFSET,
	    size - TEGRA_HDAUDIO_OFFSET, &sc->sc.sc_memh);
	sc->sc.sc_memvalid = true;
	sc->sc.sc_dmat = faa->faa_dmat;

	aprint_naive("\n");
	aprint_normal(": HDA\n");

	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error_dev(self, "failed to decode interrupt\n");
		return;
	}

	sc->sc_ih = fdtbus_intr_establish(phandle, 0, IPL_AUDIO, 0,
	    tegra_hdaudio_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt on %s\n",
		    intrstr);
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	tegra_pmc_power(PMC_PARTID_DISB, true);

	if (tegra_hdaudio_init_clocks(sc) != 0)
		return;

	tegra_hdaudio_init(sc);

	hdaudio_attach(self, &sc->sc);
}

static int
tegra_hdaudio_init_clocks(struct tegra_hdaudio_softc *sc)
{
	device_t self = sc->sc.sc_dev;
	int error;

	/* Assert resets */
	fdtbus_reset_assert(sc->sc_rst_hda);
	fdtbus_reset_assert(sc->sc_rst_hda2hdmi);
	fdtbus_reset_assert(sc->sc_rst_hda2codec_2x);

	/* Set hda to 48MHz and enable it */
	error = clk_set_rate(sc->sc_clk_hda, 48000000);
	if (error) {
		aprint_error_dev(self, "couldn't set hda frequency: %d\n",
		    error);
		return error;
	}
	error = clk_enable(sc->sc_clk_hda);
	if (error) {
		aprint_error_dev(self, "couldn't enable clock hda: %d\n",
		    error);
		return error;
	}

	/* Enable hda2hdmi clock */
	error = clk_enable(sc->sc_clk_hda2hdmi);
	if (error) {
		aprint_error_dev(self, "couldn't enable clock hda2hdmi: %d\n",
		    error);
		return error;
	}

	/* Set hda2codec_2x to 48MHz and enable it */
	error = clk_set_rate(sc->sc_clk_hda2codec_2x, 48000000);
	if (error) {
		aprint_error_dev(self,
		    "couldn't set clock hda2codec_2x frequency: %d\n", error);
		return error;
	}
	error = clk_enable(sc->sc_clk_hda2codec_2x);
	if (error) {
		aprint_error_dev(self,
		    "couldn't enable clock hda2codec_2x: %d\n", error);
		return error;
	}

	/* De-assert resets */
	fdtbus_reset_deassert(sc->sc_rst_hda);
	fdtbus_reset_deassert(sc->sc_rst_hda2hdmi);
	fdtbus_reset_deassert(sc->sc_rst_hda2codec_2x);

	return 0;
}

static void
tegra_hdaudio_init(struct tegra_hdaudio_softc *sc)
{
	tegra_reg_set_clear(sc->sc_bst, sc->sc_bsh, TEGRA_HDA_IFPS_CONFIG_REG,
	    TEGRA_HDA_IFPS_CONFIG_FPCI_EN, 0);
	tegra_reg_set_clear(sc->sc_bst, sc->sc_bsh, TEGRA_HDA_CFG_CMD_REG,
	    TEGRA_HDA_CFG_CMD_ENABLE_SERR |
	    TEGRA_HDA_CFG_CMD_BUS_MASTER |
	    TEGRA_HDA_CFG_CMD_MEM_SPACE |
	    TEGRA_HDA_CFG_CMD_IO_SPACE,
	    TEGRA_HDA_CFG_CMD_DISABLE_INTR);
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, TEGRA_HDA_CFG_BAR0_REG,
	    0xffffffff);
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, TEGRA_HDA_CFG_BAR0_REG,
	    0x00004000);
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, TEGRA_HDA_IFPS_BAR0_REG,
	    TEGRA_HDA_CFG_BAR0_START);
	tegra_reg_set_clear(sc->sc_bst, sc->sc_bsh, TEGRA_HDA_IFPS_INTR_REG,
	    TEGRA_HDA_IFPS_INTR_EN, 0);
}

static int
tegra_hdaudio_detach(device_t self, int flags)
{
	struct tegra_hdaudio_softc * const sc = device_private(self);

	hdaudio_detach(&sc->sc, flags);

	if (sc->sc_ih) {
		fdtbus_intr_disestablish(sc->sc_phandle, sc->sc_ih);
		sc->sc_ih = NULL;
	}

	sc->sc.sc_memvalid = false;

	return 0;
}

static int
tegra_hdaudio_rescan(device_t self, const char *ifattr, const int *locs)
{
	struct tegra_hdaudio_softc * const sc = device_private(self);

	return hdaudio_rescan(&sc->sc, ifattr, locs);
}

static void
tegra_hdaudio_childdet(device_t self, device_t child)
{
	struct tegra_hdaudio_softc * const sc = device_private(self);

	hdaudio_childdet(&sc->sc, child);
}

static int
tegra_hdaudio_intr(void *priv)
{
	struct tegra_hdaudio_softc * const sc = priv;

	return hdaudio_intr(&sc->sc);
}
