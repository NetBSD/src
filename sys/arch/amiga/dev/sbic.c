/*
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
 *	@(#)scsi.c	7.5 (Berkeley) 5/4/91
 *	$Id: sbic.c,v 1.5 1994/06/20 02:23:12 chopps Exp $
 */

/*
 * AMIGA AMD 33C93 scsi adaptor driver
 */

/* need to know if any tapes have been configured */
#include "st.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/buf.h>
#include <scsi/scsi_all.h>
#include <scsi/scsiconf.h>
#include <vm/vm.h>
#include <vm/vm_kern.h>
#include <vm/vm_page.h>
#include <machine/pmap.h>
#include <machine/cpu.h>
#include <amiga/amiga/device.h>
#include <amiga/amiga/custom.h>
#include <amiga/dev/dmavar.h>
#include <amiga/dev/sbicreg.h>
#include <amiga/dev/sbicvar.h>

/*
 * SCSI delays
 * In u-seconds, primarily for state changes on the SPC.
 */
#define	SBIC_CMD_WAIT	50000	/* wait per step of 'immediate' cmds */
#define	SBIC_DATA_WAIT	50000	/* wait per data in/out step */
#define	SBIC_INIT_WAIT	50000	/* wait per step (both) during init */

#define	b_cylin		b_resid
#define SBIC_WAIT(regs, until, timeo) sbicwait(regs, until, timeo, __LINE__)

extern u_int kvtop();

int  sbicicmd __P((struct sbic_softc *, int, void *, int, void *, int,u_char));
int  sbicgo __P((struct sbic_softc *, struct scsi_xfer *));
int  sbicdmaok __P((struct sbic_softc *, struct scsi_xfer *));
int  sbicgetsense __P((struct sbic_softc *, struct scsi_xfer *));
int  sbicwait __P((sbic_regmap_p, char, int , int));
int  sbiccheckdmap __P((void *, u_long, u_long)); 
int  sbicselectbus __P((struct sbic_softc *, sbic_regmap_p, u_char, u_char));
int  sbicxfstart __P((sbic_regmap_p, int, u_char, int));
int  sbicxfout __P((sbic_regmap_p regs, int, void *, int));
int  sbicfromscsiperiod __P((struct sbic_softc *, sbic_regmap_p, int));
int  sbictoscsiperiod __P((struct sbic_softc *, sbic_regmap_p, int));
int  sbicintr __P((struct sbic_softc *));
void sbicxfin __P((sbic_regmap_p regs, int, void *));
void sbicxfdone __P((struct sbic_softc *, sbic_regmap_p, int));
void sbicabort __P((struct sbic_softc *, sbic_regmap_p, char *));
void sbicerror __P((struct sbic_softc *, sbic_regmap_p, u_char));
void sbicstart __P((struct sbic_softc *));
void sbicreset __P((struct sbic_softc *));
void sbicsetdelay __P((int));
void sbic_scsidone __P((struct sbic_softc *, int));
void sbic_donextcmd __P((struct sbic_softc *));

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
int sbic_inhibit_sync = 1;
int sbic_clock_override = 0;
int sbic_no_dma = 0;

#ifdef DEBUG
#define QPRINTF(a) if (sbic_debug > 1) printf a
int	sbic_debug = 0;
int	sync_debug = 0;
int	sbic_dma_debug = 0;
#else
#define QPRINTF
#endif

/*
 * default minphys routine for sbic based controllers
 */
void
sbic_minphys(bp)
	struct buf *bp;
{
	/*
	 * no max transfer at this level
	 */
}

/*
 * must be used
 */
u_int
sbic_adinfo()
{
	/* 
	 * one request at a time please
	 */
	return(1);
}

/*
 * used by specific sbic controller
 *
 * it appears that the higher level code does nothing with LUN's
 * so I will too.  I could plug it in, however so could they
 * in scsi_scsi_cmd().
 */
int
sbic_scsicmd(xs)
	struct scsi_xfer *xs;
{
	struct sbic_pending *pendp;
	struct sbic_softc *dev;
	struct scsi_link *slp;
	int flags, s;

	slp = xs->sc_link;
	dev = slp->adapter_softc;
	flags = xs->flags;

	if (flags & SCSI_DATA_UIO)
		panic("sbic: scsi data uio requested");
	
	if (dev->sc_xs && flags & SCSI_NOMASK)
		panic("sbic_scsicmd: busy");

	s = splbio();
	pendp = &dev->sc_xsstore[slp->target][slp->lun];
	if (pendp->xs) {
		splx(s);
		return(TRY_AGAIN_LATER);
	}

	if (dev->sc_xs) {
		pendp->xs = xs;
		TAILQ_INSERT_TAIL(&dev->sc_xslist, pendp, link);
		splx(s);
		return(SUCCESSFULLY_QUEUED);
	}
	pendp->xs = NULL;
	dev->sc_xs = xs;
	splx(s);

	/*
	 * nothing is pending do it now.
	 */
	sbic_donextcmd(dev);

	if (flags & SCSI_NOMASK)
		return(COMPLETE);
	return(SUCCESSFULLY_QUEUED);
}

/*
 * entered with dev->sc_xs pointing to the next xfer to perform
 */
void
sbic_donextcmd(dev)
	struct sbic_softc *dev;
{
	struct scsi_xfer *xs;
	struct scsi_link *slp;
	int flags, phase, stat;

	xs = dev->sc_xs;
	slp = xs->sc_link;
	flags = xs->flags;

	if (flags & SCSI_DATA_IN)
		phase = DATA_IN_PHASE;
	else if (flags & SCSI_DATA_OUT)
		phase = DATA_OUT_PHASE;
	else
		phase = STATUS_PHASE;
	
	if (flags & SCSI_RESET)
		sbicreset(dev);

	dev->sc_stat[0] = -1;
	if (phase == STATUS_PHASE || flags & SCSI_NOMASK ||
	    sbicdmaok(dev, xs) == 0) 
		stat = sbicicmd(dev, slp->target, xs->cmd, xs->cmdlen, 
		    xs->data, xs->datalen, phase);
	else if (sbicgo(dev, xs) == 0)
		return;
	else 
		stat = dev->sc_stat[0];
	
	sbic_scsidone(dev, stat);
}

void
sbic_scsidone(dev, stat)
	struct sbic_softc *dev;
	int stat;
{
	struct sbic_pending *pendp;
	struct scsi_xfer *xs;
	int s, donext;

	xs = dev->sc_xs;
#ifdef DIAGNOSTIC
	if (xs == NULL)
		panic("sbic_scsidone");
#endif
	/*
	 * is this right?
	 */
	xs->status = stat;

	if (stat == 0 || xs->flags & SCSI_ERR_OK)
		xs->resid = 0;
	else {
		switch(stat) {
		case SCSI_CHECK:
			if (stat = sbicgetsense(dev, xs))
				goto bad_sense;
			xs->error = XS_SENSE;
			break;
		case SCSI_BUSY:
			xs->error = XS_BUSY;
			break;
		bad_sense:
		default:
			xs->error = XS_DRIVER_STUFFUP;
			QPRINTF(("sbic_scsicmd() bad %x\n", stat));
			break;
		}
	}
	xs->flags |= ITSDONE;

	/*
	 * grab next command before scsi_done()
	 * this way no single device can hog scsi resources.
	 */
	s = splbio();
	pendp = dev->sc_xslist.tqh_first;
	if (pendp == NULL) {
		donext = 0;
		dev->sc_xs = NULL;
	} else {
		donext = 1;
		TAILQ_REMOVE(&dev->sc_xslist, pendp, link);
		dev->sc_xs = pendp->xs;
		pendp->xs = NULL;
	}
	splx(s);
	scsi_done(xs);

	if (donext)
		sbic_donextcmd(dev);
}

int
sbicgetsense(dev, xs)
	struct sbic_softc *dev;
	struct scsi_xfer *xs;
{
	struct scsi_sense rqs;
	struct scsi_link *slp;
	int stat;

	slp = xs->sc_link;
	
	rqs.op_code = REQUEST_SENSE;
	rqs.byte2 = slp->lun << 5;
#ifdef not_yet
	rqs.length = xs->req_sense_length ? xs->req_sense_length : 
	    sizeof(xs->sense);
#else
	rqs.length = sizeof(xs->sense);
#endif
	    
	rqs.unused[0] = rqs.unused[1] = rqs.control = 0;
	
	return(sbicicmd(dev, slp->target, &rqs, sizeof(rqs), &xs->sense,
	    rqs.length, DATA_IN_PHASE));
}

int
sbicdmaok(dev, xs)
	struct sbic_softc *dev;
	struct scsi_xfer *xs;
{
	if (sbic_no_dma || xs->datalen & 0x1 || (u_int)xs->data & 0x3)
		return(0);
	/*
	 * controller supports dma to any addresses?
	 */
	else if ((dev->sc_flags & SBICF_BADDMA) == 0) 
		return(1);
	/*
	 * this address is ok for dma?
	 */
	else if (sbiccheckdmap(xs->data, xs->datalen, dev->sc_dmamask) == 0)
		return(1);
	/*
	 * we have a bounce buffer?
	 */
	else if (dev->sc_dmabuffer)
		return(1);
	return(0);
}


int
sbicwait(regs, until, timeo, line)
	sbic_regmap_p regs;
	char until;
	int timeo;
	int line;
{
	u_char val;
	int csr;

	if (timeo == 0)
		timeo = 1000000;	/* some large value.. */

	GET_SBIC_asr(regs,val);
	while ((val & until) == 0) {
		if (timeo-- == 0) {
			GET_SBIC_csr(regs, csr);
			printf("sbicwait TIMEO @%d with asr=x%x csr=x%x\n",
			    line, val, csr);
			break;
		}
		DELAY(1);
		GET_SBIC_asr(regs,val);
	}
	return(val);
}

void
sbicabort(dev, regs, where)
	struct sbic_softc *dev;
	sbic_regmap_p regs;
	char *where;
{
	u_char csr, asr;
  
	GET_SBIC_csr(regs, csr);
	GET_SBIC_asr(regs, asr);

	printf ("%s: abort %s: csr = 0x%02x, asr = 0x%02x\n", 
	    dev->sc_dev.dv_xname, where, csr, asr);

	if (dev->sc_flags & SBICF_SELECTED) {
		SET_SBIC_cmd(regs, SBIC_CMD_ABORT);
		WAIT_CIP(regs);

		GET_SBIC_asr(regs, asr);
		if (asr & (SBIC_ASR_BSY|SBIC_ASR_LCI)) {
			/* ok, get more drastic.. */
          
			SET_SBIC_cmd (regs, SBIC_CMD_RESET);
			DELAY(25);
			SBIC_WAIT(regs, SBIC_ASR_INT, 0);
			/* clears interrupt also */
			GET_SBIC_csr (regs, csr);

			dev->sc_flags &= ~SBICF_SELECTED;
			return;
		}

		do {
			SBIC_WAIT (regs, SBIC_ASR_INT, 0);
			GET_SBIC_csr (regs, csr);
		} while ((csr != SBIC_CSR_DISC) && (csr != SBIC_CSR_DISC_1)
		    && (csr != SBIC_CSR_CMD_INVALID));

		/* lets just hope it worked.. */
		dev->sc_flags &= ~SBICF_SELECTED;
	}
}

/*
 * XXX Set/reset long delays.
 *
 * if delay == 0, reset default delays
 * if delay < 0,  set both delays to default long initialization values
 * if delay > 0,  set both delays to this value
 *
 * Used when a devices is expected to respond slowly (e.g. during
 * initialization).
 */
void
sbicsetdelay(del)
	int del;
{
	static int saved_cmd_wait, saved_data_wait;

	if (del) {
		saved_cmd_wait = sbic_cmd_wait;
		saved_data_wait = sbic_data_wait;
		if (del > 0)
			sbic_cmd_wait = sbic_data_wait = del;
		else
			sbic_cmd_wait = sbic_data_wait = sbic_init_wait;
	} else {
		sbic_cmd_wait = saved_cmd_wait;
		sbic_data_wait = saved_data_wait;
	}
}

void
sbicreset(dev)
	struct sbic_softc *dev;
{
	sbic_regmap_p regs;
	u_int i, s;
	u_char my_id, csr;
	
	regs = dev->sc_sbicp;

	if (dev->sc_flags & SBICF_ALIVE)
		sbicabort(dev, regs, "reset");
		
	s = splbio();
	/* preserve our ID for now */
	GET_SBIC_myid (regs, my_id);
	my_id &= SBIC_ID_MASK;

	if (dev->sc_clkfreq < 110)
		my_id |= SBIC_ID_FS_8_10;
	else if (dev->sc_clkfreq < 160)
		my_id |= SBIC_ID_FS_12_15;
	else if (dev->sc_clkfreq < 210)
		my_id |= SBIC_ID_FS_16_20;

	my_id |= SBIC_ID_EAF /*| SBIC_ID_EHP*/ ;

	SET_SBIC_myid(regs, my_id);

	/*
	 * Disable interrupts (in dmainit) then reset the chip
	 */
	SET_SBIC_cmd(regs, SBIC_CMD_RESET);
	DELAY(25);
	SBIC_WAIT(regs, SBIC_ASR_INT, 0);
	GET_SBIC_csr(regs, csr);       /* clears interrupt also */

	/*
	 * Set up various chip parameters
	 */
	SET_SBIC_control(regs, SBIC_CTL_EDI | SBIC_CTL_IDI 
	    | SBIC_MACHINE_DMA_MODE);
	/*
	 * don't allow (re)selection (SBIC_RID_ES)
	 * until we can handle target mode!!
	 */
	SET_SBIC_rselid(regs, 0);
	SET_SBIC_syn(regs, 0);     /* asynch for now */

	/*
	 * anything else was zeroed by reset
	 */
	splx(s);

	dev->sc_flags |= SBICF_ALIVE;
	dev->sc_flags &= ~SBICF_SELECTED;
}

void
sbicerror(dev, regs, csr)
	struct sbic_softc *dev;
	sbic_regmap_p regs;
	u_char csr;
{
	struct scsi_xfer *xs;

	xs = dev->sc_xs;

#ifdef DIAGNOSTIC
	if (xs == NULL)
		panic("sbicerror");
#endif
	if (xs->flags & SCSI_SILENT)
		return;

	printf("%s: ", dev->sc_dev.dv_xname);
	printf("csr == 0x%02i\n", csr);	/* XXX */
}

/*
 * select the bus, return when selected or error.
 */
int
sbicselectbus(dev, regs, target, our_addr)
        struct sbic_softc *dev;
	sbic_regmap_p regs;
	u_char target, our_addr;
{
	u_char asr, csr, id;

	QPRINTF(("sbicselectbus %d\n", target));

	/* 
	 * if we're already selected, return (XXXX panic maybe?)
	 */
	if (dev->sc_flags & SBICF_SELECTED)
		return(1);

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
	
	SET_SBIC_cmd(regs, SBIC_CMD_SEL_ATN);

	/*
	 * wait for select (merged from seperate function may need
	 * cleanup)
	 */
	WAIT_CIP(regs);
	do {
		SBIC_WAIT(regs, SBIC_ASR_INT, 0);
		GET_SBIC_csr (regs, csr);
		QPRINTF(("%02x ", csr));
	} while (csr != (SBIC_CSR_MIS_2|MESG_OUT_PHASE)
	    && csr != (SBIC_CSR_MIS_2|CMD_PHASE) && csr != SBIC_CSR_SEL_TIMEO);

	if (csr == (SBIC_CSR_MIS_2|CMD_PHASE))
		dev->sc_flags |= SBICF_SELECTED;	/* device ignored ATN */
	else if (csr == (SBIC_CSR_MIS_2|MESG_OUT_PHASE)) {
		/*
		 * Send identify message
		 * (SCSI-2 requires an identify msg (?))
		 */
		GET_SBIC_selid(regs, id);

		/* 
		 * handle drives that don't want to be asked 
		 * whether to go sync at all.
		 */
		if (sbic_inhibit_sync && dev->sc_sync[id].state == SYNC_START) {
#ifdef DEBUG
			if (sync_debug)
				printf("Forcing target %d asynchronous.\n", id);
#endif
			dev->sc_sync[id].offset = 0;
			dev->sc_sync[id].period = sbic_min_period;
			dev->sc_sync[id].state = SYNC_DONE;
		}
	

		if (dev->sc_sync[id].state != SYNC_START)
			SEND_BYTE (regs, MSG_IDENTIFY);
		else {
			/*
			 * try to initiate a sync transfer.
			 * So compose the sync message we're going 
			 * to send to the target
			 */

#ifdef DEBUG
			if (sync_debug)
				printf("Sending sync request to target %d ... ",
				    id);
#endif
			/*
			 * setup scsi message sync message request
			 */
			dev->sc_msg[0] = MSG_IDENTIFY;
			dev->sc_msg[1] = MSG_EXT_MESSAGE;
			dev->sc_msg[2] = 3;
			dev->sc_msg[3] = MSG_SYNC_REQ;
			dev->sc_msg[4] = sbictoscsiperiod(dev, regs,
			    sbic_min_period);
			dev->sc_msg[5] = sbic_max_offset;

			if (sbicxfstart(regs, 6, MESG_OUT_PHASE, sbic_cmd_wait))
				sbicxfout(regs, 6, dev->sc_msg, MESG_OUT_PHASE);

			dev->sc_sync[id].state = SYNC_SENT;
#ifdef DEBUG
			if (sync_debug)
				printf ("sent\n");
#endif
		}

		SBIC_WAIT (regs, SBIC_ASR_INT, 0);
		GET_SBIC_csr (regs, csr);
		QPRINTF(("[%02x]", csr));
#ifdef DEBUG
		if (sync_debug && dev->sc_sync[id].state == SYNC_SENT)
			printf("csr-result of last msgout: 0x%x\n", csr);
#endif

		if (csr != SBIC_CSR_SEL_TIMEO)
			dev->sc_flags |= SBICF_SELECTED;
	}
  
	QPRINTF(("\n"));

	return(csr == SBIC_CSR_SEL_TIMEO);
}

int
sbicxfstart(regs, len, phase, wait)
	sbic_regmap_p regs;
	int len, wait;
	u_char phase;
{
	u_char id;

	if (phase == DATA_IN_PHASE || phase == MESG_IN_PHASE) {
		GET_SBIC_selid (regs, id);
		id |= SBIC_SID_FROM_SCSI;
		SET_SBIC_selid (regs, id);
		SBIC_TC_PUT (regs, (unsigned)len);
	} else if (phase == DATA_OUT_PHASE || phase == MESG_OUT_PHASE 
	    || phase == CMD_PHASE)
		SBIC_TC_PUT (regs, (unsigned)len);
	else
		SBIC_TC_PUT (regs, 0);
	QPRINTF(("sbicxfstart %d, %d, %d\n", len, phase, wait));

	return(1);
}

int
sbicxfout(regs, len, bp, phase)
	sbic_regmap_p regs;
	int len;
	void *bp;
	int phase;
{
	u_char orig_csr, csr, asr, *buf;
	int wait;
	
	buf = bp;
	wait = sbic_data_wait;

	QPRINTF(("sbicxfout {%d} %02x %02x %02x %02x %02x "
	    "%02x %02x %02x %02x %02x\n", len, buf[0], buf[1], buf[2], 
	    buf[3], buf[4], buf[5], buf[6], buf[7], buf[8], buf[9]));

	GET_SBIC_csr (regs, orig_csr);

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
#ifdef DEBUG
				if (sbic_debug)
					printf("sbicxfout fail: l%d i%x w%d\n",
					    len, asr, wait);
#endif
				return (len);
			}
			DELAY(1);
			GET_SBIC_asr (regs, asr);
		}

		SET_SBIC_data (regs, *buf);
		buf++;
	}

	QPRINTF(("sbicxfout done\n"));
	/*
	 * this leaves with one csr to be read
	 */
	return(0);
}

void
sbicxfin(regs, len, bp)
	sbic_regmap_p regs;
	int len;
	void *bp;
{
	int wait;
	u_char *obp, *buf;
	u_char orig_csr, csr, asr;
	
	wait = sbic_data_wait;
	obp = bp;
	buf = bp;

	GET_SBIC_csr (regs, orig_csr);

	QPRINTF(("sbicxfin %d, csr=%02x\n", len, orig_csr));

	WAIT_CIP (regs);
	SET_SBIC_cmd (regs, SBIC_CMD_XFER_INFO);
	for (;len > 0; len--) {
		GET_SBIC_asr (regs, asr);
		while ((asr & SBIC_ASR_DBR) == 0) {
			if ((asr & SBIC_ASR_INT) || --wait < 0) {
#ifdef DEBUG
				if (sbic_debug)
					printf("sbicxfin fail: l%d i%x w%d\n",
					    len, asr, wait);
#endif
				return;
			}

			DELAY(1);
			GET_SBIC_asr (regs, asr);
		}

		GET_SBIC_data (regs, *buf);
		buf++;
	}

	QPRINTF(("sbicxfin {%d} %02x %02x %02x %02x %02x %02x "
	    "%02x %02x %02x %02x\n", len, obp[0], obp[1], obp[2], 
	    obp[3], obp[4], obp[5], obp[6], obp[7], obp[8], obp[9]));

	/* this leaves with one csr to be read */
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
 */
int
sbicicmd(dev, target, cbuf, clen, buf, len, xferphase)
	struct sbic_softc *dev;
	void *cbuf, *buf;
	int clen, len;
	u_char xferphase;
{
	sbic_regmap_p regs;
	u_char phase, csr, asr;
	int wait;

	regs = dev->sc_sbicp;

	/* 
	 * set the sbic into non-DMA mode
	 */
	SET_SBIC_control(regs, SBIC_CTL_EDI | SBIC_CTL_IDI);

retry_selection:
	/*
	 * select the SCSI bus (it's an error if bus isn't free)
	 */
	if (sbicselectbus(dev, regs, target, dev->sc_scsiaddr))
		return(-1);
	/* 
	 * Wait for a phase change (or error) then let the device sequence 
	 * us through the various SCSI phases.
	 */
	dev->sc_stat[0] = 0xff;
	dev->sc_msg[0] = 0xff;
	phase = CMD_PHASE;

new_phase:
	wait = sbic_cmd_wait;

	GET_SBIC_csr (regs, csr);
	QPRINTF((">CSR:%02x<", csr));

	/*
	 * requesting some new phase
	 */
	if ((csr != 0xff) && (csr & 0xf0) && (csr & 0x08))
		phase = csr & PHASE;
	else if ((csr == SBIC_CSR_DISC) || (csr == SBIC_CSR_DISC_1)
	    || (csr == SBIC_CSR_S_XFERRED)) {
		dev->sc_flags &= ~SBICF_SELECTED;
		GET_SBIC_cmd_phase (regs, phase);
		if (phase == 0x60)
			GET_SBIC_tlun (regs, dev->sc_stat[0]);
		else
			return(-1);
		goto out;
	} else {
		sbicerror(dev, regs, csr);
		goto abort;
	}

	switch (phase) {
	case CMD_PHASE:
		if (sbicxfstart (regs, clen, phase, wait))
			if (sbicxfout (regs, clen, cbuf, phase))
				goto abort;
		phase = xferphase;
		break;
	case DATA_IN_PHASE:
		if (len <= 0)
			goto abort;
		wait = sbic_data_wait;
		if (sbicxfstart(regs, len, phase, wait))
			sbicxfin(regs, len, buf);
		phase = STATUS_PHASE;
		break;
	case MESG_IN_PHASE:
		if (sbicxfstart(regs, sizeof(dev->sc_msg), phase, wait) == 0)
			break;
		dev->sc_msg[0] = 0xff;
		sbicxfin(regs, sizeof(dev->sc_msg), dev->sc_msg);
		/*
		 * get the command completion interrupt, or we 
		 * can't send a new command (LCI)
		 */
		SBIC_WAIT(regs, SBIC_ASR_INT, wait);
		GET_SBIC_csr(regs, csr);
#ifdef DEBUG
		if (sync_debug)
			printf("msgin done csr 0x%x\n", csr);
#endif
		/*
		 * test whether this is a reply to our sync
		 * request
		 */
		if (dev->sc_msg[0] == MSG_EXT_MESSAGE && dev->sc_msg[1] == 3
		    && dev->sc_msg[2] == MSG_SYNC_REQ) {

			dev->sc_sync[target].period = sbicfromscsiperiod(dev,
			    regs, dev->sc_msg[3]);
			dev->sc_sync[target].offset = dev->sc_msg[4];
			dev->sc_sync[target].state = SYNC_DONE;
			SET_SBIC_syn(regs, SBIC_SYN(dev->sc_sync[target].offset,
			    dev->sc_sync[target].period));
			/* ACK the message */
			SET_SBIC_cmd(regs, SBIC_CMD_CLR_ACK);
			WAIT_CIP(regs);
			phase = CMD_PHASE;  /* or whatever */
			printf("%s: target %d now synchronous,"
			    " period=%dns, offset=%d.\n",
			    dev->sc_dev.dv_xname, target, dev->sc_msg[3] * 4,
			    dev->sc_msg[4]);
		} else if (dev->sc_msg[0] == MSG_REJECT
		    && dev->sc_sync[target].state == SYNC_SENT) {
#ifdef DEBUG
			if (sync_debug)
				printf("target %d rejected sync, going async\n",
				    target);
#endif
			dev->sc_sync[target].period = sbic_min_period;
			dev->sc_sync[target].offset = 0;
			dev->sc_sync[target].state = SYNC_DONE;
			SET_SBIC_syn(regs, SBIC_SYN(dev->sc_sync[target].offset,
			    dev->sc_sync[target].period));
			/* ACK the message */
			SET_SBIC_cmd(regs, SBIC_CMD_CLR_ACK);
			WAIT_CIP(regs);
			phase = CMD_PHASE;  /* or whatever */
		} else if (dev->sc_msg[0] == MSG_REJECT) {
			/*
			 * we'll never REJECt a REJECT message..
			 */
			/* ACK the message */
			SET_SBIC_cmd(regs, SBIC_CMD_CLR_ACK);
			WAIT_CIP(regs);
			phase = CMD_PHASE;  /* or whatever */
		} else if (dev->sc_msg[0] == MSG_CMD_COMPLETE) {
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

#ifdef DEBUG
			if (sync_debug)
				printf ("GOT CMD-COMPLETE! %d acting weird.."
				    " waiting for disconnect...\n", target);
#endif
			/* ACK the message */
			SET_SBIC_cmd (regs, SBIC_CMD_CLR_ACK);
			WAIT_CIP(regs);

			/* wait for disconnect */
			while (csr != SBIC_CSR_DISC && 
			    csr != SBIC_CSR_DISC_1) {
				DELAY(1);
				GET_SBIC_csr(regs, csr);
			}
#ifdef DEBUG
			if (sync_debug)
				printf ("ok.\nRetrying selection.\n");
#endif
			dev->sc_flags &= ~SBICF_SELECTED;
			goto retry_selection;
		} else {
#ifdef DEBUG
			if (sbic_debug || sync_debug)
				printf ("Rejecting message 0x%02x\n",
				    dev->sc_msg[0]);
#endif
			/* prepare to reject the message, NACK */
			SET_SBIC_cmd(regs, SBIC_CMD_SET_ATN);
			WAIT_CIP(regs);
			SET_SBIC_cmd(regs, SBIC_CMD_CLR_ACK);
			WAIT_CIP(regs);
			phase = MESG_OUT_PHASE;
		}
		break;

	case MESG_OUT_PHASE:
#ifdef DEBUG
		if (sync_debug)
			printf ("sending REJECT msg to last msg.\n");
#endif
		/*
		 * should only get here on reject, 
		 * since it's always US that
		 * initiate a sync transfer
		 */
		SEND_BYTE(regs, MSG_REJECT);
		phase = STATUS_PHASE;
		break;
	case DATA_OUT_PHASE:
		if (len <= 0)
			goto abort;
		wait = sbic_data_wait;
		if (sbicxfstart(regs, len, phase, wait))
			if (sbicxfout (regs, len, buf, phase))
				goto abort;
		phase = STATUS_PHASE;
		break;
	case STATUS_PHASE:
		/* 
		 * the sbic does the status/cmd-complete reading ok,
		 * so do this with its hi-level commands.
		 */
		SBIC_TC_PUT(regs, 0);
		SET_SBIC_cmd_phase(regs, 0x46);
		SET_SBIC_cmd(regs, SBIC_CMD_SEL_ATN_XFER);
		phase = BUS_FREE_PHASE;
		break;
	case BUS_FREE_PHASE:
		goto out;
	default:
		printf("%s: unexpected phase %d in icmd from %d\n",
		    dev->sc_dev.dv_xname, phase, target);
		goto abort;
	}

	/*
	 * make sure the last command was taken, 
	 * ie. we're not hunting after an ignored command..
	 */
	GET_SBIC_asr(regs, asr);
	if (asr & SBIC_ASR_LCI)
		goto abort;

	/* tapes may take a loooong time.. */
	while (asr & SBIC_ASR_BSY) {
		DELAY(1);
		GET_SBIC_asr(regs, asr);
	}

	/* 
	 * wait for last command to complete
	 */
	SBIC_WAIT (regs, SBIC_ASR_INT, wait);

	/*
	 * do it again
	 */
	goto new_phase;
abort:
	sbicabort(dev, regs, "icmd");
out:
	QPRINTF(("=STS:%02x=", dev->sc_stat[0]));
	return(dev->sc_stat[0]);
}

/*
 * Finish SCSI xfer command:  After the completion interrupt from
 * a read/write operation, sequence through the final phases in
 * programmed i/o.  This routine is a lot like sbicicmd except we
 * skip (and don't allow) the select, cmd out and data in/out phases.
 */
void
sbicxfdone(dev, regs, target)
	struct sbic_softc *dev;
	sbic_regmap_p regs;
	int target;
{
	u_char phase, csr;
	int s;

	QPRINTF(("{"));
	s = splbio();

	/*
	 * have the sbic complete on its own
	 */
	SBIC_TC_PUT(regs, 0);
	SET_SBIC_cmd_phase(regs, 0x46);
	SET_SBIC_cmd(regs, SBIC_CMD_SEL_ATN_XFER);

	do {
		SBIC_WAIT (regs, SBIC_ASR_INT, 0);
		GET_SBIC_csr (regs, csr);
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
}

int
sbicgo(dev, xs)
	struct sbic_softc *dev;
	struct scsi_xfer *xs;
{
	int i, dmaflags, count, tcount, target, len, wait;
	u_char phase, csr, asr, cmd, *addr, *tmpaddr;
	sbic_regmap_p regs;
	struct dma_chain *dcp;
	u_int deoff, dspa;
	char *dmaend;

	target = xs->sc_link->target;
	count = xs->datalen;
	addr = xs->data;

	regs = dev->sc_sbicp;
	dmaend = NULL;

	/*
	 * set the sbic into DMA mode
	 */
	SET_SBIC_control(regs, SBIC_CTL_EDI | SBIC_CTL_IDI |
	    SBIC_MACHINE_DMA_MODE);

	/*
	 * select the SCSI bus (it's an error if bus isn't free)
	 */
	if (sbicselectbus(dev, regs, target, dev->sc_scsiaddr)) {
		dev->sc_dmafree(dev);
		return(-1);
	}

	/*
	 * Wait for a phase change (or error) then let the device
	 * sequence us through command phase (we may have to take
	 * a msg in/out before doing the command).  If the disk has
	 * to do a seek, it may be a long time until we get a change
	 * to data phase so, in the absense of an explicit phase
	 * change, we assume data phase will be coming up and tell
	 * the SPC to start a transfer whenever it does.  We'll get
	 * a service required interrupt later if this assumption is
	 * wrong.  Otherwise we'll get a service required int when
	 * the transfer changes to status phase.
	 */
	phase = CMD_PHASE;

new_phase:
	wait = sbic_cmd_wait;
	switch (phase) {
	case CMD_PHASE:
		if (sbicxfstart(regs, xs->cmdlen, phase, wait))
			if (sbicxfout(regs, xs->cmdlen, xs->cmd, phase))
				goto abort;
		break;
	case MESG_IN_PHASE:
		if (sbicxfstart(regs, sizeof(dev->sc_msg), phase, wait) == 0)
			break;

		sbicxfin(regs, sizeof(dev->sc_msg), dev->sc_msg);
		/*
		 * prepare to reject any mesgin, 
		 * no matter what it might be..
		 */
		SET_SBIC_cmd(regs, SBIC_CMD_SET_ATN);
		WAIT_CIP(regs);
		SET_SBIC_cmd(regs, SBIC_CMD_CLR_ACK);
		phase = MESG_OUT_PHASE;
		break;
	case MESG_OUT_PHASE:
		SEND_BYTE(regs, MSG_REJECT);
		phase = STATUS_PHASE;
		break;
	case DATA_IN_PHASE:
	case DATA_OUT_PHASE:
		goto out;
	/*
	 * status phase can happen, if the issued read/write command
	 * is illegal (for example, reading after EOT on tape) and the 
	 * device doesn't even go to data in/out phase. So handle this 
	 * here normally, instead of going thru abort-handling.
	 */
	case STATUS_PHASE:
		dev->sc_dmafree(dev);
		sbicxfdone(dev, regs, target);
		dev->sc_flags &= ~(SBICF_INDMA | SBICF_BBUF);
		sbic_scsidone(dev, dev->sc_stat[0]);
		return(0);
	default:
		printf("%s: unexpected phase %d in go from %d\n", phase,
		    dev->sc_dev.dv_xname, target);
		goto abort;
	}

	/*
	 * make sure the last command was taken, 
	 * ie. we're not hunting after an ignored command..
	 */
	GET_SBIC_asr(regs, asr);
	if (asr & SBIC_ASR_LCI)
		goto abort;

	/* 
	 * tapes may take a loooong time..
	 */
	while (asr & SBIC_ASR_BSY) {
		DELAY(1);
		GET_SBIC_asr(regs, asr);
	}

	if (wait <= 0)            
		goto abort;

	/*
	 * wait for last command to complete
	 */
	SBIC_WAIT(regs, SBIC_ASR_INT, wait);

	GET_SBIC_csr(regs, csr);
	QPRINTF((">CSR:%02x<", csr));

	/* 
	 * requesting some new phase
	 */
	if ((csr != 0xff) && (csr & 0xf0) && (csr & 0x08))
		phase = csr & PHASE;
	else {
		sbicerror(dev, regs, csr);
		goto abort;
	}
	/*
	 * start again with for new phase
	 */
	goto new_phase;
out:
	dmaflags = 0;
	if (xs->flags & SCSI_DATA_IN)
		dmaflags |= DMAGO_READ;

	if (count > MAXPHYS)
		printf("sbicgo: bp->b_bcount > MAXPHYS %08x\n", count);

	if (dev->sc_flags & SBICF_BADDMA && 
	    sbiccheckdmap(addr, count, dev->sc_dmamask)) {
		/*
		 * need to bounce the dma.
		 */
		if (dmaflags & DMAGO_READ) {
			dev->sc_flags |= SBICF_BBUF;
			dev->sc_dmausrbuf = addr;
			dev->sc_dmausrlen = count;
		} else {	/* write: copy to dma buffer */
			bcopy (addr, dev->sc_dmabuffer, count);
		}
		addr = dev->sc_dmabuffer;	/* and use dma buffer */
	}
	tmpaddr = addr;
	len = count;
#ifdef DEBUG
	if (sbic_dma_debug & DDB_FOLLOW)
		printf("sbicgo(%d, %x, %x, %x)\n", dev->sc_dev.dv_unit,
		    addr, count, dmaflags);
#endif
	/*
	 * Build the DMA chain
	 */
	for (dcp = dev->sc_chain; count > 0; dcp++) {
		dcp->dc_addr = (char *) kvtop(addr);
		if (count < (tcount = NBPG - ((int)addr & PGOFSET)))
			tcount = count;
		addr += tcount;
		count -= tcount;
		dcp->dc_count = tcount >> 1;

		/*
		 * check if contigous, if not mark new end 
		 * else increment end and count on previous.
		 */
		if (dcp->dc_addr != dmaend)
			dmaend = dcp->dc_addr + tcount;
		else {
			dcp--;
			dmaend += tcount;
			dcp->dc_count += tcount >> 1;
		}
	}

	dev->sc_cur = dev->sc_chain;
	dev->sc_last = --dcp;
	dev->sc_tcnt = dev->sc_cur->dc_count << 1;

#ifdef DEBUG
	if (sbic_dma_debug & DDB_IO) {
		for (dcp = dev->sc_chain; dcp <= dev->sc_last; dcp++)
			printf("  %d: %d@%x\n", dcp-dev->sc_chain,
			    dcp->dc_count, dcp->dc_addr);
	}
#endif

	/*
	 * push the data cash
	 */
#if 0
	DCIS();
#elif defined(M68040)
	if (cpu040) {
		dma_cachectl(tmpaddr, len);

		dspa = (u_int)dev->sc_chain[0].dc_addr;
		deoff = (u_int)dev->sc_last->dc_addr
		    + (dev->sc_last->dc_count >> 1);
		if ((dspa & 0xF) || (deoff & 0xF))
			dev->sc_flags |= SBICF_DCFLUSH;
	}
#endif

	/*
	 * dmago() also enables interrupts for the sbic
	 */
	i = dev->sc_dmago(dev, addr, xs->datalen, dmaflags);

	SBIC_TC_PUT(regs, (unsigned)i);
	SET_SBIC_cmd(regs, SBIC_CMD_XFER_INFO);

	return(0);

abort:
	sbicabort(dev, regs, "go");
	dev->sc_dmafree(dev);
	return(-1);
}


int
sbicintr(dev)
	struct sbic_softc *dev;
{
	sbic_regmap_p regs;
	struct dma_chain *df, *dl;
	u_char asr, csr;
	int i;

	regs = dev->sc_sbicp;

	/*
	 * pending interrupt?
	 */
	GET_SBIC_asr (regs, asr);
	if ((asr & SBIC_ASR_INT) == 0)
		return(0);

	GET_SBIC_csr(regs, csr);
	QPRINTF(("[0x%x]", csr));

	if (csr == (SBIC_CSR_XFERRED|STATUS_PHASE)
	    || csr == (SBIC_CSR_MIS|STATUS_PHASE)
	    || csr == (SBIC_CSR_MIS_1|STATUS_PHASE)
	    || csr == (SBIC_CSR_MIS_2|STATUS_PHASE)) {
		/*
		 * this should be the normal i/o completion case.
		 * get the status & cmd complete msg then let the
		 * device driver look at what happened.
		 */
		sbicxfdone(dev, regs, dev->sc_xs->sc_link->target);
		if (dev->sc_flags & SBICF_BBUF)
			bcopy(dev->sc_dmabuffer, dev->sc_dmausrbuf,
			    dev->sc_dmausrlen);
		/*
		 * check for overlapping cache line, flush if so
		 */
#ifdef M68040
		if (dev->sc_flags & SBICF_DCFLUSH) {
			df = dev->sc_chain;
			dl = dev->sc_last;
			DCFL(df->dc_addr);
			DCFL(dl->dc_addr + (dl->dc_count >> 1));
		}
#endif
		dev->sc_flags &= ~(SBICF_INDMA | SBICF_BBUF | SBICF_DCFLUSH);
		dev->sc_dmafree(dev);
		sbic_scsidone(dev, dev->sc_stat[0]);
	} else if (csr == (SBIC_CSR_XFERRED|DATA_OUT_PHASE)
	    || csr == (SBIC_CSR_XFERRED|DATA_IN_PHASE)
	    || csr == (SBIC_CSR_MIS|DATA_OUT_PHASE)
	    || csr == (SBIC_CSR_MIS|DATA_IN_PHASE)
	    || csr == (SBIC_CSR_MIS_1|DATA_OUT_PHASE)
	    || csr == (SBIC_CSR_MIS_1|DATA_IN_PHASE)
	    || csr == (SBIC_CSR_MIS_2|DATA_OUT_PHASE)
	    || csr == (SBIC_CSR_MIS_2|DATA_IN_PHASE)) {
		/*
		 * do scatter-gather dma 
		 * hacking the controller chip, ouch..
		 */
		/*
		 * set next dma addr and dec count
		 */
		dev->sc_cur->dc_addr += dev->sc_tcnt;
		dev->sc_cur->dc_count -= (dev->sc_tcnt >> 1);

		if (dev->sc_cur->dc_count == 0)
			++dev->sc_cur;		/* advance to next segment */

		i = dev->sc_dmanext(dev);
		SBIC_TC_PUT(regs, (unsigned)i);
		SET_SBIC_cmd(regs, SBIC_CMD_XFER_INFO);
	} else {
		/*
		 * Something unexpected happened -- deal with it.
		 */
		dev->sc_dmastop(dev);
		sbicerror(dev, regs, csr);
		sbicabort(dev, regs, "intr");
		if (dev->sc_flags & SBICF_INDMA) {
			/*
			 * check for overlapping cache line, flush if so
			 */
#ifdef M68040
			if (dev->sc_flags & SBICF_DCFLUSH) {
				df = dev->sc_chain;
				dl = dev->sc_last;
				DCFL(df->dc_addr);
				DCFL(dl->dc_addr + (dl->dc_count >> 1));
			}
#endif
			dev->sc_flags &= 
			    ~(SBICF_INDMA | SBICF_BBUF | SBICF_DCFLUSH);
			dev->sc_dmafree(dev);
			sbic_scsidone(dev, -1);
		}
	}
	return(1);
}

/*
 * Check if DMA can not be used with specified buffer
 */

int
sbiccheckdmap(bp, len, mask)
	void *bp;
	u_long len, mask;
{
	u_char *buffer;
	u_long phy_buf;
	u_long phy_len;

	buffer = bp;

	if (len == 0)
		return(0);

	while (len) {
		phy_buf = kvtop(buffer);
		if (len < (phy_len = NBPG - ((int) buffer & PGOFSET)))
			phy_len = len;
		if (phy_buf & mask)
			return(1);
		buffer += phy_len;
		len -= phy_len;
	}
	return(0);
}

int 
sbictoscsiperiod(dev, regs, a)
	struct sbic_softc *dev;
	sbic_regmap_p regs;
	int a;
{
	unsigned int fs;
  
	/*
	 * cycle = DIV / (2*CLK)
	 * DIV = FS+2
	 * best we can do is 200ns at 20Mhz, 2 cycles
	 */
  
	GET_SBIC_myid(regs,fs);
	fs = (fs >>6) + 2;		/* DIV */
	fs = (fs * 10000) / (dev->sc_clkfreq<<1);	/* Cycle, in ns */
	if (a < 2) a = 8;		/* map to Cycles */
	return ((fs*a)>>2);		/* in 4 ns units */
}

int 
sbicfromscsiperiod(dev, regs, p)
	struct sbic_softc *dev;
	sbic_regmap_p regs;
	int p;
{
	register unsigned int fs, ret;
  
	/* Just the inverse of the above */
  
	GET_SBIC_myid(regs,fs);
	fs = (fs >>6) + 2;		/* DIV */
	fs = (fs * 10000) / (dev->sc_clkfreq<<1);   /* Cycle, in ns */
  
	ret = p << 2;			/* in ns units */
	ret = ret / fs;			/* in Cycles */
	if (ret < sbic_min_period)
		return(sbic_min_period);

	/* verify rounding */
	if (sbictoscsiperiod(dev, regs, ret) < p)
		ret++;
	return (ret >= 8) ? 0 : ret;
}

