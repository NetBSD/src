/*	$NetBSD: hpcdma.c,v 1.4 2002/03/13 13:12:27 simonb Exp $	*/

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
 * Support for SCSI DMA provided by the HPC.
 *
 * Note: We use SCSI0 offsets, etc. here.  Since the layout of SCSI0
 * and SCSI1 are the same, this is no problem.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/buf.h>

#include <machine/bus.h>

#include <sgimips/hpc/hpcvar.h>
#include <sgimips/hpc/hpcreg.h>
#include <sgimips/hpc/hpcdma.h>

/*
 * Allocate DMA Chain descriptor list
 */
void
hpcdma_init(struct hpc_attach_args *haa, struct hpc_dma_softc *sc, int ndesc)
{
	bus_dma_segment_t seg;
	struct hpc_dma_desc *hdd;
	int rseg, allocsz;

	sc->sc_bst = haa->ha_st;
	sc->sc_dmat = haa->ha_dmat;
	sc->sc_ndesc = ndesc;
	sc->sc_flags = 0;

	if (bus_space_subregion(haa->ha_st, haa->ha_sh, haa->ha_dmaoff,
	    HPC_SCSI0_REGS_SIZE, &sc->sc_bsh) != 0) {
		printf(": can't map DMA registers\n");
		return;
	}

	/* Alloc 1 additional descriptor - needed for DMA bug fix */
	allocsz = sizeof(struct hpc_dma_desc) * (ndesc + 1);
	KASSERT(allocsz <= NBPG);

	if (bus_dmamap_create(sc->sc_dmat, NBPG, 1 /*seg*/,
			      NBPG, 0, BUS_DMA_WAITOK,
			      &sc->sc_dmamap) != 0) {
		printf(": failed to create dmamap\n");
		return;
	}

	/*
	 * Allocate a block of memory for dma chaining pointers
	 */
	if (bus_dmamem_alloc(sc->sc_dmat, allocsz, 0, 0,
			     &seg, 1, &rseg, BUS_DMA_NOWAIT)) {
		printf(": can't allocate sglist\n");
		return;
	}
	/* Map pages into kernel memory */
	if (bus_dmamem_map(sc->sc_dmat, &seg, rseg, allocsz,
			   (caddr_t *)&hdd, BUS_DMA_NOWAIT)) {
		printf(": can't map sglist\n");
		bus_dmamem_free(sc->sc_dmat, &seg, rseg);
		return;
	}

	if (bus_dmamap_load(sc->sc_dmat, sc->sc_dmamap, hdd,
			    allocsz, NULL, BUS_DMA_NOWAIT)) {
		printf(": can't load sglist\n");
		return;
	}

	sc->sc_desc_kva = hdd;
	sc->sc_desc_pa = (void *) sc->sc_dmamap->dm_segs[0].ds_addr;
}


void
hpcdma_sglist_create(struct hpc_dma_softc *sc, bus_dmamap_t dmamap)
{
	struct hpc_dma_desc *hva, *hpa;
	bus_dma_segment_t *segp;
	int i;

	KASSERT(dmamap->dm_nsegs <= sc->sc_ndesc);

	hva  = sc->sc_desc_kva;
	hpa  = sc->sc_desc_pa;
	segp = dmamap->dm_segs;

#ifdef DMA_DEBUG
	printf("DMA_SGLIST<");
#endif
	for (i = dmamap->dm_nsegs; i; i--) {
#ifdef DMA_DEBUG
		printf("%p:%ld, ", (void *)segp->ds_addr, segp->ds_len);
#endif
		hva->hdd_bufptr = segp->ds_addr;
		hva->hdd_ctl    = segp->ds_len;
		hva->hdd_descptr = (u_int32_t) ++hpa;
		++hva; ++segp;
	}
	/* Work around HPC3 DMA bug */
	hva->hdd_bufptr  = 0;
	hva->hdd_ctl     = HDD_CTL_EOCHAIN;
	hva->hdd_descptr = 0;
	hva++;
#ifdef DMA_DEBUG
	printf(">\n");
#endif
	bus_dmamap_sync(sc->sc_dmat, sc->sc_dmamap,
	    0, sc->sc_dmamap->dm_mapsize,
	    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

	/* Load DMA Descriptor list */
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, HPC_SCSI0_NDBP,
			    (u_int32_t)sc->sc_desc_pa);
}

void
hpcdma_cntl(struct hpc_dma_softc *sc, uint32_t mode)
{

	bus_space_write_4(sc->sc_bst, sc->sc_bsh, HPC_SCSI0_CTL, mode);
}

void
hpcdma_reset(struct hpc_dma_softc *sc)
{

	bus_space_write_4(sc->sc_bst, sc->sc_bsh, HPC_SCSI0_CTL,
	    HPC_DMACTL_RESET);
	delay(100);
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, HPC_SCSI0_CTL, 0);
	delay(1000);
}

void
hpcdma_flush(struct hpc_dma_softc *sc)
{
	u_int32_t	mode;

	mode = bus_space_read_4(sc->sc_bst, sc->sc_bsh, HPC_SCSI0_CTL);
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, HPC_SCSI0_CTL,
	    			mode | HPC_DMACTL_FLUSH);

	/* Wait for Active bit to drop */
	while (bus_space_read_4(sc->sc_bst, sc->sc_bsh, HPC_SCSI0_CTL) &
	    HPC_DMACTL_ACTIVE) {
		bus_space_barrier(sc->sc_bst, sc->sc_bsh, HPC_SCSI0_CTL, 4,
		    BUS_SPACE_BARRIER_READ);
	}
}
