/*	$NetBSD: ioctl.h,v 1.2.2.2 2018/09/06 06:56:23 pgoyette Exp $	*/

#ifndef __NVKM_IOCTL_H__
#define __NVKM_IOCTL_H__
#include <core/os.h>
struct nvkm_client;

int nvkm_ioctl(struct nvkm_client *, bool, void *, u32, void **);
#endif
