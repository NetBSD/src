/*	$NetBSD: npde.h,v 1.3 2021/12/18 23:45:33 riastradh Exp $	*/

/* SPDX-License-Identifier: MIT */
#ifndef __NVBIOS_NPDE_H__
#define __NVBIOS_NPDE_H__
struct nvbios_npdeT {
	u32 image_size;
	bool last;
};

u32 nvbios_npdeTe(struct nvkm_bios *, u32);
u32 nvbios_npdeTp(struct nvkm_bios *, u32, struct nvbios_npdeT *);
#endif
