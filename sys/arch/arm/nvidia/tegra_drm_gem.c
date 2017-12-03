/* $NetBSD: tegra_drm_gem.c,v 1.3.8.2 2017/12/03 11:35:54 jdolecek Exp $ */

/*-
 * Copyright (c) 2015 Jared D. McNeill <jmcneill@invisible.ca>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tegra_drm_gem.c,v 1.3.8.2 2017/12/03 11:35:54 jdolecek Exp $");

#include <drm/drmP.h>
#include <uvm/uvm.h>

#include <arm/nvidia/tegra_drm.h>

struct tegra_gem_object *
tegra_drm_obj_alloc(struct drm_device *ddev, size_t size)
{
	struct tegra_drm_softc * const sc = tegra_drm_private(ddev);
	struct tegra_gem_object *obj;
	int error, nsegs;

	obj = kmem_zalloc(sizeof(*obj), KM_SLEEP);
	obj->dmat = sc->sc_dmat;
	obj->dmasize = size;

        error = bus_dmamem_alloc(obj->dmat, obj->dmasize, PAGE_SIZE, 0,
	    obj->dmasegs, 1, &nsegs, BUS_DMA_WAITOK);
	if (error)
		goto failed;
	error = bus_dmamem_map(obj->dmat, obj->dmasegs, nsegs,
	    obj->dmasize, &obj->dmap, BUS_DMA_WAITOK | BUS_DMA_COHERENT);
	if (error)
		goto free;
	error = bus_dmamap_create(obj->dmat, obj->dmasize, 1,
	    obj->dmasize, 0, BUS_DMA_WAITOK, &obj->dmamap);
	if (error)
		goto unmap;
	error = bus_dmamap_load(obj->dmat, obj->dmamap, obj->dmap,
	    obj->dmasize, NULL, BUS_DMA_WAITOK);
	if (error)
		goto destroy;

	memset(obj->dmap, 0, obj->dmasize);

	drm_gem_private_object_init(ddev, &obj->base, size);

	return obj;

destroy:
	bus_dmamap_destroy(obj->dmat, obj->dmamap);
unmap:
	bus_dmamem_unmap(obj->dmat, obj->dmap, obj->dmasize);
free:
	bus_dmamem_free(obj->dmat, obj->dmasegs, nsegs);
failed:
	kmem_free(obj, sizeof(*obj));

	return NULL;
}

void
tegra_drm_obj_free(struct tegra_gem_object *obj)
{
	bus_dmamap_unload(obj->dmat, obj->dmamap);
	bus_dmamap_destroy(obj->dmat, obj->dmamap);
	bus_dmamem_unmap(obj->dmat, obj->dmap, obj->dmasize);
	bus_dmamem_free(obj->dmat, obj->dmasegs, 1);
	kmem_free(obj, sizeof(*obj));
}

void
tegra_drm_gem_free_object(struct drm_gem_object *gem_obj)
{
	struct tegra_gem_object *obj = to_tegra_gem_obj(gem_obj);

	drm_gem_free_mmap_offset(gem_obj);
	drm_gem_object_release(gem_obj);
	tegra_drm_obj_free(obj);
}

int
tegra_drm_gem_fault(struct uvm_faultinfo *ufi, vaddr_t vaddr,
    struct vm_page **pps, int npages, int centeridx, vm_prot_t access_type,
    int flags)
{
	struct vm_map_entry *entry = ufi->entry;
	struct uvm_object *uobj = entry->object.uvm_obj;
	struct drm_gem_object *gem_obj =
	    container_of(uobj, struct drm_gem_object, gemo_uvmobj);
	struct tegra_gem_object *obj = to_tegra_gem_obj(gem_obj);
	off_t curr_offset;
	vaddr_t curr_va;
	paddr_t paddr, mdpgno;
	u_int mmapflags;
	int lcv, retval;
	vm_prot_t mapprot;

	if (UVM_ET_ISCOPYONWRITE(entry))
		return -EIO;

	curr_offset = entry->offset + (vaddr - entry->start);
	curr_va = vaddr;

	retval = 0;
	for (lcv = 0; lcv < npages; lcv++, curr_offset += PAGE_SIZE,
	    curr_va += PAGE_SIZE) {
		if ((flags & PGO_ALLPAGES) == 0 && lcv != centeridx)
			continue;
		if (pps[lcv] == PGO_DONTCARE)
			continue;

		mdpgno = bus_dmamem_mmap(obj->dmat, obj->dmasegs, 1,
		    curr_offset, access_type, BUS_DMA_PREFETCHABLE);
		if (mdpgno == -1) {
			retval = -EIO;
			break;
		}
		paddr = pmap_phys_address(mdpgno);
		mmapflags = pmap_mmap_flags(mdpgno);
		mapprot = ufi->entry->protection;

		if (pmap_enter(ufi->orig_map->pmap, curr_va, paddr, mapprot,
		    PMAP_CANFAIL | mapprot | mmapflags) != 0) {
			pmap_update(ufi->orig_map->pmap);
			uvmfault_unlockall(ufi, ufi->entry->aref.ar_amap, uobj);
			uvm_wait("tegra_drm_gem_fault");
			return -ERESTART;
		}
	}

	pmap_update(ufi->orig_map->pmap);
	uvmfault_unlockall(ufi, ufi->entry->aref.ar_amap, uobj);

	return retval;
}
