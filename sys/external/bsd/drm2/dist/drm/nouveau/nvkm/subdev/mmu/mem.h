/*	$NetBSD: mem.h,v 1.3 2021/12/19 10:51:58 riastradh Exp $	*/

#ifndef __NVKM_MEM_H__
#define __NVKM_MEM_H__
#include "priv.h"

int nvkm_mem_new_type(struct nvkm_mmu *, int type, u8 page, u64 size,
		      void *argv, u32 argc, struct nvkm_memory **);
#ifdef __NetBSD__
int nvkm_mem_map_host(struct nvkm_memory *, bus_dma_tag_t *, void **pmap,
    bus_size_t *);
#else
int nvkm_mem_map_host(struct nvkm_memory *, void **pmap);
#endif

int nv04_mem_new(struct nvkm_mmu *, int, u8, u64, void *, u32,
		 struct nvkm_memory **);
#ifdef __NetBSD__
int nv04_mem_map(struct nvkm_mmu *, struct nvkm_memory *, void *, u32,
		 bus_space_tag_t *, u64 *, u64 *, struct nvkm_vma **);
#else
int nv04_mem_map(struct nvkm_mmu *, struct nvkm_memory *, void *, u32,
		 u64 *, u64 *, struct nvkm_vma **);
#endif

int nv50_mem_new(struct nvkm_mmu *, int, u8, u64, void *, u32,
		 struct nvkm_memory **);
#ifdef __NetBSD__
int nv50_mem_map(struct nvkm_mmu *, struct nvkm_memory *, void *, u32,
		 bus_space_tag_t *, u64 *, u64 *, struct nvkm_vma **);
#else
int nv50_mem_map(struct nvkm_mmu *, struct nvkm_memory *, void *, u32,
		 u64 *, u64 *, struct nvkm_vma **);
#endif

int gf100_mem_new(struct nvkm_mmu *, int, u8, u64, void *, u32,
		  struct nvkm_memory **);
#ifdef __NetBSD__
int gf100_mem_map(struct nvkm_mmu *, struct nvkm_memory *, void *, u32,
		  bus_space_tag_t *, u64 *, u64 *, struct nvkm_vma **);
#else
int gf100_mem_map(struct nvkm_mmu *, struct nvkm_memory *, void *, u32,
		  u64 *, u64 *, struct nvkm_vma **);
#endif
#endif
