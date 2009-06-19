/**************************************************************************
 * 
 * Copyright 2006 Tungsten Graphics, Inc., Bismarck, ND. USA.
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, 
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE 
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * 
 **************************************************************************/

#ifndef _XF86MM_H_
#define _XF86MM_H_
#include <stddef.h>
#include <stdint.h>
#include "drm.h"

/*
 * Note on multithreaded applications using this interface.
 * Libdrm is not threadsafe, so common buffer, TTM, and fence objects need to
 * be protected using an external mutex.
 *
 * Note: Don't protect the following functions, as it may lead to deadlocks:
 * drmBOUnmap().
 * The kernel is synchronizing and refcounting buffer maps. 
 * User space only needs to refcount object usage within the same application.
 */


/*
 * List macros heavily inspired by the Linux kernel
 * list handling. No list looping yet.
 */

typedef struct _drmMMListHead
{
    struct _drmMMListHead *prev;
    struct _drmMMListHead *next;
} drmMMListHead;

#define DRMINITLISTHEAD(__item)		       \
  do{					       \
    (__item)->prev = (__item);		       \
    (__item)->next = (__item);		       \
  } while (0)

#define DRMLISTADD(__item, __list)		\
  do {						\
    (__item)->prev = (__list);			\
    (__item)->next = (__list)->next;		\
    (__list)->next->prev = (__item);		\
    (__list)->next = (__item);			\
  } while (0)

#define DRMLISTADDTAIL(__item, __list)		\
  do {						\
    (__item)->next = (__list);			\
    (__item)->prev = (__list)->prev;		\
    (__list)->prev->next = (__item);		\
    (__list)->prev = (__item);			\
  } while(0)

#define DRMLISTDEL(__item)			\
  do {						\
    (__item)->prev->next = (__item)->next;	\
    (__item)->next->prev = (__item)->prev;	\
  } while(0)

#define DRMLISTDELINIT(__item)			\
  do {						\
    (__item)->prev->next = (__item)->next;	\
    (__item)->next->prev = (__item)->prev;	\
    (__item)->next = (__item);			\
    (__item)->prev = (__item);			\
  } while(0)

#define DRMLISTENTRY(__type, __item, __field)   \
    ((__type *)(((char *) (__item)) - offsetof(__type, __field)))

#define DRMLISTEMPTY(__item) ((__item)->next == (__item))

#define DRMLISTFOREACHSAFE(__item, __temp, __list)			\
	for ((__item) = (__list)->next, (__temp) = (__item)->next;	\
	     (__item) != (__list);					\
	     (__item) = (__temp), (__temp) = (__item)->next)

#define DRMLISTFOREACHSAFEREVERSE(__item, __temp, __list)		\
	for ((__item) = (__list)->prev, (__temp) = (__item)->prev;	\
	     (__item) != (__list);					\
	     (__item) = (__temp), (__temp) = (__item)->prev)

typedef struct _drmFence
{
    unsigned handle;
    int fence_class;
    unsigned type; 
    unsigned flags;
    unsigned signaled;
    uint32_t sequence;
    unsigned pad[4]; /* for future expansion */
} drmFence;

typedef struct _drmBO
{
    unsigned handle;
    uint64_t mapHandle;
    uint64_t flags;
    uint64_t proposedFlags;
    unsigned mapFlags;
    unsigned long size;
    unsigned long offset;
    unsigned long start;
    unsigned replyFlags;
    unsigned fenceFlags;
    unsigned pageAlignment;
    unsigned tileInfo;
    unsigned hwTileStride;
    unsigned desiredTileStride;
    void *virtual;
    void *mapVirtual;
    int mapCount;
    unsigned pad[8];     /* for future expansion */
} drmBO;

/*
 * Fence functions.
 */

extern int drmFenceCreate(int fd, unsigned flags, int fence_class,
                          unsigned type, drmFence *fence);
extern int drmFenceReference(int fd, unsigned handle, drmFence *fence);
extern int drmFenceUnreference(int fd, const drmFence *fence);
extern int drmFenceFlush(int fd, drmFence *fence, unsigned flush_type);
extern int drmFenceSignaled(int fd, drmFence *fence, 
                            unsigned fenceType, int *signaled);
extern int drmFenceWait(int fd, unsigned flags, drmFence *fence, 
                        unsigned flush_type);
extern int drmFenceEmit(int fd, unsigned flags, drmFence *fence, 
                        unsigned emit_type);
extern int drmFenceBuffers(int fd, unsigned flags, uint32_t fence_class, drmFence *fence);


/*
 * Buffer object functions.
 */

extern int drmBOCreate(int fd, unsigned long size,
		       unsigned pageAlignment, void *user_buffer,
		       uint64_t mask, unsigned hint, drmBO *buf);
extern int drmBOReference(int fd, unsigned handle, drmBO *buf);
extern int drmBOUnreference(int fd, drmBO *buf);
extern int drmBOMap(int fd, drmBO *buf, unsigned mapFlags, unsigned mapHint,
		    void **address);
extern int drmBOUnmap(int fd, drmBO *buf);
extern int drmBOFence(int fd, drmBO *buf, unsigned flags, unsigned fenceHandle);
extern int drmBOInfo(int fd, drmBO *buf);
extern int drmBOBusy(int fd, drmBO *buf, int *busy);

extern int drmBOWaitIdle(int fd, drmBO *buf, unsigned hint);

/*
 * Initialization functions.
 */

extern int drmMMInit(int fd, unsigned long pOffset, unsigned long pSize,
		     unsigned memType);
extern int drmMMTakedown(int fd, unsigned memType);
extern int drmMMLock(int fd, unsigned memType, int lockBM, int ignoreNoEvict);
extern int drmMMUnlock(int fd, unsigned memType, int unlockBM);
extern int drmMMInfo(int fd, unsigned memType, uint64_t *size);
extern int drmBOSetStatus(int fd, drmBO *buf, 
			  uint64_t flags, uint64_t mask,
			  unsigned int hint, 
			  unsigned int desired_tile_stride,
			  unsigned int tile_info);
extern int drmBOVersion(int fd, unsigned int *major,
			unsigned int *minor,
			unsigned int *patchlevel);


#endif
