/*	$NetBSD: siop.c,v 1.20 1995/02/12 19:19:28 chopps Exp $	*/

/*
 * Copyright (c) 1994 Michael L. Hitch
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
 *	@(#)siop.c	7.5 (Berkeley) 5/4/91
 */

/*
 * AMIGA 53C710 scsi adaptor driver
 */

/* need to know if any tapes have been configured */
#include "st.h"
 
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/disklabel.h>
#include <sys/dkstat.h>
#include <sys/buf.h>
#include <scsi/scsi_all.h>
#include <scsi/scsiconf.h>
#include <machine/cpu.h>
#include <amiga/amiga/custom.h>
#include <amiga/amiga/isr.h>
#include <amiga/dev/siopreg.h>
#include <amiga/dev/siopvar.h>

extern u_int	kvtop();

/*
 * SCSI delays
 * In u-seconds, primarily for state changes on the SPC.
 */
#define	SCSI_CMD_WAIT	500000	/* wait per step of 'immediate' cmds */
#define	SCSI_DATA_WAIT	500000	/* wait per data in/out step */
#define	SCSI_INIT_WAIT	500000	/* wait per step (both) during init */

int  siopicmd __P((struct siop_softc *, int, int, void *, int, void *, int));
int  siopgo __P((struct siop_softc *, struct scsi_xfer *));
int  siopgetsense __P((struct siop_softc *, struct scsi_xfer *));
void siopabort __P((struct siop_softc *, siop_regmap_p, char *));
void sioperror __P((struct siop_softc *, siop_regmap_p, u_char));
void siopstart __P((struct siop_softc *));
void siopreset __P((struct siop_softc *));
void siopsetdelay __P((int));
void siop_scsidone __P((struct siop_softc *, int));
void siop_donextcmd __P((struct siop_softc *));
int  siopintr __P((struct siop_softc *));

/* 53C710 script */
#include <amiga/dev/siop_script.out>

/* default to not inhibit sync negotiation on any drive */
u_char siop_inhibit_sync[8] = { 0, 0, 0, 0, 0, 0, 0 }; /* initialize, so patchable */
u_char siop_allow_disc[8] = {0, 0, 0, 0, 0, 0, 0, 0};
int siop_no_dma = 0;

int siop_reset_delay = 250;	/* delay after reset, in milleseconds */
int siop_sync_period = 50;	/* synchronous transfer period, in nanoseconds */

int siop_cmd_wait = SCSI_CMD_WAIT;
int siop_data_wait = SCSI_DATA_WAIT;
int siop_init_wait = SCSI_INIT_WAIT;

/*
 * sync period transfer lookup - only valid for 66Mhz clock
 */
static struct {
	unsigned char x;	/* period from sync request message */
	unsigned char y;	/* siop_period << 4 | sbcl */
} xxx[] = {
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

#ifdef DEBUG
#define QPRINTF(a) if (siop_debug > 1) printf a
/*
 *	0x01 - full debug
 *	0x02 - DMA chaining
 *	0x04 - siopintr
 *	0x08 - phase mismatch
 *	0x10 - panic on phase mismatch
 *	0x20 - panic on unhandled exceptions
 *	0x100 - disconnect/reconnect
 */
int	siop_debug = 0;
int	siopsync_debug = 0;
int	siopdma_hits = 0;
int	siopdma_misses = 0;
int	siopchain_ints = 0;
int	siopstarts = 0;
int	siopints = 0;
int	siopphmm = 0;
#else
#define QPRINTF(a)
#endif


/*
 * default minphys routine for siop based controllers
 */
void
siop_minphys(bp)
	struct buf *bp;
{
	/*
	 * no max transfer at this level
	 */
}

/*
 * used by specific siop controller
 *
 */
int
siop_scsicmd(xs)
	struct scsi_xfer *xs;
{
	struct siop_pending *pendp;
	struct siop_softc *sc;
	struct scsi_link *slp;
	int flags, s, i;

	slp = xs->sc_link;
	sc = slp->adapter_softc;
	flags = xs->flags;

	if (flags & SCSI_DATA_UIO)
		panic("siop: scsi data uio requested");
	
	if (sc->sc_xs && flags & SCSI_POLL)
		panic("siop_scsicmd: busy");

	s = splbio();
	pendp = &sc->sc_xsstore[slp->target][slp->lun];
	if (pendp->xs) {
		splx(s);
		return(TRY_AGAIN_LATER);
	}

	if (sc->sc_xs) {
		pendp->xs = xs;
		TAILQ_INSERT_TAIL(&sc->sc_xslist, pendp, link);
		splx(s);
		return(SUCCESSFULLY_QUEUED);
	}
	pendp->xs = NULL;
	sc->sc_xs = xs;
	splx(s);

	/*
	 * nothing is pending do it now.
	 */
	siop_donextcmd(sc);

	if (flags & SCSI_POLL)
		return(COMPLETE);
	return(SUCCESSFULLY_QUEUED);
}

/*
 * entered with sc->sc_xs pointing to the next xfer to perform
 */
void
siop_donextcmd(sc)
	struct siop_softc *sc;
{
	struct scsi_xfer *xs;
	struct scsi_link *slp;
	int flags, stat, i;

	xs = sc->sc_xs;
	slp = xs->sc_link;
	flags = xs->flags;

	if (flags & SCSI_RESET)
		siopreset(sc);

	xs->cmd->bytes[0] |= slp->lun << 5;
	for (i = 0; i < 2; ++i) {
		if (sc->sc_iob[i].sc_xs == NULL) {
			sc->sc_iob[i].sc_xs = xs;
			sc->sc_cur = & sc->sc_iob[i];
			sc->sc_iob[i].sc_stat[0] = -1;
			break;
		}
	}
	if (sc->sc_cur == NULL)
		panic ("siop: too many I/O's");
	++sc->sc_active;
#if 0
if (sc->sc_active > 1) {
  printf ("active count %d\n", sc->sc_active);
}
#endif
	if (flags & SCSI_POLL || siop_no_dma) 
		stat = siopicmd(sc, slp->target, slp->lun, xs->cmd,
		    xs->cmdlen, xs->data, xs->datalen);
	else if (siopgo(sc, xs) == 0)
		return;
	else 
		stat = sc->sc_cur->sc_stat[0];
	
	siop_scsidone(sc, stat);
}

void
siop_scsidone(sc, stat)
	struct siop_softc *sc;
	int stat;
{
	struct siop_pending *pendp;
	struct scsi_xfer *xs;
	int s, donext;

	xs = sc->sc_xs;
#ifdef DIAGNOSTIC
	if (xs == NULL)
		panic("siop_scsidone");
#endif
#if 1
	if (((struct device *)(xs->sc_link->device_softc))->dv_unit < dk_ndrive)
		++dk_xfer[((struct device *)(xs->sc_link->device_softc))->dv_unit];
#endif
	/*
	 * is this right?
	 */
	xs->status = stat;

	if (stat == 0)
		xs->resid = 0;
	else {
		switch(stat) {
		case SCSI_CHECK:
			if (stat = siopgetsense(sc, xs))
				goto bad_sense;
			xs->error = XS_SENSE;
if ((xs->sense.extended_flags & SSD_KEY) == 0x0b) { /* ABORTED COMMAND */
  printf ("%s: command %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
    sc->sc_dev.dv_xname,
    xs->cmd->opcode, xs->cmd->bytes[0], xs->cmd->bytes[1], xs->cmd->bytes[1],
    xs->cmd->bytes[3], xs->cmd->bytes[4], xs->cmd->bytes[5],
    xs->cmd->bytes[6], xs->cmd->bytes[7], xs->cmd->bytes[8]);
  printf ("%s: sense %02x %02x %02x %02x %02x %02x %02 extlen %02x: %02x %02x\n",
    sc->sc_dev.dv_xname,
    xs->sense.error_code, xs->sense.extended_segment,
    xs->sense.extended_flags, xs->sense.extended_info[0],
    xs->sense.extended_info[1], xs->sense.extended_info[2],
    xs->sense.extended_info[3], xs->sense.extended_extra_len,
    xs->sense.extended_extra_bytes[0], xs->sense.extended_extra_bytes[1]);
}
			break;
		case SCSI_BUSY:
			xs->error = XS_BUSY;
			break;
		bad_sense:
		default:
			xs->error = XS_DRIVER_STUFFUP;
			QPRINTF(("siop_scsicmd() bad %x\n", stat));
			break;
		}
	}
	xs->flags |= ITSDONE;

	--sc->sc_active;
	sc->sc_cur->sc_xs = NULL;
	sc->sc_cur = NULL;
	/*
	 * if another command is already pending (i.e. it was pre-empted
	 * by a reconnecting device), make it the current command and
	 * continue
	 */

	/*
	 * grab next command before scsi_done()
	 * this way no single device can hog scsi resources.
	 */
	s = splbio();
	pendp = sc->sc_xslist.tqh_first;
	if (pendp == NULL) {
		donext = 0;
		sc->sc_xs = NULL;
	} else {
		donext = 1;
		TAILQ_REMOVE(&sc->sc_xslist, pendp, link);
		sc->sc_xs = pendp->xs;
		pendp->xs = NULL;
	}
	splx(s);
	scsi_done(xs);

	if (donext)
		siop_donextcmd(sc);
	else if (sc->sc_active && sc->sc_xs == NULL) {
/*		printf ("siop_scsidone: still active %d\n", sc->sc_active);*/
/*		sc->sc_siopp->siop_dsp = sc->sc_scriptspa + Ent_wait_reselect;*/
	}
}

int
siopgetsense(sc, xs)
	struct siop_softc *sc;
	struct scsi_xfer *xs;
{
	struct scsi_sense rqs;
	struct scsi_link *slp;
	int stat;

	slp = xs->sc_link;
	
	rqs.opcode = REQUEST_SENSE;
	rqs.byte2 = slp->lun << 5;
#ifdef not_yet
	rqs.length = xs->req_sense_length ? xs->req_sense_length : 
	    sizeof(xs->sense);
#else
	rqs.length = sizeof(xs->sense);
#endif
	if (rqs.length > sizeof (xs->sense))
		rqs.length = sizeof (xs->sense);
	rqs.unused[0] = rqs.unused[1] = rqs.control = 0;
	
	return(siopicmd(sc, slp->target, slp->lun, &rqs, sizeof(rqs),
	    &xs->sense, rqs.length));
}

void
siopabort(sc, rp, where)
	register struct siop_softc *sc;
	siop_regmap_p rp;
	char *where;
{
	int i;

	printf ("%s: abort %s: dstat %02x, sstat0 %02x sbcl %02x\n", 
	    sc->sc_dev.dv_xname,
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

          sc->sc_flags &= ~SIOP_SELECTED;
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
		sc->sc_flags &= ~SIOP_SELECTED;
		for (i = 0; i < 2; ++i) {
			if (sc->sc_iob[i].sc_xs && &sc->sc_iob[i] !=
			    sc->sc_cur) {
				printf ("siopabort: cleanup!\n");
				sc->sc_iob[i].sc_xs = NULL;
			}
		}
		sc->sc_active = 0;
	}
}

void
siopinitialize(sc)
	struct siop_softc *sc;
{
	/*
	 * Need to check that scripts is on a long word boundary
	 * and that DS is on a long word boundary.
	 * Also need to verify that dev doesn't span non-contiguous
	 * physical pages.
	 */
	sc->sc_scriptspa = kvtop(scripts);
#if 0
	sc->sc_dspa = kvtop(&sc->sc_ds);
	sc->sc_lunpa = kvtop(&sc->sc_msgout);
	sc->sc_statuspa = kvtop(&sc->sc_stat[0]);
	sc->sc_msgpa = kvtop(&sc->sc_msg[0]);
#endif
	siopreset (sc);
}

void
siopreset(sc)
	struct siop_softc *sc;
{
	siop_regmap_p rp;
	u_int i, s;
	u_char  my_id, dummy;

	rp = sc->sc_siopp;

	if (sc->sc_flags & SIOP_ALIVE)
		siopabort(sc, rp, "reset");
		
	printf("%s: ", sc->sc_dev.dv_xname);		/* XXXX */

	s = splbio();
	my_id = 7;

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
	rp->siop_sien &= ~SIOP_SIEN_RST;
	rp->siop_scntl1 |= SIOP_SCNTL1_RST;
	delay(1);
	rp->siop_scntl1 &= ~SIOP_SCNTL1_RST;
/*	rp->siop_sien |= SIOP_SIEN_RST;*/

	/*
	 * Set up various chip parameters
	 */
	rp->siop_scntl0 = SIOP_ARB_FULL | SIOP_SCNTL0_EPC | SIOP_SCNTL0_EPG;
	rp->siop_dcntl = sc->sc_clock_freq & 0xff;
	rp->siop_dmode = 0x80;	/* burst length = 4 */
	rp->siop_sien = 0x00;	/* don't enable interrupts yet */
	rp->siop_dien = 0x00;	/* don't enable interrupts yet */
	rp->siop_scid = 1 << my_id;
	rp->siop_dwt = 0x00;
	rp->siop_ctest0 |= SIOP_CTEST0_BTD | SIOP_CTEST0_EAN;
	rp->siop_ctest7 |= (sc->sc_clock_freq >> 8) & 0xff;

	/* will need to re-negotiate sync xfers */
	bzero(&sc->sc_sync, sizeof (sc->sc_sync));

	i = rp->siop_istat;
	if (i & SIOP_ISTAT_SIP)
		dummy = rp->siop_sstat0;
	if (i & SIOP_ISTAT_DIP)
		dummy = rp->siop_dstat;

	splx (s);

 	delay (siop_reset_delay * 1000);
	printf("siop id %d reset V%d\n", my_id, rp->siop_ctest8 >> 4);
	sc->sc_flags |= SIOP_ALIVE;
	sc->sc_flags &= ~(SIOP_SELECTED | SIOP_DMA);
}

/*
 * Setup Data Storage for 53C710 and start SCRIPTS processing
 */

void
siop_setup (sc, target, lun, cbuf, clen, buf, len)
	struct siop_softc *sc;
	int target;
	int lun;
	u_char *cbuf;
	int clen;
	u_char *buf;
	int len;
{
	siop_regmap_p rp = sc->sc_siopp;
	int i;
	int nchain;
	int count, tcount;
	char *addr, *dmaend;
	struct siop_iob *iob = sc->sc_cur;

#ifdef DEBUG
	if (rp->siop_sbcl & SIOP_BSY) {
		printf ("ACK! siop was busy: rp %x script %x dsa %x\n",
		    rp, &scripts, &iob->sc_ds);
		/*Debugger();*/
	}
	i = rp->siop_istat;
	if (i & SIOP_ISTAT_DIP) {
		int dstat;
		dstat = rp->siop_dstat;
	}
	if (i & SIOP_ISTAT_SIP) {
		int sstat;
		sstat = rp->siop_sstat0;
	}
#endif
	sc->sc_istat = 0;
	iob->sc_msgout[0] = MSG_IDENTIFY | lun;
/*
 * HACK - if no data transfer, allow disconnects
 */
	if (/*len == 0 || */ siop_allow_disc[target]) {
		iob->sc_msgout[0] = MSG_IDENTIFY_DR | lun;
		rp->siop_scntl1 |= SIOP_SCNTL1_ESR;
	}
	iob->sc_status = 0;
	iob->sc_stat[0] = -1;
	iob->sc_msg[0] = -1;
	iob->sc_ds.scsi_addr = (0x10000 << target) | (sc->sc_sync[target].period << 8);
	iob->sc_ds.idlen = 1;
	iob->sc_ds.idbuf = (char *) kvtop(&iob->sc_msgout[0]);
	iob->sc_ds.cmdlen = clen;
	iob->sc_ds.cmdbuf = (char *) kvtop(cbuf);
	iob->sc_ds.stslen = 1;
	iob->sc_ds.stsbuf = (char *) kvtop(&iob->sc_stat[0]);
	iob->sc_ds.msglen = 1;
	iob->sc_ds.msgbuf = (char *) kvtop(&iob->sc_msg[0]);
	iob->sc_msg[1] = -1;
	iob->sc_ds.msginlen = 1;
	iob->sc_ds.extmsglen = 1;
	iob->sc_ds.synmsglen = 3;
	iob->sc_ds.msginbuf = (char *) kvtop(&iob->sc_msg[1]);
	iob->sc_ds.extmsgbuf = (char *) kvtop(&iob->sc_msg[2]);
	iob->sc_ds.synmsgbuf = (char *) kvtop(&iob->sc_msg[3]);
	bzero(&iob->sc_ds.chain, sizeof (iob->sc_ds.chain));

	if (sc->sc_sync[target].state == SYNC_START) {
		if (siop_inhibit_sync[target]) {
			sc->sc_sync[target].state = SYNC_DONE;
			sc->sc_sync[target].offset = 0;
			sc->sc_sync[target].period = 0;
#ifdef DEBUG
			if (siopsync_debug)
				printf ("Forcing target %d asynchronous\n", target);
#endif
		}
		else {
			iob->sc_msg[2] = -1;
			iob->sc_msgout[1] = MSG_EXT_MESSAGE;
			iob->sc_msgout[2] = 3;
			iob->sc_msgout[3] = MSG_SYNC_REQ;
			iob->sc_msgout[4] = siop_sync_period / 4;
			iob->sc_msgout[5] = SIOP_MAX_OFFSET;
			iob->sc_ds.idlen = 6;
			sc->sc_sync[target].state = SYNC_SENT;
#ifdef DEBUG
			if (siopsync_debug)
				printf ("Sending sync request to target %d\n", target);
#endif
		}
	}

/*
 * If length is > 1 page, check for consecutive physical pages
 * Need to set up chaining if not
 */
	iob->iob_buf = buf;
	iob->iob_len = len;
	iob->iob_curbuf = iob->iob_curlen = 0;
	nchain = 0;
	count = len;
	addr = buf;
	dmaend = NULL;
	while (count > 0) {
		iob->sc_ds.chain[nchain].databuf = (char *) kvtop (addr);
		if (count < (tcount = NBPG - ((int) addr & PGOFSET)))
			tcount = count;
		iob->sc_ds.chain[nchain].datalen = tcount;
		addr += tcount;
		count -= tcount;
		if (iob->sc_ds.chain[nchain].databuf == dmaend) {
			dmaend += iob->sc_ds.chain[nchain].datalen;
			iob->sc_ds.chain[--nchain].datalen += tcount;
#ifdef DEBUG
			++siopdma_hits;
#endif
		}
		else {
			dmaend = iob->sc_ds.chain[nchain].databuf +
			    iob->sc_ds.chain[nchain].datalen;
			iob->sc_ds.chain[nchain].datalen = tcount;
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
			printf ("  [%d] %8x %4x\n", i, iob->sc_ds.chain[i].databuf,
			    iob->sc_ds.chain[i].datalen);
		}
	}
	if (rp->siop_sbcl & SIOP_BSY) {
		printf ("ACK! siop was busy at start: rp %x script %x dsa %x\n",
		    rp, &scripts, &iob->sc_ds);
		/*Debugger();*/
	}
#endif

	/* push data case on data the 53c710 needs to access */
	dma_cachectl (sc, sizeof (struct siop_softc));
	dma_cachectl (cbuf, clen);
	if (buf != NULL && len != 0)
		dma_cachectl (buf, len);
	if (sc->sc_active <= 1) {
		rp->siop_sbcl = sc->sc_sync[target].offset;
		rp->siop_dsa = kvtop(&iob->sc_ds);
/* XXX if disconnected devices pending, this may not work */
		rp->siop_dsp = sc->sc_scriptspa;
	} else {
		rp->siop_istat = SIOP_ISTAT_SIGP;
	}
#ifdef DEBUG
	++siopstarts;
#endif
}

/*
 * Process a DMA or SCSI interrupt from the 53C710 SIOP
 */

int
siop_checkintr(sc, istat, dstat, sstat0, status)
	struct	siop_softc *sc;
	u_char	istat;
	u_char	dstat;
	u_char	sstat0;
	int	*status;
{
	siop_regmap_p rp = sc->sc_siopp;
	struct siop_iob *iob = sc->sc_cur;
	int	target;
	int	dfifo, dbc, sstat1;

	dfifo = rp->siop_dfifo;
	dbc = rp->siop_dbc0;
	sstat1 = rp->siop_sstat1;
	rp->siop_ctest8 |= SIOP_CTEST8_CLF;
	while ((rp->siop_ctest1 & SIOP_CTEST1_FMT) == 0)
		;
	rp->siop_ctest8 &= ~SIOP_CTEST8_CLF;
#ifdef DEBUG
	++siopints;
	if (siop_debug & 0x100) {
		DCIAS(&iob->sc_stat[0]);	/* XXX */
		printf ("siopchkintr: istat %x dstat %x sstat0 %x dsps %x sbcl %x sts %x msg %x\n",
		    istat, dstat, sstat0, rp->siop_dsps, rp->siop_sbcl, iob->sc_stat[0], iob->sc_msg[0]);
		printf ("sync msg in: %02x %02x %02x %02x %02x %02x\n",
		    iob->sc_msg[0], iob->sc_msg[1], iob->sc_msg[2],
		    iob->sc_msg[3], iob->sc_msg[4], iob->sc_msg[5]);
	}
#endif
	if (dstat & SIOP_DSTAT_SIR && rp->siop_dsps == 0xff00) {
		/* Normal completion status, or check condition */
		if (rp->siop_dsa != kvtop(&iob->sc_ds)) {
			printf ("siop: invalid dsa: %x %x\n", rp->siop_dsa,
			    kvtop(&iob->sc_ds));
			panic("*** siop DSA invalid ***");
		}
		target = sc->sc_slave;
		if (sc->sc_sync[target].state == SYNC_SENT) {
#ifdef DEBUG
			if (siopsync_debug)
				printf ("sync msg in: %02x %02x %02x %02x %02x %02x\n",
				    iob->sc_msg[0], iob->sc_msg[1], iob->sc_msg[2],
				    iob->sc_msg[3], iob->sc_msg[4], iob->sc_msg[5]);
#endif
			if (iob->sc_msg[1] == 0xff)
				printf ("%s: target %d ignored sync request\n",
				    sc->sc_dev.dv_xname, target);
			else if (iob->sc_msg[1] == MSG_REJECT)
				printf ("%s: target %d rejected sync request\n",
				    sc->sc_dev.dv_xname, target);
			sc->sc_sync[target].state = SYNC_DONE;
			sc->sc_sync[target].period = 0;
			sc->sc_sync[target].offset = 0;
			if (iob->sc_msg[2] == 3 &&
			    iob->sc_msg[3] == MSG_SYNC_REQ &&
			    iob->sc_msg[5] != 0) {
				if (iob->sc_msg[4] && iob->sc_msg[4] < 100 / 4) {
#ifdef DEBUG
					printf ("%d: target %d wanted %dns period\n",
					    sc->sc_dev.dv_xname, target,
					    iob->sc_msg[4] * 4);
#endif
					/*
					 * Kludge for Maxtor XT8580S
					 * It accepts whatever we request, even
					 * though it won't work.  So we ask for
					 * a short period than we can handle.  If
					 * the device says it can do it, use 208ns.
					 * If the device says it can do less than
					 * 100ns, then we limit it to 100ns.
					 */
					if (iob->sc_msg[4] == siop_sync_period / 4)
						iob->sc_msg[4] = 208 / 4;
					else
						iob->sc_msg[4] = 100 / 4;
				}
				printf ("%s: target %d now synchronous, period=%dns, offset=%d\n",
				    sc->sc_dev.dv_xname, target,
				    iob->sc_msg[4] * 4, iob->sc_msg[5]);
				scsi_period_to_siop (sc, target);
			}
		}
#if 0
		DCIAS(sc->sc_statuspa);	/* XXX */
#else
		dma_cachectl(&iob->sc_stat[0], 1);
#endif
		*status = iob->sc_stat[0];
#ifdef DEBUG
		if (rp->siop_sbcl & SIOP_BSY) {
			/*printf ("ACK! siop was busy at end: rp %x script %x dsa %x\n",
			    rp, &scripts, &iob->sc_ds);*/
			/*Debugger();*/
		}
#endif
		if (sc->sc_active > 1)
			rp->siop_dcntl |= SIOP_DCNTL_STD;
		return 1;
	}
	if (sstat0 & SIOP_SSTAT0_M_A) {		/* Phase mismatch */
#ifdef DEBUG
		++siopphmm;
		if (iob->iob_len) {
			int adjust;
			adjust = ((dfifo - (dbc & 0x7f)) & 0x7f);
			if (sstat1 & SIOP_SSTAT1_ORF)
				++adjust;
			if (sstat1 & SIOP_SSTAT1_OLF)
				++adjust;
			iob->iob_curlen = *((long *)&rp->siop_dcmd) & 0xffffff;
			iob->iob_curlen += adjust;
			iob->iob_curbuf = *((long *)&rp->siop_dnad);
#ifdef DEBUG
			if (siop_debug & 0x100)
				printf ("Phase mismatch: dnad %x dbc %x dfifo %x dbc %x sstat1 %x adjust %x sbcl %x\n",
				    iob->iob_curbuf, iob->iob_curlen, dfifo, dbc, sstat1, adjust, rp->siop_sbcl);
#endif
		}
		if (siop_debug & 9)
			printf ("Phase mismatch: %x dsp +%x dcmd %x\n",
			    rp->siop_sbcl,
			    rp->siop_dsp - sc->sc_scriptspa,
			    *((long *)&rp->siop_dcmd));
		if (siop_debug & 0x10)
			panic ("53c710 phase mismatch");
#endif
		if ((rp->siop_sbcl & SIOP_REQ) == 0)
			printf ("Phase mismatch: REQ not asserted! %02x\n",
			    rp->siop_sbcl);
		switch (rp->siop_sbcl & 7) {
/*
 * For data out and data in phase, check for DMA chaining
 */
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
		if (rp->siop_sbcl & SIOP_BSY) {
			printf ("ACK! siop was busy at timeout: rp %x script %x dsa %x\n",
			    rp, &scripts, &iob->sc_ds);
			printf(" sbcl %x sdid %x istat %x dstat %x sstat0 %x\n",
			    rp->siop_sbcl, rp->siop_sdid, istat, dstat, sstat0);
if (!(rp->siop_sbcl & SIOP_BSY)) {
	printf ("Yikes, it's not busy now!\n");
#if 0
	*status = -1;
	if (sc->sc_active > 1)
		rp->siop_dsp = sc->sc_scriptspa + Ent_wait_reselect;
	return 1;
#endif
}
/*			rp->siop_dcntl |= SIOP_DCNTL_STD;*/
			return (0);
			Debugger();
		}
#endif
		*status = -1;
		if (sc->sc_active > 1)
			rp->siop_dsp = sc->sc_scriptspa + Ent_wait_reselect;
		return 1;
	}
	target = sc->sc_slave;
	if (sstat0 & SIOP_SSTAT0_UDC) {
#ifdef DEBUG
		printf ("%s: target %d disconnected unexpectedly\n",
		   sc->sc_dev.dv_xname, target);
#endif
#if 0
		siopabort (sc, rp, "siopchkintr");
#endif
		*status = STS_BUSY;
		if (sc->sc_active > 1)
			rp->siop_dsp = sc->sc_scriptspa + Ent_wait_reselect;
		return 1;
	}
	if (dstat & SIOP_DSTAT_SIR && rp->siop_dsps == 0xff0a) {
#ifdef DEBUG
		if (siop_debug & 0x100)
			printf ("%s: ID %x disconnected TEMP %x (+%d) buf %x len %x buf %x len %x dfifo %x dbc %x sstat1 %x\n",
			    sc->sc_dev.dv_xname, 1 << target, rp->siop_temp,
			    rp->siop_temp - sc->sc_scriptspa,
			    iob->iob_curbuf, iob->iob_curlen,
			    iob->sc_ds.chain[0].databuf, iob->sc_ds.chain[0].datalen, dfifo, dbc, sstat1);
#endif
		/*
		 * 	HACK!
		 * set current entry in disconnect state
		 * clear current
		 * if another iob available and another request pending,
		 * start another
		 */
		iob->sc_status = sc->sc_flags & (SIOP_SELECTED|SIOP_DMA);
		sc->sc_flags &= ~(SIOP_SELECTED|SIOP_DMA);
		sc->sc_cur = NULL;		/* no current device */
		sc->sc_xs = NULL;
		/* continue script to wait for reconnect */
		rp->siop_dcntl |= SIOP_DCNTL_STD;
		if (sc->sc_active < 2) {
			struct siop_pending *pendp;
			int s = splbio();
			/*
			 * can start another I/O, see if there are
			 * any pending and start them
			 */
			if (pendp = sc->sc_xslist.tqh_first) {
				TAILQ_REMOVE(&sc->sc_xslist, pendp, link);
				sc->sc_xs = pendp->xs;
				pendp->xs = NULL;
				splx(s);
#ifdef DEBUG
				if (siop_debug & 0x100)
					printf ("starting a pending command %x\n", sc->sc_xs);
#endif
				siop_donextcmd(sc);
			}
			splx(s);
		}
		return (0);
	}
	if (dstat & SIOP_DSTAT_SIR && (rp->siop_dsps == 0xff09 ||
	    rp->siop_dsps == 0xff06)) {
		int i;
#ifdef DEBUG
		if (siop_debug & 0x100)
			printf ("%s: target reselected %x %x\n",
			     sc->sc_dev.dv_xname, rp->siop_lcrc,
			     rp->siop_dsps);
#endif
		if (sc->sc_cur) {
			printf ("siop: oops - reconnect before current done\n");
/*			panic ("siop: what now?");*/
		}
		/*
		 * locate iob of reconnecting device
		 * set sc->sc_cur to iob
		 */
		for (i = 0, iob = NULL; i < 2; ++i) {
			if (sc->sc_iob[i].sc_xs == NULL)
				continue;
			if (!sc->sc_iob[i].sc_status)
				continue;
			if (!(rp->siop_lcrc & (sc->sc_iob[i].sc_ds.scsi_addr >> 16)))
				continue;
			iob = &sc->sc_iob[i];
			sc->sc_cur = iob;
			sc->sc_xs = iob->sc_xs;
			sc->sc_flags |= iob->sc_status;
			iob->sc_status = 0;
			DCIAS(kvtop(&iob->sc_stat[0]));
			rp->siop_dsa = kvtop(&iob->sc_ds);
			if (iob->iob_curlen) {
if (iob->iob_curbuf <= (u_long) iob->sc_ds.chain[0].databuf ||
    iob->iob_curbuf >= (u_long)(iob->sc_ds.chain[0].databuf + iob->sc_ds.chain[0].datalen))
	printf ("%s: saved data pointer not in chain[0]\n");
				iob->sc_ds.chain[0].databuf = (char *) iob->iob_curbuf;
				iob->sc_ds.chain[0].datalen = iob->iob_curlen;
#ifdef DEBUG
				if (siop_debug & 0x100)
					printf ("%s: reselect buf %x len %x\n",
					    sc->sc_dev.dv_xname, iob->iob_curbuf,
					    iob->iob_curlen);
#endif
				DCIAS(kvtop(&iob->sc_ds.chain));
			}
		}
		if (iob == NULL)
			panic("unable to find reconnecting device");
		rp->siop_dcntl |= SIOP_DCNTL_STD;
		return (0);
	}
	if (dstat & SIOP_DSTAT_SIR && rp->siop_dsps == 0xff07) {
		/* reselect was interrupted */
#ifdef DEBUG
		if (siop_debug & 0x100 &&
		    (rp->siop_sfbr & SIOP_CTEST2_SIGP) == 0)
			printf ("%s: reselect interrupted (Sig_P?) scntl1 %x ctest2 %x sfbr %x\n",
			    sc->sc_dev.dv_xname, rp->siop_scntl1, rp->siop_ctest2, rp->siop_sfbr);
#endif
		target = sc->sc_xs->sc_link->target;
		rp->siop_dsa = kvtop(&sc->sc_cur->sc_ds);
		rp->siop_sbcl = sc->sc_sync[target].offset;
		rp->siop_dcntl |= SIOP_DCNTL_STD;
		return (0);
	}
	if (sstat0 == 0 && dstat & SIOP_DSTAT_SIR) {
#if 0
		DCIAS(sc->sc_statuspa);
#else
		dma_cachectl (&iob->sc_stat[0], 1);
#endif
		printf ("SIOP interrupt: %x sts %x msg %x sbcl %x\n",
		    rp->siop_dsps, iob->sc_stat[0], iob->sc_msg[0],
		    rp->siop_sbcl);
		siopreset (sc);
		*status = -1;
		return 1;
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
	printf ("siopchkintr: target %x ds %x\n", target, &iob->sc_ds);
	printf ("scripts %x ds %x rp %x dsp %x dcmd %x\n", sc->sc_scriptspa,
	    kvtop(&sc->sc_iob), kvtop(rp), rp->siop_dsp,
	    *((long *)&rp->siop_dcmd));
	printf ("siopchkintr: istat %x dstat %x sstat0 %x dsps %x dsa %x sbcl %x sts %x msg %x\n",
	    istat, dstat, sstat0, rp->siop_dsps, rp->siop_dsa,
	     rp->siop_sbcl, iob->sc_stat[0], iob->sc_msg[0]);
#ifdef DEBUG
	if (siop_debug & 0x20)
		panic("siopchkintr: **** temp ****");
#endif
	siopreset (sc);		/* hard reset */
	*status = -1;
	return 1;
}

/*
 * SCSI 'immediate' command:  issue a command to some SCSI device
 * and get back an 'immediate' response (i.e., do programmed xfer
 * to get the response data).  'cbuf' is a buffer containing a scsi
 * command of length clen bytes.  'buf' is a buffer of length 'len'
 * bytes for data.  The transfer direction is determined by the device
 * (i.e., by the scsi bus data xfer phase).  If 'len' is zero, the
 * command must supply no data.  'xferphase' is the bus phase the
 * caller expects to happen after the command is issued.  It should
 * be one of DATA_IN_PHASE, DATA_OUT_PHASE or STATUS_PHASE.
 *
 * XXX - 53C710 will use DMA, but no interrupts (it's a heck of a
 * lot easier to do than to use programmed I/O).
 *
 */
int
siopicmd(sc, target, lun, cbuf, clen, buf, len)
	struct siop_softc *sc;
	int target;
	int lun;
	void *cbuf;
	int clen;
	void *buf;
	int len;
{
	siop_regmap_p rp = sc->sc_siopp;
	struct siop_iob *iob = sc->sc_cur;
	int i;
	int status;
	u_char istat;
	u_char dstat;
	u_char sstat0;

	if (sc->sc_flags & SIOP_SELECTED) {
		printf ("siopicmd%d: bus busy\n", target);
		return -1;
	}
	rp->siop_sien = 0x00;		/* disable SCSI and DMA interrupts */
	rp->siop_dien = 0x00;
	sc->sc_flags |= SIOP_SELECTED;
	sc->sc_slave = target;
#ifdef DEBUG
	if (siop_debug & 1)
		printf ("siopicmd: target %x/%x cmd %02x ds %x\n", target,
		    lun, *((char *)cbuf), &iob->sc_ds);
#endif
	siop_setup (sc, target, lun, cbuf, clen, buf, len);

	for (;;) {
		/* use cmd_wait values? */
		i = siop_cmd_wait << 1;
		while (((istat = rp->siop_istat) &
		    (SIOP_ISTAT_SIP | SIOP_ISTAT_DIP)) == 0) {
			if (--i <= 0) {
				printf ("waiting: tgt %d cmd %02x sbcl %02x dsp %x (+%x) dcmd %x ds %x\n",
				    target, *((char *)cbuf),
				    rp->siop_sbcl, rp->siop_dsp,
				    rp->siop_dsp - sc->sc_scriptspa,
				    *((long *)&rp->siop_dcmd), &iob->sc_ds);
				i = siop_cmd_wait << 2;
				/* XXXX need an upper limit and reset */
			}
			delay(10);
		}
		dstat = rp->siop_dstat;
		sstat0 = rp->siop_sstat0;
#ifdef DEBUG
		if (siop_debug & 1) {
			DCIAS(kvtop(iob->sc_stat[0]));	/* XXX should just invalidate sc->sc_stat */
			printf ("siopicmd: istat %x dstat %x sstat0 %x dsps %x sbcl %x sts %x msg %x\n",
			    istat, dstat, sstat0, rp->siop_dsps, rp->siop_sbcl,
			    iob->sc_stat[0], iob->sc_msg[0]);
		}
#endif
		if (siop_checkintr(sc, istat, dstat, sstat0, &status)) {
			sc->sc_flags &= ~SIOP_SELECTED;
			return (status);
		}
	}
}

int
siopgo(sc, xs)
	struct siop_softc *sc;
	struct scsi_xfer *xs;
{
	siop_regmap_p rp;
	int i;
	int nchain;
	int count, tcount;
	char *addr, *dmaend;

#ifdef DEBUG
	if (siop_debug & 1)
		printf ("%s: go ", sc->sc_dev.dv_xname);
#if 0
	if ((cdb->cdb[1] & 1) == 0 &&
	    ((cdb->cdb[0] == CMD_WRITE && cdb->cdb[2] == 0 && cdb->cdb[3] == 0) ||
	    (cdb->cdb[0] == CMD_WRITE_EXT && cdb->cdb[2] == 0 && cdb->cdb[3] == 0
	    && cdb->cdb[4] == 0)))
		panic ("siopgo: attempted write to block < 0x100");
#endif
#endif
#if 0
	cdb->cdb[1] |= unit << 5;
#endif

	if (sc->sc_flags & SIOP_SELECTED) {
		printf ("%s: bus busy\n", sc->sc_dev.dv_xname);
		return 1;
	}

	sc->sc_flags |= SIOP_SELECTED | SIOP_DMA;
	sc->sc_slave = xs->sc_link->target;
	rp = sc->sc_siopp;
	/* enable SCSI and DMA interrupts */
	rp->siop_sien = SIOP_SIEN_M_A | SIOP_SIEN_STO | /*SIOP_SIEN_SEL |*/ SIOP_SIEN_SGE |
	    SIOP_SIEN_UDC | SIOP_SIEN_RST | SIOP_SIEN_PAR;
	rp->siop_dien = SIOP_DIEN_BF | SIOP_DIEN_ABRT | SIOP_DIEN_SIR |
	    /*SIOP_DIEN_WTD |*/ SIOP_DIEN_IID;
#ifdef DEBUG
	if (siop_debug & 1)
		printf ("siopgo: target %x cmd %02x ds %x\n", sc->sc_slave,
		    xs->cmd->opcode, &sc->sc_cur->sc_ds);
#endif

	siop_setup(sc, sc->sc_slave, xs->sc_link->lun, xs->cmd, xs->cmdlen,
	    xs->data, xs->datalen);

	return (0);
}

/*
 * Check for 53C710 interrupts
 */

int
siopintr (sc)
	register struct siop_softc *sc;
{
	siop_regmap_p rp;
	register u_char istat, dstat, sstat0;
	int status;

	rp = sc->sc_siopp;
	istat = sc->sc_istat;
	if ((istat & (SIOP_ISTAT_SIP | SIOP_ISTAT_DIP)) == 0)
		return;
	if ((rp->siop_sien | rp->siop_dien) == 0)
		return(0);	/* no interrupts enabled */
	/* Got a valid interrupt on this device */
	dstat = sc->sc_dstat;
	sstat0 = sc->sc_sstat0;
	sc->sc_istat = 0;
#ifdef DEBUG
	if (siop_debug & 1)
		printf ("%s: intr istat %x dstat %x sstat0 %x\n",
		    sc->sc_dev.dv_xname, istat, dstat, sstat0);
	if (!sc->sc_active) {
		printf ("%s: spurious interrupt? istat %x dstat %x sstat0 %x status %x\n",
		    sc->sc_dev.dv_xname, istat, dstat, sstat0, sc->sc_cur->sc_stat[0]);
	}
#endif

#ifdef DEBUG
	if (siop_debug & 5) {
		DCIAS(kvtop(&sc->sc_cur->sc_stat[0]));
		printf ("%s: intr istat %x dstat %x sstat0 %x dsps %x sbcl %x sts %x msg %x\n",
		    sc->sc_dev.dv_xname, istat, dstat, sstat0,
		    rp->siop_dsps,  rp->siop_sbcl,
		    sc->sc_cur->sc_stat[0], sc->sc_cur->sc_msg[0]);
	}
#endif
	if (siop_checkintr (sc, istat, dstat, sstat0, &status)) {
#if 1
		if (sc->sc_active <= 1) {	/* more HACK */
			rp->siop_sien = 0;
			rp->siop_dien = 0;
		}
		if (status == 0xff)
			printf ("siopintr: status == 0xff\n");
#endif
		if (sc->sc_flags & SIOP_DMA) {
			sc->sc_flags &= ~(SIOP_DMA | SIOP_SELECTED);
			if (rp->siop_sbcl & SIOP_BSY)
				printf ("%s: SCSI bus busy at completion\n",
					sc->sc_dev.dv_xname);
			siop_scsidone(sc, sc->sc_cur->sc_stat[0]);
		}
	}
}

/*
 * This is based on the Progressive Peripherals Zeus driver and may
 * not be correct for other 53c710 boards.
 *
 */
scsi_period_to_siop (sc, target)
	struct siop_softc *sc;
{
	int period, offset, i, sxfer;

	period = sc->sc_cur->sc_msg[4];
	offset = sc->sc_cur->sc_msg[5];
	sxfer = 0;
	if (offset <= SIOP_MAX_OFFSET)
		sxfer = offset;
	for (i = 0; i < sizeof (xxx) / 2; ++i) {
		if (period <= xxx[i].x) {
			sxfer |= xxx[i].y & 0x70;
			offset = xxx[i].y & 0x03;
			break;
		}
	}
	sc->sc_sync[target].period = sxfer;
	sc->sc_sync[target].offset = offset;
#ifdef DEBUG
	printf ("sync: siop_sxfr %02x, siop_sbcl %02x\n", sxfer, offset);
#endif
}
