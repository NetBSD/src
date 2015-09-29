/*	$NetBSD: usb_mem.c,v 1.65.2.10 2015/09/29 11:38:29 skrll Exp $	*/

/*
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net) at
 * Carlstedt Research & Technology.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * USB DMA memory allocation.
 * We need to allocate a lot of small (many 8 byte, some larger)
 * memory blocks that can be used for DMA.  Using the bus_dma
 * routines directly would incur large overheads in space and time.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: usb_mem.c,v 1.65.2.10 2015/09/29 11:38:29 skrll Exp $");

#ifdef _KERNEL_OPT
#include "opt_usb.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/queue.h>
#include <sys/device.h>		/* for usbdivar.h */
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/once.h>

#ifdef DIAGNOSTIC
#include <sys/proc.h>
#endif

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>	/* just for usb_dma_t */
#include <dev/usb/usb_mem.h>
#include <dev/usb/usbhist.h>

#define	DPRINTF(FMT,A,B,C,D)	USBHIST_LOG(usbdebug,FMT,A,B,C,D)
#define	DPRINTFN(N,FMT,A,B,C,D)	USBHIST_LOGN(usbdebug,N,FMT,A,B,C,D)

#define USB_MEM_SMALL roundup(64, CACHE_LINE_SIZE)
#define USB_MEM_CHUNKS 64
#define USB_MEM_BLOCK (USB_MEM_SMALL * USB_MEM_CHUNKS)

/* This struct is overlayed on free fragments. */
struct usb_frag_dma {
	usb_dma_block_t		*ufd_block;
	u_int			ufd_offs;
	LIST_ENTRY(usb_frag_dma) ufd_next;
};

Static usbd_status	usb_block_allocmem(bus_dma_tag_t, size_t, size_t,
					   usb_dma_block_t **, bool);
Static void		usb_block_freemem(usb_dma_block_t *);

LIST_HEAD(usb_dma_block_qh, usb_dma_block);
Static struct usb_dma_block_qh usb_blk_freelist =
	LIST_HEAD_INITIALIZER(usb_blk_freelist);
kmutex_t usb_blk_lock;

#ifdef DEBUG
Static struct usb_dma_block_qh usb_blk_fraglist =
	LIST_HEAD_INITIALIZER(usb_blk_fraglist);
Static struct usb_dma_block_qh usb_blk_fulllist =
	LIST_HEAD_INITIALIZER(usb_blk_fulllist);
#endif
Static u_int usb_blk_nfree = 0;
/* XXX should have different free list for different tags (for speed) */
Static LIST_HEAD(, usb_frag_dma) usb_frag_freelist =
	LIST_HEAD_INITIALIZER(usb_frag_freelist);

Static int usb_mem_init(void);

Static int
usb_mem_init(void)
{

	mutex_init(&usb_blk_lock, MUTEX_DEFAULT, IPL_NONE);
	return 0;
}

Static usbd_status
usb_block_allocmem(bus_dma_tag_t tag, size_t size, size_t align,
		   usb_dma_block_t **dmap, bool multiseg)
{
	usb_dma_block_t *b;
	int error;

	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);
	DPRINTFN(5, "size=%zu align=%zu", size, align, 0, 0);

	ASSERT_SLEEPABLE();
	KASSERT(size != 0);
	KASSERT(mutex_owned(&usb_blk_lock));

	/* First check the free list. */
	LIST_FOREACH(b, &usb_blk_freelist, next) {
		/* Don't allocate multiple segments to unwilling callers */
		if (b->nsegs != 1 && !multiseg)
			continue;
		if (b->tag == tag && b->size >= size && b->align >= align) {
			LIST_REMOVE(b, next);
			usb_blk_nfree--;
			*dmap = b;
			DPRINTFN(6, "free list size=%zu", b->size, 0, 0, 0);
			return USBD_NORMAL_COMPLETION;
		}
	}

	DPRINTFN(6, "no free", 0, 0, 0, 0);
	b = kmem_zalloc(sizeof(*b), KM_SLEEP);
	if (b == NULL)
		return USBD_NOMEM;

	b->tag = tag;
	b->size = size;
	b->align = align;

	if (!multiseg)
		/* Caller wants one segment */
		b->nsegs = 1;
	else
		b->nsegs = (size + (PAGE_SIZE-1)) / PAGE_SIZE;

	b->segs = kmem_alloc(b->nsegs * sizeof(*b->segs), KM_SLEEP);
	if (b->segs == NULL) {
		kmem_free(b, sizeof(*b));
		return USBD_NOMEM;
	}
	b->nsegs_alloc = b->nsegs;

	error = bus_dmamem_alloc(tag, b->size, align, 0,
				 b->segs, b->nsegs,
				 &b->nsegs, BUS_DMA_NOWAIT);
	if (error)
		goto free0;

	error = bus_dmamem_map(tag, b->segs, b->nsegs, b->size,
			       &b->kaddr, BUS_DMA_NOWAIT|BUS_DMA_COHERENT);
	if (error)
		goto free1;

	error = bus_dmamap_create(tag, b->size, b->nsegs, b->size,
				  0, BUS_DMA_NOWAIT, &b->map);
	if (error)
		goto unmap;

	error = bus_dmamap_load(tag, b->map, b->kaddr, b->size, NULL,
				BUS_DMA_NOWAIT);
	if (error)
		goto destroy;

	*dmap = b;
#ifdef USB_FRAG_DMA_WORKAROUND
	memset(b->kaddr, 0, b->size);
#endif

	return USBD_NORMAL_COMPLETION;

 destroy:
	bus_dmamap_destroy(tag, b->map);
 unmap:
	bus_dmamem_unmap(tag, b->kaddr, b->size);
 free1:
	bus_dmamem_free(tag, b->segs, b->nsegs);
 free0:
	kmem_free(b->segs, b->nsegs_alloc * sizeof(*b->segs));
	kmem_free(b, sizeof(*b));
	return USBD_NOMEM;
}

#if 0
void
usb_block_real_freemem(usb_dma_block_t *b)
{
#ifdef DIAGNOSTIC
	if (cpu_softintr_p() || cpu_intr_p()) {
		printf("usb_block_real_freemem: in interrupt context\n");
		return;
	}
#endif
	bus_dmamap_unload(b->tag, b->map);
	bus_dmamap_destroy(b->tag, b->map);
	bus_dmamem_unmap(b->tag, b->kaddr, b->size);
	bus_dmamem_free(b->tag, b->segs, b->nsegs);
	kmem_free(b->segs, b->nsegs_alloc * sizeof(*b->segs));
	kmem_free(b, sizeof(*b));
}
#endif

#ifdef DEBUG
static bool
usb_valid_block_p(usb_dma_block_t *b, struct usb_dma_block_qh *qh)
{
	usb_dma_block_t *xb;
	LIST_FOREACH(xb, qh, next) {
		if (xb == b)
			return true;
	}
	return false;
}
#endif

/*
 * Do not free the memory unconditionally since we might be called
 * from an interrupt context and that is BAD.
 * XXX when should we really free?
 */
Static void
usb_block_freemem(usb_dma_block_t *b)
{

	KASSERT(mutex_owned(&usb_blk_lock));

	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);
	DPRINTFN(6, "size=%zu", b->size, 0, 0, 0);
#ifdef DEBUG
	LIST_REMOVE(b, next);
#endif
	LIST_INSERT_HEAD(&usb_blk_freelist, b, next);
	usb_blk_nfree++;
}

usbd_status
usb_allocmem(struct usbd_bus *bus, size_t size, size_t align, usb_dma_t *p)
{

	return usb_allocmem_flags(bus, size, align, p, 0);
}

usbd_status
usb_allocmem_flags(struct usbd_bus *bus, size_t size, size_t align, usb_dma_t *p,
		   int flags)
{
	bus_dma_tag_t tag = bus->ub_dmatag;
	usbd_status err;
	struct usb_frag_dma *f;
	usb_dma_block_t *b;
	int i;
	static ONCE_DECL(init_control);
	bool frag;

	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);

	ASSERT_SLEEPABLE();

	RUN_ONCE(&init_control, usb_mem_init);

	frag = (flags & USBMALLOC_MULTISEG);

	/* If the request is large then just use a full block. */
	if (size > USB_MEM_SMALL || align > USB_MEM_SMALL) {
		DPRINTFN(1, "large alloc %d", size, 0, 0, 0);
		size = (size + USB_MEM_BLOCK - 1) & ~(USB_MEM_BLOCK - 1);
		mutex_enter(&usb_blk_lock);
		err = usb_block_allocmem(tag, size, align, &p->udma_block, frag);
		if (!err) {
#ifdef DEBUG
			LIST_INSERT_HEAD(&usb_blk_fulllist, p->udma_block, next);
#endif
			p->udma_block->flags = USB_DMA_FULLBLOCK;
			p->udma_offs = 0;
		}
		mutex_exit(&usb_blk_lock);
		return err;
	}

	mutex_enter(&usb_blk_lock);
	/* Check for free fragments. */
	LIST_FOREACH(f, &usb_frag_freelist, ufd_next) {
		KDASSERTMSG(usb_valid_block_p(f->ufd_block, &usb_blk_fraglist),
		    "%s: usb frag %p: unknown block pointer %p",
		    __func__, f, f->ufd_block);
		if (f->ufd_block->tag == tag)
			break;
	}
	if (f == NULL) {
		DPRINTFN(1, "adding fragments", 0, 0, 0, 0);
		err = usb_block_allocmem(tag, USB_MEM_BLOCK, USB_MEM_SMALL, &b,
					 false);
		if (err) {
			mutex_exit(&usb_blk_lock);
			return err;
		}
#ifdef DEBUG
		LIST_INSERT_HEAD(&usb_blk_fraglist, b, next);
#endif
		b->flags = 0;
		for (i = 0; i < USB_MEM_BLOCK; i += USB_MEM_SMALL) {
			f = (struct usb_frag_dma *)((char *)b->kaddr + i);
			f->ufd_block = b;
			f->ufd_offs = i;
			LIST_INSERT_HEAD(&usb_frag_freelist, f, ufd_next);
#ifdef USB_FRAG_DMA_WORKAROUND
			i += 1 * USB_MEM_SMALL;
#endif
		}
		f = LIST_FIRST(&usb_frag_freelist);
	}
	p->udma_block = f->ufd_block;
	p->udma_offs = f->ufd_offs;
#ifdef USB_FRAG_DMA_WORKAROUND
	p->offs += USB_MEM_SMALL;
#endif
	LIST_REMOVE(f, ufd_next);
	mutex_exit(&usb_blk_lock);
	DPRINTFN(5, "use frag=%p size=%d", f, size, 0, 0);

	return USBD_NORMAL_COMPLETION;
}

void
usb_freemem(struct usbd_bus *bus, usb_dma_t *p)
{
	struct usb_frag_dma *f;

	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);

	mutex_enter(&usb_blk_lock);
	if (p->udma_block->flags & USB_DMA_FULLBLOCK) {
		KDASSERTMSG(usb_valid_block_p(p->udma_block, &usb_blk_fulllist),
		    "%s: dma %p: invalid block pointer %p",
		    __func__, p, p->udma_block);
		DPRINTFN(1, "large free", 0, 0, 0, 0);
		usb_block_freemem(p->udma_block);
		mutex_exit(&usb_blk_lock);
		return;
	}
	KDASSERTMSG(usb_valid_block_p(p->udma_block, &usb_blk_fraglist),
	    "%s: dma %p: invalid block pointer %p",
	    __func__, p, p->udma_block);
	//usb_syncmem(p, 0, USB_MEM_SMALL, BUS_DMASYNC_POSTREAD);
	f = KERNADDR(p, 0);
#ifdef USB_FRAG_DMA_WORKAROUND
	f = (void *)((uintptr_t)f - USB_MEM_SMALL);
#endif
	f->ufd_block = p->udma_block;
	f->ufd_offs = p->udma_offs;
#ifdef USB_FRAG_DMA_WORKAROUND
	f->offs -= USB_MEM_SMALL;
#endif
	LIST_INSERT_HEAD(&usb_frag_freelist, f, ufd_next);
	mutex_exit(&usb_blk_lock);
	DPRINTFN(5, "frag=%p", f, 0, 0, 0);
}

bus_addr_t
usb_dmaaddr(usb_dma_t *dma, unsigned int offset)
{
	unsigned int i;
	bus_size_t seg_offs;

	offset += dma->udma_offs;

	KASSERT(offset < dma->udma_block->size);

	if (dma->udma_block->nsegs == 1) {
		KASSERT(dma->udma_block->map->dm_segs[0].ds_len > offset);
		return dma->udma_block->map->dm_segs[0].ds_addr + offset;
	}

	/*
	 * Search for a bus_segment_t corresponding to this offset. With no
	 * record of the offset in the map to a particular dma_segment_t, we
	 * have to iterate from the start of the list each time. Could be
	 * improved
	 */
	seg_offs = 0;
	for (i = 0; i < dma->udma_block->nsegs; i++) {
		if (seg_offs + dma->udma_block->map->dm_segs[i].ds_len > offset)
			break;

		seg_offs += dma->udma_block->map->dm_segs[i].ds_len;
	}

	KASSERT(i != dma->udma_block->nsegs);
	offset -= seg_offs;
	return dma->udma_block->map->dm_segs[i].ds_addr + offset;
}

void
usb_syncmem(usb_dma_t *p, bus_addr_t offset, bus_size_t len, int ops)
{

	bus_dmamap_sync(p->udma_block->tag, p->udma_block->map, p->udma_offs + offset,
	    len, ops);
}
