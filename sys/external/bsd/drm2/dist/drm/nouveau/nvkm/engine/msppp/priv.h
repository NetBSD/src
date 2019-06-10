/*	$NetBSD: priv.h,v 1.2.6.2 2019/06/10 22:08:19 christos Exp $	*/

#ifndef __NVKM_MSPPP_PRIV_H__
#define __NVKM_MSPPP_PRIV_H__
#include <engine/msppp.h>

int nvkm_msppp_new_(const struct nvkm_falcon_func *, struct nvkm_device *,
		    int index, struct nvkm_engine **);

void g98_msppp_init(struct nvkm_falcon *);
#endif
