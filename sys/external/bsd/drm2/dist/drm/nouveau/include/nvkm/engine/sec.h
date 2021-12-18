/*	$NetBSD: sec.h,v 1.3 2021/12/18 23:45:33 riastradh Exp $	*/

/* SPDX-License-Identifier: MIT */
#ifndef __NVKM_SEC_H__
#define __NVKM_SEC_H__
#include <engine/falcon.h>
int g98_sec_new(struct nvkm_device *, int, struct nvkm_engine **);
#endif
