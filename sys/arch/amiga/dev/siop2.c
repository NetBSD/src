/*	$NetBSD: siop2.c,v 1.43 2014/09/21 15:44:47 christos Exp $ */

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
 * Copyright (c) 1994,1998 Michael L. Hitch
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
 * AMIGA 53C720/770 scsi adaptor driver
 */

#include "opt_ddb.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: siop2.c,v 1.43 2014/09/21 15:44:47 christos Exp $");

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
#include <m68k/include/cacheops.h>
#else
#define DCIAS(pa) dma_cachectl((void *)(pa), 1)
#endif
#include <amiga/amiga/custom.h>
#include <amiga/amiga/device.h>
#include <amiga/amiga/isr.h>

#define ARCH_720

#include <amiga/dev/siopreg.h>
#include <amiga/dev/siopvar.h>

/*
 * SCSI delays
 * In u-seconds, primarily for state changes on the SPC.
 */
#define	SCSI_CMD_WAIT	500000	/* wait per step of 'immediate' cmds */
#define	SCSI_DATA_WAIT	500000	/* wait per data in/out step */
#define	SCSI_INIT_WAIT	500000	/* wait per step (both) during init */

void siopng_select(struct siop_softc *);
void siopngabort(struct siop_softc *, siop_regmap_p, const char *);
void siopngerror(struct siop_softc *, siop_regmap_p, u_char);
int  siopng_checkintr(struct siop_softc *, u_char, u_char, u_short, int *);
void siopngreset(struct siop_softc *);
void siopngsetdelay(int);
void siopng_scsidone(struct siop_acb *, int);
void siopng_timeout(void *);
void siopng_sched(struct siop_softc *);
void siopng_poll(struct siop_softc *, struct siop_acb *);
void siopngintr(struct siop_softc *);
void scsi_period_to_siopng(struct siop_softc *, int);
void siopng_start(struct siop_softc *, int, int, u_char *, int, u_char *, int);
void siopng_dump_acb(struct siop_acb *);

/* 53C720/770 script */

#include <amiga/dev/siop2_script.out>

/* default to not inhibit sync negotiation on any drive */
u_char siopng_inhibit_sync[16] = {
	0, 0, 0, 0,	0, 0, 0, 0,	0, 0, 0, 0,	0, 0, 0, 0
}; /* initialize, so patchable */

u_char siopng_inhibit_wide[16] = {
	0, 0, 0, 0,	0, 0, 0, 0,	0, 0, 0, 0,	0, 0, 0, 0
}; /* initialize, so patchable */

u_char siopng_allow_disc[16] = {
	3, 3, 3, 3,	3, 3, 3, 3,	3, 3, 3, 3,	3, 3, 3, 3
};

int siopng_no_dma = 0;

int siopng_reset_delay = 250;	/* delay after reset, in milleseconds */

int siopng_cmd_wait = SCSI_CMD_WAIT;
int siopng_data_wait = SCSI_DATA_WAIT;
int siopng_init_wait = SCSI_INIT_WAIT;

/*#define DEBUG_SYNC*/

#ifdef DEBUG
/*
 *	0x01 - full debug
 *	0x02 - DMA chaining
 *	0x04 - siopngintr
 *	0x08 - phase mismatch
 *	0x10 - <not used>
 *	0x20 - panic on unhandled exceptions
 *	0x100 - disconnect/reselect
 */
int	siopng_debug = 0;
int	siopngsync_debug = 0;
int	siopngdma_hits = 0;
int	siopngdma_misses = 0;
int	siopngchain_ints = 0;
int	siopngstarts = 0;
int	siopngints = 0;
int	siopngphmm = 0;
#define SIOP_TRACE_SIZE	128
#define SIOP_TRACE(a,b,c,d) \
	siopng_trbuf[siopng_trix] = (a); \
	siopng_trbuf[siopng_trix+1] = (b); \
	siopng_trbuf[siopng_trix+2] = (c); \
	siopng_trbuf[siopng_trix+3] = (d); \
	siopng_trix = (siopng_trix + 4) & (SIOP_TRACE_SIZE - 1);
u_char	siopng_trbuf[SIOP_TRACE_SIZE];
int	siopng_trix;
void siopng_dump(struct siop_softc *);
void siopng_dump_trace(void);
#else
#define SIOP_TRACE(a,b,c,d)
#endif


static const char *siopng_chips[] = {
	"720", "720SE", "770", "0x3",
	"810A", "0x5", "0x6", "0x7",
	"0x8", "0x9", "0xA", "0xB",
	"0xC", "0xD", "0xE", "0xF",
};

/*
 * default minphys routine for siopng based controllers
 */
void
siopng_minphys(struct buf *bp)
{

	/*
	 * No max transfer at this level.
	 */
	minphys(bp);
}

/*
 * used by specific siopng controller
 *
 */
void
siopng_scsipi_request(struct scsipi_channel *chan, scsipi_adapter_req_t req,
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
			panic("siopng: scsi data uio requested");

		/* XXXX ?? */
		if (sc->sc_nexus && flags & XS_CTL_POLL)
/*			panic("siopng_scsicmd: busy");*/
			printf("siopng_scsicmd: busy\n");

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
			panic("siopng_scsipi_request");
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
			siopng_sched(sc);

		splx(s);

		if (flags & XS_CTL_POLL || siopng_no_dma)
			siopng_poll(sc, acb);
		return;

	case ADAPTER_REQ_GROW_RESOURCES:
		return;

	case ADAPTER_REQ_SET_XFER_MODE:
		return;
	}
}

void
siopng_poll(struct siop_softc *sc, struct siop_acb *acb)
{
	siop_regmap_p rp = sc->sc_siopp;
	struct scsipi_xfer *xs = acb->xs;
	int i;
	int status;
	u_char istat;
	u_char dstat;
	u_short sist;
	int s;
	int to;

	s = splbio();
	to = xs->timeout / 1000;
	if (sc->nexus_list.tqh_first)
		printf("%s: siopng_poll called with disconnected device\n",
		    device_xname(sc->sc_dev));
	for (;;) {
		/* use cmd_wait values? */
		i = 50000;
		/* XXX spl0(); */
		while (((istat = rp->siop_istat) &
		    (SIOP_ISTAT_SIP | SIOP_ISTAT_DIP)) == 0) {
			if (--i <= 0) {
#ifdef DEBUG
				printf ("waiting: tgt %d cmd %02x sbcl %02x istat %02x sbdl %04x\n         dsp %lx (+%lx) dcmd %lx ds %p timeout %d\n",
				    xs->xs_periph->periph_target, acb->cmd.opcode,
				    rp->siop_sbcl, istat, rp->siop_sbdl, rp->siop_dsp,
				    rp->siop_dsp - sc->sc_scriptspa,
				    *((volatile long *)&rp->siop_dcmd), &acb->ds, acb->xs->timeout);
#endif
				i = 50000;
				--to;
				if (to <= 0) {
					siopngreset(sc);
					return;
				}
			}
			delay(20);
		}
		sist = rp->siop_sist;
		dstat = rp->siop_dstat;
		if (siopng_checkintr(sc, istat, dstat, sist, &status)) {
			if (acb != sc->sc_nexus)
				printf("%s: siopng_poll disconnected device completed\n",
				    device_xname(sc->sc_dev));
			else if ((sc->sc_flags & SIOP_INTDEFER) == 0) {
				sc->sc_flags &= ~SIOP_INTSOFF;
				rp->siop_sien = sc->sc_sien;
				rp->siop_dien = sc->sc_dien;
			}
			siopng_scsidone(sc->sc_nexus, status);
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
siopng_sched(struct siop_softc *sc)
{
	struct scsipi_periph *periph;
	struct siop_acb *acb;
	int i;

#ifdef DEBUG
	if (sc->sc_nexus) {
		printf("%s: siopng_sched- nexus %p/%d ready %p/%d\n",
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
		printf("%s: siopng_sched didn't find ready command\n",
		    device_xname(sc->sc_dev));
#endif
		return;
	}

	if (acb->xs->xs_control & XS_CTL_RESET)
		siopngreset(sc);

#if 0
	acb->cmd.bytes[0] |= periph->periph_lun << 5;	/* XXXX */
#endif
	++sc->sc_active;
	siopng_select(sc);
}

void
siopng_scsidone(struct siop_acb *acb, int stat)
{
	struct scsipi_xfer *xs;
	struct scsipi_periph *periph;
	struct siop_softc *sc;
	int dosched = 0;

	if (acb == NULL || (xs = acb->xs) == NULL) {
#ifdef DIAGNOSTIC
		printf("siopng_scsidone: NULL acb or scsipi_xfer\n");
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
		siopng_sched(sc);
}

void
siopngabort(register struct siop_softc *sc, siop_regmap_p rp, const char *where)
{
#ifdef fix_this
	int i;
#endif

	printf ("%s: abort %s: dstat %02x, istat %02x sist %04x sien %04x sbcl %02x\n",
	    device_xname(sc->sc_dev),
	    where, rp->siop_dstat, rp->siop_istat, rp->siop_sist,
	    rp->siop_sien, rp->siop_sbcl);
	siopng_dump_registers(sc);

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
				printf ("siopngabort: cleanup!\n");
				sc->sc_iob[i].sc_xs = NULL;
			}
		}
#endif	/* fix_this */
/*		sc->sc_active = 0; */
	}
}

void
siopnginitialize(struct siop_softc *sc)
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
	sc->sc_scriptspa = kvtop((void *)__UNCONST(siopng_scripts));

	/*
	 * malloc sc_acb to ensure that DS is on a long word boundary.
	 */

	sc->sc_acb = malloc(sizeof(struct siop_acb) * SIOP_NACB,
		M_DEVBUF, M_NOWAIT);
	if (sc->sc_acb == NULL)
		panic("siopnginitialize: ACB malloc failed!");

	sc->sc_tcp[1] = 1000 / sc->sc_clock_freq;
	sc->sc_tcp[2] = 1500 / sc->sc_clock_freq;
	sc->sc_tcp[3] = 2000 / sc->sc_clock_freq;
	sc->sc_minsync = sc->sc_tcp[1];		/* in 4ns units */
	if (sc->sc_minsync < 25)
		sc->sc_minsync = 25;
	sc->sc_minsync >>= 1;			/* Using clock doubler, allow Ultra */
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
		inhibit_sync = (scsi_nosync >> shift_nosync) & 0xffff;
		shift_nosync += 16;		/* XXX maxtarget */
#ifdef DEBUG
		if (inhibit_sync)
			printf("%s: Inhibiting synchronous transfer %02x\n",
				device_xname(sc->sc_dev), inhibit_sync);
#endif
		for (i = 0; i < 16; ++i)		/* XXX maxtarget */
			if (inhibit_sync & (1 << i))
				siopng_inhibit_sync[i] = 1;
	}

	siopngreset (sc);
}

void
siopng_timeout(void *arg)
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
	siopngreset(sc);

	splx(s);
}

void
siopngreset(struct siop_softc *sc)
{
	siop_regmap_p rp;
	u_int i, s;
	u_short  dummy;
	struct siop_acb *acb;

	rp = sc->sc_siopp;

	if (sc->sc_flags & SIOP_ALIVE)
		siopngabort(sc, rp, "reset");

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
	rp->siop_stest1 |= SIOP_STEST1_DBLEN;	/* SCLK doubler enable */
	delay(20);
	rp->siop_stest3 |= SIOP_STEST3_HSC;	/* Halt SCSI clock */
	rp->siop_scntl3 = 0x15;			/* SCF/CCF*/
	rp->siop_stest1 |= SIOP_STEST1_DBLSEL;	/* SCLK doubler select */
	rp->siop_stest3 &= ~SIOP_STEST3_HSC;	/* Clear Halt SCSI clock */
	rp->siop_scntl0 = SIOP_ARB_FULL | /*SIOP_SCNTL0_EPC |*/ SIOP_SCNTL0_EPG;
	rp->siop_dcntl = sc->sc_dcntl;
	rp->siop_dmode = 0xc0;		/* XXX burst length */
	rp->siop_sien = 0x00;	/* don't enable interrupts yet */
	rp->siop_dien = 0x00;	/* don't enable interrupts yet */
	rp->siop_scid = sc->sc_channel.chan_id |
	    SIOP_SCID_RRE | SIOP_SCID_SRE;
	rp->siop_respid = 1 << sc->sc_channel.chan_id;
	rp->siop_dwt = 0x00;
	rp->siop_stime0 = 0x0c;		/* XXXXX check */

	/* will need to re-negotiate sync xfers */
	memset(&sc->sc_sync, 0, sizeof (sc->sc_sync));

	i = rp->siop_istat;
	if (i & SIOP_ISTAT_SIP)
		dummy = rp->siop_sist;
	if (i & SIOP_ISTAT_DIP)
		dummy = rp->siop_dstat;

	__USE(dummy);
	splx (s);

	delay (siopng_reset_delay * 1000);

	/*
	 * is lower half unterminated?
	 */
	if ((rp->siop_sbdl & 0x00ff) == 0x00ff) {
		printf(" no SCSI termination, host adapter deactivated.\n");
		sc->sc_channel.chan_ntargets = 0;	/* XXX */
		sc->sc_flags &= ~(SIOP_ALIVE|SIOP_INTDEFER|SIOP_INTSOFF);
		/* disable SCSI and DMA interrupts */
		sc->sc_sien = 0;
		sc->sc_dien = 0;
		rp->siop_sien = sc->sc_sien;
		rp->siop_dien = sc->sc_dien;

		return;
	}

	/*
	 * Check if upper half of SCSI bus is unterminated, and disallow
	 * disconnections if it appears to be unterminated.
	 */
	if ((rp->siop_sbdl & 0xff00) == 0xff00) {
		printf(" NO WIDE TERM");
		/* XXX need to restrict maximum target ID as well? */
		sc->sc_channel.chan_ntargets = 8;
		for (i = 0; i < 16; ++i) {
			siopng_allow_disc[i] = 0;
			siopng_inhibit_wide[i] |= 0x80;
		}
	}

	printf("siopng type %s id %d reset V%d\n",
	    siopng_chips[rp->siop_macntl>>4],
	    sc->sc_channel.chan_ntargets,
	    rp->siop_ctest3 >> 4);

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
			siopng_scsidone(sc->sc_nexus, sc->sc_nexus->stat[0]);
		}
		while ((acb = sc->nexus_list.tqh_first) > 0) {
			acb->xs->error = XS_RESET;
			siopng_scsidone(acb, acb->stat[0]);
		}
	}

	sc->sc_flags |= SIOP_ALIVE;
	sc->sc_flags &= ~(SIOP_INTDEFER|SIOP_INTSOFF);
	/* enable SCSI and DMA interrupts */
	sc->sc_sien = SIOP_SIEN_MA | SIOP_SIEN_STO | /*SIOP_SIEN_GEN |*/
	    /*SIOP_SIEN_SEL |*/ SIOP_SIEN_SGE | SIOP_SIEN_UDC |
	     SIOP_SIEN_RST | SIOP_SIEN_PAR;
	sc->sc_dien = SIOP_DIEN_BF | SIOP_DIEN_ABRT | SIOP_DIEN_SIR |
	    /*SIOP_DIEN_WTD |*/ SIOP_DIEN_IID;
	rp->siop_sien = sc->sc_sien;
	rp->siop_dien = sc->sc_dien;
	/*siopng_dump_registers(sc);*/
}

/*
 * Setup Data Storage for 53C720/770 and start SCRIPTS processing
 */

void
siopng_start(struct siop_softc *sc, int target, int lun, u_char *cbuf,
             int clen, u_char *buf, int len)
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
	if (siopng_debug & 0x100 && rp->siop_sbcl & SIOP_BSY) {
		printf ("ACK! siopng was busy: rp %p script %p dsa %p active %ld\n",
		    rp, &siopng_scripts, &acb->ds, sc->sc_active);
		printf ("istat %02x sfbr %02x respid %02x sien %04x dien %02x\n",
		    rp->siop_istat, rp->siop_sfbr, rp->siop_respid,
		    rp->siop_sien, rp->siop_dien);
#ifdef DDB
		/*Debugger();*/
#endif
	}
#endif
	acb->msgout[0] = MSG_IDENTIFY | lun;
	if (siopng_allow_disc[target] & 2 ||
	    (siopng_allow_disc[target] && len == 0))
		acb->msgout[0] = MSG_IDENTIFY_DR | lun;
	acb->status = 0;
	acb->stat[0] = -1;
	acb->msg[0] = -1;
	acb->ds.scsi_addr = (target << 16) | (sc->sc_sync[target].sxfer << 8) |
	    (sc->sc_sync[target].scntl3 << 24);
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
	acb->ds.msginbuf = acb->ds.msgbuf + 1;
	acb->ds.extmsgbuf = acb->ds.msginbuf + 1;
	acb->ds.synmsgbuf = acb->ds.extmsgbuf + 1;
	memset(&acb->ds.chain, 0, sizeof (acb->ds.chain));

	if (sc->sc_sync[target].state == NEG_WIDE) {
		if (siopng_inhibit_wide[target]) {
			sc->sc_sync[target].state = NEG_SYNC;
			sc->sc_sync[target].scntl3 &= ~SIOP_SCNTL3_EWS;
#ifdef DEBUG
			if (siopngsync_debug)
				printf ("Forcing target %d narrow\n", target);
#endif
		}
		else {
			sc->sc_sync[target].scntl3 = 0x15 |	/* XXX */
			    (sc->sc_sync[target].scntl3 & 0x88);	/* XXX */
			acb->msg[2] = -1;
			acb->msgout[1] = MSG_EXT_MESSAGE;
			acb->msgout[2] = 2;
			acb->msgout[3] = MSG_WIDE_REQ;
			acb->msgout[4] = 1;
			acb->ds.idlen = 5;
			acb->ds.synmsglen = 2;
			sc->sc_sync[target].state = NEG_WAITW;
#ifdef DEBUG
			if (siopngsync_debug)
				printf("Sending wide request to target %d\n", target);
#endif
		}
	}
	if (sc->sc_sync[target].state == NEG_SYNC) {
		if (siopng_inhibit_sync[target]) {
			sc->sc_sync[target].state = NEG_DONE;
			sc->sc_sync[target].scntl3 = 5 |		/* XXX */
			    (sc->sc_sync[target].scntl3 & 0x88);	/* XXX */
			sc->sc_sync[target].sxfer = 0;
#ifdef DEBUG
			if (siopngsync_debug)
				printf ("Forcing target %d asynchronous\n", target);
#endif
		}
		else {
			sc->sc_sync[target].scntl3 = 0x15 |	/* XXX */
			    (sc->sc_sync[target].scntl3 & 0x88);	/* XXX */
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
			if (siopngsync_debug)
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

#if DEBUG_ONLY_IF_DESPERATE
		printf("chain[%d]: count %d tcount %d vaddr %p paddr %p\n",
			nchain, count, tcount, addr,
			acb->ds.chain[nchain].databuf);
#endif
		acb->ds.chain[nchain].datalen = tcount;
		addr += tcount;
		count -= tcount;
		if (acb->ds.chain[nchain].databuf == dmaend) {
			dmaend += acb->ds.chain[nchain].datalen;
			acb->ds.chain[nchain].datalen = 0;
			acb->ds.chain[--nchain].datalen += tcount;
#ifdef DEBUG
			++siopngdma_hits;
#endif
		}
		else {
			dmaend = acb->ds.chain[nchain].databuf +
			    acb->ds.chain[nchain].datalen;
			acb->ds.chain[nchain].datalen = tcount;
#ifdef DEBUG
			if (nchain)	/* Don't count miss on first one */
				++siopngdma_misses;
#endif
		}
		++nchain;
	}
#ifdef DEBUG
	if (nchain != 1 && len != 0 && siopng_debug & 3) {
		printf ("DMA chaining set: %d\n", nchain);
		for (i = 0; i < nchain; ++i) {
			printf ("  [%d] %8p %lx\n", i, acb->ds.chain[i].databuf,
			    acb->ds.chain[i].datalen);
		}
	}
#endif

	/* push data cache for all data the 53c720/770 needs to access */
	dma_cachectl ((void *)acb, sizeof (struct siop_acb));
	dma_cachectl (cbuf, clen);
	if (buf != NULL && len != 0)
		dma_cachectl (buf, len);
#ifdef DEBUG
	if (siopng_debug & 0x100 && rp->siop_sbcl & SIOP_BSY) {
		printf ("ACK! siopng was busy at start: rp %p script %p dsa %p active %ld\n",
		    rp, &siopng_scripts, &acb->ds, sc->sc_active);
#ifdef DDB
		/*Debugger();*/
#endif
	}
#endif
	if (sc->nexus_list.tqh_first == NULL) {
		callout_reset(&acb->xs->xs_callout,
		    mstohz(acb->xs->timeout) + 1, siopng_timeout, acb);
		if (rp->siop_istat & SIOP_ISTAT_CON)
			printf("%s: siopng_select while connected?\n",
			    device_xname(sc->sc_dev));
		rp->siop_temp = 0;
#ifndef FIXME
		rp->siop_scntl3 = sc->sc_sync[target].scntl3;
#endif
		amiga_membarrier();
		rp->siop_dsa = kvtop((void *)&acb->ds);
		amiga_membarrier();
		rp->siop_dsp = sc->sc_scriptspa;
		amiga_membarrier();
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
	++siopngstarts;
#endif
}

/*
 * Process a DMA or SCSI interrupt from the 53C720/770 SIOP
 */

int
siopng_checkintr(struct siop_softc *sc, u_char istat, u_char dstat,
                 u_short sist, int *status)
{
	siop_regmap_p rp = sc->sc_siopp;
	struct siop_acb *acb = sc->sc_nexus;
	int	target = 0;
	int	dfifo, dbc, sstat0, sstat1, sstat2;

	dfifo = rp->siop_dfifo;
	dbc = rp->siop_dbc0;
	sstat0 = rp->siop_sstat0;
	sstat1 = rp->siop_sstat1;
	__USE(sstat1);
	sstat2 = rp->siop_sstat2;
	rp->siop_ctest3 |= SIOP_CTEST8_CLF;
	while ((rp->siop_ctest1 & SIOP_CTEST1_FMT) != SIOP_CTEST1_FMT)
		;
	rp->siop_ctest3 &= ~SIOP_CTEST8_CLF;
#ifdef DEBUG
	++siopngints;
#if 0
	if (siopng_debug & 0x100) {
		DCIAS(&acb->stat[0]);	/* XXX */
		printf ("siopngchkintr: istat %x dstat %x sist %x dsps %x sbcl %x sts %x msg %x\n",
		    istat, dstat, sist, rp->siop_dsps, rp->siop_sbcl, acb->stat[0], acb->msg[0]);
		printf ("sync msg in: %02x %02x %02x %02x %02x %02x\n",
		    acb->msg[0], acb->msg[1], acb->msg[2],
		    acb->msg[3], acb->msg[4], acb->msg[5]);
	}
#endif
	if (rp->siop_dsp && (rp->siop_dsp < sc->sc_scriptspa ||
	    rp->siop_dsp >= sc->sc_scriptspa + sizeof(siopng_scripts))) {
		printf ("%s: dsp not within script dsp %lx scripts %lx:%lx",
		    device_xname(sc->sc_dev), rp->siop_dsp, sc->sc_scriptspa,
		    sc->sc_scriptspa + sizeof(siopng_scripts));
		printf(" istat %x dstat %x sist %x\n",
		    istat, dstat, sist);
#ifdef DDB
		Debugger();
#endif
	}
#endif
	SIOP_TRACE('i',dstat,istat,(istat&SIOP_ISTAT_DIP)?rp->siop_dsps&0xff:sist);
	if (dstat & SIOP_DSTAT_SIR && rp->siop_dsps == 0xff00) {
		/* Normal completion status, or check condition */
#ifdef DEBUG
		if (rp->siop_dsa != kvtop((void *)&acb->ds)) {
			printf ("siopng: invalid dsa: %lx %x\n", rp->siop_dsa,
			    (unsigned)kvtop((void *)&acb->ds));
			panic("*** siopng DSA invalid ***");
		}
#endif
		target = acb->xs->xs_periph->periph_target;
		if (sc->sc_sync[target].state == NEG_WAITW) {
			if (acb->msg[1] == 0xff)
				printf ("%s: target %d ignored wide request\n",
				    device_xname(sc->sc_dev), target);
			else if (acb->msg[1] == MSG_REJECT)
				printf ("%s: target %d rejected wide request\n",
				    device_xname(sc->sc_dev), target);
			else {
				printf("%s: target %d (wide) %02x %02x %02x %02x\n",
				    device_xname(sc->sc_dev), target, acb->msg[1],
				    acb->msg[2], acb->msg[3], acb->msg[4]);
				if (acb->msg[1] == MSG_EXT_MESSAGE &&
				    acb->msg[2] == 2 &&
				    acb->msg[3] == MSG_WIDE_REQ)
					sc->sc_sync[target].scntl3 = acb->msg[4] ?
					    sc->sc_sync[target].scntl3 | SIOP_SCNTL3_EWS :
					    sc->sc_sync[target].scntl3 & ~SIOP_SCNTL3_EWS;
			}
			sc->sc_sync[target].state = NEG_SYNC;
		}
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
			    rp, &siopng_scripts, &acb->ds);*/
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
		if (acb->msg[1] == MSG_EXT_MESSAGE && acb->msg[2] == 2 &&
		    acb->msg[3] == MSG_WIDE_REQ) {
#ifdef DEBUG
			if (siopngsync_debug)
				printf ("wide msg in: %02x %02x %02x %02x %02x %02x\n",
				    acb->msg[0], acb->msg[1], acb->msg[2],
				    acb->msg[3], acb->msg[4], acb->msg[5]);
#endif
			sc->sc_sync[target].scntl3 &= ~(SIOP_SCNTL3_EWS);
			if (acb->msg[2] == 2 &&
			    acb->msg[3] == MSG_WIDE_REQ &&
			    acb->msg[4] != 0) {
				sc->sc_sync[target].scntl3 |= SIOP_SCNTL3_EWS;
				printf ("%s: target %d now wide %d\n",
				    device_xname(sc->sc_dev), target,
				    acb->msg[4]);
			}
			rp->siop_scntl3 = sc->sc_sync[target].scntl3;
			amiga_membarrier();
			if (sc->sc_sync[target].state == NEG_WAITW) {
				sc->sc_sync[target].state = NEG_SYNC;
				rp->siop_dsp = sc->sc_scriptspa + Ent_clear_ack;
				amiga_membarrier();
				return(0);
			}
			rp->siop_dcntl |= SIOP_DCNTL_STD;
			amiga_membarrier();
			sc->sc_sync[target].state = NEG_SYNC;
			return (0);
		}
		if (acb->msg[1] == MSG_EXT_MESSAGE && acb->msg[2] == 3 &&
		    acb->msg[3] == MSG_SYNC_REQ) {
#ifdef DEBUG
			if (siopngsync_debug)
				printf ("sync msg in: %02x %02x %02x %02x %02x %02x\n",
				    acb->msg[0], acb->msg[1], acb->msg[2],
				    acb->msg[3], acb->msg[4], acb->msg[5]);
#endif
			sc->sc_sync[target].sxfer = 0;
			sc->sc_sync[target].scntl3 = 5 |		/* XXX */
			    (sc->sc_sync[target].scntl3 & 0x88);	/* XXX */
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
				    (acb->msg[4] == 12) ? 50 : acb->msg[4] * 4,
				    acb->msg[5]);
				scsi_period_to_siopng (sc, target);
			}
			rp->siop_sxfer = sc->sc_sync[target].sxfer;
			rp->siop_scntl3 = sc->sc_sync[target].scntl3;
			amiga_membarrier();
			if (sc->sc_sync[target].state == NEG_WAITS) {
				sc->sc_sync[target].state = NEG_DONE;
				rp->siop_dsp = sc->sc_scriptspa + Ent_clear_ack;
				amiga_membarrier();
				return(0);
			}
			rp->siop_dcntl |= SIOP_DCNTL_STD;
			amiga_membarrier();
			sc->sc_sync[target].state = NEG_DONE;
			return (0);
		}
		/* XXX - not SDTR message */
	}
	if (sist & SIOP_SIST_MA) {		/* Phase mismatch */
#ifdef DEBUG
		++siopngphmm;
		if (acb == NULL)
			printf("%s: Phase mismatch with no active command?\n",
			    device_xname(sc->sc_dev));
#endif
		if (acb->iob_len) {
			int adjust;
			adjust = ((dfifo - (dbc & 0x7f)) & 0x7f);
			if (sstat0 & SIOP_SSTAT0_OLF)	/* sstat0 SODL lsb */
				++adjust;
			if (sstat0 & SIOP_SSTAT0_ORF)	/* sstat0 SODR lsb */
				++adjust;
			if (sstat2 & SIOP_SSTAT2_OLF1)	/* sstat2 SODL msb */
				++adjust;
			if (sstat2 & SIOP_SSTAT2_ORF1)	/* sstat2 SODR msb */
				++adjust;
			acb->iob_curlen = 
			    *((long *)__UNVOLATILE(&rp->siop_dcmd)) & 0xffffff;
			acb->iob_curlen += adjust;
			acb->iob_curbuf = 
			    *((long *)__UNVOLATILE(&rp->siop_dnad)) - adjust;
#ifdef DEBUG
			if (siopng_debug & 0x100) {
				int i;
				printf ("Phase mismatch: curbuf %lx curlen %lx dfifo %x dbc %x sstat1 %x adjust %x sbcl %x starts %d acb %p\n",
				    acb->iob_curbuf, acb->iob_curlen, dfifo,
				    dbc, sstat1, adjust, rp->siop_sbcl, siopngstarts, acb);
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
		if (siopng_debug & 9)
			printf ("Phase mismatch: %x dsp +%lx dcmd %lx\n",
			    rp->siop_sbcl,
			    rp->siop_dsp - sc->sc_scriptspa,
			    *((volatile long *)&rp->siop_dcmd));
#endif
		if ((rp->siop_sbcl & SIOP_REQ) == 0) {
			printf ("Phase mismatch: REQ not asserted! %02x dsp %lx\n",
			    rp->siop_sbcl, rp->siop_dsp);
#if defined(DEBUG) && defined(DDB)
			Debugger();
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
			amiga_membarrier();
			break;
		default:
			goto bad_phase;
		}
		return 0;
	}
	if (sist & SIOP_SIST_STO) {		/* Select timed out */
#ifdef DEBUG
		if (acb == NULL)
			printf("%s: Select timeout with no active command?\n",
			    device_xname(sc->sc_dev));
		if (rp->siop_sbcl & SIOP_BSY) {
			printf ("ACK! siop was busy at timeout: rp %p script %p dsa %p\n",
			    rp, &siopng_scripts, &acb->ds);
			printf(" sbcl %x sdid %x istat %x dstat %x sist %x\n",
			    rp->siop_sbcl, rp->siop_sdid, istat, dstat, sist);
			if (!(rp->siop_sbcl & SIOP_BSY)) {
				printf ("Yikes, it's not busy now!\n");
#if 0
				*status = -1;
				if (sc->nexus_list.tqh_first) {
					rp->siop_dsp = sc->sc_scriptspa + Ent_wait_reselect;
					amiga_membarrier();
				}
				return 1;
#endif
			}
/*			rp->siop_dcntl |= SIOP_DCNTL_STD;*/
			return (0);
		}
#endif
		*status = -1;
		acb->xs->error = XS_SELTIMEOUT;
		if (sc->nexus_list.tqh_first) {
			rp->siop_dsp = sc->sc_scriptspa + Ent_wait_reselect;
			amiga_membarrier();
		}
		return 1;
	}
	if (acb)
		target = acb->xs->xs_periph->periph_target;
	else
		target = 7;
	if (sist & SIOP_SIST_UDC) {
#ifdef DEBUG
		if (acb == NULL)
			printf("%s: Unexpected disconnect with no active command?\n",
			    device_xname(sc->sc_dev));
		printf ("%s: target %d disconnected unexpectedly\n",
		   device_xname(sc->sc_dev), target);
siopng_dump_registers(sc);
siopng_dump(sc);
#endif
#if 0
		siopngabort (sc, rp, "siopngchkintr");
#endif
		*status = STS_BUSY;
		if (sc->nexus_list.tqh_first) {
			rp->siop_dsp = sc->sc_scriptspa + Ent_wait_reselect;
			amiga_membarrier();
		}
		return (acb != NULL);
	}
	if (dstat & SIOP_DSTAT_SIR && (rp->siop_dsps == 0xff01 ||
	    rp->siop_dsps == 0xff02)) {
#ifdef DEBUG
		if (siopng_debug & 0x100)
			printf ("%s: ID %02x disconnected TEMP %lx (+%lx) curbuf %lx curlen %lx buf %p len %lx dfifo %x dbc %x sstat1 %x starts %d acb %p\n",
			    device_xname(sc->sc_dev), 1 << target, rp->siop_temp,
			    rp->siop_temp ? rp->siop_temp - sc->sc_scriptspa : 0,
			    acb->iob_curbuf, acb->iob_curlen,
			    acb->ds.chain[0].databuf, acb->ds.chain[0].datalen, dfifo, dbc, sstat1, siopngstarts, acb);
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
			if (siopng_debug & 0x100) {
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
			if (siopng_debug & 0x100)
				printf ("%s: adjusting DMA chain\n",
				    device_xname(sc->sc_dev));
			if (rp->siop_dsps == 0xff02)
				printf ("%s: ID %02x disconnected without Save Data Pointers\n",
				    device_xname(sc->sc_dev), 1 << target);
#endif
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
#ifdef DEBUG
			if (siopng_debug & 0x100)
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
			if (siopng_debug & 0x100)
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
		if (sc->sc_nexus == NULL) {
			rp->siop_dsp = sc->sc_scriptspa + Ent_wait_reselect;
			amiga_membarrier();
		}
/* XXXX start another command ? */
		if (sc->ready_list.tqh_first)
			siopng_sched(sc);
#if 0
		else
			rp->siop_dcntl |= SIOP_DCNTL_STD;
#endif
		return (0);
	}
	if (dstat & SIOP_DSTAT_SIR && rp->siop_dsps == 0xff03) {
		int reselid = rp->siop_scratcha & 0x7f;
		int reselun = rp->siop_sfbr & 0x07;

		sc->sc_sstat1 = rp->siop_sbcl;	/* XXXX save current SBCL */
#ifdef DEBUG
		if (siopng_debug & 0x100)
			printf ("%s: target ID %02x reselected dsps %lx\n",
			     device_xname(sc->sc_dev), reselid,
			     rp->siop_dsps);
		if ((rp->siop_sfbr & 0x80) == 0)
			printf("%s: Reselect message in was not identify: %x\n",
			    device_xname(sc->sc_dev), rp->siop_sfbr);
#endif
		if (sc->sc_nexus) {
#ifdef DEBUG
			if (siopng_debug & 0x100)
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
			if (reselid != ((acb->ds.scsi_addr >> 16) & 0xff) ||
			    reselun != (acb->msgout[0] & 0x07))
				continue;
			TAILQ_REMOVE(&sc->nexus_list, acb, chain);
			sc->sc_nexus = acb;
			sc->sc_flags |= acb->status;
			acb->status = 0;
			DCIAS(kvtop(&acb->stat[0]));
			rp->siop_dsa = kvtop((void *)&acb->ds);
			amiga_membarrier();
			rp->siop_sxfer =
				sc->sc_sync[acb->xs->xs_periph->periph_target].sxfer;
#ifndef FIXME
			rp->siop_scntl3 =
				sc->sc_sync[acb->xs->xs_periph->periph_target].scntl3;
			amiga_membarrier();
#endif
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
		amiga_membarrier();
		return (0);
	}
	if (dstat & SIOP_DSTAT_SIR && rp->siop_dsps == 0xff04) {
#ifdef DEBUG
		u_short ctest2 = rp->siop_ctest2;

		/* reselect was interrupted (by Sig_P or select) */
		if (siopng_debug & 0x100 ||
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
			siopng_dump(sc);
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
		amiga_membarrier();
		rp->siop_sxfer = sc->sc_sync[target].sxfer;
#ifndef FIXME
		rp->siop_scntl3 = sc->sc_sync[target].scntl3;
		amiga_membarrier();
#endif
		rp->siop_dsp = sc->sc_scriptspa;
		amiga_membarrier();
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
		amiga_membarrier();
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
	if (sist == 0 && dstat & SIOP_DSTAT_SIR) {
		dma_cachectl (&acb->stat[0], 1);
		dma_cachectl (&acb->msg[0], 1);
		printf ("SIOP interrupt: %lx sts %x msg %x %x sbcl %x\n",
		    rp->siop_dsps, acb->stat[0], acb->msg[0], acb->msg[1],
		    rp->siop_sbcl);
		siopngreset (sc);
		*status = -1;
		return 0;	/* siopngreset has cleaned up */
	}
	if (sist & SIOP_SIST_SGE)
		printf ("SIOP: SCSI Gross Error\n");
	if (sist & SIOP_SIST_PAR)
		printf ("SIOP: Parity Error\n");
	if (dstat & SIOP_DSTAT_IID)
		printf ("SIOP: Invalid instruction detected\n");
bad_phase:
	/*
	 * temporary panic for unhandled conditions
	 * displays various things about the 53C720/770 status and registers
	 * then panics.
	 * XXXX need to clean this up to print out the info, reset, and continue
	 */
	printf ("siopngchkintr: target %x ds %p\n", target, &acb->ds);
	printf ("scripts %lx ds %x rp %x dsp %lx dcmd %lx\n",
	    sc->sc_scriptspa, (unsigned)kvtop((void *)&acb->ds),
	    (unsigned)kvtop((void *)__UNVOLATILE(rp)), rp->siop_dsp,
	    *((long *)__UNVOLATILE(&rp->siop_dcmd)));
	printf ("siopngchkintr: istat %x dstat %x sist %x dsps %lx dsa %lx sbcl %x sts %x msg %x %x sfbr %x\n",
	    istat, dstat, sist, rp->siop_dsps, rp->siop_dsa,
	     rp->siop_sbcl, acb->stat[0], acb->msg[0], acb->msg[1], rp->siop_sfbr);
#ifdef DEBUG
	if (siopng_debug & 0x20)
		panic("siopngchkintr: **** temp ****");
#endif
#ifdef DDB
	Debugger ();
#endif
	siopngreset (sc);		/* hard reset */
	*status = -1;
	return 0;		/* siopngreset cleaned up */
}

void
siopng_select(struct siop_softc *sc)
{
	siop_regmap_p rp;
	struct siop_acb *acb = sc->sc_nexus;

#ifdef DEBUG
	if (siopng_debug & 1)
		printf ("%s: select ", device_xname(sc->sc_dev));
#endif

	rp = sc->sc_siopp;
	if (acb->xs->xs_control & XS_CTL_POLL || siopng_no_dma) {
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
	if (siopng_debug & 1)
		printf ("siopng_select: target %x cmd %02x ds %p\n",
		    acb->xs->xs_periph->periph_target, acb->cmd.opcode,
		    &sc->sc_nexus->ds);
#endif

	siopng_start(sc, acb->xs->xs_periph->periph_target,
		acb->xs->xs_periph->periph_lun,
	    (u_char *)&acb->cmd, acb->clen, acb->daddr, acb->dleft);

	return;
}

/*
 * 53C720/770 interrupt handler
 */

void
siopngintr(register struct siop_softc *sc)
{
	siop_regmap_p rp;
	u_char istat, dstat;
	u_short sist;
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
	sist = sc->sc_sist;
	if (dstat & SIOP_DSTAT_SIR)
		sc->sc_intcode = rp->siop_dsps;
	sc->sc_istat = 0;
#ifdef DEBUG
	if (siopng_debug & 1)
		printf ("%s: intr istat %x dstat %x sist %x\n",
		    device_xname(sc->sc_dev), istat, dstat, sist);
	if (!sc->sc_active) {
		printf ("%s: spurious interrupt? istat %x dstat %x sist %x nexus %p status %x\n",
		    device_xname(sc->sc_dev), istat, dstat, sist,
		    sc->sc_nexus, sc->sc_nexus ? sc->sc_nexus->stat[0] : 0);
	}
#endif

#ifdef DEBUG
	if (siopng_debug & 5) {
		DCIAS(kvtop(&sc->sc_nexus->stat[0]));
		printf ("%s: intr istat %x dstat %x sist %x dsps %lx sbcl %x sts %x msg %x\n",
		    device_xname(sc->sc_dev), istat, dstat, sist,
		    rp->siop_dsps,  rp->siop_sbcl,
		    sc->sc_nexus->stat[0], sc->sc_nexus->msg[0]);
	}
#endif
	if (sc->sc_flags & SIOP_INTDEFER) {
		sc->sc_flags &= ~(SIOP_INTDEFER | SIOP_INTSOFF);
		rp->siop_sien = sc->sc_sien;
		rp->siop_dien = sc->sc_dien;
	}
	if (siopng_checkintr (sc, istat, dstat, sist, &status)) {
#if 1
		if (status == 0xff)
			printf ("siopngintr: status == 0xff\n");
#endif
		if ((sc->sc_flags & (SIOP_INTSOFF | SIOP_INTDEFER)) != SIOP_INTSOFF) {
#if 0
			if (rp->siop_sbcl & SIOP_BSY) {
				printf ("%s: SCSI bus busy at completion",
					device_xname(sc->sc_dev));
				printf(" targ %d sbcl %02x sfbr %x respid %02x dsp +%x\n",
				    sc->sc_nexus->xs->xs_periph->periph_target,
				    rp->siop_sbcl, rp->siop_sfbr, rp->siop_respid,
				    rp->siop_dsp - sc->sc_scriptspa);
			}
#endif
			siopng_scsidone(sc->sc_nexus, sc->sc_nexus ?
			    sc->sc_nexus->stat[0] : -1);
		}
	}
	splx(s);
}

/*
 * This is based on the Progressive Peripherals 33 MHz Zeus driver and will
 * not be correct for other 53c710 boards.
 *
 */
void
scsi_period_to_siopng(struct siop_softc *sc, int target)
{
	int period, offset, sxfer, scntl3 = 0;

	period = sc->sc_nexus->msg[4];
	offset = sc->sc_nexus->msg[5];
#ifdef FIXME
	for (scntl3 = 1; scntl3 < 4; ++scntl3) {
		sxfer = (period * 4 - 1) / sc->sc_tcp[scntl3] - 3;
		if (sxfer >= 0 && sxfer <= 7)
			break;
	}
	if (scntl3 > 3) {
		printf("siopng sync: unable to compute sync params for period %dns\n",
		    period * 4);
		/*
		 * XXX need to pick a value we can do and renegotiate
		 */
		sxfer = scntl3 = 0;
	} else {
		sxfer = (sxfer << 4) | ((offset <= SIOP_MAX_OFFSET) ?
		    offset : SIOP_MAX_OFFSET);
#ifdef DEBUG_SYNC
		printf("siopng sync: params for period %dns: sxfer %x scntl3 %x",
		    period * 4, sxfer, scntl3);
		printf(" actual period %dns\n",
		    sc->sc_tcp[scntl3] * ((sxfer >> 4) + 4));
#endif /* DEBUG_SYNC */
	}
#else /* FIXME */
	sxfer = offset <= SIOP_MAX_OFFSET ? offset : SIOP_MAX_OFFSET;
	sxfer |= 0x20;		/* XXX XFERP: 5 */
#ifndef FIXME
	if (period <= (50 / 4))	/* XXX */
		scntl3 = 0x95;	/* Ultra, SCF: /1, CCF: /4 */
	else if (period <= (100 / 4))
		scntl3 = 0x35;	/* SCF: /2, CCF: /4 */
	else if (period <= (200 / 4))
		scntl3 = 0x55;	/* SCF: /4, CCF: /4 */
	else
		scntl3 = 0xff;	/* XXX ??? */
#else
	scntl3 = 5;
#endif
#endif
	sc->sc_sync[target].sxfer = sxfer;
	sc->sc_sync[target].scntl3 = scntl3 |
	    (sc->sc_sync[target].scntl3 & SIOP_SCNTL3_EWS);
#ifdef DEBUG_SYNC
	printf ("siopng sync: siop_sxfr %02x, siop_scntl3 %02x\n", sxfer, scntl3);
#endif
}

void
siopng_dump_registers(struct siop_softc *sc)
{
	siop_regmap_p rp = sc->sc_siopp;

	printf("  scntl0   %02x scntl1 %02x scntl2 %02x scntl3 %02x\n",
	    rp->siop_scntl0, rp->siop_scntl1, rp->siop_scntl2, rp->siop_scntl3);
	printf("  scid     %02x sxfer  %02x sdid   %02x gpreg  %02x\n",
	    rp->siop_scid, rp->siop_sxfer, rp->siop_sdid, rp->siop_gpreg);
	printf("  sfbr     %02x socl   %02x ssid   %02x sbcl   %02x\n",
	    rp->siop_sfbr, rp->siop_socl, rp->siop_ssid, rp->siop_sbcl);
	printf("  dstat    %02x sstat0 %02x sstat1 %02x sstat2 %02x\n",
	    rp->siop_dstat, rp->siop_sstat0, rp->siop_sstat1, rp->siop_sstat2);
	printf("  ctest0   %02x ctest1 %02x ctest2 %02x ctest3 %02x\n",
	    rp->siop_ctest0, rp->siop_ctest1, rp->siop_ctest2, rp->siop_ctest3);
	printf("  dfifo    %02x ctest4 %02x ctest5 %02x ctest6 %02x\n",
	    0, rp->siop_ctest4, rp->siop_ctest5, rp->siop_ctest6);
	printf("  dcmd     %02x dbc2   %02x dbc1   %02x dbc0   %02x\n",
	    rp->siop_dcmd, rp->siop_dbc2, rp->siop_dbc1, rp->siop_dbc0);
	printf("  dmode    %02x dien   %02x dwt    %02x dcntl  %02x\n",
	    rp->siop_dmode, rp->siop_dien, rp->siop_dwt, rp->siop_dcntl);
	printf("  stest0   %02x stest1 %02x stest2 %02x stest3 %02x\n",
	    rp->siop_stest0, rp->siop_stest1, rp->siop_stest2, rp->siop_stest3);
	printf("  istat    %02x sien %04x sist %04x respid %04x\n",
	    rp->siop_istat, rp->siop_sien, rp->siop_sist, rp->siop_respid);
	printf("  sidl   %04x sodl %04x sbdl %04x\n",
	    rp->siop_sidl, rp->siop_sodl, rp->siop_sbdl);
	printf("  dsps     %08lx dsp      %08lx (+%lx)\n",
	    rp->siop_dsps, rp->siop_dsp, rp->siop_dsp > sc->sc_scriptspa ?
	    rp->siop_dsp - sc->sc_scriptspa : 0);
	printf("  dsa      %08lx temp     %08lx dnad     %08lx\n",
	    rp->siop_dsa, rp->siop_temp, rp->siop_dnad);
	printf("  scratcha %08lx scratchb %08lx adder     %08lx\n",
	    rp->siop_scratcha, rp->siop_scratchb, rp->siop_adder);
}

#ifdef DEBUG

#if SIOP_TRACE_SIZE
void
siopng_dump_trace(void)
{
	int i;

	printf("siopng trace: next index %d\n", siopng_trix);
	i = siopng_trix;
	do {
		printf("%3d: '%c' %02x %02x %02x\n", i, siopng_trbuf[i],
		    siopng_trbuf[i + 1], siopng_trbuf[i + 2], siopng_trbuf[i + 3]);
		i = (i + 4) & (SIOP_TRACE_SIZE - 1);
	} while (i != siopng_trix);
}
#endif

void
siopng_dump_acb(struct siop_acb *acb)
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
siopng_dump(struct siop_softc *sc)
{
	struct siop_acb *acb;
	siop_regmap_p rp = sc->sc_siopp;
	int s;
	int i;

	s = splbio();
#if SIOP_TRACE_SIZE
	siopng_dump_trace();
#endif
	printf("%s@%p regs %p istat %x\n",
	    device_xname(sc->sc_dev), sc, rp, rp->siop_istat);
	if ((acb = sc->free_list.tqh_first) > 0) {
		printf("Free list:\n");
		while (acb) {
			siopng_dump_acb(acb);
			acb = acb->chain.tqe_next;
		}
	}
	if ((acb = sc->ready_list.tqh_first) > 0) {
		printf("Ready list:\n");
		while (acb) {
			siopng_dump_acb(acb);
			acb = acb->chain.tqe_next;
		}
	}
	if ((acb = sc->nexus_list.tqh_first) > 0) {
		printf("Nexus list:\n");
		while (acb) {
			siopng_dump_acb(acb);
			acb = acb->chain.tqe_next;
		}
	}
	if (sc->sc_nexus) {
		printf("Nexus:\n");
		siopng_dump_acb(sc->sc_nexus);
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
