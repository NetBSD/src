/*	$NetBSD: pxa2x0_i2s.c,v 1.7.34.1 2009/05/13 17:16:18 jym Exp $	*/
/*	$OpenBSD: pxa2x0_i2s.c,v 1.7 2006/04/04 11:45:40 pascoe Exp $	*/

/*
 * Copyright (c) 2005 Christopher Pascoe <pascoe@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pxa2x0_i2s.c,v 1.7.34.1 2009/05/13 17:16:18 jym Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/bus.h>

#include <arm/xscale/pxa2x0reg.h>
#include <arm/xscale/pxa2x0var.h>
#include <arm/xscale/pxa2x0_gpio.h>
#include <arm/xscale/pxa2x0_i2s.h>
#include <arm/xscale/pxa2x0_dmac.h>

struct pxa2x0_i2s_dma {
	struct pxa2x0_i2s_dma *next;
	void *addr;
	size_t size;
	bus_dmamap_t map;
#define	I2S_N_SEGS	1
	bus_dma_segment_t segs[I2S_N_SEGS];
	int nsegs;
	struct dmac_xfer *dx;
};

static void pxa2x0_i2s_dmac_ointr(struct dmac_xfer *, int);
static void pxa2x0_i2s_dmac_iintr(struct dmac_xfer *, int);

void
pxa2x0_i2s_init(struct pxa2x0_i2s_softc *sc)
{

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, I2S_SACR0, SACR0_RST);
	delay(100);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, I2S_SACR0,
	    SACR0_BCKD | SACR0_SET_TFTH(7) | SACR0_SET_RFTH(7));
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, I2S_SACR1, 0);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, I2S_SADR, 0);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, I2S_SADIV, sc->sc_sadiv);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, I2S_SACR0,
		SACR0_BCKD | SACR0_SET_TFTH(7) | SACR0_SET_RFTH(7) | SACR0_ENB);
}

int
pxa2x0_i2s_attach_sub(struct pxa2x0_i2s_softc *sc)
{
	int rv;

	rv = bus_space_map(sc->sc_iot, PXA2X0_I2S_BASE, PXA2X0_I2S_SIZE, 0,
	    &sc->sc_ioh);
	if (rv) {
		sc->sc_size = 0;
		return 1;
	}

	sc->sc_dr.ds_addr = PXA2X0_I2S_BASE + I2S_SADR;
	sc->sc_dr.ds_len = 4;

	sc->sc_sadiv = SADIV_3_058MHz;

	bus_space_barrier(sc->sc_iot, sc->sc_ioh, 0, sc->sc_size,
	    BUS_SPACE_BARRIER_READ|BUS_SPACE_BARRIER_WRITE);

	pxa2x0_i2s_init(sc);

	return 0;
}

void
pxa2x0_i2s_open(struct pxa2x0_i2s_softc *sc)
{

	if (sc->sc_open++ == 0) {
		pxa2x0_clkman_config(CKEN_I2S, 1);
	}
}

void
pxa2x0_i2s_close(struct pxa2x0_i2s_softc *sc)
{

	if (--sc->sc_open == 0) {
		pxa2x0_clkman_config(CKEN_I2S, 0);
	}
}

int
pxa2x0_i2s_detach_sub(struct pxa2x0_i2s_softc *sc)
{

	if (sc->sc_size > 0) {
		bus_space_unmap(sc->sc_iot, sc->sc_ioh, sc->sc_size);
		sc->sc_size = 0;
	}
	pxa2x0_clkman_config(CKEN_I2S, 0);

	return 0;
}

void
pxa2x0_i2s_write(struct pxa2x0_i2s_softc *sc, uint32_t data)
{

	if (sc->sc_open == 0)
		return;

	/* Clear intr and underrun bit if set. */
	if (bus_space_read_4(sc->sc_iot, sc->sc_ioh, I2S_SASR0) & SASR0_TUR)
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, I2S_SAICR, SAICR_TUR);

	/* Wait for transmit fifo to have space. */
	while ((bus_space_read_4(sc->sc_iot, sc->sc_ioh, I2S_SASR0) & SASR0_TNF)
	     == 0)
		continue;	/* nothing */

	/* Queue data */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, I2S_SADR, data);
}

void
pxa2x0_i2s_setspeed(struct pxa2x0_i2s_softc *sc, u_int *argp)
{
	/*
	 * The available speeds are in the following table.
	 * Keep the speeds in increasing order.
	 */
	static const struct speed_struct {
		int	speed;
		int	div;
	} speed_table[] = {
		{8000,	SADIV_513_25kHz},
		{11025,	SADIV_702_75kHz},
		{16000,	SADIV_1_026MHz},
		{22050,	SADIV_1_405MHz},
		{44100,	SADIV_2_836MHz},
		{48000,	SADIV_3_058MHz},
	};
	const int n = (int)__arraycount(speed_table);
	u_int arg = (u_int)*argp;
	int selected = -1;
	int i;

	if (arg < speed_table[0].speed)
		selected = 0;
	if (arg > speed_table[n - 1].speed)
		selected = n - 1;

	for (i = 1; selected == -1 && i < n; i++) {
		if (speed_table[i].speed == arg)
			selected = i;
		else if (speed_table[i].speed > arg) {
			int diff1, diff2;

			diff1 = arg - speed_table[i - 1].speed;
			diff2 = speed_table[i].speed - arg;
			if (diff1 < diff2)
				selected = i - 1;
			else
				selected = i;
		}
	}

	if (selected == -1)
		selected = 0;

	*argp = speed_table[selected].speed;

	sc->sc_sadiv = speed_table[selected].div;
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, I2S_SADIV, sc->sc_sadiv);
}

void *
pxa2x0_i2s_allocm(void *hdl, int direction, size_t size,
    struct malloc_type *type, int flags)
{
	struct pxa2x0_i2s_softc *sc = hdl;
	struct pxa2x0_i2s_dma *p;
	struct dmac_xfer *dx;
	int error;

	p = malloc(sizeof(*p), type, flags);
	if (p == NULL)
		return NULL;

	dx = pxa2x0_dmac_allocate_xfer(M_NOWAIT);
	if (dx == NULL) {
		goto fail_alloc;
	}
	p->dx = dx;

	p->size = size;
	if ((error = bus_dmamem_alloc(sc->sc_dmat, size, NBPG, 0, p->segs,
	    I2S_N_SEGS, &p->nsegs, BUS_DMA_NOWAIT)) != 0) {
		goto fail_xfer;
	}

	if ((error = bus_dmamem_map(sc->sc_dmat, p->segs, p->nsegs, size,
	    &p->addr, BUS_DMA_NOWAIT | BUS_DMA_COHERENT)) != 0) {
		goto fail_map;
	}

	if ((error = bus_dmamap_create(sc->sc_dmat, size, 1, size, 0,
	    BUS_DMA_NOWAIT, &p->map)) != 0) {
		goto fail_create;
	}

	if ((error = bus_dmamap_load(sc->sc_dmat, p->map, p->addr, size, NULL,
	    BUS_DMA_NOWAIT)) != 0) {
		goto fail_load;
	}

	dx->dx_cookie = sc;
	dx->dx_priority = DMAC_PRIORITY_NORMAL;
	dx->dx_dev_width = DMAC_DEV_WIDTH_4;
	dx->dx_burst_size = DMAC_BURST_SIZE_32;

	p->next = sc->sc_dmas;
	sc->sc_dmas = p;

	return p->addr;

fail_load:
	bus_dmamap_destroy(sc->sc_dmat, p->map);
fail_create:
	bus_dmamem_unmap(sc->sc_dmat, p->addr, size);
fail_map:
	bus_dmamem_free(sc->sc_dmat, p->segs, p->nsegs);
fail_xfer:
	pxa2x0_dmac_free_xfer(dx);
fail_alloc:
	free(p, type);
	return NULL;
}

void
pxa2x0_i2s_freem(void *hdl, void *ptr, struct malloc_type *type)
{
	struct pxa2x0_i2s_softc *sc = hdl;
	struct pxa2x0_i2s_dma **pp, *p;

	for (pp = &sc->sc_dmas; (p = *pp) != NULL; pp = &p->next) {
		if (p->addr == ptr) {
			pxa2x0_dmac_abort_xfer(p->dx);
			pxa2x0_dmac_free_xfer(p->dx);
			p->segs[0].ds_len = p->size;	/* XXX */
			bus_dmamap_unload(sc->sc_dmat, p->map);
			bus_dmamap_destroy(sc->sc_dmat, p->map);
			bus_dmamem_unmap(sc->sc_dmat, p->addr, p->size);
			bus_dmamem_free(sc->sc_dmat, p->segs, p->nsegs);

			*pp = p->next;
			free(p, type);
			return;
		}
	}
	panic("pxa2x0_i2s_freem: trying to free unallocated memory");
}

paddr_t
pxa2x0_i2s_mappage(void *hdl, void *mem, off_t off, int prot)
{
	struct pxa2x0_i2s_softc *sc = hdl;
	struct pxa2x0_i2s_dma *p;

	if (off < 0)
		return -1;

	for (p = sc->sc_dmas; p && p->addr != mem; p = p->next)
		continue;
	if (p == NULL)
		return -1;

	if (off > p->size)
		return -1;

	return bus_dmamem_mmap(sc->sc_dmat, p->segs, p->nsegs, off, prot,
	    BUS_DMA_WAITOK);
}

int
pxa2x0_i2s_round_blocksize(void *hdl, int bs, int mode,
    const struct audio_params *param)
{

	/* Enforce individual DMA block size limit */
	if (bs > DCMD_LENGTH_MASK)
		return (DCMD_LENGTH_MASK & ~0x07);

	return (bs + 0x07) & ~0x07;	/* XXX: 64-bit multiples */
}

size_t
pxa2x0_i2s_round_buffersize(void *hdl, int direction, size_t bufsize)
{

	return bufsize;
}

int
pxa2x0_i2s_halt_output(void *hdl)
{
	struct pxa2x0_i2s_softc *sc = hdl;
	int s;

	s = splaudio();
	if (sc->sc_txdma) {
		pxa2x0_dmac_abort_xfer(sc->sc_txdma->dx);
		sc->sc_txdma = NULL;
	}
	splx(s);

	return 0;
}

int
pxa2x0_i2s_halt_input(void *hdl)
{
	struct pxa2x0_i2s_softc *sc = hdl;
	int s;

	s = splaudio();
	if (sc->sc_rxdma) {
		pxa2x0_dmac_abort_xfer(sc->sc_rxdma->dx);
		sc->sc_rxdma = NULL;
	}
	splx(s);

	return 0;
}

int
pxa2x0_i2s_start_output(void *hdl, void *block, int bsize,
    void (*tx_func)(void *), void *tx_arg)
{
	struct pxa2x0_i2s_softc *sc = hdl;
	struct pxa2x0_i2s_dma *p;
	struct dmac_xfer *dx;

	if (sc->sc_txdma)
		return EBUSY;

	/* Find mapping which contains block completely */
	for (p = sc->sc_dmas;
	     p != NULL && 
	       (((char*)block < (char *)p->addr) ||
	        ((char *)block + bsize > (char *)p->addr + p->size));
	     p = p->next) {
		continue;	/* Nothing */
	}
	if (p == NULL) {
		aprint_error("pxa2x0_i2s_start_output: "
		    "request with bad start address: %p, size: %d)\n",
		    block, bsize);
		return ENXIO;
	}
	sc->sc_txdma = p;

	p->segs[0].ds_addr = p->map->dm_segs[0].ds_addr +
	                         ((char *)block - (char *)p->addr);
	p->segs[0].ds_len = bsize;

	dx = p->dx;
	dx->dx_done = pxa2x0_i2s_dmac_ointr;
	dx->dx_peripheral = DMAC_PERIPH_I2STX;
	dx->dx_flow = DMAC_FLOW_CTRL_DEST;
	dx->dx_loop_notify = DMAC_DONT_LOOP;
	dx->dx_desc[DMAC_DESC_SRC].xd_addr_hold = false;
	dx->dx_desc[DMAC_DESC_SRC].xd_nsegs = p->nsegs;
	dx->dx_desc[DMAC_DESC_SRC].xd_dma_segs = p->segs;
	dx->dx_desc[DMAC_DESC_DST].xd_addr_hold = true;
	dx->dx_desc[DMAC_DESC_DST].xd_nsegs = 1;
	dx->dx_desc[DMAC_DESC_DST].xd_dma_segs = &sc->sc_dr;

	sc->sc_txfunc = tx_func;
	sc->sc_txarg = tx_arg;

	/* Start DMA */
	return pxa2x0_dmac_start_xfer(dx);
}

int
pxa2x0_i2s_start_input(void *hdl, void *block, int bsize,
    void (*rx_func)(void *), void *rx_arg)
{
	struct pxa2x0_i2s_softc *sc = hdl;
	struct pxa2x0_i2s_dma *p;
	struct dmac_xfer *dx;

	if (sc->sc_rxdma)
		return EBUSY;

	/* Find mapping which contains block completely */
	for (p = sc->sc_dmas;
	     p != NULL && 
	       (((char*)block < (char *)p->addr) ||
	        ((char *)block + bsize > (char *)p->addr + p->size));
	     p = p->next) {
		continue;	/* Nothing */
	}
	if (p == NULL) {
		aprint_error("pxa2x0_i2s_start_input: "
		    "request with bad start address: %p, size: %d)\n",
		    block, bsize);
		return ENXIO;
	}
	sc->sc_rxdma = p;

	p->segs[0].ds_addr = p->map->dm_segs[0].ds_addr +
	                         ((char *)block - (char *)p->addr);
	p->segs[0].ds_len = bsize;

	dx = p->dx;
	dx->dx_done = pxa2x0_i2s_dmac_iintr;
	dx->dx_peripheral = DMAC_PERIPH_I2SRX;
	dx->dx_flow = DMAC_FLOW_CTRL_SRC;
	dx->dx_loop_notify = DMAC_DONT_LOOP;
	dx->dx_desc[DMAC_DESC_SRC].xd_addr_hold = true;
	dx->dx_desc[DMAC_DESC_SRC].xd_nsegs = 1;
	dx->dx_desc[DMAC_DESC_SRC].xd_dma_segs = &sc->sc_dr;
	dx->dx_desc[DMAC_DESC_DST].xd_addr_hold = false;
	dx->dx_desc[DMAC_DESC_DST].xd_nsegs = p->nsegs;
	dx->dx_desc[DMAC_DESC_DST].xd_dma_segs = p->segs;

	sc->sc_rxfunc = rx_func;
	sc->sc_rxarg = rx_arg;

	/* Start DMA */
	return pxa2x0_dmac_start_xfer(dx);
}

static void
pxa2x0_i2s_dmac_ointr(struct dmac_xfer *dx, int status)
{
	struct pxa2x0_i2s_softc *sc = dx->dx_cookie;
	int s;

	if (sc->sc_txdma == NULL) {
		panic("pxa2x_i2s_dmac_ointr: bad TX DMA descriptor!");
	}
	if (sc->sc_txdma->dx != dx) {
		panic("pxa2x_i2s_dmac_ointr: xfer mismatch!");
	}
	sc->sc_txdma = NULL;

	if (status) {
		aprint_error("pxa2x0_i2s_dmac_ointr: "
		    "non-zero completion status %d\n", status);
	}

	s = splaudio();
	(sc->sc_txfunc)(sc->sc_txarg);
	splx(s);
}

static void
pxa2x0_i2s_dmac_iintr(struct dmac_xfer *dx, int status)
{
	struct pxa2x0_i2s_softc *sc = dx->dx_cookie;
	int s;

	if (sc->sc_rxdma == NULL) {
		panic("pxa2x_i2s_dmac_iintr: bad RX DMA descriptor!");
	}
	if (sc->sc_rxdma->dx != dx) {
		panic("pxa2x_i2s_dmac_iintr: xfer mismatch!");
	}
	sc->sc_rxdma = NULL;

	if (status) {
		aprint_error("pxa2x0_i2s_dmac_iintr: "
		    "non-zero completion status %d\n", status);
	}


	s = splaudio();
	(sc->sc_rxfunc)(sc->sc_rxarg);
	splx(s);
}
