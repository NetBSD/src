/*	$NetBSD: geodeide.c,v 1.1 2004/07/09 18:38:37 bouyer Exp $	*/

/*
 * Copyright (c) 2004 Manuel Bouyer.
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
 *	This product includes software developed by Manuel Bouyer.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/*
 * Driver for the IDE part of the AMD Geode CS5530A compagnion chip
 * Docs available from AMD's web site
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: geodeide.c,v 1.1 2004/07/09 18:38:37 bouyer Exp $");

#include <sys/param.h>
#include <sys/systm.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>
#include <dev/pci/pciidereg.h>
#include <dev/pci/pciidevar.h>

#include <dev/pci/pciide_geode_reg.h>

static void geodeide_chip_map(struct pciide_softc *,
				 struct pci_attach_args *);
static void geodeide_setup_channel(struct wdc_channel *);

static int  geodeide_match(struct device *, struct cfdata *, void *);
static void geodeide_attach(struct device *, struct device *, void *);

CFATTACH_DECL(geodeide, sizeof(struct pciide_softc),
    geodeide_match, geodeide_attach, NULL, NULL);

static const struct pciide_product_desc pciide_geode_products[] = {
	{ PCI_PRODUCT_CYRIX_CX5530_IDE,
	  0,
	  "AMD Geode CX5530 IDE controller",
	  geodeide_chip_map,
	},
	{ 0,
	  0,
	  NULL,
	  NULL,
	},
};

static int
geodeide_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct pci_attach_args *pa = aux;

	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_CYRIX &&
	    PCI_CLASS(pa->pa_class) == PCI_CLASS_MASS_STORAGE &&
	    PCI_SUBCLASS(pa->pa_class) == PCI_SUBCLASS_MASS_STORAGE_IDE &&
	    pciide_lookup_product(pa->pa_id, pciide_geode_products)) 
		return(2);
	return (0);
}

static void
geodeide_attach(struct device *parent, struct device *self, void *aux)
{
	struct pci_attach_args *pa = aux;
	struct pciide_softc *sc = (void *)self;

	pciide_common_attach(sc, pa,
	    pciide_lookup_product(pa->pa_id, pciide_geode_products));
}

static void
geodeide_chip_map(struct pciide_softc *sc, struct pci_attach_args *pa)
{
	struct pciide_channel *cp;
	int channel;
	bus_size_t cmdsize, ctlsize;

	if (pciide_chipen(sc, pa) == 0)
		return;

	aprint_normal("%s: bus-master DMA support present",
	    sc->sc_wdcdev.sc_dev.dv_xname);
	pciide_mapreg_dma(sc, pa);
	aprint_normal("\n");
	if (sc->sc_dma_ok) {
		sc->sc_wdcdev.cap = WDC_CAPABILITY_DMA | WDC_CAPABILITY_UDMA |
		    WDC_CAPABILITY_IRQACK;
		sc->sc_wdcdev.irqack = pciide_irqack;
	}
	sc->sc_wdcdev.PIO_cap = 4;
	sc->sc_wdcdev.DMA_cap = 2;
	sc->sc_wdcdev.UDMA_cap = 2;
	sc->sc_wdcdev.set_modes = geodeide_setup_channel;
	sc->sc_wdcdev.channels = sc->wdc_chanarray;
	sc->sc_wdcdev.nchannels = PCIIDE_NUM_CHANNELS;
	sc->sc_wdcdev.cap |= WDC_CAPABILITY_DATA16 | WDC_CAPABILITY_DATA32 |
	    WDC_CAPABILITY_MODE;

	for (channel = 0; channel < sc->sc_wdcdev.nchannels; channel++) {
		cp = &sc->pciide_channels[channel];
		/* controller is compat-only */
		if (pciide_chansetup(sc, channel, 0) == 0)
			continue;
		pciide_mapchan(pa, cp, 0, &cmdsize, &ctlsize, pciide_pci_intr);
	}
}

static void
geodeide_setup_channel(struct wdc_channel *chp)
{
	struct ata_drive_datas *drvp;
	struct pciide_channel *cp = (struct pciide_channel*)chp;
	struct pciide_softc *sc = (struct pciide_softc *)cp->wdc_channel.ch_wdc;
	int channel = chp->ch_channel;
	int drive;
	u_int32_t dma_timing;
	u_int8_t idedma_ctl;

	/* setup DMA if needed */
	pciide_channel_dma_setup(cp);

	idedma_ctl = 0;

	/* Per drive settings */
	for (drive = 0; drive < 2; drive++) {
		drvp = &chp->ch_drive[drive];
		/* If no drive, skip */
		if ((drvp->drive_flags & DRIVE) == 0)
			continue;
		dma_timing = DMA_REG_PIO_FORMAT;
		/* add timing values, setup DMA if needed */
		if (drvp->drive_flags & DRIVE_UDMA) {
			/* Use Ultra-DMA */
			dma_timing |= geode_udma[drvp->UDMA_mode];
			idedma_ctl |= IDEDMA_CTL_DRV_DMA(drive);
		} else if (drvp->drive_flags & DRIVE_DMA) {
			/* use Multiword DMA */
			dma_timing |= geode_dma[drvp->DMA_mode];
			idedma_ctl |= IDEDMA_CTL_DRV_DMA(drive);
		} else {
			/* PIO only */
			drvp->drive_flags &= ~(DRIVE_UDMA | DRIVE_DMA);
		}
		bus_space_write_4(sc->sc_dma_iot, sc->sc_dma_ioh,
		    DMA_REG(channel, drive), dma_timing);
		bus_space_write_4(sc->sc_dma_iot, sc->sc_dma_ioh,
		    PIO_REG(channel, drive), geode_pio[drvp->PIO_mode]);
	}

	if (idedma_ctl != 0) {
		/* Add software bits in status register */
		bus_space_write_1(sc->sc_dma_iot, cp->dma_iohs[IDEDMA_CTL], 0,
		    idedma_ctl);
	}
}
