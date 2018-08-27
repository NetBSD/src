/*	$NetBSD: bsp.h,v 1.1.1.1 2018/08/27 01:34:55 riastradh Exp $	*/

#ifndef __NVKM_BSP_H__
#define __NVKM_BSP_H__
#include <engine/xtensa.h>
int g84_bsp_new(struct nvkm_device *, int, struct nvkm_engine **);
#endif
