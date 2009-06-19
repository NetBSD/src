/*
 * Copyright (C) 2007 Ben Skeggs.
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

#include "drmP.h"
#include "drm.h"
#include "nouveau_drv.h"

typedef struct {
	uint32_t save1700[5]; /* 0x1700->0x1710 */

	struct nouveau_gpuobj_ref *pramin_pt;
	struct nouveau_gpuobj_ref *pramin_bar;
} nv50_instmem_priv;

#define NV50_INSTMEM_PAGE_SHIFT 12
#define NV50_INSTMEM_PAGE_SIZE  (1 << NV50_INSTMEM_PAGE_SHIFT)
#define NV50_INSTMEM_PT_SIZE(a)	(((a) >> 12) << 3)

/*NOTE: - Assumes 0x1700 already covers the correct MiB of PRAMIN
 */
#define BAR0_WI32(g,o,v) do {                                     \
	uint32_t offset;                                          \
	if ((g)->im_backing) {                                    \
		offset = (g)->im_backing->start;                  \
	} else {                                                  \
		offset  = chan->ramin->gpuobj->im_backing->start; \
		offset += (g)->im_pramin->start;                  \
	}                                                         \
	offset += (o);                                            \
	NV_WRITE(NV_RAMIN + (offset & 0xfffff), (v));             \
} while(0)

int
nv50_instmem_init(struct drm_device *dev)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct nouveau_channel *chan;
	uint32_t c_offset, c_size, c_ramfc, c_vmpd, c_base, pt_size;
	nv50_instmem_priv *priv;
	int ret, i;
	uint32_t v;

	priv = drm_calloc(1, sizeof(*priv), DRM_MEM_DRIVER);
	if (!priv)
		return -ENOMEM;
	dev_priv->Engine.instmem.priv = priv;

	/* Save state, will restore at takedown. */
	for (i = 0x1700; i <= 0x1710; i+=4)
		priv->save1700[(i-0x1700)/4] = NV_READ(i);

	/* Reserve the last MiB of VRAM, we should probably try to avoid
	 * setting up the below tables over the top of the VBIOS image at
	 * some point.
	 */
	dev_priv->ramin_rsvd_vram = 1 << 20;
	c_offset = nouveau_mem_fb_amount(dev) - dev_priv->ramin_rsvd_vram;
	c_size   = 128 << 10;
	c_vmpd   = ((dev_priv->chipset & 0xf0) == 0x50) ? 0x1400 : 0x200;
	c_ramfc  = ((dev_priv->chipset & 0xf0) == 0x50) ? 0x0 : 0x20;
	c_base   = c_vmpd + 0x4000;
	pt_size  = NV50_INSTMEM_PT_SIZE(dev_priv->ramin->size);

	DRM_DEBUG(" Rsvd VRAM base: 0x%08x\n", c_offset);
	DRM_DEBUG("    VBIOS image: 0x%08x\n", (NV_READ(0x619f04)&~0xff)<<8);
	DRM_DEBUG("  Aperture size: %d MiB\n",
		  (uint32_t)dev_priv->ramin->size >> 20);
	DRM_DEBUG("        PT size: %d KiB\n", pt_size >> 10);

	NV_WRITE(NV50_PUNK_BAR0_PRAMIN, (c_offset >> 16));

	/* Create a fake channel, and use it as our "dummy" channels 0/127.
	 * The main reason for creating a channel is so we can use the gpuobj
	 * code.  However, it's probably worth noting that NVIDIA also setup
	 * their channels 0/127 with the same values they configure here.
	 * So, there may be some other reason for doing this.
	 *
	 * Have to create the entire channel manually, as the real channel
	 * creation code assumes we have PRAMIN access, and we don't until
	 * we're done here.
	 */
	chan = drm_calloc(1, sizeof(*chan), DRM_MEM_DRIVER);
	if (!chan)
		return -ENOMEM;
	chan->id = 0;
	chan->dev = dev;
	chan->file_priv = (struct drm_file *)-2;
	dev_priv->fifos[0] = dev_priv->fifos[127] = chan;

	/* Channel's PRAMIN object + heap */
	if ((ret = nouveau_gpuobj_new_fake(dev, 0, c_offset, 128<<10, 0,
					   NULL, &chan->ramin)))
		return ret;

	if (nouveau_mem_init_heap(&chan->ramin_heap, c_base, c_size - c_base))
		return -ENOMEM;

	/* RAMFC + zero channel's PRAMIN up to start of VM pagedir */
	if ((ret = nouveau_gpuobj_new_fake(dev, c_ramfc, c_offset + c_ramfc,
					   0x4000, 0, NULL, &chan->ramfc)))
		return ret;

	for (i = 0; i < c_vmpd; i += 4)
		BAR0_WI32(chan->ramin->gpuobj, i, 0);

	/* VM page directory */
	if ((ret = nouveau_gpuobj_new_fake(dev, c_vmpd, c_offset + c_vmpd,
					   0x4000, 0, &chan->vm_pd, NULL)))
		return ret;
	for (i = 0; i < 0x4000; i += 8) {
		BAR0_WI32(chan->vm_pd, i + 0x00, 0x00000000);
		BAR0_WI32(chan->vm_pd, i + 0x04, 0x00000000);
	}

	/* PRAMIN page table, cheat and map into VM at 0x0000000000.
	 * We map the entire fake channel into the start of the PRAMIN BAR
	 */
	if ((ret = nouveau_gpuobj_new_ref(dev, chan, NULL, 0, pt_size, 0x1000,
					  0, &priv->pramin_pt)))
		return ret;

	for (i = 0, v = c_offset; i < pt_size; i+=8, v+=0x1000) {
		if (v < (c_offset + c_size))
			BAR0_WI32(priv->pramin_pt->gpuobj, i + 0, v | 1);
		else
			BAR0_WI32(priv->pramin_pt->gpuobj, i + 0, 0x00000009);
		BAR0_WI32(priv->pramin_pt->gpuobj, i + 4, 0x00000000);
	}

	BAR0_WI32(chan->vm_pd, 0x00, priv->pramin_pt->instance | 0x63);
	BAR0_WI32(chan->vm_pd, 0x04, 0x00000000);

	/* DMA object for PRAMIN BAR */
	if ((ret = nouveau_gpuobj_new_ref(dev, chan, chan, 0, 6*4, 16, 0,
					  &priv->pramin_bar)))
		return ret;
	BAR0_WI32(priv->pramin_bar->gpuobj, 0x00, 0x7fc00000);
	BAR0_WI32(priv->pramin_bar->gpuobj, 0x04, dev_priv->ramin->size - 1);
	BAR0_WI32(priv->pramin_bar->gpuobj, 0x08, 0x00000000);
	BAR0_WI32(priv->pramin_bar->gpuobj, 0x0c, 0x00000000);
	BAR0_WI32(priv->pramin_bar->gpuobj, 0x10, 0x00000000);
	BAR0_WI32(priv->pramin_bar->gpuobj, 0x14, 0x00000000);

	/* Poke the relevant regs, and pray it works :) */
	NV_WRITE(NV50_PUNK_BAR_CFG_BASE, (chan->ramin->instance >> 12));
	NV_WRITE(NV50_PUNK_UNK1710, 0);
	NV_WRITE(NV50_PUNK_BAR_CFG_BASE, (chan->ramin->instance >> 12) |
					 NV50_PUNK_BAR_CFG_BASE_VALID);
	NV_WRITE(NV50_PUNK_BAR1_CTXDMA, 0);
	NV_WRITE(NV50_PUNK_BAR3_CTXDMA, (priv->pramin_bar->instance >> 4) |
					NV50_PUNK_BAR3_CTXDMA_VALID);

	/* Assume that praying isn't enough, check that we can re-read the
	 * entire fake channel back from the PRAMIN BAR */
	for (i = 0; i < c_size; i+=4) {
		if (NV_READ(NV_RAMIN + i) != NV_RI32(i)) {
			DRM_ERROR("Error reading back PRAMIN at 0x%08x\n", i);
			return -EINVAL;
		}
	}

	/* Global PRAMIN heap */
	if (nouveau_mem_init_heap(&dev_priv->ramin_heap,
				  c_size, dev_priv->ramin->size - c_size)) {
		dev_priv->ramin_heap = NULL;
		DRM_ERROR("Failed to init RAMIN heap\n");
	}

	/*XXX: incorrect, but needed to make hash func "work" */
	dev_priv->ramht_offset = 0x10000;
	dev_priv->ramht_bits   = 9;
	dev_priv->ramht_size   = (1 << dev_priv->ramht_bits);
	return 0;
}

void
nv50_instmem_takedown(struct drm_device *dev)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	nv50_instmem_priv *priv = dev_priv->Engine.instmem.priv;
	struct nouveau_channel *chan = dev_priv->fifos[0];
	int i;

	DRM_DEBUG("\n");

	if (!priv)
		return;

	/* Restore state from before init */
	for (i = 0x1700; i <= 0x1710; i+=4)
		NV_WRITE(i, priv->save1700[(i-0x1700)/4]);

	nouveau_gpuobj_ref_del(dev, &priv->pramin_bar);
	nouveau_gpuobj_ref_del(dev, &priv->pramin_pt);

	/* Destroy dummy channel */
	if (chan) {
		nouveau_gpuobj_del(dev, &chan->vm_pd);
		nouveau_gpuobj_ref_del(dev, &chan->ramfc);
		nouveau_gpuobj_ref_del(dev, &chan->ramin);
		nouveau_mem_takedown(&chan->ramin_heap);

		dev_priv->fifos[0] = dev_priv->fifos[127] = NULL;
		drm_free(chan, sizeof(*chan), DRM_MEM_DRIVER);
	}

	dev_priv->Engine.instmem.priv = NULL;
	drm_free(priv, sizeof(*priv), DRM_MEM_DRIVER);
}

int
nv50_instmem_populate(struct drm_device *dev, struct nouveau_gpuobj *gpuobj, uint32_t *sz)
{
	if (gpuobj->im_backing)
		return -EINVAL;

	*sz = (*sz + (NV50_INSTMEM_PAGE_SIZE-1)) & ~(NV50_INSTMEM_PAGE_SIZE-1);
	if (*sz == 0)
		return -EINVAL;

	gpuobj->im_backing = nouveau_mem_alloc(dev, NV50_INSTMEM_PAGE_SIZE,
					       *sz, NOUVEAU_MEM_FB |
					       NOUVEAU_MEM_NOVM,
					       (struct drm_file *)-2);
	if (!gpuobj->im_backing) {
		DRM_ERROR("Couldn't allocate vram to back PRAMIN pages\n");
		return -ENOMEM;
	}

	return 0;
}

void
nv50_instmem_clear(struct drm_device *dev, struct nouveau_gpuobj *gpuobj)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;

	if (gpuobj && gpuobj->im_backing) {
		if (gpuobj->im_bound)
			dev_priv->Engine.instmem.unbind(dev, gpuobj);
		nouveau_mem_free(dev, gpuobj->im_backing);
		gpuobj->im_backing = NULL;
	}
}

int
nv50_instmem_bind(struct drm_device *dev, struct nouveau_gpuobj *gpuobj)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	nv50_instmem_priv *priv = dev_priv->Engine.instmem.priv;
	uint32_t pte, pte_end, vram;

	if (!gpuobj->im_backing || !gpuobj->im_pramin || gpuobj->im_bound)
		return -EINVAL;

	DRM_DEBUG("st=0x%0llx sz=0x%0llx\n",
		  gpuobj->im_pramin->start, gpuobj->im_pramin->size);

	pte     = (gpuobj->im_pramin->start >> 12) << 3;
	pte_end = ((gpuobj->im_pramin->size >> 12) << 3) + pte;
	vram    = gpuobj->im_backing->start;

	DRM_DEBUG("pramin=0x%llx, pte=%d, pte_end=%d\n",
		  gpuobj->im_pramin->start, pte, pte_end);
	DRM_DEBUG("first vram page: 0x%llx\n",
		  gpuobj->im_backing->start);

	while (pte < pte_end) {
		INSTANCE_WR(priv->pramin_pt->gpuobj, (pte + 0)/4, vram | 1);
		INSTANCE_WR(priv->pramin_pt->gpuobj, (pte + 4)/4, 0x00000000);

		pte += 8;
		vram += NV50_INSTMEM_PAGE_SIZE;
	}

	NV_WRITE(0x070000, 0x00000001);
	while(NV_READ(0x070000) & 1);
	NV_WRITE(0x100c80, 0x00040001);
	while(NV_READ(0x100c80) & 1);
	NV_WRITE(0x100c80, 0x00060001);
	while(NV_READ(0x100c80) & 1);

	gpuobj->im_bound = 1;
	return 0;
}

int
nv50_instmem_unbind(struct drm_device *dev, struct nouveau_gpuobj *gpuobj)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	nv50_instmem_priv *priv = dev_priv->Engine.instmem.priv;
	uint32_t pte, pte_end;

	if (gpuobj->im_bound == 0)
		return -EINVAL;

	pte     = (gpuobj->im_pramin->start >> 12) << 3;
	pte_end = ((gpuobj->im_pramin->size >> 12) << 3) + pte;
	while (pte < pte_end) {
		INSTANCE_WR(priv->pramin_pt->gpuobj, (pte + 0)/4, 0x00000009);
		INSTANCE_WR(priv->pramin_pt->gpuobj, (pte + 4)/4, 0x00000000);
		pte += 8;
	}

	gpuobj->im_bound = 0;
	return 0;
}
