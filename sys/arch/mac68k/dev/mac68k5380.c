/*	$NetBSD: mac68k5380.c,v 1.1 1995/09/01 03:43:49 briggs Exp $	*/

/*
 * Copyright (c) 1995 Allen Briggs
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Allen Briggs
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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
 *
 * Derived from atari5380.c for the mac68k port of NetBSD.
 *
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/syslog.h>
#include <sys/buf.h>
#include <scsi/scsi_all.h>
#include <scsi/scsi_message.h>
#include <scsi/scsiconf.h>

/*
 * Include the driver definitions
 */
#include <atari/dev/ncr5380reg.h>

#include <machine/stdarg.h>

#include "../mac68k/via.h"

#undef splbio()
#define splbio()	splhigh()

/*
 * Set the various driver options
 */
#define	NREQ		18	/* Size of issue queue			*/
#define	AUTO_SENSE	1	/* Automatically issue a request-sense 	*/

#define	DRNAME		ncrscsi	/* used in various prints	*/
#undef	DBG_SEL			/* Show the selection process		*/
#undef	DBG_REQ			/* Show enqueued/ready requests		*/
#undef	DBG_NOWRITE		/* Do not allow writes to the targets	*/
#undef	DBG_PIO			/* Show the polled-I/O process		*/
#undef	DBG_INF			/* Show information transfer process	*/
#define	DBG_NOSTATIC		/* No static functions, all in DDB trace*/
#define	DBG_PID			/* Keep track of driver			*/
#undef 	REAL_DMA		/* Use DMA if sensible			*/
#define fair_to_keep_dma()	1
#define claimed_dma()		1
#define reconsider_dma()
#define	USE_PDMA	1	/* Use special pdma-transfer function	*/

#define	ENABLE_NCR5380(sc)	cur_softc = sc;

/*
 * softc of currently active controller (well, we only have one for now).
 */

static struct ncr_softc	*cur_softc;

struct scsi_5380 {
	volatile u_char	scsi_5380[8*16]; /* 8 regs, 1 every 16th byte. */
};

extern vm_offset_t	SCSIBase;
static volatile u_char	*ncr		= (volatile u_char *) 0x10000;
static volatile u_char	*ncr_5380_with_drq	= (volatile u_char *)  0x6000;
static volatile u_char	*ncr_5380_without_drq	= (volatile u_char *) 0x12000;

#define SCSI_5380		((struct scsi_5380 *) ncr)
#define GET_5380_REG(rnum)	SCSI_5380->scsi_5380[((rnum)<<4)]
#define SET_5380_REG(rnum,val)	(SCSI_5380->scsi_5380[((rnum)<<4)] = (val))

static __inline__ void
scsi_clr_ipend()
{
	int	tmp;

	tmp = GET_5380_REG(NCR5380_IRCV);
}

extern __inline__ void
scsi_ienable()
{
	int	s;

	s = splhigh();
	if (VIA2 == VIA2OFF)
		via_reg(VIA2, vIER) = 0x80 | (V2IF_SCSIIRQ | V2IF_SCSIDRQ);
	else
		via_reg(VIA2, rIER) = 0x80 | (V2IF_SCSIIRQ | V2IF_SCSIDRQ);
	splx(s);
}

extern __inline__ void
scsi_idisable()
{
	int	s;

	s = splhigh();
	if (VIA2 == VIA2OFF)
		via_reg(VIA2, vIER) = (V2IF_SCSIIRQ | V2IF_SCSIDRQ);
	else
		via_reg(VIA2, rIER) = (V2IF_SCSIIRQ | V2IF_SCSIDRQ);
	splx(s);
}

static void
scsi_mach_init(sc)
	struct ncr_softc	*sc;
{
	static int	initted = 0;

	if (initted++)
		panic("scsi_mach_init called again.\n");

	ncr		= (volatile u_char *)
			  (SCSIBase + (u_long) ncr);
	ncr_5380_with_drq	= (volatile u_char *)
			  (SCSIBase + (u_int) ncr_5380_with_drq);
	ncr_5380_without_drq	= (volatile u_char *)
			  (SCSIBase + (u_int) ncr_5380_without_drq);
}

static int
machine_match(pdp, cdp, auxp, cd)
	struct device	*pdp;
	struct cfdata	*cdp;
	void		*auxp;
	struct cfdriver	*cd;
{
	if (matchbyname(pdp, cdp, auxp) == 0)
		return 0;
	if (!mac68k_machine.scsi80)
		return 0;
	if (cdp->cf_unit != 0)
		return 0;
	return 1;
}

#if USE_PDMA
int		pdma_5380_dir = 0;
int		(*pdma_xfer_fun)(void) = NULL;
int		pdma_5380_sleeping = 0;

volatile int	pdma_5380_pending = 0;
volatile u_char	*pending_5380_data;
volatile u_long	pending_5380_count;

#define DEBUG 1	/* Maybe we try with this off eventually. */
#if DEBUG
int		pdma_5380_sends = 0;

volatile char	*pdma_5380_state="";
void
pdma_stat()
{
	printf("PDMA SCSI: %d xfers completed, pending = %d.\n",
		pdma_5380_sends, pdma_5380_pending);
	printf("xfer fun = 0x%x, pdma_5380_dir = %d.\n",
		pdma_xfer_fun, pdma_5380_dir);
	printf("datap = 0x%x, remainder = %d.\n",
		pending_5380_data, pending_5380_count);
	printf("pdma_5380_state = %s.\n", pdma_5380_state);
}
#endif
#endif

void
ncr5380_irq_intr(void)
{
	if (pdma_xfer_fun) {
#if DEBUG
		pdma_5380_state = "got irq interrupt in xfer.";
#endif
		/*
		 * If Mr. IRQ isn't set one might wonder how we got
		 * here.  It does happen, though.
		 */
		if (!(GET_5380_REG(NCR5380_DMSTAT) & SC_IRQ_SET)) {
			return;
		}
		/*
		 * For a phase mis-match, ATN is a "don't care," IRQ is 1 and
		 * all other bits in the Bus & Status Register are 0.  Also,
		 * the current SCSI Bus Status Register has a 1 for BSY and
		 * REQ.  Since we're just checking that this interrupt isn't a
		 * reselection or a reset, we just check for either.
		 */
		if (   ((GET_5380_REG(NCR5380_DMSTAT) & (0xff & ~SC_ATN_STAT))
			== SC_IRQ_SET)
		    && (GET_5380_REG(NCR5380_IDSTAT) & (SC_S_BSY|SC_S_REQ))) {
			scsi_idisable();
			scsi_clr_ipend();
			pdma_5380_pending = 0;
			if (pdma_5380_sleeping) wakeup(pdma_xfer_fun);
			return;
		}
		scsi_show();
		panic("Spurious interrupt during PDMA xfer.\n");
	}
	if (GET_5380_REG(NCR5380_DMSTAT) & SC_IRQ_SET) {
		scsi_idisable();
		ncr_ctrl_intr(cur_softc);
	}
}

void
ncr5380_drq_intr(void)
{
#if USE_PDMA
	if (pdma_xfer_fun) {
#if DEBUG
		pdma_5380_state = "got drq interrupt.";
#endif
		if (pdma_xfer_fun())
#if DEBUG
		pdma_5380_state = "handled drq interrupt.";
#endif
	}
#endif
}

#if USE_PDMA
static int
pdma_xfer_in()
{
	/*
	 * Can we "unroll" this any?  I don't think so--in fact, I
	 * question the safety of using long word transfers.  The
	 * device could theoretically disconnect at any time.
	 * The long word xfer is controlled by the Mac's circuitry,
	 * and we can't know how much it transferred if the device
	 * decides to disconnect on us.
	 * If it does disconnect in the middle of a long xfer, it
	 * should get a bus error--lovely.
	 */
	while (   (GET_5380_REG(NCR5380_DMSTAT) & SC_DMA_REQ)
	       && (GET_5380_REG(NCR5380_DMSTAT) & SC_DMA_REQ)
	       && (GET_5380_REG(NCR5380_DMSTAT) & SC_DMA_REQ) )
		if (pending_5380_count) {
			*((u_char *) pending_5380_data)++ = *ncr_5380_with_drq;
			pending_5380_count --;
		} else {
#if DEBUG
			pdma_5380_state = "done in xfer in.";
#endif
			SET_5380_REG(NCR5380_MODE,
				     GET_5380_REG(NCR5380_MODE) & ~SC_M_DMA);
			return 0;
		}
	return 1;
}

/*
 * Macroed for readability.
 */
#define DONE   (   (GET_5380_REG(NCR5380_DMSTAT) & SC_ACK_STAT) \
		|| (GET_5380_REG(NCR5380_IDSTAT) &    SC_S_REQ) )

static int
pdma_xfer_out()
{
	/*
	 * See comment on pdma_xfer_in(), above.
	 */
	while (   (GET_5380_REG(NCR5380_DMSTAT) & SC_DMA_REQ)
	       && (GET_5380_REG(NCR5380_DMSTAT) & SC_DMA_REQ)
	       && (GET_5380_REG(NCR5380_DMSTAT) & SC_DMA_REQ) )
		if (pending_5380_count) {
			*ncr_5380_with_drq = *((u_char *) pending_5380_data)++;
			pending_5380_count --;
		} else {
			scsi_idisable();
#if DEBUG
			pdma_5380_state = "done in xfer out--waiting.";
#endif
			while (!DONE);
#if DEBUG
			pdma_5380_state = "done in xfer out--really done.";
#endif
			pdma_5380_pending = 0;
			if (pdma_5380_sleeping) wakeup(pdma_xfer_fun);
			return 0;
		}
	return 1;
}

#define SCSI_TIMEOUT_VAL	10000000

static int
transfer_pdma(phasep, data, count)
	u_char	*phasep;
	u_char	*data;
	u_long	*count;
{
	SC_REQ	*reqp = connected;
	int	len = *count, i, scsi_timeout = SCSI_TIMEOUT_VAL;
	int	s, err;

	if (pdma_5380_pending) {
		panic("ncrscsi: transfer_pdma called when operation already "
			"pending.\n");
	}
#if DEBUG
	pdma_5380_state = "in transfer_pdma.";
#endif

	scsi_idisable();

	if (*count < 128) {	/* Don't bother with PDMA for short xfers. */
#if DEBUG
		pdma_5380_state = "using transfer_pio.";
#endif
		return transfer_pio(phasep, data, count);
	}

	switch (*phasep) {
	default:
		panic("Unexpected phase in transfer_pdma.\n");
	case PH_DATAOUT:
		pdma_5380_dir = 1;
		break;
	case PH_DATAIN:
		pdma_5380_dir = 2;
		break;
	}

	/*
	 * Match phases with target.
	 */
	SET_5380_REG(NCR5380_TCOM, *phasep);
	scsi_clr_ipend();

	/*
	 * Wait until target asserts BSY.
	 */
	while ( ((GET_5380_REG(NCR5380_IDSTAT) & SC_S_BSY) == 0) &&
		((GET_5380_REG(NCR5380_IDSTAT) & SC_S_BSY) == 0) &&
		((GET_5380_REG(NCR5380_IDSTAT) & SC_S_BSY) == 0) &&
		 (--scsi_timeout) );
	if (!scsi_timeout) {
#if DIAGNOSTIC
		printf("scsi timeout: waiting for BSY in %s.\n",
			(pdma_5380_dir == 1) ? "pdma_out" : "pdma_in");
#endif
		goto scsi_timeout_error;
	}

	/*
	 * Set DMA mode and assert data bus.
	 */
	SET_5380_REG(NCR5380_MODE, GET_5380_REG(NCR5380_MODE) | SC_M_DMA);
	SET_5380_REG(NCR5380_ICOM, GET_5380_REG(NCR5380_ICOM) | SC_ADTB);

	/*
	 * Load static/volatile values for DRQ interrupt handlers.
	 */
	pending_5380_data = (volatile u_char *) data;
	pending_5380_count = len;

#if DEBUG
	pdma_5380_state = "wait for interrupt.";
#endif

	/*
	 * Set the transfer function to be called on DRQ interrupts.
	 */
	pdma_xfer_fun = (pdma_5380_dir == 1) ? pdma_xfer_out : pdma_xfer_in;

	/*
	 * Initiate the DMA transaction--sending or receiving.
	 */
	if (pdma_5380_dir == 1) {
		SET_5380_REG(NCR5380_DMSTAT, 0);
	} else {
		SET_5380_REG(NCR5380_IRCV, 0);
	}

	pdma_5380_pending = 1;

	/*
	 * Now that we're set up, enable interrupts and drop processor
	 * priority.  We've been running at spl2 because that's what the
	 * VIA2/RBV interrupts are on and we're responding to a selection
	 * interrupt, here.
	 */
	scsi_ienable();
	s = spl1();	/* Allow lev 2 and above interrupts--we */
			/* were probably called from spl2(). */
	if (reqp->xs->flags & SCSI_NOSLEEP) {
		while (pdma_5380_pending);
	} else {
		pdma_5380_sleeping = 1;
		if ((err = tsleep(pdma_xfer_fun, PRIBIO,
			(pdma_5380_dir == 1) ? "pdma_out" : "pdma_in", 0))) {
			log(LOG_ERR, "pdma_xfer--error %d.\n", err);
		}
		pdma_5380_sleeping = 0;
	}
	splx(s);
	scsi_idisable();
	pdma_xfer_fun = NULL;

#if DEBUG
	pdma_5380_state = "back from xfer.";
	pdma_5380_sends++;
#endif

	/*
	 * Send remainder back to m.i. code.
	 */
	*count = pending_5380_count;

scsi_timeout_error:
	/*
	 * Clear the DMA mode.
	 */
	SET_5380_REG(NCR5380_MODE, GET_5380_REG(NCR5380_MODE) & ~SC_M_DMA);
	return -1;
}
#endif /* if USE_PDMA */

/* Include general routines. */
#include <atari/dev/ncr5380.c>
