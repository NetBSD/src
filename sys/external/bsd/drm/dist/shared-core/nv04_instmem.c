#include "drmP.h"
#include "drm.h"
#include "nouveau_drv.h"

static void
nv04_instmem_determine_amount(struct drm_device *dev)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	int i;

	/* Figure out how much instance memory we need */
	if (dev_priv->card_type >= NV_40) {
		/* We'll want more instance memory than this on some NV4x cards.
		 * There's a 16MB aperture to play with that maps onto the end
		 * of vram.  For now, only reserve a small piece until we know
		 * more about what each chipset requires.
		 */
		dev_priv->ramin_rsvd_vram = (1*1024* 1024);
	} else {
		/*XXX: what *are* the limits on <NV40 cards?
		 */
		dev_priv->ramin_rsvd_vram = (512*1024);
	}
	DRM_DEBUG("RAMIN size: %dKiB\n", dev_priv->ramin_rsvd_vram>>10);

	/* Clear all of it, except the BIOS image that's in the first 64KiB */
	for (i=(64*1024); i<dev_priv->ramin_rsvd_vram; i+=4)
		NV_WI32(i, 0x00000000);
}

static void
nv04_instmem_configure_fixed_tables(struct drm_device *dev)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct nouveau_engine *engine = &dev_priv->Engine;

	/* FIFO hash table (RAMHT)
	 *   use 4k hash table at RAMIN+0x10000
	 *   TODO: extend the hash table
	 */
	dev_priv->ramht_offset = 0x10000;
	dev_priv->ramht_bits   = 9;
	dev_priv->ramht_size   = (1 << dev_priv->ramht_bits); /* nr entries */
	dev_priv->ramht_size  *= 8; /* 2 32-bit values per entry in RAMHT */
	DRM_DEBUG("RAMHT offset=0x%x, size=%d\n", dev_priv->ramht_offset,
						  dev_priv->ramht_size);

	/* FIFO runout table (RAMRO) - 512k at 0x11200 */
	dev_priv->ramro_offset = 0x11200;
	dev_priv->ramro_size   = 512;
	DRM_DEBUG("RAMRO offset=0x%x, size=%d\n", dev_priv->ramro_offset,
						  dev_priv->ramro_size);

	/* FIFO context table (RAMFC)
	 *   NV40  : Not sure exactly how to position RAMFC on some cards,
	 *           0x30002 seems to position it at RAMIN+0x20000 on these
	 *           cards.  RAMFC is 4kb (32 fifos, 128byte entries).
	 *   Others: Position RAMFC at RAMIN+0x11400
	 */
	switch(dev_priv->card_type)
	{
		case NV_40:
		case NV_44:
			dev_priv->ramfc_offset = 0x20000;
			dev_priv->ramfc_size   = engine->fifo.channels *
						 nouveau_fifo_ctx_size(dev);
			break;
		case NV_30:
		case NV_20:
		case NV_17:
		case NV_11:
		case NV_10:
		case NV_04:
		default:
			dev_priv->ramfc_offset = 0x11400;
			dev_priv->ramfc_size   = engine->fifo.channels *
						 nouveau_fifo_ctx_size(dev);
			break;
	}
	DRM_DEBUG("RAMFC offset=0x%x, size=%d\n", dev_priv->ramfc_offset,
						  dev_priv->ramfc_size);
}

int nv04_instmem_init(struct drm_device *dev)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	uint32_t offset;
	int ret = 0;

	nv04_instmem_determine_amount(dev);
	nv04_instmem_configure_fixed_tables(dev);

	/* Create a heap to manage RAMIN allocations, we don't allocate
	 * the space that was reserved for RAMHT/FC/RO.
	 */
	offset = dev_priv->ramfc_offset + dev_priv->ramfc_size;

	/* On my NV4E, there's *something* clobbering the 16KiB just after
	 * where we setup these fixed tables.  No idea what it is just yet,
	 * so reserve this space on all NV4X cards for now.
	 */
	if (dev_priv->card_type >= NV_40)
		offset += 16*1024;

	ret = nouveau_mem_init_heap(&dev_priv->ramin_heap,
				    offset, dev_priv->ramin_rsvd_vram - offset);
	if (ret) {
		dev_priv->ramin_heap = NULL;
		DRM_ERROR("Failed to init RAMIN heap\n");
	}

	return ret;
}

void
nv04_instmem_takedown(struct drm_device *dev)
{
}

int
nv04_instmem_populate(struct drm_device *dev, struct nouveau_gpuobj *gpuobj, uint32_t *sz)
{
	if (gpuobj->im_backing)
		return -EINVAL;

	return 0;
}

void
nv04_instmem_clear(struct drm_device *dev, struct nouveau_gpuobj *gpuobj)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;

	if (gpuobj && gpuobj->im_backing) {
		if (gpuobj->im_bound)
			dev_priv->Engine.instmem.unbind(dev, gpuobj);
		gpuobj->im_backing = NULL;
	}
}

int
nv04_instmem_bind(struct drm_device *dev, struct nouveau_gpuobj *gpuobj)
{
	if (!gpuobj->im_pramin || gpuobj->im_bound)
		return -EINVAL;

	gpuobj->im_bound = 1;
	return 0;
}

int
nv04_instmem_unbind(struct drm_device *dev, struct nouveau_gpuobj *gpuobj)
{
	if (gpuobj->im_bound == 0)
		return -EINVAL;

	gpuobj->im_bound = 0;
	return 0;
}
