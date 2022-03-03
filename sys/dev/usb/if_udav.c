/*	$NetBSD: if_udav.c,v 1.81 2022/03/03 05:51:06 riastradh Exp $	*/
/*	$nabe: if_udav.c,v 1.3 2003/08/21 16:57:19 nabe Exp $	*/

/*
 * Copyright (c) 2003
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
 * DM9601(DAVICOM USB to Ethernet MAC Controller with Integrated 10/100 PHY)
 * The spec can be found at the following url.
 *   http://www.davicom.com.tw/big5/download/Data%20Sheet/DM9601-DS-F01-062202s.pdf
 */

/*
 * TODO:
 *	Interrupt Endpoint support
 *	External PHYs
 *	powerhook() support?
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_udav.c,v 1.81 2022/03/03 05:51:06 riastradh Exp $");

#ifdef _KERNEL_OPT
#include "opt_usb.h"
#endif

#include <sys/param.h>

#include <dev/usb/usbnet.h>
#include <dev/usb/if_udavreg.h>

/* Function declarations */
static int	udav_match(device_t, cfdata_t, void *);
static void	udav_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(udav, sizeof(struct usbnet), udav_match, udav_attach,
    usbnet_detach, usbnet_activate);

static void udav_chip_init(struct usbnet *);

static unsigned udav_uno_tx_prepare(struct usbnet *, struct mbuf *,
				    struct usbnet_chain *);
static void udav_uno_rx_loop(struct usbnet *, struct usbnet_chain *, uint32_t);
static void udav_uno_stop(struct ifnet *, int);
static void udav_uno_mcast(struct ifnet *);
static int udav_uno_mii_read_reg(struct usbnet *, int, int, uint16_t *);
static int udav_uno_mii_write_reg(struct usbnet *, int, int, uint16_t);
static void udav_uno_mii_statchg(struct ifnet *);
static int udav_uno_init(struct ifnet *);
static void udav_setiff_locked(struct usbnet *);
static void udav_reset(struct usbnet *);

static int udav_csr_read(struct usbnet *, int, void *, int);
static int udav_csr_write(struct usbnet *, int, void *, int);
static int udav_csr_read1(struct usbnet *, int);
static int udav_csr_write1(struct usbnet *, int, unsigned char);

#if 0
static int udav_mem_read(struct usbnet *, int, void *, int);
static int udav_mem_write(struct usbnet *, int, void *, int);
static int udav_mem_write1(struct usbnet *, int, unsigned char);
#endif

/* Macros */
#ifdef UDAV_DEBUG
#define DPRINTF(x)	if (udavdebug) printf x
#define DPRINTFN(n, x)	if (udavdebug >= (n)) printf x
int udavdebug = 0;
#else
#define DPRINTF(x)
#define DPRINTFN(n, x)
#endif

#define	UDAV_SETBIT(un, reg, x)	\
	udav_csr_write1(un, reg, udav_csr_read1(un, reg) | (x))

#define	UDAV_CLRBIT(un, reg, x)	\
	udav_csr_write1(un, reg, udav_csr_read1(un, reg) & ~(x))

static const struct udav_type {
	struct usb_devno udav_dev;
	uint16_t udav_flags;
#define UDAV_EXT_PHY	0x0001
#define UDAV_NO_PHY	0x0002
} udav_devs [] = {
	/* Corega USB-TXC */
	{{ USB_VENDOR_COREGA, USB_PRODUCT_COREGA_FETHER_USB_TXC }, 0},
	/* ShanTou ST268 USB NIC */
	{{ USB_VENDOR_SHANTOU, USB_PRODUCT_SHANTOU_ST268_USB_NIC }, 0},
	/* ShanTou ADM8515 */
	{{ USB_VENDOR_SHANTOU, USB_PRODUCT_SHANTOU_ADM8515 }, 0},
	/* SUNRISING SR9600 */
	{{ USB_VENDOR_SUNRISING, USB_PRODUCT_SUNRISING_SR9600 }, 0 },
	/* SUNRISING QF9700 */
	{{ USB_VENDOR_SUNRISING, USB_PRODUCT_SUNRISING_QF9700 }, UDAV_NO_PHY },
	/* QUAN DM9601 */
	{{USB_VENDOR_QUAN, USB_PRODUCT_QUAN_DM9601 }, 0},
#if 0
	/* DAVICOM DM9601 Generic? */
	/*  XXX: The following ids was obtained from the data sheet. */
	{{ 0x0a46, 0x9601 }, 0},
#endif
};
#define udav_lookup(v, p) ((const struct udav_type *)usb_lookup(udav_devs, v, p))

static const struct usbnet_ops udav_ops = {
	.uno_stop = udav_uno_stop,
	.uno_mcast = udav_uno_mcast,
	.uno_read_reg = udav_uno_mii_read_reg,
	.uno_write_reg = udav_uno_mii_write_reg,
	.uno_statchg = udav_uno_mii_statchg,
	.uno_tx_prepare = udav_uno_tx_prepare,
	.uno_rx_loop = udav_uno_rx_loop,
	.uno_init = udav_uno_init,
};

/* Probe */
static int
udav_match(device_t parent, cfdata_t match, void *aux)
{
	struct usb_attach_arg *uaa = aux;

	return udav_lookup(uaa->uaa_vendor, uaa->uaa_product) != NULL ?
		UMATCH_VENDOR_PRODUCT : UMATCH_NONE;
}

/* Attach */
static void
udav_attach(device_t parent, device_t self, void *aux)
{
	USBNET_MII_DECL_DEFAULT(unm);
	struct usbnet_mii *unmp;
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
	un->un_ops = &udav_ops;
	un->un_rx_xfer_flags = USBD_SHORT_XFER_OK;
	un->un_tx_xfer_flags = USBD_FORCE_SHORT_XFER;
	un->un_rx_list_cnt = UDAV_RX_LIST_CNT;
	un->un_tx_list_cnt = UDAV_TX_LIST_CNT;
	un->un_rx_bufsz = UDAV_BUFSZ;
	un->un_tx_bufsz = UDAV_BUFSZ;

	/* Move the device into the configured state. */
	err = usbd_set_config_no(dev, UDAV_CONFIG_NO, 1); /* idx 0 */
	if (err) {
		aprint_error_dev(self, "failed to set configuration"
		    ", err=%s\n", usbd_errstr(err));
		return;
	}

	/* get control interface */
	err = usbd_device2interface_handle(dev, UDAV_IFACE_INDEX, &iface);
	if (err) {
		aprint_error_dev(self, "failed to get interface, err=%s\n",
		       usbd_errstr(err));
		return;
	}

	un->un_iface = iface;
	un->un_flags = udav_lookup(uaa->uaa_vendor,
	    uaa->uaa_product)->udav_flags;

	/* get interface descriptor */
	id = usbd_get_interface_descriptor(un->un_iface);

	/* find endpoints */
	un->un_ed[USBNET_ENDPT_RX] = un->un_ed[USBNET_ENDPT_TX] =
	    un->un_ed[USBNET_ENDPT_INTR] = -1;
	for (i = 0; i < id->bNumEndpoints; i++) {
		ed = usbd_interface2endpoint_descriptor(un->un_iface, i);
		if (ed == NULL) {
			aprint_error_dev(self, "couldn't get endpoint %d\n", i);
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

	/* Not supported yet. */
	un->un_ed[USBNET_ENDPT_INTR] = 0;

	usbnet_attach(un, "udavdet");

	usbnet_lock_core(un);
	usbnet_busy(un);

// 	/* reset the adapter */
// 	udav_reset(un);

	/* Get Ethernet Address */
	err = udav_csr_read(un, UDAV_PAR, un->un_eaddr, ETHER_ADDR_LEN);
	usbnet_unbusy(un);
	usbnet_unlock_core(un);
	if (err) {
		aprint_error_dev(self, "read MAC address failed\n");
		return;
	}

	if (ISSET(un->un_flags, UDAV_NO_PHY))
		unmp = NULL;
	else
		unmp = &unm;

	/* initialize interface information */
	usbnet_attach_ifp(un, IFF_SIMPLEX | IFF_BROADCAST | IFF_MULTICAST,
	    0, unmp);

	return;
}

#if 0
/* read memory */
static int
udav_mem_read(struct usbnet *un, int offset, void *buf, int len)
{
	usb_device_request_t req;
	usbd_status err;

	DPRINTFN(0x200,
		("%s: %s: enter\n", device_xname(un->un_dev), __func__));

	if (usbnet_isdying(un))
		return 0;

	offset &= 0xffff;
	len &= 0xff;

	req.bmRequestType = UT_READ_VENDOR_DEVICE;
	req.bRequest = UDAV_REQ_MEM_READ;
	USETW(req.wValue, 0x0000);
	USETW(req.wIndex, offset);
	USETW(req.wLength, len);

	err = usbd_do_request(un->un_udev, &req, buf);
	if (err) {
		DPRINTF(("%s: %s: read failed. off=%04x, err=%d\n",
			 device_xname(un->un_dev), __func__, offset, err));
	}

	return err;
}

/* write memory */
static int
udav_mem_write(struct usbnet *un, int offset, void *buf, int len)
{
	usb_device_request_t req;
	usbd_status err;

	DPRINTFN(0x200,
		("%s: %s: enter\n", device_xname(un->un_dev), __func__));

	if (usbnet_isdying(un))
		return 0;

	offset &= 0xffff;
	len &= 0xff;

	req.bmRequestType = UT_WRITE_VENDOR_DEVICE;
	req.bRequest = UDAV_REQ_MEM_WRITE;
	USETW(req.wValue, 0x0000);
	USETW(req.wIndex, offset);
	USETW(req.wLength, len);

	err = usbd_do_request(un->un_udev, &req, buf);
	if (err) {
		DPRINTF(("%s: %s: write failed. off=%04x, err=%d\n",
			 device_xname(un->un_dev), __func__, offset, err));
	}

	return err;
}

/* write memory */
static int
udav_mem_write1(struct usbnet *un, int offset, unsigned char ch)
{
	usb_device_request_t req;
	usbd_status err;

	DPRINTFN(0x200,
		("%s: %s: enter\n", device_xname(un->un_dev), __func__));

	if (usbnet_isdying(un))
		return 0;

	offset &= 0xffff;

	req.bmRequestType = UT_WRITE_VENDOR_DEVICE;
	req.bRequest = UDAV_REQ_MEM_WRITE1;
	USETW(req.wValue, ch);
	USETW(req.wIndex, offset);
	USETW(req.wLength, 0x0000);

	err = usbd_do_request(un->un_udev, &req, NULL);
	if (err) {
		DPRINTF(("%s: %s: write failed. off=%04x, err=%d\n",
			 device_xname(un->un_dev), __func__, offset, err));
	}

	return err;
}
#endif

/* read register(s) */
static int
udav_csr_read(struct usbnet *un, int offset, void *buf, int len)
{
	usb_device_request_t req;
	usbd_status err;

	usbnet_isowned_core(un);
	KASSERT(!usbnet_isdying(un));

	DPRINTFN(0x200,
		("%s: %s: enter\n", device_xname(un->un_dev), __func__));

	offset &= 0xff;
	len &= 0xff;

	req.bmRequestType = UT_READ_VENDOR_DEVICE;
	req.bRequest = UDAV_REQ_REG_READ;
	USETW(req.wValue, 0x0000);
	USETW(req.wIndex, offset);
	USETW(req.wLength, len);

	err = usbd_do_request(un->un_udev, &req, buf);
	if (err) {
		DPRINTF(("%s: %s: read failed. off=%04x, err=%d\n",
			 device_xname(un->un_dev), __func__, offset, err));
	}

	return err;
}

/* write register(s) */
static int
udav_csr_write(struct usbnet *un, int offset, void *buf, int len)
{
	usb_device_request_t req;
	usbd_status err;

	usbnet_isowned_core(un);
	KASSERT(!usbnet_isdying(un));

	DPRINTFN(0x200,
		("%s: %s: enter\n", device_xname(un->un_dev), __func__));

	offset &= 0xff;
	len &= 0xff;

	req.bmRequestType = UT_WRITE_VENDOR_DEVICE;
	req.bRequest = UDAV_REQ_REG_WRITE;
	USETW(req.wValue, 0x0000);
	USETW(req.wIndex, offset);
	USETW(req.wLength, len);

	err = usbd_do_request(un->un_udev, &req, buf);
	if (err) {
		DPRINTF(("%s: %s: write failed. off=%04x, err=%d\n",
			 device_xname(un->un_dev), __func__, offset, err));
	}

	return err;
}

static int
udav_csr_read1(struct usbnet *un, int offset)
{
	uint8_t val = 0;

	usbnet_isowned_core(un);

	DPRINTFN(0x200,
		("%s: %s: enter\n", device_xname(un->un_dev), __func__));

	if (usbnet_isdying(un))
		return 0;

	return udav_csr_read(un, offset, &val, 1) ? 0 : val;
}

/* write a register */
static int
udav_csr_write1(struct usbnet *un, int offset, unsigned char ch)
{
	usb_device_request_t req;
	usbd_status err;

	usbnet_isowned_core(un);
	KASSERT(!usbnet_isdying(un));

	DPRINTFN(0x200,
		("%s: %s: enter\n", device_xname(un->un_dev), __func__));

	offset &= 0xff;

	req.bmRequestType = UT_WRITE_VENDOR_DEVICE;
	req.bRequest = UDAV_REQ_REG_WRITE1;
	USETW(req.wValue, ch);
	USETW(req.wIndex, offset);
	USETW(req.wLength, 0x0000);

	err = usbd_do_request(un->un_udev, &req, NULL);
	if (err) {
		DPRINTF(("%s: %s: write failed. off=%04x, err=%d\n",
			 device_xname(un->un_dev), __func__, offset, err));
	}

	return err;
}

static int
udav_uno_init(struct ifnet *ifp)
{
	struct usbnet * const un = ifp->if_softc;
	struct mii_data * const mii = usbnet_mii(un);
	uint8_t eaddr[ETHER_ADDR_LEN];
	int rc = 0;

	DPRINTF(("%s: %s: enter\n", device_xname(un->un_dev), __func__));

	if (usbnet_isdying(un)) {
		return EIO;
	}

	/* Cancel pending I/O and free all TX/RX buffers */
	if (ifp->if_flags & IFF_RUNNING)
		usbnet_stop(un, ifp, 1);

	usbnet_busy(un);

	memcpy(eaddr, CLLADDR(ifp->if_sadl), sizeof(eaddr));
	udav_csr_write(un, UDAV_PAR, eaddr, ETHER_ADDR_LEN);

	/* Initialize network control register */
	/*  Disable loopback  */
	UDAV_CLRBIT(un, UDAV_NCR, UDAV_NCR_LBK0 | UDAV_NCR_LBK1);

	/* Initialize RX control register */
	UDAV_SETBIT(un, UDAV_RCR, UDAV_RCR_DIS_LONG | UDAV_RCR_DIS_CRC);

	/* If we want promiscuous mode, accept all physical frames. */
	if (ifp->if_flags & IFF_PROMISC)
		UDAV_SETBIT(un, UDAV_RCR, UDAV_RCR_ALL | UDAV_RCR_PRMSC);
	else
		UDAV_CLRBIT(un, UDAV_RCR, UDAV_RCR_ALL | UDAV_RCR_PRMSC);

	/* Load the multicast filter */
	udav_setiff_locked(un);

	/* Enable RX */
	UDAV_SETBIT(un, UDAV_RCR, UDAV_RCR_RXEN);

	/* clear POWER_DOWN state of internal PHY */
	UDAV_SETBIT(un, UDAV_GPCR, UDAV_GPCR_GEP_CNTL0);
	UDAV_CLRBIT(un, UDAV_GPR, UDAV_GPR_GEPIO0);

	if (mii && (rc = mii_mediachg(mii)) == ENXIO)
		rc = 0;

	if (rc != 0) {
		usbnet_unbusy(un);
		return rc;
	}

	if (usbnet_isdying(un))
		rc = EIO;
	else
		rc = usbnet_init_rx_tx(un);

	usbnet_unbusy(un);

	return rc;
}

static void
udav_reset(struct usbnet *un)
{
    	usbnet_isowned_core(un);

	if (usbnet_isdying(un))
		return;

	DPRINTF(("%s: %s: enter\n", device_xname(un->un_dev), __func__));

	udav_chip_init(un);
}

static void
udav_chip_init(struct usbnet *un)
{
	usbnet_isowned_core(un);

	/* Select PHY */
#if 1
	/*
	 * XXX: force select internal phy.
	 *	external phy routines are not tested.
	 */
	UDAV_CLRBIT(un, UDAV_NCR, UDAV_NCR_EXT_PHY);
#else
	if (un->un_flags & UDAV_EXT_PHY) {
		UDAV_SETBIT(un, UDAV_NCR, UDAV_NCR_EXT_PHY);
	} else {
		UDAV_CLRBIT(un, UDAV_NCR, UDAV_NCR_EXT_PHY);
	}
#endif

	UDAV_SETBIT(un, UDAV_NCR, UDAV_NCR_RST);

	for (int i = 0; i < UDAV_TX_TIMEOUT; i++) {
		if (usbnet_isdying(un))
			return;
		if (!(udav_csr_read1(un, UDAV_NCR) & UDAV_NCR_RST))
			break;
		delay(10);	/* XXX */
	}
	delay(10000);		/* XXX */
}

#define UDAV_BITS	6

#define UDAV_CALCHASH(addr) \
	(ether_crc32_le((addr), ETHER_ADDR_LEN) & ((1 << UDAV_BITS) - 1))

static void
udav_setiff_locked(struct usbnet *un)
{
	struct ethercom *ec = usbnet_ec(un);
	struct ifnet * const ifp = usbnet_ifp(un);
	struct ether_multi *enm;
	struct ether_multistep step;
	uint8_t hashes[8];
	int h = 0;

	DPRINTF(("%s: %s: enter\n", device_xname(un->un_dev), __func__));

	usbnet_isowned_core(un);

	if (usbnet_isdying(un))
		return;

	if (ISSET(un->un_flags, UDAV_NO_PHY)) {
		UDAV_SETBIT(un, UDAV_RCR, UDAV_RCR_ALL);
		UDAV_SETBIT(un, UDAV_RCR, UDAV_RCR_PRMSC);
		return;
	}

	if (ifp->if_flags & IFF_PROMISC) {
		UDAV_SETBIT(un, UDAV_RCR, UDAV_RCR_ALL | UDAV_RCR_PRMSC);
		return;
	} else if (ifp->if_flags & IFF_ALLMULTI) {
allmulti:
		ifp->if_flags |= IFF_ALLMULTI;
		UDAV_SETBIT(un, UDAV_RCR, UDAV_RCR_ALL);
		UDAV_CLRBIT(un, UDAV_RCR, UDAV_RCR_PRMSC);
		return;
	}

	/* first, zot all the existing hash bits */
	memset(hashes, 0x00, sizeof(hashes));
	hashes[7] |= 0x80;	/* broadcast address */
	udav_csr_write(un, UDAV_MAR, hashes, sizeof(hashes));

	/* now program new ones */
	ETHER_LOCK(ec);
	ETHER_FIRST_MULTI(step, ec, enm);
	while (enm != NULL) {
		if (memcmp(enm->enm_addrlo, enm->enm_addrhi,
		    ETHER_ADDR_LEN) != 0) {
			ETHER_UNLOCK(ec);
			goto allmulti;
		}

		h = UDAV_CALCHASH(enm->enm_addrlo);
		hashes[h>>3] |= 1 << (h & 0x7);
		ETHER_NEXT_MULTI(step, enm);
	}
	ETHER_UNLOCK(ec);

	/* disable all multicast */
	ifp->if_flags &= ~IFF_ALLMULTI;
	UDAV_CLRBIT(un, UDAV_RCR, UDAV_RCR_ALL);

	/* write hash value to the register */
	udav_csr_write(un, UDAV_MAR, hashes, sizeof(hashes));
}

static unsigned
udav_uno_tx_prepare(struct usbnet *un, struct mbuf *m, struct usbnet_chain *c)
{
	int total_len;
	uint8_t *buf = c->unc_buf;

	DPRINTF(("%s: %s: enter\n", device_xname(un->un_dev), __func__));

	if ((unsigned)m->m_pkthdr.len > un->un_tx_bufsz - 2)
		return 0;

	/* Copy the mbuf data into a contiguous buffer */
	m_copydata(m, 0, m->m_pkthdr.len, buf + 2);
	total_len = m->m_pkthdr.len;
	if (total_len < UDAV_MIN_FRAME_LEN) {
		memset(buf + 2 + total_len, 0,
		    UDAV_MIN_FRAME_LEN - total_len);
		total_len = UDAV_MIN_FRAME_LEN;
	}

	/* Frame length is specified in the first 2bytes of the buffer */
	buf[0] = (uint8_t)total_len;
	buf[1] = (uint8_t)(total_len >> 8);
	total_len += 2;

	DPRINTF(("%s: %s: send %d bytes\n", device_xname(un->un_dev),
	    __func__, total_len));

	return total_len;
}

static void
udav_uno_rx_loop(struct usbnet *un, struct usbnet_chain *c, uint32_t total_len)
{
	struct ifnet *ifp = usbnet_ifp(un);
	uint8_t *buf = c->unc_buf;
	uint16_t pkt_len;
	uint8_t pktstat;

	DPRINTF(("%s: %s: enter\n", device_xname(un->un_dev), __func__));

	/* first byte in received data */
	pktstat = *buf;
	total_len -= sizeof(pktstat);
	buf += sizeof(pktstat);

	DPRINTF(("%s: RX Status: 0x%02x\n", device_xname(un->un_dev), pktstat));

	pkt_len = UGETW(buf);
 	total_len -= sizeof(pkt_len);
	buf += sizeof(pkt_len);

	DPRINTF(("%s: RX Length: 0x%02x\n", device_xname(un->un_dev), pkt_len));

	if (pktstat & UDAV_RSR_LCS) {
		if_statinc(ifp, if_collisions);
		return;
	}

	if (pkt_len < sizeof(struct ether_header) ||
	    pkt_len > total_len ||
	    (pktstat & UDAV_RSR_ERR)) {
		if_statinc(ifp, if_ierrors);
		return;
	}

	pkt_len -= ETHER_CRC_LEN;

	DPRINTF(("%s: Rx deliver: 0x%02x\n", device_xname(un->un_dev), pkt_len));

	usbnet_enqueue(un, buf, pkt_len, 0, 0, 0);
}

static void
udav_uno_mcast(struct ifnet *ifp)
{
	struct usbnet * const un = ifp->if_softc;

	usbnet_lock_core(un);
	usbnet_busy(un);

	udav_setiff_locked(un);

	usbnet_unbusy(un);
	usbnet_unlock_core(un);
}

/* Stop the adapter and free any mbufs allocated to the RX and TX lists. */
static void
udav_uno_stop(struct ifnet *ifp, int disable)
{
	struct usbnet * const un = ifp->if_softc;

	DPRINTF(("%s: %s: enter\n", device_xname(un->un_dev), __func__));

	udav_reset(un);
}

static int
udav_uno_mii_read_reg(struct usbnet *un, int phy, int reg, uint16_t *val)
{
	uint8_t data[2];

	DPRINTFN(0xff, ("%s: %s: enter, phy=%d reg=0x%04x\n",
		 device_xname(un->un_dev), __func__, phy, reg));

	if (usbnet_isdying(un)) {
#ifdef DIAGNOSTIC
		printf("%s: %s: dying\n", device_xname(un->un_dev),
		       __func__);
#endif
		return EINVAL;
	}

	/* XXX: one PHY only for the internal PHY */
	if (phy != 0) {
		DPRINTFN(0xff, ("%s: %s: phy=%d is not supported\n",
			 device_xname(un->un_dev), __func__, phy));
		return EINVAL;
	}

	/* select internal PHY and set PHY register address */
	udav_csr_write1(un, UDAV_EPAR,
			UDAV_EPAR_PHY_ADR0 | (reg & UDAV_EPAR_EROA_MASK));

	/* select PHY operation and start read command */
	udav_csr_write1(un, UDAV_EPCR, UDAV_EPCR_EPOS | UDAV_EPCR_ERPRR);

	/* XXX: should be wait? */

	/* end read command */
	UDAV_CLRBIT(un, UDAV_EPCR, UDAV_EPCR_ERPRR);

	/* retrieve the result from data registers */
	udav_csr_read(un, UDAV_EPDRL, data, 2);

	*val = data[0] | (data[1] << 8);

	DPRINTFN(0xff, ("%s: %s: phy=%d reg=0x%04x => 0x%04hx\n",
		device_xname(un->un_dev), __func__, phy, reg, *val));

	return 0;
}

static int
udav_uno_mii_write_reg(struct usbnet *un, int phy, int reg, uint16_t val)
{
	uint8_t data[2];

	DPRINTFN(0xff, ("%s: %s: enter, phy=%d reg=0x%04x val=0x%04hx\n",
		 device_xname(un->un_dev), __func__, phy, reg, val));

	if (usbnet_isdying(un)) {
#ifdef DIAGNOSTIC
		printf("%s: %s: dying\n", device_xname(un->un_dev),
		       __func__);
#endif
		return EIO;
	}

	/* XXX: one PHY only for the internal PHY */
	if (phy != 0) {
		DPRINTFN(0xff, ("%s: %s: phy=%d is not supported\n",
			 device_xname(un->un_dev), __func__, phy));
		return EIO;
	}

	/* select internal PHY and set PHY register address */
	udav_csr_write1(un, UDAV_EPAR,
			UDAV_EPAR_PHY_ADR0 | (reg & UDAV_EPAR_EROA_MASK));

	/* put the value to the data registers */
	data[0] = val & 0xff;
	data[1] = (val >> 8) & 0xff;
	udav_csr_write(un, UDAV_EPDRL, data, 2);

	/* select PHY operation and start write command */
	udav_csr_write1(un, UDAV_EPCR, UDAV_EPCR_EPOS | UDAV_EPCR_ERPRW);

	/* XXX: should be wait? */

	/* end write command */
	UDAV_CLRBIT(un, UDAV_EPCR, UDAV_EPCR_ERPRW);

	return 0;
}

static void
udav_uno_mii_statchg(struct ifnet *ifp)
{
	struct usbnet * const un = ifp->if_softc;
	struct mii_data * const mii = usbnet_mii(un);

	DPRINTF(("%s: %s: enter\n", ifp->if_xname, __func__));

	if (usbnet_isdying(un))
		return;

	if ((mii->mii_media_status & IFM_ACTIVE) &&
	    IFM_SUBTYPE(mii->mii_media_active) != IFM_NONE) {
		DPRINTF(("%s: %s: got link\n",
			 device_xname(un->un_dev), __func__));
		usbnet_set_link(un, true);
	}
}

#ifdef _MODULE
#include "ioconf.c"
#endif

USBNET_MODULE(udav)
