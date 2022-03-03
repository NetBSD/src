/*	$NetBSD: if_cue.c,v 1.104 2022/03/03 05:55:29 riastradh Exp $	*/

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
 * $FreeBSD: src/sys/dev/usb/if_cue.c,v 1.4 2000/01/16 22:45:06 wpaul Exp $
 */

/*
 * CATC USB-EL1210A USB to ethernet driver. Used in the CATC Netmate
 * adapters and others.
 *
 * Written by Bill Paul <wpaul@ee.columbia.edu>
 * Electrical Engineering Department
 * Columbia University, New York City
 */

/*
 * The CATC USB-EL1210A provides USB ethernet support at 10Mbps. The
 * RX filter uses a 512-bit multicast hash table, single perfect entry
 * for the station address, and promiscuous mode. Unlike the ADMtek
 * and KLSI chips, the CATC ASIC supports read and write combining
 * mode where multiple packets can be transferred using a single bulk
 * transaction, which helps performance a great deal.
 */

/*
 * Ported to NetBSD and somewhat rewritten by Lennart Augustsson.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_cue.c,v 1.104 2022/03/03 05:55:29 riastradh Exp $");

#ifdef _KERNEL_OPT
#include "opt_inet.h"
#include "opt_usb.h"
#endif

#include <sys/param.h>

#include <dev/usb/usbnet.h>
#include <dev/usb/if_cuereg.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/if_inarp.h>
#endif

#ifdef CUE_DEBUG
#define DPRINTF(x)	if (cuedebug) printf x
#define DPRINTFN(n, x)	if (cuedebug >= (n)) printf x
int	cuedebug = 0;
#else
#define DPRINTF(x)
#define DPRINTFN(n, x)
#endif

#define CUE_BUFSZ		1536
#define CUE_MIN_FRAMELEN	60
#define CUE_RX_FRAMES		1
#define CUE_TX_FRAMES		1

#define CUE_CONFIG_NO		1
#define CUE_IFACE_IDX		0

#define CUE_RX_LIST_CNT		1
#define CUE_TX_LIST_CNT		1

struct cue_type {
	uint16_t		cue_vid;
	uint16_t		cue_did;
};

struct cue_softc;

struct cue_chain {
	struct cue_softc	*cue_sc;
	struct usbd_xfer	*cue_xfer;
	char			*cue_buf;
	struct mbuf		*cue_mbuf;
	int			cue_idx;
};

struct cue_cdata {
	struct cue_chain	cue_tx_chain[CUE_TX_LIST_CNT];
	struct cue_chain	cue_rx_chain[CUE_RX_LIST_CNT];
	int			cue_tx_prod;
	int			cue_tx_cnt;
};

struct cue_softc {
	struct usbnet		cue_un;
	uint8_t			cue_mctab[CUE_MCAST_TABLE_LEN];
};

/*
 * Various supported device vendors/products.
 */
static const struct usb_devno cue_devs[] = {
	{ USB_VENDOR_CATC, USB_PRODUCT_CATC_NETMATE },
	{ USB_VENDOR_CATC, USB_PRODUCT_CATC_NETMATE2 },
	{ USB_VENDOR_SMARTBRIDGES, USB_PRODUCT_SMARTBRIDGES_SMARTLINK },
	/* Belkin F5U111 adapter covered by NETMATE entry */
};
#define cue_lookup(v, p) (usb_lookup(cue_devs, v, p))

static int cue_match(device_t, cfdata_t, void *);
static void cue_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(cue, sizeof(struct cue_softc), cue_match, cue_attach,
    usbnet_detach, usbnet_activate);

static unsigned cue_uno_tx_prepare(struct usbnet *, struct mbuf *,
				   struct usbnet_chain *);
static void cue_uno_rx_loop(struct usbnet *, struct usbnet_chain *, uint32_t);
static void cue_uno_mcast(struct ifnet *);
static void cue_uno_stop(struct ifnet *, int);
static int cue_uno_init(struct ifnet *);
static void cue_uno_tick(struct usbnet *);

static const struct usbnet_ops cue_ops = {
	.uno_stop = cue_uno_stop,
	.uno_mcast = cue_uno_mcast,
	.uno_tx_prepare = cue_uno_tx_prepare,
	.uno_rx_loop = cue_uno_rx_loop,
	.uno_init = cue_uno_init,
	.uno_tick = cue_uno_tick,
};

#ifdef CUE_DEBUG
static int
cue_csr_read_1(struct usbnet *un, int reg)
{
	usb_device_request_t	req;
	usbd_status		err;
	uint8_t			val = 0;

	if (usbnet_isdying(un))
		return 0;

	req.bmRequestType = UT_READ_VENDOR_DEVICE;
	req.bRequest = CUE_CMD_READREG;
	USETW(req.wValue, 0);
	USETW(req.wIndex, reg);
	USETW(req.wLength, 1);

	err = usbd_do_request(un->un_udev, &req, &val);

	if (err) {
		DPRINTF(("%s: cue_csr_read_1: reg=%#x err=%s\n",
		    device_xname(un->un_dev), reg, usbd_errstr(err)));
		return 0;
	}

	DPRINTFN(10,("%s: cue_csr_read_1 reg=%#x val=%#x\n",
	    device_xname(un->un_dev), reg, val));

	return val;
}
#endif

static int
cue_csr_read_2(struct usbnet *un, int reg)
{
	usb_device_request_t	req;
	usbd_status		err;
	uWord			val;

	if (usbnet_isdying(un))
		return 0;

	req.bmRequestType = UT_READ_VENDOR_DEVICE;
	req.bRequest = CUE_CMD_READREG;
	USETW(req.wValue, 0);
	USETW(req.wIndex, reg);
	USETW(req.wLength, 2);

	err = usbd_do_request(un->un_udev, &req, &val);

	DPRINTFN(10,("%s: cue_csr_read_2 reg=%#x val=%#x\n",
	    device_xname(un->un_dev), reg, UGETW(val)));

	if (err) {
		DPRINTF(("%s: cue_csr_read_2: reg=%#x err=%s\n",
		    device_xname(un->un_dev), reg, usbd_errstr(err)));
		return 0;
	}

	return UGETW(val);
}

static int
cue_csr_write_1(struct usbnet *un, int reg, int val)
{
	usb_device_request_t	req;
	usbd_status		err;

	if (usbnet_isdying(un))
		return 0;

	DPRINTFN(10,("%s: cue_csr_write_1 reg=%#x val=%#x\n",
	    device_xname(un->un_dev), reg, val));

	req.bmRequestType = UT_WRITE_VENDOR_DEVICE;
	req.bRequest = CUE_CMD_WRITEREG;
	USETW(req.wValue, val);
	USETW(req.wIndex, reg);
	USETW(req.wLength, 0);

	err = usbd_do_request(un->un_udev, &req, NULL);

	if (err) {
		DPRINTF(("%s: cue_csr_write_1: reg=%#x err=%s\n",
		    device_xname(un->un_dev), reg, usbd_errstr(err)));
		return -1;
	}

	DPRINTFN(20,("%s: cue_csr_write_1, after reg=%#x val=%#x\n",
	    device_xname(un->un_dev), reg, cue_csr_read_1(un, reg)));

	return 0;
}

#if 0
static int
cue_csr_write_2(struct usbnet *un, int reg, int aval)
{
	usb_device_request_t	req;
	usbd_status		err;
	uWord			val;
	int			s;

	if (usbnet_isdying(un))
		return 0;

	DPRINTFN(10,("%s: cue_csr_write_2 reg=%#x val=%#x\n",
	    device_xname(un->un_dev), reg, aval));

	USETW(val, aval);
	req.bmRequestType = UT_WRITE_VENDOR_DEVICE;
	req.bRequest = CUE_CMD_WRITEREG;
	USETW(req.wValue, val);
	USETW(req.wIndex, reg);
	USETW(req.wLength, 0);

	err = usbd_do_request(un->un_udev, &req, NULL);

	if (err) {
		DPRINTF(("%s: cue_csr_write_2: reg=%#x err=%s\n",
		    device_xname(un->un_dev), reg, usbd_errstr(err)));
		return -1;
	}

	return 0;
}
#endif

static int
cue_mem(struct usbnet *un, int cmd, int addr, void *buf, int len)
{
	usb_device_request_t	req;
	usbd_status		err;

	DPRINTFN(10,("%s: cue_mem cmd=%#x addr=%#x len=%d\n",
	    device_xname(un->un_dev), cmd, addr, len));

	if (cmd == CUE_CMD_READSRAM)
		req.bmRequestType = UT_READ_VENDOR_DEVICE;
	else
		req.bmRequestType = UT_WRITE_VENDOR_DEVICE;
	req.bRequest = cmd;
	USETW(req.wValue, 0);
	USETW(req.wIndex, addr);
	USETW(req.wLength, len);

	err = usbd_do_request(un->un_udev, &req, buf);

	if (err) {
		DPRINTF(("%s: cue_csr_mem: addr=%#x err=%s\n",
		    device_xname(un->un_dev), addr, usbd_errstr(err)));
		return -1;
	}

	return 0;
}

static int
cue_getmac(struct usbnet *un)
{
	usb_device_request_t	req;
	usbd_status		err;

	DPRINTFN(10,("%s: cue_getmac\n", device_xname(un->un_dev)));

	req.bmRequestType = UT_READ_VENDOR_DEVICE;
	req.bRequest = CUE_CMD_GET_MACADDR;
	USETW(req.wValue, 0);
	USETW(req.wIndex, 0);
	USETW(req.wLength, ETHER_ADDR_LEN);

	err = usbd_do_request(un->un_udev, &req, un->un_eaddr);

	if (err) {
		printf("%s: read MAC address failed\n",
		    device_xname(un->un_dev));
		return -1;
	}

	return 0;
}

#define CUE_POLY	0xEDB88320
#define CUE_BITS	9

static uint32_t
cue_crc(const char *addr)
{
	uint32_t		idx, bit, data, crc;

	/* Compute CRC for the address value. */
	crc = 0xFFFFFFFF; /* initial value */

	for (idx = 0; idx < 6; idx++) {
		for (data = *addr++, bit = 0; bit < 8; bit++, data >>= 1)
			crc = (crc >> 1) ^ (((crc ^ data) & 1) ? CUE_POLY : 0);
	}

	return crc & ((1 << CUE_BITS) - 1);
}

static void
cue_uno_mcast(struct ifnet *ifp)
{
	struct usbnet		*un = ifp->if_softc;
	struct cue_softc	*sc = usbnet_softc(un);
	struct ethercom		*ec = usbnet_ec(un);
	struct ether_multi	*enm;
	struct ether_multistep	step;
	uint32_t		h, i;

	DPRINTFN(2,("%s: cue_setiff if_flags=%#x\n",
	    device_xname(un->un_dev), ifp->if_flags));

	if (ifp->if_flags & IFF_PROMISC) {
		ETHER_LOCK(ec);
allmulti:
		ec->ec_flags |= ETHER_F_ALLMULTI;
		ETHER_UNLOCK(ec);
		for (i = 0; i < CUE_MCAST_TABLE_LEN; i++)
			sc->cue_mctab[i] = 0xFF;
		cue_mem(un, CUE_CMD_WRITESRAM, CUE_MCAST_TABLE_ADDR,
		    &sc->cue_mctab, CUE_MCAST_TABLE_LEN);
		return;
	}

	/* first, zot all the existing hash bits */
	for (i = 0; i < CUE_MCAST_TABLE_LEN; i++)
		sc->cue_mctab[i] = 0;

	/* now program new ones */
	ETHER_LOCK(ec);
	ETHER_FIRST_MULTI(step, ec, enm);
	while (enm != NULL) {
		if (memcmp(enm->enm_addrlo,
		    enm->enm_addrhi, ETHER_ADDR_LEN) != 0) {
			goto allmulti;
		}

		h = cue_crc(enm->enm_addrlo);
		sc->cue_mctab[h >> 3] |= 1 << (h & 0x7);
		ETHER_NEXT_MULTI(step, enm);
	}
	ec->ec_flags &= ~ETHER_F_ALLMULTI;
	ETHER_UNLOCK(ec);

	/*
	 * Also include the broadcast address in the filter
	 * so we can receive broadcast frames.
	 */
	if (ifp->if_flags & IFF_BROADCAST) {
		h = cue_crc(etherbroadcastaddr);
		sc->cue_mctab[h >> 3] |= 1 << (h & 0x7);
	}

	cue_mem(un, CUE_CMD_WRITESRAM, CUE_MCAST_TABLE_ADDR,
	    &sc->cue_mctab, CUE_MCAST_TABLE_LEN);
}

static void
cue_reset(struct usbnet *un)
{
	usb_device_request_t	req;
	usbd_status		err;

	DPRINTFN(2,("%s: cue_reset\n", device_xname(un->un_dev)));

	if (usbnet_isdying(un))
		return;

	req.bmRequestType = UT_WRITE_VENDOR_DEVICE;
	req.bRequest = CUE_CMD_RESET;
	USETW(req.wValue, 0);
	USETW(req.wIndex, 0);
	USETW(req.wLength, 0);

	err = usbd_do_request(un->un_udev, &req, NULL);

	if (err)
		printf("%s: reset failed\n", device_xname(un->un_dev));

	/* Wait a little while for the chip to get its brains in order. */
	usbd_delay_ms(un->un_udev, 1);
}

/*
 * Probe for a CATC chip.
 */
static int
cue_match(device_t parent, cfdata_t match, void *aux)
{
	struct usb_attach_arg *uaa = aux;

	return cue_lookup(uaa->uaa_vendor, uaa->uaa_product) != NULL ?
		UMATCH_VENDOR_PRODUCT : UMATCH_NONE;
}

/*
 * Attach the interface. Allocate softc structures, do ifmedia
 * setup and ethernet/BPF attach.
 */
static void
cue_attach(device_t parent, device_t self, void *aux)
{
	struct cue_softc *sc = device_private(self);
	struct usbnet * const un = &sc->cue_un;
	struct usb_attach_arg *uaa = aux;
	char			*devinfop;
	struct usbd_device *	dev = uaa->uaa_device;
	usbd_status		err;
	usb_interface_descriptor_t	*id;
	usb_endpoint_descriptor_t	*ed;
	int			i;

	KASSERT((void *)sc == un);

	DPRINTFN(5,(" : cue_attach: sc=%p, dev=%p", sc, dev));

	aprint_naive("\n");
	aprint_normal("\n");
	devinfop = usbd_devinfo_alloc(dev, 0);
	aprint_normal_dev(self, "%s\n", devinfop);
	usbd_devinfo_free(devinfop);

	err = usbd_set_config_no(dev, CUE_CONFIG_NO, 1);
	if (err) {
		aprint_error_dev(self, "failed to set configuration"
		    ", err=%s\n", usbd_errstr(err));
		return;
	}

	un->un_dev = self;
	un->un_udev = dev;
	un->un_sc = sc;
	un->un_ops = &cue_ops;
	un->un_rx_xfer_flags = USBD_SHORT_XFER_OK;
	un->un_tx_xfer_flags = USBD_FORCE_SHORT_XFER;
	un->un_rx_list_cnt = CUE_RX_LIST_CNT;
	un->un_tx_list_cnt = CUE_TX_LIST_CNT;
	un->un_rx_bufsz = CUE_BUFSZ;
	un->un_tx_bufsz = CUE_BUFSZ;

	err = usbd_device2interface_handle(dev, CUE_IFACE_IDX, &un->un_iface);
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
			un->un_ed[USBNET_ENDPT_INTR] = ed->bEndpointAddress;
		}
	}

	/* First level attach. */
	usbnet_attach(un, "cuedet");

#if 0
	/* Reset the adapter. */
	cue_reset(un);
#endif
	/*
	 * Get station address.
	 */
	cue_getmac(un);

	usbnet_attach_ifp(un, IFF_SIMPLEX | IFF_BROADCAST | IFF_MULTICAST,
	    0, NULL);
}

static void
cue_uno_tick(struct usbnet *un)
{
	struct ifnet		*ifp = usbnet_ifp(un);

	net_stat_ref_t nsr = IF_STAT_GETREF(ifp);
	if (cue_csr_read_2(un, CUE_RX_FRAMEERR))
		if_statinc_ref(nsr, if_ierrors);

	if_statadd_ref(nsr, if_collisions,
	    cue_csr_read_2(un, CUE_TX_SINGLECOLL));
	if_statadd_ref(nsr, if_collisions,
	    cue_csr_read_2(un, CUE_TX_MULTICOLL));
	if_statadd_ref(nsr, if_collisions,
	    cue_csr_read_2(un, CUE_TX_EXCESSCOLL));
	IF_STAT_PUTREF(ifp);
}

static void
cue_uno_rx_loop(struct usbnet *un, struct usbnet_chain *c, uint32_t total_len)
{
	struct ifnet		*ifp = usbnet_ifp(un);
	uint8_t			*buf = c->unc_buf;
	uint16_t		len;

	DPRINTFN(5,("%s: %s: total_len=%d len=%d\n",
		     device_xname(un->un_dev), __func__,
		     total_len, le16dec(buf)));

	len = UGETW(buf);
	if (total_len < 2 ||
	    len > total_len - 2 ||
	    len < sizeof(struct ether_header)) {
		if_statinc(ifp, if_ierrors);
		return;
	}

	/* No errors; receive the packet. */
	usbnet_enqueue(un, buf + 2, len, 0, 0, 0);
}

static unsigned
cue_uno_tx_prepare(struct usbnet *un, struct mbuf *m, struct usbnet_chain *c)
{
	unsigned		total_len;

	DPRINTFN(5,("%s: %s: mbuf len=%d\n",
		     device_xname(un->un_dev), __func__,
		     m->m_pkthdr.len));

	if ((unsigned)m->m_pkthdr.len > un->un_tx_bufsz - 2)
		return 0;

	/*
	 * Copy the mbuf data into a contiguous buffer, leaving two
	 * bytes at the beginning to hold the frame length.
	 */
	m_copydata(m, 0, m->m_pkthdr.len, c->unc_buf + 2);

	total_len = m->m_pkthdr.len + 2;

	/* The first two bytes are the frame length */
	c->unc_buf[0] = (uint8_t)m->m_pkthdr.len;
	c->unc_buf[1] = (uint8_t)(m->m_pkthdr.len >> 8);

	return total_len;
}

static int
cue_uno_init(struct ifnet *ifp)
{
	struct usbnet * const	un = ifp->if_softc;
	int			i, ctl;
	const u_char		*eaddr;

	DPRINTFN(10,("%s: %s: enter\n", device_xname(un->un_dev),__func__));

	/* Cancel pending I/O */
	cue_uno_stop(ifp, 1);

	/* Reset the interface. */
#if 1
	cue_reset(un);
#endif

	/* Set advanced operation modes. */
	cue_csr_write_1(un, CUE_ADVANCED_OPMODES,
	    CUE_AOP_EMBED_RXLEN | 0x03); /* 1 wait state */

	eaddr = CLLADDR(ifp->if_sadl);
	/* Set MAC address */
	for (i = 0; i < ETHER_ADDR_LEN; i++)
		cue_csr_write_1(un, CUE_PAR0 - i, eaddr[i]);

	/* Enable RX logic. */
	ctl = CUE_ETHCTL_RX_ON | CUE_ETHCTL_MCAST_ON;
	if (ifp->if_flags & IFF_PROMISC)
		ctl |= CUE_ETHCTL_PROMISC;
	cue_csr_write_1(un, CUE_ETHCTL, ctl);

	/*
	 * Set the number of RX and TX buffers that we want
	 * to reserve inside the ASIC.
	 */
	cue_csr_write_1(un, CUE_RX_BUFPKTS, CUE_RX_FRAMES);
	cue_csr_write_1(un, CUE_TX_BUFPKTS, CUE_TX_FRAMES);

	/* Set advanced operation modes. */
	cue_csr_write_1(un, CUE_ADVANCED_OPMODES,
	    CUE_AOP_EMBED_RXLEN | 0x01); /* 1 wait state */

	/* Program the LED operation. */
	cue_csr_write_1(un, CUE_LEDCTL, CUE_LEDCTL_FOLLOW_LINK);

	return usbnet_init_rx_tx(un);
}

/* Stop and reset the adapter.  */
static void
cue_uno_stop(struct ifnet *ifp, int disable)
{
	struct usbnet * const	un = ifp->if_softc;

	DPRINTFN(10,("%s: %s: enter\n", device_xname(un->un_dev), __func__));

	cue_csr_write_1(un, CUE_ETHCTL, 0);
	cue_reset(un);
}

#ifdef _MODULE
#include "ioconf.c"
#endif

USBNET_MODULE(cue)
