/* $NetBSD: sbic.c,v 1.1.4.2 2002/02/28 04:05:58 nathanw Exp $ */

/*
 * Copyright (c) 2001 Richard Earnshaw
 * All rights reserved.
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * Copyright (c) 1994 Christian E. Hopps
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
 *	from: sbic.c,v 1.21 1996/01/07 22:01:54
 */

/*
 * WD 33C93 scsi adaptor driver
 */

#if 0
/*
 * The UPROTECTED_CSR code is bogus.  It can read the csr (SCSI Status 
 * register) at times when an interrupt may be pending.  Doing this will
 * clear the interrupt, so we won't see it at times when we really need
 * to.
 */
#define UNPROTECTED_CSR
#endif

#define DEBUG
/* #define SBIC_DEBUG(a) a */

#include "opt_ddb.h"

#include <sys/param.h>

__RCSID("$NetBSD: sbic.c,v 1.1.4.2 2002/02/28 04:05:58 nathanw Exp $");

#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/kernel.h> /* For hz */
#include <sys/device.h>
#include <sys/buf.h>

#include <uvm/uvm_extern.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#include <acorn32/podulebus/sbicreg.h>
#include <acorn32/podulebus/sbicvar.h>

/*
 * SCSI delays
 * In u-seconds, primarily for state changes on the SPC.
 */
#define	SBIC_CMD_WAIT	50000	/* wait per step of 'immediate' cmds */
#define	SBIC_DATA_WAIT	50000	/* wait per data in/out step */
#define	SBIC_INIT_WAIT	50000	/* wait per step (both) during init */

#define SBIC_WAIT(regs, until, timeo) sbicwait(regs, until, timeo, __LINE__)

static int  sbicicmd		(struct sbic_softc *, int, int,
				 struct sbic_acb *);
static int  sbicgo		(struct sbic_softc *, struct scsipi_xfer *);
static int  sbicwait		(sbic_regmap_p, char, int , int);
static int  sbicselectbus	(struct sbic_softc *, sbic_regmap_p, u_char,
				 u_char, u_char);
static int  sbicxfstart		(sbic_regmap_p, int, u_char, int);
static int  sbicxfout		(sbic_regmap_p regs, int, void *, int);
static int  sbicfromscsiperiod	(struct sbic_softc *, sbic_regmap_p, int);
static int  sbictoscsiperiod	(struct sbic_softc *, sbic_regmap_p, int);
static int  sbicpoll		(struct sbic_softc *);
static int  sbicnextstate	(struct sbic_softc *, u_char, u_char);
static int  sbicmsgin		(struct sbic_softc *);
static int  sbicxfin		(sbic_regmap_p regs, int, void *);
static int  sbicabort		(struct sbic_softc *, sbic_regmap_p, char *);
static void sbicxfdone		(struct sbic_softc *, sbic_regmap_p, int);
static void sbicerror		(struct sbic_softc *, sbic_regmap_p, u_char);
static void sbicreset		(struct sbic_softc *);
static void sbic_scsidone	(struct sbic_acb *, int);
static void sbic_sched		(struct sbic_softc *);
static void sbic_save_ptrs	(struct sbic_softc *, sbic_regmap_p);

/*
 * Synch xfer parameters, and timing conversions
 */
int sbic_min_period = SBIC_SYN_MIN_PERIOD;  /* in cycles = f(ICLK,FSn) */
int sbic_max_offset = SBIC_SYN_MAX_OFFSET;  /* pure number */

int sbic_cmd_wait = SBIC_CMD_WAIT;
int sbic_data_wait = SBIC_DATA_WAIT;
int sbic_init_wait = SBIC_INIT_WAIT;

/*
 * was broken before.. now if you want this you get it for all drives
 * on sbic controllers.
 */
u_char sbic_inhibit_sync[8];
int sbic_enable_reselect = 1;
int sbic_clock_override = 0;
int sbic_no_dma = 1;	/* was 0 */
int sbic_parallel_operations = 1;

#ifdef DEBUG
sbic_regmap_p debug_sbic_regs;
int	sbicdma_ops = 0;	/* total DMA operations */
int     sbicdma_saves = 0;
#define QPRINTF(a)	if (sbic_debug > 1) printf a
#define DBGPRINTF(x,p)	if (p) printf x
#define DBG(x)		x
int	sbic_debug = 0;
int	sync_debug = 0;
int	sbic_dma_debug = 0;
int	reselect_debug = 0;
int	data_pointer_debug = 0;
u_char	debug_asr, debug_csr, routine;

void sbicdumpstate	(void);
void sbictimeout	(struct sbic_softc *);
void sbic_dump		(struct sbic_softc *);
void sbic_dump_acb	(struct sbic_acb *);

#define CSR_TRACE_SIZE 32
#if CSR_TRACE_SIZE
#define CSR_TRACE(w,c,a,x) do { \
	int s = splbio(); \
	csr_trace[csr_traceptr].whr = (w); csr_trace[csr_traceptr].csr = (c); \
	csr_trace[csr_traceptr].asr = (a); csr_trace[csr_traceptr].xtn = (x); \
	csr_traceptr = (csr_traceptr + 1) & (CSR_TRACE_SIZE - 1); \
	splx(s); \
} while (0)
int csr_traceptr;
int csr_tracesize = CSR_TRACE_SIZE;
struct {
	u_char whr;
	u_char csr;
	u_char asr;
	u_char xtn;
} csr_trace[CSR_TRACE_SIZE];
#else
#define CSR_TRACE
#endif

#define SBIC_TRACE_SIZE 0
#if SBIC_TRACE_SIZE
#define SBIC_TRACE(dev) do { \
	int s = splbio(); \
	sbic_trace[sbic_traceptr].sp = &s; \
	sbic_trace[sbic_traceptr].line = __LINE__; \
	sbic_trace[sbic_traceptr].sr = s; \
	sbic_trace[sbic_traceptr].csr = csr_traceptr; \
	sbic_traceptr = (sbic_traceptr + 1) & (SBIC_TRACE_SIZE - 1); \
	splx(s); \
} while (0)
int sbic_traceptr;
int sbic_tracesize = SBIC_TRACE_SIZE;
struct {
	void *sp;
	u_short line;
	u_short sr;
	int csr;
} sbic_trace[SBIC_TRACE_SIZE];
#else
#define SBIC_TRACE(dev)
#endif

#else
#define QPRINTF(a)
#define DBGPRINTF(x,p)
#define DBG(x)
#define CSR_TRACE
#define SBIC_TRACE
#endif

#ifndef SBIC_DEBUG
#define SBIC_DEBUG(x)
#endif

/*
 * default minphys routine for sbic based controllers
 */
void
sbic_minphys(struct buf *bp)
{
	/*
	 * No max transfer at this level.
	 */
	minphys(bp);
}

/*
 * Save DMA pointers.  Take into account partial transfer. Shut down DMA.
 */
static void
sbic_save_ptrs(struct sbic_softc *dev, sbic_regmap_p regs)
{
	int count, asr, s;
	struct sbic_acb* acb;

	SBIC_TRACE(dev);
	if (!(dev->sc_flags & SBICF_INDMA))
		return; /* DMA not active */

	s = splbio();

	acb = dev->sc_nexus;
	if (acb == NULL) {
		splx(s);
		return;
	}
	count = -1;
	do {
		GET_SBIC_asr(regs, asr);
		if (asr & SBIC_ASR_DBR) {
			printf("sbic_save_ptrs: asr %02x canceled!\n", asr);
			splx(s);
			SBIC_TRACE(dev);
			return;
		}
	} while (asr & (SBIC_ASR_BSY | SBIC_ASR_CIP));

	/* Save important state */
	/* must be done before dmastop */
	SBIC_TC_GET(regs, count);

	/* Shut down DMA ====CAREFUL==== */
	dev->sc_dmastop(dev->sc_dmah, dev->sc_dmat, acb);
	dev->sc_flags &= ~SBICF_INDMA;
#ifdef DIAGNOSTIC
	{
		int count2;

		SBIC_TC_GET(regs, count2);
		if (count2 != count)
			panic("sbic_save_ptrs: DMA was still active(%d,%d)",
			    count, count2);
	}
#endif
	/* Note where we got to before stopping.  We need this to resume
	   later. */
	acb->offset += acb->sc_tcnt - count;
	SBIC_TC_PUT(regs, 0);

	DBGPRINTF(("SBIC saving tgt %d data pointers: Offset now %d ASR:%02x",
	    dev->target, acb->offset, asr), data_pointer_debug >= 1);

	acb->sc_tcnt = 0;

	DBG(sbicdma_saves++);
	splx(s);
	SBIC_TRACE(dev);
}

/*
 * used by specific sbic controller
 *
 * it appears that the higher level code does nothing with LUN's
 * so I will too.  I could plug it in, however so could they
 * in scsi_scsi_cmd().
 */
void
sbic_scsi_request(struct scsipi_channel *chan,
			scsipi_adapter_req_t req, void *arg)
{
	struct scsipi_xfer *xs;
	struct sbic_acb *acb;
	struct sbic_softc *dev = (void *)chan->chan_adapter->adapt_dev;
	struct scsipi_periph *periph;
	int flags, s, stat;

	switch (req) {
	case ADAPTER_REQ_RUN_XFER:
		xs = arg;
		periph = xs->xs_periph;
		SBIC_TRACE(dev);
		flags = xs->xs_control;

		if (flags & XS_CTL_DATA_UIO)
			panic("sbic: scsi data uio requested");

		if (dev->sc_nexus && (flags & XS_CTL_POLL))
			panic("sbic_scsicmd: busy");

		s = splbio();
		acb = dev->free_list.tqh_first;
		if (acb)
			TAILQ_REMOVE(&dev->free_list, acb, chain);
		splx(s);

		if (acb == NULL) {
			DBG(printf("sbic_scsicmd: unable to queue request for "
			    "target %d\n", periph->periph_target));
#if defined(DDB) && defined(DEBUG)
			Debugger();
#endif
			xs->error = XS_RESOURCE_SHORTAGE;
			SBIC_TRACE(dev);
			scsipi_done(xs);
			return;
		}

		acb->flags = ACB_ACTIVE;
		if (flags & XS_CTL_DATA_IN)
			acb->flags |= ACB_DATAIN;
		acb->xs = xs;
		memcpy(&acb->cmd, xs->cmd, xs->cmdlen);
		acb->clen = xs->cmdlen;
		acb->data = xs->data;
		acb->datalen = xs->datalen;

		QPRINTF(("sbic_scsi_request: Cmd %02x (len %d), Data %p(%d)\n",
		    (unsigned) acb->cmd.opcode, acb->clen, xs->data,
		    xs->datalen));
		if (flags & XS_CTL_POLL) {
			s = splbio();
			/*
			 * This has major side effects -- it locks up the
			 * machine.
			 */

			dev->sc_flags |= SBICF_ICMD;
			do {
				while (dev->sc_nexus)
					sbicpoll(dev);
				dev->sc_nexus = acb;
				dev->sc_stat[0] = -1;
				dev->target = periph->periph_target;
				dev->lun = periph->periph_lun;
				stat = sbicicmd(dev, periph->periph_target,
				    periph->periph_lun, acb);
			} while (dev->sc_nexus != acb);

			sbic_scsidone(acb, stat);
			splx(s);
			SBIC_TRACE(dev);
			return;
		}

		s = splbio();
		TAILQ_INSERT_TAIL(&dev->ready_list, acb, chain);

		if (dev->sc_nexus) {
			splx(s);
			SBIC_TRACE(dev);
			return;
		}

		/*
		 * Nothing is active, try to start it now.
		 */
		sbic_sched(dev);
		splx(s);

		SBIC_TRACE(dev);
/* TODO:  add sbic_poll to do XS_CTL_POLL operations */
		return;

	case ADAPTER_REQ_GROW_RESOURCES:
	case ADAPTER_REQ_SET_XFER_MODE:
		/* XXX Not supported.  */
		return;
	}
}

/*
 * attempt to start the next available command
 */
static void
sbic_sched(struct sbic_softc *dev)
{
	struct scsipi_xfer *xs;
	struct scsipi_periph *periph;
	struct sbic_acb *acb;
	int flags, /*phase,*/ stat, i;

	SBIC_TRACE(dev);
	if (dev->sc_nexus)
		return;			/* a command is current active */

	SBIC_TRACE(dev);
	for (acb = dev->ready_list.tqh_first; acb; acb = acb->chain.tqe_next) {
		periph = acb->xs->xs_periph;
		i = periph->periph_target;
		if (!(dev->sc_tinfo[i].lubusy & (1 << periph->periph_lun))) {
			struct sbic_tinfo *ti = &dev->sc_tinfo[i];

			TAILQ_REMOVE(&dev->ready_list, acb, chain);
			dev->sc_nexus = acb;
			periph = acb->xs->xs_periph;
			ti = &dev->sc_tinfo[periph->periph_target];
			ti->lubusy |= (1 << periph->periph_lun);
			break;
		}
	}

	SBIC_TRACE(dev);
	if (acb == NULL)
		return;			/* did not find an available command */

	xs = acb->xs;
	periph = xs->xs_periph;
	flags = xs->xs_control;

	if (flags & XS_CTL_RESET)
		sbicreset(dev);

	DBGPRINTF(("sbic_sched(%d,%d)\n", periph->periph_target,
	    periph->periph_lun), data_pointer_debug > 1);
	DBG(if (data_pointer_debug > 1) sbic_dump_acb(acb));
	dev->sc_stat[0] = -1;
	dev->target = periph->periph_target;
	dev->lun = periph->periph_lun;

	/* Decide if we can use DMA for this transfer.  */
	if ((flags & XS_CTL_POLL) == 0
	    && !sbic_no_dma
	    && dev->sc_dmaok(dev->sc_dmah, dev->sc_dmat, acb))
		acb->flags |= ACB_DMA;

	if ((flags & XS_CTL_POLL) ||
	    (!sbic_parallel_operations && (acb->flags & ACB_DMA) == 0))
		stat = sbicicmd(dev, periph->periph_target,
		    periph->periph_lun, acb);
	else if (sbicgo(dev, xs) == 0 && xs->error != XS_SELTIMEOUT) {
		SBIC_TRACE(dev);
		return;
	} else
		stat = dev->sc_stat[0];

	sbic_scsidone(acb, stat);
	SBIC_TRACE(dev);
}

static void
sbic_scsidone(struct sbic_acb *acb, int stat)
{
	struct scsipi_xfer *xs;
	struct scsipi_periph *periph;
	struct sbic_softc *dev;
/*	int s;*/
	int dosched = 0;

	xs = acb->xs;
	periph = xs->xs_periph;
	dev = (void *)periph->periph_channel->chan_adapter->adapt_dev;
	SBIC_TRACE(dev);
#ifdef DIAGNOSTIC
	if (acb == NULL || xs == NULL) {
		printf("sbic_scsidone -- (%d,%d) no scsipi_xfer\n",
		       dev->target, dev->lun);
#ifdef DDB
		Debugger();
#endif
		return;
	}
#endif

	DBGPRINTF(("scsidone: (%d,%d)->(%d,%d)%02x acbfl=%x\n",
	    periph->periph_target, periph->periph_lun,
	    dev->target,  dev->lun,  stat, acb->flags),
	    data_pointer_debug > 1);
	DBG(if (xs->xs_periph->periph_target == dev->sc_channel.chan_id)
	    panic("target == hostid"));

	xs->status = stat;
	xs->resid = 0;
	if (xs->error == XS_NOERROR) {
		if (stat == SCSI_CHECK || stat == SCSI_BUSY)
			xs->error = XS_BUSY;
	}

	/*
	 * Remove the ACB from whatever queue it's on.  We have to do a bit of
	 * a hack to figure out which queue it's on.  Note that it is *not*
	 * necessary to cdr down the ready queue, but we must cdr down the
	 * nexus queue and see if it's there, so we can mark the unit as no
	 * longer busy.  This code is sickening, but it works.
	 */
	if (acb == dev->sc_nexus) {
		dev->sc_nexus = NULL;
		dev->sc_tinfo[periph->periph_target].lubusy &=
		    ~(1 << periph->periph_lun);
		if (dev->ready_list.tqh_first)
			dosched = 1;	/* start next command */
	} else if (dev->ready_list.tqh_last == &acb->chain.tqe_next) {
		TAILQ_REMOVE(&dev->ready_list, acb, chain);
	} else {
		register struct sbic_acb *acb2;
		for (acb2 = dev->nexus_list.tqh_first; acb2;
		    acb2 = acb2->chain.tqe_next) {
			if (acb2 == acb) {
				TAILQ_REMOVE(&dev->nexus_list, acb, chain);
				dev->sc_tinfo[periph->periph_target].lubusy
					&= ~(1 << periph->periph_lun);
				break;
			}
		}
		if (acb2)
			;
		else if (acb->chain.tqe_next) {
			TAILQ_REMOVE(&dev->ready_list, acb, chain);
		} else {
			printf("%s: can't find matching acb\n",
			    dev->sc_dev.dv_xname);
#ifdef DDB
			Debugger();
#endif
		}
	}
	/* Put it on the free list. */
	acb->flags = ACB_FREE;
	TAILQ_INSERT_HEAD(&dev->free_list, acb, chain);

	dev->sc_tinfo[periph->periph_target].cmds++;

	scsipi_done(xs);

	if (dosched)
		sbic_sched(dev);
	SBIC_TRACE(dev);
}

static int
sbicwait(sbic_regmap_p regs, char until, int timeo, int line)
{
	u_char val;
	int csr;

	SBIC_TRACE((struct sbic_softc *)0);
	if (timeo == 0)
		timeo = 1000000;	/* some large value.. */

	GET_SBIC_asr(regs,val);
	while ((val & until) == 0) {
		if (timeo-- == 0) {
			GET_SBIC_csr(regs, csr);
			printf("sbicwait TIMEO @%d with asr=x%x csr=x%x\n",
			    line, val, csr);
#if defined(DDB) && defined(DEBUG)
			Debugger();
#endif
			return val; /* Maybe I should abort */
			break;
		}
		DELAY(1);
		GET_SBIC_asr(regs,val);
	}
	SBIC_TRACE((struct sbic_softc *)0);
	return val;
}

static int
sbicabort(struct sbic_softc *dev, sbic_regmap_p regs, char *where)
{
	u_char csr, asr;

	GET_SBIC_asr(regs, asr);
	GET_SBIC_csr(regs, csr);

	printf ("%s: abort %s: csr = 0x%02x, asr = 0x%02x\n",
	    dev->sc_dev.dv_xname, where, csr, asr);


#if 0
	/* Clean up running command */
	if (dev->sc_nexus != NULL) {
		dev->sc_nexus->xs->error = XS_DRIVER_STUFFUP;
		sbic_scsidone(dev->sc_nexus, dev->sc_stat[0]);
	}
	while (acb = dev->nexus_list.tqh_first) {
		acb->xs->error = XS_DRIVER_STUFFUP;
		sbic_scsidone(acb, -1 /*acb->stat[0]*/);
	}
#endif

	/* Clean up chip itself */
	if (dev->sc_flags & SBICF_SELECTED) {
		while (asr & SBIC_ASR_DBR) {
			/* sbic is jammed w/data. need to clear it */
			/* But we don't know what direction it needs to go */
			GET_SBIC_data(regs, asr);
			printf("%s: abort %s: clearing data buffer 0x%02x\n",
			       dev->sc_dev.dv_xname, where, asr);
			GET_SBIC_asr(regs, asr);
			/* Not the read direction, then */
			if (asr & SBIC_ASR_DBR)
				SET_SBIC_data(regs, asr);
			GET_SBIC_asr(regs, asr);
		}
		WAIT_CIP(regs);
		printf("%s: sbicabort - sending ABORT command\n",
		    dev->sc_dev.dv_xname);
		SET_SBIC_cmd(regs, SBIC_CMD_ABORT);
		WAIT_CIP(regs);

		GET_SBIC_asr(regs, asr);
		if (asr & (SBIC_ASR_BSY | SBIC_ASR_LCI)) {
			/* ok, get more drastic.. */

			printf("%s: sbicabort - asr %x, trying to reset\n",
			    dev->sc_dev.dv_xname, asr);
			sbicreset(dev);
			dev->sc_flags &= ~SBICF_SELECTED;
			return -1;
		}
		printf("%s: sbicabort - sending DISC command\n",
		    dev->sc_dev.dv_xname);
		SET_SBIC_cmd(regs, SBIC_CMD_DISC);

		do {
			asr = SBIC_WAIT (regs, SBIC_ASR_INT, 0);
			GET_SBIC_csr (regs, csr);
			CSR_TRACE('a',csr,asr,0);
		} while ((csr != SBIC_CSR_DISC) && (csr != SBIC_CSR_DISC_1)
		    && (csr != SBIC_CSR_CMD_INVALID));

		/* lets just hope it worked.. */
		dev->sc_flags &= ~SBICF_SELECTED;
	}
	return -1;
}


/*
 * Initialize driver-private structures
 */

int
sbicinit(struct sbic_softc *dev)
{
	sbic_regmap_p regs;
	u_int i;
/*	u_int my_id, s;*/
/*	u_char csr;*/
	struct sbic_acb *acb;
	u_int inhibit_sync;

	extern u_long scsi_nosync;
	extern int shift_nosync;

	SBIC_DEBUG(printf("sbicinit:\n"));

	regs = &dev->sc_sbicp;

	if ((dev->sc_flags & SBICF_ALIVE) == 0) {
		TAILQ_INIT(&dev->ready_list);
		TAILQ_INIT(&dev->nexus_list);
		TAILQ_INIT(&dev->free_list);
		callout_init(&dev->sc_timo_ch);
		dev->sc_nexus = NULL;
		acb = dev->sc_acb;
		memset(acb, 0, sizeof(dev->sc_acb));

		SBIC_DEBUG(printf("sbicinit: %d\n", __LINE__));

		for (i = 0; i < sizeof(dev->sc_acb) / sizeof(*acb); i++) {
			TAILQ_INSERT_TAIL(&dev->free_list, acb, chain);
			acb++;
		}
		memset(dev->sc_tinfo, 0, sizeof(dev->sc_tinfo));
		/* make sure timeout is really not needed */
		DBG(callout_reset(&dev->sc_timo_ch, 30 * hz,
		    (void *)sbictimeout, dev));
	} else
		panic("sbic: reinitializing driver!");

	SBIC_DEBUG(printf("sbicinit: %d\n", __LINE__));

	dev->sc_flags |= SBICF_ALIVE;
	dev->sc_flags &= ~SBICF_SELECTED;

	/* initialize inhibit array */
	if (scsi_nosync) {

		SBIC_DEBUG(printf("sbicinit: %d\n", __LINE__));

		inhibit_sync = (scsi_nosync >> shift_nosync) & 0xff;
		shift_nosync += 8;

		DBGPRINTF(("%s: Inhibiting synchronous transfer %02x\n",
		    dev->sc_dev.dv_xname, inhibit_sync), inhibit_sync);

		for (i = 0; i < 8; ++i)
			if (inhibit_sync & (1 << i))
				sbic_inhibit_sync[i] = 1;
	}

	SBIC_DEBUG(printf("sbicinit: %d\n", __LINE__));

	sbicreset(dev);
	return 0;
}

static void
sbicreset(struct sbic_softc *dev)
{
	sbic_regmap_p regs;
	u_int my_id, s;
/*	u_int i;*/
	u_char csr;
/*	struct sbic_acb *acb;*/

	SBIC_DEBUG(printf("sbicreset: %d\n", __LINE__));

	regs = &dev->sc_sbicp;

	SBIC_DEBUG(printf("sbicreset: regs = %08x\n", regs));

#if 0
	if (dev->sc_flags & SBICF_ALIVE) {
		SET_SBIC_cmd(regs, SBIC_CMD_ABORT);
		WAIT_CIP(regs);
	}
#else
	SET_SBIC_cmd(regs, SBIC_CMD_ABORT);

	SBIC_DEBUG(printf("sbicreset: %d\n", __LINE__));

	WAIT_CIP(regs);

	SBIC_DEBUG(printf("sbicreset: %d\n", __LINE__));
#endif
	s = splbio();
	my_id = dev->sc_channel.chan_id & SBIC_ID_MASK;

	/* Enable advanced mode */
	my_id |= SBIC_ID_EAF /*| SBIC_ID_EHP*/ ;
	SET_SBIC_myid(regs, my_id);

	SBIC_DEBUG(printf("sbicreset: %d\n", __LINE__));

	/*
	 * Disable interrupts (in dmainit) then reset the chip
	 */
	SET_SBIC_cmd(regs, SBIC_CMD_RESET);
	DELAY(25);
	SBIC_WAIT(regs, SBIC_ASR_INT, 0);
	GET_SBIC_csr(regs, csr);       /* clears interrupt also */

	if (dev->sc_clkfreq < 110)
		my_id |= SBIC_ID_FS_8_10;
	else if (dev->sc_clkfreq < 160)
		my_id |= SBIC_ID_FS_12_15;
	else if (dev->sc_clkfreq < 210)
		my_id |= SBIC_ID_FS_16_20;

	SET_SBIC_myid(regs, my_id);

	SBIC_DEBUG(printf("sbicreset: %d\n", __LINE__));

	/*
	 * Set up various chip parameters
	 */
	SET_SBIC_control(regs, SBIC_CTL_EDI | SBIC_CTL_IDI /* | SBIC_CTL_HSP */
	    | dev->sc_dmamode);
	/*
	 * don't allow (re)selection (SBIC_RID_ES)
	 * until we can handle target mode!!
	 */
	SET_SBIC_rselid(regs, SBIC_RID_ER);
	SET_SBIC_syn(regs, 0);     /* asynch for now */

	/*
	 * anything else was zeroed by reset
	 */
	splx(s);

#if 0
	if ((dev->sc_flags & SBICF_ALIVE) == 0) {
		TAILQ_INIT(&dev->ready_list);
		TAILQ_INIT(&dev->nexus_list);
		TAILQ_INIT(&dev->free_list);
		dev->sc_nexus = NULL;
		acb = dev->sc_acb;
		memset(acb, 0, sizeof(dev->sc_acb));
		for (i = 0; i < sizeof(dev->sc_acb) / sizeof(*acb); i++) {
			TAILQ_INSERT_TAIL(&dev->free_list, acb, chain);
			acb++;
		}
		memset(dev->sc_tinfo, 0, sizeof(dev->sc_tinfo));
	} else {
		if (dev->sc_nexus != NULL) {
			dev->sc_nexus->xs->error = XS_DRIVER_STUFFUP;
			sbic_scsidone(dev->sc_nexus, dev->sc_stat[0]);
		}
		while (acb = dev->nexus_list.tqh_first) {
			acb->xs->error = XS_DRIVER_STUFFUP;
			sbic_scsidone(acb, -1 /*acb->stat[0]*/);
		}
	}

	dev->sc_flags |= SBICF_ALIVE;
#endif
	dev->sc_flags &= ~SBICF_SELECTED;
}

static void
sbicerror(struct sbic_softc *dev, sbic_regmap_p regs, u_char csr)
{
#ifdef DIAGNOSTIC
	if (dev->sc_nexus == NULL)
		panic("sbicerror");
#endif
	if (dev->sc_nexus->xs->xs_control & XS_CTL_SILENT)
		return;

	printf("%s: ", dev->sc_dev.dv_xname);
	printf("csr == 0x%02x\n", csr);	/* XXX */
}

/*
 * select the bus, return when selected or error.
 */
static int
sbicselectbus(struct sbic_softc *dev, sbic_regmap_p regs, u_char target,
    u_char lun, u_char our_addr)
{
	u_char asr, csr, id;

	SBIC_TRACE(dev);
	QPRINTF(("sbicselectbus %d\n", target));

	/*
	 * if we're already selected, return (XXXX panic maybe?)
	 */
	if (dev->sc_flags & SBICF_SELECTED) {
		SBIC_TRACE(dev);
		return 1;
	}

	/*
	 * issue select
	 */
	SBIC_TC_PUT(regs, 0);
	SET_SBIC_selid(regs, target);
	SET_SBIC_timeo(regs, SBIC_TIMEOUT(250,dev->sc_clkfreq));

	/*
	 * set sync or async
	 */
	if (dev->sc_sync[target].state == SYNC_DONE)
		SET_SBIC_syn(regs, SBIC_SYN (dev->sc_sync[target].offset,
		    dev->sc_sync[target].period));
	else
		SET_SBIC_syn(regs, SBIC_SYN (0, sbic_min_period));

	GET_SBIC_asr(regs, asr);
	if (asr & (SBIC_ASR_INT | SBIC_ASR_BSY)) {
		/* This means we got ourselves reselected upon */
/*		printf("sbicselectbus: INT/BSY asr %02x\n", asr);*/
#ifdef DDB
/*		Debugger();*/
#endif
		SBIC_TRACE(dev);
		return 1;
	}

	SET_SBIC_cmd(regs, SBIC_CMD_SEL_ATN);

	/*
	 * wait for select (merged from separate function may need
	 * cleanup)
	 */
	WAIT_CIP(regs);
	do {
		asr = SBIC_WAIT(regs, SBIC_ASR_INT | SBIC_ASR_LCI, 0);
		if (asr & SBIC_ASR_LCI) {

			DBGPRINTF(("sbicselectbus: late LCI asr %02x\n", asr),
			    reselect_debug);

			SBIC_TRACE(dev);
			return 1;
		}
		GET_SBIC_csr (regs, csr);
		CSR_TRACE('s',csr,asr,target);
		QPRINTF(("%02x ", csr));
		if (csr == SBIC_CSR_RSLT_NI || csr == SBIC_CSR_RSLT_IFY) {

			DBGPRINTF(("sbicselectbus: reselected asr %02x\n",
			    asr), reselect_debug);

			/* We need to handle this now so we don't lock
			   up later */
			sbicnextstate(dev, csr, asr);
			SBIC_TRACE(dev);
			return 1;
		}
		if (csr == SBIC_CSR_SLT || csr == SBIC_CSR_SLT_ATN) {
			panic("sbicselectbus: target issued select!");
			return 1;
		}
	} while (csr != (SBIC_CSR_MIS_2 | MESG_OUT_PHASE) &&
	    csr != (SBIC_CSR_MIS_2 | CMD_PHASE) &&
	    csr != SBIC_CSR_SEL_TIMEO);

	/* Enable (or not) reselection */
	if (!sbic_enable_reselect && dev->nexus_list.tqh_first == NULL)
		SET_SBIC_rselid (regs, 0);
	else
		SET_SBIC_rselid (regs, SBIC_RID_ER);

	if (csr == (SBIC_CSR_MIS_2 | CMD_PHASE)) {
		dev->sc_flags |= SBICF_SELECTED;  /* device ignored ATN */
		GET_SBIC_selid(regs, id);
		dev->target = id;
		GET_SBIC_tlun(regs,dev->lun);
		if (dev->lun & SBIC_TLUN_VALID)
			dev->lun &= SBIC_TLUN_MASK;
		else
			dev->lun = lun;
	} else if (csr == (SBIC_CSR_MIS_2 | MESG_OUT_PHASE)) {
		/*
		 * Send identify message
		 * (SCSI-2 requires an identify msg (?))
		 */
		GET_SBIC_selid(regs, id);
		dev->target = id;
		GET_SBIC_tlun(regs,dev->lun);
		if (dev->lun & SBIC_TLUN_VALID)
			dev->lun &= SBIC_TLUN_MASK;
		else
			dev->lun = lun;
		/*
		 * handle drives that don't want to be asked
		 * whether to go sync at all.
		 */
		if (sbic_inhibit_sync[id]
		    && dev->sc_sync[id].state == SYNC_START) {
			DBGPRINTF(("Forcing target %d asynchronous.\n", id),
			    sync_debug);

			dev->sc_sync[id].offset = 0;
			dev->sc_sync[id].period = sbic_min_period;
			dev->sc_sync[id].state = SYNC_DONE;
		}


		if (dev->sc_sync[id].state != SYNC_START){
			if ((dev->sc_nexus->xs->xs_control & XS_CTL_POLL)
			    || (dev->sc_flags & SBICF_ICMD)
			    || !sbic_enable_reselect)
				SEND_BYTE(regs, MSG_IDENTIFY | lun);
			else
				SEND_BYTE(regs, MSG_IDENTIFY_DR | lun);
		} else {
			/*
			 * try to initiate a sync transfer.
			 * So compose the sync message we're going
			 * to send to the target
			 */

			DBGPRINTF(("Sending sync request to target %d ... ",
			    id), sync_debug);

			/*
			 * setup scsi message sync message request
			 */
			dev->sc_msg[0] = MSG_IDENTIFY | lun;
			dev->sc_msg[1] = MSG_EXT_MESSAGE;
			dev->sc_msg[2] = 3;
			dev->sc_msg[3] = MSG_SYNC_REQ;
			dev->sc_msg[4] = sbictoscsiperiod(dev, regs,
			    sbic_min_period);
			dev->sc_msg[5] = sbic_max_offset;

			if (sbicxfstart(regs, 6, MESG_OUT_PHASE,
			    sbic_cmd_wait))
				sbicxfout(regs, 6, dev->sc_msg,
				    MESG_OUT_PHASE);

			dev->sc_sync[id].state = SYNC_SENT;

			DBGPRINTF(("sent\n"), sync_debug);
		}

		asr = SBIC_WAIT (regs, SBIC_ASR_INT, 0);
		GET_SBIC_csr (regs, csr);
		CSR_TRACE('y',csr,asr,target);
		QPRINTF(("[%02x]", csr));

		DBGPRINTF(("csr-result of last msgout: 0x%x\n", csr),
		    sync_debug && dev->sc_sync[id].state == SYNC_SENT);

		if (csr != SBIC_CSR_SEL_TIMEO)
			dev->sc_flags |= SBICF_SELECTED;
	}
	if (csr == SBIC_CSR_SEL_TIMEO)
		dev->sc_nexus->xs->error = XS_SELTIMEOUT;

	QPRINTF(("\n"));

	SBIC_TRACE(dev);
	return csr == SBIC_CSR_SEL_TIMEO;
}

static int
sbicxfstart(sbic_regmap_p regs, int len, u_char phase, int wait)
{
	u_char id;

	switch (phase) {
	case DATA_IN_PHASE:
	case MESG_IN_PHASE:
		GET_SBIC_selid (regs, id);
		id |= SBIC_SID_FROM_SCSI;
		SET_SBIC_selid (regs, id);
		SBIC_TC_PUT (regs, (unsigned)len);
		break;
	case DATA_OUT_PHASE:
	case MESG_OUT_PHASE:
	case CMD_PHASE:
		GET_SBIC_selid (regs, id);
		id &= ~SBIC_SID_FROM_SCSI;
		SET_SBIC_selid (regs, id);
		SBIC_TC_PUT (regs, (unsigned)len);
		break;
	default:
		SBIC_TC_PUT (regs, 0);
	}
	QPRINTF(("sbicxfstart %d, %d, %d\n", len, phase, wait));

	return 1;
}

static int
sbicxfout(sbic_regmap_p regs, int len, void *bp, int phase)
{
#ifdef UNPROTECTED_CSR
	u_char orig_csr
#endif
	u_char asr, *buf;
/*	u_char csr;*/
	int wait;

	buf = bp;
	wait = sbic_data_wait;

	QPRINTF(("sbicxfout {%d} %02x %02x %02x %02x %02x "
	    "%02x %02x %02x %02x %02x\n", len, buf[0], buf[1], buf[2],
	    buf[3], buf[4], buf[5], buf[6], buf[7], buf[8], buf[9]));

#ifdef UNPROTECTED_CSR
	GET_SBIC_csr (regs, orig_csr);
	CSR_TRACE('>',orig_csr,0,0);
#endif

	/*
	 * sigh.. WD-PROTO strikes again.. sending the command in one go
	 * causes the chip to lock up if talking to certain (misbehaving?)
	 * targets. Anyway, this procedure should work for all targets, but
	 * it's slightly slower due to the overhead
	 */
	WAIT_CIP (regs);
	SET_SBIC_cmd (regs, SBIC_CMD_XFER_INFO);
	for (;len > 0; len--) {
		GET_SBIC_asr (regs, asr);
		while ((asr & SBIC_ASR_DBR) == 0) {
			if ((asr & SBIC_ASR_INT) || --wait < 0) {

				DBGPRINTF(("sbicxfout fail: l%d i%x w%d\n",
				    len, asr, wait), sbic_debug);

				return len;
			}
/*			DELAY(1);*/
			GET_SBIC_asr (regs, asr);
		}

		SET_SBIC_data (regs, *buf);
		buf++;
	}
	SBIC_TC_GET(regs, len);
	QPRINTF(("sbicxfout done %d bytes\n", len));
	/*
	 * this leaves with one csr to be read
	 */
	return 0;
}

/* returns # bytes left to read */
static int
sbicxfin(sbic_regmap_p regs, int len, void *bp)
{
	int wait;
/*	int read;*/
	u_char *obp, *buf;
#ifdef UNPROTECTED_CSR
	u_char orig_csr, csr;
#endif
	u_char asr;

	wait = sbic_data_wait;
	obp = bp;
	buf = bp;

#ifdef UNPROTECTED_CSR
	GET_SBIC_csr (regs, orig_csr);
	CSR_TRACE('<',orig_csr,0,0);

	QPRINTF(("sbicxfin %d, csr=%02x\n", len, orig_csr));
#endif

	WAIT_CIP (regs);
	SET_SBIC_cmd (regs, SBIC_CMD_XFER_INFO);
	for (;len > 0; len--) {
		GET_SBIC_asr (regs, asr);
		if ((asr & SBIC_ASR_PE)) {
			DBG(printf("sbicxfin parity error: l%d i%x w%d\n",
			    len, asr, wait));
#if defined(DDB) && defined(DEBUG)
			Debugger();
#endif
			DBG(return ((unsigned long)buf - (unsigned long)bp));
		}
		while ((asr & SBIC_ASR_DBR) == 0) {
			if ((asr & SBIC_ASR_INT) || --wait < 0) {

				DBG(if (sbic_debug) {
	QPRINTF(("sbicxfin fail:{%d} %02x %02x %02x %02x %02x %02x "
	    "%02x %02x %02x %02x\n", len, obp[0], obp[1], obp[2],
	    obp[3], obp[4], obp[5], obp[6], obp[7], obp[8], obp[9]));
	printf("sbicxfin fail: l%d i%x w%d\n", len, asr, wait); });

				return len;
			}

#ifdef UNPROTECTED_CSR
			if (!(asr & SBIC_ASR_BSY)) {
				GET_SBIC_csr(regs, csr);
				CSR_TRACE('<',csr,asr,len);
				QPRINTF(("[CSR%02xASR%02x]", csr, asr));
			}
#endif

/*			DELAY(1);*/
			GET_SBIC_asr (regs, asr);
		}

		GET_SBIC_data (regs, *buf);
/*		QPRINTF(("asr=%02x, csr=%02x, data=%02x\n", asr, csr, *buf));*/
		buf++;
	}

	QPRINTF(("sbicxfin {%d} %02x %02x %02x %02x %02x %02x "
	    "%02x %02x %02x %02x\n", len, obp[0], obp[1], obp[2],
	    obp[3], obp[4], obp[5], obp[6], obp[7], obp[8], obp[9]));

	/* this leaves with one csr to be read */
	return len;
}

/*
 * SCSI 'immediate' command:  issue a command to some SCSI device
 * and get back an 'immediate' response (i.e., do programmed xfer
 * to get the response data).  'cbuf' is a buffer containing a scsi
 * command of length clen bytes.  'buf' is a buffer of length 'len'
 * bytes for data.  The transfer direction is determined by the device
 * (i.e., by the scsi bus data xfer phase).  If 'len' is zero, the
 * command must supply no data.
 */
static int
sbicicmd(struct sbic_softc *dev, int target, int lun, struct sbic_acb *acb)
{
	sbic_regmap_p regs;
	u_char phase, csr, asr;
	int wait;
/*	int newtarget, cmd_sent, parity_err;*/

/*	int discon;*/
	int i;

	void *cbuf, *buf;
	int clen, len;

#define CSR_LOG_BUF_SIZE 0
#if CSR_LOG_BUF_SIZE
	int bufptr;
	int csrbuf[CSR_LOG_BUF_SIZE];
	bufptr = 0;
#endif

	cbuf = &acb->cmd;
	clen = acb->clen;
	buf = acb->data;
	len = acb->datalen;

	SBIC_TRACE(dev);
	regs = &dev->sc_sbicp;

	acb->sc_tcnt = 0;

	DBG(routine = 3);
	DBG(debug_sbic_regs = regs); /* store this to allow debug calls */
	DBGPRINTF(("sbicicmd(%d,%d):%d\n", target, lun, len),
	    data_pointer_debug > 1);

	/*
	 * set the sbic into non-DMA mode
	 */
	SET_SBIC_control(regs, SBIC_CTL_EDI | SBIC_CTL_IDI /*| SBIC_CTL_HSP*/);

	dev->sc_stat[0] = 0xff;
	dev->sc_msg[0] = 0xff;
	i = 1; /* pre-load */

	/* We're stealing the SCSI bus */
	dev->sc_flags |= SBICF_ICMD;

	do {
		/*
		 * select the SCSI bus (it's an error if bus isn't free)
		 */
		if (!(dev->sc_flags & SBICF_SELECTED)
		    && sbicselectbus(dev, regs, target, lun,
			dev->sc_scsiaddr)) {
			/*printf("sbicicmd trying to select busy bus!\n");*/
			dev->sc_flags &= ~SBICF_ICMD;
			return -1;
		}

		/*
		 * Wait for a phase change (or error) then let the
		 * device sequence us through the various SCSI phases.
		 */

		wait = sbic_cmd_wait;

		asr = GET_SBIC_asr (regs, asr);
		GET_SBIC_csr (regs, csr);
		CSR_TRACE('I',csr,asr,target);
		QPRINTF((">ASR:%02xCSR:%02x<", asr, csr));

#if CSR_LOG_BUF_SIZE
		csrbuf[bufptr++] = csr;
#endif


		switch (csr) {
		case SBIC_CSR_S_XFERRED:
		case SBIC_CSR_DISC:
		case SBIC_CSR_DISC_1:
			dev->sc_flags &= ~SBICF_SELECTED;
			GET_SBIC_cmd_phase (regs, phase);
			if (phase == 0x60) {
				GET_SBIC_tlun (regs, dev->sc_stat[0]);
				i = 0; /* done */
/*				break;*/ /* Bypass all the state gobldygook */
			} else {
				DBGPRINTF(("sbicicmd: handling disconnect\n"),
				    reselect_debug > 1);

				i = SBIC_STATE_DISCONNECT;
			}
			break;

		case SBIC_CSR_XFERRED | CMD_PHASE:
		case SBIC_CSR_MIS     | CMD_PHASE:
		case SBIC_CSR_MIS_1   | CMD_PHASE:
		case SBIC_CSR_MIS_2   | CMD_PHASE:
			if (sbicxfstart(regs, clen, CMD_PHASE, sbic_cmd_wait))
				if (sbicxfout(regs, clen,
					      cbuf, CMD_PHASE))
					i = sbicabort(dev, regs,
					    "icmd sending cmd");
#if 0
			GET_SBIC_csr(regs, csr); /* Lets us reload tcount */
			WAIT_CIP(regs);
			GET_SBIC_asr(regs, asr);
			CSR_TRACE('I',csr,asr,target);
			if (asr & (SBIC_ASR_BSY | SBIC_ASR_LCI | SBIC_ASR_CIP))
				printf("next: cmd sent asr %02x, csr %02x\n",
				    asr, csr);
#endif
			break;

#if 0
		case SBIC_CSR_XFERRED | DATA_OUT_PHASE:
		case SBIC_CSR_XFERRED | DATA_IN_PHASE:
		case SBIC_CSR_MIS     | DATA_OUT_PHASE:
		case SBIC_CSR_MIS     | DATA_IN_PHASE:
		case SBIC_CSR_MIS_1   | DATA_OUT_PHASE:
		case SBIC_CSR_MIS_1   | DATA_IN_PHASE:
		case SBIC_CSR_MIS_2   | DATA_OUT_PHASE:
		case SBIC_CSR_MIS_2   | DATA_IN_PHASE:
			if (acb->datalen <= 0)
				i = sbicabort(dev, regs, "icmd out of data");
			else {
			  wait = sbic_data_wait;
			  if (sbicxfstart(regs, acb->datalen,
					  SBIC_PHASE(csr), wait))
			    if (csr & 0x01)
			      /* data in? */
			      i = sbicxfin(regs, acb->datalen, acb->data);
			    else
			      i = sbicxfout(regs, acb->datalen, acb->data,
				  SBIC_PHASE(csr));
			  acb->data += acb->datalen - i;
			  acb->datalen = i;
			  i = 1;
			}
			break;

#endif
		case SBIC_CSR_XFERRED | STATUS_PHASE:
		case SBIC_CSR_MIS     | STATUS_PHASE:
		case SBIC_CSR_MIS_1   | STATUS_PHASE:
		case SBIC_CSR_MIS_2   | STATUS_PHASE:
			/*
			 * the sbic does the status/cmd-complete reading ok,
			 * so do this with its hi-level commands.
			 */
			DBGPRINTF(("SBICICMD status phase\n"), sbic_debug);

			SBIC_TC_PUT(regs, 0);
			SET_SBIC_cmd_phase(regs, 0x46);
			SET_SBIC_cmd(regs, SBIC_CMD_SEL_ATN_XFER);
			break;

#if THIS_IS_A_RESERVED_STATE
		case BUS_FREE_PHASE:		/* This is not legal */
			if (dev->sc_stat[0] != 0xff)
				goto out;
			break;
#endif

		default:
			i = sbicnextstate(dev, csr, asr);
		}

		/*
		 * make sure the last command was taken,
		 * ie. we're not hunting after an ignored command..
		 */
		GET_SBIC_asr(regs, asr);

		/* tapes may take a loooong time.. */
		while (asr & SBIC_ASR_BSY){
			if (asr & SBIC_ASR_DBR) {
				printf("sbicicmd: Waiting while sbic is "
				    "jammed, CSR:%02x,ASR:%02x\n",
				    csr, asr);
#ifdef DDB
				Debugger();
#endif
				/* SBIC is jammed */
				/* DUNNO which direction */
				/* Try old direction */
				GET_SBIC_data(regs,i);
				GET_SBIC_asr(regs, asr);
				if (asr & SBIC_ASR_DBR) /* Wants us to write */
					SET_SBIC_data(regs,i);
			}
			GET_SBIC_asr(regs, asr);
		}

		/*
		 * wait for last command to complete
		 */
		if (asr & SBIC_ASR_LCI) {
			printf("sbicicmd: last command ignored\n");
		}
		else if (i == 1) /* Bsy */
			SBIC_WAIT(regs, SBIC_ASR_INT, wait);

		/*
		 * do it again
		 */
	} while (i > 0 && dev->sc_stat[0] == 0xff);

	/* Sometimes we need to do an extra read of the CSR */
	GET_SBIC_csr(regs, csr);
	CSR_TRACE('I',csr,asr,0xff);

#if CSR_LOG_BUF_SIZE
	if (reselect_debug > 1)
		for (i = 0; i < bufptr; i++)
			printf("CSR:%02x", csrbuf[i]);
#endif

	DBGPRINTF(("sbicicmd done(%d,%d):%d =%d=\n",
	    dev->target, lun,
	    acb->datalen,
	    dev->sc_stat[0]),
	    data_pointer_debug > 1);

	QPRINTF(("=STS:%02x=", dev->sc_stat[0]));
	dev->sc_flags &= ~SBICF_ICMD;

	SBIC_TRACE(dev);
	return dev->sc_stat[0];
}

/*
 * Finish SCSI xfer command:  After the completion interrupt from
 * a read/write operation, sequence through the final phases in
 * programmed i/o.  This routine is a lot like sbicicmd except we
 * skip (and don't allow) the select, cmd out and data in/out phases.
 */
static void
sbicxfdone(struct sbic_softc *dev, sbic_regmap_p regs, int target)
{
	u_char phase, asr, csr;
	int s;

	SBIC_TRACE(dev);
	QPRINTF(("{"));
	s = splbio();

	/*
	 * have the sbic complete on its own
	 */
	SBIC_TC_PUT(regs, 0);
	SET_SBIC_cmd_phase(regs, 0x46);
	SET_SBIC_cmd(regs, SBIC_CMD_SEL_ATN_XFER);

	do {
		asr = SBIC_WAIT (regs, SBIC_ASR_INT, 0);
		GET_SBIC_csr (regs, csr);
		CSR_TRACE('f',csr,asr,target);
		QPRINTF(("%02x:", csr));
	} while ((csr != SBIC_CSR_DISC) && (csr != SBIC_CSR_DISC_1)
	    && (csr != SBIC_CSR_S_XFERRED));

	dev->sc_flags &= ~SBICF_SELECTED;

	GET_SBIC_cmd_phase (regs, phase);
	QPRINTF(("}%02x", phase));
	if (phase == 0x60)
		GET_SBIC_tlun(regs, dev->sc_stat[0]);
	else
		sbicerror(dev, regs, csr);

	QPRINTF(("=STS:%02x=\n", dev->sc_stat[0]));
	splx(s);
	SBIC_TRACE(dev);
}

	/*
	 * No DMA chains
	 */

static int
sbicgo(struct sbic_softc *dev, struct scsipi_xfer *xs)
{
	int i, usedma;
/*	int dmaflags, count; */
/*	int wait;*/
/*	u_char cmd;*/
	u_char asr = 0, csr = 0;
/*	u_char *addr; */
	sbic_regmap_p regs;
	struct sbic_acb *acb;

	SBIC_TRACE(dev);
	dev->target = xs->xs_periph->periph_target;
	dev->lun = xs->xs_periph->periph_lun;
	acb = dev->sc_nexus;
	regs = &dev->sc_sbicp;

	usedma = acb->flags & ACB_DMA;

	DBG(routine = 1);
	DBG(debug_sbic_regs = regs); /* store this to allow debug calls */
	DBGPRINTF(("sbicgo(%d,%d)\n", dev->target, dev->lun),
	    data_pointer_debug > 1);

	/*
	 * set the sbic into DMA mode
	 */
	if (usedma)
		SET_SBIC_control(regs,
		    SBIC_CTL_EDI | SBIC_CTL_IDI | dev->sc_dmamode);
	else
		SET_SBIC_control(regs, SBIC_CTL_EDI | SBIC_CTL_IDI);


	/*
	 * select the SCSI bus (it's an error if bus isn't free)
	 */
	if (sbicselectbus(dev, regs, dev->target, dev->lun,
	    dev->sc_scsiaddr)) {
/*		printf("sbicgo: Trying to select busy bus!\n"); */
		SBIC_TRACE(dev);
		/* Not done: may need to be rescheduled */
		return 0;
	}
	dev->sc_stat[0] = 0xff;

	/*
	 * Allocate the DMA chain
	 */

	/* Mark end of segment */
	acb->sc_tcnt = 0;

	SBIC_TRACE(dev);
	/* Enable interrupts */
	dev->sc_enintr(dev);
	if (usedma) {
		int tcnt;

		acb->offset = 0;
		acb->sc_tcnt = 0;
		/* Note, this does not start DMA */
		tcnt = dev->sc_dmasetup(dev->sc_dmah, dev->sc_dmat, acb,
		    (acb->flags & ACB_DATAIN) != 0);

		DBG(dev->sc_dmatimo = tcnt ? 1 : 0);
		DBG(++sbicdma_ops);	/* count total DMA operations */
	}

	SBIC_TRACE(dev);

	/*
	 * enintr() also enables interrupts for the sbic
	 */
	DBG(debug_asr = asr);
	DBG(debug_csr = csr);

	/*
	 * Lets cycle a while then let the interrupt handler take over
	 */

	asr = GET_SBIC_asr(regs, asr);
	do {
		GET_SBIC_csr(regs, csr);
		CSR_TRACE('g', csr, asr, dev->target);

		DBG(debug_csr = csr);
		DBG(routine = 1);

		QPRINTF(("go[0x%x]", csr));

		i = sbicnextstate(dev, csr, asr);

		WAIT_CIP(regs);
		GET_SBIC_asr(regs, asr);

		DBG(debug_asr = asr);

		if (asr & SBIC_ASR_LCI)
			printf("sbicgo: LCI asr:%02x csr:%02x\n", asr, csr);
	} while (i == SBIC_STATE_RUNNING &&
	    (asr & (SBIC_ASR_INT | SBIC_ASR_LCI)));

	CSR_TRACE('g',csr,asr,i<<4);
	SBIC_TRACE(dev);
	if (i == SBIC_STATE_DONE && dev->sc_stat[0] == 0xff)
		printf("sbicgo: done & stat = 0xff\n");
	if (i == SBIC_STATE_DONE && dev->sc_stat[0] != 0xff) {
/*	if (i == SBIC_STATE_DONE && dev->sc_stat[0]) { */
		/* Did we really finish that fast? */
		return 1;
	}
	return 0;
}


int
sbicintr(struct sbic_softc *dev)
{
	sbic_regmap_p regs;
	u_char asr, csr;
/*	u_char *tmpaddr;*/
/*	struct sbic_acb *acb;*/
	int i;
/*	int newtarget, newlun;*/
/*	unsigned tcnt;*/

	regs = &dev->sc_sbicp;

	/*
	 * pending interrupt?
	 */
	GET_SBIC_asr (regs, asr);
	if ((asr & SBIC_ASR_INT) == 0)
		return 0;

	SBIC_TRACE(dev);
	do {
		GET_SBIC_csr(regs, csr);
		CSR_TRACE('i',csr,asr,dev->target);

		DBG(debug_csr = csr);
		DBG(routine = 2);

		QPRINTF(("intr[0x%x]", csr));

		i = sbicnextstate(dev, csr, asr);

		WAIT_CIP(regs);
		GET_SBIC_asr(regs, asr);

		DBG(debug_asr = asr);

#if 0
		if (asr & SBIC_ASR_LCI)
			printf("sbicintr: LCI asr:%02x csr:%02x\n", asr, csr);
#endif
	} while (i == SBIC_STATE_RUNNING &&
	    (asr & (SBIC_ASR_INT | SBIC_ASR_LCI)));
	CSR_TRACE('i', csr, asr, i << 4);
	SBIC_TRACE(dev);
	return 1;
}

/*
 * Run commands and wait for disconnect
 */
static int
sbicpoll(struct sbic_softc *dev)
{
	sbic_regmap_p regs;
	u_char asr, csr;
/*	struct sbic_pending* pendp;*/
	int i;
/*	unsigned tcnt;*/

	SBIC_TRACE(dev);
	regs = &dev->sc_sbicp;

	do {
		GET_SBIC_asr (regs, asr);

		DBG(debug_asr = asr);

		GET_SBIC_csr(regs, csr);
		CSR_TRACE('p', csr, asr, dev->target);

		DBG(debug_csr = csr);
		DBG(routine = 2);

		QPRINTF(("poll[0x%x]", csr));

		i = sbicnextstate(dev, csr, asr);

		WAIT_CIP(regs);
		GET_SBIC_asr(regs, asr);
		/* tapes may take a loooong time.. */
		while (asr & SBIC_ASR_BSY){
			if (asr & SBIC_ASR_DBR) {
				printf("sbipoll: Waiting while sbic is "
				    "jammed, CSR:%02x,ASR:%02x\n",
				    csr, asr);
#ifdef DDB
				Debugger();
#endif
				/* SBIC is jammed */
				/* DUNNO which direction */
				/* Try old direction */
				GET_SBIC_data(regs,i);
				GET_SBIC_asr(regs, asr);
				if (asr & SBIC_ASR_DBR) /* Wants us to write */
					SET_SBIC_data(regs,i);
			}
			GET_SBIC_asr(regs, asr);
		}

		if (asr & SBIC_ASR_LCI)
			printf("sbicpoll: LCI asr:%02x csr:%02x\n", asr, csr);
		else if (i == 1) /* BSY */
			SBIC_WAIT(regs, SBIC_ASR_INT, sbic_cmd_wait);
	} while (i == SBIC_STATE_RUNNING);
	CSR_TRACE('p', csr, asr, i << 4);
	SBIC_TRACE(dev);
	return 1;
}

/*
 * Handle a single msgin
 */

static int
sbicmsgin(struct sbic_softc *dev)
{
	sbic_regmap_p regs;
	int recvlen;
	u_char asr, csr, *tmpaddr;

	regs = &dev->sc_sbicp;

	dev->sc_msg[0] = 0xff;
	dev->sc_msg[1] = 0xff;

	GET_SBIC_asr(regs, asr);

	DBGPRINTF(("sbicmsgin asr=%02x\n", asr), reselect_debug > 1);

	sbic_save_ptrs(dev, regs);

	GET_SBIC_selid (regs, csr);
	SET_SBIC_selid (regs, csr | SBIC_SID_FROM_SCSI);

	SBIC_TC_PUT(regs, 0);
	tmpaddr = dev->sc_msg;
	recvlen = 1;
	do {
		while (recvlen--) {
			asr = GET_SBIC_asr(regs, asr);
			GET_SBIC_csr(regs, csr);
			QPRINTF(("sbicmsgin ready to go (csr,asr)=(%02x,%02x)\n",
				 csr, asr));

			RECV_BYTE(regs, *tmpaddr);
			CSR_TRACE('m', csr, asr, *tmpaddr);
#if 1
			/*
			 * get the command completion interrupt, or we
			 * can't send a new command (LCI)
			 */
			SBIC_WAIT(regs, SBIC_ASR_INT, 0);
			GET_SBIC_csr(regs, csr);
			CSR_TRACE('X', csr, asr, dev->target);
#else
			WAIT_CIP(regs);
			do {
				GET_SBIC_asr(regs, asr);
				csr = 0xff;
				GET_SBIC_csr(regs, csr);
				CSR_TRACE('X', csr, asr, dev->target);
				if (csr == 0xff)
					printf("sbicmsgin waiting: csr %02x "
					    "asr %02x\n", csr, asr);
			} while (csr == 0xff);
#endif

			DBGPRINTF(("sbicmsgin: got %02x csr %02x asr %02x\n",
			    *tmpaddr, csr, asr), reselect_debug > 1);

#if do_parity_check
			if (asr & SBIC_ASR_PE) {
				printf("Parity error");
				/* This code simply does not work. */
				WAIT_CIP(regs);
				SET_SBIC_cmd(regs, SBIC_CMD_SET_ATN);
				WAIT_CIP(regs);
				GET_SBIC_asr(regs, asr);
				WAIT_CIP(regs);
				SET_SBIC_cmd(regs, SBIC_CMD_CLR_ACK);
				WAIT_CIP(regs);
				if (!(asr & SBIC_ASR_LCI))
					/* Target wants to send garbled msg*/
					continue;
				printf("--fixing\n");
				/* loop until a msgout phase occurs on
				   target */
				while ((csr & 0x07) != MESG_OUT_PHASE) {
					while ((asr & SBIC_ASR_BSY) &&
					    !(asr &
						(SBIC_ASR_DBR | SBIC_ASR_INT)))
						GET_SBIC_asr(regs, asr);
					if (asr & SBIC_ASR_DBR)
						panic("msgin: jammed again!\n");
					GET_SBIC_csr(regs, csr);
					CSR_TRACE('e', csr, asr, dev->target);
					if ((csr & 0x07) != MESG_OUT_PHASE) {
						sbicnextstate(dev, csr, asr);
						sbic_save_ptrs(dev, regs);
					}
				}
				/* Should be msg out by now */
				SEND_BYTE(regs, MSG_PARITY_ERROR);
			}
			else
#endif
				tmpaddr++;

			if (recvlen) {
				/* Clear ACK */
				WAIT_CIP(regs);
				GET_SBIC_asr(regs, asr);
				GET_SBIC_csr(regs, csr);
				CSR_TRACE('X',csr,asr,dev->target);
				QPRINTF(("sbicmsgin pre byte CLR_ACK (csr,asr)=(%02x,%02x)\n",
					 csr, asr));
				SET_SBIC_cmd(regs, SBIC_CMD_CLR_ACK);
				SBIC_WAIT(regs, SBIC_ASR_INT, 0);
			}

		};

		if (dev->sc_msg[0] == 0xff) {
			printf("sbicmsgin: sbic swallowed our message\n");
			break;
		}

		DBGPRINTF(("msgin done csr 0x%x asr 0x%x msg 0x%x\n",
		    csr, asr, dev->sc_msg[0]), sync_debug);

		/*
		 * test whether this is a reply to our sync
		 * request
		 */
		if (MSG_ISIDENTIFY(dev->sc_msg[0])) {
			QPRINTF(("IFFY"));
			/* Got IFFY msg -- ack it */
		} else if (dev->sc_msg[0] == MSG_REJECT
			   && dev->sc_sync[dev->target].state == SYNC_SENT) {
			QPRINTF(("REJECT of SYN"));

			DBGPRINTF(("target %d rejected sync, going async\n",
			    dev->target), sync_debug);

			dev->sc_sync[dev->target].period = sbic_min_period;
			dev->sc_sync[dev->target].offset = 0;
			dev->sc_sync[dev->target].state = SYNC_DONE;
			SET_SBIC_syn(regs,
				     SBIC_SYN(dev->sc_sync[dev->target].offset,
					      dev->sc_sync[dev->target].period));
		} else if ((dev->sc_msg[0] == MSG_REJECT)) {
			QPRINTF(("REJECT"));
			/*
			 * we'll never REJECt a REJECT message..
			 */
		} else if ((dev->sc_msg[0] == MSG_SAVE_DATA_PTR)) {
			QPRINTF(("MSG_SAVE_DATA_PTR"));
			/*
			 * don't reject this either.
			 */
		} else if ((dev->sc_msg[0] == MSG_DISCONNECT)) {
			QPRINTF(("DISCONNECT"));

			DBGPRINTF(("sbicmsgin: got disconnect msg %s\n",
			    (dev->sc_flags & SBICF_ICMD) ? "rejecting" : ""),
			    reselect_debug > 1 &&
			    dev->sc_msg[0] == MSG_DISCONNECT);

			if (dev->sc_flags & SBICF_ICMD) {
				/* We're in immediate mode. Prevent
                                   disconnects. */
				/* prepare to reject the message, NACK */
				SET_SBIC_cmd(regs, SBIC_CMD_SET_ATN);
				WAIT_CIP(regs);
			}
		} else if (dev->sc_msg[0] == MSG_CMD_COMPLETE) {
			QPRINTF(("CMD_COMPLETE"));
			/* !! KLUDGE ALERT !! quite a few drives don't seem to
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

			DBGPRINTF(("GOT MSG %d! target %d acting weird.."
			    " waiting for disconnect...\n",
			    dev->sc_msg[0], dev->target), sync_debug);

			/* Check to see if sbic is handling this */
			GET_SBIC_asr(regs, asr);
			if (asr & SBIC_ASR_BSY)
				return SBIC_STATE_RUNNING;

			/* Let's try this: Assume it works and set
                           status to 00 */
			dev->sc_stat[0] = 0;
		} else if (dev->sc_msg[0] == MSG_EXT_MESSAGE
			   && tmpaddr == &dev->sc_msg[1]) {
			QPRINTF(("ExtMSG\n"));
			/* Read in whole extended message */
			SET_SBIC_cmd(regs, SBIC_CMD_CLR_ACK);
			SBIC_WAIT(regs, SBIC_ASR_INT, 0);
			GET_SBIC_asr(regs, asr);
			GET_SBIC_csr(regs, csr);
			QPRINTF(("CLR ACK asr %02x, csr %02x\n", asr, csr));
			RECV_BYTE(regs, *tmpaddr);
			CSR_TRACE('x',csr,asr,*tmpaddr);
			/* Wait for command completion IRQ */
			SBIC_WAIT(regs, SBIC_ASR_INT, 0);
			recvlen = *tmpaddr++;
			QPRINTF(("Recving ext msg, asr %02x csr %02x len %02x\n",
			       asr, csr, recvlen));
		} else if (dev->sc_msg[0] == MSG_EXT_MESSAGE &&
		    dev->sc_msg[1] == 3 &&
		    dev->sc_msg[2] == MSG_SYNC_REQ) {
			QPRINTF(("SYN"));
			dev->sc_sync[dev->target].period =
				sbicfromscsiperiod(dev,
						   regs, dev->sc_msg[3]);
			dev->sc_sync[dev->target].offset = dev->sc_msg[4];
			dev->sc_sync[dev->target].state = SYNC_DONE;
			SET_SBIC_syn(regs,
				     SBIC_SYN(dev->sc_sync[dev->target].offset,
					      dev->sc_sync[dev->target].period));
			printf("%s: target %d now synchronous,"
			       " period=%dns, offset=%d.\n",
			       dev->sc_dev.dv_xname, dev->target,
			       dev->sc_msg[3] * 4, dev->sc_msg[4]);
		} else {

			DBGPRINTF(("sbicmsgin: Rejecting message 0x%02x\n",
			    dev->sc_msg[0]), sbic_debug || sync_debug);

			/* prepare to reject the message, NACK */
			SET_SBIC_cmd(regs, SBIC_CMD_SET_ATN);
			WAIT_CIP(regs);
		}
		/* Clear ACK */
		WAIT_CIP(regs);
		GET_SBIC_asr(regs, asr);
		GET_SBIC_csr(regs, csr);
		CSR_TRACE('X',csr,asr,dev->target);
		QPRINTF(("sbicmsgin pre CLR_ACK (csr,asr)=(%02x,%02x)%d\n",
			 csr, asr, recvlen));
		SET_SBIC_cmd(regs, SBIC_CMD_CLR_ACK);
		SBIC_WAIT(regs, SBIC_ASR_INT, 0);
	}
#if 0
	while ((csr == SBIC_CSR_MSGIN_W_ACK) ||
	    (SBIC_PHASE(csr) == MESG_IN_PHASE));
#else
	while (recvlen > 0);
#endif

	QPRINTF(("sbicmsgin finished: csr %02x, asr %02x\n",csr, asr));

	/* Should still have one CSR to read */
	return SBIC_STATE_RUNNING;
}


/*
 * sbicnextstate()
 * return:
 *		0  == done
 *		1  == working
 *		2  == disconnected
 *		-1 == error
 */
static int
sbicnextstate(struct sbic_softc *dev, u_char csr, u_char asr)
{
	sbic_regmap_p regs;
	struct sbic_acb *acb;
/*	int i;*/
	int newtarget, newlun, wait;
/*	unsigned tcnt;*/

	SBIC_TRACE(dev);
	regs = &dev->sc_sbicp;
	acb = dev->sc_nexus;

	QPRINTF(("next[%02x,%02x]",asr,csr));

	switch (csr) {
	case SBIC_CSR_XFERRED | CMD_PHASE:
	case SBIC_CSR_MIS     | CMD_PHASE:
	case SBIC_CSR_MIS_1   | CMD_PHASE:
	case SBIC_CSR_MIS_2   | CMD_PHASE:
		sbic_save_ptrs(dev, regs);
		if (sbicxfstart(regs, acb->clen, CMD_PHASE, sbic_cmd_wait))
			if (sbicxfout(regs, acb->clen,
				      &acb->cmd, CMD_PHASE))
				goto abort;
		break;

	case SBIC_CSR_XFERRED | STATUS_PHASE:
	case SBIC_CSR_MIS     | STATUS_PHASE:
	case SBIC_CSR_MIS_1   | STATUS_PHASE:
	case SBIC_CSR_MIS_2   | STATUS_PHASE:
		/*
		 * this should be the normal i/o completion case.
		 * get the status & cmd complete msg then let the
		 * device driver look at what happened.
		 */
		sbicxfdone(dev,regs,dev->target);

		if (acb->flags & ACB_DMA) {
			DBG(dev->sc_dmatimo = 0);

			dev->sc_dmafinish(dev->sc_dmah, dev->sc_dmat, acb);

			dev->sc_flags &= ~SBICF_INDMA;
		}
		sbic_scsidone(acb, dev->sc_stat[0]);
		SBIC_TRACE(dev);
		return SBIC_STATE_DONE;

	case SBIC_CSR_XFERRED | DATA_OUT_PHASE:
	case SBIC_CSR_XFERRED | DATA_IN_PHASE:
	case SBIC_CSR_MIS     | DATA_OUT_PHASE:
	case SBIC_CSR_MIS     | DATA_IN_PHASE:
	case SBIC_CSR_MIS_1   | DATA_OUT_PHASE:
	case SBIC_CSR_MIS_1   | DATA_IN_PHASE:
	case SBIC_CSR_MIS_2   | DATA_OUT_PHASE:
	case SBIC_CSR_MIS_2   | DATA_IN_PHASE:
	{
		int i = 0;

		if ((acb->xs->xs_control & XS_CTL_POLL) ||
		    (dev->sc_flags & SBICF_ICMD) ||
		    (acb->flags & ACB_DMA) == 0) {
			/* Do PIO */
			SET_SBIC_control(regs, SBIC_CTL_EDI | SBIC_CTL_IDI);
			if (acb->datalen <= 0) {
				printf("sbicnextstate:xfer count %d asr%x csr%x\n",
				    acb->datalen, asr, csr);
				goto abort;
			}
			wait = sbic_data_wait;
			if (sbicxfstart(regs, acb->datalen,
			    SBIC_PHASE(csr), wait)) {
				if (SBIC_PHASE(csr) == DATA_IN_PHASE)
					/* data in? */
					i = sbicxfin(regs, acb->datalen,
					    acb->data);
				else
					i = sbicxfout(regs, acb->datalen,
					    acb->data, SBIC_PHASE(csr));
			}
			acb->data += acb->datalen - i;
			acb->datalen = i;
		} else {
			/* Transfer = using DMA */
			/*
			 * do scatter-gather dma
			 * hacking the controller chip, ouch..
			 */
			SET_SBIC_control(regs,
			    SBIC_CTL_EDI | SBIC_CTL_IDI | dev->sc_dmamode);
			/*
			 * set next dma addr and dec count
			 */
			sbic_save_ptrs(dev, regs);

			if (acb->offset >= acb->datalen) {
				printf("sbicnextstate:xfer offset %d asr%x csr%x\n",
				    acb->offset, asr, csr);
				goto abort;
			}
			DBGPRINTF(("next dmanext: %d(offset %d)\n",
			    dev->target, acb->offset),
			    data_pointer_debug > 1);
			DBG(dev->sc_dmatimo = 1);

			acb->sc_tcnt =
			    dev->sc_dmanext(dev->sc_dmah, dev->sc_dmat,
				acb, acb->offset);
			DBGPRINTF(("dmanext transfering %ld bytes\n",
			    acb->sc_tcnt), data_pointer_debug);
			SBIC_TC_PUT(regs, (unsigned)acb->sc_tcnt);
			SET_SBIC_cmd(regs, SBIC_CMD_XFER_INFO);
			dev->sc_flags |= SBICF_INDMA;
		}
		break;
	}
	case SBIC_CSR_XFERRED | MESG_IN_PHASE:
	case SBIC_CSR_MIS     | MESG_IN_PHASE:
	case SBIC_CSR_MIS_1   | MESG_IN_PHASE:
	case SBIC_CSR_MIS_2   | MESG_IN_PHASE:
		SBIC_TRACE(dev);
		return sbicmsgin(dev);

	case SBIC_CSR_MSGIN_W_ACK:
		/* Dunno what I'm ACKing */
		SET_SBIC_cmd(regs, SBIC_CMD_CLR_ACK);
		printf("Acking unknown msgin CSR:%02x",csr);
		break;

	case SBIC_CSR_XFERRED | MESG_OUT_PHASE:
	case SBIC_CSR_MIS     | MESG_OUT_PHASE:
	case SBIC_CSR_MIS_1   | MESG_OUT_PHASE:
	case SBIC_CSR_MIS_2   | MESG_OUT_PHASE:

		DBGPRINTF(("sending REJECT msg to last msg.\n"), sync_debug);

		sbic_save_ptrs(dev, regs);
		/*
		 * Should only get here on reject, since it's always
		 * US that initiate a sync transfer.
		 */
		SEND_BYTE(regs, MSG_REJECT);
		WAIT_CIP(regs);
		if (asr & (SBIC_ASR_BSY | SBIC_ASR_LCI | SBIC_ASR_CIP))
			printf("next: REJECT sent asr %02x\n", asr);
		SBIC_TRACE(dev);
		return SBIC_STATE_RUNNING;

	case SBIC_CSR_DISC:
	case SBIC_CSR_DISC_1:
		dev->sc_flags &= ~(SBICF_INDMA | SBICF_SELECTED);

		/* Try to schedule another target */
		DBGPRINTF(("sbicnext target %d disconnected\n", dev->target),
		    reselect_debug > 1);

		TAILQ_INSERT_HEAD(&dev->nexus_list, acb, chain);
		++dev->sc_tinfo[dev->target].dconns;
		dev->sc_nexus = NULL;

		if ((acb->xs->xs_control & XS_CTL_POLL)
		    || (dev->sc_flags & SBICF_ICMD)
		    || (!sbic_parallel_operations)) {
			SBIC_TRACE(dev);
			return SBIC_STATE_DISCONNECT;
		}
		sbic_sched(dev);
		SBIC_TRACE(dev);
		return SBIC_STATE_DISCONNECT;

	case SBIC_CSR_RSLT_NI:
	case SBIC_CSR_RSLT_IFY:
		GET_SBIC_rselid(regs, newtarget);
		/* check SBIC_RID_SIV? */
		newtarget &= SBIC_RID_MASK;
		if (csr == SBIC_CSR_RSLT_IFY) {
			/* Read IFY msg to avoid lockup */
			GET_SBIC_data(regs, newlun);
			WAIT_CIP(regs);
			newlun &= SBIC_TLUN_MASK;
			CSR_TRACE('r',csr,asr,newtarget);
		} else {
			/* Need to get IFY message */
			for (newlun = 256; newlun; --newlun) {
				GET_SBIC_asr(regs, asr);
				if (asr & SBIC_ASR_INT)
					break;
				delay(1);
			}
			newlun = 0;	/* XXXX */
			if ((asr & SBIC_ASR_INT) == 0) {

				DBGPRINTF(("RSLT_NI - no IFFY message? asr %x\n",
				    asr), reselect_debug);

			} else {
				GET_SBIC_csr(regs,csr);
				CSR_TRACE('n',csr,asr,newtarget);
				if ((csr == (SBIC_CSR_MIS | MESG_IN_PHASE)) ||
				    (csr == (SBIC_CSR_MIS_1 | MESG_IN_PHASE)) ||
				    (csr == (SBIC_CSR_MIS_2 | MESG_IN_PHASE))) {
					sbicmsgin(dev);
					newlun = dev->sc_msg[0] & 7;
				} else {
					printf("RSLT_NI - not MESG_IN_PHASE %x\n",
					    csr);
				}
			}
		}

		DBGPRINTF(("sbicnext: reselect %s from targ %d lun %d\n",
		    csr == SBIC_CSR_RSLT_NI ? "NI" : "IFY",
		    newtarget, newlun),
		    reselect_debug > 1 ||
		    (reselect_debug && csr == SBIC_CSR_RSLT_NI));

		if (dev->sc_nexus) {
			DBGPRINTF(("%s: reselect %s with active command\n",
			    dev->sc_dev.dv_xname,
			    csr == SBIC_CSR_RSLT_NI ? "NI" : "IFY"),
			    reselect_debug > 1);
#if defined(DDB) && defined (DEBUG)
/*			Debugger();*/
#endif

			TAILQ_INSERT_HEAD(&dev->ready_list, dev->sc_nexus,
			    chain);
			dev->sc_tinfo[dev->target].lubusy &= ~(1 << dev->lun);
			dev->sc_nexus = NULL;
		}
		/* Reload sync values for this target */
		if (dev->sc_sync[newtarget].state == SYNC_DONE)
			SET_SBIC_syn(regs,
			    SBIC_SYN(dev->sc_sync[newtarget].offset,
				dev->sc_sync[newtarget].period));
		else
			SET_SBIC_syn(regs, SBIC_SYN (0, sbic_min_period));
		for (acb = dev->nexus_list.tqh_first; acb;
		    acb = acb->chain.tqe_next) {
			if (acb->xs->xs_periph->periph_target != newtarget ||
			    acb->xs->xs_periph->periph_lun != newlun)
				continue;
			TAILQ_REMOVE(&dev->nexus_list, acb, chain);
			dev->sc_nexus = acb;
			dev->sc_flags |= SBICF_SELECTED;
			dev->target = newtarget;
			dev->lun = newlun;
			break;
		}
		if (acb == NULL) {
			printf("%s: reselect %s targ %d not in nexus_list %p\n",
			    dev->sc_dev.dv_xname,
			    csr == SBIC_CSR_RSLT_NI ? "NI" : "IFY", newtarget,
			    &dev->nexus_list.tqh_first);
			panic("bad reselect in sbic");
		}
		if (csr == SBIC_CSR_RSLT_IFY)
			SET_SBIC_cmd(regs, SBIC_CMD_CLR_ACK);
		break;

	default:
        abort:
		/*
		 * Something unexpected happened -- deal with it.
		 */
		printf("sbicnextstate: aborting csr %02x asr %02x\n", csr,
		    asr);
#ifdef DDB
		Debugger();
#endif
		DBG(dev->sc_dmatimo = 0);

		if (dev->sc_flags & SBICF_INDMA) {
			dev->sc_dmafinish(dev->sc_dmah, dev->sc_dmat, acb);
			dev->sc_flags &= ~SBICF_INDMA;
			DBG(dev->sc_dmatimo = 0);
		}
		SET_SBIC_control(regs, SBIC_CTL_EDI | SBIC_CTL_IDI);
		sbicerror(dev, regs, csr);
		sbicabort(dev, regs, "next");
		sbic_scsidone(acb, -1);
		SBIC_TRACE(dev);
                return SBIC_STATE_ERROR;
	}

	SBIC_TRACE(dev);
	return SBIC_STATE_RUNNING;
}

static int
sbictoscsiperiod(struct sbic_softc *dev, sbic_regmap_p regs, int a)
{
	unsigned int fs;

	/*
	 * cycle = DIV / (2*CLK)
	 * DIV = FS+2
	 * best we can do is 200ns at 20Mhz, 2 cycles
	 */

	GET_SBIC_myid(regs,fs);
	fs = (fs >> 6) + 2;		/* DIV */
	fs = (fs * 10000) / (dev->sc_clkfreq << 1);	/* Cycle, in ns */
	if (a < 2)
		a = 8;		/* map to Cycles */
	return (fs * a) >> 2;		/* in 4 ns units */
}

static int
sbicfromscsiperiod(struct sbic_softc *dev, sbic_regmap_p regs, int p)
{
	register unsigned int fs, ret;

	/* Just the inverse of the above */

	GET_SBIC_myid(regs, fs);
	fs = (fs >> 6) + 2;		/* DIV */
	fs = (fs * 10000) / (dev->sc_clkfreq << 1);   /* Cycle, in ns */

	ret = p << 2;			/* in ns units */
	ret = ret / fs;			/* in Cycles */
	if (ret < sbic_min_period)
		return sbic_min_period;

	/* verify rounding */
	if (sbictoscsiperiod(dev, regs, ret) < p)
		ret++;
	return (ret >= 8) ? 0 : ret;
}

#ifdef DEBUG

void
sbicdumpstate()
{
	u_char csr, asr;

	GET_SBIC_asr(debug_sbic_regs,asr);
	GET_SBIC_csr(debug_sbic_regs,csr);
	printf("%s: asr:csr(%02x:%02x)->(%02x:%02x)\n",
	    (routine == 1) ? "sbicgo" :
	    (routine == 2) ? "sbicintr" :
	    (routine == 3) ? "sbicicmd" :
	    (routine == 4) ? "sbicnext" : "unknown",
	    debug_asr, debug_csr, asr, csr);

}

void
sbictimeout(struct sbic_softc *dev)
{
	int s, asr;

	s = splbio();
	if (dev->sc_dmatimo) {
		if (dev->sc_dmatimo > 1) {
			printf("%s: dma timeout #%d\n",
			    dev->sc_dev.dv_xname, dev->sc_dmatimo - 1);
			GET_SBIC_asr(&dev->sc_sbicp, asr);
			if (asr & SBIC_ASR_INT) {
				/* We need to service a missed IRQ */
				printf("Servicing a missed int:(%02x,%02x)->(%02x,??)\n",
				    debug_asr, debug_csr, asr);
				sbicintr(dev);
			}
			sbicdumpstate();
		}
		dev->sc_dmatimo++;
	}
	splx(s);
	callout_reset(&dev->sc_timo_ch, 30 * hz,
	    (void *)sbictimeout, dev);
}

void
sbic_dump_acb(struct sbic_acb *acb)
{
	u_char *b = (u_char *) &acb->cmd;
	int i;

	printf("acb@%p ", acb);
	if (acb->xs == NULL) {
		printf("<unused>\n");
		return;
	}
	printf("(%d:%d) flags %2x clen %2d cmd ",
	    acb->xs->xs_periph->periph_target,
	    acb->xs->xs_periph->periph_lun, acb->flags, acb->clen);
	for (i = acb->clen; i; --i)
		printf(" %02x", *b++);
	printf("\n");
	printf("  xs: %8p data %8p:%04x ", acb->xs, acb->xs->data,
	    acb->xs->datalen);
	printf("tcnt %lx\n", acb->sc_tcnt);
}

void
sbic_dump(struct sbic_softc *dev)
{
	sbic_regmap_p regs;
	u_char csr, asr;
	struct sbic_acb *acb;
	int s;
	int i;

	s = splbio();
	regs = &dev->sc_sbicp;
#if CSR_TRACE_SIZE
	printf("csr trace: ");
	i = csr_traceptr;
	do {
		printf("%c%02x%02x%02x ", csr_trace[i].whr,
		    csr_trace[i].csr, csr_trace[i].asr, csr_trace[i].xtn);
		switch(csr_trace[i].whr) {
		case 'g':
			printf("go "); break;
		case 's':
			printf("select "); break;
		case 'y':
			printf("select+ "); break;
		case 'i':
			printf("intr "); break;
		case 'f':
			printf("finish "); break;
		case '>':
			printf("out "); break;
		case '<':
			printf("in "); break;
		case 'm':
			printf("msgin "); break;
		case 'x':
			printf("msginx "); break;
		case 'X':
			printf("msginX "); break;
		case 'r':
			printf("reselect "); break;
		case 'I':
			printf("icmd "); break;
		case 'a':
			printf("abort "); break;
		default:
			printf("? ");
		}
		switch(csr_trace[i].csr) {
		case 0x11:
			printf("INITIATOR"); break;
		case 0x16:
			printf("S_XFERRED"); break;
		case 0x20:
			printf("MSGIN_ACK"); break;
		case 0x41:
			printf("DISC"); break;
		case 0x42:
			printf("SEL_TIMEO"); break;
		case 0x80:
			printf("RSLT_NI"); break;
		case 0x81:
			printf("RSLT_IFY"); break;
		case 0x85:
			printf("DISC_1"); break;
		case 0x18: case 0x19: case 0x1a:
		case 0x1b: case 0x1e: case 0x1f:
		case 0x28: case 0x29: case 0x2a:
		case 0x2b: case 0x2e: case 0x2f:
		case 0x48: case 0x49: case 0x4a:
		case 0x4b: case 0x4e: case 0x4f:
		case 0x88: case 0x89: case 0x8a:
		case 0x8b: case 0x8e: case 0x8f:
			switch(csr_trace[i].csr & 0xf0) {
			case 0x10:
				printf("DONE_"); break;
			case 0x20:
				printf("STOP_"); break;
			case 0x40:
				printf("ERR_"); break;
			case 0x80:
				printf("REQ_"); break;
			}
			switch(csr_trace[i].csr & 7) {
			case 0:
				printf("DATA_OUT"); break;
			case 1:
				printf("DATA_IN"); break;
			case 2:
				printf("CMD"); break;
			case 3:
				printf("STATUS"); break;
			case 6:
				printf("MSG_OUT"); break;
			case 7:
				printf("MSG_IN"); break;
			default:
				printf("invld phs");
			}
			break;
		default:    printf("****"); break;
		}
		if (csr_trace[i].asr & SBIC_ASR_INT)
			printf(" ASR_INT");
		if (csr_trace[i].asr & SBIC_ASR_LCI)
			printf(" ASR_LCI");
		if (csr_trace[i].asr & SBIC_ASR_BSY)
			printf(" ASR_BSY");
		if (csr_trace[i].asr & SBIC_ASR_CIP)
			printf(" ASR_CIP");
		printf("\n");
		i = (i + 1) & (CSR_TRACE_SIZE - 1);
	} while (i != csr_traceptr);
#endif
	GET_SBIC_asr(regs, asr);
	if ((asr & SBIC_ASR_INT) == 0)
		GET_SBIC_csr(regs, csr);
	else
		csr = 0;
	printf("%s@%p regs %p asr %x csr %x\n", dev->sc_dev.dv_xname,
	    dev, regs, asr, csr);
	if ((acb = dev->free_list.tqh_first)) {
		printf("Free list:\n");
		while (acb) {
			sbic_dump_acb(acb);
			acb = acb->chain.tqe_next;
		}
	}
	if ((acb = dev->ready_list.tqh_first)) {
		printf("Ready list:\n");
		while (acb) {
			sbic_dump_acb(acb);
			acb = acb->chain.tqe_next;
		}
	}
	if ((acb = dev->nexus_list.tqh_first)) {
		printf("Nexus list:\n");
		while (acb) {
			sbic_dump_acb(acb);
			acb = acb->chain.tqe_next;
		}
	}
	if (dev->sc_nexus) {
		printf("nexus:\n");
		sbic_dump_acb(dev->sc_nexus);
	}
	printf("targ %d lun %d flags %x\n",
	    dev->target, dev->lun, dev->sc_flags);
	for (i = 0; i < 8; ++i) {
		if (dev->sc_tinfo[i].cmds > 2) {
			printf("tgt %d: cmds %d disc %d lubusy %x\n",
			    i, dev->sc_tinfo[i].cmds,
			    dev->sc_tinfo[i].dconns,
			    dev->sc_tinfo[i].lubusy);
		}
	}
	splx(s);
}

#endif
