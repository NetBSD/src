/*	$NetBSD: lut.h,v 1.3 2021/12/19 10:49:47 riastradh Exp $	*/

#ifndef __NV50_KMS_LUT_H__
#define __NV50_KMS_LUT_H__
#include <nvif/mem.h>
struct drm_property_blob;
struct drm_color_lut;
struct nv50_disp;

struct nv50_lut {
	struct nvif_mem mem[2];
};

int nv50_lut_init(struct nv50_disp *, struct nvif_mmu *, struct nv50_lut *);
void nv50_lut_fini(struct nv50_lut *);
#ifdef __NetBSD__
#  define	__lut_iomem	volatile
#  define	__iomem		__lut_iomem
#endif
u32 nv50_lut_load(struct nv50_lut *, int buffer, struct drm_property_blob *,
		  void (*)(struct drm_color_lut *, int size, void __iomem *));
#ifdef __NetBSD__
#  undef	__iomem
#endif
#endif
