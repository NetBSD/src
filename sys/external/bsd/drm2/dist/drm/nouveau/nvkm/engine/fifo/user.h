/*	$NetBSD: user.h,v 1.1.1.1 2021/12/18 20:15:38 riastradh Exp $	*/

#ifndef __NVKM_FIFO_USER_H__
#define __NVKM_FIFO_USER_H__
#include "priv.h"
int gv100_fifo_user_new(const struct nvkm_oclass *, void *, u32,
			struct nvkm_object **);
int tu102_fifo_user_new(const struct nvkm_oclass *, void *, u32,
			struct nvkm_object **);
#endif
