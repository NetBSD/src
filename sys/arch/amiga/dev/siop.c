/*	$NetBSD: siop.c,v 1.68 2014/01/22 00:25:16 christos Exp $ */

/*
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
 * 3. Neither the name of the University nor the names of its contributors
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
 *	@(#)siop.c	7.5 (Berkeley) 5/4/91
 */

/*
 * Copyright (c) 1994 Michael L. Hitch
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
 *	@(#)siop.c	7.5 (Berkeley) 5/4/91
 */

/*
 * AMIGA 53C710 scsi adaptor driver
 */

#include "opt_ddb.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: siop.c,v 1.68 2014/01/22 00:25:16 christos Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/disklabel.h>
#include <sys/buf.h>
#include <sys/malloc.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>
#include <machine/cpu.h>
#ifdef __m68k__
#include <m68k/cacheops.h>
#else
#define DCIAS(pa)
#endif
#include <amiga/amiga/custom.h>
#include <amiga/amiga/isr.h>
#include <amiga/dev/siopreg.h>
#include <amiga/dev/siopvar.h>

/*
 * SCSI delays
 * In u-seconds, primarily for state changes on the SPC.
 */
#define	SCSI_CMD_WAIT	500000	/* wait per step of 'immediate' cmds */
#define	SCSI_DATA_WAIT	500000	/* wait per data in/out step */
#define	SCSI_INIT_WAIT	500000	/* wait per step (both) during init */

void siop_select(struct siop_softc *);
void siopabort(struct siop_softc *, siop_regmap_p, const char *);
void sioperror(struct siop_softc *, siop_regmap_p, u_char);
void siopstart(struct siop_softc *);
int  siop_checkintr(struct siop_softc *, u_char, u_char, u_char, int *);
void siopreset(struct siop_softc *);
void siopsetdelay(int);
void siop_scsidone(struct siop_acb *, int);
void siop_timeout(void *);
void siop_sched(struct siop_softc *);
void siop_poll(struct siop_softc *, struct siop_acb *);
void siopintr(struct siop_softc *);
void scsi_period_to_siop(struct siop_softc *, int);
void siop_start(struct siop_softc *, int, int, u_char *, int, u_char *, int);
void siop_dump_acb(struct siop_acb *);

/* 53C710 script */
const
#include <amiga/dev/siop_script.out>

/* default to not inhibit sync negotiation on any drive */
u_char siop_inhibit_sync[8] = { 0, 0, 0, 0, 0, 0, 0 }; /* initialize, so patchable */
u_char siop_allow_disc[8] = {3, 3, 3, 3, 3, 3, 3, 3};
int siop_no_dma = 0;

int siop_reset_delay = 250;	/* delay after reset, in milleseconds */

int siop_cmd_wait = SCSI_CMD_WAIT;
int siop_data_wait = SCSI_DATA_WAIT;
int siop_init_wait = SCSI_INIT_WAIT;

#ifdef DEBUG_SYNC
/*
 * sync period transfer lookup - only valid for 66 MHz clock
 */
static struct {
	unsigned char p;	/* period from sync request message */
	unsigned char r;	/* siop_period << 4 | sbcl */
} sync_tab[] = {
	{ 60/4, 0<<4 | 1},
	{ 76/4, 1<<4 | 1},
	{ 92/4, 2<<4 | 1},
	{ 92/4, 0<<4 | 2},
	{108/4, 3<<4 | 1},
	{116/4, 1<<4 | 2},
	{120/4, 4<<4 | 1},
	{120/4, 0<<4 | 3},
	{136/4, 5<<4 | 1},
	{140/4, 2<<4 | 2},
	{152/4, 6<<4 | 1},
	{152/4, 1<<4 | 3},
	{164/4, 3<<4 | 2},
	{168/4, 7<<4 | 1},
	{180/4, 2<<4 | 3},
	{184/4, 4<<4 | 2},
	{208/4, 5<<4 | 2},
	{212/4, 3<<4 | 3},
	{232/4, 6<<4 | 2},
	{240/4, 4<<4 | 3},
	{256/4, 7<<4 | 2},
	{272/4, 5<<4 | 3},
	{300/4, 6<<4 | 3},
	{332/4, 7<<4 | 3}
};
#endif

#ifdef DEBUG
/*
 *	0x01 - full debug
 *	0x02 - DMA chaining
 *	0x04 - siopintr
 *	0x08 - phase mismatch
 *	0x10 - <not used>
 *	0x20 - panic on unhandled exceptions
 *	0x100 - disconnect/reselect
 */
int	siop_debug = 0;
int	siopsync_debug = 0;
int	siopdma_hits = 0;
int	siopdma_misses = 0;
int	siopchain_ints = 0;
int	siopstarts = 0;
int	siopints = 0;
int	siopphmm = 0;
#define SIOP_TRACE_SIZE	128
#define SIOP_TRACE(a,b,c,d) \
	siop_trbuf[siop_trix] = (a); \
	siop_trbuf[siop_trix+1] = (b); \
	siop_trbuf[siop_trix+2] = (c); \
	siop_trbuf[siop_trix+3] = (d); \
	siop_trix = (siop_trix + 4) & (SIOP_TRACE_SIZE - 1);
u_char	siop_trbuf[SIOP_TRACE_SIZE];
int	siop_trix;
void siop_dump(struct siop_softc *);
void siop_dump_trace(void);
#else
#define SIOP_TRACE(a,b,c,d)
#endif


/*
 * default minphys routine for siop based controllers
 */
void
siop_minphys(struct buf *bp)
{

	/*
	 * No max transfer at this level.
	 */
	minphys(bp);
}

/*
 * used by specific siop controller
 *
 */
void
siop_scsipi_request(struct scsipi_channel *chan, scsipi_adapter_req_t req,
                    void *arg)
{
	struct scsipi_xfer *xs;
#ifdef DIAGNOSTIC
	struct scsipi_periph *periph;
#endif
	struct siop_acb *acb;
	struct siop_softc *sc = device_private(chan->chan_adapter->adapt_dev);
	int flags, s;

	switch (req) {
	case ADAPTER_REQ_RUN_XFER:
		xs = arg;
#ifdef DIAGNOSTIC
		periph = xs->xs_periph;
#endif
		flags = xs->xs_control;

		/* XXXX ?? */
		if (flags & XS_CTL_DATA_UIO)
			panic("siop: scsi data uio requested");

		/* XXXX ?? */
		if (sc->sc_nexus && flags & XS_CTL_POLL)
/*			panic("siop_scsicmd: busy");*/
			printf("siop_scsicmd: busy\n");

		s = splbio();
		acb = sc->free_list.tqh_first;
		if (acb) {
			TAILQ_REMOVE(&sc->free_list, acb, chain);
		}
		splx(s);

#ifdef DIAGNOSTIC
		/*
		 * This should never happen as we track the resources
		 * in the mid-layer.
		 */
		if (acb == NULL) {
			scsipi_printaddr(periph);
			printf("unable to allocate acb\n");
			panic("siop_scsipi_request");
		}
#endif

		acb->flags = ACB_ACTIVE;
		acb->xs = xs;
		memcpy(&acb->cmd, xs->cmd, xs->cmdlen);
		acb->clen = xs->cmdlen;
		acb->daddr = xs->data;
		acb->dleft = xs->datalen;

		s = splbio();
		TAILQ_INSERT_TAIL(&sc->ready_list, acb, chain);

		if (sc->sc_nexus == NULL)
			siop_sched(sc);

		splx(s);

		if (flags & XS_CTL_POLL || siop_no_dma)
			siop_poll(sc, acb);
		return;

	case ADAPTER_REQ_GROW_RESOURCES:
		return;

	case ADAPTER_REQ_SET_XFER_MODE:
		return;
	}
}

void
siop_poll(struct siop_softc *sc, struct siop_acb *acb)
{
	siop_regmap_p rp = sc->sc_siopp;
	struct scsipi_xfer *xs = acb->xs;
	int i;
	int status;
	u_char istat;
	u_char dstat;
	u_char sstat0;
	int s;
	int to;

	s = splbio();
	to = xs->timeout / 1000;
	if (sc->nexus_list.tqh_first)
		printf("%s: siop_poll called with disconnected device\n",
		    device_xname(sc->sc_dev));
	for (;;) {
		/* use cmd_wait values? */
		i = 50000;
		/* XXX spl0(); */
		while (((istat = rp->siop_istat) &
		    (SIOP_ISTAT_SIP | SIOP_ISTAT_DIP)) == 0) {
			if (--i <= 0) {
#ifdef DEBUG
				printf ("waiting: tgt %d cmd %02x sbcl %02x dsp %lx (+%lx) dcmd %lx ds %p timeout %d\n",
				    xs->xs_periph->periph_target, acb->cmd.opcode,
				    rp->siop_sbcl, rp->siop_dsp,
				    rp->siop_dsp - sc->sc_scriptspa,
				    *((volatile long *)&rp->siop_dcmd), &acb->ds, acb->xs->timeout);
#endif
				i = 50000;
				--to;
				if (to <= 0) {
					siopreset(sc);
					return;
				}
			}
			delay(20);
		}
		sstat0 = rp->siop_sstat0;
		dstat = rp->siop_dstat;
		if (siop_checkintr(sc, istat, dstat, sstat0, &status)) {
			if (acb != sc->sc_nexus)
				printf("%s: siop_poll disconnected device completed\n",
				    device_xname(sc->sc_dev));
			else if ((sc->sc_flags & SIOP_INTDEFER) == 0) {
				sc->sc_flags &= ~SIOP_INTSOFF;
				rp->siop_sien = sc->sc_sien;
				rp->siop_dien = sc->sc_dien;
			}
			siop_scsidone(sc->sc_nexus, status);
		}

		if (xs->xs_status & XS_STS_DONE)
			break;
	}
	splx(s);
}

/*
 * start next command that's ready
 */
void
siop_sched(struct siop_softc *sc)
{
	struct scsipi_periph *periph;
	struct siop_acb *acb;
	int i;

#ifdef DEBUG
	if (sc->sc_nexus) {
		printf("%s: siop_sched- nexus %p/%d ready %p/%d\n",
		    device_xname(sc->sc_dev), sc->sc_nexus,
		    sc->sc_nexus->xs->xs_periph->periph_target,
		    sc->ready_list.tqh_first,
		    sc->ready_list.tqh_first->xs->xs_periph->periph_target);
		return;
	}
#endif
	for (acb = sc->ready_list.tqh_first; acb; acb = acb->chain.tqe_next) {
		periph = acb->xs->xs_periph;
		i = periph->periph_target;
		if(!(sc->sc_tinfo[i].lubusy & (1 << periph->periph_lun))) {
			struct siop_tinfo *ti = &sc->sc_tinfo[i];

			TAILQ_REMOVE(&sc->ready_list, acb, chain);
			sc->sc_nexus = acb;
			periph = acb->xs->xs_periph;
			ti = &sc->sc_tinfo[periph->periph_target];
			ti->lubusy |= (1 << periph->periph_lun);
			break;
		}
	}

	if (acb == NULL) {
#ifdef DEBUGXXX
		printf("%s: siop_sched didn't find ready command\n",
		    device_xname(sc->sc_dev));
#endif
		return;
	}

	if (acb->xs->xs_control & XS_CTL_RESET)
		siopreset(sc);

#if 0
	acb->cmd.bytes[0] |= slp->scsipi_scsi.lun << 5;	/* XXXX */
#endif
	++sc->sc_active;
	siop_select(sc);
}

void
siop_scsidone(struct siop_acb *acb, int stat)
{
	struct scsipi_xfer *xs;
	struct scsipi_periph *periph;
	struct siop_softc *sc;
	int dosched = 0;

	if (acb == NULL || (xs = acb->xs) == NULL) {
#ifdef DIAGNOSTIC
		printf("siop_scsidone: NULL acb or scsipi_xfer\n");
#if defined(DEBUG) && defined(DDB)
		Debugger();
#endif
#endif
		return;
	}

	callout_stop(&xs->xs_callout);

	periph = xs->xs_periph;
	sc = device_private(periph->periph_channel->chan_adapter->adapt_dev);

	xs->status = stat;
	xs->resid = 0;		/* XXXX */

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
	if (acb == sc->sc_nexus) {
		sc->sc_nexus = NULL;
		sc->sc_tinfo[periph->periph_target].lubusy &=
			~(1<<periph->periph_lun);
		if (sc->ready_list.tqh_first)
			dosched = 1;	/* start next command */
		--sc->sc_active;
		SIOP_TRACE('d','a',stat,0)
	} else if (sc->ready_list.tqh_last == &acb->chain.tqe_next) {
		TAILQ_REMOVE(&sc->ready_list, acb, chain);
		SIOP_TRACE('d','r',stat,0)
	} else {
		register struct siop_acb *acb2;
		for (acb2 = sc->nexus_list.tqh_first; acb2;
		    acb2 = acb2->chain.tqe_next)
			if (acb2 == acb) {
				TAILQ_REMOVE(&sc->nexus_list, acb, chain);
				sc->sc_tinfo[periph->periph_target].lubusy
					&= ~(1<<periph->periph_lun);
				--sc->sc_active;
				break;
			}
		if (acb2)
			;
		else if (acb->chain.tqe_next) {
			TAILQ_REMOVE(&sc->ready_list, acb, chain);
			--sc->sc_active;
		} else {
			printf("%s: can't find matching acb\n",
			    device_xname(sc->sc_dev));
#ifdef DDB
/*			Debugger(); */
#endif
		}
		SIOP_TRACE('d','n',stat,0);
	}
	/* Put it on the free list. */
	acb->flags = ACB_FREE;
	TAILQ_INSERT_HEAD(&sc->free_list, acb, chain);

	sc->sc_tinfo[periph->periph_target].cmds++;

	scsipi_done(xs);

	if (dosched && sc->sc_nexus == NULL)
		siop_sched(sc);
}

void
siopabort(register struct siop_softc *sc, siop_regmap_p rp, const char *where)
{
#ifdef fix_this
	int i;
#endif

	printf ("%s: abort %s: dstat %02x, sstat0 %02x sbcl %02x\n",
	    device_xname(sc->sc_dev),
	    where, rp->siop_dstat, rp->siop_sstat0, rp->siop_sbcl);

	if (sc->sc_active > 0) {
#ifdef TODO
      SET_SBIC_cmd (rp, SBIC_CMD_ABORT);
      WAIT_CIP (rp);

      GET_SBIC_asr (rp, asr);
      if (asr & (SBIC_ASR_BSY|SBIC_ASR_LCI))
        {
          /* ok, get more drastic.. */

	  SET_SBIC_cmd (rp, SBIC_CMD_RESET);
	  delay(25);
	  SBIC_WAIT(rp, SBIC_ASR_INT, 0);
	  GET_SBIC_csr (rp, csr);       /* clears interrupt also */

          return;
        }

      do
        {
          SBIC_WAIT (rp, SBIC_ASR_INT, 0);
          GET_SBIC_csr (rp, csr);
        }
      while ((csr != SBIC_CSR_DISC) && (csr != SBIC_CSR_DISC_1)
	      && (csr != SBIC_CSR_CMD_INVALID));
#endif

		/* lets just hope it worked.. */
#ifdef fix_this
		for (i = 0; i < 2; ++i) {
			if (sc->sc_iob[i].sc_xs && &sc->sc_iob[i] !=
			    sc->sc_cur) {
				printf ("siopabort: cleanup!\n");
				sc->sc_iob[i].sc_xs = NULL;
			}
		}
#endif	/* fix_this */
/*		sc->sc_active = 0; */
	}
}

void
siopinitialize(struct siop_softc *sc)
{
	int i;
	u_int inhibit_sync;
	extern u_long scsi_nosync;
	extern int shift_nosync;

	/*
	 * Need to check that scripts is on a long word boundary
	 * Also should verify that dev doesn't span non-contiguous
	 * physical pages.
	 */
	sc->sc_scriptspa = kvtop((void *)__UNCONST(scripts));

	/*
	 * malloc sc_acb to ensure that DS is on a long word boundary.
	 */

	sc->sc_acb = malloc(sizeof(struct siop_acb) * SIOP_NACB,
		M_DEVBUF, M_NOWAIT);
	if (sc->sc_acb == NULL)
		panic("siopinitialize: ACB malloc failed!");

	sc->sc_tcp[1] = 1000 / sc->sc_clock_freq;
	sc->sc_tcp[2] = 1500 / sc->sc_clock_freq;
	sc->sc_tcp[3] = 2000 / sc->sc_clock_freq;
	sc->sc_minsync = sc->sc_tcp[1];		/* in 4ns units */
	if (sc->sc_minsync < 25)
		sc->sc_minsync = 25;
	if (sc->sc_clock_freq <= 25) {
		sc->sc_dcntl |= 0x80;		/* SCLK/1 */
		sc->sc_tcp[0] = sc->sc_tcp[1];
	} else if (sc->sc_clock_freq <= 37) {
		sc->sc_dcntl |= 0x40;		/* SCLK/1.5 */
		sc->sc_tcp[0] = sc->sc_tcp[2];
	} else if (sc->sc_clock_freq <= 50) {
		sc->sc_dcntl |= 0x00;		/* SCLK/2 */
		sc->sc_tcp[0] = sc->sc_tcp[3];
	} else {
		sc->sc_dcntl |= 0xc0;		/* SCLK/3 */
		sc->sc_tcp[0] = 3000 / sc->sc_clock_freq;
	}

	if (scsi_nosync) {
		inhibit_sync = (scsi_nosync >> shift_nosync) & 0xff;
		shift_nosync += 8;
#ifdef DEBUG
		if (inhibit_sync)
			printf("%s: Inhibiting synchronous transfer %02x\n",
				device_xname(sc->sc_dev), inhibit_sync);
#endif
		for (i = 0; i < 8; ++i)
			if (inhibit_sync & (1 << i))
				siop_inhibit_sync[i] = 1;
	}

	siopreset (sc);
}

void
siop_timeout(void *arg)
{
	struct siop_acb *acb;
	struct scsipi_periph *periph;
	struct siop_softc *sc;
	int s;

	acb = arg;
	periph = acb->xs->xs_periph;
	sc = device_private(periph->periph_channel->chan_adapter->adapt_dev);
	scsipi_printaddr(periph);
	printf("timed out\n");

	s = splbio();

	acb->xs->error = XS_TIMEOUT;
	siopreset(sc);

	splx(s);
}

void
siopreset(struct siop_softc *sc)
{
	siop_regmap_p rp;
	u_int i, s;
	u_char  dummy;
	struct siop_acb *acb;

	rp = sc->sc_siopp;

	if (sc->sc_flags & SIOP_ALIVE)
		siopabort(sc, rp, "reset");

	printf("%s: ", device_xname(sc->sc_dev));		/* XXXX */

	s = splbio();

	/*
	 * Reset the chip
	 * XXX - is this really needed?
	 */
	rp->siop_istat |= SIOP_ISTAT_ABRT;	/* abort current script */
	rp->siop_istat |= SIOP_ISTAT_RST;		/* reset chip */
	rp->siop_istat &= ~SIOP_ISTAT_RST;
	/*
	 * Reset SCSI bus (do we really want this?)
	 */
	rp->siop_sien = 0;
	rp->siop_scntl1 |= SIOP_SCNTL1_RST;
	delay(1);
	rp->siop_scntl1 &= ~SIOP_SCNTL1_RST;

	/*
	 * Set up various chip parameters
	 */
	rp->siop_scntl0 = SIOP_ARB_FULL | SIOP_SCNTL0_EPC | SIOP_SCNTL0_EPG;
	rp->siop_scntl1 = SIOP_SCNTL1_ESR;
	rp->siop_dcntl = sc->sc_dcntl;
	rp->siop_dmode = 0x80;	/* burst length = 4 */
	rp->siop_sien = 0x00;	/* don't enable interrupts yet */
	rp->siop_dien = 0x00;	/* don't enable interrupts yet */
	rp->siop_scid = 1 << sc->sc_channel.chan_id;
	rp->siop_dwt = 0x00;
	rp->siop_ctest0 |= SIOP_CTEST0_BTD | SIOP_CTEST0_EAN;
	rp->siop_ctest7 |= sc->sc_ctest7;

	/* will need to re-negotiate sync xfers */
	memset(&sc->sc_sync, 0, sizeof (sc->sc_sync));

	i = rp->siop_istat;
	if (i & SIOP_ISTAT_SIP)
		dummy = rp->siop_sstat0;
	if (i & SIOP_ISTAT_DIP)
		dummy = rp->siop_dstat;

	__USE(dummy);
	splx (s);

	delay (siop_reset_delay * 1000);
	printf("siop id %d reset V%d\n", sc->sc_channel.chan_id,
	    rp->siop_ctest8 >> 4);

	if ((sc->sc_flags & SIOP_ALIVE) == 0) {
		TAILQ_INIT(&sc->ready_list);
		TAILQ_INIT(&sc->nexus_list);
		TAILQ_INIT(&sc->free_list);
		sc->sc_nexus = NULL;
		acb = sc->sc_acb;
		memset(acb, 0, sizeof(struct siop_acb) * SIOP_NACB);
		for (i = 0; i < SIOP_NACB; i++) {
			TAILQ_INSERT_TAIL(&sc->free_list, acb, chain);
			acb++;
		}
		memset(sc->sc_tinfo, 0, sizeof(sc->sc_tinfo));
	} else {
		if (sc->sc_nexus != NULL) {
			sc->sc_nexus->xs->error = XS_RESET;
			siop_scsidone(sc->sc_nexus, sc->sc_nexus->stat[0]);
		}
		while ((acb = sc->nexus_list.tqh_first) > 0) {
			acb->xs->error = XS_RESET;
			siop_scsidone(acb, acb->stat[0]);
		}
	}

	sc->sc_flags |= SIOP_ALIVE;
	sc->sc_flags &= ~(SIOP_INTDEFER|SIOP_INTSOFF);
	/* enable SCSI and DMA interrupts */
	sc->sc_sien = SIOP_SIEN_M_A | SIOP_SIEN_STO | /*SIOP_SIEN_SEL |*/ SIOP_SIEN_SGE |
	    SIOP_SIEN_UDC | SIOP_SIEN_RST | SIOP_SIEN_PAR;
	sc->sc_dien = SIOP_DIEN_BF | SIOP_DIEN_ABRT | SIOP_DIEN_SIR |
	    /*SIOP_DIEN_WTD |*/ SIOP_DIEN_IID;
	rp->siop_sien = sc->sc_sien;
	rp->siop_dien = sc->sc_dien;
}

/*
 * Setup Data Storage for 53C710 and start SCRIPTS processing
 */

void
siop_start(struct siop_softc *sc, int target, int lun, u_char *cbuf, int clen,
           u_char *buf, int len)
{
	siop_regmap_p rp = sc->sc_siopp;
	int nchain;
	int count, tcount;
	char *addr, *dmaend;
	struct siop_acb *acb = sc->sc_nexus;
#ifdef DEBUG
	int i;
#endif

#ifdef DEBUG
	if (siop_debug & 0x100 && rp->siop_sbcl & SIOP_BSY) {
		printf ("ACK! siop was busy: rp %p script %p dsa %p active %ld\n",
		    rp, &scripts, &acb->ds, sc->sc_active);
		printf ("istat %02x sfbr %02x lcrc %02x sien %02x dien %02x\n",
		    rp->siop_istat, rp->siop_sfbr, rp->siop_lcrc,
		    rp->siop_sien, rp->siop_dien);
#ifdef DDB
		/*Debugger();*/
#endif
	}
#endif
	acb->msgout[0] = MSG_IDENTIFY | lun;
	if (siop_allow_disc[target] & 2 ||
	    (siop_allow_disc[target] && len == 0))
		acb->msgout[0] = MSG_IDENTIFY_DR | lun;
	acb->status = 0;
	acb->stat[0] = -1;
	acb->msg[0] = -1;
	acb->ds.scsi_addr = (0x10000 << target) | (sc->sc_sync[target].sxfer << 8);
	acb->ds.idlen = 1;
	acb->ds.idbuf = (char *) kvtop(&acb->msgout[0]);
	acb->ds.cmdlen = clen;
	acb->ds.cmdbuf = (char *) kvtop(cbuf);
	acb->ds.stslen = 1;
	acb->ds.stsbuf = (char *) kvtop(&acb->stat[0]);
	acb->ds.msglen = 1;
	acb->ds.msgbuf = (char *) kvtop(&acb->msg[0]);
	acb->msg[1] = -1;
	acb->ds.msginlen = 1;
	acb->ds.extmsglen = 1;
	acb->ds.synmsglen = 3;
	acb->ds.msginbuf = (char *) kvtop(&acb->msg[1]);
	acb->ds.extmsgbuf = (char *) kvtop(&acb->msg[2]);
	acb->ds.synmsgbuf = (char *) kvtop(&acb->msg[3]);
	memset(&acb->ds.chain, 0, sizeof (acb->ds.chain));

	/*
	 * Negotiate wide is the initial negotiation state;  since the 53c710
	 * doesn't do wide transfers, just begin the synchronous transfer
	 * negotation here.
	 */
	if (sc->sc_sync[target].state == NEG_WIDE) {
		if (siop_inhibit_sync[target]) {
			sc->sc_sync[target].state = NEG_DONE;
			sc->sc_sync[target].sbcl = 0;
			sc->sc_sync[target].sxfer = 0;
#ifdef DEBUG
			if (siopsync_debug)
				printf ("Forcing target %d asynchronous\n", target);
#endif
		}
		else {
			acb->msg[2] = -1;
			acb->msgout[1] = MSG_EXT_MESSAGE;
			acb->msgout[2] = 3;
			acb->msgout[3] = MSG_SYNC_REQ;
#ifdef MAXTOR_SYNC_KLUDGE
			acb->msgout[4] = 50 / 4;	/* ask for ridiculous period */
#else
			acb->msgout[4] = sc->sc_minsync;
#endif
			acb->msgout[5] = SIOP_MAX_OFFSET;
			acb->ds.idlen = 6;
			sc->sc_sync[target].state = NEG_WAITS;
#ifdef DEBUG
			if (siopsync_debug)
				printf ("Sending sync request to target %d\n", target);
#endif
		}
	}

/*
 * Build physical DMA addresses for scatter/gather I/O
 */
	acb->iob_buf = buf;
	acb->iob_len = len;
	acb->iob_curbuf = acb->iob_curlen = 0;
	nchain = 0;
	count = len;
	addr = buf;
	dmaend = NULL;
	while (count > 0) {
		acb->ds.chain[nchain].databuf = (char *) kvtop (addr);
		if (count < (tcount = PAGE_SIZE - ((int) addr & PGOFSET)))
			tcount = count;
		acb->ds.chain[nchain].datalen = tcount;
		addr += tcount;
		count -= tcount;
		if (acb->ds.chain[nchain].databuf == dmaend) {
			dmaend += acb->ds.chain[nchain].datalen;
			acb->ds.chain[nchain].datalen = 0;
			acb->ds.chain[--nchain].datalen += tcount;
#ifdef DEBUG
			++siopdma_hits;
#endif
		}
		else {
			dmaend = acb->ds.chain[nchain].databuf +
			    acb->ds.chain[nchain].datalen;
			acb->ds.chain[nchain].datalen = tcount;
#ifdef DEBUG
			if (nchain)	/* Don't count miss on first one */
				++siopdma_misses;
#endif
		}
		++nchain;
	}
#ifdef DEBUG
	if (nchain != 1 && len != 0 && siop_debug & 3) {
		printf ("DMA chaining set: %d\n", nchain);
		for (i = 0; i < nchain; ++i) {
			printf ("  [%d] %8p %lx\n", i, acb->ds.chain[i].databuf,
			    acb->ds.chain[i].datalen);
		}
	}
#endif

	/* push data cache for all data the 53c710 needs to access */
	dma_cachectl ((void *)acb, sizeof (struct siop_acb));
	dma_cachectl (cbuf, clen);
	if (buf != NULL && len != 0)
		dma_cachectl (buf, len);
#ifdef DEBUG
	if (siop_debug & 0x100 && rp->siop_sbcl & SIOP_BSY) {
		printf ("ACK! siop was busy at start: rp %p script %p dsa %p active %ld\n",
		    rp, &scripts, &acb->ds, sc->sc_active);
#ifdef DDB
		/*Debugger();*/
#endif
	}
#endif
	if (sc->nexus_list.tqh_first == NULL) {
		callout_reset(&acb->xs->xs_callout,
		    mstohz(acb->xs->timeout) + 1, siop_timeout, acb);
		if (rp->siop_istat & SIOP_ISTAT_CON)
			printf("%s: siop_select while connected?\n",
			    device_xname(sc->sc_dev));
		rp->siop_temp = 0;
		rp->siop_sbcl = sc->sc_sync[target].sbcl;
		rp->siop_dsa = kvtop((void *)&acb->ds);
		rp->siop_dsp = sc->sc_scriptspa;
		SIOP_TRACE('s',1,0,0)
	} else {
		if ((rp->siop_istat & SIOP_ISTAT_CON) == 0) {
			rp->siop_istat = SIOP_ISTAT_SIGP;
			SIOP_TRACE('s',2,0,0);
		}
		else {
			SIOP_TRACE('s',3,rp->siop_istat,0);
		}
	}
#ifdef DEBUG
	++siopstarts;
#endif
}

/*
 * Process a DMA or SCSI interrupt from the 53C710 SIOP
 */

int
siop_checkintr(struct siop_softc *sc, u_char istat, u_char dstat,
               u_char sstat0, int *status)
{
	siop_regmap_p rp = sc->sc_siopp;
	struct siop_acb *acb = sc->sc_nexus;
	int	target = 0;
	int	dfifo, dbc, sstat1;

	dfifo = rp->siop_dfifo;
	dbc = rp->siop_dbc0;
	sstat1 = rp->siop_sstat1;
	rp->siop_ctest8 |= SIOP_CTEST8_CLF;
	while ((rp->siop_ctest1 & SIOP_CTEST1_FMT) != SIOP_CTEST1_FMT)
		;
	rp->siop_ctest8 &= ~SIOP_CTEST8_CLF;
#ifdef DEBUG
	++siopints;
#if 0
	if (siop_debug & 0x100) {
		DCIAS(&acb->stat[0]);	/* XXX */
		printf ("siopchkintr: istat %x dstat %x sstat0 %x dsps %x sbcl %x sts %x msg %x\n",
		    istat, dstat, sstat0, rp->siop_dsps, rp->siop_sbcl, acb->stat[0], acb->msg[0]);
		printf ("sync msg in: %02x %02x %02x %02x %02x %02x\n",
		    acb->msg[0], acb->msg[1], acb->msg[2],
		    acb->msg[3], acb->msg[4], acb->msg[5]);
	}
#endif
	if (rp->siop_dsp && (rp->siop_dsp < sc->sc_scriptspa ||
	    rp->siop_dsp >= sc->sc_scriptspa + sizeof(scripts))) {
		printf ("%s: dsp not within script dsp %lx scripts %lx:%lx",
		    device_xname(sc->sc_dev), rp->siop_dsp, sc->sc_scriptspa,
		    sc->sc_scriptspa + sizeof(scripts));
		printf(" istat %x dstat %x sstat0 %x\n",
		    istat, dstat, sstat0);
#ifdef DDB
		Debugger();
#endif
	}
#endif
	SIOP_TRACE('i',dstat,istat,(istat&SIOP_ISTAT_DIP)?rp->siop_dsps&0xff:sstat0);
	if (dstat & SIOP_DSTAT_SIR && rp->siop_dsps == 0xff00) {
		/* Normal completion status, or check condition */
#ifdef DEBUG
		if (rp->siop_dsa != kvtop((void *)&acb->ds)) {
			printf ("siop: invalid dsa: %lx %x\n", rp->siop_dsa,
			    (unsigned)kvtop((void *)&acb->ds));
			panic("*** siop DSA invalid ***");
		}
#endif
		target = acb->xs->xs_periph->periph_target;
		if (sc->sc_sync[target].state == NEG_WAITS) {
			if (acb->msg[1] == 0xff)
				printf ("%s: target %d ignored sync request\n",
				    device_xname(sc->sc_dev), target);
			else if (acb->msg[1] == MSG_REJECT)
				printf ("%s: target %d rejected sync request\n",
				    device_xname(sc->sc_dev), target);
			else
/* XXX - need to set sync transfer parameters */
				printf("%s: target %d (sync) %02x %02x %02x\n",
				    device_xname(sc->sc_dev), target, acb->msg[1],
				    acb->msg[2], acb->msg[3]);
			sc->sc_sync[target].state = NEG_DONE;
		}
		dma_cachectl(&acb->stat[0], 1);
		*status = acb->stat[0];
#ifdef DEBUG
		if (rp->siop_sbcl & SIOP_BSY) {
			/*printf ("ACK! siop was busy at end: rp %x script %x dsa %x\n",
			    rp, &scripts, &acb->ds);*/
#ifdef DDB
			/*Debugger();*/
#endif
		}
		if (acb->msg[0] != 0x00)
			printf("%s: message was not COMMAND COMPLETE: %x\n",
			    device_xname(sc->sc_dev), acb->msg[0]);
#endif
		if (sc->nexus_list.tqh_first)
			rp->siop_dcntl |= SIOP_DCNTL_STD;
		return 1;
	}
	if (dstat & SIOP_DSTAT_SIR && rp->siop_dsps == 0xff0b) {
		target = acb->xs->xs_periph->periph_target;
		if (acb->msg[1] == MSG_EXT_MESSAGE && acb->msg[2] == 3 &&
		    acb->msg[3] == MSG_SYNC_REQ) {
#ifdef DEBUG
			if (siopsync_debug)
				printf ("sync msg in: %02x %02x %02x %02x %02x %02x\n",
				    acb->msg[0], acb->msg[1], acb->msg[2],
				    acb->msg[3], acb->msg[4], acb->msg[5]);
#endif
			sc->sc_sync[target].sxfer = 0;
			sc->sc_sync[target].sbcl = 0;
			if (acb->msg[2] == 3 &&
			    acb->msg[3] == MSG_SYNC_REQ &&
			    acb->msg[5] != 0) {
#ifdef MAXTOR_KLUDGE
				/*
				 * Kludge for my Maxtor XT8580S
				 * It accepts whatever we request, even
				 * though it won't work.  So we ask for
				 * a short period than we can handle.  If
				 * the device says it can do it, use 208ns.
				 * If the device says it can do less than
				 * 100ns, then we limit it to 100ns.
				 */
				if (acb->msg[4] && acb->msg[4] < 100 / 4) {
#ifdef DEBUG
					printf ("%d: target %d wanted %dns period\n",
					    device_xname(sc->sc_dev), target,
					    acb->msg[4] * 4);
#endif
					if (acb->msg[4] == 50 / 4)
						acb->msg[4] = 208 / 4;
					else
						acb->msg[4] = 100 / 4;
				}
#endif /* MAXTOR_KLUDGE */
				printf ("%s: target %d now synchronous, period=%dns, offset=%d\n",
				    device_xname(sc->sc_dev), target,
				    acb->msg[4] * 4, acb->msg[5]);
				scsi_period_to_siop (sc, target);
			}
			rp->siop_sxfer = sc->sc_sync[target].sxfer;
			rp->siop_sbcl = sc->sc_sync[target].sbcl;
			if (sc->sc_sync[target].state == NEG_WAITS) {
				sc->sc_sync[target].state = NEG_DONE;
				rp->siop_dsp = sc->sc_scriptspa + Ent_clear_ack;
				return(0);
			}
			rp->siop_dcntl |= SIOP_DCNTL_STD;
			sc->sc_sync[target].state = NEG_DONE;
			return (0);
		}
		/* XXX - not SDTR message */
	}
	if (sstat0 & SIOP_SSTAT0_M_A) {		/* Phase mismatch */
#ifdef DEBUG
		++siopphmm;
		if (acb == NULL)
			printf("%s: Phase mismatch with no active command?\n",
			    device_xname(sc->sc_dev));
#endif
		if (acb->iob_len) {
			int adjust;
			adjust = ((dfifo - (dbc & 0x7f)) & 0x7f);
			if (sstat1 & SIOP_SSTAT1_ORF)
				++adjust;
			if (sstat1 & SIOP_SSTAT1_OLF)
				++adjust;
			acb->iob_curlen = 
			    *((long *)__UNVOLATILE(&rp->siop_dcmd)) & 0xffffff;
			acb->iob_curlen += adjust;
			acb->iob_curbuf = 
			    *((long *)__UNVOLATILE(&rp->siop_dnad)) - adjust;
#ifdef DEBUG
			if (siop_debug & 0x100) {
				int i;
				printf ("Phase mismatch: curbuf %lx curlen %lx dfifo %x dbc %x sstat1 %x adjust %x sbcl %x starts %d acb %p\n",
				    acb->iob_curbuf, acb->iob_curlen, dfifo,
				    dbc, sstat1, adjust, rp->siop_sbcl, 
				    siopstarts, acb);
				if (acb->ds.chain[1].datalen) {
					for (i = 0; acb->ds.chain[i].datalen; ++i)
						printf("chain[%d] addr %p len %lx\n",
						    i, acb->ds.chain[i].databuf,
						    acb->ds.chain[i].datalen);
				}
			}
#endif
			dma_cachectl ((void *)acb, sizeof(*acb));
		}
#ifdef DEBUG
		SIOP_TRACE('m',rp->siop_sbcl,(rp->siop_dsp>>8),rp->siop_dsp);
		if (siop_debug & 9)
			printf ("Phase mismatch: %x dsp +%lx dcmd %lx\n",
			    rp->siop_sbcl,
			    rp->siop_dsp - sc->sc_scriptspa,
			    *((volatile long *)&rp->siop_dcmd));
#endif
		if ((rp->siop_sbcl & SIOP_REQ) == 0) {
			printf ("Phase mismatch: REQ not asserted! %02x dsp %lx\n",
			    rp->siop_sbcl, rp->siop_dsp);
#if defined(DEBUG) && defined(DDB)
			/*Debugger(); XXX is*/
#endif
		}
		switch (rp->siop_sbcl & 7) {
		case 0:		/* data out */
		case 1:		/* data in */
		case 2:		/* status */
		case 3:		/* command */
		case 6:		/* message in */
		case 7:		/* message out */
			rp->siop_dsp = sc->sc_scriptspa + Ent_switch;
			break;
		default:
			goto bad_phase;
		}
		return 0;
	}
	if (sstat0 & SIOP_SSTAT0_STO) {		/* Select timed out */
#ifdef DEBUG
		if (acb == NULL)
			printf("%s: Select timeout with no active command?\n",
			    device_xname(sc->sc_dev));
		if (rp->siop_sbcl & SIOP_BSY) {
			printf ("ACK! siop was busy at timeout: rp %p script %p dsa %p\n",
			    rp, &scripts, &acb->ds);
			printf(" sbcl %x sdid %x istat %x dstat %x sstat0 %x\n",
			    rp->siop_sbcl, rp->siop_sdid, istat, dstat, sstat0);
			if (!(rp->siop_sbcl & SIOP_BSY)) {
				printf ("Yikes, it's not busy now!\n");
#if 0
				*status = -1;
				if (sc->nexus_list.tqh_first)
					rp->siop_dsp = sc->sc_scriptspa + Ent_wait_reselect;
				return 1;
#endif
			}
/*			rp->siop_dcntl |= SIOP_DCNTL_STD;*/
			return (0);
#ifdef DDB
			Debugger();
#endif
		}
#endif
		*status = -1;
		acb->xs->error = XS_SELTIMEOUT;
		if (sc->nexus_list.tqh_first)
			rp->siop_dsp = sc->sc_scriptspa + Ent_wait_reselect;
		return 1;
	}
	if (acb)
		target = acb->xs->xs_periph->periph_target;
	else
		target = 7;
	if (sstat0 & SIOP_SSTAT0_UDC) {
#ifdef DEBUG
		if (acb == NULL)
			printf("%s: Unexpected disconnect with no active command?\n",
			    device_xname(sc->sc_dev));
		printf ("%s: target %d disconnected unexpectedly\n",
		   device_xname(sc->sc_dev), target);
#endif
#if 0
		siopabort (sc, rp, "siopchkintr");
#endif
		*status = STS_BUSY;
		if (sc->nexus_list.tqh_first)
			rp->siop_dsp = sc->sc_scriptspa + Ent_wait_reselect;
		return (acb != NULL);
	}
	if (dstat & SIOP_DSTAT_SIR && (rp->siop_dsps == 0xff01 ||
	    rp->siop_dsps == 0xff02)) {
#ifdef DEBUG
		if (siop_debug & 0x100)
			printf ("%s: ID %02x disconnected TEMP %lx (+%lx) curbuf %lx curlen %lx buf %p len %lx dfifo %x dbc %x sstat1 %x starts %d acb %p\n",
			    device_xname(sc->sc_dev), 1 << target, rp->siop_temp,
			    rp->siop_temp ? rp->siop_temp - sc->sc_scriptspa : 0,
			    acb->iob_curbuf, acb->iob_curlen,
			    acb->ds.chain[0].databuf, acb->ds.chain[0].datalen, dfifo, dbc, sstat1, siopstarts, acb);
#endif
		if (acb == NULL) {
			printf("%s: Disconnect with no active command?\n",
			    device_xname(sc->sc_dev));
			return (0);
		}
		/*
		 * XXXX need to update iob_curbuf/iob_curlen to reflect
		 * current data transferred.  If device disconnected in
		 * the middle of a DMA block, they should already be set
		 * by the phase change interrupt.  If the disconnect
		 * occurs on a DMA block boundary, we have to figure out
		 * which DMA block it was.
		 */
		if (acb->iob_len && rp->siop_temp) {
			int n = rp->siop_temp - sc->sc_scriptspa;

			if (acb->iob_curlen && acb->iob_curlen != acb->ds.chain[0].datalen)
				printf("%s: iob_curbuf/len already set? n %x iob %lx/%lx chain[0] %p/%lx\n",
				    device_xname(sc->sc_dev), n, acb->iob_curbuf, acb->iob_curlen,
				    acb->ds.chain[0].databuf, acb->ds.chain[0].datalen);
			if (n < Ent_datain)
				n = (n - Ent_dataout) / 16;
			else
				n = (n - Ent_datain) / 16;
			if (n <= 0 && n > DMAMAXIO)
				printf("TEMP invalid %d\n", n);
			else {
				acb->iob_curbuf = (u_long)acb->ds.chain[n].databuf;
				acb->iob_curlen = acb->ds.chain[n].datalen;
			}
#ifdef DEBUG
			if (siop_debug & 0x100) {
				printf("%s: TEMP offset %d", device_xname(sc->sc_dev), n);
				printf(" curbuf %lx curlen %lx\n", acb->iob_curbuf,
				    acb->iob_curlen);
			}
#endif
		}
		/*
		 * If data transfer was interrupted by disconnect, iob_curbuf
		 * and iob_curlen should reflect the point of interruption.
		 * Adjust the DMA chain so that the data transfer begins
		 * at the appropriate place upon reselection.
		 * XXX This should only be done on save data pointer message?
		 */
		if (acb->iob_curlen) {
			int i, j;

#ifdef DEBUG
			if (siop_debug & 0x100)
				printf ("%s: adjusting DMA chain\n",
				    device_xname(sc->sc_dev));
			if (rp->siop_dsps == 0xff02)
				printf ("%s: ID %02x disconnected without Save Data Pointers\n",
				    device_xname(sc->sc_dev), 1 << target);
#endif
/* XXX is: 		if (rp->siop_dsps != 0xff02) { */
				/* not disconnected without save data ptr */
			for (i = 0; i < DMAMAXIO; ++i) {
				if (acb->ds.chain[i].datalen == 0)
					break;
				if (acb->iob_curbuf >= (long)acb->ds.chain[i].databuf &&
				    acb->iob_curbuf < (long)(acb->ds.chain[i].databuf +
				    acb->ds.chain[i].datalen))
					break;
			}
			if (i >= DMAMAXIO || acb->ds.chain[i].datalen == 0) {
				printf("couldn't find saved data pointer: ");
				printf("curbuf %lx curlen %lx i %d\n",
				    acb->iob_curbuf, acb->iob_curlen, i);
#ifdef DDB
				Debugger();
#endif
			}
/* XXX is: 		}			*/
#ifdef DEBUG
			if (siop_debug & 0x100)
				printf("  chain[0]: %p/%lx -> %lx/%lx\n",
				    acb->ds.chain[0].databuf,
				    acb->ds.chain[0].datalen,
				    acb->iob_curbuf,
				    acb->iob_curlen);
#endif
			acb->ds.chain[0].databuf = (char *)acb->iob_curbuf;
			acb->ds.chain[0].datalen = acb->iob_curlen;
			for (j = 1, ++i; i < DMAMAXIO && acb->ds.chain[i].datalen; ++i, ++j) {
#ifdef DEBUG
			if (siop_debug & 0x100)
				printf("  chain[%d]: %p/%lx -> %p/%lx\n", j,
				    acb->ds.chain[j].databuf,
				    acb->ds.chain[j].datalen,
				    acb->ds.chain[i].databuf,
				    acb->ds.chain[i].datalen);
#endif
				acb->ds.chain[j].databuf = acb->ds.chain[i].databuf;
				acb->ds.chain[j].datalen = acb->ds.chain[i].datalen;
			}
			if (j < DMAMAXIO)
				acb->ds.chain[j].datalen = 0;
			DCIAS(kvtop((void *)&acb->ds.chain));
		}
		++sc->sc_tinfo[target].dconns;
		/*
		 * add nexus to waiting list
		 * clear nexus
		 * try to start another command for another target/lun
		 */
		acb->status = sc->sc_flags & SIOP_INTSOFF;
		TAILQ_INSERT_HEAD(&sc->nexus_list, acb, chain);
		sc->sc_nexus = NULL;		/* no current device */
		/* start script to wait for reselect */
		if (sc->sc_nexus == NULL)
			rp->siop_dsp = sc->sc_scriptspa + Ent_wait_reselect;
/* XXXX start another command ? */
		if (sc->ready_list.tqh_first)
			siop_sched(sc);
		return (0);
	}
	if (dstat & SIOP_DSTAT_SIR && rp->siop_dsps == 0xff03) {
		int reselid = rp->siop_scratch & 0x7f;
		int reselun = rp->siop_sfbr & 0x07;

		sc->sc_sstat1 = rp->siop_sbcl;	/* XXXX save current SBCL */
#ifdef DEBUG
		if (siop_debug & 0x100)
			printf ("%s: target ID %02x reselected dsps %lx\n",
			     device_xname(sc->sc_dev), reselid,
			     rp->siop_dsps);
		if ((rp->siop_sfbr & 0x80) == 0)
			printf("%s: Reselect message in was not identify: %x\n",
			    device_xname(sc->sc_dev), rp->siop_sfbr);
#endif
		if (sc->sc_nexus) {
#ifdef DEBUG
			if (siop_debug & 0x100)
				printf ("%s: reselect ID %02x w/active\n",
				    device_xname(sc->sc_dev), reselid);
#endif
			TAILQ_INSERT_HEAD(&sc->ready_list, sc->sc_nexus, chain);
			sc->sc_tinfo[sc->sc_nexus->xs->xs_periph->periph_target].lubusy
			    &= ~(1 << sc->sc_nexus->xs->xs_periph->periph_lun);
			--sc->sc_active;
		}
		/*
		 * locate acb of reselecting device
		 * set sc->sc_nexus to acb
		 */
		for (acb = sc->nexus_list.tqh_first; acb;
		    acb = acb->chain.tqe_next) {
			if (reselid != (acb->ds.scsi_addr >> 16) ||
			    reselun != (acb->msgout[0] & 0x07))
				continue;
			TAILQ_REMOVE(&sc->nexus_list, acb, chain);
			sc->sc_nexus = acb;
			sc->sc_flags |= acb->status;
			acb->status = 0;
			DCIAS(kvtop(&acb->stat[0]));
			rp->siop_dsa = kvtop((void *)&acb->ds);
			rp->siop_sxfer =
				sc->sc_sync[acb->xs->xs_periph->periph_target].sxfer;
			rp->siop_sbcl =
				sc->sc_sync[acb->xs->xs_periph->periph_target].sbcl;
			break;
		}
		if (acb == NULL) {
			printf("%s: target ID %02x reselect nexus_list %p\n",
			    device_xname(sc->sc_dev), reselid,
			    sc->nexus_list.tqh_first);
			panic("unable to find reselecting device");
		}
		dma_cachectl ((void *)acb, sizeof(*acb));
		rp->siop_temp = 0;
		rp->siop_dcntl |= SIOP_DCNTL_STD;
		return (0);
	}
	if (dstat & SIOP_DSTAT_SIR && rp->siop_dsps == 0xff04) {
#ifdef DEBUG
		u_short ctest2 = rp->siop_ctest2;

		/* reselect was interrupted (by Sig_P or select) */
		if (siop_debug & 0x100 ||
		    (ctest2 & SIOP_CTEST2_SIGP) == 0)
			printf ("%s: reselect interrupted (Sig_P?) scntl1 %x ctest2 %x sfbr %x istat %x/%x\n",
			    device_xname(sc->sc_dev), rp->siop_scntl1,
			    ctest2, rp->siop_sfbr, istat, rp->siop_istat);
#endif
		/* XXX assumes it was not select */
		if (sc->sc_nexus == NULL) {
#ifdef DEBUG
			printf("%s: reselect interrupted, sc_nexus == NULL\n",
			    device_xname(sc->sc_dev));
#if 0
			siop_dump(sc);
#ifdef DDB
			Debugger();
#endif
#endif
#endif
			rp->siop_dcntl |= SIOP_DCNTL_STD;
			return(0);
		}
		target = sc->sc_nexus->xs->xs_periph->periph_target;
		rp->siop_temp = 0;
		rp->siop_dsa = kvtop((void *)&sc->sc_nexus->ds);
		rp->siop_sxfer = sc->sc_sync[target].sxfer;
		rp->siop_sbcl = sc->sc_sync[target].sbcl;
		rp->siop_dsp = sc->sc_scriptspa;
		return (0);
	}
	if (dstat & SIOP_DSTAT_SIR && rp->siop_dsps == 0xff06) {
		if (acb == NULL)
			printf("%s: Bad message-in with no active command?\n",
			    device_xname(sc->sc_dev));
		/* Unrecognized message in byte */
		dma_cachectl (&acb->msg[1],1);
		printf ("%s: Unrecognized message in data sfbr %x msg %x sbcl %x\n",
			device_xname(sc->sc_dev), rp->siop_sfbr, acb->msg[1], rp->siop_sbcl);
		/* what should be done here? */
		DCIAS(kvtop(&acb->msg[1]));
		rp->siop_dsp = sc->sc_scriptspa + Ent_clear_ack;
		return (0);
	}
	if (dstat & SIOP_DSTAT_SIR && rp->siop_dsps == 0xff0a) {
		/* Status phase wasn't followed by message in phase? */
		printf ("%s: Status phase not followed by message in phase? sbcl %x sbdl %x\n",
			device_xname(sc->sc_dev), rp->siop_sbcl, rp->siop_sbdl);
		if (rp->siop_sbcl == 0xa7) {
			/* It is now, just continue the script? */
			rp->siop_dcntl |= SIOP_DCNTL_STD;
			return (0);
		}
	}
	if (sstat0 == 0 && dstat & SIOP_DSTAT_SIR) {
		dma_cachectl (&acb->stat[0], 1);
		dma_cachectl (&acb->msg[0], 1);
		printf ("SIOP interrupt: %lx sts %x msg %x %x sbcl %x\n",
		    rp->siop_dsps, acb->stat[0], acb->msg[0], acb->msg[1],
		    rp->siop_sbcl);
		siopreset (sc);
		*status = -1;
		return 0;	/* siopreset has cleaned up */
	}
	if (sstat0 & SIOP_SSTAT0_SGE)
		printf ("SIOP: SCSI Gross Error\n");
	if (sstat0 & SIOP_SSTAT0_PAR)
		printf ("SIOP: Parity Error\n");
	if (dstat & SIOP_DSTAT_IID)
		printf ("SIOP: Invalid instruction detected\n");
bad_phase:
	/*
	 * temporary panic for unhandled conditions
	 * displays various things about the 53C710 status and registers
	 * then panics.
	 * XXXX need to clean this up to print out the info, reset, and continue
	 */
	printf ("siopchkintr: target %x ds %p\n", target, &acb->ds);
	printf ("scripts %lx ds %x rp %x dsp %lx dcmd %lx\n",
	    sc->sc_scriptspa, (unsigned)kvtop((void *)&acb->ds),
	    (unsigned)kvtop((void *)__UNVOLATILE(rp)), rp->siop_dsp,
	    *((volatile long *)&rp->siop_dcmd));
	printf ("siopchkintr: istat %x dstat %x sstat0 %x dsps %lx dsa %lx sbcl %x sts %x msg %x %x sfbr %x\n",
	    istat, dstat, sstat0, rp->siop_dsps, rp->siop_dsa,
	     rp->siop_sbcl, acb->stat[0], acb->msg[0], acb->msg[1], rp->siop_sfbr);
#ifdef DEBUG
	if (siop_debug & 0x20)
		panic("siopchkintr: **** temp ****");
#endif
#ifdef DDB
	Debugger ();
#endif
	siopreset (sc);		/* hard reset */
	*status = -1;
	return 0;		/* siopreset cleaned up */
}

void
siop_select(struct siop_softc *sc)
{
	siop_regmap_p rp;
	struct siop_acb *acb = sc->sc_nexus;

#ifdef DEBUG
	if (siop_debug & 1)
		printf ("%s: select ", device_xname(sc->sc_dev));
#endif

	rp = sc->sc_siopp;
	if (acb->xs->xs_control & XS_CTL_POLL || siop_no_dma) {
		sc->sc_flags |= SIOP_INTSOFF;
		sc->sc_flags &= ~SIOP_INTDEFER;
		if ((rp->siop_istat & 0x08) == 0) {
			rp->siop_sien = 0;
			rp->siop_dien = 0;
		}
#if 0
	} else if ((sc->sc_flags & SIOP_INTDEFER) == 0) {
		sc->sc_flags &= ~SIOP_INTSOFF;
		if ((rp->siop_istat & 0x08) == 0) {
			rp->siop_sien = sc->sc_sien;
			rp->siop_dien = sc->sc_dien;
		}
#endif
	}
#ifdef DEBUG
	if (siop_debug & 1)
		printf ("siop_select: target %x cmd %02x ds %p\n",
		    acb->xs->xs_periph->periph_target, acb->cmd.opcode,
		    &sc->sc_nexus->ds);
#endif

	siop_start(sc, acb->xs->xs_periph->periph_target,
		acb->xs->xs_periph->periph_lun,
	    (u_char *)&acb->cmd, acb->clen, acb->daddr, acb->dleft);

	return;
}

/*
 * 53C710 interrupt handler
 */

void
siopintr(register struct siop_softc *sc)
{
	siop_regmap_p rp;
	register u_char istat, dstat, sstat0;
	int status;
	int s = splbio();

	istat = sc->sc_istat;
	if ((istat & (SIOP_ISTAT_SIP | SIOP_ISTAT_DIP)) == 0) {
		splx(s);
		return;
	}

	/* Got a valid interrupt on this device */
	rp = sc->sc_siopp;
	dstat = sc->sc_dstat;
	sstat0 = sc->sc_sstat0;
	if (dstat & SIOP_DSTAT_SIR)
		sc->sc_intcode = rp->siop_dsps;
	sc->sc_istat = 0;
#ifdef DEBUG
	if (siop_debug & 1)
		printf ("%s: intr istat %x dstat %x sstat0 %x\n",
		    device_xname(sc->sc_dev), istat, dstat, sstat0);
	if (!sc->sc_active) {
		printf ("%s: spurious interrupt? istat %x dstat %x sstat0 %x nexus %p status %x\n",
		    device_xname(sc->sc_dev), istat, dstat, sstat0,
		    sc->sc_nexus, sc->sc_nexus ? sc->sc_nexus->stat[0] : 0);
	}
#endif

#ifdef DEBUG
	if (siop_debug & 5) {
		DCIAS(kvtop(&sc->sc_nexus->stat[0]));
		printf ("%s: intr istat %x dstat %x sstat0 %x dsps %lx sbcl %x sts %x msg %x\n",
		    device_xname(sc->sc_dev), istat, dstat, sstat0,
		    rp->siop_dsps,  rp->siop_sbcl,
		    sc->sc_nexus->stat[0], sc->sc_nexus->msg[0]);
	}
#endif
	if (sc->sc_flags & SIOP_INTDEFER) {
		sc->sc_flags &= ~(SIOP_INTDEFER | SIOP_INTSOFF);
		rp->siop_sien = sc->sc_sien;
		rp->siop_dien = sc->sc_dien;
	}
	if (siop_checkintr (sc, istat, dstat, sstat0, &status)) {
#if 1
		if (status == 0xff)
			printf ("siopintr: status == 0xff\n");
#endif
		if ((sc->sc_flags & (SIOP_INTSOFF | SIOP_INTDEFER)) != SIOP_INTSOFF) {
#if 0
			if (rp->siop_sbcl & SIOP_BSY) {
				printf ("%s: SCSI bus busy at completion",
					device_xname(sc->sc_dev));
				printf(" targ %d sbcl %02x sfbr %x lcrc %02x dsp +%x\n",
				    sc->sc_nexus->xs->xs_periph->periph_target,
				    rp->siop_sbcl, rp->siop_sfbr, rp->siop_lcrc,
				    rp->siop_dsp - sc->sc_scriptspa);
			}
#endif
			siop_scsidone(sc->sc_nexus, sc->sc_nexus ?
			    sc->sc_nexus->stat[0] : -1);
		}
	}
	splx(s);
}

/*
 * This is based on the Progressive Peripherals 33Mhz Zeus driver and will
 * not be correct for other 53c710 boards.
 *
 */
void
scsi_period_to_siop(struct siop_softc *sc, int target)
{
	int period, offset, sxfer, sbcl = 0;
#ifdef DEBUG_SYNC
	int i;
#endif

	period = sc->sc_nexus->msg[4];
	offset = sc->sc_nexus->msg[5];
#ifdef DEBUG_SYNC
	sxfer = 0;
	if (offset <= SIOP_MAX_OFFSET)
		sxfer = offset;
	for (i = 0; i < sizeof (sync_tab) / 2; ++i) {
		if (period <= sync_tab[i].p) {
			sxfer |= sync_tab[i].r & 0x70;
			sbcl = sync_tab[i].r & 0x03;
			break;
		}
	}
	printf ("siop sync old: siop_sxfr %02x, siop_sbcl %02x\n", sxfer, sbcl);
#endif
	for (sbcl = 1; sbcl < 4; ++sbcl) {
		sxfer = (period * 4 - 1) / sc->sc_tcp[sbcl] - 3;
		if (sxfer >= 0 && sxfer <= 7)
			break;
	}
	if (sbcl > 3) {
		printf("siop sync: unable to compute sync params for period %dns\n",
		    period * 4);
		/*
		 * XXX need to pick a value we can do and renegotiate
		 */
		sxfer = sbcl = 0;
	} else {
		sxfer = (sxfer << 4) | ((offset <= SIOP_MAX_OFFSET) ?
		    offset : SIOP_MAX_OFFSET);
#ifdef DEBUG_SYNC
		printf("siop sync: params for period %dns: sxfer %x sbcl %x",
		    period * 4, sxfer, sbcl);
		printf(" actual period %dns\n",
		    sc->sc_tcp[sbcl] * ((sxfer >> 4) + 4));
#endif
	}
	sc->sc_sync[target].sxfer = sxfer;
	sc->sc_sync[target].sbcl = sbcl;
#ifdef DEBUG_SYNC
	printf ("siop sync: siop_sxfr %02x, siop_sbcl %02x\n", sxfer, sbcl);
#endif
}

#ifdef DEBUG

#if SIOP_TRACE_SIZE
void
siop_dump_trace(void)
{
	int i;

	printf("siop trace: next index %d\n", siop_trix);
	i = siop_trix;
	do {
		printf("%3d: '%c' %02x %02x %02x\n", i, siop_trbuf[i],
		    siop_trbuf[i + 1], siop_trbuf[i + 2], siop_trbuf[i + 3]);
		i = (i + 4) & (SIOP_TRACE_SIZE - 1);
	} while (i != siop_trix);
}
#endif

void
siop_dump_acb(struct siop_acb *acb)
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
	printf("  xs: %p data %p:%04x ", acb->xs, acb->xs->data,
	    acb->xs->datalen);
	printf("va %p:%lx ", acb->iob_buf, acb->iob_len);
	printf("cur %lx:%lx\n", acb->iob_curbuf, acb->iob_curlen);
}

void
siop_dump(struct siop_softc *sc)
{
	struct siop_acb *acb;
	siop_regmap_p rp = sc->sc_siopp;
	int s;
	int i;

	s = splbio();
#if SIOP_TRACE_SIZE
	siop_dump_trace();
#endif
	printf("%s@%p regs %p istat %x\n",
	    device_xname(sc->sc_dev), sc, rp, rp->siop_istat);
	if ((acb = sc->free_list.tqh_first) > 0) {
		printf("Free list:\n");
		while (acb) {
			siop_dump_acb(acb);
			acb = acb->chain.tqe_next;
		}
	}
	if ((acb = sc->ready_list.tqh_first) > 0) {
		printf("Ready list:\n");
		while (acb) {
			siop_dump_acb(acb);
			acb = acb->chain.tqe_next;
		}
	}
	if ((acb = sc->nexus_list.tqh_first) > 0) {
		printf("Nexus list:\n");
		while (acb) {
			siop_dump_acb(acb);
			acb = acb->chain.tqe_next;
		}
	}
	if (sc->sc_nexus) {
		printf("Nexus:\n");
		siop_dump_acb(sc->sc_nexus);
	}
	for (i = 0; i < 8; ++i) {
		if (sc->sc_tinfo[i].cmds > 2) {
			printf("tgt %d: cmds %d disc %d lubusy %x\n",
			    i, sc->sc_tinfo[i].cmds,
			    sc->sc_tinfo[i].dconns,
			    sc->sc_tinfo[i].lubusy);
		}
	}
	splx(s);
}
#endif
