/*	$NetBSD: usb_mem_nodma.c,v 1.1.2.2 2007/05/31 23:15:18 itohy Exp $	*/

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
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
 * USB memory allocation.
 * Currently we use malloc(9) directly.  If memory fragmentation or
 * overhead matters, it should be rewritten using chunk allocations.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: usb_mem_nodma.c,v 1.1.2.2 2007/05/31 23:15:18 itohy Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#if defined(__NetBSD__) || defined(__OpenBSD__)
#include <sys/device.h>		/* for usbdivar.h */
#elif defined(__FreeBSD__)
#include <sys/endian.h>
#include <sys/module.h>
#endif
#include <sys/queue.h>

#include <machine/endian.h>

#ifdef __NetBSD__
#include <sys/extent.h>
#include <uvm/uvm_extern.h>
#endif

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem_nodma.h>

#ifdef USB_DEBUG
#define DPRINTF(x)	if (usbdebug) logprintf x
#define DPRINTFN(n,x)	if (usbdebug>(n)) logprintf x
extern int usbdebug;
#else
#define DPRINTF(x)
#define DPRINTFN(n,x)
#endif

Static void		usb_mem_free_buffers(usb_mem_tag_t *);

Static void
usb_mem_free_buffers(usb_mem_tag_t *utag)
{
	struct usb_buffer_freeq *uf, *uf1;
	int s;

	s = splusb();
	if ((uf = SIMPLEQ_FIRST(&utag->ubs_tobefreed)) != NULL)
		SIMPLEQ_INIT(&utag->ubs_tobefreed);
	splx(s);
	for ( ; uf; uf = uf1) {
		uf1 = SIMPLEQ_NEXT(uf, uf_next);
		free(uf, M_USB);
		utag->nbufs--;
	}
}

usbd_status
usb_alloc_buffer_mem(usb_mem_tag_t *utag, void *buf, size_t size,
	struct usb_buffer_mem *ub,
	void **pbufadr)
{

	USB_KASSERT(ub->ub_type == UB_NONE);

	if (!SIMPLEQ_EMPTY(&utag->ubs_tobefreed))
		usb_mem_free_buffers(utag);

#ifdef DIAGNOSTIC
	/* trap missings of usb_buffer_mem_rewind() */
	ub->u.ub_plain.ubp_cur = (void *)0xdeadbeef;
#endif

	if (buf == NULL && size != 0) {
		/*
		 * allocate buffer
		 */
		if (size < sizeof(struct usb_buffer_freeq))
		    size = sizeof(struct usb_buffer_freeq);
		if ((ub->u.ub_plain.ubp_top = ub->u.ub_plain.ubp_allocbuf =
		    *pbufadr = malloc(size, M_USB, M_WAITOK)) == NULL)
			return (USBD_NOMEM);
		DPRINTFN(5, ("usb_alloc_buffer_mem: ub=%p size=%d\n",
			     ub, size));
		utag->nbufs++;
	} else {
		/*
		 * do nothing --- just use the buffer
		 */
		ub->u.ub_plain.ubp_top = *pbufadr = buf;
		ub->u.ub_plain.ubp_allocbuf = NULL;
		DPRINTFN(5, ("usb_alloc_buffer_mem: no alloc\n"));
	}

	ub->ub_type = UB_PLAIN;

	return (USBD_NORMAL_COMPLETION);
}

void
usb_free_buffer_mem(usb_mem_tag_t *utag, struct usb_buffer_mem *ub,
	enum usbd_waitflg waitflg)
{
	int s;
	struct usb_buffer_freeq *uf;

	DPRINTFN(5, ("usb_free_buffer_mem: ub=%p\n", ub));

	USB_KASSERT(ub->ub_type == UB_PLAIN);

	if (waitflg == U_WAITOK && !SIMPLEQ_EMPTY(&utag->ubs_tobefreed))
		usb_mem_free_buffers(utag);

	if ((uf = ub->u.ub_plain.ubp_allocbuf) != NULL) {
		if (waitflg == U_WAITOK) {
			free(uf, M_USB);
			utag->nbufs--;
		} else {
			s = splusb();
			SIMPLEQ_INSERT_TAIL(&utag->ubs_tobefreed, uf, uf_next);
			splx(s);
			/* XXX request free */
		}
	}

	ub->ub_type = UB_NONE;
}

void
usb_map_mem(struct usb_buffer_mem *ub, void *buf)
{

	USB_KASSERT(ub->ub_type == UB_NONE);

	ub->ub_type = UB_PLAIN;
	ub->u.ub_plain.ubp_top = buf;
	ub->u.ub_plain.ubp_allocbuf = NULL;
#ifdef DIAGNOSTIC
	/* trap missings of usb_buffer_mem_rewind() */
	ub->u.ub_plain.ubp_cur = (void *)0xdeadbeef;
#endif
}

void
usb_map_mbuf_mem(struct usb_buffer_mem *ub, struct mbuf *chain)
{

	USB_KASSERT(ub->ub_type == UB_NONE);

	ub->ub_type = UB_MBUF;
	ub->u.ub_mbuf.ubm_top = chain;
#ifdef DIAGNOSTIC
	/* trap missings of usb_buffer_mem_rewind() */
	ub->u.ub_mbuf.ubm_cur = (void *)0xdeadbeef;
	/* ub->u.ub_mbuf.ubm_off = XXX; */
#endif
}

void
usb_unmap_mem(struct usb_buffer_mem *ub)
{

	USB_KASSERT(ub->ub_type == UB_MBUF ||
	    (ub->ub_type == UB_PLAIN && ub->u.ub_plain.ubp_allocbuf == NULL));

	ub->ub_type = UB_NONE;
}

/* set pointer at the top of buffer */
void
usb_buffer_mem_rewind(struct usb_buffer_mem *ub)
{

	switch(ub->ub_type) {
	case UB_MBUF:
		ub->u.ub_mbuf.ubm_off = 0;
		ub->u.ub_mbuf.ubm_cur = ub->u.ub_mbuf.ubm_top;
		break;
	case UB_PLAIN:
		ub->u.ub_plain.ubp_cur = ub->u.ub_plain.ubp_top;
		break;
	default:
		break;
	}
}

void
usb_mem_tag_init(usb_mem_tag_t *utag)
{

	SIMPLEQ_INIT(&utag->ubs_tobefreed);
	utag->nbufs = 0;
	DPRINTFN(5, ("usb_mem_tag_init: utag=%p\n", utag));
}

void
usb_mem_tag_finish(usb_mem_tag_t *utag)
{

	/* current usage */
#if 1
	printf("usb_mem_tag_finish: utag=%p, %d bufs\n", utag, utag->nbufs);
#else
	DPRINTF(("usb_mem_tag_finish: utag=%p, %d bufs\n", utag, utag->nbufs));
#endif

	/* free buffers */
	usb_mem_free_buffers(utag);

#if 1	/* DIAGNOSTIC */
	if (utag->nbufs)
		printf("usb_dma_tag_finish: leaked %d bufs\n", utag->nbufs);
#endif
}
