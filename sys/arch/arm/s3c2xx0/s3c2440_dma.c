/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Fleischer <paul@xpg.dk>
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/queue.h>

#include <sys/mutex.h>
#include <sys/condvar.h>

#include <machine/cpu.h>
#include <sys/bus.h>

#include <arch/arm/s3c2xx0/s3c2440_dma.h>

#include <arm/s3c2xx0/s3c2440var.h>
#include <arch/arm/s3c2xx0/s3c2440reg.h>

#include <uvm/uvm_extern.h>
#include <machine/pmap.h>

//#define S3C2440_DMA_DEBUG
#ifdef S3C2440_DMA_DEBUG
#define DPRINTF(s) do {printf s; } while (/*CONSTCOND*/0)
#else
#define DPRINTF(s) do {} while (/*CONSTCOND*/0)
#endif

#define DMAC_N_CHANNELS 4

struct dmac_desc_segs {
	bus_dma_segment_t	*ds_curseg;
	uint8_t			ds_nsegs;
};

SIMPLEQ_HEAD(dmac_xfer_state_head, dmac_xfer_state);

struct dmac_xfer_state {
	struct dmac_xfer		dxs_xfer;
	SIMPLEQ_ENTRY(dmac_xfer_state)	dxs_link;
	uint8_t				dxs_channel;
#define DMAC_NO_CHANNEL (~0)
	uint8_t				dxs_width;
	bool				dxs_complete;
	struct dmac_desc_segs		dxs_segs[2];
	uint32_t			dxs_options;
};

struct s3c2440_dmac_peripheral {
	uint8_t	dp_id;
	uint8_t	dp_channel_order[DMAC_N_CHANNELS+1];
#define PERPH_LAST DMAC_N_CHANNELS+1
	uint8_t	dp_channel_source[DMAC_N_CHANNELS];
#define PERPH_NA 7
};

struct s3c2440_dmac_channel {
	struct dmac_xfer_state		*dc_active; /* Active transfer, NULL if none */

	/* Request queue. Can easily be extended to support multiple
	   priorities */
	struct dmac_xfer_state_head	dc_queue;
};

struct s3c2440_dmac_softc {
	bus_space_tag_t			sc_iot;
	bus_space_handle_t		sc_dmach;
	bus_dma_tag_t			sc_dmat;
	struct kmutex			sc_mutex;
	struct s3c2440_dmac_channel	sc_channels[DMAC_N_CHANNELS];
	struct kmutex			sc_intr_mutex;
	struct kcondvar			sc_intr_cv;
};

static struct s3c2440_dmac_softc _s3c2440_dmac_sc;
static struct s3c2440_dmac_softc *s3c2440_dmac_sc = &_s3c2440_dmac_sc;

/* TODO: Consider making the order configurable. */
static struct s3c2440_dmac_peripheral s3c2440_peripherals[] = {
{DMAC_PERIPH_NONE, {0,1,2,3}, {0, 0, 0, 0}},
{DMAC_PERIPH_XDREQ0, {0,PERPH_LAST}, {0, PERPH_NA, PERPH_NA, PERPH_NA}},
{DMAC_PERIPH_XDREQ1, {1,PERPH_LAST}, {PERPH_NA, 0, PERPH_NA, PERPH_NA}},
{DMAC_PERIPH_UART0, {0,PERPH_LAST}, {1, PERPH_NA, PERPH_NA, PERPH_NA}},
{DMAC_PERIPH_UART1, {1,PERPH_LAST}, {PERPH_NA, 1, PERPH_NA, PERPH_NA}},
{DMAC_PERIPH_UART2, {3,PERPH_LAST}, {PERPH_NA, PERPH_NA, PERPH_NA, 0}},
{DMAC_PERIPH_I2SSDO, {0, 2, PERPH_LAST}, {5, PERPH_NA, 0, PERPH_NA}},
{DMAC_PERIPH_I2SSDI, {1, 2, PERPH_LAST}, {PERPH_NA, 2, 1, PERPH_NA}},
{DMAC_PERIPH_SDI, {3, 2, 1, PERPH_LAST}, {2, 6, 2, 1}},
{DMAC_PERIPH_SPI0, {1, PERPH_LAST}, {PERPH_NA, 3, PERPH_NA, PERPH_NA}},
{DMAC_PERIPH_SPI1, {3, PERPH_LAST}, {PERPH_NA, PERPH_NA, PERPH_NA, 2}},
{DMAC_PERIPH_PCMIN, {0, 2, PERPH_LAST}, {6, PERPH_NA, 5, PERPH_NA}},
{DMAC_PERIPH_PCMOUT, {1, 3, PERPH_LAST}, {PERPH_NA, 5, PERPH_NA, 6}},
{DMAC_PERIPH_MICIN, {2, 3, PERPH_LAST}, {PERPH_NA, PERPH_NA, 6, 5}},
{DMAC_PERIPH_MICOUT, {2, 3, PERPH_LAST}, {PERPH_NA, PERPH_NA, 6, 5}},
{DMAC_PERIPH_TIMER, {0, 2, 3, PERPH_LAST}, {3, PERPH_NA, 3, 3}},
{DMAC_PERIPH_USBEP1, {0, PERPH_LAST}, {4, PERPH_NA, PERPH_NA, PERPH_NA}},
{DMAC_PERIPH_USBEP2, {1, PERPH_LAST}, {PERPH_NA, 4, PERPH_NA, PERPH_NA}},
{DMAC_PERIPH_USBEP3, {2, PERPH_LAST}, {PERPH_NA, PERPH_NA, 4, PERPH_NA}},
{DMAC_PERIPH_USBEP4, {3, PERPH_LAST}, {PERPH_NA, PERPH_NA, PERPH_NA, 4}}
};

static void dmac_start(uint8_t channel_no, struct dmac_xfer_state*);
static void dmac_transfer_segment(uint8_t channel_no, struct dmac_xfer_state*);
static void dmac_channel_done(uint8_t channel_no);

void
s3c2440_dma_init(void)
{
	struct s3c2440_dmac_softc *sc = s3c2440_dmac_sc;
	int i;

	sc->sc_iot = s3c2xx0_softc->sc_iot;
	sc->sc_dmach = s3c2xx0_softc->sc_dmach;
	sc->sc_dmat = s3c2xx0_softc->sc_dmat;
	for(i = 0; i<DMAC_N_CHANNELS; i++) {
		sc->sc_channels[i].dc_active = NULL;
		SIMPLEQ_INIT(&sc->sc_channels[i].dc_queue);
	}

	mutex_init(&sc->sc_mutex, MUTEX_DEFAULT, IPL_BIO);

	mutex_init(&sc->sc_intr_mutex, MUTEX_DEFAULT, IPL_BIO);
	cv_init(&sc->sc_intr_cv, "s3c2440_dmaintr");

	/* Setup interrupt handler for DMA controller */
	s3c24x0_intr_establish(S3C24X0_INT_DMA0, IPL_BIO,
			       IST_EDGE_RISING, s3c2440_dma_intr, (void*)1);
	s3c24x0_intr_establish(S3C24X0_INT_DMA1, IPL_BIO,
			       IST_EDGE_RISING, s3c2440_dma_intr, (void*)2);
	s3c24x0_intr_establish(S3C24X0_INT_DMA2, IPL_BIO,
			       IST_EDGE_RISING, s3c2440_dma_intr, (void*)3);
	s3c24x0_intr_establish(S3C24X0_INT_DMA3, IPL_BIO,
			       IST_EDGE_RISING, s3c2440_dma_intr, (void*)4);
}

int
s3c2440_dma_intr(void *arg)
{
	/*struct s3c2xx0_softc *sc = s3c2xx0_softc;*/
	struct s3c2440_dmac_softc *sc;
	uint32_t status;
	int channel;
	struct s3c2440_dmac_channel *dc;

	sc = s3c2440_dmac_sc;

	channel = (int)arg - 1;
	dc = &sc->sc_channels[channel];

	DPRINTF(("s3c2440_dma_intr\n"));
	DPRINTF(("Channel %d\n", channel));

	status = bus_space_read_4(sc->sc_iot, sc->sc_dmach, DMA_STAT(channel));
	DPRINTF(("Channel %d status: %d\n", channel, status));

	if ( !(status & DMASTAT_BUSY) ) {
		struct dmac_xfer_state *dxs;
		struct dmac_xfer *dx;

		dxs = dc->dc_active;
		KASSERT(dxs != NULL);

		dx = &dxs->dxs_xfer;

		if (dx->dx_desc[DMAC_DESC_SRC].xd_increment) {
			dxs->dxs_segs[DMAC_DESC_SRC].ds_nsegs--;
			if (dxs->dxs_segs[DMAC_DESC_SRC].ds_nsegs == 0) {
				dxs->dxs_complete = TRUE;
			} else {
				dxs->dxs_segs[DMAC_DESC_SRC].ds_curseg++;
			}
		}
		if (dx->dx_desc[DMAC_DESC_DST].xd_increment) {
			dxs->dxs_segs[DMAC_DESC_DST].ds_nsegs--;
			if (dxs->dxs_segs[DMAC_DESC_DST].ds_nsegs == 0) {
				dxs->dxs_complete = TRUE;
			} else {
				dxs->dxs_segs[DMAC_DESC_DST].ds_curseg++;
			}
		}

		if (dxs->dxs_complete) {
			dxs->dxs_channel = DMAC_NO_CHANNEL;

			/* Lock the DMA mutex before tampering with
			   the channel.
			*/
			mutex_enter(&sc->sc_mutex);
			dmac_channel_done(channel);
			mutex_exit(&sc->sc_mutex);

			DPRINTF(("dx_done: %p\n", (void*)dx->dx_done));
			if (dx->dx_done != NULL) {
				(dx->dx_done)(dx, dx->dx_cookie);
			}
		} else {
			dmac_transfer_segment(channel, dxs);
		}
	}
#if 0
	if ( !(status & DMASTAT_BUSY) ) {
		s3c2440_dma_xfer_t xfer;

		xfer = dma_channel_xfer[channel];
		dma_channel_xfer[channel] = NULL;

		DPRINTF((" Channel %d completed transfer\n", channel));

		if (xfer->dx_remaining > 0 &&
		    xfer->dx_aborted == FALSE) {

			DPRINTF(("Preparing next transfer\n"));

			s3c2440_dma_xfer_start(xfer);
		} else {
			if (!xfer->dx_aborted && xfer->dx_callback != NULL)
				(xfer->dx_callback)(xfer->dx_callback_arg);

			xfer->dx_complete = TRUE;
		}

	}
#endif

#ifdef S3C2440_DMA_DEBUG
	status = bus_space_read_4(sc->sc_iot, sc->sc_dmach, DMA_CSRC(channel));
	printf("Current source for channel %d: 0x%x\n", channel, status);

	status = bus_space_read_4(sc->sc_iot, sc->sc_dmach, DMA_CDST(channel));
	printf("Current dest   for channel %d: 0x%x\n", channel, status);
#endif

	/* TODO: Remove this as it activates any thread waiting for a transfer
	   to complete */
	mutex_enter(&sc->sc_intr_mutex);
	DPRINTF(("cv_broadcast\n"));
	cv_broadcast(&sc->sc_intr_cv);
	DPRINTF(("cv_broadcast done\n"));
	mutex_exit(&sc->sc_intr_mutex);

	return 1;
}

dmac_xfer_t
s3c2440_dmac_allocate_xfer(int flags) {
	struct dmac_xfer_state *dxs;

	dxs = malloc(sizeof(struct dmac_xfer_state), M_DEVBUF, flags);

	dxs->dxs_xfer.dx_done = NULL;
	dxs->dxs_xfer.dx_sync_bus = DMAC_SYNC_BUS_AUTO;
	dxs->dxs_xfer.dx_xfer_mode = DMAC_XFER_MODE_DEMAND;
	dxs->dxs_channel = DMAC_NO_CHANNEL;

	return ((dmac_xfer_t)dxs);
}

void
s3c2440_dmac_free_xfer(dmac_xfer_t dx) {
	free(dx, M_DEVBUF);
}

int
s3c2440_dmac_start_xfer(dmac_xfer_t dx) {
	struct s3c2440_dmac_softc *sc = s3c2440_dmac_sc;
	struct dmac_xfer_state *dxs = (struct dmac_xfer_state*)dx;
	struct s3c2440_dmac_peripheral *perph;
	int i;
	bool transfer_started = FALSE;

	if (dxs->dxs_xfer.dx_peripheral != DMAC_PERIPH_NONE &&
	    dxs->dxs_xfer.dx_peripheral >= DMAC_N_PERIPH)
		return EINVAL;

	dxs->dxs_complete = FALSE;

	perph = &s3c2440_peripherals[dxs->dxs_xfer.dx_peripheral];
#ifdef DIAGNOSTIC
	DPRINTF(("dp_id: %d, dx_peripheral: %d\n", perph->dp_id, dxs->dxs_xfer.dx_peripheral));
	KASSERT(perph->dp_id == dxs->dxs_xfer.dx_peripheral);
#endif

	mutex_enter(&sc->sc_mutex);
	/* Get list of possible channels for this peripheral.
	   If none of the channels are ready to transmit, queue
	   the transfer in the one with the highest priority
	   (first in the order list).
	 */
	for(i=0;
	    perph->dp_channel_order[i] != PERPH_LAST;
	    i++) {
		uint8_t channel_no = perph->dp_channel_order[i];

#ifdef DIAGNOSTIC
		/* Check that there is a mapping for the given channel.
		   If this fails, there is something wrong in
		   s3c2440_peripherals.
		 */
		KASSERT(perph->dp_channel_source[channel_no] != PERPH_NA);
#endif

		if (sc->sc_channels[channel_no].dc_active == NULL) {
			/* Transfer can start right away */
			dmac_start(channel_no, dxs);
			transfer_started = TRUE;
			break;
		}
	}

	if (transfer_started == FALSE) {
		uint8_t channel_no = perph->dp_channel_order[0];
		/* Enqueue the transfer, as none of the DMA channels were
		   available.
		   The highest priority channel is used.
		*/
		dxs->dxs_channel = channel_no;
		SIMPLEQ_INSERT_TAIL(&sc->sc_channels[channel_no].dc_queue, dxs, dxs_link);
		DPRINTF(("Enqueued transfer on channel %d\n", channel_no));
	}

	mutex_exit(&sc->sc_mutex);

	return 0;
}

static void
dmac_start(uint8_t channel_no, struct dmac_xfer_state *dxs) {
	struct s3c2440_dmac_softc	*sc = s3c2440_dmac_sc;
	struct s3c2440_dmac_channel	*dc = &sc->sc_channels[channel_no];
	uint32_t			options;
#ifdef DIAGNOSTIC
	uint32_t			reg;
#endif
	dmac_sync_bus_t			sync_bus;
	struct dmac_xfer		*dx = &dxs->dxs_xfer;

	/* Must be called with sc_mutex locked */

	DPRINTF(("Starting DMA transfer (%p) on channel %d\n", dxs, channel_no));

	KASSERT(dc->dc_active == NULL);

#ifdef DIAGNOSTIC
	reg = bus_space_read_4(sc->sc_iot, sc->sc_dmach, DMA_STAT(channel_no));
	if (reg & DMASTAT_BUSY)
		panic("DMA channel is busy, cannot start new transfer!");

#endif

	dc->dc_active = dxs;
	dxs->dxs_channel = channel_no;
	dxs->dxs_segs[DMAC_DESC_SRC].ds_curseg = dx->dx_desc[DMAC_DESC_SRC].xd_dma_segs;
	dxs->dxs_segs[DMAC_DESC_SRC].ds_nsegs = dx->dx_desc[DMAC_DESC_SRC].xd_nsegs;
	dxs->dxs_segs[DMAC_DESC_DST].ds_curseg = dx->dx_desc[DMAC_DESC_DST].xd_dma_segs;
	dxs->dxs_segs[DMAC_DESC_DST].ds_nsegs = dx->dx_desc[DMAC_DESC_DST].xd_nsegs;

	options = DMACON_INT_INT |
		DMACON_RELOAD_NO_AUTO;

	if (dxs->dxs_xfer.dx_peripheral == DMAC_PERIPH_NONE) {
		options |= DMACON_SERVMODE_WHOLE;
	} else {
		options |= DMACON_SERVMODE_SINGLE;
	}

	switch (dxs->dxs_xfer.dx_xfer_mode) {
	case DMAC_XFER_MODE_DEMAND:
		options |= DMACON_DEMAND;
		break;
	case DMAC_XFER_MODE_HANDSHAKE:
		options |= DMACON_HANDSHAKE;
		break;
	default:
		panic("Unknown dx_xfer_mode");
	}

	sync_bus = dxs->dxs_xfer.dx_sync_bus;

	switch (dxs->dxs_xfer.dx_xfer_width) {
	case DMAC_XFER_WIDTH_8BIT:
		DPRINTF(("8-Bit (BYTE) transfer width\n"));
		options |= DMACON_DSZ_B;
		dxs->dxs_width = 1;
		break;
	case DMAC_XFER_WIDTH_16BIT:
		DPRINTF(("16-Bit (HALF-WORD) transfer width\n"));
		options |= DMACON_DSZ_HW;
		dxs->dxs_width = 2;
		break;
	case DMAC_XFER_WIDTH_32BIT:
		DPRINTF(("32-Bit (WORD) transfer width\n"));
		options |= DMACON_DSZ_W;
		dxs->dxs_width = 4;
		break;
	default:
		panic("Unknown transfer width");
	}

	if (dxs->dxs_xfer.dx_peripheral == DMAC_PERIPH_NONE) {
		options |= DMACON_SW_REQ;
		if (sync_bus == DMAC_SYNC_BUS_AUTO)
			sync_bus = DMAC_SYNC_BUS_SYSTEM;
	} else {
		uint8_t source = s3c2440_peripherals[dxs->dxs_xfer.dx_peripheral].dp_channel_source[channel_no];
		DPRINTF(("Hw request source: %d, channel: %d\n", source, channel_no));
		options |= DMACON_HW_REQ | DMACON_HW_SRCSEL(source);
		if (sync_bus == DMAC_SYNC_BUS_AUTO)
			sync_bus = DMAC_SYNC_BUS_PERIPHERAL;
	}

	if (sync_bus == DMAC_SYNC_BUS_SYSTEM) {
		DPRINTF(("Syncing with system bus\n"));
		options |= DMACON_SYNC_AHB;
	} else if (sync_bus == DMAC_SYNC_BUS_PERIPHERAL) {
		DPRINTF(("Syncing with peripheral bus\n"));
		options |= DMACON_SYNC_APB;
	} else {
		panic("No sync bus given");
	}

	dxs->dxs_options = options;

	/* We have now configured the options that will hold for all segment transfers.
	   Next, we prepare and start the transfer for the first segment */
	dmac_transfer_segment(channel_no, dxs);

}

static void
dmac_transfer_segment(uint8_t channel_no, struct dmac_xfer_state *dxs)
{
	struct s3c2440_dmac_softc	*sc = s3c2440_dmac_sc;
	/*	struct s3c2440_dmac_channel	*dc = &sc->sc_channels[channel_no];*/
	uint32_t			reg, transfer_size;
	struct dmac_xfer		*dx = &dxs->dxs_xfer;

	DPRINTF(("dmac_transfer_segment\n"));

	/* Prepare the source */
	bus_space_write_4(sc->sc_iot, sc->sc_dmach,
			  DMA_DISRC(channel_no),
			  dxs->dxs_segs[DMAC_DESC_SRC].ds_curseg->ds_addr);

	DPRINTF(("Source address: 0x%x\n", (unsigned)dxs->dxs_segs[DMAC_DESC_SRC].ds_curseg->ds_addr));
	DPRINTF(("Dest.  address: 0x%x\n", (unsigned)dxs->dxs_segs[DMAC_DESC_DST].ds_curseg->ds_addr));
	reg = 0;
	if (dx->dx_desc[DMAC_DESC_SRC].xd_bus_type == DMAC_BUS_TYPE_PERIPHERAL) {
		reg |= DISRCC_LOC_APB;
	} else {
		reg |= DISRCC_LOC_AHB;
	}
	if (dx->dx_desc[DMAC_DESC_SRC].xd_increment) {
		reg |= DISRCC_INC_INC;
	} else {
		reg |= DISRCC_INC_FIXED;
	}
	bus_space_write_4(sc->sc_iot, sc->sc_dmach, DMA_DISRCC(channel_no), reg);

	/* Prepare the destination */
	bus_space_write_4(sc->sc_iot, sc->sc_dmach,
			  DMA_DIDST(channel_no),
			  dxs->dxs_segs[DMAC_DESC_DST].ds_curseg->ds_addr);
	reg = 0;
	if (dx->dx_desc[DMAC_DESC_DST].xd_bus_type == DMAC_BUS_TYPE_PERIPHERAL) {
		reg |= DIDSTC_LOC_APB;
	} else {
		reg |= DIDSTC_LOC_AHB;
	}
	if (dx->dx_desc[DMAC_DESC_DST].xd_increment) {
		reg |= DIDSTC_INC_INC;
	} else {
		reg |= DIDSTC_INC_FIXED;
	}
	bus_space_write_4(sc->sc_iot, sc->sc_dmach, DMA_DIDSTC(channel_no), reg);

	/* Let the incrementing party decide how much data to transfer.
	   If both are incrementing, set the transfer size to the smallest one.
	 */
	if (dx->dx_desc[DMAC_DESC_SRC].xd_increment) {
		if (!dx->dx_desc[DMAC_DESC_DST].xd_increment) {
			transfer_size = dxs->dxs_segs[DMAC_DESC_SRC].ds_curseg->ds_len;
		} else {
			transfer_size = min(dxs->dxs_segs[DMAC_DESC_DST].ds_curseg->ds_len,
					    dxs->dxs_segs[DMAC_DESC_SRC].ds_curseg->ds_len);
		}
	} else {
		if (dx->dx_desc[DMAC_DESC_DST].xd_increment) {
			transfer_size = dxs->dxs_segs[DMAC_DESC_DST].ds_curseg->ds_len;
		} else {
			panic("S3C2440 DMA code does not support both source and destination being non-incrementing");
		}
	}

	/* Set options as prepared by dmac_start and add the transfer size.
	   If the transfer_size is not an even number of dxs_width,
	   ensure that all bytes are transferred by adding an extra transfer
	   of dxs_width.
	 */
	bus_space_write_4(sc->sc_iot, sc->sc_dmach, DMA_CON(channel_no),
			  dxs->dxs_options |
			  DMACON_TC(((transfer_size/dxs->dxs_width)+
				     min((transfer_size % dxs->dxs_width), 1))));

	DPRINTF(("Transfer size: %d (%d)\n", transfer_size, transfer_size/dxs->dxs_width));

	/* Start the transfer */
	reg = DMAMASKTRIG_ON;
	if (dxs->dxs_xfer.dx_peripheral == DMAC_PERIPH_NONE) {
		reg |= DMAMASKTRIG_SW_TRIG;
	}
	bus_space_write_4(sc->sc_iot, sc->sc_dmach, DMA_MASKTRIG(channel_no),
			  reg);

#if defined(S3C2440_DMA_DEBUG)
	reg = bus_space_read_4(sc->sc_iot, sc->sc_dmach, DMA_DISRC(channel_no));
	printf("DMA_DISRC: 0x%X\n", reg);

	reg = bus_space_read_4(sc->sc_iot, sc->sc_dmach, DMA_DISRCC(channel_no));
	printf("DMA_DISRCC: 0x%X\n", reg);

	reg = bus_space_read_4(sc->sc_iot, sc->sc_dmach, DMA_DIDST(channel_no));
	printf("DMA_DIDST: 0x%X\n", reg);

	reg = bus_space_read_4(sc->sc_iot, sc->sc_dmach, DMA_DIDSTC(channel_no));
	printf("DMA_DIDSTC: 0x%X\n", reg);

	reg = bus_space_read_4(sc->sc_iot, sc->sc_dmach, DMA_CON(channel_no));
	printf("DMA_CON: 0x%X\n", reg);

	reg = bus_space_read_4(sc->sc_iot, sc->sc_dmach, DMA_MASKTRIG(channel_no));
	printf("DMA_MASKTRIG: 0x%X\n", reg);

	reg = bus_space_read_4(sc->sc_iot, sc->sc_dmach, DMA_STAT(channel_no));
	printf("DMA_STAT: 0x%X\n", reg);
#endif
}

static void
dmac_channel_done(uint8_t channel_no)
{
	struct s3c2440_dmac_softc	*sc;
	struct s3c2440_dmac_channel	*dc;

	sc = s3c2440_dmac_sc;

	/* sc->sc_mutex must be held when calling this function */

	dc = &sc->sc_channels[channel_no];

	dc->dc_active = NULL;
	/* We deal with the queue before calling the
	   done callback, as it might start a new DMA
	   transfer.
	*/
	if ( SIMPLEQ_EMPTY(&dc->dc_queue) ) {
		DPRINTF(("DMA Queue empty for channel %d\n", channel_no));
	} else {
		/* There is a transfer in the queue. Start it*/
		struct dmac_xfer_state *dxs;
		DPRINTF(("Took a transfer from the queue\n"));
		dxs = SIMPLEQ_FIRST(&dc->dc_queue);
		SIMPLEQ_REMOVE_HEAD(&dc->dc_queue, dxs_link);

		dmac_start(channel_no, dxs);
	}
}

int
s3c2440_dmac_wait_xfer(dmac_xfer_t dx, int timeout) {
	uint32_t		  complete;
	int			  err = 0;
	struct s3c2440_dmac_softc *sc = s3c2440_dmac_sc;
	struct dmac_xfer_state	  *dxs = (struct dmac_xfer_state*)dx;

	mutex_enter(&sc->sc_intr_mutex);
	complete = dxs->dxs_complete;
	while(complete == 0) {
		int status;
		DPRINTF(("s3c2440_dma_xfer_wait: Complete: %x\n", complete));

		if ( (status = cv_timedwait(&sc->sc_intr_cv,
					    &sc->sc_intr_mutex, timeout)) ==
		    EWOULDBLOCK ) {
			DPRINTF(("s3c2440_dma_xfer_wait: Timed out\n"));
			complete = 1;
			err = ETIMEDOUT;
			break;
		}

		complete = dxs->dxs_complete;
	}

	mutex_exit(&sc->sc_intr_mutex);

#if 0
	if (err == 0 && dxs->dxs_aborted == 1) {
		/* Transfer was aborted */
		err = EIO;
	}
#endif

	return err;
}

void
s3c2440_dmac_abort_xfer(dmac_xfer_t dx) {
	struct s3c2440_dmac_softc	*sc = s3c2440_dmac_sc;
	struct dmac_xfer_state		*dxs = (struct dmac_xfer_state*)dx;
	struct s3c2440_dmac_channel	*dc;
	bool				wait = FALSE;

	KASSERT(dxs->dxs_channel != (uint8_t)DMAC_NO_CHANNEL);

	dc = &sc->sc_channels[dxs->dxs_channel];

	mutex_enter(&sc->sc_mutex);

	if (dc->dc_active == dxs) {
		uint32_t reg;

		bus_space_write_4(sc->sc_iot, sc->sc_dmach,
				  DMA_MASKTRIG(dxs->dxs_channel),
				  DMAMASKTRIG_STOP);
		reg = bus_space_read_4(sc->sc_iot, sc->sc_dmach,
				       DMA_MASKTRIG(dxs->dxs_channel));
		DPRINTF(("s3c2440_dma: channel %d mask trigger %x\n", dxs->dxs_channel, reg));

		if ( !(reg & DMAMASKTRIG_ON) ) {
			DPRINTF(("No wait for abort"));

			/* The transfer was aborted and the interrupt
			   was thus not triggered. We need to cleanup the
			   channel here. */
			dmac_channel_done(dxs->dxs_channel);
		} else {
			wait = TRUE;
		}
	} else {
		/* Transfer is not active, simply remove it from the queue */
		DPRINTF(("Removed transfer from queue\n"));
		SIMPLEQ_REMOVE(&dc->dc_queue, dxs, dmac_xfer_state, dxs_link);
	}

	mutex_exit(&sc->sc_mutex);

	if (wait == TRUE) {
		DPRINTF(("Abort: Wait for transfer to complete\n"));
		s3c2440_dmac_wait_xfer(dx, 0);
	}
}
