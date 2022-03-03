/*	$NetBSD: if_upl.c,v 1.76 2022/03/03 05:56:18 riastradh Exp $	*/

/*
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
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
 * Prolific PL2301/PL2302 driver
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_upl.c,v 1.76 2022/03/03 05:56:18 riastradh Exp $");

#ifdef _KERNEL_OPT
#include "opt_inet.h"
#include "opt_usb.h"
#endif

#include <sys/param.h>

#include <dev/usb/usbnet.h>

#include <net/if_types.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet/if_inarp.h>
#endif

/*
 * 7  6  5  4  3  2  1  0
 * tx rx 1  0
 * 1110 0000 rxdata
 * 1010 0000 idle
 * 0010 0000 tx over
 * 0110      tx over + rxd
 */

#define UPL_RXDATA		0x40
#define UPL_TXOK		0x80

#define UPL_CONFIG_NO		1
#define UPL_IFACE_IDX		0

/***/

#define UPL_INTR_INTERVAL	20

#define UPL_BUFSZ		1024

#define UPL_RX_LIST_CNT		1
#define UPL_TX_LIST_CNT		1

#ifdef UPL_DEBUG
#define DPRINTF(x)	if (upldebug) printf x
#define DPRINTFN(n,x)	if (upldebug >= (n)) printf x
int	upldebug = 0;
#else
#define DPRINTF(x)
#define DPRINTFN(n,x)
#endif

/*
 * Various supported device vendors/products.
 */
static const struct usb_devno sc_devs[] = {
	{ USB_VENDOR_PROLIFIC, USB_PRODUCT_PROLIFIC_PL2301 },
	{ USB_VENDOR_PROLIFIC, USB_PRODUCT_PROLIFIC_PL2302 },
	{ USB_VENDOR_PROLIFIC, USB_PRODUCT_PROLIFIC_PL25A1 },
	{ USB_VENDOR_BELKIN, USB_PRODUCT_BELKIN_F5U258 },
	{ USB_VENDOR_NI, USB_PRODUCT_NI_HTOH_7825 }
};

static int	upl_match(device_t, cfdata_t, void *);
static void	upl_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(upl, sizeof(struct usbnet), upl_match, upl_attach,
    usbnet_detach, usbnet_activate);

#if 0
static void upl_uno_intr(struct usbnet *, usbd_status);
#endif
static void upl_uno_rx_loop(struct usbnet *, struct usbnet_chain *, uint32_t);
static unsigned upl_uno_tx_prepare(struct usbnet *, struct mbuf *,
			       struct usbnet_chain *);
static int upl_uno_ioctl(struct ifnet *, u_long, void *);

static const struct usbnet_ops upl_ops = {
	.uno_tx_prepare = upl_uno_tx_prepare,
	.uno_rx_loop = upl_uno_rx_loop,
	.uno_ioctl = upl_uno_ioctl,
#if 0
	.uno_intr = upl_uno_intr,
#endif
};

static int upl_output(struct ifnet *, struct mbuf *, const struct sockaddr *,
		      const struct rtentry *);
static void upl_input(struct ifnet *, struct mbuf *);

/*
 * Probe for a Prolific chip.
 */
static int
upl_match(device_t parent, cfdata_t match, void *aux)
{
	struct usb_attach_arg *uaa = aux;

	return usb_lookup(sc_devs, uaa->uaa_vendor, uaa->uaa_product) != NULL ?
		UMATCH_VENDOR_PRODUCT : UMATCH_NONE;
}

static void
upl_attach(device_t parent, device_t self, void *aux)
{
	struct usbnet * const	un = device_private(self);
	struct usb_attach_arg	*uaa = aux;
	char			*devinfop;
	struct usbd_device *	dev = uaa->uaa_device;
	usbd_status		err;
	usb_interface_descriptor_t	*id;
	usb_endpoint_descriptor_t	*ed;
	int			i;

	DPRINTFN(5,(" : upl_attach: un=%p, dev=%p", un, dev));

	aprint_naive("\n");
	aprint_normal("\n");
	devinfop = usbd_devinfo_alloc(dev, 0);
	aprint_normal_dev(self, "%s\n", devinfop);
	usbd_devinfo_free(devinfop);

	un->un_dev = self;
	un->un_udev = dev;
	un->un_sc = un;
	un->un_ops = &upl_ops;
	un->un_rx_xfer_flags = USBD_SHORT_XFER_OK;
	un->un_tx_xfer_flags = USBD_FORCE_SHORT_XFER;
	un->un_rx_list_cnt = UPL_RX_LIST_CNT;
	un->un_tx_list_cnt = UPL_TX_LIST_CNT;
	un->un_rx_bufsz = UPL_BUFSZ;
	un->un_tx_bufsz = UPL_BUFSZ;

	err = usbd_set_config_no(dev, UPL_CONFIG_NO, 1);
	if (err) {
		aprint_error_dev(self, "failed to set configuration"
		    ", err=%s\n", usbd_errstr(err));
		return;
	}

	err = usbd_device2interface_handle(dev, UPL_IFACE_IDX, &un->un_iface);
	if (err) {
		aprint_error_dev(self, "getting interface handle failed\n");
		return;
	}

	id = usbd_get_interface_descriptor(un->un_iface);

	/* Find endpoints. */
	for (i = 0; i < id->bNumEndpoints; i++) {
		ed = usbd_interface2endpoint_descriptor(un->un_iface, i);
		if (ed == NULL) {
			aprint_error_dev(self, "couldn't get ep %d\n", i);
			return;
		}
		if (UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_IN &&
		    UE_GET_XFERTYPE(ed->bmAttributes) == UE_BULK) {
			un->un_ed[USBNET_ENDPT_RX] = ed->bEndpointAddress;
		} else if (UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_OUT &&
			   UE_GET_XFERTYPE(ed->bmAttributes) == UE_BULK) {
			un->un_ed[USBNET_ENDPT_TX] = ed->bEndpointAddress;
		}
	}

	if (un->un_ed[USBNET_ENDPT_RX] == 0 || un->un_ed[USBNET_ENDPT_TX] == 0 /*||
	    un->un_ed[USBNET_ENDPT_INTR] == 0*/) {
		aprint_error_dev(self, "missing endpoint\n");
		return;
	}

	usbnet_attach(un, "upldet");

	/* Initialize interface info.*/
	struct ifnet *ifp = usbnet_ifp(un);
	ifp->if_mtu = UPL_BUFSZ;
	ifp->if_type = IFT_OTHER;
	ifp->if_addrlen = 0;
	ifp->if_hdrlen = 0;
	ifp->if_output = upl_output;
	ifp->_if_input = upl_input;
	ifp->if_baudrate = 12000000;
	ifp->if_dlt = DLT_RAW;

	usbnet_attach_ifp(un, IFF_POINTOPOINT | IFF_NOARP | IFF_SIMPLEX,
	    0, NULL);
}

static void
upl_uno_rx_loop(struct usbnet * un, struct usbnet_chain *c, uint32_t total_len)
{

	DPRINTFN(9,("%s: %s: enter length=%d\n",
		    device_xname(un->un_dev), __func__, total_len));

	usbnet_input(un, c->unc_buf, total_len);
}

static unsigned
upl_uno_tx_prepare(struct usbnet *un, struct mbuf *m, struct usbnet_chain *c)
{
	int	total_len;

	if ((unsigned)m->m_pkthdr.len > un->un_tx_bufsz)
		return 0;

	m_copydata(m, 0, m->m_pkthdr.len, c->unc_buf);
	total_len = m->m_pkthdr.len;

	DPRINTFN(10,("%s: %s: total_len=%d\n",
		     device_xname(un->un_dev), __func__, total_len));

	return total_len;
}

static int
upl_uno_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	if (cmd == SIOCSIFMTU) {
		struct ifreq *ifr = data;

		if (ifr->ifr_mtu > UPL_BUFSZ)
			return EINVAL;
	}
	return 0;
}

static int
upl_output(struct ifnet *ifp, struct mbuf *m, const struct sockaddr *dst,
    const struct rtentry *rt0)
{
	struct usbnet * const un __unused = ifp->if_softc;

	DPRINTFN(10,("%s: %s: enter\n", device_xname(un->un_dev), __func__));

	/* If the queueing discipline needs packet classification, do it now. */
	IFQ_CLASSIFY(&ifp->if_snd, m, dst->sa_family);

	/*
	 * Queue message on interface, and start output if interface
	 * not yet active.
	 */
	return if_transmit_lock(ifp, m);
}

static void
upl_input(struct ifnet *ifp, struct mbuf *m)
{
#ifdef INET
	size_t pktlen = m->m_len;
	int s;

	s = splnet();
	if (__predict_false(!pktq_enqueue(ip_pktq, m, 0))) {
		if_statinc(ifp, if_iqdrops);
		m_freem(m);
	} else {
		if_statadd2(ifp, if_ipackets, 1, if_ibytes, pktlen);
	}
	splx(s);
#endif
}

#ifdef _MODULE
#include "ioconf.c"
#endif

USBNET_MODULE(upl)
