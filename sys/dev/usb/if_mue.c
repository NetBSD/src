/*	$NetBSD: if_mue.c,v 1.63 2022/03/03 05:48:06 riastradh Exp $	*/
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
__KERNEL_RCSID(0, "$NetBSD: if_mue.c,v 1.63 2022/03/03 05:48:06 riastradh Exp $");

#ifdef _KERNEL_OPT
#include "opt_usb.h"
#include "opt_inet.h"
#endif

#include <sys/param.h>

#include <dev/usb/usbnet.h>

#include <dev/usb/if_muereg.h>
#include <dev/usb/if_muevar.h>

#define MUE_PRINTF(un, fmt, args...)					\
	device_printf((un)->un_dev, "%s: " fmt, __func__, ##args);

#ifdef USB_DEBUG
int muedebug = 0;
#define DPRINTF(un, fmt, args...)					\
	do {								\
		if (muedebug)						\
			MUE_PRINTF(un, fmt, ##args);			\
	} while (0 /* CONSTCOND */)
#else
#define DPRINTF(un, fmt, args...)	__nothing
#endif

/*
 * Various supported device vendors/products.
 */
struct mue_type {
	struct usb_devno	mue_dev;
	uint16_t		mue_flags;
#define LAN7500		0x0001	/* LAN7500 */
#define LAN7800		0x0002	/* LAN7800 */
#define LAN7801		0x0004	/* LAN7801 */
#define LAN7850		0x0008	/* LAN7850 */
};

static const struct mue_type mue_devs[] = {
	{ { USB_VENDOR_SMSC, USB_PRODUCT_SMSC_LAN7500 }, LAN7500 },
	{ { USB_VENDOR_SMSC, USB_PRODUCT_SMSC_LAN7505 }, LAN7500 },
	{ { USB_VENDOR_SMSC, USB_PRODUCT_SMSC_LAN7800 }, LAN7800 },
	{ { USB_VENDOR_SMSC, USB_PRODUCT_SMSC_LAN7801 }, LAN7801 },
	{ { USB_VENDOR_SMSC, USB_PRODUCT_SMSC_LAN7850 }, LAN7850 }
};

#define MUE_LOOKUP(uaa)	((const struct mue_type *)usb_lookup(mue_devs, \
    uaa->uaa_vendor, uaa->uaa_product))

#define MUE_ENADDR_LO(enaddr) \
    ((enaddr[3] << 24) | (enaddr[2] << 16) | (enaddr[1] << 8) | enaddr[0])
#define MUE_ENADDR_HI(enaddr) \
    ((enaddr[5] << 8) | enaddr[4])

static int	mue_match(device_t, cfdata_t, void *);
static void	mue_attach(device_t, device_t, void *);

static uint32_t	mue_csr_read(struct usbnet *, uint32_t);
static int	mue_csr_write(struct usbnet *, uint32_t, uint32_t);
static int	mue_wait_for_bits(struct usbnet *, uint32_t, uint32_t,
		    uint32_t, uint32_t);
static uint8_t	mue_eeprom_getbyte(struct usbnet *, int, uint8_t *);
static bool	mue_eeprom_present(struct usbnet *);
static void	mue_dataport_write(struct usbnet *, uint32_t, uint32_t,
		    uint32_t, uint32_t *);
static void	mue_init_ltm(struct usbnet *);
static int	mue_chip_init(struct usbnet *);
static void	mue_set_macaddr(struct usbnet *);
static int	mue_get_macaddr(struct usbnet *, prop_dictionary_t);
static int	mue_prepare_tso(struct usbnet *, struct mbuf *);
static void	mue_setiff_locked(struct usbnet *);
static void	mue_sethwcsum_locked(struct usbnet *);
static void	mue_setmtu_locked(struct usbnet *);
static void	mue_reset(struct usbnet *);

static void	mue_uno_stop(struct ifnet *, int);
static int	mue_uno_ioctl(struct ifnet *, u_long, void *);
static int	mue_uno_mii_read_reg(struct usbnet *, int, int, uint16_t *);
static int	mue_uno_mii_write_reg(struct usbnet *, int, int, uint16_t);
static void	mue_uno_mii_statchg(struct ifnet *);
static void	mue_uno_rx_loop(struct usbnet *, struct usbnet_chain *,
				uint32_t);
static unsigned	mue_uno_tx_prepare(struct usbnet *, struct mbuf *,
				   struct usbnet_chain *);
static int	mue_uno_init(struct ifnet *);

static const struct usbnet_ops mue_ops = {
	.uno_stop = mue_uno_stop,
	.uno_ioctl = mue_uno_ioctl,
	.uno_read_reg = mue_uno_mii_read_reg,
	.uno_write_reg = mue_uno_mii_write_reg,
	.uno_statchg = mue_uno_mii_statchg,
	.uno_tx_prepare = mue_uno_tx_prepare,
	.uno_rx_loop = mue_uno_rx_loop,
	.uno_init = mue_uno_init,
};

#define MUE_SETBIT(un, reg, x)	\
	mue_csr_write(un, reg, mue_csr_read(un, reg) | (x))

#define MUE_CLRBIT(un, reg, x)	\
	mue_csr_write(un, reg, mue_csr_read(un, reg) & ~(x))

#define MUE_WAIT_SET(un, reg, set, fail)	\
	mue_wait_for_bits(un, reg, set, ~0, fail)

#define MUE_WAIT_CLR(un, reg, clear, fail)	\
	mue_wait_for_bits(un, reg, 0, clear, fail)

#define ETHER_IS_VALID(addr) \
	(!ETHER_IS_MULTICAST(addr) && !ETHER_IS_ZERO(addr))

#define ETHER_IS_ZERO(addr) \
	(!(addr[0] | addr[1] | addr[2] | addr[3] | addr[4] | addr[5]))

CFATTACH_DECL_NEW(mue, sizeof(struct usbnet), mue_match, mue_attach,
    usbnet_detach, usbnet_activate);

static uint32_t
mue_csr_read(struct usbnet *un, uint32_t reg)
{
	usb_device_request_t req;
	usbd_status err;
	uDWord val;

	if (usbnet_isdying(un))
		return 0;

	USETDW(val, 0);
	req.bmRequestType = UT_READ_VENDOR_DEVICE;
	req.bRequest = MUE_UR_READREG;
	USETW(req.wValue, 0);
	USETW(req.wIndex, reg);
	USETW(req.wLength, 4);

	err = usbd_do_request(un->un_udev, &req, &val);
	if (err) {
		MUE_PRINTF(un, "reg = %#x: %s\n", reg, usbd_errstr(err));
		return 0;
	}

	return UGETDW(val);
}

static int
mue_csr_write(struct usbnet *un, uint32_t reg, uint32_t aval)
{
	usb_device_request_t req;
	usbd_status err;
	uDWord val;

	if (usbnet_isdying(un))
		return 0;

	USETDW(val, aval);
	req.bmRequestType = UT_WRITE_VENDOR_DEVICE;
	req.bRequest = MUE_UR_WRITEREG;
	USETW(req.wValue, 0);
	USETW(req.wIndex, reg);
	USETW(req.wLength, 4);

	err = usbd_do_request(un->un_udev, &req, &val);
	if (err) {
		MUE_PRINTF(un, "reg = %#x: %s\n", reg, usbd_errstr(err));
		return -1;
	}

	return 0;
}

static int
mue_wait_for_bits(struct usbnet *un, uint32_t reg,
    uint32_t set, uint32_t clear, uint32_t fail)
{
	uint32_t val;
	int ntries;

	for (ntries = 0; ntries < 1000; ntries++) {
		val = mue_csr_read(un, reg);
		if ((val & set) || !(val & clear))
			return 0;
		if (val & fail)
			return 1;
		usbd_delay_ms(un->un_udev, 1);
	}

	return 1;
}

static int
mue_uno_mii_read_reg(struct usbnet *un, int phy, int reg, uint16_t *val)
{
	uint32_t data;

	if (un->un_phyno != phy)
		return EINVAL;

	if (MUE_WAIT_CLR(un, MUE_MII_ACCESS, MUE_MII_ACCESS_BUSY, 0)) {
		MUE_PRINTF(un, "not ready\n");
		return EBUSY;
	}

	mue_csr_write(un, MUE_MII_ACCESS, MUE_MII_ACCESS_READ |
	    MUE_MII_ACCESS_BUSY | MUE_MII_ACCESS_REGADDR(reg) |
	    MUE_MII_ACCESS_PHYADDR(phy));

	if (MUE_WAIT_CLR(un, MUE_MII_ACCESS, MUE_MII_ACCESS_BUSY, 0)) {
		MUE_PRINTF(un, "timed out\n");
		return ETIMEDOUT;
	}

	data = mue_csr_read(un, MUE_MII_DATA);
	*val = data & 0xffff;

	return 0;
}

static int
mue_uno_mii_write_reg(struct usbnet *un, int phy, int reg, uint16_t val)
{

	if (un->un_phyno != phy)
		return EINVAL;

	if (MUE_WAIT_CLR(un, MUE_MII_ACCESS, MUE_MII_ACCESS_BUSY, 0)) {
		MUE_PRINTF(un, "not ready\n");
		return EBUSY;
	}

	mue_csr_write(un, MUE_MII_DATA, val);
	mue_csr_write(un, MUE_MII_ACCESS, MUE_MII_ACCESS_WRITE |
	    MUE_MII_ACCESS_BUSY | MUE_MII_ACCESS_REGADDR(reg) |
	    MUE_MII_ACCESS_PHYADDR(phy));

	if (MUE_WAIT_CLR(un, MUE_MII_ACCESS, MUE_MII_ACCESS_BUSY, 0)) {
		MUE_PRINTF(un, "timed out\n");
		return ETIMEDOUT;
	}

	return 0;
}

static void
mue_uno_mii_statchg(struct ifnet *ifp)
{
	struct usbnet * const un = ifp->if_softc;
	struct mii_data * const mii = usbnet_mii(un);
	uint32_t flow, threshold;

	if (usbnet_isdying(un))
		return;

	if ((mii->mii_media_status & (IFM_ACTIVE | IFM_AVALID)) ==
	    (IFM_ACTIVE | IFM_AVALID)) {
		switch (IFM_SUBTYPE(mii->mii_media_active)) {
		case IFM_10_T:
		case IFM_100_TX:
		case IFM_1000_T:
			usbnet_set_link(un, true);
			break;
		default:
			break;
		}
	}

	/* Lost link, do nothing. */
	if (!usbnet_havelink(un)) {
		DPRINTF(un, "mii_media_status = %#x\n", mii->mii_media_status);
		return;
	}

	if (!(un->un_flags & LAN7500)) {
		if (un->un_udev->ud_speed == USB_SPEED_SUPER) {
			if (IFM_SUBTYPE(mii->mii_media_active) == IFM_1000_T) {
				/* Disable U2 and enable U1. */
				MUE_CLRBIT(un, MUE_USB_CFG1,
				    MUE_USB_CFG1_DEV_U2_INIT_EN);
				MUE_SETBIT(un, MUE_USB_CFG1,
				    MUE_USB_CFG1_DEV_U1_INIT_EN);
			} else {
				/* Enable U1 and U2. */
				MUE_SETBIT(un, MUE_USB_CFG1,
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
	if (un->un_flags & LAN7500)
		threshold = 0x820;
	else
		switch (un->un_udev->ud_speed) {
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
	mue_csr_write(un, (un->un_flags & LAN7500) ?
	    MUE_7500_FCT_FLOW : MUE_7800_FCT_FLOW, threshold);
	mue_csr_write(un, MUE_FLOW, flow);

	DPRINTF(un, "done\n");
}

static uint8_t
mue_eeprom_getbyte(struct usbnet *un, int off, uint8_t *dest)
{
	uint32_t val;

	if (MUE_WAIT_CLR(un, MUE_E2P_CMD, MUE_E2P_CMD_BUSY, 0)) {
		MUE_PRINTF(un, "not ready\n");
		return ETIMEDOUT;
	}

	KASSERT((off & ~MUE_E2P_CMD_ADDR_MASK) == 0);
	mue_csr_write(un, MUE_E2P_CMD, MUE_E2P_CMD_READ | MUE_E2P_CMD_BUSY |
	    off);

	if (MUE_WAIT_CLR(un, MUE_E2P_CMD, MUE_E2P_CMD_BUSY,
	    MUE_E2P_CMD_TIMEOUT)) {
		MUE_PRINTF(un, "timed out\n");
		return ETIMEDOUT;
	}

	val = mue_csr_read(un, MUE_E2P_DATA);
	*dest = val & 0xff;

	return 0;
}

static int
mue_read_eeprom(struct usbnet *un, uint8_t *dest, int off, int cnt)
{
	uint32_t val = 0; /* XXX gcc */
	uint8_t byte;
	int i, err = 0;

	/* 
	 * EEPROM pins are muxed with the LED function on LAN7800 device.
	 */
	if (un->un_flags & LAN7800) {
		val = mue_csr_read(un, MUE_HW_CFG);
		mue_csr_write(un, MUE_HW_CFG,
		    val & ~(MUE_HW_CFG_LED0_EN | MUE_HW_CFG_LED1_EN));
	}

	for (i = 0; i < cnt; i++) {
		err = mue_eeprom_getbyte(un, off + i, &byte);
		if (err)
			break;
		*(dest + i) = byte;
	}

	if (un->un_flags & LAN7800)
		mue_csr_write(un, MUE_HW_CFG, val);

	return err ? 1 : 0;
}

static bool
mue_eeprom_present(struct usbnet *un)
{
	uint32_t val;
	uint8_t sig;
	int ret;

	if (un->un_flags & LAN7500) {
		val = mue_csr_read(un, MUE_E2P_CMD);
		return val & MUE_E2P_CMD_LOADED;
	} else {
		ret = mue_read_eeprom(un, &sig, MUE_E2P_IND_OFFSET, 1);
		return (ret == 0) && (sig == MUE_E2P_IND);
	}
}

static int
mue_read_otp_raw(struct usbnet *un, uint8_t *dest, int off, int cnt)
{
	uint32_t val;
	int i, err;

	val = mue_csr_read(un, MUE_OTP_PWR_DN);

	/* Checking if bit is set. */
	if (val & MUE_OTP_PWR_DN_PWRDN_N) {
		/* Clear it, then wait for it to be cleared. */
		mue_csr_write(un, MUE_OTP_PWR_DN, 0);
		err = MUE_WAIT_CLR(un, MUE_OTP_PWR_DN, MUE_OTP_PWR_DN_PWRDN_N,
		    0);
		if (err) {
			MUE_PRINTF(un, "not ready\n");
			return 1;
		}
	}

	/* Start reading the bytes, one at a time. */
	for (i = 0; i < cnt; i++) {
		mue_csr_write(un, MUE_OTP_ADDR1,
		    ((off + i) >> 8) & MUE_OTP_ADDR1_MASK);
		mue_csr_write(un, MUE_OTP_ADDR2,
		    ((off + i) & MUE_OTP_ADDR2_MASK));
		mue_csr_write(un, MUE_OTP_FUNC_CMD, MUE_OTP_FUNC_CMD_READ);
		mue_csr_write(un, MUE_OTP_CMD_GO, MUE_OTP_CMD_GO_GO);

		err = MUE_WAIT_CLR(un, MUE_OTP_STATUS, MUE_OTP_STATUS_BUSY, 0);
		if (err) {
			MUE_PRINTF(un, "timed out\n");
			return 1;
		}
		val = mue_csr_read(un, MUE_OTP_RD_DATA);
		*(dest + i) = (uint8_t)(val & 0xff);
	}

	return 0;
}

static int
mue_read_otp(struct usbnet *un, uint8_t *dest, int off, int cnt)
{
	uint8_t sig;
	int err;

	if (un->un_flags & LAN7500)
		return 1;

	err = mue_read_otp_raw(un, &sig, MUE_OTP_IND_OFFSET, 1);
	if (err)
		return 1;
	switch (sig) {
	case MUE_OTP_IND_1:
		break;
	case MUE_OTP_IND_2:
		off += 0x100;
		break;
	default:
		DPRINTF(un, "OTP not found\n");
		return 1;
	}
	err = mue_read_otp_raw(un, dest, off, cnt);
	return err;
}

static void
mue_dataport_write(struct usbnet *un, uint32_t sel, uint32_t addr,
    uint32_t cnt, uint32_t *data)
{
	uint32_t i;

	if (MUE_WAIT_SET(un, MUE_DP_SEL, MUE_DP_SEL_DPRDY, 0)) {
		MUE_PRINTF(un, "not ready\n");
		return;
	}

	mue_csr_write(un, MUE_DP_SEL,
	    (mue_csr_read(un, MUE_DP_SEL) & ~MUE_DP_SEL_RSEL_MASK) | sel);

	for (i = 0; i < cnt; i++) {
		mue_csr_write(un, MUE_DP_ADDR, addr + i);
		mue_csr_write(un, MUE_DP_DATA, data[i]);
		mue_csr_write(un, MUE_DP_CMD, MUE_DP_CMD_WRITE);
		if (MUE_WAIT_SET(un, MUE_DP_SEL, MUE_DP_SEL_DPRDY, 0)) {
			MUE_PRINTF(un, "timed out\n");
			return;
		}
	}
}

static void
mue_init_ltm(struct usbnet *un)
{
	uint32_t idx[MUE_NUM_LTM_INDEX] = { 0, 0, 0, 0, 0, 0 };
	uint8_t temp[2];
	size_t i;

	if (mue_csr_read(un, MUE_USB_CFG1) & MUE_USB_CFG1_LTM_ENABLE) {
		if (mue_eeprom_present(un) &&
		    (mue_read_eeprom(un, temp, MUE_E2P_LTM_OFFSET, 2) == 0)) {
			if (temp[0] != sizeof(idx)) {
				DPRINTF(un, "EEPROM: unexpected size\n");
				goto done;
			}
			if (mue_read_eeprom(un, (uint8_t *)idx, temp[1] << 1,
				sizeof(idx))) {
				DPRINTF(un, "EEPROM: failed to read\n");
				goto done;
			}
			DPRINTF(un, "success\n");
		} else if (mue_read_otp(un, temp, MUE_E2P_LTM_OFFSET, 2) == 0) {
			if (temp[0] != sizeof(idx)) {
				DPRINTF(un, "OTP: unexpected size\n");
				goto done;
			}
			if (mue_read_otp(un, (uint8_t *)idx, temp[1] << 1,
				sizeof(idx))) {
				DPRINTF(un, "OTP: failed to read\n");
				goto done;
			}
			DPRINTF(un, "success\n");
		} else
			DPRINTF(un, "nothing to do\n");
	} else
		DPRINTF(un, "nothing to do\n");
done:
	for (i = 0; i < __arraycount(idx); i++)
		mue_csr_write(un, MUE_LTM_INDEX(i), idx[i]);
}

static int
mue_chip_init(struct usbnet *un)
{
	uint32_t val;

	if ((un->un_flags & LAN7500) &&
	    MUE_WAIT_SET(un, MUE_PMT_CTL, MUE_PMT_CTL_READY, 0)) {
		MUE_PRINTF(un, "not ready\n");
			return ETIMEDOUT;
	}

	MUE_SETBIT(un, MUE_HW_CFG, MUE_HW_CFG_LRST);
	if (MUE_WAIT_CLR(un, MUE_HW_CFG, MUE_HW_CFG_LRST, 0)) {
		MUE_PRINTF(un, "timed out\n");
		return ETIMEDOUT;
	}

	/* Respond to the IN token with a NAK. */
	if (un->un_flags & LAN7500)
		MUE_SETBIT(un, MUE_HW_CFG, MUE_HW_CFG_BIR);
	else
		MUE_SETBIT(un, MUE_USB_CFG0, MUE_USB_CFG0_BIR);

	if (un->un_flags & LAN7500) {
		if (un->un_udev->ud_speed == USB_SPEED_HIGH)
			val = MUE_7500_HS_RX_BUFSIZE /
			    MUE_HS_USB_PKT_SIZE;
		else
			val = MUE_7500_FS_RX_BUFSIZE /
			    MUE_FS_USB_PKT_SIZE;
		mue_csr_write(un, MUE_7500_BURST_CAP, val);
		mue_csr_write(un, MUE_7500_BULKIN_DELAY,
		    MUE_7500_DEFAULT_BULKIN_DELAY);

		MUE_SETBIT(un, MUE_HW_CFG, MUE_HW_CFG_BCE | MUE_HW_CFG_MEF);

		/* Set FIFO sizes. */
		val = (MUE_7500_MAX_RX_FIFO_SIZE - 512) / 512;
		mue_csr_write(un, MUE_7500_FCT_RX_FIFO_END, val);
		val = (MUE_7500_MAX_TX_FIFO_SIZE - 512) / 512;
		mue_csr_write(un, MUE_7500_FCT_TX_FIFO_END, val);
	} else {
		/* Init LTM. */
		mue_init_ltm(un);

		val = MUE_7800_RX_BUFSIZE;
		switch (un->un_udev->ud_speed) {
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
		mue_csr_write(un, MUE_7800_BURST_CAP, val);
		mue_csr_write(un, MUE_7800_BULKIN_DELAY,
		    MUE_7800_DEFAULT_BULKIN_DELAY);

		MUE_SETBIT(un, MUE_HW_CFG, MUE_HW_CFG_MEF);
		MUE_SETBIT(un, MUE_USB_CFG0, MUE_USB_CFG0_BCE);

		/*
		 * Set FCL's RX and TX FIFO sizes: according to data sheet this
		 * is already the default value. But we initialize it to the
		 * same value anyways, as that's what the Linux driver does.
		 */
		val = (MUE_7800_MAX_RX_FIFO_SIZE - 512) / 512;
		mue_csr_write(un, MUE_7800_FCT_RX_FIFO_END, val);
		val = (MUE_7800_MAX_TX_FIFO_SIZE - 512) / 512;
		mue_csr_write(un, MUE_7800_FCT_TX_FIFO_END, val);
	}

	/* Enabling interrupts. */
	mue_csr_write(un, MUE_INT_STATUS, ~0);

	mue_csr_write(un, (un->un_flags & LAN7500) ?
	    MUE_7500_FCT_FLOW : MUE_7800_FCT_FLOW, 0);
	mue_csr_write(un, MUE_FLOW, 0);

	/* Reset PHY. */
	MUE_SETBIT(un, MUE_PMT_CTL, MUE_PMT_CTL_PHY_RST);
	if (MUE_WAIT_CLR(un, MUE_PMT_CTL, MUE_PMT_CTL_PHY_RST, 0)) {
		MUE_PRINTF(un, "PHY not ready\n");
		return ETIMEDOUT;
	}

	/* LAN7801 only has RGMII mode. */
	if (un->un_flags & LAN7801)
		MUE_CLRBIT(un, MUE_MAC_CR, MUE_MAC_CR_GMII_EN);

	if ((un->un_flags & (LAN7500 | LAN7800)) ||
	    !mue_eeprom_present(un)) {
		/* Allow MAC to detect speed and duplex from PHY. */
		MUE_SETBIT(un, MUE_MAC_CR, MUE_MAC_CR_AUTO_SPEED |
		    MUE_MAC_CR_AUTO_DUPLEX);
	}

	MUE_SETBIT(un, MUE_MAC_TX, MUE_MAC_TX_TXEN);
	MUE_SETBIT(un, (un->un_flags & LAN7500) ?
	    MUE_7500_FCT_TX_CTL : MUE_7800_FCT_TX_CTL, MUE_FCT_TX_CTL_EN);

	MUE_SETBIT(un, (un->un_flags & LAN7500) ?
	    MUE_7500_FCT_RX_CTL : MUE_7800_FCT_RX_CTL, MUE_FCT_RX_CTL_EN);

	/* Set default GPIO/LED settings only if no EEPROM is detected. */
	if ((un->un_flags & LAN7500) && !mue_eeprom_present(un)) {
		MUE_CLRBIT(un, MUE_LED_CFG, MUE_LED_CFG_LED10_FUN_SEL);
		MUE_SETBIT(un, MUE_LED_CFG,
		    MUE_LED_CFG_LEDGPIO_EN | MUE_LED_CFG_LED2_FUN_SEL);
	}

	/* XXX We assume two LEDs at least when EEPROM is missing. */
	if (un->un_flags & LAN7800 &&
	    !mue_eeprom_present(un))
		MUE_SETBIT(un, MUE_HW_CFG,
		    MUE_HW_CFG_LED0_EN | MUE_HW_CFG_LED1_EN);

	return 0;
}

static void
mue_set_macaddr(struct usbnet *un)
{
	struct ifnet * const ifp = usbnet_ifp(un);
	const uint8_t *enaddr = CLLADDR(ifp->if_sadl);
	uint32_t lo, hi;

	lo = MUE_ENADDR_LO(enaddr);
	hi = MUE_ENADDR_HI(enaddr);

	mue_csr_write(un, MUE_RX_ADDRL, lo);
	mue_csr_write(un, MUE_RX_ADDRH, hi);
}

static int
mue_get_macaddr(struct usbnet *un, prop_dictionary_t dict)
{
	prop_data_t eaprop;
	uint32_t low, high;

	if (!(un->un_flags & LAN7500)) {
		low  = mue_csr_read(un, MUE_RX_ADDRL);
		high = mue_csr_read(un, MUE_RX_ADDRH);
		un->un_eaddr[5] = (uint8_t)((high >> 8) & 0xff);
		un->un_eaddr[4] = (uint8_t)((high) & 0xff);
		un->un_eaddr[3] = (uint8_t)((low >> 24) & 0xff);
		un->un_eaddr[2] = (uint8_t)((low >> 16) & 0xff);
		un->un_eaddr[1] = (uint8_t)((low >> 8) & 0xff);
		un->un_eaddr[0] = (uint8_t)((low) & 0xff);
		if (ETHER_IS_VALID(un->un_eaddr))
			return 0;
		else
			DPRINTF(un, "registers: %s\n",
			    ether_sprintf(un->un_eaddr));
	}

	if (mue_eeprom_present(un) && !mue_read_eeprom(un, un->un_eaddr,
	    MUE_E2P_MAC_OFFSET, ETHER_ADDR_LEN)) {
		if (ETHER_IS_VALID(un->un_eaddr))
			return 0;
		else
			DPRINTF(un, "EEPROM: %s\n",
			    ether_sprintf(un->un_eaddr));
	}

	if (mue_read_otp(un, un->un_eaddr, MUE_OTP_MAC_OFFSET,
	    ETHER_ADDR_LEN) == 0) {
		if (ETHER_IS_VALID(un->un_eaddr))
			return 0;
		else
			DPRINTF(un, "OTP: %s\n",
			    ether_sprintf(un->un_eaddr));
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
		memcpy(un->un_eaddr, prop_data_value(eaprop),
		    ETHER_ADDR_LEN);
		if (ETHER_IS_VALID(un->un_eaddr))
			return 0;
		else
			DPRINTF(un, "prop_dictionary_get: %s\n",
			    ether_sprintf(un->un_eaddr));
	}

	return 1;
}


/* 
 * Probe for a Microchip chip.
 */
static int
mue_match(device_t parent, cfdata_t match, void *aux)
{
	struct usb_attach_arg *uaa = aux;

	return (MUE_LOOKUP(uaa) != NULL) ?  UMATCH_VENDOR_PRODUCT : UMATCH_NONE;
}

static void
mue_attach(device_t parent, device_t self, void *aux)
{
	USBNET_MII_DECL_DEFAULT(unm);
	struct usbnet * const un = device_private(self);
	prop_dictionary_t dict = device_properties(self);
	struct usb_attach_arg *uaa = aux;
	struct usbd_device *dev = uaa->uaa_device;
	usb_interface_descriptor_t *id;
	usb_endpoint_descriptor_t *ed;
	char *devinfop;
	usbd_status err;
	const char *descr;
	uint32_t id_rev;
	uint8_t i;
	unsigned rx_list_cnt, tx_list_cnt;
	unsigned rx_bufsz;

	aprint_naive("\n");
	aprint_normal("\n");
	devinfop = usbd_devinfo_alloc(dev, 0);
	aprint_normal_dev(self, "%s\n", devinfop);
	usbd_devinfo_free(devinfop);

	un->un_dev = self;
	un->un_udev = dev;
	un->un_sc = un;
	un->un_ops = &mue_ops;
	un->un_rx_xfer_flags = USBD_SHORT_XFER_OK;
	un->un_tx_xfer_flags = USBD_FORCE_SHORT_XFER;

#define MUE_CONFIG_NO	1
	err = usbd_set_config_no(dev, MUE_CONFIG_NO, 1);
	if (err) {
		aprint_error_dev(self, "failed to set configuration: %s\n",
		    usbd_errstr(err));
		return;
	}

#define MUE_IFACE_IDX	0
	err = usbd_device2interface_handle(dev, MUE_IFACE_IDX, &un->un_iface);
	if (err) {
		aprint_error_dev(self, "failed to get interface handle: %s\n",
		    usbd_errstr(err));
		return;
	}

	un->un_flags = MUE_LOOKUP(uaa)->mue_flags;

	/* Decide on what our bufsize will be. */
	if (un->un_flags & LAN7500) {
		rx_bufsz = (un->un_udev->ud_speed == USB_SPEED_HIGH) ?
		    MUE_7500_HS_RX_BUFSIZE : MUE_7500_FS_RX_BUFSIZE;
		rx_list_cnt = 1;
		tx_list_cnt = 1;
	} else {
		rx_bufsz = MUE_7800_RX_BUFSIZE;
		rx_list_cnt = MUE_RX_LIST_CNT;
		tx_list_cnt = MUE_TX_LIST_CNT;
	}

	un->un_rx_list_cnt = rx_list_cnt;
	un->un_tx_list_cnt = tx_list_cnt;
	un->un_rx_bufsz = rx_bufsz;
	un->un_tx_bufsz = MUE_TX_BUFSIZE;

	/* Find endpoints. */
	id = usbd_get_interface_descriptor(un->un_iface);
	for (i = 0; i < id->bNumEndpoints; i++) {
		ed = usbd_interface2endpoint_descriptor(un->un_iface, i);
		if (ed == NULL) {
			aprint_error_dev(self, "failed to get ep %hhd\n", i);
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
	if (un->un_ed[USBNET_ENDPT_RX] == 0 ||
	    un->un_ed[USBNET_ENDPT_TX] == 0 ||
	    un->un_ed[USBNET_ENDPT_INTR] == 0) {
		aprint_error_dev(self, "failed to find endpoints\n");
		return;
	}

	/* Set these up now for mue_cmd().  */
	usbnet_attach(un, "muedet");

	un->un_phyno = 1;

	if (mue_chip_init(un)) {
		aprint_error_dev(self, "failed to initialize chip\n");
		return;
	}

	/* A Microchip chip was detected.  Inform the world. */
	id_rev = mue_csr_read(un, MUE_ID_REV);
	descr = (un->un_flags & LAN7500) ? "LAN7500" : "LAN7800";
	aprint_normal_dev(self, "%s id %#x rev %#x\n", descr,
		(unsigned)__SHIFTOUT(id_rev, MUE_ID_REV_ID),
		(unsigned)__SHIFTOUT(id_rev, MUE_ID_REV_REV));

	if (mue_get_macaddr(un, dict)) {
		aprint_error_dev(self, "failed to read MAC address\n");
		return;
	}

	struct ifnet *ifp = usbnet_ifp(un);
	ifp->if_capabilities = IFCAP_TSOv4 | IFCAP_TSOv6 |
	    IFCAP_CSUM_IPv4_Tx | IFCAP_CSUM_IPv4_Rx |
	    IFCAP_CSUM_TCPv4_Tx | IFCAP_CSUM_TCPv4_Rx |
	    IFCAP_CSUM_UDPv4_Tx | IFCAP_CSUM_UDPv4_Rx |
	    IFCAP_CSUM_TCPv6_Tx | IFCAP_CSUM_TCPv6_Rx |
	    IFCAP_CSUM_UDPv6_Tx | IFCAP_CSUM_UDPv6_Rx;

	struct ethercom *ec = usbnet_ec(un);
	ec->ec_capabilities = ETHERCAP_VLAN_MTU;
#if 0 /* XXX not yet */
	ec->ec_capabilities = ETHERCAP_VLAN_MTU | ETHERCAP_JUMBO_MTU;
#endif

	usbnet_attach_ifp(un, IFF_SIMPLEX | IFF_BROADCAST | IFF_MULTICAST,
	    0, &unm);
}

static unsigned
mue_uno_tx_prepare(struct usbnet *un, struct mbuf *m, struct usbnet_chain *c)
{
	struct ifnet * const ifp = usbnet_ifp(un);
	struct mue_txbuf_hdr hdr;
	uint32_t tx_cmd_a, tx_cmd_b;
	int csum, len, rv;
	bool tso, ipe, tpe;

	if ((unsigned)m->m_pkthdr.len > un->un_tx_bufsz - sizeof(hdr))
		return 0;

	csum = m->m_pkthdr.csum_flags;
	tso = csum & (M_CSUM_TSOv4 | M_CSUM_TSOv6);
	ipe = csum & M_CSUM_IPv4;
	tpe = csum & (M_CSUM_TCPv4 | M_CSUM_UDPv4 |
		      M_CSUM_TCPv6 | M_CSUM_UDPv6);

	len = m->m_pkthdr.len;
	if (__predict_false((!tso && len > (int)MUE_FRAME_LEN(ifp->if_mtu)) ||
			    ( tso && len > MUE_TSO_FRAME_LEN))) {
		MUE_PRINTF(un, "packet length %d\n too long", len);
		return 0;
	}

	KASSERT((len & ~MUE_TX_CMD_A_LEN_MASK) == 0);
	tx_cmd_a = len | MUE_TX_CMD_A_FCS;

	if (tso) {
		tx_cmd_a |= MUE_TX_CMD_A_LSO;
		if (__predict_true(m->m_pkthdr.segsz > MUE_TX_MSS_MIN))
			tx_cmd_b = m->m_pkthdr.segsz;
		else
			tx_cmd_b = MUE_TX_MSS_MIN;
		tx_cmd_b <<= MUE_TX_CMD_B_MSS_SHIFT;
		KASSERT((tx_cmd_b & ~MUE_TX_CMD_B_MSS_MASK) == 0);
		rv = mue_prepare_tso(un, m);
		if (__predict_false(rv))
			return 0;
	} else {
		if (ipe)
			tx_cmd_a |= MUE_TX_CMD_A_IPE;
		if (tpe)
			tx_cmd_a |= MUE_TX_CMD_A_TPE;
		tx_cmd_b = 0;
	}

	hdr.tx_cmd_a = htole32(tx_cmd_a);
	hdr.tx_cmd_b = htole32(tx_cmd_b);

	memcpy(c->unc_buf, &hdr, sizeof(hdr));
	m_copydata(m, 0, len, c->unc_buf + sizeof(hdr));

	return len + sizeof(hdr);
}

/*
 * L3 length field should be cleared.
 */
static int
mue_prepare_tso(struct usbnet *un, struct mbuf *m)
{
	struct ether_header *eh;
	struct ip *ip;
	struct ip6_hdr *ip6;
	uint16_t type, len = 0;
	int off;

	if (__predict_true(m->m_len >= (int)sizeof(*eh))) {
		eh = mtod(m, struct ether_header *);
		type = eh->ether_type;
	} else
		m_copydata(m, offsetof(struct ether_header, ether_type),
		    sizeof(type), &type);
	switch (type = htons(type)) {
	case ETHERTYPE_IP:
	case ETHERTYPE_IPV6:
		off = ETHER_HDR_LEN;
		break;
	case ETHERTYPE_VLAN:
		off = ETHER_HDR_LEN + ETHER_VLAN_ENCAP_LEN;
		break;
	default:
		return EINVAL;
	}

	if (m->m_pkthdr.csum_flags & M_CSUM_TSOv4) {
		if (__predict_true(m->m_len >= off + (int)sizeof(*ip))) {
			ip = (void *)(mtod(m, char *) + off);
			ip->ip_len = 0;
		} else
			m_copyback(m, off + offsetof(struct ip, ip_len),
			    sizeof(len), &len);
	} else {
		if (__predict_true(m->m_len >= off + (int)sizeof(*ip6))) {
			ip6 = (void *)(mtod(m, char *) + off);
			ip6->ip6_plen = 0;
		} else
			m_copyback(m, off + offsetof(struct ip6_hdr, ip6_plen),
			    sizeof(len), &len);
	}
	return 0;
}

static void
mue_setiff_locked(struct usbnet *un)
{
	struct ethercom *ec = usbnet_ec(un);
	struct ifnet * const ifp = usbnet_ifp(un);
	const uint8_t *enaddr = CLLADDR(ifp->if_sadl);
	struct ether_multi *enm;
	struct ether_multistep step;
	uint32_t pfiltbl[MUE_NUM_ADDR_FILTX][2];
	uint32_t hashtbl[MUE_DP_SEL_VHF_HASH_LEN];
	uint32_t reg, rxfilt, h, hireg, loreg;
	size_t i;

	if (usbnet_isdying(un))
		return;

	/* Clear perfect filter and hash tables. */
	memset(pfiltbl, 0, sizeof(pfiltbl));
	memset(hashtbl, 0, sizeof(hashtbl));

	reg = (un->un_flags & LAN7500) ? MUE_7500_RFE_CTL : MUE_7800_RFE_CTL;
	rxfilt = mue_csr_read(un, reg);
	rxfilt &= ~(MUE_RFE_CTL_PERFECT | MUE_RFE_CTL_MULTICAST_HASH |
	    MUE_RFE_CTL_UNICAST | MUE_RFE_CTL_MULTICAST);

	/* Always accept broadcast frames. */
	rxfilt |= MUE_RFE_CTL_BROADCAST;

	if (ifp->if_flags & IFF_PROMISC) {
		rxfilt |= MUE_RFE_CTL_UNICAST;
allmulti:	rxfilt |= MUE_RFE_CTL_MULTICAST;
		ifp->if_flags |= IFF_ALLMULTI;
		if (ifp->if_flags & IFF_PROMISC)
			DPRINTF(un, "promisc\n");
		else
			DPRINTF(un, "allmulti\n");
	} else {
		/* Now program new ones. */
		pfiltbl[0][0] = MUE_ENADDR_HI(enaddr) | MUE_ADDR_FILTX_VALID;
		pfiltbl[0][1] = MUE_ENADDR_LO(enaddr);
		i = 1;
		ETHER_LOCK(ec);
		ETHER_FIRST_MULTI(step, ec, enm);
		while (enm != NULL) {
			if (memcmp(enm->enm_addrlo, enm->enm_addrhi,
			    ETHER_ADDR_LEN)) {
				memset(pfiltbl, 0, sizeof(pfiltbl));
				memset(hashtbl, 0, sizeof(hashtbl));
				rxfilt &= ~MUE_RFE_CTL_MULTICAST_HASH;
				ETHER_UNLOCK(ec);
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
		ETHER_UNLOCK(ec);
		rxfilt |= MUE_RFE_CTL_PERFECT;
		ifp->if_flags &= ~IFF_ALLMULTI;
		if (rxfilt & MUE_RFE_CTL_MULTICAST_HASH)
			DPRINTF(un, "perfect filter and hash tables\n");
		else
			DPRINTF(un, "perfect filter\n");
	}

	for (i = 0; i < MUE_NUM_ADDR_FILTX; i++) {
		hireg = (un->un_flags & LAN7500) ?
		    MUE_7500_ADDR_FILTX(i) : MUE_7800_ADDR_FILTX(i);
		loreg = hireg + 4;
		mue_csr_write(un, hireg, 0);
		mue_csr_write(un, loreg, pfiltbl[i][1]);
		mue_csr_write(un, hireg, pfiltbl[i][0]);
	}

	mue_dataport_write(un, MUE_DP_SEL_VHF, MUE_DP_SEL_VHF_VLAN_LEN,
	    MUE_DP_SEL_VHF_HASH_LEN, hashtbl);

	mue_csr_write(un, reg, rxfilt);
}

static void
mue_sethwcsum_locked(struct usbnet *un)
{
	struct ifnet * const ifp = usbnet_ifp(un);
	uint32_t reg, val;

	reg = (un->un_flags & LAN7500) ? MUE_7500_RFE_CTL : MUE_7800_RFE_CTL;
	val = mue_csr_read(un, reg);

	if (ifp->if_capenable & IFCAP_CSUM_IPv4_Rx) {
		DPRINTF(un, "RX IPv4 hwcsum enabled\n");
		val |= MUE_RFE_CTL_IP_COE;
	} else {
		DPRINTF(un, "RX IPv4 hwcsum disabled\n");
		val &= ~MUE_RFE_CTL_IP_COE;
	}

	if (ifp->if_capenable &
	    (IFCAP_CSUM_TCPv4_Rx | IFCAP_CSUM_UDPv4_Rx |
	     IFCAP_CSUM_TCPv6_Rx | IFCAP_CSUM_UDPv6_Rx)) {
		DPRINTF(un, "RX L4 hwcsum enabled\n");
		val |= MUE_RFE_CTL_TCPUDP_COE;
	} else {
		DPRINTF(un, "RX L4 hwcsum disabled\n");
		val &= ~MUE_RFE_CTL_TCPUDP_COE;
	}

	val &= ~MUE_RFE_CTL_VLAN_FILTER;

	mue_csr_write(un, reg, val);
}

static void
mue_setmtu_locked(struct usbnet *un)
{
	struct ifnet * const ifp = usbnet_ifp(un);
	uint32_t val;

	/* Set the maximum frame size. */
	MUE_CLRBIT(un, MUE_MAC_RX, MUE_MAC_RX_RXEN);
	val = mue_csr_read(un, MUE_MAC_RX);
	val &= ~MUE_MAC_RX_MAX_SIZE_MASK;
	val |= MUE_MAC_RX_MAX_LEN(MUE_FRAME_LEN(ifp->if_mtu));
	mue_csr_write(un, MUE_MAC_RX, val);
	MUE_SETBIT(un, MUE_MAC_RX, MUE_MAC_RX_RXEN);
}

static void
mue_uno_rx_loop(struct usbnet *un, struct usbnet_chain *c, uint32_t total_len)
{
	struct ifnet * const ifp = usbnet_ifp(un);
	struct mue_rxbuf_hdr *hdrp;
	uint32_t rx_cmd_a;
	uint16_t pktlen;
	int csum;
	uint8_t *buf = c->unc_buf;
	bool v6;

	KASSERTMSG(total_len <= un->un_rx_bufsz, "%u vs %u",
	    total_len, un->un_rx_bufsz);

	do {
		if (__predict_false(total_len < sizeof(*hdrp))) {
			MUE_PRINTF(un, "packet length %u too short\n", total_len);
			if_statinc(ifp, if_ierrors);
			return;
		}

		hdrp = (struct mue_rxbuf_hdr *)buf;
		rx_cmd_a = le32toh(hdrp->rx_cmd_a);

		if (__predict_false(rx_cmd_a & MUE_RX_CMD_A_ERRORS)) {
			/*
			 * We cannot use MUE_RX_CMD_A_RED bit here;
			 * it is turned on in the cases of L3/L4
			 * checksum errors which we handle below.
			 */
			MUE_PRINTF(un, "rx_cmd_a: %#x\n", rx_cmd_a);
			if_statinc(ifp, if_ierrors);
			return;
		}

		pktlen = (uint16_t)(rx_cmd_a & MUE_RX_CMD_A_LEN_MASK);
		if (un->un_flags & LAN7500)
			pktlen -= 2;

		if (__predict_false(pktlen < ETHER_HDR_LEN + ETHER_CRC_LEN ||
		    pktlen > MCLBYTES - ETHER_ALIGN || /* XXX */
		    pktlen + sizeof(*hdrp) > total_len)) {
			MUE_PRINTF(un, "invalid packet length %d\n", pktlen);
			if_statinc(ifp, if_ierrors);
			return;
		}

		if (__predict_false(rx_cmd_a & MUE_RX_CMD_A_ICSM)) {
			csum = 0;
		} else {
			v6 = rx_cmd_a & MUE_RX_CMD_A_IPV;
			switch (rx_cmd_a & MUE_RX_CMD_A_PID) {
			case MUE_RX_CMD_A_PID_TCP:
				csum = v6 ?
				    M_CSUM_TCPv6 : M_CSUM_IPv4 | M_CSUM_TCPv4;
				break;
			case MUE_RX_CMD_A_PID_UDP:
				csum = v6 ?
				    M_CSUM_UDPv6 : M_CSUM_IPv4 | M_CSUM_UDPv4;
				break;
			case MUE_RX_CMD_A_PID_IP:
				csum = v6 ? 0 : M_CSUM_IPv4;
				break;
			default:
				csum = 0;
				break;
			}
			csum &= ifp->if_csum_flags_rx;
			if (__predict_false((csum & M_CSUM_IPv4) &&
			    (rx_cmd_a & MUE_RX_CMD_A_ICE)))
				csum |= M_CSUM_IPv4_BAD;
			if (__predict_false((csum & ~M_CSUM_IPv4) &&
			    (rx_cmd_a & MUE_RX_CMD_A_TCE)))
				csum |= M_CSUM_TCP_UDP_BAD;
		}

		usbnet_enqueue(un, buf + sizeof(*hdrp), pktlen, csum,
			       0, M_HASFCS);

		/* Attention: sizeof(hdr) = 10 */
		pktlen = roundup(pktlen + sizeof(*hdrp), 4);
		if (pktlen > total_len)
			pktlen = total_len;
		total_len -= pktlen;
		buf += pktlen;
	} while (total_len > 0);
}

static int
mue_init_locked(struct ifnet *ifp)
{
	struct usbnet * const un = ifp->if_softc;

	if (usbnet_isdying(un)) {
		DPRINTF(un, "dying\n");
		return EIO;
	}

	/* Cancel pending I/O and free all TX/RX buffers. */
	if (ifp->if_flags & IFF_RUNNING)
		usbnet_stop(un, ifp, 1);

	mue_reset(un);

	/* Set MAC address. */
	mue_set_macaddr(un);

	/* Load the multicast filter. */
	mue_setiff_locked(un);

	/* TCP/UDP checksum offload engines. */
	mue_sethwcsum_locked(un);

	/* Set MTU. */
	mue_setmtu_locked(un);

	return usbnet_init_rx_tx(un);
}

static int
mue_uno_init(struct ifnet *ifp)
{
	struct usbnet * const	un = ifp->if_softc;
	int rv;

	usbnet_lock_core(un);
	usbnet_busy(un);
	rv = mue_init_locked(ifp);
	usbnet_unbusy(un);
	usbnet_unlock_core(un);

	return rv;
}

static int
mue_uno_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	struct usbnet * const un = ifp->if_softc;

	usbnet_lock_core(un);
	usbnet_busy(un);

	switch (cmd) {
	case SIOCADDMULTI:
	case SIOCDELMULTI:
		mue_setiff_locked(un);
		break;
	case SIOCSIFCAP:
		mue_sethwcsum_locked(un);
		break;
	case SIOCSIFMTU:
		mue_setmtu_locked(un);
		break;
	default:
		break;
	}

	usbnet_unbusy(un);
	usbnet_unlock_core(un);

	return 0;
}

static void
mue_reset(struct usbnet *un)
{
	if (usbnet_isdying(un))
		return;

	/* Wait a little while for the chip to get its brains in order. */
	usbd_delay_ms(un->un_udev, 1);

//	mue_chip_init(un); /* XXX */
}

static void
mue_uno_stop(struct ifnet *ifp, int disable)
{
	struct usbnet * const un = ifp->if_softc;

	mue_reset(un);
}

#ifdef _MODULE
#include "ioconf.c"
#endif

USBNET_MODULE(mue)
