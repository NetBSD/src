/*	$NetBSD: if_uax.c,v 1.3 2003/02/16 17:18:47 augustss Exp $	*/

/*
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net).
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
 * TODO:
 *   toggle link LED
 *   do something in interrupt routine
 */

/*
 * Driver for the ASIX AX88172 Fast Ethernet USB 2.0 adapter.
 * Data sheet at
 * http://www.asix.com.tw/datasheet/mac/Ax88172.PDF
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_uax.c,v 1.3 2003/02/16 17:18:47 augustss Exp $");

#include "opt_inet.h"
#include "opt_ns.h"
#include "bpfilter.h"
#include "rnd.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/sockio.h>
#include <sys/mbuf.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/socket.h>

#include <sys/device.h>
#if NRND > 0
#include <sys/rnd.h>
#endif

#include <net/if.h>
#include <net/if_arp.h>
#include <net/if_dl.h>
#include <net/if_media.h>

#define BPF_MTAP(ifp, m) bpf_mtap((ifp)->if_bpf, (m))

#if NBPFILTER > 0
#include <net/bpf.h>
#endif

#include <net/if_ether.h>
#ifdef INET
#include <netinet/in.h>
#include <netinet/if_inarp.h>
#endif

#ifdef NS
#include <netns/ns.h>
#include <netns/ns_if.h>
#endif

#include <dev/mii/mii.h>
#include <dev/mii/miivar.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdi_util.h>
#include <dev/usb/usbdevs.h>

#include <dev/usb/if_uaxreg.h>

#ifdef UAX_DEBUG
#define DPRINTF(x)	if (uaxdebug) logprintf x
#define DPRINTFN(n,x)	if (uaxdebug >= (n)) logprintf x
int	uaxdebug = 0;
#else
#define DPRINTF(x)
#define DPRINTFN(n,x)
#endif

#define ETHER_ALIGN		2

/********** if_uaxvar.h ***********/

#define UAX_ENDPT_RX 0
#define UAX_ENDPT_TX 1
#define UAX_ENDPT_INTR 2
#define UAX_ENDPT_MAX 3

/* XXX Must be 1 for now */
#define UAX_TX_LIST_CNT		1
#define UAX_RX_LIST_CNT		2

struct uax_softc;

struct uax_chain {
	struct uax_softc	*uch_sc;
	usbd_xfer_handle	uch_xfer;
	char			*uch_buf;
	struct mbuf		*uch_mbuf;
	int			uch_idx;
};

struct uax_phy_info {
	int			phy;
	int			phy_type;
	int			phy_present;
};

struct uax_softc {
	USBBASEDEVICE		sc_dev;

	struct ethercom		sc_ec;
	struct mii_data		sc_mii;
#if NRND > 0
	rndsource_element_t	sc_rnd_source;
#endif
#define GET_IFP(sc) (&(sc)->sc_ec.ec_if)
#define GET_MII(sc) (&(sc)->sc_mii)

	usb_callout_t		sc_stat_ch;

	usbd_device_handle	sc_udev;
	usbd_interface_handle	sc_iface;
	u_int16_t		sc_vendor;
	u_int16_t		sc_product;
	int			sc_ed[UAX_ENDPT_MAX];
	usbd_pipe_handle	sc_ep[UAX_ENDPT_MAX];

	u_int8_t		sc_link;

	int			sc_chip_version;
	struct uax_phy_info	sc_phys[UAX_MAX_PHYS];
	int			sc_nphys;
	int			sc_pna;	/* XXX never set */

	u_int8_t		sc_mcast[UAX_MULTI_FILTER_SIZE];
	u_int8_t		sc_packet_filter;

	int			sc_sw_mii;

	/* Tx info */
	struct uax_chain	sc_tx_chain[UAX_TX_LIST_CNT];
#if 0
	int			sc_tx_prod;
	int			sc_tx_cons;
#endif
	int			sc_tx_cnt;

	/* Rx info */
	struct uax_chain	sc_rx_chain[UAX_RX_LIST_CNT];
	u_int			sc_rx_errs;
	struct timeval		sc_rx_notice;

	/* Interrupt info */
	struct uax_intrpkt	sc_ibuf;
	u_int			sc_intr_errs;

	struct usb_task		sc_tick_task;
	struct usb_task		sc_stop_task;

	int			sc_refcnt;
	char			sc_dying;
	char			sc_attached;
};

Static const struct usb_devno uax_devs[] = {
	{ USB_VENDOR_DLINK,		USB_PRODUCT_DLINK_DUBE100},
};
#define uax_lookup(v, p) ((struct uax_type *)usb_lookup(uax_devs, v, p))

Static void uax_start(struct ifnet *);
Static int uax_ioctl(struct ifnet *, u_long, caddr_t);
Static void uax_stop(struct ifnet *, int);
Static int uax_init(struct ifnet *);
Static void uax_watchdog(struct ifnet *);

Static void uax_stop_wrap(void *v);
Static void uax_tick(void *);
Static void uax_tick_task(void *);

Static int uax_miibus_readreg(device_ptr_t, int, int);
Static void uax_miibus_writereg(device_ptr_t, int, int, int);
Static void uax_miibus_statchg(device_ptr_t);
Static int uax_ifmedia_upd(struct ifnet *);
Static void uax_ifmedia_sts(struct ifnet *, struct ifmediareq *);

/*Static void uax_reset(struct uax_softc *sc);*/
Static void uax_setup(struct uax_softc *sc);
Static void uax_phy_init(struct uax_softc *sc, int i);
Static void uax_setup_phy(struct uax_softc *sc);
Static void uax_read_mac(struct uax_softc *, u_char *);
Static void uax_grab_mii(struct uax_softc *sc);
Static void uax_ungrab_mii(struct uax_softc *sc);

Static int uax_send(struct uax_softc *, struct mbuf *, int);
Static void uax_intr(usbd_xfer_handle, usbd_private_handle, usbd_status);
Static void uax_rxeof(usbd_xfer_handle, usbd_private_handle, usbd_status);
Static void uax_txeof(usbd_xfer_handle, usbd_private_handle, usbd_status);
Static int uax_tx_list_init(struct uax_softc *);
Static int uax_rx_list_init(struct uax_softc *);
Static int uax_newbuf(struct uax_softc *, struct uax_chain *, struct mbuf *);
Static int uax_openpipes(struct uax_softc *);
Static u_int32_t uax_crc(u_int8_t *eaddr);
Static void uax_setmulti(struct uax_softc *sc);

static inline usbd_status
uax_request(struct uax_softc *sc, uint type, uint req,
	    uint value, uint index, uint len, void *data);
static inline usbd_status
uax_request(struct uax_softc *sc, uint type, uint rq,
	    uint value, uint indx, uint len, void *data)
{
	usb_device_request_t req;

	req.bmRequestType = type;
	req.bRequest = rq;
	USETW(req.wValue, value);
	USETW(req.wIndex, indx);
	USETW(req.wLength, len);

	return (usbd_do_request(sc->sc_udev, &req, data));
}

/*******/

USB_DECLARE_DRIVER(uax);

USB_MATCH(uax)
{
	USB_MATCH_START(uax, uaa);

	if (uaa->iface != NULL)
		return (UMATCH_NONE);

	return (uax_lookup(uaa->vendor, uaa->product) != NULL ?
		UMATCH_VENDOR_PRODUCT : UMATCH_NONE);
}

/*
 * Attach the interface. Allocate softc structures, do ifmedia
 * setup and ethernet/BPF attach.
 */
USB_ATTACH(uax)
{
	USB_ATTACH_START(uax, sc, uaa);
	char			devinfo[1024];
	int			s;
	u_char			eaddr[ETHER_ADDR_LEN];
	struct ifnet		*ifp;
	struct mii_data		*mii;
	usbd_device_handle	dev = uaa->device;
	usbd_interface_handle	iface;
	usbd_status		err;
	usb_interface_descriptor_t	*id;
	usb_endpoint_descriptor_t	*ed;
	int			i;

	DPRINTFN(5,(" : uax_attach: sc=%p", sc));

	usbd_devinfo(dev, 0, devinfo);
	USB_ATTACH_SETUP;
	printf("%s: %s\n", USBDEVNAME(sc->sc_dev), devinfo);

	err = usbd_set_config_no(dev, UAX_CONFIG_NO, 1);
	if (err) {
		printf("%s: setting config no failed\n",
		    USBDEVNAME(sc->sc_dev));
		USB_ATTACH_ERROR_RETURN;
	}

	ifp = GET_IFP(sc);
	usb_init_task(&sc->sc_tick_task, uax_tick_task, sc);
	usb_init_task(&sc->sc_stop_task, uax_stop_wrap, ifp);

	err = usbd_device2interface_handle(dev, UAX_IFACE_IDX, &iface);
	if (err) {
		printf("%s: getting interface handle failed\n",
		    USBDEVNAME(sc->sc_dev));
		USB_ATTACH_ERROR_RETURN;
	}

	sc->sc_udev = dev;
	sc->sc_iface = iface;
	sc->sc_product = uaa->product;
	sc->sc_vendor = uaa->vendor;

	id = usbd_get_interface_descriptor(iface);

	/* Find endpoints. */
	for (i = 0; i < id->bNumEndpoints; i++) {
		ed = usbd_interface2endpoint_descriptor(iface, i);
		if (ed == NULL) {
			printf("%s: couldn't get endpoint descriptor %d\n",
			    USBDEVNAME(sc->sc_dev), i);
			USB_ATTACH_ERROR_RETURN;
		}
		if (UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_IN &&
		    UE_GET_XFERTYPE(ed->bmAttributes) == UE_BULK) {
			sc->sc_ed[UAX_ENDPT_RX] = ed->bEndpointAddress;
		} else if (UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_OUT &&
			   UE_GET_XFERTYPE(ed->bmAttributes) == UE_BULK) {
			sc->sc_ed[UAX_ENDPT_TX] = ed->bEndpointAddress;
		} else if (UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_IN &&
			   UE_GET_XFERTYPE(ed->bmAttributes) == UE_INTERRUPT) {
			sc->sc_ed[UAX_ENDPT_INTR] = ed->bEndpointAddress;
		}
	}

	if (sc->sc_ed[UAX_ENDPT_RX] == 0 || sc->sc_ed[UAX_ENDPT_TX] == 0 ||
	    sc->sc_ed[UAX_ENDPT_INTR] == 0) {
		printf("%s: missing endpoint\n", USBDEVNAME(sc->sc_dev));
		USB_ATTACH_ERROR_RETURN;
	}

	/* Get data from chip and set up hw. */
	uax_setup(sc);
	uax_setup_phy(sc);
	printf("%s: chip version %d\n", USBDEVNAME(sc->sc_dev),
	       sc->sc_chip_version);
#if UAX_DEBUG
	if (uaxdebug) {
		for (i = 0; i < UAX_MAX_PHYS; i++) {
			printf("%s: phy%d present=%d id=%d type=%d\n",
			       USBDEVNAME(sc->sc_dev), i,
			       sc->sc_phys[i].phy_present,
			       sc->sc_phys[i].phy,
			       sc->sc_phys[i].phy_type);
		}
	}
#endif

	s = splnet();

	/*
	 * Get station address from the EEPROM.
	 */
	uax_read_mac(sc, eaddr);

	/*
	 * A Pegasus chip was detected. Inform the world.
	 */
	printf("%s: Ethernet address %s\n", USBDEVNAME(sc->sc_dev),
	    ether_sprintf(eaddr));

	/* Initialize interface info.*/
	ifp->if_softc = sc;
	ifp->if_mtu = ETHERMTU;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_ioctl = uax_ioctl;
	ifp->if_start = uax_start;
	ifp->if_watchdog = uax_watchdog;
	ifp->if_init = uax_init;
	ifp->if_stop = uax_stop;
	strncpy(ifp->if_xname, USBDEVNAME(sc->sc_dev), IFNAMSIZ);

	IFQ_SET_READY(&ifp->if_snd);

	/* Initialize MII/media info. */
	mii = &sc->sc_mii;
	mii->mii_ifp = ifp;
	mii->mii_readreg = uax_miibus_readreg;
	mii->mii_writereg = uax_miibus_writereg;
	mii->mii_statchg = uax_miibus_statchg;
	mii->mii_flags = MIIF_AUTOTSLEEP;
	ifmedia_init(&mii->mii_media, 0, uax_ifmedia_upd, uax_ifmedia_sts);

	uax_grab_mii(sc);
	mii_attach(self, mii, ~0, MII_PHY_ANY, MII_OFFSET_ANY, 0);
	uax_ungrab_mii(sc);

	if (LIST_FIRST(&mii->mii_phys) == NULL) {
		ifmedia_add(&mii->mii_media, IFM_ETHER | IFM_NONE, 0, NULL);
		ifmedia_set(&mii->mii_media, IFM_ETHER | IFM_NONE);
	} else
		ifmedia_set(&mii->mii_media, IFM_ETHER | IFM_AUTO);

	/* Attach the interface. */
	if_attach(ifp);
	Ether_ifattach(ifp, eaddr);
#if NRND > 0
	rnd_attach_source(&sc->sc_rnd_source, USBDEVNAME(sc->sc_dev),
	    RND_TYPE_NET, 0);
#endif

	usb_callout_init(sc->sc_stat_ch);

	sc->sc_attached = 1;
	splx(s);

	usbd_add_drv_event(USB_EVENT_DRIVER_ATTACH, sc->sc_udev,
			   USBDEV(sc->sc_dev));

	USB_ATTACH_SUCCESS_RETURN;
}

USB_DETACH(uax)
{
	USB_DETACH_START(uax, sc);
	struct ifnet		*ifp = GET_IFP(sc);
	int			s;

	DPRINTFN(2,("%s: %s: enter\n", USBDEVNAME(sc->sc_dev), __func__));

	if (!sc->sc_attached) {
		/* Detached before attached finished, so just bail out. */
		return (0);
	}

	usb_uncallout(sc->sc_stat_ch, uax_tick, sc);
	/*
	 * Remove any pending tasks.  They cannot be executing because they run
	 * in the same thread as detach.
	 */
	usb_rem_task(sc->sc_udev, &sc->sc_tick_task);
	usb_rem_task(sc->sc_udev, &sc->sc_stop_task);

	s = splusb();

	if (ifp->if_flags & IFF_RUNNING)
		uax_stop(ifp, 1);

#if NRND > 0
	rnd_detach_source(&sc->sc_rnd_source);
#endif
	mii_detach(&sc->sc_mii, MII_PHY_ANY, MII_OFFSET_ANY);
	ifmedia_delete_instance(&sc->sc_mii.mii_media, IFM_INST_ANY);
	ether_ifdetach(ifp);

	if_detach(ifp);

#ifdef DIAGNOSTIC
	if (sc->sc_ep[UAX_ENDPT_TX] != NULL ||
	    sc->sc_ep[UAX_ENDPT_RX] != NULL ||
	    sc->sc_ep[UAX_ENDPT_INTR] != NULL)
		printf("%s: detach has active endpoints\n",
		       USBDEVNAME(sc->sc_dev));
#endif

	sc->sc_attached = 0;

	if (--sc->sc_refcnt >= 0) {
		/* Wait for processes to go away. */
		usb_detach_wait(USBDEV(sc->sc_dev));
	}
	splx(s);

	usbd_add_drv_event(USB_EVENT_DRIVER_DETACH, sc->sc_udev,
			   USBDEV(sc->sc_dev));

	return (0);
}

int
uax_activate(device_ptr_t self, enum devact act)
{
	struct uax_softc *sc = (struct uax_softc *)self;

	DPRINTFN(2,("%s: %s: enter\n", USBDEVNAME(sc->sc_dev), __func__));

	switch (act) {
	case DVACT_ACTIVATE:
		return (EOPNOTSUPP);
		break;

	case DVACT_DEACTIVATE:
		if_deactivate(&sc->sc_ec.ec_if);
		sc->sc_dying = 1;
		break;
	}
	return (0);
}

Static void
uax_setup(struct uax_softc *sc)
{
	uByte version;
	uByte mii_data[2];

	/* Get chip version */
	version = 0;
	(void)uax_request(sc, UT_READ_VENDOR_DEVICE, UAX_GET_MONITOR_MODE,
			  0, 0, sizeof version, &version);
	sc->sc_chip_version = version;

	/* Get PHY id */
	(void)uax_request(sc, UT_READ_VENDOR_DEVICE, UAX_READ_PHYID,
			  0, 0, sizeof mii_data, &mii_data);
	sc->sc_phys[0].phy = UAX_GET_PHY(mii_data[1]);
	sc->sc_phys[0].phy_type = UAX_GET_PHY_TYPE(mii_data[1]);
	sc->sc_phys[1].phy = UAX_GET_PHY(mii_data[0]);
	sc->sc_phys[1].phy_type = UAX_GET_PHY_TYPE(mii_data[0]);

	/* Read multicast filter array */
	(void)uax_request(sc, UT_READ_VENDOR_DEVICE, UAX_READ_MULTI_FILTER,
			  0, 0, sizeof sc->sc_mcast, &sc->sc_mcast);

	sc->sc_packet_filter = UAX_RX_BROADCAST | UAX_RX_DIRECTED;
}

Static void
uax_phy_init(struct uax_softc *sc, int i)
{
	struct uax_phy_info *p = &sc->sc_phys[i];

	DPRINTF(("uax_phy_init: i=%d\n", i));
	if (p->phy || p->phy_type != 7) {
		u_int16_t v1, v2;
		v1 = uax_miibus_readreg(&sc->sc_dev, p->phy, MII_BMSR);
		v2 = uax_miibus_readreg(&sc->sc_dev, p->phy, MII_BMCR);
		if ((v1 & v2) != 0xffff) {
#if 0
			/*
			 * The data sheet and the Windows driver isolates and
			 * power down the PHY at this point, but doing so
			 * makes mii_attach() fail often.
			 */
			uax_miibus_writereg(&sc->sc_dev, p->phy, MII_BMCR,
					    BMCR_ISO | BMCR_PDOWN);
#endif
			p->phy_present = 1;
			sc->sc_nphys++;
		}
	}
	DPRINTF(("uax_phy_init: i=%d done\n", i));
}

/* Switch to software PHY access mode. */
Static void
uax_grab_mii(struct uax_softc *sc)
{
	if (sc->sc_sw_mii++ > 0)
		return;
	(void)uax_request(sc, UT_WRITE_VENDOR_DEVICE, UAX_SOFTWARE_MII,
			  0, 0, 0, NULL);
}

/* Switch to hardware PHY access mode. */
Static void
uax_ungrab_mii(struct uax_softc *sc)
{
	if (--sc->sc_sw_mii > 0)
		return;
	(void)uax_request(sc, UT_WRITE_VENDOR_DEVICE, UAX_HARDWARE_MII,
			  0, 0, 0, NULL);
}

#define PHY_TIMEOUT 10000

/* Magic taken from the Windows driver. */
Static void
uax_setup_phy(struct uax_softc *sc)
{
	uByte status;
	int i;

	uax_grab_mii(sc);

	for (i = 0; i < PHY_TIMEOUT; i++) {
		delay(50);
		status = 0;
		uax_request(sc, UT_READ_VENDOR_DEVICE, UAX_READ_MII_OPMODE,
			    0, 0, sizeof status, &status);
		if (status & 1)
			break;
	}
	if (i >= PHY_TIMEOUT)
		printf("%s: mii status read timeout\n",USBDEVNAME(sc->sc_dev));

	for (i = 0; i < UAX_MAX_PHYS; i++)
		uax_phy_init(sc, i);

	uax_ungrab_mii(sc);

	(void)uax_request(sc, UT_WRITE_VENDOR_DEVICE, UAX_WRITE_IPG,
			  sc->sc_pna ? 0x17 : 0x15, 0, 0, NULL);
	(void)uax_request(sc, UT_WRITE_VENDOR_DEVICE, UAX_WRITE_IPG1,
			  0x0c, 0, 0, NULL);
	(void)uax_request(sc, UT_WRITE_VENDOR_DEVICE, UAX_WRITE_IPG2,
			  sc->sc_pna ? 0x14 : 0x12, 0, 0, NULL);
}

Static void
uax_read_mac(struct uax_softc *sc, u_char *dest)
{
	usbd_status err;

	if (sc->sc_dying)
		return;

	err = uax_request(sc, UT_READ_VENDOR_DEVICE, UAX_READ_NODEID,
			  0, 0, ETHER_ADDR_LEN, dest);
	if (err) {
		DPRINTF(("%s: uax_read_mac error=%s\n",
			 USBDEVNAME(sc->sc_dev), usbd_errstr(err)));
	}
}

/*
 * uax_ioctl:		[ifnet interface function]
 *
 *	Handle control requests from the operator.
 */
int
uax_ioctl(struct ifnet *ifp, u_long cmd, caddr_t data)
{
	struct uax_softc *sc = ifp->if_softc;
	struct ifreq *ifr = (struct ifreq *)data;
	int s, error;

	s = splnet();

	switch (cmd) {
	case SIOCSIFMEDIA:
	case SIOCGIFMEDIA:
		error = ifmedia_ioctl(ifp, ifr, &sc->sc_mii.mii_media, cmd);
		break;

	default:
		error = ether_ioctl(ifp, cmd, data);
		if (error == ENETRESET) {
			/*
			 * Multicast list has changed.  Set the
			 * hardware filter accordingly.
			 */
			uax_setmulti(sc);
			error = 0;
		}
		break;
	}

	/* Try to get more packets going. */
	if (IFQ_IS_EMPTY(&ifp->if_snd) == 0)
		uax_start(ifp);

	splx(s);
	return (error);
}

Static void
uax_watchdog(struct ifnet *ifp)
{
	struct uax_softc	*sc = ifp->if_softc;
	struct uax_chain	*c;
	usbd_status		stat;
	int			s;

	DPRINTFN(5,("%s: %s: enter\n", USBDEVNAME(sc->sc_dev), __func__));

	ifp->if_oerrors++;
	printf("%s: watchdog timeout\n", USBDEVNAME(sc->sc_dev));

	s = splusb();
	c = &sc->sc_tx_chain[0];
	usbd_get_xfer_status(c->uch_xfer, NULL, NULL, NULL, &stat);
	uax_txeof(c->uch_xfer, c, stat);

	if (IFQ_IS_EMPTY(&ifp->if_snd) == 0)
		uax_start(ifp);
	splx(s);
}

/*
 * Set media options.
 */
Static int
uax_ifmedia_upd(struct ifnet *ifp)
{
	struct uax_softc	*sc = ifp->if_softc;
	struct mii_data		*mii = GET_MII(sc);

	DPRINTFN(5,("%s: %s: enter\n", USBDEVNAME(sc->sc_dev), __func__));

	if (sc->sc_dying)
		return (0);

	sc->sc_link = 0;
	if (mii->mii_instance) {
		struct mii_softc	*miisc;
		for (miisc = LIST_FIRST(&mii->mii_phys); miisc != NULL;
		    miisc = LIST_NEXT(miisc, mii_list))
			 mii_phy_reset(miisc);
	}
	mii_mediachg(mii);

	return (0);
}

/*
 * Report current media status.
 */
Static void
uax_ifmedia_sts(struct ifnet *ifp, struct ifmediareq *ifmr)
{
	struct uax_softc	*sc = ifp->if_softc;
	struct mii_data		*mii = GET_MII(sc);

	DPRINTFN(5,("%s: %s: enter\n", USBDEVNAME(sc->sc_dev), __func__));

	mii_pollstat(mii);
	ifmr->ifm_active = mii->mii_media_active;
	ifmr->ifm_status = mii->mii_media_status;
}

Static int
uax_miibus_readreg(device_ptr_t dev, int phy, int reg)
{
	struct uax_softc *sc = USBGETSOFTC(dev);
	uWord val;
	usbd_status err;

	if (sc->sc_dying)
		return (0);

	/* We must limit the PHY address to avoid false hits. */
	if (phy >= sc->sc_nphys)
		return (0);

	uax_grab_mii(sc);
	err = uax_request(sc, UT_READ_VENDOR_DEVICE, UAX_READ_MII_REG,
			  phy, reg, sizeof val, &val);
	uax_ungrab_mii(sc);
	if (err) {
		DPRINTF(("%s: uax_miibus_readreg error=%s\n",
			 USBDEVNAME(sc->sc_dev), usbd_errstr(err)));
		return (0);
	}

	DPRINTFN(4,("uax_miibus_readreg: phy=%d reg=0x%x data=0x%08x\n", phy,
		    reg, UGETW(val)));
	return (UGETW(val));
}

Static void
uax_miibus_writereg(device_ptr_t dev, int phy, int reg, int data)
{
	struct uax_softc *sc = USBGETSOFTC(dev);
	uWord val;
	usbd_status err;

	if (sc->sc_dying)
		return;

	uax_grab_mii(sc);
	err = uax_request(sc, UT_WRITE_VENDOR_DEVICE, UAX_WRITE_MII_REG,
			  phy, reg, sizeof val, &val);
	uax_ungrab_mii(sc);
	if (err) {
		DPRINTF(("%s: uax_miibus_writereg error=%s\n",
			 USBDEVNAME(sc->sc_dev), usbd_errstr(err)));
	}
}

Static void
uax_miibus_statchg(device_ptr_t dev)
{
	struct uax_softc *sc = USBGETSOFTC(dev);
	/*struct mii_data	*mii = GET_MII(sc);
	  uint val;*/
	usbd_status err;

	DPRINTFN(5,("%s: %s: enter\n", USBDEVNAME(sc->sc_dev), __func__));

	if (sc->sc_dying)
		return;

#if 0
	if ((mii->mii_media_active & IFM_GMASK) == IFM_FDX)
		val = 0x02;
	else
		val = 0x00;
	err = uax_request(sc, UT_WRITE_VENDOR_DEVICE, UAX_WRITE_MEDIUM_STATUS,
			  val, 0, 0, NULL);
#endif
	if (err) {
		DPRINTF(("%s: uax_miibus_statchg error=%s\n",
			 USBDEVNAME(sc->sc_dev), usbd_errstr(err)));
	}
}

Static int
uax_send(struct uax_softc *sc, struct mbuf *m, int idx)
{
	struct uax_chain	*c;
	usbd_status		err;

	DPRINTFN(10,("%s: %s: enter\n", USBDEVNAME(sc->sc_dev),__func__));

	c = &sc->sc_tx_chain[idx];

	/* Copy data to tx buffer. */
	KASSERT(m->m_pkthdr.len <= UAX_BUFSZ);
	m_copydata(m, 0, m->m_pkthdr.len, c->uch_buf);
	c->uch_mbuf = m;

	usbd_setup_xfer(c->uch_xfer, sc->sc_ep[UAX_ENDPT_TX],
	    c, c->uch_buf, m->m_pkthdr.len,
	    USBD_FORCE_SHORT_XFER | USBD_NO_COPY,
	    UAX_TX_TIMEOUT, uax_txeof);

	DPRINTF(("%s: sending %d bytes\n", USBDEVNAME(sc->sc_dev),
		 m->m_pkthdr.len));
	/* Transmit */
	err = usbd_transfer(c->uch_xfer);
	if (err != USBD_IN_PROGRESS) {
		printf("%s: uax_send error=%s\n", USBDEVNAME(sc->sc_dev),
		       usbd_errstr(err));
		/* Stop the interface from process context. */
		usb_add_task(sc->sc_udev, &sc->sc_stop_task);
		return (EIO);
	}
	DPRINTFN(5,("%s: %s: send %d bytes\n", USBDEVNAME(sc->sc_dev),
		    __func__, m->m_pkthdr.len));

	sc->sc_tx_cnt++;

	return (0);
}

/*
 * A frame was downloaded to the chip. It's safe for us to clean up
 * the list buffers.
 */
Static void
uax_txeof(usbd_xfer_handle xfer, usbd_private_handle priv, usbd_status status)
{
	struct uax_chain	*c = priv;
	struct uax_softc	*sc = c->uch_sc;
	struct ifnet		*ifp = GET_IFP(sc);
	int			s;

	if (sc->sc_dying)
		return;

	DPRINTF(("uax_txoef: frame sent\n"));

	s = splnet();

	DPRINTFN(10,("%s: %s: enter status=%d\n", USBDEVNAME(sc->sc_dev),
		    __func__, status));

	ifp->if_timer = 0;
	ifp->if_flags &= ~IFF_OACTIVE;

	if (status != USBD_NORMAL_COMPLETION) {
		if (status == USBD_NOT_STARTED || status == USBD_CANCELLED) {
			splx(s);
			return;
		}
		ifp->if_oerrors++;
		printf("%s: usb error on tx: %s\n", USBDEVNAME(sc->sc_dev),
		    usbd_errstr(status));
		if (status == USBD_STALLED)
			usbd_clear_endpoint_stall(sc->sc_ep[UAX_ENDPT_TX]);
		splx(s);
		return;
	}

	ifp->if_opackets++;

	m_freem(c->uch_mbuf);
	c->uch_mbuf = NULL;

	if (IFQ_IS_EMPTY(&ifp->if_snd) == 0)
		uax_start(ifp);

	splx(s);
}

Static void
uax_start(struct ifnet *ifp)
{
	struct uax_softc	*sc = ifp->if_softc;
	struct mbuf		*m_head = NULL;

	DPRINTFN(5,("%s: %s: enter, link=%d\n", USBDEVNAME(sc->sc_dev),
		    __func__, sc->sc_link));

	if (sc->sc_dying)
		return;

	if (!sc->sc_link)
		return;

	if (ifp->if_flags & IFF_OACTIVE)
		return;

	IFQ_POLL(&ifp->if_snd, m_head);
	if (m_head == NULL)
		return;

	if (uax_send(sc, m_head, 0)) {
		ifp->if_flags |= IFF_OACTIVE;
		return;
	}

	IFQ_DEQUEUE(&ifp->if_snd, m_head);

#if NBPFILTER > 0
	/*
	 * If there's a BPF listener, bounce a copy of this frame
	 * to him.
	 */
	if (ifp->if_bpf)
		BPF_MTAP(ifp, m_head);
#endif

	ifp->if_flags |= IFF_OACTIVE;

	/*
	 * Set a timeout in case the chip goes out to lunch.
	 */
	ifp->if_timer = 5;
}

Static void
uax_stop_wrap(void *v)
{
	uax_stop(v, 0);
}

Static void
uax_stop(struct ifnet *ifp, int disable)
{
	struct uax_softc	*sc = ifp->if_softc;
	usbd_status		err;
	int			i;

	DPRINTFN(5,("%s: %s: enter disable=%d\n", USBDEVNAME(sc->sc_dev),
		    __func__, disable));

	ifp->if_timer = 0;

	/* XXX How do we stop the chip? */
	sc->sc_packet_filter &= ~UAX_RX_ALTERNATE;
	(void)uax_request(sc, UT_WRITE_VENDOR_DEVICE, UAX_WRITE_RX_CTRL,
			  sc->sc_packet_filter, 0, 0, NULL);
	/* XXX do more */

	usb_uncallout(sc->sc_stat_ch, uax_tick, sc);

	/* Stop transfers. */
	if (sc->sc_ep[UAX_ENDPT_RX] != NULL) {
		err = usbd_abort_pipe(sc->sc_ep[UAX_ENDPT_RX]);
		if (err) {
			printf("%s: abort rx pipe failed: %s\n",
			    USBDEVNAME(sc->sc_dev), usbd_errstr(err));
		}
		err = usbd_close_pipe(sc->sc_ep[UAX_ENDPT_RX]);
		if (err) {
			printf("%s: close rx pipe failed: %s\n",
			    USBDEVNAME(sc->sc_dev), usbd_errstr(err));
		}
		sc->sc_ep[UAX_ENDPT_RX] = NULL;
	}

	if (sc->sc_ep[UAX_ENDPT_TX] != NULL) {
		err = usbd_abort_pipe(sc->sc_ep[UAX_ENDPT_TX]);
		if (err) {
			printf("%s: abort tx pipe failed: %s\n",
			    USBDEVNAME(sc->sc_dev), usbd_errstr(err));
		}
		err = usbd_close_pipe(sc->sc_ep[UAX_ENDPT_TX]);
		if (err) {
			printf("%s: close tx pipe failed: %s\n",
			    USBDEVNAME(sc->sc_dev), usbd_errstr(err));
		}
		sc->sc_ep[UAX_ENDPT_TX] = NULL;
	}

	if (sc->sc_ep[UAX_ENDPT_INTR] != NULL) {
		err = usbd_abort_pipe(sc->sc_ep[UAX_ENDPT_INTR]);
		if (err) {
			printf("%s: abort intr pipe failed: %s\n",
			    USBDEVNAME(sc->sc_dev), usbd_errstr(err));
		}
		err = usbd_close_pipe(sc->sc_ep[UAX_ENDPT_INTR]);
		if (err) {
			printf("%s: close intr pipe failed: %s\n",
			    USBDEVNAME(sc->sc_dev), usbd_errstr(err));
		}
		sc->sc_ep[UAX_ENDPT_INTR] = NULL;
	}

	/* Free RX resources. */
	for (i = 0; i < UAX_RX_LIST_CNT; i++) {
		if (sc->sc_rx_chain[i].uch_mbuf != NULL) {
			m_freem(sc->sc_rx_chain[i].uch_mbuf);
			sc->sc_rx_chain[i].uch_mbuf = NULL;
		}
		if (sc->sc_rx_chain[i].uch_xfer != NULL) {
			usbd_free_xfer(sc->sc_rx_chain[i].uch_xfer);
			sc->sc_rx_chain[i].uch_xfer = NULL;
		}
	}

	/* Free TX resources. */
	for (i = 0; i < UAX_TX_LIST_CNT; i++) {
		if (sc->sc_tx_chain[i].uch_mbuf != NULL) {
			m_freem(sc->sc_tx_chain[i].uch_mbuf);
			sc->sc_tx_chain[i].uch_mbuf = NULL;
		}
		if (sc->sc_tx_chain[i].uch_xfer != NULL) {
			usbd_free_xfer(sc->sc_tx_chain[i].uch_xfer);
			sc->sc_tx_chain[i].uch_xfer = NULL;
		}
	}

	sc->sc_link = 0;

	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
}

/*
 * A frame has been uploaded: pass the resulting mbuf chain up to
 * the higher level protocols.
 */
Static void
uax_rxeof(usbd_xfer_handle xfer, usbd_private_handle priv, usbd_status status)
{
	struct uax_chain	*c = priv;
	struct uax_softc	*sc = c->uch_sc;
	struct ifnet		*ifp = GET_IFP(sc);
	struct mbuf		*m;
	u_int32_t		total_len;
	int			s;

	DPRINTFN(10,("%s: %s: enter\n", USBDEVNAME(sc->sc_dev), __func__));

	if (sc->sc_dying)
		return;

	if (!(ifp->if_flags & IFF_RUNNING))
		return;

	if (status != USBD_NORMAL_COMPLETION) {
		if (status == USBD_NOT_STARTED || status == USBD_CANCELLED)
			return;
		sc->sc_rx_errs++;
		if (usbd_ratecheck(&sc->sc_rx_notice)) {
			printf("%s: %u usb errors on rx: %s\n",
			    USBDEVNAME(sc->sc_dev), sc->sc_rx_errs,
			    usbd_errstr(status));
			sc->sc_rx_errs = 0;
		}
		if (status == USBD_STALLED)
			usbd_clear_endpoint_stall(sc->sc_ep[UAX_ENDPT_RX]);
		goto done;
	}

	usbd_get_xfer_status(xfer, NULL, NULL, &total_len, NULL);

	DPRINTF(("%s: got %d bytes\n", USBDEVNAME(sc->sc_dev), total_len));
	memcpy(mtod(c->uch_mbuf, char *), c->uch_buf, total_len);

	if (total_len == 0) {
		ifp->if_ierrors++;
		goto done;
	}

	/* No errors; receive the packet. */
	m = c->uch_mbuf;
	m->m_pkthdr.len = m->m_len = total_len;
	ifp->if_ipackets++;

	m->m_pkthdr.rcvif = ifp;

	s = splnet();

	/* XXX ugly */
	if (uax_newbuf(sc, c, NULL) == ENOBUFS) {
		ifp->if_ierrors++;
		goto done1;
	}

#if NBPFILTER > 0
	/*
	 * Handle BPF listeners. Let the BPF user see the packet, but
	 * don't pass it up to the ether_input() layer unless it's
	 * a broadcast packet, multicast packet, matches our ethernet
	 * address or the interface is in promiscuous mode.
	 */
	if (ifp->if_bpf)
		BPF_MTAP(ifp, m);
#endif

	DPRINTFN(10,("%s: %s: deliver %d\n", USBDEVNAME(sc->sc_dev),
		    __func__, m->m_len));
	IF_INPUT(ifp, m);
 done1:
	splx(s);

 done:

	/* Setup new transfer. */
	usbd_setup_xfer(xfer, sc->sc_ep[UAX_ENDPT_RX],
	    c, c->uch_buf, UAX_BUFSZ,
	    USBD_SHORT_XFER_OK | USBD_NO_COPY,
	    USBD_NO_TIMEOUT, uax_rxeof);
	usbd_transfer(xfer);

	DPRINTFN(10,("%s: %s: start rx\n", USBDEVNAME(sc->sc_dev),
		    __func__));
}

/*
 * Initialize an RX descriptor and attach an MBUF cluster.
 */
Static int
uax_newbuf(struct uax_softc *sc, struct uax_chain *c, struct mbuf *m)
{
	struct mbuf		*m_new = NULL;

	DPRINTFN(10,("%s: %s: enter\n", USBDEVNAME(sc->sc_dev),__func__));

	if (m == NULL) {
		MGETHDR(m_new, M_DONTWAIT, MT_DATA);
		if (m_new == NULL) {
			printf("%s: no memory for rx list "
			    "-- packet dropped!\n", USBDEVNAME(sc->sc_dev));
			return (ENOBUFS);
		}

		MCLGET(m_new, M_DONTWAIT);
		if (!(m_new->m_flags & M_EXT)) {
			printf("%s: no memory for rx list "
			    "-- packet dropped!\n", USBDEVNAME(sc->sc_dev));
			m_freem(m_new);
			return (ENOBUFS);
		}
		m_new->m_len = m_new->m_pkthdr.len = MCLBYTES;
	} else {
		m_new = m;
		m_new->m_len = m_new->m_pkthdr.len = MCLBYTES;
		m_new->m_data = m_new->m_ext.ext_buf;
	}

	m_adj(m_new, ETHER_ALIGN);
	c->uch_mbuf = m_new;

	return (0);
}

Static int
uax_rx_list_init(struct uax_softc *sc)
{
	struct uax_chain	*c;
	int			i;

	DPRINTFN(5,("%s: %s: enter\n", USBDEVNAME(sc->sc_dev), __func__));

	for (i = 0; i < UAX_RX_LIST_CNT; i++) {
		c = &sc->sc_rx_chain[i];
		c->uch_sc = sc;
		c->uch_idx = i;
		if (uax_newbuf(sc, c, NULL) == ENOBUFS)
			return (ENOBUFS);
		if (c->uch_xfer == NULL) {
			c->uch_xfer = usbd_alloc_xfer(sc->sc_udev);
			if (c->uch_xfer == NULL)
				return (ENOBUFS);
			c->uch_buf = usbd_alloc_buffer(c->uch_xfer, UAX_BUFSZ);
			if (c->uch_buf == NULL) {
				usbd_free_xfer(c->uch_xfer);
				c->uch_xfer = NULL;
				return (ENOBUFS);
			}
		}
	}

	return (0);
}

Static int
uax_tx_list_init(struct uax_softc *sc)
{
	struct uax_chain	*c;
	int			i;

	DPRINTFN(5,("%s: %s: enter\n", USBDEVNAME(sc->sc_dev), __func__));

	for (i = 0; i < UAX_TX_LIST_CNT; i++) {
		c = &sc->sc_tx_chain[i];
		c->uch_sc = sc;
		c->uch_idx = i;
		c->uch_mbuf = NULL;
		if (c->uch_xfer == NULL) {
			c->uch_xfer = usbd_alloc_xfer(sc->sc_udev);
			if (c->uch_xfer == NULL)
				return (ENOBUFS);
			c->uch_buf = usbd_alloc_buffer(c->uch_xfer, UAX_BUFSZ);
			if (c->uch_buf == NULL) {
				usbd_free_xfer(c->uch_xfer);
				c->uch_xfer = NULL;
				return (ENOBUFS);
			}
		}
	}

	return (0);
}

Static int
uax_init(struct ifnet *ifp)
{
	struct uax_softc	*sc = ifp->if_softc;

	DPRINTFN(5,("%s: %s: enter\n", USBDEVNAME(sc->sc_dev), __func__));

	if (sc->sc_dying)
		return (EIO);

	uax_stop(ifp, 0);

	/* Load the multicast filter. */
	uax_setmulti(sc);

	/* Init TX ring. */
	if (uax_tx_list_init(sc) == ENOBUFS) {
		printf("%s: tx list init failed\n", USBDEVNAME(sc->sc_dev));
		return (EIO);
	}

	/* Init RX ring. */
	if (uax_rx_list_init(sc) == ENOBUFS) {
		printf("%s: rx list init failed\n", USBDEVNAME(sc->sc_dev));
		return (EIO);
	}

	if (sc->sc_ep[UAX_ENDPT_RX] == NULL) {
		if (uax_openpipes(sc)) {
			return (EIO);
		}
	}

	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;

	usb_callout(sc->sc_stat_ch, hz, uax_tick, sc);

	sc->sc_packet_filter |= UAX_RX_ALTERNATE;
	(void)uax_request(sc, UT_WRITE_VENDOR_DEVICE, UAX_WRITE_RX_CTRL,
			  sc->sc_packet_filter, 0, 0, NULL);

	return (0);
}

Static void
uax_tick(void *xsc)
{
	struct uax_softc	*sc = xsc;

	DPRINTFN(15,("%s: %s: enter\n", USBDEVNAME(sc->sc_dev),__func__));

	if (sc == NULL)
		return;

	if (sc->sc_dying)
		return;

	/* Perform periodic stuff in process context. */
	usb_add_task(sc->sc_udev, &sc->sc_tick_task);
}

Static void
uax_tick_task(void *xsc)
{
	struct uax_softc	*sc = xsc;
	struct ifnet		*ifp;
	struct mii_data		*mii;
	int			s;

	DPRINTFN(15,("%s: %s: enter\n", USBDEVNAME(sc->sc_dev),__func__));

	if (sc->sc_dying)
		return;

	ifp = GET_IFP(sc);
	mii = GET_MII(sc);
	if (mii == NULL)
		return;

	s = splnet();

	mii_tick(mii);
	if (!sc->sc_link) {
		mii_pollstat(mii); /* XXX FreeBSD has removed this call */
		if (mii->mii_media_status & IFM_ACTIVE &&
		    IFM_SUBTYPE(mii->mii_media_active) != IFM_NONE) {
			DPRINTFN(2,("%s: %s: got link\n",
				    USBDEVNAME(sc->sc_dev),__func__));
			sc->sc_link++;
			if (IFQ_IS_EMPTY(&ifp->if_snd) == 0)
				uax_start(ifp);
		}
	}

	usb_callout(sc->sc_stat_ch, hz, uax_tick, sc);

	splx(s);
}

Static int
uax_openpipes(struct uax_softc *sc)
{
	struct uax_chain	*c;
	usbd_status		err;
	int i;

	/* Open RX and TX pipes. */
	err = usbd_open_pipe(sc->sc_iface, sc->sc_ed[UAX_ENDPT_RX],
	    USBD_EXCLUSIVE_USE, &sc->sc_ep[UAX_ENDPT_RX]);
	if (err) {
		printf("%s: open rx pipe failed: %s\n",
		    USBDEVNAME(sc->sc_dev), usbd_errstr(err));
		return (EIO);
	}
	err = usbd_open_pipe(sc->sc_iface, sc->sc_ed[UAX_ENDPT_TX],
	    USBD_EXCLUSIVE_USE, &sc->sc_ep[UAX_ENDPT_TX]);
	if (err) {
		printf("%s: open tx pipe failed: %s\n",
		    USBDEVNAME(sc->sc_dev), usbd_errstr(err));
		return (EIO);
	}
	err = usbd_open_pipe_intr(sc->sc_iface, sc->sc_ed[UAX_ENDPT_INTR],
	    USBD_EXCLUSIVE_USE, &sc->sc_ep[UAX_ENDPT_INTR], sc,
	    &sc->sc_ibuf, UAX_INTR_PKTLEN, uax_intr,
	    UAX_INTR_INTERVAL);
	if (err) {
		printf("%s: open intr pipe failed: %s\n",
		    USBDEVNAME(sc->sc_dev), usbd_errstr(err));
		return (EIO);
	}

	/* Start up the receive pipe. */
	for (i = 0; i < UAX_RX_LIST_CNT; i++) {
		c = &sc->sc_rx_chain[i];
		usbd_setup_xfer(c->uch_xfer, sc->sc_ep[UAX_ENDPT_RX],
		    c, c->uch_buf, UAX_BUFSZ,
		    USBD_SHORT_XFER_OK | USBD_NO_COPY, USBD_NO_TIMEOUT,
		    uax_rxeof);
		(void)usbd_transfer(c->uch_xfer); /* XXX */
		DPRINTFN(5,("%s: %s: start read\n", USBDEVNAME(sc->sc_dev),
			    __func__));

	}
	return (0);
}

Static void
uax_intr(usbd_xfer_handle xfer, usbd_private_handle priv, usbd_status status)
{
	struct uax_softc	*sc = priv;
	struct ifnet		*ifp = GET_IFP(sc);
	/*struct uax_intrpkt	*p = &sc->sc_ibuf;*/

	DPRINTFN(15,("%s: %s: enter\n", USBDEVNAME(sc->sc_dev),__func__));

	if (sc->sc_dying)
		return;

	if (!(ifp->if_flags & IFF_RUNNING))
		return;

	if (status != USBD_NORMAL_COMPLETION) {
		if (status == USBD_NOT_STARTED || status == USBD_CANCELLED) {
			return;
		}
		sc->sc_intr_errs++;
		if (usbd_ratecheck(&sc->sc_rx_notice)) {
			printf("%s: %u usb errors on intr: %s\n",
			    USBDEVNAME(sc->sc_dev), sc->sc_intr_errs,
			    usbd_errstr(status));
			sc->sc_intr_errs = 0;
		}
		if (status == USBD_STALLED)
			usbd_clear_endpoint_stall(sc->sc_ep[UAX_ENDPT_RX]);
		return;
	}

}

/* Polynomial calculation taken from the Windows driver. */
Static u_int32_t
uax_crc(u_int8_t *eaddr)
{
	u_int32_t crc32;
	u_int8_t byte, carry;
	int i, j;

	crc32 = 0xffffffff;
	for (i = 0; i < ETHER_ADDR_LEN; i++) {
		byte = eaddr[i];
		for (j = 0; j < 8; j++)	{
			carry = byte & 1;
			if (crc32 & 0x80000000)
				carry ^= 1;
			crc32 <<= 1;
			byte >>= 1;
			if (carry)
				crc32 ^= 0x04c11db7;
		}
	}
	return (crc32);
}

Static void
uax_setmulti(struct uax_softc *sc)
{
	struct ifnet		*ifp;
	struct ether_multi	*enm;
	struct ether_multistep	step;
	u_int32_t		h;
	int			nmcast;

	DPRINTFN(5,("%s: %s: enter\n", USBDEVNAME(sc->sc_dev), __func__));

	ifp = GET_IFP(sc);

	sc->sc_packet_filter &= ~(UAX_RX_PROMISCUOUS |
				  UAX_RX_ALL_MULTICAST |
				  UAX_RX_MULTICAST);
	if (ifp->if_flags & IFF_PROMISC) {
		sc->sc_packet_filter |= UAX_RX_PROMISCUOUS;
	} else {
		ifp->if_flags &= ~IFF_ALLMULTI;

		memset(sc->sc_mcast, 0, sizeof sc->sc_mcast);
		nmcast = 0;
		ETHER_FIRST_MULTI(step, &sc->sc_ec, enm);
		while (enm != NULL) {
			if (memcmp(enm->enm_addrlo,
				   enm->enm_addrhi, ETHER_ADDR_LEN) != 0) {
				ifp->if_flags |= IFF_ALLMULTI;
				sc->sc_packet_filter |= UAX_RX_ALL_MULTICAST;
				nmcast = 0;
				break;
			}
			h = (uax_crc(enm->enm_addrlo) >> 26) & 0x3f;
			sc->sc_mcast[h / 8] |= 1 << (h % 8);

			ETHER_NEXT_MULTI(step, enm);
			nmcast++;
		}
		if (nmcast > 0)
			sc->sc_packet_filter |= UAX_RX_MULTICAST;
	}
	/* Set the multicast filter. */
	(void)uax_request(sc, UT_WRITE_VENDOR_DEVICE, UAX_WRITE_MULTI_FILTER,
			  0, 0, sizeof sc->sc_mcast, &sc->sc_mcast);
	/* And tell the chip which mode we want. */
	(void)uax_request(sc, UT_WRITE_VENDOR_DEVICE, UAX_WRITE_RX_CTRL,
			  sc->sc_packet_filter, 0, 0, NULL);
}

