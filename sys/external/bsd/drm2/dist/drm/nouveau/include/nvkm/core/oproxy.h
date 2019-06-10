/*	$NetBSD: oproxy.h,v 1.2.6.2 2019/06/10 22:08:14 christos Exp $	*/

#ifndef __NVKM_OPROXY_H__
#define __NVKM_OPROXY_H__
#define nvkm_oproxy(p) container_of((p), struct nvkm_oproxy, base)
#include <core/object.h>

struct nvkm_oproxy {
	const struct nvkm_oproxy_func *func;
	struct nvkm_object base;
	struct nvkm_object *object;
};

struct nvkm_oproxy_func {
	void (*dtor[2])(struct nvkm_oproxy *);
	int  (*init[2])(struct nvkm_oproxy *);
	int  (*fini[2])(struct nvkm_oproxy *, bool suspend);
};

void nvkm_oproxy_ctor(const struct nvkm_oproxy_func *,
		      const struct nvkm_oclass *, struct nvkm_oproxy *);
int  nvkm_oproxy_new_(const struct nvkm_oproxy_func *,
		      const struct nvkm_oclass *, struct nvkm_oproxy **);
#endif
