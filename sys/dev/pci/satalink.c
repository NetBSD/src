/*	$NetBSD: satalink.c,v 1.2 2003/12/15 00:36:23 thorpej Exp $	*/

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of Wasabi Systems, Inc.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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
#include <sys/malloc.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>
#include <dev/pci/pciidereg.h>
#include <dev/pci/pciidevar.h>
#include <dev/pci/pciide_sii3112_reg.h>

#include <dev/ata/satareg.h>

static int  satalink_match(struct device *, struct cfdata *, void *);
static void satalink_attach(struct device *, struct device *, void *);

CFATTACH_DECL(satalink, sizeof(struct pciide_softc),
    satalink_match, satalink_attach, NULL, NULL);

static void sii3112_chip_map(struct pciide_softc*, struct pci_attach_args*);
static int  sii3112_drv_probe(struct channel_softc*);
static void sii3112_setup_channel(struct channel_softc*);

static const struct pciide_product_desc pciide_satalink_products[] =  {
	{ PCI_PRODUCT_CMDTECH_3112,
	  0,
	  "Silicon Image SATALink 3112",
	  sii3112_chip_map,
	},
	{ 0,
	  0,
	  NULL,
	  NULL
	}
};

static int
satalink_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct pci_attach_args *pa = aux;

	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_CMDTECH) {
		if (pciide_lookup_product(pa->pa_id, pciide_satalink_products))
			return (2);
	}
	return (0);
}

static void
satalink_attach(struct device *parent, struct device *self, void *aux)
{
	struct pci_attach_args *pa = aux;
	struct pciide_softc *sc = (struct pciide_softc *)self;

	pciide_common_attach(sc, pa,
	    pciide_lookup_product(pa->pa_id, pciide_satalink_products));

}

static __inline uint32_t
ba5_read_4(struct pciide_softc *sc, bus_addr_t reg)
{

	if (__predict_true(sc->sc_ba5_en != 0))
		return (bus_space_read_4(sc->sc_ba5_st, sc->sc_ba5_sh, reg));

	pci_conf_write(sc->sc_pc, sc->sc_tag, SII3112_BA5_IND_ADDR, reg);
	return (pci_conf_read(sc->sc_pc, sc->sc_tag, SII3112_BA5_IND_DATA));
}

static __inline void
ba5_write_4(struct pciide_softc *sc, bus_addr_t reg, uint32_t val)
{

	if (__predict_true(sc->sc_ba5_en != 0))
		bus_space_write_4(sc->sc_ba5_st, sc->sc_ba5_sh, reg, val);
	else {
		pci_conf_write(sc->sc_pc, sc->sc_tag, SII3112_BA5_IND_ADDR,
			       reg);
		pci_conf_write(sc->sc_pc, sc->sc_tag, SII3112_BA5_IND_DATA,
			       val);
	}
}

static void
sii3112_chip_map(struct pciide_softc *sc, struct pci_attach_args *pa)
{
	struct pciide_channel *cp;
	bus_size_t cmdsize, ctlsize;
	pcireg_t interface, scs_cmd, cfgctl;
	int channel;

	if (pciide_chipen(sc, pa) == 0)
		return;

	scs_cmd = pci_conf_read(pa->pa_pc, pa->pa_tag, SII3112_SCS_CMD);
	pci_conf_write(pa->pa_pc, pa->pa_tag, SII3112_SCS_CMD,
		       scs_cmd & SCS_CMD_BA5_EN);

	if (scs_cmd & SCS_CMD_BA5_EN) {
		aprint_verbose("%s: SATALink BA5 register space enabled\n",
		    sc->sc_wdcdev.sc_dev.dv_xname);
		if (pci_mapreg_map(pa, PCI_MAPREG_START + 0x14,
				   PCI_MAPREG_TYPE_MEM|
				   PCI_MAPREG_MEM_TYPE_32BIT, 0,
				   &sc->sc_ba5_st, &sc->sc_ba5_sh,
				   NULL, NULL) != 0)
			aprint_error("%s: unable to map SATALink BA5 "
			    "register space\n", sc->sc_wdcdev.sc_dev.dv_xname);
		else
			sc->sc_ba5_en = 1;
	} else {
		aprint_verbose("%s: SATALink BA5 register space disabled\n",
		    sc->sc_wdcdev.sc_dev.dv_xname);

		/* Enable indirect BA5 addressing. */
		cfgctl = pci_conf_read(pa->pa_pc, pa->pa_tag,
				       SII3112_PCI_CFGCTL);
		pci_conf_write(pa->pa_pc, pa->pa_tag, SII3112_PCI_CFGCTL,
			       cfgctl | CFGCTL_BA5INDEN);
	}

	aprint_normal("%s: bus-master DMA support present",
	    sc->sc_wdcdev.sc_dev.dv_xname);
	pciide_mapreg_dma(sc, pa);
	aprint_normal("\n");

	/*
	 * Rev. <= 0x01 of the 3112 have a bug that can cause data
	 * corruption if DMA transfers cross an 8K boundary.  This is
	 * apparently hard to tickle, but we'll go ahead and play it
	 * safe.
	 */
	if (PCI_REVISION(pa->pa_class) <= 0x01) {
		sc->sc_dma_maxsegsz = 8192;
		sc->sc_dma_boundary = 8192;
	}

	sc->sc_wdcdev.cap |= WDC_CAPABILITY_DATA16 | WDC_CAPABILITY_DATA32 |
	    WDC_CAPABILITY_MODE;
	sc->sc_wdcdev.PIO_cap = 4;
	if (sc->sc_dma_ok) {
		sc->sc_wdcdev.cap |= WDC_CAPABILITY_DMA | WDC_CAPABILITY_UDMA;
		sc->sc_wdcdev.cap |= WDC_CAPABILITY_IRQACK;
		sc->sc_wdcdev.irqack = pciide_irqack;
		sc->sc_wdcdev.DMA_cap = 2;
		sc->sc_wdcdev.UDMA_cap = 6;
	}
	sc->sc_wdcdev.set_modes = sii3112_setup_channel;

	/* We can use SControl and SStatus to probe for drives. */
	sc->sc_wdcdev.cap |= WDC_CAPABILITY_DRVPROBE;
	sc->sc_wdcdev.drv_probe = sii3112_drv_probe;

	sc->sc_wdcdev.channels = sc->wdc_chanarray;
	sc->sc_wdcdev.nchannels = PCIIDE_NUM_CHANNELS;

	/* 
	 * The 3112 either identifies itself as a RAID storage device
	 * or a Misc storage device.  Fake up the interface bits for
	 * what our driver expects.
	 */
	if (PCI_SUBCLASS(pa->pa_class) == PCI_SUBCLASS_MASS_STORAGE_IDE) {
		interface = PCI_INTERFACE(pa->pa_class);
	} else {
		interface = PCIIDE_INTERFACE_BUS_MASTER_DMA |
		    PCIIDE_INTERFACE_PCI(0) | PCIIDE_INTERFACE_PCI(1);
	}

	for (channel = 0; channel < sc->sc_wdcdev.nchannels; channel++) {
		cp = &sc->pciide_channels[channel];
		if (pciide_chansetup(sc, channel, interface) == 0)
			continue;
		pciide_mapchan(pa, cp, interface, &cmdsize, &ctlsize,
		    pciide_pci_intr);
	}
}

static const char *sata_speed[] = {
	"no negotiated speed",
	"1.5Gb/s",
	"<unknown 2>",
	"<unknown 3>",
	"<unknown 4>",
	"<unknown 5>",
	"<unknown 6>",
	"<unknown 7>",
	"<unknown 8>",
	"<unknown 9>",
	"<unknown 10>",
	"<unknown 11>",
	"<unknown 12>",
	"<unknown 13>",
	"<unknown 14>",
	"<unknown 15>",
};

static int
sii3112_drv_probe(struct channel_softc *chp)
{
	struct pciide_channel *cp = (struct pciide_channel *)chp;
	struct pciide_softc *sc = (struct pciide_softc *)cp->wdc_channel.wdc;
	uint32_t scontrol, sstatus;
	int rv = 0;
	uint8_t scnt, sn, cl, ch;

	/*
	 * The 3112 is a 2-port part, and only has one drive per channel
	 * (each port emulates a master drive).
	 */

	/*
	 * Request communication initialization sequence, any speed.
	 * Performing this is the equivalent of an ATA Reset.
	 */
	scontrol = SControl_DET_INIT | SControl_SPD_ANY;

	/*
	 * XXX We don't yet support SATA power management; disable all
	 * power management state transitions.
	 */
	scontrol |= SControl_IPM_NONE;

	ba5_write_4(sc, chp->channel == 0 ? 0x100 : 0x180, scontrol);
	delay(500);
	scontrol &= ~SControl_DET_INIT;
	ba5_write_4(sc, chp->channel == 0 ? 0x100 : 0x180, scontrol);
	delay(500);

	sstatus = ba5_read_4(sc, chp->channel == 0 ? 0x104 : 0x184);
	switch (sstatus & SStatus_DET_mask) {
	case SStatus_DET_NODEV:
		/* No device; be silent. */
		break;

	case SStatus_DET_DEV_NE:
		aprint_error("%s: port %d: device connected, but "
		    "communication not established\n",
		    sc->sc_wdcdev.sc_dev.dv_xname, chp->channel);
		break;

	case SStatus_DET_OFFLINE:
		aprint_error("%s: port %d: PHY offline\n",
		    sc->sc_wdcdev.sc_dev.dv_xname, chp->channel);
		break;

	case SStatus_DET_DEV:
		/*
		 * XXX ATAPI detection doesn't currently work.  Don't
		 * XXX know why.  But, it's not like the standard method
		 * XXX can detect an ATAPI device connected via a SATA/PATA
		 * XXX bridge, so at least this is no worse.  --thorpej
		 */
		bus_space_write_1(chp->cmd_iot, chp->cmd_iohs[wd_sdh], 0,
		    WDSD_IBM | (0 << 4));
		delay(10);	/* 400ns delay */
		/* Save register contents. */
		scnt = bus_space_read_1(chp->cmd_iot,
				        chp->cmd_iohs[wd_seccnt], 0);
		sn = bus_space_read_1(chp->cmd_iot,
				      chp->cmd_iohs[wd_sector], 0);
		cl = bus_space_read_1(chp->cmd_iot,
				      chp->cmd_iohs[wd_cyl_lo], 0);
		ch = bus_space_read_1(chp->cmd_iot,
				      chp->cmd_iohs[wd_cyl_hi], 0);
#if 0
		printf("%s: port %d: scnt=0x%x sn=0x%x cl=0x%x ch=0x%x\n",
		    sc->sc_wdcdev.sc_dev.dv_xname, chp->channel,
		    scnt, sn, cl, ch);
#endif
		/*
		 * scnt and sn are supposed to be 0x1 for ATAPI, but in some
		 * cases we get wrong values here, so ignore it.
		 */
		if (cl == 0x14 && ch == 0xeb)
			chp->ch_drive[0].drive_flags |= DRIVE_ATAPI;
		else
			chp->ch_drive[0].drive_flags |= DRIVE_ATA;

		aprint_normal("%s: port %d: device present, speed: %s\n",
		    sc->sc_wdcdev.sc_dev.dv_xname, chp->channel,
		    sata_speed[(sstatus & SStatus_SPD_mask) >> 
			       SStatus_SPD_shift]);
		rv |= (1 << 0);
		break;

	default:
		aprint_error("%s: port %d: unknown SStatus: 0x%08x\n",
		    sc->sc_wdcdev.sc_dev.dv_xname, chp->channel, sstatus);
	}

	return (rv);
}

static void
sii3112_setup_channel(struct channel_softc *chp)
{
	struct ata_drive_datas *drvp;
	int drive;
	u_int32_t idedma_ctl, dtm;
	struct pciide_channel *cp = (struct pciide_channel*)chp;
	struct pciide_softc *sc = (struct pciide_softc*)cp->wdc_channel.wdc;

	/* setup DMA if needed */
	pciide_channel_dma_setup(cp);

	idedma_ctl = 0;
	dtm = 0;

	for (drive = 0; drive < 2; drive++) {
		drvp = &chp->ch_drive[drive];
		/* If no drive, skip */
		if ((drvp->drive_flags & DRIVE) == 0)
			continue;
		if (drvp->drive_flags & DRIVE_UDMA) {
			/* use Ultra/DMA */
			drvp->drive_flags &= ~DRIVE_DMA;
			idedma_ctl |= IDEDMA_CTL_DRV_DMA(drive);
			dtm |= DTM_IDEx_DMA;
		} else if (drvp->drive_flags & DRIVE_DMA) {
			idedma_ctl |= IDEDMA_CTL_DRV_DMA(drive);
			dtm |= DTM_IDEx_DMA;
		} else {
			dtm |= DTM_IDEx_PIO;
		}
	}

	/*
	 * Nothing to do to setup modes; it is meaningless in S-ATA
	 * (but many S-ATA drives still want to get the SET_FEATURE
	 * command).
	 */
	if (idedma_ctl != 0) {
		/* Add software bits in status register */
		bus_space_write_1(sc->sc_dma_iot, cp->dma_iohs[IDEDMA_CTL], 0,
		    idedma_ctl);
	}
	pci_conf_write(sc->sc_pc, sc->sc_tag,
	    chp->channel == 0 ? SII3112_DTM_IDE0 : SII3112_DTM_IDE1, dtm);
}
