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

#ifndef __NOUVEAU_DRM_H__
#define __NOUVEAU_DRM_H__

#define NOUVEAU_DRM_HEADER_PATCHLEVEL 11

struct drm_nouveau_channel_alloc {
	uint32_t     fb_ctxdma_handle;
	uint32_t     tt_ctxdma_handle;

	int          channel;
	uint32_t     put_base;
	/* FIFO control regs */
	drm_handle_t ctrl;
	int          ctrl_size;
	/* DMA command buffer */
	drm_handle_t cmdbuf;
	int          cmdbuf_size;
	/* Notifier memory */
	drm_handle_t notifier;
	int          notifier_size;
};

struct drm_nouveau_channel_free {
	int channel;
};

struct drm_nouveau_grobj_alloc {
	int      channel;
	uint32_t handle;
	int      class;
};

#define NOUVEAU_MEM_ACCESS_RO	1
#define NOUVEAU_MEM_ACCESS_WO	2
#define NOUVEAU_MEM_ACCESS_RW	3
struct drm_nouveau_notifierobj_alloc {
	int      channel;
	uint32_t handle;
	int      count;

	uint32_t offset;
};

struct drm_nouveau_gpuobj_free {
	int      channel;
	uint32_t handle;
};

/* This is needed to avoid a race condition.
 * Otherwise you may be writing in the fetch area.
 * Is this large enough, as it's only 32 bytes, and the maximum fetch size is 256 bytes?
 */
#define NOUVEAU_DMA_SKIPS 8

#define NOUVEAU_MEM_FB			0x00000001
#define NOUVEAU_MEM_AGP			0x00000002
#define NOUVEAU_MEM_FB_ACCEPTABLE	0x00000004
#define NOUVEAU_MEM_AGP_ACCEPTABLE	0x00000008
#define NOUVEAU_MEM_PCI			0x00000010
#define NOUVEAU_MEM_PCI_ACCEPTABLE	0x00000020
#define NOUVEAU_MEM_PINNED		0x00000040
#define NOUVEAU_MEM_USER_BACKED		0x00000080
#define NOUVEAU_MEM_MAPPED		0x00000100
#define NOUVEAU_MEM_TILE		0x00000200
#define NOUVEAU_MEM_TILE_ZETA		0x00000400
#define NOUVEAU_MEM_INSTANCE		0x01000000 /* internal */
#define NOUVEAU_MEM_NOTIFIER            0x02000000 /* internal */
#define NOUVEAU_MEM_NOVM		0x04000000 /* internal */
#define NOUVEAU_MEM_USER		0x08000000 /* internal */
#define NOUVEAU_MEM_INTERNAL (NOUVEAU_MEM_INSTANCE | \
			      NOUVEAU_MEM_NOTIFIER | \
			      NOUVEAU_MEM_NOVM | \
			      NOUVEAU_MEM_USER)

struct drm_nouveau_mem_alloc {
	int flags;
	int alignment;
	uint64_t size;	// in bytes
	uint64_t offset;
	drm_handle_t map_handle;
};

struct drm_nouveau_mem_free {
	uint64_t offset;
	int flags;
};

struct drm_nouveau_mem_tile {
	uint64_t offset;
	uint64_t delta;
	uint64_t size;
	int flags;
};

/* FIXME : maybe unify {GET,SET}PARAMs */
#define NOUVEAU_GETPARAM_PCI_VENDOR      3
#define NOUVEAU_GETPARAM_PCI_DEVICE      4
#define NOUVEAU_GETPARAM_BUS_TYPE        5
#define NOUVEAU_GETPARAM_FB_PHYSICAL     6
#define NOUVEAU_GETPARAM_AGP_PHYSICAL    7
#define NOUVEAU_GETPARAM_FB_SIZE         8
#define NOUVEAU_GETPARAM_AGP_SIZE        9
#define NOUVEAU_GETPARAM_PCI_PHYSICAL    10
#define NOUVEAU_GETPARAM_CHIPSET_ID      11
struct drm_nouveau_getparam {
	uint64_t param;
	uint64_t value;
};

#define NOUVEAU_SETPARAM_CMDBUF_LOCATION 1
#define NOUVEAU_SETPARAM_CMDBUF_SIZE     2
struct drm_nouveau_setparam {
	uint64_t param;
	uint64_t value;
};

enum nouveau_card_type {
	NV_UNKNOWN =0,
	NV_04      =4,
	NV_05      =5,
	NV_10      =10,
	NV_11      =11,
	NV_17      =17,
	NV_20      =20,
	NV_30      =30,
	NV_40      =40,
	NV_44      =44,
	NV_50      =50,
	NV_LAST    =0xffff,
};

enum nouveau_bus_type {
	NV_AGP     =0,
	NV_PCI     =1,
	NV_PCIE    =2,
};

#define NOUVEAU_MAX_SAREA_CLIPRECTS 16

struct drm_nouveau_sarea {
	/* the cliprects */
	struct drm_clip_rect boxes[NOUVEAU_MAX_SAREA_CLIPRECTS];
	unsigned int nbox;
};

#define DRM_NOUVEAU_CARD_INIT          0x00
#define DRM_NOUVEAU_GETPARAM           0x01
#define DRM_NOUVEAU_SETPARAM           0x02
#define DRM_NOUVEAU_CHANNEL_ALLOC      0x03
#define DRM_NOUVEAU_CHANNEL_FREE       0x04
#define DRM_NOUVEAU_GROBJ_ALLOC        0x05
#define DRM_NOUVEAU_NOTIFIEROBJ_ALLOC  0x06
#define DRM_NOUVEAU_GPUOBJ_FREE        0x07
#define DRM_NOUVEAU_MEM_ALLOC          0x08
#define DRM_NOUVEAU_MEM_FREE           0x09
#define DRM_NOUVEAU_MEM_TILE           0x0a

#endif /* __NOUVEAU_DRM_H__ */
