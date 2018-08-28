/*	$NetBSD: if_mue.c,v 1.2 2018/08/27 14:59:04 rin Exp $	*/
/*	$OpenBSD: if_mue.c,v 1.3 2018/08/04 16:42:46 jsg Exp $	*/

/*
 * Copyright (c) 2018 Kevin Lo <kevlo@openbsd.org>
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

/* Driver for Microchip LAN7500/LAN7800 chipsets. */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_mue.c,v 1.2 2018/08/27 14:59:04 rin Exp $");

#ifdef _KERNEL_OPT
#include "opt_usb.h"
#include "opt_inet.h"
#endif

#include <sys/param.h>
#include <sys/cprng.h>
#include <sys/bus.h>
#include <sys/systm.h>
#include <sys/sockio.h>
#include <sys/mbuf.h>
#include <sys/mutex.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/socket.h>

#include <sys/device.h>

#include <sys/rndsource.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_ether.h>

#include <net/bpf.h>

#include <netinet/in.h>
#include <netinet/if_inarp.h>

#include <dev/mii/mii.h>
#include <dev/mii/miivar.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdi_util.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usbdevs.h>

#include <dev/usb/if_muereg.h>
#include <dev/usb/if_muevar.h>

#define MUE_PRINTF(sc, fmt, args...)					\
	device_printf((sc)->mue_dev, "%s: " fmt, __func__, ##args);

#ifdef USB_DEBUG
int muedebug = 0;
#define DPRINTF(sc, fmt, args...) 					\
	do { 								\
		if (muedebug)						\
			MUE_PRINTF(sc, fmt, ##args);			\
	} while (0 /* CONSTCOND */)
#else
#define DPRINTF(sc, fmt, args...)	/* nothing */
#endif

/*
 * Various supported device vendors/products.
 */
struct mue_type {
	struct usb_devno	mue_dev;
	uint16_t		mue_flags;
#define LAN7500		0x0001	/* LAN7500 */
};

const struct mue_type mue_devs[] = {
	{ { USB_VENDOR_SMSC, USB_PRODUCT_SMSC_LAN7500 }, LAN7500 },
	{ { USB_VENDOR_SMSC, USB_PRODUCT_SMSC_LAN7505 }, LAN7500 },
	{ { USB_VENDOR_SMSC, USB_PRODUCT_SMSC_LAN7800 }, 0 },
	{ { USB_VENDOR_SMSC, USB_PRODUCT_SMSC_LAN7801 }, 0 },
	{ { USB_VENDOR_SMSC, USB_PRODUCT_SMSC_LAN7850 }, 0 }
};

#define MUE_LOOKUP(uaa)	((const struct mue_type *)usb_lookup(mue_devs, \
    uaa->uaa_vendor, uaa->uaa_product))

#define MUE_ENADDR_LO(enaddr) \
    ((enaddr[3] << 24) | (enaddr[2] << 16) | (enaddr[1] << 8) | enaddr[0])
#define MUE_ENADDR_HI(enaddr) \
    ((enaddr[5] << 8) | enaddr[4])

static int	mue_match(device_t, cfdata_t, void *);
static void	mue_attach(device_t, device_t, void *);
static int	mue_detach(device_t, int);
static int	mue_activate(device_t, enum devact);

static uint32_t	mue_csr_read(struct mue_softc *, uint32_t);
static int	mue_csr_write(struct mue_softc *, uint32_t, uint32_t);
static int	mue_wait_for_bits(struct mue_softc *sc, uint32_t, uint32_t,
		    uint32_t, uint32_t);

static void	mue_lock_mii(struct mue_softc *);
static void	mue_unlock_mii(struct mue_softc *);

static int	mue_miibus_readreg(device_t, int, int);
static void	mue_miibus_writereg(device_t, int, int, int);
static void	mue_miibus_statchg(struct ifnet *);
static int	mue_ifmedia_upd(struct ifnet *);
static void	mue_ifmedia_sts(struct ifnet *, struct ifmediareq *);

static uint8_t	mue_eeprom_getbyte(struct mue_softc *, int, uint8_t *);
static int	mue_read_eeprom(struct mue_softc *, uint8_t *, int, int);
static bool	mue_eeprom_present(struct mue_softc *sc);

static int	mue_read_otp_raw(struct mue_softc *, uint8_t *, int, int);
static int	mue_read_otp(struct mue_softc *, uint8_t *, int, int);

static void	mue_dataport_write(struct mue_softc *, uint32_t, uint32_t,
		    uint32_t, uint32_t *);

static void	mue_init_ltm(struct mue_softc *);

static int	mue_chip_init(struct mue_softc *);

static void	mue_set_macaddr(struct mue_softc *);
static int	mue_get_macaddr(struct mue_softc *, prop_dictionary_t);

static int	mue_rx_list_init(struct mue_softc *);
static int	mue_tx_list_init(struct mue_softc *);
static int	mue_open_pipes(struct mue_softc *);
static void	mue_start_rx(struct mue_softc *);

static int	mue_encap(struct mue_softc *, struct mbuf *, int);

static void	mue_setmulti(struct mue_softc *);
static void	mue_sethwcsum(struct mue_softc *);

static void	mue_rxeof(struct usbd_xfer *, void *, usbd_status);
static void	mue_txeof(struct usbd_xfer *, void *, usbd_status);

static int	mue_init(struct ifnet *);
static int	mue_ioctl(struct ifnet *, u_long, void *);
static void	mue_watchdog(struct ifnet *);
static void	mue_reset(struct mue_softc *);
static void	mue_start(struct ifnet *);
static void	mue_stop(struct ifnet *, int);
static void	mue_tick(void *);
static void	mue_tick_task(void *);

static struct mbuf *mue_newbuf(void);

#define MUE_SETBIT(sc, reg, x)	\
	mue_csr_write(sc, reg, mue_csr_read(sc, reg) | (x))

#define MUE_CLRBIT(sc, reg, x)	\
	mue_csr_write(sc, reg, mue_csr_read(sc, reg) & ~(x))

#define MUE_WAIT_SET(sc, reg, set, fail)	\
	mue_wait_for_bits(sc, reg, set, ~0, fail)

#define MUE_WAIT_CLR(sc, reg, clear, fail)	\
	mue_wait_for_bits(sc, reg, 0, clear, fail)

#define ETHER_IS_VALID(addr) \
	(!ETHER_IS_MULTICAST(addr) && !ETHER_IS_ZERO(addr))

#define ETHER_IS_ZERO(addr) \
	(!(addr[0] | addr[1] | addr[2] | addr[3] | addr[4] | addr[5]))

#define ETHER_ALIGN 2

CFATTACH_DECL_NEW(mue, sizeof(struct mue_softc), mue_match, mue_attach,
    mue_detach, mue_activate);

static uint32_t
mue_csr_read(struct mue_softc *sc, uint32_t reg)
{
	usb_device_request_t req;
	usbd_status err;
	uDWord val;

	if (sc->mue_dying)
		return 0;

	USETDW(val, 0);
	req.bmRequestType = UT_READ_VENDOR_DEVICE;
	req.bRequest = MUE_UR_READREG;
	USETW(req.wValue, 0);
	USETW(req.wIndex, reg);
	USETW(req.wLength, 4);

	err = usbd_do_request(sc->mue_udev, &req, &val);
	if (err) {
		MUE_PRINTF(sc, "reg = 0x%x: %s\n", reg, usbd_errstr(err));
		return 0;
	}

	return UGETDW(val);
}

static int
mue_csr_write(struct mue_softc *sc, uint32_t reg, uint32_t aval)
{
	usb_device_request_t req;
	usbd_status err;
	uDWord val;

	if (sc->mue_dying)
		return 0;

	USETDW(val, aval);
	req.bmRequestType = UT_WRITE_VENDOR_DEVICE;
	req.bRequest = MUE_UR_WRITEREG;
	USETW(req.wValue, 0);
	USETW(req.wIndex, reg);
	USETW(req.wLength, 4);

	err = usbd_do_request(sc->mue_udev, &req, &val);
	if (err) {
		MUE_PRINTF(sc, "reg = 0x%x: %s\n", reg, usbd_errstr(err));
		return -1;
	}

	return 0;
}

static int
mue_wait_for_bits(struct mue_softc *sc, uint32_t reg,
    uint32_t set, uint32_t clear, uint32_t fail)
{
	uint32_t val;
	int ntries;

	for (ntries = 0; ntries < 1000; ntries++) {
		val = mue_csr_read(sc, reg);
		if ((val & set) || !(val & clear))
			return 0;
		if (val & fail)
			return 1;
		usbd_delay_ms(sc->mue_udev, 1);
	}

	return 1;
}

/* 
 * Get exclusive access to the MII registers.
 */
static void
mue_lock_mii(struct mue_softc *sc)
{
	sc->mue_refcnt++;
	mutex_enter(&sc->mue_mii_lock);
}

static void
mue_unlock_mii(struct mue_softc *sc)
{
	mutex_exit(&sc->mue_mii_lock);
	if (--sc->mue_refcnt < 0)
		usb_detach_wakeupold(sc->mue_dev);
}

static int
mue_miibus_readreg(device_t dev, int phy, int reg)
{
	struct mue_softc *sc = device_private(dev);
	uint32_t val;

	if (sc->mue_dying) {
		DPRINTF(sc, "dying\n");
		return 0;
	}

	if (sc->mue_phyno != phy)
		return 0;

	mue_lock_mii(sc);
	if (MUE_WAIT_CLR(sc, MUE_MII_ACCESS, MUE_MII_ACCESS_BUSY, 0)) {
		mue_unlock_mii(sc);
		MUE_PRINTF(sc, "not ready\n");
		return -1;
	}

	mue_csr_write(sc, MUE_MII_ACCESS, MUE_MII_ACCESS_READ |
	    MUE_MII_ACCESS_BUSY | MUE_MII_ACCESS_REGADDR(reg) |
	    MUE_MII_ACCESS_PHYADDR(phy));

	if (MUE_WAIT_CLR(sc, MUE_MII_ACCESS, MUE_MII_ACCESS_BUSY, 0)) {
		mue_unlock_mii(sc);
		MUE_PRINTF(sc, "timed out\n");
		return -1;
	}

	val = mue_csr_read(sc, MUE_MII_DATA);
	mue_unlock_mii(sc);
	return val & 0xffff;
}

static void
mue_miibus_writereg(device_t dev, int phy, int reg, int data)
{
	struct mue_softc *sc = device_private(dev);

	if (sc->mue_dying) {
		DPRINTF(sc, "dying\n");
		return;
	}

	if (sc->mue_phyno != phy) {
		DPRINTF(sc, "sc->mue_phyno (%d) != phy (%d)\n",
		    sc->mue_phyno, phy);
		return;
	}

	mue_lock_mii(sc);
	if (MUE_WAIT_CLR(sc, MUE_MII_ACCESS, MUE_MII_ACCESS_BUSY, 0)) {
		mue_unlock_mii(sc);
		MUE_PRINTF(sc, "not ready\n");
		return;
	}

	mue_csr_write(sc, MUE_MII_DATA, data);
	mue_csr_write(sc, MUE_MII_ACCESS, MUE_MII_ACCESS_WRITE |
	    MUE_MII_ACCESS_BUSY | MUE_MII_ACCESS_REGADDR(reg) |
	    MUE_MII_ACCESS_PHYADDR(phy));

	if (MUE_WAIT_CLR(sc, MUE_MII_ACCESS, MUE_MII_ACCESS_BUSY, 0))
		MUE_PRINTF(sc, "timed out\n");

	mue_unlock_mii(sc);
}

static void
mue_miibus_statchg(struct ifnet *ifp)
{
	struct mue_softc *sc = ifp->if_softc;
	struct mii_data *mii = GET_MII(sc);
	uint32_t flow, threshold;

	if (mii == NULL || ifp == NULL || (ifp->if_flags & IFF_RUNNING) == 0) {
		DPRINTF(sc, "not ready\n");
		return;
	}

	sc->mue_link = 0;
	if ((mii->mii_media_status & (IFM_ACTIVE | IFM_AVALID)) ==
	    (IFM_ACTIVE | IFM_AVALID)) {
		switch (IFM_SUBTYPE(mii->mii_media_active)) {
		case IFM_10_T:
		case IFM_100_TX:
		case IFM_1000_T:
			sc->mue_link++;
			break;
		default:
			break;
		}
	}

	/* Lost link, do nothing. */
	if (sc->mue_link == 0) {
		DPRINTF(sc, "mii_media_status = 0x%x\n", mii->mii_media_status);
		return;
	}

	if (!(sc->mue_flags & LAN7500)) {
		if (sc->mue_udev->ud_speed == USB_SPEED_SUPER) {
			if (IFM_SUBTYPE(mii->mii_media_active) == IFM_1000_T) {
				/* Disable U2 and enable U1. */
				MUE_CLRBIT(sc, MUE_USB_CFG1,
				    MUE_USB_CFG1_DEV_U2_INIT_EN);
				MUE_SETBIT(sc, MUE_USB_CFG1,
				    MUE_USB_CFG1_DEV_U1_INIT_EN);
			} else {
				/* Enable U1 and U2. */
				MUE_SETBIT(sc, MUE_USB_CFG1,
				    MUE_USB_CFG1_DEV_U1_INIT_EN |
				    MUE_USB_CFG1_DEV_U2_INIT_EN);
			}
		}
	}

	flow = 0;
	/* XXX Linux does not check IFM_FDX flag for 7800. */
	if (IFM_OPTIONS(mii->mii_media_active) & IFM_FDX) {
		if (IFM_OPTIONS(mii->mii_media_active) & IFM_ETH_TXPAUSE)
			flow |= MUE_FLOW_TX_FCEN | MUE_FLOW_PAUSE_TIME;
		if (IFM_OPTIONS(mii->mii_media_active) & IFM_ETH_RXPAUSE)
			flow |= MUE_FLOW_RX_FCEN;
	}

	/* XXX Magic numbers taken from Linux driver. */
	if (sc->mue_flags & LAN7500)
		threshold = 0x820;
	else
		switch (sc->mue_udev->ud_speed) {
		case USB_SPEED_SUPER:
			threshold = 0x817;
			break;
		case USB_SPEED_HIGH:
			threshold = 0x211;
			break;
		default:
			threshold = 0;
			break;
		}

	/* Threshold value should be set before enabling flow. */
	mue_csr_write(sc, (sc->mue_flags & LAN7500) ?
	    MUE_7500_FCT_FLOW : MUE_7800_FCT_FLOW, threshold);
	mue_csr_write(sc, MUE_FLOW, flow);

	DPRINTF(sc, "done\n");
}

/*
 * Set media options.
 */
static int
mue_ifmedia_upd(struct ifnet *ifp)
{
	struct mue_softc *sc = ifp->if_softc;
	struct mii_data *mii = GET_MII(sc);

	sc->mue_link = 0; /* XXX */

	if (mii->mii_instance) {
		struct mii_softc *miisc;
		LIST_FOREACH(miisc, &mii->mii_phys, mii_list)
			mii_phy_reset(miisc);
	}
	return mii_mediachg(mii);
}

/*
 * Report current media status.
 */
static void
mue_ifmedia_sts(struct ifnet *ifp, struct ifmediareq *ifmr)
{
	struct mue_softc *sc = ifp->if_softc;
	struct mii_data *mii = GET_MII(sc);

	mii_pollstat(mii);
	ifmr->ifm_active = mii->mii_media_active;
	ifmr->ifm_status = mii->mii_media_status;
}

static uint8_t
mue_eeprom_getbyte(struct mue_softc *sc, int off, uint8_t *dest)
{
	uint32_t val;

	if (MUE_WAIT_CLR(sc, MUE_E2P_CMD, MUE_E2P_CMD_BUSY, 0)) {
		MUE_PRINTF(sc, "not ready\n");
		return ETIMEDOUT;
	}

	mue_csr_write(sc, MUE_E2P_CMD, MUE_E2P_CMD_READ | MUE_E2P_CMD_BUSY |
	    (off & MUE_E2P_CMD_ADDR_MASK));

	if (MUE_WAIT_CLR(sc, MUE_E2P_CMD, MUE_E2P_CMD_BUSY,
	    MUE_E2P_CMD_TIMEOUT)) {
		MUE_PRINTF(sc, "timed out\n");
		return ETIMEDOUT;
	}

	val = mue_csr_read(sc, MUE_E2P_DATA);
	*dest = val & 0xff;

	return 0;
}

static int
mue_read_eeprom(struct mue_softc *sc, uint8_t *dest, int off, int cnt)
{
	uint32_t val = 0; /* XXX gcc */
	uint8_t byte;
	int i, err;

	/* 
	 * EEPROM pins are muxed with the LED function on LAN7800 device.
	 */
	if (sc->mue_product == USB_PRODUCT_SMSC_LAN7800) {
		val = mue_csr_read(sc, MUE_HW_CFG);
		mue_csr_write(sc, MUE_HW_CFG,
		    val & ~(MUE_HW_CFG_LED0_EN | MUE_HW_CFG_LED1_EN));
	}

	for (i = 0; i < cnt; i++) {
		err = mue_eeprom_getbyte(sc, off + i, &byte);
		if (err)
			break;
		*(dest + i) = byte;
	}

	if (sc->mue_product == USB_PRODUCT_SMSC_LAN7800)
		mue_csr_write(sc, MUE_HW_CFG, val);

	return err ? 1 : 0;
}

static bool
mue_eeprom_present(struct mue_softc *sc)
{
	uint32_t val;
	uint8_t sig;
	int ret;

	if (sc->mue_flags & LAN7500) {
		val = mue_csr_read(sc, MUE_E2P_CMD);
		return val & MUE_E2P_CMD_LOADED;
	} else {
		ret = mue_read_eeprom(sc, &sig, MUE_E2P_IND_OFFSET, 1);
		return (ret == 0) && (sig == MUE_E2P_IND);
	}
}

static int
mue_read_otp_raw(struct mue_softc *sc, uint8_t *dest, int off, int cnt)
{
	uint32_t val;
	int i, err;

	val = mue_csr_read(sc, MUE_OTP_PWR_DN);

	/* Checking if bit is set. */
	if (val & MUE_OTP_PWR_DN_PWRDN_N) {
		/* Clear it, then wait for it to be cleared. */
		mue_csr_write(sc, MUE_OTP_PWR_DN, 0);
		err = MUE_WAIT_CLR(sc, MUE_OTP_PWR_DN, MUE_OTP_PWR_DN_PWRDN_N,
		    0);
		if (err) {
			MUE_PRINTF(sc, "not ready\n");
			return 1;
		}
	}

	/* Start reading the bytes, one at a time. */
	for (i = 0; i < cnt; i++) {
		mue_csr_write(sc, MUE_OTP_ADDR1,
		    ((off + i) >> 8) & MUE_OTP_ADDR1_MASK);
		mue_csr_write(sc, MUE_OTP_ADDR2,
		    ((off + i) & MUE_OTP_ADDR2_MASK));
		mue_csr_write(sc, MUE_OTP_FUNC_CMD, MUE_OTP_FUNC_CMD_READ);
		mue_csr_write(sc, MUE_OTP_CMD_GO, MUE_OTP_CMD_GO_GO);

		err = MUE_WAIT_CLR(sc, MUE_OTP_STATUS, MUE_OTP_STATUS_BUSY, 0);
		if (err) {
			MUE_PRINTF(sc, "timed out\n");
			return 1;
		}
		val = mue_csr_read(sc, MUE_OTP_RD_DATA);
		*(dest + i) = (uint8_t)(val & 0xff);
	}

	return 0;
}

static int
mue_read_otp(struct mue_softc *sc, uint8_t *dest, int off, int cnt)
{
	uint8_t sig;
	int err;

	if (sc->mue_flags & LAN7500)
		return 1;

	err = mue_read_otp_raw(sc, &sig, MUE_OTP_IND_OFFSET, 1);
	if (err)
		return 1;
	switch (sig) {
	case MUE_OTP_IND_1:
		break;
	case MUE_OTP_IND_2:
		off += 0x100;
		break;
	default:
		DPRINTF(sc, "OTP not found\n");
		return 1;
	}
	err = mue_read_otp_raw(sc, dest, off, cnt);
	return err;
}

static void
mue_dataport_write(struct mue_softc *sc, uint32_t sel, uint32_t addr,
    uint32_t cnt, uint32_t *data)
{
	uint32_t i;

	if (MUE_WAIT_SET(sc, MUE_DP_SEL, MUE_DP_SEL_DPRDY, 0)) {
		MUE_PRINTF(sc, "not ready\n");
		return;
	}

	mue_csr_write(sc, MUE_DP_SEL,
	    (mue_csr_read(sc, MUE_DP_SEL) & ~MUE_DP_SEL_RSEL_MASK) | sel);

	for (i = 0; i < cnt; i++) {
		mue_csr_write(sc, MUE_DP_ADDR, addr + i);
		mue_csr_write(sc, MUE_DP_DATA, data[i]);
		mue_csr_write(sc, MUE_DP_CMD, MUE_DP_CMD_WRITE);
		if (MUE_WAIT_SET(sc, MUE_DP_SEL, MUE_DP_SEL_DPRDY, 0)) {
			MUE_PRINTF(sc, "timed out\n");
			return;
		}
	}
}

static void
mue_init_ltm(struct mue_softc *sc)
{
	uint32_t idx[MUE_NUM_LTM_INDEX] = { 0, 0, 0, 0, 0, 0 };
	uint8_t temp[2];
	size_t i;

	if (mue_csr_read(sc, MUE_USB_CFG1) & MUE_USB_CFG1_LTM_ENABLE) {
		if (mue_eeprom_present(sc) &&
		    (mue_read_eeprom(sc, temp, MUE_E2P_LTM_OFFSET, 2) == 0)) {
			if (temp[0] != sizeof(idx)) {
				DPRINTF(sc, "EEPROM: unexpected size\n");
				goto done;
			}
			if (mue_read_eeprom(sc, (uint8_t *)idx, temp[1] << 1,
				sizeof(idx))) {
				DPRINTF(sc, "EEPROM read failed\n");
				goto done;
			}
			DPRINTF(sc, "success\n");
		} else if (mue_read_otp(sc, temp, MUE_E2P_LTM_OFFSET, 2) == 0) {
			if (temp[0] != sizeof(idx)) {
				DPRINTF(sc, "OTP: unexpected size\n");
				goto done;
			}
			if (mue_read_otp(sc, (uint8_t *)idx, temp[1] << 1,
				sizeof(idx))) {
				DPRINTF(sc, "OTP read failed\n");
				goto done;
			}
			DPRINTF(sc, "success\n");
		} else {
			DPRINTF(sc, "nothing to do\n");
		}
	} else {
		DPRINTF(sc, "nothing to do\n");
	}
done:
	for (i = 0; i < __arraycount(idx); i++)
		mue_csr_write(sc, MUE_LTM_INDEX(i), idx[i]);
}

static int
mue_chip_init(struct mue_softc *sc)
{
	uint32_t val;

	if ((sc->mue_flags & LAN7500) &&
	    MUE_WAIT_SET(sc, MUE_PMT_CTL, MUE_PMT_CTL_READY, 0)) {
		MUE_PRINTF(sc, "not ready\n");
			return ETIMEDOUT;
	}

	MUE_SETBIT(sc, MUE_HW_CFG, MUE_HW_CFG_LRST);
	if (MUE_WAIT_CLR(sc, MUE_HW_CFG, MUE_HW_CFG_LRST, 0)) {
		MUE_PRINTF(sc, "timed out\n");
		return ETIMEDOUT;
	}

	/* Respond to the IN token with a NAK. */
	if (sc->mue_flags & LAN7500)
		MUE_SETBIT(sc, MUE_HW_CFG, MUE_HW_CFG_BIR);
	else
		MUE_SETBIT(sc, MUE_USB_CFG0, MUE_USB_CFG0_BIR);

	if (sc->mue_flags & LAN7500) {
		if (sc->mue_udev->ud_speed == USB_SPEED_HIGH)
			val = MUE_7500_HS_BUFSIZE /
			    MUE_HS_USB_PKT_SIZE;
		else
			val = MUE_7500_FS_BUFSIZE /
			    MUE_FS_USB_PKT_SIZE;
		mue_csr_write(sc, MUE_7500_BURST_CAP, val);
		mue_csr_write(sc, MUE_7500_BULKIN_DELAY,
		    MUE_7500_DEFAULT_BULKIN_DELAY);

		MUE_SETBIT(sc, MUE_HW_CFG, MUE_HW_CFG_BCE | MUE_HW_CFG_MEF);

		/* Set FIFO sizes. */
		val = (MUE_7500_MAX_RX_FIFO_SIZE - 512) / 512;
		mue_csr_write(sc, MUE_7500_FCT_RX_FIFO_END, val);
		val = (MUE_7500_MAX_TX_FIFO_SIZE - 512) / 512;
		mue_csr_write(sc, MUE_7500_FCT_TX_FIFO_END, val);
	} else {
		/* Init LTM. */
		mue_init_ltm(sc);

		val = MUE_7800_BUFSIZE;
		switch (sc->mue_udev->ud_speed) {
		case USB_SPEED_SUPER:
			val /= MUE_SS_USB_PKT_SIZE;
			break;
		case USB_SPEED_HIGH:
			val /= MUE_HS_USB_PKT_SIZE;
			break;
		default:
			val /= MUE_FS_USB_PKT_SIZE;
			break;
		}
		mue_csr_write(sc, MUE_7800_BURST_CAP, val);
		mue_csr_write(sc, MUE_7800_BULKIN_DELAY,
		    MUE_7800_DEFAULT_BULKIN_DELAY);

		MUE_SETBIT(sc, MUE_HW_CFG, MUE_HW_CFG_MEF);
		MUE_SETBIT(sc, MUE_USB_CFG0, MUE_USB_CFG0_BCE);

		/*
		 * Set FCL's RX and TX FIFO sizes: according to data sheet this
		 * is already the default value. But we initialize it to the
		 * same value anyways, as that's what the Linux driver does.
		 */
		val = (MUE_7800_MAX_RX_FIFO_SIZE - 512) / 512;
		mue_csr_write(sc, MUE_7800_FCT_RX_FIFO_END, val);
		val = (MUE_7800_MAX_TX_FIFO_SIZE - 512) / 512;
		mue_csr_write(sc, MUE_7800_FCT_TX_FIFO_END, val);
	}

	/* Enabling interrupts. */
	mue_csr_write(sc, MUE_INT_STATUS, ~0);

	mue_csr_write(sc, (sc->mue_flags & LAN7500) ?
	    MUE_7500_FCT_FLOW : MUE_7800_FCT_FLOW, 0);
	mue_csr_write(sc, MUE_FLOW, 0);
 
	/* Reset PHY. */
	MUE_SETBIT(sc, MUE_PMT_CTL, MUE_PMT_CTL_PHY_RST);
	if (MUE_WAIT_CLR(sc, MUE_PMT_CTL, MUE_PMT_CTL_PHY_RST, 0)) {
		MUE_PRINTF(sc, "PHY not ready\n");
		return ETIMEDOUT;
	}

	/* LAN7801 only has RGMII mode. */
	if (sc->mue_product == USB_PRODUCT_SMSC_LAN7801)
		MUE_CLRBIT(sc, MUE_MAC_CR, MUE_MAC_CR_GMII_EN);

	if ((sc->mue_flags & LAN7500) ||
	    (sc->mue_product == USB_PRODUCT_SMSC_LAN7800 &&
	    !mue_eeprom_present(sc))) {
		/* Allow MAC to detect speed and duplex from PHY. */
		MUE_SETBIT(sc, MUE_MAC_CR, MUE_MAC_CR_AUTO_SPEED |
		    MUE_MAC_CR_AUTO_DUPLEX);
	}

	MUE_SETBIT(sc, MUE_MAC_TX, MUE_MAC_TX_TXEN);
	MUE_SETBIT(sc, (sc->mue_flags & LAN7500) ?
	    MUE_7500_FCT_TX_CTL : MUE_7800_FCT_TX_CTL, MUE_FCT_TX_CTL_EN);

	/* Set the maximum frame size. */
	MUE_CLRBIT(sc, MUE_MAC_RX, MUE_MAC_RX_RXEN);
	val = mue_csr_read(sc, MUE_MAC_RX);
	val &= ~MUE_MAC_RX_MAX_SIZE_MASK;
	val |= MUE_MAC_RX_MAX_LEN(ETHER_MAX_LEN);
	mue_csr_write(sc, MUE_MAC_RX, val);
	MUE_SETBIT(sc, MUE_MAC_RX, MUE_MAC_RX_RXEN);

	MUE_SETBIT(sc, (sc->mue_flags & LAN7500) ?
	    MUE_7500_FCT_RX_CTL : MUE_7800_FCT_RX_CTL, MUE_FCT_RX_CTL_EN);

	/* Set default GPIO/LED settings only if no EEPROM is detected. */
	if ((sc->mue_flags & LAN7500) && !mue_eeprom_present(sc)) {
		MUE_CLRBIT(sc, MUE_LED_CFG, MUE_LED_CFG_LED10_FUN_SEL);
		MUE_SETBIT(sc, MUE_LED_CFG,
		    MUE_LED_CFG_LEDGPIO_EN | MUE_LED_CFG_LED2_FUN_SEL);
	}

	/* XXX We assume two LEDs at least when EEPROM is missing. */
	if (sc->mue_product == USB_PRODUCT_SMSC_LAN7800 &&
	    !mue_eeprom_present(sc))
		MUE_SETBIT(sc, MUE_HW_CFG,
		    MUE_HW_CFG_LED0_EN | MUE_HW_CFG_LED1_EN);

	return 0;
}

static void
mue_set_macaddr(struct mue_softc *sc)
{
	struct ifnet *ifp = GET_IFP(sc);
	const uint8_t *enaddr = CLLADDR(ifp->if_sadl);
	uint32_t lo, hi;

	lo = MUE_ENADDR_LO(enaddr);
	hi = MUE_ENADDR_HI(enaddr);

	mue_csr_write(sc, MUE_RX_ADDRL, lo);
	mue_csr_write(sc, MUE_RX_ADDRH, hi);
}

static int
mue_get_macaddr(struct mue_softc *sc, prop_dictionary_t dict)
{
	prop_data_t eaprop;
	uint32_t low, high;

	if (!(sc->mue_flags & LAN7500)) {
		low  = mue_csr_read(sc, MUE_RX_ADDRL);
		high = mue_csr_read(sc, MUE_RX_ADDRH);
		sc->mue_enaddr[5] = (uint8_t)((high >> 8) & 0xff);
		sc->mue_enaddr[4] = (uint8_t)((high) & 0xff);
		sc->mue_enaddr[3] = (uint8_t)((low >> 24) & 0xff);
		sc->mue_enaddr[2] = (uint8_t)((low >> 16) & 0xff);
		sc->mue_enaddr[1] = (uint8_t)((low >> 8) & 0xff);
		sc->mue_enaddr[0] = (uint8_t)((low) & 0xff);
		if (ETHER_IS_VALID(sc->mue_enaddr))
			return 0;
		else {
			DPRINTF(sc, "registers: %s\n",
			    ether_sprintf(sc->mue_enaddr));
		}
	}

	if (mue_eeprom_present(sc) && !mue_read_eeprom(sc, sc->mue_enaddr,
	    MUE_E2P_MAC_OFFSET, ETHER_ADDR_LEN)) {
		if (ETHER_IS_VALID(sc->mue_enaddr))
			return 0;
		else {
			DPRINTF(sc, "EEPROM: %s\n",
			    ether_sprintf(sc->mue_enaddr));
		}
	}

	if (mue_read_otp(sc, sc->mue_enaddr, MUE_OTP_MAC_OFFSET,
	    ETHER_ADDR_LEN) == 0) {
		if (ETHER_IS_VALID(sc->mue_enaddr))
			return 0;
		else {
			DPRINTF(sc, "OTP: %s\n",
			    ether_sprintf(sc->mue_enaddr));
		}
	}

	/*
	 * Other MD methods. This should be tried only if other methods fail.
	 * Otherwise, MAC address for internal device can be assinged to
	 * external devices on Raspberry Pi, for example.
	 */
	eaprop = prop_dictionary_get(dict, "mac-address");
	if (eaprop != NULL) {
		KASSERT(prop_object_type(eaprop) == PROP_TYPE_DATA);
		KASSERT(prop_data_size(eaprop) == ETHER_ADDR_LEN);
		memcpy(sc->mue_enaddr, prop_data_data_nocopy(eaprop),
		    ETHER_ADDR_LEN);
		if (ETHER_IS_VALID(sc->mue_enaddr))
			return 0;
		else {
			DPRINTF(sc, "prop_dictionary_get: %s\n",
			    ether_sprintf(sc->mue_enaddr));
		}
	}

	return 1;
}


/* 
 * Probe for a Microchip chip.  */
static int
mue_match(device_t parent, cfdata_t match, void *aux)
{
	struct usb_attach_arg *uaa = aux;

	return (MUE_LOOKUP(uaa) != NULL) ?  UMATCH_VENDOR_PRODUCT : UMATCH_NONE;
}

static void
mue_attach(device_t parent, device_t self, void *aux)
{
	struct mue_softc *sc = device_private(self);
	prop_dictionary_t dict = device_properties(self);
	struct usb_attach_arg *uaa = aux;
	struct usbd_device *dev = uaa->uaa_device;
	usb_interface_descriptor_t *id;
	usb_endpoint_descriptor_t *ed;
	char *devinfop;
	struct mii_data	*mii;
	struct ifnet *ifp;
	usbd_status err;
	int i, s;

	aprint_naive("\n");
	aprint_normal("\n");

	sc->mue_dev = self;
	sc->mue_udev = dev;

	devinfop = usbd_devinfo_alloc(sc->mue_udev, 0);
	aprint_normal_dev(self, "%s\n", devinfop);
	usbd_devinfo_free(devinfop);

#define MUE_CONFIG_NO	1
	err = usbd_set_config_no(dev, MUE_CONFIG_NO, 1);
	if (err) {
		aprint_error_dev(self, "failed to set configuration: %s\n",
		    usbd_errstr(err));
		return;
	}

	mutex_init(&sc->mue_mii_lock, MUTEX_DEFAULT, IPL_NONE);
	usb_init_task(&sc->mue_tick_task, mue_tick_task, sc, 0);
	usb_init_task(&sc->mue_stop_task, (void (*)(void *))mue_stop, sc, 0);

#define MUE_IFACE_IDX	0
	err = usbd_device2interface_handle(dev, MUE_IFACE_IDX, &sc->mue_iface);
	if (err) {
		aprint_error_dev(self, "failed to get interface handle: %s\n",
		    usbd_errstr(err));
		return;
	}

	sc->mue_product = uaa->uaa_product;
	sc->mue_flags = MUE_LOOKUP(uaa)->mue_flags;

	/* Decide on what our bufsize will be. */
	if (sc->mue_flags & LAN7500)
		sc->mue_bufsz = (sc->mue_udev->ud_speed == USB_SPEED_HIGH) ?
		    MUE_7500_HS_BUFSIZE : MUE_7500_FS_BUFSIZE;
	else
		sc->mue_bufsz = MUE_7800_BUFSIZE;

	/* Find endpoints. */
	id = usbd_get_interface_descriptor(sc->mue_iface);
	for (i = 0; i < id->bNumEndpoints; i++) {
		ed = usbd_interface2endpoint_descriptor(sc->mue_iface, i);
		if (ed == NULL) {
			aprint_error_dev(self, "couldn't get ep %d\n", i);
			return;
		}
		if (UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_IN &&
		    UE_GET_XFERTYPE(ed->bmAttributes) == UE_BULK) {
			sc->mue_ed[MUE_ENDPT_RX] = ed->bEndpointAddress;
		} else if (UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_OUT &&
			   UE_GET_XFERTYPE(ed->bmAttributes) == UE_BULK) {
			sc->mue_ed[MUE_ENDPT_TX] = ed->bEndpointAddress;
		} else if (UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_IN &&
			   UE_GET_XFERTYPE(ed->bmAttributes) == UE_INTERRUPT) {
			sc->mue_ed[MUE_ENDPT_INTR] = ed->bEndpointAddress;
		}
	}
	KASSERT(sc->mue_ed[MUE_ENDPT_RX] != 0);
	KASSERT(sc->mue_ed[MUE_ENDPT_TX] != 0);
	KASSERT(sc->mue_ed[MUE_ENDPT_INTR] != 0);

	s = splnet();

	sc->mue_phyno = 1;

	if (mue_chip_init(sc)) {
		aprint_error_dev(self, "chip initialization failed\n");
		splx(s);
		return;
	}

	/* A Microchip chip was detected.  Inform the world. */
	if (sc->mue_flags & LAN7500)
		aprint_normal_dev(self, "LAN7500\n");
	else
		aprint_normal_dev(self, "LAN7800\n");

	if (mue_get_macaddr(sc, dict)) {
		aprint_error_dev(self, "Ethernet address assigned randomly\n");
		cprng_fast(sc->mue_enaddr, ETHER_ADDR_LEN);
		sc->mue_enaddr[0] &= ~0x01;	/* unicast */
		sc->mue_enaddr[0] |= 0x02;	/* locally administered */
	}

	aprint_normal_dev(self, "Ethernet address %s\n",
	    ether_sprintf(sc->mue_enaddr));

	/* Initialize interface info.*/
	ifp = GET_IFP(sc);
	ifp->if_softc = sc;
	strlcpy(ifp->if_xname, device_xname(sc->mue_dev), IFNAMSIZ);
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_init = mue_init;
	ifp->if_ioctl = mue_ioctl;
	ifp->if_start = mue_start;
	ifp->if_stop = mue_stop;
	ifp->if_watchdog = mue_watchdog;

	IFQ_SET_READY(&ifp->if_snd);

	sc->mue_ec.ec_capabilities = ETHERCAP_VLAN_MTU;

	/* Initialize MII/media info. */
	mii = GET_MII(sc);
	mii->mii_ifp = ifp;
	mii->mii_readreg = mue_miibus_readreg;
	mii->mii_writereg = mue_miibus_writereg;
	mii->mii_statchg = mue_miibus_statchg;
	mii->mii_flags = MIIF_AUTOTSLEEP;

	sc->mue_ec.ec_mii = mii;
	ifmedia_init(&mii->mii_media, 0, mue_ifmedia_upd, mue_ifmedia_sts);
	mii_attach(self, mii, 0xffffffff, MII_PHY_ANY, MII_OFFSET_ANY, 0);

	if (LIST_FIRST(&mii->mii_phys) == NULL) {
		ifmedia_add(&mii->mii_media, IFM_ETHER | IFM_NONE, 0, NULL);
		ifmedia_set(&mii->mii_media, IFM_ETHER | IFM_NONE);
	} else
		ifmedia_set(&mii->mii_media, IFM_ETHER | IFM_AUTO);

	/* Attach the interface. */
	if_attach(ifp);
	ether_ifattach(ifp, sc->mue_enaddr);

	rnd_attach_source(&sc->mue_rnd_source, device_xname(sc->mue_dev),
	    RND_TYPE_NET, RND_FLAG_DEFAULT);

	callout_init(&sc->mue_stat_ch, 0);

	splx(s);

	usbd_add_drv_event(USB_EVENT_DRIVER_ATTACH, sc->mue_udev, sc->mue_dev);
}

static int
mue_detach(device_t self, int flags)
{
	struct mue_softc *sc = device_private(self);
	struct ifnet *ifp = GET_IFP(sc);
	size_t i;
	int s;

	sc->mue_dying = true;

	callout_halt(&sc->mue_stat_ch, NULL);

	for (i = 0; i < __arraycount(sc->mue_ep); i++)
		if (sc->mue_ep[i] != NULL)
			usbd_abort_pipe(sc->mue_ep[i]);

	/*
	 * Remove any pending tasks.  They cannot be executing because they run
	 * in the same thread as detach.
	 */
	usb_rem_task_wait(sc->mue_udev, &sc->mue_tick_task, USB_TASKQ_DRIVER,
	    NULL);
	usb_rem_task_wait(sc->mue_udev, &sc->mue_stop_task, USB_TASKQ_DRIVER,
	    NULL);

	s = splusb();

	if (ifp->if_flags & IFF_RUNNING)
		mue_stop(ifp, 1);

	rnd_detach_source(&sc->mue_rnd_source);
	mii_detach(&sc->mue_mii, MII_PHY_ANY, MII_OFFSET_ANY);
	ifmedia_delete_instance(&sc->mue_mii.mii_media, IFM_INST_ANY);
	if (ifp->if_softc != NULL) {
		ether_ifdetach(ifp);
		if_detach(ifp);
	}

	if (--sc->mue_refcnt >= 0) {
		/* Wait for processes to go away. */
		usb_detach_waitold(sc->mue_dev);
	}
	splx(s);

	usbd_add_drv_event(USB_EVENT_DRIVER_DETACH, sc->mue_udev, sc->mue_dev);
	
	mutex_destroy(&sc->mue_mii_lock);

	return 0;
}

static int
mue_activate(device_t self, enum devact act)
{
	struct mue_softc *sc = device_private(self);
	struct ifnet *ifp = GET_IFP(sc);

	switch (act) {
	case DVACT_DEACTIVATE:
		if_deactivate(ifp);
		sc->mue_dying = true;
		return 0;
	default:
		return EOPNOTSUPP;
	}
	return 0;
}

static int
mue_rx_list_init(struct mue_softc *sc)
{
	struct mue_cdata *cd;
	struct mue_chain *c;
	size_t i;
	int err;

	cd = &sc->mue_cdata;
	for (i = 0; i < __arraycount(cd->mue_rx_chain); i++) {
		c = &cd->mue_rx_chain[i];
		c->mue_sc = sc;
		c->mue_idx = i;
		if (c->mue_xfer == NULL) {
			err = usbd_create_xfer(sc->mue_ep[MUE_ENDPT_RX],
			    sc->mue_bufsz, 0, 0, &c->mue_xfer);
			if (err)
				return err;
			c->mue_buf = usbd_get_buffer(c->mue_xfer);
		}
	}

	return 0;
}

static int
mue_tx_list_init(struct mue_softc *sc)
{
	struct mue_cdata *cd;
	struct mue_chain *c;
	size_t i;
	int err;

	cd = &sc->mue_cdata;
	for (i = 0; i < __arraycount(cd->mue_tx_chain); i++) {
		c = &cd->mue_tx_chain[i];
		c->mue_sc = sc;
		c->mue_idx = i;
		if (c->mue_xfer == NULL) {
			err = usbd_create_xfer(sc->mue_ep[MUE_ENDPT_TX],
			    sc->mue_bufsz, USBD_FORCE_SHORT_XFER, 0,
			    &c->mue_xfer);
			if (err)
				return err;
			c->mue_buf = usbd_get_buffer(c->mue_xfer);
		}
	}

	return 0;
}

static int
mue_open_pipes(struct mue_softc *sc)
{
	usbd_status err;

	/* Open RX and TX pipes. */
	err = usbd_open_pipe(sc->mue_iface, sc->mue_ed[MUE_ENDPT_RX],
	    USBD_EXCLUSIVE_USE, &sc->mue_ep[MUE_ENDPT_RX]);
	if (err) {
		MUE_PRINTF(sc, "rx pipe: %s\n", usbd_errstr(err));
		return EIO;
	}
	err = usbd_open_pipe(sc->mue_iface, sc->mue_ed[MUE_ENDPT_TX],
	    USBD_EXCLUSIVE_USE, &sc->mue_ep[MUE_ENDPT_TX]);
	if (err) {
		MUE_PRINTF(sc, "tx pipe: %s\n", usbd_errstr(err));
		return EIO;
	}
	return 0;
}

static void
mue_start_rx(struct mue_softc *sc)
{
	struct mue_chain *c;
	size_t i;

	/* Start up the receive pipe. */
	for (i = 0; i < __arraycount(sc->mue_cdata.mue_rx_chain); i++) {
		c = &sc->mue_cdata.mue_rx_chain[i];
		usbd_setup_xfer(c->mue_xfer, c, c->mue_buf, sc->mue_bufsz,
		    USBD_SHORT_XFER_OK, USBD_NO_TIMEOUT, mue_rxeof);
		usbd_transfer(c->mue_xfer);
	}
}

static int
mue_encap(struct mue_softc *sc, struct mbuf *m, int idx)
{
	struct ifnet *ifp = GET_IFP(sc);
	struct mue_chain *c;
	usbd_status err;
	struct mue_txbuf_hdr hdr;
	int len;

	c = &sc->mue_cdata.mue_tx_chain[idx];

	hdr.tx_cmd_a = htole32((m->m_pkthdr.len & MUE_TX_CMD_A_LEN_MASK) |
	    MUE_TX_CMD_A_FCS);
	/* Disable segmentation offload. */
	hdr.tx_cmd_b = htole32(0);
	memcpy(c->mue_buf, &hdr, sizeof(hdr)); 
	len = sizeof(hdr);

	m_copydata(m, 0, m->m_pkthdr.len, c->mue_buf + len);
	len += m->m_pkthdr.len;

	usbd_setup_xfer(c->mue_xfer, c, c->mue_buf, len,
	    USBD_FORCE_SHORT_XFER, 10000, mue_txeof);

	/* Transmit */
	err = usbd_transfer(c->mue_xfer);
	if (__predict_false(err != USBD_IN_PROGRESS)) {
		DPRINTF(sc, "%s\n", usbd_errstr(err));
		mue_stop(ifp, 0);
		return EIO;
	}

	sc->mue_cdata.mue_tx_cnt++;

	return 0;
}

static void
mue_setmulti(struct mue_softc *sc)
{
	struct ifnet *ifp = GET_IFP(sc);
	const uint8_t *enaddr = CLLADDR(ifp->if_sadl);
	struct ether_multi *enm;
	struct ether_multistep step;
	uint32_t pfiltbl[MUE_NUM_ADDR_FILTX][2];
	uint32_t hashtbl[MUE_DP_SEL_VHF_HASH_LEN];
	uint32_t reg, rxfilt, h, hireg, loreg;
	int i;

	if (sc->mue_dying)
		return;

	/* Clear perfect filter and hash tables. */
	memset(pfiltbl, 0, sizeof(pfiltbl));
	memset(hashtbl, 0, sizeof(hashtbl));

	reg = (sc->mue_flags & LAN7500) ? MUE_7500_RFE_CTL : MUE_7800_RFE_CTL;
	rxfilt = mue_csr_read(sc, reg);
	rxfilt &= ~(MUE_RFE_CTL_PERFECT | MUE_RFE_CTL_MULTICAST_HASH |
	    MUE_RFE_CTL_UNICAST | MUE_RFE_CTL_MULTICAST);

	/* Always accept broadcast frames. */
	rxfilt |= MUE_RFE_CTL_BROADCAST;

	if (ifp->if_flags & (IFF_ALLMULTI | IFF_PROMISC)) {
allmulti:	rxfilt |= MUE_RFE_CTL_MULTICAST;
		if (ifp->if_flags & IFF_PROMISC) {
			rxfilt |= MUE_RFE_CTL_UNICAST;
			DPRINTF(sc, "promisc\n");
		} else {
			DPRINTF(sc, "allmulti\n");
		}
	} else {
		/* Now program new ones. */
		pfiltbl[0][0] = MUE_ENADDR_HI(enaddr) | MUE_ADDR_FILTX_VALID;
		pfiltbl[0][1] = MUE_ENADDR_LO(enaddr);
		i = 1;
		ETHER_FIRST_MULTI(step, &sc->mue_ec, enm);
		while (enm != NULL) {
			if (memcmp(enm->enm_addrlo, enm->enm_addrhi,
			    ETHER_ADDR_LEN)) {
				memset(pfiltbl, 0, sizeof(pfiltbl));
				memset(hashtbl, 0, sizeof(hashtbl));
				rxfilt &= ~MUE_RFE_CTL_MULTICAST_HASH;
				goto allmulti;
			}
			if (i < MUE_NUM_ADDR_FILTX) {
				/* Use perfect address table if possible. */
				pfiltbl[i][0] = MUE_ENADDR_HI(enm->enm_addrlo) |
				    MUE_ADDR_FILTX_VALID;
				pfiltbl[i][1] = MUE_ENADDR_LO(enm->enm_addrlo);
			} else {
				/* Otherwise, use hash table. */
				rxfilt |= MUE_RFE_CTL_MULTICAST_HASH;
				h = (ether_crc32_be(enm->enm_addrlo,
				    ETHER_ADDR_LEN) >> 23) & 0x1ff;
				hashtbl[h / 32] |= 1 << (h % 32); 
			}
			i++;
			ETHER_NEXT_MULTI(step, enm);
		}
		rxfilt |= MUE_RFE_CTL_PERFECT;
		if (rxfilt & MUE_RFE_CTL_MULTICAST_HASH) {
			DPRINTF(sc, "perfect filter and hash tables\n");
		} else {
			DPRINTF(sc, "perfect filter\n");
		}
	}

	for (i = 0; i < MUE_NUM_ADDR_FILTX; i++) {
		hireg = (sc->mue_flags & LAN7500) ?
		    MUE_7500_ADDR_FILTX(i) : MUE_7800_ADDR_FILTX(i);
		loreg = hireg + 4;
		mue_csr_write(sc, hireg, 0);
		mue_csr_write(sc, loreg, pfiltbl[i][1]);
		mue_csr_write(sc, hireg, pfiltbl[i][0]);
	}

	mue_dataport_write(sc, MUE_DP_SEL_VHF, MUE_DP_SEL_VHF_VLAN_LEN,
	    MUE_DP_SEL_VHF_HASH_LEN, hashtbl);

	mue_csr_write(sc, reg, rxfilt);
}

static void
mue_sethwcsum(struct mue_softc *sc)
{
	struct ifnet *ifp = GET_IFP(sc);
	uint32_t reg, val;

	reg = (sc->mue_flags & LAN7500) ? MUE_7500_RFE_CTL : MUE_7800_RFE_CTL;
	val = mue_csr_read(sc, reg);

	if (ifp->if_capenable & (IFCAP_CSUM_TCPv4_Rx|IFCAP_CSUM_UDPv4_Rx)) {
		DPRINTF(sc, "enabled\n");;
		val |= MUE_RFE_CTL_IGMP_COE | MUE_RFE_CTL_ICMP_COE;
		val |= MUE_RFE_CTL_TCPUDP_COE | MUE_RFE_CTL_IP_COE;
	} else {
		DPRINTF(sc, "disabled\n");;
		val &=
		    ~(MUE_RFE_CTL_IGMP_COE | MUE_RFE_CTL_ICMP_COE);
		val &=
		    ~(MUE_RFE_CTL_TCPUDP_COE | MUE_RFE_CTL_IP_COE);
        }

	val &= ~MUE_RFE_CTL_VLAN_FILTER;

	mue_csr_write(sc, reg, val);
}


static void
mue_rxeof(struct usbd_xfer *xfer, void *priv, usbd_status status)
{
	struct mue_chain *c = (struct mue_chain *)priv;
	struct mue_softc *sc = c->mue_sc;
	struct ifnet *ifp = GET_IFP(sc);
	struct mbuf *m;
	struct mue_rxbuf_hdr *hdrp;
	uint32_t rx_cmd_a, total_len;
	uint16_t pktlen;
	int s;
	char *buf = c->mue_buf;

	if (__predict_false(sc->mue_dying)) {
		DPRINTF(sc, "dying\n");
		return;
	}

	if (__predict_false(!(ifp->if_flags & IFF_RUNNING))) {
		DPRINTF(sc, "not running\n");
		return;
	}

	if (__predict_false(status != USBD_NORMAL_COMPLETION)) {
		DPRINTF(sc, "%s\n", usbd_errstr(status));
		if (status == USBD_NOT_STARTED || status == USBD_CANCELLED)
			return;
		if (usbd_ratecheck(&sc->mue_rx_notice))
			MUE_PRINTF(sc, "%s\n", usbd_errstr(status));
		if (status == USBD_STALLED)
			usbd_clear_endpoint_stall_async(
			    sc->mue_ep[MUE_ENDPT_RX]);
		goto done;
	}

	usbd_get_xfer_status(xfer, NULL, NULL, &total_len, NULL);

	if (__predict_false(total_len > sc->mue_bufsz)) {
		DPRINTF(sc, "too large transfer\n");
		goto done;
	}

	do {
		if (__predict_false(total_len < sizeof(*hdrp))) {
			DPRINTF(sc, "too short transfer\n");
			ifp->if_ierrors++;
			goto done;
		}

		hdrp = (struct mue_rxbuf_hdr *)buf;
		rx_cmd_a = le32toh(hdrp->rx_cmd_a);

		if (__predict_false(rx_cmd_a & MUE_RX_CMD_A_RED)) {
			DPRINTF(sc, "rx_cmd_a: 0x%x\n", rx_cmd_a);
			ifp->if_ierrors++;
			goto done;
		}

		/* XXX not yet */
		KASSERT((rx_cmd_a & MUE_RX_CMD_A_ICSM) == 0);

		pktlen = (uint16_t)(rx_cmd_a & MUE_RX_CMD_A_LEN_MASK);
		if (sc->mue_flags & LAN7500)
			pktlen -= 2;

		if (__predict_false(pktlen < ETHER_HDR_LEN ||
		    pktlen > MCLBYTES - ETHER_ALIGN ||
		    pktlen + sizeof(*hdrp) > total_len)) {
			DPRINTF(sc, "bad pktlen\n");
			ifp->if_ierrors++;
			goto done;
		}

		m = mue_newbuf();
		if (__predict_false(m == NULL)) {
			DPRINTF(sc, "mbuf allocation failed\n");
			ifp->if_ierrors++;
			goto done;
		}

		m_set_rcvif(m, ifp);
		m->m_pkthdr.len = m->m_len = pktlen;
		m->m_flags |= M_HASFCS;
		memcpy(mtod(m, char *), buf + sizeof(*hdrp), pktlen);

		/* Attention: sizeof(hdr) = 10 */
		pktlen = roundup(pktlen + sizeof(*hdrp), 4);
		if (pktlen > total_len)
			pktlen = total_len;
		total_len -= pktlen;
		buf += pktlen;

		s = splnet();
		if_percpuq_enqueue(ifp->if_percpuq, m);
		splx(s);
	} while (total_len > 0);

done:
	/* Setup new transfer. */
	usbd_setup_xfer(xfer, c, c->mue_buf, sc->mue_bufsz,
	    USBD_SHORT_XFER_OK, USBD_NO_TIMEOUT, mue_rxeof);
	usbd_transfer(xfer);
}

static void
mue_txeof(struct usbd_xfer *xfer, void *priv, usbd_status status)
{
	struct mue_chain *c = priv;
	struct mue_softc *sc = c->mue_sc;
	struct ifnet *ifp = GET_IFP(sc);
	int s;

	if (__predict_false(sc->mue_dying))
		return;

	s = splnet();


	if (__predict_false(status != USBD_NORMAL_COMPLETION)) {
		if (status == USBD_NOT_STARTED || status == USBD_CANCELLED) {
			splx(s);
			return;
		}
		ifp->if_oerrors++;
		MUE_PRINTF(sc, "%s\n", usbd_errstr(status));
		if (status == USBD_STALLED)
			usbd_clear_endpoint_stall_async(
			    sc->mue_ep[MUE_ENDPT_TX]);
		splx(s);
		return;
	}

	ifp->if_timer = 0;
	ifp->if_flags &= ~IFF_OACTIVE;

	if (!IFQ_IS_EMPTY(&ifp->if_snd))
		mue_start(ifp);

	ifp->if_opackets++;
	splx(s);
}

static int
mue_init(struct ifnet *ifp)
{
	struct mue_softc *sc = ifp->if_softc; 
	int s;

	if (sc->mue_dying) {
		DPRINTF(sc, "dying\n");
		return EIO;
	}

	s = splnet();

	/* Cancel pending I/O and free all TX/RX buffers. */
	if (ifp->if_flags & IFF_RUNNING)
		mue_stop(ifp, 1);

	mue_reset(sc);

	/* Set MAC address. */
	mue_set_macaddr(sc);

	/* Load the multicast filter. */
	mue_setmulti(sc);

	/* TCP/UDP checksum offload engines. */
	mue_sethwcsum(sc);

	if (mue_open_pipes(sc)) {
		splx(s);
		return EIO;
	}

	/* Init RX ring. */
	if (mue_rx_list_init(sc)) {
		MUE_PRINTF(sc, "rx list init failed\n");
		splx(s);
		return ENOBUFS;
	}

	/* Init TX ring. */
	if (mue_tx_list_init(sc)) {
		MUE_PRINTF(sc, "tx list init failed\n");
		splx(s);
		return ENOBUFS;
	}

	mue_start_rx(sc);

	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;

	splx(s);

	callout_reset(&sc->mue_stat_ch, hz, mue_tick, sc);

	return 0;
}

static int
mue_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	struct mue_softc *sc = ifp->if_softc;
	struct ifreq /*const*/ *ifr = data;
	int s, error = 0;

	s = splnet();

	switch(cmd) {
	case SIOCSIFFLAGS:
		if ((error = ifioctl_common(ifp, cmd, data)) != 0)
			break;

		switch (ifp->if_flags & (IFF_UP | IFF_RUNNING)) {
		case IFF_RUNNING:
			mue_stop(ifp, 1);
			break;
		case IFF_UP:
			mue_init(ifp);
			break;
		case IFF_UP | IFF_RUNNING:
			if ((ifp->if_flags ^ sc->mue_if_flags) == IFF_PROMISC)
				mue_setmulti(sc);
			else
				mue_init(ifp);
			break;
		}
		sc->mue_if_flags = ifp->if_flags;
		break;
	case SIOCGIFMEDIA:
	case SIOCSIFMEDIA:
		error = ifmedia_ioctl(ifp, ifr, &sc->mue_mii.mii_media, cmd);
		break;
	default:
		if ((error = ether_ioctl(ifp, cmd, data)) != ENETRESET)
			break;
		error = 0;
		if (cmd == SIOCADDMULTI || cmd == SIOCDELMULTI)
			mue_setmulti(sc);
		break;
	}
	splx(s);

	return error;
}

static void
mue_watchdog(struct ifnet *ifp)
{
	struct mue_softc *sc = ifp->if_softc;
	struct mue_chain *c;
	usbd_status stat;
	int s;

	ifp->if_oerrors++;
	MUE_PRINTF(sc, "timed out\n");

	s = splusb();
	c = &sc->mue_cdata.mue_tx_chain[0];
	usbd_get_xfer_status(c->mue_xfer, NULL, NULL, NULL, &stat);
	mue_txeof(c->mue_xfer, c, stat);

	if (!IFQ_IS_EMPTY(&ifp->if_snd))
		mue_start(ifp);
	splx(s);
}

static void
mue_reset(struct mue_softc *sc)
{
	if (sc->mue_dying)
		return;

	/* Wait a little while for the chip to get its brains in order. */
	usbd_delay_ms(sc->mue_udev, 1);

//	mue_chip_init(sc); /* XXX */
}

static void
mue_start(struct ifnet *ifp)
{
	struct mue_softc *sc = ifp->if_softc;
	struct mbuf *m;

	if (__predict_false(!sc->mue_link)) {
		DPRINTF(sc, "no link\n");
		return;
	}

	if (__predict_false((ifp->if_flags & (IFF_OACTIVE|IFF_RUNNING))
	    != IFF_RUNNING)) {
		DPRINTF(sc, "not ready\n");
		return;
	}

	IFQ_POLL(&ifp->if_snd, m);
	if (m == NULL)
		return;

	if (__predict_false(mue_encap(sc, m, 0))) {
		DPRINTF(sc, "encap failed\n");
		ifp->if_flags |= IFF_OACTIVE;
		return;
	}
	IFQ_DEQUEUE(&ifp->if_snd, m);

	bpf_mtap(ifp, m, BPF_D_OUT);
	m_freem(m);

	ifp->if_flags |= IFF_OACTIVE;

	/* Set a timeout in case the chip goes out to lunch. */
	ifp->if_timer = 5;
}

static void
mue_stop(struct ifnet *ifp, int disable __unused)
{
	struct mue_softc *sc = ifp->if_softc;
	usbd_status err;
	size_t i;

	mue_reset(sc);

	ifp->if_timer = 0;
	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);

	callout_stop(&sc->mue_stat_ch);

        /* Stop transfers. */
	for (i = 0; i < __arraycount(sc->mue_ep); i++)
		if (sc->mue_ep[i] != NULL) {
			err = usbd_abort_pipe(sc->mue_ep[i]);
			if (err)
				MUE_PRINTF(sc, "abort pipe %zu: %s\n",
				    i, usbd_errstr(err));
		}

	/* Free RX resources. */
	for (i = 0; i < __arraycount(sc->mue_cdata.mue_rx_chain); i++)
		if (sc->mue_cdata.mue_rx_chain[i].mue_xfer != NULL) {
			usbd_destroy_xfer(
			    sc->mue_cdata.mue_rx_chain[i].mue_xfer);
			sc->mue_cdata.mue_rx_chain[i].mue_xfer = NULL;
		}

	/* Free TX resources. */
	for (i = 0; i < __arraycount(sc->mue_cdata.mue_tx_chain); i++)
		if (sc->mue_cdata.mue_tx_chain[i].mue_xfer != NULL) {
			usbd_destroy_xfer(
			    sc->mue_cdata.mue_tx_chain[i].mue_xfer);
			sc->mue_cdata.mue_tx_chain[i].mue_xfer = NULL;
		}

	/* Close pipes */
	for (i = 0; i < __arraycount(sc->mue_ep); i++)
		if (sc->mue_ep[i] != NULL) {
			err = usbd_close_pipe(sc->mue_ep[i]);
			if (err)
				MUE_PRINTF(sc, "close pipe %zu: %s\n",
				    i, usbd_errstr(err));
			sc->mue_ep[i] = NULL;
		}

	sc->mue_link = 0; /* XXX */

	DPRINTF(sc, "done\n");
}

static void
mue_tick(void *xsc)
{
	struct mue_softc *sc = xsc;

	if (sc == NULL)
		return;

	if (sc->mue_dying)
		return;

	/* Perform periodic stuff in process context. */
	usb_add_task(sc->mue_udev, &sc->mue_tick_task, USB_TASKQ_DRIVER);
}

static void
mue_tick_task(void *xsc)
{
	struct mue_softc *sc = xsc;
	struct ifnet *ifp = GET_IFP(sc);
	struct mii_data *mii = GET_MII(sc);
	int s;

	if (sc == NULL)
		return;

	if (sc->mue_dying)
		return;

	s = splnet();
	mii_tick(mii);
	if (sc->mue_link == 0)
		mue_miibus_statchg(ifp);
	callout_reset(&sc->mue_stat_ch, hz, mue_tick, sc);
	splx(s);
}

static struct mbuf *
mue_newbuf(void)
{
	struct mbuf *m;

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (__predict_false(m == NULL))
		return NULL;

	MCLGET(m, M_DONTWAIT);
	if (__predict_false(!(m->m_flags & M_EXT))) {
		m_freem(m);
		return NULL;
	}

	m_adj(m, ETHER_ALIGN);

	return m;
}
