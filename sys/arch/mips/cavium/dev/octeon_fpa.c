/*	$NetBSD: octeon_fpa.c,v 1.1.18.2 2017/12/03 11:36:27 jdolecek Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: octeon_fpa.c,v 1.1.18.2 2017/12/03 11:36:27 jdolecek Exp $");

#include "opt_octeon.h"

#include "opt_octeon.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/malloc.h>

#include <sys/bus.h>
#include <machine/locore.h>
#include <machine/vmparam.h>

#include <mips/cavium/octeonvar.h>
#include <mips/cavium/include/iobusvar.h>
#include <mips/cavium/dev/octeon_fpavar.h>
#include <mips/cavium/dev/octeon_fpareg.h>

#ifdef FPADEBUG
#define	DPRINTF(x)	printf x
#else
#define	DPRINTF(x)
#endif

#define	_DMA_NSEGS	1
#define	_DMA_BUFLEN	0x01000000

/* pool descriptor */
struct octeon_fpa_desc {
};

struct octeon_fpa_softc {
	int			sc_initialized;

	bus_space_tag_t		sc_regt;
	bus_space_handle_t	sc_regh;

	bus_space_tag_t		sc_opst;
	bus_space_handle_t	sc_opsh;

	bus_dma_tag_t		sc_dmat;

	struct octeon_fpa_desc	sc_descs[8];

#ifdef OCTEON_ETH_DEBUG
	struct evcnt		sc_ev_fpaq7perr;
	struct evcnt		sc_ev_fpaq7coff;
	struct evcnt		sc_ev_fpaq7und;
	struct evcnt		sc_ev_fpaq6perr;
	struct evcnt		sc_ev_fpaq6coff;
	struct evcnt		sc_ev_fpaq6und;
	struct evcnt		sc_ev_fpaq5perr;
	struct evcnt		sc_ev_fpaq5coff;
#endif
};

void			octeon_fpa_bootstrap(struct octeon_config *);
void			octeon_fpa_reset(void);
void			octeon_fpa_int_enable(struct octeon_fpa_softc *, int);
void			octeon_fpa_buf_dma_alloc(struct octeon_fpa_buf *);

static void		octeon_fpa_init(struct octeon_fpa_softc *);
#ifdef notyet
static uint64_t		octeon_fpa_iobdma(struct octeon_fpa_softc *, int, int);
#endif

#ifdef OCTEON_ETH_DEBUG
void			octeon_fpa_intr_evcnt_attach(struct octeon_fpa_softc *);
void			octeon_fpa_intr_rml(void *);
#endif

static struct octeon_fpa_softc	octeon_fpa_softc;

/* ---- global functions */

void
octeon_fpa_bootstrap(struct octeon_config *mcp)
{
	struct octeon_fpa_softc *sc = &octeon_fpa_softc;

	sc->sc_regt = &mcp->mc_iobus_bust;
	sc->sc_opst = &mcp->mc_iobus_bust;
	sc->sc_dmat = &mcp->mc_iobus_dmat;

	octeon_fpa_init(sc);
}

void
octeon_fpa_reset(void)
{
	/* XXX */
}

#ifdef OCTEON_ETH_DEBUG
int			octeon_fpa_intr_rml_verbose;
struct evcnt		octeon_fpa_intr_evcnt;

static const struct octeon_evcnt_entry octeon_fpa_intr_evcnt_entries[] = {
#define	_ENTRY(name, type, parent, descr) \
	OCTEON_EVCNT_ENTRY(struct octeon_fpa_softc, name, type, parent, descr)
	_ENTRY(fpaq7perr,          MISC, NULL, "fpa q7 pointer"),
	_ENTRY(fpaq7coff,          MISC, NULL, "fpa q7 counter offset"),
	_ENTRY(fpaq7und,           MISC, NULL, "fpa q7 underflow"),
	_ENTRY(fpaq6perr,          MISC, NULL, "fpa q6 pointer"),
	_ENTRY(fpaq6coff,          MISC, NULL, "fpa q6 counter offset"),
	_ENTRY(fpaq6und,           MISC, NULL, "fpa q6 underflow"),
	_ENTRY(fpaq5perr,          MISC, NULL, "fpa q5 pointer"),
	_ENTRY(fpaq5coff,          MISC, NULL, "fpa q5 counter offset"),
#undef	_ENTRY
};

void
octeon_fpa_intr_evcnt_attach(struct octeon_fpa_softc *sc)
{
	OCTEON_EVCNT_ATTACH_EVCNTS(sc, octeon_fpa_intr_evcnt_entries, "fpa0");
}

void
octeon_fpa_intr_rml(void *arg)
{
	struct octeon_fpa_softc *sc;
	uint64_t reg;

	octeon_fpa_intr_evcnt.ev_count++;
	sc = &octeon_fpa_softc;
	KASSERT(sc != NULL);
	reg = octeon_fpa_int_summary();
	if (octeon_fpa_intr_rml_verbose)
		printf("%s: FPA_INT_SUM=0x%016" PRIx64 "\n", __func__, reg);
	if (reg & FPA_INT_SUM_Q7_PERR)
		OCTEON_EVCNT_INC(sc, fpaq7perr);
	if (reg & FPA_INT_SUM_Q7_COFF)
		OCTEON_EVCNT_INC(sc, fpaq7coff);
	if (reg & FPA_INT_SUM_Q7_UND)
		OCTEON_EVCNT_INC(sc, fpaq7und);
	if (reg & FPA_INT_SUM_Q6_PERR)
		OCTEON_EVCNT_INC(sc, fpaq6perr);
	if (reg & FPA_INT_SUM_Q6_COFF)
		OCTEON_EVCNT_INC(sc, fpaq6coff);
	if (reg & FPA_INT_SUM_Q6_UND)
		OCTEON_EVCNT_INC(sc, fpaq6und);
	if (reg & FPA_INT_SUM_Q5_PERR)
		OCTEON_EVCNT_INC(sc, fpaq5perr);
	if (reg & FPA_INT_SUM_Q5_COFF)
		OCTEON_EVCNT_INC(sc, fpaq5coff);
}

void
octeon_fpa_int_enable(struct octeon_fpa_softc *sc, int enable)
{
	const uint64_t int_xxx =
	    FPA_INT_ENB_Q7_PERR | FPA_INT_ENB_Q7_COFF | FPA_INT_ENB_Q7_UND |
	    FPA_INT_ENB_Q6_PERR | FPA_INT_ENB_Q6_COFF | FPA_INT_ENB_Q6_UND |
	    FPA_INT_ENB_Q5_PERR | FPA_INT_ENB_Q5_COFF | FPA_INT_ENB_Q5_UND |
	    FPA_INT_ENB_Q4_PERR | FPA_INT_ENB_Q4_COFF | FPA_INT_ENB_Q4_UND |
	    FPA_INT_ENB_Q3_PERR | FPA_INT_ENB_Q3_COFF | FPA_INT_ENB_Q3_UND |
	    FPA_INT_ENB_Q2_PERR | FPA_INT_ENB_Q2_COFF | FPA_INT_ENB_Q2_UND |
	    FPA_INT_ENB_Q1_PERR | FPA_INT_ENB_Q1_COFF | FPA_INT_ENB_Q1_UND |
	    FPA_INT_ENB_Q0_PERR | FPA_INT_ENB_Q0_COFF | FPA_INT_ENB_Q0_UND |
	    FPA_INT_ENB_FED1_DBE | FPA_INT_ENB_FED1_SBE |
	    FPA_INT_ENB_FED0_DBE | FPA_INT_ENB_FED0_SBE;

	bus_space_write_8(sc->sc_regt, sc->sc_regh, FPA_INT_SUM_OFFSET,
	    int_xxx);
	if (enable)
		bus_space_write_8(sc->sc_regt, sc->sc_regh, FPA_INT_ENB_OFFSET,
		    int_xxx);
}

uint64_t
octeon_fpa_int_summary(void)
{
	struct octeon_fpa_softc *sc = &octeon_fpa_softc;
	uint64_t summary;

	summary = bus_space_read_8(sc->sc_regt, sc->sc_regh, FPA_INT_SUM_OFFSET);
	bus_space_write_8(sc->sc_regt, sc->sc_regh, FPA_INT_SUM_OFFSET, summary);
	return summary;
}
#endif

int
octeon_fpa_buf_init(int poolno, size_t size, size_t nelems,
    struct octeon_fpa_buf **rfb)
{
	struct octeon_fpa_softc *sc = &octeon_fpa_softc;
	struct octeon_fpa_buf *fb;
	int nsegs;
	paddr_t paddr;

	nsegs = 1/* XXX */;
	fb = malloc(sizeof(*fb) + sizeof(*fb->fb_dma_segs) * nsegs, M_DEVBUF,
	    M_WAITOK | M_ZERO);
	if (fb == NULL)
		return 1;
	fb->fb_poolno = poolno;
	fb->fb_size = size;
	fb->fb_nelems = nelems;
	fb->fb_len = size * nelems;
	fb->fb_dmat = sc->sc_dmat;
	fb->fb_dma_segs = (void *)(fb + 1);
	fb->fb_dma_nsegs = nsegs;

	octeon_fpa_buf_dma_alloc(fb);

	for (paddr = fb->fb_paddr; paddr < fb->fb_paddr + fb->fb_len;
	    paddr += fb->fb_size)
		octeon_fpa_buf_put_paddr(fb, paddr);

	*rfb = fb;

	return 0;
}

void *
octeon_fpa_buf_get(struct octeon_fpa_buf *fb)
{
	paddr_t paddr;
	vaddr_t addr;

	paddr = octeon_fpa_buf_get_paddr(fb);
	if (paddr == 0)
		addr = 0;
	else
		addr = fb->fb_addr + (vaddr_t/* XXX */)(paddr - fb->fb_paddr);
	return (void *)addr;
}

void
octeon_fpa_buf_dma_alloc(struct octeon_fpa_buf *fb)
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
octeon_fpa_query(int poolno)
{
	struct octeon_fpa_softc *sc = &octeon_fpa_softc;

	return bus_space_read_8(sc->sc_regt, sc->sc_regh,
	    FPA_QUE0_AVAILABLE_OFFSET + sizeof(uint64_t) * poolno);
}

/* ---- local functions */

static inline void	octeon_fpa_init_bus(struct octeon_fpa_softc *);
static inline void	octeon_fpa_init_bus_space(struct octeon_fpa_softc *);
static inline void	octeon_fpa_init_regs(struct octeon_fpa_softc *);

void
octeon_fpa_init(struct octeon_fpa_softc *sc)
{
	if (sc->sc_initialized != 0)
		panic("%s: already initialized", __func__);
	sc->sc_initialized = 1;

	octeon_fpa_init_bus(sc);
	octeon_fpa_init_regs(sc);
#ifdef OCTEON_ETH_DEBUG
	octeon_fpa_int_enable(sc, 1);
	octeon_fpa_intr_evcnt_attach(sc);
#endif
}

void
octeon_fpa_init_bus(struct octeon_fpa_softc *sc)
{
	octeon_fpa_init_bus_space(sc);
}

void
octeon_fpa_init_bus_space(struct octeon_fpa_softc *sc)
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
octeon_fpa_init_regs(struct octeon_fpa_softc *sc)
{

	bus_space_write_8(sc->sc_regt, sc->sc_regh, FPA_CTL_STATUS_OFFSET,
	    FPA_CTL_STATUS_ENB);

/* XXX */
#ifdef OCTEON_ETH_DEBUG
	bus_space_write_8(sc->sc_regt, sc->sc_regh, FPA_INT_ENB_OFFSET,
	    FPA_INT_ENB_Q7_PERR | FPA_INT_ENB_Q7_COFF | FPA_INT_ENB_Q7_UND |
	    FPA_INT_ENB_Q6_PERR | FPA_INT_ENB_Q6_COFF | FPA_INT_ENB_Q6_UND |
	    FPA_INT_ENB_Q5_PERR | FPA_INT_ENB_Q5_COFF | FPA_INT_ENB_Q5_UND |
	    FPA_INT_ENB_Q4_PERR | FPA_INT_ENB_Q4_COFF | FPA_INT_ENB_Q4_UND |
	    FPA_INT_ENB_Q3_PERR | FPA_INT_ENB_Q3_COFF | FPA_INT_ENB_Q3_UND |
	    FPA_INT_ENB_Q2_PERR | FPA_INT_ENB_Q2_COFF | FPA_INT_ENB_Q2_UND |
	    FPA_INT_ENB_Q1_PERR | FPA_INT_ENB_Q1_COFF | FPA_INT_ENB_Q1_UND |
	    FPA_INT_ENB_Q0_PERR | FPA_INT_ENB_Q0_COFF | FPA_INT_ENB_Q0_UND |
	    FPA_INT_ENB_FED1_DBE | FPA_INT_ENB_FED1_SBE |
	    FPA_INT_ENB_FED0_DBE | FPA_INT_ENB_FED0_SBE);
#endif
}

int
octeon_fpa_available_fpa_pool(int *available, int pool_no) {
	struct octeon_fpa_softc *sc = &octeon_fpa_softc;
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

#ifdef OCTEON_ETH_DEBUG
void		octeon_fpa_dump_regs(void);
void		octeon_fpa_dump_bufs(void);
void		octeon_fpa_dump_buf(int);

#define	_ENTRY(x)	{ #x, x##_BITS, x##_OFFSET }

struct octeon_fpa_dump_reg_ {
	const char *name;
	const char *format;
	size_t	offset;
};

static const struct octeon_fpa_dump_reg_ octeon_fpa_dump_regs_[] = {
	_ENTRY(FPA_INT_SUM),
	_ENTRY(FPA_INT_ENB),
	_ENTRY(FPA_CTL_STATUS),
	_ENTRY(FPA_QUE0_AVAILABLE),
	_ENTRY(FPA_QUE1_AVAILABLE),
	_ENTRY(FPA_QUE2_AVAILABLE),
	_ENTRY(FPA_QUE3_AVAILABLE),
	_ENTRY(FPA_QUE4_AVAILABLE),
	_ENTRY(FPA_QUE5_AVAILABLE),
	_ENTRY(FPA_QUE6_AVAILABLE),
	_ENTRY(FPA_QUE7_AVAILABLE),
	_ENTRY(FPA_WART_CTL),
	_ENTRY(FPA_WART_STATUS),
	_ENTRY(FPA_BIST_STATUS),
	_ENTRY(FPA_QUE0_PAGE_INDEX),
	_ENTRY(FPA_QUE1_PAGE_INDEX),
	_ENTRY(FPA_QUE2_PAGE_INDEX),
	_ENTRY(FPA_QUE3_PAGE_INDEX),
	_ENTRY(FPA_QUE4_PAGE_INDEX),
	_ENTRY(FPA_QUE5_PAGE_INDEX),
	_ENTRY(FPA_QUE6_PAGE_INDEX),
	_ENTRY(FPA_QUE7_PAGE_INDEX),
	_ENTRY(FPA_QUE_EXP),
	_ENTRY(FPA_QUE_ACT),
};

static const char *octeon_fpa_dump_bufs_[8] = {
	[0] = "recv",
	[1] = "wq",
	[2] = "cmdbuf",
	[3] = "gbuf",
};

void
octeon_fpa_dump(void)
{
	octeon_fpa_dump_regs();
	octeon_fpa_dump_bufs();
}

void
octeon_fpa_dump_regs(void)
{
	struct octeon_fpa_softc *sc = &octeon_fpa_softc;
	const struct octeon_fpa_dump_reg_ *reg;
	uint64_t tmp;
	char buf[512];
	int i;

	for (i = 0; i < (int)__arraycount(octeon_fpa_dump_regs_); i++) {
		reg = &octeon_fpa_dump_regs_[i];
		tmp = bus_space_read_8(sc->sc_regt, sc->sc_regh, reg->offset);
		if (reg->format == NULL) {
			snprintf(buf, sizeof(buf), "%16" PRIx64, tmp);
		} else {
			snprintb(buf, sizeof(buf), reg->format, tmp);
		}
		printf("\t%-24s: %s\n", reg->name, buf);
	}
}

/*
 * XXX assume pool 7 is unused!
 */
void
octeon_fpa_dump_bufs(void)
{
	int i;

	for (i = 0; i < 8; i++)
		octeon_fpa_dump_buf(i);
}

void
octeon_fpa_dump_buf(int pool)
{
	int i;
	uint64_t ptr;
	const char *name;

	name = octeon_fpa_dump_bufs_[pool];
	if (name == NULL)
		return;
	printf("%s pool:\n", name);
	for (i = 0; (ptr = octeon_fpa_load(pool)) != 0; i++) {
		printf("\t%016" PRIx64 "%s", ptr, ((i % 4) == 3) ? "\n" : "");
		octeon_fpa_store(ptr, OCTEON_POOL_NO_DUMP, 0);
	}
	if (i % 4 != 3)
		printf("\n");
	printf("total = %d buffers\n", i);
	while ((ptr = octeon_fpa_load(OCTEON_POOL_NO_DUMP)) != 0)
		octeon_fpa_store(ptr, pool, 0);
}
#endif
