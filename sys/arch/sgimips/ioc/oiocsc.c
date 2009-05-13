/*	$NetBSD: oiocsc.c,v 1.1.6.2 2009/05/13 17:18:19 jym Exp $	*/

/*
 * Copyright (c) 2009 Stephen M. Rumble
 * Copyright (c) 2001 Wayne Knowles
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Wayne Knowles
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: oiocsc.c,v 1.1.6.2 2009/05/13 17:18:19 jym Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/buf.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/autoconf.h>
#include <machine/machtype.h>
#include <machine/sysconf.h>

#include <sgimips/ioc/oiocreg.h>
#include <sgimips/ioc/oiocvar.h>

#include <dev/ic/wd33c93reg.h>
#include <dev/ic/wd33c93var.h>

#include <opt_kgdb.h>
#include <sys/kgdb.h>

struct oiocsc_softc {
	struct wd33c93_softc	sc_wd33c93; /* Must be first */
	struct evcnt		sc_intrcnt; /* Interrupt counter */
	bus_space_handle_t	sc_sh;
	bus_space_tag_t		sc_st;
	bus_dma_tag_t		sc_dmat;
	bus_dmamap_t		sc_dmamap;
	int			sc_flags;
#define	WDSC_DMA_ACTIVE			0x1
#define	WDSC_DMA_MAPLOADED		0x2
	struct oioc_dma_softc {
		bus_space_tag_t		sc_bst;
		bus_space_handle_t	sc_bsh;
		bus_dma_tag_t		sc_dmat;

		uint32_t		sc_flags;
		uint32_t		sc_dmalow;
		int			sc_ndesc;
		bus_dmamap_t		sc_dmamap;
		ssize_t			sc_dlen;	/* # bytes transfered */
	} sc_oiocdma;
};


void	oiocsc_attach	(device_t, device_t, void *);
int	oiocsc_match	(device_t, struct cfdata *, void *);

CFATTACH_DECL_NEW(oiocsc, sizeof(struct oiocsc_softc),
    oiocsc_match, oiocsc_attach, NULL, NULL);

int	oiocsc_dmasetup	(struct wd33c93_softc *, void **, size_t *,
				int, size_t *);
int	oiocsc_dmago	(struct wd33c93_softc *);
void	oiocsc_dmastop	(struct wd33c93_softc *);
void	oiocsc_reset	(struct wd33c93_softc *);
int	oiocsc_dmaintr	(void *);
int	oiocsc_scsiintr	(void *);

/*
 * Always present on IP4, IP6, IP10.
 */
int
oiocsc_match(device_t parent, struct cfdata *cf, void *aux)
{
	struct oioc_attach_args *oa = aux;

	if (strcmp(oa->oa_name, cf->cf_name) == 0)
		return (1);

	return (0);
}

/*
 * Attach the wdsc driver
 */
void
oiocsc_attach(device_t parent, device_t self, void *aux)
{
	struct oiocsc_softc *osc = device_private(self);
	struct wd33c93_softc *sc = &osc->sc_wd33c93; 
	struct oioc_attach_args *oa = aux;
	int err;

	sc->sc_dev   = self;
	sc->sc_regt  = oa->oa_st;
	osc->sc_st   = oa->oa_st;
	osc->sc_sh   = oa->oa_sh;
	osc->sc_dmat = oa->oa_dmat;

	if ((err = bus_space_subregion(oa->oa_st, oa->oa_sh, OIOC_WD33C93_ASR,
	    OIOC_WD33C93_ASR_SIZE, &sc->sc_asr_regh)) != 0) {
		printf(": unable to map regs, err=%d\n", err);
		return;
	}

	if ((err = bus_space_subregion(oa->oa_st, oa->oa_sh, OIOC_WD33C93_DATA,
	    OIOC_WD33C93_DATA_SIZE, &sc->sc_data_regh)) != 0) {
		printf(": unable to map regs, err=%d\n", err);
		return;
	}

	if (bus_dmamap_create(osc->sc_dmat,
	    OIOC_SCSI_DMA_NSEGS * PAGE_SIZE,
	    OIOC_SCSI_DMA_NSEGS, PAGE_SIZE, PAGE_SIZE,
	    BUS_DMA_WAITOK, &osc->sc_dmamap) != 0) {
		printf(": failed to create dmamap\n");
		return;
	}

	sc->sc_dmasetup = oiocsc_dmasetup;
	sc->sc_dmago    = oiocsc_dmago;
	sc->sc_dmastop  = oiocsc_dmastop;
	sc->sc_reset	= oiocsc_reset;

	sc->sc_adapter.adapt_request = wd33c93_scsi_request;
	sc->sc_adapter.adapt_minphys = minphys;

	sc->sc_id = 0;					/* Host ID = 0 */
	sc->sc_clkfreq = 100;				/* 10MHz */

	/* Disable DMA - it's not ready for prime time, see oiocsc_dmasetup */
#if 0
	sc->sc_dmamode = (oa->oa_burst_dma) ?
	    SBIC_CTL_BURST_DMA : SBIC_CTL_DMA;
#else
	sc->sc_dmamode = 0;
#endif

	evcnt_attach_dynamic(&osc->sc_intrcnt, EVCNT_TYPE_INTR, NULL,
	    device_xname(sc->sc_dev), "intr");

	if ((cpu_intr_establish(oa->oa_irq, IPL_BIO,
	     oiocsc_scsiintr, sc)) == NULL) {
		printf(": unable to establish interrupt!\n");
		return;
	}

	wd33c93_attach(sc);
}

/*
 * Prime the hardware for a DMA transfer
 *
 * Requires splbio() interrupts to be disabled by the caller
 *
 * XXX- I'm not sure if this works properly yet. Primarily, I'm not sure
 *      that all ds_addr's after the first will be page aligned. If they're
 *      not, we apparently cannot use this DMA engine...
 *
 *      Unfortunately, I'm getting mutex panics while testing with EFS (haven't
 *      tried FFS), so I cannot yet confirm whether this works or not.
 */
int
oiocsc_dmasetup(struct wd33c93_softc *dev, void **addr, size_t *len, int datain,
    size_t *dmasize)
{
	struct oiocsc_softc *osc = (void *)dev;
	struct oioc_dma_softc *dsc = &osc->sc_oiocdma;
	int count, err, i;
	void *vaddr;

	KASSERT((osc->sc_flags & WDSC_DMA_ACTIVE) == 0);

	vaddr = *addr;
	count = dsc->sc_dlen = *len;

	if (count) {
		bus_dmamap_t dmamap = osc->sc_dmamap;

		KASSERT((osc->sc_flags & WDSC_DMA_MAPLOADED) == 0);

		/* Build list of physical addresses for this transfer */
		if ((err = bus_dmamap_load(osc->sc_dmat, osc->sc_dmamap,
				vaddr, count,
				NULL /* kernel address */,
				BUS_DMA_NOWAIT)) != 0)
			panic("%s: bus_dmamap_load err=%d",
			      device_xname(dev->sc_dev), err);

		/*
		 * We can map the contiguous buffer with up to 256 pages.
		 * The DMA low address register contains a 12-bit offset for
		 * the first page (in case the buffer isn't aligned). The 256
		 * high registers contain 16 bits each for page numbers.
		 *
		 * We will fill in the high registers here. The low register
		 * fires off the DMA engine and is set in oiocsc_dmago.
		 */
		dsc->sc_dmalow = dmamap->dm_segs[0].ds_addr &
		    OIOC_SCSI_DMA_LOW_ADDR_MASK;

		KASSERT(dmamap->dm_nsegs <= OIOC_SCSI_DMA_NSEGS);

		for (i = 0; i < dmamap->dm_nsegs; i++) { 
			uint16_t pgnum;

			pgnum = dmamap->dm_segs[i].ds_addr >>
			    OIOC_SCSI_DMA_HIGH_SHFT;

			bus_space_write_2(osc->sc_st, osc->sc_sh,
			    OIOC_SCSI_DMA_HIGH(i), pgnum);
		}

		osc->sc_flags |= WDSC_DMA_MAPLOADED;

		if (datain)
			dsc->sc_dmalow |= OIOC_SCSI_DMA_LOW_ADDR_DMADIR;
	}

	return (count);
}

/*
 * Prime the hardware for the next DMA transfer
 */
int
oiocsc_dmago(struct wd33c93_softc *dev)
{
	struct oiocsc_softc *osc = (void *)dev;
	struct oioc_dma_softc *dsc = &osc->sc_oiocdma;

	if (dsc->sc_dlen == 0)
		return(0);

	KASSERT((osc->sc_flags & WDSC_DMA_ACTIVE) == 0);
	KASSERT((osc->sc_flags & WDSC_DMA_MAPLOADED));

	osc->sc_flags |= WDSC_DMA_ACTIVE;

	bus_dmamap_sync(osc->sc_dmat, osc->sc_dmamap, 0,
	    		osc->sc_dmamap->dm_mapsize,
			BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

	/* Blastoff! */
	bus_space_write_2(osc->sc_st, osc->sc_sh,
	    OIOC_SCSI_DMA_LOW, dsc->sc_dmalow);

	return(osc->sc_dmamap->dm_mapsize);
}

/*
 * Stop DMA and unload active DMA maps
 */
void
oiocsc_dmastop(struct wd33c93_softc *dev)
{
	struct oiocsc_softc *osc = (void *)dev;

	if (osc->sc_flags & WDSC_DMA_ACTIVE) {
		/* Stop DMA, flush and sync */
		bus_space_write_4(osc->sc_st, osc->sc_sh,
		    OIOC_SCSI_DMA_FLUSH, 0);
		bus_dmamap_sync(osc->sc_dmat, osc->sc_dmamap, 0,
		    osc->sc_dmamap->dm_mapsize,
		    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);
	}
	if (osc->sc_flags & WDSC_DMA_MAPLOADED)
		bus_dmamap_unload(osc->sc_dmat, osc->sc_dmamap);
	osc->sc_flags &= ~(WDSC_DMA_ACTIVE | WDSC_DMA_MAPLOADED);
}

/*
 * Reset the controller.
 */
void
oiocsc_reset(struct wd33c93_softc *dev)
{
	struct oiocsc_softc *osc = (void *)dev;

	/* hard reset the chip */
	bus_space_read_4(osc->sc_st, osc->sc_sh, OIOC_SCSI_RESET_ON);
	delay(1000);
	bus_space_read_4(osc->sc_st, osc->sc_sh, OIOC_SCSI_RESET_OFF);
	delay(1000);
}

/*
 * WD33c93 SCSI controller interrupt
 */
int
oiocsc_scsiintr(void *arg)
{
	struct wd33c93_softc *dev = arg;
	struct oiocsc_softc *osc = arg;
	int found;

	found = wd33c93_intr(dev);
	if (found)
		osc->sc_intrcnt.ev_count++;
	return(found);
}
