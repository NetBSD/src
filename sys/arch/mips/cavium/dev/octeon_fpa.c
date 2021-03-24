/*	$NetBSD: octeon_fpa.c,v 1.10 2021/03/24 08:10:14 simonb Exp $	*/

/*
 * Copyright (c) 2007 Internet Initiative Japan, Inc.
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
 */

#undef	FPADEBUG

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: octeon_fpa.c,v 1.10 2021/03/24 08:10:14 simonb Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/kmem.h>

#include <sys/bus.h>
#include <machine/locore.h>
#include <machine/vmparam.h>

#include <mips/cavium/octeonvar.h>
#include <mips/cavium/include/iobusvar.h>
#include <mips/cavium/dev/octeon_fpareg.h>
#include <mips/cavium/dev/octeon_fpavar.h>

#ifdef FPADEBUG
#define	DPRINTF(x)	printf x
#else
#define	DPRINTF(x)
#endif

#define	_DMA_NSEGS	1
#define	_DMA_BUFLEN	0x01000000

struct octfpa_softc {
	int			sc_initialized;

	bus_space_tag_t		sc_regt;
	bus_space_handle_t	sc_regh;

	bus_space_tag_t		sc_opst;
	bus_space_handle_t	sc_opsh;

	bus_dma_tag_t		sc_dmat;
};

void			octfpa_bootstrap(struct octeon_config *);
void			octfpa_reset(void);
void			octfpa_int_enable(struct octfpa_softc *, int);
void			octfpa_buf_dma_alloc(struct octfpa_buf *);

static void		octfpa_init(struct octfpa_softc *);
#ifdef notyet
static uint64_t		octfpa_iobdma(struct octfpa_softc *, int, int);
#endif

static struct octfpa_softc	octfpa_softc;

/* ---- global functions */

void
octfpa_bootstrap(struct octeon_config *mcp)
{
	struct octfpa_softc *sc = &octfpa_softc;

	sc->sc_regt = &mcp->mc_iobus_bust;
	sc->sc_opst = &mcp->mc_iobus_bust;
	sc->sc_dmat = &mcp->mc_iobus_dmat;

	octfpa_init(sc);
}

void
octfpa_reset(void)
{
	/* XXX */
}

int
octfpa_buf_init(int poolno, size_t size, size_t nelems, struct octfpa_buf **rfb)
{
	struct octfpa_softc *sc = &octfpa_softc;
	struct octfpa_buf *fb;
	int nsegs;
	paddr_t paddr;

	nsegs = 1/* XXX */;
	fb = kmem_zalloc(sizeof(*fb) + sizeof(*fb->fb_dma_segs) * nsegs,
	    KM_SLEEP);
	fb->fb_poolno = poolno;
	fb->fb_size = size;
	fb->fb_nelems = nelems;
	fb->fb_len = size * nelems;
	fb->fb_dmat = sc->sc_dmat;
	fb->fb_dma_segs = (void *)(fb + 1);
	fb->fb_dma_nsegs = nsegs;

	octfpa_buf_dma_alloc(fb);

	for (paddr = fb->fb_paddr; paddr < fb->fb_paddr + fb->fb_len;
	    paddr += fb->fb_size)
		octfpa_buf_put_paddr(fb, paddr);

	*rfb = fb;

	return 0;
}

void *
octfpa_buf_get(struct octfpa_buf *fb)
{
	paddr_t paddr;
	vaddr_t addr;

	paddr = octfpa_buf_get_paddr(fb);
	if (paddr == 0)
		addr = 0;
	else
		addr = fb->fb_addr + (vaddr_t/* XXX */)(paddr - fb->fb_paddr);
	return (void *)addr;
}

void
octfpa_buf_dma_alloc(struct octfpa_buf *fb)
{
	int status;
	int nsegs;
	void *va;

	status = bus_dmamap_create(fb->fb_dmat, fb->fb_len,
	    fb->fb_len / PAGE_SIZE,	/* # of segments */
	    fb->fb_len,			/* we don't use s/g for FPA buf */
	    PAGE_SIZE,			/* OCTEON hates >PAGE_SIZE boundary */
	    0, &fb->fb_dmah);
	if (status != 0)
		panic("%s failed", "bus_dmamap_create");

	status = bus_dmamem_alloc(fb->fb_dmat, fb->fb_len, 128, 0,
	    fb->fb_dma_segs, fb->fb_dma_nsegs, &nsegs, 0);
	if (status != 0 || fb->fb_dma_nsegs != nsegs)
		panic("%s failed", "bus_dmamem_alloc");

	status = bus_dmamem_map(fb->fb_dmat, fb->fb_dma_segs, fb->fb_dma_nsegs,
	    fb->fb_len, &va, 0);
	if (status != 0)
		panic("%s failed", "bus_dmamem_map");

	status = bus_dmamap_load(fb->fb_dmat, fb->fb_dmah, va, fb->fb_len,
	    NULL,		/* kernel */
	    0);
	if (status != 0)
		panic("%s failed", "bus_dmamap_load");

	fb->fb_addr = (vaddr_t)va;
	fb->fb_paddr = fb->fb_dma_segs[0].ds_addr;
}

uint64_t
octfpa_query(int poolno)
{
	struct octfpa_softc *sc = &octfpa_softc;

	return bus_space_read_8(sc->sc_regt, sc->sc_regh,
	    FPA_QUE0_AVAILABLE_OFFSET + sizeof(uint64_t) * poolno);
}

/* ---- local functions */

static inline void	octfpa_init_bus(struct octfpa_softc *);
static inline void	octfpa_init_bus_space(struct octfpa_softc *);
static inline void	octfpa_init_regs(struct octfpa_softc *);

void
octfpa_init(struct octfpa_softc *sc)
{
	if (sc->sc_initialized != 0)
		panic("%s: already initialized", __func__);
	sc->sc_initialized = 1;

	octfpa_init_bus(sc);
	octfpa_init_regs(sc);
}

void
octfpa_init_bus(struct octfpa_softc *sc)
{

	octfpa_init_bus_space(sc);
}

void
octfpa_init_bus_space(struct octfpa_softc *sc)
{
	int status;

	status = bus_space_map(sc->sc_regt, FPA_BASE, FPA_SIZE, 0, &sc->sc_regh);
	if (status != 0)
		panic("can't map %s space", "register");

	status = bus_space_map(sc->sc_opst,
	    0x0001180028000000ULL/* XXX */, 0x0200/* XXX */, 0, &sc->sc_opsh);
	if (status != 0)
		panic("can't map %s space", "operations");
}

void
octfpa_init_regs(struct octfpa_softc *sc)
{

	bus_space_write_8(sc->sc_regt, sc->sc_regh, FPA_CTL_STATUS_OFFSET,
	    FPA_CTL_STATUS_ENB);
}

int
octfpa_available_fpa_pool(int *available, int pool_no) {
	struct octfpa_softc *sc = &octfpa_softc;
	size_t offset;
	uint64_t tmp;

	switch (pool_no) {
	case OCTEON_POOL_NO_PKT:
		offset = FPA_QUE0_AVAILABLE_OFFSET;
		break;
	case OCTEON_POOL_NO_WQE:
		offset = FPA_QUE1_AVAILABLE_OFFSET;
		break;
	case OCTEON_POOL_NO_CMD:
		offset = FPA_QUE2_AVAILABLE_OFFSET;
		break;
	case OCTEON_POOL_NO_SG:
		offset = FPA_QUE3_AVAILABLE_OFFSET;
		break;
	case OCTEON_POOL_NO_XXX_4:
		offset = FPA_QUE4_AVAILABLE_OFFSET;
		break;
	case OCTEON_POOL_NO_XXX_5:
		offset = FPA_QUE5_AVAILABLE_OFFSET;
		break;
	case OCTEON_POOL_NO_XXX_6:
		offset = FPA_QUE6_AVAILABLE_OFFSET;
		break;
	case OCTEON_POOL_NO_DUMP:
		offset = FPA_QUE7_AVAILABLE_OFFSET;
		break;
	default:
		return EINVAL;
	}
	tmp = bus_space_read_8(sc->sc_regt, sc->sc_regh, offset);
	if (available) {
		*available = (int)(tmp & FPA_QUEX_AVAILABLE_QUE_SIZ);
	}

	return 0;
}
