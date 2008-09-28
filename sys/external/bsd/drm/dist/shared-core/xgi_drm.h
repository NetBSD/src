/****************************************************************************
 * Copyright (C) 2003-2006 by XGI Technology, Taiwan.
 *
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation on the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT.  IN NO EVENT SHALL XGI AND/OR
 * ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ***************************************************************************/

#ifndef _XGI_DRM_H_
#define _XGI_DRM_H_

#include <linux/types.h>
#include <asm/ioctl.h>

struct drm_xgi_sarea {
	__u16 device_id;
	__u16 vendor_id;

	char device_name[32];

	unsigned int scrn_start;
	unsigned int scrn_xres;
	unsigned int scrn_yres;
	unsigned int scrn_bpp;
	unsigned int scrn_pitch;
};


struct xgi_bootstrap {
	/**
	 * Size of PCI-e GART range in megabytes.
	 */
	struct drm_map gart;
};


enum xgi_mem_location {
	XGI_MEMLOC_NON_LOCAL = 0,
	XGI_MEMLOC_LOCAL = 1,
	XGI_MEMLOC_INVALID = 0x7fffffff
};

struct xgi_mem_alloc {
	/**
	 * Memory region to be used for allocation.
	 *
	 * Must be one of XGI_MEMLOC_NON_LOCAL or XGI_MEMLOC_LOCAL.
	 */
	unsigned int location;

	/**
	 * Number of bytes request.
	 *
	 * On successful allocation, set to the actual number of bytes
	 * allocated.
	 */
	unsigned int size;

	/**
	 * Address of the memory from the graphics hardware's point of view.
	 */
	__u32 hw_addr;

	/**
	 * Offset of the allocation in the mapping.
	 */
	__u32 offset;

	/**
	 * Magic handle used to release memory.
	 *
	 * See also DRM_XGI_FREE ioctl.
	 */
	__u32 index;
};

enum xgi_batch_type {
	BTYPE_2D = 0,
	BTYPE_3D = 1,
	BTYPE_FLIP = 2,
	BTYPE_CTRL = 3,
	BTYPE_NONE = 0x7fffffff
};

struct xgi_cmd_info {
	__u32 type;
	__u32 hw_addr;
	__u32 size;
	__u32 id;
};

struct xgi_state_info {
	unsigned int _fromState;
	unsigned int _toState;
};


/*
 * Ioctl definitions
 */

#define DRM_XGI_BOOTSTRAP           0
#define DRM_XGI_ALLOC               1
#define DRM_XGI_FREE                2
#define DRM_XGI_SUBMIT_CMDLIST      3
#define DRM_XGI_STATE_CHANGE        4
#define DRM_XGI_SET_FENCE           5
#define DRM_XGI_WAIT_FENCE          6

#define XGI_IOCTL_BOOTSTRAP         DRM_IOWR(DRM_COMMAND_BASE + DRM_XGI_BOOTSTRAP, struct xgi_bootstrap)
#define XGI_IOCTL_ALLOC             DRM_IOWR(DRM_COMMAND_BASE + DRM_XGI_ALLOC, struct xgi_mem_alloc)
#define XGI_IOCTL_FREE              DRM_IOW(DRM_COMMAND_BASE + DRM_XGI_FREE, __u32)
#define XGI_IOCTL_SUBMIT_CMDLIST    DRM_IOW(DRM_COMMAND_BASE + DRM_XGI_SUBMIT_CMDLIST, struct xgi_cmd_info)
#define XGI_IOCTL_STATE_CHANGE      DRM_IOW(DRM_COMMAND_BASE + DRM_XGI_STATE_CHANGE, struct xgi_state_info)
#define XGI_IOCTL_SET_FENCE         DRM_IOWR(DRM_COMMAND_BASE + DRM_XGI_SET_FENCE, u32)
#define XGI_IOCTL_WAIT_FENCE        DRM_IOWR(DRM_COMMAND_BASE + DRM_XGI_WAIT_FENCE, u32)

#endif /* _XGI_DRM_H_ */
