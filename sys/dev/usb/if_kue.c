/*	$NetBSD: if_kue.c,v 1.111 2022/03/03 05:52:46 riastradh Exp $	*/

/*
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
 * $FreeBSD: src/sys/dev/usb/if_kue.c,v 1.14 2000/01/14 01:36:15 wpaul Exp $
 */

/*
 * Kawasaki LSI KL5KUSB101B USB to ethernet adapter driver.
 *
 * Written by Bill Paul <wpaul@ee.columbia.edu>
 * Electrical Engineering Department
 * Columbia University, New York City
 */

/*
 * The KLSI USB to ethernet adapter chip contains an USB serial interface,
 * ethernet MAC and embedded microcontroller (called the QT Engine).
 * The chip must have firmware loaded into it before it will operate.
 * Packets are passed between the chip and host via bulk transfers.
 * There is an interrupt endpoint mentioned in the software spec, however
 * it's currently unused. This device is 10Mbps half-duplex only, hence
 * there is no media selection logic. The MAC supports a 128 entry
 * multicast filter, though the exact size of the filter can depend
 * on the firmware. Curiously, while the software spec describes various
 * ethernet statistics counters, my sample adapter and firmware combination
 * claims not to support any statistics counters at all.
 *
 * Note that once we load the firmware in the device, we have to be
 * careful not to load it again: if you restart your computer but
 * leave the adapter attached to the USB controller, it may remain
 * powered on and retain its firmware. In this case, we don't need
 * to load the firmware a second time.
 *
 * Special thanks to Rob Furr for providing an ADS Technologies
 * adapter for development and testing. No monkeys were harmed during
 * the development of this driver.
 */

/*
 * Ported to NetBSD and somewhat rewritten by Lennart Augustsson.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_kue.c,v 1.111 2022/03/03 05:52:46 riastradh Exp $");

#ifdef _KERNEL_OPT
#include "opt_inet.h"
#include "opt_usb.h"
#endif

#include <sys/param.h>
#include <sys/kmem.h>

#include <dev/usb/usbnet.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/if_inarp.h>
#endif

#include <dev/usb/if_kuereg.h>
#include <dev/usb/kue_fw.h>

#ifdef KUE_DEBUG
#define DPRINTF(x)	if (kuedebug) printf x
#define DPRINTFN(n, x)	if (kuedebug >= (n)) printf x
int	kuedebug = 0;
#else
#define DPRINTF(x)
#define DPRINTFN(n, x)
#endif

struct kue_type {
	uint16_t		kue_vid;
	uint16_t		kue_did;
};

struct kue_softc {
	struct usbnet		kue_un;

	struct kue_ether_desc	kue_desc;
	uint16_t		kue_rxfilt;
	uint8_t			*kue_mcfilters;
};

#define KUE_MCFILT(x, y)	\
	(uint8_t *)&(sc->kue_mcfilters[y * ETHER_ADDR_LEN])

#define KUE_BUFSZ		1536
#define KUE_MIN_FRAMELEN	60

#define KUE_RX_LIST_CNT		1
#define KUE_TX_LIST_CNT		1

/*
 * Various supported device vendors/products.
 */
static const struct usb_devno kue_devs[] = {
	{ USB_VENDOR_3COM, USB_PRODUCT_3COM_3C19250 },
	{ USB_VENDOR_3COM, USB_PRODUCT_3COM_3C460 },
	{ USB_VENDOR_ABOCOM, USB_PRODUCT_ABOCOM_URE450 },
	{ USB_VENDOR_ADS, USB_PRODUCT_ADS_UBS10BT },
	{ USB_VENDOR_ADS, USB_PRODUCT_ADS_UBS10BTX },
	{ USB_VENDOR_ACTIONTEC, USB_PRODUCT_ACTIONTEC_AR9287 },
	{ USB_VENDOR_ALLIEDTELESYN, USB_PRODUCT_ALLIEDTELESYN_AT_USB10 },
	{ USB_VENDOR_AOX, USB_PRODUCT_AOX_USB101 },
	{ USB_VENDOR_ASANTE, USB_PRODUCT_ASANTE_EA },
	{ USB_VENDOR_ATEN, USB_PRODUCT_ATEN_UC10T },
	{ USB_VENDOR_ATEN, USB_PRODUCT_ATEN_DSB650C },
	{ USB_VENDOR_COREGA, USB_PRODUCT_COREGA_ETHER_USB_T },
	{ USB_VENDOR_DLINK, USB_PRODUCT_DLINK_DSB650C },
	{ USB_VENDOR_ENTREGA, USB_PRODUCT_ENTREGA_E45 },
	{ USB_VENDOR_ENTREGA, USB_PRODUCT_ENTREGA_XX1 },
	{ USB_VENDOR_ENTREGA, USB_PRODUCT_ENTREGA_XX2 },
	{ USB_VENDOR_IODATA, USB_PRODUCT_IODATA_USBETT },
	{ USB_VENDOR_JATON, USB_PRODUCT_JATON_EDA },
	{ USB_VENDOR_KINGSTON, USB_PRODUCT_KINGSTON_XX1 },
	{ USB_VENDOR_KLSI, USB_PRODUCT_KLSI_DUH3E10BT },
	{ USB_VENDOR_KLSI, USB_PRODUCT_KLSI_DUH3E10BTN },
	{ USB_VENDOR_LINKSYS, USB_PRODUCT_LINKSYS_USB10T },
	{ USB_VENDOR_MOBILITY, USB_PRODUCT_MOBILITY_EA },
	{ USB_VENDOR_NETGEAR, USB_PRODUCT_NETGEAR_EA101 },
	{ USB_VENDOR_NETGEAR, USB_PRODUCT_NETGEAR_EA101X },
	{ USB_VENDOR_PERACOM, USB_PRODUCT_PERACOM_ENET },
	{ USB_VENDOR_PERACOM, USB_PRODUCT_PERACOM_ENET2 },
	{ USB_VENDOR_PERACOM, USB_PRODUCT_PERACOM_ENET3 },
	{ USB_VENDOR_PORTGEAR, USB_PRODUCT_PORTGEAR_EA8 },
	{ USB_VENDOR_PORTGEAR, USB_PRODUCT_PORTGEAR_EA9 },
	{ USB_VENDOR_PORTSMITH, USB_PRODUCT_PORTSMITH_EEA },
	{ USB_VENDOR_SHARK, USB_PRODUCT_SHARK_PA },
	{ USB_VENDOR_SILICOM, USB_PRODUCT_SILICOM_U2E },
	{ USB_VENDOR_SILICOM, USB_PRODUCT_SILICOM_GPE },
	{ USB_VENDOR_SMC, USB_PRODUCT_SMC_2102USB },
};
#define kue_lookup(v, p) (usb_lookup(kue_devs, v, p))

static int kue_match(device_t, cfdata_t, void *);
static void kue_attach(device_t, device_t, void *);
static int kue_detach(device_t, int);

CFATTACH_DECL_NEW(kue, sizeof(struct kue_softc), kue_match, kue_attach,
    kue_detach, usbnet_activate);

static void kue_uno_rx_loop(struct usbnet *, struct usbnet_chain *, uint32_t);
static unsigned kue_uno_tx_prepare(struct usbnet *, struct mbuf *,
				   struct usbnet_chain *);
static void kue_uno_mcast(struct ifnet *);
static int kue_uno_init(struct ifnet *);

static const struct usbnet_ops kue_ops = {
	.uno_mcast = kue_uno_mcast,
	.uno_tx_prepare = kue_uno_tx_prepare,
	.uno_rx_loop = kue_uno_rx_loop,
	.uno_init = kue_uno_init,
};

static void kue_reset(struct usbnet *);

static usbd_status kue_ctl(struct usbnet *, int, uint8_t,
			   uint16_t, void *, uint32_t);
static int kue_load_fw(struct usbnet *);

static usbd_status
kue_setword(struct usbnet *un, uint8_t breq, uint16_t word)
{
	usb_device_request_t	req;

	DPRINTFN(10,("%s: %s: enter\n", device_xname(un->un_dev),__func__));

	req.bmRequestType = UT_WRITE_VENDOR_DEVICE;
	req.bRequest = breq;
	USETW(req.wValue, word);
	USETW(req.wIndex, 0);
	USETW(req.wLength, 0);

	return usbd_do_request(un->un_udev, &req, NULL);
}

static usbd_status
kue_ctl(struct usbnet *un, int rw, uint8_t breq, uint16_t val,
	void *data, uint32_t len)
{
	usb_device_request_t	req;

	DPRINTFN(10,("%s: %s: enter, len=%d\n", device_xname(un->un_dev),
		     __func__, len));

	if (rw == KUE_CTL_WRITE)
		req.bmRequestType = UT_WRITE_VENDOR_DEVICE;
	else
		req.bmRequestType = UT_READ_VENDOR_DEVICE;

	req.bRequest = breq;
	USETW(req.wValue, val);
	USETW(req.wIndex, 0);
	USETW(req.wLength, len);

	return usbd_do_request(un->un_udev, &req, data);
}

static int
kue_load_fw(struct usbnet *un)
{
	usb_device_descriptor_t dd;
	usbd_status		err;

	DPRINTFN(1,("%s: %s: enter\n", device_xname(un->un_dev), __func__));

	/*
	 * First, check if we even need to load the firmware.
	 * If the device was still attached when the system was
	 * rebooted, it may already have firmware loaded in it.
	 * If this is the case, we don't need to do it again.
	 * And in fact, if we try to load it again, we'll hang,
	 * so we have to avoid this condition if we don't want
	 * to look stupid.
	 *
	 * We can test this quickly by checking the bcdRevision
	 * code. The NIC will return a different revision code if
	 * it's probed while the firmware is still loaded and
	 * running.
	 */
	if (usbd_get_device_desc(un->un_udev, &dd))
		return EIO;
	if (UGETW(dd.bcdDevice) == KUE_WARM_REV) {
		printf("%s: warm boot, no firmware download\n",
		       device_xname(un->un_dev));
		return 0;
	}

	printf("%s: cold boot, downloading firmware\n",
	       device_xname(un->un_dev));

	/* Load code segment */
	DPRINTFN(1,("%s: kue_load_fw: download code_seg\n",
		    device_xname(un->un_dev)));
	/*XXXUNCONST*/
	err = kue_ctl(un, KUE_CTL_WRITE, KUE_CMD_SEND_SCAN,
	    0, __UNCONST(kue_code_seg), sizeof(kue_code_seg));
	if (err) {
		printf("%s: failed to load code segment: %s\n",
		    device_xname(un->un_dev), usbd_errstr(err));
			return EIO;
	}

	/* Load fixup segment */
	DPRINTFN(1,("%s: kue_load_fw: download fix_seg\n",
		    device_xname(un->un_dev)));
	/*XXXUNCONST*/
	err = kue_ctl(un, KUE_CTL_WRITE, KUE_CMD_SEND_SCAN,
	    0, __UNCONST(kue_fix_seg), sizeof(kue_fix_seg));
	if (err) {
		printf("%s: failed to load fixup segment: %s\n",
		    device_xname(un->un_dev), usbd_errstr(err));
			return EIO;
	}

	/* Send trigger command. */
	DPRINTFN(1,("%s: kue_load_fw: download trig_seg\n",
		    device_xname(un->un_dev)));
	/*XXXUNCONST*/
	err = kue_ctl(un, KUE_CTL_WRITE, KUE_CMD_SEND_SCAN,
	    0, __UNCONST(kue_trig_seg), sizeof(kue_trig_seg));
	if (err) {
		printf("%s: failed to load trigger segment: %s\n",
		    device_xname(un->un_dev), usbd_errstr(err));
			return EIO;
	}

	usbd_delay_ms(un->un_udev, 10);

	/*
	 * Reload device descriptor.
	 * Why? The chip without the firmware loaded returns
	 * one revision code. The chip with the firmware
	 * loaded and running returns a *different* revision
	 * code. This confuses the quirk mechanism, which is
	 * dependent on the revision data.
	 */
	(void)usbd_reload_device_desc(un->un_udev);

	DPRINTFN(1,("%s: %s: done\n", device_xname(un->un_dev), __func__));

	/* Reset the adapter. */
	kue_reset(un);

	return 0;
}

static void
kue_setiff_locked(struct usbnet *un)
{
	struct ethercom *	ec = usbnet_ec(un);
	struct kue_softc *	sc = usbnet_softc(un);
	struct ifnet * const	ifp = usbnet_ifp(un);
	struct ether_multi	*enm;
	struct ether_multistep	step;
	int			i;

	DPRINTFN(5,("%s: %s: enter\n", device_xname(un->un_dev), __func__));

	 /* If we want promiscuous mode, set the allframes bit. */
	if (ifp->if_flags & IFF_PROMISC)
		sc->kue_rxfilt |= KUE_RXFILT_PROMISC;
	else
		sc->kue_rxfilt &= ~KUE_RXFILT_PROMISC;

	if (ifp->if_flags & IFF_PROMISC) {
allmulti:
		ifp->if_flags |= IFF_ALLMULTI;
		sc->kue_rxfilt |= KUE_RXFILT_ALLMULTI|KUE_RXFILT_PROMISC;
		sc->kue_rxfilt &= ~KUE_RXFILT_MULTICAST;
		kue_setword(un, KUE_CMD_SET_PKT_FILTER, sc->kue_rxfilt);
		return;
	}

	sc->kue_rxfilt &= ~(KUE_RXFILT_ALLMULTI|KUE_RXFILT_PROMISC);

	i = 0;
	ETHER_LOCK(ec);
	ETHER_FIRST_MULTI(step, ec, enm);
	while (enm != NULL) {
		if (i == KUE_MCFILTCNT(sc) ||
		    memcmp(enm->enm_addrlo, enm->enm_addrhi,
		    ETHER_ADDR_LEN) != 0) {
			ETHER_UNLOCK(ec);
			goto allmulti;
		}

		memcpy(KUE_MCFILT(sc, i), enm->enm_addrlo, ETHER_ADDR_LEN);
		ETHER_NEXT_MULTI(step, enm);
		i++;
	}
	ETHER_UNLOCK(ec);

	ifp->if_flags &= ~IFF_ALLMULTI;

	sc->kue_rxfilt |= KUE_RXFILT_MULTICAST;
	kue_ctl(un, KUE_CTL_WRITE, KUE_CMD_SET_MCAST_FILTERS,
	    i, sc->kue_mcfilters, i * ETHER_ADDR_LEN);

	kue_setword(un, KUE_CMD_SET_PKT_FILTER, sc->kue_rxfilt);
}

/*
 * Issue a SET_CONFIGURATION command to reset the MAC. This should be
 * done after the firmware is loaded into the adapter in order to
 * bring it into proper operation.
 */
static void
kue_reset(struct usbnet *un)
{
	DPRINTFN(5,("%s: %s: enter\n", device_xname(un->un_dev), __func__));

	if (usbd_set_config_no(un->un_udev, KUE_CONFIG_NO, 1) ||
	    usbd_device2interface_handle(un->un_udev, KUE_IFACE_IDX,
					 &un->un_iface))
		printf("%s: reset failed\n", device_xname(un->un_dev));

	/* Wait a little while for the chip to get its brains in order. */
	usbd_delay_ms(un->un_udev, 10);
}

/*
 * Probe for a KLSI chip.
 */
static int
kue_match(device_t parent, cfdata_t match, void *aux)
{
	struct usb_attach_arg *uaa = aux;

	DPRINTFN(25,("kue_match: enter\n"));

	return kue_lookup(uaa->uaa_vendor, uaa->uaa_product) != NULL ?
		UMATCH_VENDOR_PRODUCT : UMATCH_NONE;
}

/*
 * Attach the interface. Allocate softc structures, do
 * setup and ethernet/BPF attach.
 */
static void
kue_attach(device_t parent, device_t self, void *aux)
{
	struct kue_softc *sc = device_private(self);
	struct usbnet * const un = &sc->kue_un;
	struct usb_attach_arg *uaa = aux;
	char			*devinfop;
	struct usbd_device *	dev = uaa->uaa_device;
	usbd_status		err;
	usb_interface_descriptor_t	*id;
	usb_endpoint_descriptor_t	*ed;
	int			i;

	KASSERT((void *)sc == un);

	DPRINTFN(5,(" : kue_attach: sc=%p, dev=%p", sc, dev));

	aprint_naive("\n");
	aprint_normal("\n");
	devinfop = usbd_devinfo_alloc(dev, 0);
	aprint_normal_dev(self, "%s\n", devinfop);
	usbd_devinfo_free(devinfop);

	un->un_dev = self;
	un->un_udev = dev;
	un->un_sc = sc;
	un->un_ops = &kue_ops;
	un->un_rx_xfer_flags = USBD_SHORT_XFER_OK;
	un->un_tx_xfer_flags = 0;
	un->un_rx_list_cnt = KUE_RX_LIST_CNT;
	un->un_tx_list_cnt = KUE_TX_LIST_CNT;
	un->un_rx_bufsz = KUE_BUFSZ;
	un->un_tx_bufsz = KUE_BUFSZ;

	err = usbd_set_config_no(dev, KUE_CONFIG_NO, 1);
	if (err) {
		aprint_error_dev(self, "failed to set configuration"
		    ", err=%s\n", usbd_errstr(err));
		return;
	}

	/* Load the firmware into the NIC. */
	if (kue_load_fw(un)) {
		aprint_error_dev(self, "loading firmware failed\n");
		return;
	}

	err = usbd_device2interface_handle(dev, KUE_IFACE_IDX, &un->un_iface);
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
		} else if (UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_IN &&
			   UE_GET_XFERTYPE(ed->bmAttributes) == UE_INTERRUPT) {
			/*
			 * The interrupt endpoint is currently unused by the
			 * KLSI part.
			 */
			un->un_ed[USBNET_ENDPT_INTR] = ed->bEndpointAddress;
		}
	}

	if (un->un_ed[USBNET_ENDPT_RX] == 0 ||
	    un->un_ed[USBNET_ENDPT_TX] == 0) {
		aprint_error_dev(self, "missing endpoint\n");
		return;
	}

	/* First level attach, so kue_ctl() works. */
	usbnet_attach(un, "kuedet");

	/* Read ethernet descriptor */
	err = kue_ctl(un, KUE_CTL_READ, KUE_CMD_GET_ETHER_DESCRIPTOR,
	    0, &sc->kue_desc, sizeof(sc->kue_desc));
	if (err) {
		aprint_error_dev(self, "could not read Ethernet descriptor\n");
		return;
	}
	memcpy(un->un_eaddr, sc->kue_desc.kue_macaddr, sizeof(un->un_eaddr));

	sc->kue_mcfilters = kmem_alloc(KUE_MCFILTCNT(sc) * ETHER_ADDR_LEN,
	    KM_SLEEP);

	usbnet_attach_ifp(un, IFF_SIMPLEX | IFF_BROADCAST | IFF_MULTICAST,
	    0, NULL);
}

static int
kue_detach(device_t self, int flags)
{
	struct kue_softc *sc = device_private(self);

	if (sc->kue_mcfilters != NULL) {
		kmem_free(sc->kue_mcfilters,
		    KUE_MCFILTCNT(sc) * ETHER_ADDR_LEN);
		sc->kue_mcfilters = NULL;
	}

	return usbnet_detach(self, flags);
}

/*
 * A frame has been uploaded: pass the resulting mbuf chain up to
 * the higher level protocols.
 */
static void
kue_uno_rx_loop(struct usbnet *un, struct usbnet_chain *c, uint32_t total_len)
{
	struct ifnet		*ifp = usbnet_ifp(un);
	uint8_t			*buf = c->unc_buf;
	unsigned		pktlen;

	if (total_len <= 1)
		return;

	DPRINTFN(10,("%s: %s: total_len=%d len=%d\n",
		     device_xname(un->un_dev), __func__,
		     total_len, le16dec(buf)));

	pktlen = le16dec(buf);
	if (pktlen > total_len - ETHER_ALIGN)
		pktlen = total_len - ETHER_ALIGN;

	if (pktlen < ETHER_MIN_LEN - ETHER_CRC_LEN ||
	    pktlen > MCLBYTES - ETHER_ALIGN) {
		if_statinc(ifp, if_ierrors);
		return;
	}

	DPRINTFN(10,("%s: %s: deliver %d\n", device_xname(un->un_dev),
		    __func__, pktlen));
	usbnet_enqueue(un, buf + 2, pktlen, 0, 0, 0);
}

static unsigned
kue_uno_tx_prepare(struct usbnet *un, struct mbuf *m, struct usbnet_chain *c)
{
	unsigned		total_len, pkt_len;

	pkt_len = m->m_pkthdr.len + 2;
	total_len = roundup2(pkt_len, 64);

	if ((unsigned)total_len > un->un_tx_bufsz) {
		DPRINTFN(10,("%s: %s: too big pktlen %u total %u\n",
		    device_xname(un->un_dev), __func__, pkt_len, total_len));
		return 0;
	}

	/* Frame length is specified in the first 2 bytes of the buffer. */
	le16enc(c->unc_buf, (uint16_t)m->m_pkthdr.len);

	/*
	 * Copy the mbuf data into a contiguous buffer after the frame length,
	 * possibly zeroing the rest of the buffer.
	 */
	m_copydata(m, 0, m->m_pkthdr.len, c->unc_buf + 2);
	if (total_len - pkt_len > 0)
		memset(c->unc_buf + pkt_len, 0, total_len - pkt_len);

	DPRINTFN(10,("%s: %s: enter pktlen %u total %u\n",
	    device_xname(un->un_dev), __func__, pkt_len, total_len));

	return total_len;
}

static int
kue_uno_init(struct ifnet *ifp)
{
	struct usbnet * const	un = ifp->if_softc;
	struct kue_softc	*sc = usbnet_softc(un);
	uint8_t			eaddr[ETHER_ADDR_LEN];

	DPRINTFN(5,("%s: %s: enter\n", device_xname(un->un_dev),__func__));

	if (usbnet_isdying(un))
		return EIO;

	/* Cancel pending I/O */
	usbnet_stop(un, ifp, 1);

	memcpy(eaddr, CLLADDR(ifp->if_sadl), sizeof(eaddr));
	/* Set MAC address */
	kue_ctl(un, KUE_CTL_WRITE, KUE_CMD_SET_MAC, 0, eaddr, ETHER_ADDR_LEN);

	sc->kue_rxfilt = KUE_RXFILT_UNICAST | KUE_RXFILT_BROADCAST;

	/* I'm not sure how to tune these. */
#if 0
	/*
	 * Leave this one alone for now; setting it
	 * wrong causes lockups on some machines/controllers.
	 */
	kue_setword(un, KUE_CMD_SET_SOFS, 1);
#endif
	kue_setword(un, KUE_CMD_SET_URB_SIZE, 64);

	/* Load the multicast filter. */
	kue_setiff_locked(un);

	return usbnet_init_rx_tx(un);
}

static void
kue_uno_mcast(struct ifnet *ifp)
{
	struct usbnet * const	un = ifp->if_softc;

	kue_setiff_locked(un);
}

#ifdef _MODULE
#include "ioconf.c"
#endif

USBNET_MODULE(kue)
