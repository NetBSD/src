/*	$NetBSD: ahsc.c,v 1.27 2001/04/25 17:53:06 bouyer Exp $	*/

/*
 * Copyright (c) 1994 Christian E. Hopps
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
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>
#include <amiga/amiga/custom.h>
#include <amiga/amiga/cc.h>
#include <amiga/amiga/cfdev.h>
#include <amiga/amiga/device.h>
#include <amiga/amiga/isr.h>
#include <amiga/dev/dmavar.h>
#include <amiga/dev/sbicreg.h>
#include <amiga/dev/sbicvar.h>
#include <amiga/dev/ahscreg.h>
#include <amiga/dev/zbusvar.h>

#include <machine/cpu.h>

void ahscattach __P((struct device *, struct device *, void *));
int ahscmatch __P((struct device *, struct cfdata *, void *));

void ahsc_enintr __P((struct sbic_softc *));
void ahsc_dmastop __P((struct sbic_softc *));
int ahsc_dmanext __P((struct sbic_softc *));
int ahsc_dmaintr __P((void *));
int ahsc_dmago __P((struct sbic_softc *, char *, int, int));

#ifdef DEBUG
void ahsc_dump __P((void));
#endif

#ifdef DEBUG
int	ahsc_dmadebug = 0;
#endif

struct cfattach ahsc_ca = {
	sizeof(struct sbic_softc), ahscmatch, ahscattach
};

/*
 * if we are an A3000 we are here.
 */
int
ahscmatch(pdp, cfp, auxp)
	struct device *pdp;
	struct cfdata *cfp;
	void *auxp;
{
	char *mbusstr;

	mbusstr = auxp;
	if (is_a3000() && matchname(auxp, "ahsc"))
		return(1);
	return(0);
}

void
ahscattach(pdp, dp, auxp)
	struct device *pdp, *dp;
	void *auxp;
{
	volatile struct sdmac *rp;
	struct sbic_softc *sc = (struct sbic_softc *)dp;
	struct cfdev *cdp, *ecdp;
	struct scsipi_adapter *adapt = &sc->sc_adapter;
	struct scsipi_channel *chan = &sc->sc_channel;

	ecdp = &cfdev[ncfdev];
	
	for (cdp = cfdev; cdp < ecdp; cdp++) {
		if (cdp->rom.manid == 8738 && 
		    cdp->rom.prodid == 35)
				break;
	}

	sc->sc_cregs = rp = ztwomap(0xdd0000);
	/*
	 * disable ints and reset bank register
	 */
	rp->CNTR = CNTR_PDMD;
	rp->DAWR = DAWR_AHSC;
	sc->sc_enintr = ahsc_enintr;
	sc->sc_dmago = ahsc_dmago;
	sc->sc_dmanext = ahsc_dmanext;
	sc->sc_dmastop = ahsc_dmastop;
	sc->sc_dmacmd = 0;

	/*
	 * eveything is a valid dma address
	 */
	sc->sc_dmamask = 0;

	if (cdp < ecdp) {
		sc->sc_sbic.sbic_asr_p =  ((vu_char *)rp + 0x43);
		sc->sc_sbic.sbic_value_p =  ((vu_char *)rp + 0x47);
		printf(": modified for Apollo cpu board\n");
	} else {
		sc->sc_sbic.sbic_asr_p =  ((vu_char *)rp + 0x41);
		sc->sc_sbic.sbic_value_p =  ((vu_char *)rp + 0x43);
		printf("\n");
	}

	sc->sc_clkfreq = sbic_clock_override ? sbic_clock_override : 143;

	/*
	 * Fill in the scsipi_adapter.
	 */
	memset(adapt, 0, sizeof(*adapt));
	adapt->adapt_dev = &sc->sc_dev;
	adapt->adapt_nchannels = 1;   
	adapt->adapt_openings = 7;
	adapt->adapt_max_periph = 1;
	adapt->adapt_request = sbic_scsipi_request;
	adapt->adapt_minphys = sbic_minphys;

	/*
	 * Fill in the scsipi_channel.
	 */
	memset(chan, 0, sizeof(*chan));
	chan->chan_adapter = adapt;
	chan->chan_bustype = &scsi_bustype;
	chan->chan_channel = 0;
	chan->chan_ntargets = 8;      
	chan->chan_nluns = 8;
	chan->chan_id = 7;

	sbicinit(sc);

	sc->sc_isr.isr_intr = ahsc_dmaintr;
	sc->sc_isr.isr_arg = sc;
	sc->sc_isr.isr_ipl = 2;
	add_isr (&sc->sc_isr);

	/*
	 * attach all scsi units on us
	 */
	config_found(dp, chan, scsiprint);
}

void
ahsc_enintr(dev)
	struct sbic_softc *dev;
{
	volatile struct sdmac *sdp;

	sdp = dev->sc_cregs;

	dev->sc_flags |= SBICF_INTR;
	sdp->CNTR = CNTR_PDMD | CNTR_INTEN;
}

int
ahsc_dmago(dev, addr, count, flags)
	struct sbic_softc *dev;
	char *addr;
	int count, flags;
{
	volatile struct sdmac *sdp;

	sdp = dev->sc_cregs;
	/*
	 * Set up the command word based on flags
	 */
	dev->sc_dmacmd = CNTR_PDMD | CNTR_INTEN;
	if ((flags & DMAGO_READ) == 0)
		dev->sc_dmacmd |= CNTR_DDIR;
#ifdef DEBUG
	if (ahsc_dmadebug & DDB_IO)
		printf("ahsc_dmago: cmd %x\n", dev->sc_dmacmd);
#endif

	dev->sc_flags |= SBICF_INTR;
	sdp->CNTR = dev->sc_dmacmd;
	sdp->ACR = (u_int) dev->sc_cur->dc_addr;
	sdp->ST_DMA = 1;

	return(dev->sc_tcnt);
}

void
ahsc_dmastop(dev)
	struct sbic_softc *dev;
{
	volatile struct sdmac *sdp;
	int s;

	sdp = dev->sc_cregs;

#ifdef DEBUG
	if (ahsc_dmadebug & DDB_FOLLOW)
		printf("ahsc_dmastop()\n");
#endif
	if (dev->sc_dmacmd) {
		s = splbio();
		if ((dev->sc_dmacmd & (CNTR_TCEN | CNTR_DDIR)) == 0) {
			/*
			 * only FLUSH if terminal count not enabled,
			 * and reading from peripheral
			 */
			sdp->FLUSH = 1;
			while ((sdp->ISTR & ISTR_FE_FLG) == 0)
				;
		}
		/* 
		 * clear possible interrupt and stop dma
		 */
		sdp->CINT = 1;
		sdp->SP_DMA = 1;
		dev->sc_dmacmd = 0;
		splx(s);
	}
}

int
ahsc_dmaintr(arg)
	void *arg;
{
	struct sbic_softc *dev = arg;
	volatile struct sdmac *sdp;
	int stat, found;

	sdp = dev->sc_cregs;
	stat = sdp->ISTR;

	if ((stat & (ISTR_INT_F|ISTR_INT_P)) == 0)
		return (0);

#ifdef DEBUG
	if (ahsc_dmadebug & DDB_FOLLOW)
		printf("%s: dmaintr 0x%x\n", dev->sc_dev.dv_xname, stat);
#endif

	/*
	 * both, SCSI and DMA interrupts arrive here. I chose
	 * arbitrarily that DMA interrupts should have higher
	 * precedence than SCSI interrupts.
	 */
	found = 0;
	if (stat & ISTR_E_INT) {
		++found;

		sdp->CINT = 1;	/* clear possible interrupt */

		/*
		 * check for SCSI ints in the same go and 
		 * eventually save an interrupt
		 */
	}

	if (dev->sc_flags & SBICF_INTR && stat & ISTR_INTS)
		found += sbicintr(dev);
	return(found);
}


int
ahsc_dmanext(dev)
	struct sbic_softc *dev;
{
	volatile struct sdmac *sdp;

	sdp = dev->sc_cregs;

	if (dev->sc_cur > dev->sc_last) {
		/* shouldn't happen !! */
		printf("ahsc_dmanext at end !!!\n");
		ahsc_dmastop(dev);
		return(0);
	}
	if ((dev->sc_dmacmd & (CNTR_TCEN | CNTR_DDIR)) == 0) {
		  /* 
		   * only FLUSH if terminal count not enabled,
		   * and reading from peripheral
		   */
		sdp->FLUSH = 1;
		while ((sdp->ISTR & ISTR_FE_FLG) == 0)
			;
	}
	/* 
	 * clear possible interrupt and stop dma
	 */
	sdp->CINT = 1;	/* clear possible interrupt */
	sdp->SP_DMA = 1;	/* stop dma */
	sdp->CNTR = dev->sc_dmacmd;
	sdp->ACR = (u_int)dev->sc_cur->dc_addr;
	sdp->ST_DMA = 1;

	dev->sc_tcnt = dev->sc_cur->dc_count << 1;
	return(dev->sc_tcnt);
}

#ifdef DEBUG
void
ahsc_dump()
{
	extern struct cfdriver ahsc_cd;
	int i;

	for (i = 0; i < ahsc_cd.cd_ndevs; ++i)
		if (ahsc_cd.cd_devs[i])
			sbic_dump(ahsc_cd.cd_devs[i]);
}
#endif
