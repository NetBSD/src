/*	$NetBSD: if_ncm.c,v 1.1 2025/01/20 13:54:55 maya Exp $	*/

/*
 * Copyright (c) 1997, 1998, 1999, 2000-2003 Bill Paul <wpaul@windriver.com>
 * Copyright (c) 2003 Craig Boston
 * Copyright (c) 2004 Daniel Hartmeier
 * All rights reserved.
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
 * ARE DISCLAIMED.  IN NO EVENT SHALL Bill Paul, THE VOICES IN HIS HEAD OR
 * THE CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * USB Network Control Model (NCM)
 * https://www.usb.org/sites/default/files/NCM10_012011.zip
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_ncm.c,v 1.1 2025/01/20 13:54:55 maya Exp $");

#include <sys/param.h>
#include <sys/types.h>

#include <sys/device.h>

#include <dev/usb/usbnet.h>
#include <dev/usb/usbcdc.h>

#include <dev/usb/if_ncmreg.h>

struct ncm_softc {
	struct usbnet	ncm_un;
	uint32_t	sc_tx_seq;
};

static int	ncm_match(device_t, cfdata_t, void *);
static void	ncm_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(ncm, sizeof(struct ncm_softc), ncm_match, ncm_attach,
    usbnet_detach, usbnet_activate);

static void	ncm_uno_rx_loop(struct usbnet *, struct usbnet_chain *,
		    uint32_t);
static unsigned	ncm_uno_tx_prepare(struct usbnet *, struct mbuf *,
		    struct usbnet_chain *);

static const struct usbnet_ops ncm_ops = {
	.uno_tx_prepare = ncm_uno_tx_prepare,
	.uno_rx_loop = ncm_uno_rx_loop,
};

static int
ncm_match(device_t parent, cfdata_t match, void *aux)
{
	struct usbif_attach_arg *uiaa = aux;

	if (uiaa->uiaa_class == UICLASS_CDC && uiaa->uiaa_subclass ==
	    UISUBCLASS_NETWORK_CONTROL_MODEL)
		return UMATCH_IFACECLASS_GENERIC;

	return UMATCH_NONE;
}

static void
ncm_attach(device_t parent, device_t self, void *aux)
{
	struct ncm_softc		*sc = device_private(self);
	struct usbnet * const		 un = &sc->ncm_un;
	struct usbif_attach_arg		*uiaa = aux;
	char				*devinfop;
	struct usbd_device		*dev = uiaa->uiaa_device;
	usb_interface_descriptor_t	*id;
	usb_endpoint_descriptor_t	*ed;
	const usb_cdc_union_descriptor_t *ud;
	usb_config_descriptor_t		*cd;
	int				 data_ifcno;
	int				 i, j, numalts;
	const usb_cdc_ethernet_descriptor_t *ue;
	char				 eaddr_str[USB_MAX_ENCODED_STRING_LEN];
	usb_device_request_t		 req;
	struct ncm_ntb_parameters	 np;


	aprint_naive("\n");
	aprint_normal("\n");
	devinfop = usbd_devinfo_alloc(dev, 0);
	aprint_normal_dev(self, "%s\n", devinfop);
	usbd_devinfo_free(devinfop);

	un->un_dev = self;
	un->un_udev = dev;
	un->un_sc = sc;
	un->un_ops = &ncm_ops;
	un->un_rx_xfer_flags = USBD_SHORT_XFER_OK;
	un->un_tx_xfer_flags = USBD_FORCE_SHORT_XFER;

	ud = (const usb_cdc_union_descriptor_t *)usb_find_desc_if(un->un_udev,
	    UDESC_CS_INTERFACE, UDESCSUB_CDC_UNION,
	    usbd_get_interface_descriptor(uiaa->uiaa_iface));
	if (ud == NULL) {
		aprint_error_dev(self, "no union descriptor\n");
		return;
	}
	data_ifcno = ud->bSlaveInterface[0];

	for (i = 0; i < uiaa->uiaa_nifaces; i++) {
		if (uiaa->uiaa_ifaces[i] != NULL) {
			id = usbd_get_interface_descriptor(
			    uiaa->uiaa_ifaces[i]);
			if (id != NULL && id->bInterfaceNumber ==
			    data_ifcno) {
				un->un_iface = uiaa->uiaa_ifaces[i];
				uiaa->uiaa_ifaces[i] = NULL;
			}
		}
	}
	if (un->un_iface == NULL) {
		aprint_error_dev(self, "no data interface\n");
		return;
	}

	/*
	 * <quote>
	 *  The Data Class interface of a networking device shall have a minimum
	 *  of two interface settings. The first setting (the default interface
	 *  setting) includes no endpoints and therefore no networking traffic is
	 *  exchanged whenever the default interface setting is selected. One or
	 *  more additional interface settings are used for normal operation, and
	 *  therefore each includes a pair of endpoints (one IN, and one OUT) to
	 *  exchange network traffic. Select an alternate interface setting to
	 *  initialize the network aspects of the device and to enable the
	 *  exchange of network traffic.
	 * </quote>
	 *
	 * Some devices, most notably cable modems, include interface settings
	 * that have no IN or OUT endpoint, therefore loop through the list of all
	 * available interface settings looking for one with both IN and OUT
	 * endpoints.
	 */
	id = usbd_get_interface_descriptor(un->un_iface);
	cd = usbd_get_config_descriptor(un->un_udev);
	numalts = usbd_get_no_alts(cd, id->bInterfaceNumber);

	for (j = 0; j < numalts; j++) {
		if (usbd_set_interface(un->un_iface, j)) {
			aprint_error_dev(un->un_dev,
					"setting alternate interface failed\n");
			return;
		}
		/* Find endpoints. */
		id = usbd_get_interface_descriptor(un->un_iface);
		un->un_ed[USBNET_ENDPT_RX] = un->un_ed[USBNET_ENDPT_TX] = 0;
		for (i = 0; i < id->bNumEndpoints; i++) {
			ed = usbd_interface2endpoint_descriptor(un->un_iface, i);
			if (!ed) {
				aprint_error_dev(self,
						"could not read endpoint descriptor\n");
				return;
			}
			if (UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_IN &&
					UE_GET_XFERTYPE(ed->bmAttributes) == UE_BULK) {
				un->un_ed[USBNET_ENDPT_RX] = ed->bEndpointAddress;
			} else if (UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_OUT &&
					UE_GET_XFERTYPE(ed->bmAttributes) == UE_BULK) {
				un->un_ed[USBNET_ENDPT_TX] = ed->bEndpointAddress;
			} else if (UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_IN &&
					UE_GET_XFERTYPE(ed->bmAttributes) == UE_INTERRUPT) {
				/* XXX: CDC spec defines an interrupt pipe, but it is not
				 * needed for simple host-to-host applications. */
			} else {
				aprint_error_dev(self, "unexpected endpoint\n");
			}
		}
		/* If we found something, try and use it... */
		if (un->un_ed[USBNET_ENDPT_RX] != 0 && un->un_ed[USBNET_ENDPT_TX] != 0)
			break;
	}

	if (un->un_ed[USBNET_ENDPT_RX] == 0) {
		aprint_error_dev(self, "could not find data bulk in\n");
		return;
	}
	if (un->un_ed[USBNET_ENDPT_TX] == 0) {
		aprint_error_dev(self, "could not find data bulk out\n");
		return;
	}

	ue = (const usb_cdc_ethernet_descriptor_t *)usb_find_desc_if(dev,
	    UDESC_CS_INTERFACE, UDESCSUB_CDC_ENF,
	    usbd_get_interface_descriptor(uiaa->uiaa_iface));
	if (!ue || usbd_get_string(dev, ue->iMacAddress, eaddr_str) ||
	    ether_aton_r(un->un_eaddr, sizeof(un->un_eaddr), eaddr_str)) {
		aprint_normal_dev(self, "faking address\n");
		un->un_eaddr[0] = 0x2a;
		uint32_t ticks = getticks();
		memcpy(&un->un_eaddr[1], &ticks, sizeof(ticks));
		un->un_eaddr[5] = (uint8_t)(device_unit(un->un_dev));
	}

	/* Query NTB tranfers sizes */
	req.bmRequestType = UT_READ_CLASS_INTERFACE;
	req.bRequest = NCM_GET_NTB_PARAMETERS;
	USETW(req.wValue, 0);
	USETW(req.wIndex, uiaa->uiaa_ifaceno);
	USETW(req.wLength, sizeof(np));
	if (usbd_do_request(un->un_udev, &req, &np) != USBD_NORMAL_COMPLETION ||
	    UGETW(np.wLength) != sizeof(np)) {
		aprint_error_dev(un->un_dev,
		    "NCM_GET_NTB_PARAMETERS failed\n");
		return;
	}
	un->un_rx_list_cnt = 1;
	un->un_tx_list_cnt = 1;
	un->un_rx_bufsz = UGETDW(np.dwNtbInMaxSize);
	un->un_tx_bufsz = UGETDW(np.dwNtbOutMaxSize);
	if (un->un_tx_bufsz < NCM_MIN_TX_BUFSZ) {
		aprint_error_dev(un->un_dev, "dwNtbOutMaxSize %u too small\n",
		    un->un_tx_bufsz);
		return;
	}

	usbnet_attach(un);
	usbnet_attach_ifp(un, IFF_SIMPLEX | IFF_BROADCAST | IFF_MULTICAST,
	    0, NULL);

	/* XXX There is no link state, pretend we are always on */
	if_link_state_change(usbnet_ifp(un), LINK_STATE_UP);
}

static void
ncm_uno_rx_loop(struct usbnet *un, struct usbnet_chain *c, uint32_t total_len)
{
	struct ifnet		*ifp = usbnet_ifp(un);
	uint8_t 		*buf = c->unc_buf;
	struct ncm_header16 	*hdr;
	struct ncm_dptab16 	*ptr;
	unsigned		 i;

	if (total_len < sizeof(*hdr) + sizeof(*ptr)) {
		aprint_error_dev(un->un_dev, "got a too small usb message\n");
		if_statinc(ifp, if_ierrors);
		return;
	}
	hdr = (struct ncm_header16 *)buf;
	if (UGETDW(hdr->dwSignature) != NCM_HDR16_SIG) {
		aprint_error_dev(un->un_dev,
		    "got a non NCM_HDR16_SIG header %08x\n",
		    UGETDW(hdr->dwSignature));
		return;
	}
	const unsigned ndp_index = UGETW(hdr->wNdpIndex);

	if (ndp_index < sizeof(*hdr) ||
	    ndp_index > total_len - sizeof(*ptr)) {
		aprint_error_dev(un->un_dev, "ndp start offset %d "
		    "bigger than data sizeof(*hdr) %zu sizeof(*ptr) %zu "
		    "total_len %u\n", ndp_index,
		    sizeof(*hdr), sizeof(*ptr),
		    total_len);
		if_statinc(ifp, if_ierrors);
		return;
	}
	ptr = (struct ncm_dptab16 *)(buf + ndp_index);
	if (UGETDW(ptr->dwSignature) != NCM_PTR16_SIG_NO_CRC) {
		aprint_error_dev(un->un_dev, "ncm dptab16 signature %08x is "
		    "weird\n", UGETDW(ptr->dwSignature));
		if_statinc(ifp, if_ierrors);
		return;
	}

	if ((unsigned)UGETW(ptr->wLength) > total_len - ndp_index) {
		aprint_error_dev(un->un_dev, "dptab16 wlength %u goes off end "
		    "ndp_index %u total_len %u\n", UGETW(ptr->wLength),
		    ndp_index, total_len);
		if_statinc(ifp, if_ierrors);
		return;
	}

	const unsigned max_datagrams = (UGETW(ptr->wLength) - 
	    offsetof(struct ncm_dptab16, datagram))/sizeof(ptr->datagram[0]);
	for (i = 0; i < max_datagrams; i++) {
		uint16_t data_start = UGETW(ptr->datagram[i].wDatagramIndex);
		uint16_t data_len = UGETW(ptr->datagram[i].wDatagramLength);

		if (data_start == 0 || data_len == 0)
			break;

		if (data_len > total_len ||
		    data_start > total_len - data_len) {
			aprint_error_dev(un->un_dev,
			    "datagram points out of buffer\n");
			if_statinc(ifp, if_ierrors);
			return;
		}
		usbnet_enqueue(un, buf + data_start, data_len, 0, 0, 0);
	}
}

static unsigned
ncm_uno_tx_prepare(struct usbnet *un, struct mbuf *m, struct usbnet_chain *c)
{
	struct ncm_softc *sc = un->un_sc;
	struct ncm_dptab16 *ptr;
	struct ncm_header16 *hdr;
	unsigned hdr_len, len;

	hdr_len = sizeof(*hdr) + sizeof(*ptr);
	len = m->m_pkthdr.len;
	KASSERT(hdr_len <= un->un_tx_bufsz);
	KASSERT(len <= un->un_tx_bufsz - hdr_len);

	hdr = (struct ncm_header16 *)c->unc_buf;
	ptr = (struct ncm_dptab16 *)(hdr + 1);
	memset(hdr, 0, sizeof(*hdr));
	memset(ptr, 0, sizeof(*ptr));

	USETDW(hdr->dwSignature, NCM_HDR16_SIG);
	USETW(hdr->wHeaderLength, sizeof(*hdr));
	USETW(hdr->wSequence, sc->sc_tx_seq);
	sc->sc_tx_seq++;

	USETW(hdr->wBlockLength, hdr_len + len);
	USETW(hdr->wNdpIndex, sizeof(*hdr));

	USETDW(ptr->dwSignature, NCM_PTR16_SIG_NO_CRC);
	USETW(ptr->wLength, sizeof(*ptr));
	USETW(ptr->wNextNdpIndex, 0);

	USETW(ptr->datagram[0].wDatagramIndex, hdr_len);
	USETW(ptr->datagram[0].wDatagramLength, len);

	USETW(ptr->datagram[1].wDatagramIndex, 0);
	USETW(ptr->datagram[1].wDatagramLength, 0);

	m_copydata(m, 0, len, c->unc_buf + hdr_len);

	return len + hdr_len;
}

#ifdef _MODULE
#include "ioconf.c"
#endif

USBNET_MODULE(ncm)
