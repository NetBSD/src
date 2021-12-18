/*	$NetBSD: priv.h,v 1.3 2021/12/18 23:45:36 riastradh Exp $	*/

/* SPDX-License-Identifier: MIT */
#ifndef __NVKM_MSPPP_PRIV_H__
#define __NVKM_MSPPP_PRIV_H__
#include <engine/msppp.h>

int nvkm_msppp_new_(const struct nvkm_falcon_func *, struct nvkm_device *,
		    int index, struct nvkm_engine **);

void g98_msppp_init(struct nvkm_falcon *);
#endif
