/*	$NetBSD: gtsc.c,v 1.39 2010/02/05 12:13:36 phx Exp $ */

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
 * 3. Neither the name of the University nor the names of its contributors
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
 * Copyright (c) 1994 Christian E. Hopps
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: gtsc.c,v 1.39 2010/02/05 12:13:36 phx Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>
#include <amiga/amiga/custom.h>
#include <amiga/amiga/cc.h>
#include <amiga/amiga/device.h>
#include <amiga/amiga/isr.h>
#include <amiga/dev/dmavar.h>
#include <amiga/dev/sbicreg.h>
#include <amiga/dev/sbicvar.h>
#include <amiga/dev/gtscreg.h>
#include <amiga/dev/zbusvar.h>
#include <amiga/dev/gvpbusvar.h>

void gtscattach(struct device *, struct device *, void *);
int gtscmatch(struct device *, struct cfdata *, void *);

void gtsc_enintr(struct sbic_softc *);
void gtsc_dmastop(struct sbic_softc *);
int gtsc_dmanext(struct sbic_softc *);
int gtsc_dmaintr(void *);
int gtsc_dmago(struct sbic_softc *, char *, int, int);

#ifdef DEBUG
void gtsc_dump(void);
#endif

int gtsc_maxdma = 0;	/* Maximum size per DMA transfer */
int gtsc_dmamask = 0;
int gtsc_dmabounce = 0;
int gtsc_clock_override = 0;

#ifdef DEBUG
int gtsc_debug = 0;
#endif

CFATTACH_DECL(gtsc, sizeof(struct sbic_softc),
    gtscmatch, gtscattach, NULL, NULL);

int
gtscmatch(struct device *pdp, struct cfdata *cfp, void *auxp)
{
	struct gvpbus_args *gap;

	gap = auxp;
	if (gap->flags & GVP_SCSI)
		return(1);
	return(0);
}

/*
 * attach all devices on our board.
 */
void
gtscattach(struct device *pdp, struct device *dp, void *auxp)
{
	volatile struct sdmac *rp;
	struct gvpbus_args *gap;
	struct sbic_softc *sc = (struct sbic_softc *)dp;
	struct scsipi_adapter *adapt = &sc->sc_adapter;
	struct scsipi_channel *chan = &sc->sc_channel;

	gap = auxp;
	sc->sc_cregs = rp = gap->zargs.va;

	/*
	 * disable ints and reset bank register
	 */
	rp->CNTR = 0;
	amiga_membarrier();
	if ((gap->flags & GVP_NOBANK) == 0) {
		rp->bank = 0;
		amiga_membarrier();
	}

	sc->sc_dmago =  gtsc_dmago;
	sc->sc_enintr = gtsc_enintr;
	sc->sc_dmanext = gtsc_dmanext;
	sc->sc_dmastop = gtsc_dmastop;
	sc->sc_dmacmd = 0;

	sc->sc_flags |= SBICF_BADDMA;
	if (gtsc_dmamask)
		sc->sc_dmamask = gtsc_dmamask;
	else if (gap->flags & GVP_24BITDMA)
		sc->sc_dmamask = ~0x00ffffff;
	else if (gap->flags & GVP_25BITDMA)
		sc->sc_dmamask = ~0x01ffffff;
	else
		sc->sc_dmamask = ~0x07ffffff;
	printf(": dmamask 0x%lx", ~sc->sc_dmamask);

	if ((gap->flags & GVP_NOBANK) == 0)
		sc->gtsc_bankmask = (~sc->sc_dmamask >> 18) & 0x01c0;

#if 0
	/*
	 * if the user requests a bounce buffer or
	 * the users kva space is not ztwo and DMA needs it
	 * try and allocate a bounce buffer.  If we allocate
	 * one and it is in ztwo space leave maxdma to user
	 * setting or default to MAXPHYS else the address must
	 * be on the chip bus so decrease it to either the users
	 * setting or 1024 bytes.
	 *
	 * XXX this needs to change if we move to multiple memory segments.
	 */
	if (gtsc_dmabounce || kvtop(sc) & sc->sc_dmamask) {
		sc->sc_dmabuffer = (char *) alloc_z2mem(MAXPHYS * 8); /* XXX */
		if (isztwomem(sc->sc_dmabuffer))
			printf(" bounce pa 0x%x", kvtop(sc->sc_dmabuffer));
		else if (gtsc_maxdma == 0) {
			gtsc_maxdma = 1024;
			printf(" bounce pa 0x%x",
			    PREP_DMA_MEM(sc->sc_dmabuffer));
		}
	}
#endif
	if (gtsc_maxdma == 0)
		gtsc_maxdma = MAXPHYS;

	printf(" flags %x", gap->flags);
	printf(" maxdma %d\n", gtsc_maxdma);

	sc->sc_sbic.sbic_asr_p = (volatile unsigned char *)rp + 0x61;
	sc->sc_sbic.sbic_value_p = (volatile unsigned char *)rp + 0x63;
	amiga_membarrier();

	sc->sc_clkfreq = gtsc_clock_override ? gtsc_clock_override :
	    ((gap->flags & GVP_14MHZ) ? 143 : 72);
	printf("sc_clkfreg: %ld.%ld MHz\n", sc->sc_clkfreq / 10, sc->sc_clkfreq % 10);

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

	sc->sc_isr.isr_intr = gtsc_dmaintr;
	sc->sc_isr.isr_arg = sc;
	sc->sc_isr.isr_ipl = 2;
	add_isr(&sc->sc_isr);

	/*
	 * attach all scsi units on us
	 */
	config_found(dp, chan, scsiprint);
}

void
gtsc_enintr(struct sbic_softc *dev)
{
	volatile struct sdmac *sdp;

	sdp = dev->sc_cregs;

	dev->sc_flags |= SBICF_INTR;
	sdp->CNTR = GVP_CNTR_INTEN;
	amiga_membarrier();
}

int
gtsc_dmago(struct sbic_softc *dev, char *addr, int count, int flags)
{
	volatile struct sdmac *sdp;

	sdp = dev->sc_cregs;
	/*
	 * Set up the command word based on flags
	 */
	dev->sc_dmacmd = GVP_CNTR_INTEN;
	if ((flags & DMAGO_READ) == 0)
		dev->sc_dmacmd |= GVP_CNTR_DDIR;

#ifdef DEBUG
	if (gtsc_debug & DDB_IO)
		printf("gtsc_dmago: cmd %x\n", dev->sc_dmacmd);
#endif
	dev->sc_flags |= SBICF_INTR;
	sdp->CNTR = dev->sc_dmacmd;
	amiga_membarrier();
	if((u_int)dev->sc_cur->dc_addr & dev->sc_dmamask) {
#if 1
		printf("gtsc_dmago: pa %p->%lx dmacmd %x",
		    dev->sc_cur->dc_addr,
		    (u_int)dev->sc_cur->dc_addr & ~dev->sc_dmamask,
		     dev->sc_dmacmd);
#endif
		sdp->ACR = 0x00f80000;	/***********************************/
	} else
		sdp->ACR = (u_int) dev->sc_cur->dc_addr;
	amiga_membarrier();
	if (dev->gtsc_bankmask) {
		sdp->bank =
		    dev->gtsc_bankmask & (((u_int)dev->sc_cur->dc_addr) >> 18);
		amiga_membarrier();
	}
	sdp->ST_DMA = 1;
	amiga_membarrier();

	/*
	 * restrict transfer count to maximum
	 */
	if (dev->sc_tcnt > gtsc_maxdma)
		dev->sc_tcnt = gtsc_maxdma;
#if 1
	if((u_int)dev->sc_cur->dc_addr & dev->sc_dmamask)
		printf(" tcnt %ld\n", dev->sc_tcnt);
#endif
	return(dev->sc_tcnt);
}

void
gtsc_dmastop(struct sbic_softc *dev)
{
	volatile struct sdmac *sdp;
	int s;

	sdp = dev->sc_cregs;

#ifdef DEBUG
	if (gtsc_debug & DDB_FOLLOW)
		printf("gtsc_dmastop()\n");
#endif
	if (dev->sc_dmacmd) {
		/*
		 * clear possible interrupt and stop DMA
		 */
		s = splbio();
		sdp->CNTR &= ~GVP_CNTR_INT_P;
		amiga_membarrier();
		sdp->SP_DMA = 1;
		amiga_membarrier();
		dev->sc_dmacmd = 0;
		splx(s);
	}
}

int
gtsc_dmaintr(void *arg)
{
	struct sbic_softc *dev = arg;
	volatile struct sdmac *sdp;
	int stat;

	sdp = dev->sc_cregs;
	stat = sdp->CNTR;
	amiga_membarrier();
	if ((stat & GVP_CNTR_INT_P) == 0)
		return (0);
#ifdef DEBUG
	if (gtsc_debug & DDB_FOLLOW)
		printf("%s: dmaintr 0x%x\n", dev->sc_dev.dv_xname, stat);
#endif
	if (dev->sc_flags & SBICF_INTR)
		if (sbicintr(dev))
			return (1);
	return(0);
}


int
gtsc_dmanext(struct sbic_softc *dev)
{
	volatile struct sdmac *sdp;

	sdp = dev->sc_cregs;

	if (dev->sc_cur > dev->sc_last) {
		/* shouldn't happen !! */
		printf("gtsc_dmanext at end !!!\n");
		gtsc_dmastop(dev);
		return(0);
	}
	/*
	 * clear possible interrupt and stop DMA
	 */
	sdp->CNTR &= ~GVP_CNTR_INT_P;
	amiga_membarrier();
	sdp->SP_DMA = 1;
	amiga_membarrier();

	sdp->CNTR = dev->sc_dmacmd;
	amiga_membarrier();
	sdp->ACR = (u_int) dev->sc_cur->dc_addr;
	amiga_membarrier();
	if (dev->gtsc_bankmask) {
		sdp->bank =
		    dev->gtsc_bankmask & ((u_int)dev->sc_cur->dc_addr >> 18);
		amiga_membarrier();
	}
	sdp->ST_DMA = 1;
	amiga_membarrier();

	dev->sc_tcnt = dev->sc_cur->dc_count << 1;
	if (dev->sc_tcnt > gtsc_maxdma)
		dev->sc_tcnt = gtsc_maxdma;
#ifdef DEBUG
	if (gtsc_debug & DDB_FOLLOW)
		printf("gtsc_dmanext ret: %ld\n", dev->sc_tcnt);
#endif
	return(dev->sc_tcnt);
}

#ifdef DEBUG
void
gtsc_dump(void)
{
	extern struct cfdriver gtsc_cd;
	struct sbic_softc *sc;
	int i;

	for (i = 0; i < gtsc_cd.cd_ndevs; ++i) {
		sc = device_lookup_private(&gtsc_cd, i);
		if (sc != NULL)
			sbic_dump(sc);
	}
}
#endif
