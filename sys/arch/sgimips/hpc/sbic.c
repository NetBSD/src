/*	$NetBSD: sbic.c,v 1.9 2002/04/05 18:27:46 bouyer Exp $	*/

/*
 * Changes Copyright (c) 2001 Wayne Knowles
 * Changes Copyright (c) 1996 Steve Woodford
 * Original Copyright (c) 1994 Christian E. Hopps
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Van Jacobson of Lawrence Berkeley Laboratory.
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
 *  This product includes software developed by the University of
 *  California, Berkeley and its contributors.
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
 *  @(#)scsi.c  7.5 (Berkeley) 5/4/91
 */

/*
 * This version of the driver is pretty well generic, so should work with
 * any flavour of WD33C93 chip.
 */
#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h> /* For hz */
#include <sys/disklabel.h>
#include <sys/dkstat.h>
#include <sys/buf.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>
#include <dev/scsipi/scsi_message.h>

#include <uvm/uvm_extern.h>

#include <machine/bus.h>

#include <sgimips/hpc/sbicreg.h>
#include <sgimips/hpc/sbicvar.h>

/*
 * SCSI delays
 * In u-seconds, primarily for state changes on the SPC.
 */
#define SBIC_CMD_WAIT	50000	/* wait per step of 'immediate' cmds */
#define SBIC_DATA_WAIT	50000	/* wait per data in/out step */
#define SBIC_INIT_WAIT	50000	/* wait per step (both) during init */

#define STATUS_UNKNOWN	0xff	/* uninitialized status */

/*
 * Convenience macro for waiting for a particular wd33c93 event
 */
#define SBIC_WAIT(regs, until, timeo) wd33c93_wait(regs, until, timeo, __LINE__)

void	wd33c93_init __P((struct wd33c93_softc *));
void	wd33c93_reset __P((struct wd33c93_softc *));
int	wd33c93_go __P((struct wd33c93_softc *, struct wd33c93_acb *));
int	wd33c93_dmaok __P((struct wd33c93_softc *, struct scsipi_xfer *));
int	wd33c93_wait __P((struct wd33c93_softc *, u_char, int , int));
u_char	wd33c93_selectbus __P((struct wd33c93_softc *, struct wd33c93_acb *));
int	wd33c93_xfout __P((struct wd33c93_softc *, int, void *));
int	wd33c93_xfin __P((struct wd33c93_softc *, int, void *));
int	wd33c93_poll __P((struct wd33c93_softc *, struct wd33c93_acb *));
int	wd33c93_nextstate __P((struct wd33c93_softc *, struct wd33c93_acb *,
				u_char, u_char));
int	wd33c93_abort __P((struct wd33c93_softc *, struct wd33c93_acb *, char *));
void	wd33c93_xferdone __P((struct wd33c93_softc *));
void	wd33c93_error __P((struct wd33c93_softc *, struct wd33c93_acb *));
void	wd33c93_scsidone __P((struct wd33c93_softc *, struct wd33c93_acb *, int));
void	wd33c93_sched __P((struct wd33c93_softc *));
void	wd33c93_dequeue __P((struct wd33c93_softc *, struct wd33c93_acb *));
void	wd33c93_dma_stop __P((struct wd33c93_softc *));
void	wd33c93_dma_setup __P((struct wd33c93_softc *, int));
int	wd33c93_msgin_phase __P((struct wd33c93_softc *, int));
void	wd33c93_msgin __P((struct wd33c93_softc *, u_char *, int));
void	wd33c93_reselect __P((struct wd33c93_softc *, int, int, int, int));
void	wd33c93_sched_msgout __P((struct wd33c93_softc *, u_short));
void	wd33c93_msgout __P((struct wd33c93_softc *));
void	wd33c93_timeout __P((void *arg));
void	wd33c93_watchdog __P((void *arg));
int	wd33c93_div2stp __P((struct wd33c93_softc *, int));
int	wd33c93_stp2div __P((struct wd33c93_softc *, int));
void	wd33c93_setsync __P((struct wd33c93_softc *, struct wd33c93_tinfo *ti));
void	wd33c93_update_xfer_mode __P((struct wd33c93_softc *, int));

static struct pool wd33c93_pool;		/* Adapter Control Blocks */
static int wd33c93_pool_initialized = 0;

/*
 * Timeouts
 */
int	wd33c93_cmd_wait	= SBIC_CMD_WAIT;
int	wd33c93_data_wait	= SBIC_DATA_WAIT;
int	wd33c93_init_wait	= SBIC_INIT_WAIT;

int	wd33c93_nodma		= 0;	/* Use polled IO transfers */
int	wd33c93_nodisc		= 0;	/* Allow command queues */
int	wd33c93_notags		= 0;	/* No Tags */

/*
 * Some useful stuff for debugging purposes
 */
#ifdef DEBUG

#define QPRINTF(a)	SBIC_DEBUG(MISC, a)

int	wd33c93_debug	= 0;		/* Debug flags */

void	wd33c93_print_csr __P((u_char));
void	wd33c93_hexdump __P((u_char *, int));

#else
#define QPRINTF(a)  /* */
#endif

static const char *wd33c93_chip_names[] = SBIC_CHIP_LIST;

/*
 * Attach instance of driver and probe for sub devices
 */
void
wd33c93_attach(dev)
	struct wd33c93_softc *dev;
{
	struct scsipi_adapter *adapt = &dev->sc_adapter;
	struct scsipi_channel *chan = &dev->sc_channel;

	adapt->adapt_dev = &dev->sc_dev;
	adapt->adapt_nchannels = 1;
	adapt->adapt_openings = 256;
	adapt->adapt_max_periph = 256; /* Max tags per device */
	adapt->adapt_ioctl = NULL;
	/* adapt_request initialized by MD interface */
	/* adapt_minphys initialized by MD interface */

	memset(chan, 0, sizeof(*chan));
	chan->chan_adapter = &dev->sc_adapter;
	chan->chan_bustype = &scsi_bustype;
	chan->chan_channel = 0;
	chan->chan_ntargets = SBIC_NTARG;
	chan->chan_nluns = SBIC_NLUN;
	chan->chan_id = dev->sc_id;

	callout_init(&dev->sc_watchdog);

	dev->sc_minsync = 200/4; /* Min SCSI sync rate in 4ns units */
	dev->sc_maxoffset = SBIC_SYN_MAX_OFFSET; /* Max Sync Offset */

	/*
	 * Add reference to adapter so that we drop the reference after
	 * config_found() to make sure the adatper is disabled.
	 */
	if (scsipi_adapter_addref(&dev->sc_adapter) != 0) {
		printf("%s: unable to enable controller\n",
		    dev->sc_dev.dv_xname);
		return;
	}

	dev->sc_cfflags = dev->sc_dev.dv_cfdata->cf_flags;
	wd33c93_init(dev);

	dev->sc_child = config_found(&dev->sc_dev, &dev->sc_channel,
				     scsiprint);
	scsipi_adapter_delref(&dev->sc_adapter);
}

/*
 * Initialize driver-private structures
 */
void
wd33c93_init(dev)
	struct wd33c93_softc *dev;
{
	u_int i;

	if (!wd33c93_pool_initialized) {
		/* All instances share the same pool */
		pool_init(&wd33c93_pool, sizeof(struct wd33c93_acb), 0, 0, 0,
		    "wd33c93_acb", NULL);
		++wd33c93_pool_initialized;
	}

	if (dev->sc_state == 0) {
		TAILQ_INIT(&dev->ready_list);

		dev->sc_nexus = NULL;
		dev->sc_disc  = 0;
		memset(dev->sc_tinfo, 0, sizeof(dev->sc_tinfo));

		callout_reset(&dev->sc_watchdog, 60 * hz, wd33c93_watchdog, dev);
	} else
		panic("wd33c93: reinitializing driver!");

	dev->sc_flags = 0;
	dev->sc_state = SBIC_IDLE;
	wd33c93_reset(dev);

	for (i = 0; i < 8; i++) {
		struct wd33c93_tinfo *ti = &dev->sc_tinfo[i];
		/*
		 * sc_flags = 0xTTRRSS
		 *
		 *   TT = Bitmask to disable Tagged Queues
		 *   RR = Bitmask to disable disconnect/reselect
		 *   SS = Bitmask to diable Sync negotiation
		 */
		ti->flags = T_NEED_RESET;
		if (dev->sc_minsync == 0 || (dev->sc_cfflags & (1<<(i+8))))
			ti->flags |= T_NOSYNC;
		if (dev->sc_cfflags & (1<<i) || wd33c93_nodisc)
			ti->flags |= T_NODISC;
		ti->period = dev->sc_minsync;
		ti->offset = 0;
	}
}

void
wd33c93_reset(dev)
	struct wd33c93_softc *dev;
{
	u_int	my_id, s;
	u_char	csr, reg;

	SET_SBIC_cmd(dev, SBIC_CMD_ABORT);
	WAIT_CIP(dev);

	s = splbio();

	if (dev->sc_reset != NULL)
		(*dev->sc_reset)(dev);

	my_id = dev->sc_channel.chan_id & SBIC_ID_MASK;
	if (dev->sc_clkfreq < 110)
		my_id |= SBIC_ID_FS_8_10;
	else if (dev->sc_clkfreq < 160)
		my_id |= SBIC_ID_FS_12_15;
	else if (dev->sc_clkfreq < 210)
		my_id |= SBIC_ID_FS_16_20;

	/* Enable advanced features */
#if 1
	my_id |= SBIC_ID_EAF;	/* XXX - MD Layer */
#endif

	SET_SBIC_myid(dev, my_id);

	/* Reset the chip */
	SET_SBIC_cmd(dev, SBIC_CMD_RESET);
	DELAY(25);
	SBIC_WAIT(dev, SBIC_ASR_INT, 0);

	/* Set up various chip parameters */
	SET_SBIC_control(dev, SBIC_CTL_EDI | SBIC_CTL_IDI);

	GET_SBIC_csr(dev, csr);		/* clears interrupt also */
	GET_SBIC_cdb1(dev, dev->sc_rev);

	switch (csr) {
	case 0x0:
		dev->sc_chip = SBIC_CHIP_WD33C93;
		break;
	case 0x1:
		SET_SBIC_queue_tag(dev, 0x55);
		GET_SBIC_queue_tag(dev, reg);
		dev->sc_chip = (reg == 0x55) ?
		    	       SBIC_CHIP_WD33C93B : SBIC_CHIP_WD33C93A;
		SET_SBIC_queue_tag(dev, 0x0);
		break;
	default:
		dev->sc_chip = SBIC_CHIP_UNKNOWN;
	}

	/*
	 * don't allow Selection (SBIC_RID_ES)
	 * until we can handle target mode!!
	 */
	SET_SBIC_rselid(dev, SBIC_RID_ER);

	/* Asynchronous for now */
	SET_SBIC_syn(dev, 0);

	dev->sc_flags = 0;
	dev->sc_state = SBIC_IDLE;

	splx(s);

	printf(": %s SCSI, rev=%d, target %d\n",
	    wd33c93_chip_names[dev->sc_chip], dev->sc_rev,
	    dev->sc_channel.chan_id);
}

void
wd33c93_error(dev, acb)
	struct wd33c93_softc *dev;
	struct wd33c93_acb *acb;
{
	struct scsipi_xfer *xs = acb->xs;

	KASSERT(xs);

	if (xs->xs_control & XS_CTL_SILENT)
		return;

	scsipi_printaddr(xs->xs_periph);
	printf("SCSI Error\n");
}

/*
 * Setup sync mode for given target
 */
void
wd33c93_setsync(dev, ti)
	struct wd33c93_softc *dev;
	struct wd33c93_tinfo *ti;
{
	u_char offset, period;

	if (ti->flags & T_SYNCMODE) {
		offset = ti->offset;
		period = wd33c93_stp2div(dev, ti->period);
	} else {
		offset = 0;
		period = 0;
	}

	SBIC_DEBUG(SYNC, ("wd33c93_setsync: sync reg = 0x%02x\n",
		       SBIC_SYN(offset, period)));
	SET_SBIC_syn(dev, SBIC_SYN(offset, period));
}

/*
 * Check if current operation can be done using DMA
 *
 * returns 1 if DMA OK, 0 for polled I/O transfer
 */
int
wd33c93_dmaok(dev, xs)
	struct wd33c93_softc *dev;
	struct scsipi_xfer *xs;
{
	if (wd33c93_nodma || (xs->xs_control & XS_CTL_POLL) || xs->datalen == 0)
		return (0);
	return(1);
}

/*
 * Setup for dma transfer
 */
void
wd33c93_dma_setup(dev, datain)
	struct wd33c93_softc *dev;
	int datain;
{
	struct wd33c93_acb *acb = dev->sc_nexus;
	int s;

	dev->sc_daddr = acb->daddr;
	dev->sc_dleft = acb->dleft;

	s = splbio();
	/* Indicate that we're in DMA mode */
	if (dev->sc_dleft) {
		dev->sc_dmasetup(dev, &dev->sc_daddr, &dev->sc_dleft,
		    datain, &dev->sc_dleft);
	}
	splx(s);
	return;
}


/*
 * Save DMA pointers.  Take into account partial transfer. Shut down DMA.
 */
void
wd33c93_dma_stop(dev)
	struct wd33c93_softc *dev;
{
 	int	count, asr;

	/* Wait until WD chip is idle */
	do {
		GET_SBIC_asr(dev, asr);	/* XXX */
		if (asr & SBIC_ASR_DBR) {
			printf("wd33c93_dma_stop: asr %02x canceled!\n", asr);
			break;
		}
	} while(asr & (SBIC_ASR_BSY|SBIC_ASR_CIP));

	/* Only need to save pointers if DMA was active */
	if (dev->sc_flags & SBICF_INDMA) {
		int s = splbio();

		/* Shut down DMA and flush FIFO's */
		dev->sc_dmastop(dev);

		/* Fetch the residual count */
		SBIC_TC_GET(dev, count);

		/* Work out how many bytes were actually transferred */
		count = dev->sc_tcnt - count;

		if (dev->sc_dleft < count)
			printf("xfer too large: dleft=%u resid=%u\n",
			    dev->sc_dleft, count);

		/* Fixup partial xfers */
		dev->sc_daddr += count;
		dev->sc_dleft -= count;
		dev->sc_tcnt   = 0;
		dev->sc_flags &= ~SBICF_INDMA;
		splx(s);
		SBIC_DEBUG(DMA, ("dma_stop\n"));
	}
	/*
	 * Ensure the WD chip is back in polled I/O mode, with nothing to
	 * transfer.
	 */
	SBIC_TC_PUT(dev, 0);
	SET_SBIC_control(dev, SBIC_CTL_EDI | SBIC_CTL_IDI);
}


/*
 * Handle new request from scsipi layer
 */
void
wd33c93_scsi_request(chan, req, arg)
	struct scsipi_channel *chan;
	scsipi_adapter_req_t req;
	void *arg;
{
	struct wd33c93_softc *dev = (void *)chan->chan_adapter->adapt_dev;
	struct scsipi_xfer *xs;
	struct scsipi_periph *periph;
	struct wd33c93_acb *acb;
	int flags, s;

	switch (req) {
	case ADAPTER_REQ_RUN_XFER:
		xs = arg;
		periph = xs->xs_periph;
		flags = xs->xs_control;

		if (flags & XS_CTL_DATA_UIO)
			panic("wd33c93: scsi data uio requested");

		if (dev->sc_nexus && (flags & XS_CTL_POLL))
			panic("wd33c93_scsicmd: busy");

		s = splbio();
		acb = (struct wd33c93_acb *)pool_get(&wd33c93_pool, PR_NOWAIT);
		splx(s);

		if (acb == NULL) {
			scsipi_printaddr(periph);
			printf("cannot allocate acb\n");
			xs->error = XS_RESOURCE_SHORTAGE;
			scsipi_done(xs);
			return;
		}

		acb->flags = ACB_ACTIVE;
		acb->xs    = xs;
		acb->clen  = xs->cmdlen;
		acb->daddr = xs->data;
		acb->dleft = xs->datalen;
		acb->timeout = xs->timeout;
		memcpy(&acb->cmd, xs->cmd, xs->cmdlen);

		if (flags & XS_CTL_POLL) {
			/*
			 * Complete currently active command(s) before
			 * issuing an immediate command
			 */
			while (dev->sc_nexus)
				wd33c93_poll(dev, dev->sc_nexus);
		}

		s = splbio();
		TAILQ_INSERT_TAIL(&dev->ready_list, acb, chain);
		acb->flags |= ACB_READY;

		/* If nothing is active, try to start it now. */
		if (dev->sc_state == SBIC_IDLE)
			wd33c93_sched(dev);
		splx(s);

		if ((flags & XS_CTL_POLL) == 0)
			return;

		if (wd33c93_poll(dev, acb)) {
			wd33c93_timeout(acb);
			if (wd33c93_poll(dev, acb)) /* 2nd retry for ABORT */
				wd33c93_timeout(acb);
		}
		return;

	case ADAPTER_REQ_GROW_RESOURCES:
		/* XXX Not supported. */
		return;

	case ADAPTER_REQ_SET_XFER_MODE:
	    {
		struct wd33c93_tinfo *ti;
		struct scsipi_xfer_mode *xm = arg;

		ti = &dev->sc_tinfo[xm->xm_target];
		ti->flags &= ~(T_NEGOTIATE|T_SYNCMODE);
		ti->period = 0;
		ti->offset = 0;

		if ((dev->sc_cfflags & (1<<(xm->xm_target+16))) == 0 &&
		    (xm->xm_mode & PERIPH_CAP_TQING) && !wd33c93_notags)
			ti->flags |= T_TAG;
		else
			ti->flags &= ~T_TAG;

		if ((xm->xm_mode & PERIPH_CAP_SYNC) != 0 &&
		    (ti->flags & T_NOSYNC) == 0 && dev->sc_minsync != 0) {
			SBIC_DEBUG(SYNC, ("target %d: sync negotiation\n",
				       xm->xm_target));
			ti->flags |= T_NEGOTIATE;
			ti->period = dev->sc_minsync;
		}
		/*
		 * If we're not going to negotiate, send the notification
		 * now, since it won't happen later.
		 */
		if ((ti->flags & T_NEGOTIATE) == 0)
			wd33c93_update_xfer_mode(dev, xm->xm_target);
		return;
	    }

	}
}

/*
 * attempt to start the next available command
 */
void
wd33c93_sched(dev)
	struct wd33c93_softc *dev;
{
	struct scsipi_periph *periph = NULL; /* Gag the compiler */
	struct wd33c93_acb *acb;
	struct wd33c93_tinfo *ti;
	struct wd33c93_linfo *li;
	int lun, tag, flags;

	if (dev->sc_state != SBIC_IDLE)
		return;

	KASSERT(dev->sc_nexus == NULL);

	/* Loop through the ready list looking for work to do... */
	TAILQ_FOREACH(acb, &dev->ready_list, chain) {
		periph = acb->xs->xs_periph;
		lun = periph->periph_lun;
		ti = &dev->sc_tinfo[periph->periph_target];
		li = TINFO_LUN(ti, lun);

		KASSERT(acb->flags & ACB_READY);

		/* Select type of tag for this command */
		if ((ti->flags & T_NODISC) != 0)
			tag = 0;
		else if ((ti->flags & T_TAG) == 0)
			tag = 0;
		else if ((acb->flags & ACB_SENSE) != 0)
			tag = 0;
		else if (acb->xs->xs_control & XS_CTL_POLL)
			tag = 0; /* No tags for polled commands */
		else
			tag = acb->xs->xs_tag_type;

		if (li == NULL) {
			/* Initialize LUN info and add to list. */
			li = malloc(sizeof(*li), M_DEVBUF, M_NOWAIT);
			if (li == NULL)
				continue;
			memset(li, 0, sizeof(*li));
			li->lun = lun;
			if (lun < SBIC_NLUN)
				ti->lun[lun] = li;
		}
		li->last_used = time.tv_sec;

		/*
		 * We've found a potential command, but is the target/lun busy?
		 */

		if (tag == 0 && li->untagged == NULL)
			li->untagged = acb; /* Issue untagged */

		if (li->untagged != NULL) {
			tag = 0;
			if ((li->state != L_STATE_BUSY) && li->used == 0) {
				/* Issue this untagged command now */
				acb = li->untagged;
				periph = acb->xs->xs_periph;
			} else	/* Not ready yet */
				continue;
		}

		acb->tag_type = tag;
		if (tag != 0) {
			if (li->queued[acb->xs->xs_tag_id])
				printf("queueing to active tag\n");
			li->queued[acb->xs->xs_tag_id] = acb;
			acb->tag_id = acb->xs->xs_tag_id;
			li->used++;
			break;
		}
		if (li->untagged != NULL && (li->state != L_STATE_BUSY)) {
			li->state = L_STATE_BUSY;
			break;
		}
		if (li->untagged == NULL && tag != 0) {
			break;
		} else
			printf("%d:%d busy\n", periph->periph_target,
			    periph->periph_lun);
	}

	if (acb == NULL) {
		SBIC_DEBUG(ACBS, ("wd33c93sched: no work\n"));
		return;			/* did not find an available command */
	}

	SBIC_DEBUG(ACBS, ("wd33c93_sched(%d,%d)\n", periph->periph_target,
		       periph->periph_lun));

	TAILQ_REMOVE(&dev->ready_list, acb, chain);
	acb->flags &= ~ACB_READY;

	flags = acb->xs->xs_control;
	if (flags & XS_CTL_RESET)
		wd33c93_reset(dev);

	/* XXX - Implicitly call scsidone on select timeout */
	if (wd33c93_go(dev, acb) != 0 || acb->xs->error == XS_SELTIMEOUT) {
		acb->dleft = dev->sc_dleft;
		wd33c93_scsidone(dev, acb, dev->sc_status);
		return;
	}

	return;
}

void
wd33c93_scsidone(dev, acb, status)
	struct wd33c93_softc	*dev;
	struct wd33c93_acb	*acb;
	int			status;
{
	struct scsipi_xfer	*xs = acb->xs;
	struct wd33c93_tinfo	*ti;
	struct wd33c93_linfo	*li;
	int			s;

#ifdef DIAGNOSTIC
	KASSERT(dev->target == xs->xs_periph->periph_target);
	KASSERT(dev->lun    == xs->xs_periph->periph_lun);
	if (acb == NULL || xs == NULL) {
		panic("wd33c93_scsidone -- (%d,%d) no scsipi_xfer\n",
		    dev->target, dev->lun);
	}
	KASSERT(acb->flags != ACB_FREE);
#endif

	SBIC_DEBUG(ACBS, ("scsidone: (%d,%d)->(%d,%d)%02x\n",
		       xs->xs_periph->periph_target, xs->xs_periph->periph_lun,
		       dev->target, dev->lun, status));
	callout_stop(&xs->xs_callout);

	xs->status = status & SCSI_STATUS_MASK;
	xs->resid = acb->dleft;

	if (xs->error == XS_NOERROR) {
		switch (xs->status) {
		case SCSI_CHECK:
		case SCSI_TERMINATED:
			/* XXX Need to read sense - return busy for now */
			/*FALLTHROUGH*/
		case SCSI_QUEUE_FULL:
		case SCSI_BUSY:
			xs->error = XS_BUSY;
			break;
		}
	}

	ti = &dev->sc_tinfo[dev->target];
	li = TINFO_LUN(ti, dev->lun);
	ti->cmds++;
	if (xs->error == XS_SELTIMEOUT) {
		/* Selection timeout -- discard this LUN if empty */
		if (li->untagged == NULL && li->used == 0) {
			if (dev->lun < SBIC_NLUN)
				ti->lun[dev->lun] = NULL;
			free(li, M_DEVBUF);
		}
	}

	wd33c93_dequeue(dev, acb);
	if (dev->sc_nexus == acb) {
		dev->sc_state = SBIC_IDLE;
		dev->sc_nexus = NULL;
		dev->sc_flags = 0;

		if (!TAILQ_EMPTY(&dev->ready_list))
			wd33c93_sched(dev);
	}

	/* place control block back on free list. */
	s = splbio();
	acb->flags = ACB_FREE;
	pool_put(&wd33c93_pool, (void *)acb);
	splx(s);

	scsipi_done(xs);
}

void
wd33c93_dequeue(dev, acb)
	struct wd33c93_softc *dev;
	struct wd33c93_acb *acb;
{
	struct wd33c93_tinfo *ti = &dev->sc_tinfo[acb->xs->xs_periph->periph_target];
	struct wd33c93_linfo *li;
	u_int32_t lun = acb->xs->xs_periph->periph_lun;

	li = TINFO_LUN(ti, lun);
#ifdef DIAGNOSTIC
	if (li == NULL || li->lun != lun)
		panic("wd33c93_dequeue: lun %x for ecb %p does not exist\n",
		      lun, acb);
#endif
	if (li->untagged == acb) {
		li->state = L_STATE_IDLE;
		li->untagged = NULL;
	}
	if (acb->tag_type && li->queued[acb->tag_id] != NULL) {
#ifdef DIAGNOSTIC
		if (li->queued[acb->tag_id] != NULL &&
		    (li->queued[acb->tag_id] != acb))
			panic("wd33c93_dequeue: slot %d for lun %x has %p "
			    "instead of acb %p\n", acb->tag_id,
			    lun, li->queued[acb->tag_id], acb);
#endif
		li->queued[acb->tag_id] = NULL;
		li->used--;
	}
}


int
wd33c93_wait(dev, until, timeo, line)
	struct wd33c93_softc	*dev;
	u_char			until;
	int			timeo;
	int			line;
{
	u_char val;

	if (timeo == 0)
		timeo = 1000000;	/* some large value.. */
	GET_SBIC_asr(dev, val);
	while ((val & until) == 0) {
		if (timeo-- == 0) {
			int csr;
			GET_SBIC_csr(dev, csr);
			printf("wd33c93_wait: TIMEO @%d with asr=x%x csr=x%x\n",
			    line, val, csr);
#if defined(DDB) && defined(DEBUG)
			Debugger();
#endif
			return(val); /* Maybe I should abort */
			break;
		}
		DELAY(1);
		GET_SBIC_asr(dev, val);
	}
	return(val);
}

int
wd33c93_abort(dev, acb, where)
	struct wd33c93_softc	*dev;
	struct wd33c93_acb	*acb;
	char			*where;
{
	u_char csr, asr;

	GET_SBIC_asr(dev, asr);
	GET_SBIC_csr(dev, csr);

	scsipi_printaddr(acb->xs->xs_periph);
	printf ("ABORT in %s: csr=0x%02x, asr=0x%02x\n", where, csr, asr);

	acb->timeout = SBIC_ABORT_TIMEOUT;
	acb->flags |= ACB_ABORT;

	/*
	 * Clean up chip itself
	 */
	if (dev->sc_nexus == acb) {
		/* Reschedule timeout. */
		callout_reset(&acb->xs->xs_callout, mstohz(acb->timeout),
		    wd33c93_timeout, acb);

		while (asr & SBIC_ASR_DBR) {
			/*
			 * wd33c93 is jammed w/data. need to clear it
			 * But we don't know what direction it needs to go
			 */
			GET_SBIC_data(dev, asr);
			printf("abort %s: clearing data buffer 0x%02x\n",
			       where, asr);
			GET_SBIC_asr(dev, asr);
			if (asr & SBIC_ASR_DBR) /* Not the read direction */
				SET_SBIC_data(dev, asr);
			GET_SBIC_asr(dev, asr);
		}

		scsipi_printaddr(acb->xs->xs_periph);
		printf("sending ABORT command\n");

		WAIT_CIP(dev);
		SET_SBIC_cmd(dev, SBIC_CMD_ABORT);
		WAIT_CIP(dev);

		GET_SBIC_asr(dev, asr);

		scsipi_printaddr(acb->xs->xs_periph);
		if (asr & (SBIC_ASR_BSY|SBIC_ASR_LCI)) {
			/*
			 * ok, get more drastic..
			 */
			printf("Resetting bus\n");
			wd33c93_reset(dev);
		} else {
			printf("sending DISCONNECT to target\n");
			SET_SBIC_cmd(dev, SBIC_CMD_DISC);
			WAIT_CIP(dev);

			do {
				SBIC_WAIT (dev, SBIC_ASR_INT, 0);
				GET_SBIC_asr(dev, asr);
				GET_SBIC_csr(dev, csr);
				SBIC_DEBUG(MISC, ("csr: 0x%02x, asr: 0x%02x\n",
					       csr, asr));
			} while ((csr != SBIC_CSR_DISC) &&
			    (csr != SBIC_CSR_DISC_1) &&
			    (csr != SBIC_CSR_CMD_INVALID));
		}
		dev->sc_state = SBIC_ERROR;
		dev->sc_flags = 0;
	}
	return SBIC_STATE_ERROR;
}


/*
 * select the bus, return when selected or error.
 *
 * Returns the current CSR following selection and optionally MSG out phase.
 * i.e. the returned CSR *should* indicate CMD phase...
 * If the return value is 0, some error happened.
 */
u_char
wd33c93_selectbus(dev, acb)
	struct wd33c93_softc *dev;
	struct wd33c93_acb *acb;
{
	struct scsipi_xfer *xs = acb->xs;
	struct wd33c93_tinfo *ti;
	u_char target, lun, asr, csr, id;

	KASSERT(dev->sc_state == SBIC_IDLE);

	target = xs->xs_periph->periph_target;
	lun    = xs->xs_periph->periph_lun;
	ti     = &dev->sc_tinfo[target];

	dev->sc_state = SBIC_SELECTING;
	dev->target    = target;
	dev->lun       = lun;

	SBIC_DEBUG(PHASE, ("wd33c93_selectbus %d: ", target));

	if ((xs->xs_control & XS_CTL_POLL) == 0)
		callout_reset(&xs->xs_callout, mstohz(acb->timeout),
		    wd33c93_timeout, acb);

	/*
	 * issue select
	 */
	SBIC_TC_PUT(dev, 0);
	SET_SBIC_selid(dev, target);
	SET_SBIC_timeo(dev, SBIC_TIMEOUT(250, dev->sc_clkfreq));

	GET_SBIC_asr(dev, asr);
	if (asr & (SBIC_ASR_INT|SBIC_ASR_BSY)) {
		/* This means we got ourselves reselected upon */
		SBIC_DEBUG(PHASE, ("WD busy (reselect?) ASR=%02x\n", asr));
		return 0;
	}

	SET_SBIC_cmd(dev, SBIC_CMD_SEL_ATN);
	WAIT_CIP(dev);

	/*
	 * wait for select (merged from separate function may need
	 * cleanup)
	 */
	do {
		asr = SBIC_WAIT(dev, SBIC_ASR_INT | SBIC_ASR_LCI, 0);
		if (asr & SBIC_ASR_LCI) {
			QPRINTF(("late LCI: asr %02x\n", asr));
			return 0;
		}

		/* Clear interrupt */
		GET_SBIC_csr (dev, csr);

		/* Reselected from under our feet? */
		if (csr == SBIC_CSR_RSLT_NI || csr == SBIC_CSR_RSLT_IFY) {
			SBIC_DEBUG(PHASE, ("got reselected, asr %02x\n", asr));
			/*
			 * We need to handle this now so we don't lock up later
			 */
			wd33c93_nextstate(dev, acb, csr, asr);
			return 0;
		}

		/* Whoops! */
		if (csr == SBIC_CSR_SLT || csr == SBIC_CSR_SLT_ATN) {
			panic("wd33c93_selectbus: target issued select!");
			return 0;
		}

	} while (csr != (SBIC_CSR_MIS_2 | MESG_OUT_PHASE) &&
		 csr != (SBIC_CSR_MIS_2 | CMD_PHASE) &&
		 csr != SBIC_CSR_SEL_TIMEO);

	/* Anyone at home? */
	if (csr == SBIC_CSR_SEL_TIMEO) {
		xs->error = XS_SELTIMEOUT;
		SBIC_DEBUG(PHASE, ("-- Selection Timeout\n"));
		return 0;
	}

	SBIC_DEBUG(PHASE, ("Selection Complete\n"));

	/* Assume we're now selected */
	GET_SBIC_selid(dev, id);
	if (id != target) {
		/* Something went wrong - wrong target was select */
		printf("wd33c93_selectbus: wrong target selected;"
		    "  WANTED %d GOT %d", target, id);
		return 0;      /* XXX: Need to call nexstate to handle? */
	}

	dev->sc_flags |= SBICF_SELECTED;
	dev->sc_state  = SBIC_CONNECTED;

	/* setup correct sync mode for this target */
	wd33c93_setsync(dev, ti);

	if (ti->flags & T_NODISC && dev->sc_disc == 0)
		SET_SBIC_rselid (dev, 0); /* Not expecting a reselect */
	else
		SET_SBIC_rselid (dev, SBIC_RID_ER);

	/*
	 * We only really need to do anything when the target goes to MSG out
	 * If the device ignored ATN, it's probably old and brain-dead,
	 * but we'll try to support it anyhow.
	 * If it doesn't support message out, it definately doesn't
	 * support synchronous transfers, so no point in even asking...
	 */
	if (csr == (SBIC_CSR_MIS_2 | MESG_OUT_PHASE)) {
		if (ti->flags & T_NEGOTIATE) {
			/* Inititae a SDTR message */
			SBIC_DEBUG(SYNC, ("Sending SDTR to target %d\n", id));
			ti->period = dev->sc_minsync;
			ti->offset = dev->sc_maxoffset;

			/* Send Sync negotiation message */
			dev->sc_omsg[0] = MSG_IDENTIFY(lun, 0); /* No Disc */
			dev->sc_omsg[1] = MSG_EXTENDED;
			dev->sc_omsg[2] = MSG_EXT_SDTR_LEN;
			dev->sc_omsg[3] = MSG_EXT_SDTR;
			dev->sc_omsg[4] = dev->sc_minsync;
			dev->sc_omsg[5] = dev->sc_maxoffset;
			wd33c93_xfout(dev, 6, dev->sc_omsg);
			dev->sc_msgout |= SEND_SDTR; /* may be rejected */
			dev->sc_flags  |= SBICF_SYNCNEGO;
		} else {
			if (dev->sc_nexus->tag_type != 0) {
				/* Use TAGS */
				SBIC_DEBUG(TAGS, ("<select %d:%d TAG=%x>\n",
					       dev->target, dev->lun,
					       dev->sc_nexus->tag_id));
				dev->sc_omsg[0] = MSG_IDENTIFY(lun, 1);
				dev->sc_omsg[1] = dev->sc_nexus->tag_type;
				dev->sc_omsg[2] = dev->sc_nexus->tag_id;
				wd33c93_xfout(dev, 3, dev->sc_omsg);
				dev->sc_msgout |= SEND_TAG;
			} else {
				int no_disc;

				/* Setup LUN nexus and disconnect privilege */
				no_disc = xs->xs_control & XS_CTL_POLL ||
					  ti->flags & T_NODISC;
				SEND_BYTE(dev, MSG_IDENTIFY(lun, !no_disc));
			}
		}
		/*
		 * There's one interrupt still to come:
		 * the change to CMD phase...
		 */
		SBIC_WAIT(dev, SBIC_ASR_INT , 0);
		GET_SBIC_csr(dev, csr);
	}

	return csr;
}

/*
 * Information Transfer *to* a SCSI Target.
 *
 * Note: Don't expect there to be an interrupt immediately after all
 * the data is transferred out. The WD spec sheet says that the Transfer-
 * Info command for non-MSG_IN phases only completes when the target
 * next asserts 'REQ'. That is, when the SCSI bus changes to a new state.
 *
 * This can have a nasty effect on commands which take a relatively long
 * time to complete, for example a START/STOP unit command may remain in
 * CMD phase until the disk has spun up. Only then will the target change
 * to STATUS phase. This is really only a problem for immediate commands
 * since we don't allow disconnection for them (yet).
 */
int
wd33c93_xfout(dev, len, bp)
	struct wd33c93_softc	*dev;
	int			len;
	void			*bp;
{
	int wait = wd33c93_data_wait;
	u_char asr, *buf = bp;

	QPRINTF(("wd33c93_xfout {%d} %02x %02x %02x %02x %02x "
		    "%02x %02x %02x %02x %02x\n", len, buf[0], buf[1], buf[2],
		    buf[3], buf[4], buf[5], buf[6], buf[7], buf[8], buf[9]));

	/*
	 * sigh.. WD-PROTO strikes again.. sending the command in one go
	 * causes the chip to lock up if talking to certain (misbehaving?)
	 * targets. Anyway, this procedure should work for all targets, but
	 * it's slightly slower due to the overhead
	 */

	SET_SBIC_control(dev, SBIC_CTL_EDI | SBIC_CTL_IDI);
	SBIC_TC_PUT (dev, (unsigned)len);

	WAIT_CIP (dev);
	SET_SBIC_cmd (dev, SBIC_CMD_XFER_INFO);

	/*
	 * Loop for each byte transferred
	 */
	do {
		GET_SBIC_asr (dev, asr);

		if (asr & SBIC_ASR_DBR) {
			if (len) {
				SET_SBIC_data (dev, *buf);
				buf++;
				len--;
			} else {
				SET_SBIC_data (dev, 0);
			}
			wait = wd33c93_data_wait;
		}
	} while (len && (asr & SBIC_ASR_INT) == 0 && wait-- > 0);

	QPRINTF(("wd33c93_xfout done: %d bytes remaining (wait:%d)\n", len, wait));

	/*
	 * Normally, an interrupt will be pending when this routing returns.
	 */
	return(len);
}

/*
 * Information Transfer *from* a Scsi Target
 * returns # bytes left to read
 */
int
wd33c93_xfin(dev, len, bp)
	struct wd33c93_softc	*dev;
	int			len;
	void			*bp;
{
	int     wait = wd33c93_data_wait;
	u_char  *buf = bp;
	u_char  asr;
#ifdef  DEBUG
	u_char  *obp = bp;
#endif
	SET_SBIC_control(dev, SBIC_CTL_EDI | SBIC_CTL_IDI);
	SBIC_TC_PUT (dev, (unsigned)len);

	WAIT_CIP (dev);
	SET_SBIC_cmd (dev, SBIC_CMD_XFER_INFO);

	/*
	 * Loop for each byte transferred
	 */
	do {
		GET_SBIC_asr (dev, asr);

		if (asr & SBIC_ASR_DBR) {
			if (len) {
				GET_SBIC_data (dev, *buf);
				buf++;
				len--;
			} else {
				u_char foo;
				GET_SBIC_data (dev, foo);
			}
			wait = wd33c93_data_wait;
		}

	} while ((asr & SBIC_ASR_INT) == 0 && wait-- > 0);

	QPRINTF(("wd33c93_xfin {%d} %02x %02x %02x %02x %02x %02x "
		    "%02x %02x %02x %02x\n", len, obp[0], obp[1], obp[2],
		    obp[3], obp[4], obp[5], obp[6], obp[7], obp[8], obp[9]));

	SBIC_TC_PUT (dev, 0);

	/*
	 * this leaves with one csr to be read
	 */
	return len;
}


/*
 * Finish SCSI xfer command:  After the completion interrupt from
 * a read/write operation, sequence through the final phases in
 * programmed i/o.
 */
void
wd33c93_xferdone(dev)
	struct wd33c93_softc *dev;
{
	u_char	phase, csr;
	int	s;

	QPRINTF(("{"));
	s = splbio();

	/*
	 * have the wd33c93 complete on its own
	 */
	SBIC_TC_PUT(dev, 0);
	SET_SBIC_cmd_phase(dev, 0x46);
	SET_SBIC_cmd(dev, SBIC_CMD_SEL_ATN_XFER);

	do {
		SBIC_WAIT (dev, SBIC_ASR_INT, 0);
		GET_SBIC_csr (dev, csr);
		QPRINTF(("%02x:", csr));
	} while ((csr != SBIC_CSR_DISC) &&
		 (csr != SBIC_CSR_DISC_1) &&
		 (csr != SBIC_CSR_S_XFERRED));

	dev->sc_flags &= ~SBICF_SELECTED;
	dev->sc_state = SBIC_DISCONNECT;

	GET_SBIC_cmd_phase (dev, phase);
	QPRINTF(("}%02x", phase));

	if (phase == 0x60)
		GET_SBIC_tlun(dev, dev->sc_status);
	else
		wd33c93_error(dev, dev->sc_nexus);

	QPRINTF(("=STS:%02x=\n", dev->sc_status));
	splx(s);
}


int
wd33c93_go(dev, acb)
	struct wd33c93_softc *dev;
	struct wd33c93_acb *acb;
{
	struct scsipi_xfer	*xs = acb->xs;
	int			i, dmaok;
	u_char			csr, asr;

	SBIC_DEBUG(ACBS, ("wd33c93_go(%d:%d)\n", dev->target, dev->lun));

	dev->sc_nexus = acb;

	dev->target = xs->xs_periph->periph_target;
	dev->lun    = xs->xs_periph->periph_lun;

	dev->sc_status = STATUS_UNKNOWN;
	dev->sc_daddr = acb->daddr;
	dev->sc_dleft = acb->dleft;

	dev->sc_msgpriq = dev->sc_msgout = dev->sc_msgoutq = 0;
	dev->sc_flags = 0;

	dmaok = wd33c93_dmaok(dev, xs);

	if (dmaok == 0)
		dev->sc_flags |= SBICF_NODMA;

	SBIC_DEBUG(DMA, ("wd33c93_go dmago:%d(tcnt=%x) dmaok=%dx\n",
		       dev->target, dev->sc_tcnt, dmaok));

	/* select the SCSI bus (it's an error if bus isn't free) */
	if ((csr = wd33c93_selectbus(dev, acb)) == 0)
		return(0); /* Not done: needs to be rescheduled */

	/*
	 * Lets cycle a while then let the interrupt handler take over.
	 */
	GET_SBIC_asr(dev, asr);
	do {
		QPRINTF(("go[0x%x] ", csr));

		/* Handle the new phase */
		i = wd33c93_nextstate(dev, acb, csr, asr);
		WAIT_CIP(dev);		/* XXX */
		if (dev->sc_state == SBIC_CONNECTED) {

			GET_SBIC_asr(dev, asr);

			if (asr & SBIC_ASR_LCI)
				printf("wd33c93_go: LCI asr:%02x csr:%02x\n", asr, csr);

			if (asr & SBIC_ASR_INT)
				GET_SBIC_csr(dev, csr);
		}

	} while (dev->sc_state == SBIC_CONNECTED &&
	    	 asr & (SBIC_ASR_INT|SBIC_ASR_LCI));

	QPRINTF(("> done i=%d stat=%02x\n", i, dev->sc_status));

	if (i == SBIC_STATE_DONE) {
		if (dev->sc_status == STATUS_UNKNOWN) {
			printf("wd33c93_go: done & stat == UNKNOWN\n");
			return 1;  /* Did we really finish that fast? */
		}
	}
	return 0;
}


int
wd33c93_intr(dev)
	struct wd33c93_softc *dev;
{
	u_char	asr, csr;
	int	i;

	/*
	 * pending interrupt?
	 */
	GET_SBIC_asr (dev, asr);
	if ((asr & SBIC_ASR_INT) == 0)
		return(0);

	GET_SBIC_csr(dev, csr);

	do {
		SBIC_DEBUG(INTS, ("intr[csr=0x%x]", csr));

		i = wd33c93_nextstate(dev, dev->sc_nexus, csr, asr);
		WAIT_CIP(dev);		/* XXX */
		if (dev->sc_state == SBIC_CONNECTED) {
			GET_SBIC_asr(dev, asr);

			if (asr & SBIC_ASR_LCI)
				printf("wd33c93_intr: LCI asr:%02x csr:%02x\n",
				    asr, csr);

			if (asr & SBIC_ASR_INT)
				GET_SBIC_csr(dev, csr);
		}
	} while (dev->sc_state == SBIC_CONNECTED &&
	    	 asr & (SBIC_ASR_INT|SBIC_ASR_LCI));

	SBIC_DEBUG(INTS, ("intr done. state=%d, asr=0x%02x\n", i, asr));

	return(1);
}

/*
 * Complete current command using polled I/O.   Used when interrupt driven
 * I/O is not allowed (ie. during boot and shutdown)
 *
 * Polled I/O is very processor intensive
 */
int
wd33c93_poll(dev, acb)
	struct wd33c93_softc *dev;
	struct wd33c93_acb *acb;
{
	u_char			asr, csr=0;
	int			i, count;
	struct scsipi_xfer	*xs = acb->xs;

	SBIC_WAIT(dev, SBIC_ASR_INT, wd33c93_cmd_wait);
	for (count=acb->timeout; count;) {
		GET_SBIC_asr (dev, asr);
		if (asr & SBIC_ASR_LCI)
			printf("wd33c93_poll: LCI; asr:%02x csr:%02x\n",
			    asr, csr);
		if (asr & SBIC_ASR_INT) {
			GET_SBIC_csr(dev, csr);
			dev->sc_flags |= SBICF_NODMA;
			i = wd33c93_nextstate(dev, dev->sc_nexus, csr, asr);
			WAIT_CIP(dev);		/* XXX */
		} else {
			DELAY(1000);
			count--;
		}

		if ((xs->xs_status & XS_STS_DONE) != 0)
			return (0);

		if (dev->sc_state == SBIC_IDLE) {
			SBIC_DEBUG(ACBS, ("[poll: rescheduling] "));
			wd33c93_sched(dev);
		}
	}
	return (1);
}

/*
 * XXX this might be common thing(check with scsipi)
 */
#define IS1BYTEMSG(m)	(((m) != 1 && (m) < 0x20) || (m) & 0x80)
#define IS2BYTEMSG(m)	(((m) & 0xf0) == 0x20)
#define ISEXTMSG(m)	((m) == 1)

static inline int
__verify_msg_format(u_char *p, int len)
{

	if (len == 1 && IS1BYTEMSG(p[0]))
		return 1;
	if (len == 2 && IS2BYTEMSG(p[0]))
		return 1;
	if (len >= 3 && ISEXTMSG(p[0]) &&
	    len == p[1] + 2)
		return 1;
	return 0;
}

/*
 * Handle message_in phase
 */
int
wd33c93_msgin_phase(dev, reselect)
	struct wd33c93_softc *dev;
	int reselect;
{
	int len;
	u_char asr, csr, *msg;

	GET_SBIC_asr(dev, asr);

	SBIC_DEBUG(MSGS, ("wd33c93msgin asr=%02x\n", asr));

	GET_SBIC_selid (dev, csr);
	SET_SBIC_selid (dev, csr | SBIC_SID_FROM_SCSI);

	SBIC_TC_PUT(dev, 0);

	SET_SBIC_control(dev, SBIC_CTL_EDI | SBIC_CTL_IDI);

	msg = dev->sc_imsg;
	len = 0;

	do {
		/* Fetch the next byte of the message */
		RECV_BYTE(dev, *msg++);
		len++;

		/*
		 * get the command completion interrupt, or we
		 * can't send a new command (LCI)
		 */
		SBIC_WAIT(dev, SBIC_ASR_INT, 0);
		GET_SBIC_csr(dev, csr);

		/*
		 * Clear ACK, and wait for the interrupt
		 * for the next byte or phase change
		 */
		SET_SBIC_cmd(dev, SBIC_CMD_CLR_ACK);
		SBIC_WAIT(dev, SBIC_ASR_INT, 0);

		if (__verify_msg_format(dev->sc_imsg, len))
			break; /* Complete message recieved */

		GET_SBIC_csr(dev, csr);
	} while (len < SBIC_MAX_MSGLEN);

	if (__verify_msg_format(dev->sc_imsg, len))
		wd33c93_msgin(dev, dev->sc_imsg, len);

	/* Should still have one CSR to read */
	return SBIC_STATE_RUNNING;
}


void wd33c93_msgin(dev, msgaddr, msglen)
	struct wd33c93_softc *dev;
	u_char *msgaddr;
	int msglen;
{
	struct wd33c93_acb    *acb = dev->sc_nexus;
	struct wd33c93_tinfo  *ti = &dev->sc_tinfo[dev->target];
	struct wd33c93_linfo  *li;
	u_char asr;

	switch (dev->sc_state) {
	case SBIC_CONNECTED:
		switch (msgaddr[0]) {
		case MSG_MESSAGE_REJECT:
			SBIC_DEBUG(MSGS, ("msgin: MSG_REJECT, "
				       "last msgout=%x\n", dev->sc_msgout));
			switch (dev->sc_msgout) {
			case SEND_TAG:
				printf("%s: tagged queuing rejected: "
				    "target %d\n",
				    dev->sc_dev.dv_xname, dev->target);
				ti->flags &= ~T_TAG;
				li = TINFO_LUN(ti, dev->lun);
				if (acb->tag_type &&
				    li->queued[acb->tag_id] != NULL) {
					li->queued[acb->tag_id] = NULL;
					li->used--;
				}
				acb->tag_type = acb->tag_id = 0;
				li->untagged = acb;
				li->state = L_STATE_BUSY;
				break;

			case SEND_SDTR:
				printf("%s: sync transfer rejected: target %d\n",
				    dev->sc_dev.dv_xname, dev->target);

				dev->sc_flags &= ~SBICF_SYNCNEGO;
				ti->flags &= ~(T_NEGOTIATE | T_SYNCMODE);
				wd33c93_update_xfer_mode(dev,
				    acb->xs->xs_periph->periph_target);
				wd33c93_setsync(dev, ti);

			case SEND_INIT_DET_ERR:
				goto abort;

			default:
				SBIC_DEBUG(MSGS, ("Unexpected MSG_REJECT\n"));
				break;
			}
			dev->sc_msgout = 0;
			break;

		case MSG_HEAD_OF_Q_TAG:
		case MSG_ORDERED_Q_TAG:
		case MSG_SIMPLE_Q_TAG:
			printf("-- Out of phase TAG;"
			    "Nexus=%d:%d Tag=%02x/%02x\n",
			    dev->target, dev->lun, msgaddr[0], msgaddr[1]);
			break;

		case MSG_DISCONNECT:
			SBIC_DEBUG(MSGS, ("msgin: DISCONNECT"));
			/*
			 * Mark the fact that all bytes have moved. The
			 * target may not bother to do a SAVE POINTERS
			 * at this stage. This flag will set the residual
			 * count to zero on MSG COMPLETE.
			 */
			if (dev->sc_dleft == 0)
				acb->flags |= ACB_COMPLETE;

			if (acb->xs->xs_control & XS_CTL_POLL)
				/* Don't allow disconnect in immediate mode */
				goto reject;
			else {  /* Allow disconnect */
				dev->sc_flags &= ~SBICF_SELECTED;
				dev->sc_state = SBIC_DISCONNECT;
			}
			if ((acb->xs->xs_periph->periph_quirks &
				PQUIRK_AUTOSAVE) == 0)
				break;
			/*FALLTHROUGH*/

		case MSG_SAVEDATAPOINTER:
			SBIC_DEBUG(MSGS, ("msgin: SAVEDATAPTR"));
			acb->daddr = dev->sc_daddr;
			acb->dleft = dev->sc_dleft;
			break;

		case MSG_RESTOREPOINTERS:
			SBIC_DEBUG(MSGS, ("msgin: RESTOREPTR"));
			dev->sc_daddr = acb->daddr;
			dev->sc_dleft = acb->dleft;
			break;

		case MSG_CMDCOMPLETE:
			/*
			 * !! KLUDGE ALERT !! quite a few drives don't seem to
			 * really like the current way of sending the
			 * sync-handshake together with the ident-message, and
			 * they react by sending command-complete and
			 * disconnecting right after returning the valid sync
			 * handshake. So, all I can do is reselect the drive,
			 * and hope it won't disconnect again. I don't think
			 * this is valid behavior, but I can't help fixing a
			 * problem that apparently exists.
			 *
			 * Note: we should not get here on `normal' command
			 * completion, as that condition is handled by the
			 * high-level sel&xfer resume command used to walk
			 * thru status/cc-phase.
			 */
			SBIC_DEBUG(MSGS, ("msgin: CMD_COMPLETE"));
			SBIC_DEBUG(SYNC, ("GOT MSG %d! target %d"
				       " acting weird.."
				       " waiting for disconnect...\n",
				       msgaddr[0], dev->target));

			/* Check to see if wd33c93 is handling this */
			GET_SBIC_asr(dev, asr);
			if (asr & SBIC_ASR_BSY)
				break;

			/* XXX: Assume it works and set status to 00 */
			dev->sc_status = 0;
			dev->sc_state = SBIC_CMDCOMPLETE;
			break;

		case MSG_EXTENDED:
			switch(msgaddr[2]) {
			case MSG_EXT_SDTR: /* Sync negotiation */
				SBIC_DEBUG(MSGS, ("msgin: EXT_SDTR; "
					       "period %d, offset %d",
					       msgaddr[3], msgaddr[4]));
				if (msgaddr[1] != 3)
					goto reject;

				ti->period = MAX(msgaddr[3], dev->sc_minsync);
				ti->offset = MIN(msgaddr[4], dev->sc_maxoffset);
				ti->flags &= ~T_NEGOTIATE;
				if (dev->sc_minsync == 0 || ti->period > 124)
					ti->offset = ti->period = 0;

				if (ti->offset == 0)
					ti->flags &= ~T_SYNCMODE; /* Async */
				else {
					int p;

					p = wd33c93_stp2div(dev, ti->period);
					ti->period = wd33c93_div2stp(dev, p);
					ti->flags |= T_SYNCMODE; /* Sync */
				}

				if ((dev->sc_flags&SBICF_SYNCNEGO) == 0)
					/* target initiated negotiation */
					wd33c93_sched_msgout(dev, SEND_SDTR);
				dev->sc_flags &= ~SBICF_SYNCNEGO;

				SBIC_DEBUG(SYNC, ("msgin(%d): SDTR(o=%d,p=%d)",
					       dev->target, ti->offset,
					       ti->period));
				wd33c93_update_xfer_mode(dev,
				    acb->xs->xs_periph->periph_target);
				wd33c93_setsync(dev, ti);
				break;

			case MSG_EXT_WDTR:
				SBIC_DEBUG(MSGS, ("msgin: EXT_WDTR ignored"));
				break;

			default:
				scsipi_printaddr(acb->xs->xs_periph);
				printf("unrecognized MESSAGE EXTENDED;"
				    " sending REJECT\n");
				goto reject;
			}
			break;

		default:
			scsipi_printaddr(acb->xs->xs_periph);
			printf("unrecognized MESSAGE; sending REJECT\n");

		reject:
			/* We don't support whatever this message is... */
			wd33c93_sched_msgout(dev, SEND_REJECT);
			break;
		}
		break;

	case SBIC_IDENTIFIED:
		/*
		 * IDENTIFY message was received and queue tag is expected now
		 */
		if ((msgaddr[0]!=MSG_SIMPLE_Q_TAG) || (dev->sc_msgify==0)) {
			printf("%s: TAG reselect without IDENTIFY;"
			    " MSG %x; sending DEVICE RESET\n",
			    dev->sc_dev.dv_xname, msgaddr[0]);
			goto reset;
		}
		SBIC_DEBUG(TAGS, ("TAG %x/%x\n", msgaddr[0], msgaddr[1]));
		if (dev->sc_nexus)
			printf("*TAG Recv with active nexus!!\n");
		wd33c93_reselect(dev, dev->target, dev->lun,
		    	      msgaddr[0], msgaddr[1]);
		break;

	case SBIC_RESELECTED:
		/*
		 * IDENTIFY message with target
		 */
		if (MSG_ISIDENTIFY(msgaddr[0])) {
			SBIC_DEBUG(PHASE, ("IFFY[%x] ", msgaddr[0]));
			dev->sc_msgify = msgaddr[0];
		} else {
			printf("%s: reselect without IDENTIFY;"
			    " MSG %x;"
			    " sending DEVICE RESET\n",
			    dev->sc_dev.dv_xname, msgaddr[0]);
			goto reset;
		}
		break;

	default:
		printf("Unexpected MESSAGE IN.  State=%d - Sending RESET\n",
		    dev->sc_state);
	reset:
		wd33c93_sched_msgout(dev, SEND_DEV_RESET);
		break;
	abort:
		wd33c93_sched_msgout(dev, SEND_ABORT);
		break;
	}
}

void
wd33c93_sched_msgout(dev, msg)
	struct wd33c93_softc *dev;
	u_short msg;
{
	u_char	asr;

	SBIC_DEBUG(SYNC,("sched_msgout: %04x\n", msg));
	dev->sc_msgpriq |= msg;

	/* Schedule MSGOUT Phase to send message */

	WAIT_CIP(dev);
	SET_SBIC_cmd(dev, SBIC_CMD_SET_ATN);
	WAIT_CIP(dev);
	GET_SBIC_asr(dev, asr);
	if (asr & SBIC_ASR_LCI) {
		printf("MSGOUT Failed!\n");
	}
	SET_SBIC_cmd(dev, SBIC_CMD_CLR_ACK);
	WAIT_CIP(dev);
}

/*
 * Send the highest priority, scheduled message
 */
void
wd33c93_msgout(dev)
	struct wd33c93_softc *dev;
{
	struct wd33c93_tinfo *ti;
	struct wd33c93_acb *acb = dev->sc_nexus;

	if (acb == NULL)
		panic("MSGOUT with no nexus");

	if (dev->sc_omsglen == 0) {
		/* Pick up highest priority message */
		dev->sc_msgout   = dev->sc_msgpriq & -dev->sc_msgpriq;
		dev->sc_msgoutq |= dev->sc_msgout;
		dev->sc_msgpriq &= ~dev->sc_msgout;
		dev->sc_omsglen = 1;		/* "Default" message len */
		switch (dev->sc_msgout) {
		case SEND_SDTR:
			ti = &dev->sc_tinfo[acb->xs->xs_periph->periph_target];
			dev->sc_omsg[0] = MSG_EXTENDED;
			dev->sc_omsg[1] = MSG_EXT_SDTR_LEN;
			dev->sc_omsg[2] = MSG_EXT_SDTR;
			dev->sc_omsg[3] = ti->period;
			dev->sc_omsg[4] = ti->offset;
			dev->sc_omsglen = 5;
			if ((dev->sc_flags & SBICF_SYNCNEGO) == 0) {
				ti->flags |= T_SYNCMODE;
				wd33c93_setsync(dev, ti);
			}
			break;
		case SEND_IDENTIFY:
			if (dev->sc_state != SBIC_CONNECTED) {
				printf("%s at line %d: no nexus\n",
				    dev->sc_dev.dv_xname, __LINE__);
			}
			dev->sc_omsg[0] =
			    MSG_IDENTIFY(acb->xs->xs_periph->periph_lun, 0);
			break;
		case SEND_TAG:
			if (dev->sc_state != SBIC_CONNECTED) {
				printf("%s at line %d: no nexus\n",
				    dev->sc_dev.dv_xname, __LINE__);
			}
			dev->sc_omsg[0] = acb->tag_type;
			dev->sc_omsg[1] = acb->tag_id;
			dev->sc_omsglen = 2;
			break;
		case SEND_DEV_RESET:
			dev->sc_omsg[0] = MSG_BUS_DEV_RESET;
			ti = &dev->sc_tinfo[dev->target];
			ti->flags &= ~T_SYNCMODE;
			wd33c93_update_xfer_mode(dev, dev->target);
			if ((ti->flags & T_NOSYNC) == 0)
				/* We can re-start sync negotiation */
				ti->flags |= T_NEGOTIATE;
			break;
		case SEND_PARITY_ERROR:
			dev->sc_omsg[0] = MSG_PARITY_ERROR;
			break;
		case SEND_ABORT:
			dev->sc_flags  |= SBICF_ABORTING;
			dev->sc_omsg[0] = MSG_ABORT;
			break;
		case SEND_INIT_DET_ERR:
			dev->sc_omsg[0] = MSG_INITIATOR_DET_ERR;
			break;
		case SEND_REJECT:
			dev->sc_omsg[0] = MSG_MESSAGE_REJECT;
			break;
		default:
			/* Wasn't expecting MSGOUT Phase */
			dev->sc_omsg[0] = MSG_NOOP;
			break;
		}
	}

	wd33c93_xfout(dev, dev->sc_omsglen, dev->sc_omsg);
}


/*
 * wd33c93_nextstate()
 * return:
 *	SBIC_STATE_DONE		== done
 *	SBIC_STATE_RUNNING	== working
 *	SBIC_STATE_DISCONNECT	== disconnected
 *	SBIC_STATE_ERROR	== error
 */
int
wd33c93_nextstate(dev, acb, csr, asr)
	struct wd33c93_softc	*dev;
	struct wd33c93_acb	*acb;
	u_char			csr, asr;
{
	SBIC_DEBUG(PHASE, ("next[a=%02x,c=%02x]: ",asr,csr));

	switch (csr) {

	case SBIC_CSR_XFERRED | CMD_PHASE:
	case SBIC_CSR_MIS     | CMD_PHASE:
	case SBIC_CSR_MIS_1   | CMD_PHASE:
	case SBIC_CSR_MIS_2   | CMD_PHASE:

		if (wd33c93_xfout(dev, acb->clen, &acb->cmd))
			goto abort;
		break;

	case SBIC_CSR_XFERRED | STATUS_PHASE:
	case SBIC_CSR_MIS     | STATUS_PHASE:
	case SBIC_CSR_MIS_1   | STATUS_PHASE:
	case SBIC_CSR_MIS_2   | STATUS_PHASE:

		SET_SBIC_control(dev, SBIC_CTL_EDI | SBIC_CTL_IDI);

		/*
		 * this should be the normal i/o completion case.
		 * get the status & cmd complete msg then let the
		 * device driver look at what happened.
		 */
		wd33c93_xferdone(dev);

		wd33c93_dma_stop(dev);

		/* Fixup byte count to be passed to higher layer */
		acb->dleft = (acb->flags & ACB_COMPLETE) ? 0 :
		    	      dev->sc_dleft;

		/*
		 * Indicate to the upper layers that the command is done
		 */
		wd33c93_scsidone(dev, acb, dev->sc_status);

		return SBIC_STATE_DONE;


	case SBIC_CSR_XFERRED | DATA_IN_PHASE:
	case SBIC_CSR_MIS     | DATA_IN_PHASE:
	case SBIC_CSR_MIS_1   | DATA_IN_PHASE:
	case SBIC_CSR_MIS_2   | DATA_IN_PHASE:
	case SBIC_CSR_XFERRED | DATA_OUT_PHASE:
	case SBIC_CSR_MIS     | DATA_OUT_PHASE:
	case SBIC_CSR_MIS_1   | DATA_OUT_PHASE:
	case SBIC_CSR_MIS_2   | DATA_OUT_PHASE:
		/*
		 * Verify that we expected to transfer data...
		 */
		if (acb->dleft <= 0) {
			printf("next: DATA phase with xfer count == %d, asr:0x%02x csr:0x%02x\n",
			    acb->dleft, asr, csr);
			goto abort;
		}

		/*
		 * Should we transfer using PIO or DMA ?
		 */
		if (acb->xs->xs_control & XS_CTL_POLL ||
		    dev->sc_flags & SBICF_NODMA) {
			/* Perfrom transfer using PIO */
			int resid;

			SBIC_DEBUG(DMA, ("PIO xfer: %d(%p:%x)\n", dev->target,
				       dev->sc_daddr, dev->sc_dleft));

			if (SBIC_PHASE(csr) == DATA_IN_PHASE)
				/* data in */
				resid = wd33c93_xfin(dev, dev->sc_dleft,
				    		 dev->sc_daddr);
			else	/* data out */
				resid = wd33c93_xfout(dev, dev->sc_dleft,
				    		  dev->sc_daddr);

			dev->sc_daddr += (acb->dleft - resid);
			dev->sc_dleft = resid;
		} else {
			int datain = SBIC_PHASE(csr) == DATA_IN_PHASE;

			/* Perform transfer using DMA */
			wd33c93_dma_setup(dev, datain);

			SET_SBIC_control(dev, SBIC_CTL_EDI | SBIC_CTL_IDI |
			    SBIC_CTL_DMA);

			SBIC_DEBUG(DMA, ("DMA xfer: %d(%p:%x)\n", dev->target,
				       dev->sc_daddr, dev->sc_dleft));

			/* Setup byte count for transfer */
			SBIC_TC_PUT(dev, (unsigned)dev->sc_dleft);

			/* Start the transfer */
			SET_SBIC_cmd(dev, SBIC_CMD_XFER_INFO);

			/* Start the DMA chip going */
			dev->sc_tcnt = dev->sc_dmago(dev);

			/* Indicate that we're in DMA mode */
			dev->sc_flags |= SBICF_INDMA;
		}
		break;

	case SBIC_CSR_XFERRED | MESG_IN_PHASE:
	case SBIC_CSR_MIS     | MESG_IN_PHASE:
	case SBIC_CSR_MIS_1   | MESG_IN_PHASE:
	case SBIC_CSR_MIS_2   | MESG_IN_PHASE:

		wd33c93_dma_stop(dev);

		/* Handle a single message in... */
		return wd33c93_msgin_phase(dev, 0);

	case SBIC_CSR_MSGIN_W_ACK:

		/*
		 * We should never see this since it's handled in
		 * 'wd33c93_msgin_phase()' but just for the sake of paranoia...
		 */
		SET_SBIC_cmd(dev, SBIC_CMD_CLR_ACK);

		printf("Acking unknown msgin CSR:%02x",csr);
		break;

	case SBIC_CSR_XFERRED | MESG_OUT_PHASE:
	case SBIC_CSR_MIS     | MESG_OUT_PHASE:
	case SBIC_CSR_MIS_1   | MESG_OUT_PHASE:
	case SBIC_CSR_MIS_2   | MESG_OUT_PHASE:

		/*
		 * Message out phase.  ATN signal has been asserted
		 */
		wd33c93_dma_stop(dev);
		wd33c93_msgout(dev);
		return SBIC_STATE_RUNNING;

	case SBIC_CSR_DISC:
	case SBIC_CSR_DISC_1:
		SBIC_DEBUG(RSEL, ("wd33c93next target %d disconnected\n",
			       dev->target));
		wd33c93_dma_stop(dev);

		dev->sc_nexus = NULL;
		dev->sc_state = SBIC_IDLE;
		dev->sc_flags = 0;

		++dev->sc_tinfo[dev->target].dconns;
		++dev->sc_disc;

		if (acb->xs->xs_control & XS_CTL_POLL || wd33c93_nodisc)
			return SBIC_STATE_DISCONNECT;

		/* Try to schedule another target */
		wd33c93_sched(dev);

		return SBIC_STATE_DISCONNECT;

	case SBIC_CSR_RSLT_NI:
	case SBIC_CSR_RSLT_IFY:
	{
		/*
		 * A reselection.
		 * Note that since we don't enable Advanced Features (assuming
		 * the WD chip is at least the 'A' revision), we're only ever
		 * likely to see the 'SBIC_CSR_RSLT_NI' status. But for the
		 * hell of it, we'll handle it anyway, for all the extra code
		 * it needs...
		 */
		u_char  newtarget, newlun;

		if (dev->sc_flags & SBICF_INDMA) {
			printf("**** RESELECT WHILE DMA ACTIVE!!! ***\n");
			wd33c93_dma_stop(dev);
		}

		dev->sc_state = SBIC_RESELECTED;
		GET_SBIC_rselid(dev, newtarget);

		/* check SBIC_RID_SIV? */
		newtarget &= SBIC_RID_MASK;

		if (csr == SBIC_CSR_RSLT_IFY) {
			/* Read Identify msg to avoid lockup */
			GET_SBIC_data(dev, newlun);
			WAIT_CIP(dev);
			newlun &= SBIC_TLUN_MASK;
			dev->sc_msgify = MSG_IDENTIFY(newlun, 0);
		} else {
			/*
			 * Need to read Identify message the hard way, assuming
			 * the target even sends us one...
			 */
			for (newlun = 255; newlun; --newlun) {
				GET_SBIC_asr(dev, asr);
				if (asr & SBIC_ASR_INT)
					break;
				DELAY(10);
			}

			/* If we didn't get an interrupt, somethink's up */
			if ((asr & SBIC_ASR_INT) == 0) {
				printf("%s: Reselect without identify? asr %x\n",
				    dev->sc_dev.dv_xname, asr);
				newlun = 0; /* XXXX */
			} else {
				/*
				 * We got an interrupt, verify that it's a
				 * change to message in phase, and if so
				 * read the message.
				 */
				GET_SBIC_csr(dev,csr);

				if (csr == (SBIC_CSR_MIS   | MESG_IN_PHASE) ||
				    csr == (SBIC_CSR_MIS_1 | MESG_IN_PHASE) ||
				    csr == (SBIC_CSR_MIS_2 | MESG_IN_PHASE)) {
					/*
					 * Yup, gone to message in.
					 * Fetch the target LUN
					 */
					dev->sc_msgify = 0;
					wd33c93_msgin_phase(dev, 1);
					newlun = dev->sc_msgify & SBIC_TLUN_MASK;
				} else {
					/*
					 * Whoops! Target didn't go to msg_in
					 * phase!!
					 */
					printf("RSLT_NI - not MESG_IN_PHASE %x\n", csr);
					newlun = 0; /* XXXSCW */
				}
			}
		}

		/* Ok, we have the identity of the reselecting target. */
		SBIC_DEBUG(RSEL, ("wd33c93next: reselect from targ %d lun %d",
			       newtarget, newlun));
		wd33c93_reselect(dev, newtarget, newlun, 0, 0);
		dev->sc_disc--;

		if (csr == SBIC_CSR_RSLT_IFY)
			SET_SBIC_cmd(dev, SBIC_CMD_CLR_ACK);
		break;
	}

	default:
	abort:
		/* Something unexpected happend -- deal with it. */
		printf("next: aborting asr 0x%02x csr 0x%02x\n", asr, csr);

#ifdef DDB
		Debugger();
#endif

		SET_SBIC_control(dev, SBIC_CTL_EDI | SBIC_CTL_IDI);
		if (acb->xs)
			wd33c93_error(dev, acb);
		wd33c93_abort(dev, acb, "next");

		if (dev->sc_flags & SBICF_INDMA) {
			wd33c93_dma_stop(dev);
			wd33c93_scsidone(dev, acb, STATUS_UNKNOWN);
		}
		return SBIC_STATE_ERROR;
	}
	return SBIC_STATE_RUNNING;
}


void
wd33c93_reselect(dev, target, lun, tag_type, tag_id)
	struct wd33c93_softc *dev;
	int target;
	int lun;
	int tag_type;
	int tag_id;
{

	struct wd33c93_tinfo *ti;
	struct wd33c93_linfo *li;
	struct wd33c93_acb *acb;

	if (dev->sc_nexus) {
		/*
		 * Whoops! We've been reselected with a
		 * command in progress!
		 * The best we can do is to put the current
		 * command back on the ready list and hope
		 * for the best.
		 */
		SBIC_DEBUG(RSEL, ("%s: reselect with active command\n",
			       dev->sc_dev.dv_xname));
		ti = &dev->sc_tinfo[dev->target];
		li = TINFO_LUN(ti, dev->lun);
		li->state = L_STATE_IDLE;

		wd33c93_dequeue(dev, dev->sc_nexus);
		TAILQ_INSERT_HEAD(&dev->ready_list, dev->sc_nexus, chain);
		dev->sc_nexus->flags |= ACB_READY;

		dev->sc_nexus = NULL;
	}

	/* Setup state for new nexus */
	acb = NULL;
	dev->sc_flags = SBICF_SELECTED;
	dev->sc_msgpriq = dev->sc_msgout = dev->sc_msgoutq = 0;

	ti = &dev->sc_tinfo[target];
	li = TINFO_LUN(ti, lun);

	if (li != NULL) {
		if (li->untagged != NULL && li->state)
			acb = li->untagged;
		else if (tag_type != MSG_SIMPLE_Q_TAG) {
			/* Wait for tag to come by during MESG_IN Phase */
			dev->target    = target; /* setup I_T_L nexus */
			dev->lun       = lun;
			dev->sc_state  = SBIC_IDENTIFIED;
			return;
		} else if (tag_type)
			acb = li->queued[tag_id];
	}

	if (acb == NULL) {
		printf("%s: reselect from target %d lun %d tag %x:%x "
		    "with no nexus; sending ABORT\n",
		    dev->sc_dev.dv_xname, target, lun, tag_type, tag_id);
		goto abort;
	}

	dev->target    = target;
	dev->lun       = lun;
	dev->sc_nexus  = acb;
	dev->sc_state  = SBIC_CONNECTED;

	/* Do an implicit RESTORE POINTERS. */
	dev->sc_daddr = acb->daddr;
	dev->sc_dleft = acb->dleft;

	/* Set sync modes for new target */
	wd33c93_setsync(dev, ti);

	if (acb->flags & ACB_RESET)
		wd33c93_sched_msgout(dev, SEND_DEV_RESET);
	else if (acb->flags & ACB_ABORT)
		wd33c93_sched_msgout(dev, SEND_ABORT);
	return;

abort:
	wd33c93_sched_msgout(dev, SEND_ABORT);
	return;

}

void
wd33c93_update_xfer_mode(sc, target)
	struct wd33c93_softc *sc;
	int target;
{
	struct wd33c93_tinfo *ti = &sc->sc_tinfo[target];
	struct scsipi_xfer_mode xm;

	xm.xm_target = target;
	xm.xm_mode = 0;
	xm.xm_period = 0;
	xm.xm_offset = 0;

	if (ti->flags & T_SYNCMODE) {
		xm.xm_mode |= PERIPH_CAP_SYNC;
		xm.xm_period = ti->period;
		xm.xm_offset = ti->offset;
	}

	if ((ti->flags & (T_NODISC|T_TAG)) == T_TAG)
		xm.xm_mode |= PERIPH_CAP_TQING;

	scsipi_async_event(&sc->sc_channel, ASYNC_EVENT_XFER_MODE, &xm);
}


/*
 * Convert SCSI Transfer Period Factor (in 4ns units) to the divisor
 * value used by the WD33c93 controller.
 *
 * cycle = DIV / (2 * CLK)
 * DIV = FS + 2
 * best we can do is 200ns at 20Mhz, 2 cycles
 */
int
wd33c93_div2stp(dev, div)
	struct wd33c93_softc *dev;
	int div;
{
	unsigned int fs;

	GET_SBIC_myid(dev, fs);
	fs = (fs >> 6) + 2;		/* DIV */
	fs = (fs * 10000) / (dev->sc_clkfreq << 1); /* Cycle, in ns */
	if (div < 2)
		div = 8;		/* map to Cycles */
	return ((fs * div) >> 2);	/* in 4 ns units */
}

/*
 * Calculate SCSI Tranfser Period Factor (4ns units each) from the
 * WD33c93 divisor value
 */
int
wd33c93_stp2div(dev, stp)
	struct wd33c93_softc *dev;
	int stp;
{
	unsigned fs, div;

	/* Just the inverse of the above */
	GET_SBIC_myid(dev, fs);
	fs = (fs >> 6) + 2;	/* DIV */
	fs = (fs * 10000) / (dev->sc_clkfreq << 1); /* Cycle, in ns */
	div = stp << 2;			/* in ns units */
	div = div / fs;			/* in Cycles */
	if (div < 2)
		return(2);

	/* verify rounding */
	if (wd33c93_div2stp(dev, div) < stp)
		div++;

	return((div >= 8) ? 0 : div);
}

void
wd33c93_timeout(arg)
	void *arg;
{
	struct wd33c93_acb *acb = arg;
	struct scsipi_xfer *xs = acb->xs;
	struct scsipi_periph *periph = xs->xs_periph;
	struct wd33c93_softc *dev =
	    (void *)periph->periph_channel->chan_adapter->adapt_dev;
	int s, asr;

	s = splbio();

	GET_SBIC_asr(dev, asr);

	scsipi_printaddr(periph);
	printf("%s: timed out; asr=0x%02x [acb %p (flags 0x%x, dleft %x)], "
	    "<state %d, nexus %p, resid %lx, msg(q %x,o %x)>",
	    dev->sc_dev.dv_xname, asr, acb, acb->flags, acb->dleft,
	    dev->sc_state, dev->sc_nexus, (long)dev->sc_dleft,
	    dev->sc_msgpriq, dev->sc_msgout);

	if (asr & SBIC_ASR_INT) {
		/* We need to service a missed IRQ */
		wd33c93_intr(dev);
	} else {
		(void) wd33c93_abort(dev, dev->sc_nexus, "timeout");
	}
	splx(s);
}


void
wd33c93_watchdog(arg)
	void *arg;
{
	struct wd33c93_softc *dev = arg;
	struct wd33c93_tinfo *ti;
	struct wd33c93_linfo *li;
	int t, s, l;
	/* scrub LUN's that have not been used in the last 10min. */
	time_t old = time.tv_sec - (10 * 60);

	for (t = 0; t < SBIC_NTARG; t++) {
		ti = &dev->sc_tinfo[t];
		for (l = 0; l < SBIC_NLUN; l++) {
			s = splbio();
			li = TINFO_LUN(ti, l);
			if (li && li->last_used < old &&
			    li->untagged == NULL && li->used == 0) {
				ti->lun[li->lun] = NULL;
				free(li, M_DEVBUF);
			}
			splx(s);
		}
	}
	callout_reset(&dev->sc_watchdog, 60 * hz, wd33c93_watchdog, dev);
}


#ifdef DEBUG
void
wd33c93_hexdump(buf, len)
	u_char *buf;
	int len;
{
	printf("{%d}:", len);
	while (len--)
		printf(" %02x", *buf++);
	printf("\n");
}


void
wd33c93_print_csr(csr)
	u_char csr;
{
	switch (SCSI_PHASE(csr)) {
	case CMD_PHASE:
		printf("CMD_PHASE\n");
		break;

	case STATUS_PHASE:
		printf("STATUS_PHASE\n");
		break;

	case DATA_IN_PHASE:
		printf("DATAIN_PHASE\n");
		break;

	case DATA_OUT_PHASE:
		printf("DATAOUT_PHASE\n");
		break;

	case MESG_IN_PHASE:
		printf("MESG_IN_PHASE\n");
		break;

	case MESG_OUT_PHASE:
		printf("MESG_OUT_PHASE\n");
		break;

	default:
		switch (csr) {
		case SBIC_CSR_DISC_1:
			printf("DISC_1\n");
			break;

		case SBIC_CSR_RSLT_NI:
			printf("RESELECT_NO_IFY\n");
			break;

		case SBIC_CSR_RSLT_IFY:
			printf("RESELECT_IFY\n");
			break;

		case SBIC_CSR_SLT:
			printf("SELECT\n");
			break;

		case SBIC_CSR_SLT_ATN:
			printf("SELECT, ATN\n");
			break;

		case SBIC_CSR_UNK_GROUP:
			printf("UNK_GROUP\n");
			break;

		default:
			printf("UNKNOWN csr=%02x\n", csr);
		}
	}
}
#endif
