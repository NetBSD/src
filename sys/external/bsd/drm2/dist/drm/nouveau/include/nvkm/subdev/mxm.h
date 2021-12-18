/*	$NetBSD: mxm.h,v 1.3 2021/12/18 23:45:33 riastradh Exp $	*/

/* SPDX-License-Identifier: MIT */
#ifndef __NVKM_MXM_H__
#define __NVKM_MXM_H__
#include <core/subdev.h>

int nv50_mxm_new(struct nvkm_device *, int, struct nvkm_subdev **);
#endif
