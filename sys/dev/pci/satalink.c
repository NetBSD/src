/*	$NetBSD: satalink.c,v 1.4 2003/12/19 03:33:52 thorpej Exp $	*/

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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>
#include <dev/pci/pciidereg.h>
#include <dev/pci/pciidevar.h>
#include <dev/pci/pciide_sii3112_reg.h>

#include <dev/ata/satareg.h>

/*
 * Register map for BA5 register space, indexed by channel.
 */
static const struct {
	bus_addr_t	ba5_IDEDMA_CMD;
	bus_addr_t	ba5_IDEDMA_CTL;
	bus_addr_t	ba5_IDEDMA_TBL;
	bus_addr_t	ba5_IDEDMA_CMD2;
	bus_addr_t	ba5_IDEDMA_CTL2;
	bus_addr_t	ba5_IDE_TF0;
	bus_addr_t	ba5_IDE_TF1;
	bus_addr_t	ba5_IDE_TF2;
	bus_addr_t	ba5_IDE_TF3;
	bus_addr_t	ba5_IDE_TF4;
	bus_addr_t	ba5_IDE_TF5;
	bus_addr_t	ba5_IDE_TF6;
	bus_addr_t	ba5_IDE_TF7;
	bus_addr_t	ba5_IDE_TF8;
	bus_addr_t	ba5_IDE_RAD;
	bus_addr_t	ba5_IDE_TF9;
	bus_addr_t	ba5_IDE_TF10;
	bus_addr_t	ba5_IDE_TF11;
	bus_addr_t	ba5_IDE_TF12;
	bus_addr_t	ba5_IDE_TF13;
	bus_addr_t	ba5_IDE_TF14;
	bus_addr_t	ba5_IDE_TF15;
	bus_addr_t	ba5_IDE_TF16;
	bus_addr_t	ba5_IDE_TF17;
	bus_addr_t	ba5_IDE_TF18;
	bus_addr_t	ba5_IDE_TF19;
	bus_addr_t	ba5_IDE_RABC;
	bus_addr_t	ba5_IDE_CMD_STS;
	bus_addr_t	ba5_IDE_CFG_STS;
	bus_addr_t	ba5_IDE_DTM;
	bus_addr_t	ba5_SControl;
	bus_addr_t	ba5_SStatus;
	bus_addr_t	ba5_SError;
} satalink_ba5_regmap[] = {
	{
		.ba5_IDEDMA_CMD		=	0x000,
		.ba5_IDEDMA_CTL		=	0x002,
		.ba5_IDEDMA_TBL		=	0x004,
		.ba5_IDEDMA_CMD2	=	0x010,
		.ba5_IDEDMA_CTL2	=	0x012,
		.ba5_IDE_TF0		=	0x080,	/* wd_data */
		.ba5_IDE_TF1		=	0x081,	/* wd_error */
		.ba5_IDE_TF2		=	0x082,	/* wd_seccnt */
		.ba5_IDE_TF3		=	0x083,	/* wd_sector */
		.ba5_IDE_TF4		=	0x084,	/* wd_cyl_lo */
		.ba5_IDE_TF5		=	0x085,	/* wd_cyl_hi */
		.ba5_IDE_TF6		=	0x086,	/* wd_sdh */
		.ba5_IDE_TF7		=	0x087,	/* wd_command */
		.ba5_IDE_TF8		=	0x08a,	/* wd_altsts */
		.ba5_IDE_RAD		=	0x08c,
		.ba5_IDE_TF9		=	0x091,	/* Features 2 */
		.ba5_IDE_TF10		=	0x092,	/* Sector Count 2 */
		.ba5_IDE_TF11		=	0x093,	/* Start Sector 2 */
		.ba5_IDE_TF12		=	0x094,	/* Cylinder Low 2 */
		.ba5_IDE_TF13		=	0x095,	/* Cylinder High 2 */
		.ba5_IDE_TF14		=	0x096,	/* Device/Head 2 */
		.ba5_IDE_TF15		=	0x097,	/* Cmd Sts 2 */
		.ba5_IDE_TF16		=	0x098,	/* Sector Count 2 ext */
		.ba5_IDE_TF17		=	0x099,	/* Start Sector 2 ext */
		.ba5_IDE_TF18		=	0x09a,	/* Cyl Low 2 ext */
		.ba5_IDE_TF19		=	0x09b,	/* Cyl High 2 ext */
		.ba5_IDE_RABC		=	0x09c,
		.ba5_IDE_CMD_STS	=	0x0a0,
		.ba5_IDE_CFG_STS	=	0x0a1,
		.ba5_IDE_DTM		=	0x0b4,
		.ba5_SControl		=	0x100,
		.ba5_SStatus		=	0x104,
		.ba5_SError		=	0x108,
	},
	{
		.ba5_IDEDMA_CMD		=	0x008,
		.ba5_IDEDMA_CTL		=	0x00a,
		.ba5_IDEDMA_TBL		=	0x00c,
		.ba5_IDEDMA_CMD2	=	0x018,
		.ba5_IDEDMA_CTL2	=	0x01a,
		.ba5_IDE_TF0		=	0x0c0,	/* wd_data */
		.ba5_IDE_TF1		=	0x0c1,	/* wd_error */
		.ba5_IDE_TF2		=	0x0c2,	/* wd_seccnt */
		.ba5_IDE_TF3		=	0x0c3,	/* wd_sector */
		.ba5_IDE_TF4		=	0x0c4,	/* wd_cyl_lo */
		.ba5_IDE_TF5		=	0x0c5,	/* wd_cyl_hi */
		.ba5_IDE_TF6		=	0x0c6,	/* wd_sdh */
		.ba5_IDE_TF7		=	0x0c7,	/* wd_command */
		.ba5_IDE_TF8		=	0x0ca,	/* wd_altsts */
		.ba5_IDE_RAD		=	0x0cc,
		.ba5_IDE_TF9		=	0x0d1,	/* Features 2 */
		.ba5_IDE_TF10		=	0x0d2,	/* Sector Count 2 */
		.ba5_IDE_TF11		=	0x0d3,	/* Start Sector 2 */
		.ba5_IDE_TF12		=	0x0d4,	/* Cylinder Low 2 */
		.ba5_IDE_TF13		=	0x0d5,	/* Cylinder High 2 */
		.ba5_IDE_TF14		=	0x0d6,	/* Device/Head 2 */
		.ba5_IDE_TF15		=	0x0d7,	/* Cmd Sts 2 */
		.ba5_IDE_TF16		=	0x0d8,	/* Sector Count 2 ext */
		.ba5_IDE_TF17		=	0x0d9,	/* Start Sector 2 ext */
		.ba5_IDE_TF18		=	0x0da,	/* Cyl Low 2 ext */
		.ba5_IDE_TF19		=	0x0db,	/* Cyl High 2 ext */
		.ba5_IDE_RABC		=	0x0dc,
		.ba5_IDE_CMD_STS	=	0x0e0,
		.ba5_IDE_CFG_STS	=	0x0e1,
		.ba5_IDE_DTM		=	0x0f4,
		.ba5_SControl		=	0x180,
		.ba5_SStatus		=	0x184,
		.ba5_SError		=	0x188,
	}
};

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

#define	BA5_READ_4(sc, chan, reg)					\
	ba5_read_4((sc), satalink_ba5_regmap[(chan)].reg)

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

#define	BA5_WRITE_4(sc, chan, reg, val)					\
	ba5_write_4((sc), satalink_ba5_regmap[(chan)].reg, (val))

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

	BA5_WRITE_4(sc, chp->channel, ba5_SControl, scontrol);
	delay(500);
	scontrol &= ~SControl_DET_INIT;
	BA5_WRITE_4(sc, chp->channel, ba5_SControl, scontrol);
	delay(500);

	sstatus = BA5_READ_4(sc, chp->channel, ba5_SStatus);
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
