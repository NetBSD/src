/*	$NetBSD: hptide.c,v 1.8 2004/01/03 01:50:53 thorpej Exp $	*/

/*
 * Copyright (c) 1999, 2000, 2001 Manuel Bouyer.
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
 */

#include <sys/param.h>
#include <sys/systm.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>
#include <dev/pci/pciidereg.h>
#include <dev/pci/pciidevar.h>
#include <dev/pci/pciide_hpt_reg.h>

static void hpt_chip_map(struct pciide_softc*, struct pci_attach_args*);
static void hpt_setup_channel(struct wdc_channel*);
static int  hpt_pci_intr(void *);

static int  hptide_match(struct device *, struct cfdata *, void *);
static void hptide_attach(struct device *, struct device *, void *);

CFATTACH_DECL(hptide, sizeof(struct pciide_softc),
    hptide_match, hptide_attach, NULL, NULL);

static const struct pciide_product_desc pciide_triones_products[] =  {
	{ PCI_PRODUCT_TRIONES_HPT302,
	  0,
	  NULL,
	  hpt_chip_map
	},
	{ PCI_PRODUCT_TRIONES_HPT366,
	  0,
	  NULL,
	  hpt_chip_map,
	},
	{ PCI_PRODUCT_TRIONES_HPT371,
	  0,
	  NULL,
	  hpt_chip_map,
	},
	{ PCI_PRODUCT_TRIONES_HPT372A,
	  0,
	  NULL,
	  hpt_chip_map
	},
	{ PCI_PRODUCT_TRIONES_HPT374,
	  0,
	  NULL,
	  hpt_chip_map
	},
	{ 0,
	  0,
	  NULL,
	  NULL
	}
};

static int
hptide_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct pci_attach_args *pa = aux;

	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_TRIONES) {
		if (pciide_lookup_product(pa->pa_id, pciide_triones_products))
			return (2);
	}
	return (0);
}

static void
hptide_attach(struct device *parent, struct device *self, void *aux)
{
	struct pci_attach_args *pa = aux;
	struct pciide_softc *sc = (struct pciide_softc *)self;

	pciide_common_attach(sc, pa,
	    pciide_lookup_product(pa->pa_id, pciide_triones_products));

}

static void
hpt_chip_map(struct pciide_softc *sc, struct pci_attach_args *pa)
{
	struct pciide_channel *cp;
	int i, compatchan, revision;
	pcireg_t interface;
	bus_size_t cmdsize, ctlsize;

	if (pciide_chipen(sc, pa) == 0)
		return;

	revision = PCI_REVISION(pa->pa_class);
	aprint_normal("%s: Triones/Highpoint ",
	    sc->sc_wdcdev.sc_dev.dv_xname);
	switch (sc->sc_pp->ide_product) {
	case PCI_PRODUCT_TRIONES_HPT302:
		aprint_normal("HPT302 IDE Controller\n");
		break;
	case PCI_PRODUCT_TRIONES_HPT371:
		aprint_normal("HPT371 IDE Controller\n");
		break;
	case PCI_PRODUCT_TRIONES_HPT374:
		aprint_normal("HPT374 IDE Controller\n");
		break;
	case PCI_PRODUCT_TRIONES_HPT372A:
		aprint_normal("HPT372A IDE Controller\n");
		break;
	case PCI_PRODUCT_TRIONES_HPT366:
		if (revision == HPT372_REV)
			aprint_normal("HPT372 IDE Controller\n");
		else if (revision == HPT370_REV)
			aprint_normal("HPT370 IDE Controller\n");
		else if (revision == HPT370A_REV)
			aprint_normal("HPT370A IDE Controller\n");
		else if (revision == HPT366_REV)
			aprint_normal("HPT366 IDE Controller\n");
		else
			aprint_normal("unknown HPT IDE controller rev %d\n",
			    revision);
		break;
	default:
		aprint_normal("unknown HPT IDE controller 0x%x\n",
		    sc->sc_pp->ide_product);
	}

	/* 
	 * when the chip is in native mode it identifies itself as a
	 * 'misc mass storage'. Fake interface in this case.
	 */
	if (PCI_SUBCLASS(pa->pa_class) == PCI_SUBCLASS_MASS_STORAGE_IDE) {
		interface = PCI_INTERFACE(pa->pa_class);
	} else {
		interface = PCIIDE_INTERFACE_BUS_MASTER_DMA |
		    PCIIDE_INTERFACE_PCI(0);
		if ((sc->sc_pp->ide_product == PCI_PRODUCT_TRIONES_HPT366 &&
		    (revision == HPT370_REV || revision == HPT370A_REV ||
		     revision == HPT372_REV)) ||
		    sc->sc_pp->ide_product == PCI_PRODUCT_TRIONES_HPT302 ||
		    sc->sc_pp->ide_product == PCI_PRODUCT_TRIONES_HPT371 ||
		    sc->sc_pp->ide_product == PCI_PRODUCT_TRIONES_HPT372A ||
		    sc->sc_pp->ide_product == PCI_PRODUCT_TRIONES_HPT374)
			interface |= PCIIDE_INTERFACE_PCI(1);
	}

	aprint_normal("%s: bus-master DMA support present",
	    sc->sc_wdcdev.sc_dev.dv_xname);
	pciide_mapreg_dma(sc, pa);
	aprint_normal("\n");
	sc->sc_wdcdev.cap = WDC_CAPABILITY_DATA16 | WDC_CAPABILITY_DATA32 |
	    WDC_CAPABILITY_MODE;
	if (sc->sc_dma_ok) {
		sc->sc_wdcdev.cap |= WDC_CAPABILITY_DMA | WDC_CAPABILITY_UDMA;
		sc->sc_wdcdev.cap |= WDC_CAPABILITY_IRQACK;
		sc->sc_wdcdev.irqack = pciide_irqack;
	}
	sc->sc_wdcdev.PIO_cap = 4;
	sc->sc_wdcdev.DMA_cap = 2;

	sc->sc_wdcdev.set_modes = hpt_setup_channel;
	sc->sc_wdcdev.channels = sc->wdc_chanarray;
	if (sc->sc_pp->ide_product == PCI_PRODUCT_TRIONES_HPT366 &&
	    revision == HPT366_REV) {
		sc->sc_wdcdev.nchannels = 1;
		sc->sc_wdcdev.UDMA_cap = 4;
	} else {
		sc->sc_wdcdev.nchannels = 2;
		if (sc->sc_pp->ide_product == PCI_PRODUCT_TRIONES_HPT374 ||
		    sc->sc_pp->ide_product == PCI_PRODUCT_TRIONES_HPT372A ||
		    sc->sc_pp->ide_product == PCI_PRODUCT_TRIONES_HPT371 ||
		    sc->sc_pp->ide_product == PCI_PRODUCT_TRIONES_HPT302 ||
		    (sc->sc_pp->ide_product == PCI_PRODUCT_TRIONES_HPT366 &&
		    revision == HPT372_REV))
			sc->sc_wdcdev.UDMA_cap = 6;
		else
			sc->sc_wdcdev.UDMA_cap = 5;
	}
	for (i = 0; i < sc->sc_wdcdev.nchannels; i++) {
		cp = &sc->pciide_channels[i];
		if (sc->sc_wdcdev.nchannels > 1) {
			compatchan = i;
			if((pciide_pci_read(sc->sc_pc, sc->sc_tag,
			   HPT370_CTRL1(i)) & HPT370_CTRL1_EN) == 0) {
				aprint_normal(
				    "%s: %s channel ignored (disabled)\n",
				    sc->sc_wdcdev.sc_dev.dv_xname, cp->name);
				cp->wdc_channel.ch_flags |= WDCF_DISABLED;
				continue;
			}
		} else {
			/*
			 * The 366 has 2 PCI IDE functions, one for primary and
			 * one for secondary. So we need to call
			 * pciide_mapregs_compat() with the real channel.
			 */
			if (pa->pa_function == 0)
				compatchan = 0;
			else if (pa->pa_function == 1)
				compatchan = 1;
			else {
				aprint_error("%s: unexpected PCI function %d\n",
				    sc->sc_wdcdev.sc_dev.dv_xname, pa->pa_function);
				return;
			}
		}
		if (pciide_chansetup(sc, i, interface) == 0)
			continue;
		if (interface & PCIIDE_INTERFACE_PCI(i)) {
			pciide_mapregs_native(pa, cp, &cmdsize,
			    &ctlsize, hpt_pci_intr);
		} else {
			pciide_mapregs_compat(pa, cp, compatchan,
			    &cmdsize, &ctlsize);
		}
		wdcattach(&cp->wdc_channel);
	}
	if ((sc->sc_pp->ide_product == PCI_PRODUCT_TRIONES_HPT366 &&
	    (revision == HPT370_REV || revision == HPT370A_REV ||
	     revision == HPT372_REV)) || 
	    sc->sc_pp->ide_product == PCI_PRODUCT_TRIONES_HPT302 ||
	    sc->sc_pp->ide_product == PCI_PRODUCT_TRIONES_HPT371 ||
	    sc->sc_pp->ide_product == PCI_PRODUCT_TRIONES_HPT372A ||
	    sc->sc_pp->ide_product == PCI_PRODUCT_TRIONES_HPT374) {
		/*
		 * HPT370_REV and highter has a bit to disable interrupts,
		 * make sure to clear it
		 */
		pciide_pci_write(sc->sc_pc, sc->sc_tag, HPT_CSEL,
		    pciide_pci_read(sc->sc_pc, sc->sc_tag, HPT_CSEL) &
		    ~HPT_CSEL_IRQDIS);
	}
	/* set clocks, etc (mandatory on 372/4, optional otherwise) */
	if ((sc->sc_pp->ide_product == PCI_PRODUCT_TRIONES_HPT366 &&
	     revision == HPT372_REV ) ||
	    sc->sc_pp->ide_product == PCI_PRODUCT_TRIONES_HPT302 ||
	    sc->sc_pp->ide_product == PCI_PRODUCT_TRIONES_HPT371 ||
	    sc->sc_pp->ide_product == PCI_PRODUCT_TRIONES_HPT372A ||
	    sc->sc_pp->ide_product == PCI_PRODUCT_TRIONES_HPT374)
		pciide_pci_write(sc->sc_pc, sc->sc_tag, HPT_SC2,
		    (pciide_pci_read(sc->sc_pc, sc->sc_tag, HPT_SC2) &
		     HPT_SC2_MAEN) | HPT_SC2_OSC_EN);
	return;
}

static void
hpt_setup_channel(struct wdc_channel *chp)
{
	struct ata_drive_datas *drvp;
	int drive;
	int cable;
	u_int32_t before, after;
	u_int32_t idedma_ctl;
	struct pciide_channel *cp = (struct pciide_channel*)chp;
	struct pciide_softc *sc = (struct pciide_softc *)cp->wdc_channel.wdc;
	int revision =
	     PCI_REVISION(pci_conf_read(sc->sc_pc, sc->sc_tag, PCI_CLASS_REG));
	const u_int32_t *tim_pio, *tim_dma, *tim_udma;

	cable = pciide_pci_read(sc->sc_pc, sc->sc_tag, HPT_CSEL);

	/* setup DMA if needed */
	pciide_channel_dma_setup(cp);

	idedma_ctl = 0;

	/* select the timing arrays for the chip */
	switch (sc->sc_pp->ide_product) {
	case PCI_PRODUCT_TRIONES_HPT374:
		tim_udma = hpt374_udma;
		tim_dma = hpt374_dma;
		tim_pio = hpt374_pio;
		break;
	case PCI_PRODUCT_TRIONES_HPT302:
	case PCI_PRODUCT_TRIONES_HPT371:
	case PCI_PRODUCT_TRIONES_HPT372A:
		tim_udma = hpt372_udma;
		tim_dma = hpt372_dma;
		tim_pio = hpt372_pio;
		break;
	case PCI_PRODUCT_TRIONES_HPT366:
	default:
		switch (revision) {
		case HPT372_REV:
			tim_udma = hpt372_udma;
			tim_dma = hpt372_dma;
			tim_pio = hpt372_pio;
			break;
		case HPT370_REV:
		case HPT370A_REV:
			tim_udma = hpt370_udma;
			tim_dma = hpt370_dma;
			tim_pio = hpt370_pio;
			break;
		case HPT366_REV:
		default:
			tim_udma = hpt366_udma;
			tim_dma = hpt366_dma;
			tim_pio = hpt366_pio;
			break;
		}
	}

	/* Per drive settings */
	for (drive = 0; drive < 2; drive++) {
		drvp = &chp->ch_drive[drive];
		/* If no drive, skip */
		if ((drvp->drive_flags & DRIVE) == 0)
			continue;
		before = pci_conf_read(sc->sc_pc, sc->sc_tag,
					HPT_IDETIM(chp->channel, drive));

		/* add timing values, setup DMA if needed */
		if (drvp->drive_flags & DRIVE_UDMA) {
			/* use Ultra/DMA */
			drvp->drive_flags &= ~DRIVE_DMA;
			if ((cable & HPT_CSEL_CBLID(chp->channel)) != 0 &&
			    drvp->UDMA_mode > 2)
				drvp->UDMA_mode = 2;
			after = tim_udma[drvp->UDMA_mode];
			idedma_ctl |= IDEDMA_CTL_DRV_DMA(drive);
		} else if (drvp->drive_flags & DRIVE_DMA) {
			/*
			 * use Multiword DMA.
			 * Timings will be used for both PIO and DMA, so adjust
			 * DMA mode if needed
			 */
			if (drvp->PIO_mode >= 3 &&
			    (drvp->DMA_mode + 2) > drvp->PIO_mode) {
				drvp->DMA_mode = drvp->PIO_mode - 2;
			}
			after = tim_dma[drvp->DMA_mode];
			idedma_ctl |= IDEDMA_CTL_DRV_DMA(drive);
		} else {
			/* PIO only */
			after = tim_pio[drvp->PIO_mode];
		}
		pci_conf_write(sc->sc_pc, sc->sc_tag,
		    HPT_IDETIM(chp->channel, drive), after);
		WDCDEBUG_PRINT(("%s: bus speed register set to 0x%08x "
		    "(BIOS 0x%08x)\n", drvp->drv_softc->dv_xname,
		    after, before), DEBUG_PROBE);
	}
	if (idedma_ctl != 0) {
		/* Add software bits in status register */
		bus_space_write_1(sc->sc_dma_iot, cp->dma_iohs[IDEDMA_CTL], 0,
		    idedma_ctl);
	}
}

static int
hpt_pci_intr(void *arg)
{
	struct pciide_softc *sc = arg;
	struct pciide_channel *cp;
	struct wdc_channel *wdc_cp;
	int rv = 0;
	int dmastat, i, crv;

	for (i = 0; i < sc->sc_wdcdev.nchannels; i++) {
		cp = &sc->pciide_channels[i];
		dmastat = bus_space_read_1(sc->sc_dma_iot,
		    cp->dma_iohs[IDEDMA_CTL], 0);
		if((dmastat & ( IDEDMA_CTL_ACT | IDEDMA_CTL_INTR)) !=
		    IDEDMA_CTL_INTR)
			continue;
		wdc_cp = &cp->wdc_channel;
		crv = wdcintr(wdc_cp);
		if (crv == 0) {
			printf("%s:%d: bogus intr\n",
			    sc->sc_wdcdev.sc_dev.dv_xname, i);
			bus_space_write_1(sc->sc_dma_iot,
			    cp->dma_iohs[IDEDMA_CTL], 0, dmastat);
		} else
			rv = 1;
	}
	return rv;
}
