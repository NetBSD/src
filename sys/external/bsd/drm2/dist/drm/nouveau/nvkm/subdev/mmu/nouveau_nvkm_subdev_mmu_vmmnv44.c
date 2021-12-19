/*	$NetBSD: nouveau_nvkm_subdev_mmu_vmmnv44.c,v 1.3 2021/12/19 10:51:58 riastradh Exp $	*/

/*
 * Copyright 2017 Red Hat Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: nouveau_nvkm_subdev_mmu_vmmnv44.c,v 1.3 2021/12/19 10:51:58 riastradh Exp $");

#include "vmm.h"

#include <subdev/timer.h>

static void
nv44_vmm_pgt_fill(struct nvkm_vmm *vmm, struct nvkm_mmu_pt *pt,
		  dma_addr_t *list, u32 ptei, u32 ptes)
{
	u32 pteo = (ptei << 2) & ~0x0000000f;
	u32 tmp[4];

	tmp[0] = nvkm_ro32(pt->memory, pteo + 0x0);
	tmp[1] = nvkm_ro32(pt->memory, pteo + 0x4);
	tmp[2] = nvkm_ro32(pt->memory, pteo + 0x8);
	tmp[3] = nvkm_ro32(pt->memory, pteo + 0xc);

	while (ptes--) {
		u32 addr = (list ? *list++ : vmm->null) >> 12;
		switch (ptei++ & 0x3) {
		case 0:
			tmp[0] &= ~0x07ffffff;
			tmp[0] |= addr;
			break;
		case 1:
			tmp[0] &= ~0xf8000000;
			tmp[0] |= addr << 27;
			tmp[1] &= ~0x003fffff;
			tmp[1] |= addr >> 5;
			break;
		case 2:
			tmp[1] &= ~0xffc00000;
			tmp[1] |= addr << 22;
			tmp[2] &= ~0x0001ffff;
			tmp[2] |= addr >> 10;
			break;
		case 3:
			tmp[2] &= ~0xfffe0000;
			tmp[2] |= addr << 17;
			tmp[3] &= ~0x00000fff;
			tmp[3] |= addr >> 15;
			break;
		}
	}

	VMM_WO032(pt, vmm, pteo + 0x0, tmp[0]);
	VMM_WO032(pt, vmm, pteo + 0x4, tmp[1]);
	VMM_WO032(pt, vmm, pteo + 0x8, tmp[2]);
	VMM_WO032(pt, vmm, pteo + 0xc, tmp[3] | 0x40000000);
}

static void __unused
nv44_vmm_pgt_pte(struct nvkm_vmm *vmm, struct nvkm_mmu_pt *pt,
		 u32 ptei, u32 ptes, struct nvkm_vmm_map *map, u64 addr)
{
	dma_addr_t tmp[4], i;

	if (ptei & 3) {
		const u32 pten = min(ptes, 4 - (ptei & 3));
		for (i = 0; i < pten; i++, addr += 0x1000)
			tmp[i] = addr;
		nv44_vmm_pgt_fill(vmm, pt, tmp, ptei, pten);
		ptei += pten;
		ptes -= pten;
	}

	while (ptes >= 4) {
		for (i = 0; i < 4; i++, addr += 0x1000)
			tmp[i] = addr >> 12;
		VMM_WO032(pt, vmm, ptei++ * 4, tmp[0] >>  0 | tmp[1] << 27);
		VMM_WO032(pt, vmm, ptei++ * 4, tmp[1] >>  5 | tmp[2] << 22);
		VMM_WO032(pt, vmm, ptei++ * 4, tmp[2] >> 10 | tmp[3] << 17);
		VMM_WO032(pt, vmm, ptei++ * 4, tmp[3] >> 15 | 0x40000000);
		ptes -= 4;
	}

	if (ptes) {
		for (i = 0; i < ptes; i++, addr += 0x1000)
			tmp[i] = addr;
		nv44_vmm_pgt_fill(vmm, pt, tmp, ptei, ptes);
	}
}

#ifndef __NetBSD__
static void
nv44_vmm_pgt_sgl(struct nvkm_vmm *vmm, struct nvkm_mmu_pt *pt,
		 u32 ptei, u32 ptes, struct nvkm_vmm_map *map)
{
	VMM_MAP_ITER_SGL(vmm, pt, ptei, ptes, map, nv44_vmm_pgt_pte);
}
#endif

static void
nv44_vmm_pgt_dma(struct nvkm_vmm *vmm, struct nvkm_mmu_pt *pt,
		 u32 ptei, u32 ptes, struct nvkm_vmm_map *map)
{
#if PAGE_SHIFT == 12
	nvkm_kmap(pt->memory);
	if (ptei & 3) {
		const u32 pten = min(ptes, 4 - (ptei & 3));
		nv44_vmm_pgt_fill(vmm, pt, map->dma, ptei, pten);
		ptei += pten;
		ptes -= pten;
		map->dma += pten;
	}

	while (ptes >= 4) {
		u32 tmp[4], i;
		for (i = 0; i < 4; i++)
			tmp[i] = *map->dma++ >> 12;
		VMM_WO032(pt, vmm, ptei++ * 4, tmp[0] >>  0 | tmp[1] << 27);
		VMM_WO032(pt, vmm, ptei++ * 4, tmp[1] >>  5 | tmp[2] << 22);
		VMM_WO032(pt, vmm, ptei++ * 4, tmp[2] >> 10 | tmp[3] << 17);
		VMM_WO032(pt, vmm, ptei++ * 4, tmp[3] >> 15 | 0x40000000);
		ptes -= 4;
	}

	if (ptes) {
		nv44_vmm_pgt_fill(vmm, pt, map->dma, ptei, ptes);
		map->dma += ptes;
	}
	nvkm_done(pt->memory);
#else
	VMM_MAP_ITER_DMA(vmm, pt, ptei, ptes, map, nv44_vmm_pgt_pte);
#endif
}

static void
nv44_vmm_pgt_unmap(struct nvkm_vmm *vmm,
		   struct nvkm_mmu_pt *pt, u32 ptei, u32 ptes)
{
	nvkm_kmap(pt->memory);
	if (ptei & 3) {
		const u32 pten = min(ptes, 4 - (ptei & 3));
		nv44_vmm_pgt_fill(vmm, pt, NULL, ptei, pten);
		ptei += pten;
		ptes -= pten;
	}

	while (ptes > 4) {
		VMM_WO032(pt, vmm, ptei++ * 4, 0x00000000);
		VMM_WO032(pt, vmm, ptei++ * 4, 0x00000000);
		VMM_WO032(pt, vmm, ptei++ * 4, 0x00000000);
		VMM_WO032(pt, vmm, ptei++ * 4, 0x00000000);
		ptes -= 4;
	}

	if (ptes)
		nv44_vmm_pgt_fill(vmm, pt, NULL, ptei, ptes);
	nvkm_done(pt->memory);
}

static const struct nvkm_vmm_desc_func
nv44_vmm_desc_pgt = {
	.unmap = nv44_vmm_pgt_unmap,
	.dma = nv44_vmm_pgt_dma,
#ifndef __NetBSD__
	.sgl = nv44_vmm_pgt_sgl,
#endif
};

static const struct nvkm_vmm_desc
nv44_vmm_desc_12[] = {
	{ PGT, 17, 4, 0x80000, &nv44_vmm_desc_pgt },
	{}
};

static void
nv44_vmm_flush(struct nvkm_vmm *vmm, int level)
{
	struct nvkm_device *device = vmm->mmu->subdev.device;
	nvkm_wr32(device, 0x100814, vmm->limit - 4096);
	nvkm_wr32(device, 0x100808, 0x000000020);
	nvkm_msec(device, 2000,
		if (nvkm_rd32(device, 0x100808) & 0x00000001)
			break;
	);
	nvkm_wr32(device, 0x100808, 0x00000000);
}

static const struct nvkm_vmm_func
nv44_vmm = {
	.valid = nv04_vmm_valid,
	.flush = nv44_vmm_flush,
	.page = {
		{ 12, &nv44_vmm_desc_12[0], NVKM_VMM_PAGE_HOST },
		{}
	}
};

int
nv44_vmm_new(struct nvkm_mmu *mmu, bool managed, u64 addr, u64 size,
	     void *argv, u32 argc, struct lock_class_key *key, const char *name,
	     struct nvkm_vmm **pvmm)
{
	struct nvkm_subdev *subdev = &mmu->subdev;
	struct nvkm_vmm *vmm;
	int ret;

	ret = nv04_vmm_new_(&nv44_vmm, mmu, 0, managed, addr, size,
			    argv, argc, key, name, &vmm);
	*pvmm = vmm;
	if (ret)
		return ret;

#ifdef __NetBSD__
    do {
	const bus_dma_tag_t dmat =
	    subdev->device->func->dma_tag(subdev->device);
	const unsigned nullsz = 16 * 1024;
	int nsegs;

	/* XXX errno NetBSD->Linux */
	ret = -bus_dmamem_alloc(dmat, nullsz, PAGE_SIZE, 0,
	    &vmm->nullseg, 1, &nsegs, BUS_DMA_WAITOK);
	if (ret) {
fail0:		vmm->nullp = NULL;
		break;
	}
	KASSERT(nsegs == 1);

	/* XXX errno NetBSD->Linux */
	ret = -bus_dmamap_create(dmat, nullsz /* size */, 1 /* maxnseg */,
	    nullsz /* maxsegsz */, 0, BUS_DMA_WAITOK, &vmm->nullmap);
	if (ret) {
fail1:		bus_dmamem_free(dmat, &vmm->nullseg, 1);
		goto fail0;
	}

	/* XXX errno NetBSD->Linux */
	ret = -bus_dmamem_map(dmat, &vmm->nullseg, 1, nullsz,
	    &vmm->nullp, BUS_DMA_WAITOK);
	if (ret) {
fail2:		bus_dmamap_destroy(dmat, vmm->nullmap);
		goto fail1;
	}

	/* XXX errno NetBSD->Linux */
	ret = -bus_dmamap_load(dmat, vmm->nullmap, vmm->nullp, nullsz,
	    NULL, BUS_DMA_WAITOK);
	if (ret) {
fail3: __unused	bus_dmamem_unmap(dmat, vmm->nullp, nullsz);
		goto fail2;
	}
	vmm->null = vmm->nullmap->dm_segs[0].ds_addr;
    } while (0);
#else
	vmm->nullp = dma_alloc_coherent(subdev->device->dev, 16 * 1024,
					&vmm->null, GFP_KERNEL);
#endif
	if (!vmm->nullp) {
		nvkm_warn(subdev, "unable to allocate dummy pages\n");
		vmm->null = 0;
	}

	return 0;
}
