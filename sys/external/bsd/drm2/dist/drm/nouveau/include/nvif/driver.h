/*	$NetBSD: driver.h,v 1.3 2018/08/27 07:32:59 riastradh Exp $	*/

#ifndef __NVIF_DRIVER_H__
#define __NVIF_DRIVER_H__

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
	void __iomem *(*map)(void *priv, u64 handle, u32 size);
	void (*unmap)(void *priv, void __iomem *ptr, u32 size);
	bool keep;
};

#ifdef __NetBSD__
#  undef	__iomem
#endif

extern const struct nvif_driver nvif_driver_nvkm;
extern const struct nvif_driver nvif_driver_drm;
extern const struct nvif_driver nvif_driver_lib;
extern const struct nvif_driver nvif_driver_null;

#endif
