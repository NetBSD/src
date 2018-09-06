/*	$NetBSD: priv.h,v 1.2.2.2 2018/09/06 06:56:27 pgoyette Exp $	*/

#ifndef __NVKM_MSVLD_PRIV_H__
#define __NVKM_MSVLD_PRIV_H__
#include <engine/msvld.h>

int nvkm_msvld_new_(const struct nvkm_falcon_func *, struct nvkm_device *,
		    int index, struct nvkm_engine **);

void g98_msvld_init(struct nvkm_falcon *);

void gf100_msvld_init(struct nvkm_falcon *);
#endif
