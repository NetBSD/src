/*	$NetBSD: if_axen.c,v 1.44 2019/06/21 14:19:46 mrg Exp $	*/
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
__KERNEL_RCSID(0, "$NetBSD: if_axen.c,v 1.44 2019/06/21 14:19:46 mrg Exp $");

#ifdef _KERNEL_OPT
#include "opt_inet.h"
#include "opt_usb.h"
#endif

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/mbuf.h>
#include <sys/module.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/systm.h>

#include <sys/rndsource.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#include <net/bpf.h>

#include <netinet/in.h>		/* XXX for netinet/ip.h */
#include <netinet/ip.h>		/* XXX for IP_MAXPACKET */

#include <dev/mii/mii.h>
#include <dev/mii/miivar.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdi_util.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usbdevs.h>

#include <dev/usb/if_axenreg.h>

#ifdef AXEN_DEBUG
#define DPRINTF(x)	do { if (axendebug) printf x; } while (/*CONSTCOND*/0)
#define DPRINTFN(n, x)	do { if (axendebug >= (n)) printf x; } while (/*CONSTCOND*/0)
int	axendebug = 0;
#else
#define DPRINTF(x)
#define DPRINTFN(n, x)
#endif

struct axen_softc;

struct axen_chain {
	struct axen_softc	*axen_sc;
	struct usbd_xfer	*axen_xfer;
	uint8_t			*axen_buf;
};

struct axen_cdata {
	struct axen_chain	axen_tx_chain[AXEN_TX_LIST_CNT];
	struct axen_chain	axen_rx_chain[AXEN_RX_LIST_CNT];
	int			axen_tx_prod;
	int			axen_tx_cnt;
};

struct axen_softc {
	device_t		axen_dev;
	struct ethercom		axen_ec;
	struct mii_data		axen_mii;
	krndsource_t		rnd_source;
	struct usbd_device *	axen_udev;
	struct usbd_interface *	axen_iface;

	uint16_t		axen_vendor;
	uint16_t		axen_product;
	uint16_t		axen_flags;

	uint16_t		axen_timer;

	int			axen_ed[AXEN_ENDPT_MAX];
	struct usbd_pipe	*axen_ep[AXEN_ENDPT_MAX];
	int			axen_if_flags;
	struct axen_cdata	axen_cdata;
	struct callout		axen_stat_ch;

	int			axen_refcnt;
	bool			axen_dying;
	bool			axen_stopping;
	bool			axen_attached;

	struct usb_task		axen_tick_task;

	kmutex_t		axen_lock;
	kmutex_t		axen_mii_lock;
	kmutex_t		axen_rxlock;
	kmutex_t		axen_txlock;
	kcondvar_t		axen_detachcv;

	int			axen_link;

	int			axen_phyno;
	struct timeval		axen_rx_notice;
	struct timeval		axen_tx_notice;
	u_int			axen_rx_bufsz;
	u_int			axen_tx_bufsz;
	int			axen_rev;

#define sc_if	axen_ec.ec_if
};

#define GET_MII(sc) (&(sc)->axen_mii)
#define GET_IFP(sc) (&(sc)->sc_if)

struct axen_type {
	struct usb_devno	axen_dev;
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
static int	axen_detach(device_t, int);
static int	axen_activate(device_t, devact_t);

CFATTACH_DECL_NEW(axen, sizeof(struct axen_softc),
	axen_match, axen_attach, axen_detach, axen_activate);

static int	axen_tx_list_init(struct axen_softc *);
static int	axen_rx_list_init(struct axen_softc *);
static struct mbuf *axen_newbuf(void);
static int	axen_encap(struct axen_softc *, struct mbuf *, int);
static void	axen_rxeof(struct usbd_xfer *, void *, usbd_status);
static int	axen_csum_flags_rx(struct ifnet *, uint32_t);
static void	axen_txeof(struct usbd_xfer *, void *, usbd_status);
static void	axen_tick(void *);
static void	axen_tick_task(void *);
static void	axen_start(struct ifnet *);
static void	axen_start_locked(struct ifnet *);
static int	axen_ioctl(struct ifnet *, u_long, void *);
static int	axen_init(struct ifnet *);
static void	axen_stop(struct ifnet *, int);
static void	axen_stop_locked(struct ifnet *, int);
static void	axen_watchdog(struct ifnet *);
static int	axen_miibus_readreg(device_t, int, int, uint16_t *);
static int	axen_miibus_writereg(device_t, int, int, uint16_t);
static void	axen_miibus_statchg(struct ifnet *);
static int	axen_cmd(struct axen_softc *, int, int, int, void *);
static int	axen_ifmedia_upd(struct ifnet *);
static void	axen_ifmedia_sts(struct ifnet *, struct ifmediareq *);
static void	axen_reset(struct axen_softc *);
static int	axen_get_eaddr(struct axen_softc *, void *);
static void	axen_iff(struct axen_softc *);
static void	axen_ax88179_init(struct axen_softc *);
static void	axen_setcoe(struct axen_softc *);

/*
 * Access functions for MII.  Take the MII lock to call axen_cmd().
 * Two forms: softc lock currently held or not.
 */
static void
axen_lock_mii(struct axen_softc *sc)
{

	mutex_enter(&sc->axen_lock);
	sc->axen_refcnt++;
	mutex_exit(&sc->axen_lock);

	mutex_enter(&sc->axen_mii_lock);
}

static void
axen_lock_mii_sc_locked(struct axen_softc *sc)
{
	KASSERT(mutex_owned(&sc->axen_lock));

	sc->axen_refcnt++;
	mutex_enter(&sc->axen_mii_lock);
}

static void
axen_unlock_mii(struct axen_softc *sc)
{

	mutex_exit(&sc->axen_mii_lock);
	mutex_enter(&sc->axen_lock);
	if (--sc->axen_refcnt < 0)
		cv_broadcast(&sc->axen_detachcv);
	mutex_exit(&sc->axen_lock);
}

static void
axen_unlock_mii_sc_locked(struct axen_softc *sc)
{
	KASSERT(mutex_owned(&sc->axen_lock));

	mutex_exit(&sc->axen_mii_lock);
	if (--sc->axen_refcnt < 0)
		cv_broadcast(&sc->axen_detachcv);
}

static int
axen_cmd(struct axen_softc *sc, int cmd, int index, int val, void *buf)
{
	usb_device_request_t req;
	usbd_status err;

	KASSERT(mutex_owned(&sc->axen_mii_lock));

	if (sc->axen_dying)
		return 0;

	if (AXEN_CMD_DIR(cmd))
		req.bmRequestType = UT_WRITE_VENDOR_DEVICE;
	else
		req.bmRequestType = UT_READ_VENDOR_DEVICE;
	req.bRequest = AXEN_CMD_CMD(cmd);
	USETW(req.wValue, val);
	USETW(req.wIndex, index);
	USETW(req.wLength, AXEN_CMD_LEN(cmd));

	err = usbd_do_request(sc->axen_udev, &req, buf);
	DPRINTFN(5, ("axen_cmd: cmd 0x%04x val 0x%04x len %d\n",
	    cmd, val, AXEN_CMD_LEN(cmd)));

	if (err) {
		DPRINTF(("%s: cmd: %d, error: %d\n", __func__, cmd, err));
		return -1;
	}

	return 0;
}

static int
axen_miibus_readreg(device_t dev, int phy, int reg, uint16_t *val)
{
	struct axen_softc * const sc = device_private(dev);
	usbd_status err;
	uint16_t data;

	mutex_enter(&sc->axen_lock);
	if (sc->axen_dying || sc->axen_phyno != phy) {
		mutex_exit(&sc->axen_lock);
		return -1;
	}
	mutex_exit(&sc->axen_lock);

	axen_lock_mii(sc);
	err = axen_cmd(sc, AXEN_CMD_MII_READ_REG, reg, phy, &data);
	axen_unlock_mii(sc);

	if (err) {
		aprint_error_dev(sc->axen_dev, "read PHY failed: %d\n", err);
		return err;
	}

	*val = le16toh(data);
	DPRINTFN(2,("axen_miibus_readreg: phy 0x%x reg 0x%x val 0x%hx\n",
	    phy, reg, *val));

	if (reg == MII_BMSR) {
		*val &= ~BMSR_EXTCAP;
	}

	return 0;
}

static int
axen_miibus_writereg(device_t dev, int phy, int reg, uint16_t val)
{
	struct axen_softc * const sc = device_private(dev);
	usbd_status err;
	uint16_t uval;

	mutex_enter(&sc->axen_lock);
	if (sc->axen_dying || sc->axen_phyno != phy) {
		mutex_exit(&sc->axen_lock);
		return -1;
	}
	mutex_exit(&sc->axen_lock);

	uval = htole16(val);

	axen_lock_mii(sc);
	err = axen_cmd(sc, AXEN_CMD_MII_WRITE_REG, reg, phy, &uval);
	axen_unlock_mii(sc);

	DPRINTFN(2, ("axen_miibus_writereg: phy 0x%x reg 0x%x val 0x%04hx\n",
	    phy, reg, val));

	if (err) {
		aprint_error_dev(sc->axen_dev, "write PHY failed: %d\n", err);
		return err;
	}

	return 0;
}

static void
axen_miibus_statchg(struct ifnet *ifp)
{
	struct axen_softc * const sc = ifp->if_softc;
	struct mii_data *mii = GET_MII(sc);
	int err;
	uint16_t val;
	uint16_t wval;

	if (sc->axen_dying)
		return;

	sc->axen_link = 0;
	if ((mii->mii_media_status & (IFM_ACTIVE | IFM_AVALID)) ==
	    (IFM_ACTIVE | IFM_AVALID)) {
		switch (IFM_SUBTYPE(mii->mii_media_active)) {
		case IFM_10_T:
		case IFM_100_TX:
			sc->axen_link++;
			break;
		case IFM_1000_T:
			sc->axen_link++;
			break;
		default:
			break;
		}
	}

	/* Lost link, do nothing. */
	if (sc->axen_link == 0)
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

	DPRINTF(("%s: val=0x%x\n", __func__, val));
	wval = htole16(val);
	axen_lock_mii(sc);
	err = axen_cmd(sc, AXEN_CMD_MAC_WRITE2, 2, AXEN_MEDIUM_STATUS, &wval);
	axen_unlock_mii(sc);
	if (err) {
		aprint_error_dev(sc->axen_dev, "media change failed\n");
		return;
	}
}

/*
 * Set media options.
 */
static int
axen_ifmedia_upd(struct ifnet *ifp)
{
	struct axen_softc * const sc = ifp->if_softc;
	struct mii_data *mii = GET_MII(sc);
	int rc;

	sc->axen_link = 0;

	if (mii->mii_instance) {
		struct mii_softc *miisc;

		LIST_FOREACH(miisc, &mii->mii_phys, mii_list)
			mii_phy_reset(miisc);
	}

	if ((rc = mii_mediachg(mii)) == ENXIO)
		return 0;
	return rc;
}

/*
 * Report current media status.
 */
static void
axen_ifmedia_sts(struct ifnet *ifp, struct ifmediareq *ifmr)
{
	struct axen_softc * const sc = ifp->if_softc;
	struct mii_data *mii = GET_MII(sc);

	mii_pollstat(mii);
	ifmr->ifm_active = mii->mii_media_active;
	ifmr->ifm_status = mii->mii_media_status;
}

static void
axen_iff(struct axen_softc *sc)
{
	struct ifnet *ifp = GET_IFP(sc);
	struct ethercom *ec = &sc->axen_ec;
	struct ether_multi *enm;
	struct ether_multistep step;
	uint32_t h = 0;
	uint16_t rxmode;
	uint8_t hashtbl[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	uint16_t wval;

	if (sc->axen_dying)
		return;

	KASSERT(mutex_owned(&sc->axen_mii_lock));

	rxmode = 0;

	/* Enable receiver, set RX mode */
	axen_cmd(sc, AXEN_CMD_MAC_READ2, 2, AXEN_MAC_RXCTL, &wval);
	rxmode = le16toh(wval);
	rxmode &= ~(AXEN_RXCTL_ACPT_ALL_MCAST | AXEN_RXCTL_PROMISC |
	    AXEN_RXCTL_ACPT_MCAST);
	ifp->if_flags &= ~IFF_ALLMULTI;

	if (ifp->if_flags & IFF_PROMISC) {
		DPRINTF(("%s: promisc\n", device_xname(sc->axen_dev)));
		rxmode |= AXEN_RXCTL_PROMISC;
allmulti:	ifp->if_flags |= IFF_ALLMULTI;
		rxmode |= AXEN_RXCTL_ACPT_ALL_MCAST
		/* | AXEN_RXCTL_ACPT_PHY_MCAST */;
	} else {
		/* now program new ones */
		DPRINTF(("%s: initializing hash table\n",
		    device_xname(sc->axen_dev)));
		ETHER_LOCK(ec);
		ETHER_FIRST_MULTI(step, ec, enm);
		while (enm != NULL) {
			if (memcmp(enm->enm_addrlo, enm->enm_addrhi,
			    ETHER_ADDR_LEN)) {
				DPRINTF(("%s: allmulti\n",
				    device_xname(sc->axen_dev)));
				memset(hashtbl, 0, sizeof(hashtbl));
				ETHER_UNLOCK(ec);
				goto allmulti;
			}
			h = ether_crc32_be(enm->enm_addrlo,
			    ETHER_ADDR_LEN) >> 26;
			hashtbl[h / 8] |= 1 << (h % 8);
			DPRINTF(("%s: %s added\n",
			    device_xname(sc->axen_dev),
			    ether_sprintf(enm->enm_addrlo)));
			ETHER_NEXT_MULTI(step, enm);
		}
		ETHER_UNLOCK(ec);
		rxmode |= AXEN_RXCTL_ACPT_MCAST;
	}

	axen_cmd(sc, AXEN_CMD_MAC_WRITE_FILTER, 8, AXEN_FILTER_MULTI, hashtbl);
	wval = htole16(rxmode);
	axen_cmd(sc, AXEN_CMD_MAC_WRITE2, 2, AXEN_MAC_RXCTL, &wval);
}

static void
axen_reset(struct axen_softc *sc)
{

	KASSERT(mutex_owned(&sc->axen_lock));
	if (sc->axen_dying)
		return;
	/* XXX What to reset? */

	/* Wait a little while for the chip to get its brains in order. */
	DELAY(1000);
}

#define AXEN_GPIO_WRITE(x, y) do {				\
	axen_cmd(sc, AXEN_CMD_WRITE_GPIO, 0, (x), NULL);	\
	usbd_delay_ms(sc->axen_udev, (y));			\
} while (/*CONSTCOND*/0)

static int
axen_get_eaddr(struct axen_softc *sc, void *addr)
{
#if 1
	return axen_cmd(sc, AXEN_CMD_MAC_READ_ETHER, 6, AXEN_CMD_MAC_NODE_ID,
	    addr);
#else
	int i, retry;
	uint8_t eeprom[20];
	uint16_t csum;
	uint16_t buf;

	for (i = 0; i < 6; i++) {
		/* set eeprom address */
		buf = htole16(i);
		axen_cmd(sc, AXEN_CMD_MAC_WRITE, 1, AXEN_MAC_EEPROM_ADDR, &buf);

		/* set eeprom command */
		buf = htole16(AXEN_EEPROM_READ);
		axen_cmd(sc, AXEN_CMD_MAC_WRITE, 1, AXEN_MAC_EEPROM_CMD, &buf);

		/* check the value is ready */
		retry = 3;
		do {
			buf = htole16(AXEN_EEPROM_READ);
			usbd_delay_ms(sc->axen_udev, 10);
			axen_cmd(sc, AXEN_CMD_MAC_READ, 1, AXEN_MAC_EEPROM_CMD,
			    &buf);
			retry--;
			if (retry < 0)
				return EINVAL;
		} while ((le16toh(buf) & 0xff) & AXEN_EEPROM_BUSY);

		/* read data */
		axen_cmd(sc, AXEN_CMD_MAC_READ2, 2, AXEN_EEPROM_READ,
		    &eeprom[i * 2]);

		/* sanity check */
		if ((i == 0) && (eeprom[0] == 0xff))
			return EINVAL;
	}

	/* check checksum */
	csum = eeprom[6] + eeprom[7] + eeprom[8] + eeprom[9];
	csum = (csum >> 8) + (csum & 0xff) + eeprom[10];
	if (csum != 0xff) {
		printf("eeprom checksum mismatchi(0x%02x)\n", csum);
		return EINVAL;
	}

	memcpy(addr, eeprom, ETHER_ADDR_LEN);
	return 0;
#endif
}

static void
axen_ax88179_init(struct axen_softc *sc)
{
	struct axen_qctrl qctrl;
	uint16_t ctl, temp;
	uint16_t wval;
	uint8_t val;

	axen_lock_mii(sc);

	/* XXX: ? */
	axen_cmd(sc, AXEN_CMD_MAC_READ, 1, AXEN_UNK_05, &val);
	DPRINTFN(5, ("AXEN_CMD_MAC_READ(0x05): 0x%02x\n", val));

	/* check AX88179 version, UA1 / UA2 */
	axen_cmd(sc, AXEN_CMD_MAC_READ, 1, AXEN_GENERAL_STATUS, &val);
	/* UA1 */
	if (!(val & AXEN_GENERAL_STATUS_MASK)) {
		sc->axen_rev = AXEN_REV_UA1;
		DPRINTF(("AX88179 ver. UA1\n"));
	} else {
		sc->axen_rev = AXEN_REV_UA2;
		DPRINTF(("AX88179 ver. UA2\n"));
	}

	/* power up ethernet PHY */
	wval = htole16(0);
	axen_cmd(sc, AXEN_CMD_MAC_WRITE2, 2, AXEN_PHYPWR_RSTCTL, &wval);

	wval = htole16(AXEN_PHYPWR_RSTCTL_IPRL);
	axen_cmd(sc, AXEN_CMD_MAC_WRITE2, 2, AXEN_PHYPWR_RSTCTL, &wval);
	usbd_delay_ms(sc->axen_udev, 200);

	/* set clock mode */
	val = AXEN_PHYCLK_ACS | AXEN_PHYCLK_BCS;
	axen_cmd(sc, AXEN_CMD_MAC_WRITE, 1, AXEN_PHYCLK, &val);
	usbd_delay_ms(sc->axen_udev, 100);

	/* set monitor mode (disable) */
	val = AXEN_MONITOR_NONE;
	axen_cmd(sc, AXEN_CMD_MAC_WRITE, 1, AXEN_MONITOR_MODE, &val);

	/* enable auto detach */
	axen_cmd(sc, AXEN_CMD_EEPROM_READ, 2, AXEN_EEPROM_STAT, &wval);
	temp = le16toh(wval);
	DPRINTFN(2,("EEPROM0x43 = 0x%04x\n", temp));
	if (!(temp == 0xffff) && !(temp & 0x0100)) {
		/* Enable auto detach bit */
		val = 0;
		axen_cmd(sc, AXEN_CMD_MAC_WRITE, 1, AXEN_PHYCLK, &val);
		val = AXEN_PHYCLK_ULR;
		axen_cmd(sc, AXEN_CMD_MAC_WRITE, 1, AXEN_PHYCLK, &val);
		usbd_delay_ms(sc->axen_udev, 100);

		axen_cmd(sc, AXEN_CMD_MAC_READ2, 2, AXEN_PHYPWR_RSTCTL, &wval);
		ctl = le16toh(wval);
		ctl |= AXEN_PHYPWR_RSTCTL_AUTODETACH;
		wval = htole16(ctl);
		axen_cmd(sc, AXEN_CMD_MAC_WRITE2, 2, AXEN_PHYPWR_RSTCTL, &wval);
		usbd_delay_ms(sc->axen_udev, 200);
		aprint_error_dev(sc->axen_dev, "enable auto detach (0x%04x)\n",
		    ctl);
	}

	/* bulkin queue setting */
	axen_cmd(sc, AXEN_CMD_MAC_READ, 1, AXEN_USB_UPLINK, &val);
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
		aprint_error_dev(sc->axen_dev, "unknown uplink bus:0x%02x\n",
		    val);
		axen_unlock_mii(sc);
		return;
	}
	axen_cmd(sc, AXEN_CMD_MAC_SET_RXSR, 5, AXEN_RX_BULKIN_QCTRL, &qctrl);

	/*
	 * set buffer high/low watermark to pause/resume.
	 * write 2byte will set high/log simultaneous with AXEN_PAUSE_HIGH.
	 * XXX: what is the best value? OSX driver uses 0x3c-0x4c as LOW-HIGH
	 * watermark parameters.
	 */
	val = 0x34;
	axen_cmd(sc, AXEN_CMD_MAC_WRITE, 1, AXEN_PAUSE_LOW_WATERMARK, &val);
	val = 0x52;
	axen_cmd(sc, AXEN_CMD_MAC_WRITE, 1, AXEN_PAUSE_HIGH_WATERMARK, &val);

	/* Set RX/TX configuration. */
	/* Set RX control register */
	ctl = AXEN_RXCTL_IPE | AXEN_RXCTL_DROPCRCERR | AXEN_RXCTL_AUTOB;
	wval = htole16(ctl);
	axen_cmd(sc, AXEN_CMD_MAC_WRITE2, 2, AXEN_MAC_RXCTL, &wval);

	/* set monitor mode (enable) */
	val = AXEN_MONITOR_PMETYPE | AXEN_MONITOR_PMEPOL | AXEN_MONITOR_RWMP;
	axen_cmd(sc, AXEN_CMD_MAC_WRITE, 1, AXEN_MONITOR_MODE, &val);
	axen_cmd(sc, AXEN_CMD_MAC_READ, 1, AXEN_MONITOR_MODE, &val);
	DPRINTF(("axen: Monitor mode = 0x%02x\n", val));

	/* set medium type */
	ctl = AXEN_MEDIUM_GIGA | AXEN_MEDIUM_FDX | AXEN_MEDIUM_EN_125MHZ |
	    AXEN_MEDIUM_RXFLOW_CTRL_EN | AXEN_MEDIUM_TXFLOW_CTRL_EN |
	    AXEN_MEDIUM_RECV_EN;
	wval = htole16(ctl);
	DPRINTF(("axen: set to medium mode: 0x%04x\n", ctl));
	axen_cmd(sc, AXEN_CMD_MAC_WRITE2, 2, AXEN_MEDIUM_STATUS, &wval);
	usbd_delay_ms(sc->axen_udev, 100);

	axen_cmd(sc, AXEN_CMD_MAC_READ2, 2, AXEN_MEDIUM_STATUS, &wval);
	DPRINTF(("axen: current medium mode: 0x%04x\n", le16toh(wval)));

	axen_unlock_mii(sc);

#if 0 /* XXX: TBD.... */
#define GMII_LED_ACTIVE		0x1a
#define GMII_PHY_PAGE_SEL	0x1e
#define GMII_PHY_PAGE_SEL	0x1f
#define GMII_PAGE_EXT		0x0007
	axen_miibus_writereg(&sc->axen_dev, sc->axen_phyno, GMII_PHY_PAGE_SEL,
	    GMII_PAGE_EXT);
	axen_miibus_writereg(&sc->axen_dev, sc->axen_phyno, GMII_PHY_PAGE,
	    0x002c);
#endif

#if 1 /* XXX: phy hack ? */
	axen_miibus_writereg(sc->axen_dev, sc->axen_phyno, 0x1F, 0x0005);
	axen_miibus_writereg(sc->axen_dev, sc->axen_phyno, 0x0C, 0x0000);
	axen_miibus_readreg(sc->axen_dev, sc->axen_phyno, 0x0001, &wval);
	axen_miibus_writereg(sc->axen_dev, sc->axen_phyno, 0x01,
	    wval | 0x0080);
	axen_miibus_writereg(sc->axen_dev, sc->axen_phyno, 0x1F, 0x0000);
#endif
}

static void
axen_setcoe(struct axen_softc *sc)
{
	struct ifnet *ifp = GET_IFP(sc);
	uint64_t enabled = ifp->if_capenable;
	uint8_t val;

	KASSERT(mutex_owned(&sc->axen_mii_lock));

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
	axen_cmd(sc, AXEN_CMD_MAC_WRITE, 1, AXEN_RX_COE, &val);

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
	axen_cmd(sc, AXEN_CMD_MAC_WRITE, 1, AXEN_TX_COE, &val);
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
	struct axen_softc * const sc = device_private(self);
	struct usb_attach_arg *uaa = aux;
	struct usbd_device *dev = uaa->uaa_device;
	usbd_status err;
	usb_interface_descriptor_t *id;
	usb_endpoint_descriptor_t *ed;
	struct mii_data	*mii;
	uint8_t eaddr[ETHER_ADDR_LEN];
	char *devinfop;
	const char *devname = device_xname(self);
	struct ifnet *ifp;
	int i, s;

	aprint_naive("\n");
	aprint_normal("\n");

	sc->axen_dev = self;
	sc->axen_udev = dev;

	devinfop = usbd_devinfo_alloc(dev, 0);
	aprint_normal_dev(self, "%s\n", devinfop);
	usbd_devinfo_free(devinfop);

	err = usbd_set_config_no(dev, AXEN_CONFIG_NO, 1);
	if (err) {
		aprint_error_dev(self, "failed to set configuration"
		    ", err=%s\n", usbd_errstr(err));
		return;
	}

	sc->axen_flags = axen_lookup(uaa->uaa_vendor, uaa->uaa_product)->axen_flags;

	usb_init_task(&sc->axen_tick_task, axen_tick_task, sc, 0);

	err = usbd_device2interface_handle(dev, AXEN_IFACE_IDX,&sc->axen_iface);
	if (err) {
		aprint_error_dev(self, "getting interface handle failed\n");
		return;
	}

	sc->axen_product = uaa->uaa_product;
	sc->axen_vendor = uaa->uaa_vendor;

	id = usbd_get_interface_descriptor(sc->axen_iface);

	/* decide on what our bufsize will be */
	switch (sc->axen_udev->ud_speed) {
	case USB_SPEED_SUPER:
		sc->axen_rx_bufsz = AXEN_BUFSZ_SS * 1024;
		break;
	case USB_SPEED_HIGH:
		sc->axen_rx_bufsz = AXEN_BUFSZ_HS * 1024;
		break;
	default:
		sc->axen_rx_bufsz = AXEN_BUFSZ_LS * 1024;
		break;
	}

	sc->axen_tx_bufsz = IP_MAXPACKET +
	    ETHER_HDR_LEN + ETHER_CRC_LEN + ETHER_VLAN_ENCAP_LEN +
	    sizeof(struct axen_sframe_hdr);

	/* Find endpoints. */
	for (i = 0; i < id->bNumEndpoints; i++) {
		ed = usbd_interface2endpoint_descriptor(sc->axen_iface, i);
		if (!ed) {
			aprint_error_dev(self, "couldn't get ep %d\n", i);
			return;
		}
		if (UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_IN &&
		    UE_GET_XFERTYPE(ed->bmAttributes) == UE_BULK) {
			sc->axen_ed[AXEN_ENDPT_RX] = ed->bEndpointAddress;
		} else if (UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_OUT &&
			   UE_GET_XFERTYPE(ed->bmAttributes) == UE_BULK) {
			sc->axen_ed[AXEN_ENDPT_TX] = ed->bEndpointAddress;
		} else if (UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_IN &&
			   UE_GET_XFERTYPE(ed->bmAttributes) == UE_INTERRUPT) {
			sc->axen_ed[AXEN_ENDPT_INTR] = ed->bEndpointAddress;
		}
	}

	/* Set these up now for axen_cmd().  */
	mutex_init(&sc->axen_mii_lock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&sc->axen_txlock, MUTEX_DEFAULT, IPL_SOFTUSB);
	mutex_init(&sc->axen_rxlock, MUTEX_DEFAULT, IPL_SOFTUSB);
	mutex_init(&sc->axen_lock, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&sc->axen_detachcv, "axendet");

	s = splnet();

	sc->axen_phyno = AXEN_PHY_ID;
	DPRINTF(("%s: phyno %d\n", device_xname(self), sc->axen_phyno));

	/* Get station address.  */
	axen_lock_mii(sc);
	if (axen_get_eaddr(sc, &eaddr)) {
		axen_unlock_mii(sc);
		printf("EEPROM checksum error\n");
		cv_destroy(&sc->axen_detachcv);
		mutex_destroy(&sc->axen_lock);
		mutex_destroy(&sc->axen_rxlock);
		mutex_destroy(&sc->axen_txlock);
		mutex_destroy(&sc->axen_mii_lock);
		return;
	}
	axen_unlock_mii(sc);

	axen_ax88179_init(sc);

	/*
	 * An ASIX chip was detected. Inform the world.
	 */
	if (sc->axen_flags & AX178A)
		aprint_normal_dev(self, "AX88178a\n");
	else if (sc->axen_flags & AX179)
		aprint_normal_dev(self, "AX88179\n");
	aprint_normal_dev(self, "Ethernet address %s\n", ether_sprintf(eaddr));

	/* Initialize interface info.*/

	ifp = &sc->sc_if;
	ifp->if_softc = sc;
	strlcpy(ifp->if_xname, devname, IFNAMSIZ);
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_ioctl = axen_ioctl;
	ifp->if_start = axen_start;
	ifp->if_init = axen_init;
	ifp->if_stop = axen_stop;

	IFQ_SET_READY(&ifp->if_snd);

	sc->axen_ec.ec_capabilities = ETHERCAP_VLAN_MTU;

	/* Adapter does not support TSOv6 (They call it LSOv2). */
	ifp->if_capabilities |= IFCAP_TSOv4 |
	    IFCAP_CSUM_IPv4_Rx	| IFCAP_CSUM_IPv4_Tx  |
	    IFCAP_CSUM_TCPv4_Rx | IFCAP_CSUM_TCPv4_Tx |
	    IFCAP_CSUM_UDPv4_Rx | IFCAP_CSUM_UDPv4_Tx |
	    IFCAP_CSUM_TCPv6_Rx | IFCAP_CSUM_TCPv6_Tx |
	    IFCAP_CSUM_UDPv6_Rx | IFCAP_CSUM_UDPv6_Tx;

	/* Initialize MII/media info. */
	mii = &sc->axen_mii;
	mii->mii_ifp = ifp;
	mii->mii_readreg = axen_miibus_readreg;
	mii->mii_writereg = axen_miibus_writereg;
	mii->mii_statchg = axen_miibus_statchg;
	mii->mii_flags = MIIF_AUTOTSLEEP;

	sc->axen_ec.ec_mii = mii;
	ifmedia_init(&mii->mii_media, 0, axen_ifmedia_upd, axen_ifmedia_sts);
	mii_attach(self, mii, 0xffffffff, MII_PHY_ANY, MII_OFFSET_ANY, 0);

	if (LIST_FIRST(&mii->mii_phys) == NULL) {
		ifmedia_add(&mii->mii_media, IFM_ETHER | IFM_NONE, 0, NULL);
		ifmedia_set(&mii->mii_media, IFM_ETHER | IFM_NONE);
	} else
		ifmedia_set(&mii->mii_media, IFM_ETHER | IFM_AUTO);

	/* Attach the interface. */
	if_attach(ifp);
	ether_ifattach(ifp, eaddr);
	rnd_attach_source(&sc->rnd_source, device_xname(sc->axen_dev),
	    RND_TYPE_NET, RND_FLAG_DEFAULT);

	callout_init(&sc->axen_stat_ch, 0);
	callout_setfunc(&sc->axen_stat_ch, axen_tick, sc);

	sc->axen_attached = true;
	splx(s);

	usbd_add_drv_event(USB_EVENT_DRIVER_ATTACH, sc->axen_udev,sc->axen_dev);

	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n");
}

static int
axen_detach(device_t self, int flags)
{
	struct axen_softc * const sc = device_private(self);
	struct ifnet *ifp = GET_IFP(sc);
	int s;

	mutex_enter(&sc->axen_lock);
	sc->axen_dying = true;
	mutex_exit(&sc->axen_lock);

	DPRINTFN(2,("%s: %s: enter\n", device_xname(sc->axen_dev), __func__));

	/* Detached before attached finished, so just bail out. */
	if (!sc->axen_attached)
		return 0;

	pmf_device_deregister(self);

	callout_halt(&sc->axen_stat_ch, NULL);
	usb_rem_task_wait(sc->axen_udev, &sc->axen_tick_task,
	    USB_TASKQ_DRIVER, NULL);

	s = splusb();

	if (ifp->if_flags & IFF_RUNNING)
		axen_stop(ifp, 1);

	mutex_enter(&sc->axen_lock);
	sc->axen_refcnt--;
	while (sc->axen_refcnt > 0) {
		/* Wait for processes to go away */
		cv_wait(&sc->axen_detachcv, &sc->axen_lock);
	}

#ifdef DIAGNOSTIC
	if (sc->axen_ep[AXEN_ENDPT_TX] != NULL ||
	    sc->axen_ep[AXEN_ENDPT_RX] != NULL ||
	    sc->axen_ep[AXEN_ENDPT_INTR] != NULL)
		aprint_debug_dev(self, "detach has active endpoints\n");
#endif

	mutex_exit(&sc->axen_lock);

	callout_destroy(&sc->axen_stat_ch);
	rnd_detach_source(&sc->rnd_source);
	mii_detach(&sc->axen_mii, MII_PHY_ANY, MII_OFFSET_ANY);
	ifmedia_delete_instance(&sc->axen_mii.mii_media, IFM_INST_ANY);
	ether_ifdetach(ifp);
	if_detach(ifp);

	sc->axen_attached = false;

	splx(s);

	usbd_add_drv_event(USB_EVENT_DRIVER_DETACH, sc->axen_udev,sc->axen_dev);

	cv_destroy(&sc->axen_detachcv);
	mutex_destroy(&sc->axen_lock);
	mutex_destroy(&sc->axen_rxlock);
	mutex_destroy(&sc->axen_txlock);
	mutex_destroy(&sc->axen_mii_lock);

	return 0;
}

static int
axen_activate(device_t self, devact_t act)
{
	struct axen_softc * const sc = device_private(self);
	struct ifnet *ifp = GET_IFP(sc);

	DPRINTFN(2,("%s: %s: enter\n", device_xname(sc->axen_dev), __func__));

	switch (act) {
	case DVACT_DEACTIVATE:
		if_deactivate(ifp);

		mutex_enter(&sc->axen_lock);
		sc->axen_dying = true;
		mutex_exit(&sc->axen_lock);

		mutex_enter(&sc->axen_rxlock);
		mutex_enter(&sc->axen_txlock);
		sc->axen_stopping = true;
		mutex_exit(&sc->axen_txlock);
		mutex_exit(&sc->axen_rxlock);

		return 0;
	default:
		return EOPNOTSUPP;
	}
}

static struct mbuf *
axen_newbuf(void)
{
	struct mbuf *m;

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == NULL)
		return NULL;

	MCLGET(m, M_DONTWAIT);
	if (!(m->m_flags & M_EXT)) {
		m_freem(m);
		return NULL;
	}

	m->m_len = m->m_pkthdr.len = MCLBYTES;
	m_adj(m, ETHER_ALIGN);

	return m;
}

static int
axen_rx_list_init(struct axen_softc *sc)
{
	struct axen_cdata *cd;
	struct axen_chain *c;
	int i;

	DPRINTF(("%s: %s: enter\n", device_xname(sc->axen_dev), __func__));

	cd = &sc->axen_cdata;
	for (i = 0; i < AXEN_RX_LIST_CNT; i++) {
		c = &cd->axen_rx_chain[i];
		c->axen_sc = sc;
		if (c->axen_xfer == NULL) {
			int err = usbd_create_xfer(sc->axen_ep[AXEN_ENDPT_RX],
			    sc->axen_rx_bufsz, 0, 0, &c->axen_xfer);
			if (err)
				return err;
			c->axen_buf = usbd_get_buffer(c->axen_xfer);
		}
	}

	return 0;
}

static int
axen_tx_list_init(struct axen_softc *sc)
{
	struct axen_cdata *cd;
	struct axen_chain *c;
	int i;

	DPRINTF(("%s: %s: enter\n", device_xname(sc->axen_dev), __func__));

	cd = &sc->axen_cdata;
	for (i = 0; i < AXEN_TX_LIST_CNT; i++) {
		c = &cd->axen_tx_chain[i];
		c->axen_sc = sc;
		if (c->axen_xfer == NULL) {
			int err = usbd_create_xfer(sc->axen_ep[AXEN_ENDPT_TX],
			    sc->axen_tx_bufsz, USBD_FORCE_SHORT_XFER, 0,
			    &c->axen_xfer);
			if (err)
				return err;
			c->axen_buf = usbd_get_buffer(c->axen_xfer);
		}
	}

	cd->axen_tx_prod = cd->axen_tx_cnt = 0;

	return 0;
}

/*
 * A frame has been uploaded: pass the resulting mbuf chain up to
 * the higher level protocols.
 */
static void
axen_rxeof(struct usbd_xfer *xfer, void * priv, usbd_status status)
{
	struct axen_chain *c = (struct axen_chain *)priv;
	struct axen_softc * const sc = c->axen_sc;
	struct ifnet *ifp = GET_IFP(sc);
	uint8_t *buf = c->axen_buf;
	struct mbuf *m;
	uint32_t total_len;
	uint32_t rx_hdr, pkt_hdr;
	uint32_t *hdr_p;
	uint16_t hdr_offset, pkt_count;
	size_t pkt_len;
	size_t temp;
	int s;

	DPRINTFN(10,("%s: %s: enter\n", device_xname(sc->axen_dev), __func__));

	mutex_enter(&sc->axen_rxlock);

	if (sc->axen_dying || sc->axen_stopping ||
	    status == USBD_INVAL || status == USBD_NOT_STARTED ||
	    status == USBD_CANCELLED || !(ifp->if_flags & IFF_RUNNING)) {
		mutex_exit(&sc->axen_rxlock);
		return;
	}

	if (status != USBD_NORMAL_COMPLETION) {
		if (usbd_ratecheck(&sc->axen_rx_notice))
			aprint_error_dev(sc->axen_dev, "usb errors on rx: %s\n",
			    usbd_errstr(status));
		if (status == USBD_STALLED)
			usbd_clear_endpoint_stall_async(sc->axen_ep[AXEN_ENDPT_RX]);
		goto done;
	}

	usbd_get_xfer_status(xfer, NULL, NULL, &total_len, NULL);

	if (total_len < sizeof(pkt_hdr)) {
		aprint_error_dev(sc->axen_dev, "rxeof: too short transfer\n");
		ifp->if_ierrors++;
		goto done;
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

	if (total_len > sc->axen_rx_bufsz) {
		aprint_error_dev(sc->axen_dev,
		    "rxeof: too large transfer (%u > %u)\n",
		    total_len, sc->axen_rx_bufsz);
		goto done;
	}

	/* sanity check */
	if (hdr_offset > total_len) {
		aprint_error_dev(sc->axen_dev,
		    "rxeof: invalid hdr offset (%u > %u)\n",
		    hdr_offset, total_len);
		ifp->if_ierrors++;
		usbd_delay_ms(sc->axen_udev, 100);
		goto done;
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
		    device_xname(sc->axen_dev), pkt_count));
		goto done;
	}
#endif

	do {
		if ((buf[0] != 0xee) || (buf[1] != 0xee)) {
			aprint_error_dev(sc->axen_dev,
			    "invalid buffer(pkt#%d), continue\n", pkt_count);
			ifp->if_ierrors += pkt_count;
			goto done;
		}

		pkt_hdr = le32toh(*hdr_p);
		pkt_len = (pkt_hdr >> 16) & 0x1fff;
		DPRINTFN(10,
		    ("%s: rxeof: packet#%d, pkt_hdr 0x%08x, pkt_len %zu\n",
		   device_xname(sc->axen_dev), pkt_count, pkt_hdr, pkt_len));

		if (pkt_hdr & (AXEN_RXHDR_CRC_ERR | AXEN_RXHDR_DROP_ERR)) {
			ifp->if_ierrors++;
			/* move to next pkt header */
			DPRINTF(("%s: %s err (pkt#%d)\n",
			    device_xname(sc->axen_dev),
			    (pkt_hdr & AXEN_RXHDR_CRC_ERR) ? "crc" : "drop",
			    pkt_count));
			goto nextpkt;
		}

		/* process each packet */
		/* allocate mbuf */
		m = axen_newbuf();
		if (m == NULL) {
			ifp->if_ierrors++;
			goto nextpkt;
		}

		/* skip pseudo header (2byte) */
		m_set_rcvif(m, ifp);
		m->m_pkthdr.len = m->m_len = pkt_len - 6;

		m->m_pkthdr.csum_flags = axen_csum_flags_rx(ifp, pkt_hdr);
		memcpy(mtod(m, char *), buf + 2, pkt_len - 6);

		mutex_exit(&sc->axen_rxlock);

		/* push the packet up */
		s = splnet();
		if_percpuq_enqueue((ifp)->if_percpuq, (m));
		splx(s);

		mutex_enter(&sc->axen_rxlock);
		if (sc->axen_stopping) {
			mutex_exit(&sc->axen_rxlock);
			return;
		}

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

done:
	mutex_exit(&sc->axen_rxlock);

	/* Setup new transfer. */
	usbd_setup_xfer(xfer, c, c->axen_buf, sc->axen_rx_bufsz,
	    USBD_SHORT_XFER_OK, USBD_NO_TIMEOUT, axen_rxeof);
	usbd_transfer(xfer);

	DPRINTFN(10,("%s: %s: start rx\n",device_xname(sc->axen_dev),__func__));
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

/*
 * A frame was downloaded to the chip. It's safe for us to clean up
 * the list buffers.
 */
static void
axen_txeof(struct usbd_xfer *xfer, void * priv, usbd_status status)
{
	struct axen_chain *c = (struct axen_chain *)priv;
	struct axen_softc * const sc = c->axen_sc;
	struct axen_cdata *cd = &sc->axen_cdata;
	struct ifnet *ifp = GET_IFP(sc);
	int s;

	mutex_enter(&sc->axen_txlock);
	if (sc->axen_stopping || sc->axen_dying) {
		mutex_exit(&sc->axen_txlock);
		return;
	}

	s = splnet();

	KASSERT(cd->axen_tx_cnt > 0);
	cd->axen_tx_cnt--;

	sc->axen_timer = 0;
	ifp->if_flags &= ~IFF_OACTIVE;

	switch (status) {
	case USBD_NOT_STARTED:
	case USBD_CANCELLED:
		break;

	case USBD_NORMAL_COMPLETION:
		if (!IFQ_IS_EMPTY(&ifp->if_snd))
			axen_start_locked(ifp);
		break;

	default:
		ifp->if_opackets++;

		ifp->if_oerrors++;
		if (usbd_ratecheck(&sc->axen_tx_notice))
			aprint_error_dev(sc->axen_dev, "usb error on tx: %s\n",
			    usbd_errstr(status));
		if (status == USBD_STALLED)
			usbd_clear_endpoint_stall_async(sc->axen_ep[AXEN_ENDPT_TX]);
		break;
	}

	splx(s);
	mutex_exit(&sc->axen_txlock);
}

static void
axen_tick(void *xsc)
{
	struct axen_softc * const sc = xsc;

	if (sc == NULL)
		return;

	mutex_enter(&sc->axen_lock);

	DPRINTFN(0xff,("%s: %s: enter\n", device_xname(sc->axen_dev),__func__));

	if (sc->axen_stopping || sc->axen_dying) {
		mutex_exit(&sc->axen_lock);
		return;
	}

	/* Perform periodic stuff in process context */
	usb_add_task(sc->axen_udev, &sc->axen_tick_task, USB_TASKQ_DRIVER);

	mutex_exit(&sc->axen_lock);
}

static void
axen_tick_task(void *xsc)
{
	struct axen_softc * const sc = xsc;
	struct ifnet *ifp;
	struct mii_data *mii;
	int s;

	if (sc == NULL)
		return;

	mutex_enter(&sc->axen_lock);
	if (sc->axen_stopping || sc->axen_dying) {
		mutex_exit(&sc->axen_lock);
		return;
	}

	ifp = GET_IFP(sc);
	mii = GET_MII(sc);
	if (mii == NULL)
		return;

	sc->axen_refcnt++;
	mutex_exit(&sc->axen_lock);

	s = splnet();

	if (sc->axen_timer != 0 && --sc->axen_timer == 0)
		axen_watchdog(ifp);

	mii_tick(mii);

	if (sc->axen_link == 0)
		axen_miibus_statchg(ifp);

	mutex_enter(&sc->axen_lock);
	if (--sc->axen_refcnt < 0)
		cv_broadcast(&sc->axen_detachcv);
	if (sc->axen_stopping || sc->axen_dying)
		callout_schedule(&sc->axen_stat_ch, hz);
	mutex_exit(&sc->axen_lock);

	splx(s);
}

static int
axen_encap(struct axen_softc *sc, struct mbuf *m, int idx)
{
	struct ifnet *ifp = GET_IFP(sc);
	struct axen_chain *c;
	usbd_status err;
	struct axen_sframe_hdr hdr;
	u_int length, boundary;

	KASSERT(mutex_owned(&sc->axen_txlock));

	c = &sc->axen_cdata.axen_tx_chain[idx];

	/* XXX Is this needed?  wMaxPacketSize? */
	switch (sc->axen_udev->ud_speed) {
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

	length = m->m_pkthdr.len + sizeof(hdr);
	KASSERT(length <= sc->axen_tx_bufsz);

	hdr.plen = htole32(m->m_pkthdr.len);

	hdr.gso = (m->m_pkthdr.csum_flags & M_CSUM_TSOv4) ?
	    m->m_pkthdr.segsz : 0;
	if ((length % boundary) == 0) {
		DPRINTF(("%s: boundary hit\n", device_xname(sc->axen_dev)));
		hdr.gso |= 0x80008000;	/* XXX enable padding */
	}
	hdr.gso = htole32(hdr.gso);

	memcpy(c->axen_buf, &hdr, sizeof(hdr));
	m_copydata(m, 0, m->m_pkthdr.len, c->axen_buf + sizeof(hdr));

	if (__predict_false(c->axen_xfer == NULL))
		return EIO;	/* XXX plugged out or down */

	usbd_setup_xfer(c->axen_xfer, c, c->axen_buf, length,
	    USBD_FORCE_SHORT_XFER, 10000, axen_txeof);

	/* Transmit */
	err = usbd_transfer(c->axen_xfer);
	if (err != USBD_IN_PROGRESS) {
		axen_stop(ifp, 0);
		return EIO;
	}

	return 0;
}

static void
axen_start_locked(struct ifnet *ifp)
{
	struct axen_softc * const sc = ifp->if_softc;
	struct mbuf *m;
	struct axen_cdata *cd = &sc->axen_cdata;
	int idx;

	KASSERT(mutex_owned(&sc->axen_txlock));

	if (sc->axen_link == 0)
		return;

	if ((ifp->if_flags & (IFF_OACTIVE | IFF_RUNNING)) != IFF_RUNNING)
		return;

	idx = cd->axen_tx_prod;
	while (cd->axen_tx_cnt < AXEN_TX_LIST_CNT) {
		IFQ_POLL(&ifp->if_snd, m);
		if (m == NULL)
			break;

		if (axen_encap(sc, m, idx)) {
			ifp->if_flags |= IFF_OACTIVE; /* XXX */
			ifp->if_oerrors++;
			break;
		}
		IFQ_DEQUEUE(&ifp->if_snd, m);

		/*
		 * If there's a BPF listener, bounce a copy of this frame
		 * to him.
		 */
		bpf_mtap(ifp, m, BPF_D_OUT);
		m_freem(m);

		idx = (idx + 1) % AXEN_TX_LIST_CNT;
		cd->axen_tx_cnt++;
	}
	cd->axen_tx_prod = idx;

	if (cd->axen_tx_cnt >= AXEN_TX_LIST_CNT)
		ifp->if_flags |= IFF_OACTIVE;

	/*
	 * Set a timeout in case the chip goes out to lunch.
	 */
	sc->axen_timer = 5;
}

static void
axen_start(struct ifnet *ifp)
{
	struct axen_softc * const sc = ifp->if_softc;

	mutex_enter(&sc->axen_txlock);
	if (!sc->axen_stopping)
		axen_start_locked(ifp);
	mutex_exit(&sc->axen_txlock);
}

static int
axen_init_locked(struct ifnet *ifp)
{
	struct axen_softc * const sc = ifp->if_softc;
	struct axen_chain *c;
	usbd_status err;
	int i, s;
	uint16_t rxmode;
	uint16_t wval;
	uint8_t bval;

	KASSERT(mutex_owned(&sc->axen_lock));

	if (sc->axen_dying)
		return EIO;

	s = splnet();

	/* Cancel pending I/O */
	axen_stop_locked(ifp, 1);

	/* Reset the ethernet interface. */
	axen_reset(sc);

	axen_lock_mii_sc_locked(sc);

	/* XXX: ? */
	bval = 0x01;
	axen_cmd(sc, AXEN_CMD_MAC_WRITE, 1, AXEN_UNK_28, &bval);

	/* Configure offloading engine. */
	axen_setcoe(sc);

	/* Program promiscuous mode and multicast filters. */
	axen_iff(sc);

	/* Enable receiver, set RX mode */
	axen_cmd(sc, AXEN_CMD_MAC_READ2, 2, AXEN_MAC_RXCTL, &wval);
	rxmode = le16toh(wval);
	rxmode |= AXEN_RXCTL_START;
	wval = htole16(rxmode);
	axen_cmd(sc, AXEN_CMD_MAC_WRITE2, 2, AXEN_MAC_RXCTL, &wval);

	axen_unlock_mii_sc_locked(sc);

	/* Open RX and TX pipes. */
	err = usbd_open_pipe(sc->axen_iface, sc->axen_ed[AXEN_ENDPT_RX],
	    USBD_EXCLUSIVE_USE, &sc->axen_ep[AXEN_ENDPT_RX]);
	if (err) {
		aprint_error_dev(sc->axen_dev, "open rx pipe failed: %s\n",
		    usbd_errstr(err));
		splx(s);
		return EIO;
	}

	err = usbd_open_pipe(sc->axen_iface, sc->axen_ed[AXEN_ENDPT_TX],
	    USBD_EXCLUSIVE_USE, &sc->axen_ep[AXEN_ENDPT_TX]);
	if (err) {
		aprint_error_dev(sc->axen_dev, "open tx pipe failed: %s\n",
		    usbd_errstr(err));
		splx(s);
		return EIO;
	}

	/* Init RX ring. */
	if (axen_rx_list_init(sc)) {
		aprint_error_dev(sc->axen_dev, "rx list init failed\n");
		splx(s);
		return ENOBUFS;
	}

	/* Init TX ring. */
	if (axen_tx_list_init(sc)) {
		aprint_error_dev(sc->axen_dev, "tx list init failed\n");
		splx(s);
		return ENOBUFS;
	}

	mutex_enter(&sc->axen_rxlock);
	mutex_enter(&sc->axen_txlock);
	sc->axen_stopping = false;

	/* Start up the receive pipe. */
	for (i = 0; i < AXEN_RX_LIST_CNT; i++) {
		c = &sc->axen_cdata.axen_rx_chain[i];

		usbd_setup_xfer(c->axen_xfer, c, c->axen_buf, sc->axen_rx_bufsz,
		    USBD_SHORT_XFER_OK, USBD_NO_TIMEOUT, axen_rxeof);
		usbd_transfer(c->axen_xfer);
	}

	mutex_exit(&sc->axen_txlock);
	mutex_exit(&sc->axen_rxlock);

	/* Indicate we are up and running. */
	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;

	splx(s);

	callout_schedule(&sc->axen_stat_ch, hz);
	return 0;
}

static int
axen_init(struct ifnet *ifp)
{
	struct axen_softc * const sc = ifp->if_softc;

	mutex_enter(&sc->axen_lock);
	int ret = axen_init_locked(ifp);
	mutex_exit(&sc->axen_lock);

	return ret;
}

static int
axen_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	struct axen_softc * const sc = ifp->if_softc;
	int s;
	int error = 0;

	s = splnet();

	switch (cmd) {
	case SIOCSIFFLAGS:
		if ((error = ifioctl_common(ifp, cmd, data)) != 0)
			break;

		switch (ifp->if_flags & (IFF_UP | IFF_RUNNING)) {
		case IFF_RUNNING:
			axen_stop(ifp, 1);
			break;
		case IFF_UP:
			axen_init(ifp);
			break;
		case IFF_UP | IFF_RUNNING:
			if ((ifp->if_flags ^ sc->axen_if_flags) == IFF_PROMISC) {
				axen_lock_mii(sc);
				axen_iff(sc);
				axen_unlock_mii(sc);
			} else
				axen_init(ifp);
			break;
		}

		mutex_enter(&sc->axen_rxlock);
		mutex_enter(&sc->axen_txlock);
		sc->axen_if_flags = ifp->if_flags;
		mutex_exit(&sc->axen_txlock);
		mutex_exit(&sc->axen_rxlock);
		break;

	default:
		if ((error = ether_ioctl(ifp, cmd, data)) != ENETRESET)
			break;

		error = 0;
		switch (cmd) {
		case SIOCADDMULTI:
		case SIOCDELMULTI:
			axen_lock_mii(sc);
			axen_iff(sc);
			axen_unlock_mii(sc);
			break;
		case SIOCSIFCAP:
			mutex_enter(&sc->axen_lock);
			axen_setcoe(sc);
			mutex_exit(&sc->axen_lock);
			break;
		default:
			break;
		}
		break;
	}
	splx(s);

	return error;
}

static void
axen_watchdog(struct ifnet *ifp)
{
	struct axen_softc * const sc = ifp->if_softc;
	struct axen_chain *c;
	usbd_status stat;
	int s;

	ifp->if_oerrors++;
	aprint_error_dev(sc->axen_dev, "watchdog timeout\n");

	s = splusb();

	c = &sc->axen_cdata.axen_tx_chain[0];
	usbd_get_xfer_status(c->axen_xfer, NULL, NULL, NULL, &stat);
	axen_txeof(c->axen_xfer, c, stat);

	if (!IFQ_IS_EMPTY(&ifp->if_snd))
		axen_start(ifp);
	splx(s);
}

/*
 * Stop the adapter and free any mbufs allocated to the
 * RX and TX lists.
 */
static void
axen_stop_locked(struct ifnet *ifp, int disable)
{
	struct axen_softc * const sc = ifp->if_softc;
	struct axen_chain *c;
	usbd_status err;
	int i;
	uint16_t rxmode, wval;

	KASSERT(mutex_owned(&sc->axen_lock));
	mutex_enter(&sc->axen_rxlock);
	mutex_enter(&sc->axen_txlock);
	sc->axen_stopping = true;
	mutex_exit(&sc->axen_txlock);
	mutex_exit(&sc->axen_rxlock);

	axen_reset(sc);

	/* Disable receiver, set RX mode */
	axen_lock_mii_sc_locked(sc);
	axen_cmd(sc, AXEN_CMD_MAC_READ2, 2, AXEN_MAC_RXCTL, &wval);
	rxmode = le16toh(wval);
	rxmode &= ~AXEN_RXCTL_START;
	wval = htole16(rxmode);
	axen_cmd(sc, AXEN_CMD_MAC_WRITE2, 2, AXEN_MAC_RXCTL, &wval);
	axen_unlock_mii_sc_locked(sc);

	sc->axen_timer = 0;
	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);

	callout_stop(&sc->axen_stat_ch);
	sc->axen_link = 0;

	/* Stop transfers. */
	if (sc->axen_ep[AXEN_ENDPT_RX] != NULL) {
		err = usbd_abort_pipe(sc->axen_ep[AXEN_ENDPT_RX]);
		if (err) {
			aprint_error_dev(sc->axen_dev,
			    "abort rx pipe failed: %s\n", usbd_errstr(err));

		}
	}

	if (sc->axen_ep[AXEN_ENDPT_TX] != NULL) {
		err = usbd_abort_pipe(sc->axen_ep[AXEN_ENDPT_TX]);
		if (err) {
			aprint_error_dev(sc->axen_dev,
			    "abort tx pipe failed: %s\n", usbd_errstr(err));
		}
	}

	if (sc->axen_ep[AXEN_ENDPT_INTR] != NULL) {
		err = usbd_abort_pipe(sc->axen_ep[AXEN_ENDPT_INTR]);
		if (err) {
			aprint_error_dev(sc->axen_dev,
			    "abort intr pipe failed: %s\n", usbd_errstr(err));
		}
	}

	/* Free RX resources. */
	for (i = 0; i < AXEN_RX_LIST_CNT; i++) {
		c = &sc->axen_cdata.axen_rx_chain[i];
		if (c->axen_xfer != NULL) {
			usbd_destroy_xfer(c->axen_xfer);
			c->axen_xfer = NULL;
		}
	}

	/* Free TX resources. */
	for (i = 0; i < AXEN_TX_LIST_CNT; i++) {
		c = &sc->axen_cdata.axen_tx_chain[i];
		if (c->axen_xfer != NULL) {
			usbd_destroy_xfer(c->axen_xfer);
			c->axen_xfer = NULL;
		}
	}

	/* Close pipes. */
	if (sc->axen_ep[AXEN_ENDPT_RX] != NULL) {
		err = usbd_close_pipe(sc->axen_ep[AXEN_ENDPT_RX]);
		if (err) {
			aprint_error_dev(sc->axen_dev,
			    "close rx pipe failed: %s\n", usbd_errstr(err));
		}
		sc->axen_ep[AXEN_ENDPT_RX] = NULL;
	}

	if (sc->axen_ep[AXEN_ENDPT_TX] != NULL) {
		err = usbd_close_pipe(sc->axen_ep[AXEN_ENDPT_TX]);
		if (err) {
			aprint_error_dev(sc->axen_dev,
			    "close tx pipe failed: %s\n", usbd_errstr(err));
		}
		sc->axen_ep[AXEN_ENDPT_TX] = NULL;
	}

	if (sc->axen_ep[AXEN_ENDPT_INTR] != NULL) {
		err = usbd_close_pipe(sc->axen_ep[AXEN_ENDPT_INTR]);
		if (err) {
			aprint_error_dev(sc->axen_dev,
			    "close intr pipe failed: %s\n", usbd_errstr(err));
		}
		sc->axen_ep[AXEN_ENDPT_INTR] = NULL;
	}
}

static void
axen_stop(struct ifnet *ifp, int disable)
{
	struct axen_softc * const sc = ifp->if_softc;

	mutex_enter(&sc->axen_lock);
	axen_stop_locked(ifp, disable);
	mutex_exit(&sc->axen_lock);
}

MODULE(MODULE_CLASS_DRIVER, if_axen, NULL);

#ifdef _MODULE
#include "ioconf.c"
#endif

static int
if_axen_modcmd(modcmd_t cmd, void *aux)
{
	int error = 0;

	switch (cmd) {
	case MODULE_CMD_INIT:
#ifdef _MODULE
		error = config_init_component(cfdriver_ioconf_axen,
		    cfattach_ioconf_axen, cfdata_ioconf_axen);
#endif
		return error;
	case MODULE_CMD_FINI:
#ifdef _MODULE
		error = config_fini_component(cfdriver_ioconf_axen,
		    cfattach_ioconf_axen, cfdata_ioconf_axen);
#endif
		return error;
	default:
		return ENOTTY;
	}
}
