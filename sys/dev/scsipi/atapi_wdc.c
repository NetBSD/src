/*	$NetBSD: atapi_wdc.c,v 1.27.2.4 1999/10/20 22:42:04 thorpej Exp $	*/

/*
 * Copyright (c) 1998 Manuel Bouyer.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#ifndef WDCDEBUG
#define WDCDEBUG
#endif /* WDCDEBUG */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/buf.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/syslog.h>
#include <sys/proc.h>

#include <vm/vm.h>

#include <machine/intr.h>
#include <machine/bus.h>

#ifndef __BUS_SPACE_HAS_STREAM_METHODS
#define    bus_space_write_multi_stream_2    bus_space_write_multi_2
#define    bus_space_write_multi_stream_4    bus_space_write_multi_4
#define    bus_space_read_multi_stream_2    bus_space_read_multi_2
#define    bus_space_read_multi_stream_4    bus_space_read_multi_4
#endif /* __BUS_SPACE_HAS_STREAM_METHODS */

#include <dev/ata/atareg.h>
#include <dev/ata/atavar.h>
#include <dev/ic/wdcreg.h>
#include <dev/ic/wdcvar.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsipiconf.h>
#include <dev/scsipi/atapiconf.h>

#define DEBUG_INTR   0x01
#define DEBUG_XFERS  0x02
#define DEBUG_STATUS 0x04
#define DEBUG_FUNCS  0x08
#define DEBUG_PROBE  0x10
#ifdef WDCDEBUG
int wdcdebug_atapi_mask = 0;
#define WDCDEBUG_PRINT(args, level) \
	if (wdcdebug_atapi_mask & (level)) \
		printf args
#else
#define WDCDEBUG_PRINT(args, level)
#endif

#define ATAPI_DELAY 10	/* 10 ms, this is used only before sending a cmd */

void  wdc_atapi_minphys  __P((struct buf *bp));
void  wdc_atapi_start	__P((struct channel_softc *,struct wdc_xfer *));
int   wdc_atapi_intr	 __P((struct channel_softc *, struct wdc_xfer *, int));
void  wdc_atapi_kill_xfer __P((struct channel_softc *, struct wdc_xfer *));
int   wdc_atapi_ctrl	 __P((struct channel_softc *, struct wdc_xfer *, int));
void  wdc_atapi_done	 __P((struct channel_softc *, struct wdc_xfer *));
void  wdc_atapi_reset	 __P((struct channel_softc *, struct wdc_xfer *));
void  wdc_atapi_scsipi_request __P((struct scsipi_channel *,
	scsipi_adapter_req_t, void *));

#define MAX_SIZE MAXPHYS

void
wdc_atapibus_attach(chp)
	struct channel_softc *chp;
{
	struct wdc_softc *wdc = chp->wdc;
	struct scsipi_adapter *adapt = &wdc->sc_atapi_adapter;
	struct scsipi_channel *chan = &chp->ch_atapi_channel;
	struct ata_atapi_attach aa;

	/*
	 * Fill in the scsipi_adapter.
	 */
	memset(adapt, 0, sizeof(*adapt));
	adapt->adapt_dev = &wdc->sc_dev;
	adapt->adapt_nchannels = wdc->nchannels;
	adapt->adapt_request = wdc_atapi_scsipi_request;
	adapt->adapt_minphys = wdc_atapi_minphys;

	/*
	 * Fill in the scsipi_channel.
	 */
	memset(chan, 0, sizeof(*chan));
	chan->chan_adapter = adapt;
	chan->chan_bustype = &atapi_bustype;
	chan->chan_channel = chp->channel;
	chan->chan_flags = SCSIPI_CHAN_OPENINGS;
	chan->chan_openings = 1;
	chan->chan_max_periph = 1;
	chan->chan_ntargets = 2;
	chan->chan_nluns = 1;

	memset(&aa, 0, sizeof(aa));
	aa.aa_type = T_ATAPI;
	aa.aa_channel = chan->chan_channel;
	aa.aa_openings = chan->chan_openings;
	aa.aa_drv_data = chp->ch_drive; /* pass the whole array */
	aa.aa_bus_private = chan;
	chp->atapibus = config_found(&wdc->sc_dev, &aa, atapi_print);
}

void
wdc_atapi_minphys(bp)
	struct buf *bp;
{

	if (bp->b_bcount > MAX_SIZE)
		bp->b_bcount = MAX_SIZE;
	minphys(bp);
}

/*
 * Kill off all pending xfers for a periph.
 *
 * Must be called at splbio().
 */
void
atapi_kill_pending(periph)
	struct scsipi_periph *periph;
{
	struct wdc_softc *wdc =
	    (void *)periph->periph_channel->chan_adapter->adapt_dev;
	struct channel_softc *chp =
	    wdc->channels[periph->periph_channel->chan_channel];

	wdc_kill_pending(chp);
}

void
wdc_atapi_kill_xfer(chp, xfer)
	struct channel_softc *chp;
	struct wdc_xfer *xfer;
{
	struct scsipi_xfer *sc_xfer = xfer->cmd;

	untimeout(wdctimeout, chp);
	/* remove this command from xfer queue */
	wdc_free_xfer(chp, xfer);
	sc_xfer->error = XS_DRIVER_STUFFUP;
	scsipi_done(sc_xfer);
}

int
wdc_atapi_get_params(chan, drive, flags, id)
	struct scsipi_channel *chan;
	int drive, flags;
	struct ataparams *id;
{
	struct wdc_softc *wdc = (void *)chan->chan_adapter->adapt_dev;
	struct channel_softc *chp = wdc->channels[chan->chan_channel];
	struct wdc_command wdc_c;

	/* if no ATAPI device detected at wdc attach time, skip */
	/*
	 * XXX this will break scsireprobe if this is of any interest for
	 * ATAPI devices one day.
	 */
	if ((chp->ch_drive[drive].drive_flags & DRIVE_ATAPI) == 0) {
		WDCDEBUG_PRINT(("wdc_atapi_get_params: drive %d not present\n",
		    drive), DEBUG_PROBE);
		return -1;
	}
	memset(&wdc_c, 0, sizeof(struct wdc_command));
	wdc_c.r_command = ATAPI_SOFT_RESET;
	wdc_c.r_st_bmask = 0;
	wdc_c.r_st_pmask = 0;
	wdc_c.flags = AT_POLL;
	wdc_c.timeout = WDC_RESET_WAIT;
	if (wdc_exec_command(&chp->ch_drive[drive], &wdc_c) != WDC_COMPLETE) {
		printf("wdc_atapi_get_params: ATAPI_SOFT_RESET failed for"
		    " drive %s:%d:%d: driver failed\n",
		    chp->wdc->sc_dev.dv_xname, chp->channel, drive);
		panic("wdc_atapi_get_params");
	}
	if (wdc_c.flags & (AT_ERROR | AT_TIMEOU | AT_DF)) {
		WDCDEBUG_PRINT(("wdc_atapi_get_params: ATAPI_SOFT_RESET "
		    "failed for drive %s:%d:%d: error 0x%x\n",
		    chp->wdc->sc_dev.dv_xname, chp->channel, drive, 
		    wdc_c.r_error), DEBUG_PROBE);
		return -1;
	}
	chp->ch_drive[drive].state = 0;

	bus_space_read_1(chp->cmd_iot, chp->cmd_ioh, wd_status);
	
	/* Some ATAPI devices need a bit more time after software reset. */
	delay(5000);
	if (ata_get_params(&chp->ch_drive[drive], AT_POLL, id) != 0) {
		WDCDEBUG_PRINT(("wdc_atapi_get_params: ATAPI_IDENTIFY_DEVICE "
		    "failed for drive %s:%d:%d: error 0x%x\n",
		    chp->wdc->sc_dev.dv_xname, chp->channel, drive, 
		    wdc_c.r_error), DEBUG_PROBE);
		return -1;
	}
	return 0;
}

void
wdc_atapi_scsipi_request(chan, req, arg)
	struct scsipi_channel *chan;
	scsipi_adapter_req_t req;
	void *arg;
{
	struct scsipi_adapter *adapt = chan->chan_adapter;
	struct scsipi_periph *periph;
	struct scsipi_xfer *sc_xfer;
	struct wdc_softc *wdc = (void *)adapt->adapt_dev;
	struct wdc_xfer *xfer;
	int channel = chan->chan_channel;
	int flags, drive, s;

	switch (req) {
	case ADAPTER_REQ_RUN_XFER:
		sc_xfer = arg;
		periph = sc_xfer->xs_periph;
		flags = sc_xfer->xs_control;
		drive = periph->periph_target;

		WDCDEBUG_PRINT(("wdc_atapi_scsipi_request %s:%d:%d\n",
		    wdc->sc_dev.dv_xname, channel, drive), DEBUG_XFERS);

		xfer = wdc_get_xfer(WDC_NOSLEEP);
		if (xfer == NULL) {
			sc_xfer->error = XS_RESOURCE_SHORTAGE;
			scsipi_done(sc_xfer);
			return;
		}

		if (sc_xfer->xs_control & XS_CTL_POLL)
			xfer->c_flags |= C_POLL;
		xfer->drive = drive;
		xfer->c_flags |= C_ATAPI;
		xfer->cmd = sc_xfer;
		xfer->databuf = sc_xfer->data;
		xfer->c_bcount = sc_xfer->datalen;
		xfer->c_start = wdc_atapi_start;
		xfer->c_intr = wdc_atapi_intr;
		xfer->c_kill_xfer = wdc_atapi_kill_xfer;
		s = splbio();
		wdc_exec_xfer(wdc->channels[channel], xfer);
#ifdef DIAGNOSTIC
		if ((sc_xfer->xs_control & XS_CTL_POLL) != 0 &&
		    (sc_xfer->xs_status & XS_STS_DONE) == 0)
			panic("wdc_atapi_scsipi_request: polled command "
			    "not done");
#endif
		splx(s);
		return;

	default:
		/* Not supported, nothing to do. */
	}
}

void
wdc_atapi_start(chp, xfer)
	struct channel_softc *chp;
	struct wdc_xfer *xfer;
{
	struct scsipi_xfer *sc_xfer = xfer->cmd;
	struct ata_drive_datas *drvp = &chp->ch_drive[xfer->drive];

	WDCDEBUG_PRINT(("wdc_atapi_start %s:%d:%d, scsi flags 0x%x \n",
	    chp->wdc->sc_dev.dv_xname, chp->channel, drvp->drive,
	    sc_xfer->xs_control), DEBUG_XFERS);
	/* Adjust C_DMA, it may have changed if we are requesting sense */
	if ((drvp->drive_flags & (DRIVE_DMA | DRIVE_UDMA)) &&
	    (sc_xfer->datalen > 0 || (xfer->c_flags & C_SENSE)))
		xfer->c_flags |= C_DMA;
	else
		xfer->c_flags &= ~C_DMA;
	/* start timeout machinery */
	if ((sc_xfer->xs_control & XS_CTL_POLL) == 0)
		timeout(wdctimeout, chp, sc_xfer->timeout * hz / 1000);
	/* Do control operations specially. */
	if (drvp->state < READY) {
		if (drvp->state != PIOMODE) {
			printf("%s:%d:%d: bad state %d in wdc_atapi_start\n",
			    chp->wdc->sc_dev.dv_xname, chp->channel,
			    xfer->drive, drvp->state);
			panic("wdc_atapi_start: bad state");
		}
		wdc_atapi_ctrl(chp, xfer, 0);
		return;
	}
	bus_space_write_1(chp->cmd_iot, chp->cmd_ioh, wd_sdh,
	    WDSD_IBM | (xfer->drive << 4));
	if (wait_for_unbusy(chp, ATAPI_DELAY) < 0) {
		printf("wdc_atapi_start: not ready, st = %02x\n",
		    chp->ch_status);
		sc_xfer->error = XS_TIMEOUT;
		wdc_atapi_reset(chp, xfer);
		return;
	}

	/*
	 * Even with WDCS_ERR, the device should accept a command packet
	 * Limit length to what can be stuffed into the cylinder register
	 * (16 bits).  Some CD-ROMs seem to interpret '0' as 65536,
	 * but not all devices do that and it's not obvious from the
	 * ATAPI spec that that behaviour should be expected.  If more
	 * data is necessary, multiple data transfer phases will be done.
	 */

	wdccommand(chp, xfer->drive, ATAPI_PKT_CMD, 
	    sc_xfer->datalen <= 0xffff ? sc_xfer->datalen : 0xffff,
	    0, 0, 0, 
	    (xfer->c_flags & C_DMA) ? ATAPI_PKT_CMD_FTRE_DMA : 0);
	
	/*
	 * If there is no interrupt for CMD input, busy-wait for it (done in 
	 * the interrupt routine. If it is a polled command, call the interrupt
	 * routine until command is done.
	 */
	if ((sc_xfer->xs_periph->periph_cap & ATAPI_CFG_DRQ_MASK) !=
	    ATAPI_CFG_IRQ_DRQ || (sc_xfer->xs_control & XS_CTL_POLL)) {
		/* Wait for at last 400ns for status bit to be valid */
		DELAY(1);
		wdc_atapi_intr(chp, xfer, 0);
	} else {
		chp->ch_flags |= WDCF_IRQ_WAIT;
	}
	if (sc_xfer->xs_control & XS_CTL_POLL) {
		while ((sc_xfer->xs_status & XS_STS_DONE) == 0) {
			/* Wait for at last 400ns for status bit to be valid */
			DELAY(1);
			wdc_atapi_intr(chp, xfer, 0);
		}
	}
}

int
wdc_atapi_intr(chp, xfer, irq)
	struct channel_softc *chp;
	struct wdc_xfer *xfer;
	int irq;
{
	struct scsipi_xfer *sc_xfer = xfer->cmd;
	struct ata_drive_datas *drvp = &chp->ch_drive[xfer->drive];
	int len, phase, i, retries=0;
	int ire, dma_err = 0;
	int dma_flags = 0;
	struct scsipi_generic _cmd_reqsense;
	struct scsipi_sense *cmd_reqsense =
	    (struct scsipi_sense *)&_cmd_reqsense;
	void *cmd;

	WDCDEBUG_PRINT(("wdc_atapi_intr %s:%d:%d\n",
	    chp->wdc->sc_dev.dv_xname, chp->channel, drvp->drive), DEBUG_INTR);

	/* Is it not a transfer, but a control operation? */
	if (drvp->state < READY) {
		printf("%s:%d:%d: bad state %d in wdc_atapi_intr\n",
		    chp->wdc->sc_dev.dv_xname, chp->channel, xfer->drive,
		    drvp->state);
		panic("wdc_atapi_intr: bad state\n");
	}
	/*
	 * If we missed an interrupt in a PIO transfer, reset and restart.
	 * Don't try to continue transfer, we may have missed cycles.
	 */
	if ((xfer->c_flags & (C_TIMEOU | C_DMA)) == C_TIMEOU) {
		sc_xfer->error = XS_TIMEOUT;
		wdc_atapi_reset(chp, xfer);
		return 1;
	} 

	/* Ack interrupt done in wait_for_unbusy */
	bus_space_write_1(chp->cmd_iot, chp->cmd_ioh, wd_sdh,
	    WDSD_IBM | (xfer->drive << 4));
	if (wait_for_unbusy(chp,
	    (irq == 0) ? sc_xfer->timeout : 0) != 0) {
		if (irq && (xfer->c_flags & C_TIMEOU) == 0)
			return 0; /* IRQ was not for us */
		printf("%s:%d:%d: device timeout, c_bcount=%d, c_skip=%d\n",
		    chp->wdc->sc_dev.dv_xname, chp->channel, xfer->drive,
		    xfer->c_bcount, xfer->c_skip);
		if (xfer->c_flags & C_DMA)
			drvp->n_dmaerrs++;
		sc_xfer->error = XS_TIMEOUT;
		wdc_atapi_reset(chp, xfer);
		return 1;
	}
	/* If we missed an IRQ and were using DMA, flag it as a DMA error */
	if ((xfer->c_flags & C_TIMEOU) && (xfer->c_flags & C_DMA))
		drvp->n_dmaerrs++;
	/* 
	 * if the request sense command was aborted, report the short sense
	 * previously recorded, else continue normal processing
	 */

	if ((xfer->c_flags & C_SENSE) != 0 &&
	    (chp->ch_status & WDCS_ERR) != 0 &&
	    (chp->ch_error & WDCE_ABRT) != 0) {
		WDCDEBUG_PRINT(("wdc_atapi_intr: request_sense aborted, "
		    "calling wdc_atapi_done(), sense 0x%x\n",
		    sc_xfer->sense.atapi_sense), DEBUG_INTR);
		wdc_atapi_done(chp, xfer);
		return 1;
	}

	if (xfer->c_flags & C_DMA) {
		dma_flags = ((sc_xfer->xs_control & XS_CTL_DATA_IN) ||
		    (xfer->c_flags & C_SENSE)) ?  WDC_DMA_READ : 0;
		dma_flags |= sc_xfer->xs_control & XS_CTL_POLL ?
		    WDC_DMA_POLL : 0;
	}
again:
	len = bus_space_read_1(chp->cmd_iot, chp->cmd_ioh, wd_cyl_lo) +
	    256 * bus_space_read_1(chp->cmd_iot, chp->cmd_ioh, wd_cyl_hi);
	ire = bus_space_read_1(chp->cmd_iot, chp->cmd_ioh, wd_ireason);
	phase = (ire & (WDCI_CMD | WDCI_IN)) | (chp->ch_status & WDCS_DRQ);
	WDCDEBUG_PRINT(("wdc_atapi_intr: c_bcount %d len %d st 0x%x err 0x%x "
	    "ire 0x%x :", xfer->c_bcount,
	    len, chp->ch_status, chp->ch_error, ire), DEBUG_INTR);

	switch (phase) {
	case PHASE_CMDOUT:
		if (xfer->c_flags & C_SENSE) {
			memset(cmd_reqsense, 0, sizeof(struct scsipi_generic));
			cmd_reqsense->opcode = REQUEST_SENSE;
			cmd_reqsense->length = xfer->c_bcount;
			cmd = cmd_reqsense;
		} else {
			cmd  = sc_xfer->cmd;
		}
		WDCDEBUG_PRINT(("PHASE_CMDOUT\n"), DEBUG_INTR);
		/* Init the DMA channel if necessary */
		if (xfer->c_flags & C_DMA) {
			if ((*chp->wdc->dma_init)(chp->wdc->dma_arg,
			    chp->channel, xfer->drive,
			    xfer->databuf, xfer->c_bcount, dma_flags) != 0) {
				sc_xfer->error = XS_DRIVER_STUFFUP;
				break;
			}
		}
		/* send packet command */
		/* Commands are 12 or 16 bytes long. It's 32-bit aligned */
		if ((chp->wdc->cap & WDC_CAPABILITY_ATAPI_NOSTREAM)) {
			if (drvp->drive_flags & DRIVE_CAP32) {
				bus_space_write_multi_4(chp->data32iot,
				    chp->data32ioh, 0,
				    (u_int32_t *)cmd,
				    sc_xfer->cmdlen >> 2);
			} else {
				bus_space_write_multi_2(chp->cmd_iot,
				    chp->cmd_ioh, wd_data,
				    (u_int16_t *)cmd,
				    sc_xfer->cmdlen >> 1);
			}
		} else {
			if (drvp->drive_flags & DRIVE_CAP32) {
				bus_space_write_multi_stream_4(chp->data32iot,
				    chp->data32ioh, 0,
				    (u_int32_t *)cmd,
				    sc_xfer->cmdlen >> 2);
			} else {
				bus_space_write_multi_stream_2(chp->cmd_iot,
				    chp->cmd_ioh, wd_data,
				    (u_int16_t *)cmd,
				    sc_xfer->cmdlen >> 1);
			}
		}
		/* Start the DMA channel if necessary */
		if (xfer->c_flags & C_DMA) {
			(*chp->wdc->dma_start)(chp->wdc->dma_arg,
			    chp->channel, xfer->drive, dma_flags);
		}

		if ((sc_xfer->xs_control & XS_CTL_POLL) == 0) {
			chp->ch_flags |= WDCF_IRQ_WAIT;
		}
		return 1;

	 case PHASE_DATAOUT:
		/* write data */
		WDCDEBUG_PRINT(("PHASE_DATAOUT\n"), DEBUG_INTR);
		if ((sc_xfer->xs_control & XS_CTL_DATA_OUT) == 0 ||
		    (xfer->c_flags & C_DMA) != 0) {
			printf("wdc_atapi_intr: bad data phase DATAOUT\n");
			if (xfer->c_flags & C_DMA) {
				(*chp->wdc->dma_finish)(chp->wdc->dma_arg,
				    chp->channel, xfer->drive, dma_flags);
				drvp->n_dmaerrs++;
			}
			sc_xfer->error = XS_TIMEOUT;
			wdc_atapi_reset(chp, xfer);
			return 1;
		}
		if (xfer->c_bcount < len) {
			printf("wdc_atapi_intr: warning: write only "
			    "%d of %d requested bytes\n", xfer->c_bcount, len);
			if ((chp->wdc->cap & WDC_CAPABILITY_ATAPI_NOSTREAM)) {
				bus_space_write_multi_2(chp->cmd_iot,
				    chp->cmd_ioh, wd_data,
				    (u_int16_t *)((char *)xfer->databuf +
				                  xfer->c_skip),
				    xfer->c_bcount >> 1);
			} else {
				bus_space_write_multi_stream_2(chp->cmd_iot,
				    chp->cmd_ioh, wd_data,
				    (u_int16_t *)((char *)xfer->databuf +
				                  xfer->c_skip),
				    xfer->c_bcount >> 1);
			}
			for (i = xfer->c_bcount; i < len; i += 2)
				bus_space_write_2(chp->cmd_iot, chp->cmd_ioh,
				    wd_data, 0);
			xfer->c_skip += xfer->c_bcount;
			xfer->c_bcount = 0;
		} else {
			if (drvp->drive_flags & DRIVE_CAP32) {
			    if ((chp->wdc->cap & WDC_CAPABILITY_ATAPI_NOSTREAM))
				bus_space_write_multi_4(chp->data32iot,
				    chp->data32ioh, 0,
				    (u_int32_t *)((char *)xfer->databuf +
				                  xfer->c_skip),
				    len >> 2);
			    else
				bus_space_write_multi_stream_4(chp->data32iot,
				    chp->data32ioh, wd_data,
				    (u_int32_t *)((char *)xfer->databuf +
				                  xfer->c_skip),
				    len >> 2);

			    xfer->c_skip += len & 0xfffffffc;
			    xfer->c_bcount -= len & 0xfffffffc;
			    len = len & 0x03;
			}
			if (len > 0) {
			    if ((chp->wdc->cap & WDC_CAPABILITY_ATAPI_NOSTREAM))
				bus_space_write_multi_2(chp->cmd_iot,
				    chp->cmd_ioh, wd_data,
				    (u_int16_t *)((char *)xfer->databuf +
				                  xfer->c_skip),
				    len >> 1);
			    else
				bus_space_write_multi_stream_2(chp->cmd_iot,
				    chp->cmd_ioh, wd_data,
				    (u_int16_t *)((char *)xfer->databuf +
				                  xfer->c_skip),
				    len >> 1);
			    xfer->c_skip += len;
			    xfer->c_bcount -= len;
			}
		}
		if ((sc_xfer->xs_control & XS_CTL_POLL) == 0) {
			chp->ch_flags |= WDCF_IRQ_WAIT;
		}
		return 1;

	case PHASE_DATAIN:
		/* Read data */
		WDCDEBUG_PRINT(("PHASE_DATAIN\n"), DEBUG_INTR);
		if (((sc_xfer->xs_control & XS_CTL_DATA_IN) == 0 &&
		    (xfer->c_flags & C_SENSE) == 0) || 
		    (xfer->c_flags & C_DMA) != 0) {
			printf("wdc_atapi_intr: bad data phase DATAIN\n");
			if (xfer->c_flags & C_DMA) {
				(*chp->wdc->dma_finish)(chp->wdc->dma_arg,
				    chp->channel, xfer->drive, dma_flags);
				drvp->n_dmaerrs++;
			}
			sc_xfer->error = XS_TIMEOUT;
			wdc_atapi_reset(chp, xfer);
			return 1;
		}
		if (xfer->c_bcount < len) {
			printf("wdc_atapi_intr: warning: reading only "
			    "%d of %d bytes\n", xfer->c_bcount, len);
			if ((chp->wdc->cap & WDC_CAPABILITY_ATAPI_NOSTREAM)) {
			    bus_space_read_multi_2(chp->cmd_iot,
			    chp->cmd_ioh, wd_data,
			    (u_int16_t *)((char *)xfer->databuf +
			                  xfer->c_skip),
			    xfer->c_bcount >> 1);
			} else {
			    bus_space_read_multi_stream_2(chp->cmd_iot,
			    chp->cmd_ioh, wd_data,
			    (u_int16_t *)((char *)xfer->databuf +
			                  xfer->c_skip),
			    xfer->c_bcount >> 1);
			}
			wdcbit_bucket(chp, len - xfer->c_bcount);
			xfer->c_skip += xfer->c_bcount;
			xfer->c_bcount = 0;
		} else {
			if (drvp->drive_flags & DRIVE_CAP32) {
			    if ((chp->wdc->cap & WDC_CAPABILITY_ATAPI_NOSTREAM))
				bus_space_read_multi_4(chp->data32iot,
				    chp->data32ioh, 0,
				    (u_int32_t *)((char *)xfer->databuf +
				                  xfer->c_skip),
				    len >> 2);
			    else
				bus_space_read_multi_stream_4(chp->data32iot,
				    chp->data32ioh, wd_data,
				    (u_int32_t *)((char *)xfer->databuf +
				                  xfer->c_skip),
				    len >> 2);
				
			    xfer->c_skip += len & 0xfffffffc;
			    xfer->c_bcount -= len & 0xfffffffc;
			    len = len & 0x03;
			}
			if (len > 0) {
			    if ((chp->wdc->cap & WDC_CAPABILITY_ATAPI_NOSTREAM))
				bus_space_read_multi_2(chp->cmd_iot,
				    chp->cmd_ioh, wd_data,
				    (u_int16_t *)((char *)xfer->databuf +
				                  xfer->c_skip), 
				    len >> 1);
			    else
				bus_space_read_multi_stream_2(chp->cmd_iot,
				    chp->cmd_ioh, wd_data,
				    (u_int16_t *)((char *)xfer->databuf +
				                  xfer->c_skip), 
				    len >> 1);
			    xfer->c_skip += len;
			    xfer->c_bcount -=len;
			}
		}
		if ((sc_xfer->xs_control & XS_CTL_POLL) == 0) {
			chp->ch_flags |= WDCF_IRQ_WAIT;
		}
		return 1;

	case PHASE_ABORTED:
	case PHASE_COMPLETED:
		WDCDEBUG_PRINT(("PHASE_COMPLETED\n"), DEBUG_INTR);
		/* turn off DMA channel */
		if (xfer->c_flags & C_DMA) {
			dma_err = (*chp->wdc->dma_finish)(chp->wdc->dma_arg,
			    chp->channel, xfer->drive, dma_flags);
			if (xfer->c_flags & C_SENSE)
				xfer->c_bcount -=
				    sizeof(sc_xfer->sense.scsi_sense);
			else
				xfer->c_bcount -= sc_xfer->datalen;
		}
		if (xfer->c_flags & C_SENSE) {
			if ((chp->ch_status & WDCS_ERR) || dma_err < 0) {
				/*
				 * request sense failed ! it's not suppossed
				 * to be possible
				 */
				if (dma_err < 0)
					drvp->n_dmaerrs++;
				sc_xfer->error = XS_RESET;
				wdc_atapi_reset(chp, xfer);
				return (1);
			} else if (xfer->c_bcount <
			    sizeof(sc_xfer->sense.scsi_sense)) {
				/* use the sense we just read */
				sc_xfer->error = XS_SENSE;
			} else {
				/*
				 * command completed, but no data was read.
				 * use the short sense we saved previsouly.
				 */
				sc_xfer->error = XS_SHORTSENSE;
			}
		} else {
			sc_xfer->resid = xfer->c_bcount;
			if (chp->ch_status & WDCS_ERR) {
				/* save the short sense */
				sc_xfer->error = XS_SHORTSENSE;
				sc_xfer->sense.atapi_sense = chp->ch_error;
				if ((sc_xfer->xs_periph->periph_quirks &
				    PQUIRK_NOSENSE) == 0) {
					/*
					 * let the driver issue a
					 * 'request sense'
					 */
					xfer->databuf = &sc_xfer->sense;
					xfer->c_bcount =
					    sizeof(sc_xfer->sense.scsi_sense);
					xfer->c_skip = 0;
					xfer->c_flags |= C_SENSE;
					untimeout(wdctimeout, chp);
					wdc_atapi_start(chp, xfer);
					return 1;
				}
			} else if (dma_err < 0) {
				drvp->n_dmaerrs++;
				sc_xfer->error = XS_RESET;
				wdc_atapi_reset(chp, xfer);
				return (1);
			}
		}
		if (xfer->c_bcount != 0) {
			WDCDEBUG_PRINT(("wdc_atapi_intr: bcount value is "
			    "%d after io\n", xfer->c_bcount), DEBUG_XFERS);
		}
#ifdef DIAGNOSTIC
		if (xfer->c_bcount < 0) {
			printf("wdc_atapi_intr warning: bcount value "
			    "is %d after io\n", xfer->c_bcount);
		}
#endif
		break;

	default:
		if (++retries<500) {
			DELAY(100);
			chp->ch_status = bus_space_read_1(chp->cmd_iot,
			    chp->cmd_ioh, wd_status);
			chp->ch_error = bus_space_read_1(chp->cmd_iot,
			    chp->cmd_ioh, wd_error);
			goto again;
		}
		printf("wdc_atapi_intr: unknown phase 0x%x\n", phase);
		if (chp->ch_status & WDCS_ERR) {
			sc_xfer->error = XS_SHORTSENSE;
			sc_xfer->sense.atapi_sense = chp->ch_error;
		} else {
			sc_xfer->error = XS_RESET;
			wdc_atapi_reset(chp, xfer);
			return (1);
		}
	}
	WDCDEBUG_PRINT(("wdc_atapi_intr: wdc_atapi_done() (end), error 0x%x "
	    "sense 0x%x\n", sc_xfer->error, sc_xfer->sense.atapi_sense),
	    DEBUG_INTR);
	wdc_atapi_done(chp, xfer);
	return (1);
}

int
wdc_atapi_ctrl(chp, xfer, irq)
	struct channel_softc *chp;
	struct wdc_xfer *xfer;
	int irq;
{
	struct scsipi_xfer *sc_xfer = xfer->cmd;
	struct ata_drive_datas *drvp = &chp->ch_drive[xfer->drive];
	char *errstring = NULL;
	int delay = (irq == 0) ? ATAPI_DELAY : 0;

	/* Ack interrupt done in wait_for_unbusy */
again:
	WDCDEBUG_PRINT(("wdc_atapi_ctrl %s:%d:%d state %d\n",
	    chp->wdc->sc_dev.dv_xname, chp->channel, drvp->drive, drvp->state),
	    DEBUG_INTR | DEBUG_FUNCS);
	bus_space_write_1(chp->cmd_iot, chp->cmd_ioh, wd_sdh,
	    WDSD_IBM | (xfer->drive << 4));
	switch (drvp->state) {
	case PIOMODE:
piomode:
		/* Don't try to set mode if controller can't be adjusted */
		if ((chp->wdc->cap & WDC_CAPABILITY_MODE) == 0)
			goto ready;
		/* Also don't try if the drive didn't report its mode */
		if ((drvp->drive_flags & DRIVE_MODE) == 0)
			goto ready;;
		wdccommand(chp, drvp->drive, SET_FEATURES, 0, 0, 0,
		    0x08 | drvp->PIO_mode, WDSF_SET_MODE);
		drvp->state = PIOMODE_WAIT;
		break;
	case PIOMODE_WAIT:
		errstring = "piomode";
		if (wait_for_unbusy(chp, delay))
			goto timeout;
		if (chp->ch_status & WDCS_ERR) {
			if (drvp->PIO_mode < 3) {
				drvp->PIO_mode = 3;
				goto piomode;
			} else {
				goto error;
			}
		}
	/* fall through */

	case DMAMODE:
		if (drvp->drive_flags & DRIVE_UDMA) {
			wdccommand(chp, drvp->drive, SET_FEATURES, 0, 0, 0,
			    0x40 | drvp->UDMA_mode, WDSF_SET_MODE);
		} else if (drvp->drive_flags & DRIVE_DMA) {
			wdccommand(chp, drvp->drive, SET_FEATURES, 0, 0, 0,
			    0x20 | drvp->DMA_mode, WDSF_SET_MODE);
		} else {
			goto ready;
		}
		drvp->state = DMAMODE_WAIT;
		break;
	case DMAMODE_WAIT:
		errstring = "dmamode";
		if (wait_for_unbusy(chp, delay))
			goto timeout;
		if (chp->ch_status & WDCS_ERR)
			goto error;
	/* fall through */

	case READY:
	ready:
		drvp->state = READY;
		xfer->c_intr = wdc_atapi_intr;
		untimeout(wdctimeout, chp);
		wdc_atapi_start(chp, xfer);
		return 1;
	}
	if ((sc_xfer->xs_control & XS_CTL_POLL) == 0) {
		chp->ch_flags |= WDCF_IRQ_WAIT;
		xfer->c_intr = wdc_atapi_ctrl;
	} else {
		goto again;
	}
	return 1;

timeout:
	if (irq && (xfer->c_flags & C_TIMEOU) == 0) {
		return 0; /* IRQ was not for us */
	}
	printf("%s:%d:%d: %s timed out\n",
	    chp->wdc->sc_dev.dv_xname, chp->channel, xfer->drive, errstring);
	sc_xfer->error = XS_TIMEOUT;
	wdc_atapi_reset(chp, xfer);
	return 1;
error:
	printf("%s:%d:%d: %s ",
	    chp->wdc->sc_dev.dv_xname, chp->channel, xfer->drive,
	    errstring);
	printf("error (0x%x)\n", chp->ch_error);
	sc_xfer->error = XS_SHORTSENSE;
	sc_xfer->sense.atapi_sense = chp->ch_error;
	wdc_atapi_reset(chp, xfer);
	return 1;
}

void
wdc_atapi_done(chp, xfer)
	struct channel_softc *chp;
	struct wdc_xfer *xfer;
{
	struct scsipi_xfer *sc_xfer = xfer->cmd;
	struct ata_drive_datas *drvp = &chp->ch_drive[xfer->drive];

	WDCDEBUG_PRINT(("wdc_atapi_done %s:%d:%d: flags 0x%x\n",
	    chp->wdc->sc_dev.dv_xname, chp->channel, xfer->drive,
	    (u_int)xfer->c_flags), DEBUG_XFERS);
	untimeout(wdctimeout, chp);
	/* remove this command from xfer queue */
	wdc_free_xfer(chp, xfer);
	if (drvp->n_dmaerrs ||
	  (sc_xfer->error != XS_NOERROR && sc_xfer->error != XS_SENSE &&
	  sc_xfer->error != XS_SHORTSENSE)) {
		drvp->n_dmaerrs = 0;
		wdc_downgrade_mode(drvp);
	}
	    
	WDCDEBUG_PRINT(("wdc_atapi_done: scsipi_done\n"), DEBUG_XFERS);
	scsipi_done(sc_xfer);
	WDCDEBUG_PRINT(("wdcstart from wdc_atapi_done, flags 0x%x\n",
	    chp->ch_flags), DEBUG_XFERS);
	wdcstart(chp);
}

void
wdc_atapi_reset(chp, xfer)
	struct channel_softc *chp;
	struct wdc_xfer *xfer;
{
	struct ata_drive_datas *drvp = &chp->ch_drive[xfer->drive];
	struct scsipi_xfer *sc_xfer = xfer->cmd;

	wdccommandshort(chp, xfer->drive, ATAPI_SOFT_RESET);
	drvp->state = 0;
	if (wait_for_unbusy(chp, WDC_RESET_WAIT) != 0) {
		printf("%s:%d:%d: reset failed\n",
		    chp->wdc->sc_dev.dv_xname, chp->channel,
		    xfer->drive);
		sc_xfer->error = XS_SELTIMEOUT;
	}
	wdc_atapi_done(chp, xfer);
	return;
}
