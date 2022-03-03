/*	$NetBSD: if_cdce.c,v 1.80 2022/03/03 05:56:18 riastradh Exp $ */

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
 * USB Communication Device Class (Ethernet Networking Control Model)
 * http://www.usb.org/developers/devclass_docs/usbcdc11.pdf
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_cdce.c,v 1.80 2022/03/03 05:56:18 riastradh Exp $");

#include <sys/param.h>

#include <dev/usb/usbnet.h>
#include <dev/usb/usbcdc.h>

#include <dev/usb/if_cdcereg.h>

struct cdce_type {
	struct usb_devno	 cdce_dev;
	uint16_t		 cdce_flags;
#define CDCE_ZAURUS	1
#define CDCE_NO_UNION	2
};

static const struct cdce_type cdce_devs[] = {
  {{ USB_VENDOR_ACERLABS, USB_PRODUCT_ACERLABS_M5632 }, CDCE_NO_UNION },
  {{ USB_VENDOR_COMPAQ, USB_PRODUCT_COMPAQ_IPAQLINUX }, CDCE_NO_UNION },
  {{ USB_VENDOR_GMATE, USB_PRODUCT_GMATE_YP3X00 }, CDCE_NO_UNION },
  {{ USB_VENDOR_MOTOROLA2, USB_PRODUCT_MOTOROLA2_USBLAN }, CDCE_ZAURUS | CDCE_NO_UNION },
  {{ USB_VENDOR_MOTOROLA2, USB_PRODUCT_MOTOROLA2_USBLAN2 }, CDCE_ZAURUS | CDCE_NO_UNION },
  {{ USB_VENDOR_PROLIFIC, USB_PRODUCT_PROLIFIC_PL2501 }, CDCE_NO_UNION },
  {{ USB_VENDOR_SHARP, USB_PRODUCT_SHARP_SL5500 }, CDCE_ZAURUS },
  {{ USB_VENDOR_SHARP, USB_PRODUCT_SHARP_A300 }, CDCE_ZAURUS | CDCE_NO_UNION },
  {{ USB_VENDOR_SHARP, USB_PRODUCT_SHARP_SL5600 }, CDCE_ZAURUS | CDCE_NO_UNION },
  {{ USB_VENDOR_SHARP, USB_PRODUCT_SHARP_C700 }, CDCE_ZAURUS | CDCE_NO_UNION },
  {{ USB_VENDOR_SHARP, USB_PRODUCT_SHARP_C750 }, CDCE_ZAURUS | CDCE_NO_UNION },
};
#define cdce_lookup(v, p) \
	((const struct cdce_type *)usb_lookup(cdce_devs, v, p))

static int	cdce_match(device_t, cfdata_t, void *);
static void	cdce_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(cdce, sizeof(struct usbnet), cdce_match, cdce_attach,
    usbnet_detach, usbnet_activate);

static void	cdce_uno_rx_loop(struct usbnet *, struct usbnet_chain *,
				 uint32_t);
static unsigned	cdce_uno_tx_prepare(struct usbnet *, struct mbuf *,
				    struct usbnet_chain *);

static const struct usbnet_ops cdce_ops = {
	.uno_tx_prepare = cdce_uno_tx_prepare,
	.uno_rx_loop = cdce_uno_rx_loop,
};

static int
cdce_match(device_t parent, cfdata_t match, void *aux)
{
	struct usbif_attach_arg *uiaa = aux;

	if (cdce_lookup(uiaa->uiaa_vendor, uiaa->uiaa_product) != NULL)
		return UMATCH_VENDOR_PRODUCT;

	if (uiaa->uiaa_class == UICLASS_CDC && uiaa->uiaa_subclass ==
	    UISUBCLASS_ETHERNET_NETWORKING_CONTROL_MODEL)
		return UMATCH_IFACECLASS_GENERIC;

	return UMATCH_NONE;
}

static void
cdce_attach(device_t parent, device_t self, void *aux)
{
	struct usbnet * const		 un = device_private(self);
	struct usbif_attach_arg		*uiaa = aux;
	char				*devinfop;
	struct usbd_device	        *dev = uiaa->uiaa_device;
	const struct cdce_type		*t;
	usb_interface_descriptor_t	*id;
	usb_endpoint_descriptor_t	*ed;
	const usb_cdc_union_descriptor_t *ud;
	usb_config_descriptor_t		*cd;
	int				 data_ifcno;
	int				 i, j, numalts;
	const usb_cdc_ethernet_descriptor_t *ue;
	char				 eaddr_str[USB_MAX_ENCODED_STRING_LEN];

	aprint_naive("\n");
	aprint_normal("\n");
	devinfop = usbd_devinfo_alloc(dev, 0);
	aprint_normal_dev(self, "%s\n", devinfop);
	usbd_devinfo_free(devinfop);

	un->un_dev = self;
	un->un_udev = dev;
	un->un_sc = un;
	un->un_ops = &cdce_ops;
	un->un_rx_xfer_flags = USBD_SHORT_XFER_OK;
	un->un_tx_xfer_flags = USBD_FORCE_SHORT_XFER;
	un->un_rx_list_cnt = CDCE_RX_LIST_CNT;
	un->un_tx_list_cnt = CDCE_TX_LIST_CNT;
	un->un_rx_bufsz = CDCE_BUFSZ;
	un->un_tx_bufsz = CDCE_BUFSZ;

	t = cdce_lookup(uiaa->uiaa_vendor, uiaa->uiaa_product);
	if (t)
		un->un_flags = t->cdce_flags;

	if (un->un_flags & CDCE_NO_UNION)
		un->un_iface = uiaa->uiaa_iface;
	else {
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

	usbnet_attach(un, "cdcedet");
	usbnet_attach_ifp(un, IFF_SIMPLEX | IFF_BROADCAST | IFF_MULTICAST,
            0, NULL);
}

static void
cdce_uno_rx_loop(struct usbnet * un, struct usbnet_chain *c, uint32_t total_len)
{
	struct ifnet		*ifp = usbnet_ifp(un);

	/* Strip off CRC added by Zaurus, if present */
	if (un->un_flags & CDCE_ZAURUS && total_len > 4)
		total_len -= 4;

	if (total_len < sizeof(struct ether_header)) {
		if_statinc(ifp, if_ierrors);
		return;
	}

	usbnet_enqueue(un, c->unc_buf, total_len, 0, 0, 0);
}

static unsigned
cdce_uno_tx_prepare(struct usbnet *un, struct mbuf *m, struct usbnet_chain *c)
{
	/* Zaurus wants a 32-bit CRC appended to every frame */
	uint32_t		 crc;
	unsigned		 extra = 0;
	unsigned		 length;

	if (un->un_flags & CDCE_ZAURUS)
		extra = sizeof(crc);

	if ((unsigned)m->m_pkthdr.len > un->un_tx_bufsz - extra)
		return 0;
	length = m->m_pkthdr.len + extra;

	m_copydata(m, 0, m->m_pkthdr.len, c->unc_buf);
	if (un->un_flags & CDCE_ZAURUS) {
		crc = htole32(~ether_crc32_le(c->unc_buf, m->m_pkthdr.len));
		memcpy(c->unc_buf + m->m_pkthdr.len, &crc, sizeof(crc));
	}

	return length;
}

#ifdef _MODULE
#include "ioconf.c"
#endif

USBNET_MODULE(cdce)
