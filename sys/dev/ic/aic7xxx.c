/*	$NetBSD: aic7xxx.c,v 1.37.2.5 1999/10/29 22:19:30 thorpej Exp $	*/

/*
 * Generic driver for the aic7xxx based adaptec SCSI controllers
 * Product specific probe and attach routines can be found in:
 * i386/eisa/aic7770.c	27/284X and aic7770 motherboard controllers
 * pci/aic7870.c	3940, 2940, aic7880, aic7870 and aic7850 controllers
 *
 * Copyright (c) 1994, 1995, 1996 Justin T. Gibbs.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice immediately at the beginning of the file, without modification,
 *    this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 * from Id: aic7xxx.c,v 1.75 1996/06/23 20:02:37 gibbs Exp
 */
/*
 * TODO:
 *	Implement Target Mode
 *
 * A few notes on how SCB paging works...
 *
 * SCB paging takes advantage of the fact that devices stay disconnected
 * from the bus a relatively long time and that while they're disconnected,
 * having the SCBs for that device down on the host adapter is of little use.
 * Instead we copy the SCB back up into kernel memory and reuse the SCB slot
 * on the card to schedule another transaction.  This can be a real payoff
 * when doing random I/O to tagged queueing devices since there are more 
 * transactions active at once for the device to sort for optimal seek
 * reduction. The algorithm goes like this...
 *
 * At the sequencer level:
 * 1) Disconnected SCBs are threaded onto a doubly linked list, headed by
 *    DISCONNECTED_SCBH using the SCB_NEXT and SCB_PREV fields.  The most
 *    recently disconnected device is always at the head.
 *
 * 2) The SCB has an added field SCB_TAG that corresponds to the kernel
 *    SCB number (ie 0-254).
 *
 * 3) When a command is queued, the hardware index of the SCB it was downloaded
 *    into is placed into the QINFIFO for easy indexing by the sequencer.
 *
 * 4) The tag field is used as the tag for tagged-queueing, for determining
 *    the related kernel SCB, and is the value put into the QOUTFIFO
 *    so the kernel doesn't have to upload the SCB to determine the kernel SCB
 *    that completed on command completes.
 *
 * 5) When a reconnect occurs, the sequencer must scan the SCB array (even
 *    in the tag case) looking for the appropriate SCB and if it can't find
 *    it, it interrupts the kernel so it can page the SCB in.
 *
 * 6) If the sequencer is successful in finding the SCB, it removes it from
 *    the doubly linked list of disconnected SCBS.
 *
 * At the kernel level:
 * 1) There are four queues that a kernel SCB may reside on:
 *	free_scbs - SCBs that are not in use and have a hardware slot assigned
 *		    to them.
 *      page_scbs - SCBs that are not in use and need to have a hardware slot
 *		    assigned to them (i.e. they will most likely cause a page
 *		    out event).
 *	waiting_scbs - SCBs that are active, don't have an assigned hardware
 *		    slot assigned to them and are waiting for either a
 *		    disconnection or a command complete to free up a slot.
 *	assigned_scbs - SCBs that were in the waiting_scbs queue, but were
 *		    assigned a slot by ahc_free_scb.
 *
 * 2) When a new request comes in, an SCB is allocated from the free_scbs or
 *    page_scbs queue with preference to SCBs on the free_scbs queue.
 *
 * 3) If there are no free slots (we retrieved the SCB off of the page_scbs
 *    queue), the SCB is inserted onto the tail of the waiting_scbs list and
 *    we attempt to run this queue down.
 *
 * 4) ahc_run_waiting_queues() looks at both the assigned_scbs and waiting_scbs
 *    queues.  In the case of the assigned_scbs, the commands are immediately
 *    downloaded and started.  For waiting_scbs, we page in all that we can
 *    ensuring we don't create a resource deadlock (see comments in
 *    ahc_run_waiting_queues()).
 *
 * 5) After we handle a bunch of command completes, we also try running the
 *    queues since many SCBs may have disconnected since the last command
 *    was started and we have at least one free slot on the card.
 *
 * 6) ahc_free_scb looks at the waiting_scbs queue for a transaction
 *    requiring a slot and moves it to the assigned_scbs queue if it
 *    finds one.  Otherwise it puts the current SCB onto the free_scbs
 *    queue for later use.
 *
 * 7) The driver handles page-in requests from the sequencer in response to
 *    the NO_MATCH sequencer interrupt.  For tagged commands, the appropriate
 *    SCB is easily found since the tag is a direct index into our kernel SCB
 *    array.  For non-tagged commands, we keep a separate array of 16 pointers
 *    that point to the single possible SCB that was paged out for that target.
 */

#include <sys/param.h>
#include <sys/systm.h>
#if defined(__NetBSD__)
#include <sys/device.h>
#include <machine/bswap.h>
#include <machine/bus.h>
#include <machine/intr.h>
#endif /* defined(__NetBSD__) */

#include <sys/malloc.h>
#include <sys/buf.h>
#include <sys/proc.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsi_message.h>
#if defined(__NetBSD__)
#include <dev/scsipi/scsipi_debug.h>
#endif
#include <dev/scsipi/scsiconf.h>

#if defined(__FreeBSD__)
#include <machine/clock.h>
#endif

#include <vm/vm.h>
#include <vm/vm_param.h>
#include <vm/pmap.h>

#if defined(__FreeBSD__)
#include <i386/scsi/aic7xxx.h>

#include <dev/aic7xxx/aic7xxx_reg.h>
#endif /* defined(__FreeBSD__) */

#if defined(__NetBSD__)
#include <dev/ic/aic7xxxreg.h>
#include <dev/ic/aic7xxxvar.h>

#define bootverbose	1

#define DEBUGTARG	DEBUGTARGET
#if DEBUGTARG < 0	/* Negative numbers for disabling cause warnings */
#undef DEBUGTARG
#define DEBUGTARG	17
#endif
#ifdef alpha		/* XXX */
/* XXX XXX NEED REAL DMA MAPPING SUPPORT XXX XXX */ 
extern paddr_t alpha_XXX_dmamap(vaddr_t);
#undef vtophys
#define	vtophys(va)	alpha_XXX_dmamap((vaddr_t) va)
#endif	/* alpha */
#endif /* defined(__NetBSD__) */

#include <sys/kernel.h>

#define	SCB_DMA_OFFSET(ahc, scb, member)				\
			((ahc)->sc_dmamap_control->dm_segs[0].ds_addr +	\
			(scb)->tag * sizeof(struct scb) +		\
			offsetof(struct scb, member))
#define KVTOPHYS(x)   vtophys(x)

#define AHC_MAXXFER	((AHC_NSEG - 1) << PGSHIFT)

#define MIN(a,b) ((a < b) ? a : b)
#define ALL_TARGETS -1

#if defined(__FreeBSD__)
u_long ahc_unit = 0;
#endif

#ifdef AHC_DEBUG
static int     ahc_debug = AHC_DEBUG;
#endif

#ifdef AHC_BROKEN_CACHE
int ahc_broken_cache = 1;

/*
 * "wbinvd" cause writing back whole cache (both CPU internal & external)
 * to memory, so that the instruction takes a lot of time.
 * This makes machine slow.
 */
#define	INVALIDATE_CACHE()	__asm __volatile("wbinvd")
#endif

/**** bit definitions for SCSIDEF ****/
#define	HSCSIID		0x07		/* our SCSI ID */
#define HWSCSIID	0x0f		/* our SCSI ID if Wide Bus */

void	ahcminphys __P((struct buf *bp));
void	ahc_scsipi_request __P((struct scsipi_channel *chan,
	    scsipi_adapter_req_t, void *));

static inline void pause_sequencer __P((struct ahc_data *ahc));
static inline void unpause_sequencer __P((struct ahc_data *ahc,
					  int unpause_always));
static inline void restart_sequencer __P((struct ahc_data *ahc));

static inline void
pause_sequencer(ahc)
	struct ahc_data *ahc;
{
	AHC_OUTB(ahc, HCNTRL, ahc->pause);

	/*
	 * Since the sequencer can disable pausing in a critical section, we
	 * must loop until it actually stops.
	 */
	while ((AHC_INB(ahc, HCNTRL) & PAUSE) == 0)
		;
}

static inline void
unpause_sequencer(ahc, unpause_always)
	struct ahc_data *ahc;
	int unpause_always;
{
	if (unpause_always
	 ||(AHC_INB(ahc, INTSTAT) & (SCSIINT | SEQINT | BRKADRINT)) == 0)
		AHC_OUTB(ahc, HCNTRL, ahc->unpause);
}

/*
 * Restart the sequencer program from address zero
 */
static inline void
restart_sequencer(ahc)
	struct ahc_data *ahc;
{
	do {
		AHC_OUTB(ahc, SEQCTL, SEQRESET|FASTMODE);
	} while((AHC_INB(ahc, SEQADDR0) != 0)
		|| (AHC_INB(ahc, SEQADDR1) != 0));

	unpause_sequencer(ahc, /*unpause_always*/TRUE);
}

/*
 * Is device which is pointed by periph connected on second scsi bus ?
 */
#define	IS_SCSIBUS_B(periph)						\
	((periph)->periph_channel->chan_channel == 1)

static u_char	ahc_abort_wscb __P((struct ahc_data *ahc, struct scb *scbp,
				    u_char prev,
				    u_char timedout_scb, u_int32_t xs_error));
static void	ahc_add_waiting_scb __P((struct ahc_data *ahc,
					 struct scb *scb));
static void	ahc_done __P((struct ahc_data *ahc, struct scb *scbp));
static void	ahc_free_scb __P((struct ahc_data *ahc, struct scb *scb));
static inline void ahc_send_scb __P((struct ahc_data *ahc, struct scb *scb));
static inline void ahc_fetch_scb __P((struct ahc_data *ahc, struct scb *scb));
static inline void ahc_page_scb __P((struct ahc_data *ahc, struct scb *out_scb,
				struct scb *in_scb));
static inline void ahc_run_waiting_queues __P((struct ahc_data *ahc));
static void	ahc_handle_seqint __P((struct ahc_data *ahc, u_int8_t intstat));
static struct scb *
		ahc_get_scb __P((struct ahc_data *ahc));
static void	ahc_loadseq __P((struct ahc_data *ahc));
static struct scb *
		ahc_new_scb __P((struct ahc_data *ahc, struct scb *scb));
static int	ahc_match_scb __P((struct scb *scb, int target, char channel));
static int	ahc_poll __P((struct ahc_data *ahc, int wait));
#ifdef AHC_DEBUG
static void	ahc_print_scb __P((struct scb *scb));
#endif
static int	ahc_reset_channel __P((struct ahc_data *ahc, char channel,
				       u_char timedout_scb, u_int32_t xs_error,
				       u_char initiate_reset));
static int	ahc_reset_device __P((struct ahc_data *ahc, int target,
				      char channel, u_char timedout_scb,
				      u_int32_t xs_error));
static void	ahc_reset_current_bus __P((struct ahc_data *ahc));
static void	ahc_run_done_queue __P((struct ahc_data *ahc));
static void	ahc_scsirate __P((struct ahc_data* ahc, u_int8_t *scsirate,
				  u_int8_t *period, u_int8_t *offset,
				  char channel, int target));
#if defined(__FreeBSD__)
static timeout_t
		ahc_timeout;
#elif defined(__NetBSD__)
static void	ahc_timeout __P((void *));
#endif
static void	ahc_busy_target __P((struct ahc_data *ahc,
				     int target, char channel));
static void	ahc_unbusy_target __P((struct ahc_data *ahc,
				       int target, char channel));
static void	ahc_construct_sdtr __P((struct ahc_data *ahc, int start_byte,
					u_int8_t period, u_int8_t offset));  
static void	ahc_construct_wdtr __P((struct ahc_data *ahc, int start_byte,
					u_int8_t bus_width));
static void	ahc_update_xfer_mode __P((struct ahc_data *ahc, int channel,
		    int target));

#if defined(__FreeBSD__)

char *ahc_name(ahc)
	struct ahc_data *ahc;
{
	static char name[10];

	sprintf(name, "ahc%d", ahc->unit);
	return (name);
}

#endif

#ifdef  AHC_DEBUG
static void
ahc_print_scb(scb)
        struct scb *scb;
{
        printf("scb:%p control:0x%x tcl:0x%x cmdlen:%d cmdpointer:0x%lx\n"
	    ,scb
	    ,scb->control
	    ,scb->tcl
	    ,scb->cmdlen
	    ,(unsigned long) scb->cmdpointer );
        printf("        datlen:%d data:0x%lx segs:0x%x segp:0x%lx\n"
	    ,scb->datalen
	    ,(unsigned long) scb->data
	    ,scb->SG_segment_count
	    ,(unsigned long) scb->SG_list_pointer);
	printf("	sg_addr:%lx sg_len:%ld\n"
	    ,(unsigned long) scb->ahc_dma[0].addr
	    ,(long) scb->ahc_dma[0].len);
}

#endif

static struct {
        u_char errno;
	char *errmesg;
} hard_error[] = {
	{ ILLHADDR,  "Illegal Host Access" },
	{ ILLSADDR,  "Illegal Sequencer Address referenced" },
	{ ILLOPCODE, "Illegal Opcode in sequencer program" },
	{ PARERR,    "Sequencer Ram Parity Error" }
};


/*
 * Valid SCSIRATE values.  (p. 3-17)
 * Provides a mapping of tranfer periods in ns to the proper value to
 * stick in the scsiscfr reg to use that transfer rate.
 */
static struct {
	short sxfr;
	/* Rates in Ultra mode have bit 8 of sxfr set */
#define		ULTRA_SXFR 0x100
	int period; /* in ns/4 */
	char *rate;
} ahc_syncrates[] = {
	{ 0x100, 12, "20.0"  },
	{ 0x110, 15, "16.0"  },
	{ 0x120, 18, "13.4"  },
	{ 0x000, 25, "10.0"  },
	{ 0x010, 31,  "8.0"  },
	{ 0x020, 37,  "6.67" },
	{ 0x030, 43,  "5.7"  },
	{ 0x040, 50,  "5.0"  },
	{ 0x050, 56,  "4.4"  },
	{ 0x060, 62,  "4.0"  },
	{ 0x070, 68,  "3.6"  }
};

static int ahc_num_syncrates =
	sizeof(ahc_syncrates) / sizeof(ahc_syncrates[0]);

/*
 * Allocate a controller structure for a new device and initialize it.
 * ahc_reset should be called before now since we assume that the card
 * is paused.
 */
#if defined(__FreeBSD__)
struct ahc_data *
ahc_alloc(unit, iobase, type, flags)
	int unit;
	u_long iobase;
#elif defined(__NetBSD__)
void
ahc_construct(ahc, st, sh, dt, type, flags)
	struct  ahc_data *ahc;
	bus_space_tag_t st;
	bus_space_handle_t sh;
	bus_dma_tag_t dt;
#endif
	ahc_type type;
	ahc_flag flags;
{

	/*
	 * find unit and check we have that many defined
	 */

#if defined(__FreeBSD__)
	struct  ahc_data *ahc;

	/*
	 * Allocate a storage area for us
	 */

	ahc = malloc(sizeof(struct ahc_data), M_TEMP, M_NOWAIT);
	if (!ahc) {
		printf("ahc%d: cannot malloc!\n", unit);
		return NULL;
	}
	bzero(ahc, sizeof(struct ahc_data));
#endif
	STAILQ_INIT(&ahc->free_scbs);
	STAILQ_INIT(&ahc->page_scbs);
	STAILQ_INIT(&ahc->waiting_scbs);
	STAILQ_INIT(&ahc->assigned_scbs);
#if defined(__FreeBSD__)
	ahc->unit = unit;
#endif
#if defined(__FreeBSD__)
	ahc->baseport = iobase;
#elif defined(__NetBSD__)
	ahc->sc_st = st;
	ahc->sc_sh = sh;
	ahc->sc_dt = dt;
#endif
	ahc->type = type;
	ahc->flags = flags;
	ahc->unpause = (AHC_INB(ahc, HCNTRL) & IRQMS) | INTEN;
	ahc->pause = ahc->unpause | PAUSE;

#if defined(__FreeBSD__)
	return (ahc);
#endif
}

void
ahc_free(ahc)
	struct ahc_data *ahc;
{
#if defined(__FreeBSD__)
	free(ahc, M_DEVBUF);
	return;
#endif
}

void
#if defined(__FreeBSD__)
ahc_reset(iobase)
	u_long iobase;
#elif defined(__NetBSD__)
ahc_reset(devname, st, sh)
	char *devname;
	bus_space_tag_t st;
	bus_space_handle_t sh;
#endif
{
        u_char hcntrl;
	int wait;

	/* Retain the IRQ type accross the chip reset */
#if defined(__FreeBSD__)
	hcntrl = (inb(HCNTRL + iobase) & IRQMS) | INTEN;

	outb(HCNTRL + iobase, CHIPRST | PAUSE);
#elif defined(__NetBSD__)
	hcntrl = (bus_space_read_1(st, sh, HCNTRL) & IRQMS) | INTEN;

	bus_space_write_1(st, sh, HCNTRL, CHIPRST | PAUSE);
#endif
	/*
	 * Ensure that the reset has finished
	 */
	DELAY(100);
	wait = 1000;
#if defined(__FreeBSD__)
	while (--wait && !(inb(HCNTRL + iobase) & CHIPRSTACK))
#elif defined(__NetBSD__)
	while (--wait && !(bus_space_read_1(st, sh, HCNTRL) & CHIPRSTACK))
#endif
		DELAY(1000);
	if(wait == 0) {
#if defined(__FreeBSD__)
		printf("ahc at 0x%lx: WARNING - Failed chip reset!  "
		       "Trying to initialize anyway.\n", iobase);
#elif defined(__NetBSD__)
		printf("%s: WARNING - Failed chip reset!  "
		       "Trying to initialize anyway.\n", devname);
#endif
	}
#if defined(__FreeBSD__)
	outb(HCNTRL + iobase, hcntrl | PAUSE);
#elif defined(__NetBSD__)
	bus_space_write_1(st, sh, HCNTRL, hcntrl | PAUSE);
#endif
}

/*
 * Look up the valid period to SCSIRATE conversion in our table.
 */
static void
ahc_scsirate(ahc, scsirate, period, offset, channel, target )
	struct	 ahc_data *ahc;
	u_int8_t *scsirate;
	u_int8_t *period;
	u_int8_t *offset;
	char	 channel;
	int	 target;
{
	int i;
	u_int32_t ultra_enb_addr;
	u_int8_t  sxfrctl0;
	u_int8_t  ultra_enb;

	i = ahc_num_syncrates; /* Default to async */
	
	if (*period >= ahc_syncrates[0].period && *offset != 0) {
		for (i = 0; i < ahc_num_syncrates; i++) {

			if (*period <= ahc_syncrates[i].period) {
				/*
				 * Watch out for Ultra speeds when ultra is not
				 * enabled and vice-versa.
				 */
				if(!(ahc->type & AHC_ULTRA) 
				 && (ahc_syncrates[i].sxfr & ULTRA_SXFR)) {
					/*
					 * This should only happen if the
					 * drive is the first to negotiate
					 * and chooses a high rate.  We'll
					 * just move down the table util
					 * we hit a non ultra speed.
					 */
					continue;
				}
				*scsirate = (ahc_syncrates[i].sxfr & 0xF0)
					  | (*offset & 0x0f);
				*period = ahc_syncrates[i].period;

#if 1
				ahc_update_xfer_mode(ahc, channel, target);
#else
				if(bootverbose) {
					printf("%s: target %d synchronous at %sMHz,"
					       " offset = 0x%x\n",
					        ahc_name(ahc), target,
						ahc_syncrates[i].rate, *offset );
				}
#endif
				break;
			}
		}
	}
	if (i >= ahc_num_syncrates) {
		/* Use asynchronous transfers. */
		*scsirate = 0;
		*period = 0;
		*offset = 0;
#if 1
		ahc_update_xfer_mode(ahc, channel, target);
#else
		if (bootverbose)
			printf("%s: target %d using asynchronous transfers\n",
			       ahc_name(ahc), target );
#endif
	}
	/*
	 * Ensure Ultra mode is set properly for
	 * this target.
	 */
	ultra_enb_addr = ULTRA_ENB;
	if(channel == 'B' || target > 7)
		ultra_enb_addr++;
	ultra_enb = AHC_INB(ahc, ultra_enb_addr);	
	sxfrctl0 = AHC_INB(ahc, SXFRCTL0);
	if (*scsirate != 0 && ahc_syncrates[i].sxfr & ULTRA_SXFR) {
		ultra_enb |= 0x01 << (target & 0x07);
		sxfrctl0 |= ULTRAEN;
	}
	else {
		ultra_enb &= ~(0x01 << (target & 0x07));
		sxfrctl0 &= ~ULTRAEN;
	}
	AHC_OUTB(ahc, ultra_enb_addr, ultra_enb);
	AHC_OUTB(ahc, SXFRCTL0, sxfrctl0);
}

/*
 * Attach all the sub-devices we can find
 */
int
ahc_attach(ahc)
	struct ahc_data *ahc;
{
	struct scsipi_adapter *adapt = &ahc->sc_adapter;
	struct scsipi_channel *chan;

#ifdef AHC_BROKEN_CACHE
	if (cpu_class == CPUCLASS_386)	/* doesn't have "wbinvd" instruction */
		ahc_broken_cache = 0;
#endif

	/*
	 * Fill in the scsipi_adapter.
	 */
	memset(adapt, 0, sizeof(*adapt));
	adapt->adapt_dev = &ahc->sc_dev;
	adapt->adapt_nchannels = (ahc->type & AHC_TWIN) ? 2 : 1;
	adapt->adapt_openings = ahc->maxscbs;
	adapt->adapt_max_periph = 1;	/* increased later of we can TQING */
	adapt->adapt_request = ahc_scsipi_request;
	adapt->adapt_minphys = ahcminphys;

	/*
	 * Fill in the scsipi_channel(s).
	 */
	chan = &ahc->sc_channels[0];
	memset(chan, 0, sizeof(*chan));
	chan->chan_adapter = adapt;
	chan->chan_bustype = &scsi_bustype;
	chan->chan_channel = 0;
	chan->chan_ntargets = (ahc->type & AHC_WIDE) ? 16 : 8;
	chan->chan_nluns = 8;
	chan->chan_id = ahc->our_id;

	if (ahc->type & AHC_TWIN) {
		chan = &ahc->sc_channels[1];
		memset(chan, 0, sizeof(*chan));
		chan->chan_adapter = adapt;
		chan->chan_channel = 1;
		chan->chan_ntargets = (ahc->type & AHC_WIDE) ? 16 : 8;
		chan->chan_nluns = 8;
		chan->chan_id = ahc->our_id_b;
		TAILQ_INIT(&chan->chan_queue);
		TAILQ_INIT(&chan->chan_complete);
	}

	/*
	 * ask the adapter what subunits are present
	 */
	if ((ahc->flags & AHC_CHANNEL_B_PRIMARY) == 0) {
		(void) config_found((void *)ahc, &ahc->sc_channels[0],
		    scsiprint);
		if (ahc->type & AHC_TWIN)
			(void) config_found((void *)ahc, &ahc->sc_channels[1],
			    scsiprint);
	} else {
		/* assert(ahc->type & AHC_TWIN); */
		config_found((void *)ahc, &ahc->sc_channels[1], scsiprint);
		config_found((void *)ahc, &ahc->sc_channels[0], scsiprint);
	}
	return 1;
}

/*
 * Send an SCB down to the card via PIO.
 * We assume that the proper SCB is already selected in SCBPTR. 
 */
static inline void
ahc_send_scb(ahc, scb)
        struct	ahc_data *ahc;
        struct	scb *scb;
{
#if BYTE_ORDER == BIG_ENDIAN
	scb->SG_list_pointer = bswap32(scb->SG_list_pointer);
	scb->cmdpointer      = bswap32(scb->cmdpointer);
#endif
	
	AHC_OUTB(ahc, SCBCNT, SCBAUTO);

	if( ahc->type == AHC_284 )
		/* Can only do 8bit PIO */
		AHC_OUTSB(ahc, SCBARRAY, scb, SCB_PIO_TRANSFER_SIZE);
	else
		AHC_OUTSL(ahc, SCBARRAY, scb,
		      (SCB_PIO_TRANSFER_SIZE + 3) / 4);
	AHC_OUTB(ahc, SCBCNT, 0); 
}

/*
 * Retrieve an SCB from the card via PIO.
 * We assume that the proper SCB is already selected in SCBPTR.
 */
static inline void
ahc_fetch_scb(ahc, scb)
	struct	ahc_data *ahc;
	struct	scb *scb;
{
	AHC_OUTB(ahc, SCBCNT, 0x80);	/* SCBAUTO */

	/* Can only do 8bit PIO for reads */
	AHC_INSB(ahc, SCBARRAY, scb, SCB_PIO_TRANSFER_SIZE);

	AHC_OUTB(ahc, SCBCNT, 0);
#if BYTE_ORDER == BIG_ENDIAN
	{
		u_char tmp;

		scb->SG_list_pointer = bswap32(scb->SG_list_pointer);
		scb->cmdpointer      = bswap32(scb->cmdpointer);
		tmp = scb->residual_data_count[0];
		scb->residual_data_count[0] = scb->residual_data_count[2];
		scb->residual_data_count[2] = tmp;
	}
#endif
}

/*
 * Swap in_scbp for out_scbp down in the cards SCB array.
 * We assume that the SCB for out_scbp is already selected in SCBPTR.
 */
static inline void
ahc_page_scb(ahc, out_scbp, in_scbp)
	struct ahc_data *ahc;
	struct scb *out_scbp;
	struct scb *in_scbp;
{
	/* Page-out */
	ahc_fetch_scb(ahc, out_scbp);
	out_scbp->flags |= SCB_PAGED_OUT;
	if(!(out_scbp->control & TAG_ENB))
	{
		/* Stick in non-tagged array */
		int index =  (out_scbp->tcl >> 4)
			   | (out_scbp->tcl & SELBUSB);
		ahc->pagedout_ntscbs[index] = out_scbp;
	}

	/* Page-in */
	in_scbp->position = out_scbp->position;
	out_scbp->position = SCB_LIST_NULL;
	ahc_send_scb(ahc, in_scbp);
	in_scbp->flags &= ~SCB_PAGED_OUT;
}

static inline void
ahc_run_waiting_queues(ahc)
	struct ahc_data *ahc;
{
	struct scb* scb;
	u_char cur_scb;

	if(!(ahc->assigned_scbs.stqh_first || ahc->waiting_scbs.stqh_first))
		return;

	pause_sequencer(ahc);
	cur_scb = AHC_INB(ahc, SCBPTR);

	/*
	 * First handle SCBs that are waiting but have been
	 * assigned a slot.
	 */
	while((scb = ahc->assigned_scbs.stqh_first) != NULL) {
		STAILQ_REMOVE_HEAD(&ahc->assigned_scbs, links);
		AHC_OUTB(ahc, SCBPTR, scb->position);
		ahc_send_scb(ahc, scb);

		/* Mark this as an active command */
		scb->flags ^= SCB_ASSIGNEDQ|SCB_ACTIVE;

		AHC_OUTB(ahc, QINFIFO, scb->position);
		if (!(scb->xs->xs_control & XS_CTL_POLL)) {
			timeout(ahc_timeout, (caddr_t)scb,
				(scb->xs->timeout * hz) / 1000);
		}
		SC_DEBUG(scb->xs->sc_link, SDEV_DB3, ("cmd_sent\n"));
	}
	/* Now deal with SCBs that require paging */
	if((scb = ahc->waiting_scbs.stqh_first) != NULL) {
		u_char disc_scb = AHC_INB(ahc, DISCONNECTED_SCBH);
		u_char active = AHC_INB(ahc, FLAGS) & (SELECTED|IDENTIFY_SEEN);
		int count = 0;

		do {
			u_char next_scb;

			/* Attempt to page this SCB in */
			if(disc_scb == SCB_LIST_NULL)
				break;

			/*
			 * Check the next SCB on in the list.
			 */
			AHC_OUTB(ahc, SCBPTR, disc_scb);
			next_scb = AHC_INB(ahc, SCB_NEXT); 

			/*
			 * We have to be careful about when we allow
			 * an SCB to be paged out.  There must always
			 * be at least one slot available for a
			 * reconnecting target in case it references
			 * an SCB that has been paged out.  Our
			 * heuristic is that either the disconnected
			 * list has at least two entries in it or
			 * there is one entry and the sequencer is
			 * actively working on an SCB which implies that
			 * it will either complete or disconnect before
			 * another reconnection can occur.
			 */
			if((next_scb != SCB_LIST_NULL) || active)
			{
				u_char out_scbi;
				struct scb* out_scbp;

				STAILQ_REMOVE_HEAD(&ahc->waiting_scbs, links);

				/*
				 * Find the in-core SCB for the one
				 * we're paging out.
				 */
				out_scbi = AHC_INB(ahc, SCB_TAG); 
				out_scbp = ahc->scbarray[out_scbi];

				/* Do the page out */
				ahc_page_scb(ahc, out_scbp, scb);

				/* Mark this as an active command */
				scb->flags ^= SCB_WAITINGQ|SCB_ACTIVE;

				/* Queue the command */
				AHC_OUTB(ahc, QINFIFO, scb->position);
				if (!(scb->xs->xs_control & XS_CTL_POLL)) {
					timeout(ahc_timeout, (caddr_t)scb,
						(scb->xs->timeout * hz) / 1000);
				}
				SC_DEBUG(scb->xs->sc_link, SDEV_DB3,
					("cmd_paged-in\n"));
				count++;

				/* Advance to the next disconnected SCB */
				disc_scb = next_scb;
			}
			else
				break;
		} while((scb = ahc->waiting_scbs.stqh_first) != NULL);

		if(count) {
			/* 
			 * Update the head of the disconnected list.
			 */
			AHC_OUTB(ahc, DISCONNECTED_SCBH, disc_scb);
			if(disc_scb != SCB_LIST_NULL) {
				AHC_OUTB(ahc, SCBPTR, disc_scb);
				AHC_OUTB(ahc, SCB_PREV, SCB_LIST_NULL);
			}
		}
	}
	/* Restore old position */
	AHC_OUTB(ahc, SCBPTR, cur_scb);
	unpause_sequencer(ahc, /*unpause_always*/FALSE);
}

/*
 * Add this SCB to the head of the "waiting for selection" list.
 */
static
void ahc_add_waiting_scb(ahc, scb)
	struct ahc_data *ahc;
	struct scb *scb;
{
	u_char next; 
	u_char curscb;

	curscb = AHC_INB(ahc, SCBPTR);
	next = AHC_INB(ahc, WAITING_SCBH);

	AHC_OUTB(ahc, SCBPTR, scb->position);
	AHC_OUTB(ahc, SCB_NEXT, next);
	AHC_OUTB(ahc, WAITING_SCBH, scb->position);

	AHC_OUTB(ahc, SCBPTR, curscb);
}

/*
 * Catch an interrupt from the adapter
 */
#if defined(__FreeBSD__)
void
#elif defined (__NetBSD__)
int
#endif
ahc_intr(arg)
        void *arg;
{
	int     intstat;
	u_char	status;
	struct	scb *scb;
	struct	scsipi_xfer *xs;
	struct	ahc_data *ahc = (struct ahc_data *)arg;

	intstat = AHC_INB(ahc, INTSTAT);
	/*
	 * Is this interrupt for me? or for
	 * someone who is sharing my interrupt?
	 */
	if (!(intstat & INT_PEND))
#if defined(__FreeBSD__)
		return;
#elif defined(__NetBSD__)
		return 0;
#endif

        if (intstat & BRKADRINT) {
		/* We upset the sequencer :-( */

		/* Lookup the error message */
		int i, error = AHC_INB(ahc, ERROR);
		int num_errors =  sizeof(hard_error)/sizeof(hard_error[0]);
		for(i = 0; error != 1 && i < num_errors; i++)
			error >>= 1;
                panic("%s: brkadrint, %s at seqaddr = 0x%x\n",
		      ahc_name(ahc), hard_error[i].errmesg,
		      (AHC_INB(ahc, SEQADDR1) << 8) |
		      AHC_INB(ahc, SEQADDR0));
        }
        if (intstat & SEQINT)
		ahc_handle_seqint(ahc, intstat);

	if (intstat & SCSIINT) {

		int scb_index = AHC_INB(ahc, SCB_TAG);
		status = AHC_INB(ahc, SSTAT1);
		scb = ahc->scbarray[scb_index];

		if (status & SCSIRSTI) {
			char channel;
			channel = AHC_INB(ahc, SBLKCTL);
			channel = channel & SELBUSB ? 'B' : 'A';
			printf("%s: Someone reset channel %c\n",
				ahc_name(ahc), channel);
			ahc_reset_channel(ahc, 
					  channel,
					  SCB_LIST_NULL,
					  XS_BUSY,
					  /* Initiate Reset */FALSE);
			scb = NULL;
		}
		else if (!(scb && (scb->flags & SCB_ACTIVE))){
			printf("%s: ahc_intr - referenced scb not "
			       "valid during scsiint 0x%x scb(%d)\n",
				ahc_name(ahc), status, scb_index);
			AHC_OUTB(ahc, CLRSINT1, status);
			unpause_sequencer(ahc, /*unpause_always*/TRUE);
			AHC_OUTB(ahc, CLRINT, CLRSCSIINT);
			scb = NULL;
		}
		else if (status & SCSIPERR) {
			/*
			 * Determine the bus phase and
			 * queue an appropriate message
			 */
			char	*phase;
			u_char	mesg_out = MSG_NOOP;
			u_char	lastphase = AHC_INB(ahc, LASTPHASE);

			xs = scb->xs;
			scsipi_printaddr(xs->xs_periph);

			switch(lastphase) {
				case P_DATAOUT:
					phase = "Data-Out";
					break;
				case P_DATAIN:
					phase = "Data-In";
					mesg_out = MSG_INITIATOR_DET_ERR;
					break;
				case P_COMMAND:
					phase = "Command";
					break;
				case P_MESGOUT:
					phase = "Message-Out";
					break;
				case P_STATUS:
					phase = "Status";
					mesg_out = MSG_INITIATOR_DET_ERR;
					break;
				case P_MESGIN:
					phase = "Message-In";
					mesg_out = MSG_PARITY_ERROR;
					break;
				default:
					phase = "unknown";
					break;
			}
			printf("parity error during %s phase.\n", phase);

			/*
			 * We've set the hardware to assert ATN if we   
			 * get a parity error on "in" phases, so all we  
			 * need to do is stuff the message buffer with
			 * the appropriate message.  "In" phases have set
			 * mesg_out to something other than MSG_NOP.
			 */
			if(mesg_out != MSG_NOOP) {
				AHC_OUTB(ahc, MSG0, mesg_out);
				AHC_OUTB(ahc, MSG_LEN, 1);
			}
			else
				/*
				 * Should we allow the target to make
				 * this decision for us?
				 */
				xs->error = XS_DRIVER_STUFFUP;
		}
		else if (status & SELTO) {
			u_char waiting;
			u_char flags;

			xs = scb->xs;
			xs->error = XS_SELTIMEOUT;
			/*
			 * Clear any pending messages for the timed out
			 * target, and mark the target as free
			 */
			flags = AHC_INB(ahc, FLAGS);
			AHC_OUTB(ahc, MSG_LEN, 0);
			ahc_unbusy_target(ahc, xs->xs_periph->periph_target,
				IS_SCSIBUS_B(xs->xs_periph) ? 'B' : 'A');
			/* Stop the selection */
			AHC_OUTB(ahc, SCSISEQ, 0);

			AHC_OUTB(ahc, SCB_CONTROL, 0);

			AHC_OUTB(ahc, CLRSINT1, CLRSELTIMEO);

			AHC_OUTB(ahc, CLRINT, CLRSCSIINT);

			/* Shift the waiting for selection queue forward */
			waiting = AHC_INB(ahc, WAITING_SCBH);
			AHC_OUTB(ahc, SCBPTR, waiting);
			waiting = AHC_INB(ahc, SCB_NEXT);
			AHC_OUTB(ahc, WAITING_SCBH, waiting);

			restart_sequencer(ahc);
		}       
		else if (!(status & BUSFREE)) {
		      scsipi_printaddr(scb->xs->xs_periph);
		      printf("Unknown SCSIINT. Status = 0x%x\n", status);
		      AHC_OUTB(ahc, CLRSINT1, status);
		      unpause_sequencer(ahc, /*unpause_always*/TRUE);
		      AHC_OUTB(ahc, CLRINT, CLRSCSIINT);
		      scb = NULL;
		}
		if(scb != NULL) {
		    /* We want to process the command */
		    untimeout(ahc_timeout, (caddr_t)scb);
		    ahc_done(ahc, scb);
		}
	}
	if (intstat & CMDCMPLT) {
		int   scb_index;

		do {
			scb_index = AHC_INB(ahc, QOUTFIFO);
			scb = ahc->scbarray[scb_index];
			if (!scb || !(scb->flags & SCB_ACTIVE)) {
				printf("%s: WARNING "
				       "no command for scb %d (cmdcmplt)\n"
				       "QOUTCNT == %d\n",
					ahc_name(ahc), scb_index,
					AHC_INB(ahc, QOUTCNT));
				AHC_OUTB(ahc, CLRINT, CLRCMDINT);
				continue;
			}
			AHC_OUTB(ahc, CLRINT, CLRCMDINT);
			untimeout(ahc_timeout, (caddr_t)scb);
			ahc_done(ahc, scb);

		} while (AHC_INB(ahc, QOUTCNT) & ahc->qcntmask);

		ahc_run_waiting_queues(ahc);
	}
#if defined(__NetBSD__)
	return 1;
#endif
}

static void
ahc_handle_seqint(ahc, intstat)
	struct ahc_data *ahc;
	u_int8_t intstat;
{
	struct scb *scb;
	u_short targ_mask;
	u_char target = (AHC_INB(ahc, SCSIID) >> 4) & 0x0f;
	u_char scratch_offset = target;
	char channel = AHC_INB(ahc, SBLKCTL) & SELBUSB ? 'B': 'A';

	if (channel == 'B')
		scratch_offset += 8;
	targ_mask = (0x01 << scratch_offset); 

	switch (intstat & SEQINT_MASK) {
	case NO_MATCH:
		if (ahc->flags & AHC_PAGESCBS) {
			/* SCB Page-in request */
			u_char tag;
			u_char next;
			u_char disc_scb;
			struct scb *outscb;
			u_char arg_1 = AHC_INB(ahc, ARG_1);

			/*
			 * We should succeed, so set this now.
			 * If we don't, and one of the methods
			 * we use to aquire an SCB calls ahc_done,
			 * we may wind up in our start routine
			 * and unpause the adapter without giving
			 * it the correct return value, which will
			 * cause a hang.
			 */
			AHC_OUTB(ahc, RETURN_1, SCB_PAGEDIN);

			if (arg_1 == SCB_LIST_NULL) {
				/* Non-tagged command */
				int index;
				
				index = target|(channel == 'B' ? SELBUSB : 0);
				scb = ahc->pagedout_ntscbs[index];
			} else
				scb = ahc->scbarray[arg_1];

			if (!(scb->flags & SCB_PAGED_OUT))
				panic("%s: Request to page in a non paged out "
				      "SCB.", ahc_name(ahc));
			/*
			 * Now to pick the SCB to page out.
			 * Either take a free SCB, an assigned SCB,
			 * an SCB that just completed, the first
			 * one on the disconnected SCB list, or
			 * as a last resort a queued SCB.
			 */
			if (ahc->free_scbs.stqh_first) {
				outscb = ahc->free_scbs.stqh_first; 
				STAILQ_REMOVE_HEAD(&ahc->free_scbs, links);
				scb->position = outscb->position;
				outscb->position = SCB_LIST_NULL;
				STAILQ_INSERT_HEAD(&ahc->page_scbs, outscb,
						   links);
				AHC_OUTB(ahc, SCBPTR, scb->position);
				ahc_send_scb(ahc, scb);
				scb->flags &= ~SCB_PAGED_OUT;
				goto pagein_done;
			}
			if (intstat & CMDCMPLT) {
				int   scb_index;

				AHC_OUTB(ahc, CLRINT, CLRCMDINT);
				scb_index = AHC_INB(ahc, QOUTFIFO);
				if (!(AHC_INB(ahc, QOUTCNT) & ahc->qcntmask))
					intstat &= ~CMDCMPLT;

				outscb = ahc->scbarray[scb_index];
				if (!outscb || !(outscb->flags & SCB_ACTIVE)) {
					printf("%s: WARNING no command for "
					       "scb %d (cmdcmplt)\n",
					       ahc_name(ahc),
					       scb_index);
					/*
					 * Fall through in hopes of finding
					 * another SCB
					 */
				} else {
					scb->position = outscb->position;
					outscb->position = SCB_LIST_NULL;
					AHC_OUTB(ahc, SCBPTR, scb->position);
					ahc_send_scb(ahc, scb);
					scb->flags &= ~SCB_PAGED_OUT;
					untimeout(ahc_timeout,
						  (caddr_t)outscb);
					ahc_done(ahc, outscb);
					goto pagein_done;
				}
			}
			disc_scb = AHC_INB(ahc, DISCONNECTED_SCBH);
			if (disc_scb != SCB_LIST_NULL) {
				AHC_OUTB(ahc, SCBPTR, disc_scb);
				tag = AHC_INB(ahc, SCB_TAG); 
				outscb = ahc->scbarray[tag];
				next = AHC_INB(ahc, SCB_NEXT);
				if (next != SCB_LIST_NULL) {
					AHC_OUTB(ahc, SCBPTR, next);
					AHC_OUTB(ahc, SCB_PREV,
						 SCB_LIST_NULL);
					AHC_OUTB(ahc, SCBPTR, disc_scb);
				}
				AHC_OUTB(ahc, DISCONNECTED_SCBH, next);
				ahc_page_scb(ahc, outscb, scb);
			} else if (AHC_INB(ahc, QINCNT) & ahc->qcntmask) {
				/*
				 * Pull one of our queued commands
				 * as a last resort
				 */
				disc_scb = AHC_INB(ahc, QINFIFO);
				AHC_OUTB(ahc, SCBPTR, disc_scb);
				tag = AHC_INB(ahc, SCB_TAG);
				outscb = ahc->scbarray[tag];
				if ((outscb->control & 0x23) != TAG_ENB) {
					/*
					 * This is not a simple tagged command
					 * so its position in the queue
					 * matters.  Take the command at the
					 * end of the queue instead.
					 */
					int i;
					u_char saved_queue[AHC_SCB_MAX];
					u_char queued = AHC_INB(ahc, QINCNT)
							& ahc->qcntmask;

					/*
					 * Count the command we removed
					 * already
					 */
					saved_queue[0] = disc_scb;
					queued++;

					/* Empty the input queue */
					for (i = 1; i < queued; i++) 
						saved_queue[i] = AHC_INB(ahc, QINFIFO);

					/*
					 * Put everyone back but the
					 * last entry
					 */
					queued--;
					for (i = 0; i < queued; i++)
						AHC_OUTB(ahc, QINFIFO,
							 saved_queue[i]);

					AHC_OUTB(ahc, SCBPTR,
						 saved_queue[queued]);
					tag = AHC_INB(ahc, SCB_TAG);
					outscb = ahc->scbarray[tag];
				}	
				untimeout(ahc_timeout, (caddr_t)outscb);
				scb->position = outscb->position;
				outscb->position = SCB_LIST_NULL;
				STAILQ_INSERT_HEAD(&ahc->waiting_scbs,
						   outscb, links);
				outscb->flags |= SCB_WAITINGQ;
				ahc_send_scb(ahc, scb);
				scb->flags &= ~SCB_PAGED_OUT;
			}
			else {
				panic("Page-in request with no candidates");
				AHC_OUTB(ahc, RETURN_1, 0);
			}
		  pagein_done:
		} else {
			printf("%s:%c:%d: no active SCB for reconnecting "
			       "target - issuing ABORT\n",
			       ahc_name(ahc), channel, target);
			printf("SAVED_TCL == 0x%x\n",
			       AHC_INB(ahc, SAVED_TCL));
			ahc_unbusy_target(ahc, target, channel);
			AHC_OUTB(ahc, SCB_CONTROL, 0);
			AHC_OUTB(ahc, CLRSINT1, CLRSELTIMEO);
			AHC_OUTB(ahc, RETURN_1, 0);
		}
		break;
	case SEND_REJECT: 
	{
		u_char rejbyte = AHC_INB(ahc, REJBYTE);
		printf("%s:%c:%d: Warning - unknown message received from "
		       "target (0x%x).  Rejecting\n", 
		       ahc_name(ahc), channel, target, rejbyte);
		break; 
	}
	case NO_IDENT: 
		panic("%s:%c:%d: Target did not send an IDENTIFY message. "
		      "SAVED_TCL == 0x%x\n",
		      ahc_name(ahc), channel, target,
		      AHC_INB(ahc, SAVED_TCL));
		break;
	case BAD_PHASE:
		printf("%s:%c:%d: unknown scsi bus phase.  Attempting to "
		       "continue\n", ahc_name(ahc), channel, target);	
		break; 
	case EXTENDED_MSG:
	{
		u_int8_t message_length;
		u_int8_t message_code;

		message_length = AHC_INB(ahc, MSGIN_EXT_LEN);
		message_code = AHC_INB(ahc, MSGIN_EXT_OPCODE);
		switch(message_code) {
		case MSG_EXT_SDTR:
		{
			u_int8_t period;
			u_int8_t offset;
			u_int8_t saved_offset;
			u_int8_t targ_scratch;
			u_int8_t maxoffset;
			u_int8_t rate;
			
			if (message_length != MSG_EXT_SDTR_LEN) {
				AHC_OUTB(ahc, RETURN_1, SEND_REJ);
				ahc->sdtrpending &= ~targ_mask;
				break;
			}
			period = AHC_INB(ahc, MSGIN_EXT_BYTE0);
			saved_offset = AHC_INB(ahc, MSGIN_EXT_BYTE1);
			targ_scratch = AHC_INB(ahc, TARG_SCRATCH
					       + scratch_offset);
			if (targ_scratch & WIDEXFER)
				maxoffset = MAX_OFFSET_16BIT;
			else
				maxoffset = MAX_OFFSET_8BIT;
			offset = MIN(saved_offset, maxoffset);
			ahc_scsirate(ahc, &rate, &period, &offset,
				     channel, target);
			/* Preserve the WideXfer flag */
			targ_scratch = rate | (targ_scratch & WIDEXFER);

			/*
			 * Update both the target scratch area and the
			 * current SCSIRATE.
			 */
			AHC_OUTB(ahc, TARG_SCRATCH + scratch_offset,
				 targ_scratch);
			AHC_OUTB(ahc, SCSIRATE, targ_scratch); 

			/*
			 * See if we initiated Sync Negotiation
			 * and didn't have to fall down to async
			 * transfers.
			 */
			if ((ahc->sdtrpending & targ_mask) != 0
			 && (saved_offset == offset)) {
				/*
				 * Don't send an SDTR back to
				 * the target
				 */
				AHC_OUTB(ahc, RETURN_1, 0);
				ahc->needsdtr &= ~targ_mask;
				ahc->sdtrpending &= ~targ_mask;
			} else {
				/*
				 * Send our own SDTR in reply
				 */
#ifdef AHC_DEBUG
				if(ahc_debug & AHC_SHOWMISC)
					printf("Sending SDTR!!\n");
#endif
				ahc_construct_sdtr(ahc, /*start_byte*/0,
						   period, offset);
				AHC_OUTB(ahc, RETURN_1, SEND_MSG);

				/*
				 * If we aren't starting a re-negotiation
				 * because we had to go async in response
				 * to a "too low" response from the target
				 * clear the needsdtr flag for this target.
				 */
				if ((ahc->sdtrpending & targ_mask) == 0)
					ahc->needsdtr &= ~targ_mask;
				else
					ahc->sdtrpending |= targ_mask;
			}
			break;
		}
		case MSG_EXT_WDTR:
		{
			u_int8_t scratch, bus_width;

			if (message_length != MSG_EXT_WDTR_LEN) {
				AHC_OUTB(ahc, RETURN_1, SEND_REJ);
				ahc->wdtrpending &= ~targ_mask;
				break;
			}

			bus_width = AHC_INB(ahc, MSGIN_EXT_BYTE0);
			scratch = AHC_INB(ahc, TARG_SCRATCH
					  + scratch_offset);

			if (ahc->wdtrpending & targ_mask) {
				/*
				 * Don't send a WDTR back to the
				 * target, since we asked first.
				 */
				AHC_OUTB(ahc, RETURN_1, 0);
				switch(bus_width){
				case BUS_8_BIT:
					scratch &= 0x7f;
					break;
				case BUS_16_BIT:
#if 1
					ahc_update_xfer_mode(ahc,
					    channel, target);
#else
					if(bootverbose)
						printf("%s: target %d using "
						       "16Bit transfers\n",
						       ahc_name(ahc), target);
#endif
					scratch |= WIDEXFER;	
					break;
				case BUS_32_BIT:
					/*
					 * How can we do 32bit transfers
					 * on a 16bit bus?
					 */
					AHC_OUTB(ahc, RETURN_1, SEND_REJ);
					printf("%s: target %d requested 32Bit "
					       "transfers.  Rejecting...\n",
					       ahc_name(ahc), target);
					break;
				default:
					break;
				}
			} else {
				/*
				 * Send our own WDTR in reply
				 */
				switch(bus_width) {
				case BUS_8_BIT:
					scratch &= 0x7f;
					break;
				case BUS_32_BIT:
				case BUS_16_BIT:
					if(ahc->type & AHC_WIDE) {
						/* Negotiate 16_BITS */
						bus_width = BUS_16_BIT;
#if 1
						ahc_update_xfer_mode(ahc,
						    channel, target);
#else
						if(bootverbose)
							printf("%s: target %d "
							       "using 16Bit "
							       "transfers\n",
							       ahc_name(ahc),
							       target);
#endif
						scratch |= WIDEXFER;	
					} else
						bus_width = BUS_8_BIT;
					break;
				default:
					break;
				}
				ahc_construct_wdtr(ahc, /*start_byte*/0,
						   bus_width);
				AHC_OUTB(ahc, RETURN_1, SEND_MSG);
			}
			
			ahc->needwdtr &= ~targ_mask;
			ahc->wdtrpending &= ~targ_mask;
			AHC_OUTB(ahc, TARG_SCRATCH + scratch_offset, scratch);
			AHC_OUTB(ahc, SCSIRATE, scratch); 
			break;
		}
		default:
			/* Unknown extended message.  Reject it. */
			AHC_OUTB(ahc, RETURN_1, SEND_REJ);
		}
	}
	case REJECT_MSG:
	{
		/*
		 * What we care about here is if we had an
		 * outstanding SDTR or WDTR message for this
		 * target.  If we did, this is a signal that
		 * the target is refusing negotiation.
		 */

		u_char targ_scratch;

		targ_scratch = AHC_INB(ahc, TARG_SCRATCH
				       + scratch_offset);

		if (ahc->wdtrpending & targ_mask){
			/* note 8bit xfers and clear flag */
			targ_scratch &= 0x7f;
			ahc->needwdtr &= ~targ_mask;
			ahc->wdtrpending &= ~targ_mask;
#if !defined(__NetBSD__) || defined(DEBUG)
			printf("%s:%c:%d: refuses WIDE negotiation.  Using "
			       "8bit transfers\n", ahc_name(ahc),
			       channel, target);
#endif
		} else if(ahc->sdtrpending & targ_mask){
			/* note asynch xfers and clear flag */
			targ_scratch &= 0xf0;
			ahc->needsdtr &= ~targ_mask;
			ahc->sdtrpending &= ~targ_mask;
#if !defined(__NetBSD__) || defined(DEBUG)
			printf("%s:%c:%d: refuses synchronous negotiation. "
			       "Using asynchronous transfers\n",
			       ahc_name(ahc),
			       channel, target);
#endif
		} else {
			/*
			 * Otherwise, we ignore it.
			 */
#ifdef AHC_DEBUG
			if(ahc_debug & AHC_SHOWMISC)
				printf("%s:%c:%d: Message reject -- ignored\n",
				       ahc_name(ahc), channel, target);
#endif
			break;
		}
		AHC_OUTB(ahc, TARG_SCRATCH + scratch_offset, targ_scratch);
		AHC_OUTB(ahc, SCSIRATE, targ_scratch);
		break;
	}
	case BAD_STATUS:
	{
		int	scb_index;
		struct	scsipi_xfer *xs;

		/* The sequencer will notify us when a command
		 * has an error that would be of interest to
		 * the kernel.  This allows us to leave the sequencer
		 * running in the common case of command completes
		 * without error.
		 */

		scb_index = AHC_INB(ahc, SCB_TAG);
		scb = ahc->scbarray[scb_index];

		/*
		 * Set the default return value to 0 (don't
		 * send sense).  The sense code will change
		 * this if needed and this reduces code
		 * duplication.
		 */
		AHC_OUTB(ahc, RETURN_1, 0);
		if (!(scb && (scb->flags & SCB_ACTIVE))) {
			printf("%s:%c:%d: ahc_intr - referenced scb "
			       "not valid during seqint 0x%x scb(%d)\n",
			       ahc_name(ahc),
			       channel, target, intstat,
			       scb_index);
			goto clear;
		}

		xs = scb->xs;

		scb->status = AHC_INB(ahc, SCB_TARGET_STATUS);

#ifdef AHC_DEBUG
		if((ahc_debug & AHC_SHOWSCBS)
		   && xs->xs_periph->periph_target == DEBUGTARG)
			ahc_print_scb(scb);
#endif
		xs->status = scb->status;
		switch(scb->status){
		case SCSI_OK:
			printf("%s: Interrupted for staus of"
			       " 0???\n", ahc_name(ahc));
			break;
		case SCSI_CHECK:
#ifdef AHC_DEBUG
			if(ahc_debug & AHC_SHOWSENSE)
			{

				scsipi_printaddr(xs->xs_periph);
				printf("requests Check Status\n");
			}
#endif

			if ((xs->error == XS_NOERROR)
			    && !(scb->flags & SCB_SENSE)) {
				struct ahc_dma_seg *sg = scb->ahc_dma;
				struct scsipi_sense *sc = &(scb->sense_cmd);
#ifdef AHC_DEBUG
				if (ahc_debug & AHC_SHOWSENSE)
				{
					scsipi_printaddr(xs->xs_periph);
					printf("Sending Sense\n");
				}
#endif
#if defined(__FreeBSD__)
				sc->op_code = REQUEST_SENSE;
#elif defined(__NetBSD__)
				sc->opcode = REQUEST_SENSE;
#endif
				sc->byte2 =  xs->xs_periph->periph_lun << 5;
				sc->length = sizeof(struct scsipi_sense_data);
				sc->control = 0;
#if defined(__NetBSD__)
				sg->addr =
					SCB_DMA_OFFSET(ahc, scb, scsi_sense);
#elif defined(__FreeBSD__)
				sg->addr = KVTOPHYS(&xs->AIC_SCSI_SENSE);
#endif
				sg->len = sizeof(struct scsipi_sense_data);
#if BYTE_ORDER == BIG_ENDIAN
				sg->len = bswap32(sg->len);
				sg->addr = bswap32(sg->addr);
#endif

				scb->control &= DISCENB;
				scb->status = 0;
				scb->SG_segment_count = 1;

#if defined(__NetBSD__)
				scb->SG_list_pointer =
					SCB_DMA_OFFSET(ahc, scb, ahc_dma);
#elif defined(__FreeBSD__)
				scb->SG_list_pointer = KVTOPHYS(sg);
#endif
				scb->data = sg->addr; 
				scb->datalen = sg->len;
#ifdef AHC_BROKEN_CACHE
				if (ahc_broken_cache)
					INVALIDATE_CACHE();
#endif
#if defined(__NetBSD__)
				scb->cmdpointer =
					SCB_DMA_OFFSET(ahc, scb, sense_cmd);
#elif defined(__FreeBSD__)
				scb->cmdpointer = KVTOPHYS(sc);
#endif
				scb->cmdlen = sizeof(*sc);

				scb->flags |= SCB_SENSE;
				ahc_send_scb(ahc, scb);
				/*
				 * Ensure that the target is "BUSY"
				 * so we don't get overlapping 
				 * commands if we happen to be doing
				 * tagged I/O.
				 */
				ahc_busy_target(ahc, target, channel);

				/*
				 * Make us the next command to run
				 */
				ahc_add_waiting_scb(ahc, scb);
				AHC_OUTB(ahc, RETURN_1, SEND_SENSE);
				break;
			}
			/*
			 * Clear the SCB_SENSE Flag and have
			 * the sequencer do a normal command
			 * complete with either a "DRIVER_STUFFUP"
			 * error or whatever other error condition
			 * we already had.
			 */
			scb->flags &= ~SCB_SENSE;
			if (xs->error == XS_NOERROR)
				xs->error = XS_DRIVER_STUFFUP;
			break;
		case SCSI_BUSY:
			xs->error = XS_BUSY;
			scsipi_printaddr(xs->xs_periph);
			printf("Target Busy\n");
			break;
		case SCSI_QUEUE_FULL:
			/*
			 * The upper level SCSI code will someday
			 * handle this properly.
			 */
			scsipi_printaddr(xs->xs_periph);
			printf("Queue Full\n");
			scb->flags |= SCB_ASSIGNEDQ;
			STAILQ_INSERT_TAIL(&ahc->assigned_scbs,scb, links);
			AHC_OUTB(ahc, RETURN_1, SEND_SENSE);
			break;
		default:
			scsipi_printaddr(xs->xs_periph);
			printf("unexpected targ_status: %x\n", scb->status);
			xs->error = XS_DRIVER_STUFFUP;
			break;
		}
		break;
	}
	case RESIDUAL:
	{
		int scb_index;
		struct scsipi_xfer *xs;

		scb_index = AHC_INB(ahc, SCB_TAG);
		scb = ahc->scbarray[scb_index];
		xs = scb->xs;
		/*
		 * Don't clobber valid resid info with
		 * a resid coming from a check sense
		 * operation.
		 */
		if (!(scb->flags & SCB_SENSE)) {
			int resid_sgs;

			/*
			 * Remainder of the SG where the transfer
			 * stopped.
			 */
			xs->resid = (AHC_INB(ahc, SCB_RESID_DCNT2)<<16) |
				    (AHC_INB(ahc, SCB_RESID_DCNT1)<<8)  |
				    AHC_INB(ahc, SCB_RESID_DCNT0);

			/*
			 * Add up the contents of all residual
			 * SG segments that are after the SG where
			 * the transfer stopped.
			 */
			resid_sgs = AHC_INB(ahc, SCB_RESID_SGCNT) - 1;
			while (resid_sgs > 0) {
			    int sg;

			    sg = scb->SG_segment_count - resid_sgs;
#if defined(__NetBSD_)
			    /* 'ahc_dma' might contain swapped values */
			    xs->resid += scb->dmamap_xfer->dm_segs[sg].ds_len;
#else
			    xs->resid += scb->ahc_dma[sg].len;
#endif
			    resid_sgs--;
			}

#if defined(__FreeBSD__)
			xs->flags |= SCSI_RESID_VALID;
#elif defined(__NetBSD__)
			/* XXX - Update to do this right */
#endif
#ifdef AHC_DEBUG
			if (ahc_debug & AHC_SHOWMISC) {
				scsipi_printaddr(xs->xs_periph);
				printf("Handled Residual of %d bytes\n"
				       ,xs->resid);
			}
#endif
		}
		break;
	}
	case ABORT_TAG:
	{
		int   scb_index;
		struct scsipi_xfer *xs;

		scb_index = AHC_INB(ahc, SCB_TAG);
		scb = ahc->scbarray[scb_index];
		xs = scb->xs;
		/*
		 * We didn't recieve a valid tag back from
		 * the target on a reconnect.
		 */
		scsipi_printaddr(xs->xs_periph);
		printf("invalid tag received -- sending ABORT_TAG\n");
		xs->error = XS_DRIVER_STUFFUP;
		untimeout(ahc_timeout, (caddr_t)scb);
		ahc_done(ahc, scb);
		break;
	}
	case AWAITING_MSG:
	{
		int   scb_index;
		scb_index = AHC_INB(ahc, SCB_TAG);
		scb = ahc->scbarray[scb_index];
		/*
		 * This SCB had a zero length command, informing
		 * the sequencer that we wanted to send a special
		 * message to this target.  We only do this for
		 * BUS_DEVICE_RESET messages currently.
		 */
		if (scb->flags & SCB_DEVICE_RESET) {
			AHC_OUTB(ahc, MSG0,
				 MSG_BUS_DEV_RESET);
			AHC_OUTB(ahc, MSG_LEN, 1);
			printf("Bus Device Reset Message Sent\n");
		} else if (scb->flags & SCB_MSGOUT_WDTR) {
			ahc_construct_wdtr(ahc, AHC_INB(ahc, MSG_LEN),
					   BUS_16_BIT);
		} else if (scb->flags & SCB_MSGOUT_SDTR) {
			u_int8_t target_scratch;
			u_int8_t ultraenable;			
			int sxfr;
			int i;

			/* Pull the user defined setting */
			target_scratch = AHC_INB(ahc, TARG_SCRATCH
						 + scratch_offset);
			
			sxfr = target_scratch & SXFR;
			if (scratch_offset < 8)
				ultraenable = AHC_INB(ahc, ULTRA_ENB);
			else
				ultraenable = AHC_INB(ahc, ULTRA_ENB + 1);
			
			if (ultraenable & targ_mask)
				/* Want an ultra speed in the table */
				sxfr |= 0x100;
			
			for (i = 0; i < ahc_num_syncrates; i++)
				if (sxfr == ahc_syncrates[i].sxfr)
					break;
							
			ahc_construct_sdtr(ahc, AHC_INB(ahc, MSG_LEN),
					   ahc_syncrates[i].period,
					   target_scratch & WIDEXFER ?
					   MAX_OFFSET_16BIT : MAX_OFFSET_8BIT);
		} else	
			panic("ahc_intr: AWAITING_MSG for an SCB that "
			      "does not have a waiting message");
		break;
	}
	case IMMEDDONE:
	{
		/*
		 * Take care of device reset messages
		 */
		u_char scbindex = AHC_INB(ahc, SCB_TAG);
		scb = ahc->scbarray[scbindex];
		if (scb->flags & SCB_DEVICE_RESET) {
			u_char targ_scratch;
			int found;
			/*
			 * Go back to async/narrow transfers and
			 * renegotiate.
			 */
			ahc_unbusy_target(ahc, target, channel);
			ahc->needsdtr |= ahc->needsdtr_orig & targ_mask;
			ahc->needwdtr |= ahc->needwdtr_orig & targ_mask;
			ahc->sdtrpending &= ~targ_mask;
			ahc->wdtrpending &= ~targ_mask;
			targ_scratch = AHC_INB(ahc, TARG_SCRATCH 
					       + scratch_offset);
			targ_scratch &= SXFR;
			AHC_OUTB(ahc, TARG_SCRATCH + scratch_offset,
				 targ_scratch);
			found = ahc_reset_device(ahc, target,
						 channel, SCB_LIST_NULL, 
						 XS_NOERROR);
			scsipi_printaddr(scb->xs->xs_periph);
			printf("Bus Device Reset delivered. "
			       "%d SCBs aborted\n", found);
			ahc->in_timeout = FALSE;
			ahc_run_done_queue(ahc);
		} else
			panic("ahc_intr: Immediate complete for "
			      "unknown operation.");
		break;
	}
	case DATA_OVERRUN:
	{
		/*
		 * When the sequencer detects an overrun, it
		 * sets STCNT to 0x00ffffff and allows the
		 * target to complete its transfer in
		 * BITBUCKET mode.
		 */
		u_char scbindex = AHC_INB(ahc, SCB_TAG);
		u_int32_t overrun;
		scb = ahc->scbarray[scbindex];
		overrun = AHC_INB(ahc, STCNT0)
			| (AHC_INB(ahc, STCNT1) << 8)
			| (AHC_INB(ahc, STCNT2) << 16);
		overrun = 0x00ffffff - overrun;
		scsipi_printaddr(scb->xs->xs_periph);
		printf("data overrun of %d bytes detected."
		       "  Forcing a retry.\n", overrun);
		/*
		 * Set this and it will take affect when the
		 * target does a command complete.
		 */
		scb->xs->error = XS_DRIVER_STUFFUP;
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
		       intstat, AHC_INB(ahc, SCSISIGI));
		break;
	}
	
clear:
	/*
	 * Clear the upper byte that holds SEQINT status
	 * codes and clear the SEQINT bit.
	 */
	AHC_OUTB(ahc, CLRINT, CLRSEQINT);

	/*
	 *  The sequencer is paused immediately on
	 *  a SEQINT, so we should restart it when
	 *  we're done.
	 */
	unpause_sequencer(ahc, /*unpause_always*/TRUE);
}

/*
 * We have a scb which has been processed by the
 * adaptor, now we look to see how the operation
 * went.
 */
static void
ahc_done(ahc, scb)
	struct ahc_data *ahc;
	struct scb *scb;
{
	struct scsipi_xfer *xs = scb->xs;

	SC_DEBUG(xs->sc_link, SDEV_DB2, ("ahc_done\n"));

#if defined(__NetBSD__)
	/*
	 * If we were a data transfer, unload the map that described
	 * the data buffer.
	 */
	if (xs->datalen) {
		bus_dmamap_sync(ahc->sc_dt, scb->dmamap_xfer, 0,
			scb->dmamap_xfer->dm_mapsize,
			(xs->xs_control & XS_CTL_DATA_IN) ?
			BUS_DMASYNC_POSTREAD : BUS_DMASYNC_POSTWRITE);
		bus_dmamap_unload(ahc->sc_dt, scb->dmamap_xfer);
	}
	/*
	 * Sync the scb map, so all it's contents are valid
	 */
	bus_dmamap_sync(ahc->sc_dt, ahc->sc_dmamap_control,
		(scb)->tag * sizeof(struct scb), sizeof(struct scb),
		BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);
		
#endif
	/*
	 * Put the results of the operation
	 * into the xfer and call whoever started it
	 */
#if defined(__NetBSD__)
	if (xs->error != XS_NOERROR) {
		/* Don't override the error value. */
	} else if (scb->flags & SCB_ABORTED) {
		xs->error = XS_DRIVER_STUFFUP;
	} else
#endif
	if(scb->flags & SCB_SENSE) {
		xs->error = XS_SENSE;
#if defined(__NetBSD__)
		bcopy(&scb->scsi_sense, &xs->sense.scsi_sense,
			sizeof(scb->scsi_sense));
#endif
	}
	if(scb->flags & SCB_SENTORDEREDTAG)
		ahc->in_timeout = FALSE;
#if defined(__FreeBSD__)
	if ((xs->flags & SCSI_ERR_OK) && !(xs->error == XS_SENSE)) {
		/* All went correctly  OR errors expected */
		xs->error = XS_NOERROR;
	}
#elif defined(__NetBSD__)
	/*
	 * Since NetBSD doesn't have error ignoring operation mode
	 * (SCSI_ERR_OK in FreeBSD), we don't have to care this case.
	 */
#endif
	ahc_free_scb(ahc, scb);
	scsipi_done(xs);
}

/*
 * Start the board, ready for normal operation
 */
int
ahc_init(ahc)
	struct  ahc_data *ahc;
{
	u_int8_t  scsi_conf, sblkctl, i;
	u_int16_t ultraenable = 0;
	int	  max_targ = 15;
#if defined(__NetBSD__)
	bus_dma_segment_t	seg;
	int			error, rseg, scb_size;
	struct scb		*scb_space;
#endif

	/*
	 * Assume we have a board at this stage and it has been reset.
	 */

	/* Handle the SCBPAGING option */
#ifndef AHC_SCBPAGING_ENABLE
	ahc->flags &= ~AHC_PAGESCBS;
#endif

	/* Determine channel configuration and who we are on the scsi bus. */
	switch ( (sblkctl = AHC_INB(ahc, SBLKCTL) & 0x0a) ) {
	    case 0:
		ahc->our_id = (AHC_INB(ahc, SCSICONF) & HSCSIID);
		ahc->flags &= ~AHC_CHANNEL_B_PRIMARY;
		if(ahc->type == AHC_394)
			printf("Channel %c, SCSI Id=%d, ", 
				ahc->flags & AHC_CHNLB ? 'B' : 'A',
				ahc->our_id);
		else
			printf("Single Channel, SCSI Id=%d, ", ahc->our_id);
		AHC_OUTB(ahc, FLAGS, SINGLE_BUS | (ahc->flags & AHC_PAGESCBS));
		break;
	    case 2:
		ahc->our_id = (AHC_INB(ahc, SCSICONF + 1) & HWSCSIID);
		ahc->flags &= ~AHC_CHANNEL_B_PRIMARY;
		if(ahc->type == AHC_394)
			printf("Wide Channel %c, SCSI Id=%d, ", 
				ahc->flags & AHC_CHNLB ? 'B' : 'A',
				ahc->our_id);
		else
			printf("Wide Channel, SCSI Id=%d, ", ahc->our_id);
		ahc->type |= AHC_WIDE;
		AHC_OUTB(ahc, FLAGS, WIDE_BUS | (ahc->flags & AHC_PAGESCBS));
		break;
	    case 8:
		ahc->our_id = (AHC_INB(ahc, SCSICONF) & HSCSIID);
		ahc->our_id_b = (AHC_INB(ahc, SCSICONF + 1) & HSCSIID);
		printf("Twin Channel, A SCSI Id=%d, B SCSI Id=%d, ",
			ahc->our_id, ahc->our_id_b);
		ahc->type |= AHC_TWIN;
		AHC_OUTB(ahc, FLAGS, TWIN_BUS | (ahc->flags & AHC_PAGESCBS));
		break;
	    default:
		printf(" Unsupported adapter type.  Ignoring\n");
		return(-1);
	}

	/* Determine the number of SCBs */

	{
		AHC_OUTB(ahc, SCBPTR, 0);
		AHC_OUTB(ahc, SCB_CONTROL, 0);
		for(i = 1; i < AHC_SCB_MAX; i++) {
			AHC_OUTB(ahc, SCBPTR, i);
			AHC_OUTB(ahc, SCB_CONTROL, i);
			if(AHC_INB(ahc, SCB_CONTROL) != i)
				break;
			AHC_OUTB(ahc, SCBPTR, 0);
			if(AHC_INB(ahc, SCB_CONTROL) != 0)
				break;
			/* Clear the control byte. */
			AHC_OUTB(ahc, SCBPTR, i);
			AHC_OUTB(ahc, SCB_CONTROL, 0);

			ahc->qcntmask |= i;     /* Update the count mask. */
		}

		/* Ensure we clear the 0 SCB's control byte. */
		AHC_OUTB(ahc, SCBPTR, 0);
		AHC_OUTB(ahc, SCB_CONTROL, 0);

		ahc->qcntmask |= i;
		ahc->maxhscbs = i;
	}

	if((ahc->maxhscbs < AHC_SCB_MAX) && (ahc->flags & AHC_PAGESCBS))
		ahc->maxscbs = AHC_SCB_MAX;
	else {
		ahc->maxscbs = ahc->maxhscbs;
		ahc->flags &= ~AHC_PAGESCBS;
	}
#if defined(__NetBSD__)
	/*
	 * We allocate the space for all control-blocks at once in
	 * dma-able memory.
	 */
	scb_size = ahc->maxscbs * sizeof(struct scb);
	if ((error = bus_dmamem_alloc(ahc->sc_dt, scb_size, 
			NBPG, 0, &seg, 1, &rseg, BUS_DMA_NOWAIT)) != 0) {
		printf("%s: unable to allocate control structures, "
			"error = %d\n", ahc_name(ahc), error);
		return -1;
	}
	if ((error = bus_dmamem_map(ahc->sc_dt, &seg, rseg, scb_size,
			(caddr_t *)&scb_space,
			BUS_DMA_NOWAIT|BUS_DMA_COHERENT)) != 0) {
		printf("%s: unable to map control structures, error = %d\n",
			ahc_name(ahc), error);
		return -1;
	}
	if ((error = bus_dmamap_create(ahc->sc_dt, scb_size, 1, scb_size,
			0, BUS_DMA_NOWAIT | ahc->sc_dmaflags,
			&ahc->sc_dmamap_control)) != 0) {
                printf("%s: unable to create control DMA map, error = %d\n",
			ahc_name(ahc), error);
                return -1;
        }
	if ((error = bus_dmamap_load(ahc->sc_dt, ahc->sc_dmamap_control,
			scb_space, scb_size, NULL, BUS_DMA_NOWAIT)) != 0) {
                printf("%s: unable to load control DMA map, error = %d\n",
                    ahc_name(ahc), error);
                return -1;
        }
	for (i = 0; i < ahc->maxscbs; i++) {
		if (ahc_new_scb(ahc, &scb_space[i]) == NULL)
			break;
		STAILQ_INSERT_HEAD(&ahc->page_scbs, &scb_space[i], links);
	}
	ahc->maxscbs = i;
#endif
	printf("%d SCBs\n", ahc->maxhscbs);

#ifdef AHC_DEBUG
	if(ahc_debug & AHC_SHOWMISC) {
		struct scb	test;
		printf("%s: hardware scb %ld bytes; kernel scb %d bytes; "
		       "ahc_dma %d bytes\n",
			ahc_name(ahc),
		        (u_long)&(test.next) - (u_long)(&test),
			sizeof(test),
			sizeof(struct ahc_dma_seg));
	}
#endif /* AHC_DEBUG */

	/* Set the SCSI Id, SXFRCTL0, SXFRCTL1, and SIMODE1, for both channels*/
	if(ahc->type & AHC_TWIN)
	{
		/*
		 * The device is gated to channel B after a chip reset,
		 * so set those values first
		 */
		AHC_OUTB(ahc, SCSIID, ahc->our_id_b);
		scsi_conf = AHC_INB(ahc, SCSICONF + 1);
		AHC_OUTB(ahc, SXFRCTL1, (scsi_conf & (ENSPCHK|STIMESEL))
					| ENSTIMER|ACTNEGEN|STPWEN);
		AHC_OUTB(ahc, SIMODE1, ENSELTIMO|ENSCSIRST|ENSCSIPERR);
		if(ahc->type & AHC_ULTRA)
			AHC_OUTB(ahc, SXFRCTL0, DFON|SPIOEN|ULTRAEN);
		else
			AHC_OUTB(ahc, SXFRCTL0, DFON|SPIOEN);

		if(scsi_conf & RESET_SCSI) {
			/* Reset the bus */
#if !defined(__NetBSD__) || (defined(__NetBSD__) && defined(DEBUG))
			if(bootverbose)
				printf("%s: Resetting Channel B\n",
				       ahc_name(ahc));
#endif
			AHC_OUTB(ahc, SCSISEQ, SCSIRSTO);
			DELAY(1000);
			AHC_OUTB(ahc, SCSISEQ, 0);

			/* Ensure we don't get a RSTI interrupt from this */
			AHC_OUTB(ahc, CLRSINT1, CLRSCSIRSTI);
			AHC_OUTB(ahc, CLRINT, CLRSCSIINT);
		}

		/* Select Channel A */
		AHC_OUTB(ahc, SBLKCTL, 0);
	}
	AHC_OUTB(ahc, SCSIID, ahc->our_id);
	scsi_conf = AHC_INB(ahc, SCSICONF);
	AHC_OUTB(ahc, SXFRCTL1, (scsi_conf & (ENSPCHK|STIMESEL))
				| ENSTIMER|ACTNEGEN|STPWEN);
	AHC_OUTB(ahc, SIMODE1, ENSELTIMO|ENSCSIRST|ENSCSIPERR);
	if(ahc->type & AHC_ULTRA)
		AHC_OUTB(ahc, SXFRCTL0, DFON|SPIOEN|ULTRAEN);
	else
		AHC_OUTB(ahc, SXFRCTL0, DFON|SPIOEN);

	if(scsi_conf & RESET_SCSI) {
		/* Reset the bus */
#if !defined(__NetBSD__) || (defined(__NetBSD__) && defined(DEBUG))
		if(bootverbose)
			printf("%s: Resetting Channel A\n", ahc_name(ahc));
#endif

		AHC_OUTB(ahc, SCSISEQ, SCSIRSTO);
		DELAY(1000);
		AHC_OUTB(ahc, SCSISEQ, 0);

		/* Ensure we don't get a RSTI interrupt from this */
		AHC_OUTB(ahc, CLRSINT1, CLRSCSIRSTI);
		AHC_OUTB(ahc, CLRINT, CLRSCSIINT);
	}

	/*
	 * Look at the information that board initialization or
	 * the board bios has left us.  In the lower four bits of each
	 * target's scratch space any value other than 0 indicates
	 * that we should initiate synchronous transfers.  If it's zero,
	 * the user or the BIOS has decided to disable synchronous
	 * negotiation to that target so we don't activate the needsdtr
	 * flag.
	 */
	ahc->needsdtr_orig = 0;
	ahc->needwdtr_orig = 0;

	/* Grab the disconnection disable table and invert it for our needs */
	if(ahc->flags & AHC_USEDEFAULTS) {
		printf("%s: Host Adapter Bios disabled.  Using default SCSI "
			"device parameters\n", ahc_name(ahc));
		ahc->discenable = 0xff;
	}
	else
		ahc->discenable = ~((AHC_INB(ahc, DISC_DSB + 1) << 8)
				   | AHC_INB(ahc, DISC_DSB));

	if(!(ahc->type & (AHC_WIDE|AHC_TWIN)))
		max_targ = 7;

	for(i = 0; i <= max_targ; i++){
		u_char target_settings;
		if (ahc->flags & AHC_USEDEFAULTS) {
			target_settings = 0; /* 10MHz */
			ahc->needsdtr_orig |= (0x01 << i);
			ahc->needwdtr_orig |= (0x01 << i);
		}
		else {
			/* Take the settings leftover in scratch RAM. */
			target_settings = AHC_INB(ahc, TARG_SCRATCH + i);

			if(target_settings & 0x0f){
				ahc->needsdtr_orig |= (0x01 << i);
				/*Default to asynchronous transfers(0 offset)*/
				target_settings &= 0xf0;
			}
			if(target_settings & 0x80){
				ahc->needwdtr_orig |= (0x01 << i);
				/*
				 * We'll set the Wide flag when we
				 * are successful with Wide negotiation.
				 * Turn it off for now so we aren't
				 * confused.
				 */
				target_settings &= 0x7f;
			}
			if(ahc->type & AHC_ULTRA) {
				/*
				 * Enable Ultra for any target that
				 * has a valid ultra syncrate setting.
				 */
				u_char rate = target_settings & 0x70;
				if(rate == 0x00 || rate == 0x10 ||
				   rate == 0x20 || rate == 0x40) {
					if(rate == 0x40) {
						/* Treat 10MHz specially */
						target_settings &= ~0x70;
					}
					else
						ultraenable |= (0x01 << i);
				}
			}
		}
		AHC_OUTB(ahc, TARG_SCRATCH+i,target_settings);
	}
	/*
	 * If we are not a WIDE device, forget WDTR.  This
	 * makes the driver work on some cards that don't
	 * leave these fields cleared when the BIOS is not
	 * installed.
	 */
	if(!(ahc->type & AHC_WIDE))
		ahc->needwdtr_orig = 0;
#if 0
	/*
	 * XXX THORPEJ
	 * We now explicity "set xfer mode", but we keep the _orig's around
	 * for that routine.
	 */
	ahc->needsdtr = ahc->needsdtr_orig;
	ahc->needwdtr = ahc->needwdtr_orig;
#else
	ahc->needsdtr = 0;
	ahc->needwdtr = 0;
#endif
	ahc->sdtrpending = 0;
	ahc->wdtrpending = 0;
	ahc->tagenable = 0;
	ahc->orderedtag = 0;

	AHC_OUTB(ahc, ULTRA_ENB, ultraenable & 0xff);
	AHC_OUTB(ahc, ULTRA_ENB + 1, (ultraenable >> 8) & 0xff);

#ifdef AHC_DEBUG
	/* How did we do? */
	if(ahc_debug & AHC_SHOWMISC)
		printf("NEEDSDTR == 0x%x\nNEEDWDTR == 0x%x\n"
			"DISCENABLE == 0x%x\n", ahc->needsdtr, 
			ahc->needwdtr, ahc->discenable);
#endif
	/*
	 * Set the number of available SCBs
	 */
	AHC_OUTB(ahc, SCBCOUNT, ahc->maxhscbs);

	/*
	 * 2's compliment of maximum tag value
	 */
	i = ahc->maxscbs;
	AHC_OUTB(ahc, COMP_SCBCOUNT, -i & 0xff);

	/*
	 * QCount mask to deal with broken aic7850s that
	 * sporadically get garbage in the upper bits of
	 * their QCount registers.
	 */
	AHC_OUTB(ahc, QCNTMASK, ahc->qcntmask);

	/* We don't have any busy targets right now */
	AHC_OUTB(ahc, ACTIVE_A, 0);
	AHC_OUTB(ahc, ACTIVE_B, 0);

	/* We don't have any waiting selections */
	AHC_OUTB(ahc, WAITING_SCBH, SCB_LIST_NULL);

	/* Our disconnection list is empty too */
	AHC_OUTB(ahc, DISCONNECTED_SCBH, SCB_LIST_NULL);

	/* Message out buffer starts empty */
	AHC_OUTB(ahc, MSG_LEN, 0x00);

	/*
	 * Load the Sequencer program and Enable the adapter
	 * in "fast" mode.
         */
#if !defined(__NetBSD__) || (defined(__NetBSD__) && defined(DEBUG))
	if(bootverbose)
		printf("%s: Downloading Sequencer Program...",
		       ahc_name(ahc));
#endif

	ahc_loadseq(ahc);

#if !defined(__NetBSD__) || (defined(__NetBSD__) && defined(DEBUG))
	if(bootverbose)
		printf("Done\n");
#endif

	AHC_OUTB(ahc, SEQCTL, FASTMODE);

	unpause_sequencer(ahc, /*unpause_always*/TRUE);

	/*
	 * Note that we are going and return (to probe)
	 */
	ahc->flags |= AHC_INIT;
	return (0);
}

void
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
        if (bp->b_bcount > ((AHC_NSEG - 1) * PAGE_SIZE)) {
                bp->b_bcount = ((AHC_NSEG - 1) * PAGE_SIZE);
        }
#if defined(__NetBSD__)
	minphys(bp);
#endif
}

/*
 * start a scsi operation given the command and
 * the data address, target, and lun all of which
 * are stored in the scsipi_xfer struct
 */
void
ahc_scsipi_request(chan, req, arg)
	struct scsipi_channel *chan;
	scsipi_adapter_req_t req;
	void *arg;
{
	struct scsipi_xfer *xs;
	struct scsipi_periph *periph;
	struct ahc_data *ahc = (void *)chan->chan_adapter->adapt_dev;
	struct	scb *scb;
	struct	ahc_dma_seg *sg;
	int	seg;		/* scatter gather seg being worked on */
	int	flags;
	u_short	mask;
	int	error;
	int	s;

	SC_DEBUG(xs->sc_link, SDEV_DB2, ("ahc_scsipi_request\n"));

	switch (req) {
	case ADAPTER_REQ_RUN_XFER:
		xs = arg;
		periph = xs->xs_periph;
		mask = (0x01 << (periph->periph_target |
		    (IS_SCSIBUS_B(xs->xs_periph) ? SELBUSB : 0)));

		/*
		 * get an scb to use. If the transfer
		 * is from a buf (possibly from interrupt time)
		 * then we can't allow it to sleep
		 */
		flags = xs->xs_control;
		scb = ahc_get_scb(ahc);
#ifdef DIAGNOSTIC
		/*
		 * This should never happen as we track the resources
		 * in the mid-layer.
		 */
		if (scb == NULL) {
			scsipi_printaddr(periph);
			printf("unable to allocate scb\n");
			panic("ahc_scsipi_request");
		}
#endif

		SC_DEBUG(xs->sc_link, SDEV_DB3, ("start scb(%p)\n", scb));
		scb->xs = xs;
		if (flags & XS_CTL_RESET) {
			scb->flags |= SCB_DEVICE_RESET|SCB_IMMED;
			scb->control |= MK_MESSAGE;
		}

		/*
		 * Put all the arguments for the xfer in the scb
		 */
		if (XS_CTL_TAGTYPE(xs) != 0)
			scb->control |= xs->xs_tag_type;
		if(ahc->discenable & mask)
			scb->control |= DISCENB;
		if((ahc->needwdtr & mask) && !(ahc->wdtrpending & mask))
		{
			scb->control |= MK_MESSAGE;
			scb->flags |= SCB_MSGOUT_WDTR;
			ahc->wdtrpending |= mask;
		}
		else if((ahc->needsdtr & mask) && !(ahc->sdtrpending & mask))
		{
			scb->control |= MK_MESSAGE;
			scb->flags |= SCB_MSGOUT_SDTR;
			ahc->sdtrpending |= mask;
		}
		scb->tcl = ((periph->periph_target << 4) & 0xF0) |
					  (IS_SCSIBUS_B(xs->xs_periph) ?
					   SELBUSB : 0) |
					  (periph->periph_lun & 0x07);
		scb->cmdlen = xs->cmdlen;
		bcopy(xs->cmd, &scb->scsi_cmd, xs->cmdlen);
		scb->cmdpointer = SCB_DMA_OFFSET(ahc, scb, scsi_cmd);
		xs->resid = 0;
		xs->status = 0;
		if (xs->datalen) {
			SC_DEBUG(xs->sc_link, SDEV_DB4,
				 ("%ld @%p:- ", (long)xs->datalen, xs->data));

			error = bus_dmamap_load(ahc->sc_dt, scb->dmamap_xfer,
				    xs->data, xs->datalen, NULL,
				    BUS_DMA_NOWAIT);
			switch (error) {
			case 0:
				break;

			case ENOMEM:
			case EAGAIN:
				xs->error = XS_RESOURCE_SHORTAGE;
				goto out_bad;

			default:
				xs->error = XS_DRIVER_STUFFUP;
				SC_DEBUGN(xs->sc_link, SDEV_DB4, ("\n"));
				printf("%s: error %d loading DMA map\n",
				    ahc_name(ahc), error);
 out_bad:
				ahc_free_scb(ahc, scb);
				scsipi_done(xs);
				return;
			}
			bus_dmamap_sync(ahc->sc_dt, scb->dmamap_xfer, 0,
				scb->dmamap_xfer->dm_mapsize,
				(flags & XS_CTL_DATA_IN) ?
				BUS_DMASYNC_PREREAD : BUS_DMASYNC_PREWRITE);
			/*
			 * Load the hardware scatter/gather map with the
			 * contents of the DMA map.
			 */
			scb->SG_list_pointer =
			    SCB_DMA_OFFSET(ahc, scb, ahc_dma);

			sg = scb->ahc_dma;
			for (seg = 0; seg < scb->dmamap_xfer->dm_nsegs; seg++) {
				sg->addr =
				    scb->dmamap_xfer->dm_segs[seg].ds_addr;
				sg->len =
				    scb->dmamap_xfer->dm_segs[seg].ds_len;
				SC_DEBUGN(xs->sc_link, SDEV_DB4, ("0x%lx",
						(u_long)sg->addr));
#if BYTE_ORDER == BIG_ENDIAN
				sg->addr = bswap32(sg->addr);
				sg->len  = bswap32(sg->len);
#endif
				sg++;
			}
			SC_DEBUGN(xs->sc_link, SDEV_DB4, ("\n"));
			scb->SG_segment_count = seg;

			/* Copy the first SG into the data pointer area */
			scb->data = scb->ahc_dma->addr;
			scb->datalen = scb->ahc_dma->len;
#ifdef AHC_BROKEN_CACHE
			if (ahc_broken_cache)
				INVALIDATE_CACHE();
#endif
		}
		else {
			/*
			 * No data xfer, use non S/G values
		 	 */
			scb->SG_segment_count = 0;
			scb->SG_list_pointer = 0;
			scb->data = 0;
			scb->datalen = 0;
		}
		bus_dmamap_sync(ahc->sc_dt, ahc->sc_dmamap_control,
			(scb)->tag * sizeof(struct scb), sizeof(struct scb),
			BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

#ifdef AHC_DEBUG
		if((ahc_debug & AHC_SHOWSCBS) &&
			(xs->sc_link->AIC_SCSI_TARGET == DEBUGTARG))
			ahc_print_scb(scb);
#endif
		s = splbio();

		if( scb->position != SCB_LIST_NULL )
		{
			/* We already have a valid slot */
			u_char curscb;

			pause_sequencer(ahc);
			curscb = AHC_INB(ahc, SCBPTR);
			AHC_OUTB(ahc, SCBPTR, scb->position);
			ahc_send_scb(ahc, scb);
			AHC_OUTB(ahc, SCBPTR, curscb);
			AHC_OUTB(ahc, QINFIFO, scb->position);
			unpause_sequencer(ahc, /*unpause_always*/FALSE);
			scb->flags |= SCB_ACTIVE;
			if (!(flags & XS_CTL_POLL)) {
				timeout(ahc_timeout, (caddr_t)scb,
					(xs->timeout * hz) / 1000);
			}
			SC_DEBUG(xs->sc_link, SDEV_DB3, ("cmd_sent\n"));
		}
		else {
			scb->flags |= SCB_WAITINGQ;
			STAILQ_INSERT_TAIL(&ahc->waiting_scbs, scb, links);
			ahc_run_waiting_queues(ahc);
		}
		if ((flags & XS_CTL_POLL) == 0) {
			splx(s);
			return;
		}
		/*
		 * If we can't use interrupts, poll for completion
		 */
		SC_DEBUG(xs->sc_link, SDEV_DB3, ("cmd_poll\n"));
		do {
			if (ahc_poll(ahc, xs->timeout)) {
				if (!(xs->xs_control & XS_CTL_SILENT))
					printf("cmd fail\n");
				ahc_timeout(scb);
				break;
			}
		} while ((xs->xs_status & XS_STS_DONE) == 0);
		splx(s); 
		return;

	case ADAPTER_REQ_GROW_RESOURCES:
		/* XXX Not supported. */
		return;

	case ADAPTER_REQ_SET_XFER_MODE:
	    {
		struct scsipi_xfer_mode *xm = arg;
		int tmask;

		tmask = 1 << xm->xm_target;

#ifdef AHC_TAGENABLE
		if (xm->xm_mode & PERIPH_CAP_TQING) {
			struct scsipi_max_openings mo;

			ahc->tagenable |= tmask;

			mo.mo_target = xm->xm_target;
			mo.mo_lun = -1;	/* all LUNs */

			/*
			 * Default to 8 openings per periph if we have
			 * a decent amount of SCB space or can page them.
			 *
			 * Otherwise, go with 4 openings in an effort to
			 * avoid letting one device hog the adapter.
			 */
			if (ahc->maxhscbs >= 16 ||
			    (ahc->flags & AHC_PAGESCBS) != 0)
				mo.mo_openings = 8;
			else
				mo.mo_openings = 4;

			scsipi_async_event(chan, ASYNC_EVENT_MAX_OPENINGS, &mo);
		}
#endif /* AHC_TAGENABLE */
		if ((xm->xm_mode & PERIPH_CAP_SYNC) != 0 &&
		    (ahc->needsdtr_orig & tmask) != 0)
			ahc->needsdtr |= tmask;
		if ((xm->xm_mode & PERIPH_CAP_WIDE16) != 0 &&
		    (ahc->needwdtr_orig & tmask) != 0)
			ahc->needwdtr |= tmask;

		/*
		 * If we're not going to negotiate, update the mode
		 * now since it won't happen later.
		 */
		if (((ahc->needsdtr | ahc->needwdtr) & tmask) == 0)
			ahc_update_xfer_mode(ahc, chan->chan_channel,
			    xm->xm_target);
		return;
	    }
	}
}

static void
ahc_update_xfer_mode(ahc, channel, target)
	struct ahc_data *ahc;
	int channel, target;
{
	struct scsipi_xfer_mode xm;
	u_int8_t target_scratch, scratch_offset, ultraenable;
	int i, sxfr, tmask;

	xm.xm_target = target;
	xm.xm_mode = 0;
	xm.xm_period = 0;
	xm.xm_offset = 0;

	tmask = 1 << (target & 7);
	scratch_offset = target + ((channel == 1) ? 8 : 0);

	if (ahc->tagenable & (1 << target))
		xm.xm_mode |= PERIPH_CAP_TQING;

	target_scratch = AHC_INB(ahc, TARG_SCRATCH + scratch_offset);
	sxfr = target_scratch & SXFR;

	if (target_scratch & WIDEXFER)
		xm.xm_mode |= PERIPH_CAP_WIDE16;

	if ((sxfr & 0x0f) != 0) {
		xm.xm_mode |= PERIPH_CAP_SYNC;
		xm.xm_offset = sxfr & 0x0f;

		if (scratch_offset < 8)
			ultraenable = AHC_INB(ahc, ULTRA_ENB);
		else
			ultraenable = AHC_INB(ahc, ULTRA_ENB + 1);

		if (ultraenable & tmask)
			sxfr |= 0x100;

		for (i = 0; i < ahc_num_syncrates; i++)
			if (sxfr == ahc_syncrates[i].sxfr)
				break;

		xm.xm_period = ahc_syncrates[i].period;
	}

	scsipi_async_event(&ahc->sc_channels[channel],
	    ASYNC_EVENT_XFER_MODE, &xm);
}


/*
 * A scb (and hence an scb entry on the board) is put onto the
 * free list.
 */
static void
ahc_free_scb(ahc, scb)
        struct	ahc_data *ahc;
        struct  scb *scb;
{
	struct scb *wscb;
	unsigned int opri;

	opri = splbio();

	/* Clean up for the next user */
	scb->flags = SCB_FREE;
	scb->control = 0;
	scb->status = 0;

	if(scb->position == SCB_LIST_NULL) {
		STAILQ_INSERT_HEAD(&ahc->page_scbs, scb, links);
	}
	/*
	 * If there are any SCBS on the waiting queue,
	 * assign the slot of this "freed" SCB to the first
	 * one.  We'll run the waiting queues after all command
	 * completes for a particular interrupt are completed
	 * or when we start another command.
	 */
	else if((wscb = ahc->waiting_scbs.stqh_first) != NULL) {
		STAILQ_REMOVE_HEAD(&ahc->waiting_scbs, links);
		wscb->position = scb->position;
		STAILQ_INSERT_TAIL(&ahc->assigned_scbs, wscb, links);
		wscb->flags ^= SCB_WAITINGQ|SCB_ASSIGNEDQ;

		/* 
		 * The "freed" SCB will need to be assigned a slot
		 * before being used, so put it in the page_scbs
		 * queue.
		 */
		scb->position = SCB_LIST_NULL;
		STAILQ_INSERT_HEAD(&ahc->page_scbs, scb, links);
	}
	else {
		STAILQ_INSERT_HEAD(&ahc->free_scbs, scb, links);
	}
#ifdef AHC_DEBUG
	ahc->activescbs--;
#endif
	splx(opri);
}

/*
 * Allocate and initialize a new scb
 */
static struct scb *
ahc_new_scb(ahc, scbp)
	struct ahc_data *ahc;
	struct scb	*scbp;
{
#if defined(__NetBSD__)
	int		error;
#endif

	if (scbp == NULL)
	    scbp = (struct scb *) malloc(sizeof(struct scb), M_TEMP, M_NOWAIT);

	if (scbp != NULL) {
		bzero(scbp, sizeof(struct scb));
#if defined(__NetBSD__)
		error =  bus_dmamap_create(ahc->sc_dt, AHC_MAXXFER, AHC_NSEG,
			    AHC_MAXXFER, 0,
			    BUS_DMA_NOWAIT|BUS_DMA_ALLOCNOW|ahc->sc_dmaflags,
			    &scbp->dmamap_xfer);
		if (error) {
			printf("%s: unable to create DMA map, error = %d\n",
			    ahc_name(ahc), error);
			free(scbp, M_TEMP);
			return (NULL);
		}
#endif
		scbp->tag = ahc->numscbs;
		if( ahc->numscbs < ahc->maxhscbs )
			scbp->position = ahc->numscbs;
		else
			scbp->position = SCB_LIST_NULL;
		ahc->numscbs++;
		/*
		 * Place in the scbarray
		 * Never is removed.
		 */
		ahc->scbarray[scbp->tag] = scbp;
	}
	else {
		printf("%s: Can't malloc SCB\n", ahc_name(ahc));
	}
	return (scbp);
}

/*
 * Get a free scb, either one already assigned to a hardware slot
 * on the adapter or one that will require an SCB to be paged out before
 * use. If there are none, see if we can allocate a new SCB.  Otherwise
 * either return an error or sleep.
 */
static struct scb *
ahc_get_scb(ahc)
        struct	ahc_data *ahc;
{
	unsigned opri;
	struct scb *scbp;

	opri = splbio();
	if((scbp = ahc->free_scbs.stqh_first)) {
		STAILQ_REMOVE_HEAD(&ahc->free_scbs, links);
	}
	else if((scbp = ahc->page_scbs.stqh_first)) {
		STAILQ_REMOVE_HEAD(&ahc->page_scbs, links);
	}
#ifdef AHC_DEBUG
	if (scbp) {
		ahc->activescbs++;
		if((ahc_debug & AHC_SHOWSCBCNT)
		  && (ahc->activescbs == ahc->maxhscbs))
			printf("%s: Max SCBs active\n", ahc_name(ahc));
	}
#endif

	splx(opri);

	return (scbp);
}

static void ahc_loadseq(ahc)
	struct ahc_data *ahc;
{
        static u_char seqprog[] = {
#if defined(__FreeBSD__)
#               include "aic7xxx_seq.h"
#endif
#if defined(__NetBSD__)
#		include <dev/microcode/aic7xxx/aic7xxx_seq.h>
#endif
	};

	AHC_OUTB(ahc, SEQCTL, PERRORDIS|SEQRESET|LOADRAM);

	AHC_OUTSB(ahc, SEQRAM, seqprog, sizeof(seqprog));

	do {
		AHC_OUTB(ahc, SEQCTL, SEQRESET|FASTMODE);
	} while((AHC_INB(ahc, SEQADDR0) != 0)
		|| (AHC_INB(ahc, SEQADDR1) != 0));
}

/*
 * Function to poll for command completion when
 * interrupts are disabled (crash dumps)
 */
static int
ahc_poll(ahc, wait)
	struct	ahc_data *ahc;
	int	wait; /* in msec */
{
	while (--wait) {
		DELAY(1000);
		if (AHC_INB(ahc, INTSTAT) & INT_PEND)
			break;
	} if (wait == 0) {
		printf("%s: board is not responding\n", ahc_name(ahc));
		return (EIO);
	}
	ahc_intr((void *)ahc);
	return (0);
}

static void
ahc_timeout(arg)
	void	*arg;
{
	struct	scb *scb = (struct scb *)arg;
	struct	ahc_data *ahc;
	int	s, found;
	u_char	bus_state;
	char	channel;

	s = splbio();

	if (!(scb->flags & SCB_ACTIVE)) {
		/* Previous timeout took care of me already */
		splx(s);
		return;
	}

	ahc =
	    (void *)scb->xs->xs_periph->periph_channel->chan_adapter->adapt_dev;

	if (ahc->in_timeout) {
		/*
		 * Some other SCB has started a recovery operation
		 * and is still working on cleaning things up.
		 */
		if (scb->flags & SCB_TIMEDOUT) {
			/*
			 * This SCB has been here before and is not the
			 * recovery SCB. Cut our losses and panic.  Its
			 * better to do this than trash a filesystem.
			 */
			panic("%s: Timed-out command times out "
				"again\n", ahc_name(ahc));
		}
		else if (!(scb->flags & SCB_ABORTED))
		{
			/*
			 * This is not the SCB that started this timeout
			 * processing.  Give this scb another lifetime so
			 * that it can continue once we deal with the
			 * timeout.
			 */
			scb->flags |= SCB_TIMEDOUT;
			timeout(ahc_timeout, (caddr_t)scb, 
				(scb->xs->timeout * hz) / 1000);
			splx(s);
			return;
		}
	}
	ahc->in_timeout = TRUE;

	/*      
	 * Ensure that the card doesn't do anything
	 * behind our back.
	 */
	pause_sequencer(ahc);

	scsipi_printaddr(scb->xs->xs_periph);
	printf("timed out ");
	/*
	 * Take a snapshot of the bus state and print out
	 * some information so we can track down driver bugs.
	 */
	bus_state = AHC_INB(ahc, LASTPHASE);

	switch(bus_state & PHASE_MASK)
	{
		case P_DATAOUT:
			printf("in dataout phase");
			break;
		case P_DATAIN:
			printf("in datain phase");
			break;
		case P_COMMAND:
			printf("in command phase");
			break;
		case P_MESGOUT:
			printf("in message out phase");
			break;
		case P_STATUS:
			printf("in status phase");
			break;
		case P_MESGIN:
			printf("in message in phase");
			break;
		default:
			printf("while idle, LASTPHASE == 0x%x",
				bus_state);
			/* 
			 * We aren't in a valid phase, so assume we're
			 * idle.
			 */
			bus_state = 0;
			break;
	}

	printf(", SCSISIGI == 0x%x\n", AHC_INB(ahc, SCSISIGI));

	/* Decide our course of action */

	if(scb->flags & SCB_ABORTED)
	{
		/*
		 * Been down this road before.
		 * Do a full bus reset.
		 */
		char channel = (scb->tcl & SELBUSB)
			   ? 'B': 'A';	
		found = ahc_reset_channel(ahc, channel, scb->tag,
					  XS_TIMEOUT, /*Initiate Reset*/TRUE);
		printf("%s: Issued Channel %c Bus Reset #1. "
		       "%d SCBs aborted\n", ahc_name(ahc), channel, found);
		ahc->in_timeout = FALSE;
	}
	else if(scb->control & TAG_ENB) {
		/*
		 * We could be starving this command
		 * try sending an ordered tag command
		 * to the target we come from.
		 *
		 * XXX This doesn't actually do anything right now.
		 * XXX --thorpej
		 */
		scb->flags |= SCB_ABORTED|SCB_SENTORDEREDTAG;
		ahc->orderedtag |= 0xFF;
		timeout(ahc_timeout, (caddr_t)scb, (5 * hz));
		unpause_sequencer(ahc, /*unpause_always*/FALSE);
		printf("Ordered Tag queued\n");
		goto done;
	}
	else {
		/*
		 * Send a Bus Device Reset Message:
		 * The target that is holding up the bus may not
		 * be the same as the one that triggered this timeout
		 * (different commands have different timeout lengths).
		 * It is also impossible to get a message to a target
		 * if we are in a "frozen" data transfer phase.  Our
		 * strategy here is to queue a bus device reset message
		 * to the timed out target if it is disconnected.
		 * Otherwise, if we have an active target we stuff the
		 * message buffer with a bus device reset message and
		 * assert ATN in the hopes that the target will let go
		 * of the bus and finally disconnect.  If this fails,
		 * we'll get another timeout 2 seconds later which will
		 * cause a bus reset.
		 *
		 * XXX If the SCB is paged out, we simply reset the
		 *     bus.  We should probably queue a new command
		 *     instead.
		 */

		/* Test to see if scb is disconnected */
		if( !(scb->flags & SCB_PAGED_OUT ) ){
			u_char active_scb;
			struct scb *active_scbp;

			active_scb = AHC_INB(ahc, SCBPTR);
			active_scbp = ahc->scbarray[AHC_INB(ahc, SCB_TAG)];
			AHC_OUTB(ahc, SCBPTR, scb->position);

			if(AHC_INB(ahc, SCB_CONTROL) & DISCONNECTED) {
				if(ahc->flags & AHC_PAGESCBS) {
					/*
					 * Pull this SCB out of the 
					 * disconnected list.
					 */
					u_char prev = AHC_INB(ahc, SCB_PREV);
					u_char next = AHC_INB(ahc, SCB_NEXT);
					if(prev == SCB_LIST_NULL) {
						/* At the head */
						AHC_OUTB(ahc, DISCONNECTED_SCBH,
						     next );
					}
					else {
						AHC_OUTB(ahc, SCBPTR, prev);
						AHC_OUTB(ahc, SCB_NEXT, next);
						if(next != SCB_LIST_NULL) {
							AHC_OUTB(ahc, SCBPTR,
							     next);
							AHC_OUTB(ahc, SCB_PREV,
							     prev);
						}
						AHC_OUTB(ahc, SCBPTR,
						     scb->position);
					}
				}
				scb->flags |= SCB_DEVICE_RESET|SCB_ABORTED;
				scb->control &= DISCENB;
				scb->control |= MK_MESSAGE;
				scb->cmdlen = 0;
				scb->SG_segment_count = 0;
				scb->SG_list_pointer = 0;
				scb->data = 0;
				scb->datalen = 0;
				ahc_send_scb(ahc, scb);
				ahc_add_waiting_scb(ahc, scb);
				timeout(ahc_timeout, (caddr_t)scb, (2 * hz));
				scsipi_printaddr(scb->xs->xs_periph);
				printf("BUS DEVICE RESET message queued.\n");
				AHC_OUTB(ahc, SCBPTR, active_scb);
				unpause_sequencer(ahc, /*unpause_always*/FALSE);
				goto done;
			}
			/* Is the active SCB really active? */
			else if((active_scbp->flags & SCB_ACTIVE) && bus_state){
				AHC_OUTB(ahc, MSG_LEN, 1);
				AHC_OUTB(ahc, MSG0, MSG_BUS_DEV_RESET);
				AHC_OUTB(ahc, SCSISIGO, bus_state|ATNO);
				scsipi_printaddr(active_scbp->xs->xs_periph);
				printf("asserted ATN - device reset in "
				       "message buffer\n");
				active_scbp->flags |=   SCB_DEVICE_RESET
						      | SCB_ABORTED;
				if(active_scbp != scb) {
					untimeout(ahc_timeout, 
						  (caddr_t)active_scbp);
					/* Give scb a new lease on life */
					timeout(ahc_timeout, (caddr_t)scb, 
						(scb->xs->timeout * hz) / 1000);
				}
				timeout(ahc_timeout, (caddr_t)active_scbp, 
					(2 * hz));
				AHC_OUTB(ahc, SCBPTR, active_scb);
				unpause_sequencer(ahc, /*unpause_always*/FALSE);
				goto done;
			}
		}
		/*
		 * No active target or a paged out SCB.
		 * Try resetting the bus.
		 */
		channel = (scb->tcl & SELBUSB) ? 'B': 'A';	
		found = ahc_reset_channel(ahc, channel, scb->tag, 
					  XS_TIMEOUT,
					  /*Initiate Reset*/TRUE);
		printf("%s: Issued Channel %c Bus Reset #2. "
			"%d SCBs aborted\n", ahc_name(ahc), channel,
			found);
		ahc->in_timeout = FALSE;
	}
done:
	splx(s);
}


/*
 * The device at the given target/channel has been reset.  Abort 
 * all active and queued scbs for that target/channel. 
 */
static int
ahc_reset_device(ahc, target, channel, timedout_scb, xs_error)
	struct ahc_data *ahc;
	int target;
	char channel;
	u_char timedout_scb;
	u_int32_t xs_error;
{
        struct scb *scbp;
	u_char active_scb;
	int i = 0;
	int found = 0;

	/* restore this when we're done */
	active_scb = AHC_INB(ahc, SCBPTR);

	/*
	 * Search the QINFIFO.
	 */
	{
		u_char saved_queue[AHC_SCB_MAX];
		u_char queued = AHC_INB(ahc, QINCNT) & ahc->qcntmask;

		for (i = 0; i < (queued - found); i++) {
			saved_queue[i] = AHC_INB(ahc, QINFIFO);
			AHC_OUTB(ahc, SCBPTR, saved_queue[i]);
			scbp = ahc->scbarray[AHC_INB(ahc, SCB_TAG)];
			if (ahc_match_scb (scbp, target, channel)){
				/*
				 * We found an scb that needs to be aborted.
				 */
				scbp->flags = SCB_ABORTED|SCB_QUEUED_FOR_DONE;
				scbp->xs->error |= xs_error;
				if(scbp->position != timedout_scb)
					untimeout(ahc_timeout, (caddr_t)scbp);
				AHC_OUTB(ahc, SCB_CONTROL, 0);
				i--;
				found++;
			}
		}
		/* Now put the saved scbs back. */
		for (queued = 0; queued < i; queued++) {
			AHC_OUTB(ahc, QINFIFO, saved_queue[queued]);
		}
	}

	/*
	 * Search waiting for selection list.
	 */
	{
		u_char next, prev;

		next = AHC_INB(ahc, WAITING_SCBH);  /* Start at head of list. */
		prev = SCB_LIST_NULL;

		while (next != SCB_LIST_NULL) {
			AHC_OUTB(ahc, SCBPTR, next);
			scbp = ahc->scbarray[AHC_INB(ahc, SCB_TAG)];
			/*
			 * Select the SCB.
			 */
			if (ahc_match_scb(scbp, target, channel)) {
				next = ahc_abort_wscb(ahc, scbp, prev,
						timedout_scb, xs_error);
				found++;
			}
			else {
				prev = next;
				next = AHC_INB(ahc, SCB_NEXT);
			}
		}
	}
	/*
	 * Go through the entire SCB array now and look for 
	 * commands for this target that are active.  These
	 * are other (most likely tagged) commands that 
	 * were disconnected when the reset occured.
	 */
	for(i = 0; i < ahc->numscbs; i++) {
		scbp = ahc->scbarray[i];
		if((scbp->flags & SCB_ACTIVE)
		  && ahc_match_scb(scbp, target, channel)) {
			/* Ensure the target is "free" */
			ahc_unbusy_target(ahc, target, channel);
			if( !(scbp->flags & SCB_PAGED_OUT) )
			{
				AHC_OUTB(ahc, SCBPTR, scbp->position);
				AHC_OUTB(ahc, SCB_CONTROL, 0);
			}
			scbp->flags = SCB_ABORTED|SCB_QUEUED_FOR_DONE;
			scbp->xs->error |= xs_error;
			if(scbp->tag != timedout_scb)
				untimeout(ahc_timeout, (caddr_t)scbp);
			found++;
		}
	}			
	AHC_OUTB(ahc, SCBPTR, active_scb);
	return found;
}

/*
 * Manipulate the waiting for selection list and return the
 * scb that follows the one that we remove.
 */
static u_char
ahc_abort_wscb (ahc, scbp, prev, timedout_scb, xs_error)
	struct ahc_data *ahc;
        struct scb *scbp;
	u_char prev;
	u_char timedout_scb;
	u_int32_t xs_error;
{       
	u_char curscbp, next;
	int target = ((scbp->tcl >> 4) & 0x0f);
	char channel = (scbp->tcl & SELBUSB) ? 'B' : 'A';
	/*
	 * Select the SCB we want to abort and
	 * pull the next pointer out of it.
	 */
	curscbp = AHC_INB(ahc, SCBPTR);
	AHC_OUTB(ahc, SCBPTR, scbp->position);
	next = AHC_INB(ahc, SCB_NEXT);

	/* Clear the necessary fields */
	AHC_OUTB(ahc, SCB_CONTROL, 0);
	AHC_OUTB(ahc, SCB_NEXT, SCB_LIST_NULL);
	ahc_unbusy_target(ahc, target, channel);

	/* update the waiting list */
	if( prev == SCB_LIST_NULL ) 
		/* First in the list */
		AHC_OUTB(ahc, WAITING_SCBH, next); 
	else {
		/*
		 * Select the scb that pointed to us 
		 * and update its next pointer.
		 */
		AHC_OUTB(ahc, SCBPTR, prev);
		AHC_OUTB(ahc, SCB_NEXT, next);
	}
	/*
	 * Point us back at the original scb position
	 * and inform the SCSI system that the command
	 * has been aborted.
	 */
	AHC_OUTB(ahc, SCBPTR, curscbp);
	scbp->flags = SCB_ABORTED|SCB_QUEUED_FOR_DONE;
	scbp->xs->error |= xs_error;
	if(scbp->tag != timedout_scb)
		untimeout(ahc_timeout, (caddr_t)scbp);
	return next;
}

static void
ahc_busy_target(ahc, target, channel)
	struct ahc_data *ahc;
	u_char target;
	char   channel;
{
	u_char active;
	u_long active_port = ACTIVE_A;

	if(target > 0x07 || channel == 'B') {
		/* 
		 * targets on the Second channel or
		 * above id 7 store info in byte two 
		 * of HA_ACTIVE
		 */
		active_port++;
	}
	active = AHC_INB(ahc, active_port);
	active |= (0x01 << (target & 0x07));
	AHC_OUTB(ahc, active_port, active);
}

static void
ahc_unbusy_target(ahc, target, channel)
	struct ahc_data *ahc;
	u_char target;
	char   channel;
{
	u_char active;
	u_long active_port = ACTIVE_A;

	if(target > 0x07 || channel == 'B') {
		/* 
		 * targets on the Second channel or
		 * above id 7 store info in byte two 
		 * of HA_ACTIVE
		 */
		active_port++;
	}
	active = AHC_INB(ahc, active_port);
	active &= ~(0x01 << (target & 0x07));
	AHC_OUTB(ahc, active_port, active);
}

static void
ahc_reset_current_bus(ahc)
	struct ahc_data *ahc;
{
	AHC_OUTB(ahc, SCSISEQ, SCSIRSTO);
	DELAY(1000);
	AHC_OUTB(ahc, SCSISEQ, 0);
}

static int
ahc_reset_channel(ahc, channel, timedout_scb, xs_error, initiate_reset)
	struct ahc_data *ahc;
	char   channel;
	u_char timedout_scb;
	u_int32_t xs_error;
	u_char initiate_reset;
{
	u_char sblkctl;
	char cur_channel;
	u_long offset, offset_max;
	int found;

	/*
	 * Clean up all the state information for the
	 * pending transactions on this bus.
	 */
	found = ahc_reset_device(ahc, ALL_TARGETS, channel, 
				 timedout_scb, xs_error);
	if(channel == 'B'){
		ahc->needsdtr |= (ahc->needsdtr_orig & 0xff00);
		ahc->sdtrpending &= 0x00ff;
		AHC_OUTB(ahc, ACTIVE_B, 0);
		offset = TARG_SCRATCH + 8;
		offset_max = TARG_SCRATCH + 16;
	}
	else if (ahc->type & AHC_WIDE){
		ahc->needsdtr = ahc->needsdtr_orig;
		ahc->needwdtr = ahc->needwdtr_orig;
		ahc->sdtrpending = 0;
		ahc->wdtrpending = 0;
		AHC_OUTB(ahc, ACTIVE_A, 0);
		AHC_OUTB(ahc, ACTIVE_B, 0);
		offset = TARG_SCRATCH;
		offset_max = TARG_SCRATCH + 16;
	}
	else{
		ahc->needsdtr |= (ahc->needsdtr_orig & 0x00ff);
		ahc->sdtrpending &= 0xff00;
		AHC_OUTB(ahc, ACTIVE_A, 0);
		offset = TARG_SCRATCH;
		offset_max = TARG_SCRATCH + 8;
	}
	for(;offset < offset_max;offset++) {
		/*
		 * Revert to async/narrow transfers
		 * until we renegotiate.
		 */
		u_char targ_scratch;

		targ_scratch = AHC_INB(ahc, offset);
		targ_scratch &= SXFR;
		AHC_OUTB(ahc, offset, targ_scratch);
	}

	/*
	 * Reset the bus if we are initiating this reset and
	 * restart/unpause the sequencer
	 */
	/* Case 1: Command for another bus is active */
	sblkctl = AHC_INB(ahc, SBLKCTL);
	cur_channel = (sblkctl & SELBUSB) ? 'B' : 'A';
	if(cur_channel != channel)
	{
		/*
		 * Stealthily reset the other bus
		 * without upsetting the current bus
		 */
		AHC_OUTB(ahc, SBLKCTL, sblkctl ^ SELBUSB);
		if( initiate_reset )
		{
			ahc_reset_current_bus(ahc);
		}
		AHC_OUTB(ahc, CLRSINT1, CLRSCSIRSTI|CLRSELTIMEO);
		AHC_OUTB(ahc, CLRINT, CLRSCSIINT);
		AHC_OUTB(ahc, SBLKCTL, sblkctl);
		unpause_sequencer(ahc, /*unpause_always*/TRUE);
	}
	/* Case 2: A command from this bus is active or we're idle */ 
	else {
		if( initiate_reset )
		{
			ahc_reset_current_bus(ahc);
		}
		AHC_OUTB(ahc, CLRSINT1, CLRSCSIRSTI|CLRSELTIMEO);
		AHC_OUTB(ahc, CLRINT, CLRSCSIINT);
		restart_sequencer(ahc);
	}
	ahc_run_done_queue(ahc);
	return found;
}

void
ahc_run_done_queue(ahc)
	struct ahc_data *ahc;
{
	int i;
	struct scb *scbp;
	
	for(i = 0; i < ahc->numscbs; i++) {
		scbp = ahc->scbarray[i];
		if(scbp->flags & SCB_QUEUED_FOR_DONE) 
			ahc_done(ahc, scbp);
	}
}
	
static int
ahc_match_scb (scb, target, channel)
        struct scb *scb;
        int target;
	char channel;
{
	int targ = (scb->tcl >> 4) & 0x0f;
	char chan = (scb->tcl & SELBUSB) ? 'B' : 'A';

	if (target == ALL_TARGETS) 
		return (chan == channel);
	else
		return ((chan == channel) && (targ == target));
}


static void
ahc_construct_sdtr(ahc, start_byte, period, offset)
	struct ahc_data *ahc;
	int start_byte;
	u_int8_t period;
	u_int8_t offset;
{
	AHC_OUTB(ahc, MSG0 + start_byte, MSG_EXTENDED);
	AHC_OUTB(ahc, MSG1 + start_byte, MSG_EXT_SDTR_LEN);
	AHC_OUTB(ahc, MSG2 + start_byte, MSG_EXT_SDTR);
	AHC_OUTB(ahc, MSG3 + start_byte, period);
	AHC_OUTB(ahc, MSG4 + start_byte, offset);
	AHC_OUTB(ahc, MSG_LEN, start_byte + 5);
}

static void
ahc_construct_wdtr(ahc, start_byte, bus_width)
	struct ahc_data *ahc;
	int start_byte;
	u_int8_t bus_width;
{
	AHC_OUTB(ahc, MSG0 + start_byte, MSG_EXTENDED);
	AHC_OUTB(ahc, MSG1 + start_byte, MSG_EXT_WDTR_LEN);
	AHC_OUTB(ahc, MSG2 + start_byte, MSG_EXT_WDTR);
	AHC_OUTB(ahc, MSG3 + start_byte, bus_width);
	AHC_OUTB(ahc, MSG_LEN, start_byte + 4);
}
