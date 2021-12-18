/*	$NetBSD: driver.h,v 1.6 2021/12/18 23:45:33 riastradh Exp $	*/

/* SPDX-License-Identifier: MIT */
#ifndef __NVIF_DRIVER_H__
#define __NVIF_DRIVER_H__
#include <nvif/os.h>
struct nvif_client;

#ifdef __NetBSD__
#  define	__nvif_iomem	volatile
#  define	__iomem		__nvif_iomem
#endif

struct nvif_driver {
	const char *name;
	int (*init)(const char *name, u64 device, const char *cfg,
		    const char *dbg, void **priv);
	void (*fini)(void *priv);
	int (*suspend)(void *priv);
	int (*resume)(void *priv);
	int (*ioctl)(void *priv, bool super, void *data, u32 size, void **hack);
#ifdef __NetBSD__
	int (*map)(void *priv, bus_space_tag_t tag, u64 handle, u32 size,
	    bus_space_handle_t *handlep, void __iomem **ptrp);
	void (*unmap)(void *priv, bus_space_tag_t tag,
	    bus_space_handle_t handle, bus_addr_t addr, void __iomem *ptr,
	    u32 size);
#else
	void __iomem *(*map)(void *priv, u64 handle, u32 size);
	void (*unmap)(void *priv, void __iomem *ptr, u32 size);
#endif
	bool keep;
};

#ifdef __NetBSD__
#  undef	__iomem
#endif
int nvif_driver_init(const char *drv, const char *cfg, const char *dbg,
		     const char *name, u64 device, struct nvif_client *);

extern const struct nvif_driver nvif_driver_nvkm;
extern const struct nvif_driver nvif_driver_drm;
extern const struct nvif_driver nvif_driver_lib;
extern const struct nvif_driver nvif_driver_null;
#endif
