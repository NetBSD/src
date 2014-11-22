/**
 * \file drmP.h
 * Private header for Direct Rendering Manager
 *
 * \author Rickard E. (Rik) Faith <faith@valinux.com>
 * \author Gareth Hughes <gareth@valinux.com>
 */

/*
 * Copyright 1999 Precision Insight, Inc., Cedar Park, Texas.
 * Copyright 2000 VA Linux Systems, Inc., Sunnyvale, California.
 * Copyright (c) 2009-2010, Code Aurora Forum.
 * All rights reserved.
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

#ifndef _DRM_P_H_
#define _DRM_P_H_

#ifdef __KERNEL__
#ifdef __alpha__
/* add include of current.h so that "current" is defined
 * before static inline funcs in wait.h. Doing this so we
 * can build the DRM (part of PI DRI). 4/21/2000 S + B */
#include <asm/current.h>
#endif				/* __alpha__ */
#include <linux/kernel.h>
#include <linux/kref.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/file.h>
#include <linux/platform_device.h>
#include <linux/pci.h>
#include <linux/jiffies.h>
#include <linux/dma-mapping.h>
#include <linux/mm.h>
#include <linux/cdev.h>
#include <linux/mutex.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/ratelimit.h>
#if defined(__alpha__) || defined(__powerpc__)
#include <asm/pgtable.h>	/* For pte_wrprotect */
#endif
#include <asm/mman.h>
#include <asm/uaccess.h>
#include <linux/types.h>
#include <linux/agp_backend.h>
#include <linux/workqueue.h>
#include <linux/poll.h>
#include <linux/atomic.h>
#include <linux/uidgid.h>
#include <linux/kref.h>
#include <linux/pm.h>
#include <linux/timer.h>
#include <linux/ktime.h>
#include <asm/pgalloc.h>
#include <drm/drm.h>
#include <drm/drm_sarea.h>
#include <asm/barrier.h>
#include <drm/drm_vma_manager.h>

#include <linux/idr.h>

#ifndef __NetBSD__
#define __OS_HAS_AGP (defined(CONFIG_AGP) || (defined(CONFIG_AGP_MODULE) && defined(MODULE)))
#endif

struct module;

struct drm_file;
struct drm_device;

struct device_node;
struct videomode;

#ifdef __NetBSD__
#include <drm/drm_os_netbsd.h>
#else
#include <drm/drm_os_linux.h>
#endif
#include <drm/drm_hashtab.h>
#include <drm/drm_mm.h>

/*
 * 4 debug categories are defined:
 *
 * CORE: Used in the generic drm code: drm_ioctl.c, drm_mm.c, drm_memory.c, ...
 *	 This is the category used by the DRM_DEBUG() macro.
 *
 * DRIVER: Used in the vendor specific part of the driver: i915, radeon, ...
 *	   This is the category used by the DRM_DEBUG_DRIVER() macro.
 *
 * KMS: used in the modesetting code.
 *	This is the category used by the DRM_DEBUG_KMS() macro.
 *
 * PRIME: used in the prime code.
 *	  This is the category used by the DRM_DEBUG_PRIME() macro.
 *
 * Enabling verbose debug messages is done through the drm.debug parameter,
 * each category being enabled by a bit.
 *
 * drm.debug=0x1 will enable CORE messages
 * drm.debug=0x2 will enable DRIVER messages
 * drm.debug=0x3 will enable CORE and DRIVER messages
 * ...
 * drm.debug=0xf will enable all messages
 *
 * An interesting feature is that it's possible to enable verbose logging at
 * run-time by echoing the debug value in its sysfs node:
 *   # echo 0xf > /sys/module/drm/parameters/debug
 */
#define DRM_UT_CORE 		0x01
#define DRM_UT_DRIVER		0x02
#define DRM_UT_KMS		0x04
#define DRM_UT_PRIME		0x08

extern __printf(2, 3)
void drm_ut_debug_printk(const char *function_name,
			 const char *format, ...);
extern __printf(2, 3)
int drm_err(const char *func, const char *format, ...);

/***********************************************************************/
/** \name DRM template customization defaults */
/*@{*/

/* driver capabilities and requirements mask */
#define DRIVER_USE_AGP     0x1
#define DRIVER_PCI_DMA     0x8
#define DRIVER_SG          0x10
#define DRIVER_HAVE_DMA    0x20
#define DRIVER_HAVE_IRQ    0x40
#define DRIVER_IRQ_SHARED  0x80
#define DRIVER_GEM         0x1000
#define DRIVER_MODESET     0x2000
#define DRIVER_PRIME       0x4000
#define DRIVER_RENDER      0x8000

#define DRIVER_BUS_PCI 0x1
#define DRIVER_BUS_PLATFORM 0x2
#define DRIVER_BUS_USB 0x3
#define DRIVER_BUS_HOST1X 0x4

/***********************************************************************/
/** \name Begin the DRM... */
/*@{*/

#define DRM_DEBUG_CODE 2	  /**< Include debugging code if > 1, then
				     also include looping detection. */

#define DRM_MAGIC_HASH_ORDER  4  /**< Size of key hash table. Must be power of 2. */
#define DRM_KERNEL_CONTEXT    0	 /**< Change drm_resctx if changed */
#define DRM_RESERVED_CONTEXTS 1	 /**< Change drm_resctx if changed */

#define DRM_MAP_HASH_OFFSET 0x10000000

/*@}*/

/***********************************************************************/
/** \name Macros to make printk easier */
/*@{*/

/**
 * Error output.
 *
 * \param fmt printf() like format string.
 * \param arg arguments
 */
#define DRM_ERROR(fmt, ...)				\
	drm_err(__func__, fmt, ##__VA_ARGS__)


#ifdef __NetBSD__
/* XXX Use device_printf, with a device.  */
#define	DRM_INFO(fmt, ...)				\
	printf("drm: " fmt, ##__VA_ARGS__)
#endif

/**
 * Rate limited error output.  Like DRM_ERROR() but won't flood the log.
 *
 * \param fmt printf() like format string.
 * \param arg arguments
 */
#define DRM_ERROR_RATELIMITED(fmt, ...)				\
({									\
	static DEFINE_RATELIMIT_STATE(_rs,				\
				      DEFAULT_RATELIMIT_INTERVAL,	\
				      DEFAULT_RATELIMIT_BURST);		\
									\
	if (__ratelimit(&_rs))						\
		drm_err(__func__, fmt, ##__VA_ARGS__);			\
})

#ifndef __NetBSD__
#define DRM_INFO(fmt, ...)				\
	printk(KERN_INFO "[" DRM_NAME "] " fmt, ##__VA_ARGS__)
#endif

#define DRM_INFO_ONCE(fmt, ...)				\
	printk_once(KERN_INFO "[" DRM_NAME "] " fmt, ##__VA_ARGS__)

/**
 * Debug output.
 *
 * \param fmt printf() like format string.
 * \param arg arguments
 */
#if DRM_DEBUG_CODE
#define DRM_DEBUG(fmt, args...)						\
	do {								\
		if (unlikely(drm_debug & DRM_UT_CORE))			\
			drm_ut_debug_printk(__func__, fmt, ##args);	\
	} while (0)

#define DRM_DEBUG_DRIVER(fmt, args...)					\
	do {								\
		if (unlikely(drm_debug & DRM_UT_DRIVER))		\
			drm_ut_debug_printk(__func__, fmt, ##args);	\
	} while (0)
#define DRM_DEBUG_KMS(fmt, args...)					\
	do {								\
		if (unlikely(drm_debug & DRM_UT_KMS))			\
			drm_ut_debug_printk(__func__, fmt, ##args);	\
	} while (0)
#define DRM_DEBUG_PRIME(fmt, args...)					\
	do {								\
		if (unlikely(drm_debug & DRM_UT_PRIME))			\
			drm_ut_debug_printk(__func__, fmt, ##args);	\
	} while (0)
#else
#define DRM_DEBUG_DRIVER(fmt, args...) do { } while (0)
#define DRM_DEBUG_KMS(fmt, args...)	do { } while (0)
#define DRM_DEBUG_PRIME(fmt, args...)	do { } while (0)
#define DRM_DEBUG(fmt, arg...)		 do { } while (0)
#endif

/*@}*/

/***********************************************************************/
/** \name Internal types and structures */
/*@{*/

#define DRM_ARRAY_SIZE(x) ARRAY_SIZE(x)

#define DRM_IF_VERSION(maj, min) (maj << 16 | min)

/**
 * Test that the hardware lock is held by the caller, returning otherwise.
 *
 * \param dev DRM device.
 * \param filp file pointer of the caller.
 */
#define LOCK_TEST_WITH_RETURN( dev, _file_priv )				\
do {										\
	if (!_DRM_LOCK_IS_HELD(_file_priv->master->lock.hw_lock->lock) ||	\
	    _file_priv->master->lock.file_priv != _file_priv)	{		\
		DRM_ERROR( "%s called without lock held, held  %d owner %p %p\n",\
			   __func__, _DRM_LOCK_IS_HELD(_file_priv->master->lock.hw_lock->lock),\
			   _file_priv->master->lock.file_priv, _file_priv);	\
		return -EINVAL;							\
	}									\
} while (0)

/**
 * Ioctl function type.
 *
 * \param inode device inode.
 * \param file_priv DRM file private pointer.
 * \param cmd command.
 * \param arg argument.
 */
typedef int drm_ioctl_t(struct drm_device *dev, void *data,
			struct drm_file *file_priv);

typedef int drm_ioctl_compat_t(struct file *filp, unsigned int cmd,
			       unsigned long arg);

#ifdef __NetBSD__
/* XXX Kludge...is there a better way to do this?  */
#define	DRM_IOCTL_NR(n)							\
	(IOCBASECMD(n) &~ (IOC_DIRMASK | (IOCGROUP(n) << IOCGROUP_SHIFT)))
#define	DRM_MAJOR	cdevsw_lookup_major(&drm_cdevsw)
#else
#define DRM_IOCTL_NR(n)                _IOC_NR(n)
#define DRM_MAJOR       226
#endif

#define DRM_AUTH	0x1
#define	DRM_MASTER	0x2
#define DRM_ROOT_ONLY	0x4
#define DRM_CONTROL_ALLOW 0x8
#define DRM_UNLOCKED	0x10
#define DRM_RENDER_ALLOW 0x20

struct drm_ioctl_desc {
	unsigned int cmd;
	int flags;
	drm_ioctl_t *func;
	unsigned int cmd_drv;
	const char *name;
};

/**
 * Creates a driver or general drm_ioctl_desc array entry for the given
 * ioctl, for use by drm_ioctl().
 */

#define DRM_IOCTL_DEF_DRV(ioctl, _func, _flags)			\
	[DRM_IOCTL_NR(DRM_##ioctl)] = {.cmd = DRM_##ioctl, .func = _func, .flags = _flags, .cmd_drv = DRM_IOCTL_##ioctl, .name = #ioctl}

struct drm_magic_entry {
	struct list_head head;
	struct drm_hash_item hash_item;
	struct drm_file *priv;
};

struct drm_vma_entry {
	struct list_head head;
	struct vm_area_struct *vma;
	pid_t pid;
};

/**
 * DMA buffer.
 */
struct drm_buf {
	int idx;		       /**< Index into master buflist */
	int total;		       /**< Buffer size */
	int order;		       /**< log-base-2(total) */
	int used;		       /**< Amount of buffer in use (for DMA) */
	unsigned long offset;	       /**< Byte offset (used internally) */
	void *address;		       /**< Address of buffer */
	unsigned long bus_address;     /**< Bus address of buffer */
	struct drm_buf *next;	       /**< Kernel-only: used for free list */
	__volatile__ int waiting;      /**< On kernel DMA queue */
	__volatile__ int pending;      /**< On hardware DMA queue */
	struct drm_file *file_priv;    /**< Private of holding file descr */
	int context;		       /**< Kernel queue for this buffer */
	int while_locked;	       /**< Dispatch this buffer while locked */
	enum {
		DRM_LIST_NONE = 0,
		DRM_LIST_FREE = 1,
		DRM_LIST_WAIT = 2,
		DRM_LIST_PEND = 3,
		DRM_LIST_PRIO = 4,
		DRM_LIST_RECLAIM = 5
	} list;			       /**< Which list we're on */

	int dev_priv_size;		 /**< Size of buffer private storage */
	void *dev_private;		 /**< Per-buffer private storage */
};

/** bufs is one longer than it has to be */
struct drm_waitlist {
	int count;			/**< Number of possible buffers */
	struct drm_buf **bufs;		/**< List of pointers to buffers */
	struct drm_buf **rp;			/**< Read pointer */
	struct drm_buf **wp;			/**< Write pointer */
	struct drm_buf **end;		/**< End pointer */
	spinlock_t read_lock;
	spinlock_t write_lock;
};

struct drm_freelist {
#ifndef __NetBSD__
	int initialized;	       /**< Freelist in use */
	atomic_t count;		       /**< Number of free buffers */
	struct drm_buf *next;	       /**< End pointer */

#ifdef __NetBSD__
	drm_waitqueue_t waiting;       /**< Processes waiting on free bufs */
#else
	wait_queue_head_t waiting;     /**< Processes waiting on free bufs */
#endif
#endif
	int low_mark;		       /**< Low water mark */
	int high_mark;		       /**< High water mark */
#ifndef __NetBSD__
	atomic_t wfh;		       /**< If waiting for high mark */
	spinlock_t lock;
#endif
};

typedef struct drm_dma_handle {
	dma_addr_t busaddr;
	void *vaddr;
	size_t size;
#ifdef __NetBSD__
	bus_dma_tag_t dmah_tag;
	bus_dmamap_t dmah_map;
	bus_dma_segment_t dmah_seg;
#endif
} drm_dma_handle_t;

/**
 * Buffer entry.  There is one of this for each buffer size order.
 */
struct drm_buf_entry {
	int buf_size;			/**< size */
	int buf_count;			/**< number of buffers */
	struct drm_buf *buflist;		/**< buffer list */
	int seg_count;
	int page_order;
	struct drm_dma_handle **seglist;

	struct drm_freelist freelist;
};

/* Event queued up for userspace to read */
struct drm_pending_event {
	struct drm_event *event;
	struct list_head link;
	struct drm_file *file_priv;
	pid_t pid; /* pid of requester, no guarantee it's valid by the time
		      we deliver the event, for tracing only */
	void (*destroy)(struct drm_pending_event *event);
};

/* initial implementaton using a linked list - todo hashtab */
struct drm_prime_file_private {
	struct list_head head;
	struct mutex lock;
};

/** File private data */
struct drm_file {
	unsigned always_authenticated :1;
	unsigned authenticated :1;
	/* Whether we're master for a minor. Protected by master_mutex */
	unsigned is_master :1;
	/* true when the client has asked us to expose stereo 3D mode flags */
	unsigned stereo_allowed :1;
	/*
	 * true if client understands CRTC primary planes and cursor planes
	 * in the plane list
	 */
	unsigned universal_planes:1;

#ifndef __NetBSD__
	struct pid *pid;
	kuid_t uid;
#endif
	drm_magic_t magic;
	struct list_head lhead;
	struct drm_minor *minor;
	unsigned long lock_count;

	/** Mapping of mm object handles to object pointers. */
	struct idr object_idr;
	/** Lock for synchronization of access to object_idr. */
	spinlock_t table_lock;

	struct file *filp;
	void *driver_priv;

	struct drm_master *master; /* master this node is currently associated with
				      N.B. not always minor->master */
	/**
	 * fbs - List of framebuffers associated with this file.
	 *
	 * Protected by fbs_lock. Note that the fbs list holds a reference on
	 * the fb object to prevent it from untimely disappearing.
	 */
	struct list_head fbs;
	struct mutex fbs_lock;

#ifdef __NetBSD__
	drm_waitqueue_t event_wait;
	struct selinfo event_selq;
#else
	wait_queue_head_t event_wait;
#endif
	struct list_head event_list;
	int event_space;

	struct drm_prime_file_private prime;
};

/** Wait queue */
struct drm_queue {
	atomic_t use_count;		/**< Outstanding uses (+1) */
	atomic_t finalization;		/**< Finalization in progress */
	atomic_t block_count;		/**< Count of processes waiting */
	atomic_t block_read;		/**< Queue blocked for reads */
#ifdef __NetBSD__
	drm_waitqueue_t read_queue;	/**< Processes waiting on block_read */
#else
	wait_queue_head_t read_queue;	/**< Processes waiting on block_read */
#endif
	atomic_t block_write;		/**< Queue blocked for writes */
#ifdef __NetBSD__
	drm_waitqueue_t write_queue;	/**< Processes waiting on block_write */
#else
	wait_queue_head_t write_queue;	/**< Processes waiting on block_write */
#endif
	atomic_t total_queued;		/**< Total queued statistic */
	atomic_t total_flushed;		/**< Total flushes statistic */
	atomic_t total_locks;		/**< Total locks statistics */
	enum drm_ctx_flags flags;	/**< Context preserving and 2D-only */
	struct drm_waitlist waitlist;	/**< Pending buffers */
#ifdef __NetBSD__
	drm_waitqueue_t flush_queue;	/**< Processes waiting until flush */
#else
	wait_queue_head_t flush_queue;	/**< Processes waiting until flush */
#endif
};

/**
 * Lock data.
 */
struct drm_lock_data {
	struct drm_hw_lock *hw_lock;	/**< Hardware lock */
	/** Private of lock holder's file (NULL=kernel) */
	struct drm_file *file_priv;
#ifdef __NetBSD__
	drm_waitqueue_t lock_queue;	/**< Queue of blocked processes */
#else
	wait_queue_head_t lock_queue;	/**< Queue of blocked processes */
#endif
	unsigned long lock_time;	/**< Time of last lock in jiffies */
	spinlock_t spinlock;
	uint32_t kernel_waiters;
	uint32_t user_waiters;
	int idle_has_lock;
};

/**
 * DMA data.
 */
struct drm_device_dma {

	struct drm_buf_entry bufs[DRM_MAX_ORDER + 1];	/**< buffers, grouped by their size order */
	int buf_count;			/**< total number of buffers */
	struct drm_buf **buflist;		/**< Vector of pointers into drm_device_dma::bufs */
	int seg_count;
	int page_count;			/**< number of pages */
	unsigned long *pagelist;	/**< page list */
	unsigned long byte_count;
	enum {
		_DRM_DMA_USE_AGP = 0x01,
		_DRM_DMA_USE_SG = 0x02,
		_DRM_DMA_USE_FB = 0x04,
		_DRM_DMA_USE_PCI_RO = 0x08
	} flags;

};

/**
 * AGP memory entry.  Stored as a doubly linked list.
 */
struct drm_agp_mem {
	unsigned long handle;		/**< handle */
	struct agp_memory *memory;
	unsigned long bound;		/**< address */
	int pages;
	struct list_head head;
};

/**
 * AGP data.
 *
 * \sa drm_agp_init() and drm_device::agp.
 */
struct drm_agp_head {
	struct agp_kern_info agp_info;		/**< AGP device information */
	struct list_head memory;
	unsigned long mode;		/**< AGP mode */
	struct agp_bridge_data *bridge;
	int enabled;			/**< whether the AGP bus as been enabled */
	int acquired;			/**< whether the AGP device has been acquired */
	unsigned long base;
	int agp_mtrr;
	int cant_use_aperture;
	unsigned long page_mask;
};

/**
 * Scatter-gather memory.
 */
struct drm_sg_mem {
	unsigned long handle;
	void *virtual;
#ifdef __NetBSD__
	size_t sg_size;
	bus_dma_tag_t sg_tag;
	bus_dmamap_t sg_map;
	unsigned int sg_nsegs;
	unsigned int sg_nsegs_max;
	bus_dma_segment_t sg_segs[];
#else
	int pages;
	struct page **pagelist;
	dma_addr_t *busaddr;
#endif
};

struct drm_sigdata {
	int context;
	struct drm_hw_lock *lock;
};

#ifdef __NetBSD__
/*
 * XXX Remember: memory mappings only.  bm_flags must include
 * BUS_SPACE_MAP_LINEAR.
 */
struct drm_bus_map {
	bus_addr_t		bm_base;
	bus_size_t		bm_size;
	bus_space_handle_t	bm_bsh;
	int			bm_flags;
};
#endif

/**
 * Kernel side of a mapping
 */
struct drm_local_map {
	resource_size_t offset;	 /**< Requested physical address (0 for SAREA)*/
	unsigned long size;	 /**< Requested physical size (bytes) */
	enum drm_map_type type;	 /**< Type of memory to map */
	enum drm_map_flags flags;	 /**< Flags */
	void *handle;		 /**< User-space: "Handle" to pass to mmap() */
				 /**< Kernel-space: kernel-virtual address */
	int mtrr;		 /**< MTRR slot used */

#ifdef __NetBSD__
	union {
		/* _DRM_FRAME_BUFFER, _DRM_AGP, _DRM_REGISTERS */
		/* XXX mtrr should be moved into this case too.  */
		struct {
			/*
			 * XXX bst seems like a waste of space, but not
			 * all accessors have the drm_device handy.
			 */
			bus_space_tag_t bst;
			bus_space_handle_t bsh;
			struct drm_bus_map *bus_map;
		} bus_space;

		/* _DRM_CONSISTENT */
		struct drm_dma_handle *dmah;

		/* _DRM_SCATTER_GATHER */
#if 0				/* XXX stored in dev->sg instead */
		struct drm_sg_mem *sg;
#endif

		/* _DRM_SHM */
		/* XXX Anything?  uvm object?  */
	} lm_data;
#endif
};

typedef struct drm_local_map drm_local_map_t;

/**
 * Mappings list
 */
struct drm_map_list {
	struct list_head head;		/**< list head */
	struct drm_hash_item hash;
	struct drm_local_map *map;	/**< mapping */
	uint64_t user_token;
	struct drm_master *master;
};

/**
 * Context handle list
 */
struct drm_ctx_list {
	struct list_head head;		/**< list head */
	drm_context_t handle;		/**< context handle */
	struct drm_file *tag;		/**< associated fd private data */
};

/* location of GART table */
#define DRM_ATI_GART_MAIN 1
#define DRM_ATI_GART_FB   2

#define DRM_ATI_GART_PCI 1
#define DRM_ATI_GART_PCIE 2
#define DRM_ATI_GART_IGP 3

struct drm_ati_pcigart_info {
	int gart_table_location;
	int gart_reg_if;
	void *addr;
	dma_addr_t bus_addr;
	dma_addr_t table_mask;
	struct drm_dma_handle *table_handle;
	struct drm_local_map mapping;
	int table_size;
};

/**
 * This structure defines the drm_mm memory object, which will be used by the
 * DRM for its buffer objects.
 */
struct drm_gem_object {
	/** Reference count of this object */
	struct kref refcount;

	/**
	 * handle_count - gem file_priv handle count of this object
	 *
	 * Each handle also holds a reference. Note that when the handle_count
	 * drops to 0 any global names (e.g. the id in the flink namespace) will
	 * be cleared.
	 *
	 * Protected by dev->object_name_lock.
	 * */
	unsigned handle_count;

	/** Related drm device */
	struct drm_device *dev;

#ifdef __NetBSD__
	/* UVM anonymous object for shared memory mappings.  */
	struct uvm_object *gemo_shm_uao;

	/* UVM object with custom pager ops for device memory mappings.  */
	struct uvm_object gemo_uvmobj;
#else
	/** File representing the shmem storage */
	struct file *filp;
#endif

	/* Mapping info for this object */
	struct drm_vma_offset_node vma_node;

	/**
	 * Size of the object, in bytes.  Immutable over the object's
	 * lifetime.
	 */
	size_t size;

	/**
	 * Global name for this object, starts at 1. 0 means unnamed.
	 * Access is covered by the object_name_lock in the related drm_device
	 */
	int name;

	/**
	 * Memory domains. These monitor which caches contain read/write data
	 * related to the object. When transitioning from one set of domains
	 * to another, the driver is called to ensure that caches are suitably
	 * flushed and invalidated
	 */
	uint32_t read_domains;
	uint32_t write_domain;

	/**
	 * While validating an exec operation, the
	 * new read/write domain values are computed here.
	 * They will be transferred to the above values
	 * at the point that any cache flushing occurs
	 */
	uint32_t pending_read_domains;
	uint32_t pending_write_domain;

#ifndef __NetBSD__	    /* XXX drm prime */
	/**
	 * dma_buf - dma buf associated with this GEM object
	 *
	 * Pointer to the dma-buf associated with this gem object (either
	 * through importing or exporting). We break the resulting reference
	 * loop when the last gem handle for this object is released.
	 *
	 * Protected by obj->object_name_lock
	 */
	struct dma_buf *dma_buf;

	/**
	 * import_attach - dma buf attachment backing this object
	 *
	 * Any foreign dma_buf imported as a gem object has this set to the
	 * attachment point for the device. This is invariant over the lifetime
	 * of a gem object.
	 *
	 * The driver's ->gem_free_object callback is responsible for cleaning
	 * up the dma_buf attachment and references acquired at import time.
	 *
	 * Note that the drm gem/prime core does not depend upon drivers setting
	 * this field any more. So for drivers where this doesn't make sense
	 * (e.g. virtual devices or a displaylink behind an usb bus) they can
	 * simply leave it as NULL.
	 */
	struct dma_buf_attachment *import_attach;
#endif
};

#include <drm/drm_crtc.h>

/**
 * struct drm_master - drm master structure
 *
 * @refcount: Refcount for this master object.
 * @minor: Link back to minor char device we are master for. Immutable.
 * @unique: Unique identifier: e.g. busid. Protected by drm_global_mutex.
 * @unique_len: Length of unique field. Protected by drm_global_mutex.
 * @unique_size: Amount allocated. Protected by drm_global_mutex.
 * @magiclist: Hash of used authentication tokens. Protected by struct_mutex.
 * @magicfree: List of used authentication tokens. Protected by struct_mutex.
 * @lock: DRI lock information.
 * @driver_priv: Pointer to driver-private information.
 */
struct drm_master {
	struct kref refcount;
	struct drm_minor *minor;
	char *unique;
	int unique_len;
	int unique_size;
	struct drm_open_hash magiclist;
	struct list_head magicfree;
	struct drm_lock_data lock;
	void *driver_priv;
};

/* Size of ringbuffer for vblank timestamps. Just double-buffer
 * in initial implementation.
 */
#define DRM_VBLANKTIME_RBSIZE 2

/* Flags and return codes for get_vblank_timestamp() driver function. */
#define DRM_CALLED_FROM_VBLIRQ 1
#define DRM_VBLANKTIME_SCANOUTPOS_METHOD (1 << 0)
#define DRM_VBLANKTIME_INVBL             (1 << 1)

/* get_scanout_position() return flags */
#define DRM_SCANOUTPOS_VALID        (1 << 0)
#define DRM_SCANOUTPOS_INVBL        (1 << 1)
#define DRM_SCANOUTPOS_ACCURATE     (1 << 2)

struct drm_bus_irq_cookie;

struct drm_bus {
	int bus_type;
	/*
	 * XXX NetBSD will have a problem with this: pci_intr_handle_t
	 * is a long on some LP64 architectures, where int is 32-bit,
	 * such as alpha and mips64.
	 */
	int (*get_irq)(struct drm_device *dev);
#ifdef __NetBSD__
	int (*irq_install)(struct drm_device *, irqreturn_t (*)(void *), int,
	    const char *, void *, struct drm_bus_irq_cookie **);
	void (*irq_uninstall)(struct drm_device *,
	    struct drm_bus_irq_cookie *);
#endif
	const char *(*get_name)(struct drm_device *dev);
	int (*set_busid)(struct drm_device *dev, struct drm_master *master);
	int (*set_unique)(struct drm_device *dev, struct drm_master *master,
			  struct drm_unique *unique);
	int (*irq_by_busid)(struct drm_device *dev, struct drm_irq_busid *p);
};

/**
 * DRM driver structure. This structure represent the common code for
 * a family of cards. There will one drm_device for each card present
 * in this family
 */
struct drm_driver {
	int (*load) (struct drm_device *, unsigned long flags);
	int (*firstopen) (struct drm_device *);
	int (*open) (struct drm_device *, struct drm_file *);
	void (*preclose) (struct drm_device *, struct drm_file *file_priv);
	void (*postclose) (struct drm_device *, struct drm_file *);
	void (*lastclose) (struct drm_device *);
	int (*unload) (struct drm_device *);
	int (*suspend) (struct drm_device *, pm_message_t state);
	int (*resume) (struct drm_device *);
	int (*dma_ioctl) (struct drm_device *dev, void *data, struct drm_file *file_priv);
	int (*dma_quiescent) (struct drm_device *);
	int (*context_dtor) (struct drm_device *dev, int context);

	/**
	 * get_vblank_counter - get raw hardware vblank counter
	 * @dev: DRM device
	 * @crtc: counter to fetch
	 *
	 * Driver callback for fetching a raw hardware vblank counter for @crtc.
	 * If a device doesn't have a hardware counter, the driver can simply
	 * return the value of drm_vblank_count. The DRM core will account for
	 * missed vblank events while interrupts where disabled based on system
	 * timestamps.
	 *
	 * Wraparound handling and loss of events due to modesetting is dealt
	 * with in the DRM core code.
	 *
	 * RETURNS
	 * Raw vblank counter value.
	 */
	u32 (*get_vblank_counter) (struct drm_device *dev, int crtc);

	/**
	 * enable_vblank - enable vblank interrupt events
	 * @dev: DRM device
	 * @crtc: which irq to enable
	 *
	 * Enable vblank interrupts for @crtc.  If the device doesn't have
	 * a hardware vblank counter, this routine should be a no-op, since
	 * interrupts will have to stay on to keep the count accurate.
	 *
	 * RETURNS
	 * Zero on success, appropriate errno if the given @crtc's vblank
	 * interrupt cannot be enabled.
	 */
	int (*enable_vblank) (struct drm_device *dev, int crtc);

	/**
	 * disable_vblank - disable vblank interrupt events
	 * @dev: DRM device
	 * @crtc: which irq to enable
	 *
	 * Disable vblank interrupts for @crtc.  If the device doesn't have
	 * a hardware vblank counter, this routine should be a no-op, since
	 * interrupts will have to stay on to keep the count accurate.
	 */
	void (*disable_vblank) (struct drm_device *dev, int crtc);

	/**
	 * Called by \c drm_device_is_agp.  Typically used to determine if a
	 * card is really attached to AGP or not.
	 *
	 * \param dev  DRM device handle
	 *
	 * \returns
	 * One of three values is returned depending on whether or not the
	 * card is absolutely \b not AGP (return of 0), absolutely \b is AGP
	 * (return of 1), or may or may not be AGP (return of 2).
	 */
	int (*device_is_agp) (struct drm_device *dev);

	/**
	 * Called by vblank timestamping code.
	 *
	 * Return the current display scanout position from a crtc, and an
	 * optional accurate ktime_get timestamp of when position was measured.
	 *
	 * \param dev  DRM device.
	 * \param crtc Id of the crtc to query.
	 * \param flags Flags from the caller (DRM_CALLED_FROM_VBLIRQ or 0).
	 * \param *vpos Target location for current vertical scanout position.
	 * \param *hpos Target location for current horizontal scanout position.
	 * \param *stime Target location for timestamp taken immediately before
	 *               scanout position query. Can be NULL to skip timestamp.
	 * \param *etime Target location for timestamp taken immediately after
	 *               scanout position query. Can be NULL to skip timestamp.
	 *
	 * Returns vpos as a positive number while in active scanout area.
	 * Returns vpos as a negative number inside vblank, counting the number
	 * of scanlines to go until end of vblank, e.g., -1 means "one scanline
	 * until start of active scanout / end of vblank."
	 *
	 * \return Flags, or'ed together as follows:
	 *
	 * DRM_SCANOUTPOS_VALID = Query successful.
	 * DRM_SCANOUTPOS_INVBL = Inside vblank.
	 * DRM_SCANOUTPOS_ACCURATE = Returned position is accurate. A lack of
	 * this flag means that returned position may be offset by a constant
	 * but unknown small number of scanlines wrt. real scanout position.
	 *
	 */
	int (*get_scanout_position) (struct drm_device *dev, int crtc,
				     unsigned int flags,
				     int *vpos, int *hpos, ktime_t *stime,
				     ktime_t *etime);

	/**
	 * Called by \c drm_get_last_vbltimestamp. Should return a precise
	 * timestamp when the most recent VBLANK interval ended or will end.
	 *
	 * Specifically, the timestamp in @vblank_time should correspond as
	 * closely as possible to the time when the first video scanline of
	 * the video frame after the end of VBLANK will start scanning out,
	 * the time immediately after end of the VBLANK interval. If the
	 * @crtc is currently inside VBLANK, this will be a time in the future.
	 * If the @crtc is currently scanning out a frame, this will be the
	 * past start time of the current scanout. This is meant to adhere
	 * to the OpenML OML_sync_control extension specification.
	 *
	 * \param dev dev DRM device handle.
	 * \param crtc crtc for which timestamp should be returned.
	 * \param *max_error Maximum allowable timestamp error in nanoseconds.
	 *                   Implementation should strive to provide timestamp
	 *                   with an error of at most *max_error nanoseconds.
	 *                   Returns true upper bound on error for timestamp.
	 * \param *vblank_time Target location for returned vblank timestamp.
	 * \param flags 0 = Defaults, no special treatment needed.
	 * \param       DRM_CALLED_FROM_VBLIRQ = Function is called from vblank
	 *	        irq handler. Some drivers need to apply some workarounds
	 *              for gpu-specific vblank irq quirks if flag is set.
	 *
	 * \returns
	 * Zero if timestamping isn't supported in current display mode or a
	 * negative number on failure. A positive status code on success,
	 * which describes how the vblank_time timestamp was computed.
	 */
	int (*get_vblank_timestamp) (struct drm_device *dev, int crtc,
				     int *max_error,
				     struct timeval *vblank_time,
				     unsigned flags);

	/* these have to be filled in */

	irqreturn_t(*irq_handler) (DRM_IRQ_ARGS);
	void (*irq_preinstall) (struct drm_device *dev);
	int (*irq_postinstall) (struct drm_device *dev);
	void (*irq_uninstall) (struct drm_device *dev);

	/* Master routines */
	int (*master_create)(struct drm_device *dev, struct drm_master *master);
	void (*master_destroy)(struct drm_device *dev, struct drm_master *master);
	/**
	 * master_set is called whenever the minor master is set.
	 * master_drop is called whenever the minor master is dropped.
	 */

	int (*master_set)(struct drm_device *dev, struct drm_file *file_priv,
			  bool from_open);
	void (*master_drop)(struct drm_device *dev, struct drm_file *file_priv,
			    bool from_release);

	int (*debugfs_init)(struct drm_minor *minor);
	void (*debugfs_cleanup)(struct drm_minor *minor);

	/**
	 * Driver-specific constructor for drm_gem_objects, to set up
	 * obj->driver_private.
	 *
	 * Returns 0 on success.
	 */
	void (*gem_free_object) (struct drm_gem_object *obj);
	int (*gem_open_object) (struct drm_gem_object *, struct drm_file *);
	void (*gem_close_object) (struct drm_gem_object *, struct drm_file *);

	/* prime: */
	/* export handle -> fd (see drm_gem_prime_handle_to_fd() helper) */
	int (*prime_handle_to_fd)(struct drm_device *dev, struct drm_file *file_priv,
				uint32_t handle, uint32_t flags, int *prime_fd);
	/* import fd -> handle (see drm_gem_prime_fd_to_handle() helper) */
	int (*prime_fd_to_handle)(struct drm_device *dev, struct drm_file *file_priv,
				int prime_fd, uint32_t *handle);
	/* export GEM -> dmabuf */
	struct dma_buf * (*gem_prime_export)(struct drm_device *dev,
				struct drm_gem_object *obj, int flags);
	/* import dmabuf -> GEM */
	struct drm_gem_object * (*gem_prime_import)(struct drm_device *dev,
				struct dma_buf *dma_buf);
	/* low-level interface used by drm_gem_prime_{import,export} */
	int (*gem_prime_pin)(struct drm_gem_object *obj);
	void (*gem_prime_unpin)(struct drm_gem_object *obj);
	struct sg_table *(*gem_prime_get_sg_table)(struct drm_gem_object *obj);
	struct drm_gem_object *(*gem_prime_import_sg_table)(
				struct drm_device *dev, size_t size,
				struct sg_table *sgt);
	void *(*gem_prime_vmap)(struct drm_gem_object *obj);
	void (*gem_prime_vunmap)(struct drm_gem_object *obj, void *vaddr);
	int (*gem_prime_mmap)(struct drm_gem_object *obj,
				struct vm_area_struct *vma);

	/* vga arb irq handler */
	void (*vgaarb_irq)(struct drm_device *dev, bool state);

	/* dumb alloc support */
	int (*dumb_create)(struct drm_file *file_priv,
			   struct drm_device *dev,
			   struct drm_mode_create_dumb *args);
	int (*dumb_map_offset)(struct drm_file *file_priv,
			       struct drm_device *dev, uint32_t handle,
			       uint64_t *offset);
	int (*dumb_destroy)(struct drm_file *file_priv,
			    struct drm_device *dev,
			    uint32_t handle);

	/* Driver private ops for this object */
#ifdef __NetBSD__
	int (*mmap_object)(struct drm_device *, off_t, size_t, int,
	    struct uvm_object **, voff_t *, struct file *);
	const struct uvm_pagerops *gem_uvm_ops;
#else
	const struct vm_operations_struct *gem_vm_ops;
#endif

	int major;
	int minor;
	int patchlevel;
	const char *name;
	const char *desc;
	const char *date;

	u32 driver_features;
	int dev_priv_size;
	const struct drm_ioctl_desc *ioctls;
	int num_ioctls;
	const struct file_operations *fops;
	union {
		struct pci_driver *pci;
		struct platform_device *platform_device;
		struct usb_driver *usb;
	} kdriver;
	const struct drm_bus *bus;

	/* List of devices hanging off this driver with stealth attach. */
	struct list_head legacy_dev_list;
};

enum drm_minor_type {
	DRM_MINOR_LEGACY,
	DRM_MINOR_CONTROL,
	DRM_MINOR_RENDER,
	DRM_MINOR_CNT,
};

#ifdef __NetBSD__		/* XXX debugfs */
struct seq_file;
#endif

/**
 * Info file list entry. This structure represents a debugfs or proc file to
 * be created by the drm core
 */
struct drm_info_list {
	const char *name; /** file name */
	int (*show)(struct seq_file*, void*); /** show callback */
	u32 driver_features; /**< Required driver features for this entry */
	void *data;
};

/**
 * debugfs node structure. This structure represents a debugfs file.
 */
struct drm_info_node {
	struct list_head list;
	struct drm_minor *minor;
	const struct drm_info_list *info_ent;
	struct dentry *dent;
};

/**
 * DRM minor structure. This structure represents a drm minor number.
 */
struct drm_minor {
	int index;			/**< Minor device number */
	int type;                       /**< Control or render */
	struct device *kdev;		/**< Linux device */
	struct drm_device *dev;

#ifndef __NetBSD__		/* XXX debugfs */
	struct dentry *debugfs_root;

	struct list_head debugfs_list;
	struct mutex debugfs_lock; /* Protects debugfs_list. */
#endif

	/* currently active master for this node. Protected by master_mutex */
	struct drm_master *master;
	struct drm_mode_group mode_group;
};


struct drm_pending_vblank_event {
	struct drm_pending_event base;
	int pipe;
	struct drm_event_vblank event;
};

struct drm_vblank_crtc {
#ifdef __NetBSD__
	drm_waitqueue_t queue;
#else
	wait_queue_head_t queue;	/**< VBLANK wait queue */
#endif
	struct timeval time[DRM_VBLANKTIME_RBSIZE];	/**< timestamp of current count */
	atomic_t count;			/**< number of VBLANK interrupts */
	atomic_t refcount;		/* number of users of vblank interruptsper crtc */
	u32 last;			/* protected by dev->vbl_lock, used */
					/* for wraparound handling */
	u32 last_wait;			/* Last vblank seqno waited per CRTC */
	unsigned int inmodeset;		/* Display driver is setting mode */
	bool enabled;			/* so we don't call enable more than
					   once per disable */
};

/**
 * DRM device structure. This structure represent a complete card that
 * may contain multiple heads.
 */
struct drm_device {
	struct list_head legacy_dev_list;/**< list of devices per driver for stealth attach cleanup */
	char *devname;			/**< For /proc/interrupts */
	int if_version;			/**< Highest interface version set */

	/** \name Lifetime Management */
	/*@{ */
	struct kref ref;		/**< Object ref-count */
	struct device *dev;		/**< Device structure of bus-device */
	struct drm_driver *driver;	/**< DRM driver managing the device */
	void *dev_private;		/**< DRM driver private data */
	struct drm_minor *control;		/**< Control node */
	struct drm_minor *primary;		/**< Primary node */
	struct drm_minor *render;		/**< Render node */
	atomic_t unplugged;			/**< Flag whether dev is dead */
	struct inode *anon_inode;		/**< inode for private address-space */
	/*@} */

	/** \name Locks */
	/*@{ */
	spinlock_t count_lock;		/**< For inuse, drm_device::open_count, drm_device::buf_use */
	struct mutex struct_mutex;	/**< For others */
	struct mutex master_mutex;      /**< For drm_minor::master and drm_file::is_master */
	/*@} */

	/** \name Usage Counters */
	/*@{ */
	int open_count;			/**< Outstanding files open */
	int buf_use;			/**< Buffers in use -- cannot alloc */
	atomic_t buf_alloc;		/**< Buffer allocation in progress */
	/*@} */

	struct list_head filelist;

	/** \name Memory management */
	/*@{ */
	struct list_head maplist;	/**< Linked list of regions */
	struct drm_open_hash map_hash;	/**< User token hash table for maps */

	/** \name Context handle management */
	/*@{ */
	struct list_head ctxlist;	/**< Linked list of context handles */
	struct mutex ctxlist_mutex;	/**< For ctxlist */

	struct idr ctx_idr;

	struct list_head vmalist;	/**< List of vmas (for debugging) */

	/*@} */

	/** \name DMA support */
	/*@{ */
	struct drm_device_dma *dma;		/**< Optional pointer for DMA support */
	/*@} */

	/** \name Context support */
	/*@{ */
	bool irq_enabled;		/**< True if irq handler is enabled */
#ifdef __NetBSD__
	struct drm_bus_irq_cookie *irq_cookie;
#endif
	__volatile__ long context_flag;	/**< Context swapping flag */
	int last_context;		/**< Last current context */
	/*@} */

	/** \name VBLANK IRQ support */
	/*@{ */

	/*
	 * At load time, disabling the vblank interrupt won't be allowed since
	 * old clients may not call the modeset ioctl and therefore misbehave.
	 * Once the modeset ioctl *has* been called though, we can safely
	 * disable them when unused.
	 */
	bool vblank_disable_allowed;

	/* array of size num_crtcs */
	struct drm_vblank_crtc *vblank;

	spinlock_t vblank_time_lock;    /**< Protects vblank count and time updates during vblank enable/disable */
	spinlock_t vbl_lock;
	struct timer_list vblank_disable_timer;

	u32 max_vblank_count;           /**< size of vblank counter register */

	/**
	 * List of events
	 */
	struct list_head vblank_event_list;
	spinlock_t event_lock;

	/*@} */

	struct drm_agp_head *agp;	/**< AGP data */

	struct pci_dev *pdev;		/**< PCI device structure */
#ifdef __alpha__
	struct pci_controller *hose;
#endif

	struct platform_device *platformdev; /**< Platform device struture */
	struct usb_device *usbdev;

#ifdef __NetBSD__
	bus_space_tag_t bst;
	struct drm_bus_map *bus_maps;
	unsigned bus_nmaps;
	bus_dma_tag_t bus_dmat;
	bus_dma_tag_t dmat;
	bool dmat_subregion_p;
	bus_addr_t dmat_subregion_min;
	bus_addr_t dmat_subregion_max;
#endif

	struct drm_sg_mem *sg;	/**< Scatter gather memory */
	unsigned int num_crtcs;                  /**< Number of CRTCs on this device */
	struct drm_sigdata sigdata;	   /**< For block_all_signals */
#ifndef __NetBSD__
	sigset_t sigmask;
#endif

	struct drm_local_map *agp_buffer_map;
	unsigned int agp_buffer_token;

        struct drm_mode_config mode_config;	/**< Current mode config */

	/** \name GEM information */
	/*@{ */
	struct mutex object_name_lock;
	struct idr object_name_idr;
	struct drm_vma_offset_manager *vma_offset_manager;
	/*@} */
	int switch_power_state;
};

#define DRM_SWITCH_POWER_ON 0
#define DRM_SWITCH_POWER_OFF 1
#define DRM_SWITCH_POWER_CHANGING 2
#define DRM_SWITCH_POWER_DYNAMIC_OFF 3

static __inline__ int drm_core_check_feature(struct drm_device *dev,
					     int feature)
{
	return ((dev->driver->driver_features & feature) ? 1 : 0);
}

static inline int drm_dev_to_irq(struct drm_device *dev)
{
	return dev->driver->bus->get_irq(dev);
}

static inline void drm_device_set_unplugged(struct drm_device *dev)
{
	smp_wmb();
	atomic_set(&dev->unplugged, 1);
}

static inline int drm_device_is_unplugged(struct drm_device *dev)
{
	int ret = atomic_read(&dev->unplugged);
	smp_rmb();
	return ret;
}

static inline bool drm_modeset_is_locked(struct drm_device *dev)
{
	return mutex_is_locked(&dev->mode_config.mutex);
}

static inline bool drm_is_render_client(const struct drm_file *file_priv)
{
	return file_priv->minor->type == DRM_MINOR_RENDER;
}

static inline bool drm_is_control_client(const struct drm_file *file_priv)
{
	return file_priv->minor->type == DRM_MINOR_CONTROL;
}

static inline bool drm_is_primary_client(const struct drm_file *file_priv)
{
	return file_priv->minor->type == DRM_MINOR_LEGACY;
}

/******************************************************************/
/** \name Internal function definitions */
/*@{*/

				/* Driver support (drm_drv.h) */
#ifndef __NetBSD__
extern long drm_ioctl(struct file *filp,
		      unsigned int cmd, unsigned long arg);
extern long drm_compat_ioctl(struct file *filp,
			     unsigned int cmd, unsigned long arg);
#endif
extern int drm_lastclose(struct drm_device *dev);
#ifndef __NetBSD__
extern bool drm_ioctl_flags(unsigned int nr, unsigned int *flags);
#endif

				/* Device support (drm_fops.h) */
extern struct mutex drm_global_mutex;
#ifdef __NetBSD__
extern int drm_open_file(struct drm_file *, void *, struct drm_minor *);
extern void drm_close_file(struct drm_file *);
#else
extern int drm_open(struct inode *inode, struct file *filp);
extern int drm_stub_open(struct inode *inode, struct file *filp);
extern ssize_t drm_read(struct file *filp, char __user *buffer,
			size_t count, loff_t *offset);
extern int drm_release(struct inode *inode, struct file *filp);
#endif

				/* Mapping support (drm_vm.h) */
#ifdef __NetBSD__
extern int drm_mmap_object(struct drm_device *, off_t, size_t, int,
    struct uvm_object **, voff_t *, struct file *);
extern paddr_t drm_mmap_paddr(struct drm_device *, off_t, int);
#else
extern int drm_mmap(struct file *filp, struct vm_area_struct *vma);
extern int drm_mmap_locked(struct file *filp, struct vm_area_struct *vma);
extern void drm_vm_open_locked(struct drm_device *dev, struct vm_area_struct *vma);
extern void drm_vm_close_locked(struct drm_device *dev, struct vm_area_struct *vma);
extern unsigned int drm_poll(struct file *filp, struct poll_table_struct *wait);

#endif

				/* Memory management support (drm_memory.h) */
#include <drm/drm_memory.h>
#ifdef __NetBSD__		/* XXX move to drm_memory.h */
extern void *drm_ioremap(struct drm_device *dev, struct drm_local_map *map);
extern void drm_iounmap(struct drm_device *dev, struct drm_local_map *map);
extern int drm_limit_dma_space(struct drm_device *, resource_size_t,
    resource_size_t);
#endif

				/* Misc. IOCTL support (drm_ioctl.h) */
extern int drm_irq_by_busid(struct drm_device *dev, void *data,
			    struct drm_file *file_priv);
extern int drm_getunique(struct drm_device *dev, void *data,
			 struct drm_file *file_priv);
extern int drm_setunique(struct drm_device *dev, void *data,
			 struct drm_file *file_priv);
extern int drm_getmap(struct drm_device *dev, void *data,
		      struct drm_file *file_priv);
extern int drm_getclient(struct drm_device *dev, void *data,
			 struct drm_file *file_priv);
extern int drm_getstats(struct drm_device *dev, void *data,
			struct drm_file *file_priv);
extern int drm_getcap(struct drm_device *dev, void *data,
		      struct drm_file *file_priv);
extern int drm_setclientcap(struct drm_device *dev, void *data,
			    struct drm_file *file_priv);
extern int drm_setversion(struct drm_device *dev, void *data,
			  struct drm_file *file_priv);
extern int drm_noop(struct drm_device *dev, void *data,
		    struct drm_file *file_priv);

				/* Context IOCTL support (drm_context.h) */
extern int drm_resctx(struct drm_device *dev, void *data,
		      struct drm_file *file_priv);
extern int drm_addctx(struct drm_device *dev, void *data,
		      struct drm_file *file_priv);
extern int drm_getctx(struct drm_device *dev, void *data,
		      struct drm_file *file_priv);
extern int drm_switchctx(struct drm_device *dev, void *data,
			 struct drm_file *file_priv);
extern int drm_newctx(struct drm_device *dev, void *data,
		      struct drm_file *file_priv);
extern int drm_rmctx(struct drm_device *dev, void *data,
		     struct drm_file *file_priv);

extern int drm_ctxbitmap_init(struct drm_device *dev);
extern void drm_ctxbitmap_cleanup(struct drm_device *dev);
extern void drm_ctxbitmap_free(struct drm_device *dev, int ctx_handle);

extern int drm_setsareactx(struct drm_device *dev, void *data,
			   struct drm_file *file_priv);
extern int drm_getsareactx(struct drm_device *dev, void *data,
			   struct drm_file *file_priv);

				/* Authentication IOCTL support (drm_auth.h) */
extern int drm_getmagic(struct drm_device *dev, void *data,
			struct drm_file *file_priv);
extern int drm_authmagic(struct drm_device *dev, void *data,
			 struct drm_file *file_priv);
extern int drm_remove_magic(struct drm_master *master, drm_magic_t magic);

/* Cache management (drm_cache.c) */
void drm_clflush_pages(struct page *pages[], unsigned long num_pages);
#ifdef __NetBSD__		/* XXX drm clflush */
void drm_clflush_pglist(struct pglist *);
void drm_clflush_page(struct page *);
void drm_clflush_virt_range(const void *, size_t);
#else
void drm_clflush_sg(struct sg_table *st);
void drm_clflush_virt_range(char *addr, unsigned long length);
#endif

				/* Locking IOCTL support (drm_lock.h) */
extern int drm_lock(struct drm_device *dev, void *data,
		    struct drm_file *file_priv);
extern int drm_unlock(struct drm_device *dev, void *data,
		      struct drm_file *file_priv);
extern int drm_lock_free(struct drm_lock_data *lock_data, unsigned int context);
extern void drm_idlelock_take(struct drm_lock_data *lock_data);
extern void drm_idlelock_release(struct drm_lock_data *lock_data);

/*
 * These are exported to drivers so that they can implement fencing using
 * DMA quiscent + idle. DMA quiescent usually requires the hardware lock.
 */

extern int drm_i_have_hw_lock(struct drm_device *dev, struct drm_file *file_priv);

				/* Buffer management support (drm_bufs.h) */
extern int drm_addbufs_agp(struct drm_device *dev, struct drm_buf_desc * request);
extern int drm_addbufs_pci(struct drm_device *dev, struct drm_buf_desc * request);
extern int drm_addmap(struct drm_device *dev, resource_size_t offset,
		      unsigned int size, enum drm_map_type type,
		      enum drm_map_flags flags, struct drm_local_map **map_ptr);
extern int drm_addmap_ioctl(struct drm_device *dev, void *data,
			    struct drm_file *file_priv);
extern int drm_rmmap(struct drm_device *dev, struct drm_local_map *map);
extern int drm_rmmap_locked(struct drm_device *dev, struct drm_local_map *map);
extern int drm_rmmap_ioctl(struct drm_device *dev, void *data,
			   struct drm_file *file_priv);
extern int drm_addbufs(struct drm_device *dev, void *data,
		       struct drm_file *file_priv);
extern int drm_infobufs(struct drm_device *dev, void *data,
			struct drm_file *file_priv);
extern int drm_markbufs(struct drm_device *dev, void *data,
			struct drm_file *file_priv);
extern int drm_freebufs(struct drm_device *dev, void *data,
			struct drm_file *file_priv);
extern int drm_mapbufs(struct drm_device *dev, void *data,
		       struct drm_file *file_priv);
extern int drm_dma_ioctl(struct drm_device *dev, void *data,
			 struct drm_file *file_priv);

				/* DMA support (drm_dma.h) */
extern int drm_legacy_dma_setup(struct drm_device *dev);
extern void drm_legacy_dma_takedown(struct drm_device *dev);
extern void drm_free_buffer(struct drm_device *dev, struct drm_buf * buf);
extern void drm_core_reclaim_buffers(struct drm_device *dev,
				     struct drm_file *filp);

				/* IRQ support (drm_irq.h) */
extern int drm_control(struct drm_device *dev, void *data,
		       struct drm_file *file_priv);
extern int drm_irq_install(struct drm_device *dev);
extern int drm_irq_uninstall(struct drm_device *dev);

extern int drm_vblank_init(struct drm_device *dev, int num_crtcs);
extern int drm_wait_vblank(struct drm_device *dev, void *data,
			   struct drm_file *filp);
extern u32 drm_vblank_count(struct drm_device *dev, int crtc);
extern u32 drm_vblank_count_and_time(struct drm_device *dev, int crtc,
				     struct timeval *vblanktime);
extern void drm_send_vblank_event(struct drm_device *dev, int crtc,
				     struct drm_pending_vblank_event *e);
extern bool drm_handle_vblank(struct drm_device *dev, int crtc);
extern int drm_vblank_get(struct drm_device *dev, int crtc);
extern void drm_vblank_put(struct drm_device *dev, int crtc);
extern void drm_vblank_off(struct drm_device *dev, int crtc);
extern void drm_vblank_cleanup(struct drm_device *dev);
extern u32 drm_get_last_vbltimestamp(struct drm_device *dev, int crtc,
				     struct timeval *tvblank, unsigned flags);
extern int drm_calc_vbltimestamp_from_scanoutpos(struct drm_device *dev,
						 int crtc, int *max_error,
						 struct timeval *vblank_time,
						 unsigned flags,
						 const struct drm_crtc *refcrtc,
						 const struct drm_display_mode *mode);
extern void drm_calc_timestamping_constants(struct drm_crtc *crtc,
					    const struct drm_display_mode *mode);


/* Modesetting support */
extern void drm_vblank_pre_modeset(struct drm_device *dev, int crtc);
extern void drm_vblank_post_modeset(struct drm_device *dev, int crtc);
extern int drm_modeset_ctl(struct drm_device *dev, void *data,
			   struct drm_file *file_priv);

				/* AGP/GART support (drm_agpsupport.h) */

#include <drm/drm_agpsupport.h>

#ifdef __NetBSD__
struct drm_agp_hooks {
	drm_ioctl_t	*agph_acquire_ioctl;
	drm_ioctl_t	*agph_release_ioctl;
	drm_ioctl_t	*agph_enable_ioctl;
	drm_ioctl_t	*agph_info_ioctl;
	drm_ioctl_t	*agph_alloc_ioctl;
	drm_ioctl_t	*agph_free_ioctl;
	drm_ioctl_t	*agph_bind_ioctl;
	drm_ioctl_t	*agph_unbind_ioctl;
	int		(*agph_release)(struct drm_device *);
	void		(*agph_clear)(struct drm_device *);
};

extern int drm_agp_release_hook(struct drm_device *);
extern void drm_agp_clear_hook(struct drm_device *);

extern int drm_agp_register(const struct drm_agp_hooks *);
extern void drm_agp_deregister(const struct drm_agp_hooks *);
#endif

				/* Stub support (drm_stub.h) */
extern int drm_setmaster_ioctl(struct drm_device *dev, void *data,
			       struct drm_file *file_priv);
extern int drm_dropmaster_ioctl(struct drm_device *dev, void *data,
				struct drm_file *file_priv);
struct drm_master *drm_master_create(struct drm_minor *minor);
extern struct drm_master *drm_master_get(struct drm_master *master);
extern void drm_master_put(struct drm_master **master);

extern void drm_put_dev(struct drm_device *dev);
extern void drm_unplug_dev(struct drm_device *dev);
extern unsigned int drm_debug;
extern unsigned int drm_rnodes;
extern unsigned int drm_universal_planes;

extern unsigned int drm_vblank_offdelay;
extern unsigned int drm_timestamp_precision;
extern unsigned int drm_timestamp_monotonic;

extern struct class *drm_class;
extern struct dentry *drm_debugfs_root;

#ifdef __NetBSD__
extern spinlock_t drm_minor_lock;
#endif
extern struct idr drm_minors_idr;

extern struct drm_local_map *drm_getsarea(struct drm_device *dev);

				/* Debugfs support */
#if defined(CONFIG_DEBUG_FS)
extern int drm_debugfs_init(struct drm_minor *minor, int minor_id,
			    struct dentry *root);
extern int drm_debugfs_create_files(const struct drm_info_list *files,
				    int count, struct dentry *root,
				    struct drm_minor *minor);
extern int drm_debugfs_remove_files(const struct drm_info_list *files,
				    int count, struct drm_minor *minor);
extern int drm_debugfs_cleanup(struct drm_minor *minor);
#else
static inline int drm_debugfs_init(struct drm_minor *minor, int minor_id,
				   struct dentry *root)
{
	return 0;
}

static inline int drm_debugfs_create_files(const struct drm_info_list *files,
					   int count, struct dentry *root,
					   struct drm_minor *minor)
{
	return 0;
}

static inline int drm_debugfs_remove_files(const struct drm_info_list *files,
					   int count, struct drm_minor *minor)
{
	return 0;
}

static inline int drm_debugfs_cleanup(struct drm_minor *minor)
{
	return 0;
}
#endif

#ifndef __NetBSD__
				/* Info file support */
extern int drm_name_info(struct seq_file *m, void *data);
extern int drm_vm_info(struct seq_file *m, void *data);
extern int drm_bufs_info(struct seq_file *m, void *data);
extern int drm_vblank_info(struct seq_file *m, void *data);
extern int drm_clients_info(struct seq_file *m, void* data);
extern int drm_gem_name_info(struct seq_file *m, void *data);
#endif


extern struct dma_buf *drm_gem_prime_export(struct drm_device *dev,
		struct drm_gem_object *obj, int flags);
extern int drm_gem_prime_handle_to_fd(struct drm_device *dev,
		struct drm_file *file_priv, uint32_t handle, uint32_t flags,
		int *prime_fd);
extern struct drm_gem_object *drm_gem_prime_import(struct drm_device *dev,
		struct dma_buf *dma_buf);
extern int drm_gem_prime_fd_to_handle(struct drm_device *dev,
		struct drm_file *file_priv, int prime_fd, uint32_t *handle);
extern void drm_gem_dmabuf_release(struct dma_buf *dma_buf);

extern int drm_prime_handle_to_fd_ioctl(struct drm_device *dev, void *data,
					struct drm_file *file_priv);
extern int drm_prime_fd_to_handle_ioctl(struct drm_device *dev, void *data,
					struct drm_file *file_priv);

#ifndef __NetBSD__		/* XXX temporary measure 20130212 */
extern int drm_prime_sg_to_page_addr_arrays(struct sg_table *sgt, struct page **pages,
					    dma_addr_t *addrs, int max_pages);
extern struct sg_table *drm_prime_pages_to_sg(struct page **pages, int nr_pages);
extern void drm_prime_gem_destroy(struct drm_gem_object *obj, struct sg_table *sg);
#endif

int drm_gem_dumb_destroy(struct drm_file *file,
			 struct drm_device *dev,
			 uint32_t handle);

#ifndef __NetBSD__		/* XXX drm prime */
void drm_prime_init_file_private(struct drm_prime_file_private *prime_fpriv);
void drm_prime_destroy_file_private(struct drm_prime_file_private *prime_fpriv);
void drm_prime_remove_buf_handle_locked(struct drm_prime_file_private *prime_fpriv, struct dma_buf *dma_buf);
#endif

#if DRM_DEBUG_CODE
#ifndef __NetBSD__
extern int drm_vma_info(struct seq_file *m, void *data);
#endif
#endif

				/* Scatter Gather Support (drm_scatter.h) */
extern void drm_legacy_sg_cleanup(struct drm_device *dev);
extern int drm_sg_alloc(struct drm_device *dev, void *data,
			struct drm_file *file_priv);
extern int drm_sg_free(struct drm_device *dev, void *data,
		       struct drm_file *file_priv);

			       /* ATI PCIGART support (ati_pcigart.h) */
extern int drm_ati_pcigart_init(struct drm_device *dev,
				struct drm_ati_pcigart_info * gart_info);
extern int drm_ati_pcigart_cleanup(struct drm_device *dev,
				   struct drm_ati_pcigart_info * gart_info);

extern drm_dma_handle_t *drm_pci_alloc(struct drm_device *dev, size_t size,
				       size_t align);
extern void __drm_pci_free(struct drm_device *dev, drm_dma_handle_t * dmah);
extern void drm_pci_free(struct drm_device *dev, drm_dma_handle_t * dmah);
#ifdef __NetBSD__
extern int drm_pci_attach(device_t, const struct pci_attach_args *,
    struct pci_dev *, struct drm_driver *, unsigned long,
    struct drm_device **);
extern int drm_pci_detach(struct drm_device *, int);
#endif

			       /* sysfs support (drm_sysfs.c) */
struct drm_sysfs_class;
extern struct class *drm_sysfs_create(struct module *owner, char *name);
extern void drm_sysfs_destroy(void);
extern int drm_sysfs_device_add(struct drm_minor *minor);
extern void drm_sysfs_hotplug_event(struct drm_device *dev);
extern void drm_sysfs_device_remove(struct drm_minor *minor);
extern int drm_sysfs_connector_add(struct drm_connector *connector);
extern void drm_sysfs_connector_remove(struct drm_connector *connector);

/* Graphics Execution Manager library functions (drm_gem.c) */
int drm_gem_init(struct drm_device *dev);
void drm_gem_destroy(struct drm_device *dev);
void drm_gem_object_release(struct drm_gem_object *obj);
void drm_gem_object_free(struct kref *kref);
int drm_gem_object_init(struct drm_device *dev,
			struct drm_gem_object *obj, size_t size);
void drm_gem_private_object_init(struct drm_device *dev,
				 struct drm_gem_object *obj, size_t size);
#ifdef __NetBSD__
void drm_gem_pager_reference(struct uvm_object *);
void drm_gem_pager_detach(struct uvm_object *);
int drm_gem_mmap_object(struct drm_device *, off_t, size_t, int,
    struct uvm_object **, voff_t *, struct file *);
int drm_gem_or_legacy_mmap_object(struct drm_device *, off_t, size_t, int,
    struct uvm_object **, voff_t *, struct file *);
#else
void drm_gem_vm_open(struct vm_area_struct *vma);
void drm_gem_vm_close(struct vm_area_struct *vma);
int drm_gem_mmap_obj(struct drm_gem_object *obj, unsigned long obj_size,
		     struct vm_area_struct *vma);
int drm_gem_mmap(struct file *filp, struct vm_area_struct *vma);
#endif

#include <drm/drm_global.h>

static inline void
drm_gem_object_reference(struct drm_gem_object *obj)
{
	kref_get(&obj->refcount);
}

static inline void
drm_gem_object_unreference(struct drm_gem_object *obj)
{
	if (obj != NULL)
		kref_put(&obj->refcount, drm_gem_object_free);
}

static inline void
drm_gem_object_unreference_unlocked(struct drm_gem_object *obj)
{
	if (obj != NULL) {
		struct drm_device *const dev = obj->dev;
		if (kref_put_mutex(&obj->refcount, drm_gem_object_free,
			&dev->struct_mutex))
			mutex_unlock(&dev->struct_mutex);
	}
}

int drm_gem_handle_create_tail(struct drm_file *file_priv,
			       struct drm_gem_object *obj,
			       u32 *handlep);
int drm_gem_handle_create(struct drm_file *file_priv,
			  struct drm_gem_object *obj,
			  u32 *handlep);
int drm_gem_handle_delete(struct drm_file *filp, u32 handle);


void drm_gem_free_mmap_offset(struct drm_gem_object *obj);
int drm_gem_create_mmap_offset(struct drm_gem_object *obj);
int drm_gem_create_mmap_offset_size(struct drm_gem_object *obj, size_t size);

struct page **drm_gem_get_pages(struct drm_gem_object *obj, gfp_t gfpmask);
void drm_gem_put_pages(struct drm_gem_object *obj, struct page **pages,
		bool dirty, bool accessed);

struct drm_gem_object *drm_gem_object_lookup(struct drm_device *dev,
					     struct drm_file *filp,
					     u32 handle);
int drm_gem_close_ioctl(struct drm_device *dev, void *data,
			struct drm_file *file_priv);
int drm_gem_flink_ioctl(struct drm_device *dev, void *data,
			struct drm_file *file_priv);
int drm_gem_open_ioctl(struct drm_device *dev, void *data,
		       struct drm_file *file_priv);
void drm_gem_open(struct drm_device *dev, struct drm_file *file_private);
void drm_gem_release(struct drm_device *dev, struct drm_file *file_private);

extern void drm_core_ioremap(struct drm_local_map *map, struct drm_device *dev);
extern void drm_core_ioremap_wc(struct drm_local_map *map, struct drm_device *dev);
extern void drm_core_ioremapfree(struct drm_local_map *map, struct drm_device *dev);

static __inline__ struct drm_local_map *drm_core_findmap(struct drm_device *dev,
							 unsigned int token)
{
	struct drm_map_list *_entry;
	list_for_each_entry(_entry, &dev->maplist, head)
	    if (_entry->user_token == token)
		return _entry->map;
	return NULL;
}

static __inline__ void drm_core_dropmap(struct drm_local_map *map)
{
}

#include <drm/drm_mem_util.h>

struct drm_device *drm_dev_alloc(struct drm_driver *driver,
				 struct device *parent);
void drm_dev_ref(struct drm_device *dev);
void drm_dev_unref(struct drm_device *dev);
int drm_dev_register(struct drm_device *dev, unsigned long flags);
void drm_dev_unregister(struct drm_device *dev);

struct drm_minor *drm_minor_acquire(unsigned int minor_id);
void drm_minor_release(struct drm_minor *minor);

/*@}*/

/* PCI section */
static __inline__ int drm_pci_device_is_agp(struct drm_device *dev)
{
	if (dev->driver->device_is_agp != NULL) {
		int err = (*dev->driver->device_is_agp) (dev);

		if (err != 2) {
			return err;
		}
	}

	return pci_find_capability(dev->pdev, PCI_CAP_ID_AGP);
}
void drm_pci_agp_destroy(struct drm_device *dev);

extern int drm_pci_init(struct drm_driver *driver, struct pci_driver *pdriver);
extern void drm_pci_exit(struct drm_driver *driver, struct pci_driver *pdriver);
extern int drm_get_pci_dev(struct pci_dev *pdev,
			   const struct pci_device_id *ent,
			   struct drm_driver *driver);

#define DRM_PCIE_SPEED_25 1
#define DRM_PCIE_SPEED_50 2
#define DRM_PCIE_SPEED_80 4

extern int drm_pcie_get_speed_cap_mask(struct drm_device *dev, u32 *speed_mask);

/* platform section */
extern int drm_platform_init(struct drm_driver *driver, struct platform_device *platform_device);

/* returns true if currently okay to sleep */
static __inline__ bool drm_can_sleep(void)
{
#ifdef __NetBSD__
	return false;		/* XXX */
#else
	if (in_atomic() || in_dbg_master() || irqs_disabled())
		return false;
	return true;
#endif
}

#ifdef __NetBSD__
static inline bool
DRM_IS_BUS_SPACE(struct drm_local_map *map)
{
	switch (map->type) {
	case _DRM_FRAME_BUFFER:
		panic("I don't know how to access drm frame buffer memory!");

	case _DRM_REGISTERS:
		return true;

	case _DRM_SHM:
		panic("I don't know how to access drm shared memory!");

	case _DRM_AGP:
		panic("I don't know how to access drm agp memory!");

	case _DRM_SCATTER_GATHER:
		panic("I don't know how to access drm scatter-gather memory!");

	case _DRM_CONSISTENT:
		/*
		 * XXX Old drm uses bus space access for this, but
		 * consistent maps don't have bus space handles!  They
		 * do, however, have kernel virtual addresses in the
		 * map->handle, so maybe that's right.
		 */
#if 0
		return false;
#endif
		panic("I don't know how to access drm consistent memory!");

	default:
		panic("I don't know what kind of memory you mean!");
	}
}

static inline uint8_t
DRM_READ8(struct drm_local_map *map, bus_size_t offset)
{
	if (DRM_IS_BUS_SPACE(map))
		return bus_space_read_1(map->lm_data.bus_space.bst,
		    map->lm_data.bus_space.bsh, offset);
	else
		return *(volatile uint8_t *)((vaddr_t)map->handle + offset);
}

static inline uint16_t
DRM_READ16(struct drm_local_map *map, bus_size_t offset)
{
	if (DRM_IS_BUS_SPACE(map))
		return bus_space_read_2(map->lm_data.bus_space.bst,
		    map->lm_data.bus_space.bsh, offset);
	else
		return *(volatile uint16_t *)((vaddr_t)map->handle + offset);
}

static inline uint32_t
DRM_READ32(struct drm_local_map *map, bus_size_t offset)
{
	if (DRM_IS_BUS_SPACE(map))
		return bus_space_read_4(map->lm_data.bus_space.bst,
		    map->lm_data.bus_space.bsh, offset);
	else
		return *(volatile uint32_t *)((vaddr_t)map->handle + offset);
}

static inline uint64_t
DRM_READ64(struct drm_local_map *map, bus_size_t offset)
{
	if (DRM_IS_BUS_SPACE(map)) {
#if _LP64			/* XXX How to detect bus_space_read_8?  */
		return bus_space_read_8(map->lm_data.bus_space.bst,
		    map->lm_data.bus_space.bsh, offset);
#elif _BYTE_ORDER == _LITTLE_ENDIAN
		/* XXX Yes, this is sketchy.  */
		return bus_space_read_4(map->lm_data.bus_space.bst,
		    map->lm_data.bus_space.bsh, offset) |
		    ((uint64_t)bus_space_read_4(map->lm_data.bus_space.bst,
			map->lm_data.bus_space.bsh, (offset + 4)) << 32);
#else
		/* XXX Yes, this is sketchy.  */
		return bus_space_read_4(map->lm_data.bus_space.bst,
		    map->lm_data.bus_space.bsh, (offset + 4)) |
		    ((uint64_t)bus_space_read_4(map->lm_data.bus_space.bst,
			map->lm_data.bus_space.bsh, offset) << 32);
#endif
	} else {
		return *(volatile uint64_t *)((vaddr_t)map->handle + offset);
	}
}

static inline void
DRM_WRITE8(struct drm_local_map *map, bus_size_t offset, uint8_t value)
{
	if (DRM_IS_BUS_SPACE(map))
		bus_space_write_1(map->lm_data.bus_space.bst,
		    map->lm_data.bus_space.bsh, offset, value);
	else
		*(volatile uint8_t *)((vaddr_t)map->handle + offset) = value;
}

static inline void
DRM_WRITE16(struct drm_local_map *map, bus_size_t offset, uint16_t value)
{
	if (DRM_IS_BUS_SPACE(map))
		bus_space_write_2(map->lm_data.bus_space.bst,
		    map->lm_data.bus_space.bsh, offset, value);
	else
		*(volatile uint16_t *)((vaddr_t)map->handle + offset) = value;
}

static inline void
DRM_WRITE32(struct drm_local_map *map, bus_size_t offset, uint32_t value)
{
	if (DRM_IS_BUS_SPACE(map))
		bus_space_write_4(map->lm_data.bus_space.bst,
		    map->lm_data.bus_space.bsh, offset, value);
	else
		*(volatile uint32_t *)((vaddr_t)map->handle + offset) = value;
}

static inline void
DRM_WRITE64(struct drm_local_map *map, bus_size_t offset, uint64_t value)
{
	if (DRM_IS_BUS_SPACE(map)) {
#if _LP64			/* XXX How to detect bus_space_write_8?  */
		bus_space_write_8(map->lm_data.bus_space.bst,
		    map->lm_data.bus_space.bsh, offset, value);
#elif _BYTE_ORDER == _LITTLE_ENDIAN
		bus_space_write_4(map->lm_data.bus_space.bst,
		    map->lm_data.bus_space.bsh, offset, (value & 0xffffffffU));
		bus_space_write_4(map->lm_data.bus_space.bst,
		    map->lm_data.bus_space.bsh, (offset + 4), (value >> 32));
#else
		bus_space_write_4(map->lm_data.bus_space.bst,
		    map->lm_data.bus_space.bsh, offset, (value >> 32));
		bus_space_write_4(map->lm_data.bus_space.bst,
		    map->lm_data.bus_space.bsh, (offset + 4),
		    (value & 0xffffffffU));
#endif
	} else {
		*(volatile uint64_t *)((vaddr_t)map->handle + offset) = value;
	}
}
#endif	/* defined(__NetBSD__) */

#ifdef __NetBSD__

/* XXX This is pretty kludgerific.  */

#include <linux/io-mapping.h>

static inline struct io_mapping *
drm_io_mapping_create_wc(struct drm_device *dev, resource_size_t addr,
    unsigned long size)
{
	return bus_space_io_mapping_create_wc(dev->bst, addr, size);
}

#endif	/* defined(__NetBSD__) */

#ifdef __NetBSD__
extern const struct cdevsw drm_cdevsw;
#endif

#endif				/* __KERNEL__ */
#endif
