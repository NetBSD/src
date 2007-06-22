/*	$NetBSD: usb_ethersubr.c,v 1.1.2.2 2007/06/22 10:55:18 itohy Exp $	*/

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
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: usb_ethersubr.c,v 1.1.2.2 2007/06/22 10:55:18 itohy Exp $");
/* __FBSDID("$FreeBSD: src/sys/dev/usb/usb_ethersubr.c,v 1.23 2007/01/08 23:21:06 alfred Exp $"); */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/sockio.h>
#include <sys/mbuf.h>
#include <sys/malloc.h>
#include <sys/socket.h>

#include <net/if.h>
#include <net/if_arp.h>
#ifdef __NetBSD__
#include <net/if_ether.h>
#endif	/* __NetBSD__ */
#ifdef __OpenBSD__
#ifdef INET
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#include <netinet/if_ether.h>
#endif
#endif	/* __OpenBSD__ */
#ifdef __FreeBSD__
#include <net/ethernet.h>
#endif	/* __FreeBSD__ */

#if NBPFILTER > 0
#include <net/bpf.h>
#endif

#include <dev/usb/usb.h>
#include <dev/usb/usb_ethersubr.h>

/*
 * Initialize an RX descriptor and attach an MBUF cluster.
 */
struct mbuf *
usb_ether_newbuf(struct mbuf *m)
{
	struct mbuf *m_new;

	if (m == NULL) {

#ifdef __FreeBSD__
		m_new = m_getcl(M_DONTWAIT, MT_DATA, M_PKTHDR);
		if (m_new == NULL)
			return (NULL);
#else
		MGETHDR(m_new, M_DONTWAIT, MT_DATA);
		if (m_new == NULL) {
			return (NULL);
		}
		MCLGET(m_new, M_DONTWAIT);
		if (!(m_new->m_flags & M_EXT)) {
			m_freem(m_new);
			return (NULL);
		}
#endif
		m_new->m_len = m_new->m_pkthdr.len = MCLBYTES;
	} else {
		m_new = m;
		m_new->m_len = m_new->m_pkthdr.len = MCLBYTES;
		m_new->m_data = m_new->m_ext.ext_buf;
	}
#ifdef ETHER_ALIGN
	m_adj(m_new, ETHER_ALIGN);
#endif
	return (m_new);
}

int
usb_ether_rx_list_init(device_ptr_t dev, struct ue_chain *c, int nchain,
    usbd_device_handle ue_udev, usbd_pipe_handle ue_pipe)
{
	int i;

	for (i = 0; i < nchain; i++, c++) {
		c->ue_dev = dev;
		c->ue_idx = i;
		if (c->ue_xfer == NULL) {
			c->ue_xfer = usbd_alloc_xfer(ue_udev, ue_pipe);
			if (c->ue_xfer == NULL)
				return (ENOMEM);
			if (usbd_map_alloc(c->ue_xfer)) {
				usbd_free_xfer(c->ue_xfer);
				c->ue_xfer = NULL;
				return (ENOMEM);
			}
			c->ue_mbuf = usb_ether_newbuf(NULL);
			if (c->ue_mbuf == NULL) {
				usbd_free_xfer(c->ue_xfer);
				c->ue_xfer = NULL;
				return (ENOBUFS);
			}
		}
	}

	return (0);
}

int
usb_ether_tx_list_init(device_ptr_t dev, struct ue_chain *c, int nchain,
    usbd_device_handle ue_udev, usbd_pipe_handle ue_pipe,
    struct ue_list_head *listhd)
{
	int i;

	/* drivers should keep list of free chain if nchain > 1 */
	KASSERT(nchain == 1 || listhd != NULL);

	if (listhd)
		SLIST_INIT(listhd);

	for (i = 0; i < nchain; i++, c++) {
		c->ue_dev = dev;
		c->ue_idx = i;
		c->ue_mbuf = NULL;
		if (c->ue_xfer == NULL) {
			c->ue_xfer = usbd_alloc_xfer(ue_udev, ue_pipe);
			if (c->ue_xfer == NULL)
				return (ENOMEM);
			if (usbd_map_alloc(c->ue_xfer)) {
				usbd_free_xfer(c->ue_xfer);
				c->ue_xfer = NULL;
				return (ENOMEM);
			}
			if (listhd)
				SLIST_INSERT_HEAD(listhd, c, ue_next);
		}
	}

	return (0);
}

void
usb_ether_rx_list_free(struct ue_chain *c, int nchain)
{
	int i;

	for (i = 0; i < nchain; i++, c++) {
		if (c->ue_mbuf != NULL) {
			m_freem(c->ue_mbuf);
			c->ue_mbuf = NULL;
		}
		if (c->ue_xfer != NULL) {
			usbd_free_xfer(c->ue_xfer);
			c->ue_xfer = NULL;
		}
	}
}

void
usb_ether_tx_list_free(struct ue_chain *c, int nchain)
{
	int i;

	for (i = 0; i < nchain; i++, c++) {
		if (c->ue_mbuf != NULL) {
			m_freem(c->ue_mbuf);
			c->ue_mbuf = NULL;
		}
		if (c->ue_xfer != NULL) {
			usbd_free_xfer(c->ue_xfer);
			c->ue_xfer = NULL;
		}
	}
}

/*
 * Map mbuf into usb xfer storage for transmission.
 * The mbuf is freed on errors.
 */
int
usb_ether_map_tx_buffer_mbuf(struct ue_chain *c, struct mbuf *m)
{
	struct mbuf		*mn;

	if (usbd_map_buffer_mbuf(c->ue_xfer, m)) {
		/* copy mbuf to contiguous memory and try again */
		if (m->m_pkthdr.len <= MHLEN) {
			m = m_pullup(m, m->m_pkthdr.len);
			if (m == NULL) {
				printf("%s: unable to rearrange Tx mbuf\n",
				    USBDEVPTRNAME(c->ue_dev));
				return (ENOBUFS);
			}
		} else {
#ifdef __FreeBSD__
			mn = m_getcl(M_DONTWAIT, MT_DATA, M_PKTHDR);
			if (mn == NULL) {
				printf("%s: unable to allocte Tx mbuf cluster\n",
				    USBDEVPTRNAME(c->ue_dev));
				m_freem(m);
				return (ENOBUFS);
			}
#else
			MGETHDR(mn, M_DONTWAIT, MT_DATA);
			if (mn == NULL) {
				m_freem(m);
				printf("%s: unable to allocte Tx mbuf\n",
				    USBDEVPTRNAME(c->ue_dev));
				return (ENOBUFS);
			}
			MCLGET(mn, M_DONTWAIT);
			if ((mn->m_flags & M_EXT) == 0) {
				m_freem(mn);
				m_freem(m);
				printf("%s: unable to allocte Tx cluster\n",
				    USBDEVPTRNAME(c->ue_dev));
				return (ENOBUFS);
			}
#endif
			m_copydata(m, 0, m->m_pkthdr.len, mtod(mn, caddr_t));
			mn->m_pkthdr.len = mn->m_len = m->m_pkthdr.len;
			m_freem(m);
			m = mn;
		}

		if (usbd_map_buffer_mbuf(c->ue_xfer, m)) {
			m_freem(m);
			return (ENOMEM);
		}
	}
	c->ue_mbuf = m;

	return (0);
}
