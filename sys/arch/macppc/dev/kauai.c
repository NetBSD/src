/*	$NetBSD: kauai.c,v 1.27 2009/11/18 22:51:07 dyoung Exp $	*/

/*-
 * Copyright (c) 2003 Tsubai Masanari.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kauai.c,v 1.27 2009/11/18 22:51:07 dyoung Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <uvm/uvm_extern.h>

#include <machine/bus.h>
#include <machine/pio.h>

#include <dev/ata/atareg.h>
#include <dev/ata/atavar.h>
#include <dev/ic/wdcvar.h>

#include <dev/ofw/openfirm.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

#include <macppc/dev/dbdma.h>

#define WDC_REG_NPORTS		8
#define WDC_AUXREG_OFFSET	0x16
#define WDC_AUXREG_NPORTS	1

#define PIO_CONFIG_REG	0x200	/* PIO and DMA access timing */
#define DMA_CONFIG_REG	0x210	/* UDMA access timing */

struct kauai_softc {
	struct wdc_softc sc_wdcdev;
	struct ata_channel *sc_chanptr;
	struct ata_channel sc_channel;
	struct wdc_regs sc_wdc_regs;
	struct ata_queue sc_queue;
	dbdma_regmap_t *sc_dmareg;
	dbdma_command_t	*sc_dmacmd;
	u_int sc_piotiming_r[2];
	u_int sc_piotiming_w[2];
	u_int sc_dmatiming_r[2];
	u_int sc_dmatiming_w[2];
	void (*sc_calc_timing)(struct kauai_softc *, int);
};

static int kauai_match(device_t, cfdata_t, void *);
static void kauai_attach(device_t, device_t, void *);
static int kauai_dma_init(void *, int, int, void *, size_t, int);
static void kauai_dma_start(void *, int, int);
static int kauai_dma_finish(void *, int, int, int);
static void kauai_set_modes(struct ata_channel *);
static void calc_timing_kauai(struct kauai_softc *, int);

CFATTACH_DECL_NEW(kauai, sizeof(struct kauai_softc),
    kauai_match, kauai_attach, NULL, NULL);

int
kauai_match(device_t parent, cfdata_t match, void *aux)
{
	struct pci_attach_args *pa = aux;

	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_APPLE) {
		switch (PCI_PRODUCT(pa->pa_id)) {
		case PCI_PRODUCT_APPLE_KAUAI:
		case PCI_PRODUCT_APPLE_UNINORTH_ATA:
		case PCI_PRODUCT_APPLE_INTREPID2_ATA:
		case PCI_PRODUCT_APPLE_SHASTA_ATA:
		    return 5;
		}
	}

	return 0;
}

void
kauai_attach(device_t parent, device_t self, void *aux)
{
	struct kauai_softc *sc = device_private(self);
	struct pci_attach_args *pa = aux;
	struct ata_channel *chp = &sc->sc_channel;
	struct wdc_regs *wdr;
	pci_intr_handle_t ih;
	paddr_t regbase, dmabase;
	int node, reg[5], i;

	sc->sc_wdcdev.sc_atac.atac_dev = self;

	sc->sc_dmacmd = dbdma_alloc(sizeof(dbdma_command_t) * 20);

	node = pcidev_to_ofdev(pa->pa_pc, pa->pa_tag);
	if (node == 0) {
		aprint_error(": cannot find kauai node\n");
		return;
	}

	if (OF_getprop(node, "assigned-addresses", reg, sizeof reg) < 12) {
		aprint_error(": cannot get address property\n");
		return;
	}
	regbase = reg[2] + 0x2000;
	dmabase = reg[2] + 0x1000;

	/*
	 * XXX PCI_INTERRUPT_REG seems to be wired to 0.
	 * XXX So use fixed intrpin and intrline values.
	 */
	if (pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_INTERRUPT_REG) == 0) {
		pa->pa_intrpin = 1;
		pa->pa_intrline = 39;
	}

	if (pci_intr_map(pa, &ih)) {
		aprint_error(": unable to map interrupt\n");
		return;
	}
	aprint_normal(": interrupting at %s\n", pci_intr_string(pa->pa_pc, ih));

	sc->sc_wdcdev.regs = wdr = &sc->sc_wdc_regs;

	wdr->cmd_iot = wdr->ctl_iot = pa->pa_memt;

	if (bus_space_map(wdr->cmd_iot, regbase, WDC_REG_NPORTS << 4, 0,
	    &wdr->cmd_baseioh) ||
	    bus_space_subregion(wdr->cmd_iot, wdr->cmd_baseioh,
			WDC_AUXREG_OFFSET << 4, 1, &wdr->ctl_ioh)) {
		aprint_error_dev(self, "couldn't map registers\n");
		return;
	}
	for (i = 0; i < WDC_NREG; i++) {
		if (bus_space_subregion(wdr->cmd_iot, wdr->cmd_baseioh, i << 4,
		    i == 0 ? 4 : 1, &wdr->cmd_iohs[i]) != 0) {
			bus_space_unmap(wdr->cmd_iot, wdr->cmd_baseioh,
			    WDC_REG_NPORTS << 4);
			aprint_error_dev(self,
			    "couldn't subregion registers\n");
			return;
		}
	}

	if (pci_intr_establish(pa->pa_pc, ih, IPL_BIO, wdcintr, chp) == NULL) {
		aprint_error_dev(self, "unable to establish interrupt\n");
		return;
	}


	sc->sc_wdcdev.sc_atac.atac_pio_cap = 4;
	sc->sc_wdcdev.sc_atac.atac_dma_cap = 2;
	sc->sc_wdcdev.sc_atac.atac_udma_cap = 5;
	sc->sc_wdcdev.sc_atac.atac_cap |= ATAC_CAP_DATA16;
	sc->sc_wdcdev.sc_atac.atac_cap |= ATAC_CAP_DMA | ATAC_CAP_UDMA;
	sc->sc_chanptr = chp;
	sc->sc_wdcdev.sc_atac.atac_channels = &sc->sc_chanptr;
	sc->sc_wdcdev.sc_atac.atac_nchannels = 1;
	sc->sc_wdcdev.dma_arg = sc;
	sc->sc_wdcdev.dma_init = kauai_dma_init;
	sc->sc_wdcdev.dma_start = kauai_dma_start;
	sc->sc_wdcdev.dma_finish = kauai_dma_finish;
	sc->sc_wdcdev.sc_atac.atac_set_modes = kauai_set_modes;
	sc->sc_calc_timing = calc_timing_kauai;
	sc->sc_dmareg = (void *)dmabase;

	chp->ch_channel = 0;
	chp->ch_atac = &sc->sc_wdcdev.sc_atac;
	chp->ch_queue = &sc->sc_queue;
	chp->ch_ndrive = 2;
	wdc_init_shadow_regs(chp);

	wdcattach(chp);
}

void
kauai_set_modes(struct ata_channel *chp)
{
	struct kauai_softc *sc = (void *)chp->ch_atac;
	struct wdc_regs *wdr = CHAN_TO_WDC_REGS(chp);
	struct ata_drive_datas *drvp0 = &chp->ch_drive[0];
	struct ata_drive_datas *drvp1 = &chp->ch_drive[1];
	struct ata_drive_datas *drvp;
	int drive;

	if ((drvp0->drive_flags & DRIVE) && (drvp1->drive_flags & DRIVE)) {
		drvp0->PIO_mode = drvp1->PIO_mode =
		    min(drvp0->PIO_mode, drvp1->PIO_mode);
	}

	for (drive = 0; drive < 2; drive++) {
		drvp = &chp->ch_drive[drive];
		if (drvp->drive_flags & DRIVE) {
			(*sc->sc_calc_timing)(sc, drive);
			bus_space_write_4(wdr->cmd_iot, wdr->cmd_baseioh,
			    PIO_CONFIG_REG, sc->sc_piotiming_r[drive]);
			bus_space_write_4(wdr->cmd_iot, wdr->cmd_baseioh,
			    DMA_CONFIG_REG, sc->sc_dmatiming_r[drive]);
		}
	}
}

/*
 * IDE transfer timings
 */
static const u_int pio_timing_kauai[] = {	/* 0xff000fff */
	0x08000a92,	/* Mode 0 */
	0x0800060f,	/*      1 */
	0x0800038b,	/*      2 */
	0x05000249,	/*      3 */
	0x04000148	/*      4 */
};
static const u_int dma_timing_kauai[] = {	/* 0x00fff000 */
	0x00618000,	/* Mode 0 */
	0x00209000,	/*      1 */
	0x00148000	/*      2 */
};
static const u_int udma_timing_kauai[] = {	/* 0x0000ffff */
	0x000070c0,	/* Mode 0 */
	0x00005d80,	/*      1 */
	0x00004a60,	/*      2 */
	0x00003a50,	/*      3 */
	0x00002a30,	/*      4 */
	0x00002921	/*      5 */
};

/*
 * Timing calculation for Kauai.
 */
void
calc_timing_kauai(struct kauai_softc *sc, int drive)
{
	struct ata_channel *chp = &sc->sc_channel;
	struct ata_drive_datas *drvp = &chp->ch_drive[drive];
	int piomode = drvp->PIO_mode;
	int dmamode = drvp->DMA_mode;
	int udmamode = drvp->UDMA_mode;
	u_int pioconf, dmaconf;

	pioconf = pio_timing_kauai[piomode];

	dmaconf = 0;
	if (drvp->drive_flags & DRIVE_DMA)
		dmaconf |= dma_timing_kauai[dmamode];
	if (drvp->drive_flags & DRIVE_UDMA)
		dmaconf |= udma_timing_kauai[udmamode];

	if (drvp->drive_flags & DRIVE_UDMA)
		dmaconf |= 1;

	sc->sc_piotiming_r[drive] = sc->sc_piotiming_w[drive] = pioconf;
	sc->sc_dmatiming_r[drive] = sc->sc_dmatiming_w[drive] = dmaconf;
}

int
kauai_dma_init(void *v, int channel, int drive, void *databuf,
	size_t datalen, int flags)
{
	struct kauai_softc *sc = v;
	dbdma_command_t *cmdp = sc->sc_dmacmd;
	struct ata_channel *chp = &sc->sc_channel;
	struct wdc_regs *wdr = CHAN_TO_WDC_REGS(chp);
	vaddr_t va = (vaddr_t)databuf;
	int read = flags & WDC_DMA_READ;
	int cmd = read ? DBDMA_CMD_IN_MORE : DBDMA_CMD_OUT_MORE;
	u_int offset;

	bus_space_write_4(wdr->cmd_iot, wdr->cmd_baseioh, DMA_CONFIG_REG,
	    read ? sc->sc_dmatiming_r[drive] : sc->sc_dmatiming_w[drive]);
	bus_space_read_4(wdr->cmd_iot, wdr->cmd_baseioh, DMA_CONFIG_REG);

	offset = va & PGOFSET;

	/* if va is not page-aligned, setup the first page */
	if (offset != 0) {
		int rest = PAGE_SIZE - offset;	/* the rest of the page */

		if (datalen > rest) {		/* if continues to next page */
			DBDMA_BUILD(cmdp, cmd, 0, rest, vtophys(va),
				DBDMA_INT_NEVER, DBDMA_WAIT_NEVER,
				DBDMA_BRANCH_NEVER);
			datalen -= rest;
			va += rest;
			cmdp++;
		}
	}

	/* now va is page-aligned */
	while (datalen > PAGE_SIZE) {
		DBDMA_BUILD(cmdp, cmd, 0, PAGE_SIZE, vtophys(va),
			DBDMA_INT_NEVER, DBDMA_WAIT_NEVER, DBDMA_BRANCH_NEVER);
		datalen -= PAGE_SIZE;
		va += PAGE_SIZE;
		cmdp++;
	}

	/* the last page (datalen <= PAGE_SIZE here) */
	cmd = read ? DBDMA_CMD_IN_LAST : DBDMA_CMD_OUT_LAST;
	DBDMA_BUILD(cmdp, cmd, 0, datalen, vtophys(va),
		DBDMA_INT_NEVER, DBDMA_WAIT_NEVER, DBDMA_BRANCH_NEVER);
	cmdp++;

	DBDMA_BUILD(cmdp, DBDMA_CMD_STOP, 0, 0, 0,
		DBDMA_INT_NEVER, DBDMA_WAIT_NEVER, DBDMA_BRANCH_NEVER);

	return 0;
}

void
kauai_dma_start(void *v, int channel, int drive)
{
	struct kauai_softc *sc = v;

	dbdma_start(sc->sc_dmareg, sc->sc_dmacmd);
}

int
kauai_dma_finish(void *v, int channel, int drive, int read)
{
	struct kauai_softc *sc = v;

	dbdma_stop(sc->sc_dmareg);
	return 0;
}
