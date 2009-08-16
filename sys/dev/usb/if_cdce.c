/*	$NetBSD: if_cdce.c,v 1.22 2009/08/16 14:18:49 tron Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: if_cdce.c,v 1.22 2009/08/16 14:18:49 tron Exp $");
#include "bpfilter.h"
#ifdef	__NetBSD__
#include "opt_inet.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/sockio.h>
#include <sys/mbuf.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/socket.h>
#include <sys/device.h>
#if defined(__OpenBSD__)
#include <sys/proc.h>
#endif

#if NRND > 0
#include <sys/rnd.h>
#endif

#include <net/if.h>
#if defined(__NetBSD__)
#include <net/if_arp.h>
#endif
#include <net/if_dl.h>
#include <net/if_media.h>

#define BPF_MTAP(ifp, m) bpf_mtap((ifp)->if_bpf, (m))

#if NBPFILTER > 0
#include <net/bpf.h>
#endif

#if defined(__NetBSD__)
#include <net/if_ether.h>
#ifdef INET
#include <netinet/in.h>
#include <netinet/if_inarp.h>
#endif
#endif /* defined(__NetBSD__) */

#if defined(__OpenBSD__)
#ifdef INET
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#include <netinet/if_ether.h>
#endif
#endif /* defined(__OpenBSD__) */


#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdi_util.h>
#include <dev/usb/usbdevs.h>
#include <dev/usb/usbcdc.h>

#include <dev/usb/if_cdcereg.h>

Static int	 cdce_tx_list_init(struct cdce_softc *);
Static int	 cdce_rx_list_init(struct cdce_softc *);
Static int	 cdce_newbuf(struct cdce_softc *, struct cdce_chain *,
		    struct mbuf *);
Static int	 cdce_encap(struct cdce_softc *, struct mbuf *, int);
Static void	 cdce_rxeof(usbd_xfer_handle, usbd_private_handle, usbd_status);
Static void	 cdce_txeof(usbd_xfer_handle, usbd_private_handle, usbd_status);
Static void	 cdce_start(struct ifnet *);
Static int	 cdce_ioctl(struct ifnet *, u_long, void *);
Static void	 cdce_init(void *);
Static void	 cdce_watchdog(struct ifnet *);
Static void	 cdce_stop(struct cdce_softc *);
Static uint32_t	 cdce_crc32(const void *, size_t);

Static const struct cdce_type cdce_devs[] = {
  {{ USB_VENDOR_PROLIFIC, USB_PRODUCT_PROLIFIC_PL2501 }, CDCE_NO_UNION },
  {{ USB_VENDOR_SHARP, USB_PRODUCT_SHARP_SL5500 }, CDCE_ZAURUS },
  {{ USB_VENDOR_SHARP, USB_PRODUCT_SHARP_A300 }, CDCE_ZAURUS | CDCE_NO_UNION },
  {{ USB_VENDOR_SHARP, USB_PRODUCT_SHARP_SL5600 }, CDCE_ZAURUS | CDCE_NO_UNION },
  {{ USB_VENDOR_SHARP, USB_PRODUCT_SHARP_C700 }, CDCE_ZAURUS | CDCE_NO_UNION },
  {{ USB_VENDOR_SHARP, USB_PRODUCT_SHARP_C750 }, CDCE_ZAURUS | CDCE_NO_UNION },
};
#define cdce_lookup(v, p) ((const struct cdce_type *)usb_lookup(cdce_devs, v, p))

USB_DECLARE_DRIVER(cdce);

USB_MATCH(cdce)
{
	USB_IFMATCH_START(cdce, uaa);

	if (cdce_lookup(uaa->vendor, uaa->product) != NULL)
		return (UMATCH_VENDOR_PRODUCT);

	if (uaa->class == UICLASS_CDC && uaa->subclass ==
	    UISUBCLASS_ETHERNET_NETWORKING_CONTROL_MODEL)
		return (UMATCH_IFACECLASS_GENERIC);

	return (UMATCH_NONE);
}

USB_ATTACH(cdce)
{
	USB_IFATTACH_START(cdce, sc, uaa);
	char				 *devinfop;
	int				 s;
	struct ifnet			*ifp;
	usbd_device_handle		 dev = uaa->device;
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

	devinfop = usbd_devinfo_alloc(dev, 0);
	USB_ATTACH_SETUP;
	sc->cdce_dev = self;
	aprint_normal_dev(self, "%s\n", devinfop);
	usbd_devinfo_free(devinfop);

	sc->cdce_udev = uaa->device;
	sc->cdce_ctl_iface = uaa->iface;

	t = cdce_lookup(uaa->vendor, uaa->product);
	if (t)
		sc->cdce_flags = t->cdce_flags;

	if (sc->cdce_flags & CDCE_NO_UNION)
		sc->cdce_data_iface = sc->cdce_ctl_iface;
	else {
		ud = (const usb_cdc_union_descriptor_t *)usb_find_desc(sc->cdce_udev,
		    UDESC_CS_INTERFACE, UDESCSUB_CDC_UNION);
		if (ud == NULL) {
			aprint_error_dev(self, "no union descriptor\n");
			USB_ATTACH_ERROR_RETURN;
		}
		data_ifcno = ud->bSlaveInterface[0];

		for (i = 0; i < uaa->nifaces; i++) {
			if (uaa->ifaces[i] != NULL) {
				id = usbd_get_interface_descriptor(
				    uaa->ifaces[i]);
				if (id != NULL && id->bInterfaceNumber ==
				    data_ifcno) {
					sc->cdce_data_iface = uaa->ifaces[i];
					uaa->ifaces[i] = NULL;
				}
			}
		}
	}

	if (sc->cdce_data_iface == NULL) {
		aprint_error_dev(self, "no data interface\n");
		USB_ATTACH_ERROR_RETURN;
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
			USB_ATTACH_ERROR_RETURN;
		}
		/* Find endpoints. */
		id = usbd_get_interface_descriptor(sc->cdce_data_iface);
		sc->cdce_bulkin_no = sc->cdce_bulkout_no = -1;
		for (i = 0; i < id->bNumEndpoints; i++) {
			ed = usbd_interface2endpoint_descriptor(sc->cdce_data_iface, i);
			if (!ed) {
				aprint_error_dev(self,
						"could not read endpoint descriptor\n");
				USB_ATTACH_ERROR_RETURN;
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
		USB_ATTACH_ERROR_RETURN;
	}
	if (sc->cdce_bulkout_no == -1 ) {
		aprint_error_dev(self, "could not find data bulk out\n");
		USB_ATTACH_ERROR_RETURN;
	}

	ue = (const usb_cdc_ethernet_descriptor_t *)usb_find_desc(dev,
            UDESC_INTERFACE, UDESCSUB_CDC_ENF);
	if (!ue || usbd_get_string(dev, ue->iMacAddress, eaddr_str)) {
		aprint_normal_dev(self, "faking address\n");
		eaddr[0]= 0x2a;
		memcpy(&eaddr[1], &hardclock_ticks, sizeof(u_int32_t));
		eaddr[5] = (u_int8_t)(device_unit(sc->cdce_dev));
	} else {
		(void)ether_nonstatic_aton(eaddr, eaddr_str);
	}

	s = splnet();

	aprint_normal_dev(self, "address %s\n", ether_sprintf(eaddr));

	ifp = GET_IFP(sc);
	ifp->if_softc = sc;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_ioctl = cdce_ioctl;
	ifp->if_start = cdce_start;
	ifp->if_watchdog = cdce_watchdog;
	strncpy(ifp->if_xname, USBDEVNAME(sc->cdce_dev), IFNAMSIZ);

	IFQ_SET_READY(&ifp->if_snd);

	if_attach(ifp);
	Ether_ifattach(ifp, eaddr);

	sc->cdce_attached = 1;
	splx(s);

	usbd_add_drv_event(USB_EVENT_DRIVER_ATTACH, sc->cdce_udev,
	    USBDEV(sc->cdce_dev));

	USB_ATTACH_SUCCESS_RETURN;
}

USB_DETACH(cdce)
{
	USB_DETACH_START(cdce, sc);
	struct ifnet	*ifp = GET_IFP(sc);
	int		 s;

	s = splusb();

	if (!sc->cdce_attached) {
		splx(s);
		return (0);
	}

	if (ifp->if_flags & IFF_RUNNING)
		cdce_stop(sc);

	ether_ifdetach(ifp);

	if_detach(ifp);

	sc->cdce_attached = 0;
	splx(s);

	return (0);
}

Static void
cdce_start(struct ifnet *ifp)
{
	struct cdce_softc	*sc = ifp->if_softc;
	struct mbuf		*m_head = NULL;

	if (sc->cdce_dying || (ifp->if_flags & IFF_OACTIVE))
		return;

	IFQ_POLL(&ifp->if_snd, m_head);
	if (m_head == NULL)
		return;

	if (cdce_encap(sc, m_head, 0)) {
		ifp->if_flags |= IFF_OACTIVE;
		return;
	}

	IFQ_DEQUEUE(&ifp->if_snd, m_head);

#if NBPFILTER > 0
	if (ifp->if_bpf)
		BPF_MTAP(ifp, m_head);
#endif

	ifp->if_flags |= IFF_OACTIVE;

	ifp->if_timer = 6;
}

Static int
cdce_encap(struct cdce_softc *sc, struct mbuf *m, int idx)
{
	struct cdce_chain	*c;
	usbd_status		 err;
	int			 extra = 0;

	c = &sc->cdce_cdata.cdce_tx_chain[idx];

	m_copydata(m, 0, m->m_pkthdr.len, c->cdce_buf);
	if (sc->cdce_flags & CDCE_ZAURUS) {
		/* Zaurus wants a 32-bit CRC appended to every frame */
		u_int32_t crc;

		crc = cdce_crc32(c->cdce_buf, m->m_pkthdr.len);
		memcpy(c->cdce_buf + m->m_pkthdr.len, &crc, 4);
		extra = 4;
	}
	c->cdce_mbuf = m;

	usbd_setup_xfer(c->cdce_xfer, sc->cdce_bulkout_pipe, c, c->cdce_buf,
	    m->m_pkthdr.len + extra, USBD_NO_COPY, 10000, cdce_txeof);
	err = usbd_transfer(c->cdce_xfer);
	if (err != USBD_IN_PROGRESS) {
		cdce_stop(sc);
		return (EIO);
	}

	sc->cdce_cdata.cdce_tx_cnt++;

	return (0);
}

Static void
cdce_stop(struct cdce_softc *sc)
{
	usbd_status	 err;
	struct ifnet	*ifp = GET_IFP(sc);
	int		 i;

	ifp->if_timer = 0;

	if (sc->cdce_bulkin_pipe != NULL) {
		err = usbd_abort_pipe(sc->cdce_bulkin_pipe);
		if (err)
			printf("%s: abort rx pipe failed: %s\n",
			    USBDEVNAME(sc->cdce_dev), usbd_errstr(err));
		err = usbd_close_pipe(sc->cdce_bulkin_pipe);
		if (err)
			printf("%s: close rx pipe failed: %s\n",
			    USBDEVNAME(sc->cdce_dev), usbd_errstr(err));
		sc->cdce_bulkin_pipe = NULL;
	}

	if (sc->cdce_bulkout_pipe != NULL) {
		err = usbd_abort_pipe(sc->cdce_bulkout_pipe);
		if (err)
			printf("%s: abort tx pipe failed: %s\n",
			    USBDEVNAME(sc->cdce_dev), usbd_errstr(err));
		err = usbd_close_pipe(sc->cdce_bulkout_pipe);
		if (err)
			printf("%s: close tx pipe failed: %s\n",
			    USBDEVNAME(sc->cdce_dev), usbd_errstr(err));
		sc->cdce_bulkout_pipe = NULL;
	}

	for (i = 0; i < CDCE_RX_LIST_CNT; i++) {
		if (sc->cdce_cdata.cdce_rx_chain[i].cdce_mbuf != NULL) {
			m_freem(sc->cdce_cdata.cdce_rx_chain[i].cdce_mbuf);
			sc->cdce_cdata.cdce_rx_chain[i].cdce_mbuf = NULL;
		}
		if (sc->cdce_cdata.cdce_rx_chain[i].cdce_xfer != NULL) {
			usbd_free_xfer(sc->cdce_cdata.cdce_rx_chain[i].cdce_xfer);
			sc->cdce_cdata.cdce_rx_chain[i].cdce_xfer = NULL;
		}
	}

	for (i = 0; i < CDCE_TX_LIST_CNT; i++) {
		if (sc->cdce_cdata.cdce_tx_chain[i].cdce_mbuf != NULL) {
			m_freem(sc->cdce_cdata.cdce_tx_chain[i].cdce_mbuf);
			sc->cdce_cdata.cdce_tx_chain[i].cdce_mbuf = NULL;
		}
		if (sc->cdce_cdata.cdce_tx_chain[i].cdce_xfer != NULL) {
			usbd_free_xfer(sc->cdce_cdata.cdce_tx_chain[i].cdce_xfer);
			sc->cdce_cdata.cdce_tx_chain[i].cdce_xfer = NULL;
		}
	}

	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
}

Static int
cdce_ioctl(struct ifnet *ifp, u_long command, void *data)
{
	struct cdce_softc	*sc = ifp->if_softc;
	struct ifaddr		*ifa = (struct ifaddr *)data;
	struct ifreq		*ifr = (struct ifreq *)data;
	int			 s, error = 0;

	if (sc->cdce_dying)
		return (EIO);

	s = splnet();

	switch(command) {
	case SIOCINITIFADDR:
		ifp->if_flags |= IFF_UP;
		cdce_init(sc);
		switch (ifa->ifa_addr->sa_family) {
#ifdef INET
		case AF_INET:
#if defined(__NetBSD__)
			arp_ifinit(ifp, ifa);
#else
			arp_ifinit(&sc->arpcom, ifa);
#endif
			break;
#endif /* INET */
		}
		break;

	case SIOCSIFMTU:
		if (ifr->ifr_mtu < ETHERMIN || ifr->ifr_mtu > ETHERMTU)
			error = EINVAL;
		else if ((error = ifioctl_common(ifp, command, data)) == ENETRESET)
			error = 0;
		break;

	case SIOCSIFFLAGS:
		if ((error = ifioctl_common(ifp, command, data)) != 0)
			break;
		/* XXX re-use ether_ioctl() */
		switch (ifp->if_flags & (IFF_UP|IFF_RUNNING)) {
		case IFF_UP:
			cdce_init(sc);
			break;
		case IFF_RUNNING:
			cdce_stop(sc);
			break;
		default:
			break;
		}
		break;

	default:
		error = ether_ioctl(ifp, command, data);
		break;
	}

	splx(s);

	return (error);
}

Static void
cdce_watchdog(struct ifnet *ifp)
{
	struct cdce_softc	*sc = ifp->if_softc;

	if (sc->cdce_dying)
		return;

	ifp->if_oerrors++;
	printf("%s: watchdog timeout\n", USBDEVNAME(sc->cdce_dev));
}

Static void
cdce_init(void *xsc)
{
	struct cdce_softc	*sc = xsc;
	struct ifnet		*ifp = GET_IFP(sc);
	struct cdce_chain	*c;
	usbd_status		 err;
	int			 s, i;

	if (ifp->if_flags & IFF_RUNNING)
		return;

	s = splnet();

	if (cdce_tx_list_init(sc) == ENOBUFS) {
		printf("%s: tx list init failed\n", USBDEVNAME(sc->cdce_dev));
		splx(s);
		return;
	}

	if (cdce_rx_list_init(sc) == ENOBUFS) {
		printf("%s: rx list init failed\n", USBDEVNAME(sc->cdce_dev));
		splx(s);
		return;
	}

	/* Maybe set multicast / broadcast here??? */

	err = usbd_open_pipe(sc->cdce_data_iface, sc->cdce_bulkin_no,
	    USBD_EXCLUSIVE_USE, &sc->cdce_bulkin_pipe);
	if (err) {
		printf("%s: open rx pipe failed: %s\n", USBDEVNAME(sc->cdce_dev),
		    usbd_errstr(err));
		splx(s);
		return;
	}

	err = usbd_open_pipe(sc->cdce_data_iface, sc->cdce_bulkout_no,
	    USBD_EXCLUSIVE_USE, &sc->cdce_bulkout_pipe);
	if (err) {
		printf("%s: open tx pipe failed: %s\n", USBDEVNAME(sc->cdce_dev),
		    usbd_errstr(err));
		splx(s);
		return;
	}

	for (i = 0; i < CDCE_RX_LIST_CNT; i++) {
		c = &sc->cdce_cdata.cdce_rx_chain[i];
		usbd_setup_xfer(c->cdce_xfer, sc->cdce_bulkin_pipe, c,
		    c->cdce_buf, CDCE_BUFSZ, USBD_SHORT_XFER_OK | USBD_NO_COPY,
		    USBD_NO_TIMEOUT, cdce_rxeof);
		usbd_transfer(c->cdce_xfer);
	}

	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;

	splx(s);
}

Static int
cdce_newbuf(struct cdce_softc *sc, struct cdce_chain *c, struct mbuf *m)
{
	struct mbuf	*m_new = NULL;

	if (m == NULL) {
		MGETHDR(m_new, M_DONTWAIT, MT_DATA);
		if (m_new == NULL) {
			printf("%s: no memory for rx list "
			    "-- packet dropped!\n", USBDEVNAME(sc->cdce_dev));
			return (ENOBUFS);
		}
		MCLGET(m_new, M_DONTWAIT);
		if (!(m_new->m_flags & M_EXT)) {
			printf("%s: no memory for rx list "
			    "-- packet dropped!\n", USBDEVNAME(sc->cdce_dev));
			m_freem(m_new);
			return (ENOBUFS);
		}
		m_new->m_len = m_new->m_pkthdr.len = MCLBYTES;
	} else {
		m_new = m;
		m_new->m_len = m_new->m_pkthdr.len = MCLBYTES;
		m_new->m_data = m_new->m_ext.ext_buf;
	}
	c->cdce_mbuf = m_new;
	return (0);
}

Static int
cdce_rx_list_init(struct cdce_softc *sc)
{
	struct cdce_cdata	*cd;
	struct cdce_chain	*c;
	int			 i;

	cd = &sc->cdce_cdata;
	for (i = 0; i < CDCE_RX_LIST_CNT; i++) {
		c = &cd->cdce_rx_chain[i];
		c->cdce_sc = sc;
		c->cdce_idx = i;
		if (cdce_newbuf(sc, c, NULL) == ENOBUFS)
			return (ENOBUFS);
		if (c->cdce_xfer == NULL) {
			c->cdce_xfer = usbd_alloc_xfer(sc->cdce_udev);
			if (c->cdce_xfer == NULL)
				return (ENOBUFS);
			c->cdce_buf = usbd_alloc_buffer(c->cdce_xfer, CDCE_BUFSZ);
			if (c->cdce_buf == NULL)
				return (ENOBUFS);
		}
	}

	return (0);
}

Static int
cdce_tx_list_init(struct cdce_softc *sc)
{
	struct cdce_cdata	*cd;
	struct cdce_chain	*c;
	int			 i;

	cd = &sc->cdce_cdata;
	for (i = 0; i < CDCE_TX_LIST_CNT; i++) {
		c = &cd->cdce_tx_chain[i];
		c->cdce_sc = sc;
		c->cdce_idx = i;
		c->cdce_mbuf = NULL;
		if (c->cdce_xfer == NULL) {
			c->cdce_xfer = usbd_alloc_xfer(sc->cdce_udev);
			if (c->cdce_xfer == NULL)
				return (ENOBUFS);
			c->cdce_buf = usbd_alloc_buffer(c->cdce_xfer, CDCE_BUFSZ);
			if (c->cdce_buf == NULL)
				return (ENOBUFS);
		}
	}

	return (0);
}

Static void
cdce_rxeof(usbd_xfer_handle xfer, usbd_private_handle priv, usbd_status status)
{
	struct cdce_chain	*c = priv;
	struct cdce_softc	*sc = c->cdce_sc;
	struct ifnet		*ifp = GET_IFP(sc);
	struct mbuf		*m;
	int			 total_len = 0;
	int			 s;

	if (sc->cdce_dying || !(ifp->if_flags & IFF_RUNNING))
		return;

	if (status != USBD_NORMAL_COMPLETION) {
		if (status == USBD_NOT_STARTED || status == USBD_CANCELLED)
			return;
		if (sc->cdce_rxeof_errors == 0)
			printf("%s: usb error on rx: %s\n",
			    USBDEVNAME(sc->cdce_dev), usbd_errstr(status));
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
	m->m_pkthdr.rcvif = ifp;

	s = splnet();

	if (cdce_newbuf(sc, c, NULL) == ENOBUFS) {
		ifp->if_ierrors++;
		goto done1;
	}

#if NBPFILTER > 0
	if (ifp->if_bpf)
		BPF_MTAP(ifp, m);
#endif

	IF_INPUT(ifp, m);

done1:
	splx(s);

done:
	/* Setup new transfer. */
	usbd_setup_xfer(c->cdce_xfer, sc->cdce_bulkin_pipe, c, c->cdce_buf,
	    CDCE_BUFSZ, USBD_SHORT_XFER_OK | USBD_NO_COPY, USBD_NO_TIMEOUT,
	    cdce_rxeof);
	usbd_transfer(c->cdce_xfer);
}

Static void
cdce_txeof(usbd_xfer_handle xfer, usbd_private_handle priv,
    usbd_status status)
{
	struct cdce_chain	*c = priv;
	struct cdce_softc	*sc = c->cdce_sc;
	struct ifnet		*ifp = GET_IFP(sc);
	usbd_status		 err;
	int			 s;

	if (sc->cdce_dying)
		return;

	s = splnet();

	ifp->if_timer = 0;
	ifp->if_flags &= ~IFF_OACTIVE;

	if (status != USBD_NORMAL_COMPLETION) {
		if (status == USBD_NOT_STARTED || status == USBD_CANCELLED) {
			splx(s);
			return;
		}
		ifp->if_oerrors++;
		printf("%s: usb error on tx: %s\n", USBDEVNAME(sc->cdce_dev),
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
cdce_activate(device_ptr_t self, enum devact act)
{
	struct cdce_softc *sc = device_private(self);

	switch (act) {
	case DVACT_ACTIVATE:
		return (EOPNOTSUPP);
		break;

	case DVACT_DEACTIVATE:
		if_deactivate(GET_IFP(sc));
		sc->cdce_dying = 1;
		break;
	}
	return (0);
}


/*  COPYRIGHT (C) 1986 Gary S. Brown.  You may use this program, or
 *  code or tables extracted from it, as desired without restriction.
 */

static uint32_t cdce_crc32_tab[] = {
	0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
	0xe963a535, 0x9e6495a3,	0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
	0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
	0xf3b97148, 0x84be41de,	0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
	0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,	0x14015c4f, 0x63066cd9,
	0xfa0f3d63, 0x8d080df5,	0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
	0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,	0x35b5a8fa, 0x42b2986c,
	0xdbbbc9d6, 0xacbcf940,	0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
	0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
	0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
	0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,	0x76dc4190, 0x01db7106,
	0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
	0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
	0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
	0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
	0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
	0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
	0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
	0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
	0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
	0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
	0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
	0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
	0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
	0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
	0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
	0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
	0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
	0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
	0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
	0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
	0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
	0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
	0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
	0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
	0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
	0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
	0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
	0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
	0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
	0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
	0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
	0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

Static uint32_t
cdce_crc32(const void *buf, size_t size)
{
	const uint8_t *p;
	uint32_t crc;

	p = buf;
	crc = ~0U;

	while (size--)
		crc = cdce_crc32_tab[(crc ^ *p++) & 0xFF] ^ (crc >> 8);

	return (crc ^ ~0U);
}
