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
 */

/*
 * DMA driver
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
#include "dmareg.h"
#include "dmavar.h"

#include "../include/cpu.h"

extern void isrlink();
extern void _insque();
extern void _remque();
extern void timeout();
extern u_int kvtop();

/*
 * The largest single request will be MAXPHYS bytes which will require
 * at most MAXPHYS/NBPG+1 chain elements to describe, i.e. if none of
 * the buffer pages are physically contiguous (MAXPHYS/NBPG) and the
 * buffer is not page aligned (+1).
 */
#define	DMAMAXIO	(MAXPHYS/NBPG+1)

struct	dma_chain {
	int	dc_count;
	char	*dc_addr;
};

struct	dma_softc {
	struct	sdmac *sc_hwaddr;
	char	sc_dmaflags;
	u_short	sc_cmd;
	struct	dma_chain *sc_cur;
	struct	dma_chain *sc_last;
	struct	dma_chain sc_chain[DMAMAXIO];
} dma_softc;

#define DMAF_NOINTR	0x01

struct	devqueue dmachan[NDMA + 1];
int	dmaintr();

/* because keybord and dma share same interrupt priority... */
int dma_initialized = 0;
/* if not in int-mode, don't pay attention for possible scsi interrupts */
int scsi_int_mode = 0;

#ifdef DEBUG
#define	DDB_FOLLOW	0x04
#define DDB_IO		0x08
int	dmadebug = 0;

void	dmatimeout();
int	dmatimo;

long	dmahits;
long	dmamisses;
long	dmabyte;
long	dmaword;
long	dmalword;
#endif

/* This setup function is also used by the SCSI driver, as it shares
   the same chip */

struct sdmac *SDMAC_address = 0;
volatile void *SCSI_address = 0;

void 
SDMAC_setup ()
{
  if (! SDMAC_address)
    {
      extern u_int SCSIADDR;	/* setup in amiga_init.c. Should really be mapped here... */
      SDMAC_address = (struct sdmac *) SCSIADDR;
      SCSI_address = &SDMAC_address->SASR;
    }
}


void
dmainit()
{
  register struct sdmac *dma;
  register int i;
  char rev;

  SDMAC_setup ();
  dma = SDMAC_address;

  /* make sure interrupts are disabled while we later on reset the scsi-chip */
  dma->CNTR = CNTR_PDMD;
  dma->DAWR = DAWR_A3000;
  dma_softc.sc_dmaflags = 0;
  dma_softc.sc_cmd = 0;
  dma_softc.sc_hwaddr = dma;
  dmachan[0].dq_forw = dmachan[0].dq_back = &dmachan[0];
  dmachan[1].dq_forw = dmachan[1].dq_back = &dmachan[1];
#ifdef DEBUG
  /* make sure timeout is really not needed */
  timeout(dmatimeout, 0, 30 * hz);
#endif

  printf("dma: A3000 %d bit DMA\n", 32);	/* XXX */
  dma_initialized = 1;
}

int
dmareq(dq)
	register struct devqueue *dq;
{
  register int s = splbio();

  if (dmachan[0].dq_forw == &dmachan[0])
    {
      insque(dq, &dmachan[0]);
      dq->dq_ctlr = 0;
      splx(s);
      return 1;
    }

  insque (dq, dmachan[1].dq_back);
  splx(s);
  return(0);
}

void
dmafree(dq)
	register struct devqueue *dq;
{
  register struct dma_softc *dc = &dma_softc;
  register struct devqueue *dn;
  register int s;

  s = splbio();
#ifdef DEBUG
  dmatimo = 0;
#endif
  if (dc->sc_cmd)
    {
      if ((dc->sc_cmd & (CNTR_TCEN | CNTR_DDIR)) == 0)
        {
          /* only FLUSH if terminal count not enabled, and reading from peripheral */
          dc->sc_hwaddr->FLUSH = 1;
          while (! (dc->sc_hwaddr->ISTR & ISTR_FE_FLG)) ;
        }
      dc->sc_hwaddr->CINT = 1;		/* clear possible interrupt */
      dc->sc_hwaddr->SP_DMA = 1;	/* stop dma */
      dc->sc_cmd = 0;
    }
  dc->sc_hwaddr->CNTR = CNTR_PDMD;	/* disable interrupts from dma/scsi */
  scsi_int_mode = 0;

  remque(dq);
  for (dn = dmachan[1].dq_forw;
       dn != &dmachan[1]; 
       dn = dn->dq_forw) 
    {
      remque ((caddr_t) dn);
      insque ((caddr_t) dn, (caddr_t) dq->dq_back);
      splx(s);
      dn->dq_ctlr = dq->dq_ctlr;
      (dn->dq_driver->d_start)(0);
      return;
    }

  splx(s);
}

int
dmago(addr, count, flags)
	register char *addr;
	register int count;
	register int flags;
{
  register struct dma_softc *dc = &dma_softc;
  register struct dma_chain *dcp;
  register char *dmaend = NULL;
  register int tcount;

  if (count > MAXPHYS)
     panic("dmago: count > MAXPHYS");
#ifdef DEBUG
  if (dmadebug & DDB_FOLLOW)
     printf("dmago(%x, %x, %x)\n", addr, count, flags);
#endif
  /*
   * Build the DMA chain
   */
  for (dcp = dc->sc_chain; count > 0; dcp++) 
    {
#ifdef DEBUG
      if (! pmap_extract(pmap_kernel(), (vm_offset_t)addr))
        panic ("dmago: no physical page for address!");
#endif

      dcp->dc_addr = (char *) kvtop(addr);
      if (count < (tcount = NBPG - ((int)addr & PGOFSET)))
	tcount = count;
      dcp->dc_count = tcount;
      addr += tcount;
      count -= tcount;
      tcount >>= 1;	/* number of words (the sdmac wants 16bit values here) */
      if (dcp->dc_addr == dmaend) 
	{
#ifdef DEBUG
	  dmahits++;
#endif
	  dmaend += dcp->dc_count;
	  (--dcp)->dc_count += tcount;
	} 
      else 
        {
#ifdef DEBUG
	  dmamisses++;
#endif
	  dmaend = dcp->dc_addr + dcp->dc_count;
	  dcp->dc_count = tcount;
	}
    }

  dc->sc_cur = dc->sc_chain;
  dc->sc_last = --dcp;
  dc->sc_dmaflags = 0;
  /*
   * Set up the command word based on flags
   */
  dc->sc_cmd = CNTR_PDMD | CNTR_INTEN;
  if ((flags & DMAGO_READ) == 0)
    dc->sc_cmd |= CNTR_DDIR;
#ifdef DEBUG
  if (dmadebug & DDB_IO)
    {
      printf("dmago: cmd %x, flags %x\n",
	     dc->sc_cmd, dc->sc_dmaflags);
      for (dcp = dc->sc_chain; dcp <= dc->sc_last; dcp++)
	printf("  %d: %d@%x\n", dcp-dc->sc_chain,
	       dcp->dc_count, dcp->dc_addr);
    }
  dmatimo = 1;
#endif

  dc->sc_hwaddr->CNTR = dc->sc_cmd;
  scsi_int_mode = 1;
  dc->sc_hwaddr->ACR = (u_int) dc->sc_cur->dc_addr;
  dc->sc_hwaddr->ST_DMA = 1;
  
  return dc->sc_cur->dc_count << 1;
}

void
dmastop()
{
  register struct dma_softc *dc = &dma_softc;
  register struct devqueue *dq;
  int s;

#ifdef DEBUG
  if (dmadebug & DDB_FOLLOW)
    printf("dmastop()\n");
  dmatimo = 0;
#endif
  if (dc->sc_cmd)
    {
      s = splbio ();
      if ((dc->sc_cmd & (CNTR_TCEN | CNTR_DDIR)) == 0)
        {
          /* only FLUSH if terminal count not enabled, and reading from peripheral */
	  dc->sc_hwaddr->FLUSH = 1;
          while (! (dc->sc_hwaddr->ISTR & ISTR_FE_FLG)) ;
        }
      dc->sc_hwaddr->CINT = 1;		/* clear possible interrupt */
      dc->sc_hwaddr->SP_DMA = 1;	/* stop dma */
      dc->sc_cmd = 0;
      splx (s);
    }

  /*
   * We may get this interrupt after a device service routine
   * has freed the dma channel.  So, ignore the intr if there's
   * nothing on the queue.
   */
  dq = dmachan[0].dq_forw;
  if (dq != &dmachan[0])
    (dq->dq_driver->d_done)(0);
}

int
dmaintr()
{
  register struct dma_softc *dc;
  register int i, stat;
  int found = 0;

  if (! dma_initialized)
    return 0;

  dc = &dma_softc;
  stat = dc->sc_hwaddr->ISTR;

  /* if no interrupt from this device, return quickly */
  if (! (stat & (ISTR_INT_F|ISTR_INT_P)))
    return 0;
  
#ifdef DEBUG
  if (dmadebug & DDB_FOLLOW)
     printf("dmaintr (0x%x)\n", stat);
#endif

  /* both, SCSI and DMA interrupts arrive here. I chose arbitrarily that
     DMA interrupts should have higher precedence than SCSI interrupts. */

  if (stat & ISTR_E_INT)
    {
      found++;

printf ("DMAINTR should no longer be entered!!\n");


      /* End-of-process interrupt, means the DMA Terminal Counter reached
         0, and we have to reload it. SCSI transfer should not be interrupted
         by this event (we'll see..) */

#ifdef DEBUG
      if (dmadebug & DDB_IO) 
	{
	  printf("dmaintr: stat %x next %d\n",
		 stat, (dc->sc_cur-dc->sc_chain)+1);
	}
#endif
      if (++dc->sc_cur <= dc->sc_last) 
	{
#ifdef DEBUG
	  dmatimo = 1;
#endif
	  /*
	   * Last chain segment, disable DMA interrupt.
	   */
	  if (dc->sc_cur == dc->sc_last && (dc->sc_dmaflags & DMAF_NOINTR))
	    dc->sc_cmd &= ~CNTR_TCEN;

	  dc->sc_hwaddr->CINT = 1;		/* clear possible interrupt */
	  dc->sc_hwaddr->CNTR = dc->sc_cmd;
	  dc->sc_hwaddr->ACR = (u_int) dc->sc_cur->dc_addr;
#ifndef DONT_WTC
  	  if (dc->sc_cmd & CNTR_TCEN)
  	    dc->sc_hwaddr->WTC = dc->sc_cur->dc_count;
#endif
	  dc->sc_hwaddr->ST_DMA = 1;
	} 
      else
	dmastop ();
	
      /* check for SCSI ints in the same go and eventually save an interrupt */
    }

  if (scsi_int_mode && (stat & ISTR_INTS))
    return found + scsiintr (0);

  return found;
}


int
dmanext ()
{
  register struct dma_softc *dc;
  register int i, stat;
  int found = 0;

  dc = &dma_softc;

#ifdef DEBUG
  if (dmadebug & DDB_IO) 
    {
      printf("dmanext: next %d\n", (dc->sc_cur-dc->sc_chain)+1);
    }
#endif
  if (++dc->sc_cur <= dc->sc_last) 
    {
#ifdef DEBUG
      dmatimo = 1;
#endif
      if ((dc->sc_cmd & (CNTR_TCEN | CNTR_DDIR)) == 0)
        {
          /* only FLUSH if terminal count not enabled, and reading from peripheral */
          dc->sc_hwaddr->FLUSH = 1;
          while (! (dc->sc_hwaddr->ISTR & ISTR_FE_FLG)) ;
        }
      dc->sc_hwaddr->CINT = 1;		/* clear possible interrupt */
      dc->sc_hwaddr->SP_DMA = 1;	/* stop dma */
      dc->sc_hwaddr->CNTR = dc->sc_cmd;
      dc->sc_hwaddr->ACR = (u_int) dc->sc_cur->dc_addr;
      dc->sc_hwaddr->ST_DMA = 1;
      
      return dc->sc_cur->dc_count << 1;
    } 
  else
    {
      /* shouldn't happen !! */
      printf ("dmanext at end !!!\n");
      dmastop ();
      return 0;
    }
}




#ifdef DEBUG
void
dmatimeout()
{
  register int s;

  s = splbio();
  if (dmatimo) 
    {
      if (dmatimo > 1)
	printf("dma: timeout #%d\n", dmatimo-1);
      dmatimo++;
    }
  splx(s);
  timeout (dmatimeout, (caddr_t)0, 30 * hz);
}
#endif
