/*
 * Copyright 2005 Stephane Marchesin
 * Copyright 2008 Stuart Bennett
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "drmP.h"
#include "drm.h"
#include "drm_sarea.h"
#include "nouveau_drv.h"
#include "nouveau_drm.h"

static int nouveau_init_card_mappings(struct drm_device *dev)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	int ret;

	/* resource 0 is mmio regs */
	/* resource 1 is linear FB */
	/* resource 2 is RAMIN (mmio regs + 0x1000000) */
	/* resource 6 is bios */

	/* map the mmio regs */
	ret = drm_addmap(dev, drm_get_resource_start(dev, 0),
			      drm_get_resource_len(dev, 0),
			      _DRM_REGISTERS, _DRM_READ_ONLY, &dev_priv->mmio);
	if (ret) {
		DRM_ERROR("Unable to initialize the mmio mapping (%d). "
			  "Please report your setup to " DRIVER_EMAIL "\n",
			  ret);
		return -EINVAL;
	}
	DRM_DEBUG("regs mapped ok at 0x%lx\n", dev_priv->mmio->offset);

	/* map larger RAMIN aperture on NV40 cards */
	dev_priv->ramin = NULL;
	if (dev_priv->card_type >= NV_40) {
		int ramin_resource = 2;
		if (drm_get_resource_len(dev, ramin_resource) == 0)
			ramin_resource = 3;

		ret = drm_addmap(dev,
				 drm_get_resource_start(dev, ramin_resource),
				 drm_get_resource_len(dev, ramin_resource),
				 _DRM_REGISTERS, _DRM_READ_ONLY,
				 &dev_priv->ramin);
		if (ret) {
			DRM_ERROR("Failed to init RAMIN mapping, "
				  "limited instance memory available\n");
			dev_priv->ramin = NULL;
		}
	}

	/* On older cards (or if the above failed), create a map covering
	 * the BAR0 PRAMIN aperture */
	if (!dev_priv->ramin) {
		ret = drm_addmap(dev,
				 drm_get_resource_start(dev, 0) + NV_RAMIN,
				 (1*1024*1024),
				 _DRM_REGISTERS, _DRM_READ_ONLY,
				 &dev_priv->ramin);
		if (ret) {
			DRM_ERROR("Failed to map BAR0 PRAMIN: %d\n", ret);
			return ret;
		}
	}

	return 0;
}

static int nouveau_stub_init(struct drm_device *dev) { return 0; }
static void nouveau_stub_takedown(struct drm_device *dev) {}

static int nouveau_init_engine_ptrs(struct drm_device *dev)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct nouveau_engine *engine = &dev_priv->Engine;

	switch (dev_priv->chipset & 0xf0) {
	case 0x00:
		engine->instmem.init	= nv04_instmem_init;
		engine->instmem.takedown= nv04_instmem_takedown;
		engine->instmem.populate	= nv04_instmem_populate;
		engine->instmem.clear		= nv04_instmem_clear;
		engine->instmem.bind		= nv04_instmem_bind;
		engine->instmem.unbind		= nv04_instmem_unbind;
		engine->mc.init		= nv04_mc_init;
		engine->mc.takedown	= nv04_mc_takedown;
		engine->timer.init	= nv04_timer_init;
		engine->timer.read	= nv04_timer_read;
		engine->timer.takedown	= nv04_timer_takedown;
		engine->fb.init		= nv04_fb_init;
		engine->fb.takedown	= nv04_fb_takedown;
		engine->graph.init	= nv04_graph_init;
		engine->graph.takedown	= nv04_graph_takedown;
		engine->graph.create_context	= nv04_graph_create_context;
		engine->graph.destroy_context	= nv04_graph_destroy_context;
		engine->graph.load_context	= nv04_graph_load_context;
		engine->graph.save_context	= nv04_graph_save_context;
		engine->fifo.channels	= 16;
		engine->fifo.init	= nouveau_fifo_init;
		engine->fifo.takedown	= nouveau_stub_takedown;
		engine->fifo.channel_id		= nv04_fifo_channel_id;
		engine->fifo.create_context	= nv04_fifo_create_context;
		engine->fifo.destroy_context	= nv04_fifo_destroy_context;
		engine->fifo.load_context	= nv04_fifo_load_context;
		engine->fifo.save_context	= nv04_fifo_save_context;
		break;
	case 0x10:
		engine->instmem.init	= nv04_instmem_init;
		engine->instmem.takedown= nv04_instmem_takedown;
		engine->instmem.populate	= nv04_instmem_populate;
		engine->instmem.clear		= nv04_instmem_clear;
		engine->instmem.bind		= nv04_instmem_bind;
		engine->instmem.unbind		= nv04_instmem_unbind;
		engine->mc.init		= nv04_mc_init;
		engine->mc.takedown	= nv04_mc_takedown;
		engine->timer.init	= nv04_timer_init;
		engine->timer.read	= nv04_timer_read;
		engine->timer.takedown	= nv04_timer_takedown;
		engine->fb.init		= nv10_fb_init;
		engine->fb.takedown	= nv10_fb_takedown;
		engine->graph.init	= nv10_graph_init;
		engine->graph.takedown	= nv10_graph_takedown;
		engine->graph.create_context	= nv10_graph_create_context;
		engine->graph.destroy_context	= nv10_graph_destroy_context;
		engine->graph.load_context	= nv10_graph_load_context;
		engine->graph.save_context	= nv10_graph_save_context;
		engine->fifo.channels	= 32;
		engine->fifo.init	= nouveau_fifo_init;
		engine->fifo.takedown	= nouveau_stub_takedown;
		engine->fifo.channel_id		= nv10_fifo_channel_id;
		engine->fifo.create_context	= nv10_fifo_create_context;
		engine->fifo.destroy_context	= nv10_fifo_destroy_context;
		engine->fifo.load_context	= nv10_fifo_load_context;
		engine->fifo.save_context	= nv10_fifo_save_context;
		break;
	case 0x20:
		engine->instmem.init	= nv04_instmem_init;
		engine->instmem.takedown= nv04_instmem_takedown;
		engine->instmem.populate	= nv04_instmem_populate;
		engine->instmem.clear		= nv04_instmem_clear;
		engine->instmem.bind		= nv04_instmem_bind;
		engine->instmem.unbind		= nv04_instmem_unbind;
		engine->mc.init		= nv04_mc_init;
		engine->mc.takedown	= nv04_mc_takedown;
		engine->timer.init	= nv04_timer_init;
		engine->timer.read	= nv04_timer_read;
		engine->timer.takedown	= nv04_timer_takedown;
		engine->fb.init		= nv10_fb_init;
		engine->fb.takedown	= nv10_fb_takedown;
		engine->graph.init	= nv20_graph_init;
		engine->graph.takedown	= nv20_graph_takedown;
		engine->graph.create_context	= nv20_graph_create_context;
		engine->graph.destroy_context	= nv20_graph_destroy_context;
		engine->graph.load_context	= nv20_graph_load_context;
		engine->graph.save_context	= nv20_graph_save_context;
		engine->fifo.channels	= 32;
		engine->fifo.init	= nouveau_fifo_init;
		engine->fifo.takedown	= nouveau_stub_takedown;
		engine->fifo.channel_id		= nv10_fifo_channel_id;
		engine->fifo.create_context	= nv10_fifo_create_context;
		engine->fifo.destroy_context	= nv10_fifo_destroy_context;
		engine->fifo.load_context	= nv10_fifo_load_context;
		engine->fifo.save_context	= nv10_fifo_save_context;
		break;
	case 0x30:
		engine->instmem.init	= nv04_instmem_init;
		engine->instmem.takedown= nv04_instmem_takedown;
		engine->instmem.populate	= nv04_instmem_populate;
		engine->instmem.clear		= nv04_instmem_clear;
		engine->instmem.bind		= nv04_instmem_bind;
		engine->instmem.unbind		= nv04_instmem_unbind;
		engine->mc.init		= nv04_mc_init;
		engine->mc.takedown	= nv04_mc_takedown;
		engine->timer.init	= nv04_timer_init;
		engine->timer.read	= nv04_timer_read;
		engine->timer.takedown	= nv04_timer_takedown;
		engine->fb.init		= nv10_fb_init;
		engine->fb.takedown	= nv10_fb_takedown;
		engine->graph.init	= nv30_graph_init;
		engine->graph.takedown	= nv20_graph_takedown;
		engine->graph.create_context	= nv20_graph_create_context;
		engine->graph.destroy_context	= nv20_graph_destroy_context;
		engine->graph.load_context	= nv20_graph_load_context;
		engine->graph.save_context	= nv20_graph_save_context;
		engine->fifo.channels	= 32;
		engine->fifo.init	= nouveau_fifo_init;
		engine->fifo.takedown	= nouveau_stub_takedown;
		engine->fifo.channel_id		= nv10_fifo_channel_id;
		engine->fifo.create_context	= nv10_fifo_create_context;
		engine->fifo.destroy_context	= nv10_fifo_destroy_context;
		engine->fifo.load_context	= nv10_fifo_load_context;
		engine->fifo.save_context	= nv10_fifo_save_context;
		break;
	case 0x40:
	case 0x60:
		engine->instmem.init	= nv04_instmem_init;
		engine->instmem.takedown= nv04_instmem_takedown;
		engine->instmem.populate	= nv04_instmem_populate;
		engine->instmem.clear		= nv04_instmem_clear;
		engine->instmem.bind		= nv04_instmem_bind;
		engine->instmem.unbind		= nv04_instmem_unbind;
		engine->mc.init		= nv40_mc_init;
		engine->mc.takedown	= nv40_mc_takedown;
		engine->timer.init	= nv04_timer_init;
		engine->timer.read	= nv04_timer_read;
		engine->timer.takedown	= nv04_timer_takedown;
		engine->fb.init		= nv40_fb_init;
		engine->fb.takedown	= nv40_fb_takedown;
		engine->graph.init	= nv40_graph_init;
		engine->graph.takedown	= nv40_graph_takedown;
		engine->graph.create_context	= nv40_graph_create_context;
		engine->graph.destroy_context	= nv40_graph_destroy_context;
		engine->graph.load_context	= nv40_graph_load_context;
		engine->graph.save_context	= nv40_graph_save_context;
		engine->fifo.channels	= 32;
		engine->fifo.init	= nv40_fifo_init;
		engine->fifo.takedown	= nouveau_stub_takedown;
		engine->fifo.channel_id		= nv10_fifo_channel_id;
		engine->fifo.create_context	= nv40_fifo_create_context;
		engine->fifo.destroy_context	= nv40_fifo_destroy_context;
		engine->fifo.load_context	= nv40_fifo_load_context;
		engine->fifo.save_context	= nv40_fifo_save_context;
		break;
	case 0x50:
	case 0x80: /* gotta love NVIDIA's consistency.. */
	case 0x90:
	case 0xA0:
		engine->instmem.init	= nv50_instmem_init;
		engine->instmem.takedown= nv50_instmem_takedown;
		engine->instmem.populate	= nv50_instmem_populate;
		engine->instmem.clear		= nv50_instmem_clear;
		engine->instmem.bind		= nv50_instmem_bind;
		engine->instmem.unbind		= nv50_instmem_unbind;
		engine->mc.init		= nv50_mc_init;
		engine->mc.takedown	= nv50_mc_takedown;
		engine->timer.init	= nv04_timer_init;
		engine->timer.read	= nv04_timer_read;
		engine->timer.takedown	= nv04_timer_takedown;
		engine->fb.init		= nouveau_stub_init;
		engine->fb.takedown	= nouveau_stub_takedown;
		engine->graph.init	= nv50_graph_init;
		engine->graph.takedown	= nv50_graph_takedown;
		engine->graph.create_context	= nv50_graph_create_context;
		engine->graph.destroy_context	= nv50_graph_destroy_context;
		engine->graph.load_context	= nv50_graph_load_context;
		engine->graph.save_context	= nv50_graph_save_context;
		engine->fifo.channels	= 128;
		engine->fifo.init	= nv50_fifo_init;
		engine->fifo.takedown	= nv50_fifo_takedown;
		engine->fifo.channel_id		= nv50_fifo_channel_id;
		engine->fifo.create_context	= nv50_fifo_create_context;
		engine->fifo.destroy_context	= nv50_fifo_destroy_context;
		engine->fifo.load_context	= nv50_fifo_load_context;
		engine->fifo.save_context	= nv50_fifo_save_context;
		break;
	default:
		DRM_ERROR("NV%02x unsupported\n", dev_priv->chipset);
		return 1;
	}

	return 0;
}

int
nouveau_card_init(struct drm_device *dev)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct nouveau_engine *engine;
	int ret;

	DRM_DEBUG("prev state = %d\n", dev_priv->init_state);

	if (dev_priv->init_state == NOUVEAU_CARD_INIT_DONE)
		return 0;
	dev_priv->ttm = 0;

	/* Determine exact chipset we're running on */
	if (dev_priv->card_type < NV_10)
		dev_priv->chipset = dev_priv->card_type;
	else
		dev_priv->chipset =
			(NV_READ(NV03_PMC_BOOT_0) & 0x0ff00000) >> 20;

	/* Initialise internal driver API hooks */
	ret = nouveau_init_engine_ptrs(dev);
	if (ret) return ret;
	engine = &dev_priv->Engine;
	dev_priv->init_state = NOUVEAU_CARD_INIT_FAILED;

	ret = nouveau_gpuobj_early_init(dev);
	if (ret) return ret;

	/* Initialise instance memory, must happen before mem_init so we
	 * know exactly how much VRAM we're able to use for "normal"
	 * purposes.
	 */
	ret = engine->instmem.init(dev);
	if (ret) return ret;

	/* Setup the memory manager */
	if (dev_priv->ttm) {
		ret = nouveau_mem_init_ttm(dev);
		if (ret) return ret;
	} else {
		ret = nouveau_mem_init(dev);
		if (ret) return ret;
	}

	ret = nouveau_gpuobj_init(dev);
	if (ret) return ret;

	/* Parse BIOS tables / Run init tables? */

	/* PMC */
	ret = engine->mc.init(dev);
	if (ret) return ret;

	/* PTIMER */
	ret = engine->timer.init(dev);
	if (ret) return ret;

	/* PFB */
	ret = engine->fb.init(dev);
	if (ret) return ret;

	/* PGRAPH */
	ret = engine->graph.init(dev);
	if (ret) return ret;

	/* PFIFO */
	ret = engine->fifo.init(dev);
	if (ret) return ret;

	/* this call irq_preinstall, register irq handler and
	 * call irq_postinstall
	 */
	ret = drm_irq_install(dev);
	if (ret) return ret;

	/* what about PVIDEO/PCRTC/PRAMDAC etc? */

	ret = nouveau_dma_channel_init(dev);
	if (ret) return ret;

	ret = nouveau_backlight_init(dev);
	if (ret)
		DRM_ERROR("Error code %d when trying to register backlight\n",
			  ret);

	dev_priv->init_state = NOUVEAU_CARD_INIT_DONE;
	return 0;
}

static void nouveau_card_takedown(struct drm_device *dev)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct nouveau_engine *engine = &dev_priv->Engine;

	DRM_DEBUG("prev state = %d\n", dev_priv->init_state);

	if (dev_priv->init_state != NOUVEAU_CARD_INIT_DOWN) {
		nouveau_backlight_exit(dev);

		nouveau_dma_channel_takedown(dev);

		engine->fifo.takedown(dev);
		engine->graph.takedown(dev);
		engine->fb.takedown(dev);
		engine->timer.takedown(dev);
		engine->mc.takedown(dev);

		nouveau_sgdma_nottm_hack_takedown(dev);
		nouveau_sgdma_takedown(dev);

		nouveau_gpuobj_takedown(dev);
		nouveau_gpuobj_del(dev, &dev_priv->vm_vram_pt);

		nouveau_mem_close(dev);
		engine->instmem.takedown(dev);

		drm_irq_uninstall(dev);

		nouveau_gpuobj_late_takedown(dev);

		dev_priv->init_state = NOUVEAU_CARD_INIT_DOWN;
	}
}

/* here a client dies, release the stuff that was allocated for its
 * file_priv */
void nouveau_preclose(struct drm_device *dev, struct drm_file *file_priv)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;

	nouveau_fifo_cleanup(dev, file_priv);
	nouveau_mem_release(file_priv,dev_priv->fb_heap);
	nouveau_mem_release(file_priv,dev_priv->agp_heap);
	nouveau_mem_release(file_priv,dev_priv->pci_heap);
}

/* first module load, setup the mmio/fb mapping */
int nouveau_firstopen(struct drm_device *dev)
{
#if defined(__powerpc__)
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct device_node *dn;
#endif
	int ret;
	/* Map any PCI resources we need on the card */
	ret = nouveau_init_card_mappings(dev);
	if (ret) return ret;

#if defined(__powerpc__)
	/* Put the card in BE mode if it's not */
	if (NV_READ(NV03_PMC_BOOT_1))
		NV_WRITE(NV03_PMC_BOOT_1,0x00000001);

	DRM_MEMORYBARRIER();
#endif

#if defined(__linux__) && defined(__powerpc__)
	/* if we have an OF card, copy vbios to RAMIN */
	dn = pci_device_to_OF_node(dev->pdev);
	if (dn)
	{
		int size;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22))
		const uint32_t *bios = of_get_property(dn, "NVDA,BMP", &size);
#else
		const uint32_t *bios = get_property(dn, "NVDA,BMP", &size);
#endif
		if (bios)
		{
			int i;
			for(i=0;i<size;i+=4)
				NV_WI32(i, bios[i/4]);
			DRM_INFO("OF bios successfully copied (%d bytes)\n",size);
		}
		else
			DRM_INFO("Unable to get the OF bios\n");
	}
	else
		DRM_INFO("Unable to get the OF node\n");
#endif
	return 0;
}

#define NV40_CHIPSET_MASK 0x00000baf
#define NV44_CHIPSET_MASK 0x00005450

int nouveau_load(struct drm_device *dev, unsigned long flags)
{
	struct drm_nouveau_private *dev_priv;
	void __iomem *regs;
	uint32_t reg0,reg1;
	uint8_t architecture = 0;

	dev_priv = drm_calloc(1, sizeof(*dev_priv), DRM_MEM_DRIVER);
	if (!dev_priv)
		return -ENOMEM;

	dev_priv->flags = flags & NOUVEAU_FLAGS;
	dev_priv->init_state = NOUVEAU_CARD_INIT_DOWN;

	DRM_DEBUG("vendor: 0x%X device: 0x%X class: 0x%X\n", dev->pci_vendor, dev->pci_device, dev->pdev->class);

	/* Time to determine the card architecture */
	regs = ioremap_nocache(pci_resource_start(dev->pdev, 0), 0x8);
	if (!regs) {
		DRM_ERROR("Could not ioremap to determine register\n");
		return -ENOMEM;
	}

	reg0 = readl(regs+NV03_PMC_BOOT_0);
	reg1 = readl(regs+NV03_PMC_BOOT_1);
#if defined(__powerpc__)
	if (reg1)
		reg0=__swab32(reg0);
#endif

	/* We're dealing with >=NV10 */
	if ((reg0 & 0x0f000000) > 0 ) {
		/* Bit 27-20 contain the architecture in hex */
		architecture = (reg0 & 0xff00000) >> 20;
	/* NV04 or NV05 */
	} else if ((reg0 & 0xff00fff0) == 0x20004000) {
		architecture = 0x04;
	}

	iounmap(regs);

	if (architecture >= 0x80) {
		dev_priv->card_type = NV_50;
	} else if (architecture >= 0x60) {
		/* FIXME we need to figure out who's who for NV6x */
		dev_priv->card_type = NV_44;
	} else if (architecture >= 0x50) {
		dev_priv->card_type = NV_50;
	} else if (architecture >= 0x40) {
		uint8_t subarch = architecture & 0xf;
		/* Selection criteria borrowed from NV40EXA */
		if (NV40_CHIPSET_MASK & (1 << subarch)) {
			dev_priv->card_type = NV_40;
		} else if (NV44_CHIPSET_MASK & (1 << subarch)) {
			dev_priv->card_type = NV_44;
		} else {
			dev_priv->card_type = NV_UNKNOWN;
		}
	} else if (architecture >= 0x30) {
		dev_priv->card_type = NV_30;
	} else if (architecture >= 0x20) {
		dev_priv->card_type = NV_20;
	} else if (architecture >= 0x17) {
		dev_priv->card_type = NV_17;
	} else if (architecture >= 0x11) {
		dev_priv->card_type = NV_11;
	} else if (architecture >= 0x10) {
		dev_priv->card_type = NV_10;
	} else if (architecture >= 0x04) {
		dev_priv->card_type = NV_04;
	} else {
		dev_priv->card_type = NV_UNKNOWN;
	}

	DRM_INFO("Detected an NV%d generation card (0x%08x)\n", dev_priv->card_type,reg0);

	if (dev_priv->card_type == NV_UNKNOWN) {
		return -EINVAL;
	}

	/* Special flags */
	if (dev->pci_device == 0x01a0) {
		dev_priv->flags |= NV_NFORCE;
	} else if (dev->pci_device == 0x01f0) {
		dev_priv->flags |= NV_NFORCE2;
	}

	dev->dev_private = (void *)dev_priv;

	return 0;
}

void nouveau_lastclose(struct drm_device *dev)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;

	/* In the case of an error dev_priv may not be be allocated yet */
	if (dev_priv && dev_priv->card_type) {
		nouveau_card_takedown(dev);

		if(dev_priv->fb_mtrr>0)
		{
			drm_mtrr_del(dev_priv->fb_mtrr, drm_get_resource_start(dev, 1),nouveau_mem_fb_amount(dev), DRM_MTRR_WC);
			dev_priv->fb_mtrr=0;
		}
	}
}

int nouveau_unload(struct drm_device *dev)
{
	drm_free(dev->dev_private, sizeof(*dev->dev_private), DRM_MEM_DRIVER);
	dev->dev_private = NULL;
	return 0;
}

int
nouveau_ioctl_card_init(struct drm_device *dev, void *data,
			struct drm_file *file_priv)
{
	return nouveau_card_init(dev);
}

int nouveau_ioctl_getparam(struct drm_device *dev, void *data, struct drm_file *file_priv)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct drm_nouveau_getparam *getparam = data;

	NOUVEAU_CHECK_INITIALISED_WITH_RETURN;

	switch (getparam->param) {
	case NOUVEAU_GETPARAM_CHIPSET_ID:
		getparam->value = dev_priv->chipset;
		break;
	case NOUVEAU_GETPARAM_PCI_VENDOR:
		getparam->value=dev->pci_vendor;
		break;
	case NOUVEAU_GETPARAM_PCI_DEVICE:
		getparam->value=dev->pci_device;
		break;
	case NOUVEAU_GETPARAM_BUS_TYPE:
		if (drm_device_is_agp(dev))
			getparam->value=NV_AGP;
		else if (drm_device_is_pcie(dev))
			getparam->value=NV_PCIE;
		else
			getparam->value=NV_PCI;
		break;
	case NOUVEAU_GETPARAM_FB_PHYSICAL:
		getparam->value=dev_priv->fb_phys;
		break;
	case NOUVEAU_GETPARAM_AGP_PHYSICAL:
		getparam->value=dev_priv->gart_info.aper_base;
		break;
	case NOUVEAU_GETPARAM_PCI_PHYSICAL:
		if ( dev -> sg )
			getparam->value=(unsigned long)dev->sg->virtual;
		else
		     {
		     DRM_ERROR("Requested PCIGART address, while no PCIGART was created\n");
		     return -EINVAL;
		     }
		break;
	case NOUVEAU_GETPARAM_FB_SIZE:
		getparam->value=dev_priv->fb_available_size;
		break;
	case NOUVEAU_GETPARAM_AGP_SIZE:
		getparam->value=dev_priv->gart_info.aper_size;
		break;
	case NOUVEAU_GETPARAM_MM_ENABLED:
		getparam->value = 0;
		break;
	case NOUVEAU_GETPARAM_VM_VRAM_BASE:
		if (dev_priv->card_type >= NV_50)
			getparam->value = 0x20000000;
		else
			getparam->value = 0;
		break;
	default:
		DRM_ERROR("unknown parameter %lld\n", getparam->param);
		return -EINVAL;
	}

	return 0;
}

int nouveau_ioctl_setparam(struct drm_device *dev, void *data, struct drm_file *file_priv)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct drm_nouveau_setparam *setparam = data;

	NOUVEAU_CHECK_INITIALISED_WITH_RETURN;

	switch (setparam->param) {
	case NOUVEAU_SETPARAM_CMDBUF_LOCATION:
		switch (setparam->value) {
		case NOUVEAU_MEM_AGP:
		case NOUVEAU_MEM_FB:
		case NOUVEAU_MEM_PCI:
		case NOUVEAU_MEM_AGP | NOUVEAU_MEM_PCI_ACCEPTABLE:
			break;
		default:
			DRM_ERROR("invalid CMDBUF_LOCATION value=%lld\n",
					setparam->value);
			return -EINVAL;
		}
		dev_priv->config.cmdbuf.location = setparam->value;
		break;
	case NOUVEAU_SETPARAM_CMDBUF_SIZE:
		dev_priv->config.cmdbuf.size = setparam->value;
		break;
	default:
		DRM_ERROR("unknown parameter %lld\n", setparam->param);
		return -EINVAL;
	}

	return 0;
}

/* waits for idle */
void nouveau_wait_for_idle(struct drm_device *dev)
{
	struct drm_nouveau_private *dev_priv=dev->dev_private;
	switch(dev_priv->card_type) {
	case NV_50:
		break;
	default: {
		/* This stuff is more or less a copy of what is seen
		 * in nv28 kmmio dump.
		 */
		uint64_t started = dev_priv->Engine.timer.read(dev);
		uint64_t stopped = started;
		uint32_t status;
		do {
			uint32_t pmc_e = NV_READ(NV03_PMC_ENABLE);
			(void)pmc_e;
			status = NV_READ(NV04_PGRAPH_STATUS);
			if (!status)
				break;
			stopped = dev_priv->Engine.timer.read(dev);
		/* It'll never wrap anyway... */
		} while (stopped - started < 1000000000ULL);
		if (status)
			DRM_ERROR("timed out with status 0x%08x\n",
			          status);
	}
	}
}

static int nouveau_suspend(struct drm_device *dev)
{
	struct mem_block *p;
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct nouveau_suspend_resume *susres = &dev_priv->susres;
	struct nouveau_engine *engine = &dev_priv->Engine;
	struct nouveau_channel *current_fifo;
	int i;

	if (dev_priv->card_type >= NV_50) {
		DRM_DEBUG("Suspend not supported for NV50+\n");
		return -ENODEV;
	}

	drm_free(susres->ramin_copy, susres->ramin_size, DRM_MEM_DRIVER);
	susres->ramin_size = 0;
	list_for_each(p, dev_priv->ramin_heap)
		if (p->file_priv && (p->start + p->size) > susres->ramin_size)
			susres->ramin_size = p->start + p->size;
	if (!(susres->ramin_copy = drm_alloc(susres->ramin_size, DRM_MEM_DRIVER))) {
		DRM_ERROR("Couldn't alloc RAMIN backing for suspend\n");
		return -ENOMEM;
	}

	for (i = 0; i < engine->fifo.channels; i++) {
		uint64_t t_start = engine->timer.read(dev);

		if (dev_priv->fifos[i] == NULL)
			continue;

		/* Give the channel a chance to idle, wait 2s (hopefully) */
		while (!nouveau_channel_idle(dev_priv->fifos[i]))
			if (engine->timer.read(dev) - t_start > 2000000000ULL) {
				DRM_ERROR("Failed to idle channel %d before"
					  "suspend.", dev_priv->fifos[i]->id);
				return -EBUSY;
			}
	}
	nouveau_wait_for_idle(dev);

	NV_WRITE(NV04_PGRAPH_FIFO, 0);
	/* disable the fifo caches */
	NV_WRITE(NV03_PFIFO_CACHES, 0x00000000);
	NV_WRITE(NV04_PFIFO_CACHE1_DMA_PUSH,
		 NV_READ(NV04_PFIFO_CACHE1_DMA_PUSH) & ~1);
	NV_WRITE(NV03_PFIFO_CACHE1_PUSH0, 0x00000000);
	NV_WRITE(NV04_PFIFO_CACHE1_PULL0, 0x00000000);

	susres->fifo_mode = NV_READ(NV04_PFIFO_MODE);

	if (dev_priv->card_type >= NV_10) {
		susres->graph_state = NV_READ(NV10_PGRAPH_STATE);
		susres->graph_ctx_control = NV_READ(NV10_PGRAPH_CTX_CONTROL);
	} else {
		susres->graph_state = NV_READ(NV04_PGRAPH_STATE);
		susres->graph_ctx_control = NV_READ(NV04_PGRAPH_CTX_CONTROL);
	}

	current_fifo = dev_priv->fifos[engine->fifo.channel_id(dev)];
	/* channel may have been deleted but no replacement yet loaded */
	if (current_fifo) {
		engine->fifo.save_context(current_fifo);
		engine->graph.save_context(current_fifo);
	}
	nouveau_wait_for_idle(dev);

	for (i = 0; i < susres->ramin_size / 4; i++)
		susres->ramin_copy[i] = NV_RI32(i << 2);

	/* reenable the fifo caches */
	NV_WRITE(NV04_PFIFO_CACHE1_DMA_PUSH,
		 NV_READ(NV04_PFIFO_CACHE1_DMA_PUSH) | 1);
	NV_WRITE(NV03_PFIFO_CACHE1_PUSH0, 0x00000001);
	NV_WRITE(NV04_PFIFO_CACHE1_PULL0, 0x00000001);
	NV_WRITE(NV03_PFIFO_CACHES, 0x00000001);
	NV_WRITE(NV04_PGRAPH_FIFO, 1);

	return 0;
}

static int nouveau_resume(struct drm_device *dev)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct nouveau_suspend_resume *susres = &dev_priv->susres;
	struct nouveau_engine *engine = &dev_priv->Engine;
	int i;

	if (!susres->ramin_copy)
		return -EINVAL;

	DRM_DEBUG("Doing resume\n");

	if (dev_priv->gart_info.type == NOUVEAU_GART_AGP) {
		struct drm_agp_info info;
		struct drm_agp_mode mode;

		/* agp bridge drivers don't re-enable agp on resume. lame. */
		if ((i = drm_agp_info(dev, &info))) {
			DRM_ERROR("Unable to get AGP info: %d\n", i);
			return i;
		}
		mode.mode = info.mode;
		if ((i = drm_agp_enable(dev, mode))) {
			DRM_ERROR("Unable to enable AGP: %d\n", i);
			return i;
		}
	}

	for (i = 0; i < susres->ramin_size / 4; i++)
		NV_WI32(i << 2, susres->ramin_copy[i]);

	engine->mc.init(dev);
	engine->timer.init(dev);
	engine->fb.init(dev);
	engine->graph.init(dev);
	engine->fifo.init(dev);

	NV_WRITE(NV04_PGRAPH_FIFO, 0);
	/* disable the fifo caches */
	NV_WRITE(NV03_PFIFO_CACHES, 0x00000000);
	NV_WRITE(NV04_PFIFO_CACHE1_DMA_PUSH,
		 NV_READ(NV04_PFIFO_CACHE1_DMA_PUSH) & ~1);
	NV_WRITE(NV03_PFIFO_CACHE1_PUSH0, 0x00000000);
	NV_WRITE(NV04_PFIFO_CACHE1_PULL0, 0x00000000);

	/* PMC power cycling PFIFO in init clobbers some of the stuff stored in
	 * PRAMIN (such as NV04_PFIFO_CACHE1_DMA_INSTANCE). this is unhelpful
	 */
	for (i = 0; i < susres->ramin_size / 4; i++)
		NV_WI32(i << 2, susres->ramin_copy[i]);

	engine->fifo.load_context(dev_priv->fifos[0]);
	NV_WRITE(NV04_PFIFO_MODE, susres->fifo_mode);

	engine->graph.load_context(dev_priv->fifos[0]);
	nouveau_wait_for_idle(dev);

	if (dev_priv->card_type >= NV_10) {
		NV_WRITE(NV10_PGRAPH_STATE, susres->graph_state);
		NV_WRITE(NV10_PGRAPH_CTX_CONTROL, susres->graph_ctx_control);
	} else {
		NV_WRITE(NV04_PGRAPH_STATE, susres->graph_state);
		NV_WRITE(NV04_PGRAPH_CTX_CONTROL, susres->graph_ctx_control);
	}

	/* reenable the fifo caches */
	NV_WRITE(NV04_PFIFO_CACHE1_DMA_PUSH,
		 NV_READ(NV04_PFIFO_CACHE1_DMA_PUSH) | 1);
	NV_WRITE(NV03_PFIFO_CACHE1_PUSH0, 0x00000001);
	NV_WRITE(NV04_PFIFO_CACHE1_PULL0, 0x00000001);
	NV_WRITE(NV03_PFIFO_CACHES, 0x00000001);
	NV_WRITE(NV04_PGRAPH_FIFO, 0x1);

	if (dev->irq_enabled)
		nouveau_irq_postinstall(dev);

	drm_free(susres->ramin_copy, susres->ramin_size, DRM_MEM_DRIVER);
	susres->ramin_copy = NULL;
	susres->ramin_size = 0;

	return 0;
}

int nouveau_ioctl_suspend(struct drm_device *dev, void *data,
				 struct drm_file *file_priv)
{
	NOUVEAU_CHECK_INITIALISED_WITH_RETURN;

	return nouveau_suspend(dev);
}

int nouveau_ioctl_resume(struct drm_device *dev, void *data,
				struct drm_file *file_priv)
{
	NOUVEAU_CHECK_INITIALISED_WITH_RETURN;

	return nouveau_resume(dev);
}
