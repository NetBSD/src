/*
 * Copyright 2005 Stephane Marchesin.
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
 * VA LINUX SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef __NOUVEAU_DRV_H__
#define __NOUVEAU_DRV_H__

#define DRIVER_AUTHOR		"Stephane Marchesin"
#define DRIVER_EMAIL		"dri-devel@lists.sourceforge.net"

#define DRIVER_NAME		"nouveau"
#define DRIVER_DESC		"nVidia Riva/TNT/GeForce"
#define DRIVER_DATE		"20060213"

#define DRIVER_MAJOR		0
#define DRIVER_MINOR		0
#define DRIVER_PATCHLEVEL	12

#define NOUVEAU_FAMILY   0x0000FFFF
#define NOUVEAU_FLAGS    0xFFFF0000

#include "nouveau_drm.h"
#include "nouveau_reg.h"

struct mem_block {
	struct mem_block *next;
	struct mem_block *prev;
	uint64_t start;
	uint64_t size;
	struct drm_file *file_priv; /* NULL: free, -1: heap, other: real files */
	int flags;
	drm_local_map_t *map;
	drm_handle_t map_handle;
};

enum nouveau_flags {
	NV_NFORCE   =0x10000000,
	NV_NFORCE2  =0x20000000
};

#define NVOBJ_ENGINE_SW		0
#define NVOBJ_ENGINE_GR		1
#define NVOBJ_ENGINE_INT	0xdeadbeef

#define NVOBJ_FLAG_ALLOW_NO_REFS	(1 << 0)
#define NVOBJ_FLAG_ZERO_ALLOC		(1 << 1)
#define NVOBJ_FLAG_ZERO_FREE		(1 << 2)
#define NVOBJ_FLAG_FAKE			(1 << 3)
struct nouveau_gpuobj {
	struct list_head list;

	int im_channel;
	struct mem_block *im_pramin;
	struct mem_block *im_backing;
	int im_bound;

	uint32_t flags;
	int refcount;

	uint32_t engine;
	uint32_t class;

	void (*dtor)(struct drm_device *, struct nouveau_gpuobj *);
	void *priv;
};

struct nouveau_gpuobj_ref {
	struct list_head list;

	struct nouveau_gpuobj *gpuobj;
	uint32_t instance;

	int channel;
	int handle;
};

struct nouveau_channel
{
	struct drm_device *dev;
	int id;

	/* owner of this fifo */
	struct drm_file *file_priv;
	/* mapping of the fifo itself */
	drm_local_map_t *map;
	/* mapping of the regs controling the fifo */
	drm_local_map_t *regs;

	/* Fencing */
	uint32_t next_sequence;

	/* DMA push buffer */
	struct nouveau_gpuobj_ref *pushbuf;
	struct mem_block          *pushbuf_mem;
	uint32_t                   pushbuf_base;

	/* FIFO user control regs */
	uint32_t user, user_size;
	uint32_t put;
	uint32_t get;
	uint32_t ref_cnt;

	/* Notifier memory */
	struct mem_block *notifier_block;
	struct mem_block *notifier_heap;
	drm_local_map_t  *notifier_map;

	/* PFIFO context */
	struct nouveau_gpuobj_ref *ramfc;

	/* PGRAPH context */
	/* XXX may be merge 2 pointers as private data ??? */
	struct nouveau_gpuobj_ref *ramin_grctx;
	void *pgraph_ctx;

	/* NV50 VM */
	struct nouveau_gpuobj     *vm_pd;
	struct nouveau_gpuobj_ref *vm_gart_pt;
	struct nouveau_gpuobj_ref *vm_vram_pt;

	/* Objects */
	struct nouveau_gpuobj_ref *ramin; /* Private instmem */
	struct mem_block          *ramin_heap; /* Private PRAMIN heap */
	struct nouveau_gpuobj_ref *ramht; /* Hash table */
	struct list_head           ramht_refs; /* Objects referenced by RAMHT */
};

struct nouveau_drm_channel {
	struct nouveau_channel *chan;

	/* DMA state */
	int max, put, cur, free;
	int push_free;
	volatile uint32_t *pushbuf;

	/* Notifiers */
	uint32_t notify0_offset;

	/* Buffer moves */
	uint32_t m2mf_dma_source;
	uint32_t m2mf_dma_destin;
};

struct nouveau_config {
	struct {
		int location;
		int size;
	} cmdbuf;
};

struct nouveau_instmem_engine {
	void	*priv;

	int	(*init)(struct drm_device *dev);
	void	(*takedown)(struct drm_device *dev);

	int	(*populate)(struct drm_device *, struct nouveau_gpuobj *,
			    uint32_t *size);
	void	(*clear)(struct drm_device *, struct nouveau_gpuobj *);
	int	(*bind)(struct drm_device *, struct nouveau_gpuobj *);
	int	(*unbind)(struct drm_device *, struct nouveau_gpuobj *);
};

struct nouveau_mc_engine {
	int  (*init)(struct drm_device *dev);
	void (*takedown)(struct drm_device *dev);
};

struct nouveau_timer_engine {
	int      (*init)(struct drm_device *dev);
	void     (*takedown)(struct drm_device *dev);
	uint64_t (*read)(struct drm_device *dev);
};

struct nouveau_fb_engine {
	int  (*init)(struct drm_device *dev);
	void (*takedown)(struct drm_device *dev);
};

struct nouveau_fifo_engine {
	void *priv;

	int  channels;

	int  (*init)(struct drm_device *);
	void (*takedown)(struct drm_device *);

	int  (*channel_id)(struct drm_device *);

	int  (*create_context)(struct nouveau_channel *);
	void (*destroy_context)(struct nouveau_channel *);
	int  (*load_context)(struct nouveau_channel *);
	int  (*save_context)(struct nouveau_channel *);
};

struct nouveau_pgraph_engine {
	int  (*init)(struct drm_device *);
	void (*takedown)(struct drm_device *);

	int  (*create_context)(struct nouveau_channel *);
	void (*destroy_context)(struct nouveau_channel *);
	int  (*load_context)(struct nouveau_channel *);
	int  (*save_context)(struct nouveau_channel *);
};

struct nouveau_engine {
	struct nouveau_instmem_engine instmem;
	struct nouveau_mc_engine      mc;
	struct nouveau_timer_engine   timer;
	struct nouveau_fb_engine      fb;
	struct nouveau_pgraph_engine  graph;
	struct nouveau_fifo_engine    fifo;
};

#define NOUVEAU_MAX_CHANNEL_NR 128
struct drm_nouveau_private {
	enum {
		NOUVEAU_CARD_INIT_DOWN,
		NOUVEAU_CARD_INIT_DONE,
		NOUVEAU_CARD_INIT_FAILED
	} init_state;

	int ttm;

	/* the card type, takes NV_* as values */
	int card_type;
	/* exact chipset, derived from NV_PMC_BOOT_0 */
	int chipset;
	int flags;

	drm_local_map_t *mmio;
	drm_local_map_t *fb;
	drm_local_map_t *ramin; /* NV40 onwards */

	int fifo_alloc_count;
	struct nouveau_channel *fifos[NOUVEAU_MAX_CHANNEL_NR];

	struct nouveau_engine Engine;
	struct nouveau_drm_channel channel;

	/* RAMIN configuration, RAMFC, RAMHT and RAMRO offsets */
	struct nouveau_gpuobj *ramht;
	uint32_t ramin_rsvd_vram;
	uint32_t ramht_offset;
	uint32_t ramht_size;
	uint32_t ramht_bits;
	uint32_t ramfc_offset;
	uint32_t ramfc_size;
	uint32_t ramro_offset;
	uint32_t ramro_size;

	/* base physical adresses */
	uint64_t fb_phys;
	uint64_t fb_available_size;

	struct {
		enum {
			NOUVEAU_GART_NONE = 0,
			NOUVEAU_GART_AGP,
			NOUVEAU_GART_SGDMA
		} type;
		uint64_t aper_base;
		uint64_t aper_size;

		struct nouveau_gpuobj *sg_ctxdma;
		struct page *sg_dummy_page;
		dma_addr_t sg_dummy_bus;

		/* nottm hack */
		struct drm_ttm_backend *sg_be;
		unsigned long sg_handle;
	} gart_info;

	/* G8x global VRAM page table */
	struct nouveau_gpuobj *vm_vram_pt;

	/* the mtrr covering the FB */
	int fb_mtrr;

	struct mem_block *agp_heap;
	struct mem_block *fb_heap;
	struct mem_block *fb_nomap_heap;
	struct mem_block *ramin_heap;
	struct mem_block *pci_heap;

        /* context table pointed to be NV_PGRAPH_CHANNEL_CTX_TABLE (0x400780) */
        uint32_t ctx_table_size;
	struct nouveau_gpuobj_ref *ctx_table;

	struct nouveau_config config;

	struct list_head gpuobj_list;

	struct nouveau_suspend_resume {
		uint32_t fifo_mode;
		uint32_t graph_ctx_control;
		uint32_t graph_state;
		uint32_t *ramin_copy;
		uint64_t ramin_size;
	} susres;
	struct backlight_device *backlight;
};

#define NOUVEAU_CHECK_INITIALISED_WITH_RETURN do {         \
	struct drm_nouveau_private *nv = dev->dev_private; \
	if (nv->init_state != NOUVEAU_CARD_INIT_DONE) {    \
		DRM_ERROR("called without init\n");        \
		return -EINVAL;                            \
	}                                                  \
} while(0)

#define NOUVEAU_GET_USER_CHANNEL_WITH_RETURN(id,cl,ch) do {  \
	struct drm_nouveau_private *nv = dev->dev_private;   \
	if (!nouveau_fifo_owner(dev, (cl), (id))) {          \
		DRM_ERROR("pid %d doesn't own channel %d\n", \
			  DRM_CURRENTPID, (id));             \
		return -EPERM;                               \
	}                                                    \
	(ch) = nv->fifos[(id)];                              \
} while(0)

/* nouveau_state.c */
extern void nouveau_preclose(struct drm_device *dev, struct drm_file *);
extern int  nouveau_load(struct drm_device *, unsigned long flags);
extern int  nouveau_firstopen(struct drm_device *);
extern void nouveau_lastclose(struct drm_device *);
extern int  nouveau_unload(struct drm_device *);
extern int  nouveau_ioctl_getparam(struct drm_device *, void *data,
				   struct drm_file *);
extern int  nouveau_ioctl_setparam(struct drm_device *, void *data,
				   struct drm_file *);
extern void nouveau_wait_for_idle(struct drm_device *);
extern int  nouveau_card_init(struct drm_device *);
extern int  nouveau_ioctl_card_init(struct drm_device *, void *data,
				    struct drm_file *);
extern int  nouveau_ioctl_suspend(struct drm_device *, void *data,
				  struct drm_file *);
extern int  nouveau_ioctl_resume(struct drm_device *, void *data,
				 struct drm_file *);

/* nouveau_mem.c */
extern int  nouveau_mem_init_heap(struct mem_block **, uint64_t start,
				 uint64_t size);
extern struct mem_block *nouveau_mem_alloc_block(struct mem_block *,
						 uint64_t size, int align2,
						 struct drm_file *, int tail);
extern void nouveau_mem_takedown(struct mem_block **heap);
extern void nouveau_mem_free_block(struct mem_block *);
extern uint64_t nouveau_mem_fb_amount(struct drm_device *);
extern void nouveau_mem_release(struct drm_file *, struct mem_block *heap);
extern int  nouveau_ioctl_mem_alloc(struct drm_device *, void *data,
				    struct drm_file *);
extern int  nouveau_ioctl_mem_free(struct drm_device *, void *data,
				   struct drm_file *);
extern int  nouveau_ioctl_mem_tile(struct drm_device *, void *data,
				   struct drm_file *);
extern struct mem_block* nouveau_mem_alloc(struct drm_device *,
					   int alignment, uint64_t size,
					   int flags, struct drm_file *);
extern void nouveau_mem_free(struct drm_device *dev, struct mem_block*);
extern int  nouveau_mem_init(struct drm_device *);
extern int  nouveau_mem_init_ttm(struct drm_device *);
extern void nouveau_mem_close(struct drm_device *);

/* nouveau_notifier.c */
extern int  nouveau_notifier_init_channel(struct nouveau_channel *);
extern void nouveau_notifier_takedown_channel(struct nouveau_channel *);
extern int  nouveau_notifier_alloc(struct nouveau_channel *, uint32_t handle,
				   int cout, uint32_t *offset);
extern int  nouveau_ioctl_notifier_alloc(struct drm_device *, void *data,
					 struct drm_file *);
extern int  nouveau_ioctl_notifier_free(struct drm_device *, void *data,
					struct drm_file *);

/* nouveau_fifo.c */
extern int  nouveau_fifo_init(struct drm_device *);
extern int  nouveau_fifo_ctx_size(struct drm_device *);
extern void nouveau_fifo_cleanup(struct drm_device *, struct drm_file *);
extern int  nouveau_fifo_owner(struct drm_device *, struct drm_file *,
			       int channel);
extern int  nouveau_fifo_alloc(struct drm_device *dev,
			       struct nouveau_channel **chan,
			       struct drm_file *file_priv,
			       struct mem_block *pushbuf,
			       uint32_t fb_ctxdma, uint32_t tt_ctxdma);
extern void nouveau_fifo_free(struct nouveau_channel *);
extern int  nouveau_channel_idle(struct nouveau_channel *chan);

/* nouveau_object.c */
extern int  nouveau_gpuobj_early_init(struct drm_device *);
extern int  nouveau_gpuobj_init(struct drm_device *);
extern void nouveau_gpuobj_takedown(struct drm_device *);
extern void nouveau_gpuobj_late_takedown(struct drm_device *);
extern int nouveau_gpuobj_channel_init(struct nouveau_channel *,
				       uint32_t vram_h, uint32_t tt_h);
extern void nouveau_gpuobj_channel_takedown(struct nouveau_channel *);
extern int nouveau_gpuobj_new(struct drm_device *, struct nouveau_channel *,
			      int size, int align, uint32_t flags,
			      struct nouveau_gpuobj **);
extern int nouveau_gpuobj_del(struct drm_device *, struct nouveau_gpuobj **);
extern int nouveau_gpuobj_ref_add(struct drm_device *, struct nouveau_channel *,
				  uint32_t handle, struct nouveau_gpuobj *,
				  struct nouveau_gpuobj_ref **);
extern int nouveau_gpuobj_ref_del(struct drm_device *,
				  struct nouveau_gpuobj_ref **);
extern int nouveau_gpuobj_ref_find(struct nouveau_channel *, uint32_t handle,
				   struct nouveau_gpuobj_ref **ref_ret);
extern int nouveau_gpuobj_new_ref(struct drm_device *,
				  struct nouveau_channel *alloc_chan,
				  struct nouveau_channel *ref_chan,
				  uint32_t handle, int size, int align,
				  uint32_t flags, struct nouveau_gpuobj_ref **);
extern int nouveau_gpuobj_new_fake(struct drm_device *,
				   uint32_t p_offset, uint32_t b_offset,
				   uint32_t size, uint32_t flags,
				   struct nouveau_gpuobj **,
				   struct nouveau_gpuobj_ref**);
extern int nouveau_gpuobj_dma_new(struct nouveau_channel *, int class,
				  uint64_t offset, uint64_t size, int access,
				  int target, struct nouveau_gpuobj **);
extern int nouveau_gpuobj_gart_dma_new(struct nouveau_channel *,
				       uint64_t offset, uint64_t size,
				       int access, struct nouveau_gpuobj **,
				       uint32_t *o_ret);
extern int nouveau_gpuobj_gr_new(struct nouveau_channel *, int class,
				 struct nouveau_gpuobj **);
extern int nouveau_ioctl_grobj_alloc(struct drm_device *, void *data,
				     struct drm_file *);
extern int nouveau_ioctl_gpuobj_free(struct drm_device *, void *data,
				     struct drm_file *);

/* nouveau_irq.c */
extern irqreturn_t nouveau_irq_handler(DRM_IRQ_ARGS);
extern void        nouveau_irq_preinstall(struct drm_device *);
extern int         nouveau_irq_postinstall(struct drm_device *);
extern void        nouveau_irq_uninstall(struct drm_device *);

/* nouveau_sgdma.c */
extern int nouveau_sgdma_init(struct drm_device *);
extern void nouveau_sgdma_takedown(struct drm_device *);
extern int nouveau_sgdma_get_page(struct drm_device *, uint32_t offset,
				  uint32_t *page);
extern struct drm_ttm_backend *nouveau_sgdma_init_ttm(struct drm_device *);
extern int nouveau_sgdma_nottm_hack_init(struct drm_device *);
extern void nouveau_sgdma_nottm_hack_takedown(struct drm_device *);

/* nouveau_dma.c */
extern int  nouveau_dma_channel_init(struct drm_device *);
extern void nouveau_dma_channel_takedown(struct drm_device *);
extern int  nouveau_dma_wait(struct drm_device *, int size);

/* nouveau_backlight.c */
extern int nouveau_backlight_init(struct drm_device *);
extern void nouveau_backlight_exit(struct drm_device *);

/* nv04_fb.c */
extern int  nv04_fb_init(struct drm_device *);
extern void nv04_fb_takedown(struct drm_device *);

/* nv10_fb.c */
extern int  nv10_fb_init(struct drm_device *);
extern void nv10_fb_takedown(struct drm_device *);

/* nv40_fb.c */
extern int  nv40_fb_init(struct drm_device *);
extern void nv40_fb_takedown(struct drm_device *);

/* nv04_fifo.c */
extern int  nv04_fifo_channel_id(struct drm_device *);
extern int  nv04_fifo_create_context(struct nouveau_channel *);
extern void nv04_fifo_destroy_context(struct nouveau_channel *);
extern int  nv04_fifo_load_context(struct nouveau_channel *);
extern int  nv04_fifo_save_context(struct nouveau_channel *);

/* nv10_fifo.c */
extern int  nv10_fifo_channel_id(struct drm_device *);
extern int  nv10_fifo_create_context(struct nouveau_channel *);
extern void nv10_fifo_destroy_context(struct nouveau_channel *);
extern int  nv10_fifo_load_context(struct nouveau_channel *);
extern int  nv10_fifo_save_context(struct nouveau_channel *);

/* nv40_fifo.c */
extern int  nv40_fifo_init(struct drm_device *);
extern int  nv40_fifo_create_context(struct nouveau_channel *);
extern void nv40_fifo_destroy_context(struct nouveau_channel *);
extern int  nv40_fifo_load_context(struct nouveau_channel *);
extern int  nv40_fifo_save_context(struct nouveau_channel *);

/* nv50_fifo.c */
extern int  nv50_fifo_init(struct drm_device *);
extern void nv50_fifo_takedown(struct drm_device *);
extern int  nv50_fifo_channel_id(struct drm_device *);
extern int  nv50_fifo_create_context(struct nouveau_channel *);
extern void nv50_fifo_destroy_context(struct nouveau_channel *);
extern int  nv50_fifo_load_context(struct nouveau_channel *);
extern int  nv50_fifo_save_context(struct nouveau_channel *);

/* nv04_graph.c */
extern void nouveau_nv04_context_switch(struct drm_device *);
extern int  nv04_graph_init(struct drm_device *);
extern void nv04_graph_takedown(struct drm_device *);
extern int  nv04_graph_create_context(struct nouveau_channel *);
extern void nv04_graph_destroy_context(struct nouveau_channel *);
extern int  nv04_graph_load_context(struct nouveau_channel *);
extern int  nv04_graph_save_context(struct nouveau_channel *);

/* nv10_graph.c */
extern void nouveau_nv10_context_switch(struct drm_device *);
extern int  nv10_graph_init(struct drm_device *);
extern void nv10_graph_takedown(struct drm_device *);
extern int  nv10_graph_create_context(struct nouveau_channel *);
extern void nv10_graph_destroy_context(struct nouveau_channel *);
extern int  nv10_graph_load_context(struct nouveau_channel *);
extern int  nv10_graph_save_context(struct nouveau_channel *);

/* nv20_graph.c */
extern int  nv20_graph_create_context(struct nouveau_channel *);
extern void nv20_graph_destroy_context(struct nouveau_channel *);
extern int  nv20_graph_load_context(struct nouveau_channel *);
extern int  nv20_graph_save_context(struct nouveau_channel *);
extern int  nv20_graph_init(struct drm_device *);
extern void nv20_graph_takedown(struct drm_device *);
extern int  nv30_graph_init(struct drm_device *);

/* nv40_graph.c */
extern int  nv40_graph_init(struct drm_device *);
extern void nv40_graph_takedown(struct drm_device *);
extern int  nv40_graph_create_context(struct nouveau_channel *);
extern void nv40_graph_destroy_context(struct nouveau_channel *);
extern int  nv40_graph_load_context(struct nouveau_channel *);
extern int  nv40_graph_save_context(struct nouveau_channel *);

/* nv50_graph.c */
extern int  nv50_graph_init(struct drm_device *);
extern void nv50_graph_takedown(struct drm_device *);
extern int  nv50_graph_create_context(struct nouveau_channel *);
extern void nv50_graph_destroy_context(struct nouveau_channel *);
extern int  nv50_graph_load_context(struct nouveau_channel *);
extern int  nv50_graph_save_context(struct nouveau_channel *);

/* nv04_instmem.c */
extern int  nv04_instmem_init(struct drm_device *);
extern void nv04_instmem_takedown(struct drm_device *);
extern int  nv04_instmem_populate(struct drm_device *, struct nouveau_gpuobj *,
				  uint32_t *size);
extern void nv04_instmem_clear(struct drm_device *, struct nouveau_gpuobj *);
extern int  nv04_instmem_bind(struct drm_device *, struct nouveau_gpuobj *);
extern int  nv04_instmem_unbind(struct drm_device *, struct nouveau_gpuobj *);

/* nv50_instmem.c */
extern int  nv50_instmem_init(struct drm_device *);
extern void nv50_instmem_takedown(struct drm_device *);
extern int  nv50_instmem_populate(struct drm_device *, struct nouveau_gpuobj *,
				  uint32_t *size);
extern void nv50_instmem_clear(struct drm_device *, struct nouveau_gpuobj *);
extern int  nv50_instmem_bind(struct drm_device *, struct nouveau_gpuobj *);
extern int  nv50_instmem_unbind(struct drm_device *, struct nouveau_gpuobj *);

/* nv04_mc.c */
extern int  nv04_mc_init(struct drm_device *);
extern void nv04_mc_takedown(struct drm_device *);

/* nv40_mc.c */
extern int  nv40_mc_init(struct drm_device *);
extern void nv40_mc_takedown(struct drm_device *);

/* nv50_mc.c */
extern int  nv50_mc_init(struct drm_device *);
extern void nv50_mc_takedown(struct drm_device *);

/* nv04_timer.c */
extern int  nv04_timer_init(struct drm_device *);
extern uint64_t nv04_timer_read(struct drm_device *);
extern void nv04_timer_takedown(struct drm_device *);

extern long nouveau_compat_ioctl(struct file *file, unsigned int cmd,
				 unsigned long arg);

/* nouveau_buffer.c */
extern struct drm_bo_driver nouveau_bo_driver;

/* nouveau_fence.c */
extern struct drm_fence_driver nouveau_fence_driver;
extern void nouveau_fence_handler(struct drm_device *dev, int channel);

#if defined(__powerpc__)
#define NV_READ(reg)        in_be32((void __iomem *)(dev_priv->mmio)->handle + (reg) )
#define NV_WRITE(reg,val)   out_be32((void __iomem *)(dev_priv->mmio)->handle + (reg) , (val) )
#else
#define NV_READ(reg)        DRM_READ32(  dev_priv->mmio, (reg) )
#define NV_WRITE(reg,val)   DRM_WRITE32( dev_priv->mmio, (reg), (val) )
#endif

/* PRAMIN access */
#if defined(__powerpc__)
#define NV_RI32(o) in_be32((void __iomem *)(dev_priv->ramin)->handle+(o))
#define NV_WI32(o,v) out_be32((void __iomem*)(dev_priv->ramin)->handle+(o), (v))
#else
#define NV_RI32(o) DRM_READ32(dev_priv->ramin, (o))
#define NV_WI32(o,v) DRM_WRITE32(dev_priv->ramin, (o), (v))
#endif

#define INSTANCE_RD(o,i) NV_RI32((o)->im_pramin->start + ((i)<<2))
#define INSTANCE_WR(o,i,v) NV_WI32((o)->im_pramin->start + ((i)<<2), (v))

#endif /* __NOUVEAU_DRV_H__ */
