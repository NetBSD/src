/* i915_drv.h -- Private header for the I915 driver -*- linux-c -*-
 */
/*
 *
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

#ifndef _I915_DRV_H_
#define _I915_DRV_H_

#include "i915_reg.h"

/* General customization:
 */

#define DRIVER_AUTHOR		"Tungsten Graphics, Inc."

#define DRIVER_NAME		"i915"
#define DRIVER_DESC		"Intel Graphics"
#define DRIVER_DATE		"20080730"

enum pipe {
	PIPE_A = 0,
	PIPE_B,
};

#define I915_NUM_PIPE	2

/* Interface history:
 *
 * 1.1: Original.
 * 1.2: Add Power Management
 * 1.3: Add vblank support
 * 1.4: Fix cmdbuffer path, add heap destroy
 * 1.5: Add vblank pipe configuration
 * 1.6: - New ioctl for scheduling buffer swaps on vertical blank
 *      - Support vertical blank on secondary display pipe
 */
#define DRIVER_MAJOR		1
#define DRIVER_MINOR		6
#define DRIVER_PATCHLEVEL	0

#define WATCH_COHERENCY	0
#define WATCH_BUF	0
#define WATCH_EXEC	0
#define WATCH_LRU	0
#define WATCH_RELOC	0
#define WATCH_INACTIVE	0
#define WATCH_PWRITE	0

typedef struct _drm_i915_ring_buffer {
	int tail_mask;
	unsigned long Size;
	u8 *virtual_start;
	int head;
	int tail;
	int space;
	drm_local_map_t map;
	struct drm_gem_object *ring_obj;
} drm_i915_ring_buffer_t;

struct mem_block {
	struct mem_block *next;
	struct mem_block *prev;
	int start;
	int size;
	struct drm_file *file_priv; /* NULL: free, -1: heap, other: real files */
};

struct opregion_header;
struct opregion_acpi;
struct opregion_swsci;
struct opregion_asle;

struct intel_opregion {
	struct opregion_header *header;
	struct opregion_acpi *acpi;
	struct opregion_swsci *swsci;
	struct opregion_asle *asle;
	int enabled;
};

typedef struct drm_i915_private {
	struct drm_device *dev;

	drm_local_map_t *sarea;
	drm_local_map_t *mmio_map;

	drm_i915_sarea_t *sarea_priv;
	drm_i915_ring_buffer_t ring;

	drm_dma_handle_t *status_page_dmah;
	void *hw_status_page;
	dma_addr_t dma_status_page;
	uint32_t counter;
	unsigned int status_gfx_addr;
	drm_local_map_t hws_map;
	struct drm_gem_object *hws_obj;

	unsigned int cpp;
	int back_offset;
	int front_offset;
	int current_page;
	int page_flipping;

	wait_queue_head_t irq_queue;
	atomic_t irq_received;
	/** Protects user_irq_refcount and irq_mask_reg */
	DRM_SPINTYPE user_irq_lock;
	/** Refcount for i915_user_irq_get() versus i915_user_irq_put(). */
	int user_irq_refcount;
	/** Cached value of IER to avoid reads in updating the bitfield */
	u32 irq_mask_reg;
	u32 pipestat[2];

	int tex_lru_log_granularity;
	int allow_batchbuffer;
	struct mem_block *agp_heap;
	unsigned int sr01, adpa, ppcr, dvob, dvoc, lvds;
	int vblank_pipe;

	struct intel_opregion opregion;

	/* Register state */
	u8 saveLBB;
	u32 saveDSPACNTR;
	u32 saveDSPBCNTR;
	u32 saveDSPARB;
	u32 savePIPEACONF;
	u32 savePIPEBCONF;
	u32 savePIPEASRC;
	u32 savePIPEBSRC;
	u32 saveFPA0;
	u32 saveFPA1;
	u32 saveDPLL_A;
	u32 saveDPLL_A_MD;
	u32 saveHTOTAL_A;
	u32 saveHBLANK_A;
	u32 saveHSYNC_A;
	u32 saveVTOTAL_A;
	u32 saveVBLANK_A;
	u32 saveVSYNC_A;
	u32 saveBCLRPAT_A;
	u32 savePIPEASTAT;
	u32 saveDSPASTRIDE;
	u32 saveDSPASIZE;
	u32 saveDSPAPOS;
	u32 saveDSPAADDR;
	u32 saveDSPASURF;
	u32 saveDSPATILEOFF;
	u32 savePFIT_PGM_RATIOS;
	u32 saveBLC_PWM_CTL;
	u32 saveBLC_PWM_CTL2;
	u32 saveFPB0;
	u32 saveFPB1;
	u32 saveDPLL_B;
	u32 saveDPLL_B_MD;
	u32 saveHTOTAL_B;
	u32 saveHBLANK_B;
	u32 saveHSYNC_B;
	u32 saveVTOTAL_B;
	u32 saveVBLANK_B;
	u32 saveVSYNC_B;
	u32 saveBCLRPAT_B;
	u32 savePIPEBSTAT;
	u32 saveDSPBSTRIDE;
	u32 saveDSPBSIZE;
	u32 saveDSPBPOS;
	u32 saveDSPBADDR;
	u32 saveDSPBSURF;
	u32 saveDSPBTILEOFF;
	u32 saveVGA0;
	u32 saveVGA1;
	u32 saveVGA_PD;
	u32 saveVGACNTRL;
	u32 saveADPA;
	u32 saveLVDS;
	u32 savePP_ON_DELAYS;
	u32 savePP_OFF_DELAYS;
	u32 saveDVOA;
	u32 saveDVOB;
	u32 saveDVOC;
	u32 savePP_ON;
	u32 savePP_OFF;
	u32 savePP_CONTROL;
	u32 savePP_DIVISOR;
	u32 savePFIT_CONTROL;
	u32 save_palette_a[256];
	u32 save_palette_b[256];
	u32 saveFBC_CFB_BASE;
	u32 saveFBC_LL_BASE;
	u32 saveFBC_CONTROL;
	u32 saveFBC_CONTROL2;
	u32 saveIER;
	u32 saveIIR;
	u32 saveIMR;
	u32 saveCACHE_MODE_0;
	u32 saveD_STATE;
	u32 saveCG_2D_DIS;
	u32 saveMI_ARB_STATE;
	u32 saveSWF0[16];
	u32 saveSWF1[16];
	u32 saveSWF2[3];
	u8 saveMSR;
	u8 saveSR[8];
	u8 saveGR[25];
	u8 saveAR_INDEX;
	u8 saveAR[21];
	u8 saveDACMASK;
	u8 saveDACDATA[256*3]; /* 256 3-byte colors */
	u8 saveCR[37];
	struct {
		/**
		 * List of objects currently involved in rendering from the
		 * ringbuffer.
		 *
		 * A reference is held on the buffer while on this list.
		 */
		struct list_head active_list;

		/**
		 * List of objects which are not in the ringbuffer but which
		 * still have a write_domain which needs to be flushed before
		 * unbinding.
		 *
		 * A reference is held on the buffer while on this list.
		 */
		struct list_head flushing_list;

		/**
		 * LRU list of objects which are not in the ringbuffer and
		 * are ready to unbind, but are still in the GTT.
		 *
		 * A reference is not held on the buffer while on this list,
		 * as merely being GTT-bound shouldn't prevent its being
		 * freed, and we'll pull it off the list in the free path.
		 */
		struct list_head inactive_list;

		/**
		 * List of breadcrumbs associated with GPU requests currently
		 * outstanding.
		 */
		struct list_head request_list;
		uint32_t next_gem_seqno;

		/**
		 * Waiting sequence number, if any
		 */
		uint32_t waiting_gem_seqno;

		/**
		 * Last seq seen at irq time
		 */
		uint32_t irq_gem_seqno;

		/**
		 * Flag if the X Server, and thus DRM, is not currently in
		 * control of the device.
		 *
		 * This is set between LeaveVT and EnterVT.  It needs to be
		 * replaced with a semaphore.  It also needs to be
		 * transitioned away from for kernel modesetting.
		 */
		int suspended;

		/**
		 * Flag if the hardware appears to be wedged.
		 *
		 * This is set when attempts to idle the device timeout.
		 * It prevents command submission from occuring and makes
		 * every pending request fail
		 */
		int wedged;

		/** Bit 6 swizzling required for X tiling */
		uint32_t bit_6_swizzle_x;
		/** Bit 6 swizzling required for Y tiling */
		uint32_t bit_6_swizzle_y;
	} mm;
} drm_i915_private_t;

enum intel_chip_family {
	CHIP_I8XX = 0x01,
	CHIP_I9XX = 0x02,
	CHIP_I915 = 0x04,
	CHIP_I965 = 0x08,
};

/** driver private structure attached to each drm_gem_object */
struct drm_i915_gem_object {
	struct drm_gem_object *obj;

	/** Current space allocated to this object in the GTT, if any. */
	struct drm_mm_node *gtt_space;

	/** This object's place on the active/flushing/inactive lists */
	struct list_head list;

	/**
	 * This is set if the object is on the active or flushing lists
	 * (has pending rendering), and is not set if it's on inactive (ready
	 * to be unbound).
	 */
	int active;

	/**
	 * This is set if the object has been written to since last bound
	 * to the GTT
	 */
	int dirty;

	/** AGP memory structure for our GTT binding. */
	DRM_AGP_MEM *agp_mem;

	struct page **page_list;

	/**
	 * Current offset of the object in GTT space.
	 *
	 * This is the same as gtt_space->start
	 */
	uint32_t gtt_offset;

	/** Boolean whether this object has a valid gtt offset. */
	int gtt_bound;

	/** How many users have pinned this object in GTT space */
	int pin_count;

	/** Breadcrumb of last rendering to the buffer. */
	uint32_t last_rendering_seqno;

	/** Current tiling mode for the object. */
	uint32_t tiling_mode;

	/** AGP mapping type (AGP_USER_MEMORY or AGP_USER_CACHED_MEMORY */
	uint32_t agp_type;

	/**
	 * Flagging of which individual pages are valid in GEM_DOMAIN_CPU when
	 * GEM_DOMAIN_CPU is not in the object's read domain.
	 */
	uint8_t *page_cpu_valid;
};

/**
 * Request queue structure.
 *
 * The request queue allows us to note sequence numbers that have been emitted
 * and may be associated with active buffers to be retired.
 *
 * By keeping this list, we can avoid having to do questionable
 * sequence-number comparisons on buffer last_rendering_seqnos, and associate
 * an emission time with seqnos for tracking how far ahead of the GPU we are.
 */
struct drm_i915_gem_request {
	/** GEM sequence number associated with this request. */
	uint32_t seqno;

	/** Time at which this request was emitted, in jiffies. */
	unsigned long emitted_jiffies;

	/** Cache domains that were flushed at the start of the request. */
	uint32_t flush_domains;

	struct list_head list;
};

struct drm_i915_file_private {
	struct {
		uint32_t last_gem_seqno;
		uint32_t last_gem_throttle_seqno;
	} mm;
};

extern struct drm_ioctl_desc i915_ioctls[];
extern int i915_max_ioctl;

				/* i915_dma.c */
extern void i915_kernel_lost_context(struct drm_device * dev);
extern int i915_driver_load(struct drm_device *, unsigned long flags);
extern int i915_driver_unload(struct drm_device *);
extern int i915_driver_open(struct drm_device *dev, struct drm_file *file_priv);
extern void i915_driver_lastclose(struct drm_device * dev);
extern void i915_driver_preclose(struct drm_device *dev,
				 struct drm_file *file_priv);
extern void i915_driver_postclose(struct drm_device *dev,
				  struct drm_file *file_priv);
extern int i915_driver_device_is_agp(struct drm_device * dev);
extern long i915_compat_ioctl(struct file *filp, unsigned int cmd,
			      unsigned long arg);
extern int i915_emit_box(struct drm_device *dev,
			 struct drm_clip_rect __user *boxes,
			 int i, int DR1, int DR4);

/* i915_irq.c */
extern int i915_irq_emit(struct drm_device *dev, void *data,
			 struct drm_file *file_priv);
extern int i915_irq_wait(struct drm_device *dev, void *data,
			 struct drm_file *file_priv);
void i915_user_irq_get(struct drm_device *dev);
void i915_user_irq_put(struct drm_device *dev);

extern irqreturn_t i915_driver_irq_handler(DRM_IRQ_ARGS);
extern void i915_driver_irq_preinstall(struct drm_device * dev);
extern int i915_driver_irq_postinstall(struct drm_device *dev);
extern void i915_driver_irq_uninstall(struct drm_device * dev);
extern int i915_vblank_pipe_set(struct drm_device *dev, void *data,
				struct drm_file *file_priv);
extern int i915_vblank_pipe_get(struct drm_device *dev, void *data,
				struct drm_file *file_priv);
extern int i915_enable_vblank(struct drm_device *dev, unsigned int crtc);
extern void i915_disable_vblank(struct drm_device *dev, unsigned int crtc);
extern u32 i915_get_vblank_counter(struct drm_device *dev, unsigned int crtc);
extern u32 gm45_get_vblank_counter(struct drm_device *dev, unsigned int crtc);
extern int i915_vblank_swap(struct drm_device *dev, void *data,
			    struct drm_file *file_priv);

void
i915_enable_pipestat(drm_i915_private_t *dev_priv, unsigned int pipe, u32 mask);

void
i915_disable_pipestat(drm_i915_private_t *dev_priv, unsigned int pipe, u32 mask);


/* i915_mem.c */
extern int i915_mem_alloc(struct drm_device *dev, void *data,
			  struct drm_file *file_priv);
extern int i915_mem_free(struct drm_device *dev, void *data,
			 struct drm_file *file_priv);
extern int i915_mem_init_heap(struct drm_device *dev, void *data,
			      struct drm_file *file_priv);
extern int i915_mem_destroy_heap(struct drm_device *dev, void *data,
				 struct drm_file *file_priv);
extern void i915_mem_takedown(struct mem_block **heap);
extern void i915_mem_release(struct drm_device * dev,
			     struct drm_file *file_priv, struct mem_block *heap);
#ifdef I915_HAVE_GEM
/* i915_gem.c */
int i915_gem_init_ioctl(struct drm_device *dev, void *data,
			struct drm_file *file_priv);
int i915_gem_create_ioctl(struct drm_device *dev, void *data,
			  struct drm_file *file_priv);
int i915_gem_pread_ioctl(struct drm_device *dev, void *data,
			 struct drm_file *file_priv);
int i915_gem_pwrite_ioctl(struct drm_device *dev, void *data,
			  struct drm_file *file_priv);
int i915_gem_mmap_ioctl(struct drm_device *dev, void *data,
			struct drm_file *file_priv);
int i915_gem_set_domain_ioctl(struct drm_device *dev, void *data,
			      struct drm_file *file_priv);
int i915_gem_sw_finish_ioctl(struct drm_device *dev, void *data,
			     struct drm_file *file_priv);
int i915_gem_execbuffer(struct drm_device *dev, void *data,
			struct drm_file *file_priv);
int i915_gem_pin_ioctl(struct drm_device *dev, void *data,
		       struct drm_file *file_priv);
int i915_gem_unpin_ioctl(struct drm_device *dev, void *data,
			 struct drm_file *file_priv);
int i915_gem_busy_ioctl(struct drm_device *dev, void *data,
			struct drm_file *file_priv);
int i915_gem_throttle_ioctl(struct drm_device *dev, void *data,
			    struct drm_file *file_priv);
int i915_gem_entervt_ioctl(struct drm_device *dev, void *data,
			   struct drm_file *file_priv);
int i915_gem_leavevt_ioctl(struct drm_device *dev, void *data,
			   struct drm_file *file_priv);
int i915_gem_set_tiling(struct drm_device *dev, void *data,
			struct drm_file *file_priv);
int i915_gem_get_tiling(struct drm_device *dev, void *data,
			struct drm_file *file_priv);
void i915_gem_load(struct drm_device *dev);
int i915_gem_proc_init(struct drm_minor *minor);
void i915_gem_proc_cleanup(struct drm_minor *minor);
int i915_gem_init_object(struct drm_gem_object *obj);
void i915_gem_free_object(struct drm_gem_object *obj);
int i915_gem_object_pin(struct drm_gem_object *obj, uint32_t alignment);
void i915_gem_object_unpin(struct drm_gem_object *obj);
void i915_gem_lastclose(struct drm_device *dev);
uint32_t i915_get_gem_seqno(struct drm_device *dev);
void i915_gem_retire_requests(struct drm_device *dev);
void i915_gem_retire_work_handler(struct work_struct *work);
void i915_gem_clflush_object(struct drm_gem_object *obj);

/* i915_gem_tiling.c */
void i915_gem_detect_bit_6_swizzle(struct drm_device *dev);

/* i915_gem_debug.c */
void i915_gem_dump_object(struct drm_gem_object *obj, int len,
			  const char *where, uint32_t mark);
#if WATCH_INACTIVE
void i915_verify_inactive(struct drm_device *dev, char *file, int line);
#else
#define i915_verify_inactive(dev, file, line)
#endif
void i915_gem_object_check_coherency(struct drm_gem_object *obj, int handle);
void i915_gem_dump_object(struct drm_gem_object *obj, int len,
			  const char *where, uint32_t mark);
void i915_dump_lru(struct drm_device *dev, const char *where);
#endif /* I915_HAVE_GEM */

/* i915_suspend.c */
extern int i915_save_state(struct drm_device *dev);
extern int i915_restore_state(struct drm_device *dev);

/* i915_opregion.c */
extern int intel_opregion_init(struct drm_device *dev);
extern void intel_opregion_free(struct drm_device *dev);
extern void opregion_asle_intr(struct drm_device *dev);
extern void opregion_enable_asle(struct drm_device *dev);

/**
 * Lock test for when it's just for synchronization of ring access.
 *
 * In that case, we don't need to do it when GEM is initialized as nobody else
 * has access to the ring.
 */
#define RING_LOCK_TEST_WITH_RETURN(dev, file_priv) do {			\
	if (((drm_i915_private_t *)dev->dev_private)->ring.ring_obj == NULL) \
		LOCK_TEST_WITH_RETURN(dev, file_priv);			\
} while (0)

#define I915_READ(reg)		DRM_READ32(dev_priv->mmio_map, (reg))
#define I915_WRITE(reg,val)	DRM_WRITE32(dev_priv->mmio_map, (reg), (val))
#define I915_READ16(reg)	DRM_READ16(dev_priv->mmio_map, (reg))
#define I915_WRITE16(reg,val)	DRM_WRITE16(dev_priv->mmio_map, (reg), (val))
#define I915_READ8(reg)		DRM_READ8(dev_priv->mmio_map, (reg))
#define I915_WRITE8(reg,val)	DRM_WRITE8(dev_priv->mmio_map, (reg), (val))

#define I915_VERBOSE 0

#define RING_LOCALS	unsigned int outring, ringmask, outcount; \
                        volatile char *virt;

#define BEGIN_LP_RING(n) do {				\
	if (I915_VERBOSE)				\
		DRM_DEBUG("BEGIN_LP_RING(%d)\n", (n));	\
	if (dev_priv->ring.space < (n)*4)		\
		i915_wait_ring(dev, (n)*4, __func__);		\
	outcount = 0;					\
	outring = dev_priv->ring.tail;			\
	ringmask = dev_priv->ring.tail_mask;		\
	virt = dev_priv->ring.virtual_start;		\
} while (0)

#define OUT_RING(n) do {					\
	if (I915_VERBOSE) DRM_DEBUG("   OUT_RING %x\n", (int)(n));	\
	*(volatile unsigned int *)(virt + outring) = (n);	\
        outcount++;						\
	outring += 4;						\
	outring &= ringmask;					\
} while (0)

#define ADVANCE_LP_RING() do {						\
	if (I915_VERBOSE) DRM_DEBUG("ADVANCE_LP_RING %x\n", outring);	\
	dev_priv->ring.tail = outring;					\
	dev_priv->ring.space -= outcount * 4;				\
	I915_WRITE(PRB0_TAIL, outring);			\
} while(0)

/**
 * Reads a dword out of the status page, which is written to from the command
 * queue by automatic updates, MI_REPORT_HEAD, MI_STORE_DATA_INDEX, or
 * MI_STORE_DATA_IMM.
 *
 * The following dwords have a reserved meaning:
 * 0x00: ISR copy, updated when an ISR bit not set in the HWSTAM changes.
 * 0x04: ring 0 head pointer
 * 0x05: ring 1 head pointer (915-class)
 * 0x06: ring 2 head pointer (915-class)
 * 0x10-0x1b: Context status DWords (GM45)
 * 0x1f: Last written status offset. (GM45)
 *
 * The area from dword 0x20 to 0x3ff is available for driver usage.
 */
#define READ_HWSP(dev_priv, reg)  (((volatile u32*)(dev_priv->hw_status_page))[reg])
#define READ_BREADCRUMB(dev_priv) READ_HWSP(dev_priv, I915_BREADCRUMB_INDEX)
#define I915_GEM_HWS_INDEX		0x20
#define I915_BREADCRUMB_INDEX		0x21

extern int i915_wait_ring(struct drm_device * dev, int n, const char *caller);

#define IS_I830(dev) ((dev)->pci_device == 0x3577)
#define IS_845G(dev) ((dev)->pci_device == 0x2562)
#define IS_I85X(dev) ((dev)->pci_device == 0x3582)
#define IS_I855(dev) ((dev)->pci_device == 0x3582)
#define IS_I865G(dev) ((dev)->pci_device == 0x2572)

#define IS_I915G(dev) ((dev)->pci_device == 0x2582 || (dev)->pci_device == 0x258a)
#define IS_I915GM(dev) ((dev)->pci_device == 0x2592)
#define IS_I945G(dev) ((dev)->pci_device == 0x2772)
#define IS_I945GM(dev) ((dev)->pci_device == 0x27A2 ||\
		        (dev)->pci_device == 0x27AE)
#define IS_I965G(dev) ((dev)->pci_device == 0x2972 || \
		       (dev)->pci_device == 0x2982 || \
		       (dev)->pci_device == 0x2992 || \
		       (dev)->pci_device == 0x29A2 || \
		       (dev)->pci_device == 0x2A02 || \
		       (dev)->pci_device == 0x2A12 || \
		       (dev)->pci_device == 0x2A42 || \
		       (dev)->pci_device == 0x2E02 || \
		       (dev)->pci_device == 0x2E12 || \
		       (dev)->pci_device == 0x2E22)

#define IS_I965GM(dev) ((dev)->pci_device == 0x2A02)

#define IS_GM45(dev) ((dev)->pci_device == 0x2A42)

#define IS_G4X(dev) ((dev)->pci_device == 0x2E02 || \
		     (dev)->pci_device == 0x2E12 || \
		     (dev)->pci_device == 0x2E22)

#define IS_IGDG(dev) ((dev)->pci_device == 0xA001)
#define IS_IGDGM(dev) ((dev)->pci_device == 0xA011)
#define IS_IGD(dev) (IS_IGDG(dev) || IS_IGDGM(dev))

#define IS_G33(dev)    ((dev)->pci_device == 0x29C2 ||	\
			(dev)->pci_device == 0x29B2 ||	\
			(dev)->pci_device == 0x29D2 ||	\
			IS_IGD(dev))

#define IS_I9XX(dev) (IS_I915G(dev) || IS_I915GM(dev) || IS_I945G(dev) || \
		      IS_I945GM(dev) || IS_I965G(dev) || IS_G33(dev))

#define IS_MOBILE(dev) (IS_I830(dev) || IS_I85X(dev) || IS_I915GM(dev) || \
			IS_I945GM(dev) || IS_I965GM(dev) || IS_GM45(dev) || \
			IS_IGD(dev))

#define I915_NEED_GFX_HWS(dev) (IS_G33(dev) || IS_GM45(dev) || IS_G4X(dev))

#define PRIMARY_RINGBUFFER_SIZE         (128*1024)

#endif
