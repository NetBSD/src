/*	$NetBSD: bha.c,v 1.32.2.1 1999/12/27 18:34:44 wrstuden Exp $	*/

/*-
 * Copyright (c) 1997, 1998, 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum and by Jason R. Thorpe of the Numerical Aerospace
 * Simulation Facility, NASA Ames Research Center.
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
 * Originally written by Julian Elischer (julian@tfs.com)
 * for TRW Financial Systems for use under the MACH(2.5) operating system.
 *
 * TRW Financial Systems, in accordance with their agreement with Carnegie
 * Mellon University, makes this software available to CMU to distribute
 * or use in any manner that they see fit as long as this message is kept with
 * the software. For this reason TFS also grants any other persons or
 * organisations permission to use or modify this software.
 *
 * TFS supplies this software to be publicly redistributed
 * on the understanding that TFS is not responsible for the correct
 * functioning of this software in any circumstances.
 */

#include "opt_ddb.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/user.h>

#include <vm/vm.h>			/* for PAGE_SIZE */

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#include <dev/ic/bhareg.h>
#include <dev/ic/bhavar.h>

#ifndef DDB
#define Debugger() panic("should call debugger here (bha.c)")
#endif /* ! DDB */

#define	BHA_MAXXFER	((BHA_NSEG - 1) << PGSHIFT)

#ifdef BHADEBUG
int     bha_debug = 0;
#endif /* BHADEBUG */

int	bha_cmd __P((bus_space_tag_t, bus_space_handle_t, struct bha_softc *,
	    int, u_char *, int, u_char *));

int	bha_scsi_cmd __P((struct scsipi_xfer *));
void	bha_minphys __P((struct buf *));

void	bha_done __P((struct bha_softc *, struct bha_ccb *));
int	bha_poll __P((struct bha_softc *, struct scsipi_xfer *, int));
void	bha_timeout __P((void *arg));

int	bha_init __P((struct bha_softc *));

int	bha_create_mailbox __P((struct bha_softc *));
void	bha_collect_mbo __P((struct bha_softc *));

void	bha_queue_ccb __P((struct bha_softc *, struct bha_ccb *));
void	bha_start_ccbs __P((struct bha_softc *));
void	bha_finish_ccbs __P((struct bha_softc *));

struct bha_ccb *bha_ccb_phys_kv __P((struct bha_softc *, bus_addr_t));
void	bha_create_ccbs __P((struct bha_softc *, int));
int	bha_init_ccb __P((struct bha_softc *, struct bha_ccb *));
struct bha_ccb *bha_get_ccb __P((struct bha_softc *, int));
void	bha_free_ccb __P((struct bha_softc *, struct bha_ccb *));

/* the below structure is so we have a default dev struct for out link struct */
struct scsipi_device bha_dev = {
	NULL,			/* Use default error handler */
	NULL,			/* have a queue, served by this */
	NULL,			/* have no async handler */
	NULL,			/* Use default 'done' routine */
};

#define BHA_RESET_TIMEOUT	2000	/* time to wait for reset (mSec) */
#define	BHA_ABORT_TIMEOUT	2000	/* time to wait for abort (mSec) */

/*
 * Number of CCBs in an allocation group; must be computed at run-time.
 */
int	bha_ccbs_per_group;

__inline struct bha_mbx_out *bha_nextmbo __P((struct bha_softc *,
	struct bha_mbx_out *));
__inline struct bha_mbx_in *bha_nextmbi __P((struct bha_softc *,
	struct bha_mbx_in *));

__inline struct bha_mbx_out *
bha_nextmbo(sc, mbo)
	struct bha_softc *sc;
	struct bha_mbx_out *mbo;
{

	if (mbo == &sc->sc_mbo[sc->sc_mbox_count - 1])
		return (&sc->sc_mbo[0]);
	return (mbo + 1);
}

__inline struct bha_mbx_in *
bha_nextmbi(sc, mbi)
	struct bha_softc *sc;
	struct bha_mbx_in *mbi;
{

	if (mbi == &sc->sc_mbi[sc->sc_mbox_count - 1])
		return (&sc->sc_mbi[0]);
	return (mbi + 1);
}

/*
 * bha_attach:
 *
 *	Finish attaching a Buslogic controller, and configure children.
 */
void
bha_attach(sc, bpd)
	struct bha_softc *sc;
	struct bha_probe_data *bpd;
{
	int initial_ccbs;

	/*
	 * Initialize the number of CCBs per group.
	 */
	if (bha_ccbs_per_group == 0)
		bha_ccbs_per_group = BHA_CCBS_PER_GROUP;

	initial_ccbs = bha_info(sc);
	if (initial_ccbs == 0) {
		printf("%s: unable to get adapter info\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	/*
	 * Fill in the adapter.
	 */
	sc->sc_adapter.scsipi_cmd = bha_scsi_cmd;
	sc->sc_adapter.scsipi_minphys = bha_minphys;

	/*
	 * fill in the prototype scsipi_link.
	 */
	sc->sc_link.scsipi_scsi.channel = SCSI_CHANNEL_ONLY_ONE;
	sc->sc_link.adapter_softc = sc;
	sc->sc_link.scsipi_scsi.adapter_target = sc->sc_scsi_id;
	sc->sc_link.adapter = &sc->sc_adapter;
	sc->sc_link.device = &bha_dev;
	sc->sc_link.openings = 4;
	sc->sc_link.scsipi_scsi.max_target =
	    (sc->sc_flags & BHAF_WIDE) ? 15 : 7;
	sc->sc_link.scsipi_scsi.max_lun =
	    (sc->sc_flags & BHAF_WIDE_LUN) ? 31 : 7;
	sc->sc_link.type = BUS_SCSI;

	TAILQ_INIT(&sc->sc_free_ccb);
	TAILQ_INIT(&sc->sc_waiting_ccb);
	TAILQ_INIT(&sc->sc_allocating_ccbs);
	TAILQ_INIT(&sc->sc_queue);

	if (bha_create_mailbox(sc) != 0)
		return;

	bha_create_ccbs(sc, initial_ccbs);
	if (sc->sc_cur_ccbs < 2) {
		printf("%s: not enough CCBs to run\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	if (bha_init(sc) != 0)
		return;

	(void) config_found(&sc->sc_dev, &sc->sc_link, scsiprint);
}

/*
 * bha_intr:
 *
 *	Interrupt service routine.
 */
int
bha_intr(arg)
	void *arg;
{
	struct bha_softc *sc = arg;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	u_char sts;

#ifdef BHADEBUG
	printf("%s: bha_intr ", sc->sc_dev.dv_xname);
#endif /* BHADEBUG */

	/*
	 * First acknowlege the interrupt, Then if it's not telling about
	 * a completed operation just return.
	 */
	sts = bus_space_read_1(iot, ioh, BHA_INTR_PORT);
	if ((sts & BHA_INTR_ANYINTR) == 0)
		return (0);
	bus_space_write_1(iot, ioh, BHA_CTRL_PORT, BHA_CTRL_IRST);

#ifdef BHADIAG
	/* Make sure we clear CCB_SENDING before finishing a CCB. */
	bha_collect_mbo(sc);
#endif

	/* Mail box out empty? */
	if (sts & BHA_INTR_MBOA) {
		struct bha_toggle toggle;

		toggle.cmd.opcode = BHA_MBO_INTR_EN;
		toggle.cmd.enable = 0;
		bha_cmd(iot, ioh, sc,
		    sizeof(toggle.cmd), (u_char *)&toggle.cmd,
		    0, (u_char *)0);
		bha_start_ccbs(sc);
	}

	/* Mail box in full? */
	if (sts & BHA_INTR_MBIF)
		bha_finish_ccbs(sc);

	return (1);
}

/*****************************************************************************
 * SCSI interface routines
 *****************************************************************************/

/*
 * bha_scsi_cmd:
 *
 *	Start a SCSI operation.
 */
int
bha_scsi_cmd(xs)
	struct scsipi_xfer *xs;
{
	struct scsipi_link *sc_link = xs->sc_link;
	struct bha_softc *sc = sc_link->adapter_softc;
	bus_dma_tag_t dmat = sc->sc_dmat;
	struct bha_ccb *ccb;
	int error, seg, flags, s;
	int fromqueue = 0, dontqueue = 0;

	SC_DEBUG(sc_link, SDEV_DB2, ("bha_scsi_cmd\n"));

	s = splbio();		/* protect the queue */

	/*
	 * If we're running the queue from bha_done(), we've been
	 * called with the first queue entry as our argument.
	 */
	if (xs == TAILQ_FIRST(&sc->sc_queue)) {
		TAILQ_REMOVE(&sc->sc_queue, xs, adapter_q);
		fromqueue = 1;
		goto get_ccb;
	}

	/* Polled requests can't be queued for later. */
	dontqueue = xs->xs_control & XS_CTL_POLL;

	/*
	 * If there are jobs in the queue, run them first.
	 */
	if (TAILQ_FIRST(&sc->sc_queue) != NULL) {
		/*
		 * If we can't queue, we have to abort, since
		 * we have to preserve order.
		 */
		if (dontqueue) {
			splx(s);
			xs->error = XS_DRIVER_STUFFUP;
			return (TRY_AGAIN_LATER);
		}

		/*
		 * Swap with the first queue entry.
		 */
		TAILQ_INSERT_TAIL(&sc->sc_queue, xs, adapter_q);
		xs = TAILQ_FIRST(&sc->sc_queue);
		TAILQ_REMOVE(&sc->sc_queue, xs, adapter_q);
		fromqueue = 1;
	}

 get_ccb:
	/*
	 * get a ccb to use. If the transfer
	 * is from a buf (possibly from interrupt time)
	 * then we can't allow it to sleep
	 */
	flags = xs->xs_control;
	if ((ccb = bha_get_ccb(sc, flags)) == NULL) {
		/*
		 * If we can't queue, we lose.
		 */
		if (dontqueue) {
			splx(s);
			xs->error = XS_DRIVER_STUFFUP;
			return (TRY_AGAIN_LATER);
		}

		/*
		 * Stuff ourselves into the queue, in front
		 * if we came off in the first place.
		 */
		if (fromqueue)
			TAILQ_INSERT_HEAD(&sc->sc_queue, xs, adapter_q);
		else
			TAILQ_INSERT_TAIL(&sc->sc_queue, xs, adapter_q);
		splx(s);
		return (SUCCESSFULLY_QUEUED);
	}

	splx(s);		/* done playing with the queue */

	ccb->xs = xs;
	ccb->timeout = xs->timeout;

	/*
	 * Put all the arguments for the xfer in the ccb
	 */
	if (flags & XS_CTL_RESET) {
		ccb->opcode = BHA_RESET_CCB;
		ccb->scsi_cmd_length = 0;
	} else {
		/* can't use S/G if zero length */
		ccb->opcode = (xs->datalen ? BHA_INIT_SCAT_GATH_CCB
					   : BHA_INITIATOR_CCB);
		bcopy(xs->cmd, &ccb->scsi_cmd,
		    ccb->scsi_cmd_length = xs->cmdlen);
	}

	if (xs->datalen) {
		/*
		 * Map the DMA transfer.
		 */
#ifdef TFS
		if (flags & XS_CTL_DATA_UIO) {
			error = bus_dmamap_load_uio(dmat,
			    ccb->dmamap_xfer, (struct uio *)xs->data,
			    (flags & XS_CTL_NOSLEEP) ? BUS_DMA_NOWAIT :
			    BUS_DMA_WAITOK);
		} else
#endif /* TFS */
		{
			error = bus_dmamap_load(dmat,
			    ccb->dmamap_xfer, xs->data, xs->datalen, NULL,
			    (flags & XS_CTL_NOSLEEP) ? BUS_DMA_NOWAIT :
			    BUS_DMA_WAITOK);
		}

		if (error) {
			if (error == EFBIG) {
				printf("%s: bha_scsi_cmd, more than %d"
				    " dma segments\n",
				    sc->sc_dev.dv_xname, BHA_NSEG);
			} else {
				printf("%s: bha_scsi_cmd, error %d loading"
				    " dma map\n",
				    sc->sc_dev.dv_xname, error);
			}
			goto bad;
		}

		bus_dmamap_sync(dmat, ccb->dmamap_xfer, 0,
		    ccb->dmamap_xfer->dm_mapsize,
		    (flags & XS_CTL_DATA_IN) ? BUS_DMASYNC_PREREAD :
		    BUS_DMASYNC_PREWRITE);

		/*
		 * Load the hardware scatter/gather map with the
		 * contents of the DMA map.
		 */
		for (seg = 0; seg < ccb->dmamap_xfer->dm_nsegs; seg++) {
			ltophys(ccb->dmamap_xfer->dm_segs[seg].ds_addr,
			    ccb->scat_gath[seg].seg_addr);
			ltophys(ccb->dmamap_xfer->dm_segs[seg].ds_len,
			    ccb->scat_gath[seg].seg_len);
		}

		ltophys(ccb->hashkey + offsetof(struct bha_ccb, scat_gath),
		    ccb->data_addr);
		ltophys(ccb->dmamap_xfer->dm_nsegs *
		    sizeof(struct bha_scat_gath), ccb->data_length);
	} else {
		/*
		 * No data xfer, use non S/G values.
		 */
		ltophys(0, ccb->data_addr);
		ltophys(0, ccb->data_length);
	}

	ccb->data_out = 0;
	ccb->data_in = 0;
	ccb->target = sc_link->scsipi_scsi.target;
	ccb->lun = sc_link->scsipi_scsi.lun;
	ltophys(ccb->hashkey + offsetof(struct bha_ccb, scsi_sense),
	    ccb->sense_ptr);
	ccb->req_sense_length = sizeof(ccb->scsi_sense);
	ccb->host_stat = 0x00;
	ccb->target_stat = 0x00;
	ccb->link_id = 0;
	ltophys(0, ccb->link_addr);

	BHA_CCB_SYNC(sc, ccb, BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

	s = splbio();
	bha_queue_ccb(sc, ccb);
	splx(s);

	SC_DEBUG(sc_link, SDEV_DB3, ("cmd_sent\n"));
	if ((flags & XS_CTL_POLL) == 0)
		return (SUCCESSFULLY_QUEUED);

	/*
	 * If we can't use interrupts, poll on completion
	 */
	if (bha_poll(sc, xs, ccb->timeout)) {
		bha_timeout(ccb);
		if (bha_poll(sc, xs, ccb->timeout))
			bha_timeout(ccb);
	}
	return (COMPLETE);

 bad:
	xs->error = XS_DRIVER_STUFFUP;
	bha_free_ccb(sc, ccb);
	return (COMPLETE);
}

/*
 * bha_minphys:
 *
 *	Limit a transfer to our maximum transfer size.
 */
void
bha_minphys(bp)
	struct buf *bp;
{

	if (bp->b_bcount > BHA_MAXXFER)
		bp->b_bcount = BHA_MAXXFER;
	minphys(bp);
}

/*****************************************************************************
 * SCSI job execution helper routines
 *****************************************************************************/

/*
 * bha_done:
 *
 *	A CCB has completed execution.  Pass the status back to the
 *	upper layer.
 */
void
bha_done(sc, ccb)
	struct bha_softc *sc;
	struct bha_ccb *ccb;
{
	bus_dma_tag_t dmat = sc->sc_dmat;
	struct scsipi_xfer *xs = ccb->xs;

	SC_DEBUG(xs->sc_link, SDEV_DB2, ("bha_done\n"));

#ifdef BHADIAG
	if (ccb->flags & CCB_SENDING) {
		printf("%s: exiting ccb still in transit!\n",
		    sc->sc_dev.dv_xname);
		Debugger();
		return;
	}
#endif
	if ((ccb->flags & CCB_ALLOC) == 0) {
		printf("%s: exiting ccb not allocated!\n",
		    sc->sc_dev.dv_xname);
		Debugger();
		return;
	}

	/*
	 * If we were a data transfer, unload the map that described
	 * the data buffer.
	 */
	if (xs->datalen) {
		bus_dmamap_sync(dmat, ccb->dmamap_xfer, 0,
		    ccb->dmamap_xfer->dm_mapsize,
		    (xs->xs_control & XS_CTL_DATA_IN) ? BUS_DMASYNC_POSTREAD :
		    BUS_DMASYNC_POSTWRITE);
		bus_dmamap_unload(dmat, ccb->dmamap_xfer);
	}

	if (xs->error == XS_NOERROR) {
		if (ccb->host_stat != BHA_OK) {
			switch (ccb->host_stat) {
			case BHA_SEL_TIMEOUT:	/* No response */
				xs->error = XS_SELTIMEOUT;
				break;
			default:	/* Other scsi protocol messes */
				printf("%s: host_stat %x\n",
				    sc->sc_dev.dv_xname, ccb->host_stat);
				xs->error = XS_DRIVER_STUFFUP;
				break;
			}
		} else if (ccb->target_stat != SCSI_OK) {
			switch (ccb->target_stat) {
			case SCSI_CHECK:
				memcpy(&xs->sense.scsi_sense,
				    &ccb->scsi_sense,
				    sizeof(xs->sense.scsi_sense));
				xs->error = XS_SENSE;
				break;
			case SCSI_BUSY:
				xs->error = XS_BUSY;
				break;
			default:
				printf("%s: target_stat %x\n",
				    sc->sc_dev.dv_xname, ccb->target_stat);
				xs->error = XS_DRIVER_STUFFUP;
				break;
			}
		} else
			xs->resid = 0;
	}

	bha_free_ccb(sc, ccb);

	xs->xs_status |= XS_STS_DONE;
	scsipi_done(xs);

	/*
	 * If there are queue entries in the software queue, try to
	 * run the first one.  We should be more or less guaranteed
	 * to succeed, since we just freed a CCB.
	 *
	 * NOTE: bha_scsi_cmd() relies on our calling it with
	 * the first entry in the queue.
	 */
	if ((xs = TAILQ_FIRST(&sc->sc_queue)) != NULL)
		(void) bha_scsi_cmd(xs);
}

/*
 * bha_poll:
 *
 *	Poll for completion of the specified job.
 */
int
bha_poll(sc, xs, count)
	struct bha_softc *sc;
	struct scsipi_xfer *xs;
	int count;
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;

	/* timeouts are in msec, so we loop in 1000 usec cycles */
	while (count) {
		/*
		 * If we had interrupts enabled, would we
		 * have got an interrupt?
		 */
		if (bus_space_read_1(iot, ioh, BHA_INTR_PORT) &
		    BHA_INTR_ANYINTR)
			bha_intr(sc);
		if (xs->xs_status & XS_STS_DONE)
			return (0);
		delay(1000);	/* only happens in boot so ok */
		count--;
	}
	return (1);
}

/*
 * bha_timeout:
 *
 *	CCB timeout handler.
 */
void
bha_timeout(arg)
	void *arg;
{
	struct bha_ccb *ccb = arg;
	struct scsipi_xfer *xs = ccb->xs;
	struct scsipi_link *sc_link = xs->sc_link;
	struct bha_softc *sc = sc_link->adapter_softc;
	int s;

	scsi_print_addr(sc_link);
	printf("timed out");

	s = splbio();

#ifdef BHADIAG
	/*
	 * If the ccb's mbx is not free, then the board has gone Far East?
	 */
	bha_collect_mbo(sc);
	if (ccb->flags & CCB_SENDING) {
		printf("%s: not taking commands!\n", sc->sc_dev.dv_xname);
		Debugger();
	}
#endif

	/*
	 * If it has been through before, then
	 * a previous abort has failed, don't
	 * try abort again
	 */
	if (ccb->flags & CCB_ABORT) {
		/* abort timed out */
		printf(" AGAIN\n");
		/* XXX Must reset! */
	} else {
		/* abort the operation that has timed out */
		printf("\n");
		ccb->xs->error = XS_TIMEOUT;
		ccb->timeout = BHA_ABORT_TIMEOUT;
		ccb->flags |= CCB_ABORT;
		bha_queue_ccb(sc, ccb);
	}

	splx(s);
}

/*****************************************************************************
 * Misc. subroutines.
 *****************************************************************************/

/*
 * bha_cmd:
 *
 *	Send a command to the Buglogic controller.
 */
int
bha_cmd(iot, ioh, sc, icnt, ibuf, ocnt, obuf)
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	struct bha_softc *sc;
	int icnt, ocnt;
	u_char *ibuf, *obuf;
{
	const char *name;
	register int i;
	int wait;
	u_char sts;
	u_char opcode = ibuf[0];

	if (sc != NULL)
		name = sc->sc_dev.dv_xname;
	else
		name = "(bha probe)";

	/*
	 * Calculate a reasonable timeout for the command.
	 */
	switch (opcode) {
	case BHA_INQUIRE_DEVICES:
	case BHA_INQUIRE_DEVICES_2:
		wait = 90 * 20000;
		break;
	default:
		wait = 1 * 20000;
		break;
	}

	/*
	 * Wait for the adapter to go idle, unless it's one of
	 * the commands which don't need this
	 */
	if (opcode != BHA_MBO_INTR_EN) {
		for (i = 20000; i; i--) {	/* 1 sec? */
			sts = bus_space_read_1(iot, ioh, BHA_STAT_PORT);
			if (sts & BHA_STAT_IDLE)
				break;
			delay(50);
		}
		if (!i) {
			printf("%s: bha_cmd, host not idle(0x%x)\n",
			    name, sts);
			return (1);
		}
	}

	/*
	 * Now that it is idle, if we expect output, preflush the
	 * queue feeding to us.
	 */
	if (ocnt) {
		while ((bus_space_read_1(iot, ioh, BHA_STAT_PORT)) &
		    BHA_STAT_DF)
			bus_space_read_1(iot, ioh, BHA_DATA_PORT);
	}

	/*
	 * Output the command and the number of arguments given
	 * for each byte, first check the port is empty.
	 */
	while (icnt--) {
		for (i = wait; i; i--) {
			sts = bus_space_read_1(iot, ioh, BHA_STAT_PORT);
			if (!(sts & BHA_STAT_CDF))
				break;
			delay(50);
		}
		if (!i) {
			if (opcode != BHA_INQUIRE_REVISION)
				printf("%s: bha_cmd, cmd/data port full\n",
				    name);
			goto bad;
		}
		bus_space_write_1(iot, ioh, BHA_CMD_PORT, *ibuf++);
	}

	/*
	 * If we expect input, loop that many times, each time,
	 * looking for the data register to have valid data
	 */
	while (ocnt--) {
		for (i = wait; i; i--) {
			sts = bus_space_read_1(iot, ioh, BHA_STAT_PORT);
			if (sts & BHA_STAT_DF)
				break;
			delay(50);
		}
		if (!i) {
			if (opcode != BHA_INQUIRE_REVISION)
				printf("%s: bha_cmd, cmd/data port empty %d\n",
				    name, ocnt);
			goto bad;
		}
		*obuf++ = bus_space_read_1(iot, ioh, BHA_DATA_PORT);
	}

	/*
	 * Wait for the board to report a finished instruction.
	 * We may get an extra interrupt for the HACC signal, but this is
	 * unimportant.
	 */
	if (opcode != BHA_MBO_INTR_EN && opcode != BHA_MODIFY_IOPORT) {
		for (i = 20000; i; i--) {	/* 1 sec? */
			sts = bus_space_read_1(iot, ioh, BHA_INTR_PORT);
			/* XXX Need to save this in the interrupt handler? */
			if (sts & BHA_INTR_HACC)
				break;
			delay(50);
		}
		if (!i) {
			printf("%s: bha_cmd, host not finished(0x%x)\n",
			    name, sts);
			return (1);
		}
	}
	bus_space_write_1(iot, ioh, BHA_CTRL_PORT, BHA_CTRL_IRST);
	return (0);

bad:
	bus_space_write_1(iot, ioh, BHA_CTRL_PORT, BHA_CTRL_SRST);
	return (1);
}

/*
 * bha_find:
 *
 *	Find the board and determine it's irq/drq.
 */
int
bha_find(iot, ioh, sc)
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	struct bha_probe_data *sc;
{
	int i;
	u_char sts;
	struct bha_extended_inquire inquire;
	struct bha_config config;
	int irq, drq;

	/* Check something is at the ports we need to access */
	sts = bus_space_read_1(iot, ioh, BHA_STAT_PORT);
	if (sts == 0xFF)
		return (0);

	/*
	 * Reset board, If it doesn't respond, assume
	 * that it's not there.. good for the probe
	 */

	bus_space_write_1(iot, ioh, BHA_CTRL_PORT,
	    BHA_CTRL_HRST | BHA_CTRL_SRST);

	delay(100);
	for (i = BHA_RESET_TIMEOUT; i; i--) {
		sts = bus_space_read_1(iot, ioh, BHA_STAT_PORT);
		if (sts == (BHA_STAT_IDLE | BHA_STAT_INIT))
			break;
		delay(1000);
	}
	if (!i) {
#ifdef BHADEBUG
		if (bha_debug)
			printf("bha_find: No answer from buslogic board\n");
#endif /* BHADEBUG */
		return (0);
	}

	/*
	 * The BusLogic cards implement an Adaptec 1542 (aha)-compatible
	 * interface. The native bha interface is not compatible with 
	 * an aha. 1542. We need to ensure that we never match an
	 * Adaptec 1542. We must also avoid sending Adaptec-compatible
	 * commands to a real bha, lest it go into 1542 emulation mode.
	 * (On an indirect bus like ISA, we should always probe for BusLogic
	 * interfaces before Adaptec interfaces).
	 */

	/*
	 * Make sure we don't match an AHA-1542A or AHA-1542B, by checking
	 * for an extended-geometry register.  The 1542[AB] don't have one.
	 */
	sts = bus_space_read_1(iot, ioh, BHA_EXTGEOM_PORT);
	if (sts == 0xFF)
		return (0);

	/*
	 * Check that we actually know how to use this board.
	 */
	delay(1000);
	inquire.cmd.opcode = BHA_INQUIRE_EXTENDED;
	inquire.cmd.len = sizeof(inquire.reply);
	i = bha_cmd(iot, ioh, (struct bha_softc *)0,
	    sizeof(inquire.cmd), (u_char *)&inquire.cmd,
	    sizeof(inquire.reply), (u_char *)&inquire.reply);

	/*
	 * Some 1542Cs (CP, perhaps not CF, may depend on firmware rev)
	 * have the extended-geometry register and also respond to
	 * BHA_INQUIRE_EXTENDED.  Make sure we never match such cards,
	 * by checking the size of the reply is what a BusLogic card returns.
	 */
	if (i) {
#ifdef BHADEBUG
		printf("bha_find: board returned %d instead of %d to %s\n",
		       i, sizeof(inquire.reply), "INQUIRE_EXTENDED");
#endif
		return (0);
	}

	/* OK, we know we've found a buslogic adaptor. */

	switch (inquire.reply.bus_type) {
	case BHA_BUS_TYPE_24BIT:
	case BHA_BUS_TYPE_32BIT:
		break;
	case BHA_BUS_TYPE_MCA:
		/* We don't grok MicroChannel (yet). */
		return (0);
	default:
		printf("bha_find: illegal bus type %c\n",
		    inquire.reply.bus_type);
		return (0);
	}

	/*
	 * Assume we have a board at this stage setup dma channel from
	 * jumpers and save int level
	 */
	delay(1000);
	config.cmd.opcode = BHA_INQUIRE_CONFIG;
	bha_cmd(iot, ioh, (struct bha_softc *)0,
	    sizeof(config.cmd), (u_char *)&config.cmd,
	    sizeof(config.reply), (u_char *)&config.reply);
	switch (config.reply.chan) {
	case EISADMA:
		drq = -1;
		break;
	case CHAN0:
		drq = 0;
		break;
	case CHAN5:
		drq = 5;
		break;
	case CHAN6:
		drq = 6;
		break;
	case CHAN7:
		drq = 7;
		break;
	default:
		printf("bha_find: illegal drq setting %x\n",
		    config.reply.chan);
		return (0);
	}

	switch (config.reply.intr) {
	case INT9:
		irq = 9;
		break;
	case INT10:
		irq = 10;
		break;
	case INT11:
		irq = 11;
		break;
	case INT12:
		irq = 12;
		break;
	case INT14:
		irq = 14;
		break;
	case INT15:
		irq = 15;
		break;
	default:
		printf("bha_find: illegal irq setting %x\n",
		    config.reply.intr);
		return (0);
	}

	/* if we want to fill in softc, do so now */
	if (sc != NULL) {
		sc->sc_irq = irq;
		sc->sc_drq = drq;
	}

	return (1);
}

/*
 * bha_disable_isacompat:
 *
 *	Disable the ISA-compatiblity ioports on PCI bha devices,
 *	to ensure they're not autoconfigured a second time as an ISA bha.
 */
int
bha_disable_isacompat(sc)
	struct bha_softc *sc;
{
	struct bha_isadisable isa_disable;

	isa_disable.cmd.opcode = BHA_MODIFY_IOPORT;
	isa_disable.cmd.modifier = BHA_IOMODIFY_DISABLE1;
	bha_cmd(sc->sc_iot, sc->sc_ioh, sc,
	    sizeof(isa_disable.cmd), (u_char*)&isa_disable.cmd,
	    0, (u_char *)0);
	return (0);
}

/*
 * bha_info:
 *
 *	Get information about the board, and report it.  We
 *	return the initial number of CCBs, 0 if we failed.
 */
int
bha_info(sc)
	struct bha_softc *sc;
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	struct bha_extended_inquire inquire;
	struct bha_config config;
	struct bha_devices devices;
	struct bha_setup setup;
	struct bha_model model;
	struct bha_revision revision;
	struct bha_digit digit;
	int i, j, initial_ccbs, rlen;
	char *p;

	/*
	 * Fetch the extended inquire information.
	 */
	inquire.cmd.opcode = BHA_INQUIRE_EXTENDED;
	inquire.cmd.len = sizeof(inquire.reply);
	bha_cmd(iot, ioh, sc,
	    sizeof(inquire.cmd), (u_char *)&inquire.cmd,
	    sizeof(inquire.reply), (u_char *)&inquire.reply);

	/*
	 * Fetch the configuration information.
	 */
	config.cmd.opcode = BHA_INQUIRE_CONFIG;
	bha_cmd(iot, ioh, sc,
	    sizeof(config.cmd), (u_char *)&config.cmd,
	    sizeof(config.reply), (u_char *)&config.reply);

	sc->sc_scsi_id = config.reply.scsi_dev;

	/*
	 * Get the firmware revision.
	 */
	p = sc->sc_firmware;
	revision.cmd.opcode = BHA_INQUIRE_REVISION;
	bha_cmd(iot, ioh, sc,
	    sizeof(revision.cmd), (u_char *)&revision.cmd,
	    sizeof(revision.reply), (u_char *)&revision.reply);
	*p++ = revision.reply.firm_revision;
	*p++ = '.';
	*p++ = revision.reply.firm_version;
	digit.cmd.opcode = BHA_INQUIRE_REVISION_3;
	bha_cmd(iot, ioh, sc,
	    sizeof(digit.cmd), (u_char *)&digit.cmd,
	    sizeof(digit.reply), (u_char *)&digit.reply);
	*p++ = digit.reply.digit;
	if (revision.reply.firm_revision >= '3' ||
	    (revision.reply.firm_revision == '3' &&
	     revision.reply.firm_version >= '3')) {
		digit.cmd.opcode = BHA_INQUIRE_REVISION_4;
		bha_cmd(iot, ioh, sc,
		    sizeof(digit.cmd), (u_char *)&digit.cmd,
		    sizeof(digit.reply), (u_char *)&digit.reply);
		*p++ = digit.reply.digit;
	}
	while (p > sc->sc_firmware && (p[-1] == ' ' || p[-1] == '\0'))
		p--;
	*p = '\0';

	/*
	 * Get the model number.
	 *
	 * Some boards do not handle the Inquire Board Model Number
	 * command correctly, or don't give correct information.
	 *
	 * So, we use the Firmware Revision and Extended Setup
	 * information to fixup the model number in these cases.
	 *
	 * The firmware version indicates:
	 *
	 *	5.xx	BusLogic "W" Series Hose Adapters
	 *		BT-948/958/958D
	 *
	 *	4.xx	BusLogic "C" Series Host Adapters
	 *		BT-946C/956C/956CD/747C/757C/757CD/445C/545C/540CF
	 *
	 *	3.xx	BusLogic "S" Series Host Adapters
	 *		BT-747S/747D/757S/757D/445S/545S/542D
	 *		BT-542B/742A (revision H)
	 *
	 *	2.xx	BusLogic "A" Series Host Adapters
	 *		BT-542B/742A (revision G and below)
	 *
	 *	0.xx	AMI FastDisk VLB/EISA BusLogic Clone Host Adapter
	 */
	if (inquire.reply.bus_type == BHA_BUS_TYPE_24BIT &&
	    sc->sc_firmware[0] < '3')
		sprintf(sc->sc_model, "542B");
	else if (inquire.reply.bus_type == BHA_BUS_TYPE_32BIT &&
	    sc->sc_firmware[0] == '2' &&
	    (sc->sc_firmware[2] == '1' ||
	     (sc->sc_firmware[2] == '2' && sc->sc_firmware[3] == '0')))
		sprintf(sc->sc_model, "742A");
	else if (inquire.reply.bus_type == BHA_BUS_TYPE_32BIT &&
	    sc->sc_firmware[0] == '0')
		sprintf(sc->sc_model, "747A");
	else {
		p = sc->sc_model;
		model.cmd.opcode = BHA_INQUIRE_MODEL;
		model.cmd.len = sizeof(model.reply);
		bha_cmd(iot, ioh, sc,
		    sizeof(model.cmd), (u_char *)&model.cmd,
		    sizeof(model.reply), (u_char *)&model.reply);
		*p++ = model.reply.id[0];
		*p++ = model.reply.id[1];
		*p++ = model.reply.id[2];
		*p++ = model.reply.id[3];
		while (p > sc->sc_model && (p[-1] == ' ' || p[-1] == '\0'))
			p--;
		*p++ = model.reply.version[0];
		*p++ = model.reply.version[1];
		while (p > sc->sc_model && (p[-1] == ' ' || p[-1] == '\0'))
			p--;
		*p = '\0';
	}

	/* Enable round-robin scheme - appeared at firmware rev. 3.31. */
	if (strcmp(sc->sc_firmware, "3.31") >= 0)
		sc->sc_flags |= BHAF_STRICT_ROUND_ROBIN;

	/*
	 * Determine some characteristics about our bus.
	 */
	if (inquire.reply.scsi_flags & BHA_SCSI_WIDE)
		sc->sc_flags |= BHAF_WIDE;
	if (inquire.reply.scsi_flags & BHA_SCSI_DIFFERENTIAL)
		sc->sc_flags |= BHAF_DIFFERENTIAL;
	if (inquire.reply.scsi_flags & BHA_SCSI_ULTRA)
		sc->sc_flags |= BHAF_ULTRA;

	/*
	 * Determine some characterists of the board.
	 */
	sc->sc_max_dmaseg = inquire.reply.sg_limit;

	/*
	 * Determine the maximum CCB cound and whether or not
	 * tagged queueing is available on this host adapter.
	 *
	 * Tagged queueing works on:
	 *
	 *	"W" Series adapters
	 *	"C" Series adapters with firmware >= 4.22
	 *	"S" Series adapters with firmware >= 3.35
	 *
	 * The internal CCB counts are:
	 *
	 *	192	BT-948/958/958D
	 *	100	BT-946C/956C/956CD/747C/757C/757CD/445C
	 *	50	BT-545C/540CF
	 *	30	BT-747S/747D/757S/757D/445S/545S/542D/542B/742A
	 */
	switch (sc->sc_firmware[0]) {
	case '5':
		sc->sc_hw_ccbs = 192;
		sc->sc_flags |= BHAF_TAGGED_QUEUEING;      
		break;

	case '4':
		if (sc->sc_model[0] == '5')
			sc->sc_hw_ccbs = 50;
		else
			sc->sc_hw_ccbs = 100;
		if (strcmp(sc->sc_firmware, "4.22") >= 0)
			sc->sc_flags |= BHAF_TAGGED_QUEUEING;
		break;

	case '3':
		if (strcmp(sc->sc_firmware, "3.35") >= 0)
			sc->sc_flags |= BHAF_TAGGED_QUEUEING;
		/* FALLTHROUGH */

	default:
		sc->sc_hw_ccbs = 30;
	}

	/*
	 * Set the mailbox size to be just larger than the internal
	 * CCB count.
	 *
	 * XXX We should consider making this a large number on
	 * boards with strict round-robin mode, as it would allow
	 * us to expand the openings available to the upper layer.
	 * The CCB count is what the host adapter can process
	 * concurrently, but we can queue up to 255 in the mailbox
	 * regardless.
	 */
	if (sc->sc_flags & BHAF_STRICT_ROUND_ROBIN) {
#if 0
		sc->sc_mbox_count = 255;
#else
		sc->sc_mbox_count = sc->sc_hw_ccbs + 8;
#endif
	} else {
		/*
		 * Only 32 in this case; non-strict round-robin must
		 * scan the entire mailbox for new commands, which
		 * is not very efficient.
		 */
		sc->sc_mbox_count = 32;
	}

	/*
	 * The maximum number of CCBs we allow is the number we can
	 * enqueue.
	 */
	sc->sc_max_ccbs = sc->sc_mbox_count;

	/*
	 * Obtain setup information.
	 */
	rlen = sizeof(setup.reply) +
	    ((sc->sc_flags & BHAF_WIDE) ? sizeof(setup.reply_w) : 0);
	setup.cmd.opcode = BHA_INQUIRE_SETUP;
	setup.cmd.len = rlen;
	bha_cmd(iot, ioh, sc,
	    sizeof(setup.cmd), (u_char *)&setup.cmd,
	    rlen, (u_char *)&setup.reply);

	printf("%s: model BT-%s, firmware %s\n", sc->sc_dev.dv_xname,
	    sc->sc_model, sc->sc_firmware);

	printf("%s: %d H/W CCBs", sc->sc_dev.dv_xname, sc->sc_hw_ccbs);
	if (setup.reply.sync_neg)
		printf(", sync");
	if (setup.reply.parity)
		printf(", parity");
	if (sc->sc_flags & BHAF_TAGGED_QUEUEING)
		printf(", tagged queueing");
	if (sc->sc_flags & BHAF_WIDE_LUN)
		printf(", wide LUN support");
	printf("\n");

	/*
	 * Poll targets 0 - 7.
	 */
	devices.cmd.opcode = BHA_INQUIRE_DEVICES;
	bha_cmd(iot, ioh, sc,
	    sizeof(devices.cmd), (u_char *)&devices.cmd,
	    sizeof(devices.reply), (u_char *)&devices.reply);

	/* Count installed units. */
	initial_ccbs = 0;
	for (i = 0; i < 8; i++) {
		for (j = 0; j < 8; j++) {
			if (((devices.reply.lun_map[i] >> j) & 1) == 1)
				initial_ccbs++;
		}
	}

	/*
	 * Poll targets 8 - 15 if we have a wide bus.
	 */
	if (sc->sc_flags & BHAF_WIDE) {
		devices.cmd.opcode = BHA_INQUIRE_DEVICES_2;
		bha_cmd(iot, ioh, sc,
		    sizeof(devices.cmd), (u_char *)&devices.cmd,
		    sizeof(devices.reply), (u_char *)&devices.reply);

		for (i = 0; i < 8; i++) {
			for (j = 0; j < 8; j++) {
				if (((devices.reply.lun_map[i] >> j) & 1) == 1)
					initial_ccbs++;
			}
		}
	}

	/*
	 * Double the initial CCB count, for good measure.
	 */
	initial_ccbs *= 2;

	/*
	 * Sanity check the initial CCB count; don't create more than
	 * we can enqueue (sc_max_ccbs), and make sure there are some
	 * at all.
	 */
	if (initial_ccbs > sc->sc_max_ccbs)
		initial_ccbs = sc->sc_max_ccbs;
	if (initial_ccbs == 0)
		initial_ccbs = 2;

	return (initial_ccbs);
}

/*
 * bha_init:
 *
 *	Initialize the board.
 */
int
bha_init(sc)
	struct bha_softc *sc;
{
	struct bha_toggle toggle;
	struct bha_mailbox mailbox;
	struct bha_mbx_out *mbo;
	struct bha_mbx_in *mbi;
	int i;

	/*
	 * Set up the mailbox.  We always run the mailbox in round-robin.
	 */
	for (i = 0; i < sc->sc_mbox_count; i++) {
		mbo = &sc->sc_mbo[i];
		mbi = &sc->sc_mbi[i];

		mbo->cmd = BHA_MBO_FREE;
		BHA_MBO_SYNC(sc, mbo, BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

		mbi->comp_stat = BHA_MBI_FREE;
		BHA_MBI_SYNC(sc, mbi, BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);
	}

	sc->sc_cmbo = sc->sc_tmbo = &sc->sc_mbo[0];
	sc->sc_tmbi = &sc->sc_mbi[0];

	sc->sc_mbofull = 0;

	/*
	 * If the board supports strict round-robin, enable that.
	 */
	if (sc->sc_flags & BHAF_STRICT_ROUND_ROBIN) {
		toggle.cmd.opcode = BHA_ROUND_ROBIN;
		toggle.cmd.enable = 1;
		bha_cmd(sc->sc_iot, sc->sc_ioh, sc,
		    sizeof(toggle.cmd), (u_char *)&toggle.cmd,
		    0, NULL);
	}

	/*
	 * Give the mailbox to the board.
	 */
	mailbox.cmd.opcode = BHA_MBX_INIT_EXTENDED;
	mailbox.cmd.nmbx = sc->sc_mbox_count;
	ltophys(sc->sc_dmamap_mbox->dm_segs[0].ds_addr, mailbox.cmd.addr);
	bha_cmd(sc->sc_iot, sc->sc_ioh, sc,
	    sizeof(mailbox.cmd), (u_char *)&mailbox.cmd,
	    0, (u_char *)0);

	return (0);
}

/*****************************************************************************
 * CCB execution engine
 *****************************************************************************/

/*
 * bha_queue_ccb:
 *
 *	Queue a CCB to be sent to the controller, and send it if possible.
 */
void
bha_queue_ccb(sc, ccb)
	struct bha_softc *sc;
	struct bha_ccb *ccb;
{

	TAILQ_INSERT_TAIL(&sc->sc_waiting_ccb, ccb, chain);
	bha_start_ccbs(sc);
}

/*
 * bha_start_ccbs:
 *
 *	Send as many CCBs as we have empty mailboxes for.
 */
void
bha_start_ccbs(sc)
	struct bha_softc *sc;
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	struct bha_ccb_group *bcg;
	struct bha_mbx_out *mbo;
	struct bha_ccb *ccb;

	mbo = sc->sc_tmbo;

	while ((ccb = TAILQ_FIRST(&sc->sc_waiting_ccb)) != NULL) {
		if (sc->sc_mbofull >= sc->sc_mbox_count) {
#ifdef DIAGNOSTIC
			if (sc->sc_mbofull > sc->sc_mbox_count)
				panic("bha_start_ccbs: mbofull > mbox_count");
#endif
			/*
			 * No mailboxes available; attempt to collect ones
			 * that have already been used.
			 */
			bha_collect_mbo(sc);
			if (sc->sc_mbofull == sc->sc_mbox_count) {
				/*
				 * Still no more available; have the
				 * controller interrupt us when it
				 * frees one.
				 */
				struct bha_toggle toggle;

				toggle.cmd.opcode = BHA_MBO_INTR_EN;
				toggle.cmd.enable = 1;
				bha_cmd(iot, ioh, sc,
				    sizeof(toggle.cmd), (u_char *)&toggle.cmd,
				    0, (u_char *)0);
				break;
			}
		}

		TAILQ_REMOVE(&sc->sc_waiting_ccb, ccb, chain);
#ifdef BHADIAG
		ccb->flags |= CCB_SENDING;
#endif

		/*
		 * Put the CCB in the mailbox.
		 */
		bcg = BHA_CCB_GROUP(ccb);
		ltophys(bcg->bcg_dmamap->dm_segs[0].ds_addr +
		    BHA_CCB_OFFSET(ccb), mbo->ccb_addr);
		if (ccb->flags & CCB_ABORT)
			mbo->cmd = BHA_MBO_ABORT;
		else
			mbo->cmd = BHA_MBO_START;

		BHA_MBO_SYNC(sc, mbo,
		    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

		/* Tell the card to poll immediately. */
		bus_space_write_1(iot, ioh, BHA_CMD_PORT, BHA_START_SCSI);

		if ((ccb->xs->xs_control & XS_CTL_POLL) == 0)
			timeout(bha_timeout, ccb, (ccb->timeout * hz) / 1000);

		++sc->sc_mbofull;
		mbo = bha_nextmbo(sc, mbo);
	}

	sc->sc_tmbo = mbo;
}

/*
 * bha_finish_ccbs:
 *
 *	Finalize the execution of CCBs in our incoming mailbox.
 */
void
bha_finish_ccbs(sc)
	struct bha_softc *sc;
{
	struct bha_mbx_in *mbi;
	struct bha_ccb *ccb;
	int i;

	mbi = sc->sc_tmbi;

	BHA_MBI_SYNC(sc, mbi, BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);

	if (mbi->comp_stat == BHA_MBI_FREE) {
		for (i = 0; i < sc->sc_mbox_count; i++) {
			if (mbi->comp_stat != BHA_MBI_FREE) {
				printf("%s: mbi not in round-robin order\n",
				    sc->sc_dev.dv_xname);
				goto again;
			}
			mbi = bha_nextmbi(sc, mbi);
			BHA_MBI_SYNC(sc, mbi,
			    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);
		}
#ifdef BHADIAGnot
		printf("%s: mbi interrupt with no full mailboxes\n",
		    sc->sc_dev.dv_xname);
#endif
		return;
	}

 again:
	do {
		ccb = bha_ccb_phys_kv(sc, phystol(mbi->ccb_addr));
		if (ccb == NULL) {
			printf("%s: bad mbi ccb pointer 0x%08x; skipping\n",
			    sc->sc_dev.dv_xname, phystol(mbi->ccb_addr));
			goto next;
		}

		BHA_CCB_SYNC(sc, ccb,
		    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);

#ifdef BHADEBUG
		if (bha_debug) {
			struct scsi_generic *cmd = &ccb->scsi_cmd;
			printf("op=%x %x %x %x %x %x\n",
			    cmd->opcode, cmd->bytes[0], cmd->bytes[1],
			    cmd->bytes[2], cmd->bytes[3], cmd->bytes[4]);
			printf("comp_stat %x for mbi addr = 0x%p, ",
			    mbi->comp_stat, mbi);
			printf("ccb addr = %p\n", ccb);
		}
#endif /* BHADEBUG */

		switch (mbi->comp_stat) {
		case BHA_MBI_OK:
		case BHA_MBI_ERROR:
			if ((ccb->flags & CCB_ABORT) != 0) {
				/*
				 * If we already started an abort, wait for it
				 * to complete before clearing the CCB.  We
				 * could instead just clear CCB_SENDING, but
				 * what if the mailbox was already received?
				 * The worst that happens here is that we clear
				 * the CCB a bit later than we need to.  BFD.
				 */
				goto next;
			}
			break;

		case BHA_MBI_ABORT:
		case BHA_MBI_UNKNOWN:
			/*
			 * Even if the CCB wasn't found, we clear it anyway.
			 * See preceeding comment.
			 */
			break;

		default:
			printf("%s: bad mbi comp_stat %02x; skipping\n",
			    sc->sc_dev.dv_xname, mbi->comp_stat);
			goto next;
		}

		untimeout(bha_timeout, ccb);
		bha_done(sc, ccb);

	next:
		mbi->comp_stat = BHA_MBI_FREE;
		BHA_CCB_SYNC(sc, ccb,
		    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

		mbi = bha_nextmbi(sc, mbi);
		BHA_MBI_SYNC(sc, mbi,
		    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);
	} while (mbi->comp_stat != BHA_MBI_FREE);

	sc->sc_tmbi = mbi;
}

/*****************************************************************************
 * Mailbox management functions.
 *****************************************************************************/

/*
 * bha_create_mailbox:
 *
 *	Create the mailbox structures.  Helper function for bha_attach().
 *
 *	NOTE: The Buslogic hardware only gets one DMA address for the
 *	mailbox!  It expects:
 *
 *		mailbox_out[mailbox_size]
 *		mailbox_in[mailbox_size]
 */
int
bha_create_mailbox(sc)
	struct bha_softc *sc;
{
	bus_dma_segment_t seg;
	size_t size;
	int error, rseg;

	size = (sizeof(struct bha_mbx_out) * sc->sc_mbox_count) +
	       (sizeof(struct bha_mbx_in)  * sc->sc_mbox_count);

	error = bus_dmamem_alloc(sc->sc_dmat, size, PAGE_SIZE, 0, &seg,
	    1, &rseg, sc->sc_dmaflags);
	if (error) {
		printf("%s: unable to allocate mailboxes, error = %d\n",
		    sc->sc_dev.dv_xname, error);
		goto bad_0;
	}

	error = bus_dmamem_map(sc->sc_dmat, &seg, rseg, size,
	    (caddr_t *)&sc->sc_mbo, sc->sc_dmaflags | BUS_DMA_COHERENT);
	if (error) {
		printf("%s: unable to map mailboxes, error = %d\n",
		    sc->sc_dev.dv_xname, error);
		goto bad_1;
	}

	memset(sc->sc_mbo, 0, size);

	error = bus_dmamap_create(sc->sc_dmat, size, 1, size, 0,
	    sc->sc_dmaflags, &sc->sc_dmamap_mbox);
	if (error) {
		printf("%s: unable to create mailbox DMA map, error = %d\n",
		    sc->sc_dev.dv_xname, error);
		goto bad_2;
	}

	error = bus_dmamap_load(sc->sc_dmat, sc->sc_dmamap_mbox,
	    sc->sc_mbo, size, NULL, 0);
	if (error) {
		printf("%s: unable to load mailbox DMA map, error = %d\n",
		    sc->sc_dev.dv_xname, error);
		goto bad_3;
	}

	sc->sc_mbi = (struct bha_mbx_in *)(sc->sc_mbo + sc->sc_mbox_count);

	return (0);

 bad_3:
	bus_dmamap_destroy(sc->sc_dmat, sc->sc_dmamap_mbox);
 bad_2:
	bus_dmamem_unmap(sc->sc_dmat, (caddr_t)sc->sc_mbo, size);
 bad_1:
	bus_dmamem_free(sc->sc_dmat, &seg, rseg);
 bad_0:
	return (error);
}

/*
 * bha_collect_mbo:
 *
 *	Garbage collect mailboxes that are no longer in use.
 */
void
bha_collect_mbo(sc)
	struct bha_softc *sc;
{
	struct bha_mbx_out *mbo;
#ifdef BHADIAG
	struct bha_ccb *ccb;
#endif
	
	mbo = sc->sc_cmbo;

	while (sc->sc_mbofull > 0) {
		BHA_MBO_SYNC(sc, mbo,
		    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);
		if (mbo->cmd != BHA_MBO_FREE)
			break;

#ifdef BHADIAG
		ccb = bha_ccb_phys_kv(sc, phystol(mbo->ccb_addr));
		ccb->flags &= ~CCB_SENDING;
#endif

		--sc->sc_mbofull;
		mbo = bha_nextmbo(sc, mbo);
	}

	sc->sc_cmbo = mbo;
}

/*****************************************************************************
 * CCB management functions
 *****************************************************************************/

__inline void bha_reset_ccb __P((struct bha_ccb *));

__inline void
bha_reset_ccb(ccb)
	struct bha_ccb *ccb;
{

	ccb->flags = 0;
}

/*
 * bha_create_ccbs:
 *
 *	Create a set of CCBs.
 *
 *	We determine the target CCB count, and then keep creating them
 *	until we reach the target, or fail.  CCBs that are allocated
 *	but not "created" are left on the allocating list.
 */
void
bha_create_ccbs(sc, count)
	struct bha_softc *sc;
	int count;
{
	struct bha_ccb_group *bcg;
	struct bha_ccb *ccb;
	bus_dma_segment_t seg;
	bus_dmamap_t ccbmap;
	int target, i, error, rseg;

	/*
	 * If the current CCB count is already the max number we're
	 * allowed to have, bail out now.
	 */
	if (sc->sc_cur_ccbs == sc->sc_max_ccbs)
		return;

	/*
	 * Compute our target count, and clamp it down to the max
	 * number we're allowed to have.
	 */
	target = sc->sc_cur_ccbs + count;
	if (target > sc->sc_max_ccbs)
		target = sc->sc_max_ccbs;

	/*
	 * If there are CCBs on the allocating list, don't allocate a
	 * CCB group yet.
	 */
	if (TAILQ_FIRST(&sc->sc_allocating_ccbs) != NULL)
		goto have_allocating_ccbs;

 allocate_group:
	error = bus_dmamem_alloc(sc->sc_dmat, PAGE_SIZE,
	    PAGE_SIZE, 0, &seg, 1, &rseg, sc->sc_dmaflags | BUS_DMA_NOWAIT);
	if (error) {
		printf("%s: unable to allocate CCB group, error = %d\n",
		    sc->sc_dev.dv_xname, error);
		goto bad_0;
	}

	error = bus_dmamem_map(sc->sc_dmat, &seg, rseg, PAGE_SIZE,
	    (caddr_t *)&bcg,
	    sc->sc_dmaflags | BUS_DMA_NOWAIT | BUS_DMA_COHERENT);
	if (error) {
		printf("%s: unable to map CCB group, error = %d\n",
		    sc->sc_dev.dv_xname, error);
		goto bad_1;
	}

	memset(bcg, 0, PAGE_SIZE);

	error = bus_dmamap_create(sc->sc_dmat, PAGE_SIZE,
	    1, PAGE_SIZE, 0, sc->sc_dmaflags | BUS_DMA_NOWAIT, &ccbmap);
	if (error) {
		printf("%s: unable to create CCB group DMA map, error = %d\n",
		    sc->sc_dev.dv_xname, error);
		goto bad_2;
	}

	error = bus_dmamap_load(sc->sc_dmat, ccbmap, bcg, PAGE_SIZE, NULL,
	    sc->sc_dmaflags | BUS_DMA_NOWAIT);
	if (error) {
		printf("%s: unable to load CCB group DMA map, error = %d\n",
		    sc->sc_dev.dv_xname, error);
		goto bad_3;
	}

	bcg->bcg_dmamap = ccbmap;

#ifdef DIAGNOSTIC
	if (BHA_CCB_GROUP(&bcg->bcg_ccbs[0]) !=
	    BHA_CCB_GROUP(&bcg->bcg_ccbs[bha_ccbs_per_group - 1]))
		panic("bha_create_ccbs: CCB group size botch");
#endif

	/*
	 * Add all of the CCBs in this group to the allocating list.
	 */
	for (i = 0; i < bha_ccbs_per_group; i++) {
		ccb = &bcg->bcg_ccbs[i];
		TAILQ_INSERT_TAIL(&sc->sc_allocating_ccbs, ccb, chain);
	}

 have_allocating_ccbs:
	/*
	 * Loop over the allocating list until we reach our CCB target.
	 * If we run out on the list, we'll allocate another group's
	 * worth.
	 */
	while (sc->sc_cur_ccbs < target) {
		ccb = TAILQ_FIRST(&sc->sc_allocating_ccbs);
		if (ccb == NULL)
			goto allocate_group;
		if (bha_init_ccb(sc, ccb) != 0) {
			/*
			 * We were unable to initialize the CCB.
			 * This is likely due to a resource shortage,
			 * so bail out now.
			 */
			return;
		}
	}

	/*
	 * If we got here, we've reached our target!
	 */
	return;

 bad_3:
	bus_dmamap_destroy(sc->sc_dmat, ccbmap);
 bad_2:
	bus_dmamem_unmap(sc->sc_dmat, (caddr_t)bcg, PAGE_SIZE);
 bad_1:
	bus_dmamem_free(sc->sc_dmat, &seg, rseg);
 bad_0:
	return;
}

/*
 * bha_init_ccb:
 *
 *	Initialize a CCB; helper function for bha_create_ccbs().
 */
int
bha_init_ccb(sc, ccb)
	struct bha_softc *sc;
	struct bha_ccb *ccb;
{
	struct bha_ccb_group *bcg = BHA_CCB_GROUP(ccb);
	int hashnum, error;

	/*
	 * Create the DMA map for this CCB.
	 *
	 * XXX ALLOCNOW is a hack to prevent bounce buffer shortages
	 * XXX in the ISA case.  A better solution is needed.
	 */
	error = bus_dmamap_create(sc->sc_dmat, BHA_MAXXFER, BHA_NSEG,
	    BHA_MAXXFER, 0, BUS_DMA_NOWAIT | BUS_DMA_ALLOCNOW | sc->sc_dmaflags,
	    &ccb->dmamap_xfer);
	if (error) {
		printf("%s: unable to create CCB DMA map, error = %d\n",
		    sc->sc_dev.dv_xname, error);
		return (error);
	}

	TAILQ_REMOVE(&sc->sc_allocating_ccbs, ccb, chain);

	/*
	 * Put the CCB into the phystokv hash table.
	 */
	ccb->hashkey = bcg->bcg_dmamap->dm_segs[0].ds_addr +
	    BHA_CCB_OFFSET(ccb);
	hashnum = CCB_HASH(ccb->hashkey);
	ccb->nexthash = sc->sc_ccbhash[hashnum];
	sc->sc_ccbhash[hashnum] = ccb;
	bha_reset_ccb(ccb);
	
	TAILQ_INSERT_HEAD(&sc->sc_free_ccb, ccb, chain);
	sc->sc_cur_ccbs++;

	return (0);
}

/*
 * bha_get_ccb:
 *
 *	Get a CCB for the SCSI operation.  If there are none left,
 *	wait until one becomes available, if we can.
 */
struct bha_ccb *
bha_get_ccb(sc, flags)
	struct bha_softc *sc;
	int flags;
{
	struct bha_ccb *ccb;
	int s;

	s = splbio();

	for (;;) {
		ccb = TAILQ_FIRST(&sc->sc_free_ccb);
		if (ccb) {
			TAILQ_REMOVE(&sc->sc_free_ccb, ccb, chain);
			break;
		}
		if ((flags & XS_CTL_NOSLEEP) != 0)
			goto out;
		tsleep(&sc->sc_free_ccb, PRIBIO, "bhaccb", 0);
	}

	ccb->flags |= CCB_ALLOC;

out:
	splx(s);
	return (ccb);
}

/*
 * bha_free_ccb:
 *
 *	Put a CCB back onto the free list.
 */
void
bha_free_ccb(sc, ccb)
	struct bha_softc *sc;
	struct bha_ccb *ccb;
{
	int s;

	s = splbio();

	bha_reset_ccb(ccb);
	TAILQ_INSERT_HEAD(&sc->sc_free_ccb, ccb, chain);

	/*
	 * If there were none, wake anybody waiting for one to come free,
	 * starting with queued entries.
	 */
	if (TAILQ_NEXT(ccb, chain) == NULL)
		wakeup(&sc->sc_free_ccb);

	splx(s);
}

/*
 * bha_ccb_phys_kv:
 *
 *	Given a CCB DMA address, locate the CCB in kernel virtual space.
 */
struct bha_ccb *
bha_ccb_phys_kv(sc, ccb_phys)
	struct bha_softc *sc;
	bus_addr_t ccb_phys;
{
	int hashnum = CCB_HASH(ccb_phys);
	struct bha_ccb *ccb = sc->sc_ccbhash[hashnum];

	while (ccb) {
		if (ccb->hashkey == ccb_phys)
			break;
		ccb = ccb->nexthash;
	}
	return (ccb);
}
