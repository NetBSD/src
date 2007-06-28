/*	$NetBSD: gcscide.c,v 1.4 2007/06/28 10:22:52 xtraeme Exp $	*/

/*
 * Copyright (c) 2007 The NetBSD Foundation.
 * All rights reserved.
 *
 * This code is derived from software contributed to the NetBSD Foundation
 * by Juan Romero Pardines.
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
 *        This product includes software developed by Juan Romero Pardines
 *        for the NetBSD Foundation, Inc. and its contributors.
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
__KERNEL_RCSID(0, "$NetBSD: gcscide.c,v 1.4 2007/06/28 10:22:52 xtraeme Exp $");

#include <sys/param.h>
#include <sys/systm.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>
#include <dev/pci/pciidereg.h>
#include <dev/pci/pciidevar.h>

#include <machine/cpufunc.h>

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
#define GCSCIDE_PIO_FORMAT		0x80000000UL

static int	gcscide_match(struct device *, struct cfdata *, void *);
static void	gcscide_attach(struct device *, struct device *, void *);

static void	gcscide_chip_map(struct pciide_softc *, struct pci_attach_args *);
static void	gcscide_setup_channel(struct ata_channel *);

/* PIO Format 1 timings */
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

CFATTACH_DECL(gcscide, sizeof(struct pciide_softc),
    gcscide_match, gcscide_attach, NULL, NULL);

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
gcscide_match(struct device *parent, struct cfdata *cfdata, void *aux)
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
gcscide_attach(struct device *parent, struct device *self, void *aux)
{
	struct pci_attach_args *pa = aux;
	struct pciide_softc *sc = (struct pciide_softc *)self;

	pciide_common_attach(sc, pa,
	    pciide_lookup_product(pa->pa_id, pciide_gcscide_products));
}

static void
gcscide_chip_map(struct pciide_softc *sc, struct pci_attach_args *pa)
{
	pcireg_t interface;
	bus_size_t cmdsize, ctlsize;

	if (pciide_chipen(sc, pa) == 0)
		return;

	aprint_verbose("%s: bus-master DMA support present",
	    sc->sc_wdcdev.sc_atac.atac_dev.dv_xname);
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
	    &cmdsize, &ctlsize, pciide_pci_intr);
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
		if ((drvp->drive_flags & DRIVE) == 0)
			continue;
		if ((drvp->drive_flags & DRIVE_UDMA) ||
		    (drvp->drive_flags & DRIVE_DMA)) {
			reg = rdmsr(drive ? GCSCIDE_ATAC_CH0D1_DMA :
			    GCSCIDE_ATAC_CH0D0_DMA);
			/* Preserve PIO Format bit */
			reg &= ~GCSCIDE_PIO_FORMAT;
		}

		if (drvp->drive_flags & DRIVE_UDMA) {
			s = splbio();
			drvp->drive_flags &= ~DRIVE_DMA;
			splx(s);
			/* Set UDMA and MDMA timings */
			reg |= gcscide_udma_timings[drvp->UDMA_mode];
			reg |= gcscide_mdma_timings[drvp->DMA_mode];

			wrmsr(drive ? GCSCIDE_ATAC_CH0D1_DMA :
			    GCSCIDE_ATAC_CH0D0_DMA, reg);

		} else if (drvp->drive_flags & DRIVE_DMA) {
			/*
			 * Disable Ultra DMA and set a MDMA mode.
			 */
			if (reg & gcscide_udma_timings[drvp->UDMA_mode])
				reg &= ~gcscide_udma_timings[drvp->UDMA_mode];

			reg |= gcscide_mdma_timings[drvp->DMA_mode];

			wrmsr(drive ? GCSCIDE_ATAC_CH0D1_DMA :
			    GCSCIDE_ATAC_CH0D0_DMA, reg);
		} else {
			/*
			 * Set PIO Format 1 timings.
			 */
			reg = rdmsr(drive ? GCSCIDE_ATAC_CH0D1_DMA :
		    	    GCSCIDE_ATAC_CH0D0_DMA);
			wrmsr(drive ? GCSCIDE_ATAC_CH0D1_DMA :
		    	    GCSCIDE_ATAC_CH0D0_DMA, reg | GCSCIDE_PIO_FORMAT);
		}
		/* Set PIO mode and timing */
		wrmsr(drive ? GCSCIDE_ATAC_CH0D1_PIO : GCSCIDE_ATAC_CH0D0_PIO,
		    gcscide_pio_timings[drvp->PIO_mode]);
	}
}
