/*	$NetBSD: if_le.c,v 1.3 2001/05/30 12:28:46 mrg Exp $	*/

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

#include "opt_inet.h"
#include "bpfilter.h"

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

static int	le_match __P((struct device *, struct cfdata *, void *));
static void	le_attach __P((struct device *, struct device *, void *));

struct cfattach le_ca = {
	sizeof(struct le_softc), le_match, le_attach
};

#if defined(_KERNEL_OPT)
#include "opt_ddb.h"
#endif

#ifdef DDB
#define	integrate
#define hide
#else
#define	integrate	static __inline
#define hide		static
#endif

int le_intr  __P((void *));

hide void lewrcsr __P((struct lance_softc *, u_int16_t, u_int16_t));
hide u_int16_t lerdcsr __P((struct lance_softc *, u_int16_t));  

hide void
lewrcsr(sc, port, val)
	struct lance_softc *sc;
	u_int16_t port, val;
{
	struct le_softc *lesc = (struct le_softc *)sc;

	bus_space_write_2(lesc->sc_bustag, lesc->sc_reg, LEREG1_RAP, port);
	bus_space_write_2(lesc->sc_bustag, lesc->sc_reg, LEREG1_RDP, val);
}

hide u_int16_t
lerdcsr(sc, port)
	struct lance_softc *sc;
	u_int16_t port;
{
	struct le_softc *lesc = (struct le_softc *)sc;

	bus_space_write_2(lesc->sc_bustag, lesc->sc_reg, LEREG1_RAP, port);
	return (bus_space_read_2(lesc->sc_bustag, lesc->sc_reg, LEREG1_RDP));
} 

int
le_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct confargs *ca = aux;
	int addr;

	if (strcmp(ca->ca_name, "le"))
		return 0;

	switch(cf->cf_unit) {

	case 0:
		addr = LANCE_PORT;
		break;
	default:
		return 0;
	}

	if (badaddr((void *)addr, 1))
		return 0;

	return 1;
}

#define	LE_MEMSIZE	(32*1024)

void
le_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct le_softc *lesc = (struct le_softc *)self;
	struct lance_softc *sc = &lesc->sc_am7990.lsc;
	struct confargs *ca = aux;

	bus_dma_tag_t dmat;
	bus_dma_segment_t seg;
	int rseg;

	/*struct confargs *ca = aux;*/
	u_char *id;
	int i;
	caddr_t kvaddr;

	switch (sc->sc_dev.dv_unit) {
	case 0:
		id = (u_char *)(ETHER_ID);
		break;
	default:
		panic("le_attach");
	}

	lesc->sc_bustag = ca->ca_bustag;
	dmat = lesc->sc_dmatag = ca->ca_dmatag;

	if (bus_space_map(ca->ca_bustag, ca->ca_addr,
			  8,	/* size */
			  BUS_SPACE_MAP_LINEAR,
			  &lesc->sc_reg) != 0) {
		printf(": cannot map registers\n");
		return;
	}

	/*
	 * Allocate a physically contiguous DMA area for the chip.
	 */
	if (bus_dmamem_alloc(dmat, LE_MEMSIZE, 0, 0, &seg, 1,
			     &rseg, BUS_DMA_NOWAIT)) {
		printf(": can't allocate DMA area\n");
		return;
	}
	/* Map pages into kernel memory */
	if (bus_dmamem_map(dmat, &seg, rseg, LE_MEMSIZE,
	    &kvaddr, BUS_DMA_NOWAIT|BUS_DMA_COHERENT)) {
		printf(": can't map DMA area\n");
		bus_dmamem_free(dmat, &seg, rseg);
		return;
	}
	/* Build DMA map so we can get physical address */
	if (bus_dmamap_create(dmat, LE_MEMSIZE, 1, LE_MEMSIZE,
			      0, BUS_DMA_NOWAIT, &lesc->sc_dmamap)) {
		printf(": can't create DMA map\n");
		goto bad;
	}
	if (bus_dmamap_load(dmat, lesc->sc_dmamap, kvaddr, LE_MEMSIZE,
			    NULL, BUS_DMA_NOWAIT)) {
		printf(": can't load DMA map\n");
		goto bad;
	}

	sc->sc_memsize = LE_MEMSIZE;	/* 16K Buffer space*/
	sc->sc_mem  = (void *) MIPS_PHYS_TO_KSEG1(kvaddr);
	sc->sc_addr = lesc->sc_dmamap->dm_segs[0].ds_addr;

	sc->sc_conf3 = LE_C3_BSWP;

	/* Copy Ethernet hardware address from NVRAM */
	for (i=0; i < 6; i++)
		sc->sc_enaddr[i] = id[i*4+3];	/* XXX */

	sc->sc_copytodesc = lance_copytobuf_contig;
	sc->sc_copyfromdesc = lance_copyfrombuf_contig;
	sc->sc_copytobuf = lance_copytobuf_contig;
	sc->sc_copyfrombuf = lance_copyfrombuf_contig;
	sc->sc_zerobuf = lance_zerobuf_contig;

	sc->sc_rdcsr = lerdcsr;
	sc->sc_wrcsr = lewrcsr;
	sc->sc_hwinit = NULL;

	evcnt_attach_dynamic(&lesc->sc_intrcnt, EVCNT_TYPE_INTR, NULL,
			     self->dv_xname, "intr");
	bus_intr_establish(lesc->sc_bustag, SYS_INTR_ETHER, 0, 0,
			   le_intr, lesc);

	am7990_config(&lesc->sc_am7990);
	return;

bad:
	bus_dmamem_unmap(dmat, kvaddr, LE_MEMSIZE);
	bus_dmamem_free(dmat, &seg, rseg);
}

int
le_intr(arg)
	void *arg;
{
	struct le_softc *lesc = arg;

	lesc->sc_intrcnt.ev_count++;
	return am7990_intr(&lesc->sc_am7990);
}
