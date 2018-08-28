/*	$NetBSD: priv.h,v 1.2 2018/08/27 04:58:32 riastradh Exp $	*/

#ifndef __NVKM_MSPPP_PRIV_H__
#define __NVKM_MSPPP_PRIV_H__
#include <engine/msppp.h>

int nvkm_msppp_new_(const struct nvkm_falcon_func *, struct nvkm_device *,
		    int index, struct nvkm_engine **);

void g98_msppp_init(struct nvkm_falcon *);
#endif
