/*
 * Copyright (c) 1982, 1990 The Regents of the University of California.
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
 *	@(#)dma.c
 *	$Id: gvp11dma.c,v 1.4 1994/02/11 07:01:46 chopps Exp $
 */

/*
 * GVP11 (Series II) DMA driver
 */

#include "param.h"
#include "systm.h"
#include "time.h"
#include "kernel.h"
#include "proc.h"

#include "vm/vm.h"
#include "vm/vm_kern.h"
#include "vm/vm_page.h"
#include "vm/vm_statistics.h"
#include "machine/pmap.h"

#include "device.h"
#include "gvp11dmareg.h"
#include "gvp11scsi.h"
#include "dmavar.h"
#include "scsivar.h"

#include "../include/cpu.h"

extern void timeout();

static void dmafree (register struct scsi_softc *dev);
static int dmago (struct scsi_softc *dev, char *addr, register int count, register int flags);
static int dmanext (struct scsi_softc *dev);
static void dmastop (struct scsi_softc *dev);
int gvp11dmaintr (void);

extern int	scsi_nscsi;	/* number of 33C93 controllers configured */
extern struct scsi_softc scsi_softc[];

int	gvp11dmaintr();

int	gvp11_max_dma = MAXPHYS;	/* Maximum size per DMA transfer */

#ifdef DEBUG
int	dmagvp11debug = 0;

void	gvp11dmatimeout();
#endif

void
gvp11dmainit (ac, dev)
     struct amiga_ctlr *ac;
     struct scsi_softc *dev;
{
  register struct sdmac *dma;
  register int i;
  char rev;

  dma = (struct sdmac *) ac->amiga_addr;

  dma->CNTR = 0;	/* disable interrupts from dma/scsi */

  dev->sc_cmd = 0;
  dev->sc_hwaddr = dma;
  i = ac->amiga_unit;
#ifdef DEBUG
  /* make sure timeout is really not needed */
  timeout(gvp11dmatimeout, 0, 30 * hz);
#endif

  dev->dmafree = (dmafree_t) dmafree;
  dev->dmago = (dmago_t) dmago;
  dev->dmanext = (dmanext_t) dmanext;
  dev->dmastop = (dmastop_t) dmastop;


  printf("dma%d: GVPII DMA\n", i);
}

static void
dmafree(dev)
	register struct scsi_softc *dev;
{
  int unit = dev->sc_ac->amiga_unit;
  register int s;

  s = splbio();
#ifdef DEBUG
  dev->dmatimo = 0;
#endif
  if (dev->sc_cmd)
    {
      ((struct sdmac *)dev->sc_hwaddr)->CNTR &= ~GVP_CNTR_INT_P; /* clear possible interrupt */
      ((struct sdmac *)dev->sc_hwaddr)->SP_DMA = 1;	/* stop dma */
      dev->sc_cmd = 0;
    }
  ((struct sdmac *)dev->sc_hwaddr)->CNTR = 0;	/* disable interrupts from dma/scsi */
  dev->sc_flags &= ~SCSI_INTR;

  splx(s);
}

static int
dmago(dev, addr, count, flags)
	register struct scsi_softc *dev;
	register char *addr;
	register int count;
	register int flags;
{
  int unit = dev->sc_ac->amiga_unit;

  /*
   * Set up the command word based on flags
   */
  dev->sc_cmd = GVP_CNTR_INTEN;
  if ((flags & DMAGO_READ) == 0)
    dev->sc_cmd |= GVP_CNTR_DDIR;
#ifdef DEBUG
  if (dmagvp11debug & DDB_IO)
    {
      printf("dmago: cmd %x\n", dev->sc_cmd);
    }
  dev->dmatimo = 1;
#endif

  ((struct sdmac *)dev->sc_hwaddr)->CNTR = dev->sc_cmd;
  dev->sc_flags |= SCSI_INTR;
  ((struct sdmac *)dev->sc_hwaddr)->ACR = (u_int) dev->sc_cur->dc_addr;
  ((struct sdmac *)dev->sc_hwaddr)->ST_DMA = 1;
  
  if (dev->sc_tc > gvp11_max_dma)
    dev->sc_tc = gvp11_max_dma;		/* restrict transfer count to maximum */
  return dev->sc_tc;
}

static void
dmastop(dev)
     register struct scsi_softc *dev;
{
  register struct devqueue *dq;
  int unit = dev->sc_ac->amiga_unit;
  int s;

#ifdef DEBUG
  if (dmagvp11debug & DDB_FOLLOW)
    printf("dmastop()\n");
  dev->dmatimo = 0;
#endif
  if (dev->sc_cmd)
    {
      s = splbio ();
      ((struct sdmac *)dev->sc_hwaddr)->CNTR &= ~GVP_CNTR_INT_P;	/* clear possible interrupt */
      ((struct sdmac *)dev->sc_hwaddr)->SP_DMA = 1;	/* stop dma */
      dev->sc_cmd = 0;
      splx (s);
    }

  /*
   * We may get this interrupt after a device service routine
   * has freed the dma channel.  So, ignore the intr if there's
   * nothing on the queue.
   */
#if 0		/* XXX - not needed anymore? */
  dq = dev->sc_sq.dq_forw;
  if (dq != &dmachan[unit])
    (dq->dq_driver->d_done)(unit);
#endif
}

int
gvp11dmaintr()
{
  register struct scsi_softc *dev;
  register int i, stat;
  int found = 0;

  for (i = 0, dev = scsi_softc; i < scsi_nscsi; i++, dev++)
    {
      if (dev->dmago != (dmago_t) dmago)
	continue;			/* This controller isn't ours */
      stat = ((struct sdmac *)dev->sc_hwaddr)->CNTR;
      
      if (! (stat & GVP_CNTR_INT_P))
	continue;
  
#ifdef DEBUG
      if (dmagvp11debug & DDB_FOLLOW)
	printf("dmaintr (%d, 0x%x)\n", i, stat);
#endif

      if (dev->sc_flags & SCSI_INTR)
	found += scsiintr (i);
    }

  return found;
}


static int
dmanext (dev)
     register struct scsi_softc *dev;
{
  register int unit = dev->sc_ac->amiga_unit;
  register int i, stat;

  if (dev->sc_cur <= dev->sc_last)
    {
#ifdef DEBUG
      dev->dmatimo = 1;
#endif
      ((struct sdmac *)dev->sc_hwaddr)->CNTR &= ~GVP_CNTR_INT_P; /* clear possible interrupt */
      ((struct sdmac *)dev->sc_hwaddr)->SP_DMA = 1;        /* stop dma */
      ((struct sdmac *)dev->sc_hwaddr)->CNTR = dev->sc_cmd;
      ((struct sdmac *)dev->sc_hwaddr)->ACR = (u_int) dev->sc_cur->dc_addr;
      ((struct sdmac *)dev->sc_hwaddr)->ST_DMA = 1;
      
      dev->sc_tc = dev->sc_cur->dc_count << 1;
      if (dev->sc_tc > gvp11_max_dma)
        dev->sc_tc = gvp11_max_dma;
      return dev->sc_tc;
    } 
  else
    {
      /* shouldn't happen !! */
      printf ("dmanext at end !!!\n");
      dmastop (dev);
      return 0;
    }
}




#ifdef DEBUG
void
gvp11dmatimeout()
{
  register int i, s;

  for (i = 0; i < scsi_nscsi; i++)
    {
      if (scsi_softc[i].dmago != (dmago_t) dmago)
	continue;
      s = splbio();
      if (scsi_softc[i].dmatimo)
	{
	  if (scsi_softc[i].dmatimo > 1)
	    printf("dma%d: timeout #%d\n", i, scsi_softc[i].dmatimo-1);
	  scsi_softc[i].dmatimo++;
	}
      splx(s);
    }
  timeout (gvp11dmatimeout, (caddr_t)0, 30 * hz);
}
#endif
