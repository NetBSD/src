/*	$NetBSD: satalink.c,v 1.13 2004/05/07 13:01:49 sekiya Exp $	*/

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
#include <dev/ata/atareg.h>

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
	bus_addr_t	ba5_SActive;		/* 3114 */
	bus_addr_t	ba5_SMisc;
	bus_addr_t	ba5_PHY_CONFIG;
	bus_addr_t	ba5_SIEN;
	bus_addr_t	ba5_SFISCfg;
} satalink_ba5_regmap[] = {
	{	/* Channel 0 */
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
		.ba5_SActive		=	0x10c,
		.ba5_SMisc		=	0x140,
		.ba5_PHY_CONFIG		=	0x144,
		.ba5_SIEN		=	0x148,
		.ba5_SFISCfg		=	0x14c,
	},
	{	/* Channel 1 */
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
		.ba5_SActive		=	0x18c,
		.ba5_SMisc		=	0x1c0,
		.ba5_PHY_CONFIG		=	0x1c4,
		.ba5_SIEN		=	0x1c8,
		.ba5_SFISCfg		=	0x1cc,
	},
	{	/* Channel 2 (3114) */
		.ba5_IDEDMA_CMD		=	0x200,
		.ba5_IDEDMA_CTL		=	0x202,
		.ba5_IDEDMA_TBL		=	0x204,
		.ba5_IDEDMA_CMD2	=	0x210,
		.ba5_IDEDMA_CTL2	=	0x212,
		.ba5_IDE_TF0		=	0x280,	/* wd_data */
		.ba5_IDE_TF1		=	0x281,	/* wd_error */
		.ba5_IDE_TF2		=	0x282,	/* wd_seccnt */
		.ba5_IDE_TF3		=	0x283,	/* wd_sector */
		.ba5_IDE_TF4		=	0x284,	/* wd_cyl_lo */
		.ba5_IDE_TF5		=	0x285,	/* wd_cyl_hi */
		.ba5_IDE_TF6		=	0x286,	/* wd_sdh */
		.ba5_IDE_TF7		=	0x287,	/* wd_command */
		.ba5_IDE_TF8		=	0x28a,	/* wd_altsts */
		.ba5_IDE_RAD		=	0x28c,
		.ba5_IDE_TF9		=	0x291,	/* Features 2 */
		.ba5_IDE_TF10		=	0x292,	/* Sector Count 2 */
		.ba5_IDE_TF11		=	0x293,	/* Start Sector 2 */
		.ba5_IDE_TF12		=	0x294,	/* Cylinder Low 2 */
		.ba5_IDE_TF13		=	0x295,	/* Cylinder High 2 */
		.ba5_IDE_TF14		=	0x296,	/* Device/Head 2 */
		.ba5_IDE_TF15		=	0x297,	/* Cmd Sts 2 */
		.ba5_IDE_TF16		=	0x298,	/* Sector Count 2 ext */
		.ba5_IDE_TF17		=	0x299,	/* Start Sector 2 ext */
		.ba5_IDE_TF18		=	0x29a,	/* Cyl Low 2 ext */
		.ba5_IDE_TF19		=	0x29b,	/* Cyl High 2 ext */
		.ba5_IDE_RABC		=	0x29c,
		.ba5_IDE_CMD_STS	=	0x2a0,
		.ba5_IDE_CFG_STS	=	0x2a1,
		.ba5_IDE_DTM		=	0x2b4,
		.ba5_SControl		=	0x300,
		.ba5_SStatus		=	0x304,
		.ba5_SError		=	0x308,
		.ba5_SActive		=	0x30c,
		.ba5_SMisc		=	0x340,
		.ba5_PHY_CONFIG		=	0x344,
		.ba5_SIEN		=	0x348,
		.ba5_SFISCfg		=	0x34c,
	},
	{	/* Channel 3 (3114) */
		.ba5_IDEDMA_CMD		=	0x208,
		.ba5_IDEDMA_CTL		=	0x20a,
		.ba5_IDEDMA_TBL		=	0x20c,
		.ba5_IDEDMA_CMD2	=	0x218,
		.ba5_IDEDMA_CTL2	=	0x21a,
		.ba5_IDE_TF0		=	0x2c0,	/* wd_data */
		.ba5_IDE_TF1		=	0x2c1,	/* wd_error */
		.ba5_IDE_TF2		=	0x2c2,	/* wd_seccnt */
		.ba5_IDE_TF3		=	0x2c3,	/* wd_sector */
		.ba5_IDE_TF4		=	0x2c4,	/* wd_cyl_lo */
		.ba5_IDE_TF5		=	0x2c5,	/* wd_cyl_hi */
		.ba5_IDE_TF6		=	0x2c6,	/* wd_sdh */
		.ba5_IDE_TF7		=	0x2c7,	/* wd_command */
		.ba5_IDE_TF8		=	0x2ca,	/* wd_altsts */
		.ba5_IDE_RAD		=	0x2cc,
		.ba5_IDE_TF9		=	0x2d1,	/* Features 2 */
		.ba5_IDE_TF10		=	0x2d2,	/* Sector Count 2 */
		.ba5_IDE_TF11		=	0x2d3,	/* Start Sector 2 */
		.ba5_IDE_TF12		=	0x2d4,	/* Cylinder Low 2 */
		.ba5_IDE_TF13		=	0x2d5,	/* Cylinder High 2 */
		.ba5_IDE_TF14		=	0x2d6,	/* Device/Head 2 */
		.ba5_IDE_TF15		=	0x2d7,	/* Cmd Sts 2 */
		.ba5_IDE_TF16		=	0x2d8,	/* Sector Count 2 ext */
		.ba5_IDE_TF17		=	0x2d9,	/* Start Sector 2 ext */
		.ba5_IDE_TF18		=	0x2da,	/* Cyl Low 2 ext */
		.ba5_IDE_TF19		=	0x2db,	/* Cyl High 2 ext */
		.ba5_IDE_RABC		=	0x2dc,
		.ba5_IDE_CMD_STS	=	0x2e0,
		.ba5_IDE_CFG_STS	=	0x2e1,
		.ba5_IDE_DTM		=	0x2f4,
		.ba5_SControl		=	0x380,
		.ba5_SStatus		=	0x384,
		.ba5_SError		=	0x388,
		.ba5_SActive		=	0x38c,
		.ba5_SMisc		=	0x3c0,
		.ba5_PHY_CONFIG		=	0x3c4,
		.ba5_SIEN		=	0x3c8,
		.ba5_SFISCfg		=	0x3cc,
	},
};

#define	ba5_SIS		0x214		/* summary interrupt status */

/* Interrupt steering bit in BA5[0x200]. */
#define	IDEDMA_CMD_INT_STEER	(1U << 1)

static int  satalink_match(struct device *, struct cfdata *, void *);
static void satalink_attach(struct device *, struct device *, void *);

CFATTACH_DECL(satalink, sizeof(struct pciide_softc),
    satalink_match, satalink_attach, NULL, NULL);

static void sii3112_chip_map(struct pciide_softc*, struct pci_attach_args*);
static void sii3114_chip_map(struct pciide_softc*, struct pci_attach_args*);
static void sii3112_drv_probe(struct wdc_channel*);
static void sii3112_setup_channel(struct wdc_channel*);

static const struct pciide_product_desc pciide_satalink_products[] =  {
	{ PCI_PRODUCT_CMDTECH_3112,
	  0,
	  "Silicon Image SATALink 3112",
	  sii3112_chip_map,
	},
	{ PCI_PRODUCT_CMDTECH_3512,
	  0,
	  "Silicon Image SATALink 3512",
	  sii3112_chip_map,
	},
	{ PCI_PRODUCT_CMDTECH_3114,
	  0,
	  "Silicon Image SATALink 3114",
	  sii3114_chip_map,
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
ba5_read_4_ind(struct pciide_softc *sc, bus_addr_t reg)
{
	uint32_t rv;
	int s;

	s = splbio();
	pci_conf_write(sc->sc_pc, sc->sc_tag, SII3112_BA5_IND_ADDR, reg);
	rv = pci_conf_read(sc->sc_pc, sc->sc_tag, SII3112_BA5_IND_DATA);
	splx(s);

	return (rv);
}

static __inline uint32_t
ba5_read_4(struct pciide_softc *sc, bus_addr_t reg)
{

	if (__predict_true(sc->sc_ba5_en != 0))
		return (bus_space_read_4(sc->sc_ba5_st, sc->sc_ba5_sh, reg));

	return (ba5_read_4_ind(sc, reg));
}

#define	BA5_READ_4(sc, chan, reg)					\
	ba5_read_4((sc), satalink_ba5_regmap[(chan)].reg)

static __inline void
ba5_write_4_ind(struct pciide_softc *sc, bus_addr_t reg, uint32_t val)
{
	int s;

	s = splbio();
	pci_conf_write(sc->sc_pc, sc->sc_tag, SII3112_BA5_IND_ADDR, reg);
	pci_conf_write(sc->sc_pc, sc->sc_tag, SII3112_BA5_IND_DATA, val);
	splx(s);
}

static __inline void
ba5_write_4(struct pciide_softc *sc, bus_addr_t reg, uint32_t val)
{

	if (__predict_true(sc->sc_ba5_en != 0))
		bus_space_write_4(sc->sc_ba5_st, sc->sc_ba5_sh, reg, val);
	else
		ba5_write_4_ind(sc, reg, val);
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

#define	SII3112_RESET_BITS						\
	(SCS_CMD_PBM_RESET | SCS_CMD_ARB_RESET |			\
	 SCS_CMD_FF1_RESET | SCS_CMD_FF0_RESET |			\
	 SCS_CMD_IDE1_RESET | SCS_CMD_IDE0_RESET)

	/*
	 * Reset everything and then unblock all of the interrupts.
	 */
	scs_cmd = pci_conf_read(pa->pa_pc, pa->pa_tag, SII3112_SCS_CMD);
	pci_conf_write(pa->pa_pc, pa->pa_tag, SII3112_SCS_CMD,
		       scs_cmd | SII3112_RESET_BITS);
	delay(50 * 1000);
	pci_conf_write(pa->pa_pc, pa->pa_tag, SII3112_SCS_CMD,
		       scs_cmd & SCS_CMD_BA5_EN);
	delay(50 * 1000);

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

static void
sii3114_mapreg_dma(struct pciide_softc *sc, struct pci_attach_args *pa)
{
	struct pciide_channel *pc;
	int chan, reg;
	bus_size_t size;

	sc->sc_wdcdev.dma_arg = sc;
	sc->sc_wdcdev.dma_init = pciide_dma_init;
	sc->sc_wdcdev.dma_start = pciide_dma_start;
	sc->sc_wdcdev.dma_finish = pciide_dma_finish;

	if (sc->sc_wdcdev.sc_dev.dv_cfdata->cf_flags &
	    PCIIDE_OPTIONS_NODMA) {
		aprint_normal(
		    ", but unused (forced off by config file)");
		sc->sc_dma_ok = 0;
		return;
	}

	/*
	 * Slice off a subregion of BA5 for each of the channel's DMA
	 * registers.
	 */

	sc->sc_dma_iot = sc->sc_ba5_st;
	for (chan = 0; chan < 4; chan++) {
		pc = &sc->pciide_channels[chan];
		for (reg = 0; reg < IDEDMA_NREGS; reg++) {
			size = 4;
			if (size > (IDEDMA_SCH_OFFSET - reg))
				size = IDEDMA_SCH_OFFSET - reg;
			if (bus_space_subregion(sc->sc_ba5_st,
			    sc->sc_ba5_sh,
			    satalink_ba5_regmap[chan].ba5_IDEDMA_CMD + reg,
			    size, &pc->dma_iohs[reg]) != 0) {
				sc->sc_dma_ok = 0;
				aprint_normal(", but can't subregion offset "
				    "%lu size %lu",
				    (u_long) satalink_ba5_regmap[
						chan].ba5_IDEDMA_CMD + reg,
				    (u_long) size);
				return;
			}
		}
	}

	/* DMA registers all set up! */
	sc->sc_dmat = pa->pa_dmat;
	sc->sc_dma_ok = 1;
}

static int
sii3114_chansetup(struct pciide_softc *sc, int channel)
{
	static const char *channel_names[] = {
		"port 0",
		"port 1",
		"port 2",
		"port 3",
	};
	struct pciide_channel *cp = &sc->pciide_channels[channel];

	sc->wdc_chanarray[channel] = &cp->wdc_channel;

	/*
	 * We must always keep the Interrupt Steering bit set in channel 2's
	 * IDEDMA_CMD register.
	 */
	if (channel == 2)
		cp->idedma_cmd = IDEDMA_CMD_INT_STEER;

	cp->name = channel_names[channel];
	cp->wdc_channel.ch_channel = channel;
	cp->wdc_channel.ch_wdc = &sc->sc_wdcdev;
	cp->wdc_channel.ch_queue =
	    malloc(sizeof(struct ata_queue), M_DEVBUF, M_NOWAIT);
	if (cp->wdc_channel.ch_queue == NULL) {
		aprint_error("%s %s channel: "
		    "can't allocate memory for command queue",
		    sc->sc_wdcdev.sc_dev.dv_xname, cp->name);
		return (0);
	}
	return (1);
}

static void
sii3114_mapchan(struct pciide_channel *cp)
{
	struct pciide_softc *sc = (struct pciide_softc *)cp->wdc_channel.ch_wdc;
	struct wdc_channel *wdc_cp = &cp->wdc_channel;
	int i;

	cp->compat = 0;
	cp->ih = sc->sc_pci_ih;

	wdc_cp->cmd_iot = sc->sc_ba5_st;
	if (bus_space_subregion(sc->sc_ba5_st, sc->sc_ba5_sh,
			satalink_ba5_regmap[wdc_cp->ch_channel].ba5_IDE_TF0,
			9, &wdc_cp->cmd_baseioh) != 0) {
		aprint_error("%s: couldn't subregion %s cmd base\n",
		    sc->sc_wdcdev.sc_dev.dv_xname, cp->name);
		goto bad;
	}

	wdc_cp->ctl_iot = sc->sc_ba5_st;
	if (bus_space_subregion(sc->sc_ba5_st, sc->sc_ba5_sh,
			satalink_ba5_regmap[wdc_cp->ch_channel].ba5_IDE_TF8,
			1, &cp->ctl_baseioh) != 0) {
		aprint_error("%s: couldn't subregion %s ctl base\n",
		    sc->sc_wdcdev.sc_dev.dv_xname, cp->name);
		goto bad;
	}
	wdc_cp->ctl_ioh = cp->ctl_baseioh;

	for (i = 0; i < WDC_NREG; i++) {
		if (bus_space_subregion(wdc_cp->cmd_iot, wdc_cp->cmd_baseioh,
					i, i == 0 ? 4 : 1,
					&wdc_cp->cmd_iohs[i]) != 0) {
			aprint_error("%s: couldn't subregion %s channel "
				     "cmd regs\n",
			    sc->sc_wdcdev.sc_dev.dv_xname, cp->name);
			goto bad;
		}
	}
	wdc_cp->data32iot = wdc_cp->cmd_iot;
	wdc_cp->data32ioh = wdc_cp->cmd_iohs[0];
	wdcattach(wdc_cp);
	return;

 bad:
	cp->wdc_channel.ch_flags |= WDCF_DISABLED;
}

static void
sii3114_chip_map(struct pciide_softc *sc, struct pci_attach_args *pa)
{
	struct pciide_channel *cp;
	pcireg_t scs_cmd;
	pci_intr_handle_t intrhandle;
	const char *intrstr;
	int channel;

	if (pciide_chipen(sc, pa) == 0)
		return;

#define	SII3114_RESET_BITS						\
	(SCS_CMD_PBM_RESET | SCS_CMD_ARB_RESET |			\
	 SCS_CMD_FF1_RESET | SCS_CMD_FF0_RESET |			\
	 SCS_CMD_FF3_RESET | SCS_CMD_FF2_RESET |			\
	 SCS_CMD_IDE1_RESET | SCS_CMD_IDE0_RESET |			\
	 SCS_CMD_IDE3_RESET | SCS_CMD_IDE2_RESET)

	/*
	 * Reset everything and then unblock all of the interrupts.
	 */
	scs_cmd = pci_conf_read(pa->pa_pc, pa->pa_tag, SII3112_SCS_CMD);
	pci_conf_write(pa->pa_pc, pa->pa_tag, SII3112_SCS_CMD,
		       scs_cmd | SII3114_RESET_BITS);
	delay(50 * 1000);
	pci_conf_write(pa->pa_pc, pa->pa_tag, SII3112_SCS_CMD,
		       scs_cmd & SCS_CMD_M66EN);
	delay(50 * 1000);

	/*
	 * On the 3114, the BA5 register space is always enabled.  In
	 * order to use the 3114 in any sane way, we must use this BA5
	 * register space, and so we consider it an error if we cannot
	 * map it.
	 *
	 * As a consequence of using BA5, our register mapping is different
	 * from a normal PCI IDE controller's, and so we are unable to use
	 * most of the common PCI IDE register mapping functions.
	 */
	if (pci_mapreg_map(pa, PCI_MAPREG_START + 0x14,
			   PCI_MAPREG_TYPE_MEM|
			   PCI_MAPREG_MEM_TYPE_32BIT, 0,
			   &sc->sc_ba5_st, &sc->sc_ba5_sh,
			   NULL, NULL) != 0) {
		aprint_error("%s: unable to map SATALink BA5 "
		    "register space\n", sc->sc_wdcdev.sc_dev.dv_xname);
		return;
	}
	sc->sc_ba5_en = 1;

	aprint_verbose("%s: %dMHz PCI bus\n", sc->sc_wdcdev.sc_dev.dv_xname,
	    (scs_cmd & SCS_CMD_M66EN) ? 66 : 33);

	/*
	 * Set the Interrupt Steering bit in the IDEDMA_CMD register of
	 * channel 2.  This is required at all times for proper operation
	 * when using the BA5 register space (otherwise interrupts from
	 * all 4 channels won't work).
	 */
	BA5_WRITE_4(sc, 2, ba5_IDEDMA_CMD, IDEDMA_CMD_INT_STEER);

	aprint_normal("%s: bus-master DMA support present",
	    sc->sc_wdcdev.sc_dev.dv_xname);
	sii3114_mapreg_dma(sc, pa);
	aprint_normal("\n");

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
	sc->sc_wdcdev.drv_probe = sii3112_drv_probe;

	sc->sc_wdcdev.channels = sc->wdc_chanarray;
	sc->sc_wdcdev.nchannels = 4;

	/* Map and establish the interrupt handler. */
	if (pci_intr_map(pa, &intrhandle) != 0) {
		aprint_error("%s: couldn't map native-PCI interrupt\n",
		    sc->sc_wdcdev.sc_dev.dv_xname);
		return;
	}
	intrstr = pci_intr_string(pa->pa_pc, intrhandle);
	sc->sc_pci_ih = pci_intr_establish(pa->pa_pc, intrhandle, IPL_BIO,
					   /* XXX */
					   pciide_pci_intr, sc);
	if (sc->sc_pci_ih != NULL) {
		aprint_normal("%s: using %s for native-PCI interrupt\n",
			      sc->sc_wdcdev.sc_dev.dv_xname,
			      intrstr ? intrstr : "unknown interrupt");
	} else {
		aprint_error("%s: couldn't establish native-PCI interrupt",
			     sc->sc_wdcdev.sc_dev.dv_xname);
		if (intrstr != NULL)
			aprint_normal(" at %s", intrstr);
		aprint_normal("\n");
		return;
	}

	for (channel = 0; channel < sc->sc_wdcdev.nchannels; channel++) {
		cp = &sc->pciide_channels[channel];
		if (sii3114_chansetup(sc, channel) == 0)
			continue;
		sii3114_mapchan(cp);
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

static void
sii3112_drv_probe(struct wdc_channel *chp)
{
	struct pciide_channel *cp = (struct pciide_channel *)chp;
	struct pciide_softc *sc = (struct pciide_softc *)cp->wdc_channel.ch_wdc;
	uint32_t scontrol, sstatus;
	uint8_t scnt, sn, cl, ch;
	int i;

	/* XXX This should be done by other code. */
	for (i = 0; i < 2; i++) {
		chp->ch_drive[i].chnl_softc = chp;
		chp->ch_drive[i].drive = i;
	}

	/*
	 * The 3112 is a 2-port part, and only has one drive per channel
	 * (each port emulates a master drive).
	 *
	 * The 3114 is similar, but has 4 channels.
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

	BA5_WRITE_4(sc, chp->ch_channel, ba5_SControl, scontrol);
	delay(50 * 1000);
	scontrol &= ~SControl_DET_INIT;
	BA5_WRITE_4(sc, chp->ch_channel, ba5_SControl, scontrol);
	delay(50 * 1000);

	sstatus = BA5_READ_4(sc, chp->ch_channel, ba5_SStatus);
#if 0
	aprint_normal("%s: port %d: SStatus=0x%08x, SControl=0x%08x\n",
	    sc->sc_wdcdev.sc_dev.dv_xname, chp->ch_channel, sstatus,
	    BA5_READ_4(sc, chp->ch_channel, ba5_SControl));
#endif
	switch (sstatus & SStatus_DET_mask) {
	case SStatus_DET_NODEV:
		/* No device; be silent. */
		break;

	case SStatus_DET_DEV_NE:
		aprint_error("%s: port %d: device connected, but "
		    "communication not established\n",
		    sc->sc_wdcdev.sc_dev.dv_xname, chp->ch_channel);
		break;

	case SStatus_DET_OFFLINE:
		aprint_error("%s: port %d: PHY offline\n",
		    sc->sc_wdcdev.sc_dev.dv_xname, chp->ch_channel);
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
		    sc->sc_wdcdev.sc_dev.dv_xname, chp->ch_channel,
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
		    sc->sc_wdcdev.sc_dev.dv_xname, chp->ch_channel,
		    sata_speed[(sstatus & SStatus_SPD_mask) >> 
			       SStatus_SPD_shift]);
		break;

	default:
		aprint_error("%s: port %d: unknown SStatus: 0x%08x\n",
		    sc->sc_wdcdev.sc_dev.dv_xname, chp->ch_channel, sstatus);
	}
}

static void
sii3112_setup_channel(struct wdc_channel *chp)
{
	struct ata_drive_datas *drvp;
	int drive;
	u_int32_t idedma_ctl, dtm;
	struct pciide_channel *cp = (struct pciide_channel*)chp;
	struct pciide_softc *sc = (struct pciide_softc*)cp->wdc_channel.ch_wdc;

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
	BA5_WRITE_4(sc, chp->ch_channel, ba5_IDE_DTM, dtm);
}
