/*	$NetBSD: ramnv40.h,v 1.2.6.2 2019/06/10 22:08:21 christos Exp $	*/

#ifndef __NV40_FB_RAM_H__
#define __NV40_FB_RAM_H__
#define nv40_ram(p) container_of((p), struct nv40_ram, base)
#include "ram.h"

struct nv40_ram {
	struct nvkm_ram base;
	u32 ctrl;
	u32 coef;
};

int nv40_ram_new_(struct nvkm_fb *fb, enum nvkm_ram_type, u64, u32,
		  struct nvkm_ram **);
#endif
