/* Written by Phil Nelson for the pc532.  Used source with the following
 * copyrights as a model. 
 *
 *   dp.c:  A NCR DP8490 driver for the pc532.
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
 *	$Id: dp.c,v 1.1.1.1 1993/09/09 23:53:52 phil Exp $
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

/* Do we need to initialize. */
int	dp_needs_init = 1;

/* Do we need a reset . */
int	dp_needs_reset = 1;


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

struct scsi_xfer *cur_xs = NULL;

int dp_scsi_cmd(struct scsi_xfer *xs)
{
	struct	iovec	 *iovp;
	int	flags;

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

if (xs->cmd->opcode == 0) return (COMPLETE);

	xs->error = XS_DRIVER_STUFFUP;
	return (HAD_ERROR);
}


/*===========================================================================*
 *				dp_intr					     * 
 *===========================================================================*/

void dp_intr (struct intrframe frame)
{
	u_char stat1, stat2;

	scsi_select_ctlr (DP8490);
	stat1 = RD_ADR(u_char, DP_STAT1);
	stat2 = RD_ADR(u_char, DP_STAT2);
printf ("dp_intr stat1=0x%x, stat2=0x%x\n", stat1, stat2);
	dp_clear_isr();
}


/*===========================================================================*
 *				dp_initialize				     *
 *===========================================================================*/

dp_initialize()
{
	scsi_select_ctlr (DP8490);
	WR_ADR (u_char, DP_ICMD, 0);	/* Nothing happening. */
	WR_ADR (u_char, DP_MODE, 0);	/* Disable everything. */
	RD_ADR (u_char, DP_EMR_ISR);	/* Clear any pending interrupt. */	
	WR_ADR (u_char, DP_EMR_ISR, DP_EF_RESETIP);			\
	WR_ADR (u_char, DP_EMR_ISR, DP_EF_NOP);
	WR_ADR (u_char, DP_STAT2, 0x80); /* scsi adr 7. */
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
  int i = MAX_WAIT;
  volatile int j;

  while (i--) {
    /* Must be clear for 2 usec, so read twice */
    if (RD_ADR (u_char, DP_STAT1) & (DP_S_BSY | DP_S_SEL)) continue;
    for (j = 5; j; j--);
    if (RD_ADR (u_char, DP_STAT1) & (DP_S_BSY | DP_S_SEL)) continue;
    return OK;
  }
  dp_needs_reset = 0;
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
  for (i = 0;; ++i) {			/* wait for target to assert SEL */
    stat1 = RD_ADR (u_char, DP_STAT1);
    if (stat1 & DP_S_BSY) break;	/* select successful */
    if (i > MAX_WAIT) {			/* timeout */
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
  /* May need other stuff here to syncronize between processes.... */

  RD_ADR (u_char, ICU_IO) &= ~ICU_SCSI_BIT;	/* i/o, not port */
  RD_ADR (u_char, ICU_DIR) &= ~ICU_SCSI_BIT;	/* output */
  if (ctlr == DP8490)
    RD_ADR (u_char, ICU_DATA) &= ~ICU_SCSI_BIT;	/* select = 0 for 8490 */
  else
    RD_ADR (u_char, ICU_DATA) |= ICU_SCSI_BIT;	/* select = 1 for AIC6250 */
}

