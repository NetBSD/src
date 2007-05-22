/*	$NetBSD: usb_mem.c,v 1.31.10.1 2007/05/22 14:57:47 itohy Exp $	*/
/*	$FreeBSD: src/sys/dev/usb/usb_mem.c,v 1.10 2006/09/06 23:44:24 imp Exp $	*/

/*-
 * Copyright (c) 1998, 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net) at
 * Carlstedt Research & Technology.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by ITOH Yasufumi (itohy@NetBSD.org).
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: usb_mem.c,v 1.31.10.1 2007/05/22 14:57:47 itohy Exp $");
/* __FBSDID("$FreeBSD: src/sys/dev/usb/usb_mem.c,v 1.10 2006/09/06 23:44:24 imp Exp $"); */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#if defined(__NetBSD__) || defined(__OpenBSD__)
#include <sys/device.h>		/* for usbdivar.h */
#include <machine/bus.h>
#elif defined(__FreeBSD__)
#include <sys/endian.h>
#include <sys/module.h>
#include <sys/bus.h>
#endif
#include <sys/queue.h>

#include <machine/bus.h>
#include <machine/endian.h>

#ifdef __NetBSD__
#include <sys/extent.h>
#include <uvm/uvm_extern.h>
#endif

#ifdef DIAGNOSTIC
#include <sys/proc.h>
#endif

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#define USBMEM_PRIVATE
#include <dev/usb/usb_mem.h>

#ifdef USBMEM_DEBUG
# define USBMEM_DEBUGPARAMS	, const char *filename, int lineno
# define USBMEM_DEBUGARGS	, filename, lineno
#endif

#ifdef USB_DEBUG
#define DPRINTF(x)	if (usbdebug) logprintf x
#define DPRINTFN(n,x)	if (usbdebug>(n)) logprintf x
extern int usbdebug;
#else
#define DPRINTF(x)
#define DPRINTFN(n,x)
#endif

#define USB_MEM_SMALL 64
#define USB_MEM_CHUNKS (PAGE_SIZE / USB_MEM_SMALL)
#define USB_MEM_BLOCK (USB_MEM_SMALL * USB_MEM_CHUNKS)

/* This struct is overlayed on free fragments. */
struct usb_frag_dma {
	usb_dma_block_t *block;
	u_int offs;
	LIST_ENTRY(usb_frag_dma) next;
};

#ifdef __FreeBSD__
Static bus_dmamap_callback_t usbmem_callback, usb_alloc_buffer_dma_callback,
			     usb_map_dma_callback;
#endif
Static usbd_status	usb_block_allocmem(usb_dma_tag_t *, size_t, size_t,
					   usb_dma_block_t **);
Static void		usb_block_freemem(usb_dma_tag_t *, usb_dma_block_t *);
Static void		usb_dma_free_dmamem(usb_dma_tag_t *,
					     struct usb_dmamem *);
Static void		usb_dma_free_dmamems(void *);

#ifdef __FreeBSD__
Static void
usbmem_callback(void *arg, bus_dma_segment_t *segs, int nseg, int error)
{
	int i;
	usb_dma_block_t *p = arg;

	if (error == EFBIG) {
		printf("usb: mapping too large\n");
		return;
	}

	p->nsegs = nseg;
	for (i = 0; i < nseg && i < sizeof p->segs / sizeof *p->segs; i++)
		p->segs[i] = segs[i];
}
#endif	/* __FreeBSD__ */

Static usbd_status
usb_block_allocmem(usb_dma_tag_t *utag, size_t size, size_t align,
		   usb_dma_block_t **dmap)
{
	bus_dma_tag_t tag;
	usb_dma_block_t *p;
	int s;

	DPRINTFN(5, ("usb_block_allocmem: size=%lu align=%lu\n",
		     (u_long)size, (u_long)align));

#ifdef DIAGNOSTIC
	if (!curproc) {
		panic("usb_block_allocmem: in interrupt context, size=%lu\n",
		    (unsigned long) size);
	}
#endif

	s = splusb();
	/* First check the free list. */
	for (p = LIST_FIRST(&utag->blk_freelist); p; p = LIST_NEXT(p, next)) {
		if (p->size >= size && p->size < size * 2 &&
		    p->align >= align) {
			LIST_REMOVE(p, next);
			utag->blk_nfree--;
			splx(s);
			*dmap = p;
			DPRINTFN(6,("usb_block_allocmem: free list size=%lu\n",
				    (u_long)p->size));
			return (USBD_NORMAL_COMPLETION);
		}
	}
	splx(s);

	DPRINTFN(6, ("usb_block_allocmem: no free\n"));
	p = malloc(sizeof *p, M_USB, M_WAITOK);
	if (p == NULL)
		return (USBD_NOMEM);

#if defined(__NetBSD__) || defined(__OpenBSD__)
	tag = utag->tag;
	p->size = size;
	p->align = align;
	if (bus_dmamem_alloc(tag, p->size, align, 0,
			     p->segs, sizeof(p->segs)/sizeof(p->segs[0]),
			     &p->nsegs, BUS_DMA_WAITOK))
		goto free0;

	if (bus_dmamem_map(tag, p->segs, p->nsegs, p->size,
			   &p->kaddr, BUS_DMA_WAITOK|BUS_DMA_COHERENT))
		goto free1;

	if (bus_dmamap_create(tag, p->size, 1, p->size,
			      0, BUS_DMA_WAITOK, &p->map))
		goto unmap;

	if (bus_dmamap_load(tag, p->map, p->kaddr, p->size, NULL,
			    BUS_DMA_WAITOK))
		goto destroy;

	*dmap = p;
	utag->nblks++;
	return (USBD_NORMAL_COMPLETION);

destroy:
	bus_dmamap_destroy(tag, p->map);
unmap:
	bus_dmamem_unmap(tag, p->kaddr, p->size);
free1:
	bus_dmamem_free(tag, p->segs, p->nsegs);
free0:
	free(p, M_USB);
	return (USBD_NOMEM);
#endif	/* defined(__NetBSD__) || defined(__OpenBSD__) */

#ifdef __FreeBSD__
	tag = utag->tag_parent;
#if __FreeBSD_version >= 500000
	if (bus_dma_tag_create(tag, align ? align : 1, 0,
	    BUS_SPACE_MAXADDR_32BIT, BUS_SPACE_MAXADDR, NULL, NULL,
	    size, sizeof(p->segs) / sizeof(p->segs[0]), size,
	    0, NULL, NULL, &p->tag) == ENOMEM)
#else
	if (bus_dma_tag_create(tag, align ? align : 1, 0,
	    BUS_SPACE_MAXADDR_32BIT, BUS_SPACE_MAXADDR, NULL, NULL,
	    size, sizeof(p->segs) / sizeof(p->segs[0]), size,
	    0, &p->tag) == ENOMEM)
#endif
	{
		goto free;
	}

	p->size = size;
	p->align = align;
	if (bus_dmamem_alloc(p->tag, (void **)&p->kaddr,
	    BUS_DMA_WAITOK | BUS_DMA_COHERENT, &p->map))
		goto tagfree;

	if (bus_dmamap_load(p->tag, p->map, (void *)p->kaddr, p->size,
	    usbmem_callback, p, 0))
		goto memfree;

	*dmap = p;
	utag->nblks++;
	return (USBD_NORMAL_COMPLETION);

memfree:
	bus_dmamem_free(p->tag, (void *)p->kaddr, p->map);
tagfree:
	bus_dma_tag_destroy(p->tag);
free:
	free(p, M_USB);
	return (USBD_NOMEM);
#endif /* __FreeBSD__ */
}

/*
 * Do not free the memory unconditionally since we might be called
 * from an interrupt context and that is BAD.
 * XXX when should we really free?
 */
Static void
usb_block_freemem(usb_dma_tag_t *utag, usb_dma_block_t *p)
{
	int s;

	DPRINTFN(6, ("usb_block_freemem: size=%lu\n", (u_long)p->size));
	s = splusb();
	LIST_INSERT_HEAD(&utag->blk_freelist, p, next);
	utag->blk_nfree++;
	splx(s);
}

usbd_status
usb_allocmem(usb_dma_tag_t *utag, size_t size, size_t align, usb_dma_t *p
	USBMEM_DEBUGPARAMS)
{
	usbd_status err;
	struct usb_frag_dma *f;
	usb_dma_block_t *b;
	int i;
	int s;
#ifdef USBMEM_DEBUG
	usb_dma_block_t *bd;
	struct usb_frag_debug *fd;
#endif

	/* If the request is large then just use a full block. */
	if (size > USB_MEM_SMALL || align > USB_MEM_SMALL) {
		DPRINTFN(1, ("usb_allocmem: large alloc %d\n", (int)size));
		size = (size + USB_MEM_BLOCK - 1) & ~(USB_MEM_BLOCK - 1);
		err = usb_block_allocmem(utag, size, align, &p->block);
		if (!err) {
			p->block->flags = USB_DMA_FULLBLOCK;
			p->offs = 0;
			p->len = size;
#ifdef USBMEM_DEBUG
			TAILQ_FOREACH(bd, &utag->blks_lease, b_debug_next) {
				if (bd == p->block) {
					printf("%s: block %p already in lease %s:%d\n",
					    utag->devname, bd,
					    bd->b_fn, bd->b_ln);
					TAILQ_REMOVE(&utag->blks_lease, bd, b_debug_next);
					break;
				}
			}
			p->block->b_fn = filename;
			p->block->b_ln = lineno;
			TAILQ_INSERT_HEAD(&utag->blks_lease, p->block, b_debug_next);
#endif
		}
		return (err);
	}

	s = splusb();
	/* Check for free fragments. */
	if ((f = LIST_FIRST(&utag->frag_freelist)) == NULL) {
		DPRINTFN(1, ("usb_allocmem: adding fragments\n"));
		err = usb_block_allocmem(utag, USB_MEM_BLOCK, USB_MEM_SMALL,&b);
		if (err) {
			splx(s);
			return (err);
		}
		b->flags = 0;
		USB_KASSERT2(sizeof *f <= USB_MEM_SMALL,
		    ("USB_MEM_SMALL(%d) is too small for struct usb_frag_dma(%zd)\n",
		    USB_MEM_SMALL, sizeof *f));
		for (i = 0; i < USB_MEM_BLOCK; i += USB_MEM_SMALL) {
			f = (struct usb_frag_dma *)(b->kaddr + i);
			f->block = b;
			f->offs = i;
			LIST_INSERT_HEAD(&utag->frag_freelist, f, next);
			utag->nfrags++;
		}
		f = LIST_FIRST(&utag->frag_freelist);
	}
	p->block = f->block;
	p->offs = f->offs;
	p->len = USB_MEM_SMALL;
	LIST_REMOVE(f, next);
#ifdef USBMEM_DEBUG
	if ((fd = TAILQ_FIRST(&utag->free_frag_debug)) == NULL) {
		printf("%s: USBMEM_DEBUG frag reached %d, debugging will be inaccurate\n",
		    utag->devname, USBMEM_DEBUG_NFRAG);
	} else {
		TAILQ_REMOVE(&utag->free_frag_debug, fd, f_debug_next);
		fd->f_fn = filename;
		fd->f_ln = lineno;
		fd->f_frag = f;
		TAILQ_INSERT_HEAD(&utag->frags_lease, fd, f_debug_next);
		if (++utag->frag_debug_used > utag->frag_debug_used_max)
			utag->frag_debug_used_max = utag->frag_debug_used;
	}
#endif
	splx(s);
	DPRINTFN(5, ("usb_allocmem: use frag=%p size=%d\n", f, (int)size));
	return (USBD_NORMAL_COMPLETION);
}

void
usb_freemem(usb_dma_tag_t *utag, usb_dma_t *p
	USBMEM_DEBUGPARAMS)
{
	struct usb_frag_dma *f;
	int s;
#ifdef USBMEM_DEBUG
	struct usb_dma_block *bd;
	struct usb_frag_debug *fd;
#endif

	if (p->block->flags & USB_DMA_FULLBLOCK) {
		DPRINTFN(1, ("usb_freemem: large free\n"));
#ifdef USBMEM_DEBUG
		TAILQ_FOREACH(bd, &utag->blks_lease, b_debug_next) {
			if (bd == p->block)
				break;
		}
		if (bd == NULL) {
			printf("%s: freeing wrong block %p %s:%d, possibly freed at %s:%d\n",
			    utag->devname, p->block, filename, lineno,
			    p->block->b_fn, p->block->b_ln);
		} else {
			TAILQ_REMOVE(&utag->blks_lease, bd, b_debug_next);
		}
		p->block->b_fn = filename;
		p->block->b_ln = lineno;
#endif
		usb_block_freemem(utag, p->block);
		return;
	}
	f = KERNADDR(p, 0);
#ifdef USBMEM_DEBUG
	TAILQ_FOREACH(fd, &utag->frags_lease, f_debug_next) {
		if (fd->f_frag == f)
			break;
	}
	if (fd == NULL) {
		printf("%s: freeing wrong frag %p %s:%d\n",
		    utag->devname, f, filename, lineno);
		TAILQ_FOREACH_REVERSE(fd, &utag->free_frag_debug,
		    usb_debug_frag_head, f_debug_next) {
			if (fd->f_frag == f) {
				printf("%s: it was already freed at %s:%d\n",
				    utag->devname, fd->f_fn, fd->f_ln);
				break;
			}
		}
	} else {
		TAILQ_REMOVE(&utag->frags_lease, fd, f_debug_next);
		--utag->frag_debug_used;
		fd->f_fn = filename;
		fd->f_ln = lineno;
		TAILQ_INSERT_TAIL(&utag->free_frag_debug, fd, f_debug_next);
	}
#endif
	f->block = p->block;
	f->offs = p->offs;
	s = splusb();
	LIST_INSERT_HEAD(&utag->frag_freelist, f, next);
	splx(s);
	DPRINTFN(5, ("usb_freemem: frag=%p\n", f));
}

/*
 * Support for mapping/allocating DMA buffer
 */

#define PUTUD(utag, ud)	SIMPLEQ_INSERT_HEAD(&(utag)->uds_idle, (ud), ud_next)

Static void
usb_dma_free_dmamem(usb_dma_tag_t *utag, struct usb_dmamem *ud)
{

	/*
	 * Free DMA buffer
	 */
#if defined(__NetBSD__) || defined(__OpenBSD__)
	bus_dmamem_unmap(utag->tag, ud->ud_kaddr, ud->ud_len);
	bus_dmamem_free(utag->tag, ud->ud_segs, ud->ud_nsegs);
#elif defined(__FreeBSD__)
	bus_dmamem_free(ud->ud_tag, ud->ud_kaddr, ud->ud_map);
	bus_dma_tag_destroy(ud->ud_tag);
#endif

	/*
	 * Free DMA memory structure
	 */
	PUTUD(utag, ud);
}

Static void
usb_dma_free_dmamems(void *arg)
{
	usb_dma_tag_t *utag = arg;
	struct usb_dmamem *ud, *ud1;
	int s;

#if 0
	s = splusb();
	while ((ud = SIMPLEQ_FIRST(&utag->uds_tobefreed)) != NULL) {
		SIMPLEQ_REMOVE_HEAD(&utag->uds_tobefreed, ud_next);
		splx(s);
		usb_dma_free_dmamem(utag, ud);
	}
	splx(s);
#else	/* hack to reduce splusb() protection */
	s = splusb();
	if ((ud = SIMPLEQ_FIRST(&utag->uds_tobefreed)) != NULL)
		SIMPLEQ_INIT(&utag->uds_tobefreed);
	splx(s);
	for ( ; ud; ud = ud1) {
		ud1 = SIMPLEQ_NEXT(ud, ud_next);
		usb_dma_free_dmamem(utag, ud);
	}
#endif
}

#ifdef __FreeBSD__
Static void
usb_alloc_buffer_dma_callback(void *arg, bus_dma_segment_t *segs, int nseg,
	int error)
{
	struct usb_buffer_dma *ub = arg;

	ub->ub_error = error;
	if (error == 0) {
		if (segs != ub->ub_nsegs) {
			for (i = 0; i < nseg; i++)
				ub->ub_segs[i] = segs[i];
		}
		ub->ub_nsegs = nseg;
	}

	wakeup(ub);
}
#endif /* __FreeBSD__ */

/*
 * Map or allocate buffer
 */
usbd_status
usb_alloc_buffer_dma(usb_dma_tag_t *utag,
	struct usb_buffer_dma *ub,
	void *buf,			/* allocate new buffer if NULL */
	size_t size,
	void **pbufadr	/* (out) kernel address of allocated or mapped buffer */
	USBMEM_DEBUGPARAMS)
{
	struct usb_dmamem *ud = NULL;
	int err;
#ifdef USBMEM_DEBUG
	struct usb_dmamem *dd;
#endif

#if 1
	if (!SIMPLEQ_EMPTY(&utag->uds_tobefreed))
		usb_dma_free_dmamems(utag);
#endif

#if 1 /*def DIAGNOSTIC*/
	if (ub->ub_type != UB_NONE)
		panic("usb_alloc_buffer_dma: ub_type == %d", ub->ub_type);
#endif

	if (size == 0) {
		DPRINTFN(5, ("usb_alloc_buffer_dma: type=UB_NONE ub=%p size=0\n",
			     ub));
		return (USBD_NORMAL_COMPLETION);
	}

	if (buf == NULL) {
		/*
		 * allocate buffer
		 */

		/* Use generic USB buffer for small transfer. */
		if (size <= USB_MEM_SMALL /* XXX tunable parameter */
		    && usb_allocmem(utag, size, 0, &ub->ub_allocbuf
				    USBMEM_DEBUGARGS) ==
		    USBD_NORMAL_COMPLETION) {
#if defined(__NetBSD__) || defined(__OpenBSD__)
			ub->ub_bufseg.ds_addr = DMAADDR(&ub->ub_allocbuf, 0);
			ub->ub_bufseg.ds_len  = size;
#endif
#ifdef __FreeBSD__
			ub->ub_nsegs = 1;
			ub->ub_segs[0].ds_addr = DMAADDR(&ub->ub_allocbuf, 0);
			ub->ub_segs[0].ds_len  = size;
#endif
			ub->ub_type = UB_BUF;

			/* kernel address of allocated or mapped buffer */
			*pbufadr = KERNADDR(&ub->ub_allocbuf, 0);

			DPRINTFN(5, ("usb_alloc_buffer_dma: type=UB_BUF ub=%p size=%lu\n",
				     ub, (u_long)size));

			return (USBD_NORMAL_COMPLETION);
		}


		/* get dmamap structure */
		if ((ud = SIMPLEQ_FIRST(&utag->uds_idle)) != NULL) {
			SIMPLEQ_REMOVE_HEAD(&utag->uds_idle, ud_next);
		} else {
			if ((ud = malloc(sizeof *ub, M_USB, M_WAITOK)) == NULL)
				return (USBD_NOMEM);
			utag->nbufs++;
		}

#ifdef USBMEM_DEBUG
		TAILQ_FOREACH(dd, &utag->uds_lease, ud_debug_next) {
			if (dd == ud) {
				printf("%s: usb_alloc_buffer_dma: dma %p already in lease %s:%d possibly at %s:%d\n",
				    utag->devname, ud, filename, lineno,
				    ud->ud_fn, ud->ud_ln);
				TAILQ_REMOVE(&utag->uds_lease, dd,
				    ud_debug_next);
				break;
			}
		}
		ud->ud_fn = filename;
		ud->ud_ln = lineno;
		TAILQ_INSERT_HEAD(&utag->uds_lease, ud, ud_debug_next);
#endif
		ud->ud_len = size;
		ub->ub_dmamem = ud;


		ub->ub_type = UB_ALLOC_MAP;

		/*
		 * need to allocate DMA memory
		 */
#if defined(__NetBSD__) || defined(__OpenBSD__)
		if (bus_dmamem_alloc(utag->tag, size, PAGE_SIZE, 0,
		    ud->ud_segs, USB_DMA_NSEG, &ud->ud_nsegs,
		    BUS_DMA_WAITOK) != 0) {
			PUTUD(utag, ud);
			return (USBD_NOMEM);
		}
		if (bus_dmamem_map(utag->tag, ud->ud_segs,
		    ud->ud_nsegs, size, &ud->ud_kaddr,
		    BUS_DMA_WAITOK | BUS_DMA_COHERENT) != 0) {
			goto free;
		}
#elif defined(__FreeBSD__)
#if __FreeBSD_version >= 500000
		if (bus_dma_tag_create(utag->tag_parent, 1, 0,
		    BUS_SPACE_MAXADDR_32BIT, BUS_SPACE_MAXADDR, NULL, NULL,
		    size, USB_DMA_NSEG, size,
		    0, NULL, NULL, &ud->ud_tag) == ENOMEM)
#else
		if (bus_dma_tag_create(utag->tag_parent, 1, 0,
		    BUS_SPACE_MAXADDR_32BIT, BUS_SPACE_MAXADDR, NULL, NULL,
		    size, USB_DMA_NSEG, size,
		    0, &ud->ud_tag) == ENOMEM)
#endif
		{
			PUTUD(utag, ud);
			return (USBD_NOMEM);
		}
		ub->ub_curtag = ud->ud_tag;
		if (bus_dmamem_alloc(ud->ud_tag, &ud->ud_kaddr,
		    BUS_DMA_WAITOK | BUS_DMA_COHERENT, &ud->ud_map))
			goto freetag;
#endif	/* __FreeBSD__ */

		buf = ud->ud_kaddr;
		DPRINTFN(5, ("usb_alloc_buffer_dma: type=UB_ALLOC_MAP ub=%p, buf=%p size=%lu\n",
			     ub, buf, (u_long)size));
	} else {
		/*
		 * No alloc, just map the buffer
		 */
#ifdef __FreeBSD__
		if ((ub->ub_flags & (USB_BUFFL_ASYNCMAP|USB_BUFFL_MAP)) == 0) {
			if (bus_dmamap_create(utag->tag_buffer, 0, &ub->ub_map)) {
				PUTUD(utag, ub);
				return (USBD_NOMEM);
			}
			ub->ub_flags |= USB_BUFFL_MAP;
		}
		ub->ub_curtag = (ub->ub_flags & USB_BUFFL_ASYNCMAP) ?
		    ub->ub_asynctag : utag->tag_buffer;
#endif
		ub->ub_type = UB_MAP;
		DPRINTFN(5, ("usb_alloc_buffer_dma: type=UB_MAP ub=%p buf=%p size=%lu\n",
			     ub, buf, (u_long)size));
	}

#if defined(__NetBSD__) || defined(__OpenBSD__)
	if ((ub->ub_flags & (USB_BUFFL_ASYNCMAP|USB_BUFFL_MAP)) == 0) {
		if (bus_dmamap_create(utag->tag, size, USB_DMA_NSEG, size, 0,
		    BUS_DMA_WAITOK, &ub->ub_map) != 0)
			goto free;
		ub->ub_flags |= USB_BUFFL_MAP;
	}
#endif

	/*
	 * load buffer into DMA space
	 */
#if defined(__NetBSD__) || defined(__OpenBSD__)
	if ((err = bus_dmamap_load(utag->tag, ub->ub_map, buf, size,
	    NULL, BUS_DMA_WAITOK)) != 0)
		goto unmap;
#elif defined(__FreeBSD__)
	err = bus_dmamap_load(ub->ub_curtag,
	    (ub->ub_type == UB_ALLOC_MAP) ? ud->ud_map : ub->ub_map,
	    buf, size, usb_alloc_buffer_dma_callback, ub, 0);
	if (err == EINPROGRESS) {
		tsleep(ub, PRIBIO, "usbld", 0);
		/* return from usb_map_buffer_callback */
		err = ub->ub_error;
	}
	if (err != 0) {
		goto destroy;
	}
#endif

	*pbufadr = buf; /* kernel address of allocated or mapped buffer */
	return (USBD_NORMAL_COMPLETION);

#if defined(__NetBSD__) || defined(__OpenBSD__)
unmap:
	if (ub->ub_type == UB_ALLOC_MAP)
		bus_dmamem_unmap(utag->tag, ud->ud_kaddr, size);
	if (ub->ub_flags & USB_BUFFL_MAP) {
		bus_dmamap_destroy(utag->tag, ub->ub_map);
		ub->ub_flags &= ~USB_BUFFL_MAP;
	}
	if (ub->ub_type == UB_ALLOC_MAP)
free:		bus_dmamem_free(utag->tag, ud->ud_segs, ud->ud_nsegs);
#elif defined(__FreeBSD__)
destroy:
	if (ub->ub_flags & USB_BUFFL_MAP) {
		bus_dmamap_destroy(utag->tag_buffer, ub->ub_map);
		ub->ub_flags &= ~USB_BUFFL_MAP;
	}
	if (ub->ub_type == UB_ALLOC_MAP) {
		bus_dmamem_free(ud->ud_tag, ud->ud_kaddr, ud->ud_map);
freetag:
		bus_dma_tag_destroy(ud->ud_tag);
	}
#endif

	if (ud)
		PUTUD(utag, ud);

	ub->ub_type = UB_NONE;

	return (USBD_NOMEM);
}

/*
 * Unmap (and free if it was allocated)
 */
void
usb_free_buffer_dma(usb_dma_tag_t *utag, struct usb_buffer_dma *ub,
	enum usbd_waitflg waitflg
	USBMEM_DEBUGPARAMS)
{
	int s;
	int no_task;
	struct usb_dmamem *ud;
#ifdef USBMEM_DEBUG
	struct usb_dmamem *dd;
#endif

	DPRINTFN(5, ("usb_free_buffer_dma: type=%s ub=%p\n",
		     ub->ub_type == UB_BUF ? "UB_BUF" :
		     ub->ub_type == UB_ALLOC_MAP ? "UB_ALLOC_MAP" :
		     ub->ub_type == UB_MAP ? "UB_MAP" :
		     ub->ub_type == UB_NONE ? "UB_NONE" :
		     "?",
		     ub));

	ud = ub->ub_dmamem;	/* UB_ALLOC_MAP only */

#ifdef USBMEM_DEBUG
	if (ub->ub_type == UB_ALLOC_MAP) {
		TAILQ_FOREACH(dd, &utag->uds_lease, ud_debug_next) {
			if (dd == ud)
				break;
		}
		if (dd == NULL) {
			printf("%s: usb_free_buffer_dma: dmamem %p not in lease %s:%d possibly freed at %s:%d\n",
			    utag->devname, ud, filename, lineno,
			    ud->ud_fn, ud->ud_ln);
		} else {
			ud->ud_fn = filename;
			ud->ud_ln = lineno;
			TAILQ_REMOVE(&utag->uds_lease, ud, ud_debug_next);
		}
	}
#endif
#if 1
	if (waitflg == U_WAITOK && !SIMPLEQ_EMPTY(&utag->uds_tobefreed))
		usb_dma_free_dmamems(utag);
#endif

	/*
	 * unmap (and free) buffer
	 */
	switch (ub->ub_type) {
	case UB_BUF:
		usb_freemem(utag, &ub->ub_allocbuf USBMEM_DEBUGARGS);
		break;

	case UB_ALLOC_MAP:
	case UB_MAP:
		/* unload buffer */
#if defined(__NetBSD__) || defined(__OpenBSD__)
		bus_dmamap_unload(utag->tag, ub->ub_map);
#elif defined(__FreeBSD__)
		bus_dmamap_unload(ub->ub_curtag,
		    (ub->ub_type == UB_ALLOC_MAP) ? ud->ud_map : ub->ub_map);
#endif
		if ((ub->ub_flags & USB_BUFFL_MAP) && waitflg == U_WAITOK) {
#if defined(__NetBSD__) || defined(__OpenBSD__)
			bus_dmamap_destroy(utag->tag, ub->ub_map);
#elif defined(__FreeBSD__)
			bus_dmamap_destroy(utag->tag_buffer, ub->ub_map);
#endif
			ub->ub_flags &= ~USB_BUFFL_MAP;
		}

		if (ub->ub_type == UB_ALLOC_MAP) {
			if (waitflg == U_WAITOK) {
				usb_dma_free_dmamem(utag, ud);
			} else {
				s = splusb();
				no_task = SIMPLEQ_EMPTY(&utag->uds_tobefreed);
				SIMPLEQ_INSERT_TAIL(&utag->uds_tobefreed, ud,
				    ud_next);
				/* request free */
				if (no_task)
					usb_add_task(NULL /* XXX this arg is unused */,
					    &utag->unmap_task, USB_TASKQ_HC);
				splx(s);
			}
		}

		break;

	default:
#ifdef DIAGNOSTIC
		panic("usb_sync_buffer_dma: wrong type %d", ub->ub_type);
#endif
	case UB_NONE:
		break;
	}

	ub->ub_type = UB_NONE;
}

#ifdef __FreeBSD__
Static void
usb_map_dma_callback(void *arg, bus_dma_segment_t *segs, int nseg,
	int error)
{
	struct usb_buffer_dma *ub = arg;

	ub->ub_error = error;
	if (error == 0) {
		if (segs != ub->ub_nsegs) {
			for (i = 0; i < nseg; i++)
				ub->ub_segs[i] = segs[i];
		}
		ub->ub_nsegs = nseg;
	}
}
#endif /* __FreeBSD__ */

/*
 * Map plain region into DMA buffer
 */
void
usb_map_dma(usb_dma_tag_t *utag, struct usb_buffer_dma *ub,
	void *buf, size_t buflen)
{
	int err;

	USB_KASSERT((ub->ub_flags & (USB_BUFFL_ASYNCMAP|USB_BUFFL_MAP)) ==
	    USB_BUFFL_ASYNCMAP);
	USB_KASSERT(ub->ub_type == UB_NONE);

#if defined(__NetBSD__) || defined(__OpenBSD__)
	err = bus_dmamap_load(utag->tag, ub->ub_map, buf, buflen, NULL,
	    BUS_DMA_NOWAIT);
#elif defined(__FreeBSD__)
	ub->ub_curtag = ub->ub_asynctag;
	err = bus_dmamap_load(ub->ub_asynctag, ub->ub_map,
	    buf, buflen, usb_map_dma_callback, ub, BUS_DMA_NOWAIT);
#endif

	/*
	 * We allocated DMA resources with BUS_DMA_ALLOCNOW, and
	 * bus_dmamap_load() should never block.
	 * Any error here means programming error of usbdi device
	 * driver (missing usbd_unmap_buffer() for previous xfer, etc.).
	 */
	if (err)
		panic("usb_map_dma: bus_dmamap_load faled, err = %d\n", err);

	ub->ub_type = UB_MAP;
}

/*
 * Map mbuf(9) chain into DMA buffer
 */
void
usb_map_mbuf_dma(usb_dma_tag_t *utag, struct usb_buffer_dma *ub,
	struct mbuf *chain)
{
	int err;

	USB_KASSERT((ub->ub_flags & (USB_BUFFL_ASYNCMAP|USB_BUFFL_MAP)) ==
	    USB_BUFFL_ASYNCMAP);
	USB_KASSERT(ub->ub_type == UB_NONE);

#if defined(__NetBSD__) || defined(__OpenBSD__)
	err = bus_dmamap_load_mbuf(utag->tag, ub->ub_map, chain,
	    BUS_DMA_NOWAIT);
#elif defined(__FreeBSD__)
	ub->ub_curtag = ub->ub_asynctag;
	err = bus_dmamap_load_mbuf_sg(ub->ub_asynctag, ub->ub_map, chain,
	    &ub->ub_segs, USB_DMA_NSEG, BUS_DMA_NOWAIT);
#endif

	/*
	 * We allocated DMA resources with BUS_DMA_ALLOCNOW, and
	 * bus_dmamap_load() should never block.
	 * Any error here means programming error of usbdi device
	 * driver (missing usbd_unmap_buffer() for previous xfer, etc.).
	 */
	if (err)
		panic("usb_map_mbuf_dma: bus_dmamap_load faled, err = %d\n",
		    err);

	ub->ub_type = UB_MAP;
}

/*
 * Unmap buffer that has been mapped by usb_map_dma or usb_map_mbuf_dma()
 */
void
usb_unmap_dma(usb_dma_tag_t *utag, struct usb_buffer_dma *ub)
{

	USB_KASSERT((ub->ub_flags & (USB_BUFFL_ASYNCMAP|USB_BUFFL_MAP)) ==
	    USB_BUFFL_ASYNCMAP);
	USB_KASSERT(ub->ub_type == UB_MAP);

	/* unload buffer */
#if defined(__NetBSD__) || defined(__OpenBSD__)
	bus_dmamap_unload(utag->tag, ub->ub_map);
#elif defined(__FreeBSD__)
	bus_dmamap_unload(ub->ub_asynctag, ub->ub_map);
#endif
	ub->ub_type = UB_NONE;
}

/*
 * Pre-allocate resources so that mapping won't block
 */
usbd_status
usb_alloc_dma_resources(usb_dma_tag_t *utag, struct usb_buffer_dma *ub)
{

	USB_KASSERT((ub->ub_flags & USB_BUFFL_ASYNCMAP) == 0);
	USB_KASSERT(ub->ub_type == UB_NONE);

	if ((ub->ub_flags & USB_BUFFL_MAP)) {
#if defined(__NetBSD__) || defined(__OpenBSD__)
		bus_dmamap_destroy(utag->tag, ub->ub_map);
#elif defined(__FreeBSD__)
		bus_dmamap_destroy(utag->tag_buffer, ub->ub_map);
#endif
		ub->ub_flags &= ~USB_BUFFL_MAP;
	}

#if defined(__NetBSD__) || defined(__OpenBSD__)
	if (bus_dmamap_create(utag->tag, MAXPHYS, USB_DMA_NSEG, MAXPHYS,
	    0, BUS_DMA_WAITOK | BUS_DMA_ALLOCNOW, &ub->ub_map) != 0)
		return (USBD_NOMEM);
#elif defined(__FreeBSD__)
#if __FreeBSD_version >= 500000
	if (bus_dma_tag_create(utag->tag_parent, 1, 0,
	    BUS_SPACE_MAXADDR_32BIT, BUS_SPACE_MAXADDR, NULL, NULL,
	    MAXPHYS, USB_DMA_NSEG, MAXPHYS,
	    BUS_DMA_ALLOCNOW, NULL, NULL, &ub->ub_asynctag) == ENOMEM)
#else
	if (bus_dma_tag_create(utag->tag_parent, 1, 0,
	    BUS_SPACE_MAXADDR_32BIT, BUS_SPACE_MAXADDR, NULL, NULL,
	    MAXPHYS, USB_DMA_NSEG, MAXPHYS,
	    BUS_DMA_ALLOCNOW, &ub->ub_asynctag) == ENOMEM)
#endif
	{
		return (USBD_NOMEM);
	}
	if (bus_dmamap_create(ub->ub_asynctag, 0, &ub->ub_map) == ENOMEM) {
		bus_dma_tag_destroy(ub->ub_asynctag);
		return (USBD_NOMEM);
	}
#endif	/* __FreeBSD__ */

	ub->ub_flags |= USB_BUFFL_ASYNCMAP;

	return (USBD_NORMAL_COMPLETION);
}

/*
 * Free pre-allocated resources for mapping
 */
void
usb_free_dma_resources(usb_dma_tag_t *utag, struct usb_buffer_dma *ub)
{

	USB_KASSERT((ub->ub_flags & (USB_BUFFL_ASYNCMAP | USB_BUFFL_MAP))
	    == USB_BUFFL_ASYNCMAP);
	USB_KASSERT(ub->ub_type == UB_NONE);

#if defined(__NetBSD__) || defined(__OpenBSD__)
	bus_dmamap_destroy(utag->tag, ub->ub_map);
#elif defined(__FreeBSD__)
	bus_dmamap_destroy(ub->ub_asynctag, ub->ub_map);
	bus_dma_tag_destroy(ub->ub_asynctag);
#endif

	ub->ub_flags &= ~USB_BUFFL_ASYNCMAP;
}

/*
 * Sync DMA buffer
 */
void
usb_sync_buffer_dma(usb_dma_tag_t *utag, struct usb_buffer_dma *ub, int ops)
{

	DPRINTFN(5, ("usb_sync_buffer_dma: type=%s ub=%p%s%s%s%s\n",
		     ub->ub_type == UB_BUF ? "UB_BUF" :
		     ub->ub_type == UB_ALLOC_MAP ? "UB_ALLOC_MAP" :
		     ub->ub_type == UB_MAP ? "UB_MAP" :
		     ub->ub_type == UB_NONE ? "UB_NONE" :
		     "?",
		     ub,
		     (ops & BUS_DMASYNC_PREREAD) ? " PreR" : "",
		     (ops & BUS_DMASYNC_PREWRITE) ? " PreW" : "",
		     (ops & BUS_DMASYNC_POSTREAD) ? " PostR" : "",
		     (ops & BUS_DMASYNC_POSTWRITE) ? " PostW" : ""));

	switch (ub->ub_type) {
	case UB_BUF:
		USB_MEM_SYNC(utag, &ub->ub_allocbuf, ops);
		break;
	case UB_ALLOC_MAP:
	case UB_MAP:
#if defined(__NetBSD__) || defined(__OpenBSD__)
		bus_dmamap_sync(utag->tag, ub->ub_map, 0,
		    ub->ub_map->dm_mapsize, ops);
#elif defined(__FreeBSD__)
		bus_dmamap_sync(ub->ub_curtag, (ub->ub_type == UB_ALLOC_MAP) ?
		    ub->ub_dmamem->ud_map : ub_ub_map,
		    ops);
#endif
		break;
	default:
#ifdef DIAGNOSTIC
		panic("usb_sync_buffer_dma: wrong type %d", ub->ub_type);
#endif
	case UB_NONE:
		DPRINTFN(5, ("usb_sync_buffer_dma: no buffer\n"));
		break;
	}
}

/*
 * Clenup DMA tag just before it is freed
 */
void
usb_clean_buffer_dma(usb_dma_tag_t *utag, struct usb_buffer_dma *ub
	USBMEM_DEBUGPARAMS)
{

	/* Make sure buffer is absent. */
	if (ub->ub_type != UB_NONE) {
		printf("usb_clean_buffer_dma: have buffer, type %d\n",
		    ub->ub_type);
		usb_free_buffer_dma(utag, ub, M_WAITOK USBMEM_DEBUGARGS);
	}

	/* DMA map may exist if buffer has been freed asynchronously. */
	if ((ub->ub_flags & USB_BUFFL_MAP)) {
#if defined(__NetBSD__) || defined(__OpenBSD__)
		bus_dmamap_destroy(utag->tag, ub->ub_map);
#elif defined(__FreeBSD__)
		bus_dmamap_destroy(utag->tag_buffer, ub->ub_map);
#endif
		ub->ub_flags &= ~USB_BUFFL_MAP;
	}

	/* Check pre-allocated DMA resource and warn if exist. */
	if ((ub->ub_flags & USB_BUFFL_ASYNCMAP) != 0) {
		printf("usb_clean_buffer_dma: have DMA resources\n");
		usb_free_dma_resources(utag, ub);
	}
}

/*
 * Sync DMA buffer and usb_insert_transfer().
 */
usbd_status
usb_insert_transfer_dma(usbd_xfer_handle xfer,
	usb_dma_tag_t *utag, struct usb_buffer_dma *ub)
{
	int ops;

	if (ub) {
		/*
		 *  Even if we have no data portion we still need to
		 * sync the dmamap for the request data in the SETUP
		 * packet.
		 */
		ops = 0;
		if (!usbd_xfer_isread(xfer) || xfer->rqflags & URQ_REQUEST)
			ops = BUS_DMASYNC_PREWRITE;

		if (usbd_xfer_isread(xfer))
			ops |= BUS_DMASYNC_PREREAD;

		usb_sync_buffer_dma(utag, ub, ops);
	}

	return usb_insert_transfer(xfer);
}

/*
 * Sync DMA buffer and usb_transfer_complete().
 */
void
usb_transfer_complete_dma(usbd_xfer_handle xfer,
	usb_dma_tag_t *utag, struct usb_buffer_dma *ub)
{

	if (ub)
		usb_sync_buffer_dma(utag, ub,
		    usbd_xfer_isread(xfer) ? BUS_DMASYNC_POSTREAD :
		    BUS_DMASYNC_POSTWRITE);

	usb_transfer_complete(xfer);
}

void
usb_dma_tag_init(usb_dma_tag_t *utag
#ifdef USBMEM_DEBUG
	, device_ptr_t dev
#endif
	USBMEM_DEBUGPARAMS)
{
#ifdef USBMEM_DEBUG
	struct usb_frag_debug *f;
#endif

	LIST_INIT(&utag->blk_freelist);
	LIST_INIT(&utag->frag_freelist);
	SIMPLEQ_INIT(&utag->uds_idle);
	SIMPLEQ_INIT(&utag->uds_tobefreed);
	utag->blk_nfree = 0;
	utag->nblks = utag->nfrags = utag->nbufs = 0;

	usb_init_task(&utag->unmap_task, usb_dma_free_dmamems, utag);

#ifdef USBMEM_DEBUG
	TAILQ_INIT(&utag->uds_lease);
	TAILQ_INIT(&utag->blks_lease);
	TAILQ_INIT(&utag->frags_lease);
	TAILQ_INIT(&utag->free_frag_debug);
	utag->frag_debug_used = utag->frag_debug_used_max = 0;
	for (f = utag->frag_debug_chunk;
	    f < utag->frag_debug_chunk + USBMEM_DEBUG_NFRAG; f++) {
		TAILQ_INSERT_TAIL(&utag->free_frag_debug, f, f_debug_next);
	}
	utag->devname = USBDEVPTRNAME(dev);
	printf("%s: USBMEM_DEBUG: enabled %s:%d\n",
	    utag->devname, filename, lineno);
#endif
	DPRINTFN(5, ("usb_dma_tag_init: utag=%p\n", utag));
}

void
usb_dma_tag_finish(usb_dma_tag_t *utag
	USBMEM_DEBUGPARAMS)
{
	struct usb_frag_dma *f;
	usb_dma_block_t *b;
	struct usb_dmamem *ud;
#ifdef USBMEM_DEBUG
	struct usb_dma_block *bd;
	struct usb_frag_debug *fd;
	struct usb_dmamem *dd;
#endif

	/* current usage */
#if 1
	printf("usb_dma_tag_finish: utag=%p, %d blks, %d frags, %d bufs\n",
		utag, utag->nblks, utag->nfrags, utag->nbufs);
#else
	DPRINTF(("usb_dma_tag_finish: utag=%p, %d blks, %d frags, %d bufs\n",
		  utag, utag->nblks, utag->nfrags, utag->nbufs));
#endif

#ifdef USBMEM_DEBUG
	/*
	 * print leaks
	 */

	TAILQ_FOREACH_REVERSE(bd, &utag->blks_lease, usb_debug_blk_head,
	    b_debug_next) {
		printf("%s: block %p leaked, allocated at %s:%d\n",
		    utag->devname, bd, bd->b_fn, bd->b_ln);
	}

	TAILQ_FOREACH_REVERSE(fd, &utag->frags_lease, usb_debug_frag_head,
	    f_debug_next) {
		printf("%s: fragment %p leaked, allocated at %s:%d\n",
		    utag->devname, fd, fd->f_fn, fd->f_ln);
	}
	printf("%s: max number of fragments = %d\n",
	    utag->devname, utag->frag_debug_used_max);

	TAILQ_FOREACH_REVERSE(dd, &utag->uds_lease, usb_debug_dmamem_head,
	    ud_debug_next) {
		printf("%s: dmamem %p leaked, allocated at %s:%d\n",
		    utag->devname, dd, dd->ud_fn, dd->ud_ln);
	}
#endif

	/* 1. free DMA memory */

	/*
	 * First, scan flagments and add relevant blocks to block free list.
	 * Fragment structures themselves use the spece of the block
	 * and need not be freed.
	 */
	while ((f = LIST_FIRST(&utag->frag_freelist)) != NULL) {
		b = f->block;
		if ((b->flags & USB_DMA_MARKED_TO_FREE) == 0) {
			b->flags = USB_DMA_FULLBLOCK | USB_DMA_MARKED_TO_FREE;
			LIST_INSERT_HEAD(&utag->blk_freelist, b, next);
		}
		LIST_REMOVE(f, next);
		utag->nfrags--;
	}

	/*
	 * Second, free blocks.
	 */
	while ((b = LIST_FIRST(&utag->blk_freelist)) != NULL) {
#if defined(__NetBSD__) || defined(__OpenBSD__)
		bus_dmamap_unload(utag->tag, b->map);
		bus_dmamem_unmap(utag->tag, b->kaddr, b->size);
		bus_dmamap_destroy(utag->tag, b->map);
		bus_dmamem_free(utag->tag, b->segs, b->nsegs);
#elif defined(__FreeBSD__)
		bus_dmamap_unload(b->tag, b->map);
		bus_dmamem_free(b->tag, (void *)b->kaddr, b->map);
		bus_dma_tag_destroy(b->tag);
#endif

		LIST_REMOVE(b, next);
		free(b, M_USB);
		utag->nblks--;
	}

	utag->blk_nfree = 0;

	/* 2. free DMA memory structures */
	usb_rem_task(NULL /* XXX this arg is unused */, &utag->unmap_task);
	usb_dma_free_dmamems(utag);
	while ((ud = SIMPLEQ_FIRST(&utag->uds_idle)) != NULL) {
		SIMPLEQ_REMOVE_HEAD(&utag->uds_idle, ud_next);
		free(ud, M_USB);
		utag->nbufs--;
	}

#if 1	/* DIAGNOSTIC */
	if (utag->nblks || utag->nfrags || utag->nbufs)
		printf("usb_dma_tag_finish: leaked %d blks, %d frags, %d bufs\n",
		    utag->nblks, utag->nfrags, utag->nbufs);
#endif
}
