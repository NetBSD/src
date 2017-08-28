/**
 * Copyright (c) 2010-2012 Broadcom. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer,
 *    without modification.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The names of the above-listed copyright holders may not be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * ALTERNATIVELY, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2, as published by the Free
 * Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <interface/compat/vchi_bsd.h>

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/bus.h>
#include <sys/kmem.h>

#include <linux/completion.h>

#include <uvm/uvm_extern.h>

#include <arch/arm/broadcom/bcm2835_mbox.h>
#include <arch/arm/broadcom/bcm2835var.h>

#define TOTAL_SLOTS (VCHIQ_SLOT_ZERO_SLOTS + 2 * 32)

#define VCHIQ_DOORBELL_IRQ IRQ_ARM_DOORBELL_0

#define IS_USER_ADDRESS(va)	\
	((vaddr_t)(va) >= VM_MIN_ADDRESS && (vaddr_t)(va) < VM_MAX_ADDRESS)

#include "vchiq_arm.h"
#include "vchiq_2835.h"
#include "vchiq_connected.h"

#define MAX_FRAGMENTS (VCHIQ_NUM_CURRENT_BULKS * 2)

typedef struct vchiq_2835_state_struct {
   int inited;
   VCHIQ_ARM_STATE_T arm_state;
} VCHIQ_2835_ARM_STATE_T;

/* BSD DMA */
static bus_dmamap_t dma_map;

static unsigned int g_cache_line_size = CACHE_LINE_SIZE;
static unsigned int g_fragments_size;
static char *g_fragments_base;
static char *g_free_fragments;

struct semaphore g_free_fragments_sema;
static struct semaphore g_free_fragments_mutex;


int __init
vchiq_platform_init(VCHIQ_STATE_T *state)
{
	VCHIQ_SLOT_ZERO_T *vchiq_slot_zero;
	bus_dma_segment_t dma_segs[1];
	int dma_nsegs;
	void *slot_mem;
	bus_addr_t slot_phys;
	int slot_mem_size, frag_mem_size;
	int err;
	int i;

	_sema_init(&g_free_fragments_mutex, 1);

	g_cache_line_size = 32;

	g_fragments_size = 2 * g_cache_line_size;

	/* Allocate space for the channels in coherent memory */
	slot_mem_size = PAGE_ALIGN(TOTAL_SLOTS * VCHIQ_SLOT_SIZE);
	frag_mem_size = PAGE_ALIGN(g_fragments_size * MAX_FRAGMENTS);

	dma_nsegs = __arraycount(dma_segs);
	err = bus_dmamem_alloc(&bcm2835_bus_dma_tag,
	    slot_mem_size + frag_mem_size, PAGE_SIZE, 0,
	    dma_segs, dma_nsegs, &dma_nsegs, BUS_DMA_WAITOK);
	if (err) {
		vchiq_log_error(vchiq_core_log_level, "Unable to allocate channel memory");
		err = -ENOMEM;
		goto failed_alloc;
	}

	err = bus_dmamem_map(&bcm2835_bus_dma_tag,
	    dma_segs, dma_nsegs, slot_mem_size + frag_mem_size,
	    (void **)&slot_mem, BUS_DMA_COHERENT | BUS_DMA_WAITOK);
	if (err) {
		vchiq_log_error(vchiq_core_log_level, "Unable to map channel memory");
		err = -ENOMEM;
		goto failed_alloc;
	}

	err = bus_dmamap_create(&bcm2835_bus_dma_tag,
	    slot_mem_size + frag_mem_size, 1,	/* maxsize, nsegments */
	    slot_mem_size + frag_mem_size, 0,	/* maxsegsize, boundary */
	    BUS_DMA_WAITOK,
	    &dma_map);
	if (err) {
		vchiq_log_error(vchiq_core_log_level, "Unable to create DMA map");
		err = -ENOMEM;
		goto failed_alloc;
	}

	err = bus_dmamap_load(&bcm2835_bus_dma_tag, dma_map, slot_mem,
	    slot_mem_size + frag_mem_size, NULL, BUS_DMA_WAITOK);
	if (err) {
		vchiq_log_error(vchiq_core_log_level, "cannot load DMA map (%d)", err);
		err = -ENOMEM;
		goto failed_load;
	}
	slot_phys = dma_map->dm_segs[0].ds_addr;

	vchiq_log_info(vchiq_arm_log_level,
	    "%s: slot_phys = %lx\n", __func__, slot_phys);

	WARN_ON(((int)slot_mem & (PAGE_SIZE - 1)) != 0);

	vchiq_slot_zero = vchiq_init_slots(slot_mem, slot_mem_size);
	if (!vchiq_slot_zero) {
		err = -EINVAL;
		goto failed_init_slots;
	}

	vchiq_slot_zero->platform_data[VCHIQ_PLATFORM_FRAGMENTS_OFFSET_IDX] =
		(int)slot_phys + slot_mem_size;
	vchiq_slot_zero->platform_data[VCHIQ_PLATFORM_FRAGMENTS_COUNT_IDX] =
		MAX_FRAGMENTS;

	g_fragments_base = (char *)slot_mem + slot_mem_size;
	slot_mem_size += frag_mem_size;

	g_free_fragments = g_fragments_base;
	for (i = 0; i < (MAX_FRAGMENTS - 1); i++) {
		*(char **)&g_fragments_base[i*g_fragments_size] =
			&g_fragments_base[(i + 1)*g_fragments_size];
	}
	*(char **)&g_fragments_base[i * g_fragments_size] = NULL;

	_sema_init(&g_free_fragments_sema, MAX_FRAGMENTS);

	if (vchiq_init_state(state, vchiq_slot_zero, 0/*slave*/) !=
		VCHIQ_SUCCESS) {
		err = -EINVAL;
		goto failed_vchiq_init;
	}

	/* Send the base address of the slots to VideoCore */
	dsb(); /* Ensure all writes have completed */

	bus_dmamap_sync(&bcm2835_bus_dma_tag, dma_map, 0, slot_mem_size,
	    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);
	bcm_mbox_write(BCM2835_MBOX_CHAN_VCHIQ, (unsigned int)slot_phys);
	bus_dmamap_sync(&bcm2835_bus_dma_tag, dma_map, 0, slot_mem_size,
	    BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);

	vchiq_log_info(vchiq_arm_log_level,
		"vchiq_init - done (slots %x, phys %x)",
		(unsigned int)vchiq_slot_zero, (unsigned int)slot_phys);

	vchiq_call_connected_callbacks();

   return 0;

failed_vchiq_init:
failed_init_slots:
failed_load:
	bus_dmamap_unload(&bcm2835_bus_dma_tag, dma_map);
failed_alloc:
	bus_dmamap_destroy(&bcm2835_bus_dma_tag, dma_map);

   return err;
}

void __exit
vchiq_platform_exit(VCHIQ_STATE_T *state)
{

	bus_dmamap_unload(&bcm2835_bus_dma_tag, dma_map);
	bus_dmamap_destroy(&bcm2835_bus_dma_tag, dma_map);
}


VCHIQ_STATUS_T
vchiq_platform_init_state(VCHIQ_STATE_T *state)
{
   VCHIQ_STATUS_T status = VCHIQ_SUCCESS;
   state->platform_state = kzalloc(sizeof(VCHIQ_2835_ARM_STATE_T), GFP_KERNEL);
   ((VCHIQ_2835_ARM_STATE_T*)state->platform_state)->inited = 1;
   status = vchiq_arm_init_state(state, &((VCHIQ_2835_ARM_STATE_T*)state->platform_state)->arm_state);
   if(status != VCHIQ_SUCCESS)
   {
      ((VCHIQ_2835_ARM_STATE_T*)state->platform_state)->inited = 0;
   }
   return status;
}

VCHIQ_ARM_STATE_T*
vchiq_platform_get_arm_state(VCHIQ_STATE_T *state)
{
   if(!((VCHIQ_2835_ARM_STATE_T*)state->platform_state)->inited)
   {
      BUG();
   }
   return &((VCHIQ_2835_ARM_STATE_T*)state->platform_state)->arm_state;
}

int
vchiq_copy_from_user(void *dst, const void *src, int size)
{
	vaddr_t va = (vaddr_t)src;

	if (IS_USER_ADDRESS(va)) {
		int error = copyin(src, dst, size);
		return error ? VCHIQ_ERROR : VCHIQ_SUCCESS;
	} else {
		kcopy(src, dst, size);
		return VCHIQ_SUCCESS;
	}
}

typedef struct bulkinfo_struct {
	void		*pagelist;
	bus_dma_segment_t pagelist_sgs[1];
	bus_size_t	pagelist_size;
	bus_dmamap_t	pagelist_map;
	bus_dmamap_t	dmamap;
	struct proc	*proc;
	void		*buf;
	int		size;
} BULKINFO_T;

/* There is a potential problem with partial cache lines (pages?)
** at the ends of the block when reading. If the CPU accessed anything in
** the same line (page?) then it may have pulled old data into the cache,
** obscuring the new data underneath. We can solve this by transferring the
** partial cache lines separately, and allowing the ARM to copy into the
** cached area.
*/
VCHIQ_STATUS_T
vchiq_prepare_bulk_data(VCHIQ_BULK_T *bulk, VCHI_MEM_HANDLE_T memhandle,
	void *buf, int size, int dir)
{
	PAGELIST_T *pagelist;
	BULKINFO_T *bi;
	int nsegs;
	int ret;

	vchiq_log_info(vchiq_arm_log_level,
	    "%s: buf %p size %08x dir %s", __func__, buf, size,
	    dir == VCHIQ_BULK_RECEIVE ? "read" : "write");

	vaddr_t va = (vaddr_t)buf;
	const size_t maxsegs = atop(round_page(va + size) - trunc_page(va));
 	const int uvmflags = (dir == VCHIQ_BULK_RECEIVE ?
	    VM_PROT_READ : VM_PROT_WRITE);
	const int dmaflags = (dir == VCHIQ_BULK_RECEIVE ?
	    BUS_DMA_READ : BUS_DMA_WRITE);

	WARN_ON(memhandle != VCHI_MEM_HANDLE_INVALID);

	bi = kmem_alloc(sizeof(*bi), KM_SLEEP);
	bi->buf = buf;
	bi->size = size;
	bi->pagelist_size = sizeof(PAGELIST_T) +
	    (maxsegs * sizeof(unsigned long));
	bi->proc = curproc;

	ret = bus_dmamem_alloc(&bcm2835_bus_dma_tag, bi->pagelist_size,
	    0 /*CACHE_LINE_SIZE*/, 0, bi->pagelist_sgs,
	    __arraycount(bi->pagelist_sgs), &nsegs, BUS_DMA_WAITOK);

	if (ret != 0)
		goto fail1;

	ret = bus_dmamem_map(&bcm2835_bus_dma_tag, bi->pagelist_sgs, nsegs,
	    bi->pagelist_size, &bi->pagelist, BUS_DMA_COHERENT | BUS_DMA_WAITOK);
	if (ret != 0)
		goto fail2;

	pagelist = bi->pagelist;

	ret = bus_dmamap_create(&bcm2835_bus_dma_tag, bi->pagelist_size,
	    nsegs, bi->pagelist_size, 0, BUS_DMA_WAITOK, &bi->pagelist_map);
	if (ret != 0)
		goto fail3;

	ret = bus_dmamap_load(&bcm2835_bus_dma_tag, bi->pagelist_map, pagelist,
	    bi->pagelist_size, NULL, BUS_DMA_WAITOK | BUS_DMA_WRITE);
	if (ret != 0)
		goto fail4;

	/*
	 * Need to wire the buffer pages in.
	 */
	if (IS_USER_ADDRESS(buf)) {
		ret = uvm_vslock(bi->proc->p_vmspace, buf, size, uvmflags);
		if (ret != 0) {
			printf("%s: uvm_vslock failed (%d)\n", __func__, ret);
			goto fail5;
		}
	}

	ret = bus_dmamap_create(&bcm2835_bus_dma_tag, size, maxsegs, size, 0,
	    BUS_DMA_WAITOK, &bi->dmamap);

	if (ret != 0)
		goto fail6;

	ret = bus_dmamap_load(&bcm2835_bus_dma_tag, bi->dmamap, buf, size,
	    curproc, BUS_DMA_WAITOK | dmaflags);

	if (ret != 0)
		goto fail7;

	bulk->handle = memhandle;
	/*
	 * We've now got the bus_addr_t for the pagelist we want the transfer
	 * to use.
	 */
	bulk->data = (void *)bi->pagelist_map->dm_segs[0].ds_addr;

	pagelist->type = (dir == VCHIQ_BULK_RECEIVE) ?
	    PAGELIST_READ : PAGELIST_WRITE;
	pagelist->length = size;
	pagelist->offset = va & L2_S_OFFSET;

	/*
	 * busdma already coalesces contiguous pages for us
	 */
	for (int i = 0; i < bi->dmamap->dm_nsegs; i++) {
		bus_addr_t addr = bi->dmamap->dm_segs[i].ds_addr;
		bus_size_t len = bi->dmamap->dm_segs[i].ds_len;
		bus_size_t off = addr & L2_S_OFFSET;
		int npgs = ((off + len + L2_S_OFFSET) >> L2_S_SHIFT);

		pagelist->addrs[i] = addr & ~L2_S_OFFSET;
		pagelist->addrs[i] |= npgs - 1;
	}

	/* Partial cache lines (fragments) require special measures */
	if ((pagelist->type == PAGELIST_READ) &&
	    ((pagelist->offset & (g_cache_line_size - 1)) ||
	    ((pagelist->offset + pagelist->length) & (g_cache_line_size - 1)))) {
		char *fragments;

		if (down_interruptible(&g_free_fragments_sema) != 0) {
			goto fail7;
		}

		WARN_ON(g_free_fragments == NULL);

		down(&g_free_fragments_mutex);
		fragments = g_free_fragments;
		WARN_ON(fragments == NULL);
		g_free_fragments = *(char **) g_free_fragments;
		up(&g_free_fragments_mutex);
		pagelist->type = PAGELIST_READ_WITH_FRAGMENTS +
		    (fragments - g_fragments_base) / g_fragments_size;
		bus_dmamap_sync(&bcm2835_bus_dma_tag, dma_map,
		    (char *)fragments - g_fragments_base, sizeof(*fragments),
		    BUS_DMASYNC_PREREAD);
	}

	/*
	 * Store the BULKINFO_T address in remote_data, which isn't used by the
	 * slave.
	 */
	bulk->remote_data = bi;

	bus_dmamap_sync(&bcm2835_bus_dma_tag, bi->pagelist_map, 0,
	    bi->pagelist_size, BUS_DMASYNC_PREWRITE);

	bus_dmamap_sync(&bcm2835_bus_dma_tag, bi->dmamap, 0, bi->size,
	    pagelist->type == PAGELIST_WRITE ?
	    BUS_DMASYNC_PREWRITE : BUS_DMASYNC_PREREAD);

	return VCHIQ_SUCCESS;

fail7:
	bus_dmamap_destroy(&bcm2835_bus_dma_tag, bi->dmamap);

fail6:
	if (IS_USER_ADDRESS(bi->buf))
		uvm_vsunlock(curproc->p_vmspace, bi->buf, bi->size);

fail5:
	bus_dmamap_unload(&bcm2835_bus_dma_tag, bi->pagelist_map);

fail4:
	bus_dmamap_destroy(&bcm2835_bus_dma_tag, bi->pagelist_map);

fail3:
	bus_dmamem_unmap(&bcm2835_bus_dma_tag, bi->pagelist, bi->pagelist_size);

fail2:
	bus_dmamem_free(&bcm2835_bus_dma_tag, bi->pagelist_sgs,
	    __arraycount(bi->pagelist_sgs));

fail1:
	kmem_free(bi, sizeof(*bi));
	return VCHIQ_ERROR;
}

void
vchiq_complete_bulk(VCHIQ_BULK_T *bulk)
{
	if (bulk && bulk->remote_data && bulk->actual) {
		int actual = bulk->actual;
		BULKINFO_T *bi = bulk->remote_data;
		PAGELIST_T *pagelist = bi->pagelist;

		vchiq_log_trace(vchiq_arm_log_level,
			"free_pagelist - %x, %d", (unsigned int)pagelist, actual);

		bus_dmamap_sync(&bcm2835_bus_dma_tag, bi->pagelist_map, 0,
		    bi->pagelist_size, BUS_DMASYNC_POSTWRITE);

		bus_dmamap_sync(&bcm2835_bus_dma_tag, bi->dmamap, 0, bi->size,
		    pagelist->type == PAGELIST_WRITE ?
		    BUS_DMASYNC_POSTWRITE : BUS_DMASYNC_POSTREAD);

		/* Deal with any partial cache lines (fragments) */
		if (pagelist->type >= PAGELIST_READ_WITH_FRAGMENTS) {
			char *fragments = g_fragments_base +
			    (pagelist->type - PAGELIST_READ_WITH_FRAGMENTS) *
			    g_fragments_size;
			int head_bytes, tail_bytes;

			bus_dmamap_sync(&bcm2835_bus_dma_tag, dma_map,
			    (char *)fragments - g_fragments_base, g_fragments_size,
			    BUS_DMASYNC_POSTREAD);

			head_bytes = (g_cache_line_size - pagelist->offset) &
				(g_cache_line_size - 1);
			tail_bytes = (pagelist->offset + actual) &
				(g_cache_line_size - 1);

			if ((actual >= 0) && (head_bytes != 0)) {
				if (head_bytes > actual)
					head_bytes = actual;

				if (IS_USER_ADDRESS(bi->buf)) {
					copyout_proc(bi->proc, fragments,
					    bi->buf, head_bytes);
				} else {
					kcopy(fragments, bi->buf, head_bytes);
				}
			}
			if ((actual >= 0) && (head_bytes < actual) &&
			    (tail_bytes != 0)) {
				void *t = (char *)bi->buf + bi->size -
				    tail_bytes;

				 if (IS_USER_ADDRESS(bi->buf)) {
					copyout_proc(bi->proc,
					    fragments + g_cache_line_size, t,
					    tail_bytes);
				} else {
					kcopy(fragments + g_cache_line_size, t,
					    tail_bytes);
				}
			}

			down(&g_free_fragments_mutex);
			*(char **)fragments = g_free_fragments;
			g_free_fragments = fragments;
			up(&g_free_fragments_mutex);
			up(&g_free_fragments_sema);
		}
		bus_dmamap_unload(&bcm2835_bus_dma_tag, bi->dmamap);
		bus_dmamap_destroy(&bcm2835_bus_dma_tag, bi->dmamap);
		if (IS_USER_ADDRESS(bi->buf))
			uvm_vsunlock(bi->proc->p_vmspace, bi->buf, bi->size);

		bus_dmamap_unload(&bcm2835_bus_dma_tag, bi->pagelist_map);
		bus_dmamap_destroy(&bcm2835_bus_dma_tag, bi->pagelist_map);
		bus_dmamem_unmap(&bcm2835_bus_dma_tag, bi->pagelist,
		    bi->pagelist_size);
		bus_dmamem_free(&bcm2835_bus_dma_tag, bi->pagelist_sgs,
		    __arraycount(bi->pagelist_sgs));
		kmem_free(bi, sizeof(*bi));
	}
}

void
vchiq_transfer_bulk(VCHIQ_BULK_T *bulk)
{
	/*
	 * This should only be called on the master (VideoCore) side, but
	 * provide an implementation to avoid the need for ifdefery.
	 */
	BUG();
}

void
vchiq_dump_platform_state(void *dump_context)
{
	char buf[80];
	int len;
	len = snprintf(buf, sizeof(buf),
		"  Platform: 2835 (VC master)");
	vchiq_dump(dump_context, buf, len + 1);
}

VCHIQ_STATUS_T
vchiq_platform_suspend(VCHIQ_STATE_T *state)
{
   return VCHIQ_ERROR;
}

VCHIQ_STATUS_T
vchiq_platform_resume(VCHIQ_STATE_T *state)
{
   return VCHIQ_SUCCESS;
}

void
vchiq_platform_paused(VCHIQ_STATE_T *state)
{
}

void
vchiq_platform_resumed(VCHIQ_STATE_T *state)
{
}

int
vchiq_platform_videocore_wanted(VCHIQ_STATE_T* state)
{
   return 1; // autosuspend not supported - videocore always wanted
}

int
vchiq_platform_use_suspend_timer(void)
{
   return 0;
}
void
vchiq_dump_platform_use_state(VCHIQ_STATE_T *state)
{
	vchiq_log_info(vchiq_arm_log_level, "Suspend timer not in use");
}
void
vchiq_platform_handle_timeout(VCHIQ_STATE_T *state)
{
	(void)state;
}
