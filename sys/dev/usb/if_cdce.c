/*	$NetBSD: if_cdce.c,v 1.38.14.10 2016/12/12 13:15:39 skrll Exp $ */

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
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_cdce.c,v 1.38.14.10 2016/12/12 13:15:39 skrll Exp $");

#ifdef _KERNEL_OPT
#include "opt_inet.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/sockio.h>
#include <sys/mbuf.h>
#include <sys/kernel.h>
#include <sys/socket.h>
#include <sys/device.h>

#include <net/if.h>
#include <net/if_arp.h>
#include <net/if_dl.h>
#include <net/if_media.h>

#include <net/bpf.h>

#include <net/if_ether.h>
#ifdef INET
#include <netinet/in.h>
#include <netinet/if_inarp.h>
#endif



#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdi_util.h>
#include <dev/usb/usbdevs.h>
#include <dev/usb/usbcdc.h>

#include <dev/usb/if_cdcereg.h>

Static int	 cdce_tx_list_init(struct cdce_softc *);
Static void	 cdce_tx_list_free(struct cdce_softc *);
Static int	 cdce_rx_list_init(struct cdce_softc *);
Static void	 cdce_rx_list_free(struct cdce_softc *);
Static int	 cdce_newbuf(struct cdce_softc *, struct cdce_chain *,
		    struct mbuf *);
Static int	 cdce_encap(struct cdce_softc *, struct mbuf *, int);
Static void	 cdce_rxeof(struct usbd_xfer *, void *, usbd_status);
Static void	 cdce_txeof(struct usbd_xfer *, void *, usbd_status);
Static void	 cdce_start(struct ifnet *);
Static void	 cdce_start_locked(struct ifnet *);
Static int	 cdce_ioctl(struct ifnet *, u_long, void *);
Static int	 cdce_init(struct ifnet *);
Static int	 cdce_init_locked(struct ifnet *);
Static void	 cdce_watchdog(struct ifnet *);
Static void	 cdce_stop(struct ifnet *, int);
Static void	 cdce_stop_locked(struct ifnet *, int);

Static const struct cdce_type cdce_devs[] = {
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

int cdce_match(device_t, cfdata_t, void *);
void cdce_attach(device_t, device_t, void *);
int cdce_detach(device_t, int);
int cdce_activate(device_t, enum devact);
extern struct cfdriver cdce_cd;
CFATTACH_DECL_NEW(cdce, sizeof(struct cdce_softc), cdce_match, cdce_attach,
    cdce_detach, cdce_activate);

int
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

void
cdce_attach(device_t parent, device_t self, void *aux)
{
	struct cdce_softc *sc = device_private(self);
	struct usbif_attach_arg *uiaa = aux;
	char				*devinfop;
	struct ifnet			*ifp;
	struct usbd_device	        *dev = uiaa->uiaa_device;
	const struct cdce_type		*t;
	usb_interface_descriptor_t	*id;
	usb_endpoint_descriptor_t	*ed;
	const usb_cdc_union_descriptor_t *ud;
	usb_config_descriptor_t		*cd;
	int				 data_ifcno;
	int				 i, j, numalts;
	u_char				 eaddr[ETHER_ADDR_LEN];
	const usb_cdc_ethernet_descriptor_t *ue;
	char				 eaddr_str[USB_MAX_ENCODED_STRING_LEN];

	sc->cdce_dev = self;

	aprint_naive("\n");
	aprint_normal("\n");

	devinfop = usbd_devinfo_alloc(dev, 0);
	aprint_normal_dev(self, "%s\n", devinfop);
	usbd_devinfo_free(devinfop);

	mutex_init(&sc->cdce_lock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&sc->cdce_txlock, MUTEX_DEFAULT, IPL_SOFTUSB);
	mutex_init(&sc->cdce_rxlock, MUTEX_DEFAULT, IPL_SOFTUSB);

	sc->cdce_udev = uiaa->uiaa_device;
	sc->cdce_ctl_iface = uiaa->uiaa_iface;

	t = cdce_lookup(uiaa->uiaa_vendor, uiaa->uiaa_product);
	if (t)
		sc->cdce_flags = t->cdce_flags;

	if (sc->cdce_flags & CDCE_NO_UNION)
		sc->cdce_data_iface = sc->cdce_ctl_iface;
	else {
		ud = (const usb_cdc_union_descriptor_t *)usb_find_desc(sc->cdce_udev,
		    UDESC_CS_INTERFACE, UDESCSUB_CDC_UNION);
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
					sc->cdce_data_iface = uiaa->uiaa_ifaces[i];
					uiaa->uiaa_ifaces[i] = NULL;
				}
			}
		}
	}

	if (sc->cdce_data_iface == NULL) {
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
	id = usbd_get_interface_descriptor(sc->cdce_data_iface);
	cd = usbd_get_config_descriptor(sc->cdce_udev);
	numalts = usbd_get_no_alts(cd, id->bInterfaceNumber);

	for (j = 0; j < numalts; j++) {
		if (usbd_set_interface(sc->cdce_data_iface, j)) {
			aprint_error_dev(sc->cdce_dev,
					"setting alternate interface failed\n");
			return;
		}
		/* Find endpoints. */
		id = usbd_get_interface_descriptor(sc->cdce_data_iface);
		sc->cdce_bulkin_no = sc->cdce_bulkout_no = -1;
		for (i = 0; i < id->bNumEndpoints; i++) {
			ed = usbd_interface2endpoint_descriptor(sc->cdce_data_iface, i);
			if (!ed) {
				aprint_error_dev(self,
						"could not read endpoint descriptor\n");
				return;
			}
			if (UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_IN &&
					UE_GET_XFERTYPE(ed->bmAttributes) == UE_BULK) {
				sc->cdce_bulkin_no = ed->bEndpointAddress;
			} else if (UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_OUT &&
					UE_GET_XFERTYPE(ed->bmAttributes) == UE_BULK) {
				sc->cdce_bulkout_no = ed->bEndpointAddress;
			} else if (UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_IN &&
					UE_GET_XFERTYPE(ed->bmAttributes) == UE_INTERRUPT) {
				/* XXX: CDC spec defines an interrupt pipe, but it is not
				 * needed for simple host-to-host applications. */
			} else {
				aprint_error_dev(self, "unexpected endpoint\n");
			}
		}
		/* If we found something, try and use it... */
		if ((sc->cdce_bulkin_no != -1) && (sc->cdce_bulkout_no != -1))
			break;
	}

	if (sc->cdce_bulkin_no == -1) {
		aprint_error_dev(self, "could not find data bulk in\n");
		return;
	}
	if (sc->cdce_bulkout_no == -1 ) {
		aprint_error_dev(self, "could not find data bulk out\n");
		return;
	}

	ue = (const usb_cdc_ethernet_descriptor_t *)usb_find_desc(dev,
	    UDESC_CS_INTERFACE, UDESCSUB_CDC_ENF);
	if (!ue || usbd_get_string(dev, ue->iMacAddress, eaddr_str) ||
	    ether_aton_r(eaddr, sizeof(eaddr), eaddr_str)) {
		aprint_normal_dev(self, "faking address\n");
		eaddr[0]= 0x2a;
		memcpy(&eaddr[1], &hardclock_ticks, sizeof(uint32_t));
		eaddr[5] = (uint8_t)(device_unit(sc->cdce_dev));
	}

	aprint_normal_dev(self, "address %s\n", ether_sprintf(eaddr));

	ifp = GET_IFP(sc);
	ifp->if_softc = sc;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_extflags = IFEF_START_MPSAFE;
	ifp->if_init = cdce_init;
	ifp->if_ioctl = cdce_ioctl;
	ifp->if_start = cdce_start;
	ifp->if_watchdog = cdce_watchdog;
	strncpy(ifp->if_xname, device_xname(sc->cdce_dev), IFNAMSIZ);

	IFQ_SET_READY(&ifp->if_snd);

	if_initialize(ifp);
	sc->cdce_ipq = if_percpuq_create(&sc->cdce_ec.ec_if);
	ether_ifattach(ifp, eaddr);
	if_register(ifp);

	sc->cdce_attached = 1;

	usbd_add_drv_event(USB_EVENT_DRIVER_ATTACH, sc->cdce_udev,
	    sc->cdce_dev);

	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n");

	return;
}

int
cdce_detach(device_t self, int flags)
{
	struct cdce_softc *sc = device_private(self);
	struct ifnet	*ifp = GET_IFP(sc);
	int		 s;

	pmf_device_deregister(self);

	s = splusb();

	if (!sc->cdce_attached) {
		splx(s);
		return 0;
	}

	if (ifp->if_flags & IFF_RUNNING)
		cdce_stop(ifp, 1);

	ether_ifdetach(ifp);

	if_detach(ifp);

	sc->cdce_attached = 0;
	splx(s);

	return 0;
}

Static void
cdce_start(struct ifnet *ifp)
{
	struct cdce_softc	*sc = ifp->if_softc;
	KASSERT(ifp->if_extflags & IFEF_START_MPSAFE);

	mutex_enter(&sc->cdce_txlock);
	cdce_start_locked(ifp);
	mutex_exit(&sc->cdce_txlock);
}

Static void
cdce_start_locked(struct ifnet *ifp)
{
	struct cdce_softc	*sc = ifp->if_softc;
	struct mbuf		*m_head = NULL;

	if (sc->cdce_dying || (ifp->if_flags & IFF_OACTIVE))
		return;

	IFQ_POLL(&ifp->if_snd, m_head);
	if (m_head == NULL)
		return;

	if (cdce_encap(sc, m_head, 0)) {
		return;
	}

	IFQ_DEQUEUE(&ifp->if_snd, m_head);

	bpf_mtap(ifp, m_head);

	ifp->if_flags |= IFF_OACTIVE;

	ifp->if_timer = 6;
}

Static int
cdce_encap(struct cdce_softc *sc, struct mbuf *m, int idx)
{
	struct cdce_chain	*c = &sc->cdce_cdata.cdce_tx_chain[idx];
	int			 extra = 0;

	m_copydata(m, 0, m->m_pkthdr.len, c->cdce_buf);
	if (sc->cdce_flags & CDCE_ZAURUS) {
		/* Zaurus wants a 32-bit CRC appended to every frame */
		uint32_t crc;

		crc = htole32(~ether_crc32_le(c->cdce_buf, m->m_pkthdr.len));
		memcpy(c->cdce_buf + m->m_pkthdr.len, &crc, sizeof(crc));
		extra = sizeof(crc);
	}
	c->cdce_mbuf = m;

	usbd_setup_xfer(c->cdce_xfer, c, c->cdce_buf, m->m_pkthdr.len + extra,
	    USBD_FORCE_SHORT_XFER, 10000, cdce_txeof);
	usbd_status err = usbd_transfer(c->cdce_xfer);
	if (err != USBD_IN_PROGRESS) {
		cdce_stop(GET_IFP(sc), 0);
		return EIO;
	}

	sc->cdce_cdata.cdce_tx_cnt++;

	return 0;
}

Static void
cdce_stop(struct ifnet *ifp, int disable)
{
	struct cdce_softc * const sc = ifp->if_softc;

	mutex_enter(&sc->cdce_lock);
	cdce_stop_locked(ifp, disable);
	mutex_exit(&sc->cdce_lock);
}

Static void
cdce_stop_locked(struct ifnet *ifp, int disable)
{
	struct cdce_softc * const sc = ifp->if_softc;
	usbd_status	 err;

	ifp->if_timer = 0;

	if (sc->cdce_bulkin_pipe != NULL) {
		err = usbd_abort_pipe(sc->cdce_bulkin_pipe);
		if (err)
			printf("%s: abort rx pipe failed: %s\n",
			    device_xname(sc->cdce_dev), usbd_errstr(err));
	}

	if (sc->cdce_bulkout_pipe != NULL) {
		err = usbd_abort_pipe(sc->cdce_bulkout_pipe);
		if (err)
			printf("%s: abort tx pipe failed: %s\n",
			    device_xname(sc->cdce_dev), usbd_errstr(err));
	}

	cdce_rx_list_free(sc);

	cdce_tx_list_free(sc);

	if (sc->cdce_bulkin_pipe != NULL) {
		err = usbd_close_pipe(sc->cdce_bulkin_pipe);
		if (err)
			printf("%s: close rx pipe failed: %s\n",
			    device_xname(sc->cdce_dev), usbd_errstr(err));
		sc->cdce_bulkin_pipe = NULL;
	}

	if (sc->cdce_bulkout_pipe != NULL) {
		err = usbd_close_pipe(sc->cdce_bulkout_pipe);
		if (err)
			printf("%s: close tx pipe failed: %s\n",
			    device_xname(sc->cdce_dev), usbd_errstr(err));
		sc->cdce_bulkout_pipe = NULL;
	}

	ifp->if_timer = 0;
	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
}

Static int
cdce_ioctl(struct ifnet *ifp, u_long command, void *data)
{
	struct cdce_softc	*sc = ifp->if_softc;
	int			 s, error = 0;

	if (sc->cdce_dying)
		return EIO;

	s = splnet();
	error = ether_ioctl(ifp, command, data);
	splx(s);

	if (error == ENETRESET)
		error = 0;

	return error;
}

Static void
cdce_watchdog(struct ifnet *ifp)
{
	struct cdce_softc	*sc = ifp->if_softc;

	if (sc->cdce_dying)
		return;

	ifp->if_oerrors++;
	printf("%s: watchdog timeout\n", device_xname(sc->cdce_dev));
}

Static int
cdce_init(struct ifnet *ifp)
{
	struct cdce_softc * const sc = ifp->if_softc;

	mutex_enter(&sc->cdce_lock);
	int ret = cdce_init_locked(ifp);
	mutex_exit(&sc->cdce_lock);

	return ret;
}

Static int
cdce_init_locked(struct ifnet *ifp)
{
	struct cdce_softc * const sc = ifp->if_softc;
	struct cdce_chain	*c;
	usbd_status		 err;
	int			 i;

	err = usbd_open_pipe(sc->cdce_data_iface, sc->cdce_bulkin_no,
	    USBD_EXCLUSIVE_USE, &sc->cdce_bulkin_pipe);
	if (err) {
		printf("%s: open rx pipe failed: %s\n", device_xname(sc->cdce_dev),
		    usbd_errstr(err));
		goto fail;
	}

	err = usbd_open_pipe(sc->cdce_data_iface, sc->cdce_bulkout_no,
	    USBD_EXCLUSIVE_USE, &sc->cdce_bulkout_pipe);
	if (err) {
		printf("%s: open tx pipe failed: %s\n",
		    device_xname(sc->cdce_dev), usbd_errstr(err));
		goto fail1;
	}

	if (cdce_tx_list_init(sc)) {
		printf("%s: tx list init failed\n", device_xname(sc->cdce_dev));
		goto fail2;
	}

	if (cdce_rx_list_init(sc)) {
		printf("%s: rx list init failed\n", device_xname(sc->cdce_dev));
		goto fail3;
	}

	for (i = 0; i < CDCE_RX_LIST_CNT; i++) {
		c = &sc->cdce_cdata.cdce_rx_chain[i];

		usbd_setup_xfer(c->cdce_xfer, c, c->cdce_buf, CDCE_BUFSZ,
		    USBD_SHORT_XFER_OK, USBD_NO_TIMEOUT, cdce_rxeof);
		usbd_transfer(c->cdce_xfer);
	}

	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;

	return 0;

fail3:
	cdce_tx_list_free(sc);
fail2:
	usbd_close_pipe(sc->cdce_bulkout_pipe);
fail1:
	usbd_close_pipe(sc->cdce_bulkin_pipe);
fail:
	return EIO;
}

Static int
cdce_newbuf(struct cdce_softc *sc, struct cdce_chain *c, struct mbuf *m)
{
	struct mbuf	*m_new = NULL;

	if (m == NULL) {
		MGETHDR(m_new, M_DONTWAIT, MT_DATA);
		if (m_new == NULL) {
			printf("%s: no memory for rx list "
			    "-- packet dropped!\n", device_xname(sc->cdce_dev));
			return ENOBUFS;
		}
		MCLGET(m_new, M_DONTWAIT);
		if (!(m_new->m_flags & M_EXT)) {
			printf("%s: no memory for rx list "
			    "-- packet dropped!\n", device_xname(sc->cdce_dev));
			m_freem(m_new);
			return ENOBUFS;
		}
		m_new->m_len = m_new->m_pkthdr.len = MCLBYTES;
	} else {
		m_new = m;
		m_new->m_len = m_new->m_pkthdr.len = MCLBYTES;
		m_new->m_data = m_new->m_ext.ext_buf;
	}
	c->cdce_mbuf = m_new;
	return 0;
}

Static int
cdce_rx_list_init(struct cdce_softc *sc)
{
	struct cdce_cdata	*cd = &sc->cdce_cdata;
	struct cdce_chain	*c;
	int			 i;

	for (i = 0; i < CDCE_RX_LIST_CNT; i++) {
		c = &cd->cdce_rx_chain[i];
		c->cdce_sc = sc;
		c->cdce_idx = i;
		if (cdce_newbuf(sc, c, NULL) == ENOBUFS)
			return ENOBUFS;
		if (c->cdce_xfer == NULL) {
			int err = usbd_create_xfer(sc->cdce_bulkin_pipe,
			    CDCE_BUFSZ, USBD_SHORT_XFER_OK, 0, &c->cdce_xfer);
			if (err)
				return err;
			c->cdce_buf = usbd_get_buffer(c->cdce_xfer);
		}
	}

	return 0;
}

Static void
cdce_rx_list_free(struct cdce_softc *sc)
{
	/* Free RX resources */
	for (size_t i = 0; i < CDCE_RX_LIST_CNT; i++) {
		if (sc->cdce_cdata.cdce_rx_chain[i].cdce_mbuf != NULL) {
			m_freem(sc->cdce_cdata.cdce_rx_chain[i].cdce_mbuf);
			sc->cdce_cdata.cdce_rx_chain[i].cdce_mbuf = NULL;
		}
		if (sc->cdce_cdata.cdce_rx_chain[i].cdce_xfer != NULL) {
			usbd_destroy_xfer(sc->cdce_cdata.cdce_rx_chain[i].cdce_xfer);
			sc->cdce_cdata.cdce_rx_chain[i].cdce_xfer = NULL;
		}
	}
}

Static int
cdce_tx_list_init(struct cdce_softc *sc)
{
	struct cdce_cdata	*cd = &sc->cdce_cdata;
	struct cdce_chain	*c;
	int			 i;

	for (i = 0; i < CDCE_TX_LIST_CNT; i++) {
		c = &cd->cdce_tx_chain[i];
		c->cdce_sc = sc;
		c->cdce_idx = i;
		c->cdce_mbuf = NULL;
		if (c->cdce_xfer == NULL) {
			int err = usbd_create_xfer(sc->cdce_bulkout_pipe,
			    CDCE_BUFSZ, USBD_FORCE_SHORT_XFER, 0,
			    &c->cdce_xfer);
			if (err)
				return err;
			c->cdce_buf = usbd_get_buffer(c->cdce_xfer);
		}
	}

	return 0;
}

Static void
cdce_tx_list_free(struct cdce_softc *sc)
{
	/* Free TX resources */
	for (size_t i = 0; i < CDCE_TX_LIST_CNT; i++) {
		if (sc->cdce_cdata.cdce_tx_chain[i].cdce_mbuf != NULL) {
			m_freem(sc->cdce_cdata.cdce_tx_chain[i].cdce_mbuf);
			sc->cdce_cdata.cdce_tx_chain[i].cdce_mbuf = NULL;
		}
		if (sc->cdce_cdata.cdce_tx_chain[i].cdce_xfer != NULL) {
			usbd_destroy_xfer(sc->cdce_cdata.cdce_tx_chain[i].cdce_xfer);
			sc->cdce_cdata.cdce_tx_chain[i].cdce_xfer = NULL;
		}
	}
}

Static void
cdce_rxeof(struct usbd_xfer *xfer, void *priv, usbd_status status)
{
	struct cdce_chain	*c = priv;
	struct cdce_softc	*sc = c->cdce_sc;
	struct ifnet		*ifp = GET_IFP(sc);
	struct mbuf		*m;
	int			 total_len = 0;

	if (sc->cdce_dying || !(ifp->if_flags & IFF_RUNNING))
		return;

	if (status != USBD_NORMAL_COMPLETION) {
		if (status == USBD_NOT_STARTED || status == USBD_CANCELLED)
			return;
		if (sc->cdce_rxeof_errors == 0)
			printf("%s: usb error on rx: %s\n",
			    device_xname(sc->cdce_dev), usbd_errstr(status));
		if (status == USBD_STALLED)
			usbd_clear_endpoint_stall_async(sc->cdce_bulkin_pipe);
		DELAY(sc->cdce_rxeof_errors * 10000);
		sc->cdce_rxeof_errors++;
		goto done;
	}

	sc->cdce_rxeof_errors = 0;

	usbd_get_xfer_status(xfer, NULL, NULL, &total_len, NULL);
	if (sc->cdce_flags & CDCE_ZAURUS)
		total_len -= 4;	/* Strip off CRC added by Zaurus */
	if (total_len <= 1)
		goto done;

	m = c->cdce_mbuf;
	memcpy(mtod(m, char *), c->cdce_buf, total_len);

	if (total_len < sizeof(struct ether_header)) {
		ifp->if_ierrors++;
		goto done;
	}

	ifp->if_ipackets++;
	m->m_pkthdr.len = m->m_len = total_len;
	m_set_rcvif(m, ifp);

	if (cdce_newbuf(sc, c, NULL) == ENOBUFS) {
		ifp->if_ierrors++;
		goto done;
	}

	bpf_mtap(ifp, m);

	if_percpuq_enqueue(sc->cdce_ipq, (m));

done:
	/* Setup new transfer. */
	usbd_setup_xfer(c->cdce_xfer, c, c->cdce_buf, CDCE_BUFSZ,
	    USBD_SHORT_XFER_OK, USBD_NO_TIMEOUT, cdce_rxeof);
	usbd_transfer(c->cdce_xfer);
}

Static void
cdce_txeof(struct usbd_xfer *xfer, void *priv,
    usbd_status status)
{
	struct cdce_chain	*c = priv;
	struct cdce_softc	*sc = c->cdce_sc;
	struct ifnet		*ifp = GET_IFP(sc);
	usbd_status		 err;

	if (sc->cdce_dying)
		return;

	int s = splnet();

	ifp->if_timer = 0;
	ifp->if_flags &= ~IFF_OACTIVE;

	if (status != USBD_NORMAL_COMPLETION) {
		if (status == USBD_NOT_STARTED || status == USBD_CANCELLED) {
			splx(s);
			return;
		}
		ifp->if_oerrors++;
		printf("%s: usb error on tx: %s\n", device_xname(sc->cdce_dev),
		    usbd_errstr(status));
		if (status == USBD_STALLED)
			usbd_clear_endpoint_stall_async(sc->cdce_bulkout_pipe);
		splx(s);
		return;
	}

	usbd_get_xfer_status(c->cdce_xfer, NULL, NULL, NULL, &err);

	if (c->cdce_mbuf != NULL) {
		m_freem(c->cdce_mbuf);
		c->cdce_mbuf = NULL;
	}

	if (err)
		ifp->if_oerrors++;
	else
		ifp->if_opackets++;

	if (IFQ_IS_EMPTY(&ifp->if_snd) == 0)
		cdce_start(ifp);

	splx(s);
}

int
cdce_activate(device_t self, enum devact act)
{
	struct cdce_softc *sc = device_private(self);

	switch (act) {
	case DVACT_DEACTIVATE:
		if_deactivate(GET_IFP(sc));
		sc->cdce_dying = 1;
		return 0;
	default:
		return EOPNOTSUPP;
	}
}
