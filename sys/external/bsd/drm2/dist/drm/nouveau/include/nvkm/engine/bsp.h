/*	$NetBSD: bsp.h,v 1.2.2.2 2018/09/06 06:56:24 pgoyette Exp $	*/

#ifndef __NVKM_BSP_H__
#define __NVKM_BSP_H__
#include <engine/xtensa.h>
int g84_bsp_new(struct nvkm_device *, int, struct nvkm_engine **);
#endif
