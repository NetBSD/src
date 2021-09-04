/* $NetBSD: apple_dart.c,v 1.2 2021/09/04 12:35:31 jmcneill Exp $ */

/*-
 * Copyright (c) 2021 Mark Kettenis <kettenis@openbsd.org>
 * Copyright (c) 2021 Jared McNeill <jmcneill@invisible.ca>
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

//#define APPLE_DART_DEBUG

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: apple_dart.c,v 1.2 2021/09/04 12:35:31 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/kmem.h>
#include <sys/vmem.h>

#include <arm/cpufunc.h>

#include <dev/fdt/fdtvar.h>

/*
 * DART registers
 */
#define	DART_TLB_OP		0x0020
#define	 DART_TLB_OP_FLUSH	__BIT(20)
#define	 DART_TLB_OP_BUSY	__BIT(2)
#define	DART_TLB_OP_SIDMASK	0x0034
#define	DART_ERR_STATUS		0x0040
#define	DART_ERR_ADDRL		0x0050
#define	DART_ERR_ADDRH		0x0054
#define	DART_CONFIG(sid)	(0x0100 + (sid) * 0x4)
#define	 DART_CONFIG_TXEN	__BIT(7)
#define	DART_TTBR(sid, idx)	(0x0200 + (sid) * 0x10 + (idx) * 0x4)
#define	 DART_TTBR_VALID	__BIT(31)
#define	 DART_TTBR_SHIFT	12

#define	DART_APERTURE_START	0x00100000
#define	DART_APERTURE_SIZE	0x3fe00000
#define	DART_PAGE_SIZE		16384
#define	DART_PAGE_MASK		(DART_PAGE_SIZE - 1)

#define	DART_L1_TABLE		0xb
#define	DART_L2_INVAL		0x0
#define	DART_L2_PAGE		0x3

#define	DART_ROUND_PAGE(pa)	(((pa) + DART_PAGE_MASK) & ~DART_PAGE_MASK)
#define	DART_TRUNC_PAGE(pa)	((pa) & ~DART_PAGE_MASK)

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "apple,dart-m1",		.value = 16 },
	DEVICE_COMPAT_EOL
};

static struct arm32_dma_range apple_dart_dma_ranges[] = {
	[0] = {
		.dr_sysbase = 0,
		.dr_busbase = 0,
		.dr_len = UINTPTR_MAX,
		.dr_flags = _BUS_DMAMAP_COHERENT,
	}
};

struct apple_dart_map_state {
	bus_addr_t ams_dva;
	bus_size_t ams_len;
};

struct apple_dart_dma {
	bus_dmamap_t dma_map;
	bus_dma_segment_t dma_seg;
	bus_size_t dma_size;
	void *dma_kva;
};

#define	DART_DMA_MAP(_dma)	((_dma)->dma_map)
#define	DART_DMA_LEN(_dma)	((_dma)->dma_size)
#define	DART_DMA_DVA(_dma)	((_dma)->dma_map->dm_segs[0].ds_addr)
#define	DART_DMA_KVA(_dma)	((_dma)->dma_kva)

struct apple_dart_softc {
	device_t sc_dev;
	int sc_phandle;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	bus_dma_tag_t sc_dmat;

	uint64_t sc_sid_mask;
	u_int sc_nsid;

	vmem_t *sc_dvamap;

	struct apple_dart_dma *sc_l1;
	struct apple_dart_dma **sc_l2;
	u_int sc_nl2;

	struct arm32_bus_dma_tag sc_bus_dmat;
};

#define DART_READ(sc, reg) \
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define	DART_WRITE(sc, reg, val) \
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static void
apple_dart_flush_tlb(struct apple_dart_softc *sc)
{
	dsb(sy);
	isb();

	DART_WRITE(sc, DART_TLB_OP_SIDMASK, sc->sc_sid_mask);
	DART_WRITE(sc, DART_TLB_OP, DART_TLB_OP_FLUSH);
	while ((DART_READ(sc, DART_TLB_OP) & DART_TLB_OP_BUSY) != 0) {
		__asm volatile ("yield" ::: "memory");
	}
}

static struct apple_dart_dma *
apple_dart_dma_alloc(bus_dma_tag_t dmat, bus_size_t size, bus_size_t align)
{
	struct apple_dart_dma *dma;
	int nsegs, error;

	dma = kmem_zalloc(sizeof(*dma), KM_SLEEP);
	dma->dma_size = size;

	error = bus_dmamem_alloc(dmat, size, align, 0, &dma->dma_seg, 1,
	    &nsegs, BUS_DMA_WAITOK);
	if (error != 0) {
		goto destroy;
	}

	error = bus_dmamem_map(dmat, &dma->dma_seg, nsegs, size,
	    &dma->dma_kva, BUS_DMA_WAITOK | BUS_DMA_NOCACHE);
	if (error != 0) {
		goto free;
	}

	error = bus_dmamap_create(dmat, size, 1, size, 0,
	    BUS_DMA_WAITOK | BUS_DMA_ALLOCNOW, &dma->dma_map);
	if (error != 0) {
		goto dmafree;
	}

	error = bus_dmamap_load(dmat, dma->dma_map, dma->dma_kva, size,
	    NULL, BUS_DMA_WAITOK);
	if (error != 0) {
		goto unmap;
	}

	memset(dma->dma_kva, 0, size);

	return dma;

destroy:
	bus_dmamap_destroy(dmat, dma->dma_map);
unmap:
	bus_dmamem_unmap(dmat, dma->dma_kva, size);
free:
	bus_dmamem_free(dmat, &dma->dma_seg, 1);
dmafree:
	kmem_free(dma, sizeof(*dma));
	return NULL;
}

static int
apple_dart_intr(void *priv)
{
	struct apple_dart_softc * const sc = priv;
	char fdt_path[128];
	uint64_t addr;
	uint32_t status;

	status = DART_READ(sc, DART_ERR_STATUS);
	addr = DART_READ(sc, DART_ERR_ADDRL);
	addr |= (uint64_t)DART_READ(sc, DART_ERR_ADDRH) << 32;
	DART_WRITE(sc, DART_ERR_STATUS, status);

	fdtbus_get_path(sc->sc_phandle, fdt_path, sizeof(fdt_path));

	printf("%s (%s): error addr 0x%016lx status 0x%08x\n",
	    device_xname(sc->sc_dev), fdt_path, addr, status);

	return 1;
}

static volatile uint64_t *
apple_dart_lookup_tte(struct apple_dart_softc *sc, bus_addr_t dva)
{
	int idx = dva / DART_PAGE_SIZE;
	int l2_idx = idx / (DART_PAGE_SIZE / sizeof(uint64_t));
	int tte_idx = idx % (DART_PAGE_SIZE / sizeof(uint64_t));
	volatile uint64_t *l2;

	l2 = DART_DMA_KVA(sc->sc_l2[l2_idx]);
	return &l2[tte_idx];
}

static void
apple_dart_unload_map(struct apple_dart_softc *sc, bus_dmamap_t map)
{
	struct apple_dart_map_state *ams = map->_dm_iommu;
	volatile uint64_t *tte;
	int seg;

	/* For each segment */
	for (seg = 0; seg < map->dm_nsegs; seg++) {
		u_long len, dva;

		if (ams[seg].ams_len == 0) {
			continue;
		}

		dva = ams[seg].ams_dva;
		len = ams[seg].ams_len;

		while (len > 0) {
			tte = apple_dart_lookup_tte(sc, dva);
			*tte = DART_L2_INVAL;

			dva += DART_PAGE_SIZE;
			len -= DART_PAGE_SIZE;
		}

		vmem_xfree(sc->sc_dvamap, ams[seg].ams_dva, ams[seg].ams_len);

		ams[seg].ams_dva = 0;
		ams[seg].ams_len = 0;
	}

	apple_dart_flush_tlb(sc);
}

static int
apple_dart_load_map(struct apple_dart_softc *sc, bus_dmamap_t map)
{
	struct apple_dart_map_state *ams = map->_dm_iommu;
	volatile uint64_t *tte;
	int seg, error;

	/* For each segment */
	for (seg = 0; seg < map->dm_nsegs; seg++) {
		paddr_t pa = map->dm_segs[seg]._ds_paddr;
		psize_t off = pa - DART_TRUNC_PAGE(pa);
		u_long len, dva;

		len = DART_ROUND_PAGE(map->dm_segs[seg].ds_len + off);

#ifdef APPLE_DART_DEBUG
		device_printf(sc->sc_dev, "load pa=%#lx off=%lu len=%lu ",
		    pa, off, len);
#endif

		error = vmem_xalloc(sc->sc_dvamap, len, DART_PAGE_SIZE, 0,
		    0, VMEM_ADDR_MIN, VMEM_ADDR_MAX, VM_BESTFIT|VM_NOSLEEP,
		    &dva);
		if (error != 0) {
			apple_dart_unload_map(sc, map);
#ifdef APPLE_DART_DEBUG
			printf("error=%d\n", error);
#endif
			return error;
		}

#ifdef APPLE_DART_DEBUG
		printf("dva=%#lx\n", dva);
#endif

		ams[seg].ams_dva = dva;
		ams[seg].ams_len = len;

		map->dm_segs[seg].ds_addr = dva + off;

		pa = DART_TRUNC_PAGE(pa);
		while (len > 0) {
			tte = apple_dart_lookup_tte(sc, dva);
			*tte = pa | DART_L2_PAGE;

			pa += DART_PAGE_SIZE;
			dva += DART_PAGE_SIZE;
			len -= DART_PAGE_SIZE;
		}
	}

	apple_dart_flush_tlb(sc);

	return 0;
}

static int
apple_dart_dmamap_create(bus_dma_tag_t t, bus_size_t size, int nsegments,
    bus_size_t maxsegsz, bus_size_t boundary, int flags, bus_dmamap_t *dmamap)
{
	struct apple_dart_softc *sc = t->_cookie;
	struct apple_dart_map_state *ams;
	bus_dmamap_t map;
	int error;

	error = sc->sc_dmat->_dmamap_create(sc->sc_dmat, size, nsegments,
	    maxsegsz, boundary, flags, &map);
	if (error != 0) {
		return error;
	}

	ams = kmem_zalloc(map->_dm_segcnt * sizeof(*ams),
	    (flags & BUS_DMA_NOWAIT) != 0 ? KM_NOSLEEP : KM_SLEEP);
	if (ams == NULL) {
		sc->sc_dmat->_dmamap_destroy(sc->sc_dmat, map);
		return ENOMEM;
	}

	map->_dm_iommu = ams;
	*dmamap = map;
	return 0;
}

static void
apple_dart_dmamap_destroy(bus_dma_tag_t t, bus_dmamap_t map)
{
	struct apple_dart_softc *sc = t->_cookie;
	struct apple_dart_map_state *ams = map->_dm_iommu;

	kmem_free(ams, map->_dm_segcnt * sizeof(*ams));
	sc->sc_dmat->_dmamap_destroy(sc->sc_dmat, map);
}

static int
apple_dart_dmamap_load(bus_dma_tag_t t, bus_dmamap_t map, void *buf,
    size_t buflen, struct proc *p, int flags)
{
	struct apple_dart_softc *sc = t->_cookie;
	int error;

	error = sc->sc_dmat->_dmamap_load(sc->sc_dmat, map,
	    buf, buflen, p, flags);
	if (error != 0) {
		return error;
	}

	error = apple_dart_load_map(sc, map);
	if (error != 0) {
		sc->sc_dmat->_dmamap_unload(sc->sc_dmat, map);
	}

	return error;
}

static int
apple_dart_dmamap_load_mbuf(bus_dma_tag_t t, bus_dmamap_t map,
    struct mbuf *m, int flags)
{
	struct apple_dart_softc *sc = t->_cookie;
	int error;

	error = sc->sc_dmat->_dmamap_load_mbuf(sc->sc_dmat, map,
	    m, flags);
	if (error != 0) {
		return error;
	}

	error = apple_dart_load_map(sc, map);
	if (error != 0) {
		sc->sc_dmat->_dmamap_unload(sc->sc_dmat, map);
	}

	return error;
}

static int
apple_dart_dmamap_load_uio(bus_dma_tag_t t, bus_dmamap_t map,
    struct uio *uio, int flags)
{
	struct apple_dart_softc *sc = t->_cookie;
	int error;

	error = sc->sc_dmat->_dmamap_load_uio(sc->sc_dmat, map,
	    uio, flags);
	if (error != 0) {
		return error;
	}

	error = apple_dart_load_map(sc, map);
	if (error != 0) {
		sc->sc_dmat->_dmamap_unload(sc->sc_dmat, map);
	}

	return error;
}

static int
apple_dart_dmamap_load_raw(bus_dma_tag_t t, bus_dmamap_t map,
    bus_dma_segment_t *segs, int nsegs, bus_size_t size, int flags)
{
	struct apple_dart_softc *sc = t->_cookie;
	int error;

	error = sc->sc_dmat->_dmamap_load_raw(sc->sc_dmat, map,
	    segs, nsegs, size, flags);
	if (error != 0) {
		return error;
	}

	error = apple_dart_load_map(sc, map);
	if (error != 0) {
		sc->sc_dmat->_dmamap_unload(sc->sc_dmat, map);
	}

	return error;
}

static void
apple_dart_dmamap_unload(bus_dma_tag_t t, bus_dmamap_t map)
{
	struct apple_dart_softc *sc = t->_cookie;

	apple_dart_unload_map(sc, map);
	sc->sc_dmat->_dmamap_unload(sc->sc_dmat, map);
}

static bus_dma_tag_t
apple_dart_iommu_map(device_t dev, const u_int *data, bus_dma_tag_t dmat)
{
	struct apple_dart_softc * const sc = device_private(dev);

	return &sc->sc_bus_dmat;
}

const struct fdtbus_iommu_func apple_dart_iommu_funcs = {
	.map = apple_dart_iommu_map,
};

static int
apple_dart_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
apple_dart_attach(device_t parent, device_t self, void *aux)
{
	struct apple_dart_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	uint64_t sidmask64;
	uint32_t sidmask32;
	char intrstr[128];
	volatile uint64_t *l1;
	bus_addr_t addr;
	bus_size_t size;
	u_int sid, idx;
	paddr_t pa;
	void *ih;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}
	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error(": couldn't decode interrupt\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_phandle = phandle;
	sc->sc_dmat = faa->faa_dmat;
	sc->sc_bst = faa->faa_bst;
	if (bus_space_map(sc->sc_bst, addr, size,
	    _ARM_BUS_SPACE_MAP_STRONGLY_ORDERED, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}
	sc->sc_nsid = of_compatible_lookup(phandle, compat_data)->value;

	if (of_getprop_uint64(phandle, "sid-mask", &sidmask64) == 0) {
		sc->sc_sid_mask = sidmask64;
	} else if (of_getprop_uint32(phandle, "sid-mask", &sidmask32) == 0) {
		sc->sc_sid_mask = sidmask32;
	} else {
		sc->sc_sid_mask = 0xffff;
	}

	aprint_naive("\n");
	aprint_normal(": Apple DART @ %#lx/%#lx, %u SIDs (mask 0x%lx)\n",
	    addr, size, sc->sc_nsid, sc->sc_sid_mask);

	KASSERT(sc->sc_nsid == 16);
	KASSERT(sc->sc_sid_mask == 0xffff);

	sc->sc_dvamap = vmem_create(device_xname(self),
	    DART_APERTURE_START, DART_APERTURE_SIZE, DART_PAGE_SIZE,
	    NULL, NULL, NULL, 0, VM_SLEEP, IPL_HIGH);
	if (sc->sc_dvamap == NULL) {
		aprint_error_dev(self, "couldn't allocate DVA map\n");
		return;
	}

	/* Disable translations */
	for (sid = 0; sid < sc->sc_nsid; sid++) {
		DART_WRITE(sc, DART_CONFIG(sid), 0);
	}

	/* Remove page tables */
	for (sid = 0; sid < sc->sc_nsid; sid++) {
		for (idx = 0; idx < 4; idx++) {
			DART_WRITE(sc, DART_TTBR(sid, idx), 0);
		}
	}
	apple_dart_flush_tlb(sc);

	/*
	 * Build translation tables. We pre-allocate the translation
	 * tables for the entire aperture such that we don't have to worry
	 * about growing them in an mpsafe manner later.
	 */

	const u_int ntte = howmany(DART_APERTURE_START + DART_APERTURE_SIZE - 1,
				   DART_PAGE_SIZE);
	const u_int nl2 = howmany(ntte, DART_PAGE_SIZE / sizeof(uint64_t));
	const u_int nl1 = howmany(nl2, DART_PAGE_SIZE / sizeof(uint64_t));

	sc->sc_l1 = apple_dart_dma_alloc(sc->sc_dmat,
	    nl1 * DART_PAGE_SIZE, DART_PAGE_SIZE);
	if (sc->sc_l1 == NULL) {
		aprint_error_dev(self, "couldn't allocate L1 tables\n");
		return;
	}
	sc->sc_l2 = kmem_zalloc(nl2 * sizeof(*sc->sc_l2), KM_SLEEP);
	sc->sc_nl2 = nl2;

	l1 = DART_DMA_KVA(sc->sc_l1);
	for (idx = 0; idx < nl2; idx++) {
		sc->sc_l2[idx] = apple_dart_dma_alloc(sc->sc_dmat,
		    DART_PAGE_SIZE, DART_PAGE_SIZE);
		if (sc->sc_l2[idx] == NULL) {
			aprint_error_dev(self,
			    "couldn't allocate L2 tables\n");
			return;
		}
		l1[idx] = DART_DMA_DVA(sc->sc_l2[idx]) | DART_L1_TABLE;
	}

	/* Install page tables */
	for (sid = 0; sid < sc->sc_nsid; sid++) {
		pa = DART_DMA_DVA(sc->sc_l1);
		for (idx = 0; idx < nl1; idx++) {
			DART_WRITE(sc, DART_TTBR(sid, idx),
			    (pa >> DART_TTBR_SHIFT) | DART_TTBR_VALID);
			pa += DART_PAGE_SIZE;
		}
	}
	apple_dart_flush_tlb(sc);

	/* Enable translations */
	for (sid = 0; sid < sc->sc_nsid; sid++) {
		DART_WRITE(sc, DART_CONFIG(sid), DART_CONFIG_TXEN);
	}

	ih = fdtbus_intr_establish_xname(phandle, 0, IPL_HIGH, FDT_INTR_MPSAFE,
	    apple_dart_intr, sc, device_xname(self));
	if (ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt on %s\n",
		    intrstr);
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	/* Setup bus DMA tag */
	sc->sc_bus_dmat = *sc->sc_dmat;
	sc->sc_bus_dmat._ranges = apple_dart_dma_ranges;
	sc->sc_bus_dmat._nranges = 1;
	sc->sc_bus_dmat._cookie = sc;
	sc->sc_bus_dmat._dmamap_create = apple_dart_dmamap_create;
	sc->sc_bus_dmat._dmamap_destroy = apple_dart_dmamap_destroy;
	sc->sc_bus_dmat._dmamap_load = apple_dart_dmamap_load;
	sc->sc_bus_dmat._dmamap_load_mbuf = apple_dart_dmamap_load_mbuf;
	sc->sc_bus_dmat._dmamap_load_uio = apple_dart_dmamap_load_uio;
	sc->sc_bus_dmat._dmamap_load_raw = apple_dart_dmamap_load_raw;
	sc->sc_bus_dmat._dmamap_unload = apple_dart_dmamap_unload;

	fdtbus_register_iommu(self, phandle, &apple_dart_iommu_funcs);
}

CFATTACH_DECL_NEW(apple_dart, sizeof(struct apple_dart_softc),
	apple_dart_match, apple_dart_attach, NULL, NULL);
