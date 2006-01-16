/*	$NetBSD: pciide_pnpbios.c,v 1.21 2006/01/16 20:30:19 bouyer Exp $	*/

/*
 * Copyright (c) 1999 Soren S. Jorvang.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Handle the weird "almost PCI" IDE on Toshiba Porteges.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pciide_pnpbios.c,v 1.21 2006/01/16 20:30:19 bouyer Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/bus.h>

#include <dev/ic/wdcreg.h>
#include <dev/isa/isavar.h>
#include <dev/isa/isadmavar.h>

#include <i386/pnpbios/pnpbiosvar.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <dev/pci/pciidereg.h>
#include <dev/pci/pciidevar.h>

static int	pciide_pnpbios_match(struct device *, struct cfdata *, void *);
static void	pciide_pnpbios_attach(struct device *, struct device *, void *);
void		pciide_pnpbios_setup_channel(struct ata_channel *);

extern void	pciide_channel_dma_setup(struct pciide_channel *);
extern int	pciide_dma_init(void *, int, int, void *, size_t, int);
extern void	pciide_dma_start(void *, int, int);
extern int	pciide_dma_finish(void *, int, int, int);
extern int	pciide_compat_intr (void *);

CFATTACH_DECL(pciide_pnpbios, sizeof(struct pciide_softc),
    pciide_pnpbios_match, pciide_pnpbios_attach, NULL, NULL);

int
pciide_pnpbios_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct pnpbiosdev_attach_args *aa = aux;

	if (strcmp(aa->idstr, "TOS7300") == 0)
		return 1;

	return 0;
}

void
pciide_pnpbios_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct pciide_softc *sc = (void *)self;
	struct pnpbiosdev_attach_args *aa = aux;
	struct pciide_channel *cp;
	struct ata_channel *wdc_cp;
	struct wdc_regs *wdr;
	bus_space_tag_t compat_iot;
	bus_space_handle_t cmd_baseioh, ctl_ioh;
	int i;

	printf("\n");
	pnpbios_print_devres(self, aa);

	printf("%s: Toshiba Extended IDE Controller\n", self->dv_xname);

	if (pnpbios_io_map(aa->pbt, aa->resc, 2, &sc->sc_dma_iot,
	    &sc->sc_dma_ioh) != 0) {
		printf("%s: unable to map DMA registers\n", self->dv_xname);
		return;
	}
	if (pnpbios_io_map(aa->pbt, aa->resc, 0, &compat_iot,
	    &cmd_baseioh) != 0) {
		printf("%s: unable to map command registers\n", self->dv_xname);
		return;
	}
	if (pnpbios_io_map(aa->pbt, aa->resc, 1, &compat_iot,
	    &ctl_ioh) != 0) {
		printf("%s: unable to map control register\n", self->dv_xname);
		return;
	}

	sc->sc_dmat = &pci_bus_dma_tag;

	sc->sc_dma_ok = 1;
	sc->sc_wdcdev.dma_arg = sc;
	sc->sc_wdcdev.dma_init = pciide_dma_init;
	sc->sc_wdcdev.dma_start = pciide_dma_start;
	sc->sc_wdcdev.dma_finish = pciide_dma_finish;
	sc->sc_wdcdev.sc_atac.atac_channels = sc->wdc_chanarray;
	sc->sc_wdcdev.sc_atac.atac_nchannels = 1;
	sc->sc_wdcdev.sc_atac.atac_cap |= ATAC_CAP_DATA16 | ATAC_CAP_DATA32;
	sc->sc_wdcdev.sc_atac.atac_cap |= ATAC_CAP_DMA | ATAC_CAP_UDMA;
        sc->sc_wdcdev.sc_atac.atac_pio_cap = 4;
        sc->sc_wdcdev.sc_atac.atac_dma_cap = 2;		/* XXX */
        sc->sc_wdcdev.sc_atac.atac_udma_cap = 2;	/* XXX */
#if 0 /* XXX */
	sc->sc_wdcdev.set_modes = pciide_pnpbios_setup_channel;
#endif

	wdc_allocate_regs(&sc->sc_wdcdev);

	cp = &sc->pciide_channels[0];
	sc->wdc_chanarray[0] = &cp->ata_channel;
	cp->ata_channel.ch_channel = 0;
	cp->ata_channel.ch_atac = &sc->sc_wdcdev.sc_atac;
	cp->ata_channel.ch_queue = malloc(sizeof(struct ata_queue),
					  M_DEVBUF, M_NOWAIT);
	cp->ata_channel.ch_ndrive = 2;
	if (cp->ata_channel.ch_queue == NULL) {
		printf("%s: unable to allocate memory for command queue\n",
			self->dv_xname);
		return;
	}

	wdc_cp = &cp->ata_channel;
	wdr = CHAN_TO_WDC_REGS(wdc_cp);
	wdr->cmd_iot = compat_iot;
	wdr->cmd_baseioh = cmd_baseioh;

	for (i = 0; i < WDC_NREG; i++) {
		if (bus_space_subregion(wdr->cmd_iot, wdr->cmd_baseioh, i,
		    i == 0 ? 4 : 1, &wdr->cmd_iohs[i]) != 0) {
			    printf("%s: unable to subregion control register\n",
				self->dv_xname);
			    return;
		}
	}
	wdc_init_shadow_regs(wdc_cp);

	wdr->ctl_iot = wdr->data32iot = compat_iot;
	wdr->ctl_ioh = wdr->data32ioh = ctl_ioh;

	cp->compat = 1;

	cp->ih = pnpbios_intr_establish(aa->pbt, aa->resc, 0, IPL_BIO,
					pciide_compat_intr, cp);

	wdcattach(wdc_cp);
}

void
pciide_pnpbios_setup_channel(chp)
	struct ata_channel *chp;
{
	struct pciide_channel *cp = CHAN_TO_PCHAN(chp);

	pciide_channel_dma_setup(cp);
}
