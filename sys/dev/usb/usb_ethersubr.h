/*	$NetBSD: usb_ethersubr.h,v 1.1.2.1 2007/06/12 13:58:25 itohy Exp $	*/

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by ITOH Yasufumi.
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

/*-
 * Copyright (c) 1997, 1998, 1999, 2000
 *	Bill Paul <wpaul@ee.columbia.edu>.  All rights reserved.
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
 *	This product includes software developed by Bill Paul.
 * 4. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Bill Paul AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL Bill Paul OR THE VOICES IN HIS HEAD
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $FreeBSD: src/sys/dev/usb/usb_ethersubr.h,v 1.12 2007/01/08 23:21:06 alfred Exp $
 */

#ifndef _USB_ETHERSUBR_H_
#define _USB_ETHERSUBR_H_

#include <dev/usb/usbdi.h>

struct ue_chain {
	device_ptr_t		ue_dev;		/* softc of the device */
	usbd_xfer_handle	ue_xfer;
	struct mbuf		*ue_mbuf;
	int			ue_idx;
	SLIST_ENTRY(ue_chain)	ue_next;
};
SLIST_HEAD(ue_list_head, ue_chain);

struct mbuf	*usb_ether_newbuf(struct mbuf *);
int		usb_ether_rx_list_init(device_ptr_t, struct ue_chain *, int,
		    usbd_device_handle, usbd_pipe_handle);
int		usb_ether_tx_list_init(device_ptr_t, struct ue_chain *, int,
		    usbd_device_handle, usbd_pipe_handle,
		    struct ue_list_head *);
void		usb_ether_rx_list_free(struct ue_chain *, int);
void		usb_ether_tx_list_free(struct ue_chain *, int);

int		usb_ether_map_tx_buffer_mbuf(struct ue_chain *, struct mbuf *);

#ifdef __FreeBSD__
struct usb_qdat {
	struct ifnet	*ifp;
	void		(*if_rxstart)(struct ifnet *);
};

void		usb_register_netisr(void);
void		usb_ether_input(struct mbuf *);
void		usb_tx_done(struct mbuf *);

struct usb_taskqueue {
	int dummy;
};

void usb_ether_task_init(device_ptr_t, int, struct usb_taskqueue *);
void usb_ether_task_enqueue(struct usb_taskqueue *, struct task *);
void usb_ether_task_drain(struct usb_taskqueue *, struct task *);
void usb_ether_task_destroy(struct usb_taskqueue *);
#endif	/* __FreeBSD__ */

#endif /* _USB_ETHERSUBR_H_ */
