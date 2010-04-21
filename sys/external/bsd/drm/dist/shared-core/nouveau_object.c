/*
 * Copyright (C) 2006 Ben Skeggs.
 *
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

/*
 * Authors:
 *   Ben Skeggs <darktama@iinet.net.au>
 */

#include "drmP.h"
#include "drm.h"
#include "nouveau_drv.h"
#include "nouveau_drm.h"

/* NVidia uses context objects to drive drawing operations.

   Context objects can be selected into 8 subchannels in the FIFO,
   and then used via DMA command buffers.

   A context object is referenced by a user defined handle (CARD32). The HW
   looks up graphics objects in a hash table in the instance RAM.

   An entry in the hash table consists of 2 CARD32. The first CARD32 contains
   the handle, the second one a bitfield, that contains the address of the
   object in instance RAM.

   The format of the second CARD32 seems to be:

   NV4 to NV30:

   15: 0  instance_addr >> 4
   17:16  engine (here uses 1 = graphics)
   28:24  channel id (here uses 0)
   31	  valid (use 1)

   NV40:

   15: 0  instance_addr >> 4   (maybe 19-0)
   21:20  engine (here uses 1 = graphics)
   I'm unsure about the other bits, but using 0 seems to work.

   The key into the hash table depends on the object handle and channel id and
   is given as:
*/
static uint32_t
nouveau_ramht_hash_handle(struct drm_device *dev, int channel, uint32_t handle)
{
	struct drm_nouveau_private *dev_priv=dev->dev_private;
	uint32_t hash = 0;
	int i;

	DRM_DEBUG("ch%d handle=0x%08x\n", channel, handle);

	for (i=32;i>0;i-=dev_priv->ramht_bits) {
		hash ^= (handle & ((1 << dev_priv->ramht_bits) - 1));
		handle >>= dev_priv->ramht_bits;
	}
	if (dev_priv->card_type < NV_50)
		hash ^= channel << (dev_priv->ramht_bits - 4);
	hash <<= 3;

	DRM_DEBUG("hash=0x%08x\n", hash);
	return hash;
}

static int
nouveau_ramht_entry_valid(struct drm_device *dev, struct nouveau_gpuobj *ramht,
			  uint32_t offset)
{
	struct drm_nouveau_private *dev_priv=dev->dev_private;
	uint32_t ctx = INSTANCE_RD(ramht, (offset + 4)/4);

	if (dev_priv->card_type < NV_40)
		return ((ctx & NV_RAMHT_CONTEXT_VALID) != 0);
	return (ctx != 0);
}

static int
nouveau_ramht_insert(struct drm_device *dev, struct nouveau_gpuobj_ref *ref)
{
	struct drm_nouveau_private *dev_priv=dev->dev_private;
	struct nouveau_channel *chan = dev_priv->fifos[ref->channel];
	struct nouveau_gpuobj *ramht = chan->ramht ? chan->ramht->gpuobj : NULL;
	struct nouveau_gpuobj *gpuobj = ref->gpuobj;
	uint32_t ctx, co, ho;

	if (!ramht) {
		DRM_ERROR("No hash table!\n");
		return -EINVAL;
	}

	if (dev_priv->card_type < NV_40) {
		ctx = NV_RAMHT_CONTEXT_VALID | (ref->instance >> 4) |
		      (ref->channel   << NV_RAMHT_CONTEXT_CHANNEL_SHIFT) |
		      (gpuobj->engine << NV_RAMHT_CONTEXT_ENGINE_SHIFT);
	} else
	if (dev_priv->card_type < NV_50) {
		ctx = (ref->instance >> 4) |
		      (ref->channel   << NV40_RAMHT_CONTEXT_CHANNEL_SHIFT) |
		      (gpuobj->engine << NV40_RAMHT_CONTEXT_ENGINE_SHIFT);
	} else {
		ctx = (ref->instance  >> 4) |
		      (gpuobj->engine << NV40_RAMHT_CONTEXT_ENGINE_SHIFT);
	}

	co = ho = nouveau_ramht_hash_handle(dev, ref->channel, ref->handle);
	do {
		if (!nouveau_ramht_entry_valid(dev, ramht, co)) {
			DRM_DEBUG("insert ch%d 0x%08x: h=0x%08x, c=0x%08x\n",
				  ref->channel, co, ref->handle, ctx);
			INSTANCE_WR(ramht, (co + 0)/4, ref->handle);
			INSTANCE_WR(ramht, (co + 4)/4, ctx);

			list_add_tail(&ref->list, &chan->ramht_refs);
			return 0;
		}
		DRM_DEBUG("collision ch%d 0x%08x: h=0x%08x\n",
			  ref->channel, co, INSTANCE_RD(ramht, co/4));

		co += 8;
		if (co >= dev_priv->ramht_size)
			co = 0;
	} while (co != ho);

	DRM_ERROR("RAMHT space exhausted. ch=%d\n", ref->channel);
	return -ENOMEM;
}

static void
nouveau_ramht_remove(struct drm_device *dev, struct nouveau_gpuobj_ref *ref)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct nouveau_channel *chan = dev_priv->fifos[ref->channel];
	struct nouveau_gpuobj *ramht = chan->ramht ? chan->ramht->gpuobj : NULL;
	uint32_t co, ho;

	if (!ramht) {
		DRM_ERROR("No hash table!\n");
		return;
	}

	co = ho = nouveau_ramht_hash_handle(dev, ref->channel, ref->handle);
	do {
		if (nouveau_ramht_entry_valid(dev, ramht, co) &&
		    (ref->handle == INSTANCE_RD(ramht, (co/4)))) {
			DRM_DEBUG("remove ch%d 0x%08x: h=0x%08x, c=0x%08x\n",
				  ref->channel, co, ref->handle,
				  INSTANCE_RD(ramht, (co + 4)));
			INSTANCE_WR(ramht, (co + 0)/4, 0x00000000);
			INSTANCE_WR(ramht, (co + 4)/4, 0x00000000);

			list_del(&ref->list);
			return;
		}

		co += 8;
		if (co >= dev_priv->ramht_size)
			co = 0;
	} while (co != ho);

	DRM_ERROR("RAMHT entry not found. ch=%d, handle=0x%08x\n",
		  ref->channel, ref->handle);
}

int
nouveau_gpuobj_new(struct drm_device *dev, struct nouveau_channel *chan,
		   int size, int align, uint32_t flags,
		   struct nouveau_gpuobj **gpuobj_ret)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct nouveau_engine *engine = &dev_priv->Engine;
	struct nouveau_gpuobj *gpuobj;
	struct mem_block *pramin = NULL;
	int ret;

	DRM_DEBUG("ch%d size=%d align=%d flags=0x%08x\n",
		  chan ? chan->id : -1, size, align, flags);

	if (!dev_priv || !gpuobj_ret || *gpuobj_ret != NULL)
		return -EINVAL;

	gpuobj = drm_calloc(1, sizeof(*gpuobj), DRM_MEM_DRIVER);
	if (!gpuobj)
		return -ENOMEM;
	DRM_DEBUG("gpuobj %p\n", gpuobj);
	gpuobj->flags = flags;
	gpuobj->im_channel = chan ? chan->id : -1;

	list_add_tail(&gpuobj->list, &dev_priv->gpuobj_list);

	/* Choose between global instmem heap, and per-channel private
	 * instmem heap.  On <NV50 allow requests for private instmem
	 * to be satisfied from global heap if no per-channel area
	 * available.
	 */
	if (chan) {
		if (chan->ramin_heap) {
			DRM_DEBUG("private heap\n");
			pramin = chan->ramin_heap;
		} else
		if (dev_priv->card_type < NV_50) {
			DRM_DEBUG("global heap fallback\n");
			pramin = dev_priv->ramin_heap;
		}
	} else {
		DRM_DEBUG("global heap\n");
		pramin = dev_priv->ramin_heap;
	}

	if (!pramin) {
		DRM_ERROR("No PRAMIN heap!\n");
		return -EINVAL;
	}

	if (!chan && (ret = engine->instmem.populate(dev, gpuobj, &size))) {
		nouveau_gpuobj_del(dev, &gpuobj);
		return ret;
	}

	/* Allocate a chunk of the PRAMIN aperture */
	gpuobj->im_pramin = nouveau_mem_alloc_block(pramin, size,
						    drm_order(align),
						    (struct drm_file *)-2, 0);
	if (!gpuobj->im_pramin) {
		nouveau_gpuobj_del(dev, &gpuobj);
		return -ENOMEM;
	}
	gpuobj->im_pramin->flags = NOUVEAU_MEM_INSTANCE;

	if (!chan && (ret = engine->instmem.bind(dev, gpuobj))) {
		nouveau_gpuobj_del(dev, &gpuobj);
		return ret;
	}

	if (gpuobj->flags & NVOBJ_FLAG_ZERO_ALLOC) {
		int i;

		for (i = 0; i < gpuobj->im_pramin->size; i += 4)
			INSTANCE_WR(gpuobj, i/4, 0);
	}

	*gpuobj_ret = gpuobj;
	return 0;
}

int
nouveau_gpuobj_early_init(struct drm_device *dev)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;

	DRM_DEBUG("\n");

	INIT_LIST_HEAD(&dev_priv->gpuobj_list);

	return 0;
}

int
nouveau_gpuobj_init(struct drm_device *dev)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	int ret;

	DRM_DEBUG("\n");

	if (dev_priv->card_type < NV_50) {
		if ((ret = nouveau_gpuobj_new_fake(dev, dev_priv->ramht_offset,
						   ~0, dev_priv->ramht_size,
						   NVOBJ_FLAG_ZERO_ALLOC |
						   NVOBJ_FLAG_ALLOW_NO_REFS,
						   &dev_priv->ramht, NULL)))
			return ret;
	}

	return 0;
}

void
nouveau_gpuobj_takedown(struct drm_device *dev)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;

	DRM_DEBUG("\n");

	nouveau_gpuobj_del(dev, &dev_priv->ramht);
}

void
nouveau_gpuobj_late_takedown(struct drm_device *dev)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct nouveau_gpuobj *gpuobj = NULL;
	struct list_head *entry, *tmp;

	DRM_DEBUG("\n");

	list_for_each_safe(entry, tmp, &dev_priv->gpuobj_list) {
		gpuobj = list_entry(entry, struct nouveau_gpuobj, list);

		DRM_ERROR("gpuobj %p still exists at takedown, refs=%d\n",
			  gpuobj, gpuobj->refcount);
		gpuobj->refcount = 0;
		nouveau_gpuobj_del(dev, &gpuobj);
	}
}

int
nouveau_gpuobj_del(struct drm_device *dev, struct nouveau_gpuobj **pgpuobj)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct nouveau_engine *engine = &dev_priv->Engine;
	struct nouveau_gpuobj *gpuobj;

	DRM_DEBUG("gpuobj %p\n", pgpuobj ? *pgpuobj : NULL);

	if (!dev_priv || !pgpuobj || !(*pgpuobj))
		return -EINVAL;
	gpuobj = *pgpuobj;

	if (gpuobj->refcount != 0) {
		DRM_ERROR("gpuobj refcount is %d\n", gpuobj->refcount);
		return -EINVAL;
	}

	if (gpuobj->dtor)
		gpuobj->dtor(dev, gpuobj);

	if (gpuobj->im_backing) {
		if (gpuobj->flags & NVOBJ_FLAG_FAKE)
			drm_free(gpuobj->im_backing,
				 sizeof(*gpuobj->im_backing), DRM_MEM_DRIVER);
		else
			engine->instmem.clear(dev, gpuobj);
	}

	if (gpuobj->im_pramin) {
		if (gpuobj->flags & NVOBJ_FLAG_FAKE)
			drm_free(gpuobj->im_pramin, sizeof(*gpuobj->im_pramin),
				 DRM_MEM_DRIVER);
		else
			nouveau_mem_free_block(gpuobj->im_pramin);
	}

	list_del(&gpuobj->list);

	*pgpuobj = NULL;
	drm_free(gpuobj, sizeof(*gpuobj), DRM_MEM_DRIVER);
	return 0;
}

static int
nouveau_gpuobj_instance_get(struct drm_device *dev,
			    struct nouveau_channel *chan,
			    struct nouveau_gpuobj *gpuobj, uint32_t *inst)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct nouveau_gpuobj *cpramin;

	/* <NV50 use PRAMIN address everywhere */
	if (dev_priv->card_type < NV_50) {
		*inst = gpuobj->im_pramin->start;
		return 0;
	}

	if (chan && gpuobj->im_channel != chan->id) {
		DRM_ERROR("Channel mismatch: obj %d, ref %d\n",
			  gpuobj->im_channel, chan->id);
		return -EINVAL;
	}

	/* NV50 channel-local instance */
	if (chan > 0) {
		cpramin = chan->ramin->gpuobj;
		*inst = gpuobj->im_pramin->start - cpramin->im_pramin->start;
		return 0;
	}

	/* NV50 global (VRAM) instance */
	if (gpuobj->im_channel < 0) {
		/* ...from global heap */
		if (!gpuobj->im_backing) {
			DRM_ERROR("AII, no VRAM backing gpuobj\n");
			return -EINVAL;
		}
		*inst = gpuobj->im_backing->start;
		return 0;
	} else {
		/* ...from local heap */
		cpramin = dev_priv->fifos[gpuobj->im_channel]->ramin->gpuobj;
		*inst = cpramin->im_backing->start +
			(gpuobj->im_pramin->start - cpramin->im_pramin->start);
		return 0;
	}

	return -EINVAL;
}

int
nouveau_gpuobj_ref_add(struct drm_device *dev, struct nouveau_channel *chan,
		       uint32_t handle, struct nouveau_gpuobj *gpuobj,
		       struct nouveau_gpuobj_ref **ref_ret)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct nouveau_gpuobj_ref *ref;
	uint32_t instance;
	int ret;

	DRM_DEBUG("ch%d h=0x%08x gpuobj=%p\n",
		  chan ? chan->id : -1, handle, gpuobj);

	if (!dev_priv || !gpuobj || (ref_ret && *ref_ret != NULL))
		return -EINVAL;

	if (!chan && !ref_ret)
		return -EINVAL;

	ret = nouveau_gpuobj_instance_get(dev, chan, gpuobj, &instance);
	if (ret)
		return ret;

	ref = drm_calloc(1, sizeof(*ref), DRM_MEM_DRIVER);
	if (!ref)
		return -ENOMEM;
	ref->gpuobj   = gpuobj;
	ref->channel  = chan ? chan->id : -1;
	ref->instance = instance;

	if (!ref_ret) {
		ref->handle = handle;

		ret = nouveau_ramht_insert(dev, ref);
		if (ret) {
			drm_free(ref, sizeof(*ref), DRM_MEM_DRIVER);
			return ret;
		}
	} else {
		ref->handle = ~0;
		*ref_ret = ref;
	}

	ref->gpuobj->refcount++;
	return 0;
}

int nouveau_gpuobj_ref_del(struct drm_device *dev, struct nouveau_gpuobj_ref **pref)
{
	struct nouveau_gpuobj_ref *ref;

	DRM_DEBUG("ref %p\n", pref ? *pref : NULL);

	if (!dev || !pref || *pref == NULL)
		return -EINVAL;
	ref = *pref;

	if (ref->handle != ~0)
		nouveau_ramht_remove(dev, ref);

	if (ref->gpuobj) {
		ref->gpuobj->refcount--;

		if (ref->gpuobj->refcount == 0) {
			if (!(ref->gpuobj->flags & NVOBJ_FLAG_ALLOW_NO_REFS))
				nouveau_gpuobj_del(dev, &ref->gpuobj);
		}
	}

	*pref = NULL;
	drm_free(ref, sizeof(ref), DRM_MEM_DRIVER);
	return 0;
}

int
nouveau_gpuobj_new_ref(struct drm_device *dev,
		       struct nouveau_channel *oc, struct nouveau_channel *rc,
		       uint32_t handle, int size, int align, uint32_t flags,
		       struct nouveau_gpuobj_ref **ref)
{
	struct nouveau_gpuobj *gpuobj = NULL;
	int ret;

	if ((ret = nouveau_gpuobj_new(dev, oc, size, align, flags, &gpuobj)))
		return ret;

	if ((ret = nouveau_gpuobj_ref_add(dev, rc, handle, gpuobj, ref))) {
		nouveau_gpuobj_del(dev, &gpuobj);
		return ret;
	}

	return 0;
}

int
nouveau_gpuobj_ref_find(struct nouveau_channel *chan, uint32_t handle,
			struct nouveau_gpuobj_ref **ref_ret)
{
	struct nouveau_gpuobj_ref *ref;
	struct list_head *entry, *tmp;

	list_for_each_safe(entry, tmp, &chan->ramht_refs) {
		ref = list_entry(entry, struct nouveau_gpuobj_ref, list);

		if (ref->handle == handle) {
			if (ref_ret)
				*ref_ret = ref;
			return 0;
		}
	}

	return -EINVAL;
}

int
nouveau_gpuobj_new_fake(struct drm_device *dev, uint32_t p_offset,
			uint32_t b_offset, uint32_t size,
			uint32_t flags, struct nouveau_gpuobj **pgpuobj,
			struct nouveau_gpuobj_ref **pref)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct nouveau_gpuobj *gpuobj = NULL;
	int i;

	DRM_DEBUG("p_offset=0x%08x b_offset=0x%08x size=0x%08x flags=0x%08x\n",
		  p_offset, b_offset, size, flags);

	gpuobj = drm_calloc(1, sizeof(*gpuobj), DRM_MEM_DRIVER);
	if (!gpuobj)
		return -ENOMEM;
	DRM_DEBUG("gpuobj %p\n", gpuobj);
	gpuobj->im_channel = -1;
	gpuobj->flags      = flags | NVOBJ_FLAG_FAKE;

	list_add_tail(&gpuobj->list, &dev_priv->gpuobj_list);

	if (p_offset != ~0) {
		gpuobj->im_pramin = drm_calloc(1, sizeof(struct mem_block),
					       DRM_MEM_DRIVER);
		if (!gpuobj->im_pramin) {
			nouveau_gpuobj_del(dev, &gpuobj);
			return -ENOMEM;
		}
		gpuobj->im_pramin->start = p_offset;
		gpuobj->im_pramin->size  = size;
	}

	if (b_offset != ~0) {
		gpuobj->im_backing = drm_calloc(1, sizeof(struct mem_block),
					       DRM_MEM_DRIVER);
		if (!gpuobj->im_backing) {
			nouveau_gpuobj_del(dev, &gpuobj);
			return -ENOMEM;
		}
		gpuobj->im_backing->start = b_offset;
		gpuobj->im_backing->size  = size;
	}

	if (gpuobj->flags & NVOBJ_FLAG_ZERO_ALLOC) {
		for (i = 0; i < gpuobj->im_pramin->size; i += 4)
			INSTANCE_WR(gpuobj, i/4, 0);
	}

	if (pref) {
		if ((i = nouveau_gpuobj_ref_add(dev, NULL, 0, gpuobj, pref))) {
			nouveau_gpuobj_del(dev, &gpuobj);
			return i;
		}
	}

	if (pgpuobj)
		*pgpuobj = gpuobj;
	return 0;
}


static int
nouveau_gpuobj_class_instmem_size(struct drm_device *dev, int class)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;

	/*XXX: dodgy hack for now */
	if (dev_priv->card_type >= NV_50)
		return 24;
	if (dev_priv->card_type >= NV_40)
		return 32;
	return 16;
}

/*
   DMA objects are used to reference a piece of memory in the
   framebuffer, PCI or AGP address space. Each object is 16 bytes big
   and looks as follows:

   entry[0]
   11:0  class (seems like I can always use 0 here)
   12    page table present?
   13    page entry linear?
   15:14 access: 0 rw, 1 ro, 2 wo
   17:16 target: 0 NV memory, 1 NV memory tiled, 2 PCI, 3 AGP
   31:20 dma adjust (bits 0-11 of the address)
   entry[1]
   dma limit (size of transfer)
   entry[X]
   1     0 readonly, 1 readwrite
   31:12 dma frame address of the page (bits 12-31 of the address)
   entry[N]
   page table terminator, same value as the first pte, as does nvidia
   rivatv uses 0xffffffff

   Non linear page tables need a list of frame addresses afterwards,
   the rivatv project has some info on this.

   The method below creates a DMA object in instance RAM and returns a handle
   to it that can be used to set up context objects.
*/
int
nouveau_gpuobj_dma_new(struct nouveau_channel *chan, int class,
		       uint64_t offset, uint64_t size, int access,
		       int target, struct nouveau_gpuobj **gpuobj)
{
	struct drm_device *dev = chan->dev;
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	int ret;
	uint32_t is_scatter_gather = 0;

	/* Total number of pages covered by the request.
	 */
	const unsigned int page_count = (size + PAGE_SIZE - 1) / PAGE_SIZE;


	DRM_DEBUG("ch%d class=0x%04x offset=0x%llx size=0x%llx\n",
		  chan->id, class, offset, size);
	DRM_DEBUG("access=%d target=%d\n", access, target);

	switch (target) {
        case NV_DMA_TARGET_AGP:
                 offset += dev_priv->gart_info.aper_base;
                 break;
        case NV_DMA_TARGET_PCI_NONLINEAR:
                /*assume the "offset" is a virtual memory address*/
                is_scatter_gather = 1;
                /*put back the right value*/
                target = NV_DMA_TARGET_PCI;
                break;
        default:
                break;
        }

	ret = nouveau_gpuobj_new(dev, chan,
				 is_scatter_gather ? ((page_count << 2) + 12) : nouveau_gpuobj_class_instmem_size(dev, class),
				 16,
				 NVOBJ_FLAG_ZERO_ALLOC | NVOBJ_FLAG_ZERO_FREE,
				 gpuobj);
	if (ret) {
		DRM_ERROR("Error creating gpuobj: %d\n", ret);
		return ret;
	}

	if (dev_priv->card_type < NV_50) {
		uint32_t frame, adjust, pte_flags = 0;
		adjust = offset &  0x00000fff;
		if (access != NV_DMA_ACCESS_RO)
				pte_flags |= (1<<1);

		if ( ! is_scatter_gather )
			{
			frame  = offset & ~0x00000fff;

			INSTANCE_WR(*gpuobj, 0, ((1<<12) | (1<<13) |
					(adjust << 20) |
					 (access << 14) |
					 (target << 16) |
					  class));
			INSTANCE_WR(*gpuobj, 1, size - 1);
			INSTANCE_WR(*gpuobj, 2, frame | pte_flags);
			INSTANCE_WR(*gpuobj, 3, frame | pte_flags);
			}
		else
			{
			/* Intial page entry in the scatter-gather area that
			 * corresponds to the base offset
			 */
			unsigned int idx = offset / PAGE_SIZE;

			uint32_t instance_offset;
			unsigned int i;

			if ((idx + page_count) > dev->sg->pages) {
				DRM_ERROR("Requested page range exceedes "
					  "allocated scatter-gather range!");
				return -E2BIG;
			}

			DRM_DEBUG("Creating PCI DMA object using virtual zone starting at %#llx, size %d\n", offset, (uint32_t)size);
	                INSTANCE_WR(*gpuobj, 0, ((1<<12) | (0<<13) |
                                (adjust << 20) |
                                (access << 14) |
                                (target << 16) |
                                class));
			INSTANCE_WR(*gpuobj, 1, (uint32_t) size-1);


			/*write starting at the third dword*/
			instance_offset = 2;

			/*for each PAGE, get its bus address, fill in the page table entry, and advance*/
			for (i = 0; i < page_count; i++) {
				if (dev->sg->busaddr[idx] == 0) {
					dev->sg->busaddr[idx] =
						pci_map_page(dev->pdev,
							     dev->sg->pagelist[idx],
							     0,
							     PAGE_SIZE,
							     DMA_BIDIRECTIONAL);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27))
					/* Not a 100% sure this is the right kdev in all cases. */
					if (dma_mapping_error(&dev->primary->kdev, dev->sg->busaddr[idx])) {
#else
					if (dma_mapping_error(dev->sg->busaddr[idx])) {
#endif
						return -ENOMEM;
					}
				}

				frame = (uint32_t) dev->sg->busaddr[idx];
				INSTANCE_WR(*gpuobj, instance_offset,
					    frame | pte_flags);

				idx++;
				instance_offset ++;
			}
			}
	} else {
		uint32_t flags0, flags5;

		if (target == NV_DMA_TARGET_VIDMEM) {
			flags0 = 0x00190000;
			flags5 = 0x00010000;
		} else {
			flags0 = 0x7fc00000;
			flags5 = 0x00080000;
		}

		INSTANCE_WR(*gpuobj, 0, flags0 | class);
		INSTANCE_WR(*gpuobj, 1, offset + size - 1);
		INSTANCE_WR(*gpuobj, 2, offset);
		INSTANCE_WR(*gpuobj, 5, flags5);
	}

	(*gpuobj)->engine = NVOBJ_ENGINE_SW;
	(*gpuobj)->class  = class;
	return 0;
}

int
nouveau_gpuobj_gart_dma_new(struct nouveau_channel *chan,
			    uint64_t offset, uint64_t size, int access,
			    struct nouveau_gpuobj **gpuobj,
			    uint32_t *o_ret)
{
	struct drm_device *dev = chan->dev;
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	int ret;

	if (dev_priv->gart_info.type == NOUVEAU_GART_AGP ||
	    (dev_priv->card_type >= NV_50 &&
	     dev_priv->gart_info.type == NOUVEAU_GART_SGDMA)) {
		ret = nouveau_gpuobj_dma_new(chan, NV_CLASS_DMA_IN_MEMORY,
					     offset, size, access,
					     NV_DMA_TARGET_AGP, gpuobj);
		if (o_ret)
			*o_ret = 0;
	} else
	if (dev_priv->gart_info.type == NOUVEAU_GART_SGDMA) {
		*gpuobj = dev_priv->gart_info.sg_ctxdma;
		if (offset & ~0xffffffffULL) {
			DRM_ERROR("obj offset exceeds 32-bits\n");
			return -EINVAL;
		}
		if (o_ret)
			*o_ret = (uint32_t)offset;
		ret = (*gpuobj != NULL) ? 0 : -EINVAL;
	} else {
		DRM_ERROR("Invalid GART type %d\n", dev_priv->gart_info.type);
		return -EINVAL;
	}

	return ret;
}

/* Context objects in the instance RAM have the following structure.
 * On NV40 they are 32 byte long, on NV30 and smaller 16 bytes.

   NV4 - NV30:

   entry[0]
   11:0 class
   12   chroma key enable
   13   user clip enable
   14   swizzle enable
   17:15 patch config:
       scrcopy_and, rop_and, blend_and, scrcopy, srccopy_pre, blend_pre
   18   synchronize enable
   19   endian: 1 big, 0 little
   21:20 dither mode
   23    single step enable
   24    patch status: 0 invalid, 1 valid
   25    context_surface 0: 1 valid
   26    context surface 1: 1 valid
   27    context pattern: 1 valid
   28    context rop: 1 valid
   29,30 context beta, beta4
   entry[1]
   7:0   mono format
   15:8  color format
   31:16 notify instance address
   entry[2]
   15:0  dma 0 instance address
   31:16 dma 1 instance address
   entry[3]
   dma method traps

   NV40:
   No idea what the exact format is. Here's what can be deducted:

   entry[0]:
   11:0  class  (maybe uses more bits here?)
   17    user clip enable
   21:19 patch config
   25    patch status valid ?
   entry[1]:
   15:0  DMA notifier  (maybe 20:0)
   entry[2]:
   15:0  DMA 0 instance (maybe 20:0)
   24    big endian
   entry[3]:
   15:0  DMA 1 instance (maybe 20:0)
   entry[4]:
   entry[5]:
   set to 0?
*/
int
nouveau_gpuobj_gr_new(struct nouveau_channel *chan, int class,
		      struct nouveau_gpuobj **gpuobj)
{
	struct drm_device *dev = chan->dev;
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	int ret;

	DRM_DEBUG("ch%d class=0x%04x\n", chan->id, class);

	ret = nouveau_gpuobj_new(dev, chan,
				 nouveau_gpuobj_class_instmem_size(dev, class),
				 16,
				 NVOBJ_FLAG_ZERO_ALLOC | NVOBJ_FLAG_ZERO_FREE,
				 gpuobj);
	if (ret) {
		DRM_ERROR("Error creating gpuobj: %d\n", ret);
		return ret;
	}

	if (dev_priv->card_type >= NV_50) {
		INSTANCE_WR(*gpuobj, 0, class);
		INSTANCE_WR(*gpuobj, 5, 0x00010000);
	} else {
	switch (class) {
	case NV_CLASS_NULL:
		INSTANCE_WR(*gpuobj, 0, 0x00001030);
		INSTANCE_WR(*gpuobj, 1, 0xFFFFFFFF);
		break;
	default:
		if (dev_priv->card_type >= NV_40) {
			INSTANCE_WR(*gpuobj, 0, class);
#ifdef __BIG_ENDIAN
			INSTANCE_WR(*gpuobj, 2, 0x01000000);
#endif
		} else {
#ifdef __BIG_ENDIAN
			INSTANCE_WR(*gpuobj, 0, class | 0x00080000);
#else
			INSTANCE_WR(*gpuobj, 0, class);
#endif
		}
	}
	}

	(*gpuobj)->engine = NVOBJ_ENGINE_GR;
	(*gpuobj)->class  = class;
	return 0;
}

static int
nouveau_gpuobj_channel_init_pramin(struct nouveau_channel *chan)
{
	struct drm_device *dev = chan->dev;
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct nouveau_gpuobj *pramin = NULL;
	int size, base, ret;

	DRM_DEBUG("ch%d\n", chan->id);

	/* Base amount for object storage (4KiB enough?) */
	size = 0x1000;
	base = 0;

	/* PGRAPH context */

	if (dev_priv->card_type == NV_50) {
		/* Various fixed table thingos */
		size += 0x1400; /* mostly unknown stuff */
		size += 0x4000; /* vm pd */
		base  = 0x6000;
		/* RAMHT, not sure about setting size yet, 32KiB to be safe */
		size += 0x8000;
		/* RAMFC */
		size += 0x1000;
		/* PGRAPH context */
		size += 0x70000;
	}

	DRM_DEBUG("ch%d PRAMIN size: 0x%08x bytes, base alloc=0x%08x\n",
		  chan->id, size, base);
	ret = nouveau_gpuobj_new_ref(dev, NULL, NULL, 0, size, 0x1000, 0,
				     &chan->ramin);
	if (ret) {
		DRM_ERROR("Error allocating channel PRAMIN: %d\n", ret);
		return ret;
	}
	pramin = chan->ramin->gpuobj;

	ret = nouveau_mem_init_heap(&chan->ramin_heap,
				    pramin->im_pramin->start + base, size);
	if (ret) {
		DRM_ERROR("Error creating PRAMIN heap: %d\n", ret);
		nouveau_gpuobj_ref_del(dev, &chan->ramin);
		return ret;
	}

	return 0;
}

int
nouveau_gpuobj_channel_init(struct nouveau_channel *chan,
			    uint32_t vram_h, uint32_t tt_h)
{
	struct drm_device *dev = chan->dev;
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct nouveau_gpuobj *vram = NULL, *tt = NULL;
	int ret, i;

	INIT_LIST_HEAD(&chan->ramht_refs);

	DRM_DEBUG("ch%d vram=0x%08x tt=0x%08x\n", chan->id, vram_h, tt_h);

	/* Reserve a block of PRAMIN for the channel
	 *XXX: maybe on <NV50 too at some point
	 */
	if (0 || dev_priv->card_type == NV_50) {
		ret = nouveau_gpuobj_channel_init_pramin(chan);
		if (ret)
			return ret;
	}

	/* NV50 VM
	 *  - Allocate per-channel page-directory
	 *  - Point offset 0-512MiB at shared PCIEGART table
	 *  - Point offset 512-1024MiB at shared VRAM table
	 */
	if (dev_priv->card_type >= NV_50) {
		uint32_t vm_offset;

		vm_offset = (dev_priv->chipset & 0xf0) == 0x50 ? 0x1400 : 0x200;
		vm_offset += chan->ramin->gpuobj->im_pramin->start;
		if ((ret = nouveau_gpuobj_new_fake(dev, vm_offset, ~0, 0x4000,
						   0, &chan->vm_pd, NULL)))
			return ret;
		for (i=0; i<0x4000; i+=8) {
			INSTANCE_WR(chan->vm_pd, (i+0)/4, 0x00000000);
			INSTANCE_WR(chan->vm_pd, (i+4)/4, 0xdeadcafe);
		}

		if ((ret = nouveau_gpuobj_ref_add(dev, NULL, 0,
						  dev_priv->gart_info.sg_ctxdma,
						  &chan->vm_gart_pt)))
			return ret;
		INSTANCE_WR(chan->vm_pd, (0+0)/4,
			    chan->vm_gart_pt->instance | 0x03);
		INSTANCE_WR(chan->vm_pd, (0+4)/4, 0x00000000);

		if ((ret = nouveau_gpuobj_ref_add(dev, NULL, 0,
						  dev_priv->vm_vram_pt,
						  &chan->vm_vram_pt)))
			return ret;
		INSTANCE_WR(chan->vm_pd, (8+0)/4,
			    chan->vm_vram_pt->instance | 0x61);
		INSTANCE_WR(chan->vm_pd, (8+4)/4, 0x00000000);
	}

	/* RAMHT */
	if (dev_priv->card_type < NV_50) {
		ret = nouveau_gpuobj_ref_add(dev, NULL, 0, dev_priv->ramht,
					     &chan->ramht);
		if (ret)
			return ret;
	} else {
		ret = nouveau_gpuobj_new_ref(dev, chan, chan, 0,
					     0x8000, 16,
					     NVOBJ_FLAG_ZERO_ALLOC,
					     &chan->ramht);
		if (ret)
			return ret;
	}

	/* VRAM ctxdma */
	if (dev_priv->card_type >= NV_50) {
		ret = nouveau_gpuobj_dma_new(chan, NV_CLASS_DMA_IN_MEMORY,
					     0, 0x100000000ULL,
					     NV_DMA_ACCESS_RW,
					     NV_DMA_TARGET_AGP, &vram);
		if (ret) {
			DRM_ERROR("Error creating VRAM ctxdma: %d\n", ret);
			return ret;
		}
	} else
	if ((ret = nouveau_gpuobj_dma_new(chan, NV_CLASS_DMA_IN_MEMORY,
					  0, dev_priv->fb_available_size,
					  NV_DMA_ACCESS_RW,
					  NV_DMA_TARGET_VIDMEM, &vram))) {
		DRM_ERROR("Error creating VRAM ctxdma: %d\n", ret);
		return ret;
	}

	if ((ret = nouveau_gpuobj_ref_add(dev, chan, vram_h, vram, NULL))) {
		DRM_ERROR("Error referencing VRAM ctxdma: %d\n", ret);
		return ret;
	}

	/* TT memory ctxdma */
	if (dev_priv->card_type >= NV_50) {
		tt = vram;
	} else
	if (dev_priv->gart_info.type != NOUVEAU_GART_NONE) {
		ret = nouveau_gpuobj_gart_dma_new(chan, 0,
						  dev_priv->gart_info.aper_size,
						  NV_DMA_ACCESS_RW, &tt, NULL);
	} else
	if (dev_priv->pci_heap) {
		ret = nouveau_gpuobj_dma_new(chan, NV_CLASS_DMA_IN_MEMORY,
					     0, dev->sg->pages * PAGE_SIZE,
					     NV_DMA_ACCESS_RW,
					     NV_DMA_TARGET_PCI_NONLINEAR, &tt);
	} else {
		DRM_ERROR("Invalid GART type %d\n", dev_priv->gart_info.type);
		ret = -EINVAL;
	}

	if (ret) {
		DRM_ERROR("Error creating TT ctxdma: %d\n", ret);
		return ret;
	}

	ret = nouveau_gpuobj_ref_add(dev, chan, tt_h, tt, NULL);
	if (ret) {
		DRM_ERROR("Error referencing TT ctxdma: %d\n", ret);
		return ret;
	}

	return 0;
}

void
nouveau_gpuobj_channel_takedown(struct nouveau_channel *chan)
{
	struct drm_device *dev = chan->dev;
	struct list_head *entry, *tmp;
	struct nouveau_gpuobj_ref *ref;

	DRM_DEBUG("ch%d\n", chan->id);

	list_for_each_safe(entry, tmp, &chan->ramht_refs) {
		ref = list_entry(entry, struct nouveau_gpuobj_ref, list);

		nouveau_gpuobj_ref_del(dev, &ref);
	}

	nouveau_gpuobj_ref_del(dev, &chan->ramht);

	nouveau_gpuobj_del(dev, &chan->vm_pd);
	nouveau_gpuobj_ref_del(dev, &chan->vm_gart_pt);
	nouveau_gpuobj_ref_del(dev, &chan->vm_vram_pt);

	if (chan->ramin_heap)
		nouveau_mem_takedown(&chan->ramin_heap);
	if (chan->ramin)
		nouveau_gpuobj_ref_del(dev, &chan->ramin);

}

int nouveau_ioctl_grobj_alloc(struct drm_device *dev, void *data,
			      struct drm_file *file_priv)
{
	struct nouveau_channel *chan;
	struct drm_nouveau_grobj_alloc *init = data;
	struct nouveau_gpuobj *gr = NULL;
	int ret;

	NOUVEAU_CHECK_INITIALISED_WITH_RETURN;
	NOUVEAU_GET_USER_CHANNEL_WITH_RETURN(init->channel, file_priv, chan);

	//FIXME: check args, only allow trusted objects to be created

	if (init->handle == ~0)
		return -EINVAL;

	if (nouveau_gpuobj_ref_find(chan, init->handle, NULL) == 0)
		return -EEXIST;

	ret = nouveau_gpuobj_gr_new(chan, init->class, &gr);
	if (ret) {
		DRM_ERROR("Error creating gr object: %d (%d/0x%08x)\n",
			  ret, init->channel, init->handle);
		return ret;
	}

	if ((ret = nouveau_gpuobj_ref_add(dev, chan, init->handle, gr, NULL))) {
		DRM_ERROR("Error referencing gr object: %d (%d/0x%08x\n)",
			  ret, init->channel, init->handle);
		nouveau_gpuobj_del(dev, &gr);
		return ret;
	}

	return 0;
}

int nouveau_ioctl_gpuobj_free(struct drm_device *dev, void *data,
			      struct drm_file *file_priv)
{
	struct drm_nouveau_gpuobj_free *objfree = data;
	struct nouveau_gpuobj_ref *ref;
	struct nouveau_channel *chan;
	int ret;

	NOUVEAU_CHECK_INITIALISED_WITH_RETURN;
	NOUVEAU_GET_USER_CHANNEL_WITH_RETURN(objfree->channel, file_priv, chan);

	if ((ret = nouveau_gpuobj_ref_find(chan, objfree->handle, &ref)))
		return ret;
	nouveau_gpuobj_ref_del(dev, &ref);

	return 0;
}
