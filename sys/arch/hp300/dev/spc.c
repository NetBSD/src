/* $NetBSD: spc.c,v 1.1 2003/08/01 01:18:46 tsutsui Exp $ */

/*
 * Copyright (c) 2003 Izumi Tsutsui.
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "opt_ddb.h"

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: spc.c,v 1.1 2003/08/01 01:18:46 tsutsui Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <machine/bus.h>
#include <machine/cpu.h>
#include <machine/intr.h>

#include <hp300/dev/dioreg.h>
#include <hp300/dev/diovar.h>
#include <hp300/dev/diodevs.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsi_message.h> 
#include <dev/scsipi/scsiconf.h>

#include <dev/ic/mb89352reg.h>
#include <dev/ic/mb89352var.h>

#include <hp300/dev/hp98265reg.h>
#include <hp300/dev/dmareg.h>
#include <hp300/dev/dmavar.h>

static int  spc_dio_match __P((struct device *, struct cfdata *, void *));
static void spc_dio_attach __P((struct device *, struct device *, void *));
static void spc_dio_dmastart __P((struct spc_softc *, void *, size_t, int));
static void spc_dio_dmadone __P((struct spc_softc *));
static void spc_dio_dmago __P((void *));
static void spc_dio_dmastop __P((void *));

struct spc_dio_softc {
	struct spc_softc sc_spc;	/* MI spc softc */

	/* DIO specific goo. */
	struct bus_space_tag sc_tag;	/* bus space tag with oddbyte func */
	bus_space_handle_t sc_iohsc;	/* bus space handle for HPSCSI */
	struct dmaqueue sc_dq;		/* DMA job queue */
	u_int sc_dflags;		/* DMA flags */
#define SCSI_DMA32	0x01		/* 32-bit DMA should be used */
#define SCSI_HAVEDMA	0x02		/* controller has DMA channel */
#define SCSI_DATAIN	0x04		/* DMA direction */
};

CFATTACH_DECL(spc, sizeof(struct spc_dio_softc),
    spc_dio_match, spc_dio_attach, NULL, NULL);

static int
spc_dio_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct dio_attach_args *da = aux;

	switch (da->da_id) {
	case DIO_DEVICE_ID_SCSI0:
	case DIO_DEVICE_ID_SCSI1:
	case DIO_DEVICE_ID_SCSI2:
	case DIO_DEVICE_ID_SCSI3:
		return 1;
	}

	return 0;
}

static void
spc_dio_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct spc_dio_softc *dsc = (struct spc_dio_softc *)self;
	struct spc_softc *sc = &dsc->sc_spc;
	struct dio_attach_args *da = aux;
	bus_space_tag_t iot = &dsc->sc_tag;
	bus_space_handle_t iohsc, iohspc;
	u_int8_t id;

	memcpy(iot, da->da_bst, sizeof(struct bus_space_tag));
	dio_set_bus_space_oddbyte(iot);

	if (bus_space_map(iot, da->da_addr, da->da_size, 0, &iohsc)) {
		printf(": can't map SCSI registers\n");
		return;
	}

	if (bus_space_subregion(iot, iohsc, SPC_OFFSET, SPC_SIZE, &iohspc)) {
		printf(": can't map SPC registers\n");
		return;
	}

	printf(": 98265A SCSI");

	bus_space_write_1(iot, iohsc, HPSCSI_ID, 0xff);
	DELAY(100);
	id = bus_space_read_1(iot, iohsc, HPSCSI_ID);
	if ((id & ID_WORD_DMA) == 0) {
		printf(", 32-bit DMA");
		dsc->sc_dflags |= SCSI_DMA32;
	}
	id &= ID_MASK;
	printf(", SCSI ID %d\n", id);

	sc->sc_iot = iot;
	sc->sc_ioh = iohspc;
	sc->sc_initiator = id;

	sc->sc_dma_start = spc_dio_dmastart;
	sc->sc_dma_done  = spc_dio_dmadone;

	dsc->sc_iohsc = iohsc;
	dsc->sc_dq.dq_softc = dsc;
	dsc->sc_dq.dq_start = spc_dio_dmago;
	dsc->sc_dq.dq_done  = spc_dio_dmastop;

	bus_space_write_1(iot, iohsc, HPSCSI_CSR, 0x00);
	bus_space_write_1(iot, iohsc, HPSCSI_HCONF, 0x00);

	dio_intr_establish(spc_intr, (void *)sc, da->da_ipl, IPL_BIO);

	spc_attach(sc);

	/* Enable SPC interrupts. */
	bus_space_write_1(iot, iohsc, HPSCSI_CSR, CSR_IE);
}

static
void spc_dio_dmastart(sc, addr, size, datain)
	struct spc_softc *sc;
	void *addr;
	size_t size;
	int datain;
{
	struct spc_dio_softc *dsc = (struct spc_dio_softc *)sc;

	dsc->sc_dq.dq_chan = DMA0 | DMA1;
	dsc->sc_dflags |= SCSI_HAVEDMA;
	if (datain)
		dsc->sc_dflags |= SCSI_DATAIN;
	else
		dsc->sc_dflags &= ~SCSI_DATAIN;

	if (dmareq(&dsc->sc_dq) != 0)
		/* DMA channel is available, so start DMA immediately */
		spc_dio_dmago((void *)dsc);
	/* else dma start function will be called later from dmafree(). */
}

static
void spc_dio_dmago(arg)
	void *arg;
{
	struct spc_dio_softc *dsc = (struct spc_dio_softc *)arg;
	struct spc_softc *sc = &dsc->sc_spc;
	bus_space_tag_t iot;
	bus_space_handle_t iohsc, iohspc;
	int len, chan;
	u_int32_t dmaflags;
	u_int8_t cmd;

	iot = sc->sc_iot;
	iohspc = sc->sc_ioh;
	iohsc = dsc->sc_iohsc;

	bus_space_write_1(iot, iohsc, HPSCSI_HCONF, 0);

	cmd = CSR_IE;
	dmaflags = DMAGO_NOINT;
	chan = dsc->sc_dq.dq_chan;
	if ((dsc->sc_dflags & SCSI_DATAIN) != 0) {
		cmd |= CSR_DMAIN;
		dmaflags |= DMAGO_READ;
	}
	if ((dsc->sc_dflags & SCSI_DMA32) != 0 &&
	    ((u_int)sc->sc_dp & 3) == 0 &&
	    (sc->sc_dleft & 3) == 0) {
		cmd |= CSR_DMA32;
		dmaflags |= DMAGO_LWORD;
	} else
		dmaflags |= DMAGO_WORD;

	dmago(chan, sc->sc_dp, sc->sc_dleft, dmaflags);

	bus_space_write_1(iot, iohsc, HPSCSI_CSR, cmd);
	cmd |= (chan == 0) ? CSR_DE0 : CSR_DE1;
	bus_space_write_1(iot, iohsc, HPSCSI_CSR, cmd);

	cmd = SCMD_XFR;
	len = sc->sc_dleft;

	if ((len & (DEV_BSIZE - 1)) != 0) /* XXX ??? */ {
		cmd |= SCMD_PAD;
#if 0
		if ((dsc->sc_dflags & SCSI_DATAIN) != 0)
			len += 2; /* XXX ??? */
#endif
	}

	bus_space_write_1(iot, iohspc, TCH, len >> 16);
	bus_space_write_1(iot, iohspc, TCM, len >>  8);
	bus_space_write_1(iot, iohspc, TCL, len);
	bus_space_write_1(iot, iohspc, PCTL, sc->sc_phase | PCTL_BFINT_ENAB);
	bus_space_write_1(iot, iohspc, SCMD, cmd);

	sc->sc_flags |= SPC_DOINGDMA;
}

static
void spc_dio_dmadone(sc)
	struct spc_softc *sc;
{
	struct spc_dio_softc *dsc = (struct spc_dio_softc *)sc;
	bus_space_tag_t iot;
	bus_space_handle_t ioh, iohsc;
	int resid, trans;
	u_int8_t cmd;

	iot = sc->sc_iot;
	ioh = sc->sc_ioh;
	iohsc = dsc->sc_iohsc;

	/* wait DMA complete */
	if ((bus_space_read_1(iot, ioh, SSTS) & SSTS_BUSY) != 0) {
		int timeout = 1000; /* XXX how long? */
		while ((bus_space_read_1(iot, ioh, SSTS) & SSTS_BUSY) != 0) {
			if (--timeout < 0)
				printf("%s: DMA complete timeout\n",
				    sc->sc_dev.dv_xname);
			DELAY(1);
		}
	}

	if ((dsc->sc_dflags & SCSI_HAVEDMA) != 0) {
		dmafree(&dsc->sc_dq);
		dsc->sc_dflags &= ~SCSI_HAVEDMA;
	}

	cmd = bus_space_read_1(iot, iohsc, HPSCSI_CSR);
	cmd &= ~(CSR_DE1|CSR_DE0);
	bus_space_write_1(iot, iohsc, HPSCSI_CSR, cmd);

	resid = bus_space_read_1(iot, ioh, TCH) << 16 |
	    bus_space_read_1(iot, ioh, TCM) << 8 |
	    bus_space_read_1(iot, ioh, TCL);
	trans = sc->sc_dleft - resid;
	sc->sc_dp += trans;
	sc->sc_dleft -= trans;

	sc->sc_flags &= ~SPC_DOINGDMA;
}

static
void spc_dio_dmastop(arg)
	void *arg;
{
	struct spc_dio_softc *dsc = (struct spc_dio_softc *)arg;
	struct spc_softc *sc = &dsc->sc_spc;
	u_int8_t cmd;

	/* XXX When is this function called? */
	cmd = bus_space_read_1(sc->sc_iot, dsc->sc_iohsc, HPSCSI_CSR);
	cmd &= ~(CSR_DE1|CSR_DE0);
	bus_space_write_1(sc->sc_iot, dsc->sc_iohsc, HPSCSI_CSR, cmd);

	dsc->sc_dflags &= ~SCSI_HAVEDMA;
	sc->sc_flags &= ~SPC_DOINGDMA;
}
