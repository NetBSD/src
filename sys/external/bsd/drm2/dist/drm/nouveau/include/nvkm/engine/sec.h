/*	$NetBSD: sec.h,v 1.1.1.2 2021/12/18 20:15:37 riastradh Exp $	*/

/* SPDX-License-Identifier: MIT */
#ifndef __NVKM_SEC_H__
#define __NVKM_SEC_H__
#include <engine/falcon.h>
int g98_sec_new(struct nvkm_device *, int, struct nvkm_engine **);
#endif
