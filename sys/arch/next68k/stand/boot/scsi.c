/*      $NetBSD: scsi.c,v 1.2.22.1 2002/06/23 17:39:01 jdolecek Exp $        */
/*
 * Copyright (c) 1994, 1997 Rolf Grossmann
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
 *      This product includes software developed by Rolf Grossmann.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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
 */

#include <sys/param.h>
#include <next68k/dev/espreg.h>
#include <dev/ic/ncr53c9xreg.h>
#include <dev/scsipi/scsi_message.h>
#if 0
#include <next/next/prominfo.h>
#else
#include <next68k/next68k/nextrom.h>
#endif
#include "scsireg.h"
#include "dmareg.h"
#include "scsivar.h"

#include <lib/libsa/stand.h>

struct  scsi_softc scsi_softc, *sc = &scsi_softc;
char the_dma_buffer[MAX_DMASIZE+DMA_ENDALIGNMENT], *dma_buffer;

int scsi_msgin(void);
int dma_start(char *addr, int len);
int dma_done(void);

void scsi_init(void);
void scsierror(char *error);
short scsi_getbyte(volatile caddr_t sr);
int scsi_wait_for_intr(void);
int scsiicmd(char target, char lun,
	 u_char *cbuf, int clen, char *addr, int len);

#ifdef SCSI_DEBUG
#define DPRINTF(x) printf x;
#else
#define DPRINTF(x)
#endif

void
scsi_init(void)
{
    volatile caddr_t sr;
    struct dma_dev *dma;

    sr = P_SCSI;
    dma = (struct dma_dev *)P_SCSI_CSR;

    dma_buffer = DMA_ALIGN(char *, the_dma_buffer);
    
    P_FLOPPY[FLP_CTRL] &= ~FLC_82077_SEL;	/* select SCSI chip */

    /* first reset dma */
    dma->dd_csr        = DMACSR_RESET;
    DELAY(200);
    sr[ESP_DCTL]       = ESPDCTL_20MHZ | ESPDCTL_INTENB | ESPDCTL_RESET;
    DELAY(10);
    sr[ESP_DCTL]       = ESPDCTL_20MHZ | ESPDCTL_INTENB;
    DELAY(10);

    /* then reset the SCSI chip */
    sr[NCR_CMD]        = NCRCMD_RSTCHIP;
    sr[NCR_CMD]        = NCRCMD_NOP;
    DELAY(500);

    /* now reset the SCSI bus */
    sr[NCR_CMD]        = NCRCMD_RSTSCSI;
    DELAY(18000000);	/* XXX should be about 2-3 seconds at least */
    
    /* then reset the SCSI chip again and initialize it properly */
    sr[NCR_CMD]        = NCRCMD_RSTCHIP;
    sr[NCR_CMD]        = NCRCMD_NOP;
    DELAY(500);
    sr[NCR_CFG1]       = NCRCFG1_SLOW | NCRCFG1_BUSID;
    sr[NCR_CFG2]       = 0;
    sr[NCR_CCF]        = 4; /* S5RCLKCONV_FACTOR(20); */
    sr[NCR_TIMEOUT]    = 152; /* S5RSELECT_TIMEOUT(20,250); */
    sr[NCR_SYNCOFF]    = 0;
    sr[NCR_SYNCTP]     = 5;
   /*
    sc->sc_intrstatus  = sr->s5r_intrstatus;
    sc->sc_intrstatus  = sr->s5r_intrstatus;
    */
    sr[NCR_CFG1]       = NCRCFG1_PARENB | NCRCFG1_BUSID;

    sc->sc_state       = SCSI_IDLE;
}

void
scsierror(char *error)
{
    printf("scsierror: %s.\n", error);
}

short
scsi_getbyte(volatile caddr_t sr)
{
    if ((sr[NCR_FFLAG] & NCRFIFO_FF) == 0) 
    {
	printf("getbyte: no data!\n");
	return -1;
    }
    return sr[NCR_FIFO];
}

int
scsi_wait_for_intr(void)
{
#if 0
  extern struct prominfo *pi;
  volitle int = pi->pi_intrstat; /* ### use constant? */
#else
  extern char *mg;
#define	MON(type, off) (*(type *)((u_int) (mg) + off))
  volatile int *intrstat = MON(volatile int *,MG_intrstat);
  volatile int *intrmask = MON(volatile int *,MG_intrmask);
#endif
    int count;

    for(count = 0; count < SCSI_TIMEOUT; count++) {
			DPRINTF(("  *intrstat = 0x%x\t*intrmask = 0x%x\n",*intrstat,*intrmask));

	if (*intrstat & SCSI_INTR)
	    return 0;
		}

    printf("scsiicmd: timed out.\n");
    return -1;
}

int
scsiicmd(char target, char lun,
	 u_char *cbuf, int clen,
	 char *addr, int len)
{
    volatile caddr_t sr;
    int i;

    DPRINTF(("scsiicmd: [%x, %d] -> %d (%lx, %d)\n",*cbuf, clen,
	     target, (long)addr, len));
    sr = P_SCSI;

    if (sc->sc_state != SCSI_IDLE) {
        scsierror("scsiiscmd: bad state");
	return EIO;
    }
    sc->sc_result = 0;

    /* select target */
    sr[NCR_CMD]   = NCRCMD_FLUSH;
    DELAY(10);
    sr[NCR_SELID] = target;
    sr[NCR_FIFO]  = MSG_IDENTIFY(lun, 0);
    for (i=0; i<clen; i++)
	sr[NCR_FIFO] = cbuf[i];
    sr[NCR_CMD]   = NCRCMD_SELATN;
    sc->sc_state  = SCSI_SELECTING;
    
    while(sc->sc_state != SCSI_DONE) {
	if (scsi_wait_for_intr()) /* maybe we'd better use real intrs ? */
	    return EIO;

	if (sc->sc_state == SCSI_DMA)
	{
	    /* registers are not valid on dma intr */
	    sc->sc_status = sc->sc_seqstep = sc->sc_intrstatus = 0;
	    DPRINTF(("scsiicmd: dma intr\n"));
	} else {
	    /* scsi processing */
	    sc->sc_status     = sr[NCR_STAT];
	    sc->sc_seqstep    = sr[NCR_STEP];
	    sc->sc_intrstatus = sr[NCR_INTR];
	    DPRINTF(("scsiicmd: regs[intr=%x, stat=%x, step=%x]\n",
		     sc->sc_intrstatus, sc->sc_status, sc->sc_seqstep));
	}
	
	if (sc->sc_intrstatus & NCRINTR_SBR) {
	    scsierror("scsi bus reset");
	    return EIO;
	}
	
	if ((sc->sc_status & NCRSTAT_GE)
	    || (sc->sc_intrstatus & NCRINTR_ILL)) {
	    scsierror("software error");
	    return EIO;
	}
	if (sc->sc_status & NCRSTAT_PE)
	{
	    scsierror("parity error");
	    return EIO;
	}

	switch(sc->sc_state)
	{
	  case SCSI_SELECTING:
	      if (sc->sc_intrstatus & NCRINTR_DIS) 
	      {
		  sc->sc_state = SCSI_IDLE;
		  return EUNIT;	/* device not present */
	      }
	      
#define NCRINTR_DONE (NCRINTR_BS | NCRINTR_FC)
	      if ((sc->sc_intrstatus & NCRINTR_DONE) != NCRINTR_DONE)
	      {
		  scsierror("selection failed");
		  return EIO;
	      }
	      sc->sc_state = SCSI_HASBUS;
	      break;
	  case SCSI_HASBUS:
	      if (sc->sc_intrstatus & NCRINTR_DIS)
	      {
		  scsierror("target disconnected");
		  return EIO;
	      }
	      break;
	  case SCSI_DMA:
	      if (sc->sc_intrstatus & NCRINTR_DIS)
	      {
		  scsierror("target disconnected");
		  return EIO;
	      }
	      if (dma_done() != 0)
		  return EIO;
	      continue;
	  case SCSI_CLEANUP:
	      if (sc->sc_intrstatus & NCRINTR_DIS)
	      {
		  sc->sc_state = SCSI_DONE;
		  continue;
	      }
	      DPRINTF(("hmm ... no disconnect on cleanup?\n"));
	      sc->sc_state = SCSI_DONE;	/* maybe ... */
	      break;
	}

	/* transfer information now */
	switch(sc->sc_status & NCRSTAT_PHASE)
	{
	  case DATA_IN_PHASE:
	      if (dma_start(addr, len) != 0)
		  return EIO;
	      break;
	  case DATA_OUT_PHASE:
	      scsierror("data out phase not implemented");
	      return EIO;
	  case STATUS_PHASE:
	      DPRINTF(("status phase: "));
	      sr[NCR_CMD] = NCRCMD_ICCS;
	      sc->sc_result = scsi_getbyte(sr);
	      DPRINTF(("status is 0x%x.\n", sc->sc_result));
	      break;
	  case MSG_IN_PHASE:
	      if (scsi_msgin() != 0)
		  return EIO;
	      break;
	  default:
	      DPRINTF(("phase not implemented: 0x%x.\n",
		      sc->sc_status & NCRSTAT_PHASE));
              scsierror("bad phase");
	      return EIO;
	}
    }

    sc->sc_state = SCSI_IDLE;
    return -sc->sc_result;
}
    
int
scsi_msgin(void)
{
    volatile caddr_t sr;
    u_char msg;

    sr = P_SCSI;

    msg = scsi_getbyte(sr);
    if (msg)
    {
	printf("unexpected msg: 0x%x.\n",msg);
	return -1;
    }
    if ((sc->sc_intrstatus & NCRINTR_FC) == 0)
    {
	printf("not function complete.\n");
	return -1;
    }
    sc->sc_state = SCSI_CLEANUP;
    sr[NCR_CMD]  = NCRCMD_MSGOK;
    return 0;
}

int
dma_start(char *addr, int len)
{
    volatile caddr_t sr;
    struct dma_dev *dma;
    
    
    sr = P_SCSI;
    dma = (struct dma_dev *)P_SCSI_CSR;
    
    if (len > MAX_DMASIZE)
    {
	scsierror("dma too long");
	return -1;
    }

    if (addr == NULL || len == 0)
    {
#if 0 /* I'd take that as an error in my code */
	DPRINTF(("hmm ... no dma requested.\n"));
	sr[NCR_TCL] = 0;
	sr[NCR_TCM] = 1;
	sr[NCR_CMD] = NCRCMD_NOP;
	sr[NCR_CMD] = NCRCMD_DMA | NCRCMD_TRPAD;
	return 0;
#else
	scsierror("unrequested dma");
	return -1;
#endif
    }
    
    DPRINTF(("dma start: %lx, %d byte.\n", (long)addr, len));

    DPRINTF(("dma_bufffer: start: 0x%lx end: 0x%lx \n", 
				(long)dma_buffer,(long)DMA_ENDALIGN(char *, dma_buffer+len)));

    sc->dma_addr = addr;
    sc->dma_len = len;
    
    sr[NCR_TCL]  = len & 0xff;
    sr[NCR_TCM]  = len >> 8;
    sr[NCR_CMD]  = NCRCMD_DMA | NCRCMD_NOP;
    sr[NCR_CMD]  = NCRCMD_DMA | NCRCMD_TRANS;

#if 0
    dma->dd_csr = DMACSR_READ | DMACSR_RESET;
    dma->dd_next_initbuf = dma_buffer;
    dma->dd_limit = DMA_ENDALIGN(char *, dma_buffer+len);
    dma->dd_csr = DMACSR_READ | DMACSR_SETENABLE;
#else
    dma->dd_csr = 0;
    dma->dd_csr = DMACSR_INITBUF | DMACSR_READ | DMACSR_RESET;
    dma->dd_next_initbuf = dma_buffer;
    dma->dd_limit = DMA_ENDALIGN(char *, dma_buffer+len);
    dma->dd_csr = DMACSR_READ | DMACSR_SETENABLE;
#endif

    sr[ESP_DCTL] = ESPDCTL_20MHZ|ESPDCTL_INTENB|ESPDCTL_DMAMOD|ESPDCTL_DMARD;

    sc->sc_state = SCSI_DMA;
    return 0;
}

int
dma_done(void)
{
    volatile caddr_t sr;
    struct dma_dev *dma;
    int count, state;
    
    sr = P_SCSI;
    dma = (struct dma_dev *)P_SCSI_CSR;

    state = dma->dd_csr & (DMACSR_BUSEXC | DMACSR_COMPLETE
			   | DMACSR_SUPDATE | DMACSR_ENABLE);

    count = sr[NCR_TCM]<<8 | sr[NCR_TCL];
    DPRINTF(("dma state = 0x%x, remain = %d.\n", state, count));
    
    if (state & DMACSR_ENABLE) 
    {

			DPRINTF(("dma still enabled, flushing DCTL.\n"));

	sr[ESP_DCTL] = ESPDCTL_20MHZ | ESPDCTL_INTENB | ESPDCTL_DMAMOD
		       | ESPDCTL_DMARD | ESPDCTL_FLUSH;
/*	DELAY(5); */
	sr[ESP_DCTL] = ESPDCTL_20MHZ | ESPDCTL_INTENB | ESPDCTL_DMAMOD
		       | ESPDCTL_DMARD;
/*	DELAY(5); */

	return 0;
    }

    sr[ESP_DCTL] = ESPDCTL_20MHZ | ESPDCTL_INTENB;
    count = sr[NCR_TCM]<<8 | sr[NCR_TCL];
    dma->dd_csr = DMACSR_RESET;

    DPRINTF(("dma done. remain = %d, state = 0x%x.\n", count, state));

    if (count != 0)
    {
      printf("WARNING: unexpected %d characters remain in dma\n",count);
	scsierror("dma transfer incomplete");
#if 0
	return -1;
#endif
    }

    if (state & DMACSR_COMPLETE)
    {
	bcopy(dma_buffer, sc->dma_addr, sc->dma_len);
	sc->sc_state = SCSI_HASBUS;
	return 0;
    }
    if (state & DMACSR_BUSEXC)
    {
	scsierror("dma failed");
	return -1;
    }
    scsierror("dma not completed\n");
    
    return -1;
}
