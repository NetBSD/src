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
#include "gvp11dmareg.h"
#include "gvp11scsi.h"
#include "dmavar.h"

#include "../include/cpu.h"

extern void isrlink();
extern void _insque();
extern void _remque();
extern void timeout();
extern u_int kvtop();

static int dmareq (register struct devqueue *dq);
static void dmafree (register struct devqueue *dq);
static int dmago (int unit, char *addr, register int count, register int flags);
static int dmanext (int unit);
static void dmastop (int unit);
int gvp11dmaintr (void);

#define NDMA NGVP11SCSI

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

static struct	dma_softc {
	struct	sdmac *sc_hwaddr;
	char	sc_dmaflags;
	u_short	sc_cmd;
	struct	dma_chain *sc_cur;
	struct	dma_chain *sc_last;
	struct	dma_chain sc_chain[DMAMAXIO];
} dma_softc[NDMA];

#define DMAF_NOINTR	0x01

static struct	devqueue dmachan[NDMA + 1];
int	gvp11dmaintr();

/* because keybord and dma share same interrupt priority... */
static int dma_initialized = 0;
/* if not in int-mode, don't pay attention for possible scsi interrupts */
static int scsi_int_mode[NDMA];

#ifdef DEBUG
#define	DDB_FOLLOW	0x04
#define DDB_IO		0x08
int	dmagvp11debug = 0;

void	gvp11dmatimeout();
static int	dmatimo[NDMA];

static long	dmahits[NDMA];
static long	dmamisses[NDMA];
static long	dmabyte[NDMA];
static long	dmaword[NDMA];
static long	dmalword[NDMA];
#endif

void
gvp11dmainit (ac, dreq, dfree, dgo, dnext, dstop)
     struct amiga_ctlr *ac;
     dmareq_t   *dreq;
     dmafree_t  *dfree;
     dmago_t    *dgo;
     dmanext_t  *dnext;
     dmastop_t  *dstop;
{
  register struct sdmac *dma;
  struct dma_softc *dc;
  register int i;
  char rev;

  dma = (struct sdmac *) ac->amiga_addr;
  dc  = dma_softc + ac->amiga_unit;


  dma->CNTR = 0;	/* disable interrupts from dma/scsi */

  dc->sc_dmaflags = 0;
  dc->sc_cmd = 0;
  dc->sc_hwaddr = dma;
  i = ac->amiga_unit;
  dmachan[i].dq_forw = dmachan[i].dq_back = &dmachan[i];
  dmachan[NDMA].dq_forw = dmachan[NDMA].dq_back = &dmachan[NDMA];
#ifdef DEBUG
  /* make sure timeout is really not needed */
  timeout(gvp11dmatimeout, 0, 30 * hz);
#endif

  *dreq  = dmareq;
  *dfree = dmafree;
  *dgo   = dmago;
  *dnext = dmanext;
  *dstop = dmastop;


  printf("dma%d: GVPII DMA\n", i);
  dma_initialized = 1;
}

static int
dmareq(dq)
	register struct devqueue *dq;
{
  register int s = splbio();
  register int ctlr = dq->dq_ctlr;

  if (dmachan[ctlr].dq_forw == &dmachan[ctlr])
    {
      insque(dq, &dmachan[ctlr]);
      splx(s);
      return 1;
    }

  insque (dq, dmachan[NDMA].dq_back);
  splx(s);
  return(0);
}

static void
dmafree(dq)
	register struct devqueue *dq;
{
  int unit = dq->dq_ctlr;
  register struct dma_softc *dc = &dma_softc[unit];
  register struct devqueue *dn;
  register int s;

  s = splbio();
#ifdef DEBUG
  dmatimo[unit] = 0;
#endif
  if (dc->sc_cmd)
    {
      dc->sc_hwaddr->CNTR &= ~GVP_CNTR_INT_P; /* clear possible interrupt */
      dc->sc_hwaddr->SP_DMA = 1;	/* stop dma */
      dc->sc_cmd = 0;
    }
  dc->sc_hwaddr->CNTR = 0;	/* disable interrupts from dma/scsi */
  scsi_int_mode[unit] = 0;

  remque(dq);
  for (dn = dmachan[NDMA].dq_forw;
       dn != &dmachan[NDMA]; 
       dn = dn->dq_forw) 
    {
      if (dn->dq_ctlr == unit)
	{
	  remque ((caddr_t) dn);
	  insque ((caddr_t) dn, (caddr_t) dq->dq_back);
	  splx(s);
	  (dn->dq_driver->d_start)(unit);
	  return;
	}
    }

  splx(s);
}

static int
dmago(unit, addr, count, flags)
        int unit;
	register char *addr;
	register int count;
	register int flags;
{
  register struct dma_softc *dc = &dma_softc[unit];
  register struct dma_chain *dcp;
  register char *dmaend = NULL;
  register int tcount;

  if (count > MAXPHYS)
     panic("dmago: count > MAXPHYS");
#ifdef DEBUG
  if (dmagvp11debug & DDB_FOLLOW)
     printf("dmago(%d, %x, %x, %x)\n", unit, addr, count, flags);
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
	  dmahits[unit]++;
#endif
	  dmaend += dcp->dc_count;
	  (--dcp)->dc_count += tcount;
	} 
      else 
        {
#ifdef DEBUG
	  dmamisses[unit]++;
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
  dc->sc_cmd = GVP_CNTR_INTEN;
  if ((flags & DMAGO_READ) == 0)
    dc->sc_cmd |= GVP_CNTR_DDIR;
#ifdef DEBUG
  if (dmagvp11debug & DDB_IO)
    {
      printf("dmago: cmd %x, flags %x\n",
	     dc->sc_cmd, dc->sc_dmaflags);
      for (dcp = dc->sc_chain; dcp <= dc->sc_last; dcp++)
	printf("  %d: %d@%x\n", dcp-dc->sc_chain,
	       dcp->dc_count, dcp->dc_addr);
    }
  dmatimo[unit] = 1;
#endif

  DCIS();				/* push data cache */
  dc->sc_hwaddr->CNTR = dc->sc_cmd;
  scsi_int_mode[unit] = 1;
  dc->sc_hwaddr->ACR = (u_int) dc->sc_cur->dc_addr;
  dc->sc_hwaddr->ST_DMA = 1;
  
  return dc->sc_cur->dc_count << 1;
}

static void
dmastop(unit)
     register int unit;
{
  register struct dma_softc *dc = &dma_softc[unit];
  register struct devqueue *dq;
  int s;

#ifdef DEBUG
  if (dmagvp11debug & DDB_FOLLOW)
    printf("dmastop()\n");
  dmatimo[unit] = 0;
#endif
  if (dc->sc_cmd)
    {
      s = splbio ();
      dc->sc_hwaddr->CNTR &= ~GVP_CNTR_INT_P;	/* clear possible interrupt */
      dc->sc_hwaddr->SP_DMA = 1;	/* stop dma */
      dc->sc_cmd = 0;
      splx (s);
    }

  /*
   * We may get this interrupt after a device service routine
   * has freed the dma channel.  So, ignore the intr if there's
   * nothing on the queue.
   */
  dq = dmachan[unit].dq_forw;
  if (dq != &dmachan[unit])
    (dq->dq_driver->d_done)(unit);
}

int
gvp11dmaintr()
{
  register struct dma_softc *dc;
  register int i, stat;
  int found = 0;

  if (! dma_initialized)
    return 0;

  for (i = 0, dc = dma_softc; i < NDMA; i++, dc++)
    {
      stat = dc->sc_hwaddr->CNTR;
      
      if (! (stat & GVP_CNTR_INT_P))
	continue;
  
#ifdef DEBUG
      if (dmagvp11debug & DDB_FOLLOW)
	printf("dmaintr (%d, 0x%x)\n", i, stat);
#endif

      if (scsi_int_mode[i])
	found += scsiintr (i);
    }

  return found;
}


static int
dmanext (unit)
     register int unit;
{
  register struct dma_softc *dc;
  register int i, stat;
  int found = 0;

  dc = &dma_softc[unit];

#ifdef DEBUG
  if (dmagvp11debug & DDB_IO) 
    {
      printf("dmanext(%d): next %d\n", unit, (dc->sc_cur-dc->sc_chain)+1);
    }
#endif
  if (++dc->sc_cur <= dc->sc_last) 
    {
#ifdef DEBUG
      dmatimo[unit] = 1;
#endif
      dc->sc_hwaddr->CNTR &= ~GVP_CNTR_INT_P; /* clear possible interrupt */
      dc->sc_hwaddr->SP_DMA = 1;        /* stop dma */
      dc->sc_hwaddr->CNTR = dc->sc_cmd;
      dc->sc_hwaddr->ACR = (u_int) dc->sc_cur->dc_addr;
      dc->sc_hwaddr->ST_DMA = 1;
      
      return dc->sc_cur->dc_count << 1;
    } 
  else
    {
      /* shouldn't happen !! */
      printf ("dmanext at end !!!\n");
      dmastop (i);
      return 0;
    }
}




#ifdef DEBUG
void
gvp11dmatimeout()
{
  register int i, s;

  for (i = 0; i < NDMA; i++) 
    {
      s = splbio();
      if (dmatimo[i]) 
	{
	  if (dmatimo[i] > 1)
	    printf("dma%d: timeout #%d\n", i, dmatimo-1);
	  dmatimo[i]++;
	}
      splx(s);
    }
  timeout (gvp11dmatimeout, (caddr_t)0, 30 * hz);
}
#endif
