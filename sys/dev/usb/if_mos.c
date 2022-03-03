/*	$NetBSD: if_mos.c,v 1.17 2022/03/03 05:54:21 riastradh Exp $	*/
/*	$OpenBSD: if_mos.c,v 1.40 2019/07/07 06:40:10 kevlo Exp $	*/

/*
 * Copyright (c) 2008 Johann Christian Rode <jcrode@gmx.net>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * Copyright (c) 2005, 2006, 2007 Jonathan Gray <jsg@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * Copyright (c) 1997, 1998, 1999, 2000-2003
 *	Bill Paul <wpaul@windriver.com>.  All rights reserved.
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
 */

/*
 * Moschip MCS7730/MCS7830/MCS7832 USB to Ethernet controller 
 * The datasheet is available at the following URL: 
 * http://www.moschip.com/data/products/MCS7830/Data%20Sheet_7830.pdf
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_mos.c,v 1.17 2022/03/03 05:54:21 riastradh Exp $");

#include <sys/param.h>

#include <dev/usb/usbnet.h>
#include <dev/usb/if_mosreg.h>

#define MOS_PAUSE_REWRITES	3

#define MOS_TIMEOUT		1000

#define MOS_RX_LIST_CNT		1
#define MOS_TX_LIST_CNT		1

/* Maximum size of a fast ethernet frame plus one byte for the status */
#define MOS_BUFSZ	 	(ETHER_MAX_LEN+1)

/*
 * USB endpoints.
 */
#define MOS_ENDPT_RX		0
#define MOS_ENDPT_TX		1
#define MOS_ENDPT_INTR		2
#define MOS_ENDPT_MAX		3

/*
 * USB vendor requests.
 */
#define MOS_UR_READREG		0x0e
#define MOS_UR_WRITEREG		0x0d

#define MOS_CONFIG_NO		1
#define MOS_IFACE_IDX		0

struct mos_type {
	struct usb_devno	mos_dev;
	u_int16_t		mos_flags;
#define MCS7730	0x0001		/* MCS7730 */
#define MCS7830	0x0002		/* MCS7830 */
#define MCS7832	0x0004		/* MCS7832 */
};

#define MOS_INC(x, y)           (x) = (x + 1) % y

#ifdef MOS_DEBUG
#define DPRINTF(x)      do { if (mosdebug) printf x; } while (0)
#define DPRINTFN(n,x)   do { if (mosdebug >= (n)) printf x; } while (0)
int     mosdebug = 0;
#else
#define DPRINTF(x)	__nothing
#define DPRINTFN(n,x)	__nothing
#endif

/*
 * Various supported device vendors/products.
 */
static const struct mos_type mos_devs[] = {
	{ { USB_VENDOR_MOSCHIP, USB_PRODUCT_MOSCHIP_MCS7730 }, MCS7730 },
	{ { USB_VENDOR_MOSCHIP, USB_PRODUCT_MOSCHIP_MCS7830 }, MCS7830 },
	{ { USB_VENDOR_MOSCHIP, USB_PRODUCT_MOSCHIP_MCS7832 }, MCS7832 },
	{ { USB_VENDOR_SITECOMEU, USB_PRODUCT_SITECOMEU_LN030 }, MCS7830 },
};
#define mos_lookup(v, p) ((const struct mos_type *)usb_lookup(mos_devs, v, p))

static int mos_match(device_t, cfdata_t, void *);
static void mos_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(mos, sizeof(struct usbnet),
	mos_match, mos_attach, usbnet_detach, usbnet_activate);

static void mos_uno_rx_loop(struct usbnet *, struct usbnet_chain *, uint32_t);
static unsigned mos_uno_tx_prepare(struct usbnet *, struct mbuf *,
				   struct usbnet_chain *);
static void mos_uno_mcast(struct ifnet *);
static int mos_uno_init(struct ifnet *);
static void mos_chip_init(struct usbnet *);
static void mos_uno_stop(struct ifnet *ifp, int disable);
static int mos_uno_mii_read_reg(struct usbnet *, int, int, uint16_t *);
static int mos_uno_mii_write_reg(struct usbnet *, int, int, uint16_t);
static void mos_uno_mii_statchg(struct ifnet *);
static void mos_reset(struct usbnet *);

static int mos_reg_read_1(struct usbnet *, int);
static int mos_reg_read_2(struct usbnet *, int);
static int mos_reg_write_1(struct usbnet *, int, int);
static int mos_reg_write_2(struct usbnet *, int, int);
static int mos_readmac(struct usbnet *);
static int mos_writemac(struct usbnet *);
static int mos_write_mcast(struct usbnet *, uint8_t *);

static const struct usbnet_ops mos_ops = {
	.uno_stop = mos_uno_stop,
	.uno_mcast = mos_uno_mcast,
	.uno_read_reg = mos_uno_mii_read_reg,
	.uno_write_reg = mos_uno_mii_write_reg,
	.uno_statchg = mos_uno_mii_statchg,
	.uno_tx_prepare = mos_uno_tx_prepare,
	.uno_rx_loop = mos_uno_rx_loop,
	.uno_init = mos_uno_init,
};

static int
mos_reg_read_1(struct usbnet *un, int reg)
{
	usb_device_request_t	req;
	usbd_status		err;
	uByte			val = 0;

	if (usbnet_isdying(un))
		return 0;

	req.bmRequestType = UT_READ_VENDOR_DEVICE;
	req.bRequest = MOS_UR_READREG;
	USETW(req.wValue, 0);
	USETW(req.wIndex, reg);
	USETW(req.wLength, 1);

	err = usbd_do_request(un->un_udev, &req, &val);

	if (err) {
		aprint_error_dev(un->un_dev, "read reg %x\n", reg);
		return 0;
	}

	return val;
}

static int
mos_reg_read_2(struct usbnet *un, int reg)
{
	usb_device_request_t	req;
	usbd_status		err;
	uWord			val;

	if (usbnet_isdying(un))
		return 0;

	USETW(val,0);

	req.bmRequestType = UT_READ_VENDOR_DEVICE;
	req.bRequest = MOS_UR_READREG;
	USETW(req.wValue, 0);
	USETW(req.wIndex, reg);
	USETW(req.wLength, 2);

	err = usbd_do_request(un->un_udev, &req, &val);

	if (err) {
		aprint_error_dev(un->un_dev, "read reg2 %x\n", reg);
		return 0;
	}

	return UGETW(val);
}

static int
mos_reg_write_1(struct usbnet *un, int reg, int aval)
{
	usb_device_request_t	req;
	usbd_status		err;
	uByte			val;

	if (usbnet_isdying(un))
		return 0;

	val = aval;

	req.bmRequestType = UT_WRITE_VENDOR_DEVICE;
	req.bRequest = MOS_UR_WRITEREG;
	USETW(req.wValue, 0);
	USETW(req.wIndex, reg);
	USETW(req.wLength, 1);

	err = usbd_do_request(un->un_udev, &req, &val);

	if (err)
		aprint_error_dev(un->un_dev, "write reg %x <- %x\n", 
		    reg, aval);

	return 0;
}

static int
mos_reg_write_2(struct usbnet *un, int reg, int aval)
{
	usb_device_request_t	req;
	usbd_status		err;
	uWord			val;

	USETW(val, aval);

	if (usbnet_isdying(un))
		return EIO;

	req.bmRequestType = UT_WRITE_VENDOR_DEVICE;
	req.bRequest = MOS_UR_WRITEREG;
	USETW(req.wValue, 0);
	USETW(req.wIndex, reg);
	USETW(req.wLength, 2);

	err = usbd_do_request(un->un_udev, &req, &val);

	if (err)
		aprint_error_dev(un->un_dev, "write reg2 %x <- %x\n", 
		    reg, aval);

	return 0;
}

static int
mos_readmac(struct usbnet *un)
{
	usb_device_request_t	req;
	usbd_status		err;

	if (usbnet_isdying(un))
		return 0;

	req.bmRequestType = UT_READ_VENDOR_DEVICE;
	req.bRequest = MOS_UR_READREG;
	USETW(req.wValue, 0);
	USETW(req.wIndex, MOS_MAC);
	USETW(req.wLength, ETHER_ADDR_LEN);

	err = usbd_do_request(un->un_udev, &req, un->un_eaddr);

	if (err)
		aprint_error_dev(un->un_dev, "%s: failed", __func__);

	return err;
}

static int
mos_writemac(struct usbnet *un)
{
	usb_device_request_t	req;
	usbd_status		err;

	if (usbnet_isdying(un))
		return 0;

	req.bmRequestType = UT_WRITE_VENDOR_DEVICE;
	req.bRequest = MOS_UR_WRITEREG;
	USETW(req.wValue, 0);
	USETW(req.wIndex, MOS_MAC);
	USETW(req.wLength, ETHER_ADDR_LEN);

	err = usbd_do_request(un->un_udev, &req, un->un_eaddr);

	if (err)
		aprint_error_dev(un->un_dev, "%s: failed", __func__);

	return 0;
}

static int
mos_write_mcast(struct usbnet *un, uint8_t *hashtbl)
{
	usb_device_request_t	req;
	usbd_status		err;

	if (usbnet_isdying(un))
		return EIO;

	req.bmRequestType = UT_WRITE_VENDOR_DEVICE;
	req.bRequest = MOS_UR_WRITEREG;
	USETW(req.wValue, 0);
	USETW(req.wIndex, MOS_MCAST_TABLE);
	USETW(req.wLength, 8);

	err = usbd_do_request(un->un_udev, &req, hashtbl);

	if (err) {
		aprint_error_dev(un->un_dev, "%s: failed", __func__);
		return(-1);
	}

	return 0;
}

static int
mos_uno_mii_read_reg(struct usbnet *un, int phy, int reg, uint16_t *val)
{
	int			i, res;

	mos_reg_write_2(un, MOS_PHY_DATA, 0);
	mos_reg_write_1(un, MOS_PHY_CTL, (phy & MOS_PHYCTL_PHYADDR) |
	    MOS_PHYCTL_READ);
	mos_reg_write_1(un, MOS_PHY_STS, (reg & MOS_PHYSTS_PHYREG) |
	    MOS_PHYSTS_PENDING);

	for (i = 0; i < MOS_TIMEOUT; i++) {
		if (usbnet_isdying(un)) {
			*val = 0;
			return ENXIO;
		}
		if (mos_reg_read_1(un, MOS_PHY_STS) & MOS_PHYSTS_READY)
			break;
	}
	if (i == MOS_TIMEOUT) {
		aprint_error_dev(un->un_dev, "read PHY failed\n");
		*val = 0;
		return EIO;
	}

	res = mos_reg_read_2(un, MOS_PHY_DATA);
	*val = res;

	DPRINTFN(10,("%s: %s: phy %d reg %d val %u\n",
	    device_xname(un->un_dev), __func__, phy, reg, res));

	return 0;
}

static int
mos_uno_mii_write_reg(struct usbnet *un, int phy, int reg, uint16_t val)
{
	int			i;

	DPRINTFN(10,("%s: %s: phy %d reg %d val %u\n",
	    device_xname(un->un_dev), __func__, phy, reg, val));

	mos_reg_write_2(un, MOS_PHY_DATA, val);
	mos_reg_write_1(un, MOS_PHY_CTL, (phy & MOS_PHYCTL_PHYADDR) |
	    MOS_PHYCTL_WRITE);
	mos_reg_write_1(un, MOS_PHY_STS, (reg & MOS_PHYSTS_PHYREG) |
	    MOS_PHYSTS_PENDING);

	for (i = 0; i < MOS_TIMEOUT; i++) {
		if (usbnet_isdying(un))
			return ENXIO;
		if (mos_reg_read_1(un, MOS_PHY_STS) & MOS_PHYSTS_READY)
			break;
	}
	if (i == MOS_TIMEOUT) {
		aprint_error_dev(un->un_dev, "write PHY failed\n");
		return EIO;
	}

	return 0;
}

void
mos_uno_mii_statchg(struct ifnet *ifp)
{
	struct usbnet * const		un = ifp->if_softc;
	struct mii_data * const		mii = usbnet_mii(un);
	int				val, err;

	if (usbnet_isdying(un))
		return;

	DPRINTFN(10,("%s: %s: enter\n", device_xname(un->un_dev), __func__));

	/* disable RX, TX prior to changing FDX, SPEEDSEL */
	val = mos_reg_read_1(un, MOS_CTL);
	val &= ~(MOS_CTL_TX_ENB | MOS_CTL_RX_ENB);
	mos_reg_write_1(un, MOS_CTL, val);

	/* reset register which counts dropped frames */
	mos_reg_write_1(un, MOS_FRAME_DROP_CNT, 0);

	if ((mii->mii_media_active & IFM_GMASK) == IFM_FDX)
		val |= MOS_CTL_FDX_ENB;
	else
		val &= ~(MOS_CTL_FDX_ENB);

	if ((mii->mii_media_status & (IFM_ACTIVE | IFM_AVALID)) ==
	    (IFM_ACTIVE | IFM_AVALID)) {
		switch (IFM_SUBTYPE(mii->mii_media_active)) {
		case IFM_100_TX:
			val |=  MOS_CTL_SPEEDSEL;
			break;
		case IFM_10_T:
			val &= ~(MOS_CTL_SPEEDSEL);
			break;
		}
		usbnet_set_link(un, true);
	}

	/* re-enable TX, RX */
	val |= (MOS_CTL_TX_ENB | MOS_CTL_RX_ENB);
	err = mos_reg_write_1(un, MOS_CTL, val);

	if (err)
		aprint_error_dev(un->un_dev, "media change failed\n");
}

static void
mos_uno_mcast(struct ifnet *ifp)
{
	struct usbnet		*un = ifp->if_softc;
	struct ethercom		*ec = usbnet_ec(un);
	struct ether_multi	*enm;
	struct ether_multistep	step;
	u_int32_t h = 0;
	u_int8_t rxmode, mchash[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

	if (usbnet_isdying(un))
		return;

	rxmode = mos_reg_read_1(un, MOS_CTL);
	rxmode &= ~(MOS_CTL_ALLMULTI | MOS_CTL_RX_PROMISC);

	ETHER_LOCK(ec);
	if (ifp->if_flags & IFF_PROMISC) {
		ec->ec_flags |= ETHER_F_ALLMULTI;
		ETHER_UNLOCK(ec);
		/* run promisc. mode */
		rxmode |= MOS_CTL_ALLMULTI; /* ??? */
		rxmode |= MOS_CTL_RX_PROMISC;
		goto update;
	}
	ec->ec_flags &= ~ETHER_F_ALLMULTI;
	ETHER_FIRST_MULTI(step, ec, enm);
	while (enm != NULL) {
		if (memcmp(enm->enm_addrlo, enm->enm_addrhi, ETHER_ADDR_LEN)) {
			ec->ec_flags |= ETHER_F_ALLMULTI;
			ETHER_UNLOCK(ec);
			memset(mchash, 0, sizeof(mchash)); /* correct ??? */
			/* accept all multicast frame */
			rxmode |= MOS_CTL_ALLMULTI;
			goto update;
		}
		h = ether_crc32_be(enm->enm_addrlo, ETHER_ADDR_LEN);
		/* 3(31:29) and 3(28:26) sampling to have uint8_t[8] */
		mchash[h >> 29] |= 1 << ((h >> 26) % 8);
		ETHER_NEXT_MULTI(step, enm);
	}
	ETHER_UNLOCK(ec);
	/* MOS receive filter is always on */
 update:
	/* 
	 * The datasheet claims broadcast frames were always accepted
	 * regardless of filter settings. But the hardware seems to
	 * filter broadcast frames, so pass them explicitly.
	 */
	mchash[7] |= 0x80;
	mos_write_mcast(un, mchash);
	mos_reg_write_1(un, MOS_CTL, rxmode);
}

static void
mos_reset(struct usbnet *un)
{
	u_int8_t ctl;

	if (usbnet_isdying(un))
		return;

	ctl = mos_reg_read_1(un, MOS_CTL);
	ctl &= ~(MOS_CTL_RX_PROMISC | MOS_CTL_ALLMULTI | MOS_CTL_TX_ENB |
	    MOS_CTL_RX_ENB);
	/* Disable RX, TX, promiscuous and allmulticast mode */
	mos_reg_write_1(un, MOS_CTL, ctl);

	/* Reset frame drop counter register to zero */
	mos_reg_write_1(un, MOS_FRAME_DROP_CNT, 0);

	/* Wait a little while for the chip to get its brains in order. */
	DELAY(1000);
}

void
mos_chip_init(struct usbnet *un)
{
	int	i;

	/*
	 * Rev.C devices have a pause threshold register which needs to be set
	 * at startup.
	 */
	if (mos_reg_read_1(un, MOS_PAUSE_TRHD) != -1) {
		for (i = 0; i < MOS_PAUSE_REWRITES; i++)
			mos_reg_write_1(un, MOS_PAUSE_TRHD, 0);
	}
}

/*
 * Probe for a MCS7x30 chip.
 */
static int
mos_match(device_t parent, cfdata_t match, void *aux)
{
	struct usb_attach_arg *uaa = aux;

	return (mos_lookup(uaa->uaa_vendor, uaa->uaa_product) != NULL ?
	    UMATCH_VENDOR_PRODUCT : UMATCH_NONE);
}

/*
 * Attach the interface.
 */
static void
mos_attach(device_t parent, device_t self, void *aux)
{
	USBNET_MII_DECL_DEFAULT(unm);
	struct usbnet *		un = device_private(self);
	struct usb_attach_arg	*uaa = aux;
	struct usbd_device	*dev = uaa->uaa_device;
	usbd_status		err;
	usb_interface_descriptor_t 	*id;
	usb_endpoint_descriptor_t 	*ed;
	char			*devinfop;
	int			i;

	aprint_naive("\n");
	aprint_normal("\n");
	devinfop = usbd_devinfo_alloc(dev, 0);
	aprint_normal_dev(self, "%s\n", devinfop);
	usbd_devinfo_free(devinfop);

	un->un_dev = self;
	un->un_udev = dev;
	un->un_sc = un;
	un->un_ops = &mos_ops;
	un->un_rx_xfer_flags = USBD_SHORT_XFER_OK;
	un->un_tx_xfer_flags = USBD_FORCE_SHORT_XFER;
	un->un_rx_list_cnt = MOS_RX_LIST_CNT;
	un->un_tx_list_cnt = MOS_TX_LIST_CNT;
	un->un_rx_bufsz = un->un_tx_bufsz = MOS_BUFSZ;

	err = usbd_set_config_no(dev, MOS_CONFIG_NO, 1);
	if (err) {
		aprint_error_dev(self, "failed to set configuration"
		    ", err=%s\n", usbd_errstr(err));
		return;
	}

	err = usbd_device2interface_handle(dev, MOS_IFACE_IDX, &un->un_iface);
	if (err) {
		aprint_error_dev(self, "failed getting interface handle"
		    ", err=%s\n", usbd_errstr(err));
		return;
	}

	un->un_flags = mos_lookup(uaa->uaa_vendor, uaa->uaa_product)->mos_flags;

	id = usbd_get_interface_descriptor(un->un_iface);

	/* Find endpoints. */
	for (i = 0; i < id->bNumEndpoints; i++) {
		ed = usbd_interface2endpoint_descriptor(un->un_iface, i);
		if (!ed) {
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

	if (un->un_flags & MCS7730)
		aprint_normal_dev(self, "MCS7730\n");
	else if (un->un_flags & MCS7830)
		aprint_normal_dev(self, "MCS7830\n");
	else if (un->un_flags & MCS7832)
		aprint_normal_dev(self, "MCS7832\n");

	/* Set these up now for register access. */
	usbnet_attach(un, "mosdet");

	mos_chip_init(un);

	/*
	 * Read MAC address, inform the world.
	 */
	err = mos_readmac(un);
	if (err) {
		aprint_error_dev(self, "couldn't read MAC address\n");
		return;
	}

	struct ifnet *ifp = usbnet_ifp(un);
	ifp->if_capabilities = ETHERCAP_VLAN_MTU;

	usbnet_attach_ifp(un, IFF_SIMPLEX | IFF_BROADCAST | IFF_MULTICAST,
	    0, &unm);
}

/*
 * A frame has been uploaded: pass the resulting mbuf chain up to
 * the higher level protocols.
 */
void
mos_uno_rx_loop(struct usbnet * un, struct usbnet_chain *c, uint32_t total_len)
{
	struct ifnet		*ifp = usbnet_ifp(un);
	uint8_t			*buf = c->unc_buf;
	u_int8_t		rxstat;
	u_int16_t		pktlen = 0;

	DPRINTFN(5,("%s: %s: enter len %u\n",
	    device_xname(un->un_dev), __func__, total_len));

	if (total_len <= 1)
		return;

	/* evaluate status byte at the end */
	pktlen = total_len - 1;
	if (pktlen > un->un_rx_bufsz) {
		if_statinc(ifp, if_ierrors);
		return;
	}
	rxstat = buf[pktlen] & MOS_RXSTS_MASK;

	if (rxstat != MOS_RXSTS_VALID) {
		DPRINTF(("%s: erroneous frame received: ",
		    device_xname(un->un_dev)));
		if (rxstat & MOS_RXSTS_SHORT_FRAME)
			DPRINTF(("frame size less than 64 bytes\n"));
		if (rxstat & MOS_RXSTS_LARGE_FRAME)
			DPRINTF(("frame size larger than 1532 bytes\n"));
		if (rxstat & MOS_RXSTS_CRC_ERROR)
			DPRINTF(("CRC error\n"));
		if (rxstat & MOS_RXSTS_ALIGN_ERROR)
			DPRINTF(("alignment error\n"));
		if_statinc(ifp, if_ierrors);
		return;
	}

	if (pktlen < sizeof(struct ether_header) ) {
		if_statinc(ifp, if_ierrors);
		return;
	}

	usbnet_enqueue(un, c->unc_buf, pktlen, 0, 0, 0);
}

static unsigned
mos_uno_tx_prepare(struct usbnet *un, struct mbuf *m, struct usbnet_chain *c)
{
	int			length;

	if ((unsigned)m->m_pkthdr.len > un->un_tx_bufsz)
		return 0;

	m_copydata(m, 0, m->m_pkthdr.len, c->unc_buf);
	length = m->m_pkthdr.len;

	DPRINTFN(5,("%s: %s: len %u\n",
	    device_xname(un->un_dev), __func__, length));

	return length;
}

static int
mos_uno_init(struct ifnet *ifp)
{
	struct usbnet * const un = ifp->if_softc;
	u_int8_t		rxmode;
	unsigned char		ipgs[2];

	if (usbnet_isdying(un))
		return EIO;

	/* Cancel pending I/O */
	usbnet_stop(un, ifp, 1);

	/* Reset the ethernet interface. */
	mos_reset(un);

	/* Write MAC address. */
	mos_writemac(un);

	/* Read and set transmitter IPG values */
	ipgs[0] = mos_reg_read_1(un, MOS_IPG0);
	ipgs[1] = mos_reg_read_1(un, MOS_IPG1);
	mos_reg_write_1(un, MOS_IPG0, ipgs[0]);
	mos_reg_write_1(un, MOS_IPG1, ipgs[1]);

	/* Enable receiver and transmitter, bridge controls speed/duplex mode */
	rxmode = mos_reg_read_1(un, MOS_CTL);
	rxmode |= MOS_CTL_RX_ENB | MOS_CTL_TX_ENB | MOS_CTL_BS_ENB;
	rxmode &= ~(MOS_CTL_SLEEP);
	mos_reg_write_1(un, MOS_CTL, rxmode);

	return usbnet_init_rx_tx(un);
}

void
mos_uno_stop(struct ifnet *ifp, int disable)
{
	struct usbnet * const un = ifp->if_softc;

	mos_reset(un);
}
