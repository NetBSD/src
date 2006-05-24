/*	$NetBSD: wdc_pnpbus.c,v 1.2.2.2 2006/05/24 15:48:20 tron Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: wdc_pnpbus.c,v 1.2.2.2 2006/05/24 15:48:20 tron Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/bus.h>
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
	struct	ata_queue sc_chqueue;
	struct	wdc_regs sc_wdc_regs;
	void	*sc_ih;
};

static int	wdc_pnpbus_probe(struct device *, struct cfdata *, void *);
static void	wdc_pnpbus_attach(struct device *, struct device *, void *);

CFATTACH_DECL(wdc_pnpbus, sizeof(struct wdc_pnpbus_softc),
    wdc_pnpbus_probe, wdc_pnpbus_attach, NULL, NULL);

static int
wdc_pnpbus_probe(struct device *parent, struct cfdata *match, void *aux)
{
	struct pnpbus_dev_attach_args *pna = aux;
	int ret = 0;

	/* XXX special case the Powerstack E1, it has wdc builtin on 14E
	 * while the siop is builtin on 14L.  No idea how this works at all.
	 */
	if (strcmp(res->VitalProductData.PrintableModel, "(e1)") == 0)
		return ret;

	if (strcmp(pna->pna_devid, "PNP0600") == 0)
		ret = 1;

	if (ret)
		pnpbus_scan(pna, pna->pna_ppc_dev);

	return ret;
}

static void
wdc_pnpbus_attach(struct device *parent, struct device *self, void *aux)
{
	struct wdc_pnpbus_softc *sc = (void *)self;
	struct wdc_regs *wdr;
	struct pnpbus_dev_attach_args *pna = aux;
	int cmd_iobase, cmd_len, aux_iobase, aux_len, i;

	sc->sc_wdcdev.regs = wdr = &sc->sc_wdc_regs;

	wdr->cmd_iot = pna->pna_iot;
	wdr->ctl_iot = pna->pna_iot;
	pnpbus_getioport(&pna->pna_res, 0, &cmd_iobase, &cmd_len);
	pnpbus_getioport(&pna->pna_res, 1, &aux_iobase, &aux_len);

	if (pnpbus_io_map(&pna->pna_res, 0, &wdr->cmd_iot, &wdr->cmd_baseioh) ||
	    pnpbus_io_map(&pna->pna_res, 1, &wdr->ctl_iot, &wdr->ctl_ioh)) {
		printf("%s: couldn't map registers\n",
		    sc->sc_wdcdev.sc_atac.atac_dev.dv_xname);
	}

	for (i = 0; i < cmd_len; i++) {
		if (bus_space_subregion(wdr->cmd_iot,
		      wdr->cmd_baseioh, i, i == 0 ? 4 : 1,
		      &wdr->cmd_iohs[i]) != 0) {
			printf(": couldn't subregion registers\n");
			return;
		}
	}

	wdr->data32iot = wdr->cmd_iot;
	wdr->data32ioh = wdr->cmd_iohs[0];

	sc->sc_wdcdev.cap |= WDC_CAPABILITY_PREATA;
	sc->sc_wdcdev.sc_atac.atac_cap |= ATAC_CAP_DATA16;
	if (device_cfdata(&sc->sc_wdcdev.sc_atac.atac_dev)->cf_flags &
	    WDC_OPTIONS_32)
		sc->sc_wdcdev.sc_atac.atac_cap |= ATAC_CAP_DATA32;

	sc->sc_wdcdev.sc_atac.atac_pio_cap = 0;
	sc->sc_chanlist[0] = &sc->sc_channel;
	sc->sc_wdcdev.sc_atac.atac_channels = sc->sc_chanlist;
	sc->sc_wdcdev.sc_atac.atac_nchannels = 1;
	sc->sc_channel.ch_channel = 0;
	sc->sc_channel.ch_atac = &sc->sc_wdcdev.sc_atac;
	sc->sc_channel.ch_queue = &sc->sc_chqueue;
	sc->sc_channel.ch_ndrive = 2;
	wdc_init_shadow_regs(&sc->sc_channel);

	sc->sc_ih = pnpbus_intr_establish(0, IPL_BIO, wdcintr, &sc->sc_channel,
	    &pna->pna_res);

	wdcattach(&sc->sc_channel);
}
