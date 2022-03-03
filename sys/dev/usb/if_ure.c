/*	$NetBSD: if_ure.c,v 1.45 2022/03/03 05:51:27 riastradh Exp $	*/
/*	$OpenBSD: if_ure.c,v 1.10 2018/11/02 21:32:30 jcs Exp $	*/

/*-
 * Copyright (c) 2015-2016 Kevin Lo <kevlo@FreeBSD.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
 */

/* RealTek RTL8152/RTL8153 10/100/Gigabit USB Ethernet device */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_ure.c,v 1.45 2022/03/03 05:51:27 riastradh Exp $");

#ifdef _KERNEL_OPT
#include "opt_usb.h"
#include "opt_inet.h"
#endif

#include <sys/param.h>
#include <sys/cprng.h>

#include <net/route.h>

#include <dev/usb/usbnet.h>

#include <netinet/in_offload.h>		/* XXX for in_undefer_cksum() */
#ifdef INET6
#include <netinet/in.h>
#include <netinet6/in6_offload.h>	/* XXX for in6_undefer_cksum() */
#endif

#include <dev/ic/rtl81x9reg.h>		/* XXX for RTK_GMEDIASTAT */
#include <dev/usb/if_urereg.h>
#include <dev/usb/if_urevar.h>

#define URE_PRINTF(un, fmt, args...) \
	device_printf((un)->un_dev, "%s: " fmt, __func__, ##args);

#define URE_DEBUG
#ifdef URE_DEBUG
#define DPRINTF(x)	do { if (uredebug) printf x; } while (0)
#define DPRINTFN(n, x)	do { if (uredebug >= (n)) printf x; } while (0)
int	uredebug = 0;
#else
#define DPRINTF(x)
#define DPRINTFN(n, x)
#endif

#define ETHER_IS_ZERO(addr) \
	(!(addr[0] | addr[1] | addr[2] | addr[3] | addr[4] | addr[5]))

static const struct usb_devno ure_devs[] = {
	{ USB_VENDOR_REALTEK, USB_PRODUCT_REALTEK_RTL8152 },
	{ USB_VENDOR_REALTEK, USB_PRODUCT_REALTEK_RTL8153 }
};

#define URE_BUFSZ	(16 * 1024)

static void	ure_reset(struct usbnet *);
static uint32_t	ure_txcsum(struct mbuf *);
static int	ure_rxcsum(struct ifnet *, struct ure_rxpkt *);
static void	ure_rtl8152_init(struct usbnet *);
static void	ure_rtl8153_init(struct usbnet *);
static void	ure_disable_teredo(struct usbnet *);
static void	ure_init_fifo(struct usbnet *);

static void	ure_uno_stop(struct ifnet *, int);
static void	ure_uno_mcast(struct ifnet *);
static int	ure_uno_mii_read_reg(struct usbnet *, int, int, uint16_t *);
static int	ure_uno_mii_write_reg(struct usbnet *, int, int, uint16_t);
static void	ure_uno_miibus_statchg(struct ifnet *);
static unsigned ure_uno_tx_prepare(struct usbnet *, struct mbuf *,
				   struct usbnet_chain *);
static void	ure_uno_rx_loop(struct usbnet *, struct usbnet_chain *,
				uint32_t);
static int	ure_uno_init(struct ifnet *);

static int	ure_match(device_t, cfdata_t, void *);
static void	ure_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(ure, sizeof(struct usbnet), ure_match, ure_attach,
    usbnet_detach, usbnet_activate);

static const struct usbnet_ops ure_ops = {
	.uno_stop = ure_uno_stop,
	.uno_mcast = ure_uno_mcast,
	.uno_read_reg = ure_uno_mii_read_reg,
	.uno_write_reg = ure_uno_mii_write_reg,
	.uno_statchg = ure_uno_miibus_statchg,
	.uno_tx_prepare = ure_uno_tx_prepare,
	.uno_rx_loop = ure_uno_rx_loop,
	.uno_init = ure_uno_init,
};

static int
ure_ctl(struct usbnet *un, uint8_t rw, uint16_t val, uint16_t index,
    void *buf, int len)
{
	usb_device_request_t req;
	usbd_status err;

	if (usbnet_isdying(un))
		return 0;

	if (rw == URE_CTL_WRITE)
		req.bmRequestType = UT_WRITE_VENDOR_DEVICE;
	else
		req.bmRequestType = UT_READ_VENDOR_DEVICE;
	req.bRequest = UR_SET_ADDRESS;
	USETW(req.wValue, val);
	USETW(req.wIndex, index);
	USETW(req.wLength, len);

	DPRINTFN(5, ("ure_ctl: rw %d, val %04hu, index %04hu, len %d\n",
	    rw, val, index, len));
	err = usbd_do_request(un->un_udev, &req, buf);
	if (err) {
		DPRINTF(("ure_ctl: error %d\n", err));
		return -1;
	}

	return 0;
}

static int
ure_read_mem(struct usbnet *un, uint16_t addr, uint16_t index,
    void *buf, int len)
{
	return ure_ctl(un, URE_CTL_READ, addr, index, buf, len);
}

static int
ure_write_mem(struct usbnet *un, uint16_t addr, uint16_t index,
    void *buf, int len)
{
	return ure_ctl(un, URE_CTL_WRITE, addr, index, buf, len);
}

static uint8_t
ure_read_1(struct usbnet *un, uint16_t reg, uint16_t index)
{
	uint32_t val;
	uint8_t temp[4];
	uint8_t shift;

	shift = (reg & 3) << 3;
	reg &= ~3;

	ure_read_mem(un, reg, index, &temp, 4);
	val = UGETDW(temp);
	val >>= shift;

	return val & 0xff;
}

static uint16_t
ure_read_2(struct usbnet *un, uint16_t reg, uint16_t index)
{
	uint32_t val;
	uint8_t temp[4];
	uint8_t shift;

	shift = (reg & 2) << 3;
	reg &= ~3;

	ure_read_mem(un, reg, index, &temp, 4);
	val = UGETDW(temp);
	val >>= shift;

	return val & 0xffff;
}

static uint32_t
ure_read_4(struct usbnet *un, uint16_t reg, uint16_t index)
{
	uint8_t temp[4];

	ure_read_mem(un, reg, index, &temp, 4);
	return UGETDW(temp);
}

static int
ure_write_1(struct usbnet *un, uint16_t reg, uint16_t index, uint32_t val)
{
	uint16_t byen;
	uint8_t temp[4];
	uint8_t shift;

	byen = URE_BYTE_EN_BYTE;
	shift = reg & 3;
	val &= 0xff;

	if (reg & 3) {
		byen <<= shift;
		val <<= (shift << 3);
		reg &= ~3;
	}

	USETDW(temp, val);
	return ure_write_mem(un, reg, index | byen, &temp, 4);
}

static int
ure_write_2(struct usbnet *un, uint16_t reg, uint16_t index, uint32_t val)
{
	uint16_t byen;
	uint8_t temp[4];
	uint8_t shift;

	byen = URE_BYTE_EN_WORD;
	shift = reg & 2;
	val &= 0xffff;

	if (reg & 2) {
		byen <<= shift;
		val <<= (shift << 3);
		reg &= ~3;
	}

	USETDW(temp, val);
	return ure_write_mem(un, reg, index | byen, &temp, 4);
}

static int
ure_write_4(struct usbnet *un, uint16_t reg, uint16_t index, uint32_t val)
{
	uint8_t temp[4];

	USETDW(temp, val);
	return ure_write_mem(un, reg, index | URE_BYTE_EN_DWORD, &temp, 4);
}

static uint16_t
ure_ocp_reg_read(struct usbnet *un, uint16_t addr)
{
	uint16_t reg;

	ure_write_2(un, URE_PLA_OCP_GPHY_BASE, URE_MCU_TYPE_PLA, addr & 0xf000);
	reg = (addr & 0x0fff) | 0xb000;

	return ure_read_2(un, reg, URE_MCU_TYPE_PLA);
}

static void
ure_ocp_reg_write(struct usbnet *un, uint16_t addr, uint16_t data)
{
	uint16_t reg;

	ure_write_2(un, URE_PLA_OCP_GPHY_BASE, URE_MCU_TYPE_PLA, addr & 0xf000);
	reg = (addr & 0x0fff) | 0xb000;

	ure_write_2(un, reg, URE_MCU_TYPE_PLA, data);
}

static int
ure_uno_mii_read_reg(struct usbnet *un, int phy, int reg, uint16_t *val)
{

	if (un->un_phyno != phy)
		return EINVAL;

	/* Let the rgephy driver read the URE_PLA_PHYSTATUS register. */
	if (reg == RTK_GMEDIASTAT) {
		*val = ure_read_1(un, URE_PLA_PHYSTATUS, URE_MCU_TYPE_PLA);
		return USBD_NORMAL_COMPLETION;
	}

	*val = ure_ocp_reg_read(un, URE_OCP_BASE_MII + reg * 2);

	return 0;
}

static int
ure_uno_mii_write_reg(struct usbnet *un, int phy, int reg, uint16_t val)
{

	if (un->un_phyno != phy)
		return EINVAL;

	ure_ocp_reg_write(un, URE_OCP_BASE_MII + reg * 2, val);

	return 0;
}

static void
ure_uno_miibus_statchg(struct ifnet *ifp)
{
	struct usbnet * const un = ifp->if_softc;
	struct mii_data * const mii = usbnet_mii(un);

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
			if ((un->un_flags & URE_FLAG_8152) != 0)
				break;
			usbnet_set_link(un, true);
			break;
		default:
			break;
		}
	}
}

static void
ure_rcvfilt_locked(struct usbnet *un)
{
	struct ethercom *ec = usbnet_ec(un);
	struct ifnet *ifp = usbnet_ifp(un);
	struct ether_multi *enm;
	struct ether_multistep step;
	uint32_t mchash[2] = { 0, 0 };
	uint32_t h = 0, rxmode;

	usbnet_isowned_core(un);

	if (usbnet_isdying(un))
		return;

	rxmode = ure_read_4(un, URE_PLA_RCR, URE_MCU_TYPE_PLA);
	rxmode &= ~(URE_RCR_AAP | URE_RCR_AM);
	/* continue to accept my own DA and bcast frames */

	ETHER_LOCK(ec);
	if (ifp->if_flags & IFF_PROMISC) {
		ec->ec_flags |= ETHER_F_ALLMULTI;
		ETHER_UNLOCK(ec);
		/* run promisc. mode */
		rxmode |= URE_RCR_AM;	/* ??? */
		rxmode |= URE_RCR_AAP;
		goto update;
	}
	ec->ec_flags &= ~ETHER_F_ALLMULTI;
	ETHER_FIRST_MULTI(step, ec, enm);
	while (enm != NULL) {
		if (memcmp(enm->enm_addrlo, enm->enm_addrhi, ETHER_ADDR_LEN)) {
			ec->ec_flags |= ETHER_F_ALLMULTI;
			ETHER_UNLOCK(ec);
			/* accept all mcast frames */
			rxmode |= URE_RCR_AM;
			mchash[0] = mchash[1] = ~0U; /* necessary ?? */
			goto update;
		}
		h = ether_crc32_be(enm->enm_addrlo, ETHER_ADDR_LEN);
		mchash[h >> 31] |= 1 << ((h >> 26) & 0x1f);
		ETHER_NEXT_MULTI(step, enm);
	}
	ETHER_UNLOCK(ec);
	if (h != 0) {
		rxmode |= URE_RCR_AM;	/* activate mcast hash filter */
		h = bswap32(mchash[0]);
		mchash[0] = bswap32(mchash[1]);
		mchash[1] = h;
	}
 update:
	ure_write_4(un, URE_PLA_MAR0, URE_MCU_TYPE_PLA, mchash[0]);
	ure_write_4(un, URE_PLA_MAR4, URE_MCU_TYPE_PLA, mchash[1]);
	ure_write_4(un, URE_PLA_RCR, URE_MCU_TYPE_PLA, rxmode);
}

static void
ure_reset(struct usbnet *un)
{
	int i;

	usbnet_isowned_core(un);

	ure_write_1(un, URE_PLA_CR, URE_MCU_TYPE_PLA, URE_CR_RST);

	for (i = 0; i < URE_TIMEOUT; i++) {
		if (usbnet_isdying(un))
			return;
		if (!(ure_read_1(un, URE_PLA_CR, URE_MCU_TYPE_PLA) &
		    URE_CR_RST))
			break;
		usbd_delay_ms(un->un_udev, 10);
	}
	if (i == URE_TIMEOUT)
		URE_PRINTF(un, "reset never completed\n");
}

static int
ure_init_locked(struct ifnet *ifp)
{
	struct usbnet * const un = ifp->if_softc;
	uint8_t eaddr[8];

	usbnet_isowned_core(un);

	if (usbnet_isdying(un))
		return EIO;

	/* Cancel pending I/O. */
	if (ifp->if_flags & IFF_RUNNING)
		usbnet_stop(un, ifp, 1);

	/* Set MAC address. */
	memset(eaddr, 0, sizeof(eaddr));
	memcpy(eaddr, CLLADDR(ifp->if_sadl), ETHER_ADDR_LEN);
	ure_write_1(un, URE_PLA_CRWECR, URE_MCU_TYPE_PLA, URE_CRWECR_CONFIG);
	ure_write_mem(un, URE_PLA_IDR, URE_MCU_TYPE_PLA | URE_BYTE_EN_SIX_BYTES,
	    eaddr, 8);
	ure_write_1(un, URE_PLA_CRWECR, URE_MCU_TYPE_PLA, URE_CRWECR_NORAML);

	/* Reset the packet filter. */
	ure_write_2(un, URE_PLA_FMC, URE_MCU_TYPE_PLA,
	    ure_read_2(un, URE_PLA_FMC, URE_MCU_TYPE_PLA) &
	    ~URE_FMC_FCR_MCU_EN);
	ure_write_2(un, URE_PLA_FMC, URE_MCU_TYPE_PLA,
	    ure_read_2(un, URE_PLA_FMC, URE_MCU_TYPE_PLA) |
	    URE_FMC_FCR_MCU_EN);

	/* Enable transmit and receive. */
	ure_write_1(un, URE_PLA_CR, URE_MCU_TYPE_PLA,
	    ure_read_1(un, URE_PLA_CR, URE_MCU_TYPE_PLA) | URE_CR_RE |
	    URE_CR_TE);

	ure_write_2(un, URE_PLA_MISC_1, URE_MCU_TYPE_PLA,
	    ure_read_2(un, URE_PLA_MISC_1, URE_MCU_TYPE_PLA) &
	    ~URE_RXDY_GATED_EN);

	/* Accept multicast frame or run promisc. mode. */
	ure_rcvfilt_locked(un);

	return usbnet_init_rx_tx(un);
}

static int
ure_uno_init(struct ifnet *ifp)
{
	int ret = ure_init_locked(ifp);

	return ret;
}

static void
ure_uno_stop(struct ifnet *ifp, int disable __unused)
{
	struct usbnet * const un = ifp->if_softc;

	ure_reset(un);
}

static void
ure_rtl8152_init(struct usbnet *un)
{
	uint32_t pwrctrl;

	/* Disable ALDPS. */
	ure_ocp_reg_write(un, URE_OCP_ALDPS_CONFIG, URE_ENPDNPS | URE_LINKENA |
	    URE_DIS_SDSAVE);
	usbd_delay_ms(un->un_udev, 20);

	if (un->un_flags & URE_FLAG_VER_4C00) {
		ure_write_2(un, URE_PLA_LED_FEATURE, URE_MCU_TYPE_PLA,
		    ure_read_2(un, URE_PLA_LED_FEATURE, URE_MCU_TYPE_PLA) &
		    ~URE_LED_MODE_MASK);
	}

	ure_write_2(un, URE_USB_UPS_CTRL, URE_MCU_TYPE_USB,
	    ure_read_2(un, URE_USB_UPS_CTRL, URE_MCU_TYPE_USB) &
	    ~URE_POWER_CUT);
	ure_write_2(un, URE_USB_PM_CTRL_STATUS, URE_MCU_TYPE_USB,
	    ure_read_2(un, URE_USB_PM_CTRL_STATUS, URE_MCU_TYPE_USB) &
	    ~URE_RESUME_INDICATE);

	ure_write_2(un, URE_PLA_PHY_PWR, URE_MCU_TYPE_PLA,
	    ure_read_2(un, URE_PLA_PHY_PWR, URE_MCU_TYPE_PLA) |
	    URE_TX_10M_IDLE_EN | URE_PFM_PWM_SWITCH);
	pwrctrl = ure_read_4(un, URE_PLA_MAC_PWR_CTRL, URE_MCU_TYPE_PLA);
	pwrctrl &= ~URE_MCU_CLK_RATIO_MASK;
	pwrctrl |= URE_MCU_CLK_RATIO | URE_D3_CLK_GATED_EN;
	ure_write_4(un, URE_PLA_MAC_PWR_CTRL, URE_MCU_TYPE_PLA, pwrctrl);
	ure_write_2(un, URE_PLA_GPHY_INTR_IMR, URE_MCU_TYPE_PLA,
	    URE_GPHY_STS_MSK | URE_SPEED_DOWN_MSK | URE_SPDWN_RXDV_MSK |
	    URE_SPDWN_LINKCHG_MSK);

	/* Enable Rx aggregation. */
	ure_write_2(un, URE_USB_USB_CTRL, URE_MCU_TYPE_USB,
	    ure_read_2(un, URE_USB_USB_CTRL, URE_MCU_TYPE_USB) &
	    ~URE_RX_AGG_DISABLE);

	/* Disable ALDPS. */
	ure_ocp_reg_write(un, URE_OCP_ALDPS_CONFIG, URE_ENPDNPS | URE_LINKENA |
	    URE_DIS_SDSAVE);
	usbd_delay_ms(un->un_udev, 20);

	ure_init_fifo(un);

	ure_write_1(un, URE_USB_TX_AGG, URE_MCU_TYPE_USB,
	    URE_TX_AGG_MAX_THRESHOLD);
	ure_write_4(un, URE_USB_RX_BUF_TH, URE_MCU_TYPE_USB, URE_RX_THR_HIGH);
	ure_write_4(un, URE_USB_TX_DMA, URE_MCU_TYPE_USB,
	    URE_TEST_MODE_DISABLE | URE_TX_SIZE_ADJUST1);
}

static void
ure_rtl8153_init(struct usbnet *un)
{
	uint16_t val;
	uint8_t u1u2[8];
	int i;

	/* Disable ALDPS. */
	ure_ocp_reg_write(un, URE_OCP_POWER_CFG,
	    ure_ocp_reg_read(un, URE_OCP_POWER_CFG) & ~URE_EN_ALDPS);
	usbd_delay_ms(un->un_udev, 20);

	memset(u1u2, 0x00, sizeof(u1u2));
	ure_write_mem(un, URE_USB_TOLERANCE,
	    URE_MCU_TYPE_USB | URE_BYTE_EN_SIX_BYTES, u1u2, sizeof(u1u2));

	for (i = 0; i < URE_TIMEOUT; i++) {
		if (usbnet_isdying(un))
			return;
		if (ure_read_2(un, URE_PLA_BOOT_CTRL, URE_MCU_TYPE_PLA) &
		    URE_AUTOLOAD_DONE)
			break;
		usbd_delay_ms(un->un_udev, 10);
	}
	if (i == URE_TIMEOUT)
		URE_PRINTF(un, "timeout waiting for chip autoload\n");

	for (i = 0; i < URE_TIMEOUT; i++) {
		if (usbnet_isdying(un))
			return;
		val = ure_ocp_reg_read(un, URE_OCP_PHY_STATUS) &
		    URE_PHY_STAT_MASK;
		if (val == URE_PHY_STAT_LAN_ON || val == URE_PHY_STAT_PWRDN)
			break;
		usbd_delay_ms(un->un_udev, 10);
	}
	if (i == URE_TIMEOUT)
		URE_PRINTF(un, "timeout waiting for phy to stabilize\n");

	ure_write_2(un, URE_USB_U2P3_CTRL, URE_MCU_TYPE_USB,
	    ure_read_2(un, URE_USB_U2P3_CTRL, URE_MCU_TYPE_USB) &
	    ~URE_U2P3_ENABLE);

	if (un->un_flags & URE_FLAG_VER_5C10) {
		val = ure_read_2(un, URE_USB_SSPHYLINK2, URE_MCU_TYPE_USB);
		val &= ~URE_PWD_DN_SCALE_MASK;
		val |= URE_PWD_DN_SCALE(96);
		ure_write_2(un, URE_USB_SSPHYLINK2, URE_MCU_TYPE_USB, val);

		ure_write_1(un, URE_USB_USB2PHY, URE_MCU_TYPE_USB,
		    ure_read_1(un, URE_USB_USB2PHY, URE_MCU_TYPE_USB) |
		    URE_USB2PHY_L1 | URE_USB2PHY_SUSPEND);
	} else if (un->un_flags & URE_FLAG_VER_5C20) {
		ure_write_1(un, URE_PLA_DMY_REG0, URE_MCU_TYPE_PLA,
		    ure_read_1(un, URE_PLA_DMY_REG0, URE_MCU_TYPE_PLA) &
		    ~URE_ECM_ALDPS);
	}
	if (un->un_flags & (URE_FLAG_VER_5C20 | URE_FLAG_VER_5C30)) {
		val = ure_read_1(un, URE_USB_CSR_DUMMY1, URE_MCU_TYPE_USB);
		if (ure_read_2(un, URE_USB_BURST_SIZE, URE_MCU_TYPE_USB) ==
		    0)
			val &= ~URE_DYNAMIC_BURST;
		else
			val |= URE_DYNAMIC_BURST;
		ure_write_1(un, URE_USB_CSR_DUMMY1, URE_MCU_TYPE_USB, val);
	}

	ure_write_1(un, URE_USB_CSR_DUMMY2, URE_MCU_TYPE_USB,
	    ure_read_1(un, URE_USB_CSR_DUMMY2, URE_MCU_TYPE_USB) |
	    URE_EP4_FULL_FC);

	ure_write_2(un, URE_USB_WDT11_CTRL, URE_MCU_TYPE_USB,
	    ure_read_2(un, URE_USB_WDT11_CTRL, URE_MCU_TYPE_USB) &
	    ~URE_TIMER11_EN);

	ure_write_2(un, URE_PLA_LED_FEATURE, URE_MCU_TYPE_PLA,
	    ure_read_2(un, URE_PLA_LED_FEATURE, URE_MCU_TYPE_PLA) &
	    ~URE_LED_MODE_MASK);

	if ((un->un_flags & URE_FLAG_VER_5C10) &&
	    un->un_udev->ud_speed != USB_SPEED_SUPER)
		val = URE_LPM_TIMER_500MS;
	else
		val = URE_LPM_TIMER_500US;
	ure_write_1(un, URE_USB_LPM_CTRL, URE_MCU_TYPE_USB,
	    val | URE_FIFO_EMPTY_1FB | URE_ROK_EXIT_LPM);

	val = ure_read_2(un, URE_USB_AFE_CTRL2, URE_MCU_TYPE_USB);
	val &= ~URE_SEN_VAL_MASK;
	val |= URE_SEN_VAL_NORMAL | URE_SEL_RXIDLE;
	ure_write_2(un, URE_USB_AFE_CTRL2, URE_MCU_TYPE_USB, val);

	ure_write_2(un, URE_USB_CONNECT_TIMER, URE_MCU_TYPE_USB, 0x0001);

	ure_write_2(un, URE_USB_POWER_CUT, URE_MCU_TYPE_USB,
	    ure_read_2(un, URE_USB_POWER_CUT, URE_MCU_TYPE_USB) &
	    ~(URE_PWR_EN | URE_PHASE2_EN));
	ure_write_2(un, URE_USB_MISC_0, URE_MCU_TYPE_USB,
	    ure_read_2(un, URE_USB_MISC_0, URE_MCU_TYPE_USB) &
	    ~URE_PCUT_STATUS);

	memset(u1u2, 0xff, sizeof(u1u2));
	ure_write_mem(un, URE_USB_TOLERANCE,
	    URE_MCU_TYPE_USB | URE_BYTE_EN_SIX_BYTES, u1u2, sizeof(u1u2));

	ure_write_2(un, URE_PLA_MAC_PWR_CTRL, URE_MCU_TYPE_PLA,
	    URE_ALDPS_SPDWN_RATIO);
	ure_write_2(un, URE_PLA_MAC_PWR_CTRL2, URE_MCU_TYPE_PLA,
	    URE_EEE_SPDWN_RATIO);
	ure_write_2(un, URE_PLA_MAC_PWR_CTRL3, URE_MCU_TYPE_PLA,
	    URE_PKT_AVAIL_SPDWN_EN | URE_SUSPEND_SPDWN_EN |
	    URE_U1U2_SPDWN_EN | URE_L1_SPDWN_EN);
	ure_write_2(un, URE_PLA_MAC_PWR_CTRL4, URE_MCU_TYPE_PLA,
	    URE_PWRSAVE_SPDWN_EN | URE_RXDV_SPDWN_EN | URE_TX10MIDLE_EN |
	    URE_TP100_SPDWN_EN | URE_TP500_SPDWN_EN | URE_TP1000_SPDWN_EN |
	    URE_EEE_SPDWN_EN);

	val = ure_read_2(un, URE_USB_U2P3_CTRL, URE_MCU_TYPE_USB);
	if (!(un->un_flags & (URE_FLAG_VER_5C00 | URE_FLAG_VER_5C10)))
		val |= URE_U2P3_ENABLE;
	else
		val &= ~URE_U2P3_ENABLE;
	ure_write_2(un, URE_USB_U2P3_CTRL, URE_MCU_TYPE_USB, val);

	memset(u1u2, 0x00, sizeof(u1u2));
	ure_write_mem(un, URE_USB_TOLERANCE,
	    URE_MCU_TYPE_USB | URE_BYTE_EN_SIX_BYTES, u1u2, sizeof(u1u2));

	/* Disable ALDPS. */
	ure_ocp_reg_write(un, URE_OCP_POWER_CFG,
	    ure_ocp_reg_read(un, URE_OCP_POWER_CFG) & ~URE_EN_ALDPS);
	usbd_delay_ms(un->un_udev, 20);

	ure_init_fifo(un);

	/* Enable Rx aggregation. */
	ure_write_2(un, URE_USB_USB_CTRL, URE_MCU_TYPE_USB,
	    ure_read_2(un, URE_USB_USB_CTRL, URE_MCU_TYPE_USB) &
	    ~URE_RX_AGG_DISABLE);

	val = ure_read_2(un, URE_USB_U2P3_CTRL, URE_MCU_TYPE_USB);
	if (!(un->un_flags & (URE_FLAG_VER_5C00 | URE_FLAG_VER_5C10)))
		val |= URE_U2P3_ENABLE;
	else
		val &= ~URE_U2P3_ENABLE;
	ure_write_2(un, URE_USB_U2P3_CTRL, URE_MCU_TYPE_USB, val);

	memset(u1u2, 0xff, sizeof(u1u2));
	ure_write_mem(un, URE_USB_TOLERANCE,
	    URE_MCU_TYPE_USB | URE_BYTE_EN_SIX_BYTES, u1u2, sizeof(u1u2));
}

static void
ure_disable_teredo(struct usbnet *un)
{
	ure_write_4(un, URE_PLA_TEREDO_CFG, URE_MCU_TYPE_PLA,
	    ure_read_4(un, URE_PLA_TEREDO_CFG, URE_MCU_TYPE_PLA) &
	    ~(URE_TEREDO_SEL | URE_TEREDO_RS_EVENT_MASK | URE_OOB_TEREDO_EN));
	ure_write_2(un, URE_PLA_WDT6_CTRL, URE_MCU_TYPE_PLA,
	    URE_WDT6_SET_MODE);
	ure_write_2(un, URE_PLA_REALWOW_TIMER, URE_MCU_TYPE_PLA, 0);
	ure_write_4(un, URE_PLA_TEREDO_TIMER, URE_MCU_TYPE_PLA, 0);
}

static void
ure_init_fifo(struct usbnet *un)
{
	uint32_t rxmode, rx_fifo1, rx_fifo2;
	int i;

	ure_write_2(un, URE_PLA_MISC_1, URE_MCU_TYPE_PLA,
	    ure_read_2(un, URE_PLA_MISC_1, URE_MCU_TYPE_PLA) |
	    URE_RXDY_GATED_EN);

	ure_disable_teredo(un);

	rxmode = ure_read_4(un, URE_PLA_RCR, URE_MCU_TYPE_PLA);
	rxmode &= ~URE_RCR_ACPT_ALL;
	rxmode |= URE_RCR_APM | URE_RCR_AB; /* accept my own DA and bcast */
	ure_write_4(un, URE_PLA_RCR, URE_MCU_TYPE_PLA, rxmode);

	if (!(un->un_flags & URE_FLAG_8152)) {
		if (un->un_flags & (URE_FLAG_VER_5C00 | URE_FLAG_VER_5C10 |
		    URE_FLAG_VER_5C20))
			ure_ocp_reg_write(un, URE_OCP_ADC_CFG,
			    URE_CKADSEL_L | URE_ADC_EN | URE_EN_EMI_L);
		if (un->un_flags & URE_FLAG_VER_5C00)
			ure_ocp_reg_write(un, URE_OCP_EEE_CFG,
			    ure_ocp_reg_read(un, URE_OCP_EEE_CFG) &
			    ~URE_CTAP_SHORT_EN);
		ure_ocp_reg_write(un, URE_OCP_POWER_CFG,
		    ure_ocp_reg_read(un, URE_OCP_POWER_CFG) |
		    URE_EEE_CLKDIV_EN);
		ure_ocp_reg_write(un, URE_OCP_DOWN_SPEED,
		    ure_ocp_reg_read(un, URE_OCP_DOWN_SPEED) |
		    URE_EN_10M_BGOFF);
		ure_ocp_reg_write(un, URE_OCP_POWER_CFG,
		    ure_ocp_reg_read(un, URE_OCP_POWER_CFG) |
		    URE_EN_10M_PLLOFF);
		ure_ocp_reg_write(un, URE_OCP_SRAM_ADDR, URE_SRAM_IMPEDANCE);
		ure_ocp_reg_write(un, URE_OCP_SRAM_DATA, 0x0b13);
		ure_write_2(un, URE_PLA_PHY_PWR, URE_MCU_TYPE_PLA,
		    ure_read_2(un, URE_PLA_PHY_PWR, URE_MCU_TYPE_PLA) |
		    URE_PFM_PWM_SWITCH);

		/* Enable LPF corner auto tune. */
		ure_ocp_reg_write(un, URE_OCP_SRAM_ADDR, URE_SRAM_LPF_CFG);
		ure_ocp_reg_write(un, URE_OCP_SRAM_DATA, 0xf70f);

		/* Adjust 10M amplitude. */
		ure_ocp_reg_write(un, URE_OCP_SRAM_ADDR, URE_SRAM_10M_AMP1);
		ure_ocp_reg_write(un, URE_OCP_SRAM_DATA, 0x00af);
		ure_ocp_reg_write(un, URE_OCP_SRAM_ADDR, URE_SRAM_10M_AMP2);
		ure_ocp_reg_write(un, URE_OCP_SRAM_DATA, 0x0208);
	}

	ure_reset(un);

	ure_write_1(un, URE_PLA_CR, URE_MCU_TYPE_PLA, 0);

	ure_write_1(un, URE_PLA_OOB_CTRL, URE_MCU_TYPE_PLA,
	    ure_read_1(un, URE_PLA_OOB_CTRL, URE_MCU_TYPE_PLA) &
	    ~URE_NOW_IS_OOB);

	ure_write_2(un, URE_PLA_SFF_STS_7, URE_MCU_TYPE_PLA,
	    ure_read_2(un, URE_PLA_SFF_STS_7, URE_MCU_TYPE_PLA) &
	    ~URE_MCU_BORW_EN);
	for (i = 0; i < URE_TIMEOUT; i++) {
		if (usbnet_isdying(un))
			return;
		if (ure_read_1(un, URE_PLA_OOB_CTRL, URE_MCU_TYPE_PLA) &
		    URE_LINK_LIST_READY)
			break;
		usbd_delay_ms(un->un_udev, 10);
	}
	if (i == URE_TIMEOUT)
		URE_PRINTF(un, "timeout waiting for OOB control\n");
	ure_write_2(un, URE_PLA_SFF_STS_7, URE_MCU_TYPE_PLA,
	    ure_read_2(un, URE_PLA_SFF_STS_7, URE_MCU_TYPE_PLA) |
	    URE_RE_INIT_LL);
	for (i = 0; i < URE_TIMEOUT; i++) {
		if (usbnet_isdying(un))
			return;
		if (ure_read_1(un, URE_PLA_OOB_CTRL, URE_MCU_TYPE_PLA) &
		    URE_LINK_LIST_READY)
			break;
		usbd_delay_ms(un->un_udev, 10);
	}
	if (i == URE_TIMEOUT)
		URE_PRINTF(un, "timeout waiting for OOB control\n");

	ure_write_2(un, URE_PLA_CPCR, URE_MCU_TYPE_PLA,
	    ure_read_2(un, URE_PLA_CPCR, URE_MCU_TYPE_PLA) &
	    ~URE_CPCR_RX_VLAN);
	ure_write_2(un, URE_PLA_TCR0, URE_MCU_TYPE_PLA,
	    ure_read_2(un, URE_PLA_TCR0, URE_MCU_TYPE_PLA) |
	    URE_TCR0_AUTO_FIFO);

	/* Configure Rx FIFO threshold and coalescing. */
	ure_write_4(un, URE_PLA_RXFIFO_CTRL0, URE_MCU_TYPE_PLA,
	    URE_RXFIFO_THR1_NORMAL);
	if (un->un_udev->ud_speed == USB_SPEED_FULL) {
		rx_fifo1 = URE_RXFIFO_THR2_FULL;
		rx_fifo2 = URE_RXFIFO_THR3_FULL;
	} else {
		rx_fifo1 = URE_RXFIFO_THR2_HIGH;
		rx_fifo2 = URE_RXFIFO_THR3_HIGH;
	}
	ure_write_4(un, URE_PLA_RXFIFO_CTRL1, URE_MCU_TYPE_PLA, rx_fifo1);
	ure_write_4(un, URE_PLA_RXFIFO_CTRL2, URE_MCU_TYPE_PLA, rx_fifo2);

	/* Configure Tx FIFO threshold. */
	ure_write_4(un, URE_PLA_TXFIFO_CTRL, URE_MCU_TYPE_PLA,
	    URE_TXFIFO_THR_NORMAL);
}

static void
ure_uno_mcast(struct ifnet *ifp)
{
	struct usbnet * const un = ifp->if_softc;

	usbnet_lock_core(un);

	ure_rcvfilt_locked(un);

	usbnet_unlock_core(un);
}

static int
ure_match(device_t parent, cfdata_t match, void *aux)
{
	struct usb_attach_arg *uaa = aux;

	return usb_lookup(ure_devs, uaa->uaa_vendor, uaa->uaa_product) != NULL ?
	    UMATCH_VENDOR_PRODUCT : UMATCH_NONE;
}

static void
ure_attach(device_t parent, device_t self, void *aux)
{
	USBNET_MII_DECL_DEFAULT(unm);
	struct usbnet * const un = device_private(self);
	struct usb_attach_arg *uaa = aux;
	struct usbd_device *dev = uaa->uaa_device;
	usb_interface_descriptor_t *id;
	usb_endpoint_descriptor_t *ed;
	int error, i;
	uint16_t ver;
	uint8_t eaddr[8]; /* 2byte padded */
	char *devinfop;
	uint32_t maclo, machi;

	aprint_naive("\n");
	aprint_normal("\n");
	devinfop = usbd_devinfo_alloc(dev, 0);
	aprint_normal_dev(self, "%s\n", devinfop);
	usbd_devinfo_free(devinfop);

	un->un_dev = self;
	un->un_udev = dev;
	un->un_sc = un;
	un->un_ops = &ure_ops;
	un->un_rx_xfer_flags = USBD_SHORT_XFER_OK;
	un->un_tx_xfer_flags = USBD_FORCE_SHORT_XFER;
	un->un_rx_list_cnt = URE_RX_LIST_CNT;
	un->un_tx_list_cnt = URE_TX_LIST_CNT;
	un->un_rx_bufsz = URE_BUFSZ;
	un->un_tx_bufsz = URE_BUFSZ;

#define URE_CONFIG_NO	1 /* XXX */
	error = usbd_set_config_no(dev, URE_CONFIG_NO, 1);
	if (error) {
		aprint_error_dev(self, "failed to set configuration: %s\n",
		    usbd_errstr(error));
		return; /* XXX */
	}

	if (uaa->uaa_product == USB_PRODUCT_REALTEK_RTL8152)
		un->un_flags |= URE_FLAG_8152;

#define URE_IFACE_IDX  0 /* XXX */
	error = usbd_device2interface_handle(dev, URE_IFACE_IDX, &un->un_iface);
	if (error) {
		aprint_error_dev(self, "failed to get interface handle: %s\n",
		    usbd_errstr(error));
		return; /* XXX */
	}

	id = usbd_get_interface_descriptor(un->un_iface);
	for (i = 0; i < id->bNumEndpoints; i++) {
		ed = usbd_interface2endpoint_descriptor(un->un_iface, i);
		if (ed == NULL) {
			aprint_error_dev(self, "couldn't get ep %d\n", i);
			return; /* XXX */
		}
		if (UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_IN &&
		    UE_GET_XFERTYPE(ed->bmAttributes) == UE_BULK) {
			un->un_ed[USBNET_ENDPT_RX] = ed->bEndpointAddress;
		} else if (UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_OUT &&
		    UE_GET_XFERTYPE(ed->bmAttributes) == UE_BULK) {
			un->un_ed[USBNET_ENDPT_TX] = ed->bEndpointAddress;
		}
	}

	/* Set these up now for ure_ctl().  */
	usbnet_attach(un, "uredet");

	un->un_phyno = 0;

	ver = ure_read_2(un, URE_PLA_TCR1, URE_MCU_TYPE_PLA) & URE_VERSION_MASK;
	switch (ver) {
	case 0x4c00:
		un->un_flags |= URE_FLAG_VER_4C00;
		break;
	case 0x4c10:
		un->un_flags |= URE_FLAG_VER_4C10;
		break;
	case 0x5c00:
		un->un_flags |= URE_FLAG_VER_5C00;
		break;
	case 0x5c10:
		un->un_flags |= URE_FLAG_VER_5C10;
		break;
	case 0x5c20:
		un->un_flags |= URE_FLAG_VER_5C20;
		break;
	case 0x5c30:
		un->un_flags |= URE_FLAG_VER_5C30;
		break;
	default:
		/* fake addr?  or just fail? */
		break;
	}
	aprint_normal_dev(self, "RTL%d %sver %04x\n",
	    (un->un_flags & URE_FLAG_8152) ? 8152 : 8153,
	    (un->un_flags != 0) ? "" : "unknown ",
	    ver);

	usbnet_lock_core(un);
	if (un->un_flags & URE_FLAG_8152)
		ure_rtl8152_init(un);
	else
		ure_rtl8153_init(un);

	if ((un->un_flags & URE_FLAG_VER_4C00) ||
	    (un->un_flags & URE_FLAG_VER_4C10))
		ure_read_mem(un, URE_PLA_IDR, URE_MCU_TYPE_PLA, eaddr,
		    sizeof(eaddr));
	else
		ure_read_mem(un, URE_PLA_BACKUP, URE_MCU_TYPE_PLA, eaddr,
		    sizeof(eaddr));
	usbnet_unlock_core(un);
	if (ETHER_IS_ZERO(eaddr)) {
		maclo = 0x00f2 | (cprng_strong32() & 0xffff0000);
		machi = cprng_strong32() & 0xffff;
		eaddr[0] = maclo & 0xff;
		eaddr[1] = (maclo >> 8) & 0xff;
		eaddr[2] = (maclo >> 16) & 0xff;
		eaddr[3] = (maclo >> 24) & 0xff;
		eaddr[4] = machi & 0xff;
		eaddr[5] = (machi >> 8) & 0xff;
	}
	memcpy(un->un_eaddr, eaddr, sizeof(un->un_eaddr));

	struct ifnet *ifp = usbnet_ifp(un);

	/*
	 * We don't support TSOv4 and v6 for now, that are required to
	 * be handled in software for some cases.
	 */
	ifp->if_capabilities = IFCAP_CSUM_IPv4_Tx |
	    IFCAP_CSUM_TCPv4_Tx | IFCAP_CSUM_UDPv4_Tx;
#ifdef INET6
	ifp->if_capabilities |= IFCAP_CSUM_TCPv6_Tx | IFCAP_CSUM_UDPv6_Tx;
#endif
	if (un->un_flags & ~URE_FLAG_VER_4C00) {
		ifp->if_capabilities |= IFCAP_CSUM_IPv4_Rx |
		    IFCAP_CSUM_TCPv4_Rx | IFCAP_CSUM_UDPv4_Rx |
		    IFCAP_CSUM_TCPv6_Rx | IFCAP_CSUM_UDPv6_Rx;
	}
	struct ethercom *ec = usbnet_ec(un);
	ec->ec_capabilities = ETHERCAP_VLAN_MTU;
#ifdef notyet
	ec->ec_capabilities |= ETHERCAP_JUMBO_MTU;
#endif

	unm.un_mii_phyloc = un->un_phyno;
	usbnet_attach_ifp(un, IFF_SIMPLEX | IFF_BROADCAST | IFF_MULTICAST,
	    0, &unm);
}

static void
ure_uno_rx_loop(struct usbnet *un, struct usbnet_chain *c, uint32_t total_len)
{
	struct ifnet *ifp = usbnet_ifp(un);
	uint8_t *buf = c->unc_buf;
	uint16_t pkt_len = 0;
	uint16_t pkt_count = 0;
	struct ure_rxpkt rxhdr;

	do {
		if (total_len < sizeof(rxhdr)) {
			DPRINTF(("too few bytes left for a packet header\n"));
			if_statinc(ifp, if_ierrors);
			return;
		}

		buf += roundup(pkt_len, 8);

		memcpy(&rxhdr, buf, sizeof(rxhdr));
		total_len -= sizeof(rxhdr);

		pkt_len = le32toh(rxhdr.ure_pktlen) & URE_RXPKT_LEN_MASK;
		DPRINTFN(4, ("next packet is %d bytes\n", pkt_len));
		if (pkt_len > total_len) {
			DPRINTF(("not enough bytes left for next packet\n"));
			if_statinc(ifp, if_ierrors);
			return;
		}

		total_len -= roundup(pkt_len, 8);
		buf += sizeof(rxhdr);

		usbnet_enqueue(un, buf, pkt_len - ETHER_CRC_LEN,
			       ure_rxcsum(ifp, &rxhdr), 0, 0);

		pkt_count++;
		
	} while (total_len > 0);

	if (pkt_count)
		rnd_add_uint32(usbnet_rndsrc(un), pkt_count);
}

static int
ure_rxcsum(struct ifnet *ifp, struct ure_rxpkt *rp)
{
	int enabled = ifp->if_csum_flags_rx, flags = 0;
	uint32_t csum, misc;

	if (enabled == 0)
		return 0;

	csum = le32toh(rp->ure_csum);
	misc = le32toh(rp->ure_misc);

	if (csum & URE_RXPKT_IPV4_CS) {
		flags |= M_CSUM_IPv4;
		if (csum & URE_RXPKT_TCP_CS)
			flags |= M_CSUM_TCPv4;
		if (csum & URE_RXPKT_UDP_CS)
			flags |= M_CSUM_UDPv4;
	} else if (csum & URE_RXPKT_IPV6_CS) {
		flags = 0;
		if (csum & URE_RXPKT_TCP_CS)
			flags |= M_CSUM_TCPv6;
		if (csum & URE_RXPKT_UDP_CS)
			flags |= M_CSUM_UDPv6;
	}

	flags &= enabled;
	if (__predict_false((flags & M_CSUM_IPv4) &&
	    (misc & URE_RXPKT_IP_F)))
		flags |= M_CSUM_IPv4_BAD;
	if (__predict_false(
	   ((flags & (M_CSUM_TCPv4 | M_CSUM_TCPv6)) && (misc & URE_RXPKT_TCP_F))
	|| ((flags & (M_CSUM_UDPv4 | M_CSUM_UDPv6)) && (misc & URE_RXPKT_UDP_F))
	))
		flags |= M_CSUM_TCP_UDP_BAD;

	return flags;
}

static unsigned
ure_uno_tx_prepare(struct usbnet *un, struct mbuf *m, struct usbnet_chain *c)
{
	struct ure_txpkt txhdr;
	uint32_t frm_len = 0;
	uint8_t *buf = c->unc_buf;

	if ((unsigned)m->m_pkthdr.len > un->un_tx_bufsz - sizeof(txhdr))
		return 0;

	/* header */
	txhdr.ure_pktlen = htole32(m->m_pkthdr.len | URE_TXPKT_TX_FS |
	    URE_TXPKT_TX_LS);
	txhdr.ure_csum = htole32(ure_txcsum(m));
	memcpy(buf, &txhdr, sizeof(txhdr));
	buf += sizeof(txhdr);
	frm_len = sizeof(txhdr);

	/* packet */
	m_copydata(m, 0, m->m_pkthdr.len, buf);
	frm_len += m->m_pkthdr.len;

	DPRINTFN(2, ("tx %d bytes\n", frm_len));

	return frm_len;
}

/*
 * We need to calculate L4 checksum in software, if the offset of
 * L4 header is larger than 0x7ff = 2047.
 */
static uint32_t
ure_txcsum(struct mbuf *m)
{
	struct ether_header *eh;
	int flags = m->m_pkthdr.csum_flags;
	uint32_t data = m->m_pkthdr.csum_data;
	uint32_t reg = 0;
	int l3off, l4off;
	uint16_t type;

	if (flags == 0)
		return 0;

	if (__predict_true(m->m_len >= (int)sizeof(*eh))) {
		eh = mtod(m, struct ether_header *);
		type = eh->ether_type;
	} else
		m_copydata(m, offsetof(struct ether_header, ether_type),
		    sizeof(type), &type);
	switch (type = htons(type)) {
	case ETHERTYPE_IP:
	case ETHERTYPE_IPV6:
		l3off = ETHER_HDR_LEN;
		break;
	case ETHERTYPE_VLAN:
		l3off = ETHER_HDR_LEN + ETHER_VLAN_ENCAP_LEN;
		break;
	default:
		return 0;
	}

	if (flags & (M_CSUM_TCPv4 | M_CSUM_UDPv4)) {
		l4off = l3off + M_CSUM_DATA_IPv4_IPHL(data);
		if (__predict_false(l4off > URE_L4_OFFSET_MAX)) {
			in_undefer_cksum(m, l3off, flags);
			return 0;
		}
		reg |= URE_TXPKT_IPV4_CS;
		if (flags & M_CSUM_TCPv4)
			reg |= URE_TXPKT_TCP_CS;
		else
			reg |= URE_TXPKT_UDP_CS;
		reg |= l4off << URE_L4_OFFSET_SHIFT;
	}
#ifdef INET6
	else if (flags & (M_CSUM_TCPv6 | M_CSUM_UDPv6)) {
		l4off = l3off + M_CSUM_DATA_IPv6_IPHL(data);
		if (__predict_false(l4off > URE_L4_OFFSET_MAX)) {
			in6_undefer_cksum(m, l3off, flags);
			return 0;
		}
		reg |= URE_TXPKT_IPV6_CS;
		if (flags & M_CSUM_TCPv6)
			reg |= URE_TXPKT_TCP_CS;
		else
			reg |= URE_TXPKT_UDP_CS;
		reg |= l4off << URE_L4_OFFSET_SHIFT;
	}
#endif
	else if (flags & M_CSUM_IPv4)
		reg |= URE_TXPKT_IPV4_CS;

	return reg;
}

#ifdef _MODULE
#include "ioconf.c"
#endif

USBNET_MODULE(ure)
