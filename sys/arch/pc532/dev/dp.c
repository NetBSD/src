/* Written by Phil Nelson for the pc532.  Used source with the following
 * copyrights as a model. 
 *
 *	dp.c:  A NCR DP8490 driver for the pc532.
 *
 *	$Id: dp.c,v 1.2 1993/10/01 22:59:31 phil Exp $
 */

/*
 * (Mostly) Written by Julian Elischer (julian@tfs.com)
 * for TRW Financial Systems for use under the MACH(2.5) operating system.
 *
 * TRW Financial Systems, in accordance with their agreement with Carnegie
 * Mellon University, makes this software available to CMU to distribute
 * or use in any manner that they see fit as long as this message is kept with
 * the software. For this reason TFS also grants any other persons or
 * organisations permission to use or modify this software.
 *
 * TFS supplies this software to be publicly redistributed
 * on the understanding that TFS is not responsible for the correct
 * functioning of this software in any circumstances.
 *
 */

/*
 * a FEW lines in this driver come from a MACH adaptec-disk driver
 * so the copyright below is included:
 *
 * Copyright 1990 by Open Software Foundation,
 * Grenoble, FRANCE
 *
 * 		All Rights Reserved
 * 
 *   Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appears in all copies and
 * that both the copyright notice and this permission notice appear in
 * supporting documentation, and that the name of OSF or Open Software
 * Foundation not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission.
 * 
 *   OSF DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS,
 * IN NO EVENT SHALL OSF BE LIABLE FOR ANY SPECIAL, INDIRECT, OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT,
 * NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "sys/types.h"
#include "sys/param.h"
#include "sys/systm.h"
#include "sys/errno.h"
#include "sys/ioctl.h"
#include "sys/buf.h"
#include "machine/stdarg.h"
#include "sys/proc.h"
#include "sys/user.h"
#include "sys/dkbad.h"
#include "sys/disklabel.h"
#include "scsi/scsi_all.h"
#include "scsi/scsiconf.h"

#include "machine/frame.h"
#include "device.h"
#include "dpreg.h"

#include "../pc532/icu.h"

/* Some constants (may need to be changed!) */
#define DP_NSEG		16

int dpprobe(struct pc532_device *);
int dpattach(struct pc532_device *);
int dp_scsi_cmd(struct scsi_xfer *);
void dpminphys(struct buf *);
long int dp_adapter_info(int);
void dp_intr(void);

struct scsidevs *
scsi_probe(int masunit, struct scsi_switch *sw, int physid, int type, int want);

struct	pc532_driver dpdriver = {
	dpprobe, dpattach, "dp",
};

struct scsi_switch dp_switch = {
	"dp",
	dp_scsi_cmd,
	dpminphys,
	0,
	0,
	dp_adapter_info,
	0, 0, 0
};

/* Sense command. */
u_char sense_cmd[] = { 3, 0, 0, 0, sizeof(struct scsi_sense_data), 0};

/* Do we need to initialize. */
int	dp_needs_init = 1;

/* Do we need a reset . */
int	dp_needs_reset = 1;

/* SCSI phase we are currently in . . . */
int	dp_scsi_phase;

/* SCSI Driver state */
int	dp_dvr_state = DP_DVR_READY;

/* For polled error reporting. */
int	dp_intr_retval;

/* For counting the retries. */
int	dp_try_count;

/* Give the interrupt routine access to the current scsi_xfer info block. */
struct scsi_xfer *cur_xs = NULL;

/* Initial probe for a device.  If it is using the dp controller,
   just say yes so that attach can be the one to find the real drive. */

int dpprobe(struct pc532_device *dvp)
{
  /* If we call this, we need to add SPL_DP to the bio mask! */
  PL_bio |= SPL_DP;
  PL_zero |= PL_bio;

  if (dp_needs_init)
     dp_initialize();

  if (dp_needs_reset)
     dp_reset();

  /* All pc532s should have one, so we don't check ! :) */
  return (1);	
}


int dpattach(struct pc532_device *dvp)
{
	int r;

	r = scsi_attach(0, 7, &dp_switch,
		&dvp->pd_drive, &dvp->pd_unit, dvp->pd_flags);

	return(r);
}

void dpminphys(struct buf *bp)
{
	if(bp->b_bcount > ((DP_NSEG - 1) * NBPG))
		bp->b_bcount = ((DP_NSEG - 1) * NBPG);
}

long int dp_adapter_info(int unit)
{
	return (1);    /* only 1 outstanding request. */
}


/* Do a scsi command. */

int dp_scsi_cmd(struct scsi_xfer *xs)
{
	struct	iovec	 *iovp;
	int	flags;
	int	retval;		/* Return values from functions. */
	int	x;		/* for splbio()  & splhigh() */
        int	dvr_state;
	int	ti_val;		/* For timeouts. */

	x = splhigh();
	dvr_state = dp_dvr_state;
	if (dvr_state == DP_DVR_READY)
	  dp_dvr_state = DP_DVR_STARTED;
	splx(x);

	if (dvr_state != DP_DVR_STARTED)
	  return (TRY_AGAIN_LATER);

	cur_xs = xs;

	/* Some initial checks. */
	flags = xs->flags;
	if (!(flags & INUSE)) {
		printf("dp xs not in use!");
		xs->flags |= INUSE;
	}
	if (flags & ITSDONE) {
		printf("dp xs already done!");
		xs->flags &= ~ITSDONE;
	}

	if (dp_needs_reset)
		dp_reset();

/* info ... */
printf ("flags=0x%x, targ=%d, lu=%d, cmdlen=%d, cmd=%x\n", xs->flags,
xs->targ, xs->lu, xs->cmdlen, xs->cmd->opcode);

	retval = dp_start_cmd (xs);

	if (retval == SUCCESSFULLY_QUEUED && (xs->flags & SCSI_NOMASK)) {
		/* No interrupts available! */
		ti_val = WAIT_MUL * xs->timeout;
		while (dp_dvr_state != DP_DVR_READY) {
			if (RD_ADR(u_char, DP_STAT2) & DP_S_IRQ) {
				dp_intr();
				ti_val = WAIT_MUL * xs->timeout;
			}
			if (--ti_val == 0) {
				/* Software Timeout! */
printf ("scsi timeout! stat1=0x%x stat2=0x%x\n",
RD_ADR(u_char,DP_STAT1), RD_ADR(u_char,DP_STAT2));
				dp_reset();
				xs->error = XS_SWTIMEOUT;
				dp_dvr_state = DP_DVR_READY;
				return HAD_ERROR;
			}
		}		
		retval = dp_intr_retval;
	}

	return (retval);
}


/*===========================================================================*
 *				dp_intr					     * 
 *===========================================================================*/

/* This is where a lot of the work happens!  This is called in non-interrupt
   mode when an interrupt would have happened.  It is also the real interrupt
   routine.  It uses dp_dvr_state to determine the next actions along with
   cur_xs->flags.  */

void dp_intr (void)
{
    u_char isr;
    u_char new_phase;
    u_char status;
    u_char message;
    int ret;

    scsi_select_ctlr (DP8490);
    WR_ADR(u_char, DP_EMR_ISR, DP_EF_ISR_NEXT);
    isr = RD_ADR (u_char, DP_EMR_ISR);
    dp_clear_isr();


	if (isr & DP_ISR_BSYERR) {
		printf ("Busy error?\n");
	}

        if (!(isr & DP_ISR_APHS)) {
	 	printf ("Not an APHS!\n");
		return;
	}

	new_phase = (RD_ADR(u_char, DP_STAT1) >> 2) & 7;

printf ("dp_intr isr=0x%x new_phase = 0x%x\n", isr, new_phase);

	switch (dp_dvr_state) {

	case DP_DVR_ARB: 	/* Next comes the command phase! */
		if (new_phase != DP_PHASE_CMD) {
printf ("Phase mismatch cmd!\n");
		}
		dp_dvr_state = DP_DVR_CMD;
		ret = dp_pdma_out (cur_xs->cmd, cur_xs->cmdlen, DP_PHASE_CMD);
		dp_clear_isr();
		break;
		
	case DP_DVR_CMD:	/* Next comes the data i/o phase if needed. */
		if (cur_xs->flags & SCSI_DATA_UIO) {
		   /* UIO work. */
		   panic ("scsi uio");
		} else if (cur_xs->flags & SCSI_DATA_IN) {
		    if (new_phase != DP_PHASE_DATAI) {
printf ("Phase mismatch in. \n");
		    }
		    dp_dvr_state = DP_DVR_DATA;
		    ret = dp_pdma_in (cur_xs->data, cur_xs->datalen,
		    			 DP_PHASE_DATAI);
		    dp_clear_isr();
		    break;
		}  else if (cur_xs->flags & SCSI_DATA_OUT) {
		    if (new_phase != DP_PHASE_DATAO) {
printf ("Phase mismatch out. \n");
		    }
		    dp_dvr_state = DP_DVR_DATA;
		    ret = dp_pdma_out (cur_xs->data, cur_xs->datalen,
		    			 DP_PHASE_DATAO);
		    dp_clear_isr();
		    break;
		}
		/* Fall through to next phase. */
	case DP_DVR_DATA:	/* Next comes the stat phase */
		if (new_phase != DP_PHASE_STATUS) {
printf ("Phase mismatch stat. \n");
		}
		dp_dvr_state = DP_DVR_STAT;
		dp_pdma_in (&status, 1, DP_PHASE_STATUS);
		dp_clear_isr();
		while (!(RD_ADR(u_char, DP_STAT2) & DP_S_IRQ)) /* wait */;
		new_phase = (RD_ADR(u_char, DP_STAT1) >> 2) & 7;
		if (new_phase != DP_PHASE_MSGI) {
			printf ("msgi phase mismatch\n");
		}
		dp_pdma_in (&message, 1, DP_PHASE_MSGI);
		dp_clear_isr();
		while (!(RD_ADR(u_char, DP_STAT2) & DP_S_IRQ)) /* wait */;
		dp_clear_isr();
printf ("status = 0x%x, message = 0x%x\n", status, message);
		if (status != SCSI_OK && dp_try_count < cur_xs->retries) {
printf ("dp_try_count = %d\n", dp_try_count);
		    dp_restart_cmd();
		}
		break;

	default:
		/* TEMP error generation!!! */
		dp_reset();
		dp_dvr_state = DP_DVR_READY;
		cur_xs->error = XS_DRIVER_STUFFUP;
		dp_intr_retval = HAD_ERROR;	
		/* If this true interrupt code, call the done routine. */
		if (cur_xs->when_done) {
		  (*(cur_xs->when_done))(cur_xs->done_arg, cur_xs->done_arg2);
		}
	}

    if (dp_dvr_state == DP_DVR_STAT) {
printf ("dvr_stat: dp_try_count = %d\n", dp_try_count);
	WR_ADR (u_char, DP_MODE, 0);	/* Turn off monbsy, dma, ... */
	if (status == SCSI_OK) {
	  cur_xs->error = XS_NOERROR;
	  dp_intr_retval = COMPLETE;
	} else if (status & SCSI_BUSY) {
	  cur_xs->error = XS_BUSY;
	  dp_intr_retval = HAD_ERROR;
	} else if (status & SCSI_CHECK) {
	  /* Do a sense command. */
	  cur_xs->error = XS_SENSE;
	  dp_intr_retval = HAD_ERROR;
	  dp_get_sense (cur_xs);
	}
	cur_xs->flags |= ITSDONE;
	dp_dvr_state = DP_DVR_READY;
	/* If this true interrupt code, call the done routine. */
	if (cur_xs->when_done) {
	  (*(cur_xs->when_done))(cur_xs->done_arg, cur_xs->done_arg2);
	}
    }
printf ("exit dp_intr.\n");
}


/*===========================================================================*
 *				dp_initialize				     *
 *===========================================================================*/

dp_initialize()
{
	scsi_select_ctlr (DP8490);
	WR_ADR (u_char, DP_ICMD, DP_EMODE);	/* Set Enhanced mode */
	WR_ADR (u_char, DP_MODE, 0);		/* Disable everything. */
	WR_ADR (u_char, DP_EMR_ISR, DP_EF_RESETIP);
	WR_ADR (u_char, DP_EMR_ISR, DP_EF_NOP);
	WR_ADR (u_char, DP_STAT2, 0x80); 	/* scsi adr 7. */
	dp_scsi_phase = DP_PHASE_NONE;
}


/*===========================================================================*
 *				dp_reset				     * 
 *===========================================================================*/

/*
 * Reset dp SCSI bus.
 */

dp_reset()
{
  volatile int i;
  int x = splbio();

  scsi_select_ctlr (DP8490);
  WR_ADR (u_char, DP_MODE, 0);			/* get into harmless state */
  WR_ADR (u_char, DP_OUTDATA, 0);
  WR_ADR (u_char, DP_ICMD, DP_A_RST|DP_EMODE);	/* assert RST on SCSI bus */
  for (i = 55; i; --i);				/* wait 25 usec */
  WR_ADR (u_char, DP_ICMD, DP_EMODE);		/* deassert RST, get off bus */
  WR_ADR (u_char, DP_EMR_ISR, DP_EF_ISR_NEXT | DP_EMR_APHS);
  WR_ADR (u_char, DP_EMR_ISR, DP_INT_MASK);	/* set interrupt mask */
  splx(x);
  for (i = 800000; i; --i);			/* wait 360 msec */
  dp_needs_reset = 0;
}

/*===========================================================================*
 *				dp_wait_bus_free			     * 
 *===========================================================================*/
/* Wait for the SCSI bus to become free.  Currently polled because I am
 * assuming a single initiator configuration -- so this code would not be
 * running if the bus were busy.
 */
int
dp_wait_bus_free()
{
  int i;
  volatile int j;

  i = WAIT_MUL * 2000;
  while (i--) {
    /* Must be clear for 2 usec, so read twice */
    if (RD_ADR (u_char, DP_STAT1) & (DP_S_BSY | DP_S_SEL)) continue;
    for (j = 5; j; j--);
    if (RD_ADR (u_char, DP_STAT1) & (DP_S_BSY | DP_S_SEL)) continue;
    return OK;
  }
  dp_needs_reset = 1;
  return NOT_OK;
}

/*===========================================================================*
 *				dp_select				     * 
 *===========================================================================*/
/* Select SCSI device, set up for command transfer.
 */
int
dp_select(adr)
long adr;
{
  int i, stat1;

  WR_ADR (u_char, DP_OUTDATA, adr);	/* SCSI bus address */
  WR_ADR (u_char, DP_ICMD, DP_A_SEL | DP_ENABLE_DB | DP_EMODE);
  WR_ADR (u_char, DP_STAT2, 0x80); 	/* scsi adr 7. */
  for (i = 0;; ++i) {			/* wait for target to assert SEL */
    stat1 = RD_ADR (u_char, DP_STAT1);
    if (stat1 & DP_S_BSY) break;	/* select successful */
    if (i > WAIT_MUL * 2000) {			/* timeout */
      printf ("SCSI: SELECT timeout adr %d\n", adr);
      dp_reset();
      return NOT_OK;
    }
  }
  WR_ADR (u_char, DP_ICMD, DP_EMODE);	/* clear SEL, disable data out */
  WR_ADR (u_char, DP_OUTDATA, 0);
  dp_clear_isr();
  WR_ADR (u_char, DP_TCMD, 4);		/* bogus phase, guarantee mismatch */
  WR_ADR (u_char, DP_MODE, DP_M_BSY | DP_M_DMA);
  return OK;
}

/*===========================================================================*
 *				scsi_select_ctlr
 *===========================================================================*/
/* Select a SCSI device.
 */
scsi_select_ctlr (ctlr)
int ctlr;
{
  /* May need other stuff here to syncronize between dp & aic. */

  RD_ADR (u_char, ICU_IO) &= ~ICU_SCSI_BIT;	/* i/o, not port */
  RD_ADR (u_char, ICU_DIR) &= ~ICU_SCSI_BIT;	/* output */
  if (ctlr == DP8490)
    RD_ADR (u_char, ICU_DATA) &= ~ICU_SCSI_BIT;	/* select = 0 for 8490 */
  else
    RD_ADR (u_char, ICU_DATA) |= ICU_SCSI_BIT;	/* select = 1 for AIC6250 */
}

/*===========================================================================*
 *				dp_start_cmd				     *
 *===========================================================================*/

int dp_start_cmd(struct scsi_xfer *xs)
{
#if 0
  WR_ADR (u_char, DP_OUTDATA, 1 << xs->targ);	/* SCSI bus address */
  WR_ADR (u_char, DP_EMR_ISR, DP_EF_ARB);
  dp_dvr_state = DP_DVR_ARB;
#else

  /* This is not the "right" way to start it.  We should just have the
	chip do the select for us and interrupt at the end. */

  if (!dp_wait_bus_free()) {
	xs->error = XS_BUSY;
	return TRY_AGAIN_LATER;
  }

  if (!dp_select (1 << xs->targ)) {
	xs->error = XS_DRIVER_STUFFUP;
	return HAD_ERROR;
  }
#endif

  /* After selection, we now wait for the APHS interrupt! */
  dp_dvr_state = DP_DVR_ARB;	/* Just finished the select/arbitration */
  dp_try_count = 1;

  if (!(xs->flags & SCSI_NOMASK)) {
    /* Set up the timeout! */
    printf ("dp timeouts not done\n");
  }
  return SUCCESSFULLY_QUEUED;
}

/*===========================================================================*
 *				dp_restart_cmd				     *
 *===========================================================================*/

int dp_restart_cmd()
{
#if 0
  WR_ADR (u_char, DP_OUTDATA, xs->targ);	/* SCSI bus address */
  WR_ADR (u_char, DP_EMR_ISR, DP_EF_ARB);
  dp_dvr_state = DP_DVR_ARB;
#endif

  /* This is not the "right" way to start it.  We should just have the
	chip do the select for us and interrupt at the end. */

DELAY(50);
printf ("restart .. stat1=0x%x stat2=0x%x\n", RD_ADR(u_char, DP_STAT1),
RD_ADR(u_char, DP_STAT2));
  if (!dp_wait_bus_free()) {
	cur_xs->error = XS_BUSY;
	return;
  }

printf ("restart .1 stat1=0x%x stat2=0x%x\n", RD_ADR(u_char, DP_STAT1),
RD_ADR(u_char, DP_STAT2));
printf ("cur_xs->targ=%d\n",cur_xs->targ);
  if (!dp_select (1 << cur_xs->targ)) {
	cur_xs->error = XS_DRIVER_STUFFUP;
	return;
  }

printf ("restart .2 stat1=0x%x stat2=0x%x\n", RD_ADR(u_char, DP_STAT1),
RD_ADR(u_char, DP_STAT2));
  /* After selection, we now wait for the APHS interrupt! */
  dp_dvr_state = DP_DVR_ARB;	/* Just finished the select/arbitration */
  dp_try_count++;

  if (!(cur_xs->flags & SCSI_NOMASK)) {
    /* Set up the timeout! */
    printf ("dp timeouts not done\n");
  }
}

/*===========================================================================*
 *				dp_pdma_out				     *
 *===========================================================================*/

/* Note:  in NetBSD, the scsi dma addresses are set by the mapping hardware
   to inhibit cache.  There is therefore, no need to worry about cache hits
   during access to dma addresses. */

int dp_pdma_out(char *buf, int count, int phase)
{
  u_int stat2;

  /* Set it up. */
  WR_ADR(u_char, DP_TCMD, phase);
  RD_ADR(u_char, DP_MODE) |= DP_M_DMA;
  WR_ADR(u_char, DP_ICMD, DP_ENABLE_DB | DP_EMODE);
  WR_ADR(u_char, DP_START_SEND, 0);

  /* Do the pdma */
  while (count > sizeof(long)) {
    WR_ADR(long, DP_DMA, *(((long *)buf)++));
    count -= sizeof(long);
  }

  /* All but the last byte. */
  while (count-- > 1) {
    WR_ADR(u_char, DP_DMA, *(buf++));
  }

  while (1) {
    stat2 = RD_ADR(u_char, DP_STAT2);
    if (stat2 & (DP_S_IRQ | DP_S_DRQ)) break;
  }

  if (stat2 & DP_S_DRQ)
    WR_ADR(u_char, DP_DMA_EOP, *buf);
  else {
    /* dma error! */
printf ("dma write error!\n");
    cur_xs->error = XS_DRIVER_STUFFUP;
    /* Clear dma mode, just in case, and disable the bus. */
    RD_ADR (u_char, DP_MODE) &= ~DP_M_DMA;
    WR_ADR (u_char, DP_ICMD, DP_EMODE);
    return NOT_OK;
  }

  /* Clear dma mode, just in case, and disable the bus. */
  RD_ADR (u_char, DP_MODE) &= ~DP_M_DMA;
  WR_ADR (u_char, DP_ICMD, DP_EMODE);

  return OK;
}

/*===========================================================================*
 *				dp_pdma_in				     *
 *===========================================================================*/

/* Note:  in NetBSD, the scsi dma addresses are set by the mapping hardware
   to inhibit cache.  There is therefore, no need to worry about cache hits
   during access to dma addresses. */

int dp_pdma_in(char *buf, int count, int phase)
{
  u_char *dma_adr = (u_char *) DP_DMA;	/* Address for last few bytes. */

  /* Set it up. */
  WR_ADR(u_char, DP_TCMD, phase);
  RD_ADR(u_char, DP_MODE) |= DP_M_DMA;
  WR_ADR(u_char, DP_EMR_ISR, DP_EF_START_RCV | DP_EMR_APHS);

  /* Do the pdma */
  while (count >= sizeof(long)) {
    *(((long *)buf)++) = RD_ADR(long, DP_DMA);
    count -= sizeof(long);
  }

  while (count-- > 0) {
    *(buf++) = RD_ADR(u_char, (dma_adr++));
  }

  /* Clear dma mode, just in case, and disable the bus. */
  RD_ADR (u_char, DP_MODE) &= ~DP_M_DMA;
  WR_ADR (u_char, DP_ICMD, DP_EMODE);

  return OK;
}

/*===========================================================================*
 *				dp_get_sense				     *
 *===========================================================================*/

dp_get_sense (struct scsi_xfer *xs)
{
  u_char status;
  u_char message;
  int 	 ret;

printf ("sense 1.\n");
  if (!dp_wait_bus_free()) {
	xs->error = XS_BUSY;
	return;
  }

printf ("sense 2.\n");
  if (!dp_select (1 << xs->targ)) {
	xs->error = XS_DRIVER_STUFFUP;
	return;
  }

printf ("sense 3.\n");
  sense_cmd[1] = xs->lu << 5;
  ret = dp_pdma_out (sense_cmd, 6, DP_PHASE_CMD);
printf ("dp_pdma_out ret=%d\n", ret);
  dp_clear_isr();

printf ("sense 4. ");
printf ("stat1=0x%x stat2=0x%x\n", RD_ADR(u_char, DP_STAT1),
RD_ADR(u_char, DP_STAT2));
  while (!(RD_ADR(u_char, DP_STAT2) & DP_S_IRQ)) /* wait */;
  
printf ("sense 5.\n");
  dp_pdma_in ((u_char *)&xs->sense, sizeof(struct scsi_sense_data), DP_PHASE_DATAI);
  dp_clear_isr();

printf ("sense 6.\n");
  while (!(RD_ADR(u_char, DP_STAT2) & DP_S_IRQ)) ;

printf ("sense 7.\n");
  dp_pdma_in (&status, 1, DP_PHASE_STATUS);
  dp_clear_isr();

printf ("sense 8.\n");
  while (!(RD_ADR(u_char, DP_STAT2) & DP_S_IRQ)) /* wait */;
  dp_pdma_in (&message, 1, DP_PHASE_MSGI);
  dp_clear_isr();

printf ("sense status = 0x%x\n", status);

  if (status & SCSI_BUSY) {
	xs->error = XS_BUSY;
  }
  WR_ADR (u_char, DP_MODE, 0);	/* Turn off monbsy, dma, ... */
}
