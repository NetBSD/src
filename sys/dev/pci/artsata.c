/*	$NetBSD: artsata.c,v 1.8.2.1 2006/02/01 14:52:08 yamt Exp $	*/

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

#include "opt_pciide.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>
#include <dev/pci/pciidereg.h>
#include <dev/pci/pciidevar.h>
#include <dev/pci/pciide_i31244_reg.h>

#include <dev/ata/satareg.h>
#include <dev/ata/satavar.h>
#include <dev/ata/atareg.h>
#include <dev/ata/atavar.h>

static void artisea_chip_map(struct pciide_softc*, struct pci_attach_args *);

static int  artsata_match(struct device *, struct cfdata *, void *);
static void artsata_attach(struct device *, struct device *, void *);

static const struct pciide_product_desc pciide_artsata_products[] =  {
	{ PCI_PRODUCT_INTEL_31244,
	  0,
	  "Intel 31244 Serial ATA Controller",
	  artisea_chip_map,
	},
	{ 0,
	  0,
	  NULL,
	  NULL
	}
};

struct artisea_cmd_map
{
	u_int8_t offset;
	u_int8_t size;
};

static const struct artisea_cmd_map artisea_dpa_cmd_map[] =
{
	{ARTISEA_SUPDDR, 4},	/* 0 Data */
	{ARTISEA_SUPDER, 1},	/* 1 Error */
	{ARTISEA_SUPDCSR, 2},	/* 2 Sector Count */
	{ARTISEA_SUPDSNR, 2},	/* 3 Sector Number */
	{ARTISEA_SUPDCLR, 2},	/* 4 Cylinder Low */
	{ARTISEA_SUPDCHR, 2},	/* 5 Cylinder High */
	{ARTISEA_SUPDDHR, 1},	/* 6 Device/Head */
	{ARTISEA_SUPDCR, 1},	/* 7 Command */
	{ARTISEA_SUPDSR, 1},	/* 8 Status */
	{ARTISEA_SUPDFR, 2}	/* 9 Feature */
};

#define ARTISEA_NUM_CHAN 4

CFATTACH_DECL(artsata, sizeof(struct pciide_softc),
    artsata_match, artsata_attach, NULL, NULL);

static int
artsata_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct pci_attach_args *pa = aux;

	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_INTEL) {
		if (pciide_lookup_product(pa->pa_id, pciide_artsata_products))
			return (2);
	}
	return (0);
}

static void
artsata_attach(struct device *parent, struct device *self, void *aux)
{
	struct pci_attach_args *pa = aux;
	struct pciide_softc *sc = (struct pciide_softc *)self;

	pciide_common_attach(sc, pa,
	    pciide_lookup_product(pa->pa_id, pciide_artsata_products));

}

static void
artisea_drv_probe(struct ata_channel *chp)
{
	struct pciide_softc *sc = CHAN_TO_PCIIDE(chp);
	struct wdc_regs *wdr = CHAN_TO_WDC_REGS(chp);
	uint32_t scontrol, sstatus;
	uint16_t scnt, sn, cl, ch;
	int i, s;

	/* XXX This should be done by other code. */
	for (i = 0; i < 2; i++) {
		chp->ch_drive[i].chnl_softc = chp;
		chp->ch_drive[i].drive = i;
	}

	/*
	 * First we have to bring the PHYs online, in case the firmware
	 * has not already done so.  The 31244 leaves the disks off-line
	 * on reset to avoid excessive power surges due to multiple spindle
	 * spin up.
	 *
	 * The work-around for errata #1 says that we must write 0 to the
	 * port first to be sure of correctly initializing the device.
	 *
	 * XXX will this try to bring multiple disks on-line too quickly?
	 */
	bus_space_write_4 (wdr->cmd_iot, wdr->cmd_baseioh,
	    ARTISEA_SUPERSET_DPA_OFF + ARTISEA_SUPDSSCR, 0);
	scontrol = SControl_IPM_NONE | SControl_SPD_ANY | SControl_DET_INIT;
	bus_space_write_4 (wdr->cmd_iot, wdr->cmd_baseioh,
	    ARTISEA_SUPERSET_DPA_OFF + ARTISEA_SUPDSSCR, scontrol);

	scontrol &= ~SControl_DET_INIT;
	bus_space_write_4 (wdr->cmd_iot, wdr->cmd_baseioh,
	    ARTISEA_SUPERSET_DPA_OFF + ARTISEA_SUPDSSCR, scontrol);

	delay(50 * 1000);
	sstatus = bus_space_read_4(wdr->cmd_iot, wdr->cmd_baseioh,
	    ARTISEA_SUPERSET_DPA_OFF + ARTISEA_SUPDSSSR);

	switch (sstatus & SStatus_DET_mask) {
	case SStatus_DET_NODEV:
		/* No Device; be silent.  */
		break;

	case SStatus_DET_DEV_NE:
		aprint_error("%s: port %d: device connected, but "
		    "communication not established\n",
		    sc->sc_wdcdev.sc_atac.atac_dev.dv_xname, chp->ch_channel);
		break;

	case SStatus_DET_OFFLINE:
		aprint_error("%s: port %d: PHY offline\n",
		    sc->sc_wdcdev.sc_atac.atac_dev.dv_xname, chp->ch_channel);
		break;

	case SStatus_DET_DEV:
		bus_space_write_1(wdr->cmd_iot, wdr->cmd_iohs[wd_sdh], 0,
		    WDSD_IBM);
		delay(10);	/* 400ns delay */
		scnt = bus_space_read_2(wdr->cmd_iot,
		    wdr->cmd_iohs[wd_seccnt], 0);
		sn = bus_space_read_2(wdr->cmd_iot,
		    wdr->cmd_iohs[wd_sector], 0);
		cl = bus_space_read_2(wdr->cmd_iot,
		    wdr->cmd_iohs[wd_cyl_lo], 0);
		ch = bus_space_read_2(wdr->cmd_iot,
		    wdr->cmd_iohs[wd_cyl_hi], 0);
		printf("%s: port %d: scnt=0x%x sn=0x%x cl=0x%x ch=0x%x\n",
		    sc->sc_wdcdev.sc_atac.atac_dev.dv_xname, chp->ch_channel,
		    scnt, sn, cl, ch);
		/*
		 * scnt and sn are supposed to be 0x1 for ATAPI, but in some
		 * cases we get wrong values here, so ignore it.
		 */
		s = splbio();
		if (cl == 0x14 && ch == 0xeb)
			chp->ch_drive[0].drive_flags |= DRIVE_ATAPI;
		else
			chp->ch_drive[0].drive_flags |= DRIVE_ATA;
		splx(s);

		aprint_normal("%s: port %d: device present, speed: %s\n",
		    sc->sc_wdcdev.sc_atac.atac_dev.dv_xname, chp->ch_channel,
		    sata_speed(sstatus));
		break;

	default:
		aprint_error("%s: port %d: unknown SStatus: 0x%08x\n",
		    sc->sc_wdcdev.sc_atac.atac_dev.dv_xname, chp->ch_channel,
		    sstatus);
	}

}

static void
artisea_mapregs(struct pci_attach_args *pa, struct pciide_channel *cp,
	bus_size_t *cmdsizep, bus_size_t *ctlsizep, int (*pci_intr)(void *))
{
	struct pciide_softc *sc = CHAN_TO_PCIIDE(&cp->ata_channel);
	struct ata_channel *wdc_cp = &cp->ata_channel;
	struct wdc_regs *wdr = CHAN_TO_WDC_REGS(wdc_cp);
	const char *intrstr;
	pci_intr_handle_t intrhandle;
	int i;

	cp->compat = 0;

	if (sc->sc_pci_ih == NULL) {
		if (pci_intr_map(pa, &intrhandle) != 0) {
			aprint_error("%s: couldn't map native-PCI interrupt\n",
			    sc->sc_wdcdev.sc_atac.atac_dev.dv_xname);
			goto bad;
		}
		intrstr = pci_intr_string(pa->pa_pc, intrhandle);
		sc->sc_pci_ih = pci_intr_establish(pa->pa_pc,
		    intrhandle, IPL_BIO, pci_intr, sc);
		if (sc->sc_pci_ih != NULL) {
			aprint_normal("%s: using %s for native-PCI interrupt\n",
			    sc->sc_wdcdev.sc_atac.atac_dev.dv_xname,
			    intrstr ? intrstr : "unknown interrupt");
		} else {
			aprint_error(
			    "%s: couldn't establish native-PCI interrupt",
			    sc->sc_wdcdev.sc_atac.atac_dev.dv_xname);
			if (intrstr != NULL)
				aprint_normal(" at %s", intrstr);
			aprint_normal("\n");
			goto bad;
		}
	}
	cp->ih = sc->sc_pci_ih;
	wdr->cmd_iot = sc->sc_ba5_st;
	if (bus_space_subregion (sc->sc_ba5_st, sc->sc_ba5_sh,
	    ARTISEA_DPA_PORT_BASE(wdc_cp->ch_channel), 0x200,
	    &wdr->cmd_baseioh) != 0) {
		aprint_error("%s: couldn't map %s channel cmd regs\n",
		    sc->sc_wdcdev.sc_atac.atac_dev.dv_xname, cp->name);
		goto bad;
	}

	wdr->ctl_iot = sc->sc_ba5_st;
	if (bus_space_subregion(wdr->cmd_iot, wdr->cmd_baseioh,
	    ARTISEA_SUPDDCTLR, 1, &cp->ctl_baseioh) != 0) {
		aprint_error("%s: couldn't map %s channel ctl regs\n",
		    sc->sc_wdcdev.sc_atac.atac_dev.dv_xname, cp->name);
		goto bad;
	}
	wdr->ctl_ioh = cp->ctl_baseioh;

	for (i = 0; i < WDC_NREG + 2; i++) {

		if (bus_space_subregion(wdr->cmd_iot, wdr->cmd_baseioh,
		    artisea_dpa_cmd_map[i].offset, artisea_dpa_cmd_map[i].size,
		    &wdr->cmd_iohs[i]) != 0) {
			aprint_error("%s: couldn't subregion %s channel "
				     "cmd regs\n",
			    sc->sc_wdcdev.sc_atac.atac_dev.dv_xname, cp->name);
			goto bad;
		}
	}
	wdr->data32iot = wdr->cmd_iot;
	wdr->data32ioh = wdr->cmd_iohs[0];

	wdcattach(wdc_cp);
	return;

bad:
	cp->ata_channel.ch_flags |= ATACH_DISABLED;
	return;
}

static int
artisea_chansetup(struct pciide_softc *sc, int channel, pcireg_t interface)
{
	struct pciide_channel *cp = &sc->pciide_channels[channel];
	sc->wdc_chanarray[channel] = &cp->ata_channel;
	cp->name = PCIIDE_CHANNEL_NAME(channel);
	cp->ata_channel.ch_channel = channel;
	cp->ata_channel.ch_atac = &sc->sc_wdcdev.sc_atac;
	cp->ata_channel.ch_queue =
	    malloc(sizeof(struct ata_queue), M_DEVBUF, M_NOWAIT);
	cp->ata_channel.ch_ndrive = 2;
	if (cp->ata_channel.ch_queue == NULL) {
		aprint_error("%s %s channel: "
		    "can't allocate memory for command queue",
		sc->sc_wdcdev.sc_atac.atac_dev.dv_xname, cp->name);
		return 0;
	}
	return 1;
}

static void
artisea_mapreg_dma(struct pciide_softc *sc, struct pci_attach_args *pa)
{
	struct pciide_channel *pc;
	int chan;
	u_int32_t dma_ctl;
	u_int32_t cacheline_len;

	aprint_normal("%s: bus-master DMA support present",
	    sc->sc_wdcdev.sc_atac.atac_dev.dv_xname);

	sc->sc_dma_ok = 1;

	/*
	 * Errata #4 says that if the cacheline length is not set correctly,
	 * we can get corrupt MWI and Memory-Block-Write transactions.
	 */
	cacheline_len = PCI_CACHELINE(pci_conf_read (pa->pa_pc, pa->pa_tag,
	    PCI_BHLC_REG));
	if (cacheline_len == 0) {
		aprint_normal(", but unused (cacheline size not set in PCI conf)\n");
		sc->sc_dma_ok = 0;
		return;
	}

	/*
	 * Final step of the work-around is to force the DMA engine to use
	 * the cache-line length information.
	 */
	dma_ctl = pci_conf_read(pa->pa_pc, pa->pa_tag, ARTISEA_PCI_SUDCSCR);
	dma_ctl |= SUDCSCR_DMA_WCAE | SUDCSCR_DMA_RCAE;
	pci_conf_write(pa->pa_pc, pa->pa_tag, ARTISEA_PCI_SUDCSCR, dma_ctl);

	sc->sc_wdcdev.dma_arg = sc;
	sc->sc_wdcdev.dma_init = pciide_dma_init;
	sc->sc_wdcdev.dma_start = pciide_dma_start;
	sc->sc_wdcdev.dma_finish = pciide_dma_finish;
	sc->sc_dma_iot = sc->sc_ba5_st;
	sc->sc_dmat = pa->pa_dmat;

	if (sc->sc_wdcdev.sc_atac.atac_dev.dv_cfdata->cf_flags &
	    PCIIDE_OPTIONS_NODMA) {
		aprint_normal(
		    ", but unused (forced off by config file)\n");
		sc->sc_dma_ok = 0;
		return;
	}

	/*
	 * Set up the default handles for the DMA registers.
	 * Just reserve 32 bits for each handle, unless space
	 * doesn't permit it.
	 */
	for (chan = 0; chan < ARTISEA_NUM_CHAN; chan++) {
		pc = &sc->pciide_channels[chan];
		if (bus_space_subregion(sc->sc_ba5_st, sc->sc_ba5_sh,
		    ARTISEA_DPA_PORT_BASE(chan) + ARTISEA_SUPDDCMDR, 2,
		    &pc->dma_iohs[IDEDMA_CMD]) != 0 ||
		    bus_space_subregion(sc->sc_ba5_st, sc->sc_ba5_sh,
		    ARTISEA_DPA_PORT_BASE(chan) + ARTISEA_SUPDDSR, 1,
		    &pc->dma_iohs[IDEDMA_CTL]) != 0 ||
		    bus_space_subregion(sc->sc_ba5_st, sc->sc_ba5_sh,
		    ARTISEA_DPA_PORT_BASE(chan) + ARTISEA_SUPDDDTPR, 4,
		    &pc->dma_iohs[IDEDMA_TBL]) != 0) {
			sc->sc_dma_ok = 0;
			aprint_normal(", but can't subregion registers\n");
			return;
		}
	}

	aprint_normal("\n");
}

static void
artisea_chip_map_dpa(struct pciide_softc *sc, struct pci_attach_args *pa)
{
	struct pciide_channel *cp;
	bus_size_t cmdsize, ctlsize;
	pcireg_t interface;
	int channel;

	interface = PCI_INTERFACE(pa->pa_class);

	aprint_normal("%s: interface wired in DPA mode\n",
	    sc->sc_wdcdev.sc_atac.atac_dev.dv_xname);

	if (pci_mapreg_map(pa, ARTISEA_PCI_DPA_BASE, PCI_MAPREG_MEM_TYPE_64BIT,
	    0, &sc->sc_ba5_st, &sc->sc_ba5_sh, NULL, NULL) != 0)
		return;

	artisea_mapreg_dma(sc, pa);

	sc->sc_wdcdev.cap = WDC_CAPABILITY_WIDEREGS;

	sc->sc_wdcdev.sc_atac.atac_cap |= ATAC_CAP_DATA16 | ATAC_CAP_DATA32;
	sc->sc_wdcdev.sc_atac.atac_pio_cap = 4;
	if (sc->sc_dma_ok) {
		sc->sc_wdcdev.sc_atac.atac_cap |= ATAC_CAP_DMA | ATAC_CAP_UDMA;
		sc->sc_wdcdev.irqack = pciide_irqack;
		sc->sc_wdcdev.sc_atac.atac_dma_cap = 2;
		sc->sc_wdcdev.sc_atac.atac_udma_cap = 6;
	}
	sc->sc_wdcdev.sc_atac.atac_set_modes = sata_setup_channel;

	sc->sc_wdcdev.sc_atac.atac_channels = sc->wdc_chanarray;
	sc->sc_wdcdev.sc_atac.atac_nchannels = ARTISEA_NUM_CHAN;
	sc->sc_wdcdev.sc_atac.atac_probe = artisea_drv_probe;

	wdc_allocate_regs(&sc->sc_wdcdev);

	/*
	 * Perform a quick check to ensure that the device isn't configured
	 * in Spread-spectrum clocking mode.  This feature is buggy and has
	 * been removed from the latest documentation.
	 *
	 * Note that although this bit is in the Channel regs, it's the same
	 * for all channels, so we check it just once here.
	 */
	if ((bus_space_read_4 (sc->sc_ba5_st, sc->sc_ba5_sh,
	    ARTISEA_DPA_PORT_BASE(0) + ARTISEA_SUPERSET_DPA_OFF +
	    ARTISEA_SUPDPFR) & SUPDPFR_SSCEN) != 0) {
		aprint_error("%s: Spread-specturm clocking not supported by device\n",
		     sc->sc_wdcdev.sc_atac.atac_dev.dv_xname);
		return;
	}

	/* Clear the LED0-only bit.  */
	pci_conf_write (pa->pa_pc, pa->pa_tag, ARTISEA_PCI_SUECSR0,
	    pci_conf_read (pa->pa_pc, pa->pa_tag, ARTISEA_PCI_SUECSR0) &
	    ~SUECSR0_LED0_ONLY);

	for (channel = 0; channel < sc->sc_wdcdev.sc_atac.atac_nchannels;
	     channel++) {
		cp = &sc->pciide_channels[channel];
		if (artisea_chansetup(sc, channel, interface) == 0)
			continue;
		/* XXX We can probably do interrupts more efficiently.  */
		artisea_mapregs(pa, cp, &cmdsize, &ctlsize, pciide_pci_intr);
	}
}

static void
artisea_chip_map(struct pciide_softc *sc, struct pci_attach_args *pa)
{
	struct pciide_channel *cp;
	bus_size_t cmdsize, ctlsize;
	pcireg_t interface;
	int channel;

	if (pciide_chipen(sc, pa) == 0)
		return;

	interface = PCI_INTERFACE(pa->pa_class);

	if (interface == 0) {
		artisea_chip_map_dpa (sc, pa);
		return;
	}

	aprint_normal("%s: bus-master DMA support present",
	    sc->sc_wdcdev.sc_atac.atac_dev.dv_xname);
#ifdef PCIIDE_I31244_DISABLEDMA
	if (sc->sc_pp->ide_product == PCI_PRODUCT_INTEL_31244 &&
	    PCI_REVISION(pa->pa_class) == 0) {
		aprint_normal(" but disabled due to rev. 0");
		sc->sc_dma_ok = 0;
	} else
#endif
		pciide_mapreg_dma(sc, pa);
	aprint_normal("\n");

	/*
	 * XXX Configure LEDs to show activity.
	 */

	sc->sc_wdcdev.sc_atac.atac_cap |= ATAC_CAP_DATA16 | ATAC_CAP_DATA32;
	sc->sc_wdcdev.sc_atac.atac_pio_cap = 4;
	if (sc->sc_dma_ok) {
		sc->sc_wdcdev.sc_atac.atac_cap |= ATAC_CAP_DMA | ATAC_CAP_UDMA;
		sc->sc_wdcdev.irqack = pciide_irqack;
		sc->sc_wdcdev.sc_atac.atac_dma_cap = 2;
		sc->sc_wdcdev.sc_atac.atac_udma_cap = 6;
	}
	sc->sc_wdcdev.sc_atac.atac_set_modes = sata_setup_channel;

	sc->sc_wdcdev.sc_atac.atac_channels = sc->wdc_chanarray;
	sc->sc_wdcdev.sc_atac.atac_nchannels = PCIIDE_NUM_CHANNELS;

	wdc_allocate_regs(&sc->sc_wdcdev);

	for (channel = 0; channel < sc->sc_wdcdev.sc_atac.atac_nchannels;
	     channel++) {
		cp = &sc->pciide_channels[channel];
		if (pciide_chansetup(sc, channel, interface) == 0)
			continue;
		pciide_mapchan(pa, cp, interface, &cmdsize, &ctlsize,
		    pciide_pci_intr);
	}
}
