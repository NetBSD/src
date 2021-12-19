/*	$NetBSD: nouveau_nvkm_subdev_mmu_umem.c,v 1.3 2021/12/19 10:51:58 riastradh Exp $	*/

/*
 * Copyright 2017 Red Hat Inc.
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
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: nouveau_nvkm_subdev_mmu_umem.c,v 1.3 2021/12/19 10:51:58 riastradh Exp $");

#include "umem.h"
#include "ummu.h"

#include <core/client.h>
#include <core/memory.h>
#include <subdev/bar.h>

#include <nvif/class.h>
#include <nvif/if000a.h>
#include <nvif/unpack.h>

static const struct nvkm_object_func nvkm_umem;
struct nvkm_memory *
nvkm_umem_search(struct nvkm_client *client, u64 handle)
{
	struct nvkm_client *master = client->object.client;
	struct nvkm_memory *memory = NULL;
	struct nvkm_object *object;
	struct nvkm_umem *umem;

	object = nvkm_object_search(client, handle, &nvkm_umem);
	if (IS_ERR(object)) {
		if (client->super && client != master) {
			spin_lock(&master->lock);
			list_for_each_entry(umem, &master->umem, head) {
				if (umem->object.object == handle) {
					memory = nvkm_memory_ref(umem->memory);
					break;
				}
			}
			spin_unlock(&master->lock);
		}
	} else {
		umem = nvkm_umem(object);
		if (!umem->priv || client->super)
			memory = nvkm_memory_ref(umem->memory);
	}

	return memory ? memory : ERR_PTR(-ENOENT);
}

static int
nvkm_umem_unmap(struct nvkm_object *object)
{
	struct nvkm_umem *umem = nvkm_umem(object);

	if (!umem->map)
		return -EEXIST;

	if (umem->io) {
		if (!IS_ERR(umem->bar)) {
			struct nvkm_device *device = umem->mmu->subdev.device;
			nvkm_vmm_put(nvkm_bar_bar1_vmm(device), &umem->bar);
		} else {
			umem->bar = NULL;
		}
	} else {
#ifdef __NetBSD__
		bus_dmamem_unmap(umem->dmat, umem->map, umem->size);
#else
		vunmap(umem->map);
#endif
		umem->map = NULL;
	}

	return 0;
}

static int
#ifdef __NetBSD__
nvkm_umem_map(struct nvkm_object *object, void *argv, u32 argc,
	      enum nvkm_object_map *type,
	      bus_space_tag_t *tag, u64 *handle, u64 *length)
#else
nvkm_umem_map(struct nvkm_object *object, void *argv, u32 argc,
	      enum nvkm_object_map *type, u64 *handle, u64 *length)
#endif
{
	struct nvkm_umem *umem = nvkm_umem(object);
	struct nvkm_mmu *mmu = umem->mmu;

	if (!umem->mappable)
		return -EINVAL;
	if (umem->map)
		return -EEXIST;

	if ((umem->type & NVKM_MEM_HOST) && !argc) {
#ifdef __NetBSD__
		int ret = nvkm_mem_map_host(umem->memory, &umem->dmat,
		    &umem->map, &umem->size);
#else
		int ret = nvkm_mem_map_host(umem->memory, &umem->map);
#endif
		if (ret)
			return ret;

#ifdef __NetBSD__
		*tag = NULL;
#endif
		*handle = (unsigned long)(void *)umem->map;
		*length = nvkm_memory_size(umem->memory);
		*type = NVKM_OBJECT_MAP_VA;
		return 0;
	} else
	if ((umem->type & NVKM_MEM_VRAM) ||
	    (umem->type & NVKM_MEM_KIND)) {
#ifdef __NetBSD__
		int ret = mmu->func->mem.umap(mmu, umem->memory, argv, argc,
					      tag, handle, length, &umem->bar);
#else
		int ret = mmu->func->mem.umap(mmu, umem->memory, argv, argc,
					      handle, length, &umem->bar);
#endif
		if (ret)
			return ret;

		*type = NVKM_OBJECT_MAP_IO;
	} else {
		return -EINVAL;
	}

	umem->io = (*type == NVKM_OBJECT_MAP_IO);
	return 0;
}

static void *
nvkm_umem_dtor(struct nvkm_object *object)
{
	struct nvkm_umem *umem = nvkm_umem(object);
	spin_lock(&umem->object.client->lock);
	list_del_init(&umem->head);
	spin_unlock(&umem->object.client->lock);
	nvkm_memory_unref(&umem->memory);
	return umem;
}

static const struct nvkm_object_func
nvkm_umem = {
	.dtor = nvkm_umem_dtor,
	.map = nvkm_umem_map,
	.unmap = nvkm_umem_unmap,
};

int
nvkm_umem_new(const struct nvkm_oclass *oclass, void *argv, u32 argc,
	      struct nvkm_object **pobject)
{
	struct nvkm_mmu *mmu = nvkm_ummu(oclass->parent)->mmu;
	union {
		struct nvif_mem_v0 v0;
	} *args = argv;
	struct nvkm_umem *umem;
	int type, ret = -ENOSYS;
	u8  page;
	u64 size;

	if (!(ret = nvif_unpack(ret, &argv, &argc, args->v0, 0, 0, true))) {
		type = args->v0.type;
		page = args->v0.page;
		size = args->v0.size;
	} else
		return ret;

	if (type >= mmu->type_nr)
		return -EINVAL;

	if (!(umem = kzalloc(sizeof(*umem), GFP_KERNEL)))
		return -ENOMEM;
	nvkm_object_ctor(&nvkm_umem, oclass, &umem->object);
	umem->mmu = mmu;
	umem->type = mmu->type[type].type;
	umem->priv = oclass->client->super;
	INIT_LIST_HEAD(&umem->head);
	*pobject = &umem->object;

	if (mmu->type[type].type & NVKM_MEM_MAPPABLE) {
		page = max_t(u8, page, PAGE_SHIFT);
		umem->mappable = true;
	}

	ret = nvkm_mem_new_type(mmu, type, page, size, argv, argc,
				&umem->memory);
	if (ret)
		return ret;

	spin_lock(&umem->object.client->lock);
	list_add(&umem->head, &umem->object.client->umem);
	spin_unlock(&umem->object.client->lock);

	args->v0.page = nvkm_memory_page(umem->memory);
	args->v0.addr = nvkm_memory_addr(umem->memory);
	args->v0.size = nvkm_memory_size(umem->memory);
	return 0;
}
