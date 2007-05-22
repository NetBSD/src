/*	$NetBSD: usb_mem_nodma.h,v 1.1.2.1 2007/05/22 14:57:48 itohy Exp $	*/

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

struct usb_buffer_mem {
	enum { UB_NONE, UB_PLAIN, UB_MBUF } ub_type;
	union {
		struct {
			caddr_t ubp_top;	/* start address */
			caddr_t ubp_cur;	/* current pointer */
			void *ubp_allocbuf;	/* allocated buffer or NULL */
		} ub_plain;
		struct {
			struct mbuf *ubm_top;	/* start mbuf */
			struct mbuf *ubm_cur;	/* current mbuf */
			int ubm_off;		/* current offset in cur mbuf */
		} ub_mbuf;
	} u;
};

struct usb_buffer_freeq {
	SIMPLEQ_ENTRY(usb_buffer_freeq)	uf_next;
};

/*
 * per host controller
 */
typedef struct usb_mem_tag {
	/* for delayed free */
	SIMPLEQ_HEAD(, usb_buffer_freeq) ubs_tobefreed;
	/* for statistical and diagnostic purposes */
	int	nbufs;		/* number of allocated buffers */
} usb_mem_tag_t;

usbd_status	usb_alloc_buffer_mem(usb_mem_tag_t *, void *, size_t,
		    struct usb_buffer_mem *, void **);
void		usb_free_buffer_mem(usb_mem_tag_t *, struct usb_buffer_mem *,
		    enum usbd_waitflg);
void		usb_map_mem(struct usb_buffer_mem *, void *);
void		usb_map_mbuf_mem(struct usb_buffer_mem *, struct mbuf *);
void		usb_unmap_mem(struct usb_buffer_mem *);
void		usb_buffer_mem_rewind(struct usb_buffer_mem *);
void		usb_mem_tag_init(usb_mem_tag_t *);
void		usb_mem_tag_finish(usb_mem_tag_t *);
