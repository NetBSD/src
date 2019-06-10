/*	$NetBSD: bsp.h,v 1.2.6.2 2019/06/10 22:08:14 christos Exp $	*/

#ifndef __NVKM_BSP_H__
#define __NVKM_BSP_H__
#include <engine/xtensa.h>
int g84_bsp_new(struct nvkm_device *, int, struct nvkm_engine **);
#endif
