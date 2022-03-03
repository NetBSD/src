/*	$NetBSD: if_url.c,v 1.79 2022/03/03 05:50:22 riastradh Exp $	*/

/*
 * Copyright (c) 2001, 2002
 *     Shingo WATANABE <nabe@nabechan.org>.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/*
 * The RTL8150L(Realtek USB to fast ethernet controller) spec can be found at
 *   ftp://ftp.realtek.com.tw/lancard/data_sheet/8150/8150v14.pdf
 *   ftp://152.104.125.40/lancard/data_sheet/8150/8150v14.pdf
 */

/*
 * TODO:
 *	Interrupt Endpoint support
 *	External PHYs
 *	powerhook() support?
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_url.c,v 1.79 2022/03/03 05:50:22 riastradh Exp $");

#ifdef _KERNEL_OPT
#include "opt_inet.h"
#include "opt_usb.h"
#endif

#include <sys/param.h>

#include <net/if_ether.h>
#ifdef INET
#include <netinet/in.h>
#include <netinet/if_inarp.h>
#endif

#include <dev/mii/urlphyreg.h>

#include <dev/usb/usbnet.h>

#include <dev/usb/if_urlreg.h>

/* Function declarations */
static int	url_match(device_t, cfdata_t, void *);
static void	url_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(url, sizeof(struct usbnet), url_match, url_attach,
    usbnet_detach, usbnet_activate);

static unsigned	url_uno_tx_prepare(struct usbnet *, struct mbuf *,
				   struct usbnet_chain *);
static void url_uno_rx_loop(struct usbnet *, struct usbnet_chain *, uint32_t);
static int url_uno_mii_read_reg(struct usbnet *, int, int, uint16_t *);
static int url_uno_mii_write_reg(struct usbnet *, int, int, uint16_t);
static int url_uno_ioctl(struct ifnet *, u_long, void *);
static void url_uno_stop(struct ifnet *, int);
static void url_uno_mii_statchg(struct ifnet *);
static int url_uno_init(struct ifnet *);
static void url_rcvfilt_locked(struct usbnet *);
static void url_reset(struct usbnet *);

static int url_csr_read_1(struct usbnet *, int);
static int url_csr_read_2(struct usbnet *, int);
static int url_csr_write_1(struct usbnet *, int, int);
static int url_csr_write_2(struct usbnet *, int, int);
static int url_csr_write_4(struct usbnet *, int, int);
static int url_mem(struct usbnet *, int, int, void *, int);

static const struct usbnet_ops url_ops = {
	.uno_stop = url_uno_stop,
	.uno_ioctl = url_uno_ioctl,
	.uno_read_reg = url_uno_mii_read_reg,
	.uno_write_reg = url_uno_mii_write_reg,
	.uno_statchg = url_uno_mii_statchg,
	.uno_tx_prepare = url_uno_tx_prepare,
	.uno_rx_loop = url_uno_rx_loop,
	.uno_init = url_uno_init,
};

/* Macros */
#ifdef URL_DEBUG
#define DPRINTF(x)	if (urldebug) printf x
#define DPRINTFN(n, x)	if (urldebug >= (n)) printf x
int urldebug = 0;
#else
#define DPRINTF(x)
#define DPRINTFN(n, x)
#endif

#define	URL_SETBIT(un, reg, x)	\
	url_csr_write_1(un, reg, url_csr_read_1(un, reg) | (x))

#define	URL_SETBIT2(un, reg, x)	\
	url_csr_write_2(un, reg, url_csr_read_2(un, reg) | (x))

#define	URL_CLRBIT(un, reg, x)	\
	url_csr_write_1(un, reg, url_csr_read_1(un, reg) & ~(x))

#define	URL_CLRBIT2(un, reg, x)	\
	url_csr_write_2(un, reg, url_csr_read_2(un, reg) & ~(x))

static const struct url_type {
	struct usb_devno url_dev;
	uint16_t url_flags;
#define URL_EXT_PHY	0x0001
} url_devs [] = {
	/* MELCO LUA-KTX */
	{{ USB_VENDOR_MELCO, USB_PRODUCT_MELCO_LUAKTX }, 0},
	/* Realtek RTL8150L Generic (GREEN HOUSE USBKR100) */
	{{ USB_VENDOR_REALTEK, USB_PRODUCT_REALTEK_RTL8150L}, 0},
	/* Longshine LCS-8138TX */
	{{ USB_VENDOR_ABOCOM, USB_PRODUCT_ABOCOM_LCS8138TX}, 0},
	/* Micronet SP128AR */
	{{ USB_VENDOR_MICRONET, USB_PRODUCT_MICRONET_SP128AR}, 0},
	/* OQO model 01 */
	{{ USB_VENDOR_OQO, USB_PRODUCT_OQO_ETHER01}, 0},
};
#define url_lookup(v, p) ((const struct url_type *)usb_lookup(url_devs, v, p))


/* Probe */
static int
url_match(device_t parent, cfdata_t match, void *aux)
{
	struct usb_attach_arg *uaa = aux;

	return url_lookup(uaa->uaa_vendor, uaa->uaa_product) != NULL ?
		UMATCH_VENDOR_PRODUCT : UMATCH_NONE;
}
/* Attach */
static void
url_attach(device_t parent, device_t self, void *aux)
{
	USBNET_MII_DECL_DEFAULT(unm);
	struct usbnet * const un = device_private(self);
	struct usb_attach_arg *uaa = aux;
	struct usbd_device *dev = uaa->uaa_device;
	struct usbd_interface *iface;
	usbd_status err;
	usb_interface_descriptor_t *id;
	usb_endpoint_descriptor_t *ed;
	char *devinfop;
	int i;

	aprint_naive("\n");
	aprint_normal("\n");
	devinfop = usbd_devinfo_alloc(dev, 0);
	aprint_normal_dev(self, "%s\n", devinfop);
	usbd_devinfo_free(devinfop);

	un->un_dev = self;
	un->un_udev = dev;
	un->un_sc = un;
	un->un_ops = &url_ops;
	un->un_rx_xfer_flags = USBD_SHORT_XFER_OK;
	un->un_tx_xfer_flags = USBD_FORCE_SHORT_XFER;
	un->un_rx_list_cnt = URL_RX_LIST_CNT;
	un->un_tx_list_cnt = URL_TX_LIST_CNT;
	un->un_rx_bufsz = URL_BUFSZ;
	un->un_tx_bufsz = URL_BUFSZ;

	/* Move the device into the configured state. */
	err = usbd_set_config_no(dev, URL_CONFIG_NO, 1);
	if (err) {
		aprint_error_dev(self, "failed to set configuration"
		    ", err=%s\n", usbd_errstr(err));
		return;
	}

	/* get control interface */
	err = usbd_device2interface_handle(dev, URL_IFACE_INDEX, &iface);
	if (err) {
		aprint_error_dev(self, "failed to get interface, err=%s\n",
		       usbd_errstr(err));
		return;
	}

	un->un_iface = iface;
	un->un_flags = url_lookup(uaa->uaa_vendor, uaa->uaa_product)->url_flags;
#if 0
	if (un->un_flags & URL_EXT_PHY) {
		un->un_read_reg_cb = url_ext_mii_read_reg;
		un->un_write_reg_cb = url_ext_mii_write_reg;
	}
#endif

	/* get interface descriptor */
	id = usbd_get_interface_descriptor(un->un_iface);

	/* find endpoints */
	un->un_ed[USBNET_ENDPT_RX] = un->un_ed[USBNET_ENDPT_TX] =
	    un->un_ed[USBNET_ENDPT_INTR] = 0;
	for (i = 0; i < id->bNumEndpoints; i++) {
		ed = usbd_interface2endpoint_descriptor(un->un_iface, i);
		if (ed == NULL) {
			aprint_error_dev(self,
			    "couldn't get endpoint %d\n", i);
			return;
		}
		if ((ed->bmAttributes & UE_XFERTYPE) == UE_BULK &&
		    UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_IN)
			un->un_ed[USBNET_ENDPT_RX] = ed->bEndpointAddress;
		else if ((ed->bmAttributes & UE_XFERTYPE) == UE_BULK &&
			 UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_OUT)
			un->un_ed[USBNET_ENDPT_TX] = ed->bEndpointAddress;
		else if ((ed->bmAttributes & UE_XFERTYPE) == UE_INTERRUPT &&
			 UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_IN)
			un->un_ed[USBNET_ENDPT_INTR] = ed->bEndpointAddress;
	}

	if (un->un_ed[USBNET_ENDPT_RX] == 0 ||
	    un->un_ed[USBNET_ENDPT_TX] == 0 ||
	    un->un_ed[USBNET_ENDPT_INTR] == 0) {
		aprint_error_dev(self, "missing endpoint\n");
		return;
	}

	/* Set these up now for url_mem().  */
	usbnet_attach(un, "urldet");

	usbnet_lock_core(un);
	usbnet_busy(un);

	/* reset the adapter */
	url_reset(un);

	/* Get Ethernet Address */
	err = url_mem(un, URL_CMD_READMEM, URL_IDR0, (void *)un->un_eaddr,
		      ETHER_ADDR_LEN);
	usbnet_unbusy(un);
	usbnet_unlock_core(un);
	if (err) {
		aprint_error_dev(self, "read MAC address failed\n");
		return;
	}

	/* initialize interface information */
	usbnet_attach_ifp(un, IFF_SIMPLEX | IFF_BROADCAST | IFF_MULTICAST,
	    0, &unm);
}

/* read/write memory */
static int
url_mem(struct usbnet *un, int cmd, int offset, void *buf, int len)
{
	usb_device_request_t req;
	usbd_status err;

	usbnet_isowned_core(un);

	DPRINTFN(0x200,
		("%s: %s: enter\n", device_xname(un->un_dev), __func__));

	if (usbnet_isdying(un))
		return 0;

	if (cmd == URL_CMD_READMEM)
		req.bmRequestType = UT_READ_VENDOR_DEVICE;
	else
		req.bmRequestType = UT_WRITE_VENDOR_DEVICE;
	req.bRequest = URL_REQ_MEM;
	USETW(req.wValue, offset);
	USETW(req.wIndex, 0x0000);
	USETW(req.wLength, len);

	err = usbd_do_request(un->un_udev, &req, buf);
	if (err) {
		DPRINTF(("%s: url_mem(): %s failed. off=%04x, err=%d\n",
			 device_xname(un->un_dev),
			 cmd == URL_CMD_READMEM ? "read" : "write",
			 offset, err));
	}

	return err;
}

/* read 1byte from register */
static int
url_csr_read_1(struct usbnet *un, int reg)
{
	uint8_t val = 0;

	DPRINTFN(0x100,
		 ("%s: %s: enter\n", device_xname(un->un_dev), __func__));

	return url_mem(un, URL_CMD_READMEM, reg, &val, 1) ? 0 : val;
}

/* read 2bytes from register */
static int
url_csr_read_2(struct usbnet *un, int reg)
{
	uWord val;

	DPRINTFN(0x100,
		 ("%s: %s: enter\n", device_xname(un->un_dev), __func__));

	USETW(val, 0);
	return url_mem(un, URL_CMD_READMEM, reg, &val, 2) ? 0 : UGETW(val);
}

/* write 1byte to register */
static int
url_csr_write_1(struct usbnet *un, int reg, int aval)
{
	uint8_t val = aval;

	DPRINTFN(0x100,
		 ("%s: %s: enter\n", device_xname(un->un_dev), __func__));

	return url_mem(un, URL_CMD_WRITEMEM, reg, &val, 1) ? -1 : 0;
}

/* write 2bytes to register */
static int
url_csr_write_2(struct usbnet *un, int reg, int aval)
{
	uWord val;

	DPRINTFN(0x100,
		 ("%s: %s: enter\n", device_xname(un->un_dev), __func__));

	USETW(val, aval);

	return url_mem(un, URL_CMD_WRITEMEM, reg, &val, 2) ? -1 : 0;
}

/* write 4bytes to register */
static int
url_csr_write_4(struct usbnet *un, int reg, int aval)
{
	uDWord val;

	DPRINTFN(0x100,
		 ("%s: %s: enter\n", device_xname(un->un_dev), __func__));

	USETDW(val, aval);

	return url_mem(un, URL_CMD_WRITEMEM, reg, &val, 4) ? -1 : 0;
}

static int
url_init_locked(struct ifnet *ifp)
{
	struct usbnet * const un = ifp->if_softc;
	const u_char *eaddr;
	int i;

	DPRINTF(("%s: %s: enter\n", device_xname(un->un_dev), __func__));

	usbnet_isowned_core(un);

	if (usbnet_isdying(un))
		return EIO;

	/* Cancel pending I/O and free all TX/RX buffers */
	usbnet_stop(un, ifp, 1);

	eaddr = CLLADDR(ifp->if_sadl);
	for (i = 0; i < ETHER_ADDR_LEN; i++)
		url_csr_write_1(un, URL_IDR0 + i, eaddr[i]);

	/* Init transmission control register */
	URL_CLRBIT(un, URL_TCR,
		   URL_TCR_TXRR1 | URL_TCR_TXRR0 |
		   URL_TCR_IFG1 | URL_TCR_IFG0 |
		   URL_TCR_NOCRC);

	/* Init receive control register */
	URL_SETBIT2(un, URL_RCR, URL_RCR_TAIL | URL_RCR_AD | URL_RCR_AB);

	/* Accept multicast frame or run promisc. mode */
	url_rcvfilt_locked(un);

	/* Enable RX and TX */
	URL_SETBIT(un, URL_CR, URL_CR_TE | URL_CR_RE);

	return usbnet_init_rx_tx(un);
}

static int
url_uno_init(struct ifnet *ifp)
{
	struct usbnet * const un = ifp->if_softc;

	usbnet_busy(un);
	int ret = url_init_locked(ifp);
	usbnet_unbusy(un);

	return ret;
}

static void
url_reset(struct usbnet *un)
{
	int i;

	DPRINTF(("%s: %s: enter\n", device_xname(un->un_dev), __func__));

	if (usbnet_isdying(un))
		return;

	URL_SETBIT(un, URL_CR, URL_CR_SOFT_RST);

	for (i = 0; i < URL_TX_TIMEOUT; i++) {
		if (!(url_csr_read_1(un, URL_CR) & URL_CR_SOFT_RST))
			break;
		delay(10);	/* XXX */
	}

	delay(10000);		/* XXX */
}

static void
url_rcvfilt_locked(struct usbnet *un)
{
	struct ifnet * const ifp = usbnet_ifp(un);
	struct ethercom *ec = usbnet_ec(un);
	struct ether_multi *enm;
	struct ether_multistep step;
	uint32_t mchash[2] = { 0, 0 };
	int h = 0, rcr;

	DPRINTF(("%s: %s: enter\n", device_xname(un->un_dev), __func__));

	usbnet_isowned_core(un);

	if (usbnet_isdying(un))
		return;

	rcr = url_csr_read_2(un, URL_RCR);
	rcr &= ~(URL_RCR_AAP | URL_RCR_AAM | URL_RCR_AM);

	ETHER_LOCK(ec);
	if (ifp->if_flags & IFF_PROMISC) {
		ec->ec_flags |= ETHER_F_ALLMULTI;
		ETHER_UNLOCK(ec);
		/* run promisc. mode */
		rcr |= URL_RCR_AAM; /* ??? */
		rcr |= URL_RCR_AAP;
		goto update;
	}
	ec->ec_flags &= ~ETHER_F_ALLMULTI;
	ETHER_FIRST_MULTI(step, ec, enm);
	while (enm != NULL) {
		if (memcmp(enm->enm_addrlo, enm->enm_addrhi, ETHER_ADDR_LEN)) {
			ec->ec_flags |= ETHER_F_ALLMULTI;
			ETHER_UNLOCK(ec);
			/* accept all multicast frames */
			rcr |= URL_RCR_AAM;
			goto update;
		}
		h = ether_crc32_be(enm->enm_addrlo, ETHER_ADDR_LEN);
		/* 1(31) and 5(30:26) bit sampling */
		mchash[h >> 31] |= 1 << ((h >> 26) & 0x1f);
		ETHER_NEXT_MULTI(step, enm);
	}
	ETHER_UNLOCK(ec);
	if (h != 0)
		rcr |= URL_RCR_AM;	/* activate mcast hash filter */
	url_csr_write_4(un, URL_MAR0, mchash[0]);
	url_csr_write_4(un, URL_MAR4, mchash[1]);
 update:
	url_csr_write_2(un, URL_RCR, rcr);
}

static unsigned
url_uno_tx_prepare(struct usbnet *un, struct mbuf *m, struct usbnet_chain *c)
{
	int total_len;

	DPRINTF(("%s: %s: enter\n", device_xname(un->un_dev),__func__));

	KASSERT(un->un_tx_bufsz >= URL_MIN_FRAME_LEN);
	if ((unsigned)m->m_pkthdr.len > un->un_tx_bufsz)
		return 0;

	/* Copy the mbuf data into a contiguous buffer */
	m_copydata(m, 0, m->m_pkthdr.len, c->unc_buf);
	total_len = m->m_pkthdr.len;

	if (total_len < URL_MIN_FRAME_LEN) {
		memset(c->unc_buf + total_len, 0,
		    URL_MIN_FRAME_LEN - total_len);
		total_len = URL_MIN_FRAME_LEN;
	}

	DPRINTF(("%s: %s: send %d bytes\n", device_xname(un->un_dev),
		 __func__, total_len));

	return total_len;
}

static void
url_uno_rx_loop(struct usbnet *un, struct usbnet_chain *c, uint32_t total_len)
{
	struct ifnet *ifp = usbnet_ifp(un);
	url_rxhdr_t rxhdr;

	DPRINTF(("%s: %s: enter\n", device_xname(un->un_dev),__func__));

	if (total_len <= ETHER_CRC_LEN || total_len <= sizeof(rxhdr)) {
		if_statinc(ifp, if_ierrors);
		return;
	}

	memcpy(&rxhdr, c->unc_buf + total_len - ETHER_CRC_LEN, sizeof(rxhdr));

	DPRINTF(("%s: RX Status: %dbytes%s%s%s%s packets\n",
		 device_xname(un->un_dev),
		 UGETW(rxhdr) & URL_RXHDR_BYTEC_MASK,
		 UGETW(rxhdr) & URL_RXHDR_VALID_MASK ? ", Valid" : "",
		 UGETW(rxhdr) & URL_RXHDR_RUNTPKT_MASK ? ", Runt" : "",
		 UGETW(rxhdr) & URL_RXHDR_PHYPKT_MASK ? ", Physical match" : "",
		 UGETW(rxhdr) & URL_RXHDR_MCASTPKT_MASK ? ", Multicast" : ""));

	if ((UGETW(rxhdr) & URL_RXHDR_VALID_MASK) == 0) {
		if_statinc(ifp, if_ierrors);
		return;
	}

	total_len -= ETHER_CRC_LEN;

	DPRINTF(("%s: %s: deliver %d\n", device_xname(un->un_dev),
		 __func__, total_len));
	usbnet_enqueue(un, c->unc_buf, total_len, 0, 0, 0);
}

#if 0
static void url_intr(void)
{
}
#endif

static int
url_uno_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	struct usbnet * const un = ifp->if_softc;

	usbnet_lock_core(un);
	usbnet_busy(un);

	switch (cmd) {
	case SIOCADDMULTI:
	case SIOCDELMULTI:
		url_rcvfilt_locked(un);
		break;
	default:
		break;
	}

	usbnet_unbusy(un);
	usbnet_unlock_core(un);

	return 0;
}

/* Stop the adapter and free any mbufs allocated to the RX and TX lists. */
static void
url_uno_stop(struct ifnet *ifp, int disable)
{
	struct usbnet * const un = ifp->if_softc;

	DPRINTF(("%s: %s: enter\n", device_xname(un->un_dev), __func__));

	url_reset(un);
}

static int
url_uno_mii_read_reg(struct usbnet *un, int phy, int reg, uint16_t *val)
{
	uint16_t data;
	usbd_status err = USBD_NORMAL_COMPLETION;

	DPRINTFN(0xff, ("%s: %s: enter, phy=%d reg=0x%04x\n",
		 device_xname(un->un_dev), __func__, phy, reg));

	/* XXX: one PHY only for the RTL8150 internal PHY */
	if (phy != 0) {
		DPRINTFN(0xff, ("%s: %s: phy=%d is not supported\n",
			 device_xname(un->un_dev), __func__, phy));
		return EINVAL;
	}

	switch (reg) {
	case MII_BMCR:		/* Control Register */
		reg = URL_BMCR;
		break;
	case MII_BMSR:		/* Status Register */
		reg = URL_BMSR;
		break;
	case MII_PHYIDR1:
	case MII_PHYIDR2:
		*val = 0;
		goto R_DONE;
		break;
	case MII_ANAR:		/* Autonegotiation advertisement */
		reg = URL_ANAR;
		break;
	case MII_ANLPAR:	/* Autonegotiation link partner abilities */
		reg = URL_ANLP;
		break;
	case URLPHY_MSR:	/* Media Status Register */
		reg = URL_MSR;
		break;
	default:
		printf("%s: %s: bad register %04x\n",
		       device_xname(un->un_dev), __func__, reg);
		return EINVAL;
	}

	if (reg == URL_MSR)
		data = url_csr_read_1(un, reg);
	else
		data = url_csr_read_2(un, reg);
	*val = data;

 R_DONE:
	DPRINTFN(0xff, ("%s: %s: phy=%d reg=0x%04x => 0x%04hx\n",
		 device_xname(un->un_dev), __func__, phy, reg, *val));

	return err;
}

static int
url_uno_mii_write_reg(struct usbnet *un, int phy, int reg, uint16_t val)
{

	DPRINTFN(0xff, ("%s: %s: enter, phy=%d reg=0x%04x val=0x%04hx\n",
		 device_xname(un->un_dev), __func__, phy, reg, val));

	/* XXX: one PHY only for the RTL8150 internal PHY */
	if (phy != 0) {
		DPRINTFN(0xff, ("%s: %s: phy=%d is not supported\n",
			 device_xname(un->un_dev), __func__, phy));
		return EINVAL;
	}

	switch (reg) {
	case MII_BMCR:		/* Control Register */
		reg = URL_BMCR;
		break;
	case MII_BMSR:		/* Status Register */
		reg = URL_BMSR;
		break;
	case MII_PHYIDR1:
	case MII_PHYIDR2:
		return 0;
	case MII_ANAR:		/* Autonegotiation advertisement */
		reg = URL_ANAR;
		break;
	case MII_ANLPAR:	/* Autonegotiation link partner abilities */
		reg = URL_ANLP;
		break;
	case URLPHY_MSR:	/* Media Status Register */
		reg = URL_MSR;
		break;
	default:
		printf("%s: %s: bad register %04x\n",
		       device_xname(un->un_dev), __func__, reg);
		return EINVAL;
	}

	if (reg == URL_MSR)
		url_csr_write_1(un, reg, val);
	else
		url_csr_write_2(un, reg, val);

	return 0;
}

static void
url_uno_mii_statchg(struct ifnet *ifp)
{
	struct usbnet * const un = ifp->if_softc;

	DPRINTF(("%s: %s: enter\n", ifp->if_xname, __func__));

	/* XXX */
	usbnet_set_link(un, true);
}

#if 0
/*
 * external PHYs support, but not test.
 */
static usbd_status
url_ext_mii_read_reg(struct usbnet *un, int phy, int reg)
{
	uint16_t val;

	DPRINTF(("%s: %s: enter, phy=%d reg=0x%04x\n",
		 device_xname(un->un_dev), __func__, phy, reg));

	url_csr_write_1(un, URL_PHYADD, phy & URL_PHYADD_MASK);
	/*
	 * RTL8150L will initiate a MII management data transaction
	 * if PHYCNT_OWN bit is set 1 by software. After transaction,
	 * this bit is auto cleared by TRL8150L.
	 */
	url_csr_write_1(un, URL_PHYCNT,
			(reg | URL_PHYCNT_PHYOWN) & ~URL_PHYCNT_RWCR);
	for (i = 0; i < URL_TIMEOUT; i++) {
		if ((url_csr_read_1(un, URL_PHYCNT) & URL_PHYCNT_PHYOWN) == 0)
			break;
	}
	if (i == URL_TIMEOUT) {
		printf("%s: MII read timed out\n", device_xname(un->un_dev));
	}

	val = url_csr_read_2(un, URL_PHYDAT);

	DPRINTF(("%s: %s: phy=%d reg=0x%04x => 0x%04x\n",
		 device_xname(un->un_dev), __func__, phy, reg, val));

	return USBD_NORMAL_COMPLETION;
}

static usbd_status
url_ext_mii_write_reg(struct usbnet *un, int phy, int reg, int data)
{

	DPRINTF(("%s: %s: enter, phy=%d reg=0x%04x data=0x%04x\n",
		 device_xname(un->un_dev), __func__, phy, reg, data));

	url_csr_write_2(un, URL_PHYDAT, data);
	url_csr_write_1(un, URL_PHYADD, phy);
	url_csr_write_1(un, URL_PHYCNT, reg | URL_PHYCNT_RWCR);	/* Write */

	for (i=0; i < URL_TIMEOUT; i++) {
		if (url_csr_read_1(un, URL_PHYCNT) & URL_PHYCNT_PHYOWN)
			break;
	}

	if (i == URL_TIMEOUT) {
		printf("%s: MII write timed out\n",
		       device_xname(un->un_dev));
		return USBD_TIMEOUT;
	}

	return USBD_NORMAL_COMPLETION;
}
#endif

#ifdef _MODULE
#include "ioconf.c"
#endif

USBNET_MODULE(url)
