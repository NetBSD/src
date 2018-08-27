/*	$NetBSD: nouveau_nvif.c,v 1.4 2018/08/27 14:47:53 riastradh Exp $	*/

/*
 * Copyright 2014 Red Hat Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors: Ben Skeggs <bskeggs@redhat.com>
 */

/*******************************************************************************
 * NVIF client driver - NVKM directly linked
 ******************************************************************************/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: nouveau_nvif.c,v 1.4 2018/08/27 14:47:53 riastradh Exp $");

#include <core/client.h>
#include <core/notify.h>
#include <core/ioctl.h>

#include <nvif/client.h>
#include <nvif/driver.h>
#include <nvif/notify.h>
#include <nvif/event.h>
#include <nvif/ioctl.h>

#include "nouveau_drm.h"
#include "nouveau_usif.h"

#ifdef __NetBSD__
#define	__iomem	__nvif_iomem
static int
nvkm_client_map(void *priv, bus_space_tag_t tag, u64 busaddr, u32 size,
    bus_space_handle_t *handlep, void __iomem **ptrp)
{
	struct nvkm_client *client = nvxx_client(priv);
	int ret;

	if (tag == client->mmiot &&
	    client->mmioaddr <= busaddr &&
	    busaddr - client->mmioaddr <= client->mmiosz) {
		const bus_size_t offset = busaddr - client->mmioaddr;
		if (size > client->mmiosz - offset) {
			DRM_ERROR("Invalid register access\n");
			return -EFAULT;
		}
		ret = -bus_space_subregion(client->mmiot, client->mmioh,
		    offset, size, handlep);
		if (ret)
			return ret;
		*ptrp = bus_space_vaddr(tag, *handlep);
		return 0;
	}

	ret = -bus_space_map(tag, busaddr, size, BUS_SPACE_MAP_LINEAR,
	    handlep);
	if (ret)
		return ret;
	*ptrp = bus_space_vaddr(tag, *handlep);
	return 0;
}

static void
nvkm_client_unmap(void *priv, bus_space_tag_t tag, bus_space_handle_t handle,
    bus_addr_t busaddr, void __iomem *ptr, u32 size)
{
	struct nvkm_client *client = nvxx_client(priv);

	KASSERTMSG(ptr == bus_space_vaddr(tag, handle),
	    "nvkm_client ptr %p != %p [bus_space_vaddr(%p, %"PRIxVADDR")]",
	    ptr, bus_space_vaddr(tag, handle), tag, handle);

	if (tag == client->mmiot &&
	    client->mmioaddr <= busaddr &&
	    busaddr - client->mmioaddr <= client->mmiosz) {
		const bus_size_t offset = busaddr - client->mmioaddr;
		KASSERT(size <= client->mmiosz - offset);
		/* Nothing to do to release a subregion.  */
		return;
	}

	bus_space_unmap(tag, handle, size);
}
#else
static void
nvkm_client_unmap(void *priv, void __iomem *ptr, u32 size)
{
	iounmap(ptr);
}

static void __iomem *
nvkm_client_map(void *priv, u64 handle, u32 size)
{
	return ioremap(handle, size);
}
#endif

static int
nvkm_client_ioctl(void *priv, bool super, void *data, u32 size, void **hack)
{
	return nvkm_ioctl(priv, super, data, size, hack);
}

static int
nvkm_client_resume(void *priv)
{
	return nvkm_client_init(priv);
}

static int
nvkm_client_suspend(void *priv)
{
	return nvkm_client_fini(priv, true);
}

static void
nvkm_client_driver_fini(void *priv)
{
	struct nvkm_client *client = priv;
	nvkm_client_del(&client);
}

static int
nvkm_client_ntfy(const void *header, u32 length, const void *data, u32 size)
{
	const union {
		struct nvif_notify_req_v0 v0;
	} *args = header;
	u8 route;

	if (length == sizeof(args->v0) && args->v0.version == 0) {
		route = args->v0.route;
	} else {
		WARN_ON(1);
		return NVKM_NOTIFY_DROP;
	}

	switch (route) {
	case NVDRM_NOTIFY_NVIF:
		return nvif_notify(header, length, data, size);
	case NVDRM_NOTIFY_USIF:
		return usif_notify(header, length, data, size);
	default:
		WARN_ON(1);
		break;
	}

	return NVKM_NOTIFY_DROP;
}

static int
nvkm_client_driver_init(const char *name, u64 device, const char *cfg,
			const char *dbg, void **ppriv)
{
	struct nvkm_client *client;
	int ret;

	ret = nvkm_client_new(name, device, cfg, dbg, &client);
	*ppriv = client;
	if (ret)
		return ret;

	client->ntfy = nvkm_client_ntfy;
	return 0;
}

const struct nvif_driver
nvif_driver_nvkm = {
	.name = "nvkm",
	.init = nvkm_client_driver_init,
	.fini = nvkm_client_driver_fini,
	.suspend = nvkm_client_suspend,
	.resume = nvkm_client_resume,
	.ioctl = nvkm_client_ioctl,
	.map = nvkm_client_map,
	.unmap = nvkm_client_unmap,
	.keep = false,
};
