/*	$NetBSD: le_elb.c,v 1.3 2003/08/16 15:40:41 hannken Exp $	*/

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Juergen Hannken-Illjes.
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
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
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
__KERNEL_RCSID(0, "$NetBSD: le_elb.c,v 1.3 2003/08/16 15:40:41 hannken Exp $");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>

#include <machine/bus.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#include <dev/ic/lancereg.h>
#include <dev/ic/lancevar.h>
#include <dev/ic/am79900reg.h>
#include <dev/ic/am79900var.h>

#include <evbppc/explora/dev/elbvar.h>

#define LE_MEMSIZE	16384
#define LE_RDP		0x10	/* Indirect data register. */
#define LE_RAP		0x14	/* Indirect address register. */
#define LE_NPORTS	32

struct le_elb_softc {
	struct am79900_softc sc_am79900;
	bus_dma_tag_t sc_dmat;
	bus_dmamap_t sc_dmam;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	void *sc_ih;
};

static int	le_elb_probe(struct device *, struct cfdata *, void *);
static void	le_elb_attach(struct device *, struct device *, void *);
static u_int16_t le_rdcsr(struct lance_softc *, u_int16_t);
static void	le_wrcsr(struct lance_softc *, u_int16_t, u_int16_t);
static void	le_copytodesc(struct lance_softc *, void *, int, int);
static void	le_copyfromdesc(struct lance_softc *, void *, int, int);
static void	le_copytobuf(struct lance_softc *, void *, int, int);
static void	le_copyfrombuf(struct lance_softc *, void *, int, int);
static void	le_zerobuf(struct lance_softc *, int, int);

CFATTACH_DECL(le_elb, sizeof(struct le_elb_softc),
    le_elb_probe, le_elb_attach, NULL, NULL);

int
le_elb_probe(struct device *parent, struct cfdata *cf, void *aux)
{
	struct elb_attach_args *oaa = aux;

	if (strcmp(oaa->elb_name, cf->cf_name) != 0)
		return 0;

	return (1);
}

void
le_elb_attach(struct device *parent, struct device *self, void *aux)
{
	struct le_elb_softc *msc = (void *)self;
	struct lance_softc *sc = &msc->sc_am79900.lsc;
	struct elb_attach_args *eaa = aux;
	bus_dma_segment_t seg;
	int i, rseg;

	printf("\n");

	if (booted_device == NULL)	/*XXX*/
		booted_device = self;

	msc->sc_iot = eaa->elb_bt;
	msc->sc_dmat = eaa->elb_dmat;

	bus_space_map(msc->sc_iot, eaa->elb_base, LE_NPORTS, 0, &msc->sc_ioh);

	/*
	 * Allocate a DMA area for the card.
	 */
	if (bus_dmamem_alloc(msc->sc_dmat, LE_MEMSIZE, PAGE_SIZE, 0,
	    &seg, 1, &rseg, BUS_DMA_NOWAIT)) {
		printf("%s: couldn't allocate memory for card\n",
		    sc->sc_dev.dv_xname);
		return;
	}
	if (bus_dmamem_map(msc->sc_dmat, &seg, rseg, LE_MEMSIZE,
	    (caddr_t *)&sc->sc_mem,
	    BUS_DMA_NOWAIT|BUS_DMA_COHERENT)) {
		printf("%s: couldn't map memory for card\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	/*
	 * Create and load the DMA map for the DMA area.
	 */
	if (bus_dmamap_create(msc->sc_dmat, LE_MEMSIZE, 1,
	    LE_MEMSIZE, 0, BUS_DMA_NOWAIT, &msc->sc_dmam)) {
		printf("%s: couldn't create DMA map\n",
		    sc->sc_dev.dv_xname);
		bus_dmamem_free(msc->sc_dmat, &seg, rseg);
		return;
	}
	if (bus_dmamap_load(msc->sc_dmat, msc->sc_dmam,
	    sc->sc_mem, LE_MEMSIZE, NULL, BUS_DMA_NOWAIT)) {
		printf("%s: coundn't load DMA map\n",
		    sc->sc_dev.dv_xname);
		bus_dmamem_free(msc->sc_dmat, &seg, rseg);
		return;
	}

	/*
	 * This is magic -- DMA doesn't work without address
	 * bit 30 set to one.
	 */
	sc->sc_addr = 0x40000000 | msc->sc_dmam->dm_segs[0].ds_addr;
	sc->sc_memsize = LE_MEMSIZE;

	sc->sc_copytodesc = le_copytodesc;
	sc->sc_copyfromdesc = le_copyfromdesc;
	sc->sc_copytobuf = le_copytobuf;
	sc->sc_copyfrombuf = le_copyfrombuf;
	sc->sc_zerobuf = le_zerobuf;

	sc->sc_rdcsr = le_rdcsr;
	sc->sc_wrcsr = le_wrcsr;

	printf("%s", sc->sc_dev.dv_xname);

	/* Save the MAC address. */
	for (i = 0; i < 3; i++) {
		sc->sc_enaddr[i*2] = le_rdcsr(sc, 12+i);
		sc->sc_enaddr[i*2+1] = le_rdcsr(sc, 12+i) >> 8;
	}

	am79900_config(&msc->sc_am79900);

	/* Chip is stopped. Set "software style" to 32-bit. */
	le_wrcsr(sc, LE_CSR58, 2);

	intr_establish(eaa->elb_irq, IST_LEVEL, IPL_NET, am79900_intr, sc);
}

/*
 * Read from an indirect CSR.
 */
static u_int16_t
le_rdcsr(struct lance_softc *sc, u_int16_t reg)
{
	struct le_elb_softc *lesc = (struct le_elb_softc *)sc;
	bus_space_tag_t iot = lesc->sc_iot;
	bus_space_handle_t ioh = lesc->sc_ioh;
	u_int16_t val;

	bus_space_write_4(iot, ioh, LE_RAP, reg);
	val = bus_space_read_4(iot, ioh, LE_RDP);

	return(val);
}

/*
 * Write to an indirect CSR.
 */
static void
le_wrcsr(struct lance_softc *sc, u_int16_t reg, u_int16_t val)
{
	struct le_elb_softc *lesc = (struct le_elb_softc *)sc;
	bus_space_tag_t iot = lesc->sc_iot;
	bus_space_handle_t ioh = lesc->sc_ioh;

	bus_space_write_4(iot, ioh, LE_RAP, reg);
	bus_space_write_4(iot, ioh, LE_RDP, val);
}

/*
 * Copy data to memory and swap bytes.
 */
static void
le_copytodesc(struct lance_softc *sc, void *from, int boff, int len)
{
	struct le_elb_softc *msc = (struct le_elb_softc *)sc;
	volatile u_int32_t *src = from;
	volatile u_int32_t *dst = (u_int32_t *)((u_char *)sc->sc_mem+boff);
	int todo = len;

	/* XXX lance_setladrf should be modified to use u_int32_t instead.
	 * The init block contains u_int16_t values that require
	 * special swapping.
	 */
	if (boff == LE_INITADDR(sc) && len == sizeof(struct leinit)) {
		src[3] = (src[3] >> 16) | (src[3] << 16);
		src[4] = (src[4] >> 16) | (src[4] << 16);
	}

	todo /= sizeof(u_int32_t);
	while (todo-- > 0)
		*dst++ = bswap32(*src++);

	bus_dmamap_sync(msc->sc_dmat, msc->sc_dmam, boff, len,
	    BUS_DMASYNC_PREWRITE);
}

/*
 * Copy data from memory and swap bytes.
 */
static void
le_copyfromdesc(struct lance_softc *sc, void *to, int boff, int len)
{
	struct le_elb_softc *msc = (struct le_elb_softc *)sc;
	volatile u_int32_t *src = (u_int32_t *)((u_char *)sc->sc_mem+boff);
	volatile u_int32_t *dst = to;

	bus_dmamap_sync(msc->sc_dmat, msc->sc_dmam, boff, len,
	    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);

	len /= sizeof(u_int32_t);
	while (len-- > 0)
		*dst++ = bswap32(*src++);
}

/*
 * Copy data to memory.
 */
static void
le_copytobuf(struct lance_softc *sc, void *from, int boff, int len)
{
	struct le_elb_softc *msc = (struct le_elb_softc *)sc;
	volatile caddr_t buf = (caddr_t)((u_char *)sc->sc_mem+boff);

	memcpy(buf, from, len);

	bus_dmamap_sync(msc->sc_dmat, msc->sc_dmam, boff, len,
	    BUS_DMASYNC_PREWRITE);
}

/*
 * Copy data from memory.
 */
static void
le_copyfrombuf(struct lance_softc *sc, void *to, int boff, int len)
{
	struct le_elb_softc *msc = (struct le_elb_softc *)sc;
	volatile caddr_t buf = (caddr_t)((u_char *)sc->sc_mem+boff);

	bus_dmamap_sync(msc->sc_dmat, msc->sc_dmam, boff, len,
	    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);

	memcpy(to, buf, len);
}

/*
 * Zero memory.
 */
static void
le_zerobuf(struct lance_softc *sc, int boff, int len)  
{
	struct le_elb_softc *msc = (struct le_elb_softc *)sc;
	volatile caddr_t buf = (caddr_t)((u_char *)sc->sc_mem+boff);

	memset(buf, 0, len);

	bus_dmamap_sync(msc->sc_dmat, msc->sc_dmam, boff, len,
	    BUS_DMASYNC_PREWRITE);
}
