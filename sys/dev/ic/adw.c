/* $NetBSD: adw.c,v 1.12.2.3 1999/10/20 20:40:51 thorpej Exp $	 */

/*
 * Generic driver for the Advanced Systems Inc. SCSI controllers
 *
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * Author: Baldassare Dante Profeta <dante@mclink.it>
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

#include <machine/bus.h>
#include <machine/intr.h>

#include <vm/vm.h>
#include <vm/vm_param.h>
#include <vm/pmap.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#include <dev/ic/adwlib.h>
#include <dev/ic/adw.h>

#ifndef DDB
#define	Debugger()	panic("should call debugger here (adw.c)")
#endif				/* ! DDB */

/******************************************************************************/


static int adw_alloc_ccbs __P((ADW_SOFTC *));
static int adw_create_ccbs __P((ADW_SOFTC *, ADW_CCB *, int));
static void adw_free_ccb __P((ADW_SOFTC *, ADW_CCB *));
static void adw_reset_ccb __P((ADW_CCB *));
static int adw_init_ccb __P((ADW_SOFTC *, ADW_CCB *));
static ADW_CCB *adw_get_ccb __P((ADW_SOFTC *));
static void adw_queue_ccb __P((ADW_SOFTC *, ADW_CCB *));
static void adw_start_ccbs __P((ADW_SOFTC *));

static void adw_scsipi_request __P((struct scsipi_channel *,
	scsipi_adapter_req_t, void *));
static int adw_build_req __P((ADW_SOFTC *, ADW_CCB *));
static void adw_build_sglist __P((ADW_CCB *, ADW_SCSI_REQ_Q *, ADW_SG_BLOCK *));
static void adwminphys __P((struct buf *));
static void adw_wide_isr_callback __P((ADW_SOFTC *, ADW_SCSI_REQ_Q *));
static void adw_sbreset_callback __P((ADW_SOFTC *));

static int adw_poll __P((ADW_SOFTC *, struct scsipi_xfer *, int));
static void adw_timeout __P((void *));


/******************************************************************************/

#define ADW_ABORT_TIMEOUT       10000	/* time to wait for abort (mSec) */
#define ADW_WATCH_TIMEOUT       10000	/* time to wait for watchdog (mSec) */

/******************************************************************************/
/*                                Control Blocks routines                     */
/******************************************************************************/


static int
adw_alloc_ccbs(sc)
	ADW_SOFTC      *sc;
{
	bus_dma_segment_t seg;
	int             error, rseg;

	/*
         * Allocate the control blocks.
         */
	if ((error = bus_dmamem_alloc(sc->sc_dmat, sizeof(struct adw_control),
			   NBPG, 0, &seg, 1, &rseg, BUS_DMA_NOWAIT)) != 0) {
		printf("%s: unable to allocate control structures,"
		       " error = %d\n", sc->sc_dev.dv_xname, error);
		return (error);
	}
	if ((error = bus_dmamem_map(sc->sc_dmat, &seg, rseg,
		   sizeof(struct adw_control), (caddr_t *) & sc->sc_control,
				 BUS_DMA_NOWAIT | BUS_DMA_COHERENT)) != 0) {
		printf("%s: unable to map control structures, error = %d\n",
		       sc->sc_dev.dv_xname, error);
		return (error);
	}
	/*
         * Create and load the DMA map used for the control blocks.
         */
	if ((error = bus_dmamap_create(sc->sc_dmat, sizeof(struct adw_control),
			   1, sizeof(struct adw_control), 0, BUS_DMA_NOWAIT,
				       &sc->sc_dmamap_control)) != 0) {
		printf("%s: unable to create control DMA map, error = %d\n",
		       sc->sc_dev.dv_xname, error);
		return (error);
	}
	if ((error = bus_dmamap_load(sc->sc_dmat, sc->sc_dmamap_control,
			   sc->sc_control, sizeof(struct adw_control), NULL,
				     BUS_DMA_NOWAIT)) != 0) {
		printf("%s: unable to load control DMA map, error = %d\n",
		       sc->sc_dev.dv_xname, error);
		return (error);
	}
	return (0);
}


/*
 * Create a set of ccbs and add them to the free list.  Called once
 * by adw_init().  We return the number of CCBs successfully created.
 */
static int
adw_create_ccbs(sc, ccbstore, count)
	ADW_SOFTC      *sc;
	ADW_CCB        *ccbstore;
	int             count;
{
	ADW_CCB        *ccb;
	int             i, error;

	bzero(ccbstore, sizeof(ADW_CCB) * count);
	for (i = 0; i < count; i++) {
		ccb = &ccbstore[i];
		if ((error = adw_init_ccb(sc, ccb)) != 0) {
			printf("%s: unable to initialize ccb, error = %d\n",
			       sc->sc_dev.dv_xname, error);
			return (i);
		}
		TAILQ_INSERT_TAIL(&sc->sc_free_ccb, ccb, chain);
	}

	return (i);
}


/*
 * A ccb is put onto the free list.
 */
static void
adw_free_ccb(sc, ccb)
	ADW_SOFTC      *sc;
	ADW_CCB        *ccb;
{
	int             s;

	s = splbio();
	adw_reset_ccb(ccb);
	TAILQ_INSERT_HEAD(&sc->sc_free_ccb, ccb, chain);
	splx(s);
}


static void
adw_reset_ccb(ccb)
	ADW_CCB        *ccb;
{

	ccb->flags = 0;
}


static int
adw_init_ccb(sc, ccb)
	ADW_SOFTC      *sc;
	ADW_CCB        *ccb;
{
	int	hashnum, error;

	/*
         * Create the DMA map for this CCB.
         */
	error = bus_dmamap_create(sc->sc_dmat,
				  (ADW_MAX_SG_LIST - 1) * PAGE_SIZE,
			 ADW_MAX_SG_LIST, (ADW_MAX_SG_LIST - 1) * PAGE_SIZE,
		   0, BUS_DMA_NOWAIT | BUS_DMA_ALLOCNOW, &ccb->dmamap_xfer);
	if (error) {
		printf("%s: unable to create DMA map, error = %d\n",
		       sc->sc_dev.dv_xname, error);
		return (error);
	}

	/*
	 * put in the phystokv hash table
	 * Never gets taken out.
	 */
	ccb->hashkey = sc->sc_dmamap_control->dm_segs[0].ds_addr +
	    ADW_CCB_OFF(ccb);
	hashnum = CCB_HASH(ccb->hashkey);
	ccb->nexthash = sc->sc_ccbhash[hashnum];
	sc->sc_ccbhash[hashnum] = ccb;
	adw_reset_ccb(ccb);
	return (0);
}


/*
 * Get a free ccb
 *
 * If there are none, see if we can allocate a new one
 */
static ADW_CCB *
adw_get_ccb(sc)
	ADW_SOFTC      *sc;
{
	ADW_CCB        *ccb = 0;
	int             s;

	s = splbio();
	ccb = TAILQ_FIRST(&sc->sc_free_ccb);
	if (ccb != NULL) {
		TAILQ_REMOVE(&sc->sc_free_ccb, ccb, chain);
		ccb->flags |= CCB_ALLOC;
	}
	splx(s);
	return (ccb);
}


/*
 * Given a physical address, find the ccb that it corresponds to.
 */
ADW_CCB *
adw_ccb_phys_kv(sc, ccb_phys)
	ADW_SOFTC	*sc;
	u_int32_t	ccb_phys;
{
	int hashnum = CCB_HASH(ccb_phys);
	ADW_CCB *ccb = sc->sc_ccbhash[hashnum];

	while (ccb) {
		if (ccb->hashkey == ccb_phys)
			break;
		ccb = ccb->nexthash;
	}
	return (ccb);
}


/*
 * Queue a CCB to be sent to the controller, and send it if possible.
 */
static void
adw_queue_ccb(sc, ccb)
	ADW_SOFTC      *sc;
	ADW_CCB        *ccb;
{
	int		s;

	s = splbio();
	TAILQ_INSERT_TAIL(&sc->sc_waiting_ccb, ccb, chain);
	splx(s);

	adw_start_ccbs(sc);
}


static void
adw_start_ccbs(sc)
	ADW_SOFTC      *sc;
{
	ADW_CCB        *ccb;
	int		s;

	while ((ccb = sc->sc_waiting_ccb.tqh_first) != NULL) {

		while (AdvExeScsiQueue(sc, &ccb->scsiq) == ADW_BUSY);

		s = splbio();
		TAILQ_REMOVE(&sc->sc_waiting_ccb, ccb, chain);
		splx(s);

		if ((ccb->xs->xs_control & XS_CTL_POLL) == 0)
			timeout(adw_timeout, ccb, (ccb->timeout * hz) / 1000);
	}
}


/******************************************************************************/
/*                           SCSI layer interfacing routines                  */
/******************************************************************************/


int
adw_init(sc)
	ADW_SOFTC      *sc;
{
	u_int16_t       warn_code;


	sc->cfg.lib_version = (ADW_LIB_VERSION_MAJOR << 8) |
		ADW_LIB_VERSION_MINOR;
	sc->cfg.chip_version =
		ADW_GET_CHIP_VERSION(sc->sc_iot, sc->sc_ioh, sc->bus_type);

	/*
	 * Reset the chip to start and allow register writes.
	 */
	if (ADW_FIND_SIGNATURE(sc->sc_iot, sc->sc_ioh) == 0) {
		panic("adw_init: adw_find_signature failed");
	} else {
		AdvResetChip(sc->sc_iot, sc->sc_ioh);

		warn_code = AdvInitFromEEP(sc);
		if (warn_code & ASC_WARN_EEPROM_CHKSUM)
			printf("%s: Bad checksum found. "
			       "Setting default values\n",
			       sc->sc_dev.dv_xname);
		if (warn_code & ASC_WARN_EEPROM_TERMINATION)
			printf("%s: Bad bus termination setting."
			       "Using automatic termination.\n",
			       sc->sc_dev.dv_xname);

		/*
		 * Reset the SCSI Bus if the EEPROM indicates that SCSI Bus
		 * Resets should be performed.
		 */
		if (sc->bios_ctrl & BIOS_CTRL_RESET_SCSI_BUS)
			AdvResetSCSIBus(sc);
	}

	sc->isr_callback = (ADW_CALLBACK) adw_wide_isr_callback;
	sc->sbreset_callback = (ADW_CALLBACK) adw_sbreset_callback;

	return (0);
}


void
adw_attach(sc)
	ADW_SOFTC      *sc;
{
	struct scsipi_adapter *adapt = &sc->sc_adapter;
	struct scsipi_channel *chan = &sc->sc_channel;
	int             i, error;

	/*
	 * Initialize the ASC3550.
	 */
	switch (AdvInitAsc3550Driver(sc)) {
	case ASC_IERR_MCODE_CHKSUM:
		panic("%s: Microcode checksum error",
		      sc->sc_dev.dv_xname);
		break;

	case ASC_IERR_ILLEGAL_CONNECTION:
		panic("%s: All three connectors are in use",
		      sc->sc_dev.dv_xname);
		break;

	case ASC_IERR_REVERSED_CABLE:
		panic("%s: Cable is reversed",
		      sc->sc_dev.dv_xname);
		break;

	case ASC_IERR_SINGLE_END_DEVICE:
		panic("%s: single-ended device is attached to"
		      " one of the connectors",
		      sc->sc_dev.dv_xname);
		break;
	}

	/*
	 * Fill in the scsipi_adapter.
	 */
	memset(adapt, 0, sizeof(*adapt));
	adapt->adapt_dev = &sc->sc_dev;
	adapt->adapt_nchannels = 1;
	/* adapt_openings filled in below */
	/* adapt_max_periph filled in below */
	adapt->adapt_request = adw_scsipi_request;
	adapt->adapt_minphys = adwminphys;

	/*
	 * Fill in the scsipi_channel.
	 */
	memset(chan, 0, sizeof(*chan));
	chan->chan_adapter = adapt;
	chan->chan_bustype = &scsi_bustype;
	chan->chan_channel = 0;
	chan->chan_ntargets = ADW_MAX_TID + 1;
	chan->chan_nluns = 7;
	chan->chan_id = sc->chip_scsi_id;

	TAILQ_INIT(&sc->sc_free_ccb);
	TAILQ_INIT(&sc->sc_waiting_ccb);

	/*
         * Allocate the Control Blocks.
         */
	error = adw_alloc_ccbs(sc);
	if (error)
		return; /* (error) */ ;

	/*
	 * Create and initialize the Control Blocks.
	 */
	i = adw_create_ccbs(sc, sc->sc_control->ccbs, ADW_MAX_CCB);
	if (i == 0) {
		printf("%s: unable to create control blocks\n",
		       sc->sc_dev.dv_xname);
		return; /* (ENOMEM) */ ;
	} else if (i != ADW_MAX_CCB) {
		printf("%s: WARNING: only %d of %d control blocks"
		       " created\n",
		       sc->sc_dev.dv_xname, i, ADW_MAX_CCB);
	}

	adapt->adapt_openings = i;
	adapt->adapt_max_periph = adapt->adapt_openings;

	config_found(&sc->sc_dev, &sc->sc_channel, scsiprint);
}


static void
adwminphys(bp)
	struct buf     *bp;
{

	if (bp->b_bcount > ((ADW_MAX_SG_LIST - 1) * PAGE_SIZE))
		bp->b_bcount = ((ADW_MAX_SG_LIST - 1) * PAGE_SIZE);
	minphys(bp);
}


/*
 * start a scsi operation given the command and the data address.
 * Also needs the unit, target and lu.
 */
static void
adw_scsipi_request(chan, req, arg)
	struct scsipi_channel *chan;
	scsipi_adapter_req_t req;
	void *arg;
{
	struct scsipi_xfer *xs;
	ADW_SOFTC      *sc = (void *)chan->chan_adapter->adapt_dev;
	ADW_CCB        *ccb;

	switch (req) {
	case ADAPTER_REQ_RUN_XFER:
		xs = arg;

		/*
		 * Get a CCB to use.
		 */
		ccb = adw_get_ccb(sc);
#ifdef DIAGNOSTIC
		/*
		 * This should never happen as we track the resources
		 * in the mid-layer.
		 */
		if (ccb == NULL) {
			scsipi_printaddr(xs->xs_periph);
			printf("unable to allocate ccb\n");
			panic("adw_scsipi_request");
		}
#endif

		ccb->xs = xs;
		ccb->timeout = xs->timeout;

		if (adw_build_req(sc, ccb)) {
			adw_queue_ccb(sc, ccb);

			if ((xs->xs_control & XS_CTL_POLL) == 0)
				return;

			/*
			 * Not allowed to use interrupts, poll for completion.
			 */
			if (adw_poll(sc, xs, ccb->timeout)) {
				adw_timeout(ccb);
				if (adw_poll(sc, xs, ccb->timeout))
					adw_timeout(ccb);
			}
		}
		return;

	case ADAPTER_REQ_GROW_RESOURCES:
		/* XXX Not supported. */
		return;

	case ADAPTER_REQ_SET_XFER_MODE:
		/* XXX XXX XXX */
		return;

	case ADAPTER_REQ_GET_XFER_MODE:
		/* XXX XXX XXX */
		return;
	}
}

/*
 * Build a request structure for the Wide Boards.
 */
static int
adw_build_req(sc, ccb)
	ADW_SOFTC	*sc;
	ADW_CCB		*ccb;
{
	struct scsipi_xfer *xs = ccb->xs;
	struct scsipi_periph *periph = xs->xs_periph;
	bus_dma_tag_t   dmat = sc->sc_dmat;
	ADW_SCSI_REQ_Q *scsiqp;
	int             error;

	scsiqp = &ccb->scsiq;
	bzero(scsiqp, sizeof(ADW_SCSI_REQ_Q));

	/*
	 * Set the ADW_SCSI_REQ_Q 'ccb_ptr' to point to the
	 * physical CCB structure.
	 */
	scsiqp->ccb_ptr = ccb->hashkey;

	/*
	 * Build the ADW_SCSI_REQ_Q request.
	 */

	/*
	 * Set CDB length and copy it to the request structure.
	 */
	bcopy(xs->cmd, &scsiqp->cdb, scsiqp->cdb_len = xs->cmdlen);

	scsiqp->target_id = periph->periph_target;
	scsiqp->target_lun = periph->periph_lun;

	scsiqp->vsense_addr = &ccb->scsi_sense;
	scsiqp->sense_addr = ccb->hashkey +
	    offsetof(struct adw_ccb, scsi_sense);
	scsiqp->sense_len = sizeof(struct scsipi_sense_data);

	/*
	 * Build ADW_SCSI_REQ_Q for a scatter-gather buffer command.
	 */
	if (xs->datalen) {
		/*
                 * Map the DMA transfer.
                 */
#ifdef TFS
		if (xs->xs_control & SCSI_DATA_UIO) {
			error = bus_dmamap_load_uio(dmat,
			    ccb->dmamap_xfer, (struct uio *) xs->data,
			    BUS_DMA_NOWAIT);
		} else
#endif /* TFS */
		{
			error = bus_dmamap_load(dmat,
			    ccb->dmamap_xfer, xs->data, xs->datalen, NULL,
			    BUS_DMA_NOWAIT);
		}

		switch (error) {
		case 0:
			break;

		case ENOMEM:
		case EAGAIN:
			xs->error = XS_RESOURCE_SHORTAGE;
			goto out_bad;

		default:
			xs->error = XS_DRIVER_STUFFUP;
			printf("%s: error %d loading DMA map\n",
			    sc->sc_dev.dv_xname, error);
 out_bad:
			adw_free_ccb(sc, ccb);
			scsipi_done(xs);
			return (0);
		}
		bus_dmamap_sync(dmat, ccb->dmamap_xfer, 0,
		    ccb->dmamap_xfer->dm_mapsize,
		    (xs->xs_control & XS_CTL_DATA_IN) ?
		    BUS_DMASYNC_PREREAD : BUS_DMASYNC_PREWRITE);

		/*
		 * Build scatter-gather list.
		 */
		scsiqp->data_cnt = xs->datalen;
		scsiqp->vdata_addr = xs->data;
		scsiqp->data_addr = ccb->dmamap_xfer->dm_segs[0].ds_addr;
		bzero(ccb->sg_block, sizeof(ADW_SG_BLOCK) * ADW_NUM_SG_BLOCK);
		adw_build_sglist(ccb, scsiqp, ccb->sg_block);
	} else {
		/*
                 * No data xfer, use non S/G values.
                 */
		scsiqp->data_cnt = 0;
		scsiqp->vdata_addr = 0;
		scsiqp->data_addr = 0;
	}

	return (1);
}


/*
 * Build scatter-gather list for Wide Boards.
 */
static void
adw_build_sglist(ccb, scsiqp, sg_block)
	ADW_CCB        *ccb;
	ADW_SCSI_REQ_Q *scsiqp;
	ADW_SG_BLOCK   *sg_block;
{
	u_long          sg_block_next_addr;	/* block and its next */
	u_int32_t       sg_block_physical_addr;
	int             sg_block_index, i;	/* how many SG entries */
	bus_dma_segment_t *sg_list = &ccb->dmamap_xfer->dm_segs[0];
	int             sg_elem_cnt = ccb->dmamap_xfer->dm_nsegs;


	sg_block_next_addr = (u_long) sg_block;	/* allow math operation */
	sg_block_physical_addr = ccb->hashkey +
	    offsetof(struct adw_ccb, sg_block[0]);
	scsiqp->sg_real_addr = sg_block_physical_addr;

	/*
	 * If there are more than NO_OF_SG_PER_BLOCK dma segments (hw sg-list)
	 * then split the request into multiple sg-list blocks.
	 */

	sg_block_index = 0;
	do {
		sg_block->first_entry_no = sg_block_index;
		for (i = 0; i < NO_OF_SG_PER_BLOCK; i++) {
			sg_block->sg_list[i].sg_addr = sg_list->ds_addr;
			sg_block->sg_list[i].sg_count = sg_list->ds_len;

			if (--sg_elem_cnt == 0) {
				/* last entry, get out */
				scsiqp->sg_entry_cnt = sg_block_index + i + 1;
				sg_block->last_entry_no = sg_block_index + i;
				sg_block->sg_ptr = NULL; /* next link = NULL */
				return;
			}
			sg_list++;
		}
		sg_block_next_addr += sizeof(ADW_SG_BLOCK);
		sg_block_physical_addr += sizeof(ADW_SG_BLOCK);

		sg_block_index += NO_OF_SG_PER_BLOCK;
		sg_block->sg_ptr = sg_block_physical_addr;
		sg_block->last_entry_no = sg_block_index - 1;
		sg_block = (ADW_SG_BLOCK *) sg_block_next_addr;	/* virt. addr */
	} while (1);
}


int
adw_intr(arg)
	void           *arg;
{
	ADW_SOFTC      *sc = arg;

	AdvISR(sc);
	return (1);
}


/*
 * Poll a particular unit, looking for a particular xs
 */
static int
adw_poll(sc, xs, count)
	ADW_SOFTC      *sc;
	struct scsipi_xfer *xs;
	int             count;
{

	/* timeouts are in msec, so we loop in 1000 usec cycles */
	while (count) {
		adw_intr(sc);
		if (xs->xs_status & XS_STS_DONE)
			return (0);
		delay(1000);	/* only happens in boot so ok */
		count--;
	}
	return (1);
}


static void
adw_timeout(arg)
	void           *arg;
{
	ADW_CCB        *ccb = arg;
	struct scsipi_xfer *xs = ccb->xs;
	struct scsipi_periph *periph = xs->xs_periph;
	ADW_SOFTC      *sc =
	    (void *)periph->periph_channel->chan_adapter->adapt_dev;
	int             s;

	scsipi_printaddr(periph);
	printf("timed out");

	s = splbio();

	/*
         * If it has been through before, then a previous abort has failed,
         * don't try abort again, reset the bus instead.
         */
	if (ccb->flags & CCB_ABORTED) {
	/*
	 * Abort Timed Out
	 * Lets try resetting the bus!
	 */
		printf(" AGAIN. Resetting SCSI Bus\n");
		ccb->flags &= ~CCB_ABORTED;
		/* AdvResetSCSIBus() will call sbreset_callback() */
		AdvResetSCSIBus(sc);
	} else {
	/*
	 * Abort the operation that has timed out
	 */
		printf("\n");
		xs->error = XS_TIMEOUT;
		ccb->flags |= CCB_ABORTING;
		/* ADW_ABORT_CCB() will implicitly call isr_callback() */
		ADW_ABORT_CCB(sc, ccb);
	}

	splx(s);
}


/******************************************************************************/
/*                           WIDE boards Interrupt callbacks                  */
/******************************************************************************/


/*
 * adw_wide_isr_callback() - Second Level Interrupt Handler called by AdvISR()
 *
 * Interrupt callback function for the Wide SCSI Adv Library.
 */
static void
adw_wide_isr_callback(sc, scsiq)
	ADW_SOFTC      *sc;
	ADW_SCSI_REQ_Q *scsiq;
{
	bus_dma_tag_t   dmat = sc->sc_dmat;
	ADW_CCB        *ccb;
	struct scsipi_xfer *xs;
	struct scsipi_sense_data *s1, *s2;
	int		 s;
	//int           underrun = ASC_FALSE;


	ccb = adw_ccb_phys_kv(sc, scsiq->ccb_ptr);

	untimeout(adw_timeout, ccb);

	if(ccb->flags & CCB_ABORTING) {
		printf("Retrying request\n");
		ccb->flags &= ~CCB_ABORTING;
		ccb->flags |= CCB_ABORTED;
		s = splbio();
		adw_queue_ccb(sc, ccb);
		splx(s);
		return;
	}

	xs = ccb->xs;


	/*
         * If we were a data transfer, unload the map that described
         * the data buffer.
         */
	if (xs->datalen) {
		bus_dmamap_sync(dmat, ccb->dmamap_xfer, 0,
				ccb->dmamap_xfer->dm_mapsize,
			 (xs->xs_control & XS_CTL_DATA_IN) ?
			 BUS_DMASYNC_POSTREAD : BUS_DMASYNC_POSTWRITE);
		bus_dmamap_unload(dmat, ccb->dmamap_xfer);
	}
	if ((ccb->flags & CCB_ALLOC) == 0) {
		printf("%s: exiting ccb not allocated!\n", sc->sc_dev.dv_xname);
		Debugger();
		return;
	}
	/*
	 * Check for an underrun condition.
	 */
	/*
	 * if (xs->request_bufflen != 0 && scsiqp->data_cnt != 0) {
	 * ASC_DBG1(1, "adw_isr_callback: underrun condition %lu bytes\n",
	 * scsiqp->data_cnt); underrun = ASC_TRUE; }
	 */
	/*
	 * 'done_status' contains the command's ending status.
	 */
	switch (scsiq->done_status) {
	case QD_NO_ERROR:
		switch (scsiq->host_status) {
		case QHSTA_NO_ERROR:
			xs->error = XS_NOERROR;
			xs->resid = 0;
			break;
		case QHSTA_M_SEL_TIMEOUT:
		default:
			/* QHSTA error occurred. */
			xs->error = XS_DRIVER_STUFFUP;
			break;
		}
		/*
		 * If there was an underrun without any other error,
		 * set DID_ERROR to indicate the underrun error.
		 *
		 * Note: There is no way yet to indicate the number
		 * of underrun bytes.
		 */
		/*
		 * if (xs->error == XS_NOERROR && underrun == ASC_TRUE) {
		 * scp->result = HOST_BYTE(DID_UNDERRUN); }
		 */ break;

	case QD_WITH_ERROR:
		switch (scsiq->host_status) {
		case QHSTA_NO_ERROR:
			switch(scsiq->scsi_status) {
			case SS_CHK_CONDITION:
			case SS_CMD_TERMINATED:
				s1 = &ccb->scsi_sense;
				s2 = &xs->sense.scsi_sense;
				*s2 = *s1;
				xs->error = XS_SENSE;
				break;
			case SS_TARGET_BUSY:
			case SS_RSERV_CONFLICT:
			case SS_QUEUE_FULL:
				xs->error = XS_DRIVER_STUFFUP;
				break;
			case SS_CONDITION_MET:
			case SS_INTERMID:
			case SS_INTERMID_COND_MET:
				xs->error = XS_DRIVER_STUFFUP;
				break;
			case SS_GOOD:
				break;
			}
			break;

		case QHSTA_M_SEL_TIMEOUT:
			xs->error = XS_DRIVER_STUFFUP;
			break;

		default:
			/* Some other QHSTA error occurred. */
			xs->error = XS_DRIVER_STUFFUP;
			break;
		}
		break;

	case QD_ABORTED_BY_HOST:
		xs->error = XS_DRIVER_STUFFUP;
		break;

	default:
		xs->error = XS_DRIVER_STUFFUP;
		break;
	}

	adw_free_ccb(sc, ccb);
	xs->xs_status |= XS_STS_DONE;
	scsipi_done(xs);
}


static void
adw_sbreset_callback(sc)
	ADW_SOFTC	*sc;
{
}
