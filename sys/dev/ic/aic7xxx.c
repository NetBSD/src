/*	$NetBSD: aic7xxx.c,v 1.66.2.5 2001/09/26 19:54:52 nathanw Exp $	*/

/*
 * Generic driver for the aic7xxx based adaptec SCSI controllers
 * Product specific probe and attach routines can be found in:
 * i386/eisa/ahc_eisa.c	27/284X and aic7770 motherboard controllers
 * pci/ahc_pci.c	3985, 3980, 3940, 2940, aic7895, aic7890,
 *			aic7880, aic7870, aic7860, and aic7850 controllers
 *
 * Copyright (c) 1994, 1995, 1996, 1997, 1998, 1999, 2000 Justin T. Gibbs.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer,
 *    without modification.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * Alternatively, this software may be distributed under the terms of the
 * the GNU Public License ("GPL").
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: src/sys/dev/aic7xxx/aic7xxx.c,v 1.42 2000/03/18 22:28:18 gibbs Exp $
 */
/*
 * A few notes on features of the driver.
 *
 * SCB paging takes advantage of the fact that devices stay disconnected
 * from the bus a relatively long time and that while they're disconnected,
 * having the SCBs for these transactions down on the host adapter is of
 * little use.  Instead of leaving this idle SCB down on the card we copy
 * it back up into kernel memory and reuse the SCB slot on the card to
 * schedule another transaction.  This can be a real payoff when doing random
 * I/O to tagged queueing devices since there are more transactions active at
 * once for the device to sort for optimal seek reduction. The algorithm goes
 * like this...
 *
 * The sequencer maintains two lists of its hardware SCBs.  The first is the
 * singly linked free list which tracks all SCBs that are not currently in
 * use.  The second is the doubly linked disconnected list which holds the
 * SCBs of transactions that are in the disconnected state sorted most
 * recently disconnected first.  When the kernel queues a transaction to
 * the card, a hardware SCB to "house" this transaction is retrieved from
 * either of these two lists.  If the SCB came from the disconnected list,
 * a check is made to see if any data transfer or SCB linking (more on linking
 * in a bit) information has been changed since it was copied from the host
 * and if so, DMAs the SCB back up before it can be used.  Once a hardware
 * SCB has been obtained, the SCB is DMAed from the host.  Before any work
 * can begin on this SCB, the sequencer must ensure that either the SCB is
 * for a tagged transaction or the target is not already working on another
 * non-tagged transaction.  If a conflict arises in the non-tagged case, the
 * sequencer finds the SCB for the active transactions and sets the SCB_LINKED
 * field in that SCB to this next SCB to execute.  To facilitate finding
 * active non-tagged SCBs, the last four bytes of up to the first four hardware
 * SCBs serve as a storage area for the currently active SCB ID for each
 * target.
 *
 * When a device reconnects, a search is made of the hardware SCBs to find
 * the SCB for this transaction.  If the search fails, a hardware SCB is
 * pulled from either the free or disconnected SCB list and the proper
 * SCB is DMAed from the host.  If the MK_MESSAGE control bit is set
 * in the control byte of the SCB while it was disconnected, the sequencer
 * will assert ATN and attempt to issue a message to the host.
 *
 * When a command completes, a check for non-zero status and residuals is
 * made.  If either of these conditions exists, the SCB is DMAed back up to
 * the host so that it can interpret this information.  Additionally, in the
 * case of bad status, the sequencer generates a special interrupt and pauses
 * itself.  This allows the host to setup a request sense command if it
 * chooses for this target synchronously with the error so that sense
 * information isn't lost.
 *
 */

#include "opt_ddb.h"
#include "opt_ahc.h"

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/scsiio.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsi_message.h>
#include <dev/scsipi/scsipi_debug.h>
#include <dev/scsipi/scsiconf.h>

#include <uvm/uvm_extern.h>

#include <dev/ic/aic7xxxvar.h>
#include <dev/microcode/aic7xxx/sequencer.h>
#include <dev/microcode/aic7xxx/aic7xxx_reg.h>
#include <dev/microcode/aic7xxx/aic7xxx_seq.h>

#define ALL_CHANNELS '\0'
#define ALL_TARGETS_MASK 0xFFFF
#define INITIATOR_WILDCARD	(~0)

#define	SIM_IS_SCSIBUS_B(ahc, periph)	\
	((periph)->periph_channel->chan_channel == 1)
#define	SIM_CHANNEL(ahc, periph)	\
	(SIM_IS_SCSIBUS_B(ahc, periph) ? 'B' : 'A')
#define	SIM_SCSI_ID(ahc, periph)	\
	(SIM_IS_SCSIBUS_B(ahc, periph) ? ahc->our_id_b : ahc->our_id)
#define	SCB_IS_SCSIBUS_B(scb)	\
	(((scb)->hscb->tcl & SELBUSB) != 0)
#define	SCB_TARGET(scb)	\
	(((scb)->hscb->tcl & TID) >> 4)
#define	SCB_CHANNEL(scb) \
	(SCB_IS_SCSIBUS_B(scb) ? 'B' : 'A')
#define	SCB_LUN(scb)	\
	((scb)->hscb->tcl & LID)
#define SCB_TARGET_OFFSET(scb)		\
	(SCB_TARGET(scb) + (SCB_IS_SCSIBUS_B(scb) ? 8 : 0))
#define SCB_TARGET_MASK(scb)		\
	(0x01 << (SCB_TARGET_OFFSET(scb)))
#define TCL_CHANNEL(ahc, tcl)		\
	((((ahc)->features & AHC_TWIN) && ((tcl) & SELBUSB)) ? 'B' : 'A')
#define TCL_SCSI_ID(ahc, tcl)		\
	(TCL_CHANNEL((ahc), (tcl)) == 'B' ? (ahc)->our_id_b : (ahc)->our_id)
#define TCL_TARGET(tcl) (((tcl) & TID) >> TCL_TARGET_SHIFT)
#define TCL_LUN(tcl) ((tcl) & LID)

#define XS_TCL(ahc, xs) \
	((((xs)->xs_periph->periph_target << 4) & 0xF0) \
	    | (SIM_IS_SCSIBUS_B((ahc), (xs)->xs_periph) ? SELBUSB : 0) \
	    | ((xs)->xs_periph->periph_lun & 0x07))

const char * const ahc_chip_names[] =
{
	"NONE",
	"aic7770",
	"aic7850",
	"aic7855",
	"aic7859",
	"aic7860",
	"aic7870",
	"aic7880",
	"aic7890/91",
	"aic7892",
	"aic7895",
	"aic7896/97",
	"aic7899"
};

typedef enum {
	ROLE_UNKNOWN,
	ROLE_INITIATOR,
	ROLE_TARGET
} role_t;

struct ahc_devinfo {
	int	  our_scsiid;
	int	  target_offset;
	u_int16_t target_mask;
	u_int8_t  target;
	u_int8_t  lun;
	char	  channel;
	role_t	  role;		/*
				 * Only guaranteed to be correct if not
				 * in the busfree state.
				 */
};

typedef enum {
	SEARCH_COMPLETE,
	SEARCH_COUNT,
	SEARCH_REMOVE
} ahc_search_action;

#ifdef AHC_DEBUG
static int     ahc_debug = AHC_DEBUG;
#endif

static int	ahcinitscbdata(struct ahc_softc *);
static void	ahcfiniscbdata(struct ahc_softc *);

#if UNUSED
static void	ahc_dump_targcmd(struct target_cmd *);
#endif
static void	ahc_shutdown(void *arg);
static void	ahc_action(struct scsipi_channel *,
				scsipi_adapter_req_t, void *);
static int	ahc_ioctl(struct scsipi_channel *, u_long, caddr_t, int,
			  struct proc *);
static void	ahc_execute_scb(void *, bus_dma_segment_t *, int);
static int	ahc_poll(struct ahc_softc *, int);
static void	ahc_setup_data(struct ahc_softc *, struct scsipi_xfer *,
			       struct scb *);
static void	ahc_freeze_devq(struct ahc_softc *, struct scsipi_periph *);
static void	ahcallocscbs(struct ahc_softc *);
#if UNUSED
static void	ahc_scb_devinfo(struct ahc_softc *, struct ahc_devinfo *,
				struct scb *);
#endif
static void	ahc_fetch_devinfo(struct ahc_softc *, struct ahc_devinfo *);
static void	ahc_compile_devinfo(struct ahc_devinfo *, u_int, u_int, u_int,
				    char, role_t);
static u_int	ahc_abort_wscb(struct ahc_softc *, u_int, u_int);
static void	ahc_done(struct ahc_softc *, struct scb *);
static struct tmode_tstate *
		ahc_alloc_tstate(struct ahc_softc *, u_int, char);
#if UNUSED
static void	ahc_free_tstate(struct ahc_softc *, u_int, char, int);
#endif
static void 	ahc_handle_seqint(struct ahc_softc *, u_int);
static void	ahc_handle_scsiint(struct ahc_softc *, u_int);
static void	ahc_build_transfer_msg(struct ahc_softc *,
				       struct ahc_devinfo *);
static void	ahc_setup_initiator_msgout(struct ahc_softc *,
					   struct ahc_devinfo *,
					   struct scb *);
static void	ahc_setup_target_msgin(struct ahc_softc *,
				       struct ahc_devinfo *);
static void	ahc_clear_msg_state(struct ahc_softc *);
static void	ahc_handle_message_phase(struct ahc_softc *,
					 struct scsipi_periph *);
static int	ahc_sent_msg(struct ahc_softc *, u_int, int);

static int	ahc_parse_msg(struct ahc_softc *, struct scsipi_periph *,
			      struct ahc_devinfo *);
static void	ahc_handle_ign_wide_residue(struct ahc_softc *,
					    struct ahc_devinfo *);
static void	ahc_handle_devreset(struct ahc_softc *, struct ahc_devinfo *,
				    int, char *, int);
#ifdef AHC_DUMP_SEQ
static void	ahc_dumpseq(struct ahc_softc *);
#endif
static void	ahc_loadseq(struct ahc_softc *);
static int	ahc_check_patch(struct ahc_softc *, const struct patch **,
				int, int *);
static void	ahc_download_instr(struct ahc_softc *, int, u_int8_t *);
static int	ahc_match_scb(struct scb *, int, char, int, u_int, role_t);
#if defined(AHC_DEBUG)
static void	ahc_print_scb(struct scb *);
#endif
static int	ahc_search_qinfifo(struct ahc_softc *, int, char, int, u_int,
				   role_t, scb_flag, ahc_search_action);
static int	ahc_reset_channel(struct ahc_softc *, char, int);
static int	ahc_abort_scbs(struct ahc_softc *, int, char, int, u_int,
			       role_t, int);
static int	ahc_search_disc_list(struct ahc_softc *, int,
				     char, int, u_int, int, int, int);
static u_int	ahc_rem_scb_from_disc_list(struct ahc_softc *, u_int, u_int);
static void	ahc_add_curscb_to_free_list(struct ahc_softc *);
static void	ahc_clear_intstat(struct ahc_softc *);
static void	ahc_reset_current_bus(struct ahc_softc *);
static const struct ahc_syncrate *
		ahc_devlimited_syncrate(struct ahc_softc *, u_int *);
static const struct ahc_syncrate *
		ahc_find_syncrate(struct ahc_softc *, u_int *, u_int);
static u_int	ahc_find_period(struct ahc_softc *, u_int, u_int);
static void	ahc_validate_offset(struct ahc_softc *,
				const struct ahc_syncrate *, u_int *, int);
static void	ahc_update_target_msg_request(struct ahc_softc *,
					      struct ahc_devinfo *,
					      struct ahc_initiator_tinfo *,
					      int, int);
static void	ahc_set_syncrate(struct ahc_softc *, struct ahc_devinfo *,
				 const struct ahc_syncrate *, u_int, u_int,
				 u_int, int, int);
static void	ahc_set_width(struct ahc_softc *, struct ahc_devinfo *,
			      u_int, u_int, int, int);
static void	ahc_set_tags(struct ahc_softc *, struct ahc_devinfo *, int);
static void	ahc_update_xfer_mode(struct ahc_softc *, struct ahc_devinfo *);
static void	ahc_construct_sdtr(struct ahc_softc *, u_int, u_int);

static void	ahc_construct_wdtr(struct ahc_softc *, u_int);

static void	ahc_calc_residual(struct scb *);

static void	ahc_update_pending_syncrates(struct ahc_softc *);

static void	ahc_set_recoveryscb(struct ahc_softc *, struct scb *);

static void     ahc_timeout (void *);
static __inline int  sequencer_paused(struct ahc_softc *);
static __inline void pause_sequencer(struct ahc_softc *);
static __inline void unpause_sequencer(struct ahc_softc *);
static 		void restart_sequencer(struct ahc_softc *);
static __inline u_int ahc_index_busy_tcl(struct ahc_softc *, u_int, int);

static __inline void	 ahc_busy_tcl(struct ahc_softc *, struct scb *);
static __inline int	ahc_isbusy_tcl(struct ahc_softc *, struct scb *);

static __inline void	   ahc_freeze_ccb(struct scb *);
static __inline void	   ahcsetccbstatus(struct scsipi_xfer *, int);
static void		   ahc_run_qoutfifo(struct ahc_softc *);

static __inline struct ahc_initiator_tinfo *
			   ahc_fetch_transinfo(struct ahc_softc *,
					       char, u_int, u_int,
					       struct tmode_tstate **);
static void	   ahcfreescb(struct ahc_softc *, struct scb *);
static __inline	struct scb *ahcgetscb(struct ahc_softc *);

static int ahc_createdmamem(bus_dma_tag_t, int, int, bus_dmamap_t *,
			    caddr_t *, bus_addr_t *, bus_dma_segment_t *,
			    int *, const char *, const char *);
static void ahc_freedmamem(bus_dma_tag_t, int, bus_dmamap_t,
			   caddr_t, bus_dma_segment_t *, int);
static void ahcminphys(struct buf *);

static __inline void ahc_swap_hscb(struct hardware_scb *);
static __inline void ahc_swap_sg(struct ahc_dma_seg *);
static int ahc_istagged_device(struct ahc_softc *, struct scsipi_xfer *, int);

#if defined(AHC_DEBUG) && 0
static void ahc_dumptinfo(struct ahc_softc *, struct ahc_initiator_tinfo *);
#endif

static __inline void
ahc_swap_hscb(struct hardware_scb *hscb)
{
	hscb->SG_pointer = htole32(hscb->SG_pointer);
	hscb->data = htole32(hscb->data);
	hscb->datalen = htole32(hscb->datalen);
	/*
	 * No need to swap cmdpointer; it's either 0 or set to
	 * cmdstore_busaddr, which is already swapped.
	 */
}

static __inline void
ahc_swap_sg(struct ahc_dma_seg *sg)
{
	sg->addr = htole32(sg->addr);
	sg->len = htole32(sg->len);
}

static void
ahcminphys(bp)
	struct buf *bp;
{
/*
 * Even though the card can transfer up to 16megs per command
 * we are limited by the number of segments in the dma segment
 * list that we can hold.  The worst case is that all pages are
 * discontinuous physically, hense the "page per segment" limit
 * enforced here.
 */
	if (bp->b_bcount > AHC_MAXTRANSFER_SIZE) {
		bp->b_bcount = AHC_MAXTRANSFER_SIZE;
	}
	minphys(bp);
}


static __inline u_int32_t
ahc_hscb_busaddr(struct ahc_softc *ahc, u_int index)
{
	return (ahc->scb_data->hscb_busaddr
		+ (sizeof(struct hardware_scb) * index));
}

#define AHC_BUSRESET_DELAY	25	/* Reset delay in us */

static __inline int
sequencer_paused(struct ahc_softc *ahc)
{
	return ((ahc_inb(ahc, HCNTRL) & PAUSE) != 0);
}

static __inline void
pause_sequencer(struct ahc_softc *ahc)
{
	ahc_outb(ahc, HCNTRL, ahc->pause);

	/*
	 * Since the sequencer can disable pausing in a critical section, we
	 * must loop until it actually stops.
	 */
	while (sequencer_paused(ahc) == 0)
		;
}

static __inline void
unpause_sequencer(struct ahc_softc *ahc)
{
	if ((ahc_inb(ahc, INTSTAT) & (SCSIINT | SEQINT | BRKADRINT)) == 0)
		ahc_outb(ahc, HCNTRL, ahc->unpause);
}

/*
 * Restart the sequencer program from address zero
 */
static void
restart_sequencer(struct ahc_softc *ahc)
{
	u_int i;

	pause_sequencer(ahc);

	/*
	 * Everytime we restart the sequencer, there
	 * is the possiblitity that we have restarted
	 * within a three instruction window where an
	 * SCB has been marked free but has not made it
	 * onto the free list.  Since SCSI events(bus reset,
	 * unexpected bus free) will always freeze the
	 * sequencer, we cannot close this window.  To
	 * avoid losing an SCB, we reconsitute the free
	 * list every time we restart the sequencer.
	 */
	ahc_outb(ahc, FREE_SCBH, SCB_LIST_NULL);
	for (i = 0; i < ahc->scb_data->maxhscbs; i++) {

		ahc_outb(ahc, SCBPTR, i);
		if (ahc_inb(ahc, SCB_TAG) == SCB_LIST_NULL)
			ahc_add_curscb_to_free_list(ahc);
	}
	ahc_outb(ahc, SEQCTL, FASTMODE|SEQRESET);
	unpause_sequencer(ahc);
}

static __inline u_int
ahc_index_busy_tcl(struct ahc_softc *ahc, u_int tcl, int unbusy)
{
	u_int scbid;

	scbid = ahc->untagged_scbs[tcl];
	if (unbusy) {
		ahc->untagged_scbs[tcl] = SCB_LIST_NULL;
		bus_dmamap_sync(ahc->parent_dmat, ahc->shared_data_dmamap,
		    UNTAGGEDSCB_OFFSET * 256, 256, BUS_DMASYNC_PREWRITE);
	}

	return (scbid);
}

static __inline void
ahc_busy_tcl(struct ahc_softc *ahc, struct scb *scb)
{
	ahc->untagged_scbs[scb->hscb->tcl] = scb->hscb->tag;
	bus_dmamap_sync(ahc->parent_dmat, ahc->shared_data_dmamap,
	    UNTAGGEDSCB_OFFSET * 256, 256, BUS_DMASYNC_PREWRITE);
}

static __inline int
ahc_isbusy_tcl(struct ahc_softc *ahc, struct scb *scb)
{
	return ahc->untagged_scbs[scb->hscb->tcl] != SCB_LIST_NULL;
}

static __inline void
ahc_freeze_ccb(struct scb *scb)
{
	struct scsipi_xfer *xs = scb->xs;

	if (!(scb->flags & SCB_FREEZE_QUEUE)) {
		scsipi_periph_freeze(xs->xs_periph, 1);
		scb->flags |= SCB_FREEZE_QUEUE;
	}
}

static __inline void
ahcsetccbstatus(struct scsipi_xfer *xs, int status)
{
	xs->error = status;
}

static __inline struct ahc_initiator_tinfo *
ahc_fetch_transinfo(struct ahc_softc *ahc, char channel, u_int our_id,
		    u_int remote_id, struct tmode_tstate **tstate)
{
	/*
	 * Transfer data structures are stored from the perspective
	 * of the target role.  Since the parameters for a connection
	 * in the initiator role to a given target are the same as
	 * when the roles are reversed, we pretend we are the target.
	 */
	if (channel == 'B')
		our_id += 8;
	*tstate = ahc->enabled_targets[our_id];
	return (&(*tstate)->transinfo[remote_id]);
}

static void
ahc_run_qoutfifo(struct ahc_softc *ahc)
{
	struct scb *scb;
	u_int  scb_index;

	bus_dmamap_sync(ahc->parent_dmat, ahc->shared_data_dmamap, 0,
	     256, BUS_DMASYNC_POSTREAD);

	while (ahc->qoutfifo[ahc->qoutfifonext] != SCB_LIST_NULL) {
		scb_index = ahc->qoutfifo[ahc->qoutfifonext];
		ahc->qoutfifo[ahc->qoutfifonext++] = SCB_LIST_NULL;

		scb = &ahc->scb_data->scbarray[scb_index];
		if (scb_index >= ahc->scb_data->numscbs
		  || (scb->flags & SCB_ACTIVE) == 0) {
			printf("%s: WARNING no command for scb %d "
			       "(cmdcmplt)\nQOUTPOS = %d\n",
			       ahc_name(ahc), scb_index,
			       ahc->qoutfifonext - 1);
			continue;
		}

		/*
		 * Save off the residual
		 * if there is one.
		 */
		if (scb->hscb->residual_SG_count != 0)
			ahc_calc_residual(scb);
		else
			scb->xs->resid = 0;
#ifdef AHC_DEBUG
		if (ahc_debug & AHC_SHOWSCBS) {
			scsipi_printaddr(scb->xs->xs_periph);
			printf("run_qoutfifo: SCB %x complete\n",
			    scb->hscb->tag);
		}
#endif
		ahc_done(ahc, scb);
	}
}


/*
 * An scb (and hence an scb entry on the board) is put onto the
 * free list.
 */
static void
ahcfreescb(struct ahc_softc *ahc, struct scb *scb)
{
	struct hardware_scb *hscb;
	int opri;

	hscb = scb->hscb;

#ifdef AHC_DEBUG
	if (ahc_debug & AHC_SHOWSCBALLOC)
		printf("%s: free SCB tag %x\n", ahc_name(ahc), hscb->tag);
#endif

	opri = splbio();

	if ((ahc->flags & AHC_RESOURCE_SHORTAGE) != 0 ||
	    (scb->flags & SCB_RECOVERY_SCB) != 0) {
		ahc->flags &= ~AHC_RESOURCE_SHORTAGE;
		scsipi_channel_thaw(&ahc->sc_channel, 1);
		if (ahc->features & AHC_TWIN)
			scsipi_channel_thaw(&ahc->sc_channel_b, 1);
	}

	/* Clean up for the next user */
	scb->flags = SCB_FREE;
	hscb->control = 0;
	hscb->status = 0;

	SLIST_INSERT_HEAD(&ahc->scb_data->free_scbs, scb, links);

	splx(opri);
}

/*
 * Get a free scb, either one already assigned to a hardware slot
 * on the adapter or one that will require an SCB to be paged out before
 * use. If there are none, see if we can allocate a new SCB.  Otherwise
 * either return an error or sleep.
 */
static __inline struct scb *
ahcgetscb(struct ahc_softc *ahc)
{
	struct scb *scbp;
	int opri;;

	opri = splbio();
	if ((scbp = SLIST_FIRST(&ahc->scb_data->free_scbs))) {
		SLIST_REMOVE_HEAD(&ahc->scb_data->free_scbs, links);
	} else {
		ahcallocscbs(ahc);
		scbp = SLIST_FIRST(&ahc->scb_data->free_scbs);
		if (scbp != NULL)
			SLIST_REMOVE_HEAD(&ahc->scb_data->free_scbs, links);
	}

	splx(opri);

#ifdef AHC_DEBUG
	if (ahc_debug & AHC_SHOWSCBALLOC) {
		if (scbp != NULL)
			printf("%s: new SCB, tag %x\n", ahc_name(ahc),
			    scbp->hscb->tag);
		else
			printf("%s: failed to allocate new SCB\n",
			    ahc_name(ahc));
	}
#endif

	return (scbp);
}

static int
ahc_createdmamem(tag, size, flags, mapp, vaddr, baddr, seg, nseg, myname, what)
	bus_dma_tag_t tag;
	int size;
	int flags;
	bus_dmamap_t *mapp;
	caddr_t *vaddr;
	bus_addr_t *baddr;
	bus_dma_segment_t *seg;
	int *nseg;
	const char *myname, *what;
{
	int error, level = 0;

	if ((error = bus_dmamem_alloc(tag, size, PAGE_SIZE, 0,
			seg, 1, nseg, BUS_DMA_NOWAIT)) != 0) {
		printf("%s: failed to allocate DMA mem for %s, error = %d\n",
			myname, what, error);
		goto out;
	}
	level++;

	if ((error = bus_dmamem_map(tag, seg, *nseg, size, vaddr,
			BUS_DMA_NOWAIT|BUS_DMA_COHERENT)) != 0) {
		printf("%s: failed to map DMA mem for %s, error = %d\n",
			myname, what, error);
		goto out;
	}
	level++;

	if ((error = bus_dmamap_create(tag, size, 1, size, 0,
			BUS_DMA_NOWAIT | flags, mapp)) != 0) {
                printf("%s: failed to create DMA map for %s, error = %d\n",
			myname, what, error);
		goto out;
        }
	level++;

	if ((error = bus_dmamap_load(tag, *mapp, *vaddr, size, NULL,
			BUS_DMA_NOWAIT)) != 0) {
                printf("%s: failed to load DMA map for %s, error = %d\n",
			myname, what, error);
		goto out;
        }

	*baddr = (*mapp)->dm_segs[0].ds_addr;

#ifdef AHC_DEBUG
	printf("%s: dmamem for %s at busaddr %lx virt %lx nseg %d size %d\n",
	    myname, what, (unsigned long)*baddr, (unsigned long)*vaddr,
	    *nseg, size);
#endif

	return 0;
out:
	switch (level) {
	case 3:
		bus_dmamap_destroy(tag, *mapp);
		/* FALLTHROUGH */
	case 2:
		bus_dmamem_unmap(tag, *vaddr, size);
		/* FALLTHROUGH */
	case 1:
		bus_dmamem_free(tag, seg, *nseg);
		break;
	default:
		break;
	}

	return error;
}

static void
ahc_freedmamem(tag, size, map, vaddr, seg, nseg)
	bus_dma_tag_t tag;
	int size;
	bus_dmamap_t map;
	caddr_t vaddr;
	bus_dma_segment_t *seg;
	int nseg;
{

	bus_dmamap_unload(tag, map);
	bus_dmamap_destroy(tag, map);
	bus_dmamem_unmap(tag, vaddr, size);
	bus_dmamem_free(tag, seg, nseg);
}

char *
ahc_name(struct ahc_softc *ahc)
{
	return (ahc->sc_dev.dv_xname);
}

#ifdef AHC_DEBUG
static void
ahc_print_scb(struct scb *scb)
{
	struct hardware_scb *hscb = scb->hscb;

	printf("scb:%p tag %x control:0x%x tcl:0x%x cmdlen:%d cmdpointer:0x%lx\n",
		scb,
		hscb->tag,
		hscb->control,
		hscb->tcl,
		hscb->cmdlen,
		(unsigned long)le32toh(hscb->cmdpointer));
	printf("        datlen:%u data:0x%lx segs:0x%x segp:0x%lx\n",
		le32toh(hscb->datalen),
		(unsigned long)(le32toh(hscb->data)),
		hscb->SG_count,
		(unsigned long)(le32toh(hscb->SG_pointer)));
	printf("	sg_addr:%lx sg_len:%lu\n",
		(unsigned long)(le32toh(scb->sg_list[0].addr)),
		(unsigned long)(le32toh(scb->sg_list[0].len)));
	printf("	cdb:%x %x %x %x %x %x %x %x %x %x %x %x\n",
		hscb->cmdstore[0], hscb->cmdstore[1], hscb->cmdstore[2],
		hscb->cmdstore[3], hscb->cmdstore[4], hscb->cmdstore[5],
		hscb->cmdstore[6], hscb->cmdstore[7], hscb->cmdstore[8],
		hscb->cmdstore[9], hscb->cmdstore[10], hscb->cmdstore[11]);
}
#endif

static const struct {
        u_int8_t errno;
	const char *errmesg;
} hard_error[] = {
	{ ILLHADDR,	"Illegal Host Access" },
	{ ILLSADDR,	"Illegal Sequencer Address referrenced" },
	{ ILLOPCODE,	"Illegal Opcode in sequencer program" },
	{ SQPARERR,	"Sequencer Parity Error" },
	{ DPARERR,	"Data-path Parity Error" },
	{ MPARERR,	"Scratch or SCB Memory Parity Error" },
	{ PCIERRSTAT,	"PCI Error detected" },
	{ CIOPARERR,	"CIOBUS Parity Error" },
};
static const int num_errors = sizeof(hard_error)/sizeof(hard_error[0]);

static const struct {
        u_int8_t phase;
        u_int8_t mesg_out; /* Message response to parity errors */
	const char *phasemsg;
} phase_table[] = {
	{ P_DATAOUT,	MSG_NOOP,		"in Data-out phase"	},
	{ P_DATAIN,	MSG_INITIATOR_DET_ERR,	"in Data-in phase"	},
	{ P_COMMAND,	MSG_NOOP,		"in Command phase"	},
	{ P_MESGOUT,	MSG_NOOP,		"in Message-out phase"	},
	{ P_STATUS,	MSG_INITIATOR_DET_ERR,	"in Status phase"	},
	{ P_MESGIN,	MSG_PARITY_ERROR,	"in Message-in phase"	},
	{ P_BUSFREE,	MSG_NOOP,		"while idle"		},
	{ 0,		MSG_NOOP,		"in unknown phase"	}
};
static const int num_phases = (sizeof(phase_table)/sizeof(phase_table[0])) - 1;

/*
 * Valid SCSIRATE values.  (p. 3-17)
 * Provides a mapping of transfer periods in ns to the proper value to
 * stick in the scsiscfr reg to use that transfer rate.
 */
#define AHC_SYNCRATE_DT		0
#define AHC_SYNCRATE_ULTRA2	1
#define AHC_SYNCRATE_ULTRA	3
#define AHC_SYNCRATE_FAST	6
static const struct ahc_syncrate ahc_syncrates[] = {
      /* ultra2    fast/ultra  period     rate */
	{ 0x42,      0x000,      9,      "80.0" },
	{ 0x03,      0x000,     10,      "40.0" },
	{ 0x04,      0x000,     11,      "33.0" },
	{ 0x05,      0x100,     12,      "20.0" },
	{ 0x06,      0x110,     15,      "16.0" },
	{ 0x07,      0x120,     18,      "13.4" },
	{ 0x08,      0x000,     25,      "10.0" },
	{ 0x19,      0x010,     31,      "8.0"  },
	{ 0x1a,      0x020,     37,      "6.67" },
	{ 0x1b,      0x030,     43,      "5.7"  },
	{ 0x1c,      0x040,     50,      "5.0"  },
	{ 0x00,      0x050,     56,      "4.4"  },
	{ 0x00,      0x060,     62,      "4.0"  },
	{ 0x00,      0x070,     68,      "3.6"  },
	{ 0x00,      0x000,      0,      NULL   }
};

/*
 * Allocate a controller structure for a new device and initialize it.
 */
int
ahc_alloc(struct ahc_softc *ahc, bus_space_handle_t sh, bus_space_tag_t st,
	  bus_dma_tag_t parent_dmat, ahc_chip chip, ahc_feature features,
	  ahc_flag flags)
{
	struct scb_data *scb_data;

	scb_data = malloc(sizeof (struct scb_data), M_DEVBUF, M_NOWAIT);
	if (scb_data == NULL) {
		printf("%s: cannot malloc softc!\n", ahc_name(ahc));
		return -1;
	}
	memset(scb_data, 0, sizeof (struct scb_data));
	LIST_INIT(&ahc->pending_ccbs);
	ahc->tag = st;
	ahc->bsh = sh;
	ahc->parent_dmat = parent_dmat;
	ahc->chip = chip;
	ahc->features = features;
	ahc->flags = flags;
	ahc->scb_data = scb_data;

	ahc->unpause = (ahc_inb(ahc, HCNTRL) & IRQMS) | INTEN;
	/* The IRQMS bit is only valid on VL and EISA chips */
	if ((ahc->chip & AHC_PCI) != 0)
		ahc->unpause &= ~IRQMS;
	ahc->pause = ahc->unpause | PAUSE;
	return (0);
}

void
ahc_free(ahc)
	struct ahc_softc *ahc;
{
	ahcfiniscbdata(ahc);
	if (ahc->init_level != 0)
		ahc_freedmamem(ahc->parent_dmat, ahc->shared_data_size,
		    ahc->shared_data_dmamap, ahc->qoutfifo,
		    &ahc->shared_data_seg, ahc->shared_data_nseg);

	if (ahc->scb_data != NULL)
		free(ahc->scb_data, M_DEVBUF);
	if (ahc->bus_data != NULL)
		free(ahc->bus_data, M_DEVBUF);
	return;
}

static int
ahcinitscbdata(struct ahc_softc *ahc)
{
	struct scb_data *scb_data;
	int i;

	scb_data = ahc->scb_data;
	SLIST_INIT(&scb_data->free_scbs);
	SLIST_INIT(&scb_data->sg_maps);

	/* Allocate SCB resources */
	scb_data->scbarray =
	    (struct scb *)malloc(sizeof(struct scb) * AHC_SCB_MAX,
				 M_DEVBUF, M_NOWAIT);
	if (scb_data->scbarray == NULL)
		return (ENOMEM);
	memset(scb_data->scbarray, 0, sizeof(struct scb) * AHC_SCB_MAX);

	/* Determine the number of hardware SCBs and initialize them */

	scb_data->maxhscbs = ahc_probe_scbs(ahc);
	/* SCB 0 heads the free list */
	ahc_outb(ahc, FREE_SCBH, 0);
	for (i = 0; i < ahc->scb_data->maxhscbs; i++) {
		ahc_outb(ahc, SCBPTR, i);

		/* Clear the control byte. */
		ahc_outb(ahc, SCB_CONTROL, 0);

		/* Set the next pointer */
		ahc_outb(ahc, SCB_NEXT, i+1);

		/* Make the tag number invalid */
		ahc_outb(ahc, SCB_TAG, SCB_LIST_NULL);
	}

	/* Make sure that the last SCB terminates the free list */
	ahc_outb(ahc, SCBPTR, i-1);
	ahc_outb(ahc, SCB_NEXT, SCB_LIST_NULL);

	/* Ensure we clear the 0 SCB's control byte. */
	ahc_outb(ahc, SCBPTR, 0);
	ahc_outb(ahc, SCB_CONTROL, 0);

	scb_data->maxhscbs = i;

	if (ahc->scb_data->maxhscbs == 0)
		panic("%s: No SCB space found", ahc_name(ahc));

	/*
	 * Create our DMA tags.  These tags define the kinds of device
	 * accessible memory allocations and memory mappings we will
	 * need to perform during normal operation.
	 *
	 * Unless we need to further restrict the allocation, we rely
	 * on the restrictions of the parent dmat, hence the common
	 * use of MAXADDR and MAXSIZE.
	 */

	if (ahc_createdmamem(ahc->parent_dmat,
	    AHC_SCB_MAX * sizeof(struct hardware_scb), ahc->sc_dmaflags,
	    &scb_data->hscb_dmamap,
	    (caddr_t *)&scb_data->hscbs, &scb_data->hscb_busaddr,
	    &scb_data->hscb_seg, &scb_data->hscb_nseg, ahc_name(ahc),
	    "hardware SCB structures") < 0)
		goto error_exit;

	scb_data->init_level++;

	if (ahc_createdmamem(ahc->parent_dmat,
	    AHC_SCB_MAX * sizeof(struct scsipi_sense_data), ahc->sc_dmaflags,
	    &scb_data->sense_dmamap, (caddr_t *)&scb_data->sense,
	    &scb_data->sense_busaddr, &scb_data->sense_seg,
	    &scb_data->sense_nseg, ahc_name(ahc), "sense buffers") < 0)
		goto error_exit;

	scb_data->init_level++;

	/* Perform initial CCB allocation */
	memset(scb_data->hscbs, 0, AHC_SCB_MAX * sizeof(struct hardware_scb));
	ahcallocscbs(ahc);

	if (scb_data->numscbs == 0) {
		printf("%s: ahc_init_scb_data - "
		       "Unable to allocate initial scbs\n",
		       ahc_name(ahc));
		goto error_exit;
	}

	scb_data->init_level++;

	/*
         * Note that we were successfull
         */
        return 0;

error_exit:

	return ENOMEM;
}

static void
ahcfiniscbdata(struct ahc_softc *ahc)
{
	struct scb_data *scb_data;

	scb_data = ahc->scb_data;

	switch (scb_data->init_level) {
	default:
	case 3:
	{
		struct sg_map_node *sg_map;

		while ((sg_map = SLIST_FIRST(&scb_data->sg_maps))!= NULL) {
			SLIST_REMOVE_HEAD(&scb_data->sg_maps, links);
			ahc_freedmamem(ahc->parent_dmat, PAGE_SIZE,
			    sg_map->sg_dmamap, (caddr_t)sg_map->sg_vaddr,
			    &sg_map->sg_dmasegs, sg_map->sg_nseg);
			free(sg_map, M_DEVBUF);
		}
	}
	/*FALLTHROUGH*/
	case 2:
		ahc_freedmamem(ahc->parent_dmat,
		    AHC_SCB_MAX * sizeof(struct scsipi_sense_data),
		    scb_data->sense_dmamap, (caddr_t)scb_data->sense,
		    &scb_data->sense_seg, scb_data->sense_nseg);
	/*FALLTHROUGH*/
	case 1:
		ahc_freedmamem(ahc->parent_dmat,
		    AHC_SCB_MAX * sizeof(struct hardware_scb),
		    scb_data->hscb_dmamap, (caddr_t)scb_data->hscbs,
		    &scb_data->hscb_seg, scb_data->hscb_nseg);
	/*FALLTHROUGH*/
	}
	if (scb_data->scbarray != NULL)
		free(scb_data->scbarray, M_DEVBUF);
}

int
ahc_reset(struct ahc_softc *ahc)
{
	u_int	sblkctl;
	int	wait;

#ifdef AHC_DUMP_SEQ
	if (ahc->init_level == 0)
		ahc_dumpseq(ahc);
#endif
	ahc_outb(ahc, HCNTRL, CHIPRST | ahc->pause);
	/*
	 * Ensure that the reset has finished
	 */
	wait = 1000;
	do {
		DELAY(1000);
	} while (--wait && !(ahc_inb(ahc, HCNTRL) & CHIPRSTACK));

	if (wait == 0) {
		printf("%s: WARNING - Failed chip reset!  "
		       "Trying to initialize anyway.\n", ahc_name(ahc));
	}
	ahc_outb(ahc, HCNTRL, ahc->pause);

	/* Determine channel configuration */
	sblkctl = ahc_inb(ahc, SBLKCTL) & (SELBUSB|SELWIDE);
	/* No Twin Channel PCI cards */
	if ((ahc->chip & AHC_PCI) != 0)
		sblkctl &= ~SELBUSB;
	switch (sblkctl) {
	case 0:
		/* Single Narrow Channel */
		break;
	case 2:
		/* Wide Channel */
		ahc->features |= AHC_WIDE;
		break;
	case 8:
		/* Twin Channel */
		ahc->features |= AHC_TWIN;
		break;
	default:
		printf(" Unsupported adapter type.  Ignoring\n");
		return(-1);
	}

	return (0);
}

/*
 * Called when we have an active connection to a target on the bus,
 * this function finds the nearest syncrate to the input period limited
 * by the capabilities of the bus connectivity of the target.
 */
static const struct ahc_syncrate *
ahc_devlimited_syncrate(struct ahc_softc *ahc, u_int *period) {
	u_int	maxsync;

	if ((ahc->features & AHC_ULTRA2) != 0) {
		if ((ahc_inb(ahc, SBLKCTL) & ENAB40) != 0
		 && (ahc_inb(ahc, SSTAT2) & EXP_ACTIVE) == 0) {
			maxsync = AHC_SYNCRATE_ULTRA2;
		} else {
			maxsync = AHC_SYNCRATE_ULTRA;
		}
	} else if ((ahc->features & AHC_ULTRA) != 0) {
		maxsync = AHC_SYNCRATE_ULTRA;
	} else {
		maxsync = AHC_SYNCRATE_FAST;
	}
	return (ahc_find_syncrate(ahc, period, maxsync));
}

/*
 * Look up the valid period to SCSIRATE conversion in our table.
 * Return the period and offset that should be sent to the target
 * if this was the beginning of an SDTR.
 */
static const struct ahc_syncrate *
ahc_find_syncrate(struct ahc_softc *ahc, u_int *period, u_int maxsync)
{
	const struct ahc_syncrate *syncrate;

	syncrate = &ahc_syncrates[maxsync];
	while ((syncrate->rate != NULL)
	    && ((ahc->features & AHC_ULTRA2) == 0
	     || (syncrate->sxfr_u2 != 0))) {

		if (*period <= syncrate->period) {
			/*
			 * When responding to a target that requests
			 * sync, the requested rate may fall between
			 * two rates that we can output, but still be
			 * a rate that we can receive.  Because of this,
			 * we want to respond to the target with
			 * the same rate that it sent to us even
			 * if the period we use to send data to it
			 * is lower.  Only lower the response period
			 * if we must.
			 */
			if (syncrate == &ahc_syncrates[maxsync])
				*period = syncrate->period;
			break;
		}
		syncrate++;
	}

	if ((*period == 0)
	 || (syncrate->rate == NULL)
	 || ((ahc->features & AHC_ULTRA2) != 0
	  && (syncrate->sxfr_u2 == 0))) {
		/* Use asynchronous transfers. */
		*period = 0;
		syncrate = NULL;
	}
	return (syncrate);
}

static u_int
ahc_find_period(struct ahc_softc *ahc, u_int scsirate, u_int maxsync)
{
	const struct ahc_syncrate *syncrate;

	if ((ahc->features & AHC_ULTRA2) != 0)
		scsirate &= SXFR_ULTRA2;
	else
		scsirate &= SXFR;

	syncrate = &ahc_syncrates[maxsync];
	while (syncrate->rate != NULL) {

		if ((ahc->features & AHC_ULTRA2) != 0) {
			if (syncrate->sxfr_u2 == 0)
				break;
			else if (scsirate == (syncrate->sxfr_u2 & SXFR_ULTRA2))
				return (syncrate->period);
		} else if (scsirate == (syncrate->sxfr & SXFR)) {
				return (syncrate->period);
		}
		syncrate++;
	}
	return (0); /* async */
}

static void
ahc_validate_offset(struct ahc_softc *ahc, const struct ahc_syncrate *syncrate,
		    u_int *offset, int wide)
{
	u_int maxoffset;

	/* Limit offset to what we can do */
	if (syncrate == NULL) {
		maxoffset = 0;
	} else if ((ahc->features & AHC_ULTRA2) != 0) {
		maxoffset = MAX_OFFSET_ULTRA2;
	} else {
		if (wide)
			maxoffset = MAX_OFFSET_16BIT;
		else
			maxoffset = MAX_OFFSET_8BIT;
	}
	*offset = MIN(*offset, maxoffset);
}

static void
ahc_update_target_msg_request(struct ahc_softc *ahc,
			      struct ahc_devinfo *devinfo,
			      struct ahc_initiator_tinfo *tinfo,
			      int force, int paused)
{
	u_int targ_msg_req_orig;

	targ_msg_req_orig = ahc->targ_msg_req;
	if (tinfo->current.period != tinfo->goal.period
	 || tinfo->current.width != tinfo->goal.width
	 || tinfo->current.offset != tinfo->goal.offset
	 || (force
	  && (tinfo->goal.period != 0
	   || tinfo->goal.width != MSG_EXT_WDTR_BUS_8_BIT))) {
		ahc->targ_msg_req |= devinfo->target_mask;
	} else {
		ahc->targ_msg_req &= ~devinfo->target_mask;
	}

	if (ahc->targ_msg_req != targ_msg_req_orig) {
		/* Update the message request bit for this target */
		if ((ahc->features & AHC_HS_MAILBOX) != 0) {
			if (paused) {
				ahc_outb(ahc, TARGET_MSG_REQUEST,
					 ahc->targ_msg_req & 0xFF);
				ahc_outb(ahc, TARGET_MSG_REQUEST + 1,
					 (ahc->targ_msg_req >> 8) & 0xFF);
			} else {
				ahc_outb(ahc, HS_MAILBOX,
					 0x01 << HOST_MAILBOX_SHIFT);
			}
		} else {
			if (!paused)
				pause_sequencer(ahc);

			ahc_outb(ahc, TARGET_MSG_REQUEST,
				 ahc->targ_msg_req & 0xFF);
			ahc_outb(ahc, TARGET_MSG_REQUEST + 1,
				 (ahc->targ_msg_req >> 8) & 0xFF);

			if (!paused)
				unpause_sequencer(ahc);
		}
	}
}

static void
ahc_set_syncrate(struct ahc_softc *ahc, struct ahc_devinfo *devinfo,
		 const struct ahc_syncrate *syncrate,
		 u_int period, u_int offset, u_int type, int paused, int done)
{
	struct	ahc_initiator_tinfo *tinfo;
	struct	tmode_tstate *tstate;
	u_int	old_period;
	u_int	old_offset;
	int	active = (type & AHC_TRANS_ACTIVE) == AHC_TRANS_ACTIVE;

	if (syncrate == NULL) {
		period = 0;
		offset = 0;
	}

	tinfo = ahc_fetch_transinfo(ahc, devinfo->channel, devinfo->our_scsiid,
				    devinfo->target, &tstate);
	old_period = tinfo->current.period;
	old_offset = tinfo->current.offset;

	if ((type & AHC_TRANS_CUR) != 0
	 && (old_period != period || old_offset != offset)) {
		u_int	scsirate;

		scsirate = tinfo->scsirate;
		if ((ahc->features & AHC_ULTRA2) != 0) {

			/* XXX */
			/* Force single edge until DT is fully implemented */
			scsirate &= ~(SXFR_ULTRA2|SINGLE_EDGE|ENABLE_CRC);
			if (syncrate != NULL)
				scsirate |= syncrate->sxfr_u2|SINGLE_EDGE;

			if (active)
				ahc_outb(ahc, SCSIOFFSET, offset);
		} else {

			scsirate &= ~(SXFR|SOFS);
			/*
			 * Ensure Ultra mode is set properly for
			 * this target.
			 */
			tstate->ultraenb &= ~devinfo->target_mask;
			if (syncrate != NULL) {
				if (syncrate->sxfr & ULTRA_SXFR) {
					tstate->ultraenb |=
						devinfo->target_mask;
				}
				scsirate |= syncrate->sxfr & SXFR;
				scsirate |= offset & SOFS;
			}
			if (active) {
				u_int sxfrctl0;

				sxfrctl0 = ahc_inb(ahc, SXFRCTL0);
				sxfrctl0 &= ~FAST20;
				if (tstate->ultraenb & devinfo->target_mask)
					sxfrctl0 |= FAST20;
				ahc_outb(ahc, SXFRCTL0, sxfrctl0);
			}
		}
		if (active)
			ahc_outb(ahc, SCSIRATE, scsirate);

		tinfo->scsirate = scsirate;
		tinfo->current.period = period;
		tinfo->current.offset = offset;

		/* Update the syncrates in any pending scbs */
		ahc_update_pending_syncrates(ahc);
	}

	if ((type & AHC_TRANS_GOAL) != 0) {
		tinfo->goal.period = period;
		tinfo->goal.offset = offset;
	}

	if ((type & AHC_TRANS_USER) != 0) {
		tinfo->user.period = period;
		tinfo->user.offset = offset;
	}

	ahc_update_target_msg_request(ahc, devinfo, tinfo,
				      /*force*/FALSE,
				      paused);
}

static void
ahc_set_width(struct ahc_softc *ahc, struct ahc_devinfo *devinfo,
	      u_int width, u_int type, int paused, int done)
{
	struct ahc_initiator_tinfo *tinfo;
	struct tmode_tstate *tstate;
	u_int  oldwidth;
	int    active = (type & AHC_TRANS_ACTIVE) == AHC_TRANS_ACTIVE;

	tinfo = ahc_fetch_transinfo(ahc, devinfo->channel, devinfo->our_scsiid,
				    devinfo->target, &tstate);
	oldwidth = tinfo->current.width;

	if ((type & AHC_TRANS_CUR) != 0 && oldwidth != width) {
		u_int	scsirate;

		scsirate =  tinfo->scsirate;
		scsirate &= ~WIDEXFER;
		if (width == MSG_EXT_WDTR_BUS_16_BIT)
			scsirate |= WIDEXFER;

		tinfo->scsirate = scsirate;

		if (active)
			ahc_outb(ahc, SCSIRATE, scsirate);

		tinfo->current.width = width;
	}

	if ((type & AHC_TRANS_GOAL) != 0)
		tinfo->goal.width = width;
	if ((type & AHC_TRANS_USER) != 0)
		tinfo->user.width = width;

	ahc_update_target_msg_request(ahc, devinfo, tinfo,
				      /*force*/FALSE, paused);
}

static void
ahc_set_tags(struct ahc_softc *ahc, struct ahc_devinfo *devinfo, int enable)
{
	struct ahc_initiator_tinfo *tinfo;
	struct tmode_tstate *tstate;

	tinfo = ahc_fetch_transinfo(ahc, devinfo->channel, devinfo->our_scsiid,
				    devinfo->target, &tstate);

	if (enable) {
		tstate->tagenable |= devinfo->target_mask;
	} else {
		tstate->tagenable &= ~devinfo->target_mask;
		tstate->tagdisable |= devinfo->target_mask;
	}
}

static void
ahc_update_xfer_mode(struct ahc_softc *ahc, struct ahc_devinfo *devinfo)
{
	struct scsipi_xfer_mode xm;
	struct ahc_initiator_tinfo *tinfo;
	struct tmode_tstate *tstate;

	if (ahc->inited_targets[devinfo->target] != 2)
		return;

	tinfo = ahc_fetch_transinfo(ahc, devinfo->channel, devinfo->our_scsiid,
				    devinfo->target, &tstate);

	xm.xm_target = devinfo->target;
	xm.xm_mode = 0;
	xm.xm_period = tinfo->current.period;
	xm.xm_offset = tinfo->current.offset;
	if (tinfo->current.width == 1) 
		xm.xm_mode |= PERIPH_CAP_WIDE16;
	if (tinfo->current.period)
		xm.xm_mode |= PERIPH_CAP_SYNC;
	if (tstate->tagenable & devinfo->target_mask)
		xm.xm_mode |= PERIPH_CAP_TQING;
	scsipi_async_event(
	    devinfo->channel == 'B' ? &ahc->sc_channel_b : &ahc->sc_channel,
	    ASYNC_EVENT_XFER_MODE, &xm);
}

/*
 * Attach all the sub-devices we can find
 */
int
ahc_attach(struct ahc_softc *ahc)
{
	ahc->sc_adapter.adapt_dev = &ahc->sc_dev;
	ahc->sc_adapter.adapt_nchannels = (ahc->features & AHC_TWIN) ? 2 : 1;
	if (ahc->flags & AHC_PAGESCBS) {
		ahc->sc_adapter.adapt_openings = AHC_SCB_MAX;
		ahc->sc_adapter.adapt_max_periph = 16;
	} else {
		ahc->sc_adapter.adapt_openings =  ahc->scb_data->maxhscbs;
		if (ahc->scb_data->maxhscbs >= 16)
			ahc->sc_adapter.adapt_max_periph = 16;
		else
			ahc->sc_adapter.adapt_max_periph = 4;
	}
	ahc->sc_adapter.adapt_ioctl = ahc_ioctl;
	ahc->sc_adapter.adapt_minphys = ahcminphys;
	ahc->sc_adapter.adapt_request = ahc_action;

	ahc->sc_channel.chan_adapter = &ahc->sc_adapter;
	ahc->sc_channel.chan_bustype = &scsi_bustype;
	ahc->sc_channel.chan_channel = 0;
	ahc->sc_channel.chan_ntargets = (ahc->features & AHC_WIDE) ? 16 : 8;
	ahc->sc_channel.chan_nluns = 8;
	ahc->sc_channel.chan_id = ahc->our_id;
	
	if (ahc->features & AHC_TWIN) {
		ahc->sc_channel_b = ahc->sc_channel;
		ahc->sc_channel_b.chan_id = ahc->our_id_b;
		ahc->sc_channel_b.chan_channel = 1;
	}

	if ((ahc->flags & AHC_CHANNEL_B_PRIMARY) == 0) {
		config_found((void *)ahc, &ahc->sc_channel, scsiprint);
		if (ahc->features & AHC_TWIN)
			config_found((void *)ahc, &ahc->sc_channel_b,
			    scsiprint);
	} else {
		config_found((void *)ahc, &ahc->sc_channel_b, scsiprint);
		config_found((void *)ahc, &ahc->sc_channel, scsiprint);
	}
	return 1;
}

static void
ahc_fetch_devinfo(struct ahc_softc *ahc, struct ahc_devinfo *devinfo)
{
	u_int	saved_tcl;
	role_t	role;
	int	our_id;

	if (ahc_inb(ahc, SSTAT0) & TARGET)
		role = ROLE_TARGET;
	else
		role = ROLE_INITIATOR;

	if (role == ROLE_TARGET
	 && (ahc->features & AHC_MULTI_TID) != 0
	 && (ahc_inb(ahc, SEQ_FLAGS) & CMDPHASE_PENDING) != 0) {
		/* We were selected, so pull our id from TARGIDIN */
		our_id = ahc_inb(ahc, TARGIDIN) & OID;
	} else if ((ahc->features & AHC_ULTRA2) != 0)
		our_id = ahc_inb(ahc, SCSIID_ULTRA2) & OID;
	else
		our_id = ahc_inb(ahc, SCSIID) & OID;

	saved_tcl = ahc_inb(ahc, SAVED_TCL);
	ahc_compile_devinfo(devinfo, our_id, TCL_TARGET(saved_tcl),
			    TCL_LUN(saved_tcl), TCL_CHANNEL(ahc, saved_tcl),
			    role);
}

static void
ahc_compile_devinfo(struct ahc_devinfo *devinfo, u_int our_id, u_int target,
		    u_int lun, char channel, role_t role)
{
	devinfo->our_scsiid = our_id;
	devinfo->target = target;
	devinfo->lun = lun;
	devinfo->target_offset = target;
	devinfo->channel = channel;
	devinfo->role = role;
	if (channel == 'B')
		devinfo->target_offset += 8;
	devinfo->target_mask = (0x01 << devinfo->target_offset);
}

/*
 * Catch an interrupt from the adapter
 */
int
ahc_intr(void *arg)
{
	struct	ahc_softc *ahc;
	u_int	intstat;

	ahc = (struct ahc_softc *)arg;

	intstat = ahc_inb(ahc, INTSTAT);

	/*
	 * Any interrupts to process?
	 */
	if ((intstat & INT_PEND) == 0) {
		if (ahc->bus_intr && ahc->bus_intr(ahc)) {
#ifdef AHC_DEBUG
			printf("%s: bus intr: CCHADDR %x HADDR %x SEQADDR %x\n",
			    ahc_name(ahc),
			    ahc_inb(ahc, CCHADDR) |
			    (ahc_inb(ahc, CCHADDR+1) << 8)
			    | (ahc_inb(ahc, CCHADDR+2) << 16)
			    | (ahc_inb(ahc, CCHADDR+3) << 24),
			    ahc_inb(ahc, HADDR) | (ahc_inb(ahc, HADDR+1) << 8)
			    | (ahc_inb(ahc, HADDR+2) << 16)
			    | (ahc_inb(ahc, HADDR+3) << 24),
			    ahc_inb(ahc, SEQADDR0) |
			    (ahc_inb(ahc, SEQADDR1) << 8));
#endif
			return 1;
		}
		return 0;
	}

#ifdef AHC_DEBUG
	if (ahc_debug & AHC_SHOWINTR) {
		printf("%s: intstat %x\n", ahc_name(ahc), intstat);
	}
#endif

	if (intstat & CMDCMPLT) {
		ahc_outb(ahc, CLRINT, CLRCMDINT);
		ahc_run_qoutfifo(ahc);
	}
	if (intstat & BRKADRINT) {
		/*
		 * We upset the sequencer :-(
		 * Lookup the error message
		 */
		int i, error, num_errors;

		error = ahc_inb(ahc, ERROR);
		num_errors =  sizeof(hard_error)/sizeof(hard_error[0]);
		for (i = 0; error != 1 && i < num_errors; i++)
			error >>= 1;
		panic("%s: brkadrint, %s at seqaddr = 0x%x\n",
		      ahc_name(ahc), hard_error[i].errmesg,
		      ahc_inb(ahc, SEQADDR0) |
		      (ahc_inb(ahc, SEQADDR1) << 8));

		/* Tell everyone that this HBA is no longer available */
		ahc_abort_scbs(ahc, AHC_TARGET_WILDCARD, ALL_CHANNELS,
			       AHC_LUN_WILDCARD, SCB_LIST_NULL, ROLE_UNKNOWN,
			       XS_DRIVER_STUFFUP);
	}
	if (intstat & SEQINT)
		ahc_handle_seqint(ahc, intstat);

	if (intstat & SCSIINT)
		ahc_handle_scsiint(ahc, intstat);

	return 1;
}

static struct tmode_tstate *
ahc_alloc_tstate(struct ahc_softc *ahc, u_int scsi_id, char channel)
{
	struct tmode_tstate *master_tstate;
	struct tmode_tstate *tstate;
	int i, s;

	master_tstate = ahc->enabled_targets[ahc->our_id];
	if (channel == 'B') {
		scsi_id += 8;
		master_tstate = ahc->enabled_targets[ahc->our_id_b + 8];
	}
	if (ahc->enabled_targets[scsi_id] != NULL
	 && ahc->enabled_targets[scsi_id] != master_tstate)
		panic("%s: ahc_alloc_tstate - Target already allocated",
		      ahc_name(ahc));
	tstate = malloc(sizeof(*tstate), M_DEVBUF, M_NOWAIT);
	if (tstate == NULL)
		return (NULL);

	/*
	 * If we have allocated a master tstate, copy user settings from
	 * the master tstate (taken from SRAM or the EEPROM) for this
	 * channel, but reset our current and goal settings to async/narrow
	 * until an initiator talks to us.
	 */
	if (master_tstate != NULL) {
		memcpy(tstate, master_tstate, sizeof(*tstate));
		tstate->ultraenb = 0;
		for (i = 0; i < 16; i++) {
			memset(&tstate->transinfo[i].current, 0,
			      sizeof(tstate->transinfo[i].current));
			memset(&tstate->transinfo[i].goal, 0,
			      sizeof(tstate->transinfo[i].goal));
		}
	} else
		memset(tstate, 0, sizeof(*tstate));
	s = splbio();
	ahc->enabled_targets[scsi_id] = tstate;
	splx(s);
	return (tstate);
}

#if UNUSED
static void
ahc_free_tstate(struct ahc_softc *ahc, u_int scsi_id, char channel, int force)
{
	struct tmode_tstate *tstate;

	/* Don't clean up the entry for our initiator role */
	if ((ahc->flags & AHC_INITIATORMODE) != 0
	 && ((channel == 'B' && scsi_id == ahc->our_id_b)
	  || (channel == 'A' && scsi_id == ahc->our_id))
	 && force == FALSE)
		return;

	if (channel == 'B')
		scsi_id += 8;
	tstate = ahc->enabled_targets[scsi_id];
	if (tstate != NULL)
		free(tstate, M_DEVBUF);
	ahc->enabled_targets[scsi_id] = NULL;
}
#endif

static void
ahc_handle_seqint(struct ahc_softc *ahc, u_int intstat)
{
	struct scb *scb;
	struct ahc_devinfo devinfo;

	ahc_fetch_devinfo(ahc, &devinfo);

	/*
	 * Clear the upper byte that holds SEQINT status
	 * codes and clear the SEQINT bit. We will unpause
	 * the sequencer, if appropriate, after servicing
	 * the request.
	 */
	ahc_outb(ahc, CLRINT, CLRSEQINT);
	switch (intstat & SEQINT_MASK) {
	case NO_MATCH:
	{
		/* Ensure we don't leave the selection hardware on */
		ahc_outb(ahc, SCSISEQ,
			 ahc_inb(ahc, SCSISEQ) & (ENSELI|ENRSELI|ENAUTOATNP));

		printf("%s:%c:%d: no active SCB for reconnecting "
		       "target - issuing BUS DEVICE RESET\n",
		       ahc_name(ahc), devinfo.channel, devinfo.target);
		printf("SAVED_TCL == 0x%x, ARG_1 == 0x%x, SEQ_FLAGS == 0x%x\n",
		       ahc_inb(ahc, SAVED_TCL), ahc_inb(ahc, ARG_1),
		       ahc_inb(ahc, SEQ_FLAGS));
		ahc->msgout_buf[0] = MSG_BUS_DEV_RESET;
		ahc->msgout_len = 1;
		ahc->msgout_index = 0;
		ahc->msg_type = MSG_TYPE_INITIATOR_MSGOUT;
		ahc_outb(ahc, MSG_OUT, HOST_MSG);
		ahc_outb(ahc, SCSISIGO, ahc_inb(ahc, LASTPHASE) | ATNO);
		break;
	}
	case UPDATE_TMSG_REQ:
		ahc_outb(ahc, TARGET_MSG_REQUEST, ahc->targ_msg_req & 0xFF);
		ahc_outb(ahc, TARGET_MSG_REQUEST + 1,
			 (ahc->targ_msg_req >> 8) & 0xFF);
		ahc_outb(ahc, HS_MAILBOX, 0);
		break;
	case SEND_REJECT:
	{
		u_int rejbyte = ahc_inb(ahc, ACCUM);
		printf("%s:%c:%d: Warning - unknown message received from "
		       "target (0x%x).  Rejecting\n",
		       ahc_name(ahc), devinfo.channel, devinfo.target, rejbyte);
		break;
	}
	case NO_IDENT:
	{
		/*
		 * The reconnecting target either did not send an identify
		 * message, or did, but we didn't find and SCB to match and
		 * before it could respond to our ATN/abort, it hit a dataphase.
		 * The only safe thing to do is to blow it away with a bus
		 * reset.
		 */
		int found;

		printf("%s:%c:%d: Target did not send an IDENTIFY message. "
		       "LASTPHASE = 0x%x, SAVED_TCL == 0x%x\n",
		       ahc_name(ahc), devinfo.channel, devinfo.target,
		       ahc_inb(ahc, LASTPHASE), ahc_inb(ahc, SAVED_TCL));
		found = ahc_reset_channel(ahc, devinfo.channel,
					  /*initiate reset*/TRUE);
		printf("%s: Issued Channel %c Bus Reset. "
		       "%d SCBs aborted\n", ahc_name(ahc), devinfo.channel,
		       found);
		return;
	}
	case BAD_PHASE:
	{
		u_int lastphase;

		lastphase = ahc_inb(ahc, LASTPHASE);
		if (lastphase == P_BUSFREE) {
			printf("%s:%c:%d: Missed busfree.  Curphase = 0x%x\n",
			       ahc_name(ahc), devinfo.channel, devinfo.target,
			       ahc_inb(ahc, SCSISIGI));
			restart_sequencer(ahc);
			return;
		} else {
			printf("%s:%c:%d: unknown scsi bus phase %x.  "
			       "Attempting to continue\n",
			       ahc_name(ahc), devinfo.channel, devinfo.target,
			       ahc_inb(ahc, SCSISIGI));
		}
		break;
	}
	case BAD_STATUS:
	{
		u_int  scb_index;
		struct hardware_scb *hscb;
		struct scsipi_xfer *xs;
		/*
		 * The sequencer will notify us when a command
		 * has an error that would be of interest to
		 * the kernel.  This allows us to leave the sequencer
		 * running in the common case of command completes
		 * without error.  The sequencer will already have
		 * dma'd the SCB back up to us, so we can reference
		 * the in kernel copy directly.
		 */
		scb_index = ahc_inb(ahc, SCB_TAG);
		scb = &ahc->scb_data->scbarray[scb_index];

		/* ahc_print_scb(scb); */

		/*
		 * Set the default return value to 0 (don't
		 * send sense).  The sense code will change
		 * this if needed.
		 */
		ahc_outb(ahc, RETURN_1, 0);
		if (!(scb_index < ahc->scb_data->numscbs
		   && (scb->flags & SCB_ACTIVE) != 0)) {
			printf("%s:%c:%d: ahc_intr - referenced scb "
			       "not valid during seqint 0x%x scb(%d)\n",
			       ahc_name(ahc), devinfo.channel,
			       devinfo.target, intstat, scb_index);
			goto unpause;
		}

		hscb = scb->hscb;
		xs = scb->xs;

		/* Don't want to clobber the original sense code */
		if ((scb->flags & SCB_SENSE) != 0) {
			/*
			 * Clear the SCB_SENSE Flag and have
			 * the sequencer do a normal command
			 * complete.
			 */
			scb->flags &= ~SCB_SENSE;
			ahcsetccbstatus(xs, XS_DRIVER_STUFFUP);
			break;
		}
		/* Freeze the queue unit the client sees the error. */
		ahc_freeze_devq(ahc, xs->xs_periph);
		ahc_freeze_ccb(scb);
		xs->status = hscb->status;
		switch (hscb->status) {
		case SCSI_STATUS_OK:
			printf("%s: Interrupted for status of 0???\n",
			       ahc_name(ahc));
			break;
		case SCSI_STATUS_CMD_TERMINATED:
		case SCSI_STATUS_CHECK_COND:
#if defined(AHC_DEBUG)
			if (ahc_debug & AHC_SHOWSENSE) {
				scsipi_printaddr(xs->xs_periph);
				printf("Check Status, resid %d datalen %d\n",
				    xs->resid, xs->datalen);
			}
#endif

			if (xs->error == XS_NOERROR &&
			    !(scb->flags & SCB_SENSE)) {
				struct ahc_dma_seg *sg;
				struct scsipi_sense *sc;
				struct ahc_initiator_tinfo *tinfo;
				struct tmode_tstate *tstate;

				sg = scb->sg_list;
				sc = (struct scsipi_sense *)(&hscb->cmdstore);
				/*
				 * Save off the residual if there is one.
				 */
				if (hscb->residual_SG_count != 0)
					ahc_calc_residual(scb);
				else
					xs->resid = 0;

#ifdef AHC_DEBUG
				if (ahc_debug & AHC_SHOWSENSE) {
					scsipi_printaddr(xs->xs_periph);
					printf("Sending Sense\n");
				}
#endif
				sg->addr = ahc->scb_data->sense_busaddr +
				   (hscb->tag*sizeof(struct scsipi_sense_data));
				sg->len = sizeof (struct scsipi_sense_data);

				sc->opcode = REQUEST_SENSE;
				sc->byte2 =  SCB_LUN(scb) << 5;
				sc->unused[0] = 0;
				sc->unused[1] = 0;
				sc->length = sg->len;
				sc->control = 0;

				/*
				 * Would be nice to preserve DISCENB here,
				 * but due to the way we page SCBs, we can't.
				 */
				hscb->control = 0;

				/*
				 * This request sense could be because the
				 * the device lost power or in some other
				 * way has lost our transfer negotiations.
				 * Renegotiate if appropriate.  Unit attention
				 * errors will be reported before any data
				 * phases occur.
				 */
				ahc_calc_residual(scb);
#if defined(AHC_DEBUG)
				if (ahc_debug & AHC_SHOWSENSE) {
					scsipi_printaddr(xs->xs_periph);
					printf("Sense: datalen %d resid %d"
					       "chan %d id %d targ %d\n",
					    xs->datalen, xs->resid,
					    devinfo.channel, devinfo.our_scsiid,
					    devinfo.target);
				}
#endif
				if (xs->datalen > 0 &&
				    xs->resid == xs->datalen) {
					tinfo = ahc_fetch_transinfo(ahc,
							    devinfo.channel,
							    devinfo.our_scsiid,
							    devinfo.target,
							    &tstate);
					ahc_update_target_msg_request(ahc,
							      &devinfo,
							      tinfo,
							      /*force*/TRUE,
							      /*paused*/TRUE);
				}
				hscb->status = 0;
				hscb->SG_count = 1;
				hscb->SG_pointer = scb->sg_list_phys;
				hscb->data = sg->addr;
				hscb->datalen = sg->len;
				hscb->cmdpointer = hscb->cmdstore_busaddr;
				hscb->cmdlen = sizeof(*sc);
				scb->sg_count = hscb->SG_count;
				ahc_swap_hscb(hscb);
				ahc_swap_sg(scb->sg_list);
				scb->flags |= SCB_SENSE;
				/*
				 * Ensure the target is busy since this
				 * will be an untagged request.
				 */
				ahc_busy_tcl(ahc, scb);
				ahc_outb(ahc, RETURN_1, SEND_SENSE);

				/*
				 * Ensure we have enough time to actually
				 * retrieve the sense.
				 */
				if (!(scb->xs->xs_control & XS_CTL_POLL)) {
					callout_reset(&scb->xs->xs_callout,
					    5 * hz, ahc_timeout, scb);
				}
			}
			break;
		case SCSI_STATUS_QUEUE_FULL:
		case SCSI_STATUS_BUSY:
			xs->error = XS_BUSY;
			break;
		}
		break;
	}
	case TRACE_POINT:
	{
		printf("SSTAT2 = 0x%x DFCNTRL = 0x%x\n", ahc_inb(ahc, SSTAT2),
		       ahc_inb(ahc, DFCNTRL));
		printf("SSTAT3 = 0x%x DSTATUS = 0x%x\n", ahc_inb(ahc, SSTAT3),
		       ahc_inb(ahc, DFSTATUS));
		printf("SSTAT0 = 0x%x, SCB_DATACNT = 0x%x\n",
		       ahc_inb(ahc, SSTAT0),
		       ahc_inb(ahc, SCB_DATACNT));
		break;
	}
	case HOST_MSG_LOOP:
	{
		/*
		 * The sequencer has encountered a message phase
		 * that requires host assistance for completion.
		 * While handling the message phase(s), we will be
		 * notified by the sequencer after each byte is
		 * transferred so we can track bus phases.
		 *
		 * If this is the first time we've seen a HOST_MSG_LOOP,
		 * initialize the state of the host message loop.
		 */
		if (ahc->msg_type == MSG_TYPE_NONE) {
			u_int bus_phase;

			bus_phase = ahc_inb(ahc, SCSISIGI) & PHASE_MASK;
			if (bus_phase != P_MESGIN
			 && bus_phase != P_MESGOUT) {
				printf("ahc_intr: HOST_MSG_LOOP bad "
				       "phase 0x%x\n",
				      bus_phase);
				/*
				 * Probably transitioned to bus free before
				 * we got here.  Just punt the message.
				 */
				ahc_clear_intstat(ahc);
				restart_sequencer(ahc);
			}

			if (devinfo.role == ROLE_INITIATOR) {
				struct scb *scb;
				u_int scb_index;

				scb_index = ahc_inb(ahc, SCB_TAG);
				scb = &ahc->scb_data->scbarray[scb_index];

				if (bus_phase == P_MESGOUT)
					ahc_setup_initiator_msgout(ahc,
								   &devinfo,
								   scb);
				else {
					ahc->msg_type =
					    MSG_TYPE_INITIATOR_MSGIN;
					ahc->msgin_index = 0;
				}
			} else {
				if (bus_phase == P_MESGOUT) {
					ahc->msg_type =
					    MSG_TYPE_TARGET_MSGOUT;
					ahc->msgin_index = 0;
				} else
					/* XXX Ever executed??? */
					ahc_setup_target_msgin(ahc, &devinfo);
			}
		}

		/* Pass a NULL path so that handlers generate their own */
		ahc_handle_message_phase(ahc, /*path*/NULL);
		break;
	}
	case PERR_DETECTED:
	{
		/*
		 * If we've cleared the parity error interrupt
		 * but the sequencer still believes that SCSIPERR
		 * is true, it must be that the parity error is
		 * for the currently presented byte on the bus,
		 * and we are not in a phase (data-in) where we will
		 * eventually ack this byte.  Ack the byte and
		 * throw it away in the hope that the target will
		 * take us to message out to deliver the appropriate
		 * error message.
		 */
		if ((intstat & SCSIINT) == 0
		 && (ahc_inb(ahc, SSTAT1) & SCSIPERR) != 0) {
			u_int curphase;

			/*
			 * The hardware will only let you ack bytes
			 * if the expected phase in SCSISIGO matches
			 * the current phase.  Make sure this is
			 * currently the case.
			 */
			curphase = ahc_inb(ahc, SCSISIGI) & PHASE_MASK;
			ahc_outb(ahc, LASTPHASE, curphase);
			ahc_outb(ahc, SCSISIGO, curphase);
			ahc_inb(ahc, SCSIDATL);
		}
		break;
	}
	case DATA_OVERRUN:
	{
		/*
		 * When the sequencer detects an overrun, it
		 * places the controller in "BITBUCKET" mode
		 * and allows the target to complete its transfer.
		 * Unfortunately, none of the counters get updated
		 * when the controller is in this mode, so we have
		 * no way of knowing how large the overrun was.
		 */
		u_int scbindex = ahc_inb(ahc, SCB_TAG);
		u_int lastphase = ahc_inb(ahc, LASTPHASE);
		int i;

		scb = &ahc->scb_data->scbarray[scbindex];
		for (i = 0; i < num_phases; i++) {
			if (lastphase == phase_table[i].phase)
				break;
		}
		scsipi_printaddr(scb->xs->xs_periph);
		printf("data overrun detected %s."
		       "  Tag == 0x%x.\n",
		       phase_table[i].phasemsg,
  		       scb->hscb->tag);
		scsipi_printaddr(scb->xs->xs_periph);
		printf("%s seen Data Phase.  Length = %d.  NumSGs = %d.\n",
		       ahc_inb(ahc, SEQ_FLAGS) & DPHASE ? "Have" : "Haven't",
		       scb->xs->datalen, scb->sg_count);
		if (scb->sg_count > 0) {
			for (i = 0; i < scb->sg_count; i++) {
				printf("sg[%d] - Addr 0x%x : Length %d\n",
				       i,
				       le32toh(scb->sg_list[i].addr),
				       le32toh(scb->sg_list[i].len));
			}
		}
		/*
		 * Set this and it will take affect when the
		 * target does a command complete.
		 */
		ahc_freeze_devq(ahc, scb->xs->xs_periph);
		ahcsetccbstatus(scb->xs, XS_DRIVER_STUFFUP);
		ahc_freeze_ccb(scb);
		break;
	}
	case TRACEPOINT:
	{
		printf("TRACEPOINT: RETURN_1 = %d\n", ahc_inb(ahc, RETURN_1));
		printf("TRACEPOINT: RETURN_2 = %d\n", ahc_inb(ahc, RETURN_2));
		printf("TRACEPOINT: ARG_1    = %d\n", ahc_inb(ahc, ARG_1));
		printf("TRACEPOINT: ARG_2    = %d\n", ahc_inb(ahc, ARG_2));
		printf("TRACEPOINT: CCHADDR =  %x\n",
		    ahc_inb(ahc, CCHADDR) | (ahc_inb(ahc, CCHADDR+1) << 8)
		    | (ahc_inb(ahc, CCHADDR+2) << 16)
		    | (ahc_inb(ahc, CCHADDR+3) << 24));
#if 0
		printf("SSTAT1 == 0x%x\n", ahc_inb(ahc, SSTAT1));
		printf("SSTAT0 == 0x%x\n", ahc_inb(ahc, SSTAT0));
		printf(", SCSISIGI == 0x%x\n", ahc_inb(ahc, SCSISIGI));
		printf("TRACEPOINT: CCHCNT = %d, SG_COUNT = %d\n",
		       ahc_inb(ahc, CCHCNT), ahc_inb(ahc, SG_COUNT));
		printf("TRACEPOINT: SCB_TAG = %d\n", ahc_inb(ahc, SCB_TAG));
		printf("TRACEPOINT1: CCHADDR = %d, CCHCNT = %d, SCBPTR = %d\n",
		       ahc_inb(ahc, CCHADDR)
		    | (ahc_inb(ahc, CCHADDR+1) << 8)
		    | (ahc_inb(ahc, CCHADDR+2) << 16)
		    | (ahc_inb(ahc, CCHADDR+3) << 24),
		       ahc_inb(ahc, CCHCNT)
		    | (ahc_inb(ahc, CCHCNT+1) << 8)
		    | (ahc_inb(ahc, CCHCNT+2) << 16),
		       ahc_inb(ahc, SCBPTR));
		printf("TRACEPOINT: WAITING_SCBH = %d\n", ahc_inb(ahc, WAITING_SCBH));
		printf("TRACEPOINT: SCB_TAG = %d\n", ahc_inb(ahc, SCB_TAG));
#if DDB > 0
		cpu_Debugger();
#endif
#endif
		break;
	}
#if NOT_YET
	/* XXX Fill these in later */
	case MESG_BUFFER_BUSY:
		break;
	case MSGIN_PHASEMIS:
		break;
#endif
	default:
		printf("ahc_intr: seqint, "
		       "intstat == 0x%x, scsisigi = 0x%x\n",
		       intstat, ahc_inb(ahc, SCSISIGI));
		break;
	}

unpause:
	/*
	 *  The sequencer is paused immediately on
	 *  a SEQINT, so we should restart it when
	 *  we're done.
	 */
	unpause_sequencer(ahc);
}

static void
ahc_handle_scsiint(struct ahc_softc *ahc, u_int intstat)
{
	u_int	scb_index;
	u_int	status;
	struct	scb *scb;
	char	cur_channel;
	char	intr_channel;

	if ((ahc->features & AHC_TWIN) != 0
	 && ((ahc_inb(ahc, SBLKCTL) & SELBUSB) != 0))
		cur_channel = 'B';
	else
		cur_channel = 'A';
	intr_channel = cur_channel;

	status = ahc_inb(ahc, SSTAT1);
	if (status == 0) {
		if ((ahc->features & AHC_TWIN) != 0) {
			/* Try the other channel */
		 	ahc_outb(ahc, SBLKCTL, ahc_inb(ahc, SBLKCTL) ^ SELBUSB);
			status = ahc_inb(ahc, SSTAT1);
		 	ahc_outb(ahc, SBLKCTL, ahc_inb(ahc, SBLKCTL) ^ SELBUSB);
			intr_channel = (cur_channel == 'A') ? 'B' : 'A';
		}
		if (status == 0) {
			printf("%s: Spurious SCSI interrupt\n", ahc_name(ahc));
			return;
		}
	}

	scb_index = ahc_inb(ahc, SCB_TAG);
	if (scb_index < ahc->scb_data->numscbs) {
		scb = &ahc->scb_data->scbarray[scb_index];
		if ((scb->flags & SCB_ACTIVE) == 0
		 || (ahc_inb(ahc, SEQ_FLAGS) & IDENTIFY_SEEN) == 0)
			scb = NULL;
	} else
		scb = NULL;

	if ((status & SCSIRSTI) != 0) {
		printf("%s: Someone reset channel %c\n",
			ahc_name(ahc), intr_channel);
		ahc_reset_channel(ahc, intr_channel, /* Initiate Reset */FALSE);
	} else if ((status & SCSIPERR) != 0) {
		/*
		 * Determine the bus phase and queue an appropriate message.
		 * SCSIPERR is latched true as soon as a parity error
		 * occurs.  If the sequencer acked the transfer that
		 * caused the parity error and the currently presented
		 * transfer on the bus has correct parity, SCSIPERR will
		 * be cleared by CLRSCSIPERR.  Use this to determine if
		 * we should look at the last phase the sequencer recorded,
		 * or the current phase presented on the bus.
		 */
		u_int mesg_out;
		u_int curphase;
		u_int errorphase;
		u_int lastphase;
		int   i;

		lastphase = ahc_inb(ahc, LASTPHASE);
		curphase = ahc_inb(ahc, SCSISIGI) & PHASE_MASK;
		ahc_outb(ahc, CLRSINT1, CLRSCSIPERR);
		/*
		 * For all phases save DATA, the sequencer won't
		 * automatically ack a byte that has a parity error
		 * in it.  So the only way that the current phase
		 * could be 'data-in' is if the parity error is for
		 * an already acked byte in the data phase.  During
		 * synchronous data-in transfers, we may actually
		 * ack bytes before latching the current phase in
		 * LASTPHASE, leading to the discrepancy between
		 * curphase and lastphase.
		 */
		if ((ahc_inb(ahc, SSTAT1) & SCSIPERR) != 0
		 || curphase == P_DATAIN)
			errorphase = curphase;
		else
			errorphase = lastphase;

		for (i = 0; i < num_phases; i++) {
			if (errorphase == phase_table[i].phase)
				break;
		}
		mesg_out = phase_table[i].mesg_out;
		if (scb != NULL)
			scsipi_printaddr(scb->xs->xs_periph);
		else
			printf("%s:%c:%d: ", ahc_name(ahc),
			       intr_channel,
			       TCL_TARGET(ahc_inb(ahc, SAVED_TCL)));

		printf("parity error detected %s. "
		       "SEQADDR(0x%x) SCSIRATE(0x%x)\n",
		       phase_table[i].phasemsg,
		       ahc_inb(ahc, SEQADDR0) | (ahc_inb(ahc, SEQADDR1) << 8),
		       ahc_inb(ahc, SCSIRATE));

		/*
		 * We've set the hardware to assert ATN if we
		 * get a parity error on "in" phases, so all we
		 * need to do is stuff the message buffer with
		 * the appropriate message.  "In" phases have set
		 * mesg_out to something other than MSG_NOP.
		 */
		if (mesg_out != MSG_NOOP) {
			if (ahc->msg_type != MSG_TYPE_NONE)
				ahc->send_msg_perror = TRUE;
			else
				ahc_outb(ahc, MSG_OUT, mesg_out);
		}
		ahc_outb(ahc, CLRINT, CLRSCSIINT);
		unpause_sequencer(ahc);
	} else if ((status & BUSFREE) != 0
		&& (ahc_inb(ahc, SIMODE1) & ENBUSFREE) != 0) {
		/*
		 * First look at what phase we were last in.
		 * If its message out, chances are pretty good
		 * that the busfree was in response to one of
		 * our abort requests.
		 */
		u_int lastphase = ahc_inb(ahc, LASTPHASE);
		u_int saved_tcl = ahc_inb(ahc, SAVED_TCL);
		u_int target = TCL_TARGET(saved_tcl);
		u_int initiator_role_id = TCL_SCSI_ID(ahc, saved_tcl);
		char channel = TCL_CHANNEL(ahc, saved_tcl);
		int printerror = 1;

		ahc_outb(ahc, SCSISEQ,
			 ahc_inb(ahc, SCSISEQ) & (ENSELI|ENRSELI|ENAUTOATNP));
		if (lastphase == P_MESGOUT) {
			u_int message;
			u_int tag;

			message = ahc->msgout_buf[ahc->msgout_index - 1];
			tag = SCB_LIST_NULL;
			switch (message) {
			case MSG_ABORT_TAG:
				tag = scb->hscb->tag;
				/* FALLTRHOUGH */
			case MSG_ABORT:
				scsipi_printaddr(scb->xs->xs_periph);
				printf("SCB %x - Abort %s Completed.\n",
				       scb->hscb->tag, tag == SCB_LIST_NULL ?
				       "" : "Tag");
				ahc_abort_scbs(ahc, target, channel,
					       TCL_LUN(saved_tcl), tag,
					       ROLE_INITIATOR,
					       XS_DRIVER_STUFFUP);
				printerror = 0;
				break;
			case MSG_BUS_DEV_RESET:
			{
				struct ahc_devinfo devinfo;

				if (scb != NULL &&
				    (scb->xs->xs_control & XS_CTL_RESET)
				 && ahc_match_scb(scb, target, channel,
						  TCL_LUN(saved_tcl),
						  SCB_LIST_NULL,
						  ROLE_INITIATOR)) {
					ahcsetccbstatus(scb->xs, XS_NOERROR);
				}
				ahc_compile_devinfo(&devinfo,
						    initiator_role_id,
						    target,
						    TCL_LUN(saved_tcl),
						    channel,
						    ROLE_INITIATOR);
				ahc_handle_devreset(ahc, &devinfo,
						    XS_RESET,
						    "Bus Device Reset",
						    /*verbose_level*/0);
				printerror = 0;
				break;
			}
			default:
				break;
			}
		}
		if (printerror != 0) {
			int i;

			if (scb != NULL) {
				u_int tag;

				if ((scb->hscb->control & TAG_ENB) != 0)
					tag = scb->hscb->tag;
				else
					tag = SCB_LIST_NULL;
				ahc_abort_scbs(ahc, target, channel,
					       SCB_LUN(scb), tag,
					       ROLE_INITIATOR,
					       XS_DRIVER_STUFFUP);
				scsipi_printaddr(scb->xs->xs_periph);
			} else {
				/*
				 * We had not fully identified this connection,
				 * so we cannot abort anything.
				 */
				printf("%s: ", ahc_name(ahc));
			}
			for (i = 0; i < num_phases; i++) {
				if (lastphase == phase_table[i].phase)
					break;
			}
			printf("Unexpected busfree %s\n"
			       "SEQADDR == 0x%x\n",
			       phase_table[i].phasemsg, ahc_inb(ahc, SEQADDR0)
				| (ahc_inb(ahc, SEQADDR1) << 8));
		}
		ahc_clear_msg_state(ahc);
		ahc_outb(ahc, SIMODE1, ahc_inb(ahc, SIMODE1) & ~ENBUSFREE);
		ahc_outb(ahc, CLRSINT1, CLRBUSFREE|CLRSCSIPERR);
		ahc_outb(ahc, CLRINT, CLRSCSIINT);
		restart_sequencer(ahc);
	} else if ((status & SELTO) != 0) {
		u_int scbptr;

		scbptr = ahc_inb(ahc, WAITING_SCBH);
		ahc_outb(ahc, SCBPTR, scbptr);
		scb_index = ahc_inb(ahc, SCB_TAG);

		if (scb_index < ahc->scb_data->numscbs) {
			scb = &ahc->scb_data->scbarray[scb_index];
			if ((scb->flags & SCB_ACTIVE) == 0)
				scb = NULL;
		} else
			scb = NULL;

		if (scb == NULL) {
			printf("%s: ahc_intr - referenced scb not "
			       "valid during SELTO scb(%d, %d)\n",
			       ahc_name(ahc), scbptr, scb_index);
		} else {
			u_int tag;

			tag = SCB_LIST_NULL;
			if ((scb->hscb->control & TAG_ENB) != 0)
				tag = scb->hscb->tag;

			ahc_abort_scbs(ahc, SCB_TARGET(scb), SCB_CHANNEL(scb),
				       SCB_LUN(scb), tag,
				       ROLE_INITIATOR, XS_SELTIMEOUT);
		}
		/* Stop the selection */
		ahc_outb(ahc, SCSISEQ, 0);

		/* No more pending messages */
		ahc_clear_msg_state(ahc);

		/*
		 * Although the driver does not care about the
		 * 'Selection in Progress' status bit, the busy
		 * LED does.  SELINGO is only cleared by a sucessful
		 * selection, so we must manually clear it to ensure
		 * the LED turns off just incase no future successful
		 * selections occur (e.g. no devices on the bus).
		 */
		ahc_outb(ahc, CLRSINT0, CLRSELINGO);

		/* Clear interrupt state */
		ahc_outb(ahc, CLRSINT1, CLRSELTIMEO|CLRBUSFREE|CLRSCSIPERR);
		ahc_outb(ahc, CLRINT, CLRSCSIINT);
		restart_sequencer(ahc);
	} else {
		scsipi_printaddr(scb->xs->xs_periph);
		printf("Unknown SCSIINT. Status = 0x%x\n", status);
		ahc_outb(ahc, CLRSINT1, status);
		ahc_outb(ahc, CLRINT, CLRSCSIINT);
		unpause_sequencer(ahc);
	}
}

static void
ahc_build_transfer_msg(struct ahc_softc *ahc, struct ahc_devinfo *devinfo)
{
	/*
	 * We need to initiate transfer negotiations.
	 * If our current and goal settings are identical,
	 * we want to renegotiate due to a check condition.
	 */
	struct	ahc_initiator_tinfo *tinfo;
	struct	tmode_tstate *tstate;
	int	dowide;
	int	dosync;

	tinfo = ahc_fetch_transinfo(ahc, devinfo->channel, devinfo->our_scsiid,
				    devinfo->target, &tstate);
	dowide = tinfo->current.width != tinfo->goal.width;
	dosync = tinfo->current.period != tinfo->goal.period;

	if (!dowide && !dosync) {
		dowide = tinfo->goal.width != MSG_EXT_WDTR_BUS_8_BIT;
		dosync = tinfo->goal.period != 0;
	}

	if (dowide) {
		ahc_construct_wdtr(ahc, tinfo->goal.width);
	} else if (dosync) {
		const struct	ahc_syncrate *rate;
		u_int	period;
		u_int	offset;

		period = tinfo->goal.period;
		rate = ahc_devlimited_syncrate(ahc, &period);
		offset = tinfo->goal.offset;
		ahc_validate_offset(ahc, rate, &offset,
				    tinfo->current.width);
		ahc_construct_sdtr(ahc, period, offset);
	} else {
		panic("ahc_intr: AWAITING_MSG for negotiation, "
		      "but no negotiation needed\n");
	}
}

static void
ahc_setup_initiator_msgout(struct ahc_softc *ahc, struct ahc_devinfo *devinfo,
			   struct scb *scb)
{
	/*
	 * To facilitate adding multiple messages together,
	 * each routine should increment the index and len
	 * variables instead of setting them explicitly.
	 */
	ahc->msgout_index = 0;
	ahc->msgout_len = 0;

	if ((scb->flags & SCB_DEVICE_RESET) == 0
	 && ahc_inb(ahc, MSG_OUT) == MSG_IDENTIFYFLAG) {
		u_int identify_msg;

		identify_msg = MSG_IDENTIFYFLAG | SCB_LUN(scb);
		if ((scb->hscb->control & DISCENB) != 0)
			identify_msg |= MSG_IDENTIFY_DISCFLAG;
		ahc->msgout_buf[ahc->msgout_index++] = identify_msg;
		ahc->msgout_len++;

		if ((scb->hscb->control & TAG_ENB) != 0) {
			ahc->msgout_buf[ahc->msgout_index++] =
						scb->xs->xs_tag_type;
			ahc->msgout_buf[ahc->msgout_index++] = scb->hscb->tag;
			ahc->msgout_len += 2;
		}
	}

	if (scb->flags & SCB_DEVICE_RESET) {
		ahc->msgout_buf[ahc->msgout_index++] = MSG_BUS_DEV_RESET;
		ahc->msgout_len++;
		scsipi_printaddr(scb->xs->xs_periph);
		printf("Bus Device Reset Message Sent\n");
	} else if (scb->flags & SCB_ABORT) {
		if ((scb->hscb->control & TAG_ENB) != 0)
			ahc->msgout_buf[ahc->msgout_index++] = MSG_ABORT_TAG;
		else
			ahc->msgout_buf[ahc->msgout_index++] = MSG_ABORT;
		ahc->msgout_len++;
		scsipi_printaddr(scb->xs->xs_periph);
		printf("Abort Message Sent\n");
	} else if ((ahc->targ_msg_req & devinfo->target_mask) != 0) {
		ahc_build_transfer_msg(ahc, devinfo);
	} else {
		printf("ahc_intr: AWAITING_MSG for an SCB that "
		       "does not have a waiting message");
		panic("SCB = %d, SCB Control = %x, MSG_OUT = %x "
		      "SCB flags = %x", scb->hscb->tag, scb->hscb->control,
		      ahc_inb(ahc, MSG_OUT), scb->flags);
	}

	/*
	 * Clear the MK_MESSAGE flag from the SCB so we aren't
	 * asked to send this message again.
	 */
	ahc_outb(ahc, SCB_CONTROL, ahc_inb(ahc, SCB_CONTROL) & ~MK_MESSAGE);
	ahc->msgout_index = 0;
	ahc->msg_type = MSG_TYPE_INITIATOR_MSGOUT;
}

static void
ahc_setup_target_msgin(struct ahc_softc *ahc, struct ahc_devinfo *devinfo)
{
	/*
	 * To facilitate adding multiple messages together,
	 * each routine should increment the index and len
	 * variables instead of setting them explicitly.
	 */
	ahc->msgout_index = 0;
	ahc->msgout_len = 0;

	if ((ahc->targ_msg_req & devinfo->target_mask) != 0)
		ahc_build_transfer_msg(ahc, devinfo);
	else
		panic("ahc_intr: AWAITING target message with no message");

	ahc->msgout_index = 0;
	ahc->msg_type = MSG_TYPE_TARGET_MSGIN;
}

static int
ahc_handle_msg_reject(struct ahc_softc *ahc, struct ahc_devinfo *devinfo)
{
	/*
	 * What we care about here is if we had an
	 * outstanding SDTR or WDTR message for this
	 * target.  If we did, this is a signal that
	 * the target is refusing negotiation.
	 */
	struct scb *scb;
	u_int scb_index;
	u_int last_msg;
	int   response = 0;

	scb_index = ahc_inb(ahc, SCB_TAG);
	scb = &ahc->scb_data->scbarray[scb_index];

	/* Might be necessary */
	last_msg = ahc_inb(ahc, LAST_MSG);

	if (ahc_sent_msg(ahc, MSG_EXT_WDTR, /*full*/FALSE)) {
		struct ahc_initiator_tinfo *tinfo;
		struct tmode_tstate *tstate;

#ifdef AHC_DEBUG_NEG
		/* note 8bit xfers */
		printf("%s:%c:%d: refuses WIDE negotiation.  Using "
		       "8bit transfers\n", ahc_name(ahc),
		       devinfo->channel, devinfo->target);
#endif
		ahc_set_width(ahc, devinfo,
			      MSG_EXT_WDTR_BUS_8_BIT,
			      AHC_TRANS_ACTIVE|AHC_TRANS_GOAL,
			      /*paused*/TRUE, /*done*/TRUE);
		/*
		 * No need to clear the sync rate.  If the target
		 * did not accept the command, our syncrate is
		 * unaffected.  If the target started the negotiation,
		 * but rejected our response, we already cleared the
		 * sync rate before sending our WDTR.
		 */
		tinfo = ahc_fetch_transinfo(ahc, devinfo->channel,
					    devinfo->our_scsiid,
					    devinfo->target, &tstate);
		if (tinfo->goal.period) {
			u_int period;

			/* Start the sync negotiation */
			period = tinfo->goal.period;
			ahc_devlimited_syncrate(ahc, &period);
			ahc->msgout_index = 0;
			ahc->msgout_len = 0;
			ahc_construct_sdtr(ahc, period, tinfo->goal.offset);
			ahc->msgout_index = 0;
			response = 1;
		} else 
			ahc_update_xfer_mode(ahc, devinfo);
	} else if (ahc_sent_msg(ahc, MSG_EXT_SDTR, /*full*/FALSE)) {
		/* note asynch xfers and clear flag */
		ahc_set_syncrate(ahc, devinfo, /*syncrate*/NULL, /*period*/0,
				 /*offset*/0, AHC_TRANS_ACTIVE|AHC_TRANS_GOAL,
				 /*paused*/TRUE, /*done*/TRUE);
#ifdef AHC_DEBUG_NEG
		printf("%s:%c:%d: refuses synchronous negotiation. "
		       "Using asynchronous transfers\n",
		       ahc_name(ahc),
		       devinfo->channel, devinfo->target);
#endif
		ahc_update_xfer_mode(ahc, devinfo);
	} else if ((scb->hscb->control & TAG_ENB) != 0) {
		printf("%s:%c:%d: refuses tagged commands.  Performing "
		       "non-tagged I/O\n", ahc_name(ahc),
		       devinfo->channel, devinfo->target);

		ahc_set_tags(ahc, devinfo, FALSE);
		ahc_update_xfer_mode(ahc, devinfo);

		/*
		 * Resend the identify for this CCB as the target
		 * may believe that the selection is invalid otherwise.
		 */
		ahc_outb(ahc, SCB_CONTROL, ahc_inb(ahc, SCB_CONTROL)
					  & ~MSG_SIMPLE_Q_TAG);
	 	scb->hscb->control &= ~MSG_SIMPLE_Q_TAG;
		ahc_outb(ahc, MSG_OUT, MSG_IDENTIFYFLAG);
		ahc_outb(ahc, SCSISIGO, ahc_inb(ahc, SCSISIGO) | ATNO);

		/*
		 * Requeue all tagged commands for this target
		 * currently in our posession so they can be
		 * converted to untagged commands.
		 */
		ahc_search_qinfifo(ahc, SCB_TARGET(scb), SCB_CHANNEL(scb),
				   SCB_LUN(scb), /*tag*/SCB_LIST_NULL,
				   ROLE_INITIATOR, SCB_REQUEUE,
				   SEARCH_COMPLETE);
	} else {
		/*
		 * Otherwise, we ignore it.
		 */
		printf("%s:%c:%d: Message reject for %x -- ignored\n",
		       ahc_name(ahc), devinfo->channel, devinfo->target,
		       last_msg);
	}
	return (response);
}

static void
ahc_clear_msg_state(struct ahc_softc *ahc)
{
	ahc->msgout_len = 0;
	ahc->msgin_index = 0;
	ahc->msg_type = MSG_TYPE_NONE;
	ahc_outb(ahc, MSG_OUT, MSG_NOOP);
}

static void
ahc_handle_message_phase(struct ahc_softc *ahc, struct scsipi_periph *periph)
{
	struct	ahc_devinfo devinfo;
	u_int	bus_phase;
	int	end_session;

	ahc_fetch_devinfo(ahc, &devinfo);
	end_session = FALSE;
	bus_phase = ahc_inb(ahc, SCSISIGI) & PHASE_MASK;

reswitch:
	switch (ahc->msg_type) {
	case MSG_TYPE_INITIATOR_MSGOUT:
	{
		int lastbyte;
		int phasemis;
		int msgdone;

		if (ahc->msgout_len == 0)
			panic("REQINIT interrupt with no active message");

		phasemis = bus_phase != P_MESGOUT;
		if (phasemis) {
			if (bus_phase == P_MESGIN) {
				/*
				 * Change gears and see if
				 * this messages is of interest to
				 * us or should be passed back to
				 * the sequencer.
				 */
				ahc_outb(ahc, CLRSINT1, CLRATNO);
				ahc->send_msg_perror = FALSE;
				ahc->msg_type = MSG_TYPE_INITIATOR_MSGIN;
				ahc->msgin_index = 0;
				goto reswitch;
			}
			end_session = TRUE;
			break;
		}

		if (ahc->send_msg_perror) {
			ahc_outb(ahc, CLRSINT1, CLRATNO);
			ahc_outb(ahc, CLRSINT1, CLRREQINIT);
			ahc_outb(ahc, SCSIDATL, MSG_PARITY_ERROR);
			break;
		}

		msgdone	= ahc->msgout_index == ahc->msgout_len;
		if (msgdone) {
			/*
			 * The target has requested a retry.
			 * Re-assert ATN, reset our message index to
			 * 0, and try again.
			 */
			ahc->msgout_index = 0;
			ahc_outb(ahc, SCSISIGO, ahc_inb(ahc, SCSISIGO) | ATNO);
		}

		lastbyte = ahc->msgout_index == (ahc->msgout_len - 1);
		if (lastbyte) {
			/* Last byte is signified by dropping ATN */
			ahc_outb(ahc, CLRSINT1, CLRATNO);
		}

		/*
		 * Clear our interrupt status and present
		 * the next byte on the bus.
		 */
		ahc_outb(ahc, CLRSINT1, CLRREQINIT);
		ahc_outb(ahc, SCSIDATL, ahc->msgout_buf[ahc->msgout_index++]);
		break;
	}
	case MSG_TYPE_INITIATOR_MSGIN:
	{
		int phasemis;
		int message_done;

		phasemis = bus_phase != P_MESGIN;

		if (phasemis) {
			ahc->msgin_index = 0;
			if (bus_phase == P_MESGOUT
			 && (ahc->send_msg_perror == TRUE
			  || (ahc->msgout_len != 0
			   && ahc->msgout_index == 0))) {
				ahc->msg_type = MSG_TYPE_INITIATOR_MSGOUT;
				goto reswitch;
			}
			end_session = TRUE;
			break;
		}

		/* Pull the byte in without acking it */
		ahc->msgin_buf[ahc->msgin_index] = ahc_inb(ahc, SCSIBUSL);

		message_done = ahc_parse_msg(ahc, periph, &devinfo);

		if (message_done) {
			/*
			 * Clear our incoming message buffer in case there
			 * is another message following this one.
			 */
			ahc->msgin_index = 0;

			/*
			 * If this message illicited a response,
			 * assert ATN so the target takes us to the
			 * message out phase.
			 */
			if (ahc->msgout_len != 0)
				ahc_outb(ahc, SCSISIGO,
					 ahc_inb(ahc, SCSISIGO) | ATNO);
		} else
			ahc->msgin_index++;

		/* Ack the byte */
		ahc_outb(ahc, CLRSINT1, CLRREQINIT);
		ahc_inb(ahc, SCSIDATL);
		break;
	}
	case MSG_TYPE_TARGET_MSGIN:
	{
		int msgdone;
		int msgout_request;

		if (ahc->msgout_len == 0)
			panic("Target MSGIN with no active message");

		/*
		 * If we interrupted a mesgout session, the initiator
		 * will not know this until our first REQ.  So, we
		 * only honor mesgout requests after we've sent our
		 * first byte.
		 */
		if ((ahc_inb(ahc, SCSISIGI) & ATNI) != 0
		 && ahc->msgout_index > 0)
			msgout_request = TRUE;
		else
			msgout_request = FALSE;

		if (msgout_request) {

			/*
			 * Change gears and see if
			 * this messages is of interest to
			 * us or should be passed back to
			 * the sequencer.
			 */
			ahc->msg_type = MSG_TYPE_TARGET_MSGOUT;
			ahc_outb(ahc, SCSISIGO, P_MESGOUT | BSYO);
			ahc->msgin_index = 0;
			/* Dummy read to REQ for first byte */
			ahc_inb(ahc, SCSIDATL);
			ahc_outb(ahc, SXFRCTL0,
				 ahc_inb(ahc, SXFRCTL0) | SPIOEN);
			break;
		}

		msgdone = ahc->msgout_index == ahc->msgout_len;
		if (msgdone) {
			ahc_outb(ahc, SXFRCTL0,
				 ahc_inb(ahc, SXFRCTL0) & ~SPIOEN);
			end_session = TRUE;
			break;
		}

		/*
		 * Present the next byte on the bus.
		 */
		ahc_outb(ahc, SXFRCTL0, ahc_inb(ahc, SXFRCTL0) | SPIOEN);
		ahc_outb(ahc, SCSIDATL, ahc->msgout_buf[ahc->msgout_index++]);
		break;
	}
	case MSG_TYPE_TARGET_MSGOUT:
	{
		int lastbyte;
		int msgdone;

		/*
		 * The initiator signals that this is
		 * the last byte by dropping ATN.
		 */
		lastbyte = (ahc_inb(ahc, SCSISIGI) & ATNI) == 0;

		/*
		 * Read the latched byte, but turn off SPIOEN first
		 * so that we don't inadvertantly cause a REQ for the
		 * next byte.
		 */
		ahc_outb(ahc, SXFRCTL0, ahc_inb(ahc, SXFRCTL0) & ~SPIOEN);
		ahc->msgin_buf[ahc->msgin_index] = ahc_inb(ahc, SCSIDATL);
		msgdone = ahc_parse_msg(ahc, periph, &devinfo);
		if (msgdone == MSGLOOP_TERMINATED) {
			/*
			 * The message is *really* done in that it caused
			 * us to go to bus free.  The sequencer has already
			 * been reset at this point, so pull the ejection
			 * handle.
			 */
			return;
		}

		ahc->msgin_index++;

		/*
		 * XXX Read spec about initiator dropping ATN too soon
		 *     and use msgdone to detect it.
		 */
		if (msgdone == MSGLOOP_MSGCOMPLETE) {
			ahc->msgin_index = 0;

			/*
			 * If this message illicited a response, transition
			 * to the Message in phase and send it.
			 */
			if (ahc->msgout_len != 0) {
				ahc_outb(ahc, SCSISIGO, P_MESGIN | BSYO);
				ahc_outb(ahc, SXFRCTL0,
					 ahc_inb(ahc, SXFRCTL0) | SPIOEN);
				ahc->msg_type = MSG_TYPE_TARGET_MSGIN;
				ahc->msgin_index = 0;
				break;
			}
		}

		if (lastbyte)
			end_session = TRUE;
		else {
			/* Ask for the next byte. */
			ahc_outb(ahc, SXFRCTL0,
				 ahc_inb(ahc, SXFRCTL0) | SPIOEN);
		}

		break;
	}
	default:
		panic("Unknown REQINIT message type");
	}

	if (end_session) {
		ahc_clear_msg_state(ahc);
		ahc_outb(ahc, RETURN_1, EXIT_MSG_LOOP);
	} else
		ahc_outb(ahc, RETURN_1, CONT_MSG_LOOP);
}

/*
 * See if we sent a particular extended message to the target.
 * If "full" is true, the target saw the full message.
 * If "full" is false, the target saw at least the first
 * byte of the message.
 */
static int
ahc_sent_msg(struct ahc_softc *ahc, u_int msgtype, int full)
{
	int found;
	int index;

	found = FALSE;
	index = 0;

	while (index < ahc->msgout_len) {
		if (ahc->msgout_buf[index] == MSG_EXTENDED) {

			/* Found a candidate */
			if (ahc->msgout_buf[index+2] == msgtype) {
				u_int end_index;

				end_index = index + 1
					  + ahc->msgout_buf[index + 1];
				if (full) {
					if (ahc->msgout_index > end_index)
						found = TRUE;
				} else if (ahc->msgout_index > index)
					found = TRUE;
			}
			break;
		} else if (ahc->msgout_buf[index] >= MSG_SIMPLE_Q_TAG
			&& ahc->msgout_buf[index] <= MSG_IGN_WIDE_RESIDUE) {

			/* Skip tag type and tag id or residue param*/
			index += 2;
		} else {
			/* Single byte message */
			index++;
		}
	}
	return (found);
}

static int
ahc_parse_msg(struct ahc_softc *ahc, struct scsipi_periph *periph,
	      struct ahc_devinfo *devinfo)
{
	struct	ahc_initiator_tinfo *tinfo;
	struct	tmode_tstate *tstate;
	int	reject;
	int	done;
	int	response;
	u_int	targ_scsirate;

	done = MSGLOOP_IN_PROG;
	response = FALSE;
	reject = FALSE;
	tinfo = ahc_fetch_transinfo(ahc, devinfo->channel, devinfo->our_scsiid,
				    devinfo->target, &tstate);
	targ_scsirate = tinfo->scsirate;

	/*
	 * Parse as much of the message as is available,
	 * rejecting it if we don't support it.  When
	 * the entire message is available and has been
	 * handled, return MSGLOOP_MSGCOMPLETE, indicating
	 * that we have parsed an entire message.
	 *
	 * In the case of extended messages, we accept the length
	 * byte outright and perform more checking once we know the
	 * extended message type.
	 */
	switch (ahc->msgin_buf[0]) {
	case MSG_MESSAGE_REJECT:
		response = ahc_handle_msg_reject(ahc, devinfo);
		/* FALLTHROUGH */
	case MSG_NOOP:
		done = MSGLOOP_MSGCOMPLETE;
		break;
	case MSG_IGN_WIDE_RESIDUE:
	{
		/* Wait for the whole message */
		if (ahc->msgin_index >= 1) {
			if (ahc->msgin_buf[1] != 1
			 || tinfo->current.width == MSG_EXT_WDTR_BUS_8_BIT) {
				reject = TRUE;
				done = MSGLOOP_MSGCOMPLETE;
			} else
				ahc_handle_ign_wide_residue(ahc, devinfo);
		}
		break;
	}
	case MSG_EXTENDED:
	{
		/* Wait for enough of the message to begin validation */
		if (ahc->msgin_index < 2)
			break;
		switch (ahc->msgin_buf[2]) {
		case MSG_EXT_SDTR:
		{
			const struct	 ahc_syncrate *syncrate;
			u_int	 period;
			u_int	 offset;
			u_int	 saved_offset;

			if (ahc->msgin_buf[1] != MSG_EXT_SDTR_LEN) {
				reject = TRUE;
				break;
			}

			/*
			 * Wait until we have both args before validating
			 * and acting on this message.
			 *
			 * Add one to MSG_EXT_SDTR_LEN to account for
			 * the extended message preamble.
			 */
			if (ahc->msgin_index < (MSG_EXT_SDTR_LEN + 1))
				break;

			period = ahc->msgin_buf[3];
			saved_offset = offset = ahc->msgin_buf[4];
			syncrate = ahc_devlimited_syncrate(ahc, &period);
			ahc_validate_offset(ahc, syncrate, &offset,
					    targ_scsirate & WIDEXFER);
			ahc_set_syncrate(ahc, devinfo,
					 syncrate, period, offset,
					 AHC_TRANS_ACTIVE|AHC_TRANS_GOAL,
					 /*paused*/TRUE, /*done*/TRUE);
			ahc_update_xfer_mode(ahc, devinfo);

			/*
			 * See if we initiated Sync Negotiation
			 * and didn't have to fall down to async
			 * transfers.
			 */
			if (ahc_sent_msg(ahc, MSG_EXT_SDTR, /*full*/TRUE)) {
				/* We started it */
				if (saved_offset != offset) {
					/* Went too low - force async */
					reject = TRUE;
				}
			} else {
				/*
				 * Send our own SDTR in reply
				 */
				ahc->msgout_index = 0;
				ahc->msgout_len = 0;
				ahc_construct_sdtr(ahc, period, offset);
				ahc->msgout_index = 0;
				response = TRUE;
			}
			done = MSGLOOP_MSGCOMPLETE;
			break;
		}
		case MSG_EXT_WDTR:
		{
			u_int	bus_width;
			u_int	sending_reply;

			sending_reply = FALSE;
			if (ahc->msgin_buf[1] != MSG_EXT_WDTR_LEN) {
				reject = TRUE;
				break;
			}

			/*
			 * Wait until we have our arg before validating
			 * and acting on this message.
			 *
			 * Add one to MSG_EXT_WDTR_LEN to account for
			 * the extended message preamble.
			 */
			if (ahc->msgin_index < (MSG_EXT_WDTR_LEN + 1))
				break;

			bus_width = ahc->msgin_buf[3];
			if (ahc_sent_msg(ahc, MSG_EXT_WDTR, /*full*/TRUE)) {
				/*
				 * Don't send a WDTR back to the
				 * target, since we asked first.
				 */
				switch (bus_width){
				default:
					/*
					 * How can we do anything greater
					 * than 16bit transfers on a 16bit
					 * bus?
					 */
					reject = TRUE;
					printf("%s: target %d requested %dBit "
					       "transfers.  Rejecting...\n",
					       ahc_name(ahc), devinfo->target,
					       8 * (0x01 << bus_width));
					/* FALLTHROUGH */
				case MSG_EXT_WDTR_BUS_8_BIT:
					bus_width = MSG_EXT_WDTR_BUS_8_BIT;
					break;
				case MSG_EXT_WDTR_BUS_16_BIT:
					break;
				}
			} else {
				/*
				 * Send our own WDTR in reply
				 */
				switch (bus_width) {
				default:
					if (ahc->features & AHC_WIDE) {
						/* Respond Wide */
						bus_width =
						    MSG_EXT_WDTR_BUS_16_BIT;
						break;
					}
					/* FALLTHROUGH */
				case MSG_EXT_WDTR_BUS_8_BIT:
					bus_width = MSG_EXT_WDTR_BUS_8_BIT;
					break;
				}
				ahc->msgout_index = 0;
				ahc->msgout_len = 0;
				ahc_construct_wdtr(ahc, bus_width);
				ahc->msgout_index = 0;
				response = TRUE;
				sending_reply = TRUE;
			}
			ahc_set_width(ahc, devinfo, bus_width,
				      AHC_TRANS_ACTIVE|AHC_TRANS_GOAL,
				      /*paused*/TRUE, /*done*/TRUE);

			/* After a wide message, we are async */
			ahc_set_syncrate(ahc, devinfo,
					 /*syncrate*/NULL, /*period*/0,
					 /*offset*/0, AHC_TRANS_ACTIVE,
					 /*paused*/TRUE, /*done*/FALSE);
			if (sending_reply == FALSE && reject == FALSE) {

				if (tinfo->goal.period) {
					const struct	ahc_syncrate *rate;
					u_int	period;
					u_int	offset;

					/* Start the sync negotiation */
					period = tinfo->goal.period;
					rate = ahc_devlimited_syncrate(ahc,
								       &period);
					offset = tinfo->goal.offset;
					ahc_validate_offset(ahc, rate, &offset,
							  tinfo->current.width);
					ahc->msgout_index = 0;
					ahc->msgout_len = 0;
					ahc_construct_sdtr(ahc, period, offset);
					ahc->msgout_index = 0;
					response = TRUE;
				} else 
					ahc_update_xfer_mode(ahc, devinfo);
			}
			done = MSGLOOP_MSGCOMPLETE;
			break;
		}
		default:
			/* Unknown extended message.  Reject it. */
			reject = TRUE;
			break;
		}
		break;
	}
	case MSG_BUS_DEV_RESET:
		ahc_handle_devreset(ahc, devinfo,
				    XS_RESET, "Bus Device Reset Received",
				    /*verbose_level*/0);
		restart_sequencer(ahc);
		done = MSGLOOP_TERMINATED;
		break;
	case MSG_ABORT_TAG:
	case MSG_ABORT:
	case MSG_CLEAR_QUEUE:
		/* Target mode messages */
		if (devinfo->role != ROLE_TARGET) {
			reject = TRUE;
			break;
		}
#if AHC_TARGET_MODE
		ahc_abort_scbs(ahc, devinfo->target, devinfo->channel,
			       devinfo->lun,
			       ahc->msgin_buf[0] == MSG_ABORT_TAG
						  ? SCB_LIST_NULL
						  : ahc_inb(ahc, INITIATOR_TAG),
			       ROLE_TARGET, XS_DRIVER_STUFFUP);

		tstate = ahc->enabled_targets[devinfo->our_scsiid];
		if (tstate != NULL) {
			struct tmode_lstate* lstate;

			lstate = tstate->enabled_luns[devinfo->lun];
			if (lstate != NULL) {
				ahc_queue_lstate_event(ahc, lstate,
						       devinfo->our_scsiid,
						       ahc->msgin_buf[0],
						       /*arg*/0);
				ahc_send_lstate_events(ahc, lstate);
			}
		}
		done = MSGLOOP_MSGCOMPLETE;
#else
		panic("ahc: got target mode message");
#endif
		break;
	case MSG_TERM_IO_PROC:
	default:
		reject = TRUE;
		break;
	}

	if (reject) {
		/*
		 * Setup to reject the message.
		 */
		ahc->msgout_index = 0;
		ahc->msgout_len = 1;
		ahc->msgout_buf[0] = MSG_MESSAGE_REJECT;
		done = MSGLOOP_MSGCOMPLETE;
		response = TRUE;
	}

	if (done != MSGLOOP_IN_PROG && !response)
		/* Clear the outgoing message buffer */
		ahc->msgout_len = 0;

	return (done);
}

static void
ahc_handle_ign_wide_residue(struct ahc_softc *ahc, struct ahc_devinfo *devinfo)
{
	u_int scb_index;
	struct scb *scb;

	scb_index = ahc_inb(ahc, SCB_TAG);
	scb = &ahc->scb_data->scbarray[scb_index];
	if ((ahc_inb(ahc, SEQ_FLAGS) & DPHASE) == 0
	 || !(scb->xs->xs_control & XS_CTL_DATA_IN)) {
		/*
		 * Ignore the message if we haven't
		 * seen an appropriate data phase yet.
		 */
	} else {
		/*
		 * If the residual occurred on the last
		 * transfer and the transfer request was
		 * expected to end on an odd count, do
		 * nothing.  Otherwise, subtract a byte
		 * and update the residual count accordingly.
		 */
		u_int resid_sgcnt;

		resid_sgcnt = ahc_inb(ahc, SCB_RESID_SGCNT);
		if (resid_sgcnt == 0
		 && ahc_inb(ahc, DATA_COUNT_ODD) == 1) {
			/*
			 * If the residual occurred on the last
			 * transfer and the transfer request was
			 * expected to end on an odd count, do
			 * nothing.
			 */
		} else {
			u_int data_cnt;
			u_int32_t data_addr;
			u_int sg_index;

			data_cnt = (ahc_inb(ahc, SCB_RESID_DCNT + 2) << 16)
				 | (ahc_inb(ahc, SCB_RESID_DCNT + 1) << 8)
				 | (ahc_inb(ahc, SCB_RESID_DCNT));

			data_addr = (ahc_inb(ahc, SHADDR + 3) << 24)
				  | (ahc_inb(ahc, SHADDR + 2) << 16)
				  | (ahc_inb(ahc, SHADDR + 1) << 8)
				  | (ahc_inb(ahc, SHADDR));

			data_cnt += 1;
			data_addr -= 1;

			sg_index = scb->sg_count - resid_sgcnt;

			if (sg_index != 0
			 && (le32toh(scb->sg_list[sg_index].len) < data_cnt)) {
				u_int32_t sg_addr;

				sg_index--;
				data_cnt = 1;
				data_addr = le32toh(scb->sg_list[sg_index].addr)
					  + le32toh(scb->sg_list[sg_index].len)
					  - 1;

				/*
				 * The physical address base points to the
				 * second entry as it is always used for
				 * calculating the "next S/G pointer".
				 */
				sg_addr = scb->sg_list_phys
					+ (sg_index* sizeof(*scb->sg_list));
				ahc_outb(ahc, SG_NEXT + 3, sg_addr >> 24);
				ahc_outb(ahc, SG_NEXT + 2, sg_addr >> 16);
				ahc_outb(ahc, SG_NEXT + 1, sg_addr >> 8);
				ahc_outb(ahc, SG_NEXT, sg_addr);
			}

			ahc_outb(ahc, SCB_RESID_DCNT + 2, data_cnt >> 16);
			ahc_outb(ahc, SCB_RESID_DCNT + 1, data_cnt >> 8);
			ahc_outb(ahc, SCB_RESID_DCNT, data_cnt);

			ahc_outb(ahc, SHADDR + 3, data_addr >> 24);
			ahc_outb(ahc, SHADDR + 2, data_addr >> 16);
			ahc_outb(ahc, SHADDR + 1, data_addr >> 8);
			ahc_outb(ahc, SHADDR, data_addr);
		}
	}
}

static void
ahc_handle_devreset(struct ahc_softc *ahc, struct ahc_devinfo *devinfo,
		    int status, char *message,
		    int verbose_level)
{
	int found;

	found = ahc_abort_scbs(ahc, devinfo->target, devinfo->channel,
			       AHC_LUN_WILDCARD, SCB_LIST_NULL, devinfo->role,
			       status);

	/*
	 * Go back to async/narrow transfers and renegotiate.
	 * ahc_set_width and ahc_set_syncrate can cope with NULL
	 * paths.
	 */
	ahc_set_width(ahc, devinfo, MSG_EXT_WDTR_BUS_8_BIT,
		      AHC_TRANS_CUR, /*paused*/TRUE, /*done*/FALSE);
	ahc_set_syncrate(ahc, devinfo, /*syncrate*/NULL,
			 /*period*/0, /*offset*/0, AHC_TRANS_CUR,
			 /*paused*/TRUE, /*done*/FALSE);
	ahc_update_xfer_mode(ahc, devinfo);

	if (message != NULL && (verbose_level <= 0))
		printf("%s: %s on %c:%d. %d SCBs aborted\n", ahc_name(ahc),
		       message, devinfo->channel, devinfo->target, found);
}

/*
 * We have an scb which has been processed by the
 * adaptor, now we look to see how the operation
 * went.
 */
static void
ahc_done(struct ahc_softc *ahc, struct scb *scb)
{
	struct scsipi_xfer *xs;
	struct scsipi_periph *periph;
	int requeue = 0;
	int target;


	xs = scb->xs;
	periph = xs->xs_periph;
	LIST_REMOVE(scb, plinks);

	callout_stop(&scb->xs->xs_callout);

#ifdef AHC_DEBUG
	if (ahc_debug & AHC_SHOWCMDS) {
		scsipi_printaddr(periph);
		printf("ahc_done opcode %d tag %x\n", xs->cmdstore.opcode,
		    scb->hscb->tag);
	}
#endif

	target = periph->periph_target;

	if (xs->datalen) {
		int op;

		if (xs->xs_control & XS_CTL_DATA_IN)
			op = BUS_DMASYNC_POSTREAD;
		else
			op = BUS_DMASYNC_POSTWRITE;
		bus_dmamap_sync(ahc->parent_dmat, scb->dmamap, 0,
		    scb->dmamap->dm_mapsize, op);
		bus_dmamap_unload(ahc->parent_dmat, scb->dmamap);
	}

	/*
	 * Unbusy this target/channel/lun.
	 * XXX if we are holding two commands per lun,
	 *     send the next command.
	 */
	if (!(scb->hscb->control & TAG_ENB))
		ahc_index_busy_tcl(ahc, scb->hscb->tcl, /*unbusy*/TRUE);

	/*
	 * If the recovery SCB completes, we have to be
	 * out of our timeout.
	 */
	if ((scb->flags & SCB_RECOVERY_SCB) != 0) {

		struct	scb *scbp;

		/*
		 * We were able to complete the command successfully,
		 * so reinstate the timeouts for all other pending
		 * commands.
		 */
		scbp = ahc->pending_ccbs.lh_first;
		while (scbp != NULL) {
			struct scsipi_xfer *txs = scbp->xs;

			if (!(txs->xs_control & XS_CTL_POLL)) {
				callout_reset(&scbp->xs->xs_callout,
				    (scbp->xs->timeout * hz) / 1000,
				    ahc_timeout, scbp);
			}
			scbp = LIST_NEXT(scbp, plinks);
		}

		/*
		 * Ensure that we didn't put a second instance of this
		 * SCB into the QINFIFO.
		 */
		ahc_search_qinfifo(ahc, SCB_TARGET(scb), SCB_CHANNEL(scb),
				   SCB_LUN(scb), scb->hscb->tag,
				   ROLE_INITIATOR, /*status*/0,
				   SEARCH_REMOVE);
		if (xs->error != XS_NOERROR)
			ahcsetccbstatus(xs, XS_TIMEOUT);
		scsipi_printaddr(xs->xs_periph);
		printf("no longer in timeout, status = %x\n", xs->status);
	}

	if (xs->error != XS_NOERROR) {
		/* Don't clobber any existing error state */
	} else if ((scb->flags & SCB_SENSE) != 0) {
		/*
		 * We performed autosense retrieval.
		 *
		 * zero the sense data before having
		 * the drive fill it.  The SCSI spec mandates
		 * that any untransferred data should be
		 * assumed to be zero.  Complete the 'bounce'
		 * of sense information through buffers accessible
		 * via bus-space by copying it into the clients
		 * csio.
		 */
		memset(&xs->sense.scsi_sense, 0, sizeof(xs->sense.scsi_sense));
		memcpy(&xs->sense.scsi_sense,
		    &ahc->scb_data->sense[scb->hscb->tag],
		    le32toh(scb->sg_list->len));
		xs->error = XS_SENSE;
	}
	if (scb->flags & SCB_FREEZE_QUEUE) {
		scsipi_periph_thaw(periph, 1);
		scb->flags &= ~SCB_FREEZE_QUEUE;
	}

	requeue = scb->flags & SCB_REQUEUE;
	ahcfreescb(ahc, scb);

	if (requeue) {
		xs->error = XS_REQUEUE;
	}
	scsipi_done(xs);
}

/*
 * Determine the number of SCBs available on the controller
 */
int
ahc_probe_scbs(struct ahc_softc *ahc) {
	int i;

	for (i = 0; i < AHC_SCB_MAX; i++) {
		ahc_outb(ahc, SCBPTR, i);
		ahc_outb(ahc, SCB_CONTROL, i);
		if (ahc_inb(ahc, SCB_CONTROL) != i)
			break;
		ahc_outb(ahc, SCBPTR, 0);
		if (ahc_inb(ahc, SCB_CONTROL) != 0)
			break;
	}
	return (i);
}

/*
 * Start the board, ready for normal operation
 */
int
ahc_init(struct ahc_softc *ahc)
{
	int	  max_targ = 15;
	int	  i;
	int	  term;
	u_int	  scsi_conf;
	u_int	  scsiseq_template;
	u_int	  ultraenb;
	u_int	  discenable;
	u_int	  tagenable;
	size_t	  driver_data_size;
	u_int32_t physaddr;

#ifdef AHC_PRINT_SRAM
	printf("Scratch Ram:");
	for (i = 0x20; i < 0x5f; i++) {
		if (((i % 8) == 0) && (i != 0)) {
			printf ("\n              ");
		}
		printf (" 0x%x", ahc_inb(ahc, i));
	}
	if ((ahc->features & AHC_MORE_SRAM) != 0) {
		for (i = 0x70; i < 0x7f; i++) {
			if (((i % 8) == 0) && (i != 0)) {
				printf ("\n              ");
			}
			printf (" 0x%x", ahc_inb(ahc, i));
		}
	}
	printf ("\n");
#endif

	/*
	 * Assume we have a board at this stage and it has been reset.
	 */
	if ((ahc->flags & AHC_USEDEFAULTS) != 0)
		ahc->our_id = ahc->our_id_b = 7;

	/*
	 * Default to allowing initiator operations.
	 */
	ahc->flags |= AHC_INITIATORMODE;

	/*
	 * DMA tag for our command fifos and other data in system memory
	 * the card's sequencer must be able to access.  For initiator
	 * roles, we need to allocate space for the qinfifo, qoutfifo,
	 * and untagged_scb arrays each of which are composed of 256
	 * 1 byte elements.  When providing for the target mode role,
	 * we additionally must provide space for the incoming target
	 * command fifo.
	 */
	driver_data_size = 3 * 256 * sizeof(u_int8_t);

	if (ahc_createdmamem(ahc->parent_dmat, driver_data_size,
	    ahc->sc_dmaflags,
	    &ahc->shared_data_dmamap, (caddr_t *)&ahc->qoutfifo,
	    &ahc->shared_data_busaddr, &ahc->shared_data_seg,
	    &ahc->shared_data_nseg, ahc_name(ahc), "shared data") < 0)
		return (ENOMEM);

	ahc->init_level++;

	/* Allocate SCB data now that parent_dmat is initialized */
	if (ahc->scb_data->maxhscbs == 0)
		if (ahcinitscbdata(ahc) != 0)
			return (ENOMEM);

	ahc->qinfifo = &ahc->qoutfifo[256];
	ahc->untagged_scbs = &ahc->qinfifo[256];
	/* There are no untagged SCBs active yet. */
	for (i = 0; i < 256; i++)
		ahc->untagged_scbs[i] = SCB_LIST_NULL;

	/* All of our queues are empty */
	for (i = 0; i < 256; i++)
		ahc->qoutfifo[i] = SCB_LIST_NULL;

	bus_dmamap_sync(ahc->parent_dmat, ahc->shared_data_dmamap, 0,
	    driver_data_size, BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

	/*
	 * Allocate a tstate to house information for our
	 * initiator presence on the bus as well as the user
	 * data for any target mode initiator.
	 */
	if (ahc_alloc_tstate(ahc, ahc->our_id, 'A') == NULL) {
		printf("%s: unable to allocate tmode_tstate.  "
		       "Failing attach\n", ahc_name(ahc));
		return (-1);
	}

	if ((ahc->features & AHC_TWIN) != 0) {
		if (ahc_alloc_tstate(ahc, ahc->our_id_b, 'B') == NULL) {
			printf("%s: unable to allocate tmode_tstate.  "
			       "Failing attach\n", ahc_name(ahc));
			return (-1);
		}
 		printf("Twin Channel, A SCSI Id=%d, B SCSI Id=%d, primary %c, ",
		       ahc->our_id, ahc->our_id_b,
		       ahc->flags & AHC_CHANNEL_B_PRIMARY? 'B': 'A');
	} else {
		if ((ahc->features & AHC_WIDE) != 0) {
			printf("Wide ");
		} else {
			printf("Single ");
		}
		printf("Channel %c, SCSI Id=%d, ", ahc->channel, ahc->our_id);
	}

	ahc_outb(ahc, SEQ_FLAGS, 0);

	if (ahc->scb_data->maxhscbs < AHC_SCB_MAX) {
		ahc->flags |= AHC_PAGESCBS;
		printf("%d/%d SCBs\n", ahc->scb_data->maxhscbs, AHC_SCB_MAX);
	} else {
		ahc->flags &= ~AHC_PAGESCBS;
		printf("%d SCBs\n", ahc->scb_data->maxhscbs);
	}

#ifdef AHC_DEBUG
	if (ahc_debug & AHC_SHOWMISC) {
		printf("%s: hardware scb %lu bytes; kernel scb %lu bytes; "
		       "ahc_dma %lu bytes\n",
			ahc_name(ahc),
		        (unsigned long) sizeof(struct hardware_scb),
			(unsigned long) sizeof(struct scb),
			(unsigned long) sizeof(struct ahc_dma_seg));
	}
#endif /* AHC_DEBUG */

	/* Set the SCSI Id, SXFRCTL0, SXFRCTL1, and SIMODE1, for both channels*/
	if (ahc->features & AHC_TWIN) {

		/*
		 * The device is gated to channel B after a chip reset,
		 * so set those values first
		 */
		term = (ahc->flags & AHC_TERM_ENB_B) != 0 ? STPWEN : 0;
		if ((ahc->features & AHC_ULTRA2) != 0)
			ahc_outb(ahc, SCSIID_ULTRA2, ahc->our_id_b);
		else
			ahc_outb(ahc, SCSIID, ahc->our_id_b);
		scsi_conf = ahc_inb(ahc, SCSICONF + 1);
		ahc_outb(ahc, SXFRCTL1, (scsi_conf & (ENSPCHK|STIMESEL))
					|term|ENSTIMER|ACTNEGEN);
		ahc_outb(ahc, SIMODE1, ENSELTIMO|ENSCSIRST|ENSCSIPERR);
		ahc_outb(ahc, SXFRCTL0, DFON|SPIOEN);

		if ((scsi_conf & RESET_SCSI) != 0
		 && (ahc->flags & AHC_INITIATORMODE) != 0)
			ahc->flags |= AHC_RESET_BUS_B;

		/* Select Channel A */
		ahc_outb(ahc, SBLKCTL, ahc_inb(ahc, SBLKCTL) & ~SELBUSB);
	}
	term = (ahc->flags & AHC_TERM_ENB_A) != 0 ? STPWEN : 0;
	if ((ahc->features & AHC_ULTRA2) != 0)
		ahc_outb(ahc, SCSIID_ULTRA2, ahc->our_id);
	else
		ahc_outb(ahc, SCSIID, ahc->our_id);
	scsi_conf = ahc_inb(ahc, SCSICONF);
	ahc_outb(ahc, SXFRCTL1, (scsi_conf & (ENSPCHK|STIMESEL))
				|term
				|ENSTIMER|ACTNEGEN);
	ahc_outb(ahc, SIMODE1, ENSELTIMO|ENSCSIRST|ENSCSIPERR);
	ahc_outb(ahc, SXFRCTL0, DFON|SPIOEN);

	if ((scsi_conf & RESET_SCSI) != 0
	 && (ahc->flags & AHC_INITIATORMODE) != 0)
		ahc->flags |= AHC_RESET_BUS_A;

	/*
	 * Look at the information that board initialization or
	 * the board bios has left us.
	 */
	ultraenb = 0;
	tagenable = ALL_TARGETS_MASK;

	/* Grab the disconnection disable table and invert it for our needs */
	if (ahc->flags & AHC_USEDEFAULTS) {
		printf("%s: Host Adapter Bios disabled.  Using default SCSI "
			"device parameters\n", ahc_name(ahc));
		ahc->flags |= AHC_EXTENDED_TRANS_A|AHC_EXTENDED_TRANS_B|
			      AHC_TERM_ENB_A|AHC_TERM_ENB_B;
		discenable = ALL_TARGETS_MASK;
		if ((ahc->features & AHC_ULTRA) != 0)
			ultraenb = ALL_TARGETS_MASK;
	} else {
		discenable = ~((ahc_inb(ahc, DISC_DSB + 1) << 8)
			   | ahc_inb(ahc, DISC_DSB));
		if ((ahc->features & (AHC_ULTRA|AHC_ULTRA2)) != 0)
			ultraenb = (ahc_inb(ahc, ULTRA_ENB + 1) << 8)
				      | ahc_inb(ahc, ULTRA_ENB);
	}

	if ((ahc->features & (AHC_WIDE|AHC_TWIN)) == 0)
		max_targ = 7;

	for (i = 0; i <= max_targ; i++) {
		struct ahc_initiator_tinfo *tinfo;
		struct tmode_tstate *tstate;
		u_int our_id;
		u_int target_id;
		char channel;

		channel = 'A';
		our_id = ahc->our_id;
		target_id = i;
		if (i > 7 && (ahc->features & AHC_TWIN) != 0) {
			channel = 'B';
			our_id = ahc->our_id_b;
			target_id = i % 8;
		}
		tinfo = ahc_fetch_transinfo(ahc, channel, our_id,
					    target_id, &tstate);
		/* Default to async narrow across the board */
		memset(tinfo, 0, sizeof(*tinfo));
		if (ahc->flags & AHC_USEDEFAULTS) {
			if ((ahc->features & AHC_WIDE) != 0)
				tinfo->user.width = MSG_EXT_WDTR_BUS_16_BIT;

			/*
			 * These will be truncated when we determine the
			 * connection type we have with the target.
			 */
			tinfo->user.period = ahc_syncrates->period;
			tinfo->user.offset = ~0;
		} else {
			u_int scsirate;
			u_int16_t mask;

			/* Take the settings leftover in scratch RAM. */
			scsirate = ahc_inb(ahc, TARG_SCSIRATE + i);
			mask = (0x01 << i);
			if ((ahc->features & AHC_ULTRA2) != 0) {
				u_int offset;
				u_int maxsync;

				if ((scsirate & SOFS) == 0x0F) {
					/*
					 * Haven't negotiated yet,
					 * so the format is different.
					 */
					scsirate = (scsirate & SXFR) >> 4
						 | (ultraenb & mask)
						  ? 0x08 : 0x0
						 | (scsirate & WIDEXFER);
					offset = MAX_OFFSET_ULTRA2;
				} else
					offset = ahc_inb(ahc, TARG_OFFSET + i);
				maxsync = AHC_SYNCRATE_ULTRA2;
				if ((ahc->features & AHC_DT) != 0)
					maxsync = AHC_SYNCRATE_DT;
				tinfo->user.period =
				    ahc_find_period(ahc, scsirate, maxsync);
				if (offset == 0)
					tinfo->user.period = 0;
				else
					tinfo->user.offset = ~0;
			} else if ((scsirate & SOFS) != 0) {
				tinfo->user.period =
				    ahc_find_period(ahc, scsirate,
						    (ultraenb & mask)
						   ? AHC_SYNCRATE_ULTRA
						   : AHC_SYNCRATE_FAST);
				if (tinfo->user.period != 0)
					tinfo->user.offset = ~0;
			}
			if ((scsirate & WIDEXFER) != 0
			 && (ahc->features & AHC_WIDE) != 0)
				tinfo->user.width = MSG_EXT_WDTR_BUS_16_BIT;
		}
		tinfo->goal = tinfo->user; /* force negotiation */
		tstate->ultraenb = ultraenb;
		tstate->discenable = discenable;
		tstate->tagenable = 0; /* Wait until the XPT says its okay */
		tstate->tagdisable = 0;
	}
	ahc->user_discenable = discenable;
	ahc->user_tagenable = tagenable;

	/*
	 * Tell the sequencer where it can find our arrays in memory.
	 */
	physaddr = ahc->scb_data->hscb_busaddr;
	ahc_outb(ahc, HSCB_ADDR, physaddr & 0xFF);
	ahc_outb(ahc, HSCB_ADDR + 1, (physaddr >> 8) & 0xFF);
	ahc_outb(ahc, HSCB_ADDR + 2, (physaddr >> 16) & 0xFF);
	ahc_outb(ahc, HSCB_ADDR + 3, (physaddr >> 24) & 0xFF);

	physaddr = ahc->shared_data_busaddr;
	ahc_outb(ahc, SCBID_ADDR, physaddr & 0xFF);
	ahc_outb(ahc, SCBID_ADDR + 1, (physaddr >> 8) & 0xFF);
	ahc_outb(ahc, SCBID_ADDR + 2, (physaddr >> 16) & 0xFF);
	ahc_outb(ahc, SCBID_ADDR + 3, (physaddr >> 24) & 0xFF);

	/* Target mode incomding command fifo */
	physaddr += 3 * 256 * sizeof(u_int8_t);
	ahc_outb(ahc, TMODE_CMDADDR, physaddr & 0xFF);
	ahc_outb(ahc, TMODE_CMDADDR + 1, (physaddr >> 8) & 0xFF);
	ahc_outb(ahc, TMODE_CMDADDR + 2, (physaddr >> 16) & 0xFF);
	ahc_outb(ahc, TMODE_CMDADDR + 3, (physaddr >> 24) & 0xFF);

	/*
	 * Initialize the group code to command length table.
	 * This overrides the values in TARG_SCSIRATE, so only
	 * setup the table after we have processed that information.
	 */
	ahc_outb(ahc, CMDSIZE_TABLE, 5);
	ahc_outb(ahc, CMDSIZE_TABLE + 1, 9);
	ahc_outb(ahc, CMDSIZE_TABLE + 2, 9);
	ahc_outb(ahc, CMDSIZE_TABLE + 3, 0);
	ahc_outb(ahc, CMDSIZE_TABLE + 4, 15);
	ahc_outb(ahc, CMDSIZE_TABLE + 5, 11);
	ahc_outb(ahc, CMDSIZE_TABLE + 6, 0);
	ahc_outb(ahc, CMDSIZE_TABLE + 7, 0);

	/* Tell the sequencer of our initial queue positions */
	ahc_outb(ahc, KERNEL_QINPOS, 0);
	ahc_outb(ahc, QINPOS, 0);
	ahc_outb(ahc, QOUTPOS, 0);

#ifdef AHC_DEBUG
	if (ahc_debug & AHC_SHOWMISC)
		printf("DISCENABLE == 0x%x\nULTRAENB == 0x%x\n",
		       discenable, ultraenb);
#endif

	/* Don't have any special messages to send to targets */
	ahc_outb(ahc, TARGET_MSG_REQUEST, 0);
	ahc_outb(ahc, TARGET_MSG_REQUEST + 1, 0);

	/*
	 * Use the built in queue management registers
	 * if they are available.
	 */
	if ((ahc->features & AHC_QUEUE_REGS) != 0) {
		ahc_outb(ahc, QOFF_CTLSTA, SCB_QSIZE_256);
		ahc_outb(ahc, SDSCB_QOFF, 0);
		ahc_outb(ahc, SNSCB_QOFF, 0);
		ahc_outb(ahc, HNSCB_QOFF, 0);
	}


	/* We don't have any waiting selections */
	ahc_outb(ahc, WAITING_SCBH, SCB_LIST_NULL);

	/* Our disconnection list is empty too */
	ahc_outb(ahc, DISCONNECTED_SCBH, SCB_LIST_NULL);

	/* Message out buffer starts empty */
	ahc_outb(ahc, MSG_OUT, MSG_NOOP);

	/*
	 * Setup the allowed SCSI Sequences based on operational mode.
	 * If we are a target, we'll enable select in operations once
	 * we've had a lun enabled.
	 */
	scsiseq_template = ENSELO|ENAUTOATNO|ENAUTOATNP;
	if ((ahc->flags & AHC_INITIATORMODE) != 0)
		scsiseq_template |= ENRSELI;
	ahc_outb(ahc, SCSISEQ_TEMPLATE, scsiseq_template);

	/*
	 * Load the Sequencer program and Enable the adapter
	 * in "fast" mode.
         */
#ifdef AHC_DEBUG
	printf("%s: Downloading Sequencer Program...",
	       ahc_name(ahc));
#endif

	ahc_loadseq(ahc);

	/* We have to wait until after any system dumps... */
	shutdownhook_establish(ahc_shutdown, ahc);

	return (0);
}

static int
ahc_ioctl(struct scsipi_channel *channel, u_long cmd, caddr_t addr, int flag,
	  struct proc *p)
{
	struct ahc_softc *ahc = (void *)channel->chan_adapter->adapt_dev;
	int s, ret = ENOTTY;

	switch (cmd) {
	case SCBUSIORESET:
		s = splbio();
		ahc_reset_channel(ahc, channel->chan_channel == 1 ? 'B' : 'A',
		    TRUE);
		splx(s);
		ret = 0;
		break;
	default:
		break;
	}

	return ret;
}


/*
 * XXX fvdl the busy_tcl checks and settings should only be done
 * for the non-tagged queueing case, but we don't do tagged queueing
 * yet, so..
 */
static void
ahc_action(struct scsipi_channel *chan, scsipi_adapter_req_t req, void *arg)
{
	struct scsipi_xfer *xs;
	struct scsipi_periph *periph;
	struct ahc_softc *ahc = (void *)chan->chan_adapter->adapt_dev;
	struct scb *scb;
	struct hardware_scb *hscb;
	struct ahc_initiator_tinfo *tinfo;
	struct tmode_tstate *tstate;
	u_int target_id;
	u_int our_id;
	int s, tcl;
	u_int16_t mask;
	char channel;

	switch (req) {
	case ADAPTER_REQ_RUN_XFER:
		xs = arg;
		periph = xs->xs_periph;

		SC_DEBUG(xs->xs_periph, SCSIPI_DB3, ("ahc_action\n"));

		/* must protect the queue */
		s = splbio();

		tcl = XS_TCL(ahc, xs);

		if (!ahc_istagged_device(ahc, xs, 0) &&
		     ahc_index_busy_tcl(ahc, tcl, FALSE) != SCB_LIST_NULL) {
			panic("ahc_action: not tagged and device busy");
		}


		target_id = periph->periph_target;
		our_id = SIM_SCSI_ID(ahc, periph);

		/*
		 * get an scb to use.
		 */
		if ((scb = ahcgetscb(ahc)) == NULL) {
			xs->error = XS_RESOURCE_SHORTAGE;
			scsipi_done(xs);
			splx(s);
			return;
		}

		tcl = XS_TCL(ahc, xs);

#ifdef DIAGNOSTIC
		if (!ahc_istagged_device(ahc, xs, 0) &&
		    ahc_index_busy_tcl(ahc, tcl, FALSE) != SCB_LIST_NULL)
			panic("ahc: queuing for busy target");
#endif

		scb->xs = xs;
		hscb = scb->hscb;
		hscb->tcl = tcl;

		if (xs->xs_tag_type) {
#ifdef DIAGNOSTIC
		if (ahc_istagged_device(ahc, xs, 0) == 0)
			panic("ahc_action: taggged command for untagged device");
#endif
			scb->hscb->control |= TAG_ENB;
		} else
			ahc_busy_tcl(ahc, scb);

		splx(s);

		channel = SIM_CHANNEL(ahc, periph);
		if (ahc->inited_channels[channel - 'A'] == 0) {
			if ((channel == 'A' &&
			     (ahc->flags & AHC_RESET_BUS_A)) ||
			    (channel == 'B' &&
			     (ahc->flags & AHC_RESET_BUS_B))) {
				s = splbio();
				ahc_reset_channel(ahc, channel, TRUE);
				splx(s);
			}
			ahc->inited_channels[channel - 'A'] = 1;
		}

		/*
		 * Put all the arguments for the xfer in the scb
		 */

		mask = SCB_TARGET_MASK(scb);
		tinfo = ahc_fetch_transinfo(ahc,
			    SIM_CHANNEL(ahc, periph),
			    our_id, target_id, &tstate);
		if (ahc->inited_targets[target_id] == 0) {
			struct ahc_devinfo devinfo;
			s = splbio();
			ahc_compile_devinfo(&devinfo, our_id, target_id,
			    periph->periph_lun,
			    SIM_CHANNEL(ahc, periph),
			    ROLE_INITIATOR);
			ahc_update_target_msg_request(ahc, &devinfo, tinfo,
			    TRUE, FALSE);
			ahc->inited_targets[target_id] = 1;
			splx(s);
		}
		hscb->scsirate = tinfo->scsirate;
		hscb->scsioffset = tinfo->current.offset;
		if ((tstate->ultraenb & mask) != 0)
			hscb->control |= ULTRAENB;

		if ((tstate->discenable & mask) != 0)
			hscb->control |= DISCENB;

		if (xs->xs_control & XS_CTL_RESET) {
			hscb->cmdpointer = 0;
			scb->flags |= SCB_DEVICE_RESET;
			hscb->control |= MK_MESSAGE;
			ahc_execute_scb(scb, NULL, 0);
		}

		ahc_setup_data(ahc, xs, scb);
		return;
	case ADAPTER_REQ_GROW_RESOURCES:
		/* XXX not supported */
		return;
	case ADAPTER_REQ_SET_XFER_MODE:
	{
		struct scsipi_xfer_mode *xm = arg;
		struct ahc_devinfo devinfo;
		int target_id, our_id;
		char channel;

		target_id = xm->xm_target;	
		our_id = chan->chan_id;
		channel = (chan->chan_channel == 1) ? 'B' : 'A';
		s = splbio();
		tinfo = ahc_fetch_transinfo(ahc, channel, our_id, target_id,
		    &tstate);
		ahc_compile_devinfo(&devinfo, our_id, target_id,
		    0, channel, ROLE_INITIATOR);
		ahc->inited_targets[target_id] = 2;
		if (xm->xm_mode & PERIPH_CAP_TQING &&
		    (tstate->tagdisable & devinfo.target_mask) == 0) {
			ahc_set_tags(ahc, &devinfo, TRUE);
		}
		splx(s);
		ahc_update_xfer_mode(ahc, &devinfo);
	}
	}
}

static void
ahc_execute_scb(void *arg, bus_dma_segment_t *dm_segs, int nsegments)
{
	struct	 scb *scb;
	struct scsipi_xfer *xs;
	struct	 ahc_softc *ahc;
	int	 s;

	scb = (struct scb *)arg;
	xs = scb->xs;
	ahc = (void *)xs->xs_periph->periph_channel->chan_adapter->adapt_dev;


	if (nsegments != 0) {
		struct	  ahc_dma_seg *sg;
		bus_dma_segment_t *end_seg;
		int op;

		end_seg = dm_segs + nsegments;

		/* Copy the first SG into the data pointer area */
		scb->hscb->data = dm_segs->ds_addr;
		scb->hscb->datalen = dm_segs->ds_len;

		/* Copy the segments into our SG list */
		sg = scb->sg_list;
		while (dm_segs < end_seg) {
			sg->addr = dm_segs->ds_addr;
			sg->len = dm_segs->ds_len;
			ahc_swap_sg(sg);
			sg++;
			dm_segs++;
		}

		/* Note where to find the SG entries in bus space */
		scb->hscb->SG_pointer = scb->sg_list_phys;

		if (xs->xs_control & XS_CTL_DATA_IN)
			op = BUS_DMASYNC_PREREAD;
		else
			op = BUS_DMASYNC_PREWRITE;

		bus_dmamap_sync(ahc->parent_dmat, scb->dmamap, 0,
		    scb->dmamap->dm_mapsize, op);

	} else {
		scb->hscb->SG_pointer = 0;
		scb->hscb->data = 0;
		scb->hscb->datalen = 0;
	}

	scb->sg_count = scb->hscb->SG_count = nsegments;

	s = splbio();

	/*
	 * Last time we need to check if this SCB needs to
	 * be aborted.
	 */
	if (xs->xs_status & XS_STS_DONE) {
		if (!ahc_istagged_device(ahc, xs, 0))
			ahc_index_busy_tcl(ahc, scb->hscb->tcl, TRUE);
		if (nsegments != 0)
			bus_dmamap_unload(ahc->parent_dmat, scb->dmamap);
		ahcfreescb(ahc, scb);
		splx(s);
		return;
	}

#ifdef DIAGNOSTIC
	if (scb->sg_count > 255)
		panic("ahc bad sg_count");
#endif

	ahc_swap_hscb(scb->hscb);

	LIST_INSERT_HEAD(&ahc->pending_ccbs, scb, plinks);

	scb->flags |= SCB_ACTIVE;

	if (!(xs->xs_control & XS_CTL_POLL))
		callout_reset(&scb->xs->xs_callout, (xs->timeout * hz) / 1000,
		    ahc_timeout, scb);

	if ((scb->flags & SCB_TARGET_IMMEDIATE) != 0) {
#if 0
		printf("Continueing Immediate Command %d:%d\n",
		       xs->xs_periph->periph_target,
		       xs->xs_periph->periph_lun);
#endif
		pause_sequencer(ahc);
		if ((ahc->flags & AHC_PAGESCBS) == 0)
			ahc_outb(ahc, SCBPTR, scb->hscb->tag);
		ahc_outb(ahc, SCB_TAG, scb->hscb->tag);
		ahc_outb(ahc, RETURN_1, CONT_MSG_LOOP);
		unpause_sequencer(ahc);
	} else {

#if 0
		printf("tag %x at qpos %u vaddr %p paddr 0x%lx\n",
		    scb->hscb->tag, ahc->qinfifonext,
		    &ahc->qinfifo[ahc->qinfifonext],
		    ahc->shared_data_busaddr + 1024 + ahc->qinfifonext);
#endif

		ahc->qinfifo[ahc->qinfifonext++] = scb->hscb->tag;

		bus_dmamap_sync(ahc->parent_dmat, ahc->shared_data_dmamap,
		    QINFIFO_OFFSET * 256, 256, BUS_DMASYNC_PREWRITE);

		if ((ahc->features & AHC_QUEUE_REGS) != 0) {
			ahc_outb(ahc, HNSCB_QOFF, ahc->qinfifonext);
		} else {
			pause_sequencer(ahc);
			ahc_outb(ahc, KERNEL_QINPOS, ahc->qinfifonext);
			unpause_sequencer(ahc);
		}
	}

#ifdef AHC_DEBUG
	if (ahc_debug & AHC_SHOWCMDS) {
		scsi_print_addr(xs->xs_periph);
		printf("opcode %d tag %x len %d flags %x control %x fpos %u"
		    " rate %x\n",
		    xs->cmdstore.opcode, scb->hscb->tag, scb->hscb->datalen,
		    scb->flags, scb->hscb->control, ahc->qinfifonext,
		    scb->hscb->scsirate);
	}
#endif

	if (!(xs->xs_control & XS_CTL_POLL)) {
		splx(s);
		return;
	}
	/*
	 * If we can't use interrupts, poll for completion
	 */
	SC_DEBUG(xs->xs_periph, SCSIPI_DB3, ("cmd_poll\n"));
	do {
		if (ahc_poll(ahc, xs->timeout)) {
			if (!(xs->xs_control & XS_CTL_SILENT))
				printf("cmd fail\n");
			ahc_timeout(scb);
			break;
		}
	} while (!(xs->xs_status & XS_STS_DONE));
	splx(s);
	return;
}

static int
ahc_poll(struct ahc_softc *ahc, int wait)
{
	while (--wait) {
		DELAY(1000);
		if (ahc_inb(ahc, INTSTAT) & INT_PEND)
			break;
	}

	if (wait == 0) {
		printf("%s: board is not responding\n", ahc_name(ahc));
		return (EIO);
	}

	ahc_intr((void *)ahc);
	return (0);
}

static void
ahc_setup_data(struct ahc_softc *ahc, struct scsipi_xfer *xs,
	       struct scb *scb)
{
	struct hardware_scb *hscb;

	hscb = scb->hscb;
	xs->resid = xs->status = 0;

	hscb->cmdlen = xs->cmdlen;
	memcpy(hscb->cmdstore, xs->cmd, xs->cmdlen);
	hscb->cmdpointer = hscb->cmdstore_busaddr;

	/* Only use S/G if there is a transfer */
	if (xs->datalen) {
		int error;

		error = bus_dmamap_load(ahc->parent_dmat,
			    scb->dmamap, xs->data,
			    xs->datalen, NULL,
			    ((xs->xs_control & XS_CTL_NOSLEEP) ?
			     BUS_DMA_NOWAIT : BUS_DMA_WAITOK) |
			    BUS_DMA_STREAMING |
			    ((xs->xs_control & XS_CTL_DATA_IN) ?
			     BUS_DMA_READ : BUS_DMA_WRITE));
		if (error) {
#ifdef AHC_DEBUG
			printf("%s: in ahc_setup_data(): bus_dmamap_load() "
			    "= %d\n",
			    ahc_name(ahc),
			    error);
#endif
			if (!ahc_istagged_device(ahc, xs, 0))
				ahc_index_busy_tcl(ahc, hscb->tcl, TRUE);
			xs->error = XS_RESOURCE_SHORTAGE;	/* XXX fvdl */
			scsipi_done(xs);
			return;
		}
		ahc_execute_scb(scb,
		    scb->dmamap->dm_segs,
		    scb->dmamap->dm_nsegs);
	} else {
		ahc_execute_scb(scb, NULL, 0);
	}
}

static void
ahc_freeze_devq(struct ahc_softc *ahc, struct scsipi_periph *periph)
{
	int	target;
	char	channel;
	int	lun;

	target = periph->periph_target;
	lun = periph->periph_lun;
	channel = periph->periph_channel->chan_channel;

	ahc_search_qinfifo(ahc, target, channel, lun,
			   /*tag*/SCB_LIST_NULL, ROLE_UNKNOWN,
			   SCB_REQUEUE, SEARCH_COMPLETE);
}

static void
ahcallocscbs(struct ahc_softc *ahc)
{
	struct scb_data *scb_data;
	struct scb *next_scb;
	struct sg_map_node *sg_map;
	bus_addr_t physaddr;
	struct ahc_dma_seg *segs;
	int newcount;
	int i;

	scb_data = ahc->scb_data;
	if (scb_data->numscbs >= AHC_SCB_MAX)
		/* Can't allocate any more */
		return;

	next_scb = &scb_data->scbarray[scb_data->numscbs];

	sg_map = malloc(sizeof(*sg_map), M_DEVBUF, M_NOWAIT);

	if (sg_map == NULL)
		return;

	if (ahc_createdmamem(ahc->parent_dmat, PAGE_SIZE, ahc->sc_dmaflags,
	    &sg_map->sg_dmamap,
	    (caddr_t *)&sg_map->sg_vaddr, &sg_map->sg_physaddr,
	    &sg_map->sg_dmasegs, &sg_map->sg_nseg, ahc_name(ahc),
	    "SG space") < 0) {
		free(sg_map, M_DEVBUF);
		return;
	}

	SLIST_INSERT_HEAD(&scb_data->sg_maps, sg_map, links);

	segs = sg_map->sg_vaddr;
	physaddr = sg_map->sg_physaddr;

	newcount = (PAGE_SIZE / (AHC_NSEG * sizeof(struct ahc_dma_seg)));
	for (i = 0; scb_data->numscbs < AHC_SCB_MAX && i < newcount; i++) {
		int error;

		next_scb->sg_list = segs;
		/*
		 * The sequencer always starts with the second entry.
		 * The first entry is embedded in the scb.
		 */
		next_scb->sg_list_phys = physaddr + sizeof(struct ahc_dma_seg);
		next_scb->flags = SCB_FREE;
		error = bus_dmamap_create(ahc->parent_dmat,
			    AHC_MAXTRANSFER_SIZE, AHC_NSEG, MAXBSIZE, 0,
			    BUS_DMA_NOWAIT|BUS_DMA_ALLOCNOW|ahc->sc_dmaflags,
			    &next_scb->dmamap);
		if (error != 0)
			break;
		next_scb->hscb = &scb_data->hscbs[scb_data->numscbs];
		next_scb->hscb->tag = ahc->scb_data->numscbs;
		next_scb->hscb->cmdstore_busaddr =
		    ahc_hscb_busaddr(ahc, next_scb->hscb->tag)
		  + offsetof(struct hardware_scb, cmdstore);
		next_scb->hscb->cmdstore_busaddr =
		    htole32(next_scb->hscb->cmdstore_busaddr);
		SLIST_INSERT_HEAD(&ahc->scb_data->free_scbs, next_scb, links);
		segs += AHC_NSEG;
		physaddr += (AHC_NSEG * sizeof(struct ahc_dma_seg));
		next_scb++;
		ahc->scb_data->numscbs++;
	}
#ifdef AHC_DEBUG
	if (ahc_debug & AHC_SHOWSCBALLOC)
		printf("%s: allocated %d new SCBs count now %d\n",
		    ahc_name(ahc), i - 1, ahc->scb_data->numscbs);
#endif
}

#ifdef AHC_DUMP_SEQ
static void
ahc_dumpseq(struct ahc_softc* ahc)
{
	int i;
	int max_prog;

	if ((ahc->chip & AHC_BUS_MASK) < AHC_PCI)
		max_prog = 448;
	else if ((ahc->features & AHC_ULTRA2) != 0)
		max_prog = 768;
	else
		max_prog = 512;

	ahc_outb(ahc, SEQCTL, PERRORDIS|FAILDIS|FASTMODE|LOADRAM);
	ahc_outb(ahc, SEQADDR0, 0);
	ahc_outb(ahc, SEQADDR1, 0);
	for (i = 0; i < max_prog; i++) {
		u_int8_t ins_bytes[4];

		ahc_insb(ahc, SEQRAM, ins_bytes, 4);
		printf("0x%08x\n", ins_bytes[0] << 24
				 | ins_bytes[1] << 16
				 | ins_bytes[2] << 8
				 | ins_bytes[3]);
	}
}
#endif

static void
ahc_loadseq(struct ahc_softc *ahc)
{
	const struct patch *cur_patch;
	int i;
	int downloaded;
	int skip_addr;
	u_int8_t download_consts[4];

	/* Setup downloadable constant table */
#if 0
	/* No downloaded constants are currently defined. */
	download_consts[TMODE_NUMCMDS] = ahc->num_targetcmds;
#endif

	cur_patch = patches;
	downloaded = 0;
	skip_addr = 0;
	ahc_outb(ahc, SEQCTL, PERRORDIS|FAILDIS|FASTMODE|LOADRAM);
	ahc_outb(ahc, SEQADDR0, 0);
	ahc_outb(ahc, SEQADDR1, 0);

	for (i = 0; i < sizeof(seqprog)/4; i++) {
		if (ahc_check_patch(ahc, &cur_patch, i, &skip_addr) == 0) {
			/*
			 * Don't download this instruction as it
			 * is in a patch that was removed.
			 */
                        continue;
		}
		ahc_download_instr(ahc, i, download_consts);
		downloaded++;
	}
	ahc_outb(ahc, SEQCTL, PERRORDIS|FAILDIS|FASTMODE);
	restart_sequencer(ahc);

#ifdef AHC_DEBUG
	printf(" %d instructions downloaded\n", downloaded);
#endif
}

static int
ahc_check_patch(struct ahc_softc *ahc, const struct patch **start_patch,
		int start_instr, int *skip_addr)
{
	const struct	patch *cur_patch;
	const struct	patch *last_patch;
	int	num_patches;

	num_patches = sizeof(patches)/sizeof(struct patch);
	last_patch = &patches[num_patches];
	cur_patch = *start_patch;

	while (cur_patch < last_patch && start_instr == cur_patch->begin) {

		if (cur_patch->patch_func(ahc) == 0) {

			/* Start rejecting code */
			*skip_addr = start_instr + cur_patch->skip_instr;
			cur_patch += cur_patch->skip_patch;
		} else {
			/* Accepted this patch.  Advance to the next
			 * one and wait for our intruction pointer to
			 * hit this point.
			 */
			cur_patch++;
		}
	}

	*start_patch = cur_patch;
	if (start_instr < *skip_addr)
		/* Still skipping */
		return (0);

	return (1);
}

static void
ahc_download_instr(struct ahc_softc *ahc, int instrptr, u_int8_t *dconsts)
{
	union	ins_formats instr;
	struct	ins_format1 *fmt1_ins;
	struct	ins_format3 *fmt3_ins;
	u_int	opcode;

	/* Structure copy */
	memcpy(&instr, &seqprog[instrptr * 4], sizeof instr);

	instr.integer = le32toh(instr.integer);

	fmt1_ins = &instr.format1;
	fmt3_ins = NULL;

	/* Pull the opcode */
	opcode = instr.format1.opcode;
	switch (opcode) {
	case AIC_OP_JMP:
	case AIC_OP_JC:
	case AIC_OP_JNC:
	case AIC_OP_CALL:
	case AIC_OP_JNE:
	case AIC_OP_JNZ:
	case AIC_OP_JE:
	case AIC_OP_JZ:
	{
		const struct patch *cur_patch;
		int address_offset;
		u_int address;
		int skip_addr;
		int i;

		fmt3_ins = &instr.format3;
		address_offset = 0;
		address = fmt3_ins->address;
		cur_patch = patches;
		skip_addr = 0;

		for (i = 0; i < address;) {

			ahc_check_patch(ahc, &cur_patch, i, &skip_addr);

			if (skip_addr > i) {
				int end_addr;

				end_addr = MIN(address, skip_addr);
				address_offset += end_addr - i;
				i = skip_addr;
			} else {
				i++;
			}
		}
		address -= address_offset;
		fmt3_ins->address = address;
		/* FALLTHROUGH */
	}
	case AIC_OP_OR:
	case AIC_OP_AND:
	case AIC_OP_XOR:
	case AIC_OP_ADD:
	case AIC_OP_ADC:
	case AIC_OP_BMOV:
		if (fmt1_ins->parity != 0) {
			fmt1_ins->immediate = dconsts[fmt1_ins->immediate];
		}
		fmt1_ins->parity = 0;
		/* FALLTHROUGH */
	case AIC_OP_ROL:
		if ((ahc->features & AHC_ULTRA2) != 0) {
			int i, count;

			/* Calculate odd parity for the instruction */
			for (i = 0, count = 0; i < 31; i++) {
				u_int32_t mask;

				mask = 0x01 << i;
				if ((instr.integer & mask) != 0)
					count++;
			}
			if ((count & 0x01) == 0)
				instr.format1.parity = 1;
		} else {
			/* Compress the instruction for older sequencers */
			if (fmt3_ins != NULL) {
				instr.integer =
					fmt3_ins->immediate
				      | (fmt3_ins->source << 8)
				      | (fmt3_ins->address << 16)
				      |	(fmt3_ins->opcode << 25);
			} else {
				instr.integer =
					fmt1_ins->immediate
				      | (fmt1_ins->source << 8)
				      | (fmt1_ins->destination << 16)
				      |	(fmt1_ins->ret << 24)
				      |	(fmt1_ins->opcode << 25);
			}
		}
		instr.integer = htole32(instr.integer);
		ahc_outsb(ahc, SEQRAM, instr.bytes, 4);
		break;
	default:
		panic("Unknown opcode encountered in seq program");
		break;
	}
}

static void
ahc_set_recoveryscb(struct ahc_softc *ahc, struct scb *scb)
{

	if ((scb->flags & SCB_RECOVERY_SCB) == 0) {
		struct scb *scbp;

		scb->flags |= SCB_RECOVERY_SCB;

		/*
		 * Take all queued, but not sent SCBs out of the equation.
		 * Also ensure that no new CCBs are queued to us while we
		 * try to fix this problem.
		 */
		scsipi_channel_freeze(&ahc->sc_channel, 1);
		if (ahc->features & AHC_TWIN)
			scsipi_channel_freeze(&ahc->sc_channel_b, 1);

		/*
		 * Go through all of our pending SCBs and remove
		 * any scheduled timeouts for them.  We will reschedule
		 * them after we've successfully fixed this problem.
		 */
		scbp = ahc->pending_ccbs.lh_first;
		while (scbp != NULL) {
			callout_stop(&scbp->xs->xs_callout);
			scbp = scbp->plinks.le_next;
		}
	}
}

static void
ahc_timeout(void *arg)
{
	struct	scb *scb;
	struct	ahc_softc *ahc;
	int	s, found;
	u_int	last_phase;
	int	target;
	int	lun;
	int	i;
	char	channel;

	scb = (struct scb *)arg;
	ahc =
	    (void *)scb->xs->xs_periph->periph_channel->chan_adapter->adapt_dev;

	s = splbio();

	/*
	 * Ensure that the card doesn't do anything
	 * behind our back.  Also make sure that we
	 * didn't "just" miss an interrupt that would
	 * affect this timeout.
	 */
	do {
		ahc_intr(ahc);
		pause_sequencer(ahc);
	} while (ahc_inb(ahc, INTSTAT) & INT_PEND);

	if ((scb->flags & SCB_ACTIVE) == 0) {
		/* Previous timeout took care of me already */
		printf("Timedout SCB handled by another timeout\n");
		unpause_sequencer(ahc);
		splx(s);
		return;
	}

	target = SCB_TARGET(scb);
	channel = SCB_CHANNEL(scb);
	lun = SCB_LUN(scb);

	scsipi_printaddr(scb->xs->xs_periph);
	printf("SCB %x - timed out ", scb->hscb->tag);
	/*
	 * Take a snapshot of the bus state and print out
	 * some information so we can track down driver bugs.
	 */
	last_phase = ahc_inb(ahc, LASTPHASE);

	for (i = 0; i < num_phases; i++) {
		if (last_phase == phase_table[i].phase)
			break;
	}
	printf("%s", phase_table[i].phasemsg);

	printf(", SEQADDR == 0x%x\n",
	       ahc_inb(ahc, SEQADDR0) | (ahc_inb(ahc, SEQADDR1) << 8));
	printf("SCSIRATE == 0x%x\n", ahc_inb(ahc, SCSIRATE));

#ifdef AHC_DEBUG
	ahc_print_scb(scb);
#endif

#if 0
	printf("SSTAT1 == 0x%x\n", ahc_inb(ahc, SSTAT1));
	printf("SSTAT3 == 0x%x\n", ahc_inb(ahc, SSTAT3));
	printf("SCSIPHASE == 0x%x\n", ahc_inb(ahc, SCSIPHASE));
	printf("SCSIOFFSET == 0x%x\n", ahc_inb(ahc, SCSIOFFSET));
	printf("SEQ_FLAGS == 0x%x\n", ahc_inb(ahc, SEQ_FLAGS));
	printf("SCB_DATAPTR == 0x%x\n", ahc_inb(ahc, SCB_DATAPTR)
				      | ahc_inb(ahc, SCB_DATAPTR + 1) << 8
				      | ahc_inb(ahc, SCB_DATAPTR + 2) << 16
				      | ahc_inb(ahc, SCB_DATAPTR + 3) << 24);
	printf("SCB_DATACNT == 0x%x\n", ahc_inb(ahc, SCB_DATACNT)
				      | ahc_inb(ahc, SCB_DATACNT + 1) << 8
				      | ahc_inb(ahc, SCB_DATACNT + 2) << 16);
	printf("SCB_SGCOUNT == 0x%x\n", ahc_inb(ahc, SCB_SGCOUNT));
	printf("CCSCBCTL == 0x%x\n", ahc_inb(ahc, CCSCBCTL));
	printf("CCSCBCNT == 0x%x\n", ahc_inb(ahc, CCSCBCNT));
	printf("DFCNTRL == 0x%x\n", ahc_inb(ahc, DFCNTRL));
	printf("DFSTATUS == 0x%x\n", ahc_inb(ahc, DFSTATUS));
	printf("CCHCNT == 0x%x\n", ahc_inb(ahc, CCHCNT));
	if (scb->sg_count > 0) {
		for (i = 0; i < scb->sg_count; i++) {
			printf("sg[%d] - Addr 0x%x : Length %d\n",
			       i,
			       le32toh(scb->sg_list[i].addr),
			       le32toh(scb->sg_list[i].len));
		}
	}
#endif
	if (scb->flags & (SCB_DEVICE_RESET|SCB_ABORT)) {
		/*
		 * Been down this road before.
		 * Do a full bus reset.
		 */
bus_reset:
		ahcsetccbstatus(scb->xs, XS_TIMEOUT);
		found = ahc_reset_channel(ahc, channel, /*Initiate Reset*/TRUE);
		printf("%s: Issued Channel %c Bus Reset. "
		       "%d SCBs aborted\n", ahc_name(ahc), channel, found);
	} else {
		/*
		 * If we are a target, transition to bus free and report
		 * the timeout.
		 *
		 * The target/initiator that is holding up the bus may not
		 * be the same as the one that triggered this timeout
		 * (different commands have different timeout lengths).
		 * If the bus is idle and we are actiing as the initiator
		 * for this request, queue a BDR message to the timed out
		 * target.  Otherwise, if the timed out transaction is
		 * active:
		 *   Initiator transaction:
		 *	Stuff the message buffer with a BDR message and assert
		 *	ATN in the hopes that the target will let go of the bus
		 *	and go to the mesgout phase.  If this fails, we'll
		 *	get another timeout 2 seconds later which will attempt
		 *	a bus reset.
		 *
		 *   Target transaction:
		 *	Transition to BUS FREE and report the error.
		 *	It's good to be the target!
		 */
		u_int active_scb_index;

		active_scb_index = ahc_inb(ahc, SCB_TAG);

		if (last_phase != P_BUSFREE
		  && (active_scb_index < ahc->scb_data->numscbs)) {
			struct scb *active_scb;

			/*
			 * If the active SCB is not from our device,
			 * assume that another device is hogging the bus
			 * and wait for it's timeout to expire before
			 * taking additional action.
			 */
			active_scb = &ahc->scb_data->scbarray[active_scb_index];
			if (active_scb->hscb->tcl != scb->hscb->tcl) {
				u_int	newtimeout;

				scsipi_printaddr(scb->xs->xs_periph);
				printf("Other SCB Timeout%s",
			 	       (scb->flags & SCB_OTHERTCL_TIMEOUT) != 0
				       ? " again\n" : "\n");
				scb->flags |= SCB_OTHERTCL_TIMEOUT;
				newtimeout = MAX(active_scb->xs->timeout,
						 scb->xs->timeout);
				callout_reset(&scb->xs->xs_callout,
				    (newtimeout * hz) / 1000,
				    ahc_timeout, scb);
				splx(s);
				return;
			}

			/* It's us */
			if ((scb->hscb->control & TARGET_SCB) != 0) {

				/*
				 * Send back any queued up transactions
				 * and properly record the error condition.
				 */
				ahc_freeze_devq(ahc, scb->xs->xs_periph);
				ahcsetccbstatus(scb->xs, XS_TIMEOUT);
				ahc_freeze_ccb(scb);
				ahc_done(ahc, scb);

				/* Will clear us from the bus */
				restart_sequencer(ahc);
				splx(s);
				return;
			}

			ahc_set_recoveryscb(ahc, active_scb);
			ahc_outb(ahc, MSG_OUT, MSG_BUS_DEV_RESET);
			ahc_outb(ahc, SCSISIGO, last_phase|ATNO);
			scsipi_printaddr(active_scb->xs->xs_periph);
			printf("BDR message in message buffer\n");
			active_scb->flags |=  SCB_DEVICE_RESET;
			callout_reset(&active_scb->xs->xs_callout,
			    2 * hz, ahc_timeout, active_scb);
			unpause_sequencer(ahc);
		} else {
			int	 disconnected;

			/* XXX Shouldn't panic.  Just punt instead */
			if ((scb->hscb->control & TARGET_SCB) != 0)
				panic("Timed-out target SCB but bus idle");

			if (last_phase != P_BUSFREE
			 && (ahc_inb(ahc, SSTAT0) & TARGET) != 0) {
				/* XXX What happened to the SCB? */
				/* Hung target selection.  Goto busfree */
				printf("%s: Hung target selection\n",
				       ahc_name(ahc));
				restart_sequencer(ahc);
				splx(s);
				return;
			}

			if (ahc_search_qinfifo(ahc, target, channel, lun,
					       scb->hscb->tag, ROLE_INITIATOR,
					       /*status*/0, SEARCH_COUNT) > 0) {
				disconnected = FALSE;
			} else {
				disconnected = TRUE;
			}

			if (disconnected) {
				u_int active_scb;

				ahc_set_recoveryscb(ahc, scb);
				/*
				 * Simply set the MK_MESSAGE control bit.
				 */
				scb->hscb->control |= MK_MESSAGE;
				scb->flags |= SCB_QUEUED_MSG
					   |  SCB_DEVICE_RESET;

				/*
				 * Mark the cached copy of this SCB in the
				 * disconnected list too, so that a reconnect
				 * at this point causes a BDR or abort.
				 */
				active_scb = ahc_inb(ahc, SCBPTR);
				if (ahc_search_disc_list(ahc, target,
							 channel, lun,
							 scb->hscb->tag,
							 /*stop_on_first*/TRUE,
							 /*remove*/FALSE,
							 /*save_state*/FALSE)) {
					u_int scb_control;

					scb_control = ahc_inb(ahc, SCB_CONTROL);
					scb_control |= MK_MESSAGE;
					ahc_outb(ahc, SCB_CONTROL, scb_control);
				}
				ahc_outb(ahc, SCBPTR, active_scb);
				ahc_index_busy_tcl(ahc, scb->hscb->tcl,
						   /*unbusy*/TRUE);

				/*
				 * Actually re-queue this SCB in case we can
				 * select the device before it reconnects.
				 * Clear out any entries in the QINFIFO first
				 * so we are the next SCB for this target
				 * to run.
				 */
				ahc_search_qinfifo(ahc, SCB_TARGET(scb),
						   channel, SCB_LUN(scb),
						   SCB_LIST_NULL,
						   ROLE_INITIATOR,
						   SCB_REQUEUE,
						   SEARCH_COMPLETE);
				scsipi_printaddr(scb->xs->xs_periph);
				printf("Queuing a BDR SCB\n");
				ahc->qinfifo[ahc->qinfifonext++] =
				    scb->hscb->tag;

				bus_dmamap_sync(ahc->parent_dmat,
				    ahc->shared_data_dmamap,
				    QINFIFO_OFFSET * 256, 256,
				    BUS_DMASYNC_PREWRITE);

				if ((ahc->features & AHC_QUEUE_REGS) != 0) {
					ahc_outb(ahc, HNSCB_QOFF,
						 ahc->qinfifonext);
				} else {
					ahc_outb(ahc, KERNEL_QINPOS,
						 ahc->qinfifonext);
				}
				callout_reset(&scb->xs->xs_callout, 2 * hz,
				    ahc_timeout, scb);
				unpause_sequencer(ahc);
			} else {
				/* Go "immediatly" to the bus reset */
				/* This shouldn't happen */
				ahc_set_recoveryscb(ahc, scb);
				scsipi_printaddr(scb->xs->xs_periph);
				printf("SCB %x: Immediate reset.  "
					"Flags = 0x%x\n", scb->hscb->tag,
					scb->flags);
				goto bus_reset;
			}
		}
	}
	splx(s);
}

static int
ahc_search_qinfifo(struct ahc_softc *ahc, int target, char channel,
		   int lun, u_int tag, role_t role, scb_flag status,
		   ahc_search_action action)
{
	struct	 scb *scbp;
	u_int8_t qinpos;
	u_int8_t qintail;
	int	 found;

	qinpos = ahc_inb(ahc, QINPOS);
	qintail = ahc->qinfifonext;
	found = 0;

	/*
	 * Start with an empty queue.  Entries that are not chosen
	 * for removal will be re-added to the queue as we go.
	 */
	ahc->qinfifonext = qinpos;

	bus_dmamap_sync(ahc->parent_dmat, ahc->shared_data_dmamap,
	    QINFIFO_OFFSET * 256, 256, BUS_DMASYNC_POSTREAD);

	while (qinpos != qintail) {
		scbp = &ahc->scb_data->scbarray[ahc->qinfifo[qinpos]];
		if (ahc_match_scb(scbp, target, channel, lun, tag, role)) {
			/*
			 * We found an scb that needs to be removed.
			 */
			switch (action) {
			case SEARCH_COMPLETE:
				if (!(scbp->xs->xs_status & XS_STS_DONE)) {
					scbp->flags |= status;
					scbp->xs->error = XS_NOERROR;
				}
				ahc_freeze_ccb(scbp);
				ahc_done(ahc, scbp);
				break;
			case SEARCH_COUNT:
				ahc->qinfifo[ahc->qinfifonext++] =
				    scbp->hscb->tag;
				break;
			case SEARCH_REMOVE:
				break;
			}
			found++;
		} else {
			ahc->qinfifo[ahc->qinfifonext++] = scbp->hscb->tag;
		}
		qinpos++;
	}

	bus_dmamap_sync(ahc->parent_dmat, ahc->shared_data_dmamap,
	    QINFIFO_OFFSET * 256, 256, BUS_DMASYNC_PREWRITE);

	if ((ahc->features & AHC_QUEUE_REGS) != 0) {
		ahc_outb(ahc, HNSCB_QOFF, ahc->qinfifonext);
	} else {
		ahc_outb(ahc, KERNEL_QINPOS, ahc->qinfifonext);
	}

	return (found);
}

/*
 * Abort all SCBs that match the given description (target/channel/lun/tag),
 * setting their status to the passed in status if the status has not already
 * been modified from CAM_REQ_INPROG.  This routine assumes that the sequencer
 * is paused before it is called.
 */
static int
ahc_abort_scbs(struct ahc_softc *ahc, int target, char channel,
	       int lun, u_int tag, role_t role, int status)
{
	struct	scb *scbp;
	u_int	active_scb;
	int	i;
	int	found;

	/* restore this when we're done */
	active_scb = ahc_inb(ahc, SCBPTR);

	found = ahc_search_qinfifo(ahc, target, channel, lun, SCB_LIST_NULL,
				   role, SCB_REQUEUE, SEARCH_COMPLETE);

	/*
	 * Search waiting for selection list.
	 */
	{
		u_int8_t next, prev;

		next = ahc_inb(ahc, WAITING_SCBH);  /* Start at head of list. */
		prev = SCB_LIST_NULL;

		while (next != SCB_LIST_NULL) {
			u_int8_t scb_index;

			ahc_outb(ahc, SCBPTR, next);
			scb_index = ahc_inb(ahc, SCB_TAG);
			if (scb_index >= ahc->scb_data->numscbs) {
				panic("Waiting List inconsistency. "
				      "SCB index == %d, yet numscbs == %d.",
				      scb_index, ahc->scb_data->numscbs);
			}
			scbp = &ahc->scb_data->scbarray[scb_index];
			if (ahc_match_scb(scbp, target, channel,
					  lun, SCB_LIST_NULL, role)) {

				next = ahc_abort_wscb(ahc, next, prev);
			} else {

				prev = next;
				next = ahc_inb(ahc, SCB_NEXT);
			}
		}
	}
	/*
	 * Go through the disconnected list and remove any entries we
	 * have queued for completion, 0'ing their control byte too.
	 * We save the active SCB and restore it ourselves, so there
	 * is no reason for this search to restore it too.
	 */
	ahc_search_disc_list(ahc, target, channel, lun, tag,
			     /*stop_on_first*/FALSE, /*remove*/TRUE,
			     /*save_state*/FALSE);

	/*
	 * Go through the hardware SCB array looking for commands that
	 * were active but not on any list.
	 */
	for(i = 0; i < ahc->scb_data->maxhscbs; i++) {
		u_int scbid;

		ahc_outb(ahc, SCBPTR, i);
		scbid = ahc_inb(ahc, SCB_TAG);
		scbp = &ahc->scb_data->scbarray[scbid];
		if (scbid < ahc->scb_data->numscbs
		 && ahc_match_scb(scbp, target, channel, lun, tag, role))
			ahc_add_curscb_to_free_list(ahc);
	}

	/*
	 * Go through the pending CCB list and look for
	 * commands for this target that are still active.
	 * These are other tagged commands that were
	 * disconnected when the reset occurred.
	 */
	{
		struct scb *scb;

		scb = ahc->pending_ccbs.lh_first;
		while (scb != NULL) {
			scbp = scb;
			scb = scb->plinks.le_next;
			if (ahc_match_scb(scbp, target, channel,
					  lun, tag, role)) {
				if (!(scbp->xs->xs_status & XS_STS_DONE))
					ahcsetccbstatus(scbp->xs, status);
				ahc_freeze_ccb(scbp);
				ahc_done(ahc, scbp);
				found++;
			}
		}
	}
	ahc_outb(ahc, SCBPTR, active_scb);
	return found;
}

static int
ahc_search_disc_list(struct ahc_softc *ahc, int target, char channel,
		     int lun, u_int tag, int stop_on_first, int remove,
		     int save_state)
{
	struct	scb *scbp;
	u_int	next;
	u_int	prev;
	u_int	count;
	u_int	active_scb;

	count = 0;
	next = ahc_inb(ahc, DISCONNECTED_SCBH);
	prev = SCB_LIST_NULL;

	if (save_state) {
		/* restore this when we're done */
		active_scb = ahc_inb(ahc, SCBPTR);
	} else
		/* Silence compiler */
		active_scb = SCB_LIST_NULL;

	while (next != SCB_LIST_NULL) {
		u_int scb_index;

		ahc_outb(ahc, SCBPTR, next);
		scb_index = ahc_inb(ahc, SCB_TAG);
		if (scb_index >= ahc->scb_data->numscbs) {
			panic("Disconnected List inconsistency. "
			      "SCB index == %d, yet numscbs == %d.",
			      scb_index, ahc->scb_data->numscbs);
		}
		scbp = &ahc->scb_data->scbarray[scb_index];
		if (ahc_match_scb(scbp, target, channel, lun,
				  tag, ROLE_INITIATOR)) {
			count++;
			if (remove) {
				next =
				    ahc_rem_scb_from_disc_list(ahc, prev, next);
			} else {
				prev = next;
				next = ahc_inb(ahc, SCB_NEXT);
			}
			if (stop_on_first)
				break;
		} else {
			prev = next;
			next = ahc_inb(ahc, SCB_NEXT);
		}
	}
	if (save_state)
		ahc_outb(ahc, SCBPTR, active_scb);
	return (count);
}

static u_int
ahc_rem_scb_from_disc_list(struct ahc_softc *ahc, u_int prev, u_int scbptr)
{
	u_int next;

	ahc_outb(ahc, SCBPTR, scbptr);
	next = ahc_inb(ahc, SCB_NEXT);

	ahc_outb(ahc, SCB_CONTROL, 0);

	ahc_add_curscb_to_free_list(ahc);

	if (prev != SCB_LIST_NULL) {
		ahc_outb(ahc, SCBPTR, prev);
		ahc_outb(ahc, SCB_NEXT, next);
	} else
		ahc_outb(ahc, DISCONNECTED_SCBH, next);

	return (next);
}

static void
ahc_add_curscb_to_free_list(struct ahc_softc *ahc)
{
	/* Invalidate the tag so that ahc_find_scb doesn't think it's active */
	ahc_outb(ahc, SCB_TAG, SCB_LIST_NULL);

	ahc_outb(ahc, SCB_NEXT, ahc_inb(ahc, FREE_SCBH));
	ahc_outb(ahc, FREE_SCBH, ahc_inb(ahc, SCBPTR));
}

/*
 * Manipulate the waiting for selection list and return the
 * scb that follows the one that we remove.
 */
static u_int
ahc_abort_wscb(struct ahc_softc *ahc, u_int scbpos, u_int prev)
{
	u_int curscb, next;

	/*
	 * Select the SCB we want to abort and
	 * pull the next pointer out of it.
	 */
	curscb = ahc_inb(ahc, SCBPTR);
	ahc_outb(ahc, SCBPTR, scbpos);
	next = ahc_inb(ahc, SCB_NEXT);

	/* Clear the necessary fields */
	ahc_outb(ahc, SCB_CONTROL, 0);

	ahc_add_curscb_to_free_list(ahc);

	/* update the waiting list */
	if (prev == SCB_LIST_NULL) {
		/* First in the list */
		ahc_outb(ahc, WAITING_SCBH, next);

		/*
		 * Ensure we aren't attempting to perform
		 * selection for this entry.
		 */
		ahc_outb(ahc, SCSISEQ, (ahc_inb(ahc, SCSISEQ) & ~ENSELO));
	} else {
		/*
		 * Select the scb that pointed to us
		 * and update its next pointer.
		 */
		ahc_outb(ahc, SCBPTR, prev);
		ahc_outb(ahc, SCB_NEXT, next);
	}

	/*
	 * Point us back at the original scb position.
	 */
	ahc_outb(ahc, SCBPTR, curscb);
	return next;
}

static void
ahc_clear_intstat(struct ahc_softc *ahc)
{
	/* Clear any interrupt conditions this may have caused */
	ahc_outb(ahc, CLRSINT0, CLRSELDO|CLRSELDI|CLRSELINGO);
	ahc_outb(ahc, CLRSINT1, CLRSELTIMEO|CLRATNO|CLRSCSIRSTI
				|CLRBUSFREE|CLRSCSIPERR|CLRPHASECHG|
				CLRREQINIT);
	ahc_outb(ahc, CLRINT, CLRSCSIINT);
}

static void
ahc_reset_current_bus(struct ahc_softc *ahc)
{
	u_int8_t scsiseq;

	ahc_outb(ahc, SIMODE1, ahc_inb(ahc, SIMODE1) & ~ENSCSIRST);
	scsiseq = ahc_inb(ahc, SCSISEQ);
	ahc_outb(ahc, SCSISEQ, scsiseq | SCSIRSTO);
	DELAY(AHC_BUSRESET_DELAY);
	/* Turn off the bus reset */
	ahc_outb(ahc, SCSISEQ, scsiseq & ~SCSIRSTO);

	ahc_clear_intstat(ahc);

	/* Re-enable reset interrupts */
	ahc_outb(ahc, SIMODE1, ahc_inb(ahc, SIMODE1) | ENSCSIRST);
}

static int
ahc_reset_channel(struct ahc_softc *ahc, char channel, int initiate_reset)
{
	u_int	initiator, target, max_scsiid;
	u_int	sblkctl;
	u_int	our_id;
	int	found;
	int	restart_needed;
	char	cur_channel;

	ahc->pending_device = NULL;

	pause_sequencer(ahc);

	/*
	 * Run our command complete fifos to ensure that we perform
	 * completion processing on any commands that 'completed'
	 * before the reset occurred.
	 */
	ahc_run_qoutfifo(ahc);

	/*
	 * Reset the bus if we are initiating this reset
	 */
	sblkctl = ahc_inb(ahc, SBLKCTL);
	cur_channel = 'A';
	if ((ahc->features & AHC_TWIN) != 0
	 && ((sblkctl & SELBUSB) != 0))
	    cur_channel = 'B';
	if (cur_channel != channel) {
		/* Case 1: Command for another bus is active
		 * Stealthily reset the other bus without
		 * upsetting the current bus.
		 */
		ahc_outb(ahc, SBLKCTL, sblkctl ^ SELBUSB);
		ahc_outb(ahc, SIMODE1, ahc_inb(ahc, SIMODE1) & ~ENBUSFREE);
		ahc_outb(ahc, SCSISEQ,
			 ahc_inb(ahc, SCSISEQ) & (ENSELI|ENRSELI|ENAUTOATNP));
		if (initiate_reset)
			ahc_reset_current_bus(ahc);
		ahc_clear_intstat(ahc);
		ahc_outb(ahc, SBLKCTL, sblkctl);
		restart_needed = FALSE;
	} else {
		/* Case 2: A command from this bus is active or we're idle */
		ahc_clear_msg_state(ahc);
		ahc_outb(ahc, SIMODE1, ahc_inb(ahc, SIMODE1) & ~ENBUSFREE);
		ahc_outb(ahc, SCSISEQ,
			 ahc_inb(ahc, SCSISEQ) & (ENSELI|ENRSELI|ENAUTOATNP));
		if (initiate_reset)
			ahc_reset_current_bus(ahc);
		ahc_clear_intstat(ahc);

		/*
		 * Since we are going to restart the sequencer, avoid
		 * a race in the sequencer that could cause corruption
		 * of our Q pointers by starting over from index 0.
		 */
		ahc->qoutfifonext = 0;
		if ((ahc->features & AHC_QUEUE_REGS) != 0)
			ahc_outb(ahc, SDSCB_QOFF, 0);
		else
			ahc_outb(ahc, QOUTPOS, 0);
		restart_needed = TRUE;
	}

	/*
	 * Clean up all the state information for the
	 * pending transactions on this bus.
	 */
	found = ahc_abort_scbs(ahc, AHC_TARGET_WILDCARD, channel,
			       AHC_LUN_WILDCARD, SCB_LIST_NULL,
			       ROLE_UNKNOWN, XS_RESET);
	if (channel == 'B') {
		our_id = ahc->our_id_b;
	} else {
		our_id = ahc->our_id;
	}

	max_scsiid = (ahc->features & AHC_WIDE) ? 15 : 7;

	/*
	 * Revert to async/narrow transfers until we renegotiate.
	 */
	for (target = 0; target <= max_scsiid; target++) {

		if (ahc->enabled_targets[target] == NULL)
			continue;
		for (initiator = 0; initiator <= max_scsiid; initiator++) {
			struct ahc_devinfo devinfo;

			ahc_compile_devinfo(&devinfo, target, initiator,
					    AHC_LUN_WILDCARD,
					    channel, ROLE_UNKNOWN);
			ahc_set_width(ahc, &devinfo,
				      MSG_EXT_WDTR_BUS_8_BIT,
				      AHC_TRANS_CUR, /*paused*/TRUE, FALSE);
			ahc_set_syncrate(ahc, &devinfo,
					 /*syncrate*/NULL, /*period*/0,
					 /*offset*/0, AHC_TRANS_CUR,
					 /*paused*/TRUE, FALSE);
			ahc_update_xfer_mode(ahc, &devinfo);
		}
	}

	if (restart_needed)
		restart_sequencer(ahc);
	else
		unpause_sequencer(ahc);
	return found;
}

static int
ahc_match_scb(struct scb *scb, int target, char channel,
	      int lun, u_int tag, role_t role)
{
	int targ = SCB_TARGET(scb);
	char chan = SCB_CHANNEL(scb);
	int slun = SCB_LUN(scb);
	int match;

	match = ((chan == channel) || (channel == ALL_CHANNELS));
	if (match != 0)
		match = ((targ == target) || (target == AHC_TARGET_WILDCARD));
	if (match != 0)
		match = ((lun == slun) || (lun == AHC_LUN_WILDCARD));

	return match;
}

static void
ahc_construct_sdtr(struct ahc_softc *ahc, u_int period, u_int offset)
{
	ahc->msgout_buf[ahc->msgout_index++] = MSG_EXTENDED;
	ahc->msgout_buf[ahc->msgout_index++] = MSG_EXT_SDTR_LEN;
	ahc->msgout_buf[ahc->msgout_index++] = MSG_EXT_SDTR;
	ahc->msgout_buf[ahc->msgout_index++] = period;
	ahc->msgout_buf[ahc->msgout_index++] = offset;
	ahc->msgout_len += 5;
}

static void
ahc_construct_wdtr(struct ahc_softc *ahc, u_int bus_width)
{
	ahc->msgout_buf[ahc->msgout_index++] = MSG_EXTENDED;
	ahc->msgout_buf[ahc->msgout_index++] = MSG_EXT_WDTR_LEN;
	ahc->msgout_buf[ahc->msgout_index++] = MSG_EXT_WDTR;
	ahc->msgout_buf[ahc->msgout_index++] = bus_width;
	ahc->msgout_len += 4;
}

static void
ahc_calc_residual(struct scb *scb)
{
	struct	hardware_scb *hscb;

	hscb = scb->hscb;

	/*
	 * If the disconnected flag is still set, this is bogus
	 * residual information left over from a sequencer
	 * pagin/pageout, so ignore this case.
	 */
	if ((scb->hscb->control & DISCONNECTED) == 0) {
		u_int32_t resid;
		int	  resid_sgs;
		int	  sg;

		/*
		 * Remainder of the SG where the transfer
		 * stopped.
		 */
		resid = (hscb->residual_data_count[2] << 16)
		      |	(hscb->residual_data_count[1] <<8)
		      |	(hscb->residual_data_count[0]);

		/*
		 * Add up the contents of all residual
		 * SG segments that are after the SG where
		 * the transfer stopped.
		 */
		resid_sgs = scb->hscb->residual_SG_count - 1/*current*/;
		sg = scb->sg_count - resid_sgs;
		while (resid_sgs > 0) {

			resid += le32toh(scb->sg_list[sg].len);
			sg++;
			resid_sgs--;
		}
		scb->xs->resid = resid;
	}

	/*
	 * Clean out the residual information in this SCB for its
	 * next consumer.
	 */
	hscb->residual_SG_count = 0;

#ifdef AHC_DEBUG
	if (ahc_debug & AHC_SHOWMISC) {
		scsipi_printaddr(scb->xs->xs_periph);
		printf("Handled Residual of %ld bytes\n" ,(long)scb->xs->resid);
	}
#endif
}

static void
ahc_update_pending_syncrates(struct ahc_softc *ahc)
{
	struct	scb *scb;
	int	pending_ccb_count;
	int	i;
	u_int	saved_scbptr;

	/*
	 * Traverse the pending SCB list and ensure that all of the
	 * SCBs there have the proper settings.
	 */
	scb = LIST_FIRST(&ahc->pending_ccbs);
	pending_ccb_count = 0;
	while (scb != NULL) {
		struct ahc_devinfo devinfo;
		struct scsipi_xfer *xs;
		struct scb *pending_scb;
		struct hardware_scb *pending_hscb;
		struct ahc_initiator_tinfo *tinfo;
		struct tmode_tstate *tstate;
		u_int  our_id, remote_id;

		xs = scb->xs;
		pending_scb = scb;
		pending_hscb = pending_scb->hscb;
		our_id = SCB_IS_SCSIBUS_B(pending_scb)
		       ? ahc->our_id_b : ahc->our_id;
		remote_id = xs->xs_periph->periph_target;
		ahc_compile_devinfo(&devinfo, our_id, remote_id,
				    SCB_LUN(pending_scb),
				    SCB_CHANNEL(pending_scb),
				    ROLE_UNKNOWN);
		tinfo = ahc_fetch_transinfo(ahc, devinfo.channel,
					    our_id, remote_id, &tstate);
		pending_hscb->control &= ~ULTRAENB;
		if ((tstate->ultraenb & devinfo.target_mask) != 0)
			pending_hscb->control |= ULTRAENB;
		pending_hscb->scsirate = tinfo->scsirate;
		pending_hscb->scsioffset = tinfo->current.offset;
		pending_ccb_count++;
		scb = LIST_NEXT(scb, plinks);
	}

	if (pending_ccb_count == 0)
		return;

	saved_scbptr = ahc_inb(ahc, SCBPTR);
	/* Ensure that the hscbs down on the card match the new information */
	for (i = 0; i < ahc->scb_data->maxhscbs; i++) {
		u_int scb_tag;

		ahc_outb(ahc, SCBPTR, i);
		scb_tag = ahc_inb(ahc, SCB_TAG);
		if (scb_tag != SCB_LIST_NULL) {
			struct	ahc_devinfo devinfo;
			struct	scb *pending_scb;
			struct scsipi_xfer *xs;
			struct	hardware_scb *pending_hscb;
			struct	ahc_initiator_tinfo *tinfo;
			struct	tmode_tstate *tstate;
			u_int	our_id, remote_id;
			u_int	control;

			pending_scb = &ahc->scb_data->scbarray[scb_tag];
			if (pending_scb->flags == SCB_FREE)
				continue;
			pending_hscb = pending_scb->hscb;
			xs = pending_scb->xs;
			our_id = SCB_IS_SCSIBUS_B(pending_scb)
			       ? ahc->our_id_b : ahc->our_id;
			remote_id = xs->xs_periph->periph_target;
			ahc_compile_devinfo(&devinfo, our_id, remote_id,
					    SCB_LUN(pending_scb),
					    SCB_CHANNEL(pending_scb),
					    ROLE_UNKNOWN);
			tinfo = ahc_fetch_transinfo(ahc, devinfo.channel,
						    our_id, remote_id, &tstate);
			control = ahc_inb(ahc, SCB_CONTROL);
			control &= ~ULTRAENB;
			if ((tstate->ultraenb & devinfo.target_mask) != 0)
				control |= ULTRAENB;
			ahc_outb(ahc, SCB_CONTROL, control);
			ahc_outb(ahc, SCB_SCSIRATE, tinfo->scsirate);
			ahc_outb(ahc, SCB_SCSIOFFSET, tinfo->current.offset);
		}
	}
	ahc_outb(ahc, SCBPTR, saved_scbptr);
}

#if UNUSED
static void
ahc_dump_targcmd(struct target_cmd *cmd)
{
	u_int8_t *byte;
	u_int8_t *last_byte;
	int i;

	byte = &cmd->initiator_channel;
	/* Debugging info for received commands */
	last_byte = &cmd[1].initiator_channel;

	i = 0;
	while (byte < last_byte) {
		if (i == 0)
			printf("\t");
		printf("%#x", *byte++);
		i++;
		if (i == 8) {
			printf("\n");
			i = 0;
		} else {
			printf(", ");
		}
	}
}
#endif

static void
ahc_shutdown(void *arg)
{
	struct	ahc_softc *ahc;
	int	i;
	u_int	sxfrctl1_a, sxfrctl1_b;

	ahc = (struct ahc_softc *)arg;

	pause_sequencer(ahc);

	/*
	 * Preserve the value of the SXFRCTL1 register for all channels.
	 * It contains settings that affect termination and we don't want
	 * to disturb the integrity of the bus during shutdown in case
	 * we are in a multi-initiator setup.
	 */
	sxfrctl1_b = 0;
	if ((ahc->features & AHC_TWIN) != 0) {
		u_int sblkctl;

		sblkctl = ahc_inb(ahc, SBLKCTL);
		ahc_outb(ahc, SBLKCTL, sblkctl | SELBUSB);
		sxfrctl1_b = ahc_inb(ahc, SXFRCTL1);
		ahc_outb(ahc, SBLKCTL, sblkctl & ~SELBUSB);
	}

	sxfrctl1_a = ahc_inb(ahc, SXFRCTL1);

	/* This will reset most registers to 0, but not all */
	ahc_reset(ahc);

	if ((ahc->features & AHC_TWIN) != 0) {
		u_int sblkctl;

		sblkctl = ahc_inb(ahc, SBLKCTL);
		ahc_outb(ahc, SBLKCTL, sblkctl | SELBUSB);
		ahc_outb(ahc, SXFRCTL1, sxfrctl1_b);
		ahc_outb(ahc, SBLKCTL, sblkctl & ~SELBUSB);
	}
	ahc_outb(ahc, SXFRCTL1, sxfrctl1_a);

	ahc_outb(ahc, SCSISEQ, 0);
	ahc_outb(ahc, SXFRCTL0, 0);
	ahc_outb(ahc, DSPCISTATUS, 0);

	for (i = TARG_SCSIRATE; i < HA_274_BIOSCTRL; i++)
		ahc_outb(ahc, i, 0);
}

#if defined(AHC_DEBUG) && 0
static void
ahc_dumptinfo(struct ahc_softc *ahc, struct ahc_initiator_tinfo *tinfo)
{
	printf("%s: tinfo: rate %u\n", ahc_name(ahc), tinfo->scsirate);

	printf("\tcurrent:\n");
	printf("\t\twidth %u period %u offset %u flags %x\n",
	    tinfo->current.width, tinfo->current.period,
	    tinfo->current.offset, tinfo->current.ppr_flags);

	printf("\tgoal:\n");
	printf("\t\twidth %u period %u offset %u flags %x\n",
	    tinfo->goal.width, tinfo->goal.period,
	    tinfo->goal.offset, tinfo->goal.ppr_flags);

	printf("\tuser:\n");
	printf("\t\twidth %u period %u offset %u flags %x\n",
	    tinfo->user.width, tinfo->user.period,
	    tinfo->user.offset, tinfo->user.ppr_flags);
}
#endif

static int
ahc_istagged_device(struct ahc_softc *ahc, struct scsipi_xfer *xs,
		    int nocmdcheck)
{
#ifdef AHC_NO_TAGS
	return 0;
#else
	char channel;
	u_int our_id, target;
	struct tmode_tstate *tstate;
	struct ahc_devinfo devinfo;

	channel = SIM_CHANNEL(ahc, xs->xs_periph);
	our_id = SIM_SCSI_ID(ahc, xs->xs_periph);
	target = xs->xs_periph->periph_target;
	(void)ahc_fetch_transinfo(ahc, channel, our_id, target, &tstate);

	ahc_compile_devinfo(&devinfo, our_id, target,
	    xs->xs_periph->periph_lun, channel, ROLE_INITIATOR);

	return (tstate->tagenable & devinfo.target_mask);
#endif
}
