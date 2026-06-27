/*	$NetBSD: mpc5200_ata.c,v 1.1 2026/06/27 13:28:35 rkujawa Exp $	*/

/*-
 * Copyright (c) 2008, 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Radoslaw Kujawa and Robert Swindells.
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

/*
 * Driver for the Freescale MPC5200B on-chip ATA Controller, in PIO mode.
 *
 * This is a reworking of Robert Swindells' MPC5200 IDE driver (mpc5200_ide.c).
 *
 * DMA is deliberately not implemented. The MPC5200B can ONLY reach memory
 * from the ATA FIFO through the BestComm SDMA engine, and that path is both
 * documented-broken and widely reported to silently corrupt data when caches 
 * are on. 
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mpc5200_ata.c,v 1.1 2026/06/27 13:28:35 rkujawa Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/bus.h>

#include <sys/endian.h>

#include <machine/intr.h>

#include <dev/ata/atavar.h>
#include <dev/ic/wdcvar.h>

#include <powerpc/mpc5200/obiovar.h>
#include <powerpc/mpc5200/cdmvar.h>
#include <powerpc/mpc5200/mpc5200reg.h>
#include <powerpc/mpc5200/mpc5200_atareg.h>
#include <powerpc/mpc5200/mpc5200_atavar.h>

/*
 * The ATA controller's INTRQ is hardwired to SIU peripheral source 7
 */
#define	MPCATA_PERIPH_SOURCE	7
#define	MPCATA_IRQ		(64 + MPCATA_PERIPH_SOURCE)

/* Highest PIO mode the controller and this driver support. */
#define	MPCATA_PIO_MAX		4

/*
 * Transceiver propagation delay added to the read-strobe widths (t2_8/t2_16),
 */
#define	MPCATA_XCVR_PROP_NS	10

static int	mpcata_match(device_t, cfdata_t, void *);
static void	mpcata_attach(device_t, device_t, void *);

static void	mpcata_set_modes(struct ata_channel *);
static void	mpcata_program_pio(struct mpcata_softc *, int);
static void	mpcata_datain(struct ata_channel *, int, void *, size_t);
static void	mpcata_dataout(struct ata_channel *, int, void *, size_t);

CFATTACH_DECL_NEW(mpcata, sizeof(struct mpcata_softc),
    mpcata_match, mpcata_attach, NULL, NULL);

/*
 * ATA-4 PIO-mode minimum bus timings
 */
static const struct mpcata_pio_timing {
	uint16_t	t0;	/* cycle time			*/
	uint16_t	t1;	/* address setup to DIOR/DIOW	*/
	uint16_t	t2_8;	/* 8-bit DIOR/DIOW pulse width	*/
	uint16_t	t2_16;	/* 16-bit DIOR/DIOW pulse width	*/
	uint16_t	t4;	/* DIOW data hold		*/
	uint16_t	ta;	/* IORDY setup			*/
} mpcata_pio_timing[] = {
	{ 600, 70, 290, 165, 30, 35 },	/* PIO mode 0 */
	{ 383, 50, 290, 125, 20, 35 },	/* PIO mode 1 */
	{ 240, 30, 290, 100, 15, 35 },	/* PIO mode 2 */
	{ 180, 30,  80,  80, 10, 35 },	/* PIO mode 3 */
	{ 120, 25,  70,  70, 10, 35 },	/* PIO mode 4 */
};

static int
mpcata_match(device_t parent, cfdata_t cf, void *aux)
{
	struct obio_attach_args *oba = aux;

	if (strcmp(oba->obio_name, "ata") == 0)
		return 1;

	return 0;
}

static void
mpcata_attach(device_t parent, device_t self, void *aux)
{
	struct mpcata_softc *sc = device_private(self);
	struct obio_attach_args *oba = aux;
	struct ata_channel *chp = &sc->sc_channel;
	struct wdc_regs *wdr = &sc->sc_wdc_regs;
	bus_addr_t addr;
	bus_size_t size;
	int irq, ist, i;

	aprint_naive("\n");
	aprint_normal(": MPC5200 ATA controller\n");

	sc->sc_iot = oba->obio_bst;

	/*
	 * Map the whole ATA register block.
	 */
	addr = oba->obio_addr != 0 ? oba->obio_addr :
	    (MPC5200_MBAR_DEFAULT + MPC5200_REG_ATA);
	size = oba->obio_size != 0 ? oba->obio_size : ATA_REG_SIZE;
	if (bus_space_map(sc->sc_iot, addr, size, 0, &sc->sc_ioh) != 0) {
		aprint_error_dev(self, "can't map registers\n");
		return;
	}

	sc->sc_wdcdev.sc_atac.atac_dev = self;
	sc->sc_wdcdev.regs = wdr;

	/*
	 * Lay out the wdc command/control handles.
	 */
	wdr->cmd_iot = wdr->ctl_iot = sc->sc_iot;
	wdr->cmd_baseioh = sc->sc_ioh;
	wdr->cmd_ios = size;
	for (i = 0; i < WDC_NREG; i++) {
		if (bus_space_subregion(wdr->cmd_iot, sc->sc_ioh,
		    ATA_DRIVE_BASE + i * ATA_DRIVE_STRIDE,
		    (i == 0) ? 2 : 1, &wdr->cmd_iohs[i]) != 0) {
			aprint_error_dev(self, "can't subregion taskfile\n");
			goto fail;
		}
	}
	if (bus_space_subregion(wdr->ctl_iot, sc->sc_ioh, ATA_DRIVE_CTRL, 1,
	    &wdr->ctl_ioh) != 0) {
		aprint_error_dev(self, "can't subregion control register\n");
		goto fail;
	}
	wdc_init_shadow_regs(wdr);

	/* Fill in the wdc/channel description. */
	sc->sc_wdcdev.sc_atac.atac_cap = ATAC_CAP_DATA16;
	sc->sc_wdcdev.sc_atac.atac_pio_cap = MPCATA_PIO_MAX;
	sc->sc_wdcdev.sc_atac.atac_channels = sc->sc_chanarray;
	sc->sc_wdcdev.sc_atac.atac_nchannels = 1;
	sc->sc_wdcdev.sc_atac.atac_set_modes = mpcata_set_modes;
	sc->sc_wdcdev.wdc_maxdrives = 2;
	/* The 16-bit data register is byte-swapped relative to the host. */
	sc->sc_wdcdev.datain_pio = mpcata_datain;
	sc->sc_wdcdev.dataout_pio = mpcata_dataout;

	sc->sc_chanarray[0] = chp;
	chp->ch_channel = 0;
	chp->ch_atac = &sc->sc_wdcdev.sc_atac;

	/*
	 * Bring the host state machine to a known state
	 */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, ATA_CONFIG,
	    ATA_CONFIG_SMR | ATA_CONFIG_FR);
	delay(10);
	mpcata_program_pio(sc, 0);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, ATA_CONFIG,
	    ATA_CONFIG_IE | ATA_CONFIG_IORDY);
	/* Clear any latched host read/write error flags (write-1-to-clear). */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, ATA_HOST_STATUS,
	    ATA_HOST_STATUS_RERR | ATA_HOST_STATUS_WERR);

	/*
	 * Establish the drive interrupt.
	 */
	if (!obio_decode_interrupt(oba->obio_node, 0, &irq, &ist)) {
		irq = MPCATA_IRQ;
		ist = IST_LEVEL;
	}
	sc->sc_ih = intr_establish(irq, ist, IPL_BIO, wdcintr, chp);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self,
		    "can't establish irq %d, using polled I/O\n", irq);
		sc->sc_wdcdev.sc_atac.atac_cap |= ATAC_CAP_NOIRQ;
	} else {
		aprint_normal_dev(self, "interrupting at irq %d\n", irq);
	}

	wdcattach(chp);
	return;

fail:
	bus_space_unmap(sc->sc_iot, sc->sc_ioh, size);
}

static uint8_t
mpcata_ns_to_count(uint32_t ns, uint32_t ipb_hz, uint32_t extra_ns)
{
	uint64_t period_ps = 1000000000000ULL / ipb_hz;
	uint64_t spec_ps = (uint64_t)(ns + extra_ns) * 1000;
	uint64_t count = (spec_ps + period_ps - 1) / period_ps;

	if (count > 255)
		count = 255;
	return (uint8_t)count;
}

/*
 * Program the host PIO timing registers for the given mode.
 */
static void
mpcata_program_pio(struct mpcata_softc *sc, int mode)
{
	const struct mpcata_pio_timing *t;
	uint32_t ipb, pio1, pio2;
	uint8_t t0, t1, t2_8, t2_16, t4, ta;

	if (mode < 0 || mode > MPCATA_PIO_MAX)
		mode = 0;
	t = &mpcata_pio_timing[mode];
	ipb = mpc5200_cdm_get_ipb_freq();	/* never returns zero */

	t0    = mpcata_ns_to_count(t->t0,    ipb, 0);
	t1    = mpcata_ns_to_count(t->t1,    ipb, 0);
	t2_8  = mpcata_ns_to_count(t->t2_8,  ipb, 2 * MPCATA_XCVR_PROP_NS);
	t2_16 = mpcata_ns_to_count(t->t2_16, ipb, 2 * MPCATA_XCVR_PROP_NS);
	t4    = mpcata_ns_to_count(t->t4,    ipb, 0);
	ta    = mpcata_ns_to_count(t->ta,    ipb, 0);

	pio1 = ((uint32_t)t0 << 24) | ((uint32_t)t2_8 << 16) |
	    ((uint32_t)t2_16 << 8);
	pio2 = ((uint32_t)t4 << 24) | ((uint32_t)t1 << 16) |
	    ((uint32_t)ta << 8);

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, ATA_PIO1, pio1);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, ATA_PIO2, pio2);
	sc->sc_pio_mode = mode;
}

static void
mpcata_set_modes(struct ata_channel *chp)
{
	struct mpcata_softc *sc = (struct mpcata_softc *)chp->ch_atac;
	int drive, mode = MPCATA_PIO_MAX;

	for (drive = 0; drive < sc->sc_wdcdev.wdc_maxdrives; drive++) {
		struct ata_drive_datas *drvp = &chp->ch_drive[drive];

		if (drvp->drive_type == ATA_DRIVET_NONE)
			continue;
		if (drvp->PIO_mode < mode)
			mode = drvp->PIO_mode;
	}

	mpcata_program_pio(sc, mode);

	for (drive = 0; drive < sc->sc_wdcdev.wdc_maxdrives; drive++) {
		struct ata_drive_datas *drvp = &chp->ch_drive[drive];

		if (drvp->drive_type == ATA_DRIVET_NONE)
			continue;
		drvp->PIO_mode = sc->sc_pio_mode;
		drvp->drive_flags &= ~(ATA_DRIVE_DMA | ATA_DRIVE_UDMA);
	}
}

/* 
 * Surely this can be done in a more elegant way, but the controller is slow enough
 * that it doesn't matter. 
 */
static void
mpcata_datain(struct ata_channel *chp, int flags, void *bf, size_t len)
{
	struct wdc_regs *wdr = CHAN_TO_WDC_REGS(chp);
	uint16_t *p = bf;

	while (len >= sizeof(*p)) {
		*p++ = htole16(bus_space_read_2(wdr->cmd_iot,
		    wdr->cmd_iohs[wd_data], 0));
		len -= sizeof(*p);
	}
}

static void
mpcata_dataout(struct ata_channel *chp, int flags, void *bf, size_t len)
{
	struct wdc_regs *wdr = CHAN_TO_WDC_REGS(chp);
	uint16_t *p = bf;

	while (len >= sizeof(*p)) {
		bus_space_write_2(wdr->cmd_iot, wdr->cmd_iohs[wd_data], 0,
		    le16toh(*p++));
		len -= sizeof(*p);
	}
}

