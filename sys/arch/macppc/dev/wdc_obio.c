/*	$NetBSD: wdc_obio.c,v 1.58.16.1 2016/10/05 20:55:31 skrll Exp $	*/

/*-
 * Copyright (c) 1998, 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum and by Onno van der Linden.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: wdc_obio.c,v 1.58.16.1 2016/10/05 20:55:31 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kmem.h>

#include <uvm/uvm_extern.h>

#include <sys/bus.h>
#include <machine/autoconf.h>
#include <machine/pio.h>

#include <dev/ata/atareg.h>
#include <dev/ata/atavar.h>
#include <dev/ic/wdcvar.h>

#include <dev/ofw/openfirm.h>

#include <macppc/dev/dbdma.h>

#define WDC_REG_NPORTS		8
#define WDC_AUXREG_OFFSET	0x16
#define WDC_DEFAULT_PIO_IRQ	13	/* XXX */
#define WDC_DEFAULT_DMA_IRQ	2	/* XXX */

#define WDC_OPTIONS_DMA 0x01

/*
 * XXX This code currently doesn't even try to allow 32-bit data port use.
 */

struct wdc_obio_softc {
	struct wdc_softc sc_wdcdev;
	struct ata_channel *sc_chanptr;
	struct ata_channel sc_channel;
	struct ata_queue sc_chqueue;
	struct wdc_regs sc_wdc_regs;
	bus_space_handle_t sc_dmaregh;
	dbdma_regmap_t *sc_dmareg;
	dbdma_command_t	*sc_dmacmd;
	u_int sc_dmaconf[2];	/* per target value of CONFIG_REG */
	void *sc_ih, *sc_dma;
};

static int wdc_obio_match(device_t, cfdata_t, void *);
static void wdc_obio_attach(device_t, device_t, void *);
static int wdc_obio_detach(device_t, int);
static int wdc_obio_dma_init(void *, int, int, void *, size_t, int);
static void wdc_obio_dma_start(void *, int, int);
static int wdc_obio_dma_finish(void *, int, int, int);

static void wdc_obio_select(struct ata_channel *, int);
static void adjust_timing(struct ata_channel *);
static void ata4_adjust_timing(struct ata_channel *);

CFATTACH_DECL_NEW(wdc_obio, sizeof(struct wdc_obio_softc),
    wdc_obio_match, wdc_obio_attach, wdc_obio_detach, NULL);

static const char * const ata_names[] = {
    "heathrow-ata",
    "keylargo-ata",
    "ohare-ata",
    NULL
};

int
wdc_obio_match(device_t parent, cfdata_t match, void *aux)
{
	struct confargs *ca = aux;

	/* XXX should not use name */
	if (strcmp(ca->ca_name, "ATA") == 0 ||
	    strcmp(ca->ca_name, "ata") == 0 ||
	    strcmp(ca->ca_name, "ata0") == 0 ||
	    strcmp(ca->ca_name, "ide") == 0)
		return 1;

	if (of_compatible(ca->ca_node, ata_names) >= 0)
		return 1;

	return 0;
}

void
wdc_obio_attach(device_t parent, device_t self, void *aux)
{
	struct wdc_obio_softc *sc = device_private(self);
	struct wdc_regs *wdr;
	struct confargs *ca = aux;
	struct ata_channel *chp = &sc->sc_channel;
	int intr, i, type = IST_EDGE;
	int use_dma = 0;
	char path[80];

	sc->sc_wdcdev.sc_atac.atac_dev = self;
	if (device_cfdata(sc->sc_wdcdev.sc_atac.atac_dev)->cf_flags &
	    WDC_OPTIONS_DMA) {
		if (ca->ca_nreg >= 16 || ca->ca_nintr == -1)
			use_dma = 1;	/* XXX Don't work yet. */
	}

	if (ca->ca_nintr >= 4 && ca->ca_nreg >= 8) {
		intr = ca->ca_intr[0];
		aprint_normal(" irq %d", intr);
		if (ca->ca_nintr > 8) {
			type = ca->ca_intr[1] ? IST_LEVEL : IST_EDGE;
		}
		aprint_normal(", %s triggered", (type == IST_EDGE) ? "edge" : "level");
	} else if (ca->ca_nintr == -1) {
		intr = WDC_DEFAULT_PIO_IRQ;
		aprint_normal(" irq property not found; using %d", intr);
	} else {
		aprint_error(": couldn't get irq property\n");
		return;
	}

	if (use_dma)
		aprint_normal(": DMA transfer");

	aprint_normal("\n");

	sc->sc_wdcdev.regs = wdr = &sc->sc_wdc_regs;

	wdr->cmd_iot = wdr->ctl_iot = ca->ca_tag;

	if (bus_space_map(wdr->cmd_iot, ca->ca_baseaddr + ca->ca_reg[0],
	    WDC_REG_NPORTS << 4, 0, &wdr->cmd_baseioh) ||
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
#if 0
	wdr->data32iot = wdr->cmd_iot;
	wdr->data32ioh = wdr->cmd_ioh;
#endif

	sc->sc_ih = intr_establish(intr, type, IPL_BIO, wdcintr, chp);

	if (use_dma) {
		sc->sc_dmacmd = dbdma_alloc(sizeof(dbdma_command_t) * 20,
		    &sc->sc_dma);
		/*
		 * XXX
		 * we don't use ca->ca_reg[3] for size here because at least
		 * on the PB3400c it says 0x200 for both IDE channels ( the
		 * one on the mainboard and the other on the mediabay ) but
		 * their start addresses are only 0x100 apart. Since those
		 * DMA registers are always 0x100 or less we don't really 
		 * have to care though
		 */
		if (bus_space_map(wdr->cmd_iot, ca->ca_baseaddr + ca->ca_reg[2],
		    0x100, BUS_SPACE_MAP_LINEAR, &sc->sc_dmaregh)) {

			aprint_error_dev(self,
			    "unable to map DMA registers (%08x)\n",
			    ca->ca_reg[2]);
			/* should unmap stuff here */
			return;
		}
		sc->sc_dmareg = bus_space_vaddr(wdr->cmd_iot, sc->sc_dmaregh);

		sc->sc_wdcdev.sc_atac.atac_cap |= ATAC_CAP_DMA;
		sc->sc_wdcdev.sc_atac.atac_dma_cap = 2;
		if (strcmp(ca->ca_name, "ata-4") == 0) {
			sc->sc_wdcdev.sc_atac.atac_cap |= ATAC_CAP_UDMA;
			sc->sc_wdcdev.sc_atac.atac_udma_cap = 4;
			sc->sc_wdcdev.sc_atac.atac_set_modes = 
			    ata4_adjust_timing;
		} else {
			sc->sc_wdcdev.sc_atac.atac_set_modes = adjust_timing;
		}
#ifdef notyet
		/* Minimum cycle time is 150ns (DMA MODE 1) on ohare. */
		if (ohare) {
			sc->sc_wdcdev.sc_atac.atac_pio_cap = 3;
			sc->sc_wdcdev.sc_atac.atac_dma_cap = 1;
		}
#endif
	} else {
		/* all non-DMA controllers can use adjust_timing */
		sc->sc_wdcdev.sc_atac.atac_set_modes = adjust_timing;
		sc->sc_dmacmd = NULL;
	}

	sc->sc_wdcdev.sc_atac.atac_pio_cap = 4;
	sc->sc_wdcdev.sc_atac.atac_cap |= ATAC_CAP_DATA16 /*| ATAC_CAP_DATA32*/;
	sc->sc_chanptr = chp;
	sc->sc_wdcdev.sc_atac.atac_channels = &sc->sc_chanptr;
	sc->sc_wdcdev.sc_atac.atac_nchannels = 1;
	sc->sc_wdcdev.wdc_maxdrives = 2;
	sc->sc_wdcdev.dma_arg = sc;
	sc->sc_wdcdev.dma_init = wdc_obio_dma_init;
	sc->sc_wdcdev.dma_start = wdc_obio_dma_start;
	sc->sc_wdcdev.dma_finish = wdc_obio_dma_finish;
	chp->ch_channel = 0;
	chp->ch_atac = &sc->sc_wdcdev.sc_atac;
	chp->ch_queue = &sc->sc_chqueue;

	wdc_init_shadow_regs(chp);

#define OHARE_FEATURE_REG	0xf3000038

	/* XXX Enable wdc1 by feature reg. */
	memset(path, 0, sizeof(path));
	OF_package_to_path(ca->ca_node, path, sizeof(path));
	if (strcmp(path, "/bandit@F2000000/ohare@10/ata@21000") == 0) {
		u_int x;

		x = in32rb(OHARE_FEATURE_REG);
		x |= 8;
		out32rb(OHARE_FEATURE_REG, x);
	}

	wdcattach(chp);
}

/* Multiword DMA transfer timings */
struct ide_timings {
	int cycle;	/* minimum cycle time [ns] */
	int active;	/* minimum command active time [ns] */
};
static const struct ide_timings pio_timing[5] = {
	{ 600, 180 },    /* Mode 0 */
	{ 390, 150 },    /*      1 */
	{ 240, 105 },    /*      2 */
	{ 180,  90 },    /*      3 */
	{ 120,  75 }     /*      4 */
};
static const struct ide_timings dma_timing[3] = {
	{ 480, 240 },	/* Mode 0 */
	{ 165,  90 },	/* Mode 1 */
	{ 120,  75 }	/* Mode 2 */
};

static const struct ide_timings udma_timing[5] = {
	{ 120, 180 },	/* Mode 0 */
	{  90, 150 },	/* Mode 1 */
	{  60, 120 },	/* Mode 2 */
	{  45,  90 },	/* Mode 3 */
	{  30,  90 }	/* Mode 4 */
};

#define TIME_TO_TICK(time) howmany((time), 30)
#define PIO_REC_OFFSET 4
#define PIO_REC_MIN 1
#define PIO_ACT_MIN 1
#define DMA_REC_OFFSET 1
#define DMA_REC_MIN 1
#define DMA_ACT_MIN 1

#define ATA4_TIME_TO_TICK(time)  howmany((time), 15) /* 15 ns clock */

#define CONFIG_REG (0x200)		/* IDE access timing register */

void
wdc_obio_select(struct ata_channel *chp, int drive)
{
	struct wdc_obio_softc *sc = (struct wdc_obio_softc *)chp->ch_atac;
	struct wdc_regs *wdr = CHAN_TO_WDC_REGS(chp);

	bus_space_write_4(wdr->cmd_iot, wdr->cmd_baseioh,
			CONFIG_REG, sc->sc_dmaconf[drive]);
}

void
adjust_timing(struct ata_channel *chp)
{
	struct wdc_obio_softc *sc = (struct wdc_obio_softc *)chp->ch_atac;
	int drive;
	int min_cycle = 0, min_active = 0;
	int cycle_tick = 0, act_tick = 0, inact_tick = 0, half_tick;

	for (drive = 0; drive < 2; drive++) {
		u_int conf = 0;
		struct ata_drive_datas *drvp;
		
		drvp = &chp->ch_drive[drive];
		/* set up pio mode timings */
		if (drvp->drive_type != ATA_DRIVET_NONE) {
			int piomode = drvp->PIO_mode;
			min_cycle = pio_timing[piomode].cycle;
			min_active = pio_timing[piomode].active;
			
			cycle_tick = TIME_TO_TICK(min_cycle);
			act_tick = TIME_TO_TICK(min_active);
			if (act_tick < PIO_ACT_MIN)
				act_tick = PIO_ACT_MIN;
			inact_tick = cycle_tick - act_tick - PIO_REC_OFFSET;
			if (inact_tick < PIO_REC_MIN)
				inact_tick = PIO_REC_MIN;
			/* mask: 0x000007ff */
			conf |= (inact_tick << 5) | act_tick;
		}
		/* Set up DMA mode timings */
		if (drvp->drive_flags & ATA_DRIVE_DMA) {
			int dmamode = drvp->DMA_mode;
			min_cycle = dma_timing[dmamode].cycle;
			min_active = dma_timing[dmamode].active;
			cycle_tick = TIME_TO_TICK(min_cycle);
			act_tick = TIME_TO_TICK(min_active);
			inact_tick = cycle_tick - act_tick - DMA_REC_OFFSET;
			if (inact_tick < DMA_REC_MIN)
				inact_tick = DMA_REC_MIN;
			half_tick = 0;	/* XXX */
			/* mask: 0xfffff800 */
			conf |=
					(half_tick << 21) |
					(inact_tick << 16) | (act_tick << 11);
		}
#ifdef DEBUG
		if (conf) {
			printf("conf[%d] = 0x%x, cyc = %d (%d ns), act = %d (%d ns), inact = %d\n",
					drive, conf, cycle_tick, min_cycle, act_tick, min_active, inact_tick);
		}
#endif
		sc->sc_dmaconf[drive] = conf;
	}
	sc->sc_wdcdev.select = 0;
	if (sc->sc_dmaconf[0]) {
		wdc_obio_select(chp,0);
		if (sc->sc_dmaconf[1] &&
		    (sc->sc_dmaconf[0] != sc->sc_dmaconf[1])) {
			sc->sc_wdcdev.select = wdc_obio_select;
		}
	} else if (sc->sc_dmaconf[1]) {
		wdc_obio_select(chp,1);
	}
}

void
ata4_adjust_timing(struct ata_channel *chp)
{
	struct wdc_obio_softc *sc = (struct wdc_obio_softc *)chp->ch_atac;
	int drive;
	int min_cycle = 0, min_active = 0;
	int cycle_tick = 0, act_tick = 0, inact_tick = 0;

	for (drive = 0; drive < 2; drive++) {
		u_int conf = 0;
		struct ata_drive_datas *drvp;

		drvp = &chp->ch_drive[drive];
		/* set up pio mode timings */

		if (drvp->drive_type != ATA_DRIVET_NONE) {
			int piomode = drvp->PIO_mode;
			min_cycle = pio_timing[piomode].cycle;
			min_active = pio_timing[piomode].active;

			cycle_tick = ATA4_TIME_TO_TICK(min_cycle);
			act_tick = ATA4_TIME_TO_TICK(min_active);
			inact_tick = cycle_tick - act_tick;
			/* mask: 0x000003ff */
			conf |= (inact_tick << 5) | act_tick;
		}
		/* set up dma mode timings */
		if (drvp->drive_flags & ATA_DRIVE_DMA) {
			int dmamode = drvp->DMA_mode;
			min_cycle = dma_timing[dmamode].cycle;
			min_active = dma_timing[dmamode].active;
			cycle_tick = ATA4_TIME_TO_TICK(min_cycle);
			act_tick = ATA4_TIME_TO_TICK(min_active);
			inact_tick = cycle_tick - act_tick;
			/* mask: 0x001ffc00 */
			conf |= (act_tick << 10) | (inact_tick << 15);
		}
		/* set up udma mode timings */
		if (drvp->drive_flags & ATA_DRIVE_UDMA) {
			int udmamode = drvp->UDMA_mode;
			min_cycle = udma_timing[udmamode].cycle;
			min_active = udma_timing[udmamode].active;
			act_tick = ATA4_TIME_TO_TICK(min_active);
			cycle_tick = ATA4_TIME_TO_TICK(min_cycle);
			/* mask: 0x1ff00000 */
			conf |= (cycle_tick << 21) | (act_tick << 25) | 0x100000;
		}
#ifdef DEBUG
		if (conf) {
			printf("ata4 conf[%d] = 0x%x, cyc = %d (%d ns), act = %d (%d ns), inact = %d\n",
					drive, conf, cycle_tick, min_cycle, act_tick, min_active, inact_tick);
		}
#endif
		sc->sc_dmaconf[drive] = conf;
	}
	sc->sc_wdcdev.select = 0;
	if (sc->sc_dmaconf[0]) {
		wdc_obio_select(chp,0);
		if (sc->sc_dmaconf[1] &&
		    (sc->sc_dmaconf[0] != sc->sc_dmaconf[1])) {
			sc->sc_wdcdev.select = wdc_obio_select;
		}
	} else if (sc->sc_dmaconf[1]) {
		wdc_obio_select(chp,1);
	}
}

int
wdc_obio_detach(device_t self, int flags)
{
	struct wdc_obio_softc *sc = device_private(self);
	int error;

	if ((error = wdcdetach(self, flags)) != 0)
		return error;

	intr_disestablish(sc->sc_ih);

	/* Unmap our i/o space. */
	bus_space_unmap(sc->sc_wdcdev.regs->cmd_iot,
			sc->sc_wdcdev.regs->cmd_baseioh, WDC_REG_NPORTS << 4);

	/* Unmap DMA registers. */
	if (sc->sc_dmacmd != NULL) {

		bus_space_unmap(sc->sc_wdcdev.regs->cmd_iot,
		    sc->sc_dmaregh, 0x100);
		dbdma_free(sc->sc_dma, sizeof(dbdma_command_t) * 20);
	}
	return 0;
}

int
wdc_obio_dma_init(void *v, int channel, int drive, void *databuf,
	size_t datalen, int flags)
{
	struct wdc_obio_softc *sc = v;
	vaddr_t va = (vaddr_t)databuf;
	dbdma_command_t *cmdp;
	u_int cmd, offset;
	int read = flags & WDC_DMA_READ;

	cmdp = sc->sc_dmacmd;
	cmd = read ? DBDMA_CMD_IN_MORE : DBDMA_CMD_OUT_MORE;

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
wdc_obio_dma_start(void *v, int channel, int drive)
{
	struct wdc_obio_softc *sc = v;

	dbdma_start(sc->sc_dmareg, sc->sc_dmacmd);
}

int
wdc_obio_dma_finish(void *v, int channel, int drive, int read)
{
	struct wdc_obio_softc *sc = v;

	dbdma_stop(sc->sc_dmareg);
	return 0;
}
