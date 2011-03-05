/*
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef _I915_DRM_H_
#define _I915_DRM_H_

/* Please note that modifications to all structs defined here are
 * subject to backwards-compatibility constraints.
 */

#include "drm.h"

/* Each region is a minimum of 16k, and there are at most 255 of them.
 */
#define I915_NR_TEX_REGIONS 255	/* table size 2k - maximum due to use
				 * of chars for next/prev indices */
#define I915_LOG_MIN_TEX_REGION_SIZE 14

typedef struct _drm_i915_init {
	enum {
		I915_INIT_DMA = 0x01,
		I915_CLEANUP_DMA = 0x02,
		I915_RESUME_DMA = 0x03,

		/* Since this struct isn't versioned, just used a new
		 * 'func' code to indicate the presence of dri2 sarea
		 * info. */
		I915_INIT_DMA2 = 0x04
	} func;
	unsigned int mmio_offset;
	int sarea_priv_offset;
	unsigned int ring_start;
	unsigned int ring_end;
	unsigned int ring_size;
	unsigned int front_offset;
	unsigned int back_offset;
	unsigned int depth_offset;
	unsigned int w;
	unsigned int h;
	unsigned int pitch;
	unsigned int pitch_bits;
	unsigned int back_pitch;
	unsigned int depth_pitch;
	unsigned int cpp;
	unsigned int chipset;
	unsigned int sarea_handle;
} drm_i915_init_t;

typedef struct drm_i915_sarea {
	struct drm_tex_region texList[I915_NR_TEX_REGIONS + 1];
	int last_upload;	/* last time texture was uploaded */
	int last_enqueue;	/* last time a buffer was enqueued */
	int last_dispatch;	/* age of the most recently dispatched buffer */
	int ctxOwner;		/* last context to upload state */
	int texAge;
	int pf_enabled;		/* is pageflipping allowed? */
	int pf_active;
	int pf_current_page;	/* which buffer is being displayed? */
	int perf_boxes;		/* performance boxes to be displayed */
	int width, height;      /* screen size in pixels */

	drm_handle_t front_handle;
	int front_offset;
	int front_size;

	drm_handle_t back_handle;
	int back_offset;
	int back_size;

	drm_handle_t depth_handle;
	int depth_offset;
	int depth_size;

	drm_handle_t tex_handle;
	int tex_offset;
	int tex_size;
	int log_tex_granularity;
	int pitch;
	int rotation;           /* 0, 90, 180 or 270 */
	int rotated_offset;
	int rotated_size;
	int rotated_pitch;
	int virtualX, virtualY;

	unsigned int front_tiled;
	unsigned int back_tiled;
	unsigned int depth_tiled;
	unsigned int rotated_tiled;
	unsigned int rotated2_tiled;

	/* compat defines for the period of time when pipeA_* got renamed
	 * to planeA_*.  They mean pipe, really.
	 */
#define planeA_x pipeA_x
#define planeA_y pipeA_y
#define planeA_w pipeA_w
#define planeA_h pipeA_h
#define planeB_x pipeB_x
#define planeB_y pipeB_y
#define planeB_w pipeB_w
#define planeB_h pipeB_h
	int pipeA_x;
	int pipeA_y;
	int pipeA_w;
	int pipeA_h;
	int pipeB_x;
	int pipeB_y;
	int pipeB_w;
	int pipeB_h;

	/* Triple buffering */
	drm_handle_t third_handle;
	int third_offset;
	int third_size;
	unsigned int third_tiled;

	/* buffer object handles for the static buffers.  May change
	 * over the lifetime of the client, though it doesn't in our current
	 * implementation.
	 */
	unsigned int front_bo_handle;
	unsigned int back_bo_handle;
	unsigned int third_bo_handle;
	unsigned int depth_bo_handle;
} drm_i915_sarea_t;

/* Driver specific fence types and classes.
 */

/* The only fence class we support */
#define DRM_I915_FENCE_CLASS_ACCEL 0
/* Fence type that guarantees read-write flush */
#define DRM_I915_FENCE_TYPE_RW 2
/* MI_FLUSH programmed just before the fence */
#define DRM_I915_FENCE_FLAG_FLUSHED 0x01000000

/* Flags for perf_boxes
 */
#define I915_BOX_RING_EMPTY    0x1
#define I915_BOX_FLIP          0x2
#define I915_BOX_WAIT          0x4
#define I915_BOX_TEXTURE_LOAD  0x8
#define I915_BOX_LOST_CONTEXT  0x10

/* I915 specific ioctls
 * The device specific ioctl range is 0x40 to 0x79.
 */
#define DRM_I915_INIT		0x00
#define DRM_I915_FLUSH		0x01
#define DRM_I915_FLIP		0x02
#define DRM_I915_BATCHBUFFER	0x03
#define DRM_I915_IRQ_EMIT	0x04
#define DRM_I915_IRQ_WAIT	0x05
#define DRM_I915_GETPARAM	0x06
#define DRM_I915_SETPARAM	0x07
#define DRM_I915_ALLOC		0x08
#define DRM_I915_FREE		0x09
#define DRM_I915_INIT_HEAP	0x0a
#define DRM_I915_CMDBUFFER	0x0b
#define DRM_I915_DESTROY_HEAP	0x0c
#define DRM_I915_SET_VBLANK_PIPE	0x0d
#define DRM_I915_GET_VBLANK_PIPE	0x0e
#define DRM_I915_VBLANK_SWAP	0x0f
#define DRM_I915_MMIO		0x10
#define DRM_I915_HWS_ADDR	0x11
#define DRM_I915_EXECBUFFER	0x12
#define DRM_I915_GEM_INIT	0x13
#define DRM_I915_GEM_EXECBUFFER	0x14
#define DRM_I915_GEM_PIN	0x15
#define DRM_I915_GEM_UNPIN	0x16
#define DRM_I915_GEM_BUSY	0x17
#define DRM_I915_GEM_THROTTLE	0x18
#define DRM_I915_GEM_ENTERVT	0x19
#define DRM_I915_GEM_LEAVEVT	0x1a
#define DRM_I915_GEM_CREATE	0x1b
#define DRM_I915_GEM_PREAD	0x1c
#define DRM_I915_GEM_PWRITE	0x1d
#define DRM_I915_GEM_MMAP	0x1e
#define DRM_I915_GEM_SET_DOMAIN	0x1f
#define DRM_I915_GEM_SW_FINISH	0x20
#define DRM_I915_GEM_SET_TILING	0x21
#define DRM_I915_GEM_GET_TILING	0x22
#define DRM_I915_GEM_GET_APERTURE 0x23
#define DRM_I915_GEM_MMAP_GTT	0x24
#define DRM_I915_GET_PIPE_FROM_CRTC_ID	0x25

#define DRM_IOCTL_I915_INIT		DRM_IOW( DRM_COMMAND_BASE + DRM_I915_INIT, drm_i915_init_t)
#define DRM_IOCTL_I915_FLUSH		DRM_IO ( DRM_COMMAND_BASE + DRM_I915_FLUSH)
#define DRM_IOCTL_I915_FLIP		DRM_IOW( DRM_COMMAND_BASE + DRM_I915_FLIP, drm_i915_flip_t)
#define DRM_IOCTL_I915_BATCHBUFFER	DRM_IOW( DRM_COMMAND_BASE + DRM_I915_BATCHBUFFER, drm_i915_batchbuffer_t)
#define DRM_IOCTL_I915_IRQ_EMIT         DRM_IOWR(DRM_COMMAND_BASE + DRM_I915_IRQ_EMIT, drm_i915_irq_emit_t)
#define DRM_IOCTL_I915_IRQ_WAIT         DRM_IOW( DRM_COMMAND_BASE + DRM_I915_IRQ_WAIT, drm_i915_irq_wait_t)
#define DRM_IOCTL_I915_GETPARAM         DRM_IOWR(DRM_COMMAND_BASE + DRM_I915_GETPARAM, drm_i915_getparam_t)
#define DRM_IOCTL_I915_SETPARAM         DRM_IOW( DRM_COMMAND_BASE + DRM_I915_SETPARAM, drm_i915_setparam_t)
#define DRM_IOCTL_I915_ALLOC            DRM_IOWR(DRM_COMMAND_BASE + DRM_I915_ALLOC, drm_i915_mem_alloc_t)
#define DRM_IOCTL_I915_FREE             DRM_IOW( DRM_COMMAND_BASE + DRM_I915_FREE, drm_i915_mem_free_t)
#define DRM_IOCTL_I915_INIT_HEAP        DRM_IOW( DRM_COMMAND_BASE + DRM_I915_INIT_HEAP, drm_i915_mem_init_heap_t)
#define DRM_IOCTL_I915_CMDBUFFER	DRM_IOW( DRM_COMMAND_BASE + DRM_I915_CMDBUFFER, drm_i915_cmdbuffer_t)
#define DRM_IOCTL_I915_DESTROY_HEAP	DRM_IOW( DRM_COMMAND_BASE + DRM_I915_DESTROY_HEAP, drm_i915_mem_destroy_heap_t)
#define DRM_IOCTL_I915_SET_VBLANK_PIPE	DRM_IOW( DRM_COMMAND_BASE + DRM_I915_SET_VBLANK_PIPE, drm_i915_vblank_pipe_t)
#define DRM_IOCTL_I915_GET_VBLANK_PIPE	DRM_IOR( DRM_COMMAND_BASE + DRM_I915_GET_VBLANK_PIPE, drm_i915_vblank_pipe_t)
#define DRM_IOCTL_I915_VBLANK_SWAP	DRM_IOWR(DRM_COMMAND_BASE + DRM_I915_VBLANK_SWAP, drm_i915_vblank_swap_t)
#define DRM_IOCTL_I915_MMIO             DRM_IOWR(DRM_COMMAND_BASE + DRM_I915_MMIO, drm_i915_mmio)
#define DRM_IOCTL_I915_EXECBUFFER	DRM_IOWR(DRM_COMMAND_BASE + DRM_I915_EXECBUFFER, struct drm_i915_execbuffer)
#define DRM_IOCTL_I915_GEM_INIT		DRM_IOW(DRM_COMMAND_BASE + DRM_I915_GEM_INIT, struct drm_i915_gem_init)
#define DRM_IOCTL_I915_GEM_EXECBUFFER	DRM_IOW(DRM_COMMAND_BASE + DRM_I915_GEM_EXECBUFFER, struct drm_i915_gem_execbuffer)
#define DRM_IOCTL_I915_GEM_PIN		DRM_IOWR(DRM_COMMAND_BASE + DRM_I915_GEM_PIN, struct drm_i915_gem_pin)
#define DRM_IOCTL_I915_GEM_UNPIN	DRM_IOW(DRM_COMMAND_BASE + DRM_I915_GEM_UNPIN, struct drm_i915_gem_unpin)
#define DRM_IOCTL_I915_GEM_BUSY		DRM_IOWR(DRM_COMMAND_BASE + DRM_I915_GEM_BUSY, struct drm_i915_gem_busy)
#define DRM_IOCTL_I915_GEM_THROTTLE	DRM_IO ( DRM_COMMAND_BASE + DRM_I915_GEM_THROTTLE)
#define DRM_IOCTL_I915_GEM_ENTERVT	DRM_IO(DRM_COMMAND_BASE + DRM_I915_GEM_ENTERVT)
#define DRM_IOCTL_I915_GEM_LEAVEVT	DRM_IO(DRM_COMMAND_BASE + DRM_I915_GEM_LEAVEVT)
#define DRM_IOCTL_I915_GEM_CREATE	DRM_IOWR(DRM_COMMAND_BASE + DRM_I915_GEM_CREATE, struct drm_i915_gem_create)
#define DRM_IOCTL_I915_GEM_PREAD	DRM_IOW (DRM_COMMAND_BASE + DRM_I915_GEM_PREAD, struct drm_i915_gem_pread)
#define DRM_IOCTL_I915_GEM_PWRITE	DRM_IOW (DRM_COMMAND_BASE + DRM_I915_GEM_PWRITE, struct drm_i915_gem_pwrite)
#define DRM_IOCTL_I915_GEM_MMAP		DRM_IOWR(DRM_COMMAND_BASE + DRM_I915_GEM_MMAP, struct drm_i915_gem_mmap)
#define DRM_IOCTL_I915_GEM_MMAP_GTT	DRM_IOWR(DRM_COMMAND_BASE + DRM_I915_GEM_MMAP_GTT, struct drm_i915_gem_mmap_gtt)
#define DRM_IOCTL_I915_GEM_SET_DOMAIN	DRM_IOW (DRM_COMMAND_BASE + DRM_I915_GEM_SET_DOMAIN, struct drm_i915_gem_set_domain)
#define DRM_IOCTL_I915_GEM_SW_FINISH	DRM_IOW (DRM_COMMAND_BASE + DRM_I915_GEM_SW_FINISH, struct drm_i915_gem_sw_finish)
#define DRM_IOCTL_I915_GEM_SET_TILING	DRM_IOWR (DRM_COMMAND_BASE + DRM_I915_GEM_SET_TILING, struct drm_i915_gem_set_tiling)
#define DRM_IOCTL_I915_GEM_GET_TILING	DRM_IOWR (DRM_COMMAND_BASE + DRM_I915_GEM_GET_TILING, struct drm_i915_gem_get_tiling)
#define DRM_IOCTL_I915_GEM_GET_APERTURE	DRM_IOR  (DRM_COMMAND_BASE + DRM_I915_GEM_GET_APERTURE, struct drm_i915_gem_get_aperture)
#define DRM_IOCTL_I915_GET_PIPE_FROM_CRTC_ID DRM_IOWR(DRM_COMMAND_BASE + DRM_I915_GET_PIPE_FROM_CRTC_ID, struct drm_i915_get_pipe_from_crtc_id)

/* Asynchronous page flipping:
 */
typedef struct drm_i915_flip {
	/*
	 * This is really talking about planes, and we could rename it
	 * except for the fact that some of the duplicated i915_drm.h files
	 * out there check for HAVE_I915_FLIP and so might pick up this
	 * version.
	 */
	/* XXXMRG: make this unsigned? */
	int pipes;
} drm_i915_flip_t;

/* Allow drivers to submit batchbuffers directly to hardware, relying
 * on the security mechanisms provided by hardware.
 */
typedef struct drm_i915_batchbuffer {
	int start;		/* agp offset */
	int used;		/* nr bytes in use */
	int DR1;		/* hw flags for GFX_OP_DRAWRECT_INFO */
	int DR4;		/* window origin for GFX_OP_DRAWRECT_INFO */
	int num_cliprects;	/* mulitpass with multiple cliprects? */
	struct drm_clip_rect __user *cliprects;	/* pointer to userspace cliprects */
} drm_i915_batchbuffer_t;

/* As above, but pass a pointer to userspace buffer which can be
 * validated by the kernel prior to sending to hardware.
 */
typedef struct _drm_i915_cmdbuffer {
	char __user *buf;	/* pointer to userspace command buffer */
	int sz;			/* nr bytes in buf */
	int DR1;		/* hw flags for GFX_OP_DRAWRECT_INFO */
	int DR4;		/* window origin for GFX_OP_DRAWRECT_INFO */
	int num_cliprects;	/* mulitpass with multiple cliprects? */
	struct drm_clip_rect __user *cliprects;	/* pointer to userspace cliprects */
} drm_i915_cmdbuffer_t;

/* Userspace can request & wait on irq's:
 */
typedef struct drm_i915_irq_emit {
	int __user *irq_seq;
} drm_i915_irq_emit_t;

typedef struct drm_i915_irq_wait {
	int irq_seq;
} drm_i915_irq_wait_t;

/* Ioctl to query kernel params:
 */
#define I915_PARAM_IRQ_ACTIVE            1
#define I915_PARAM_ALLOW_BATCHBUFFER     2
#define I915_PARAM_LAST_DISPATCH         3
#define I915_PARAM_CHIPSET_ID            4
#define I915_PARAM_HAS_GEM               5
#define I915_PARAM_NUM_FENCES_AVAIL      6

typedef struct drm_i915_getparam {
	int param;
	int __user *value;
} drm_i915_getparam_t;

/* Ioctl to set kernel params:
 */
#define I915_SETPARAM_USE_MI_BATCHBUFFER_START            1
#define I915_SETPARAM_TEX_LRU_LOG_GRANULARITY             2
#define I915_SETPARAM_ALLOW_BATCHBUFFER                   3
#define I915_SETPARAM_NUM_USED_FENCES                     4

typedef struct drm_i915_setparam {
	int param;
	int value;
} drm_i915_setparam_t;

/* A memory manager for regions of shared memory:
 */
#define I915_MEM_REGION_AGP 1

typedef struct drm_i915_mem_alloc {
	int region;
	int alignment;
	int size;
	int __user *region_offset;	/* offset from start of fb or agp */
} drm_i915_mem_alloc_t;

typedef struct drm_i915_mem_free {
	int region;
	int region_offset;
} drm_i915_mem_free_t;

typedef struct drm_i915_mem_init_heap {
	int region;
	int size;
	int start;
} drm_i915_mem_init_heap_t;

/* Allow memory manager to be torn down and re-initialized (eg on
 * rotate):
 */
typedef struct drm_i915_mem_destroy_heap {
	int region;
} drm_i915_mem_destroy_heap_t;

/* Allow X server to configure which pipes to monitor for vblank signals
 */
#define	DRM_I915_VBLANK_PIPE_A	1
#define	DRM_I915_VBLANK_PIPE_B	2

typedef struct drm_i915_vblank_pipe {
	int pipe;
} drm_i915_vblank_pipe_t;

/* Schedule buffer swap at given vertical blank:
 */
typedef struct drm_i915_vblank_swap {
	drm_drawable_t drawable;
	enum drm_vblank_seq_type seqtype;
	unsigned int sequence;
} drm_i915_vblank_swap_t;

#define I915_MMIO_READ	0
#define I915_MMIO_WRITE 1

#define I915_MMIO_MAY_READ	0x1
#define I915_MMIO_MAY_WRITE	0x2

#define MMIO_REGS_IA_PRIMATIVES_COUNT		0
#define MMIO_REGS_IA_VERTICES_COUNT		1
#define MMIO_REGS_VS_INVOCATION_COUNT		2
#define MMIO_REGS_GS_PRIMITIVES_COUNT		3
#define MMIO_REGS_GS_INVOCATION_COUNT		4
#define MMIO_REGS_CL_PRIMITIVES_COUNT		5
#define MMIO_REGS_CL_INVOCATION_COUNT		6
#define MMIO_REGS_PS_INVOCATION_COUNT		7
#define MMIO_REGS_PS_DEPTH_COUNT		8

typedef struct drm_i915_mmio_entry {
	unsigned int flag;
	unsigned int offset;
	unsigned int size;
} drm_i915_mmio_entry_t;

typedef struct drm_i915_mmio {
	unsigned int read_write:1;
	unsigned int reg:31;
	void __user *data;
} drm_i915_mmio_t;

typedef struct drm_i915_hws_addr {
	uint64_t addr;
} drm_i915_hws_addr_t;

/*
 * Relocation header is 4 uint32_ts
 * 0 - 32 bit reloc count
 * 1 - 32-bit relocation type
 * 2-3 - 64-bit user buffer handle ptr for another list of relocs.
 */
#define I915_RELOC_HEADER 4

/*
 * type 0 relocation has 4-uint32_t stride
 * 0 - offset into buffer
 * 1 - delta to add in
 * 2 - buffer handle
 * 3 - reserved (for optimisations later).
 */
/*
 * type 1 relocation has 4-uint32_t stride.
 * Hangs off the first item in the op list.
 * Performed after all valiations are done.
 * Try to group relocs into the same relocatee together for
 * performance reasons.
 * 0 - offset into buffer
 * 1 - delta to add in
 * 2 - buffer index in op list.
 * 3 - relocatee index in op list.
 */
#define I915_RELOC_TYPE_0 0
#define I915_RELOC0_STRIDE 4
#define I915_RELOC_TYPE_1 1
#define I915_RELOC1_STRIDE 4


struct drm_i915_op_arg {
	uint64_t next;
	uint64_t reloc_ptr;
	int handled;
	unsigned int pad64;
	union {
		struct drm_bo_op_req req;
		struct drm_bo_arg_rep rep;
	} d;

};

struct drm_i915_execbuffer {
	uint64_t ops_list;
	uint32_t num_buffers;
	struct drm_i915_batchbuffer batch;
	drm_context_t context; /* for lockless use in the future */
	struct drm_fence_arg fence_arg;
};

struct drm_i915_gem_init {
	/**
	 * Beginning offset in the GTT to be managed by the DRM memory
	 * manager.
	 */
	uint64_t gtt_start;
	/**
	 * Ending offset in the GTT to be managed by the DRM memory
	 * manager.
	 */
	uint64_t gtt_end;
};

struct drm_i915_gem_create {
	/**
	 * Requested size for the object.
	 *
	 * The (page-aligned) allocated size for the object will be returned.
	 */
	uint64_t size;
	/**
	 * Returned handle for the object.
	 *
	 * Object handles are nonzero.
	 */
	uint32_t handle;
	uint32_t pad;
};

struct drm_i915_gem_pread {
	/** Handle for the object being read. */
	uint32_t handle;
	uint32_t pad;
	/** Offset into the object to read from */
	uint64_t offset;
	/** Length of data to read */
	uint64_t size;
	/**
	 * Pointer to write the data into.
	 *
	 * This is a fixed-size type for 32/64 compatibility.
	 */
	uint64_t data_ptr;
};

struct drm_i915_gem_pwrite {
	/** Handle for the object being written to. */
	uint32_t handle;
	uint32_t pad;
	/** Offset into the object to write to */
	uint64_t offset;
	/** Length of data to write */
	uint64_t size;
	/**
	 * Pointer to read the data from.
	 *
	 * This is a fixed-size type for 32/64 compatibility.
	 */
	uint64_t data_ptr;
};

struct drm_i915_gem_mmap {
	/** Handle for the object being mapped. */
	uint32_t handle;
	uint32_t pad;
	/** Offset in the object to map. */
	uint64_t offset;
	/**
	 * Length of data to map.
	 *
	 * The value will be page-aligned.
	 */
	uint64_t size;
	/**
	 * Returned pointer the data was mapped at.
	 *
	 * This is a fixed-size type for 32/64 compatibility.
	 */
	uint64_t addr_ptr;
};

struct drm_i915_gem_mmap_gtt {
	/** Handle for the object being mapped. */
	uint32_t handle;
	uint32_t pad;
	/**
	 * Fake offset to use for subsequent mmap call
	 *
	 * This is a fixed-size type for 32/64 compatibility.
	 */
	uint64_t offset;
};

struct drm_i915_gem_set_domain {
	/** Handle for the object */
	uint32_t handle;

	/** New read domains */
	uint32_t read_domains;

	/** New write domain */
	uint32_t write_domain;
};

struct drm_i915_gem_sw_finish {
	/** Handle for the object */
	uint32_t handle;
};

struct drm_i915_gem_relocation_entry {
	/**
	 * Handle of the buffer being pointed to by this relocation entry.
	 *
	 * It's appealing to make this be an index into the mm_validate_entry
	 * list to refer to the buffer, but this allows the driver to create
	 * a relocation list for state buffers and not re-write it per
	 * exec using the buffer.
	 */
	uint32_t target_handle;

	/**
	 * Value to be added to the offset of the target buffer to make up
	 * the relocation entry.
	 */
	uint32_t delta;

	/** Offset in the buffer the relocation entry will be written into */
	uint64_t offset;

	/**
	 * Offset value of the target buffer that the relocation entry was last
	 * written as.
	 *
	 * If the buffer has the same offset as last time, we can skip syncing
	 * and writing the relocation.  This value is written back out by
	 * the execbuffer ioctl when the relocation is written.
	 */
	uint64_t presumed_offset;

	/**
	 * Target memory domains read by this operation.
	 */
	uint32_t read_domains;

	/**
	 * Target memory domains written by this operation.
	 *
	 * Note that only one domain may be written by the whole
	 * execbuffer operation, so that where there are conflicts,
	 * the application will get -EINVAL back.
	 */
	uint32_t write_domain;
};

/** @{
 * Intel memory domains
 *
 * Most of these just align with the various caches in
 * the system and are used to flush and invalidate as
 * objects end up cached in different domains.
 */
/** CPU cache */
#define I915_GEM_DOMAIN_CPU		0x00000001
/** Render cache, used by 2D and 3D drawing */
#define I915_GEM_DOMAIN_RENDER		0x00000002
/** Sampler cache, used by texture engine */
#define I915_GEM_DOMAIN_SAMPLER		0x00000004
/** Command queue, used to load batch buffers */
#define I915_GEM_DOMAIN_COMMAND		0x00000008
/** Instruction cache, used by shader programs */
#define I915_GEM_DOMAIN_INSTRUCTION	0x00000010
/** Vertex address cache */
#define I915_GEM_DOMAIN_VERTEX		0x00000020
/** GTT domain - aperture and scanout */
#define I915_GEM_DOMAIN_GTT		0x00000040
/** @} */

struct drm_i915_gem_exec_object {
	/**
	 * User's handle for a buffer to be bound into the GTT for this
	 * operation.
	 */
	uint32_t handle;

	/** Number of relocations to be performed on this buffer */
	uint32_t relocation_count;
	/**
	 * Pointer to array of struct drm_i915_gem_relocation_entry containing
	 * the relocations to be performed in this buffer.
	 */
	uint64_t relocs_ptr;

	/** Required alignment in graphics aperture */
	uint64_t alignment;

	/**
	 * Returned value of the updated offset of the object, for future
	 * presumed_offset writes.
	 */
	uint64_t offset;
};

struct drm_i915_gem_execbuffer {
	/**
	 * List of buffers to be validated with their relocations to be
	 * performend on them.
	 *
	 * This is a pointer to an array of struct drm_i915_gem_validate_entry.
	 *
	 * These buffers must be listed in an order such that all relocations
	 * a buffer is performing refer to buffers that have already appeared
	 * in the validate list.
	 */
	uint64_t buffers_ptr;
	uint32_t buffer_count;

	/** Offset in the batchbuffer to start execution from. */
	uint32_t batch_start_offset;
	/** Bytes used in batchbuffer from batch_start_offset */
	uint32_t batch_len;
	uint32_t DR1;
	uint32_t DR4;
	uint32_t num_cliprects;
	/** This is a struct drm_clip_rect *cliprects */
	uint64_t cliprects_ptr;
};

struct drm_i915_gem_pin {
	/** Handle of the buffer to be pinned. */
	uint32_t handle;
	uint32_t pad;

	/** alignment required within the aperture */
	uint64_t alignment;

	/** Returned GTT offset of the buffer. */
	uint64_t offset;
};

struct drm_i915_gem_unpin {
	/** Handle of the buffer to be unpinned. */
	uint32_t handle;
	uint32_t pad;
};

struct drm_i915_gem_busy {
	/** Handle of the buffer to check for busy */
	uint32_t handle;

	/** Return busy status (1 if busy, 0 if idle) */
	uint32_t busy;
};

#define I915_TILING_NONE	0
#define I915_TILING_X		1
#define I915_TILING_Y		2

#define I915_BIT_6_SWIZZLE_NONE		0
#define I915_BIT_6_SWIZZLE_9		1
#define I915_BIT_6_SWIZZLE_9_10		2
#define I915_BIT_6_SWIZZLE_9_11		3
#define I915_BIT_6_SWIZZLE_9_10_11	4
/* Not seen by userland */
#define I915_BIT_6_SWIZZLE_UNKNOWN	5

struct drm_i915_gem_set_tiling {
	/** Handle of the buffer to have its tiling state updated */
	uint32_t handle;

	/**
	 * Tiling mode for the object (I915_TILING_NONE, I915_TILING_X,
	 * I915_TILING_Y).
	 *
	 * This value is to be set on request, and will be updated by the
	 * kernel on successful return with the actual chosen tiling layout.
	 *
	 * The tiling mode may be demoted to I915_TILING_NONE when the system
	 * has bit 6 swizzling that can't be managed correctly by GEM.
	 *
	 * Buffer contents become undefined when changing tiling_mode.
	 */
	uint32_t tiling_mode;

	/**
	 * Stride in bytes for the object when in I915_TILING_X or
	 * I915_TILING_Y.
	 */
	uint32_t stride;

	/**
	 * Returned address bit 6 swizzling required for CPU access through
	 * mmap mapping.
	 */
	uint32_t swizzle_mode;
};

struct drm_i915_gem_get_tiling {
	/** Handle of the buffer to get tiling state for. */
	uint32_t handle;

	/**
	 * Current tiling mode for the object (I915_TILING_NONE, I915_TILING_X,
	 * I915_TILING_Y).
	 */
	uint32_t tiling_mode;

	/**
	 * Returned address bit 6 swizzling required for CPU access through
	 * mmap mapping.
	 */
	uint32_t swizzle_mode;
};

struct drm_i915_gem_get_aperture {
	/** Total size of the aperture used by i915_gem_execbuffer, in bytes */
	uint64_t aper_size;

	/**
	 * Available space in the aperture used by i915_gem_execbuffer, in
	 * bytes
	 */
	uint64_t aper_available_size;
};

struct drm_i915_get_pipe_from_crtc_id {
	/** ID of CRTC being requested **/
	uint32_t crtc_id;

	/** pipe of requested CRTC **/
	uint32_t pipe;
};

#endif				/* _I915_DRM_H_ */
