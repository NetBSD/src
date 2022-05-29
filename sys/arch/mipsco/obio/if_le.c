/*	$NetBSD: if_le.c,v 1.14 2022/05/29 10:45:05 rin Exp $	*/

/*-
 * Copyright (c) 1996, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Adam Glass, Gordon W. Ross and Wayne Knowles
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
__KERNEL_RCSID(0, "$NetBSD: if_le.c,v 1.14 2022/05/29 10:45:05 rin Exp $");

#include "opt_inet.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/syslog.h>
#include <sys/socket.h>
#include <sys/device.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/if_inarp.h>
#endif

#include <machine/cpu.h>
#include <machine/mainboard.h>
#include <machine/autoconf.h>
#include <machine/bus.h>

#include <dev/ic/lancereg.h>
#include <dev/ic/lancevar.h>
#include <dev/ic/am7990reg.h>
#include <dev/ic/am7990var.h>

/*
 * LANCE registers.
 */

#define	LEREG1_RDP	2	/* Offset to LANCE data register */
#define	LEREG1_RAP	6	/* Offset to LANCE address register */

/*
 * Ethernet software status per interface.
 * The real stuff is in dev/ic/am7990var.h
 */
struct	le_softc {
	struct am7990_softc	sc_am7990;	/* glue to MI code */
	bus_space_tag_t		sc_bustag;
	bus_dma_tag_t		sc_dmatag;
	bus_space_handle_t	sc_reg;		/* LANCE registers */
        bus_dmamap_t		sc_dmamap;
	struct evcnt		sc_intrcnt;	/* Interrupt event counter */
};

static int	le_match(device_t, cfdata_t, void *);
static void	le_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(le, sizeof(struct le_softc),
    le_match, le_attach, NULL, NULL);

static int le_attached;

#if defined(_KERNEL_OPT)
#include "opt_ddb.h"
#endif

#ifdef DDB
#define	integrate
#define hide
#else
#define	integrate	static inline
#define hide		static
#endif

int le_intr(void *);

hide void lewrcsr(struct lance_softc *, uint16_t, uint16_t);
hide uint16_t lerdcsr(struct lance_softc *, uint16_t);  

hide void
lewrcsr(struct lance_softc *sc, uint16_t port, uint16_t val)
{
	struct le_softc *lesc = (struct le_softc *)sc;

	bus_space_write_2(lesc->sc_bustag, lesc->sc_reg, LEREG1_RAP, port);
	bus_space_write_2(lesc->sc_bustag, lesc->sc_reg, LEREG1_RDP, val);
}

hide uint16_t
lerdcsr(struct lance_softc *sc, uint16_t port)
{
	struct le_softc *lesc = (struct le_softc *)sc;

	bus_space_write_2(lesc->sc_bustag, lesc->sc_reg, LEREG1_RAP, port);
	return (bus_space_read_2(lesc->sc_bustag, lesc->sc_reg, LEREG1_RDP));
} 

int
le_match(device_t parent, cfdata_t cf, void *aux)
{
	struct confargs *ca = aux;
	int addr;

	if (strcmp(ca->ca_name, "le"))
		return 0;

	if (le_attached)
		return 0;

	addr = LANCE_PORT;
	if (badaddr((void *)addr, 1))
		return 0;

	return 1;
}

#define	LE_MEMSIZE	(32*1024)

void
le_attach(device_t parent, device_t self, void *aux)
{
	struct le_softc *lesc = device_private(self);
	struct lance_softc *sc = &lesc->sc_am7990.lsc;
	struct confargs *ca = aux;

	bus_dma_tag_t dmat;
	bus_dma_segment_t seg;
	int rseg;
	uint8_t *id;
	int i;
	void *kvaddr;

	sc->sc_dev = self;
	id = (uint8_t *)ETHER_ID;
	lesc->sc_bustag = ca->ca_bustag;
	dmat = lesc->sc_dmatag = ca->ca_dmatag;

	if (bus_space_map(ca->ca_bustag, ca->ca_addr,
			  8,	/* size */
			  BUS_SPACE_MAP_LINEAR,
			  &lesc->sc_reg) != 0) {
		aprint_error(": cannot map registers\n");
		return;
	}

	/*
	 * Allocate a physically contiguous DMA area for the chip.
	 */
	if (bus_dmamem_alloc(dmat, LE_MEMSIZE, 0, 0, &seg, 1,
			     &rseg, BUS_DMA_NOWAIT)) {
		aprint_error(": can't allocate DMA area\n");
		goto bad_bsunmap;
	}
	/* Map pages into kernel memory */
	if (bus_dmamem_map(dmat, &seg, rseg, LE_MEMSIZE,
	    &kvaddr, BUS_DMA_NOWAIT|BUS_DMA_COHERENT)) {
		aprint_error(": can't map DMA area\n");
		goto bad_free;
	}
	/* Build DMA map so we can get physical address */
	if (bus_dmamap_create(dmat, LE_MEMSIZE, 1, LE_MEMSIZE,
			      0, BUS_DMA_NOWAIT, &lesc->sc_dmamap)) {
		aprint_error(": can't create DMA map\n");
		goto bad_unmap;
	}
	if (bus_dmamap_load(dmat, lesc->sc_dmamap, kvaddr, LE_MEMSIZE,
			    NULL, BUS_DMA_NOWAIT)) {
		aprint_error(": can't load DMA map\n");
		goto bad_destroy;
	}

	sc->sc_memsize = LE_MEMSIZE;	/* 16K Buffer space*/
	sc->sc_mem  = (void *)MIPS_PHYS_TO_KSEG1(kvaddr);
	sc->sc_addr = lesc->sc_dmamap->dm_segs[0].ds_addr;

	sc->sc_conf3 = LE_C3_BSWP;

	/* Copy Ethernet hardware address from NVRAM */
	for (i = 0; i < ETHER_ADDR_LEN; i++)
		sc->sc_enaddr[i] = id[i * 4 + 3];	/* XXX */

	sc->sc_copytodesc = lance_copytobuf_contig;
	sc->sc_copyfromdesc = lance_copyfrombuf_contig;
	sc->sc_copytobuf = lance_copytobuf_contig;
	sc->sc_copyfrombuf = lance_copyfrombuf_contig;
	sc->sc_zerobuf = lance_zerobuf_contig;

	sc->sc_rdcsr = lerdcsr;
	sc->sc_wrcsr = lewrcsr;
	sc->sc_hwinit = NULL;

	evcnt_attach_dynamic(&lesc->sc_intrcnt, EVCNT_TYPE_INTR, NULL,
			     device_xname(self), "intr");
	bus_intr_establish(lesc->sc_bustag, SYS_INTR_ETHER, 0, 0,
			   le_intr, lesc);

	am7990_config(&lesc->sc_am7990);
	return;

 bad_destroy:
	bus_dmamap_destroy(dmat, lesc->sc_dmamap);
 bad_unmap:
	bus_dmamem_unmap(dmat, kvaddr, LE_MEMSIZE);
 bad_free:
	bus_dmamem_free(dmat, &seg, rseg);
 bad_bsunmap:
	bus_space_unmap(ca->ca_bustag, lesc->sc_reg, 8);
}

int
le_intr(void *arg)
{
	struct le_softc *lesc = arg;

	lesc->sc_intrcnt.ev_count++;
	return am7990_intr(&lesc->sc_am7990);
}
