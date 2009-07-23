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
#include "nouveau_drm.h"
#include "nouveau_drv.h"
#include "nouveau_reg.h"
#include "nouveau_swmthd.h"

void
nouveau_irq_preinstall(struct drm_device *dev)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;

	/* Master disable */
	NV_WRITE(NV03_PMC_INTR_EN_0, 0);
}

int
nouveau_irq_postinstall(struct drm_device *dev)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;

	/* Master enable */
	NV_WRITE(NV03_PMC_INTR_EN_0, NV_PMC_INTR_EN_0_MASTER_ENABLE);

	return 0;
}

void
nouveau_irq_uninstall(struct drm_device *dev)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;

	/* Master disable */
	NV_WRITE(NV03_PMC_INTR_EN_0, 0);
}

static void
nouveau_fifo_irq_handler(struct drm_device *dev)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct nouveau_engine *engine = &dev_priv->Engine;
	uint32_t status, reassign;

	reassign = NV_READ(NV03_PFIFO_CACHES) & 1;
	while ((status = NV_READ(NV03_PFIFO_INTR_0))) {
		uint32_t chid, get;

		NV_WRITE(NV03_PFIFO_CACHES, 0);

		chid = engine->fifo.channel_id(dev);
		get  = NV_READ(NV03_PFIFO_CACHE1_GET);

		if (status & NV_PFIFO_INTR_CACHE_ERROR) {
			uint32_t mthd, data;
			int ptr;

			ptr = get >> 2;
			if (dev_priv->card_type < NV_40) {
				mthd = NV_READ(NV04_PFIFO_CACHE1_METHOD(ptr));
				data = NV_READ(NV04_PFIFO_CACHE1_DATA(ptr));
			} else {
				mthd = NV_READ(NV40_PFIFO_CACHE1_METHOD(ptr));
				data = NV_READ(NV40_PFIFO_CACHE1_DATA(ptr));
			}

			DRM_INFO("PFIFO_CACHE_ERROR - "
				 "Ch %d/%d Mthd 0x%04x Data 0x%08x\n",
				 chid, (mthd >> 13) & 7, mthd & 0x1ffc, data);

			NV_WRITE(NV03_PFIFO_CACHE1_GET, get + 4);
			NV_WRITE(NV04_PFIFO_CACHE1_PULL0, 1);

			status &= ~NV_PFIFO_INTR_CACHE_ERROR;
			NV_WRITE(NV03_PFIFO_INTR_0, NV_PFIFO_INTR_CACHE_ERROR);
		}

		if (status & NV_PFIFO_INTR_DMA_PUSHER) {
			DRM_INFO("PFIFO_DMA_PUSHER - Ch %d\n", chid);

			status &= ~NV_PFIFO_INTR_DMA_PUSHER;
			NV_WRITE(NV03_PFIFO_INTR_0, NV_PFIFO_INTR_DMA_PUSHER);

			NV_WRITE(NV04_PFIFO_CACHE1_DMA_STATE, 0x00000000);
			if (NV_READ(NV04_PFIFO_CACHE1_DMA_PUT) != get)
				NV_WRITE(NV04_PFIFO_CACHE1_DMA_GET, get + 4);
		}

		if (status) {
			DRM_INFO("Unhandled PFIFO_INTR - 0x%08x\n", status);
			NV_WRITE(NV03_PFIFO_INTR_0, status);
			NV_WRITE(NV03_PMC_INTR_EN_0, 0);
		}

		NV_WRITE(NV03_PFIFO_CACHES, reassign);
	}

	NV_WRITE(NV03_PMC_INTR_0, NV_PMC_INTR_0_PFIFO_PENDING);
}

struct nouveau_bitfield_names {
	uint32_t mask;
	const char * name;
};

static struct nouveau_bitfield_names nouveau_nstatus_names[] =
{
	{ NV04_PGRAPH_NSTATUS_STATE_IN_USE,       "STATE_IN_USE" },
	{ NV04_PGRAPH_NSTATUS_INVALID_STATE,      "INVALID_STATE" },
	{ NV04_PGRAPH_NSTATUS_BAD_ARGUMENT,       "BAD_ARGUMENT" },
	{ NV04_PGRAPH_NSTATUS_PROTECTION_FAULT,   "PROTECTION_FAULT" }
};

static struct nouveau_bitfield_names nouveau_nstatus_names_nv10[] =
{
	{ NV10_PGRAPH_NSTATUS_STATE_IN_USE,       "STATE_IN_USE" },
	{ NV10_PGRAPH_NSTATUS_INVALID_STATE,      "INVALID_STATE" },
	{ NV10_PGRAPH_NSTATUS_BAD_ARGUMENT,       "BAD_ARGUMENT" },
	{ NV10_PGRAPH_NSTATUS_PROTECTION_FAULT,   "PROTECTION_FAULT" }
};

static struct nouveau_bitfield_names nouveau_nsource_names[] =
{
	{ NV03_PGRAPH_NSOURCE_NOTIFICATION,       "NOTIFICATION" },
	{ NV03_PGRAPH_NSOURCE_DATA_ERROR,         "DATA_ERROR" },
	{ NV03_PGRAPH_NSOURCE_PROTECTION_ERROR,   "PROTECTION_ERROR" },
	{ NV03_PGRAPH_NSOURCE_RANGE_EXCEPTION,    "RANGE_EXCEPTION" },
	{ NV03_PGRAPH_NSOURCE_LIMIT_COLOR,        "LIMIT_COLOR" },
	{ NV03_PGRAPH_NSOURCE_LIMIT_ZETA,         "LIMIT_ZETA" },
	{ NV03_PGRAPH_NSOURCE_ILLEGAL_MTHD,       "ILLEGAL_MTHD" },
	{ NV03_PGRAPH_NSOURCE_DMA_R_PROTECTION,   "DMA_R_PROTECTION" },
	{ NV03_PGRAPH_NSOURCE_DMA_W_PROTECTION,   "DMA_W_PROTECTION" },
	{ NV03_PGRAPH_NSOURCE_FORMAT_EXCEPTION,   "FORMAT_EXCEPTION" },
	{ NV03_PGRAPH_NSOURCE_PATCH_EXCEPTION,    "PATCH_EXCEPTION" },
	{ NV03_PGRAPH_NSOURCE_STATE_INVALID,      "STATE_INVALID" },
	{ NV03_PGRAPH_NSOURCE_DOUBLE_NOTIFY,      "DOUBLE_NOTIFY" },
	{ NV03_PGRAPH_NSOURCE_NOTIFY_IN_USE,      "NOTIFY_IN_USE" },
	{ NV03_PGRAPH_NSOURCE_METHOD_CNT,         "METHOD_CNT" },
	{ NV03_PGRAPH_NSOURCE_BFR_NOTIFICATION,   "BFR_NOTIFICATION" },
	{ NV03_PGRAPH_NSOURCE_DMA_VTX_PROTECTION, "DMA_VTX_PROTECTION" },
	{ NV03_PGRAPH_NSOURCE_DMA_WIDTH_A,        "DMA_WIDTH_A" },
	{ NV03_PGRAPH_NSOURCE_DMA_WIDTH_B,        "DMA_WIDTH_B" },
};

static void
nouveau_print_bitfield_names(uint32_t value,
                             const struct nouveau_bitfield_names *namelist,
                             const int namelist_len)
{
	int i;
	for(i=0; i<namelist_len; ++i) {
		uint32_t mask = namelist[i].mask;
		if(value & mask) {
			printk(" %s", namelist[i].name);
			value &= ~mask;
		}
	}
	if(value)
		printk(" (unknown bits 0x%08x)", value);
}

static int
nouveau_graph_chid_from_grctx(struct drm_device *dev)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	uint32_t inst;
	int i;

	if (dev_priv->card_type < NV_40)
		return dev_priv->Engine.fifo.channels;
	else
	if (dev_priv->card_type < NV_50)
		inst = (NV_READ(0x40032c) & 0xfffff) << 4;
	else
		inst = NV_READ(0x40032c) & 0xfffff;

	for (i = 0; i < dev_priv->Engine.fifo.channels; i++) {
		struct nouveau_channel *chan = dev_priv->fifos[i];

		if (!chan || !chan->ramin_grctx)
			continue;

		if (dev_priv->card_type < NV_50) {
			if (inst == chan->ramin_grctx->instance)
				break;
		} else {
			if (inst == INSTANCE_RD(chan->ramin_grctx->gpuobj, 0))
				break;
		}
	}

	return i;
}

static int
nouveau_graph_trapped_channel(struct drm_device *dev, int *channel_ret)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct nouveau_engine *engine = &dev_priv->Engine;
	int channel;

	if (dev_priv->card_type < NV_10)
		channel = (NV_READ(NV04_PGRAPH_TRAPPED_ADDR) >> 24) & 0xf;
	else
	if (dev_priv->card_type < NV_40)
		channel = (NV_READ(NV04_PGRAPH_TRAPPED_ADDR) >> 20) & 0x1f;
	else
		channel = nouveau_graph_chid_from_grctx(dev);

	if (channel >= engine->fifo.channels || !dev_priv->fifos[channel]) {
		DRM_ERROR("AIII, invalid/inactive channel id %d\n", channel);
		return -EINVAL;
	}

	*channel_ret = channel;
	return 0;
}

struct nouveau_pgraph_trap {
	int channel;
	int class;
	int subc, mthd, size;
	uint32_t data, data2;
	uint32_t nsource, nstatus;
};

static void
nouveau_graph_trap_info(struct drm_device *dev,
			struct nouveau_pgraph_trap *trap)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	uint32_t address;

	trap->nsource = trap->nstatus = 0;
	if (dev_priv->card_type < NV_50) {
		trap->nsource = NV_READ(NV03_PGRAPH_NSOURCE);
		trap->nstatus = NV_READ(NV03_PGRAPH_NSTATUS);
	}

	if (nouveau_graph_trapped_channel(dev, &trap->channel))
		trap->channel = -1;
	address = NV_READ(NV04_PGRAPH_TRAPPED_ADDR);

	trap->mthd = address & 0x1FFC;
	trap->data = NV_READ(NV04_PGRAPH_TRAPPED_DATA);
	if (dev_priv->card_type < NV_10) {
		trap->subc  = (address >> 13) & 0x7;
	} else {
		trap->subc  = (address >> 16) & 0x7;
		trap->data2 = NV_READ(NV10_PGRAPH_TRAPPED_DATA_HIGH);
	}

	if (dev_priv->card_type < NV_10) {
		trap->class = NV_READ(0x400180 + trap->subc*4) & 0xFF;
	} else if (dev_priv->card_type < NV_40) {
		trap->class = NV_READ(0x400160 + trap->subc*4) & 0xFFF;
	} else if (dev_priv->card_type < NV_50) {
		trap->class = NV_READ(0x400160 + trap->subc*4) & 0xFFFF;
	} else {
		trap->class = NV_READ(0x400814);
	}
}

static void
nouveau_graph_dump_trap_info(struct drm_device *dev, const char *id,
			     struct nouveau_pgraph_trap *trap)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	uint32_t nsource = trap->nsource, nstatus = trap->nstatus;

	DRM_INFO("%s - nSource:", id);
	nouveau_print_bitfield_names(nsource, nouveau_nsource_names,
	                             ARRAY_SIZE(nouveau_nsource_names));
	printk(", nStatus:");
	if (dev_priv->card_type < NV_10)
		nouveau_print_bitfield_names(nstatus, nouveau_nstatus_names,
	                             ARRAY_SIZE(nouveau_nstatus_names));
	else
		nouveau_print_bitfield_names(nstatus, nouveau_nstatus_names_nv10,
	                             ARRAY_SIZE(nouveau_nstatus_names_nv10));
	printk("\n");

	DRM_INFO("%s - Ch %d/%d Class 0x%04x Mthd 0x%04x Data 0x%08x:0x%08x\n",
		 id, trap->channel, trap->subc, trap->class, trap->mthd,
		 trap->data2, trap->data);
}

static inline void
nouveau_pgraph_intr_notify(struct drm_device *dev, uint32_t nsource)
{
	struct nouveau_pgraph_trap trap;
	int unhandled = 0;

	nouveau_graph_trap_info(dev, &trap);

	if (nsource & NV03_PGRAPH_NSOURCE_ILLEGAL_MTHD) {
		/* NV4 (nvidia TNT 1) reports software methods with
		 * PGRAPH NOTIFY ILLEGAL_MTHD
		 */
		DRM_DEBUG("Got NV04 software method method %x for class %#x\n",
			  trap.mthd, trap.class);

		if (nouveau_sw_method_execute(dev, trap.class, trap.mthd)) {
			DRM_ERROR("Unable to execute NV04 software method %x "
				  "for object class %x. Please report.\n",
				  trap.mthd, trap.class);
			unhandled = 1;
		}
	} else {
		unhandled = 1;
	}

	if (unhandled)
		nouveau_graph_dump_trap_info(dev, "PGRAPH_NOTIFY", &trap);
}

static inline void
nouveau_pgraph_intr_error(struct drm_device *dev, uint32_t nsource)
{
	struct nouveau_pgraph_trap trap;
	int unhandled = 0;

	nouveau_graph_trap_info(dev, &trap);
	trap.nsource = nsource;

	if (nsource & NV03_PGRAPH_NSOURCE_ILLEGAL_MTHD) {
		if (trap.channel >= 0 && trap.mthd == 0x0150) {
			nouveau_fence_handler(dev, trap.channel);
		} else
		if (nouveau_sw_method_execute(dev, trap.class, trap.mthd)) {
			unhandled = 1;
		}
	} else {
		unhandled = 1;
	}

	if (unhandled)
		nouveau_graph_dump_trap_info(dev, "PGRAPH_ERROR", &trap);
}

static inline void
nouveau_pgraph_intr_context_switch(struct drm_device *dev)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct nouveau_engine *engine = &dev_priv->Engine;
	uint32_t chid;

	chid = engine->fifo.channel_id(dev);
	DRM_DEBUG("PGRAPH context switch interrupt channel %x\n", chid);

	switch(dev_priv->card_type) {
	case NV_04:
	case NV_05:
		nouveau_nv04_context_switch(dev);
		break;
	case NV_10:
	case NV_11:
	case NV_17:
		nouveau_nv10_context_switch(dev);
		break;
	default:
		DRM_ERROR("Context switch not implemented\n");
		break;
	}
}

static void
nouveau_pgraph_irq_handler(struct drm_device *dev)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	uint32_t status;

	while ((status = NV_READ(NV03_PGRAPH_INTR))) {
		uint32_t nsource = NV_READ(NV03_PGRAPH_NSOURCE);

		if (status & NV_PGRAPH_INTR_NOTIFY) {
			nouveau_pgraph_intr_notify(dev, nsource);

			status &= ~NV_PGRAPH_INTR_NOTIFY;
			NV_WRITE(NV03_PGRAPH_INTR, NV_PGRAPH_INTR_NOTIFY);
		}

		if (status & NV_PGRAPH_INTR_ERROR) {
			nouveau_pgraph_intr_error(dev, nsource);

			status &= ~NV_PGRAPH_INTR_ERROR;
			NV_WRITE(NV03_PGRAPH_INTR, NV_PGRAPH_INTR_ERROR);
		}

		if (status & NV_PGRAPH_INTR_CONTEXT_SWITCH) {
			nouveau_pgraph_intr_context_switch(dev);

			status &= ~NV_PGRAPH_INTR_CONTEXT_SWITCH;
			NV_WRITE(NV03_PGRAPH_INTR,
				 NV_PGRAPH_INTR_CONTEXT_SWITCH);
		}

		if (status) {
			DRM_INFO("Unhandled PGRAPH_INTR - 0x%08x\n", status);
			NV_WRITE(NV03_PGRAPH_INTR, status);
		}

		if ((NV_READ(NV04_PGRAPH_FIFO) & (1 << 0)) == 0)
			NV_WRITE(NV04_PGRAPH_FIFO, 1);
	}

	NV_WRITE(NV03_PMC_INTR_0, NV_PMC_INTR_0_PGRAPH_PENDING);
}

static void
nv50_pgraph_irq_handler(struct drm_device *dev)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	uint32_t status, nsource;

	status = NV_READ(NV03_PGRAPH_INTR);
	nsource = NV_READ(NV03_PGRAPH_NSOURCE);

	if (status & 0x00000020) {
		nouveau_pgraph_intr_error(dev,
					  NV03_PGRAPH_NSOURCE_ILLEGAL_MTHD);

		status &= ~0x00000020;
		NV_WRITE(NV03_PGRAPH_INTR, 0x00000020);
	}

	if (status & 0x00100000) {
		nouveau_pgraph_intr_error(dev,
					  NV03_PGRAPH_NSOURCE_DATA_ERROR);

		status &= ~0x00100000;
		NV_WRITE(NV03_PGRAPH_INTR, 0x00100000);
	}

	if (status & 0x00200000) {
		int r;

		nouveau_pgraph_intr_error(dev, nsource |
					  NV03_PGRAPH_NSOURCE_PROTECTION_ERROR);

		DRM_ERROR("magic set 1:\n");
		for (r = 0x408900; r <= 0x408910; r += 4)
			DRM_ERROR("\t0x%08x: 0x%08x\n", r, NV_READ(r));
		NV_WRITE(0x408900, NV_READ(0x408904) | 0xc0000000);
		for (r = 0x408e08; r <= 0x408e24; r += 4)
			DRM_ERROR("\t0x%08x: 0x%08x\n", r, NV_READ(r));
		NV_WRITE(0x408e08, NV_READ(0x408e08) | 0xc0000000);

		DRM_ERROR("magic set 2:\n");
		for (r = 0x409900; r <= 0x409910; r += 4)
			DRM_ERROR("\t0x%08x: 0x%08x\n", r, NV_READ(r));
		NV_WRITE(0x409900, NV_READ(0x409904) | 0xc0000000);
		for (r = 0x409e08; r <= 0x409e24; r += 4)
			DRM_ERROR("\t0x%08x: 0x%08x\n", r, NV_READ(r));
		NV_WRITE(0x409e08, NV_READ(0x409e08) | 0xc0000000);

		status &= ~0x00200000;
		NV_WRITE(NV03_PGRAPH_NSOURCE, nsource);
		NV_WRITE(NV03_PGRAPH_INTR, 0x00200000);
	}

	if (status) {
		DRM_INFO("Unhandled PGRAPH_INTR - 0x%08x\n", status);
		NV_WRITE(NV03_PGRAPH_INTR, status);
	}

	{
		const int isb = (1 << 16) | (1 << 0);

		if ((NV_READ(0x400500) & isb) != isb)
			NV_WRITE(0x400500, NV_READ(0x400500) | isb);
	}

	NV_WRITE(NV03_PMC_INTR_0, NV_PMC_INTR_0_PGRAPH_PENDING);
}

static void
nouveau_crtc_irq_handler(struct drm_device *dev, int crtc)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;

	if (crtc&1) {
		NV_WRITE(NV_CRTC0_INTSTAT, NV_CRTC_INTR_VBLANK);
	}

	if (crtc&2) {
		NV_WRITE(NV_CRTC1_INTSTAT, NV_CRTC_INTR_VBLANK);
	}
}

static void
nouveau_nv50_display_irq_handler(struct drm_device *dev)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	uint32_t val = NV_READ(NV50_DISPLAY_SUPERVISOR);

	DRM_INFO("NV50_DISPLAY_INTR - 0x%08X\n", val);

	NV_WRITE(NV50_DISPLAY_SUPERVISOR, val);
}

static void
nouveau_nv50_i2c_irq_handler(struct drm_device *dev)
{
	struct drm_nouveau_private *dev_priv = dev->dev_private;

	DRM_INFO("NV50_I2C_INTR - 0x%08X\n", NV_READ(NV50_I2C_CONTROLLER));

	/* This seems to be the way to acknowledge an interrupt. */
	NV_WRITE(NV50_I2C_CONTROLLER, 0x7FFF7FFF);
}

irqreturn_t
nouveau_irq_handler(DRM_IRQ_ARGS)
{
	struct drm_device *dev = (struct drm_device*)arg;
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	uint32_t status;

	status = NV_READ(NV03_PMC_INTR_0);
	if (!status)
		return IRQ_NONE;

	if (status & NV_PMC_INTR_0_PFIFO_PENDING) {
		nouveau_fifo_irq_handler(dev);
		status &= ~NV_PMC_INTR_0_PFIFO_PENDING;
	}

	if (status & NV_PMC_INTR_0_PGRAPH_PENDING) {
		if (dev_priv->card_type >= NV_50)
			nv50_pgraph_irq_handler(dev);
		else
			nouveau_pgraph_irq_handler(dev);

		status &= ~NV_PMC_INTR_0_PGRAPH_PENDING;
	}

	if (status & NV_PMC_INTR_0_CRTCn_PENDING) {
		nouveau_crtc_irq_handler(dev, (status>>24)&3);
		status &= ~NV_PMC_INTR_0_CRTCn_PENDING;
	}

	if (status & NV_PMC_INTR_0_NV50_DISPLAY_PENDING) {
		nouveau_nv50_display_irq_handler(dev);
		status &= ~NV_PMC_INTR_0_NV50_DISPLAY_PENDING;
	}

	if (status & NV_PMC_INTR_0_NV50_I2C_PENDING) {
		nouveau_nv50_i2c_irq_handler(dev);
		status &= ~NV_PMC_INTR_0_NV50_I2C_PENDING;
	}

	if (status)
		DRM_ERROR("Unhandled PMC INTR status bits 0x%08x\n", status);

	return IRQ_HANDLED;
}
