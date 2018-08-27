/*	$NetBSD: ioctl.h,v 1.1.1.1 2018/08/27 01:36:13 riastradh Exp $	*/

#ifndef __NVKM_IOCTL_H__
#define __NVKM_IOCTL_H__
#include <core/os.h>
struct nvkm_client;

int nvkm_ioctl(struct nvkm_client *, bool, void *, u32, void **);
#endif
