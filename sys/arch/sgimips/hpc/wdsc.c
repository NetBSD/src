/*	$NetBSD: wdsc.c,v 1.33.14.1 2015/04/06 15:18:01 skrll Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: wdsc.c,v 1.33.14.1 2015/04/06 15:18:01 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/buf.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#include <machine/cpu.h>
#include <sys/bus.h>
#include <machine/autoconf.h>
#include <machine/machtype.h>
#include <machine/sysconf.h>

#include <sgimips/hpc/hpcvar.h>
#include <sgimips/hpc/hpcreg.h>
#include <sgimips/hpc/hpcdma.h>

#include <dev/ic/wd33c93reg.h>
#include <dev/ic/wd33c93var.h>

#include <opt_kgdb.h>
#include <sys/kgdb.h>

struct wdsc_softc {
	struct wd33c93_softc	sc_wd33c93; /* Must be first */
	struct evcnt		sc_intrcnt; /* Interrupt counter */
	bus_dma_tag_t		sc_dmat;
	bus_dmamap_t		sc_dmamap;
	int			sc_flags;
#define	WDSC_DMA_ACTIVE			0x1
#define	WDSC_DMA_MAPLOADED		0x2
	struct hpc_dma_softc	sc_hpcdma;
};


int	wdsc_match(device_t, cfdata_t, void *);
void	wdsc_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(wdsc, sizeof(struct wdsc_softc),
    wdsc_match, wdsc_attach, NULL, NULL);

int	wdsc_dmasetup(struct wd33c93_softc *, void ** ,size_t *, int, size_t *);
int	wdsc_dmago(struct wd33c93_softc *);
void	wdsc_dmastop(struct wd33c93_softc *);
void	wdsc_reset(struct wd33c93_softc *);
int	wdsc_dmaintr(void *);
int	wdsc_scsiintr(void *);

/*
 * Match for SCSI devices on the onboard and GIO32 adapter WD33C93 chips
 */
int
wdsc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct hpc_attach_args *haa = aux;

	if (strcmp(haa->ha_name, cf->cf_name) == 0) {
		vaddr_t reset, asr, reg;

		reset = MIPS_PHYS_TO_KSEG1(haa->ha_sh + haa->ha_dmaoff +
		    haa->hpc_regs->scsi0_ctl);
		asr = MIPS_PHYS_TO_KSEG1(haa->ha_sh + haa->ha_devoff);

		if (platform.badaddr((void *)reset, sizeof(reset)))
			return 0;

		*(volatile uint32_t *)reset = haa->hpc_regs->scsi_dmactl_reset;
		delay(1000);
		*(volatile uint32_t *)reset = 0x0;

		if (platform.badaddr((void *)asr, sizeof(asr)))
			return 0;

		reg = *(volatile uint32_t *)asr;
		if (haa->hpc_regs->revision == 3) {
			if ((reg & 0xff) == SBIC_ASR_INT)
				return 1;
		} else {
			if (((reg >> 8) & 0xff) == SBIC_ASR_INT)
				return 1;
		}
	}

	return 0;
}

/*
 * Attach the wdsc driver
 */
void
wdsc_attach(device_t parent, device_t self, void *aux)
{
	struct wdsc_softc *wsc = device_private(self);
	struct wd33c93_softc *sc = &wsc->sc_wd33c93;
	struct hpc_attach_args *haa = aux;
	int err;

	sc->sc_dev = self;
	sc->sc_regt = haa->ha_st;
	wsc->sc_dmat = haa->ha_dmat;

	wsc->sc_hpcdma.hpc = haa->hpc_regs;

	if ((err = bus_space_subregion(haa->ha_st, haa->ha_sh,
	    haa->ha_devoff + 0 + 3, 1, &sc->sc_asr_regh)) != 0) {
		printf(": unable to map asr reg, err=%d\n", err);
		return;
	}

	if ((err = bus_space_subregion(haa->ha_st, haa->ha_sh,
	    haa->ha_devoff + 4 + 3,  1, &sc->sc_data_regh)) != 0) {
		printf(": unable to map data reg, err=%d\n", err);
		return;
	}

	if (bus_dmamap_create(wsc->sc_dmat, MAXPHYS,
	    wsc->sc_hpcdma.hpc->scsi_dma_segs,
	    wsc->sc_hpcdma.hpc->scsi_dma_segs_size,
	    wsc->sc_hpcdma.hpc->scsi_dma_segs_size,
	    BUS_DMA_WAITOK, &wsc->sc_dmamap) != 0) {
		printf(": failed to create dmamap\n");
		return;
	}

	sc->sc_dmasetup = wdsc_dmasetup;
	sc->sc_dmago    = wdsc_dmago;
	sc->sc_dmastop  = wdsc_dmastop;
	sc->sc_reset	= wdsc_reset;

	sc->sc_adapter.adapt_request = wd33c93_scsi_request;
	sc->sc_adapter.adapt_minphys = minphys;

	sc->sc_id = 0;					/* Host ID = 0 */
	sc->sc_clkfreq = 200;				/* 20MHz */
	sc->sc_dmamode = SBIC_CTL_BURST_DMA;

	evcnt_attach_dynamic(&wsc->sc_intrcnt, EVCNT_TYPE_INTR, NULL,
	    device_xname(sc->sc_dev), "intr");

	if ((cpu_intr_establish(haa->ha_irq, IPL_BIO,
	     wdsc_scsiintr, wsc)) == NULL) {
		printf(": unable to establish interrupt!\n");
		return;
	}

	hpcdma_init(haa, &wsc->sc_hpcdma, wsc->sc_hpcdma.hpc->scsi_dma_segs);
	wd33c93_attach(sc);
}

/*
 * Prime the hardware for a DMA transfer
 *
 * Requires splbio() interrupts to be disabled by the caller
 */
int
wdsc_dmasetup(struct wd33c93_softc *sc, void **addr, size_t *len, int datain,
    size_t *dmasize)
{
	struct wdsc_softc *wsc = (struct wdsc_softc *)sc;
	struct hpc_dma_softc *dsc = &wsc->sc_hpcdma;
	int count, err;
	void *vaddr;

	KASSERT((wsc->sc_flags & WDSC_DMA_ACTIVE) == 0);

	vaddr = *addr;
	count = dsc->sc_dlen = *len;
	if (count) {
		KASSERT((wsc->sc_flags & WDSC_DMA_MAPLOADED) == 0);

		/* Build list of physical addresses for this transfer */
		if ((err = bus_dmamap_load(wsc->sc_dmat, wsc->sc_dmamap,
		    vaddr, count, NULL /* kernel address */,
		    BUS_DMA_NOWAIT)) != 0)
			panic("%s: bus_dmamap_load err=%d",
			    device_xname(sc->sc_dev), err);

		hpcdma_sglist_create(dsc, wsc->sc_dmamap);
		wsc->sc_flags |= WDSC_DMA_MAPLOADED;

		if (datain) {
			dsc->sc_dmacmd =
			    wsc->sc_hpcdma.hpc->scsi_dma_datain_cmd;
			dsc->sc_flags |= HPCDMA_READ;
		} else {
			dsc->sc_dmacmd =
			    wsc->sc_hpcdma.hpc->scsi_dma_dataout_cmd;
			dsc->sc_flags &= ~HPCDMA_READ;
		}
	}
	return count;
}

/*
 * Prime the hardware for the next DMA transfer
 */
int
wdsc_dmago(struct wd33c93_softc *sc)
{
	struct wdsc_softc *wsc = (struct wdsc_softc *)sc;
	struct hpc_dma_softc *dsc = &wsc->sc_hpcdma;

	if (dsc->sc_dlen == 0)
		return 0;

	KASSERT((wsc->sc_flags & WDSC_DMA_ACTIVE) == 0);
	KASSERT((wsc->sc_flags & WDSC_DMA_MAPLOADED));

	wsc->sc_flags |= WDSC_DMA_ACTIVE;

	bus_dmamap_sync(wsc->sc_dmat, wsc->sc_dmamap,
	    0, wsc->sc_dmamap->dm_mapsize,
	    (dsc->sc_flags & HPCDMA_READ) ?
	    BUS_DMASYNC_PREREAD : BUS_DMASYNC_PREWRITE);

	hpcdma_cntl(dsc, dsc->sc_dmacmd);	/* Thunderbirds are go! */

	return wsc->sc_dmamap->dm_mapsize;
}

/*
 * Stop DMA and unload active DMA maps
 */
void
wdsc_dmastop(struct wd33c93_softc *sc)
{
	struct wdsc_softc *wsc = (struct wdsc_softc *)sc;
	struct hpc_dma_softc *dsc = &wsc->sc_hpcdma;

	if (wsc->sc_flags & WDSC_DMA_ACTIVE) {
		if (dsc->sc_flags & HPCDMA_READ)
			hpcdma_flush(dsc);
		hpcdma_cntl(dsc, 0);	/* Stop DMA */
		bus_dmamap_sync(wsc->sc_dmat, wsc->sc_dmamap,
		    0, wsc->sc_dmamap->dm_mapsize,
		    (dsc->sc_flags & HPCDMA_READ) ?
		    BUS_DMASYNC_POSTREAD : BUS_DMASYNC_POSTWRITE);
	}
	if (wsc->sc_flags & WDSC_DMA_MAPLOADED)
		bus_dmamap_unload(wsc->sc_dmat, wsc->sc_dmamap);
	wsc->sc_flags &= ~(WDSC_DMA_ACTIVE | WDSC_DMA_MAPLOADED);
}

/*
 * Reset the controller.
 */
void
wdsc_reset(struct wd33c93_softc *sc)
{
	struct wdsc_softc *wsc = (struct wdsc_softc *)sc;
	struct hpc_dma_softc *dsc = &wsc->sc_hpcdma;

	hpcdma_reset(dsc);
}

/*
 * WD33c93 SCSI controller interrupt
 */
int
wdsc_scsiintr(void *arg)
{
	struct wdsc_softc *wsc = arg;
	struct wd33c93_softc *sc = &wsc->sc_wd33c93;
	int found;

	found = wd33c93_intr(sc);
	if (found)
		wsc->sc_intrcnt.ev_count++;
	return found;
}
