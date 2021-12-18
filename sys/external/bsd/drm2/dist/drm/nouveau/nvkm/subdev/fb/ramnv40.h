/*	$NetBSD: ramnv40.h,v 1.3 2021/12/18 23:45:39 riastradh Exp $	*/

/* SPDX-License-Identifier: MIT */
#ifndef __NV40_FB_RAM_H__
#define __NV40_FB_RAM_H__
#define nv40_ram(p) container_of((p), struct nv40_ram, base)
#include "ram.h"

struct nv40_ram {
	struct nvkm_ram base;
	u32 ctrl;
	u32 coef;
};

int nv40_ram_new_(struct nvkm_fb *fb, enum nvkm_ram_type, u64,
		  struct nvkm_ram **);
#endif
