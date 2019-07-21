/*	$NetBSD: if_cdce.c,v 1.53 2019/07/21 10:27:56 mrg Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: if_cdce.c,v 1.53 2019/07/21 10:27:56 mrg Exp $");

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
#include <sys/rndsource.h>

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
#include <dev/usb/usbdivar.h>
#include <dev/usb/usbdi_util.h>
#include <dev/usb/usbdevs.h>
#include <dev/usb/usbcdc.h>

#include <dev/usb/if_cdcereg.h>

struct cdce_type {
	struct usb_devno	 cdce_dev;
	uint16_t		 cdce_flags;
#define CDCE_ZAURUS	1
#define CDCE_NO_UNION	2
};

struct cdce_softc;

struct cdce_chain {
	struct cdce_softc	*cdce_sc;
	struct usbd_xfer	*cdce_xfer;
	char			*cdce_buf;
	struct mbuf		*cdce_mbuf;
	int			 cdce_accum;
	int			 cdce_idx;
};

struct cdce_cdata {
	struct cdce_chain	 cdce_rx_chain[CDCE_RX_LIST_CNT];
	struct cdce_chain	 cdce_tx_chain[CDCE_TX_LIST_CNT];
	int			 cdce_tx_prod;
	int			 cdce_tx_cnt;
};

struct cdce_softc {
	device_t cdce_dev;
	struct ethercom		 cdce_ec;
	krndsource_t	 rnd_source;
#define GET_IFP(sc) (&(sc)->cdce_ec.ec_if)
	struct usbd_device *	 cdce_udev;
	struct usbd_interface *	 cdce_ctl_iface;
	struct usbd_interface *	 cdce_data_iface;
	int			 cdce_bulkin_no;
	struct usbd_pipe *	 cdce_bulkin_pipe;
	int			 cdce_bulkout_no;
	struct usbd_pipe *	 cdce_bulkout_pipe;
	int			 cdce_unit;
	struct cdce_cdata	 cdce_cdata;
	int			 cdce_rxeof_errors;
	uint16_t		 cdce_flags;
	uint16_t		 cdce_timer;

	bool			 cdce_attached;
	bool			 cdce_dying;
	bool			 cdce_stopping;

	struct usb_task		 cdce_tick_task;
	struct callout		 cdce_stat_ch;

	kmutex_t		 cdce_lock;
	kmutex_t		 cdce_mii_lock;
	kmutex_t		 cdce_rxlock;
	kmutex_t		 cdce_txlock;
};

static int	cdce_tx_list_init(struct cdce_softc *);
static int	cdce_rx_list_init(struct cdce_softc *);
static int	cdce_newbuf(struct cdce_softc *, struct cdce_chain *,
		    struct mbuf *);
static int	cdce_encap(struct cdce_softc *, struct mbuf *, int);
static void	cdce_rxeof(struct usbd_xfer *, void *, usbd_status);
static void	cdce_txeof(struct usbd_xfer *, void *, usbd_status);
static void	cdce_start(struct ifnet *);
static int	cdce_ioctl(struct ifnet *, u_long, void *);
static void	cdce_init(void *);
static void	cdce_stop(struct cdce_softc *);
static void	cdce_tick(void *);
static void	cdce_tick_task(void *);

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

int cdce_match(device_t, cfdata_t, void *);
void cdce_attach(device_t, device_t, void *);
int cdce_detach(device_t, int);
int cdce_activate(device_t, enum devact);

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

	mutex_init(&sc->cdce_txlock, MUTEX_DEFAULT, IPL_SOFTUSB);
	mutex_init(&sc->cdce_rxlock, MUTEX_DEFAULT, IPL_SOFTUSB);
	mutex_init(&sc->cdce_lock, MUTEX_DEFAULT, IPL_NONE);

	usb_init_task(&sc->cdce_tick_task, cdce_tick_task, sc, USB_TASKQ_MPSAFE);

	aprint_normal_dev(self, "address %s\n", ether_sprintf(eaddr));

	ifp = GET_IFP(sc);
	ifp->if_softc = sc;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_extflags = IFEF_MPSAFE;
	ifp->if_ioctl = cdce_ioctl;
	ifp->if_start = cdce_start;
	strlcpy(ifp->if_xname, device_xname(sc->cdce_dev), IFNAMSIZ);

	IFQ_SET_READY(&ifp->if_snd);

	if_attach(ifp);
	ether_ifattach(ifp, eaddr);

	callout_init(&sc->cdce_stat_ch, CALLOUT_MPSAFE);
	callout_setfunc(&sc->cdce_stat_ch, cdce_tick, sc);

	sc->cdce_attached = true;

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

	mutex_enter(&sc->cdce_lock);
	sc->cdce_dying = true;
	mutex_exit(&sc->cdce_lock);

	if (!sc->cdce_attached)
		return 0;

	pmf_device_deregister(self);

	callout_halt(&sc->cdce_stat_ch, NULL);
	usb_rem_task_wait(sc->cdce_udev, &sc->cdce_tick_task,
	    USB_TASKQ_DRIVER, NULL);

	if (ifp->if_flags & IFF_RUNNING) {
		IFNET_LOCK(ifp);
		cdce_stop(sc);
		IFNET_UNLOCK(ifp);
	}

	callout_destroy(&sc->cdce_stat_ch);
	ether_ifdetach(ifp);
	if_detach(ifp);
	sc->cdce_attached = false;

	usbd_add_drv_event(USB_EVENT_DRIVER_DETACH, sc->cdce_udev,sc->cdce_dev);

	mutex_destroy(&sc->cdce_lock);
	mutex_destroy(&sc->cdce_rxlock);
	mutex_destroy(&sc->cdce_txlock);

	return 0;
}

static void
cdce_start_locked(struct ifnet *ifp)
{
	struct cdce_softc	*sc = ifp->if_softc;
	struct mbuf		*m_head = NULL;

	KASSERT(mutex_owned(&sc->cdce_txlock));

	if (sc->cdce_dying || sc->cdce_cdata.cdce_tx_cnt == CDCE_TX_LIST_CNT)
		return;

	IFQ_POLL(&ifp->if_snd, m_head);
	if (m_head == NULL)
		return;

	if (cdce_encap(sc, m_head, 0)) {
		ifp->if_flags |= IFF_OACTIVE;
		return;
	}

	IFQ_DEQUEUE(&ifp->if_snd, m_head);

	bpf_mtap(ifp, m_head, BPF_D_OUT);

	/*
	 * Set a timeout in case the chip goes out to lunch.
	 */
	sc->cdce_timer = 6;
}

static void
cdce_start(struct ifnet *ifp)
{
	struct cdce_softc * const sc = ifp->if_softc;

	mutex_enter(&sc->cdce_txlock);
	cdce_start_locked(ifp);
	mutex_exit(&sc->cdce_txlock);
}

static int
cdce_encap(struct cdce_softc *sc, struct mbuf *m, int idx)
{
	struct cdce_chain	*c;
	usbd_status		 err;
	int			 extra = 0;

	KASSERT(mutex_owned(&sc->cdce_txlock));

	c = &sc->cdce_cdata.cdce_tx_chain[idx];

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
	err = usbd_transfer(c->cdce_xfer);
	if (err != USBD_IN_PROGRESS) {
		/* XXXSMP IFNET_LOCK */
		cdce_stop(sc);
		return EIO;
	}

	sc->cdce_cdata.cdce_tx_cnt++;
	KASSERT(sc->cdce_cdata.cdce_tx_cnt <= CDCE_TX_LIST_CNT);

	return 0;
}

static void
cdce_stop_locked(struct cdce_softc *sc)
{
	usbd_status	 err;
	struct ifnet	*ifp = GET_IFP(sc);
	int		 i;

	/* XXXSMP can't KASSERT(IFNET_LOCKED(ifp)); */
	KASSERT(mutex_owned(&sc->cdce_lock));

	mutex_enter(&sc->cdce_rxlock);
	mutex_enter(&sc->cdce_txlock);
	sc->cdce_stopping = true;
	mutex_exit(&sc->cdce_txlock);
	mutex_exit(&sc->cdce_rxlock);

	/*
	 * XXXSMP Would like to
	 *	KASSERT(IFNET_LOCKED(ifp))
	 * here but the locking order is:
	 *	ifnet -> sc lock -> rxlock -> txlock
	 * and sc lock is already held.
	 */
	ifp->if_flags &= ~IFF_RUNNING;
	sc->cdce_timer = 0;

	callout_stop(&sc->cdce_stat_ch);

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

	for (i = 0; i < CDCE_RX_LIST_CNT; i++) {
		if (sc->cdce_cdata.cdce_rx_chain[i].cdce_mbuf != NULL) {
			m_freem(sc->cdce_cdata.cdce_rx_chain[i].cdce_mbuf);
			sc->cdce_cdata.cdce_rx_chain[i].cdce_mbuf = NULL;
		}
		if (sc->cdce_cdata.cdce_rx_chain[i].cdce_xfer != NULL) {
			usbd_destroy_xfer
			    (sc->cdce_cdata.cdce_rx_chain[i].cdce_xfer);
			sc->cdce_cdata.cdce_rx_chain[i].cdce_xfer = NULL;
		}
	}

	for (i = 0; i < CDCE_TX_LIST_CNT; i++) {
		if (sc->cdce_cdata.cdce_tx_chain[i].cdce_mbuf != NULL) {
			m_freem(sc->cdce_cdata.cdce_tx_chain[i].cdce_mbuf);
			sc->cdce_cdata.cdce_tx_chain[i].cdce_mbuf = NULL;
		}
		if (sc->cdce_cdata.cdce_tx_chain[i].cdce_xfer != NULL) {
			usbd_destroy_xfer(
				sc->cdce_cdata.cdce_tx_chain[i].cdce_xfer);
			sc->cdce_cdata.cdce_tx_chain[i].cdce_xfer = NULL;
		}
	}

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
}

static void
cdce_stop(struct cdce_softc *sc)
{

	mutex_enter(&sc->cdce_lock);
	cdce_stop_locked(sc);
	mutex_exit(&sc->cdce_lock);
}

static int
cdce_ioctl(struct ifnet *ifp, u_long command, void *data)
{
	struct cdce_softc	*sc = ifp->if_softc;
	struct ifaddr		*ifa = (struct ifaddr *)data;
	struct ifreq		*ifr = (struct ifreq *)data;
	int			 error = 0;

	if (sc->cdce_dying)
		return EIO;

	switch(command) {
	case SIOCINITIFADDR:
		ifp->if_flags |= IFF_UP;
		cdce_init(sc);
		switch (ifa->ifa_addr->sa_family) {
#ifdef INET
		case AF_INET:
			arp_ifinit(ifp, ifa);
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

	if (error == ENETRESET)
		error = 0;

	return error;
}

static void
cdce_watchdog(struct ifnet *ifp)
{
	struct cdce_softc *sc = ifp->if_softc;
	struct cdce_chain *c;
	usbd_status stat;

	KASSERT(mutex_owned(&sc->cdce_lock));

	if (sc->cdce_dying)
		return;

	ifp->if_oerrors++;
	aprint_error_dev(sc->cdce_dev, "watchdog timeout\n");

	mutex_enter(&sc->cdce_txlock);

	c = &sc->cdce_cdata.cdce_rx_chain[0];
	usbd_get_xfer_status(c->cdce_xfer, NULL, NULL, NULL, &stat);
	cdce_txeof(c->cdce_xfer, c, stat);

	if (!IFQ_IS_EMPTY(&ifp->if_snd))
		cdce_start_locked(ifp);

	mutex_exit(&sc->cdce_txlock);
}

static void
cdce_init(void *xsc)
{
	struct cdce_softc	*sc = xsc;
	struct ifnet		*ifp = GET_IFP(sc);
	struct cdce_chain	*c;
	usbd_status		 err;
	int			 i;

	mutex_enter(&sc->cdce_lock);
	if (ifp->if_flags & IFF_RUNNING)
		goto out;

	/* Maybe set multicast / broadcast here??? */

	err = usbd_open_pipe(sc->cdce_data_iface, sc->cdce_bulkin_no,
	    USBD_EXCLUSIVE_USE | USBD_MPSAFE, &sc->cdce_bulkin_pipe);
	if (err) {
		printf("%s: open rx pipe failed: %s\n", device_xname(sc->cdce_dev),
		    usbd_errstr(err));
		goto out;
	}

	err = usbd_open_pipe(sc->cdce_data_iface, sc->cdce_bulkout_no,
	    USBD_EXCLUSIVE_USE | USBD_MPSAFE, &sc->cdce_bulkout_pipe);
	if (err) {
		printf("%s: open tx pipe failed: %s\n",
		    device_xname(sc->cdce_dev), usbd_errstr(err));
		goto out;
	}

	if (cdce_tx_list_init(sc)) {
		printf("%s: tx list init failed\n", device_xname(sc->cdce_dev));
		goto out;
	}

	if (cdce_rx_list_init(sc)) {
		printf("%s: rx list init failed\n", device_xname(sc->cdce_dev));
		goto out;
	}

	mutex_enter(&sc->cdce_rxlock);
	mutex_enter(&sc->cdce_txlock);
	sc->cdce_stopping = false;

	for (i = 0; i < CDCE_RX_LIST_CNT; i++) {
		c = &sc->cdce_cdata.cdce_rx_chain[i];

		usbd_setup_xfer(c->cdce_xfer, c, c->cdce_buf, CDCE_BUFSZ,
		    USBD_SHORT_XFER_OK, USBD_NO_TIMEOUT, cdce_rxeof);
		usbd_transfer(c->cdce_xfer);
	}

	mutex_exit(&sc->cdce_txlock);
	mutex_exit(&sc->cdce_rxlock);

	ifp->if_flags |= IFF_RUNNING;

	callout_schedule(&sc->cdce_stat_ch, hz);

out:
	mutex_exit(&sc->cdce_lock);
}

static int
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

static int
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
			return ENOBUFS;
		if (c->cdce_xfer == NULL) {
			int err = usbd_create_xfer(sc->cdce_bulkin_pipe,
			    CDCE_BUFSZ, 0, 0, &c->cdce_xfer);
			if (err)
				return err;
			c->cdce_buf = usbd_get_buffer(c->cdce_xfer);
		}
	}

	return 0;
}

static int
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

static void
cdce_rxeof(struct usbd_xfer *xfer, void *priv, usbd_status status)
{
	struct cdce_chain	*c = priv;
	struct cdce_softc	*sc = c->cdce_sc;
	struct ifnet		*ifp = GET_IFP(sc);
	struct mbuf		*m;
	int			 total_len = 0;

	mutex_enter(&sc->cdce_rxlock);

	if (sc->cdce_dying || sc->cdce_stopping ||
	    status == USBD_INVAL || status == USBD_NOT_STARTED ||
	    status == USBD_CANCELLED || !(ifp->if_flags & IFF_RUNNING)) {
		mutex_exit(&sc->cdce_rxlock);
		return;
	}

	if (status != USBD_NORMAL_COMPLETION) {
		if (sc->cdce_rxeof_errors == 0)
			printf("%s: usb error on rx: %s\n",
			    device_xname(sc->cdce_dev), usbd_errstr(status));
		if (status == USBD_STALLED)
			usbd_clear_endpoint_stall_async(sc->cdce_bulkin_pipe);
		usbd_delay_ms(sc->cdce_udev, 10);
		sc->cdce_rxeof_errors++;
		goto done;
	}

	sc->cdce_rxeof_errors = 0;

	usbd_get_xfer_status(xfer, NULL, NULL, &total_len, NULL);
	if (sc->cdce_flags & CDCE_ZAURUS)
		total_len -= 4;	/* Strip off CRC added by Zaurus */
	if (total_len <= 1) {
		ifp->if_ierrors++;
		goto done;
	}

	m = c->cdce_mbuf;
	memcpy(mtod(m, char *), c->cdce_buf, total_len);

	if (total_len < sizeof(struct ether_header)) {
		ifp->if_ierrors++;
		goto done;
	}

	m->m_pkthdr.len = m->m_len = total_len;
	m_set_rcvif(m, ifp);

	if (cdce_newbuf(sc, c, NULL) == ENOBUFS) {
		ifp->if_ierrors++;
		goto done;
	}

	mutex_exit(&sc->cdce_rxlock);
	if_percpuq_enqueue(ifp->if_percpuq, m);
	mutex_enter(&sc->cdce_rxlock);

done:
	if (sc->cdce_stopping || sc->cdce_dying) {
		mutex_exit(&sc->cdce_rxlock);
		return;
	}

	mutex_exit(&sc->cdce_rxlock);

	/* Setup new transfer. */
	usbd_setup_xfer(c->cdce_xfer, c, c->cdce_buf, CDCE_BUFSZ,
	    USBD_SHORT_XFER_OK, USBD_NO_TIMEOUT, cdce_rxeof);
	usbd_transfer(c->cdce_xfer);
}

static void
cdce_txeof(struct usbd_xfer *xfer, void *priv, usbd_status status)
{
	struct cdce_chain	*c = priv;
	struct cdce_softc	*sc = c->cdce_sc;
	struct ifnet		*ifp = GET_IFP(sc);
	usbd_status		 err;

	mutex_enter(&sc->cdce_txlock);

	if (sc->cdce_dying)
		goto out;

	KASSERT(sc->cdce_cdata.cdce_tx_cnt > 0);
	sc->cdce_cdata.cdce_tx_cnt--;

	sc->cdce_timer = 0;

	switch (status) {
	case USBD_NOT_STARTED:
	case USBD_CANCELLED:
		goto out;

	case USBD_NORMAL_COMPLETION:
		break;

	default:
		ifp->if_oerrors++;
		printf("%s: usb error on tx: %s\n", device_xname(sc->cdce_dev),
		    usbd_errstr(status));
		if (status == USBD_STALLED)
			usbd_clear_endpoint_stall_async(sc->cdce_bulkout_pipe);
		goto out;
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
		cdce_start_locked(ifp);

out:
	mutex_exit(&sc->cdce_txlock);
}

static void
cdce_tick(void *xsc)
{
	struct cdce_softc * const sc = xsc;

	if (sc == NULL)
		return;

	mutex_enter(&sc->cdce_lock);

	if (sc->cdce_stopping || sc->cdce_dying) {
		mutex_exit(&sc->cdce_lock);
		return;
	}

	/* Perform periodic stuff in process context */
	usb_add_task(sc->cdce_udev, &sc->cdce_tick_task, USB_TASKQ_DRIVER);

	mutex_exit(&sc->cdce_lock);
}

static void
cdce_tick_task(void *xsc)
{
	struct cdce_softc * const sc = xsc;
	struct ifnet *ifp;

	if (sc == NULL)
		return;

	mutex_enter(&sc->cdce_lock);

	if (sc->cdce_stopping || sc->cdce_dying) {
		mutex_exit(&sc->cdce_lock);
		return;
	}

	ifp = GET_IFP(sc);

	if (sc->cdce_timer != 0 && --sc->cdce_timer == 0)
		cdce_watchdog(ifp);
	if (!sc->cdce_stopping && !sc->cdce_dying)
		callout_schedule(&sc->cdce_stat_ch, hz);

	mutex_exit(&sc->cdce_lock);
}

int
cdce_activate(device_t self, enum devact act)
{
	struct cdce_softc *sc = device_private(self);

	switch (act) {
	case DVACT_DEACTIVATE:
		if_deactivate(GET_IFP(sc));

		mutex_enter(&sc->cdce_lock);
		sc->cdce_dying = true;
		mutex_exit(&sc->cdce_lock);

		mutex_enter(&sc->cdce_rxlock);
		mutex_enter(&sc->cdce_txlock);
		sc->cdce_stopping = true;
		mutex_exit(&sc->cdce_txlock);
		mutex_exit(&sc->cdce_rxlock);

		return 0;
	default:
		return EOPNOTSUPP;
	}
}
