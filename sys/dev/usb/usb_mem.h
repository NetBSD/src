/*	$NetBSD: usb_mem.h,v 1.23.32.1 2007/05/22 14:57:47 itohy Exp $	*/
/*	$FreeBSD: src/sys/dev/usb/usb_mem.h,v 1.21 2005/01/06 01:43:29 imp Exp $	*/

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

/* track memory allocation/free */
#define USBMEM_DEBUG

#define USB_DMA_NSEG (btoc(MAXPHYS) + 1)

typedef struct usb_dma_block {
#ifdef __FreeBSD__
	bus_dma_tag_t tag;
#endif
	bus_dmamap_t map;
        caddr_t kaddr;
        bus_dma_segment_t segs[1];
        int nsegs;
        size_t size;
        size_t align;
	int flags;
#define USB_DMA_FULLBLOCK	0x0001
#define USB_DMA_MARKED_TO_FREE	0x0002
	LIST_ENTRY(usb_dma_block) next;
#ifdef USBMEM_DEBUG
	TAILQ_ENTRY(usb_dma_block)	b_debug_next;
	const char			*b_fn;
	int				b_ln;
#endif
} usb_dma_block_t;

#if defined(__NetBSD__) || defined(__OpenBSD__)
#define DMAADDR(dma, o) \
	((dma)->block->map->dm_segs[0].ds_addr + (dma)->offs + (o))
#define USB_MEM_SYNC(utag, dma, ops) \
	bus_dmamap_sync((utag)->tag, (dma)->block->map, \
			(dma)->offs, (dma)->len, (ops))
#define USB_MEM_SYNC2(utag, dma, off, len, ops) \
	bus_dmamap_sync((utag)->tag, (dma)->block->map, \
			(dma)->offs + (off), (len), (ops))
#elif defined(__FreeBSD__)
#define DMAADDR(dma, o) \
	((dma)->block->segs[0].ds_addr + (dma)->offs + (o))
/*
 * XXX overkill
 * FreeBSD can't sync map partially
 */
#define USB_MEM_SYNC(utag, dma, ops) \
	bus_dmamap_sync((dma)->block->tag, (dma)->block->map, (ops))
#define USB_MEM_SYNC2(utag, dma, off, len, ops) \
	bus_dmamap_sync((dma)->block->tag, (dma)->block->map, (ops))
#endif

#define KERNADDR(dma, o) \
	((void *)((dma)->block->kaddr + (dma)->offs + (o)))

typedef struct {
	struct usb_dma_block *block;
	u_int offs;
	u_int len;
} usb_dma_t;

/*
 * DMA memory
 */
struct usb_dmamem {
#if defined(__NetBSD__) || defined(__OpenBSD__)
	bus_dma_segment_t ud_segs[USB_DMA_NSEG];
	int		ud_nsegs;
	size_t		ud_len;
	caddr_t		ud_kaddr;
#elif defined(__FreeBSD__)
	void		*ud_kaddr;
	bus_dma_tag_t	ud_tag;
	bus_dmamap_t	ud_map;
#endif

	SIMPLEQ_ENTRY(usb_dmamem)	ud_next;

#ifdef USBMEM_DEBUG
	TAILQ_ENTRY(usb_dmamem)		ud_debug_next;
	const char			*ud_fn;
	int				ud_ln;
#endif
};

/*
 * DMA buffer
 */
struct usb_buffer_dma {
	enum { UB_NONE, UB_BUF, UB_ALLOC_MAP, UB_MAP } ub_type;
	int		ub_flags;
#define USB_BUFFL_ASYNCMAP	0x01	/* created with BUS_DMA_ALLOCNOW */
#define USB_BUFFL_MAP		0x02	/* created without BUS_DMA_ALLOCNOW */

	usb_dma_t	ub_allocbuf;
	struct usb_dmamem *ub_dmamem;

#if defined(__NetBSD__) || defined(__OpenBSD__)
	bus_dmamap_t	ub_map;
	bus_dma_segment_t ub_bufseg;
#define _USB_BUFFER_NSEGS(ub)	(ub)->ub_map->dm_nsegs
#define USB_BUFFER_NSEGS(ub)	\
	((ub)->ub_type == UB_BUF ? 1 : _USB_BUFFER_NSEGS(ub))
#define USB_BUFFER_SEGS(ub)	\
	((ub)->ub_type == UB_BUF ? \
	 &(ub)->ub_bufseg : (ub)->ub_map->dm_segs)

#elif defined(__FreeBSD__)
	bus_dma_tag_t	ub_curtag;
	bus_dma_tag_t	ub_asynctag;
	bus_dmamap_t	ub_map;
	bus_dma_segment_t ub_segs[USB_DMA_NSEG];
	int		ub_nsegs;
	int		ub_error;
#define _USB_BUFFER_NSEGS(ub)	(ub)->ub_nsegs
#define USB_BUFFER_NSEGS(ub)	(_USB_BUFFER_NSEGS(ub))
#define USB_BUFFER_SEGS(ub)	(ub)->ub_segs
#endif
};


#ifdef USBMEM_DEBUG
struct usb_frag_debug {
	const char			*f_fn;
	int				f_ln;
	struct usb_frag_dma		*f_frag;
	TAILQ_ENTRY(usb_frag_debug)	f_debug_next;
};
#endif

/*
 * per host controller
 */
typedef struct usb_dma_tag {
	/*
	 * bus_dma(9) tag, filled by host controller driver
	 */
#if defined(__NetBSD__) || defined(__OpenBSD__)
	bus_dma_tag_t	tag;
#endif
#ifdef __FreeBSD__
	bus_dma_tag_t	tag_parent;
	bus_dma_tag_t	tag_buffer;
#endif

	/*
	 * Lists of free DMA memory
	 */
	LIST_HEAD(, usb_dma_block) blk_freelist;
	int	blk_nfree;
	LIST_HEAD(, usb_frag_dma) frag_freelist;
	/* for statistical and diagnostic purposes */
	int	nblks;		/* number of allocated buffers */
	int	nfrags;		/* number of allocated fragments */

	/* for delayed free */
	SIMPLEQ_HEAD(, usb_dmamem) uds_idle, uds_tobefreed;
	/* for statistical and diagnostic purposes */
	int	nbufs;		/* number of allocated buffer structures */

	struct usb_task		unmap_task;

#ifdef USBMEM_DEBUG
	const char			*devname;
	TAILQ_HEAD(usb_debug_dmamem_head, usb_dmamem)	uds_lease;
	TAILQ_HEAD(usb_debug_blk_head, usb_dma_block)	blks_lease;
	TAILQ_HEAD(usb_debug_frag_head, usb_frag_debug)
					frags_lease, free_frag_debug;
#define USBMEM_DEBUG_NFRAG	100
	struct usb_frag_debug		frag_debug_chunk[USBMEM_DEBUG_NFRAG];
	int	frag_debug_used, frag_debug_used_max;
#endif
} usb_dma_tag_t;

#ifdef USBMEM_DEBUG
#define usb_allocmem		usb_allocmem_debug
#define usb_freemem		usb_freemem_debug
#define usb_alloc_buffer_dma	usb_alloc_buffer_dma_debug
#define usb_free_buffer_dma	usb_free_buffer_dma_debug
#define usb_clean_buffer_dma	usb_clean_buffer_dma_debug
#define usb_dma_tag_init	usb_dma_tag_init_debug
#define usb_dma_tag_finish	usb_dma_tag_finish_debug
#define USBMEM_DEBUGPARAMS	, const char *, int
#else
#define USBMEM_DEBUGPARAMS
#endif

usbd_status	usb_allocmem(usb_dma_tag_t *, size_t, size_t, usb_dma_t *
		    USBMEM_DEBUGPARAMS);
void		usb_freemem(usb_dma_tag_t *, usb_dma_t *
		    USBMEM_DEBUGPARAMS);
usbd_status	usb_alloc_buffer_dma(usb_dma_tag_t *, struct usb_buffer_dma *,
		    void *, size_t, void **
		    USBMEM_DEBUGPARAMS);
void		usb_free_buffer_dma(usb_dma_tag_t *, struct usb_buffer_dma *,
		    enum usbd_waitflg
		    USBMEM_DEBUGPARAMS);
void		usb_map_dma(usb_dma_tag_t *, struct usb_buffer_dma *,
		    void *, size_t);
void		usb_map_mbuf_dma(usb_dma_tag_t *, struct usb_buffer_dma *,
		    struct mbuf *);
void		usb_unmap_dma(usb_dma_tag_t *u, struct usb_buffer_dma *);
usbd_status	usb_alloc_dma_resources(usb_dma_tag_t *,
		    struct usb_buffer_dma *);
void		usb_free_dma_resources(usb_dma_tag_t *,
		    struct usb_buffer_dma *);
void		usb_sync_buffer_dma(usb_dma_tag_t *, struct usb_buffer_dma *,
		    int /*ops*/);
void		usb_clean_buffer_dma(usb_dma_tag_t *, struct usb_buffer_dma *
		    USBMEM_DEBUGPARAMS);
usbd_status	usb_insert_transfer_dma(usbd_xfer_handle, usb_dma_tag_t *,
		    struct usb_buffer_dma *);
void		usb_transfer_complete_dma(usbd_xfer_handle, usb_dma_tag_t *,
		    struct usb_buffer_dma *);
void		usb_dma_tag_init(usb_dma_tag_t *
#ifdef USBMEM_DEBUG
		    , device_ptr_t
#endif
		    USBMEM_DEBUGPARAMS);
void		usb_dma_tag_finish(usb_dma_tag_t *
		    USBMEM_DEBUGPARAMS);

#ifdef USBMEM_DEBUG
#undef USBMEM_DEBUGPARAMS
#ifndef USBMEM_PRIVATE
#undef usb_allocmem
#define usb_allocmem(utag, sz, al, dma) \
	usb_allocmem_debug(utag, sz, al, dma, __FILE__, __LINE__)
#undef usb_freemem
#define usb_freemem(utag, dma) \
	usb_freemem_debug(utag, dma, __FILE__, __LINE__)
#undef usb_alloc_buffer_dma
#define usb_alloc_buffer_dma(utag, ub, ptr, sz, padr) \
	usb_alloc_buffer_dma_debug(utag, ub, ptr, sz, padr, __FILE__, __LINE__)
#undef usb_free_buffer_dma
#define usb_free_buffer_dma(utag, ub, w) \
	usb_free_buffer_dma_debug(utag, ub, w, __FILE__, __LINE__)
#undef usb_clean_buffer_dma
#define usb_clean_buffer_dma(utag, ub) \
	usb_clean_buffer_dma_debug(utag, ub, __FILE__, __LINE__)
#undef usb_dma_tag_init
#define usb_dma_tag_init(utag) \
	usb_dma_tag_init_debug(utag, (device_ptr_t)sc, __FILE__, __LINE__)
#undef usb_dma_tag_finish
#define usb_dma_tag_finish(utag) \
	usb_dma_tag_finish_debug(utag, __FILE__, __LINE__)
#endif	/* !USBMEM_PRIVATE */
#endif	/* USBMEM_DEBUG */
