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
 *
 * MULTICONTROLLER support only working for multiple controllers of the 
 * same kind at the moment !! 
 *
 */

/*
 * AMIGA AMD 33C93 scsi adaptor driver
 */
#include "a3000scsi.h"
#include "a2091scsi.h"
#include "gvp11scsi.h"
#define NSCSI (NA3000SCSI + NA2091SCSI + NGVP11SCSI)
#if NSCSI > 0

#ifndef lint
static char rcsid[] = "$Header: /cvsroot/src/sys/arch/amiga/dev/Attic/scsi.c,v 1.4 1993/10/30 23:41:33 mw Exp $";
#endif

#include "sys/param.h"
#include "sys/systm.h"
#include "sys/buf.h"
#include "device.h"

#include "dmavar.h"
#include "scsivar.h"
#include "scsireg.h"

#include "../amiga/custom.h"

#include "machine/cpu.h"

static int sbic_wait (volatile sbic_padded_regmap_t *regs, char until, int timeo, int line);
static void scsiabort (register struct scsi_softc *dev, register volatile sbic_padded_regmap_t *regs, char *where);
static void scsierror (register struct scsi_softc *dev, register volatile sbic_padded_regmap_t *regs, u_char csr);
static int issue_select (register struct scsi_softc *dev, register volatile sbic_padded_regmap_t *regs, u_char target, u_char our_addr);
static int wait_for_select (register struct scsi_softc *dev, register volatile sbic_padded_regmap_t *regs);
static int ixfer_start (register volatile sbic_padded_regmap_t *regs, int len, u_char phase, register int wait);
static int ixfer_out (register volatile sbic_padded_regmap_t *regs, int len, register u_char *buf, int phase);
static void ixfer_in (register volatile sbic_padded_regmap_t *regs, int len, register u_char *buf);
static int scsiicmd (struct scsi_softc *dev, int target, u_char *cbuf, int clen, u_char *buf, int len, u_char xferphase);
static void finishxfer (struct scsi_softc *dev, register volatile sbic_padded_regmap_t *regs, int target);



/*
 * SCSI delays
 * In u-seconds, primarily for state changes on the SPC.
 */
#define	SCSI_CMD_WAIT	50000	/* wait per step of 'immediate' cmds */
#define	SCSI_DATA_WAIT	50000	/* wait per data in/out step */
#define	SCSI_INIT_WAIT	50000	/* wait per step (both) during init */

extern void _insque();
extern void _remque();

#if 0
int	scsiinit(), scsigo(), scsiintr(), scsixfer();
void	scsistart(), scsidone(), scsifree(), scsireset();
struct	driver scsidriver = {
	scsiinit, "scsi", (int (*)())scsistart, scsigo, scsiintr,
	(int (*)())scsidone,
};
#endif


#if NA3000SCSI > 0
int a3000scsiinit();

struct driver a3000scsidriver = {
        a3000scsiinit, "a3000scsi", (int (*)())scsistart, scsigo, scsiintr,
	(int (*)())scsidone,
};
#endif

#if NA2091SCSI > 0
int a2091scsiinit();

struct driver a2091scsidriver = {
        a2091scsiinit, "a2091scsi", (int (*)())scsistart, scsigo, scsiintr,
	(int (*)())scsidone,
};
#endif

#if NGVP11SCSI > 0
int gvp11scsiinit();

struct driver gvp11scsidriver = {
        gvp11scsiinit, "GVPIIscsi", (int (*)())scsistart, scsigo, scsiintr,
	(int (*)())scsidone,
};
#endif


struct	scsi_softc scsi_softc[NSCSI];

int scsi_cmd_wait = SCSI_CMD_WAIT;
int scsi_data_wait = SCSI_DATA_WAIT;
int scsi_init_wait = SCSI_INIT_WAIT;

/*
 * Synch xfer parameters, and timing conversions
 */
static int sbic_min_period = SBIC_SYN_MIN_PERIOD;  /* in cycles = f(ICLK,FSn) */
static int sbic_max_offset = SBIC_SYN_MAX_OFFSET;  /* pure number */

static int 
sbic_to_scsi_period(dev, regs, a)
     struct scsi_softc *dev;
     sbic_padded_regmap_t *regs;
{
  unsigned int fs;
  
  /* cycle = DIV / (2*CLK) */
  /* DIV = FS+2 */
  /* best we can do is 200ns at 20Mhz, 2 cycles */
  
  GET_SBIC_myid(regs,fs);
  fs = (fs >>6) + 2;                              /* DIV */
  fs = (fs * 10000) / (dev->sc_clock_freq<<1);   /* Cycle, in ns */
  if (a < 2) a = 8;                               /* map to Cycles */
  return ((fs*a)>>2);                             /* in 4 ns units */
}

static int 
scsi_period_to_sbic(dev, regs, p)
     struct scsi_softc *dev;
     sbic_padded_regmap_t *regs;
{
  register unsigned int fs, ret;
  
  /* Just the inverse of the above */
  
  GET_SBIC_myid(regs,fs);
  fs = (fs >>6) + 2;                              /* DIV */
  fs = (fs * 10000) / (dev->sc_clock_freq<<1);   /* Cycle, in ns */
  
  ret = p << 2;                                   /* in ns units */
  ret = ret / fs;                                 /* in Cycles */
  if (ret < sbic_min_period)
    return sbic_min_period;
  /* verify rounding */
  if (sbic_to_scsi_period(dev, regs, ret) < p)
    ret++;
  return (ret >= 8) ? 0 : ret;
}

/* default to not inhibit sync negotiation on any drive */
u_char inhibit_sync[NSCSI][8] = { 0, 0, 0, 0, 1, 0, 0 }; /* initialize, so patchable */
int scsi_no_dma = 0;

#ifdef DEBUG
int	scsi_debug = 0;
int	sync_debug = 0;
#define WAITHIST
#define QUASEL
#endif

#ifdef QUASEL
#define QPRINTF(a) if (scsi_debug > 1) printf a
#else
#define QPRINTF
#endif

#ifdef WAITHIST
#define MAXWAIT	1022
u_int	ixstart_wait[MAXWAIT+2];
u_int	ixin_wait[MAXWAIT+2];
u_int	ixout_wait[MAXWAIT+2];
u_int	mxin_wait[MAXWAIT+2];
u_int	mxin2_wait[MAXWAIT+2];
u_int	cxin_wait[MAXWAIT+2];
u_int	fxfr_wait[MAXWAIT+2];
u_int	sgo_wait[MAXWAIT+2];
#define HIST(h,w) (++h[((w)>MAXWAIT? MAXWAIT : ((w) < 0 ? -1 : (w))) + 1]);
#else
#define HIST(h,w)
#endif

#define	b_cylin		b_resid

static sbic_wait (regs, until, timeo, line)
    volatile sbic_padded_regmap_t *regs;
    char                        until;
    int				timeo;
    int				line;
{
  register unsigned char        val;

  if (! timeo)
    timeo = 1000000;	/* some large value.. */

  GET_SBIC_asr (regs,val);
  while ((val & until) == 0) 
    {
      if (!timeo--) 
        {
	  int csr;
	  GET_SBIC_csr (regs, csr);
          printf("sbic_wait TIMEO @%d with asr=x%x csr=x%x\n", line, val, csr);
          break;
        }
      DELAY(1);
      GET_SBIC_asr(regs,val);
    }
  return val;
}

#define SBIC_WAIT(regs, until, timeo) sbic_wait (regs, until, timeo, __LINE__)


static void
scsiabort(dev, regs, where)
	register struct scsi_softc *dev;
	volatile register sbic_padded_regmap_t *regs;
	char *where;
{
  u_char csr, asr;
  
  GET_SBIC_csr (regs, csr);
  GET_SBIC_asr (regs, asr);

  printf ("scsi%d: abort %s: csr = 0x%02x, asr = 0x%02x\n", 
	  dev->sc_ac->amiga_unit,
	  where, csr, asr);

  if (dev->sc_flags & SCSI_SELECTED)
    {
      SET_SBIC_cmd (regs, SBIC_CMD_ABORT);
      WAIT_CIP (regs);

      GET_SBIC_asr (regs, asr);
      if (asr & (SBIC_ASR_BSY|SBIC_ASR_LCI))
        {
          /* ok, get more drastic.. */
          
	  SET_SBIC_cmd (regs, SBIC_CMD_RESET);
	  DELAY(25);
	  SBIC_WAIT(regs, SBIC_ASR_INT, 0);
	  GET_SBIC_csr (regs, csr);       /* clears interrupt also */

          dev->sc_flags &= ~SCSI_SELECTED;
          return;
        }

      do
        {
          SBIC_WAIT (regs, SBIC_ASR_INT, 0);
          GET_SBIC_csr (regs, csr);
        }
      while ((csr != SBIC_CSR_DISC) && (csr != SBIC_CSR_DISC_1)
	      && (csr != SBIC_CSR_CMD_INVALID));
	      
      /* lets just hope it worked.. */
      dev->sc_flags &= ~SCSI_SELECTED;
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
scsi_delay(delay)
	int delay;
{
	static int saved_cmd_wait, saved_data_wait;

	if (delay) {
		saved_cmd_wait = scsi_cmd_wait;
		saved_data_wait = scsi_data_wait;
		if (delay > 0)
			scsi_cmd_wait = scsi_data_wait = delay;
		else
			scsi_cmd_wait = scsi_data_wait = scsi_init_wait;
	} else {
		scsi_cmd_wait = saved_cmd_wait;
		scsi_data_wait = saved_data_wait;
	}
}

static int initialized[NSCSI];
int scsi_clock_override = 0;

#if NA3000SCSI > 0
int
a3000scsiinit(ac)
	register struct amiga_ctlr *ac;
{
  register struct scsi_softc *dev = &scsi_softc[ac->amiga_unit];
  register sbic_padded_regmap_t *regs;

  if (! ac->amiga_addr)
    return 0;

  if (initialized[ac->amiga_unit])
    return 0;
 
  initialized[ac->amiga_unit] = 1;
  
  /* initialize dma */
  a3000dmainit (ac, &dev->dmareq, &dev->dmafree, &dev->dmago,
		&dev->dmanext, &dev->dmastop);

  /* advance ac->amiga_addr to point to the real sbic-registers */
  ac->amiga_addr = (caddr_t) ((int)ac->amiga_addr + 0x41);
  regs = (sbic_padded_regmap_t *) ac->amiga_addr;
  dev->sc_clock_freq = scsi_clock_override ? scsi_clock_override : 143;


  /* hardwired IPL */
  ac->amiga_ipl = 2;
  dev->sc_ac = ac;
  dev->sc_dq.dq_driver = &a3000scsidriver;
  dev->sc_sq.dq_forw = dev->sc_sq.dq_back = &dev->sc_sq;
  scsireset (ac->amiga_unit);

  /* make sure IPL2 interrupts are delivered to the cpu when the sbic
     generates some. Note that this does not yet enable sbic-interrupts,
     this is handled in dma.c, which selectively enables interrupts only
     while DMA requests are pending.
     
     Note that enabling PORTS interrupts also enables keyboard interrupts
     as soon as the corresponding int-enable bit in CIA-A is set. */
     
  custom.intreq = INTF_PORTS;
  custom.intena = INTF_SETCLR | INTF_PORTS;
  return(1);
}
#endif

#if NA2091SCSI > 0
int
a2091scsiinit(ac)
     register struct amiga_ctlr *ac;
{
  register struct scsi_softc *dev = &scsi_softc[ac->amiga_unit];
  register sbic_padded_regmap_t *regs;

  if (! ac->amiga_addr)
    return 0;

  if (initialized[ac->amiga_unit])
    return 0;
 
  initialized[ac->amiga_unit] = 1;
  
  /* initialize dma */
  a2091dmainit (ac, &dev->dmareq, &dev->dmafree, &dev->dmago,
		&dev->dmanext, &dev->dmastop);

  /* advance ac->amiga_addr to point to the real sbic-registers */
  ac->amiga_addr = (caddr_t) ((int)ac->amiga_addr + 0x91);
  regs = (sbic_padded_regmap_t *) ac->amiga_addr;
  dev->sc_clock_freq = scsi_clock_override ? scsi_clock_override : 77;


  /* hardwired IPL */
  ac->amiga_ipl = 2;
  dev->sc_ac = ac;
  dev->sc_dq.dq_driver = &a2091scsidriver;
  dev->sc_sq.dq_forw = dev->sc_sq.dq_back = &dev->sc_sq;
  scsireset (ac->amiga_unit);

  /* make sure IPL2 interrupts are delivered to the cpu when the sbic
     generates some. Note that this does not yet enable sbic-interrupts,
     this is handled in dma.c, which selectively enables interrupts only
     while DMA requests are pending.
     
     Note that enabling PORTS interrupts also enables keyboard interrupts
     as soon as the corresponding int-enable bit in CIA-A is set. */
     
  custom.intreq = INTF_PORTS;
  custom.intena = INTF_SETCLR | INTF_PORTS;
  return(1);
}
#endif

#if NGVP11SCSI > 0
int
gvp11scsiinit(ac)
     register struct amiga_ctlr *ac;
{
  register struct scsi_softc *dev = &scsi_softc[ac->amiga_unit];
  register sbic_padded_regmap_t *regs;

  if (! ac->amiga_addr)
    return 0;

  if (initialized[ac->amiga_unit])
    return 0;
 
  initialized[ac->amiga_unit] = 1;
  
  /* initialize dma */
  gvp11dmainit (ac, &dev->dmareq, &dev->dmafree, &dev->dmago,
		&dev->dmanext, &dev->dmastop);

  /* advance ac->amiga_addr to point to the real sbic-registers */
  ac->amiga_addr = (caddr_t) ((int)ac->amiga_addr + 0x61);
  regs = (sbic_padded_regmap_t *) ac->amiga_addr;
  dev->sc_clock_freq = scsi_clock_override ? scsi_clock_override : 77;


  /* hardwired IPL */
  ac->amiga_ipl = 2;
  dev->sc_ac = ac;
  dev->sc_dq.dq_driver = &gvp11scsidriver;
  dev->sc_sq.dq_forw = dev->sc_sq.dq_back = &dev->sc_sq;
  scsireset (ac->amiga_unit);

  /* make sure IPL2 interrupts are delivered to the cpu when the sbic
     generates some. Note that this does not yet enable sbic-interrupts,
     this is handled in dma.c, which selectively enables interrupts only
     while DMA requests are pending.
     
     Note that enabling PORTS interrupts also enables keyboard interrupts
     as soon as the corresponding int-enable bit in CIA-A is set. */
     
  custom.intreq = INTF_PORTS;
  custom.intena = INTF_SETCLR | INTF_PORTS;
  return(1);
}
#endif


void
scsireset(unit)
     register int unit;
{
  register struct scsi_softc *dev = &scsi_softc[unit];
  volatile register sbic_padded_regmap_t *regs =
				(sbic_padded_regmap_t *)dev->sc_ac->amiga_addr;
  u_int i, s;
  u_char  my_id, csr;

  if (dev->sc_flags & SCSI_ALIVE)
    scsiabort(dev, regs, "reset");
		
  printf("scsi%d: ", unit);

  s = splbio();
  /* preserve our ID for now */
  GET_SBIC_myid (regs, my_id);
  my_id &= SBIC_ID_MASK;

  if (dev->sc_clock_freq < 110)
    my_id |= SBIC_ID_FS_8_10;
  else if (dev->sc_clock_freq < 160)
    my_id |= SBIC_ID_FS_12_15;
  else if (dev->sc_clock_freq < 210)
    my_id |= SBIC_ID_FS_16_20;

  my_id |= SBIC_ID_EAF/* |SBIC_ID_EHP*/;

  SET_SBIC_myid (regs, my_id);

  /*
   * Disable interrupts (in dmainit) then reset the chip
   */
  SET_SBIC_cmd (regs, SBIC_CMD_RESET);
  DELAY(25);
  SBIC_WAIT(regs, SBIC_ASR_INT, 0);
  GET_SBIC_csr (regs, csr);       /* clears interrupt also */

  /*
   * Set up various chip parameters
   */
  SET_SBIC_control (regs, (/*SBIC_CTL_HHP |*/ SBIC_CTL_EDI | SBIC_CTL_IDI |
                           /* | SBIC_CTL_HSP | */ SBIC_MACHINE_DMA_MODE));
  /* don't allow (re)selection (SBIC_RID_ES) until we can handle target mode!! */
  SET_SBIC_rselid (regs, 0 /* | SBIC_RID_ER | SBIC_RID_ES | SBIC_RID_DSP */);
  SET_SBIC_syn (regs, 0);     /* asynch for now */

  /* anything else was zeroed by reset */

#if 0
  /* go async for now */
  dev->sc_sync = 0;
  printf ("async, ");
#endif

  splx (s);

  printf("scsi id %d\n", my_id & SBIC_ID_MASK);
  dev->sc_flags |= SCSI_ALIVE;
  dev->sc_flags &= ~SCSI_SELECTED;
}

static void
scsierror(dev, regs, csr)
	register struct scsi_softc *dev;
	volatile register sbic_padded_regmap_t *regs;
	u_char csr;
{
        int unit = dev->sc_ac->amiga_unit;
	char *sep = "";

	printf("scsi%d: ", unit);
#if 0
	if (ints & INTS_RST) {
		DELAY(100);
		if (regs->scsi_aconf & HCONF_SD)
			printf("spurious RST interrupt");
		else
			printf("hardware error - check fuse");
		sep = ", ";
	}
	if ((ints & INTS_HARD_ERR) || regs->scsi_serr) {
		if (regs->scsi_serr & SERR_SCSI_PAR) {
			printf("%sparity err", sep);
			sep = ", ";
		}
		if (regs->scsi_serr & SERR_SPC_PAR) {
			printf("%sSPC parity err", sep);
			sep = ", ";
		}
		if (regs->scsi_serr & SERR_TC_PAR) {
			printf("%sTC parity err", sep);
			sep = ", ";
		}
		if (regs->scsi_serr & SERR_PHASE_ERR) {
			printf("%sphase err", sep);
			sep = ", ";
		}
		if (regs->scsi_serr & SERR_SHORT_XFR) {
			printf("%ssync short transfer err", sep);
			sep = ", ";
		}
		if (regs->scsi_serr & SERR_OFFSET) {
			printf("%ssync offset error", sep);
			sep = ", ";
		}
	}
	if (ints & INTS_TIMEOUT)
		printf("%sSPC select timeout error", sep);
	if (ints & INTS_SRV_REQ)
		printf("%sspurious SRV_REQ interrupt", sep);
	if (ints & INTS_CMD_DONE)
		printf("%sspurious CMD_DONE interrupt", sep);
	if (ints & INTS_DISCON)
		printf("%sspurious disconnect interrupt", sep);
	if (ints & INTS_RESEL)
		printf("%sspurious reselect interrupt", sep);
	if (ints & INTS_SEL)
		printf("%sspurious select interrupt", sep);
#else
	printf ("csr == 0x%02x", csr);	/* XXX */
#endif
	printf("\n");
}

static int
issue_select(dev, regs, target, our_addr)
        register struct scsi_softc *dev;
	volatile register sbic_padded_regmap_t *regs;
	u_char target, our_addr;
{
  u_char  asr, csr;

  QPRINTF (("issue_select %d\n", target));

  /* if we're already selected, return */
  if (dev->sc_flags & SCSI_SELECTED)	/* XXXX */
    return 1;
  
  SBIC_TC_PUT (regs, 0);
  SET_SBIC_selid (regs, target);
  SET_SBIC_timeo (regs, SBIC_TIMEOUT(250,dev->sc_clock_freq));
  if (dev->sc_sync[target].state == SYNC_DONE)
    {
      /* set to negotiated values */
      SET_SBIC_syn (regs, SBIC_SYN (dev->sc_sync[target].offset, 
				    dev->sc_sync[target].period));
    }
  else
    {
      /* set to async */
      SET_SBIC_syn (regs, SBIC_SYN (0, sbic_min_period));
    }
  SET_SBIC_cmd (regs, SBIC_CMD_SEL_ATN);

  return (0);
}

static int
wait_for_select(dev, regs)
        register struct scsi_softc *dev;
	volatile register sbic_padded_regmap_t *regs;
{
  u_char  asr, csr;

  QPRINTF (("wait_for_select: "));

  WAIT_CIP (regs);
  do
    {
      SBIC_WAIT (regs, SBIC_ASR_INT, 0);
      GET_SBIC_csr (regs, csr);
      QPRINTF (("%02x ", csr));
    }
  while (csr != (SBIC_CSR_MIS_2|MESG_OUT_PHASE)
	 && csr != SBIC_CSR_SEL_TIMEO);

  /* Send identify message (SCSI-2 requires an identify msg (?)) */
  if (csr == (SBIC_CSR_MIS_2|MESG_OUT_PHASE))
    {
      u_char id;
      GET_SBIC_selid (regs, id);

      /* handle drives that don't want to be asked whether to go
	 sync at all. */
      if (inhibit_sync[dev->sc_ac->amiga_unit][id]
	  && dev->sc_sync[id].state == SYNC_START)
	{
#ifdef DEBUG
	  if (sync_debug)
	    printf ("Forcing target %d asynchronous.\n", id);
#endif
	  dev->sc_sync[id].offset = 0;
	  dev->sc_sync[id].period = sbic_min_period;
	  dev->sc_sync[id].state = SYNC_DONE;
	}
	

      if (dev->sc_sync[id].state == SYNC_START)
	{
	  /* then try to initiate a sync transfer.

	     So compose the sync message we're going to send to the target */

#ifdef DEBUG
	  if (sync_debug)
	    printf ("Sending sync request to target %d ... ", id);
#endif
	  dev->sc_msg[0] = MSG_IDENTIFY; /* no MSG_IDENTIFY_DR yet */
	  dev->sc_msg[1] = MSG_EXT_MESSAGE; /* sync request is extended message */
	  dev->sc_msg[2] = 3;		 /* length */
	  dev->sc_msg[3] = MSG_SYNC_REQ;
	  dev->sc_msg[4] = sbic_to_scsi_period (dev, regs, sbic_min_period);
	  dev->sc_msg[5] = sbic_max_offset;

	  if (ixfer_start (regs, 6, MESG_OUT_PHASE, scsi_cmd_wait)) 
	    ixfer_out (regs, 6, dev->sc_msg, MESG_OUT_PHASE);

	  dev->sc_sync[id].state = SYNC_SENT;
#ifdef DEBUG
	  if (sync_debug)
	    printf ("sent\n");
#endif
	}
      else
	{
	  /* just send identify then */
	  SEND_BYTE (regs, MSG_IDENTIFY);
	}

      SBIC_WAIT (regs, SBIC_ASR_INT, 0);
      GET_SBIC_csr (regs, csr);
      QPRINTF (("[%02x]", csr));
#ifdef DEBUG
      if (sync_debug && dev->sc_sync[id].state == SYNC_SENT)
	printf ("csr-result of last msgout: 0x%x\n", csr);
#endif

      if (csr != SBIC_CSR_SEL_TIMEO)
	dev->sc_flags |= SCSI_SELECTED;
    }
  
  QPRINTF(("\n"));

  return csr == SBIC_CSR_SEL_TIMEO;
}

static int
ixfer_start(regs, len, phase, wait)
	volatile register sbic_padded_regmap_t *regs;
	int len;
	u_char phase;
	register int wait;
{
#if 0
	regs->scsi_tch = len >> 16;
	regs->scsi_tcm = len >> 8;
	regs->scsi_tcl = len;
	regs->scsi_pctl = phase;
	regs->scsi_tmod = 0; /*XXX*/
	regs->scsi_scmd = SCMD_XFR | SCMD_PROG_XFR;

	/* wait for xfer to start or svc_req interrupt */
	while ((regs->scsi_ssts & SSTS_BUSY) == 0) {
		if (regs->scsi_ints || --wait < 0) {
#ifdef DEBUG
			if (scsi_debug)
				printf("ixfer_start fail: i%x, w%d\n",
				       regs->scsi_ints, wait);
#endif
			HIST(ixstart_wait, wait)
			return (0);
		}
		DELAY(1);
	}
	HIST(ixstart_wait, wait)
	return (1);
#else
	if (phase == DATA_IN_PHASE || phase == MESG_IN_PHASE)
	  {
	    u_char id;

	    GET_SBIC_selid (regs, id);
	    id |= SBIC_SID_FROM_SCSI;
	    SET_SBIC_selid (regs, id);
	    
	    SBIC_TC_PUT (regs, (unsigned)len);
	  }
	else if (phase == DATA_OUT_PHASE || phase == MESG_OUT_PHASE 
		 || phase == CMD_PHASE )
	  {
	    SBIC_TC_PUT (regs, (unsigned)len);
	  }
	else
	  { 
	    SBIC_TC_PUT (regs, 0);
	  }
  
	QPRINTF(("ixfer_start %d, %d, %d\n", len, phase, wait));

	return 1;
#endif
}

static int
ixfer_out(regs, len, buf, phase)
	volatile register sbic_padded_regmap_t *regs;
	int len;
	register u_char *buf;
	int phase;
{
  register int wait = scsi_data_wait;
  u_char orig_csr, csr, asr;

  QPRINTF(("ixfer_out {%d} %02x %02x %02x %02x %02x %02x\n", 
  	   len, buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]));

  GET_SBIC_csr (regs, orig_csr);

  /* sigh.. WD-PROTO strikes again.. sending the command in one go causes
     the chip to lock up if talking to certain (misbehaving?) targets.
     Anyway, this procedure should work for all targets, but it's slightly
     slower due to the overhead */
#if 0
  if (phase == CMD_PHASE)
    {
      WAIT_CIP (regs);
#if 0
      for (; len > 0; len--)
        {
	  /* clear possible last interrupt */
	  GET_SBIC_csr (regs, csr);

	  /* send the byte, and expect an interrupt afterwards */
	  SEND_BYTE (regs, *buf);
          buf++;
          SBIC_WAIT (regs, SBIC_ASR_INT, 0);	/* XXX  */
        }
#else
      SET_SBIC_cmd (regs, SBIC_CMD_XFER_INFO);
      for (;len > 0; len--)
        {
          WAIT_CIP (regs);
          GET_SBIC_csr (regs, csr);
	  SET_SBIC_cmd (regs, SBIC_CMD_XFER_INFO | SBIC_CMD_SBT);
          GET_SBIC_asr (regs, asr);
          while (!(asr & SBIC_ASR_DBR)) 
	    {
	      DELAY(1);
	      GET_SBIC_asr (regs, asr);
	    }

          SET_SBIC_data (regs, *buf);
          buf++;
          while (!(asr & SBIC_ASR_INT)) 
	    {
	      DELAY(1);
	      GET_SBIC_asr (regs, asr);
	    }

        }

#endif
    }
  else
#endif
    {
      WAIT_CIP (regs);
      SET_SBIC_cmd (regs, SBIC_CMD_XFER_INFO);
      for (;len > 0; len--)
        {
          GET_SBIC_asr (regs, asr);
          while (!(asr & SBIC_ASR_DBR)) 
	    {
              if ((asr & SBIC_ASR_INT) || --wait < 0) 
	        {
#ifdef DEBUG
	          if (scsi_debug)
		    printf("ixfer_out fail: l%d i%x w%d\n",
		           len, asr, wait);
#endif
	          HIST(ixout_wait, wait)
	          return (len);
	        }
	      DELAY(1);
	      GET_SBIC_asr (regs, asr);
	    }

          SET_SBIC_data (regs, *buf);
          buf++;
        }
    }

  QPRINTF(("ixfer_out done\n"));
  /* this leaves with one csr to be read */
  HIST(ixout_wait, wait)
  return (0);
}

static void
ixfer_in(regs, len, buf)
	volatile register sbic_padded_regmap_t *regs;
	int len;
	register u_char *buf;
{
  register int wait = scsi_data_wait;
  u_char *obp = buf;
  u_char orig_csr, csr, asr;

  GET_SBIC_csr (regs, orig_csr);

  QPRINTF(("ixfer_in %d, csr=%02x\n", len, orig_csr));

  WAIT_CIP (regs);
  SET_SBIC_cmd (regs, SBIC_CMD_XFER_INFO);
  for (;len > 0; len--)
    {
      GET_SBIC_asr (regs, asr);
      while (!(asr & SBIC_ASR_DBR)) 
	{
          if ((asr & SBIC_ASR_INT) || --wait < 0) 
	    {
#ifdef DEBUG
	      if (scsi_debug)
		printf("ixfer_in fail: l%d i%x w%d\n",
		       len, asr, wait);
#endif
	      HIST(ixin_wait, wait)
	      return;
	    }

	  DELAY(1);
	  GET_SBIC_asr (regs, asr);
	}

      GET_SBIC_data (regs, *buf);
      buf++;
    }

  QPRINTF(("ixfer_in {%d} %02x %02x %02x %02x %02x %02x\n", 
  	   len, obp[0], obp[1], obp[2], obp[3], obp[4], obp[5]));

  /* this leaves with one csr to be read */
  HIST(ixin_wait, wait)
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
static int
scsiicmd(dev, target, cbuf, clen, buf, len, xferphase)
	struct scsi_softc *dev;
	int target;
	u_char *cbuf;
	int clen;
	u_char *buf;
	int len;
	u_char xferphase;
{
  volatile register sbic_padded_regmap_t *regs =
				(sbic_padded_regmap_t *)dev->sc_ac->amiga_addr;
  u_char phase, csr, asr;
  register int wait;


  /* set the sbic into non-DMA mode */
  SET_SBIC_control (regs, (/*SBIC_CTL_HHP |*/ SBIC_CTL_EDI | SBIC_CTL_IDI |
                           /* | SBIC_CTL_HSP | */ 0));

retry_selection:
  /* select the SCSI bus (it's an error if bus isn't free) */
  if (issue_select (dev, regs, target, dev->sc_scsi_addr))
    return -1;
  if (wait_for_select (dev, regs))
    return -1;
  /*
   * Wait for a phase change (or error) then let the device
   * sequence us through the various SCSI phases.
   */
  dev->sc_stat[0] = 0xff;
  dev->sc_msg[0] = 0xff;
  phase = CMD_PHASE;
  while (1) 
    {
      wait = scsi_cmd_wait;

      GET_SBIC_csr (regs, csr);
      QPRINTF((">CSR:%02x<", csr));

      HIST(cxin_wait, wait);
      if ((csr != 0xff) && (csr & 0xf0) && (csr & 0x08)) /* requesting some new phase */
	phase = csr & PHASE;
      else if ((csr == SBIC_CSR_DISC) || (csr == SBIC_CSR_DISC_1)
	       || (csr == SBIC_CSR_S_XFERRED))
	{
	  dev->sc_flags &= ~SCSI_SELECTED;
	  GET_SBIC_cmd_phase (regs, phase);
	  if (phase == 0x60)
	    GET_SBIC_tlun (regs, dev->sc_stat[0]);
	  else
	    return -1;

	  goto out;
	}
      else 
	{
	  scsierror(dev, regs, csr);
	  goto abort;
	}

      switch (phase) 
	{
	case CMD_PHASE:
	  if (ixfer_start (regs, clen, phase, wait))
	    if (ixfer_out (regs, clen, cbuf, phase))
	      goto abort;
	  phase = xferphase;
	  break;

	case DATA_IN_PHASE:
	  if (len <= 0)
	    goto abort;
	  wait = scsi_data_wait;
	  if (ixfer_start (regs, len, phase, wait))
	    ixfer_in (regs, len, buf);
	  phase = STATUS_PHASE;
	  break;

	case MESG_IN_PHASE:
	  if (ixfer_start (regs, sizeof (dev->sc_msg), phase, wait))
	    {
	      dev->sc_msg[0] = 0xff;
	      ixfer_in (regs, sizeof (dev->sc_msg), dev->sc_msg);
	      /* get the command completion interrupt, or we can't send
		 a new command (LCI) */
	      SBIC_WAIT (regs, SBIC_ASR_INT, wait);
	      GET_SBIC_csr (regs, csr);
#ifdef DEBUG
	      if (sync_debug)
		printf ("msgin finished with csr 0x%x\n", csr);
#endif
	      /* test whether this is a reply to our sync request */
	      if (dev->sc_msg[0] == MSG_EXT_MESSAGE
		  && dev->sc_msg[1] == 3
		  && dev->sc_msg[2] == MSG_SYNC_REQ)
		{
		  dev->sc_sync[target].period = scsi_period_to_sbic (dev, regs, dev->sc_msg[3]);
		  dev->sc_sync[target].offset = dev->sc_msg[4];
		  dev->sc_sync[target].state = SYNC_DONE;
		  SET_SBIC_syn (regs, SBIC_SYN (dev->sc_sync[target].offset, 
						dev->sc_sync[target].period));
		  /* ACK the message */
		  SET_SBIC_cmd (regs, SBIC_CMD_CLR_ACK);
		  WAIT_CIP (regs);
		  phase = CMD_PHASE;  /* or whatever */
		  printf ("scsi%d: target %d now synchronous, period=%dns, offset=%d.\n",
			  dev->sc_ac->amiga_unit, target,
			  dev->sc_msg[3] * 4, dev->sc_msg[4]);
		}
	      else if (dev->sc_msg[0] == MSG_REJECT
		       && dev->sc_sync[target].state == SYNC_SENT)
		{
#ifdef DEBUG
		  if (sync_debug)
		    printf ("target %d rejected sync, going async\n", target);
#endif
		  dev->sc_sync[target].period = sbic_min_period;
		  dev->sc_sync[target].offset = 0;
		  dev->sc_sync[target].state = SYNC_DONE;
		  SET_SBIC_syn (regs, SBIC_SYN (dev->sc_sync[target].offset, 
						dev->sc_sync[target].period));
		  /* ACK the message */
		  SET_SBIC_cmd (regs, SBIC_CMD_CLR_ACK);
		  WAIT_CIP (regs);
		  phase = CMD_PHASE;  /* or whatever */
		}
	      else if (dev->sc_msg[0] == MSG_REJECT)
		{
		  /* coming to think of it, we'll never REJECt a REJECT
		     message.. :-) */
		  
		  /* ACK the message */
		  SET_SBIC_cmd (regs, SBIC_CMD_CLR_ACK);
		  WAIT_CIP (regs);
		  phase = CMD_PHASE;  /* or whatever */
		}
	      else if (dev->sc_msg[0] == MSG_CMD_COMPLETE)
		{
		  /* !! KLUDGE ALERT !!

		     quite a few drives don't seem to really like the current
		     way of sending the sync-handshake together with the
		     ident-message, and they react by sending command-complete
		     and disconnecting right after returning the valid sync
		     handshake. So, all I can do is reselect the drive, and
		     hope it won't disconnect again. I don't think this is valid
		     behavior, but I can't help fixing a problem that apparently
		     exists.

		     Note: we should not get here on `normal' command completion,
		     as that condition is handled by the high-level sel&xfer resume
		     command used to walk thru status/cc-phase. */

#ifdef DEBUG
		  if (sync_debug)
		    printf ("GOT CMD-COMPLETE! %d acting weird.. waiting for disconnect...\n", target);
#endif
		  /* ACK the message */
		  SET_SBIC_cmd (regs, SBIC_CMD_CLR_ACK);
		  WAIT_CIP (regs);

		  /* wait for disconnect */
		  while ((csr != SBIC_CSR_DISC) && (csr != SBIC_CSR_DISC_1))
		    {
		      DELAY (1);
		      GET_SBIC_csr (regs, csr);
		    }
#ifdef DEBUG
		  if (sync_debug)
		    printf ("ok.\nRetrying selection.\n");
#endif
		  dev->sc_flags &= ~SCSI_SELECTED;
		  goto retry_selection;
		}
	      else
		{
#ifdef DEBUG
		  if (scsi_debug || sync_debug)
		    printf ("Rejecting message 0x%02x\n", dev->sc_msg[0]);
#endif
		  /* prepare to reject the message, NACK */
		  SET_SBIC_cmd (regs, SBIC_CMD_SET_ATN);
		  WAIT_CIP (regs);
		  SET_SBIC_cmd (regs, SBIC_CMD_CLR_ACK);
		  WAIT_CIP (regs);
		  phase = MESG_OUT_PHASE;
		}
	    }
	  break;

	case MESG_OUT_PHASE:
#ifdef DEBUG
	  if (sync_debug)
	    printf ("Sending REJECT message to last received message.\n");
#endif
	  /* should only get here on reject, since it's always US that
	     initiate a sync transfer */
	  SEND_BYTE (regs, MSG_REJECT);
	  phase = STATUS_PHASE;
	  break;

	case DATA_OUT_PHASE:
	  if (len <= 0)
	    goto abort;
	  wait = scsi_data_wait;
	  if (ixfer_start (regs, len, phase, wait)) 
	    {
	      if (ixfer_out (regs, len, buf, phase))
		goto abort;
	    }
	  phase = STATUS_PHASE;
	  break;

	case STATUS_PHASE:
	  /* the sbic does the status/cmd-complete reading ok, so do this
	     with its hi-level commands. */
	  SBIC_TC_PUT (regs, 0);
	  SET_SBIC_cmd_phase (regs, 0x46);
	  SET_SBIC_cmd (regs, SBIC_CMD_SEL_ATN_XFER);
	  phase = BUS_FREE_PHASE;
	  break;

	case BUS_FREE_PHASE:
	  goto out;

	default:
	  printf("scsi: unexpected phase %d in icmd from %d\n",
		 phase, target);
	  goto abort;
	}

      /* make sure the last command was taken, ie. we're not hunting after
         an ignored command.. */
      GET_SBIC_asr (regs, asr);
      if (asr & SBIC_ASR_LCI)
        goto abort;

      /* tapes may take a loooong time.. */
      while (asr & SBIC_ASR_BSY)
        {
          DELAY(1);
          GET_SBIC_asr (regs, asr);
        }

#if 0
      if (wait <= 0)            
	goto abort;
#endif

      /* wait for last command to complete */
      SBIC_WAIT (regs, SBIC_ASR_INT, wait);
    }

abort:
  scsiabort(dev, regs, "icmd");
out:
  return (dev->sc_stat[0]);
}

/*
 * Finish SCSI xfer command:  After the completion interrupt from
 * a read/write operation, sequence through the final phases in
 * programmed i/o.  This routine is a lot like scsiicmd except we
 * skip (and don't allow) the select, cmd out and data in/out phases.
 */
static void
finishxfer(dev, regs, target)
	struct scsi_softc *dev;
	volatile register sbic_padded_regmap_t *regs;
	int target;
{
  u_char phase, csr;
  int s;

  QPRINTF(("{"));
  s = splbio();
  /* have the sbic complete on its own */
  SBIC_TC_PUT (regs, 0);
  SET_SBIC_cmd_phase (regs, 0x46);
  SET_SBIC_cmd (regs, SBIC_CMD_SEL_ATN_XFER);

  do
    {
      SBIC_WAIT (regs, SBIC_ASR_INT, 0);
      GET_SBIC_csr (regs, csr);
      QPRINTF(("%02x:", csr));
    }
  while ((csr != SBIC_CSR_DISC) && (csr != SBIC_CSR_DISC_1)
         && (csr != SBIC_CSR_S_XFERRED));

  dev->sc_flags &= ~SCSI_SELECTED;
  GET_SBIC_cmd_phase (regs, phase);
  QPRINTF(("}%02x\n", phase));
  if (phase == 0x60)
    GET_SBIC_tlun (regs, dev->sc_stat[0]);
  else
    scsierror (dev, regs, csr);

  splx (s);
}

int
scsi_test_unit_rdy(ctlr, slave, unit)
	int ctlr, slave, unit;
{
	register struct scsi_softc *dev = &scsi_softc[ctlr];
	static struct scsi_cdb6 cdb = { CMD_TEST_UNIT_READY };

	cdb.lun = unit;
	return (scsiicmd(dev, slave, (u_char *)&cdb, sizeof(cdb), (u_char *)0, 0,
			 STATUS_PHASE));
}

int
scsi_start_stop_unit (ctlr, slave, unit, start)
	int ctlr, slave, unit;
{
	register struct scsi_softc *dev = &scsi_softc[ctlr];
	static struct scsi_cdb6 cdb = { CMD_LOADUNLOAD };

	cdb.lun = unit;
	/* we don't set the immediate bit, so we wait for the 
	   command to succeed.
	   We also don't touch the LoEj bit, which is primarily meant
	   for floppies. */
	cdb.len = start & 0x01;
	return (scsiicmd(dev, slave, (u_char *)&cdb, sizeof(cdb), (u_char *)0, 0,
			 STATUS_PHASE));
}


int
scsi_request_sense(ctlr, slave, unit, buf, len)
	int ctlr, slave, unit;
	u_char *buf;
	unsigned len;
{
	register struct scsi_softc *dev = &scsi_softc[ctlr];
	static struct scsi_cdb6 cdb = { CMD_REQUEST_SENSE };

	cdb.lun = unit;
	cdb.len = len;
	return (scsiicmd(dev, slave, (u_char *)&cdb, sizeof(cdb), buf, len, DATA_IN_PHASE));
}

int
scsi_immed_command(ctlr, slave, unit, cdb, buf, len, rd)
	int ctlr, slave, unit;
	struct scsi_fmt_cdb *cdb;
	u_char *buf;
	unsigned len;
{
	register struct scsi_softc *dev = &scsi_softc[ctlr];

	cdb->cdb[1] |= (unit << 5);
	return (scsiicmd(dev, slave, (u_char *) cdb->cdb, cdb->len, buf, len,
			 rd != 0? DATA_IN_PHASE : DATA_OUT_PHASE));
}

/*
 * The following routines are test-and-transfer i/o versions of read/write
 * for things like reading disk labels and writing core dumps.  The
 * routine scsigo should be used for normal data transfers, NOT these
 * routines.
 */
int
scsi_tt_read(ctlr, slave, unit, buf, len, blk, bshift)
	int ctlr, slave, unit;
	u_char *buf;
	u_int len;
	daddr_t blk;
	int bshift;
{
	register struct scsi_softc *dev = &scsi_softc[ctlr];
	struct scsi_cdb10 cdb;
	int stat;
	int old_wait = scsi_data_wait;

	scsi_data_wait = 300000;
	bzero(&cdb, sizeof(cdb));
	cdb.cmd = CMD_READ_EXT;
	cdb.lun = unit;
	blk >>= bshift;
	cdb.lbah = blk >> 24;
	cdb.lbahm = blk >> 16;
	cdb.lbalm = blk >> 8;
	cdb.lbal = blk;
	cdb.lenh = len >> (8 + DEV_BSHIFT + bshift);
	cdb.lenl = len >> (DEV_BSHIFT + bshift);
	stat = scsiicmd(dev, slave, (u_char *) &cdb, sizeof(cdb), buf, len, DATA_IN_PHASE);
	scsi_data_wait = old_wait;
	return (stat);
}

int
scsi_tt_write(ctlr, slave, unit, buf, len, blk, bshift)
	int ctlr, slave, unit;
	u_char *buf;
	u_int len;
	daddr_t blk;
	int bshift;
{
	register struct scsi_softc *dev = &scsi_softc[ctlr];
	struct scsi_cdb10 cdb;
	int stat;
	int old_wait = scsi_data_wait;

	scsi_data_wait = 300000;

	bzero(&cdb, sizeof(cdb));
	cdb.cmd = CMD_WRITE_EXT;
	cdb.lun = unit;
	blk >>= bshift;
	cdb.lbah = blk >> 24;
	cdb.lbahm = blk >> 16;
	cdb.lbalm = blk >> 8;
	cdb.lbal = blk;
	cdb.lenh = len >> (8 + DEV_BSHIFT + bshift);
	cdb.lenl = len >> (DEV_BSHIFT + bshift);
	stat = scsiicmd(dev, slave, (u_char *) &cdb, sizeof(cdb), buf, len, DATA_OUT_PHASE);
	scsi_data_wait = old_wait;
	return (stat);
}

int
scsireq(dq)
	register struct devqueue *dq;
{
	register struct devqueue *hq;

	hq = &scsi_softc[dq->dq_ctlr].sc_sq;
	insque(dq, hq->dq_back);
	if (dq->dq_back == hq)
		return(1);
	return(0);
}

int
scsiustart (int unit)
{
	register struct scsi_softc *dev = &scsi_softc[unit];

	if (dev->dmareq(&dev->sc_dq))
		return(1);
	return(0);
}

void
scsistart (int unit)
{
	register struct devqueue *dq;
	
	dq = scsi_softc[unit].sc_sq.dq_forw;
	(dq->dq_driver->d_go)(dq->dq_unit);
}

int
scsigo(ctlr, slave, unit, bp, cdb, pad)
	int ctlr, slave, unit;
	struct buf *bp;
	struct scsi_fmt_cdb *cdb;
	int pad;
{
  register struct scsi_softc *dev = &scsi_softc[ctlr];
  volatile register sbic_padded_regmap_t *regs =
			(sbic_padded_regmap_t *)dev->sc_ac->amiga_addr;
  int i, dmaflags;
  u_char phase, csr, asr, cmd;

  cdb->cdb[1] |= unit << 5;

  /* this should only happen on character devices, and there only if user
     programs have not been written taking care of not passing odd aligned
     buffers. dd was a bad example of this sin.. */

/* XXXX do all with polled I/O */

  if (scsi_no_dma || (((int)bp->b_un.b_addr & 3) || (bp->b_bcount & 1)))
    {
      register struct devqueue *dq;

      dev->dmafree(&dev->sc_dq);

      /* in this case do the transfer with programmed I/O :-( This is
         probably still faster than doing the transfer with DMA into a
         buffer and copying it later to its final destination, comments? */
      scsiicmd (dev, slave, (u_char *) cdb->cdb, cdb->len, 
		bp->b_un.b_addr, bp->b_bcount,
		bp->b_flags & B_READ ? DATA_IN_PHASE : DATA_OUT_PHASE);

      dq = dev->sc_sq.dq_forw;
      dev->sc_flags &=~ SCSI_IO;
      (dq->dq_driver->d_intr)(dq->dq_unit, dev->sc_stat[0]);
      return dev->sc_stat[0];
    }

  /* set the sbic into DMA mode */
  SET_SBIC_control (regs, (/*SBIC_CTL_HHP |*/ SBIC_CTL_EDI | SBIC_CTL_IDI |
                           /* | SBIC_CTL_HSP | */ SBIC_MACHINE_DMA_MODE));

  /* select the SCSI bus (it's an error if bus isn't free) */
  if (issue_select(dev, regs, slave, dev->sc_scsi_addr) || wait_for_select(dev, regs)) 
    {
      dev->dmafree(&dev->sc_dq);
      return (1);
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
  while (1) 
    {
      register int wait = scsi_cmd_wait;
      register struct devqueue *dq;

      switch (phase) 
	{
	case CMD_PHASE:
	  if (ixfer_start(regs, cdb->len, phase, wait))
	    if (ixfer_out(regs, cdb->len, cdb->cdb, phase))
	      goto abort;
	  break;

	case MESG_IN_PHASE:
	  if (ixfer_start (regs, sizeof (dev->sc_msg), phase, wait))
	    {
	      ixfer_in (regs, sizeof (dev->sc_msg), dev->sc_msg);
	      /* prepare to reject any mesgin, no matter what it might be.. */
	      SET_SBIC_cmd (regs, SBIC_CMD_SET_ATN);
	      WAIT_CIP (regs);
	      SET_SBIC_cmd (regs, SBIC_CMD_CLR_ACK);
	      phase = MESG_OUT_PHASE;
	    }
	  break;

	case MESG_OUT_PHASE:
	  SEND_BYTE (regs, MSG_REJECT);
	  phase = STATUS_PHASE;
	  break;

	case DATA_IN_PHASE:
	case DATA_OUT_PHASE:
	  goto out;

	/* status phase can happen, if the issued read/write command is
	   illegal (for example, reading after EOT on tape) and the device
	   doesn't even go to data in/out phase. So handle this here
	   normally, instead of going thru abort-handling. */
	case STATUS_PHASE:
          dev->dmafree(&dev->sc_dq);
	  finishxfer (dev, regs, slave);
          dq = dev->sc_sq.dq_forw;
	  dev->sc_flags &=~ SCSI_IO;
          (dq->dq_driver->d_intr)(dq->dq_unit, dev->sc_stat[0]);
          return 0;

	default:
	  printf("scsi: unexpected phase %d in go from %d\n",
		 phase, slave);
	  goto abort;
	}

      /* make sure the last command was taken, ie. we're not hunting after
         an ignored command.. */
      GET_SBIC_asr (regs, asr);
      if (asr & SBIC_ASR_LCI)
        goto abort;

      /* tapes may take a loooong time.. */
      while (asr & SBIC_ASR_BSY)
        {
          DELAY(1);
          GET_SBIC_asr (regs, asr);
        }

      if (wait <= 0)            
	goto abort;

      /* wait for last command to complete */
      SBIC_WAIT (regs, SBIC_ASR_INT, wait);

      GET_SBIC_csr (regs, csr);
      QPRINTF((">CSR:%02x<", csr));

      HIST(sgo_wait, wait);
      if ((csr != 0xff) && (csr & 0xf0) && (csr & 0x08))	/* requesting some new phase */
	phase = csr & PHASE;
      else 
	{
	  scsierror(dev, regs, csr);
	  goto abort;
	}
    }

out:
  dmaflags = DMAGO_NOINT;
  if (bp->b_flags & B_READ)
    dmaflags |= DMAGO_READ;
  if ((int)bp->b_un.b_addr & 3)
    panic ("not long-aligned buffer address in scsi_go");
  if (bp->b_bcount & 1)
    panic ("odd transfer count in scsi_go");

  /* dmago() also enables interrupts for the sbic */
  i = dev->dmago(ctlr, bp->b_un.b_addr, bp->b_bcount, dmaflags);

  SBIC_TC_PUT (regs, (unsigned)i);
  SET_SBIC_cmd (regs, SBIC_CMD_XFER_INFO);

  return (0);

abort:
  scsiabort(dev, regs, "go");
  dev->dmafree(&dev->sc_dq);
  return (1);
}

void
scsidone (int unit)
{
  volatile register sbic_padded_regmap_t *regs =
			(sbic_padded_regmap_t *)scsi_softc[unit].sc_ac->amiga_addr;

#ifdef DEBUG
  if (scsi_debug)
    printf("scsi%d: done called!\n", unit);
#endif
}

int
scsiintr (int unit)
{
  register struct scsi_softc *dev = &scsi_softc[unit];
  volatile register sbic_padded_regmap_t *regs =
				(sbic_padded_regmap_t *)dev->sc_ac->amiga_addr;
  register u_char asr, csr, phase;
  register struct devqueue *dq;
  int i;

  GET_SBIC_asr (regs, asr);
  if (! (asr & SBIC_ASR_INT))
    return 0;

  GET_SBIC_csr (regs, csr);
QPRINTF(("[0x%x]", csr));

  if (csr == (SBIC_CSR_XFERRED|STATUS_PHASE)
      || csr == (SBIC_CSR_MIS|STATUS_PHASE)
      || csr == (SBIC_CSR_MIS_1|STATUS_PHASE)
      || csr == (SBIC_CSR_MIS_2|STATUS_PHASE))
    {
      /*
       * this should be the normal i/o completion case.
       * get the status & cmd complete msg then let the
       * device driver look at what happened.
       */
      dq = dev->sc_sq.dq_forw;
      finishxfer(dev, regs, dq->dq_slave);
      dev->sc_flags &=~ SCSI_IO;
      dev->dmafree (&dev->sc_dq);
      (dq->dq_driver->d_intr)(dq->dq_unit, dev->sc_stat[0]);
    } 
  else if (csr == (SBIC_CSR_XFERRED|DATA_OUT_PHASE) || csr == (SBIC_CSR_XFERRED|DATA_IN_PHASE)
  	   || csr == (SBIC_CSR_MIS|DATA_OUT_PHASE) || csr == (SBIC_CSR_MIS|DATA_IN_PHASE)
  	   || csr == (SBIC_CSR_MIS_1|DATA_OUT_PHASE) || csr == (SBIC_CSR_MIS_1|DATA_IN_PHASE)
  	   || csr == (SBIC_CSR_MIS_2|DATA_OUT_PHASE) || csr == (SBIC_CSR_MIS_2|DATA_IN_PHASE))
    {
      /* do scatter-gather dma hacking the controller chip, ouch.. */
      i = dev->dmanext (unit);
      SBIC_TC_PUT (regs, (unsigned)i);
      SET_SBIC_cmd (regs, SBIC_CMD_XFER_INFO);
    }
  else 
    {
      /* Something unexpected happened -- deal with it. */
      dev->dmastop (unit);
      scsierror(dev, regs, csr);
      scsiabort(dev, regs, "intr");
      if (dev->sc_flags & SCSI_IO) 
	{
	  dev->sc_flags &=~ SCSI_IO;
	  dev->dmafree (&dev->sc_dq);
	  dq = dev->sc_sq.dq_forw;
	  (dq->dq_driver->d_intr)(dq->dq_unit, -1);
	}
    }

  return 1;
}

void
scsifree(dq)
	register struct devqueue *dq;
{
	register struct devqueue *hq;

	hq = &scsi_softc[dq->dq_ctlr].sc_sq;
	remque(dq);
	if ((dq = hq->dq_forw) != hq)
		(dq->dq_driver->d_start)(dq->dq_unit);
}

/*
 * (XXX) The following routine is needed for the SCSI tape driver
 * to read odd-size records.
 */

#include "st.h"
#if NST > 0
int
scsi_tt_oddio(ctlr, slave, unit, buf, len, b_flags, freedma)
	int ctlr, slave, unit, b_flags;
	u_char *buf;
	u_int len;
{
	register struct scsi_softc *dev = &scsi_softc[ctlr];
	struct scsi_cdb6 cdb;
	u_char iphase;
	int stat;

	/*
	 * First free any DMA channel that was allocated.
	 * We can't use DMA to do this transfer.
	 */
	if (freedma)
		dev->dmafree(&dev->sc_dq);
	/*
	 * Initialize command block
	 */
	bzero(&cdb, sizeof(cdb));
	cdb.lun  = unit;
	cdb.lbam = (len >> 16) & 0xff;
	cdb.lbal = (len >> 8) & 0xff;
	cdb.len = len & 0xff;
	if (buf == 0) {
		cdb.cmd = CMD_SPACE;
		cdb.lun |= 0x00;
		len = 0;
		iphase = MESG_IN_PHASE;
	} else if (b_flags & B_READ) {
		cdb.cmd = CMD_READ;
		iphase = DATA_IN_PHASE;
	} else {
		cdb.cmd = CMD_WRITE;
		iphase = DATA_OUT_PHASE;
	}
	/*
	 * Perform command (with very long delays)
	 */
	scsi_delay(30000000);
	stat = scsiicmd(dev, slave, (u_char *) &cdb, sizeof(cdb), buf, len, iphase);
	scsi_delay(0);
	return (stat);
}
#endif
#endif
