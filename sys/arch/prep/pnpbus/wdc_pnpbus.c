/*	$NetBSD: wdc_pnpbus.c,v 1.13.2.1 2017/12/03 11:36:38 jdolecek Exp $	*/

/*-
 * Copyright (c) 1998, 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum and by Onno van der Linden.
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
__KERNEL_RCSID(0, "$NetBSD: wdc_pnpbus.c,v 1.13.2.1 2017/12/03 11:36:38 jdolecek Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <sys/bus.h>
#include <machine/intr.h>
#include <machine/isa_machdep.h>
#include <machine/residual.h>

#include <dev/ata/atavar.h>
#include <dev/ic/wdcvar.h>

#include <prep/pnpbus/pnpbusvar.h>

/* options passed via the 'flags' config keyword */
#define WDC_OPTIONS_32	0x01 /* try to use 32bit data I/O */

struct wdc_pnpbus_softc {
	struct	wdc_softc sc_wdcdev;
	struct	ata_channel *sc_chanlist[1];
	struct	ata_channel sc_channel;
	struct	wdc_regs sc_wdc_regs;
	void	*sc_ih;
};

static int	wdc_pnpbus_probe(device_t, cfdata_t, void *);
static void	wdc_pnpbus_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(wdc_pnpbus, sizeof(struct wdc_pnpbus_softc),
    wdc_pnpbus_probe, wdc_pnpbus_attach, NULL, NULL);

static int
wdc_pnpbus_probe(device_t parent, cfdata_t match, void *aux)
{
	struct pnpbus_dev_attach_args *pna = aux;
	int ret = 0;

	/* XXX special case the Powerstack E1, it has wdc builtin on 14E
	 * while the siop is builtin on 14L.  No idea how this works at all.
	 */
	if (strcmp(res->VitalProductData.PrintableModel, "(e1)") == 0)
		return ret;

	/* XXX special case the MTX604/mcp750.  The onboard IDE is actually
	 * a PCIIDE chip.
	 */

	if (strcmp(res->VitalProductData.PrintableModel,
	    "000000000000000000000000000(e2)") == 0)
		return ret;

	if (strcmp(pna->pna_devid, "PNP0600") == 0)
		ret = 1;

	if (ret)
		pnpbus_scan(pna, pna->pna_ppc_dev);

	return ret;
}

static void
wdc_pnpbus_attach(device_t parent, device_t self, void *aux)
{
	struct wdc_pnpbus_softc *sc = device_private(self);
	struct wdc_regs *wdr;
	struct pnpbus_dev_attach_args *pna = aux;
	int cmd_iobase, cmd_len, aux_iobase, aux_len, i;

	sc->sc_wdcdev.sc_atac.atac_dev = self;
	sc->sc_wdcdev.regs = wdr = &sc->sc_wdc_regs;

	wdr->cmd_iot = pna->pna_iot;
	wdr->ctl_iot = pna->pna_iot;
	pnpbus_getioport(&pna->pna_res, 0, &cmd_iobase, &cmd_len);
	pnpbus_getioport(&pna->pna_res, 1, &aux_iobase, &aux_len);

	if (pnpbus_io_map(&pna->pna_res, 0, &wdr->cmd_iot, &wdr->cmd_baseioh) ||
	    pnpbus_io_map(&pna->pna_res, 1, &wdr->ctl_iot, &wdr->ctl_ioh)) {
		aprint_error_dev(self, "couldn't map registers\n");
	}

	for (i = 0; i < cmd_len; i++) {
		if (bus_space_subregion(wdr->cmd_iot,
		      wdr->cmd_baseioh, i, i == 0 ? 4 : 1,
		      &wdr->cmd_iohs[i]) != 0) {
			aprint_error(": couldn't subregion registers\n");
			return;
		}
	}

	wdr->data32iot = wdr->cmd_iot;
	wdr->data32ioh = wdr->cmd_iohs[0];

	sc->sc_wdcdev.cap |= WDC_CAPABILITY_PREATA;
	sc->sc_wdcdev.sc_atac.atac_cap |= ATAC_CAP_DATA16;
	if (device_cfdata(sc->sc_wdcdev.sc_atac.atac_dev)->cf_flags &
	    WDC_OPTIONS_32)
		sc->sc_wdcdev.sc_atac.atac_cap |= ATAC_CAP_DATA32;

	sc->sc_wdcdev.sc_atac.atac_pio_cap = 0;
	sc->sc_chanlist[0] = &sc->sc_channel;
	sc->sc_wdcdev.sc_atac.atac_channels = sc->sc_chanlist;
	sc->sc_wdcdev.sc_atac.atac_nchannels = 1;
	sc->sc_wdcdev.wdc_maxdrives = 2;
	sc->sc_channel.ch_channel = 0;
	sc->sc_channel.ch_atac = &sc->sc_wdcdev.sc_atac;

	wdc_init_shadow_regs(wdr);

	sc->sc_ih = pnpbus_intr_establish(0, IPL_BIO, IST_PNP,
	    wdcintr, &sc->sc_channel, &pna->pna_res);

	aprint_normal("\n");
	wdcattach(&sc->sc_channel);
}
