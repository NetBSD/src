/*	$NetBSD: wdsc.c,v 1.1 2001/08/19 03:16:22 wdk Exp $	*/

/*
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

/*
 * TODO:
 *
 *    Support for 2nd SCSI controller
 *    evcnt(9) hooks
 *    remove struct dma_chain
 *    dma{setup,stop,go} API similar to NCR93c9x MI driver
 *    improve hpcdma functions
 *    cleanup softc stuff
 */

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

#include <sgimips/hpc/hpcvar.h>
#include <sgimips/hpc/hpcreg.h>
#include <sgimips/hpc/hpcdma.h>

#include <sgimips/hpc/sbicreg.h>
#include <sgimips/hpc/sbicvar.h>

struct wdsc_softc {
	struct sbic_softc	sc_sbic; /* Must be first */

	struct hpc_dma_softc	sc_hpcdma;
};


void    wdsc_attach __P((struct device *, struct device *, void *));
int     wdsc_match  __P((struct device *, struct cfdata *, void *));

struct cfattach wdsc_ca = {
	sizeof(struct wdsc_softc), wdsc_match, wdsc_attach
};

extern struct cfdriver wdsc_cd;

void    wdsc_enintr     __P((struct sbic_softc *));
int     wdsc_dmasetup   __P((struct sbic_softc *, bus_dmamap_t, int));
int     wdsc_dmago      __P((struct sbic_softc *));
void    wdsc_dmastop    __P((struct sbic_softc *));
int     wdsc_dmaintr    __P((void *));
int     wdsc_scsiintr   __P((void *));

#define MAX_SCSI_XFER   (512*1024)
#define MAX_SEG_SZ	8192
#define	MAX_DMA_SZ	MAX_SCSI_XFER
#define	DMA_SEGS	(MAX_DMA_SZ/MAX_SEG_SZ)

/*
 * Match for SCSI devices on the onboard WD33C93 chip
 */
int
wdsc_match(pdp, cf, auxp)
    struct device *pdp;
    struct cfdata *cf;
    void *auxp;
{
    struct hpc_attach_args *haa = auxp;

    if (strcmp(haa->ha_name, wdsc_cd.cd_name))
	return (0);
    return (1);
}

/*
 * Attach the wdsc driver
 */
void
wdsc_attach(pdp, dp, auxp)
    struct device *pdp, *dp;
    void *auxp;
{
    struct sbic_softc *sc = (void *)dp;
    struct wdsc_softc *wdsc = (void *)dp;
    struct hpc_attach_args *haa = auxp;
    int err;

    sc->sc_regt = haa->ha_iot;
    sc->sc_dmat = haa->ha_dmat;

    if ((err = bus_space_subregion(haa->ha_iot, haa->ha_ioh,
					HPC_SCSI0_DEVREGS,
	     				HPC_SCSI0_DEVREGS_SIZE,
	     				&sc->sc_regh)) != 0) {
		printf(": unable to map WD33C93 regs, err=%d\n", err);
		goto fail;
    }

    if (bus_dmamap_create(sc->sc_dmat, MAX_DMA_SZ,
			      DMA_SEGS, MAX_SEG_SZ, MAX_SEG_SZ,
			      BUS_DMA_WAITOK,
			      &sc->sc_dmamap) != 0) {
		printf(": failed to create dmamap\n");
		goto fail;
    }

    hpcdma_init(haa, &wdsc->sc_hpcdma, DMA_SEGS);

    sc->sc_enintr   = wdsc_enintr;
    sc->sc_dmasetup = wdsc_dmasetup;
    sc->sc_dmago    = wdsc_dmago;
    sc->sc_dmastop  = wdsc_dmastop;

    sc->sc_adapter.adapt_dev = &sc->sc_dev;
    sc->sc_adapter.adapt_nchannels = 1;
    sc->sc_adapter.adapt_openings = 7; 
    sc->sc_adapter.adapt_max_periph = 1;
    sc->sc_adapter.adapt_ioctl = NULL; 
    sc->sc_adapter.adapt_minphys = minphys;
    sc->sc_adapter.adapt_request = sbic_scsi_request;
    sc->sc_channel.chan_adapter = &sc->sc_adapter;
    sc->sc_channel.chan_bustype = &scsi_bustype;
    sc->sc_channel.chan_channel = 0;
    sc->sc_channel.chan_ntargets = 8;
    sc->sc_channel.chan_nluns = 8;
    sc->sc_channel.chan_id = 7;

    printf(": WD33C93 SCSI, target %d\n", sc->sc_channel.chan_id);

    /*
     * Controller clock frequency * 10
     */
    sc->sc_clkfreq = 200;

    /*
     * Initialise the hardware
     */
    sbicinit(sc);

    /* XXX: 1 = IRQ_LOCAL0 + 1 */
    if ((cpu_intr_establish(1, IPL_BIO, wdsc_scsiintr, sc)) == NULL) {
		printf(": unable to establish interrupt!\n");
		goto fail;
    }

    (void)config_found(dp, &sc->sc_channel, scsiprint);

fail:
    return;
}

/*
 * Enable DMA interrupts
 */
void
wdsc_enintr(dev)
    struct sbic_softc *dev;
{
    dev->sc_flags |= SBICF_INTR;
}

/*
 * Prime the hardware for a DMA transfer
 */
int
wdsc_dmasetup(dev, dmamap, flags)
    struct sbic_softc *dev;
    bus_dmamap_t dmamap;
    int flags;
{
    struct hpc_dma_softc *dsc = &((struct wdsc_softc *)dev)->sc_hpcdma;
    struct sbic_acb *acb = dev->sc_nexus;
    int     s;
    int     count, err;
    void   *vaddr;

    KASSERT((dsc->sc_flags & HPC_DMA_ACTIVE) == 0);

    vaddr = acb->sc_kv.dc_addr;
    count = acb->sc_kv.dc_count;

    if (count) {
	    s = splbio();

	    /* have dmamap for the transfering addresses */
	    if ((err=bus_dmamap_load(dev->sc_dmat, dev->sc_dmamap,
				vaddr, count,
				NULL /* kernel address */,   
				BUS_DMA_NOWAIT)) != 0)
		    panic("%s: bus_dmamap_load err=%d",
			  dev->sc_dev.dv_xname, err);
    
	    dev->sc_flags |= SBICF_DMALOADED; /* XXX - Move to MD */
	    dev->sc_flags |= SBICF_INTR;

	    hpcdma_sglist_create(dsc, dev->sc_dmamap);

	    dsc->sc_dmacmd = HPC_DMACTL_ACTIVE;	/* XXX - remove tests in MI */
	    if (flags & ACB_DATAIN) {
		    dsc->sc_flags  |= HPC_DMA_READ;
	    } else {
		    dsc->sc_dmacmd |= HPC_DMACTL_DIR;
		    dsc->sc_flags  &= ~HPC_DMA_READ;
	    }
	    splx(s);
    }
    return(count);
}

/*
 * Prime the hardware for the next DMA transfer
 */
int
wdsc_dmago(dev)
    struct sbic_softc *dev;
{
    struct hpc_dma_softc *dsc = &((struct wdsc_softc *)dev)->sc_hpcdma;

    if (dev->sc_tcnt == 0) {
	    return(0);
    }

    KASSERT((dsc->sc_flags & HPC_DMA_ACTIVE) == 0);
    dsc->sc_flags |= HPC_DMA_ACTIVE;

    bus_dmamap_sync(dev->sc_dmat, dev->sc_dmamap, 0, dev->sc_tcnt,
		BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

    hpcdma_cntl(dsc, dsc->sc_dmacmd);	/* Thunderbirds are go! */

    return(dev->sc_dmamap->dm_mapsize);
}

/*
 * Stop DMA, and disable interrupts
 */
void
wdsc_dmastop(dev)
    struct sbic_softc *dev;
{
    struct hpc_dma_softc *dsc = &((struct wdsc_softc *)dev)->sc_hpcdma;

    if (dsc->sc_flags & HPC_DMA_ACTIVE) {
	    if (dsc->sc_flags & HPC_DMA_READ)
		    hpcdma_flush(dsc);
	    hpcdma_cntl(dsc, 0);	/* Stop DMA */
	    bus_dmamap_sync(dev->sc_dmat, dev->sc_dmamap, 0,
		dev->sc_dmamap->dm_mapsize,
		BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);
	    dsc->sc_flags &= ~HPC_DMA_ACTIVE;
    }
    if (dev->sc_flags & SBICF_DMALOADED)
	    bus_dmamap_unload(dev->sc_dmat, dev->sc_dmamap);
}

/*
 * SCSI controller interrupt
 */
int
wdsc_scsiintr(arg)
    void *arg;
{
    struct sbic_softc *dev = arg;
    int found;

    found = sbicintr(dev);
    return(found);
}
