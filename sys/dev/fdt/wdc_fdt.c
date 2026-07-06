/*	$NetBSD: wdc_fdt.c,v 1.1 2026/07/06 23:45:48 thorpej Exp $	*/

/*-
 * Copyright (c) 2025 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
__KERNEL_RCSID(0, "$NetBSD: wdc_fdt.c,v 1.1 2026/07/06 23:45:48 thorpej Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>

#include <dev/fdt/fdtvar.h>

#include <dev/ic/wdcreg.h>
#include <dev/ata/atavar.h>
#include <dev/ic/wdcvar.h>

struct wdc_fdt_softc {
	struct wdc_softc sc_wdcdev;
	struct ata_channel *wdc_chanlist[1];
	struct ata_channel ata_channel;
	struct wdc_regs wdc_regs;
	void *sc_ih;
	uint8_t pio_mode[2];
};

struct wdc_fdt_config {
	void	(*set_modes)(struct ata_channel *);
};

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "ata-generic" },
	DEVICE_COMPAT_EOL
};

static int
wdc_fdt_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
wdc_fdt_attach(device_t parent, device_t self, void *aux)
{
	struct wdc_fdt_softc *sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	struct wdc_regs *wdr;
	bus_addr_t cmd_addr, ctl_addr;
	bus_size_t cmd_size, ctl_size;
	char intrstr[128];
	int error;

	const struct wdc_fdt_config *config =
            of_compatible_lookup(phandle, compat_data)->data;

	sc->sc_wdcdev.sc_atac.atac_dev = self;
	sc->sc_wdcdev.regs = wdr = &sc->wdc_regs;
	wdr->cmd_iot = faa->faa_bst;
	wdr->ctl_iot = faa->faa_bst;

	if (fdtbus_get_reg(phandle, 0, &cmd_addr, &cmd_size) != 0) {
		aprint_error(": couldn't get command registers\n");
		return;
	}

	if (fdtbus_get_reg(phandle, 1, &ctl_addr, &ctl_size) != 0) {
		aprint_error(": couldn't get control registers\n");
		return;
	}

	error = bus_space_map(wdr->cmd_iot, cmd_addr, cmd_size, 0,
			      &wdr->cmd_baseioh);
	if (error) {
		aprint_error(": couldn't map command registers (error=%d)\n",
		    error);
		return;
	}

	error = bus_space_map(wdr->ctl_iot, ctl_addr, ctl_size, 0,
			      &wdr->ctl_ioh);
	if (error) {
		aprint_error(": couldn't map control registers (error=%d)\n",
		    error);
		return;
	}

	bus_size_t data_regsize = 2;
	if (of_getprop_bool(phandle, "ata-generic,use8bit")) {
		data_regsize = 1;
	} else {
		sc->sc_wdcdev.sc_atac.atac_cap |= ATAC_CAP_DATA16;
		if (!of_getprop_bool(phandle, "ata-generic,use16bit")) {
			sc->sc_wdcdev.sc_atac.atac_cap |= ATAC_CAP_DATA32;
			data_regsize = 4;
		}
	}

	uint32_t reg_shift;
	if (of_getprop_uint32(phandle, "reg-shift", &reg_shift) < 0) {
		reg_shift = 0;
	}

	for (int i = 0; i < WDC_NREG; i++) {
		error = bus_space_subregion(wdr->cmt_iot, wdr->cmd_baseioh,
					    i << reg_shift,
					    i == wd_data ? data_regsize : 1,
					    &wdr->cmd_iohs[i]);
		(void) data_regsize;
		if (error) {
			aprint_error(": couldn't subregion command "
				     "register %d (error=%d)\n", i, error);
			return;
		}
	}

	uint32_t max_pio;
	if (of_getprop_uint32(phandle, "pio-mode", &max_pio) < 0 ||
	    config == NULL || config->set_modes == NULL) {
		max_pio = 0;
	}

	sc->sc_wdcdev.sc_atac.atac_pio_cap = max_pio;
	sc->wdc_chanlist[0] = &sc->ata_channel;
	sc->sc_wdcdev.sc_atac.atac_channels = sc->wdc_chanlist;
	sc->sc_wdcdev.sc_atac.atac_nchannels = 1;
	sc->sc_wdcdev.sc_atac.atac_set_modes = config != NULL ?
	    config->set_modes : NULL;
	sc->sc_wdcdev.wdc_maxdrives = 2;
	sc->ata_channel.ch_channel = 0;
	sc->ata_channel.ch_atac = &sc->sc_wdcdev.sc_atac;

	wdc_init_shadow_regs(wdr);

	aprint_normal("\n");

	if (! fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		goto use_polled_mode;
	}

	sc->sc_ih = fdtbus_intr_establish_xname(phandle, 0, IPL_BIO,
	    0, wdcintr, &sc->ata_channel, device_xname(self));
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt at %s\n",
		    intrstr);
 use_polled_mode:
		aprint_verbose_dev(self, "using polled I/O\n");
		sc->sc_wdcdev.sc_atac.atac_cap |= ATAC_CAP_NOIRQ;
	} else {
		aprint_normal_dev(self, "interrupting at %s\n", intrstr);
	}

	if (wdcprobe(wdr)) {
		wdcattach(&sc->ata_channel);
	} else {
		aprint_normal_dev(self, "no drives present.\n");
	}
}

CFATTACH_DECL_NEW(wdc_fdt, sizeof(struct wdc_fdt_softc),
    wdc_fdt_match, wdc_fdt_attach, NULL, NULL);
