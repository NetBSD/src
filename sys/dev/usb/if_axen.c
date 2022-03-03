/*	$NetBSD: if_axen.c,v 1.77 2022/03/03 05:51:17 riastradh Exp $	*/
/*	$OpenBSD: if_axen.c,v 1.3 2013/10/21 10:10:22 yuo Exp $	*/

/*
 * Copyright (c) 2013 Yojiro UO <yuo@openbsd.org>
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
 * ASIX Electronics AX88178a USB 2.0 ethernet and AX88179 USB 3.0 Ethernet
 * driver.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_axen.c,v 1.77 2022/03/03 05:51:17 riastradh Exp $");

#ifdef _KERNEL_OPT
#include "opt_usb.h"
#endif

#include <sys/param.h>

#include <netinet/in.h>		/* XXX for netinet/ip.h */
#include <netinet/ip.h>		/* XXX for IP_MAXPACKET */

#include <dev/usb/usbnet.h>

#include <dev/usb/if_axenreg.h>

#ifdef AXEN_DEBUG
#define DPRINTF(x)	do { if (axendebug) printf x; } while (/*CONSTCOND*/0)
#define DPRINTFN(n, x)	do { if (axendebug >= (n)) printf x; } while (/*CONSTCOND*/0)
int	axendebug = 0;
#else
#define DPRINTF(x)
#define DPRINTFN(n, x)
#endif

struct axen_type {
	struct usb_devno	axen_devno;
	uint16_t		axen_flags;
#define AX178A	0x0001		/* AX88178a */
#define AX179	0x0002		/* AX88179 */
};

/*
 * Various supported device vendors/products.
 */
static const struct axen_type axen_devs[] = {
#if 0 /* not tested */
	{ { USB_VENDOR_ASIX, USB_PRODUCT_ASIX_AX88178A}, AX178A },
#endif
	{ { USB_VENDOR_ASIX, USB_PRODUCT_ASIX_AX88179}, AX179 },
	{ { USB_VENDOR_DLINK, USB_PRODUCT_DLINK_DUB1312}, AX179 }
};

#define axen_lookup(v, p) ((const struct axen_type *)usb_lookup(axen_devs, v, p))

static int	axen_match(device_t, cfdata_t, void *);
static void	axen_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(axen, sizeof(struct usbnet),
	axen_match, axen_attach, usbnet_detach, usbnet_activate);

static int	axen_cmd(struct usbnet *, int, int, int, void *);
static void	axen_reset(struct usbnet *);
static int	axen_get_eaddr(struct usbnet *, void *);
static void	axen_ax88179_init(struct usbnet *);

static void	axen_uno_stop(struct ifnet *, int);
static int	axen_uno_ioctl(struct ifnet *, u_long, void *);
static void	axen_uno_mcast(struct ifnet *);
static int	axen_uno_mii_read_reg(struct usbnet *, int, int, uint16_t *);
static int	axen_uno_mii_write_reg(struct usbnet *, int, int, uint16_t);
static void	axen_uno_mii_statchg(struct ifnet *);
static void	axen_uno_rx_loop(struct usbnet *, struct usbnet_chain *,
				 uint32_t);
static unsigned	axen_uno_tx_prepare(struct usbnet *, struct mbuf *,
				    struct usbnet_chain *);
static int	axen_uno_init(struct ifnet *);

static const struct usbnet_ops axen_ops = {
	.uno_stop = axen_uno_stop,
	.uno_ioctl = axen_uno_ioctl,
	.uno_mcast = axen_uno_mcast,
	.uno_read_reg = axen_uno_mii_read_reg,
	.uno_write_reg = axen_uno_mii_write_reg,
	.uno_statchg = axen_uno_mii_statchg,
	.uno_tx_prepare = axen_uno_tx_prepare,
	.uno_rx_loop = axen_uno_rx_loop,
	.uno_init = axen_uno_init,
};

static int
axen_cmd(struct usbnet *un, int cmd, int index, int val, void *buf)
{
	usb_device_request_t req;
	usbd_status err;

	usbnet_isowned_core(un);

	if (usbnet_isdying(un))
		return 0;

	if (AXEN_CMD_DIR(cmd))
		req.bmRequestType = UT_WRITE_VENDOR_DEVICE;
	else
		req.bmRequestType = UT_READ_VENDOR_DEVICE;
	req.bRequest = AXEN_CMD_CMD(cmd);
	USETW(req.wValue, val);
	USETW(req.wIndex, index);
	USETW(req.wLength, AXEN_CMD_LEN(cmd));

	err = usbd_do_request(un->un_udev, &req, buf);
	DPRINTFN(5, ("axen_cmd: cmd 0x%04x val 0x%04x len %d\n",
	    cmd, val, AXEN_CMD_LEN(cmd)));

	if (err) {
		DPRINTF(("%s: cmd: %d, error: %d\n", __func__, cmd, err));
		return -1;
	}

	return 0;
}

static int
axen_uno_mii_read_reg(struct usbnet *un, int phy, int reg, uint16_t *val)
{
	uint16_t data;

	if (un->un_phyno != phy)
		return EINVAL;

	usbd_status err = axen_cmd(un, AXEN_CMD_MII_READ_REG, reg, phy, &data);
	if (err)
		return EIO;

	*val = le16toh(data);
	if (reg == MII_BMSR)
		*val &= ~BMSR_EXTCAP;

	return 0;
}

static int
axen_uno_mii_write_reg(struct usbnet *un, int phy, int reg, uint16_t val)
{
	uint16_t uval = htole16(val);

	if (un->un_phyno != phy)
		return EINVAL;

	usbd_status err = axen_cmd(un, AXEN_CMD_MII_WRITE_REG, reg, phy, &uval);
	if (err)
		return EIO;

	return 0;
}

static void
axen_uno_mii_statchg(struct ifnet *ifp)
{
	struct usbnet * const un = ifp->if_softc;
	struct mii_data * const mii = usbnet_mii(un);
	int err;
	uint16_t val;
	uint16_t wval;

	if (usbnet_isdying(un))
		return;

	if ((mii->mii_media_status & (IFM_ACTIVE | IFM_AVALID)) ==
	    (IFM_ACTIVE | IFM_AVALID)) {
		switch (IFM_SUBTYPE(mii->mii_media_active)) {
		case IFM_10_T:
		case IFM_100_TX:
			usbnet_set_link(un, true);
			break;
		case IFM_1000_T:
			usbnet_set_link(un, true);
			break;
		default:
			break;
		}
	}

	/* Lost link, do nothing. */
	if (!usbnet_havelink(un))
		return;

	val = 0;
	if ((mii->mii_media_active & IFM_FDX) != 0)
		val |= AXEN_MEDIUM_FDX;

	val |= AXEN_MEDIUM_RXFLOW_CTRL_EN | AXEN_MEDIUM_TXFLOW_CTRL_EN |
	    AXEN_MEDIUM_RECV_EN;
	switch (IFM_SUBTYPE(mii->mii_media_active)) {
	case IFM_1000_T:
		val |= AXEN_MEDIUM_GIGA | AXEN_MEDIUM_EN_125MHZ;
		break;
	case IFM_100_TX:
		val |= AXEN_MEDIUM_PS;
		break;
	case IFM_10_T:
		/* doesn't need to be handled */
		break;
	}

	DPRINTF(("%s: val=%#x\n", __func__, val));
	wval = htole16(val);
	err = axen_cmd(un, AXEN_CMD_MAC_WRITE2, 2, AXEN_MEDIUM_STATUS, &wval);
	if (err)
		aprint_error_dev(un->un_dev, "media change failed\n");
}

static void
axen_setiff_locked(struct usbnet *un)
{
	struct ifnet * const ifp = usbnet_ifp(un);
	struct ethercom *ec = usbnet_ec(un);
	struct ether_multi *enm;
	struct ether_multistep step;
	uint32_t h = 0;
	uint16_t rxmode;
	uint8_t hashtbl[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	uint16_t wval;

	if (usbnet_isdying(un))
		return;

	usbnet_isowned_core(un);

	rxmode = 0;

	/* Enable receiver, set RX mode */
	axen_cmd(un, AXEN_CMD_MAC_READ2, 2, AXEN_MAC_RXCTL, &wval);
	rxmode = le16toh(wval);
	rxmode &= ~(AXEN_RXCTL_ACPT_ALL_MCAST | AXEN_RXCTL_PROMISC |
	    AXEN_RXCTL_ACPT_MCAST);

	if (ifp->if_flags & IFF_PROMISC) {
		DPRINTF(("%s: promisc\n", device_xname(un->un_dev)));
		rxmode |= AXEN_RXCTL_PROMISC;
allmulti:
		ETHER_LOCK(ec);
		ec->ec_flags |= ETHER_F_ALLMULTI;
		ETHER_UNLOCK(ec);
		rxmode |= AXEN_RXCTL_ACPT_ALL_MCAST
		/* | AXEN_RXCTL_ACPT_PHY_MCAST */;
	} else {
		/* now program new ones */
		DPRINTF(("%s: initializing hash table\n",
		    device_xname(un->un_dev)));
		ETHER_LOCK(ec);
		ec->ec_flags &= ~ETHER_F_ALLMULTI;

		ETHER_FIRST_MULTI(step, ec, enm);
		while (enm != NULL) {
			if (memcmp(enm->enm_addrlo, enm->enm_addrhi,
			    ETHER_ADDR_LEN)) {
				DPRINTF(("%s: allmulti\n",
				    device_xname(un->un_dev)));
				memset(hashtbl, 0, sizeof(hashtbl));
				ETHER_UNLOCK(ec);
				goto allmulti;
			}
			h = ether_crc32_be(enm->enm_addrlo,
			    ETHER_ADDR_LEN) >> 26;
			hashtbl[h / 8] |= 1 << (h % 8);
			DPRINTF(("%s: %s added\n",
			    device_xname(un->un_dev),
			    ether_sprintf(enm->enm_addrlo)));
			ETHER_NEXT_MULTI(step, enm);
		}
		ETHER_UNLOCK(ec);
		rxmode |= AXEN_RXCTL_ACPT_MCAST;
	}

	axen_cmd(un, AXEN_CMD_MAC_WRITE_FILTER, 8, AXEN_FILTER_MULTI, hashtbl);
	wval = htole16(rxmode);
	axen_cmd(un, AXEN_CMD_MAC_WRITE2, 2, AXEN_MAC_RXCTL, &wval);
}

static void
axen_reset(struct usbnet *un)
{
	usbnet_isowned_core(un);
	if (usbnet_isdying(un))
		return;
	/* XXX What to reset? */

	/* Wait a little while for the chip to get its brains in order. */
	DELAY(1000);
}

static int
axen_get_eaddr(struct usbnet *un, void *addr)
{
#if 1
	return axen_cmd(un, AXEN_CMD_MAC_READ_ETHER, 6, AXEN_CMD_MAC_NODE_ID,
	    addr);
#else
	int i, retry;
	uint8_t eeprom[20];
	uint16_t csum;
	uint16_t buf;

	for (i = 0; i < 6; i++) {
		/* set eeprom address */
		buf = htole16(i);
		axen_cmd(un, AXEN_CMD_MAC_WRITE, 1, AXEN_MAC_EEPROM_ADDR, &buf);

		/* set eeprom command */
		buf = htole16(AXEN_EEPROM_READ);
		axen_cmd(un, AXEN_CMD_MAC_WRITE, 1, AXEN_MAC_EEPROM_CMD, &buf);

		/* check the value is ready */
		retry = 3;
		do {
			buf = htole16(AXEN_EEPROM_READ);
			usbd_delay_ms(un->un_udev, 10);
			axen_cmd(un, AXEN_CMD_MAC_READ, 1, AXEN_MAC_EEPROM_CMD,
			    &buf);
			retry--;
			if (retry < 0)
				return EINVAL;
		} while ((le16toh(buf) & 0xff) & AXEN_EEPROM_BUSY);

		/* read data */
		axen_cmd(un, AXEN_CMD_MAC_READ2, 2, AXEN_EEPROM_READ,
		    &eeprom[i * 2]);

		/* sanity check */
		if ((i == 0) && (eeprom[0] == 0xff))
			return EINVAL;
	}

	/* check checksum */
	csum = eeprom[6] + eeprom[7] + eeprom[8] + eeprom[9];
	csum = (csum >> 8) + (csum & 0xff) + eeprom[10];
	if (csum != 0xff) {
		printf("eeprom checksum mismatch(0x%02x)\n", csum);
		return EINVAL;
	}

	memcpy(addr, eeprom, ETHER_ADDR_LEN);
	return 0;
#endif
}

static void
axen_ax88179_init(struct usbnet *un)
{
	struct axen_qctrl qctrl;
	uint16_t ctl, temp;
	uint16_t wval;
	uint8_t val;

	usbnet_lock_core(un);
	usbnet_busy(un);

	/* XXX: ? */
	axen_cmd(un, AXEN_CMD_MAC_READ, 1, AXEN_UNK_05, &val);
	DPRINTFN(5, ("AXEN_CMD_MAC_READ(0x05): 0x%02x\n", val));

	/* check AX88179 version, UA1 / UA2 */
	axen_cmd(un, AXEN_CMD_MAC_READ, 1, AXEN_GENERAL_STATUS, &val);
	/* UA1 */
	if (!(val & AXEN_GENERAL_STATUS_MASK)) {
		DPRINTF(("AX88179 ver. UA1\n"));
	} else {
		DPRINTF(("AX88179 ver. UA2\n"));
	}

	/* power up ethernet PHY */
	wval = htole16(0);
	axen_cmd(un, AXEN_CMD_MAC_WRITE2, 2, AXEN_PHYPWR_RSTCTL, &wval);

	wval = htole16(AXEN_PHYPWR_RSTCTL_IPRL);
	axen_cmd(un, AXEN_CMD_MAC_WRITE2, 2, AXEN_PHYPWR_RSTCTL, &wval);
	usbd_delay_ms(un->un_udev, 200);

	/* set clock mode */
	val = AXEN_PHYCLK_ACS | AXEN_PHYCLK_BCS;
	axen_cmd(un, AXEN_CMD_MAC_WRITE, 1, AXEN_PHYCLK, &val);
	usbd_delay_ms(un->un_udev, 100);

	/* set monitor mode (disable) */
	val = AXEN_MONITOR_NONE;
	axen_cmd(un, AXEN_CMD_MAC_WRITE, 1, AXEN_MONITOR_MODE, &val);

	/* enable auto detach */
	axen_cmd(un, AXEN_CMD_EEPROM_READ, 2, AXEN_EEPROM_STAT, &wval);
	temp = le16toh(wval);
	DPRINTFN(2,("EEPROM0x43 = 0x%04x\n", temp));
	if (!(temp == 0xffff) && !(temp & 0x0100)) {
		/* Enable auto detach bit */
		val = 0;
		axen_cmd(un, AXEN_CMD_MAC_WRITE, 1, AXEN_PHYCLK, &val);
		val = AXEN_PHYCLK_ULR;
		axen_cmd(un, AXEN_CMD_MAC_WRITE, 1, AXEN_PHYCLK, &val);
		usbd_delay_ms(un->un_udev, 100);

		axen_cmd(un, AXEN_CMD_MAC_READ2, 2, AXEN_PHYPWR_RSTCTL, &wval);
		ctl = le16toh(wval);
		ctl |= AXEN_PHYPWR_RSTCTL_AUTODETACH;
		wval = htole16(ctl);
		axen_cmd(un, AXEN_CMD_MAC_WRITE2, 2, AXEN_PHYPWR_RSTCTL, &wval);
		usbd_delay_ms(un->un_udev, 200);
		aprint_error_dev(un->un_dev, "enable auto detach (0x%04x)\n",
		    ctl);
	}

	/* bulkin queue setting */
	axen_cmd(un, AXEN_CMD_MAC_READ, 1, AXEN_USB_UPLINK, &val);
	switch (val) {
	case AXEN_USB_FS:
		DPRINTF(("uplink: USB1.1\n"));
		qctrl.ctrl	 = 0x07;
		qctrl.timer_low	 = 0xcc;
		qctrl.timer_high = 0x4c;
		qctrl.bufsize	 = AXEN_BUFSZ_LS - 1;
		qctrl.ifg	 = 0x08;
		break;
	case AXEN_USB_HS:
		DPRINTF(("uplink: USB2.0\n"));
		qctrl.ctrl	 = 0x07;
		qctrl.timer_low	 = 0x02;
		qctrl.timer_high = 0xa0;
		qctrl.bufsize	 = AXEN_BUFSZ_HS - 1;
		qctrl.ifg	 = 0xff;
		break;
	case AXEN_USB_SS:
		DPRINTF(("uplink: USB3.0\n"));
		qctrl.ctrl	 = 0x07;
		qctrl.timer_low	 = 0x4f;
		qctrl.timer_high = 0x00;
		qctrl.bufsize	 = AXEN_BUFSZ_SS - 1;
		qctrl.ifg	 = 0xff;
		break;
	default:
		aprint_error_dev(un->un_dev, "unknown uplink bus:0x%02x\n",
		    val);
		usbnet_unbusy(un);
		usbnet_unlock_core(un);
		return;
	}
	axen_cmd(un, AXEN_CMD_MAC_SET_RXSR, 5, AXEN_RX_BULKIN_QCTRL, &qctrl);

	/*
	 * set buffer high/low watermark to pause/resume.
	 * write 2byte will set high/log simultaneous with AXEN_PAUSE_HIGH.
	 * XXX: what is the best value? OSX driver uses 0x3c-0x4c as LOW-HIGH
	 * watermark parameters.
	 */
	val = 0x34;
	axen_cmd(un, AXEN_CMD_MAC_WRITE, 1, AXEN_PAUSE_LOW_WATERMARK, &val);
	val = 0x52;
	axen_cmd(un, AXEN_CMD_MAC_WRITE, 1, AXEN_PAUSE_HIGH_WATERMARK, &val);

	/* Set RX/TX configuration. */
	/* Set RX control register */
	ctl = AXEN_RXCTL_IPE | AXEN_RXCTL_DROPCRCERR | AXEN_RXCTL_AUTOB;
	wval = htole16(ctl);
	axen_cmd(un, AXEN_CMD_MAC_WRITE2, 2, AXEN_MAC_RXCTL, &wval);

	/* set monitor mode (enable) */
	val = AXEN_MONITOR_PMETYPE | AXEN_MONITOR_PMEPOL | AXEN_MONITOR_RWMP;
	axen_cmd(un, AXEN_CMD_MAC_WRITE, 1, AXEN_MONITOR_MODE, &val);
	axen_cmd(un, AXEN_CMD_MAC_READ, 1, AXEN_MONITOR_MODE, &val);
	DPRINTF(("axen: Monitor mode = 0x%02x\n", val));

	/* set medium type */
	ctl = AXEN_MEDIUM_GIGA | AXEN_MEDIUM_FDX | AXEN_MEDIUM_EN_125MHZ |
	    AXEN_MEDIUM_RXFLOW_CTRL_EN | AXEN_MEDIUM_TXFLOW_CTRL_EN |
	    AXEN_MEDIUM_RECV_EN;
	wval = htole16(ctl);
	DPRINTF(("axen: set to medium mode: 0x%04x\n", ctl));
	axen_cmd(un, AXEN_CMD_MAC_WRITE2, 2, AXEN_MEDIUM_STATUS, &wval);
	usbd_delay_ms(un->un_udev, 100);

	axen_cmd(un, AXEN_CMD_MAC_READ2, 2, AXEN_MEDIUM_STATUS, &wval);
	DPRINTF(("axen: current medium mode: 0x%04x\n", le16toh(wval)));

#if 0 /* XXX: TBD.... */
#define GMII_LED_ACTIVE		0x1a
#define GMII_PHY_PAGE_SEL	0x1e
#define GMII_PHY_PAGE_SEL	0x1f
#define GMII_PAGE_EXT		0x0007
	usbnet_mii_writereg(un->un_dev, un->un_phyno, GMII_PHY_PAGE_SEL,
	    GMII_PAGE_EXT);
	usbnet_mii_writereg(un->un_dev, un->un_phyno, GMII_PHY_PAGE,
	    0x002c);
#endif

#if 1 /* XXX: phy hack ? */
	usbnet_mii_writereg(un->un_dev, un->un_phyno, 0x1F, 0x0005);
	usbnet_mii_writereg(un->un_dev, un->un_phyno, 0x0C, 0x0000);
	usbnet_mii_readreg(un->un_dev, un->un_phyno, 0x0001, &wval);
	usbnet_mii_writereg(un->un_dev, un->un_phyno, 0x01, wval | 0x0080);
	usbnet_mii_writereg(un->un_dev, un->un_phyno, 0x1F, 0x0000);
#endif

	usbnet_unbusy(un);
	usbnet_unlock_core(un);
}

static void
axen_setoe_locked(struct usbnet *un)
{
	struct ifnet * const ifp = usbnet_ifp(un);
	uint64_t enabled = ifp->if_capenable;
	uint8_t val;

	usbnet_isowned_core(un);

	val = AXEN_RXCOE_OFF;
	if (enabled & IFCAP_CSUM_IPv4_Rx)
		val |= AXEN_RXCOE_IPv4;
	if (enabled & IFCAP_CSUM_TCPv4_Rx)
		val |= AXEN_RXCOE_TCPv4;
	if (enabled & IFCAP_CSUM_UDPv4_Rx)
		val |= AXEN_RXCOE_UDPv4;
	if (enabled & IFCAP_CSUM_TCPv6_Rx)
		val |= AXEN_RXCOE_TCPv6;
	if (enabled & IFCAP_CSUM_UDPv6_Rx)
		val |= AXEN_RXCOE_UDPv6;
	axen_cmd(un, AXEN_CMD_MAC_WRITE, 1, AXEN_RX_COE, &val);

	val = AXEN_TXCOE_OFF;
	if (enabled & IFCAP_CSUM_IPv4_Tx)
		val |= AXEN_TXCOE_IPv4;
	if (enabled & IFCAP_CSUM_TCPv4_Tx)
		val |= AXEN_TXCOE_TCPv4;
	if (enabled & IFCAP_CSUM_UDPv4_Tx)
		val |= AXEN_TXCOE_UDPv4;
	if (enabled & IFCAP_CSUM_TCPv6_Tx)
		val |= AXEN_TXCOE_TCPv6;
	if (enabled & IFCAP_CSUM_UDPv6_Tx)
		val |= AXEN_TXCOE_UDPv6;
	axen_cmd(un, AXEN_CMD_MAC_WRITE, 1, AXEN_TX_COE, &val);
}

static int
axen_uno_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	struct usbnet * const un = ifp->if_softc;

	usbnet_lock_core(un);
	usbnet_busy(un);

	switch (cmd) {
	case SIOCSIFCAP:
		axen_setoe_locked(un);
		break;
	default:
		break;
	}

	usbnet_unbusy(un);
	usbnet_unlock_core(un);

	return 0;
}

static void
axen_uno_mcast(struct ifnet *ifp)
{
	struct usbnet * const un = ifp->if_softc;

	usbnet_lock_core(un);
	usbnet_busy(un);

	axen_setiff_locked(un);

	usbnet_unbusy(un);
	usbnet_unlock_core(un);
}

static int
axen_match(device_t parent, cfdata_t match, void *aux)
{
	struct usb_attach_arg *uaa = aux;

	return axen_lookup(uaa->uaa_vendor, uaa->uaa_product) != NULL ?
		UMATCH_VENDOR_PRODUCT : UMATCH_NONE;
}

static void
axen_attach(device_t parent, device_t self, void *aux)
{
	USBNET_MII_DECL_DEFAULT(unm);
	struct usbnet * const un = device_private(self);
	struct usb_attach_arg *uaa = aux;
	struct usbd_device *dev = uaa->uaa_device;
	usbd_status err;
	usb_interface_descriptor_t *id;
	usb_endpoint_descriptor_t *ed;
	char *devinfop;
	uint16_t axen_flags;
	int i;

	aprint_naive("\n");
	aprint_normal("\n");
	devinfop = usbd_devinfo_alloc(dev, 0);
	aprint_normal_dev(self, "%s\n", devinfop);
	usbd_devinfo_free(devinfop);

	un->un_dev = self;
	un->un_udev = dev;
	un->un_sc = un;
	un->un_ops = &axen_ops;
	un->un_rx_xfer_flags = USBD_SHORT_XFER_OK;
	un->un_tx_xfer_flags = USBD_FORCE_SHORT_XFER;
	un->un_rx_list_cnt = AXEN_RX_LIST_CNT;
	un->un_tx_list_cnt = AXEN_TX_LIST_CNT;

	err = usbd_set_config_no(dev, AXEN_CONFIG_NO, 1);
	if (err) {
		aprint_error_dev(self, "failed to set configuration"
		    ", err=%s\n", usbd_errstr(err));
		return;
	}

	axen_flags = axen_lookup(uaa->uaa_vendor, uaa->uaa_product)->axen_flags;

	err = usbd_device2interface_handle(dev, AXEN_IFACE_IDX, &un->un_iface);
	if (err) {
		aprint_error_dev(self, "getting interface handle failed\n");
		return;
	}

	/* decide on what our bufsize will be */
	switch (dev->ud_speed) {
	case USB_SPEED_SUPER:
		un->un_rx_bufsz = AXEN_BUFSZ_SS * 1024;
		break;
	case USB_SPEED_HIGH:
		un->un_rx_bufsz = AXEN_BUFSZ_HS * 1024;
		break;
	default:
		un->un_rx_bufsz = AXEN_BUFSZ_LS * 1024;
		break;
	}
	un->un_tx_bufsz = IP_MAXPACKET + ETHER_HDR_LEN + ETHER_CRC_LEN +
	    ETHER_VLAN_ENCAP_LEN + sizeof(struct axen_sframe_hdr);

	/* Find endpoints. */
	id = usbd_get_interface_descriptor(un->un_iface);
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
#if 0 /* not used yet */
		} else if (UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_IN &&
			   UE_GET_XFERTYPE(ed->bmAttributes) == UE_INTERRUPT) {
			un->un_ed[USBNET_ENDPT_INTR] = ed->bEndpointAddress;
#endif
		}
	}

	/* Set these up now for axen_cmd().  */
	usbnet_attach(un, "axendet");

	un->un_phyno = AXEN_PHY_ID;
	DPRINTF(("%s: phyno %d\n", device_xname(self), un->un_phyno));

	/* Get station address.  */
	usbnet_lock_core(un);
	usbnet_busy(un);
	if (axen_get_eaddr(un, &un->un_eaddr)) {
		usbnet_unbusy(un);
		usbnet_unlock_core(un);
		printf("EEPROM checksum error\n");
		return;
	}
	usbnet_unbusy(un);
	usbnet_unlock_core(un);

	axen_ax88179_init(un);

	/* An ASIX chip was detected. Inform the world.  */
	if (axen_flags & AX178A)
		aprint_normal_dev(self, "AX88178a\n");
	else if (axen_flags & AX179)
		aprint_normal_dev(self, "AX88179\n");
	else
		aprint_normal_dev(self, "(unknown)\n");

	struct ethercom *ec = usbnet_ec(un);
	ec->ec_capabilities = ETHERCAP_VLAN_MTU;

	/* Adapter does not support TSOv6 (They call it LSOv2). */
	struct ifnet *ifp = usbnet_ifp(un);
	ifp->if_capabilities |= IFCAP_TSOv4 |
	    IFCAP_CSUM_IPv4_Rx	| IFCAP_CSUM_IPv4_Tx  |
	    IFCAP_CSUM_TCPv4_Rx | IFCAP_CSUM_TCPv4_Tx |
	    IFCAP_CSUM_UDPv4_Rx | IFCAP_CSUM_UDPv4_Tx |
	    IFCAP_CSUM_TCPv6_Rx | IFCAP_CSUM_TCPv6_Tx |
	    IFCAP_CSUM_UDPv6_Rx | IFCAP_CSUM_UDPv6_Tx;

	usbnet_attach_ifp(un, IFF_SIMPLEX | IFF_BROADCAST | IFF_MULTICAST,
	    0, &unm);
}

static int
axen_csum_flags_rx(struct ifnet *ifp, uint32_t pkt_hdr)
{
	int enabled_flags = ifp->if_csum_flags_rx;
	int csum_flags = 0;
	int l3_type, l4_type;

	if (enabled_flags == 0)
		return 0;

	l3_type = (pkt_hdr & AXEN_RXHDR_L3_TYPE_MASK) >>
	    AXEN_RXHDR_L3_TYPE_OFFSET;

	if (l3_type == AXEN_RXHDR_L3_TYPE_IPV4)
		csum_flags |= M_CSUM_IPv4;

	l4_type = (pkt_hdr & AXEN_RXHDR_L4_TYPE_MASK) >>
	    AXEN_RXHDR_L4_TYPE_OFFSET;

	switch (l4_type) {
	case AXEN_RXHDR_L4_TYPE_TCP:
		if (l3_type == AXEN_RXHDR_L3_TYPE_IPV4)
			csum_flags |= M_CSUM_TCPv4;
		else
			csum_flags |= M_CSUM_TCPv6;
		break;
	case AXEN_RXHDR_L4_TYPE_UDP:
		if (l3_type == AXEN_RXHDR_L3_TYPE_IPV4)
			csum_flags |= M_CSUM_UDPv4;
		else
			csum_flags |= M_CSUM_UDPv6;
		break;
	default:
		break;
	}

	csum_flags &= enabled_flags;
	if ((csum_flags & M_CSUM_IPv4) && (pkt_hdr & AXEN_RXHDR_L3CSUM_ERR))
		csum_flags |= M_CSUM_IPv4_BAD;
	if ((csum_flags & ~M_CSUM_IPv4) && (pkt_hdr & AXEN_RXHDR_L4CSUM_ERR))
		csum_flags |= M_CSUM_TCP_UDP_BAD;

	return csum_flags;
}

static void
axen_uno_rx_loop(struct usbnet *un, struct usbnet_chain *c, uint32_t total_len)
{
	struct ifnet *ifp = usbnet_ifp(un);
	uint8_t *buf = c->unc_buf;
	uint32_t rx_hdr, pkt_hdr;
	uint32_t *hdr_p;
	uint16_t hdr_offset, pkt_count;
	size_t pkt_len;
	size_t temp;

	if (total_len < sizeof(pkt_hdr)) {
		aprint_error_dev(un->un_dev, "rxeof: too short transfer\n");
		if_statinc(ifp, if_ierrors);
		return;
	}

	/*
	 * buffer map
	 * [packet #0]...[packet #n][pkt hdr#0]..[pkt hdr#n][recv_hdr]
	 * each packet has 0xeeee as psuedo header..
	 */
	hdr_p = (uint32_t *)(buf + total_len - sizeof(uint32_t));
	rx_hdr = le32toh(*hdr_p);
	hdr_offset = (uint16_t)(rx_hdr >> 16);
	pkt_count  = (uint16_t)(rx_hdr & 0xffff);

	/* sanity check */
	if (hdr_offset > total_len) {
		aprint_error_dev(un->un_dev,
		    "rxeof: invalid hdr offset (%u > %u)\n",
		    hdr_offset, total_len);
		if_statinc(ifp, if_ierrors);
		usbd_delay_ms(un->un_udev, 100);
		return;
	}

	/* point first packet header */
	hdr_p = (uint32_t *)(buf + hdr_offset);

	/*
	 * ax88179 will pack multiple ip packet to a USB transaction.
	 * process all of packets in the buffer
	 */

#if 1 /* XXX: paranoiac check. need to remove later */
#define AXEN_MAX_PACKED_PACKET 200
	if (pkt_count > AXEN_MAX_PACKED_PACKET) {
		DPRINTF(("%s: Too many packets (%d) in a transaction, discard.\n",
		    device_xname(un->un_dev), pkt_count));
		return;
	}
#endif

	if (pkt_count)
		rnd_add_uint32(usbnet_rndsrc(un), pkt_count);

	do {
		if ((buf[0] != 0xee) || (buf[1] != 0xee)) {
			aprint_error_dev(un->un_dev,
			    "invalid buffer(pkt#%d), continue\n", pkt_count);
			if_statadd(ifp, if_ierrors, pkt_count);
			return;
		}

		pkt_hdr = le32toh(*hdr_p);
		pkt_len = (pkt_hdr >> 16) & 0x1fff;
		DPRINTFN(10,
		    ("%s: rxeof: packet#%d, pkt_hdr 0x%08x, pkt_len %zu\n",
		   device_xname(un->un_dev), pkt_count, pkt_hdr, pkt_len));

		if (pkt_hdr & (AXEN_RXHDR_CRC_ERR | AXEN_RXHDR_DROP_ERR)) {
			if_statinc(ifp, if_ierrors);
			/* move to next pkt header */
			DPRINTF(("%s: %s err (pkt#%d)\n",
			    device_xname(un->un_dev),
			    (pkt_hdr & AXEN_RXHDR_CRC_ERR) ? "crc" : "drop",
			    pkt_count));
			goto nextpkt;
		}

		usbnet_enqueue(un, buf + ETHER_ALIGN, pkt_len - 6,
			       axen_csum_flags_rx(ifp, pkt_hdr), 0, 0);

nextpkt:
		/*
		 * prepare next packet
		 * as each packet will be aligned 8byte boundary,
		 * need to fix up the start point of the buffer.
		 */
		temp = ((pkt_len + 7) & 0xfff8);
		buf = buf + temp;
		hdr_p++;
		pkt_count--;
	} while (pkt_count > 0);
}

static unsigned
axen_uno_tx_prepare(struct usbnet *un, struct mbuf *m, struct usbnet_chain *c)
{
	struct axen_sframe_hdr hdr;
	u_int length, boundary;

	if ((unsigned)m->m_pkthdr.len > un->un_tx_bufsz - sizeof(hdr))
		return 0;
	length = m->m_pkthdr.len + sizeof(hdr);

	/* XXX Is this needed?  wMaxPacketSize? */
	switch (un->un_udev->ud_speed) {
	case USB_SPEED_SUPER:
		boundary = 4096;
		break;
	case USB_SPEED_HIGH:
		boundary = 512;
		break;
	default:
		boundary = 64;
		break;
	}

	hdr.plen = htole32(m->m_pkthdr.len);

	hdr.gso = (m->m_pkthdr.csum_flags & M_CSUM_TSOv4) ?
	    m->m_pkthdr.segsz : 0;
	if ((length % boundary) == 0) {
		DPRINTF(("%s: boundary hit\n", device_xname(un->un_dev)));
		hdr.gso |= 0x80008000;	/* XXX enable padding */
	}
	hdr.gso = htole32(hdr.gso);

	memcpy(c->unc_buf, &hdr, sizeof(hdr));
	m_copydata(m, 0, m->m_pkthdr.len, c->unc_buf + sizeof(hdr));

	return length;
}

static int
axen_init_locked(struct ifnet *ifp)
{
	struct usbnet * const un = ifp->if_softc;
	uint16_t rxmode;
	uint16_t wval;
	uint8_t bval;

	usbnet_isowned_core(un);

	if (usbnet_isdying(un))
		return EIO;

	/* Cancel pending I/O */
	usbnet_stop(un, ifp, 1);

	/* Reset the ethernet interface. */
	axen_reset(un);

	/* XXX: ? */
	bval = 0x01;
	axen_cmd(un, AXEN_CMD_MAC_WRITE, 1, AXEN_UNK_28, &bval);

	/* Configure offloading engine. */
	axen_setoe_locked(un);

	/* Program promiscuous mode and multicast filters. */
	axen_setiff_locked(un);

	/* Enable receiver, set RX mode */
	axen_cmd(un, AXEN_CMD_MAC_READ2, 2, AXEN_MAC_RXCTL, &wval);
	rxmode = le16toh(wval);
	rxmode |= AXEN_RXCTL_START;
	wval = htole16(rxmode);
	axen_cmd(un, AXEN_CMD_MAC_WRITE2, 2, AXEN_MAC_RXCTL, &wval);

	return usbnet_init_rx_tx(un);
}

static int
axen_uno_init(struct ifnet *ifp)
{
	int ret = axen_init_locked(ifp);

	return ret;
}

static void
axen_uno_stop(struct ifnet *ifp, int disable)
{
	struct usbnet * const un = ifp->if_softc;
	uint16_t rxmode, wval;

	axen_reset(un);

	/* Disable receiver, set RX mode */
	axen_cmd(un, AXEN_CMD_MAC_READ2, 2, AXEN_MAC_RXCTL, &wval);
	rxmode = le16toh(wval);
	rxmode &= ~AXEN_RXCTL_START;
	wval = htole16(rxmode);
	axen_cmd(un, AXEN_CMD_MAC_WRITE2, 2, AXEN_MAC_RXCTL, &wval);
}

#ifdef _MODULE
#include "ioconf.c"
#endif

USBNET_MODULE(axen)
