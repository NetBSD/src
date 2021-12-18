/*	$NetBSD: ioctl.h,v 1.1.1.2 2021/12/18 20:23:00 riastradh Exp $	*/

/* SPDX-License-Identifier: MIT */
#ifndef __NVKM_IOCTL_H__
#define __NVKM_IOCTL_H__
#include <core/os.h>
struct nvkm_client;

int nvkm_ioctl(struct nvkm_client *, bool, void *, u32, void **);
#endif
