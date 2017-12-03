/*	$NetBSD: gcscide.c,v 1.14.2.2 2017/12/03 11:36:18 jdolecek Exp $	*/

/*-
 * Copyright (c) 2007 Juan Romero Pardines.
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Driver for the IDE Controller of the National Semiconductor/AMD
 * CS5535 Companion device. Available on systems with an AMD Geode GX2
 * CPU, for example the decTOP.
 *
 * Datasheet at:
 *
 * http://www.amd.com/files/connectivitysolutions/geode/geode_gx/31506_cs5535_databook.pdf
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: gcscide.c,v 1.14.2.2 2017/12/03 11:36:18 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>
#include <dev/pci/pciidereg.h>
#include <dev/pci/pciidevar.h>

#include <machine/cpufunc.h>

/*
 * 6.4 - ATA-5 Controller Register Definitions.
 */
#define GCSCIDE_MSR_ATAC_BASE 		0x51300000
#define GCSCIDE_ATAC_GLD_MSR_CAP 	(GCSCIDE_MSR_ATAC_BASE + 0)
#define GCSCIDE_ATAC_GLD_MSR_CONFIG 	(GCSCIDE_MSR_ATAC_BASE + 0x01)
#define GCSCIDE_ATAC_GLD_MSR_SMI 	(GCSCIDE_MSR_ATAC_BASE + 0x02)
#define GCSCIDE_ATAC_GLD_MSR_ERROR 	(GCSCIDE_MSR_ATAC_BASE + 0x03)
#define GCSCIDE_ATAC_GLD_MSR_PM 	(GCSCIDE_MSR_ATAC_BASE + 0x04)
#define GCSCIDE_ATAC_GLD_MSR_DIAG 	(GCSCIDE_MSR_ATAC_BASE + 0x05)
#define GCSCIDE_ATAC_IO_BAR 		(GCSCIDE_MSR_ATAC_BASE + 0x08)
#define GCSCIDE_ATAC_RESET 		(GCSCIDE_MSR_ATAC_BASE + 0x10)
#define GCSCIDE_ATAC_CH0D0_PIO 		(GCSCIDE_MSR_ATAC_BASE + 0x20)
#define GCSCIDE_ATAC_CH0D0_DMA 		(GCSCIDE_MSR_ATAC_BASE + 0x21)
#define GCSCIDE_ATAC_CH0D1_PIO 		(GCSCIDE_MSR_ATAC_BASE + 0x22)
#define GCSCIDE_ATAC_CH0D1_DMA 		(GCSCIDE_MSR_ATAC_BASE + 0x23)
#define GCSCIDE_ATAC_PCI_ABRTERR 	(GCSCIDE_MSR_ATAC_BASE + 0x24)
#define GCSCIDE_ATAC_BM0_CMD_PRIM 	0x00
#define GCSCIDE_ATAC_BM0_STS_PRIM 	0x02
#define GCSCIDE_ATAC_BM0_PRD 		0x04
/*
 * ATAC_CH0D0_DMA registers:
 *
 * PIO Format (bit 31): Format 1 allows independent control of command
 * and data per drive, while Format 0 selects the slowest speed
 * of the two drives.
 */
#define GCSCIDE_ATAC_PIO_FORMAT		(1 << 31) /* PIO Mode Format 1 */
/*
 * DMA_SEL (bit 20): sets Ultra DMA mode (if enabled) or Multi-word
 * DMA mode (if disabled).
 */
#define GCSCIDE_ATAC_DMA_SEL		(1 << 20)

static int	gcscide_match(device_t, cfdata_t, void *);
static void	gcscide_attach(device_t, device_t, void *);

static void	gcscide_chip_map(struct pciide_softc *,
    const struct pci_attach_args *);
static void	gcscide_setup_channel(struct ata_channel *);

/* PIO Format 1 settings */
static const uint32_t gcscide_pio_timings[] = {
	0xf7f4f7f4,	/* PIO Mode 0 */
	0x53f3f173,	/* PIO Mode 1 */
	0x13f18141,	/* PIO Mode 2 */
	0x51315131,	/* PIO Mode 3 */
	0x11311131	/* PIO Mode 4 */
};

static const uint32_t gcscide_mdma_timings[] = {
	0x7f0ffff3,	/* MDMA Mode 0 */
	0x7f035352,	/* MDMA Mode 1 */
	0x7f024241 	/* MDMA Mode 2 */
};

static const uint32_t gcscide_udma_timings[] = {
	0x7f7436a1,	/* Ultra DMA Mode 0 */
	0x7f733481,	/* Ultra DMA Mode 1 */
	0x7f723261,	/* Ultra DMA Mode 2 */
	0x7f713161,	/* Ultra DMA Mode 3 */
	0x7f703061	/* Ultra DMA Mode 4 */
};

CFATTACH_DECL_NEW(gcscide, sizeof(struct pciide_softc),
    gcscide_match, gcscide_attach, pciide_detach, NULL);

static const struct pciide_product_desc pciide_gcscide_products[] = {
	{
		PCI_PRODUCT_NS_CS5535_IDE,
		0,
		"National Semiconductor/AMD CS5535 IDE Controller",
		gcscide_chip_map
	},
	{ 0, 0, NULL, NULL }
};

static int
gcscide_match(device_t parent, cfdata_t cfdata, void *aux)
{
	struct pci_attach_args *pa = (struct pci_attach_args *)aux;

	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_NS &&
	    PCI_CLASS(pa->pa_class) == PCI_CLASS_MASS_STORAGE &&
	    PCI_SUBCLASS(pa->pa_class) == PCI_SUBCLASS_MASS_STORAGE_IDE &&
	    pciide_lookup_product(pa->pa_id, pciide_gcscide_products))
		return 2;
	return 0;
}

static void
gcscide_attach(device_t parent, device_t self, void *aux)
{
	struct pci_attach_args *pa = aux;
	struct pciide_softc *sc = device_private(self);

	sc->sc_wdcdev.sc_atac.atac_dev = self;

	pciide_common_attach(sc, pa,
	    pciide_lookup_product(pa->pa_id, pciide_gcscide_products));
}

static void
gcscide_chip_map(struct pciide_softc *sc, const struct pci_attach_args *pa)
{
	pcireg_t interface;

	if (pciide_chipen(sc, pa) == 0)
		return;

	aprint_verbose_dev(sc->sc_wdcdev.sc_atac.atac_dev,
	    "bus-master DMA support present");
	pciide_mapreg_dma(sc, pa);
	aprint_verbose("\n");

	sc->sc_wdcdev.sc_atac.atac_cap = ATAC_CAP_DATA16 | ATAC_CAP_DATA32;
	if (sc->sc_dma_ok) {
		sc->sc_wdcdev.sc_atac.atac_cap |= ATAC_CAP_DMA | ATAC_CAP_UDMA;
		sc->sc_wdcdev.irqack = pciide_irqack;
	}

	sc->sc_wdcdev.sc_atac.atac_pio_cap = 4;
	sc->sc_wdcdev.sc_atac.atac_dma_cap = 2;
	sc->sc_wdcdev.sc_atac.atac_udma_cap = 4;
	sc->sc_wdcdev.sc_atac.atac_set_modes = gcscide_setup_channel;
	sc->sc_wdcdev.sc_atac.atac_channels = sc->wdc_chanarray;
	sc->sc_wdcdev.sc_atac.atac_nchannels = 1;

	interface = PCI_INTERFACE(pa->pa_class);

	wdc_allocate_regs(&sc->sc_wdcdev);

	if (pciide_chansetup(sc, 0, interface) == 0)
		return;

	pciide_mapchan(pa, &sc->pciide_channels[0], interface,
	    pciide_pci_intr);
}

static void
gcscide_setup_channel(struct ata_channel *chp)
{
	struct pciide_channel *cp = CHAN_TO_PCHAN(chp);
	struct ata_drive_datas *drvp;
	uint64_t reg = 0;
	int drive, s;

	pciide_channel_dma_setup(cp);

	for (drive = 0; drive < 2; drive++) {
		drvp = &chp->ch_drive[drive];
		if (drvp->drive_type == ATA_DRIVET_NONE)
			continue;

		reg = rdmsr(drive ? GCSCIDE_ATAC_CH0D1_DMA :
		    GCSCIDE_ATAC_CH0D0_DMA);

		if (drvp->drive_flags & ATA_DRIVE_UDMA) {
			s = splbio();
			drvp->drive_flags &= ~ATA_DRIVE_DMA;
			splx(s);
			/* Enable the Ultra DMA mode bit */
			reg |= GCSCIDE_ATAC_DMA_SEL;
			/* set the Ultra DMA mode */
			reg |= gcscide_udma_timings[drvp->UDMA_mode];

			wrmsr(drive ? GCSCIDE_ATAC_CH0D1_DMA :
			    GCSCIDE_ATAC_CH0D0_DMA, reg);

		} else if (drvp->drive_flags & ATA_DRIVE_DMA) {
			/* Enable the Multi-word DMA bit */
			reg &= ~GCSCIDE_ATAC_DMA_SEL;
			/* set the Multi-word DMA mode */
			reg |= gcscide_mdma_timings[drvp->DMA_mode];

			wrmsr(drive ? GCSCIDE_ATAC_CH0D1_DMA :
			    GCSCIDE_ATAC_CH0D0_DMA, reg);
		}

		/* Always use PIO Format 1. */
		wrmsr(drive ? GCSCIDE_ATAC_CH0D1_DMA :
		    GCSCIDE_ATAC_CH0D0_DMA, reg | GCSCIDE_ATAC_PIO_FORMAT);

		/* Set PIO mode */
		wrmsr(drive ? GCSCIDE_ATAC_CH0D1_PIO : GCSCIDE_ATAC_CH0D0_PIO,
		    gcscide_pio_timings[drvp->PIO_mode]);
	}
}
