/*	$NetBSD: gsp.h,v 1.1.1.1 2021/12/18 20:15:37 riastradh Exp $	*/

#ifndef __NVKM_GSP_H__
#define __NVKM_GSP_H__
#define nvkm_gsp(p) container_of((p), struct nvkm_gsp, subdev)
#include <core/subdev.h>
#include <core/falcon.h>

struct nvkm_gsp {
	struct nvkm_subdev subdev;
	struct nvkm_falcon falcon;
};

int gv100_gsp_new(struct nvkm_device *, int, struct nvkm_gsp **);
#endif
